/****************************************************************************
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

#include "stl.h"
#include "cms.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "bmu_api.h"

static const char * BmuControllerStateStr[] = {
   MDMVS_INIT,
   MDMVS_IDLE,
   MDMVS_SLEEP,
   MDMVS_CHARGEINIT,
   MDMVS_PRECHARGE,
   MDMVS_FASTCHARGE,
   MDMVS_TOPOFF,
   MDMVS_CHARGESUSPENDED,
   MDMVS_DISCHARGE,
   MDMVS_ETERNALSLEEP,
   MDMVS_FORCEDDISCHARGE
};

static const char * BatteryLifeTestStateStr[] = {
   MDMVS_IDLE,
   MDMVS_BEGIN,
   MDMVS_PTD,
   MDMVS_STARTCHARGE,
   MDMVS_HWIMP,
   MDMVS_MONITORCHARGE,
   MDMVS_STARTSWIMP,
   MDMVS_COMPLSWIMP,
   MDMVS_DISCHARGE,
   MDMVS_COMPLETE,
};

CmsRet stl_batteryManagementObject(_BatteryManagementObject *obj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE. This handler function des not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_batteryManagementConfigurationObject(_BatteryManagementConfigurationObject *obj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE. This handler function des not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_batteryManagementStatusObject(_BatteryManagementStatusObject *obj,
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   BmuStatus_type bmuStatus;
   char dateTimeBuf[BUFLEN_64];

   if (obj) {
      ret = BmuMsg_send(mdmLibCtx.msgHandle, mdmLibCtx.eid, CMS_MSG_BMU_CONTROLLER_STATUS_GET, 0, &bmuStatus);
      if (CMSRET_SUCCESS == ret) {
         if (NULL == obj->version) {
            CMSMEM_REPLACE_STRING_FLAGS(obj->version, bmuStatus.Version, mdmLibCtx.allocFlags);
            cmsTms_getXSIDateTime(bmuStatus.BuildDateTime, dateTimeBuf, sizeof(dateTimeBuf));
            CMSMEM_REPLACE_STRING_FLAGS(obj->buildDateTime, dateTimeBuf, mdmLibCtx.allocFlags);
         }
         obj->operatingOnBattery = bmuStatus.OperatingOnBattery;
         if (bmuStatus.State <= kBmuForcedDischarge) {
            CMSMEM_REPLACE_STRING_FLAGS(obj->state, BmuControllerStateStr[bmuStatus.State], mdmLibCtx.allocFlags);
         }
         obj->numberOfPresentBatteries = bmuStatus.NumberOfPresentBatteries;
         obj->inputVoltage = bmuStatus.InputVoltage;
         obj->temperature = bmuStatus.Temperature;
         obj->estimatedMinutesRemaining = bmuStatus.EstimatedMinutesRemaining;
         obj->batteryCurrent = bmuStatus.BatteryCurrent;
         obj->upsSecondsOnBattery = bmuStatus.UpsSecondsOnBattery;
         ret = CMSRET_SUCCESS;
      }
      else {
         ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
      }
   }
   return ret;
}

CmsRet stl_batteryManagementBatteryStatusObject(_BatteryManagementBatteryStatusObject *obj,
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   BatteryStatus_type batteryStatus;
   char dateTimeBuf[BUFLEN_64];

   if (obj) {
      ret = BmuMsg_send(mdmLibCtx.msgHandle, mdmLibCtx.eid, CMS_MSG_BMU_BATTERY_STATUS_GET, obj->index, &batteryStatus);
      if (CMSRET_SUCCESS == ret) {
         obj->batteryPresent = batteryStatus.BatteryPresent;
         obj->batteryValid = batteryStatus.BatteryValid;
         obj->batteryBad = batteryStatus.BatteryBad;
         obj->batterySelected = batteryStatus.BatterySelected;
         obj->batteryFullyCharged = batteryStatus.BatteryFullyCharged;
         obj->batteryChargeLow = batteryStatus.BatteryChargeLow;
         obj->batteryChargeLowPercent = batteryStatus.BatteryChargeLowPercent;
         obj->batteryChargeDepleted = batteryStatus.BatteryChargeDepleted;
         obj->batteryChargeStateUnknown = batteryStatus.BatteryChargeStateUnknown;
         obj->batteryChargeCapacity = batteryStatus.BatteryChargeCapacity;
         obj->batteryActualCapacity = batteryStatus.BatteryActualCapacity;
         obj->batteryFullChargeVoltage = batteryStatus.BatteryFullChargeVoltage;
         obj->batteryDepletedVoltage = batteryStatus.BatteryDepletedVoltage;
         obj->batteryMeasuredVoltage = batteryStatus.BatteryMeasuredVoltage;
         obj->batteryPercentCharge = batteryStatus.BatteryPercentCharge;
         obj->batteryEstimatedMinutesRemaining = batteryStatus.BatteryEstimatedMinutesRemaining;
         obj->batteryTemperature = batteryStatus.BatteryTemperature;
         obj->batteryLifeTestCount = batteryStatus.BatteryLifeTestCount;
         cmsTms_getXSIDateTime(batteryStatus.BatteryLastLifeTest, dateTimeBuf, sizeof(dateTimeBuf));
         CMSMEM_REPLACE_STRING_FLAGS(obj->batteryLastLifeTest, dateTimeBuf, mdmLibCtx.allocFlags);
         cmsTms_getXSIDateTime(batteryStatus.BatteryNextLifeTest, dateTimeBuf, sizeof(dateTimeBuf));
         CMSMEM_REPLACE_STRING_FLAGS(obj->batteryNextLifeTest, dateTimeBuf, mdmLibCtx.allocFlags);
         if (batteryStatus.BatteryLifeTestState <= kLTStateComplete) {
            CMSMEM_REPLACE_STRING_FLAGS(obj->batteryLifeTestState, BatteryLifeTestStateStr[batteryStatus.BatteryLifeTestState], mdmLibCtx.allocFlags);
         }
         obj->batteryStateofHealth = batteryStatus.BatteryStateofHealth;
         ret = CMSRET_SUCCESS;
      }
      else {
         ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
      }
   }
   return ret;
}

CmsRet stl_batteryManagementBatteryNonVolObject(_BatteryManagementBatteryNonVolObject *obj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE. This handler function des not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif
