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
*      TMCtl API wrapper utility.
*
*****************************************************************************/

#if defined(SUPPORT_TMCTL)

/* ---- Include Files ----------------------------------------------------- */

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut_wan.h"
#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
#include "rut_gponwan.h"
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 */
#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
#include "rut_epon_oam.h"
#include "rut_eponwan.h"
#endif /* DMP_X_BROADCOM_COM_EPONWAN_1 */

#include "tmctl_api.h"
#include "rut_tmctl_wrap.h"
#if defined(DMP_X_BROADCOM_COM_GPONWAN_1) || defined(DMP_X_BROADCOM_COM_EPONWAN_1)
#include "rdpactl_api.h"
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 || DMP_X_BROADCOM_COM_EPONWAN_1 */


/* ---- Private Constants and Types --------------------------------------- */


/* ---- Private Function Prototypes --------------------------------------- */

static BOOL applyTmctlCfg(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p);

#ifndef BUILD_CUSTOMER
#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
#ifndef DESKTOP_LINUX
BOOL getGponLinkEnableFlag(void);
#endif /* DESKTOP_LINUX */
#ifdef INIT_WAN_QUEUE_ON_LINKUP
static BOOL getGponLinkStatusFlag(void);
#endif /* INIT_WAN_QUEUE_ON_LINKUP */
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 */
#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
#ifdef INIT_WAN_QUEUE_ON_LINKUP
static BOOL getEponLinkStatusFlag(void);
#endif /* INIT_WAN_QUEUE_ON_LINKUP */
#endif /* DMP_X_BROADCOM_COM_EPONWAN_1 */
#if defined(DMP_X_BROADCOM_COM_GPONWAN_1) || defined(DMP_X_BROADCOM_COM_EPONWAN_1)
#ifdef INIT_WAN_QUEUE_ON_LINKUP
static BOOL getPonLinkStatusFlag(void);
#endif /* INIT_WAN_QUEUE_ON_LINKUP */
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 || DMP_X_BROADCOM_COM_EPONWAN_1 */
#endif /* BUILD_CUSTOMER*/


/* ---- Public Variables -------------------------------------------------- */


/* ---- Private Variables ------------------------------------------------- */


/* ---- Functions --------------------------------------------------------- */

/* The TM owner is determined by according to the following.
 *  - SFU mode:          OMCI/OAM (fixed)
 *  - HGU "BBF247" mode: backhaul (OMCI)
 *  - HGU "CAR" mode:    frontend (RGW)
 *
 * In SFU or HGU BBF247 mode, the following configuration is applied to the
 * underlying driver.
 *    * OMCI configuration on T-CONT queues.
 *    * OMCI configuration on VEIP queues (only one ETH port).
 *    * OMCI's GEM to queue mapping.
 *    * OMCI's creation of the default LAN queues.
 * In SFU or HGU BBF247 mode, the following configuration is not applied to
 * the underlying driver.
 *    * WAN/LAN queues configured in the CMS. They are used as reference to
 *      the classification rules.
 *
 * In the HGU CAR mode, the following configuration is applied to the
 * underlying driver.
 *    * RGW creation and configuration on the WAN/LAN queues.
 * In the HGU CAR mode, the following configuration is not applied to the
 * underlying driver.
 *    * OMCI configuration on T-CONT queues.
 *    * OMCI configuration on VEIP queues.
 *    * OMCI configuration on GEM to queue mapping.
 */

UINT32 rut_tmctl_getQueueOwner(void)
{
    CmsRet ret = CMSRET_SUCCESS;
    UINT32 owner;
    UINT32 scheme;

    ret = rut_tmctl_getQueueScheme(&owner, &scheme);
    if (ret != CMSRET_SUCCESS)
    {
        owner = TMCTL_OWNER_FE;
    }
    return owner;
}

UINT32 rut_tmctl_getQueueMap(void)
{
    CmsRet ret = CMSRET_SUCCESS;
    UINT32 owner;
    UINT32 scheme;

    ret = rut_tmctl_getQueueScheme(&owner, &scheme);
    if (ret != CMSRET_SUCCESS)
    {
        scheme = QID_PRIO_MAP_Q7P7;
    }
    return scheme;
}

CmsRet rut_tmctl_getQueueScheme(UINT32 *owner, UINT32 *scheme)
{
    CmsRet ret = CMSRET_SUCCESS;
#ifdef DMP_X_BROADCOM_COM_TM_1
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    _BCMTrafficManagementObject *tmObj = NULL;

    // Nested locks are allowed, but there must be an equal number of unlocks.
    ret = cmsLck_acquireLockWithTimeout(6 * MSECS_IN_SEC);
    if (ret != CMSRET_SUCCESS)
        return ret;

    if ((ret = cmsObj_get(MDMOID_BCM_TRAFFIC_MANAGEMENT, &iidStack, 0,
                          (void**)&tmObj)) == CMSRET_SUCCESS)
    {
        *scheme = tmObj->qidPrioMap;
        *owner = tmObj->owner;
        cmsObj_free((void**)&tmObj);
    }
    else
    {
        cmsLog_error("cmsObj_get(TM) failed, rc=%d", ret);
    }

    cmsLck_releaseLock();
#else
    *owner = TMCTL_OWNER_FE;
    *scheme = QID_PRIO_MAP_Q7P7;
#endif /* DMP_X_BROADCOM_COM_TM_1 */

    return ret;
}

BOOL rut_tmctl_isUserTMCtlOwner(eTmctlUser user __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
    cmsLog_debug("Skip TMCtl configuration, user=%d", user);
    return FALSE;
#else /* DESKTOP_LINUX */
    if (user == TMCTL_USER_OMCI)
    {
        if (rut_tmctl_getQueueOwner() == TMCTL_OWNER_FE)
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }

    if (user == TMCTL_USER_OAM)
    {
#ifdef EPON_SFU
        return TRUE;
#else
        if (rut_tmctl_getQueueOwner() == TMCTL_OWNER_FE)
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
#endif
    }

    if (rut_tmctl_getQueueOwner() == TMCTL_OWNER_BH)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
#endif /* DESKTOP_LINUX */
}

#ifdef BUILD_CUSTOMER
#include "rut_wanlayer2.h"
static BOOL getPonWanStatusFlag(void)
{
    WanPonIntfObject *wanPonIntfCfg = NULL;
    CmsRet ret = CMSRET_SUCCESS;
    BOOL upFlag = FALSE;
    InstanceIdStack ponWanIid;
    
    if (rut_isWanTypeEpon() == TRUE)
    {
        if ((ret = rutWl2_getEponWanIidStack(&ponWanIid)) != CMSRET_SUCCESS)
            cmsLog_error("could not get EponWanIidStack, ret=%d", ret);
    }
    else
    {
        if ((ret = rutWl2_getGponWanIidStack(&ponWanIid)) != CMSRET_SUCCESS)
            cmsLog_error("could not get GponWanIidStack, ret=%d", ret);
    }

    if (ret == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_WAN_PON_INTF, &ponWanIid, OGF_NO_VALUE_UPDATE, (void **)&wanPonIntfCfg)) != CMSRET_SUCCESS)
            cmsLog_error("cmsObj_get  <MDMOID_WAN_PON_INTF> error. ret=%d", ret);
        else
        {
            upFlag = (0 == cmsUtl_strcmp(wanPonIntfCfg->status, MDMVS_UP));
            cmsObj_free((void **) &wanPonIntfCfg);
        }
    }
    
    return upFlag;
}
#endif /* BUILD_CUSTOMER */


#if defined(DMP_X_BROADCOM_COM_GPONWAN_1) || defined(DMP_X_BROADCOM_COM_EPONWAN_1)
static BOOL applyTmctlCfg(eTmctlUser user, tmctl_devType_e devType,
  tmctl_if_t *if_p)
#else
static BOOL applyTmctlCfg(eTmctlUser user, tmctl_devType_e devType __attribute__((unused)),
  tmctl_if_t *if_p __attribute__((unused)))
#endif
{
    BOOL applyFlag = FALSE;

    if (user == TMCTL_USER_MAX)
    {
        applyFlag = TRUE;
    }
    else if (rut_tmctl_isUserTMCtlOwner(user) == TRUE)
    {
        applyFlag = TRUE;

#ifdef INIT_WAN_QUEUE_ON_LINKUP
#if defined(DMP_X_BROADCOM_COM_GPONWAN_1) || defined(DMP_X_BROADCOM_COM_EPONWAN_1)
        /* Skip WAN queue configuration if L2 link status is down. */
        if ((user == TMCTL_USER_TR69) && (devType == TMCTL_DEV_ETH) &&
         ((strstr(if_p->ethIf.ifname, GPON_IFC_STR)) ||
         (strstr(if_p->ethIf.ifname, EPON_IFC_STR))))
        {
            rdpa_if wan_if = rdpa_if_none;
            if (strstr(if_p->ethIf.ifname, GPON_IFC_STR))
            {
                wan_if = rdpa_wan_type_to_if(rdpa_wan_gpon);
            }
            else if (strstr(if_p->ethIf.ifname, EPON_IFC_STR))
            {
                wan_if = rdpa_wan_type_to_if(rdpa_wan_epon);
            }

#ifdef REMOVE_WAN_QUEUE_ON_LINKDOWN
            if (getPonLinkStatusFlag() == FALSE)
#else
#ifdef BUILD_CUSTOMER
            if ((getPonLinkCfgFlag(rut_getRdpaWanType(wan_if)) == FALSE)
                 && (getPonWanStatusFlag() == FALSE))
#else
            if ((getPonLinkCfgFlag(rut_getRdpaWanType(wan_if)) == FALSE)
                 && (getPonLinkStatusFlag() == FALSE))
#endif
#endif /* REMOVE_WAN_QUEUE_ON_LINKDOWN */
            {
                applyFlag = FALSE;
            }
        }
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 || DMP_X_BROADCOM_COM_EPONWAN_1 */
#endif /* INIT_WAN_QUEUE_ON_LINKUP */
    }

    return applyFlag;
}

#if defined(DMP_X_BROADCOM_COM_GPONWAN_1) || defined(DMP_X_BROADCOM_COM_EPONWAN_1)
#ifdef INIT_WAN_QUEUE_ON_LINKUP
#ifndef BUILD_CUSTOMER
static BOOL getPonLinkStatusFlag(void)
{
    UBOOL8 upFlag = FALSE;

    if (rut_isWanTypeEpon() == TRUE)
    {
#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
        upFlag = getEponLinkStatusFlag();
#endif /* DMP_X_BROADCOM_COM_EPONWAN_1 */
    }
    else
    {
#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
        upFlag = getGponLinkStatusFlag();
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 */
    }

    return upFlag;
}
#endif /* BUILD_CUSTOMER */
#endif /* INIT_WAN_QUEUE_ON_LINKUP */

#ifndef REMOVE_WAN_QUEUE_ON_LINKDOWN
BOOL getPonLinkCfgFlag(int wan_type)
{
    int rootTmId = -1;
    BOOL found = FALSE;
    int rc;

    if ((rc = rdpaCtl_GetRootTm(RDPA_IOCTL_DEV_PORT, rdpa_wan_type_to_if(wan_type), &rootTmId, &found))) /* ??? */
    {
        cmsLog_error("rdpaCtl_GetRootTm(wan0) failed, rc=%d", rc);
        return FALSE;
    }

    if (!found || rootTmId < 0)
    {
        cmsLog_notice("Cannot find root tm for wan%d", rdpa_wan_type_to_if(wan_type));
        return FALSE;
    }

    return TRUE;
}
#endif /* REMOVE_WAN_QUEUE_ON_LINKDOWN */
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 || DMP_X_BROADCOM_COM_EPONWAN_1 */

#ifndef BUILD_CUSTOMER
#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
#ifndef DESKTOP_LINUX
BOOL getGponLinkEnableFlag(void)
{
#if defined(SUPPORT_DM_PURE181)
    InstanceIdStack optIntfIid = EMPTY_INSTANCE_ID_STACK;
    OpticalInterfaceObject *optIntfObj = NULL;
    UBOOL8 enableFlag = FALSE;

    if (rutOptical_getIntfByIfName(GPON_WAN_IF_NAME, &optIntfIid, &optIntfObj) == FALSE)
    {
        cmsLog_error("rutOptical_getIntfByIfName() did not find GPON interface");
        return enableFlag;
    }

    cmsLog_debug("GPON link enable=%d", optIntfObj->enable);
    enableFlag = optIntfObj->enable;
    cmsObj_free((void **) &optIntfObj);
    return enableFlag;
#else
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    WanGponLinkCfgObject *gponLinkCfgP = NULL;
    UBOOL8 enableFlag = FALSE;

    if (rutGpon_getGponLinkByIfName((char*)GPON_WAN_IF_NAME, &iidStack,
      &gponLinkCfgP) != TRUE)
    {
        cmsLog_error("rutGpon_getGponLinkByIfName() failed");
        return enableFlag;
    }

    cmsLog_debug("GPON link enable=%d", gponLinkCfgP->enable);
    enableFlag = gponLinkCfgP->enable;
    cmsObj_free((void**)&gponLinkCfgP);
    return enableFlag;
#endif
}
#endif /* DESKTOP_LINUX */

