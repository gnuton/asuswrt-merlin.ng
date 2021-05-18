/***********************************************************************
 *
 *  Copyright (c) 2010  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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

#ifdef SUPPORT_QOS
/*
 * This file contains mostly TR98 specific functions, but there are some
 * common functions which are used by both TR98 and TR181.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <ctype.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "bcmnet.h"
#include "cms_core.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_eid.h"
#include "cms_qos.h"
#include "cms_dal.h"
#include "cms_boardcmds.h"
#include "rcl.h"
#include "odl.h"
#include "rut_util.h"
#include "rut_qos.h"
#include "rut_atm.h"
#include "rut_dsl.h"
#include "rut_lan.h"
#include "rut_wan.h"
#include "rut_wanlayer2.h"
#include "rut_xtmlinkcfg.h"
#include "bcmxtmcfg.h"
#include "devctl_xtm.h"
#include "skb_defines.h"
#include "rut_tmctl_wrap.h"




/** Local functions **/

#ifdef SUPPORT_DSL
static CmsRet rutQos_xtmQueueConfig(QosCommandType cmdType, CmsQosQueueInfo *qInfo);
#endif

#if defined(SUPPORT_TMCTL)
static CmsRet rutQos_tmGetDevInfoByIfName(const char *l2ifname, 
                             tmctl_devType_e *devType_p,
                             tmctl_if_t *if_p);
#endif

