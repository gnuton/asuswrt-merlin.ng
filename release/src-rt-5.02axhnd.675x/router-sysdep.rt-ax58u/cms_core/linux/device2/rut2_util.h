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

#ifndef __RUT2_UTIL_H__
#define __RUT2_UTIL_H__


/*!\file rut2_util.h
 * \brief random assortment of useful system level interface functions which applies to device2 only
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */



#include "cms.h"
#include "cms_msg.h"
#include "mdm_object.h"


/** Generic function to change the NumberOfEntries field in the parent object.
 *
 * @param ancestorOid  (IN) The ancestor oid which contains the NumberOfEntries parameter.
 * @param decendednOid (IN) The oid which is being created or deleted.
 * @param iidStack (IN) iidStack of the object being created or deleted.
 * @param delta (IN)    The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumGeneric_dev2(MdmObjectId ancestorOid,
                               MdmObjectId decendentOid,
                               const InstanceIdStack *iidStack,
                               SINT32 delta);


/** Change the InterfaceNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) DEV2_DEVICE object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumInterfaceStack(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the linkNumberOfEntries field in the ATM object.
 *
 * @param iidStack (IN) iidStack of the ATM object.  This iidStack is
 *                      used to find the ancestor ATM.Link object.
 * @param delta (IN)    The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumAtmLink(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the linkNumberOfEntries field in the PTM object.
 *
 * @param iidStack (IN) iidStack of the PTM.  This iidStack is
 *                      used to find the ancestor PTM.LINK
 * @param delta (IN)    The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumPtmLink(const InstanceIdStack *iidStack, SINT32 delta);

/** Change the lineNumberOfEntries field in the DSL object.
 *
 * @param iidStack (IN) iidStack of the DSL_LINE object.  This iidStack is
 *                      used to find the ancestor DSL Line Object
 * @param delta (IN)    The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumDslLine(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the channelNumberOfEntries field in the ancestor
 *  DSLObject.
 *
 * @param iidStack (IN) iidStack of the DSL_CHANNEL Object.  This iidStack is
 *                      used to find the ancestor DSL_CHANNEL
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumDslChannel(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the bondingGroupNumberOfEntries field in the ancestor
 *  DSLObject.
 *
 * @param iidStack (IN) iidStack of the DSL_BONDING.  This iidStack is
 *                      used to find the ancestor DSL Bonding object
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumDslBondingGroup(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the InterfaceNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) DEV2_ETHERNET object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumEthInterface(const InstanceIdStack *iidStack, SINT32 delta);

/** Change the InterfaceNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) DEV2_ETHERNET object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumEthernetLink(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the InterfaceNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) DEV2_BRIDGING object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumBridge(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the InterfaceNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) DEV2_BRIDGING object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumBridgeFilter(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the InterfaceNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) DEV2_BRIDGE object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumBridgePort(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the InterfaceNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) DEV2_BRIDGE object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumBridgeVlan(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the InterfaceNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) DEV2_BRIDGE object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumBridgeVlanPort(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the InterfaceNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) DEV2_IP object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumIpInterface(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the IPv4AddressNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) DEV2_IP_INTERFACE object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumIpv4Address(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the poolNumberOfEntries field in the ancestor Dev2Dhcpv4ServerObject.
 *
 * @param iidStack (IN) iidStack of the DHCPV4_SERVER_POOL. This iidStack is
 *                      used to find the ancestor Dev2Dhcpv4ServerObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumDhcpv4ServerPool(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the staticAddressNumberOfEntries field in the ancestor
 *  Dev2Dhcpv4ServerPoolObject.
 *
 * @param iidStack (IN) iidStack of the DHCPV4_SERVER_POOL_STATIC_ADDRESS. This iidStack is
 *                      used to find the ancestor Dev2Dhcpv4ServerPoolObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumDhcpv4ServerPoolStaticAddress(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the clientAddressNumberOfEntries field in the ancestor
 *  Dev2Dhcpv4ServerPoolObject.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT. This iidStack is
 *                      used to find the ancestor Dev2Dhcpv4ServerPoolObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumDhcpv4ServerPoolClient(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the IPv4AddressNumberOfEntries field in the ancestor
 *  Dev2Dhcpv4ServerPoolClientObject.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT_I_PV4_ADDRESS. 
 *                      This iidStack is used to find the ancestor Dev2Dhcpv4ServerPoolClientObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumDhcpv4ServerPoolClientIPv4Address(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the optionNumberOfEntries field in the ancestor
 *  Dev2Dhcpv4ServerPoolClientObject.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT_OPTION.
 *                      This iidStack is used to find the ancestor Dev2Dhcpv4ServerPoolClientObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumDhcpv4ServerPoolClientOption(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the clientNumberOfEntries field in the ancestor
 *  Dev2Dhcpv4Object.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_DHCPV4. This iidStack is
 *                      used to find the ancestor Dev2Dhcpv4Object
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumDhcpv4Client(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the sentOptionNumberOfEntries field in the ancestor
 *  Dev2Dhcpv4ClientObject.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_DHCPV4_CLIENT. This iidStack is
 *                      used to find the ancestor Dev2Dhcpv4ClientObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumDhcpv4ClientSentOption(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the reqOptionNumberOfEntries field in the ancestor
 *  Dev2Dhcpv4ClientObject.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_DHCPV4_CLIENT. This iidStack is
 *                      used to find the ancestor Dev2Dhcpv4ClientObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumDhcpv4ClientReqOption(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the hostNumberOfEntries field in the ancestor
 *  Dev2HostsObject.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_HOST. This iidStack is
 *                      used to find the ancestor Dev2HostsObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumHost(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the IPv4AddressNumberOfEntries field in the ancestor
 *  Dev2HostObject.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_HOST_IPV4_ADDRESS. This iidStack is
 *                      used to find the ancestor Dev2HostObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumHostIPv4Address(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the IPv6AddressNumberOfEntries field in the ancestor
 *  Dev2HostObject.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_HOST_IPV6_ADDRESS. This iidStack is
 *                      used to find the ancestor Dev2HostObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumHostIPv6Address(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the IPv4ForwardingNumberOfEntries field in the ancestor DevRouter obj.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_IPV4_FORWARDING object.
 *                      This iidStack is used to find the ancestor Dev2RouterObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void modifyNumRouterIpv4ForwardingEntry(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the IPv6ForwardingNumberOfEntries field in the ancestor DevRouter obj.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_IPV6_FORWARDING object.
 *                      This iidStack is used to find the ancestor Dev2RouterObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void modifyNumRouterIpv6ForwardingEntry(const InstanceIdStack *iidStack, SINT32 delta);

#if SUPPORT_RIP

/** Change the RipIntfSettingNumberOfEntries field in the ancestor obj.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_RIP object.
 *                      This iidStack is used to find the ancestor Dev2RipObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 */

void modifyNumRipIntfSettingEntry(const InstanceIdStack *iidStack, SINT32 delta);

#endif

#ifdef DMP_DEVICE2_IPV6ROUTING_1

/** Change the RouteInfoIntfSettingNumberOfEntries field in the ancestor obj.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_ROUTE_INFO_INTF_SETTING object.
 *                      This iidStack is used to find the ancestor Dev2RouteInfoObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 */

void modifyNumRouteInfoIntfSettingEntry(const InstanceIdStack *iidStack, SINT32 delta);

#endif

/** Change the manageableDeviceNumberOfEntries field in the ancestor
 *  Dev2ManagementServerObject.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_MANAGEMENT_SERVER. This iidStack is
 *                      used to find the ancestor Dev2ManagementServerObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumManageableDevices_dev2(const InstanceIdStack *iidStack, SINT32 delta);

/** Change the PPP NumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) Dev2PppObject  object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumPppInterface(const InstanceIdStack *iidStack, SINT32 delta);

/** Change the SampleSet NumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) Dev2SampleSetObject  object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumSampleSets_dev2(const InstanceIdStack *iidStack, SINT32 delta);

/** Change the Parameter NumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) Dev2SampleParameterObject  object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumParameters_dev2(const InstanceIdStack *iidStack, SINT32 delta);

/** Change the usb host deviceNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) Dev2UsbHostObject  object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumUsbDevice(const InstanceIdStack *iidStack, SINT32 delta);

/** Change the usb host device configurationNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) Dev2UsbHostDeviceObject  object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumUsbDeviceConfig(const InstanceIdStack *iidStack, SINT32 delta);

/** Change the usb host device configration interfaceNumberOfEntries field in the ancestor object.
 *
 * @param iidStack (IN) iidStack of the (ancestor) Dev2UsbHostDeviceConfigObject  object.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumUsbDeviceConfigIfc(const InstanceIdStack *iidStack, SINT32 delta);

/** Change the lineNumberOfEntries field in the FAST object.
 *
 * @param iidStack (IN) iidStack of the FAST_LINE object.  This iidStack is
 *                      used to find the ancestor FAST Line Object
 * @param delta (IN)    The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumFastLine(const InstanceIdStack *iidStack, SINT32 delta);

void rutUtil_modifyNumEthLag_dev2(const InstanceIdStack *iidStack, SINT32 delta);

/** Get the available virtual interface index to be used for creating the
 *  virtual interface name of a layer 2 interface.
 *
 * @param l2Ifname (IN) Layer 2 interface name.
 * @param nextVlanIndex (OUT) the next available virtual interface name index.
 *
 * @return CmsRet
 */
CmsRet rutUtil_getAvailVlanIndex_dev2(const char *l2Ifname, SINT32 *nextVlanIndex);

#endif //__RUT2_UTIL_H__