#ifdef INIT_WAN_QUEUE_ON_LINKUP
static BOOL getGponLinkStatusFlag(void)
{
#if defined(SUPPORT_DM_PURE181)
    InstanceIdStack optIntfIid = EMPTY_INSTANCE_ID_STACK;
    OpticalInterfaceObject *optIntfObj = NULL;
    UBOOL8 upFlag = FALSE;

    if (rutOptical_getIntfByIfName(GPON_WAN_IF_NAME, &optIntfIid, &optIntfObj) == FALSE)
    {
        cmsLog_error("rutOptical_getIntfByIfName() did not find GPON interface");
        return upFlag;
    }

    if (!(cmsUtl_strcmp(optIntfObj->status, MDMVS_UP)) &&
        (optIntfObj->enable == TRUE))
    {
        upFlag = TRUE;
    }

    cmsObj_free((void **) &optIntfObj);
    return upFlag;
#else
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    WanGponLinkCfgObject *gponLinkCfgP = NULL;
    UBOOL8 upFlag = FALSE;

    if (rutGpon_getGponLinkByIfName((char*)GPON_WAN_IF_NAME, &iidStack,
      &gponLinkCfgP) != TRUE)
    {
        cmsLog_error("rutGpon_getGponLinkByIfName() failed");
        return upFlag;
    }

    if ((!cmsUtl_strcmp(gponLinkCfgP->linkStatus, MDMVS_UP)) &&
      (gponLinkCfgP->enable == TRUE))
    {
        upFlag = TRUE;
    }

    cmsObj_free((void**)&gponLinkCfgP);
    return upFlag;
#endif
}
#endif /* INIT_WAN_QUEUE_ON_LINKUP */
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 */

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
#ifdef INIT_WAN_QUEUE_ON_LINKUP
static BOOL getEponLinkStatusFlag(void)
{
#if defined(SUPPORT_DM_PURE181)
    InstanceIdStack optIntfIid = EMPTY_INSTANCE_ID_STACK;
    OpticalInterfaceObject *optIntfObj = NULL;
    UBOOL8 upFlag = FALSE;

    if (rutOptical_getIntfByIfName(EPON_WAN_IF_NAME, &optIntfIid, &optIntfObj) == FALSE)
    {
        cmsLog_error("rutOptical_getIntfByIfName() did not find EPON interface");
        return upFlag;
    }

    if (!(cmsUtl_strcmp(optIntfObj->status, MDMVS_UP)) &&
        (optIntfObj->enable == TRUE))
    {
        upFlag = TRUE;
    }

    cmsObj_free((void **) &optIntfObj);
    return upFlag;
#else
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    WanEponLinkCfgObject *eponLinkCfgP = NULL;
    UBOOL8 upFlag = FALSE;

    if (rutEpon_getEponLinkByIfName((char*)EPON_WAN_IF_NAME, &iidStack,
      &eponLinkCfgP) != TRUE)
    {
        cmsLog_error("rutEpon_getEponLinkByIfName() failed");
        return upFlag;
    }

    if ((!cmsUtl_strcmp(eponLinkCfgP->linkStatus, MDMVS_UP)) &&
      (eponLinkCfgP->enable == TRUE))
    {
        upFlag = TRUE;
    }

    cmsObj_free((void**)&eponLinkCfgP);
    return upFlag;
#endif
}
#endif /* INIT_WAN_QUEUE_ON_LINKUP */
#endif /* DMP_X_BROADCOM_COM_EPONWAN_1 */
#endif /* BUILD_CUSTOMER */


/* upper leyer is unaware of the real number of queues,
*  priority is pushed up, so sp queues will be at the top */
static int adapt_prio(int prio, tmctl_devType_e devType, tmctl_if_t *if_p)
{
    tmctl_portTmParms_t tmParms_p;

    memset(&tmParms_p, 0x0, sizeof(tmctl_portTmParms_t));
    tmctl_getPortTmParms(devType, if_p, &tmParms_p);

    switch (tmParms_p.numQueues)
    {
        case 32:
            return prio + 24;
        case 16:
            return prio + 16;
        default:
            return prio;
    }
}

