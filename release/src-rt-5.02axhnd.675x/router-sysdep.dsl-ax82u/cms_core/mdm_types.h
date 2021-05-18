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
#ifndef __MDM_TYPES_H__
#define __MDM_TYPES_H__

/*!\file mdm_types.h
 * \brief Header file containing basic data types used in the MDM layer.
 * These may be useful to external callers as well.
 */

#include "cms.h"
#include "cms_mdm.h"
#include "cms_tms.h"





/** A structure for storing validation data for a parameter name node.
 *
 */
typedef struct
{
   void *min;  /**< If string type, this points to enum of valid values, if any. */
   void *max;  /**< If string type, this points to max allowed length, if specified. */
} ValidatorData;



/** Parameter is writable, used in MdmParamNode.flags. */
#define PRN_WRITABLE                        0x0001

/** This parameter will deny active notification request, used in MdmParamNode.flags. */
#define PRN_DENY_ACTIVE_NOTIFICATION        0x0002

/** Changing this parameter will always generate active notification, used in MdmParamNode.flags. */
#define PRN_FORCED_ACTIVE_NOTIFICATION      0x0004

/** This parameter will always be written out to the config file, used in MdmParamNode.flags. */
#define PRN_ALWAYS_WRITE_TO_CONFIG_FILE     0x0008

/** This parameter value should never be written out to the config file, used in MdmParamNode.flags. */
#define PRN_NEVER_WRITE_TO_CONFIG_FILE      0x0010

/** This parameter contains big data, don't keep a copy in mdm, used in MdmParamNode.flags. */
#define PRN_TRANSFER_DATA_BUFFER            0x0020

/** Do not send actual value to ACS, send empty string, used in MdmParamNode.flags. */
#define PRN_TR69_PASSWORD                   0x0040

/** Encrypt value in the config file, used in MdmParamNode.flags. */
#define PRN_CONFIG_PASSWORD                 0x0080

/** Do not send this param to the ACS, used in MdmParamNode.flags. */
#define PRN_HIDE_PARAMETER_FROM_ACS         0x0100

/** Notify ssk of changes to LowerLayers param (for intfstack), used in MdmParamNode.flags. */
#define PRN_NOTIFY_SSK_LOWERLAYERS_CHANGED  0x0200

/** Notify ssk of changes to Alias param (for intfstack), used in MdmParamNode.flags. */
#define PRN_NOTIFY_SSK_ALIAS_CHANGED        0x0400

/** Generate unique alias value on object creation (for TR181), used in MdmParamNode.flags. */
#define PRN_AUTO_GENERATE_ALIAS             0x0800

/** Count actual persistent instances for xyzNumberOfEntries param, used in MdmParamNode.flags. */
#define PRN_COUNT_PERSISTENT_INSTANCE       0x1000


/** Macro for testing whether the given paramNode allows writes. */
#define IS_PARAM_WRITABLE(n) ((n)->flags & PRN_WRITABLE)

/** Macro for getting the MdmParamType of the param node. */
#define PARAM_NODE_TYPE(n) ((n)->type)


/** A node in the MDM tree for representing a leaf parameter name.
 *
 * This structure should only be used as an opaque handle by most
 * callers.  Only internal layers in the CMS API should use the
 * internal fields directly.
 */
typedef struct
{
   const char *name;             /**< Name of this parameter. */
   const char *profile;          /**< Profile name */
   struct MdmObjectNode *parent; /**< Containing object node. */
   MdmParamTypes      type;      /**< Enumerated type of this param */
   UINT16      flags;            /**< Flags for this node, see PRN_xxx. */
   UINT16      offsetInObject;  /**< Number of bytes from beginning of object. */
   char *      defaultValue;   /**< Default value as specified by TRx or broadcom. */
   char *      suggestedValue; /**< Could be different from default value. */
   ValidatorData vData;  /**< Data used for validation. */
} MdmParamNode;



/** Structure to track parameter attributes.
 *
 * We expect there will be relatively few parameter attributes set,
 * so attributes maintained on a linked list to save space.
 */
typedef struct AttributesDescNode
{
   UINT32                 paramNum; /**< nth parameter from the top of the object */
   struct AttributesDescNode *next; /**< Next AttributesDescNode, if any */
   MdmNodeAttributes      nodeAttr; /**< attrs for this parameter node */
} AttributesDescNode;


/* A flag to tell whether a particular instance of an object is to written into
 * config file or not
 */
#define IDN_NON_PERSISTENT_INSTANCE 0x0001

/** Structure to track multiple instances of an object.
 *
 * These structures are organized in a singly linked list.  The the head
 * of the list is a InstanceHeadNode.
 */
typedef struct InstanceDescNode
{
   UINT32             instanceId; /**< The instance id of this node. */
   struct InstanceDescNode *next; /**< Next InstanceDescNode, if any. */
   MdmNodeAttributes    nodeAttr; /**< The attributes for this instance object */
   AttributesDescNode * attrDesc; /**< The attributes for the parameters for this instance of the object, if any */
   UBOOL8          deletePending; /**< This instance is about to be deleted. */
   UINT8           reserved;
   UINT16          flags; /**< Flags for this node, see IDN_xxx */
   SINT32          lastWritePid; /**< PID/TID of last obj write */
   CmsTimestamp    lastWriteTs;  /**< Timestamp of last obj write */
   void *                objData; /**< The values for the parameters for this instance of the object. */
} InstanceDescNode;


/** Structure to track multiple instances of sub-trees.
 *
 * This is pointed to by the objData field of any object node whose
 * instance depth is greater than 0.
 * If the object node is not an instance node, then there is just
 * a singly linked list of InstanceHeadNodes.  This scenario is called
 * indirect 1.
 * If the object node is an instance node, then
 * there will be a singly linked list of InstanceHeadNodes, each with
 * a different InstanceIdStack.  The InstanceIdStack identifies which sub-tree
 * this instance object is in.  From each InstanceHeadNode, there is
 * a singly linked list of InstanceDescNodes, which represent the
 * instances of this instance object in this particular sub-tree.
 * So basically, from each  instance object, there is a two dimensional lattice
 * of InstanceHeadNodes in the vertical dimension and InstanceDescNodes
 * in the horizontal direction.  This scenario is called indirect 2.
 */
typedef struct InstanceHeadNode
{
   InstanceIdStack    iidStack;   /**< Instance id's needed to reach this instance node. */
   MdmNodeAttributes  nodeAttr;   /**< The attributes for this instance object. */
   struct InstanceHeadNode *next; /**< Next InstanceHeadNode */
   UINT32             nextInstanceIdToAssign; /**< Next instance id to assign (indirect 2) */
   AttributesDescNode * attrDesc; /** The attributes for the parameters of this
                                   * instance of the object, if any (indirect 1) */
   UBOOL8          deletePending; /** This instance is about to be deleted. */
   SINT32          lastWritePid; /**< PID/TID of last obj write (indirect 1) */
   CmsTimestamp    lastWriteTs;  /**< Timestamp of last obj write (indirect 1) */
   void *             objData;    /**< Linked list of instances (indirect 2) or the
                                   * mdmObject (indirect 1) */
} InstanceHeadNode;



/*!\brief Object is an instance node, i.e. ends with .{i}.
 * Used in MdmObjectNode.flags.
 *
 * This does not necessarily mean that management entities are
 * allowed to create instances.  OBN_DYNAMIC_INSTANCES indicates
 * that ability.
 */
#define OBN_INSTANCE_NODE           0x01

/** Object allows createObjectIntance and deleteObjectInstance from management
 * entities; used in MdmObjectNode.flags. 
 * Note that there is another type of instance node, called a multiple
 * instance node, which can have multiple instances, but these instances
 * are created and deleted by RCL/RUT (internal MDM) functions.  */
#define OBN_DYNAMIC_INSTANCES       0x02

/** Do not write this object or any meta info about this object or any
 *  descendents of this object into the config file.  Used in MdmObjectNode.flags. 
 *  This is used for object sub-trees which are strictly dynamic reporting
 *  of info, e.g. LANDevice.{i}.Hosts.
 */
#define OBN_PRUNE_WRITE_TO_CONFIG_FILE      0x04

/** A flag to tell tr69c to not report this object, or any child parameters
 * or objects to the ACS.
 */
#define OBN_HIDE_OBJECT_FROM_ACS            0x08

/** Automatically re-order and compact instances based on the order param.
 */
#define OBN_AUTO_ORDER_INSTANCES            0x10




/** Macro for testing whether the given objNode is an instance node */
#define IS_INSTANCE_NODE(n) ((n)->flags & OBN_INSTANCE_NODE)

/** Macro for testing whether the given objNode allows add/delete of instances by management entities. */
#define IS_DYNAMIC_INSTANCE_NODE(n) (((n)->flags & (OBN_INSTANCE_NODE|OBN_DYNAMIC_INSTANCES)) == (OBN_INSTANCE_NODE|OBN_DYNAMIC_INSTANCES))

/** Macro for testing whether the given objNode is in the top portion of the
 * tree where there are no sub-trees. */
#define IS_INDIRECT0(n) ((n)->instanceDepth == 0)

/** Macro for testing whether the given objNode part of a sub-tree, but
 * is not an instance node itself. */
#define IS_INDIRECT1(n) (((n)->instanceDepth > 0) && (!(IS_INSTANCE_NODE(n))))

/** An indirect 2 node is just another name for instance node. */
#define IS_INDIRECT2(n) (((n)->instanceDepth > 0) && (IS_INSTANCE_NODE(n)))

/*! \brief Macro to test whether we are dealing with an instance object of the form
 * InternetGatewayDevice.WANDevice.1
 *
 * In this case, the iidStack will be the same as the objNode's instance depth
 */
#define IS_INDIRECT2_WITH_INSTANCE_ID(n, s) ((IS_INDIRECT2(n)) && \
                                             (DEPTH_OF_IIDSTACK(s) == (n)->instanceDepth))

/*! \brief  Macro to test whether we are dealing with an instance object of the form
 * InternetGatewayDevice.WANDevice.
 *
 * In this case, the iidStack will be one less than the objNode's instance depth
 */
#define IS_INDIRECT2_WO_INSTANCE_ID(n, s) ((IS_INDIRECT2(n)) && \
                                            (DEPTH_OF_IIDSTACK(s) == (n)->instanceDepth - 1))

/*! \brief  Is this node an AUTO ORDER node?
 */
#define IS_AUTO_ORDER_NODE(n)   ((n)->flags & OBN_AUTO_ORDER_INSTANCES)


/** A node in the MDM tree for representing an object.
 *
 * This structure should only be used as an opaque handle by most
 * callers.  Only internal layers in the CMS API should use the
 * internal fields directly.
 *
 * There are 3 sub-types of an MdmObjectNode.
 *
 * Indirect 0: MdmObjectNodes whose instance depth is 0.  These MdmObjectNodes
 *             do not have any {i} in their path name.
 *
 * Indirect 1: MdmObjectNodes whose instance depth is > 0.  These MdmObjectNodes
 *             contain one or more {i} in their path name, but their path names
 *             do not end in {i}.
 *
 * Indirect 2: MdmObjectNodes whose instance depth is > 0.  These MdmObjectNodes
 *             contain one or more {i} in their path name and their path names
 *             end in {i}.  These are also refered to as instance nodes.  These
 *             are the only nodes which allow addObjectInstance and deleteObjectInstance
 *             operations.
 */
typedef struct MdmObjectNode
{
   MdmObjectId oid;        /**< oid of this object. */
   UINT8       lockZone;   /**< which lock zone does this obj belong in */
   char *      name;       /**< name of this object. */
   const char *profile;    /**< Profile name */
   UINT8       flags;      /**< Flags for this node, see OBN_xxx */
   UINT8       instanceDepth;     /**< How many instance nodes above this node. */
   struct MdmObjectNode *parent;  /**< parent of this node. */
   MdmNodeAttributes   nodeAttr;  /**< This node's attributes. */
   AttributesDescNode * attrDesc; /** The attributes for the parameters of this object.
                                   * Note this field is only used if the object node
                                   * has instance depth 0. */
   UINT16      numParamNodes;  /**< Number of MdmParamNodes in this object. */
   UINT16      numChildObjNodes;  /**< Number of child object nodes */
   MdmParamNode *params;      /**< Array of this object's MdmParamNodes. */
   struct MdmObjectNode *childObjNodes;  /**< Array of this object's child MdmObjectNodes. */
   SINT32       lastWritePid; /**< PID/TID of last obj write (indirect 0) */
   CmsTimestamp lastWriteTs;  /**< Timestamp of last obj write (indirect 0) */
   void *      objData;           /**< Parameter data for this object.
                                   * Note if this is an instance node, objData
                                   * points to an InstanceHeadNode. */
} MdmObjectNode;


#endif /* __MDM_TYPES_H__ */
