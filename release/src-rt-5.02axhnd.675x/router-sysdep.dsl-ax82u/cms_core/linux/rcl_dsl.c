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

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_dsl.h"


#ifdef DMP_ADSLWAN_1

CmsRet rcl_wanDslIntfCfgObject( _WanDslIntfCfgObject *newObj,
                const _WanDslIntfCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   

   if (ADD_NEW(newObj, currObj) && newObj->enable)
   {
      /*
       * This will happen at bootup time.  DSL Initialization and the
       * data model are not well matched at the moment.  We can have 
       * up to 3 DSL WANDevices, WanDevice.1 = ATM DSL, WANDevice.2 = PTM DSL
       * and WANDevice.12 = PTM Bonded DSL.  But we only need to call rutDsl_configUp
       * once, and that single call initialize/bring up ATM, PTM, and bonded PTM.
       */
      if (!mdmShmCtx->dslInitDone)
      {
         ret = rutDsl_configUp(newObj);
         mdmShmCtx->dslInitDone = TRUE;
      }
   }

   else if (ENABLE_EXISTING(newObj, currObj) ||
            rutDsl_isDslConfigChanged(newObj, currObj))
   {
      /* enable existing intf */

      ret = rutDsl_configUp(newObj);
   }
   
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /* Do not configure DSL link if this WANDevice is not XDSL device(ex: EthWan) */
      if ( currObj->enable )
      {
         rutDsl_configDown();
      }
   }

   return ret;
}

CmsRet rcl_wanBertTestObject( _WanBertTestObject *newObj,
                              const _WanBertTestObject *currObj,
                              const InstanceIdStack *iidStack,
                              char **errorParam __attribute__((unused)),
                              CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */

   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return ret;
   }

   ret = rutWan_setAdslBertInfo(newObj,currObj,iidStack);

   return ret;
}


CmsRet rcl_wanDslIntfStatsObject( _WanDslIntfStatsObject *newObj __attribute__((unused)),
                const _WanDslIntfStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}


CmsRet rcl_wanDslIntfStatsTotalObject( _WanDslIntfStatsTotalObject *newObj __attribute__((unused)),
                const _WanDslIntfStatsTotalObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}


CmsRet rcl_wanDslIntfStatsShowtimeObject( _WanDslIntfStatsShowtimeObject *newObj __attribute__((unused)),
                const _WanDslIntfStatsShowtimeObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_wanDslIntfStatsCurrentDayObject( _WanDslIntfStatsCurrentDayObject *newObj __attribute__((unused)),
                const _WanDslIntfStatsCurrentDayObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

#ifdef NOT_SUPPORTED
CmsRet rcl_wanDslIntfStatsLastShowtimeObject( _WanDslIntfStatsLastShowtimeObject *newObj __attribute__((unused)),
                const _WanDslIntfStatsLastShowtimeObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_wanDslIntfStatsQuarterHourObject( _WanDslIntfStatsQuarterHourObject *newObj __attribute__((unused)),
                const _WanDslIntfStatsQuarterHourObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}


#ifdef NOT_SUPPORTED
CmsRet rcl_wanDslDiagObject( _WanDslDiagObject *newObj __attribute__((unused)),
                const _WanDslDiagObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

#endif  /* NOT_SUPPORTED */


CmsRet rcl_wanDslProprietaryDiagObject( _WanDslProprietaryDiagObject *newObj __attribute__((unused)),
                const _WanDslProprietaryDiagObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_wanDslDiagPlnObject( _WanDslDiagPlnObject *newObj __attribute__((unused)),
                const _WanDslDiagPlnObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_wanDslDiagNonLinearityObject( _WanDslDiagNonLinearityObject *newObj __attribute__((unused)),
                const _WanDslDiagNonLinearityObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_wanDslTestParamsObject( _WanDslTestParamsObject *newObj __attribute__((unused)),
                                   const _WanDslTestParamsObject *currObj __attribute__((unused)),
                                   const InstanceIdStack *iidStack __attribute__((unused)),
                                   char **errorParam __attribute__((unused)),
                                   CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#else

/*
 * On non-DSL systems, these stubs must be present, but should be left empty.
 */
CmsRet rcl_wanDslIntfCfgObject( _WanDslIntfCfgObject *newObj __attribute__((unused)),
                const _WanDslIntfCfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_wanDslLinkCfgObject( _WanDslLinkCfgObject *newObj __attribute__((unused)),
                                 const WanDslLinkCfgObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_wanDslIntfStatsObject( _WanDslIntfStatsObject *newObj __attribute__((unused)),
                const _WanDslIntfStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_wanDslIntfStatsTotalObject( _WanDslIntfStatsTotalObject *newObj __attribute__((unused)),
                const _WanDslIntfStatsTotalObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}




#endif  /* DMP_ADSLWAN_1 */

