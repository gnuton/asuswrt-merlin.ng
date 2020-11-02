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

#ifndef __RUT_IPTALBES_H__
#define __RUT_IPTALBES_H__


/*!\file rut_iptables.h
 * \brief System level interface functions for iptables functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"

#define CONNTRACK_MAX         1000

#define IP_TBL_COL_NUM     0
#define IP_TBL_COL_PKTS    1
#define IP_TBL_COL_BYTES   2
#define IP_TBL_COL_TARGET  3
#define IP_TBL_COL_PROT    4
#define IP_TBL_COL_OPT     5
#define IP_TBL_COL_IN      6
#define IP_TBL_COL_OUT     7
#define IP_TBL_COL_SRC     8
#define IP_TBL_COL_DST     9
#define IP_TBL_COL_MAX     10

#define UPNP_IP_ADDRESS    "239.255.255.250"



/** Insert the ip related modules
 *
 * @param (IN) void
 * @return void
 */
void rutIpt_insertIpModules(void);

      
/* Open WAN port to accept connection request connection from ACS
 *
 * accept a tcp packet on destination TR69_CONN_REQ_PORT from the remote
 * to allow connect request from remote ACS
 * @param ifcName (IN) This is the wan interface name string for which the NAT policies applied to
 */
void rutIpt_acceptConnReqForTr69c(char *ifcName); 


/** Insert the LAN/WAN NAT Masquerade rules for the given ifname.
 *
 * @param ifName      (IN) ifname
 * @param localSubnet (IN) localsubnet in dotted form
 * @param localSubnetMask  (IN) localsubnetmask in dotted form
 * @param isFullCone (IN) is FullCone nat enabled on this interface.
 *
 */
void rutIpt_insertNatMasquerade(const char *ifName, const char *localSubnet, const char *localSubnetMask, UBOOL8 isFullCone);



/** Delete the LAN/WAN NAT Masquerade rules for the given ifname.
 *
 * @param ifName      (IN) ifname
 * @param localSubnet (IN) localsubnet in dotted form
 * @param localSubnetMask  (IN) localsubnetmask in dotted form
 *
 */
void rutIpt_deleteNatMasquerade(const char *ifName, const char *localSubnet, const char *localSubnetMask);



/** Initialize the NAT for the wan interface (ifName) with iptables commands.
 *
 * @param ifName (IN)  The WAN connection interface name.
 * @param isFullCone  (IN)  If fullcone NAT is enabled on this connection.
 */
void rutIpt_initNat(const char *ifName, UBOOL8 isFullCone);


/** Run all NAT and firewall related commands for a WAN connection.
 *
 * This is called when a WAN connection comes up.  There is no corresponding
 * call when the wan connection goes down.  
 *
 * @param ifName (IN) The WAN connection interface name.
 * @param isFullCone  (IN)  If fullcone NAT is enabled on this connection.
 * @param isNatEnabled (IN) is NAT enabled on this connection.
 * @param isFirewallEnabled (IN) is Firewall enabled on this connection.
 *
 */
void rutIpt_initNatAndFirewallOnWanConnection(const char *ifName,
                                              UBOOL8 isFullCone,
                                              UBOOL8 isNatEnabled,
                                              UBOOL8 isFirewallEnabled);



/** Initialize the ipsec policy
 *
 * Initailize the ipsec for the wan interface with iptables commands.
 * @param ifcName       (IN) This is the wan interface name string for which the ipsec firewall policies applied to
 */
void rutIpt_initIpSecPolicy(const char *ifName);

/** Remove the ipsec policy
 *
 * Remove the ipsec for the wan interface with iptables commands.
 * @param ifcName       (IN) This is the wan interface name string for which the ipsec firewall policies applied to
 */
void rutIpt_removeIpSecPolicy(const char *ifName);

/** Setup firewall for DHCPv6
 *
 * Setup firewall rules to block or unblock DHCPv6 packets.
 * @param unblock       (IN) unblock if TRUE, otherwise block.
 * @param ifcName       (IN) This is the wan interface name string for which the firewall policies applied to
 */
void rutIpt_setupFirewallForDHCPv6(UBOOL8 unblock, const char *ifName);

void rutIpt_defaultLANSetup6(const char *ifName);

/** Initialize firewall exceptions
 *
 * Initailize firewall incoming and outgoing exceptions for the interface with iptables commands.
 * @param ifcName       (IN) This is the interface name string for which the firewall policies applied to
 */

void rutIpt_initFirewallExceptions(const char *ifName);

void rutIpt_initFirewallExceptions_igd(const char *ifName);

void rutIpt_initFirewallExceptions_dev2(const char *ifName);

#if defined(SUPPORT_DM_LEGACY98)
#define rutIpt_initFirewallExceptions(i)   rutIpt_initFirewallExceptions_igd(i)
#elif defined(SUPPORT_DM_HYBRID)
#define rutIpt_initFirewallExceptions(i)   rutIpt_initFirewallExceptions_igd(i)
#elif defined(SUPPORT_DM_PURE181)
#define rutIpt_initFirewallExceptions(i)   rutIpt_initFirewallExceptions_dev2(i)
#elif defined(SUPPORT_DM_DETECT)
#define rutIpt_initFirewallExceptions(i)   (cmsMdm_isDataModelDevice2() ? \
                                   rutIpt_initFirewallExceptions_dev2(i) : \
                                   rutIpt_initFirewallExceptions_igd(i))
#endif



/** Initialize the firewall
 *
 * Initailize the firewall for the interface with iptables commands.
 * @param domain        (IN) Communication domain, either PF_INET or PF_INET6.
 * @param ifcName       (IN) This is the interface name string for which the firewall policies applied to
 */
void rutIpt_initFirewall(SINT32 domain, const char *ifName);


/**  Remove all IP table rules attach with the given interface.
 * @param ifcName       (IN) This is the wan interface name string for which the firewall policies applied to
 * @param isIPv4          (IN) indicate if IPv4 or IPv6
 */
void rutIpt_removeInterfaceIptableRules(const char *ifcName, UBOOL8 isIPv4);

 
/**  Remove  remove ip table rules for the interface ifcName on a particular table and chain
 * @param domain        (IN) Communication domain, either PF_INET or PF_INET6.
 * @param ifcName       (IN) This is the wan interface name string 
 * @param table         (IN) the table name
 * @param chain         (IN) the chain name
 * @return        TRUE if more to be removed
 */
UBOOL8  rutIpt_removeIptableRules(SINT32 domain, const char *ifcName, const char *table, const char *chain);

/**  Execute commands to setup NAT Masquerade for PPP on demand
 * @param ifcName       (IN) This is the wan interface name string 
 */
void rutIpt_initNatForPppOnDemand(char *ifcName, UBOOL8 fullCone);


/**  execute nat commands to enable DNS forwarding
 *
 * Forwarding is from bridgeName (normally br0) to a single (primary) dns server.
 * dnsprobe will tell us if the primary dns server goes down and we
 * need to switch to a secondary one.
 * Not sure if the hard coded br0 will cause problems
 * when we have multiple LAN subnets.
 *
 * @param bridgeName  (IN)            bridgeName  - bridge name (br0, br1)
 * @param dns1 (IN)            dns1 dns ip address
 */
void rutIpt_setDnsForwarding(const char *bridgeName, const char *dns1);


/**  execute nat commands to add iptable rule to stop multicast on the wan interface (for upnp)
 * @wanIfcName  (IN)          WAN interface name
 * @addRules  (IN)            TRUE = add rule, FALSE = delete rule
 */
void rutIpt_upnpConfigStopMulticast(char *wanIfcName, UBOOL8 addRules);


/** add rip iptable rule for the WAN interface.
 *
 * @ifcName  (IN)     WAN interface name
 */
void rutIpt_ripAddIptableRule(const char *ifcName);


/** remove ALL rip IP table rules.
 */
void rutIpt_ripRemoveAllIptableRules();


/** Activate Url Filter: insmod necessary modules and add an iptables chain. 
 *
 */
void rutIpt_activeUrlFilter(void);


/** Deactivate Url Filter: Delete iptables chain and references. 
 */
void rutIpt_deactiveUrlFilter(void);


/** Configure Url filter list mode. 
 *
 * @param  type (IN)  whitelist /blacklist mode
 */
void rutIpt_configUrlFilterMode(char *type) ;


/** Configure Url filter list entry. 
 *
 * @param obj (IN)
 * @param add  (IN)
 */
void rutIpt_urlFilterConfig(void *Obj, UBOOL8 add) ;

/** Add nat iptable rules for advanced DMZ
 *
 * @param localLanIp (IN) LAN Ip address
 * @param localIp    (IN) WNA IP address served as the destination for postrout
 * @param nonDmzIp   (IN) non DMZ ip address
 * @param nonDmzMask (IN) non DMZ mask
 */
void rutIpt_addAdvancedDmzIpRules(const char *localLanIp, 
                               const char *localIp, 
                               const char *nonDmzIp, 
                               const char *nonDmzMask);

/** Del nat iptable rules for advanced DMZ
 *
 * @param localLanIp (IN) LAN Ip address
 * @param localIp    (IN) WNA IP address served as the destination for postrout
 * @param nonDmzIp   (IN) non DMZ ip address
 * @param nonDmzMask (IN) non DMZ mask
 */
void rutIpt_delAdvancedDmzIpRules(const char *localLanIp, 
                               const char *localIp, 
                               const char *nonDmzIp, 
                               const char *nonDmzMask);

/** execute iptables rules for port mapping feature
 *
 * @param ifName (IN) ifName of the WAN ppp or ip connection.
 */
void rutIpt_activatePortMappingEntries(const char *ifName);
void rutIpt_activatePortMappingEntries_igd(const char *ifName);
void rutIpt_activatePortMappingEntries_dev2(const char *ifName);

#if defined(SUPPORT_DM_LEGACY98)
#define rutIpt_activatePortMappingEntries(i)  rutIpt_activatePortMappingEntries_igd((i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutIpt_activatePortMappingEntries(i)  rutIpt_activatePortMappingEntries_igd((i))
#elif defined(SUPPORT_DM_PURE181)
#define rutIpt_activatePortMappingEntries(i)  rutIpt_activatePortMappingEntries_dev2((i))
#elif defined(SUPPORT_DM_DETECT)
#define rutIpt_activatePortMappingEntries(i)  (cmsMdm_isDataModelDevice2() ? \
                                  rutIpt_activatePortMappingEntries_dev2((i)) : \
                                  rutIpt_activatePortMappingEntries_igd((i)))
#endif





/** insert port triggering module. 
 */
void rutIpt_insertPortTriggeringModules(void);

/** iptables rules for IGMP
 *
 * @param (IN) add
 * @param (IN) ifname
 * @return void
 */
void rutIpt_igmpRules(UBOOL8 add, const char *ifname);

/** iptables rules for NAT rules for interface group
 *
 * @param ifName (IN) wan interface name 
 * @param bridgeIfName (IN) bridge interface name (br1...)

 * @return void
 */
void rutIpt_initNatForIntfGroup(const char *ifName, const char *bridgeIfName,
				UBOOL8 fullCone);

/** remove the DNAT for DNS on non default bridge (br1 up)
 *
 * @param bridgeIfName (IN)  bridge interface name
 */
void rutIpt_removeBridgeIfNameIptableRules(const char *bridgeIfName);

/** iptables rules for IP filter Out
 *
 * @param (IN) obj
 * @param (IN) ifName
 * @param (IN) add
 * @return void
 */
void rutIpt_addIpFilterOut(const IpFilterCfgObject *obj, const char *ifName, UBOOL8 add);

/** Add or delete an iptables rule for IP filter In/Firewall Exception
 *
 * @param (IN) InObj
 * @param (IN) ifName
 * @param (IN) add
 */
void rutIpt_doIpFilterIn(const void *InObj, const char *ifName, UBOOL8 add);

/** issue iptables rules redirect http and telnet ports
 *
 * @param ifName (IN)  Wan interface name
 * @param ifName (IN)  the lan (br0) ip address
 */
void rutIpt_redirectHttpTelnetPorts(const char *ifName, const char *ipAddress);

/** Insert iptables rules for TCP MSS option manipulation
 *
 * @param domain (IN)  Communication domain, either PF_INET or PF_INET6.
 * @param ifName (IN)  Wan interface name
 */
void rutIpt_insertTCPMSSRules(SINT32 domain, const char *ifName);

/** delete iptables rules for TCP MSS option manipulation
 *
 * @param domain (IN)  Communication domain, either PF_INET or PF_INET6.
 * @param ifName (IN)  Wan interface name
*/
void rutIpt_deleteTCPMSSRules(SINT32 domain, const char *ifName);


/** issue ip6tables/iptables rules for TCP MSS option manipulation for IP tunnel
 */
void rutIpt_TCPMSSforIPTunnel(const char *wanIfName, UBOOL8 is6rd);

/** load all the necessary modules to support qos.
 */
void rutIpt_qosLoadModule(void);

/** load all modules required for mcpd
 */
void rutIpt_McpdLoadModules(void);




#ifdef SUPPORT_IPV6

/**  Construct ip6table rule to allow dhcpv6 traffic (TCP/UDP port 546).
 * @param unbloc       (IN) 
 * @param ifName       (IN) This is the wan interface for which the firewall policies applied to
 */
void rutIpt_setupFirewallForDHCPv6(UBOOL8 unblock, const char *ifName);

/** Insert the IPv6 related modules
 *
 * @param (IN) void
 * @return void
 */
void rutIpt_insertIpModules6(void);

/** In Forwarding chain, trigger a chain to verify incoming source IP address according to the prefix delegation info 
 *
 * @param (IN) void
 * @return void
 */
void rutIpt_createRoutingChain6(void);

/** Configure rtchain 
 *
 * @param (IN) prefix
 * @param (IN) ifName
 * @param (IN) add
 * @return void
 */
void rutIpt_configRoutingChain6(const char *prefix, const char *ifName, UBOOL8 add);

/** iptables rules for MLD
 *
 * @param (IN) add
 * @param (IN) ifname
 * @return void
 */
void rutIpt_mldRules(UBOOL8 add, const char *ifname);
#endif

#ifdef DMP_X_ITU_ORG_GPON_1
/** iptables rules for omci ipHost ipOptions
 *
 * @param (IN) oid
 * @param (IN) options
 * @param (IN) ifname
 * @return void
 */
void rutIpt_omciIpHostRules(UINT32 oid, UINT8 options, char *ifName);
#endif


/** Creates a /proc/sys/net/ipv4/conf/xxxx/rp_filter for Anti IPSpoofing.  Required the WAN 
 * interface is up so that the directory xxxx (ppp0) is there. In TR98, firewall is always initialized after WAN interface 
 * is up but in TR181, firewall is initialized before wan interace is up and this has be called later on 
 * after the wan interface is up
 *
 * @param (IN) ifName
 */ 
void rutIpt_activateFirewallAntiIPspoofing(const char *ifName);

#if defined(SUPPORT_DM_HYBRID) || defined(SUPPORT_DM_PURE181) || defined(SUPPORT_DM_DETECT)

/** XXX Can these be moved to the DAL?  No callers in cms_core */

/** Get Dev2FirewallExceptionObject and iidstack by full path.
 *
 * @param (IN) ipIntfFullPath
 * @param (OUT) fwExObj
 * @param (OUT) iidStack
 * @return CmsRet enum.
 */
CmsRet rutIpt_GetfwExceptionbyFullPath_dev2(char *ipIntfFullPath,Dev2FirewallExceptionObject **fwExObj,InstanceIdStack *iidStack);

/** Add Dev2FirewallExceptionObject instance by full path.
 *
 * @param (IN) ipIntfFullPath
 * @return CmsRet enum.
 */
CmsRet rutIpt_AddfwExceptionforIPDevice_dev2(char *ipIntfFullPath);

/** remove Dev2FirewallExceptionObject instance by full path.
 *
 * @param (IN) ipIntfFullPath
 * @return CmsRet enum.
 */
CmsRet rutIpt_RemovefwExceptionforIPDevice_dev2(char *ipIntfFullPath);

/** remove Dev2FirewallExceptionRuleObject instance that rule taget equal "target" 
 *    and below Dev2FirewallExceptionObject  ipIntfFullPath .
 *
 * @param (IN) ipIntfFullPath
 * @param (IN) target : MDMVS_ACCEPT or MDMVS_DROP
 * @return CmsRet enum.
 */
CmsRet rutIpt_RemovefwExceptionRule_dev2(char *ipIntfFullPath,char *target);

#endif  /* DM_HYBRID || DM_PURE181 || DM_DETECT */




/** Add or delete firewall exception rule into kernel.
 *
 */
void rutIpt_doFirewallExceptionRule_dev2(const _Dev2FirewallExceptionRuleObject *InObj,
                                         const char *ifName, UBOOL8 add);


/** Return TRUE if the given firewall exception object is valid.
 *
 */
UBOOL8 rutIpt_isFirewallExceptionValid_dev2(const Dev2FirewallExceptionRuleObject *commonExceptionObj);


/** Return TRUE if the two firewall exception objects are the same.
 *
 */
UBOOL8 rutIpt_isFirewallExceptionSame_dev2(const Dev2FirewallExceptionRuleObject *obj1,
                                const Dev2FirewallExceptionRuleObject *obj2);


/** Check for duplicate firewall exception rule
 *
 */
UBOOL8 rutIpt_isDuplicateFirewallException_dev2(const Dev2FirewallExceptionRuleObject *obj,
                                             const InstanceIdStack *iidStack);


#if defined(DMP_BRIDGING_1) || defined(DMP_DEVICE2_BRIDGE_1) /* aka SUPPORT_PORT_MAP */
/** Add/Delete the rule which disassociate other bridge interface(!localSubnet) from the given wan interface IP table FORWARD rules.
 * @action                  (IN) 'A'/'D': Add/Delete.
 * @param domain            (IN) Communication domain, either PF_INET or PF_INET6.
 * @param wanIfcName        (IN) This is the wan interface name string for which the firewall policies applied to
 * @param localSubnet       (IN) This is the Subnetaddr of the bridge on which the wan interface FORWARD rules ACCEPT, but DROP other bridges.
 * @param localSubnetmask   (IN) This is the Subnetmask of the bridge on which the wan interface FORWARD rules ACCEPT, but DROP other bridges.
 */
 void rutIpt_disassociateOtherBridgesFromWanIntf(char action, SINT32 domain, const char *wanIfcName, const char *localSubnet, const char *localSubnetmask);
#endif /* DMP_BRIDGING_1   aka SUPPORT_PORT_MAP */


#ifdef DMP_DEVICE2_BRIDGE_1 /* aka SUPPORT_PMAP */
/** Add or delete NAT rule for interface group
 * @isAdd                  (IN) '1'/'0': Add/Delete.
 * @param ifName           (IN) Communication domain, either PF_INET or PF_INET6.
 * @param bridgeIfName     (IN) This is the wan interface name string for which the firewall policies applied to
 * @param isFullCone       (IN) This is the Subnetaddr of the bridge on which the wan interface FORWARD rules ACCEPT, but DROP other bridges.
 */

void rutIpt_configNatForIntfGroup_dev2(UBOOL8 isAdd, const char *ifName, const char *bridgeIfName, UBOOL8 isFullCone);
#endif


void rutIpt_BeepSecurityModule(void);
void rutIpt_BeepPortMappingModule(void);

void insertModuleByName(char *moduleName);

#ifdef DMP_DEVICE2_ETHLAG_1
CmsRet insertEthBondingModule(void);
#endif /* DMP_DEVICE2_ETHLAG_1 */

#endif /* __RUT_IPTALBES_H__ */

