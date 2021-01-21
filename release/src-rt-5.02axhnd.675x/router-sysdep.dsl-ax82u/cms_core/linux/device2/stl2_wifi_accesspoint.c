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

#ifdef DMP_DEVICE2_WIFIACCESSPOINT_1

/*!\file stl2_wifi_accesspoint.c
 * \brief This file contains TR181 and X_BROADCOM_COM Wifi Access Point (LAN)
 *        side functions.  General Wifi objects (wifi and radio) and
 *        End Point specific functions are in separate files.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"

CmsRet stl_dev2WifiAccessPointObject( _Dev2WifiAccessPointObject *obj __attribute__((unused)),
                                      const InstanceIdStack *iidStack __attribute__((unused)))
{
    Dev2WifiSsidObject *ssidObj = rutWifi_get_AP_SSID_dev2(obj);
    if(ssidObj) {
        if(!cmsUtl_strcmp(ssidObj->status,MDMVS_UP)) {
            CMSMEM_REPLACE_STRING_FLAGS(obj->status,MDMVS_ENABLED,mdmLibCtx.allocFlags);
        } else
            CMSMEM_REPLACE_STRING_FLAGS(obj->status,MDMVS_DISABLED,mdmLibCtx.allocFlags);
        cmsObj_free((void **)&ssidObj);
    }
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;;

}


CmsRet stl_dev2WifiAccessPointSecurityObject( _Dev2WifiAccessPointSecurityObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;;
}

CmsRet stl_dev2WifiAccessPointAccountingObject( _Dev2WifiAccessPointAccountingObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;;
}

CmsRet stl_dev2WifiAccessPointWpsObject( _Dev2WifiAccessPointWpsObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;;
}


CmsRet stl_dev2WifiAssociatedDeviceObject( _Dev2WifiAssociatedDeviceObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiAssociateddeviceStatsObject( _Dev2WifiAssociateddeviceStatsObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2WifiAccessPointAcObject ( _Dev2WifiAccessPointAcObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiAccessPointAcStatsObject ( _Dev2WifiAccessPointAcStatsObject *obj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif  /* DMP_DEVICE2_WIFIACCESSPOINT_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */
