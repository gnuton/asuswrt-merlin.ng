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

#ifdef DMP_DEVICE2_QOS_1

#include "cms.h"
#include "cms_qdm.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_qos.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_qos.h"
#include "rut_tmctl_wrap.h"




/*
 * TR98 and TR181 QoS objects are quite similar, but the implementation is
 * slightly different.  Here are some brief notes on TR181 QoS implementation:
 *
 * The Queue Object is a layer 2 only concept.  Queues are configured via
 * a common function: rutQos_qMgmtQueueConfig().
 *
 * When a layer 2 interface goes up or down, the TR181 code will call
 * rutQos_reconfigAllQueuesOnLayer2Intf_dev2().  When a layer 2 interface
 * is deleted, the RCL code will call rutQos_deleteQueues_dev2().
 *
 *
 * The Classification Object is a mix of layer 2 and layer 3.  It's ingress
 * interface can be either a layer 2 interface (LAN side eth, LAN side wifi)
 * or a layer 3 WAN side interface, e.g. ptm0.1.  Its egress interface
 * always points to a layer 3 interface, even though underneath the layer 3
 * interface is a layer 2 interface with the actual queues.  The Policer
 * Object is pointed to by the Classification object, so the Policer object
 * is like a child object of the Classification object even though the Policer
 * object has its own table.
 *
 * The main function to trigger a reconfiguration of the Classifiers is
 * rutQos_reconfigAllClassifications_dev2().  It is called by the TR181 code
 * when:
 *
 * - a layer 2 link which could be referred to by the Classification
 *   ingress comes UP or goes DOWN.
 * - a layer 3 WAN service comes UP or goes DOWN.
 */




static void modifyClassificationNumEntries(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_QOS,
                                 MDMOID_DEV2_QOS_CLASSIFICATION, iidStack, delta);
}

static void modifyPolicerNumEntries(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_QOS,
                                 MDMOID_DEV2_QOS_POLICER, iidStack, delta);
}

static void modifyQueueNumEntries(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_QOS,
                                 MDMOID_DEV2_QOS_QUEUE, iidStack, delta);
}

#ifdef DMP_DEVICE2_QOSSTATS_1
static void modifyQueueStatsNumEntries(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_QOS,
                                 MDMOID_DEV2_QOS_QUEUE_STATS, iidStack, delta);
}
#endif  /* DMP_DEVICE2_QOSSTATS_1 */

static void modifyShaperNumEntries(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_QOS,
                                 MDMOID_DEV2_QOS_SHAPER, iidStack, delta);
}



