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

/*!\file rcl2_wifi_accesspoint.c
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
#include "rut2_util.h"
#include "rut2_wifi.h"
#include "cms_qdm.h"
#include <wlcsm_linux.h>
#include <wlcsm_lib_api.h>

CmsRet rcl_dev2WifiAccessPointObject( _Dev2WifiAccessPointObject *newObj,
                                      const _Dev2WifiAccessPointObject *currObj,
                                      const InstanceIdStack *iidStack __attribute__((unused)),
                                      char **errorParam __attribute__((unused)),
                                      CmsRet *errorCode __attribute__((unused)))
{
    /* UINT32 radioIndex; */
    CmsRet ret = CMSRET_SUCCESS;

    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumWifiAccessPoint(iidStack, 1);
        if (mdmLibCtx.eid == EID_SSK)
            return CMSRET_SUCCESS;
        else
            return CMSRET_REQUEST_DENIED;
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumWifiAccessPoint(iidStack, -1);
        return CMSRET_SUCCESS;
    }

    if(!newObj->enable && currObj->enable) {
        rutWifi_Clear_AssocicatedDevices(newObj,iidStack);
    }
    if (mdmLibCtx.eid != EID_WLMNGR && mdmLibCtx.eid  != EID_SSK)
    {
        UINT32 radioIndex = 0;
        paramNodeList *changedParams = NULL;

        radioIndex = newObj->X_BROADCOM_COM_Adapter;

        if ((ret = cmsObj_compareObjects(newObj, currObj, &changedParams)) == CMSRET_SUCCESS)
        {
            int apIndex = newObj->X_BROADCOM_COM_Index;
            if (changedParams != NULL)
            {
                ret = rutWifi_updateWlmngr(MDMOID_DEV2_WIFI_ACCESS_POINT, radioIndex, apIndex, 0, changedParams);

                while (changedParams)
                {
                    paramNodeList *tmpParamNode = changedParams;
                    changedParams = tmpParamNode->nextNode;
                    cmsMem_free(tmpParamNode);
                }
            }
        }
    }

    return ret;
}


CmsRet rcl_dev2WifiAccessPointSecurityObject( _Dev2WifiAccessPointSecurityObject *newObj __attribute__((unused)),
        const _Dev2WifiAccessPointSecurityObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    UINT32 radioIndex = 0;
    UINT32 apIndex = 0;
    Dev2WifiAccessPointObject *wifiAccessPointObj;

    paramNodeList *changedParams = NULL;
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj)) {
        return CMSRET_SUCCESS;
    }
    else if (DELETE_EXISTING(newObj, currObj)) {
        return CMSRET_SUCCESS;
    }


    if (mdmLibCtx.eid != EID_WLMNGR && (newObj && currObj)) {

        cmsLog_debug("....newObj->reset:%d\n",newObj->reset);

        if (cmsObj_getAncestor(MDMOID_DEV2_WIFI_ACCESS_POINT,
                               MDMOID_DEV2_WIFI_ACCESS_POINT_SECURITY,
                               iidStack,(void **)&wifiAccessPointObj) != CMSRET_SUCCESS) {

            return CMSRET_INVALID_PARAM_VALUE;
        }

        radioIndex = wifiAccessPointObj->X_BROADCOM_COM_Adapter;
        apIndex = wifiAccessPointObj->X_BROADCOM_COM_Index;

        if (newObj->reset) {
            _Dev2WifiAccessPointSecurityObject *defaultObj = NULL;
            ret = cmsObj_get(MDMOID_DEV2_WIFI_ACCESS_POINT_SECURITY, iidStack, OGF_DEFAULT_VALUES, (void **)&defaultObj);
            if (ret != CMSRET_SUCCESS) {
                cmsObj_free((void **)&wifiAccessPointObj);
                return ret;
            }
            newObj->reset = FALSE;
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->modeEnabled, defaultObj->modeEnabled, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, defaultObj->wlAuthAkm, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode,defaultObj->wlAuthMode, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->WEPKey, defaultObj->WEPKey, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->keyPassphrase, defaultObj->keyPassphrase, mdmLibCtx.allocFlags);
            newObj->rekeyingInterval = defaultObj->rekeyingInterval;

            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->radiusServerIPAddr, defaultObj->radiusServerIPAddr, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->secondaryRadiusServerIPAddr, defaultObj->secondaryRadiusServerIPAddr, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->radiusSecret, defaultObj->radiusSecret, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->secondaryRadiusSecret, defaultObj->radiusSecret, mdmLibCtx.allocFlags);
            newObj->radiusServerPort= defaultObj->radiusServerPort;
            newObj->secondaryRadiusServerPort = defaultObj->secondaryRadiusServerPort;

            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey1, defaultObj->X_BROADCOM_COM_WlKey1, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey2, defaultObj->X_BROADCOM_COM_WlKey2, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey3, defaultObj->X_BROADCOM_COM_WlKey3, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey4, defaultObj->X_BROADCOM_COM_WlKey4, mdmLibCtx.allocFlags);
            newObj->X_BROADCOM_COM_WlKeyIndex = defaultObj->X_BROADCOM_COM_WlKeyIndex;
            newObj->X_BROADCOM_COM_WlKeyBit = defaultObj->X_BROADCOM_COM_WlKeyBit;

            cmsObj_free((void **)&defaultObj);
        } else {
            if (cmsUtl_strcmp(newObj->modeEnabled, currObj->modeEnabled)) {

                _Dev2WifiRadioObject *radioObj = NULL;
                InstanceIdStack radio_iidStack = EMPTY_INSTANCE_ID_STACK;

                cmsLog_debug("radioIndex:%d\n",radioIndex);

                if((ret=rutWifi_get_AP_Radio_dev2(wifiAccessPointObj,(void **)&radioObj,&radio_iidStack))!=CMSRET_SUCCESS) {
                    //if((ret=cmsObj_get(MDMOID_DEV2_WIFI_RADIO,&radio_iidStack,0,(void **)&radioObj))!=CMSRET_SUCCESS) {
                    cmsLog_debug("....failed to get radio:%d...\n",ret);
                    cmsObj_free((void **)&wifiAccessPointObj);
                    return CMSRET_INVALID_PARAM_VALUE;
                }

                if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WEP_64) || !cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WEP_128)) {

                    if(cmsUtl_strcmp(radioObj->X_BROADCOM_COM_WlNmode,"off")) {
                        cmsLog_debug("set wlNmod set off");
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(radioObj->X_BROADCOM_COM_WlNmode, "off", mdmLibCtx.allocFlags);
                        cmsObj_set(radioObj,&radio_iidStack);
                    } else
                        cmsLog_debug("already nmode off?");

                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, STR_OPEN, mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, STR_OPEN, mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, STR_ENABLED, mdmLibCtx.allocFlags);
                } else {

                    cmsLog_debug("....\n");
                    if(!cmsUtl_strcmp(radioObj->X_BROADCOM_COM_WlNmode,"auto")) {
                        cmsLog_debug("set wlNmode to auto\n");
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(radioObj->X_BROADCOM_COM_WlNmode, "auto", mdmLibCtx.allocFlags);
                        cmsObj_set(radioObj,&radio_iidStack);
                    } else
                        cmsLog_debug("nmode is arleady auto\n");

                    if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_NONE)) {
                        /* if modeEnable is NONE */
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, STR_OPEN, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, STR_OPEN, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, "disabled", mdmLibCtx.allocFlags);

                    } else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA2_PERSONAL)) {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, STR_AKM_PSK2, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "none", mdmLibCtx.allocFlags);
                    } else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA_WPA2_PERSONAL)) {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, STR_AKM_PSK_PSK2, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "none", mdmLibCtx.allocFlags);
                    } else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA2_ENTERPRISE)) {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, STR_AKM_WPA2, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "radius", mdmLibCtx.allocFlags);
                    } else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA_WPA2_ENTERPRISE)) {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, STR_AKM_WPA_WPA2, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "radius", mdmLibCtx.allocFlags);
                    } else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA_PERSONAL)) {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, STR_AKM_PSK, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, STR_OPEN, mdmLibCtx.allocFlags);
                    } else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA_ENTERPRISE)) {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, STR_AKM_WPA, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "radius", mdmLibCtx.allocFlags);
                    } 

                }

                cmsObj_free((void **)&radioObj);
            }
            if (cmsUtl_strcmp(newObj->WEPKey, currObj->WEPKey)) {
                char *tempbuf=NULL;
                int buflen=0;
                cmsLog_debug("newObj->WEPKey:%s and currObj->WEPKey:%s\n",newObj->WEPKey,currObj->WEPKey);
                cmsUtl_hexStringToBinaryBuf(newObj->WEPKey,&tempbuf,&buflen);
                if(buflen)
                    cmsLog_debug("newkey binary:%s\n",tempbuf);

                switch(newObj->X_BROADCOM_COM_WlKeyIndex)
                {
                case 1:
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey1, newObj->WEPKey, mdmLibCtx.allocFlags);
                    break;
                case 2:
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey2, newObj->WEPKey, mdmLibCtx.allocFlags);
                    break;
                case 3:
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey3, newObj->WEPKey, mdmLibCtx.allocFlags);
                    break;
                case 4:
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey4, newObj->WEPKey, mdmLibCtx.allocFlags);
                    break;
                default:
                    newObj->X_BROADCOM_COM_WlKeyIndex = 1;
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey1, newObj->WEPKey, mdmLibCtx.allocFlags);
                    break;
                }
            }
        }

        if ((mdmLibCtx.eid != EID_SSK) &&
                (ret = cmsObj_compareObjects(newObj, currObj, &changedParams)) == CMSRET_SUCCESS && changedParams) {

            ret = rutWifi_updateWlmngr(MDMOID_DEV2_WIFI_ACCESS_POINT_SECURITY, radioIndex, apIndex, 0, changedParams);

            while (changedParams)
            {
                paramNodeList *tmpParamNode = changedParams;
                changedParams = tmpParamNode->nextNode;
                cmsMem_free(tmpParamNode);
            }
        }
        cmsObj_free((void **) &wifiAccessPointObj);
    }

    return ret;
}


CmsRet rcl_dev2WifiAccessPointAccountingObject( _Dev2WifiAccessPointAccountingObject *newObj __attribute__((unused)),
        const _Dev2WifiAccessPointAccountingObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
     
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiAccessPointWpsObject( _Dev2WifiAccessPointWpsObject *newObj __attribute__((unused)),
        const _Dev2WifiAccessPointWpsObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        return CMSRET_SUCCESS;
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        return CMSRET_SUCCESS;
    }

    if ( newObj )
    {
        if (!IS_EMPTY_STRING(newObj->configMethodsSupported) && !IS_EMPTY_STRING(newObj->configMethodsEnabled))
        {
            if (cmsUtl_strstr(newObj->configMethodsSupported, newObj->configMethodsEnabled) == NULL)
            {
                return CMSRET_INVALID_PARAM_VALUE;
            }
        }
    }


    if (mdmLibCtx.eid == EID_TR69C || mdmLibCtx.eid == EID_CWMPD)
    {
        UINT32 radioIndex = 0;
        UINT32 apIndex = 0;
        paramNodeList *changedParams = NULL;

        Dev2WifiAccessPointObject *wifiAccessPointObj;

        if (cmsObj_get(MDMOID_DEV2_WIFI_ACCESS_POINT, iidStack, 0, (void *) &wifiAccessPointObj) == CMSRET_SUCCESS)
        {
            radioIndex = wifiAccessPointObj->X_BROADCOM_COM_Adapter;
            apIndex = wifiAccessPointObj->X_BROADCOM_COM_Index;
            cmsObj_free((void **) &wifiAccessPointObj);
        }

        if (newObj)
        {
            if (newObj->enable)
            {
                CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_Wsc_mode, "enabled", mdmLibCtx.allocFlags);
            }
            else
            {
                CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_Wsc_mode, "disabled", mdmLibCtx.allocFlags);
            }
        }

        if ((ret = cmsObj_compareObjects(newObj, currObj, &changedParams)) == CMSRET_SUCCESS)
        {
            if (changedParams != NULL)
            {
                ret = rutWifi_updateWlmngr(MDMOID_DEV2_WIFI_ACCESS_POINT_WPS, radioIndex, apIndex, 0, changedParams);

                while (changedParams)
                {
                    paramNodeList *tmpParamNode = changedParams;
                    changedParams = tmpParamNode->nextNode;
                    cmsMem_free(tmpParamNode);
                }
            }
        }
    }

    return ret;
}


CmsRet rcl_dev2WifiAssociatedDeviceObject( _Dev2WifiAssociatedDeviceObject *newObj __attribute__((unused)),
        const _Dev2WifiAssociatedDeviceObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumWifiAssociatedDevice(iidStack, 1);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumWifiAssociatedDevice(iidStack, -1);
        rutWifi_update_STA_HostEntry(currObj,iidStack,MDM_INVALID_ONLY);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiAssociateddeviceStatsObject( _Dev2WifiAssociateddeviceStatsObject *newObj __attribute__((unused)),
        const _Dev2WifiAssociateddeviceStatsObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2WifiAccessPointAcObject ( _Dev2WifiAccessPointAcObject *newObj __attribute__((unused)),
        const _Dev2WifiAccessPointAcObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiAccessPointAcStatsObject ( _Dev2WifiAccessPointAcStatsObject *newObj __attribute__((unused)),
        const _Dev2WifiAccessPointAcStatsObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


#endif  /* DMP_DEVICE2_WIFIACCESSPOINT_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */
