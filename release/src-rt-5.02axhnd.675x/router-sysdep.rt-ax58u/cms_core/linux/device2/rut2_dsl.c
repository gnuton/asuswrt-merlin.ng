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

#ifdef DMP_DEVICE2_DSL_1

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
#include "rut_atm.h"
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
#include "qdm_intf.h"
#include "rut2_fast.h"

extern int GetXmtRate(adslMibInfo *pMib, int pathId);
extern int GetRcvRate(adslMibInfo *pMib, int pathId);
extern int f2DecI(int val, int q);
extern int GetAdsl2Sq(adsl2DataConnectionInfo *p2, int q);
extern int f2DecF(int val, int q);
extern CmsRet rutDsl_configPPPoA(const char *cmdLineIn, const char *baseIfName, SINT32 *pppPid);

CmsRet rutdsl_getLineIdByLineIidStack_dev2(const InstanceIdStack *iidStack, UINT32 *lineId)
{
   CmsRet ret = CMSRET_INTERNAL_ERROR;
   Dev2DslLineObject *dslLineObj = NULL;

   if ((ret=cmsObj_get(MDMOID_DEV2_DSL_LINE, iidStack, OGF_NO_VALUE_UPDATE, (void **)&dslLineObj)) == CMSRET_SUCCESS)
   {
      *lineId = dslLineObj->X_BROADCOM_COM_BondingLineNumber;
      cmsObj_free((void **)&dslLineObj);
   }
   else
   {
      cmsLog_error("Fail to get cmsObj_get(MDMOID_DEV2_DSL_LINE). ret=%d", ret);
      *lineId = 0;
   }

   return ret;
}

CmsRet rutdsl_getAdslMibByLineIidStack_dev2(const InstanceIdStack *iidStack, adslMibInfo *adslMib, UINT32 *line)
{
   long	size = sizeof(adslMibInfo);
   Dev2DslLineObject *dslLineObj;
   UINT32 lineId = 0;

   if (cmsObj_get(MDMOID_DEV2_DSL_LINE, iidStack, OGF_NO_VALUE_UPDATE, (void **)&dslLineObj) == CMSRET_SUCCESS)
   {
      lineId = dslLineObj->X_BROADCOM_COM_BondingLineNumber;
      cmsObj_free((void **)&dslLineObj);
   }

   if (line) 
      *line = lineId;

   return xdslCtl_GetObjectValue(lineId, NULL, 0, (char *)adslMib, &size);
}

CmsRet rutdsl_getLineIdByChannelIidStack_dev2(const InstanceIdStack *iidStack, UINT32 *lineId)
{
   CmsRet ret = CMSRET_INTERNAL_ERROR;
   MdmPathDescriptor pathDesc;
   Dev2DslChannelObject *dslChannelObj = NULL;
   Dev2DslLineObject *dslLineObj = NULL;
   
   *lineId = 0;
   if ((ret=cmsObj_get(MDMOID_DEV2_DSL_CHANNEL,iidStack,OGF_NO_VALUE_UPDATE,(void **)&dslChannelObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Fail to get cmsObj_get(MDMOID_DEV2_DSL_CHANNEL). ret=%d", ret);
      goto out;
   }
   INIT_PATH_DESCRIPTOR(&pathDesc);
   if ((ret=cmsMdm_fullPathToPathDescriptor(dslChannelObj->lowerLayers, &pathDesc)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", dslChannelObj->lowerLayers, ret);
      goto release_channel;
   }
   if ((ret = cmsObj_get(MDMOID_DEV2_DSL_LINE,&pathDesc.iidStack,OGF_NO_VALUE_UPDATE,(void **)&dslLineObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Cannot get line object, ret=%d", ret);
      goto release_channel;
   }
   *lineId = dslLineObj->X_BROADCOM_COM_BondingLineNumber;
   cmsObj_free((void **) &dslLineObj);

release_channel:
   cmsObj_free((void **) &dslChannelObj);
out:
   return ret;
}

CmsRet rutdsl_getAdslMibByChannelIidStack_dev2(const InstanceIdStack *iidStack, adslMibInfo *adslMib, UINT32 *line)
{
   long	size = sizeof(adslMibInfo);
   UINT32 lineId = 0;

   rutdsl_getLineIdByChannelIidStack_dev2(iidStack, &lineId);
   if (line) 
      *line = lineId;

   return xdslCtl_GetObjectValue(lineId, NULL, 0, (char *)adslMib, &size);
}

 
void xdslUtil_CfgProfileInit_dev2(adslCfgProfile * pAdslCfg,  Dev2DslLineObject *pDslLineObj)
{
    long dslCfgParam = pDslLineObj->X_BROADCOM_COM_DslCfgParam;

    pAdslCfg->adslHsModeSwitchTime = pDslLineObj->X_BROADCOM_COM_DslHsModeSwitchTime;
    pAdslCfg->adslLOMTimeThldSec = pDslLineObj->X_BROADCOM_COM_DslLOMTimeThldSec;
    pAdslCfg->adslPwmSyncClockFreq = pDslLineObj->X_BROADCOM_COM_DslPwmSyncClockFreq;
    pAdslCfg->adslShowtimeMarginQ4 = pDslLineObj->X_BROADCOM_COM_DslShowtimeMarginQ4;
    pAdslCfg->adslTrainingMarginQ4 = pDslLineObj->X_BROADCOM_COM_DslTrainingMarginQ4;
    pAdslCfg->adslDemodCapMask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg1Mask;
    pAdslCfg->adslDemodCapValue = pDslLineObj->X_BROADCOM_COM_DslPhyCfg1Value;
    pAdslCfg->adslDemodCap2Mask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg2Mask;
    pAdslCfg->adslDemodCap2Value = pDslLineObj->X_BROADCOM_COM_DslPhyCfg2Value;
    pAdslCfg->adsl2Param = pDslLineObj->X_BROADCOM_COM_DslParam;
#ifdef SUPPORT_CFG_PROFILE
    pAdslCfg->xdslAuxFeaturesMask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg3Mask;
    pAdslCfg->xdslAuxFeaturesValue = pDslLineObj->X_BROADCOM_COM_DslPhyCfg3Value;
    pAdslCfg->vdslCfgFlagsMask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg4Mask;
    pAdslCfg->vdslCfgFlagsValue = pDslLineObj->X_BROADCOM_COM_DslPhyCfg4Value;
    pAdslCfg->xdslCfg1Mask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg5Mask;
    pAdslCfg->xdslCfg1Value = pDslLineObj->X_BROADCOM_COM_DslPhyCfg5Value;
    pAdslCfg->xdslCfg2Mask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg6Mask;
    pAdslCfg->xdslCfg2Value = pDslLineObj->X_BROADCOM_COM_DslPhyCfg6Value;
    pAdslCfg->xdslCfg3Mask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg7Mask;
    pAdslCfg->xdslCfg3Value = pDslLineObj->X_BROADCOM_COM_DslPhyCfg7Value;
    pAdslCfg->xdslCfg4Mask = pDslLineObj->X_BROADCOM_COM_DslPhyCfg8Mask;
    pAdslCfg->xdslCfg4Value = pDslLineObj->X_BROADCOM_COM_DslPhyCfg8Value;
#endif
    pAdslCfg->maxUsDataRateKbps = pDslLineObj->X_BROADCOM_COM_DslPhyUsDataRateKbps;
    pAdslCfg->maxDsDataRateKbps = pDslLineObj->X_BROADCOM_COM_DslPhyDsDataRateKbps;
    pAdslCfg->maxAggrDataRateKbps = pDslLineObj->X_BROADCOM_COM_DslPhyAggrDataRateKbps;
    pAdslCfg->xdslMiscCfgParam = pDslLineObj->X_BROADCOM_COM_DslPhyMiscCfgParam;
    
    cmsLog_debug("AdslModulationCfg=%s\n", pDslLineObj->X_BROADCOM_COM_AdslModulationCfg);

    /* Modulation type */
    dslCfgParam &= ~kAdslCfgModMask;
    if(cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg, MDMVS_ADSL_MODULATION_ALL)) {
        /* Note: MDMVS_ADSL_MODULATION_ALL does not include AnnexM */
        dslCfgParam |= kAdslCfgModGdmtOnly | kAdslCfgModGliteOnly | kAdslCfgModAdsl2Only | kAdslCfgModAdsl2pOnly | kAdslCfgModT1413Only;
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
        dslCfgParam |= kDslCfgModVdsl2Only;
#ifdef SUPPORT_DSL_GFAST
        dslCfgParam |= kDslCfgModGfastOnly;
#endif
#endif
        pAdslCfg->adsl2Param |= kAdsl2CfgReachExOn;
    }
    else {
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_G_DMT))
            dslCfgParam |= kAdslCfgModGdmtOnly;
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_G_LITE))
            dslCfgParam |= kAdslCfgModGliteOnly;
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_G_DMT_BIS))
            dslCfgParam |= kAdslCfgModAdsl2Only;
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_RE_ADSL))
            pAdslCfg->adsl2Param |= kAdsl2CfgReachExOn;
        else
            pAdslCfg->adsl2Param &= ~kAdsl2CfgReachExOn;
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_2PLUS))
            dslCfgParam |= kAdslCfgModAdsl2pOnly;
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_ANSI_T1_413))
            dslCfgParam |= kAdslCfgModT1413Only;
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_VDSL2))
            dslCfgParam |= kDslCfgModVdsl2Only;
#ifdef SUPPORT_DSL_GFAST
        if (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg,MDMVS_G_FAST))
            dslCfgParam |= kDslCfgModGfastOnly;
#endif
#endif
    }

    if ( (cmsUtl_isSubOptionPresent(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg, MDMVS_ANNEXM)) ||
        (pDslLineObj->X_BROADCOM_COM_ADSL2_AnnexM == TRUE) )
        pAdslCfg->adsl2Param |= kAdsl2CfgAnnexMEnabled;
    else
        pAdslCfg->adsl2Param &= ~(kAdsl2CfgAnnexMEnabled | kAdsl2CfgAnnexMOnly | kAdsl2CfgAnnexMpXMask);

    /* Phone line pair */
    dslCfgParam &= ~kAdslCfgLinePairMask;
    if (pDslLineObj->lineNumber == ADSL_LINE_INNER_PAIR)
        dslCfgParam |= kAdslCfgLineInnerPair;
    else
        dslCfgParam |= kAdslCfgLineOuterPair;

    /* Bit swap */
    pAdslCfg->adslDemodCapMask |= kXdslBitSwapEnabled;
    if (!strcmp(pDslLineObj->X_BROADCOM_COM_Bitswap, MDMVS_ON))
        pAdslCfg->adslDemodCapValue |= kXdslBitSwapEnabled;
    else
        pAdslCfg->adslDemodCapValue &= ~kXdslBitSwapEnabled;
    
    /* SRA */
    pAdslCfg->adslDemodCapMask |= kXdslSRAEnabled;
    if (!strcmp(pDslLineObj->X_BROADCOM_COM_SRA, MDMVS_ON))
        pAdslCfg->adslDemodCapValue |= kXdslSRAEnabled;
    else
        pAdslCfg->adslDemodCapValue &= ~kXdslSRAEnabled;
    
    if(pAdslCfg->adsl2Param & kAdsl2CfgAnnexMEnabled) {
        if(kAdslCfgModAny == (dslCfgParam & kAdslCfgModMask))
            pAdslCfg->adsl2Param |= kAdsl2CfgAnnexMOnly;
        else
            pAdslCfg->adsl2Param &= ~kAdsl2CfgAnnexMOnly;
    }
#ifdef DMP_VDSL2WAN_1
    pAdslCfg->vdslParam = 0;
    pAdslCfg->vdslParam1 = 0;
    if( dslCfgParam & kDslCfgModVdsl2Only ){
        if(pDslLineObj->X_BROADCOM_COM_VDSL_8a)
            pAdslCfg->vdslParam |= kVdslProfile8a;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_8b)
            pAdslCfg->vdslParam |= kVdslProfile8b;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_8c)
            pAdslCfg->vdslParam |= kVdslProfile8c;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_8d)
            pAdslCfg->vdslParam |= kVdslProfile8d;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_12a)
            pAdslCfg->vdslParam |= kVdslProfile12a;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_12b)
            pAdslCfg->vdslParam |= kVdslProfile12b;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_17a)
            pAdslCfg->vdslParam |= kVdslProfile17a;
        if(pDslLineObj->X_BROADCOM_COM_VDSL_30a)
            pAdslCfg->vdslParam |= kVdslProfile30a;
#ifdef CONFIG_VDSLBRCMPRIV1_SUPPORT
        if(pDslLineObj->X_BROADCOM_COM_VDSL_BrcmPriv1)
            pAdslCfg->vdslParam |= kVdslProfileBrcmPriv1;
#endif
#ifdef CONFIG_VDSLBRCMPRIV2_SUPPORT
        if(pDslLineObj->X_BROADCOM_COM_VDSL_BrcmPriv2)
            pAdslCfg->vdslParam |= kVdslProfileBrcmPriv2;
#endif
        if(pDslLineObj->X_BROADCOM_COM_VDSL_US0_8a)
            pAdslCfg->vdslParam |= kVdslUS0Mask;
    }
#ifdef SUPPORT_DSL_GFAST
    if(dslCfgParam & kDslCfgModGfastOnly) {
       rutfast_cfgProfileInit_dev2(pAdslCfg,pDslLineObj->X_BROADCOM_COM_BondingLineNumber);
    }
#endif
#endif

    dslCfgParam &= ~kAdslCfgDemodCapMask;
    if(pAdslCfg->adslDemodCapMask)
        dslCfgParam |= kAdslCfgDemodCapOn;

    dslCfgParam &= ~kAdslCfgDemodCap2Mask;
    if(pAdslCfg->adslDemodCap2Mask)
        dslCfgParam |= kAdslCfgDemodCap2On;

    dslCfgParam &= ~kAdslCfgTrellisMask;
    if(pAdslCfg->adslDemodCapValue&kXdslTrellisEnabled)
        dslCfgParam |= kAdslCfgTrellisOn;
    else
        dslCfgParam |= kAdslCfgTrellisOff;

