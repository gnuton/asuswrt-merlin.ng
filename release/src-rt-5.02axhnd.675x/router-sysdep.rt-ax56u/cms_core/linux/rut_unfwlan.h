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

#ifndef __RUT_UNF_WLAN_H__
#define __RUT_UNF_WLAN_H__


/*!\file rut_unfwlan.h
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


/** Return the currently detected wifi phy type.
 *
 * Returns the wifi phy type after accessing the wlctl utility.
 *
 * @param radioIndex (IN) The wl radio index, which starts from 0.
 */
char *rutWlan_getPhyType(int radioIndex);

/** Return the enable status of wifi radio.
 *
 * @param radioIndex (IN) The wl radio index, which starts from 0.
 */
UBOOL8 rutWlan_getRadioEnabled(int radioIndex);

/** Return all possible channels for a wifi radio.
 *
 * The returned channel list is a string of all supported channels, separated by ','
 *
 * @param value (IN/OUT) The destination of the result string.
 * @param size (IN) The length of parameter 'value'.
 * @param radioIndex (IN) The wl radio index, which starts from 0.
 */
void rutWlan_getPossibleChannels(char *value, size_t size, int radioIndex);

/** Calculate standard string based on current band and mode of WiFi radio.
 *
 * @param wlBand (IN) The wl radio band, which is either BAND_A or BAND_B.
 * @param wlgMode (IN) The wl radio gmode, which indicates further the mode WiFi
 * radio operates in if it is BAND_B.
 *
 */
const char *rutWlan_getStandard(int wlBand, int wlgMode);
void rutWlan_getWlBandGmode(const char *standard, int *wlBand, int *wlgMode);

/** Calculate rate in bps for maxBitRate.
 *
 * @param maxBitRate (IN) The max bit rate in Mbps.
 *
 */
long rutWlan_getWlRate(char *maxBitRate);

/** Calculate rate in mbps for wlRate.
 *
 * @param maxBitRate (IN/OUT) The max bit rate in Mbps, this is a string.
 * @param size (IN) The size of destination buffer.
 * @param wlRate (IN) The bit rate in bps.
 *
 */
void rutWlan_getMaxBitRate(char *maxBitRate, size_t size, long wlRate);

/** Get BeaconType from wlAuthMode.
 *
 * @param wlRate (IN) The auth mode as defined in BRCM parameter nodes.
 */
const char *rutWlan_getBeaconType(const char *wlAuthMode);

/** Return a const string indicating wlAuthMode based on current TR98 defined parameter values.
 *
 * @param beaconType (IN).
 * @param basicAuthMode (IN).
 * @param wpaAuthMode (IN).
 * @param 11iAuthMode (IN).
 * @param wepEnabled (IN).
 *
 */
const char *rutWlan_getWlAuthMode(const char *beaconType, const char *basicAuthMode,
                                  const char *wpaAuthMode, const char *IEEE11iAuthMode,
                                  UBOOL8 wepEnabled);

UBOOL8 rutWlan_getWepEnabled(const char *beaconType, const char *basicEncryptModes,
                             const char *wpaEncryptModes, const char *IEEE11iEncryptModes);

const char *rutWlan_getCrypto(const char *beaconType,
                              const char *WPAEncryptModes,
                              const char *IEEE11iEncryptModes);

const char *rutWlan_getBasicDataTransmitRates(int wlBand, int wlgMode, const char *wlBasicRate);

const char* rutWlan_getWlBasicRate(const char *basicDataTransmitRates, int wlBand, int wlgMode);

const char *rutWlan_getDeviceOperationMode(const char *wlMode);

const char *rutWlan_getWlMode(const char *deviceOperationMode);
#endif /* __RUT_UNFWLAN_H__ */
