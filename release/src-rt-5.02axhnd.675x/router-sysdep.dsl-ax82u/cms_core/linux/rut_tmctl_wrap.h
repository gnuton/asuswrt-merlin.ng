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

/*****************************************************************************
*    Description:
*
*      TMCtl API wrapper functions header file.
*
*****************************************************************************/
#ifndef RUT_TMCTL_WRAP_H
#define RUT_TMCTL_WRAP_H

/* ---- Include Files ----------------------------------------------------- */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "tmctl_defs.h"
#include "tmctl_api.h"


/* ---- Constants and Types ----------------------------------------------- */


/* ---- Variable Externs -------------------------------------------------- */


/* ---- Function Prototypes ----------------------------------------------- */

#define rutcwmp_tmctl_portTmInit(arg...)      rutwrap_tmctl_portTmInit(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_portTmUninit(arg...)    rutwrap_tmctl_portTmUninit(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_getQueueCfg(arg...)     rutwrap_tmctl_getQueueCfg(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_delQueueCfg(arg...)     rutwrap_tmctl_delQueueCfg(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_getPortShaper(arg...)   rutwrap_tmctl_getPortShaper(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_setPortShaper(arg...)   rutwrap_tmctl_setPortShaper(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_getQueueDropAlg(arg...) rutwrap_tmctl_getQueueDropAlg(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_setQueueDropAlg(arg...) rutwrap_tmctl_setQueueDropAlg(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_getQueueStats(arg...)   rutwrap_tmctl_getQueueStats(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_getPortTmParms(arg...)  rutwrap_tmctl_getPortTmParms(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_setQueueCfg(arg...)     rutwrap_tmctl_setQueueCfg(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_getQueueStats(arg...)   rutwrap_tmctl_getQueueStats(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_setQueueDropAlgExt(arg...) rutwrap_tmctl_setQueueDropAlgExt(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_setQueueSize(arg...)    rutwrap_tmctl_setQueueSize(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_createPolicer(arg...)    rutwrap_tmctl_createPolicer(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_modifyPolicer(arg...)    rutwrap_tmctl_modifyPolicer(TMCTL_USER_TR69, ##arg)
#define rutcwmp_tmctl_deletePolicer(arg...)    rutwrap_tmctl_deletePolicer(TMCTL_USER_TR69, ##arg)

#define rutomci_tmctl_portTmInit(arg...)      rutwrap_tmctl_portTmInitExt(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_portTmUninit(arg...)    rutwrap_tmctl_portTmUninit(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_getQueueCfg(arg...)     rutwrap_tmctl_getQueueCfg(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_delQueueCfg(arg...)     rutwrap_tmctl_delQueueCfg(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_getPortShaper(arg...)   rutwrap_tmctl_getPortShaper(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_setPortShaper(arg...)   rutwrap_tmctl_setPortShaper(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_getQueueDropAlg(arg...) rutwrap_tmctl_getQueueDropAlg(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_setQueueDropAlg(arg...) rutwrap_tmctl_setQueueDropAlg(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_getQueueStats(arg...)   rutwrap_tmctl_getQueueStats(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_getPortTmParms(arg...)  rutwrap_tmctl_getPortTmParms(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_setQueueCfg(arg...)     rutwrap_tmctl_setQueueCfg(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_getQueueStats(arg...)   rutwrap_tmctl_getQueueStats(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_setQueueDropAlgExt(arg...) rutwrap_tmctl_setQueueDropAlgExt(TMCTL_USER_OMCI, ##arg)
#define rutomci_tmctl_setQueueSize(arg...)    rutwrap_tmctl_setQueueSize(TMCTL_USER_OMCI, ##arg)

#define rutoam_tmctl_portTmInit(arg...)       rutwrap_tmctl_portTmInit(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_portTmUninit(arg...)     rutwrap_tmctl_portTmUninit(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_getQueueCfg(arg...)      rutwrap_tmctl_getQueueCfg(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_delQueueCfg(arg...)      rutwrap_tmctl_delQueueCfg(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_getPortShaper(arg...)    rutwrap_tmctl_getPortShaper(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_setPortShaper(arg...)    rutwrap_tmctl_setPortShaper(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_getQueueDropAlg(arg...)  rutwrap_tmctl_getQueueDropAlg(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_setQueueDropAlg(arg...)  rutwrap_tmctl_setQueueDropAlg(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_getQueueStats(arg...)    rutwrap_tmctl_getQueueStats(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_getPortTmParms(arg...)   rutwrap_tmctl_getPortTmParms(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_setQueueCfg(arg...)      rutwrap_tmctl_setQueueCfg(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_getQueueStats(arg...)    rutwrap_tmctl_getQueueStats(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_setQueueDropAlgExt(arg...) rutwrap_tmctl_setQueueDropAlgExt(TMCTL_USER_OAM, ##arg)
#define rutoam_tmctl_setQueueSize(arg...)     rutwrap_tmctl_setQueueSize(TMCTL_USER_OAM, ##arg)

