/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2011:proprietary:standard

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

#ifdef SUPPORT_DSL
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
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_atm.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "rut_xtmlinkcfg.h"

#include "adslctlapi.h"
#include "AdslMibDef.h"
#include "bcmadsl.h"
#include "bcmxdsl.h"
#include "devctl_xtm.h"
#include "rut_dsl.h"
#include "rut_wanlayer2.h"
#include "devctl_adsl.h"
#include "adsl_api_trace.h"
#include "DiagDef.h"


CmsRet cmsAdsl_formatHLINString(char **paramValue, SINT32 *data, int maxDataStrLen)
{
   int count, totalCount=0;
   int i;
   char *dataStr, *dataPtr;
   char tmpBuf[50];
   SINT16 *pHlinDataPtrH=NULL;
   SINT16 *pHlinDataPtrL=NULL;

   cmsAdsl_formatHLINStringTrace();

   dataStr = malloc(maxDataStrLen);
   if (dataStr == NULL)
   {
      return CMSRET_RESOURCE_EXCEEDED;
   }
   else 
   {
      dataStr[0] = '\0';
      dataPtr = &dataStr[0];
      
      /* number of tone 512 */
      dataStr[0] = '\0';
      dataPtr = &dataStr[0];
      for (i = 0; i < NUM_TONE_GROUP; i++) 
      {
         pHlinDataPtrH = (SINT16*)(&data[i]);
         pHlinDataPtrL = (SINT16*)(&data[i]) + 1;
         count = sprintf(tmpBuf,"%d,%d", (*pHlinDataPtrH),(*pHlinDataPtrL));
         if ((totalCount + count) > maxDataStrLen)
         {
            cmsLog_error("After formating to element %d of data array, DSL driver data length (%d) exceeds max TR98 length limit %d\n", 
                         i,(totalCount+count),maxDataStrLen);
            break;
         }
         else
         {
            //pHlinDataPtr = (SINT16*)(&data[i]);
            count = sprintf(dataPtr,"%d,%d", (*pHlinDataPtrH),(*pHlinDataPtrL));
         }
         dataPtr += count;
         totalCount += count;
         if ((i+1) < NUM_TONE_GROUP)
         {
            count = sprintf(dataPtr,",");
            dataPtr += count;
            totalCount += count;
         }
      } /* loop */
   } /* dataStr ok */
   CMSMEM_REPLACE_STRING_FLAGS(*paramValue,dataStr,ALLOC_SHARED_MEM);
   free(dataStr);
   return (CMSRET_SUCCESS);
}

int cmsAdsl_paramNameToOid(char *paramName)
{
	if (strcmp(paramName,"QLNpsds") == 0)
	{
	   return (kOidAdslPrivQuietLineNoiseDsPerToneGroup);
	}
	if (strcmp(paramName,"QLNpsus") == 0)
	{
	   return (kOidAdslPrivQuietLineNoiseUsPerToneGroup);
	}
	if (strcmp(paramName,"SNRpsds") == 0)
	{
	   return (kOidAdslPrivSNRDsPerToneGroup);
	}
	if (strcmp(paramName,"SNRpsus") == 0)
	{
	   return (kOidAdslPrivSNRUsPerToneGroup);
	}
	if (strcmp(paramName,"HLOGpsds") == 0)
	{
	   return (kOidAdslPrivChanCharLogDsPerToneGroup);
	}
	if (strcmp(paramName,"HLOGpsus") == 0)
	{
	   return (kOidAdslPrivChanCharLogUsPerToneGroup);
	}
	if (strcmp(paramName,"BITSpsds") == 0)
	{
	   return (kOidAdslPrivBitAllocDsPerToneGroup);
	}
	return 0;
}

 CmsRet cmsAdsl_formatSubCarrierDataString(char **paramValue, void *data, char *paramName, int maxDataStrLen)
{
   int count;
   int totalCount = 0;
   char tmpBuf[50];
   int i, oid;
   char *dataStr, *dataPtr;
   ulong value;
   SINT16 *shortDataPtr;
   SINT8 *byteDataPtr;

   cmsAdsl_formatSubCarrierDataStringTrace();

   dataStr = malloc(maxDataStrLen);
   if (dataStr == NULL)
   {
      return CMSRET_RESOURCE_EXCEEDED;
   }
   else 
   {
      dataStr[0] = '\0';
      dataPtr = &dataStr[0];

      oid = cmsAdsl_paramNameToOid(paramName);

      switch (oid)
      {
      case kOidAdslPrivQuietLineNoiseDsPerToneGroup:
      case kOidAdslPrivQuietLineNoiseUsPerToneGroup:
         shortDataPtr = (SINT16*)data;
         for (i = 0; i < NUM_TONE_GROUP; i++)
         {
            if (shortDataPtr[i] == 0)
            {
               value = 255;
            }
            else
            {
               value= (ulong) ((-shortDataPtr[i]-368) >> 3);
               if (value > 254)
               {
                  value = 255;
               }
            }
            count = sprintf(tmpBuf,"%d", (UINT8)value);
            if ((totalCount + count) <= maxDataStrLen)
            {
               count = sprintf(dataPtr,"%d", (UINT8)value);
               totalCount += count;
               dataPtr += count;
               if ((i+1) < NUM_TONE_GROUP)
               {
                  count = sprintf(dataPtr,",");
                  dataPtr += count;
                  totalCount += count;
               }
            }
            else
            {
               break;
            }
         } /* NUM_TONE_GROUP loop */
         break;

      case kOidAdslPrivSNRDsPerToneGroup:
      case kOidAdslPrivSNRUsPerToneGroup:
         shortDataPtr = (SINT16*)data;
         for (i = 0; i < NUM_TONE_GROUP; i++)
         {
            if (shortDataPtr[i] == 0)
            {
               value = 255;
            }
            else
            {
               value= (ulong) ((shortDataPtr[i]+512) >> 3);
               if (value > 254)
                  value = 255;
            }
            count = sprintf(tmpBuf,"%d", (UINT8)value);
            if ((totalCount + count) <= maxDataStrLen)
            {
               count = sprintf(dataPtr,"%d", (UINT8)value);
               totalCount += count;
               dataPtr += count;
               if ((i+1) < NUM_TONE_GROUP)
               {
                  count = sprintf(dataPtr,",");
                  dataPtr += count;
                  totalCount += count;
               }
            }
            else 
            {
               break;
            }
         } /* NUM_TONE_GROUP loop */
         break;

      case kOidAdslPrivChanCharLogDsPerToneGroup:
      case kOidAdslPrivChanCharLogUsPerToneGroup:
         shortDataPtr = (SINT16*)data;
         for (i = 0; i < NUM_TONE_GROUP; i++)
         {
            if (shortDataPtr[i] == 0)
            {
               value = 0x3ff;
            }
            else
            {
               value = (ulong) (((96-shortDataPtr[i])*5) >> 3);
               if (value > 0x3FE) 
               {
                  value=0x3FF;
               }
            }
            count = sprintf(tmpBuf,"%u", (unsigned int)value);
            if ((totalCount + count) <= maxDataStrLen)
            {
               count = sprintf(dataPtr,"%u", (unsigned int)value);
               totalCount += count;
               dataPtr += count;
               if ((i+1) < NUM_TONE_GROUP)
               {
                  count = sprintf(dataPtr,",");
                  dataPtr += count;
                  totalCount += count;
               }
            }
            else 
            {
               break;
            }
         } /* NUM_TONE_GROUP loop */
         break;
            
      case kOidAdslPrivBitAllocDsPerToneGroup:
         byteDataPtr = (SINT8*)data;
         for (i = 0; i < NUM_TONE_GROUP; i++)
         {
            count = sprintf(tmpBuf,"%d", (byteDataPtr[i]));
            if ((totalCount + count) <= maxDataStrLen)
            {
               count = sprintf(dataPtr,"%d", byteDataPtr[i]);
               totalCount += count;
               dataPtr += count;
               if ((i+1) < NUM_TONE_GROUP)
               {
                  count = sprintf(dataPtr,",");
                  dataPtr += count;
                  totalCount += count;
               }
            }
            else 
            {
               break;
            }
         } /* NUM_TONE_GROUP loop */

         if (i < NUM_TONE_GROUP)
         {
            cmsLog_error("[oid %d]After formating to element %d of data array, DSL driver data length (%d) exceeds max TR98 length limit %d\n", 
                         oid,i,(totalCount+count),maxDataStrLen);
         }
         break;
      }/* switch */
   } /* dataStr */

   CMSMEM_REPLACE_STRING_FLAGS(*paramValue,dataStr,ALLOC_SHARED_MEM);
   free(dataStr);
   return (CMSRET_SUCCESS);
} /* cmsAdsl_formatSubCarrierDataString */

CmsRet cmsAdsl_formatPertoneGroupQ4String(char **paramValue, void *data, int maxDataStrLen)
{
   int count;
   int totalCount = 0;
   char tmpBuf[50];
   int i;
   char *dataStr, *dataPtr;
   /* only for GAINpsds now */
   SINT16 *shortDataPtr = (SINT16*)data;

   cmsAdsl_formatPertoneGroupQ4StringTrace();

   dataStr = malloc(maxDataStrLen);
   if (dataStr == NULL)
   {
      return CMSRET_RESOURCE_EXCEEDED;
   }
   else 
   {
      dataStr[0] = '\0';
      dataPtr = &dataStr[0];
      for (i = 0; i < NUM_TONE_GROUP; i++)
      {
         count = sprintf(tmpBuf,"%s", QnToString(shortDataPtr[i],4));
         if ((totalCount+count) <= maxDataStrLen)
         {
            count = sprintf(dataPtr,"%s", QnToString(shortDataPtr[i],4));
            dataPtr += count;
            totalCount += count;
            if ((i+1) < NUM_TONE_GROUP)
            {
               count = sprintf(dataPtr,",");
               dataPtr += count;
               totalCount += count;
            }
         }
         else
         {
            cmsLog_error("[Q4string]After formating to element %d of data array, DSL driver data length (%d) exceeds max TR98 length limit\
 %d\n",
                         i,(totalCount+count),maxDataStrLen);
            break;
         }
      } /* num_tone_group */
   }
   CMSMEM_REPLACE_STRING_FLAGS(*paramValue,dataStr,ALLOC_SHARED_MEM);
   free(dataStr);
   return (CMSRET_SUCCESS);
} /* getToneDataStringU */


#ifdef DMP_ADSLWAN_1

void xdslUtil_CfgProfileInit(adslCfgProfile * pAdslCfg,  WanDslIntfCfgObject * pDlIntfCfg)
{
    long dslCfgParam = pDlIntfCfg->X_BROADCOM_COM_DslCfgParam;
    
    pAdslCfg->adslHsModeSwitchTime = pDlIntfCfg->X_BROADCOM_COM_DslHsModeSwitchTime;
    pAdslCfg->adslLOMTimeThldSec = pDlIntfCfg->X_BROADCOM_COM_DslLOMTimeThldSec;
    pAdslCfg->adslPwmSyncClockFreq = pDlIntfCfg->X_BROADCOM_COM_DslPwmSyncClockFreq;
    pAdslCfg->adslShowtimeMarginQ4 = pDlIntfCfg->X_BROADCOM_COM_DslShowtimeMarginQ4;
    pAdslCfg->adslTrainingMarginQ4 = pDlIntfCfg->X_BROADCOM_COM_DslTrainingMarginQ4;
    pAdslCfg->adslDemodCapMask = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg1Mask;
    pAdslCfg->adslDemodCapValue = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg1Value;
    pAdslCfg->adslDemodCap2Mask = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg2Mask;
    pAdslCfg->adslDemodCap2Value = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg2Value;
    pAdslCfg->adsl2Param = pDlIntfCfg->X_BROADCOM_COM_DslParam;
#ifdef SUPPORT_CFG_PROFILE
    pAdslCfg->xdslAuxFeaturesMask = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg3Mask;
    pAdslCfg->xdslAuxFeaturesValue = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg3Value;
    pAdslCfg->vdslCfgFlagsMask = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg4Mask;
    pAdslCfg->vdslCfgFlagsValue = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg4Value;
    pAdslCfg->xdslCfg1Mask = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg5Mask;
    pAdslCfg->xdslCfg1Value = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg5Value;
    pAdslCfg->xdslCfg2Mask = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg6Mask;
    pAdslCfg->xdslCfg2Value = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg6Value;
    pAdslCfg->xdslCfg3Mask = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg7Mask;
    pAdslCfg->xdslCfg3Value = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg7Value;
    pAdslCfg->xdslCfg4Mask = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg8Mask;
    pAdslCfg->xdslCfg4Value = pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg8Value;
#endif
    pAdslCfg->maxUsDataRateKbps = pDlIntfCfg->X_BROADCOM_COM_DslPhyUsDataRateKbps;
    pAdslCfg->maxDsDataRateKbps = pDlIntfCfg->X_BROADCOM_COM_DslPhyDsDataRateKbps;
    pAdslCfg->maxAggrDataRateKbps = pDlIntfCfg->X_BROADCOM_COM_DslPhyAggrDataRateKbps;
    pAdslCfg->xdslMiscCfgParam = pDlIntfCfg->X_BROADCOM_COM_DslPhyMiscCfgParam;
    
    cmsLog_debug("AdslModulationCfg=%s\n", pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg);

    /* Modulation type */
    dslCfgParam &= ~kAdslCfgModMask;
    if(cmsUtl_isSubOptionPresent(pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg, MDMVS_ADSL_MODULATION_ALL)) {
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
        if (cmsUtl_isSubOptionPresent(pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_G_DMT))
            dslCfgParam |= kAdslCfgModGdmtOnly;
        if (cmsUtl_isSubOptionPresent(pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_G_LITE))
            dslCfgParam |= kAdslCfgModGliteOnly;
        if (cmsUtl_isSubOptionPresent(pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_G_DMT_BIS))
            dslCfgParam |= kAdslCfgModAdsl2Only;
        if (cmsUtl_isSubOptionPresent(pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_RE_ADSL))
            pAdslCfg->adsl2Param |= kAdsl2CfgReachExOn;
        else
            pAdslCfg->adsl2Param &= ~kAdsl2CfgReachExOn;
        if (cmsUtl_isSubOptionPresent(pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_2PLUS))
            dslCfgParam |= kAdslCfgModAdsl2pOnly;
        if (cmsUtl_isSubOptionPresent(pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg,MDMVS_ADSL_ANSI_T1_413))
            dslCfgParam |= kAdslCfgModT1413Only;
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
        if (cmsUtl_isSubOptionPresent(pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg,MDMVS_VDSL2))
            dslCfgParam |= kDslCfgModVdsl2Only;
#ifdef SUPPORT_DSL_GFAST
        if (cmsUtl_isSubOptionPresent(pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg,MDMVS_G_FAST))
            dslCfgParam |= kDslCfgModGfastOnly;
#endif
#endif
    }

    if ( (cmsUtl_isSubOptionPresent(pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg, MDMVS_ANNEXM)) ||
        (pDlIntfCfg->X_BROADCOM_COM_ADSL2_AnnexM == TRUE) )
        pAdslCfg->adsl2Param |= kAdsl2CfgAnnexMEnabled;
    else
        pAdslCfg->adsl2Param &= ~(kAdsl2CfgAnnexMEnabled | kAdsl2CfgAnnexMOnly | kAdsl2CfgAnnexMpXMask);

    /* Phone line pair */
    dslCfgParam &= ~kAdslCfgLinePairMask;
    if (!strcmp(pDlIntfCfg->X_BROADCOM_COM_PhoneLinePair, MDMVS_INNER_PAIR))
        dslCfgParam |= kAdslCfgLineInnerPair;
    else
        dslCfgParam |= kAdslCfgLineOuterPair;

    /* Bit swap */
    pAdslCfg->adslDemodCapMask |= kXdslBitSwapEnabled;
    if (!strcmp(pDlIntfCfg->X_BROADCOM_COM_Bitswap, MDMVS_ON))
        pAdslCfg->adslDemodCapValue |= kXdslBitSwapEnabled;
    else
        pAdslCfg->adslDemodCapValue &= ~kXdslBitSwapEnabled;
    
    /* SRA */
    pAdslCfg->adslDemodCapMask |= kXdslSRAEnabled;
    if (!strcmp(pDlIntfCfg->X_BROADCOM_COM_SRA, MDMVS_ON))
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
        if(pDlIntfCfg->X_BROADCOM_COM_VDSL_8a)
            pAdslCfg->vdslParam |= kVdslProfile8a;
        if(pDlIntfCfg->X_BROADCOM_COM_VDSL_8b)
            pAdslCfg->vdslParam |= kVdslProfile8b;
        if(pDlIntfCfg->X_BROADCOM_COM_VDSL_8c)
            pAdslCfg->vdslParam |= kVdslProfile8c;
        if(pDlIntfCfg->X_BROADCOM_COM_VDSL_8d)
            pAdslCfg->vdslParam |= kVdslProfile8d;
        if(pDlIntfCfg->X_BROADCOM_COM_VDSL_12a)
            pAdslCfg->vdslParam |= kVdslProfile12a;
        if(pDlIntfCfg->X_BROADCOM_COM_VDSL_12b)
            pAdslCfg->vdslParam |= kVdslProfile12b;
        if(pDlIntfCfg->X_BROADCOM_COM_VDSL_17a)
            pAdslCfg->vdslParam |= kVdslProfile17a;
        if(pDlIntfCfg->X_BROADCOM_COM_VDSL_30a)
            pAdslCfg->vdslParam |= kVdslProfile30a;
#ifdef CONFIG_VDSLBRCMPRIV1_SUPPORT
        if(pDlIntfCfg->X_BROADCOM_COM_VDSL_BrcmPriv1)
            pAdslCfg->vdslParam |= kVdslProfileBrcmPriv1;
#endif
#ifdef CONFIG_VDSLBRCMPRIV2_SUPPORT
        if(pDlIntfCfg->X_BROADCOM_COM_VDSL_BrcmPriv2)
            pAdslCfg->vdslParam |= kVdslProfileBrcmPriv2;
#endif
        if(pDlIntfCfg->X_BROADCOM_COM_VDSL_US0_8a)
            pAdslCfg->vdslParam |= kVdslUS0Mask;
    }
