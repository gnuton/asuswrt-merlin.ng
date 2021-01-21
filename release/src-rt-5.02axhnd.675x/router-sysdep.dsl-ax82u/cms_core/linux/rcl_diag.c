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


#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "devctl_atm.h"
#include "rut_diag.h"
#include "devctl_adsl.h"

#if defined(DMP_DOWNLOAD_1)|| defined(DMP_UPLOAD_1)
/* this header file is installed by userspace/private/libs/tr143_utils */
#include "tr143_defs.h"
#endif

CmsRet rcl_ipPingDiagObject( _IPPingDiagObject *newObj,
                             const _IPPingDiagObject *currObj,
                             const InstanceIdStack *iidStack __attribute__((unused)),
                             char **errorParam,
                             CmsRet *errorCode)
{
   CmsRet ret=CMSRET_SUCCESS;
   char cmdLine[BUFLEN_128];
   char ifName[CMS_IFNAME_LENGTH]={0};
   int pid;

   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    */
   if (ADD_NEW(newObj, currObj))
   {
      return ret;
   }

   if (mdmLibCtx.eid != EID_SSK)
   {
      // is diagnosticsState changed?
      if (cmsUtl_strcmp(currObj->diagnosticsState, newObj->diagnosticsState) != 0)
      {
         // diagnosticsState is changed but not to MDMVS_REQUESTED
         if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0)
         {
            cmsLog_debug("Mgmt apps may only write requested to this object");
            return CMSRET_INVALID_ARGUMENTS;
         }
      }
      // diagnosticsState is still MDMVS_NONE so return CMSRET_SUCCESS
      // to make other parameters can be changed
      else if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_NONE) == 0)
      {
         return CMSRET_SUCCESS;
      }
   }

   if (cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0)
   {
      cmsLog_notice("Sending message to SMD to start doing Ping");
      cmsLog_debug("newObj->diagnosticsState %s, repetitions %d, size %d, host %s",
                   newObj->diagnosticsState,(int)newObj->numberOfRepetitions,
                   newObj->dataBlockSize,newObj->host);

      /* now checks to see if we have all the required parameter or not */
      if (newObj->host == NULL)
      {
         *errorParam = cmsMem_strdupFlags("Host",mdmLibCtx.allocFlags);
         *errorCode = CMSRET_INVALID_PARAM_VALUE;
         return CMSRET_INVALID_ARGUMENTS;
      }

      /* check on interface parameter to see if it's valid  */
      if (!IS_EMPTY_STRING(newObj->interface))
      {
         if ((ret = qdmIntf_fullPathToIntfnameLocked(newObj->interface, ifName)) != CMSRET_SUCCESS)
         {
            cmsLog_error("qdmIntf_fullPathToIntfname(%s) returns error ret=%d", newObj->interface,ret);
            *errorParam = cmsMem_strdupFlags("Interface",mdmLibCtx.allocFlags);
            *errorCode = CMSRET_INVALID_PARAM_VALUE;
            return CMSRET_INVALID_ARGUMENTS;
         }
      }

      /* -m option is used for httpd and tr69c only where the cms message sytem are used for 
       * sending message back to ssk to update the IPPingDiagObject
       */
      if (mdmLibCtx.eid != EID_TR69C && mdmLibCtx.eid != EID_CWMPD)
      {
         if (cmsUtl_strlen(ifName) == 0)
            sprintf(cmdLine, "-c %d -s %d -q -m %s", (int)newObj->numberOfRepetitions,
                    newObj->dataBlockSize, newObj->host);
         else
            sprintf(cmdLine, "-c %d -s %d -q -m %s -I %s", (int)newObj->numberOfRepetitions,
                    newObj->dataBlockSize, newObj->host, ifName);
      }
      else
      {
         // add -d option when ping is executed from tr69c/cwmpd application
         if (cmsUtl_strlen(ifName) == 0)
            sprintf(cmdLine, "-c %d -s %d -q -m -d %d %s", (int)newObj->numberOfRepetitions,
                    newObj->dataBlockSize, mdmLibCtx.eid, newObj->host);
         else
            sprintf(cmdLine, "-c %d -s %d -q -m -d %d %s -I %s", (int)newObj->numberOfRepetitions,
                    newObj->dataBlockSize, mdmLibCtx.eid, newObj->host, ifName);
      }

      cmsLog_debug("ping command string=%s", cmdLine);
   
      pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_PING, cmdLine, strlen(cmdLine)+1);
      
      if (pid == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start PING test.");
         ret = CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("Start PING msg sent, new PING pid=%d", pid);
      }   

      /*
       * At Plugfest 1/15/08, an ACS vendor suggested that when we start
       * a new ping diagnostic, that we should clear the results from
       * the previous diagnostic.
       */
      newObj->successCount = 0;
      newObj->failureCount = 0;
      newObj->averageResponseTime = 0;
      newObj->minimumResponseTime = 0;
      newObj->maximumResponseTime = 0;

   } /* requested */
   else if (cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_NONE) == 0)
   {
      ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_PING, NULL, 0);
      
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to stop PING test.");
         ret = CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("Stop PING msg sent");
      }   
   } /* none */
   return (ret);
}


