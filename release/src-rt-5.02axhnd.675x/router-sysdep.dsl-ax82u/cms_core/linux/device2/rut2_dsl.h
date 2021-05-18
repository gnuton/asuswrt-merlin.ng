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
#ifndef __RUT2_DSL_H__
#define __RUT2_DSL_H__


/*!\file rut2_dsl.h
 * \brief System level interface functions for DSL functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_atm.h"
#include "devctl_xtm.h"
#include "AdslMibDef.h"

UBOOL8 rutdsl_isDslConfigChanged_dev2(const _Dev2DslLineObject *newObj, const _Dev2DslLineObject *currObj);
CmsRet rutdsl_configUp_dev2(Dev2DslLineObject *dslLineObj);
void rutdsl_configDown_dev2(void);
CmsRet rutdsl_getAdslShowTimeStats_dev2(Dev2DslLineStatsShowtimeObject *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getAdslCurrentDayStats_dev2(Dev2DslLineStatsCurrentDayObject *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getAdslQuarterHourStats_dev2(Dev2DslLineStatsQuarterHourObject *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getAdslTestParamsInfo_dev2(void *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getAdslTotalStats_dev2(Dev2DslLineStatsTotalObject *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getdslLineStats_dev2(Dev2DslLineStatsObject *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getAdslBertInfo_dev2(void *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_setAdslBertInfo_dev2(void *new, const void *curr, const InstanceIdStack *iidStack __attribute__((unused)));
CmsRet rutdsl_getLineInfo_dev2(Dev2DslLineObject *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getChannelInfo_dev2(Dev2DslChannelObject *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getCurrentDayChannelStats_dev2(Dev2DslChannelStatsCurrentDayObject *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getQuarterHourChannelStats_dev2(Dev2DslChannelStatsQuarterHourObject *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getShowTimeChannelStats_dev2(Dev2DslChannelStatsShowtimeObject *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getTotalChannelStats_dev2(Dev2DslChannelStatsTotalObject *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getdslChannelStats_dev2(Dev2DslChannelStatsObject *obj, const InstanceIdStack *iidStack);

#if defined(SUPPORT_DSL_BONDING)
CmsRet rutdsl_getBondingGroupStats(Dev2DslBondingGroupStatsObject *obj);
CmsRet rutdsl_getBondingGroupTotalStats(Dev2DslBondingGroupStatsTotalObject *obj);
CmsRet rutdsl_getBondingGroupCurrentDayStats(Dev2DslBondingGroupStatsCurrentDayObject *obj);
CmsRet rutdsl_getBondingGroupQuarterHourStats(Dev2DslBondingGroupStatsQuarterHourObject *obj);
CmsRet rutdsl_getBondingGroupTotalStats(Dev2DslBondingGroupStatsTotalObject *obj);
#endif

CmsRet rutdsl_getLineIdByLineIidStack_dev2(const InstanceIdStack *iidStack, UINT32 *lineId);
CmsRet rutdsl_getLineIdByChannelIidStack_dev2(const InstanceIdStack *iidStack, UINT32 *lineId);

#endif  /* __RUT2_DSL_H__ */

