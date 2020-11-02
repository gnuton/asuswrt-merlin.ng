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

#ifdef DMP_ADSLWAN_1

#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1

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
#include "rut_xtmlinkcfg.h"

#include "adslctlapi.h"
#include "AdslMibDef.h"
#include "bcmadsl.h"
#include "bcmxdsl.h"
#include "devctl_xtm.h"
#include "rut_dsl.h"
#include "rut_wanlayer2.h"
#include "devctl_adsl.h"

UBOOL8 rutDsl_isDslBondingEnabled(void)
{
   WanDslIntfCfgObject *dslIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UBOOL8 enabled = FALSE;

#ifdef DMP_PTMWAN_1
   ret = rutWl2_getPtmDslIntfObject(&iidStack, &dslIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get PTM DslIntf obj, ret=%d", ret);
      return FALSE;
   }
#endif

#ifdef DMP_X_BROADCOM_COM_ATMWAN_1
   if (dslIntfObj == NULL) {
      memset (&iidStack, 0, sizeof (InstanceIdStack)) ;
      ret = rutWl2_getAtmDslIntfObject(&iidStack, &dslIntfObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to get ATM DslIntf obj, ret=%d", ret);
         return FALSE;
      }
   }
#endif

   if (dslIntfObj) {
      enabled = dslIntfObj->X_BROADCOM_COM_EnableBonding;
      cmsObj_free((void **) &dslIntfObj);
   }
   else
      enabled = (UBOOL8) 0 ;
   
   cmsLog_debug("returning enabled=%d", enabled);

   return enabled;
}

#endif /* DMP_X_BROADCOM_COM_DSLBONDING_1 */

#endif /* DMP_ADSLWAN_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */
