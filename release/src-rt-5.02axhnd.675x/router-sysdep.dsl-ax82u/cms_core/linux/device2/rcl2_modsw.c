/***********************************************************************
 *
 *  Copyright (c) 2006-2009  Broadcom Corporation
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

#ifdef DMP_DEVICE2_SM_BASELINE_1

#include "cms_core.h"
#include "cms_util.h"
#include "cms_msg_modsw.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut2_modsw.h"
#include "cms_qdm.h"
#include "cms_params_modsw.h"


/*!\file rcl_modsw.c
 * \brief This file contains generic modular sw functions (not specific to
 *        any specific execution env.)
 *
 */

static UBOOL8 isParentEeUp(const char *parentExecEnv)
{
   UBOOL8 isUp = FALSE;

   if (IS_EMPTY_STRING(parentExecEnv))
   {
      /* Always return true if no parentEE found */
      isUp = TRUE;
   }
   else
   {
      MdmPathDescriptor pathDesc;
      ExecEnvObject *parent = NULL;
      CmsRet ret;

      ret = cmsMdm_fullPathToPathDescriptor(parentExecEnv, &pathDesc);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not convert %s", parentExecEnv);
      }
      else
      {
         ret = cmsObj_get(pathDesc.oid, &(pathDesc.iidStack),
                          OGF_NO_VALUE_UPDATE, (void **)&parent);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("could not get parent EE obj, ret=%d", ret);
         }
         else
         {
            isUp = !cmsUtl_strcmp(parent->status, MDMVS_UP);
            cmsObj_free((void **)&parent);
         }
      }
   }

   return isUp;
}


CmsRet rcl_swModulesObject( _SwModulesObject *newObj __attribute((unused)),
                const _SwModulesObject *currObj __attribute((unused)),
                const InstanceIdStack *iidStack __attribute((unused)),
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   /* 
    * During system startup, MDM needs to be initialized completely before 
    * operation on this object is being done.  BBCD is launched after SSK completes initialization.
    */ 
   if (mdmShmCtx->inMdmInit)
   {
      return CMSRET_SUCCESS;
   }
   
   if ((newObj && currObj) && 
       (newObj->X_BROADCOM_COM_PreinstallNeeded !=
        currObj->X_BROADCOM_COM_PreinstallNeeded))
   {
      if (newObj->X_BROADCOM_COM_PreinstallNeeded)
      {
         CmsRet ret;
         CmsMsgHeader reqMsg = EMPTY_MSG_HEADER;

         reqMsg.type = CMS_MSG_REQUEST_PREINSTALL_EE_CHANGE;
         reqMsg.src = mdmLibCtx.eid;
         reqMsg.dst = EID_BBCD;
         reqMsg.flags_event = 1;

         if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, 
                         &reqMsg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("msg send failed, ret=%d", ret);
         }
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_execEnvObject( _ExecEnvObject *newObj,
                const _ExecEnvObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   /* 
    * During system startup, MDM needs to be initialized completely before 
    * operation on this object is being done.  BBCD is launched after SSK completes initialization.
    */ 
   if (mdmShmCtx->inMdmInit)
   {
      return ret;
   }
   
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumExecEnvEntry(iidStack, 1);

	  /* initialize parameters */
      if (IS_EMPTY_STRING(newObj->X_BROADCOM_COM_ContainerName))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_ContainerName, "", mdmLibCtx.allocFlags);
      }
      if (IS_EMPTY_STRING(newObj->X_BROADCOM_COM_MngrAppName))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_MngrAppName, "", mdmLibCtx.allocFlags);
      }

      if (newObj->enable)
      {
         char *fullPath=NULL;
         MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;

         cmsLog_debug("(bootup) Sending startEE msg to start %d", newObj->X_BROADCOM_COM_MngrEid);

         pathDesc.oid = MDMOID_EXEC_ENV;
         pathDesc.iidStack = *iidStack;
         ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPath);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("could not create fullPath to myself, ret=%d", ret);
            return ret;
         }

         rutModsw_sendMsgTo(EID_BBCD, CMS_MSG_START_EE, fullPath, 
                            sizeof(EErequestStartMsgBody));
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      }

      return CMSRET_SUCCESS;
   }
   else if(DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumExecEnvEntry(iidStack, -1);
      /* this object cannot be deleted by mgmt entity, so we do not
       * have to send message to stop the ExecEnv on deleteInstance.
       */

      return CMSRET_SUCCESS;
   }

   if (!(newObj && currObj))
   {
      return CMSRET_SUCCESS;
   }

   if (newObj->requestedRunLevel != currObj->requestedRunLevel)
   {
      if (newObj->currentRunLevel != -1)
      {
         newObj->currentRunLevel = newObj->requestedRunLevel;
      }

      /* The value of this parameter is not part of the device configuration
       * and is always -1 when read.
       */
      newObj->requestedRunLevel = -1;
   }
   if (newObj->enable != currObj->enable)
   {
      char *fullPath=NULL;
      MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;

      /* change in enable parameter detected */
      cmsLog_debug("(runtime) change of enable to %d", newObj->enable);

      pathDesc.oid = MDMOID_EXEC_ENV;
      pathDesc.iidStack = *iidStack;
      ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not create fullPath to myself, ret=%d", ret);
         return ret;
      }

      if (newObj->enable)
      {
         if (cmsUtl_strcmp(currObj->status, MDMVS_DISABLED)==0)
         {
            rutModsw_sendMsgTo(EID_BBCD, CMS_MSG_START_EE,
                               fullPath, sizeof(EErequestStartMsgBody));
         }
         else if (cmsUtl_strcmp(currObj->status, MDMVS_ERROR)==0)
         {
            /* 
             * Allow enable an EE at ERROR staus.
             */
            rutModsw_sendMsgTo(EID_BBCD, CMS_MSG_START_EE,
                               fullPath, sizeof(EErequestStartMsgBody));
         }
         else if (cmsUtl_strcmp(currObj->status, MDMVS_UP))
         {
            cmsLog_debug("start fail!");             
            /* Don't allow enable a running EE again */
            ret = CMSRET_INVALID_PARAM_VALUE;
         }

         /* EE is enabled. set currentRunLevel to initialRunLevel if it is relevant. */
         if (newObj->currentRunLevel != -1)
         {
            newObj->currentRunLevel = newObj->initialRunLevel;
         }
      }
      else
      {
         if (cmsUtl_strcmp(currObj->status, MDMVS_UP)==0)
         {
            rutModsw_sendMsgTo(EID_BBCD, CMS_MSG_STOP_EE,
                               fullPath, sizeof(EErequestStopMsgBody));
         }
         else if (cmsUtl_strcmp(currObj->status, MDMVS_ERROR)==0)
         {
            /* 
             * Disable an EE at ERROR status, simply change the status
             * to DISABLED
             */
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_DISABLED,
                                        mdmLibCtx.allocFlags);
         }
         else if (cmsUtl_strcmp(currObj->status, MDMVS_DISABLED)==0)
         {
            /* Don't allow disable a disabled EE again */
            cmsLog_debug("stop fail!");             
            ret = CMSRET_INVALID_PARAM_VALUE;
         }
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
   }
   else if (cmsUtl_strcmp(newObj->status, currObj->status))
   {
      if (newObj->enable)
      {
         if (cmsUtl_strcmp(newObj->status, MDMVS_X_BROADCOM_COM_STARTING) == 0 ||
             cmsUtl_strcmp(newObj->status, MDMVS_X_BROADCOM_COM_STOPPING) == 0 ||
             cmsUtl_strcmp(newObj->status, MDMVS_DISABLED) == 0)
         {
            UBOOL8 isParentUp;

            isParentUp = isParentEeUp(newObj->parentExecEnv);

            if (!isParentUp && !cmsUtl_strcmp(newObj->status, MDMVS_DISABLED))
            {
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_DISABLED,
                                                 mdmLibCtx.allocFlags);
            }
            else
            {
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_ERROR,
                                                 mdmLibCtx.allocFlags);
            }
         }
         else if (cmsUtl_strcmp(newObj->status, MDMVS_UP) == 0)
         {
            /* status changes to UP. set currentRunLevel to initialRunLevel
             * if it is relevant.
             */
            if (newObj->currentRunLevel != -1)
            {
               newObj->currentRunLevel = newObj->initialRunLevel;
            }
         }
      }
      else
      {
         if (cmsUtl_strcmp(newObj->status, MDMVS_DISABLED))
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_ERROR,
                                              mdmLibCtx.allocFlags);
         }
      }
   }

   return ret;
}


CmsRet rcl_dUObject( _DUObject *newObj,
                const _DUObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumDeploymentUnitEntry(iidStack, 1);
   } /* add new */
   else if(DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumDeploymentUnitEntry(iidStack, -1);
   }
   return CMSRET_SUCCESS;
}


static CmsRet sendRequestEuStateChanged(CmsEntityId mngrEid,
                                        const char *euid,
                                        const char *name,
                                        const char *operation)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   EUrequestStateChangedMsgBody *msgPayload = NULL;

   /* code here to initialize and send the rest of the message to felixd */
   reqMsg = cmsMem_alloc(sizeof(CmsMsgHeader) + sizeof(EUrequestStateChangedMsgBody),
                         ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = CMS_MSG_REQUEST_EU_STATE_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = mngrEid;
   reqMsg->flags_request = 1;
   reqMsg->dataLength = sizeof(EUrequestStateChangedMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (EUrequestStateChangedMsgBody*)body;

   cmsUtl_strncpy(msgPayload->euid, euid, sizeof(msgPayload->euid));
   cmsUtl_strncpy(msgPayload->name, name, sizeof(msgPayload->name));
   cmsUtl_strncpy(msgPayload->operation, operation, sizeof(msgPayload->operation));
   
   cmsLog_debug("sending to mngrEid=%d operation=%s", mngrEid, msgPayload->operation);

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message to MngrEid %d (ret=%d)",
                    mngrEid, ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

static CmsRet sendEuBootupNotification(CmsEntityId mngrEid,
                                       const char *euName,
                                       const char *euid,
                                       const char *username,
                                       const char *duName)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   EUBootupNotificationMsgBody *msgPayload;

   reqMsg = cmsMem_alloc(sizeof(CmsMsgHeader) + sizeof(EUBootupNotificationMsgBody),
                         ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = CMS_MSG_EU_BOOTUP_SETUP;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = mngrEid;
   reqMsg->flags_request = 1;
   reqMsg->dataLength = sizeof(EUBootupNotificationMsgBody);

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (EUBootupNotificationMsgBody*)body;

   cmsUtl_strncpy(msgPayload->euName, euName, sizeof(msgPayload->euName));
   cmsUtl_strncpy(msgPayload->euid, euid, sizeof(msgPayload->euid));
   cmsUtl_strncpy(msgPayload->username, username, sizeof(msgPayload->username));
   cmsUtl_strncpy(msgPayload->duName, duName, sizeof(msgPayload->duName));
   
   cmsLog_debug("sending to mngrEid=%d euName=%s duName=%s", mngrEid, 
                msgPayload->euName, msgPayload->duName);

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message to MngrEid %d (ret=%d)",
                    mngrEid, ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}

CmsRet rcl_eUObject( _EUObject *newObj,
                const _EUObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   UBOOL8 STARTEU;
   CmsRet ret = CMSRET_SUCCESS;
   CmsEntityId mngrEid;

   /* 
    * During system startup, MDM needs to be initialized completely before 
    * operation on this object is being done.  The EE program manager should handle
    * the EU activation when it's up.
    */ 
   if (mdmShmCtx->inMdmInit)
   {
      return ret;
   }
   
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumExecutionUnitEntry(iidStack, 1);

      /* initialize parameters */
      if (IS_EMPTY_STRING(newObj->X_BROADCOM_COM_MngrAppName))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_MngrAppName, "", mdmLibCtx.allocFlags);
      }

      /* startup with existing EU, need to restore container's config */
      if (!IS_EMPTY_STRING(newObj->name))
      {
         MdmPathDescriptor pathDesc;
         char *fullStr=NULL;
         ExecEnvObject *eeObject = NULL;
         InstanceIdStack dummyIid;
         
         /* retrive the EE object for this EU to determine 2 things:
          * 1. is ee enable?
          * 2. is ee->status enabled?
          * If EE is not enabled nor active, do not try to activate
          */
         if (qdmModsw_getExecEnvObjectByFullPathLocked(newObj->executionEnvRef, &eeObject, &dummyIid) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get find ExecEnvFullPath %s",newObj->executionEnvRef);
            return CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            if ((eeObject->enable == FALSE) || (cmsUtl_strcmp(eeObject->status, MDMVS_UP) != 0))
            {
               cmsLog_debug("EE %s is disabled or not UP, do not activate EU",newObj->executionEnvRef);
               cmsObj_free((void **)&eeObject);
               return CMSRET_SUCCESS;
            }
         }
         mngrEid = eeObject->X_BROADCOM_COM_MngrEid;
         cmsObj_free((void **)&eeObject);
         memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
         pathDesc.oid = MDMOID_EU;
         pathDesc.iidStack = *iidStack;

         cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullStr);

         if (IS_EMPTY_STRING(fullStr))
         {
            cmsLog_error("cannot get full path of EU");
         }
         else
         {
            char duName[BUFLEN_32]={0};

            if (qdmModsw_getDeployUnitNameByEuFullPathLocked(fullStr,
                                     duName, sizeof(duName)) == CMSRET_SUCCESS)
            {
               sendEuBootupNotification(mngrEid,
                                        newObj->name,
                                        newObj->EUID,
                                        newObj->X_BROADCOM_COM_Username,
                                        duName);
            }
            else
            {
               cmsLog_error("cannot get duName by euFullPath<%s>, euObj cannot setup container", fullStr);
            }
         }
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullStr);

         /* start EU if autoStart is true */
         if (newObj->autoStart)
         {
            ret = sendRequestEuStateChanged(mngrEid,
                                            newObj->EUID,
                                            newObj->name,
                                            SW_MODULES_OPERATION_START);
         }
      }

      return ret;
   }
   else if(DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumExecutionUnitEntry(iidStack, -1);
      return ret;
   }

   /* modify existing */
   if (newObj->X_BROADCOM_COM_autoRelaunch && !currObj->X_BROADCOM_COM_autoRelaunch)
   {
      cmsLog_debug("autoRelaunch is enabled");

      /* reset the restartCount */
      newObj->X_BROADCOM_COM_restartCount = 0;
   }

   cmsLog_debug("new requestedState=%s curr requestedState=%s currstatus=%s",
         newObj->requestedState, currObj->requestedState, currObj->status);

   if (cmsUtl_strcmp(newObj->requestedState, currObj->requestedState))
   {
      /* we have a change in the requested state */
      if (!cmsUtl_strcmp(newObj->requestedState, MDMVS_ACTIVE))
      {
         /* caller requested start EU, check current status */
         if (!cmsUtl_strcmp(currObj->status, MDMVS_STOPPING))
         {
            cmsLog_error("Requested ACTIVE, but EU is in STOPPING");
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState,
                                        "",
                                        mdmLibCtx.allocFlags);
            return CMSRET_INVALID_ARGUMENTS;
         }
         else if (!cmsUtl_strcmp(currObj->status, MDMVS_STARTING) ||
                  !cmsUtl_strcmp(currObj->status, MDMVS_ACTIVE))
         {
            cmsLog_notice("Requested ACTIVE, but already %s, no action", currObj->status);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState,
                                        "",
                                        mdmLibCtx.allocFlags);
            return CMSRET_SUCCESS;
         }
         else if (!cmsUtl_strcmp(currObj->status, MDMVS_IDLE))
         {
            cmsLog_debug("Requested ACTIVE, currently idle, start EU");
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState,
                                        "",
                                        mdmLibCtx.allocFlags);
            STARTEU = TRUE;
         }
         else
         {
            cmsLog_error("Bad status %s", currObj->status);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState,
                                        "",
                                        mdmLibCtx.allocFlags);
            return CMSRET_INTERNAL_ERROR;
         }
      }
      else if (!cmsUtl_strcmp(newObj->requestedState, MDMVS_IDLE))
      {
         /* caller requested stop EU, check current status */
         if (!cmsUtl_strcmp(currObj->status, MDMVS_STARTING))
         {
            cmsLog_error("Requested IDLE, but EU is in STARTING");
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState,
                                        "",
                                        mdmLibCtx.allocFlags);
            return CMSRET_INVALID_ARGUMENTS;
         }
         else if (!cmsUtl_strcmp(currObj->status, MDMVS_STOPPING) ||
                  !cmsUtl_strcmp(currObj->status, MDMVS_IDLE))
         {
            cmsLog_notice("Requested IDLE, but already %s, no action", currObj->status);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState,
                                        "",
                                        mdmLibCtx.allocFlags);
            return CMSRET_SUCCESS;
         }
         else if (!cmsUtl_strcmp(currObj->status, MDMVS_ACTIVE))
         {
            cmsLog_debug("Requested IDLE, currently ACTIVE, stop EU");
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState,
                                        "",
                                        mdmLibCtx.allocFlags);
            STARTEU = FALSE;
         }
         else
         {
            cmsLog_error("Bad status %s", currObj->status);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState,
                                        "",
                                        mdmLibCtx.allocFlags);
            return CMSRET_INTERNAL_ERROR;
         }
      }
      else
      {
         cmsLog_error("Bad requested state %s", newObj->requestedState);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->requestedState,
                                     "",
                                     mdmLibCtx.allocFlags);
         return CMSRET_INTERNAL_ERROR;
      }
   }
   else
   {
      /* don't handle any other param changes (including autostart) */
      return CMSRET_SUCCESS;
   }

   if (qdmModsw_getMngrEidByExecEnvFullPathLocked(newObj->executionEnvRef, &mngrEid) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get mngrEid for ExecEnvFullPath %s",
            newObj->executionEnvRef);
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (STARTEU)
   {
      ret = sendRequestEuStateChanged(mngrEid,
                                      newObj->EUID,
                                      newObj->name,
                                      SW_MODULES_OPERATION_START);
   }
   else
   {
      ret = sendRequestEuStateChanged(mngrEid,
                                      newObj->EUID,
                                      newObj->name,
                                      SW_MODULES_OPERATION_STOP);
   }

   return ret;
}


CmsRet rcl_extensionsObject( _ExtensionsObject *newObj __attribute((unused)),
                             const _ExtensionsObject *currObj __attribute((unused)),
                             const InstanceIdStack *iidStack __attribute((unused)),
                             char **errorParam __attribute((unused)),
                             CmsRet *errorCode __attribute((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_busObject( _BusObject *newObj,
                      const _BusObject *currObj,
                      const InstanceIdStack *iidStack,
                      char **errorParam __attribute((unused)),
                      CmsRet *errorCode __attribute((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_busObjectPathObject( _BusObjectPathObject *newObj,
                                const _BusObjectPathObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute((unused)),
                                CmsRet *errorCode __attribute((unused)))
{
   /* add new */
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumBusObjectPathEntry(iidStack, 1);
   } /* delete existing */
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumBusObjectPathEntry(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_busInterfaceObject( _BusInterfaceObject *newObj,
                               const _BusInterfaceObject *currObj,
                               const InstanceIdStack *iidStack,
                               char **errorParam __attribute((unused)),
                               CmsRet *errorCode __attribute((unused)))
{
   /* add new */
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumBusInterfaceEntry(iidStack, 1);
   } /* delete or editing existing */
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumBusInterfaceEntry(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_busMethodObject( _BusMethodObject *newObj,
                            const _BusMethodObject *currObj,
                            const InstanceIdStack *iidStack,
                            char **errorParam __attribute((unused)),
                            CmsRet *errorCode __attribute((unused)))
{
   /* add new */
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumBusMethodEntry(iidStack, 1);
   } /* delete existing */
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumBusMethodEntry(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_busSignalObject( _BusSignalObject *newObj,
                            const _BusSignalObject *currObj,
                            const InstanceIdStack *iidStack,
                            char **errorParam __attribute((unused)),
                            CmsRet *errorCode __attribute((unused)))
{
   /* add new */
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumBusSignalEntry(iidStack, 1);
   } /* delete existing */
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumBusSignalEntry(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_busPropertyObject( _BusPropertyObject *newObj,
                              const _BusPropertyObject *currObj,
                              const InstanceIdStack *iidStack,
                              char **errorParam __attribute((unused)),
                              CmsRet *errorCode __attribute((unused)))
{
   /* add new */
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumBusPropertyEntry(iidStack, 1);
   } /* delete existing */
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumBusPropertyEntry(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_busClientObject( _BusClientObject *newObj,
                            const _BusClientObject *currObj,
                            const InstanceIdStack *iidStack,
                            char **errorParam __attribute((unused)),
                            CmsRet *errorCode __attribute((unused)))
{
   return CMSRET_SUCCESS;
}


static CmsRet sendExtEuClientPrivilegeChanged(CmsEntityId mngrEid,
                                              const char *username,
                                              const char *fullPath,
                                              const char *operation)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   unsigned char *body = NULL;
   ExtEuClientPrivilegeChangedMsgBody *msgPayload = NULL;

   /* code here to initialize and send the rest of the message to pmd */
   reqMsg = cmsMem_alloc(sizeof(CmsMsgHeader) + sizeof(ExtEuClientPrivilegeChangedMsgBody),
                         ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      cmsLog_error("alloc memory error!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* initialize header fields */
   reqMsg->type = CMS_MSG_EXT_EU_CLIENT_PRIVILEGE_CHANGE;
   reqMsg->src = mdmLibCtx.eid;
   reqMsg->dst = mngrEid;
   reqMsg->flags_request = 1;
   reqMsg->dataLength = sizeof(ExtEuClientPrivilegeChangedMsgBody);
   reqMsg->flags_bounceIfNotRunning = 1;

   /* copy file into the payload and send message */
   body = (unsigned char *)(reqMsg + 1);
   msgPayload = (ExtEuClientPrivilegeChangedMsgBody*)body;

   cmsUtl_strncpy(msgPayload->username, username, sizeof(msgPayload->username));
   cmsUtl_strncpy(msgPayload->fullPath, fullPath, sizeof(msgPayload->fullPath));
   cmsUtl_strncpy(msgPayload->operation, operation, sizeof(msgPayload->operation));
   
   cmsLog_debug("sending to mngrEid=%d operation=%s", mngrEid, msgPayload->operation);

   ret = cmsMsg_send(mdmLibCtx.msgHandle, reqMsg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message to MngrEid %d (ret=%d)",
                    mngrEid, ret);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}


UBOOL8 isMessageAllowedToSend(const char *euFullPath)
{
   UBOOL8 ret = FALSE;
   UBOOL8 found = FALSE;
   DUObject *duObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   while (!found &&
          cmsObj_getNextFlags(MDMOID_DU,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &duObj) == CMSRET_SUCCESS)
   {
      char *string = NULL, *token = NULL;

      string = duObj->executionUnitList;
      while (string != NULL)
      {
         token = strsep(&string, ",");

         if (!cmsUtl_strcmp(token, euFullPath))
         {
            found = TRUE;

            /* Only allow when DU status is NOT uninstalling
               to avoid error caused by EUObject is already deleted */
            if (duObj->status != NULL &&
                strcmp(duObj->status, MDMVS_UPDATING) != 0 &&
                strcmp(duObj->status, MDMVS_UNINSTALLING) != 0)
            {
               ret = TRUE;
            }

            break;
         }
      }

      cmsObj_free((void **)&duObj);
   }

   return ret;
}

CmsRet rcl_busClientPrivilegeObject( _BusClientPrivilegeObject *newObj,
                                     const _BusClientPrivilegeObject *currObj,
                                     const InstanceIdStack *iidStack,
                                     char **errorParam __attribute((unused)),
                                     CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsEntityId mngrEid;
   char euFullPath[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
   EUObject *euObj = NULL;
   InstanceIdStack iidStackEu = EMPTY_INSTANCE_ID_STACK;

   /* add new */
   if (ADD_NEW(newObj, currObj))
   {
      cmsLog_debug("ADD: name=%s, path=%s, interface=%s, member=%s, memberType=%s\n",
                   newObj->wellknownName, newObj->objectPath, newObj->interface,
                   newObj->member, newObj->memberType);

      /* increase number of bus client privileges */
      rutUtil_modifyNumBusClientPrivilegeEntry(iidStack, 1);

      return ret;
   }

   /* do not hide object when it's deleted */
   if (DELETE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("DELETE: name=%s, path=%s, interface=%s, member=%s, memberType=%s\n",
                   currObj->wellknownName, currObj->objectPath, currObj->interface,
                   currObj->member, currObj->memberType);

      /* decrease number of bus client privileges */
      rutUtil_modifyNumBusClientPrivilegeEntry(iidStack, -1);

      mdmLibCtx.hideObjectsPendingDelete = FALSE;
   }

   /* iidStackEu is started with BusClientPrivilegeObject iidStack */
   memcpy(&iidStackEu, iidStack, sizeof(InstanceIdStack));

   /* get EUObject */
   ret = cmsObj_getAncestorFlags(MDMOID_EU,
                                 MDMOID_BUS_CLIENT_PRIVILEGE,
                                 &iidStackEu,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&euObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get EUObject from BusClientPrivilegeObject iidStack, ret=%d", ret);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* get execution unit full path */
   ret = qdmModsw_getExecUnitFullPathByEuidLocked(euObj->EUID,
                                                  euFullPath,
                                                  sizeof(euFullPath));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get Execution Unit full path of EUID %s",
                   euObj->EUID);
      ret = CMSRET_INVALID_ARGUMENTS;
      goto out;
   }

   /* check message is allowed to send */
   if (!isMessageAllowedToSend(euFullPath) ||
       !cmsUtl_strcmp(euObj->status, MDMVS_STARTING))
   {
      cmsLog_notice("CMS_MSG_EXT_EU_CLIENT_PRIVILEGE_CHANGE is NOT allowed to send.");
      ret = CMSRET_SUCCESS;
      goto out;
   }

   /* get management application Eid */
   ret = qdmModsw_getMngrEidByExecEnvFullPathLocked(euObj->executionEnvRef,
                                                    &mngrEid);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get mngrEid for ExecEnvFullPath %s",
                   euObj->executionEnvRef);
      ret = CMSRET_INVALID_ARGUMENTS;
      goto out;
   }

    /* delete existing */
   if (DELETE_EXISTING(newObj, currObj))
   {
      /* reset to hide object when it's deleted */
      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

      /* send EU client privilege changed message to spd
         for updating the busgate policy of the client. */
      ret = sendExtEuClientPrivilegeChanged(mngrEid,
                                            euObj->X_BROADCOM_COM_Username,
                                            euFullPath,
                                            SW_MODULES_OPERATION_UPDATE_PRIVILEGE);
   }
   /* modify existing */
   else
   {
      cmsLog_debug("EDIT: name=%s, path=%s, interface=%s, member=%s, memberType=%s\n",
                   newObj->wellknownName, newObj->objectPath, newObj->interface,
                   newObj->member, newObj->memberType);

      /* send EU client privilege changed message to spd
         for updating the busgate policy of the client. */
      ret = sendExtEuClientPrivilegeChanged(mngrEid,
                                            euObj->X_BROADCOM_COM_Username,
                                            euFullPath,
                                            SW_MODULES_OPERATION_UPDATE_PRIVILEGE);
   }

out:
   /* free EUObject */
   cmsObj_free((void **)&euObj);

   return ret;
}


CmsRet rcl_manifestObject( _ManifestObject *newObj,
                          const _ManifestObject *currObj,
                          const InstanceIdStack *iidStack,
                          char **errorParam __attribute((unused)),
                          CmsRet *errorCode __attribute((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      if (IS_EMPTY_STRING(newObj->exposedPorts))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->exposedPorts, "", mdmLibCtx.allocFlags);
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dmAccessObject(_DmAccessObject *newObj,
                          const _DmAccessObject *currObj,
                          const InstanceIdStack *iidStack,
                          char **errorParam,
                          CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}


#endif  /* DMP_DEVICE2_SM_BASELINE_1 */

