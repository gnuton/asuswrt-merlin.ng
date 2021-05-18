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
#ifdef DMP_DEVICE2_BASELINE_1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "rut2_util.h"

void rutUtil_modifyNumInterfaceStack(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DEVICE,
                                 MDMOID_DEV2_INTERFACE_STACK, iidStack, delta);
}

#ifdef DMP_DEVICE2_ATMLINK_1
void rutUtil_modifyNumAtmLink(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_ATM, MDMOID_DEV2_ATM_LINK, iidStack, delta);
}
#endif

#ifdef DMP_DEVICE2_PTMLINK_1 
void rutUtil_modifyNumPtmLink(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_PTM, MDMOID_DEV2_PTM_LINK, iidStack, delta);
}
#endif

#ifdef DMP_DEVICE2_DSL_1
void rutUtil_modifyNumDslLine(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DSL, MDMOID_DEV2_DSL_LINE, iidStack, delta);
}
#endif

#ifdef DMP_DEVICE2_DSL_1
void rutUtil_modifyNumDslChannel(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DSL, MDMOID_DEV2_DSL_CHANNEL, iidStack, delta);
}
#endif

#ifdef DMP_DEVICE2_BONDEDDSL_1
void rutUtil_modifyNumDslBondingGroup(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DSL, MDMOID_DEV2_DSL_BONDING_GROUP, iidStack, delta);
}
#endif

#ifdef DMP_DEVICE2_FAST_1
void rutUtil_modifyNumFastLine(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_FAST, MDMOID_DEV2_FAST_LINE, iidStack, delta);
}
#endif

#ifdef DMP_DEVICE2_ETHERNETINTERFACE_1
void rutUtil_modifyNumEthInterface(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_ETHERNET,
                             MDMOID_DEV2_ETHERNET_INTERFACE, iidStack, delta);
}
#endif

#ifdef DMP_DEVICE2_ETHERNETLINK_1
void rutUtil_modifyNumEthernetLink(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_ETHERNET,
                                 MDMOID_DEV2_ETHERNET_LINK, iidStack, delta);
}
#endif

#ifdef DMP_DEVICE2_BRIDGE_1
void rutUtil_modifyNumBridge(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_BRIDGING,
                                 MDMOID_DEV2_BRIDGE, iidStack, delta);
}

void rutUtil_modifyNumBridgeFilter(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_BRIDGING,
                                 MDMOID_DEV2_BRIDGE_FILTER, iidStack, delta);
}

void rutUtil_modifyNumBridgePort(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_BRIDGE,
                                 MDMOID_DEV2_BRIDGE_PORT, iidStack, delta);
}

void rutUtil_modifyNumBridgeVlan(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_BRIDGE,
                                 MDMOID_DEV2_BRIDGE_VLAN, iidStack, delta);
}

void rutUtil_modifyNumBridgeVlanPort(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_BRIDGE,
                                 MDMOID_DEV2_BRIDGE_VLAN_PORT, iidStack, delta);
}
#endif /* DMP_DEVICE2_BRIDGE_1 */

void rutUtil_modifyNumIpInterface(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IP,
                                 MDMOID_DEV2_IP_INTERFACE, iidStack, delta);
}

void rutUtil_modifyNumIpv4Address(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IP_INTERFACE,
                                 MDMOID_DEV2_IPV4_ADDRESS, iidStack, delta);
}

void rutUtil_modifyNumDhcpv4ServerPool(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DHCPV4_SERVER,
                                 MDMOID_DEV2_DHCPV4_SERVER_POOL,
                                 iidStack,
                                 delta);
}

void rutUtil_modifyNumDhcpv4ServerPoolStaticAddress(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DHCPV4_SERVER_POOL,
                                 MDMOID_DEV2_DHCPV4_SERVER_POOL_STATIC_ADDRESS,
                                 iidStack, delta);
}

#ifdef DMP_DEVICE2_DHCPV4SERVERCLIENTINFO_1
void rutUtil_modifyNumDhcpv4ServerPoolClient(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DHCPV4_SERVER_POOL,
                                 MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT,
                                 iidStack,
                                 delta);
}

void rutUtil_modifyNumDhcpv4ServerPoolClientIPv4Address(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT,
                                 MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT_I_PV4_ADDRESS,
                                 iidStack,
                                 delta);
}

void rutUtil_modifyNumDhcpv4ServerPoolClientOption(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT,
                                 MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT_OPTION,
                                 iidStack,
                                 delta);
}
#endif   /* DMP_DEVICE2_DHCPV4SERVERCLIENTINFO_1 */

