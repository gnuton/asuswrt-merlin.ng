/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
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


#ifdef DMP_DEVICE2_BASELINE_1


#ifdef DMP_DEVICE2_ROUTING_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_wan.h"
#include "rut_route.h"
#include "rut2_route.h"
#include "rut_upnp.h"
#include "rut_rip.h"


/*!\file rcl2_route.c
 * \brief This file contains Device2 Device.Routing objects related functions.
 *
 */




CmsRet rcl_dev2RoutingObject( _Dev2RoutingObject *newObj __attribute__((unused)),
                const _Dev2RoutingObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* Nothing to do here, just return */
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2RouterObject( _Dev2RouterObject *newObj,
                const _Dev2RouterObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   /* XXX TODO should also add code for updating counts, although I don't
    * know when we would add a second router object.
    */

   cmsLog_debug("Entered:");

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* just set our status to ENABLED.  There is no action associated
       * with the router object itself.
       */
      CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_error("Disabling or deleting Route table is not supported!");
      if (newObj)
      {
         /* somebody disabled this object, so set status to DISABLED.
          * Why would anyone do this?
          * We don't actually disable all routes in this table.
          */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
      }

      return CMSRET_SUCCESS;
   }

   if (newObj && currObj &&
        (newObj->IPv4ForwardingNumberOfEntries != currObj->IPv4ForwardingNumberOfEntries
#ifdef DMP_DEVICE2_IPV6ROUTING_1
          || newObj->IPv6ForwardingNumberOfEntries != currObj->IPv6ForwardingNumberOfEntries
#endif
         ))
   {
      /* if the numberOfEntries is changing, that means an entry is being
       * added or deleted.  Don't bother re-evaluating default gateway,
       * some other action will follow and we will re-evalute then.
       */
      return CMSRET_SUCCESS;
   }


   /*
    * Whenever a set is done, re-evaluate IPv4 and IPv6 default gateway.
    */
   if (newObj && currObj && newObj->enable)
   {
      char defaultGateway[CMS_IFNAME_LENGTH]={0};

      rutRt_selectActiveIpvxDefaultGateway_dev2(CMS_AF_SELECT_IPV4,
                            newObj->X_BROADCOM_COM_DefaultConnectionServices,
                            defaultGateway);

      /* See comments in rutRt_doSystemDefaultGateway */
      if (!IS_EMPTY_STRING(defaultGateway) &&
          ((cmsUtl_strcmp(currObj->X_BROADCOM_COM_ActiveDefaultGateway, defaultGateway)) ||
           (!rutRt_isDefaultGatewayIfNameInRouteTable(defaultGateway))))
      {

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_ActiveDefaultGateway,
                                     defaultGateway, mdmLibCtx.allocFlags);

         if (!IS_EMPTY_STRING(newObj->X_BROADCOM_COM_ActiveDefaultGateway))
         {
            CmsRet r2;

            r2 = rutRt_configActiveDefaultGateway(newObj->X_BROADCOM_COM_ActiveDefaultGateway);
            if (r2 != CMSRET_SUCCESS)
            {
               cmsLog_error("failed to set default gateway of %s",
                     newObj->X_BROADCOM_COM_ActiveDefaultGateway);
               /* complain but keep going */
            }

#ifdef SUPPORT_UPNP
            /* restart upnp */
            if (rut_isUpnpEnabled())
            {
               r2 = rut_restartUpnpWithWanIfc(newObj->X_BROADCOM_COM_ActiveDefaultGateway);
               if (r2 != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to restart upnp. error=%d", r2);
               }
            }
#endif /* SUPPORT_UPNP */
         }
         else
         {
            /* active gateway is empty */
            rut_doSystemAction("rut", "route del default 2>/dev/null");
         }
      }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      memset(defaultGateway, 0, sizeof(defaultGateway));

      rutRt_selectActiveIpvxDefaultGateway_dev2(CMS_AF_SELECT_IPV6,
                         newObj->X_BROADCOM_COM_DefaultIpv6ConnectionServices,
                         defaultGateway);

      if (cmsUtl_strcmp(currObj->X_BROADCOM_COM_ActiveIpv6DefaultGateway, defaultGateway))
      {
         char gwIpAddr[CMS_IPADDR_LENGTH]={0};
         char origin[BUFLEN_32]={0};

         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_ActiveIpv6DefaultGateway,
                                     defaultGateway, mdmLibCtx.allocFlags);

         if (!IS_EMPTY_STRING(newObj->X_BROADCOM_COM_ActiveIpv6DefaultGateway))
         {
            if (qdmRt_getGatewayIpv6AddrByIfNameLocked_dev2(
                               newObj->X_BROADCOM_COM_ActiveIpv6DefaultGateway,
                               gwIpAddr, origin) != CMSRET_SUCCESS)
            {
               cmsLog_notice("could not get gwip by ifname<%s>", 
                             newObj->X_BROADCOM_COM_ActiveIpv6DefaultGateway);
            }
            else
            {
               /* finally we can add the IPv6 default gateway (if origin
                * is not RA since RA routes are added inside the kernel) */
               if (cmsUtl_strcmp(origin, MDMVS_RA))
               {
                  /* should delete previous default gateway if there was one */
                  if (!IS_EMPTY_STRING(currObj->X_BROADCOM_COM_ActiveIpv6DefaultGateway))
                  {
                     rutWan_configIpv6DefaultGateway("del", NULL,
                             currObj->X_BROADCOM_COM_ActiveIpv6DefaultGateway);
                  }

                  rutWan_configIpv6DefaultGateway("add", gwIpAddr,
                          newObj->X_BROADCOM_COM_ActiveIpv6DefaultGateway);
               }
            }

         }
         else
         {
            /* new active gateway is empty, delete previous default gw */
            if (!IS_EMPTY_STRING(currObj->X_BROADCOM_COM_ActiveIpv6DefaultGateway))
            {
               if (qdmRt_getGatewayIpv6AddrByIfNameLocked_dev2(
                                  currObj->X_BROADCOM_COM_ActiveIpv6DefaultGateway,
                                  gwIpAddr, origin) != CMSRET_SUCCESS)
               {
                  cmsLog_notice("could not get gwip by ifname<%s>", 
                                currObj->X_BROADCOM_COM_ActiveIpv6DefaultGateway);
               }
               else
               {
                  /* FIXME: should delete previous default gateway if there was one */

                  /* delete the default gw if it's static */
                  if (!cmsUtl_strcmp(origin, MDMVS_STATIC))
                  {
                     rutWan_configIpv6DefaultGateway("del", NULL,
                               currObj->X_BROADCOM_COM_ActiveIpv6DefaultGateway);
                  }
               }
            }
         }

         /* XXX TODO: also restart UPnP here ? */
      }
