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
#ifndef __RUT_ROUTE_H__
#define __RUT_ROUTE_H__


/*!\file rut_route.h
 * \brief System level interface functions for routing functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"

#define POLICY_ROUTING_TABLE_FOLDER "/var/iproute2"
#define POLICY_ROUTING_TABLE_FILE "/var/iproute2/rt_tables"
#define POLICY_ROUTING_VERSION_FILE "/var/iproute2/version"
#define IPROUTE2_OLD_VERSION "iproute2-ss100224"
#define RT_TABLE_BASE 200
#define RT_TABLE_MAX  8
#define POLICY_ROUTING_PREFIX     "pr_"
#define INTERFACE_GROUP_PREFIX    "ig_"
#define RT_TABLE_GPON_IPHOST 10
#define RT_TABLE_GPON_IPV6HOST 11



/** Fetch an active default gateway from.
 * 1). the default gateway list  in the order if the WAN interface is up
 * 2). If 1). fails, try to find a routed and connected WAN in the system to be used as default gateway
 * 3). if 2). fails, set defaultGateway to NULL (no system active default gateway). 
 * 
 * @param gatewayList      (IN) the default gateway list
 * @param defaultGateway   (OUT) WAN interface name for the active default gateway. Will be NLL if 
 *                               no active default gateway found.
 *
 */
void rutRt_fetchActiveDefaultGateway(const char *gatewayList, char *defaultGateway);




/** Remove this WAN interface (ifName) in the default gateway list if it is
 *  in there.  Also will trigger re-selection of active default gateway if
 *  this deleted WAN interface is currently the active default gateway.
 * 
 * @param ifName                (IN) the WAN inteface name
 * 
 */
void rutRt_removeDefaultGatewayIfUsed(const char* ifName);
void rutRt_removeDefaultGatewayIfUsed_igd(const char* ifName);
void rutRt_removeDefaultGatewayIfUsed_dev2(const char* ifName);


#if defined(SUPPORT_DM_LEGACY98)
#define rutRt_removeDefaultGatewayIfUsed(i)  rutRt_removeDefaultGatewayIfUsed_igd((i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutRt_removeDefaultGatewayIfUsed(i)  rutRt_removeDefaultGatewayIfUsed_igd((i))
#elif defined(SUPPORT_DM_PURE181)
#define rutRt_removeDefaultGatewayIfUsed(i)  rutRt_removeDefaultGatewayIfUsed_dev2((i))
#elif defined(SUPPORT_DM_DETECT)
#define rutRt_removeDefaultGatewayIfUsed(i)  (cmsMdm_isDataModelDevice2() ? \
                           rutRt_removeDefaultGatewayIfUsed_dev2((i)) : \
                           rutRt_removeDefaultGatewayIfUsed_igd((i)))
#endif




/** Delete the current default gateway and add the specified gateway as
 * the system default gateway.  Action only.
 * 
 * @param gwIfName      (IN) The new gateway WAN interface name
 *
 * @return CmsRet
 */
CmsRet rutRt_configActiveDefaultGateway(const char *gwIfName);


/** If the specified gwIfName is the current system defalult gateway,
 * remove it.
 *
 * @param gwIfName      (IN) The gateway WAN interface name
 */
void rutRt_unconfigActiveDefaultGateway(const char *gwIfName);



/** After a wan interface is up, the system will do a fresh new default
 * gateway setting.
 *
 * @param (IN) isIPv4
 * @return CmsRet
 */
CmsRet rutRt_doSystemDefaultGateway(UBOOL8 isIPv4);


/** activate L3ForwardingEntryObject entries for routing
 *
 * @param (IN) ifName
 * @return CmsRet
 */
CmsRet rutRt_activateL3ForwardingEntry(const char* ifName);


/** Remove L3ForwardingEntryObject entries for configuration when the 
 *   interface is removed (ifName).
 *
 * @param (IN) ifName
 * @return CmsRet
 */
CmsRet rutRt_removeL3ForwardingEntry(const char* ifName);




/** check if only the interface name should be used for setting up default
 *  gateway.  If not, return FALSE and fill outGwIpAddress with ip address
 *  of inGwIfc.  Seems to be important for IPoA and dhcp client on LAN bridge.
 *
 * @param inGwIfc          (IN) the inteface name for the default gateway
 * @param outGwIpAddress   (OUT) the getway ip address for the defaultgateway
 * @return  TRUE -- use interface name only to add default gateway.
 *          FALSE --use the gateway ip address in outGwAddress
 */
UBOOL8 rutRt_useGatewayIfcNameOnly(const char *inGwIfc, char *outGwIpAddress);
UBOOL8 rutRt_useGatewayIfcNameOnly_igd(const char *inGwIfc, char *outGwIpAddress);
UBOOL8 rutRt_useGatewayIfcNameOnly_dev2(const char *inGwIfc, char *outGwIpAddress);

#if defined(SUPPORT_DM_LEGACY98)
#define rutRt_useGatewayIfcNameOnly(i, o)  rutRt_useGatewayIfcNameOnly_igd((i), (o))
#elif defined(SUPPORT_DM_HYBRID)
#define rutRt_useGatewayIfcNameOnly(i, o)  rutRt_useGatewayIfcNameOnly_igd((i), (o))
#elif defined(SUPPORT_DM_PURE181)
#define rutRt_useGatewayIfcNameOnly(i, o)  rutRt_useGatewayIfcNameOnly_dev2((i), (o))
#elif defined(SUPPORT_DM_DETECT)
#define rutRt_useGatewayIfcNameOnly(i, o)  (cmsMdm_isDataModelDevice2() ? \
                               rutRt_useGatewayIfcNameOnly_dev2((i), (o)) : \
                               rutRt_useGatewayIfcNameOnly_igd((i), (o)))
#endif



/** Return TRUE if the specified ifName is the current default gateway
 *  Looks in /proc.
 *
 * @param ifName (IN) the ifname to query
 * @return TRUE if the specified ifName is the current default gateway
 *              according to /proc
 */
UBOOL8 rutRt_isDefaultGatewayIfNameInRouteTable(const char *ifName);



/** Delete the specified ifName from the list.  This function will modify the list.
 *
 * @param ifName  (IN) ifName to delete.
 * @param list    (IN/OUT) the list to modify
 *
 */
void rutRt_removeIfNameFromList(const char *ifName, char *list);



/** Action only function for adding policy routing rules for  both policy routing or
 *  routed layer 3 interface group.  Will update the iproute2/rt_tables file
 *
 * @param fromPolicyRoute  (IN)If TRUE, this rule is from policy route, else from routed interface group
 * @param ruleName         (IN) policy route rule name
 * @param srcIfName        (IN) source interface name (br0, br1, etc)
 * @param srcIP            (IN) source Ip address. For interface group, it is not used.
 * @param gatewayIP        (IN) gateway ip address
 * @param outIfName        (IN) out interface name (WAN)
 *
 * @return CmsRet
 */
CmsRet rutRt_addPolicyRouting(UBOOL8 fromPolicyRoute, const char *ruleName, const char *srcIfName,
                            const char *srcIP, const char *gatewayIP,
                            const char *outIfName);
   

/** Action only function for deleting policy routing rules for both policy routing or
 *  routed layer 3 interface group. Will update the iproute2/rt_tables file
 *
 * @param fromPolicyRoute  (IN)If TRUE, this rule is from policy route, else from routed interface group
 * @param ruleNameToDelete (IN) rule name to be deleted
 *
 * @return CmsRet
 */
CmsRet rutRt_deletePolicyRouting(UBOOL8 fromPolicyRoute, const char *ruleNameToDelete);


/** add a static route action only.
 *
 * @param destIPAddress     (IN) destination IP address
 * @param destSubnetMask    (IN) destination subnet mask
 * @param gatewayIPAddress  (IN) default gateway IP address of this static route
 * @param interface         (IN) default interface of this static route
 * @param forwardingMetric  (IN) hop number to the destination.  Pass in an empty
 *                               string to let the kernel set the metric, otherwise,
 *                               pass in an integer, in the form of a string, to
 *                               specify the metric.
 *
 * @return CmsRet enum.
 */

CmsRet rutRt_addStaticRouteAction(const char *destIPAddress,
                                  const char *destSubnetMask,
                                  const char *gatewayIPAddress,
                                  const char *interface,
                                  SINT32 forwardingMetric);
                                  
/** delete a static route action only.
 *
 * @param destIPAddress      (IN) destination IP address
 * @param destSubnetMask     (IN) destination subnet mask
 * @param gatewayIPAddress   (IN) default gateway IP address of this static route
 * @param interface          (IN) default interface of this static route
 *
 * @return CmsRet enum.
 **/                                 
CmsRet rutRt_deleteStaticRouteAction(const char *destIPAddress,
                                     const char *destSubnetMask,
                                     const char *gatewayIPAddress,
                                     const char *interface);                                 




/** Add system static route(created by system not by user)  
 *
 * @param DNSServers    (IN) DNSServers IP addresses 
 * @param mask          (IN) destination subnet mask
 * @param gateway       (IN) default gateway IP address of this static route
 * @param ifName        (IN) WAN interface name
 * @return CmsRet
 */
CmsRet rutRt_addSystemStaticRoute(const char* DNSServers, 
                                  const char *mask, 
                                  const char *gateway, 
                                  const char *ifName);


#ifdef SUPPORT_POLICYROUTING
UBOOL8 getActionInfoFromPolicyRoutingRuleName(const char *ruleName,char *srcIfName,char *srcIP,char *gatewayIP,char *outIfName);
UBOOL8 getActionInfoFromPolicyRoutingRuleName_igd(const char *ruleName,char *srcIfName,char *srcIP,char *gatewayIP,char *outIfName);
UBOOL8 getActionInfoFromPolicyRoutingRuleName_dev2(const char *ruleName,char *srcIfName,char *srcIP,char *gatewayIP,char *outIfName);
void policyRoutingAction( const char *prefixedRuleName, const char *srcIfName, const char *srcIP, const char *gatewayIP, const char *outIfName, SINT16 idx, UBOOL8 add);

#if defined(SUPPORT_DM_LEGACY98)
#define getActionInfoFromPolicyRoutingRuleName(ruleName, srcIfName, srcIP, gatewayIP, outIfName)  getActionInfoFromPolicyRoutingRuleName_igd((ruleName),(srcIfName),(srcIP),(gatewayIP),(outIfName))
#elif defined(SUPPORT_DM_HYBRID)
#define getActionInfoFromPolicyRoutingRuleName(ruleName, srcIfName, srcIP, gatewayIP, outIfName)  getActionInfoFromPolicyRoutingRuleName_igd((ruleName),(srcIfName),(srcIP),(gatewayIP),(outIfName))
#elif defined(SUPPORT_DM_PURE181)
#define getActionInfoFromPolicyRoutingRuleName(ruleName, srcIfName, srcIP, gatewayIP, outIfName)  getActionInfoFromPolicyRoutingRuleName_dev2((ruleName),(srcIfName),(srcIP),(gatewayIP),(outIfName))
#elif defined(SUPPORT_DM_DETECT)
#define getActionInfoFromPolicyRoutingRuleName(ruleName, srcIfName, srcIP, gatewayIP, outIfName)  (cmsMdm_isDataModelDevice2() ? \
                               getActionInfoFromPolicyRoutingRuleName_dev2((ruleName),(srcIfName),(srcIP),(gatewayIP),(outIfName)) : \
                               getActionInfoFromPolicyRoutingRuleName_igd((ruleName),(srcIfName),(srcIP),(gatewayIP),(outIfName)))
#endif
#endif /* SUPPORT_POLICYROUTING */


#if defined(DMP_BRIDGING_1) || defined(DMP_DEVICE2_BRIDGE_1) /* aka SUPPORT_PORT_MAP */
UBOOL8 getActionInfoFromPortMappingRuleName(const char *ruleName,char *srcIfName,char *srcIP,char *gatewayIP,char *outIfName);
UBOOL8 getActionInfoFromPortMappingRuleName_igd(const char *ruleName,char *srcIfName,char *srcIP,char *gatewayIP,char *outIfName);
UBOOL8 getActionInfoFromPortMappingRuleName_dev2(const char *ruleName,char *srcIfName,char *srcIP,char *gatewayIP,char *outIfName);

#if defined(SUPPORT_DM_LEGACY98)
#define getActionInfoFromPortMappingRuleName(ruleName, srcIfName, srcIP, gatewayIP, outIfName)  getActionInfoFromPortMappingRuleName_igd((ruleName),(srcIfName),(srcIP),(gatewayIP),(outIfName))
#elif defined(SUPPORT_DM_HYBRID)
#define getActionInfoFromPortMappingRuleName(ruleName, srcIfName, srcIP, gatewayIP, outIfName)  getActionInfoFromPortMappingRuleName_igd((ruleName),(srcIfName),(srcIP),(gatewayIP),(outIfName))
#elif defined(SUPPORT_DM_PURE181)
#define getActionInfoFromPortMappingRuleName(ruleName, srcIfName, srcIP, gatewayIP, outIfName)  getActionInfoFromPortMappingRuleName_dev2((ruleName),(srcIfName),(srcIP),(gatewayIP),(outIfName))
#elif defined(SUPPORT_DM_DETECT)
#define getActionInfoFromPortMappingRuleName(ruleName, srcIfName, srcIP, gatewayIP, outIfName)  (cmsMdm_isDataModelDevice2() ? \
                               getActionInfoFromPortMappingRuleName_dev2((ruleName),(srcIfName),(srcIP),(gatewayIP),(outIfName)) : \
                               getActionInfoFromPortMappingRuleName_igd((ruleName),(srcIfName),(srcIP),(gatewayIP),(outIfName)))
#endif
#endif /* SUPPORT_POLICYROUTING */

#endif /* __RUT_ROUTE_H__ */

