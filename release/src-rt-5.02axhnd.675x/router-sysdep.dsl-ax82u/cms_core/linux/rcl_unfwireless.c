/***********************************************************************
 *
 *  Copyright (c) 2018  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

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

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_pmap.h"
#include "rut_lan.h"
#include "rut_unfwlan.h"
#include "rut_wifiwan.h"
#include "wifi_constants.h"
#include "device2/rut2_unfwifi.h"

#ifdef BRCM_WLAN
#ifdef DMP_WIFILAN_1

CmsRet rcl_lanWlanObject(_LanWlanObject *newObj,
                         const _LanWlanObject *currObj,
                         const InstanceIdStack *iidStack,
                         char **errorParam __attribute__((unused)),
                         CmsRet *errorCode __attribute__((unused)))
{
    _WlBaseCfgObject *cfgObj;
    _WlVirtIntfCfgObject *virtObj;
    InstanceIdStack iidStackVirt;
    const char *wlAuthMode, *crypto;
    UBOOL8 wepEnabled;

    CmsRet ret = CMSRET_SUCCESS;

    if ((newObj != NULL) && (currObj != NULL))
    {
        ret = cmsObj_get(MDMOID_WL_BASE_CFG, iidStack, OGF_NO_VALUE_UPDATE, (void **)&cfgObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to get MDMOID_WL_BASE_CFG object");
            return ret;
        }

        iidStackVirt = *iidStack;
        PUSH_INSTANCE_ID(&iidStackVirt, 1);
        ret = cmsObj_get(MDMOID_WL_VIRT_INTF_CFG, &iidStackVirt, OGF_NO_VALUE_UPDATE, (void **)&virtObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to get VIRT_INTF_CFG object instance 1");
            cmsObj_free((void **)&cfgObj);
            return ret;
        }

        if (currObj->enable != newObj->enable)
        {
            cfgObj->wlEnbl = newObj->enable ? 1 : 0;
            //add radioEnabled for pass cdrouter test
            newObj->radioEnabled = newObj->enable;
        }
        //add radioEnabled for pass cdrouter test
        if (currObj->radioEnabled != newObj->radioEnabled)
        {
            cfgObj->wlEnbl = newObj->radioEnabled ? 1 : 0;
            newObj->enable = newObj->radioEnabled;
        }

        if (currObj->channel != newObj->channel)
        {
            cfgObj->wlChannel = newObj->channel;
        }

        if (0 != cmsUtl_strcmp(currObj->maxBitRate, newObj->maxBitRate))
        {
            cfgObj->wlRate = rutWlan_getWlRate(newObj->maxBitRate);
        }

        if (0 != cmsUtl_strcmp(currObj->BSSID, newObj->BSSID))
        {
            CMSMEM_REPLACE_STRING_FLAGS(virtObj->wlBssMacAddr, newObj->BSSID, mdmLibCtx.allocFlags);
        }

        if (0 != cmsUtl_strcmp(currObj->SSID, newObj->SSID))
        {
            CMSMEM_REPLACE_STRING_FLAGS(virtObj->wlSsid, newObj->SSID, mdmLibCtx.allocFlags);
        }

        if (currObj->SSIDAdvertisementEnabled != newObj->SSIDAdvertisementEnabled)
        {
            virtObj->wlHide = newObj->SSIDAdvertisementEnabled ? 0 : 1;
        }

        if (currObj->MACAddressControlEnabled != newObj->MACAddressControlEnabled)
        {
            const char *wlFltMacMode;
            wlFltMacMode = (newObj->MACAddressControlEnabled == 1) ? WL_FLT_MAC_ALLOW : WL_FLT_MAC_DENY;
            CMSMEM_REPLACE_STRING_FLAGS(virtObj->wlFltMacMode, wlFltMacMode, mdmLibCtx.allocFlags);
        }

        if (0 != cmsUtl_strcmp(currObj->standard, newObj->standard))
        {
            rutWlan_getWlBandGmode(newObj->standard, &(cfgObj->wlBand), &(cfgObj->wlgMode));
        }

        if (0 != cmsUtl_strcmp(currObj->regulatoryDomain, newObj->regulatoryDomain))
        {
            CMSMEM_REPLACE_STRING_FLAGS(cfgObj->wlCountry,
                                        newObj->regulatoryDomain,
                                        mdmLibCtx.allocFlags);
        }

        if (currObj->WEPKeyIndex != newObj->WEPKeyIndex)
        {
            SINT32 len;
            InstanceIdStack WEPKeyStack = *iidStack;
            PUSH_INSTANCE_ID(&WEPKeyStack, newObj->WEPKeyIndex);
            _LanWlanWepKeyObject *WEPKeyObj;

            ret = cmsObj_get(MDMOID_LAN_WLAN_WEP_KEY, &WEPKeyStack, OGF_NO_VALUE_UPDATE, (void **)&WEPKeyObj);
            if (ret == CMSRET_SUCCESS)
            {
                len = cmsUtl_strlen(WEPKeyObj->WEPKey);
                if (len == 5 || len == 10)
                {
                    virtObj->wlKeyBit = WL_BIT_KEY_64;
                    virtObj->wlKeyIndex64 = newObj->WEPKeyIndex;
                    CMSMEM_REPLACE_STRING_FLAGS(newObj->WEPEncryptionLevel, "40-bit", mdmLibCtx.allocFlags);
                }
                else if (len == 13 || len == 26)
                {
                    virtObj->wlKeyBit = WL_BIT_KEY_128;
                    virtObj->wlKeyIndex128 = newObj->WEPKeyIndex;
                    CMSMEM_REPLACE_STRING_FLAGS(newObj->WEPEncryptionLevel, "104-bit", mdmLibCtx.allocFlags);
                }
                else
                {
                    virtObj->wlKeyBit = WL_BIT_KEY_128;
                    virtObj->wlKeyIndex128 = newObj->WEPKeyIndex;
                    CMSMEM_REPLACE_STRING_FLAGS(newObj->WEPEncryptionLevel, "Disabled", mdmLibCtx.allocFlags);
                }
                cmsObj_free((void **)&WEPKeyObj);
            }
            else
            {
                cmsLog_error("Failed to get wep key instance %d", newObj->WEPKeyIndex);
            }
        }

        if (0 != cmsUtl_strcmp(currObj->X_BROADCOM_COM_IfName,
                               newObj->X_BROADCOM_COM_IfName))
        {
            CMSMEM_REPLACE_STRING_FLAGS(virtObj->wlIfcname,
                                        newObj->X_BROADCOM_COM_IfName,
                                        mdmLibCtx.allocFlags);
        }

        if (currObj->X_BROADCOM_COM_HideSSID !=
            newObj->X_BROADCOM_COM_HideSSID)
        {
            virtObj->wlHide = newObj->X_BROADCOM_COM_HideSSID;
        }

        if (currObj->X_BROADCOM_COM_TxPowerPercent !=
            newObj->X_BROADCOM_COM_TxPowerPercent)
        {
            cfgObj->wlTxPwrPcnt = newObj->X_BROADCOM_COM_TxPowerPercent;
        }

        if (0 != cmsUtl_strcmp(currObj->beaconType, newObj->beaconType)
            || 0 != cmsUtl_strcmp(currObj->basicEncryptionModes, newObj->basicEncryptionModes)
            || 0 != cmsUtl_strcmp(currObj->WPAEncryptionModes, newObj->WPAEncryptionModes)
            || 0 != cmsUtl_strcmp(currObj->IEEE11iEncryptionModes, newObj->IEEE11iEncryptionModes)
            || 0 != cmsUtl_strcmp(currObj->basicAuthenticationMode, newObj->basicAuthenticationMode)
            || 0 != cmsUtl_strcmp(currObj->WPAAuthenticationMode, newObj->WPAAuthenticationMode)
            || 0 != cmsUtl_strcmp(currObj->IEEE11iAuthenticationMode, newObj->IEEE11iAuthenticationMode))
        {
            wepEnabled = rutWlan_getWepEnabled(newObj->beaconType,
                                               newObj->basicEncryptionModes,
                                               newObj->WPAEncryptionModes,
                                               newObj->IEEE11iEncryptionModes);

            //set virtObj->wlWep for pass cdrouter test
            if (wepEnabled)
                CMSMEM_REPLACE_STRING_FLAGS(virtObj->wlWep, "enabled", mdmLibCtx.allocFlags);
            else
                CMSMEM_REPLACE_STRING_FLAGS(virtObj->wlWep, "disable", mdmLibCtx.allocFlags);

            wlAuthMode = rutWlan_getWlAuthMode(newObj->beaconType,
                                               newObj->basicAuthenticationMode,
                                               newObj->WPAAuthenticationMode,
                                               newObj->IEEE11iAuthenticationMode,
                                               wepEnabled);

            CMSMEM_REPLACE_STRING_FLAGS(virtObj->wlAuthMode, wlAuthMode, mdmLibCtx.allocFlags);

            crypto = rutWlan_getCrypto(newObj->beaconType,
                                       newObj->WPAEncryptionModes,
                                       newObj->IEEE11iEncryptionModes);

            CMSMEM_REPLACE_STRING_FLAGS(virtObj->wlWpa, crypto, mdmLibCtx.allocFlags);
        }

        if (0 != cmsUtl_strcmp(currObj->basicDataTransmitRates,
                                newObj->basicDataTransmitRates))
        {
            const char *wlBasicRate;
            wlBasicRate = rutWlan_getWlBasicRate(newObj->basicDataTransmitRates,
                                                 cfgObj->wlBand, cfgObj->wlgMode);
            CMSMEM_REPLACE_STRING_FLAGS(cfgObj->wlBasicRate, wlBasicRate,
                                        mdmLibCtx.allocFlags);
        }

        if (0 != cmsUtl_strcmp(currObj->deviceOperationMode,
                               newObj->deviceOperationMode))
        {
            const char *wlMode;
            wlMode = rutWlan_getWlMode(newObj->deviceOperationMode);
            CMSMEM_REPLACE_STRING_FLAGS(cfgObj->wlMode,
                                        wlMode,
                                        mdmLibCtx.allocFlags);
        }

        ret = cmsObj_set(virtObj, &iidStackVirt);
        if (ret == CMSRET_SUCCESS)
        {
            ret = cmsObj_set(cfgObj, iidStack);
        }

        cmsObj_free((void **)&virtObj);
        cmsObj_free((void **)&cfgObj);
    }
    return ret;
}

CmsRet rcl_lanWlanAssociatedDeviceEntryObject( _LanWlanAssociatedDeviceEntryObject *newObj __attribute__((unused)),
        const _LanWlanAssociatedDeviceEntryObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_lanWlanWepKeyObject( _LanWlanWepKeyObject *newObj,
                                const _LanWlanWepKeyObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
    InstanceIdStack iidStackVirt;
    InstanceIdStack iidStackWlKey;
    _WlVirtIntfCfgObject *virtObj;
    void *wlKeyCfgObj;
    UINT32 keyIndex;
    CmsRet ret = CMSRET_SUCCESS;

    if ((newObj != NULL) && (currObj != NULL))
    {
        keyIndex = PEEK_INSTANCE_ID(iidStack);

        iidStackVirt = *iidStack;
        POP_INSTANCE_ID(&iidStackVirt);
        PUSH_INSTANCE_ID(&iidStackVirt, 1);
        ret = cmsObj_get(MDMOID_WL_VIRT_INTF_CFG, &iidStackVirt,
                         OGF_NO_VALUE_UPDATE, (void **)&virtObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to get VIRT_INTF_CFG object instance 1");
            return ret;
        }

        iidStackWlKey = *iidStack;
        POP_INSTANCE_ID(&iidStackWlKey);
        PUSH_INSTANCE_ID(&iidStackWlKey, 1);
        PUSH_INSTANCE_ID(&iidStackWlKey, keyIndex);
        if (virtObj->wlKeyBit == WL_BIT_KEY_64)
        {
            ret = cmsObj_get(MDMOID_WL_KEY64_CFG, &iidStackWlKey,
                             OGF_NO_VALUE_UPDATE, (void **)&wlKeyCfgObj);
        }
        else
        {
            ret = cmsObj_get(MDMOID_WL_KEY128_CFG, &iidStackWlKey,
                             OGF_NO_VALUE_UPDATE, (void **)&wlKeyCfgObj);
        }

        if (ret != CMSRET_SUCCESS)
        {
            cmsObj_free((void **)&virtObj);
            return ret;
        }

        if (0 != cmsUtl_strcmp(currObj->WEPKey, newObj->WEPKey))
        {
            if (virtObj->wlKeyBit == WL_BIT_KEY_64)
            {
                _WlKey64CfgObject *wlKey64CfgObj;
                wlKey64CfgObj = (_WlKey64CfgObject *)wlKeyCfgObj;
                CMSMEM_REPLACE_STRING_FLAGS(wlKey64CfgObj->wlKey64,
                                            newObj->WEPKey,
                                            mdmLibCtx.allocFlags);
            }
            else
            {
                _WlKey128CfgObject *wlKey128CfgObj;
                wlKey128CfgObj = (_WlKey128CfgObject *)wlKeyCfgObj;
                CMSMEM_REPLACE_STRING_FLAGS(wlKey128CfgObj->wlKey128,
                                            newObj->WEPKey,
                                            mdmLibCtx.allocFlags);
            }
        }

        ret = cmsObj_set(wlKeyCfgObj, &iidStackWlKey);

        cmsObj_free((void **)&virtObj);
        cmsObj_free((void **)&wlKeyCfgObj);
        rut2_sendWifiChange();
    }
    return ret;
}


CmsRet rcl_lanWlanPreSharedKeyObject( _LanWlanPreSharedKeyObject *newObj,
                                      const _LanWlanPreSharedKeyObject *currObj,
                                      const InstanceIdStack *iidStack,
                                      char **errorParam __attribute__((unused)),
                                      CmsRet *errorCode __attribute__((unused)))
{
    InstanceIdStack iidStackVirt;
    _WlVirtIntfCfgObject *virtObj;
    UINT32 keyIndex;
    CmsRet ret = CMSRET_SUCCESS;

    if ((newObj != NULL) && (currObj != NULL))
    {
        keyIndex = PEEK_INSTANCE_ID(iidStack);
        if (keyIndex != 1)
        {
            return CMSRET_SUCCESS;
        }

        iidStackVirt = *iidStack;
        POP_INSTANCE_ID(&iidStackVirt);
        PUSH_INSTANCE_ID(&iidStackVirt, 1);
        ret = cmsObj_get(MDMOID_WL_VIRT_INTF_CFG, &iidStackVirt,
                         OGF_NO_VALUE_UPDATE, (void **)&virtObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to get VIRT_INTF_CFG object instance 1");
            return ret;
        }

        if (0 != cmsUtl_strcmp(currObj->preSharedKey, newObj->preSharedKey))
        {
            CMSMEM_REPLACE_STRING_FLAGS(virtObj->wlWpaPsk,
                                        newObj->preSharedKey,
                                        mdmLibCtx.allocFlags);
        }
        //use obj->keyPassphrase in cdrouter test
        if (0 != cmsUtl_strcmp(currObj->keyPassphrase, newObj->keyPassphrase))
        {
            CMSMEM_REPLACE_STRING_FLAGS(virtObj->wlWpaPsk,
                                        newObj->keyPassphrase,
                                        mdmLibCtx.allocFlags);
        }
        ret = cmsObj_set(virtObj, &iidStackVirt);
        cmsObj_free((void **)&virtObj);
        rut2_sendWifiChange();
    }
    return ret;
}

#endif /* DMP_WIFILAN_1 */

