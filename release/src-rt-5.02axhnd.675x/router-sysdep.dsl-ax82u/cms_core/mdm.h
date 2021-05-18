/***********************************************************************
 *
 *  Copyright (c) 2006-2007  Broadcom Corporation
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

#ifndef __MDM_H__
#define __MDM_H__

#include "cms.h"
#include "cms_mdm.h"
#include "cms_tms.h"
#include "cms_lck.h"
#include "cms_eid.h"
#include "mdm_types.h"
#include "oal.h"
#include "mdm_objectid.h"
#include "mdm_object.h"



/*!\file mdm.h
 * \brief Header file for the Memory Data Mode (MDM) layer internal API.
 *
 * The functions in this file are only used by other functions
 * within the cms_core library.
 *
 * The MDM layer also exports some functions to outside callers.
 * See cms_mdm.h for those functions.
 */


/** Count how many child MdmObjectNodes are directly under the specified MdmObjectNode.
 *
 * This function returns the number of child object nodes are directly under
 * the given object.  It does not go down more than one level.
 *
 * @param parentPathDesc  (IN) The parent object path descriptor to operate on.
 * @param childObjectNodeCount (OUT) The number of child objects under the object.
 * @return CmsRet enum.
 */
CmsRet mdm_getChildObjectNodeCount(const MdmPathDescriptor *parentPathDesc,
                                   UINT32 *childObjectNodeCount);


CmsRet mdm_getPersistentInstanceCount(const char *paramName,
                                      MdmObjectId oid,
                                      const InstanceIdStack *iidStack,
                                      UINT32 *persistentInstanceCount);


/** Get the MdmPathDescriptor of the next child object.
 *
 * This only returns all the first level child objects of the parent
 * object node.  This function is object node based.
 * The paramName field in both the parentPathDesc and pathDesc are ignored.
 *
 * @param parentPathDesc (IN) The parent MdmPathDescriptor to operate on.
 *
 * @param pathDesc (IN/OUT) If the pathDesc is empty (freshly initialized) on entry,
 *                          then the first child object will be returned.
 *                          Otherwise, the next child object after the child
 *                          specified in pathDesc will be returned.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_getNextChildObjPathDesc(const MdmPathDescriptor *parentPathDesc,
                                   MdmPathDescriptor *pathDesc);




/** Get the MdmPathDescriptor of the next object.
 *
 * This is the depth first traversal of the tree, constrained to go no
 * further than the parentObjNode.  This function is object node based.
 * The paramName field in both the parentPathDesc and pathDesc are ignored.
 *
 * @param parentPathDesc (IN) The parent MdmPathDescriptor to operate on.
 *
 * @param pathDesc (IN/OUT) If the pathDesc is empty (freshly initialized) on entry,
 *                          then the first child will be returned.
 *                          Otherwise, the next child after the child specified 
 *                          in pathDesc will be returned.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_getNextObjPathDesc(const MdmPathDescriptor *parentPathDesc,
                              MdmPathDescriptor *pathDesc);

    

/** Count how many parameters are directly under the specified MdmObjectNode.
 *
 * This function returns the number of parameters directly under
 * the given object.  It does not go count the parameters of any
 * other objects.
 *
 * @param pathDesc  (IN)  Count the number of parameters under the object
 *                        specified by this pathDesc.
 * @param paramNodeCount (OUT) The number of parameters under the object.
 * @return CmsRet enum.
 */
CmsRet mdm_getParamNameCount(const MdmPathDescriptor *pathDesc,
                             UINT32 *paramCount);



/** Get the next parameter node of this object node.
 *
 * @param pathDesc (IN/OUT) If the paramName field of the MdmPathDescriptor is empty
 *                          on entry, then the paramName field will be
 *                          filled in with the name of the first parameter
 *                          of the object as specified by the oid and iidStack
 *                          fields of the MdmpathDescriptor.  If the paramName
 *                          field is not empty, then the next parameter name
 *                          will be placed in the paramName field.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_getNextParamName(MdmPathDescriptor *pathDesc);




/** callback function to be used in mdm_addObjectInstance and mdm_deleteObjectInstance */
typedef CmsRet (*MdmOperationCallback)(const MdmObjectNode *objNode, const InstanceIdStack *iidStack, const void *mdmObj, void *cbContext);


/** Add an instance of an object.
 *
 * This call may result in the creation of additional object instances in the sub-tree.
 *
 * @param pathDesc (IN/OUT) pathDesc.oid should be set to the object id of the new instance to be created.
 *                          On entry, the pathDesc.iidStack contains either instance information
 *                          up to the objNode in the MDM tree OR the instance information including
 *                          the new instance number to be created.  If the former, then MDM will
 *                          assign the next available instance number to the newly created object.
 *                          If the latter, MDM will use the instance number that the caller
 *                          provided if that instance number is not already in use (but return error
 *                          if the instance number is in use.)  On successful return,
 *                          iidStack will contain the instance information for the newly created
 *                          object instance.  
 *                          pathDesc.paramName is not used and must be empyt.
 * @param cbFunc (IN) If not NULL, this function will be called for every object
 *                    instance in the sub-tree that was created by this call.
 * @param cbContext (IN/OUT) This pointer will be passed to the callback function
 *                           for use by the callback function.
 * @return CmsRet enum.
 */
CmsRet mdm_addObjectInstance(MdmPathDescriptor *pathDesc,
                             MdmOperationCallback cbFunc,
                             void *cbContext);



/** Delete an instance of an object.
 *
 * mwang_todo: Still need to figure out how to handle/pass back notifications
 * for sub-tree deletes.  Original comment:  On both successful or unsuccessful
 * return, contains access attributes of all the instances that were or would have been
 * deleted.  This parameter is necessary because deleteObjectInstance
 * is recursive and can trigger delete object instances further
 * down in the sub-tree.
 *
 * @param pathDesc (IN) pathDesc.oid should be set to the object id of object to be deleted.
 *                      pathDesc.iidStack contains instance information of the instance to be deleted.
 *                      Note that pathDesc.iidStack is not modified by this function,
 *                      so on return pathDesc.iidStack will not be valid
 *                      (it points to a non-existent instance.)
 *                      pathDesc.paramName is not used.
 * @param cbFunc (IN) If not NULL, this function will be called for every object
 *                    instance in the sub-tree that was deleted by this call.
 * @param cbContext (IN/OUT) This pointer will be passed to the callback function
 *                           for use by the callback function.
 * @return CmsRet enum.
 */
CmsRet mdm_deleteObjectInstance(const MdmPathDescriptor *pathDesc,
                                MdmOperationCallback cbFunc,
                                void *cbContext);


/** Return true if the object instance is about to be deleted.
 *
 * @param mdmObj   (IN) The mdmObj which is being queried.
 * @param iidStack (IN) The iidStack of the mdmObj which is being queried.
 *
 * @return TRUE if the mdmObject being queried is about to be deleted.
 */
UBOOL8 mdm_isObjectDeletePending(const void *mdmObj, const InstanceIdStack *iidStack);

/** Modifies flags for an Instance  
 *
 * @param objNode   (IN) The mdmObj which is being queried.
 * @param iidStack (IN) The iidStack of the mdmObj which is being queried.
 * @param flags  (IN) flags to be set.
 *
 * @return CmsRet enum.
 */

CmsRet mdm_setInstanceFlags(const MdmObjectNode *objNode,const InstanceIdStack *iidStack, UINT16 flags);

/** Return flags for an Instance  
 *
 * @param objNode  (IN) The mdmObj which is being queried.
 * @param iidStack (IN) The iidStack of the mdmObj which is being queried.
 * @param flags  (OUT) current flags.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_getInstanceFlags(const MdmObjectNode *objNode,const InstanceIdStack *iidStack, UINT16 *flags);


/** Return generic object path name from oid.
 *
 * This function returns the string from the oidStringTable, using the oid
 * as the index into the table.
 *
 * @param oid (IN) oid of desired object.
 * @return char * of generic object path.  NULL if oid is invalid.
 */
const char *mdm_oidToGenericPath(MdmObjectId oid);



/** Return oid from generic object path name.
 *
 * @param genericPath (IN)  generic object path.
 * @param oid         (OUT) if generic object path is a valid and known path,
 *                          then this will be set to the corresponding oid.
 * @return CmsRet enum.
 */
CmsRet mdm_genericPathToOid(const char *genericPath, MdmObjectId *oid);



/** Given a MdmPathDescriptor that specifies a parameter name, return
 *  return the type of the parameter in a string.
 * 
 * @param pathDesc (IN) The parameter name.
 * @return const char string representation of the param type.
 * Caller must not free or modify the string that is returned.
 * If the pathDesc is invalid, string of "invalid" will be returned.
 */
const char *mdm_getParamType(const MdmPathDescriptor *pathDesc);


/** Given a MdmPathDescriptor that specifies a parameter name, return
 *  return the base type of the parameter's data type in a string.
 * 
 * @param pathDesc (IN) The parameter name.
 * @return const char string representation of the base type of paramter's type.
 * Caller must not free or modify the string that is returned.
 * If the pathDesc is invalid, string of "invalid" will be returned.
 */
const char *mdm_getParamBaseType(const MdmPathDescriptor *pathDesc);

/** Is this parameter considered a password in the TR-069 protocol?
 *
 * If a GetParameterValue is called on a parameter that is considered
 * a password by the TR-069 protocol, the tr69c should return an empty
 * string instead of the actual value.  This API only tells tr69c
 * if this parameter is a password parameter.  tr69c code is responsible
 * for replacing the actual value with an empty string.
 *
 * @param      (IN)  Path descriptor pointing to the parameter in question.
 * @param      (OUT) TRUE if pathDesc specifies a param that is considered
 *                   a password in the TR-069 protocol.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_getParamIsTr69Password(const MdmPathDescriptor *pathDesc,
                                  UBOOL8 *isPassword);



/** Is this object or parameter hidden from the TR-069 ACS.
 *
 * We might not want to send all of our parameters to the ACS.
 * This entry point allows other parameter walking functions,
 * (e.g. phl_getNextPath, mdm_getNextChildObjPathDesc, etc). to determine
 * if this object or parameter should be hidden from tr69c and hence the
 * TR-069 ACS.  From an architectural perspective, tr69c should do the
 * hiding.  But from a coding perspective, it is much cleaner to do the
 * hiding in the phl and mdm layers.
 *
 * @param      (IN)  Path descriptor pointing to the object or parameter in question.
 * @param      (OUT) TRUE if pathDesc specifies an object or param that is hidden
 *                   from tr69c.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_getPathDescHiddenFromAcs(const MdmPathDescriptor *pathDesc,
                                    UBOOL8 *hidden);



/** Can we add/delete instances of the object or write to the parameter.
 *
 * @param      (IN)  Path descriptor pointing to the object or parameter in question.
 * @param      (OUT) TRUE if pathDesc specifies an object and caller is allowed
 *                   to create/delete instances of this object; or if pathDesc
 *                   specifies a parameter and caller is allowed to write to
 *                   the specified parameter.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_getPathDescWritable(const MdmPathDescriptor *pathDesc,
                               UBOOL8 *writable);





/** Get attributes for the specified parameter.
 *
 * @param pathDesc  (IN) Path descriptor to the parameter whose attributes are
 *                       to be retreived.
 * @param nodeAttr (OUT) Caller allocates storage for nodeAttr.  This function
 *                       fills it in.
 * @returns CmsRet enum.
 */
CmsRet mdm_getParamAttributes(const MdmPathDescriptor *pathDesc,
                              MdmNodeAttributes *nodeAttr);



/** Set attributes for the specified parameter or object.
 *
 * If pathDesc contains an object name, then the attributes for the
 * sub-tree starting at the object name will be set.
 *
 * @param pathDesc  (IN) Path descriptor to the parameter whose attributes are
 *                       to be retreived.
 * @param nodeAttr  (IN) New attributes to set.
 * @param testOnly  (IN) If TRUE, check if the specified attributes can be set,
 *                       but don't actually do it.
 * @returns CmsRet enum.
 */
CmsRet mdm_setParamAttributes(const MdmPathDescriptor *pathDesc,
                              const MdmNodeAttributes *nodeAttr,
                              UBOOL8 testOnly);


/** Check if parameter value has changed.
 * 
 * Note that this function should only be called on parameters
 * which has active or passive notification attribute set.
 * Calling this function with a parameter which does not have
 * active or passive notification attribute will always result in
 * FALSE return value, even though the value may have been changed.
 *
 * @param *pathDesc (IN) Pointer to the parameter name path descriptor.
 * @return TRUE if the parameter value has been changed.  FALSE otherwise.
 */
UBOOL8 mdm_isParamValueChanged(const MdmPathDescriptor *pathDesc);


/** Get number of parameters whose value has changed and who has
 *  either passive or active change notification attribute.
 *
 * @return The total number of parameter value changes.
 */
UINT32 mdm_getNumberOfParamValueChanges(void);


/** Clear Parameter Value Change status for all values in the MDM.
 *
 * This function clears the status of all the parameter value changes.
 */
void mdm_clearAllParamValueChanges(void);


/** Check if the current entity is in the specified access bitmask.
 *
 * The current entity is the entity that called cmsMdm_init().
 *
 * @param accessBitMask (IN) The accessBitMask to check against.
 *
 * @return TRUE if the current entity is in the access bitmask, otherwise,
 *         FALSE.
 */
UBOOL8 mdm_isInAccessList(UINT16 accessBitMask);


/** Check if the caller is allowed to change the fields in the given MdmObject.
 * 
 * This function looks at every field in the given MdmObject, determines if the
 * field has changed from the value in the MDM, and then determines if the caller
 * is allowed to change that field by checking with the access list field of the
 * parameter attribute.
 *
 * @param mdmObj   (IN) The modified MdmObject to be checked.
 * @param iidStack (IN) The instance information for the modified MdmObject.
 * @return CmsRet enum.  Specifically, returns CMSRET_SUCCESS if the caller is
 *                       allowed to make the changes, CMSRET_REQUEST_DENIED if caller
 *                       is not allowed to make the changes.
 */
CmsRet mdm_checkAccessPermissions(const void *mdmObj,
                                  const InstanceIdStack *iidStack);
                                  


/** Get a MdmObject filled with default values.
 *
 * The object instance does not have to exist in the MDM.
 *
 * @param oid (IN) The OID of the MdmObject to get.
 * @param mdmObj (OUT) On successful return, pointer an MdmObject containing
 *                     default values for the object.  The caller is
 *                     responsible for freeing the object.
 * @return CmsRet enum.
 */
CmsRet mdm_getDefaultObject(MdmObjectId oid,
                            void **mdmObj);


/** Get a pointer to the MdmObject from the MDM.
 *
 * @param oid      (IN) The OID of the MdmObject to get.
 * @param iidStack (IN) If the desired MdmObject is a singleton,
 *                      iidStack may be NULL or empty.  If the
 *                      caller knows exactly which instance to get,
 *                      iidStack contains the exact instance info.
 * @param mdmObj  (OUT) If successful, a pointer to the internal MdmObject
 *                      is returned.  The caller may modify the object, but
 *                      the usual access list, notifications, validation checking
 *                      will not be done.  Caller must not free the mdmObj.
 *                      This function is for advanced MDM internal uses;
 *                      consider using mdm_getObject or one its variants instead.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_getObjectPointer(MdmObjectId oid,
                            const InstanceIdStack *iidStack,
                            void **mdmObj);



/** Get a copy of the MdmObject from the MDM.
 *
 * @param oid (IN) The OID of the MdmObject to get.
 * @param iidStack (IN) If the desired MdmObject is a singleton,
 *                      iidStack may be NULL or empty.  If the
 *                      caller knows exactly which instance to get,
 *                      iidStack contains the exact instance info.
 * @param mdmObj (OUT) If successful, a copy of the requested MdmObject will
 *                     be returned.  If caller makes any modifications to the
 *                     object, caller must call mdm_setObject for the changes
 *                     to be stored in the MDM.  Caller is responsible for
 *                     freeing the MdmObject.
 * @return CmsRet enum.
 */
CmsRet mdm_getObject(MdmObjectId oid,
                     const InstanceIdStack *iidStack,
                     void **mdmObj);




/** Get a copy of the next MdmObject instance.
 *
 * This function can only be used for objects that can have multiple
 * instances.  If an object can only have one instance, use mdm_getObject.
 *
 * @param oid (IN) The OID of the MdmObject to get.
 * @param iidStack (IN/OUT) If the iidStack is freshly initialized (empty),
 *                          then the first instance of the object will be
 *                          returned.  Otherwise, on successful return,
 *                          the iidStack is filled in with the instance id
 *                          information for the returned MdmObject.
 * @param mdmObj (OUT) If successful, a copy of the requested MdmObject will
 *                     be returned.  If caller makes any modifications to the
 *                     object, caller must call mdm_setObject for the changes
 *                     to be stored in the MDM.  Caller is responsible for
 *                     freeing the MdmObject.
 * @return CmsRet enum.
 */
CmsRet mdm_getNextObject(MdmObjectId oid,
                         InstanceIdStack *iidStack,
                         void **mdmObj);



/** Get a copy of the next MdmObject instance under the specified parent
 * instance.
 *
 * This function can only be used for objects that can have multiple
 * instances.  If an object can only have one instance, use mdm_getObject.
 *
 * @param oid (IN) The OID of the MdmObject to get.
 * @param parentIidStack (IN) If the iidStack of the parent sub-tree.
 *                            This parentIidStack constrains the search
 *                            of the getNextInSubTree function to only return
 *                            mdmObjects within a certain sub-tree.
 *                            This param may be NULL if no sub-tree checking is needed.
 * @param iidStack (IN/OUT) If the iidStack is freshly initialized (empty),
 *                          then the first instance of the object will be
 *                          returned.  Otherwise, on successful return,
 *                          the iidStack is filled in with the instance id
 *                          information for the returned MdmObject.
 * @param mdmObj (OUT) On successful return, pointer to MdmObject in
 *                     the MDM tree.  The caller must NOT modify the
 *                     object.  The caller must NOT free the object.
 * @param mdmObj (OUT) If successful, a copy of the requested MdmObject will
 *                     be returned.  If caller makes any modifications to the
 *                     object, caller must call mdm_setObject for the changes
 *                     to be stored in the MDM.  Caller is responsible for
 *                     freeing the MdmObject.
 * @return CmsRet enum.
 */
CmsRet mdm_getNextObjectInSubTree(MdmObjectId oid,
                                  const InstanceIdStack *parentIidStack,
                                  InstanceIdStack *iidStack,
                                  void **mdmObj);



/** Get a copy of the MdmObject that is the ancestor (higher in the sub-tree) of
 * the specified decendent object.
 *
 * Specifically, ancestor can be parent, grand parent, great-grand-parent,
 * or uncle, great-uncle, or great-great-uncle of the decendent.
 * But ancestor does not include the brothers of the decendent.
 * From the data model point of view, the ancestor object can be
 * uniquely identified with the information in the iidStack of the
 * decendent and the ancestor can claim that the decendent is in its sub-tree.
 *
 * @param ancestorOid (IN) The oid of the desired ancestor object.
 * @param decendentOid (IN) The oid of the decendent object.
 * @param iidStack (IN/OUT) On entry, this will be the iidStack of the
 *                          decendent object.  On successful return,
 *                          this will contain the iidStack of the 
 *                          ancestor object.
 * @param mdmObj (OUT) If successful, a copy of the requested MdmObject will
 *                     be returned.  If caller makes any modifications to the
 *                     object, caller must call mdm_setObject for the changes
 *                     to be stored in the MDM.  Caller is responsible for
 *                     freeing the MdmObject.
 * @return CmsRet enum.
 */
CmsRet mdm_getAncestorObject(MdmObjectId ancestorOid,
                             MdmObjectId decendentOid,
                             InstanceIdStack *iidStack,
                             void **mdmObj);



/** Validate a parameter value which is in string format.
 *
 * Note this function may convert the string to a binary type
 * if this is actually a binary type.
 *
 * @param pathDesc (IN) a MdmPathDescriptor for a parameter name.
 * @param strValue (IN) The string to be validated.
 * @return CmsRet enum.  Specifically, CMSRET_SUCCESS if the string
 *                       is valid, CMSRET_INVALID_ARGUMENTS if the
 *                       value is not valid, or some other error.
 */
CmsRet mdm_validateString(const MdmPathDescriptor *pathDesc,
                          const char *strValue);


/** Validates all fields in the given MdmObject.
 *
 * This function looks at every field in the given MdmObject, determines if the
 * field has changed from the value in the MDM, and then determines if the new
 * value is valid.
 *
 * @param mdmObj   (IN) The modified MdmObject to be checked.
 * @param iidStack (IN) The instance information for the modified MdmObject.
 * @return CmsRet enum.  Specifically, returns CMSRET_SUCCESS if the MdmObject
 *                       contains all valid values, and CMSRET_INVALID_PARAM_VALUE if
 *                       the MdmObject contains any invalid values.
 */
CmsRet mdm_validateObject(const void *mdmObj,
                          const InstanceIdStack *iidStack);
                                  