#ifdef ANNEX_C
    pAdslCfg->adslAnnexCParam = dslCfgParam;
#else
    pAdslCfg->adslAnnexAParam = dslCfgParam;
#endif
    cmsLog_debug("*** adslCfgParam=0x%X vdslParam=0x%X ***\n", dslCfgParam, pAdslCfg->vdslParam);
}

void xdslUtil_IntfCfgInit_dev2(adslCfgProfile *pAdslCfg,  Dev2DslLineObject *pDslLineObj)
{
    int len;
    char    cfgModType[BUFLEN_128];
#ifdef ANNEX_C
    ulong   dslCfgParam = pAdslCfg->adslAnnexCParam;
#else
    ulong   dslCfgParam = pAdslCfg->adslAnnexAParam;
#endif
    pDslLineObj->X_BROADCOM_COM_DslCfgParam = dslCfgParam;
    pDslLineObj->X_BROADCOM_COM_DslHsModeSwitchTime = pAdslCfg->adslHsModeSwitchTime;
    pDslLineObj->X_BROADCOM_COM_DslLOMTimeThldSec = pAdslCfg->adslLOMTimeThldSec;
    pDslLineObj->X_BROADCOM_COM_DslPwmSyncClockFreq = pAdslCfg->adslPwmSyncClockFreq;
    pDslLineObj->X_BROADCOM_COM_DslShowtimeMarginQ4 = pAdslCfg->adslShowtimeMarginQ4;
    pDslLineObj->X_BROADCOM_COM_DslTrainingMarginQ4 = pAdslCfg->adslTrainingMarginQ4;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg1Mask = pAdslCfg->adslDemodCapMask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg1Value = pAdslCfg->adslDemodCapValue;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg2Mask = pAdslCfg->adslDemodCap2Mask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg2Value = pAdslCfg->adslDemodCap2Value;
    pDslLineObj->X_BROADCOM_COM_DslParam = pAdslCfg->adsl2Param;
#ifdef SUPPORT_CFG_PROFILE
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg3Mask = pAdslCfg->xdslAuxFeaturesMask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg3Value = pAdslCfg->xdslAuxFeaturesValue;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg4Mask = pAdslCfg->vdslCfgFlagsMask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg4Value = pAdslCfg->vdslCfgFlagsValue;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg5Mask = pAdslCfg->xdslCfg1Mask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg5Value = pAdslCfg->xdslCfg1Value;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg6Mask = pAdslCfg->xdslCfg2Mask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg6Value = pAdslCfg->xdslCfg2Value;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg7Mask = pAdslCfg->xdslCfg3Mask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg7Value = pAdslCfg->xdslCfg3Value;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg8Mask = pAdslCfg->xdslCfg4Mask;
    pDslLineObj->X_BROADCOM_COM_DslPhyCfg8Value = pAdslCfg->xdslCfg4Value;
#endif

    pDslLineObj->X_BROADCOM_COM_DslPhyUsDataRateKbps = pAdslCfg->maxUsDataRateKbps;
    pDslLineObj->X_BROADCOM_COM_DslPhyDsDataRateKbps = pAdslCfg->maxDsDataRateKbps;
    pDslLineObj->X_BROADCOM_COM_DslPhyAggrDataRateKbps = pAdslCfg->maxAggrDataRateKbps;
    pDslLineObj->X_BROADCOM_COM_DslPhyMiscCfgParam = pAdslCfg->xdslMiscCfgParam;

    /* Modulations */
    cmsMem_free(pDslLineObj->X_BROADCOM_COM_AdslModulationCfg);
    if((kAdslCfgModAny == (dslCfgParam & kAdslCfgModMask)) && !(pAdslCfg->adsl2Param & kAdsl2CfgAnnexMEnabled)) {
        pDslLineObj->X_BROADCOM_COM_AdslModulationCfg = cmsMem_strdup(MDMVS_ADSL_MODULATION_ALL);
    }
    else {
        memset(cfgModType, 0, BUFLEN_128);
        
        if(dslCfgParam & kAdslCfgModGdmtOnly) {
            strcat(cfgModType,MDMVS_ADSL_G_DMT);
            strcat(cfgModType,", ");
        }
        if(dslCfgParam & kAdslCfgModGliteOnly) {
            strcat(cfgModType,MDMVS_ADSL_G_LITE);
            strcat(cfgModType,", ");
        }
        if(dslCfgParam & kAdslCfgModT1413Only) {
            strcat(cfgModType,MDMVS_ADSL_ANSI_T1_413);
            strcat(cfgModType,", ");
        }
        if(dslCfgParam & kAdslCfgModAdsl2Only) {
            strcat(cfgModType,MDMVS_ADSL_G_DMT_BIS);
            strcat(cfgModType,", ");
        }
        if(pAdslCfg->adsl2Param & kAdsl2CfgReachExOn) {
            strcat(cfgModType,MDMVS_ADSL_RE_ADSL);
            strcat(cfgModType,", ");
        }
        if(dslCfgParam & kAdslCfgModAdsl2pOnly) {
            strcat(cfgModType,MDMVS_ADSL_2PLUS);
            strcat(cfgModType,", ");
        }
        if(pAdslCfg->adsl2Param & kAdsl2CfgAnnexMEnabled) {
            strcat(cfgModType,MDMVS_ANNEXM);
            strcat(cfgModType,", ");
        }
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
        if(dslCfgParam & kDslCfgModVdsl2Only) {
            strcat(cfgModType,MDMVS_VDSL2);
            strcat(cfgModType,", ");
        }
#ifdef SUPPORT_DSL_GFAST
        if(dslCfgParam & kDslCfgModGfastOnly) {
            strcat(cfgModType,MDMVS_G_FAST);
            strcat(cfgModType,", ");
        }
#endif
#endif
        /* take out the last ", " */
        len = strlen(cfgModType);
        if (len > 2) {
           cfgModType[len-2] = '\0';
           pDslLineObj->X_BROADCOM_COM_AdslModulationCfg = cmsMem_strdup(cfgModType);
        }
        else {
           /* default will be all */
           pDslLineObj->X_BROADCOM_COM_AdslModulationCfg = cmsMem_strdup(MDMVS_ADSL_MODULATION_ALL);
        }
    }
    pDslLineObj->X_BROADCOM_COM_ADSL2_AnnexM = ((pAdslCfg->adsl2Param & kAdsl2CfgAnnexMEnabled) != 0);

    /* VDSL2 profile */
#ifdef  DMP_VDSL2WAN_1
    pDslLineObj->X_BROADCOM_COM_VDSL_8a = ((pAdslCfg->vdslParam & kVdslProfile8a) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_8b = ((pAdslCfg->vdslParam & kVdslProfile8b) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_8c = ((pAdslCfg->vdslParam & kVdslProfile8c) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_8d = ((pAdslCfg->vdslParam & kVdslProfile8d) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_12a = ((pAdslCfg->vdslParam & kVdslProfile12a) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_12b = ((pAdslCfg->vdslParam & kVdslProfile12b) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_17a = ((pAdslCfg->vdslParam & kVdslProfile17a) != 0);
    pDslLineObj->X_BROADCOM_COM_VDSL_30a = ((pAdslCfg->vdslParam & kVdslProfile30a) != 0);
#ifdef CONFIG_VDSLBRCMPRIV1_SUPPORT
    pDslLineObj->X_BROADCOM_COM_VDSL_BrcmPriv1 = ((pAdslCfg->vdslParam & kVdslProfileBrcmPriv1) != 0);
#endif
#ifdef CONFIG_VDSLBRCMPRIV2_SUPPORT
    pDslLineObj->X_BROADCOM_COM_VDSL_BrcmPriv2 = ((pAdslCfg->vdslParam & kVdslProfileBrcmPriv2) != 0);
#endif
    pDslLineObj->X_BROADCOM_COM_VDSL_US0_8a = ((pAdslCfg->vdslParam & kVdslUS0Mask) != 0);
#endif

    /* Capability */
   /* sra */
    cmsMem_free(pDslLineObj->X_BROADCOM_COM_SRA);
    if (pAdslCfg->adslDemodCapValue & kXdslSRAEnabled)
        pDslLineObj->X_BROADCOM_COM_SRA = cmsMem_strdup(MDMVS_ON);
    else
        pDslLineObj->X_BROADCOM_COM_SRA = cmsMem_strdup(MDMVS_OFF);
    /* bitswap */
    cmsMem_free(pDslLineObj->X_BROADCOM_COM_Bitswap);
    if (pAdslCfg->adslDemodCapValue & kXdslBitSwapEnabled)
        pDslLineObj->X_BROADCOM_COM_Bitswap = cmsMem_strdup(MDMVS_ON);
    else
        pDslLineObj->X_BROADCOM_COM_Bitswap = cmsMem_strdup(MDMVS_OFF);
    
    if(kAdslCfgLineOuterPair == (dslCfgParam & kAdslCfgLinePairMask))
       pDslLineObj->lineNumber = ADSL_LINE_OUTER_PAIR;
    else
       pDslLineObj->lineNumber = ADSL_LINE_INNER_PAIR; 
}

UBOOL8 rutdsl_isDslConfigChanged_dev2(const _Dev2DslLineObject *newObj, const _Dev2DslLineObject *currObj)
{
   UBOOL8 changed=FALSE;

   if (!POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      return FALSE;
   }

   /*
    * need to check for a lot more parameter than this.
    * But these will be enough to detect run-time changes of VDSL profiles.
    * The POTENTIAL_CHANGE_OF_EXISTING macro guarantees both pointers are not null.
    */
   if (
       (0 != strcmp(newObj->X_BROADCOM_COM_AdslModulationCfg, currObj->X_BROADCOM_COM_AdslModulationCfg)) ||
       (0 != strcmp(newObj->X_BROADCOM_COM_Bitswap, currObj->X_BROADCOM_COM_Bitswap)) ||
       (0 != strcmp(newObj->X_BROADCOM_COM_SRA, currObj->X_BROADCOM_COM_SRA)) ||
       (newObj->lineNumber != currObj->lineNumber) ||
       (newObj->X_BROADCOM_COM_ADSL2_AnnexM != currObj->X_BROADCOM_COM_ADSL2_AnnexM)
#ifdef DMP_VDSL2WAN_1
       || (newObj->X_BROADCOM_COM_VDSL_8a != currObj->X_BROADCOM_COM_VDSL_8a) ||
       (newObj->X_BROADCOM_COM_VDSL_8a != currObj->X_BROADCOM_COM_VDSL_8b) ||
       (newObj->X_BROADCOM_COM_VDSL_8c != currObj->X_BROADCOM_COM_VDSL_8c) ||
       (newObj->X_BROADCOM_COM_VDSL_8d != currObj->X_BROADCOM_COM_VDSL_8d) ||
       (newObj->X_BROADCOM_COM_VDSL_12a != currObj->X_BROADCOM_COM_VDSL_12a) ||
       (newObj->X_BROADCOM_COM_VDSL_12b != currObj->X_BROADCOM_COM_VDSL_12b) ||
       (newObj->X_BROADCOM_COM_VDSL_17a != currObj->X_BROADCOM_COM_VDSL_17a) ||
       (newObj->X_BROADCOM_COM_VDSL_30a != currObj->X_BROADCOM_COM_VDSL_30a) ||
       (newObj->X_BROADCOM_COM_VDSL_US0_8a != currObj->X_BROADCOM_COM_VDSL_US0_8a)
#ifdef CONFIG_VDSLBRCMPRIV1_SUPPORT
       || (newObj->X_BROADCOM_COM_VDSL_BrcmPriv1 != currObj->X_BROADCOM_COM_VDSL_BrcmPriv1)
#endif
#ifdef CONFIG_VDSLBRCMPRIV2_SUPPORT
       || (newObj->X_BROADCOM_COM_VDSL_BrcmPriv2 != currObj->X_BROADCOM_COM_VDSL_BrcmPriv2) 
#endif
#endif
       )
   {
      changed = TRUE;
   }
   return (changed);
}


CmsRet rutdsl_configUp_dev2(Dev2DslLineObject *dslLineObj)
{
   UBOOL8 xDSLBonding = FALSE;
   adslCfgProfile adslCfg;

   /* modulationType has been deprecated */
   if (!dslLineObj->X_BROADCOM_COM_Bitswap &&
       ((dslLineObj->lineNumber != 1) || (dslLineObj->lineNumber != 2)) &&
       !dslLineObj->X_BROADCOM_COM_SRA)
   {
      cmsLog_error("Invalid dslLineObj parameters");
      return CMSRET_INVALID_PARAM_VALUE;
   }

#ifdef DMP_VDSL2WAN_1
   cmsLog_debug("bitswap %s, phone line number (1=innerPair) %d, sra %s, modulation %s, profile 8a=%d 8b=%d 8c=%d 8c=%d",
                 dslLineObj->X_BROADCOM_COM_Bitswap,
                 dslLineObj->lineNumber,
                 dslLineObj->X_BROADCOM_COM_SRA,
                 dslLineObj->X_BROADCOM_COM_AdslModulationCfg,
                 dslLineObj->X_BROADCOM_COM_VDSL_8a,
                 dslLineObj->X_BROADCOM_COM_VDSL_8b,
                 dslLineObj->X_BROADCOM_COM_VDSL_8c,
                 dslLineObj->X_BROADCOM_COM_VDSL_8d);
#endif
   /* initialialize adslCfgProfile */
   memset((char *) &adslCfg, 0x00, sizeof(adslCfg));
   xdslUtil_CfgProfileInit_dev2(&adslCfg, dslLineObj);
#ifdef SUPPORT_DSL_GFAST
   rutfast_cfgProfileInit_dev2(&adslCfg,dslLineObj->X_BROADCOM_COM_BondingLineNumber);
#endif

#ifdef SUPPORT_DSL_BONDING
	xDSLBonding = rutDsl_isDslBondingEnabled();
#ifdef SUPPORT_MULTI_PHY
	if( !(adslCfg.xdslMiscCfgParam & BCM_SWITCHPHY_DISABLED) ){
		/* Image type sanity check */
		if( TRUE == xDSLBonding ) {
			if( BCM_IMAGETYPE_BONDING != (adslCfg.xdslMiscCfgParam & BCM_IMAGETYPE_MSK) ) {
				adslCfg.xdslMiscCfgParam = (adslCfg.xdslMiscCfgParam &~BCM_IMAGETYPE_MSK) | BCM_IMAGETYPE_BONDING;
				cmsLog_notice("xDSL image type is out of sync, XTM is in bonding mode");
			}
		}
		else {
			if( BCM_IMAGETYPE_SINGLELINE != (adslCfg.xdslMiscCfgParam & BCM_IMAGETYPE_MSK) ) {
				adslCfg.xdslMiscCfgParam = (adslCfg.xdslMiscCfgParam &~BCM_IMAGETYPE_MSK) | BCM_IMAGETYPE_SINGLELINE;
				cmsLog_notice("xDSL image type is out of sync, XTM is in non-bonding mode");
			}
		}
	}
#endif /* SUPPORT_MULTI_PHY */
#endif /* SUPPORT_DSL_BONDING */

   cmsAdsl_initialize(&adslCfg);
   {
      XTM_INITIALIZATION_PARMS InitParms;
      XTM_INTERFACE_CFG IntfCfg;

      memset((UINT8 *)  &InitParms, 0x00, sizeof(InitParms));
      memset((UINT8 *)  &IntfCfg, 0x00, sizeof(IntfCfg));
      
      InitParms.bondConfig.sConfig.ptmBond = BC_PTM_BONDING_DISABLE ;
      InitParms.bondConfig.sConfig.atmBond = BC_ATM_BONDING_DISABLE ;
      
      if (xDSLBonding == TRUE) {
         char cmdStr[BUFLEN_128];
         InitParms.bondConfig.sConfig.ptmBond = BC_PTM_BONDING_ENABLE ;
         InitParms.bondConfig.sConfig.atmBond = BC_ATM_BONDING_ENABLE ;
         cmsLog_notice("xDSLBonding is enabled, disable CMF");
         snprintf(cmdStr, sizeof(cmdStr), "cmf disable") ;
         rut_doSystemAction("rcl", cmdStr);  
      }

      cmsLog_debug("DSL PTM Bonding %s", InitParms.bondConfig.sConfig.ptmBond ? "Enable" : "Disable");
      cmsLog_debug("DSL ATM Bonding %s", InitParms.bondConfig.sConfig.atmBond ? "Enable" : "Disable");

#if defined(SUPPORT_DSL_BONDING)
      InitParms.bondConfig.sConfig.autoSenseAtm = BC_ATM_AUTO_SENSE_ENABLE ;
#if defined(SUPPORT_EXT_DSL_BONDING)
      /* 63268 currently has this configuration. */
      InitParms.ulPortConfig = PC_INTERNAL_EXTERNAL ;
#endif
#endif

      devCtl_xtmInitialize( &InitParms ) ;

      /* TBD. Need configuration to indicate how many ports to enable. */
      IntfCfg.ulIfAdminStatus = ADMSTS_UP;
      devCtl_xtmSetInterfaceCfg( PORT_PHY0_FAST, &IntfCfg );
      devCtl_xtmSetInterfaceCfg( PORT_PHY0_INTERLEAVED, &IntfCfg );
   }

   cmsAdsl_start();

   if (xDSLBonding == TRUE)
   {
      xdslCtl_ConnectionStart(1);   /* Start line 1 */
   }

	 return CMSRET_SUCCESS;
}

void rutdsl_configDown_dev2(void)
{
   cmsAdsl_stop();
   cmsAdsl_uninitialize();
}

/* line show time stats */
CmsRet rutdsl_getAdslShowTimeStats_dev2(Dev2DslLineStatsShowtimeObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {
      obj->erroredSecs = adslMib.adslPerfData.perfSinceShowTime.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfSinceShowTime.adslSES;
      return (CMSRET_SUCCESS);
   }
   else 
   {
      return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
   }
} /* rutdsl_getAdslShowTimeStats_dev2 */

CmsRet rutdsl_getAdslCurrentDayStats_dev2(Dev2DslLineStatsCurrentDayObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {  
      obj->erroredSecs = adslMib.adslPerfData.perfCurr1Day.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfCurr1Day.adslSES;
      return (CMSRET_SUCCESS);
   }
   else 
   {
      return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
   }
} /* rutdsl_getAdslCurrentDayStats_dev2 */

CmsRet rutdsl_getAdslQuarterHourStats_dev2(Dev2DslLineStatsQuarterHourObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {  
      obj->erroredSecs = adslMib.adslPerfData.perfCurr15Min.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfCurr15Min.adslSES;
      return (CMSRET_SUCCESS);
   }
   else 
   {
      return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
   }
} /* rutdsl_getAdslQuarterHourStats_dev2 */

CmsRet rutdsl_getAdslTestParamsInfo_dev2(void *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   long len;
   char oidStr[] = { kOidAdslPrivate, 0 };
   char   oidStr1[] = { kOidAdslPrivate, kOidAdslPrivExtraInfo, 0 };
   UINT16 bandObjData[NUM_BAND];
   SINT16 subcarrierData[NUM_TONE_GROUP];
   UINT8 gFactor = 1;
   Dev2DslLineTestParamsObject *testParamObj = (Dev2DslLineTestParamsObject*)obj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   char dataStr[24];
   UINT32 lineId;

   if (rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,&lineId) == CMSRET_SUCCESS)
   {
      /* get G-factor objects --VDSL only */
#ifdef DMP_DEVICE2_VDSL2_1
      testParamObj->HLOGGds = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSds;
      testParamObj->HLOGGus = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSus;
      testParamObj->QLNGds = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSds;
      testParamObj->QLNGus = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSus;
      testParamObj->SNRGds = adslMib.gFactors.Gfactor_MEDLEYSETds;
      testParamObj->SNRGus = adslMib.gFactors.Gfactor_MEDLEYSETus;
#endif
            
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
      if (adslMib.adslConnection.modType == kVdslModVdsl2)
      {      
         /* per-band objects --VDSL only */ 
         dataStr[0] = '\0';
         oidStr[1] = kOidAdslPrivLATNdsperband;
         len = sizeof(bandObjData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)bandObjData, &len);
         sprintf(dataStr,"%d,%d,%d,%d,%d",bandObjData[0],bandObjData[1],bandObjData[2],bandObjData[3],bandObjData[4]);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->LATNds,dataStr,mdmLibCtx.allocFlags);
         
         dataStr[0] = '\0';
         oidStr[1] = kOidAdslPrivLATNusperband;
         len = sizeof(bandObjData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&bandObjData, &len);
         sprintf(dataStr,"%d,%d,%d,%d,%d", bandObjData[0],bandObjData[1],bandObjData[2],bandObjData[3],bandObjData[4]);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->LATNus,dataStr,mdmLibCtx.allocFlags);
            
         dataStr[0] = '\0';
         oidStr[1] = kOidAdslPrivSATNdsperband;
         len = sizeof(bandObjData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&bandObjData, &len);
         sprintf(dataStr,"%d,%d,%d,%d,%d", bandObjData[0],bandObjData[1],bandObjData[2],bandObjData[3],bandObjData[4]);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->SATNds,dataStr,mdmLibCtx.allocFlags);

         dataStr[0] = '\0';
         oidStr[1] = kOidAdslPrivSATNusperband;
         len = sizeof(bandObjData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&bandObjData, &len);
         sprintf(dataStr,"%d,%d,%d,%d,%d", bandObjData[0],bandObjData[1],bandObjData[2],bandObjData[3],bandObjData[4]);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(testParamObj->SATNus,dataStr,mdmLibCtx.allocFlags);
         
      } /* vdsl only */
#endif /* defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1) */

      testParamObj->SNRMTds = 128;
      testParamObj->SNRMTus = 128;
      testParamObj->QLNMTds = 128;
      testParamObj->QLNMTus = 128;
      testParamObj->HLOGMTds = 32;
      testParamObj->HLOGMTus = 32;
      
      /* before getting tone data, first set the gfactor */
      oidStr1[2] = kOidAdslPrivSetFlagActualGFactor;
      gFactor = 1;
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) BCMADSL_STATUS_SUCCESS)
      {
         /* get QLNpsds */
         oidStr[1] = kOidAdslPrivQuietLineNoiseDsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->QLNpsds,subcarrierData,(char*)"QLNpsds",MAX_QLN_STRING);
      }

      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) BCMADSL_STATUS_SUCCESS)
      {
         /* get QLNpsus */
         oidStr[1] = kOidAdslPrivQuietLineNoiseUsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->QLNpsus,subcarrierData,(char*)"QLNpsus",MAX_QLN_STRING);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) BCMADSL_STATUS_SUCCESS)
      {
         /* get SNR */
         oidStr[1] = kOidAdslPrivSNRDsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->SNRpsds,subcarrierData,(char*)"SNRpsds",MAX_PS_STRING);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) BCMADSL_STATUS_SUCCESS)
      {
         oidStr[1] = kOidAdslPrivSNRUsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->SNRpsus,subcarrierData,(char*)"SNRpsus",MAX_PS_STRING);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) BCMADSL_STATUS_SUCCESS)
      {
         /* get HLOG */
         oidStr[1] = kOidAdslPrivChanCharLogDsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->HLOGpsds,subcarrierData,(char*)"HLOGpsds",MAX_LOGARITHMIC_STRING);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) BCMADSL_STATUS_SUCCESS)
      {
         oidStr[1] = kOidAdslPrivChanCharLogUsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->HLOGpsus,subcarrierData,(char*)"HLOGpsus",MAX_LOGARITHMIC_STRING);
      }
      ret = CMSRET_SUCCESS;
   } /* get mib statistics ok */
   return ret;
} /* rutdsl_getAdslTestParamsInfo_dev2 */

