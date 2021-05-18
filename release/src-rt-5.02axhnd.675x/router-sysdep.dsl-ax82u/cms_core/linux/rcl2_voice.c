/***********************************************************************
# <:copyright-BRCM:2011:proprietary:standard
# 
#    Copyright (c) 2011 Broadcom 
#    All Rights Reserved
# 
#  This program is the proprietary software of Broadcom and/or its
#  licensors, and may only be used, duplicated, modified or distributed pursuant
#  to the terms and conditions of a separate, written license agreement executed
#  between you and Broadcom (an "Authorized License").  Except as set forth in
#  an Authorized License, Broadcom grants no license (express or implied), right
#  to use, or waiver of any kind with respect to the Software, and Broadcom
#  expressly reserves all rights in and to the Software and all intellectual
#  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
#  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
#  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
# 
#  Except as expressly set forth in the Authorized License,
# 
#  1. This program, including its structure, sequence and organization,
#     constitutes the valuable trade secrets of Broadcom, and you shall use
#     all reasonable efforts to protect the confidentiality thereof, and to
#     use this information only in connection with your use of Broadcom
#     integrated circuit products.
# 
#  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
#     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
#     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
#     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
#     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
#     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
#     PERFORMANCE OF THE SOFTWARE.
# 
#  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
#     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
#     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
#     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
#     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
#     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
#     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
#     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
#     LIMITED REMEDY.
# :>
************************************************************************/

#ifdef BRCM_VOICE_SUPPORT
#ifdef DMP_VOICE_SERVICE_2
#include <time.h>
#include "rcl.h"
#include "cms_util.h"
#include "rut_util.h"
#include "mdm.h"
#include "rut2_voice.h"
#include "dect_msg.h"
#include "sys/time.h"
#include "rut_wan.h"


#include <cms_log.h>
#include <cms_msg.h>

/* mwang_todo: these handler function should be put in more precise
 * DMP_<profilename> defines.
 */


CmsRet rcl_voiceObject( _VoiceObject *newObj,
                const _VoiceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
       /* remove voiceServiceNumberOfEntries counter because
        * TR98 data model doesn't support counter
        */
   }
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If loglevel changed just inform voice and return */
      if ( cmsUtl_strcmp(newObj->X_BROADCOM_COM_LoggingLevel, currObj->X_BROADCOM_COM_LoggingLevel) )
      {
         sendLoggingChangeMsgToVoice(0);
         return CMSRET_SUCCESS;
      }
      else if ( cmsUtl_strcmp(newObj->X_BROADCOM_COM_ModuleLogLevels, currObj->X_BROADCOM_COM_ModuleLogLevels) )
      {
         sendLoggingChangeMsgToVoice(1);
         return CMSRET_SUCCESS;
      }
      else if ( cmsUtl_strcmp(newObj->X_BROADCOM_COM_CCTKTraceLevel, currObj->X_BROADCOM_COM_CCTKTraceLevel) )
      {
         sendLoggingChangeMsgToVoice(1);
         return CMSRET_SUCCESS;
      }
      else if ( cmsUtl_strcmp(newObj->X_BROADCOM_COM_CCTKTraceGroup, currObj->X_BROADCOM_COM_CCTKTraceGroup) )
      {
         sendLoggingChangeMsgToVoice(1);
         return CMSRET_SUCCESS;
      }

      if ( cmsUtl_strcmp(newObj->X_BROADCOM_COM_BoundIpAddr, currObj->X_BROADCOM_COM_BoundIpAddr) )
      {
         CmsMsgHeader msg=EMPTY_MSG_HEADER;
         if ( rutIsVoipRunning() )
         {
            if( (newObj->X_BROADCOM_COM_BoundIpAddr == NULL) ||
                (cmsUtl_strcmp(newObj->X_BROADCOM_COM_BoundIpAddr, "0.0.0.0") == 0) )
            {
               /* boundIp has been reset, shutdown voice */
               msg.type = CMS_MSG_SHUTDOWN_VOICE;
               cmsLog_debug("boundIP changed to 0.0.0.0 or NULL --> voice stop required\n");
            }
            else
            {
               /* ipaddress has been changed, restart voice */
#if defined(BRCM_PKTCBL_SUPPORT)
               /* need to restart voice to trigger config file re-download */
               msg.type = CMS_MSG_REBOOT_VOICE;
#else
               msg.type = CMS_MSG_RESTART_VOICE_CALLMGR;
#endif
               cmsLog_debug("Voice is running and boundIP changed --> restart required\n");
            }
         }
         else
         {
            if( (newObj->X_BROADCOM_COM_BoundIpAddr != NULL) &&
                (cmsUtl_strcmp(newObj->X_BROADCOM_COM_BoundIpAddr, "0.0.0.0") != 0) )
            {
               /* Voip is not running and valid boundIP was set so Start voice */
               msg.type = CMS_MSG_START_VOICE;
               cmsLog_debug("Voice not running --> start required\n ");
            }
            else
            {
               cmsLog_error("Invalid or NULL X_BROADCOM_COM_BoundIpAddr! \n ");
            }
         }

         msg.src = mdmLibCtx.eid;
         msg.dst = EID_SSK;
         msg.flags_event = 1;

         if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not send START/RESTART msg to ssk, ret=%d", ret);
         }
      }
      else if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
   }
   if ((newObj == NULL && currObj != NULL)) /* delete */
   {
       /* remove voiceServiceNumberOfEntries counter because
        * TR98 data model doesn't support counter
        */
   }

   return ret;
}


CmsRet rcl_voiceCapObject( _VoiceCapObject *newObj,
                const _VoiceCapObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCapSipObject( _VoiceCapSipObject *newObj,
                const _VoiceCapSipObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCapSipClientObject( _VoiceCapSipClientObject *newObj,
                const _VoiceCapSipClientObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCapPotsObject( _VoiceCapPotsObject *newObj,
                const _VoiceCapPotsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_voiceCapCodecsObject
**
**  PURPOSE:        Adds, modifies or deletes a voice capabilities object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_voiceCapCodecsObject( _VoiceCapCodecsObject *newObj,
                const _VoiceCapCodecsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceCapObject * vObj = NULL;
    InstanceIdStack localIidStack = EMPTY_INSTANCE_ID_STACK;

    localIidStack = *iidStack;
    POP_INSTANCE_ID( &localIidStack ); /* switch to Voice Service level */

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_get( MDMOID_VOICE_CAP, &localIidStack, 0, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->codecNumberOfEntries++;
            cmsObj_set((const void *)vObj, &localIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get object, ret =%d\n", ret);
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_get( MDMOID_VOICE_CAP, &localIidStack, 0, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->codecNumberOfEntries--;
            cmsObj_set((const void *)vObj, &localIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get object, ret =%d\n", ret);
        }
    }
 
    return ret;
}

CmsRet rcl_qualityIndicatorObject( _QualityIndicatorObject *newObj,
                const _QualityIndicatorObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceReservedPortsObject( _VoiceReservedPortsObject *newObj,
                const _VoiceReservedPortsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceServicePotsObject( _VoiceServicePotsObject *newObj,
                const _VoiceServicePotsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_potsRingerObject( _PotsRingerObject *newObj,
                const _PotsRingerObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_pOTSFxoObject
**
**  PURPOSE:        Adds, modifies or deletes a POTS FXO object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_pOTSFxoObject( _POTSFxoObject *newObj,
                const _POTSFxoObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceServicePotsObject *vObj = NULL;
    InstanceIdStack localIidStack = EMPTY_INSTANCE_ID_STACK;

    localIidStack = *iidStack;
    POP_INSTANCE_ID( &localIidStack ); /* switch to Voice Service level */

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_get( MDMOID_VOICE_SERVICE_POTS, &localIidStack, 0, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->FXONumberOfEntries++;
            cmsObj_set((const void *)vObj, &localIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get object, ret =%d\n", ret);
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_get( MDMOID_VOICE_SERVICE_POTS, &localIidStack, 0, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->FXONumberOfEntries--;
            cmsObj_set((const void *)vObj, &localIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get object, ret =%d\n", ret);
        }
    }
 
    return ret;
}

/*****************************************************************************
**  FUNCTION:       rcl_pOTSFxsObject
**
**  PURPOSE:        Adds, modifies or deletes a POTS FXS object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_pOTSFxsObject( _POTSFxsObject *newObj,
                const _POTSFxsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceServicePotsObject *vObj = NULL;
    InstanceIdStack localIidStack = EMPTY_INSTANCE_ID_STACK;

    localIidStack = *iidStack;
    POP_INSTANCE_ID( &localIidStack ); /* switch to Voice Service level */

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_get( MDMOID_VOICE_SERVICE_POTS, &localIidStack, 0, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->FXSNumberOfEntries++;
            cmsObj_set((const void *)vObj, &localIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get object, ret =%d\n", ret);
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_get( MDMOID_VOICE_SERVICE_POTS, &localIidStack, 0, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->FXSNumberOfEntries--;
            cmsObj_set((const void *)vObj, &localIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get object, ret =%d\n", ret);
        }
    }
 
    return ret;
}

CmsRet rcl_voiceProcessingObject( _VoiceProcessingObject *newObj,
                const _VoiceProcessingObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_diagTestsObject( _DiagTestsObject *newObj,
                const _DiagTestsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet   ret;

    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        if( cmsUtl_strcmp(newObj->diagnosticsState, currObj->diagnosticsState) )
        {
            if( !cmsUtl_strcmp(newObj->diagnosticsState, MDMVS_REQUESTED) && 
                 mdmLibCtx.eid != EID_VOICE )
            {
                /* setting diagnosticsState to "Requested" will trigger MLT
                 * testing by sending request message to voice application
                 */
                int  fxsNo;
                char buf[sizeof(CmsMsgHeader) + sizeof(VoiceDiagMsgBody)];
                
                CmsMsgHeader *msg = (CmsMsgHeader *) buf;
                VoiceDiagMsgBody *info = (VoiceDiagMsgBody *) &(buf[sizeof(CmsMsgHeader)]);

                memset(buf, 0, sizeof(buf));
                msg = (CmsMsgHeader *) buf;
                info = (VoiceDiagMsgBody *) &(buf[sizeof(CmsMsgHeader)]);
                /* setup messege header */
                msg->type = CMS_MSG_VOICE_DIAG;
                msg->src = mdmLibCtx.eid;
                msg->dst = EID_VOICE;
                msg->flags_request = 1;
                msg->dataLength = sizeof(VoiceDiagMsgBody);

                /* setup message body */
                rutVoice_mapL2ObjInstToNum( MDMOID_DIAG_TESTS, (InstanceIdStack *)iidStack, &fxsNo );

                msg->wordData = (UINT32)fxsNo;
                info->type = VOICE_DIAG_LINE_TEST_CMD;
                sprintf(info->cmdLine, "%s", newObj->testSelector );

                if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, msg)) != CMSRET_SUCCESS)
                {
                     cmsLog_error("could not send reset statistics message msg to Call Manager, ret=%d", ret);
                }
            }
        }
        else if ( !cmsUtl_strcmp(newObj->testSelector, currObj->testSelector) ||
             !cmsUtl_strcmp(newObj->testResult, currObj->testResult) )
        {
            /* according to TR104v2 spec, the value of diagnosticsState
             * should be reset to "None"
             */
            CMSMEM_REPLACE_STRING_FLAGS(newObj->diagnosticsState, MDMVS_NONE, mdmLibCtx.allocFlags);
        }

    }
    return CMSRET_SUCCESS;
}

CmsRet rcl_fXODiagTestsObject( _FXODiagTestsObject *newObj,
                const _FXODiagTestsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dECTObject( _DECTObject *newObj,
                const _DECTObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dECTPortableObject( _DECTPortableObject *newObj,
                const _DECTPortableObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dECTBaseObject( _DECTBaseObject *newObj,
                const _DECTBaseObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceServiceSipObject( _VoiceServiceSipObject *newObj,
                const _VoiceServiceSipObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        /* notify voice to restart */
        if ( mdmLibCtx.eid != EID_VOICE )
        {
            sendCfgChangeMsgToVoice();
        }
    }
    return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_sipClientObject
**
**  PURPOSE:        Adds, modifies or deletes a SIP client object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_sipClientObject( _SipClientObject *newObj,
                const _SipClientObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceServiceSipObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_SERVICE_SIP, MDMOID_SIP_CLIENT, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->clientNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        /* notify voice to restart */
        if ( mdmLibCtx.eid != EID_VOICE )
        {
            sendCfgChangeMsgToVoice();
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_SERVICE_SIP, MDMOID_SIP_CLIENT, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->clientNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_sIPClientContactObject
**
**  PURPOSE:        Adds, modifies or deletes a SIP client object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_sIPClientContactObject( _SIPClientContactObject *newObj,
                const _SIPClientContactObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    _SipClientObject *vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        /* enable object automatically */
        newObj->enable = TRUE;

        if ( (ret = cmsObj_getAncestor( MDMOID_SIP_CLIENT, MDMOID_SIP_CLIENT_CONTACT, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->contactNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        /* notify voice to restart */
        if ( mdmLibCtx.eid != EID_VOICE )
        {
            sendCfgChangeMsgToVoice();
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_SIP_CLIENT, MDMOID_SIP_CLIENT_CONTACT, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->contactNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_sIPNetworkObject
**
**  PURPOSE:        Adds, modifies or deletes a SIP network object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_sIPNetworkObject( _SIPNetworkObject *newObj,
                const _SIPNetworkObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceServiceSipObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_SERVICE_SIP, MDMOID_SIP_CLIENT, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->networkNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        /* notify voice to restart */
        if ( mdmLibCtx.eid != EID_VOICE )
        {
            sendCfgChangeMsgToVoice();
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_SERVICE_SIP, MDMOID_SIP_CLIENT, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->networkNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
 
    return CMSRET_SUCCESS;
}

CmsRet rcl_sIPNetworkResponseMapObject( _SIPNetworkResponseMapObject *newObj,
                const _SIPNetworkResponseMapObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_callControlObject( _CallControlObject *newObj,
                const _CallControlObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        /* notify voice to restart */
        if ( mdmLibCtx.eid != EID_VOICE )
        {
            sendCfgChangeMsgToVoice();
        }
    }
    return CMSRET_SUCCESS;
}

CmsRet rcl_callControlCallingFeaturesObject( _CallControlCallingFeaturesObject *newObj,
                const _CallControlCallingFeaturesObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_callingFeaturesSetObject
**
**  PURPOSE:        Adds, modifies or deletes a calling features object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_callingFeaturesSetObject( _CallingFeaturesSetObject *newObj,
                const _CallingFeaturesSetObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    CallControlCallingFeaturesObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL_CALLING_FEATURES, MDMOID_CALLING_FEATURES_SET, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->setNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL_CALLING_FEATURES, MDMOID_CALLING_FEATURES_SET, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->setNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
 
    return CMSRET_SUCCESS;
}

CmsRet rcl_callControlNumberingPlanObject( _CallControlNumberingPlanObject *newObj,
                const _CallControlNumberingPlanObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_callControlPrefixInfoObject( _CallControlPrefixInfoObject *newObj,
                const _CallControlPrefixInfoObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_callControlIncomingMapObject
**
**  PURPOSE:        Adds, modifies or deletes an incoming map object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_callControlIncomingMapObject( _CallControlIncomingMapObject *newObj,
                const _CallControlIncomingMapObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    CallControlObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_INCOMING_MAP, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->incomingMapNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        /* notify voice to update routing */
        if ( mdmLibCtx.eid != EID_VOICE )
        {
            sendRouteChangeMsgToVoice();
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_INCOMING_MAP, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->incomingMapNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
            /* notify voice to update routing */
            if ( mdmLibCtx.eid != EID_VOICE )
            {
                sendRouteChangeMsgToVoice();
            }
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
 
    return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_callControlOutgoingMapObject
**
**  PURPOSE:        Adds, modifies or deletes an outgoing map object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_callControlOutgoingMapObject( _CallControlOutgoingMapObject *newObj,
                const _CallControlOutgoingMapObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    CallControlObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_OUTGOING_MAP, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->outgoingMapNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_OUTGOING_MAP, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->outgoingMapNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
 
    return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_callControlExtensionObject
**
**  PURPOSE:        Adds, modifies or deletes a call control extension object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_callControlExtensionObject( _CallControlExtensionObject *newObj,
                const _CallControlExtensionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    CallControlObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_EXTENSION, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->extensionNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        /* notify voice to restart */
        if ( mdmLibCtx.eid != EID_VOICE )
        {
            sendCfgChangeMsgToVoice();
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_EXTENSION, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->extensionNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
 
    return CMSRET_SUCCESS;
}

CmsRet rcl_extensionStatsObject( _ExtensionStatsObject *newObj,
                const _ExtensionStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_extensionIncomingCallsObject( _ExtensionIncomingCallsObject *newObj,
                const _ExtensionIncomingCallsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_extensionOutgoingCallsObject( _ExtensionOutgoingCallsObject *newObj,
                const _ExtensionOutgoingCallsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_extensionRtpStatsObject( _ExtensionRtpStatsObject *newObj,
                const _ExtensionRtpStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_extensionDspStatsObject( _ExtensionDspStatsObject *newObj,
                const _ExtensionDspStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_callControlLineObject
**
**  PURPOSE:        Adds, modifies or deletes a call control line object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_callControlLineObject( _CallControlLineObject *newObj,
                const _CallControlLineObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    CallControlObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_LINE, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->lineNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        /* notify voice to restart */
        if ( mdmLibCtx.eid != EID_VOICE )
        {
            sendCfgChangeMsgToVoice();
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_CALL_CONTROL, MDMOID_CALL_CONTROL_LINE, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->lineNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
 
    return CMSRET_SUCCESS;
}

CmsRet rcl_lineStatsObject( _LineStatsObject *newObj,
                const _LineStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
        if ( newObj->resetStatistics )
        {
            /* Reset statistics flag set, reset it */
            newObj->resetStatistics = 0;

            CmsMsgHeader msg = EMPTY_MSG_HEADER;
            CmsRet ret;
            UINT32 lineInst;

            /* Get line instance */
            lineInst = PEEK_INSTANCE_ID(iidStack);

            /* Reset Statistics in Call Manager for specific line */
            msg.type = CMS_MSG_VOICE_STATISTICS_RESET;
            msg.src = mdmLibCtx.eid;
            msg.dst = EID_VOICE;
            msg.flags_request = 1;
            msg.wordData = lineInst;

            if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg)) != CMSRET_SUCCESS)
            {
                cmsLog_error("could not send reset statistics message msg to Call Manager, ret=%d", ret);
            }
        }
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_lineIncomingCallsObject( _LineIncomingCallsObject *newObj,
                const _LineIncomingCallsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_lineOutgoingCallsObject( _LineOutgoingCallsObject *newObj,
                const _LineOutgoingCallsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_lineRtpStatsObject( _LineRtpStatsObject *newObj,
                const _LineRtpStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_lineDspStatsObject( _LineDspStatsObject *newObj,
                const _LineDspStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallLogObject( _VoiceCallLogObject *newObj,
                const _VoiceCallLogObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_VOICE_CALL_LOG, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->callLogNumberOfEntries++;	/**< CallLogNumberOfEntries */
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_VOICE_CALL_LOG, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->callLogNumberOfEntries--;	/**< CallLogNumberOfEntries */
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
   return CMSRET_SUCCESS;
}

CmsRet rcl_callLogSessionObject( _CallLogSessionObject *newObj,
                const _CallLogSessionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    _VoiceCallLogObject * vCallLogObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_CALL_LOG, MDMOID_CALL_LOG_SESSION, &ancestorIidStack, (void **) &vCallLogObj )) == CMSRET_SUCCESS )
        {
            vCallLogObj->sessionNumberOfEntries++;	/**< SessionNumberOfEntries */
            cmsObj_set((const void *)vCallLogObj, &ancestorIidStack);
            cmsObj_free((void **)&vCallLogObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_CALL_LOG, MDMOID_CALL_LOG_SESSION, &ancestorIidStack, (void **) &vCallLogObj )) == CMSRET_SUCCESS )
        {
            vCallLogObj->sessionNumberOfEntries--;	/**< SessionNumberOfEntries */
            cmsObj_set((const void *)vCallLogObj, &ancestorIidStack);
            cmsObj_free((void **)&vCallLogObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
   return CMSRET_SUCCESS;
}

CmsRet rcl_callLogSessionStatsObject( _CallLogSessionStatsObject *newObj,
                const _CallLogSessionStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_callLogSessionDestinationObject( _CallLogSessionDestinationObject *newObj,
                const _CallLogSessionDestinationObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_destinationVoiceQualityObject( _DestinationVoiceQualityObject *newObj,
                const _DestinationVoiceQualityObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sessionDestinationDspObject( _SessionDestinationDspObject *newObj,
                const _SessionDestinationDspObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_destinationDspReceiveCodecObject( _DestinationDspReceiveCodecObject *newObj,
                const _DestinationDspReceiveCodecObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_destinationDspTransmitCodecObject( _DestinationDspTransmitCodecObject *newObj,
                const _DestinationDspTransmitCodecObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sessionDestinationRtpObject( _SessionDestinationRtpObject *newObj,
                const _SessionDestinationRtpObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sessionSourceObject( _SessionSourceObject *newObj,
                const _SessionSourceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sourceVoiceQualityObject( _SourceVoiceQualityObject *newObj,
                const _SourceVoiceQualityObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sessionSourceDspObject( _SessionSourceDspObject *newObj,
                const _SessionSourceDspObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sourceDSPReceiveCodecObject( _SourceDSPReceiveCodecObject *newObj,
                const _SourceDSPReceiveCodecObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sourceDSPTransmitCodecObject( _SourceDSPTransmitCodecObject *newObj,
                const _SourceDSPTransmitCodecObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_sessionSourceRtpObject( _SessionSourceRtpObject *newObj,
                const _SessionSourceRtpObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_callLogSignalingPerformanceObject( _CallLogSignalingPerformanceObject *newObj,
                const _CallLogSignalingPerformanceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_voIPProfileObject
**
**  PURPOSE:        Adds, modifies or deletes a VOIP profile object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_voIPProfileObject( _VoIPProfileObject *newObj,
                const _VoIPProfileObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_IP_PROFILE, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->voIPProfileNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_IP_PROFILE, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->voIPProfileNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    return CMSRET_SUCCESS;
}

CmsRet rcl_voIPProfileRTPObject( _VoIPProfileRTPObject *newObj,
                const _VoIPProfileRTPObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voIPProfileFaxT38Object( _VoIPProfileFaxT38Object *newObj,
                const _VoIPProfileFaxT38Object *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voIPProfileRTPRedundancyObject( _VoIPProfileRTPRedundancyObject *newObj,
                const _VoIPProfileRTPRedundancyObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voIPProfileSRTPObject( _VoIPProfileSRTPObject *newObj,
                const _VoIPProfileSRTPObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voIPProfileRTCPObject( _VoIPProfileRTCPObject *newObj,
                const _VoIPProfileRTCPObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

/*****************************************************************************
**  FUNCTION:       rcl_codecProfileObject
**
**  PURPOSE:        Adds, modifies or deletes a codec profile object in MDM
**
**  INPUT PARMS:    
**
**  OUTPUT PARMS:   
**
**  RETURNS:        CMSRET_SUCCESS if success, error otherwise
**
*****************************************************************************/
CmsRet rcl_codecProfileObject( _CodecProfileObject *newObj,
                const _CodecProfileObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    VoiceObject * vObj = NULL;
    InstanceIdStack ancestorIidStack = *iidStack;

    if ((newObj != NULL && currObj == NULL)) /* new instance creation */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_CODEC_PROFILE, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->codecProfileNumberOfEntries++;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    if ((newObj != NULL && currObj != NULL)) /* modify */
    {
    }
    if ((newObj == NULL && currObj != NULL)) /* delete */
    {
        if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_CODEC_PROFILE, &ancestorIidStack, (void **) &vObj )) == CMSRET_SUCCESS )
        {
            vObj->codecProfileNumberOfEntries--;
            cmsObj_set((const void *)vObj, &ancestorIidStack);
            cmsObj_free((void **)&vObj);
        }
        else
        {
            cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
            return ret;
        }
    }
    return CMSRET_SUCCESS;
}

CmsRet rcl_voiceServiceContactObject( _VoiceServiceContactObject *newObj,
                const _VoiceServiceContactObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_contactNumberObject( _ContactNumberObject *newObj,
                const _ContactNumberObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

#endif /* DMP_VOICE_SERVICE_2 */
#endif /* BRCM_VOICE_SUPPORT */
