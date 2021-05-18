/*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
:>
*/

#ifndef __RUT_LAN_H__
#define __RUT_LAN_H__


/*!\file rut_lan.h
 * \brief System level interface functions for LAN functionality.
 *
 * In most cases, the functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"


#define UDHCPD_DECLINE "/var/udhcpd.decline"


#if defined(DESKTOP_LINUX) && !defined(BUILD_DESKTOP_BEEP)

#define DNS_FYI_FILENAME        "./dns"
#define REAL_DNS_FYI_FILENAME   "./dns.real"
#define UDHCPD_CONFIG_FILENAME  "./udhcpd.conf"
#else

/*
 * /etc/resolv.conf is inside the read-only root file system image.
 * So we have to make a symlink from it to /var/fyi/sys/dns, which 
 * is in the ramfs, which we can write to.
 *
 * Similarly, /etc/udhcpd.conf is actually a symlink to 
 * /var/udhcpd/udhcpd.conf
 */
#define DNS_FYI_FILENAME        "/var/fyi/sys/dns"
#define REAL_DNS_FYI_FILENAME   "/var/fyi/sys/dns.real"
#define UDHCPD_CONFIG_FILENAME  "/var/udhcpd/udhcpd.conf"
#endif


/** Create a bridge under LANDevice.
 * 
 * Create an instance of LANDevice object.  Create a single 
 * IP Interface object under the LAN Device.  Configure the IP Interface
 * object and also the LanHostConfigManagement object (for dhcpd) under
 * the LANDevice.  But do not enable the IP interface.
 * 
 * This function is provided to other RCL/RUT functions (right now only
 * for rut_pmap.c)  It is not used by rcl_lan.c itself.
 * 
 * @param bridgeNumber (OUT) The number of the bridge that was created, e.g.
 *                           if br1 was created, bridgeNumber will be set to 1.
 * 
 * @return CmsRet enum.
 */
CmsRet rutLan_addBridge(UINT32 *bridgeNumber);


/** Delete the LANDevice that contains the specified bridge.
 * 
 * Delete the entire LANDevice instance that contains the specified
 * bridge.  Any eth, usb, or wireless interfaces under the LANDevice
 * will also be deleted.
 * 
 * This function is provided to other RCL/RUT functions (right now only
 * for rut_pmap.c)  It is not used by rcl_lan.c itself.
 * 
 * @param bridgeIfName (IN) The name of the bridge to create, e.g. br1
 *  
 */
void rutLan_deleteBridge(const char *bridgeIfName);


/** Add a ethernet interface under the default bridge (br0)
 * 
 * It does not make sense to create a eth interface under any other
 * bridge except br0.  For portmapping/bridging operations, use
 * rutLan_moveEthInterface().
 * 
 * This function is provided to other RCL/RUT functions (right now only
 * for rut_ethswitch.c).  It is not used by rcl_lan.c itself.
 * 
 * @param ethIfName (IN) The name of the ethernet interface to create.
 * 
 * @return CmsRet enum.
 */
CmsRet rutLan_addEthInterface(const char *ethIfName);


/** Delete the specified ethernet interface.
 * 
 * The bridge name does not have to be specified, this function can
 * find the ethernet interface from any bridge.
 * 
 * This function is provided to other RCL/RUT functions (right now only
 * for rut_ethswitch.c).  It is not used by rcl_lan.c itself.
 * 
 * @param ethIfName (IN) The name of the ethernet interface to create.
 * 
 * @return CmsRet enum.
 */
void rutLan_deleteEthInterface(const char *ethIfName);


/** Get the specified ethernet interface on the LAN side.
 *
 * @param ifName     (IN) The interface name of the eth interface to get.
 * @param iidStack  (OUT) the iidStack of the requested eth interface.
 * @param lanEthObj (OUT) The requested LanEthernetInterfaceConfig object.  Caller
 *                        is responsible for calling cmsObj_free() on this object.
 *
 * @return CmsRet enum.
 */
CmsRet rutLan_getEthInterface(const char *ifName, InstanceIdStack *iidStack, LanEthIntfObject **lanEthObj);


/** Move the specified ethernet interface from one bridge to another.
 * 
 * Internally, the LAN_ETH_INTF object is still deleted from 
 * the source bridge and created under the destination bridge, but
 * all the parameter settings are preserved.
 * 
 * This function is provided to other RCL/RUT functions (right now only
 * for rut_pmap.c)  It is not used by rcl_lan.c itself.
 * 
 * @param ethIfName (IN) The ethernet interface name to move.
 * @param fromBridgeIfName (IN) The name of the bridge that the ethernet interface 
 *                              is currently under.
 * @param toBridgeIfName   (IN) The name of the bridge to move the ethernet 
 *                              interface to.
 * 
 * @return CmsRet enum.
 */
CmsRet rutLan_moveEthInterface(const char *ethIfName, const char *fromBridgeIfName, const char *toBridgeIfName);


CmsRet rutLan_addMocaInterface(const char *ifName);

void rutLan_deleteMocaInterface(const char *ifName);

CmsRet rutLan_getMocaInterface(const char *ifName, InstanceIdStack *iidStack, LanMocaIntfObject **lanMocaObj);

CmsRet rutLan_moveMocaInterface(const char *ifName, const char *fromBridgeIfName, const char *toBridgeIfName);


CmsRet rutLan_moveUsbInterface(const char *usbIfName, const char *fromBridgeIfName, const char *toBridgeIfName);


/** Move the specified wifi interface from one bridge to another.
 * 
 * @param IfName (IN) The wifi interface name to move.
 * @param fromBridgeIfName (IN) The name of the bridge that the wifi interface 
 *                              is currently under.
 * @param toBridgeIfName   (IN) The name of the bridge to move the wifi 
 *                              interface to.
 * 
 * @return CmsRet enum.
 */
CmsRet rutLan_moveWlanInterface(const char *ifName, const char *fromBridgeIfName, const char *toBridgeIfName);


/** Get the specified wifi interface on the LAN side.
 *
 * @param ifName     (IN) The wifi name of the wifi interface to get.
 * @param iidStack  (OUT) the iidStack of the requested eth interface.
 * @param wlVirtIntfCfgObj (OUT) The requested wlVirtIntfCfgObj object.  Caller
 *                        is responsible for calling cmsObj_free() on this object.
 *
 * @return CmsRet enum.
 */
CmsRet rutLan_getWlanInterface(const char *ifName, InstanceIdStack *iidStack,  WlVirtIntfCfgObject **wlVirtIntfCfgObj );


/** Add a wifi interface under the default bridge (br0)
 * 
 * @param wlIfName (IN) The name of the wifi interface to create.
 * 
 * @return CmsRet enum.
 */
CmsRet rutLan_addWlanInterface(const char *wlIfName);


/** Delete the specified wifi interface.
 * 
 * The bridge name does not have to be specified, this function can
 * find the wifi interface from any bridge.
 * 
 * @param wlIfName (IN) The name of the wifi interface to create.
 * 
 * @return CmsRet enum.
 */
void rutLan_deleteWlanInterface(const char *wlIfName);



/** Do runtime action to create and enable the specified bridge.
 * 
 *
 * @param bridgeIfName (IN) The bridge name, e.g. br0
 * @param isNewBridge (IN) True if this a new bridge that needs to be created using brctl addbr.
 * @param ipAddr (IN) IP address of the bridge in dotted decimal format.
 * @param netmask (IN) netmask of the bridge interface.
 * @param broadcast (IN) broadcast address of the bridge interface.
 * 
 */
void rutLan_enableBridge(const char *bridgeIfName,
                         UBOOL8 isNewBridge,
                         const char *ipAddr,
                         const char *netmask,
                         const char *broadcast);


/** Do runtime action to disable and delete the specified bridge.
 * 
 * This is used both by rcl_lan.c and other RCL/RUT functions.
 * 
 * @param bridgeIfName (IN) The bridge name, e.g. br0
 */
void rutLan_disableBridge(const char *bridgeIfName);




/** Get the next available interface number.
 * 
 * @return -1 on failure, otherwise, the next available bridge number.
 */
SINT32 rutLan_getNextAvailableBridgeNumber(void);
SINT32 rutLan_getNextAvailableBridgeNumber_igd(void);
SINT32 rutLan_getNextAvailableBridgeNumber_dev2(void);

#if defined(SUPPORT_DM_LEGACY98)
#define rutLan_getNextAvailableBridgeNumber()   rutLan_getNextAvailableBridgeNumber_igd()
#elif defined(SUPPORT_DM_HYBRID)
#define rutLan_getNextAvailableBridgeNumber()   rutLan_getNextAvailableBridgeNumber_igd()
#elif defined(SUPPORT_DM_PURE181)
#define rutLan_getNextAvailableBridgeNumber()   rutLan_getNextAvailableBridgeNumber_dev2()
#elif defined(SUPPORT_DM_DETECT)
#define rutLan_getNextAvailableBridgeNumber()   (cmsMdm_isDataModelDevice2() ? \
                                   rutLan_getNextAvailableBridgeNumber_dev2() : \
                                   rutLan_getNextAvailableBridgeNumber_igd())
#endif




/** bring the specified LAN interface UP. 
 * 
 * @param lanIfName    (IN) lan interface name
 * 
 */
void rutLan_enableInterface(const char *lanIfName);


/** Add the specified LAN or WAN interface to the specified bridge 
 * 
 * @param ifName       (IN) interface name
 * @param isWanIntf    (IN) is the first interface a WAN interface? 
 * @param bridgeIfName (IN) bridge interface name
 * 
 */
void rutLan_addInterfaceToBridge(const char *lanIfName, UBOOL8 isWanIntf, const char *bridgeIfName);


/** bring the specified LAN interface down.
 * 
 * @param lanIfName   (IN) lan interface name
 * 
 */
void rutLan_disableInterface(const char *lanIfName);


/** Remove the specified LAN interface from the specified bridge.
 * 
 * @param lanIfName   (IN) lan interface name
 * @param bridgeIfName (IN) bridge interface name
 * 
 */
void rutLan_removeInterfaceFromBridge(const char *lanIfName, const char *bridgeIfName);


/** Get the bridge name that this LAN interface belongs to.
 * 
 * @param lanOid       (IN) MDMOID of the LAN interface object.
 * @param iidStack     (IN) The iidStack of the LAN interface.
 * @param bridgeIfName (OUT) A buffer which will be filled in with the name
 *                           of the parent bridge.  This buffer should be at
 *                           least 32 bytes.
 * 
 * @return CmsRet enum.
 */
CmsRet rutLan_getParentBridgeIfName(MdmObjectId mdmOid, const InstanceIdStack *iidStack, char *bridgeIfName);
 
 
/** Get the bridge name that this eth interface belongs to.
 * 
 * @param ethIfName    (IN) The name of the eth interface.
 * @param bridgeIfName (OUT) A buffer which will be filled in with the name
 *                           of the parent bridge.  This buffer should be at
 *                           least 32 bytes.
 * 
 * @return CmsRet enum.
 */
CmsRet rutLan_getParentBridgeIfNameOfEth(const char *lanIfName, char *bridgeIfName);


/** Create "/var/fyi/sys/dns" file.  /etc/resolv.conf is a symlink to this file.
 *
 * This file contains the currently active DNS servers.  If none, there will be 0.0.0.0
 * If dnsproxy is enabled, this file will also contain the local domain name.
 *
 */                            
void rutLan_createResolvCfg(void);




/** Create "/etc/udhcpd.conf" file.
 * 
 */  
void rutLan_createDhcpdCfg(void);

void rutLan_createDhcpdCfg_igd(void);

void rutLan_createDhcpdCfg_dev2(void);

#if defined(SUPPORT_DM_LEGACY98)
#define rutLan_createDhcpdCfg()   rutLan_createDhcpdCfg_igd()
#elif defined(SUPPORT_DM_HYBRID)
#define rutLan_createDhcpdCfg()   rutLan_createDhcpdCfg_igd()
#elif defined(SUPPORT_DM_PURE181)
#define rutLan_createDhcpdCfg()   rutLan_createDhcpdCfg_dev2()
#elif defined(SUPPORT_DM_DETECT)
#define rutLan_createDhcpdCfg()   (cmsMdm_isDataModelDevice2() ? \
                                   rutLan_createDhcpdCfg_dev2() : \
                                   rutLan_createDhcpdCfg_igd())
#endif




/** Find out if the parameters have been changed or not.
 * 
 * @param *newObj       (IN) the new _LanHostCfgObject.
 * @param *CurrObj      (IN) the current _LanHostCfgObject.
 * @return BOOL  TRUE -- currObj object parameters in the check differ from newObj.  FALSE -- no change
 */
UBOOL8 rutLan_isHostCfgChanged(const _LanHostCfgObject *newObj, const _LanHostCfgObject *currObj);


/** Return true if the IPv4 address or netmask has changed.
 * 
 * This function is used to detect an edit condition on existing, enabled object.
 * 
 * @param newObj (IN) new IP interface object.
 * @param currObj (IN) current IP interface object.
 * 
 * @return TRUE if ip address or netmask has changed.
 */
UBOOL8 rutLan_isIpv4IntfChanged(const _LanIpIntfObject *newObj, const _LanIpIntfObject *currObj);

/* Return true if IPV4 address or netmask has changed, rutLan_isIpv4IntfChanged function has
 * more conditions to check abou the interface, not only address 
 */

UBOOL8 rutLan_isIpv4AddressChanged(const _LanIpIntfObject *newObj, const _LanIpIntfObject *currObj);

/** Return true if the IPv6 address has changed.
 * 
 * This function is used to detect if br0 got a new IPv6 address (either static or delegated)
 * 
 * @param newObj (IN) new IP interface object.
 * @param currObj (IN) current IP interface object.
 * 
 * @return TRUE if ip address or netmask has changed.
 */
UBOOL8 rutLan_isIpv6IntfChanged(const _LanIpIntfObject *newObj, const _LanIpIntfObject *currObj);


/** Get the lan interface status and return them in the provided buffers.
 *
 * @param ifName (IN) Name of the lan interface.
 * @param statusStr (OUT) Will contain the status of the lan interface, the buffer
 *                        must be at least 16 bytes long.
 * @param hwAddr (OUT) will contain the mac address of the lan interface, the buffer
 *                     must be at least 32 bytes long.
 */
void rutLan_getIntfStatus(const char *ifName, char *statusStr, char *hwAddr);


/** Get the lan interface MAC address
 *
 * @param ifName (IN) Name of the lan interface.
 * @param hwAddr (OUT) will contain the 6-byte mac address of the lan interface
 */
void rutLan_getIntfMacAddr(const char *ifName, UINT8 *hwAddr);


/** Return the ifname of the first LAN Ip Interface that is in DHCP mode
 *  and is in the Connected state.  Used in homeplug devices.
 *  This function is only needed in TR98 mode.
 *
 * @param ifName (OUT) On success, this buffer will be filled in with the
 *                     ifname of a LAN Ip Interface in DHCP mode and is
 *                     in the Connected state.
 *
 * @return TRUE if an appropriate interface is found.
 */
UBOOL8 rutLan_findFirstDhcpcAndConnected(char *ifName);
UBOOL8 rutLan_findFirstDhcpcAndConnected_igd(char *ifName);

#define rutLan_findFirstDhcpcAndConnected(i)   rutLan_findFirstDhcpcAndConnected_igd((i))




CmsRet rutLan_setLanIPv4Info_dev2(const char *ifName,
                                  const char *addr, const char *subnetMask);

UBOOL8 rutLan_isDhcpv4ServerPoolChanged_dev2(
                                const _Dev2Dhcpv4ServerPoolObject *newObj,
                                const _Dev2Dhcpv4ServerPoolObject *currObj);

UBOOL8 rutLan_isDhcpv4ServerPoolStaticAddressChanged_dev2(
                    const _Dev2Dhcpv4ServerPoolStaticAddressObject *newObj,
                    const _Dev2Dhcpv4ServerPoolStaticAddressObject *currObj);

void rutLan_reconfigNatForAddressChange_dev2(const char *oldIpAddr, const char *oldMask,
                                        const char *newIpAddr, const char *newMask);


/** Return true if there is a DHCP server enabled for any of the LAN subnets
 * in the modem.
 *
 * @return true if there is a DHCP server enabled for any of the LAN subnets
 * in the modem.
 */
UBOOL8 rutLan_isDhcpdEnabled(void);

UBOOL8 rutLan_isDhcpdEnabled_igd(void);

UBOOL8 rutLan_isDhcpdEnabled_dev2(void);

#if defined(SUPPORT_DM_LEGACY98)
#define rutLan_isDhcpdEnabled()  rutLan_isDhcpdEnabled_igd()
#elif defined(SUPPORT_DM_HYBRID)
#define rutLan_isDhcpdEnabled()  rutLan_isDhcpdEnabled_igd()
#elif defined(SUPPORT_DM_PURE181)
#define rutLan_isDhcpdEnabled()  rutLan_isDhcpdEnabled_dev2()
#elif defined(SUPPORT_DM_DETECT)
#define rutLan_isDhcpdEnabled()  (cmsMdm_isDataModelDevice2() ? \
                                  rutLan_isDhcpdEnabled_dev2() : \
                                  rutLan_isDhcpdEnabled_igd())
#endif



CmsRet rutLan_getLanDevByBridgeIfName(const char *brIfName, InstanceIdStack *iidStack, LanDevObject **lanDev);

/** It is called when a WAN service (ip/ppp) is added (and on startup) or deleted.
 * If dhcpd is not need in the system -- all bridge WAN service or all disabled  at LAN page.
 * if dhcpd is needed, a new udhpd.conf will be created and dhcpd is restarted.
 *
 * @return CmsRet enum.
 */
CmsRet rutLan_updateDhcpd(void);


/** Re-insert all the WAN to LAN nat rules for address masquerading because
 * the LAN side IP address changed.
 *
 * @param oldIpAddr (IN) The previous br0 IP Address.
 * @param oldMask   (IN) The previous br0 netmask;
 * @param newIpAddr (IN) The new br0 IP address.
 * @param newMask   (IN) The new br0 netmask;
 */
void rutLan_reconfigNatForAddressChange(const char *oldIpAddr, const char *oldMask,
                                        const char *newIpAddr, const char *newMask);


/** create shadow vlan interface for the ethernet interface.  ie. create eth0.0 for eth0
 * eth1, eth1.0, etc.
 *
 *  @param baseIfName (IN) base interface name
 *  @param bridgeIfName (IN) the bridge interface name this interface belong to
 *  @param baseIfName (IN)  If TRUE, add the vlan. If FALSE, remove it.
 *
 * @return CmsRet enum.
 */
CmsRet rutLan_configShadowVlanInterface(const char *baseIfName, const char *bridgeIfName, UBOOL8 add);

void rutLan_configEthIfMcastRule(const char *realIf, const char *vlanif);

void rutLan_configEponVlanIfMcastRule(const char *realIf, const char *vlanif);

#ifdef SUPPORT_LANVLAN
void rutLan_AddDefaultLanVlanInterface(char *l2IfName);
void rutLan_RemoveDefaultLanVlanInterface(char *l2IfName);
void rutLan_RemoveLanVlanInterface(char *l2IfName, char *vlanList);
void rutLan_CreateLanVlanIf(char *l2IfName, char *vlanList,  const InstanceIdStack *iidStack, UBOOL8 addnew);
#endif