/* line stats total */
CmsRet rutdsl_getAdslTotalStats_dev2(Dev2DslLineStatsTotalObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (obj == NULL)
   {
      return (CMSRET_SUCCESS);
   }

   if (rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {  
      obj->erroredSecs = adslMib.adslPerfData.perfTotal.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfTotal.adslSES;
      obj->X_BROADCOM_COM_UpstreamEs = adslMib.adslTxPerfTotal.adslESs;
      obj->X_BROADCOM_COM_UpstreamSes = adslMib.adslTxPerfTotal.adslSES;
      obj->X_BROADCOM_COM_UpstreamUas = adslMib.adslTxPerfTotal.adslUAS;
      obj->X_BROADCOM_COM_DownstreamUas = adslMib.adslPerfData.perfTotal.adslUAS;
   }

   return (CMSRET_SUCCESS);
} /* rutdsl_getTotalStats_dev2 */

CmsRet rutdsl_getdslLineStats_dev2(Dev2DslLineStatsObject *obj, const InstanceIdStack *iidStack)
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   UINT16 xdslStatus;
   adslMibInfo adslMib;
   ADSL_CONNECTION_INFO adslConnInfo;
   CmsRet ret=CMSRET_SUCCESS;
   UINT32 lineId;
   
   cmsLog_debug("Entered");

   memset(&adslConnInfo, 0, sizeof(ADSL_CONNECTION_INFO));
   rutdsl_getLineIdByLineIidStack_dev2(iidStack, &lineId);
   ret = xdslCtl_GetConnectionInfo(lineId, &adslConnInfo);
   xdslStatus = adslConnInfo.LinkState;
   cmsLog_debug("adslConnInfo[%d].LinkState=%d", lineId, xdslStatus);

   if (xdslStatus == BCM_ADSL_LINK_UP) 
   {      
      if (rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
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
         obj->totalStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
         obj->showtimeStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
         obj->lastShowtimeStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
         obj->currentDayStart = adslMib.adslPerfData.adslPerfCurr1DayTimeElapsed;
         obj->quarterHourStart = adslMib.adslPerfData.adslPerfCurr15MinTimeElapsed;
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
   }

   return (ret);
#endif /* DESKTOP_LINUX */
} /* rutdsl_getdslLineStats_dev2 */

CmsRet rutdsl_getdslChannelStats_dev2(Dev2DslChannelStatsObject *obj, const InstanceIdStack *iidStack)
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   UINT16 xdslStatus;
   adslMibInfo adslMib;
   ADSL_CONNECTION_INFO adslConnInfo;
   CmsRet ret=CMSRET_SUCCESS;
   UINT32 lineId;
   
   cmsLog_debug("Entered");

   memset(&adslConnInfo, 0, sizeof(ADSL_CONNECTION_INFO));
   rutdsl_getLineIdByChannelIidStack_dev2(iidStack, &lineId);
   ret = xdslCtl_GetConnectionInfo(lineId, &adslConnInfo);
   xdslStatus = adslConnInfo.LinkState;
   cmsLog_debug("adslConnInfo.LinkState=%d", xdslStatus);

   if (xdslStatus == BCM_ADSL_LINK_UP) 
   {
      if (rutdsl_getAdslMibByChannelIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
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
         obj->totalStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
         obj->showtimeStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
         obj->lastShowtimeStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
         obj->currentDayStart = adslMib.adslPerfData.adslPerfCurr1DayTimeElapsed;
         obj->quarterHourStart = adslMib.adslPerfData.adslPerfCurr15MinTimeElapsed;
      } /* adslMib retrieved */
   }
   else
   {
      /* TR181: if down, cpe must reset statistics */
      memset(obj,0,sizeof(Dev2DslChannelStatsObject));
   }

   return (ret);
#endif /* DESKTOP_LINUX */
} /* rutdsl_getdslChannelStats_dev2 */

CmsRet rutdsl_getAdslBertInfo_dev2(void *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   Dev2DslLineBertTestObject *bertObj = (Dev2DslLineBertTestObject *)obj;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered");

   if ((ret = rutdsl_getAdslMibByLineIidStack_dev2(iidStack,&adslMib,NULL)) == CMSRET_SUCCESS)
   {
      if (adslMib.adslBertStatus.bertSecCur == 0)
      {
         if (cmsUtl_strcmp(bertObj->bertTestStatus, MDMVS_RUNNING) == 0) 
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(bertObj->bertTestMode,MDMVS_STOP,mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(bertObj->bertTestStatus,MDMVS_NOT_RUNNING,mdmLibCtx.allocFlags);
         }
         else
         {
            ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
         }
      }
      else
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(bertObj->bertTestStatus,MDMVS_RUNNING,mdmLibCtx.allocFlags);
      }
      bertObj->elapsedTime = adslMib.adslBertStatus.bertSecElapsed;
      bertObj->totalTime = adslMib.adslBertStatus.bertSecTotal;
      bertObj->bitsTestedCntHigh = adslMib.adslBertStatus.bertTotalBits.cntHi;
      bertObj->bitsTestedCntLow = adslMib.adslBertStatus.bertTotalBits.cntLo;
      bertObj->errBitsCntHigh = adslMib.adslBertStatus.bertErrBits.cntHi;
      bertObj->errBitsCntLow = adslMib.adslBertStatus.bertErrBits.cntLo;
   }
   return (ret);
}

CmsRet rutdsl_setAdslBertInfo_dev2(void *new, const void *curr, const InstanceIdStack *iidStack)
{
   Dev2DslLineBertTestObject *newObj = (Dev2DslLineBertTestObject*)new;
   Dev2DslLineBertTestObject *currObj = (Dev2DslLineBertTestObject*)curr;
   UINT32 lineId = 0;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered");

   rutdsl_getLineIdByLineIidStack_dev2(iidStack,&lineId);

   if (cmsUtl_strcmp(newObj->bertTestMode, MDMVS_START) == 0) 
   {
      if ((cmsUtl_strcmp(currObj->bertTestStatus, MDMVS_RUNNING) == 0) ||
          (newObj->bertTestDuration < 1)) 
      {
         /* if test is running or to be run without sufficient input, just return */
         return ret;
      }
      ret = xdslCtl_BertStartEx(lineId,(unsigned long)newObj->bertTestDuration);
   }
   else
   {
      if (cmsUtl_strcmp(currObj->bertTestStatus, MDMVS_RUNNING) == 0)
      {
         /* stop test */
         ret = xdslCtl_BertStopEx(lineId);
      }
   }
   return ret;
}

/* rutWan_getIntfInfo is splitted into line and channel info */
CmsRet rutdsl_getLineInfo_dev2(Dev2DslLineObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   ADSL_CONNECTION_INFO adslConnInfo;
   CmsRet ret = CMSRET_SUCCESS;
   UINT16 xdslStatus;
   char value[BUFLEN_32];
   unsigned char pwrState;
   int xDsl2Mode = 0;
   adsl2ConnectionInfo *pAdsl2Info = &adslMib.adsl2Info2lp[0];
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
   adsl2ConnectionInfo *pAdsl2Info2 = &adslMib.adsl2Info2lp[1];
   vdsl2ConnectionInfo *pVdsl2Info = &adslMib.vdslInfo[0];
   vdsl2ConnectionInfo *pVdsl2Info2 = &adslMib.vdslInfo[1];
   /* allowed profile */
   char   oidStr[] = { 95 };  /* kOidAdslPhyCfg */
   adslCfgProfile adslCfg;
   long   dataLen = sizeof(adslCfgProfile);
   int len;
   int n, numDs;
   short data[5];
   char  oidStr1[]={kOidAdslPrivate,kOidAdslPrivExtraInfo,kOidAdslPrivBandPlanDSNegDiscoveryPresentation};
   char  oidStr2[]={kOidAdslPrivate,kOidAdslPrivSNRMusperband};
   bandPlanDescriptor32	usNegBandPlanDiscPresentation,dsNegBandPlanDiscPresentation;
   char substr[BUFLEN_8], dataStr[BUFLEN_24];
   char *substrPtr;
   int subStrLen, dataStrLen;
#endif /* defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1) */
   adslVersionInfo adslVer;
   UBOOL8 catPhyType = FALSE;
   UBOOL8 gfast = FALSE;

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

      /* it is up, but I need to see if it is FAST mode or not */


      /* modulation type is replaced by standardUsed */
      switch (adslMib.adslConnection.modType)
      {
      case kAdslModAdsl2:
      case kAdslModAdsl2p:
      case kAdslModReAdsl2:
      case kVdslModVdsl2:
         xDsl2Mode = 1;
         break;

#ifdef SUPPORT_DSL_GFAST
      case kXdslModGfast:
         xDsl2Mode = 1;
         gfast = TRUE;
         break;
#endif

      default:
         xDsl2Mode = 0;
         break;
      } /* modulationType */

      if (gfast == FALSE)
      {
         /* if it's gfast, everything will be read and updated, except for the status because the status for FAST is in FAST object */
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
      }

      switch (adslMib.adslLine.adslLineCoding)
      {
      case kAdslLineCodingDMT:
         cmsLog_debug("kAdslLineCodingDMT");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->lineEncoding,MDMVS_DMT,mdmLibCtx.allocFlags);
         break;
      case kAdslLineCodingCAP:
         cmsLog_debug("kAdslLineCodingCAP");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->lineEncoding,MDMVS_CAP,mdmLibCtx.allocFlags);
         break;
      case kAdslLineCodingQAM:
         cmsLog_debug("kAdslLineCodingQAM");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->lineEncoding,MDMVS_QAM,mdmLibCtx.allocFlags);
         break;
      default:
         cmsLog_debug("adslLineInfo.adslLineCoding %d",adslMib.adslLine.adslLineCoding);
      } /* lineCoding */
      
      if( adslMib.adslConnection.modType < kAdslModAdsl2 ) 
      {
         if( kAdslTrellisOn == adslMib.adslConnection.trellisCoding ) 
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisD, MDMVS_ON, mdmLibCtx.allocFlags);
         }
         else 
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisD, MDMVS_OFF, mdmLibCtx.allocFlags);
         }
      }
      else 
      {
         if( adslMib.adslConnection.trellisCoding2 & kAdsl2TrellisRxEnabled )
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisD, MDMVS_ON, mdmLibCtx.allocFlags);
         }
         else 
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisD, MDMVS_OFF, mdmLibCtx.allocFlags);
         }
         if( adslMib.adslConnection.trellisCoding2 & kAdsl2TrellisTxEnabled ) 
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisU, MDMVS_ON, mdmLibCtx.allocFlags);
         }
         else 
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisU, MDMVS_OFF, mdmLibCtx.allocFlags);
         }
      }


      pwrState = adslMib.xdslInfo.pwrState;
      if ( 0 == pwrState ) 
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_LinkPowerState,MDMVS_L0,mdmLibCtx.allocFlags);
      }
      else if ( 2 == pwrState )
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_LinkPowerState,MDMVS_L2,mdmLibCtx.allocFlags);
      }
      else
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_LinkPowerState,MDMVS_L3,mdmLibCtx.allocFlags);
      }
      
      cmsLog_debug("Power state: %s",obj->X_BROADCOM_COM_LinkPowerState);

      obj->downstreamNoiseMargin = adslMib.adslPhys.adslCurrSnrMgn;
      obj->upstreamNoiseMargin = adslMib.adslAtucPhys.adslCurrSnrMgn;
#ifdef DMP_DEVICE2_VDSL2_1
      obj->downstreamAttenuation = adslMib.adslPhys.adslCurrAtn;
      obj->upstreamAttenuation = adslMib.adslAtucPhys.adslCurrAtn;
#endif
      obj->downstreamPower = adslMib.adslAtucPhys.adslCurrOutputPwr;
      obj->upstreamPower = adslMib.adslPhys.adslCurrOutputPwr;
      obj->downstreamMaxBitRate = adslMib.adslPhys.adslCurrAttainableRate / 1000; 
      obj->upstreamMaxBitRate = adslMib.adslAtucPhys.adslCurrAttainableRate / 1000;

      /* xDSL2 framing parameters */
      if ( xDsl2Mode )
      {
         xdslFramingInfo     *pRxFramingParam = &adslMib.xdslInfo.dirInfo[0].lpInfo[0];
         xdslFramingInfo     *pTxFramingParam = &adslMib.xdslInfo.dirInfo[1].lpInfo[0];
         
         cmsLog_debug("xDSL2 framing");
         
         /* Delay and INP for path 0 */
         snprintf(value, sizeof(value), "%d", pRxFramingParam->delay);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamDelay, value, mdmLibCtx.allocFlags);
         snprintf(value, sizeof(value), "%d", pTxFramingParam->delay);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamDelay, value, mdmLibCtx.allocFlags);
         snprintf(value, sizeof(value), "%4.2f", (float)pRxFramingParam->INP/2);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamINP, value, mdmLibCtx.allocFlags);
         snprintf(value, sizeof(value), "%4.2f", (float)pTxFramingParam->INP/2);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamINP, value, mdmLibCtx.allocFlags);

#if !defined(DMP_VDSL2WAN_1) && !defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
         /* ADSL2/ADSL2+ specific mode */
         obj->X_BROADCOM_COM_ADSL2_DownstreamMSGc = pAdsl2Info->rcv2Info.MSGc;
         obj->X_BROADCOM_COM_ADSL2_UpstreamMSGc = pAdsl2Info->xmt2Info.MSGc;
         obj->X_BROADCOM_COM_ADSL2_DownstreamB = pAdsl2Info->rcv2Info.B;
         obj->X_BROADCOM_COM_ADSL2_UpstreamB = pAdsl2Info->xmt2Info.B;
         obj->X_BROADCOM_COM_ADSL2_DownstreamM = pAdsl2Info->rcv2Info.M;
         obj->X_BROADCOM_COM_ADSL2_UpstreamM = pAdsl2Info->xmt2Info.M;
         obj->X_BROADCOM_COM_ADSL2_DownstreamT = pAdsl2Info->rcv2Info.T;
         obj->X_BROADCOM_COM_ADSL2_UpstreamT = pAdsl2Info->xmt2Info.T;
         obj->X_BROADCOM_COM_ADSL2_DownstreamR = pAdsl2Info->rcv2Info.R;
         obj->X_BROADCOM_COM_ADSL2_UpstreamR = pAdsl2Info->xmt2Info.R;

         snprintf(value, BUFLEN_32, "%d.%04d",
                  (SINT32)f2DecI(GetAdsl2Sq(&pAdsl2Info->rcv2Info,10000),10000),
                  (SINT32)f2DecF(GetAdsl2Sq(&pAdsl2Info->rcv2Info,10000),10000));
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_DownstreamS,value,mdmLibCtx.allocFlags);
         snprintf(value, BUFLEN_32, "%d.%04d",
                  (SINT32)f2DecI(GetAdsl2Sq(&pAdsl2Info->xmt2Info,10000),10000),
                  (SINT32)f2DecF(GetAdsl2Sq(&pAdsl2Info->xmt2Info,10000),10000));
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_UpstreamS,value,mdmLibCtx.allocFlags);

         obj->X_BROADCOM_COM_ADSL2_DownstreamL = pAdsl2Info->rcv2Info.L;
         obj->X_BROADCOM_COM_ADSL2_UpstreamL = pAdsl2Info->xmt2Info.L;
         obj->X_BROADCOM_COM_DownstreamD = pAdsl2Info->rcv2Info.D;
         obj->X_BROADCOM_COM_UpstreamD = pAdsl2Info->xmt2Info.D;

         /* 2nd latency */
         obj->X_BROADCOM_COM_ADSL2_DownstreamMSGc_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_UpstreamMSGc_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_DownstreamB_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_UpstreamB_2= 0;
         obj->X_BROADCOM_COM_ADSL2_DownstreamM_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_UpstreamM_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_DownstreamT_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_UpstreamT_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_DownstreamR_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_UpstreamR_2 = 0;

         snprintf(value, BUFLEN_32, "%d.%d", 0, 0);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_DownstreamS_2, value,mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_UpstreamS_2, value,mdmLibCtx.allocFlags);

         obj->X_BROADCOM_COM_ADSL2_DownstreamL_2 = 0;
         obj->X_BROADCOM_COM_ADSL2_UpstreamL_2 = 0;
         obj->X_BROADCOM_COM_DownstreamD_2 = 0;
         obj->X_BROADCOM_COM_UpstreamD_2 = 0;

         snprintf(value, sizeof(value), "%d.%d", 0, 0);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamDelay_2, value, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamDelay_2, value, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamINP_2, value, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamINP_2, value, mdmLibCtx.allocFlags);
#else
         /* Delay and INP for path 1 */
         pRxFramingParam = &adslMib.xdslInfo.dirInfo[0].lpInfo[1];
         pTxFramingParam = &adslMib.xdslInfo.dirInfo[1].lpInfo[1];
         snprintf(value, sizeof(value), "%d", pRxFramingParam->delay);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamDelay_2, value, mdmLibCtx.allocFlags);
         snprintf(value, sizeof(value), "%d", pTxFramingParam->delay);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamDelay_2, value, mdmLibCtx.allocFlags);
         
         snprintf(value, sizeof(value), "%4.2f", (float)pRxFramingParam->INP/2);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamINP_2, value, mdmLibCtx.allocFlags);
         snprintf(value, sizeof(value), "%4.2f", (float)pTxFramingParam->INP/2);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamINP_2, value, mdmLibCtx.allocFlags);

         if ( (kVdslModVdsl2 != adslMib.adslConnection.modType)
#ifdef SUPPORT_DSL_GFAST
               && (kXdslModGfast != adslMib.adslConnection.modType)
#endif
            ) {
            /* ADSL2/ADSL2+ Mode */
            obj->X_BROADCOM_COM_ADSL2_DownstreamMSGc = pAdsl2Info->rcv2Info.MSGc;
            obj->X_BROADCOM_COM_ADSL2_UpstreamMSGc = pAdsl2Info->xmt2Info.MSGc;
            obj->X_BROADCOM_COM_ADSL2_DownstreamB = pAdsl2Info->rcv2Info.B;
            obj->X_BROADCOM_COM_ADSL2_UpstreamB = pAdsl2Info->xmt2Info.B;
            obj->X_BROADCOM_COM_ADSL2_DownstreamM = pAdsl2Info->rcv2Info.M;
            obj->X_BROADCOM_COM_ADSL2_UpstreamM = pAdsl2Info->xmt2Info.M;
            obj->X_BROADCOM_COM_ADSL2_DownstreamT = pAdsl2Info->rcv2Info.T;
            obj->X_BROADCOM_COM_ADSL2_UpstreamT = pAdsl2Info->xmt2Info.T;
            obj->X_BROADCOM_COM_ADSL2_DownstreamR = pAdsl2Info->rcv2Info.R;
            obj->X_BROADCOM_COM_ADSL2_UpstreamR = pAdsl2Info->xmt2Info.R;
            
            snprintf(value, BUFLEN_32, "%d.%04d",
                     (SINT32)f2DecI(GetAdsl2Sq(&pAdsl2Info->rcv2Info,10000),10000),
                     (SINT32)f2DecF(GetAdsl2Sq(&pAdsl2Info->rcv2Info,10000),10000));
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_DownstreamS,value,mdmLibCtx.allocFlags);
            snprintf(value, BUFLEN_32, "%d.%04d",
                     (SINT32)f2DecI(GetAdsl2Sq(&pAdsl2Info->xmt2Info,10000),10000),
                     (SINT32)f2DecF(GetAdsl2Sq(&pAdsl2Info->xmt2Info,10000),10000));
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_UpstreamS,value,mdmLibCtx.allocFlags);
            
            obj->X_BROADCOM_COM_ADSL2_DownstreamL = pAdsl2Info->rcv2Info.L;
            obj->X_BROADCOM_COM_ADSL2_UpstreamL = pAdsl2Info->xmt2Info.L;
            obj->X_BROADCOM_COM_DownstreamD = pAdsl2Info->rcv2Info.D;
            obj->X_BROADCOM_COM_UpstreamD = pAdsl2Info->xmt2Info.D;
            
            /* 2nd latency */
            obj->X_BROADCOM_COM_ADSL2_DownstreamMSGc_2 = pAdsl2Info2->rcv2Info.MSGc;
            obj->X_BROADCOM_COM_ADSL2_UpstreamMSGc_2 = pAdsl2Info2->xmt2Info.MSGc;
            obj->X_BROADCOM_COM_ADSL2_DownstreamB_2 = pAdsl2Info2->rcv2Info.B;
            obj->X_BROADCOM_COM_ADSL2_UpstreamB_2 = pAdsl2Info2->xmt2Info.B;
            obj->X_BROADCOM_COM_ADSL2_DownstreamM_2 = pAdsl2Info2->rcv2Info.M;
            obj->X_BROADCOM_COM_ADSL2_UpstreamM_2 = pAdsl2Info2->xmt2Info.M;
            obj->X_BROADCOM_COM_ADSL2_DownstreamT_2 = pAdsl2Info2->rcv2Info.T;
            obj->X_BROADCOM_COM_ADSL2_UpstreamT_2 = pAdsl2Info2->xmt2Info.T;
            obj->X_BROADCOM_COM_ADSL2_DownstreamR_2 = pAdsl2Info2->rcv2Info.R;
            obj->X_BROADCOM_COM_ADSL2_UpstreamR_2 = pAdsl2Info2->xmt2Info.R;
            
            snprintf(value, BUFLEN_32, "%d.%04d",
                     (SINT32)f2DecI(GetAdsl2Sq(&pAdsl2Info2->rcv2Info,10000),10000),
                     (SINT32)f2DecF(GetAdsl2Sq(&pAdsl2Info2->rcv2Info,10000),10000));
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_DownstreamS_2,value,mdmLibCtx.allocFlags);
            snprintf(value, BUFLEN_32, "%d.%04d",
                     (SINT32)f2DecI(GetAdsl2Sq(&pAdsl2Info2->xmt2Info,10000),10000),
                     (SINT32)f2DecF(GetAdsl2Sq(&pAdsl2Info2->xmt2Info,10000),10000));
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_UpstreamS_2,value,mdmLibCtx.allocFlags);
            
            obj->X_BROADCOM_COM_ADSL2_DownstreamL_2 = pAdsl2Info2->rcv2Info.L;
            obj->X_BROADCOM_COM_ADSL2_UpstreamL_2 = pAdsl2Info2->xmt2Info.L;
            obj->X_BROADCOM_COM_DownstreamD_2 = pAdsl2Info2->rcv2Info.D;
            obj->X_BROADCOM_COM_UpstreamD_2 = pAdsl2Info2->xmt2Info.D;
         }
         else {
            /* VDSL2 Mode */
            pRxFramingParam = &adslMib.xdslInfo.dirInfo[0].lpInfo[0];
            pTxFramingParam = &adslMib.xdslInfo.dirInfo[1].lpInfo[0];
            obj->X_BROADCOM_COM_ADSL2_DownstreamB = pVdsl2Info->rcv2Info.B[0];
            obj->X_BROADCOM_COM_ADSL2_UpstreamB = pVdsl2Info->xmt2Info.B[0];
            obj->X_BROADCOM_COM_ADSL2_DownstreamM = pVdsl2Info->rcv2Info.M;
            obj->X_BROADCOM_COM_ADSL2_UpstreamM = pVdsl2Info->xmt2Info.M;
            obj->X_BROADCOM_COM_ADSL2_DownstreamT = pVdsl2Info->rcv2Info.T;
            obj->X_BROADCOM_COM_ADSL2_UpstreamT = pVdsl2Info->xmt2Info.T;
            obj->X_BROADCOM_COM_ADSL2_DownstreamR = pVdsl2Info->rcv2Info.R;
            obj->X_BROADCOM_COM_ADSL2_UpstreamR = pVdsl2Info->xmt2Info.R;
            
            snprintf(value, sizeof(value), "%3.4f", (pRxFramingParam->S.denom) ? (float)pRxFramingParam->S.num/(float)pRxFramingParam->S.denom : 0);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_DownstreamS,value,mdmLibCtx.allocFlags);
            snprintf(value, sizeof(value), "%3.4f", (pTxFramingParam->S.denom) ? (float)pRxFramingParam->S.num/(float)pRxFramingParam->S.denom : 0);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_UpstreamS,value,mdmLibCtx.allocFlags);
            
            obj->X_BROADCOM_COM_ADSL2_DownstreamL = pRxFramingParam->L;
            obj->X_BROADCOM_COM_ADSL2_UpstreamL = pTxFramingParam->L;
            obj->X_BROADCOM_COM_DownstreamD = pVdsl2Info->rcv2Info.D;
            obj->X_BROADCOM_COM_UpstreamD = pVdsl2Info->xmt2Info.D;
            obj->X_BROADCOM_COM_VDSL_DownstreamI = pVdsl2Info->rcv2Info.I;
            obj->X_BROADCOM_COM_VDSL_UpstreamI = pVdsl2Info->xmt2Info.I;
            obj->X_BROADCOM_COM_VDSL_DownstreamN = pVdsl2Info->rcv2Info.N;
            obj->X_BROADCOM_COM_VDSL_UpstreamN = pVdsl2Info->xmt2Info.N;
            
            /* 2nd latency */
            pRxFramingParam = &adslMib.xdslInfo.dirInfo[0].lpInfo[0];
            pTxFramingParam = &adslMib.xdslInfo.dirInfo[1].lpInfo[0];
            obj->X_BROADCOM_COM_ADSL2_DownstreamB_2 = pVdsl2Info2->rcv2Info.B[0];
            obj->X_BROADCOM_COM_ADSL2_UpstreamB_2 = pVdsl2Info2->xmt2Info.B[0];
            obj->X_BROADCOM_COM_ADSL2_DownstreamM_2 = pVdsl2Info2->rcv2Info.M;
            obj->X_BROADCOM_COM_ADSL2_UpstreamM_2 = pVdsl2Info2->xmt2Info.M;
            obj->X_BROADCOM_COM_ADSL2_DownstreamT_2 = pVdsl2Info2->rcv2Info.T;
            obj->X_BROADCOM_COM_ADSL2_UpstreamT_2 = pVdsl2Info2->xmt2Info.T;
            obj->X_BROADCOM_COM_ADSL2_DownstreamR_2 = pVdsl2Info2->rcv2Info.R;
            obj->X_BROADCOM_COM_ADSL2_UpstreamR_2 = pVdsl2Info2->xmt2Info.R;
            snprintf(value, sizeof(value), "%3.4f", (pRxFramingParam->S.denom) ? (float)pRxFramingParam->S.num/(float)pRxFramingParam->S.denom : 0);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_DownstreamS_2,value,mdmLibCtx.allocFlags);
            snprintf(value, sizeof(value), "%3.4f", (pTxFramingParam->S.denom) ? (float)pRxFramingParam->S.num/(float)pRxFramingParam->S.denom : 0);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_ADSL2_UpstreamS_2,value,mdmLibCtx.allocFlags);

            obj->X_BROADCOM_COM_ADSL2_DownstreamL_2 = pRxFramingParam->L;
            obj->X_BROADCOM_COM_ADSL2_UpstreamL_2 = pTxFramingParam->L;
            obj->X_BROADCOM_COM_DownstreamD_2 = pVdsl2Info2->rcv2Info.D;
            obj->X_BROADCOM_COM_UpstreamD_2 = pVdsl2Info2->xmt2Info.D;
            obj->X_BROADCOM_COM_VDSL_DownstreamI_2 = pVdsl2Info2->rcv2Info.I;
            obj->X_BROADCOM_COM_VDSL_UpstreamI_2 = pVdsl2Info2->xmt2Info.I;
            obj->X_BROADCOM_COM_VDSL_DownstreamN_2 = pVdsl2Info2->rcv2Info.N;
            obj->X_BROADCOM_COM_VDSL_UpstreamN_2 = pVdsl2Info2->xmt2Info.N;
            
            /* current profile */
            cmsLog_debug("vdsl2Profile=0x%x", pVdsl2Info->vdsl2Profile);
            switch (pVdsl2Info->vdsl2Profile)
            {
            case kVdslProfile8a:
               strcpy(value,"8a");
               break;
            case kVdslProfile8b:
               strcpy(value,"8b");
               break;
            case kVdslProfile8c:
               strcpy(value,"8c");
               break;
            case kVdslProfile8d:
               strcpy(value,"8d");
               break;
            case kVdslProfile12a:
               strcpy(value,"12a");
               break;
            case kVdslProfile12b:
               strcpy(value,"12b");
               break;
            case kVdslProfile17a:
               strcpy(value,"17a");
               break;
            case kVdslProfile30a:
               strcpy(value,"30a");
               break;
#ifdef CONFIG_VDSLBRCMPRIV1_SUPPORT
            case kVdslProfileBrcmPriv1:
               strcpy(value,"BrcmPriv1");
               break;
#endif
#ifdef CONFIG_VDSLBRCMPRIV2_SUPPORT
            case kVdslProfileBrcmPriv2:
               strcpy(value,"BrcmPriv2");
               break;
#endif
            default:
               /* this parameter is irrelevant for gfast */
               if (!gfast)
               {
                  cmsLog_error("unrecognized profile 0x%x (set to 8a)", pVdsl2Info->vdsl2Profile);
               }
               strcpy(value,"8a");
               break;
            } /* switch vdsl2Profile */

            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->currentProfile, value, mdmLibCtx.allocFlags);

            xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber,oidStr,sizeof(oidStr),(char*)&adslCfg, &dataLen);
            value[0] = '\0';
            if (adslCfg.vdslParam & kVdslProfile8a)
            {
               strcat(value,"8a,");
            }
            if (adslCfg.vdslParam & kVdslProfile8b)
            {
               strcat(value,"8b,");
            }
            if (adslCfg.vdslParam & kVdslProfile8c)
            {
               strcat(value,"8c,");
            }
            if (adslCfg.vdslParam & kVdslProfile8d)
            {
               strcat(value,"8d,");
            }
            if (adslCfg.vdslParam & kVdslProfile12a)
            {
               strcat(value,"12a,");
            }
            if (adslCfg.vdslParam & kVdslProfile12b)
            {
               strcat(value,"12b,");
            }
            if (adslCfg.vdslParam & kVdslProfile17a)
            {
               strcat(value,"17a,");
            }
            if (adslCfg.vdslParam & kVdslProfile30a)
            {
               strcat(value,"30a,");
            }
#ifdef CONFIG_VDSLBRCMPRIV1_SUPPORT
            if (adslCfg.vdslParam & kVdslProfileBrcmPriv1)
            {
               strcat(value,"BrcmPriv1,");
            }
#endif
#ifdef CONFIG_VDSLBRCMPRIV2_SUPPORT
            if (adslCfg.vdslParam & kVdslProfileBrcmPriv2)
            {
               strcat(value,"BrcmPriv2,");
            }
#endif
            /* wipe out the last , */
            len = strlen(value);
            value[len-1] = '\0';
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->allowedProfiles, value, mdmLibCtx.allocFlags);

            /* The following code are based on adslctl code to gather data from driver */
            obj->ACTSNRMODEds = adslMib.xdslAtucPhys.SNRmode;
            obj->ACTSNRMODEus = adslMib.xdslPhys.SNRmode;
            obj->ACTUALCE = adslMib.xdslPhys.actualCE;
            
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

            /* these are per band data */
            /* first get dsNegBandPlanDiscPresentation */
            dataLen=sizeof(bandPlanDescriptor32);
            xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber, oidStr1, sizeof(oidStr1), (char *)&dsNegBandPlanDiscPresentation, &dataLen);

            /* get usNegBandPlanDiscPresentation*/
            oidStr1[2]=kOidAdslPrivBandPlanUSNegDiscoveryPresentation;
            xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber, oidStr1, sizeof(oidStr1), (char *)&usNegBandPlanDiscPresentation, &dataLen);

            dataLen = 5*sizeof(short);
            oidStr2[1]=kOidAdslPrivSNRMusperband;
            xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber, oidStr2, sizeof(oidStr2), (char *)&data[0], &dataLen);
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
            xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber, oidStr2, sizeof(oidStr2), (char *)&data[0], &dataLen);
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
         }
#endif /* !defined(DMP_VDSL2WAN_1) && !defined(DMP_X_BROADCOM_COM_VDSL2WAN_1) */
      } /* xDSLl2 */
      else 
      {
         int Nds, Nus, Lds, Lus, Rds;
         float Sds, INPds, INPus;         
         cmsLog_debug("Not xDSLl2");
         if ((!((0 == adslMib.adslConnection.rcvInfo.R) && (0 != adslMib.adslConnection.rcvInfo.D))) &&
                (255 >= adslMib.adslConnection.rcvInfo.K) && (adslMib.adslConnection.rcvInfo.S!=0))
         {
             Nds = (adslMib.adslConnection.rcvInfo.K*adslMib.adslConnection.rcvInfo.S+adslMib.adslConnection.rcvInfo.R)/adslMib.adslConnection.rcvInfo.S;
             Lds = (Nds<<3)*adslMib.adslConnection.rcvInfo.S;
             if (0 != Lds)
                 INPds = (float)(adslMib.adslConnection.rcvInfo.R*adslMib.adslConnection.rcvInfo.D<<2)/(float)Lds;
             else
                 INPds = -1;
             Sds = (float)(adslMib.adslConnection.rcvInfo.S);
             Rds = adslMib.adslConnection.rcvInfo.R;
         }
         else
         {
             /* detect S=0.5 */
             if (255 > adslMib.adslConnection.rcvInfo.K)
                 Nds = adslMib.adslConnection.rcvInfo.K+(adslMib.adslConnection.rcvInfo.R<<1);
             else
                 Nds = (adslMib.adslConnection.rcvInfo.K+adslMib.adslConnection.rcvInfo.R)>>1;
             Lds = Nds<<4;
             if (0 != Lds)
                 INPds = (float)(adslMib.adslConnection.rcvInfo.R*adslMib.adslConnection.rcvInfo.D<<1)/(float)Lds;
             else
                 INPds = -1.0;
             Sds = 0.5;
             Rds = adslMib.adslConnection.rcvInfo.R>>1;
         }
         Nus = adslMib.adslConnection.xmtInfo.K+adslMib.adslConnection.xmtInfo.R;
         Lus = (Nus<<3)*adslMib.adslConnection.xmtInfo.S;
         if (Lus!=0)
             INPus = (float)(adslMib.adslConnection.xmtInfo.R*adslMib.adslConnection.xmtInfo.D<<2)/(float)Lus;
         else
             INPus = -1.0;
         
         obj->X_BROADCOM_COM_DownstreamK = adslMib.adslConnection.rcvInfo.K;
         obj->X_BROADCOM_COM_UpstreamK = adslMib.adslConnection.xmtInfo.K;
         
         obj->X_BROADCOM_COM_DownstreamR = Rds;
         obj->X_BROADCOM_COM_UpstreamR = adslMib.adslConnection.xmtInfo.R;
        
         snprintf(value,BUFLEN_32,"%4.2f", Sds);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamS,value,mdmLibCtx.allocFlags);
         snprintf(value, BUFLEN_32,"%4.2f",(float)adslMib.adslConnection.xmtInfo.S);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamS,value,mdmLibCtx.allocFlags);

         obj->X_BROADCOM_COM_DownstreamD = adslMib.adslConnection.rcvInfo.D;
         obj->X_BROADCOM_COM_UpstreamD = adslMib.adslConnection.xmtInfo.D;

         
         snprintf(value,BUFLEN_32,"%4.2f", (Sds* (float)adslMib.adslConnection.rcvInfo.D)/4.0);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamDelay,value,mdmLibCtx.allocFlags);

         snprintf(value,BUFLEN_32,"%4.2f",(float)((adslMib.adslConnection.xmtInfo.S* adslMib.adslConnection.xmtInfo.D))/4.0);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamDelay,value,mdmLibCtx.allocFlags);
         
         if (-1.0 != INPds)
             snprintf(value,BUFLEN_32, "%4.2f", INPds);
         else
             snprintf(value,BUFLEN_32, "n/a");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_DownstreamINP,value,mdmLibCtx.allocFlags);
         if (-1.0 != INPus)
             snprintf(value,BUFLEN_32, "%4.2f", INPus);
         else
             snprintf(value,BUFLEN_32, "n/a");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_UpstreamINP,value,mdmLibCtx.allocFlags);

      } /* !adsl2 */
      /* Values pending on PHY/driver */
      obj->XTURANSIStd = 0;
      obj->XTURANSIRev = 0;
      obj->XTUCANSIStd = 0;
      obj->XTUCANSIRev = 0;

#ifdef DMP_DEVICE2_VDSL2_1      
      obj->TRELLISds = adslMib.adslConnection.trellisCoding2 & kAdsl2TrellisRxEnabled;
      obj->TRELLISus = adslMib.adslConnection.trellisCoding2 & kAdsl2TrellisTxEnabled;
#endif

      memset((void*)&adslVer, 0, sizeof(adslVer));
      xdslCtl_GetVersion(0, &adslVer);
      switch (adslMib.adslConnection.modType)
      {
      case kAdslModGdmt:
         strcpy(value,"G.992.1"); 
         catPhyType = TRUE;
         break;
      case kAdslModT1413:
         strcpy(value,"T1.413");
         break;
      case kAdslModGlite:
         strcpy(value,"G.992.2");
         break;
      case kAdslModAnnexI:
         strcpy(value,"G.992.3_Annex_I"); 
         break;
      case kAdslModAdsl2:
         strcpy(value,"G.992.3"); 
         catPhyType = TRUE;
         break;
      case kAdslModAdsl2p:
         strcpy(value,"G.992.5");
         catPhyType = TRUE;
         break;
      case kAdslModReAdsl2:
         strcpy(value,"G.992.3_Annex_L");
         break;
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
      case kVdslModVdsl2:
         strcpy(value,"G.993.2");
         switch((adslMib.adsl2Info.adsl2Mode)>>  kXdslModeAnnexShift)
         {
         case kAdslTypeAnnexB:
            strcat(value,"_Annex_B");
            break;
         case kAdslTypeAnnexC:
            strcat(value,"_Annex_C");
            break;
         default:
            strcat(value,"_Annex_A");
            break;
         } /* xdsl */
         break;
#ifdef SUPPORT_DSL_GFAST
      case kXdslModGfast:
         strcpy(value,"G.9701");
         break;
#endif
#endif
      } /* modType */

      if (catPhyType == TRUE)
      {
         /* TR181 enumeration */
         if (adslVer.phyType == kAdslTypeAnnexA)
         {
            strcat(value,"_Annex_A");
         }
         else if (adslVer.phyType == kAdslTypeAnnexB)
         {
            strcat(value,"_Annex_B");
         }
         else
         {
            strcat(value,"_Annex_C");
         }
      }
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->standardUsed,value,mdmLibCtx.allocFlags);
   } /* if up */
   cmsLog_debug("End: ret %d",ret);
   
   return (ret);
} /* get dslLineInfo */

CmsRet rutdsl_getChannelInfo_dev2(Dev2DslChannelObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   ADSL_CONNECTION_INFO adslConnInfo;
   CmsRet ret = CMSRET_SUCCESS;
   UINT16 xdslStatus;
   int xDsl2Mode = 0;
   MdmPathDescriptor pathDesc;
   Dev2DslLineObject *dslLineObj;
   int bondingLineNumber=0;

   if (obj == NULL)
   {
      return ret;
   }

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));


   memset(&adslConnInfo, 0, sizeof(ADSL_CONNECTION_INFO));

   /* first get the line object underneath this channel */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   cmsMdm_fullPathToPathDescriptor(obj->lowerLayers, &pathDesc);
   if (cmsObj_get(MDMOID_DEV2_DSL_LINE,&pathDesc.iidStack,0,(void **)&dslLineObj) != CMSRET_SUCCESS)
   {
      cmsLog_error("Cannot get line object for this channel, ret=%d", ret);
      return ret;
   }
   bondingLineNumber = dslLineObj->X_BROADCOM_COM_BondingLineNumber;
   cmsObj_free((void **) &dslLineObj);

   ret = xdslCtl_GetConnectionInfo(bondingLineNumber, &adslConnInfo);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("xdslCtl_GetConnectionInfo failed, ret=%d", ret);
      return ret;
   }

   xdslStatus = adslConnInfo.LinkState;
   cmsLog_debug("adslConnInfo.LinkState=%d", xdslStatus);
   
   if (xdslStatus != BCM_ADSL_LINK_UP)
   {
      cmsLog_debug("Down, return unchanged");
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   long adslMibSize=sizeof(adslMib);
   
   cmsLog_debug("BCM_XDSL_LINK_UP (0)");

   if (CMSRET_SUCCESS != xdslCtl_GetObjectValue(bondingLineNumber, NULL, 0, (char *) &adslMib, &adslMibSize))
   {
      cmsLog_error("could not get MIB for line %d", bondingLineNumber);
      return CMSRET_INTERNAL_ERROR;
   }
   if(adslMib.adslTrainingState != kAdslTrainingConnected) 
   {
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   obj->downstreamCurrRate = GetRcvRate(&adslMib, 0);
   obj->upstreamCurrRate = GetXmtRate(&adslMib, 0);
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
   if(xDsl2Mode) {
      obj->X_BROADCOM_COM_DownstreamCurrRate_2 = GetRcvRate(&adslMib, 1);
      obj->X_BROADCOM_COM_UpstreamCurrRate_2 = GetXmtRate(&adslMib, 1);
   }
   else
#endif
   {
      obj->X_BROADCOM_COM_DownstreamCurrRate_2 = 0;
      obj->X_BROADCOM_COM_UpstreamCurrRate_2 = 0;
   }
   return ret;
} /* get dslChannelInfo */

/* channelTotalStats */
CmsRet rutdsl_getTotalChannelStats_dev2(Dev2DslChannelStatsTotalObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByChannelIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {  
      obj->X_BROADCOM_COM_ReceiveBlocks = adslMib.xdslChanPerfData[0].perfTotal.adslChanReceivedBlks;
      obj->X_BROADCOM_COM_TransmitBlocks = adslMib.xdslChanPerfData[0].perfTotal.adslChanTransmittedBlks;
      obj->XTUCFECErrors = adslMib.adslStatSincePowerOn.xmtStat.cntRSCor;
      obj->XTURFECErrors = adslMib.adslPerfData.perfTotal.adslFECs;
      obj->X_BROADCOM_COM_RxRsUncorrectable = adslMib.xdslStat[0].rcvStat.cntRSUncor;
      obj->X_BROADCOM_COM_TxRsUncorrectable = 0;
      obj->X_BROADCOM_COM_RxRsCorrectable = adslMib.xdslStat[0].rcvStat.cntRSCor;;
      obj->X_BROADCOM_COM_TxRsCorrectable = adslMib.xdslStat[0].xmtStat.cntRSCor;
      obj->X_BROADCOM_COM_RxRsWords = adslMib.xdslStat[0].rcvStat.cntRS;
      obj->X_BROADCOM_COM_TxRsWords = adslMib.xdslStat[0].xmtStat.cntRS;
      obj->XTURHECErrors = adslMib.atmStatSincePowerOn.rcvStat.cntHEC;
      obj->XTUCHECErrors = adslMib.atmStatSincePowerOn.xmtStat.cntHEC;
      obj->XTURCRCErrors = adslMib.adslStatSincePowerOn.rcvStat.cntSFErr;
      obj->XTUCCRCErrors = adslMib.adslStatSincePowerOn.xmtStat.cntSFErr;
      obj->X_BROADCOM_COM_DownstreamOCD = adslMib.atmStat2lp[0].rcvStat.cntOCD;
      obj->X_BROADCOM_COM_UpstreamOCD = adslMib.atmStat2lp[0].xmtStat.cntOCD;
      obj->X_BROADCOM_COM_UpstreamLCD = adslMib.atmStat2lp[0].xmtStat.cntLCD;
      obj->X_BROADCOM_COM_DownstreamLCD = adslMib.atmStat2lp[0].rcvStat.cntLCD;
      obj->X_BROADCOM_COM_DownstreamTotalCells = adslMib.atmStat2lp[0].rcvStat.cntCellTotal;
      obj->X_BROADCOM_COM_UpstreamTotalCells = adslMib.atmStat2lp[0].xmtStat.cntCellTotal;
      obj->X_BROADCOM_COM_UpstreamDataCells = adslMib.atmStat2lp[0].xmtStat.cntCellData;
      obj->X_BROADCOM_COM_DownstreamDataCells = adslMib.atmStat2lp[0].rcvStat.cntCellData;
      obj->X_BROADCOM_COM_UpstreamBitErrors = adslMib.atmStat2lp[0].xmtStat.cntBitErrs;
      obj->X_BROADCOM_COM_DownstreamBitErrors = adslMib.atmStat2lp[0].rcvStat.cntBitErrs;
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
      obj->X_BROADCOM_COM_ReceiveBlocks_2 = adslMib.xdslStat[1].rcvStat.cntSF;
      obj->X_BROADCOM_COM_TransmitBlocks_2 = adslMib.xdslStat[1].xmtStat.cntSF;
      obj->X_BROADCOM_COM_RxRsUncorrectable_2 = adslMib.xdslStat[1].rcvStat.cntRSUncor;
      obj->X_BROADCOM_COM_TxRsUncorrectable_2 = 0;
      obj->X_BROADCOM_COM_RxRsCorrectable_2 = adslMib.xdslStat[1].rcvStat.cntRSCor;;
      obj->X_BROADCOM_COM_TxRsCorrectable_2 = adslMib.xdslStat[1].xmtStat.cntRSCor;
      obj->X_BROADCOM_COM_RxRsWords_2 = adslMib.xdslStat[1].rcvStat.cntRS;
      obj->X_BROADCOM_COM_TxRsWords_2 = adslMib.xdslStat[1].xmtStat.cntRS;
      obj->X_BROADCOM_COM_XTURHECErrors_2 = adslMib.atmStat2lp[1].xmtStat.cntHEC;
      obj->X_BROADCOM_COM_XTUCHECErrors_2 = adslMib.atmStat2lp[1].rcvStat.cntHEC;
      obj->X_BROADCOM_COM_DownstreamOCD_2 = adslMib.atmStat2lp[1].rcvStat.cntOCD;
      obj->X_BROADCOM_COM_UpstreamOCD_2 = adslMib.atmStat2lp[1].xmtStat.cntOCD;
      obj->X_BROADCOM_COM_UpstreamLCD_2 = adslMib.atmStat2lp[1].xmtStat.cntLCD;
      obj->X_BROADCOM_COM_DownstreamLCD_2 = adslMib.atmStat2lp[1].rcvStat.cntLCD;
      obj->X_BROADCOM_COM_DownstreamTotalCells_2 = adslMib.atmStat2lp[1].rcvStat.cntCellTotal;
      obj->X_BROADCOM_COM_UpstreamTotalCells_2 = adslMib.atmStat2lp[1].xmtStat.cntCellTotal;
      obj->X_BROADCOM_COM_UpstreamDataCells_2 = adslMib.atmStat2lp[1].xmtStat.cntCellData;
      obj->X_BROADCOM_COM_DownstreamDataCells_2 = adslMib.atmStat2lp[1].rcvStat.cntCellData;
      obj->X_BROADCOM_COM_UpstreamBitErrors_2 = adslMib.atmStat2lp[1].xmtStat.cntBitErrs;
      obj->X_BROADCOM_COM_DownstreamBitErrors_2 = adslMib.atmStat2lp[1].rcvStat.cntBitErrs;
#else
      obj->X_BROADCOM_COM_ReceiveBlocks_2 = 0;
      obj->X_BROADCOM_COM_TransmitBlocks_2 = 0;
      obj->X_BROADCOM_COM_RxRsUncorrectable_2 = 0;
      obj->X_BROADCOM_COM_TxRsUncorrectable_2 = 0;
      obj->X_BROADCOM_COM_RxRsCorrectable_2 = 0;
      obj->X_BROADCOM_COM_TxRsCorrectable_2 = 0;
      obj->X_BROADCOM_COM_RxRsWords_2 = 0;
      obj->X_BROADCOM_COM_TxRsWords_2 = 0;
      obj->X_BROADCOM_COM_XTURHECErrors_2 = 0;
      obj->X_BROADCOM_COM_XTUCHECErrors_2 = 0;
      obj->X_BROADCOM_COM_DownstreamOCD_2 = 0;
      obj->X_BROADCOM_COM_UpstreamOCD_2 = 0;
      obj->X_BROADCOM_COM_UpstreamLCD_2 = 0;
      obj->X_BROADCOM_COM_DownstreamLCD_2 = 0;
      obj->X_BROADCOM_COM_DownstreamTotalCells_2 = 0;
      obj->X_BROADCOM_COM_UpstreamTotalCells_2 = 0;
      obj->X_BROADCOM_COM_UpstreamDataCells_2 = 0;
      obj->X_BROADCOM_COM_DownstreamDataCells_2 = 0;
      obj->X_BROADCOM_COM_UpstreamBitErrors_2 = 0;
      obj->X_BROADCOM_COM_DownstreamBitErrors_2 = 0;
#endif  /* defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1) */

#ifdef YEN_REMOVE
      /* these are in line stats total */
      obj->erroredSecs = adslMib.adslPerfData.perfTotal.adslESs;
      obj->X_BROADCOM_COM_UpstreamEs = adslMib.adslTxPerfTotal.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfTotal.adslSES;
      obj->X_BROADCOM_COM_UpstreamSes = adslMib.adslTxPerfTotal.adslSES;
      obj->X_BROADCOM_COM_UpstreamUas = adslMib.adslTxPerfTotal.adslUAS;
      obj->X_BROADCOM_COM_DownstreamUas = adslMib.adslPerfData.perfTotal.adslUAS;
#endif
   }
   else
   {
      cmsLog_error("Cannot get adslMib");
   }
   
   return (CMSRET_SUCCESS);
} /* rutdsl_getTotalChannelStats */

/* rutdsl_getShowTimeChannelStats */
CmsRet rutdsl_getShowTimeChannelStats_dev2(Dev2DslChannelStatsShowtimeObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByChannelIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {  
      obj->XTUCFECErrors = adslMib.adslTxPerfSinceShowTime.adslFECs;
      obj->XTURFECErrors = adslMib.adslPerfData.perfSinceShowTime.adslFECs;
      obj->XTURHECErrors = adslMib.atmStat.rcvStat.cntHEC;
      obj->XTUCHECErrors = adslMib.atmStat.xmtStat.cntHEC;
      obj->XTURCRCErrors = adslMib.adslStat.rcvStat.cntSFErr;
      obj->XTUCCRCErrors = adslMib.adslStat.xmtStat.cntSFErr;
   }
   else
   {
      obj->XTUCFECErrors = 0;
      obj->XTURFECErrors = 0;
      obj->XTURHECErrors = 0;
      obj->XTUCHECErrors = 0;
      obj->XTURCRCErrors = 0;
      obj->XTUCCRCErrors = 0;
   }
   return (CMSRET_SUCCESS);
} /* rutdsl_getShowtimeChannelStats */

/* quarter hour stats */
CmsRet rutdsl_getQuarterHourChannelStats_dev2(Dev2DslChannelStatsQuarterHourObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByChannelIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {  
      obj->XTUCFECErrors = adslMib.adslTxPerfCur15Min.adslFECs;
      obj->XTURFECErrors = adslMib.adslPerfData.perfCurr15Min.adslFECs;
      obj->XTURHECErrors = 0;
      obj->XTUCHECErrors = 0;
      obj->XTURCRCErrors = adslMib.xdslChanPerfData[0].perfCurr15Min.adslChanUncorrectBlks;
      obj->XTUCCRCErrors = adslMib.xdslChanPerfData[0].perfCurr15Min.adslChanTxCRC;
   }
   else
   {
      obj->XTUCFECErrors = 0;
      obj->XTURFECErrors = 0;
      obj->XTURHECErrors = 0;
      obj->XTUCHECErrors = 0;
      obj->XTURCRCErrors = 0;
      obj->XTUCCRCErrors = 0;
   }
   return (CMSRET_SUCCESS);
} /* rutdsl_getQuarterHourChannelStats */

/* current day channel stats */
CmsRet rutdsl_getCurrentDayChannelStats_dev2(Dev2DslChannelStatsCurrentDayObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutdsl_getAdslMibByChannelIidStack_dev2(iidStack,&adslMib,NULL) == CMSRET_SUCCESS)
   {  
      obj->XTUCFECErrors = adslMib.adslTxPerfCur1Day.adslFECs;
      obj->XTURFECErrors = adslMib.adslPerfData.perfCurr1Day.adslFECs;
      obj->XTURHECErrors = 0;
      obj->XTUCHECErrors = 0;
      obj->XTURCRCErrors = adslMib.xdslChanPerfData[0].perfCurr1Day.adslChanUncorrectBlks;
      obj->XTUCCRCErrors = adslMib.xdslChanPerfData[0].perfCurr1Day.adslChanTxCRC;
   }
   else
   {
      obj->XTUCFECErrors = 0;
      obj->XTURFECErrors = 0;
      obj->XTURHECErrors = 0;
      obj->XTUCHECErrors = 0;
      obj->XTURCRCErrors = 0;
      obj->XTUCCRCErrors = 0;
   }
   return (CMSRET_SUCCESS);
} /* rutdsl_getCurrentDayChannelStats */

CmsRet rutDsl_initPPPoA_dev2(const InstanceIdStack * iidStack, void *obj)    
{
   SINT32 vpi=0;
   SINT32 vci=0;
   UBOOL8 isVCMux;
   SINT32 portId=0;   
   char atmEncap[BUFLEN_16]={0};   
   char staticIPAddrFlag[BUFLEN_32];
   char passwordFlag[BUFLEN_32];
   char cmdLine[BUFLEN_128];
   char l2IfName[CMS_IFNAME_LENGTH]={0};
   SINT32 pid=0;
   CmsRet ret=CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc;
   _Dev2AtmLinkObject *atmLinkObject = NULL;     // For Encapsulation ,DestinationAddress 
   _Dev2PppInterfaceObject *newObj = (_Dev2PppInterfaceObject *) obj;

   cmsLog_debug("Enter:");

   if((ret = cmsMdm_fullPathToPathDescriptor(newObj->lowerLayers,&pathDesc)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if(pathDesc.oid != MDMOID_DEV2_ATM_LINK) //For PPPoA, lowlayer OID must be MDMOID_DEV2_ATM_LINK
   {
      cmsLog_error("could not get atmLinkObject object!, lowlayer oid:%d", pathDesc.oid);   
      return CMSRET_INVALID_ARGUMENTS;   
   }

   if ((ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, 0, (void **) &atmLinkObject)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get atmLinkObject object!, ret=%d", ret);
      return ret;      
   }
   
   if ((ret = qdmIntf_getIntfnameFromFullPathLocked_dev2(newObj->lowerLayers, 
                                                         l2IfName, 
                                                         sizeof(l2IfName))) != CMSRET_SUCCESS)
   {
      cmsObj_free((void **) &atmLinkObject);
      cmsLog_error("qdmIntf_getIntfnameFromFullPathLocked_dev2 failed. ret %d", ret);
      return ret;
   }

   if ((ret = cmsUtl_atmVpiVciStrToNum_dev2(atmLinkObject->destinationAddress, &vpi, &vci)) != CMSRET_SUCCESS)
   {
      cmsObj_free((void **) &atmLinkObject);
      return ret;
   }

   if (!cmsUtl_strcmp(atmLinkObject->encapsulation, MDMVS_LLC))
   {
      isVCMux = 0;
   }
   else
   {
      isVCMux = 1;
   }

#ifdef DMP_DEVICE2_X_BROADCOM_COM_ATMLINK_1
   portId = atmLinkObject->X_BROADCOM_COM_ATMInterfaceId;
#endif

   cmsObj_free((void **) &atmLinkObject);

   if (!isVCMux)
   {
      /* VCMux is default for pppoa - no flag.  If non default, ie. LLC, add the flag "-l" */
      strncpy(atmEncap, "-l", sizeof(atmEncap)-1);      
   }

   cmsLog_debug("%d/%d/%d atmEncap=%s", portId, vpi, vci, atmEncap);
      
    /* original code in BcmPppoe_startPppd */
   passwordFlag[0] = staticIPAddrFlag[0] = '\0';

   /* password is an optional parameter */
   if (newObj->password && newObj->password[0] != '\0')
   {
      snprintf(passwordFlag, sizeof(passwordFlag), "-p %s", newObj->password);
   }
       
   /* static IP address */
   if (newObj->X_BROADCOM_COM_UseStaticIPAddress)
   {
      snprintf(staticIPAddrFlag, sizeof(staticIPAddrFlag), "-A %s", newObj->X_BROADCOM_COM_LocalIPAddress);
   }
   
   snprintf(cmdLine, sizeof(cmdLine), "%s -a %s.%d.%d.%d -u %s %s %s -f %d %s",
      newObj->name,
      l2IfName, portId, vpi, vci,
      newObj->username, passwordFlag,
      atmEncap, cmsUtl_pppAuthToNum(newObj->authenticationProtocol), staticIPAddrFlag); 

   /* enable ppp debugging if it is selected */
   if (newObj->X_BROADCOM_COM_Enable_Debug)
   {
      strncat(cmdLine, " -d", sizeof(cmdLine)-1);
   }

#ifdef TODO
   /* IP extension */
   if (newObj->X_BROADCOM_COM_IPExtension)
   {
      strncat(cmdLine, " -x", sizeof(cmdLine)-1);
   }
#endif

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   /* IPv6 */
   if (newObj->X_BROADCOM_COM_IPv6Enabled)
   {
      strncat(cmdLine, " -6", sizeof(cmdLine)-1);
   }
#endif

   ret = rutDsl_configPPPoA(cmdLine,newObj->name,&(pid));

   if (ret == CMSRET_SUCCESS)
   {
      newObj->X_BROADCOM_COM_Pid = pid;
   }

   return ret;
}


#define MAX_XDSL_DATA_LENGTH  61430   /* max length defined in the spec. */
#ifdef DMP_DEVICE2_DSLDIAGNOSTICS_1
CmsRet rutdsl_getAdslLoopDiagStatus_dev2(void *obj, const InstanceIdStack *iidStack __attribute__((unused)))
{
   adslMibInfo adslMib;
   long	size = sizeof(adslMib);
   Dev2DslDiagnosticsADSLLineTestObject *dslDiagObj = (Dev2DslDiagnosticsADSLLineTestObject*)obj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   int testCompleted = DIAG_DSL_LOOPBACK_ERROR;
   UINT32 lineId = 0;

   /* lineId is determined by obj->Interface which is fullpath DSL.Channel.{i}.
         And the DSL line is derived from DSL.Channel.{i} --> LowerLayers
    */
   qdmDsl_getLineIdFromChannelFullPathLocked_dev2(dslDiagObj->interface,&lineId);

   if (ret = xdslCtl_GetObjectValue(lineId, NULL, 0, (char *)&adslMib, &size) != CMSRET_SUCCESS)
   {
      cmsLog_debug("Unable to read diagnosticsState from driver");
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dslDiagObj->diagnosticsState,MDMVS_ERROR_INTERNAL,mdmLibCtx.allocFlags);
      return (ret);
   }
   else
   {
      /* reading for indication from driver that test is completed */
      testCompleted = adslMib.adslPhys.adslLDCompleted;
      
      cmsLog_debug(" testCompleted %d (2== completed)", testCompleted);
      
      if ((testCompleted == DIAG_DSL_LOOPBACK_ERROR) || (testCompleted == DIAG_DSL_LOOPBACK_ERROR_BUT_RETRY))
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dslDiagObj->diagnosticsState,MDMVS_ERROR_INTERNAL,mdmLibCtx.allocFlags);
         ret = CMSRET_SUCCESS;
      }
      else if (testCompleted == DIAG_DSL_LOOPBACK_SUCCESS)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dslDiagObj->diagnosticsState,MDMVS_COMPLETE,mdmLibCtx.allocFlags);
         ret = CMSRET_SUCCESS;
      }  /* get adslLdCompleted */
   } /* read from ADSL driver OK */
   return (ret);
}
#endif /* DMP_DEVICE2_DSLDIAGNOSTICS_1 */

#if defined(SUPPORT_DSL_BONDING)
CmsRet rutdsl_getBondingGroupStats(Dev2DslBondingGroupStatsObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   long adslMibSize;
   Dev2AtmLinkObject *pAtmLinkObj;
   Dev2PtmLinkObject *pPtmLinkObj;
   MdmPathDescriptor pathDesc;
   char *bondingGroupFullPath = NULL;
   CmsRet ret = CMSRET_SUCCESS; 
   InstanceIdStack xtmIidStack;
   UBOOL8 found = FALSE;
   UINT64 dontCare;
   UINT64 byteTx, byteRx, byteTxTotal=0, byteRxTotal=0;
   UINT64 packetTx, packetRx, packetTxTotal=0, packetRxTotal=0;
   UINT32 errorsTx, errorsRx, errorsTxTotal=0, errorsRxTotal=0;
   UINT64 packetUniTx, packetUniRx, packetUniTxTotal=0, packetUniRxTotal=0;
   UINT64 packetMultiTx, packetMultiRx, packetMultiTxTotal=0, packetMultiRxTotal=0;
   UINT64 packetBcastTx, packetBcastRx, packetBcastTxTotal=0, packetBcastRxTotal=0;
   UINT32 packetDiscardTx, packetDiscardRx, packetDiscardTxTotal=0, packetDiscardRxTotal=0;
   
   cmsLog_debug("entered");

   adslMibSize=sizeof(adslMib);

   /* line 0 */
   if (CMSRET_SUCCESS != xdslCtl_GetObjectValue(0, NULL, 0, (char *) &adslMib, &adslMibSize))
   {
      cmsLog_error("could not get MIB for line 0");
      return CMSRET_INTERNAL_ERROR;
   }

   obj->totalStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
   obj->currentDayStart = adslMib.adslPerfData.adslPerfCurr1DayTimeElapsed;
   obj->quarterHourStart = adslMib.adslPerfData.adslPerfCurr15MinTimeElapsed;

   /* aggregated counters retrieved from interface stats for all lowerlayers channels (ptmx,atmx) */
   /* get full pathname of this bonding group */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_DSL_BONDING_GROUP;
   pathDesc.iidStack = *iidStack;
   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &bondingGroupFullPath)) == CMSRET_SUCCESS)
   {
      /* look for all ptm and atm interfaces with bondingGroupFullPath name, add up the stats */
      INIT_INSTANCE_ID_STACK(&xtmIidStack);
      while ((ret = cmsObj_getNext(MDMOID_DEV2_ATM_LINK, &xtmIidStack, (void **)&pAtmLinkObj)) == CMSRET_SUCCESS)
      {
         if ((pAtmLinkObj->enable == TRUE) &&
             (cmsUtl_strcmp(pAtmLinkObj->lowerLayers,bondingGroupFullPath) == 0))
         {
            found = TRUE;
            rut_getIntfStats_uint64(pAtmLinkObj->name,
                                    &byteRx,&packetRx,
                                    &dontCare/*byteMultiRx*/,&packetMultiRx,
                                    &packetUniRx,&packetBcastRx,
                                    &errorsRx,&packetDiscardRx,
                                    &byteTx,&packetTx,
                                    &dontCare/*byteMultiTx*/,&packetMultiTx,
                                    &packetUniTx,&packetBcastTx,
                                    &errorsTx,&packetDiscardTx);
            byteTxTotal += byteTx;
            byteRxTotal += byteRx;
            packetTxTotal += packetTx;
            packetRxTotal += packetRx;
            errorsTxTotal += (UINT32)errorsTx;
            errorsRxTotal += (UINT32)errorsRx;
            packetUniTxTotal += packetUniTx;
            packetUniRxTotal += packetUniRx;
            packetDiscardTxTotal += (UINT32)packetDiscardTx;
            packetDiscardRxTotal += (UINT32)packetDiscardRx;
            packetMultiTxTotal += packetMultiTx;
            packetMultiRxTotal += packetMultiRx;
            packetBcastTxTotal += packetBcastTx;
            packetBcastRxTotal += packetBcastRx;
         }
         cmsObj_free((void **) &pAtmLinkObj);
      } /* while ATM links */
      if (!found)
      {
         INIT_INSTANCE_ID_STACK(&xtmIidStack);
         while ((ret = cmsObj_getNext(MDMOID_DEV2_PTM_LINK, &xtmIidStack, (void **)&pPtmLinkObj)) == CMSRET_SUCCESS)
         {
            if ((pPtmLinkObj->enable == TRUE) &&
                (cmsUtl_strcmp(pPtmLinkObj->lowerLayers,bondingGroupFullPath) == 0))
            {
               found = TRUE;
               rut_getIntfStats_uint64(pPtmLinkObj->name,
                                       &byteRx,&packetRx,
                                       &dontCare/*byteMultiRx*/,&packetMultiRx,
                                       &packetUniRx,&packetBcastRx,
                                       &errorsRx,&packetDiscardRx,
                                       &byteTx,&packetTx,
                                       &dontCare/*byteMultiTx*/,&packetMultiTx,
                                       &packetUniTx,&packetBcastTx,
                                       &errorsTx,&packetDiscardTx);
               byteTxTotal += byteTx;
               byteRxTotal += byteRx;
               packetTxTotal += packetTx;
               packetRxTotal += packetRx;
               errorsTxTotal += (UINT32)errorsTx;
               errorsRxTotal += (UINT32)errorsRx;
               packetUniTxTotal += packetUniTx;
               packetUniRxTotal += packetUniRx;
               packetDiscardTxTotal += (UINT32)packetDiscardTx;
               packetDiscardRxTotal += (UINT32)packetDiscardRx;
               packetMultiTxTotal += packetMultiTx;
               packetMultiRxTotal += packetMultiRx;
               packetBcastTxTotal += packetBcastTx;
               packetBcastRxTotal += packetBcastRx;
            }
            cmsObj_free((void **) &pPtmLinkObj);
         } /* while PTM links */
      } /* try ptm link */
      CMSMEM_FREE_BUF_AND_NULL_PTR(bondingGroupFullPath);

      /* if ptm or atm is found */
      if (found)
      {
         /* store aggregated values */
         obj->bytesSent = byteTxTotal;
         obj->bytesReceived = byteRxTotal;
         obj->packetsSent += packetTx;
         obj->packetsReceived += packetRx;
         obj->errorsSent += errorsTx;
         obj->errorsReceived += errorsRx;
         obj->unicastPacketsSent += packetUniTx;
         obj->unicastPacketsReceived += packetUniRx;
         obj->discardPacketsSent += packetDiscardTx;
         obj->discardPacketsReceived += packetDiscardRx;
         obj->multicastPacketsSent += packetMultiTx;
         obj->multicastPacketsReceived += packetMultiRx;
         obj->broadcastPacketsSent += packetBcastTx;
         obj->broadcastPacketsReceived += packetBcastRx;
      }
   }

   return (CMSRET_SUCCESS);
}

CmsRet rutdsl_getBondingGroupTotalStats(Dev2DslBondingGroupStatsTotalObject *obj)
{
   adslMibInfo adslMib;
   long adslMibSize;

   cmsLog_debug("entered");

   adslMibSize=sizeof(adslMib);

   /* line 0 */
   if (CMSRET_SUCCESS != xdslCtl_GetObjectValue(0, NULL, 0, (char *) &adslMib, &adslMibSize))
   {
      cmsLog_error("could not get MIB for line 0");
      return CMSRET_INTERNAL_ERROR;
   }
   obj->erroredSeconds = adslMib.adslPerfData.perfTotal.adslESs;
   obj->severelyErroredSeconds = adslMib.adslPerfData.perfTotal.adslSES;
   obj->unavailableSeconds = adslMib.adslPerfData.perfTotal.adslUAS;

   /* line 1 */
   adslMibSize=sizeof(adslMib);
   if (CMSRET_SUCCESS != xdslCtl_GetObjectValue(1, NULL, 0, (char *) &adslMib, &adslMibSize))
   {
      cmsLog_error("could not get MIB for line 1");
      return CMSRET_INTERNAL_ERROR;
   }
   obj->erroredSeconds += adslMib.adslPerfData.perfTotal.adslESs;
   obj->severelyErroredSeconds += adslMib.adslPerfData.perfTotal.adslSES;
   obj->unavailableSeconds += adslMib.adslPerfData.perfTotal.adslUAS;

   return (CMSRET_SUCCESS);
}

CmsRet rutdsl_getBondingGroupCurrentDayStats(Dev2DslBondingGroupStatsCurrentDayObject *obj)
{
   adslMibInfo adslMib;
   long adslMibSize;

   cmsLog_debug("entered");

   adslMibSize=sizeof(adslMib);

   /* line 0 */
   if (CMSRET_SUCCESS != xdslCtl_GetObjectValue(0, NULL, 0, (char *) &adslMib, &adslMibSize))
   {
      cmsLog_error("could not get MIB for line 0");
      return CMSRET_INTERNAL_ERROR;
   }
   obj->erroredSeconds = adslMib.adslPerfData.perfCurr1Day.adslESs;
   obj->severelyErroredSeconds = adslMib.adslPerfData.perfCurr1Day.adslSES;
   obj->unavailableSeconds = adslMib.adslPerfData.perfCurr1Day.adslUAS;

   /* line 1 */
   adslMibSize=sizeof(adslMib);
   if (CMSRET_SUCCESS != xdslCtl_GetObjectValue(1, NULL, 0, (char *) &adslMib, &adslMibSize))
   {
      cmsLog_error("could not get MIB for line 1");
      return CMSRET_INTERNAL_ERROR;
   }
   obj->erroredSeconds += adslMib.adslPerfData.perfCurr1Day.adslESs;
   obj->severelyErroredSeconds += adslMib.adslPerfData.perfCurr1Day.adslSES;
   obj->unavailableSeconds += adslMib.adslPerfData.perfCurr1Day.adslUAS;

   return (CMSRET_SUCCESS);
}

CmsRet rutdsl_getBondingGroupQuarterHourStats(Dev2DslBondingGroupStatsQuarterHourObject *obj)
{
   adslMibInfo adslMib;
   long adslMibSize;

   cmsLog_debug("entered");

   adslMibSize=sizeof(adslMib);

   /* line 0 */
   if (CMSRET_SUCCESS != xdslCtl_GetObjectValue(0, NULL, 0, (char *) &adslMib, &adslMibSize))
   {
      cmsLog_error("could not get MIB for line 0");
      return CMSRET_INTERNAL_ERROR;
   }
   obj->erroredSeconds = adslMib.adslPerfData.perfCurr15Min.adslESs;
   obj->severelyErroredSeconds = adslMib.adslPerfData.perfCurr15Min.adslSES;
   obj->unavailableSeconds = adslMib.adslPerfData.perfCurr15Min.adslUAS;

   /* line 1 */
   adslMibSize=sizeof(adslMib);
   if (CMSRET_SUCCESS != xdslCtl_GetObjectValue(1, NULL, 0, (char *) &adslMib, &adslMibSize))
   {
      cmsLog_error("could not get MIB for line 1");
      return CMSRET_INTERNAL_ERROR;
   }
   obj->erroredSeconds += adslMib.adslPerfData.perfCurr15Min.adslESs;
   obj->severelyErroredSeconds += adslMib.adslPerfData.perfCurr15Min.adslSES;
   obj->unavailableSeconds += adslMib.adslPerfData.perfCurr15Min.adslUAS;

   return (CMSRET_SUCCESS);
}

#endif /* bonding */


#endif /* DMP_DEVICE2_DSL_1 */


#endif /*  DMP_DEVICE2_BASELINE_1 */