#define rutsuper_tmctl_portTmInit(arg...)       rutwrap_tmctl_portTmInit(TMCTL_USER_MAX, ##arg)
#define rutsuper_tmctl_portTmUninit(arg...)     rutwrap_tmctl_portTmUninit(TMCTL_USER_MAX, ##arg)
#define rutsuper_tmctl_getQueueCfg(arg...)      rutwrap_tmctl_getQueueCfg(TMCTL_USER_MAX, ##arg)
#define rutsuper_tmctl_delQueueCfg(arg...)      rutwrap_tmctl_delQueueCfg(TMCTL_USER_MAX, ##arg)
#define rutsuper_tmctl_getPortShaper(arg...)    rutwrap_tmctl_getPortShaper(TMCTL_USER_MAX, ##arg)
#define rutsuper_tmctl_setPortShaper(arg...)    rutwrap_tmctl_setPortShaper(TMCTL_USER_MAX, ##arg)
#define rutsuper_tmctl_getQueueDropAlg(arg...)  rutwrap_tmctl_getQueueDropAlg(TMCTL_USER_MAX, ##arg)
#define rutsuper_tmctl_setQueueDropAlg(arg...)  rutwrap_tmctl_setQueueDropAlg(TMCTL_USER_MAX, ##arg)
#define rutsuper_tmctl_getQueueStats(arg...)    rutwrap_tmctl_getQueueStats(TMCTL_USER_MAX, ##arg)
#define rutsuper_tmctl_getPortTmParms(arg...)   rutwrap_tmctl_getPortTmParms(TMCTL_USER_MAX, ##arg)
#define rutsuper_tmctl_setQueueCfg(arg...)      rutwrap_tmctl_setQueueCfg(TMCTL_USER_MAX, ##arg)
#define rutsuper_tmctl_setQueueDropAlgExt(arg...) rutwrap_tmctl_setQueueDropAlgExt(TMCTL_USER_MAX, ##arg)
#define rutsuper_tmctl_setQueueSize(arg...)     rutwrap_tmctl_setQueueSize(TMCTL_USER_MAX, ##arg)

BOOL rut_tmctl_isUserTMCtlOwner(eTmctlUser user);
UINT32 rut_tmctl_getQueueOwner(void);
UINT32 rut_tmctl_getQueueMap(void);
CmsRet rut_tmctl_getQueueScheme(UINT32 *owner, UINT32 *scheme);

tmctl_ret_e rutwrap_tmctl_portTmInit(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, uint32_t cfgFlags);
tmctl_ret_e rutwrap_tmctl_portTmInitExt(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, uint32_t cfgFlags, int numQueues);
tmctl_ret_e rutwrap_tmctl_portTmUninit(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p);
tmctl_ret_e rutwrap_tmctl_getQueueCfg(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, int queueId, tmctl_queueCfg_t *qcfg_p);
tmctl_ret_e rutwrap_tmctl_delQueueCfg(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, int queueId);
tmctl_ret_e rutwrap_tmctl_getPortShaper(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, tmctl_shaper_t *shaper_p);
tmctl_ret_e rutwrap_tmctl_setPortShaper(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, tmctl_shaper_t *shaper_p);
tmctl_ret_e rutwrap_tmctl_getQueueDropAlg(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, int queueId, tmctl_queueDropAlg_t *dropAlg_p);
tmctl_ret_e rutwrap_tmctl_setQueueDropAlg(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, int queueId, tmctl_queueDropAlg_t *dropAlg_p);
tmctl_ret_e rutwrap_tmctl_getQueueStats(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, int queueId, tmctl_queueStats_t *stats_p);
tmctl_ret_e rutwrap_tmctl_getPortTmParms(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, tmctl_portTmParms_t *tmParms_p);
tmctl_ret_e rutwrap_tmctl_setQueueCfg(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, tmctl_queueCfg_t *qcfg_p);
tmctl_ret_e rutwrap_tmctl_getQueueStats(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, int queueId, tmctl_queueStats_t *stats_p);
tmctl_ret_e rutwrap_tmctl_setQueueDropAlgExt(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, int queueId, tmctl_queueDropAlg_t *dropAlg_p);
tmctl_ret_e rutwrap_tmctl_setQueueSize(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p, int queueId, int size);
tmctl_ret_e rutwrap_tmctl_createPolicer(eTmctlUser user, tmctl_policer_t *policer_p);
tmctl_ret_e rutwrap_tmctl_modifyPolicer(eTmctlUser user, tmctl_policer_t *policer_p);
tmctl_ret_e rutwrap_tmctl_deletePolicer(eTmctlUser user, tmctl_dir_e dir, int policerId);

#if defined(DMP_X_BROADCOM_COM_GPONWAN_1) || defined(DMP_X_BROADCOM_COM_EPONWAN_1)
BOOL getPonLinkCfgFlag(int wan_type);
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 || DMP_X_BROADCOM_COM_EPONWAN_1 */

#endif /* RUT_TMCTL_WRAP_H */