/** Given an objNode, return the size of the MdmObject described by the objNode.
 *
 * Basically, take the offsetInObject of the last parameter in the
 * objNode and round up to the next 4 byte boundary.
 * However, if the last parameter is a 64 bit type, then size if offsetInObject + 8.
 * If the objNode has no params, still allocate enough space to hold
 * a MdmObjectId, which is stuffed in the beginning of every MdmObject.
 * (e.g. WANDevice.{i}.X_BROADCOM_COM_ATM_Interface_Stats.)
 *
 * @param objNode (IN) The MdmObjectNode.
 *
 * @return size of the MdmObject.
 */
UINT32 mdm_getObjectSize(const MdmObjectNode *objNode);


/** Make a copy of an MdmObject.
 *
 * The function knows which object is given to it by the embedded
 * oid field in the object structure.
 *
 * @param obj   (IN) The MdmObject to copy.
 * @param flags (IN) flags to pass to cmsMem_alloc when allocating
 *                   memory for the new MdmObject.
 * @return void * pointer to the new MdmObject, or NULL.
 */
void *mdm_dupObject(const void *obj,
                    UINT32 flags);


/** Free a MdmObject.
 *
 * Free a MdmObject and any string buffers it is pointing to.
 * The function knows which object is given to it by the embedded
 * oid field in the object structure.
 *
 * @param mdmObj (IN) Address of the pointer to the MdmObject to be freed.
 */
void mdm_freeObject(void **mdmObj);


/** Set the MdmObject of the specified objNode at the specified iiStack.
 *
 * @param mdmObj (IN/OUT) The current MdmObject inside the MDM is freed,
 *                      and the given mdmObj is inserted directly into the MDM.
 *                      Therefore, on success, the mdmObj be "stolen" from the
 *                      caller and the mdmObj pointer will set to NULL by this
 *                      function.  On failure, the caller is responsible
 *                      for freeing the mdmObj.
 * @param iidStack (IN) The specific instance of the MdmObject to set.
 * @param doValueChanged (IN) Update valueChanged bits in parameter attributes
 *                            and generate active change notification if 
 *                            appropriate.
 * @return CmsRet enum.
 */
CmsRet mdm_setObject(void **mdmObj,
                     const InstanceIdStack *iidStack,
                     UBOOL8 doValueChanged);


/** Return the MdmObjectNode corresponding to the given oid.
 *
 * @param oid (IN) MdmObjectId.
 * @return pointer to MdmObjectNode or NULL if oid is invalid.
 */
MdmObjectNode *mdm_getObjectNode(MdmObjectId oid);


/** Return the MdmParamNode corresponding to the specified parameter name.
 *
 * @param oid       (IN) The MdmObjectId of the object containing the desired paramNode.
 * @param paramName (IN) The name of the parameter.
 * @return pointer to MdmParamNode or NULL if pathDesc is invalid.
 */
MdmParamNode *mdm_getParamNode(MdmObjectId oid, const char *paramName);


/** Set the value of the parameter in the given MdmObject.
 *
 * @param paramNode (IN) The MdmParamNode of the param to be set.
 * @param strValue  (IN) The string representation of the value to be set.  This
 *                       function does not modify or steal this string.
 *                       The caller is responsible for appropriate cleanup of
 *                       this string.
 * @param allocFlags(IN) If the value to be set is a string, the allocation flags
 *                       controls where the string buffer will be allocated from.
 * @param mdmObj(IN/OUT) The MdmObject which contains the param to be modified.
 * @return CmsRet enum.
 */
CmsRet mdm_setParamNodeString(const MdmParamNode *paramNode,
                              const char *strValue,
                              UINT32 allocFlags,
                              void *mdmObj);


/** Get the value of the parameter in the given MdmObject.
 *
 * @param paramNode (IN) The MdmParamNode of the param to be set.
 * @param mdmObj    (IN) The MdmObject containing the desired param.
 * @param allocFlags(IN) The allocation flags to use for allocating the
 *                       return string value buffer.
 * @param strValue (OUT) On successful return, this variable will point to
 *                       a string buffer containing the requested value.
 *                       The string buffer will be allocated from the memory
 *                       pool specified by the allocFlags argument.
 *                       The caller is reponsible for freeing this string buffer.
 * @return CmsRet enum.
 */
CmsRet mdm_getParamNodeString(const MdmParamNode *paramNode,
                              const void *mdmObj,
                              UINT32 allocFlags,
                              char **strValue);



/** callback function to be used in mdm_traverseParamNodes.  If callback function
 * returns FALSE, the traversal will be aborted.  */
typedef UBOOL8 (*MdmParamTraverseCallback)(const MdmParamNode *paramNode,
                                           void *newParamVal,
                                           void *currParamVal,
                                           void *cbContext);


/** Generic traversal function for all the parameter nodes and parameter
 *  values for a specified object node.
 *
 * @param oid      (IN) The object id whose parameters to traverse.
 * @param iidStack (IN) The iidStack which specifies the particular instance of
 *                      the mdmObj to use.
 * @param newMdmObj (IN/OUT) an optional new mdmObj to pass in.
 * @param cbFunc        (IN) If not NULL, this function will be called for every
 *                           param node in the object.
 * @param cbContext (IN/OUT) This pointer will be passed to the callback function
 *                           for use by the callback function.
 *
 * @return CmsRet enum.
 */
