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

#include "cms.h"
#include "cms_phl.h"
#include "cms_obj.h"
#include "cms_mdm.h"
#include "cms_mem.h"
#include "cms_log.h"
#include "cms_util.h"
#include "mdm.h"
#include "odl.h"

#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
#include "phl_ene.h"
#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)


/*
 * CMS PHL API is used by TR69.  Use a longer timeout to increase chance of
 * getting a lock and also to enable backoff-retry when we lock all zones.
 */
#define PHL_LOCK_TIMEOUT  (CMSLCK_MAX_HOLDTIME * 2)

#define PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(o) {\
   if (phl_isAllZones(zones)) rc = lck_autoLockAllZonesWithBackoff(\
                                     o, PHL_LOCK_TIMEOUT, __FUNCTION__);\
   else rc = lck_autoLockZones(zones, o, __FUNCTION__); \
   if (rc != CMSRET_SUCCESS) return rc; }

static UBOOL8 phl_isAllZones(const UBOOL8 *zones)
{
   UINT32 i;
   for (i=0; i < MDM_MAX_LOCK_ZONES; i++)
   {
      if (zones[i] == FALSE)
      {
         return FALSE;
      }
   }
   // All zones are marked TRUE
   return TRUE;
}

static CmsRet phl_fillLockZones(const MdmPathDescriptor *pathDescs,
                                UINT32 numPathDescs, UBOOL8 contained,
                                UBOOL8 *zones)
{
    UINT32 i;
    UINT8 zone;

    memset(zones, FALSE, MDM_MAX_LOCK_ZONES);

    for (i=0; i < numPathDescs; i++)
    {
      zone = cmsLck_getLockZone(pathDescs[i].oid);
      if (zone == MDM_INVALID_LOCK_ZONE)
      {
         // Some callers might set a bad OID in pathDesc and expect PHL code to
         // handle it.  Locking code should just ignore it and allow PHL code
         // to deal with it.
         continue;
      }
      // !contained means the operation can go beyond the specified pathDesc.
      if (!contained && cmsLck_isTopLevelLockZone(zone))
      {
         // To be conservative, if a non-contained operation starts from a
         // top level zone, meaning it can walk into other zones, just lock
         // all zones.
         memset(zones, TRUE, MDM_MAX_LOCK_ZONES);

         /* For slower platforms, doing operation from root DM takes a little longer */
         cmsLck_setHoldTimeWarnThresh(CMSLCK_MAX_HOLDTIME * 2);
         
         return CMSRET_SUCCESS;
      }
      zones[zone] = TRUE;
   }

   return CMSRET_SUCCESS;
}

static CmsRet phl_fillLockZonesParamValue(const PhlSetParamValue_t *paramValues,
                                          UINT32 numParamValues, UBOOL8 *zones)
{
   MdmPathDescriptor *pathDescs;
   UINT32 i;
   UINT32 count=0;
   CmsRet ret;

   // Allocate an arry of pathDesc and copy oid from paramVaues.
   pathDescs = cmsMem_alloc(sizeof(MdmPathDescriptor) * numParamValues, ALLOC_ZEROIZE);
   if (pathDescs == NULL)
   {
      cmsLog_error("Could not allocate pathDescs");
      return CMSRET_RESOURCE_EXCEEDED;
   }

   for (i=0; i < numParamValues; i++)
   {
      // tr69c will detect invalid param name and set error status
      // but expects to execute the rest of the PHL code to handle it.
      if (paramValues[i].status == CMSRET_SUCCESS)
      {
         pathDescs[i].oid = paramValues[i].pathDesc.oid;
         count++;
      }
   }

   ret = phl_fillLockZones(pathDescs, count, TRUE, zones);
   cmsMem_free(pathDescs);

   return ret;
}

static CmsRet phl_fillLockZonesParamAttr(const PhlSetParamAttr_t *paramAttrs,
                                          UINT32 numParamAttrs, UBOOL8 *zones)
{
   MdmPathDescriptor *pathDescs;
   UINT32 i;
   CmsRet ret;

   // Allocate an arry of pathDesc and copy oid from paramVaues.
   pathDescs = cmsMem_alloc(sizeof(MdmPathDescriptor) * numParamAttrs, ALLOC_ZEROIZE);
   if (pathDescs == NULL)
   {
      cmsLog_error("Could not allocate pathDescs");
      return CMSRET_RESOURCE_EXCEEDED;
   }

   for (i=0; i < numParamAttrs; i++)
   {
      pathDescs[i].oid = paramAttrs[i].pathDesc.oid;
   }

   ret = phl_fillLockZones(pathDescs, numParamAttrs, TRUE, zones);
   cmsMem_free(pathDescs);

   return ret;
}


static UBOOL8 phl_sameObjDescs(const MdmPathDescriptor *pPathDesc1,
                               const MdmPathDescriptor *pPathDesc2)
{
   return ((pPathDesc1->oid == pPathDesc2->oid) &&
           (cmsMdm_compareIidStacks(&(pPathDesc1->iidStack), &(pPathDesc2->iidStack)) == 0));

}


/** call mdm_getNextObjPathDesc() with additional check for objects
 *  that should be hidden from tr69c.
 */
static CmsRet phl_wrappedGetNextObjPathDesc(const MdmPathDescriptor *pRootPath,
                                            MdmPathDescriptor *pNextPath)
{
   MdmPathDescriptor lastHiddenPath;
   UBOOL8 found=FALSE;
   CmsRet ret=CMSRET_SUCCESS;

   INIT_PATH_DESCRIPTOR(&lastHiddenPath);


   while (!found && ret == CMSRET_SUCCESS)
   {
      ret = mdm_getNextObjPathDesc(pRootPath, pNextPath);

      if (ret == CMSRET_SUCCESS)
      {
         found = TRUE;

         /*
          * Normally, we would be done at this point.  But if the caller is tr69c,
          * we have to make sure this object is not hidden from tr69c.
          */
         if (mdmLibCtx.eid == EID_TR69C ||
             mdmLibCtx.eid == EID_CWMPD ||
             mdmLibCtx.eid == EID_TR69C_2)
         {
            UBOOL8 hidden=FALSE;


            mdm_getPathDescHiddenFromAcs(pNextPath, &hidden);

#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
            if (!hidden)
            {
               /* further check */
               ene_getPathDescHiddenFromAcs(pNextPath, &hidden);
            }
#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)

            if (hidden)
            {
               /* this object is hidden, so go around again and get the next object */
               found = FALSE;

               /* remember the top of a sub-tree which is hidden from ACS */
               if ((lastHiddenPath.oid == 0) ||
                   (FALSE == mdm_isPathDescContainedInSubTree(&lastHiddenPath, pNextPath)))
               {
                  lastHiddenPath = *pNextPath;
               }
            }


            /*
             * We have to do this in case the user marks a top level object as
             * hideObjectFromAcs="true", but does not mark the objects below that
             * object with hideObjectFromAcs.  mdm_getNextObjPathDesc() will keep
             * traversing the tree, so we have to detect those objects and skip them.
             */
            if (lastHiddenPath.oid != 0 && found)
            {
               if (mdm_isPathDescContainedInSubTree(&lastHiddenPath, pNextPath))
               {
                  found = FALSE;
               }
            }
         }
      }
   }

   return ret;
}