CmsRet rutQos_qMgmtQueueConfig(QosCommandType cmdType, const void *qObj)
{
   CmsQosQueueInfo qInfo;
   CmsRet ret;
   
   memset(&qInfo, 0, sizeof(qInfo));
   /* convert TR98 or TR181 QoS qObj to common qinfo struct */
   qdmQos_convertDmQueueObjToCmsQueueInfoLocked(qObj, &qInfo);


   cmsLog_debug("%s queue instance:", (cmdType == QOS_COMMAND_CONFIG)? "Config" : "Unconfig");
   cmsLog_debug("Enable=%d", qInfo.enable);
   cmsLog_debug("Interface=%s", qInfo.intfName);
   cmsLog_debug("QueueId=%d", qInfo.queueId);
   cmsLog_debug("QueueName=%s", qInfo.queueName);
   cmsLog_debug("Scheduler=%s", qInfo.schedulerAlgorithm);
   cmsLog_debug("Precedence=%d", qInfo.queuePrecedence);
   cmsLog_debug("Weight=%d", qInfo.queueWeight);
   cmsLog_debug("DropAlg=%s", qInfo.dropAlgorithm);
   cmsLog_debug("LoMinThreshold=%u", qInfo.loMinThreshold);
   cmsLog_debug("LoMaxThreshold=%u", qInfo.loMaxThreshold);
   cmsLog_debug("HiMinThreshold=%u", qInfo.hiMinThreshold);
   cmsLog_debug("HiMaxThreshold=%u", qInfo.hiMaxThreshold);
   cmsLog_debug("DslLatency=%d", qInfo.dslLatency);
   cmsLog_debug("PtmPriority=%d", qInfo.ptmPriority);

   /* configure the queue only if it is enabled */
   if (!qInfo.enable)
   {
      cmsLog_debug("queue is not enabled. No action is taken.");
      return CMSRET_SUCCESS;
   }

   if (!qdmQos_isQosEnabled())
   {
      /* enable the queue if qos is disabled, to allow traffic */
      cmsLog_debug("Enable the queue because qos is disabled.");
      cmdType = QOS_COMMAND_CONFIG;
      qInfo.enable = 1;
   }

   if (IS_EMPTY_STRING(qInfo.intfName))
   {
      if (cmdType == QOS_COMMAND_CONFIG)
      {
         cmsLog_error("Config intfName is empty");
         return CMSRET_INVALID_ARGUMENTS;
      }
      cmsLog_debug("Unconfig queueInterface is empty. No action is taken.");
      return CMSRET_SUCCESS;
   }

   if (!cmsUtl_strncmp(qInfo.intfName, ETH_IFC_STR, strlen(ETH_IFC_STR)) ||
       !cmsUtl_strncmp(qInfo.intfName, MOCA_IFC_STR, strlen(MOCA_IFC_STR)))
   {
      /* Ethernet or MoCA interface : could be LAN or WAN */
      if ((ret = rutQos_tmQueueConfig(cmdType, &qInfo)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_tmQueueConfig returns error. ret=%d", ret);
      }

      if ((!qInfo.isWan) && (cmdType == QOS_COMMAND_UNCONFIG))
      {
          UINT32 qNum;

          /* Re-create default queues. */
          rutQos_getIntfNumOfCreatedQueues(qObj, &qNum);

          if (qNum == 0)
          {
             rutQos_tmPortInit(qInfo.intfName, qInfo.isWan);
          }
      }
   }
#if defined(SUPPORT_DSL)
   else if (!cmsUtl_strncmp(qInfo.intfName, ATM_IFC_STR, strlen(ATM_IFC_STR)) ||
            !cmsUtl_strncmp(qInfo.intfName, PTM_IFC_STR, strlen(PTM_IFC_STR)))
   {
      if ((ret = rutQos_xtmQueueConfig(cmdType, &qInfo)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_xtmQueueConfig returns error. ret=%d", ret);
      }
   }
#endif   
   else if (((cmsUtl_strstr(qInfo.intfName, "GPON")) != NULL) ||
            ((cmsUtl_strstr(qInfo.intfName, "Epon")) != NULL) ||
            ((cmsUtl_strstr(qInfo.intfName, "EPON")) != NULL) ||
            (!cmsUtl_strncmp(qInfo.intfName, EPON_IFC_STR, strlen(EPON_IFC_STR))) ||
            (!cmsUtl_strncmp(qInfo.intfName, GPON_IFC_STR, strlen(EPON_IFC_STR))))
   {
      if ((ret = rutQos_tmQueueConfig(cmdType, &qInfo)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_tmQueueConfig returns error. name=%s, ret=%d", qInfo.intfName, ret);
      }
   }
   else
   {
      cmsLog_debug("No action taken on interface [%s]", qInfo.intfName);
      ret = CMSRET_SUCCESS;
   }

   return ret;
}  /* End of rutQos_qMgmtQueueConfig() */


#ifdef SUPPORT_DSL
CmsRet rutQos_xtmQueueConfig(QosCommandType cmdType, CmsQosQueueInfo *qInfo)
{
   UINT32 i;
   UINT8  subprio;
   UINT8  qid;
   UBOOL8 qExist;

   XTM_ADDR addr;
   XTM_CONN_CFG connCfg;
   PXTM_TRANSMIT_QUEUE_PARMS pTxQ;
   CmsRet ret;

   if (qInfo->queueId == 1)
   {
      cmsLog_debug("cannot add or delete the default queue which is associated with the xtm connection.");
      return CMSRET_SUCCESS;
   }

   if (cmsUtl_strstr(qInfo->intfName, ATM_IFC_STR) ||
       cmsUtl_strstr(qInfo->intfName, IPOA_IFC_STR))
   {
      if (qInfo->queueId < 1 || qInfo->queueId > MAX_ATM_TRANSMIT_QUEUES)
      {
         cmsLog_error("Invalid ATM queue ID %d", qInfo->queueId);
         return CMSRET_INVALID_ARGUMENTS;
      }
   }
   else
   {
      if (qInfo->queueId < 1 || qInfo->queueId > MAX_PTM_TRANSMIT_QUEUES)
      {
         cmsLog_error("Invalid PTM queue ID %d", qInfo->queueId);
         return CMSRET_INVALID_ARGUMENTS;
      }
   }
   
   if (qInfo->queuePrecedence < 1 || qInfo->queuePrecedence > XTM_QOS_LEVELS)
   {
      cmsLog_error("Invalid queue precedence %d", qInfo->queuePrecedence);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* qid is 0 based in xtm */
   qid = qInfo->queueId - 1;

   /* subpriority is 0 based in xtm and is in reverse order of the queue precedence.
    * i.e. the higher the queue precedence value, the lower the subpriority value.
    * precedence(1) is subpriority(7); precedence(8) is subpriority(0);.....
    */
   subprio = XTM_QOS_LEVELS - qInfo->queuePrecedence;

   memset(&addr, 0, sizeof(addr));
   memset(&connCfg, 0, sizeof(connCfg));

   if (cmsUtl_strstr(qInfo->intfName, ATM_IFC_STR) || cmsUtl_strstr(qInfo->intfName, IPOA_IFC_STR))
   {
      UINT16 vpi, vci;
      UINT32 interfaceId;
      
      if ((ret = qdmXtm_getAtmIntfInfoByNameLocked(qInfo->intfName, &vpi, &vci,
                                             &interfaceId)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmxtm_getAtmIntfInfoByNameLocked returns error %s", ret);
         return ret;
      }

      addr.ulTrafficType    = TRAFFIC_TYPE_ATM;
      addr.u.Vcc.ulPortMask = PORTID_TO_PORTMASK(interfaceId);
      addr.u.Vcc.usVpi      = vpi;
      addr.u.Vcc.usVci      = vci;

      if ((ret = devCtl_xtmGetConnCfg(&addr, &connCfg)) != CMSRET_SUCCESS)
      {
         if (cmdType == QOS_COMMAND_UNCONFIG)
         {
            cmsLog_debug("devCtl_xtmGetConnCfg returns error. ret=%d. Skipping queue unconfig.", ret);
         }
         else
         {
            cmsLog_debug("devCtl_xtmGetConnCfg returns error. ret=%d. Skipping queue config.", ret);
         }
         return CMSRET_SUCCESS;
      }
   }
#ifdef SUPPORT_PTM
   else if (cmsUtl_strstr(qInfo->intfName, PTM_IFC_STR))
   {
      UINT32 ptmPortId;
      UBOOL8 ptmPriorityLow;
      UBOOL8 ptmPriorityHigh;
      
      if ((ret = qdmXtm_getPtmIntfInfoByNameLocked(qInfo->intfName, &ptmPortId,
                        &ptmPriorityLow, &ptmPriorityHigh)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmxtm_getPtmIntfInfoByNameLocked returns error %s", ret);
         return ret;
      }
      
      addr.ulTrafficType    = TRAFFIC_TYPE_PTM;
      addr.u.Vcc.ulPortMask = PORTID_TO_PORTMASK(ptmPortId);
      if (ptmPriorityHigh == TRUE && ptmPriorityLow  == TRUE)
      {
         addr.u.Flow.ulPtmPriority = PTM_PRI_HIGH | PTM_PRI_LOW;
      }
      else if (ptmPriorityHigh == TRUE)
      {
         addr.u.Flow.ulPtmPriority = PTM_PRI_HIGH;
      }
      else if (ptmPriorityLow == TRUE)
      {
         addr.u.Flow.ulPtmPriority = PTM_PRI_LOW;
      }
      else
      {
         cmsLog_error("PTM priority is neither LOW nor HIGH.");
         return CMSRET_INVALID_ARGUMENTS;
      }

      if ((ret = devCtl_xtmGetConnCfg(&addr, &connCfg)) != CMSRET_SUCCESS)
      {
         if (cmdType == QOS_COMMAND_UNCONFIG)
         {
            cmsLog_debug("devCtl_xtmGetConnCfg returns error. ret=%d. Skipping queue unconfig.", ret);
         }
         else
         {
            cmsLog_debug("devCtl_xtmGetConnCfg returns error. ret=%d. Skipping queue config.", ret);
         }
         return CMSRET_SUCCESS;
      }
   }
#endif
   else
   {
      cmsLog_error("could not find info for %s", qInfo->intfName);
      return CMSRET_INVALID_ARGUMENTS;
   }

   qExist = FALSE;
   for (i = 0, pTxQ = connCfg.TransmitQParms; i < connCfg.ulTransmitQParmsSize; i++, pTxQ++)
   {
      if (pTxQ->ucQosQId == qid &&
          pTxQ->ulPortId == PORTID_TO_PORTMASK(qInfo->dslLatency) &&
          (qInfo->ptmPriority == -1 ||
           pTxQ->ulPtmPriority == (qInfo->ptmPriority? PTM_PRI_HIGH : PTM_PRI_LOW)))
      {
         /* queue exists */
         qExist = TRUE;
         break;
      }
   }

   if (qExist)
   {
      if (cmdType == QOS_COMMAND_CONFIG)
      {
         /* add queue */
         cmsLog_debug("Queue already exists. Need not add.");
         return CMSRET_SUCCESS;
      }
      else
      {
         /* delete queue */
         PXTM_TRANSMIT_QUEUE_PARMS pTxQ2;

         for (i++, pTxQ2 = pTxQ + 1; i < connCfg.ulTransmitQParmsSize; i++, pTxQ++, pTxQ2++)
         {
            memcpy(pTxQ, pTxQ2, sizeof(XTM_TRANSMIT_QUEUE_PARMS));
         }
         connCfg.ulTransmitQParmsSize--;
      }
   }
   else
   {
      if (cmdType == QOS_COMMAND_CONFIG)
      {
         /* add queue */
         memset(pTxQ, 0, sizeof(XTM_TRANSMIT_QUEUE_PARMS));
         pTxQ->ucQosQId = qid;
         pTxQ->ulPortId = PORTID_TO_PORTMASK(qInfo->dslLatency);
         pTxQ->usSize   = HOST_XTM_NR_TXBDS;

         if (strcmp(qInfo->schedulerAlgorithm, MDMVS_WFQ) == 0)
         {
            pTxQ->ucWeightAlg = WA_WFQ;
         }
         else  /* WRR or SP */
         {
            pTxQ->ucWeightAlg = WA_CWRR;
         }
         pTxQ->ulWeightValue = qInfo->queueWeight;
         pTxQ->ucSubPriority = subprio;

         if (!cmsUtl_strcmp(qInfo->dropAlgorithm, MDMVS_RED))
         {
            pTxQ->ucDropAlg = WA_RED;
         }
         else if (!cmsUtl_strcmp(qInfo->dropAlgorithm, MDMVS_WRED))
         {
            pTxQ->ucDropAlg = WA_WRED;
         }
         else
         {
            pTxQ->ucDropAlg = WA_DT;
         }
         pTxQ->ucLoMinThresh = qInfo->loMinThreshold;
         pTxQ->ucLoMaxThresh = qInfo->loMaxThreshold;
         pTxQ->ucHiMinThresh = qInfo->hiMinThreshold;
         pTxQ->ucHiMaxThresh = qInfo->hiMaxThreshold;
   
         if (qInfo->minBitRate > QOS_QUEUE_NO_SHAPING)
         {
            pTxQ->ulMinBitRate = qInfo->minBitRate;
         }
         else
         {
            pTxQ->ulMinBitRate = 0;  /* no shaping */
         }
         if (qInfo->shapingRate > QOS_QUEUE_NO_SHAPING)
         {
            pTxQ->ulShapingRate = qInfo->shapingRate;
         }
         else
         {
            pTxQ->ulShapingRate = 0;  /* no shaping */
         }
         pTxQ->usShapingBurstSize = qInfo->shapingBurstSize;
         
         if (qInfo->ptmPriority == 0)
         {
            pTxQ->ulPtmPriority = PTM_PRI_LOW;
         }
         else if (qInfo->ptmPriority == 1)
         {
            pTxQ->ulPtmPriority = PTM_PRI_HIGH;
         }
         connCfg.ulTransmitQParmsSize++;
      }
      else
      {
         /* delete queue */
         cmsLog_debug("Queue does not exist. Need not delete.");
         return CMSRET_SUCCESS;
      }
   }

   if ((ret = devCtl_xtmSetConnCfg(&addr, &connCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("devCtl_xtmSetConnCfg returns error. ret=%d", ret);
   }

   return ret;

}  /* End of rutQos_xtmQueueConfig() */
#endif /* SUPPORT_DSL */


UINT32 rutQos_getQidMark(const char *queueIntfName,
                         UBOOL8 isWan,
                         UINT32 queueId,
                         UINT32 queuePrecedence,
                         char *schedulerAlgorithm,
                         SINT32 dslLat,
                         SINT32 ptmPriority)
{    
   UINT32 dslLatency = 0;
   UINT32 ptmPrio    = 0;
   UINT32 qid        = 0;
   UINT32 qidMark    = 0;

   cmsLog_debug("Entered: intfName=%s isWan=%d queueId=%d prec=%d algo=%s dslLat=%d ptmPrio=%d",
                queueIntfName, isWan, queueId, queuePrecedence,
                schedulerAlgorithm, dslLat, ptmPriority);

   if (!cmsUtl_strncmp(queueIntfName, ATM_IFC_STR, strlen(ATM_IFC_STR)) ||
       !cmsUtl_strncmp(queueIntfName, PTM_IFC_STR, strlen(PTM_IFC_STR)))
   {
      if (ptmPriority == 1)
      {
         ptmPrio = 1;
      }
      if (dslLat == 1)
      {
         dslLatency = 1;
      }
      
      /* For xtm queue, there is no backward compatibility issue.
       * We can always use X_BROADCOM_COM_QueueId as qid.
       * Note that qid mark is zero-based.
       */
      qid = queueId - 1;
   }
   else
   {
      if (cmsUtl_strcmp(schedulerAlgorithm, MDMVS_SP) == 0)
      {
         /* For non-xtm SP queue, there is backward compatibility issue
          * because X_BROADCOM_COM_QueueId of the old Eth egress queue
          * did not follow the current assignment scheme.
          * Therefore, we have to use queue priority as qid.
          * Note that qid mark is zero-based.
          */    
         if ((!cmsUtl_strncmp(queueIntfName, ETH_IFC_STR, strlen(ETH_IFC_STR)) ||
              !cmsUtl_strncmp(queueIntfName, MOCA_IFC_STR, strlen(MOCA_IFC_STR))) &&
             isWan)
         {
#if defined(SUPPORT_TMCTL)
            if (rut_tmctl_getQueueMap() == QID_PRIO_MAP_Q0P7)
            {
                qid = queueId - 1;
            }
            else
#endif /* SUPPORT_TMCTL */
            {
               qid = ETHWAN_QOS_LEVELS - queuePrecedence;
            }
         }
#ifdef BRCM_WLAN
         else if (!cmsUtl_strncmp(queueIntfName, WLAN_IFC_STR, strlen(WLAN_IFC_STR)))
         {
            qid = WLAN_QOS_LEVELS - queuePrecedence;
         }
#endif
         else
         {
            /* LANEthernetInterfaceConfig
             * MocaInterfaceConfig
             * WANGPONLinkConfig
             */
#if defined(SUPPORT_TMCTL)
            if (rut_tmctl_getQueueMap() == QID_PRIO_MAP_Q0P7)
            {
                qid = queueId - 1;
            }
            else
#endif /* SUPPORT_TMCTL */
            {
               qid = ETH_QOS_LEVELS - queuePrecedence;
            }
         }
      }
      else
      {
         /* For non-xtm WRR or WFQ queue, there is no backward compatibility
          * issue because the old Eth egress queue only supports SP. This
          * queue must be for TMCTL. We can use X_BROADCOM_COM_QueueId as qid.
          * Note that qid mark is zero-based.
          */
         qid = queueId - 1;
      }
   }
      	       
   /* qid mark bit fields:
    *   ATM- dslLatency(bit4), qid(bit3-0)
    *   PTM- dslLatency(bit4), ptmPriority(bit3), qid(bit2-0)
    */
   qidMark = ((dslLatency << 4) | (ptmPrio << 3) | (qid & 0xf)) & 0x1f;

   cmsLog_debug("qidMark=%d", qidMark);

   return qidMark;

}  /* End of rutQos_getQidMark() */


#ifdef DMP_QOS_1

CmsRet rutQos_qMgmtQueueReconfig_igd(char *ifcname, UBOOL8 layer2Intf)
{
   CmsRet   ret;
   InstanceIdStack iidStack;
   QMgmtQueueObject *qObj = NULL;
   char l2Ifcname[BUFLEN_40];
   SINT32 setClassRule = 0;

   cmsLog_notice("Enter: ifcname=%s layer2Intf=%d", ifcname, layer2Intf);

   if (layer2Intf)
   {
      strcpy(l2Ifcname, ifcname);
   }
   else if ((ret = rutWl2_getL2IfnameFromL3Ifname(ifcname, l2Ifcname)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutWl2_getL2IfnameFromL3Ifname() returns error. ret=%d", ret);
      return ret;
   }

   cmsLog_notice("final layer2 ifname=%s", l2Ifcname);

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_QUEUE, &iidStack, (void **)&qObj)) == CMSRET_SUCCESS)
   {
      char qIfcname[BUFLEN_40];

      qIfcname[0] = '\0';
      if ((ret = qdmIntf_fullPathToIntfnameLocked(qObj->queueInterface, qIfcname)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmIntf_fullPathToIntfnameLocked returns error. ret=%d", ret);
         cmsLog_error("qIntf=%s", qObj->queueInterface);
         cmsObj_free((void **)&qObj);
         return ret;
      }
      if (cmsUtl_strcmp(l2Ifcname, qIfcname) == 0)
      {
         cmsLog_debug("found matching queue object");

         /* reconfig the queue object instance, but not the class rules
          * associated with this queue object.
          * Class rules will be reconfig after all the queues have been reconfig.
          */
         /* first config the new queue object instance */
         if ((ret = rutQos_qMgmtQueueConfig(QOS_COMMAND_CONFIG, qObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_qMgmtQueueConfig returns error. ret=%d", ret);
            cmsObj_free((void **)&qObj);
            return ret;
         }

         /* Searching through classificaton rule list, see if there's a rule related to this queue */
         if (!setClassRule)
         {
            InstanceIdStack iidStack2;
            _QMgmtClassificationObject *cObj = NULL;

            INIT_INSTANCE_ID_STACK(&iidStack2);
            while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack2, (void **)&cObj)) == CMSRET_SUCCESS)
            {
               if (cObj->classQueue == (SINT32) iidStack.instance[0])
               {
                  cmsLog_debug("found matching class object");	 
                  setClassRule = 1;
                  cmsObj_free((void **)&cObj);
                  break;
               }
               cmsObj_free((void **)&cObj);
            }
            if (ret != CMSRET_SUCCESS && ret != CMSRET_NO_MORE_INSTANCES)
            {
               cmsLog_error("get Q_MGMT_CLASSIFICATION returns error. ret=%d", ret);
               cmsObj_free((void **)&qObj);
               return ret;
            }
         }
      }
      cmsObj_free((void **)&qObj);
   }
   if (ret != CMSRET_SUCCESS && ret != CMSRET_NO_MORE_INSTANCES)
   {
      cmsLog_error("get Q_MGMT_QUEUE returns error. ret=%d", ret);
      return ret;
   }

   cmsLog_debug("setClassRule=%d", setClassRule);

   /* rclQos_classUnconfig and rclQos_classConfig will do all the classification rules related*/
   if (setClassRule)
   {
      /* First unconfig all the class rules. */
      if ((ret = rclQos_classUnconfig(NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rclQos_classUnconfig returns error. ret=%d", ret);
         return ret;
      }

      /* Then config all the class rules. */
      if ((ret = rclQos_classConfig(NULL, TRUE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rclQos_classConfig returns error. ret=%d", ret);
         return ret;
      }
   }

#ifdef SUPPORT_FCCTL
   char cmd[BUFLEN_64];
   sprintf(cmd, "fcctl flush --if %s --silent ", ifcname);
   rut_doSystemAction("rutQos_qMgmtQueueReconfig", cmd);
#endif
   
   return CMSRET_SUCCESS;

}  /* End of rutQos_qMgmtQueueReconfig_igd() */

/* Delete or unconfigure all the QoS queues associated with a layer 2 interface.
 */
CmsRet rutQos_qMgmtQueueOperation_igd(const char *l2Ifcname, UBOOL8 isRealDel)
{
   CmsRet   ret;
   InstanceIdStack iidStack;
   QMgmtQueueObject *qObj = NULL;
   UBOOL8   prevHideObjectsPendingDelete;
   char     ifcname[CMS_IFNAME_LENGTH];

   cmsLog_debug("enter. ifcname=%s isRealDel=%d", l2Ifcname, isRealDel);

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_QUEUE, &iidStack, (void **)&qObj)) == CMSRET_SUCCESS)
   {
      ifcname[0] = '\0';
      if ((ret = qdmIntf_fullPathToIntfnameLocked(qObj->queueInterface, ifcname)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmIntf_fullPathToIntfnameLocked returns error. ret=%d", ret);
         cmsLog_error("qIntf=%s", qObj->queueInterface);
         cmsObj_free((void **)&qObj);
         return ret;
      }
      if (cmsUtl_strcmp(l2Ifcname, ifcname) == 0)
      {
         if (isRealDel)
         {
            /* Delete all the QoS classes associated with this queue instance */
            if ((ret = rutQos_qMgmtClassOperation(iidStack.instance[iidStack.currentDepth - 1],
                                                  isRealDel)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_qMgmtClassOperation returns error. ret=%d", ret);
               cmsObj_free((void **)&qObj);
               return ret;
            }
            /*
             * This function is typically called when the WANIPConnection object or 
             * WANPPPConnection object is getting deleted.
             * When this queue instance is being deleted, we need to know the interface
             * type of the connection object being deleted.
             * The problem is the MDM will normally hide this object because it is
             * pending a delete.  Set the special mdmLibCtx.hideObjectsPendingDelete
             * variable to FALSE to override this behavior.
             */
            prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
            mdmLibCtx.hideObjectsPendingDelete = FALSE;

            cmsObj_deleteInstance(MDMOID_Q_MGMT_QUEUE, &iidStack);

            mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

            INIT_INSTANCE_ID_STACK(&iidStack);
         }
         else
         {
            /* Unconfig all the QoS classes associated with this queue instance */
            if ((ret = rutQos_qMgmtClassOperation(iidStack.instance[iidStack.currentDepth - 1],
                                                  isRealDel)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_qMgmtClassOperation returns error. ret=%d", ret);
               cmsObj_free((void **)&qObj);
               return ret;
            }
            /* now unconfig the queue instance */
            if ((ret = rutQos_qMgmtQueueConfig(QOS_COMMAND_UNCONFIG, qObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_qMgmtQueueConfig returns error. ret=%d", ret);
               cmsObj_free((void **)&qObj);
               return ret;
            }
         }
      }
      cmsObj_free((void **)&qObj);
   }
   if (ret == CMSRET_NO_MORE_INSTANCES)
   {
      ret = CMSRET_SUCCESS;
   }
   return ret;

}  /* End of rutQos_qMgmtQueueOperation_igd() */

void rutQos_getIntfNumOfCreatedQueues_igd(const void *queueObj,
                                          UINT32 *numQueues)
{
   QMgmtQueueObject *qObj = NULL;
   InstanceIdStack iidStack;
   UINT32 createdQNum = 0;
   CmsRet ret;
   QMgmtQueueObject *pMgmtQ = (_QMgmtQueueObject*)queueObj;
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_QUEUE, &iidStack, (void**)&qObj))
      == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(qObj->queueInterface, pMgmtQ->queueInterface) == 0)
      {
          createdQNum++;
      }
      cmsObj_free((void**)&qObj);
   }

   if (numQueues != NULL)
   {
      *numQueues = createdQNum;
      cmsLog_debug("qIntf=%s, qnum=%d", pMgmtQ->queueInterface, createdQNum);
   }

}

#endif  /* DMP_QOS_1 */


void rutQos_getIntfNumQueuesAndLevels(const char *l2IfName,
                                      UINT32 *numQueues, UINT32 *numLevels)
{
   UINT32 maxLevels;
   UINT32 txqs;

   if (cmsUtl_strstr(l2IfName, ATM_IFC_STR) || cmsUtl_strstr(l2IfName, IPOA_IFC_STR))
   {
      txqs = MAX_ATM_TRANSMIT_QUEUES;
      maxLevels = XTM_QOS_LEVELS;
   }
   else if (cmsUtl_strstr(l2IfName, PTM_IFC_STR))
   {
      txqs = MAX_PTM_TRANSMIT_QUEUES;
      maxLevels = XTM_QOS_LEVELS;
   }
   else if (cmsUtl_strstr(l2IfName, GPON_IFC_STR) || cmsUtl_strstr(l2IfName, EPON_IFC_STR))
   {
      txqs = MAX_PON_TRANSMIT_QUEUES;
      maxLevels = PON_QOS_LEVELS;
   }
   else if (cmsUtl_strstr(l2IfName, WLAN_IFC_STR))
   {
      txqs = MAX_WLAN_TRANSMIT_QUEUES;
      maxLevels = WLAN_QOS_LEVELS;
   }
   else
   {
      /*
       * All matches based on ifname have failed, so it must be an eth
       * interface, right?  Or assume it is?  Print warning if not.
       */
      UBOOL8 isEthWan;

      if ((cmsUtl_strstr(l2IfName, ETH_IFC_STR) == NULL) &&
          (cmsUtl_strstr(l2IfName, MOCA_IFC_STR) == NULL))
      {
         cmsLog_error("Unsupported l2IfName %s, assume eth");
      }

      isEthWan = qdmIntf_isLayer2IntfNameUpstreamLocked(l2IfName);
      if (isEthWan)
      {
         txqs = MAX_ETHWAN_TRANSMIT_QUEUES;
         maxLevels = ETHWAN_QOS_LEVELS;
      }
      else
      {
         txqs = MAX_ETH_TRANSMIT_QUEUES;
         maxLevels = ETH_QOS_LEVELS;
      }
   }

   cmsLog_debug("l2IfName=%s txqs=%d maxLevels=%d ",
                l2IfName, txqs, maxLevels);

   if (numLevels != NULL)
   {
      *numLevels = maxLevels;
   }

   if (numQueues != NULL)
   {
      *numQueues = txqs;
   }
}


CmsRet rutQos_convertPrecedenceToPriority(const char *l2IfName, UINT32 prec,
                                          UINT32 *prio)
{
   UINT32 maxLevel;

   rutQos_getIntfNumQueuesAndLevels(l2IfName, NULL, &maxLevel);

   if (prec < 1 || prec > maxLevel)
   {
      cmsLog_error("Precedence val %d is out of range [1-%d]", prec, maxLevel);
      return CMSRET_INVALID_ARGUMENTS;
   }

   *prio = 1 + maxLevel - prec;

   cmsLog_debug("maxLevel=%d prec=%d ==> prio=%d", maxLevel, prec, *prio);

   return CMSRET_SUCCESS;

}  /* End of rutQos_convertPrecedenceToPriority() */


/* This API is called by WEB-GUI. It supports the queue ID assignment
 * in two QID-priority mapping schemes: Q7P7 and Q0P7.
 * When the Q7P7 scheme is used:
 * - For SP+WRR+WFQ combo-scheduling, SP qid MUST be higher than any
 *   existing WRR or WFQ queue id.
 * - For WRR or WFQ queue, find the available qid starting from the lowest
 *   qid. Make sure the qid is lower than any of the existing SP qid.
 * When the Q0P7 scheme is used, the SP qid and WRR/WFQ qid range are swapped.
 */
CmsRet rutQos_getAvailableQueueId(const char *l2Ifname,
                                  UINT32 prio, const char *alg, UINT32 *qId)
{
   UINT32 maxQId;
   int id;
   char idArray[MAX_TRANSMIT_QUEUES];
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 checkQId;
   UINT32 startSpQId, endSpQId;
   UINT32 qidPrioMap = QID_PRIO_MAP_Q7P7;

   /* set all qid's to "available" */
   memset(idArray, CMS_QUEUE_SCHED_UNSPEC, sizeof(idArray));

   /* initialize the return queue id to 0 */
   *qId = 0;

   rutQos_getIntfNumQueuesAndLevels(l2Ifname, &maxQId, NULL);

   ret = rutQos_fillQueueIdArray(l2Ifname, maxQId, idArray);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("rutQos_fillQueueIdArray returned error %d, no available QId info", ret);
      return ret;
   }

#if defined(SUPPORT_TMCTL)
   qidPrioMap = rut_tmctl_getQueueMap();
#endif /* SUPPORT_TMCTL */

   if (qidPrioMap == QID_PRIO_MAP_Q7P7)
   {
      checkQId = prio;
      startSpQId = checkQId;
      endSpQId = maxQId;
   }
   else
   {
      checkQId = 1 + maxQId - prio;
      startSpQId = 0;
      endSpQId = checkQId;
   }

   if (cmsUtl_strcmp(alg, MDMVS_SP) == 0)
   {
      if (idArray[checkQId - 1] == CMS_QUEUE_SCHED_UNSPEC)
      {
         for (id = (int)startSpQId; id < (int)endSpQId; id++)
         {
            if (idArray[id] == CMS_QUEUE_SCHED_WRR_WFQ)
            {
               cmsLog_error("SP qid must be in a different block from existing WRR/WFQ qid.");
               ret = CMSRET_INVALID_ARGUMENTS;
               break;
            }
         }
      }
      else
      {
         /* qid has been taken. Something wrong. */
         cmsLog_error("Duplicated SP queue at interface %s.", l2Ifname);
         ret = CMSRET_INVALID_ARGUMENTS;
      }

      if (ret == CMSRET_SUCCESS)
      {
         *qId = checkQId;

         cmsLog_debug("found available qId=%d", *qId);
         idArray[*qId - 1] = CMS_QUEUE_SCHED_SP;  /* for debug display */
      }
   }
   else
   {
      UBOOL8 foundSP = FALSE;

      id = (qidPrioMap == QID_PRIO_MAP_Q7P7) ? 0 : (maxQId - 1);

      while (1)
      {
         if (idArray[id] == CMS_QUEUE_SCHED_UNSPEC)
         {
            if (foundSP)
            {
               cmsLog_error("WRR/WFQ qid must be in a different block from existing SP qid.");
               ret = CMSRET_INVALID_ARGUMENTS;
            }
            else
            {
               *qId = id + 1;  /* queue id starts from 1 */

               cmsLog_debug("found available qId=%d", *qId);
               idArray[*qId - 1] = CMS_QUEUE_SCHED_WRR_WFQ;  /* for debug display */
            }
            break;
         }
         else if (idArray[id] == CMS_QUEUE_SCHED_SP)
         {
            foundSP = TRUE;
         }

         (qidPrioMap == QID_PRIO_MAP_Q7P7) ? id++ : id--;
         if ((id >= (int)maxQId) || (id < 0))
         {
            break;
         }
      }
   }

      if (*qId == 0)
      {
         cmsLog_error("Run out of Queues for %s alg=%s prio=%d",
                      l2Ifname, alg, prio);
         ret = CMSRET_INVALID_ARGUMENTS;
      }

   for (id = 0; id < (int)maxQId; id++)
   {
      cmsLog_debug("idArray[%d]=%c", id, idArray[id]);
   }

   return ret;
}  /* End of rutQos_getAvailableQueueId() */


#ifdef DMP_QOS_1

CmsRet rutQos_fillQueueIdArray_igd(const char *l2IfName, UINT32 maxQId, char *idArray)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   QMgmtQueueObject *qObj = NULL;
   char ifName[CMS_IFNAME_LENGTH];
   CmsRet ret;


   /* loop through the queue table */
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_QUEUE, &iidStack, (void **)&qObj)) == CMSRET_SUCCESS)
   {
      /* convert mdm full path string to queue interface name */
      if ((ret = qdmIntf_fullPathToIntfnameLocked(qObj->queueInterface, ifName)) != CMSRET_SUCCESS)
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
}

static CmsRet rutQos_addAccordingQueueStatsobject(const _QMgmtQueueObject *qObj)
{
   InstanceIdStack stats_iidStack;
   _QMgmtQueueStatsObject *qStatsObj = NULL;
   CmsRet ret;

   /* add a new queue stats object instance */
   INIT_INSTANCE_ID_STACK(&stats_iidStack);
   if ((ret = cmsObj_addInstance(MDMOID_Q_MGMT_QUEUE_STATS, &stats_iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_addInstance returns error, ret=%d", ret);
      return ret;
   }

   /* get the object, it will be initially filled in with default values */
   if ((ret = cmsObj_get(MDMOID_Q_MGMT_QUEUE_STATS, &stats_iidStack, OGF_NO_VALUE_UPDATE, (void **)&qStatsObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_get returns error. ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_Q_MGMT_QUEUE_STATS, &stats_iidStack);
      return ret;
   }

   qStatsObj->X_BROADCOM_COM_QueueId = qObj->X_BROADCOM_COM_QueueId;
   CMSMEM_REPLACE_STRING_FLAGS(qStatsObj->queueInterface, qObj->queueInterface, mdmLibCtx.allocFlags);

   /* set the Queue Stats Object instance */
   if ((ret = cmsObj_set(qStatsObj, &stats_iidStack)) != CMSRET_SUCCESS)
   {
      CmsRet r2;
      cmsLog_error("cmsObj_set returns error, ret = %d", ret);

      /* since set failed, we have to delete the instance that we just added */       
      if ((r2 = cmsObj_deleteInstance(MDMOID_Q_MGMT_QUEUE_STATS, &stats_iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_deleteInstance returns error, r2=%d", r2);
      }

      cmsObj_free((void **)&qStatsObj);
      return ret;
   }

   cmsObj_free((void **)&qStatsObj);
   return CMSRET_SUCCESS;
}

CmsRet rutQos_setDefaultEthQueues_igd(const char *ifname, UBOOL8 isWan)
{
   InstanceIdStack iidStack;
   _QMgmtQueueObject *qObj = NULL;
   CmsQosQueueInfo qInfo;
   UINT32 precedence;
   UINT32 maxQueues;
   UINT32 qid;
   UBOOL8 configExist = FALSE;
   char qname[8];
   char *fullPath=NULL;
   CmsRet ret;
   
   cmsLog_debug("Enter: ifname=%s isWan=%d", ifname, isWan);

   /* convert user friendly interface name to mdm full path */
   if ((ret = qdmIntf_intfnameToFullPathLocked(ifname, TRUE, &fullPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("qdmIntf_intfnameToFullPath for %s returns error. ret=%d", ifname, ret);
      return CMSRET_INVALID_ARGUMENTS;
   }
   /* strip the ending '.' */
   fullPath[strlen(fullPath)-1] = '\0';

   /* see if any QoS queue had been configured */   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_QUEUE, &iidStack, (void **)&qObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(qObj->queueInterface, fullPath) == 0)
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
      if ((ret = cmsObj_addInstance(MDMOID_Q_MGMT_QUEUE, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_addInstance returns error, ret=%d", ret);
         break;
      }

      /* get the object, it will be initially filled in with default values */
      if ((ret = cmsObj_get(MDMOID_Q_MGMT_QUEUE, &iidStack, 0, (void **)&qObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_get returns error. ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_Q_MGMT_QUEUE, &iidStack);
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

      qObj->queueEnable            = TRUE;
      qObj->queuePrecedence        = precedence;
      qObj->X_BROADCOM_COM_QueueId = qid;
      CMSMEM_REPLACE_STRING_FLAGS(qObj->X_BROADCOM_COM_QueueName, qname, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(qObj->queueInterface, fullPath, mdmLibCtx.allocFlags);

      /* set the Queue Object instance */
      if ((ret = cmsObj_set(qObj, &iidStack)) != CMSRET_SUCCESS)
      {
         CmsRet r2;
         cmsLog_error("cmsObj_set returns error, ret = %d", ret);
          
         /* since set failed, we have to delete the instance that we just added */       
         if ((r2 = cmsObj_deleteInstance(MDMOID_Q_MGMT_QUEUE, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_deleteInstance returns error, r2=%d", r2);
         }
         
         cmsObj_free((void **)&qObj);
         break;
      }

      if ((ret = rutQos_addAccordingQueueStatsobject(qObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_addAccordingQueueStatsobject returns error, ret = %d", ret);
         cmsObj_free((void **)&qObj);
         break;
      }

      cmsObj_free((void **)&qObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
   return ret;
}  /* rutQos_setDefaultEthQueues() */


#ifdef BRCM_WLAN
/*
 * This function is called when a TR98 Wifi SSID (interfaces) comes up or down.
 * -- do cmsObj_set on all Q_MGMT_QUEUE objects associated with this ifname.
 *    wlan queues do not need configuration, but the set will also activate
 *    any classifiers associated with this queue.
 * -- call rutQos_defaultWlPolicy to install default classifiers (these do
 *    not have any classification entries in the data model.)
 */
void rutQos_setDefaultWlQueues_igd(const char *ifname, UBOOL8 enabled)
{
   InstanceIdStack iidStack;
   _QMgmtQueueObject *qObj = NULL;
   char *wlanPath = NULL;
   CmsRet ret;

   cmsLog_debug("Enter: ifname=%s enabled=%d", ifname, enabled);

   /* get the full mdm path of the wlan object */
   if ((ret = rut_intfnameToFullPath(ifname, TRUE, &wlanPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_intfnameToFullPath for %s returns error. ret=%d",
                    ifname, ret);
      return;
   }
   /* strip the ending '.' */
   wlanPath[strlen(wlanPath)-1] = '\0';

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_QUEUE, &iidStack, (void **)&qObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(qObj->queueInterface, wlanPath) == 0)
      {
         qObj->queueEnable = enabled;
         if ((ret = cmsObj_set((void *)qObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set <MDMOID_Q_MGMT_QUEUE> returns error. ret = %d", ret);
            cmsObj_free((void **)&qObj);
            break;
         }
      }
      cmsObj_free((void **)&qObj);
   }
   cmsMem_free((void *)wlanPath);

   /* activate/deactivate default classifiers */
   rutQos_doDefaultWlPolicy(ifname, enabled);

   return;

}  /* End of rutQos_setDefaultWlQueues_igd() */
#endif  /* BRCM_WLAN */


void rutQos_updateInterfaceFullPaths(const char *srcIntfFullPath,
                                      const char *destIntfFullPath)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   QMgmtQueueObject *qObj = NULL;
   QMgmtClassificationObject *cObj = NULL;

   cmsLog_debug("Enter: src=%s dest=%s", srcIntfFullPath, destIntfFullPath);

   while (cmsObj_getNextFlags(MDMOID_Q_MGMT_QUEUE, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&qObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(qObj->queueInterface, srcIntfFullPath) == 0)
      {
         CmsRet r2;
         cmsLog_debug("replace Queue inst %s", cmsMdm_dumpIidStack(&iidStack));
         CMSMEM_REPLACE_STRING_FLAGS(qObj->queueInterface, destIntfFullPath, mdmLibCtx.allocFlags);
         /*
          * Just update MDM, but do not trigger a call to the RCL handler
          * function.  As far as the underlying queue is concerned, nothing
          * has changed.  We are only moving the interface from one
          * LAN bridge to another LAN bridge.
          */
         r2 = cmsObj_setFlags((void *)qObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set <MDMOID_Q_MGMT_QUEUE> returns error. ret = %d", r2);
         }
      }
      cmsObj_free((void **)&qObj);
   }


   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNextFlags(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&cObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(cObj->classInterface, srcIntfFullPath) == 0)
      {
         CmsRet r2;
         cmsLog_debug("replace Class inst %s", cmsMdm_dumpIidStack(&iidStack));
         CMSMEM_REPLACE_STRING_FLAGS(cObj->classInterface, destIntfFullPath, mdmLibCtx.allocFlags);
         /*
          * Technically, we should do a regular set, but since a new
          * queue reconfig will be triggered when the interface is moved,
          * just update MDM here... do not trigger a call to the RCL handler.
          */
         r2 = cmsObj_setFlags((void *)cObj, &iidStack, OSF_NO_RCL_CALLBACK);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set <MDMOID_Q_MGMT_CLASS> returns error. ret = %d", r2);
         }
      }
      cmsObj_free((void **)&cObj);
   }


   cmsLog_debug("Exit:");
}

void rutQos_portShapingConfigAll_igd(void)
{
#ifdef DMP_ETHERNETWAN_1
   InstanceIdStack wanIidStack = EMPTY_INSTANCE_ID_STACK;
   WanEthIntfObject *wanEthObj = NULL;
   /* Get QoS Port Shaping information from Ethernet WAN */
   if (rutWl2_getWanEthObject(&wanIidStack, &wanEthObj) == CMSRET_SUCCESS)
   {
      if (wanEthObj->X_BROADCOM_COM_IfName != NULL && wanEthObj->enable && wanEthObj->shapingRate != -1)
      {
        //  printf("\nAZU> @@@ DMP_ETHERNETWAN %s %d  @@@\n",__FUNCTION__,__LINE__);
         rutQos_tmPortShaperCfg(wanEthObj->X_BROADCOM_COM_IfName,
                                wanEthObj->shapingRate,
                                wanEthObj->shapingBurstSize,
                                wanEthObj->status,
                                TRUE);
      }
      cmsObj_free((void **) &wanEthObj);
   }
#endif

   InstanceIdStack lanIidStack = EMPTY_INSTANCE_ID_STACK;
   LanEthIntfObject *lanEthObj = NULL;
   /* Get QoS Port Shaping information from Ethernet LAN */
   while (cmsObj_getNext(MDMOID_LAN_ETH_INTF, &lanIidStack, (void **) &lanEthObj) == CMSRET_SUCCESS)
   {
      if (lanEthObj->X_BROADCOM_COM_IfName != NULL && lanEthObj->enable)
      {
      //    printf("\nAZU> @@@  %s %d  @@@\n",__FUNCTION__,__LINE__);
         rutQos_tmPortShaperCfg(lanEthObj->X_BROADCOM_COM_IfName,
                                lanEthObj->X_BROADCOM_COM_ShapingRate,
                                lanEthObj->X_BROADCOM_COM_ShapingBurstSize,
                                lanEthObj->status,
                                FALSE);
      }
      cmsObj_free((void **) &lanEthObj);
   }

}  /* End of rutQos_portShapingConfigAll() */

#endif  /* DMP_QOS_1 */

#if defined(SUPPORT_TMCTL)

static CmsRet rutQos_tmGetDevInfoByIfName(const char *l2ifname, tmctl_devType_e *devType_p, tmctl_if_t *if_p)
{
   if (cmsUtl_strstr(l2ifname, ETH_IFC_STR))
   {
      *devType_p = TMCTL_DEV_ETH;
      if_p->ethIf.ifname = l2ifname;
   }
   else if (cmsUtl_strstr(l2ifname, MOCA_IFC_STR))
   {
      /* TODO: filled interface infomation depends on device type */
      *devType_p = TMCTL_DEV_ETH;
      if_p->ethIf.ifname = l2ifname;
   }
   else if (cmsUtl_strstr(l2ifname, GPON_IFC_STR))
   {
      /* in HGU mode, we configur GPON interface by using a ETH type */
      *devType_p = TMCTL_DEV_ETH;
      if_p->ethIf.ifname = l2ifname;
   }
   else if (cmsUtl_strstr(l2ifname, EPON_IFC_STR))
   {
      /* TODO: filled interface infomation depends on device type */
      *devType_p = TMCTL_DEV_EPON;
      if_p->ethIf.ifname = l2ifname;
   }
   else if (cmsUtl_strstr(l2ifname, ATM_IFC_STR) || cmsUtl_strstr(l2ifname, PTM_IFC_STR))
   {
      *devType_p = TMCTL_DEV_XTM;
      if_p->xtmIf.ifname = l2ifname;
   }
   else
      return CMSRET_INVALID_ARGUMENTS;

   return CMSRET_SUCCESS;
}


/*******************************************************************************
 *
 * Function: rutQos_tmPortInit
 *
 * Init QoS settings and create a default queue of the specified port.
 *
 *******************************************************************************/
CmsRet rutQos_tmPortInit(const char *l2ifname, UBOOL8 isWan)
{
   CmsRet ret = CMSRET_SUCCESS;
   tmctl_if_t intf;
   tmctl_devType_e devType;
   int tmctl_flags = 0;

   cmsLog_debug("Enter: ifname=%s isWan=%d", l2ifname, isWan);

   if ((ret = rutQos_tmGetDevInfoByIfName(l2ifname, &devType, &intf)) != CMSRET_SUCCESS)
   {
      cmsLog_notice("interface %s is unsupported device type, ret=%d", l2ifname, ret);
      goto Exit;
   }

   tmctl_flags = isWan ? TMCTL_SCHED_TYPE_SP_WRR : TMCTL_SCHED_TYPE_SP;
#ifdef BCM_PON_XRDP
   if ((isWan == TRUE) && (rut_isWanTypeEpon() == TRUE))
   {
       int rc = 0;
       rdpa_epon_mode eponMode = rdpa_epon_none;
       
       rc = rdpaCtl_get_epon_mode(&eponMode);

       if (rc == 0 && (eponMode == rdpa_epon_ctc || eponMode == rdpa_epon_cuc))
           tmctl_flags = TMCTL_SCHED_TYPE_SP;
   }
#endif

   /* Erase VLAN part to create default queues for LAN VLAN group interface (ethX.N) */
   if (!isWan && strchr(l2ifname, '.'))
   {
       *strchr(l2ifname, '.') = 0;
   }

   /* Initialize the default CMS QoS queues. */
   if (rutcwmp_tmctl_portTmInit(devType, &intf, tmctl_flags) == TMCTL_ERROR)
   {
      cmsLog_error("tmctl_portTmInit returns error, ifname=%s", l2ifname);
      ret = CMSRET_INTERNAL_ERROR;
      goto Exit;
   }

   /* TODO: Currently, Runner needs CMS queues to pass traffic. 
      But if CMS creates these default queues on FAP platform, 
      it will enable FAP TM and downgrade throughput of Gigabit 
      Ethernet port. Because customers may not use BRCM CMS but 
      only use tmctl, this platform dependent code should be 
      moved to tmctl in the future. */
#if !defined(SUPPORT_FAPCTL)
   /* Create default queues based on the CWMP default QID/priority scheme. */
   if ((ret = rutQos_setDefaultEthQueues(l2ifname, isWan)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutQos_setDefaultEthQueues returns error. ifname=%s, ret=%d\n", l2ifname, ret);
      goto Exit;
   }
#endif

Exit:
   return ret;
}  /* End of rutQos_tmPortInit() */


/*******************************************************************************
 *
 * Function: rutQos_tmPortUninit
 *
 * Un-init QoS settings of the specified port.
 *
 *******************************************************************************/
CmsRet rutQos_tmPortUninit(const char *l2ifname, UBOOL8 isWan __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   tmctl_if_t intf;
   tmctl_devType_e devType;
   
   cmsLog_debug("Enter: ifname=%s isWan=%d", l2ifname, isWan);
   
   if ((ret = rutQos_tmGetDevInfoByIfName(l2ifname, &devType, &intf)) != CMSRET_SUCCESS)
   {
      cmsLog_notice("interface %s is unsupported device type, ret=%d", l2ifname, ret);
      return ret;
   }

   if (rutcwmp_tmctl_portTmUninit(devType, &intf) == TMCTL_ERROR)
   {
      cmsLog_error("tmctl_portTmUninit returns error");
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
                    
}  /* End of rutQos_tmPortUninit() */


/*******************************************************************************
 *
 * Function: rutQos_tmQueueConfig
 *
 * Configures or Un-configures a queue by queueId of specified port.
 * It configures the queue weight, precedence, scheduler algorithm,
 * shaping rate, shaping burst size, and minimum rate.
 *
 *******************************************************************************/
CmsRet rutQos_tmQueueConfig(QosCommandType cmdType, CmsQosQueueInfo *qInfo)
{
   CmsRet ret = CMSRET_SUCCESS;
   tmctl_if_t    intf;
   tmctl_devType_e devType;
   tmctl_sched_e schedMode;
   UINT32        priority;
   BOOL          bestEffort = FALSE;
   tmctl_queueCfg_t qcfg;

   cmsLog_debug("Entered: ifname=%s isWan=%d qId=%u qWt=%u qPrec=%u sched=%s shapingRate=%d shapingBurstSize=%u minRate=%d",
                qInfo->intfName, qInfo->isWan, qInfo->queueId, qInfo->queueWeight, qInfo->queuePrecedence,
                qInfo->schedulerAlgorithm, qInfo->shapingRate, qInfo->shapingBurstSize, qInfo->minBitRate);
   cmsLog_debug("Entered: dropAlg=%s loMinThr=%u loMaxThr=%u hiMinThr=%u hiMaxThr=%u",
                qInfo->dropAlgorithm, qInfo->loMinThreshold, qInfo->loMaxThreshold,
                qInfo->hiMinThreshold, qInfo->hiMaxThreshold);

   if ((ret = rutQos_tmGetDevInfoByIfName(qInfo->intfName, &devType, &intf)) != CMSRET_SUCCESS)
   {
      cmsLog_notice("interface %s is unsupported device type, ret=%d", qInfo->intfName, ret);
      return ret;
   }

   if(devType == TMCTL_DEV_ETH)
   {
      if (qInfo->isWan)
      {
#if defined(DMP_ETHERNETWAN_1) || defined(DMP_DEVICE2_ETHERNETINTERFACE_1)
         /* Eth Wan */
         if (qInfo->queueId < 1 || qInfo->queueId > MAX_ETHWAN_TRANSMIT_QUEUES)
         {
            cmsLog_error("Invalid Eth WAN queue ID %d", qInfo->queueId);
            return CMSRET_INVALID_ARGUMENTS;
         }
         if (qInfo->queuePrecedence < 1 || qInfo->queuePrecedence > ETHWAN_QOS_LEVELS)
         {
            cmsLog_error("Invalid queue precedence %d", qInfo->queuePrecedence);
            return CMSRET_INVALID_ARGUMENTS;
         }
   
         priority = ETHWAN_QOS_LEVELS - qInfo->queuePrecedence;
         if (priority == 0)
         {
            bestEffort = TRUE;
         }
#else
         return CMSRET_SUCCESS;
#endif   /* DMP_ETHERNETWAN_1 */      
      }
      else
      {
         /* Eth Lan */
         if (qInfo->queueId < 1 || qInfo->queueId > MAX_ETH_TRANSMIT_QUEUES)
         {
            cmsLog_error("Invalid Eth LAN queue ID %d", qInfo->queueId);
            return CMSRET_INVALID_ARGUMENTS;
         }
         if (qInfo->queuePrecedence < 1 || qInfo->queuePrecedence > ETH_QOS_LEVELS)
         {
            cmsLog_error("Invalid queue precedence %d", qInfo->queuePrecedence);
            return CMSRET_INVALID_ARGUMENTS;
         }
   
         priority = ETH_QOS_LEVELS - qInfo->queuePrecedence;
      }

      memset(&qcfg, 0x0, sizeof(tmctl_queueCfg_t));
      qcfg.qid = qInfo->queueId - 1; /* qid is zero based */

#if defined(BCM_PON) || defined(BCM_DSL_XRDP)
      if (cmdType == QOS_COMMAND_UNCONFIG)
        return rutcwmp_tmctl_delQueueCfg(devType, &intf, qcfg.qid);
#endif

      if (cmsUtl_strcmp(qInfo->schedulerAlgorithm, MDMVS_WFQ) == 0)
      {
         schedMode = TMCTL_SCHED_WFQ;
      }
      else if (cmsUtl_strcmp(qInfo->schedulerAlgorithm, MDMVS_WRR) == 0)
      {
         schedMode = TMCTL_SCHED_WRR;
      }
      else
      {
         schedMode = TMCTL_SCHED_SP;
      }

      {
         SINT32           kbps, minKbps;

         kbps    = (qInfo->shapingRate > 0)? qInfo->shapingRate/1000 : 0;
         minKbps = (qInfo->minBitRate > 0)? qInfo->minBitRate/1000 : 0;
   
         /* Add TM queue */
         cmsLog_debug("Add TM queue. ifname=%s qid=%d schedMode=%d qWt=%d shapingRate=%d bestEffort=%d",
                      qInfo->intfName, qInfo->queueId-1, schedMode, qInfo->queueWeight, kbps, bestEffort ? 1 : 0);
                      
         qcfg.priority                = priority;
         if (qInfo->isWan)
         {
             qcfg.qsize               = TMCTL_DEF_ETH_Q_SZ_US;   /* default Ethernet qsize */
         }
         else
         {
             qcfg.qsize               = TMCTL_DEF_ETH_Q_SZ_DS;   /* default Ethernet qsize */
         }
         qcfg.schedMode               = schedMode;
         qcfg.weight                  = qInfo->queueWeight;
         qcfg.shaper.shapingRate      = kbps;
         qcfg.shaper.shapingBurstSize = qInfo->shapingBurstSize;
         qcfg.shaper.minRate          = minKbps;
         qcfg.bestEffort              = bestEffort;
      
         if (cmdType == QOS_COMMAND_UNCONFIG)
            qcfg.qsize = 0;

         if (rutcwmp_tmctl_setQueueCfg(devType, &intf, &qcfg) == TMCTL_ERROR)
         {
            cmsLog_error("Add TM queue. ifname=%s qid=%d priority=%d schedMode=%d qWt=%d shapingRate=%d bestEffort=%d",
                         qInfo->intfName, qInfo->queueId-1, priority, schedMode, qInfo->queueWeight, kbps, bestEffort ? 1 : 0);
            ret = CMSRET_INTERNAL_ERROR;
         }
         else if (qcfg.qsize)
         {
            tmctl_dropAlg_e   dropAlg;
   
            if (cmsUtl_strcmp(qInfo->dropAlgorithm, MDMVS_RED) == 0)
            {
               dropAlg = TMCTL_DROP_RED;
            }
            else if (cmsUtl_strcmp(qInfo->dropAlgorithm, MDMVS_WRED) == 0)
            {
               dropAlg = TMCTL_DROP_WRED;
            }
            else
            {
               dropAlg = TMCTL_DROP_DT;
            }
   
            if (dropAlg != TMCTL_DROP_DT)
            {
               tmctl_queueDropAlg_t dropAlgData;

               dropAlgData.dropAlgorithm = dropAlg;
               dropAlgData.dropAlgLo.redMinThreshold = (qInfo->loMinThreshold * qcfg.qsize) / 100; 
               dropAlgData.dropAlgLo.redMaxThreshold = (qInfo->loMaxThreshold * qcfg.qsize) / 100; 
               dropAlgData.dropAlgHi.redMinThreshold = (qInfo->hiMinThreshold * qcfg.qsize) / 100; 
               dropAlgData.dropAlgHi.redMaxThreshold = (qInfo->hiMaxThreshold * qcfg.qsize) / 100; 
               dropAlgData.dropAlgLo.redPercentage = dropAlgData.dropAlgHi.redPercentage = 100;
               /* FIXME!! prio_mask? default to 0 now */
               dropAlgData.priorityMask0 = dropAlgData.priorityMask1 = 0;
               
               if (rutcwmp_tmctl_setQueueDropAlgExt(devType, &intf, qInfo->queueId-1,
                   &dropAlgData) == TMCTL_ERROR)
               {
                  cmsLog_error("rutcwmp_tmctl_setQueueDropAlgExt returns error");
                  ret = CMSRET_INTERNAL_ERROR;
               }
            }
         }
      }
   }

   return ret;
   
}  /* End of rutQos_tmQueueConfig() */


/*******************************************************************************
 *
 * Function: rutQos_tmPortShaperCfg
 *
 * Configures the port shaper parameters of the specified port.
 * It configures shaper rate in kbps and bust size in bytes.
 *
 *******************************************************************************/
CmsRet rutQos_tmPortShaperCfg(const char* l2ifname, SINT32 portbps, SINT32 portMbs,
                              char* status, UBOOL8 isWan __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   SINT32 portKbps = -1;
   tmctl_if_t intf;
   tmctl_shaper_t shaper;
   tmctl_devType_e devType;

   cmsLog_debug("Entered: l2ifname=%s status=%s portbps=%d portMbs=%d",
                l2ifname, status, portbps, portMbs);

   if (cmsUtl_strlen(l2ifname) == 0)
   {
      cmsLog_error("NULL or empty l2ifname");
      return CMSRET_INTERNAL_ERROR;
   }

   if (!status || cmsUtl_strcmp(status, MDMVS_UP) != 0)
   {
      cmsLog_debug("%s link is not up. No action is taken.", l2ifname);
      return CMSRET_SUCCESS;
   }

   if ((ret = rutQos_tmGetDevInfoByIfName(l2ifname, &devType, &intf)) != CMSRET_SUCCESS)
   {
      cmsLog_notice("interface %s is unsupported device type, ret=%d", l2ifname, ret);
      return ret;
   }

   if(portbps != -1)
   {
      portKbps = portbps / 1000;
   }
   
   shaper.shapingRate      = (portKbps > 0)? portKbps : 0;
   shaper.shapingBurstSize = portMbs;
   shaper.minRate          = 0;  /* not used for port shaping */
   
   if (rutcwmp_tmctl_setPortShaper(devType, &intf, &shaper) == TMCTL_ERROR)
   {
      cmsLog_error("tmctl_setPortShaper ERROR!");
      ret = CMSRET_INTERNAL_ERROR;
   }
   
   return ret;
   
}  /* End of rutQos_tmPortShaperCfg() */

/*******************************************************************************
 *
 * Function: rutQos_tmGetQueueStats
 *
 * Get queue statistics by queueId of specified port.
 *
 *******************************************************************************/
CmsRet rutQos_tmGetQueueStats(const char* l2ifname, SINT32 queueId,
                              tmctl_queueStats_t *queueStats)
{
   CmsRet ret = CMSRET_SUCCESS;
   tmctl_devType_e devType = TMCTL_DEV_ETH;
   tmctl_if_t intf;
   
   cmsLog_debug("Entered: l2ifname=%s queueId=%d", l2ifname, queueId);

   if ((ret = rutQos_tmGetDevInfoByIfName(l2ifname, &devType, &intf)) != CMSRET_SUCCESS)
   {
      cmsLog_notice("interface %s is unsupported device type, ret=%d", l2ifname, ret);
      return ret;
   }

   if (rutcwmp_tmctl_getQueueStats(devType, &intf, queueId, queueStats) == TMCTL_ERROR)
      ret = CMSRET_INTERNAL_ERROR;
   
   return ret;
}

#else /* SUPPORT_TMCTL is not defined */

CmsRet rutQos_tmPortInit(const char *l2ifname __attribute__((unused)), UBOOL8 isWan __attribute__((unused)))
{
   cmsLog_debug("Port TM will not be initialized if TMCTL is not supported.");
   return CMSRET_SUCCESS;
}  /* End of rutQos_tmPortInit() */

CmsRet rutQos_tmPortUninit(const char *l2ifname __attribute__((unused)), UBOOL8 isWan __attribute__((unused)))
{
   cmsLog_debug("Port TM will not be un-initialized if TMCTL is not supported.");
   return CMSRET_SUCCESS;
}  /* End of rutQos_tmPortUninit() */

CmsRet rutQos_tmQueueConfig(QosCommandType cmdType __attribute__((unused)),
                            CmsQosQueueInfo *qInfo __attribute__((unused)))
{
   cmsLog_debug("Queue will not be configured if TMCTL is not supported.");
   return CMSRET_SUCCESS;
}  /* End of rutQos_tmQueueConfig() */

CmsRet rutQos_tmPortShaperCfg(const char* l2ifname __attribute__((unused)), SINT32 portbps __attribute__((unused)), SINT32 portMbs __attribute__((unused)),
                              char* status __attribute__((unused)), UBOOL8 isWan __attribute__((unused)))
{
   cmsLog_debug("Port Shaper will not be configured if TMCTL is not supported.");
   return CMSRET_SUCCESS;
}  /* End of rutQos_tmPortShaperCfg() */

CmsRet rutQos_tmGetQueueStats(const char* l2ifname __attribute__((unused)), SINT32 queueId __attribute__((unused)),
                              tmctl_queueStats_t *queueStats __attribute__((unused)))
{
   cmsLog_debug("Can not get queue statistics if TMCTL is not supported.");
   return CMSRET_SUCCESS;
} /* End of rutQos_tmGetQueueStats() */

#endif   /* defined(SUPPORT_TMCTL) */


#endif  /* SUPPORT_QOS */

