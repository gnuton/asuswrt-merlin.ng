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


#ifdef DMP_DEVICE2_PERIODICSTATSBASE_1

#include "cms.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rcl.h"
#include "rut_periodicstats.h"

#include "periodicstat.h"

static CmsRet rclPeriodicstat_sendSampleSetMsg_dev2(CmsMsgType msgType,  const InstanceIdStack *iidStack, const _Dev2SampleSetObject *smpObj);

static CmsRet rclPeriodicstat_sendParameterMsg_dev2(CmsMsgType msgType,
      const InstanceIdStack *smpsetIidStack, const _Dev2SampleSetObject *smpObj,
      const InstanceIdStack *paramIidStack, const _Dev2SampleParameterObject *parameterObj);

static int rclPeriodicstat_checkRestartSampleSet_dev2(const _Dev2SampleSetObject *newObj, const _Dev2SampleSetObject *currObj);

CmsRet rcl_dev2PeriodicStatObject( _Dev2PeriodicStatObject *newObj,
                const _Dev2PeriodicStatObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   CmsRet ret=CMSRET_SUCCESS;
   int pid;

   // This application should live forever until system down.
   if ( ADD_NEW(newObj, currObj) )
   {
      pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_PERIODICSTAT, NULL, 0);
      
      if (pid == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start periodic statistic daemon.");
         ret = CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("Start periodic statistic msg sent, new PING pid=%d", pid);
      }   
   }
   return ret;
}

CmsRet rcl_dev2SampleSetObject( _Dev2SampleSetObject *newObj,
                const _Dev2SampleSetObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   CmsRet ret = CMSRET_SUCCESS;
   int needRestart = FALSE; 
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2PeriodicStatObject *periodicObj = NULL;

   if ( ADD_NEW(newObj, currObj) )
   {
      rutUtil_modifyNumSampleSets_dev2(iidStack, 1);
   }

   if ( DELETE_EXISTING(newObj, currObj) )
   {
      rutUtil_modifyNumSampleSets_dev2(iidStack, -1);
   }

   ret = cmsObj_getAncestor(MDMOID_DEV2_PERIODIC_STAT, MDMOID_DEV2_SAMPLE_SET, &ancestorIidStack, (void **)&periodicObj);
   if ( ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_SAMPLE_SET, MDMOID_DEV2_SAMPLE_PARAMETER,
                   cmsMdm_dumpIidStack(iidStack));
      return ret;
   }

   /* check MinSampleInterval */
   if ( newObj != NULL && periodicObj->minSampleInterval != 0 && newObj->sampleInterval < periodicObj->minSampleInterval )
   {
      cmsLog_notice("Reject to set the SampleInterval to %u which less then the MinSampleInterval (%u) of PeriodicStatistics.",
                    newObj->sampleInterval, periodicObj->minSampleInterval);
      cmsObj_free((void **)&periodicObj);
      return CMSRET_INVALID_PARAM_VALUE;
   }

   /* check MaxReportSamples */
   if ( newObj != NULL && newObj->reportSamples > periodicObj->maxReportSamples && periodicObj->maxReportSamples != 0 )
   {
      cmsLog_notice("Reject to set the ReportSamples to %u which larger then the MaxReportSamples (%u) of PeriodicStatistics.",
                    newObj->reportSamples, periodicObj->maxReportSamples);
      cmsObj_free((void **)&periodicObj);
      return CMSRET_INVALID_PARAM_VALUE;
   }
   cmsObj_free((void **)&periodicObj);
   

   /* If the value of SampleInterval/ReportSamples are changed, make to restart the SampleSet */
   needRestart = rclPeriodicstat_checkRestartSampleSet_dev2(newObj, currObj);

   if ( newObj != NULL )
   {
#ifdef DMP_DEVICE2_PERIODICSTATSADV_1
      /* Initiate a periodic statistics */
      if ( ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj) )
      {
         needRestart = TRUE;
      }

      /* Disable a sampleset, the collected data should be kept. */
      if ( currObj != NULL && currObj->enable == TRUE && newObj->enable == FALSE )
      {
         needRestart = FALSE;
         ret = rclPeriodicstat_sendSampleSetMsg_dev2(CMS_MSG_PERIODICSTAT_PAUSE_SAMPLESET, iidStack, currObj);
      }

      /* Trigger a force sample */
      if ( newObj->forceSample == TRUE )
      {
         needRestart = FALSE;
         ret = rclPeriodicstat_sendSampleSetMsg_dev2(CMS_MSG_PERIODICSTAT_FORCE_SAMPLE, iidStack, newObj);
      }

      if ( cmsUtl_strcmp(newObj->status, MDMVS_TRIGGER) == 0 )
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
      }
#else
      /* Initiate a periodic statistics when name is set*/
      if ( !IS_EMPTY_STRING(newObj->name) && (currObj == NULL || IS_EMPTY_STRING(currObj->name)) )
      {
         // This is a new sampleset, do not restart it.
         needRestart = TRUE;
      }
#endif /* DMP_DEVICE2_PERIODICSTATSADV_1 */
   }
   /* Turn off a periodic statistics */
   if ( DELETE_EXISTING(newObj, currObj) )
   {
      needRestart = FALSE;
      ret = rclPeriodicstat_sendSampleSetMsg_dev2(CMS_MSG_PERIODICSTAT_STOP_SAMPLESET, iidStack, currObj);
   }

   if ( needRestart )
   {
      ret = rclPeriodicstat_sendSampleSetMsg_dev2(CMS_MSG_PERIODICSTAT_START_SAMPLESET, iidStack, newObj);
   }

   return ret;
}

CmsRet rcl_dev2SampleParameterObject( _Dev2SampleParameterObject *newObj,
                const _Dev2SampleParameterObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack ancestorIidStack = *iidStack;
   Dev2SampleSetObject *smpsetObj = NULL;

   if ( ADD_NEW(newObj, currObj) )
   {
      rutUtil_modifyNumParameters_dev2(iidStack, 1);
   }

   if ( DELETE_EXISTING(newObj, currObj) )
   {
      rutUtil_modifyNumParameters_dev2(iidStack, -1);
   }

   ret = cmsObj_getAncestor(MDMOID_DEV2_SAMPLE_SET, MDMOID_DEV2_SAMPLE_PARAMETER, &ancestorIidStack, (void **)&smpsetObj);
   if ( ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   MDMOID_DEV2_SAMPLE_SET, MDMOID_DEV2_SAMPLE_PARAMETER,
                   cmsMdm_dumpIidStack(iidStack));
      return ret;
   }

#ifdef DMP_DEVICE2_PERIODICSTATSADV_1
   /* Initiate a periodic statistics */
   if ( ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj) )
   {
      ret = rclPeriodicstat_sendParameterMsg_dev2(CMS_MSG_PERIODICSTAT_ADD_PARAMETER, &ancestorIidStack, smpsetObj, iidStack, newObj); 
   }

   /* Turn off a periodic statistics */
   if ( DELETE_OR_DISABLE_EXISTING(newObj, currObj) )
   {
      ret = rclPeriodicstat_sendParameterMsg_dev2(CMS_MSG_PERIODICSTAT_DELETE_PARAMETER, &ancestorIidStack, smpsetObj, iidStack, currObj);
   }

#else
   /* Initiate a periodic statistics */
   if ( newObj != NULL && newObj->reference != NULL && (currObj == NULL || IS_EMPTY_STRING(currObj->reference)) )
   {
      ret = rclPeriodicstat_sendParameterMsg_dev2(CMS_MSG_PERIODICSTAT_ADD_PARAMETER, &ancestorIidStack, smpsetObj, iidStack, newObj); 
   }
   /* Turn off a periodic statistics */
   if ( DELETE_EXISTING(newObj, currObj) )
   {
      ret = rclPeriodicstat_sendParameterMsg_dev2(CMS_MSG_PERIODICSTAT_DELETE_PARAMETER, &ancestorIidStack, smpsetObj, iidStack, currObj);
   }
#endif /* DMP_DEVICE2_PERIODICSTATSADV_1 */
   
   cmsObj_free((void **)&smpsetObj);

   return ret;
}


static CmsRet rclPeriodicstat_sendSampleSetMsg_dev2(CmsMsgType msgType,  const InstanceIdStack *iidStack, const _Dev2SampleSetObject *smpObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   if ( iidStack == NULL || smpObj == NULL )
      return CMSRET_INVALID_ARGUMENTS;

   SampleSetInfo *info = cmsMem_alloc(sizeof(SampleSetInfo), ALLOC_ZEROIZE);
   if ( info == NULL )
      return CMSRET_RESOURCE_EXCEEDED;

   if (!IS_EMPTY_STRING(smpObj->name))
      sprintf(info->name, "%s", smpObj->name);
   info->sampleInterval = smpObj->sampleInterval;
   info->reportSamples = smpObj->reportSamples;
   info->sampleCount = 0;
   info->parameters = NULL;
#ifdef DMP_DEVICE2_PERIODICSTATSADV_1
   info->fetchSamples = smpObj->fetchSamples;
#endif
   info->id = PEEK_INSTANCE_ID(iidStack);
   ret = rutPeriodicstat_sendSampleSetMsg(msgType, info);

   cmsMem_free(info);

   return ret;

}

static CmsRet rclPeriodicstat_sendParameterMsg_dev2(CmsMsgType msgType,
      const InstanceIdStack *smpsetIidStack, const _Dev2SampleSetObject *smpObj,
      const InstanceIdStack *paramIidStack, const  _Dev2SampleParameterObject *parameterObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   SampleSetInfo *smpsetInfo = NULL;
   SampleParameterInfo *paramInfo = NULL;

   if ( smpObj == NULL || parameterObj == NULL )
      return CMSRET_INVALID_ARGUMENTS;

   smpsetInfo = (SampleSetInfo *)cmsMem_alloc(sizeof(SampleSetInfo), ALLOC_ZEROIZE);
   paramInfo = (SampleParameterInfo *)cmsMem_alloc(sizeof(SampleParameterInfo), ALLOC_ZEROIZE);

   if (smpsetInfo != NULL && paramInfo != NULL)
   {
      smpsetInfo->id = PEEK_INSTANCE_ID(smpsetIidStack);
      if (!IS_EMPTY_STRING(parameterObj->reference))
         sprintf(paramInfo->name, "%s", parameterObj->reference);
      paramInfo->id = PEEK_INSTANCE_ID(paramIidStack);
#ifdef DMP_DEVICE2_PERIODICSTATSADV_1
      if (cmsUtl_strcmp(parameterObj->sampleMode, MDMVS_CURRENT)==0)
         paramInfo->sampleMode = SAMPLEMODE_CURRENT; 
      else
         paramInfo->sampleMode = SAMPLEMODE_CHANGE;

      paramInfo->lowThreshold = parameterObj->lowThreshold;
      paramInfo->highThreshold = parameterObj->highThreshold;
#endif
      ret = rutPeriodicstat_sendParameterMsg(msgType, smpsetInfo, paramInfo);
   }
   else
      ret = CMSRET_RESOURCE_EXCEEDED;

   if (smpsetInfo)
      cmsMem_free(smpsetInfo);
   if (paramInfo)
      cmsMem_free(paramInfo);
   return ret;

}

static int rclPeriodicstat_checkRestartSampleSet_dev2(const _Dev2SampleSetObject *newObj, const _Dev2SampleSetObject *currObj)
{
   if ( newObj != NULL && currObj != NULL )
   {
#ifdef DMP_DEVICE2_PERIODICSTATSADV_1
      if ( newObj->enable == FALSE )
         return FALSE;
#endif /* DMP_DEVICE2_PERIODICSTATSADV_1 */
      if ( newObj->sampleInterval != currObj->sampleInterval )
         return TRUE;

      if ( newObj->reportSamples != currObj->reportSamples )
         return TRUE;
   }
   return FALSE;
}
#endif /* DMP_DEVICE2_PERIODICSTATSBASE_1 */
