/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
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
#ifndef __RUT_NETWORK_H__
#define __RUT_NETWORK_H__


/*!\file rut_network.h
 * \brief System level interface functions for networking functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"


/** This is the top level function for starting all DNS configuration.
 *  It is used by both TR98 and TR181 code.
 *  Creates udhcp.conf, resolv.conf, and dnsinfo.conf.  Notifies udhcpd and
 *  dnsproxy.
 *
 * @return CmsRet enum.
 */
CmsRet rutNtwk_configActiveDnsIp(void);


#define CMS_FAKE_STATIC_DNS_IFNAME  "StaticDNS"


/** Select the activeDNSServers and also return which interface the
 *  activeDNSServers came from (in activeDNSIfName) using the following
 *  algorithm:
 *
 * 1). If staticDNSServers were provided, just use those and return.
 *     This means that a configured static DNS has the highest preference.
 *     If staticDNSServers are used, activeDNSIfName will contain CMS_FAKE_STATIC_DNS_IFNAME.
 * 2). If no static DNS servers, try dnsIfNameList in order.  Use the DNS
 *     servers from the first interface that is UP.
 * 3). If nothing found, then use the DNS servers from any routed and
 *     connected WAN service in the system.
 * 4). If nothing found, then set it to default ("0.0.0.0,0.0.0.0")
 * 
 * This function works in either IPv4 mode or IPv6 mode, depending on the
 * ipvx flag that is passed in.  It is also data model independent.
 *
 * A better name for this function would be "elect" active DNS servers and
 * ifName, as in, given a set of possible DNS servers and interfaces, choose
 * the "active"/"system default" DNS servers.
 *
 * @param ipvx  (IN) If CMS_AF_SELECT_IPV4, this func operates in IPv4 only.
 *                   If CMS_AF_SELECT_IPV6, this func operates in IPv6 only.
 *                   No other values are allowed.
 * @param dnsIfNameList    (IN) list of Linux intf names to check, with
 *                              most preferred intf name first.
 * @param staticDnsServers (IN) pre-configured static DNS IP address(es)
 * @param activeDnsIfName (OUT) the WAN ifname which is UP.  If static DNS
 *                 servers are used, will contain CMS_FAKE_STATIC_DNS_IFNAME.
 *                 If none found, then it will be an empty string.  Caller
 *                 must provided a buffer of at least CMS_IFNAME_LENGTH bytes.
 * @param activeDnsServers (OUT) active dns ip.  Can default "0.0.0.0,0.0.0.0".
 *                 The caller must pass in a buffer that is at least
 *                 (CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH) bytes long.
 *
 * @return CmsRet  Error code are returned when there is a serious error.
 *                 If no active DNS servers are found (case 4), the return
 *                 code is still CMSRET_SUCCESS.
 */
CmsRet rutNtwk_selectActiveIpvxDnsServers(UINT32 ipvx,
                                         const char *dnsIfNameList,
                                         const char *staticDnsServers,
                                         char *activeDnsIfName,
                                         char *activeDNSServers);




/** Given an Layer 3 interface name, return a comma separate list of IPv4
 *  or IPv6 servers associated with this interface.
 *
 * @param ipvx  (IN) If CMS_AF_SELECT_IPV4, return IPv4 DNSServers
 *                   If CMS_AF_SELECT_IPV6, return IPv6 DNSServers.
 *                   If CMS_AF_SELECT_IPVX, return IPV6 or IPv4 DNSServers.
 * @param ifName     (IN)   Wan interface name
 * @param DNSServers (OUT)  IPv4 DNS ip addresses from this interface.
 *                     The caller must pass in a buffer that is at least
 *                     (CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH) bytes long.
 *
 * @return CmsRet enum
 */
CmsRet rutNtwk_getIpvxDnsServersFromIfName(UINT32 ipvx, const char *ifName, char *DNSServers);
CmsRet rutNtwk_getIpvxDnsServersFromIfName_igd(UINT32 ipvx, const char *ifName, char *DNSServers);
CmsRet rutNtwk_getIpvxDnsServersFromIfName_dev2(UINT32 ipvx, const char *ifName, char *DNSServers);

