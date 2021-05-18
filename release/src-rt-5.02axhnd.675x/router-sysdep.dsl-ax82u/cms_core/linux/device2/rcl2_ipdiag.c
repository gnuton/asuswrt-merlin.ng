/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
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


#ifdef DMP_DEVICE2_BASELINE_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "cms_qdm.h"

#if defined(DMP_DEVICE2_DOWNLOAD_1) || defined(DMP_DEVICE2_UPLOAD_1)
/* this header file is installed by userspace/private/libs/tr143_utils */
#include "tr143_defs.h"
#endif

/*!\file rcl2_ipdiag.c
 * \brief This file contains device 2 device.ip.diagnostics objects related functions.
 *
 */
#ifdef DMP_DEVICE2_IPPING_1
CmsRet rcl_dev2IpDiagnosticsObject( _Dev2IpDiagnosticsObject *newObj __attribute__((unused)),
                                    const _Dev2IpDiagnosticsObject *currObj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                    char **errorParam __attribute__((unused)),
                                    CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2IpPingDiagObject( _Dev2IpPingDiagObject *newObj,
                                 const _Dev2IpPingDiagObject *currObj,
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
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

      /* -m option is used for httpd and tr69c/cwmpd only where the cms message sytem are used for 
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
#endif /* #ifdef DMP_DEVICE2_IPPING_1 */

#ifdef DMP_DEVICE2_TRACEROUTE_1
CmsRet rcl_dev2IpDiagTraceRouteObject( _Dev2IpDiagTraceRouteObject *newObj,
                                     const _Dev2IpDiagTraceRouteObject *currObj,
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS;   
   char cmdLine[BUFLEN_128];
   char if_name_cmd[BUFLEN_32] = "-i ";
   unsigned int timeout;
   int pid;

   InstanceIdStack routeHopsIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2IpDiagTraceRouteRouteHopsObject *routeHopsObj = NULL;

   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    */
   if (ADD_NEW(newObj, currObj))
   {
      return ret;
   }

   if (newObj == NULL)
      return CMSRET_INTERNAL_ERROR;

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

   /* this object will be updated more than once, but this part should only be
    * executed when the diagnosticsState be set from None to Requested or 
    * from Complete to Requested. */
   if ((cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0) &&
       (cmsUtl_strcmp(currObj->diagnosticsState,MDMVS_REQUESTED) != 0))
   {
      cmsLog_notice("Sending message to SMD to start doing traceroute");
      cmsLog_debug("newObj->diagnosticsState %s, interface %s maxhops %d, dscp %d, host %s",
                   newObj->diagnosticsState, newObj->interface, (int)newObj->numberOfTries,
                   newObj->DSCP,newObj->host);

      /* now checks to see if we have all the required parameter or not */
      if (IS_EMPTY_STRING(newObj->host))
      {
         *errorParam = cmsMem_strdupFlags("Host",mdmLibCtx.allocFlags);
         *errorCode = CMSRET_INVALID_PARAM_VALUE;
         return CMSRET_INVALID_ARGUMENTS;
      }

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

      /* the timeout resolution of traceroute is 1 second */
      timeout = (newObj->timeout > 1000)? newObj->timeout/1000 : 1;

      if (mdmLibCtx.eid != EID_TR69C && mdmLibCtx.eid != EID_CWMPD)
      {
         sprintf(cmdLine, "%s -w %u -q %u -t %u -m %d -M %s %u", if_name_cmd, 
                timeout, newObj->numberOfTries, (newObj->DSCP * 2), 
                newObj->maxHopCount, newObj->host, newObj->dataBlockSize);
      }
      else
      {
         sprintf(cmdLine, "%s -w %u -q %u -t %u -m %d -M -R %u %s %u", if_name_cmd, 
                timeout, newObj->numberOfTries, (newObj->DSCP * 2), 
                newObj->maxHopCount, mdmLibCtx.eid, newObj->host, newObj->dataBlockSize);
      }

      cmsLog_debug("traceroute command string=%s", cmdLine);
   
      /*
       * Before we start a new traceroute diagnostic,
       * that we should clear the results from
       * the previous diagnostic.
       */
      newObj->responseTime = 0;

      /* delete all routehops object. */
      while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_IP_DIAG_TRACE_ROUTE_ROUTE_HOPS, &routeHopsIidStack, 0, (void **)&routeHopsObj)) == CMSRET_SUCCESS)
      {
         cmsObj_deleteInstance(MDMOID_DEV2_IP_DIAG_TRACE_ROUTE_ROUTE_HOPS, &routeHopsIidStack);
         cmsObj_free((void **) &routeHopsObj);
         INIT_INSTANCE_ID_STACK(&routeHopsIidStack);
      }
      newObj->routeHopsNumberOfEntries = 0;
      
      pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_TRACERT, cmdLine, strlen(cmdLine)+1);
      
      if (pid == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start TRACERT test.");
         ret = CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("Start TRACERT msg sent, new TRACERT pid=%d", pid);
      }
   } /* requested */
   
   return CMSRET_SUCCESS;
};


