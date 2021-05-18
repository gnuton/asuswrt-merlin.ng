/***********************************************************************
 *
 *  Copyright (c) 2014  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2014:proprietary:standard

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


#ifdef DMP_DEVICE2_IPSEC_1

#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_ipsec.h"
 

CmsRet rcl_dev2IpsecObject( _Dev2IpsecObject *newObj,
                const _Dev2IpsecObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* just set our status to ENABLED.  There is no action associated
       * with the ipsec object itself.
       */
      CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,
                                  mdmLibCtx.allocFlags);
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if (newObj)
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2IpsecFilterObject( _Dev2IpsecFilterObject *newObj,
                const _Dev2IpsecFilterObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj && newObj->enable)
   {
      if (newObj->X_BROADCOM_COM_TunnelName)
      {
         if ((ret = rutIPSec_config()) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutIPSec_config failed, ret=%d", ret);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                        mdmLibCtx.allocFlags);
            goto done;
         }
         if ((ret = rutIPSec_restart()) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutIPSec_restart failed, ret=%d", ret);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                        mdmLibCtx.allocFlags);
            goto done;
         }

         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,
                                     mdmLibCtx.allocFlags);
      }
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if (newObj)
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);

         if (newObj->X_BROADCOM_COM_TunnelName)
         {
            if ((ret = rutIPSec_config()) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutIPSec_config failed, ret=%d", ret);
               goto done;
            }
            if ((ret = rutIPSec_restart()) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutIPSec_restart failed, ret=%d", ret);
               goto done;
            }
         }
      }
   }

done:
    return ret;
}


CmsRet rcl_dev2IpsecProfileObject( _Dev2IpsecProfileObject *newObj __attribute__((unused)),
                const _Dev2IpsecProfileObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_ikev1CfgObject( _Ikev1CfgObject *newObj __attribute__((unused)),
                const _Ikev1CfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_manualModeCfgObject( _ManualModeCfgObject *newObj __attribute__((unused)),
                const _ManualModeCfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

#endif /* DMP_DEVICE2_IPSEC_1 */


