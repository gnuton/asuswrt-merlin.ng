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

#ifdef DMP_DEVICE2_BONDEDDSL_1 

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_dsl.h"

CmsRet rcl_dev2DslBondingGroupObject( _Dev2DslBondingGroupObject *newObj,
                                      const _Dev2DslBondingGroupObject *currObj,
                                      const InstanceIdStack *iidStack __attribute__((unused)),
                                      char **errorParam __attribute__((unused)),
                                      CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   if (ADD_NEW(newObj, currObj))
   {
      return (CMSRET_SUCCESS);
   }

   IF_STATUS_HAS_CHANGED_SET_LASTCHANGE(newObj, currObj);

   if (newObj->enable != currObj->enable)
   {
      /* No action is really being done when the bonding mode is changed, 
       * a reboot is required for reconfiguration 
       */
      ret = CMSRET_SUCCESS_REBOOT_REQUIRED;
   }
   return (ret);
}

CmsRet rcl_dev2DslBondingGroupBondedChannelObject( _Dev2DslBondingGroupBondedChannelObject *newObj __attribute__((unused)),
                                                   const _Dev2DslBondingGroupBondedChannelObject *currObj __attribute__((unused)),
                                                   const InstanceIdStack *iidStack __attribute__((unused)),
                                                   char **errorParam __attribute__((unused)),
                                                   CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslBondingGroupBondedChannelEthernetObject( _Dev2DslBondingGroupBondedChannelEthernetObject *newObj __attribute__((unused)),
                                                           const _Dev2DslBondingGroupBondedChannelEthernetObject *currObj __attribute__((unused)),
                                                           const InstanceIdStack *iidStack __attribute__((unused)),
                                                           char **errorParam __attribute__((unused)),
                                                           CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslBondingGroupBondedChannelEthernetStatsObject(_Dev2DslBondingGroupBondedChannelEthernetStatsObject *newObj __attribute__((unused)),
                                                                const _Dev2DslBondingGroupBondedChannelEthernetStatsObject *currObj __attribute__((unused)),
                                                                const InstanceIdStack *iidStack __attribute__((unused)),
                                                                char **errorParam __attribute__((unused)),
                                                                CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslBondingGroupStatsObject( _Dev2DslBondingGroupStatsObject *newObj __attribute__((unused)),
                                           const _Dev2DslBondingGroupStatsObject *currObj __attribute__((unused)),
                                           const InstanceIdStack *iidStack __attribute__((unused)),
                                           char **errorParam __attribute__((unused)),
                                           CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslBondingGroupStatsTotalObject( _Dev2DslBondingGroupStatsTotalObject *newObj __attribute__((unused)),
                                                const _Dev2DslBondingGroupStatsTotalObject *currObj __attribute__((unused)),
                                                const InstanceIdStack *iidStack __attribute__((unused)),
                                                char **errorParam __attribute__((unused)),
                                                CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslBondingGroupStatsQuarterHourObject( _Dev2DslBondingGroupStatsQuarterHourObject *newObj __attribute__((unused)),
                                                      const _Dev2DslBondingGroupStatsQuarterHourObject *currObj __attribute__((unused)),
                                                      const InstanceIdStack *iidStack __attribute__((unused)),
                                                      char **errorParam __attribute__((unused)),
                                                      CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslBondingGroupEthernetObject( _Dev2DslBondingGroupEthernetObject *newObj __attribute__((unused)),
                                              const _Dev2DslBondingGroupEthernetObject *currObj __attribute__((unused)),
                                              const InstanceIdStack *iidStack __attribute__((unused)),
                                              char **errorParam __attribute__((unused)),
                                              CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslBondingGroupEthernetStatsObject( _Dev2DslBondingGroupEthernetStatsObject *newObj __attribute__((unused)),
                                                   const _Dev2DslBondingGroupEthernetStatsObject *currObj __attribute__((unused)),
                                                   const InstanceIdStack *iidStack __attribute__((unused)),
                                                   char **errorParam __attribute__((unused)),
                                                   CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2DslBondingGroupStatsCurrentDayObject( _Dev2DslBondingGroupStatsCurrentDayObject *newObj __attribute__((unused)),
                                                     const _Dev2DslBondingGroupStatsCurrentDayObject *currObj __attribute__((unused)),
                                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                                     char **errorParam __attribute__((unused)),
                                                     CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

#endif  /* DMP_DEVICE2_BONDEDDSL_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */
