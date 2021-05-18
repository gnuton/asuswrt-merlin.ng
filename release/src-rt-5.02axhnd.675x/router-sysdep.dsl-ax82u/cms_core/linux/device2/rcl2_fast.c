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

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut2_fast.h"
#include "rut2_dsl.h"
#include "devctl_adsl.h"

CmsRet rcl_dev2FastObject( _Dev2FastObject *newObj __attribute__((unused)),
                           const _Dev2FastObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
   cmsLog_debug("Enter");
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2FastLineObject( _Dev2FastLineObject *newObj,
                               const _Dev2FastLineObject *currObj,
                               const InstanceIdStack *iidStack,
                               char **errorParam __attribute__((unused)),
                               CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2DslLineObject *dslLineObj;
   InstanceIdStack dslIidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;

   cmsLog_debug("Enter");
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }
      
   /*
    * Update FAST Line NumberOfEntries on successful add or delete.
    */
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumFastLine(iidStack, 1);
      /* fast object is just used for status updating, and not actually doing initialization */
      /* dsl object will call rutDsl_configUp which will also do the FAST configuration because DSL and FAST is in 1 driver */
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      /* although we are not supposed to delete a fast line */
      rutUtil_modifyNumFastLine(iidStack, -1);
   }
   else if (ENABLE_EXISTING(newObj, currObj) ||
            rutfast_isConfigChanged_dev2(newObj, currObj))
   {
      while (!found && (cmsObj_getNext(MDMOID_DEV2_DSL_LINE,&dslIidStack,(void **)&dslLineObj) == CMSRET_SUCCESS))
      {
         if (dslLineObj->X_BROADCOM_COM_BondingLineNumber == currObj->X_BROADCOM_COM_BondingLineNumber)
         {
            found = TRUE;
            break;
         }
         cmsObj_free((void **) &dslLineObj);
      }
      if (!found)
      {
         cmsLog_error("Cannot get line object for reinitialization, ret=%d", ret);
         return ret;
      }
      /* enable existing intf */
      ret = rutdsl_configUp_dev2(dslLineObj);
      cmsObj_free((void **) &dslLineObj);
   }
   return ret;
}

CmsRet rcl_dev2FastLineStatsObject( _Dev2FastLineStatsObject *newObj __attribute__((unused)),
                                    const _Dev2FastLineStatsObject *currObj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                    char **errorParam __attribute__((unused)),
                                    CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2FastLineStatsTotalObject( _Dev2FastLineStatsTotalObject *newObj __attribute__((unused)),
                                         const _Dev2FastLineStatsTotalObject *currObj __attribute__((unused)),
                                         const InstanceIdStack *iidStack __attribute__((unused)),
                                         char **errorParam __attribute__((unused)),
                                         CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2FastLineStatsShowtimeObject( _Dev2FastLineStatsShowtimeObject *newObj,
                                            const _Dev2FastLineStatsShowtimeObject *currObj,
                                            const InstanceIdStack *iidStack,
                                            char **errorParam,
                                            CmsRet *errorCode)
{
   return (CMSRET_SUCCESS);
}

#if 0
/* notSupported */
CmsRet rcl_dev2FastLineStatsLastShowtimeObject( _Dev2FastLineStatsLastShowtimeObject *newObj __attribute__((unused)),
                                                const _Dev2FastLineStatsLastShowtimeObject *currObj __attribute__((unused)),
                                                const InstanceIdStack *iidStack __attribute__((unused)),
                                                char **errorParam __attribute__((unused)),
                                                CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}
#endif

CmsRet rcl_dev2FastLineStatsCurrentDayObject( _Dev2FastLineStatsCurrentDayObject *newObj __attribute__((unused)),
                                              const _Dev2FastLineStatsCurrentDayObject *currObj __attribute__((unused)),
                                              const InstanceIdStack *iidStack __attribute__((unused)),
                                              char **errorParam __attribute__((unused)),
                                              CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2FastLineStatsQuarterHourObject( _Dev2FastLineStatsQuarterHourObject *newObj __attribute__((unused)),
                                               const _Dev2FastLineStatsQuarterHourObject *currObj __attribute__((unused)),
                                               const InstanceIdStack *iidStack __attribute__((unused)),
                                               char **errorParam __attribute__((unused)),
                                               CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2FastLineTestParamsObject( _Dev2FastLineTestParamsObject *newObj __attribute__((unused)),
                                         const _Dev2FastLineTestParamsObject *currObj __attribute__((unused)),
                                         const InstanceIdStack *iidStack __attribute__((unused)),
                                         char **errorParam __attribute__((unused)),
                                         CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


#endif /* DMP_DEVICE2_FAST_1 */


#endif /* DMP_DEVICE2_BASELINE_1 */
