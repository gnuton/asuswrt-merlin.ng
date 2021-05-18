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
#ifndef __RUT_WAN6_H__
#define __RUT_WAN6_H__

/*!\file rut_wan6.h
 * \brief System level interface functions for IPv6 WAN functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */

#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut_iptables.h"
#include "rut_multicast.h"

#include "rut_ipconcfg.h"
#include "rut_pppconcfg.h"
#include "rut_iptunnel.h"

#ifdef DMP_X_BROADCOM_COM_IPV6_1
#define RESET_IPV6_CONNSTATUS(p) \
   do { \
      if ((p) != NULL)  \
         {REPLACE_STRING_IF_NOT_EQUAL_FLAGS((p)->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_UNCONFIGURED, mdmLibCtx.allocFlags);}   \
   } while (0)

#else
#define RESET_IPV6_CONNSTATUS(p)  
#endif


 /** This function checks if the IPv6 connStatus changes 
 *
 * @param *newObj            (IN) the new object
 * @param *currObj            (IN) the old object
 * @param isIpObj              (IN) indicate if it's IpConnObj or PppConnObj
 *
 * @return UBOOL8.
 */
UBOOL8 rutWan_isIpv6ConnStatusChanged( const void *newObj,  const void *currObj, UBOOL8 isIpObj);


 /** This function checks if the IPv4 connStatus changes 
 *
 * @param *newObj            (IN) the new object
 * @param *currObj            (IN) the old object
 * @param isIpObj              (IN) indicate if IpConnObj or PppConnObj
 *
 * @return UBOOL8.
 */
UBOOL8 rutWan_isIpv4ConnStatusChanged( const void *newObj,  const void *currObj, UBOOL8 isIpObj);


/** Stop the services of an IPv6 wan interface
 *
 * @param *pObj    (IN) the wan connection object
 * @param isIpObj  (IN) indicate if WanIpObj or WanPppObj
 *
 * @return CmsRet enum.
 *
 */
CmsRet rutCfg_tearDownWanCon6(const void *pObj, UBOOL8 isIpObj);


/** Stop the IPv6 wan connection
 *
 * @param *pObj    (IN) the wan connection object
 * @param isIpObj  (IN) indicate if WanIpObj or WanPppObj
 *
 * @return CmsRet enum.
 *
 */
CmsRet rutCfg_stopWanCon6(const void *pObj, UBOOL8 isIpObj);


#ifdef BRCM_PKTCBL_SUPPORT
/** Get the voice bound interface string
 *
 * @param ifName (OUT) Pointer to interface string buffer
 * @param len    (IN)  Size of the buffer
 *
 * @return None
 */
void rutWan_getVoiceBoundIfName_dev2(char *ifName, int len);

#if defined(SUPPORT_DM_LEGACY98)
#elif defined(SUPPORT_DM_HYBRID)
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_getVoiceBoundIfName(i, l)    rutWan_getVoiceBoundIfName_dev2(i, l)
#elif defined(SUPPORT_DM_DETECT)
#endif /* SUPPORT_DM_LEGACY98 */

#endif /* BRCM_PKTCBL_SUPPORT */


#ifdef SUPPORT_IPV6
/** Restart a dhcpv6 client on a WAN interface.
 * 
 * @param ifName (IN) Name of interface to start the dhcpv6 client on.
 * @param dynamicIpEnabled (IN) Boolean indicating whether wan ip address is dynamically assigned.
 * @param pdEnabled (IN) Boolean indicating whether prefix delegation should be requested.
 * @param aftrName (IN) Boolean indicating whether aftr name should be requested.
 * @param mapt     (IN) Boolean indicating whether MAP-T should be requested.
 * @param mape     (IN) Boolean indicating whether MAP-E should be requested.
 * 
 * @return pid of the dhcp6c process that was started, or CMS_INVALID_PID on error.
 */
UINT32 rutWan_restartDhcp6c(const char *ifName, UBOOL8 dynamicIpEnabled, 
                            UBOOL8 pdEnabled, UBOOL8 aftrName, UBOOL8 mapt, UBOOL8 mape);


/** Stop a dhcpv6 client on a WAN interface.
 * 
 * @param ifName (IN) Name of interface to start the dhcpv6 client on.
 * @param pid (IN) pid of any previous dhcp6c started on this interface.
 * 
 * @return CmsRet enum.
 */
CmsRet rutWan_stopDhcp6c(const char *ifName, UINT32 pid);


