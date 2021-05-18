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

#ifndef __RUT2_WIFI_H__
#define __RUT2_WIFI_H__


/*!\file rut2_wifi.h
 * \brief Header file for common TR181 Wifi helper functions.
 *
 * The functions in this file should only be called by
 * RCL2, STL2, and other RUT2 functions.
 */


#include "cms.h"
#ifndef BUILD_BRCM_UNFWLCFG
#include <wlcsm_lib_api.h>
#endif

/** tmp buf used to form a config line, delcared in rut2_wifi.h */
extern char wifi_configBuf[BUFLEN_128];


/** Write a config string to "nvram"
 *
 * @param configStr  (IN) config string
 *
 */
void rutWifi_writeNvram(const char *configStr);
/**  API to find SsidObj by interfaceName
 *
 * @param ifName (IN)   interface name such as "wl0.1"
 * @param ssidObj(OUT)  SsidObj found
 * @param iidStack(OUT) iidStack
 *
 */
UBOOL8 rutWifi_getWlanSsidObjByIfName(const char *ifName,
                                      Dev2WifiSsidObject **ssidObj,
                                      InstanceIdStack *iidStack);

struct RadioCounters {
    UINT32 PLCPErrorCount;
    UINT32 FCSErrorCount;
    UINT32 invalidMACCount;
    UINT32 packetsOtherReceived;
};

struct SSIDCounters {
    UINT32 retransCount;
    UINT32 failedRetransCount;
    UINT32 retryCount;
    UINT32 multipleRetryCount;
    UINT32 ACKFailureCount;
    UINT32 aggregatedPacketCount;
};

CmsRet rutWifi_getRadioCounters(const char *devName, struct RadioCounters *rCounters);
CmsRet rutWifi_getSSIDCounters(const char *devName, struct SSIDCounters *sCounters);
#ifdef DMP_DEVICE2_WIFIACCESSPOINT_1
void rutWifi_find_AP_ByIndex_locked(int radioIndex, int ssidIndex, InstanceIdStack *iidStack, void **obj);
#ifndef BUILD_BRCM_UNFWLCFG
void rutWifi_AssociatedDeviceUpdated_dev2(WL_STALIST_SUMMARIES *sta_summaries,InstanceIdStack *on_apIidStack,_Dev2WifiAssociatedDeviceObject *sta);
Dev2WifiSsidObject*  rutWifi_get_AP_SSID_dev2(_Dev2WifiAccessPointObject  *obj);

CmsRet rutWifi_update_STA_HostEntry(Dev2WifiAssociatedDeviceObject *staObj,InstanceIdStack *sta_iidStack,int remove);
CmsRet rutWifi_get_AP_Radio_dev2(_Dev2WifiAccessPointObject  *apObj,void **radioObj,InstanceIdStack *iidStack);
void rutWifi_Clear_AssocicatedDevices(Dev2WifiAccessPointObject *apObj,InstanceIdStack *iidStack);
#endif  /* BUILD_BRCM_UNFWLCFG */
CmsRet rutWifi_update_STA_HostEntry(Dev2WifiAssociatedDeviceObject *staObj,InstanceIdStack *sta_iidStack,int remove);
#define MDM_ACTIVE_ENTRY (2)
#define MDM_REMOVE_ENTRY (0)
#define MDM_INVALID_ONLY (1)
#endif
#endif  /* __RUT2_WIFI_H__ */
