/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
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

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "mdm.h"
#include "rut_util.h"
#include "rut_pmap.h"
#include "rut_lan.h"
#include "rut_unfwlan.h"
#include "wifi_constants.h"

void rutWlan_modifyVirtIntfFilters(const InstanceIdStack *iidStack __attribute__((unused)),
                                   UBOOL8 enable __attribute__((unused)))
{
#ifdef DMP_BRIDGING_1

    WlVirtIntfCfgObject *virtIntfObj=NULL;
    InstanceIdStack virtIidStack=EMPTY_INSTANCE_ID_STACK;
    CmsRet ret;

    cmsLog_debug("master wlEnbl=%d", enable);

    while ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_WL_VIRT_INTF_CFG, iidStack, &virtIidStack, OGF_NO_VALUE_UPDATE, (void **) &virtIntfObj)) == CMSRET_SUCCESS)
    {
        if (enable && virtIntfObj->wlEnblSsid)
        {
            cmsLog_debug("enabling virtIntf %s", virtIntfObj->wlIfcname);
            rutWlan_enableVirtIntfFilter(virtIntfObj->wlIfcname, atoi(&(virtIntfObj->wlBrName[2])));
        }
        else if (!enable)
        {
            cmsLog_debug("disabling virtIntf %s", virtIntfObj->wlIfcname);
            rutWlan_disableVirtIntfFilter(virtIntfObj->wlIfcname);

            if (strcmp(virtIntfObj->wlBrName, "br0"))
            {
                /*
                 * wlBrName is an internal book-keeping parameter used by wlmngr.
                 * set it to br0 to be consistent with the filterBridgeRefence setting
                 * of -1.  This is also done at the bottom of rcl_wlVirtIntfCfgObject.
                 */
                CMSMEM_REPLACE_STRING_FLAGS(virtIntfObj->wlBrName, "br0", mdmLibCtx.allocFlags);
                cmsObj_set(virtIntfObj, &virtIidStack);
            }
        }

        cmsObj_free((void **) &virtIntfObj);
    }

#endif
}

#ifdef DMP_BRIDGING_1
void rutWlan_enableVirtIntfFilter(const char *wlIfcname, UINT32 bridgeKey)
{
    L2BridgingFilterObject *filterObj=NULL;
    InstanceIdStack filterIidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret;

    cmsLog_debug("wlIfcname=%s bridgeKey=%d", wlIfcname, bridgeKey);

    if ((ret = rutPMap_ifNameToFilterObject(wlIfcname, &filterIidStack, &filterObj)) != CMSRET_SUCCESS)
    {
        cmsLog_debug("could not get filterObj for %s", wlIfcname);
    }
    else
    {
        /*
         * The bridgeKey is just the number in the brx linux interface name.
         * So br0 has bridgeKey of 0,
         * br1 has bridgeKey of 1, etc.
         */
        cmsLog_debug("got filter at %p iidStack=%s", filterObj, cmsMdm_dumpIidStack(&filterIidStack));

        if (filterObj->filterBridgeReference != (int) bridgeKey)
        {
            filterObj->filterBridgeReference = bridgeKey;

            if ((ret = cmsObj_set(filterObj, &filterIidStack)) != CMSRET_SUCCESS)
            {
                cmsLog_error("could not set FilterBridgeReference to %d, ret=%d", filterObj->filterBridgeReference, ret);
            }

            cmsObj_free((void **) &filterObj);
        }
    }

    return;
}


void rutWlan_disableVirtIntfFilter(const char *wlIfcname)
{
    L2BridgingFilterObject *filterObj=NULL;
    InstanceIdStack filterIidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret;

    cmsLog_debug("wlIfcname=%s", wlIfcname);

    /*
     * by setting the FilterBridgeRef to -1, the rut_pmap will disassociate
     * this interface from its bridge.
     */
    if ((ret = rutPMap_ifNameToFilterObject(wlIfcname, &filterIidStack, &filterObj)) != CMSRET_SUCCESS)
    {
        cmsLog_debug("could not get filterObj for %s", wlIfcname);
    }
    else
    {
        cmsLog_debug("filterBridgeRef=%d", filterObj->filterBridgeReference);

        if (filterObj->filterBridgeReference != -1)
        {
            filterObj->filterBridgeReference = -1;
            if ((ret = cmsObj_set(filterObj, &filterIidStack)) != CMSRET_SUCCESS)
            {
                cmsLog_error("could not set FilterBridgeReference to -1, ret=%d", ret);
            }
        }

        cmsObj_free((void **) &filterObj);
    }

    return;
}

char *rutWlan_getPhyType(int radioIndex)
{
    char *phyType = NULL;
    char buf[BUFLEN_256], fileName[BUFLEN_128];
    FILE *fp = NULL;
    int ret;

    snprintf(fileName, sizeof(fileName), "/var/wl%d", radioIndex);
    snprintf(buf, sizeof(buf), "wlctl -i wl%d phytype > %s", radioIndex, fileName);
    ret = system(buf);
    if (ret != 0)
    {
        return NULL;
    }

    // parse the phytype
    fp = fopen(fileName, "r");
    if (fp == NULL)
    {
        return NULL;
    }

    if (fgets(buf, sizeof(buf), fp) != NULL)
    {
        ret = atoi(buf);
        switch (ret)
        {
            case WL_PHYTYPE_A:
                phyType = "a";
                break;

            case WL_PHYTYPE_B:
                phyType = "b";
                break;

            case WL_PHYTYPE_G:
            case WL_PHYTYPE_LP:
                //it does not make difference for webui at this moment
                phyType = "g";
                break;

            case WL_PHYTYPE_N:
            case WL_PHYTYPE_HT:
            case WL_PHYTYPE_LCN:
                phyType = "n";
                break;

            case WL_PHYTYPE_AC:
                phyType = "v";
                break;

            default:
                phyType = "b";
        }
    }
    fclose(fp);
    return phyType;
}

UBOOL8 rutWlan_getRadioEnabled(int radioIndex)
{
    char buf[BUFLEN_128] = {0};
    FILE *fp = NULL;
    UBOOL8 value = TRUE;

    snprintf(buf, sizeof(buf), "wlctl -i wl%d radio", radioIndex);
    fp = popen(buf, "r");
    if (fp != NULL)
    {
        fgets(buf, sizeof(buf), fp);
        if (strstr(buf, "1") != NULL)
        {
            value = FALSE;
        }
        pclose(fp);
    }
    return value;
}

void rutWlan_getPossibleChannels(char *value, size_t size, int radioIndex)
{
    char buf[BUFLEN_1024] = {0};
    FILE *fp = NULL;
    char *pBuf, *pDest;
    char *saveptr = NULL;
    int remainLen;

    snprintf(buf, sizeof(buf), "wlctl -i wl%d channels", radioIndex);
    fp = popen(buf, "r");
    if (fp != NULL)
    {
        fgets(buf, sizeof(buf), fp);
        memset(value, 0x00, size);
        remainLen = size;
        pDest = value;
        for (pBuf = strtok_r(buf, " \n", &saveptr); pBuf != NULL; pBuf = strtok_r(NULL, " \n", &saveptr))
        {
            if (remainLen < size)
            {
                strncat(pDest, ",", remainLen - 1);
                remainLen -= 1;
                if (remainLen <= 1)
                {
                    break;
                }
                pDest += 1;
            }

            strncpy(pDest, pBuf, remainLen - 1);
            remainLen -= strlen(pBuf);
            if (remainLen <= 1)
            {
                break;
            }
            pDest += strlen(pBuf);
        }
        pclose(fp);
    }
}

const char *rutWlan_getStandard(int wlBand, int wlgMode)
{
    switch (wlBand)
    {
        case BAND_A:
            return "a";
        case BAND_B:
            if (wlgMode == 0)
            {
                return "b";
            }
            else
            {
                return "g";
            }
        default:
            break;
    }
    return "";
}

void rutWlan_getWlBandGmode(const char *standard, int *wlBand, int *wlgMode)
{
    if (0 == cmsUtl_strcmp(standard, "a"))
    {
        *wlBand = BAND_A;
    }
    else if (0 == cmsUtl_strcmp(standard, "g"))
    {
        *wlBand = BAND_B;
        *wlgMode = 1;
    }
    else
    {
        *wlBand = BAND_B;
        *wlgMode = 0;
    }
}

long rutWlan_getWlRate(char *maxBitRate)
{
    int mbps;
    long wlRate;

    if (maxBitRate == NULL)
    {
        return 0;
    }

    if (0 == cmsUtl_strcmp(maxBitRate, "Auto"))
    {
        wlRate = 0;
    }
    else
    {
        mbps = atoi(maxBitRate);
        wlRate = mbps * 1000000;
    }
    return wlRate;
}

void rutWlan_getMaxBitRate(char *maxBitRate, size_t size, long wlRate)
{
    int mbps;

    if (wlRate == 0)
    {
        strncpy(maxBitRate, "Auto", size - 1);
    }
    else
    {
        mbps = wlRate / 1000000;
        snprintf(maxBitRate, size, "%d", mbps);
    }
}

const char *rutWlan_getBeaconType(const char *wlAuthMode)
{
    if (0 == cmsUtl_strcmp(wlAuthMode, "open") ||
        0 == cmsUtl_strcmp(wlAuthMode, "shared") ||
        0 == cmsUtl_strcmp(wlAuthMode, "radius"))
    {
        return "Basic";
    }

    if (0 == cmsUtl_strcmp(wlAuthMode, "wpa") ||
        0 == cmsUtl_strcmp(wlAuthMode, "psk"))
    {
        return "WPA";
    }

    /*use strncmp here since set akm to psk2 from WEBGUI will make wlAuthMode="psk2 "*/
    if (0 == cmsUtl_strcmp(wlAuthMode, "wpa2") ||
        0 == cmsUtl_strncmp(wlAuthMode, "psk2", cmsUtl_strlen("psk2")))
    {
        return "11i";
    }

    if (0 == cmsUtl_strcmp(wlAuthMode, "wpa wpa2") ||
        0 == cmsUtl_strcmp(wlAuthMode, "psk psk2"))
    {
        return "WPAand11i";
    }
    return "None";
}

const char *rutWlan_getWlAuthMode(const char *beaconType, const char *basicAuthMode,
                                  const char *wpaAuthMode, const char *IEEE11iAuthMode,
                                  UBOOL8 wepEnabled)
{
    UBOOL8 psk, psk2, wpa, wpa2;

    if (0 == cmsUtl_strcmp(beaconType, "Basic"))
    {
        if (0 == cmsUtl_strcmp(basicAuthMode, "EAPAuthentication"))
        {
            return "radius";
        }
        else if (0 == cmsUtl_strcmp(basicAuthMode, "SharedAuthentication"))
        {
            if (wepEnabled)
            {
                return "shared";
            }
        }
    }
    else if (0 == cmsUtl_strcmp(beaconType, "WPA"))
    {
        if(0 == cmsUtl_strcmp(wpaAuthMode, "PSKAuthentication"))
        {
            return "psk";
        }
        else if (0 == cmsUtl_strcmp(wpaAuthMode, "EAPAuthentication"))
        {
            return "wpa";
        }
    }
    else if (0 == cmsUtl_strcmp(beaconType, "11i"))
    {
        if (0 == cmsUtl_strcmp(IEEE11iAuthMode, "PSKAuthentication"))
        {
            return "psk2";
        }
        else if (0 == cmsUtl_strcmp(IEEE11iAuthMode, "EAPAuthentication"))
        {
            return "wpa2";
        }
    }
    else if (0 == cmsUtl_strcmp(beaconType, "WPAand11i"))
    {
        psk = psk2 = wpa = wpa2 = FALSE;

        if (0 == cmsUtl_strcmp(wpaAuthMode, "PSKAuthentication"))
        {
            psk = TRUE;
        }
        else if (0 == cmsUtl_strcmp(wpaAuthMode, "EAPAuthentication"))
        {
            wpa = TRUE;
        }

        if (0 == cmsUtl_strcmp(IEEE11iAuthMode, "PSKAuthentication"))
        {
            psk2 = TRUE;
        }
        else if (0 == cmsUtl_strcmp(IEEE11iAuthMode, "EAPAuthentication"))
        {
            wpa2 = TRUE;
        }

        if (wpa || wpa2)
        {
            if (wpa && wpa2)
            {
                return "wpa wpa2";
            }
            else
            {
                return wpa ? "wpa" : "wpa2";
            }
        }

        if (psk || psk2)
        {
            if (psk && psk2)
            {
                return "psk psk2";
            }
            else
            {
                return psk ? "psk" : "psk2";
            }
        }
    }
    return "open";
}