#ifdef DMP_DEVICE2_DHCPV4CLIENT_1
void rutUtil_modifyNumDhcpv4Client(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DHCPV4,
                                 MDMOID_DEV2_DHCPV4_CLIENT,
                                 iidStack,
                                 delta);
}
void rutUtil_modifyNumDhcpv4ClientSentOption(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DHCPV4_CLIENT,
                                 MDMOID_DEV2_DHCPV4_CLIENT_SENT_OPTION,
                                 iidStack,
                                 delta);
}
void rutUtil_modifyNumDhcpv4ClientReqOption(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DHCPV4_CLIENT,
                                 MDMOID_DEV2_DHCPV4_CLIENT_REQ_OPTION,
                                 iidStack,
                                 delta);
}
#endif  /* DMP_DEVICE2_DHCPV4CLIENT_1 */

#ifdef DMP_DEVICE2_HOSTS_2
void rutUtil_modifyNumHost(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_HOSTS,
                                 MDMOID_DEV2_HOST,
                                 iidStack, delta);
}

void rutUtil_modifyNumHostIPv4Address(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_HOST,
                                 MDMOID_DEV2_HOST_IPV4_ADDRESS,
                                 iidStack, delta);
}

void rutUtil_modifyNumHostIPv6Address(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_HOST,
                                 MDMOID_DEV2_HOST_IPV6_ADDRESS,
                                 iidStack, delta);
}
#endif  /* DMP_DEVICE2_HOSTS_2 */


void modifyNumRouterIpv4ForwardingEntry(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_ROUTER,
                        MDMOID_DEV2_IPV4_FORWARDING,
                        iidStack,
                        delta);
}

void modifyNumRouterIpv6ForwardingEntry(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_ROUTER,
                        MDMOID_DEV2_IPV6_FORWARDING,
                        iidStack,
                        delta);
}

#if SUPPORT_RIP
void modifyNumRipIntfSettingEntry(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_RIP,
                        MDMOID_DEV2_RIP_INTF_SETTING,
                        iidStack,
                        delta);
}
#endif

#ifdef DMP_DEVICE2_IPV6ROUTING_1
void modifyNumRouteInfoIntfSettingEntry(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_ROUTE_INFO,
                        MDMOID_DEV2_ROUTE_INFO_INTF_SETTING,
                        iidStack,
                        delta);
}
#endif

#ifdef DMP_DEVICE2_NEIGHBORDISCOVERY_1
void modifyNumNDInfoIntfSettingEntry(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_NEIGHBOR_DISCOVERY,
                        MDMOID_DEV2_NEIGHBOR_DISCOVERY_INTERFACE_SETTING,
                        iidStack,
                        delta);
}
#endif


#ifdef DMP_DEVICE2_DEVICEASSOCIATION_1
void rut_modifyNumManageableDevices_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_MANAGEMENT_SERVER,
                                 MDMOID_DEV2_MANAGEMENT_SERVER_MANAGEABLE_DEVICE,
                                 iidStack, delta);
}
#endif  /* DMP_DEVICE2_DEVICEASSOCIATION_1 */

#ifdef DMP_DEVICE2_PERIODICSTATSBASE_1
void rutUtil_modifyNumSampleSets_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_PERIODIC_STAT,
                        MDMOID_DEV2_SAMPLE_SET,
                        iidStack,
                        delta);
}

void rutUtil_modifyNumParameters_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_SAMPLE_SET,
                        MDMOID_DEV2_SAMPLE_PARAMETER,
                        iidStack,
                        delta);
}
#endif /* DMP_DEVICE2_PERIODICSTATSBASE_1 */


void rutUtil_modifyNumPppInterface(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_PPP,
                                 MDMOID_DEV2_PPP_INTERFACE, iidStack, delta);
}


#ifdef DMP_DEVICE2_USBHOSTSBASIC_1
void rutUtil_modifyNumUsbDevice(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_USB_HOST,
                                 MDMOID_DEV2_USB_HOST_DEVICE, iidStack, delta);
}

#ifdef DMP_DEVICE2_USBHOSTSADV_1
void rutUtil_modifyNumUsbDeviceConfig(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_USB_HOST_DEVICE,
                                 MDMOID_DEV2_USB_HOST_DEVICE_CONFIG, iidStack, delta);
}

void rutUtil_modifyNumUsbDeviceConfigIfc(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_USB_HOST_DEVICE_CONFIG,
                                 MDMOID_DEV2_USB_HOST_DEVICE_CONFIG_IFC, iidStack, delta);
}
#endif
#endif

void rutUtil_modifyNumGeneric_dev2(MdmObjectId ancestorOid,
                                   MdmObjectId decendentOid,
                                   const InstanceIdStack *iidStack,
                                   SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   void *mdmObj=NULL;
   UINT32 *num;
   CmsRet ret;

   if (mdmShmCtx->inMdmInit)
   {
      /*
       * During system startup, we might have loaded from a config file.
       * The config file already contains the correct count of these objects.
       * So don't update the count in the parent object.
       */
      cmsLog_debug("don't update count in oid %d for new object oid %d (delta=%d)",
                   ancestorOid, decendentOid, delta);
      return;
   }

   ret = cmsObj_getAncestor(ancestorOid, decendentOid, &ancestorIidStack, (void **) &mdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   ancestorOid, decendentOid,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   switch (decendentOid)
   {
   case MDMOID_DEV2_INTERFACE_STACK:
      num = &(((_Dev2DeviceObject*) mdmObj)->interfaceStackNumberOfEntries);
      break;

#ifdef DMP_DEVICE2_ATMLINK_1
   case MDMOID_DEV2_ATM_LINK:
      num = &(((_Dev2AtmObject*) mdmObj)->linkNumberOfEntries);
      break;
#endif
#ifdef DMP_DEVICE2_PTMLINK_1
   case MDMOID_DEV2_PTM_LINK:
      num = &(((_Dev2PtmObject*) mdmObj)->linkNumberOfEntries);
      break;
#endif
#ifdef DMP_DEVICE2_DSL_1
   case MDMOID_DEV2_DSL_LINE:
      num = &(((_Dev2DslObject*) mdmObj)->lineNumberOfEntries);
      break;

   case MDMOID_DEV2_DSL_CHANNEL:
      num = &(((_Dev2DslObject*) mdmObj)->channelNumberOfEntries);
      break;
#endif /* DMP_DEVICE2_DSL_1 */
#ifdef DMP_DEVICE2_BONDEDDSL_1
   case MDMOID_DEV2_DSL_BONDING_GROUP:
      num = &(((_Dev2DslObject*) mdmObj)->bondingGroupNumberOfEntries);
      break;
#endif
#ifdef DMP_DEVICE2_ETHERNETINTERFACE_1
   case MDMOID_DEV2_ETHERNET_INTERFACE:
      num = &(((_Dev2EthernetObject*) mdmObj)->interfaceNumberOfEntries);
      break;
#endif

#ifdef DMP_DEVICE2_ETHERNETLINK_1
   case MDMOID_DEV2_ETHERNET_LINK:
      num = &(((_Dev2EthernetObject*) mdmObj)->linkNumberOfEntries);
      break;
#endif

#ifdef DMP_DEVICE2_VLANTERMINATION_1
   case MDMOID_DEV2_VLAN_TERMINATION:
      num = &(((_Dev2EthernetObject *) mdmObj)->VLANTerminationNumberOfEntries);
      break;
#endif

#ifdef DMP_DEVICE2_BRIDGE_1
   case MDMOID_DEV2_BRIDGE:
      num = &(((_Dev2BridgingObject*) mdmObj)->bridgeNumberOfEntries);
      break;

#ifdef DMP_DEVICE2_BRIDGEFILTER_1
   case MDMOID_DEV2_BRIDGE_FILTER:
      num = &(((_Dev2BridgingObject*) mdmObj)->filterNumberOfEntries);
      break;
#endif

   case MDMOID_DEV2_BRIDGE_PORT:
      num = &(((_Dev2BridgeObject*) mdmObj)->portNumberOfEntries);
      break;

#ifdef DMP_DEVICE2_VLANBRIDGE_1
   case MDMOID_DEV2_BRIDGE_VLAN:
      num = &(((_Dev2BridgeObject*) mdmObj)->VLANNumberOfEntries);
      break;

   case MDMOID_DEV2_BRIDGE_VLAN_PORT:
      num = &(((_Dev2BridgeObject*) mdmObj)->VLANPortNumberOfEntries);
      break;
#endif /* DMP_DEVICE2_VLANBRIDGE_1 */
#endif /* DMP_DEVICE2_BRIDGE_1 */

   case MDMOID_DEV2_IP_INTERFACE:
      num = &(((_Dev2IpObject*) mdmObj)->interfaceNumberOfEntries);
      break;

   case MDMOID_DEV2_IPV4_ADDRESS:
      num = &(((_Dev2IpInterfaceObject*) mdmObj)->IPv4AddressNumberOfEntries);
      break;

   case MDMOID_DEV2_IPV4_FORWARDING:
      num = &(((_Dev2RouterObject*) mdmObj)->IPv4ForwardingNumberOfEntries);
      break;      

#ifdef DMP_DEVICE2_IPV6ROUTING_1
   case MDMOID_DEV2_IPV6_FORWARDING:
      num = &(((_Dev2RouterObject*) mdmObj)->IPv6ForwardingNumberOfEntries);
      break;
#endif

#ifdef SUPPORT_RIP
   case MDMOID_DEV2_RIP_INTF_SETTING:
      num = &(((Dev2RipObject*) mdmObj)->interfaceSettingNumberOfEntries);
      break;
#endif

#ifdef DMP_DEVICE2_IPV6ROUTING_1
   case MDMOID_DEV2_ROUTE_INFO_INTF_SETTING:
      num = &(((Dev2RouteInfoObject*) mdmObj)->interfaceSettingNumberOfEntries);
      break;
#endif

#ifdef DMP_DEVICE2_NEIGHBORDISCOVERY_1
   case MDMOID_DEV2_NEIGHBOR_DISCOVERY_INTERFACE_SETTING:
      num = &(((Dev2NeighborDiscoveryObject*) mdmObj)->interfaceSettingNumberOfEntries);
      break;
#endif  /* DMP_DEVICE2_NEIGHBORDISCOVERY_1 */

#ifdef DMP_DEVICE2_DHCPV4RELAY_1
   case  MDMOID_DEV2_DHCPV4_RELAY_FORWARDING:
      num = &(((Dev2Dhcpv4RelayObject*) mdmObj)->forwardingNumberOfEntries);
      break;
#endif

   case MDMOID_DEV2_DHCPV4_SERVER_POOL:
      num = &(((_Dev2Dhcpv4ServerObject*) mdmObj)->poolNumberOfEntries);
      break;
      
   case MDMOID_DEV2_DHCPV4_SERVER_POOL_STATIC_ADDRESS:
      num = &(((_Dev2Dhcpv4ServerPoolObject*) mdmObj)->staticAddressNumberOfEntries);
      break;

#ifdef DMP_DEVICE2_DHCPV4SERVERCLIENTINFO_1
   case MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT:
      num = &(((_Dev2Dhcpv4ServerPoolObject*) mdmObj)->clientNumberOfEntries);
      break;
      
   case MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT_I_PV4_ADDRESS:
      num = &(((_Dev2Dhcpv4ServerPoolClientObject*) mdmObj)->IPv4AddressNumberOfEntries);
      break;
      
   case MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT_OPTION:
      num = &(((_Dev2Dhcpv4ServerPoolClientObject*) mdmObj)->optionNumberOfEntries);
      break;
#endif    /* DMP_DEVICE2_DHCPV4SERVERCLIENTINFO_1 */

   case MDMOID_DEV2_PPP_INTERFACE:
      num = &(((_Dev2PppObject*) mdmObj)->interfaceNumberOfEntries);
      break;

#ifdef DMP_DEVICE2_HOSTS_2
   case MDMOID_DEV2_HOST:
      num = &(((_Dev2HostsObject*) mdmObj)->hostNumberOfEntries);
      break;

   case MDMOID_DEV2_HOST_IPV4_ADDRESS:
      num = &(((_Dev2HostObject*) mdmObj)->IPv4AddressNumberOfEntries);
      break;

   case MDMOID_DEV2_HOST_IPV6_ADDRESS:
      num = &(((_Dev2HostObject*) mdmObj)->IPv6AddressNumberOfEntries);
      break;
#endif  /* DMP_DEVICE2_HOSTS_2 */

#ifdef DMP_DEVICE2_DEVICEASSOCIATION_1
   case MDMOID_DEV2_MANAGEMENT_SERVER_MANAGEABLE_DEVICE:
      num = &(((_Dev2ManagementServerObject*) mdmObj)->manageableDeviceNumberOfEntries);
      break;
#endif  /* DMP_DEVICE2_DEVICEASSOCIATION_1 */

   case MDMOID_DEV2_DNS_SERVER:
      num = &(((Dev2DnsClientObject*) mdmObj)->serverNumberOfEntries);
      break;  
	  	  
#ifdef DMP_DEVICE2_SM_BASELINE_1
   case MDMOID_EXEC_ENV:
      num = &(((_SwModulesObject *) mdmObj)->execEnvNumberOfEntries);
      break;

   case MDMOID_DU:
      num = &(((_SwModulesObject *) mdmObj)->deploymentUnitNumberOfEntries);
      break;

   case MDMOID_EU:
      num = &(((_SwModulesObject *) mdmObj)->executionUnitNumberOfEntries);
      break;

   case MDMOID_BUS_OBJECT_PATH:
      num = &(((_BusObject *) mdmObj)->objectPathNumberOfEntries);
      break;

   case MDMOID_BUS_INTERFACE:
      num = &(((_BusObjectPathObject *) mdmObj)->interfaceNumberOfEntries);
      break;

   case MDMOID_BUS_METHOD:
      num = &(((_BusInterfaceObject *) mdmObj)->methodNumberOfEntries);
      break;

   case MDMOID_BUS_SIGNAL:
      num = &(((_BusInterfaceObject *) mdmObj)->signalNumberOfEntries);
      break;

   case MDMOID_BUS_PROPERTY:
      num = &(((_BusInterfaceObject *) mdmObj)->propertyNumberOfEntries);
      break;

   case MDMOID_BUS_CLIENT_PRIVILEGE:
      num = &(((_BusClientObject *) mdmObj)->privilegeNumberOfEntries);
      break;
#endif

#ifdef DMP_DEVICE2_ADVANCEDFIREWALL_1
   case MDMOID_DEV2_FIREWALL_LEVEL:
      num = &(((_Dev2FirewallObject*) mdmObj)->levelNumberOfEntries);
      break;

   case MDMOID_DEV2_FIREWALL_CHAIN:
      num = &(((_Dev2FirewallObject*) mdmObj)->chainNumberOfEntries);
      break;

   case MDMOID_DEV2_FIREWALL_CHAIN_RULE:
      num = &(((_Dev2FirewallChainObject*) mdmObj)->ruleNumberOfEntries);
      break;
#endif
#ifdef DMP_X_BROADCOM_COM_CONTAINER_1
   case MDMOID_CONTAINER_INFO:
      num = &(((_ContainerObject *) mdmObj)->containerNumberOfEntries);
      break;
#endif

#ifdef DMP_DEVICE2_DHCPV4CLIENT_1
   case MDMOID_DEV2_DHCPV4_CLIENT:
      num = &(((Dev2Dhcpv4Object *) mdmObj)->clientNumberOfEntries);
      break;
   case MDMOID_DEV2_DHCPV4_CLIENT_SENT_OPTION:
      num = &(((Dev2Dhcpv4ClientObject *) mdmObj)->sentOptionNumberOfEntries);
      break;
   case MDMOID_DEV2_DHCPV4_CLIENT_REQ_OPTION:
      num = &(((Dev2Dhcpv4ClientObject *) mdmObj)->reqOptionNumberOfEntries);
      break;
#endif

#ifdef DMP_DEVICE2_QOS_1
   case MDMOID_DEV2_QOS_CLASSIFICATION:
      num = &(((Dev2QosObject *) mdmObj)->classificationNumberOfEntries);
      break;

   case MDMOID_DEV2_QOS_POLICER:
      num = &(((Dev2QosObject *) mdmObj)->policerNumberOfEntries);
      break;

   case MDMOID_DEV2_QOS_QUEUE:
      num = &(((Dev2QosObject *) mdmObj)->queueNumberOfEntries);
      break;

#ifdef DMP_DEVICE2_QOSSTATS_1
   case MDMOID_DEV2_QOS_QUEUE_STATS:
      num = &(((Dev2QosObject *) mdmObj)->queueStatsNumberOfEntries);
      break;
#endif  /* DMP_DEVICE2_QOSSTATS_1 */

   case MDMOID_DEV2_QOS_SHAPER:
      num = &(((Dev2QosObject *) mdmObj)->shaperNumberOfEntries);
      break;
#endif  /* DMP_DEVICE2_QOS_1 */

#ifdef DMP_DEVICE2_PERIODICSTATSBASE_1
   case MDMOID_DEV2_SAMPLE_SET:
      num = &(((Dev2PeriodicStatObject *) mdmObj)->sampleSetNumberOfEntries);
      break;


   case MDMOID_DEV2_SAMPLE_PARAMETER:
      num = &(((Dev2SampleSetObject *) mdmObj)->parameterNumberOfEntries);
      break;
#endif
   case MDMOID_DEV2_NAT_INTF_SETTING:
      num = &(((Dev2NatObject *) mdmObj)->interfaceSettingNumberOfEntries);
      break;
   
   case MDMOID_DEV2_NAT_PORT_MAPPING:
      num = &(((Dev2NatObject *) mdmObj)->portMappingNumberOfEntries);
      break;

#ifdef DMP_DEVICE2_IEEE1905BASELINE_1
   case MDMOID_DEV2_IEEE1905_AL_IFC:
       num = &(((_Dev2Ieee1905AlObject *)mdmObj)->interfaceNumberOfEntries);
       break;

#ifdef DMP_DEVICE2_IEEE1905LINKMETRIC_1
   case MDMOID_DEV2_IEEE1905_AL_IFC_LINK:
       num = &(((_Dev2Ieee1905AlIfcObject *)mdmObj)->linkNumberOfEntries);
       break;
#endif

#ifdef DMP_DEVICE2_IEEE1905NETWORKTOPOLOGY_1
   case MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE:
       num = &(((_Dev2Ieee1905AlNetworkTopologyObject *)mdmObj)->IEEE1905DeviceNumberOfEntries);
       break;

   case MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IFC:
       num = &(((_Dev2Ieee1905AlNetworkTopologyIeee1905DeviceObject *)mdmObj)->interfaceNumberOfEntries);
       break;

   case MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_NON_IEEE1905_NEIGHBOR:
       num = &(((_Dev2Ieee1905AlNetworkTopologyIeee1905DeviceObject *)mdmObj)->nonIEEE1905NeighborNumberOfEntries);
       break;

   case MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR:
       num = &(((_Dev2Ieee1905AlNetworkTopologyIeee1905DeviceObject *)mdmObj)->IEEE1905NeighborNumberOfEntries);
       break;

   case MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_BRIDGING_TUPLE:
       num = &(((_Dev2Ieee1905AlNetworkTopologyIeee1905DeviceObject *)mdmObj)->bridgingTupleNumberOfEntries);
       break;

   case MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR_METRIC:
       num = &(((_Dev2Ieee1905AlNetworkTopologyIeee1905DeviceIeee1905NeighborObject *)mdmObj)->metricNumberOfEntries);
       break;
#endif
#endif  /* DMP_DEVICE2_IEEE1905BASELINE_1 */

#ifdef DMP_STORAGESERVICE_1
   case MDMOID_STORAGE_SERVICE:
      num = &(((Dev2ServicesObject *) mdmObj)->storageServiceNumberOfEntries);
      break;
#endif

      /* these are device2 objects that applies to both TR98 and TR181 */
#ifdef DMP_DEVICE2_XMPPBASIC_1	  
   case MDMOID_DEV2_XMPP_CONN:
      num = &(((Dev2XmppObject*) mdmObj)->connectionNumberOfEntries);
      break;
#endif
#ifdef DMP_DEVICE2_XMPPADVANCED_1
   case MDMOID_DEV2_XMPP_CONN_SERVER:
      num = &(((Dev2XmppConnObject*) mdmObj)->serverNumberOfEntries);
      break;
#endif

#ifdef DMP_DEVICE2_USBHOSTSBASIC_1
   case MDMOID_DEV2_USB_HOST_DEVICE:
      num = &(((Dev2UsbHostObject *) mdmObj)->deviceNumberOfEntries);
      break;

#ifdef DMP_DEVICE2_USBHOSTSADV_1
   case MDMOID_DEV2_USB_HOST_DEVICE_CONFIG:
      num = &(((Dev2UsbHostDeviceObject *) mdmObj)->configurationNumberOfEntries);
      break;

   case MDMOID_DEV2_USB_HOST_DEVICE_CONFIG_IFC:
      num = &(((Dev2UsbHostDeviceConfigObject *) mdmObj)->interfaceNumberOfEntries);
      break;
#endif  /* DMP_DEVICE2_USBHOSTSADV_1 */
#endif  /* DMP_DEVICE2_USBHOSTSBASIC_1 */

#ifdef DMP_DEVICE2_CERTIFICATES_1
   case MDMOID_DEV2_SECURITY_CERTIFICATE:
      num = &(((Dev2SecurityObject*) mdmObj)->certificateNumberOfEntries);
      break;
#endif  /* DMP_DEVICE2_CERTIFICATES_1 */

   case MDMOID_DEV2_DEVICE_VENDOR_CONFIG_FILE:
      num = &(((_Dev2DeviceInfoObject*) mdmObj)->vendorConfigFileNumberOfEntries);
      break;

   case MDMOID_DEV2_WIFI_SSID:
      num = &(((_Dev2WifiObject*) mdmObj)->SSIDNumberOfEntries);
      break;

   case MDMOID_DEV2_WIFI_ACCESS_POINT:
      num = &(((_Dev2WifiObject*) mdmObj)->accessPointNumberOfEntries);
      break;

   case MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE:
      num = &(((_Dev2WifiAccessPointObject*) mdmObj)->associatedDeviceNumberOfEntries);
      break;

#ifdef DMP_DEVICE2_FAST_1
   case MDMOID_DEV2_FAST_LINE:
      num = &(((_Dev2FastObject*) mdmObj)->lineNumberOfEntries);
      break;
#endif
#ifdef DMP_DEVICE2_ETHLAG_1
   case MDMOID_LA_G:
      num = &(((Dev2EthernetObject *) mdmObj)->LAGNumberOfEntries);
      break;
#endif

   default:
      cmsLog_error("cannot handle oid %d", decendentOid);
      cmsObj_free(&mdmObj);
      return;
   }

   if ((delta < 0) &&
       (((UINT32) (delta * -1)) > *num))
   {
      cmsLog_error("underflow detected for %s %s, delta=%d num=%d",
                   mdm_oidToGenericPath(ancestorOid),
                   cmsMdm_dumpIidStack(&ancestorIidStack),
                   delta, *num);
   }
   else
   {
      *num += delta;

      if ((ret = cmsObj_set(mdmObj, &ancestorIidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("set on %s %s failed, ret=%d",
                      mdm_oidToGenericPath(ancestorOid),
                      cmsMdm_dumpIidStack(&ancestorIidStack),
                      ret);
      }
   }

   cmsObj_free(&mdmObj);

   return;
}

/* these applies to TR98 and TR181 */
void rutUtil_modifyNumXmppConnServer_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_XMPP_CONN, MDMOID_DEV2_XMPP_CONN_SERVER, iidStack, delta);
}

void rutUtil_modifyNumXmppConn_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_XMPP, MDMOID_DEV2_XMPP_CONN, iidStack, delta);
}

void rutUtil_modifyNumWifiSsid(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_WIFI, MDMOID_DEV2_WIFI_SSID, iidStack, delta);
}

void rutUtil_modifyNumWifiAccessPoint(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_WIFI, MDMOID_DEV2_WIFI_ACCESS_POINT, iidStack, delta);
}

void rutUtil_modifyNumWifiAssociatedDevice(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_WIFI_ACCESS_POINT, MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE, iidStack, delta);
}

#ifdef DMP_DEVICE2_CERTIFICATES_1
/* these applies to TR181 only */
void rutUtil_modifyNumSecCert_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_SECURITY, MDMOID_DEV2_SECURITY_CERTIFICATE, iidStack, delta);
}
#endif

#ifdef DMP_X_BROADCOM_COM_CONTAINER_1
void rutUtil_modifyNumContainerEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_CONTAINER, MDMOID_CONTAINER_INFO, iidStack, delta);
}
#endif /* DMP_X_BROADCOM_COM_CONTAINER_1 */

#ifdef DMP_DEVICE2_SM_BASELINE_1
void rutUtil_modifyNumExecEnvEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_SW_MODULES, MDMOID_EXEC_ENV, iidStack, delta);
}

void rutUtil_modifyNumDeploymentUnitEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_SW_MODULES, MDMOID_DU, iidStack, delta);
}

void rutUtil_modifyNumExecutionUnitEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_SW_MODULES, MDMOID_EU, iidStack, delta);
}

void rutUtil_modifyNumBusObjectPathEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_BUS, MDMOID_BUS_OBJECT_PATH, iidStack, delta);
}

void rutUtil_modifyNumBusInterfaceEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_BUS_OBJECT_PATH, MDMOID_BUS_INTERFACE, iidStack, delta);
}

void rutUtil_modifyNumBusMethodEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_BUS_INTERFACE, MDMOID_BUS_METHOD, iidStack, delta);
}

void rutUtil_modifyNumBusSignalEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_BUS_INTERFACE, MDMOID_BUS_SIGNAL, iidStack, delta);
}

void rutUtil_modifyNumBusPropertyEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_BUS_INTERFACE, MDMOID_BUS_PROPERTY, iidStack, delta);
}

void rutUtil_modifyNumBusClientPrivilegeEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_BUS_CLIENT, MDMOID_BUS_CLIENT_PRIVILEGE, iidStack, delta);
}


#ifdef DMP_DEVICE2_ETHLAG_1
void rutUtil_modifyNumEthLag_dev2(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_ETHERNET,  MDMOID_LA_G, iidStack, delta);
}
#endif /* DMP_DEVICE2_ETHLAG_1 */

