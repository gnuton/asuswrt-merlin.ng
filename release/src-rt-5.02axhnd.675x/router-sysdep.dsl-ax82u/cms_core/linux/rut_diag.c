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

#include "cms_util.h"
#include "cms_core.h"
#include "rut_util.h"
#include "rut_diag.h"
#include "mdm.h"
#include "devctl_atm.h"

#ifdef DMP_X_BROADCOM_COM_ETHERNETOAM_1
#include "prctl.h"
#include "odl.h"
#endif

#ifdef DMP_ADSLWAN_1


CmsRet rutDiag_runAtmOamLoopbackTest(int type, void *new,
                                   const void *curr __attribute__((unused)),
                                   const InstanceIdStack *iidStack)
{
   WanDslLinkCfgObject *dslLinkCfg=NULL;
   CmsRet ret = CMSRET_INVALID_ARGUMENTS;
   SINT32 vpi, vci, port;
   ATM_VCC_ADDR vccAddr;
   ATMDRV_OAM_LOOPBACK results;

   WanAtm5LoopbackDiagObject *newObj = (WanAtm5LoopbackDiagObject*)new;
   //WanAtm5LoopbackDiagObject *currObj = (WanAtm5LoopbackDiagObject*)curr;


   /*
    * Now we must be dealing with runtime modification of this object.
    * Check for error conditions first.
    */
   /*if ((cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) == 0) &&
       (cmsUtl_strcmp(currObj->diagnosticsState, MDMVS_REQUESTED) == 0))
   {
      cmsLog_debug("Cannot start OAM Loopback on while it is currently running.");
      return CMSRET_INVALID_ARGUMENTS;
   }*/

   if (cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0)
   {
      cmsLog_debug("newObj->diagnosticsState %s, repetitions %d, timeout %d",
                   newObj->diagnosticsState,(int)newObj->numberOfRepetitions,
                   newObj->timeout);


      /* Get the VPI/VCI info */
      if ( cmsObj_get(MDMOID_WAN_DSL_LINK_CFG, iidStack, 0, (void **) &dslLinkCfg) == CMSRET_SUCCESS )
      {
         port = dslLinkCfg->X_BROADCOM_COM_ATMInterfaceId;
         cmsUtl_atmVpiVciStrToNum(dslLinkCfg->destinationAddress, &vpi, &vci);
         cmsObj_free((void **) &dslLinkCfg);


         vccAddr.ulInterfaceId = port;
         vccAddr.usVpi = vpi;
         if (type == OAM_F4_LB_SEGMENT_TYPE)
         {
            vccAddr.usVci = VCI_OAM_F4_SEGMENT;
         }

         else if (type == OAM_F4_LB_END_TO_END_TYPE)
         {
            vccAddr.usVci = VCI_OAM_F4_END_TO_END;
         }
         else
         {
            vccAddr.usVci = vci;
         }

         ret = devCtl_atmSendOamLoopbackTest(type,&vccAddr,newObj->numberOfRepetitions,
                                             newObj->timeout, &results);

         /* Assuming we only do OAM loopback test with very small iteration (1);
          * we are blocking and waiting for the test result, update MDM immediately.
          * However, if when bigger number of repetition of OAM loopback test is required,
          * we would need to keep the result at the ATM driver level; periodically wakeUpMonitor
          * SSK which then calls the driver to get the current OAM test result, and then update
          * the MDM.   We cannot just block here and wait for results.
          */

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState,MDMVS_COMPLETE, mdmLibCtx.allocFlags);
         newObj->successCount = results.received;
         newObj->failureCount = (results.sent - results.received);
         newObj->averageResponseTime = results.avgResponseTime;
         newObj->minimumResponseTime = results.minResponseTime;
         newObj->maximumResponseTime = results.maxResponseTime;

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
         }/* send msg_diag if tr69c initiated this test */
      } /* dslLinkCfg */
   } /* requested */
   else
   {
      /* don't do anything if not requested */
      ret = CMSRET_SUCCESS;
   }
   return (ret);

}

#endif  /* DMP_ADSLWAN_1 */


#ifdef DMP_X_BROADCOM_COM_ETHERNETOAM_1
void rutEthOam_set3ahService(UBOOL8 enabled)
{
   InstanceIdStack ieee8023ahCfgIidStack;
   Ieee8023ahCfgObject *ieee8023ahCfg = NULL;
   char cmd[BUFLEN_1024];
   char ifNameBuf[BUFLEN_32];
   char oamIdBuf[BUFLEN_32];
   char variableRetrievalBuf[BUFLEN_32];
   char linkEventsBuf[BUFLEN_32];
   char remoteLoopbackBuf[BUFLEN_32];
   char activeModeBuf[BUFLEN_32];
   char autoEventBuf[BUFLEN_32];
   CmsRet ret = CMSRET_SUCCESS;
   int pid;
   UINT32 specificEid;

   cmd[0] = ifNameBuf[0] = oamIdBuf[0] = variableRetrievalBuf[0] = '\0';
   linkEventsBuf[0] = remoteLoopbackBuf[0] = activeModeBuf[0] = autoEventBuf[0] = '\0';

   cmsLog_debug("enter. enabled=%d", enabled);
   
   INIT_INSTANCE_ID_STACK(&ieee8023ahCfgIidStack);
   if ((ret = cmsObj_get(MDMOID_IEEE8023AH_CFG, &ieee8023ahCfgIidStack, 0, (void **)&ieee8023ahCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_get <MDMOID_IEEE8023AH_CFG> returns error. ret=%d", ret);
      return;
   }

   snprintf(cmd, sizeof(cmd), "tmsctl 3ah stop");
   rut_doSystemAction("rutEthOam_set3ahService", cmd);

   specificEid = MAKE_SPECIFIC_EID(ieee8023ahCfg->tmsctl3ahPid, EID_TMSCTL); 

   if(enabled && ieee8023ahCfg->enabled)
   {
      if(ieee8023ahCfg->ifName)
      {
         snprintf(ifNameBuf, sizeof(ifNameBuf)," -i %s", ieee8023ahCfg->ifName);
      }
      snprintf(oamIdBuf, sizeof(oamIdBuf), " -m %d", ieee8023ahCfg->oamId);
      if(ieee8023ahCfg->variableRetrievalEnabled)
      {
         snprintf(variableRetrievalBuf, sizeof(variableRetrievalBuf), "+vr");
      }
      if(ieee8023ahCfg->linkEventsEnabled)
      {
         snprintf(linkEventsBuf, sizeof(linkEventsBuf), "+le");
      }
      if(ieee8023ahCfg->remoteLoopbackEnabled)
      {
         snprintf(remoteLoopbackBuf, sizeof(remoteLoopbackBuf), "+lb");
      }
      if(ieee8023ahCfg->activeModeEnabled)
      {
         snprintf(activeModeBuf, sizeof(activeModeBuf), "+ac");
      }
      if(ieee8023ahCfg->autoEventEnabled)
      {
         snprintf(autoEventBuf, sizeof(autoEventBuf), " -e");
      }
      snprintf(cmd, sizeof(cmd), "3ah start%s%s%s -f %s%s%s%s",
         ifNameBuf, oamIdBuf, autoEventBuf,
         variableRetrievalBuf, linkEventsBuf, remoteLoopbackBuf, activeModeBuf);
      pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, specificEid, cmd, strlen(cmd)+1);
      if (pid == CMS_INVALID_PID)
      {
         cmsLog_error("Failed to restart tmsctl 3ah mode.");
         ret = CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("tmsctl 3ah mode is restarted, pid=%d.", pid);
         ieee8023ahCfg->tmsctl3ahPid = pid;
         if ((ret = cmsObj_setFlags((void *)ieee8023ahCfg,
                                    &ieee8023ahCfgIidStack,
                                    OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_setFlags ieee8023ahCfg error %d", ret);
         }
      }
   }
   else
   {
      if(ieee8023ahCfg->tmsctl3ahPid != CMS_INVALID_PID)
      {
         if ((ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0)) != CMSRET_SUCCESS)
         {
            cmsLog_debug("Failed to stop tmsctl 3ah mode.");
         }
         else
         {
            cmsLog_debug("tmsctl 3ah mode is stopped.");
         }
      }
   }
   cmsObj_free((void **) &ieee8023ahCfg);
   return;
}

void rutEthOam_set1agService(UBOOL8 enabled)
{
   InstanceIdStack ieee8021agCfgIidStack;
   Ieee8021agCfgObject *ieee8021agCfg = NULL;
   InstanceIdStack localMepIidStack;
   LocalMepObject *localMep = NULL;
   InstanceIdStack remoteMepIidStack;
   RemoteMepObject *remoteMep = NULL;
   UBOOL8 enbl1ag = FALSE;
   UBOOL8 enbl1731 = FALSE;
   char cmd[BUFLEN_1024];
   char mdIdBuf[BUFLEN_32];
   char mdLvlBuf[BUFLEN_32];
   char maIdBuf[BUFLEN_32];
   char megIdBuf[BUFLEN_32];
   char ccmIntervalBuf[BUFLEN_32];
   char locIntfBuf[BUFLEN_32];
   char locMepIdBuf[BUFLEN_32];
   char locVlanIdBuf[BUFLEN_32];
   char locCcmEnblBuf[BUFLEN_64];
   char remMepIdBuf[BUFLEN_32];
   CmsRet ret = CMSRET_SUCCESS;
   int pid;
   UINT32 specificEid;

   cmd[0] = mdIdBuf[0] = mdLvlBuf[0] = ccmIntervalBuf[0] = locIntfBuf[0] = '\0';
   locMepIdBuf[0] = locVlanIdBuf[0] = locCcmEnblBuf[0] = remMepIdBuf[0] = '\0';

   cmsLog_debug("enter. enabled=%d", enabled);

   /* MDMOID_IEEE8021AG_CFG */
   INIT_INSTANCE_ID_STACK(&ieee8021agCfgIidStack);
   if ((ret = cmsObj_get(MDMOID_IEEE8021AG_CFG, &ieee8021agCfgIidStack, 0, (void **)&ieee8021agCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_get <MDMOID_IEEE8021AG_CFG> returns error. ret=%d", ret);
      return;
   }
   enbl1ag = ieee8021agCfg->enabled;
   enbl1731 = ieee8021agCfg->Y1731Enabled;
   if(ieee8021agCfg->mdId)
   {
      snprintf(mdIdBuf, sizeof(mdIdBuf)," -d %s", ieee8021agCfg->mdId);
      snprintf(megIdBuf, sizeof(megIdBuf)," -a %s", ieee8021agCfg->mdId);
   }
   snprintf(mdLvlBuf, sizeof(mdLvlBuf), " -l %d", ieee8021agCfg->mdLevel);
   if(ieee8021agCfg->maId)
   {
      snprintf(maIdBuf, sizeof(maIdBuf)," -a %s", ieee8021agCfg->maId);
   }
   snprintf(ccmIntervalBuf, sizeof(ccmIntervalBuf), " -t %d", ieee8021agCfg->ccmInterval);

   /* MDMOID_LOCAL_MEP */
   INIT_INSTANCE_ID_STACK(&localMepIidStack);
   if ((ret = cmsObj_getNext(MDMOID_LOCAL_MEP, &localMepIidStack, (void **)&localMep)) != CMSRET_SUCCESS)
   {
      //cmsLog_error("cmsObj_getNext <MDMOID_LOCAL_MEP> returns error. ret=%d", ret);
      cmsObj_free((void **) &ieee8021agCfg);
      return;
   }
   if(localMep->ifName)
   {
      snprintf(locIntfBuf, sizeof(locIntfBuf), " -i %s", localMep->ifName);
   }
   snprintf(locMepIdBuf, sizeof(locMepIdBuf), " -m %d", localMep->mepId);
   if(localMep->vlanId > 0)
   {
      snprintf(locVlanIdBuf, sizeof(locVlanIdBuf), " -v %d", localMep->vlanId);
   }
   if(localMep->ccmEnabled)
   {
      snprintf(locCcmEnblBuf, sizeof(locCcmEnblBuf), " -s ccm%s", ccmIntervalBuf);
   }
   cmsObj_free((void **) &localMep);

   /* MDMOID_REMOTE_MEP */
   INIT_INSTANCE_ID_STACK(&remoteMepIidStack);
   if ((ret = cmsObj_getNext(MDMOID_REMOTE_MEP, &remoteMepIidStack, (void **)&remoteMep)) != CMSRET_SUCCESS)
   {
      //cmsLog_error("cmsObj_getNext <MDMOID_REMOTE_MEP> returns error. ret=%d", ret);
      cmsObj_free((void **) &ieee8021agCfg);
      return;
   }
   if(remoteMep->mepId > 0)
   {
      snprintf(remMepIdBuf, sizeof(remMepIdBuf), " -r %d", remoteMep->mepId);
   }
   cmsObj_free((void **) &remoteMep); 

   snprintf(cmd, sizeof(cmd), "tmsctl 1731 stop");
   rut_doSystemAction("rutEthOam_set1agService", cmd);

   snprintf(cmd, sizeof(cmd), "tmsctl 1ag stop");
   rut_doSystemAction("rutEthOam_set1agService", cmd);

   specificEid = MAKE_SPECIFIC_EID(ieee8021agCfg->tmsctl1agPid, EID_TMSCTL); 

   if(enabled && enbl1ag)
   {   
      if(enbl1731)
      {
         snprintf(cmd, sizeof(cmd), "1731 start%s%s%s%s%s%s%s",
            locIntfBuf, megIdBuf, mdLvlBuf, locMepIdBuf,            
            locVlanIdBuf, locCcmEnblBuf, remMepIdBuf);
      }
      else
      {
         snprintf(cmd, sizeof(cmd), "1ag start%s%s%s%s%s%s%s%s",
            locIntfBuf, mdIdBuf, mdLvlBuf, maIdBuf, locMepIdBuf,            
            locVlanIdBuf, locCcmEnblBuf, remMepIdBuf);
      }
      pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, specificEid, cmd, strlen(cmd)+1);
      if (pid == CMS_INVALID_PID)
      {
         cmsLog_error("Failed to restart tmsctl 1ag/1731 mode.");
         ret = CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("tmsctl 1ag/1731 mode is restarted, pid=%d.", pid);
         ieee8021agCfg->tmsctl1agPid = pid;
         if ((ret = cmsObj_setFlags((void *)ieee8021agCfg,
                                    &ieee8021agCfgIidStack,
                                    OSF_NO_RCL_CALLBACK)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_setFlags ieee8021agCfg error %d", ret);
         }
      }
   }
   else
   {
      if(ieee8021agCfg->tmsctl1agPid != CMS_INVALID_PID)
      {
         if ((ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0)) != CMSRET_SUCCESS)
         {
            cmsLog_debug("Failed to stop tmsctl 1ag/1731 mode.");
         }
         else
         {
            cmsLog_debug("tmsctl 1ag/1731 mode is stopped.");
         }
      }
   }

   cmsObj_free((void **) &ieee8021agCfg);
   return;
}
#endif /* DMP_X_BROADCOM_COM_ETHERNETOAM_1 */
