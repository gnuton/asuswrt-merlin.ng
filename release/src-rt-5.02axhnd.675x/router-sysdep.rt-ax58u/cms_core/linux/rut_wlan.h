/***********************************************************************
 *
 *  Copyright (c) 2009  Broadcom Corporation
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

#ifndef __RUT_WLAN_H__
#define __RUT_WLAN_H__


/*!\file rut_wlan.h
 * \brief System level interface functions for WLAN functionality.
 *
 * In most cases, the functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */



/** Go through all the child WlVirtIntf objects and make sure
 *  their corresponding filter->filterBridgeReference fields are 
 *  updated to reflect the master wlEnbl parameter.
 * 
 * If the master wlEnbl parameter is FALSE, then all filterBridgeReferences
 * should be set to -1.  If master wlEnbl is TRUE, and the virtIntf->wlEnblSsid
 * is also true, then the filterBridgeReference should be set to the approriate
 * bridge entry.
 * 
 * @param iidStack (IN) The iidStack of the WlBaseCfg object.
 * @param enable   (IN) The wlEnbl parameter in the WlBaseCfg object.
 * 
 */
void rutWlan_modifyVirtIntfFilters(const InstanceIdStack *iidStack,
                                   UBOOL8 enable);


/** Set the filterBridgeRef for this virtIntf accordingly.
 *
 *  This will have the effect of putting this virtIntf under the
 *  bridge specified in the virtIntf object.
 * 
 * @param wlIfName  (IN) The wl interface name.
 * @param bridgeKey (IN) The bridge key that the interface should be moved to.
 */
void rutWlan_enableVirtIntfFilter(const char *wlIfName, UINT32 bridgeKey);


/** Set the filterBridgeRef for this virtIntf to -1.
 *
 *  This will have the effect of removing this virtIntf from the bridge.
 *
 * @param wlIfName  (IN) The wl interface name.
 */
void rutWlan_disableVirtIntfFilter(const char *wlIfName);

void rutWlan_requestRestart(char *cmd, int index);

CmsRet rutWlan_get_sta_Host(const _LanWlanAssociatedDeviceEntryObject *sta, InstanceIdStack *host_iidStack, LanHostEntryObject **host);
CmsRet rutWifi_AssociatedDeviceUpdate(void *sta_summaries);
CmsRet rutWifi_modify_sta_count(const InstanceIdStack *sta_iidStack,int delta);
void rutWlan_clear_ap_stas(const _WlVirtIntfCfgObject *virtualInf,const InstanceIdStack *iidStack);
#endif /* __RUT_WLAN_H__ */
