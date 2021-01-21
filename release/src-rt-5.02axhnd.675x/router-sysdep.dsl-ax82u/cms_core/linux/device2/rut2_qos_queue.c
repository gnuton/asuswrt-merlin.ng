/***********************************************************************
 *
 *  Copyright (c) 2009-2013  Broadcom Corporation
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

#ifdef DMP_DEVICE2_QOS_1
/* All of this code is part of the DEVICE2_QOS_1 profile (which is enabled
 * only in Pure TR181 mode.
 */

#include "cms_core.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_qos.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_qos.h"
#include "rut_wan.h"


void rutQos_reconfigAllQueuesOnLayer2Intf_dev2(const char *l2IntfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2QosQueueObject *qObj = NULL;
   char *fullPath=NULL;
   CmsRet ret;

   cmsLog_notice("Enter: l2IntfName=%s", l2IntfName);

   /* convert l2IntfName to fullpath for comparison */
   if ((ret = qdmIntf_intfnameToFullPathLocked(l2IntfName, TRUE, &fullPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("qdmIntf_intfnameToFullPath for %s returns error. ret=%d",
                    l2IntfName, ret);
      return;
   }
   else
   {
      cmsLog_debug("l2IntfName %s ==> %s", l2IntfName, fullPath);
   }

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_QUEUE, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **) &qObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(qObj->interface, fullPath))
      {
         CmsRet r2;

         /*
          * Just do a set without modifying the object.  This will cause
          * the RCL handler function for the queue obj to get called,
          * and it will configure or unconfigure the queue.
          */
         r2 = cmsObj_set(qObj, &iidStack);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("set of qObj->name %s failed, r2=%d",
                         qObj->X_BROADCOM_COM_QueueName, r2);
         }
      }

      cmsObj_free((void **) &qObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);

   if (ret != CMSRET_SUCCESS && ret != CMSRET_NO_MORE_INSTANCES)
   {
      cmsLog_error("error while traversing queues, ret=%d", ret);
   }

   return;
}


void rutQos_deleteQueues_dev2(const char *intfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack savedIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2QosQueueObject *qObj = NULL;
   char *fullPath=NULL;
   CmsRet ret;
   UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;

   cmsLog_notice("Entered: intfName=%s", intfName);

   /* so we can still do path conversion on the intf that is being deleted */
   mdmLibCtx.hideObjectsPendingDelete = FALSE;
   ret = qdmIntf_intfnameToFullPathLocked(intfName, TRUE, &fullPath);
   mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get fullPath on %s, ret=%d", intfName, ret);
      return;
   }
   else
   {
      cmsLog_debug("intfName %s ==> %s", intfName, fullPath);
   }


   /* delete all queues matching this intf */
   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_QUEUE, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&qObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(fullPath, qObj->interface))
      {
         cmsLog_debug("found match %s at instance %d, delete it!",
                      qObj->interface, PEEK_INSTANCE_ID(&iidStack));
         cmsObj_deleteInstance(MDMOID_DEV2_QOS_QUEUE, &iidStack);
      }
      else
      {
         /* save this iidStack in case we delete the next obj instance */
         savedIidStack = iidStack;
      }

      cmsObj_free((void **) &qObj);
      iidStack = savedIidStack;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);

   return;
}


CmsRet rutQos_fillQueueIdArray_dev2(const char *l2IfName, UINT32 maxQId, char *idArray)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2QosQueueObject *qObj = NULL;
   char ifName[CMS_IFNAME_LENGTH];
   CmsRet ret;


   /* loop through the queue table */
   while ((ret = cmsObj_getNext(MDMOID_DEV2_QOS_QUEUE, &iidStack, (void **)&qObj)) == CMSRET_SUCCESS)
   {
      /* convert mdm full path string to queue interface name */
      if ((ret = qdmIntf_fullPathToIntfnameLocked(qObj->interface, ifName)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmIntf_fullPathToIntfname returns error. ret=%d", ret);
         cmsObj_free((void **)&qObj);
         continue;
      }

      if (cmsUtl_strcmp(l2IfName, ifName))
      {
         cmsObj_free((void **)&qObj);
         continue;
      }

      if (qObj->X_BROADCOM_COM_QueueId < 1 || qObj->X_BROADCOM_COM_QueueId > maxQId)
      {
         cmsLog_error("Found invalid existing queueId %d (maxQId=%d)",
                       qObj->X_BROADCOM_COM_QueueId, maxQId);
         cmsObj_free((void **)&qObj);
         ret = CMSRET_INTERNAL_ERROR;
         break;
      }

      if (cmsUtl_strcmp(qObj->schedulerAlgorithm, MDMVS_SP) == 0)
      {
         idArray[qObj->X_BROADCOM_COM_QueueId - 1] = CMS_QUEUE_SCHED_SP;
      }
      else
      {
         idArray[qObj->X_BROADCOM_COM_QueueId - 1] = CMS_QUEUE_SCHED_WRR_WFQ;
      }
      cmsObj_free((void **)&qObj);
   }

   if (ret == CMSRET_NO_MORE_INSTANCES)
   {
      ret = CMSRET_SUCCESS;
   }

   return ret;

}  /* End of rutQos_fillQueueIdArray_dev2() */


CmsRet rutQos_getWanQosInfo_dev2(UBOOL8 *bridgeQos, UBOOL8 *routeQos)
{
   char intfNameBuf[CMS_IFNAME_LENGTH]={0};

   /*
    * Simplify the algorithm at first.  Since the caller, rutQos_doDefaultPolicy
    * does not care about bridgeQos, set that to FALSE.  For routedQos,
    * if there are any routed WAN connection UP, set to TRUE.
    */
   *bridgeQos = FALSE;

   *routeQos = rutWan_findFirstRoutedAndConnected(intfNameBuf);

   return CMSRET_SUCCESS;
}


CmsRet rutQos_setDefaultEthQueues_dev2(const char *ifname, UBOOL8 isWan)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2QosQueueObject *qObj = NULL;
   CmsQosQueueInfo qInfo;
   UINT32 precedence;
   UINT32 maxQueues;
   UINT32 qid;
   UBOOL8 configExist = FALSE;
   char qname[8];
   char *fullPath=NULL;
   CmsRet ret;

   cmsLog_debug("Enter: ifname=%s isWan=%d", ifname, isWan);

   /* convert l2IntfName to fullpath for comparison */
   if ((ret = qdmIntf_intfnameToFullPathLocked(ifname, TRUE, &fullPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("qdmIntf_intfnameToFullPath for %s returns error. ret=%d", ifname, ret);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* see if any QoS queue had been configured */   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_DEV2_QOS_QUEUE, &iidStack, (void **)&qObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(qObj->interface, fullPath) == 0)
      {
         configExist = TRUE;

         memset(&qInfo, 0, sizeof(qInfo));
         /* convert TR98 or TR181 QoS qObj to common qinfo struct */
         qdmQos_convertDmQueueObjToCmsQueueInfoLocked(qObj, &qInfo);

         /* configure tm for the existing CMS queue. */
         if ((ret = rutQos_tmQueueConfig(QOS_COMMAND_CONFIG, &qInfo)) != CMSRET_SUCCESS)
         {
            cmsObj_free((void **)&qObj);
            cmsLog_error("rutQos_tmQueueConfig returns error. ret=%d", ret);
            break;
         }
      }
      cmsObj_free((void **)&qObj);
   }
   if (ret != CMSRET_SUCCESS && ret != CMSRET_NO_MORE_INSTANCES)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      return ret;
   }
   if (configExist)
   {
      cmsLog_debug("QoS Queues for %s had been configured", ifname);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      return CMSRET_SUCCESS;
   }

   cmsLog_notice("Init QoS Queues for %s", ifname);
   
   if (isWan)
      maxQueues = MAX_ETHWAN_TRANSMIT_QUEUES;
   else
      maxQueues = MAX_ETH_TRANSMIT_QUEUES;
   
   /* create default queues for the ETH interface */
   for (precedence = 1; precedence <= maxQueues; precedence++)
   {
      /* add a new queue object instance */
      INIT_INSTANCE_ID_STACK(&iidStack);
      if ((ret = cmsObj_addInstance(MDMOID_DEV2_QOS_QUEUE, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_addInstance returns error, ret=%d", ret);
         break;
      }

      /* get the object, it will be initially filled in with default values */
      if ((ret = cmsObj_get(MDMOID_DEV2_QOS_QUEUE, &iidStack, 0, (void **)&qObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_get returns error. ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_DEV2_QOS_QUEUE, &iidStack);
         break;
      }

#if defined(SUPPORT_TMCTL)
      if (rut_tmctl_getQueueMap() == QID_PRIO_MAP_Q0P7)
      {
         qid = precedence;
      }
      else
#endif /* SUPPORT_TMCTL */
      {
         qid = 1 + maxQueues - precedence;
      }

      /* queueName */
      if (isWan)
         sprintf(qname, "WAN Q%u", qid);
      else
         sprintf(qname, "LAN Q%u", qid);

      qObj->enable                 = TRUE;
      qObj->precedence             = precedence;
      qObj->X_BROADCOM_COM_QueueId = qid;
      CMSMEM_REPLACE_STRING_FLAGS(qObj->X_BROADCOM_COM_QueueName, qname, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(qObj->interface, fullPath, mdmLibCtx.allocFlags);

      /* set the Queue Object instance */
      if ((ret = cmsObj_set(qObj, &iidStack)) != CMSRET_SUCCESS)
      {
         CmsRet r2;
         cmsLog_error("cmsObj_set returns error, ret = %d", ret);
          
         /* since set failed, we have to delete the instance that we just added */       
         if ((r2 = cmsObj_deleteInstance(MDMOID_DEV2_QOS_QUEUE, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_deleteInstance returns error, r2=%d", r2);
         }
         
         cmsObj_free((void **)&qObj);
         break;
      }

      cmsObj_free((void **)&qObj);
   }
   
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
   return ret;
   
}

void rutQos_getIntfNumOfCreatedQueues_dev2(const void *queueObj,
                                          UINT32 *numQueues)
{
   Dev2QosQueueObject *qObj = NULL;
   InstanceIdStack iidStack;
   UINT32 createdQNum = 0;
   CmsRet ret;
   Dev2QosQueueObject *dev2QObj= (_Dev2QosQueueObject*)queueObj;

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_DEV2_QOS_QUEUE, &iidStack, (void**)&qObj))
          == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(qObj->interface, dev2QObj->interface) == 0)
      {
          createdQNum++;
      }
      cmsObj_free((void**)&qObj);
   }

   if (numQueues != NULL)
   {
      *numQueues = createdQNum;
      cmsLog_debug("qIntf=%s, qnum=%d", ((_Dev2QosQueueObject*)queueObj)->interface, createdQNum);
   }

}

void rutQos_reconfigShaperOnLayer2Intf_dev2(const char *l2IntfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2QosShaperObject *shaperObj = NULL;
  
   char *fullPath=NULL;
   CmsRet ret;

   cmsLog_notice("Enter: l2IntfName=%s", l2IntfName);

   /* convert l2IntfName to fullpath for comparison */
   if ((ret = qdmIntf_intfnameToFullPathLocked(l2IntfName, TRUE, &fullPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("qdmIntf_intfnameToFullPath for %s returns error. ret=%d",
                    l2IntfName, ret);
      return;
   }
   else
   {
      cmsLog_debug("l2IntfName %s ==> %s", l2IntfName, fullPath);
   }

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_SHAPER, &iidStack, OGF_NO_VALUE_UPDATE,
                                     (void **) &shaperObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(shaperObj->interface, fullPath))
      {
         CmsRet r2;
         SINT32 shapingRate = -1;
         if (shaperObj->enable)
         {
            shapingRate = shaperObj->shapingRate;
         }
         r2 = rutQos_tmPortShaperCfg(l2IntfName, shapingRate, (SINT32)shaperObj->shapingBurstSize, MDMVS_UP, FALSE);
         if (r2 == CMSRET_SUCCESS)
         {
            cmsLog_debug("Set QoS port shaper successfully, intf=%s, shapingRate=%d, shapingBurstSize=%u",
                         l2IntfName, shapingRate, shaperObj->shapingBurstSize);
         }
         else
         {
            cmsLog_error("Could not set QoS port shaper, intf=%s, shapingRate=%d, shapingBurstSize=%u, retTm=%d",
                         l2IntfName, shapingRate, shaperObj->shapingBurstSize, ret);
         }
      }
      cmsObj_free((void **) &shaperObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);

   if (ret != CMSRET_SUCCESS && ret != CMSRET_NO_MORE_INSTANCES)
   {
      cmsLog_error("error while traversing shaper objects, ret=%d", ret);
   }

   return;
}

#endif  /* DMP_DEVICE2_QOS_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */

