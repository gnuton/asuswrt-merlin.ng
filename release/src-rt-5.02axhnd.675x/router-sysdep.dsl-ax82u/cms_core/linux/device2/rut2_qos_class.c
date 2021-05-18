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
#include "rut_iptables.h"




/* Local helper functions */
static UBOOL8 isAffectClassifications_dev2(const char *intfName);




void rutQos_reconfigAllClassifications_dev2(const char *intfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   UBOOL8 doReconfig=FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_notice("====  Entered: intfName=%s", intfName);

   /*
    * determine if the intf that came up or down is related to any
    * classifications.  If not, save some CPU cycles and do nothing.
    */
   if ((intfName == NULL) ||
       (!cmsUtl_strncmp(intfName, WLAN_IFC_STR, strlen(WLAN_IFC_STR))) ||
       isAffectClassifications_dev2(intfName))
   {
      doReconfig = TRUE;
   }

   if (!doReconfig)
   {
      cmsLog_debug("No reconfig needed for %s", intfName);
      return;
   }

   /* for more classification rules, the operation may take longer  */
   cmsLck_setHoldTimeWarnThresh(12000);
   /* make sure that all the required modules for qos support are loaded */
   rutIpt_qosLoadModule();


   /* walk over all classifications and unconfig everything that was
    * previously configured */
   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&cObj)) == CMSRET_SUCCESS)
   {
      cmsLog_debug("unconfig [%s] status %s", cObj->X_BROADCOM_COM_ClassName, cObj->status);
      if (!cmsUtl_strcmp(cObj->status, MDMVS_ENABLED))
      {
         rutQos_qMgmtClassConfig(QOS_COMMAND_UNCONFIG, cObj);

         /* record the fact that we unconfigured.  Just setting status
          * will not trigger any action in rcl_dev2QosClassificationObject
          */
         CMSMEM_REPLACE_STRING_FLAGS(cObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
         cmsObj_set(cObj, &iidStack);

      }

      cmsObj_free((void **)&cObj);
   }

   /* now walk over all classifications again and Config everything that
    * needs to be configured.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&cObj)) == CMSRET_SUCCESS)
   {
      UBOOL8 ingressUp=FALSE;
      UBOOL8 egressUp=FALSE;
      char intfNameBuf[CMS_IFNAME_LENGTH]={0};
      CmsRet r2;

      if (IS_EMPTY_STRING(cObj->interface))
      {
         ingressUp = TRUE;
      }
      else if (!cmsUtl_strcmp(cObj->interface, MDMVS_LAN) ||
               !cmsUtl_strcmp(cObj->interface, MDMVS_WAN) ||
               !cmsUtl_strcmp(cObj->interface, MDMVS_LOCAL))
      {
         ingressUp = TRUE;
      }
      else
      {
         r2 = qdmIntf_fullPathToIntfnameLocked(cObj->interface, intfNameBuf);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not convert %s to intfName, r2=%d",
                         cObj->interface, r2);
         }
         else
         {
            ingressUp = cmsNet_isInterfaceLinkUp(intfNameBuf);
         }
      }

      /*
       * If ingress is UP, then we continue to check if egress is UP.
       * If ingress is not UP, then don't do any more work because we
       * are not going to config this classification anyways.
       */
      if (ingressUp)
      {
         UBOOL8 isEnabled=FALSE;

         r2 = qdmQos_getQueueInfoByClassQueueLocked(cObj->X_BROADCOM_COM_ClassQueue,
                                                    &isEnabled, NULL, NULL);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get info on Queue %d", cObj->X_BROADCOM_COM_ClassQueue);
         }
         else
         {
            if (isEnabled && cmsNet_isInterfaceLinkUp(cObj->X_BROADCOM_COM_egressInterface))
            {
               egressUp = TRUE;
            }
         }
      }

      cmsLog_debug("config [%s] ingress=%s ingressUp=%d egr=%s egressUp=%d",
                   cObj->X_BROADCOM_COM_ClassName, cObj->interface, ingressUp,
                   cObj->X_BROADCOM_COM_egressInterface, egressUp);

      if (ingressUp && egressUp)
      {
         rutQos_qMgmtClassConfig(QOS_COMMAND_CONFIG, cObj);

         /* record the fact that we unconfigured.  Just setting status
          * will not trigger any action in rcl_dev2QosClassificationObject
          */
         CMSMEM_REPLACE_STRING_FLAGS(cObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
         cmsObj_set(cObj, &iidStack);
      }

      cmsObj_free((void **)&cObj);
   }


#ifdef BRCM_WLAN
   /* see rclQos_classConfig (basically defaultPolicy for wlan) */
   if(intfName && cmsUtl_strstr(intfName, WLAN_IFC_STR))
   {
      Dev2QosQueueObject *qObj = NULL;
      char qIntf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      char *fullPath=NULL;
      int found = 0;

      /* convert l2IntfName to fullpath for comparison */
      if ((ret = qdmIntf_intfnameToFullPathLocked(intfName, TRUE, &fullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmIntf_intfnameToFullPath for %s returns error. ret=%d",
                    intfName, ret);
      }
      else
      {
         cmsLog_debug("l2IntfName %s ==> %s", intfName, fullPath);

         INIT_INSTANCE_ID_STACK(&iidStack);
         while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_QUEUE, &iidStack,
                                           OGF_NO_VALUE_UPDATE,
                                           (void **)&qObj)) == CMSRET_SUCCESS)
         {
            if (cmsUtl_strstr(fullPath, TR181_WIFI_INTF_PATH) != NULL &&
                  !cmsUtl_strcmp(qObj->interface, fullPath))
            {
               /* each Wifi.SSID has 8 queues, but we only want to do default
                * policy for each interface once.
                */
               if (qObj->enable &&
                   !cmsUtl_strcmp(qObj->status, MDMVS_ENABLED))
               {
                  rutQos_doDefaultWlPolicy(intfName, TRUE);
               }
               else
               {
                  rutQos_doDefaultWlPolicy(intfName, FALSE);
               }
               found = 1;
            }
            cmsObj_free((void **)&qObj);
            if(found)
               break;
         }
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      }
   }
#endif  /* BRCM_WLAN */


   /* Delete and then re-add the DSCP marks to make sure they are last (well,
    * close to last... I am following the same order as rclQos_classConfig */
   {
      Dev2QosObject *qosObj=NULL;
      CmsRet r2;

      INIT_INSTANCE_ID_STACK(&iidStack);
      r2 = cmsObj_get(MDMOID_DEV2_QOS, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&qosObj);
      if (r2 != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get QOS obj, ret=%d", ret);
      }
      else
      {
         rutQos_doDefaultDSCPMarkPolicy(QOS_COMMAND_UNCONFIG, qosObj->defaultDSCPMark);
         rutQos_doDefaultDSCPMarkPolicy(QOS_COMMAND_CONFIG, qosObj->defaultDSCPMark);
         cmsObj_free((void **)&qosObj);
      }
   }


   /* these are the hardcoded priority settings for various protocols */
   rutQos_doDefaultPolicy();

#ifdef SUPPORT_FCCTL
   rut_doSystemAction("rutQos_reconfigAllClassifications", "fcctl flush --silent");
#endif

   cmsLog_debug("Exit");

   return;
}


CmsRet rutQos_qMgmtClassDelete_dev2(const char *l3IntfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack savedIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   char *l3FullPath=NULL;
   CmsRet ret;

   cmsLog_debug("Entered: l3IntfName=%s", l3IntfName);

   /* so we can still do path conversion on the intf that is being deleted */
   {
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;
      ret = qdmIntf_intfnameToFullPathLocked(l3IntfName, FALSE, &l3FullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not convert %s to fullpath, ret=%d", l3IntfName, ret);
      }
      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
   }

   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }
   else
   {
      cmsLog_debug("l3IntfName %s ==> %s", l3IntfName, l3FullPath);
   }

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&cObj)) == CMSRET_SUCCESS)
   {
      CmsRet r2;

      /*
       * If there is a match on the L3 ingress interface fullpath or the
       * L3 egress interface name, delete it.
       */
      if (!cmsUtl_strcmp(cObj->interface, l3FullPath) ||
          !cmsUtl_strcmp(cObj->X_BROADCOM_COM_egressInterface, l3IntfName))
      {
         r2 = cmsObj_deleteInstance(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Delete of Class instance at %d failed, ret=%d",
                         PEEK_INSTANCE_ID(&iidStack), r2);
         }
         else
         {
            /* since we deleted this instance, restore iidStack to last
             * non-deleted one.
             */
            iidStack = savedIidStack;
         }
      }

      cmsObj_free((void **) &cObj);

      /* save this iidStack in case we delete the next one */
      savedIidStack = iidStack;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(l3FullPath);

   return CMSRET_SUCCESS;
}


void rutQos_deleteClassByEgressQueueInstance_dev2(SINT32 queueInstance)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack savedIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   CmsRet ret;

   cmsLog_debug("Entered: queueInstance=%d", queueInstance);

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&cObj)) == CMSRET_SUCCESS)
   {
      CmsRet r2;

      if (cObj->X_BROADCOM_COM_ClassQueue == queueInstance)
      {
         r2 = cmsObj_deleteInstance(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Delete of Class instance at %d failed, ret=%d",
                         PEEK_INSTANCE_ID(&iidStack), r2);
         }
         else
         {
            /* since we deleted this instance, restore iidStack to last
             * non-deleted one.
             */
            iidStack = savedIidStack;
         }
      }

      cmsObj_free((void **) &cObj);

      /* save this iidStack in case we delete the next one */
      savedIidStack = iidStack;
   }

   return;
}


UBOOL8 rutQos_isClassificationChanged_dev2(const Dev2QosClassificationObject *newObj,
                                  const Dev2QosClassificationObject *currObj)
{
   /* Note this function does not check all the params in the
    * classification object.  Only those that directly affect how the
    * classification is configured into the kernel.
    */
   if (newObj->enable != currObj->enable ||
       newObj->ethertype != currObj->ethertype ||
       newObj->ethertypeExclude != currObj->ethertypeExclude ||
       cmsUtl_strcmp(newObj->interface, currObj->interface) ||
       cmsUtl_strcmp(newObj->destIP, currObj->destIP) ||
       cmsUtl_strcmp(newObj->destMask, currObj->destMask) ||
       newObj->destIPExclude != currObj->destIPExclude ||
       cmsUtl_strcmp(newObj->sourceIP, currObj->sourceIP) ||
       cmsUtl_strcmp(newObj->sourceMask, currObj->sourceMask) ||
       newObj->sourceIPExclude != currObj->sourceIPExclude ||
       newObj->protocol != currObj->protocol ||
       newObj->protocolExclude != currObj->protocolExclude ||
       newObj->destPort != currObj->destPort ||
       newObj->destPortRangeMax != currObj->destPortRangeMax ||
       newObj->destPortExclude != currObj->destPortExclude ||
       newObj->sourcePort != currObj->sourcePort ||
       newObj->sourcePortRangeMax != currObj->sourcePortRangeMax ||
       newObj->sourcePortExclude != currObj->sourcePortExclude ||
       cmsUtl_strcmp(newObj->destMACAddress, currObj->destMACAddress) ||
       cmsUtl_strcmp(newObj->destMACMask, currObj->destMACMask) ||
       newObj->destMACExclude != currObj->destMACExclude ||
       cmsUtl_strcmp(newObj->sourceMACAddress, currObj->sourceMACAddress) ||
       cmsUtl_strcmp(newObj->sourceMACMask, currObj->sourceMACMask) ||
       newObj->sourceMACExclude != currObj->sourceMACExclude ||
       cmsUtl_strcmp(newObj->sourceVendorClassID, currObj->sourceClientID) ||
       newObj->sourceVendorClassIDExclude != currObj->sourceVendorClassIDExclude ||
       cmsUtl_strcmp(newObj->sourceUserClassID, currObj->sourceUserClassID) ||
       newObj->sourceUserClassIDExclude != currObj->sourceUserClassIDExclude ||
       newObj->DSCPCheck != currObj->DSCPCheck ||
       newObj->DSCPExclude != currObj->DSCPExclude ||
       newObj->DSCPMark != currObj->DSCPMark ||
       newObj->ethernetPriorityCheck != currObj->ethernetPriorityCheck ||
       newObj->ethernetPriorityExclude != currObj->ethernetPriorityExclude ||
       newObj->ethernetPriorityMark != currObj->ethernetPriorityMark ||
       newObj->X_BROADCOM_COM_VLANIDTag != currObj->X_BROADCOM_COM_VLANIDTag ||
       cmsUtl_strcmp(newObj->X_BROADCOM_COM_egressInterface, currObj->X_BROADCOM_COM_egressInterface) ||
       newObj->X_BROADCOM_COM_ClassQueue != currObj->X_BROADCOM_COM_ClassQueue ||
       newObj->X_BROADCOM_COM_ClassPolicer != currObj->X_BROADCOM_COM_ClassPolicer ||
       newObj->X_BROADCOM_COM_ClassRate != currObj->X_BROADCOM_COM_ClassRate)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


CmsRet rutQos_fillClassKeyArray_dev2(UINT32 *keyArray)
{
   InstanceIdStack iidStack;
   Dev2QosClassificationObject *cObj = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **)&cObj)) == CMSRET_SUCCESS)
   {
      if (cObj->X_BROADCOM_COM_ClassKey < 1 || cObj->X_BROADCOM_COM_ClassKey > QOS_CLS_MAX_ENTRY)
      {
         cmsLog_error("Found invalid clsKey %d", cObj->X_BROADCOM_COM_ClassKey);
         ret = CMSRET_INTERNAL_ERROR;
         cmsObj_free((void **)&cObj);
         break;
      }

      keyArray[cObj->X_BROADCOM_COM_ClassKey - 1] = 1;
      cmsObj_free((void **)&cObj);
   }

   if (ret == CMSRET_NO_MORE_INSTANCES)
   {
      ret = CMSRET_SUCCESS;
   }

   return ret;
}


