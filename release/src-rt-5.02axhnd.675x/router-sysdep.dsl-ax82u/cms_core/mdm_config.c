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
#include "cms_util.h"
#include "cms_mgm.h"
#include "cms_lck.h"
#include "nanoxml.h"
#include "mdm.h"
#include "mdm_private.h"
#include "oal.h"


/** This file deals only with writing a config file out.
 *
 */
 

static CmsRet dumpObjectNode(const MdmObjectNode *objNode,
                             const InstanceIdStack *iidStack,
                             const MdmNodeAttributes *parentAttr,
                             const char *indentBuf,
                             char *buf,
                             UINT32 *idx,
                             UINT32 max);
static void dumpChangedParams(const MdmObjectNode *objNode,
                              const InstanceIdStack *iidStack,
                              const MdmNodeAttributes *nodeAttr,
                              const AttributesDescNode *attrDesc,
                              const char *indentBuf,
                              char *buf,
                              UINT32 *idx,
                              UINT32 max);
static UBOOL8 hasAnyParamsOrAttrsChanged(const MdmObjectNode *objNode,
                                         const InstanceIdStack *iidStack,
                                         const MdmNodeAttributes *parentAttr);

/** this is a context object that is used for calls to mdm_traverseParamNodes */
struct changed_params_context
{
   UBOOL8 changed;
   UBOOL8 isDynamicIpoe;  /**< are we currently processing a WANIPConnection object with
                           * connectionType=routed, addressingType=DHCP */
   char *buf;
   char *indentBuf;
   UINT32 idx;
   UINT32 max;
   UINT32 paramIndex;
   MdmObjectId parentOid;    /**< needed by PRN_COUNT_PERSISTENT_INSTANCE */
   InstanceIdStack iidStack; /**< needed by PRN_COUNT_PERSISTENT_INSTANCE */
   const MdmNodeAttributes *nodeAttr;
   AttributesDescNode *attrDesc;
};



struct nanoxml_context nxmlCtx;


CmsRet mdm_serializeToBuf(char *buf, UINT32 *len)
{
   UINT32 wrote;
   UINT32 idx = 0;
   UINT32 max = *len;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmNodeAttributes attr;
   char indentBuf[1]={0};  /* initial indent is no spaces/empty string */
   CmsRet ret=CMSRET_SUCCESS;

   INIT_NODE_ATTRIBUTES(&attr);
   attr.accessBitMask = DEFAULT_ACCESS_BIT_MASK;
   attr.notification = DEFAULT_NOTIFICATION_VALUE;

   if (mdmLibCtx.dumpAll)
   {
      wrote = snprintf(buf, max, "Dump of Entire MDM, this is NOT the config file\n");
   }
   else
   {
      wrote = snprintf(buf, max, "<?xml version=\"1.0\"?>\n<%s %s=\"%s\">\n",
                       CONFIG_FILE_TOP_NODE,
                       CONFIG_FILE_ATTR_VERSION,
                       CMS_CONFIG_FILE_VERSION);
   }

   if (wrote < max)
   {
      max -= wrote;
      idx += wrote;

      ret = dumpObjectNode(mdmShmCtx->rootObjNode, &iidStack, &attr, indentBuf, buf, &idx, max);

      if ((idx < max) && (!mdmLibCtx.dumpAll))
      {
         wrote = snprintf(&(buf[idx]), max - idx, "</%s>\n", CONFIG_FILE_TOP_NODE);
         idx += (wrote < max - idx) ? wrote : max - idx;
      }
   }

   if (ret != CMSRET_SUCCESS)
   {
      *len = 0;
      return ret;
   }
   else if (idx >= max)
   {
      cmsLog_error("config file could not fit in %d bytes", (*len));
      return CMSRET_RESOURCE_EXCEEDED;
   }
   else
   {
      *len = idx + 1; /* plus 1 to include last NULL char */
      return CMSRET_SUCCESS;
   }
}


CmsRet mdm_serializeObjectToBuf(const MdmObjectId oid, char *buf, UINT32 *len)
{
   UINT32 wrote;
   UINT32 idx = 0;
   UINT32 max = *len;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmObjectNode *objNode = NULL;
   char indentBuf[1]={0};  /* initial indent is no spaces/empty string */
   CmsRet ret=CMSRET_SUCCESS;

   objNode = mdm_getObjectNode(oid);
   if (objNode == NULL)
   {
      cmsLog_error("Cannot get object node from OID=%d", oid);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   wrote = snprintf(buf, max, "Dump %s in MDM\n", objNode->name);

   if (wrote < max)
   {
      max -= wrote;
      idx += wrote;

      ret = dumpObjectNode(objNode, &iidStack, &(objNode->nodeAttr), indentBuf, buf, &idx, max);

      if (idx < max)
      {
         wrote = snprintf(&(buf[idx]), max - idx, "</%s>\n", CONFIG_FILE_TOP_NODE);
         idx += (wrote < max - idx) ? wrote : max - idx;
      }
   }

   if (ret != CMSRET_SUCCESS)
   {
      *len = 0;
      return ret;
   }
   else if (idx >= max)
   {
      cmsLog_error("object information could not fit in %d bytes", (*len));
      return CMSRET_RESOURCE_EXCEEDED;
   }
   else
   {
      *len = idx + 1; /* plus 1 to include last NULL char */
      return CMSRET_SUCCESS;
   }
}


/** Dump the contents of the specified objNode to buf, and also recurse
 *  to child objNodes.
 *
 */
CmsRet dumpObjectNode(const MdmObjectNode *objNode,
                      const InstanceIdStack *iidStack,
                      const MdmNodeAttributes *parentAttr,
                      const char *indentBuf,
                      char *buf,
                      UINT32 *idx,
                      UINT32 max)
{
   CmsRet ret=CMSRET_SUCCESS;
   
   if ((IS_INDIRECT0(objNode)) ||
       (IS_INDIRECT1(objNode)) ||
       (IS_INDIRECT2(objNode) && (DEPTH_OF_IIDSTACK(iidStack) == objNode->instanceDepth)))
   {
      MdmNodeAttributes *currAttr=NULL;
      AttributesDescNode *attrDesc=NULL;

      if ((mdmLibCtx.dumpAll) ||
          (hasAnyParamsOrAttrsChanged(objNode, iidStack, parentAttr) && !(objNode->flags & OBN_PRUNE_WRITE_TO_CONFIG_FILE)))
      {
         UINT32 i, wrote;
         char *indentBuf2;
         UINT32 indentBuf2Len;

         /*
          * get the attributes for the current node and the
          * parameter specific attributes chain, if there is one.
          * Do this before we malloc so we can return easily.
          */
         ret = mdm_getAllNodeAttributes(objNode, iidStack, &currAttr, &attrDesc);
         if (ret != CMSRET_SUCCESS)
         {
            /* this shouldn't happen */
            cmsLog_error("could not find attribute info for %s %s",
                         mdm_oidToGenericPath(objNode->oid),
                         cmsMdm_dumpIidStack(iidStack));
            cmsAst_assert(0);
            return CMSRET_INVALID_ARGUMENTS;
         }


         /*
          * Dump the parameters of this node and recurse to child objNodes.
          */
         indentBuf2Len = strlen(indentBuf) + CMS_CONFIG_FILE_INDENT + 1;
         indentBuf2 = cmsMem_alloc(indentBuf2Len, ALLOC_ZEROIZE);

         if (indentBuf2 != NULL) {

            memset(indentBuf2, 0x20, indentBuf2Len - 1); /* fill with spaces */


            if (IS_INDIRECT2(objNode))
            {
                UINT32 topInstanceId = PEEK_INSTANCE_ID(iidStack);
               wrote = snprintf(&buf[*idx], max - (*idx), "%s<%s %s=\"%u\"",
                                indentBuf2, objNode->name, CONFIG_FILE_ATTR_INSTANCE, topInstanceId);
            }
            else
            {
               wrote = snprintf(&buf[*idx], max - (*idx), "%s<%s", indentBuf2, objNode->name);
            }
            *idx += (wrote < max - (*idx)) ? wrote : max - (*idx);


            /* check for changed notification settings */
            if (currAttr->notification != parentAttr->notification)
            {
               wrote = snprintf(&buf[*idx], max - (*idx), " %s=\"%d\"", CONFIG_FILE_ATTR_NOTIFICATION, currAttr->notification);
               *idx += (wrote < max - (*idx)) ? wrote : max - (*idx);
            }

            /* check for changed accessBitMask */
            if (currAttr->accessBitMask != parentAttr->accessBitMask)
            {
               char *accessStr=NULL;

               cmsEid_getStringNamesFromBitMask(currAttr->accessBitMask, &accessStr);
               if (strlen(accessStr) > 0)
               {
                  wrote = snprintf(&buf[*idx], max - (*idx), " %s=\"%s\"", CONFIG_FILE_ATTR_ACCESS_LIST, accessStr);
                  *idx += (wrote < max - (*idx)) ? wrote : max - (*idx);
               }
               CMSMEM_FREE_BUF_AND_NULL_PTR(accessStr);
            }

            wrote = snprintf(&buf[*idx], max - (*idx), ">\n");
            *idx += (wrote < max - (*idx)) ? wrote : max - (*idx);


            /* Go write out the params */
            dumpChangedParams(objNode, iidStack, currAttr, attrDesc, indentBuf2, buf, idx, max);


            for (i=0; i < objNode->numChildObjNodes && ((*idx) < max); i++)
            {
               dumpObjectNode(&(objNode->childObjNodes[i]), iidStack, currAttr, indentBuf2, buf, idx, max);
            }

            wrote = snprintf(&buf[*idx], max - (*idx), "%s</%s>\n", indentBuf2, objNode->name);
            *idx += (wrote < max - (*idx)) ? wrote : max - (*idx);

            CMSMEM_FREE_BUF_AND_NULL_PTR(indentBuf2);
         }
      }
   }
   else if (IS_INDIRECT2(objNode) && (DEPTH_OF_IIDSTACK(iidStack)+1 == objNode->instanceDepth))
   {
      InstanceIdStack childIidStack;
      InstanceHeadNode *instHead;
      InstanceDescNode *instDesc;
      UINT32 wrote;

      instHead = mdm_getInstanceHead(objNode, iidStack);
      instDesc = (InstanceDescNode *) instHead->objData;
      while ((instDesc != NULL) && ((*idx) < max))
      {
         /* recursively dump each instance of this object, which is the top of a sub-tree */
         if ((mdmLibCtx.dumpAll) || !(instDesc->flags & IDN_NON_PERSISTENT_INSTANCE))
         {
            childIidStack = *iidStack;
            PUSH_INSTANCE_ID(&childIidStack, instDesc->instanceId);

            dumpObjectNode(objNode, &childIidStack, parentAttr, indentBuf, buf, idx, max);

         }
         instDesc = instDesc->next;
      }

      /* if we do not care to save the object, then we do not care about the next instanceId either; this way, the instance
	 should always start at 1 */
      if ((instHead->nextInstanceIdToAssign != 1)
	  && ((mdmLibCtx.dumpAll) || !(objNode->flags & OBN_PRUNE_WRITE_TO_CONFIG_FILE)))
      {
         wrote = snprintf(&buf[*idx], max - (*idx), "  %s<%s %s=\"%u\" ></%s>\n", indentBuf, objNode->name, CONFIG_FILE_ATTR_NEXT_INSTANCE, instHead->nextInstanceIdToAssign, objNode->name);
         *idx += (wrote < max - (*idx)) ? wrote : max - (*idx);
      }
   }
   else
   {
      cmsLog_error("could not determine action, objNode %s depth=%d iidStack=%s",
                   mdm_oidToGenericPath(objNode->oid),
                   objNode->instanceDepth,
                   cmsMdm_dumpIidStack(iidStack));
      cmsAst_assert(0);
      ret = CMSRET_INTERNAL_ERROR;
   }


   return ret;
}


static UBOOL8 changedParamsCallbackFunc(const MdmParamNode *paramNode,
                                        void *newVal __attribute__((unused)),
                                        void *currVal,
                                        void *cbContext)
{
   struct changed_params_context *ctx = (struct changed_params_context *) cbContext;
   UINT32 wrote;
   char valBuf[BUFLEN_32];
   char *currStr;
   AttributesDescNode *attrDesc;
   UBOOL8 doTraverse = TRUE;

   currStr = valBuf;
   ctx->changed = FALSE;

   /* first check if the param's has a unique attrDesc entry */
   if ((attrDesc = mdm_findAttributesDesc(ctx->attrDesc, ctx->paramIndex)) != NULL)
   {
      if (mdm_compareNodeAttributes(ctx->nodeAttr, &(attrDesc->nodeAttr)))
      {
         ctx->changed = TRUE;
      }
   }

   if ((paramNode->flags & PRN_NEVER_WRITE_TO_CONFIG_FILE) && (!mdmLibCtx.dumpAll))
   {
      /*
       * Don't set ctx->changed to FALSE because it could be true
       * due to changes in node attributes.  Set currStr to default value string,
       * even if ctx->changed == TRUE, we will only print out the
       * attributes, and the default value of the parameter.
       */
      if (paramNode->defaultValue) 
      {
         currStr = paramNode->defaultValue;
      }
      else /* default value is NULL */
      { 
         /* for all number types, set the value to 0 */
         if (paramNode->type == MPT_INTEGER || 
             paramNode->type == MPT_UNSIGNED_INTEGER ||
             paramNode->type == MPT_LONG64 ||
             paramNode->type == MPT_UNSIGNED_LONG64) 
         {
            sprintf(valBuf, "0");
         }
         else if (paramNode->type == MPT_BOOLEAN) 
         {
             /* boolean type with no specified default value defaults to false */
             sprintf(valBuf, "FALSE");
         }
         else if (paramNode->type == MPT_DATE_TIME) 
         {
            /* datetime type with no specified default value defaults to 0001-01-01T00:00:00Z */
            sprintf(valBuf, "0001-01-01T00:00:00Z");
         }
         else if (paramNode->type == MPT_HEX_BINARY) 
         {
            sprintf(valBuf, "00");
         }
         else 
         {
            /* for string types, setting currStr to NULL is OK */
            currStr = paramNode->defaultValue;
         }
      } 
   }
   else if (!mdmLibCtx.dumpAll && ctx->isDynamicIpoe &&
            (!strcmp(paramNode->name, "ExternalIPAddress") ||
             !strcmp(paramNode->name, "SubnetMask") ||
             !strcmp(paramNode->name, "DefaultGateway") ||
             !strcmp(paramNode->name, "DNSServers") ||
             !strcmp(paramNode->name, "IPInterfaceIPAddress") ||
             !strcmp(paramNode->name, "IPInterfaceSubnetMask")))
   {
      /* see comment in dumpChangedParams */
      currStr = paramNode->defaultValue;
   }
   else 
   {
   /*
    * Even if we know attributes has changed, we still need to go through this
    * switch code below so we get the value string into valBuf.
    */
   switch(paramNode->type)
   {
   case MPT_STRING:
   case MPT_DATE_TIME:
   case MPT_BASE64:
   case MPT_HEX_BINARY:
   case MPT_UUID:
   case MPT_IP_ADDR:
   case MPT_MAC_ADDR:
      {
         char **currStrPtr = (char **) currVal;
         currStr = (*currStrPtr);

         if (paramNode->flags & PRN_ALWAYS_WRITE_TO_CONFIG_FILE)
         {
            ctx->changed = TRUE;
            break;
         }

         if (cmsUtl_strcmp(currStr, paramNode->defaultValue))
         {
            if (IS_PARAM_WRITABLE(paramNode))
            {
               ctx->changed = TRUE;
            }
         }

         break;
      }

   case MPT_INTEGER:
      {
         SINT32 *ivalPtr = (SINT32 *) currVal;
         SINT32 ival = (*ivalPtr);

         /* convert integer to string */
         sprintf(valBuf, "%d", ival);

         if (paramNode->flags & PRN_ALWAYS_WRITE_TO_CONFIG_FILE)
         {
            ctx->changed = TRUE;
            break;
         }

         if (((paramNode->defaultValue == NULL) && (ival != 0)) ||
             ((paramNode->defaultValue != NULL) && (strcmp(valBuf, paramNode->defaultValue))))
         {
            if (IS_PARAM_WRITABLE(paramNode))
            {
               ctx->changed = TRUE;
            }
         }
         break;
      }

   case MPT_UNSIGNED_INTEGER:
   case MPT_STATS_COUNTER32:
      {
         UINT32 *uvalPtr = (UINT32 *) currVal;
         UINT32 uval = (*uvalPtr);

         if ((paramNode->flags & PRN_COUNT_PERSISTENT_INSTANCE) &&
             (!mdmLibCtx.dumpAll))
         {
            /*
             * When we write to config file, we want to report the actual
             * number of persistent instances, not just the value of of the
             * xyzNumberOfEntries parameter in the MDM.
             * But if we are currently doing a "dumpmdm" (mdmLibCtx.dumpAll),
             * then dump the actual value in the MDM.
             */
            UINT32 count=0;
            if (CMSRET_SUCCESS == mdm_getPersistentInstanceCount(paramNode->name,
                                    ctx->parentOid, &ctx->iidStack, &count))
            {
               sprintf(valBuf, "%u", count);
            }
            else
            {
               cmsLog_error("Could not get persistent instance count, set to 0");
               sprintf(valBuf, "0");
            }
         }
         else
         {
            /* convert integer to string */
            sprintf(valBuf, "%u", uval);
         }

         if (paramNode->flags & PRN_ALWAYS_WRITE_TO_CONFIG_FILE)
         {
            ctx->changed = TRUE;
            break;
         }

         if (((paramNode->defaultValue == NULL) && (uval != 0)) ||
             ((paramNode->defaultValue != NULL) && (strcmp(valBuf, paramNode->defaultValue))))
         {
            if (IS_PARAM_WRITABLE(paramNode))
            {
               ctx->changed = TRUE;
            }
         }
         break;
      }

   case MPT_LONG64:
      {
         SINT64 *ivalPtr = (SINT64 *) currVal;
         SINT64 ival = (*ivalPtr);

         /* convert integer to string */
         sprintf(valBuf, "%" PRId64, ival);

         if (paramNode->flags & PRN_ALWAYS_WRITE_TO_CONFIG_FILE)
         {
            ctx->changed = TRUE;
            break;
         }

         if (((paramNode->defaultValue == NULL) && (ival != 0)) ||
             ((paramNode->defaultValue != NULL) && (strcmp(valBuf, paramNode->defaultValue))))
         {
            if (IS_PARAM_WRITABLE(paramNode))
            {
               ctx->changed = TRUE;
            }
         }
         break;
      }

   case MPT_UNSIGNED_LONG64:
   case MPT_STATS_COUNTER64:
      {
         UINT64 *uvalPtr = (UINT64 *) currVal;
         UINT64 uval = (*uvalPtr);;

         /* convert integer to string */
         sprintf(valBuf, "%" PRIu64, uval);

         if (paramNode->flags & PRN_ALWAYS_WRITE_TO_CONFIG_FILE)
         {
            ctx->changed = TRUE;
            break;
         }

         if (((paramNode->defaultValue == NULL) && (uval != 0)) ||
             ((paramNode->defaultValue != NULL) && (strcmp(valBuf, paramNode->defaultValue))))
         {
            if (IS_PARAM_WRITABLE(paramNode))
            {
               ctx->changed = TRUE;
            }
         }
         break;
      }

   case MPT_BOOLEAN:
      {
         UBOOL8 *bvalPtr = (UBOOL8 *) currVal;
         UBOOL8 bval = (*bvalPtr);

         sprintf(valBuf, "%s", (bval ? "TRUE" : "FALSE"));

         if (paramNode->flags & PRN_ALWAYS_WRITE_TO_CONFIG_FILE)
         {
            ctx->changed = TRUE;
            break;
         }

         if (bval == FALSE)
         {
            if ((paramNode->defaultValue != NULL) &&
                (strncasecmp(paramNode->defaultValue, "false", 5)) &&
                (strcmp(paramNode->defaultValue, "0")))
            {
               if (IS_PARAM_WRITABLE(paramNode))
               {
                  ctx->changed = TRUE;
               }
            }
         }
         else
         {
            if ((paramNode->defaultValue == NULL) ||
                (strncasecmp(paramNode->defaultValue, "true", 4) && strcmp(paramNode->defaultValue, "1")))
            {
               if (IS_PARAM_WRITABLE(paramNode))
               {
                  ctx->changed = TRUE;
               }
            }
         }
         break;
      }
     } /* end of switch(paramNode->type) */
   } /* if PRN_NEVER_WRITE_TO_CONFIG_FILE & !dumpAll */

   if (mdmLibCtx.dumpAll || ctx->changed)
   {
      if (ctx->buf != NULL)
      {
         /*
          * We were called by dumpChangedParams.
          */
         wrote = snprintf(&(ctx->buf[ctx->idx]), ctx->max - ctx->idx, "%s<%s",
                          ctx->indentBuf, paramNode->name);
         ctx->idx += (wrote < ctx->max - ctx->idx) ? wrote : ctx->max - ctx->idx;

         /* Is there an attrDesc for this param and is it different from parent obj node? */
         if ((attrDesc != NULL) &&
             (attrDesc->nodeAttr.notification != ctx->nodeAttr->notification))
         {
            wrote = snprintf(&(ctx->buf[ctx->idx]), ctx->max - ctx->idx, " %s=\"%d\"", CONFIG_FILE_ATTR_NOTIFICATION, attrDesc->nodeAttr.notification);
            ctx->idx += (wrote < ctx->max - ctx->idx) ? wrote : ctx->max - ctx->idx;
         }

         if ((attrDesc != NULL) &&
             (attrDesc->nodeAttr.accessBitMask != ctx->nodeAttr->accessBitMask))
         {
            char *accessStr=NULL;

            cmsEid_getStringNamesFromBitMask(attrDesc->nodeAttr.accessBitMask, &accessStr);
            if (strlen(accessStr) > 0)
            {
               wrote = snprintf(&(ctx->buf[ctx->idx]), ctx->max - ctx->idx, " %s=\"%s\"", CONFIG_FILE_ATTR_ACCESS_LIST, accessStr);
               ctx->idx += (wrote < ctx->max - ctx->idx) ? wrote : ctx->max - ctx->idx;
            }
            CMSMEM_FREE_BUF_AND_NULL_PTR(accessStr);
         }

         

         if (mdmLibCtx.dumpAll)
         {
            /*
             * If dumpAll is set, assume we are doing a dumpmdm, so dump the
             * MDM exactly as it is in memory, with no password or xml escape processing.
             */
            if (currStr == NULL)
            {
               wrote = snprintf(&(ctx->buf[ctx->idx]), ctx->max - ctx->idx, ">%s</%s>\n",
                                "", paramNode->name);
            }
            else
            {
               wrote = snprintf(&(ctx->buf[ctx->idx]), ctx->max - ctx->idx, ">%s</%s>\n",
                                currStr, paramNode->name);
            }
         }
         else
         {
            if (paramNode->flags & PRN_CONFIG_PASSWORD)
            {
               char *encryptedStr=NULL;

               /*
                * Do base64 encode of a config password.  base64 encoding does not
                * include XML reserved characters, so we don't have to worry about that.
                */
               cmsB64_encode((unsigned char *) currStr, strlen(currStr)+1, &encryptedStr);

               wrote = snprintf(&(ctx->buf[ctx->idx]), ctx->max - ctx->idx, ">%s</%s>\n",
                                encryptedStr, paramNode->name);

               cmsMem_free(encryptedStr);
            }
            else
            {
               char *escapedString=NULL;

               /*
                * This is the most likely a dump of config to go to the flash.
                * We need to escape XML reserved characters in the string values.
                */

               if (paramNode->type == MPT_STRING &&
                   cmsXml_isEscapeNeeded(currStr))
               {
                  cmsXml_escapeString(currStr, &escapedString);
               }

               if (currStr == NULL) 
               {
                  wrote = snprintf(&(ctx->buf[ctx->idx]), ctx->max - ctx->idx, ">%s</%s>\n",
                                   "", paramNode->name);
               }
               else
               {
                  wrote = snprintf(&(ctx->buf[ctx->idx]), ctx->max - ctx->idx, ">%s</%s>\n",
                                   (escapedString ? escapedString : currStr), paramNode->name);
               }

               /* cmsMem_free can handle NULL input arg */
               cmsMem_free(escapedString);
            }
         }

         ctx->idx += (wrote < ctx->max - ctx->idx) ? wrote : ctx->max - ctx->idx;
      }
      else
      {
         /*
          * we were called by hasAnyParamsOrAttrsChanged.
          * once one hanged parameter is detected, there is no need to keep traversing,
          * so abort traversal.
          */
         doTraverse = FALSE;
      }
   }

   ctx->paramIndex++;

   return doTraverse;
}


/** Return TRUE if any param values or attributes in the specified MdmObject
 *  or its sub-tree has changed from default value or parent attributes.
 */
UBOOL8 hasAnyParamsOrAttrsChanged(const MdmObjectNode *objNode,
                                  const InstanceIdStack *iidStack,
                                  const MdmNodeAttributes *parentAttr)
{
   MdmNodeAttributes *currAttr=NULL;
   struct changed_params_context ctx;
   AttributesDescNode *attrDesc = NULL;
   CmsRet ret;

   memset(&ctx, 0, sizeof(ctx));
   ctx.parentOid = objNode->oid;
   ctx.iidStack = *iidStack;

   ret = mdm_getAllNodeAttributes(objNode, iidStack, &currAttr, &attrDesc);
   if (ret != CMSRET_SUCCESS)
   {
      /* this shouldn't happen */
      cmsLog_error("could not find attribute info for %s %s",
                   mdm_oidToGenericPath(objNode->oid),
                   cmsMdm_dumpIidStack(iidStack));
      cmsAst_assert(0);
      return FALSE;
   }

   /*
    * Only check local params if this node has any params (as indicated by
    * currAttr != NULL).  Otherwise, this is an indirect 2 node with instances
    * below it, don't check this node, see recursion code below.
    */
   if (currAttr != NULL)
   {
      CmsRet ret;

      if ((mdm_compareNodeAttributes(currAttr, parentAttr)) || (attrDesc != NULL))
      {
         /*
          * The attributes on the node is different, or there are
          * individual parameters with different attributes.  No need to
          * check any further, return TRUE now.
          */
         return TRUE;
      }

      ret = mdm_traverseParamNodes(objNode->oid,
                                   iidStack,
                                   NULL,
                                   changedParamsCallbackFunc,
                                   &ctx);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_traverseParamNodes failed, ret=%d", ret);
         cmsAst_assert(0); /* this should never happen */
         return FALSE;
      }

      if (ctx.changed)
      {
         return ctx.changed;
      }
   }


   /* no changed params or attributes, recurse to sub-tree */
   if ((IS_INDIRECT0(objNode)) || (IS_INDIRECT1(objNode)))
   {
      UINT32 i;

      for (i=0; i < objNode->numChildObjNodes && !ctx.changed; i++)
      {
         ctx.changed = hasAnyParamsOrAttrsChanged(&(objNode->childObjNodes[i]), iidStack, currAttr);
      }
   }
   else if (IS_INDIRECT2(objNode) && (DEPTH_OF_IIDSTACK(iidStack) == objNode->instanceDepth))
   {
      /*
       * Always consider indirect 2 nodes as changed because we must always
       * write out the instance number of this node.
       */
      ctx.changed = TRUE;
   }
   else if (IS_INDIRECT2(objNode) && (DEPTH_OF_IIDSTACK(iidStack)+1 == objNode->instanceDepth))
   {
      InstanceHeadNode *instHead;

      instHead = mdm_getInstanceHead(objNode, iidStack);
      if (instHead == NULL)
      {
         cmsLog_error("could not find instHead %s on %s",
                      cmsMdm_dumpIidStack(iidStack),
                      mdm_oidToGenericPath(objNode->oid));
      }
      else
      {
         if (instHead->objData != NULL)
         {
            /*
             * The presence of an instDesc means we need to write out
             * at least the instance number of this object out.  So return TRUE.
             */
            ctx.changed = TRUE;
         }
      }
   }
   else
   {
      cmsLog_error("could not determine action, objNode %s depth=%d iidStack=%s",
                   mdm_oidToGenericPath(objNode->oid),
                   objNode->instanceDepth,
                   cmsMdm_dumpIidStack(iidStack));
      cmsAst_assert(0);
      ctx.changed = FALSE;
   }
   

   return ctx.changed;
}


void dumpChangedParams(const MdmObjectNode *objNode,
                       const InstanceIdStack *iidStack,
                       const MdmNodeAttributes *nodeAttr,
                       const AttributesDescNode *attrDesc,
                       const char *indentBuf,
                       char *buf,
                       UINT32 *idx,
                       UINT32 max)
{
   struct changed_params_context ctx;
   char *indentBuf2;
   UINT32 indentBuf2Len;
   CmsRet ret;

   indentBuf2Len = strlen(indentBuf) + CMS_CONFIG_FILE_INDENT + 1;
   indentBuf2 = cmsMem_alloc(indentBuf2Len, ALLOC_ZEROIZE);

   if (indentBuf2 != NULL) {

      memset(indentBuf2, 0x20, indentBuf2Len - 1); /* fill with spaces */

      ctx.changed = FALSE;
      ctx.buf = buf;
      ctx.indentBuf = indentBuf2;
      ctx.idx = *idx;
      ctx.max = max;
      ctx.parentOid = objNode->oid;
      ctx.iidStack = *iidStack;
      ctx.paramIndex = 0;
      ctx.nodeAttr = nodeAttr;
      ctx.attrDesc = (AttributesDescNode *) attrDesc;

      /*
       * If this is a WANIPConnection object, and its AddressingType is DHCP
       * it ExternalIPAddress, SubnetMask, DefaultGateway, and DNSServers,
       * parameters should not be written out to config file because they
       * will be acquired again when system boots.  We need special code to
       * prevent these parameter values from being written out because the
       * parameters are marked as read-write for the static addressingType case.
       */
      ctx.isDynamicIpoe = FALSE;

      if (objNode->oid == MDMOID_WAN_IP_CONN)
      {
         WanIpConnObject *wanIpConnObj=NULL;

         if ((ret = mdm_getObjectPointer(objNode->oid, iidStack, (void **) &wanIpConnObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to get WAN_IP_CONN object");
         }
         else
         {
            if (!cmsUtl_strcmp(wanIpConnObj->connectionType, MDMVS_IP_ROUTED) &&
                !cmsUtl_strcmp(wanIpConnObj->addressingType, MDMVS_DHCP))
            {
               cmsLog_debug("detected dynamic IPoE object");
               ctx.isDynamicIpoe = TRUE;
            }
         }
      }

      /* same idea as WAN_IP_CONN */
      if (objNode->oid == MDMOID_LAN_IP_INTF)
      {
         LanIpIntfObject *lanIpObj=NULL;

         if ((ret = mdm_getObjectPointer(objNode->oid, iidStack, (void **) &lanIpObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to get LAN_IP_INTF object");
         }
         else
         {
            if (!cmsUtl_strcmp(lanIpObj->IPInterfaceAddressingType, MDMVS_DHCP))
            {
               cmsLog_debug("detected dhcpc on LAN side");
               ctx.isDynamicIpoe = TRUE;
            }
         }
      }


      ret = mdm_traverseParamNodes(objNode->oid,
                                   iidStack,
                                   NULL,
                                   changedParamsCallbackFunc,
                                   &ctx);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_traverseParamNodes failed, ret=%d", ret);
         cmsAst_assert(0); /* this should never happen */
      }

      *idx = ctx.idx;

      CMSMEM_FREE_BUF_AND_NULL_PTR(indentBuf2);
   }

   return;
}





