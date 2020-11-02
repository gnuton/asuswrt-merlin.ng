/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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


#ifndef __ODL_H__
#define __ODL_H__

/*!\file odl.h
 * \brief header file for Object Dispatch Layer functions.
 * The interfaces in this file are only used by the phl, obj, rcl and stl.
 * Management entities should not include this file or call any
 * of its functions.
 */

#include "mdm.h"
#include "cms_phl.h"


/** Initialize the odl.
 *
 * @return CmsRet enum;
 */
CmsRet odl_init(void);


/** Clean up the odl.
 *
 */
void odl_cleanup(void);


/** Set (write) an array of parameter values.
 *
 * This is used by the Parameter Handling Layer (PHL).
 *
 * @param paramValueArray (IN) Array of OdlParamValue structures.
 * @param numParamValues  (IN) Number of OdlParamValue structures in the array.
 * @return CmsRet enum.
 */
CmsRet odl_set(PhlSetParamValue_t *paramValueArray,
               UINT32 numParamValues);


/** Set a new MdmObject.
 *
 * This is used by the object layer to change an object.  This is only used
 * on the first call from an upper layer management entity such as httpd
 * or tr69c.
 *
 * @param newMdmObj  (IN) The new MdmObject values.  Regardless of success or failure,
 *                        the caller must deal with (e.g. free) this object.
 *                        This function will not steal this object from the caller.
 * @param iidStack   (IN) Instance information to for the MdmObject.
 * @return CmsRet enum.
 */
CmsRet odl_setObjectExternal(const void *newMdmObj,
                             const InstanceIdStack *iidStack);


/** Set a new MdmObject.
 *
 * This is used by the object layer to change an object.  This is used on
 * subsequent/nested calls to cmsObj_set's, usually by RCL and RUT functions.
 * Also called directly by some internal MDM functions.
 *
 * @param newMdmObj  (IN) The new MdmObject values.  Regardless of success or failure,
 *                        the caller must deal with (e.g. free) this object.
 *                        This function will not steal this object from the caller.
 * @param currMdmObj (IN) The current MdmObject values.  This function will
 *                        not free this object.  The caller must deal (e.g.
 *                        free) this object.
 * @param iidStack   (IN) Instance information to for the MdmObject.
 * @return CmsRet enum.
 */
CmsRet odl_setObjectInternal(const void *newMdmObj,
                             const void *currMdmObj,
                             const InstanceIdStack *iidStack);


/** Set a modified MdmObject without without doing RCL callback.
 *
 * This function can be called as the first set in a set sequence, or
 * in the middle of a set sequence.  If this is the first set in a set
 * sequence, then the set queue must be empty, and the newMdmObj will be
 * pushed into the MDM immediately with no RCL callback.
 * If this is not the first set in a set sequence, then newMdmObj may or
 * may not be in the set queue.  If newMdmObj is not in the set queue, then
 * the newMdmObj will be pushed into the MDM immediately with no RCL callback.
 * However, if the newMdmObj is currently in the set queue, then it will
 * not be pushed into the MDM immediately; rather, newMdmObj will be pushed
 * into the MDM by the caller who put the object in the set queue.
 *
 * @param newMdmObj  (IN) The new MdmObject values.  Regardless of success or
 *                        failure, the caller must deal with (e.g. free)
 *                        this object.  This function will not steal this
 *                        object from the caller.
 * @param iidStack   (IN) Instance information to for the MdmObject.
 * @return CmsRet enum.
 */
CmsRet odl_setObjectNoRclCallback(const void *newMdmObj,
                                  const InstanceIdStack *iidStack);


/** Get (read) a value from the MDM with flags.
 *
 * This is used by the Parameter Handling Layer (PHL).
 *
 * @param pathDesc (IN) param to be read.
 * @param getFlags (IN) Same as getFlags in cmsObj_get().
 * @param value (OUT) On successful return, contains a pointer to the
 *                    string representation of the requested value.
 *                    The caller is responsible for freeing the buffer.
 * @return CmsRet enum.
 */
