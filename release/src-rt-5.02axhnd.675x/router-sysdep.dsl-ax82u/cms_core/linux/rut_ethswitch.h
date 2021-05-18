/***********************************************************************
 *
 *  Copyright (c) 2007-2008  Broadcom Corporation
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

#ifndef __RUT_ETHSWITCH_H__
#define __RUT_ETHSWITCH_H__


/*!\file rut_ethswitch.h
 * \brief System level interface functions for ethernet switch functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"
#include "cms_qos.h"




void rutEsw_createVirtualPorts(const char *ifName, UINT32 numIfc);

void rutEsw_deleteVirtualPorts(const char *ifName, UINT32 numIfc);


/** Insert kernel module neede for virtual ports on ethernet switch.
 *
 * This function could be generalized to cover other kernel module insertions.
 *
 */
void rutEsw_insertEthernetSwitchKernelModule(void);


/** Remove kernel module for virtual ports on ethernet switch.
 */
void rutEsw_removeEthernetSwitchKernelModule(void);


/** Get the number of virtual ports supported on the given interface/ethernet switch.
 *
 * @param interfaceName (IN) The name of the interface/ethernet switch.
 *
 * @return 1 if the given interface name does not support virtual ports,
 *           or greater than 1 indicating the number of virtual ports supported by
 *           this interface.
 */
UINT32 rutEsw_getNumberOfVirtualPorts(const char *interfaceName);


/** Return true if the virtual ports feature is enabled.
 * 
 * @return TRUE if the virtual ports feature is enabled.
 */
UBOOL8 rutEsw_isVirtualPortsEnabled(void);


/** Return true if the ethernet switch is on the specified interface.
 * 
 * @return TRUE if the ethernet switch is on the specified interface.
 */
UBOOL8 rutEsw_isEthernetSwitchIfNameMatch(const char *ifName);


/** Return true if the ethernet switch is on eth0
 * 
 * @return TRUE if the ethernet switch is on eth0.
 */
UBOOL8 rutEsw_isEthernetSwitchOnEth0(void);




/** Call this function whenever "something" changes which could affect
 *  port pause setting.  This function determines the correct port pause
 *  setting and sets it.  All code should call this function instead of
 *  trying to implement the logic and/or setting the port pause feature locally.
 *
 *  "something" is multicast precendence or QoS classifications.
 *
 *  @param excludeClassKey  (IN) If a classification is currently being
 *                unconfigured or deleted, exclude it from consideration.
 *                If all classifications should be considered, pass in
 *                QOS_CLS_INVALID_INDEX
 */
void rutEsw_updatePortPauseFlowCtrlSetting(UINT32 excludeClassKey);


/** Call this function whenever "something" changes which could affect
 *  Real HW switching setting.  This function determines the correct
 *  setting and sets it.  All code should call this function instead of
 *  trying to implement the logic and/or setting Real HW switching locally.
 *
 *  "something" is LANVLAN or QoS classifications.
 *
 *  @param excludeClassKey  (IN) If a classification is currently being
 *                unconfigured or deleted, exclude it from consideration.
 *                If all classifications should be considered, pass in
 *                QOS_CLS_INVALID_INDEX
 */
void rutEsw_updateRealHwSwitchingSetting(UINT32 excludeClassKey);




#endif /*RUT_ETHSWITCH_H_*/