tmctl_ret_e rutwrap_tmctl_portTmInit(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, uint32_t cfgFlags)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        /* -1 will set num_queues to default */
        return tmctl_portTmInit(devType, if_p, cfgFlags, -1);
    }
}

tmctl_ret_e rutwrap_tmctl_portTmInitExt(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, uint32_t cfgFlags, int numQueues)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        return tmctl_portTmInit(devType, if_p, cfgFlags, numQueues);
    }
}

tmctl_ret_e rutwrap_tmctl_portTmUninit(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p)
{
    if (rut_tmctl_isUserTMCtlOwner(user) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        return tmctl_portTmUninit(devType, if_p);
    }
}

tmctl_ret_e rutwrap_tmctl_getQueueCfg(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, int queueId,
  tmctl_queueCfg_t *qcfg_p)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        return tmctl_getQueueCfg(devType, if_p, queueId, qcfg_p);
    }
}

tmctl_ret_e rutwrap_tmctl_delQueueCfg(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, int queueId)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        return tmctl_delQueueCfg(devType, if_p, queueId);
    }
}

tmctl_ret_e rutwrap_tmctl_getPortShaper(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, tmctl_shaper_t *shaper_p)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        return tmctl_getPortShaper(devType, if_p, shaper_p);
    }
}

tmctl_ret_e rutwrap_tmctl_setPortShaper(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, tmctl_shaper_t *shaper_p)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        return tmctl_setPortShaper(devType, if_p, shaper_p);
    }
}

tmctl_ret_e rutwrap_tmctl_getQueueDropAlg(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, int queueId,
  tmctl_queueDropAlg_t *dropAlg_p)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        return tmctl_getQueueDropAlg(devType, if_p, queueId, dropAlg_p);
    }
}

tmctl_ret_e rutwrap_tmctl_setQueueDropAlg(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, int queueId,
  tmctl_queueDropAlg_t *dropAlg_p)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        return tmctl_setQueueDropAlg(devType, if_p, queueId, dropAlg_p);
    }
}

tmctl_ret_e rutwrap_tmctl_getQueueStats(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, int queueId,
  tmctl_queueStats_t *stats_p)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        return tmctl_getQueueStats(devType, if_p, queueId, stats_p);
    }
}

tmctl_ret_e rutwrap_tmctl_getPortTmParms(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, tmctl_portTmParms_t *tmParms_p)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        return tmctl_getPortTmParms(devType, if_p, tmParms_p);
    }
}

tmctl_ret_e rutwrap_tmctl_setQueueCfg(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, tmctl_queueCfg_t *qcfg_p)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        qcfg_p->priority = adapt_prio(qcfg_p->priority, devType, if_p);
        return tmctl_setQueueCfg(devType, if_p, qcfg_p);
    }
}

tmctl_ret_e rutwrap_tmctl_setQueueDropAlgExt(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, int queueId,
  tmctl_queueDropAlg_t *dropAlg_p)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        return tmctl_setQueueDropAlgExt(devType, if_p, queueId, dropAlg_p);
    }
}

tmctl_ret_e rutwrap_tmctl_setQueueSize(eTmctlUser user,
  tmctl_devType_e devType, tmctl_if_t *if_p, int queueId, int size)
{
    if (applyTmctlCfg(user, devType, if_p) == FALSE)
    {
        cmsLog_debug("Skip TMCtl configuration, user=%d", user);
        return TMCTL_SUCCESS;
    }
    else
    {
        return tmctl_setQueueSize(devType, if_p, queueId, size);
    }
}

tmctl_ret_e rutwrap_tmctl_createPolicer(eTmctlUser user __attribute__((unused)), tmctl_policer_t *policer_p)
{
   return tmctl_createPolicer(policer_p);
}

tmctl_ret_e rutwrap_tmctl_modifyPolicer(eTmctlUser user __attribute__((unused)), tmctl_policer_t *policer_p)
{
	return tmctl_modifyPolicer(policer_p);
}

tmctl_ret_e rutwrap_tmctl_deletePolicer(eTmctlUser user __attribute__((unused)), tmctl_dir_e dir, int policerId)
{
	return tmctl_deletePolicer(dir, policerId);
}

#endif /* SUPPORT_TMCTL */
