/***********************************************************************
 *
 *  Copyright (c) 2008  Broadcom Corporation
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

#include "stl.h"
#include "cms_util.h"
#include "rut_lan.h"
#include "rut_dsl.h"

#ifdef DMP_ADSLWAN_1

CmsRet stl_wanDslIntfCfgObject(_WanDslIntfCfgObject *obj,
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS_OBJECT_UNCHANGED;

   cmsLog_debug("Entered");

   if (!obj->enable)
   {
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
      return CMSRET_SUCCESS;
   }

   /*
    * DslLink is enabled, get status from the kernel.
    */
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend we are up in ATM mode */
   if (!cmsUtl_strcmp(obj->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM))
   {
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
      ret = CMSRET_SUCCESS;
   }
#else

   ret = rutDsl_getIntfInfo(obj, iidStack);
   cmsLog_debug("rutDsl_getIntfInfo returned %d (status=%s)", ret, obj->status);
#endif /* DESKTOP_LINUX */

   return ret;
}

CmsRet stl_wanBertTestObject(_WanBertTestObject *obj __attribute__((unused)),
                   const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS; /* just assume this always changes the object */

   cmsLog_debug("Entered");

#ifndef DESKTOP_LINUX
   ret = rutWan_getAdslBertInfo(obj,iidStack);
#endif /* DESKTOP_LINUX */

   return ret;
}

CmsRet stl_wanDslIntfStatsObject(_WanDslIntfStatsObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wanDslIntfStatsTotalObject(_WanDslIntfStatsTotalObject *obj __attribute__((unused)),
                 const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret;

   if (obj == NULL)
   {
      WanDslIntfCfgObject *dslCfgObj;
      UINT32 lineId = 0;

      if (cmsObj_get(MDMOID_WAN_DSL_INTF_CFG, iidStack, OGF_NO_VALUE_UPDATE, (void **)&dslCfgObj) == CMSRET_SUCCESS)
      {
         lineId = dslCfgObj->X_BROADCOM_COM_BondingLineNumber;
         cmsObj_free((void **)&dslCfgObj);
      }
      ret = rutWan_clearAdslTotalStats(lineId);
   }
   else
   {
      ret = rutWan_getAdslTotalStats(obj,iidStack);
   }
   return ret;
#endif /* DESKTOP_LINUX */
}

CmsRet stl_wanDslIntfStatsShowtimeObject(_WanDslIntfStatsShowtimeObject *obj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret;

   if (obj == NULL)
   {
      WanDslIntfCfgObject *dslCfgObj;
      UINT32 lineId = 0;

      if (cmsObj_get(MDMOID_WAN_DSL_INTF_CFG, iidStack, OGF_NO_VALUE_UPDATE, (void **)&dslCfgObj) == CMSRET_SUCCESS)
      {
         lineId = dslCfgObj->X_BROADCOM_COM_BondingLineNumber;
         cmsObj_free((void **)&dslCfgObj);
      }
      /* same function used to call total and showtime stats */
      ret = rutWan_clearAdslTotalStats(lineId);
   }
   else
   {
      ret = rutWan_getAdslShowTimeStats(obj,iidStack);
   }
   return(ret);
#endif /* DESKTOP_LINUX */
}

CmsRet stl_wanDslIntfStatsCurrentDayObject(_WanDslIntfStatsCurrentDayObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret;

   if (obj == NULL)
   {
      /* there is no function to clear this stats */
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   else
   {

      ret = rutWan_getAdslCurrentDayStats(obj,iidStack);
   }
   return(ret);
#endif
}

CmsRet stl_wanDslIntfStatsQuarterHourObject(_WanDslIntfStatsQuarterHourObject *obj __attribute__((unused)),
           const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret;

   if (obj == NULL)
   {
      /* there is no function to clear this stats */
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
   else
   {

      ret = rutWan_getAdslQuarterHourStats(obj,iidStack);
   }
   return(ret);
#endif
}

#ifdef DMP_VDSL2WAN_1
CmsRet stl_wanDslTestParamsObject(_WanDslTestParamsObject *obj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret;

   if (obj == NULL)
   {
      /* there is no function to clear this stats */
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   else
   {
      ret = rutWan_getAdslTestParamsInfo(obj,iidStack);
   }
   return(ret);
#endif
}
#endif /* DMP_VDSL2WAN_1 */

#ifdef NOT_SUPPORTED

CmsRet stl_wanDslIntfStatsLastShowtimeObject(_WanDslIntfStatsLastShowtimeObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif /* NOT_SUPPORTED */

CmsRet stl_wanDslProprietaryDiagObject(_WanDslProprietaryDiagObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wanDslDiagPlnObject(_WanDslDiagPlnObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wanDslDiagNonLinearityObject(_WanDslDiagNonLinearityObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


#else

/*
 * On non-DSL systems, these stubs must be present, but should be left empty.
 */

CmsRet stl_wanDslIntfCfgObject(_WanDslIntfCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wanDslLinkCfgObject(_WanDslLinkCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wanDslIntfStatsObject(_WanDslIntfStatsObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wanDslIntfStatsTotalObject(_WanDslIntfStatsTotalObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


#endif /* DMP_ADSLWAN_1 */