#ifdef DMP_X_BROADCOM_COM_WIFILAN_1
CmsRet rcl_wlanAdapterObject( _WlanAdapterObject *newObj __attribute__((unused)),
                              const _WlanAdapterObject *currObj __attribute__((unused)),
                              const InstanceIdStack *iidStack __attribute__((unused)),
                              char **errorParam __attribute__((unused)),
                              CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlBaseCfgObject(_WlBaseCfgObject *newObj,
                           const _WlBaseCfgObject *currObj,
                           const InstanceIdStack *iidStack,
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    char buffer[BUFLEN_256];
    _LanWlanObject *lanObj;
    CmsRet ret;

    if (newObj != NULL)
    {
        ret = cmsObj_get(MDMOID_LAN_WLAN, iidStack, OGF_NO_VALUE_UPDATE, (void **)&lanObj);
        if (ret != CMSRET_SUCCESS)
        {
            return ret;
        }

        if ((currObj == NULL) || (currObj->wlEnbl != newObj->wlEnbl))
        {
            lanObj->enable = (newObj->wlEnbl == 0) ? FALSE : TRUE;
            //add radioEnabled for pass cdrouter test
            lanObj->radioEnabled = (newObj->wlEnbl == 0) ? FALSE : TRUE;
        }

        if ((currObj == NULL) || (currObj->wlChannel != newObj->wlChannel))
        {
            lanObj->channel = newObj->wlChannel;
        }

        if ((currObj == NULL) || (currObj->wlRate != newObj->wlRate))
        {
            rutWlan_getMaxBitRate((char *)&buffer, sizeof(buffer), newObj->wlRate);
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->maxBitRate, buffer, mdmLibCtx.allocFlags);
        }

        if ((currObj == NULL) ||
            (currObj->wlBand != newObj->wlBand) ||
            (currObj->wlgMode != newObj->wlgMode) ||
            (0 != cmsUtl_strcmp(currObj->wlBasicRate, newObj->wlBasicRate)))
        {
            const char *standard, *basicDataTransmitRates;
            standard = rutWlan_getStandard(newObj->wlBand, newObj->wlgMode);
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->standard, standard, mdmLibCtx.allocFlags);
            basicDataTransmitRates = rutWlan_getBasicDataTransmitRates(newObj->wlBand,
                                                                       newObj->wlgMode,
                                                                       newObj->wlBasicRate);
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->basicDataTransmitRates,
                                        basicDataTransmitRates,
                                        mdmLibCtx.allocFlags);
        }

        if ((currObj == NULL) ||
            (0 != cmsUtl_strcmp(currObj->wlCountry, newObj->wlCountry)))
        {
            char buffer[3] = {0};
            strncpy((char *)&buffer, newObj->wlCountry, sizeof(buffer) - 1);
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->regulatoryDomain, buffer, mdmLibCtx.allocFlags);
        }

        if ((currObj == NULL) ||
            (0 != cmsUtl_strcmp(currObj->wlMode, newObj->wlMode)))
        {
            const char *deviceOperationMode;
            deviceOperationMode = rutWlan_getDeviceOperationMode(newObj->wlMode);
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->deviceOperationMode,
                                        deviceOperationMode,
                                        mdmLibCtx.allocFlags);
        }

        if ((currObj == NULL) ||
            (currObj->wlTxPwrPcnt != newObj->wlTxPwrPcnt))
        {
            lanObj->X_BROADCOM_COM_TxPowerPercent = newObj->wlTxPwrPcnt;
        }

        ret = cmsObj_set(lanObj, iidStack);
        if (ret != CMSRET_SUCCESS)
        {
            return ret;
        }
        cmsObj_free((void **)&lanObj);
        rut2_sendWifiChange();
    }
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlStaticWdsCfgObject( _WlStaticWdsCfgObject *newObj __attribute__((unused)),
                                 const _WlStaticWdsCfgObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlWdsCfgObject( _WlWdsCfgObject *newObj __attribute__((unused)),
                           const _WlWdsCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlScanWdsCfgObject( _WlScanWdsCfgObject *newObj __attribute__((unused)),
                               const _WlScanWdsCfgObject *currObj __attribute__((unused)),
                               const InstanceIdStack *iidStack __attribute__((unused)),
                               char **errorParam __attribute__((unused)),
                               CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlMimoCfgObject( _WlMimoCfgObject *newObj __attribute__((unused)),
                            const _WlMimoCfgObject *currObj __attribute__((unused)),
                            const InstanceIdStack *iidStack __attribute__((unused)),
                            char **errorParam __attribute__((unused)),
                            CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlSesCfgObject( _WlSesCfgObject *newObj __attribute__((unused)),
                           const _WlSesCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlVirtIntfCfgObject( _WlVirtIntfCfgObject *newObj,
                                const _WlVirtIntfCfgObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
    _LanWlanObject *lanObj;
    UINT32 bssid_index;
    InstanceIdStack iidStackLan;
    const char *basicEncryptModes, *wpaEncryptModes;
    CmsRet ret = CMSRET_SUCCESS;

    bssid_index = PEEK_INSTANCE_ID(iidStack);
    /* For the sync up of mdm data tree, we only care about instance 1
     * because this is the MAIN BSSID object.*/
    if (bssid_index != 1)
    {
        return CMSRET_SUCCESS;
    }

    if (newObj != NULL)
    {
        iidStackLan = *iidStack;
        POP_INSTANCE_ID(&iidStackLan);
        ret = cmsObj_get(MDMOID_LAN_WLAN, &iidStackLan, OGF_NO_VALUE_UPDATE, (void **)&lanObj);
        if (ret != CMSRET_SUCCESS)
        {
            return ret;
        }

        if ((currObj == NULL) || (0 != cmsUtl_strcmp(currObj->wlSsid, newObj->wlSsid)))
        {
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->SSID, newObj->wlSsid, mdmLibCtx.allocFlags);
        }

        if ((currObj == NULL) || (0 != cmsUtl_strcmp(currObj->wlBssMacAddr, newObj->wlBssMacAddr)))
        {
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->BSSID, newObj->wlBssMacAddr, mdmLibCtx.allocFlags);
        }

        if ((currObj == NULL) || (currObj->wlHide != newObj->wlHide))
        {
            lanObj->SSIDAdvertisementEnabled = (newObj->wlHide == 0) ? TRUE : FALSE;
        }

        if ((currObj == NULL) || (0 != cmsUtl_strcmp(currObj->wlAuthMode, newObj->wlAuthMode)))
        {
            const char *beaconType, *authMode;

            beaconType = rutWlan_getBeaconType(newObj->wlAuthMode);
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->beaconType, beaconType,
                                        mdmLibCtx.allocFlags);

            /* Update basicAuthenticationMode. */
            if (0 == cmsUtl_strcmp(newObj->wlAuthMode, "radius"))
            {
                authMode = "EAPAuthentication";
            }
            else if (0 == cmsUtl_strcmp(newObj->wlAuthMode, "shared"))
            {
                authMode = "SharedAuthentication";
            }
            else
            {
                authMode = "None";
            }
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->basicAuthenticationMode,
                                        authMode,
                                        mdmLibCtx.allocFlags);

            /* Update IEEE11iAuthenticationMode and WPAAuthenticationMode. */
            if ((0 == cmsUtl_strcmp(newObj->wlAuthMode, "wpa2")) ||
                (0 == cmsUtl_strcmp(newObj->wlAuthMode, "wpa")))
            {
                authMode = "EAPAuthentication";
            }
            else
            {
                authMode = "PSKAuthentication";
            }
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->IEEE11iAuthenticationMode,
                                        authMode,
                                        mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->WPAAuthenticationMode,
                                        authMode,
                                        mdmLibCtx.allocFlags);
        }

        if ((currObj == NULL) ||
            (0 != cmsUtl_strcmp(currObj->wlFltMacMode, newObj->wlFltMacMode)))
        {
            if (0 == cmsUtl_strcmp(newObj->wlFltMacMode, WL_FLT_MAC_ALLOW))
            {
                lanObj->MACAddressControlEnabled = 1;
            }
            else
            {
                lanObj->MACAddressControlEnabled = 0;
            }
        }

        if ((currObj == NULL) ||
            (0 != cmsUtl_strcmp(currObj->wlWpaPsk, newObj->wlWpaPsk)))
        {
            _LanWlanPreSharedKeyObject *preSharedKeyObj;
            InstanceIdStack iidStackPreSharedKey;
            iidStackPreSharedKey = *iidStack;
            ret = cmsObj_get(MDMOID_LAN_WLAN_PRE_SHARED_KEY,
                             &iidStackPreSharedKey,
                             OGF_NO_VALUE_UPDATE,
                             (void **)&preSharedKeyObj);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("Failed to retrieve PreSharedKey");
            }
            else
            {
                CMSMEM_REPLACE_STRING_FLAGS(preSharedKeyObj->preSharedKey,
                                            newObj->wlWpaPsk,
                                            mdmLibCtx.allocFlags);
                //use obj->keyPassphrase in cdrouter test
                CMSMEM_REPLACE_STRING_FLAGS(preSharedKeyObj->keyPassphrase,
                                            newObj->wlWpaPsk,
                                            mdmLibCtx.allocFlags);
               
                cmsObj_set(preSharedKeyObj, &iidStackPreSharedKey);
                cmsObj_free((void **)&preSharedKeyObj);
            }
        }

        if ((currObj == NULL) ||
            (0 != cmsUtl_strcmp(currObj->wlIfcname, newObj->wlIfcname)))
        {
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->X_BROADCOM_COM_IfName,
                                        newObj->wlIfcname,
                                        mdmLibCtx.allocFlags);
        }
        lanObj->X_BROADCOM_COM_HideSSID = newObj->wlHide;
        lanObj->WEPKeyIndex = (newObj->wlKeyBit == WL_BIT_KEY_64) ?
                              newObj->wlKeyIndex64 : newObj->wlKeyIndex128;

        /* Update basicEncryptionModes. */
        if ((currObj == NULL) 
            || (0 != cmsUtl_strcmp(currObj->wlWep, newObj->wlWep)) 
            || (0 != cmsUtl_strcmp(currObj->wlWpa, newObj->wlWpa)))
        {
            wpaEncryptModes = NULL;
            if (0 == cmsUtl_strcmp(newObj->wlWep, "enabled"))
            {
                if (0 == cmsUtl_strcmp(newObj->wlWpa, "tkip"))
                {
                    wpaEncryptModes = "WEPandTKIPEncryption";
                }
                else if (0 == cmsUtl_strcmp(newObj->wlWpa, "aes"))
                {
                    wpaEncryptModes = "WEPandAESEncryption";
                }
                else if (0 == cmsUtl_strcmp(newObj->wlWpa, "tkip+aes"))
                {
                    wpaEncryptModes = "WEPandTKIPandAESEncryption";
                }
                else
                {
                    wpaEncryptModes = "WEPEncryption";
                }
                basicEncryptModes = "WEPEncryption";
            }
            else
            {
                if (0 == cmsUtl_strcmp(newObj->wlWpa, "tkip"))
                {
                    wpaEncryptModes = "TKIPEncryption";
                }
                else if (0 == cmsUtl_strcmp(newObj->wlWpa, "aes"))
                {
                    wpaEncryptModes = "AESEncryption";
                }
                else if (0 == cmsUtl_strcmp(newObj->wlWpa, "tkip+aes"))
                {
                    wpaEncryptModes = "TKIPandAESEncryption";
                }
                basicEncryptModes = "None";
            }
            CMSMEM_REPLACE_STRING_FLAGS(lanObj->basicEncryptionModes,
                                        basicEncryptModes,
                                        mdmLibCtx.allocFlags);
            if (wpaEncryptModes != NULL)
            {
                CMSMEM_REPLACE_STRING_FLAGS(lanObj->WPAEncryptionModes,
                                            wpaEncryptModes,
                                            mdmLibCtx.allocFlags);
                CMSMEM_REPLACE_STRING_FLAGS(lanObj->IEEE11iEncryptionModes,
                                            wpaEncryptModes,
                                            mdmLibCtx.allocFlags);
            }
        }

        ret = cmsObj_set(lanObj, &iidStackLan);
        cmsObj_free((void **)&lanObj);
        rut2_sendWifiChange();
    }
    return ret;
}

CmsRet rcl_wlMacFltObject( _WlMacFltObject *newObj __attribute__((unused)),
                           const _WlMacFltObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlKey64CfgObject( _WlKey64CfgObject *newObj,
                             const _WlKey64CfgObject *currObj,
                             const InstanceIdStack *iidStack,
                             char **errorParam __attribute__((unused)),
                             CmsRet *errorCode __attribute__((unused)))
{
    _LanWlanWepKeyObject *WEPKeyObj;
    _WlVirtIntfCfgObject *virtObj;
    InstanceIdStack iidStackWEPKey;
    InstanceIdStack iidStackVirt;
    UINT32 keyIndex;
    CmsRet ret = CMSRET_SUCCESS;

    if (newObj != NULL)
    {
        keyIndex = PEEK_INSTANCE_ID(iidStack);

        iidStackWEPKey = *iidStack;
        POP_INSTANCE_ID(&iidStackWEPKey);
        POP_INSTANCE_ID(&iidStackWEPKey);
        PUSH_INSTANCE_ID(&iidStackWEPKey, keyIndex);
        ret = cmsObj_get(MDMOID_LAN_WLAN_WEP_KEY, &iidStackWEPKey,
                         OGF_NO_VALUE_UPDATE, (void **)&WEPKeyObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to retrieve WEP Key for keyIndex %d", keyIndex);
            return ret;
        }

        iidStackVirt = *iidStack;
        POP_INSTANCE_ID(&iidStackVirt);
        ret = cmsObj_get(MDMOID_WL_VIRT_INTF_CFG, &iidStackVirt,
                         OGF_NO_VALUE_UPDATE, (void **)&virtObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to get VIRT_INTF_CFG object instance 1");
            cmsObj_free((void **)&WEPKeyObj);
            return ret;
        }

        if ((currObj == NULL) ||
            (0 != cmsUtl_strcmp(currObj->wlKey64, newObj->wlKey64)))
        {
            if (virtObj->wlKeyBit == WL_BIT_KEY_64)
            {
                CMSMEM_REPLACE_STRING_FLAGS(WEPKeyObj->WEPKey,
                                            newObj->wlKey64,
                                            mdmLibCtx.allocFlags);
            }
        }

        ret = cmsObj_set(WEPKeyObj, &iidStackWEPKey);

        cmsObj_free((void **)&virtObj);
        cmsObj_free((void **)&WEPKeyObj);
        rut2_sendWifiChange();
    }
    return ret;
}

CmsRet rcl_wlKey128CfgObject( _WlKey128CfgObject *newObj,
                              const _WlKey128CfgObject *currObj,
                              const InstanceIdStack *iidStack,
                              char **errorParam __attribute__((unused)),
                              CmsRet *errorCode __attribute__((unused)))
{
    _LanWlanWepKeyObject *WEPKeyObj;
    _WlVirtIntfCfgObject *virtObj;
    InstanceIdStack iidStackWEPKey;
    InstanceIdStack iidStackVirt;
    UINT32 keyIndex;
    CmsRet ret = CMSRET_SUCCESS;

    if (newObj != NULL)
    {
        keyIndex = PEEK_INSTANCE_ID(iidStack);

        iidStackWEPKey = *iidStack;
        POP_INSTANCE_ID(&iidStackWEPKey);
        POP_INSTANCE_ID(&iidStackWEPKey);
        PUSH_INSTANCE_ID(&iidStackWEPKey, keyIndex);
        ret = cmsObj_get(MDMOID_LAN_WLAN_WEP_KEY, &iidStackWEPKey,
                         OGF_NO_VALUE_UPDATE, (void **)&WEPKeyObj);
        if (ret != CMSRET_SUCCESS)
        {
            return ret;
        }

        iidStackVirt = *iidStack;
        POP_INSTANCE_ID(&iidStackVirt);
        ret = cmsObj_get(MDMOID_WL_VIRT_INTF_CFG, &iidStackVirt,
                         OGF_NO_VALUE_UPDATE, (void **)&virtObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to get VIRT_INTF_CFG object instance 1");
            cmsObj_free((void **)&WEPKeyObj);
            return ret;
        }

        if ((currObj == NULL) ||
            (0 != cmsUtl_strcmp(currObj->wlKey128, newObj->wlKey128)))
        {
            if (virtObj->wlKeyBit == WL_BIT_KEY_128)
            {
                CMSMEM_REPLACE_STRING_FLAGS(WEPKeyObj->WEPKey,
                                            newObj->wlKey128,
                                            mdmLibCtx.allocFlags);
            }
        }

        ret = cmsObj_set(WEPKeyObj, &iidStackWEPKey);

        cmsObj_free((void **)&virtObj);
        cmsObj_free((void **)&WEPKeyObj);
        rut2_sendWifiChange();
    }
    return ret;
}

CmsRet rcl_wlWpsCfgObject( _WlWpsCfgObject *newObj __attribute__((unused)),
                           const _WlWpsCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_lanWlanVirtMbssObject( _LanWlanVirtMbssObject *newObj __attribute__((unused)),
                                  const _LanWlanVirtMbssObject *currObj __attribute__((unused)),
                                  const InstanceIdStack *iidStack __attribute__((unused)),
                                  char **errorParam __attribute__((unused)),
                                  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

/*
 * these next 3 certificate related functions could be used for general
 * certificate purposes on the system, but for now, only used by WLAN code.
 */
CmsRet rcl_wapiCertificateObject( _WapiCertificateObject *newObj __attribute__((unused)),
                                  const _WapiCertificateObject *currObj __attribute__((unused)),
                                  const InstanceIdStack *iidStack __attribute__((unused)),
                                  char **errorParam __attribute__((unused)),
                                  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wapiAsCertificateObject( _WapiAsCertificateObject *newObj __attribute__((unused)),
                                    const _WapiAsCertificateObject *currObj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                    char **errorParam __attribute__((unused)),
                                    CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wapiIssuedCertificateObject( _WapiIssuedCertificateObject *newObj __attribute__((unused)),
                                        const _WapiIssuedCertificateObject *currObj __attribute__((unused)),
                                        const InstanceIdStack *iidStack __attribute__((unused)),
                                        char **errorParam __attribute__((unused)),
                                        CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlanNvramObject( _WlanNvramObject *newObj __attribute__((unused)),
                            const _WlanNvramObject *currObj __attribute__((unused)),
                            const InstanceIdStack *iidStack __attribute__((unused)),
                            char **errorParam __attribute__((unused)),
                            CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}
#endif  /* DMP_X_BROADCOM_COM_WIFILAN_1 */

#endif /* BRCM_WLAN */