/** call mdm_getNextChildObjectPathDesc() with additional check for objects
 *  that should be hidden from tr69c.
 */
static CmsRet phl_wrappedGetNextChildObjPathDesc(const MdmPathDescriptor *pRootPath,
                                                 MdmPathDescriptor *pNextPath)
{
   UBOOL8 found=FALSE;
   CmsRet ret=CMSRET_SUCCESS;

   while (!found && ret == CMSRET_SUCCESS)
   {
      ret = mdm_getNextChildObjPathDesc(pRootPath, pNextPath);

      if (ret == CMSRET_SUCCESS)
      {
         found = TRUE;

         /*
          * Normally, we would be done at this point.  But if the caller is tr69c,
          * we have to make sure this object is not hidden from tr69c.
          */
         if (mdmLibCtx.eid == EID_TR69C ||
             mdmLibCtx.eid == EID_CWMPD ||
             mdmLibCtx.eid == EID_TR69C_2)
         {
            UBOOL8 hidden=FALSE;
            mdm_getPathDescHiddenFromAcs(pNextPath, &hidden);

#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
            if (!hidden)
            {
               /* further check */
               ene_getPathDescHiddenFromAcs(pNextPath, &hidden);
            }
#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
            
            if (hidden)
            {
               /* this object is hidden, so go around again and get the next object */
               found = FALSE;
            }
         }
      }
   }

   return ret;
}


/** call mdm_getNextParamName() with additional check for parameters taht should be
 * hidden from tr69c.
 */
static CmsRet phl_wrappedGetNextParamName(MdmPathDescriptor *path)
{
   UBOOL8 found=FALSE;
   CmsRet ret=CMSRET_SUCCESS;

   while (!found && ret == CMSRET_SUCCESS)
   {
      ret = mdm_getNextParamName(path);

      if (ret == CMSRET_SUCCESS)
      {
         found = TRUE;

         /*
          * Normally, we would be done at this point.  But if the caller is tr69c,
          * we have to make sure this parameter is not hidden from tr69c.
          */
         if (mdmLibCtx.eid == EID_TR69C ||
             mdmLibCtx.eid == EID_CWMPD ||
             mdmLibCtx.eid == EID_TR69C_2)
         {
            UBOOL8 hidden=FALSE;
            mdm_getPathDescHiddenFromAcs(path, &hidden);

#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
            if (!hidden)
            {
               /* further check */
               ene_getPathDescHiddenFromAcs(path, &hidden);
            }
#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
            
            if (hidden)
            {
               /* this parameter is hidden, so go around again and get the next parameter */
               found = FALSE;
            }
         }
      }
   }

   return ret;
}


/** Count the number of parameters that are not hidden from tr69c.
 */
static CmsRet phl_wrappedGetParamNameCount(const MdmPathDescriptor *path, UINT32 *numParams)
{
   MdmPathDescriptor paramPath;
   CmsRet ret=CMSRET_SUCCESS;

   if (IS_PARAM_NAME_PRESENT(path))
   {
      cmsLog_error("pathDesc must not have param name when calling this function");
      return CMSRET_INVALID_ARGUMENTS;
   }

   *numParams = 0;
   paramPath = *path;

   while ((ret = phl_wrappedGetNextParamName(&paramPath)) == CMSRET_SUCCESS)
   {
      (*numParams)++;
   }

   return CMSRET_SUCCESS;
}




/** Get the next param or object name in the walk.
 *  Must be called with lock held.
 *
 * @param paramOnly (IN) Report parameter names only.  Do not report object names.
 *                       GetParameterValues and GetParameterAttributes calls this
 *                       function with paramOnly=TRUE because they don't want the 
 *                       object names.  But GetParameterNames calls this function with
 *                       paramOnly=FALSE because it wants the object names as well as
 *                       the paramNames.
 */
static CmsRet phl_getNextPath(UBOOL8             paramOnly,
                              UBOOL8             nextLevelOnly,
                              const MdmPathDescriptor  *pRootPath,
                              MdmPathDescriptor  *pNextPath)
{
   MdmPathDescriptor path;
   CmsRet            rc = CMSRET_SUCCESS;

   if (pRootPath->oid == 0 || pRootPath->paramName[0] != 0)
   {
      cmsLog_error("invalid root object path");
      return CMSRET_INTERNAL_ERROR;
   }

   if (pNextPath->oid == 0)
   {
      /* This is the first call to tree traversal.
       * Return the root object path.
       */
      *pNextPath = *pRootPath;

      if (!paramOnly)
      {
         return rc;
      }
   }

   /* Set path to pNextPath and do the traversing */
   path = *pNextPath;

   /* If this getnext is not for paramOnly and is nextLevelOnly and
    * pNextPath and pRootPath are not the same object, pNextPath must be the
    * direct child object of pRootPath.  We want to return the next
    * direct child object of pRootPath.
    */
   if (!paramOnly && nextLevelOnly && !phl_sameObjDescs(pRootPath, &path))
   {
      /* Return the next direct child object of pRootPath. */
      rc = phl_wrappedGetNextChildObjPathDesc(pRootPath, &path);
      if (rc != CMSRET_SUCCESS && rc != CMSRET_NO_MORE_INSTANCES)
      {
         cmsLog_error("mdm_getNextChildObjPathDesc error %d", rc);
      }
   }
   else
   {
      /* In this case, we want to return either the next parameter or
       * the next hierarchical object after pNextPath object.
       */
      while (rc == CMSRET_SUCCESS)
      {
         /* Get the next parameter of the path object */
         rc = phl_wrappedGetNextParamName(&path);
         if (rc == CMSRET_SUCCESS)
         {
            /* done */
            break;   /* out of while (rc == CMSRET_SUCCESS) */
         }
         else if (rc == CMSRET_NO_MORE_INSTANCES)
         {
            /* There are no more parameter in the path object.
             * If this getnext is for paramOnly and is nextLevelOnly,
             * then done with finding nextParam.
             * Otherwise, find the next hierarchical object after
             * path and continue traversing.
             */
            if (!paramOnly || !nextLevelOnly)
            {
               /* find the next hierarchical object path */

               /* if pRootPath and path have the same object path,
                * we know that this will be the first call to
                * mdm_getNextObjPathDesc. path must be set to 0
                * prior to the first call to mdm_getNextObjPathDesc.
                */
               if (phl_sameObjDescs(pRootPath, &path))
               {
                  memset(&path, 0, sizeof(MdmPathDescriptor));
               }

	       if(nextLevelOnly)
                  rc = phl_wrappedGetNextChildObjPathDesc(pRootPath, &path);
	       else 
               rc = phl_wrappedGetNextObjPathDesc(pRootPath, &path);
               if (rc == CMSRET_SUCCESS)
               {
                  /* If not paramOnly, just return path object */
                  if (!paramOnly)
                  {
                     /* done */
                     break;   /* out of while (rc == CMSRET_SUCCESS) */
                  }
               }
               else if (rc != CMSRET_NO_MORE_INSTANCES)
               {
                  cmsLog_error("mdm_getNextObjPathDesc error %d", rc);
               }
            }
         }
         else
         {
            cmsLog_error("mdm_getNextParamName error %d", rc);
         }
      }  /* while (rc == CMSRET_SUCCESS) */
   }

   if (rc == CMSRET_SUCCESS)
   {
      /* Return the next path */
      *pNextPath = path;
   }

   return rc;

}  /* End of phl_getNextPath() */


CmsRet cmsPhl_getNextPath(UBOOL8             paramOnly,
                          UBOOL8             nextLevelOnly,
                          const MdmPathDescriptor  *pRootPath,
                          MdmPathDescriptor  *pNextPath)
{
   UBOOL8 zones[MDM_MAX_LOCK_ZONES]={FALSE};
   CmsRet rc;

   if (pRootPath == NULL || pNextPath == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((rc = phl_fillLockZones(pRootPath, 1, FALSE, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }
   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pRootPath->oid);

   rc = phl_getNextPath(paramOnly, nextLevelOnly, pRootPath, pNextPath);

   lck_autoUnlockZones(zones, pRootPath->oid, __FUNCTION__);

   return rc;

}   /* End of cmsPhl_getNextPath() */


CmsRet cmsPhl_setParameterValues(PhlSetParamValue_t *pSetParamValueList,
                                 SINT32             numEntries)
{
   UBOOL8               zones[MDM_MAX_LOCK_ZONES]={FALSE};
   SINT32               i = 0, j = 0;
   PhlSetParamValue_t   *pSetParamValue;
   UBOOL8               writable;
   MdmNodeAttributes    nodeAttr;
   CmsRet               rc = CMSRET_SUCCESS;
   CmsRet               ret = CMSRET_SUCCESS;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if (numEntries == 0)
   {
      // Preserve current behavior, even though it is questionable.
      return CMSRET_SUCCESS;
   }
   if (pSetParamValueList == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((rc = phl_fillLockZonesParamValue(pSetParamValueList, numEntries, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }
   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pSetParamValueList->pathDesc.oid);

   cmsLog_debug("--->entered, numEntries=%d", numEntries);

   for (i = 0, pSetParamValue = pSetParamValueList;
        i < numEntries;
        i++, pSetParamValue++)
   {
      /*
       * An early fault may have been detected in doSetParameterValues.
       */
      if (pSetParamValue->status != CMSRET_SUCCESS)
      {
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      /*
       * make sure the caller is in the access list.
       * A good side effect of getting the attributes is that we verify
       * the pathDesc points to a valid object.
       */
      ret = mdm_getParamAttributes(&(pSetParamValue->pathDesc), &nodeAttr);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("invalid param %s%s",
                      mdm_oidToGenericPath(pSetParamValue->pathDesc.oid),
                      pSetParamValue->pathDesc.paramName);
         pSetParamValue->status = CMSRET_INVALID_PARAM_NAME;
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      if (!mdm_isInAccessList(nodeAttr.accessBitMask) &&
          !mdm_isFullWriteAccessEid(mdmLibCtx.eid))
      {
         const CmsEntityInfo *eInfo __attribute__ ((unused));

         eInfo = cmsEid_getEntityInfo(mdmLibCtx.eid);
         cmsLog_error("caller %s (eid=%d) is not in access list 0x%x",
                      eInfo->name, mdmLibCtx.eid, nodeAttr.accessBitMask);
         pSetParamValue->status = CMSRET_REQUEST_DENIED;
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      /* make sure pathDesc points to a writable param */
      mdm_getPathDescWritable(&(pSetParamValue->pathDesc), &writable);
      if (!writable && !mdm_isFullWriteAccessEid(mdmLibCtx.eid))
      {
         cmsLog_error("param %s%s is NOT writable",
                      mdm_oidToGenericPath(pSetParamValue->pathDesc.oid),
                      pSetParamValue->pathDesc.paramName);
         /*
          * Plugfest 1/21/08: should be 9008 (SET_NON_WRITABLE_PARAM)
          * not 9001 (REQUEST_DENIED).  Also, do not break out of the
          * loop.  Keep iterating through the params in the array to
          * gather more error codes on the other parameters.
          */
         pSetParamValue->status = CMSRET_SET_NON_WRITABLE_PARAM;
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      /* make sure the type given by the ACS matches what we think it is */
      if (cmsUtl_strcmp(pSetParamValue->pParamType,
                        mdm_getParamType(&pSetParamValue->pathDesc)) &&
          cmsUtl_strcmp(pSetParamValue->pParamType,
                        mdm_getParamBaseType(&pSetParamValue->pathDesc)))
      {
         cmsLog_error("invalid param type detected for %s%s, got=%s exp=%s",
                      mdm_oidToGenericPath(pSetParamValue->pathDesc.oid),
                      pSetParamValue->pathDesc.paramName,
                      pSetParamValue->pParamType,
                      mdm_getParamBaseType(&pSetParamValue->pathDesc));
         pSetParamValue->status = CMSRET_INVALID_PARAM_TYPE;
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      /* make sure the new string value is valid */
      pSetParamValue->status = mdm_validateString(&(pSetParamValue->pathDesc), pSetParamValue->pValue);
      if (pSetParamValue->status != CMSRET_SUCCESS)
      {
         cmsLog_error("invalid param value detected for %s%s, value=%s",
                      mdm_oidToGenericPath(pSetParamValue->pathDesc.oid),
                      pSetParamValue->pathDesc.paramName,
                      pSetParamValue->pValue);
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }

      /* make sure parameter name is not duplicated
       * to fix issue in UNH TR-069 Certification test case 5.59
       * SetParameterValues Same Parameter Multiple Times
       */
      for (j = (i + 1); j < numEntries; j++)
      {
         if ((pSetParamValue->pathDesc.oid == pSetParamValueList[j].pathDesc.oid) &&
             (cmsUtl_strcmp(pSetParamValue->pathDesc.paramName, pSetParamValueList[j].pathDesc.paramName) == 0) &&
             (cmsMdm_compareIidStacks(&(pSetParamValue->pathDesc.iidStack), &(pSetParamValueList[j].pathDesc.iidStack)) == 0))
         {
            pSetParamValue->status = CMSRET_INVALID_ARGUMENTS;
            pSetParamValueList[j].status = CMSRET_INVALID_ARGUMENTS;
         }
      }
      if (pSetParamValue->status != CMSRET_SUCCESS)
      {
         cmsLog_error("duplicate param name detected for %s%s, value=%s",
                      mdm_oidToGenericPath(pSetParamValue->pathDesc.oid),
                      pSetParamValue->pathDesc.paramName,
                      pSetParamValue->pValue);
         rc = CMSRET_INVALID_ARGUMENTS;
         continue;
      }
   }  /* for (i = 0; ....) */

   if (rc == CMSRET_SUCCESS)
   {
      /* call the object dispatch layer api */   
      rc = odl_set(pSetParamValueList, numEntries);
      if (rc != CMSRET_SUCCESS && rc != CMSRET_SUCCESS_REBOOT_REQUIRED)
      {
         cmsLog_error("odl_set error %d", rc);
      }
   }

   lck_autoUnlockZones(zones, pSetParamValueList->pathDesc.oid, __FUNCTION__);

   return rc;

}  /* End of cmsPhl_setParameterValues() */


/* Must be called with autoZoneLocking already done by caller. */
static CmsRet phl_getPathCount(const MdmPathDescriptor *pPathList,
                               SINT32            numEntries,
                               UBOOL8            paramOnly,
                               UBOOL8            nextLevelOnly,
                               SINT32            *pPathCnt)
{
   SINT32            i;
   const MdmPathDescriptor *pPath;
   CmsRet            rc = CMSRET_SUCCESS;

   *pPathCnt = 0;

   /* loop through the requested name list */
   for (i = 0, pPath = pPathList;
        i < numEntries && rc == CMSRET_SUCCESS;
        i++, pPath++)
   {
      if (pPath->paramName[0] != 0)
      {
         /* pPath is a parameter path */
         *pPathCnt += 1;
      }
      else
      {
         /* pPath is an object path */
         /* traverse the subtree below pPath to find the
          * total number of parameters in the subtree.
          */
         MdmPathDescriptor nextObj = *pPath;

         while (rc == CMSRET_SUCCESS)
         {
            UINT32   numParams = 0;

            if (!paramOnly)
            {
               /* count this object */
               *pPathCnt += 1;
            }

            /* Find the number of parameters contained in this object */
            rc = phl_wrappedGetParamNameCount(&nextObj, &numParams);
            if (rc == CMSRET_SUCCESS)
            {
               /* count the parameters */
               *pPathCnt += numParams;

               if (nextLevelOnly)
               {
                  if (paramOnly)
                  {
                     /* done counting all the parameters */
                     break;   /* out of while (rc == CMSRET_SUCCESS) */
                  }
                  else
                  {
                     /* nextLevelOnly && !paramOnly */
                     /* for this case, we need to count the direct child
                      * objects of pPath.
                      */
                     /* nextObj must be set to 0 prior to the first call
                      * to mdm_getNextChildObjPathDesc.
                      */
                     memset(&nextObj, 0, sizeof(MdmPathDescriptor));

                     while (rc == CMSRET_SUCCESS)
                     {
                        /* get the next child object of pPath */
                        rc = phl_wrappedGetNextChildObjPathDesc(pPath, &nextObj);
                        if (rc == CMSRET_SUCCESS)
                        {
                           *pPathCnt += 1;
                        }
                        else if (rc != CMSRET_NO_MORE_INSTANCES)
                        {
                           cmsLog_error("mdm_getNextChildObjPathDesc error %d", rc);
                        }
                     }
                  }
               }
               else
               {
                  /* get the next hierarchical object after nextObj */

                  /* if pPath and nextObj have the same object path,
                   * we know that this will be the first call to
                   * mdm_getNextObjPathDesc. nextObj must be set to 0
                   * prior to the first call to mdm_getNextObjPathDesc.
                   */
                  if (phl_sameObjDescs(pPath, &nextObj))
                  {
                     memset(&nextObj, 0, sizeof(MdmPathDescriptor));
                  }

                  rc = phl_wrappedGetNextObjPathDesc(pPath, &nextObj);
                  if (rc != CMSRET_SUCCESS && rc != CMSRET_NO_MORE_INSTANCES)
                  {
                     cmsLog_error("mdm_getNextObjPathDesc error %d", rc);
                  }
               }
            }
            else
            {
               cmsLog_error("mdm_getParamNameCount error %d", rc);
            }
         }  /* while (rc == CMSRET_SUCCESS) */
         
         if (rc == CMSRET_NO_MORE_INSTANCES)
         {
            rc = CMSRET_SUCCESS;
         }
      }
   }  /* for (i = 0; ...) */

   if (rc != CMSRET_SUCCESS)
   {
      *pPathCnt = 0;
   }

   return rc;

}  /* End of phl_getPathCount() */


CmsRet cmsPhl_getPathCount(const MdmPathDescriptor *pPathList,
                           SINT32            numEntries,
                           UBOOL8            paramOnly,
                           UBOOL8            nextLevelOnly,
                           SINT32            *pPathCnt)
{
   UBOOL8 zones[MDM_MAX_LOCK_ZONES]={FALSE};
   CmsRet rc;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if (pPathCnt == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }
   if (numEntries == 0)
   {
      // Preserve existing behavior, even though it is questionable.
      *pPathCnt = 0;
      return CMSRET_SUCCESS;
   }
   if (pPathList == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((rc = phl_fillLockZones(pPathList, numEntries, FALSE, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }
   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pPathList->oid);

   rc = phl_getPathCount(pPathList, numEntries, paramOnly, nextLevelOnly, pPathCnt);

   lck_autoUnlockZones(zones, pPathList->oid, __FUNCTION__);

   return rc;
}


CmsRet cmsPhl_getParamValueFlags(const MdmPathDescriptor   *pPath,
                                 UINT32 getFlags,
                                 PhlGetParamValue_t  **pParamValue)
{
   PhlGetParamValue_t   *pResp;
   CmsRet               rc = CMSRET_SUCCESS;

   if ((pPath == NULL) || (pParamValue == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   *pParamValue = NULL;

   if (pPath->paramName[0] == 0)
   {
      cmsLog_error("invalid parameter name");
      return CMSRET_INVALID_PARAM_NAME;
   }

   /* allocate memory for parameter value response */
   pResp = cmsMem_alloc(sizeof(PhlGetParamValue_t), ALLOC_ZEROIZE);
   if (pResp == NULL)
   {
      cmsLog_error("cmsMem_alloc failed");
      return CMSRET_INTERNAL_ERROR;
   }

   if ((rc = lck_autoLockZone(pPath->oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(pResp);
      return rc;
   }

   /* get parameter value from MDM */
   rc = odl_getFlags(pPath, getFlags, &(pResp->pValue));
   if (rc == CMSRET_SUCCESS)
   {
      pResp->pathDesc   = *pPath;
      pResp->pParamType = mdm_getParamBaseType(pPath);
      *pParamValue      = pResp;
   }
   else
   {
      cmsLog_debug("odl_getFlags error %d,  pPath->paramName %s", rc, pPath->paramName);
      cmsPhl_freeGetParamValueBuf(pResp, 1);
   }

   lck_autoUnlockZone(pPath->oid, __FUNCTION__);

   return rc;

}  /* End of cmsPhl_getParamValueFlags() */


CmsRet cmsPhl_getParameterValuesFlags(const MdmPathDescriptor  *pPathList,
                                      SINT32             numEntries,
                                      UBOOL8             nextLevelOnly,
                                      UINT32             getFlags,
                                      PhlGetParamValue_t **pParamValueList,
                                      SINT32             *pNumParamValueEntries)
{
   UBOOL8               zones[MDM_MAX_LOCK_ZONES]={FALSE};
   SINT32               i, numParams, numRespEntries;
   const MdmPathDescriptor    *pPath;
   PhlGetParamValue_t   *pRespBuf, *pResp;
   CmsRet               rc = CMSRET_SUCCESS;
   
   if ((pParamValueList == NULL) || (pNumParamValueEntries == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   *pParamValueList       = NULL;
   *pNumParamValueEntries = 0;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if (numEntries == 0)
   {
      // Preserve existng behavior, even though it is questionable.
      return CMSRET_SUCCESS;
   }
   if (pPathList == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((rc = phl_fillLockZones(pPathList, numEntries, FALSE, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }
   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pPathList->oid);

   /* find out how many parameter-value pairs will be in the response */
   numParams = 0;
   rc = phl_getPathCount(pPathList, numEntries, TRUE, nextLevelOnly, &numParams);
   if (rc != CMSRET_SUCCESS || numParams == 0)
   {
      lck_autoUnlockZones(zones, pPathList->oid, __FUNCTION__);
      return rc;
   }

   /* allocate memory for parameter value response */
   pRespBuf = cmsMem_alloc(numParams * sizeof(PhlGetParamValue_t), ALLOC_ZEROIZE);
   if (pRespBuf == NULL)
   {
      cmsLog_error("cmsMem_alloc failed");
      lck_autoUnlockZones(zones, pPathList->oid, __FUNCTION__);
      return CMSRET_INTERNAL_ERROR;
   }


   /* loop through the requested name list */

   pResp          = pRespBuf;
   numRespEntries = 0;

   for (i = 0, pPath = pPathList;
        i < numEntries && rc == CMSRET_SUCCESS;
        i++, pPath++)
   {
      if (pPath->paramName[0] != 0)
      {
         /* this is a parameter path */
         rc = odl_getFlags(pPath, getFlags, &(pResp->pValue));
         if (rc == CMSRET_SUCCESS)
         {
            pResp->pathDesc   = *pPath;
            pResp->pParamType = mdm_getParamBaseType(pPath);
            pResp++;
            numRespEntries++;
         }
         else
         {
            cmsLog_error("odl_getFlags error %d", rc);
         }
      }
      else
      {
         /* this is an object path */
         /* traverse the sub-tree below the object path */
         MdmPathDescriptor nextPath;

         /* set nextPath to 0 to start traversing */
         memset(&nextPath, 0, sizeof(MdmPathDescriptor));

         while (numRespEntries < numParams && rc == CMSRET_SUCCESS)
         {
            rc = phl_getNextPath(TRUE, nextLevelOnly, pPath, &nextPath);
            if (rc == CMSRET_SUCCESS)
            {
               rc = odl_getFlags(&nextPath, getFlags, &(pResp->pValue));
               if (rc == CMSRET_SUCCESS)
               {
                  pResp->pathDesc   = nextPath;
                  pResp->pParamType = mdm_getParamBaseType(&(pResp->pathDesc));
                  pResp++;
                  numRespEntries++;
               }
               else
               {
                  cmsLog_error("odl_getFlags error %d", rc);
               }
            }
            else if (rc == CMSRET_NO_MORE_INSTANCES)
            {
               rc = CMSRET_SUCCESS;
               break;   /* out of while (rc == CMSRET_SUCCESS) */
            }
            else
            {
               cmsLog_error("phl_getNextPath error %d", rc);
            }
         }  /* while (rc == CMSRET_SUCCESS) */
      }
   }  /* for (i = 0; ....) */

   if (rc == CMSRET_SUCCESS)
   {
      *pParamValueList       = pRespBuf;
      *pNumParamValueEntries = numRespEntries;
   }
   else
   {
      cmsPhl_freeGetParamValueBuf(pRespBuf, numParams);
   }

   lck_autoUnlockZones(zones, pPathList->oid, __FUNCTION__);

   return rc;

}  /* End of cmsPhl_getParameterValues() */


CmsRet cmsPhl_getParamInfo(const MdmPathDescriptor *pPath,
                           PhlGetParamInfo_t **pParamInfo)
{
   PhlGetParamInfo_t *pResp;
   CmsRet            rc = CMSRET_SUCCESS;
   CmsRet ret;

   if ((pPath == NULL) || (pParamInfo == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   *pParamInfo = NULL;

   /* allocate memory for parameter name info */
   pResp = cmsMem_alloc(sizeof(PhlGetParamInfo_t), ALLOC_ZEROIZE);
   if (pResp == NULL)
   {
      cmsLog_error("cmsMem_alloc failed");
      return CMSRET_INTERNAL_ERROR;
   }

   if ((rc = lck_autoLockZone(pPath->oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(pResp);
      return rc;
   }

   /* get info from MDM */
   if ((ret = mdm_getPathDescWritable(pPath, &(pResp->writable))) == CMSRET_SUCCESS)
   {
      if (IS_PARAM_NAME_PRESENT(pPath))
      {
         /*
          * The MdmPathDescriptor can point to an object or a parameter.
          * Only query about tr69c password if the MdmPathDescriptor points
          * to a parameter name.
          */
         ret = mdm_getParamIsTr69Password(pPath, &(pResp->isTr69Password));
         if (ret == CMSRET_SUCCESS)
         {
            MdmParamNode *paramNode;

            /* get the profile name of this parameter */
            if ((paramNode = mdm_getParamNode(pPath->oid, pPath->paramName)))
            {
               pResp->profile = paramNode->profile;
            }
         }
      }
      else
      {
         MdmObjectNode *objectNode;

         /* get the profile name of this object */
         if ((objectNode = mdm_getObjectNode(pPath->oid)))
         {
            pResp->profile = objectNode->profile;
         }
      }
   }

   if (ret == CMSRET_SUCCESS)
   {
      pResp->pathDesc = *pPath;
      *pParamInfo     = pResp;
   }
   else
   {
      rc = ret;
      cmsLog_error("error %d", rc);
      cmsMem_free(pResp);
   }

   lck_autoUnlockZone(pPath->oid, __FUNCTION__);

   return rc;

}  /* End of cmsPhl_getParamInfo() */


CmsRet cmsPhl_getParameterNames(const MdmPathDescriptor *pPath,
                                UBOOL8            nextLevelOnly,
                                PhlGetParamInfo_t **pParamInfoList,
                                SINT32            *pNumEntries)
{
   UBOOL8            zones[MDM_MAX_LOCK_ZONES]={FALSE};
   SINT32            numParams=0;
   SINT32            numRespEntries;
   PhlGetParamInfo_t *pRespBuf, *pResp;
   CmsRet            rc = CMSRET_SUCCESS;

   if ((pPath == NULL) || (pParamInfoList == NULL) || (pNumEntries == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   *pParamInfoList = NULL;
   *pNumEntries    = 0;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((rc = phl_fillLockZones(pPath, 1, FALSE, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }
   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pPath->oid);

   rc = phl_getPathCount(pPath, 1, FALSE, nextLevelOnly, &numParams);
   if (rc != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsPhl_getPathCount error %d", rc);
      lck_autoUnlockZones(zones, pPath->oid, __FUNCTION__);
      return rc;
   }

   if (numParams == 0)
   {
      cmsLog_error("numParams = 0");
      lck_autoUnlockZones(zones, pPath->oid, __FUNCTION__);
      return CMSRET_INTERNAL_ERROR;
   }

   /* allocate memory for parameter name response */
   pRespBuf = cmsMem_alloc(numParams * sizeof(PhlGetParamInfo_t), ALLOC_ZEROIZE);
   if (pRespBuf == NULL)
   {
      cmsLog_error("cmsMem_alloc failed");
      lck_autoUnlockZones(zones, pPath->oid, __FUNCTION__);
      return CMSRET_INTERNAL_ERROR;
   }


   pResp          = pRespBuf;
   numRespEntries = 0;

   if (pPath->paramName[0] != 0)
   {
      /* this is a parameter name */
      pResp->pathDesc = *pPath;

      rc = mdm_getPathDescWritable(&(pResp->pathDesc), &(pResp->writable));
      if (rc == CMSRET_SUCCESS)
      {
         pResp++;
         numRespEntries++;
      }
      else
      {
         cmsLog_error("mdm_getPathDescWritable error %d", rc);
      }
   }
   else
   {
      /* this is an object name */
      /* traverse the sub-tree below the object node */

      MdmPathDescriptor nextPath;

      /* set nextPath to 0 to start traversing */
      memset(&nextPath, 0, sizeof(MdmPathDescriptor));

      while (rc == CMSRET_SUCCESS)
      {
         rc = phl_getNextPath(FALSE, nextLevelOnly, pPath, &nextPath);
         if (rc == CMSRET_SUCCESS)
         {
            rc = mdm_getPathDescWritable(&nextPath, &(pResp->writable));
            if (rc == CMSRET_SUCCESS)
            {
               pResp->pathDesc = nextPath;
               pResp++;
               numRespEntries++;
            }
            else
            {
               cmsLog_error("mdm_getPathDescWritable error %d", rc);
            }
         }
         else if (rc == CMSRET_NO_MORE_INSTANCES)
         {
            rc = CMSRET_SUCCESS;
            break;   /* out of while (rc == CMSRET_SUCCESS) */
         }
         else
         {
            cmsLog_error("phl_getNextPath error %d", rc);
         }
      }  /* while (rc == CMSRET_SUCCESS) */
   }

   if (rc == CMSRET_SUCCESS)
   {
      *pParamInfoList = pRespBuf;
      *pNumEntries    = numRespEntries;
   }
   else
   {
      cmsMem_free(pRespBuf);
   }

   lck_autoUnlockZones(zones, pPath->oid, __FUNCTION__);

   return rc;

}  /* End of cmsPhl_getParameterNames() */


CmsRet cmsPhl_setParameterAttributes(const PhlSetParamAttr_t *pSetParamAttrList,
                                     SINT32            numEntries)
{
   UBOOL8            zones[MDM_MAX_LOCK_ZONES]={FALSE};
   SINT32            i;
   const PhlSetParamAttr_t *pSetParamAttr;
   UBOOL8            testOnly = TRUE;
   CmsRet            rc = CMSRET_SUCCESS;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if (numEntries == 0)
   {
      // Preserve existing behavior, even though it is questionable.
      return CMSRET_SUCCESS;
   }
   if (pSetParamAttrList == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((rc = phl_fillLockZonesParamAttr(pSetParamAttrList, numEntries, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }
   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pSetParamAttrList->pathDesc.oid);

   /*
    * We need to loop through the setParamAttrList twice,
    * first with testOnly = TRUE, and if there are no errors, loop
    * through a second time with testOnly = FALSE.  This is to satisfy
    * TR69 atomic set requirements.  See A.3.2.4.
    */

   /* Perhaps we should only allow EID_TR69C to do this?  i.e. check mdmLibCtx.eid? */
    
   /* loop through the set parameter attribute list */
   pSetParamAttr = pSetParamAttrList;
   for (i = 0; i < numEntries && rc == CMSRET_SUCCESS; i++, pSetParamAttr++)
   {
      if ((pSetParamAttr->attributes.accessBitMaskChange == 0) &&
          (pSetParamAttr->attributes.notificationChange  == 0))
      {
         continue;   /* does not change anything */
      }

      rc = mdm_setParamAttributes(&(pSetParamAttr->pathDesc), &(pSetParamAttr->attributes), testOnly);
      if (rc != CMSRET_SUCCESS)
      {
         lck_autoUnlockZones(zones, pSetParamAttrList->pathDesc.oid, __FUNCTION__);
         return rc;
      }
   }  /* for (i = 0; ....) */

   /* OK, the test run succeeded, now do the real set */
   testOnly = FALSE;
   pSetParamAttr = pSetParamAttrList;
   for (i = 0; i < numEntries && rc == CMSRET_SUCCESS; i++, pSetParamAttr++)
   {
      if ((pSetParamAttr->attributes.accessBitMaskChange == 0) &&
          (pSetParamAttr->attributes.notificationChange  == 0))
      {
         continue;   /* does not change anything */
      }

      rc = mdm_setParamAttributes(&(pSetParamAttr->pathDesc), &(pSetParamAttr->attributes), FALSE);
      if (rc != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_setParamAttributes failure %d", rc);
      }
   }  /* for (i = 0; ....) */

   lck_autoUnlockZones(zones, pSetParamAttrList->pathDesc.oid, __FUNCTION__);

   return rc;

}  /* End of cmsPhl_setParameterAttributes() */


CmsRet cmsPhl_getParamAttr(const MdmPathDescriptor *pPath,
                           PhlGetParamAttr_t **pParamAttr)
{
   PhlGetParamAttr_t    *pResp;
   CmsRet               rc = CMSRET_SUCCESS;

   if ((pPath == NULL) || (pParamAttr == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   *pParamAttr = NULL;

   if (pPath->paramName[0] == 0)
   {
      cmsLog_error("invalid parameter name");
      return CMSRET_INVALID_PARAM_NAME;
   }

   /* allocate memory for parameter value response */
   pResp = cmsMem_alloc(sizeof(PhlGetParamAttr_t), ALLOC_ZEROIZE);
   if (pResp == NULL)
   {
      cmsLog_error("cmsMem_alloc failed");
      return CMSRET_INTERNAL_ERROR;
   }

   if ((rc = lck_autoLockZone(pPath->oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(pResp);
      return rc;
   }

   /* get parameter attributes from MDM */
   rc = mdm_getParamAttributes(pPath, &(pResp->attributes));
   if (rc == CMSRET_SUCCESS)
   {
      pResp->pathDesc = *pPath;
      *pParamAttr     = pResp;
   }
   else
   {
      cmsLog_error("mdm_getParamAttributes error %d", rc);
      cmsMem_free(pResp);
   }

   lck_autoUnlockZone(pPath->oid, __FUNCTION__);

   return rc;

}  /* End of cmsPhl_getParamAttr() */


CmsRet cmsPhl_getParameterAttributes(const MdmPathDescriptor *pPathList,
                                     SINT32            numEntries,
                                     UBOOL8            nextLevelOnly,
                                     PhlGetParamAttr_t **pParamAttrList,
                                     SINT32            *pNumParamAttrEntries)
{
   UBOOL8            zones[MDM_MAX_LOCK_ZONES]={FALSE};
   SINT32            i, numParams, numRespEntries;
   const MdmPathDescriptor *pPath;
   PhlGetParamAttr_t *pRespBuf, *pResp;
   CmsRet            rc = CMSRET_SUCCESS;

   if ((pParamAttrList == NULL) || (pNumParamAttrEntries == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   *pParamAttrList       = NULL;
   *pNumParamAttrEntries = 0;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if (numEntries == 0)
   {
      // Preserve existing behavior, even though it is questionable.
      return CMSRET_SUCCESS;
   }
   if (pPathList == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((rc = phl_fillLockZones(pPathList, numEntries, FALSE, zones)) != CMSRET_SUCCESS)
   {
      return rc;
   }
   PHL_AUTOLOCK_ZONES_AND_RETURN_ON_ERROR(pPathList->oid);

   /* find out how many parameter-value pairs will be in the response */
   numParams = 0;
   rc = phl_getPathCount(pPathList, numEntries, TRUE, nextLevelOnly, &numParams);
   if (rc != CMSRET_SUCCESS || numParams == 0)
   {
      lck_autoUnlockZones(zones, pPathList->oid, __FUNCTION__);
      return rc;
   }

   /* allocate memory for parameter attribute response */
   pRespBuf = cmsMem_alloc(numParams * sizeof(PhlGetParamAttr_t), ALLOC_ZEROIZE);
   if (pRespBuf == NULL)
   {
      cmsLog_error("cmsMem_alloc failed");
      lck_autoUnlockZones(zones, pPathList->oid, __FUNCTION__);
      return CMSRET_INTERNAL_ERROR;
   }


   /* loop through the requested name list */
   
   pResp          = pRespBuf;
   numRespEntries = 0;

   for (i = 0, pPath = pPathList;
        i < numEntries && rc == CMSRET_SUCCESS;
        i++, pPath++)
   {
      if (pPath->paramName[0] != 0)
      {
         /* this is a parameter path */
         rc = mdm_getParamAttributes(pPath, &(pResp->attributes));
         if (rc == CMSRET_SUCCESS)
         {
            pResp->pathDesc = *pPath;
            pResp++;
            numRespEntries++;
         }
         else
         {
            cmsLog_error("mdm_getParamAttributes error %d", rc);
         }
      }
      else
      {
         /* this is an object path */
         /* traverse the sub-tree below the object node */
         MdmPathDescriptor nextPath;

         /* set nextPath to 0 to start traversing */
         memset(&nextPath, 0, sizeof(MdmPathDescriptor));

         while (rc == CMSRET_SUCCESS)
         {
            rc = phl_getNextPath(TRUE, nextLevelOnly, pPath, &nextPath);
            if (rc == CMSRET_SUCCESS)
            {
               rc = mdm_getParamAttributes(&nextPath, &(pResp->attributes));
               if (rc == CMSRET_SUCCESS)
               {
                  pResp->pathDesc = nextPath;
                  pResp++;
                  numRespEntries++;
               }
               else
               {
                  cmsLog_error("mdm_getParamAttributes error %d", rc);
               }
            }
            else if (rc == CMSRET_NO_MORE_INSTANCES)
            {
               rc = CMSRET_SUCCESS;
               break;
            }
            else
            {
               cmsLog_error("phl_getNextPath error %d", rc);
            }
         }  /* while (rc == CMSRET_SUCCESS) */
      }
   }  /* for (i = 0; ....) */

   if (rc == CMSRET_SUCCESS)
   {
      *pParamAttrList       = pRespBuf;
      *pNumParamAttrEntries = numRespEntries;
   }
   else
   {
      cmsMem_free(pRespBuf);
   }

   lck_autoUnlockZones(zones, pPathList->oid, __FUNCTION__);

   return rc;

}  /* End of cmsPhl_getParameterAttributes() */


CmsRet cmsPhl_addObjInstance(MdmPathDescriptor *pPath)
{
   CmsRet   rc = CMSRET_SUCCESS;

   if (pPath == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (pPath->paramName[0] != 0)
   {
      cmsLog_error("invalid object path");
      return CMSRET_INVALID_PARAM_NAME;
   }

   if ((rc = lck_autoLockZone(pPath->oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return rc;
   }

   /* do we have to check access list for add operation? */

   rc = odl_addObjectInstance(pPath);
   if (rc != CMSRET_SUCCESS)
   {
      cmsLog_error("odl_addObjInstance failure %d", rc);
   }

   lck_autoUnlockZone(pPath->oid, __FUNCTION__);

   return rc;
      
}  /* End of cmsPhl_addObjInstance() */


CmsRet cmsPhl_delObjInstance(const MdmPathDescriptor *pPath)
{
   CmsRet   rc = CMSRET_SUCCESS;

   if (pPath == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (pPath->paramName[0] != 0)
   {
      cmsLog_error("Invalid object path");
      return CMSRET_INVALID_ARGUMENTS;
   }

   if ((rc = lck_autoLockZone(pPath->oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return rc;
   }

   /* do we have to check access list for delete operation? */

   rc = odl_deleteObjectInstance(pPath);
   if (rc != CMSRET_SUCCESS)
   {
      cmsLog_error("odl_delObjInstance failure %d", rc);
   }

   lck_autoUnlockZone(pPath->oid, __FUNCTION__);

   return rc;

}  /* End of cmsPhl_delObjInstance() */


void cmsPhl_freeGetParamValueBuf(PhlGetParamValue_t *pBuf,
                                 SINT32             numEntries)
{
   SINT32               i;
   PhlGetParamValue_t   *ptr = pBuf;

   for (i = 0; (i < numEntries) && (ptr != NULL); i++, ptr++)
   {
      cmsMem_free(ptr->pValue);
   }
   cmsMem_free(pBuf);

}  /* End of cmsPhl_freeGetParamValueBuf() */


UINT32 cmsPhl_getNumberOfParamValueChanges(void)
{
   UINT32 n = 0;
   CmsRet ret;

   CHECK_MDM_EXTERNAL_CALLER_0(__FUNCTION__);

   ret = lck_autoLockAllZonesWithBackoff(0, PHL_LOCK_TIMEOUT, __FUNCTION__);
   if (ret == CMSRET_SUCCESS)
   {
      n = mdm_getNumberOfParamValueChanges();
      lck_autoUnlockAllZones(__FUNCTION__);
   }
   else
   {
      cmsLog_error("Could not acquire all locks, ret=%d", ret);
   }

   return n;

}  /* End of cmsPhl_getNumberOfParamValueChanges() */

UBOOL8 cmsPhl_isParamValueChanged(const MdmPathDescriptor *pathDesc)
{
   UBOOL8 ret = FALSE;

   if (pathDesc == NULL)
   {
      cmsLog_error("NULL pathDesc");
      return FALSE;
   }

   CHECK_MDM_EXTERNAL_CALLER_0(__FUNCTION__);

   if (lck_autoLockZone(pathDesc->oid, __FUNCTION__) == CMSRET_SUCCESS)
   {
      ret = mdm_isParamValueChanged(pathDesc);
      lck_autoUnlockZone(pathDesc->oid, __FUNCTION__);
   }

   return ret;

}  /* End of cmsPhl_isParamValueChanged() */

void cmsPhl_clearAllParamValueChanges(void)
{
   CmsRet ret;

   CHECK_MDM_EXTERNAL_CALLER_V(__FUNCTION__);

   ret = lck_autoLockAllZonesWithBackoff(0, PHL_LOCK_TIMEOUT, __FUNCTION__);
   if (ret == CMSRET_SUCCESS)
   {
      mdm_clearAllParamValueChanges();
      lck_autoUnlockAllZones(__FUNCTION__);
   }
   else
   {
      cmsLog_error("Could not acquire all locks, ret=%d", ret);
   }

   return;

}  /* End of cmsPhl_clearAllParamValueChanges() */
