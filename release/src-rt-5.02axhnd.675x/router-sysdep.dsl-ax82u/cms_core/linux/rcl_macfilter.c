/***********************************************************************
 *
 *  Copyright (c) 2006-2010  Broadcom Corporation
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_ebtables.h"

CmsRet rcl_macFilterObject( _MacFilterObject *newObj __attribute__((unused)),
                const _MacFilterObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

#ifdef DMP_X_BROADCOM_COM_SECURITY_1
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   MacFilterCfgObject *macfilterCfg = NULL;
   WanIpConnObject *wan_ip_conn = NULL;
   char ifName[BUFLEN_32];

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));
   
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if (newObj != NULL && newObj->enable)
   {
      cmsLog_debug("Changing mac filter policy: policy=%s", newObj->policy);

      if (cmsUtl_strcmp(newObj->policy, MDMVS_BLOCKED) && cmsUtl_strcmp(newObj->policy, MDMVS_FORWARD))
      {
         cmsLog_error("mac filter INVALID policy: %s", newObj->policy);
         return CMSRET_INVALID_PARAM_VALUE;
      }
      else
      {
         /* Mac filter policy can be changed only if there is no mac filter entry */
         if((currObj != NULL) && cmsObj_getNextInSubTree(MDMOID_MAC_FILTER_CFG, iidStack, &iidStack1, (void **)&macfilterCfg) == CMSRET_SUCCESS)
         {
            cmsLog_error("mac filter policy cannot be changed since there is a rule entry");
            cmsObj_free((void **) &macfilterCfg);
            return CMSRET_INTERNAL_ERROR;
         }

         iidStack1 = *iidStack;
         if (cmsObj_getAncestor(MDMOID_WAN_IP_CONN, MDMOID_MAC_FILTER, 	
                                                           &iidStack1, (void **) &wan_ip_conn) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get WanIpConnObject. ret=%d", ret);
            return CMSRET_INTERNAL_ERROR;
         }
      
         strncpy(ifName, wan_ip_conn->X_BROADCOM_COM_IfName, sizeof(ifName));
         cmsObj_free((void **) &wan_ip_conn);	
		 
         if (cmsUtl_strcmp(newObj->policy, MDMVS_BLOCKED) == 0)
         {
            /* Add rule to block all traffic for the interface if the policy is BLOCKED */
            rutEbt_changeMacFilterPolicy(ifName, FALSE);
         }
         else
         {
            rutEbt_changeMacFilterPolicy(ifName, TRUE);
         }
      }

   }
#endif
   return ret;
}

CmsRet rcl_macFilterCfgObject( _MacFilterCfgObject *newObj __attribute__((unused)),
                const _MacFilterCfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   
#ifdef DMP_X_BROADCOM_COM_SECURITY_1
   WanIpConnObject *wan_ip_conn = NULL;
   MacFilterObject *macFilterObj = NULL;
   MacFilterCfgObject *macFilterCfg = NULL;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack parentIidStack = *iidStack;
   char ifName[BUFLEN_32], policy[BUFLEN_32];
   
   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));
   
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if (cmsObj_getAncestor(MDMOID_WAN_IP_CONN, MDMOID_MAC_FILTER_CFG, 	
                                                     &parentIidStack, (void **) &wan_ip_conn) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get WanIpConnObject. ret=%d", ret);
      return CMSRET_INTERNAL_ERROR;
   }

   strncpy(ifName, wan_ip_conn->X_BROADCOM_COM_IfName, sizeof(ifName));
   cmsObj_free((void **) &wan_ip_conn);	

   parentIidStack = *iidStack;

   if (cmsObj_getAncestor(MDMOID_MAC_FILTER, MDMOID_MAC_FILTER_CFG, 	
                                                     &parentIidStack, (void **) &macFilterObj) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get MacFilterObject. ret=%d", ret);
      return CMSRET_INTERNAL_ERROR;
   }

   strncpy(policy, macFilterObj->policy, sizeof(policy));
   cmsObj_free((void **) &macFilterObj);	

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Enabling mac filter feature with policy/ifName: %s/%s", policy, ifName);

      if ((cmsUtl_strcmp(newObj->sourceMAC, "\0") != 0 && cmsUtl_isValidMacAddress(newObj->sourceMAC) == FALSE) ||\
           (cmsUtl_strcmp(newObj->destinationMAC, "\0") != 0 && cmsUtl_isValidMacAddress(newObj->destinationMAC) == FALSE) )
      {
         cmsLog_error("Invalid mac address");
         return CMSRET_INVALID_PARAM_VALUE;		
      }

      if (cmsUtl_strcmp(newObj->direction, MDMVS_LAN_TO_WAN) && cmsUtl_strcmp(newObj->direction, MDMVS_WAN_TO_LAN) &&\
           cmsUtl_strcmp(newObj->direction, MDMVS_BOTH) )
      {
         cmsLog_error("Invalid direction: %s", newObj->direction);
         return CMSRET_INVALID_PARAM_VALUE;
      }

      if (cmsUtl_strcmp(newObj->protocol, MDMVS_PPPOE) && cmsUtl_strcmp(newObj->protocol, MDMVS_IPV4) &&\
           cmsUtl_strcmp(newObj->protocol, MDMVS_IPV6) && cmsUtl_strcmp(newObj->protocol, MDMVS_APPLETALK) &&\
           cmsUtl_strcmp(newObj->protocol, MDMVS_IPX) && cmsUtl_strcmp(newObj->protocol, MDMVS_NETBEUI) &&\
           cmsUtl_strcmp(newObj->protocol, MDMVS_IGMP) && cmsUtl_strcmp(newObj->protocol, MDMVS_NONE) )
      {
         cmsLog_error("Invalid protocol");
         return CMSRET_INVALID_PARAM_VALUE;
      }

      while (cmsObj_getNextInSubTree(MDMOID_MAC_FILTER_CFG, &parentIidStack, &iidStack1,
                                              (void **)&macFilterCfg)  == CMSRET_SUCCESS)
      {
         if(cmsMdm_compareIidStacks(&iidStack1, iidStack))
         {
            if (cmsUtl_strcmp(macFilterCfg->sourceMAC, newObj->sourceMAC) == 0 && \
                        cmsUtl_strcmp(macFilterCfg->direction, newObj->direction) == 0 &&\
                        cmsUtl_strcmp(macFilterCfg->destinationMAC, newObj->destinationMAC) == 0 && \
                        cmsUtl_strcmp(macFilterCfg->protocol, newObj->protocol) == 0 )
            {
               cmsLog_error("filter rule exits already");  
               cmsObj_free((void **) &macFilterCfg);	
               return CMSRET_INVALID_ARGUMENTS;
            }
            cmsObj_free((void **) &macFilterCfg);	 
         }
      }

      rutEbt_addMacFilter(newObj, ifName, policy, TRUE);	  
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting mac filter feature");

      /*
       * The set on the object from dalSec_addMacFilter could have failed.
       * In that case, enable would be false, so don't take
       * any action since it was not put there in the first place.
       */
      if(currObj->enable != FALSE)
      {
         rutEbt_addMacFilter(currObj, ifName, policy, FALSE);
      }
   }   
#endif
   return ret;
}