#endif  /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */
   }

   cmsLog_debug("Exit:");

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2Ipv4ForwardingObject( _Dev2Ipv4ForwardingObject *newObj,
                const _Dev2Ipv4ForwardingObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Entered:");
   /*
    * Update forwardNumberOfEntries on successful add or delete.
    */
   if (ADD_NEW(newObj, currObj))
   {
      modifyNumRouterIpv4ForwardingEntry(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      modifyNumRouterIpv4ForwardingEntry(iidStack, -1);
   }

   /* add and enable static route, or enable existing static route */
   if  ((newObj != NULL && newObj->enable && currObj == NULL) ||                       /* startup */
         (newObj != NULL && newObj->enable && currObj != NULL) ||                      /* add new route */
         (newObj != NULL && newObj->enable && currObj != NULL && currObj->enable))     /* reactivate an existing route */
   {
      if (IS_EMPTY_STRING(newObj->X_BROADCOM_COM_PolicyRoutingName))
      {
         /* For static route */
         if (newObj->staticRoute && !IS_EMPTY_STRING(newObj->destIPAddress))
         {
            char l3IntfNameBuf[CMS_IFNAME_LENGTH]={0};
            /* convert mdm full path string to queue interface name */
            if (!IS_EMPTY_STRING(newObj->interface) &&
                (ret = qdmIntf_fullPathToIntfnameLocked(newObj->interface, l3IntfNameBuf)) != CMSRET_SUCCESS)
            {
               cmsLog_error("qdmIntf_fullPathToIntfnameLocked on %s ret=%d", newObj->interface, ret);
               return ret;
            }
         
            if ((ret = rutRt_addStaticRouteAction(newObj->destIPAddress, 
                                            newObj->destSubnetMask,
                                            newObj->gatewayIPAddress,
                                            l3IntfNameBuf,
                                            newObj->forwardingMetric)) != CMSRET_SUCCESS)
            {
               return ret;
            }
         }
      }
#if SUPPORT_POLICYROUTING
      /* For policy route */
      else
      {
         char l3IntfNameBuf[CMS_IFNAME_LENGTH]={0};
		
         if (qdmIntf_fullPathToIntfnameLocked(newObj->interface, l3IntfNameBuf) != CMSRET_SUCCESS)
         {
            cmsLog_error("qdmIntf_fullPathToIntfnameLocked on %s", newObj->interface);
         }
         else if ((ret = rut_isValidPolicyRouting_dev2(newObj, *iidStack)) == CMSRET_SUCCESS && 
                   qdmIpIntf_isWanInterfaceUpLocked(l3IntfNameBuf, TRUE))
         {
            UBOOL8 fromPolicyRoute = TRUE;
            ret = rutRt_addPolicyRouting(fromPolicyRoute,
                                         newObj->X_BROADCOM_COM_PolicyRoutingName,
                                         newObj->X_BROADCOM_COM_SourceIfName, 
                                         newObj->X_BROADCOM_COM_SourceIPAddress, 
                                         newObj->gatewayIPAddress,
                                         l3IntfNameBuf);
         }
      }
#endif
   }

   /* remove ifc, or disable ifc*/
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if (IS_EMPTY_STRING(currObj->X_BROADCOM_COM_PolicyRoutingName))
      {
         /* For static route */
         if (currObj->staticRoute && currObj->destIPAddress && currObj->destSubnetMask)
         {
           char l3IntfNameBuf[CMS_IFNAME_LENGTH]={0};
  		
           if (!IS_EMPTY_STRING(currObj->interface) && 
               (ret = qdmIntf_fullPathToIntfnameLocked(currObj->interface, l3IntfNameBuf)) != CMSRET_SUCCESS)
           {
              cmsLog_error("qdmIntf_fullPathToIntfnameLocked on %s", currObj->interface);
           }
           else
           {
              ret = rutRt_deleteStaticRouteAction(currObj->destIPAddress, 
                                                currObj->destSubnetMask,
                                                currObj->gatewayIPAddress,
                                                l3IntfNameBuf);
           }
         }
      }
#if SUPPORT_POLICYROUTING
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


CmsRet rcl_dev2RipObject( _Dev2RipObject *newObj __attribute__((unused)),
                const _Dev2RipObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
#ifdef SUPPORT_RIP
   cmsLog_debug("Entered:");
   
   /* For startup, ripd will be configured after wan interface up*/
   
   /* config or unconfig each entries in the rip interface setting table */
   if (newObj != NULL && currObj != NULL && newObj->enable != currObj->enable)
   {
     cmsLog_debug("newObj->enable:%d, currObj->enable:%d", newObj->enable, currObj->enable);
	 
     if (newObj->enable)
     {
        rutRip_restart_dev2();
     }
     else
     {
        rutRip_stop();
     }
   }
#endif

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2RipIntfSettingObject(  _Dev2RipIntfSettingObject *newObj __attribute__((unused)),
                const _Dev2RipIntfSettingObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
#ifdef SUPPORT_RIP
   cmsLog_debug("Entered:");

   if (ADD_NEW(newObj, currObj))
   {
      modifyNumRipIntfSettingEntry(iidStack, 1);
      return CMSRET_SUCCESS;
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      modifyNumRipIntfSettingEntry(iidStack, -1);
   }
   
   if (newObj && !rutRip_isConfigValid_dev2(newObj))
   {
      cmsLog_error("Invalid rip configuration");
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* Detele an entry or change config, either, should restart ripd */
   rutRip_restart_dev2();
   
#endif

   return CMSRET_SUCCESS;
}
#endif

#ifdef DMP_DEVICE2_IPV6ROUTING_1
/* the IPv6 routing functions should probably go into their own file.  rcl2_route6.c ? */

CmsRet rcl_dev2Ipv6ForwardingObject( _Dev2Ipv6ForwardingObject *newObj,
                const _Dev2Ipv6ForwardingObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   char cmd[BUFLEN_264];
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }
   
   if (ADD_NEW(newObj, currObj))
   {
      modifyNumRouterIpv6ForwardingEntry(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      modifyNumRouterIpv6ForwardingEntry(iidStack, -1);
   }

   /* add and enable static route, or enable existing static route */
   if  ((newObj != NULL && newObj->enable && currObj == NULL) || /* startup */
        (newObj != NULL && newObj->enable && currObj != NULL) || /* add new route */
        (newObj != NULL && newObj->enable && currObj != NULL && currObj->enable))  /* reactivate an existing route */
   {
      char str[BUFLEN_264];
      FILE* errFs = NULL;

      cmsLog_debug("Adding ipv6 static route entry: addr=%s, gtwy=%s, wanif=%s, metric=%d",
                   newObj->destIPPrefix, newObj->nextHop,
                   newObj->interface, newObj->forwardingMetric);

      if (!cmsUtl_strcmp(newObj->origin, MDMVS_RA))
      {
         cmsLog_debug("routing entry of RA, no action needed");
         return ret;
      }

      if (!cmsUtl_strcmp(newObj->origin, MDMVS_STATIC) && IS_EMPTY_STRING(newObj->destIPPrefix))
      {
         cmsLog_debug("routing entry of ::/0, no action needed");
         return ret;
      }

      if (IS_EMPTY_STRING(newObj->destIPPrefix) ||
          (IS_EMPTY_STRING(newObj->nextHop) &&
           IS_EMPTY_STRING(newObj->interface)))
      {
         cmsLog_error("Invalid ipv6 static route");
         return CMSRET_INVALID_ARGUMENTS;
      }

      /* start static route command */
      if ( newObj->forwardingMetric == -1 )
      {
         sprintf(cmd, "ip -6 ro add %s ", newObj->destIPPrefix);
      }
      else
      {
         sprintf(cmd, "ip -6 ro add %s metric %d", newObj->destIPPrefix, 
                                                   newObj->forwardingMetric);
      }

      if (!IS_EMPTY_STRING(newObj->nextHop))
      {
         strcat(cmd, " via ");
         strcat(cmd, newObj->nextHop);
      }
      if (!IS_EMPTY_STRING(newObj->interface))
      {
         char l3IntfNameBuf[CMS_IFNAME_LENGTH]={0};
         if ((ret = qdmIntf_fullPathToIntfnameLocked_dev2(newObj->interface, l3IntfNameBuf)) != CMSRET_SUCCESS)
         {
            cmsLog_error("qdmIntf_fullPathToIntfnameLocked_dev2 on %s ret=%d",
                          newObj->interface, ret);
         }
         else      
         {
            strcat(cmd, " dev ");
            strcat(cmd, l3IntfNameBuf);
         }
      }
      strcat(cmd, " 2> /var/addrt");
      rut_doSystemAction("rcl_dev2Ipv6ForwardingObject", cmd);

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
                   currObj->destIPPrefix, currObj->nextHop,
                   currObj->interface, currObj->forwardingMetric);

      if (!cmsUtl_strcmp(currObj->origin, MDMVS_RA))
      {
         cmsLog_debug("routing entry of RA, no action needed");
         return ret;
      }

      if (!cmsUtl_strcmp(currObj->origin, MDMVS_STATIC) && IS_EMPTY_STRING(currObj->destIPPrefix))
      {
         cmsLog_debug("routing entry of ::/0, no action needed");
         return ret;
      }

      if (IS_EMPTY_STRING(currObj->destIPPrefix))
      {
         cmsLog_error("Invalid ipv6 static route");
         return CMSRET_INVALID_ARGUMENTS;
      }

      sprintf(cmd, "ip -6 ro del %s", currObj->destIPPrefix);
      strcat(cmd, " 2> /var/rmrt");
      rut_doSystemAction("rcl_dev2Ipv6ForwardingObject", cmd);
   }
   
   return ret;

}


CmsRet rcl_dev2RouteInfoObject( _Dev2RouteInfoObject *newObj,
                const _Dev2RouteInfoObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2RouteInfoIntfSettingObject( _Dev2RouteInfoIntfSettingObject *newObj,
                const _Dev2RouteInfoIntfSettingObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused))) 
{
   CmsRet ret = CMSRET_SUCCESS;
   cmsLog_debug("Entered:");
   /*
    * Update IterfaceSettingNumberOfEntries on successful add or delete.
    */
   if (ADD_NEW(newObj, currObj))
   {
      modifyNumRouteInfoIntfSettingEntry(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      modifyNumRouteInfoIntfSettingEntry(iidStack, -1);
   }

   return ret;
}

#endif  /* DMP_DEVICE2_IPV6ROUTING_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */

