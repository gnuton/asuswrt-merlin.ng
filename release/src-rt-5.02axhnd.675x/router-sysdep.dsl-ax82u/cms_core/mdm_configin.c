/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2006:proprietary:standard
 * 
 *  This program is the proprietary software of Broadcom and/or its
 *  licensors, and may only be used, duplicated, modified or distributed pursuant
 *  to the terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied), right
 *  to use, or waiver of any kind with respect to the Software, and Broadcom
 *  expressly reserves all rights in and to the Software and all intellectual
 *  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 *  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 *  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 * 
 *  Except as expressly set forth in the Authorized License,
 * 
 *  1. This program, including its structure, sequence and organization,
 *     constitutes the valuable trade secrets of Broadcom, and you shall use
 *     all reasonable efforts to protect the confidentiality thereof, and to
 *     use this information only in connection with your use of Broadcom
 *     integrated circuit products.
 * 
 *  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 *     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 *     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
 *     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
 *     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
 *     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
 *     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
 *     PERFORMANCE OF THE SOFTWARE.
 * 
 *  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
 *     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 *     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
 *     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
 *     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
 *     LIMITED REMEDY.
 * :>
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


static UBOOL8 isAllWhiteSpace(const char *p, UINT32 len);
static CmsRet parseVersionNumber(const char *buf, UINT32 *major, UINT32 *minor);
static UINT32 myMajor=0;
static UINT32 myMinor=0;

/* config write related functions */
static void mdm_tagBeginCallbackFunc(nxml_t handle, const char *tagName, UINT32 len);
static void mdm_attrBeginCallbackFunc(nxml_t handle, const char *attrName, UINT32 len);
static void mdm_attrValueCallbackFunc(nxml_t handle, const char *attrValue, UINT32 len, SINT32 more);
static void mdm_dataCallbackFunc(nxml_t handle, const char *data, UINT32 len, SINT32 more);
static void mdm_tagEndCallbackFunc(nxml_t handle, const char *tagName, UINT32 len);


#ifdef CMS_CONFIG_COMPAT

/* older config file conversion functions */
static void convert_v1_password(void);
static void convert_v1_v2_defaultGateway(void);
static void convert_v3_tunnelObjects(MdmObjectId oid, char *buf);

#endif


CmsRet mdm_loadConfig(void)
{
   char *buf=NULL;
   UINT32 origLen, len;
   CmsRet ret;
   UBOOL8 tryEtcDefaultCfg=TRUE;
   char *selector = CMS_CONFIG_PRIMARY;


   if ((origLen = cmsImg_getConfigFlashSize()) == 0)
   {
      cmsLog_error("cmsImg_getConfigFlashSize returned 0!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* malloc a buffer for holding the config. */
   if ((buf = cmsMem_alloc(origLen, 0)) == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", origLen);
      return CMSRET_RESOURCE_EXCEEDED;
   }


   /*
    * First try to load primary config file from flash.
    */
   len = origLen;
   if (((ret = oal_readConfigFlashToBuf(selector, buf, &len)) == CMSRET_SUCCESS) &&
       (len > 0))
   {
      cmsLog_notice("Got primary config file from flash (len=%d), validating....", len);
      if ((ret = mdm_validateConfigBuf(buf, len)) != CMSRET_SUCCESS)
      {
         /*
          * Validation of the config buffer failed, so do not load it 
          * into the MDM.
          */
         cmsLog_error("primary config file from flash is unrecognized or invalid.");

         /*
          * In case we have a 3.x config file, invalidate the config flash
          * so that later, if user does dumpcfg
          * he does not see the 3.x config file and think that we are using it.
          */
         oal_invalidateConfigFlash(CMS_CONFIG_PRIMARY);
      }
   }

#ifdef SUPPORT_BACKUP_PSI
   if (((ret != CMSRET_SUCCESS) || len == 0) &&
       (cmsImg_isBackupConfigFlashAvailable()))
   {
      cmsLog_notice("trying to read backup config flash");

      len = origLen;
      selector = CMS_CONFIG_BACKUP;
      if (((ret = oal_readConfigFlashToBuf(selector, buf, &len)) == CMSRET_SUCCESS) &&
          (len > 0))
      {
         cmsLog_debug("Got backup config file from flash, len=%d, validating...", len);
         ret = mdm_validateConfigBuf(buf, len);

         /*
          * since backup psi is a new feature, there is no need to check for 3.x
          * config file in backup psi area.
          */

#ifdef SUPPORT_BACKUP_PSI_MIRROR_MODE
         /*
          * Wipe out the invalid backup PSI if we are in Mirror Mode only.
          * Otherwise, the backup psi contains the per-device defaults (SUPPORT_BACKUP_PSI_DEVICE_DEFAULT)
          * which _should_ be valid.  Leave it around for debug purposes.
          */
         if (ret != CMSRET_SUCCESS)
         {
            oal_invalidateConfigFlash(CMS_CONFIG_BACKUP);
         }
#endif
      }
   }
#endif /* SUPPORT_BACKUP_PSI */


   if ((ret == CMSRET_SUCCESS) && (len > 0))
   {
      cmsLog_notice("config file from flash (%s) is valid, len=%d, loading into MDM....",
                    selector, len);
      mdm_loadValidatedConfigBufIntoMdm(buf, len);
      tryEtcDefaultCfg = FALSE;
   }
   else
   {
      cmsLog_notice("no valid config file found in flash");
   }


   /*
    * If we could not get a config file from flash, see if there
    * is a default config file at /etc/default.cfg
    */
   if (tryEtcDefaultCfg)
   {
      char filename[BUFLEN_1024];
      
      sprintf(filename, "/etc/default.cfg");

      /*
       * If we could not load a config file from flash, see if there
       * is a config file at /etc/default.cfg that we can load.
       */
      len = origLen;
      if (((ret = oal_readConfigFileToBuf(filename, buf, &len)) == CMSRET_SUCCESS) &&
          (len > 0))
      {
         cmsLog_notice("Got config file from %s (len=%d), validating....", filename, len);
         if ((ret = mdm_validateConfigBuf(buf, len)) != CMSRET_SUCCESS)
         {
            cmsLog_error("config file from %s is invalid", filename);
         }
         else
         {
            cmsLog_notice("config file from %s is valid, loading into MDM....", filename);
            mdm_loadValidatedConfigBufIntoMdm(buf, len);

            /*
             * Some customers like to see the config file in the flash
             * immediately, so flush the config into the flash now.
             */
            cmsLck_acquireLock();
            cmsMgm_saveConfigToFlash();
            cmsLck_releaseLock();
         }
      }
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(buf);

   /* by the time we get down here, always return success */
   return CMSRET_SUCCESS;
}



CmsRet mdm_validateConfigBuf(const char *buf, UINT32 len)
{
   nxml_t nxmlHandle;
   nxml_settings nxmlSettings;
   char *endp = NULL;
   SINT32 rc;
   UINT32 maxLen;
   CmsRet ret=CMSRET_SUCCESS;

   /*
    * When reading from configflash that has been zeroized,
    * we get length of 1.  Way too short to hold valid config file.
    */
   if (buf == NULL || len <= 1)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }
   

   maxLen = cmsImg_getConfigFlashSize();
   if (len > maxLen)
   {
      cmsLog_error("config buf length %d is greater than max %d", len, maxLen);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* initialize our callback context state */
   memset(&nxmlCtx, 0, sizeof(nxmlCtx));
   nxmlCtx.loadMdm = FALSE;
   nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_NONE;
   nxmlCtx.ret = CMSRET_SUCCESS;   

   /* set callback function handlers */
   nxmlSettings.tag_begin = mdm_tagBeginCallbackFunc;
   nxmlSettings.attribute_begin = mdm_attrBeginCallbackFunc;
   nxmlSettings.attribute_value = mdm_attrValueCallbackFunc;
   nxmlSettings.data = mdm_dataCallbackFunc;
   nxmlSettings.tag_end = mdm_tagEndCallbackFunc;

   if ((rc = xmlOpen(&nxmlHandle, &nxmlSettings)) == 0)
   {
      cmsLog_error("xmlOpen failed");
      return CMSRET_INTERNAL_ERROR;
   }

   /* push our data into the nanoxml parser, it will then call our callback funcs. */
   rc = xmlWrite(nxmlHandle, (char *)buf, len, &endp);


   /* check for error conditions. */
   if (rc == 0)
   {
      cmsLog_error("nanoxml parser returned error.");
      ret = CMSRET_INVALID_ARGUMENTS;
   }
   else if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("nxmlCtx.ret = %d", nxmlCtx.ret);
      ret = nxmlCtx.ret;
   }   
   else if (!isAllWhiteSpace(endp, buf+len-endp))
   {
      /* nanoxml endp points at one past the last processed char */
      cmsLog_error("not all data processed, buf %p (len 0x%x) "
                   "endp at %p, buf+len-endp=%d",
                   buf, len, endp, buf+len-endp);
      ret = CMSRET_INVALID_ARGUMENTS;
   }
   else if (!nxmlCtx.topNodeFound || !nxmlCtx.versionFound)
   {
      cmsLog_error("did not find top node and/or version");
      ret = nxmlCtx.ret;
   }

   xmlClose(nxmlHandle);

   return ret;
}


void mdm_loadValidatedConfigBufIntoMdm(const char *buf, UINT32 len)
{
   nxml_t nxmlHandle;
   nxml_settings nxmlSettings;
   char *endp = NULL;
   SINT32 rc;


   /* initialize our callback context state */
   memset(&nxmlCtx, 0, sizeof(nxmlCtx));
   nxmlCtx.loadMdm = TRUE;
   nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_NONE;
   nxmlCtx.ret = CMSRET_SUCCESS;

   /* set callback function handlers */
   nxmlSettings.tag_begin = mdm_tagBeginCallbackFunc;
   nxmlSettings.attribute_begin = mdm_attrBeginCallbackFunc;
   nxmlSettings.attribute_value = mdm_attrValueCallbackFunc;
   nxmlSettings.data = mdm_dataCallbackFunc;
   nxmlSettings.tag_end = mdm_tagEndCallbackFunc;

   if ((rc = xmlOpen(&nxmlHandle, &nxmlSettings)) == 0)
   {
      cmsLog_error("xmlOpen failed");
      return;
   }

   /* push our data into the nanoxml parser, it will then call our callback funcs. */
   rc = xmlWrite(nxmlHandle, (char *)buf, len, &endp);


   /* check for error conditions. */
   if (rc == 0)
   {
      cmsLog_error("nanoxml parser returned error.");
   }
   else if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      cmsLog_error("parsing returned error %d", nxmlCtx.ret);
   }
   else if (!isAllWhiteSpace(endp, buf+len-endp))
   {
      /* nanoxml endp seems to point at the last processed char */
      cmsLog_error("not all data processed, buf %p (len 0x%x) "
                   "endp at %p, expected endp %p",
                   buf, len, endp, buf+len-2);
   }

   else if (!nxmlCtx.topNodeFound || !nxmlCtx.versionFound)
   {
      cmsLog_error("did not find top node and/or version");
   }

   xmlClose(nxmlHandle);

   return;
}


void mdm_tagBeginCallbackFunc(nxml_t handle __attribute__((unused)),
                              const char *tagName,
                              UINT32 len)
{
   char buf[NXML_MAX_NAME_SIZE+1];
   MdmObjectNode *objNode;
   MdmParamNode *paramNode;
   CmsRet ret;
   MdmObjectNode *compatChildObjNode=NULL;

   if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      /* don't traverse any more if error is detected. */
      return;
   }

   strncpy(buf, tagName, len);
   buf[len]='\0';

#ifdef CMS_CONFIG_IGNORE_UNRECOGNIZED
   if (nxmlCtx.ignoreTag)
   {
      printf("Ignoring unrecognized param/obj tag %s inside %s\n",
                   buf, nxmlCtx.ignoreTag);
      return;
   }
#endif

#ifdef CMS_CONFIG_COMPAT
   /************** Begin convert v1,v2 config file ****************/

   /*
    * In v1, v2, the Time object had X_BROADCOM_COM_NTPEnable.
    * In v3, we use the BBF defined Enable parameter.
    */
   if ((nxmlCtx.versionMajor < 3) &&
       (nxmlCtx.objNode != NULL) && (MDMOID_TIME_SERVER_CFG == nxmlCtx.objNode->oid) &&
       (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_NTPEnable")))
   {
      snprintf(buf, len+1, "Enable");
      cmsLog_debug("converted X_BROADCOM_COM_NTPEnable to %s", buf);
   }

   /* 
    * In v2 and v2, there was a WanPPPConnection.X_BROADCOM_COM_BcastAddr.
    * In v3, we deleted that parameter.  So just pretend the current param
    * is X_BROADCOM_COM_IfName so that we can have a paramNode.  We won't do
    * anything to the X_BROADCOM_COM_IfName param though, we just need to
    * point to a similar string type param node so that later processing will
    * not get confused.
    */
   if ((nxmlCtx.versionMajor < 3) &&
       (nxmlCtx.objNode != NULL) && (MDMOID_WAN_PPP_CONN == nxmlCtx.objNode->oid) &&
       (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_BcastAddr")))
   {
      cmsLog_debug("fake X_BROADCOM_COM_BcastAddr to X_BROADCOM_COM_IfName");
      snprintf(buf, len+1, "X_BROADCOM_COM_IfName");
   }

   /*
    * In December 2011, 4in6Tunnel and 6in4Tunnel were
    * renamed to V4inV6Tunnel and V6inV4Tunnel, respectively.
    */
   if ((nxmlCtx.versionMajor < 4) && (nxmlCtx.objNode != NULL))
   {
      convert_v3_tunnelObjects(nxmlCtx.objNode->oid, buf);
   }

   /*
    * In 4.14, X_BROADCOM_COM_LineSetting was renamed to
    * X_BROADCOM_COM_DectLineSetting
    */
   if ((nxmlCtx.objNode != NULL) && (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_LineSetting")))
   {
      snprintf(buf, NXML_MAX_NAME_SIZE, "X_BROADCOM_COM_DectLineSetting");
      cmsLog_debug("converted X_BROADCOM_COM_LineSetting to %s", buf);
   }

   /*
    * In 4.14L.04 (Oct 2013), X_BROADCOM_COM_MLDSnoopingConfig was moved
    * up two levels.  It is now a child of LANDevice.{i}.
    */
   if (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_MldSnoopingConfig"))
   {
      cmsLog_debug("detected MldSnoopingObject, currObj->name %s",
                   nxmlCtx.objNode->name);

      if (nxmlCtx.objNode->oid == MDMOID_LAN_DEV ||
          nxmlCtx.objNode->oid == MDMOID_DEV2_BRIDGE)
      {
         cmsLog_debug("MldSnoopingConfig already in correct place, do nothing");
      }
      else
      {
         MdmObjectNode *parentObjNode = nxmlCtx.objNode->parent;
         UINT32 count=0;
         UBOOL8 found=FALSE;

         while (!found && parentObjNode && count < 3)
         {
            cmsLog_debug("compat: searching for LAN_DEV: curr %d, count=%d",
                          parentObjNode->oid, count);
            if (parentObjNode->oid == MDMOID_LAN_DEV)
            {
               found = TRUE;
               compatChildObjNode = mdm_getChildObjectNode(parentObjNode, buf);
            }
            else
            {
               parentObjNode = parentObjNode->parent;
               count++;
            }
         }
      }
   }

   /**************** end config file conversion ****************/
#endif


   if (nxmlCtx.objNode == NULL)
   {
      /*
       * This is very early in the config file.  It must start with the
       * CpeConfigFile node followed by the InternetGatewayDevice node.
       */
      if (!strcmp(buf, CONFIG_FILE_TOP_NODE))
      {
         if (nxmlCtx.topNodeFound)
         {
            cmsLog_error("multiple %s nodes detected", CONFIG_FILE_TOP_NODE);
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            cmsLog_debug("%s node detected", CONFIG_FILE_TOP_NODE);
            nxmlCtx.topNodeFound = TRUE;
         }
      }
      else if (!strcmp(buf, mdmShmCtx->rootObjNode->name))
      {
         cmsLog_debug("%s node detected", mdmShmCtx->rootObjNode->name);
         nxmlCtx.objNode = mdmShmCtx->rootObjNode;

         ret = mdm_getDefaultObject(nxmlCtx.objNode->oid, &(nxmlCtx.mdmObj));
         if (ret != CMSRET_SUCCESS)
         {
            nxmlCtx.ret = ret;
         }
      }
      else if (!strcmp(buf, CONFIG_FILE_PSI_TOP_NODE))
      {
         cmsLog_notice("PSI top node detected");
         nxmlCtx.ret = CMSRET_CONFIG_PSI;
      }         
      else
      {
         cmsLog_error("Invalid start node %s", buf);
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }

      return;
   }


   /*
    * a tag can start a child object node, a peer object node,
    * or a parameter node of the current object node.
    */
   if (((objNode = mdm_getChildObjectNode(nxmlCtx.objNode, buf)) != NULL) ||
       ((nxmlCtx.gotCurrObjEndTag == TRUE) &&
        (nxmlCtx.objNode->parent != NULL) &&
        ((objNode = mdm_getChildObjectNode(nxmlCtx.objNode->parent, buf)) != NULL)) ||
       (compatChildObjNode != NULL))
   {

      if (nxmlCtx.objNode == objNode->parent)
      {
         cmsLog_debug("%s --> child obj %s", nxmlCtx.objNode->name, objNode->name);
      }
      else
      {
         cmsLog_debug("%s --> peer obj %s", nxmlCtx.objNode->name, objNode->name);
      }

      nxmlCtx.gotCurrObjEndTag = FALSE;

      if (nxmlCtx.loadMdm)
      {
         /*
          * Since we are transitioning away from the current object, 
          * set any attributes of the current objNode sub-tree that we detected.
          */
         if (nxmlCtx.attr.accessBitMaskChange || nxmlCtx.attr.notificationChange)
         {
            cmsLog_debug("set sub-tree attr notif=(%d)%d access=(%d)0x%x",
                         nxmlCtx.attr.notificationChange,
                         nxmlCtx.attr.notification,
                         nxmlCtx.attr.accessBitMaskChange,
                         nxmlCtx.attr.accessBitMask);

            ret = mdm_setSubTreeParamAttributes(nxmlCtx.objNode,
                                                &(nxmlCtx.iidStack),
                                                &(nxmlCtx.attr),
                                                FALSE);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("setSubTree attr failed for %s %s notification=(%d)%d accessBitMask=(%d)0x%x ret=%d",
                            nxmlCtx.objNode->name,
                            cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)),
                            nxmlCtx.attr.notificationChange,
                            nxmlCtx.attr.notification,
                            nxmlCtx.attr.accessBitMaskChange,
                            nxmlCtx.attr.accessBitMask,
                            ret);
               nxmlCtx.ret = ret;
            }

            nxmlCtx.attr.accessBitMaskChange = 0;
            nxmlCtx.attr.notificationChange = 0;
         }


         /*
          * Push new object into MDM.  We could optimize this code to detect
          * if any changes have actually been made to the mdmObj so we
          * don't end up pushing the exact same object into the MDM.
          *
          * Every new value we've put into the mdmObj has been validated
          * in the dataCallbackFunc, so there is no need to validate the
          * entire mdmObj here.
          *
          * This one catches the case where we are transitioning further down
          * into the MDM hierarchy.
          */
         if (nxmlCtx.mdmObj != NULL && nxmlCtx.nextInstanceNode == FALSE)
         {
            cmsLog_debug("setting obj %s %s",
                         mdm_oidToGenericPath(*((MdmObjectId *) nxmlCtx.mdmObj)),
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
            ret = mdm_setObject(&(nxmlCtx.mdmObj), &(nxmlCtx.iidStack), FALSE);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("setObject failed, %d", ret);
               mdm_freeObject(&(nxmlCtx.mdmObj));
               nxmlCtx.ret = ret;
            }
         }
      }
      else
      {
         /* not loading mdm, just free the mdmObj */
         if (nxmlCtx.mdmObj)
         {
            mdm_freeObject(&(nxmlCtx.mdmObj));
         }
      }


      nxmlCtx.nextInstanceNode = FALSE;

#ifdef CMS_CONFIG_COMPAT
      if (compatChildObjNode != NULL)
      {
         objNode = compatChildObjNode;
      }
#endif

      /* record the new object node that we are working on. */
      nxmlCtx.objNode = objNode;


      /*
       * node's attributes are inheritied from the parent (I don't think I
       * need this line below.  Any changes to attributes should have
       * been written out in the config file.)
       */
      nxmlCtx.attr = objNode->parent->nodeAttr;


      /* start a new MdmObject */
      ret = mdm_getDefaultObject(nxmlCtx.objNode->oid, &(nxmlCtx.mdmObj));
      if (ret != CMSRET_SUCCESS)
      {
         nxmlCtx.ret = ret;
      }
   }
   else if ((paramNode = mdm_getParamNode(nxmlCtx.objNode->oid, buf)) != NULL)
   {
      cmsLog_debug("%s :: param %s", nxmlCtx.objNode->name, paramNode->name);

      if (nxmlCtx.nextInstanceNode)
      {
         cmsLog_error("param node %s detected under a next instance object", paramNode->name);
         nxmlCtx.paramNode = paramNode;
         return;
      }

      /*
       * We could be transitioning from obj to param, if any attribute
       * has changed, set them for the current objNode sub-tree.
       * The fourth parameter of setSubTreeParamAttributes is testOnly,
       * so we pass in !loadMdm, which which means when loadMdm=FALSE, testOnly=TRUE.
       */
      if ((nxmlCtx.loadMdm) &&
          (nxmlCtx.attr.accessBitMaskChange || nxmlCtx.attr.notificationChange))
      {
         cmsLog_debug("set sub-tree attr notif=(%d)%d access=(%d)0x%x",
                      nxmlCtx.attr.notificationChange,
                      nxmlCtx.attr.notification,
                      nxmlCtx.attr.accessBitMaskChange,
                      nxmlCtx.attr.accessBitMask);

         ret = mdm_setSubTreeParamAttributes(nxmlCtx.objNode,
                                             &(nxmlCtx.iidStack),
                                             &(nxmlCtx.attr),
                                             FALSE);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("setSubTree attr failed for %s %s notification=(%d)%d accessBitMask=(%d)0x%x ret=%d",
                         nxmlCtx.objNode->name,
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)),
                         nxmlCtx.attr.notificationChange,
                         nxmlCtx.attr.notification,
                         nxmlCtx.attr.accessBitMaskChange,
                         nxmlCtx.attr.accessBitMask,
                         ret);
            nxmlCtx.ret = ret;
         }

         nxmlCtx.attr.accessBitMaskChange = 0;
         nxmlCtx.attr.notificationChange = 0;
      }


      if (nxmlCtx.paramNode != NULL)
      {
         cmsLog_error("embedded param node detected %s %s",
                      nxmlCtx.paramNode->name, paramNode->name);
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }
      else
      {
         nxmlCtx.paramNode = paramNode;
      }
   }
   else
   {
#ifdef CMS_CONFIG_IGNORE_UNRECOGNIZED
      nxmlCtx.ignoreTag = cmsMem_strdup(buf);
      printf("Ignoring unrecognized param/obj tag %s\n", nxmlCtx.ignoreTag);
#else
      cmsLog_error("Unrecognized tag %s", buf);
      nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
#endif
   }

   return;
}

static void mdm_attrBeginCallbackFunc(nxml_t handle __attribute__((unused)),
                                      const char *attrName,
                                      UINT32 len)
{
   char buf[NXML_MAX_NAME_SIZE+1];

   if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      /* don't traverse any more if error has been detected. */
      return;
   }
   
   strncpy(buf, attrName, len);
   buf[len]='\0';

   cmsLog_debug("%s", buf);

#ifdef CMS_CONFIG_IGNORE_UNRECOGNIZED
   if (nxmlCtx.ignoreTag)
   {
      cmsLog_notice("Ignoring attrTag %s (ignoreTag=%s)",
                   buf, nxmlCtx.ignoreTag);
      return;
   }
#endif

   if (nxmlCtx.currXmlAttr != MDM_CONFIG_XMLATTR_NONE)
   {
      cmsLog_error("overlapping attrs, currXmlAttr=%d", nxmlCtx.currXmlAttr);
      nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      return;
   }

   if (!strcmp(CONFIG_FILE_ATTR_VERSION, buf))
   {
      nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_VERSION;
   }
   else if (!strcmp(CONFIG_FILE_ATTR_INSTANCE, buf))
   {
      nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_INSTANCE;
   }
   else if (!strcmp(CONFIG_FILE_ATTR_NEXT_INSTANCE, buf))
   {
      nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_NEXT_INSTANCE;
   }
   else if (!strcmp(CONFIG_FILE_ATTR_ACCESS_LIST, buf))
   {
      nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_ACCESS_LIST;
   }
   else if (!strcmp(CONFIG_FILE_ATTR_NOTIFICATION, buf))
   {
      nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_NOTIFICATION;
   }
   else
   {
      cmsLog_error("Unrecognized attribute %s", buf);
      nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
   }

   return;
}


static void mdm_attrValueCallbackFunc(nxml_t handle __attribute__((unused)),
                                      const char *attrValue,
                                      UINT32 len,
                                      SINT32 more)
{
   char *buf;

   if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      /* don't traverse any more if error has been detected. */
      return;
   }

   /*
    * our attribute values are always written out in a single line,
    * so more should always be 0.
    */
   if (more != 0)
   {
      cmsLog_error("multi-line attribute value detected");
      nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      return;
   }

#ifdef CMS_CONFIG_IGNORE_UNRECOGNIZED
   if (nxmlCtx.ignoreTag)
   {
      cmsLog_notice("Ignoring attrValue (ignoreTag=%s)", nxmlCtx.ignoreTag);
      return;
   }
#endif

   if ((buf = cmsMem_alloc(len+1, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", len+1);
      nxmlCtx.ret = CMSRET_RESOURCE_EXCEEDED;
      return;
   }

   strncpy(buf, attrValue, len);
   buf[len]='\0';

   cmsLog_debug("(more=%d)%s", more, buf);

   switch(nxmlCtx.currXmlAttr)
   {
   case MDM_CONFIG_XMLATTR_VERSION:
      {
         /*
          * Config file version may help us with conversion later on.
          * For now, just print it out.
          */
         if (nxmlCtx.objNode != NULL)
         {
            cmsLog_error("version attr can only appear in the top node");
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            cmsLog_debug("config file version=%s", buf);
            nxmlCtx.ret = parseVersionNumber(buf, &nxmlCtx.versionMajor, &nxmlCtx.versionMinor);

            if (nxmlCtx.ret == CMSRET_SUCCESS)
            {
               nxmlCtx.versionFound = TRUE;

               parseVersionNumber(CMS_CONFIG_FILE_VERSION, &myMajor, &myMinor);

#ifdef CMS_CONFIG_COMPAT
               /*
                * If CMS_CONFIG_COMPAT is defined, then we can accept any version
                * of the config file that is less than or equal to my current version.
                */
               if (nxmlCtx.versionMajor > myMajor)
               {
                  cmsLog_error("config file version number is %d, this software only supports up to %d",
                               nxmlCtx.versionMajor, myMajor);
                  nxmlCtx.ret = CMSRET_INVALID_CONFIG_FILE;
               }
#else
               /*
                * If CMS_CONFIG_COMPAT is not defined, then we can only accept
                * config files that have exactly the same version as my current version.
                */
               if (nxmlCtx.versionMajor != myMajor)
               {
                  cmsLog_error("config file version number is %d, this software only supports %d",
                               nxmlCtx.versionMajor, myMajor);
                  cmsLog_error("You may need to enable backward compatibility of CMS Config files in make menuconfig");
                  nxmlCtx.ret = CMSRET_INVALID_CONFIG_FILE;
               }
#endif  /* CMS_CONFIG_COMPAT */
            }
         }
         break;
      }

   case MDM_CONFIG_XMLATTR_INSTANCE:
      {
         UINT32 instanceId;
         CmsRet ret;

         if ((ret = cmsUtl_strtoul(buf, NULL, 0, &instanceId)) != CMSRET_SUCCESS)
         {
            cmsLog_error("invalid instance number %s", buf);
            nxmlCtx.ret = ret;
         }
         else if (!(IS_INDIRECT2(nxmlCtx.objNode)))
         {
            cmsLog_error("got instance number on non-indirect2 node, %s",
                         nxmlCtx.objNode->name);
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else if (DEPTH_OF_IIDSTACK(&(nxmlCtx.iidStack)) + 1 != nxmlCtx.objNode->instanceDepth)
         {
            cmsLog_error("instance depth mismatch on %s, instance depth=%d iidStack %s",
                         nxmlCtx.objNode->name,
                         nxmlCtx.objNode->instanceDepth,
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            MdmNodeAttributes parentAttr;

            /*
             * Get the parent node's attributes.  The newly created sub-tree
             * will inherit these attributes.  But be careful, the location
             * of the parent's attributes depend on the type of node it is.
             */
            if (nxmlCtx.loadMdm)
            {
               if (IS_INDIRECT0(nxmlCtx.objNode->parent))
               {
                  parentAttr = nxmlCtx.objNode->parent->nodeAttr;
               }
               else if (IS_INDIRECT1(nxmlCtx.objNode->parent))
               {
                  InstanceHeadNode *instHead;

                  instHead = mdm_getInstanceHead(nxmlCtx.objNode->parent, &(nxmlCtx.iidStack));
                  if (instHead == NULL)
                  {
                     cmsLog_error("could not find instHead for %s %s",
                                  nxmlCtx.objNode->parent,
                                  cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
                     nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
                  }
                  else
                  {
                     parentAttr = instHead->nodeAttr;
                  }
               }
               else
               {
                  /* must be indirect 2 */
                  InstanceDescNode *instDesc;

                  instDesc = mdm_getInstanceDescFromObjNode(nxmlCtx.objNode->parent,
                                                            &(nxmlCtx.iidStack));
                  if (instDesc == NULL)
                  {
                     cmsLog_error("could not find instDesc for %s %s",
                                  nxmlCtx.objNode->parent->name,
                                  cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
                     nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
                  }
                  else
                  {
                     parentAttr = instDesc->nodeAttr;
                  }
               }
            }
                  

            /* create the instance in the MDM */
            PUSH_INSTANCE_ID(&(nxmlCtx.iidStack), instanceId);
            cmsLog_debug("Got instanceId for indirect 2 node %s %s",
                         mdm_oidToGenericPath(nxmlCtx.objNode->oid),
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));

            if (nxmlCtx.loadMdm)
            {
               ret = mdm_createSubTree(nxmlCtx.objNode,
                                       nxmlCtx.objNode->instanceDepth,
                                       &(nxmlCtx.iidStack),
                                       NULL, NULL);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("create subtree for obj %s %s failed, ret=%d",
                               nxmlCtx.objNode->name,
                               cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)),
                               ret);
                  nxmlCtx.ret = ret;
               }
               else
               {
                  parentAttr.notificationChange = 1;
                  parentAttr.accessBitMaskChange = 1;

                  cmsLog_debug("set sub-tree attr notif=(%d)%d access=(%d)0x%x",
                               parentAttr.notificationChange,
                               parentAttr.notification,
                               parentAttr.accessBitMaskChange,
                               parentAttr.accessBitMask);


                  ret = mdm_setSubTreeParamAttributes(nxmlCtx.objNode,
                                                      &(nxmlCtx.iidStack),
                                                      &parentAttr,
                                                      FALSE);
                  if (ret != CMSRET_SUCCESS)
                  {
                     cmsLog_error("setSubTree attr failed for %s %s notification=(%d)%d accessBitMask=(%d)0x%x, ret=%d",
                                  nxmlCtx.objNode->name,
                                  cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)),
                                  parentAttr.notificationChange,
                                  parentAttr.notification,
                                  parentAttr.accessBitMaskChange,
                                  parentAttr.accessBitMask,
                                  ret);
                     nxmlCtx.ret = ret;
                  }
               }

            }
         }

         break;
      }

   case MDM_CONFIG_XMLATTR_NEXT_INSTANCE:
      {
         UINT32 nextInstanceIdToAssign;
         CmsRet ret;

         if ((ret = cmsUtl_strtoul(buf, NULL, 0, &nextInstanceIdToAssign)) != CMSRET_SUCCESS)
         {
            cmsLog_error("invalid next instance number %s", buf);
            nxmlCtx.ret = ret;
         }
         else if (!(IS_INDIRECT2(nxmlCtx.objNode)))
         {
            cmsLog_error("got next instance number on non-indirect2 node, %s",
                         nxmlCtx.objNode->name);
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else if (DEPTH_OF_IIDSTACK(&(nxmlCtx.iidStack)) + 1 != nxmlCtx.objNode->instanceDepth)
         {
            cmsLog_error("instance depth mismatch on %s, instance depth=%d iidStack %s",
                         nxmlCtx.objNode->name,
                         nxmlCtx.objNode->instanceDepth,
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {

            nxmlCtx.nextInstanceNode = TRUE;

            /*
             * Just set the nextInstanceId here.  We know there are no other 
             * attributes or parameters that we need to collect for this object.
             */
            if (nxmlCtx.loadMdm)
            {
               InstanceHeadNode *instHead;

               cmsLog_debug("setting nextInstanceId to %u for objNode %s iidStack=%s",
                            nextInstanceIdToAssign, nxmlCtx.objNode->name, cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));

               if ((instHead = mdm_getInstanceHead(nxmlCtx.objNode, &(nxmlCtx.iidStack))) == NULL)
               {
                  cmsLog_error("could not find instHead for %s %s", nxmlCtx.objNode->name, cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
               }
               else
               {
                  instHead->nextInstanceIdToAssign = nextInstanceIdToAssign;  
               }
            }

            /* 
             * Even though we don't need to do anything with the iidStack here,
             * push an instance id on the iidStack now because when we hit the
             * end tag, we will pop it off.
             */
            PUSH_INSTANCE_ID(&(nxmlCtx.iidStack), 1);
            cmsLog_debug("pushed fake iidStack, now is %s", cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
         }

         break;
      }


   case MDM_CONFIG_XMLATTR_ACCESS_LIST:
      {
         UINT16 accessBitMask=0;
         CmsRet ret;

         ret = cmsEid_getBitMaskFromStringNames(buf, &accessBitMask);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("conversion of %s to accessBitMask failed, ret=%d",
                         buf, ret);
            nxmlCtx.ret = ret;
         }
         else
         {
            nxmlCtx.attr.accessBitMaskChange = 1;
            nxmlCtx.attr.accessBitMask = accessBitMask;
         }

         /* we need to do more when we are acutally pushing data into MDM */
         break;
      }

   case MDM_CONFIG_XMLATTR_NOTIFICATION:
      {
         UINT32 notification;
         CmsRet ret;

         ret = cmsUtl_strtoul(buf, NULL, 0, &notification);
         if ((ret != CMSRET_SUCCESS) ||
             ((notification != NDA_TR69_NO_NOTIFICATION) && 
              (notification != NDA_TR69_PASSIVE_NOTIFICATION) && 
              (notification != NDA_TR69_ACTIVE_NOTIFICATION)))
         {
            cmsLog_error("invalid notification number %s", buf);
            nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            nxmlCtx.attr.notificationChange = 1;
            nxmlCtx.attr.notification = notification;
         }

         break;
      }

   case MDM_CONFIG_XMLATTR_NONE:
      {
         cmsLog_error("got attr value with no attr tag in progress.");
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
         break;
      }

   default:
      {
         cmsLog_error("Unrecognized attribute %d", nxmlCtx.currXmlAttr);
         nxmlCtx.ret = CMSRET_INTERNAL_ERROR;
         break;
      }

   } /* end of switch(nxmlCtx.currXmlAttr) */


   /* we are done with attr processing */
   nxmlCtx.currXmlAttr = MDM_CONFIG_XMLATTR_NONE;

   CMSMEM_FREE_BUF_AND_NULL_PTR(buf);
}




static void mdm_dataCallbackFunc(nxml_t handle __attribute__((unused)),
                                 const char *data,
                                 UINT32 len,
                                 SINT32 more)
{

   if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      /* don't traverse any more if error is detected. */
      return;
   }

   /* 
    * I don't know if we will ever have data that spans multiple
    * lines.  Don't implement this feature for now.  So this code
    * will only use the last line that was detected and drop
    * all previous lines.
    */
   if (more != 0)
   {
      cmsLog_error("support for multiple data lines not implemented yet.");
      nxmlCtx.ret = CMSRET_INTERNAL_ERROR;
      return;
   }

   if (nxmlCtx.paramValue != NULL)
   {
      cmsLog_error("multiple data callbacks detected, paramValue=%s", nxmlCtx.paramValue);
      nxmlCtx.ret = CMSRET_INTERNAL_ERROR;
      return;
   }


#ifdef CMS_CONFIG_IGNORE_UNRECOGNIZED
   if (nxmlCtx.ignoreTag)
   {
      /* Since we are ignoring the tag, ignore the data also. */
      cmsLog_notice("ignoreTag=%s, ignoring data", nxmlCtx.ignoreTag);
      return;
   }
#endif

   /*
    * we are allocating data in heap memory.  If this data is for
    * a string param, mdm_setParamNodeString will make a copy in shared
    * memory and set that in the mdmObj.  If we have very large string
    * objects, we could optimize this by allocating from shared memory
    * here and let mdm_setParamNodeString steal our buffer.  For small string
    * value buffers, its no big deal.
    */
   if ((nxmlCtx.paramValue = cmsMem_alloc(len+1, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", len+1);
   }
   else
   {
      strncpy(nxmlCtx.paramValue, data, len);
      nxmlCtx.paramValue[len]='\0';

      cmsLog_debug("(more=%d)%s", more, nxmlCtx.paramValue);

      /*
       * For string params, we need to unescape any XML character escape
       * sequences before sending to MDM.
       */
      if (nxmlCtx.paramNode->type == MPT_STRING &&
          cmsXml_isUnescapeNeeded(nxmlCtx.paramValue))
      {
         char *unescapedString=NULL;

         cmsXml_unescapeString(nxmlCtx.paramValue, &unescapedString);

         cmsMem_free(nxmlCtx.paramValue);
         nxmlCtx.paramValue = unescapedString;
      }
   }

   return;
}


void mdm_tagEndCallbackFunc(nxml_t handle __attribute__((unused)),
                            const char *tagName,
                            UINT32 len)
{
   UBOOL8 compatNormalPop = TRUE;
   char buf[NXML_MAX_NAME_SIZE+1];

   if (nxmlCtx.ret != CMSRET_SUCCESS)
   {
      /* don't traverse any more if error is detected. */
      return;
   }

   strncpy(buf, tagName, len);
   buf[len]='\0';

#ifdef CMS_CONFIG_IGNORE_UNRECOGNIZED
   if (nxmlCtx.ignoreTag)
   {
      if (!cmsUtl_strcmp(nxmlCtx.ignoreTag, buf))
      {
         /* found matching end tag.  Go back to normal parsing state. */
         cmsLog_notice("Hit matching ignoreTag %s", buf);
         CMSMEM_FREE_BUF_AND_NULL_PTR(nxmlCtx.ignoreTag);
      }
      else
      {
         /* found matching end tag of the current tag, but we are still
          * inside an ignored object. */
         cmsLog_notice("Ignoring End Tag %s (ignoreTag=%s)",
                       buf, nxmlCtx.ignoreTag);
      }
      return;
   }
#endif

   if (nxmlCtx.paramNode != NULL)
   {
      /*
       * This is the end tag of a param.  The value of the param is in the
       * paramValue field.  Update my mdmObj.
       */

#ifdef CMS_CONFIG_COMPAT
      /************** Begin convert v1,v2 config file ****************/

      /*
       * In v1, v2, the Time object had X_BROADCOM_COM_NTPEnable.
       * In v3, we use the BBF defined Enable parameter.
       */
      if ((nxmlCtx.versionMajor < 3) &&
          (nxmlCtx.objNode != NULL) && (MDMOID_TIME_SERVER_CFG == nxmlCtx.objNode->oid) &&
          (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_NTPEnable")))
      {
         snprintf(buf, len+1, "Enable");
         cmsLog_debug("converted X_BROADCOM_COM_NTPEnable to %s", buf);
      }

      /* 
       * In v2 and v2, there was a WanPPPConnection.X_BROADCOM_COM_BcastAddr.
       * In v3, we deleted that parameter, so just ignore it and return here.
       */
      if ((nxmlCtx.versionMajor < 3) &&
          (nxmlCtx.objNode != NULL) && (MDMOID_WAN_PPP_CONN == nxmlCtx.objNode->oid) &&
          (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_BcastAddr")))
      {
         cmsLog_debug("ignore X_BROADCOM_COM_BcastAddr in WanPppConn");
         CMSMEM_FREE_BUF_AND_NULL_PTR(nxmlCtx.paramValue);
         nxmlCtx.paramNode = NULL;
         return;
      }

   /**************** end config file conversion ****************/
#endif

      if (strcmp(nxmlCtx.paramNode->name, buf))
      {
         cmsLog_error("unexpected end tag, expected param name %s got %s",
                      nxmlCtx.paramNode->name, buf);
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }
      else if (nxmlCtx.mdmObj == NULL)
      {
         cmsLog_error("no mdmObj but end param tag.");
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }
      else if ((nxmlCtx.paramValue == NULL) &&
               ((nxmlCtx.paramNode->type != MPT_STRING)  &&
                (nxmlCtx.paramNode->type != MPT_DATE_TIME) &&
                (nxmlCtx.paramNode->type != MPT_BASE64) &&
                (nxmlCtx.paramNode->type != MPT_UUID) &&
                (nxmlCtx.paramNode->type != MPT_IP_ADDR) &&
                (nxmlCtx.paramNode->type != MPT_MAC_ADDR) &&
                (nxmlCtx.paramNode->type != MPT_HEX_BINARY)))
      {
         /* non-string types must have a value */
         cmsLog_error("no param value but end param tag.");
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }
      else
      {
         CmsRet r1=CMSRET_SUCCESS;
         CmsRet r2=CMSRET_SUCCESS;

#ifdef CMS_CONFIG_COMPAT
         /************* Begin convert v1 config file ****************/
         if ((nxmlCtx.versionMajor == 1) &&
             (!cmsUtl_strcmp(nxmlCtx.paramNode->name, "AdminPassword") ||
              !cmsUtl_strcmp(nxmlCtx.paramNode->name, "SupportPassword") ||
              !cmsUtl_strcmp(nxmlCtx.paramNode->name, "UserPassword") ||
              (!cmsUtl_strcmp(nxmlCtx.paramNode->name, "Password") && !cmsUtl_strcmp(nxmlCtx.objNode->name, "WANPPPConnection"))))
         {
            cmsLog_notice("v1 password detected, paramNode->name=%s", nxmlCtx.paramNode->name);
            convert_v1_password();
         }
         
         /************* End convert v1 config file ****************/
         /*
          * Prior to 4.16L.01, the LinkEncapsulationUsed param was incorrectly
          * set to MDMVS_G_992_3_ANNEX_K_PTM (PTM over ADSL).  Starting with
          * 4.16L.01, all CMS code has been changed to use
          * MDMVS_G_993_2_ANNEX_K_PTM (PTM over VDSL).  If we detect a config
          * file with the old incorrect setting, just convert it.
          * Since the old/incorrect string and the new/correct string are
          * the same length, we can just replace inside the same buffer.
          */
         if (!cmsUtl_strcmp(nxmlCtx.paramNode->name, "LinkEncapsulationUsed")  &&
             !cmsUtl_strcmp(nxmlCtx.paramValue, MDMVS_G_992_3_ANNEX_K_PTM))
         {
            snprintf(nxmlCtx.paramValue, len+1, "%s", MDMVS_G_993_2_ANNEX_K_PTM);
            cmsLog_debug("converted %s to %s",
                         MDMVS_G_992_3_ANNEX_K_PTM, nxmlCtx.paramValue);
         }


         if ((nxmlCtx.versionMajor < 3) &&
             (nxmlCtx.objNode->oid == MDMOID_L3_FORWARDING) &&
             (!cmsUtl_strcmp(nxmlCtx.paramNode->name, "DefaultConnectionService")) &&
             (nxmlCtx.paramValue != NULL) &&
             (nxmlCtx.loadMdm))
         {
            cmsLog_debug("converting v1/v2 defaultConnectionService");
            convert_v1_v2_defaultGateway();
         }

         /************* End convert v1,v2 config file ****************/
#endif

         if (nxmlCtx.paramNode->flags & PRN_CONFIG_PASSWORD)
         {
            char *plaintext=NULL;
            UINT32 plaintextLen;

            /* in some very weird corner cases (e.g. unit tests), password may be blank */
            if (nxmlCtx.paramValue)
            {
               r1 = cmsB64_decode(nxmlCtx.paramValue, (unsigned char **) &plaintext, &plaintextLen);
               if (r1 == CMSRET_SUCCESS)
               {
                  cmsLog_debug("decoded string=%s", plaintext);
                  cmsMem_free(nxmlCtx.paramValue);
                  nxmlCtx.paramValue = plaintext;
               }
            }
         }

         if ((r1 != CMSRET_SUCCESS) ||
             ((r2 = mdm_validateParamNodeString(nxmlCtx.paramNode, nxmlCtx.paramValue)) != CMSRET_SUCCESS))
         {
            cmsLog_error("invalid string %s", nxmlCtx.paramValue);
            nxmlCtx.ret = (r1 != CMSRET_SUCCESS) ? r1 : r2;
         }
         else
         {
            r2 = mdm_setParamNodeString(nxmlCtx.paramNode, nxmlCtx.paramValue, mdmLibCtx.allocFlags, nxmlCtx.mdmObj);
            if (r2 != CMSRET_SUCCESS)
            {
               nxmlCtx.ret = r2;
            }
            else
            {
               cmsLog_debug("set obj %s :: param %s -> value %s",
                            nxmlCtx.objNode->name,
                            nxmlCtx.paramNode->name,
                            nxmlCtx.paramValue);
            }

            if ((nxmlCtx.ret == CMSRET_SUCCESS) &&
                (nxmlCtx.loadMdm) &&
                (nxmlCtx.attr.accessBitMaskChange || nxmlCtx.attr.notificationChange))
            {

               cmsLog_debug("set single param attr notif=(%d)%d access=(%d)0x%x",
                            nxmlCtx.attr.notificationChange,
                            nxmlCtx.attr.notification,
                            nxmlCtx.attr.accessBitMaskChange,
                            nxmlCtx.attr.accessBitMask);

               r2 = mdm_setSingleParamNodeAttributes(nxmlCtx.paramNode,
                                                     &(nxmlCtx.iidStack),
                                                     &(nxmlCtx.attr),
                                                     FALSE);
               if (r2 != CMSRET_SUCCESS)
               {
                  cmsLog_error("setSingle attr failed for %s %s notification=(%d)%d accessBitMask=(%d)0x%x ret=%d",
                               nxmlCtx.paramNode->name,
                               cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)),
                               nxmlCtx.attr.notificationChange,
                               nxmlCtx.attr.notification,
                               nxmlCtx.attr.accessBitMaskChange,
                               nxmlCtx.attr.accessBitMask, 
                               r2);
                  nxmlCtx.ret = r2;
               }
            }

            nxmlCtx.attr.accessBitMaskChange = 0;
            nxmlCtx.attr.notificationChange = 0;
         }

         CMSMEM_FREE_BUF_AND_NULL_PTR(nxmlCtx.paramValue);
         nxmlCtx.paramNode = NULL;
      }
   }
   else
   {
      /*
       * This must be the end tag of an object.
       */
#ifdef CMS_CONFIG_COMPAT
      /*
       * In December 2011, 4in6Tunnel and 6in4Tunnel were
       * renamed to V4inV6Tunnel and V6inV4Tunnel, respectively.
       */
      if ((nxmlCtx.versionMajor < 4) && (nxmlCtx.objNode != NULL))
      {
         convert_v3_tunnelObjects(nxmlCtx.objNode->oid, buf);
      }

      /*
       * In 4.14, X_BROADCOM_COM_LineSetting was renamed to
       * X_BROADCOM_COM_DectLineSetting
       */
      if ((nxmlCtx.objNode != NULL) && (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_LineSetting")))
      {
         snprintf(buf, NXML_MAX_NAME_SIZE, "X_BROADCOM_COM_DectLineSetting");
         cmsLog_debug("converted X_BROADCOM_COM_LineSetting to %s", buf);
      }

      /*
       * In 4.14L.04 (Oct 2013), X_BROADCOM_COM_MLDSnoopingConfig was moved
       * out from under X_BROADCOM_COM_IPv6LANHostConfigManagement.  But
       * old config files will still have this end tag, so replace it
       * with the expected endTag of LANDevice and also don't pop up
       * to the next level because we already popped up to the right
       * level at the end of X_BROADCOM_COM_MLDSnoopingConfig.
       */
      if (!cmsUtl_strcmp(buf, "X_BROADCOM_COM_IPv6LANHostConfigManagement") &&
          (nxmlCtx.objNode->oid == MDMOID_LAN_DEV))
      {
         sprintf(buf, "LANDevice");
         compatNormalPop = FALSE;
      }
#endif /* CMS_CONFIG_COMPAT */


      /* verify the end tag matches the start tag */
      if ((!strcmp(nxmlCtx.objNode->name, buf)) ||
          ((nxmlCtx.objNode == mdmShmCtx->rootObjNode) && (!strcmp(buf, CONFIG_FILE_TOP_NODE))))
      {
         /* this is the correct tag matching case, do nothing. */
      }
      else
      {
         cmsLog_error("unexpected end tag, expected obj name %s got %s",
                      nxmlCtx.objNode->name, buf);
         nxmlCtx.ret = CMSRET_INVALID_ARGUMENTS;
      }


      /*
       * As a special case check, there could be a snipet that looks
       * like this which is at the end of the config file:
       *       <someobject notification=2 accessList=tr69c,telnetd>
       *       </someobject>
       *     </someParentObject>
       *   </InternetGatewayDevice>
       * </DslCpeConfig>
       * (There is no transition to another object or a param.)
       * In which case, the next few lines of code will set the attribute for
       * that object.
       */
      if ((nxmlCtx.ret == CMSRET_SUCCESS) &&
          (nxmlCtx.loadMdm) &&
          (nxmlCtx.attr.accessBitMaskChange || nxmlCtx.attr.notificationChange))
      {
         CmsRet r2;

         cmsLog_debug("set sub-tree attr notif=(%d)%d access=(%d)0x%x",
                      nxmlCtx.attr.notificationChange,
                      nxmlCtx.attr.notification,
                      nxmlCtx.attr.accessBitMaskChange,
                      nxmlCtx.attr.accessBitMask);

         r2 = mdm_setSubTreeParamAttributes(nxmlCtx.objNode,
                                            &(nxmlCtx.iidStack),
                                            &(nxmlCtx.attr),
                                            FALSE);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("setSubTree attr failed for %s %s notification=(%d)%d accessBitMask=(%d)0x%x ret=%d",
                         nxmlCtx.objNode->name,
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)),
                         nxmlCtx.attr.notificationChange,
                         nxmlCtx.attr.notification,
                         nxmlCtx.attr.accessBitMaskChange,
                         nxmlCtx.attr.accessBitMask,
                         r2);
            nxmlCtx.ret = r2;
         }

         nxmlCtx.attr.accessBitMaskChange = 0;
         nxmlCtx.attr.notificationChange = 0;
      }

      /*
       * Set the MdmObject into the MDM.  We need to do this before we pop
       * any instance id's off of the iidStack.  Only the first end object
       * tag will have an MdmObject.  All other higher level objects were
       * already set when the parsing transitioned down to the lower level
       * begin tag.
       */
      if (nxmlCtx.ret == CMSRET_SUCCESS)
      {
         if (nxmlCtx.mdmObj != NULL && nxmlCtx.nextInstanceNode == FALSE)
         {
            cmsLog_debug("setting obj %s %s",
                         mdm_oidToGenericPath(*((MdmObjectId *)nxmlCtx.mdmObj)),
                         cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
            if (nxmlCtx.loadMdm)
            {
               CmsRet r2;

               r2 = mdm_setObject(&(nxmlCtx.mdmObj), &(nxmlCtx.iidStack), FALSE);
               if (r2 != CMSRET_SUCCESS)
               {
                  cmsLog_error("setObject failed, %d", r2);
                  mdm_freeObject(&(nxmlCtx.mdmObj));
                  nxmlCtx.ret = r2;
               }
            }
            else
            {
               mdm_freeObject(&(nxmlCtx.mdmObj));
            }
         }
      }


      if ((nxmlCtx.objNode->parent != NULL) && compatNormalPop)
      {
         /* normal end of object processing. */

         if (IS_INDIRECT2(nxmlCtx.objNode))
         {
            POP_INSTANCE_ID(&(nxmlCtx.iidStack));
         }

         nxmlCtx.objNode = nxmlCtx.objNode->parent;
         cmsLog_debug("pop up to %s %s",
                      mdm_oidToGenericPath(nxmlCtx.objNode->oid),
                      cmsMdm_dumpIidStack(&(nxmlCtx.iidStack)));
      }

      /*
       * Remember the fact we have seen the end tag for this object.
       * This is needed for the special case where we have an object
       * called WEPKey and a parameter called WEPKey and we need to
       * know whether we are about to load the WEPKey object or the
       * WEPKey parameter.  WEPKey object can only be loaded when
       * the end tag of the previous object has been seen.
       */
      nxmlCtx.gotCurrObjEndTag = TRUE;
   }
}


UBOOL8 isAllWhiteSpace(const char *p, UINT32 len)
{
   UINT32 i=0;

   cmsLog_debug("checking %d bytes starting at 0x%x", len, *p);
   for (i=0; i<len; i++)
   {
      cmsLog_debug("p is 0x%x", *p);
      if (*p != 0 &&    /* terminator byte */
          *p != 0xa &&  /* new line */
          *p != 0xd &&  /* carriage return */
          *p != 0x20)   /* space */
      {
         return FALSE;
      }
      p++;
   }

   return TRUE;
}


CmsRet parseVersionNumber(const char *buf, UINT32 *major, UINT32 *minor)
{
   char *copy, *midpoint;
   CmsRet ret;

   copy = cmsMem_strdup(buf);

   midpoint = strstr(copy, ".");

   if (midpoint == NULL)
   {
      /*
       * There is no .  That is OK, treat the whole buffer as a major number.
       */

      ret = cmsUtl_strtoul(copy, NULL, 0, major);
      *minor = 0;
   }
   else
   {
      /* break the string */
      *midpoint = (char) 0;
      midpoint++;

      ret = cmsUtl_strtoul(copy, NULL, 0, major);
      if (ret == CMSRET_SUCCESS)
      {
         ret = cmsUtl_strtoul(midpoint, NULL, 0, minor);
      }
   }


   cmsMem_free(copy);

   return ret;
}


#ifdef CMS_CONFIG_COMPAT

/*
 * Convert version 1 config file password field to version 2+ config file password.
 * In version 1, the passwords were stored in cleartext.  In version 2 and beyond,
 * we expect these fields to be base64 encoded.  So encode the password.
 */
void convert_v1_password(void)
{
   char *encryptedStr=NULL;

   if (nxmlCtx.paramValue)
   {
      cmsB64_encode((unsigned char *) nxmlCtx.paramValue, strlen(nxmlCtx.paramValue)+1, &encryptedStr);

      cmsMem_free(nxmlCtx.paramValue);

      nxmlCtx.paramValue = encryptedStr;
   }
}


void convert_v1_v2_defaultGateway(void)
{
   char fullPath[BUFLEN_1024];
   char ifName[CMS_IFNAME_LENGTH];
   MdmPathDescriptor pathDesc;
   MdmParamNode *paramNode=NULL;
   void *obj=NULL;
   CmsRet ret;

   /* need to put a . at the end of the pathname for conversion */
   snprintf(fullPath, sizeof(fullPath), "%s.", nxmlCtx.paramValue);

   cmsLog_debug("looking up %s", fullPath);
   if ((ret = cmsMdm_fullPathToPathDescriptor(fullPath, &pathDesc)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert %s to pathDesc, ret=%d", fullPath, ret);
      return;
   }

   /* get the ppp or ip object, this is a direct pointer to the object, so don't free it */
   ret = mdm_getObjectPointer(pathDesc.oid, &(pathDesc.iidStack), &obj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get the MDM object for oid %d iidStack %s",
                   pathDesc.oid, cmsMdm_dumpIidStack(&(pathDesc.iidStack)));
      return;
   }

   /* get the ifName */
   if (pathDesc.oid == MDMOID_WAN_PPP_CONN)
   {
      strcpy(ifName, ((WanPppConnObject *) obj)->X_BROADCOM_COM_IfName);
   }
   else if (pathDesc.oid == MDMOID_WAN_IP_CONN)
   {
      strcpy(ifName, ((WanIpConnObject *) obj)->X_BROADCOM_COM_IfName);
   }
   else 
   {
      cmsLog_error("unexpected oid %d", pathDesc.oid);
      return;
   }

   cmsLog_debug("ifname is %s", ifName);

   /* get the paramNode ptr to the X_BROADCOM_COM_DefaultConnectionServices parameter in the same object */
   if ((paramNode = mdm_getParamNode(nxmlCtx.objNode->oid, "X_BROADCOM_COM_DefaultConnectionServices")) == NULL)
   {
      cmsLog_error("could not get paramNode for X_BROADCOM_COM_DefaultConnectionServices");
      return;
   }

   /* directly set the parameter */
   ret = mdm_setParamNodeString(paramNode, ifName, mdmLibCtx.allocFlags, nxmlCtx.mdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set X_BROADCOM_COM_DefaultConnectionServices parameter to %s, ret=%d",
                   ifName, ret);
      return;
   }
                   
   /* free the current param value and set to NULL */
   CMSMEM_FREE_BUF_AND_NULL_PTR(nxmlCtx.paramValue);
   cmsLog_debug("conversion successful, set old DefaultConnectionService to %s", nxmlCtx.paramValue);

   return;
}


/*
 * In December 2011, 4in6Tunnel and 6in4Tunnel were
 * renamed to V4inV6Tunnel and V6inV4Tunnel, respectively.
 */
static void convert_v3_tunnelObjects(MdmObjectId oid, char *buf)
{
   if (MDMOID_IP_TUNNEL == oid ||
       MDMOID_IPV4IN_IPV6_TUNNEL == oid ||
       MDMOID_IPV6IN_IPV4_TUNNEL == oid)
   {
      if (!cmsUtl_strcmp(buf, "4in6Tunnel"))
      {
         cmsLog_debug("convert 4in6Tunnel to V4inV6Tunnel");
         sprintf(buf, "V4inV6Tunnel");
      }
      else if (!cmsUtl_strcmp(buf, "6in4Tunnel"))
      {
         cmsLog_debug("convert 6in4Tunnel to V6inV4Tunnel");
         sprintf(buf, "V6inV4Tunnel");
      }
   }
}

#endif  /* CMS_CONFIG_COMPAT */
