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

#include <time.h>
#include "rcl.h"
#include "cms_util.h"
#include "rut_util.h"
#include "mdm.h"
#include "sys/time.h"
#include "rut_wan.h"


#ifdef BRCM_VOICE_SUPPORT
#include "dect_msg.h"
#include "rut_voice.h"
#include "endpoint_api.h"
#include <cms_log.h>

/* mwang_todo: these handler function should be put in more precise
 * DMP_<profilename> defines.
 */

CmsRet rcl_voiceObject( _VoiceObject *newObj,
                const _VoiceObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret;
   CmsMsgHeader msg=EMPTY_MSG_HEADER;

   if ( mdmLibCtx.eid == EID_VOICE )
   {
      return CMSRET_SUCCESS;
   }

   /* If object is being modified */
   if ( newObj != NULL && currObj != NULL )
   {
      /* If loglevel changed just inform voice and return */
      if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_LoggingLevel, currObj->X_BROADCOM_COM_LoggingLevel))
      {
         sendLoggingChangeMsgToVoice(0);
         return CMSRET_SUCCESS;
      }
      else if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_ModuleLogLevels, currObj->X_BROADCOM_COM_ModuleLogLevels))
      {
         sendLoggingChangeMsgToVoice(1);
         return CMSRET_SUCCESS;
      }
      else if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_CCTKTraceLevel, currObj->X_BROADCOM_COM_CCTKTraceLevel))
      {
         sendLoggingChangeMsgToVoice(1);
         return CMSRET_SUCCESS;
      }
      else if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_CCTKTraceGroup, currObj->X_BROADCOM_COM_CCTKTraceGroup))
      {
         sendLoggingChangeMsgToVoice(1);
         return CMSRET_SUCCESS;
      }

      if ( cmsUtl_strcmp(newObj->X_BROADCOM_COM_BoundIpAddr, currObj->X_BROADCOM_COM_BoundIpAddr) )
      {
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
#ifdef DMP_X_ITU_ORG_GPON_1
      else if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_ManagementProtocol, currObj->X_BROADCOM_COM_ManagementProtocol))
      {
         if( cmsUtl_strcmp(newObj->X_BROADCOM_COM_ManagementProtocol, MDMVS_TR69) == 0 )
         { 
            if ( !rutIsVoipRunning() )
            {
               /* Reset the boundIpaddress */
               CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_BoundIpAddr, "0.0.0.0", mdmLibCtx.allocFlags);

               /* If mgmt protocol changed to TR69 and voice isnt running ---> Initialize voice */
               msg.type = CMS_MSG_INIT_VOICE;            
               msg.src = mdmLibCtx.eid;
               msg.dst = EID_SSK;
               msg.flags_event = 1;  
               if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("could not send INIT_VOICE msg to ssk, ret=%d", ret);
               }         
            }
         }
      }
#endif /*  DMP_X_ITU_ORG_GPON_1  */            
   }
   else if (newObj == NULL && currObj != NULL)
   {
      /* wow, someone is deleting the voiceservice object?  This shouldn't happen.
       * should I stop voice? */

      cmsLog_error("voiceObject is being deleted!");
   }
   else if (currObj == NULL && newObj != NULL)
   {
      /* New Instance creation */
#if defined(DMP_X_ITU_ORG_GPON_1) || defined(DMP_EPON_VOICE_OAM)
      char opticalWanType[CMS_IFNAME_LENGTH];
#endif /* DMP_X_ITU_ORG_GPON_1 || DMP_EPON_VOICE_OAM */
      
#ifdef DMP_X_ITU_ORG_GPON_1
      if( rutWan_getOpticalWanType( &opticalWanType[0] ) == CMSRET_SUCCESS )
      {
         if( cmsUtl_strcmp(opticalWanType, MDMVS_GPON) == 0 )
         {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_ManagementProtocol, MDMVS_OMCI, mdmLibCtx.allocFlags);
         }
      }
#endif /*  DMP_X_ITU_ORG_GPON_1  */      

#ifdef DMP_EPON_VOICE_OAM   
      if( rutWan_getOpticalWanType( &opticalWanType[0] ) == CMSRET_SUCCESS )
      {
         if( cmsUtl_strcmp(opticalWanType, MDMVS_EPON) == 0 )
         {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_ManagementProtocol, MDMVS_OAM, mdmLibCtx.allocFlags);
         }
      }
#endif /* DMP_EPON_VOICE_OAM */

   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCapObject( _VoiceCapObject *newObj ,
                const _VoiceCapObject *currObj ,
                const InstanceIdStack *iidStack __attribute__((unused)) ,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* This object only created on bootup or initialized from XML file */
   int maxLine,maxCnx,maxVp,maxSess;
   char regionList[BUFLEN_256];
   char sigProt[BUFLEN_256];

   rutVoice_getMaxLine( &maxLine );
   rutVoice_getMaxCnx( &maxCnx );
   rutVoice_getSupportedAlpha2Locales( regionList, BUFLEN_256 );
   rutVoice_getSigProt( sigProt, BUFLEN_256 );
   rutVoice_getMaxVoiceProfiles( &maxVp );
   rutVoice_getMaxSessPerLine( &maxSess );

   newObj->maxProfileCount = maxVp;
   newObj->maxLineCount = maxLine;
   newObj->maxSessionsPerLine = maxSess;
   newObj->maxSessionCount = maxCnx;
   CMSMEM_REPLACE_STRING_FLAGS(newObj->signalingProtocols,sigProt,mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(newObj->regions,regionList,mdmLibCtx.allocFlags);
   newObj->RTCP = 0;
   newObj->SRTP = 0;
   /* newObj->SRTPKeyingMethods; */
   /* newObj->SRTPEncryptionKeySizes; */
   newObj->RTPRedundancy = 0;
   newObj->DSCPCoupled = 0;
   newObj->ethernetTaggingCoupled = 0;
   newObj->PSTNSoftSwitchOver = 0;
   newObj->faxT38  = 1;
   newObj->faxPassThrough = 0;
   newObj->modemPassThrough = 0;
   newObj->toneGeneration = 0;
   /* newObj->toneDescriptionsEditable = 0; */
   /* newObj->patternBasedToneGeneration = 0; */
   /* newObj->fileBasedToneGeneration = 0; */
   /* newObj->toneFileFormats; */
   newObj->ringGeneration = 0;
   /* newObj->ringDescriptionsEditable = 0; */
   /* newObj->patternBasedRingGeneration = 0; */
   /* newObj->ringPatternEditable = 0; */
   /* newObj->fileBasedRingGeneration = 0; */
   /* newObj->ringFileFormats; */
   newObj->digitMap = 1;
   newObj->numberingPlan = 0;
   newObj->buttonMap = 0;
   newObj->voicePortTests = 0;

   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCapSipObject( _VoiceCapSipObject *newObj ,
                const _VoiceCapSipObject *currObj ,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* This object only created on bootup or initialized from XML file */
   char role[BUFLEN_256];
   char extensions[BUFLEN_256];
   char transports[BUFLEN_256];
   char uriSchemes[BUFLEN_256];

#ifdef SIPLOAD
   rutVoice_getSipRole( role, BUFLEN_256 );
   rutVoice_getSipExtensions( extensions, BUFLEN_256 );
   rutVoice_getSipTransports( transports, BUFLEN_256 );
   rutVoice_getSipUriSchemes( uriSchemes, BUFLEN_256 );
#endif

   CMSMEM_REPLACE_STRING_FLAGS(newObj->role,role,mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(newObj->extensions,extensions,mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(newObj->transports,transports,mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(newObj->URISchemes,uriSchemes,mdmLibCtx.allocFlags);
   newObj->eventSubscription = 0;
   newObj->responseMap = 0;
   /* newObj->TLSAuthenticationProtocols; */
   /* newObj->TLSAuthenticationKeySizes; */
   /* newObj->TLSEncryptionProtocols; */
   /* newObj->TLSEncryptionKeySizes; */
   /* newObj->TLSKeyExchangeProtocols; */
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCapMgcpObject( _VoiceCapMgcpObject *newObj __attribute__((unused)),
                const _VoiceCapMgcpObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#if 0
CmsRet rcl_voiceCapH323Object( _VoiceCapH323Object *newObj __attribute__((unused)),
                const _VoiceCapH323Object *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_voiceCapCodecsObject( _VoiceCapCodecsObject *newObj ,
                const _VoiceCapCodecsObject *currObj ,
                const InstanceIdStack *iidStack ,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      char pTime[BUFLEN_64];
      char name[BUFLEN_64];
      int entryId,silSup,bitRate;

      rutVoice_getCodecPtime( PEEK_INSTANCE_ID(iidStack), pTime, BUFLEN_64 );
      rutVoice_getCodecName( PEEK_INSTANCE_ID(iidStack), name, BUFLEN_64 );
      rutVoice_getCodecEntryId( PEEK_INSTANCE_ID(iidStack), &entryId );
      rutVoice_getCodecSilSup( PEEK_INSTANCE_ID(iidStack), &silSup );
      rutVoice_getCodecBitRate( PEEK_INSTANCE_ID(iidStack), &bitRate );

      newObj->bitRate = bitRate;
      newObj->entryID = entryId;
      newObj->silenceSuppression = silSup;

      CMSMEM_REPLACE_STRING_FLAGS(newObj->packetizationPeriod, pTime, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(newObj->codec, name, mdmLibCtx.allocFlags);
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceProfObject( _VoiceProfObject *newObj,
                const _VoiceProfObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      /* Check to see if another line object can be created */
      int totalNumLines = 0;
      int numSp, numAcc, maxLines, i, maxCnx;

      VoiceObject * vObj = NULL;
      CmsRet ret;
      InstanceIdStack ancestorIidStack = *iidStack;     


      rutVoice_getNumSrvProv(&numSp);

      /* requires -1 because this new instance has already been "created" */
      for ( i = 0; i < (numSp-1); i++ )
      {
         rutVoice_getNumAccPerSrvProv(i, &numAcc);
         totalNumLines += numAcc;
      }
      rutVoice_getMaxLineInstances(&maxLines);

      if ( totalNumLines >= maxLines )
      {
         cmsLog_error("Cannot create Voice Profile object\n");
         return CMSRET_INTERNAL_ERROR;
      }

      int spNum = 0;
      int vpInst = 0;
      vpInst = INSTANCE_ID_AT_DEPTH(iidStack, 1);

      /* Check to see if another voice profile object can be created */
      if ( (rutVoice_assignSpNumToVpInst( vpInst, &spNum )) == CMSRET_SUCCESS )
      {
         newObj->X_BROADCOM_COM_SPNum = spNum;
      }
      else
      {
         cmsLog_error("Unable to create new Voice Profile object");
         return CMSRET_INTERNAL_ERROR;
      }

      rutVoice_getMaxCnx( &maxCnx );

      /* Set the maximum call sessions supported by the endpoint */
      newObj->maxSessions = maxCnx;
      

      /* Update VoiceService.i.voiceProfileNumberOfEntries field */
      /* Get ancestor object */
      if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_VOICE_PROF, &ancestorIidStack, (void **) &vObj )) != CMSRET_SUCCESS )
      {
         cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
         return ret;
      }
      else
      {
         vObj->voiceProfileNumberOfEntries += 1;
         
         /* Set VoiceService.i.voiceProfileNumberOfEntries field */
         if ((ret = cmsObj_set( vObj, &ancestorIidStack )) != CMSRET_SUCCESS )
         {
            cmsLog_error("Unable to set value to mdm, ret = %d\n", ret);
            cmsObj_free((void **) &vObj);
            return ret;
         }

         cmsObj_free((void **) &vObj);         
      }

      
   }
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
   }
   if ((newObj == NULL && currObj != NULL)) /* delete */
   {

      VoiceObject * vObj = NULL;
      CmsRet ret;


      int vpInst = INSTANCE_ID_AT_DEPTH(iidStack, 1); /* vp instance to delete */


      /* Retrieve the voice service instance that is the parent of the voice profile instance to delete */
      InstanceIdStack ancestorIidStack = *iidStack;


      rutVoice_updateSpNum( vpInst );

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
      

      if ( (ret = cmsObj_getAncestor( MDMOID_VOICE, MDMOID_VOICE_PROF, &ancestorIidStack, (void **) &vObj )) != CMSRET_SUCCESS )
      {
         cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
         return ret;
      }
      else
      {
         vObj->voiceProfileNumberOfEntries -= 1;
         
         /* Set VoiceService.i.voiceProfileNumberOfEntries field */
         if ((ret = cmsObj_set( vObj, &ancestorIidStack )) != CMSRET_SUCCESS )
         {
            cmsLog_error("Unable to set value to mdm, ret = %d\n", ret);
            cmsObj_free((void **) &vObj);
            return ret;
         }

         cmsObj_free((void **) &vObj);         
      }

   }
   /* Send message to voice to update routing table */
   sendRouteChangeMsgToVoice();
   return CMSRET_SUCCESS;
}


CmsRet rcl_voiceProfProviderObject( _VoiceProfProviderObject *newObj ,
                const _VoiceProfProviderObject *currObj ,
                const InstanceIdStack *iidStack ,
                char **errorParam ,
                CmsRet *errorCode )
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_voiceProfSipObject( _VoiceProfSipObject *newObj,
                const _VoiceProfSipObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
      /* Send message to voice to update routing table */
      sendRouteChangeMsgToVoice();
   }
   return CMSRET_SUCCESS;
}


CmsRet rcl_voiceProfSipSubscribeObject( _VoiceProfSipSubscribeObject *newObj ,
                const _VoiceProfSipSubscribeObject *currObj ,
                const InstanceIdStack *iidStack ,
                char **errorParam ,
                CmsRet *errorCode )
{
   return CMSRET_SUCCESS;
}

#if 0
CmsRet rcl_voiceProfSipResponseObject( _VoiceProfSipResponseObject *newObj __attribute__((unused)),
                const _VoiceProfSipResponseObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
CmsRet rcl_voiceProfMgcpObject( _VoiceProfMgcpObject *newObj __attribute__((unused)),
                const _VoiceProfMgcpObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#if 0
CmsRet rcl_voiceProfH323Object( _VoiceProfH323Object *newObj __attribute__((unused)),
                const _VoiceProfH323Object *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_voiceProfRtpObject( _VoiceProfRtpObject *newObj,
                const _VoiceProfRtpObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
   }
   return CMSRET_SUCCESS;
}

#if 0
CmsRet rcl_voiceProfRtpRtcpObject( _VoiceProfRtpRtcpObject *newObj __attribute__((unused)),
                const _VoiceProfRtpRtcpObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_voiceProfRtpSrtpObject( _VoiceProfRtpSrtpObject *newObj __attribute__((unused)),
                const _VoiceProfRtpSrtpObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceProfRtpRedundancyObject( _VoiceProfRtpRedundancyObject *newObj __attribute__((unused)),
                const _VoiceProfRtpRedundancyObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#if 0
CmsRet rcl_voiceProfLineNumberingPlanObject( _VoiceProfLineNumberingPlanObject *newObj __attribute__((unused)),
                const _VoiceProfLineNumberingPlanObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfNumberingPlanPrefixInfoObject( _VoiceProfNumberingPlanPrefixInfoObject *newObj __attribute__((unused)),
                const _VoiceProfNumberingPlanPrefixInfoObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfToneObject( _VoiceProfToneObject *newObj __attribute__((unused)),
                const _VoiceProfToneObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfToneEventObject( _VoiceProfToneEventObject *newObj __attribute__((unused)),
                const _VoiceProfToneEventObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfToneDescriptionObject( _VoiceProfToneDescriptionObject *newObj __attribute__((unused)),
                const _VoiceProfToneDescriptionObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfTonePatternObject( _VoiceProfTonePatternObject *newObj __attribute__((unused)),
                const _VoiceProfTonePatternObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfButtonMapObject( _VoiceProfButtonMapObject *newObj __attribute__((unused)),
                const _VoiceProfButtonMapObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceProfButtonMapButtonObject( _VoiceProfButtonMapButtonObject *newObj __attribute__((unused)),
                const _VoiceProfButtonMapButtonObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_voiceProfFaxT38Object( _VoiceProfFaxT38Object *newObj,
                const _VoiceProfFaxT38Object *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      /* Create line objects */
      CmsRet localRet;
      InstanceIdStack localIidStack = EMPTY_INSTANCE_ID_STACK;
      void *lineObj = NULL;

      /*
       * Figure out if there are Line objects under me.  If not, then create the
       * standard set of Line objects.
       */
      localRet = cmsObj_getNextInSubTreeFlags( MDMOID_VOICE_LINE,
                                               iidStack,
                                               &localIidStack,
                                               OGF_NO_VALUE_UPDATE,
                                               &lineObj );

      if ( localRet == CMSRET_SUCCESS )
      {
         /*
          * If I was able to get a Line object under me, then there is no
          * need to create Line object.  This happens during startup time.
          */
         cmsLog_debug("Line objects already exist");
         cmsObj_free((void **) &lineObj);
      }
      else
      {
         localIidStack = *iidStack;
         localRet = cmsObj_addInstance( MDMOID_VOICE_LINE, &localIidStack );

         if (localRet != CMSRET_SUCCESS)
         {
            cmsLog_error("addObjectInstance: Failed, ret=%d", localRet);
            return localRet;
         }
      }

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         if ((sendCfgChangeMsgToVoice()) != CMSRET_SUCCESS)
         {
            cmsLog_error("sendCfgChangeMsgToVoice failed");
         }
      }
   }
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineObject( _VoiceLineObject *newObj,
                const _VoiceLineObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      int totalNumLines = 0;
      int numSp, numAcc, maxLines, i;
      CmsRet ret;
      VoiceProfObject *vpObj;
      InstanceIdStack ancestorIidStack = *iidStack;

      /* Check to see if we can create another line object */
      rutVoice_getNumSrvProv(&numSp);
      for ( i = 0; i < numSp; i++ )
      {
         rutVoice_getNumAccPerSrvProv(i, &numAcc);
         totalNumLines += numAcc;
      }
      rutVoice_getMaxLineInstances(&maxLines);

      if ( totalNumLines >= maxLines )
      {
         cmsObj_getAncestor( MDMOID_VOICE_PROF, MDMOID_VOICE_LINE, &ancestorIidStack, (void **) &vpObj );
         /* Temporary increase the number of lines */
         vpObj->numberOfLines += 1;
         cmsObj_set( vpObj, &ancestorIidStack );
         cmsLog_error("Cannot create line object\n");
         cmsObj_free((void **) &vpObj);
         return CMSRET_INTERNAL_ERROR;
      }

      rutVoice_assignCMAcnt();

      /* Update VoiceProfile.i.NumberOfLines field */
      /* Get ancestor object */
      if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_PROF, MDMOID_VOICE_LINE, &ancestorIidStack, (void **) &vpObj )) != CMSRET_SUCCESS )
      {
         cmsLog_error("Unable to get ancestor object, ret =%d\n", ret);
         return ret;
      }
      else
      {
         vpObj->numberOfLines += 1;

         /* Set VoiceProfile.i.NumberofLines field */
         if ((ret = cmsObj_set( vpObj, &ancestorIidStack )) != CMSRET_SUCCESS )
         {
            cmsLog_error("Unable to set value to mdm, ret = %d\n", ret);
            cmsObj_free((void **) &vpObj);
            return ret;
         }

         cmsObj_free((void **) &vpObj);
      }

      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
   }
   else if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
   }
   else if ((newObj == NULL && currObj != NULL )) /* delete */
   {
      int line, vp;
      VoiceProfObject *vpObj = NULL;
      CmsRet ret;

      /* Retrieve the line instance to delete */
      line = INSTANCE_ID_AT_DEPTH(iidStack, 2);

      /* Retrieve the voice profile instance that is the parent of the line instance to delete */
      InstanceIdStack ancestorIidStack = *iidStack;

      /* Get ancestor object */
      if ( (ret = cmsObj_getAncestor( MDMOID_VOICE_PROF, MDMOID_VOICE_LINE, &ancestorIidStack, (void **) &vpObj )) != CMSRET_SUCCESS )
      {
         cmsLog_error("Unable to get ancestor object");
         return ret;
      }
      else
      {
         vp = INSTANCE_ID_AT_DEPTH(&ancestorIidStack, 1);

         vpObj->numberOfLines -= 1;

         /* Set VoiceProfile.i.NumberofLines field */
         if ((ret = cmsObj_set( vpObj, &ancestorIidStack )) != CMSRET_SUCCESS )
         {
            cmsLog_error("Unable to set value to mdm, ret = %d\n", ret);
            cmsObj_free((void **) &vpObj);
            return ret;
         }

         rutVoice_updateCMAcntNum( vp, line );

         if ( mdmLibCtx.eid != EID_VOICE )
         {
            sendCfgChangeMsgToVoice();
         }

         cmsObj_free((void **) &vpObj);
      }
      /* Send message to voice to update routing table */
      sendRouteChangeMsgToVoice();
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineSipObject( _VoiceLineSipObject *newObj,
                const _VoiceLineSipObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
      /* Send message to voice to update routing table */
      sendRouteChangeMsgToVoice();
   }
   return CMSRET_SUCCESS;
}

#if 0
CmsRet rcl_voiceLineSipEventSubscribeObject( _VoiceLineSipEventSubscribeObject *newObj __attribute__((unused)),
                const _VoiceLineSipEventSubscribeObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceLineMgcpObject( _VoiceLineMgcpObject *newObj,
                const _VoiceLineMgcpObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
   }
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceLineH323Object( _VoiceLineH323Object *newObj __attribute__((unused)),
                const _VoiceLineH323Object *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceLineRingerObject( _VoiceLineRingerObject *newObj __attribute__((unused)),
                const _VoiceLineRingerObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceLineRingerEventObject( _VoiceLineRingerEventObject *newObj __attribute__((unused)),
                const _VoiceLineRingerEventObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceLineRingerDescriptionObject( _VoiceLineRingerDescriptionObject *newObj __attribute__((unused)),
                const _VoiceLineRingerDescriptionObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif
#if 0
CmsRet rcl_voiceLineRingerPatternObject( _VoiceLineRingerPatternObject *newObj __attribute__((unused)),
                const _VoiceLineRingerPatternObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_voiceLineCallingFeaturesObject( _VoiceLineCallingFeaturesObject *newObj,
                const _VoiceLineCallingFeaturesObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   if ((newObj != NULL && currObj == NULL))      /* new instance creation */
   {
      int maxSess;

      rutVoice_getMaxSessPerLine( &maxSess );

      /* Set the maximum conferencing sessions supported by the endpoint */
      newObj->maxSessions = maxSess;
   }
   else if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineProcessingObject( _VoiceLineProcessingObject *newObj __attribute__((unused)),
                const _VoiceLineProcessingObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineCodecObject( _VoiceLineCodecObject *newObj __attribute__((unused)),
                const _VoiceLineCodecObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineCodecListObject( _VoiceLineCodecListObject *newObj,
                const _VoiceLineCodecListObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      char name[BUFLEN_64];
      int entryId,bitRate;

      rutVoice_getCodecName( PEEK_INSTANCE_ID(iidStack), name, BUFLEN_64 );
      rutVoice_getCodecEntryId( PEEK_INSTANCE_ID(iidStack), &entryId );
      rutVoice_getCodecBitRate( PEEK_INSTANCE_ID(iidStack), &bitRate );

      newObj->bitRate = bitRate;
      newObj->entryID = entryId;

      /* Only set defaults if it hasnt been set in flash */
      if( newObj->packetizationPeriod == NULL )
      {
         /* Set default ptime of 20 */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->packetizationPeriod, "20", mdmLibCtx.allocFlags);
      }

      CMSMEM_REPLACE_STRING_FLAGS(newObj->codec, name, mdmLibCtx.allocFlags);
   }
   else if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
   }
   return CMSRET_SUCCESS;
}

#if 0
CmsRet rcl_voiceLineSessionObject( _VoiceLineSessionObject *newObj __attribute__((unused)),
                const _VoiceLineSessionObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_voiceLineStatsObject( _VoiceLineStatsObject *newObj ,
                const _VoiceLineStatsObject *currObj ,
                const InstanceIdStack *iidStack ,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* If resetStatistics is true, reset this stats object */
   if ( newObj != NULL && currObj != NULL && newObj->resetStatistics )
   {
      CmsMsgHeader msg = EMPTY_MSG_HEADER;
      CmsRet ret;
      MdmObjectId tempId;
      UINT16 tempSeq;
      UINT32 lineInst;

      /* Save MDM Object ID and sequence number before memset */
      tempId = newObj->_oid;
      tempSeq = newObj->_sequenceNum;

      /* Reset */
      memset( newObj, 0, sizeof(_VoiceLineStatsObject) );
      newObj->_oid = tempId;
      newObj->_sequenceNum = tempSeq;

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
   else if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      CmsRet localRet;
      InstanceIdStack localIidStack=EMPTY_INSTANCE_ID_STACK;
      void *listObj=NULL;

      /*
       * Figure out if there are List objects under me.  If not, then create the
       * standard set of List objects.
       */
      localRet = cmsObj_getNextInSubTreeFlags(MDMOID_VOICE_LINE_CODEC_LIST,
                                              iidStack,
                                              &localIidStack,
                                              OGF_NO_VALUE_UPDATE,
                                              &listObj);
      if (localRet == CMSRET_SUCCESS)
      {
         /*
          * If I was able to get a List object under me, then there is no
          * need to create List objects.  This happens during startup time.
          */
         cmsObj_free((void **) &listObj);
      }
      else
      {
         int maxCodecs, j;

         rutVoice_getMaxCodecs( &maxCodecs );

         for ( j = 0; j < maxCodecs; j++)
         {
            /*
             * LocalIidStack will be modified by each call of cmsObj_addInstance,
             * so we have to set it back to the parent iidStack before each call.
             */
            localIidStack = *iidStack;

            localRet = cmsObj_addInstance(MDMOID_VOICE_LINE_CODEC_LIST, &localIidStack);
            if (localRet != CMSRET_SUCCESS)
            {
               cmsLog_error("addObjectInstance: Failed, ret=%d", localRet);
               return localRet;
            }
         }

         /* If change is coming from VOICE, no need to notify VOICE */
         if ( mdmLibCtx.eid != EID_VOICE )
         {
            sendCfgChangeMsgToVoice();
         }
      }
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_voicePhyIntfObject( _VoicePhyIntfObject *newObj ,
                const _VoicePhyIntfObject *currObj ,
                const InstanceIdStack *iidStack ,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj == NULL)) /* new instance creation */
   {
      char description[BUFLEN_32];
      char phyPort[BUFLEN_4];
      int iiD = PEEK_INSTANCE_ID(iidStack);
      int  type;

      newObj->interfaceID = iiD - 1;
      sprintf( phyPort,"%01d",(iiD-1) );

      CMSMEM_REPLACE_STRING_FLAGS(newObj->phyPort,phyPort,mdmLibCtx.allocFlags);

      if( rutVoice_getPhysEndptType( (iiD-1), &type ) != CMSRET_SUCCESS )
      {
          type = EPTYPE_FXS;
      }

      memset(description, 0, sizeof(description));
      if( type == EPTYPE_FXS ){
         sprintf( description,"FXS%d",(iiD-1) );
      }
      else if( type == EPTYPE_PSTN ){
         sprintf( description,"FXO");
      }
      else if( type == EPTYPE_DECT ){
         sprintf( description,"DECT");
      }
      else{
         sprintf( description,"NOSIG");
      }

      CMSMEM_REPLACE_STRING_FLAGS(newObj->description,description,mdmLibCtx.allocFlags);

      return CMSRET_SUCCESS;
   }
   else if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
   }
   /* Send message to voice to update routing table */
   sendRouteChangeMsgToVoice();

   return ( CMSRET_SUCCESS );
}

CmsRet rcl_voicePhyIntfTestsObject( _VoicePhyIntfTestsObject *newObj,
                const _VoicePhyIntfTestsObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
   }
   /* Send message to voice to update routing table */
   sendRouteChangeMsgToVoice();
   return CMSRET_SUCCESS;
}

CmsRet rcl_voicePstnObject( _VoicePstnObject *newObj,
                const _VoicePstnObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendCfgChangeMsgToVoice();
      }
      /* Send message to voice to update routing table */
      sendRouteChangeMsgToVoice();
   }
   return CMSRET_SUCCESS;
}

#if defined( DMP_X_BROADCOM_COM_NTR_1 )
CmsRet rcl_voiceNtrObject( _VoiceNtrObject *newObj,
                const _VoiceNtrObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE (NTR Task), no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendNtrCfgChangeMsgToVoice();
      }
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceNtrHistoryObject(_VoiceNtrHistoryObject *newObj,
                const _VoiceNtrHistoryObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   /* We dont want to send a NTR config changed message because
    * History is changed automatically by the NTR task */
   if ((newObj != NULL && currObj != NULL)) /* modify */
   {
      /* If change is coming from VOICE, no need to notify VOICE */
      if ( mdmLibCtx.eid != EID_VOICE )
      {
         sendNtrCfgChangeMsgToVoice();
      }
   }
   return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_NTR_1 */


#ifdef DMP_X_BROADCOM_COM_DECTENDPOINT_1

CmsRet rcl_voiceDectSystemSettingObject( _VoiceDectSystemSettingObject *newObj,
                                const _VoiceDectSystemSettingObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam,
                                CmsRet *errorCode)
{
   /* Nothing needs to be done since a change to the DECT interface object
   ** MDM data would have been triggered by the VOICE application applying
   ** its own processing - any other source of the change would be erroneous.
   */
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDECTHandsetObject( _VoiceDECTHandsetObject *newObj,
                                   const _VoiceDECTHandsetObject *currObj,
                                   const InstanceIdStack *iidStack,
                                   char **errorParam,
                                   CmsRet *errorCode)
{
   int i, j, handsetId, maxVp, maxLines, serviceId, vpInst;
   InstanceIdStack vpIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack lineIidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack localIidStack = EMPTY_INSTANCE_ID_STACK;
   void   *obj;
   CmsRet ret;
   char   name[20];

   if ( newObj != NULL &&  currObj == NULL ) /* Creation of object instance */
   {
      if(mdmLibCtx.eid == EID_DECT) /* Only reset the object's parameters when DECT is the one adding it. */
      {
         newObj->X_BROADCOM_COM_MANIC = 0;
         newObj->X_BROADCOM_COM_IPUI = 0;
         newObj->X_BROADCOM_COM_MODIC = 0;
         newObj->X_BROADCOM_COM_ID = 0;

         time_t calendar_time;
         struct tm *local_time = NULL;
         char date_string[32];

         /* Get the local time information */
         calendar_time = time(NULL);
         if (calendar_time != -1)
         {
            local_time = localtime (&calendar_time);
         }

         if ( local_time )
         {
            strftime(date_string, sizeof(date_string), "%Y-%m-%dT%H:%M:%S", local_time);
         }
         else
         {
            snprintf(date_string, sizeof(date_string), "2010-01-01T00:00:00Z");
         }

         CMSMEM_REPLACE_STRING_FLAGS( newObj->subscriptionTime, date_string, mdmLibCtx.allocFlags );
         CMSMEM_REPLACE_STRING_FLAGS( newObj->status, "unreachable", mdmLibCtx.allocFlags );
         CMSMEM_REPLACE_STRING_FLAGS( newObj->X_BROADCOM_COM_Call_Interception, MDMVS_YES, mdmLibCtx.allocFlags );
      }
   }
   else if( newObj != NULL && currObj != NULL ) /* Modification of an object instance */
   {
      /* If the current handset id is unassigned, or if we are making a
       * modification to the handset id, then make sure to add the handset to
       * the attached handset list for every line.
       */
      if( currObj->X_BROADCOM_COM_ID == 0 && currObj->X_BROADCOM_COM_ID != newObj->X_BROADCOM_COM_ID )
      {
         handsetId = newObj->X_BROADCOM_COM_ID;
         handsetId = (handsetId == 0 || handsetId >= 32 ) ? 0 : (handsetId - 1);

         /* create internal name for this handset if it doesn't exist */
         if( newObj->X_BROADCOM_COM_Name == NULL )
         {
            sprintf(name, "handset %d", handsetId + 1);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_Name, name, mdmLibCtx.allocFlags);
         }

         /* get current voice service instance (should be 1) */
         serviceId = INSTANCE_ID_AT_DEPTH( iidStack, 0 );
         if(CMSRET_SUCCESS != rutVoice_getNumSrvProv( &maxVp ) || maxVp <= 0)
         {
            return CMSRET_INTERNAL_ERROR;
         }
         PUSH_INSTANCE_ID( &vpIidStack, serviceId );

         /* Get the voice profile for each service provider */
         for(i = 0; i < maxVp; i++)
         {
            rutVoice_mapSpNumToVpInst( i, &vpInst );
            PUSH_INSTANCE_ID( &vpIidStack, vpInst );
            ret = cmsObj_get( MDMOID_VOICE_PROF, &vpIidStack, OGF_NO_VALUE_UPDATE, &obj );

            if( ret != CMSRET_SUCCESS )
            {
               return CMSRET_INTERNAL_ERROR;
            }

            if( ((_VoiceProfObject *) obj)->numberOfLines <= 0 )
            {
               cmsObj_free(&obj);
               cmsLog_debug("Line instance not exist in voice profile %d\n", i);
               return CMSRET_INTERNAL_ERROR;
            }
            else
            {
               maxLines = ((_VoiceProfObject *) obj)->numberOfLines;
            }
            cmsObj_free(&obj);

            /* check line is already attached with dect handset */
            if ( rutIsLineAttachedWithDectHS( handsetId ) )
            {
               break;
            }

            /* Get the voice line for each account in the voice profile */
            for(j = 0; j < maxLines; j++)
            {
               if(CMSRET_SUCCESS != (ret = cmsObj_getNextInSubTreeFlags(MDMOID_VOICE_LINE, &vpIidStack, &lineIidStack, OGF_NO_VALUE_UPDATE, &obj)))
               {
                  cmsLog_error("Can't retrieve Line instance (ret = %d)", ret);
                  return CMSRET_INTERNAL_ERROR;
               }
               cmsObj_free(&obj);

               /* Get attached handset list and add the new handset to the list */
               if(CMSRET_SUCCESS != cmsObj_getNextInSubTree(MDMOID_VOICE_LINE_ATTACHED_HANDSET, &lineIidStack, &localIidStack, &obj))
               {
                  cmsLog_error("Can't retrieve attached handset list");
                  return CMSRET_INTERNAL_ERROR;
               }
               else
               {
                  ((_VoiceLineAttachedHandsetObject *)obj)->element |=  ( 1 << handsetId );
                  ((_VoiceLineAttachedHandsetObject *)obj)->totalNumber++;
                  cmsObj_set(obj, &localIidStack);
                  cmsObj_free(&obj);
               }
            }
         }

         /* Send Message to Voice Indicating that registered DECT handset list has changed */
         CmsMsgHeader msg = EMPTY_MSG_HEADER;
         msg.type = CMS_MSG_VOICE_DECT_REGHSETLIST_UPDATE;
         msg.src = mdmLibCtx.eid;
         msg.dst = EID_VOICE;
         msg.flags_event = 1;
         msg.flags_bounceIfNotRunning = 1;
         msg.wordData = newObj->X_BROADCOM_COM_ID;

         if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not send reset statistics message msg to Call Manager, ret=%d", ret);
         }
      }
      else if( newObj->X_BROADCOM_COM_Delete )
      {
         /* Send Message to Voice Indicating that de-registered DECT handset list has changed */
         CmsMsgHeader msg = EMPTY_MSG_HEADER;

         msg.type = CMS_MSG_VOICE_DECT_HS_DELETE;
         msg.src = EID_CONSOLED;
         msg.dst = EID_DECT;
         msg.flags_event = 1;
         msg.flags_bounceIfNotRunning = 1;
         msg.wordData = newObj->X_BROADCOM_COM_ID;

         if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not send reset statistics message msg to Call Manager, ret=%d", ret);
         }

         newObj->X_BROADCOM_COM_Delete = 0;
      }
   }
   else if( newObj == NULL && currObj != NULL ) /* deletion of an object instance */
   {
      if( currObj->X_BROADCOM_COM_ID != 0 )
      {
         handsetId = currObj->X_BROADCOM_COM_ID;
         handsetId = (handsetId == 0 || handsetId >= 32 ) ? 0 : (handsetId - 1);
         /* get current voice service instance, should be 1 */
         serviceId = INSTANCE_ID_AT_DEPTH(iidStack, 0);
         if(CMSRET_SUCCESS != rutVoice_getNumSrvProv( &maxVp ) || maxVp <= 0)
         {
            return CMSRET_INTERNAL_ERROR;
         }
         PUSH_INSTANCE_ID(&vpIidStack, serviceId );

         for(i = 0; i < maxVp; i++)
         {
            /* get voice profile for each service provider */
            rutVoice_mapSpNumToVpInst( i, &vpInst );
            PUSH_INSTANCE_ID(&vpIidStack, vpInst );
            ret = cmsObj_get(MDMOID_VOICE_PROF, &vpIidStack, OGF_NO_VALUE_UPDATE, &obj);

            if( ret == CMSRET_SUCCESS )
            {
               if( ((_VoiceProfObject *) obj)->numberOfLines <= 0 )
               {
                  cmsObj_free(&obj);
                  cmsLog_debug("Line instance not exist in voice profile %d\n", i);
                  return CMSRET_INTERNAL_ERROR;
               }
               else
               {
                  maxLines = ((_VoiceProfObject *) obj)->numberOfLines;
               }
               cmsObj_free(&obj);
            }
            else
            {
               return CMSRET_INTERNAL_ERROR;
            }

            /* get line profile for each account in voice profile */
            for(j = 0; j < maxLines; j++)
            {
               if(CMSRET_SUCCESS != (ret = cmsObj_getNextInSubTreeFlags(MDMOID_VOICE_LINE, &vpIidStack, &lineIidStack, OGF_NO_VALUE_UPDATE, &obj)))
               {
                  cmsLog_debug("Can't retrieve Line instance ret = %d", ret);
                  return CMSRET_INTERNAL_ERROR;
               }
               cmsObj_free(&obj);

               /* get attached handset list */
               if(CMSRET_SUCCESS != cmsObj_getNextInSubTree(MDMOID_VOICE_LINE_ATTACHED_HANDSET, &lineIidStack, &localIidStack, &obj))
               {
                  cmsLog_debug("Can't retrieve attached handset list \n");
                  return CMSRET_INTERNAL_ERROR;
               }
               else
               {
                  if(((_VoiceLineAttachedHandsetObject *)obj)->element & ( 1 << handsetId ))
                  {
                     ((_VoiceLineAttachedHandsetObject *)obj)->element &=  ~( 1 << handsetId );
                     if(((_VoiceLineAttachedHandsetObject *)obj)->totalNumber > 0 )
                     {
                        ((_VoiceLineAttachedHandsetObject *)obj)->totalNumber--;
                     }

                     cmsObj_set(obj, &localIidStack);
                  }
                  cmsObj_free(&obj);
               }
            }
         }

         /* Send Message to Voice Indicating that registered DECT handset list has changed */
         CmsMsgHeader msg = EMPTY_MSG_HEADER;
         msg.type = CMS_MSG_VOICE_DECT_REGHSETLIST_UPDATE;
         msg.src = mdmLibCtx.eid;
         msg.dst = EID_VOICE;
         msg.flags_event = 1;
         msg.flags_bounceIfNotRunning = 1;
         msg.wordData = currObj->X_BROADCOM_COM_ID;

         if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not send reset statistics message msg to Call Manager, ret=%d", ret);
         }
      }
   }
   /* Send message to voice to update routing table */
   sendRouteChangeMsgToVoice();
   rutVoice_dectListUpdate( DECT_DELAY_LIST_INTERNAL_NAMES, 0 );

   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectPinCodeObject( _VoiceDectPinCodeObject *newObj,
                const _VoiceDectPinCodeObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectClockMasterObject( _VoiceDectClockMasterObject *newObj,
                const _VoiceDectClockMasterObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectResetBaseObject( _VoiceDectResetBaseObject *newObj,
                const _VoiceDectResetBaseObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   int     maxHset, i;
   void   *obj;
   CmsRet  ret;
   InstanceIdStack ancestorIidStack = *iidStack;
   InstanceIdStack localIidStack    = EMPTY_INSTANCE_ID_STACK;

   cmsLog_debug("rcl function  \n");
   if ((newObj != NULL && currObj != NULL) && (newObj->element > 0)) /* modify */
   {

      if(cmsObj_getAncestorFlags( MDMOID_VOICE_DECT_SYSTEM_SETTING, MDMOID_VOICE_DECT_RESET_BASE, &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &obj ) == CMSRET_SUCCESS )
      {
         cmsLog_debug("rcl function  2\n");
         /* clear system setting */
         CMSMEM_FREE_BUF_AND_NULL_PTR(((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_LinkDate);
         ((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_Type = 0;
         ((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_DectId = 0;
         ((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_MANIC = 0;
         ((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_MODIC = 0;
         ((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_MaxNumberOfHandsets = 0; /**< X_BROADCOM_COM_MaxNumberOfHandsets */
         ((_VoiceDectSystemSettingObject *)obj)->X_BROADCOM_COM_ServiceEnabled = 1; /**< X_BROADCOM_COM_ServiceEnabled */

         cmsObj_set(&obj, &ancestorIidStack);
         cmsObj_free( &obj );

         cmsLog_debug("rcl function 3 \n");
         /* clear register handset */
         rutVoice_getMaxDectHset( &maxHset );
         cmsLog_debug("rcl function 4 \n");
         for(i = 0; i < maxHset; i++)
         {
            INIT_INSTANCE_ID_STACK(&localIidStack);
            ret = cmsObj_getNextInSubTree(MDMOID_DECT_HANDSET, &ancestorIidStack, &localIidStack, &obj);
            if(ret == CMSRET_SUCCESS )
            {
               cmsObj_free( &obj );
               cmsObj_deleteInstance(MDMOID_DECT_HANDSET, &localIidStack);
            }
         }
         /* keep original value, should be 0 */
         newObj->element = currObj->element;
         cmsLog_debug("rcl function 5 \n");
      }
   }
   /* Send message to voice to update routing table */
   sendRouteChangeMsgToVoice();
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectFirmwareVersionObject( _VoiceDectFirmwareVersionObject *newObj,
                const _VoiceDectFirmwareVersionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectHardwareVersionObject( _VoiceDectHardwareVersionObject *newObj,
                const _VoiceDectHardwareVersionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectEEPROMVersionObject( _VoiceDectEEPROMVersionObject *newObj,
                const _VoiceDectEEPROMVersionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_voiceLineSettingObject( _VoiceLineSettingObject *newObj,
                const _VoiceLineSettingObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineNameObject( _VoiceLineNameObject *newObj,
                const _VoiceLineNameObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   if( newObj != NULL && currObj == NULL) /* newly created object */
   {
      if(newObj->element == NULL)
      {
         InstanceIdStack ancestorIidStack = *iidStack;
         char name[20];
         unsigned int lineId = 0;

         /* Get the line number */
         lineId = INSTANCE_ID_AT_DEPTH(&ancestorIidStack, 2);
         snprintf( name, 20, "Line %u", lineId );

         CMSMEM_REPLACE_STRING_FLAGS( newObj->element, name, mdmLibCtx.allocFlags );
      }
   }
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineDectLineIdObject( _VoiceLineDectLineIdObject *newObj,
                const _VoiceLineDectLineIdObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   /* Send message to voice to update routing table */
   sendRouteChangeMsgToVoice();
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineAttachedHandsetObject( _VoiceLineAttachedHandsetObject *newObj,
                const _VoiceLineAttachedHandsetObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   /* Send message to voice to update routing table */
   sendRouteChangeMsgToVoice();
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineMelodyObject( _VoiceLineMelodyObject *newObj,
                const _VoiceLineMelodyObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineDialingPrefixObject( _VoiceLineDialingPrefixObject *newObj,
                const _VoiceLineDialingPrefixObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineVolumnObject( _VoiceLineVolumnObject *newObj,
                const _VoiceLineVolumnObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineBlockedNumberObject( _VoiceLineBlockedNumberObject *newObj,
                const _VoiceLineBlockedNumberObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineMultiCallModeObject( _VoiceLineMultiCallModeObject *newObj,
                const _VoiceLineMultiCallModeObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineIntrusionCallObject( _VoiceLineIntrusionCallObject *newObj,
                const _VoiceLineIntrusionCallObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceLineCLIRObject( _VoiceLineCLIRObject *newObj,
                const _VoiceLineCLIRObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   rutVoice_dectListUpdate( DECT_DELAY_LIST_LINE_SETTINGS, INSTANCE_ID_AT_DEPTH(iidStack, 2) );
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceDectSupportListObject( _VoiceDectSupportListObject *newObj,
                const _VoiceDectSupportListObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceContactNameObject( _VoiceContactNameObject *newObj,
                const _VoiceContactNameObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceContactFirstNameObject( _VoiceContactFirstNameObject *newObj,
                const _VoiceContactFirstNameObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceContactLineIdObject( _VoiceContactLineIdObject *newObj,
                const _VoiceContactLineIdObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_voiceContactListObject( _VoiceContactListObject *newObj,
                const _VoiceContactListObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   if ( newObj != NULL &&  currObj == NULL ) /* Creation of object instance */
   {
      int  i;
      CmsRet localRet;

      /* added 3 contactNumber instances */
      for(i = 1; i <= 3; i ++)
      {
         InstanceIdStack localIidStack = *iidStack;

         PUSH_INSTANCE_ID( &localIidStack, i );
         localRet = cmsObj_addInstance(MDMOID_VOICE_CONTACT_NUMBER, &localIidStack);
         if (localRet != CMSRET_SUCCESS)
         {
            cmsLog_error("addObjectInstance: Failed, ret=%d", localRet);
            return localRet;
         }
      } 
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceContactMelodyObject( _VoiceContactMelodyObject *newObj,
                const _VoiceContactMelodyObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceContactNumberObject( _VoiceContactNumberObject *newObj,
                const _VoiceContactNumberObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallTypeObject( _VoiceCallTypeObject *newObj,
                const _VoiceCallTypeObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallNameObject( _VoiceCallNameObject *newObj,
                const _VoiceCallNameObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallNumberObject( _VoiceCallNumberObject *newObj,
                const _VoiceCallNumberObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallDateTimeObject( _VoiceCallDateTimeObject *newObj,
                const _VoiceCallDateTimeObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallLineNameObject( _VoiceCallLineNameObject *newObj,
                const _VoiceCallLineNameObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallLineIdObject( _VoiceCallLineIdObject *newObj,
                const _VoiceCallLineIdObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallNumberOfMissedCallsObject( _VoiceCallNumberOfMissedCallsObject *newObj,
                const _VoiceCallNumberOfMissedCallsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallNewFlagObject( _VoiceCallNewFlagObject *newObj,
                const _VoiceCallNewFlagObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_voiceCallListObject( _VoiceCallListObject *newObj,
                const _VoiceCallListObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_DECTENDPOINT_1 */

#endif /* BRCM_VOICE_SUPPORT */