#if defined(SUPPORT_DM_LEGACY98)
#define rutNtwk_getIpvxDnsServersFromIfName(v, i, s)   rutNtwk_getIpvxDnsServersFromIfName_igd((v), (i), (s))
#elif defined(SUPPORT_DM_HYBRID)
#define rutNtwk_getIpvxDnsServersFromIfName(v, i, s)   rutNtwk_getIpvxDnsServersFromIfName_igd((v), (i), (s))
#elif defined(SUPPORT_DM_PURE181)
#define rutNtwk_getIpvxDnsServersFromIfName(v, i, s)   rutNtwk_getIpvxDnsServersFromIfName_dev2((v), (i), (s))
#elif defined(SUPPORT_DM_DETECT)
#define rutNtwk_getIpvxDnsServersFromIfName(v, i, s)   (cmsMdm_isDataModelDevice2() ? \
                           rutNtwk_getIpvxDnsServersFromIfName_dev2((v), (i), (s)) : \
                           rutNtwk_getIpvxDnsServersFromIfName_igd((v), (i), (s)))
#endif


/** Given an Layer 3 interface name, return a comma separate list of IPv4 DNS
 *  servers associated with this interface.
 *
 * @param ifName     (IN)   Wan interface name
 * @param DNSServers (OUT)  IPv4 DNS ip addresses from this interface.
 *                     The caller must pass in a buffer that is at least
 *                     (CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH) bytes long.
 *
 * @return CmsRet enum
 */
CmsRet rutNtwk_getIpv4DnsServersFromIfName(const char *ifName, char *DNSServers);
CmsRet rutNtwk_getIpv4DnsServersFromIfName_igd(const char *ifName, char *DNSServers);
CmsRet rutNtwk_getIpv4DnsServersFromIfName_dev2(const char *ifName, char *DNSServers);

#if defined(SUPPORT_DM_LEGACY98)
#define rutNtwk_getIpv4DnsServersFromIfName(i, a)   rutNtwk_getIpv4DnsServersFromIfName_igd((i), (a))
#elif defined(SUPPORT_DM_HYBRID)
#define rutNtwk_getIpv4DnsServersFromIfName(i, a)   rutNtwk_getIpv4DnsServersFromIfName_igd((i), (a))
#elif defined(SUPPORT_DM_PURE181)
#define rutNtwk_getIpv4DnsServersFromIfName(i, a)   rutNtwk_getIpv4DnsServersFromIfName_dev2((i), (a))
#elif defined(SUPPORT_DM_DETECT)
#define rutNtwk_getIpv4DnsServersFromIfName(i, a)   (cmsMdm_isDataModelDevice2() ? \
                           rutNtwk_getIpv4DnsServersFromIfName_dev2((i), (a)) : \
                           rutNtwk_getIpv4DnsServersFromIfName_igd((i), (a)))
#endif


/** Given an Layer 3 interface name, return a comma separate list of IPv6 DNS
 *  servers associated with this interface.
 *
 * @param ifName     (IN)   Wan interface name
 * @param DNSServers (OUT)  IPv6 DNS ip addresses from this interface.
 *                    The caller must pass in a buffer that is at least
 *                    (CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH) bytes long.
 *
 * @return CmsRet enum
 */
CmsRet rutNtwk_getIpv6DnsServersFromIfName(const char *ifName, char *DNSServers);
CmsRet rutNtwk_getIpv6DnsServersFromIfName_igd(const char *ifName, char *DNSServers);
CmsRet rutNtwk_getIpv6DnsServersFromIfName_dev2(const char *ifName, char *DNSServers);

#if defined(SUPPORT_DM_LEGACY98)
#define rutNtwk_getIpv6DnsServersFromIfName(i, a)   rutNtwk_getIpv6DnsServersFromIfName_igd((i), (a))
#elif defined(SUPPORT_DM_HYBRID)
#define rutNtwk_getIpv6DnsServersFromIfName(i, a)   rutNtwk_getIpv6DnsServersFromIfName_dev2((i), (a))
#elif defined(SUPPORT_DM_PURE181)
#define rutNtwk_getIpv6DnsServersFromIfName(i, a)   rutNtwk_getIpv6DnsServersFromIfName_dev2((i), (a))
#elif defined(SUPPORT_DM_DETECT)
#define rutNtwk_getIpv6DnsServersFromIfName(i, a)   (cmsMdm_isDataModelDevice2() ? \
                           rutNtwk_getIpv6DnsServersFromIfName_dev2((i), (a)) : \
                           rutNtwk_getIpv6DnsServersFromIfName_dev2((i), (a)))
#endif




/** Reevaluate and set the active dns info after a WAN service connection changes
 * 
 * @param isIPv4    (IN) indication of IPv4 or IPv6
 * @return CmsRet enum.
 */
CmsRet rutNtwk_doSystemDns(UBOOL8 isIPv4);



/** Shared helper function: remove an ifName from an ifNameList
 *
 * @param ifName     (IN) ifName to remove
 * @param ifNameList (IN/OUT)  Caller provides this list of ifNames and
 *                            this function will remove the specified ifName
 *                            from the list.  Note this function modifies this
 *                            list buffer.
 */
void rutNtwk_removeIfNameFromList(const char *ifName, char *ifNameList);




/** In the WAN interface deletion, need to remove this WAN interface name from
 *  DNSIfName list.
 *
 * @param ipvx  (IN) If CMS_AF_SELECT_IPV4, delete ifName from IPv4 DNSIfNameList
 *                   If CMS_AF_SELECT_IPV6, delete ifName from IPv6 DNSIfNameList
 *                   If CMS_AF_SELECT_IPVX, delete ifName from both IPv4 and
 *                   IPV6 ifNameLists.
 * @param ifName (IN) name of Linux intf name to delete.
 *
 */
void rutNtwk_removeIpvxDnsIfNameFromList(UINT32 ipvx, const char *ifName);
void rutNtwk_removeIpvxDnsIfNameFromList_igd(UINT32 ipvx, const char *ifName);
void rutNtwk_removeIpvxDnsIfNameFromList_dev2(UINT32 ipvx, const char *ifName);

#if defined(SUPPORT_DM_LEGACY98)
#define rutNtwk_removeIpvxDnsIfNameFromList(i, v)   rutNtwk_removeIpvxDnsIfNameFromList_igd((i), (v))
#elif defined(SUPPORT_DM_HYBRID)
#define rutNtwk_removeIpvxDnsIfNameFromList(i, v)   rutNtwk_removeIpvxDnsIfNameFromList_igd((i), (v))
#elif defined(SUPPORT_DM_PURE181)
#define rutNtwk_removeIpvxDnsIfNameFromList(i, v)   rutNtwk_removeIpvxDnsIfNameFromList_dev2((i), (v))
#elif defined(SUPPORT_DM_DETECT)
#define rutNtwk_removeIpvxDnsIfNameFromList(i, v)   (cmsMdm_isDataModelDevice2() ? \
                       rutNtwk_removeIpvxDnsIfNameFromList_dev2((i), (v)) : \
                       rutNtwk_removeIpvxDnsIfNameFromList_igd((i), (v)))
#endif




/** Retore special dhcp config file (normally dns relay) to default
 * 
 * @return CmsRet enum
 */
 CmsRet rutNtwk_RestoreDefaultDhcpConfig(void);

 
/** Create a new LAN device for non DMZ LAN with default parameters
  *
  * @return CmsRet enum
  */
CmsRet rutNtwk_addNonDmzLan(void);

/** Delete LAN device for non DMZ LAN
  *
  * @return void
  */
void rutNtwk_deleteNonDmzLan(void);

/** Return the global Advnaced DMZ enable flag
  *
  * @return TRUE or FALSE
  */
UBOOL8 rutNtwk_isAdvancedDmzEnabled(void);


/** After interface is up, create a dhcp configuration file for
  *  non DMZ subnet
  * 
  * @param externIpAddress      (IN) The wan interface ip address
  *
  * @return CmsRet enum
  */

CmsRet rutNtwk_startNonDmzDhcpd(char *externIpAddress);

/** for advanced dmz with dynamic MRE, the interface
 * name need to be put in the kernel arp table for outside
 * to see
 *
 */

void rutNtwk_startAdvancedDmzArp(const char *ifcName);


/** Get the domain name of the system.
 *
 * This function is only defined if the DNSPROXY feature is compiled in.
 *
 * @param domainName  (OUT) caller passes in a pointer to char *.  This function
 *                          will allocate a buffer big enough to hold the domain name.
 *                          Caller is responsible for freeing the buffer.
 */
void rutNtwk_getDomainName(char **domainName);

#endif /* __RUT_NETWORK_H__ */      

