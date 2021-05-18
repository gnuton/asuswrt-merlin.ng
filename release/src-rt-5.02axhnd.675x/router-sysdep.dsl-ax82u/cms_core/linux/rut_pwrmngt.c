#ifdef DMP_X_BROADCOM_COM_PWRMNGT_1 /* aka SUPPORT_PWRMNGT */
/***********************************************************************
 *
 *  Copyright (c) 2008-2010  Broadcom Corporation
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

#include "cms.h"
#include "odl.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_pwrmngt.h"
#include "bcmpwrmngtcfg.h"
#include "devctl_pwrmngt.h"
#include "ethswctl_api.h"

CmsRet rut_restartPowerManagement(const _PwrMngtObject *pwrMngtObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   PWRMNGT_CONFIG_PARAMS PwrMngtCfgParams;
   UINT32 mask = 0;

   memset(&PwrMngtCfgParams, 0, sizeof(PWRMNGT_CONFIG_PARAMS));
#if defined(SUPPORT_HOSTMIPS_PWRSAVE)
   PwrMngtCfgParams.cpuspeed      = pwrMngtObj->CPUSpeed;
   mask |= PWRMNGT_CFG_PARAM_CPUSPEED_MASK;
#endif
   PwrMngtCfgParams.cpur4kwait    = pwrMngtObj->CPUr4kWaitEn;
   mask |= PWRMNGT_CFG_PARAM_CPU_R4K_WAIT_MASK;
#if defined(SUPPORT_DDR_SELF_REFRESH_PWRSAVE)
   PwrMngtCfgParams.dramSelfRefresh= pwrMngtObj->DRAMSelfRefreshEn;
   mask |= PWRMNGT_CFG_PARAM_MEM_SELF_REFRESH_MASK;
#endif
#if defined(SUPPORT_AVS_PWRSAVE) || defined(SUPPORT_MOCA_AVS)
   PwrMngtCfgParams.avs           =  pwrMngtObj->avsEn;
   mask |= PWRMNGT_CFG_PARAM_MEM_AVS_MASK;
#endif

   if ((ret = PwrMngtCtl_SetConfig(&PwrMngtCfgParams, mask, mdmLibCtx.msgHandle)) != CMSRET_SUCCESS)
      return ret;

#if defined(SUPPORT_ETH_PWRSAVE)
   if (bcm_phy_apd_set(pwrMngtObj->ethAutoPwrDwnEn))
      return CMSRET_INTERNAL_ERROR;
#endif

#if defined(SUPPORT_ETH_DEEP_GREEN_MODE)
   if (bcm_DeepGreenMode_set(pwrMngtObj->ethAutoPwrDwnEn))
      return CMSRET_INTERNAL_ERROR;
#endif

#if defined(SUPPORT_ENERGY_EFFICIENT_ETHERNET)
   if (bcm_phy_eee_set(pwrMngtObj->ethEEE))
      return CMSRET_INTERNAL_ERROR;
#endif

   return ret;   
} /* rut_restartPowerManagement */

#endif /* aka SUPPORT_PWRMNGT */
