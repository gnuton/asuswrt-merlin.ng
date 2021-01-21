/*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
:>
*/

#ifndef __RUT_WIFIWAN_H__
#define __RUT_WIFIWAN_H__


/*!\file rut_wifiwan.h
 * \brief System level interface functions for wifi interface functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"


/** find the WanWifiIntf object with the given layer 2 ifName.
 *
 * @param ifName (IN) layer 2 ifName of WanWifiIntf to find.
 * @param iidStack (OUT) iidStack of the WanWifiIntf object found.
 * @param wlIntfCfg (OUT) if not null, this will contain a pointer to the found
 *                         WanWifiIntf object.  Caller is responsible for calling
 *                         cmsObj_free() on this object.
 *
 * @return UBOOL8 indicating whether the desired WanWifiIntf object was found.
 */
UBOOL8 rutWanWifi_getWlIntfByIfName(char *ifName, InstanceIdStack *iidStack, WanWifiIntfObject **wlIntfCfg);


/** if device has Wifi wan  
 * 
 * @param none
 *
 * @return UBOOL8 indicating whether Wifi wan be set.
 */
UBOOL8 rutWanWifi_isWlWanMode();


/** Given the iidStack of the (layer 2) Wifi WAN Intf Config object, find
 *  the child IP connection object and return the Layer 3 intf name (should
 *  be wifi0).
 *
 * @param parentIidStack (IN) iidStack of the WifiWan Intf Object.
 * @param intfName (OUT) Layer 3 intfname.  Must point to a buffer of at
 *                       least CMS_IFNAME_LENGTH bytes.
 *
 * @return CmsRet
 */
CmsRet rutWifiWan_getLayer3IntfNameByIidStack(const InstanceIdStack *parentIidStack,
                                              char *intfName);


/** Move the Wl interface from Lan side to Wan side as Wan Layer2 interface.
 *
 * @param ifName (IN) Name of the Wl interface.
 */
void rutWan_moveWifiLanToWan(const char *ifName);


/** Move the Wan Wifi Layer2 interface from Wan side to Lan side.
 *
 * @param ifName (IN) Name of the Wl interface.
 */
void rutWan_moveWifiWanToLan(const char *ifName);


/** Get the (layer 2) WanWifi InterfaceConfig object.
 *
 * @param iidStack (OUT) The iidStack of the requested object.
 * @param wanWifiIntfObj (OUT) The requested object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWifiWan_getWanWifiObject(InstanceIdStack *iidStack,
                                   WanWifiIntfObject **wanWifiIntfObj);


/** Get the Wifi as WAN interface status and return it in the provided buffer.
 *  Note this function is special for wifi as WAN.  This function returns
 *  MDMVS_UP in statusStr if the interface has associated with the AP.
 *  (should it be associated and authenticated?)  Cannot simply use
 *  cmsNet_isInterfaceLinkUp in this situation.
 *
 * @param ifName (IN) Name of the Wl interface.
 * @param statusStr (OUT) Will contain the status of the Wl interface, the buffer
 *                        must be at least 16 bytes long.
 */
void rutWifiWan_getIntfStatus(const char *ifName, char *statusStr);

#endif  /* __RUT_WIFIWAN_H__ */

