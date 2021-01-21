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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "bcmnet.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rut_diag.h"
#include "rcl.h"
#include "rut_atm.h"
#include "rut_util.h"
#include "adslctlapi.h"
#include "devctl_xtm.h"
#include "cms_boardcmds.h"
#include "cms_qos.h"

atmQosDefs atmQosMap[] = 
{
   /* cmpStr(data Model), atmctl form */
   {MDMVS_UBRWPCR, "ubr_pcr"},
   {MDMVS_CBR,     "cbr"},
   {MDMVS_VBR_RT,  "rtvbr"},
   {MDMVS_VBR_NRT, "nrtvbr"},
   {MDMVS_UBR,     "ubr"},
   {MDMVS_UBR_PLUS,""},
   {MDMVS_ABR,     ""},          /* not support now */
   {NULL, NULL}
};

/* convert data module form to atmctl form */
CmsRet rutAtm_categoryConvertion(const char *inCmpStr, char *outResultStr, int outStrLen) 
{
   CmsRet ret = CMSRET_INVALID_ARGUMENTS;
   atmQosDefs *atmQosMapPtr = atmQosMap;

   if (inCmpStr != NULL)
   {
      while (atmQosMapPtr->cmpStr != NULL)
      {
         if (strncmp(inCmpStr, atmQosMapPtr->cmpStr, strlen(atmQosMapPtr->cmpStr)) == 0)
         {
          
            strncpy(outResultStr, atmQosMapPtr->retStr, outStrLen);
            return CMSRET_SUCCESS;
         }
         atmQosMapPtr++;
      }
   }
   
   outResultStr[0] = '\0';
   cmsLog_error("Invalid atmQos parameter");
   return ret;
   
}


#ifdef DMP_ADSLWAN_1
/***************************************************************************
 * find the traffic descriptor index that matches the given
 * return traffic descriptor index or 0 if none found.
****************************************************************************/
static UINT32 rutAtm_getTrffDscrIndex(const WanDslLinkCfgObject *dslLinkCfg __attribute__((unused)))
{
   UINT32 index = 0;
   
#ifndef DESKTOP_LINUX

   char line[BUFLEN_264];
   char col[6][BUFLEN_16];
   char category[BUFLEN_32];
   UINT32 parm1 = 0, parm2 = 0, parm3 = 0;
   SINT32 parm4 = -1;
   FILE* fs = NULL;

   if (rutAtm_categoryConvertion(dslLinkCfg->ATMQoS, category, sizeof(category)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Fail to get index");
      return index;
   }
   rut_doSystemAction("rut_atm", atmTdteString "show > /var/tdteshow");

   if ((fs = fopen("/var/tdteshow", "r")) == NULL)
   {
      cmsLog_error("Failed to get /var/tdteshow");
      return index;
   }
   
   /* title = (index type pcd scr mbs mcr) */
   while ((fgets(line, BUFLEN_264, fs) != NULL ) && index == 0) 
   {
      if (strstr(line, category) != NULL) 
      {
         sscanf(line, "%s %s %s %s %s %s", col[0], col[1], col[2], col[3], col[4], col[5]);
         parm1 = strtoul(col[2], (char **)NULL, 10);
         parm2 = strtoul(col[3], (char **)NULL, 10);
         parm3 = strtoul(col[4], (char **)NULL, 10);
         parm4 = strtol(col[5], (char **)NULL, 10);
         if (parm4 == 0)
            parm4 = -1;
            
         if (strcmp(category, col[1]) == 0 &&
              dslLinkCfg->ATMPeakCellRate == parm1 &&
              dslLinkCfg->ATMSustainableCellRate == parm2 &&
              dslLinkCfg->ATMMaximumBurstSize == parm3 &&
              dslLinkCfg->X_BROADCOM_COM_ATMMinimumCellRate == parm4)
            index = strtoul(col[0], (char **)NULL, 10);
      }
   }

   fclose(fs);
   rut_doSystemAction("rut_atm", "rm /var/tdteshow");
   
#endif

   return index;
}


/*
 * Compose atm traffic descriptor info.
 * Description  : add/delete vcc to BcmAtm using atmctl.
 * For correct ATM shaping, the transmit queue size needs to be
 * number_of_lan_receive_buffers / number_of_transmit_queues
 * (80 Ethernet buffers / 8 transmit queues = 10 buffer queue size)
 */
static void rutAtm_ctlTrffDscrConfig(const WanDslLinkCfgObject *dslLinkCfg)
{

   if( rutAtm_getTrffDscrIndex(dslLinkCfg) == 0 )
   {
      char cmdStr[BUFLEN_128];
      char atmCtlStr[BUFLEN_16];

      cmdStr[0] = '\0';

      rutAtm_categoryConvertion(dslLinkCfg->ATMQoS, atmCtlStr, sizeof(atmCtlStr));
      snprintf(cmdStr, sizeof(cmdStr), "%sadd %s ", atmTdteString, atmCtlStr);

      if (strncmp(dslLinkCfg->ATMQoS, MDMVS_CBR, sizeof(MDMVS_CBR)) == 0 ||
          strncmp(dslLinkCfg->ATMQoS, MDMVS_UBRWPCR, sizeof(MDMVS_UBRWPCR)) == 0)
      {
          /* atmctl operate tdte --add cbr/ubr-w-pcr peakCellRate */
          snprintf(cmdStr + strlen(cmdStr), sizeof(cmdStr) - strlen(cmdStr), " %d", dslLinkCfg->ATMPeakCellRate);
      }
      else if (strncmp(dslLinkCfg->ATMQoS, MDMVS_VBR_RT, sizeof(MDMVS_VBR_RT)) == 0 ||
               strncmp(dslLinkCfg->ATMQoS, MDMVS_VBR_NRT, sizeof(MDMVS_VBR_NRT)) == 0) 
      {
          /* atmctl operate tdte --add rtvbr/nrtvbr peakCellRate SustainCellRate MaxBurstCellRate */
          snprintf(cmdStr + strlen(cmdStr), sizeof(cmdStr) - strlen(cmdStr), " %d %d %d", dslLinkCfg->ATMPeakCellRate, 
                   dslLinkCfg->ATMSustainableCellRate, dslLinkCfg->ATMMaximumBurstSize);
      }

      if ((dslLinkCfg->X_BROADCOM_COM_ATMMinimumCellRate > 0) &&
          (strncmp(dslLinkCfg->ATMQoS, MDMVS_UBR, sizeof(MDMVS_UBR)) == 0 ||
           strncmp(dslLinkCfg->ATMQoS, MDMVS_UBRWPCR, sizeof(MDMVS_UBRWPCR)) == 0))
      {
          snprintf(cmdStr + strlen(cmdStr), sizeof(cmdStr) - strlen(cmdStr), " %d",
                   dslLinkCfg->X_BROADCOM_COM_ATMMinimumCellRate);
      }

      if (cmdStr[0] != '\0')
         rut_doSystemAction("rut_atm", cmdStr);
   }
}


CmsRet rutAtm_ctlVccAdd(const WanDslLinkCfgObject *dslLinkCfg)
{
   CmsRet ret = CMSRET_SUCCESS;
   XTM_ADDR Addr;
   XTM_CONN_CFG ConnCfg;
   PXTM_TRANSMIT_QUEUE_PARMS pTxQ;
   SINT32 nVpi = 0;
   SINT32 nVci = 0;

   if ((ret = cmsUtl_atmVpiVciStrToNum(dslLinkCfg->destinationAddress, &nVpi, &nVci)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   rutAtm_ctlTrffDscrConfig(dslLinkCfg);
   
   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
   memset((UINT8 *) &ConnCfg, 0x00, sizeof(ConnCfg));

   Addr.ulTrafficType    = TRAFFIC_TYPE_ATM;
   Addr.u.Vcc.ulPortMask = PORTID_TO_PORTMASK(dslLinkCfg->X_BROADCOM_COM_ATMInterfaceId);
   Addr.u.Vcc.usVpi      = (UINT16) nVpi;
   Addr.u.Vcc.usVci      = (UINT16) nVci;

   ConnCfg.ulAtmAalType                = AAL_5;
   ConnCfg.ulAdminStatus               = ADMSTS_UP;
   ConnCfg.ulTransmitTrafficDescrIndex = rutAtm_getTrffDscrIndex(dslLinkCfg);
   ConnCfg.ulTransmitQParmsSize        = 1;
   if (!cmsUtl_strcmp(dslLinkCfg->X_BROADCOM_COM_GrpScheduler, MDMVS_WRR))
   {
      ConnCfg.ConnArbs[0][0].ulWeightAlg = WA_CWRR;
   }
   else
   {
      ConnCfg.ConnArbs[0][0].ulWeightAlg = WA_DISABLED;
   }
   ConnCfg.ConnArbs[0][0].ulWeightValue = dslLinkCfg->X_BROADCOM_COM_GrpWeight;
   ConnCfg.ConnArbs[0][0].ulSubPriority = XTM_QOS_LEVELS - dslLinkCfg->X_BROADCOM_COM_GrpPrecedence;

   if (!cmsUtl_strcmp(dslLinkCfg->linkType, MDMVS_EOA))
   {
      if (!cmsUtl_strcmp(dslLinkCfg->ATMEncapsulation, MDMVS_LLC))
      {
         ConnCfg.ulHeaderType = HT_LLC_SNAP_ETHERNET;
      }
      else
      {
         ConnCfg.ulHeaderType = HT_VC_MUX_ETHERNET;
      }
   }
   else
   {
      if (!cmsUtl_strcmp(dslLinkCfg->linkType, MDMVS_PPPOA))
      {
         if (!cmsUtl_strcmp(dslLinkCfg->ATMEncapsulation, MDMVS_LLC))
         {
            ConnCfg.ulHeaderType = HT_LLC_ENCAPS_PPP;
         }
         else
         {
            ConnCfg.ulHeaderType = HT_VC_MUX_PPPOA;
         }
      }
      else
      {
         if (!cmsUtl_strcmp(dslLinkCfg->linkType, MDMVS_IPOA))
         {
            if (!cmsUtl_strcmp(dslLinkCfg->ATMEncapsulation, MDMVS_LLC))
            {
               ConnCfg.ulHeaderType = HT_LLC_SNAP_ROUTE_IP;
            }
            else
            {
               ConnCfg.ulHeaderType = HT_VC_MUX_IPOA;
            }
         }
         else
         {
            ConnCfg.ulHeaderType = HT_LLC_SNAP_ETHERNET;
         }
      }
   }

   /* Configure the default queue associated with this connection */
   pTxQ = &ConnCfg.TransmitQParms[0];
   pTxQ->usSize = HOST_XTM_NR_TXBDS;

   if (!cmsUtl_strcmp(dslLinkCfg->X_BROADCOM_COM_SchedulerAlgorithm, MDMVS_WFQ))
   {
      pTxQ->ucWeightAlg = WA_WFQ;
   }
   else  /* WRR or SP */
   {
      pTxQ->ucWeightAlg = WA_CWRR;
   }
   pTxQ->ulWeightValue = dslLinkCfg->X_BROADCOM_COM_QueueWeight;   //ConnCfg.ConnArbs[0][0].ulWeightValue;
   pTxQ->ucSubPriority = XTM_QOS_LEVELS - dslLinkCfg->X_BROADCOM_COM_QueuePrecedence;
   pTxQ->ucQosQId      = 0;   /* qid of the default queue is 0 */

   if (!cmsUtl_strcmp(dslLinkCfg->X_BROADCOM_COM_DropAlgorithm, MDMVS_RED))
   {
      pTxQ->ucDropAlg = WA_RED;
   }
   else if (!cmsUtl_strcmp(dslLinkCfg->X_BROADCOM_COM_DropAlgorithm, MDMVS_WRED))
   {
      pTxQ->ucDropAlg = WA_WRED;
   }
   else
   {
      pTxQ->ucDropAlg = WA_DT;
   }
   pTxQ->ucLoMinThresh = dslLinkCfg->X_BROADCOM_COM_LowClassMinThreshold;
   pTxQ->ucLoMaxThresh = dslLinkCfg->X_BROADCOM_COM_LowClassMaxThreshold;
   pTxQ->ucHiMinThresh = dslLinkCfg->X_BROADCOM_COM_HighClassMinThreshold;
   pTxQ->ucHiMaxThresh = dslLinkCfg->X_BROADCOM_COM_HighClassMaxThreshold;

   if (Addr.u.Vcc.ulPortMask == (PORT_PHY0_PATH0 | PORT_PHY0_PATH1))
   {
      pTxQ->ulPortId = PORT_PHY0_PATH0;
   }
   else
   {
      pTxQ->ulPortId = Addr.u.Vcc.ulPortMask;
   }

   if ((ret = devCtl_xtmSetConnCfg( &Addr, &ConnCfg )) != CMSRET_SUCCESS)
   {
      cmsLog_error("devCtl_xtmSetConnCfg returns error. ret=%d", ret);
   }

   return ret;

}  /* End of rut_atmCtlVccAdd() */


CmsRet rutAtm_createInterface(const _WanDslLinkCfgObject *newObj)
{
   CmsRet ret;
   XTM_ADDR Addr;
   SINT32 vpi;
   SINT32 vci;
   char devName[BUFLEN_32];
   
   /* form the ptm interface name */
   cmsLog_debug("Create ATM interface %s", newObj->X_BROADCOM_COM_IfName);
   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));

   if ((ret = cmsUtl_atmVpiVciStrToNum(newObj->destinationAddress, &vpi, &vci)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert destinationAddress %s", newObj->destinationAddress);
      return ret;
   }
   
   Addr.ulTrafficType = TRAFFIC_TYPE_ATM;
   Addr.u.Vcc.ulPortMask = PORTID_TO_PORTMASK(newObj->X_BROADCOM_COM_ATMInterfaceId);
   Addr.u.Vcc.usVpi = (UINT16) vpi;
   Addr.u.Vcc.usVci = (UINT16) vci;

   strcpy(devName, newObj->X_BROADCOM_COM_IfName);

   if ((ret = devCtl_xtmCreateNetworkDevice(&Addr, devName)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to start atm interface %s. error=%d", devName, ret);
      return ret;
   }

   cmsLog_debug("devCtl_xtmCreateNetworkDevice ret=%d", ret);
   
   return ret;
}


CmsRet rutAtm_deleteInterface(const _WanDslLinkCfgObject *currObj)
{
   CmsRet ret;
   XTM_ADDR Addr;
   SINT32 vpi;
   SINT32 vci;

   cmsLog_debug("Delete ATM interface %s", currObj->X_BROADCOM_COM_IfName);

   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
   
   if ((ret = cmsUtl_atmVpiVciStrToNum(currObj->destinationAddress, &vpi, &vci)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert destinationAddress %s", currObj->destinationAddress);
      return ret;
   }
   
   Addr.ulTrafficType = TRAFFIC_TYPE_ATM;
   Addr.u.Vcc.ulPortMask = PORTID_TO_PORTMASK(currObj->X_BROADCOM_COM_ATMInterfaceId);
   Addr.u.Vcc.usVpi = (UINT16) vpi;
   Addr.u.Vcc.usVci = (UINT16) vci;

   if ((ret = devCtl_xtmDeleteNetworkDevice(&Addr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to delete atm interface %s. error=%d", currObj->X_BROADCOM_COM_IfName, ret);
   }

   cmsLog_debug("devCtl_xtmDeleteNetworkDevice ret=%d", ret);

   return ret;
}



CmsRet rutAtm_ctlVccDelete(const WanDslLinkCfgObject *dslLinkCfg)
{
   CmsRet ret = CMSRET_SUCCESS;
   XTM_ADDR Addr;
   SINT32 nVpi = 0;
   SINT32 nVci = 0;

   if ((ret = cmsUtl_atmVpiVciStrToNum(dslLinkCfg->destinationAddress, &nVpi, &nVci)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
   Addr.ulTrafficType    = TRAFFIC_TYPE_ATM;
   Addr.u.Vcc.ulPortMask = PORTID_TO_PORTMASK(dslLinkCfg->X_BROADCOM_COM_ATMInterfaceId);
   Addr.u.Vcc.usVpi      = (UINT16) nVpi;
   Addr.u.Vcc.usVci      = (UINT16) nVci;
   if ((ret = devCtl_xtmSetConnCfg( &Addr, NULL )) != CMSRET_SUCCESS)
   {
      cmsLog_error("devCtl_xtmSetConnCfg returns error. ret=%d", ret);
   }

   return ret;

}  /* End of rut_atmCtlVccDelete() */
#endif  /* DMP_ADSLWAN_1 */

