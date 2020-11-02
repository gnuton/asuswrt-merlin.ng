/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
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

#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_OPTICAL_1


#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_qos.h"

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
#include "rut_eponwan.h"
#endif

#include "rdpa_types.h"
#include "rdpactl_api.h"

#if defined(EPON_HGU) || defined(GPON_HGU)
static CmsRet reconfigure_queues(const char *ifname, UBOOL8 enabled)
{
    CmsRet ret = CMSRET_SUCCESS;

    if (enabled)
    {
        cmsLog_debug("Enabling TM for interface %s", ifname);

        if ((ret = rutQos_tmPortUninit(ifname, TRUE)) != CMSRET_SUCCESS)
        {
            cmsLog_error("rutQos_tmPortUninit failed, ret=%d", ret);
            goto Exit;
        }

        if ((ret = rutQos_tmPortInit(ifname, TRUE)) != CMSRET_SUCCESS)
        {
            cmsLog_error("rutQos_tmPortInit failed, ret=%d", ret);
            goto Exit;
        }

#ifdef DMP_DEVICE2_QOS_1
        rutQos_reconfigAllQueuesOnLayer2Intf_dev2(ifname);
#endif
    }
    else
    {
        cmsLog_debug("Disabling TM for interface %s", ifname);

        if ((ret = rutQos_tmPortUninit(ifname, TRUE)) != CMSRET_SUCCESS)
            goto Exit;
    }

Exit:
    return ret;
}
#endif

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
static CmsRet handle_epon_change(const char *ifname, UBOOL8 enabled)
{
    CmsRet ret = CMSRET_SUCCESS;

#ifdef EPON_SFU
    if (enabled)
    {
        cmsLog_debug("Enabling EPON interface %s", ifname);
        rutLan_enableInterface(ifname);
    }
    else
    {
        cmsLog_debug("Disabling EPON interface %s", ifname);
        rutLan_disableInterface(ifname);
    }
#endif
#ifdef EPON_HGU
    XponObject *xponObj = NULL;
    InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;

    if ((ret = cmsObj_get(MDMOID_XPON, &iidStack2, 0, (void **) &xponObj) != CMSRET_SUCCESS))
    {
        cmsLog_error("get of MDMOID_XPON object failed, ret=%d", ret);
        goto Exit;
    }

    if (xponObj->oamSelection & OAM_DPOE_SUPPORT)
        rdpaCtl_set_epon_mode(rdpa_epon_dpoe);
    else if (xponObj->oamSelection & OAM_BCM_SUPPORT)
        rdpaCtl_set_epon_mode(rdpa_epon_bcm);
    else if (xponObj->oamSelection & OAM_CUC_SUPPORT)
        rdpaCtl_set_epon_mode(rdpa_epon_cuc_dyn);
    else
        rdpaCtl_set_epon_mode(rdpa_epon_ctc_dyn);

    ret = reconfigure_queues(ifname, enabled);
#endif

Exit:
    return ret;
}
#endif

#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
static CmsRet handle_gpon_change(const char *ifname, UBOOL8 enabled)
{
    CmsRet ret = CMSRET_SUCCESS;

#ifdef GPON_HGU
    ret = reconfigure_queues(ifname, enabled);
#endif

    return ret;
}
#endif


CmsRet rcl_deviceOpticalObject( _DeviceOpticalObject *newObj __attribute__((unused)),
    const _DeviceOpticalObject *currObj __attribute__((unused)),
    const InstanceIdStack *iidStack __attribute__((unused)),
    char **errorParam __attribute__((unused)),
    CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_opticalInterfaceObject( _OpticalInterfaceObject *newObj __attribute__((unused)),
    const _OpticalInterfaceObject *currObj __attribute__((unused)),
    const InstanceIdStack *iidStack __attribute__((unused)),
    char **errorParam __attribute__((unused)),
    CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    UBOOL8 new_enabled, curr_enabled;
    rdpa_wan_type wan_type;

    new_enabled = newObj && newObj->enable && newObj->status && !strcmp(newObj->status, MDMVS_UP);
    curr_enabled = currObj && currObj->enable && currObj->status && !strcmp(currObj->status, MDMVS_UP);

    if (new_enabled == curr_enabled)
    {
        cmsLog_debug("WAN EPON status did not change");
        goto Exit;
    }

    if ((ret = rdpaCtl_get_wan_type(rdpa_wan_type_to_if(rdpa_wan_gpon), &wan_type)) != CMSRET_SUCCESS)
    {
        if ((ret = rdpaCtl_get_wan_type(rdpa_wan_type_to_if(rdpa_wan_epon), &wan_type)) != CMSRET_SUCCESS)
        {
            cmsLog_debug("Could not get WAN type. ret=%d", ret);
            goto Exit;
        }
    }

    switch (wan_type)
    {
#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
    case rdpa_wan_epon:
    case rdpa_wan_xepon:
        ret = handle_epon_change(new_enabled ? newObj->name : currObj->name, new_enabled);
        break;
#endif
#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
    case rdpa_wan_gpon:
    case rdpa_wan_xgpon:   
        ret = handle_gpon_change(new_enabled ? newObj->name : currObj->name, new_enabled);
        break;
#endif
    default:
        break;
    }

Exit:
    return ret;
}

CmsRet rcl_opticalInterfaceStatsObject( _OpticalInterfaceStatsObject *newObj __attribute__((unused)),
    const _OpticalInterfaceStatsObject *currObj __attribute__((unused)),
    const InstanceIdStack *iidStack __attribute__((unused)),
    char **errorParam __attribute__((unused)),
    CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

#endif    // DMP_DEVICE2_OPTICAL_1

#endif  /* DMP_DEVICE2_BASELINE_1 */
