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

#ifndef __MDM_PRIVATE_H__
#define __MDM_PRIVATE_H__

#include "cms.h"
#include "cms_mdm.h"
#include "mdm_types.h"


/*!\file mdm_private.h
 * \brief This header file contains functions used by various files
 *        in the MDM layer.
 *
 *  The functions in this file are not intended for use by other layers
 *  in the cms_core library, nor should it be used by management apps. 
 *  See also mdm.h and cms_mdm.h.
 *
 */



#ifdef SUPPORT_PORT_MAP
/*
 * These portmap defines are probably obsolete.  Should delete them.
 */
#define LAN_DATA_PATH             "/var/lan"
#define IFC_LAN_MAX               4
#define IFC_ENET_ID               1
#define IFC_ENET1_ID            (IFC_ENET_ID+1)
#define IFC_USB_ID              (IFC_ENET_ID + IFC_LAN_MAX)
#define IFC_HPNA_ID             (IFC_USB_ID + IFC_LAN_MAX)
#define IFC_WIRELESS_ID         (IFC_HPNA_ID + IFC_LAN_MAX)
#define IFC_SUBNET_ID           (IFC_WIRELESS_ID + IFC_LAN_MAX) // for sencond LAN subnet
#define IFC_ENET0_VNET_ID       (IFC_SUBNET_ID + IFC_LAN_MAX)
#define IFC_ENET1_VNET_ID       (IFC_ENET0_VNET_ID + IFC_LAN_MAX)
#endif


/** Load the config file from flash and initialize the MDM with it.
 */
CmsRet mdm_loadConfig(void);


/** Make any adjustments to the MDM for the hardware that we are running on.
 *
 * This function is called after the config file has been loaded (or an
 * attempt at loading the config file) to ensure the needed structures 
 * in the MDM are present.  It also enables interfaces that are specified
 * as disabled by default in the TR98 specification, but we may need them
 * enabled in order to configure the device.
 *
 * @return CmsRet enum.
 */ 
CmsRet mdm_adjustForHardware(void);

CmsRet mdm_adjustForHardware_igd(void);

CmsRet mdm_adjustForHardware_dev2(void);

#if defined(SUPPORT_DM_LEGACY98)
#define mdm_adjustForHardware()   mdm_adjustForHardware_igd()
#elif defined(SUPPORT_DM_HYBRID)
#define mdm_adjustForHardware()   mdm_adjustForHardware_igd()
#elif defined(SUPPORT_DM_PURE181)
#define mdm_adjustForHardware()   mdm_adjustForHardware_dev2()
#elif defined(SUPPORT_DM_DETECT)
#define mdm_adjustForHardware()  (cmsMdm_isDataModelDevice2() ? \
                                  mdm_adjustForHardware_dev2() : \
                                  mdm_adjustForHardware_igd())
#endif


#ifdef DMP_DEVICE2_BASELINE_1
CmsRet mdm_initIpObject_dev2(UBOOL8 supportIpv4, UBOOL8 supportIpv6);

CmsRet mdm_addDefaultLanIpInterfaceObject_dev2(const char *ifname,
                         const char *groupName,
                         UBOOL8 supportIpv4, UBOOL8 supportIpv6,
                         const char *lowerLayer,
                         MdmPathDescriptor *ipIntfPathDesc);

CmsRet mdmInit_addIpInterfaceObject_dev2(const char *ifname,
                            const char *groupName,
                            UBOOL8 supportIpv4 __attribute((unused)),
                            UBOOL8 supportIpv6 __attribute((unused)),
                            UBOOL8 isUpstream,
                            UBOOL8 isBridgeService,
                            UBOOL8 isBridgeIpAddrNeeded,
                            const char *referedBridgeName,
                            const char *lowerLayer,
                            MdmPathDescriptor *ipIntfPathDesc);

CmsRet mdmInit_addBridgePortObject_dev2(const MdmPathDescriptor *brPathDesc,
                                        const char *ifname,
                                        UBOOL8 isManagementPort,
                                        const char *lowerLayers,
                                        MdmPathDescriptor *pathDesc);

CmsRet mdmInit_addFullPathToBridge_dev2(const char *bridgeIfName,
                                        const char *lowerLayerIfName,
                                        const char *lowerLayerFullPath);

CmsRet mdmInit_addDhcpv4ClientObject_dev2(const char *ipIntfFullPath);

#ifdef DMP_DEVICE2_ROUTING_1
CmsRet mdm_initRouterObject_dev2(void);
#endif

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
CmsRet mdm_addDefaultDhcpv6ServerObjects_dev2(const char *ipIntfFullPath);
CmsRet mdm_addDefaultRouterAdvertisementObjects_dev2(const char *ipIntfFullPath);
#endif
#endif //DMP_DEVICE2_BASELINE_1

/** Sets the default Strict Priority queues for the Ethernet interface.
 * This function is used by both TR98 and TR181 code.
 *
 * @param fullPath (IN) full mdm path of the interface.
 * @param isWan (IN) whether the interface is WAN or LAN.
 * @param enable (IN) whether the queue shall be enable or not.
 *
 * @return CmsRet enum.
 */
CmsRet addDefaultEthQueueObject(const char *fullPath, UBOOL8 isWan, UBOOL8 enable);

/** Add a QoS queue with the specified parameters
 *
 * @param precedence (IN) queue precedence
 * @param qid (IN) queue ID
 * @param fullPath (IN) full mdm path of the interface.
 * @param qname (IN) queue name.
 * @param enable (IN) whether the queue shall be enable or not.
 *
 * @return CmsRet enum.
 */
CmsRet mdmInit_addQosQueue(UINT32 precedence, UINT32 qid,
                           const char *fullPath, const char *qname,
                           UBOOL8 enable);

CmsRet mdmInit_addQosQueue_igd(UINT32 precedence, UINT32 qid,
                               const char *fullPath, const char *qname,
                               UBOOL8 enable);

CmsRet mdmInit_addQosQueue_dev2(UINT32 precedence, UINT32 qid,
                               const char *fullPath, const char *qname,
                               UBOOL8 enable);

#if defined(SUPPORT_DM_LEGACY98)
#define mdmInit_addQosQueue(p, q, i, n, e)  mdmInit_addQosQueue_igd((p), (q), (i), (n), (e))
#elif defined(SUPPORT_DM_HYBRID)
#define mdmInit_addQosQueue(p, q, i, n, e)  mdmInit_addQosQueue_igd((p), (q), (i), (n), (e))
#elif defined(SUPPORT_DM_PURE181)
#define mdmInit_addQosQueue(p, q, i, n, e)  mdmInit_addQosQueue_dev2((p), (q), (i), (n), (e))
#elif defined(SUPPORT_DM_DETECT)
#define mdmInit_addQosQueue(p, q, i, n, e)  (cmsMdm_isDataModelDevice2() ? \
                  mdmInit_addQosQueue_dev2((p), (q), (i), (n), (e)) : \
                  mdmInit_addQosQueue_igd((p), (q), (i), (n), (e)))
#endif

#ifdef BRCM_VOICE_SUPPORT
/** Make any adjustments to the MDM for the hardware that we are running on.
 * 
 * See description for mdm_adjustForHardware().
 * 
 * @return CmsRet enum.
 */
CmsRet mdm_adjustForVoiceHardware(void);
#endif

#ifdef DMP_STORAGESERVICE_1
/** Make sure Storageservice object is present, if not add it.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_addDefaultStorageServiceObject(void);
#endif

#ifdef DMP_DEVICE2_USBHOSTSBASIC_1
/** Make sure UsbHost object is present, if not add it.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_addDefaultUsbHostObject(void);
#endif

/** Increase the number of WANDevices in the top level IGD object.
 *
 * @param added (IN) The number to increase the count by.
 */
void mdm_increaseWanDeviceCount(UINT32 added);


/** Make sure configured DSL WANDevices are present, if not add them.
 *
 * Will check for ATM and PTM WanDevices.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_addDefaultWanDslObjects(void);

CmsRet addDefaultEponObjects(void);


/*
 * Some Homeplug init functions.
 */
UBOOL8 isPlcInterfaceExist(void);
CmsRet mdmInit_addHomePlugInterfaceObject(const char *ifName, UBOOL8 isUpstream, char **homePlugFullPath);
CmsRet mdmInit_addUpstreamHomePlugWanService(const char *intfName);
CmsRet mdmInit_addDownstreamHomePlugPort(const char *intfName);
CmsRet mdmInit_addDhcpv6ClientObject_dev2(const char * ipIntfPathRef);





/** Recursively traverse the entire MDM tree and call the RCL handler
 *  function for all objects.
 *
 * This is done after the config has been loaded into the MDM and
 * all adjustments have been made by mdm_adjustForHardware().
 * We are pretty much past the point of no return here, so even if RCL
 * handler functions encounter errors, just keep going.
 *
 * @param objNode  (IN) MdmObjectNode to activate.
 * @param iidStack (IN) Instance to activate.
 */
void mdm_activateObjects(const MdmObjectNode *objNode, 
                         const InstanceIdStack *iidStack);



/** Create a sub-tree starting from this objNode.
 *
 * This is a recursive function.
 */
CmsRet mdm_createSubTree(MdmObjectNode *objNode,
                         UINT32 depth,
                         const InstanceIdStack *iidStack,
                         MdmOperationCallback cbFunc,
                         void *cbContext);


/** Find a child object node with the specified name.
 *
 * @return pointer to child objNode.
 */
MdmObjectNode *mdm_getChildObjectNode(MdmObjectNode *objNode,
                                      const char *childObjNodeName);


/** Get the instance head containing iidStack on this objNode.
 *
 * From mdm.c
 *
 * @return pointer to instHead.
 */
InstanceHeadNode *mdm_getInstanceHead(const MdmObjectNode *objNode,
                                      const InstanceIdStack *iidStack);


/** Get the specified instance id from the instHead chain.
 *
 * @return pointer to instDesc.
 */
InstanceDescNode *mdm_getInstanceDesc(const InstanceHeadNode *instHead,
                                      UINT32 instanceId);


/** Get the instance desc for the specified iidStack on the objNode.
 *
 * @return pointer to instDesc.
 */
InstanceDescNode *mdm_getInstanceDescFromObjNode(const MdmObjectNode *objNode,
                                                 const InstanceIdStack *iidStack);




/** Set attributes for all parameters under this object node, which also has
 * the effect of setting the attributes of all parameter nodes in the sub-tree
 * rooted at the object node.
 *
 * @param objNode  (IN) The obj node to set the attribute information for.
 * @param iidStack (IN) The instance information for the obj node.
 * @param nodeAttr (IN) New attributes to set.
 * @param testOnly (IN) If TRUE, check if the specified attributes can be set,
 *                      but don't actually do it.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_setSubTreeParamAttributes(MdmObjectNode *objNode,
                                     const InstanceIdStack *iidStack,
                                     const MdmNodeAttributes *nodeAttr,
                                     UBOOL8 testOnly);


/** Set the attributes of a single paramNode.
 *
 * @param paramNode(IN) The param node to set the attribute information for.
 * @param iidStack (IN) The instance information for the param node.
 * @param nodeAttr (IN) New attributes to set.
 * @param testOnly (IN) If TRUE, check if the specified attributes can be set,
 *                      but don't actually do it.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_setSingleParamNodeAttributes(const MdmParamNode *paramNode,
                                        const InstanceIdStack *iidStack,
                                        const MdmNodeAttributes *nodeAttr,
                                        UBOOL8 testOnly);


/** Compare the accessBitMask and notification fields of the
 *  two given MdmNodeAttributes.
 *
 * But does not compare the valueChanged field.
 *
 * @returns 0 if they are the same, 1 if they are different.
 */
SINT32 mdm_compareNodeAttributes(const MdmNodeAttributes *attr1,
                                 const MdmNodeAttributes *attr2);



/** Return the node attributes and the pointer to the AttributesDescNode list, if any,
 *  for the specified objNode and iidStack.
 */
CmsRet mdm_getAllNodeAttributes(const MdmObjectNode *objNode,
                                const InstanceIdStack *iidStack,
                                MdmNodeAttributes **nodeAttr,
                                AttributesDescNode **attrDesc);


/** Find the AttributesDescNode for the nth paramNode.
 *
 * The list is not in order, so exhuastive search of the list is required.
 */
AttributesDescNode *mdm_findAttributesDesc(AttributesDescNode *head, UINT32 n);


/** Validate a parameter value which is in string format.
 *
 * Note this function may convert the string to a binary type
 * if this is actually a binary type.
 *
 * @param paramNode (IN) The paramNode of the parameter to verify.
 * @param strValue  (IN) The string to be validated.
 * @return CmsRet enum.
 */

