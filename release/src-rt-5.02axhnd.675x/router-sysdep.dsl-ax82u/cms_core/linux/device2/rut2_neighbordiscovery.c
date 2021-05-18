/***********************************************************************
 *
 *  Copyright (c) 2016  Broadcom 
 *  All Rights Reserved
 *
<:label-BRCM:2016:proprietary:standard

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

#ifdef DMP_DEVICE2_NEIGHBORDISCOVERY_1

#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_route.h"
#include "rut2_route.h"
#include "cms_mdm.h"
#include "cms_obj.h"
#include "cms_phl.h"
#include "cms_qdm.h"
#include "cms_util.h"


CmsRet rutNd_activateNeighborDiscovery_dev2(const char *ipIntfFullPath)
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 found = FALSE;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2NeighborDiscoveryInterfaceSettingObject *ndIntfSettingObj=NULL;


   cmsLog_debug("Enter: ipIntfFullPath=%s", ipIntfFullPath);


   while (!found && (cmsObj_getNext(MDMOID_DEV2_NEIGHBOR_DISCOVERY_INTERFACE_SETTING,
                        &iidStack, 
                        (void **)&ndIntfSettingObj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(ipIntfFullPath, ndIntfSettingObj->interface))
      {
         found = TRUE;
         break;
      }
      cmsObj_free((void **)&ndIntfSettingObj);
   }

   /* Not found! Add instance for NeighborDiscovery for this interface */
   if (!found)
   {
      INIT_INSTANCE_ID_STACK(&iidStack);
      if ((ret = cmsObj_addInstance(MDMOID_DEV2_NEIGHBOR_DISCOVERY_INTERFACE_SETTING, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Create Dev2NeighborDiscoveryInterfaceSettingObject failed!, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }
      if ((ret=cmsObj_get(MDMOID_DEV2_NEIGHBOR_DISCOVERY_INTERFACE_SETTING, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&ndIntfSettingObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not find newly created instance, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }
      CMSMEM_REPLACE_STRING_FLAGS(ndIntfSettingObj->interface, ipIntfFullPath, mdmLibCtx.allocFlags);
      cmsLog_debug("create NeighborDiscoveryIntfSetting Object for %s", ipIntfFullPath);
   }
   

   if ((ret = cmsObj_set(ndIntfSettingObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Set of NeighborDiscoveryIntfSetting object failed, ret=%d", ret);
   }
   
   cmsObj_free((void **)&ndIntfSettingObj);
   
   return ret;
}

void rutNd_deactivateNeighborDiscovery_dev2(const char *ipIntfFullPath)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack savedIidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2NeighborDiscoveryInterfaceSettingObject *ndIntfSettingObj=NULL;

   cmsLog_debug("Enter: ipIntfFullPath=%s", ipIntfFullPath);

   while ((ret = cmsObj_getNext(MDMOID_DEV2_NEIGHBOR_DISCOVERY_INTERFACE_SETTING,
                        &iidStack, 
                        (void **)&ndIntfSettingObj)) == CMSRET_SUCCESS)
   {
      if (IS_EMPTY_STRING(ndIntfSettingObj->interface) || !cmsUtl_strcmp(ndIntfSettingObj->interface, ipIntfFullPath))
      {
         cmsObj_deleteInstance(MDMOID_DEV2_NEIGHBOR_DISCOVERY_INTERFACE_SETTING, &iidStack);

         /* since we are deleting while traversing, must restore to
          * last good/known instance.
          */
         iidStack = savedIidStack;
      }
      savedIidStack = iidStack;

      cmsObj_free((void **)&ndIntfSettingObj);
   }
   
   cmsLog_debug("Exit");

   return;
}
#endif  /* DMP_DEVICE2_NEIGHBORDISCOVERY_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */
