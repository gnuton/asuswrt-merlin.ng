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

#include "cms_core.h"
#include "cms_dal.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"

#include "rut_spdsvc.h"

static void rut_checkResultHistory( SpeedServiceObject *spdsvcObj);

CmsRet rutSpdsvc_runSpeedService( SpeedServiceObject *spdsvcObj )
{
   char cmd[BUFLEN_256] = {0};
   char *ptr = cmd;
   int mode;

   /** Use this symbolic link file name to indicate this execution
    * was from CMS.
    */
   ptr += sprintf(cmd, "ss ");

   /* check dataPath */
   if (spdsvcObj->dataPath == NULL || spdsvcObj->dataPath[0] == '\0')
   {
      cmsLog_error("dataPath is not assigned");
      return CMSRET_INVALID_PARAM_VALUE;
   }

   if (spdsvcObj->dataPath[0] == 'H')
      ptr += sprintf(ptr, "client ");
   else
      ptr += sprintf(ptr, "client-sw ");

   /* check the testing mode */
   if (strcmp(spdsvcObj->mode, MDMVS_CLIENT_SEND) == 0)
   {
      ptr += sprintf(ptr, "send ");
      mode = SPDSVC_CLIENT_SEND;
   }
   else if (strcmp(spdsvcObj->mode, MDMVS_CLIENT_BW) == 0)
   {
      ptr += sprintf(ptr, "bw ");
      mode = SPDSVC_CLIENT_BW;
   }
   else
   {
      cmsLog_error("speed test mode is invalid");
      return CMSRET_INVALID_PARAM_VALUE;
   }

   if (mode == SPDSVC_CLIENT_SEND || mode == SPDSVC_CLIENT_BW)
   {
      /* check direction */
      if (spdsvcObj->direction == NULL || spdsvcObj->direction[0] == '\0')
      {
         cmsLog_error("direction is not assigned");
         return CMSRET_INVALID_PARAM_VALUE;
      }

      if (spdsvcObj->direction[0] == 'u')
         ptr += sprintf(ptr, "us ");
      else
         ptr += sprintf(ptr, "ds ");

      /* check server ip port*/
      if (spdsvcObj->serverIpAddr == NULL || spdsvcObj->serverIpAddr[0] == '\0')
      {
         cmsLog_error("invalid server IP address");
         return CMSRET_INVALID_PARAM_VALUE;
      }
      if (spdsvcObj->tcpPort == 0)
      {
         cmsLog_error("invalid TCP port number");
         return CMSRET_INVALID_PARAM_VALUE;
      }
      ptr += sprintf(ptr, "%s %u ",
                     spdsvcObj->serverIpAddr,
                     spdsvcObj->tcpPort);

      ptr += sprintf(ptr, "%u %u %u",
                     spdsvcObj->stepDuration,
                     spdsvcObj->packetLength,
                     spdsvcObj->startingBwKbps);

      /* band width test only parameters: algo, steps, loss rate */
      if (mode == SPDSVC_CLIENT_BW)
      {
         if (cmsUtl_strcmp(spdsvcObj->algorithm, MDMVS_BIN) == 0)
         {
            if (spdsvcObj->latencyTolerancePercentage == -1)
               ptr += sprintf(ptr, " %s %u %u",
                              "bin",
                              spdsvcObj->maxSteps,
                              spdsvcObj->acceptablePercentageLoss);
            else
               ptr += sprintf(ptr, " %s %u %u %d",
                              "bin",
                              spdsvcObj->maxSteps,
                              spdsvcObj->acceptablePercentageLoss,
                              spdsvcObj->latencyTolerancePercentage);
         }
         if (cmsUtl_strcmp(spdsvcObj->algorithm, MDMVS_RAMP) == 0)
         {
            if (spdsvcObj->latencyTolerancePercentage == -1)
               ptr += sprintf(ptr, " %s %u %u",
                              "ramp",
                              spdsvcObj->maxSteps,
                              spdsvcObj->maxLossPercentage);
            else
               ptr += sprintf(ptr, " %s %u %u %d",
                              "ramp",
                              spdsvcObj->maxSteps,
                              spdsvcObj->maxLossPercentage,
                              spdsvcObj->latencyTolerancePercentage);
         }
         else if (cmsUtl_strcmp(spdsvcObj->algorithm, MDMVS_FAST) == 0)
         {
            ptr += sprintf(ptr, " %s %u %u", "fast", spdsvcObj->maxSteps, spdsvcObj->acceptablePercentageLoss);
         }
         else if (cmsUtl_strcmp(spdsvcObj->algorithm, MDMVS_RXRATE) == 0)
         {
            ptr += sprintf(ptr, " rxrate");
         }
      }

      cmsLog_debug("test command:[%s]", cmd);
   }
   else 
   {
      return CMSRET_INVALID_PARAM_VALUE;
   }

   // mark command from CMS and put it into background
   ptr += sprintf(ptr, " fromCMS&");
   rut_doSystemAction("rut", cmd);
   return CMSRET_SUCCESS;
}

void rutSpdsvc_addPreviousTestResult
   (const SpeedServiceObject *currObj,
    SpeedServiceObject *newObj)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _ResultHistoryObject *resultObj = NULL;
   UINT32 flags=OGF_NO_VALUE_UPDATE;
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsObj_addInstance(MDMOID_RESULT_HISTORY, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not create result history entry object, ret=%d", ret);
      return;
   }

   /*
    * Now we need to set some parameter on the result history object.
    */
   if ((ret = cmsObj_get(MDMOID_RESULT_HISTORY, &iidStack, flags, (void **) &resultObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get result history entry object, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_RESULT_HISTORY, &iidStack);
      return;
   }
   resultObj->goodPut = currObj->goodPut;
   resultObj->payloadRate = currObj->payloadRate;
   resultObj->packetLoss = currObj->packetLoss;
   resultObj->avgLatency = currObj->avgLatency;
   resultObj->adjustReceivedRate = currObj->adjustReceivedRate;
   resultObj->receivedTime = currObj->receivedTime;
   resultObj->overhead = currObj->overhead;
   CMSMEM_REPLACE_STRING_FLAGS(resultObj->runTime, newObj->lastRunTime, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(resultObj->direction, currObj->direction, mdmLibCtx.allocFlags);

   if ((ret = cmsObj_set(resultObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not set result history entry object, ret=%d", ret);
      cmsObj_free((void **) &resultObj);
      cmsObj_deleteInstance(MDMOID_RESULT_HISTORY, &iidStack);
      return;
   }

   cmsObj_free((void **) &resultObj);
   CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->lastRunTime);
   newObj->resultHistoryNumberOfEntries += 1;

   rut_checkResultHistory(newObj);
}

static void rut_checkResultHistory( SpeedServiceObject *spdsvcObj)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _ResultHistoryObject *resultObj = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   while (spdsvcObj->resultHistoryNumberOfEntries > 9)
   {
      /* delete the first (oldest) result history entry instance */
      if ((ret = cmsObj_getNext(MDMOID_RESULT_HISTORY, &iidStack, (void **) &resultObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get the oldest result history entry object, ret=%d", ret);
         return;
      }
      if ((ret = cmsObj_deleteInstance(MDMOID_RESULT_HISTORY, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not delete the oldest result history entry object, ret=%d", ret);
         return;
      }
      spdsvcObj->resultHistoryNumberOfEntries -= 1;
   }
}