/** When rutQos_reconfigAllClassifications_dev2 is called with some
 *  Layer 2 or Layer 3 intfName, it needs to know if this intfName is
 *  referenced by any classifications, on either ingress or egress.  If so,
 *  we need to do reconfig.  (But if not, we can save ourselves some
 *  work and do nothing).
 *
 *  @intfName  (IN) Linux interface name to check
 *
 *  @return TRUE if given intfName is referenced by any classifications.
 */
UBOOL8 isAffectClassifications_dev2(const char *intfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   char *l2FullPath=NULL;
   char *l3FullPath=NULL;
   UBOOL8 referenced=FALSE;

   cmsLog_debug("Entered: intfName=%s", intfName);

   /* intfName may be L2 or L3, so do fullpath conversion on both */
   {
      /* so we can still do path conversion on the intf that is being deleted */
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;
      qdmIntf_intfnameToFullPathLocked(intfName, TRUE, &l2FullPath);
      qdmIntf_intfnameToFullPathLocked(intfName, FALSE, &l3FullPath);
      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
   }

   cmsLog_debug("intfName %s ==> L2 %s", intfName, l2FullPath);
   cmsLog_debug("intfName %s ==> L3 %s", intfName, l3FullPath);


   while (!referenced &&
          cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &cObj) == CMSRET_SUCCESS)
   {
      if ((l2FullPath && !cmsUtl_strcmp(cObj->interface, l2FullPath)) ||
          (l3FullPath && !cmsUtl_strcmp(cObj->interface, l3FullPath)) ||
          !cmsUtl_strcmp(cObj->X_BROADCOM_COM_egressInterface, intfName))
      {
         cmsLog_debug("Affects [%s] ingress=%s egress=%s",
                      cObj->X_BROADCOM_COM_ClassName,
                      cObj->interface, cObj->X_BROADCOM_COM_egressInterface);
         referenced = TRUE;
      }

      cmsObj_free((void **)&cObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(l2FullPath);
   CMSMEM_FREE_BUF_AND_NULL_PTR(l3FullPath);

   return referenced;
}


UBOOL8 rutQos_isAnotherClassPolicerExist_dev2(UINT32 excludeClsKey,
                                              const char *egressL2IntfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   UBOOL8 exist=FALSE;

   cmsLog_debug("Entered: egressL2IntfName=%s exclude=%d", egressL2IntfName, excludeClsKey);

   /* strangely, the algorithm for determining another policer does not
    * look at policer properties.  It only looks for matching egress
    * L2 IntfName.
    */
   while (!exist &&
          cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &cObj) == CMSRET_SUCCESS)
   {
      if (cObj->X_BROADCOM_COM_ClassKey != excludeClsKey &&
          cObj->enable &&
          !cmsUtl_strcmp(cObj->status, MDMVS_ENABLED) &&
          cObj->X_BROADCOM_COM_ClassQueue > 0)
      {
         CmsRet r2;
         char l2IntfName[CMS_IFNAME_LENGTH]={0};

         r2 = qdmQos_getQueueInfoByClassQueueLocked(cObj->X_BROADCOM_COM_ClassQueue,
                                             NULL, NULL, l2IntfName);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get QueueInfo on %d, ret=%d",
                          cObj->X_BROADCOM_COM_ClassQueue, r2);
         }
         else
         {
            if (cmsUtl_strcmp(egressL2IntfName, l2IntfName) == 0)
            {
               cmsLog_debug("Found exist l2IntfName %s at %d",
                             l2IntfName, cObj->X_BROADCOM_COM_ClassQueue);
               exist = TRUE;
            }
         }
      }

      cmsObj_free((void **)&cObj);
   }

   return exist;
}

CmsRet rutQos_fillPolicerInfo_dev2(const SINT32 instance, const tmctl_dir_e dir, const UINT32 policerInfo)
{
    Dev2QosPolicerObject *pObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret;
    
    if (instance <= 0)
    {
        cmsLog_error("invalid instance=%d", instance);
        return CMSRET_INVALID_ARGUMENTS;
    }
    
    cmsLog_debug("policerUpdateIndex insance %d, dir %d, policerInfo %d.", instance, dir, policerInfo);
    /* the fullpath of the Policer table is Device.QoS.Policer.{i}.
     * so we just need to push the instance number into the first position
     * of the instance id stack.
     */
    PUSH_INSTANCE_ID(&iidStack, instance);
    if ((ret = cmsObj_get(MDMOID_DEV2_QOS_POLICER, &iidStack, 0, (void **) &pObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsObj_get <MDMOID_DEV2_QOS_POLICER> returns error. ret=%d", ret);
        return ret;
    }
    
    if (dir == TMCTL_DIR_UP)
        pObj->X_BROADCOM_COM_UsPolicerInfo = policerInfo;
    else
        pObj->X_BROADCOM_COM_DsPolicerInfo = policerInfo;
    
    if ((ret = cmsObj_set((void *)pObj, &iidStack)) != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsObj_set <MDMOID_DEV2_QOS_POLICER> returns error. ret=%d", ret);	  
    }
    
    cmsObj_free((void **) &pObj);	
    return ret;
}

UBOOL8 rutQos_isAnotherClassRateLimitExist_dev2(UINT32 excludeClsKey,
                                                const char *egressL2IntfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2QosClassificationObject *cObj = NULL;
   UBOOL8 exist=FALSE;

   cmsLog_debug("Entered: egressL2IntfName=%s exclude=%d", egressL2IntfName, excludeClsKey);

   while (!exist &&
          cmsObj_getNextFlags(MDMOID_DEV2_QOS_CLASSIFICATION, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &cObj) == CMSRET_SUCCESS)
   {
      if (cObj->X_BROADCOM_COM_ClassKey != excludeClsKey &&
          cObj->X_BROADCOM_COM_ClassRate != QOS_RESULT_NO_CHANGE &&
          cObj->enable &&
          !cmsUtl_strcmp(cObj->status, MDMVS_ENABLED) &&
          cObj->X_BROADCOM_COM_ClassQueue > 0)
      {
         CmsRet r2;
         char l2IntfName[CMS_IFNAME_LENGTH]={0};

         r2 = qdmQos_getQueueInfoByClassQueueLocked(cObj->X_BROADCOM_COM_ClassQueue,
                                             NULL, NULL, l2IntfName);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get QueueInfo on %d, ret=%d",
                          cObj->X_BROADCOM_COM_ClassQueue, r2);
         }
         else
         {
            if (cmsUtl_strcmp(egressL2IntfName, l2IntfName) == 0)
            {
               cmsLog_debug("Found exist l2IntfName %s at %d",
                             l2IntfName, cObj->X_BROADCOM_COM_ClassQueue);
               exist = TRUE;
            }
         }
      }

      cmsObj_free((void **)&cObj);
   }

   return exist;
}

#endif  /* DMP_DEVICE2_QOS_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */

