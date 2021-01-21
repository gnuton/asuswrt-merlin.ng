/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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

#ifdef DMP_X_BROADCOM_COM_SECURITY_1

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




CmsRet rcl_dev2MacFilterObject( _Dev2MacFilterObject *newObj,
                const _Dev2MacFilterObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   Dev2MacFilterCfgObject *macfilterCfg = NULL;
   char ifName[CMS_IFNAME_LENGTH]={0};

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   if (newObj != NULL && newObj->enable)
   {
      cmsLog_debug("Changing mac filter policy: policy=%s", newObj->policy);

      /* Mac filter policy can be changed only if there is no mac filter entry */
      if ((currObj != NULL) &&
          (cmsObj_getNextInSubTree(MDMOID_DEV2_MAC_FILTER_CFG,
                                   iidStack, &iidStack1,
                                   (void **)&macfilterCfg) == CMSRET_SUCCESS))
      {
         cmsLog_error("mac filter policy cannot be changed since there is a rule entry");
         cmsObj_free((void **) &macfilterCfg);
         return CMSRET_INTERNAL_ERROR;
      }

      /* Get WAN interface name*/
      qdmIntf_getIntfnameFromFullPathLocked_dev2(newObj->IPInterface, ifName, sizeof(ifName));

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
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting mac filter Obj");

      /* Remove rule to block all traffic for the interface if the policy is BLOCKED */               
      if (cmsUtl_strcmp(currObj->policy, MDMVS_BLOCKED) == 0)
      {
         /* Get WAN interface name*/
         qdmIntf_getIntfnameFromFullPathLocked_dev2(currObj->IPInterface, ifName, sizeof(ifName));
         rutEbt_changeMacFilterPolicy(ifName, TRUE);
      }
   }

   return ret;
}


CmsRet rcl_dev2MacFilterCfgObject( _Dev2MacFilterCfgObject *newObj,
                const _Dev2MacFilterCfgObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2MacFilterObject *macFilterObj = NULL;
   Dev2MacFilterCfgObject *macFilterCfg = NULL;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack parentIidStack = *iidStack;
   char ifName[CMS_IFNAME_LENGTH]={0};
   char policy[BUFLEN_32]={0};
   
   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));


   parentIidStack = *iidStack;

   if ((ret = cmsObj_getAncestor(MDMOID_DEV2_MAC_FILTER,
                                 MDMOID_DEV2_MAC_FILTER_CFG,
                                 &parentIidStack, (void **) &macFilterObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get MacFilterObject. ret=%d", ret);
      return CMSRET_INTERNAL_ERROR;
   }

   /* Get WAN interface name and policy out of macFilterObj, then free it */
   qdmIntf_getIntfnameFromFullPathLocked_dev2(macFilterObj->IPInterface, ifName, sizeof(ifName));
   cmsUtl_strncpy(policy, macFilterObj->policy, sizeof(policy));
   cmsObj_free((void **) &macFilterObj);


   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Enabling mac filter feature with policy/ifName: %s/%s", policy, ifName);

      if ((!IS_EMPTY_STRING(newObj->sourceMAC) && cmsUtl_isValidMacAddress(newObj->sourceMAC) == FALSE) ||
          (!IS_EMPTY_STRING(newObj->destinationMAC) && cmsUtl_isValidMacAddress(newObj->destinationMAC) == FALSE) )
      {
         cmsLog_error("Invalid mac address");
         return CMSRET_INVALID_PARAM_VALUE;		
      }

      while (cmsObj_getNextInSubTree(MDMOID_DEV2_MAC_FILTER_CFG, &parentIidStack, &iidStack1,
                                              (void **)&macFilterCfg)  == CMSRET_SUCCESS)
      {
         if (cmsMdm_compareIidStacks(&iidStack1, iidStack))
         {
            if (cmsUtl_strcmp(macFilterCfg->sourceMAC, newObj->sourceMAC) == 0 &&
                cmsUtl_strcmp(macFilterCfg->direction, newObj->direction) == 0 &&
                cmsUtl_strcmp(macFilterCfg->destinationMAC, newObj->destinationMAC) == 0 &&
                cmsUtl_strcmp(macFilterCfg->protocol, newObj->protocol) == 0 )
            {
               cmsLog_error("filter rule exits already");  
               cmsObj_free((void **) &macFilterCfg);	
               return CMSRET_INVALID_ARGUMENTS;
            }
         }

         cmsObj_free((void **) &macFilterCfg);
      }

      rutEbt_addMacFilter_raw(
           newObj->protocol
         , newObj->direction
         , newObj->sourceMAC
         , newObj->destinationMAC
         , ifName, policy, TRUE);

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
         rutEbt_addMacFilter_raw(
              currObj->protocol
            , currObj->direction
            , currObj->sourceMAC
            , currObj->destinationMAC
            , ifName, policy, FALSE);
      }
   }   

   return ret;
}

#endif    /* DMP_X_BROADCOM_COM_SECURITY_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */

