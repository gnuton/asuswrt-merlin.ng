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

#ifndef __RUT_EBTALBES_H__
#define __RUT_EBTALBES_H__



/*!\file rut_ebtables.h
 * \brief System level interface functions for ebtables functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"


/** execute access time restriction setting.
 * 
 * @param *Obj       (IN) the new _AccessTimeRestrictionObject.
 * @param add         (IN) add or delete
 * @return  none
 */
void rut_accessTimeRestriction(const _AccessTimeRestrictionObject *Obj, const UBOOL8 add);


/** mac filter policy change.
 * 
 * @param *ifName     (IN) interface name
 * @param isForward   (IN) 
 * @return  none
 */
void rutEbt_changeMacFilterPolicy(const char *ifName, UBOOL8 isForward);


/** execute mac filter setting.
 * 
 * @param protocol       (IN) Protocol
 * @param direction      (IN) Direction
 * @param sourceMAC      (IN) SourceMAC
 * @param destinationMAC (IN) DestinationMAC
 * @param *ifName        (IN) interface name
 * @param *policy        (IN) rule policy
 * @param add            (IN) add or delete
 * @return  none
 */
void rutEbt_addMacFilter_raw(char* protocol ,char* direction ,char* sourceMAC ,char* destinationMAC ,const char *ifName, const char *policy, UBOOL8 add);


/** execute mac filter setting.
 * 
 * @param *InObj       (IN) the new MacFilterCfgObject.
 * @param *ifName     (IN) interface name
 * @param *policy      (IN) rule policy
 * @param add           (IN) add or delete
 * @return  none
 */
void rutEbt_addMacFilter(const _MacFilterCfgObject *InObj, const char *ifName, const char *policy, UBOOL8 add);


/** execute mac filter setting.
 * 
 * @return  none
 */
void rutEbt_avoidDhcpAcceleration(void);

/** Add pppoe interface to the bridge for PPPoE pass-through configuration
 * 
 * @param *cmdLine       (IN) cmd line pointer
 * @param *cmdLen        (IN) cmd line length
 * @param *baseL3IfName  (IN) pppoe input interface name (vlan interface name)
 *
 * @return  none
 */
void rutEbt_addPppIntfToBridge(char *cmdLine, UINT32 cmdLen, const char *baseL3IfName);

/** Remove pppoe interface from the bridge for PPPoE pass-through configuration
 * 
 * @param *baseL3IfName  (IN) pppoe input interface name (vlan interface name)
 *
 * @return  none
 */

void rutEbt_removePppIntfFromBridge(const char *baseL3IfName);

/** Add default rules for IPv6 at LAN sides
 * 
 * @param void
 *
 * @return  none
 */

void rutEbt_defaultLANSetup6(void);


/** Remove pppoe interface from the bridge for PPPoE pass-through configuration
 * 
 * @param *prefix       (IN) IPv6 addr prefix
 * @param *add          (IN) add or remove rule
 *
 * @return  none
 */

void rutEbt_configICMPv6Reply(const char *prefix, UBOOL8 add);

#endif


