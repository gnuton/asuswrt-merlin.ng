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
/* this only applies to device 2, and also us some function in rut_atm.c */

#ifdef DMP_DEVICE2_PTMLINK_1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "bcmnet.h"
#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "cms_boardcmds.h"
#include "cms_qos.h"
#include "rut_diag.h"
#include "rut_system.h"
#include "devctl_xtm.h"

CmsRet rutptm_fillL2IfName_dev2(const Layer2IfNameType ifNameType, char **ifName)
{
   CmsRet ret;
   SINT32 intfArray[IFC_WAN_MAX];
   SINT32 index = 0;
   char *prefix;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char L2IfName[CMS_IFNAME_LENGTH];
   Dev2PtmLinkObject *ptmLinkObj;
   Dev2PtmObject *ptmObj;
   
   memset((UINT8 *) &intfArray, 0, sizeof(intfArray));
   
   if ((ret = cmsObj_get(MDMOID_DEV2_PTM, &iidStack, 0, (void *) &ptmObj)) == CMSRET_SUCCESS)
   {
      if (ptmObj->linkNumberOfEntries >= IFC_WAN_MAX)
      {
         cmsLog_error("Only %d interface allowed", IFC_WAN_MAX);
         cmsObj_free((void **) &ptmObj);
         return CMSRET_INTERNAL_ERROR;      
      }
   }
   else
   {
      cmsLog_debug("This should never happen, Device.PTM object is not found");
      return CMSRET_INTERNAL_ERROR;      
   }
   // Done with this object.  Free it now.
   cmsObj_free((void **) &ptmObj);

   switch (ifNameType)
   {
   case PTM_EOA:
      while ((ret = cmsObj_getNext(MDMOID_DEV2_PTM_LINK, &iidStack, (void **) &ptmLinkObj)) == CMSRET_SUCCESS)
      {
         if (ptmLinkObj->name == NULL)
         {
            /* this is one we just created and is NULL, so break */
            cmsObj_free((void **) &ptmLinkObj);
            break;
         }
         else
         {
            index = atoi(&(ptmLinkObj)->name[strlen(PTM_IFC_STR)]);
            if (index <= IFC_WAN_MAX) 
            {
               cmsLog_debug("ptmLinkObj->name=%s, index=%d", ptmLinkObj->name, index);
               intfArray[index] = 1;            /* mark the interface used */
            }
         }
         cmsObj_free((void **) &ptmLinkObj);
      }
      prefix = PTM_IFC_STR;
      break;

   default:
         cmsLog_error("Wrong type=%d", ifNameType);
         return CMSRET_INTERNAL_ERROR;
   }

   for (index = 0; index < IFC_WAN_MAX; index++)
   {
      cmsLog_debug("intfArray[%d]=%d", index, intfArray[index]);
      if (intfArray[index] == 0)
      {
         sprintf(L2IfName, "%s%d", prefix, index);
         ret = CMSRET_SUCCESS;
         break;
      }
   }

   CMSMEM_REPLACE_STRING_FLAGS(*ifName, L2IfName, mdmLibCtx.allocFlags);
   cmsLog_debug("Get Layer2 ifName=%s", *ifName);

   return ret;
}

CmsRet rutptm_fillLowerLayer(char **lowerLayer)
{
   Dev2DslChannelObject *dslChannelObj;
   Dev2DslBondingGroupObject *dslBondingGroupObj;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc;
   UBOOL8 found = FALSE;
   char *channelFullPathString=NULL;
   char *fullPathString=NULL;
   UBOOL8 operational = FALSE;
   char allLowerLayersStringBuf[MDM_MULTI_FULLPATH_BUFLEN]={0};
#ifdef DMP_DEVICE2_FAST_1
   Dev2FastLineObject *fastLineObj;
   char *fastLineFullPathString=NULL;
#endif

   /* in our system, the second dsl line used for bonding is not configurable,
    * so, when a ptm link is created, it is always on the top of PTM channel that is stacked on the
    * primary line.   
    */
   /* in a bonding board & image, then the ATM link is stacked on the 
    * top of the ATM bonding group.
    */
#ifdef DMP_DEVICE2_BONDEDDSL_1
   qdmDsl_isDslBondingGroupStatusOperational_dev2(MDMVS_ETHERNET,&operational);
   if (operational == TRUE)
   {
      while (!found && cmsObj_getNext(MDMOID_DEV2_DSL_BONDING_GROUP, &iidStack, (void **) &dslBondingGroupObj) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(dslBondingGroupObj->bondScheme, MDMVS_ETHERNET))
         {
            found = TRUE;
            INIT_PATH_DESCRIPTOR(&pathDesc);
            pathDesc.oid = MDMOID_DEV2_DSL_BONDING_GROUP;
            pathDesc.iidStack = iidStack;
            cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathString);
            sprintf(allLowerLayersStringBuf,"%s",fullPathString);
            CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
         }
         cmsObj_free((void **) &dslBondingGroupObj);      
      }
   }
#endif   
   while (!found &&
          cmsObj_getNext(MDMOID_DEV2_DSL_CHANNEL, &iidStack, (void **) &dslChannelObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(dslChannelObj->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM))
      {
         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_DEV2_DSL_CHANNEL;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &channelFullPathString);
         sprintf(allLowerLayersStringBuf,"%s",channelFullPathString);
         CMSMEM_FREE_BUF_AND_NULL_PTR(channelFullPathString);
         found = TRUE;

#ifdef DMP_DEVICE2_FAST_1
         /* if fast is in, the lower layer of PTM also includes FAST.Line */
         INIT_INSTANCE_ID_STACK(&iidStack);
         /* only interested in the first line.  If the 2nd line exists, it will be the bonding group */
         if (cmsObj_getNext(MDMOID_DEV2_FAST_LINE, &iidStack, (void **) &fastLineObj) == CMSRET_SUCCESS)
         {
            INIT_PATH_DESCRIPTOR(&pathDesc);
            pathDesc.oid = MDMOID_DEV2_FAST_LINE;
            pathDesc.iidStack = iidStack;
            cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fastLineFullPathString);
            cmsUtl_addFullPathToCSL(fastLineFullPathString,allLowerLayersStringBuf,sizeof(allLowerLayersStringBuf));
            CMSMEM_FREE_BUF_AND_NULL_PTR(fastLineFullPathString);
            cmsObj_free((void **) &fastLineObj);
         }
#endif /* DMP_DEVICE2_FAST_1 */

      }
      cmsObj_free((void **) &dslChannelObj);
   }

   if (found)
   {
      CMSMEM_REPLACE_STRING_FLAGS(*lowerLayer, allLowerLayersStringBuf, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }
   return CMSRET_INTERNAL_ERROR;
}

CmsRet rutptm_setConnCfg_dev2(const _Dev2PtmLinkObject *newObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   XTM_ADDR Addr;
   XTM_CONN_CFG ConnCfg;
   PXTM_TRANSMIT_QUEUE_PARMS pTxQ;
   
   cmsLog_debug("Enter");
   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
   memset((UINT8 *) &ConnCfg, 0x00, sizeof(ConnCfg));

   Addr.ulTrafficType     = TRAFFIC_TYPE_PTM;
   Addr.u.Flow.ulPortMask = PORTID_TO_PORTMASK(newObj->X_BROADCOM_COM_PTMPortId);
   
   if (newObj->X_BROADCOM_COM_PTMPriorityHigh == TRUE &&
       newObj->X_BROADCOM_COM_PTMPriorityLow  == TRUE)
   {
      Addr.u.Flow.ulPtmPriority = PTM_PRI_HIGH | PTM_PRI_LOW;
   }
   else if (newObj->X_BROADCOM_COM_PTMPriorityHigh == TRUE)
   {
      Addr.u.Flow.ulPtmPriority = PTM_PRI_HIGH;
   }
   else if (newObj->X_BROADCOM_COM_PTMPriorityLow == TRUE)
   {
      Addr.u.Flow.ulPtmPriority = PTM_PRI_LOW;
   }
   else
   {
      cmsLog_error("PTM priority is neither LOW nor HIGH.");
      return CMSRET_INVALID_ARGUMENTS;
   }

   ConnCfg.ulHeaderType         = HT_LLC_SNAP_ETHERNET;
   ConnCfg.ulAdminStatus        = ADMSTS_UP;
   ConnCfg.ulTransmitQParmsSize = 1;
   if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_GrpScheduler, MDMVS_WRR))
   {
      ConnCfg.ConnArbs[0][0].ulWeightAlg = WA_CWRR;
   }
   else
   {
      ConnCfg.ConnArbs[0][0].ulWeightAlg = WA_DISABLED;
   }
   ConnCfg.ConnArbs[0][0].ulWeightValue = newObj->X_BROADCOM_COM_GrpWeight;
   ConnCfg.ConnArbs[0][0].ulSubPriority = XTM_QOS_LEVELS - newObj->X_BROADCOM_COM_GrpPrecedence;

   cmsLog_debug("ConnCfg.ulTransmitQParmsSize=%d, Addr.u.Flow.ulPtmPriority=%d", ConnCfg.ulTransmitQParmsSize, Addr.u.Flow.ulPtmPriority);
   
   /* Configure the default queue associated with this connection */
   pTxQ = &ConnCfg.TransmitQParms[0];
   pTxQ->usSize = HOST_XTM_NR_TXBDS;

   if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_SchedulerAlgorithm, MDMVS_WFQ))
   {
      pTxQ->ucWeightAlg = WA_WFQ;
   }
   else  /* WRR or SP */
   {
      pTxQ->ucWeightAlg = WA_CWRR;
   }
   pTxQ->ulWeightValue = newObj->X_BROADCOM_COM_QueueWeight;   //ConnCfg.ConnArbs[0][0].ulWeightValue;
   pTxQ->ucSubPriority = XTM_QOS_LEVELS - newObj->X_BROADCOM_COM_QueuePrecedence;
   pTxQ->ucQosQId      = 0;   /* qid of the default queue is 0 */

   if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_DropAlgorithm, MDMVS_RED))
   {
      pTxQ->ucDropAlg = WA_RED;
   }
   else if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_DropAlgorithm, MDMVS_WRED))
   {
      pTxQ->ucDropAlg = WA_WRED;
   }
   else
   {
      pTxQ->ucDropAlg = WA_DT;
   }
   pTxQ->ucLoMinThresh = newObj->X_BROADCOM_COM_LowClassMinThreshold;
   pTxQ->ucLoMaxThresh = newObj->X_BROADCOM_COM_LowClassMaxThreshold;
   pTxQ->ucHiMinThresh = newObj->X_BROADCOM_COM_HighClassMinThreshold;
   pTxQ->ucHiMaxThresh = newObj->X_BROADCOM_COM_HighClassMaxThreshold;

   if (newObj->X_BROADCOM_COM_QueueMinimumRate > QOS_QUEUE_NO_SHAPING)
   {
      pTxQ->ulMinBitRate = newObj->X_BROADCOM_COM_QueueMinimumRate;
   }
   else
   {
      pTxQ->ulMinBitRate = 0;  /* no shaping */
   }
   if (newObj->X_BROADCOM_COM_QueueShapingRate > QOS_QUEUE_NO_SHAPING)
   {
      pTxQ->ulShapingRate = newObj->X_BROADCOM_COM_QueueShapingRate;
   }
   else
   {
      pTxQ->ulShapingRate = 0;  /* no shaping */
   }
   pTxQ->usShapingBurstSize = newObj->X_BROADCOM_COM_QueueShapingBurstSize;

   if (Addr.u.Flow.ulPortMask == (PORT_PHY0_PATH0 | PORT_PHY0_PATH1))
   {
      pTxQ->ulPortId = PORT_PHY0_PATH0;
   }
   else
   {
      pTxQ->ulPortId = Addr.u.Flow.ulPortMask;
   }

   /* If the flow-PtmPriority is either HIGH-only or Low-only,
    *    set the queue-PtmPriority to the flow-PtmPriority value.
    * If the flow-PtmPriority is HIGH-LOW, this must be the first queue.
    *    set the queue-PtmPriority to PTM_PRI_LOW.
    */ 
   if( Addr.u.Flow.ulPtmPriority == PTM_PRI_HIGH )
   {
      /* The flow-PtmPriority is HIGH-only. */
      pTxQ->ulPtmPriority = PTM_PRI_HIGH; /* set ptm priority of the queue */
   }
   else
   {
      /* The flow-PtmPriority is either HIGH-LOW or LOW-only. */
      pTxQ->ulPtmPriority = PTM_PRI_LOW;  /* set ptm priority of the queue */
   }

   if ((ret = devCtl_xtmSetConnCfg( &Addr, &ConnCfg )) != CMSRET_SUCCESS)
   {
      cmsLog_error("devCtl_xtmSetConnCfg returns error. ret=%d", ret);
   }

   return ret;
}


CmsRet rutptm_createInterface_dev2(const _Dev2PtmLinkObject *newObj)
{
   CmsRet ret;
   XTM_ADDR Addr;
   UINT32 priorityLow = newObj->X_BROADCOM_COM_PTMPriorityLow;
   UINT32 priorityHigh = 0;
   
   /* form the ptm interface name */
   cmsLog_debug("Create PTM interface %s", newObj->name);
   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));

   Addr.ulTrafficType = TRAFFIC_TYPE_PTM;
   Addr.u.Flow.ulPortMask = PORTID_TO_PORTMASK(newObj->X_BROADCOM_COM_PTMPortId);
   /* 
    * ulPtmPriority is a mask for both priorityLow and priorityHigh, so just OR them
    * and X_BROADCOM_COM_PTMPriorityLow is either 0 or 1 here so no need to adjust but 
    * X_BROADCOM_COM_PTMPriorityHigh need to be adjusted.
    */
   if (newObj->X_BROADCOM_COM_PTMPriorityHigh == TRUE)
   {
      /* since PTM_PRI_HIGH  is defined as  0x02 */
      priorityHigh = PTM_PRI_HIGH; 
   }
   Addr.u.Flow.ulPtmPriority = priorityHigh | priorityLow;

   if ((ret = devCtl_xtmCreateNetworkDevice(&Addr, newObj->name)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to start ptm interface %s. error=%d", newObj->name, ret);
      return ret;
   }

   cmsLog_debug("devCtl_xtmCreateNetworkDevice ret=%d", ret);
   
   return ret;
}


CmsRet rutptm_deleteInterface_dev2(const _Dev2PtmLinkObject *currObj)
{
   CmsRet ret;
   XTM_ADDR Addr;
   UINT32 priorityLow = currObj->X_BROADCOM_COM_PTMPriorityLow;
   UINT32 priorityHigh = 0;

   cmsLog_debug("Delete PTM interface %s", currObj->name);

   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
   Addr.ulTrafficType = TRAFFIC_TYPE_PTM;
   Addr.u.Flow.ulPortMask = PORTID_TO_PORTMASK(currObj->X_BROADCOM_COM_PTMPortId);
   /* 
    * ulPtmPriority is a mask for both priorityLow and priorityHigh, so just OR them
    * and X_BROADCOM_COM_PTMPriorityLow is either 0 or 1 here so no need to adjust but 
    * X_BROADCOM_COM_PTMPriorityHigh need to be adjusted.
    */
   if (currObj->X_BROADCOM_COM_PTMPriorityHigh == TRUE)
   {
      /* since PTM_PRI_HIGH  is defined as  0x02 */
      priorityHigh = PTM_PRI_HIGH; 
   }
   Addr.u.Flow.ulPtmPriority = priorityHigh | priorityLow;

   if ((ret = devCtl_xtmDeleteNetworkDevice(&Addr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to delete ptm interface %s. error=%d", currObj->name, ret);
   }

   cmsLog_debug("devCtl_xtmDeleteNetworkDevice ret=%d", ret);

   return ret;
}

CmsRet rutptm_deleteConnCfg_dev2(const _Dev2PtmLinkObject *newObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   XTM_ADDR Addr;
   XTM_CONN_CFG ConnCfg;
   
   cmsLog_debug("Enter");
   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
   memset((UINT8 *) &ConnCfg, 0x00, sizeof(ConnCfg));

   Addr.ulTrafficType     = TRAFFIC_TYPE_PTM;
   Addr.u.Flow.ulPortMask = PORTID_TO_PORTMASK(newObj->X_BROADCOM_COM_PTMPortId);
   
   if (newObj->X_BROADCOM_COM_PTMPriorityHigh == TRUE &&
       newObj->X_BROADCOM_COM_PTMPriorityLow  == TRUE)
   {
      Addr.u.Flow.ulPtmPriority = PTM_PRI_HIGH | PTM_PRI_LOW;
   }
   else if (newObj->X_BROADCOM_COM_PTMPriorityHigh == TRUE)
   {
      Addr.u.Flow.ulPtmPriority = PTM_PRI_HIGH;
   }
   else if (newObj->X_BROADCOM_COM_PTMPriorityLow == TRUE)
   {
      Addr.u.Flow.ulPtmPriority = PTM_PRI_LOW;
   }
   else
   {
      cmsLog_error("PTM priority is neither LOW nor HIGH.");
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((ret = devCtl_xtmSetConnCfg( &Addr, NULL )) != CMSRET_SUCCESS)
   {
      cmsLog_error("devCtl_xtmSetConnCfg returns error. ret=%d", ret);
   }

   return ret;
}

void rutptm_getLinkStats_dev2(Dev2PtmLinkStatsObject *stats, char *linkName, int port, UBOOL8 reset)
{
   UINT64 dontCare;
   UINT32 ulPortId = PORTID_TO_PORTMASK(port);
   XTM_INTERFACE_STATS IntfStats;
   XTM_INTERFACE_CFG Cfg;
   int error = 0;
   CmsRet ret;
   UINT64 errorsRx, errorsTx;
   UINT64 discardPacketsRx, discardPacketsTx;

   if (reset)
   {
      devCtl_xtmGetInterfaceStatistics(ulPortId, &IntfStats, reset);
   }
   else
   {
      /* get the network interface stats first */
      rut_getIntfStats_uint64(linkName,
                              &dontCare,&dontCare,
                              &dontCare/*byteMultiRx*/,&stats->multicastPacketsReceived,
                              &stats->unicastPacketsReceived,&stats->broadcastPacketsReceived,
                              &errorsRx,&discardPacketsRx,
                              &dontCare,&dontCare,
                              &dontCare/*byteMultiTx*/,&stats->multicastPacketsSent,
                              &stats->unicastPacketsSent,&stats->broadcastPacketsSent,
                              &errorsTx,&discardPacketsTx);
      stats->errorsReceived = (UINT32)errorsRx;
      stats->errorsSent = (UINT32)errorsTx;
      stats->discardPacketsReceived = (UINT32)discardPacketsRx;
      stats->discardPacketsSent = (UINT32)discardPacketsTx;
      stats->unknownProtoPacketsReceived = stats->discardPacketsReceived;

      /* then some SAR specific statistics */
      ret = devCtl_xtmGetInterfaceCfg(ulPortId,&Cfg);
      if ( (ret != CMSRET_SUCCESS) ||
           ((ret == CMSRET_SUCCESS) && (Cfg.ulIfOperStatus  == OPRSTS_DOWN)) )
      {
         error = 1;
      }
      else
      {
         memset((UINT8 *) &IntfStats, 0x00, sizeof(IntfStats));
         ret = devCtl_xtmGetInterfaceStatistics(ulPortId, &IntfStats, reset);
         if (ret != CMSRET_SUCCESS)
         {
            error = 1;
         }
         else
         {
            stats->bytesReceived = (UINT64) IntfStats.ulIfInOctets;
            stats->bytesSent =  (UINT64)IntfStats.ulIfOutOctets;
            stats->packetsReceived = (UINT64)IntfStats.ulIfInPackets;
            stats->packetsSent = (UINT64)IntfStats.ulIfOutPackets;
            stats->discardPacketsReceived = IntfStats.ulIfInPacketErrors;
            stats->X_BROADCOM_COM_InOAMCells = IntfStats.ulIfInOamRmCells;
            stats->X_BROADCOM_COM_OutOAMCells = IntfStats.ulIfOutOamRmCells;
            stats->X_BROADCOM_COM_InASMCells = IntfStats.ulIfInAsmCells;
            stats->X_BROADCOM_COM_OutASMCells = IntfStats.ulIfOutAsmCells;
            stats->X_BROADCOM_COM_InCellErrors = IntfStats.ulIfInCellErrors;
         }
      } /* no error from xtm PORT */
      if (error)
      {
         stats->bytesReceived = 0;
         stats->bytesSent =  0;
         stats->packetsReceived = 0;
         stats->packetsSent = 0;
         stats->X_BROADCOM_COM_InOAMCells = 0;
         stats->X_BROADCOM_COM_OutOAMCells = 0;
         stats->X_BROADCOM_COM_InASMCells = 0;
         stats->X_BROADCOM_COM_OutASMCells = 0;
         stats->X_BROADCOM_COM_InCellErrors = 0;
      }
   } /* !reset */
}

#endif   /* DMP_DEVICE2_PTMLINK_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */
