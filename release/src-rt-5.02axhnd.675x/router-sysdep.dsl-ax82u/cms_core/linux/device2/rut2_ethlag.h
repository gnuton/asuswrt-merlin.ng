/***********************************************************************
 *
 *  Copyright (c) 2009-2016  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2016:proprietary:standard

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

#ifndef __RUT2_LAG_H__
#define __RUT2_LAG_H__

#include "cms_core.h"
#include "cms_util.h"
#include "qdm_intf.h"
#include "rcl.h"
#include "rut_util.h"
//#include "rut2_erouter.h"

#ifdef DMP_DEVICE2_ETHLAG_1

enum {
    LAG_PARAM_SELECT_LOGIC,
    LAG_PARAM_LACP_RATE
};

enum {
    LAG_SELECT_LOGIC_STABLE = 0,
    LAG_SELECT_LOGIC_BANDWIDTH,
    LAG_SELECT_LOGIC_COUNT,
    _LAG_SELECT_LOGIC_MAX_ = LAG_SELECT_LOGIC_COUNT,
};

enum {
    LAG_LACP_RATE_SLOW,
    LAG_LACP_RATE_FAST,
    _LAG_LACP_RATE_MAX_ = LAG_LACP_RATE_FAST,
};

CmsRet rutLag_loadModule(void);
void rutEthLag_configInterface(const _Dev2EthLAGObject *obj, UBOOL8 add);
void rutEthLag_enableInterface( const char *ifName);
void rutEthLag_disableInterface(const char *ifName);
void rutEthLag_configEthIntfOnBridge(const char *brName, const char *ethIfName, UBOOL8 add);
void rutEthLag_configEthIntfOnLagIntf(const char *ethLagName, const char *ethIfName, UBOOL8 add);
CmsRet rutEthLag_getAvailableIntf(char *ethLagIntfs, SINT32 ethLagIntfsLen);


#if 0
CmsRet rutLag_configMaster(const LagAggregObject *newObj, UBOOL8 add);
CmsRet rutLag_updateMode(const LagAggregObject *newObj);
CmsRet rutLag_updateSelection(const LagAggregObject *obj);
CmsRet rutLag_updateHashPolicy(const LagAggregObject *obj);
void rutLag_doPortIsolation(void);
CmsRet rutLag_addAggregator(const LagAggregObject *newObj);
CmsRet rutLag_delAggregator(const LagAggregObject *newObj);
CmsRet rutLag_enableAggregator(const LagAggregObject *newObj);
CmsRet rutLag_disableAggregator(const LagAggregObject *newObj);
CmsRet rutLag_updateAggregator(const LagAggregObject *newObj, const LagAggregObject *currObj);
CmsRet rutLag_portInterface(const char *aggreg_name, const char *if_name, UBOOL8 add);
CmsRet rutLag_cmNotifyState(UBOOL8 lag_enabled);
#endif

#endif  /* DMP_DEVICE2_ETHLAG_1 */

#endif /* __RUT2_LAG_H__ */

#endif /* DMP_DEVICE2_BASELINE_1 */