CmsRet odl_getFlags(const MdmPathDescriptor *pathDesc,
                    UINT32 getFlags,
                    char **value);

#define odl_get(p,v) odl_getFlags(p,0,v)


/** Get a copy of the MdmObject from the MDM, updated with the latest values
 *  as reported by the STL.
 *
 * This is used by the object layer.
 * This is also used by the RCL and STL to read current values from the MDM
 * because it looks for the object in the set queue.
 * This function will call the STL handler function to get the latest
 * values for the mdmObject.
 *
 * @param oid      (IN) Oid of the object to get.
 * @param iidStack (IN) Instance information for the object to get.
 * @param getFlags (IN) Same as getFlags in cmsObj_get().
 * @param mdmObj (OUT) If successful, a copy of the requested MdmObject will
 *                     be returned.  If caller makes any modifications to the
 *                     object, caller must call odl_setObject() for the changes
 *                     to the stored in the MDM.  The caller is responsible
 *                     for freeing this object by calling odl_freeObject().
 * @return CmsRet enum.
 */
CmsRet odl_getObject(MdmObjectId oid,
                     const InstanceIdStack *iidStack,
                     UINT32 getFlags,
                     void **mdmObj);


/** Get a copy of the MdmObject from the MDM, filled with default values.
 *
 * This is used by the object layer, and RCL/STL layers.
 *
 * @param oid      (IN) Oid of the object to get.
 * @param mdmObj (OUT) If successful, a copy of the requested MdmObject will
 *                     be returned.  The caller is responsible
 *                     for freeing this object by calling odl_freeObject().
 * 
 * @return CmsRet enum.
 */
CmsRet odl_getDefaultObject(MdmObjectId oid, void **mdmObj);



/** Get a copy of the next MdmObject instance, updated with the latest
 *  values as reported by the STL.
 *
 * This function can only be used for objects that can have multiple
 * instances.  If an object can only have one instance, use odl_getObject.
 * This is used by the RCL and STL to read current values from the MDM.
 * This function will call the STL handler function to get the latest
 * values for the mdmObject.
 *
 * @param oid (IN) The OID of the MdmObject to get.
 * @param iidStack (IN/OUT) If the iidStack is freshly initialized (empty),
 *                          then the first instance of the object will be
 *                          returned.  Otherwise, on successful return,
 *                          the iidStack is filled in with the instance id
 *                          information for the returned MdmObject.
 * @param getFlags (IN) Same as getFlags in cmsObj_getNextObject().
 * @param mdmObj (OUT) If successful, a copy of the requested MdmObject will
 *                     be returned.  If caller makes any modifications to the
 *                     object, caller must call odl_setObject() for the changes
 *                     to be stored in the MDM.  Caller is responsible for
 *                     freeing this object by calling odl_freeObject().
 * @return CmsRet enum.
 */
CmsRet odl_getNextObject(MdmObjectId oid,
                         InstanceIdStack *iidStack,
                         UINT32 getFlags,
                         void **mdmObj);



/** Get a copy of the next MdmObject instance under the specified parent
 * instance, updated with the latest values as reported by the STL.
 *
 * This function can only be used for objects that can have multiple
 * instances.  If an object can only have one instance, use odl_getObject.
 * This is used by the RCL and STL to read current values from the MDM.
 * This function will call the STL handler function to get the latest
 * values for the mdmObject.
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
 * @param getFlags (IN) Same as getFlags in cmsObj_getNextInSubTree().
 * @param mdmObj (OUT) If successful, a copy of the requested MdmObject will
 *                     be returned.  If caller makes any modifications to the
 *                     object, caller must call odl_setObject() for the changes
 *                     to be stored in the MDM.  Caller is responsible for
 *                     freeing the object by calling odl_freeObject().
 * @return CmsRet enum.
 */
CmsRet odl_getNextObjectInSubTree(MdmObjectId oid,
                                  const InstanceIdStack *parentIidStack,
                                  InstanceIdStack *iidStack,
                                  UINT32 getFlags,
                                  void **mdmObj);



/** Get a copy of the MdmObject that is the ancestor (higher in the sub-tree) of
 * the specified decendent object, updated by the latest values as reported
 * by the STL.
 *
 * See the description in mdm_getAncestorObject().
 *
 * @param ancestorOid (IN) The oid of the desired ancestor object.
 * @param decendentOid (IN) The oid of the decendent object.
 * @param iidStack (IN/OUT) On entry, this will be the iidStack of the
 *                          decendent object.  On successful return,
 *                          this will contain the iidStack of the 
 *                          ancestor object.
 * @param getFlags (IN) Same as getFlags in cmsObj_getAncestorObject().
 * @param mdmObj (OUT) If successful, a copy of the requested MdmObject will
 *                     be returned.  If caller makes any modifications to the
 *                     object, caller must call odl_setObject() for the changes
 *                     to be stored in the MDM.  Caller is responsible for
 *                     freeing the object by calling odl_freeObject().
 * @return CmsRet enum.
 */
CmsRet odl_getAncestorObject(MdmObjectId ancestorOid,
                             MdmObjectId decendentOid,
                             InstanceIdStack *iidStack,
                             UINT32 getFlags,
                             void **mdmObj);



/** Add an instance of an object.
 *
 * @param pathDesc (IN/OUT) The object where the new instance is to be created.
 *                          On successful return, the iidStack of the pathDesc
 *                          will contain the newly created instance number.
 *
 * @return CmsRet enum.
 */
CmsRet odl_addObjectInstance(MdmPathDescriptor *pathDesc);




/** Delete an instance of an object.
 *
 * @param pathDesc (IN) The object to be deleted.  Note that the iidStack of the
 *                      pathDesc is not modified by this function.
 *
 * @return CmsRet enum.
 */
CmsRet odl_deleteObjectInstance(const MdmPathDescriptor *pathDesc);



/** Free a MdmObject while checking for the object in the set queue.
 *
 * This is used by the RCL handler functions to free a MdmObject.
 * If the MdmObject to be freed is currently in the set queue, then the
 * free will not occur immediately; instead, it will be freed by odl_set.
 * If the specified MdmObject is not in the set queue, then it is
 * freed immediately via mdm_freeObject().
 *
 * @param mdmObj (IN/OUT) Address of pointer to the MdmObject to be freed.
 */
void odl_freeObject(void **mdmObj);


/** Modifies flags for an Instance  
 *
 * @param objNode   (IN) The mdmObj which is being queried.
 * @param iidStack (IN) The iidStack of the mdmObj which is being queried.
 * @param flags  (IN) flags to be set.
 *
 * @return CmsRet enum.
 */
CmsRet odl_setInstanceFlags(MdmObjectId oid, const InstanceIdStack *iidStack, UINT16 flags);

/** Return flags for an Instance  
 *
 * @param objNode  (IN) The mdmObj which is being queried.
 * @param iidStack (IN) The iidStack of the mdmObj which is being queried.
 * @param flags  (OUT) current flags.
 *
 * @return CmsRet enum.
 */
CmsRet odl_getInstanceFlags(MdmObjectId oid, const InstanceIdStack *iidStack, UINT16 *flags);

/** Clear/zeroize all statistics in this object.
 *
 * This function is only used by the object layer, which operates on entire objects.
 * There is no function call that allows the caller to clear only a single statistic.
 *
 * @param oid      (IN) The oid of the object whose statistics are to be cleared.
 * @param iidStack (IN) Instance information for the object.
 * @return CmsRet enum.
 */
CmsRet odl_clearStatistics(MdmObjectId oid,
                           const InstanceIdStack *iidStack);

#endif /* __ODL_H__ */
