/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
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

#ifdef DMP_ADSLWAN_1
#ifdef DMP_PTMWAN_1

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
#include "rcl.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "cms_boardcmds.h"
#include "cms_qos.h"
#include "rut_diag.h"

#include "devctl_xtm.h"


CmsRet rutPtm_setConnCfg(const _WanPtmLinkCfgObject *newObj)
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


CmsRet rutPtm_createInterface(const _WanPtmLinkCfgObject *newObj)
{
   CmsRet ret;
   XTM_ADDR Addr;
   UINT32 priorityLow = newObj->X_BROADCOM_COM_PTMPriorityLow;
   UINT32 priorityHigh = 0;
   
   /* form the ptm interface name */
   cmsLog_debug("Create PTM interface %s", newObj->X_BROADCOM_COM_IfName);
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

   if ((ret = devCtl_xtmCreateNetworkDevice(&Addr, newObj->X_BROADCOM_COM_IfName)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to start ptm interface %s. error=%d", newObj->X_BROADCOM_COM_IfName, ret);
      return ret;
   }

   cmsLog_debug("devCtl_xtmCreateNetworkDevice ret=%d", ret);
   
   return ret;
}


CmsRet rutPtm_deleteInterface(const _WanPtmLinkCfgObject *currObj)
{
   CmsRet ret;
   XTM_ADDR Addr;
   UINT32 priorityLow = currObj->X_BROADCOM_COM_PTMPriorityLow;
   UINT32 priorityHigh = 0;

   cmsLog_debug("Delete PTM interface %s", currObj->X_BROADCOM_COM_IfName);

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
      cmsLog_error("Failed to delete ptm interface %s. error=%d", currObj->X_BROADCOM_COM_IfName, ret);
   }

   cmsLog_debug("devCtl_xtmDeleteNetworkDevice ret=%d", ret);

   return ret;
}

CmsRet rutPtm_deleteConnCfg(const _WanPtmLinkCfgObject *newObj)
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

#endif   /* DMP_PTMWAN_1 */
#endif  /* DMP_ADSLWAN_1 */
