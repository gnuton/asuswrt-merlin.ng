/***********************************************************************
 *
 *  Copyright (c) 2008  Broadcom Corporation
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

#ifndef __RUT_UTIL_H__
#define __RUT_UTIL_H__


/*!\file rut_util.h
 * \brief random assortment of useful system level interface functions.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */



#include "cms.h"
#include "cms_msg.h"
#include "mdm_object.h"

/** Check to see if both new and current objects are NULL.
 *
 * @param newObj (IN)   New object
 * @param currObj (IN)  Current object
 * @return CmsRet       enum.
 *
 */
CmsRet rut_validateObjects(const void *newObj, const void *currObj);


/** Return a broadcast ip string from input ip and subnetmask string
 *
 * @param inIpStr          (IN)  Input ip string
 * @param inSubnetMaskStr  (IN)  Input subnet mask string
 * @param outBcastStr      (OUT) broadcast ip string
 * @return CmsRet          enum.
 *
 */
CmsRet rut_getBCastFromIpSubnetMask(char* inIpStr, char* inSubnetMaskStr, char *outBcastStr);



/** Generic function to change the NumberOfEntries field in the parent object.
 *
 * @param ancestorOid  (IN) The ancestor oid which contains the NumberOfEntries parameter.
 * @param decendednOid (IN) The oid which is being created or deleted.
 * @param iidStack (IN) iidStack of the object being created or deleted.
 * @param delta (IN)    The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumGeneric(MdmObjectId ancestorOid,
                          MdmObjectId decendentOid,
                          const InstanceIdStack *iidStack,
                          SINT32 delta);


/** Change the ManageableDeviceNumberOfEntries field in the ManagementServer object.
 *
 * @param iidStack (IN) iidStack of the ManageableDevice object.  This iidStack is
 *                      used to find the ancestor ManagementServer object.
 * @param delta (IN)    The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumManageableDevices(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the LANDeviceNumberOfEntries field in the InternetGatewayDevice object.
 *
 * @param iidStack (IN) iidStack of the LanDevObject.  This iidStack is
 *                      used to find the ancestor IGDObject.
 * @param delta (IN)    The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumLanDev(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the WANDeviceNumberOfEntries field in the InternetGatewayDevice object.
 *
 * @param iidStack (IN) iidStack of the WanDevObject.  This iidStack is
 *                      used to find the ancestor IGDObject.
 * @param delta (IN)    The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumWanDev(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the LANEthernetInterfaceNumberOfEntries field in the ancestor
 *  LanDevObject.
 *
 * @param iidStack (IN) iidStack of the LanEthIntfObject.  This iidStack is
 *                      used to find the ancestor LanDevObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumEthIntf(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the LANUSBInterfaceNumberOfEntries field in the ancestor
 *  LanDevObject.
 *
 * @param iidStack (IN) iidStack of the LanUsbIntfObject.  This iidStack is
 *                      used to find the ancestor LanDevObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumUsbIntf(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the LANMocaInterfaceNumberOfEntries field in the ancestor
 *  LanDevObject.
 *
 * @param iidStack (IN) iidStack of the LanMocaIntfObject.  This iidStack is
 *                      used to find the ancestor LanDevObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumMocaIntf(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the LANWlanConfigurationNumberOfEntries field in the ancestor
 *  LanDevObject.
 *
 * @param iidStack (IN) iidStack of the LanWlanConfigObject.  This iidStack is
 *                      used to find the ancestor LanDevObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumWlanConf(const InstanceIdStack *iidStack, SINT32 delta);

/** Change the LANEponConfigurationNumberOfEntries field in the ancestor
 *  LanDevObject.
 *
 * @param iidStack (IN) iidStack of the LanWlanConfigObject.  This iidStack is
 *                      used to find the ancestor LanDevObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumEponIntf(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the IpInterfaceNumberOfEntries field in the ancestor
 *  LanHostCfgObject.
 *
 * @param iidStack (IN) iidStack of the LanIpIntfObject.  This iidStack is
 *                      used to find the ancestor LanHostCfgObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumLanIpIntf(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the hostNumberOfEntries field in the ancestor LanHostsObject.
 *
 * @param iidStack (IN) iidStack of the LanHostEntryObject.  This iidStack is
 *                      used to find the ancestor LanHostsObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumLanHosts(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the WANConnectionNumberOfEntries field in the ancestor WanDevObject.
 *
 * @param iidStack (IN) iidStack of the WanConnObject.  This iidStack is
 *                      used to find the ancestor WanDevObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumWanConn(const InstanceIdStack *iidStack, SINT32 delta);



/** Change the WANIPConnectionNumberOfEntries field in the ancestor WanConnDevObject.
 *
 * @param iidStack (IN) iidStack of the WanIpConnObject.  This iidStack is
 *                      used to find the ancestor WanConnDevObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumIpConn(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the WANPPPConnectionNumberOfEntries field in the ancestor WanConnDevObject.
 *
 * @param iidStack (IN) iidStack of the WanPppConnObject.  This iidStack is
 *                      used to find the ancestor WanConnDevObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumPppConn(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the PortMappingNumberOfEntries field in the ancestor WanIPConnectionObject.
 *
 * @param iidStack (IN) iidStack of the PortMappingObject.  This iidStack is
 *                      used to find the ancestor WanIPConnectionObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumIpPortMapping(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the PortMappingNumberOfEntries field in the ancestor WanPPPConnectionObject.
 *
 * @param iidStack (IN) iidStack of the PortMappingObject.  This iidStack is
 *                      used to find the ancestor WanPPPConnectionObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumPppPortMapping(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the ForwardNumberOfEntries field in the ancestor Layer3Forwarding.
 *
 * @param iidStack (IN) iidStack of the Forwarding.  This iidStack is
 *                      used to find the ancestor Layer3Forwarding.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumL3Forwarding(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the BridgeNumberOfEntries field in the ancestor Layer2BridgingObject.
 *
 * @param iidStack (IN) iidStack of the BridgingEntry.  This iidStack is
 *                      used to find the ancestor Layer3Forwarding.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumL2Bridging(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the FilterNumberOfEntries field in the ancestor Layer2BridgingObject.
 *
 * @param iidStack (IN) iidStack of the BridgingFilter.  This iidStack is
 *                      used to find the ancestor Layer3Forwarding.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumL2BridgingFilter(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the AvailableInterfaceNumberOfEntries field in the ancestor Layer2BridgingObject.
 *
 * @param iidStack (IN) iidStack of the BridgingIntf.  This iidStack is
 *                      used to find the ancestor Layer3Forwarding.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumL2BridgingIntf(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the QueueNumberOfEntries field in the ancestor QMgmtObject.
 *
 * @param iidStack (IN) iidStack of the QMgmtQueueObject.  This iidStack is
 *                      used to find the ancestor QMgmtObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumQMgmtQueue(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the PolicerNumberOfEntries field in the ancestor QMgmtObject.
 *
 * @param iidStack (IN) iidStack of the QMgmtQueueObject.  This iidStack is
 *                      used to find the ancestor QMgmtObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumQMgmtPolicer(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the ClassificationNumberOfEntries field in the ancestor QMgmtObject.
 *
 * @param iidStack (IN) iidStack of the QMgmtClassification.  This iidStack is
 *                      used to find the ancestor QMgmtObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumQMgmtClassification(const InstanceIdStack *iidStack, SINT32 delta);

/** Change the vendorConfigFileNumberOfEntries field in the ancestor QMgmtObject.
 *
 * @param iidStack (IN) iidStack of the QMgmtClassification.  This iidStack is
 *                      used to find the ancestor QMgmtObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumVendorConfigFiles(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the SampleSetNumberOfEntries field in the ancestor PeriodicStatObject.
 *
 * @param iidStack (IN) iidStack of the SampleSet.  This iidStack is
 *                      used to find the ancestor PeriodicStatObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumSampleSets(const InstanceIdStack *iidStack, SINT32 delta);


/** Change the ParameterNumberOfEntries field in the ancestor SampleSetObject.
 *
 * @param iidStack (IN) iidStack of the SampleParameter.  This iidStack is
 *                      used to find the ancestor SampleSetObject.
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rut_modifyNumParameters(const InstanceIdStack *iidStack, SINT32 delta);


/** Macro to determine if this is a new MdmObject creation case.
 */
#define ADD_NEW(n, c) (((n) != NULL) && (c == NULL))


/** Macro to determine if this is a MdmObject deletion case
 */
#define DELETE_EXISTING(n, c) (((n) == NULL) && (c != NULL))


/** Macro to determine if an MdmObject is being added and enabled,
 *  or if an existing MdmObject that is currently disabled is now is being enabled.
 */
#define ENABLE_NEW_OR_ENABLE_EXISTING(n, c) \
   (((n) != NULL && (n)->enable && (c) == NULL) || \
    ((n) != NULL && (n)->enable && (c) != NULL && !((c)->enable)))
    
/** Macro to determine if a new MdmObject is being enabled.
      It could be happen at bootup time or runtime
 */
#define ENABLE_NEW(n, c) \
   ((n) != NULL && (n)->enable && (c) == NULL)
   
/** Macro to determine if an existing MdmObject is being enabled.
 */
#define ENABLE_EXISTING(n, c) \
   ((n) != NULL && (n)->enable && (c) != NULL && !((c)->enable))

/** Macro to determine if an existing MdmObject that is currently enabled
 *  is now being disabled.
 */
#define DISABLE_EXISTING(n, c) \
    ((n) != NULL && !((n)->enable) && (c) != NULL && (c)->enable)

/** Macro to determine if an MdmObject is being deleted (which implies disble),
 *  or if an existing MdmObject that is currently enabled is now being disabled.
 */
#define DELETE_OR_DISABLE_EXISTING(n, c) \
   (((n) == NULL) ||                                                    \
    ((n) != NULL && !((n)->enable) && (c) != NULL && (c)->enable))

/** Macro to determine if this is POTENTIALLY a modification of an existing
 * and enabled object.  
 * 
 * This macro must be used in conjunction with another function which
 * you provide that determines if any of the fields in the object has
 * changed.  This macro only verifies that the object currently exists
 * and is enabled. Example usage:
 * 
 * if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj) && fieldsHaveChanged(newObj, currObj)
 * 
 */
#define POTENTIAL_CHANGE_OF_EXISTING(n, c) \
   (((n) != NULL && (n)->enable && (c) != NULL && (c)->enable))



/** Macro which checks if the status param has changed, and if it has, records
 *  the current time in X_BROADCOM_COM_LastChange.
 */
#define IF_STATUS_HAS_CHANGED_SET_LASTCHANGE(n, c)  do { \
   if ((n) != NULL && (c) != NULL && cmsUtl_strcmp((c)->status, (n)->status)) \
      (n)->X_BROADCOM_COM_LastChange = cmsTms_getSeconds(); } while (0)


/** Macro which reports the number of seconds since the last status change,
 *  if the obj is not NULL.
 */
#define IF_OBJ_NOT_NULL_GET_LASTCHANGE(n)   do { \
   if ((n) != NULL) \
      (n)->lastChange = cmsTms_getSeconds() - (n)->X_BROADCOM_COM_LastChange; \
   } while (0)




UBOOL8 rut_getIfAddr(const char *devname, char* ip);
UBOOL8 rut_getIfDestAddr(char *devname, char* ip);
UBOOL8 rut_getIfSubnet(const char *devname, char* ipSubnet);
UBOOL8 rut_getIfMask(const char *devname, char* ip);
CmsRet rut_getIfStatusHwaddr(const char *devname, char *statusStr, char *hwAddr);




/** send msg to smd utility function.
 *
 * Will send any msg type with or without some data.
 * Sends the message and blocks waiting for reply.
 *
 * @param msgType    (IN)   Message type
 * @param wordData   (IN)   optional 4 byte value that can be put into the wordData field
 *                          of the msg header.
 * @param msgData    (IN)   optional data to send with the message, if NULL, no data.
 * @param msgDataLen (IN)   if msgData is not NULL, the length of the data
 * @return The wordData field of the reply message.  In most cases, this is a CmsRet enum.
 *         However, for some messages, e.g. RESTART_PPP and RESTART_DHCPC, the wordData
 *         will be the pid of the newly started app.
 */
