/***********************************************************************
 *
 *  Copyright (c) 2007-2013  Broadcom Corporation
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

#include "cms.h"
#include "odl.h"
#include "cms_obj.h"
#include "cms_lck.h"
#include "cms_util.h"
#include "sysutil.h"
#include "mdm.h"

// both of these are from locks.c (they are not for general use, just odl.c)
extern CmsLockThreadInfo *lck_getExistingLockThread(SINT32 tid);
extern SINT32 pidNsOffset;


/*
 * One element in the set queue.
 */
typedef struct odl_set_entry
{
   struct odl_set_entry *prev;
   struct odl_set_entry *next;
   MdmObjectNode *objNode;
   InstanceIdStack iidStack;
   UBOOL8 orderValueChanged;
   void *newMdmObj;
   void *currMdmObj;
} OdlSetQueueEntry;


/** timeout mdmObj after 50 milli-seconds */
#define ODL_GET_CACHE_TIMEOUT 50


/** some macros to keep track of which function we are currently in */
#define ODL_RECORD_CALL_ENTRY(fcode, eoid) \
      zone = cmsLck_getLockZone(eoid); \
      lastFuncCode = mdmShmCtx->lockMeta->owners[zone].currFuncCode; \
      lastOid = mdmShmCtx->lockMeta->owners[zone].currOid; \
      mdmShmCtx->lockMeta->owners[zone].currFuncCode = (UINT8) (fcode); \
      mdmShmCtx->lockMeta->owners[zone].currOid = (eoid)

#define ODL_RECORD_CALL_EXIT \
      mdmShmCtx->lockMeta->owners[zone].currFuncCode = lastFuncCode;  \
      mdmShmCtx->lockMeta->owners[zone].currOid = lastOid


/* local functions */
static CmsLockThreadInfo *getMyLockThread(void);
static void freeSetQueue(OdlSetQueue *setQ);
void freeSetQueueEntry(OdlSetQueue *setQ, OdlSetQueueEntry **entry);
static void reverseSetQueue(const OdlSetQueue *setQ);
static OdlSetQueueEntry *getSetQueueEntry(OdlSetQueue *setQ,
                                          MdmObjectNode *objNode,
                                          const InstanceIdStack *iidStack);
static UBOOL8 findInSetQueue(const OdlSetQueue *setQ,
                             MdmObjectId oid, const InstanceIdStack *iidStack,
                             void **mdmObj);
static UBOOL8 isPointerInSetQueue(const OdlSetQueue *setQ, const void *mdmObj);
static CmsRet callSetHandlerFunc(MdmObjectId oid,
                                 const InstanceIdStack *iidStack,
                                 void *newMdmObj,
                                 const void *currMdmObj,
                                 char **errorParam,
                                 CmsRet *errorCode);
static CmsRet callGetHandlerFunc(MdmObjectId oid,
                                 const InstanceIdStack *iidStack,
                                 void *mdmObj);
static CmsRet callGetHandlerAndUpdateMdm(CmsLockThreadInfo *thread,
                                         MdmObjectId oid,
                                         const InstanceIdStack *iidStack,
                                         void **mdmObj);
static void undoAllInstanceMovesUsingNewOrderValues(const OdlSetQueue *setQ);


CmsRet odl_init(void)
{
   return CMSRET_SUCCESS;
}

void odl_cleanup(void)
{
   /* nothing to do here. */
   return;
}


CmsRet odl_set(PhlSetParamValue_t *paramValueArray,
               UINT32 numParamValues)
{
   UINT32 i;
   UINT32 numSetEntries=0;
   CmsLockThreadInfo *thread;
   OdlSetQueue *setQ;
   MdmObjectNode *objNode;
   MdmParamNode *paramNode;
   CmsRet ret=CMSRET_SUCCESS;;

   thread = getMyLockThread();
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread for tid %d", sysUtl_gettid());
      return CMSRET_INTERNAL_ERROR;
   }
   
   INVALIDATE_ODL_GET_CACHE;
   setQ = &(thread->setQ);

   for (i=0; (i < numParamValues) && (ret == CMSRET_SUCCESS); i++)
   {
      PhlSetParamValue_t *paramEntry;
      OdlSetQueueEntry *setEntry;

      paramEntry = &(paramValueArray[i]);
      paramEntry->status = CMSRET_SUCCESS;

      if (i == 0)
      {
         /*
          * make sure there is nothing left over in the set queue.
          * If there is, free it, but we can still continue on.
          * XXX is this too restrictive?  This means we cannot call
          * cmsPhl_set from inside RCL/STL/RUT.  So far, not a problem.
          */
         if (setQ->count > 0)
         {
            cmsLog_error("still (%d) entries left in the queue, free all.",
                         setQ->count);
            cmsAst_assert(0);
            freeSetQueue(setQ);
         }
      }

      cmsLog_debug("getting oid %d param %s", paramEntry->pathDesc.oid, paramEntry->pathDesc.paramName);

      if (((objNode = mdm_getObjectNode(paramEntry->pathDesc.oid)) == NULL) ||
          ((paramNode = mdm_getParamNode(paramEntry->pathDesc.oid, paramEntry->pathDesc.paramName)) == NULL))
      {
         paramEntry->status = CMSRET_INVALID_PARAM_NAME;
         undoAllInstanceMovesUsingNewOrderValues(setQ);
         freeSetQueue(setQ);
         return CMSRET_INVALID_ARGUMENTS;
      }

      /* either find an existing set entry or create a new one */
      if ((setEntry = getSetQueueEntry(setQ, objNode, &(paramEntry->pathDesc.iidStack))) == NULL)
      {
         cmsLog_error("alloc of setEntry failed.");
         paramEntry->status = CMSRET_RESOURCE_EXCEEDED;
         undoAllInstanceMovesUsingNewOrderValues(setQ);
         freeSetQueue(setQ);
         return CMSRET_RESOURCE_EXCEEDED;
      }

      if (setEntry->newMdmObj == NULL)
      {
         cmsLog_debug("new setEntry for oid %d %s %s",
                      setEntry->objNode->oid,
                      mdm_oidToGenericPath(setEntry->objNode->oid),
                      cmsMdm_dumpIidStack(&(setEntry->iidStack)));

         numSetEntries++;

         /*
          * Use OGF_NO_VALUE_UPDATE to get the newMdmObj directly from MDM.
          * Do not trigger the STL handler func since we only want
          * the current state of the MDM object.
          */
         ret = odl_getObject(setEntry->objNode->oid,
                             &(setEntry->iidStack),
                             OGF_NO_VALUE_UPDATE,
                             &(setEntry->newMdmObj));

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("could not get mdmObject for %s %s, ret=%d",
                         mdm_oidToGenericPath(setEntry->objNode->oid),
                         cmsMdm_dumpIidStack(&(setEntry->iidStack)),
                         ret);
            paramEntry->status = ret;
            undoAllInstanceMovesUsingNewOrderValues(setQ);
            freeSetQueue(setQ);
            return ret;
         }
         else
         {
            setEntry->currMdmObj = mdm_dupObject(setEntry->newMdmObj, mdmLibCtx.allocFlags);
            if ((setEntry->currMdmObj == NULL))
            {
               cmsLog_error("malloc failed when duping newMdmObj");
               paramEntry->status = CMSRET_RESOURCE_EXCEEDED;
               undoAllInstanceMovesUsingNewOrderValues(setQ);
               freeSetQueue(setQ);
               return CMSRET_RESOURCE_EXCEEDED;
            }
         }
      }

      /*
       * Now modify the parameter in the newMdmObj.
       * The PHL has already validated this string value, so by the
       * time we get here, we know that paramEntry->pValue is valid.
       */
      ret = mdm_setParamNodeString(paramNode, paramEntry->pValue, mdmLibCtx.allocFlags, setEntry->newMdmObj);
      paramEntry->status = ret;

      /*
       * Check if we need to deal with auto order.
       */
      if ((ret == CMSRET_SUCCESS) &&
          (IS_AUTO_ORDER_NODE(objNode)) &&
          !cmsUtl_strcmp(paramNode->name, "Order"))
      {
         UINT32 newOrderVal = mdm_getOrderValue(objNode, setEntry->newMdmObj);
         cmsLog_debug("detected change, newOrderVal=%d", newOrderVal);
         mdm_moveInstanceUsingNewOrderValue(objNode, &(setEntry->iidStack), newOrderVal);
         setEntry->orderValueChanged = TRUE;
      }

   }  /* end of for loop over paramValueArray */


   /*
    * At this point, we have a set queue filled with modified objects
    * and ready to be sent to the RCL handler functions.  The objects
    * are put in the queue in the same order as they appeared in the
    * paramValueArray.  In the unlikely event that the ACS sent the objects
    * in an order that is incompatible with the order that our system/
    * RCL handler functions expect them to be in, we can modify the
    * order of the objects in the queue at this point.
    */

   if (ret == CMSRET_SUCCESS)
   {
      OdlSetQueueEntry *forwardSetEntry = (OdlSetQueueEntry *) setQ->head;
      OdlSetQueueEntry *backwardSetEntry;
      char *errorParam = NULL;
      CmsRet errorCode = CMSRET_SUCCESS;

      while ((forwardSetEntry != NULL) && (IS_CMSRET_A_SUCCESS_VARIANT(ret)))
      {
         cmsLog_debug("calling rcl for %s %s newObj=%p currObj=%p",
                      mdm_oidToGenericPath(forwardSetEntry->objNode->oid),
                      cmsMdm_dumpIidStack(&(forwardSetEntry->iidStack)),
                      forwardSetEntry->newMdmObj,
                      forwardSetEntry->currMdmObj);

         ret = callSetHandlerFunc(forwardSetEntry->objNode->oid,
                                  &(forwardSetEntry->iidStack),
                                  forwardSetEntry->newMdmObj,
                                  forwardSetEntry->currMdmObj,
                                  &errorParam,
                                  &errorCode);

         if (!(IS_CMSRET_A_SUCCESS_VARIANT(ret)))
         {
            /* now we have to find the errorParam in the paramValueArray
             * that caused the error and set status appropriately.
             */
            UBOOL8 foundErrorParam=FALSE;
            PhlSetParamValue_t *paramEntry=NULL;

            for (i=0; (i < numParamValues) && (!foundErrorParam); i++)
            {
               paramEntry = &(paramValueArray[i]);

               if ((errorParam != NULL) &&
                   (forwardSetEntry->objNode->oid == paramEntry->pathDesc.oid) &&
                   (cmsMdm_compareIidStacks(&(forwardSetEntry->iidStack), &(paramEntry->pathDesc.iidStack)) == 0) &&
                   (!strcasecmp(paramEntry->pathDesc.paramName, errorParam)))
               {
                  paramEntry->status = (errorCode != CMSRET_SUCCESS) ?
                                       errorCode : ret;
                  if (!(IS_CMSRET_A_TR69C_VARIANT(paramEntry->status)))
                  {
                     paramEntry->status = CMSRET_INTERNAL_ERROR;
                  }
                  foundErrorParam = TRUE;
               }
            }

            if (!foundErrorParam)
            {
               if (errorParam == NULL)
               {
                  cmsLog_error("rcl did not specify error param name on error %d on %s",
                               ret,
                               mdm_oidToGenericPath(forwardSetEntry->objNode->oid));
               }
               else
               {
                  cmsLog_error("could not find rcl returned error param name %s in %s",
                               errorParam,
                               mdm_oidToGenericPath(forwardSetEntry->objNode->oid));
               }

               /* find the first parameter name in the current object and
                * assign error to that parameter.
                */
               for (i=0; (i < numParamValues) && (!foundErrorParam); i++)
               {
                  paramEntry = &(paramValueArray[i]);

                  if ((forwardSetEntry->objNode->oid == paramEntry->pathDesc.oid) &&
                      (cmsMdm_compareIidStacks(&(forwardSetEntry->iidStack), &(paramEntry->pathDesc.iidStack)) == 0))
                  {
                     paramEntry->status = ret;
                     if (!(IS_CMSRET_A_TR69C_VARIANT(paramEntry->status)))
                     {
                        paramEntry->status = CMSRET_INTERNAL_ERROR;
                     }
                     foundErrorParam = TRUE;
                  }
               }

               if (foundErrorParam)
               {
                  cmsLog_error("setting error %d on %s",
                               paramEntry->status, paramEntry->pathDesc.paramName);
               }
               else
               {
                  cmsLog_error("Could not even find the object to assign error to, %s %s!",
                               mdm_oidToGenericPath(forwardSetEntry->objNode->oid),
                               cmsMdm_dumpIidStack(&(forwardSetEntry->iidStack)));
                  cmsAst_assert(0);
               }
            }

            /*
             * Since we have failed, undo any re-ordering of the instances
             * that we have done prior to the set.  Must do this before
             * reverseSetQueue.
             */
            undoAllInstanceMovesUsingNewOrderValues(setQ);

            /* 
             * Now we have to roll back by calling the rcl handler funcs
             * which we have called with MdmObject parameters reversed.
             */
            reverseSetQueue(setQ);
            backwardSetEntry = forwardSetEntry->prev;
            while (backwardSetEntry != NULL)
            {
               CmsRet backwardRet;

               backwardRet = callSetHandlerFunc(backwardSetEntry->objNode->oid,
                                                &(backwardSetEntry->iidStack),
                                                backwardSetEntry->newMdmObj,
                                                backwardSetEntry->currMdmObj,
                                                &errorParam,
                                                &errorCode);

               /*
                * ret already contains the error encountered during the set.
                * We don't much care about any errors encountered during
                * the undo, except for CMSRET_FAIL_REBOOT_REQUIRED.
                */
               if (backwardRet == CMSRET_FAIL_REBOOT_REQUIRED)
               {
                  ret = backwardRet;
               }

               backwardSetEntry = backwardSetEntry->prev;

            } /* end of while loop over backwardSetEntry */
         }


         forwardSetEntry = forwardSetEntry->next;

      } /* end of while loop to set all entries in set queue */

      if (IS_CMSRET_A_SUCCESS_VARIANT(ret))
      {
         /*
          * All the RCL handler functions have returned success,
          * so now we can update the MDM with all the objects.
          */
         OdlSetQueueEntry *forwardSetEntry = (OdlSetQueueEntry *) setQ->head;

         while (forwardSetEntry != NULL)
         {
            CmsRet r2;

            r2 = mdm_setObject(&(forwardSetEntry->newMdmObj),
                               &(forwardSetEntry->iidStack), TRUE);

            if (r2 != CMSRET_SUCCESS)
            {
               /*
                * We've done all the checking up front (valid parameters,
                * access list), so we know that mdm_setObject will succeed
                * at this point.
                */
               cmsLog_error("mdm_setObject unexpectedly failed! ret=%d", ret);
            }

            /* Also normalize the Order values, if applicable */
            if (IS_AUTO_ORDER_NODE(forwardSetEntry->objNode) &&
                forwardSetEntry->orderValueChanged)
            {
               mdm_normalizeOrderValues(forwardSetEntry->objNode,
                                        &(forwardSetEntry->iidStack));
            }

            forwardSetEntry = forwardSetEntry->next;
         }
      }


   } /* end of if case where we decide to call rcl set handler funcs */


   if (setQ->count != numSetEntries)
   {
      /*
       * Something is wrong.  The number of setq entries should match what
       * we created in this function.  Complain, but keep going.
       */
      cmsLog_error("Expected %d setQ entries, got %d",
                   numSetEntries, setQ->count);
   }

   /*
    * Is this too restrictive?  We assume we are not called from a
    * RCL/STL/RUT function, which may have created its own setq entry.
    * So far, this is not a problem.
    */
   freeSetQueue(setQ);

   cmsLog_debug("Done, ret=%d", ret);

   return ret;
}


CmsRet odl_setObjectExternal(const void *newMdmObj, const InstanceIdStack *iidStack)
{
   MdmObjectId oid;
   MdmObjectNode *objNode;
   void *currMdmObj;
   CmsLockThreadInfo *thread;
   CmsRet ret;

   if ((newMdmObj == NULL) || (iidStack == NULL))
   {
      cmsLog_error("bad input args");
      return CMSRET_INVALID_ARGUMENTS;
   }

   thread = getMyLockThread();
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread for tid %d", sysUtl_gettid());
      return CMSRET_INTERNAL_ERROR;
   }

   oid = *((MdmObjectId *) newMdmObj);

   if ((objNode = mdm_getObjectNode(oid)) == NULL)
   {
      cmsLog_error("Could not find objNode for oid %d", oid);
      return CMSRET_INVALID_ARGUMENTS;
   }

   ret = mdm_getObject(oid, iidStack, &currMdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get currMdmObj for oid %d", oid);
      return ret;
   }

   if (thread->setQ.count != 0)
   {
      cmsLog_error("Begin: found %d entries in setQ (expected 0), free all!",
                   thread->setQ.count);
      freeSetQueue(&(thread->setQ));
   }


   cmsLog_debug("starting new external set of %s iidStack=%s", objNode->name, cmsMdm_dumpIidStack(iidStack));

   ret = odl_setObjectInternal(newMdmObj, currMdmObj, iidStack);

   cmsObj_free((void **) &currMdmObj);

   cmsLog_debug("finished external set of %s iidStack=%s", objNode->name, cmsMdm_dumpIidStack(iidStack));

   if (thread->setQ.count != 0)
   {
      cmsLog_error("End: found %d entries in setQ (expected 0), free all!",
                   thread->setQ.count);
      freeSetQueue(&(thread->setQ));
   }

   return ret;
}


CmsRet odl_setObjectInternal(const void *newMdmObj,
                             const void *currMdmObj,
                             const InstanceIdStack *iidStack)

{
   MdmObjectId oid;
   MdmObjectNode *objNode;
   void *dupedNewMdmObj;
   char *errorParam=NULL;
   CmsLockThreadInfo *thread;
   OdlSetQueue *setQ;
   OdlSetQueueEntry *setEntry;
   UBOOL8 orderValueChanged=FALSE;
   CmsRet ret, errorCode=CMSRET_SUCCESS;

   if ((newMdmObj == NULL) || (iidStack == NULL))
   {
      cmsLog_error("bad input args");
      return CMSRET_INVALID_ARGUMENTS;
   }


   oid = *((MdmObjectId *) newMdmObj);

   if ((objNode = mdm_getObjectNode(oid)) == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }


   /*
    * Make sure the new object is valid.  Note that unlike the PHL->odl_set path,
    * where the params are validated in the PHL, in the OBJ->odl_setObjectxxx path,
    * the object is validated in the ODL.
    */
   if ((ret = mdm_validateObject(newMdmObj, iidStack)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   thread = getMyLockThread();
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread for tid %d", sysUtl_gettid());
      return CMSRET_INTERNAL_ERROR;
   }

   setQ = &(thread->setQ);
   if (isPointerInSetQueue(setQ, newMdmObj))
   {
      /*
       * The MdmObj to be set is in the set queue.  Just return
       * success indication to the caller.  The MdmObj will be pushed into
       * the MDM by the original caller.
       */
      cmsLog_debug("found %s (%p) iidStack=%s in SetQueue", objNode->name, newMdmObj, cmsMdm_dumpIidStack(iidStack));
      return CMSRET_SUCCESS;
   }


   cmsLog_debug("starting new internal set of %s iidStack=%s", objNode->name, cmsMdm_dumpIidStack(iidStack));


   INVALIDATE_ODL_GET_CACHE;

   /*
    * We need to dup the newMdmObj because we might modify it,
    * and then steal it.  But in order to allow rcl/rut functions
    * to call cmsObj_set, the caller will pass in a const pointer
    * to newMdmObj, not the address of the pointer to newMdmObj.
    * So we need to operate on a local copy of newMdmObj, not the
    * newMdmObj passed in by caller.
    */
   if ((dupedNewMdmObj = mdm_dupObject(newMdmObj, mdmLibCtx.allocFlags)) == NULL)
   {
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /*
    * This will push our object into the setQueue.
    * We do this so that if the rcl handler function for this object
    * calls an rut function, and then the rut function calls cmsObj_get on
    * this object again, it will see the new object with the new values to
    * be set instead of the curr object inside the MDM.
    */
   if ((setEntry = getSetQueueEntry(setQ, objNode, iidStack)) == NULL)
   {
      cmsLog_error("alloc of setEntry failed.");
      cmsObj_free(&dupedNewMdmObj);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   setEntry->newMdmObj = dupedNewMdmObj;
   setEntry->currMdmObj = (void *) currMdmObj;

   /*
    * If this is an Auto Order objNode, and the Order param has changed,
    * we must re-order the objects before calling the RCL handler function.
    * If currMdmObj is NULL, that means we are doing activateObjects during
    * startup.  Do not need to deal with order changes during this time.
    */
   if (IS_AUTO_ORDER_NODE(objNode))
   {
      UINT32 newOrderVal, currOrderVal=0;

      newOrderVal = mdm_getOrderValue(objNode, dupedNewMdmObj);
      if (currMdmObj)
      {
         currOrderVal = mdm_getOrderValue(objNode, currMdmObj);
      }

      if ((currOrderVal != 0) && (currOrderVal != newOrderVal))
      {
         mdm_moveInstanceUsingNewOrderValue(objNode, iidStack, newOrderVal);
         orderValueChanged = TRUE;
      }
   }


   ret = callSetHandlerFunc(objNode->oid, iidStack, dupedNewMdmObj, currMdmObj,
                            &errorParam, &errorCode);

   CMSMEM_FREE_BUF_AND_NULL_PTR(errorParam);

   /*
    * The dupedNewMdmObj will be dealt with (freed, stolen, etc) below.
    * The currMdmObj will be dealt with by the caller.
    * So clear the reference to both from the setEntry so that when we
    * call freeSetQueueEntry(), that function doesn't also free the objects.
    */
   setEntry->newMdmObj = NULL;
   setEntry->currMdmObj = NULL;
   freeSetQueueEntry(setQ, &setEntry);

   if (IS_CMSRET_A_SUCCESS_VARIANT(ret))
   {
      CmsRet r2;

      /*
       * Tell the MDM to replace the current MdmObj with new one.
       * Note that if mdm_setObject succeeds, dupedNewMdmObj will be stolen
       * by MDM.
       * Also be careful with return values here: overwrite ret only if
       * r2 is an error.  Otherwise, ret could be CMSRET_SUCCESS_REBOOT_NEEDED,
       * and r2 is CMSRET_SUCCESS.
       */
      r2 = mdm_setObject(&dupedNewMdmObj, iidStack, TRUE);
      if (r2 != CMSRET_SUCCESS)
      {
         ret = r2;
         mdm_freeObject(&dupedNewMdmObj);
      }

      if (orderValueChanged)
      {
         mdm_normalizeOrderValues(objNode, iidStack);
      }
   }
   else
   {
      if (orderValueChanged)
      {
         /* undo the re-order of the table based on new Order value */
         UINT32 currOrderVal = mdm_getOrderValue(objNode, currMdmObj);
         mdm_moveInstanceUsingNewOrderValue(objNode, iidStack, currOrderVal);
      }

      mdm_freeObject(&dupedNewMdmObj);
   }

   cmsLog_debug("finished internal set of %s iidStack=%s, ret=%d", objNode->name, cmsMdm_dumpIidStack(iidStack), ret);

   return ret;
}


/*
 * This function was added by Jeff to deal with a "order" parameter in one
 * of the QoS objects that needs to be renumbered and compacted whenever 
 * one of them changes.  The original odl_setObject function was causing
 * a bunch of recursive calls to renumber and compact, so this function
 * was created which modifies the MDM object only, but does not call
 * the RCL handler function.
 * Starting with 4.14L.04, the "autoOrder" attribute will cause the MDM
 * to do auto ordering, so developers do not have to use this function
 * to manually re-order the objects which contain an "Order" param anymore.
 * See the code in odl_set and odl_setObjectInternal.
 */
CmsRet odl_setObjectNoRclCallback(const void *newMdmObj,
                     const InstanceIdStack *iidStack)
{
   MdmObjectId oid;
   MdmObjectNode *objNode;
   void *dupedNewMdmObj;
   CmsLockThreadInfo *thread;
   CmsRet ret;

   if ((newMdmObj == NULL) || (iidStack == NULL))
   {
      cmsLog_error("bad input args");
      return CMSRET_INVALID_ARGUMENTS;
   }


   oid = *((MdmObjectId *) newMdmObj);

   if ((objNode = mdm_getObjectNode(oid)) == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }


   /*
    * Make sure the new object is valid.  Note that unlike the PHL->odl_set path,
    * where the params are validated in the PHL, in the OBJ->odl_setObjectxxx path,
    * the object is validated in the ODL.
    */
   if ((ret = mdm_validateObject(newMdmObj, iidStack)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   thread = getMyLockThread();
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread for tid %d", sysUtl_gettid());
      return CMSRET_INTERNAL_ERROR;
   }

   if (isPointerInSetQueue(&(thread->setQ), newMdmObj))
   {
      /*
       * The MdmObj to be set is in the set queue.  Just return
       * success indication to the caller.  The MdmObj will be pushed into
       * the MDM by the original caller.
       */
      cmsLog_debug("found %s (%p) iidStack=%s in SetQueue", objNode->name, newMdmObj, cmsMdm_dumpIidStack(iidStack));
      return CMSRET_SUCCESS;
   }


   cmsLog_debug("starting new NO_RCL_CALLBACK set of %s iidStack=%s", objNode->name, cmsMdm_dumpIidStack(iidStack));


   INVALIDATE_ODL_GET_CACHE;


   /*
    * We need to dup the newMdmObj because mdm_setObject will steal it,
    * but we told the caller he is responsible for freeing his object.
    */
   if ((dupedNewMdmObj = mdm_dupObject(newMdmObj, mdmLibCtx.allocFlags)) == NULL)
   {
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /*
    * Tell the MDM to replace the current MdmObj with new one.
    * Note that if mdm_setObject succeeds, the newMdmObj will be stolen
    * by MDM.
    */
   ret = mdm_setObject(&dupedNewMdmObj, iidStack, TRUE);
   if (ret != CMSRET_SUCCESS)
   {
      mdm_freeObject(&dupedNewMdmObj);
   }

   return ret;
}


/** Call the set (RCL) handler function associated with the OID, but do not
 *  update the mdmObject in the MDM yet.
 */
CmsRet callSetHandlerFunc(MdmObjectId oid,
                          const InstanceIdStack *iidStack,
                          void *newMdmObj,
                          const void *currMdmObj,
                          char **errorParam,
                          CmsRet *errorCode)
{
   const MdmOidInfoEntry *oidInfo;
   UINT8 zone;
   UINT8 lastFuncCode;
   MdmObjectId lastOid;
   CmsRet ret, retPre=CMSRET_SUCCESS, retPost=CMSRET_SUCCESS;

   if ((oidInfo = mdm_getOidInfo(oid)) == NULL)
   {
      cmsLog_error("Could not find OID info for oid %d", oid);
      return CMSRET_INTERNAL_ERROR;
   }

   if (oidInfo->rclHandlerFunc == NULL)
   {
      cmsLog_error("oid %d has NULL rcl handler func", oid);
      return CMSRET_INTERNAL_ERROR;
   }

   ODL_RECORD_CALL_ENTRY('r', oid);

   if (oidInfo->rclHandlerFuncPreHook != NULL)
   {
      UBOOL8 done=FALSE;
      retPre = oidInfo->rclHandlerFuncPreHook(newMdmObj, currMdmObj, iidStack,
                                              errorParam, errorCode, &done);
      if (done || !IS_CMSRET_A_SUCCESS_VARIANT(retPre))
      {
         ODL_RECORD_CALL_EXIT;
         return retPre;
      }
   }

   ret = oidInfo->rclHandlerFunc(newMdmObj, currMdmObj, iidStack,
                                 errorParam, errorCode);

   if (!IS_CMSRET_A_SUCCESS_VARIANT(ret))
   {
      ODL_RECORD_CALL_EXIT;
      return ret;
   }

   if (oidInfo->rclHandlerFuncPostHook != NULL)
   {
      retPost = oidInfo->rclHandlerFuncPostHook(newMdmObj, currMdmObj, iidStack,
                                              errorParam, errorCode);
   }

   if (IS_CMSRET_A_SUCCESS_VARIANT(retPost))
   {
      if ((ret == CMSRET_SUCCESS) && 
          (retPre == CMSRET_SUCCESS) && 
          (retPost == CMSRET_SUCCESS))
      {
         // All handlers agree on CMSRET_SUCCESS, so no change needed to
         // return value.
      }
      else if ((ret == CMSRET_SUCCESS_REBOOT_REQUIRED) ||
               (retPre == CMSRET_SUCCESS_REBOOT_REQUIRED) ||
               (retPost == CMSRET_SUCCESS_REBOOT_REQUIRED))
      {
         // At least one handler returned REBOOT_REQUIRED, so that is the
         // final return value.
         ret = CMSRET_SUCCESS_REBOOT_REQUIRED;
      }
      else if ((ret == CMSRET_SUCCESS_APPLY_NOT_COMPLETE) ||
               (retPre == CMSRET_SUCCESS_APPLY_NOT_COMPLETE) ||
               (retPost == CMSRET_SUCCESS_APPLY_NOT_COMPLETE))
      {
         // At least one handler returned APPLY_NOT_COMPLETE, so that is the
         // final return value.
         ret = CMSRET_SUCCESS_APPLY_NOT_COMPLETE;
      }
      else
      {
         // log an error message but return ret in this case.
         cmsLog_error("Unexpected return values retPre=%d ret=%d", retPre, ret);
      }
   }

   ODL_RECORD_CALL_EXIT;

   return ret;
}


/** Call the get (STL) handler function associated with the OID.
 */
CmsRet callGetHandlerFunc(MdmObjectId oid,
                          const InstanceIdStack *iidStack,
                          void *mdmObj)
{
   const MdmOidInfoEntry *oidInfo;
   UINT8 zone;
   UINT8 lastFuncCode;
   MdmObjectId lastOid;
   CmsRet ret, retPost=CMSRET_SUCCESS_OBJECT_UNCHANGED;

   if ((oidInfo = mdm_getOidInfo(oid)) == NULL)
   {
      cmsLog_error("Could not find OID info for oid %d", oid);
      return CMSRET_INTERNAL_ERROR;
   }
   
   if (oidInfo->stlHandlerFunc == NULL)
   {
      cmsLog_error("oid %d has NULL stl handler func", oid);
      return CMSRET_INTERNAL_ERROR;
   }
   
   ODL_RECORD_CALL_ENTRY('s', oid);

   ret = oidInfo->stlHandlerFunc(mdmObj, iidStack);
   if ((ret == CMSRET_SUCCESS) || (ret == CMSRET_SUCCESS_OBJECT_UNCHANGED))
   {
      if (oidInfo->stlHandlerFuncPostHook != NULL)
      {
         retPost = oidInfo->stlHandlerFuncPostHook(mdmObj, iidStack);
         if ((retPost == CMSRET_SUCCESS) || (retPost == CMSRET_SUCCESS_OBJECT_UNCHANGED))
         {
            if ((ret == CMSRET_SUCCESS_OBJECT_UNCHANGED) &&
                (retPost == CMSRET_SUCCESS_OBJECT_UNCHANGED))
            {
               // Both handlers agree on CMSRET_SUCCESS_OBJECT_UNCHANGED,
               // so no change needed to return value.
            }
            else
            {
               // At least one the handlers returned a plain CMSRET_SUCCESS,
               // so the final return value is CMSRET_SUCCESS.
               ret = CMSRET_SUCCESS;
            }
         }
         else
         {
            // post hook had error so return that error.
            ret = retPost;
         }
      }
   }

   ODL_RECORD_CALL_EXIT;

   return ret;
}


CmsLockThreadInfo *getMyLockThread(void)
{
   CmsLockThreadInfo *thread;
   SINT32 tid;

   tid = sysUtl_gettid() + pidNsOffset;
   thread = lck_getExistingLockThread(tid);
   return thread;
}


OdlSetQueueEntry *getSetQueueEntry(OdlSetQueue *setQ, MdmObjectNode *objNode, const InstanceIdStack *iidStack)
{
   OdlSetQueueEntry *entry = (OdlSetQueueEntry *) setQ->head;
   UBOOL8 found = FALSE;

   while ((!found) && (entry != NULL))
   {
      if ((objNode == entry->objNode) &&
          (!(cmsMdm_compareIidStacks(iidStack, &(entry->iidStack)))))
      {
         found = TRUE;
      }
      else
      {
         entry = entry->next;
      }
   }

   if (found)
   {
      return entry;
   }

   /*
    * We did not find an existing SetQueueEntry for this objNode.
    * Allocate it and append at the end of setQ.
    * The entry is only used internally by ODL, so it does not need
    * to be in shared mem.  
    */
   entry = cmsMem_alloc(sizeof(OdlSetQueueEntry), ALLOC_ZEROIZE);
   if (entry != NULL)
   {
      entry->objNode = objNode;
      entry->iidStack = *iidStack;
      setQ->count++;
      if (setQ->head == NULL)
      {
         setQ->head = entry;
         setQ->tail = entry;
      }
      else
      {
         OdlSetQueueEntry *tail = (OdlSetQueueEntry *) setQ->tail;
         tail->next = entry;
         entry->prev = (OdlSetQueueEntry *) setQ->tail;
         setQ->tail = entry;
      }
   }

   return entry;
}


UBOOL8 findInSetQueue(const OdlSetQueue *setQ,
                      MdmObjectId oid, const InstanceIdStack *iidStack,
                      void **mdmObj)
{
   OdlSetQueueEntry *entry = (OdlSetQueueEntry *) setQ->head;
   UBOOL8 found=FALSE;

   while ((entry != NULL) && (!found))
   {
      if ((entry->objNode->oid == oid) &&
          (!cmsMdm_compareIidStacks(&(entry->iidStack), iidStack)))
      {
         (*mdmObj) = entry->newMdmObj;
         found = TRUE;
      }

      entry = entry->next;
   }

   return found;
}


UBOOL8 isPointerInSetQueue(const OdlSetQueue *setQ, const void *mdmObj)
{
   OdlSetQueueEntry *entry = (OdlSetQueueEntry *) setQ->head;
   UBOOL8 found=FALSE;

   while ((entry != NULL) && (!found))
   {
      if (entry->newMdmObj == mdmObj)
      {
         found = TRUE;
      }

      entry = entry->next;
   }

   return found;
}


/** Reverse the position of the newMdmObj and currMdmObj in the set queue.
 *
 * This is done if an error was encountered as we are travelling forward
 * in the set queue.  Now we need to undo.  We have to switch the position
 * of the mdmObj in the set queue itself because the rcl handler functions
 * may call the odl_get family of functions to get objects, and those
 * odl_get functions will look in the set queue.  Reversing the position
 * of the mdmObj in the set queue itself allows the rcl handler functions
 * to get the right mdmObj from the set queue.
 */
void reverseSetQueue(const OdlSetQueue *setQ)
{
   OdlSetQueueEntry *entry = (OdlSetQueueEntry *) setQ->head;
   void *tmpMdmObj;

   while (entry != NULL)
   {
      tmpMdmObj = entry->newMdmObj;
      entry->newMdmObj = entry->currMdmObj;
      entry->currMdmObj = tmpMdmObj;

      entry = entry->next;
   }

   return;
}


/** If an error occured while we are processing a bunch of "sets" from the
 * PHL layer, we might have re-ordered one or more auto order tables
 * during the set processing.  Undo those re-orders.
 */
void undoAllInstanceMovesUsingNewOrderValues(const OdlSetQueue *setQ)
{
   OdlSetQueueEntry *entry = (OdlSetQueueEntry *) setQ->head;

   while (entry != NULL)
   {
      if (entry->orderValueChanged && entry->currMdmObj)
      {
         UINT32 origOrderVal;
         origOrderVal = mdm_getOrderValue(entry->objNode, entry->currMdmObj);
         mdm_moveInstanceUsingNewOrderValue(entry->objNode, &entry->iidStack,
                                            origOrderVal);
         entry->orderValueChanged = FALSE;
      }

      entry = entry->next;
   }
}


void freeSetQueue(OdlSetQueue *setQ)
{
   OdlSetQueueEntry *entry = (OdlSetQueueEntry *) setQ->head;
   OdlSetQueueEntry *tmpEntry;

   while (entry != NULL)
   {
      if (entry->newMdmObj != NULL)
      {
         mdm_freeObject(&(entry->newMdmObj));
      }

      if (entry->currMdmObj != NULL)
      {
         mdm_freeObject(&(entry->currMdmObj));
      }

      tmpEntry = entry->next;
      cmsMem_free(entry);
      entry = tmpEntry;
   }

   setQ->head = NULL;
   setQ->tail = NULL;
   setQ->count = 0;
}


void freeSetQueueEntry(OdlSetQueue *setQ, OdlSetQueueEntry **entry)
{
   OdlSetQueueEntry *currEntry = (OdlSetQueueEntry *) setQ->head;
   UBOOL8 found=FALSE;

   if (setQ->head == (void *)(*entry))
   {
      found = TRUE;
      setQ->head = (*entry)->next;
      if (setQ->head == NULL)
      {
         setQ->tail = NULL;
      }
      setQ->count--;
   }
   else
   {
      while (currEntry != NULL)
      {
         if (currEntry->next == (*entry))
         {
            found = TRUE;
            currEntry->next = (*entry)->next;
            if (setQ->tail == (void *)(*entry))
            {
               setQ->tail = currEntry;
            }
            setQ->count--;
            break;
         }
         currEntry = currEntry->next;
      }
   }

   if (!found)
   {
      cmsLog_error("could not find setQueueEntry, free it anyways");
   }
   
   if ((*entry)->newMdmObj != NULL)
   {
      mdm_freeObject(&((*entry)->newMdmObj));
   }

   if ((*entry)->currMdmObj != NULL)
   {
      mdm_freeObject(&((*entry)->currMdmObj));
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR((*entry));
   return;
}


CmsRet odl_getFlags(const MdmPathDescriptor *pathDesc, UINT32 getFlags, char **value)
{
   MdmParamNode *paramNode;
   void *mdmObj=NULL;
   CmsLockThreadInfo *thread;
   CmsRet ret=CMSRET_SUCCESS;

   if (pathDesc == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }
   
   if ((paramNode = mdm_getParamNode(pathDesc->oid, pathDesc->paramName)) == NULL)
   {
      return CMSRET_INVALID_PARAM_NAME;
   }

   thread = getMyLockThread();
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread for tid %d", sysUtl_gettid());
      return CMSRET_INTERNAL_ERROR;
   }

   /*
    * Check for cached object first.
    * object caching is only for the phl -> odl_get path because the phl
    * may be processing an envelope with gets for many parameters in the same
    * object.  Any other odl_getNext, odl_getAncestor, or odl_set will
    * invalidate this cache entry.
    */
   if (thread->getCache.mdmObj != NULL)
   {
      MdmObjectId oid;

      oid = GET_MDM_OBJECT_ID(thread->getCache.mdmObj);
      if ((oid == pathDesc->oid) &&
          (!cmsMdm_compareIidStacks(&(thread->getCache.iidStack),
                                    &(pathDesc->iidStack))))
      {
         UINT32 delta;
         CmsTimestamp nowTs;
         cmsTms_get(&nowTs);
         delta = cmsTms_deltaInMilliSeconds(&nowTs, &(thread->getCache.tms));
         if (delta < ODL_GET_CACHE_TIMEOUT)
         {
            /* cache hit, just use cached mdmObj */
            mdmObj = thread->getCache.mdmObj;
         }
         else
         {
            /* right object, but too old. */
            INVALIDATE_ODL_GET_CACHE;
         }
      }
      else
      {
         /* cache not empty, but is not what we want */
         INVALIDATE_ODL_GET_CACHE;
      }
   }

   if (mdmObj == NULL)
   {
      /* Get updated mdmObj, including STL handler callback */
      if ((ret = odl_getObject(pathDesc->oid, &(pathDesc->iidStack), getFlags, &mdmObj)) != CMSRET_SUCCESS)
      {
         return ret;
      }

      /*
       * When there is recursive call of cmsPhl_getParamValue(), for example, 
       * CWMPD/TR69c receive GetParameterValues of MDMOID_DEV2_HOST object, 
       * getCache.mdmObj be set in 2nd call. Invalidate the getCache before set.
       */
      if (thread->getCache.mdmObj != NULL)
      {
         INVALIDATE_ODL_GET_CACHE;
      }

      /* store newly read object in the get cache. Get cache should not point to a buffer
       * in shared memory, as this buffer is only accessed by the calling process.
       */
      thread->getCache.mdmObj = mdm_dupObject(mdmObj, ALLOC_ZEROIZE);
      thread->getCache.iidStack = pathDesc->iidStack;
      cmsTms_get(&(thread->getCache.tms));
   }


   /* return the value as a string, with string allocated from heap mem. */
   ret = mdm_getParamNodeString(paramNode, mdmObj, ALLOC_ZEROIZE, value);

   /*
    * In order to allow RCL/STL/RUT functions to call QDM functions which call
    * cmsPhl_getParamValue, we need to allow cmsPhl_getParamValue to call
    * this function.  But that opens the possibility that our retrieved mdmObj
    * from obj_getObject() may actually be a pointer to an object in the setq,
    * so we must not free it later on.
    * Check for condition and free mdmObj when it is in shared memory,
    * and it is not in SetQueue.
    */
   if ((mdmObj != thread->getCache.mdmObj) &&
       (isPointerInSetQueue(&(thread->setQ), mdmObj) == FALSE))
   {
       mdm_freeObject(&mdmObj);
   }

   return ret;
}


CmsRet odl_getObject(MdmObjectId oid, const InstanceIdStack *iidStack,
                     UINT32 getFlags, void **mdmObj)
{
   CmsLockThreadInfo *thread;
   CmsRet ret;

   thread = getMyLockThread();
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread for tid %d", sysUtl_gettid());
      return CMSRET_INTERNAL_ERROR;
   }

   /*
    * rcl may be calling us and the object being requested may be 
    * in the setq.  So check there first, if not there, then get it
    * from the MDM/STL.
    *
    * For gets from the object layer, this check always fails because the
    * setq is empty.
    */
   if (findInSetQueue(&(thread->setQ), oid, iidStack, mdmObj))
   {
      if ((*mdmObj) != NULL)
      {
         /*
          * Give the rcl handler function a direct pointer to the object in
          * the set queue (not a copy) because the rcl handler function may
          * modify the object.  That modified object will then be written to
          * the MDM at the end of the odl_set sequence.
          */
         cmsLog_debug("returning oid %d (%p) %s from setQueue",
                      oid, *mdmObj, cmsMdm_dumpIidStack(iidStack));
         return CMSRET_SUCCESS;
      }
   }

   INVALIDATE_ODL_GET_CACHE;

   /* Get the current object from the MDM. */
   if ((ret = mdm_getObject(oid, iidStack, mdmObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if ((getFlags & OGF_NO_VALUE_UPDATE) == 0)
   {
      ret = callGetHandlerAndUpdateMdm(thread, oid, iidStack, mdmObj);
   }

   return ret;
}


CmsRet odl_getDefaultObject(MdmObjectId oid, void **mdmObj)
{
   return (mdm_getDefaultObject(oid, mdmObj));
}


CmsRet odl_getNextObject(MdmObjectId oid, InstanceIdStack *iidStack, UINT32 getFlags, void **mdmObj)
{
   InstanceIdStack parentIidStack = EMPTY_INSTANCE_ID_STACK;

   return (odl_getNextObjectInSubTree(oid, &parentIidStack, iidStack, getFlags, mdmObj));
}



CmsRet odl_getNextObjectInSubTree(MdmObjectId oid,
                                  const InstanceIdStack *parentIidStack,
                                  InstanceIdStack *iidStack,
                                  UINT32 getFlags,
                                  void **mdmObj)
{
   UBOOL8 keepLooking=TRUE;
   CmsLockThreadInfo *thread;
   void *setQueueObj=NULL;
   CmsRet ret;

   thread = getMyLockThread();
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread for tid %d", sysUtl_gettid());
      return CMSRET_INTERNAL_ERROR;
   }

   /* Get the next object from the MDM (while obeying hidePendingDelete) */
   while (keepLooking)
   {
      ret = mdm_getNextObjectInSubTree(oid, parentIidStack, iidStack, mdmObj);
      if (ret != CMSRET_SUCCESS)
      {
         return ret;
      }

      if (mdmLibCtx.hideObjectsPendingDelete &&
          mdm_isObjectDeletePending(*mdmObj, iidStack))
      {
         /*
          * This object is about to be deleted, "hide" it from the caller
          * so that the object appears as if it was not there.
          */
         mdm_freeObject(mdmObj);
      }
      else
      {
         keepLooking = FALSE;
      }
   }

   /* check if the object is in the set queue */
   if (findInSetQueue(&(thread->setQ), oid, iidStack, &setQueueObj))
   {
      if (setQueueObj != NULL)
      {
         /*
          * Give the rcl handler function a direct pointer to the object in
          * the set queue (not a copy) because the rcl handler function may
          * modify the object.  That modified object will then be written to
          * the MDM at the end of the odl_set sequence.
          */
         mdm_freeObject(mdmObj);
         *mdmObj = setQueueObj;
         cmsLog_debug("returning oid %d (%p) %s from setQueue",
                      oid, *mdmObj, cmsMdm_dumpIidStack(iidStack));
         return CMSRET_SUCCESS;
      }
   }

   INVALIDATE_ODL_GET_CACHE;


   if ((getFlags & OGF_NO_VALUE_UPDATE) == 0)
   {
      ret = callGetHandlerAndUpdateMdm(thread, oid, iidStack, mdmObj);
   }

   return ret;
}


CmsRet odl_getAncestorObject(MdmObjectId ancestorOid,
                             MdmObjectId decendentOid,
                             InstanceIdStack *iidStack,
                             UINT32 getFlags,
                             void **mdmObj)
{
   InstanceIdStack tmpIidStack = *iidStack;
   CmsLockThreadInfo *thread;
   void *setQueueObj=NULL;
   CmsRet ret;

   thread = getMyLockThread();
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread for tid %d", sysUtl_gettid());
      return CMSRET_INTERNAL_ERROR;
   }

   /* Get the current (parent) object from the MDM. */
   ret = mdm_getAncestorObject(ancestorOid, decendentOid, &tmpIidStack, mdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }

   /* check if the requested (ancestor) object is in the set queue */
   if (findInSetQueue(&(thread->setQ), ancestorOid, &tmpIidStack, &setQueueObj))
   {
      if (setQueueObj != NULL)
      {
         /*
          * Give the rcl handler function a direct pointer to the object in
          * the set queue (not a copy) because the rcl handler function may
          * modify the object.  That modified object will then be written to
          * the MDM at the end of the odl_set sequence.
          */
         mdm_freeObject(mdmObj);
         *mdmObj = setQueueObj;
         *iidStack = tmpIidStack;
         cmsLog_debug("returning oid %d (%p) %s from setQueue",
                      ancestorOid, *mdmObj, cmsMdm_dumpIidStack(iidStack));
         return CMSRET_SUCCESS;
      }
   }

   INVALIDATE_ODL_GET_CACHE;

   if ((getFlags & OGF_NO_VALUE_UPDATE) == 0)
   {
      ret = callGetHandlerAndUpdateMdm(thread, ancestorOid, &tmpIidStack,
                                       mdmObj);
      if ((ret == CMSRET_SUCCESS_OBJECT_UNCHANGED) || (ret == CMSRET_SUCCESS))
      {
         *iidStack = tmpIidStack;
      }
   }

   return ret;
}


CmsRet callGetHandlerAndUpdateMdm(CmsLockThreadInfo *thread,
                                  MdmObjectId oid,
                                  const InstanceIdStack *iidStack,
                                  void **mdmObj)
{
   MdmObjectNode *objNode=NULL;
   OdlSetQueueEntry *setEntry=NULL;
   CmsRet ret;

   /*
    * Before calling the STL handler, push this mdmObj into the setq.
    * This will handle scenario where STL handler func calls RUT
    * func, which calls cmsObj_get or cmsPhl_get on this object.
    * We want to return the values from the new mdmObj, which has
    * not yet been pushed into the MDM.
    */
   if (((objNode = mdm_getObjectNode(oid)) == NULL) ||
       ((setEntry = getSetQueueEntry(&(thread->setQ), objNode, iidStack)) == NULL))
   {
      cmsLog_error("getObjNode or getSetEntry failed "
                   "(oid=%d objNode=%p setEntry=%p)",
                   oid, objNode, setEntry);
      ret = CMSRET_RESOURCE_EXCEEDED;
   }
   else
   {
      setEntry->newMdmObj = *mdmObj;

      ret = callGetHandlerFunc(oid, iidStack, *mdmObj);

      /*
       * once the STL handler func is done, we can undo this
       * setq entry.  The code below doesn't know anything
       * special happened here.
       */
      setEntry->newMdmObj = NULL;
      freeSetQueueEntry(&(thread->setQ), &setEntry);
   }

   if (ret == CMSRET_SUCCESS_OBJECT_UNCHANGED)
   {
      /*
       * stl handler function did not change the data, so we don't
       * need to push the mdmObj back into the MDM.  Just return our
       * current copy of mdmObj and change the return value to a
       * regular CMSRET_SUCCESS so the caller will not get confused.
       */
       ret = CMSRET_SUCCESS;
   }
   else if (ret == CMSRET_SUCCESS)
   {
      /*
       * stl handler function has changed the data.  Push the updated
       * mdmObj into the MDM.
       */
      ret = mdm_setObject(mdmObj, iidStack, TRUE);
      if (ret != CMSRET_SUCCESS)
      {
         mdm_freeObject(mdmObj);
      }
      else
      {
         /* mdm_setObject stole our mdmObj from us, need to get it again. */
         ret = mdm_getObject(oid, iidStack, mdmObj);
      }
   }
   else
   {
      /* error condition */
      mdm_freeObject(mdmObj);
   }

   return ret;
}


CmsRet addObjectCallBack(const MdmObjectNode *objNode __attribute__((unused)),
                         const InstanceIdStack *iidStack,
                         const void *mdmObj,
                         void *context)
{
   UINT32 *objectsAdded = (UINT32 *) context;   
   CmsRet ret;

   cmsLog_debug("got callback on %s", mdm_oidToGenericPath(objNode->oid));


   /*
    * There are two things to watch out for here:
    * 1. This mdmObj is a pointer to the actual object in the MDM, so it must
    * be dup'ed before being pushed into the MDM.  Cannot push this mdmObj
    * into the MDM.
    * 2. Need to protect against nested calls which reference this object,
    * so use the set queue.
    * odl_setObjectInternal does both of these things for us, so use it.
    */
   ret = odl_setObjectInternal(mdmObj, NULL, iidStack);


   if (1)
   {
      if (objectsAdded != NULL) (*objectsAdded)++;
   }

   return ret;
}


CmsRet delObjectCallBack(const MdmObjectNode *objNode,
                         const InstanceIdStack *iidStack,
                         const void *mdmObj,
                         void *context)
{
   const MdmOidInfoEntry *oidInfo;
   UINT32 *deleteError = (UINT32 *) context;
   char *errorParam=NULL;
   CmsRet errorCode=CMSRET_SUCCESS;
   CmsRet ret;

   if ((oidInfo = mdm_getOidInfo(objNode->oid)) == NULL)
   {
      cmsLog_error("Could not find OID info for oid %d", objNode->oid);
      return CMSRET_INTERNAL_ERROR;
   }

   cmsLog_debug("got callback on %s", oidInfo->fullPath);

   ret = callSetHandlerFunc(objNode->oid, iidStack, NULL, mdmObj,
                            &errorParam, &errorCode);

   if (!IS_CMSRET_A_SUCCESS_VARIANT(ret))
   {
      if (deleteError != NULL) (*deleteError)++;
   }

   return ret;
}

CmsRet odl_addObjectInstance(MdmPathDescriptor *pathDesc)
{
   MdmObjectNode *objNode;
   CmsLockThreadInfo *thread;
   UINT32 objectsAdded=0;
   CmsRet ret;

   if (pathDesc == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }
   
   if ((objNode = mdm_getObjectNode(pathDesc->oid)) == NULL)
   {
      return CMSRET_INVALID_PARAM_NAME;
   }
   
   if (!(IS_INSTANCE_NODE(objNode)))
   {
      cmsLog_error("cannot do add instance on non-instance node %s",
                   mdm_oidToGenericPath(objNode->oid));
      return CMSRET_INVALID_ARGUMENTS;
   }
   
   if (IS_EXTERNAL_CALLER && !(IS_DYNAMIC_INSTANCE_NODE(objNode)))
   {
      /*
       * An external caller/management entity is trying to create an
       * object instance on a non-dynamic instance node.  Only allow
       * it if the caller is a special priviledged eid (e.g. ssk).
       */
      if (!mdm_isFullWriteAccessEid(mdmLibCtx.eid))
      {
         cmsLog_error("instance add on %s not allowed from management app",
                      mdm_oidToGenericPath(objNode->oid));
         return CMSRET_INVALID_ARGUMENTS;
      }
   }

   thread = getMyLockThread();
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread for tid %d", sysUtl_gettid());
      return CMSRET_INTERNAL_ERROR;
   }
   INVALIDATE_ODL_GET_CACHE;

   /*
    * mdm_addObjectInstance() will call our callback function for
    * every object instance created.  Our callback function will
    * then call the associated rcl handler function.
    * If we encounter an error along the way, the object creation
    * process is stopped, and this function is responsible for undo.
    */
   ret = mdm_addObjectInstance(pathDesc, addObjectCallBack, &objectsAdded);
   
   if ((ret != CMSRET_SUCCESS) && (ret != CMSRET_SUCCESS_REBOOT_REQUIRED))
   {
      if (objectsAdded > 0)
      {
         CmsRet r2;
         cmsLog_error("%d objects already added when addObject failed, undo", objectsAdded);
         r2 = mdm_deleteObjectInstance(pathDesc, delObjectCallBack, NULL);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("undo failed, reboot required.");
            ret = CMSRET_FAIL_REBOOT_REQUIRED;
         }
      }
   }
   
   return ret;
}


CmsRet odl_deleteObjectInstance(const MdmPathDescriptor *pathDesc)
{
   UINT32 deleteError=0;
   MdmObjectNode *objNode;
   CmsLockThreadInfo *thread;
   CmsRet ret;

   if (pathDesc == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }
   
   if ((objNode = mdm_getObjectNode(pathDesc->oid)) == NULL)
   {
      return CMSRET_INVALID_PARAM_NAME;
   }

   if (!(IS_INSTANCE_NODE(objNode)))
   {
      cmsLog_error("cannot do delete instance on non-instance node %s",
                   mdm_oidToGenericPath(objNode->oid));
      return CMSRET_INVALID_ARGUMENTS;
   }
   
   if (IS_EXTERNAL_CALLER && !(IS_DYNAMIC_INSTANCE_NODE(objNode)))
   {
      /*
       * An external caller/management entity is trying to delete an
       * object instance on a non-dynamic instance node.  Only allow
       * it if the caller is a special priviledged eid (e.g. ssk).
       */
      if (!mdm_isFullWriteAccessEid(mdmLibCtx.eid))
      {
         cmsLog_error("instance delete on %s not allowed from management app",
                      mdm_oidToGenericPath(objNode->oid));
         return CMSRET_INVALID_ARGUMENTS;
      }
   }

   thread = getMyLockThread();
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread for tid %d", sysUtl_gettid());
      return CMSRET_INTERNAL_ERROR;
   }
   INVALIDATE_ODL_GET_CACHE;

   /*
    * mdm_deleteObjectInstance() will call our callback function for
    * every object instance deleted.  Our callback function will
    * then call the associated rcl handler function.
    *
    * When a RCL handler function is asked to delete an object, 
    * it must return either CMSRET_SUCCESS,
    * or CMSRET_SUCCESS_REBOOT_REQUIRED if it is unable to perform
    * the runtime action.  This is because we must be able to delete
    * the object from the MDM.
    *
    * The deleteError variable tracks the number of errors returned
    * by the RCL handler functions.  If any of the RCL handler functions
    * returns an error, convert that to CMSRET_SUCCESS_REBOOT_REQUIRED.
    * However, if mdm_deleteObjectInstance returns an error but 
    * deleteError == 0, then there was an error in the pathDesc that
    * was passed in.  In that case, return the actual error (the ACS likes
    * to delete non-existent objects to make sure they are not there.)
    */
   cmsLog_debug("delete %s %s",
                mdm_oidToGenericPath(objNode->oid),
                cmsMdm_dumpIidStack(&(pathDesc->iidStack)));

   ret = mdm_deleteObjectInstance(pathDesc, delObjectCallBack, &deleteError);
   if ((ret != CMSRET_SUCCESS) && (ret != CMSRET_SUCCESS_REBOOT_REQUIRED))
   {
      if (deleteError > 0)
      {
         cmsLog_error("%d rcl handler error detected, reboot required", deleteError);
         ret = CMSRET_SUCCESS_REBOOT_REQUIRED;
      }
   }
   
   return ret;
}


CmsRet odl_clearStatistics(MdmObjectId oid, const InstanceIdStack *iidStack)
{
   CmsLockThreadInfo *thread;
   void *mdmObj=NULL;
   CmsRet ret;

   thread = getMyLockThread();
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread for tid %d", sysUtl_gettid());
      return CMSRET_INTERNAL_ERROR;
   }
   INVALIDATE_ODL_GET_CACHE;

   /*
    * I don't actually need the mdmObj in this case, but its a good way
    * to verify the oid and iidStack passed in by the caller are valid.
    */
   if ((ret = mdm_getObject(oid, iidStack, &mdmObj)) == CMSRET_SUCCESS)
   {
      mdm_freeObject(&mdmObj);
      // by convention, when we want to clear statistics, use NULL mdmObj.
      ret = callGetHandlerFunc(oid, iidStack, NULL);
   }

   return ret;
}


void odl_freeObject(void **mdmObj)
{
   CmsLockThreadInfo *thread;

   if (mdmObj == NULL || (*mdmObj) == NULL)
   {
      return;
   }

   // MDM objects are tracked regardless of whether they are in the setQ or
   // not.  So when they are freed, we must untrack them.
   lck_autoUntrackMdmObj(*mdmObj);

   thread = getMyLockThread();
   if ((thread != NULL) &&
       (isPointerInSetQueue(&(thread->setQ), *mdmObj)))
   {
      /*
       * The MdmObj to be freed is in the set queue.  Steal the object
       * from the caller, but leave the object in the set queue.  odl_set will
       * be responsible for the MdmObj at this point.
       */
      *mdmObj = NULL;
      return;
   }

   mdm_freeObject(mdmObj);
   return;
}


CmsRet odl_setInstanceFlags(MdmObjectId oid, const InstanceIdStack *iidStack, UINT16 flags)
{

   MdmObjectNode *objNode;

   if ((objNode = mdm_getObjectNode(oid)) == NULL)
   {
      cmsLog_debug("Invalid arguments 0x%08X", (uintptr_t)objNode);
      return CMSRET_INVALID_ARGUMENTS;
   }

   return mdm_setInstanceFlags(objNode,iidStack,flags);


}

CmsRet odl_getInstanceFlags(MdmObjectId oid, const InstanceIdStack *iidStack, UINT16 *flags)
{

   MdmObjectNode *objNode;

   if ((objNode = mdm_getObjectNode(oid)) == NULL)
   {
      cmsLog_debug("Invalid arguments 0x%08X", (uintptr_t)objNode);
      return CMSRET_INVALID_ARGUMENTS;
   }

   return mdm_getInstanceFlags(objNode,iidStack,flags);

}