#ifdef SUPPORT_DSL_GFAST
    if(dslCfgParam & kDslCfgModGfastOnly) {
       if(!pDlIntfCfg->X_BROADCOM_COM_FAST_106a)
           pAdslCfg->vdslParam |= kGfastProfile106aDisable;
       if(!pDlIntfCfg->X_BROADCOM_COM_FAST_212a)
           pAdslCfg->vdslParam |= kGfastProfile212aDisable;
       if(!pDlIntfCfg->X_BROADCOM_COM_FAST_106b)
           pAdslCfg->vdslParam |= kGfastProfile106bDisable;
       if(!pDlIntfCfg->X_BROADCOM_COM_FAST_106c)
           pAdslCfg->vdslParam |= kGfastProfile106cDisable;
       if(!pDlIntfCfg->X_BROADCOM_COM_FAST_212c)
           pAdslCfg->vdslParam |= kGfastProfile212cDisable;
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

void xdslUtil_IntfCfgInit(adslCfgProfile *pAdslCfg, WanDslIntfCfgObject *pDlIntfCfg)
{
    int len;
    char    cfgModType[BUFLEN_128];
#ifdef ANNEX_C
    ulong   dslCfgParam = pAdslCfg->adslAnnexCParam;
#else
    ulong   dslCfgParam = pAdslCfg->adslAnnexAParam;
#endif

    pDlIntfCfg->X_BROADCOM_COM_DslCfgParam = dslCfgParam;
    pDlIntfCfg->X_BROADCOM_COM_DslHsModeSwitchTime = pAdslCfg->adslHsModeSwitchTime;
    pDlIntfCfg->X_BROADCOM_COM_DslLOMTimeThldSec = pAdslCfg->adslLOMTimeThldSec;
    pDlIntfCfg->X_BROADCOM_COM_DslPwmSyncClockFreq = pAdslCfg->adslPwmSyncClockFreq;
    pDlIntfCfg->X_BROADCOM_COM_DslShowtimeMarginQ4 = pAdslCfg->adslShowtimeMarginQ4;
    pDlIntfCfg->X_BROADCOM_COM_DslTrainingMarginQ4 = pAdslCfg->adslTrainingMarginQ4;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg1Mask = pAdslCfg->adslDemodCapMask;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg1Value = pAdslCfg->adslDemodCapValue;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg2Mask = pAdslCfg->adslDemodCap2Mask;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg2Value = pAdslCfg->adslDemodCap2Value;
    pDlIntfCfg->X_BROADCOM_COM_DslParam = pAdslCfg->adsl2Param;
#ifdef SUPPORT_CFG_PROFILE
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg3Mask = pAdslCfg->xdslAuxFeaturesMask;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg3Value = pAdslCfg->xdslAuxFeaturesValue;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg4Mask = pAdslCfg->vdslCfgFlagsMask;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg4Value = pAdslCfg->vdslCfgFlagsValue;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg5Mask = pAdslCfg->xdslCfg1Mask;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg5Value = pAdslCfg->xdslCfg1Value;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg6Mask = pAdslCfg->xdslCfg2Mask;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg6Value = pAdslCfg->xdslCfg2Value;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg7Mask = pAdslCfg->xdslCfg3Mask;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg7Value = pAdslCfg->xdslCfg3Value;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg8Mask = pAdslCfg->xdslCfg4Mask;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyCfg8Value = pAdslCfg->xdslCfg4Value;
#endif

    pDlIntfCfg->X_BROADCOM_COM_DslPhyUsDataRateKbps = pAdslCfg->maxUsDataRateKbps;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyDsDataRateKbps = pAdslCfg->maxDsDataRateKbps;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyAggrDataRateKbps = pAdslCfg->maxAggrDataRateKbps;
    pDlIntfCfg->X_BROADCOM_COM_DslPhyMiscCfgParam = pAdslCfg->xdslMiscCfgParam;

    /* Modulations */
    cmsMem_free(pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg);
    if((kAdslCfgModAny == (dslCfgParam & kAdslCfgModMask)) && !(pAdslCfg->adsl2Param & kAdsl2CfgAnnexMEnabled)) {
        pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg = cmsMem_strdupFlags(MDMVS_ADSL_MODULATION_ALL, mdmLibCtx.allocFlags);
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
           pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg = cmsMem_strdupFlags(cfgModType, mdmLibCtx.allocFlags);
        }
        else {
           /* default will be all */
           pDlIntfCfg->X_BROADCOM_COM_AdslModulationCfg = cmsMem_strdupFlags(MDMVS_ADSL_MODULATION_ALL, mdmLibCtx.allocFlags);
        }
    }
    pDlIntfCfg->X_BROADCOM_COM_ADSL2_AnnexM = ((pAdslCfg->adsl2Param & kAdsl2CfgAnnexMEnabled) != 0);

    /* VDSL2 profile */
#ifdef  DMP_VDSL2WAN_1
    pDlIntfCfg->X_BROADCOM_COM_VDSL_8a = ((pAdslCfg->vdslParam & kVdslProfile8a) != 0);
    pDlIntfCfg->X_BROADCOM_COM_VDSL_8b = ((pAdslCfg->vdslParam & kVdslProfile8b) != 0);
    pDlIntfCfg->X_BROADCOM_COM_VDSL_8c = ((pAdslCfg->vdslParam & kVdslProfile8c) != 0);
    pDlIntfCfg->X_BROADCOM_COM_VDSL_8d = ((pAdslCfg->vdslParam & kVdslProfile8d) != 0);
    pDlIntfCfg->X_BROADCOM_COM_VDSL_12a = ((pAdslCfg->vdslParam & kVdslProfile12a) != 0);
    pDlIntfCfg->X_BROADCOM_COM_VDSL_12b = ((pAdslCfg->vdslParam & kVdslProfile12b) != 0);
    pDlIntfCfg->X_BROADCOM_COM_VDSL_17a = ((pAdslCfg->vdslParam & kVdslProfile17a) != 0);
    pDlIntfCfg->X_BROADCOM_COM_VDSL_30a = ((pAdslCfg->vdslParam & kVdslProfile30a) != 0);
#ifdef CONFIG_VDSLBRCMPRIV1_SUPPORT
    pDlIntfCfg->X_BROADCOM_COM_VDSL_BrcmPriv1 = ((pAdslCfg->vdslParam & kVdslProfileBrcmPriv1) != 0);
#endif
#ifdef CONFIG_VDSLBRCMPRIV2_SUPPORT
    pDlIntfCfg->X_BROADCOM_COM_VDSL_BrcmPriv2 = ((pAdslCfg->vdslParam & kVdslProfileBrcmPriv2) != 0);
#endif
    pDlIntfCfg->X_BROADCOM_COM_VDSL_US0_8a = ((pAdslCfg->vdslParam & kVdslUS0Mask) != 0);
#ifdef SUPPORT_DSL_GFAST
    pDlIntfCfg->X_BROADCOM_COM_FAST_106a = ((pAdslCfg->vdslParam & kGfastProfile106aDisable) == 0);
    pDlIntfCfg->X_BROADCOM_COM_FAST_212a = ((pAdslCfg->vdslParam & kGfastProfile212aDisable) == 0);
    pDlIntfCfg->X_BROADCOM_COM_FAST_106b = ((pAdslCfg->vdslParam & kGfastProfile106bDisable) == 0);
    pDlIntfCfg->X_BROADCOM_COM_FAST_106c = ((pAdslCfg->vdslParam & kGfastProfile106cDisable) == 0);
    pDlIntfCfg->X_BROADCOM_COM_FAST_212c = ((pAdslCfg->vdslParam & kGfastProfile212cDisable) == 0);
#endif
#endif

    /* Capability */
   /* sra */
    cmsMem_free(pDlIntfCfg->X_BROADCOM_COM_SRA);
    if (pAdslCfg->adslDemodCapValue & kXdslSRAEnabled)
        pDlIntfCfg->X_BROADCOM_COM_SRA = cmsMem_strdupFlags(MDMVS_ON, mdmLibCtx.allocFlags);
    else
        pDlIntfCfg->X_BROADCOM_COM_SRA = cmsMem_strdupFlags(MDMVS_OFF, mdmLibCtx.allocFlags);
    /* bitswap */
    cmsMem_free(pDlIntfCfg->X_BROADCOM_COM_Bitswap);
    if (pAdslCfg->adslDemodCapValue & kXdslBitSwapEnabled)
        pDlIntfCfg->X_BROADCOM_COM_Bitswap = cmsMem_strdupFlags(MDMVS_ON, mdmLibCtx.allocFlags);
    else
        pDlIntfCfg->X_BROADCOM_COM_Bitswap = cmsMem_strdupFlags(MDMVS_OFF, mdmLibCtx.allocFlags);
    
    cmsMem_free(pDlIntfCfg->X_BROADCOM_COM_PhoneLinePair);
    if(kAdslCfgLineOuterPair == (dslCfgParam & kAdslCfgLinePairMask))
        pDlIntfCfg->X_BROADCOM_COM_PhoneLinePair = cmsMem_strdupFlags(MDMVS_OUTER_PAIR, mdmLibCtx.allocFlags);
    else
        pDlIntfCfg->X_BROADCOM_COM_PhoneLinePair = cmsMem_strdupFlags(MDMVS_INNER_PAIR, mdmLibCtx.allocFlags);
}

UBOOL8 rutDsl_isDslConfigChanged(const _WanDslIntfCfgObject *newObj, const _WanDslIntfCfgObject *currObj)
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
       (0 != strcmp(newObj->X_BROADCOM_COM_PhoneLinePair, currObj->X_BROADCOM_COM_PhoneLinePair)) ||
       (newObj->X_BROADCOM_COM_ADSL2_AnnexM != currObj->X_BROADCOM_COM_ADSL2_AnnexM)
#ifdef DMP_VDSL2WAN_1
       || (newObj->X_BROADCOM_COM_VDSL_8a != currObj->X_BROADCOM_COM_VDSL_8a) ||
       (newObj->X_BROADCOM_COM_VDSL_8b != currObj->X_BROADCOM_COM_VDSL_8b) ||
       (newObj->X_BROADCOM_COM_VDSL_8c != currObj->X_BROADCOM_COM_VDSL_8c) ||
       (newObj->X_BROADCOM_COM_VDSL_8d != currObj->X_BROADCOM_COM_VDSL_8d) ||
       (newObj->X_BROADCOM_COM_VDSL_12a != currObj->X_BROADCOM_COM_VDSL_12a) ||
       (newObj->X_BROADCOM_COM_VDSL_12b != currObj->X_BROADCOM_COM_VDSL_12b) ||
       (newObj->X_BROADCOM_COM_VDSL_17a != currObj->X_BROADCOM_COM_VDSL_17a) ||
       (newObj->X_BROADCOM_COM_VDSL_30a != currObj->X_BROADCOM_COM_VDSL_30a) ||
       (newObj->X_BROADCOM_COM_VDSL_US0_8a != currObj->X_BROADCOM_COM_VDSL_US0_8a)
#endif
       )
   {
      changed = TRUE;
   }

   return changed;
}


CmsRet rutDsl_configUp(WanDslIntfCfgObject *dslIntfCfg)
{
	 UBOOL8 xDSLBonding = FALSE;
	 adslCfgProfile adslCfg;

	 if (!dslIntfCfg->X_BROADCOM_COM_Bitswap &&
				 !dslIntfCfg->X_BROADCOM_COM_PhoneLinePair &&
				 !dslIntfCfg->X_BROADCOM_COM_SRA &&
				 !dslIntfCfg->modulationType)
	 {
			cmsLog_error("Invalid dslIntfCfg parameters");
			return CMSRET_INVALID_PARAM_VALUE;
	 }

#ifdef DMP_VDSL2WAN_1
	 cmsLog_debug("bitswap %s, phone %s, sra %s, modulation %s, profile 8a=%d 8b=%d 8c=%d 8c=%d",
				 dslIntfCfg->X_BROADCOM_COM_Bitswap,
				 dslIntfCfg->X_BROADCOM_COM_PhoneLinePair,
				 dslIntfCfg->X_BROADCOM_COM_SRA,
				 dslIntfCfg->X_BROADCOM_COM_AdslModulationCfg,
				 dslIntfCfg->X_BROADCOM_COM_VDSL_8a,
				 dslIntfCfg->X_BROADCOM_COM_VDSL_8b,
				 dslIntfCfg->X_BROADCOM_COM_VDSL_8c,
				 dslIntfCfg->X_BROADCOM_COM_VDSL_8d);
#endif
	/* initialialize adslCfgProfile */
	memset((char *) &adslCfg, 0x00, sizeof(adslCfg));

	xdslUtil_CfgProfileInit(&adslCfg, dslIntfCfg);
	
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
#endif
#endif

#ifdef SECONDARY_AFEID_FN
	{
		unsigned int	afeIds[2] = {0, 0};
		int	i = 0;
		char	*pToken, *pLast, line[64], tokenSeperator[] = " \n";
		FILE	*fs = fopen(SECONDARY_AFEID_FN, "r");
		if (NULL != fs) {
			if(fgets(line, 64, fs) != NULL) {
				pToken = strtok_r(line, tokenSeperator, &pLast);
				while((NULL != pToken) && (i < 2)) {
					if ((pToken[0] == '0') && ((pToken[1] == 'x') || (pToken[1] == 'X')))
						afeIds[i] = strtoul(pToken+2, NULL, 16);
					pToken = strtok_r(NULL, tokenSeperator, &pLast);
					i++;
				}
				if((afeIds[0] != 0) && (afeIds[1] != 0)) {
					if(CMSRET_SUCCESS != xdslCtl_DiagProcessDbgCommand(0,DIAG_DEBUG_CMD_OVERRIDE_2ND_AFEIDS,0,afeIds[0],afeIds[1]))
						cmsLog_error("xdslCtl_DiagProcessDbgCommand(0,DIAG_DEBUG_CMD_OVERRIDE_2ND_AFEIDS...) failed!");
				}
				else
					cmsLog_error("No valid afeId in %s file!", SECONDARY_AFEID_FN);
			}
			else
				cmsLog_error("fgets from %s file failed!", SECONDARY_AFEID_FN);
		}
	}
#endif

	 cmsAdsl_initialize(&adslCfg);

	 {
			XTM_INITIALIZATION_PARMS InitParms;
			XTM_INTERFACE_CFG IntfCfg;

			memset((UINT8 *)  &InitParms, 0x00, sizeof(InitParms));
			memset((UINT8 *)  &IntfCfg, 0x00, sizeof(IntfCfg));

			InitParms.bondConfig.sConfig.ptmBond = BC_PTM_BONDING_DISABLE ;
			InitParms.bondConfig.sConfig.atmBond = BC_ATM_BONDING_DISABLE ;

			if (xDSLBonding == TRUE) {
				 InitParms.bondConfig.sConfig.ptmBond = BC_PTM_BONDING_ENABLE ;
				 InitParms.bondConfig.sConfig.atmBond = BC_ATM_BONDING_ENABLE ;
			}

			cmsLog_debug("DSL PTM Bonding %s", InitParms.bondConfig.sConfig.ptmBond ? "Enable" : "Disable");
			cmsLog_debug("DSL ATM Bonding %s", InitParms.bondConfig.sConfig.atmBond ? "Enable" : "Disable");

#if defined(SUPPORT_DSL_BONDING)
			if (InitParms.bondConfig.sConfig.atmBond == BC_ATM_BONDING_ENABLE)
			InitParms.bondConfig.sConfig.autoSenseAtm = BC_ATM_AUTO_SENSE_ENABLE ;
         else
			   InitParms.bondConfig.sConfig.autoSenseAtm = BC_ATM_AUTO_SENSE_DISABLE ;
#if defined(SUPPORT_EXT_DSL_BONDING)
			/* 63268 currently has this configuration. */
         InitParms.ulPortConfig = PC_INTERNAL_EXTERNAL ;
#endif
#else
		   InitParms.bondConfig.sConfig.autoSenseAtm = BC_ATM_AUTO_SENSE_DISABLE ;
#endif

			devCtl_xtmInitialize( &InitParms ) ;

			/* TBD. Need configuration to indicate how many ports to enable. */
			IntfCfg.ulIfAdminStatus = ADMSTS_UP;
			devCtl_xtmSetInterfaceCfg( PORT_PHY0_FAST, &IntfCfg );
			devCtl_xtmSetInterfaceCfg( PORT_PHY0_INTERLEAVED, &IntfCfg );
	 }

	 cmsAdsl_start();

	 if (xDSLBonding == TRUE)
			xdslCtl_ConnectionStart(1);   /* Start line 1 */

	 return CMSRET_SUCCESS;
}
#endif /* DMP_ADSLWAN_1 */

void rutDsl_configDown(void)
{
   cmsAdsl_stop();
   cmsAdsl_uninitialize();
}

#ifdef DMP_ADSLWAN_1
UBOOL8 rutDsl_getDslWanDevIidStack(UBOOL8 isAtm, InstanceIdStack *wanDevIid)
{
   _WanDevObject *wanDev=NULL;
   _WanDslIntfCfgObject *dslIntfCfg=NULL; 
   InstanceIdStack  dslIntfIid = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;
   CmsRet ret;

   INIT_INSTANCE_ID_STACK(wanDevIid);
   
   /* find ATM/PTM  WANDEV according to isAtm.  */
   while (!found &&
          (ret = cmsObj_getNext(MDMOID_WAN_DEV, wanDevIid, (void **)&wanDev)) == CMSRET_SUCCESS)
   {
      cmsObj_free((void **)&wanDev);  /* no longer needed */

      if ((ret = cmsObj_getNextInSubTree(MDMOID_WAN_DSL_INTF_CFG, wanDevIid, &dslIntfIid, (void **)&dslIntfCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_getNextInSubTree <MDMOID_WAN_DSL_INTF_CFG> error. ret=%d", ret);
         return ret;
      }
      cmsLog_debug("dslIntfCfg->linkEncapsulationUsed=%s", dslIntfCfg->linkEncapsulationUsed);
      
      if ((isAtm && cmsUtl_strcmp(dslIntfCfg->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM) == 0) ||
          (!isAtm && cmsUtl_strcmp(dslIntfCfg->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM) == 0))
      {
          found = TRUE;
      }

      cmsObj_free((void **)&dslIntfCfg);  /* no longer needed */
   }

   /* no ATM/PTM WANDEV in the system */
   if (!found)
   {
      cmsLog_error("DSL WanDev does not exist?");
   }

   cmsLog_debug("isATM=%d, wanDevIid=%s, found=%d", isAtm, cmsMdm_dumpIidStack(wanDevIid), found);

   return found;
   
}


UBOOL8 rutDsl_getDslLinkByIfName(char *ifName, InstanceIdStack *iidStack, WanDslLinkCfgObject **dslLinkCfg)
{
   _WanDslLinkCfgObject *obj=NULL;
   UBOOL8 found=FALSE;
   UBOOL8 isAtm;
   InstanceIdStack wanDevIid;

   isAtm = TRUE;
   if (rutDsl_getDslWanDevIidStack(isAtm, &wanDevIid) == FALSE)
   {
      return found;
   }
   while (cmsObj_getNextInSubTree(MDMOID_WAN_DSL_LINK_CFG, &wanDevIid, iidStack, (void **)&obj) == CMSRET_SUCCESS)
   {
      cmsLog_debug("ifName = %s", obj->X_BROADCOM_COM_IfName);
      if (cmsUtl_strcmp(obj->X_BROADCOM_COM_IfName, ifName) ==0)
      {
         found = TRUE;
         break;
      }
      else
      {
         cmsObj_free((void **) &obj);
      }
   }

   if (found)
   {
      if (dslLinkCfg != NULL)
      {
         *dslLinkCfg = obj;
      }
      else
      {
         cmsObj_free((void **) &obj);
      }
   }

   cmsLog_debug("found=%d, ifName=%s", found, ifName);
   
   return found;
}
#endif /* DMP_ADSLWAN_1 */

#ifdef DMP_PTMWAN_1
UBOOL8 rutDsl_getPtmLinkByIfName(char *ifName, InstanceIdStack *iidStack, WanPtmLinkCfgObject **ptmLinkCfg)
{
   _WanPtmLinkCfgObject *obj;
   UBOOL8 found=FALSE;
   UBOOL8 isAtm;
   InstanceIdStack wanDevIid;

   isAtm = FALSE;
   if (rutDsl_getDslWanDevIidStack(isAtm, &wanDevIid) == FALSE)
   {
      return found;
   }
   while (cmsObj_getNextInSubTree(MDMOID_WAN_PTM_LINK_CFG, &wanDevIid, iidStack, (void **)&obj) == CMSRET_SUCCESS)
   {
      cmsLog_debug("ifName = %s", obj->X_BROADCOM_COM_IfName);
      if (cmsUtl_strcmp(obj->X_BROADCOM_COM_IfName, ifName) ==0)
      {
         found = TRUE;
         break;
      }
      else
      {
         cmsObj_free((void **) &obj);
      }
   }

   if (found)
   {
      if (ptmLinkCfg != NULL)
      {
         *ptmLinkCfg = obj;
      }
      else
      {
         cmsObj_free((void **) &obj);
      }
   }

   cmsLog_debug("found=%d, ifName=%s", found, ifName);
   
   return found;
}
#endif /* DMP_PTMWAN_1 */

CmsRet rutDsl_configPPPoA(const char *cmdLineIn, const char *baseIfName, SINT32 *pppPid)
{
   SINT32 pid=CMS_INVALID_PID;
   CmsRet ret=CMSRET_SUCCESS;
   char cmdLine[BUFLEN_128];

   strncpy(cmdLine, cmdLineIn, sizeof(cmdLine)-1);
   
   cmsLog_debug("pppoe string=%s on base interface %s", cmdLine, baseIfName);
   
   pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_PPP, cmdLine, strlen(cmdLine)+1);

   if (pid == CMS_INVALID_PID)
   {
      cmsLog_error("Failed to start pppoe.");
      ret = CMSRET_INTERNAL_ERROR;
   }

   cmsLog_debug("Restart pppoe msg sent, new pppoe pid=%d", pid);
   *pppPid =pid;

   cmsLog_debug("ret %d, pppPid %d", ret, *pppPid);

   return ret;

}

#ifdef DMP_ADSLWAN_1
CmsRet rutDsl_fillL2IfName(const Layer2IfNameType ifNameType, char **ifName)
{
   CmsRet ret;
   SINT32 intfArray[IFC_WAN_MAX];
   SINT32 index = 0;
   char *prefix;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char L2IfName[CMS_IFNAME_LENGTH];
   
   memset((UINT8 *) &intfArray, 0, sizeof(intfArray));
   
   switch (ifNameType)
   {
      case ATM_EOA:
      case ATM_PPPOA:
      {
         WanDslLinkCfgObject *dslLink=NULL;
         UBOOL8 isAtm = TRUE;
         InstanceIdStack wanDevIid;

         if (rutDsl_getDslWanDevIidStack(isAtm, &wanDevIid) == FALSE)
         {
            cmsLog_error("Fail to get WanDev iidStack.");
            return CMSRET_INTERNAL_ERROR;
         }
         while ((index < IFC_WAN_MAX) &&
            (ret = cmsObj_getNextInSubTree(MDMOID_WAN_DSL_LINK_CFG, &wanDevIid, &iidStack, (void **) &dslLink)) == CMSRET_SUCCESS)
         {
            if (dslLink->X_BROADCOM_COM_IfName == NULL)
            {
               /* this is one we just created and is NULL, so break */
               cmsObj_free((void **) &dslLink);
               break;
            }
            else
            {
               index = atoi(&(dslLink)->X_BROADCOM_COM_IfName[strlen(ATM_IFC_STR)]);
               cmsLog_debug("dslLink->ifname=%s, index=%d", dslLink->X_BROADCOM_COM_IfName, index);
               intfArray[index] = 1;            /* mark the interface used */
            }
            cmsObj_free((void **) &dslLink);
         }
         prefix = ATM_IFC_STR;
      }
      break;
      
#ifdef DMP_PTMWAN_1
      case PTM_EOA:
      {
         WanPtmLinkCfgObject *ptmLink=NULL;
         UBOOL8 isAtm = FALSE;
         InstanceIdStack wanDevIid;

         if (rutDsl_getDslWanDevIidStack(isAtm, &wanDevIid) == FALSE)
         {
            cmsLog_error("Fail to get WanDev iidStack.");
            return CMSRET_INTERNAL_ERROR;
         }         
         while ((index < IFC_WAN_MAX) &&
            (ret = cmsObj_getNextInSubTree(MDMOID_WAN_PTM_LINK_CFG, &wanDevIid, &iidStack, (void **) &ptmLink)) == CMSRET_SUCCESS)
         {
            if (ptmLink->X_BROADCOM_COM_IfName == NULL)
            {
               /* this is one we just created and is NULL, so break */
               cmsObj_free((void **) &ptmLink);
               break;
            }
            else
            {
               index = atoi(&(ptmLink)->X_BROADCOM_COM_IfName[strlen(PTM_IFC_STR)]);
               cmsLog_debug("ptmLink->ifname=%s, index=%d", ptmLink->X_BROADCOM_COM_IfName, index);
               intfArray[index] = 1;            /* mark the interface used */
            }
            cmsObj_free((void **) &ptmLink);
         }
         prefix = PTM_IFC_STR;
         
      }
      break;
#endif

      case ATM_IPOA:
      {
         WanDslLinkCfgObject *dslLink=NULL;
        UBOOL8 isAtm = TRUE;
         InstanceIdStack wanDevIid;

         if (rutDsl_getDslWanDevIidStack(isAtm, &wanDevIid) == FALSE)
         {
            cmsLog_error("Fail to get WanDev iidStack.");
            return CMSRET_INTERNAL_ERROR;
         }
         while ((index < IFC_WAN_MAX) &&
            (ret = cmsObj_getNextInSubTree(MDMOID_WAN_DSL_LINK_CFG, &wanDevIid, &iidStack, (void **) &dslLink)) == CMSRET_SUCCESS)
         {
            if (dslLink->X_BROADCOM_COM_IfName == NULL)
            {
               /* this is one we just created and is NULL, so break */
               cmsObj_free((void **) &dslLink);
               break;
            }
            else if (strstr(dslLink->X_BROADCOM_COM_IfName, IPOA_IFC_STR))
            {
               index = atoi(&(dslLink)->X_BROADCOM_COM_IfName[strlen(IPOA_IFC_STR)]);
               cmsLog_debug("dslLink->ifname=%s, index=%d", dslLink->X_BROADCOM_COM_IfName, index);
               intfArray[index] = 1;            /* mark the interface used */
            }
            cmsObj_free((void **) &dslLink);
         }
         prefix = IPOA_IFC_STR;
      }
      break;

      default:
      {
         cmsLog_error("Wrong type=%d", ifNameType);
         return CMSRET_INTERNAL_ERROR;
      }
   }

   
   if (index > IFC_WAN_MAX)
   {
      cmsLog_error("Only %d interface allowed", IFC_WAN_MAX);
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      SINT32 i;
      for (i = 0; i < IFC_WAN_MAX; i++)
      {
         cmsLog_debug("intfArray[%d]=%d", i, intfArray[i]);
         if (intfArray[i] == 0)
         {
            sprintf(L2IfName, "%s%d", prefix, i);
            ret = CMSRET_SUCCESS;
            break;
         }
      }
   }

   CMSMEM_REPLACE_STRING_FLAGS(*ifName, L2IfName, mdmLibCtx.allocFlags);

   cmsLog_debug("Get Layer2 ifName=%s", *ifName);

   return ret;
   
}

CmsRet rutDsl_initPPPoA_igd(const InstanceIdStack *iidStack, void * Obj)
{
   SINT32 pid;
   char cmdLine[BUFLEN_256];
   char staticIPAddrFlag[BUFLEN_32];
   char passwordFlag[BUFLEN_32];
   UBOOL8 isPPPoAorIPoA=FALSE;
   UBOOL8 isVCMux;
   XTM_ADDR xtmAddr;
   char l2IfName[CMS_IFNAME_LENGTH]={0};
   SINT32 portId=0;
   SINT32 vpi=0;
   SINT32 vci=0;
   char atmEncap[BUFLEN_16]={0};   
   CmsRet ret=CMSRET_SUCCESS;
   _WanPppConnObject *newObj = (_WanPppConnObject *) Obj;
   char idlelimit[BUFLEN_32] = {0};

   
   cmsLog_debug("Enter");

   memset(&xtmAddr, 0, sizeof(xtmAddr));
   
   /* Need to get layer 2 interface name, connMode and xtmAddr to form the layer 3 name
   * and service name 
   */
   if ((ret = rutWl2_getL2IfName(MDMOID_WAN_PPP_CONN, iidStack, l2IfName)) != CMSRET_SUCCESS)
   {
      return ret;
   }   

   /* Need isVCMux and xtmAddr for pppoa */
   if ((ret = rutWl2_getXtmInfo(MDMOID_WAN_PPP_CONN, 
                               iidStack,
                               &isPPPoAorIPoA,
                               &isVCMux,
                               &xtmAddr)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   portId = PORTMASK_TO_PORTID(xtmAddr.u.Vcc.ulPortMask);
   vpi = xtmAddr.u.Vcc.usVpi;
   vci = xtmAddr.u.Vcc.usVci;

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
      newObj->X_BROADCOM_COM_IfName,
      l2IfName, portId, vpi, vci,
      newObj->username, passwordFlag,
      atmEncap, cmsUtl_pppAuthToNum(newObj->PPPAuthenticationProtocol), staticIPAddrFlag); 

   /* enable ppp debugging if it is selected */
   if (newObj->X_BROADCOM_COM_Enable_Debug)
   {
      strncat(cmdLine, " -d", sizeof(cmdLine)-1);
   }

   /* enable dial-on-demand if it is selected */
   if (!cmsUtl_strcmp(newObj->connectionTrigger, MDMVS_ONDEMAND))
   {
      snprintf(idlelimit, sizeof(idlelimit), " -D -I %d", newObj->idleDisconnectTime);
      strncat(cmdLine, idlelimit, sizeof(cmdLine) -1);
   }

   /* IP extension */
   if (newObj->X_BROADCOM_COM_IPExtension)
   {
      strncat(cmdLine, " -x", sizeof(cmdLine)-1);
   }

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
      newObj->X_BROADCOM_COM_PppdPid = pid;
   }
    
   return ret;

}
#endif /* DMP_ADSLWAN_1 */

CmsRet rutDsl_initIPoA(const InstanceIdStack *iidStack __attribute__((unused)),
                       _WanIpConnObject *newObj)
{
   CmsRet ret=CMSRET_SUCCESS;
   char cmdStr[BUFLEN_128];
   char bCastStr[BUFLEN_24];
      
   if ((ret = rut_getBCastFromIpSubnetMask
      (newObj->externalIPAddress, newObj->subnetMask, bCastStr)) != CMSRET_SUCCESS)
   {  
      return ret;
   }         

   if (rut_waitIntfExists(newObj->X_BROADCOM_COM_IfName))
   {
      snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s %s mtu 1500 netmask %s broadcast %s", 
         newObj->X_BROADCOM_COM_IfName, newObj->externalIPAddress, newObj->subnetMask, bCastStr);
      rut_doSystemAction("rcl", cmdStr);
     
      snprintf(cmdStr, sizeof(cmdStr), "sendarp -s %s -d %s", "br0", "br0");
      rut_doSystemAction("rcl", cmdStr);  
   }
   else
   {
      cmsLog_error("Unable to create %s", newObj->X_BROADCOM_COM_IfName);
      ret = CMSRET_INTERNAL_ERROR;
   }
   
   return ret;
}