CmsRet rcl_dev2IpDiagTraceRouteRouteHopsObject( _Dev2IpDiagTraceRouteRouteHopsObject *newObj,
                                     const _Dev2IpDiagTraceRouteRouteHopsObject *currObj,
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
   Dev2IpDiagTraceRouteObject *tracertObj = NULL;
   InstanceIdStack accentIidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 flags=OGF_NO_VALUE_UPDATE;
   CmsRet ret;

   if (ADD_NEW(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_TRACE_ROUTE, &accentIidStack, flags, (void **) &tracertObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IpDiagTraceRoute object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      tracertObj->routeHopsNumberOfEntries ++;

      if ((ret = cmsObj_set(tracertObj, &accentIidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set IpDiagTraceRoute object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }
   
      cmsObj_free((void **) &tracertObj);
   }
   return CMSRET_SUCCESS;
};

#endif /* DMP_DEVICE2_TRACEROUTE_1 */

#ifdef DMP_DEVICE2_UPLOAD_1
CmsRet rcl_dev2IpDiagUploadObject( _Dev2IpDiagUploadObject *newObj,
                                   const _Dev2IpDiagUploadObject *currObj,
                                   const InstanceIdStack *iidStack __attribute__((unused)),
                                   char **errorParam __attribute__((unused)),
                                   CmsRet *errorCode __attribute__((unused)))
{
   char buf[BUFLEN_512]={0};
   int pid, loglevel;
   char if_name_cmd[BUFLEN_32] = "-i ";
   
   InstanceIdStack perConnResultIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2UploadPerConnResultObject *perConnResultObj = NULL;

   cmsLog_debug("===> Enter");

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
      
      loglevel = newObj->X_BROADCOM_COM_LogLevel;
      if (loglevel != LOG_LEVEL_ERR && loglevel != LOG_LEVEL_NOTICE && loglevel != LOG_LEVEL_DEBUG)
         loglevel = DEFAULT_LOG_LEVEL;

      // reset test results
      unlink(TR143_UPLOAD_RESULT_FILE);

      /** delete all routehops object. 
       * TO-DO: TR-143 also request to reset the nextInstanceId 
       * */
      while (cmsObj_getNextFlags(MDMOID_DEV2_UPLOAD_PER_CONN_RESULT, &perConnResultIidStack, 0, (void **)&perConnResultObj) == CMSRET_SUCCESS)
      {
         cmsObj_deleteInstance(MDMOID_DEV2_UPLOAD_PER_CONN_RESULT, &perConnResultIidStack);
         cmsObj_free((void **) &perConnResultObj);
         INIT_INSTANCE_ID_STACK(&perConnResultIidStack);
      }
      newObj->perConnectionResultNumberOfEntries= 0;

      // reset the next generated instance ID.
      MdmObjectNode *objNode;
      if ((objNode = mdm_getObjectNode(MDMOID_DEV2_UPLOAD_PER_CONN_RESULT)) != NULL)
      {
         InstanceHeadNode *instHead = (InstanceHeadNode *) objNode->objData;
         if (instHead != NULL)
            instHead->nextInstanceIdToAssign = 1;
      }

      if (newObj->enablePerConnectionResults == TRUE) 
      {
         if (newObj->numberOfConnections > newObj->uploadDiagnosticsMaxConnections)
         {
            cmsLog_error("numberOfConnections(%u) must not set greater than MaxConnections(%u)",
                         newObj->numberOfConnections, newObj->uploadDiagnosticsMaxConnections);
            return CMSRET_INVALID_ARGUMENTS;
         }
         else
            snprintf(buf, sizeof(buf), "%s -c %d -d %d -l %d -u %s -D %d", if_name_cmd, newObj->numberOfConnections, newObj->DSCP, newObj->testFileLength, newObj->uploadURL, loglevel);
      }
      else
         snprintf(buf, sizeof(buf), "%s -d %d -l %d -u %s -D %d", if_name_cmd, newObj->DSCP, newObj->testFileLength, newObj->uploadURL, loglevel);


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
      // Stop Running test
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
      if ((mdmLibCtx.eid == EID_TR69C || mdmLibCtx.eid == EID_CWMPD) && 
           strcmp(newObj->diagnosticsState, currObj->diagnosticsState))
         return CMSRET_INVALID_PARAM_VALUE;
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2UploadPerConnResultObject( _Dev2UploadPerConnResultObject *newObj,
                                          const _Dev2UploadPerConnResultObject *currObj,
                                          const InstanceIdStack *iidStack __attribute__((unused)),
                                          char **errorParam __attribute__((unused)),
                                          CmsRet *errorCode __attribute__((unused)))
{
   Dev2IpDiagUploadObject *uploadObj = NULL;
   InstanceIdStack accentIidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 flags=OGF_NO_VALUE_UPDATE;
   CmsRet ret;

   cmsLog_debug("===> Enter");
   if (ADD_NEW(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_UPLOAD, &accentIidStack, flags, (void **) &uploadObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IpDiagUpload object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      uploadObj->perConnectionResultNumberOfEntries ++;

      if ((ret = cmsObj_setFlags(uploadObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set IpDiagUpload object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }
   
      cmsObj_free((void **) &uploadObj);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_DOWNLOAD, &accentIidStack, flags, (void **) &uploadObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IpDiagUpload object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      uploadObj->perConnectionResultNumberOfEntries --;

      if ((ret = cmsObj_setFlags(uploadObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set IpDiagUpload object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      cmsObj_free((void **) &uploadObj);
   }
   return CMSRET_SUCCESS;
};


#endif /* DMP_DEVICE2_UPLOAD_1 */

#ifdef DMP_DEVICE2_DOWNLOAD_1
CmsRet rcl_dev2IpDiagDownloadObject( _Dev2IpDiagDownloadObject *newObj,
                                     const _Dev2IpDiagDownloadObject *currObj,
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
   char buf[BUFLEN_512]={0};
   int pid, loglevel;
   char if_name_cmd[BUFLEN_32] = "-i ";
   
   InstanceIdStack perConnResultIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2DownloadPerConnResultObject *perConnResultObj = NULL;

   cmsLog_debug("===> Enter");

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

      loglevel = newObj->X_BROADCOM_COM_LogLevel;
      if (loglevel != LOG_LEVEL_ERR && loglevel != LOG_LEVEL_NOTICE && loglevel != LOG_LEVEL_DEBUG)
         loglevel = DEFAULT_LOG_LEVEL;

      // reset test results
      unlink(TR143_DOWNLOAD_RESULT_FILE);

      /** delete all routehops object. 
       * TO-DO: TR-143 also request to reset the nextInstanceId 
       * */
      while (cmsObj_getNextFlags(MDMOID_DEV2_DOWNLOAD_PER_CONN_RESULT, &perConnResultIidStack, 0, (void **)&perConnResultObj) == CMSRET_SUCCESS)
      {
         cmsObj_deleteInstance(MDMOID_DEV2_DOWNLOAD_PER_CONN_RESULT, &perConnResultIidStack);
         cmsObj_free((void **) &perConnResultObj);
         INIT_INSTANCE_ID_STACK(&perConnResultIidStack);
      }
      newObj->perConnectionResultNumberOfEntries= 0;

      // reset the next generated instance ID.
      MdmObjectNode *objNode;
      if ((objNode = mdm_getObjectNode(MDMOID_DEV2_DOWNLOAD_PER_CONN_RESULT)) != NULL)
      {
         InstanceHeadNode *instHead = (InstanceHeadNode *) objNode->objData;
         if (instHead != NULL)
            instHead->nextInstanceIdToAssign = 1;
      }

      if (newObj->enablePerConnectionResults == TRUE) 
      {
         if (newObj->numberOfConnections > newObj->downloadDiagnosticsMaxConnections)
         {
            cmsLog_error("numberOfConnections(%u) must not set greater than MaxConnections(%u)",
                         newObj->numberOfConnections, newObj->downloadDiagnosticsMaxConnections);
            return CMSRET_INVALID_ARGUMENTS;
         }
         else
            snprintf(buf, sizeof(buf), "%s -c %d -d %d -u %s -D %d", if_name_cmd, newObj->numberOfConnections, newObj->DSCP, newObj->downloadURL, loglevel);
      }
      else
         snprintf(buf, sizeof(buf), "%s -d %d -u %s -D %d", if_name_cmd, newObj->DSCP, newObj->downloadURL, loglevel);


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
            cmsLog_error("failed to stop download diag test.");
            return CMSRET_INTERNAL_ERROR;
         }
         cmsLog_debug("Stop download diag msg sent");
      }

      // ACS server try to set a invalid value into diagnosticsState parameter.
      if ((mdmLibCtx.eid == EID_TR69C || mdmLibCtx.eid == EID_CWMPD) && 
           strcmp(newObj->diagnosticsState, currObj->diagnosticsState))
         return CMSRET_INVALID_PARAM_VALUE;
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DownloadPerConnResultObject( _Dev2DownloadPerConnResultObject *newObj,
                                            const _Dev2DownloadPerConnResultObject *currObj,
                                            const InstanceIdStack *iidStack __attribute__((unused)),
                                            char **errorParam __attribute__((unused)),
                                            CmsRet *errorCode __attribute__((unused)))
{
   Dev2IpDiagDownloadObject *downloadObj = NULL;
   InstanceIdStack accentIidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 flags=OGF_NO_VALUE_UPDATE;
   CmsRet ret;

   cmsLog_debug("===> Enter");
   if (ADD_NEW(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_DOWNLOAD, &accentIidStack, flags, (void **) &downloadObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IpDiagDownload object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      downloadObj->perConnectionResultNumberOfEntries ++;

      if ((ret = cmsObj_setFlags(downloadObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set IpDiagDownload object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }
   
      cmsObj_free((void **) &downloadObj);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      if ((ret = cmsObj_get(MDMOID_DEV2_IP_DIAG_DOWNLOAD, &accentIidStack, flags, (void **) &downloadObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IpDiagDownload object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      downloadObj->perConnectionResultNumberOfEntries --;

      if ((ret = cmsObj_setFlags(downloadObj, &accentIidStack, OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not set IpDiagDownload object, ret=%d", ret);
         return CMSRET_INTERNAL_ERROR;
      }

      cmsObj_free((void **) &downloadObj);
   }
   return CMSRET_SUCCESS;
};

#endif /* DMP_DEVICE2_DOWNLOAD_1 */


#ifdef DMP_DEVICE2_UDPECHO_1
CmsRet rcl_dev2IpDiagUDPEchoConfigObject( _Dev2IpDiagUDPEchoConfigObject *newObj,
                                          const _Dev2IpDiagUDPEchoConfigObject *currObj,
                                          const InstanceIdStack *iidStack __attribute__((unused)),
                                          char **errorParam __attribute__((unused)),
                                          CmsRet *errorCode __attribute__((unused)))
{
   int pid;
   UINT32 l;
   char buf[BUFLEN_512]={0};
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
#endif /* DMP_DEVICE2_UDPECHO_1 */

#ifdef DMP_DEVICE2_UDPECHODIAG_1
CmsRet rcl_dev2UDPEchoDiagObject( _Dev2UDPEchoDiagObject *newObj,
                                  const _Dev2UDPEchoDiagObject *currObj,
                                  const InstanceIdStack *iidStack __attribute__((unused)),
                                  char **errorParam __attribute__((unused)),
                                  CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#endif    /* DMP_DEVICE2_BASELINE_1 */
