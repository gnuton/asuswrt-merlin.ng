/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard 

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
 *
 ************************************************************************/


#ifdef  DMP_BASELINE_1


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_atm.h"
#include "rut_util.h"
#include "rut_iptables.h"
#include "rut_wan.h"
#include "rut_pmap.h"
#include "rut_upnp.h"
#include "rut_route.h"
#ifdef SUPPORT_POLICYROUTING
#include "rut_policyrouting.h"
#endif
#include "rut_dnsproxy.h"


CmsRet rcl_l3ForwardingEntryObject( _L3ForwardingEntryObject *newObj,
                                 const _L3ForwardingEntryObject *currObj,
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   /*
    * Update forwardNumberOfEntries on successful add or delete.
    */
   if (ADD_NEW(newObj, currObj))
   {
      rut_modifyNumL3Forwarding(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rut_modifyNumL3Forwarding(iidStack, -1);
   }

   /* add and enable static route, or enable existing static route */
   if  ((newObj != NULL && newObj->enable && currObj == NULL) ||                       /* startup */
         (newObj != NULL && newObj->enable && currObj != NULL) ||                      /* add new route */
         (newObj != NULL && newObj->enable && currObj != NULL && currObj->enable))     /* reactivate an existing route */
   {
      /* For static route */
      if (newObj->X_BROADCOM_COM_PolicyRoutingName == NULL)
      {
         if ((ret = rutRt_addStaticRouteAction(newObj->destIPAddress, 
                                               newObj->destSubnetMask,
                                               newObj->gatewayIPAddress,
                                               newObj->interface,
                                               newObj->forwardingMetric)) != CMSRET_SUCCESS)
         {
            return ret;
         }
      }
#ifdef SUPPORT_POLICYROUTING
      /* For policy route */
      else
      {
         if ((ret = rut_isValidPolicyRouting_igd(newObj, *iidStack)) == CMSRET_SUCCESS && 
              rut_isWanInterfaceUp(newObj->interface, TRUE))
         {
            UBOOL8 fromPolicyRoute = TRUE;
            ret = rutRt_addPolicyRouting(fromPolicyRoute,
                                         newObj->X_BROADCOM_COM_PolicyRoutingName,
                                         newObj->X_BROADCOM_COM_SourceIfName, 
                                         newObj->sourceIPAddress, 
                                         newObj->gatewayIPAddress, 
                                         newObj->interface);
         }
      }
#endif

   }

   /* remove ifc, or disable ifc*/
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /* For static route */
      if (currObj->X_BROADCOM_COM_PolicyRoutingName == NULL)
      {
         ret = rutRt_deleteStaticRouteAction(currObj->destIPAddress, 
                                             currObj->destSubnetMask,
                                             currObj->gatewayIPAddress,
                                             currObj->interface);
      }
#ifdef SUPPORT_POLICYROUTING
      /* For policy route */
      else
      {
         UBOOL8 fromPolicyRoute = TRUE;
         /* Remove this rule from the system (action).  
         * NOTE: Ignore the return code since it might be not in the rt_tables if the WAN interface is not up
         */
         rutRt_deletePolicyRouting(fromPolicyRoute, currObj->X_BROADCOM_COM_PolicyRoutingName);
      }
#endif
   }
   
   return ret;

}



