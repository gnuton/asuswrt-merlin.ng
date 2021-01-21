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

#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_WIFIRADIO_1

#include "mdm.h"
#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_wifi.h"
#include "rut2_unfwifi.h"
#include "cms_qdm.h"

CmsRet rcl_dev2WifiObject( _Dev2WifiObject *newObj __attribute__((unused)),
                           const _Dev2WifiObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiBsdCfgObject( _Dev2WifiBsdCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiBsdCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiWpsCfgObject( _Dev2WifiWpsCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiWpsCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiNasCfgObject( _Dev2WifiNasCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiNasCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiDebugMonitorCfgObject( _Dev2WifiDebugMonitorCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiDebugMonitorCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiCeventdCfgObject( _Dev2WifiCeventdCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiCeventdCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiHapdCfgObject( _Dev2WifiHapdCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiHapdCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiSsdCfgObject( _Dev2WifiSsdCfgObject *newObj __attribute__((unused)),
                           const _Dev2WifiSsdCfgObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiWbdCfgObject( _Dev2WifiWbdCfgObject *newObj  __attribute__((unused)),
                const _Dev2WifiWbdCfgObject *currObj  __attribute__((unused)),
                const InstanceIdStack *iidStack  __attribute__((unused)),
                char **errorParam  __attribute__((unused)),
                CmsRet *errorCode  __attribute__((unused)))
{
   rut2_sendWifiChange();
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiWbdCfgMbssObject( _Dev2WifiWbdCfgMbssObject *newObj  __attribute__((unused)),
                const _Dev2WifiWbdCfgMbssObject *currObj  __attribute__((unused)),
                const InstanceIdStack *iidStack  __attribute__((unused)),
                char **errorParam  __attribute__((unused)),
                CmsRet *errorCode  __attribute__((unused)))
{
   rut2_sendWifiChange();
   return CMSRET_SUCCESS;
}               

CmsRet rcl_dev2WifiRadioObject( _Dev2WifiRadioObject *newObj,
                                const _Dev2WifiRadioObject *currObj,
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
    if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
    {
        CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_UP, mdmLibCtx.allocFlags);
    }

    if (DISABLE_EXISTING(newObj, currObj))
    {
        CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
    }

    if (newObj && currObj)
    {
        if (newObj->autoChannelEnable != currObj->autoChannelEnable)
        {
            if (newObj->autoChannelEnable == TRUE)
                newObj->channel = 0;
            else if (newObj->channel == 0)
            {
                if (strcmp(newObj->operatingFrequencyBand, MDMVS_2_4GHZ) == 0)
                    newObj->channel = 1;
                else
                    newObj->channel = 36;
            }
        }
        else if (newObj->channel != currObj->channel && newObj->channel)
        {
            newObj->autoChannelEnable=0;
        }
    }

    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiRadioStatsObject( _Dev2WifiRadioStatsObject *newObj __attribute__((unused)),
                                     const _Dev2WifiRadioStatsObject *currObj __attribute__((unused)),
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2WifiNeighboringwifidiagnosticObject( _Dev2WifiNeighboringwifidiagnosticObject *newObj,
        const _Dev2WifiNeighboringwifidiagnosticObject *currObj,
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS;
   char cmdLine[BUFLEN_16];
   int pid;

   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    */
   if (ADD_NEW(newObj, currObj))
   {
      return ret;
   }

   if (cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0)
   {
      if (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED))
      {
         cmsLog_notice("Sending message to SMD to start doing wlDiag");
   
         sprintf(cmdLine, "%s", mdmLibCtx.eid == EID_TR69C ? "-n" : "");
   
         cmsLog_debug("wlDiag command string=%s", cmdLine);
      
         pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_WLDIAG, cmdLine, strlen(cmdLine)+1);
         
         if (pid == CMS_INVALID_PID)
         {
            cmsLog_error("failed to start wlDiag test.");
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            cmsLog_debug("Start wlDiag msg sent, pid=%d", pid);
         }   
      }   
   } /* requested */
   else if (cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_CANCELED) == 0)
   {
      if (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_CANCELED))
      {
         cmsLog_notice("Sending message to SMD to stop wlDiag");
   	  
         ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_WLDIAG, NULL, 0);
         
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to stop wlDiag test.");
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            cmsLog_debug("Stop wlDiag msg sent");
         }   
      }   
   }
   
   return (ret);
}


CmsRet rcl_dev2WifiNeighboringwifidiagnosticResultObject( _Dev2WifiNeighboringwifidiagnosticResultObject *newObj __attribute__((unused)),
        const _Dev2WifiNeighboringwifidiagnosticResultObject *currObj __attribute__((unused)),
        const InstanceIdStack *iidStack __attribute__((unused)),
        char **errorParam __attribute__((unused)),
        CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiRadioAcsdCfgObject( _Dev2WifiRadioAcsdCfgObject *newObj __attribute__((unused)),
                                     const _Dev2WifiRadioAcsdCfgObject *currObj __attribute__((unused)),
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiSsidObject( _Dev2WifiSsidObject *newObj,
                               const _Dev2WifiSsidObject *currObj,
                               const InstanceIdStack *iidStack,
                               char **errorParam __attribute__((unused)),
                               CmsRet *errorCode __attribute__((unused)))
{
    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumWifiSsid(iidStack, 1);
        if (mdmLibCtx.eid == EID_SSK)
            return CMSRET_SUCCESS;
        else
            return CMSRET_REQUEST_DENIED;
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumWifiSsid(iidStack, -1);
        return CMSRET_SUCCESS;
    }

    if (newObj != NULL)
    {
        // force BSSID the same as MACAddress
        if (cmsUtl_strcmp(newObj->BSSID, newObj->MACAddress) != 0)
        {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->BSSID, newObj->MACAddress, mdmLibCtx.allocFlags);
        }
    }

    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2WifiSsidStatsObject( _Dev2WifiSsidStatsObject *newObj __attribute__((unused)),
                                    const _Dev2WifiSsidStatsObject *currObj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                    char **errorParam __attribute__((unused)),
                                    CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiSsidBsdCfgObject( _Dev2WifiSsidBsdCfgObject *newObj __attribute__((unused)),
                                    const _Dev2WifiSsidBsdCfgObject *currObj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                    char **errorParam __attribute__((unused)),
                                    CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiSsidHspotCfgObject( _Dev2WifiSsidHspotCfgObject *newObj __attribute__((unused)),
                                     const _Dev2WifiSsidHspotCfgObject *currObj __attribute__((unused)),
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiSsidSsdCfgObject( _Dev2WifiSsidSsdCfgObject *newObj __attribute__((unused)),
                                     const _Dev2WifiSsidSsdCfgObject *currObj __attribute__((unused)),
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_WIFIRADIO_1 */

#ifdef DMP_DEVICE2_WIFIACCESSPOINT_1
CmsRet rcl_dev2WifiAccessPointObject( _Dev2WifiAccessPointObject *newObj __attribute__((unused)),
                                      const _Dev2WifiAccessPointObject *currObj __attribute__((unused)),
                                      const InstanceIdStack *iidStack __attribute__((unused)),
                                      char **errorParam __attribute__((unused)),
                                      CmsRet *errorCode __attribute__((unused)))
{
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

    
    rut2_sendWifiChange();
    return CMSRET_SUCCESS;
}


static CmsRet updateNvramWEPKeyChanged( _Dev2WifiAccessPointSecurityObject *newObj,
                                        const _Dev2WifiAccessPointSecurityObject *currObj)
{
    CmsRet ret = CMSRET_SUCCESS;
    int isChanged = FALSE;
    char *targetKey = NULL;

    if (newObj == NULL || currObj == NULL)
        return CMSRET_SUCCESS;

    // check key index change
    if (newObj->X_BROADCOM_COM_WlKeyIndex != currObj->X_BROADCOM_COM_WlKeyIndex)
    {
        isChanged = TRUE;
    }

    // check referenced key change
    switch(newObj->X_BROADCOM_COM_WlKeyIndex)
    {
        case 1:
            targetKey = newObj->X_BROADCOM_COM_WlKey1;
            if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WlKey1, currObj->X_BROADCOM_COM_WlKey1) != 0)
                isChanged = TRUE;
            break;
        case 2:
            targetKey = newObj->X_BROADCOM_COM_WlKey2;
            if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WlKey1, currObj->X_BROADCOM_COM_WlKey2) != 0)
                isChanged = TRUE;
            break;
        case 3:
            targetKey = newObj->X_BROADCOM_COM_WlKey3;
            if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WlKey1, currObj->X_BROADCOM_COM_WlKey3) != 0)
                isChanged = TRUE;
            break;
        case 4:
            targetKey = newObj->X_BROADCOM_COM_WlKey4;
            if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WlKey1, currObj->X_BROADCOM_COM_WlKey4) != 0)
                isChanged = TRUE;
            break;
        default:
            newObj->X_BROADCOM_COM_WlKeyIndex = 1;
            targetKey = newObj->X_BROADCOM_COM_WlKey1;
            if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WlKey1, currObj->X_BROADCOM_COM_WlKey1) != 0)
                isChanged = TRUE;
            break;
    }

    /** WEPKey only accept hexstring. 
     * translate wlkey[n] here if it is a ascii string.
     */
    if (isChanged && targetKey != NULL)
    {
        int i, keyLength = strlen(targetKey);
        char *hexStr = NULL;

        if (((keyLength % 2) != 0) && (keyLength == 5 || keyLength == 13)) // 5 or 13 ascii
        {
            ret = cmsUtl_binaryBufToHexString(targetKey, keyLength, &hexStr);
        }
        else if (((keyLength % 2) == 0) && (keyLength == 10 || keyLength == 26)) // 10 or 26 hex string
        {
            for (i = 0 ; i < keyLength ; i++)
                if(!isxdigit(targetKey[i]))
                    ret = CMSRET_INVALID_PARAM_VALUE;

            if (ret == CMSRET_SUCCESS)
                hexStr = cmsMem_strdup(targetKey);
        }
        else if(keyLength == 0)
        { // for unset key
            ret = CMSRET_SUCCESS;
        }
        else
        {
            ret = CMSRET_INVALID_PARAM_VALUE;
        }

        if (ret == CMSRET_SUCCESS && hexStr != NULL)
        {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->WEPKey, hexStr, mdmLibCtx.allocFlags);
            cmsMem_free(hexStr);
        }
    }

    return ret;
}

CmsRet rcl_dev2WifiAccessPointSecurityObject( _Dev2WifiAccessPointSecurityObject *newObj,
                                              const _Dev2WifiAccessPointSecurityObject *currObj,
                                              const InstanceIdStack *iidStack __attribute__((unused)),
                                              char **errorParam __attribute__((unused)),
                                              CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    Dev2WifiAccessPointObject *wifiAccessPointObj;

    cmsLog_debug("===> Enter");
    if (ADD_NEW(newObj, currObj)) {
        return CMSRET_SUCCESS;
    }
    else if (DELETE_EXISTING(newObj, currObj)) {
        return CMSRET_SUCCESS;
    }

    if (newObj && currObj)
    {
        cmsLog_debug("....newObj->reset:%d\n",newObj->reset);
        InstanceIdStack ancestorIidStack = *iidStack;

        if (cmsObj_getAncestor(MDMOID_DEV2_WIFI_ACCESS_POINT,
                    MDMOID_DEV2_WIFI_ACCESS_POINT_SECURITY,
                    &ancestorIidStack,(void **)&wifiAccessPointObj) != CMSRET_SUCCESS)
        {
            return CMSRET_INVALID_PARAM_VALUE;
        }

        // implement reset 
        if (newObj->reset) 
        {
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
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->preSharedKey, defaultObj->preSharedKey, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->keyPassphrase, defaultObj->keyPassphrase, mdmLibCtx.allocFlags);

            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey1, defaultObj->X_BROADCOM_COM_WlKey1, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey2, defaultObj->X_BROADCOM_COM_WlKey2, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey3, defaultObj->X_BROADCOM_COM_WlKey3, mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_WlKey4, defaultObj->X_BROADCOM_COM_WlKey4, mdmLibCtx.allocFlags);
            newObj->X_BROADCOM_COM_WlKeyIndex = defaultObj->X_BROADCOM_COM_WlKeyIndex;

            cmsObj_free((void **)&defaultObj);
        } 
        else 
        {
            // check mode changed
            if (cmsUtl_strcmp(newObj->modeEnabled, currObj->modeEnabled) != 0)
            {
                _Dev2WifiRadioObject *radioObj = NULL;
                InstanceIdStack radio_iidStack = EMPTY_INSTANCE_ID_STACK;

                if((ret=rutWifi_get_AP_Radio_dev2(wifiAccessPointObj,(void **)&radioObj,&radio_iidStack))!=CMSRET_SUCCESS)
                {
                    cmsLog_debug("....failed to get radio:%d...\n",ret);
                    cmsObj_free((void **)&wifiAccessPointObj);
                    return CMSRET_INVALID_PARAM_VALUE;
                }

                // WEP64 or WEP128
                if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WEP_64) || !cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WEP_128))
                {
                    // turn off "n-mode"
                    if(cmsUtl_strcmp(radioObj->X_BROADCOM_COM_WlNmode,"off"))
                    {
                        cmsLog_debug("set wlNmod set off");
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(radioObj->X_BROADCOM_COM_WlNmode, "off", mdmLibCtx.allocFlags);
                        cmsObj_setFlags(radioObj,&radio_iidStack, OSF_NO_RCL_CALLBACK);
                    }

                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, MDMVS_OPEN, mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, MDMVS_OPEN, mdmLibCtx.allocFlags);
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, "enabled", mdmLibCtx.allocFlags);
                } 
                else
                {
                    // turn on "n-mode"
                    cmsLog_debug("set wlNmode to auto\n");
                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(radioObj->X_BROADCOM_COM_WlNmode, "auto", mdmLibCtx.allocFlags);
                    cmsObj_setFlags(radioObj,&radio_iidStack, OSF_NO_RCL_CALLBACK);

                    REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlWep, "disabled", mdmLibCtx.allocFlags);

                    if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_NONE))
                    {
                        /* if modeEnable is NONE */
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, MDMVS_OPEN, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, MDMVS_OPEN, mdmLibCtx.allocFlags);
                    }
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA_PERSONAL))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, MDMVS_PSK, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, MDMVS_OPEN, mdmLibCtx.allocFlags);
                    } 
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA2_PERSONAL))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, MDMVS_PSK2, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, MDMVS_OPEN, mdmLibCtx.allocFlags);
                    } 
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA_WPA2_PERSONAL))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, MDMVS_PSK_PSK2, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, MDMVS_OPEN, mdmLibCtx.allocFlags);
                    } 
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA_ENTERPRISE))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, MDMVS_WPA, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "radius", mdmLibCtx.allocFlags);
                    } 
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA2_ENTERPRISE))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, MDMVS_WPA2, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "radius", mdmLibCtx.allocFlags);
                    } 
                    else if (!cmsUtl_strcmp(newObj->modeEnabled, MDMVS_WPA_WPA2_ENTERPRISE))
                    {
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthAkm, MDMVS_WPA_WPA2, mdmLibCtx.allocFlags);
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->wlAuthMode, "radius", mdmLibCtx.allocFlags);
                    }
                }

                cmsObj_free((void **)&radioObj);
            }

            // check WEPKey change
            if (cmsUtl_strcmp(newObj->WEPKey, currObj->WEPKey))
            {
                cmsLog_debug("newObj->WEPKey:%s and currObj->WEPKey:%s",newObj->WEPKey,currObj->WEPKey);

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
                } /* switch(wlkeyindex) */
            } /* WEPKey */

            // check nvram wep key or index change and update WEPKey
            ret = updateNvramWEPKeyChanged(newObj, currObj);
        } /* no reset */

        cmsObj_free((void **)&wifiAccessPointObj);
    }

    if (ret == CMSRET_SUCCESS)
        rut2_sendWifiChange();

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

    rut2_sendWifiChange();
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

#ifdef DMP_DEVICE2_WIFIENDPOINT_1

CmsRet rcl_dev2WifiEndPointObject( _Dev2WifiEndPointObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   /* example for getting radio index
   UINT32 radioIndex;
   radioIndex = qdmWifi_getRadioIndexBySsidFullPathLocked_dev2(newObj->SSIDReference);
   */

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointStatsObject( _Dev2WifiEndPointStatsObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointStatsObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointSecurityObject( _Dev2WifiEndPointSecurityObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointSecurityObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointProfileObject( _Dev2WifiEndPointProfileObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointProfileObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointProfileSecurityObject( _Dev2WifiEndPointProfileSecurityObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointProfileSecurityObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointWpsObject( _Dev2WifiEndPointWpsObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointWpsObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointAcObject( _Dev2WifiEndPointAcObject *newObj,
                const _Dev2WifiEndPointAcObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointAcStatsObject( _Dev2WifiEndPointAcStatsObject *newObj,
                const _Dev2WifiEndPointAcStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_WIFIENDPOINT_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */
