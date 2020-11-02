/*
* <:copyright-BRCM:2013:proprietary:standard
* 
*    Copyright (c) 2013 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
:>
*/


#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_QOS_1


#include "stl.h"
#include "cms_util.h"
#include "rut_qos.h"

#include "rut_tmctl_wrap.h"

CmsRet stl_dev2QosObject(_Dev2QosObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2QosClassificationObject(_Dev2QosClassificationObject *obj __attribute__((unused)), 
                                   const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2QosPolicerObject(_Dev2QosPolicerObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2QosQueueObject(_Dev2QosQueueObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2QosQueueStatsObject(_Dev2QosQueueStatsObject *obj,
                                   const InstanceIdStack *iidStack __attribute__((unused)))
{
#if defined(SUPPORT_TMCTL)

   char intfName[BUFLEN_32];
   UINT32 queueId = 0;
   CmsRet ret = CMSRET_SUCCESS;

   obj->outputPackets = 0;
   obj->outputBytes = 0;
   obj->droppedPackets = 0;
   obj->droppedBytes = 0;
   obj->queueOccupancyPackets = 0;
   obj->queueOccupancyPercentage = 0;

   ret = rutQos_getQueueIdFromQueueFullPath(obj->queue,
                                            &queueId);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get queue ID from full path %s, ret=%d",
                   obj->queue, ret);
      return ret;
   }

   ret = rutQos_getInterfaceNameFromFullPath(obj->interface,
                                             intfName,
                                             sizeof(intfName));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get interface name from full path %s, ret=%d",
                   obj->interface, ret);
      return ret;
   }

   if (ret == CMSRET_SUCCESS && obj->enable == TRUE)
   {
      tmctl_queueStats_t queueStats;

      ret = rutQos_tmGetQueueStats(intfName, (SINT32)queueId, &queueStats);

      if (ret == CMSRET_SUCCESS)
      {
         obj->outputPackets = queueStats.txPackets;
         obj->outputBytes = queueStats.txBytes;
         obj->droppedPackets = queueStats.droppedPackets;
         obj->droppedBytes = queueStats.droppedBytes;
         cmsLog_debug("Get QoS queue stats successfully, intf=%s, queueId=%d, outputPackets=%u, outputBytes=%u, droppedPackets=%u, droppedBytes=%u",
                      intfName, queueId, obj->outputPackets, obj->outputBytes,
                      obj->droppedPackets, obj->droppedBytes);
      }
      else
      {
         cmsLog_error("Could not get QoS queue stats, intf=%s, queueId=%d, ret=%d",
                      intfName, queueId, ret);
      }
   }

   return ret;

#else    // SUPPORT_TMCTL

   cmsLog_error("Could not get QoS queue stats since TMCTL feature is not supported");
   return CMSRET_INTERNAL_ERROR;

#endif    // SUPPORT_TMCTL
}

CmsRet stl_dev2QosShaperObject(_Dev2QosShaperObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#ifdef NOT_SUPPORTED
CmsRet stl_dev2QosAppObject(_Dev2QosAppObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2QosFlowObject(_Dev2QosFlowObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* NOT_SUPPORTED */

#endif  /* DMP_DEVICE2_QOS_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