CmsRet rcl_l3ForwardingObject( _L3ForwardingObject *newObj __attribute__((unused)),
                const _L3ForwardingObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   CmsRet ret = CMSRET_SUCCESS;
   
   cmsLog_debug("Enter");

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }
   
   if (ADD_NEW(newObj, currObj))
   {
      /* no action on startup.  just return. */
      return ret;
   }   


   if (!newObj || !currObj)
   {
      /* if one of them is NULL, no action needed, just return */
      return ret;
   }
   
   /* If the list has been changed (only from user apps), it will reevaluate this list and
   * try to find the  WAN interface as active default gateway from the list.
   */
   if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_DefaultConnectionServices, 
      currObj->X_BROADCOM_COM_DefaultConnectionServices))
   {   
      char activeDefaultGateway[CMS_IFNAME_LENGTH]={0};

      /* Fetch the active default gateway.  If none found, activeDefaultGateway is NULL
      * and system has no active default gateway. 
      */
      rutRt_fetchActiveDefaultGateway(newObj->X_BROADCOM_COM_DefaultConnectionServices, activeDefaultGateway);

      if (activeDefaultGateway[0] == '\0')
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_ActiveDefaultGateway);
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_ActiveDefaultGateway, 
            activeDefaultGateway, mdmLibCtx.allocFlags);
      }            
   }

   /* Action only - config system default gateway.
   * Can be continue from above code OR from rutRt_doSystemDefaultGateway where
   * the system has a WAN connection change.
   */
   if (newObj->X_BROADCOM_COM_ActiveDefaultGateway)
   {
      if ((ret = rutRt_configActiveDefaultGateway(newObj->X_BROADCOM_COM_ActiveDefaultGateway)) == CMSRET_SUCCESS)
      {
         cmsLog_debug("Done setting %s as system default gateway", newObj->X_BROADCOM_COM_ActiveDefaultGateway);
#ifdef SUPPORT_UPNP
         /* just restart upnp */
         if (rut_isUpnpEnabled())
         {
            CmsRet r2;
           
            if ((r2 = rut_restartUpnpWithWanIfc(newObj->X_BROADCOM_COM_ActiveDefaultGateway)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to restart upnp. error=%d", r2);
            }
         }
#endif /* SUPPORT_UPNP */  
      }
      else
      {
          cmsLog_error("Fail to set %s as system default gateway", newObj->X_BROADCOM_COM_ActiveDefaultGateway);
      }
   }
   else /* newObj->X_BROADCOM_COM_ActiveDefaultGateway is empty */
   {
      if (currObj->X_BROADCOM_COM_ActiveDefaultGateway)
      {
         /*newObj->X_BROADCOM_COM_ActiveDefaultGateway is NULL and 
         * currObj->X_BROADCOM_COM_ActiveDefaultGateway is not NULL,
         * just delete the default gateway from system (the WAN interface is down or deleted)
         */
         rut_doSystemAction("rut", "route del default 2>/dev/null");
      }
      else
      {
         cmsLog_debug("No action on default gateway.");
      }
   }

   cmsLog_debug("Exit, ret=%d", ret);
   
   return ret;
   
}




#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
CmsRet rcl_ipv6L3ForwardingObject( _IPv6L3ForwardingObject *newObj __attribute__((unused)),
                const _IPv6L3ForwardingObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   
   CmsRet ret, r2;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   if (newObj != NULL && currObj == NULL)
   {
      /* for single instance object, this is the startup condition */
      cmsLog_debug("start up condition");

      /* no action to take */
   }
   else if (newObj != NULL && currObj != NULL)
   {
      /* edit curr instance */
      cmsLog_debug("new->defaultConnectionService=%s, old->defaultConnectionService=%s\n",
                    newObj->defaultConnectionService, currObj->defaultConnectionService);

      if (IS_EMPTY_STRING(newObj->defaultConnectionService) && 
          !IS_EMPTY_STRING(currObj->defaultConnectionService))
      {
         /* Configure the system with no default gateway */
         if ((r2 = rutWan_configIPv6DfltGateway(FALSE, currObj->defaultConnectionService)) != CMSRET_SUCCESS)
         {
            cmsLog_debug("rutWan_configIPv6DfltGateway returns error. ret=%d", r2);
         }
      }
      else if (IS_EMPTY_STRING(currObj->defaultConnectionService) &&
               !IS_EMPTY_STRING(newObj->defaultConnectionService))
      {
         if (rut_isWanInterfaceUp(newObj->defaultConnectionService, FALSE))
         {
            /* Add the default gateway */
            if ((ret = rutWan_configIPv6DfltGateway(TRUE, newObj->defaultConnectionService)) != CMSRET_SUCCESS)
            {
               cmsLog_debug("rutWan_configIPv6DfltGateway returns error. ret=%d", ret);
            }
         }
      }
      else if (!IS_EMPTY_STRING(currObj->defaultConnectionService) &&
               !IS_EMPTY_STRING(newObj->defaultConnectionService))
      {
         /* Change the default gateway from one connection to the other */
         if ((ret = rutWan_configIPv6DfltGateway(FALSE, currObj->defaultConnectionService)) != CMSRET_SUCCESS)
         {
            cmsLog_debug("rutWan_configIPv6DfltGateway returns error. ret=%d", ret);
         }

         if (rut_isWanInterfaceUp(newObj->defaultConnectionService, FALSE))
         {
            if ((ret = rutWan_configIPv6DfltGateway(TRUE, newObj->defaultConnectionService)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutWan_configIPv6DfltGateway returns error. ret=%d", ret);
            }
         }
      }
   }

   return CMSRET_SUCCESS;

}  /* End of rcl_ipv6L3ForwardingObject() */

CmsRet rcl_ipv6L3ForwardingEntryObject( _IPv6L3ForwardingEntryObject *newObj __attribute__((unused)),
                const _IPv6L3ForwardingEntryObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   char cmd[BUFLEN_264];
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   /* add and enable static route, or enable existing static route */
   if  ((newObj != NULL && newObj->enable && currObj == NULL) || /* startup */
        (newObj != NULL && newObj->enable && currObj != NULL) || /* add new route */
        (newObj != NULL && newObj->enable && currObj != NULL && currObj->enable))  /* reactivate an existing route */
   {
      char str[BUFLEN_264];
      FILE* errFs = NULL;

      cmsLog_debug("Adding ipv6 static route entry: addr=%s, gtwy=%s, wanif=%s, metric=%d",
                   newObj->destIPv6Address, newObj->gatewayIPv6Address,
                   newObj->interface, newObj->forwardingMetric);

      if (IS_EMPTY_STRING(newObj->destIPv6Address) ||
          (IS_EMPTY_STRING(newObj->gatewayIPv6Address) &&
           IS_EMPTY_STRING(newObj->interface)))
      {
         cmsLog_error("Invalid ipv6 static route");
         return CMSRET_INVALID_ARGUMENTS;
      }

      /* start static route command */
      if ( newObj->forwardingMetric == -1 )
      {
         sprintf(cmd, "ip -6 ro add %s ", newObj->destIPv6Address);
      }
      else
      {
         sprintf(cmd, "ip -6 ro add %s metric %d", newObj->destIPv6Address, 
                                                   newObj->forwardingMetric);
      }

      if (!IS_EMPTY_STRING(newObj->gatewayIPv6Address))
      {
         strcat(cmd, " via ");
         strcat(cmd, newObj->gatewayIPv6Address);
      }
      if (!IS_EMPTY_STRING(newObj->interface))
      {
         strcat(cmd, " dev ");
         strcat(cmd, newObj->interface);
      }
      strcat(cmd, " 2> /var/addrt");
      rut_doSystemAction("rcl_ipv6l3forward", cmd);

      str[0] = '\0';
      errFs = fopen("/var/addrt", "r");
      if (errFs != NULL ) 
      {
         if (fgets(str, BUFLEN_264, errFs) == NULL)
         {
            cmsLog_debug("fgets return null");
         }
         fclose(errFs);
         unlink("/var/addrt");
      }
       /* if error is not the existence of route ("file exists") */
      if ( str[0] != '\0' && strstr(str, "exist") == NULL )
      {
         cmsLog_error("Error = %s", str);
         ret = CMSRET_REQUEST_DENIED;
      }
   }

   /* remove ifc, or disable ifc*/
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting ipv6 static route entry: addr=%s, gtwy=%s, wanif=%s, metric=%d",
                   currObj->destIPv6Address, currObj->gatewayIPv6Address,
                   currObj->interface, currObj->forwardingMetric);

      if (IS_EMPTY_STRING(currObj->destIPv6Address))
      {
         cmsLog_error("Invalid ipv6 static route");
         return CMSRET_INVALID_ARGUMENTS;
      }

      sprintf(cmd, "ip -6 ro del %s", currObj->destIPv6Address);
      strcat(cmd, " 2> /var/rmrt");
      rut_doSystemAction("rcl_ipv6l3forward", cmd);
   }
   
   return ret;

}  /* End of rcl_ipv6L3ForwardingEntryObject() */ 

#endif

#endif  /* DMP_BASELINE_1 */