/** Set the ipv6 address of br0.
 * 
 * @param newAddr (IN) The new address to be set.
 * @param newIfName (IN) The interface name of the new address.
 * @param oldAddr (IN) The old address to be removed.
 * @param oldIfName (IN) The interface name of the old address.
 * 
 * @return CmsRet enum.
 */
CmsRet rutLan_setIPv6Address(char *newAddr, char *newIfName,
                             char *oldAddr, char *oldIfName);

/** Fetch the prefix information for stateful DHCPv6 server
 * 
 * @param ifname (IN)  interface name
 * @param sitePrefix (OUT)  prefix information
 * 
 * @return CmsRet enum.
 */
CmsRet rut_getDhcp6sPrefixFromInterface(const char *ifname, char ** sitePrefix);

/** Restart the dhcpv6 server on br0.
 * 
 * @param dns6Servers (IN) The dns server addresses to be advertise.
 * @param ipv6LanCfgObj (IN) 
 * 
 * @return CmsRet enum.
 */
CmsRet rut_restartDhcp6s(const char *dns6Servers, const char *domainName,
                UBOOL8 isStateful, const char *minIntfID, const char *maxIntfID,
                SINT32 leaseTime, const char *sitePrefix);

/** Stop the dhcpv6 server on br0.
 * 
 * @return CmsRet enum.
 */
CmsRet rut_stopDhcp6s(void);


/** Create the radvd.conf.
 * 
 * @return CmsRet enum.
 */
CmsRet rut_createRadvdConf(void);


/** Restart radvd daemon
 * 
 * @return CmsRet enum.
 */
CmsRet rut_restartRadvd(void);


/** Stop the radvd daemon on br0.
 * 
 * @return CmsRet enum.
 */
CmsRet rut_stopRadvd(void);


/** Restart radvd daemon if necessary while bridge topology changing
 *
 * @param bridgeName (IN) 
 *
 * @return CmsRet enum.
 */
CmsRet rut_restartRadvdForBridge(const char *bridgeName);

#if defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
/** PURE181, Restart radvd daemon if necessary while bridge topology changing 
 *
 * @param bridgeName (IN) 
 *
 * @return CmsRet enum.
 */
CmsRet rutRa_restartRadvdForBridge(const char *bridgeName);
#endif

/** Return LAN Device IIDStack for the given bridge.
 *
 * @param bridgeIfName  (IN) Bridge If Name.
 * @param iidStack (OUT) InstanceIdStack.
 *
 * @return CmsRet(SUCCESS) if IID stack found for bridge else 
 *         ERROR.
 */
CmsRet rutLan_getLanDevIidStackOfBridge(const char *bridgeIfName, InstanceIdStack *iidStack);

CmsRet rutLan_getHostEntryByMacAddr(const InstanceIdStack *parentIidStack, const char *macAddr, InstanceIdStack *iidStack, LanHostEntryObject **hostEntry);

#ifdef DMP_X_BROADCOM_COM_DLNA_1
/** 
 *  Update the DLNA dms configuration once the associated interface is removed.
 */
CmsRet rutLan_updateDlna(void);
CmsRet rutLan_updateDlna_igd(void);
CmsRet rutLan_updateDlna_dev2(void);

#if defined(SUPPORT_DM_LEGACY98)
#define rutLan_updateDlna()   rutLan_updateDlna_igd()
#elif defined(SUPPORT_DM_HYBRID)
#define rutLan_updateDlna()   rutLan_updateDlna_igd()
#elif defined(SUPPORT_DM_PURE181)
#define rutLan_updateDlna()   rutLan_updateDlna_dev2()
#elif defined(SUPPORT_DM_DETECT)
#define rutLan_updateDlna()   (cmsMdm_isDataModelDevice2() ? \
                                   rutLan_updateDlna_dev2() : \
                                   rutLan_updateDlna_igd())
#endif

#endif
#endif /*__RUT_LAN_H__ */
