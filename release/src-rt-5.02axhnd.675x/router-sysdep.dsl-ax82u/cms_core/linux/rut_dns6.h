/*
#
#  Copyright 2011, Broadcom Corporation
#
# <:label-BRCM:2011:proprietary:standard
# 
#  This program is the proprietary software of Broadcom and/or its
#  licensors, and may only be used, duplicated, modified or distributed pursuant
#  to the terms and conditions of a separate, written license agreement executed
#  between you and Broadcom (an "Authorized License").  Except as set forth in
#  an Authorized License, Broadcom grants no license (express or implied), right
#  to use, or waiver of any kind with respect to the Software, and Broadcom
#  expressly reserves all rights in and to the Software and all intellectual
#  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
#  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
#  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
# 
#  Except as expressly set forth in the Authorized License,
# 
#  1. This program, including its structure, sequence and organization,
#     constitutes the valuable trade secrets of Broadcom, and you shall use
#     all reasonable efforts to protect the confidentiality thereof, and to
#     use this information only in connection with your use of Broadcom
#     integrated circuit products.
# 
#  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
#     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
#     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
#     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
#     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
#     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
#     PERFORMANCE OF THE SOFTWARE.
# 
#  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
#     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
#     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
#     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
#     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
#     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
#     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
#     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
#     LIMITED REMEDY.
# :>
*/

#ifndef __RUT_DNS6_H__
#define __RUT_DNS6_H__

/** 
 * @param prefix (OUT)  The prefix associated with the WAN interface
 *
 * @param wanIfName (IN)  The WAN interface
 * @return UBOOL8 - TRUE --> get prefix
 */
UBOOL8 fetchPrefix(const char *wanIfName, char *prefix);

/** 
 * @param prefix (OUT)  The IPv6 dnsServers associated with the WAN interface
 *
 * @return UBOOL8 - TRUE --> get dnsServers6
 */

UBOOL8 fetchActiveDnsServers6(char *dnsServers6);



/** 
 *
 * @param wanIfName (IN)  The WAN interface
 * @param activeDNServer6 (IN)  The active DNSServres for IPv6
 * @return UBOOL8 - TRUE --> this Wan has the active dns
 */
UBOOL8 isActiveDNSServer6(const char *wanIfName, const char *activeDNServer6);


/** 
 *
 * @param wanIfName (IN)  The WAN interface
 * @param activeDNSServers6 (IN)  The activeDNSServer for IPv6 
 * @param ifName6  (OUT)  The WAN interface name for IPv6, same as WAN interface if prefix is fetched.
 * @param subnetCidr6  (OUT)  The prefix (subnet)
 * @param DNSServer6  (OUT)  The DNSServers for this IPv6 WAN.
 * @return CmsRet
 */
CmsRet getDnsParam6(const char *wanIfName, 
                    const char *activeDNSServers6,
                    char *ifName6, 
                    char *subnetCidr6, 
                    char *DNSServer6);

#endif /*  __RUT_DNS6_H__ */
