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

#ifndef __RUT2_DHCPV4_H__
#define __RUT2_DHCPV4_H__

/*!\file rut2_dhcpv4.h
 * \brief Helper functions for dhcpv4 functionality.
 *
 * In most cases, the functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"
#include "cms_eid.h"
#include "cms_msg.h"


/** When IP.Interface is deleted, call this helper function to delete
 *  any DHCPv4 Relay Forwarding entries which are pointing to the IP.Interface
 *  which is being deleted.
 *
 *  @param ipIntfFullPath (IN) fullpath of the IP.Interface which is being deleted.
 *
 */
void rutDhcpv4Relay_deleteForwardingEntry_dev2(const char *ipIntfFullPath);


/** Set pid to dhcpv4 client after dhcpc is succeesfully launced.
 *
 * @param ipIntfFullPath  (IN) WAN ip interface full path
 * @param pid    (IN) pid to be set in dhcpv4 client object
 * @param status (IN) status to be set in dhcpv4 client object
 *
 */
void rutDhcpv4_setClientPidAndStatusByIpIntfFullPath_dev2(const char *ipIntfFullPath,
                              const SINT32 pid, const char *status);


/** Get pid to dhcpv4 client after dhcpc is succeesfully launced.
 *
 * @param ipIntfFullPath  (IN) WAN ip interface full path
 * @param pid (OUT) pid of dhcpv4 client object
 *
 * @return CmsRet       enum.
 */
CmsRet rutDhcpv4_getClientPidByIpIntfFullPath_dev2(const char *ipIntfFullPath,  SINT32 *pid);


/** return UBOOL8 TRUE if there is a DHCPv4 client for the specified IP.Interface
 *
 * @param ifName  (IN) IP.Interface fullpath
 *
 * @return UBOOL8 TRUE if there is a DHCPv4 client for the specified IP.Interface
 */
UBOOL8 rutDhcpv4_isClientEnabled_dev2(const char *ipIntfFullPath);


/** Stop the DHCPv4 client for the specified IP.Interface fullpath
 *
 * @param inIntfFullPath (IN) IP.Interface fullpath
 */
void rutDhcpv4_stopClientByIpIntfFullPath_dev2(const char *ipIntfFullPath);


/** Stop the DHCPv4 client with the specified PID
 *
 * @param pid (IN) pid of DHCPv4 client
 */
void rutDhcpv4_stopClientByPid_dev2(SINT32 pid);


/** config the DHCPv4 client (start or stop) by on the interface by ifName 
 *
 * @param ifName (IN)  interface name with the DHCPv4 client 
 * @param enableFlag (IN)  if enableFlag is TRUE, start the dhcpv4 client; if FALSE, stop the client.
 *
 * @return CmsRet       enum. 
 */
CmsRet rutDhcpv4_configClientByIfName_dev2(const char *ifName, UBOOL8 enableFlag);


/** config the DHCPv4 client (start or stop) by on the interface by ifName 
 *
 * @param ifName (IN)  bridge interface name with the DHCPv4 client 
 *
 * @return CmsRet       enum. 
 *
 * NOTE: rutDhcpv4_restartDhcpv4Client_idg is not implemented since hybrid
 * model seems to work correct when lan interface is added to br0.  This function may
 * be needed later on
 */

CmsRet rutDhcpv4_restartDhcpv4Client_dev2(const char *bridgeIfName);

#endif  /* __RUT2_DHCPV4_H__ */

