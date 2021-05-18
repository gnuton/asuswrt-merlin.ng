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

#ifndef __RUT2_DHCPV6_H__
#define __RUT2_DHCPV6_H__

/*!\file rut2_dhcpv6.h
 * \brief Helper functions for dhcpv6 functionality.
 *
 * In most cases, the functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"
#include "cms_eid.h"
#include "cms_msg.h"


/** return UBOOL8 TRUE if there is a DHCPv6 client for the specified IP.Interface
 *
 * @param ifName  (IN) IP.Interface fullpath
 * @param iana      (OUT) iana request
 * @param iapd      (OUT) iapd request
 * @param um       (OUT) unnumbered model
 * @param pid        (OUT) pid of dhcpv6 client object
 *
 * @return UBOOL8 TRUE if there is a DHCPv6 client for the specified IP.Interface
 */
UBOOL8 rutDhcpv6_isClientEnabled_dev2(const char *ipIntfFullPath, UBOOL8 *iana,
                         UBOOL8 *iapd, UBOOL8 *um, SINT32 *pid);


/** Set pid to dhcpv6 client after dhcpc is succeesfully launced.
 *
 * @param ipIntfFullPath  (IN) WAN ip interface full path
 * @param pid    (IN) pid to be set in dhcpv6 client object
 * @param status (IN) status to be set in dhcpv6 client object
 *
 */
void rutDhcpv6_setClientPidAndStatusByIpIntfFullPath_dev2(const char *ipIntfFullPath,
                              const SINT32 pid, const char *status);


/** update IANAPrefixes for dhcp6s
 *
 * @param ifpath          (IN) LAN ip interface full path
 * @param prefixes        (OUT)
 * @param prefixValue     (OUT)
 *
 */
void rutDhcpv6_updateIANAPrefixes(const char *ifpath, char **prefixes, char *prefixValue);


/** update IANAPrefixes by ULA for dhcp6s
 *
 * @param ifpath          (IN) LAN ip interface full path
 * @param prefixes        (OUT)
 * @param prefixValue     (OUT)
 *
 */
void rutDhcpv6_updateIANAPrefixesByULA(const char *ifpath, char **prefixes, char *prefixValue);


/** update aftr
 *
 * @param ifpath      (IN) WAN ip interface full path
 * @param aftr        (IN)
 *
 */
void rutDhcpv6_updateAftr(const char *ifpath, const char *aftr);

/** update MAPT/MAPE
 *
 * @param ifpath          (IN) WAN ip interface full path
 * @param mechanism       (IN)
 * @param brprefix        (IN)
 * @param ipv4prefix      (IN)
 * @param ipv6prefix      (IN)
 * @param ealen           (IN)
 * @param psidoffset      (IN)
 * @param psidlen         (IN)
 * @param psid            (IN)
 * @param isFMR           (IN)
 *
 */
void rutDhcpv6_updateMap(const char *ifpath, const char *mechanism, const char *brprefix, const char *ipv4prefix,
        const char *ipv6prefix, UINT32 ealen, UINT32 psidoffset, UINT32 psidlen,
        UINT32 psid, UBOOL8 isFMR);

#endif  /* __RUT2_DHCPV4_H__ */