/** Set the ipv6 address of a WAN interface.
 * 
 * @param newAddr (IN) The new address to be set.
 * @param newIfName (IN) The interface name of the new address.
 * @param oldAddr (IN) The old address to be removed.
 * @param newIfName (IN) The interface name of the old address.
 * 
 * @return CmsRet enum.
 */
CmsRet rutWan_setIPv6Address(const char *newAddr, const char *newIfName,
                             const char *oldAddr, const char *oldIfName);


/** Add prefix delegation routing
 * 
 * @param wanPrefix (IN)
 * @param add       (IN)
 * 
 * @return CmsRet enum.
 */
CmsRet rutWan_configPDRoute(const char *wanPrefix, UBOOL8 add);


/** Add or delete IPv6 default gateway.
 *
 * @param op         (IN) Either "add" or "del"
 * @param gwIpAddr   (IN) The gateway IP Addr
 * @param gwIntfName (IN) The outgoing WAN interface
 */
void rutWan_configIpv6DefaultGateway(const char *op,
                                     const char *gwIpAddr,
                                     const char *gwIntfName);

#endif


#ifdef DMP_X_BROADCOM_COM_IPV6_1
/** Add delegated address
 * 
 * @param srvName   (IN) Name of WAN interface or tunnel
 * @param ipv6str      (IN) IPv6 address to be assigned
 * @param lanIntf      (IN) Name of LAN interface to be assigned
 * @param Mode        (IN) Specify this is from WAN or tunnel
 * 
 * @return CmsRet enum.
 */
CmsRet rutWan_addDelegatedAddrEntry(const char *srvName, const char *ipv6str, const char *lanIntf, const char * mode);


/** Delete delegated address if the service is down
 * 
 * @param ServiceName (IN) Name of WAN interface or tunnel
 * @param Mode            (IN) Specify this is from WAN or tunnel
 * 
 * @return CmsRet enum.
 */
CmsRet rutWan_deleteDelegatedAddrEntry(const char *srvName, const char * mode);


/** Activate an IPv6 static route entry associated with a WAN interface or its external address.
 *
 * @param ifcName     (IN) WAN interface name
 * @param ifcAddr     (IN) External IPv6 address of the WAN interface
 *
 * @return CmsRet enum.
 **/
CmsRet rutWan_activateIPv6StaticRoute(const char *ifcName, const char *ifcAddr);


/** Get the ipv6 dns servers acquired from the wan connection.
 * 
 * @param wanDnsConn (IN) broadcom interface name of the WAN connection
 * @param dnsServer (OUT) The dns servers acquired from wan.
 * @param domainName(OUT) The domain name acquired from wan.
 * 
 * @return CmsRet enum.
 */
CmsRet rutWan_getDns6Server(const char *wanDnsConn, char **dnsServer, char **domainName);


/** Get the ipv6 site prefix delegated from the wan connection.
 * 
 * @param wanPdConn (IN) broadcom interface name of the WAN connection
 * @param sitePrefix (OUT) The site prefix delegated from wan.
 * @param sitePrefixOld (OUT) The old site prefix to be aged out once wan renumbered.
 * @param pltime (OUT) The prefered lifetime of the site prefix delegated from wan.
 * @param vltime (OUT) The valid lifetime of the site prefix delegated from wan.
 * @param vltimeOld (OUT) The valid lifetime of the old site prefix.
 * 
 * @return CmsRet enum.
 */
CmsRet rutWan_getDelegatedPrefix(const char *wanPdConn, char **sitePrefix, char **sitePrefixOld, 
                                                                  SINT32 *pltime, SINT32 *vltime, SINT32 *vltimeOld);


/** Add prefix delegation info
 * 
 * @param srvName   (IN) Name of WAN interface or tunnel
 * @param wanPrefix  (IN) Prefix info to be delegated
 * @param wanPrefixOld (IN) Prefix info to be aged out
 * @param pltime       (IN) Prefix info to be delegated
 * @param vltime       (IN) Prefix info to be delegated
 * @param vltimeOld    (IN) Prefix info to be aged out
 * @param mode        (IN) Specify this is from WAN or tunnel
 * 
 * @return CmsRet enum.
 */
CmsRet rutWan_addPDEntry(const char *srvName, const char *wanPrefix, const char *wanPrefixOld, SINT32 pltime, SINT32 vltime, SINT32 vltimeOld, const char * mode);


/** Delete prefix delegation info if the service is down
 * 
 * @param ServiceName (IN) Name of WAN interface or tunnel
 * @param Mode            (IN) Specify this is from WAN or tunnel
 * 
 * @return CmsRet enum.
 */
CmsRet rutWan_deletePDEntry(const char *srvName, const char * mode);


/** Action to configure/unconfigure wan connection as the system default gateway.
 * 
 * @param select     (IN) if ifname is selected as default gateway.
 * @param ifname   (IN) broadcom interface name.
 * 
 * @return CmsRet enum.
 */
CmsRet rutWan_configIPv6DfltGateway(UBOOL8 select, const char *ifname);


/** Handle all actions while IPv6ConnStatus in WanIpConnObj changes.
 * 
 * @param newObj  (IN) 
 * @param currObj  (IN) 
 * @param iidStack  (IN) iidStack of the WanIpConnObject.
 * 
 * @return CmsRet enum.
 */
CmsRet rutWan_ipv6IpConnProcess(_WanIpConnObject *newObj, 
                                const _WanIpConnObject *currObj, 
                                                                    const InstanceIdStack *iidStack);


/** Handle all actions while IPv6ConnStatus in WanPppConnObj changes.
 * 
 * @param newObj  (IN) 
 * @param currObj  (IN) 
 * @param iidStack  (IN) iidStack of the WanPppConnObject.
 * 
 * @return CmsRet enum.
 */
CmsRet rutWan_ipv6PppConnProcess(_WanPppConnObject *newObj, 
                                 const _WanPppConnObject *currObj, 
                                                                       const InstanceIdStack *iidStack);


/** Start the IPv6 wan connection
 *
 * @param iidStack     (IN) iidStack of the WanIpConnObject. 
 * @param *newObj    (IN) 
 *
 * @return CmsRet enum.
 */
CmsRet rutCfg_startWanIpConnection6(const InstanceIdStack *iidStack,
                                                                         _WanIpConnObject *newObj);


/** Stop the IPv6 wan connection
 *
 * @param dhcp6cPid      (IN) pid of dhcp6c on the interface
 * @param pdEnabled     (IN) indicate if prefix delegation on
 * @param *intfName     (IN) broadcom ifName of the interface
 *
 * @return CmsRet enum.
 *
 */
CmsRet rutCfg_stopWanConnection6(UINT32 dhcp6cPid, UBOOL8 pdEnabled, 
                                                                      const char *intfName);


/** Start the ppp wan connection
 *
 * @param iidStack    (IN) iidStack of the WanPppConnObject.
 * @param *newObj   (IN) 
 *
 * @return CmsRet enum.
 *
 */
CmsRet rutCfg_startWanPppConnection6(const InstanceIdStack *iidStack,
                                   _WanPppConnObject *newObj);


/** After dhcp6c gets the external address/dns/prefix delegation info, setup
 * the services such as dhcp6s, radvd, mld, gateway, dns, and etc..
 *
 * @param *pObj   (IN) a pointer to _WanPppConnObject/_WanIpConnObject
 * @UBOOL8 isIpObj (IN)  is it a IpConnObject
 * @return CmsRet enum.
 */
CmsRet rutCfg_setupWanConnection6(const void *pObj __attribute__((unused)),
								  UBOOL8 isIpObj __attribute__((unused)));


/** The actions to disable/delete an IPv6 IPoE connection
 *
 * @param  *newObj             (IN) 
 * @param  *currObj             (IN) 
 * @param  iidStack              (IN) iidStack of the WanIpConnObject.
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_ipv6IpConnDisable(_WanIpConnObject *newObj, const _WanIpConnObject *currObj, const InstanceIdStack *iidStack);


/** The actions to disable/delete an IPv6 PPPoE connection
 *
 * @param  *newObj             (IN) 
 * @param  *currObj             (IN) 
 * @param  iidStack              (IN) iidStack of the WanPppConnObject.
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_ipv6PppConnDisable(_WanPppConnObject *newObj, const _WanPppConnObject *currObj, const InstanceIdStack *iidStack);


/** The actions to delete the corresponding default route while receiving zero lifetime RA
 *
 * @param  *gateway            (IN) 
 * @param  *ifName             (IN) 
 *
 */
void rutWan_removeZeroLifeGtwy6(const char *gateway, const char *ifName);


/** The actions to delete the corresponding ra route while receiving zero lifetime RA or Lflag=0
 *
 * @param  *wanPrefix            (IN) 
 * @param  *ifName             (IN) 
 *
 */
void rutWan_updateRaRoute(const char *wanPrefix, const char *ifName);

#endif


/** Check if any default gateway exists
 *
 * @return UBOOL8
 */
UBOOL8 rutWan_isDfltGtwyExist(void);



#endif// __RUT_WAN6_H__