int f2DecI(int val, int q)
{
   return (val/q);
}

int f2DecF(int val, int q)
{
   int      sn = val >> 31;
   return (((val ^ sn) - sn) % q);
}

int GetAdsl2Sq(adsl2DataConnectionInfo *p2, int q)
{
   return (0 == p2->L) ? 0 : (((p2->B+1)*p2->M + p2->R)*8*q)/p2->L;
}

int GetRcvRate(adslMibInfo *pMib, int pathId)
{
    return pMib->xdslInfo.dirInfo[0].lpInfo[pathId].dataRate;
}

int GetXmtRate(adslMibInfo *pMib, int pathId)
{
    return pMib->xdslInfo.dirInfo[1].lpInfo[pathId].dataRate;
}

CmsRet rutWan_getAdslMibByIidStack(const InstanceIdStack *iidStack, adslMibInfo *adslMib, UINT32 *line)
{
   long	size = sizeof(adslMibInfo);
   WanDslIntfCfgObject *dslCfgObj;
   UINT32 lineId = 0;

   if (cmsObj_get(MDMOID_WAN_DSL_INTF_CFG, iidStack, OGF_NO_VALUE_UPDATE, (void **)&dslCfgObj) == CMSRET_SUCCESS)
   {
      lineId = dslCfgObj->X_BROADCOM_COM_BondingLineNumber;
      cmsObj_free((void **)&dslCfgObj);
   }

   if (line) 
      *line = lineId;

   return xdslCtl_GetObjectValue(lineId, NULL, 0, (char *)adslMib, &size);
}

#ifdef DMP_ADSLWAN_1
/* 
 * statistics include adslMib.adslStat, adslMib.adslPerfData,  adslMib.adslTxPerfTotal,
 * adslMib.atmStat, adslMib.adslChanIntl, adslMib.adslChanIntlPerfData,
 * adslMib.adslChanFastPerfData, adslMib.adslStatSincePowerOnm adslMib.adslPerfData
 */
CmsRet rutWan_getAdslShowTimeStats(WanDslIntfStatsShowtimeObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutWan_getAdslMibByIidStack(iidStack, &adslMib, NULL) == CMSRET_SUCCESS)
   {
      obj->receiveBlocks = adslMib.adslStat.rcvStat.cntSF;
      obj->transmitBlocks = adslMib.adslStat.xmtStat.cntSF;
      obj->cellDelin = adslMib.adslPerfData.perfSinceShowTime.adslLCDS;
      obj->linkRetrain = adslMib.adslPerfData.failSinceShowTime.adslRetr;
      obj->initErrors = adslMib.adslPerfData.failSinceShowTime.adslInitErr;
      obj->initTimeouts = adslMib.adslPerfData.failSinceShowTime.adslInitTo;
      obj->lossOfFraming = adslMib.adslPerfData.perfSinceShowTime.adslLofs;
      obj->erroredSecs = adslMib.adslPerfData.perfSinceShowTime.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfSinceShowTime.adslSES;
      obj->FECErrors = adslMib.adslPerfData.perfSinceShowTime.adslFECs;
      obj->ATUCFECErrors = adslMib.adslTxPerfSinceShowTime.adslFECs;
      obj->HECErrors = adslMib.atmStat.rcvStat.cntHEC;
      obj->ATUCHECErrors = adslMib.atmStat.xmtStat.cntHEC;
      obj->CRCErrors = adslMib.adslStat.rcvStat.cntSFErr;
      obj->ATUCCRCErrors = adslMib.adslStat.xmtStat.cntSFErr;

      return (CMSRET_SUCCESS);
   }
   else 
   {
      return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
   }
} /* MDMOID_WAN_DSL_INTF_STATS_SHOWTIME */

CmsRet rutWan_getAdslTotalStats(WanDslIntfStatsTotalObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutWan_getAdslMibByIidStack(iidStack, &adslMib, NULL) == CMSRET_SUCCESS)
   {  
      obj->receiveBlocks = adslMib.xdslChanPerfData[0].perfTotal.adslChanReceivedBlks;
      obj->transmitBlocks = adslMib.xdslChanPerfData[0].perfTotal.adslChanTransmittedBlks;
      obj->linkRetrain = adslMib.adslPerfData.failTotal.adslRetr;
      obj->initErrors = adslMib.adslPerfData.failTotal.adslInitErr;
      obj->initTimeouts = adslMib.adslPerfData.failTotal.adslInitTo;
      obj->lossOfFraming = adslMib.adslPerfData.perfTotal.adslLofs;
      obj->ATUCFECErrors = adslMib.adslStatSincePowerOn.xmtStat.cntRSCor;
      obj->FECErrors = adslMib.adslPerfData.perfTotal.adslFECs;
      obj->X_BROADCOM_COM_RxRsUncorrectable = adslMib.xdslStat[0].rcvStat.cntRSUncor;
      obj->X_BROADCOM_COM_TxRsUncorrectable = 0;
      obj->X_BROADCOM_COM_RxRsCorrectable = adslMib.xdslStat[0].rcvStat.cntRSCor;;
      obj->X_BROADCOM_COM_TxRsCorrectable = adslMib.xdslStat[0].xmtStat.cntRSCor;
      obj->X_BROADCOM_COM_RxRsWords = adslMib.xdslStat[0].rcvStat.cntRS;
      obj->X_BROADCOM_COM_TxRsWords = adslMib.xdslStat[0].xmtStat.cntRS;
      obj->HECErrors = adslMib.atmStatSincePowerOn.rcvStat.cntHEC;
      obj->ATUCHECErrors = adslMib.atmStatSincePowerOn.xmtStat.cntHEC;
      obj->CRCErrors = adslMib.adslStatSincePowerOn.rcvStat.cntSFErr;
      obj->ATUCCRCErrors = adslMib.adslStatSincePowerOn.xmtStat.cntSFErr;
      obj->cellDelin = adslMib.adslPerfData.perfTotal.adslLCDS;
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
      obj->X_BROADCOM_COM_HECErrors_2 = adslMib.atmStat2lp[1].xmtStat.cntHEC;
      obj->X_BROADCOM_COM_ATUCHECErrors_2 = adslMib.atmStat2lp[1].rcvStat.cntHEC;
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
      obj->X_BROADCOM_COM_HECErrors_2 = 0;
      obj->X_BROADCOM_COM_ATUCHECErrors_2 = 0;
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
      obj->erroredSecs = adslMib.adslPerfData.perfTotal.adslESs;
      obj->X_BROADCOM_COM_UpstreamEs = adslMib.adslTxPerfTotal.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfTotal.adslSES;
      obj->X_BROADCOM_COM_UpstreamSes = adslMib.adslTxPerfTotal.adslSES;
      obj->X_BROADCOM_COM_UpstreamUas = adslMib.adslTxPerfTotal.adslUAS;
      obj->X_BROADCOM_COM_DownstreamUas = adslMib.adslPerfData.perfTotal.adslUAS;
   }

   return (CMSRET_SUCCESS);
} /* rutWan_getTotalStats */
#endif /* DMP_ADSLWAN_1 */