#ifdef DMP_ATMLOOPBACK_1
CmsRet rcl_wanAtm5LoopbackDiagObject( _WanAtm5LoopbackDiagObject *newObj,
                const _WanAtm5LoopbackDiagObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }

   rutDiag_runAtmOamLoopbackTest(OAM_LB_SEGMENT_TYPE,newObj,currObj,iidStack);
   return CMSRET_SUCCESS;
}

CmsRet rcl_wanAtmF4EndToEndLoopbackDiagObject( _WanAtmF4EndToEndLoopbackDiagObject *newObj,
                const _WanAtmF4EndToEndLoopbackDiagObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }

   rutDiag_runAtmOamLoopbackTest(OAM_F4_LB_END_TO_END_TYPE,newObj,currObj,iidStack);
   return CMSRET_SUCCESS;
}


CmsRet rcl_wanAtmF4LoopbackDiagObject( _WanAtmF4LoopbackDiagObject *newObj,
                const _WanAtmF4LoopbackDiagObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }

   rutDiag_runAtmOamLoopbackTest(OAM_F4_LB_SEGMENT_TYPE,newObj,currObj,iidStack);
   return CMSRET_SUCCESS;
}


CmsRet rcl_wanAtmF5EndToEndLoopbackDiagObject( _WanAtmF5EndToEndLoopbackDiagObject *newObj,
                const _WanAtmF5EndToEndLoopbackDiagObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }

   rutDiag_runAtmOamLoopbackTest(OAM_LB_END_TO_END_TYPE,newObj,currObj,iidStack);
   return CMSRET_SUCCESS;
}
#endif /* DMP_ATMLOOPBACK_1 */

#ifdef DMP_X_BROADCOM_COM_ETHERNETOAM_1
CmsRet rcl_ethernetOamObject( _EthernetOamObject *newObj __attribute__((unused)),
                const _EthernetOamObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_ieee8023ahCfgObject( _Ieee8023ahCfgObject *newObj,
                const _Ieee8023ahCfgObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));
   
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if (ADD_NEW(newObj, currObj))
   {
      /*
       * New object instance.  This only happens at startup time.
       */
      cmsLog_debug("System boots up");
      rutEthOam_set3ahService(TRUE);
   }
   else if (newObj != NULL && currObj != NULL)
   {
      cmsLog_debug("Change 3ah settings");
      rutEthOam_set3ahService(TRUE);
   }
   return ret;
}

CmsRet rcl_ieee8021agCfgObject( _Ieee8021agCfgObject *newObj,
                const _Ieee8021agCfgObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));
   
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if (ADD_NEW(newObj, currObj))
   {
      /*
       * New object instance.  This only happens at startup time.
       */
      cmsLog_debug("System boots up");
      rutEthOam_set1agService(TRUE);
   }
   else if (newObj != NULL && currObj != NULL)
   {
      cmsLog_debug("Change 1ag settings");
      rutEthOam_set1agService(TRUE);
   }
   return ret;
}

CmsRet rcl_localMepObject( _LocalMepObject *newObj __attribute__((unused)),
                const _LocalMepObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_remoteMepObject( _RemoteMepObject *newObj __attribute__((unused)),
                const _RemoteMepObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_eoam1agLoopbackDiagObject( _Eoam1agLoopbackDiagObject *newObj __attribute__((unused)),
                const _Eoam1agLoopbackDiagObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   InstanceIdStack ieee8021agCfgIidStack;
   Ieee8021agCfgObject *ieee8021agCfg = NULL;
   UBOOL8 eoam1agEnbl = FALSE;
   char cmd[BUFLEN_1024];
   char modeBuf[BUFLEN_8];
   CmsRet ret = CMSRET_INVALID_ARGUMENTS;
   FILE *file = NULL;
   char line[BUFLEN_256];

   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }

   cmsLog_debug("newObj->diagnosticsState=%s, localMepId=%d, targetMacAddress=%s, numberOfRepetitions=%d",
                newObj->diagnosticsState, newObj->localMepId, 
                newObj->targetMacAddress, newObj->numberOfRepetitions);

   if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0)
   {
      /* don't do anything if not requested */
      return CMSRET_SUCCESS;
   }

   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState, MDMVS_ERROR, mdmLibCtx.allocFlags);
   newObj->successCount = 0;
   newObj->failureCount = 0;

   /* MDMOID_IEEE8021AG_CFG */
   INIT_INSTANCE_ID_STACK(&ieee8021agCfgIidStack);
   if ((ret = cmsObj_get(MDMOID_IEEE8021AG_CFG, &ieee8021agCfgIidStack, 0, (void **)&ieee8021agCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_get <MDMOID_IEEE8021AG_CFG> returns error. ret=%d", ret);
      return ret;
   }

   eoam1agEnbl = ieee8021agCfg->enabled;
   if(ieee8021agCfg->Y1731Enabled)
   {
      snprintf(modeBuf, sizeof(modeBuf), "1731");
   }
   else
   {
      snprintf(modeBuf, sizeof(modeBuf), "1ag");
   }
  
   cmsObj_free((void **) &ieee8021agCfg);

   if(!eoam1agEnbl)
   {
      cmsLog_debug("Ethernet OAM is disabled.");
      return CMSRET_INVALID_ARGUMENTS;
   }
   
   snprintf(cmd, sizeof(cmd), "tmsctl %s send_lbm -m %d -c %d %s",
      modeBuf, newObj->localMepId, newObj->numberOfRepetitions, newObj->targetMacAddress);
   rut_doSystemAction("Eoam1agLoopbackDiag", cmd);   

   file = fopen(ETHOAM_LBM_RESULT_FILE, "r");
   if(file != NULL)
   {
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState, MDMVS_COMPLETE, mdmLibCtx.allocFlags);
      while ((fgets(line, BUFLEN_256, file) != NULL )) 
      {
         newObj->successCount = atoi(line);
         newObj->failureCount = newObj->numberOfRepetitions - newObj->successCount;
      }
      fclose(file);
   }
   else
   {
      /* LBM never executed before */
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState, MDMVS_ERROR, mdmLibCtx.allocFlags);
   }      

   if (mdmLibCtx.eid == EID_TR69C)
   {
      CmsMsgHeader msg = EMPTY_MSG_HEADER;
      msg.type = CMS_MSG_DIAG;
      msg.src =  EID_SMD;
      msg.dst = mdmLibCtx.eid;

      msg.flags_event = 1;
      if (cmsMsg_send(mdmLibCtx.msgHandle, &msg) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not send out CMS_MSG_DIAG event msg");
      }
      else
      {
         cmsLog_debug("Send out CMS_MSG_DIAG event msg.");
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_eoam1agLinktraceDiagObject( _Eoam1agLinktraceDiagObject *newObj __attribute__((unused)),
                const _Eoam1agLinktraceDiagObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   InstanceIdStack ieee8021agCfgIidStack;
   Ieee8021agCfgObject *ieee8021agCfg = NULL;
   UBOOL8 eoam1agEnbl = FALSE;
   char cmd[BUFLEN_1024];
   char modeBuf[BUFLEN_8];
   char ttlBuf[BUFLEN_16];
   CmsRet ret = CMSRET_INVALID_ARGUMENTS;
   FILE *file = NULL;
   char line[BUFLEN_1024];

   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }

   cmsLog_debug("newObj->diagnosticsState=%s, localMepId=%d, targetMacAddress=%s, numberOfRepetitions=%d",
                newObj->diagnosticsState, newObj->localMepId, 
                newObj->targetMacAddress, newObj->maxHopCount);

   if (cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) != 0)
   {
      /* don't do anything if not requested */
      return CMSRET_SUCCESS;
   }

   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState, MDMVS_ERROR, mdmLibCtx.allocFlags);
   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->mepList, "", mdmLibCtx.allocFlags);

   /* MDMOID_IEEE8021AG_CFG */
   INIT_INSTANCE_ID_STACK(&ieee8021agCfgIidStack);
   if ((ret = cmsObj_get(MDMOID_IEEE8021AG_CFG, &ieee8021agCfgIidStack, 0, (void **)&ieee8021agCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_get <MDMOID_IEEE8021AG_CFG> returns error. ret=%d", ret);
      return ret;
   }

   eoam1agEnbl = ieee8021agCfg->enabled;
   if(ieee8021agCfg->Y1731Enabled)
   {
      snprintf(modeBuf, sizeof(modeBuf), "1731");
   }
   else
   {
      snprintf(modeBuf, sizeof(modeBuf), "1ag");
   }
  
   cmsObj_free((void **) &ieee8021agCfg);

   if(!eoam1agEnbl)
   {
      cmsLog_debug("Ethernet OAM is disabled.");
      return CMSRET_INVALID_ARGUMENTS;
   }
   if(newObj->maxHopCount > 0)
   {
      snprintf(ttlBuf, sizeof(ttlBuf), "-t %d", newObj->maxHopCount);
   }
   else
   {
      ttlBuf[0] = '\0';
   }
   snprintf(cmd, sizeof(cmd), "tmsctl %s send_ltm -m %d %s %s",
      modeBuf, newObj->localMepId, ttlBuf, newObj->targetMacAddress);
   rut_doSystemAction("Eoam1agLinkTraceDiag", cmd);   

   file = fopen(ETHOAM_LTM_RESULT_FILE, "r");
   if(file != NULL)
   {
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState, MDMVS_COMPLETE, mdmLibCtx.allocFlags);
      if (fgets(line, BUFLEN_1024, file)) 
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->mepList, line, mdmLibCtx.allocFlags);
      }
      fclose(file);
   }
   else
   {
      /* LTM never executed before */
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState, MDMVS_ERROR, mdmLibCtx.allocFlags);
   }      

   if (mdmLibCtx.eid == EID_TR69C)
   {
      CmsMsgHeader msg = EMPTY_MSG_HEADER;
      msg.type = CMS_MSG_DIAG;
      msg.src =  EID_SMD;
      msg.dst = mdmLibCtx.eid;

      msg.flags_event = 1;
      if (cmsMsg_send(mdmLibCtx.msgHandle, &msg) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not send out CMS_MSG_DIAG event msg");
      }
      else
      {
         cmsLog_debug("Send out CMS_MSG_DIAG event msg.");
      }
   }

   return CMSRET_SUCCESS;
}

#endif

#ifdef DMP_DSLDIAGNOSTICS_1
CmsRet rcl_wanDslDiagObject( _WanDslDiagObject *newObj,
                             const _WanDslDiagObject *currObj,
                             const InstanceIdStack *iidStack,
                             char **errorParam __attribute__((unused)),
                             CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }

   if ((mdmLibCtx.eid != EID_SSK) &&
       cmsUtl_strcmp(newObj->loopDiagnosticsState, MDMVS_REQUESTED))
   {
      cmsLog_debug("Mgmt apps may only write requested to this object");
      return CMSRET_INVALID_ARGUMENTS;
   }
   if (cmsUtl_strcmp(newObj->loopDiagnosticsState,MDMVS_REQUESTED) == 0) 
   {
      /* DSL Loop Diagnostics will bring ADSL link down.  ACS (TR69) expects
       * a set request response after initiating this test.  This RCL needs to return
       * set success before initiating this test to allow time for TR69 client to send
       * the response.
       * 
       * 1. If management apps initiates the test, message is sent SSK and returns.
       * 2. SSK receives the message, it does 2 things:
       *    a. calls objSet again to set loopDiagnosticsState to REQUESTED.
       *    This is to avoid having SSK having to initiate the test.  This routine will be called 
       *    again.  ADSL driver will be called to start the test.
       *    b. SSK starts to poll for the result of the test.
       */

      if (mdmLibCtx.eid != EID_SSK)
      {
         UINT32 msgDataLen = sizeof(dslDiagMsgBody);
         char buf[sizeof(CmsMsgHeader) + sizeof(dslDiagMsgBody)]={0};
         CmsMsgHeader *msg = (CmsMsgHeader *) buf;
         dslDiagMsgBody *info = (dslDiagMsgBody *) &(buf[sizeof(CmsMsgHeader)]);
      
         cmsLog_notice("Sending message to SSK to start doing checking the results, iidStack %s",cmsMdm_dumpIidStack(iidStack));
         msg->type = CMS_MSG_WATCH_DSL_LOOP_DIAG;
         msg->src = mdmLibCtx.eid;
         msg->dst = EID_SSK;
         msg->flags_request = 1;
         msg->dataLength = msgDataLen;
         
         info->iidStack = *iidStack;

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->loopDiagnosticsState,MDMVS_NONE,mdmLibCtx.allocFlags);
         if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, msg)) != CMSRET_SUCCESS)
         {
            cmsLog_debug("Could not send out CMS_MSG_DSL_LOOP_DIAG_MONITOR ret=%d", ret);
         }
         else
         {
            cmsLog_debug("CMS_MSG_DSL_LOOP_DIAG_MONITOR msg sent");
         }
      } /* !ssk */
      else
      {
         UINT32 lineId=0;
         WanDslIntfCfgObject *dslCfgObj=NULL;

         /* Give application like TR69c a chance to do a reponse to server
            first before bringing down xDSL link */
         sleep(1);
         cmsLog_debug("Ask ADSL driver to start the test.");
         if (cmsObj_get(MDMOID_WAN_DSL_INTF_CFG, iidStack, 0,(void **)&dslCfgObj) == CMSRET_SUCCESS)
         {
            lineId = dslCfgObj->X_BROADCOM_COM_BondingLineNumber;
            cmsObj_free((void **) &dslCfgObj);
         }
         ret = xdslCtl_SetTestMode(lineId, ADSL_TEST_DIAGMODE);
      }
   } /* requested */
   return (ret);
}
#endif /* DMP_DSLDIAGNOSTICS_1 */

CmsRet rcl_capabilitiesObject( _CapabilitiesObject *newObj __attribute__((unused)),
                const _CapabilitiesObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_performanceDiagObject( _PerformanceDiagObject *newObj __attribute__((unused)),
                const _PerformanceDiagObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#if defined(DMP_DOWNLOAD_1)
CmsRet rcl_dlDiagObject( _DlDiagObject *newObj,
      const _DlDiagObject *currObj,
      const InstanceIdStack *iidStack __attribute__((unused)),
      char **errorParam __attribute__((unused)),
      CmsRet *errorCode __attribute__((unused)))
{
   char buf[BUFLEN_256]={0};
   int pid;
   char if_name_cmd[BUFLEN_32] = "-i ";

   if (ADD_NEW(newObj, currObj)) return CMSRET_SUCCESS;

   if (!strcmp(newObj->diagnosticsState, MDMVS_REQUESTED)) 
   {
      if (!IS_EMPTY_STRING(newObj->interface))
      {
         if (qdmIntf_fullPathToIntfnameLocked(newObj->interface, &if_name_cmd[3]) != CMSRET_SUCCESS)
         {
            cmsLog_error("interface %s is invalid", newObj->interface);
            return CMSRET_INVALID_PARAM_VALUE;
         }
      }
      else 
         if_name_cmd[0] = '\0';

      if (IS_EMPTY_STRING(newObj->downloadURL))
      {
         cmsLog_error("downloadURL is empty");
         return CMSRET_INVALID_PARAM_VALUE;
      }

      unlink(TR143_DOWNLOAD_RESULT_FILE);
      snprintf(buf, sizeof(buf), "%s -d %d -u %s", if_name_cmd, newObj->DSCP, newObj->downloadURL);

      pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_DOWNLOAD_DIAG, buf, strlen(buf)+1);
      if (pid == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start download diag test.");
         return CMSRET_INTERNAL_ERROR;
      }

      cmsLog_debug("Start download diag sent, pid=%d, cmd=%s", pid, buf);
   }
   else
   {
      // Stop Running test
      if (!strcmp(currObj->diagnosticsState, MDMVS_REQUESTED))
      {
         unlink(TR143_DOWNLOAD_RESULT_FILE);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->diagnosticsState, MDMVS_NONE, mdmLibCtx.allocFlags);
         if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_DOWNLOAD_DIAG, NULL, 0) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to stop upload diag test.");
            return CMSRET_INTERNAL_ERROR;
         }
         cmsLog_debug("Stop download diag msg sent");
      }

      // ACS server try to set a invalid value into diagnosticsState parameter.
      if (mdmLibCtx.eid == EID_TR69C && strcmp(newObj->diagnosticsState, currObj->diagnosticsState))
         return CMSRET_INVALID_PARAM_VALUE;
   }

   return CMSRET_SUCCESS;
}
#endif

#if defined(DMP_UPLOAD_1)
CmsRet rcl_ulDiagObject( _UlDiagObject *newObj,
      const _UlDiagObject *currObj,
      const InstanceIdStack *iidStack __attribute__((unused)),
      char **errorParam __attribute__((unused)),
      CmsRet *errorCode __attribute__((unused)))
{
   char buf[BUFLEN_256]={0};
   int pid;
   char if_name_cmd[BUFLEN_32]="-i ";

   if (ADD_NEW(newObj, currObj)) return CMSRET_SUCCESS;

   if (!strcmp(newObj->diagnosticsState, MDMVS_REQUESTED)) 
   {
      if (!IS_EMPTY_STRING(newObj->interface))
      {
         if (qdmIntf_fullPathToIntfnameLocked(newObj->interface, &if_name_cmd[3]) != CMSRET_SUCCESS)
         {
            cmsLog_error("interface %s is invalid", newObj->interface);
            return CMSRET_INVALID_PARAM_VALUE;
         }
      }
      else 
         if_name_cmd[0] = '\0';

      if (IS_EMPTY_STRING(newObj->uploadURL))
      {
         cmsLog_error("uploadURL is empty");
         return CMSRET_INVALID_PARAM_VALUE;
      }

      if (newObj->testFileLength == 0)
      {
         cmsLog_error("testFileLength is zero");
         return CMSRET_INVALID_PARAM_VALUE;
      }

      unlink(TR143_UPLOAD_RESULT_FILE);
      snprintf(buf, sizeof(buf), "%s -d %d -l %d -u %s", if_name_cmd, newObj->DSCP, newObj->testFileLength, newObj->uploadURL);

      pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_UPLOAD_DIAG, buf, strlen(buf)+1);
      if (pid == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start upload diag test.");
         return CMSRET_INTERNAL_ERROR;
      }

      cmsLog_debug("Start upload diag sent, pid=%d, cmd=%s", pid, buf);
   }
   else
   {
      if (!strcmp(currObj->diagnosticsState, MDMVS_REQUESTED))
      {
         unlink(TR143_UPLOAD_RESULT_FILE);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->diagnosticsState, MDMVS_NONE, mdmLibCtx.allocFlags);
         if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_UPLOAD_DIAG, NULL, 0) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to stop upload diag test.");
            return CMSRET_INTERNAL_ERROR;
         }

         cmsLog_debug("Stop upload diag msg sent");
      }

      // ACS server try to set a invalid value into diagnosticsState parameter.
      if (mdmLibCtx.eid == EID_TR69C && strcmp(newObj->diagnosticsState, currObj->diagnosticsState))
         return CMSRET_INVALID_PARAM_VALUE;
   }

   return CMSRET_SUCCESS;
}
#endif

#if defined(DMP_UDPECHO_1)
CmsRet rcl_uDPEchoCfgObject( _UDPEchoCfgObject *newObj,
      const _UDPEchoCfgObject *currObj,
      const InstanceIdStack *iidStack __attribute__((unused)),
      char **errorParam __attribute__((unused)),
      CmsRet *errorCode __attribute__((unused)))
{
   int pid;
   UINT32 l;
   char buf[BUFLEN_256]={0};
   char if_name_cmd[BUFLEN_32]={0};

   if (ADD_NEW(newObj, currObj)) return CMSRET_SUCCESS;
   if (mdmLibCtx.eid == EID_UDPECHO) return CMSRET_SUCCESS;

   //clear old firewall rules
   if (currObj->interface)
   {
      cmsUtl_strncpy(buf, currObj->interface, sizeof(buf));
   }

   if (buf[0])
   {
      l = strlen(buf);
      if ((buf[l-1]) != '.' && l < sizeof(buf) -1) 
      {
         buf[l] = '.';
         buf[l + 1] = '\0';
      }
      if (qdmIntf_fullPathToIntfnameLocked(buf, if_name_cmd) == CMSRET_SUCCESS)
         sprintf(buf, "iptables -w -D INPUT -i %s -p udp --dport %d -j ACCEPT", if_name_cmd, currObj->UDPPort);
   }
   else
      sprintf(buf, "iptables -w -D INPUT -p udp --dport %d -j ACCEPT", currObj->UDPPort);

   cmsLog_debug("UDPEchoCfg, del firewall rule = %s", buf);
   rut_doSystemAction("UDPEchoCfg", buf);   

   if (newObj->enable)
   {
      if (ENABLE_EXISTING(newObj, currObj))
      {
         //reset all statistics
         CMSMEM_REPLACE_STRING_FLAGS(newObj->timeFirstPacketReceived, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->timeLastPacketReceived, UNKNOWN_DATETIME_STRING, mdmLibCtx.allocFlags);
         newObj->bytesReceived = 0;
         newObj->bytesResponded = 0;
         newObj->packetsReceived = 0;
         newObj->packetsResponded = 0;
         newObj->X_BROADCOM_COM_PacketsRespondedFail = 0;
      }

      pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_UDPECHO, NULL, 0);
      if (pid == CMS_INVALID_PID)
      {
         cmsLog_error("failed to restart udp echo server(reset).");
         return CMSRET_INTERNAL_ERROR;
      }

      cmsLog_debug("Start echo server(reset) sent, pid=%d", pid);

      if (newObj->interface) 
         strncpy(buf, newObj->interface, sizeof(buf));
      else
         buf[0] = '\0';
      if (buf[0])
      {
         l = strlen(buf);
         if ((buf[l-1]) != '.' && l < sizeof(buf) -1) 
         {
            buf[l] = '.';
            buf[l + 1] = '\0';
         }
         if (qdmIntf_fullPathToIntfnameLocked(buf, if_name_cmd) == CMSRET_SUCCESS)
            sprintf(buf, "iptables -w -A INPUT -i %s -p udp --dport %d -j ACCEPT", if_name_cmd, newObj->UDPPort);
      }
      else
         sprintf(buf, "iptables -w -A INPUT -p udp --dport %d -j ACCEPT", newObj->UDPPort);

      cmsLog_debug("UDPEchoCfg, add firewall rule = %s", buf);
      rut_doSystemAction("UDPEchoCfg", buf);   

      return CMSRET_SUCCESS;
   }

   if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))	
   {
      pid = rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_UDPECHO, NULL, 0);
      if (pid == CMS_INVALID_PID)
         cmsLog_debug("failed to stop udp echo server (maybe not start yet).");
      else
         cmsLog_debug("stop echo server sent, pid=%d", pid);
      return CMSRET_SUCCESS;
   }

   return CMSRET_SUCCESS;
}
#endif