UBOOL8 rutWlan_getWepEnabled(const char *beaconType, const char *basicEncryptModes,
                             const char *wpaEncryptModes, const char *IEEE11iEncryptModes)
{
    UBOOL8 wepEnabled = FALSE;

    if (0 == cmsUtl_strcmp(beaconType, "Basic"))
    {
        if (0 == cmsUtl_strcmp(basicEncryptModes, "WEPEncryption"))
        {
            wepEnabled = TRUE;
        }
    }
    else if (0 == cmsUtl_strcmp(beaconType, "WPA"))
    {
        if ((0 == cmsUtl_strcmp(wpaEncryptModes, "WEPEncryption")) ||
            (0 == cmsUtl_strcmp(wpaEncryptModes, "WEPandTKIPEncryption")) ||
            (0 == cmsUtl_strcmp(wpaEncryptModes, "WEPandAESEncryption")) ||
            (0 == cmsUtl_strcmp(wpaEncryptModes, "WEPandTKIPandAESEncryption")))
        {
            wepEnabled = TRUE;
        }
    }
    else if (0 == cmsUtl_strcmp(beaconType, "11i"))
    {
        if ((0 == cmsUtl_strcmp(IEEE11iEncryptModes, "WEPEncryption")) ||
            (0 == cmsUtl_strcmp(IEEE11iEncryptModes, "WEPandTKIPEncryption")) ||
            (0 == cmsUtl_strcmp(IEEE11iEncryptModes, "WEPandAESEncryption")) ||
            (0 == cmsUtl_strcmp(IEEE11iEncryptModes, "WEPandTKIPandAESEncryption")))
        {
            wepEnabled = TRUE;
        }
    }
    else if (0 == cmsUtl_strcmp(beaconType, "WPAand11i"))
    {
        if ((0 == cmsUtl_strcmp(wpaEncryptModes, "WEPEncryption")) ||
            (0 == cmsUtl_strcmp(IEEE11iEncryptModes, "WEPEncryption")))
        {
            wepEnabled = TRUE;
        }
    }
    return wepEnabled;
}

const char *rutWlan_getCrypto(const char *beaconType,
                              const char *WPAEncryptModes,
                              const char *IEEE11iEncryptModes)
{
    const char *crypto = "tkip";
    const char *target = NULL;

    if (0 == cmsUtl_strcmp(beaconType, "WPA"))
    {
        target = WPAEncryptModes;
    }
    else if ((0 == cmsUtl_strcmp(beaconType, "11i")) ||
             (0 == cmsUtl_strcmp(beaconType, "WPAand11i")))
    {
        target = IEEE11iEncryptModes;
    }

    if (target != NULL)
    {
        if ((0 == cmsUtl_strcmp(target, "WEPandTKIPEncryption")) ||
            (0 == cmsUtl_strcmp(target, "TKIPEncryption")))
        {
            crypto = "tkip";
        }
        else if ((0 == cmsUtl_strcmp(target, "WEPandAESEncryption")) ||
                 (0 == cmsUtl_strcmp(target, "AESEncryption")))
        {
            crypto = "aes";
        }
        else if ((0 == cmsUtl_strcmp(target, "WEPandTKIPandAESEncryption")) ||
                 (0 == cmsUtl_strcmp(target, "TKIPandAESEncryption")))
        {
            crypto = "tkip+aes";
        }
    }
    return crypto;
}

const char *rutWlan_getBasicDataTransmitRates(int wlBand, int wlgMode, const char *wlBasicRate)
{
    const char *result = "";

    switch (wlBand)
    {
        case BAND_A:
            if (0 == cmsUtl_strcmp(wlBasicRate, WL_BASIC_RATE_DEFAULT))
            {
                result = "54";
            }
            if (0 == cmsUtl_strcmp(wlBasicRate, WL_BASIC_RATE_ALL))
            {
                result = "1,2,5.5,6,9,11,12,18,24,36,48,54";
            }
            if (0 == cmsUtl_strcmp(wlBasicRate, WL_BASIC_RATE_WIFI_2))
            {
                result = "6,12,14";
            }
            if (0 == cmsUtl_strcmp(wlBasicRate, WL_BASIC_RATE_1_2))
            {
                result = "6,12";
            }
            break;

        case BAND_B:
            if (wlgMode == 0)
            {
                if (0 == cmsUtl_strcmp(wlBasicRate, WL_BASIC_RATE_DEFAULT))
                {
                    result = "11";
                }
                if (0 == cmsUtl_strcmp(wlBasicRate, WL_BASIC_RATE_ALL))
                {
                    result = "1,2,5.5,6,9,11";
                }
                if (0 == cmsUtl_strcmp(wlBasicRate, WL_BASIC_RATE_WIFI_2))
                {
                    result = "1,2,5.5,6,11,12,24";
                }
                if (0 == cmsUtl_strcmp(wlBasicRate, WL_BASIC_RATE_1_2))
                {
                    result = "1,2";
                }
            }
            else
            {
                // .11g
                if (0 == cmsUtl_strcmp(wlBasicRate, WL_BASIC_RATE_DEFAULT))
                {
                    result = "54";
                }
                if (0 == cmsUtl_strcmp(wlBasicRate, WL_BASIC_RATE_ALL))
                {
                    result = "1,2,5.5,6,9,11,12,18,24,36,48,54";
                }
                if (0 == cmsUtl_strcmp(wlBasicRate, WL_BASIC_RATE_WIFI_2))
                {
                    result = "6,12,24";
                }
                if (0 == cmsUtl_strcmp(wlBasicRate, WL_BASIC_RATE_1_2))
                {
                    result = "6,12";
                }
            }
            break;

        default:
            break;
    }
    return result;
}

const char* rutWlan_getWlBasicRate(const char *basicDataTransmitRates, int wlBand, int wlgMode)
{
    const char *result = WL_BASIC_RATE_DEFAULT;

    if (wlBand == BAND_A)
    {
        if (0 == cmsUtl_strcmp(basicDataTransmitRates, "54"))
        {
            result = WL_BASIC_RATE_DEFAULT;
        }
        else if (0 == cmsUtl_strcmp(basicDataTransmitRates, "1,2,5.5,6,9,11,12,18,24,36,48,54"))
        {
            result = WL_BASIC_RATE_ALL;
        }
        else if (0 == cmsUtl_strcmp(basicDataTransmitRates, "6,12,24"))
        {
            result = WL_BASIC_RATE_WIFI_2;
        }
        else if (0 == cmsUtl_strcmp(basicDataTransmitRates, "6,12"))
        {
            result = WL_BASIC_RATE_1_2;
        }
    }
    else if (wlgMode == 0)
    {  //b mode rate????
        if (0 == cmsUtl_strcmp(basicDataTransmitRates, "11"))
        {
            result = WL_BASIC_RATE_DEFAULT;
        }
        else if (0 == cmsUtl_strcmp(basicDataTransmitRates, "1,2,5.5,6,9,11"))
        {
            result = WL_BASIC_RATE_ALL;
        }
        else if (0 == cmsUtl_strcmp(basicDataTransmitRates, "1,2,5.5,6,11,12,24"))
        {
            result = WL_BASIC_RATE_WIFI_2;
        }
        else if (0 == cmsUtl_strcmp(basicDataTransmitRates, "1,2"))
        {
            result = WL_BASIC_RATE_1_2;
        }
    }
    else
    { //G Mode
        if (0 == cmsUtl_strcmp(basicDataTransmitRates, "54"))
        {
            result = WL_BASIC_RATE_DEFAULT;
        }
        else if (0 == cmsUtl_strcmp(basicDataTransmitRates, "1,2,5.5,6,9,11,12,18,24,36,48,54"))
        {
            result = WL_BASIC_RATE_ALL;
        }
        else if (0 == cmsUtl_strcmp(basicDataTransmitRates, "6,12,24"))
        {
            result = WL_BASIC_RATE_WIFI_2;
        }
        else if (0 == cmsUtl_strcmp(basicDataTransmitRates, "6,12"))
        {
            result = WL_BASIC_RATE_1_2;
        }
    }
    return result;
}

const char *rutWlan_getDeviceOperationMode(const char *wlMode)
{
    const char *result = "InfrastructureAccessPoint";
    if (0 == cmsUtl_strcmp(wlMode, WL_OPMODE_AP))
    {
        result = "InfrastructureAccessPoint";
    }
    else if (0 == cmsUtl_strcmp(wlMode, WL_OPMODE_WDS))
    {
        result = "WirelessBridge";
    }
    return result;
}

const char *rutWlan_getWlMode(const char *deviceOperationMode)
{
    const char *result = WL_OPMODE_AP;
    if (0 == cmsUtl_strcmp(deviceOperationMode, "InfrastructureAccessPoint"))
    {
        result = WL_OPMODE_AP;
    }
    else if (0 == cmsUtl_strcmp(deviceOperationMode, "WirelessBridge"))
    {
        result = WL_OPMODE_WDS;
    }
    return result;
}

#endif /* DMP_BRIDGING_1 */