UINT32 rut_sendMsgToSmd(CmsMsgType msgType, UINT32 wordData,
                        const void *msgData, UINT32 msgDataLen);

UINT32 rut_sendEventMsgToSmd(CmsMsgType msgType, UINT32 wordData,
                        const void *msgData, UINT32 msgDataLen);

CmsRet rut_sendMsgToSsk(CmsMsgType msgType, UINT32 wordData,
                        void *msgData, UINT32 msgDataLen);


/** Generic message sending util function used by all the other rut_sendMsgxxx
 *
 * @param dst        (IN)   destination EID
 * @param msgType    (IN)   Message type
 * @param wordData   (IN)   optional 4 byte value that can be put into the wordData field
 *                          of the msg header.
 *
 * @param event      (IN) Is this an event msg?  If TRUE, request must be FALSE
 * @param request    (IN) Is this a request msg?  If TRUE, event must be FALSE
 * @param getReply   (IN) Use cmsMsg_sendAndGetReply, else just use cmsMsg_send
 *
 * @param msgData    (IN)   optional data to send with the message, if NULL, no data.
 * @param msgDataLen (IN)   if msgData is not NULL, the length of the data
 *
 * @return  UINT32 which could be a PID or CmsRet code, depending on the msgType
 */
UINT32 rut_sendMsgCommon(CmsEntityId dst, CmsMsgType msgType, UINT32 wordData,
                       UBOOL8 event, UBOOL8 request, UBOOL8 getReply,
                       const void *msgData, UINT32 msgDataLen);




/** send msg to dhcpd telling it to look at the dhcpd.conf file again.
 *
 */
void rut_sendReloadMsgToDhcpd(void);


/** Run the given command in a separate shell using the system(3) library function.
 *
 * This function will block until the command completes.
 * Because the command is run using the system(3) library function,
 * the command string can contain I/O redirection, as in
 * iptables -w -L > /tmp/iptables_out
 *
 * @param from (IN) string identifying who is calling this func.
 * @param cmd  (IN) complete command string with args.
 *
 */
void rut_doSystemAction(const char* from, char *cmd); 



/** Run the given command in a separate shell using the system(3) library function.
 *
 * This function waits for a predefined timeout until the command completes.
 * Because the command is run using the system(3) library function,
 * the command string can contain I/O redirection, as in
 * iptables -w -L > /tmp/iptables_out
 *
 * @param from (IN) string identifying who is calling this func.
 * @param cmd  (IN) complete command string with args.
 * @return CmsRet enum
 *
 */
CmsRet rut_doSystemActionWithTimeout(const char* from, char *cmd);


/** Get default gateway interface name.
  * @param from (OUT) ifcName -- the return interface name,  or '\0' if not found.
  * @return void
  */
void rut_getDefaultGatewayInterfaceName(char *ifcName);




/** Return true if any WAN interface has NAT enabled.
 *
 * @return TRUE if any WAN interface has NAT enabled.
 */
UBOOL8 rut_isAnyNatEnabled(void);

UBOOL8 rut_isAnyNatEnabled_igd(void);

UBOOL8 rutNat_isAnyNatEnabled_dev2(void);


#if defined(SUPPORT_DM_LEGACY98)
#define rut_isAnyNatEnabled()  rut_isAnyNatEnabled_igd()
#elif defined(SUPPORT_DM_HYBRID)
#define rut_isAnyNatEnabled()  rut_isAnyNatEnabled_igd()
#elif defined(SUPPORT_DM_PURE181)
#define rut_isAnyNatEnabled()  rutNat_isAnyNatEnabled_dev2()
#elif defined(SUPPORT_DM_DETECT)
#define rut_isAnyNatEnabled()  (cmsMdm_isDataModelDevice2() ? \
                                rutNat_isAnyNatEnabled_dev2() : \
                                rut_isAnyNatEnabled_igd())
#endif




/** Check if the wan interface is nat enabled
 *  This is the old TR98 API.  New code should call
 *  qdmIpIntf_isNatEnabledOnIpIntfFullPathLocked() or
 *  qdmIpIntf_isNatEnabledOnIntfNameLocked() instead.
 *
  * @param (IN)   ifcName -- The interface name to be checked
  * @return TRUE or FALSE
  */
UBOOL8 rut_isWanInterfaceNatEnable_igd(const char *ifcName);




/** Check if the wan interface is bridged or not
  * @param (IN)   ifcName -- The interface name to be checked
  * @return TRUE or FALSE
  */
UBOOL8 rut_isWanInterfaceBridged(const char *ifcName);


/** Check if the application is running or not
  * @param (IN)   eid -- The application eid
  * @return TRUE or FALSE
  */
UBOOL8 rut_isApplicationRunning(CmsEntityId eid);


/** Check if the application is active or not
  * (Active means either DLS_LAUNCHED or DLS_RUNNING).
  * @param (IN)   eid -- The application eid
  * @return TRUE or FALSE
  */
UBOOL8 rut_isApplicationActive(CmsEntityId eid);


/** Insert the Url Filter related modules
 *
 * @param (IN) void
 * @return void
 */
void rut_UrlFilterLoadModule(void);


/** execute iptables rule for DMZ feature
 *
 * @param (IN) void
 * @return void
 */
void rut_activateDmzRule(void);


/** This function returns the interface tr98 full path name from the interface linux name.
 *
 * @param intfname   (IN) the interface linux name.
 * @param layer2     (IN) boolean to indicate whether intfname is a layer 2 or layer 3 interface name.
 * @param mdmPath    (OUT)the interface tr98 full path name. caller shall free the memory after used.
 * @return CmsRet         enum.
 */
CmsRet rut_intfnameToFullPath(const char *intfname, UBOOL8 layer2, char **mdmPath);

/** This function returns the interface linux name from the interface tr98 full path name.
 *
 * @param mdmPath    (IN) the interface tr98 full path name.
 * @param intfname   (OUT)the interface linux name. caller shall have allocated memory for it.
 * @return CmsRet         enum.
 */
CmsRet rut_fullPathToIntfname(const char *mdmPath, char *intfname);




CmsRet rut_setIfState(char *ifName, UBOOL8 up);

/** Check if the interface is created by OMCI ipHost ME
 *
 * @param ipAddress    (IN)  pointer of ipAddress
 * @param meId         (OUT)  pointer of ipHost ME ID
*
 * @return cmsret.
 */
CmsRet rut_isGponIpHostInterface(char *ifname, UINT32 *meId);

#if defined(OMCI_TR69_DUAL_STACK)
/** get ip address of ipHost interface
 *
 * @param ifname        (IN)  interface name
 * @param ipAddress    (IN)  pointer to save ipAddress
 * @param isIPv4       (OUT) is the address IPv4
 *
 * @return cmsret.
 */
CmsRet rutOmci_getIpHostAddress(char *ifname, char **ipAddress, UBOOL8 *isIPv4);
#endif

/** Change the number of XMPP connection field in the ancestor
 *  Dev2XmppConnServerObject.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_XMPP_CONN. This iidStack is
 *                      used to find the ancestor Dev2XmppConnServerObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumXmppConnServer(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumXmppConnServer_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumXmppConnServer_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumXmppConnServer(a,b)   rutUtil_modifyNumXmppConnServer_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumXmppConnServer(a,b)  rutUtil_modifyNumXmppConnServer_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumXmppConnServer(a,b)  rutUtil_modifyNumXmppConnServer_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumXmppConnServer(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                               rutUtil_modifyNumXmppConnServer_dev2((a),(b)) : \
                                               rutUtil_modifyNumXmppConnServer_igd((a),(b)))
#endif

/** Change the number of XMPP connection field in the ancestor
 *  Dev2XmppObject.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_XMPP. This iidStack is
 *                      used to find the ancestor Dev2XmppConnObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */

void rutUtil_modifyNumXmppConn(const InstanceIdStack *iidStack, SINT32 delta);
UBOOL8 rutUtil_modifyNumXmppConn_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumXmppConn_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumXmppConn(a,b)   rutUtil_modifyNumXmppConn_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumXmppConn(a,b)  rutUtil_modifyNumXmppConn_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumXmppConn(a,b)  rutUtil_modifyNumXmppConn_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumXmppConn(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                         rutUtil_modifyNumXmppConn_dev2((a),(b)) : \
                                         rutUtil_modifyNumXmppConn_igd((a),(b)))
#endif


/** Change the number of security certificates field in the ancestor
 *  Dev2SecurityObject.
 *
 * @param iidStack (IN) iidStack of the MDMOID_DEV2_SECURITY_CERTIFICATE. This iidStack is
 *                      used to find the ancestor of Dev2SecurityCertificateObject
 * @param delta    (IN) The number of increment or decrement the count by.
 *                      This number should only be 1 or -1.
 *
 */
void rutUtil_modifyNumSecCert_dev2(const InstanceIdStack *iidStack, SINT32 delta);


/** This function returns if the WAN type is EPON or not.
 *
 */
UBOOL8 rut_isWanTypeEpon(void);

/** This function returns RDPA WAN type.
 *
 */
UINT32 rut_getRdpaWanType(int wan_if);

void rutUtil_modifyNumExecEnvEntry(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumExecEnvEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumExecEnvEntry_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumExecEnvEntry(a,b)  rutUtil_modifyNumExecEnvEntry_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumExecEnvEntry(a,b)  rutUtil_modifyNumExecEnvEntry_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumExecEnvEntry(a,b)  rutUtil_modifyNumExecEnvEntry_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumExecEnvEntry(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                             rutUtil_modifyNumExecEnvEntry_dev2((a),(b)) : \
                                             rutUtil_modifyNumExecEnvEntry_igd((a),(b)))
#endif

void rutUtil_modifyNumDeploymentUnitEntry(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDeploymentUnitEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumDeploymentUnitEntry_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumDeploymentUnitEntry(a,b)  rutUtil_modifyNumDeploymentUnitEntry_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumDeploymentUnitEntry(a,b)  rutUtil_modifyNumDeploymentUnitEntry_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumDeploymentUnitEntry(a,b)  rutUtil_modifyNumDeploymentUnitEntry_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumDeploymentUnitEntry(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                                    rutUtil_modifyNumDeploymentUnitEntry_dev2((a),(b)) : \
                                                    rutUtil_modifyNumDeploymentUnitEntry_igd((a),(b)))
#endif

void rutUtil_modifyNumExecutionUnitEntry(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumExecutionUnitEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumExecutionUnitEntry_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumExecutionUnitEntry(a,b)  rutUtil_modifyNumExecutionUnitEntry_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumExecutionUnitEntry(a,b)  rutUtil_modifyNumExecutionUnitEntry_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumExecutionUnitEntry(a,b)  rutUtil_modifyNumExecutionUnitEntry_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumExecutionUnitEntry(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                                   rutUtil_modifyNumExecutionUnitEntry_dev2((a),(b)) : \
                                                   rutUtil_modifyNumExecutionUnitEntry_igd((a),(b)))
#endif

void rutUtil_modifyNumBusObjectPathEntry(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumBusObjectPathEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumBusObjectPathEntry_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumBusObjectPathEntry(a,b)  rutUtil_modifyNumBusObjectPathEntry_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumBusObjectPathEntry(a,b)  rutUtil_modifyNumBusObjectPathEntry_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumBusObjectPathEntry(a,b)  rutUtil_modifyNumBusObjectPathEntry_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumBusObjectPathEntry(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                                   rutUtil_modifyNumBusObjectPathEntry_dev2((a),(b)) : \
                                                   rutUtil_modifyNumBusObjectPathEntry_igd((a),(b)))
#endif

void rutUtil_modifyNumBusInterfaceEntry(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumBusInterfaceEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumBusInterfaceEntry_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumBusInterfaceEntry(a,b)  rutUtil_modifyNumBusInterfaceEntry_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumBusInterfaceEntry(a,b)  rutUtil_modifyNumBusInterfaceEntry_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumBusInterfaceEntry(a,b)  rutUtil_modifyNumBusInterfaceEntry_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumBusInterfaceEntry(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                                  rutUtil_modifyNumBusInterfaceEntry_dev2((a),(b)) : \
                                                  rutUtil_modifyNumBusInterfaceEntry_igd((a),(b)))
#endif

void rutUtil_modifyNumBusMethodEntry(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumBusMethodEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumBusMethodEntry_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumBusMethodEntry(a,b)  rutUtil_modifyNumBusMethodEntry_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumBusMethodEntry(a,b)  rutUtil_modifyNumBusMethodEntry_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumBusMethodEntry(a,b)  rutUtil_modifyNumBusMethodEntry_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumBusMethodEntry(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                               rutUtil_modifyNumBusMethodEntry_dev2((a),(b)) : \
                                               rutUtil_modifyNumBusMethodEntry_igd((a),(b)))
#endif

void rutUtil_modifyNumBusSignalEntry(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumBusSignalEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumBusSignalEntry_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumBusSignalEntry(a,b)  rutUtil_modifyNumBusSignalEntry_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumBusSignalEntry(a,b)  rutUtil_modifyNumBusSignalEntry_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumBusSignalEntry(a,b)  rutUtil_modifyNumBusSignalEntry_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumBusSignalEntry(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                               rutUtil_modifyNumBusSignalEntry_dev2((a),(b)) : \
                                               rutUtil_modifyNumBusSignalEntry_igd((a),(b)))
#endif

void rutUtil_modifyNumBusPropertyEntry(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumBusPropertyEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumBusPropertyEntry_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumBusPropertyEntry(a,b)  rutUtil_modifyNumBusPropertyEntry_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumBusPropertyEntry(a,b)  rutUtil_modifyNumBusPropertyEntry_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumBusPropertyEntry(a,b)  rutUtil_modifyNumBusPropertyEntry_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumBusPropertyEntry(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                                 rutUtil_modifyNumBusPropertyEntry_dev2((a),(b)) : \
                                                 rutUtil_modifyNumBusPropertyEntry_igd((a),(b)))
#endif

void rutUtil_modifyNumBusClientPrivilegeEntry(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumBusClientPrivilegeEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumBusClientPrivilegeEntry_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumBusClientPrivilegeEntry(a,b)  rutUtil_modifyNumBusClientPrivilegeEntry_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumBusClientPrivilegeEntry(a,b)  rutUtil_modifyNumBusClientPrivilegeEntry_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumBusClientPrivilegeEntry(a,b)  rutUtil_modifyNumBusClientPrivilegeEntry_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumBusClientPrivilegeEntry(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                                        rutUtil_modifyNumBusClientPrivilegeEntry_dev2((a),(b)) : \
                                                        rutUtil_modifyNumBusClientPrivilegeEntry_igd((a),(b)))
#endif

void rutUtil_modifyNumContainerEntry(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumContainerEntry_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumContainerEntry_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumContainerEntry(a,b)  rutUtil_modifyNumContainerEntry_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumContainerEntry(a,b)  rutUtil_modifyNumContainerEntry_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumContainerEntry(a,b)  rutUtil_modifyNumContainerEntry_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumContainerEntry(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                               rutUtil_modifyNumContainerEntry_dev2((a),(b)) : \
                                               rutUtil_modifyNumContainerEntry_igd((a),(b)))
#endif


void rutUtil_modifyNumEthLag(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumEthLag_dev2(const InstanceIdStack *iidStack, SINT32 delta);
void rutUtil_modifyNumEthLag_igd(const InstanceIdStack *iidStack, SINT32 delta);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUtil_modifyNumEthLag(a,b)  rutUtil_modifyNumEthLag_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUtil_modifyNumEthLag(a,b)  rutUtil_modifyNumEthLag_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutUtil_modifyNumEthLag(a,b)  rutUtil_modifyNumEthLag_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutUtil_modifyNumEthLag(a,b)  (cmsMdm_isDataModelDevice2() ? \
                                                   rutUtil_modifyNumEthLag_dev2((a),(b)) : \
                                                   rutUtil_modifyNumEthLag_igd((a),(b)))
#endif


/** This function returns the port tr69c/tr69c_2 listens on for connection requests from the ACS.
 *
 * @param (IN) void
 * @return port
 */
UINT32 rut_getTr69cConnReqPort (CmsExtendEntityIndex eeId);

/*
 * send a ACS_CONFIG_CHANGED event msg to smd with source acsConfigId,
 * then forwarded to registered process.
 */
CmsRet rut_sendAcsConfigChangedMsgToSmd(const char *acsConfigId);

#endif //__RUT_UTIL_H__
