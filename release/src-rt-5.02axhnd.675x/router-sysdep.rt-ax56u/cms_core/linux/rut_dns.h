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

#ifndef __RUT_DNS_H__
#define __RUT_DNS_H__


/** Create a file "var/dnsinfo.conf" file with connected WAN interface (connectionStatus is "Connected"
 * and subnet (if this WAN is part of the interface grouping) and WAN dns info.
 *  Format:  
 *    WAN interface name IPV4, WAN interface name IPV6;subnet(CIDR) IPv4, subnet(CIDR) IPv6;WAN DNS IPv4, WAN DNS IPv6;ProcessName(s) -- if more
 *    than one process use the wan, it will be like 'tr69c,sipd' with a common as the seperator.
 *
 *  Eg:  (IPV4 only)
 *   ppp0.1,;192.168.1.0/24,;10.6.33.1,63.70.210.40,;;    // default group (system default dns)
 *   atm0.1,;192.168.2.0/24,;10.6.37.1,63.70.210.40,;;    // IPoE with some lan ports in an interface group on subnet "192.168.2.1/24" 
 *   atm1.1,;127.0.0.1/32,;10.7.2.8,20.1.2.5,;tr69c       // No interface grouping WAN service for TR69 or voice
 *
 *  Eg:  (IPv4 and IPv6)
 *   ppp0.1,ppp0.1;192.168.1.0/24,200f:0:0:1::1/64;10.6.33.1,63.70.210.40,123f:567f:0:1::1/64;;   // default group (system default dns)
 *   atm0.1,atm0.1;192.168.2.0/24,200f:0:0:1::1/64;10.6.33.1,63.70.210.40,123f:567f:0:1::1/64 // IPoE with some lan ports in an interface group on subnet "192.168.2.1/24" 
 *                                                                                     // todo: ipv6 interface group   
 *   atm1.1,atm1.1;127.0.0/32,0::1/64;10.7.2.8,20.1.2.5,123f:567f:0:1::1/64;tr69c           // No interface grouping WAN service for TR69 or voice. 
 *
 * @return CmsRet enum. 
 */

CmsRet rutDns_createDnsInfoConf(void);




/** Do one pass of writing to /var/dnsinfo.conf
 *
 * @param fp              (IN) File pointer to write to.
 * @param needDefaultDns  (IN) in first pass, it is TRUE.  Second pass, FALSE.
 *
 * @return CmsRet enum.
 */
CmsRet rutDns_dumpDnsInfo(FILE *fp, UBOOL8 needDefaultDns);

CmsRet rutDns_dumpDnsInfo_igd(FILE *fp, UBOOL8 needDefaultDns);

CmsRet rutDns_dumpDnsInfo_dev2(FILE *fp, UBOOL8 needDefaultDns);

#if defined(SUPPORT_DM_LEGACY98)
#define rutDns_dumpDnsInfo(f, d)       rutDns_dumpDnsInfo_igd((f), (d))
#elif defined(SUPPORT_DM_HYBRID)
#define rutDns_dumpDnsInfo(f, d)       rutDns_dumpDnsInfo_igd((f), (d))
#elif defined(SUPPORT_DM_PURE181)
#define rutDns_dumpDnsInfo(f, d)       rutDns_dumpDnsInfo_dev2((f), (d))
#elif defined(SUPPORT_DM_DETECT)
#define rutDns_dumpDnsInfo(f, d)       (cmsMdm_isDataModelDevice2() ? \
                                     rutDns_dumpDnsInfo_dev2((f), (d)) : \
                                     rutDns_dumpDnsInfo_igd((f), (d)))
#endif


CmsRet rutDns_writeDnsInfoLine(FILE *fp,
                               const char *ifName4,
                               const char *ifName6,
                               const char *subnetCidr4,
                               const char *subnetCidr6,
                               const char *DNSServer4,
                               const char *DNSServer6);


/*
 * Write out the statically configured hosts to the given file handle.
 */
void rutDns_writeStaticHosts(FILE *fp);
void rutDns_writeStaticHosts_igd(FILE *fp);
void rutDns_writeStaticHosts_dev2(FILE *fp);

#if defined(SUPPORT_DM_LEGACY98)
#define rutDns_writeStaticHosts(f)    rutDns_writeStaticHosts_igd((f))
#elif defined(SUPPORT_DM_HYBRID)
#define rutDns_writeStaticHosts(f)    rutDns_writeStaticHosts_igd((f))
#elif defined(SUPPORT_DM_PURE181)
#define rutDns_writeStaticHosts(f)    rutDns_writeStaticHosts_dev2((f))
#elif defined(SUPPORT_DM_DETECT)
#define rutDns_writeStaticHosts(f)    (cmsMdm_isDataModelDevice2() ? \
                                       rutDns_writeStaticHosts_dev2((f)) : \
                                       rutDns_writeStaticHosts_igd((f)))
#endif


#endif /* __RUT_DNS_H__ */