CmsRet rutWan_clearAdslTotalStats(UINT32 lineId)
{
   CmsRet ret;

   if ((ret = xdslCtl_ResetStatCounters(lineId)) != CMSRET_SUCCESS)
   {
      cmsLog_debug("error reseting ADSL stats, lineId=%d", lineId);
   }
   return (ret);
}


/** Return TRUE if this is an ATM connection.
 * 
 * Algorithm suggested by Tony Tran.
 */
UBOOL8 isAtmConnection(const adslMibInfo *pAdslMib)
{
   UBOOL8 isAtm=TRUE;  /* assume an ATM connection by default */

#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
   UBOOL8 isVdsl=(pAdslMib->adslConnection.modType == kVdslModVdsl2);
   cmsLog_debug("isVdsl=%d", isVdsl);
   cmsLog_debug("MIB rcv2Info %d %d", pAdslMib->vdslInfo[0].rcv2Info.tmType[0], pAdslMib->vdslInfo[0].rcv2Info.tmType[1]);
   cmsLog_debug("MIB xmt2Info %d %d", pAdslMib->vdslInfo[0].xmt2Info.tmType[0], pAdslMib->vdslInfo[0].xmt2Info.tmType[1]);

   /* a 1 in tmType[0] means DPAPI_DATA_ATM */
   /* a 4 in tmType[0] means DPAPI_DATA_NITRO - ATM with header compression, possible with Broadcom CO */
   if (isVdsl)
   {
      if ((pAdslMib->vdslInfo[0].xmt2Info.tmType[0] != 1) &&
          (pAdslMib->vdslInfo[0].xmt2Info.tmType[0] != 4))
      {
         /* this is a PTM connection */
         isAtm = FALSE;
      }
   }
   else
#endif /* defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1) */
   {
      UINT8 connType=pAdslMib->adsl2Info2lp[0].xmtChanInfo.connectionType;

      cmsLog_debug("adsl connType=%d", connType);
      if ((connType != 1) && (connType != 4))
      {
         /* this is a PTM connection */
         isAtm = FALSE;
      }
   }

   cmsLog_debug("isAtm=%d", isAtm);
   return isAtm;
}


#ifdef DMP_ADSLWAN_1
CmsRet rutDsl_getIntfInfo(WanDslIntfCfgObject *obj, const InstanceIdStack *iidStack)
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
#endif
   UBOOL8 appendAnnex = FALSE;

   if (obj == NULL)
   {
      return ret;
   }

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   memset(&adslConnInfo, 0, sizeof(ADSL_CONNECTION_INFO));
   ret = xdslCtl_GetConnectionInfo(obj->X_BROADCOM_COM_BondingLineNumber, &adslConnInfo);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("xdslCtl_GetConnectionInfo failed, ret=%d", ret);
      return ret;
   }

#ifdef SUPPORT_VECTORINGD
   obj->errorSamplesAvailable = adslConnInfo.errorSamplesAvailable;
#endif
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
      if ((cmsUtl_strcmp(obj->status,MDMVS_NOSIGNAL) == 0) ||
          (cmsUtl_strcmp(obj->status,MDMVS_DISABLED) == 0))
      {
         return CMSRET_SUCCESS_OBJECT_UNCHANGED;
      }
      else
      {
         cmsLog_debug("BCM_XDSL_LINK_DOWN (1)");
         /* TR69 does not define a "down" state.  Is "NoSignal" the right thing here? */
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_NOSIGNAL, mdmLibCtx.allocFlags);
         return CMSRET_SUCCESS;
      }
   }
   else if (xdslStatus == BCM_ADSL_TRAINING_G994)
   {
      /* I'm just guessing that this XDSL state maps to initializing */
      cmsLog_debug("BCM_XDSL_TRAINING_G994 (5)");
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_INITIALIZING, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }
   else if (xdslStatus >= BCM_ADSL_TRAINING_G992_EXCHANGE && xdslStatus <= BCM_ADSL_TRAINING_G993_STARTED)
   {
      /* I'm just guessing that these XDSL states map to establishing link */
      cmsLog_debug("xdsl training %d", xdslStatus);
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_ESTABLISHINGLINK, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }
   else if (xdslStatus != BCM_ADSL_LINK_UP)
   {
      /* TR69 also specifies error status.  Don't know which XDSL state
       * corresponds to that. */
      cmsLog_debug("some other kind of status %d", xdslStatus);
      if (cmsUtl_strcmp(obj->status,MDMVS_UP) == 0)
          CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_INITIALIZING, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }


   if (xdslStatus == BCM_ADSL_LINK_UP)
   {
      long adslMibSize=sizeof(adslMib);

      cmsLog_debug("BCM_XDSL_LINK_UP (0)");

      if (CMSRET_SUCCESS != xdslCtl_GetObjectValue(obj->X_BROADCOM_COM_BondingLineNumber, NULL, 0, (char *) &adslMib, &adslMibSize))
      {
         cmsLog_error("could not get MIB for line %d", obj->X_BROADCOM_COM_BondingLineNumber);
         return CMSRET_INTERNAL_ERROR;
      }

      if(adslMib.adslTrainingState != kAdslTrainingConnected) 
      {
          REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_INITIALIZING, mdmLibCtx.allocFlags);
          return CMSRET_SUCCESS;
      }

      /*
       * If we have 2 DSL WANDevices, one will have DSLIntfCfg.LinkEncapsulationUsed
       * set to ATM, and one set to PTM.  When the system first boots, we don't
       * know if the DSL line will train to ATM or PTM.  But once the DSL line comes
       * up and we know whether we are in ATM or PTM, we want to set the DSLLinkCfg.status
       * up on correct WANDevice.DSLIntfCfg.
       */
      if (isAtmConnection(&adslMib))
      {
         /*
          * We are in ATM mode.  If this WANDevice.DSLIntfCfg is the
          * PTM mode one, we set its status to DISABLED.  When training, this
          * instance was also updated with the training status.
          */
         if (0 == cmsUtl_strcmp(obj->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM))
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
            return CMSRET_SUCCESS;
         }
      }
      else
      {
         /*
          * we are up in PTM mode.  If this WANDevice.DSLIntfCfg is not 
          * the PTM mode one, we set its status to DISABLED.  When training, this
          * instance was also updated with the training status.
          */
         if (cmsUtl_strcmp(obj->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM))
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
            return CMSRET_SUCCESS;
         }
      }

      
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
      
   
      switch (adslMib.adslConnection.modType)
      {
      case kAdslModGdmt:
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->modulationType,MDMVS_ADSL_G_DMT,mdmLibCtx.allocFlags);
         break;
      case kAdslModT1413:
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->modulationType,MDMVS_ADSL_ANSI_T1_413,mdmLibCtx.allocFlags);
         break;
      case kAdslModGlite:
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->modulationType,MDMVS_ADSL_G_LITE,mdmLibCtx.allocFlags);
         break;
      case kAdslModAdsl2:
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->modulationType,MDMVS_ADSL_G_DMT_BIS,mdmLibCtx.allocFlags);
         xDsl2Mode = 1;
         break;
      case kAdslModAdsl2p:
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->modulationType,MDMVS_ADSL_2PLUS,mdmLibCtx.allocFlags);
         xDsl2Mode = 1;
         break;
      case kAdslModReAdsl2:
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->modulationType,MDMVS_ADSL_RE_ADSL,mdmLibCtx.allocFlags);
         xDsl2Mode = 1;
         break;
      case kVdslModVdsl2:
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->modulationType,MDMVS_VDSL2,mdmLibCtx.allocFlags);
         xDsl2Mode = 1;
         break;
#ifdef SUPPORT_DSL_GFAST
      case kXdslModGfast:
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->modulationType,MDMVS_G_FAST,mdmLibCtx.allocFlags);
         xDsl2Mode = 1;
         break;
#endif
      default:
         break;
      } /* modulationType */

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

      switch (adslMib.adslConnection.chType)
      {
      case kAdslFastChannel:
         cmsLog_debug("kAdslFastChannel");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->dataPath,MDMVS_FAST,mdmLibCtx.allocFlags);
         break;
      case kAdslIntlChannel:
         cmsLog_debug("kAdslIntlChannel");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->dataPath,MDMVS_INTERLEAVED,mdmLibCtx.allocFlags);
         break;
      default:
         cmsLog_debug("adslMib.adslConnection.chType %d",adslMib.adslConnection.chType);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->dataPath,MDMVS_NONE,mdmLibCtx.allocFlags);
      } /* dataPath */
      
      if( adslMib.adslConnection.modType < kAdslModAdsl2 ) {
         if( kAdslTrellisOn == adslMib.adslConnection.trellisCoding ) {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisD, MDMVS_ON, mdmLibCtx.allocFlags);
         }
         else {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisD, MDMVS_OFF, mdmLibCtx.allocFlags);
         }
      }
      else {
         if( adslMib.adslConnection.trellisCoding2 & kAdsl2TrellisRxEnabled ) {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisD, MDMVS_ON, mdmLibCtx.allocFlags);
         }
         else {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisD, MDMVS_OFF, mdmLibCtx.allocFlags);
         }
         if( adslMib.adslConnection.trellisCoding2 & kAdsl2TrellisTxEnabled ) {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisU, MDMVS_ON, mdmLibCtx.allocFlags);
         }
         else {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_TrellisU, MDMVS_OFF, mdmLibCtx.allocFlags);
         }
      }

      // [JIRA SWBCACPE-10306]: TR-098 requires
      // upstreamCurrRate, downstreamCurrRate, upstreamMaxRate, downstreamMaxRate
      // in Kbps instead of Bps
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

      pwrState = adslMib.xdslInfo.pwrState;
      if ( 0 == pwrState ) {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_LinkPowerState,MDMVS_L0,mdmLibCtx.allocFlags);
      }
      else if ( 2 == pwrState ) {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_LinkPowerState,MDMVS_L2,mdmLibCtx.allocFlags);
      }
      else {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->X_BROADCOM_COM_LinkPowerState,MDMVS_L3,mdmLibCtx.allocFlags);
      }
      
      cmsLog_debug("Power state: %s",obj->X_BROADCOM_COM_LinkPowerState);
      
      obj->downstreamNoiseMargin = adslMib.adslPhys.adslCurrSnrMgn;
      obj->upstreamNoiseMargin = adslMib.adslAtucPhys.adslCurrSnrMgn;
      obj->downstreamAttenuation = adslMib.adslPhys.adslCurrAtn;
      obj->upstreamAttenuation = adslMib.adslAtucPhys.adslCurrAtn;
      obj->downstreamPower = adslMib.adslAtucPhys.adslCurrOutputPwr;
      obj->upstreamPower = adslMib.adslPhys.adslCurrOutputPwr;
      // [JIRA SWBCACPE-10306]: TR-098 requires
      // upstreamMaxRate, downstreamMaxRate
      // in Kbps instead of Bps
      obj->downstreamMaxRate = adslMib.adslPhys.adslCurrAttainableRate / 1000; 
      obj->upstreamMaxRate = adslMib.adslAtucPhys.adslCurrAttainableRate / 1000;
      
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
            snprintf(value, sizeof(value), "%3.4f", (pTxFramingParam->S.denom) ? (float)pTxFramingParam->S.num/(float)pTxFramingParam->S.denom : 0);
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
            pRxFramingParam = &adslMib.xdslInfo.dirInfo[0].lpInfo[1];
            pTxFramingParam = &adslMib.xdslInfo.dirInfo[1].lpInfo[1];
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
            snprintf(value, sizeof(value), "%3.4f", (pTxFramingParam->S.denom) ? (float)pTxFramingParam->S.num/(float)pTxFramingParam->S.denom : 0);
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
#ifdef SUPPORT_DSL_GFAST
            if(kXdslModGfast == adslMib.adslConnection.modType)
            {
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
                  if(0 != pVdsl2Info->vdsl2Profile)   /* Old G.fast PHY reports 0 as 106a profile */
                     cmsLog_error("unrecognized profile 0x%x (set to 106a)", pVdsl2Info->vdsl2Profile);
                  strcpy(value,"106a");
                  break;
               }
            }
            else
#endif
            {
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
                  cmsLog_error("unrecognized profile 0x%x (set to empty string)", pVdsl2Info->vdsl2Profile);
                  strcpy(value,"");
                  break;
               } /* switch vdsl2Profile */
            }
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
#ifdef SUPPORT_DSL_GFAST
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
      obj->ATURANSIStd = 0;
      obj->ATURANSIRev = 0;
      obj->ATUCANSIStd = 0;
      obj->ATUCANSIRev = 0;
      obj->lastShowtimeStart = adslMib.adslPerfData.adslSinceLinkTimeElapsed;
      obj->currentDayStart = adslMib.adslPerfData.adslPerfCurr1DayTimeElapsed;
      obj->quarterHourStart = adslMib.adslPerfData.adslPerfCurr15MinTimeElapsed;
      
      obj->TRELLISds = adslMib.adslConnection.trellisCoding2 & kAdsl2TrellisRxEnabled;
      obj->TRELLISus = adslMib.adslConnection.trellisCoding2 & kAdsl2TrellisTxEnabled;

      switch (adslMib.adslConnection.modType)
      {
      case kAdslModGdmt:
         appendAnnex=TRUE;
         strcpy(value,"G.992.1");
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
         appendAnnex = TRUE;
         strcpy(value,"G.992.3");
         break;
      case kAdslModAdsl2p:
         appendAnnex = TRUE;
         strcpy(value,"G.992.5");
         break;
      case kAdslModReAdsl2:
         strcpy(value,"G.992.3_Annex_L");
         break;
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
      case kVdslModVdsl2:
         appendAnnex = TRUE;
         strcpy(value,"G.993.2");
         break;
#endif
      } /* modType */
      if (appendAnnex)
      {
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
      }
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->standardUsed,value,mdmLibCtx.allocFlags);
   } /* cmsAdsl_getStatistics OK */
   
   cmsLog_debug("End: ret %d",ret);
   
   return (ret);
}


CmsRet rutWan_getAdslBertInfo_igd(void *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   CmsRet ret = CMSRET_SUCCESS;
   WanBertTestObject *bertObj = (WanBertTestObject*)obj;

   if ((ret = rutWan_getAdslMibByIidStack(iidStack, &adslMib, NULL)) == CMSRET_SUCCESS)
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


CmsRet rutWan_setAdslBertInfo_igd(void *new, const void *curr, const InstanceIdStack *iidStack)
{
   WanBertTestObject *newObj = (WanBertTestObject*)new;
   WanBertTestObject *currObj = (WanBertTestObject*)curr;
   CmsRet ret = CMSRET_SUCCESS;
   WanDslIntfCfgObject *dslCfgObj;
   UINT32 lineId = 0;

   if (cmsObj_get(MDMOID_WAN_DSL_INTF_CFG, iidStack, OGF_NO_VALUE_UPDATE, (void **)&dslCfgObj) == CMSRET_SUCCESS)
   {
      lineId = dslCfgObj->X_BROADCOM_COM_BondingLineNumber;
      cmsObj_free((void **) &dslCfgObj);
   }

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

CmsRet rutWan_getAdslCurrentDayStats(WanDslIntfStatsCurrentDayObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutWan_getAdslMibByIidStack(iidStack, &adslMib, NULL) == CMSRET_SUCCESS)
   {  
      /* Values pending on driver */
      obj->linkRetrain = 0;
      obj->initErrors = 0;
      obj->initTimeouts = 0;
      obj->HECErrors = 0;
      obj->ATUCHECErrors = 0;

      obj->cellDelin = adslMib.adslPerfData.perfCurr1Day.adslLCDS;
      obj->lossOfFraming = adslMib.adslPerfData.perfCurr1Day.adslLofs;
      obj->erroredSecs = adslMib.adslPerfData.perfCurr1Day.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfCurr1Day.adslSES;
      obj->FECErrors = adslMib.adslPerfData.perfCurr1Day.adslFECs;
      obj->ATUCFECErrors = adslMib.adslTxPerfCur1Day.adslFECs;
      obj->ATUCCRCErrors = adslMib.xdslChanPerfData[0].perfCurr1Day.adslChanTxCRC;
      obj->CRCErrors = adslMib.xdslChanPerfData[0].perfCurr1Day.adslChanUncorrectBlks;
      obj->receiveBlocks = adslMib.xdslChanPerfData[0].perfCurr1Day.adslChanReceivedBlks;
      obj->transmitBlocks = adslMib.xdslChanPerfData[0].perfCurr1Day.adslChanTransmittedBlks;

      return (CMSRET_SUCCESS);
   }
   else 
   {
      return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
   }
} /* MDMOID_WAN_DSL_INTF_STATS_CURRENTDAY */

CmsRet rutWan_getAdslQuarterHourStats(WanDslIntfStatsQuarterHourObject *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;

   cmsLog_debug("Entered");

   if (rutWan_getAdslMibByIidStack(iidStack, &adslMib, NULL) == CMSRET_SUCCESS)
   {  
      /* Values pending on driver */
      obj->linkRetrain = 0;
      obj->initErrors = 0;
      obj->initTimeouts = 0;
      obj->HECErrors = 0;
      obj->ATUCHECErrors = 0;

      obj->cellDelin = adslMib.adslPerfData.perfCurr15Min.adslLCDS;
      obj->lossOfFraming = adslMib.adslPerfData.perfCurr15Min.adslLofs;
      obj->erroredSecs = adslMib.adslPerfData.perfCurr15Min.adslESs;
      obj->severelyErroredSecs = adslMib.adslPerfData.perfCurr15Min.adslSES;
      obj->FECErrors = adslMib.adslPerfData.perfCurr15Min.adslFECs;
      obj->ATUCFECErrors = adslMib.adslTxPerfCur15Min.adslFECs;
      obj->ATUCCRCErrors = adslMib.xdslChanPerfData[0].perfCurr15Min.adslChanTxCRC;
      obj->CRCErrors = adslMib.xdslChanPerfData[0].perfCurr15Min.adslChanUncorrectBlks;
      obj->receiveBlocks = adslMib.xdslChanPerfData[0].perfCurr15Min.adslChanReceivedBlks;
      obj->transmitBlocks = adslMib.xdslChanPerfData[0].perfCurr15Min.adslChanTransmittedBlks;;

      return (CMSRET_SUCCESS);
   }
   else 
   {
      return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
   }
} /* MDMOID_WAN_DSL_INTF_STATS_QUARTER_HOUR */

#endif /* DMP_ADSLWAN_1 */

#ifdef SUPPORT_DSL_BONDING

UBOOL8 rutDsl_isDslBondingEnabled(void)
{
   UBOOL8 enabled=FALSE;

   qdmDsl_isDslBondingEnabled(&enabled);

   return(enabled);
}

#endif /* SUPPORT_DSL_BONDING */

#define MAX_XDSL_DATA_LENGTH  61430   /* max length defined in the spec. */
#ifdef DMP_DSLDIAGNOSTICS_1

CmsRet rutWan_getAdslLoopDiagStatus_igd(void *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   WanDslDiagObject *dslDiagObj = (WanDslDiagObject*)obj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   int testCompleted = DIAG_DSL_LOOPBACK_ERROR;

   if (rutWan_getAdslMibByIidStack(iidStack, &adslMib, NULL) != CMSRET_SUCCESS)
   {
      cmsLog_debug("Unable to read diagnosticsState from driver");
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dslDiagObj->loopDiagnosticsState,MDMVS_ERROR_INTERNAL,mdmLibCtx.allocFlags);
      return (ret);
   }
   else
   {
      /* reading for indication from driver that test is completed */
      testCompleted = adslMib.adslPhys.adslLDCompleted;
      
      cmsLog_debug(" testCompleted %d (2== completed)", testCompleted);
      
      if ((testCompleted == DIAG_DSL_LOOPBACK_ERROR) || (testCompleted == DIAG_DSL_LOOPBACK_ERROR_BUT_RETRY))
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dslDiagObj->loopDiagnosticsState,MDMVS_ERROR_INTERNAL,mdmLibCtx.allocFlags);
         ret = CMSRET_SUCCESS;
      }
      else if (testCompleted == DIAG_DSL_LOOPBACK_SUCCESS)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dslDiagObj->loopDiagnosticsState,MDMVS_COMPLETE,mdmLibCtx.allocFlags);
         ret = CMSRET_SUCCESS;
      }  /* get adslLdCompleted */
   } /* read from ADSL driver OK */
   return (ret);
}

CmsRet rutWan_getAdslLoopDiagResultAndLinkUp(DslLoopDiagData *pResult, UINT32 lineId)
{
   adslMibInfo adslMib;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   char   oidStr[] = { kOidAdslPrivate, 0 };
   char   oidStr1[] = { kOidAdslPrivate, kOidAdslPrivExtraInfo, 0 };
   UINT8 gFactor = 1;
   long len;
   long	size = sizeof(adslMib);

   if ((ret = xdslCtl_GetObjectValue(lineId, NULL, 0, (char *)&adslMib, &size)) != CMSRET_SUCCESS)
   {
      cmsLog_debug("Unable to read result from driver");
      return (ret);
   }
   else
   {
      pResult->testCompleted = TRUE;

      /* get G-factor objects --VDSL only */
      pResult->HLINGds = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSds;
      pResult->HLINGus = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSus; 
      pResult->HLOGGds = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSds;
      pResult->HLOGGus = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSus;
      pResult->QLNGds = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSds;
      pResult->QLNGus = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSus;
      pResult->SNRGds = adslMib.gFactors.Gfactor_MEDLEYSETds;;
      pResult->SNRGus = adslMib.gFactors.Gfactor_MEDLEYSETus;
      
#if defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1)
      if (adslMib.adslConnection.modType == kVdslModVdsl2)
      {
         /* per-band objects --VDSL only */ 
         oidStr[1] = kOidAdslPrivLATNdsperband;
         len = sizeof(pResult->LATNds);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->LATNds, &len);

         oidStr[1] = kOidAdslPrivLATNusperband;
         len = sizeof(pResult->LATNus);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->LATNus, &len);

         oidStr[1] = kOidAdslPrivSATNdsperband;
         len = sizeof(pResult->SATNds);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->SATNds, &len);

         oidStr[1] = kOidAdslPrivSATNusperband;
         len = sizeof(pResult->SATNus);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->SATNus, &len);

         oidStr[1] = kOidAdslPrivSNRMdsperband;
         len = sizeof(pResult->SNRMpbds);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->SNRMpbds, &len);

         oidStr[1] = kOidAdslPrivSNRMusperband;
         len = sizeof(pResult->SNRMpbus);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->SNRMpbus, &len);
      } /* vdsl only */
#endif /* defined(DMP_VDSL2WAN_1) || defined(DMP_X_BROADCOM_COM_VDSL2WAN_1) */

      pResult->ACTPSDds = adslMib.xdslAtucPhys.actualPSD;
      pResult->ACTPSDus = adslMib.xdslPhys.actualPSD;

      pResult->SNRMTds = adslMib.adslPhys.SNRMT;
      pResult->SNRMTus = adslMib.adslAtucPhys.SNRMT;
      pResult->QLNMTds = adslMib.adslPhys.QLNMT;
      pResult->QLNMTus = adslMib.adslAtucPhys.QLNMT;
      pResult->HLOGMTds = adslMib.adslPhys.HLOGMT;
      pResult->HLOGMTus = adslMib.adslAtucPhys.HLOGMT;
      
      pResult->HLINSCds = adslMib.adslAtucPhys.adslHlinScaleFactor;
      pResult->ACTATPus = adslMib.adslPhys.adslCurrOutputPwr;
      pResult->ACTATPds = adslMib.adslAtucPhys.adslCurrOutputPwr;

      /* first we need to set the Gfactor */
      oidStr1[2] = kOidAdslPrivSetFlagActualGFactor;
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == CMSRET_SUCCESS) 
      {
         /* get HLINpsds */         
         oidStr[1] = kOidAdslPrivChanCharLinDsPerToneGroup;
         len = sizeof(pResult->HLINpsds);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->HLINpsds,&len);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == CMSRET_SUCCESS) 
      {
         /* get HLINpsus */         
         oidStr[1] = kOidAdslPrivChanCharLinUsPerToneGroup;
         len = sizeof(pResult->HLINpsus);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->HLINpsus,&len);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == CMSRET_SUCCESS) 
      {
         /* get QLNpsds */
         oidStr[1] = kOidAdslPrivQuietLineNoiseDsPerToneGroup;
         len = sizeof(pResult->QLNpsds);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->QLNpsds,&len);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == CMSRET_SUCCESS) 
      {  
         /* get QLNpsus */
         oidStr[1] = kOidAdslPrivQuietLineNoiseUsPerToneGroup;
         len = sizeof(pResult->QLNpsus);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->QLNpsus,&len);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == CMSRET_SUCCESS) 
      {
         /* get SNRpsds */
         oidStr[1] = kOidAdslPrivSNRDsPerToneGroup;
         len = sizeof(pResult->SNRpsds);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->SNRpsds,&len);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == CMSRET_SUCCESS) 
      {
         /* get SNRpsus */
         oidStr[1] = kOidAdslPrivSNRUsPerToneGroup;
         len = sizeof(pResult->SNRpsus);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->SNRpsus,&len);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == CMSRET_SUCCESS) 
      {
         /* get HLOGpsds */
         oidStr[1] = kOidAdslPrivChanCharLogDsPerToneGroup;
         len = sizeof(pResult->HLOGpsds);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->HLOGpsds,&len);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == CMSRET_SUCCESS) 
      {  
         /* get HLOGusds */
         oidStr[1] = kOidAdslPrivChanCharLogUsPerToneGroup;
         len = sizeof(pResult->HLOGpsus);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->HLOGpsus,&len);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == CMSRET_SUCCESS) 
      {
         /* get BITpsds */
         oidStr[1] = kOidAdslPrivBitAllocDsPerToneGroup;
         len = sizeof(pResult->BITSpsds);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->BITSpsds,&len);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == CMSRET_SUCCESS) 
      {
         /* get GAINSpsds */
         oidStr[1] = kOidAdslPrivGainDsPerToneGroup;
         len = sizeof(pResult->GAINSpsds);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)&pResult->GAINSpsds,&len);
      } /* set g factor first */
      ret = CMSRET_SUCCESS;
   }    /* finish reading statistics, bring the link back up */
   devCtl_adslConnectionStart();

   return ret;
}
#endif /* DMP_DSLDIAGNOSTICS_1 */

#ifdef DMP_VDSL2WAN_1
CmsRet rutWan_getVceMacAddress(void* vceMacAddress __attribute__((unused)),
                               int lineId __attribute__((unused)))
{
#ifdef SUPPORT_VECTORINGD
  adslMibInfo adslMib;
  CmsRet ret = CMSRET_REQUEST_DENIED;
  char oidStr[] = { kOidAdslPrivate, 0 };
  long len;
  long adslMibSize=sizeof(adslMib);
  CmsRet res;

  if (CMSRET_SUCCESS ==xdslCtl_GetObjectValue(lineId, NULL, 0, (char *) &adslMib, &adslMibSize))
  {
    if (adslMib.adslConnection.modType == kVdslModVdsl2)
    {
      cmsLog_debug("vectSM.state %d",adslMib.vectSM.state);
      if ((adslMib.vectSM.state==VECT_FULL) || (adslMib.vectSM.state==VECT_RUNNING))
      {
        len = sizeof(VceMacAddress);
        oidStr[1] = kOIdAdslPrivGetVceMacAddress;
        res = xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)vceMacAddress, &len);
        ret = CMSRET_SUCCESS;
      }
    else
      cmsLog_error("rutWan_getVceMacAddress: not possible in vectSM.state %d",adslMib.vectSM.state);
    }
  else
    cmsLog_error("rutWan_getVceMacAddress: not possible in modType=%d",adslMib.adslConnection.modType);
  }
  else
  {
    cmsLog_error("rutWan_getVceMacAddress: MIB ERROR");
    ret = CMSRET_OBJECT_NOT_FOUND;
  }
  return ret;
#else
  return CMSRET_OBJECT_NOT_FOUND;
#endif
}

CmsRet rutWan_getErrorSamples(void *obj __attribute__((unused)),
                              long *len __attribute__((unused)),
                              int lineId __attribute__((unused)))
{
#ifdef SUPPORT_VECTORINGD
  adslMibInfo adslMib;
  CmsRet ret = CMSRET_REQUEST_DENIED;
  char oidStr[] = { kOidAdslPrivate, 0 };
  char oidStr1[] = { kOidAdslPrivate, kOidAdslPrivExtraInfo, 0 };
  UINT8 gFactor = 1;
  long len2 = 0;
  long adslMibSize=sizeof(adslMib);
  CmsRet res;

  *len = sizeof(VectorErrorSample);
  
  if (CMSRET_SUCCESS==xdslCtl_GetObjectValue(lineId, NULL, 0, (char *) &adslMib, &adslMibSize))
  {
    if (adslMib.adslConnection.modType == kVdslModVdsl2)
    {
      if (adslMib.vectData.esCtrl.readIdx != adslMib.vectData.esCtrl.writeIdx)
      {
        cmsLog_debug("\tcopying error samples");
        oidStr[1] = kOIdAdslPrivGetVectErrorSamples;
        res = xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)obj, len);
        oidStr1[2] = kOIdAdslPrivIncrementErrorSamplesReadPtr;
        res = xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len2);
        ret = CMSRET_SUCCESS;
      }
    }
  }
  else
    ret = CMSRET_OBJECT_NOT_FOUND;
  return ret;
#else
  return CMSRET_OBJECT_NOT_FOUND;
#endif
}

CmsRet rutWan_getAdslTestParamsInfo(void *obj, const InstanceIdStack *iidStack)
{
   adslMibInfo adslMib;
   long len;
   char oidStr[] = { kOidAdslPrivate, 0 };
   char   oidStr1[] = { kOidAdslPrivate, kOidAdslPrivExtraInfo, 0 };
   UINT16 bandObjData[NUM_BAND];
   SINT16 subcarrierData[NUM_TONE_GROUP];
   UINT8 gFactor = 1;
   WanDslTestParamsObject *testParamObj = (WanDslTestParamsObject*)obj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   char dataStr[BUFLEN_64];
   UINT32 lineId;

   if (rutWan_getAdslMibByIidStack(iidStack, &adslMib, &lineId) == CMSRET_SUCCESS)
   {
         /* get G-factor objects --VDSL only */
         testParamObj->HLOGGds = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSds;
         testParamObj->HLOGGus = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSus;
         testParamObj->QLNGds = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSds;
         testParamObj->QLNGus = adslMib.gFactors.Gfactor_SUPPORTERCARRIERSus;
         testParamObj->SNRGds = adslMib.gFactors.Gfactor_MEDLEYSETds;
         testParamObj->SNRGus = adslMib.gFactors.Gfactor_MEDLEYSETus;
            
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
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) CMSRET_SUCCESS)
      {
         /* get QLNpsds */
         oidStr[1] = kOidAdslPrivQuietLineNoiseDsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->QLNpsds,subcarrierData,(char*)"QLNpsds",MAX_QLN_STRING);
      }

      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) CMSRET_SUCCESS)
      {
         /* get QLNpsus */
         oidStr[1] = kOidAdslPrivQuietLineNoiseUsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->QLNpsus,subcarrierData,(char*)"QLNpsus",MAX_QLN_STRING);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) CMSRET_SUCCESS)
      {
         /* get SNR */
         oidStr[1] = kOidAdslPrivSNRDsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->SNRpsds,subcarrierData,(char*)"SNRpsds",MAX_PS_STRING);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) CMSRET_SUCCESS)
      {
         oidStr[1] = kOidAdslPrivSNRUsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->SNRpsus,subcarrierData,(char*)"SNRpsus",MAX_PS_STRING);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) CMSRET_SUCCESS)
      {
         /* get HLOG */
         oidStr[1] = kOidAdslPrivChanCharLogDsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->HLOGpsds,subcarrierData,(char*)"HLOGpsds",MAX_LOGARITHMIC_STRING);
      }
      len = 1;
      if((xdslCtl_SetObjectValue(lineId, oidStr1, sizeof(oidStr1), (char *)&gFactor, &len)) == (CmsRet) CMSRET_SUCCESS)
      {
         oidStr[1] = kOidAdslPrivChanCharLogUsPerToneGroup;
         len = sizeof(subcarrierData);
         xdslCtl_GetObjectValue(lineId, oidStr, sizeof(oidStr), (char *)subcarrierData,&len);
         cmsAdsl_formatSubCarrierDataString(&testParamObj->HLOGpsus,subcarrierData,(char*)"HLOGpsus",MAX_LOGARITHMIC_STRING);
      }
      ret = CMSRET_SUCCESS;
   } /* get mib statistics ok */
   return ret;
}
#endif /* DMP_VDSL2WAN_1 */


#ifdef DMP_X_BROADCOM_COM_SELT_1
CmsRet rutWan_getSeltStatus(void *obj)
{
   SeltCfgObject *seltCfgObj = (SeltCfgObject*)obj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   int testCompleted = 0;

   ret = xdslCtl_isSeltTestComplete(seltCfgObj->lineNumber, &testCompleted);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_debug("Unable to get SELT test status from driver");
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(seltCfgObj->seltTestState,MDMVS_ERROR_INTERNAL,mdmLibCtx.allocFlags);
      return (ret);
   }
   else
   {
      /* reading for indication from driver that test is completed */
      cmsLog_debug(" testCompleted %d ", testCompleted);
         
      if (testCompleted == DIAG_DSL_SELT_STATE_TEST_COMPLETE)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(seltCfgObj->seltTestState,MDMVS_COMPLETE,mdmLibCtx.allocFlags);
         ret = CMSRET_SUCCESS;

         /* restart DSL connection, assuming driver already read and store all the results at this point */
         xdslCtl_ConnectionStart(seltCfgObj->lineNumber);
      }  
   } /* read from ADSL driver OK */
   return (ret);
}
#endif /* DMP_X_BROADCOM_COM_SELT_1 */

#endif /* SUPPORT_DSL */