CmsRet rcl_dev2QosObject( _Dev2QosObject *newObj,
                      const _Dev2QosObject *currObj,
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{

   /*
    * The only thing we need to detect is the change of the DSCP mark at
    * runtime.  Don't need to set at bootup because DSCP mark will be set
    * when any Layer 3 service comes up in rutQos_reconfigAllClassifications_dev2.
    */
   if (newObj && currObj && newObj->defaultDSCPMark != currObj->defaultDSCPMark)
   {
      /* delete the current default DSCP mark policy */
      rutQos_doDefaultDSCPMarkPolicy(QOS_COMMAND_UNCONFIG, currObj->defaultDSCPMark);

      /* reconfig all classifications to ensure proper ordering of the rules */
      rutQos_reconfigAllClassifications_dev2(NULL);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2QosClassificationObject( _Dev2QosClassificationObject *newObj,
                                    const _Dev2QosClassificationObject *currObj,
                                    const InstanceIdStack *iidStack,
                                    char **errorParam __attribute__((unused)),
                                    CmsRet *errorCode __attribute__((unused)))
{


   if (ADD_NEW(newObj, currObj))
   {
      /*
       * ADD_NEW happens at bootup or when a new instance of this object
       * is created.  Don't do anything in this case.  If bootup, when
       * a Layer 3 WAN service comes up, rcl_dev2IpObject will call
       * rutQos_reconfigAllClassifications, which will config all classifiers.
       * If add new, user will fill in the obj, and we will reconfig at that time.
       */
      modifyClassificationNumEntries(iidStack, 1);
   }

   if (newObj && currObj && newObj->enable)
   {
      /*
       * classifiers are special.  The order in which the classifications
       * are configured is important, so you can just configure one
       * classification (which might be in the middle of the table),
       * you need to unconfigure them all and then configure them all.
       * A global reconfig of the classifiers are done whenever
       * a L3 WAN service comes up or goes down.  That is done from
       * rcl_dev2IpObject, not here.  However, if an individual classification
       * is changed, we trigger a global reconfig from here.
       */
      if (rutQos_isClassificationChanged_dev2(newObj, currObj))
      {
         cmsLog_debug("classification %s at inst %d changed, reconfigAllClassifications",
                      newObj->X_BROADCOM_COM_ClassName, PEEK_INSTANCE_ID(iidStack));
         rutQos_reconfigAllClassifications_dev2(NULL);
      }
   }

   if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /* We can unconfig an individual classification without doing a global reconfig */
      rutQos_qMgmtClassConfig(QOS_COMMAND_UNCONFIG, currObj);

      if (DELETE_EXISTING(newObj, currObj))
      {
         modifyClassificationNumEntries(iidStack, -1);
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2QosPolicerObject( _Dev2QosPolicerObject *newObj,
                             const _Dev2QosPolicerObject *currObj,
                             const InstanceIdStack *iidStack,
                             char **errorParam __attribute__((unused)),
                             CmsRet *errorCode __attribute__((unused)))
{

   cmsLog_debug("Entered");

   if (ADD_NEW(newObj, currObj))
   {
      modifyPolicerNumEntries(iidStack, 1);
      /*
       * ADD_NEW happens at bootup or when a new instance of this object
       * is created.  Don't do anything in this case.  If bootup, when
       * a Layer 3 WAN service comes up, rcl_dev2IpObject will call
       * rutQos_reconfigAllClassifications, which will config all classifiers
       * and policers.  If add new, user will fill in the obj, and we will
       * reconfig at that time.
       */
   }

   if (newObj && currObj && newObj->enable &&
       rutQos_isPolicerChanged_dev2(newObj, currObj))
   {
      /* reconfig all classifications to ensure proper ordering of the rules.
       * Classifications include the policer.
       */
      rutQos_reconfigAllClassifications_dev2(NULL);
   }


   if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      UBOOL8 isReferred=FALSE;
      CmsRet ret;

      ret = qdmQos_referenceCheckLocked(CMS_QOS_REF_TARGET_POLICER,
                                        PEEK_INSTANCE_ID(iidStack),
                                        &isReferred);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("error during reference check, ret=%d", ret);
         return ret;
      }

      if (isReferred)
      {
         cmsLog_error("Cannot disable or delete Policer while it is referenced by classifier");
         return CMSRET_INVALID_ARGUMENTS;
      }

      /*
       * If we get here, that means this policer is not referenced by
       * any classifiers.  So disabling or deleting it will have no effect.
       * So no need to trigger reconfig of classifications.
       */

      if (DELETE_EXISTING(newObj, currObj))
      {
         modifyPolicerNumEntries(iidStack, -1);
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2QosQueueObject( _Dev2QosQueueObject *newObj,
                           const _Dev2QosQueueObject *currObj,
                           const InstanceIdStack *iidStack,
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{

   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("Entered");

   if (ADD_NEW(newObj, currObj))
   {
      /*
       * Either start-up config or adding new object instance.
       * We don't need to take any action at this point for either cases.
       * -- For start-up config, we will take action when the interface is up.
       * -- For adding new object instance, we will take action when the
       *    parameters of the new object instance is set.
       */
      modifyQueueNumEntries(iidStack, 1);
   }

   if (newObj != NULL && currObj != NULL)
   {
      UBOOL8 enableStateChanged = TRUE;
      {
         _Dev2QosObject *qosObj=NULL;

         INIT_INSTANCE_ID_STACK(&iidStack);
         ret = cmsObj_get(MDMOID_DEV2_QOS, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&qosObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get QOS obj, ret=%d", ret);
         }
         else
         {
            /* we are in the case of switched on/off QoS from web if X_BROADCOM_COM_EnableStateChanged is set */
            enableStateChanged = qosObj->X_BROADCOM_COM_EnableStateChanged;
            cmsObj_free((void **)&qosObj);
         }
      }

      /*
       * Queue obj is being modified or disabled, or
       * rutQos_reconfigAllQueuesOnLayer2Intf_dev2 is doing a set on this obj
       * to tell us to update our state.  If we had previously configured
       * this queue (as indicated by status == MDMVS_ENABLED),
       * unconfigure everything first.
       */
      if (!cmsUtl_strcmp(currObj->status, MDMVS_ENABLED))
      {
         cmsLog_notice("unconfig queue %s on intf %s",
                       currObj->X_BROADCOM_COM_QueueName, currObj->interface);

         /* don't reconfigure any classifiers here when enableStateChanged is TRUE,
          * we will handle it outside RCL, see dalQos_configQosMgmt_dev2().
          */
         if (newObj->enable == FALSE && currObj->enable == TRUE && !enableStateChanged)
         {
            /*
             * Someone has disabled this specific queue, reconfig all
             * classifiers before deconfig this queue.
             */
            rutQos_reconfigAllClassifications_dev2(NULL);
         }

         if ((ret = rutQos_qMgmtQueueConfig(QOS_COMMAND_UNCONFIG, currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_qMgmtQueueConfig UNCONFIG returns error. ret=%d", ret);
            return ret;
         }

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
      }

      if (newObj->enable)
      {
         char statusBuf[BUFLEN_64]={0};

         ret = qdmIntf_getStatusFromFullPathLocked_dev2(newObj->interface,
                                             statusBuf, sizeof(statusBuf));
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get status for %s, ret=%d",
                         newObj->interface, ret);
            return ret;
         }
         cmsLog_notice("intf %s status=%s", newObj->interface, statusBuf);

         cmsLog_notice("configuring queue %s for %s",
               newObj->X_BROADCOM_COM_QueueName, newObj->interface);

         /* Queue is enabled, config it */
         if ((ret = rutQos_qMgmtQueueConfig(QOS_COMMAND_CONFIG, newObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_qMgmtQueueConfig CONFIG returns error. ret=%d", ret);
            return ret;
         }

         /* Mark the fact we successfully configured */
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);

         /* don't reconfigure any classifiers here when enableStateChanged is TRUE,
          * we will handle it outside RCL, see dalQos_configQosMgmt_dev2().
          */
         if (newObj->enable == TRUE && currObj->enable == FALSE && !enableStateChanged)
         {
            /*
             * Someone has enabled this specific queue, reconfig all
             * classifiers after configuring this queue.
             */
            rutQos_reconfigAllClassifications_dev2(NULL);
         }
      }
   }

   if (DELETE_EXISTING(newObj, currObj))
   {
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      modifyQueueNumEntries(iidStack, -1);

      /* TR181 says we should set classifiers which reference this queue
       * to via egressInterface to NULL (or delete the classifier like we do
       * in TR98).
       */
      rutQos_deleteClassByEgressQueueInstance_dev2(PEEK_INSTANCE_ID(iidStack));

      /*
       * This queue is being deleted.  If this queue was previously configured,
       * (as indicated by status == MDMVS_ENABLED), unconfigure this queue.
       */
      if (!cmsUtl_strcmp(currObj->status, MDMVS_ENABLED))
      {
         if ((ret = rutQos_qMgmtQueueConfig(QOS_COMMAND_UNCONFIG, currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_qMgmtQueueConfig UNCONFIG returns error. ret=%d", ret);
         }
      }

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
   }

   return ret;
}


#ifdef DMP_DEVICE2_QOSSTATS_1

CmsRet rcl_dev2QosQueueStatsObject( _Dev2QosQueueStatsObject *newObj,
                                const _Dev2QosQueueStatsObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      modifyQueueStatsNumEntries(iidStack, 1);
   }

   if (newObj && currObj && newObj->enable)
   {
      char intfName[BUFLEN_32];
      UINT32 queueId = 0;
      CmsRet ret = CMSRET_SUCCESS;

      ret = rutQos_getQueueIdFromQueueFullPath(newObj->queue,
                                               &queueId);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get queue ID from full path %s, ret=%d",
                      newObj->queue, ret);
         return ret;
      }

      ret = rutQos_getInterfaceNameFromFullPath(newObj->interface,
                                                intfName,
                                                sizeof(intfName));
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get interface name from full path %s, ret=%d",
                      newObj->interface, ret);
         return ret;
      }

      // change status to enable
      CMSMEM_REPLACE_STRING_FLAGS(newObj->status,
                                  MDMVS_ENABLED,
                                  mdmLibCtx.allocFlags);
   }

   if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if (DELETE_EXISTING(newObj, currObj))
      {
         modifyQueueStatsNumEntries(iidStack, -1);
      }
      else if (newObj != NULL && newObj->enable == FALSE)
      {
         // change status to disable
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status,
                                     MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);
      }
   }

   return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_QOSSTATS_1 */

CmsRet rcl_dev2QosShaperObject( _Dev2QosShaperObject *newObj,
                            const _Dev2QosShaperObject *currObj,
                            const InstanceIdStack *iidStack,
                            char **errorParam __attribute__((unused)),
                            CmsRet *errorCode __attribute__((unused)))
{
#if defined(SUPPORT_TMCTL)

   char intfName[BUFLEN_32];
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      modifyShaperNumEntries(iidStack, 1);
      return ret;
   }

   // only validate interface when newObj is not NULL
   if (newObj != NULL)
   {
      ret = rutQos_getInterfaceNameFromFullPath(newObj->interface,
                                                intfName,
                                                sizeof(intfName));
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get interface name from full path %s, ret=%d",
                      newObj->interface, ret);
         return ret;
      }

      // On 63138/63148/63158 and 63268, setting port shaper
      // only is supported on Eth LAN and Eth WAN
      if (cmsUtl_strstr(intfName, "eth") == NULL )
      {
         cmsLog_error("Setting port shaper for interface name %s cannot be supported",
                      newObj->interface);
         return CMSRET_INVALID_PARAM_VALUE;
      }
   }

   if (newObj && currObj && newObj->enable)
   {
      // change status to enable
      CMSMEM_REPLACE_STRING_FLAGS(newObj->status,
                                  MDMVS_ENABLED,
                                  mdmLibCtx.allocFlags);

      if (currObj->enable == FALSE ||
          newObj->shapingRate != currObj->shapingRate ||
          newObj->shapingBurstSize != currObj->shapingBurstSize)
      {
         ret = rutQos_tmPortShaperCfg(intfName, (SINT32) newObj->shapingRate,
                                    (SINT32) newObj->shapingBurstSize, MDMVS_UP, FALSE); 

         if (ret == CMSRET_SUCCESS)
         {
            cmsLog_debug("Set QoS port shaper successfully, intf=%s, shapingRate=%d, shapingBurstSize=%u",
                         intfName, newObj->shapingRate, newObj->shapingBurstSize);
         }
         else
         {
            cmsLog_error("Could not set QoS port shaper, intf=%s, shapingRate=%d, shapingBurstSize=%u, retTm=%d",
                         intfName, newObj->shapingRate, newObj->shapingBurstSize, ret);
         }
      }
   }

   if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if (DELETE_EXISTING(newObj, currObj))
      {
         modifyShaperNumEntries(iidStack, -1);

         rutQos_getInterfaceNameFromFullPath(currObj->interface,
                                             intfName,
                                             sizeof(intfName));

         // if shapingRate is -1 then no shaper
         ret = rutQos_tmPortShaperCfg(intfName, -1, currObj->shapingBurstSize, MDMVS_UP, FALSE);
         if (ret == CMSRET_SUCCESS)
         {
            cmsLog_debug("Set QoS port shaper successfully, intf=%s, shapingRate=%d, shapingBurstSize=%u",
                         intfName, -1, currObj->shapingBurstSize);
         }
         else
         {
            cmsLog_error("Could not set QoS port shaper, intf=%s, shapingRate=%d, shapingBurstSize=%u, retTm=%d",
                         intfName, -1, currObj->shapingBurstSize, ret);
         }
      }
      else if (newObj != NULL && newObj->enable == FALSE)
      {
         // change status to disable
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status,
                                     MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);

         // if shapingRate is -1 then no shaper
         ret = rutQos_tmPortShaperCfg(intfName, -1, currObj->shapingBurstSize, MDMVS_UP, FALSE);
         if (ret == CMSRET_SUCCESS)
         {
            cmsLog_debug("Set QoS port shaper successfully, intf=%s, shapingRate=%d, shapingBurstSize=%u",
                         intfName, -1, currObj->shapingBurstSize);
         }
         else
         {
            cmsLog_error("Could not set QoS port shaper, intf=%s, shapingRate=%d, shapingBurstSize=%u, retTm=%d",
                         intfName, -1, currObj->shapingBurstSize, ret);
         }
      }
   }

   return CMSRET_SUCCESS;

#else    // SUPPORT_TMCTL

   cmsLog_error("Could not set QoS port shaper since TMCTL feature is not supported");
   return CMSRET_INTERNAL_ERROR;

#endif    // SUPPORT_TMCTL
}


#ifdef NOT_SUPPORTED

CmsRet rcl_dev2QosAppObject( _Dev2QosAppObject *newObj __attribute__((unused)),
                         const _Dev2QosAppObject *currObj __attribute__((unused)),
                         const InstanceIdStack *iidStack __attribute__((unused)),
                         char **errorParam __attribute__((unused)),
                         CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2QosFlowObject( _Dev2QosFlowObject *newObj __attribute__((unused)),
                          const _Dev2QosFlowObject *currObj __attribute__((unused)),
                          const InstanceIdStack *iidStack __attribute__((unused)),
                          char **errorParam __attribute__((unused)),
                          CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#endif /* NOT_SUPPORTED */


#endif  /* DMP_DEVICE2_QOS_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
