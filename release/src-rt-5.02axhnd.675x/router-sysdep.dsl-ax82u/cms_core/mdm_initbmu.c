/***********************************************************************
 *
 *  Copyright (c) 2014  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2014:proprietary:standard

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

#if defined(DMP_X_BROADCOM_COM_BMU_1)

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"


CmsRet mdm_addDefaultBmuObject(void)
{
   BatteryManagementBatteryStatusObject *batteryStatusObj=NULL;
   BatteryManagementBatteryNonVolObject *batteryNonVolObj=NULL;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   int i;

   /* look for InternetGatewayDevice.X_BROADCOM_COM_BMU.BatteryStatus.1 */
   if(mdm_getNextObject(MDMOID_BATTERY_MANAGEMENT_BATTERY_STATUS, &iidStack, (void **)&batteryStatusObj) == CMSRET_SUCCESS)
   {
      /* BatteryStatus object already created nothing to be done */
      mdm_freeObject((void **)&batteryStatusObj);
   }
   else
   {
      /* Add X_BROADCOM_COM_BMU.BatteryStatus.{i}. */
      for ( i = 0; i < 2; i++)
      {                       
         cmsLog_notice("Creating BatteryStatus.%d sub-tree", i+1);
         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_BATTERY_MANAGEMENT_BATTERY_STATUS;
         PUSH_INSTANCE_ID(&pathDesc.iidStack, i+1);

         if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) == CMSRET_SUCCESS)
         {
            iidStack = pathDesc.iidStack;            
            if ((ret = mdm_getObject(MDMOID_BATTERY_MANAGEMENT_BATTERY_STATUS, &iidStack, (void **)&batteryStatusObj)) == CMSRET_SUCCESS)
            {
               batteryStatusObj->index = i;
               ret = mdm_setObject((void **) &batteryStatusObj, &iidStack, FALSE);
            }
         }
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set batteryStatusObj, ret = %d", ret);
         }
      }
   }

   /* look for InternetGatewayDevice.X_BROADCOM_COM_BMU.BatteryNonVol.1 */
   INIT_INSTANCE_ID_STACK(&iidStack);
   if(mdm_getNextObject(MDMOID_BATTERY_MANAGEMENT_BATTERY_NON_VOL, &iidStack, (void **)&batteryNonVolObj) == CMSRET_SUCCESS)
   {
      /* batteryNonVol object already created nothing to be done */
      mdm_freeObject((void **)&batteryNonVolObj);
   }
   else
   {
      /* Add X_BROADCOM_COM_BMU.batteryNonVol.{i}. */
      for ( i = 0; i < 2; i++)
      {                       
         cmsLog_notice("Creating batteryNonVol.%d sub-tree", i+1);
         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_BATTERY_MANAGEMENT_BATTERY_NON_VOL;
         PUSH_INSTANCE_ID(&pathDesc.iidStack, i+1);

         if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) == CMSRET_SUCCESS)
         {
            iidStack = pathDesc.iidStack;            
            if ((ret = mdm_getObject(MDMOID_BATTERY_MANAGEMENT_BATTERY_NON_VOL, &iidStack, (void **) &batteryNonVolObj)) == CMSRET_SUCCESS)
            {
               batteryNonVolObj->index = i;
               ret = mdm_setObject((void **) &batteryNonVolObj, &iidStack, FALSE);
            }
         }
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set batteryNonVolObj, ret = %d", ret);
         }
      }
   }

   return  CMSRET_SUCCESS;  
}

#endif
