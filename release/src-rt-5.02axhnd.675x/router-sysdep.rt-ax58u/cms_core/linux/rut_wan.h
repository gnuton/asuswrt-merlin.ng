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
#ifndef __RUT_WAN_H__
#define __RUT_WAN_H__


/*!\file rut_wan.h
 * \brief System level interface functions for generic WAN functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms_boardcmds.h"
#include "devctl_xtm.h"
#include "cms.h"
#include "rut_wan6.h"


#define INTERFACE_RETRY_COUNT       50
#define USLEEP_COUNT                5000

#ifdef DMP_X_BROADCOM_COM_AUTODETECTION_1
#define STOP_PPPD(n) \
   ((n)->X_BROADCOM_COM_StopPppD == TRUE)
#else
#define STOP_PPPD(n) (FALSE)
#endif

/** wait until the interface is up.
 * 
 * @param ifcName    (IN) interface name.
 * @return BOOL    TRUE is up, FALSE is not up.
 */
UBOOL8 rut_waitIntfExists(const char *ifName);


/** Start the wan network interface and  does ifconfig up if they are vlan and msc
 *
 * @param vlanMuxId         (IN) VlanId (for vlan mux)
 * @param vlanMuxPr         (IN) Vlan Priority (for vlan mux)
 * @param vlanTpid          (IN) Vlan TPID (for vlan mux)
 * @param l2IfName          (IN) Physical interface name
 * @param baseL3IfName      (IN) vlan interface name
 * @param connMode          (IN) Connection mode
 * @param isBridge          (IN) TRUE if bridge wan connection
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_startL3Interface(SINT32 vlanMuxId,
                              SINT32 vlanMuxPr,
                              UINT32 vlanTpid,
                              char * l2IfName,
                              char * baseL3IfName,
                              ConnectionModeType connMode,
                              UBOOL8 isBridge);



/** Activate the IPoE or IPoW interface
 * 
 * @param *newObj       (IN) the new _WanIpConnObject.
 * @return CmsRet enum
 */
CmsRet rutWan_activateIpEthInterface(_WanIpConnObject *newObj);




/** Start a dhcp client on a WAN (IPoE or IPoW) interface.
 * 
 * @param ifName (IN) Name of interface to start the dhcp client on.
 * @param vid (IN) dhcp client option 60 vendor id.
 * @param duid (IN) dhcp client option 61 duid.
 * @param iaid (IN) dhcp client option 61 iaid.
 * @param uid (IN) dhcp client option 77 uid.
 * @param op125 (IN) dhcp client option 125 enable/disable
 * @param ipAddress (IN) dhcp client option 50 request IP address.
 * @param serverIpAddress (IN) dhcp client option 54 request server IP address.
 * @param leasedTime (IN) dhcp client option 51 request leased time.
 * @param op212 (IN) dhcp client option 212 enable/disable
 * 
 * @return pid of the dhcpc process that was started, or CMS_INVALID_PID on error.
 */
SINT32 rutWan_startDhcpc(const char *ifName, const char *vid, const char *duid,
                         const char *iaid, const char *uid, UBOOL8 op125,
                         const char *ipAddress, const char *serverIpAddress,
                         const char *leasedTime, UBOOL8 op212);


/** Get the device information needed for DHCP option 25
 *
 * @param oui          (OUT) ManufacturerOUI, caller must supply buf of at least 65 bytes
 * @param serialNum    (OUT) caller must supply buf of at least 7 bytes
 * @param productClass (OUT) caller must supply buf of at least 65 bytes
 *
 * @return CmsRet
 */
CmsRet rutWan_getDhcpDeviceInfo(char *oui, char *serialNum, char *productClass);
CmsRet rutWan_getDhcpDeviceInfo_igd(char *oui, char *serialNum, char *productClass);
CmsRet rutWan_getDhcpDeviceInfo_dev2(char *oui, char *serialNum, char *productClass);


#if defined(SUPPORT_DM_LEGACY98)
#define rutWan_getDhcpDeviceInfo(o, s, p)   rutWan_getDhcpDeviceInfo_igd((o), (s), (p))
#elif defined(SUPPORT_DM_HYBRID)
#define rutWan_getDhcpDeviceInfo(o, s, p)   rutWan_getDhcpDeviceInfo_igd((o), (s), (p))
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_getDhcpDeviceInfo(o, s, p)   rutWan_getDhcpDeviceInfo_dev2((o), (s), (p))
#elif defined(SUPPORT_DM_DETECT)
#define rutWan_getDhcpDeviceInfo(o, s, p)   (cmsMdm_isDataModelDevice2() ? \
                            rutWan_getDhcpDeviceInfo_dev2((o), (s), (p)) : \
                            rutWan_getDhcpDeviceInfo_igd((o), (s), (p)))
#endif




/** Get interface index by its name.
 * 
 * @param ifName (IN) Name of interface
 *
 * @return interface index
 */
SINT32 rut_wanGetIntfIndex(const char *ifcName) ;

/** Initialize PPPoE interface
 * 
 * @param *iidStack      (IN) iidStack of  ppp object.
 * @param *newObj        (IN) the ppp object.
 *
 * @return CmsRet enum
 */
CmsRet rutWan_initPPPoE(const InstanceIdStack *iidStack, void *newObj);

CmsRet rutWan_initPPPoE_igd(const InstanceIdStack *iidStack, void *newObj);

CmsRet rutWan_initPPPoE_dev2(const InstanceIdStack *iidStack, void *newObj);

#if defined(SUPPORT_DM_LEGACY98)
#define rutWan_initPPPoE(i, o)   rutWan_initPPPoE_igd((i), (o))
#elif defined(SUPPORT_DM_HYBRID)
#define rutWan_initPPPoE(i, o)   rutWan_initPPPoE_igd((i), (o))   //~~~ for Ipv4, use _igd, but ipv6, should be _dev2 ? or use function cmsMdm_isDataModelDevice2 in the code?
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_initPPPoE(i, o)   rutWan_initPPPoE_dev2((i), (o))
#elif defined(SUPPORT_DM_DETECT)
#define rutWan_initPPPoE(i, o)   (cmsMdm_isDataModelDevice2() ? \
                                    rutWan_initPPPoE_dev2((i), (o)) : \
                                    rutWan_initPPPoE_igd((i), (o)))
#endif



/** Clean up PPPoE interface
 * 
 * @param *iidStack      (IN) iidStack of ppp object.
 * @param *currObj       (IN) the curr ppp object.
 *
 * @return CmsRet enum
 */
CmsRet rutWan_cleanUpPPPoE(const InstanceIdStack *iidStack, const void *currObj);
CmsRet rutWan_cleanUpPPPoE_igd(const InstanceIdStack *iidStack, const void *currObj); 
CmsRet rutWan_cleanUpPPPoE_dev2(const InstanceIdStack *iidStack, const void *currObj);

#if defined(SUPPORT_DM_LEGACY98)
#define rutWan_cleanUpPPPoE(i, o)   rutWan_cleanUpPPPoE_igd((i), (o))
#elif defined(SUPPORT_DM_HYBRID)
#define rutWan_cleanUpPPPoE(i, o)   rutWan_cleanUpPPPoE_igd((i), (o))   //~~~ for Ipv4, use _igd, but ipv6, should be _dev2 ?
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_cleanUpPPPoE(i, o)   rutWan_cleanUpPPPoE_dev2((i), (o))
#elif defined(SUPPORT_DM_DETECT)
#define rutWan_cleanUpPPPoE(i, o)   (cmsMdm_isDataModelDevice2() ? \
                                    rutWan_cleanUpPPPoE_dev2((i), (o)) : \
                                    rutWan_cleanUpPPPoE_igd((i), (o)))
#endif





/** Initialize PPP IP Extension.  Remember only one PVC is allowed for ppp ip extension 
 *   and IP extension requires special udhcp configuration and iptables rules and reconfigure
 *   the br0 and etc.
 * 
 * @param *newObj       (IN) the new _WanPppConnObject.
 * @return CmsRet enum
 */
CmsRet rut_startPppIpExtension(const _WanPppConnObject *newObj, const UBOOL8 isPpp);


/** Return TRUE if currently configuration is for EthWan interface.
 */
UBOOL8 rutWan_isEthWanIntf(InstanceIdStack *iidStack);




/** Check if all pvc are bridge protocol.
 *  This is an obsolete function only used by TR98 code.  All new code
 *  should call qdmIpIntf_isAllBridgeWanServiceLocked().
 *
 * @return TRUE - all pvcs are bridges,  FALSE =  found the no bridge pvc.
 */
UBOOL8 rutWan_isAllBridgePvcs(void);
UBOOL8 rutWan_isAllBridgePvcs_igd(void);

#define rutWan_isAllBridgePvcs()   rutWan_isAllBridgePvcs_igd()




/** check if the interface is up or not
 * 
 * @param ifcName            (IN) the inteface name
 * @param isIPv4              (IN) indication of IPv4 or IPv6
 * @return  TRUE --  interface is up.  FALSE -- interface is down
 */
UBOOL8 rut_isWanInterfaceUp(const char *ifcName, UBOOL8 isIPv4);




/** Return the number of IP or PPP connections in the connected state under this WANDevice.
 *
 * @param iidStack (IN) - The WANDevice we are talking about.
 *
 * @return the number of IP or PPP connections in the connected state under this WANDevice.
 */
UINT16 rutWan_getNumberOfActiveConnections(const InstanceIdStack *iidStack);


/** For ppp ip extenstion and ppp ip extension/Dynamic IPoE with advanced dmz
 * to set up relay lan
 *
 * @param ifcName       (IN) the WAN inteface name
 * @param defGw         (IN) the default gateway
 * @param externIp      (IN) Wan Ip address
 * @param isPpp         (IN) If TRUE, it is ppp, else dynamic IPoE
 *
 * @return CmsRet enum
 */
CmsRet rutWan_IpExtensionRelay(const char *ifcName, const char *defGw, const char *externIp, const UBOOL8 isPpp);



/** Send CMS_MSG_WAIT_FOR_DSL_LINK_UP request to ssk
 * to update the WAN connectionStatus for the wan object
 *
 * @param Oid(IN)  oid of the wan object
 * @param iidStack (IN)  iidStack of the wan object
 * @param isAddPvc (IN)  TRUE for add, FALSE for del
 * @param isStatic (IN)  If TRUE, it is is bridge, static IPoE and IPoA, FALSE: pppox or dynamic IPoE
 * @param isDeleted (IN)  If TRUE, the WAN interface is deleted.
 * @param isAutoDetectChange (IN)  If TRUE, the auto detect enable flag changed
 
 *
 */
void rutWan_sendConnectionUpdateMsg(const MdmObjectId oid, 
                                    const InstanceIdStack *iidStack,
                                    const UBOOL8 isAddPvc,
                                    const UBOOL8 isStatic,
                                    const UBOOL8 isDeleted,
                                    const UBOOL8 isAutoDetectChange);


/** Get MTU information of the interface
 *
 * @param domain    (IN) communication domain, either PF_INET or PF_INET6.
 * @param ifname    (IN) Interface name
 *
 */
SINT32 rutWan_getInterfaceMTU(SINT32 domain, const char *ifname);



/**  Get the next available ppp or pppoa interface name, e.g., ppp0, pppoa1, ppp2 ...
 *
 * @param isPPPoE (IN)  TRUE == PPPoE. FALSE == PPPoA
 * @param pppName (OUT) The next ppp name.
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_fillPppIfName(UBOOL8 isPPPoE, char *pppName);


/** Get the number of used hardware transmit queues left in the pool.
 *
 * @param wanType     (IN) wan link type, either ATM or PTM.
 * @param usedQueues  (OUT) the number of used queues.
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_getNumUsedQueues(WanLinkType wanType, UINT32 *usedQueues);


/** Get the number of unused hardware transmit queues left in the pool.
 *
 * @param wanType       (IN) wan link type, either ATM or PTM.
 * @param unusedQueues  (OUT) the number of unused queues.
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_getNumUnusedQueues(WanLinkType wanType, UINT32 *unusedQueues);


/** Do a check before remove dns and default gateway entry.  Called whne the wan connection
 *  is being removed,
 *
 * @param ifName        (IN) the removed wan interface name 
 * @param DNSServers    (IN) the removed wan DSNServers (only for static dns entry removal)
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_removeDnsAndDefaultGatewayIfUsed(const char *ifName, const char *DNSServers);


 
/** This function creates and fills the WAN layer 3 interface name and service name from either 
 * WANIP/PPPConnection Object.  Uses GET_MDM_OBJECT_ID on newObj to find out IP or PPP.
 * It will use iidStack to get the underline layer 2 link config object for the layer 2inteface name and connMode.
 * This function should not fail normally unless the layer 2 link config information is invalid.  
 * If failed, the function will return CMSRET_INVALID_ARGUMENTS and if the connectionStatus is "Unconfigured", the
 * WanIP/PPPConnection object will be removed and rcl_wanIp/PPPConnectionObject will return CMSRET_SUCCESS so
 * that system will not reboot.
 *
 * @param wanConnObj (IN/OUT) newObj->X_BROADCOM_COM_IfName and newObj->name will be filled with
 *                            correct names if they are NULL. 
 * @param iidStack   (IN) iidStack of the WanIPConnObject or WanPPPconnObject If this is belong toThis iidStack is
 *                        used to find the ancestor WanDslLinkCfg object what the linkType is
 *
 * @return CmsRet enum.
 */
 
CmsRet rutWan_fillWanL3IfNameAndServiceName(void *wanConnObj,
                                            const InstanceIdStack *iidStack);



/**  Get the available conId from portId/Vpi/Vci 
 */
CmsRet rutWan_getAvailableConIdForMSC(const char *wanL2IfName, SINT32 *outConId);



/** check status of interface.
 * 
 * @param ifcName    (IN) interface name.
 * @return BOOL    TRUE is up, FALSE is down.
 */
UBOOL8 rut_checkInterfaceUp(const char *ifcName);




/** Move Ethernet interface from LAN to WAN
 *
 */
void rutWan_moveEthLanToWan(const char *ifName);
void rutWan_moveEthLanToWan_igd(const char *ifName);
void rutEth_moveEthLanToWan_dev2(const char *ifName);

#if defined(SUPPORT_DM_LEGACY98)
#define rutWan_moveEthLanToWan(i)       rutWan_moveEthLanToWan_igd((i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutWan_moveEthLanToWan(i)       rutWan_moveEthLanToWan_igd((i))
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_moveEthLanToWan(i)       rutEth_moveEthLanToWan_dev2((i))
#elif defined(SUPPORT_DM_DETECT)
#define rutWan_moveEthLanToWan(i)   (cmsMdm_isDataModelDevice2() ? \
                             rutEth_moveEthLanToWan_dev2((i)) : \
                             rutWan_moveEthLanToWan_igd((i)))
#endif


/** Move Ethernet interface from WAN to LAN
 *
 */
void rutWan_moveEthWanToLan(const char *ifName);
void rutWan_moveEthWanToLan_igd(const char *ifName);
void rutEth_moveEthWanToLan_dev2(const char *ifName);

#if defined(SUPPORT_DM_LEGACY98)
#define rutWan_moveEthWanToLan(i)       rutWan_moveEthWanToLan_igd((i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutWan_moveEthWanToLan(i)       rutWan_moveEthWanToLan_igd((i))
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_moveEthWanToLan(i)       rutEth_moveEthWanToLan_dev2((i))
#elif defined(SUPPORT_DM_DETECT)
#define rutWan_moveEthWanToLan(i)   (cmsMdm_isDataModelDevice2() ? \
                             rutEth_moveEthWanToLan_dev2((i)) : \
                             rutWan_moveEthWanToLan_igd((i)))
#endif




void rutWan_moveMocaLanToWan(const char *ifName);

void rutWan_moveMocaWanToLan(const char *ifName);




/** Find the WANIPConnection or WANPPPConnection object with the
 * given ifName.
 *
 * @param ifName    (IN) the (L3) interface name to search for.
 * @param iidStack (OUT) if found, the iidStack of the found object.
 * @param obj      (OUT) if found, the WANIPConnection or WANPPPConnection object.
 *                       the caller is responsible for freeing this buffer.
 * @param isPpp    (OUT) if found, this is set to true if the returned object
 *                       is a WANPPPConnection object.  Otherwise, it is a 
 *                       WANIPConnection object.
 *
 * @return CMSRET_SUCCESS if the object was found.
 */
CmsRet rutWan_getIpOrPppObjectByIfname(const char *ifName, InstanceIdStack *iidStack, void **obj, UBOOL8 *isPpp);




/** This function returns the L3 interface name of the first routed and
 *  connected IPv4 WAN connection.  Note this function looks at IPv4
 *  connection status only, not IPv6 connection status.  For IPv4/IPv6
 *  functionality, use rutWan_findFirstIpvxRoutedAndConnected().
 *
 * @param ifName   (OUT) if found, this buffer will contain the WAN L3 interface name
 *                       of an IPv4 routed and connected connection.  This
 *                       must be a buffer of CMS_IFNAME_LENGTH or longer.
 *
 * @return TRUE if an IPv4 routed and connected connection was found.
 */
UBOOL8 rutWan_findFirstRoutedAndConnected(char *ifName);
UBOOL8 rutWan_findFirstRoutedAndConnected_igd(char *ifName);
UBOOL8 rutWan_findFirstRoutedAndConnected_dev2(char *ifName);

#if defined(SUPPORT_DM_LEGACY98)
#define rutWan_findFirstRoutedAndConnected(i)   rutWan_findFirstRoutedAndConnected_igd((i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutWan_findFirstRoutedAndConnected(i)   rutWan_findFirstRoutedAndConnected_igd((i))
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_findFirstRoutedAndConnected(i)   rutWan_findFirstRoutedAndConnected_dev2((i))
#elif defined(SUPPORT_DM_DETECT)
#define rutWan_findFirstRoutedAndConnected(i)   (cmsMdm_isDataModelDevice2() ? \
                             rutWan_findFirstRoutedAndConnected_dev2((i)) : \
                             rutWan_findFirstRoutedAndConnected_igd((i)))
#endif


/** Find the first IPv4/IPv6 routed and connected L3 interface.
 *
 *  @param ipvx  (IN) If CMS_AF_SELECT_IPV4, look for IPv4 interfaces that are UP & routed.
 *                    If CMS_AF_SELECT_IPV6, look for IPv6 interfaces that are UP & routed.
 *                    If CMS_AF_SELECT_IPVX, look for IPv6 interfaces that are UP & routed first,
 *                    and if not found, try IPv4 interfaces.
 *  @param intfName (OUT) Caller must supply a buffer of at least
 *                        CMS_IFNAME_LENGTH bytes.  On success, will contain
 *                        the Linux intf name of UP & routed interface.
 *
 *  @return TRUE if found.
 */
UBOOL8 rutWan_findFirstIpvxRoutedAndConnected(UINT32 ipvx, char *intfName);
UBOOL8 rutWan_findFirstIpvxRoutedAndConnected_igd(UINT32 ipvx, char *intfName);
UBOOL8 rutWan_findFirstIpvxRoutedAndConnected_dev2(UINT32 ipvx, char *intfName);

#if defined(SUPPORT_DM_LEGACY98)
#define rutWan_findFirstIpvxRoutedAndConnected(v, i)   rutWan_findFirstIpvxRoutedAndConnected_igd((v), (i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutWan_findFirstIpvxRoutedAndConnected(v, i)   rutWan_findFirstIpvxRoutedAndConnected_igd((v), (i))
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_findFirstIpvxRoutedAndConnected(v, i)   rutWan_findFirstIpvxRoutedAndConnected_dev2((v), (i))
#elif defined(SUPPORT_DM_DETECT)
#define rutWan_findFirstIpvxRoutedAndConnected(v, i)   (cmsMdm_isDataModelDevice2() ? \
                         rutWan_findFirstIpvxRoutedAndConnected_dev2((v), (i)) : \
                         rutWan_findFirstIpvxRoutedAndConnected_igd((v), (i)))
#endif




/** Find the first NAT'ed and connection L3 WAN service (for UPnP).
 *  This function is for IPv4 only.
 *
 * @param natIntfName (OUT) first NAT'd WAN connection found.  Caller must
 *                          provide a buffer of at least CMS_IFNAME_LENGTH bytes.
 * @param excludeIntfName (IN) exclude this intfname.
 *
 * @return TRUE if found.
 */
UBOOL8 rutWan_findFirstNattedAndConnected(char *natIntfname, const char *excludeIntfName);
UBOOL8 rutWan_findFirstNattedAndConnected_igd(char *natIntfName, const char *excludeIntfName);
UBOOL8 rutWan_findFirstNattedAndConnected_dev2(char *natIntfName, const char *excludeIntfName);

#if defined(SUPPORT_DM_LEGACY98)
#define rutWan_findFirstNattedAndConnected(v, i)   rutWan_findFirstNattedAndConnected_igd((v), (i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutWan_findFirstNattedAndConnected(v, i)   rutWan_findFirstNattedAndConnected_igd((v), (i))
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_findFirstNattedAndConnected(v, i)   rutWan_findFirstNattedAndConnected_dev2((v), (i))
#elif defined(SUPPORT_DM_DETECT)
#define rutWan_findFirstNattedAndConnected(v, i)   (cmsMdm_isDataModelDevice2() ? \
                         rutWan_findFirstNattedAndConnected_dev2((v), (i)) : \
                         rutWan_findFirstNattedAndConnected_igd((v), (i)))
#endif




/** This function will fill the index array with the  max. allowed interfaces (#define IFC_WAN_MAX, in cms.h)
*  can be configured for ppp interfaces in the system.
 *
 * @param intfArray   (IN/OUT) Initially intfArray is set to 0, and when return all pppX interfaces
 *                             in the system will be marked to 1 for finding out the next ppp index to use.
 *                
 */
CmsRet rutWan_fillInPPPIndexArray(SINT32 *intfArray);
CmsRet rutWan_fillInPPPIndexArray_igd(SINT32 *intfArray);
CmsRet rutWan_fillInPPPIndexArray_dev2(SINT32 *intfArray);

#if defined(SUPPORT_DM_LEGACY98)
#define rutWan_fillInPPPIndexArray(i)   rutWan_fillInPPPIndexArray_igd((i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutWan_fillInPPPIndexArray(i)   rutWan_fillInPPPIndexArray_igd((i))
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_fillInPPPIndexArray(i)   rutWan_fillInPPPIndexArray_dev2((i))
#elif defined(SUPPORT_DM_DETECT)
#define rutWan_fillInPPPIndexArray(i)   (cmsMdm_isDataModelDevice2() ? \
                             rutWan_fillInPPPIndexArray_dev2((i)) : \
                             rutWan_fillInPPPIndexArray_igd((i)))
#endif





/** Get the external ip address from the only one PPP connection is the
* system.  The externalIPAddress will be used for min/max in udncpd.conf
* @param ipAddress (OUT) The ipAddress that was found. 
*
* @return CMSRET_SUCCESS if an ipAddress was found.  CMSRET_OBJECT_NOT_FOUND if
*         ipAddress was not found.  
*/
CmsRet rutWan_getIpExIpAddress(char *ipAddress);


/** Send request message to ssk to check wan link status.
 *
 */
void rutWan_sendWanLinkCheckMessage(void);


/** Get layer 2 interface name, baseL3IfName and connMode for starting EoA type
 * WAN layer 3 servcie.  Call by rutWan_startL3Interface.
 *
 * @param wanConnOid       (IN) oid is either MDMOID_WAN_PPP_CONN or MDMOID_WAN_IP_CONN
 * @param iidStack         (IN) iidStack of the WanIP/PPPConnObject.  This iidStack is
 *                              used to find the ancestor WanLinkCfg object  
 * @param vlanMuxId        (IN) vlan mux id.
 * @param conId            (IN) connection id for MSP 
 * @param L3IfName         (IN) layer 3 interface name
 * @param l2IfName         (OUT) layer 2 interface name
 * @param baseL3IfName     (OUT) base layer 3 interface name
 * @param connMode         (OUT) Connection Mode
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_getL3InterfaceInfo(MdmObjectId wanConnOid, 
                                 const InstanceIdStack *iidStack,
                                 SINT32 vlanMuxId,
                                 SINT32 connId,
                                 const char *L3IfName,                              
                                 char *l2IfName,
                                 char *baseL3IfName,
                                 ConnectionModeType *connMode);

  

/** Stop EoA type WAN layer 3 servcie. Called by both IPoE and PPPoE WAN connection.
 *
 * @param wanConnOid       (IN) oid is either MDMOID_WAN_PPP_CONN or MDMOID_WAN_IP_CONN
 * @param iidStack         (IN) iidStack of the WanIP/PPPConnObject.  This iidStack is
 *                              used to find the ancestor WanLinkCfg object  
 * @param vlanMuxId        (IN) vlan mux id.
 * @param conId            (IN) connection id for MSP 
 * @param L3IfName         (IN) layer 3 interface name
 *
 * @return CmsRet enum.
 */                               
CmsRet rutWan_stopL3Interface(MdmObjectId wanConnOid, 
                              const InstanceIdStack *iidStack,
                              SINT32 vlanMuxId,
                              SINT32 connId,
                              const char *L3IfName);                                 


/** After a routed WAN connection is up, redo the sysetm default gateway and dns
 * since the configuration might be changed.
 *
 * @param  isIPv4      (IN) indication of IPv4 or IPv6
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_doDefaultSystemGatewayAndDNS(UBOOL8 isIPv4);


/** When WAN IP or PPP interface is removed, need to perform some removal
 * on related objects
 *
 * @param  ifName      (IN) WAN interface name
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_deleteWanIpOrPppConnection(const char *ifName);


/** Start PPPoA Wan Connection
 *
 * @param newObj   (IN)  WanPpp Connection object.
 * @param iidStack (IN) iidStack of Object
 * @return CmsRet enum.
 */
CmsRet rutWan_startPPPoA(const InstanceIdStack *iidStack, void *newObj);


/** Start IPoA Wan Connection
 *
 * @param newObj   (IN)  WanIp Connection object.
 * @param iidStack (IN) iidStack of the WanIPConnObject or WanPPPconnObject If this is belong toThis iidStack is
 *                        used to find the ancestor WanDslLinkCfg object what the linkType is
 * @return CmsRet enum.
 */

CmsRet rutWan_startIPoA(const InstanceIdStack *iidStack, _WanIpConnObject *newObj);

/** Check if the modem is configured for ppp ip extension.
 *  NOTE: If it is ppp ip extension, it should be only pvc in the modem
 *
 * @param (IN) void
 * @return BOOL
 */
UBOOL8 rutWan_IsPPPIpExtension(void);


/** Convert the  the input ip address and mask string to output broadcast and subnet strings 
 *
 * @param (IN) InIpAddressStr    IP address string
 * @param (IN) InMaskStr         IP Mask string
 * @param (OUT) outBCastStr      broadcast string   
 * @param (OUT) outSubnetStr     subnet string
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_getBcastStrAndSubnetFromIpAndMask(char *InIpAddressStr, 
                                                char *InMaskStr,
                                                char *outBCastStr,
                                                char *outSubnetStr);

                                                
/** Clear the WAN interface ip address with "ifconfig WanfName 0.0.0.0" command.
 *  This is used when the dhcp server is down and the dncp lease expires on the modem
 *  and the modem layer 3 configuration is teared down (Connectionstatus is
 *  from "Connected" to "Connecting") and, wan interface ip has to be cleared to prevent 
 *  the lan traffic passing through this wan interface.
 *
 * @param ifName     (IN) the inteface name of the WAN connection
 *
 * @return  NONE
 */
void rutWan_clearWanConnIpAdress(const char *ifName);


/** This is the transient layer 2 link status used only in the case of layer2 link up or down and during the set oporation on 
 * WanIP/PPP connection object so that the operation on the set can be carried out disregarding the real layer 2 link status.
 * NOTE: This call should be use only inside the rcl_wanIpConnObject or rcl_wanPPPConnObject functions.
 *
 * @param transientL2LinkStatus     (IN) the current layer 2 link status when the cmsObj_set is called
 *
 * @return  TRUE or FALSE
 */
UBOOL8 rutWan_isTransientLayer2LinkUp(const char *transientL2LinkStatus);

/** This is the action function to launching pppd 
 *  and is called by both  rutWan_initPPPoE(TR98) , and rutWan_initPPPoE_dev2(TR181)
 *
 * @param cmdLine          (IN) the launching command string
 * @param baseL3IfName     (IN) the input interface name (eg. ptm0.1)
 * @param addPppToBridge   (IN) if TRUE, the interface will be added to "br0"
 * @param pppPid           (OUT) the returned pid operation succeed
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_configPPPoE(const char *cmdLine, const char *baseL3IfName, UBOOL8 addPppToBridge, SINT32 *pppPid);

/** This function reads the proc entry which indicates the optical wan type. Note that
 *  the proc entry also exists for non PON designs. 
 *
 * @param optIfWanType  (OUT) returns optical wan type, 'None' if unsuccessful
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_getOpticalWanType(char * optIfWanType);

#ifdef DMP_DEVICE2_OPTICAL_1
UBOOL8 rutOptical_getIntfByIfName(const char *ifName, InstanceIdStack *iidStack, OpticalInterfaceObject **optIntfObj);
UBOOL8 rutOptical_getIntfByIfNameEnabled(const char *ifName, InstanceIdStack *iidStack, OpticalInterfaceObject **optIntfObj, UBOOL8 enabled);
#endif /* DMP_DEVICE2_OPTICAL_1 */

#endif /* __RUT_WAN_H__ */