CmsRet mdm_validateParamNodeString(const MdmParamNode *paramNode,
                                   const char *strValue);



/** Load a validated config buf into the MDM.
 *
 * Caller should have verified the buf before calling this function,
 * so other than memory allocation failures, it is guarenteed to succeed.
 *
 * @param buf (IN) Buffer containing the validated config file.
 * @param len (IN) Length of buffer.
 */
void mdm_loadValidatedConfigBufIntoMdm(const char *buf, UINT32 len);


/** Get the first and last valid pointers into the OID info array.
 * This function will get the right table for the data model mode.
 *
 * @param begin (OUT) First valid entry in the OID info array.
 * @param end   (OUT) Last valid entry in the OID info array.
 */
void mdm_getOidInfoPtrs(const MdmOidInfoEntry **begin, const MdmOidInfoEntry **end);

#if defined(DMP_X_BROADCOM_COM_RDPA_1)
/** Get the rdpa if from the eth if name.
 *
 * @param ifname (IN) ethif name.
 * @param rdpaIf_p   (OUT) rdpa if enumerator.
 * @param arraySz (IN) rdpa interface ID Array size.
 */
CmsRet mdm_getRpdaIfByIfname(const char* ifname, int* rdpaIf_p);

#define RDPA_WAN_TYPE_PSP_KEY     "RdpaWanType"
#endif
/** Lets rest of mdm_init code know that we are using a completely unmodifed
 *  default MDM configuration.
 */
extern UBOOL8 mdmUsingDefaultConfig;


/** Nanoxml does not pass context to callback func, so we have to use a global var */
extern struct nanoxml_context nxmlCtx;


/*!\enum mdm_config_attribute
 * \brief Attributes recognized by mdm_config.c.
 */
enum mdm_config_attribute
{
   MDM_CONFIG_XMLATTR_NONE=0,
   MDM_CONFIG_XMLATTR_VERSION=1,
   MDM_CONFIG_XMLATTR_INSTANCE=2,
   MDM_CONFIG_XMLATTR_NEXT_INSTANCE=3,
   MDM_CONFIG_XMLATTR_ACCESS_LIST=4,
   MDM_CONFIG_XMLATTR_NOTIFICATION=5
};


/* some string constants used in the config file */
#define CONFIG_FILE_TOP_NODE            "DslCpeConfig"
#define CONFIG_FILE_PSI_TOP_NODE        "psitree"
#define CONFIG_FILE_ATTR_VERSION        "version"
#define CONFIG_FILE_ATTR_INSTANCE       "instance"
#define CONFIG_FILE_ATTR_NEXT_INSTANCE  "nextInstance"
#define CONFIG_FILE_ATTR_ACCESS_LIST    "accessList"
#define CONFIG_FILE_ATTR_NOTIFICATION   "notification"




/** this is a context structure for use during nanoxml callback. */
struct nanoxml_context
{
   UBOOL8 loadMdm;             /**< Insert results of xml parsing into MDM */
   UBOOL8 topNodeFound;        /**< Have we seen the DslCpeConfig node yet? */
   UBOOL8 versionFound;        /**< Have we seen the version attribute yet? */
   UBOOL8 nextInstanceNode;    /**< currently processing a special nextInstance node */
   UBOOL8 gotCurrObjEndTag;    /**< we have seen the end tag for the current object */

   UINT32 versionMajor;      /**< Config file version major */
   UINT32 versionMinor;      /**< Config file version minor.  Not used, but feels weird if not provided. */

   MdmObjectNode *objNode;   /**< Current objNode we are processing */
   InstanceIdStack iidStack; /**< IidStack of the mdmObj we are building. */
   void          *mdmObj;    /**< Current mdmObj we are building */
   MdmParamNode  *paramNode; /**< Current paramNode we are processing */
   char          *paramValue; /**< Value of current param in string format. */
   MdmNodeAttributes attr;   /**< MdmNodeAttribute */
   enum mdm_config_attribute currXmlAttr; /**< XML attribute we are currently processing */
   char          *ignoreTag; /**< Ignore this param/obj tag, and everything in between until we see matching end tag */
   CmsRet        ret;        /**< Any errors detected during callback. */
};



#endif /* __MDM_PRIVATE_H__ */
