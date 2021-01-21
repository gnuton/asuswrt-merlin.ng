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

#ifdef DMP_DEVICE2_WIFIRADIO_1

/*!\file stl2_wifi.c
* \brief This file contains TR181 and X_BROADCOM_COM Wifi objects which
 *        are used by both the AccessPoint and EndPoint.
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

#include "rut2_wifi.h"

#include "wlcsm_lib_api.h"

#include "wlsysutil.h"

CmsRet stl_dev2WifiObject( _Dev2WifiObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2WifiRadioObject( _Dev2WifiRadioObject *obj,
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
    int radioIndex = 0;
    int   i, size, band = 0;
    char varValue[BUFLEN_256] = {0};
    int   possibleChannels[BUFLEN_32] = {0};
    char  channelBuf[BUFLEN_1024] = {0};
    char  *p = channelBuf;
    CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

    /* Calculate and return the TR181 LastChange */
    IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);
    if (obj->enable == TRUE)
        CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
    else
        CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);

    if (mdmLibCtx.eid != EID_WLMNGR && mdmLibCtx.eid != EID_SSK )
    {
        sscanf(obj->name, "wl%d", &radioIndex);

        memset(varValue, 0, sizeof(varValue));
        if (wlcsm_mngr_dm_get(radioIndex, 0,  MDMOID_DEV2_WIFI_RADIO, offsetof(Dev2WifiRadioObject, channel), varValue) != NULL)
        {
            if (obj->autoChannelEnable)
            {
                obj->channel = (unsigned int)atoi(varValue);
            }
            CMSMEM_REPLACE_STRING_FLAGS(obj->channelsInUse, varValue, mdmLibCtx.allocFlags);
        }

        if (cmsUtl_strcmp(obj->operatingFrequencyBand, "2.4GHz") == 0) // 2.4G
        {
            band = 2;
            if(!cmsUtl_strcasecmp(obj->operatingStandards,"v"))
            {
                REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->operatingStandards,"n", mdmLibCtx.allocFlags);
            }
        } 
        else if (cmsUtl_strcmp(obj->operatingFrequencyBand, "5GHz") == 0) // 5G
        {
            band = 5;
            if(!cmsUtl_strcasecmp(obj->operatingStandards,"v"))
            {
                REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->operatingStandards,"ac", mdmLibCtx.allocFlags);
            }
        }

        if (band != 0)
        {
            // update possible Channels, always use 20 bandwidth to get all possible channels.
            size = wlgetChannelList (obj->name, band, 20, NULL, possibleChannels, 32);
            if (size > 0)
            {
                for (i = 0 ; i < size-1 ; i++)
                    p += sprintf(p, "%d,", possibleChannels[i]);
                p += sprintf(p, "%d", possibleChannels[i]);

                REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->possibleChannels, channelBuf, mdmLibCtx.allocFlags);
            }
        }
    }

    return ret;
}

CmsRet stl_dev2WifiRadioStatsObject(_Dev2WifiRadioStatsObject *obj,
                                    const InstanceIdStack *iidStack)
{
    _Dev2WifiSsidObject *ssidObj = NULL;
    InstanceIdStack ssidIidStack = EMPTY_INSTANCE_ID_STACK;
    MdmPathDescriptor pathDesc;
    char *fullPathString = NULL;
    CmsRet ret = CMSRET_SUCCESS;

    char radioName[BUFLEN_16] = "\0";
    struct RadioCounters radioCounter = {0};

    UINT64 bytesReceived = 0, packetsReceived = 0, multicastBytesReceived = 0, multicastPacketsReceived = 0;
    UINT64 unicastPacketsReceived = 0, broadcastPacketsReceived = 0, errorsReceived = 0, discardPacketsReceived = 0;
    UINT64 bytesSent = 0, packetsSent = 0, multicastBytesSent = 0, multicastPacketsSent = 0;
    UINT64 unicastPacketsSent = 0, broadcastPacketsSent = 0, errorsSent = 0, discardPacketsSent = 0;

    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_DEV2_WIFI_RADIO;
    memcpy(&pathDesc.iidStack, iidStack, sizeof(InstanceIdStack));
    cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathString);

    obj->bytesReceived = 0;
    obj->packetsReceived = 0;
    obj->errorsReceived = 0;
    obj->discardPacketsReceived = 0;

    obj->bytesSent = 0;
    obj->packetsSent = 0;
    obj->errorsSent = 0;
    obj->discardPacketsSent = 0;
    obj->PLCPErrorCount = 0;
    obj->FCSErrorCount = 0;
    obj->invalidMACCount = 0;
    obj->packetsOtherReceived = 0;

    while ((ret = cmsObj_getNext(MDMOID_DEV2_WIFI_SSID, &ssidIidStack, (void **)&ssidObj)) == CMSRET_SUCCESS)
    {
        if ( cmsUtl_strcmp(fullPathString, ssidObj->lowerLayers) == 0 )
        {
            // use first matched ssid name as radio device name
            if (strlen(radioName) == 0)
                sprintf(radioName, ssidObj->name);

            rut_getIntfStats_uint64(ssidObj->name, &bytesReceived, &packetsReceived,
                                    &multicastBytesReceived, &multicastPacketsReceived, &unicastPacketsReceived, &broadcastPacketsReceived,
                                    &errorsReceived, &discardPacketsReceived,
                                    &bytesSent, &packetsSent,
                                    &multicastBytesSent, &multicastPacketsSent, &unicastPacketsSent, &broadcastPacketsSent,
                                    &errorsSent, &discardPacketsSent);

            obj->bytesReceived += bytesReceived;
            obj->packetsReceived += packetsReceived;
            obj->errorsReceived += (UINT32)errorsReceived;
            obj->discardPacketsReceived += (UINT32)discardPacketsReceived;

            obj->bytesSent += bytesSent;
            obj->packetsSent += packetsSent;
            obj->errorsSent += (UINT32)errorsSent;
            obj->discardPacketsSent += (UINT32)discardPacketsSent;
        }

        cmsObj_free((void **) &ssidObj);
    }

    // wifi only statistical counters
    if (strlen(radioName)!=0)
    {
        if ((ret = rutWifi_getRadioCounters(radioName, &radioCounter)) == CMSRET_SUCCESS)
        {
            obj->PLCPErrorCount = radioCounter.PLCPErrorCount;
            obj->FCSErrorCount = radioCounter.FCSErrorCount;
            obj->invalidMACCount = radioCounter.PLCPErrorCount;
            obj->packetsOtherReceived = radioCounter.packetsOtherReceived;
        }
    }

    CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2WifiSsidObject( _Dev2WifiSsidObject *obj,
                               const InstanceIdStack *iidStack __attribute__((unused)))
{
    UBOOL8 isLinkUp;
    char currentStatus[BUFLEN_16]= {0};
    char macAddrStr[MAC_STR_LEN+1]= {0};
    char propageLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]= {0};
    char lowerLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]= {0};
    int i = 0;
    MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
    _Dev2WifiRadioObject *radioObj = NULL;
    CmsRet ret;

    // get the first lower layer full path
    cmsUtl_strncpy(propageLayerBuf, obj->lowerLayers, sizeof(propageLayerBuf));
    while ((propageLayerBuf[i] != 0) &&
            (propageLayerBuf[i] != ',') &&
            (propageLayerBuf[i] != ' ') && (i < sizeof(lowerLayerBuf)-1))
    {
        lowerLayerBuf[i] = propageLayerBuf[i];
        i++;
    }
    /* Assume SSID lowerLayer will only have one fullpath */
    ret = cmsMdm_fullPathToPathDescriptor(lowerLayerBuf, &pathDesc);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not convert lowerLayer %s, ret=%d", obj->lowerLayers, ret);
        return ret;
    }

    if (pathDesc.oid != MDMOID_DEV2_WIFI_RADIO)
    {
        cmsLog_error("Expected LowerLayers to point to Wifi Radio (%d) got %d",
                     MDMOID_DEV2_WIFI_RADIO, pathDesc.oid);
        return ret;
    }

    ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, 0, (void **)&radioObj);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not get Wifi Radio object, ret=%d", ret);
        return ret;
    }
    else
    {
        if (!radioObj->enable && obj->enable)
        {
            CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
            obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
        }
        cmsObj_free((void *) &radioObj);
    }

    rutLan_getIntfStatus(obj->name, currentStatus, macAddrStr);
    isLinkUp = cmsNet_isInterfaceLinkUp(obj->name);

    if (isLinkUp && cmsUtl_strcmp(obj->status, MDMVS_UP))
    {
        /* Link has come UP */
        CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
        obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
        rutQos_reconfigAllQueuesOnLayer2Intf_dev2(obj->name);

        /* some classifications may reference this intf on ingress side,
         * so check if they need to be configured.
         */
        rutQos_reconfigAllClassifications_dev2(obj->name);
    }
    else if (!isLinkUp && !cmsUtl_strcmp(obj->status, MDMVS_UP))
    {
        /* Link has gone down */
        CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_DORMANT, mdmLibCtx.allocFlags);
        obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
        rutQos_reconfigAllQueuesOnLayer2Intf_dev2(obj->name);

        /* some classifications may reference this intf on ingress side,
         * so check if they need to be unconfigured.
         */
        rutQos_reconfigAllClassifications_dev2(obj->name);
    }

    /* always update the mac addr if it has changed */
    if (cmsUtl_strcmp(obj->MACAddress, macAddrStr) || cmsUtl_strcmp(obj->BSSID, macAddrStr))
    {
        CMSMEM_REPLACE_STRING_FLAGS(obj->MACAddress, macAddrStr, mdmLibCtx.allocFlags);
        CMSMEM_REPLACE_STRING_FLAGS(obj->BSSID, macAddrStr, mdmLibCtx.allocFlags);
        obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
    }

    /* Calculate and return the TR181 LastChange */
    IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);

    return CMSRET_SUCCESS;
}

CmsRet stl_dev2WifiSsidStatsObject(_Dev2WifiSsidStatsObject *obj,
                                   const InstanceIdStack *iidStack)
{
    _Dev2WifiSsidObject *ssidObj = NULL;
    InstanceIdStack parentIidStack = *iidStack;
    CmsRet ret = CMSRET_SUCCESS;

    if ((ret = cmsObj_getAncestor(MDMOID_DEV2_WIFI_SSID,
                                  MDMOID_DEV2_WIFI_SSID_STATS,
                                  &parentIidStack,
                                  (void **) &ssidObj)) == CMSRET_SUCCESS)
    {
        if (obj == NULL)
        {
            rut_clearWanIntfStats(ssidObj->name);
        }
        else
        {
            UINT64 bytesReceived = 0, packetsReceived = 0, multicastBytesReceived = 0, multicastPacketsReceived = 0;
            UINT64 unicastPacketsReceived = 0, broadcastPacketsReceived = 0, errorsReceived = 0, discardPacketsReceived = 0;
            UINT64 bytesSent = 0, packetsSent = 0, multicastBytesSent = 0, multicastPacketsSent = 0;
            UINT64 unicastPacketsSent = 0, broadcastPacketsSent = 0, errorsSent = 0, discardPacketsSent = 0;

            struct SSIDCounters ssidCounter = {0};

            rut_getIntfStats_uint64(ssidObj->name, &bytesReceived, &packetsReceived,
                                    &multicastBytesReceived, &multicastPacketsReceived, &unicastPacketsReceived, &broadcastPacketsReceived,
                                    &errorsReceived, &discardPacketsReceived,
                                    &bytesSent, &packetsSent,
                                    &multicastBytesSent, &multicastPacketsSent, &unicastPacketsSent, &broadcastPacketsSent,
                                    &errorsSent, &discardPacketsSent);

            obj->bytesReceived = bytesReceived;
            obj->packetsReceived = packetsReceived;
            obj->multicastPacketsReceived = multicastPacketsReceived;
            obj->unicastPacketsReceived = unicastPacketsReceived;
            obj->broadcastPacketsReceived = broadcastPacketsReceived;
            obj->errorsReceived = (UINT32)errorsReceived;
            obj->discardPacketsReceived = (UINT32)discardPacketsReceived;

            obj->bytesSent = bytesSent;
            obj->packetsSent = packetsSent;
            obj->multicastPacketsSent = multicastPacketsSent;
            obj->unicastPacketsSent = unicastPacketsSent;
            obj->broadcastPacketsSent = broadcastPacketsSent;
            obj->errorsSent = (UINT32)errorsSent;
            obj->discardPacketsSent = (UINT32)discardPacketsSent;

            obj->unknownProtoPacketsReceived = 0;

            // wifi only statistical counters
            rutWifi_getSSIDCounters(ssidObj->name, &ssidCounter);
            obj->retransCount = ssidCounter.retransCount;
            obj->failedRetransCount = ssidCounter.failedRetransCount;
            obj->retryCount = ssidCounter.retryCount;
            obj->multipleRetryCount = ssidCounter.multipleRetryCount;
            obj->ACKFailureCount = ssidCounter.ACKFailureCount;
            obj->aggregatedPacketCount = ssidCounter.aggregatedPacketCount;
        }
        cmsObj_free((void **) &ssidObj);
    }

    return ret;

}

#endif  /* DMP_DEVICE2_WIFIRADIO_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */
