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

#ifndef __RUT_PMAP_H__
#define __RUT_PMAP_H__


/*!\file rut_pmap.h
 * \brief System level interface functions for port mapping/layer 2 bridging functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"



/** Create a new Layer 2 bridging filter interface object with the given name.
 * 
 * This is a convenience function made available to other RCL handler functions.
 * The bridging RCL handler functions do not use this.
 * 
 * @param ifName (IN) name of the filter interface.
 * @param isWanIntf (IN) True if this interface is a WAN interface.
 * @param bridgeRef (IN) Bridge ref of the filter interface.
 * 
 * @return CmsRet enum.
 */     
CmsRet rutPMap_addFilter(const char *ifName, UBOOL8 isWanIntf, SINT32 bridgeRef);


/** Find the Layer 2 bridging filter object with the specified FilterInterface name.
 * 
 * This function is also called by the DAL.
 * 
 * @param filterInterface (IN) The filter interface to look for.
 * @param iidStack (OUT) If found, this iidStack will be filled out with
 *                       the instance info for the found obj.
 * 
 * @param filterIntfObj (OUT) If found, this pointer will be set to the
 *                L2BridgingFilterObject.
 * 
 * @return CmsRet enum.
 */
CmsRet rutPMap_getFilter(const char *filterInterface, InstanceIdStack *iidStack, L2BridgingFilterObject **filterObj);


/** Delete the filter with the specified filterInterface.
 * 
 * This is a convenience function made available to other RCL handler functions.
 * The bridging RCL handler functions do not use this.
 * 
 * @param availInterfaceReference (IN) The fullpath name of the filter interface to delete.
 * @param isWanIntf (IN) True if this interface is a WAN interface.
 */
void rutPMap_deleteFilter(const char *availInterfaceReference, UBOOL8 isWanIntf);


/** Add an available interface with the specified name.
 * 
 * This is a convenience function made available to other RCL handler functions.
 * The bridging RCL handler functions do not use this.
 * 
 * @param ifName (IN) name of the available interface.
 * @param isWanIntf (IN) True if this interface is a WAN interface.
 * 
 * @return CmsRet enum.
 */
CmsRet rutPMap_addAvailableInterface(const char *ifName, UBOOL8 isWanIntf);


/** Find the Layer 2 bridging available interface object with the specified
 *  InterfaceReference parameter.
 * 
 * This function is also used by the DAL.
 * 
 * @param availInterfaceReference (IN) Value of the InterfaceReference
 *                       parameter in the AvailableInterface object. 
 * @param iidStack (OUT) If found, this iidStack will be filled out with
 *                       the instance info for the found obj.
 * @param availIntfObj (OUT) If found, this pointer will be set to the
 *                L2BridgingIntfObject.  The caller is responsible for freeing
 *                the object.
 * 
 * @return CmsRet enum.
 */
CmsRet rutPMap_getAvailableInterfaceByRef(const char *availInterfaceReference, InstanceIdStack *iidStack, L2BridgingIntfObject **availIntfObj);


/** Find the Layer 2 bridging available interface object with the specified
 *  AvailableInterfaceKey.
 * 
 * This function is also used by the DAL.
 * 
 * @param availInterfaceKey (IN) Value of the AvailableInterfaceKey
 *                       parameter in the AvailableInterface object. 
 * @param iidStack (OUT) If found, this iidStack will be filled out with
 *                       the instance info for the found obj.
 * @param availIntfObj (OUT) If found, this pointer will be set to the
 *                L2BridgingIntfObject.  The caller is responsible for freeing
 *                the object.
 * 
 * @return CmsRet enum.
 */
CmsRet rutPMap_getAvailableInterfaceByKey(UINT32 availableInterfaceKey, InstanceIdStack *iidStack, L2BridgingIntfObject **availIntfObj);


/** Delete the specified available interface.
 * 
 * This is a convenience function made available to other RCL handler functions.
 * The bridging RCL handler functions do not use this.
 * 
 * @param availableInterfaceReference (IN) name of the available interface.
 * @param isWanIntf (IN) True if this interface is a WAN interface.
 */
void rutPMap_deleteAvailableInterface(const char *availableInterfaceReference, UBOOL8 isWanIntf);


/** Convert a WAN IfName to the full param name (IP/PPP connection objects) in the
 *  format required by the InterfaceReference parameter name of the AvailableInterface object.
 * 
 * This function is also used by the DAL.
 * 
 * @param ifName (IN) name of the available interface.
 * @param availInterfaceReference (OUT) This must be a buffer of 256 bytes long.
 *                    On success, this buffer will be filled with the full param name.
 * 
 * @return CmsRet enum.
 */
CmsRet rutPMap_wanIfNameToAvailableInterfaceReference(const char *wanIfName, char *availableInterfaceReference);


/** Convert a LAN IfName to the full param name in the format required
 * by the InterfaceReference parameter name of the AvailableInterface object.
 * 
 * This function is also used by the DAL.
 * 
 * @param ifName (IN) name of the available interface.
 * @param availInterfaceReference (OUT) This must be a buffer of 256 bytes long.
 *                    On success, this buffer will be filled with the full param name.
 * 
 * @return CmsRet enum.
 */
CmsRet rutPMap_lanIfNameToAvailableInterfaceReference(const char *lanIfName, char *availableInterfaceReference);


/** Given the InterfaceReference parameter in the available interface object
 *  return the corresponding object and its iidStack.
 * 
 * @param availableInterfaceReference (IN) The interface reference parameter value 
 *               in the available interface object.
 * @param iidStack (OUT) The iidStack of the returned MdmObject.  This pointer
 *                       must not be NULL.
 * @param obj    (OUT) The MdmObject corresponding to the interface reference.
 *                     The caller is responsible for freeing this object.
 *                     This pointer must not be NULL.
 * 
 * @return CmsRet enum
 */
CmsRet rutPMap_availableInterfaceReferenceToMdmObject(const char *availableInterfaceReference, InstanceIdStack *iidStack, void **obj);


/** Convert the InterfaceReference parameter in the available interface object
 *  to the Linux interface name.
 * 
 * This function is also used by the DAL.
 * 
 * @param availableInterfaceReference (IN) The interface reference parameter value 
 *               in the available interface object.
 * @param ifName (OUT) This buffer must be at least 32 bytes long.  On successful
 *               return, this will contain the Linux ifName associated with
 *               the availableInterfaceReference.
 * 
 * @return CmsRet enum
 */
CmsRet rutPMap_availableInterfaceReferenceToIfName(const char *availableInterfaceReference, char *ifName);


/** Convert the FilterInterface parameter in the filter object to the
 *  Linux interface name.
 * 
 * @param filterInterface (IN) the filterInterface parameter of the filter object.
 * @param ifName (OUT) On successful return, this buffer will contain the
 *                     Linux interface name referenced by the filter reference.
 *                     This buffer should be at least 32 bytes long.
 * 
 * @return CmsRet enum.
 */
CmsRet rutPMap_filterInterfaceToIfName(const char *filterInterface, char *ifName);


/** Given a linux ifname, return the corresponding Filter object and iidStack.
 *
 * This works for both LAN and WAN ifNames.
 * 
 * @param ifName (IN) The ifName.
 * @param iidStack (OUT) On successful return, this contains the iidStack of the
 *                       filter object.
 * @param filterObj(OUT) On successful return, this contains the filter object.
 *                       The caller is responsible for freeing it.
 * 
 * @return CmsRet enum.
 */
CmsRet rutPMap_ifNameToFilterObject(const char *ifName, InstanceIdStack *iidStack, L2BridgingFilterObject **filterObj);


/** Find the Layer 2 bridging entry object with the given bridge key.
 * 
 * This function is also used by the DAL.
 * 
 * @param bridgeKey (IN) bridge key.
 * @param iidStack (OUT) If found, this iidStack will be filled out with
 *                       the instance info for the found obj.
 * @param bridgeObj (OUT) If found, this pointer will be set to the
 *                        L2BridgingEntryObject.  Caller is responsible for
 *                        freeing this object.
 * 
 * @return CmsRet enum.
 */
CmsRet rutPMap_getBridgeByKey(UINT32 bridgeKey,
                              InstanceIdStack *iidStack,
                              L2BridgingEntryObject **bridgeObj);
                              

/** Find the Layer 2 bridging entry object with the given bridge ifName.
 * 
 * This function is also used by the DAL.
 * 
 * @param bridgeName (IN) bridge IfName.
 * @param iidStack (OUT) If found, this iidStack will be filled out with
 *                       the instance info for the found obj.
 * @param bridgeObj (OUT) If found, this pointer will be set to the
 *                        L2BridgingEntryObject.  Caller is responsible for
 *                        freeing this object.
 * 
 * @return CmsRet enum.
 */
CmsRet rutPMap_getBridgeByName(const char *bridgeName, 
                               InstanceIdStack *iidStack, 
                               L2BridgingEntryObject **bridgeObj);

                              
CmsRet rutPMap_setLanBridgeEnable(const char *bridgeIfName, UBOOL8 enable);


/** Return true if the FilterInterface parameter of the filter object
 *  points to a LAN interface.
 *
 * @return TRUE if the filter interface points to a LAN interface.
 */                  
UBOOL8 rutPMap_isLanInterfaceFilter(const char *filterInterface);


/** Return true if the FilterInterface parameter of the filter object
 *  points to a WAN interface.
 *
 * @return TRUE if the filter interface points to a WAN interface.
 */
UBOOL8 rutPMap_isWanInterfaceFilter(const char *filterInterface);


/** Move a LAN interface from one bridge to another.
 * 
 * @param filterIntf (IN) interface to be moved.
 * @param srcBridgeRef (IN) Bridge that the interface is currently in.
 * @param destBridgeRef (IN) Bridge that the interface is going to move to.
 * 
 * @return CmsRet enum. 
 */
CmsRet rutPMap_moveLanInterface(const char *filterInterface, SINT32 srcBridgeRef, SINT32 destBridgeRef);

CmsRet rutPMap_associateWanIntfWithBridge(const char *filterInterface, SINT32 bridgeRef);

CmsRet rutPMap_disassociateWanIntfFromBridge(const char *filterInterface, SINT32 bridgeRef, UBOOL8 isEditWanFilter);      

CmsRet rutPMap_associateDhcpVendorIdWithBridge(const char *dhcpVendorIds, SINT32 bridgeRef);

CmsRet rutPMap_disassociateDhcpVendorIdFromBridge(const char *dhcpVendorIds, SINT32 bridgeRef);

/** Return TRUE if this WAN service/connection is used in an interface group
 * 
 * We say that a WAN service/connection is used in an interface group if
 * it is grouped with a bridge other than br0.  
 *
 * @param ifName (IN) Name of the WAN service/connection, e.g. ppp0.
 * 
 * @return TRUE if this WAN service/connection is used in an interface group.
 */
UBOOL8 rutPMap_isWanUsedForIntfGroup(const char *ifName);


/** Return TRUE if this WAN service/connection is used in an interface group
 * 
 * We say that a WAN service/connection is used in an interface group if
 * it is grouped with a bridge other than br0.  
 *
 * @param availableInterfaceReference (IN) Full pathname of the WAN service/connection,
 *                  without the trailing "." so that it can be used to do matching
 *                  on the AvailableInterfaceObject->InterfaceReference parameter,
 *                  e.g. InternetGatewayDevice.WANDevice.2.WanConnectionDevice.1.WANIPConnection.1
 * 
 * @return TRUE if this WAN service/connection is used in an interface group.
 */
UBOOL8 rutPMap_isWanUsedForIntfGroupFullPathName(const char *availableInterfaceReference);


/** Find out the bridgeIfName (eg. br1) from the interface
 * @param ifName (IN) WAN interface name 
 * @param bridgeIfName (OUT) the bridge interface name
 * @param isWan  (IN) indicate LAN/WAN interface
 * 
 * @return CmsRet enum. 
 */
CmsRet rutPMap_getBridgeIfNameFromIfName(const char *ifName, char *bridgeIfName, UBOOL8 isWan);


/** Configure (action only) the policy routing on the wan interface if it is used in the routed interface group.
 *   
 * 
 * @param add (IN) If TRUE, add, else delete
 * @param wanIfName (IN) wan interface name
 * @param defaultGateway (IN) wan ifc default gateway
 * 
 * @return CmsRet enum. 
 */
CmsRet rutPMap_configPolicyRoute(const UBOOL8 add, const char *ifName, const char *defaultGateway);


/** Rmove the interface group entry if the wan interface is part of it.
 * 
 * @param fullPathName (IN) full path name for this wan interface
 * 
 * @return none 
 */
 void rutPMap_removeInterfaceGroup(const char *fullPathName);

/** Check if the bridge belongs to an interface group with a bridged PVC
 * 
 * @param BridgeKey (IN)
 * 
 * @return boolean
 */
UBOOL8 rutPMap_isBridgedInterfaceGroup(UINT32 BridgeKey);
 
 /** Get the subnet for a routed wan interface group
 * 
 * @param wanIfName (IN) wan interface name
 * @param subnetCidr (OUT) the subnet associated with this interface group in cidr format
 * 
 * @return CmsRet enum. 
 */
CmsRet rutPMap_getSubnetFromWanIfName(const char *ifName, char *subnetCidr);

 /** Get bridgeName (br0,..) and groupName from a wan interface name 
 * 
 * @param wanIfName (IN) wan interface name
 * @param groupName (OUT) the groupName associated with this interface group
 * @param groupName (OUT) the bridgeName associated with this interface group
 * 
 * @return CmsRet enum. 
 */
CmsRet rutPMap_getGroupAndBridgeNameFromWanIfName(const char *wanIfName, char *groupName,  char *bridgeName);

#ifdef DMP_BRIDGING_1
 /** Get IGMP snooping status for a bridged wan interface 
 * 
 * @param wanIfName (IN) wan interface name
 * 
 * @return UBOOL8 - TRUE (if snooping is enabled) else FALSE. 
 */
UBOOL8 rutPMap_getIgmpSnoopingForBridgedWanIf(const char *wanIfName);

void rutPMap_deleteFilterWithVlan(char *l2IfName, UINT32 vid);
#endif /* DMP_BRIDGING_1 */

#if defined(DMP_BRIDGING_1) || defined(DMP_DEVICE2_BRIDGE_1) /* aka SUPPORT_PORT_MAP */
CmsRet rutPMap_doPoliceRoutingOnWanIfc(const UBOOL8 add,
                                       const char *wanIfName, 
                                       const char *defaultGateway,
                                       const char *bridgeIfName,
                                       const char *intfGroupName);
#endif /* DMP_BRIDGING_1 || DMP_DEVICE2_BRIDGE_1 */

#ifdef SUPPORT_LANVLAN
CmsRet rutPMap_getFilterWithVlan(const char *filterInterface, SINT32 VLANIDFilter, InstanceIdStack *iidStack, L2BridgingFilterObject **filterObj);
#endif

CmsRet rutPMap_getBridgeKey(const char *bridgeName, UINT32 *bridgeKey);
CmsRet rutPMap_disassocAllFilterIntfFromBridge(const char *bridgeName);
void rutPMap_deleteBridge(const char *bridgeName);

#endif // __RUT_PMAP_H__