CmsRet mdm_traverseParamNodes(MdmObjectId oid,
                              const InstanceIdStack *iidStack,
                              void *newMdmObj,
                              MdmParamTraverseCallback cbFunc,
                              void *cbContext);




/** Get the value of the Order param from the MDM object.  Caller must
 *  ensure that the objNode is an Auto Order node by using the macro
 *  IS_AUTO_ORDER_NODE.
 *
 *  @param objNode  (IN) the MdmObjNode
 *  @param mdmObj   (IN) the object node to get the order value from
 *
 *  @return the value of the Order param
 */
UINT32 mdm_getOrderValue(const MdmObjectNode *objNode, const void *mdmObj);


/** Set the value of the Order param in the MDM object.  Caller must
 *  ensure that the objNode is an Auto Order node by using the macro
 *  IS_AUTO_ORDER_NODE.
 *
 *  @param objNode  (IN) the MdmObjNode
 *  @param mdmObj   (IN) the object node to set the order value
 *  @param orderVal (IN) the new Order value to set
 */
void mdm_setOrderValue(const MdmObjectNode *objNode, void *mdmObj, UINT32 orderVal);


/** Move the specified instance using the new Order value.  This function
 *  is called when a mdmObj gets its Order value changed so its position
 *  in the table must also change accordingly.
 *
 *  @param objNode  (IN) the MdmObjNode
 *  @param iidStack (IN) specifies the mdmObj to move
 *  @param newOrderVal (IN) use this new Order value when repositioning the
 *                          mdmObj.  Ignore the Order value that is currently
 *                          in the mdmObj (because it is about to be changed).
 */
void mdm_moveInstanceUsingNewOrderValue(const MdmObjectNode *objNode,
                                        const InstanceIdStack *iidStack,
                                        UINT32 newOrderVal);


/** Re-number the Order values in the table.  No RCL callbacks are done.
 *
 * This is needed after an object instance
 * is deleted and after the Order param of an object is changed.
 *
 * @param objNode  (IN) the MdmObjNode
 * @param iidStack (IN) specifies the mdmObj, but internally, this function
 *                      will pop off the last instance number because the
 *                      normalization is done over the entire table which
 *                      contains this instance
 */
void mdm_normalizeOrderValues(const MdmObjectNode *objNode, const InstanceIdStack *iidStack);




/** Write current MDM to a buffer and return the buffer to the caller.
 *
 * This function is provided to external apps as a debugging tool.
 * To write the current MDM configuration to flash, simply call
 * cmsMgm_saveConfigToFlash().
 *
 * @param buf (IN/OUT) Caller provides this buffer, and this function will fill
 *                     the buffer with the XML representation of the MDM.
 * @param len (IN/OUT) On entry, len contains the length of the buffer.
 *                     On successful exit, len contains the number of bytes
 *                     actually written.
 * @return CmsRet enum.
 */
CmsRet mdm_serializeToBuf(char *buf, UINT32 *len);



/** Write given object information to a buffer and return the buffer to the caller.
 *
 * This function is provided to external apps as a debugging tool.
 *
 * @param oid      IN) The object id whose parameters to traverse.
 * @param buf (IN/OUT) Caller provides this buffer, and this function will fill
 *                     the buffer with the XML representation of the MDM.
 * @param len (IN/OUT) On entry, len contains the length of the buffer.
 *                     On successful exit, len contains the number of bytes
 *                     actually written.
 * @return CmsRet enum.
 */
CmsRet mdm_serializeObjectToBuf(const MdmObjectId oid, char *buf, UINT32 *len);



/** validate a buf containing a config file.
 *
 * See description in cmsMgm_validateConfigBuf().
 *
 * @param buf (IN) buffer containing a config file in XML format.
 * @param len (IN) length of buffer.
 * @return cmsRet enum.
 */
CmsRet mdm_validateConfigBuf(const char *buf, UINT32 len);




/** Macro to set the depth of the instance id stack */
#define SET_IIDSTACK_DEPTH(s, d) ((s)->currentDepth = (d))


/** Macro to determine if the instance id stack is empty */
#define IS_IIDSTACK_EMPTY(s) ((s)->currentDepth == 0)


/** Return true if objNode2/iidStack2 is in a sub-tree of objNode1/iidStack1
 *
 * This might be only used by mdm.c, but it does not hurt to
 * export a generally useful function such as this.
 *
 * @param objNode1  (IN) pointer to higher MdmObjectNode.
 * @param iidStack1 (IN) pointer to higher instanceIdStack.
 * @param objNode2  (IN) pointer to potential sub-tree MdmObjectNode.
 * @param iidStack2 (IN) pointer to potential sub-tree instanceIdStack.
 * @return TRUE if objNode2/iidStack2 is in a sub-tree of objNode1/iidStack1,
 *         including if objNode2 == objNode1 and iidStack2 == iidStack1.
 *         Otherwise, return false.
 */
UBOOL8 mdm_isContainedInSubTree(const MdmObjectNode *objNode1,
                                const InstanceIdStack *iidStack1,
                                const MdmObjectNode *objNode2,
                                const InstanceIdStack *iidStack2);


/** Return true if pathDesc2 is in a sub-tree of pathDesc1
 *
 * This function just calls mdm_isContainedInSubTree().
 *
 * @param pathDesc1 (IN) pointer to higher MdmPathDescriptor.
 * @param pathDesc2 (IN) pointer to potential sub-tree MdmPathDescriptor.
 * @return TRUE if pathDesc2 is in a sub-tree of pathDesc1,
 *         including if pathDesc2 == pathDesc1.
 *         Otherwise, return false.
 */
UBOOL8 mdm_isPathDescContainedInSubTree(const MdmPathDescriptor *pathDesc1,
                                        const MdmPathDescriptor *pathDesc2);



/** Initialize parent back pointers in the tree.
 *  This is used during mdm_init to create backpointers in the MDM.
 *  This is a recursive function.
 *
 * @param (IN/OUT) Pointer to MdmObjectNode to operate on.
 */
void mdm_initParentPointers(MdmObjectNode *objNode);


/** Get the MdmOidInfoEntry for the given oid.
 *
 * @param oid (IN) requested OID.
 * @return ptr to MdmOidInfoEntry which must not be modified and not freed.
 */
const MdmOidInfoEntry *mdm_getOidInfo(MdmObjectId oid);


/** Return TRUE if the given eid is one of the system's special priviledged
 *  eid's that is allowed to bypass read/write rules of the data model.
 *
 * @param eid (IN) The eid to query.
 *
 * @return TRUE if the given eid is one of the system's special priviledged
 *  eid's that is allowed to bypass read/write rules of the data model.
 */
UBOOL8 mdm_isFullWriteAccessEid(CmsEntityId eid);


/** Delcared in mdm_binaryHelper.c.  Should use mdm_isFullWriteAccessEid to
 *  test an EID.  Don't access array directly.
 */
extern CmsEntityId fullWriteAccessEidArray[];


/** Allocate a MdmPathDescriptor structure.
 * This function allows mdm.o to be shipped as binary object.
 *
 * @return allocated MdmPathDescriptor structure.
 */
MdmPathDescriptor *mdm_allocatePathDescriptor(UINT32 flags);


/** Return the MDM_MAX_OID.
 * This function allows mdm.o to be shipped as binary object.
 *
 * @return the MDM_MAX_OID.
 */
UINT32 mdm_getMaxOid(void);


/** Return the MAX_MDM_INSTANCE_DEPTH.
 * This function allows mdm.o to be shipped as binary object.
 *
 * @return the MAX_MDM_INSTANCE_DEPTH.
 */
UINT32 mdm_getMaxInstanceDepth(void);


/** Return the MAX_MDM_PARAM_NAME_LENGTH.
 * This function allows mdm.o to be shipped as binary object.
 *
 * @return the MAX_MDM_PARAM_NAME_LENGTH
 */
UINT32 mdm_getMaxParamNameLength(void);


/** Initialize the MdmPathDescriptor structure.
 * This function allows mdm.o to be shipped as binary object.
 *
 * @param OUT MdmPathDescriptor to initialize.
 */
void mdm_initPathDescriptor(MdmPathDescriptor *pathDesc);


/** Initialize the paramName field in the MdmPathDescriptor structure.
 * This function allows mdm.o to be shipped as binary object.
 *
 * @param OUT paramName to initialize.
 */
void mdm_initParamName(char *paramName);


/** Get the default access bit mask.
 * This function allows mdm.o to be shipped as binary object.
 *
 * @return the default access bit mask.
 */
UINT16 mdm_getDefaultAccessBitMask(void);


/** Send a CMS_MSG_INTFSTACK_LOWERLAYER_CHANGED to ssk.
 * in mdm_binaryhelper.c
 *
 */
extern CmsRet mdm_sendLowerLayersChangedMsg(MdmObjectId oid,
                                            const InstanceIdStack *iidStack,
                                            const char *newLowerLayersStr,
                                            const char *currLowerLayersStr);


/** Send a CMS_MSG_INTFSTACK_ALIAS_CHANGED to ssk.
 * in mdm_binaryhelper.c
 *
 */
extern CmsRet mdm_sendAliasChangedMsg(MdmObjectId oid,
                                      const InstanceIdStack *iidStack);


/** Send a CMS_MSG_OBJECT_DELETED to ssk.
 * in mdm_binaryhelper.c
 */
CmsRet mdm_sendObjectDeletedMsg(MdmObjectId oid,
                                const InstanceIdStack *iidStack);


/** Validate MDM object sequence number (in mdm_binaryhelper.c).
 * When an app gets a MDM object, the object has a sequence number.  If the
 * app writes the object back to the MDM, the sequence number is checked
 * against the internal MDM sequence number for that object.  If they do not
 * match, that means another app has written to the object between this app's
 * read and write. See also mdm_allowSequenceNumError which controls
 * the behavior of this function.
 * @return CMSRET_SUCCESS if valid, otherwise CMSRET_OBJECT_SEQUENCE_ERROR.
 */
CmsRet mdm_validateSequenceNum(MdmObjectId oid, const InstanceIdStack *iidStack,
                    UINT16 currSeqNum, UINT16 newSeqNum,
                    SINT32 lastWritePid, const CmsTimestamp *lastWriteTs);
                    
                    
/** Normally, sequence number errors are not allowed, but this function can
 *  be modified/customized to allow it.  (in mdm_binaryhelper.c)
 */
UBOOL8 mdm_allowSequenceNumError();


/*
 * All of the lck_auto* functions below are in lock.c and provided to the
 * OBJ, PHL, and MGM layers to (auto) lock the appropriate zones for the caller.
 */
CmsRet lck_autoLockZone(MdmObjectId oid, const char *where);
CmsRet lck_autoLockZones(const UBOOL8 *zones,
                         MdmObjectId oid, const char *where);
CmsRet lck_autoLockAllZonesWithBackoff(MdmObjectId oid,
                          UINT32 timeoutMilliSeconds, const char *where);

void lck_autoUnlockZone(MdmObjectId oid, const char *where);
void lck_autoUnlockZones(const UBOOL8 *zones,
                         MdmObjectId oid, const char *where);
void lck_autoUnlockAllZones(const char *where);
void lck_autoTrackMdmObj(const void *mdmObj);
void lck_autoUntrackMdmObj(const void *mdmObj);


/** Return total number of times an app has entered into all MDM lock zones.
 *  Usually, the code just wants to see if the count is 0 or not.
 */
UINT32 lck_getTotalZoneEntryCounts();

/** Macro to determine if we are being called from an external application or
*   from inside the MDM (rcl/rut/qdm).
*/
#define CHECK_MDM_EXTERNAL_CALLER(where) if (lck_getTotalZoneEntryCounts() > 0) {\
               cmsLog_error("%s cannot be called from inside MDM", where); \
               return CMSRET_RECURSION_ERROR; }

#define CHECK_MDM_EXTERNAL_CALLER_0(where) if (lck_getTotalZoneEntryCounts() > 0) {\
               cmsLog_error("%s cannot be called from inside MDM", where); \
               return 0; }

#define CHECK_MDM_EXTERNAL_CALLER_V(where) if (lck_getTotalZoneEntryCounts() > 0) {\
               cmsLog_error("%s cannot be called from inside MDM", where); \
               return; }

#define IS_EXTERNAL_CALLER (lck_getTotalZoneEntryCounts() == 0)

/** Maximum times an app can enter into a MDM lock zone.
 * The variable that tracks the entry count is an UINT8, hence this limit.
 * This is used to detect out-of-control recursions.
 */
#define MDM_MAX_ENTRY_COUNT   255


/** Struct to hold all lock related structs in MdmShmCtx */
typedef struct
{
   CmsLockOwnerInfo  owners[MDM_MAX_LOCK_ZONES];
   CmsLockThreadInfo threads[MDM_MAX_LOCK_THREADS];
   CmsLockStats      stats;
   UINT32            traceLevel;  /**< For now, just 0 or 1 */
} MdmLockMetaInfo;

/** This structure contains information needed by users of the shared
 *  memory and is placed at the beginning of the shared memory region.
 *
 * Do not use ifdefs to conditionally compile fields in or out.
 * Prevents the mdm.o binary only module from breaking at customer site.
 */
typedef struct
{
   MdmObjectNode **oidNodePtrTable; /** Table of pointers to MdmObjectNodes. */
   MdmObjectNode *rootObjNode;  /**< The InternetGatewayDevice node. */
   char         *stringsStart;  /**< Start of strings area. */
   void          *mallocStart;  /**< Start of region used for shared memory alloc. */
   char          *shmEnd;       /**< First byte after shared memory region, so
                                 *   all valid addresses must be less than this. */
   UINT32 numValueChanges;  /**< Number of parameters with active or passive notification 
                             *   that has had their value changed by a management entity
                             *   other than tr69c.
                             */
   UBOOL8         inMdmInit;    /**< TRUE if we are currently initializing the MDM. */
   UBOOL8         dslInitDone;  /**< TRUE if rutDsl_configUp() has been called once already */
   UBOOL8         mocaInitDone; /**< TRUE if moca initialize has been called once already */
   UINT8    isDataModelDevice2; /**< 1: root=Device; 0: root=InternetGatewayDevice */
   MdmLockMetaInfo *lockMeta;    /**< MDM zone lock meta-info table */
} MdmSharedMemContext;


/** Data shared among the various layers and files of the cms_core library.
 *
 *  This structure just keeps all these global variables in one place.
 */
typedef struct
{
   UBOOL8   initDone; /**< Has cms_core been initialized yet. */
   CmsEntityId eid; /**< CmsEntityId of the user of this library.
                     *   This was set during cmsMdm_init(). */
   UINT16 accessBit;/**< Access bit for the entity, set during cmsMdm_init().
                     *   This comes from the CmsEntityInfo structure for this entity */
   SINT32      pid; /**< Pid of the user of this library. */
   void *msgHandle; /**< Message Handle of the user.
                     *   The RCL/STL handler functions may need to use the
                     *   message handle to send requests, notifications, etc. */

   SINT32 shmId;    /**< The share memory id for the MDM.
                     *   This is only used if CMS_SHARED_MEM is defined. */

   void *shmAddr;   /**< The address where shared memory is attached.
                     *   This is only used if CMS_SHARED_MEM is defined. */

   UINT32 allocFlags; /**< Flags to use during memory allocations which may
                       *   need to be in shared memory. */
   UBOOL8  dumpAll;   /**< We are currently dumping the entire MDM instead of just
                       *   the config. */
   UBOOL8  hideObjectsPendingDelete; /**< by default this is TRUE, so odl_getNextInSubTree
                                      *   will skip over any object that is pending delete */
} MdmLibraryContext;


/** Global data for the cms_core lib, accessible by all layers in the cms_core library. */
extern MdmLibraryContext mdmLibCtx;

/** Info stored in shared mem, accessible by all layers in the cms_core library. */
extern MdmSharedMemContext *mdmShmCtx;



#endif /* __MDM_H__ */