#endif /* DMP_DEVICE2_SM_BASELINE_1 */

CmsRet rutUtil_getAvailVlanIndex_dev2(const char *l2Ifname, SINT32 *nextVlanIndex)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2BridgePortObject *brPortObj = NULL;
   Dev2VlanTerminationObject *vlanTermObj = NULL;
   SINT32 l2IfnameLen;
   SINT32 numVlanIntf = 0;
   SINT32 idx;
   SINT32 vlanIndexArray[IFC_WAN_MAX+1] = {0};
   char *p = NULL;

   if (IS_EMPTY_STRING(l2Ifname))
   {
      cmsLog_error("l2Ifname is empty.");
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (strchr(l2Ifname, '.'))
   {
      cmsLog_error("Invalid l2Ifname %s. Shall not contain dot.");
      return CMSRET_INVALID_ARGUMENTS;
   }

   l2IfnameLen = strlen(l2Ifname);

   /* First loop thru all the bridge non-management ports to find all vlan
    * interface index that are in used.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE_PORT,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&brPortObj) == CMSRET_SUCCESS)
   {
      if (!brPortObj->managementPort &&
          !cmsUtl_strncmp(brPortObj->name, l2Ifname, l2IfnameLen))
      {
         /* Skip this port if it points to a VLAN termination object */
         if (cmsUtl_strcasestr(brPortObj->lowerLayers, "VLANTermination") == NULL)
         {
            /* Find the digit after '.' and mark it to 1 in the array for later use. */
            p = strchr(brPortObj->name, '.');
            if ((p != NULL) && isdigit(*(p+1)))
            {
               idx = atoi(p+1);
               cmsLog_debug(" idx %d", idx);
               if (idx > IFC_VLAN_MAX)
               {
                  cmsLog_debug(" Max idx is %d, current idx %d", IFC_VLAN_MAX, idx);
                  numVlanIntf++;            
               }
               else if (idx == 0)
               {
                  vlanIndexArray[idx] = 1;
                  /* don't count ethx.0 */
               }
               else
               {
                  if (vlanIndexArray[idx] == 0)
                  {
                     vlanIndexArray[idx] = 1;
                     numVlanIntf++;
                  }
               }
            }
         }
      }         
      cmsObj_free((void **) &brPortObj);
   }

   /* Then loop thru all vlan termination objects to find all vlan
    * interface index that are in used.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNextFlags(MDMOID_DEV2_VLAN_TERMINATION,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&vlanTermObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strncmp(vlanTermObj->name, l2Ifname, l2IfnameLen))
      {
         /* Find the digit after '.' and mark it to 1 in the array for later use. */
         p = strchr(vlanTermObj->name, '.');
         if ((p != NULL) && isdigit(*(p+1)))
         {
            idx = atoi(p+1);
            cmsLog_debug(" idx %d", idx);
            if (idx > IFC_VLAN_MAX)
            {
               cmsLog_debug(" Max idx is %d, current idx %d", IFC_VLAN_MAX, idx);
               numVlanIntf++;            
            }
            else if (idx == 0)
            {
               vlanIndexArray[idx] = 1;
               /* don't count ethx.0 */
            }
            else
            {
               if (vlanIndexArray[idx] == 0)
               {
                  vlanIndexArray[idx] = 1;
                  numVlanIntf++;
               }
            }
         }
      }         
      cmsObj_free((void **) &vlanTermObj);
   }

   idx = 0;

   if (numVlanIntf < IFC_VLAN_MAX)
   {
      for (idx = 1; idx <= IFC_VLAN_MAX; idx++)
      {
         if (vlanIndexArray[idx] == 0)
         {
            cmsLog_debug("found available vlanIndex=%d", idx);
            *nextVlanIndex = idx;
            break;
         }
      }
   }

   if (numVlanIntf >= IFC_VLAN_MAX || idx > IFC_VLAN_MAX)
   {
      cmsLog_error("L2Intf %s already has max number of vlanIntf (%d)", l2Ifname, IFC_VLAN_MAX);
      ret = CMSRET_RESOURCE_EXCEEDED;
   }

   cmsLog_debug("Exit ret %d *nextVlanIndex %d", ret, *nextVlanIndex );

   return ret;
}

#endif /* DMP_DEVICE2_BASELINE_1 */
