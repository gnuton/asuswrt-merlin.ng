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
#ifndef __RUT2_FAST_H__
#define __RUT2_FAST_H__


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
#include "devctl_xtm.h"
#include "AdslMibDef.h"

CmsRet rutfast_getShowTimeStats_dev2(Dev2FastLineStatsShowtimeObject *obj);
CmsRet rutfast_getCurrentDayStats_dev2(Dev2FastLineStatsCurrentDayObject *obj);
CmsRet rutfast_getQuarterHourStats_dev2(Dev2FastLineStatsQuarterHourObject *obj);
CmsRet rutfast_getTestParamsInfo_dev2(void *obj);
CmsRet rutfast_getTotalStats_dev2(Dev2FastLineStatsTotalObject *obj);
CmsRet rutfast_getLineStats_dev2(Dev2FastLineStatsObject *obj);
CmsRet rutfast_getLineInfo_dev2(Dev2FastLineObject *obj, const InstanceIdStack *iidStack);
UBOOL8 rutfast_isConfigChanged_dev2(const _Dev2FastLineObject *newObj, const  _Dev2FastLineObject *currObj);
void rutfast_cfgProfileInit_dev2(adslCfgProfile *pAdslCfg,  unsigned char lineId);
void rutfast_intfCfgInit_dev2(adslCfgProfile *pAdslCfg, Dev2FastLineObject *pFastLineObj);

#endif  /* __RUT2_FAST_H__ */

