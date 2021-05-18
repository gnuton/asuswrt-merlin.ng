/***********************************************************************
 *
 *  Copyright (c) 2015  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2015:proprietary:standard

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

#ifdef DMP_DEVICE2_FAST_1

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
#include "rcl.h"
#include "rut_util.h"
#include "rut_wan.h"

#include "adslctlapi.h"
#include "AdslMibDef.h"
#include "bcmadsl.h"
#include "bcmxdsl.h"
#include "devctl_xtm.h"
#include "rut_dsl.h"
#include "rut_wanlayer2.h"
#include "devctl_adsl.h"


/* line show time stats */
CmsRet rutfast_getShowTimeStats_dev2(Dev2FastLineStatsShowtimeObject *obj)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (cmsAdsl_getAdslMib(&adslMib) == CMSRET_SUCCESS)
   {
      obj->erroredSecs = adslMib.adslPerfData.perfSinceShowTime.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfSinceShowTime.adslSES;
      obj->LOSS = adslMib.adslPerfData.perfSinceShowTime.adslLoss;
      obj->UAS = adslMib.adslPerfData.perfSinceShowTime.adslUAS;
      obj->LORS = adslMib.adslPerfData.perfSinceShowTime.xdslLORS;
      obj->RTXUC = adslMib.rtxCounterData.cntDS.perfSinceShowTime.rtx_uc;
      obj->RTXTX = adslMib.rtxCounterData.cntUS.perfSinceShowTime.rtx_tx;
      obj->successBSW = adslMib.gfastOlrCounterData.cntDS.perfSinceShowTime.bswCompleted;
      obj->successSRA = adslMib.gfastOlrCounterData.cntDS.perfSinceShowTime.sraCompleted;
      obj->successFRA = adslMib.gfastOlrCounterData.cntDS.perfSinceShowTime.fraCompleted;
      obj->successRPA = adslMib.gfastOlrCounterData.cntDS.perfSinceShowTime.rpaCompleted;
      obj->successTIGA = adslMib.gfastOlrCounterData.cntDS.perfSinceShowTime.tigaCompleted;

      return (CMSRET_SUCCESS);
   }
   else 
   {
      return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
   }
} /* rutfast_getShowTimeStats_dev2 */

CmsRet rutfast_getCurrentDayStats_dev2(Dev2FastLineStatsCurrentDayObject *obj)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (cmsAdsl_getAdslMib(&adslMib) == CMSRET_SUCCESS)
   {  
      obj->erroredSecs = adslMib.adslPerfData.perfCurr1Day.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfCurr1Day.adslSES;
      obj->LOSS = adslMib.adslPerfData.perfCurr1Day.adslLoss;
      obj->UAS = adslMib.adslPerfData.perfCurr1Day.adslUAS;
      obj->LORS = adslMib.adslPerfData.perfCurr1Day.xdslLORS;
      obj->RTXUC = adslMib.rtxCounterData.cntDS.perfCurr1Day.rtx_uc;
      obj->RTXTX = adslMib.rtxCounterData.cntUS.perfCurr1Day.rtx_tx;
      obj->successBSW = adslMib.gfastOlrCounterData.cntDS.perfCurr1Day.bswCompleted;
      obj->successSRA = adslMib.gfastOlrCounterData.cntDS.perfCurr1Day.sraCompleted;
      obj->successFRA = adslMib.gfastOlrCounterData.cntDS.perfCurr1Day.fraCompleted;
      obj->successRPA = adslMib.gfastOlrCounterData.cntDS.perfCurr1Day.rpaCompleted;
      obj->successTIGA = adslMib.gfastOlrCounterData.cntDS.perfCurr1Day.tigaCompleted;

      return (CMSRET_SUCCESS);
   }
   else 
   {
      return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
   }
} /* rutdsl_getAdslCurrentDayStats_dev2 */

CmsRet rutfast_getQuarterHourStats_dev2(Dev2FastLineStatsQuarterHourObject *obj)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (cmsAdsl_getAdslMib(&adslMib) == CMSRET_SUCCESS)
   {  
      obj->erroredSecs = adslMib.adslPerfData.perfCurr15Min.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfCurr15Min.adslSES;
      obj->LOSS = adslMib.adslPerfData.perfCurr15Min.adslLoss;
      obj->UAS = adslMib.adslPerfData.perfCurr15Min.adslUAS;
      obj->LORS = adslMib.adslPerfData.perfCurr15Min.xdslLORS;
      obj->RTXUC = adslMib.rtxCounterData.cntDS.perfCurr15Min.rtx_uc;
      obj->RTXTX = adslMib.rtxCounterData.cntUS.perfCurr15Min.rtx_tx;
      obj->successBSW = adslMib.gfastOlrCounterData.cntDS.perfCurr15Min.bswCompleted;
      obj->successSRA = adslMib.gfastOlrCounterData.cntDS.perfCurr15Min.sraCompleted;
      obj->successFRA = adslMib.gfastOlrCounterData.cntDS.perfCurr15Min.fraCompleted;
      obj->successRPA = adslMib.gfastOlrCounterData.cntDS.perfCurr15Min.rpaCompleted;
      obj->successTIGA = adslMib.gfastOlrCounterData.cntDS.perfCurr15Min.tigaCompleted;

      return (CMSRET_SUCCESS);
   }
   else 
   {
      return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
   }
} /* rutfast_getQuarterHourStats_dev2 */

CmsRet rutfast_getTestParamsInfo_dev2(void *obj)
{
   adslMibInfo adslMib;
   long len;
   char oidStr[] = { kOidAdslPrivate, 0 };
   char   oidStr1[] = { kOidAdslPrivate, kOidAdslPrivExtraInfo, 0 };
   SINT16 subcarrierData[NUM_TONE_GROUP];
   UINT8 gFactor = 1;
   Dev2FastLineTestParamsObject *testParamObj = (Dev2FastLineTestParamsObject*)obj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   xdslFramingInfo *pTxFramingParam;

   if (cmsAdsl_getAdslMib(&adslMib) == (CmsRet) BCMADSL_STATUS_SUCCESS)
   {
#ifdef DMP_DEVICE2_VDSL2_1
      /* get G-factor objects --VDSL and fast? */
      testParamObj->SNRGds = adslMib.gFactors.Gfactor_MEDLEYSETds;
      testParamObj->SNRGus = adslMib.gFactors.Gfactor_MEDLEYSETus;
#endif
      testParamObj->SNRMTds = 128;
      testParamObj->SNRMTus = 128;
      
      /* ACTINP and ACTINPREIN */
      pTxFramingParam = &adslMib.xdslInfo.dirInfo[1].lpInfo[0];
      testParamObj->ACTINP = (pTxFramingParam->INP+1)/2;
      testParamObj->ACTINPREIN = (pTxFramingParam->INPrein+1)/2;

      /* UpstreamCurrRate, DownstreamCurrRate,  */
      testParamObj->upstreamCurrRate = adslMib.xdslInfo.dirInfo[1].lpInfo[0].dataRate;
      testParamObj->downstreamCurrRate = adslMib.xdslInfo.dirInfo[0].lpInfo[0].dataRate;

      /* before getting tone data, first set the gfactor */
      oidStr1[2] = kOidAdslPrivSetFlagActualGFactor;
      gFactor = 1;
      len = 1;
      if((cmsAdsl_setAdslMibObject(oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) BCMADSL_STATUS_SUCCESS)
      {
         /* get SNR */
         oidStr[1] = kOidAdslPrivSNRDsPerToneGroup;
         len = sizeof(subcarrierData);
         cmsAdsl_getAdslMibObject(oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->SNRpsds,subcarrierData,(char*)"SNRpsds",MAX_PS_STRING);
      }
      len = 1;

      if((cmsAdsl_setAdslMibObject(oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) BCMADSL_STATUS_SUCCESS)
      {
         oidStr[1] = kOidAdslPrivSNRUsPerToneGroup;
         len = sizeof(subcarrierData);
         cmsAdsl_getAdslMibObject(oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->SNRpsus,subcarrierData,(char*)"SNRpsus",MAX_PS_STRING);
      }

      /* NFEC, RFEC */
      testParamObj->NFEC = adslMib.xdslInfo.dirInfo[0].lpInfo[0].N;
      testParamObj->RFEC = adslMib.xdslInfo.dirInfo[0].lpInfo[0].R;

      ret = CMSRET_SUCCESS;
   } /* get mib statistics ok */
   return ret;
} /* rutdsl_getAdslTestParamsInfo_dev2 */

/* line stats total */
CmsRet rutfast_getTotalStats_dev2(Dev2FastLineStatsTotalObject *obj)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (obj == NULL)
   {
      return (CMSRET_SUCCESS);
   }

   if (cmsAdsl_getAdslMib(&adslMib) == CMSRET_SUCCESS)
   {  
      obj->erroredSecs = adslMib.adslPerfData.perfTotal.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfTotal.adslSES;
      obj->LOSS = adslMib.adslPerfData.perfTotal.adslLoss;
      obj->UAS = adslMib.adslPerfData.perfTotal.adslUAS;
      obj->LORS = adslMib.adslPerfData.perfTotal.xdslLORS;
      obj->RTXUC = adslMib.rtxCounterData.cntDS.perfTotal.rtx_uc;
      obj->RTXTX = adslMib.rtxCounterData.cntUS.perfTotal.rtx_tx;
      obj->successBSW = adslMib.gfastOlrCounterData.cntDS.perfTotal.bswCompleted;
      obj->successSRA = adslMib.gfastOlrCounterData.cntDS.perfTotal.sraCompleted;
      obj->successFRA = adslMib.gfastOlrCounterData.cntDS.perfTotal.fraCompleted;
      obj->successRPA = adslMib.gfastOlrCounterData.cntDS.perfTotal.rpaCompleted;
      obj->successTIGA = adslMib.gfastOlrCounterData.cntDS.perfTotal.tigaCompleted;
   }

   return (CMSRET_SUCCESS);
} /* rutdsl_getTotalStats_dev2 */

CmsRet rutfast_getLineStats_dev2(Dev2FastLineStatsObject *obj)
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   UINT16 xdslStatus;
   adslMibInfo adslMib;
   ADSL_CONNECTION_INFO adslConnInfo;
   CmsRet ret=CMSRET_SUCCESS;
   
   cmsLog_debug("Entered");

   memset(&adslConnInfo, 0, sizeof(ADSL_CONNECTION_INFO));
   ret = xdslCtl_GetConnectionInfo(0, &adslConnInfo);
   xdslStatus = adslConnInfo.LinkState;
   cmsLog_debug("adslConnInfo.LinkState=%d", xdslStatus);

   if (xdslStatus == BCM_ADSL_LINK_UP) 
   {      
      if (cmsAdsl_getAdslMib(&adslMib) == CMSRET_SUCCESS)
      {
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
         obj->bytesSent = adslMib.atmStat2lp[1].xmtStat.cntCellTotal;
         obj->bytesReceived = adslMib.atmStat2lp[1].rcvStat.cntCellData;
#else      
         obj->bytesSent = adslMib.atmStat2lp[0].xmtStat.cntCellTotal;
         obj->bytesReceived = adslMib.atmStat2lp[0].rcvStat.cntCellData;
#endif /* defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1) */
         obj->packetsSent = adslMib.adslStat.xmtStat.cntSF;
         obj->packetsReceived = adslMib.adslStat.rcvStat.cntSF;
         obj->errorsSent = adslMib.adslStat.xmtStat.cntSFErr;
         obj->errorsReceived = adslMib.adslStat.rcvStat.cntSFErr;
         obj->discardPacketsSent = adslMib.atmStat.xmtStat.cntCellDrop; 
         obj->discardPacketsReceived = adslMib.atmStat.rcvStat.cntCellDrop; 
         obj->totalStart = adslMib.adslPerfData.adslSinceDrvStartedTimeElapsed;
         obj->showtimeStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
         obj->lastShowtimeStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
         obj->currentDayStart = adslMib.adslPerfData.adslPerfCurr1DayTimeElapsed;
         obj->quarterHourStart = adslMib.adslPerfData.adslPerfCurr15MinTimeElapsed;
         obj->eocBytesSent = adslMib.adslStat.eocStat.bytesSent;
         obj->eocBytesReceived = adslMib.adslStat.eocStat.bytesReceived;
         obj->eocPacketsSent = adslMib.adslStat.eocStat.packetsSent;
         obj->eocPacketsReceived = adslMib.adslStat.eocStat.packetsReceived;
         obj->eocMessagesSent = adslMib.adslStat.eocStat.messagesSent;
         obj->eocMessagesReceived = adslMib.adslStat.eocStat.messagesReceived;
      } /* adslMib retrieved */
   }
   else
   {
      /* TR181: if down, cpe must reset statistics */
      obj->bytesSent = 0;
      obj->bytesReceived = 0;
      obj->packetsSent = 0;
      obj->packetsReceived = 0;
      obj->errorsSent = 0;
      obj->errorsReceived = 0;
      obj->discardPacketsSent = 0;
      obj->discardPacketsReceived = 0;
      obj->totalStart = 0;
      obj->showtimeStart = 0;
      obj->lastShowtimeStart = 0;
      obj->currentDayStart = 0;
      obj->quarterHourStart = 0;
      obj->eocBytesSent = 0;
      obj->eocBytesReceived = 0;
      obj->eocPacketsSent = 0;
      obj->eocPacketsReceived = 0;
      obj->eocMessagesSent = 0;
      obj->eocMessagesReceived = 0;
   }

   return (ret);
#endif /* DESKTOP_LINUX */
} /* rutdsl_getdslLineStats_dev2 */


/* rutWan_getIntfInfo is splitted into line and channel info */
CmsRet rutfast_getLineInfo_dev2(Dev2FastLineObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   ADSL_CONNECTION_INFO adslConnInfo;
   CmsRet ret = CMSRET_SUCCESS;
   UINT16 xdslStatus;
   unsigned char pwrState;
   adslVersionInfo adslVer;
   vdsl2ConnectionInfo *pVdsl2Info = &adslMib.vdslInfo[0];
   char value[BUFLEN_32];
   char   oidStr[] = { 95 };  /* kOidAdslPhyCfg */
   adslCfgProfile adslCfg;
   long   dataLen = sizeof(adslCfgProfile);
   int len;

   if (obj == NULL)
   {
      return ret;
   }

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   memset(&adslConnInfo, 0, sizeof(ADSL_CONNECTION_INFO));
   /* get link number (0 or 1) from the instance ID?  This parameter is kept for API purposes */
   ret = xdslCtl_GetConnectionInfo(obj->X_BROADCOM_COM_BondingLineNumber, &adslConnInfo);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("xdslCtl_GetConnectionInfo failed, ret=%d", ret);
      return ret;
   }

   xdslStatus = adslConnInfo.LinkState;
   cmsLog_debug("adslConnInfo.LinkState=%d", xdslStatus);
   
   /* If the driver returns BCM_XDSL_LINK_DOWN, this is the same as NOSIGNAL and DISABLED, no change is needed.
    * If link is training, and not up yet, we have no choice but to update both instances with such
    * status since we don't know if this is PTM or ATM.
    * If the driver status is BCM_XDSL_LINK_UP, we now know that we are either PTM or ATM,
    * we update the correct instance with the status.
    * Then the other instance will have status updated to DISABLED.
    */
   if (xdslStatus == BCM_ADSL_LINK_DOWN)
   {
      cmsLog_debug("BCM_XDSL_LINK_DOWN (1)");
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }
   else if (xdslStatus == BCM_ADSL_TRAINING_G994)
   {
      /* I'm just guessing that this XDSL state maps to initializing */
      cmsLog_debug("BCM_XDSL_TRAINING_G994 (5)");
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }
   else if (xdslStatus >= BCM_ADSL_TRAINING_G992_EXCHANGE && xdslStatus <= BCM_ADSL_TRAINING_G993_STARTED)
   {
      /* I'm just guessing that these XDSL states map to establishing link */
      cmsLog_debug("xdsl training %d", xdslStatus);
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }
   else if (xdslStatus != BCM_ADSL_LINK_UP)
   {
      /* TR69 also specifies error status.  Don't know which XDSL state
       * corresponds to that. */
      cmsLog_debug("some other kind of status %d", xdslStatus);
      if (cmsUtl_strcmp(obj->status,MDMVS_UP) == 0)
          CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }


   if (xdslStatus == BCM_ADSL_LINK_UP)
   {
      long adslMibSize=sizeof(adslMib);

      cmsLog_debug("BCM_XDSL_LINK_UP (0)");

      /* line 0 and line 1: if not bonding, bondingLineNumber is always 0 */
      if (CMSRET_SUCCESS != xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber, NULL, 0, (char *) &adslMib, &adslMibSize))
      {
         cmsLog_error("could not get MIB for line %d", obj->X_BROADCOM_COM_BondingLineNumber);
         return CMSRET_INTERNAL_ERROR;
      }

      if(adslMib.adslTrainingState != kAdslTrainingConnected) 
      {
         cmsLog_debug("adslTraining state: initializing");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
         return CMSRET_SUCCESS;
      }

      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);

      /* update firmware version */
      memset((void*)&adslVer, 0, sizeof(adslVer));
      xdslCtl_GetVersion(0, &adslVer);
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->firmwareVersion,adslVer.phyVerStr,mdmLibCtx.allocFlags);

      pwrState = adslMib.xdslInfo.pwrState;
      if ( 0 == pwrState ) 
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->powerManagementState,MDMVS_L0,mdmLibCtx.allocFlags);
      }
      else
      {
         /* currently, only L0 and L3.  L2.1 and L2.2 are still under study */
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->powerManagementState,MDMVS_L3,mdmLibCtx.allocFlags);
      }
      
      cmsLog_debug("Power state: %s",obj->powerManagementState);

      obj->downstreamNoiseMargin = adslMib.adslPhys.adslCurrSnrMgn;
      obj->upstreamNoiseMargin = adslMib.adslAtucPhys.adslCurrSnrMgn;
#ifdef DMP_DEVICE2_VDSL2_1 /* same for gfast */
      obj->downstreamAttenuation = adslMib.adslPhys.adslCurrAtn;
      obj->upstreamAttenuation = adslMib.adslAtucPhys.adslCurrAtn;
#endif
      obj->downstreamPower = adslMib.adslAtucPhys.adslCurrOutputPwr;
      obj->upstreamPower = adslMib.adslPhys.adslCurrOutputPwr;
      obj->downstreamMaxBitRate = adslMib.adslPhys.adslCurrAttainableRate / 1000; 
      obj->upstreamMaxBitRate = adslMib.adslAtucPhys.adslCurrAttainableRate / 1000;

      /* current profile, the current gfast profile is 106a. The other profile is still under study */
      if(kXdslModGfast == adslMib.adslConnection.modType) {
         /* current profile */
         cmsLog_debug("vdsl2Profile=0x%x", pVdsl2Info->vdsl2Profile);
         switch (pVdsl2Info->vdsl2Profile)
         {
         case kGfastProfile106a:
            strcpy(value,"106a");
            break;
         case kGfastProfile212a:
            strcpy(value,"212a");
            break;
         case kGfastProfile106b:
            strcpy(value,"106b");
            break;
         case kGfastProfile106c:
            strcpy(value,"106c");
            break;
         case kGfastProfile212c:
            strcpy(value,"212c");
            break;
         default:
            if (pVdsl2Info->vdsl2Profile != 0)
            {
               cmsLog_error("unrecognized profile 0x%x (set to 106a)", pVdsl2Info->vdsl2Profile);
            }
            strcpy(value,"106a");
            break;
         } /* switch vdsl2Profile */

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->currentProfile, value, mdmLibCtx.allocFlags);

         xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber,oidStr,sizeof(oidStr),(char*)&adslCfg, &dataLen);
         value[0] = '\0';
         if (0 == (adslCfg.vdslParam & kGfastProfile106aDisable))
         {
            strcat(value,"106a,");
         }
         if (0 == (adslCfg.vdslParam & kGfastProfile212aDisable))
         {
            strcat(value,"212a,");
         }
         if (0 == (adslCfg.vdslParam & kGfastProfile106bDisable))
         {
            strcat(value,"106b,");
         }
         if (0 == (adslCfg.vdslParam & kGfastProfile106cDisable))
         {
            strcat(value,"106c,");
         }
         if (0 == (adslCfg.vdslParam & kGfastProfile212cDisable))
         {
            strcat(value,"212c,");
         }
         /* wipe out the last , */
         len = strlen(value);
         value[len-1] = '\0';
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->allowedProfiles, value, mdmLibCtx.allocFlags);
      }

      /* TR98 defines a range value (0 to 1280) for parameter UPBOKLE.
       * The value SHALL be coded as an unsigned 16 bit number in the range
       * 0 (coded as 0) to 128 dB (coded as 1280) in steps of 0.1 dB.
       * AdslMib UPBOkle is reported by DSL/PHY driver in steps of 0.01 dB,
       * instead of 0.1 dB. That makes the UPBOkle value 10 times bigger.
       * Therefore, we need to devide the value of UPBOkle by 10. 
       */ 
      obj->UPBOKLE = adslMib.xdslPhys.UPBOkle / 10;
      if (obj->UPBOKLE > 1280)
      {
         /* adslMib UPBOkle exceeds max value defined by TR98.
          * Set the parameter to max so that when cmsObj_get()
          * is called to get the DslIntfCfg object, mdm will not
          * detect an out-of-range value and return error.
          */
         cmsLog_error("adslMib UPBOkle %d is out of range. Set DslIntfCfg UPBOKLE to max 1280.", obj->UPBOKLE);
         obj->UPBOKLE = 1280; 
      }

      /* these are per band data (BITSRMCpsds, BITSRMCpsus */
      /* to be filled in when driver dta is available */

#if 0
      /* BITSRMCpsds should be similar to this */
      dataLen=sizeof(bandPlanDescriptor32);
      xdslCtl_GetObjectValue(0, oidStr1, sizeof(oidStr1), (char *)&dsNegBandPlanDiscPresentation, &dataLen);
      
      /* get usNegBandPlanDiscPresentation*/
      oidStr1[2]=kOidAdslPrivBandPlanUSNegDiscoveryPresentation;
      xdslCtl_GetObjectValue(0, oidStr1, sizeof(oidStr1), (char *)&usNegBandPlanDiscPresentation, &dataLen);
      
      dataLen = 5*sizeof(short);
      oidStr2[1]=kOidAdslPrivSNRMusperband;
      xdslCtl_GetObjectValue(0, oidStr2, sizeof(oidStr2), (char *)&data[0], &dataLen);
      dataStr[0] = '\0';
      for(n=0;n<=4;n++) 
      {
         if(n<usNegBandPlanDiscPresentation.noOfToneGroups)
         {
            substrPtr=&substr[0];
            if (n!=0)
            {
               substr[0]=',';
               substrPtr++;
            }
            if(data[n]<-511 || data[n] >511)
            {
               sprintf(substrPtr,"-512");
            }
            else
            {
               sprintf(substrPtr,"%d",data[n]);
            }
         }
         subStrLen = strlen(substr);
         dataStrLen = strlen(dataStr);
         if ((dataStrLen+subStrLen) < BUFLEN_24)
         {
            strcat(dataStr,substr);
         }
         else
         {
            /* just stop printing more data */
            break;
         }
      }
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->SNRMpbus,dataStr,mdmLibCtx.allocFlags);
      memset(dataStr,0,sizeof(dataStr));
      dataLen = 5*sizeof(short);
      oidStr2[1]=kOidAdslPrivSNRMdsperband;
      xdslCtl_GetObjectValue(0, oidStr2, sizeof(oidStr2), (char *)&data[0], &dataLen);
      if (dsNegBandPlanDiscPresentation.noOfToneGroups==4)
      {
         numDs = 4;
      }
      else
      {
         numDs = 3;
      }
      for(n=0;n<numDs;n++) 
      {
         if(n<dsNegBandPlanDiscPresentation.noOfToneGroups)
         {
            substrPtr = &substr[0];
            if (n != 0)
            {
               substr[0] = ',';
               substrPtr++;
            }
            if(data[n]<-511 || data[n] >511)
            {
               sprintf(substrPtr,"-512");
            }
            else
            {
               sprintf(substrPtr,"%d",data[n]);
            }
         }
         subStrLen = strlen(substr);
         dataStrLen = strlen(dataStr);
         if ((dataStrLen+subStrLen) < BUFLEN_24)
         {
            strcat(dataStr,substr);
         }
         else
         {
            /* just stop printing more data */
            break;
         }
      }
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->SNRMpbds,dataStr,mdmLibCtx.allocFlags);
#endif /* 0 */
      
      /* to be fill in when driver data is available */
      /* obj->successFailureCause = 0; notSupported */
      obj->UPBOKLER = 0;

      obj->lastTransmittedDownstreamSignal = adslMib.adslDiag.ldLastStateDS;
      obj->lastTransmittedUpstreamSignal = adslMib.adslDiag.ldLastStateUS;;
      obj->SNRRMCds = adslMib.xdslPhys.snrmRoc;
      obj->SNRRMCus = adslMib.xdslAtucPhys.snrmRoc;
      obj->FEXTCANCELds = FALSE;
      obj->FEXTCANCELus = FALSE;
      obj->ETRds = adslMib.xdslInfo.dirInfo[0].lpInfo[0].etrRate;
      obj->ETRus = adslMib.xdslInfo.dirInfo[1].lpInfo[0].etrRate;
      obj->ATTETRds = 0;
      obj->ATTETRus = 0;
      obj->MINEFTR = adslMib.adslStat.ginpStat.cntDS.minEFTR;

   } /* LINK UP */
   cmsLog_debug("End: ret %d",ret);
   
   return (ret);
} /* get fastLineInfo */

UBOOL8 rutfast_isConfigChanged_dev2(const _Dev2FastLineObject *newObj, const  _Dev2FastLineObject *currObj)
{
   UBOOL8 changed=FALSE;
   
   if (!POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      return FALSE;
   }
   /* checking for potential profiles changes on FAST object */
   if ((newObj->X_BROADCOM_COM_FAST_106a != currObj->X_BROADCOM_COM_FAST_106a) ||
       (newObj->X_BROADCOM_COM_FAST_212a != currObj->X_BROADCOM_COM_FAST_212a) ||
       (newObj->X_BROADCOM_COM_FAST_106b != currObj->X_BROADCOM_COM_FAST_106b) ||
       (newObj->X_BROADCOM_COM_FAST_106c != currObj->X_BROADCOM_COM_FAST_106c) ||
       (newObj->X_BROADCOM_COM_FAST_212c != currObj->X_BROADCOM_COM_FAST_212c)
       )
   {
      changed = TRUE;
   }
   return (changed);
}

void rutfast_cfgProfileInit_dev2(adslCfgProfile * pAdslCfg,  unsigned char lineId)
{
   Dev2FastLineObject *pFastLineObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;

   while (!found && (cmsObj_getNext(MDMOID_DEV2_FAST_LINE, &iidStack, (void **) &pFastLineObj) == CMSRET_SUCCESS))
   {
      if (pFastLineObj->X_BROADCOM_COM_BondingLineNumber == lineId)
      {
         found = TRUE;
         break;
      }
      cmsObj_free((void **) &pFastLineObj);
   }
   if (!found)
   {
      return;
   }
   if(!pFastLineObj->X_BROADCOM_COM_FAST_106a)
   {
      pAdslCfg->vdslParam |= kGfastProfile106aDisable;
   }
   if(!pFastLineObj->X_BROADCOM_COM_FAST_212a)
   {
      pAdslCfg->vdslParam |= kGfastProfile212aDisable;
   }
   if(!pFastLineObj->X_BROADCOM_COM_FAST_106b)
   {
      pAdslCfg->vdslParam |= kGfastProfile106bDisable;
   }
   if(!pFastLineObj->X_BROADCOM_COM_FAST_106c)
   {
      pAdslCfg->vdslParam |= kGfastProfile106cDisable;
   }
   if(!pFastLineObj->X_BROADCOM_COM_FAST_212c)
   {
      pAdslCfg->vdslParam |= kGfastProfile212cDisable;
   }
   cmsObj_free((void **) &pFastLineObj);
}

void rutfast_intfCfgInit_dev2(adslCfgProfile *pAdslCfg, Dev2FastLineObject *pFastLineObj)
{
   pFastLineObj->X_BROADCOM_COM_FAST_106a = ((pAdslCfg->vdslParam & kGfastProfile106aDisable) == 0);
   pFastLineObj->X_BROADCOM_COM_FAST_212a = ((pAdslCfg->vdslParam & kGfastProfile212aDisable) == 0);
   pFastLineObj->X_BROADCOM_COM_FAST_106b = ((pAdslCfg->vdslParam & kGfastProfile106bDisable) == 0);
   pFastLineObj->X_BROADCOM_COM_FAST_106c = ((pAdslCfg->vdslParam & kGfastProfile106cDisable) == 0);
   pFastLineObj->X_BROADCOM_COM_FAST_212c = ((pAdslCfg->vdslParam & kGfastProfile212cDisable) == 0);
}


#endif /* DMP_DEVICE2_FAST_1 */


#endif /*  DMP_DEVICE2_BASELINE_1 */
