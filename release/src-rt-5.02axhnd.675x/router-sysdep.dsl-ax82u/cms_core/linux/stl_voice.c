/****************************************************************************
*
*  Copyright (c) 2007-2012 Broadcom Corporation
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
****************************************************************************
*
*  Filename: stl_voice.c
*
****************************************************************************
*  Description:
*
*
****************************************************************************/

#include "stl.h"
#include "cms.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "cms_mem.h"
#include "mdm.h"
#include "rut_voice.h"

#ifdef BRCM_VOICE_SUPPORT
#ifndef CMS_LOG3
#define CMS_LOG3
#endif /* CMS_LOG3 */
#include <cms_log.h>

/* -------------- Private defines ---------------- */

#define STL_MSG_RECEIVE_TIMEOUT 2000
#define STL_MSG_RECEIVE_LIMIT   2


/* Note, those #define must match the ones present in vrgEndpt.h as they are
** used for offseting the information passed on the message from VOICE to STL.
**
** Long term, we should have only one #define for those used in all modules.
*/
#ifdef DMP_X_BROADCOM_COM_DECTENDPOINT_1
#   define DECT_DATA_ACCESS_CODE_LEN      4
#   define DECT_DATA_LINK_DATE_LEN        10
#   define DECT_DATA_HANDSET_STATUS_LEN   32
#   define DECT_DATA_IPEI_LEN             5
#endif

/* -------------- Functions ---------------- */

CmsRet stl_voiceObject(_VoiceObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCapObject(_VoiceCapObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCapSipObject(_VoiceCapSipObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCapMgcpObject(_VoiceCapMgcpObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#if 0
CmsRet stl_voiceCapH323Object(_VoiceCapH323Object *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_voiceCapCodecsObject(_VoiceCapCodecsObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceProfObject(_VoiceProfObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceProfProviderObject(_VoiceProfProviderObject *obj , const InstanceIdStack *iidStack )
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceProfSipObject(_VoiceProfSipObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_voiceProfSipSubscribeObject(_VoiceProfSipSubscribeObject *obj , const InstanceIdStack *iidStack )
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#if 0
CmsRet stl_voiceProfSipResponseObject(_VoiceProfSipResponseObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
CmsRet stl_voiceProfMgcpObject(_VoiceProfMgcpObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#if 0
CmsRet stl_voiceProfH323Object(_VoiceProfH323Object *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_voiceProfRtpObject(_VoiceProfRtpObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#if 0
CmsRet stl_voiceProfRtpRtcpObject(_VoiceProfRtpRtcpObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_voiceProfRtpSrtpObject(_VoiceProfRtpSrtpObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceProfRtpRedundancyObject(_VoiceProfRtpRedundancyObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#if 0
CmsRet stl_voiceProfLineNumberingPlanObject(_VoiceProfLineNumberingPlanObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfNumberingPlanPrefixInfoObject(_VoiceProfNumberingPlanPrefixInfoObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfToneObject(_VoiceProfToneObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfToneEventObject(_VoiceProfToneEventObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfToneDescriptionObject(_VoiceProfToneDescriptionObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfTonePatternObject(_VoiceProfTonePatternObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfButtonMapObject(_VoiceProfButtonMapObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceProfButtonMapButtonObject(_VoiceProfButtonMapButtonObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_voiceProfFaxT38Object(_VoiceProfFaxT38Object *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineSipObject(_VoiceLineSipObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#if 0
CmsRet stl_voiceLineSipEventSubscribeObject(_VoiceLineSipEventSubscribeObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceLineMgcpObject(_VoiceLineMgcpObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceLineH323Object(_VoiceLineH323Object *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceLineRingerObject(_VoiceLineRingerObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceLineRingerEventObject(_VoiceLineRingerEventObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceLineRingerDescriptionObject(_VoiceLineRingerDescriptionObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#if 0
CmsRet stl_voiceLineRingerPatternObject(_VoiceLineRingerPatternObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_voiceLineCallingFeaturesObject(_VoiceLineCallingFeaturesObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineProcessingObject(_VoiceLineProcessingObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineCodecObject(_VoiceLineCodecObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineCodecListObject(_VoiceLineCodecListObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#if 0
CmsRet stl_voiceLineSessionObject(_VoiceLineSessionObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

/* Currently we update call statistics in MDM when a "voice show stats" is done in the command line,
   as this calls the DAL get function which invokes this internal function */
CmsRet stl_voiceLineStatsObject(_VoiceLineStatsObject *obj, const InstanceIdStack *iidStack )
{
   CmsMsgHeader msg = EMPTY_MSG_HEADER;
   CmsMsgHeader *reply = NULL;
   CmsRet ret;
   UINT32  cmAcntNum;
   UBOOL8 receive = TRUE;
   MdmObjectId _oid;
   UINT16 _seq;
   int recvCount = 0;
   InstanceIdStack ancestorIidStack = *iidStack;
   VoiceLineObject *lineObj = NULL;

   /* If stats object is being retreived by VOICE, no need to notify VOICE */
   if ( mdmLibCtx.eid != EID_VOICE )
   {
      if ( rutIsVoipRunning() )
      {
         if ( (ret = cmsObj_getAncestorFlags(MDMOID_VOICE_LINE, MDMOID_VOICE_LINE_STATS, &ancestorIidStack, OGF_NO_VALUE_UPDATE, (void **) &lineObj )) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not get line object, ret =%d\n", ret);
            return ret;
         }

         cmAcntNum = lineObj->X_BROADCOM_COM_CMAcntNum;
         cmsObj_free((void **) &lineObj);

         /* Get Statistics in Call Manager for specific account */
         msg.type = CMS_MSG_VOICE_STATISTICS_REQUEST;
         msg.src = mdmLibCtx.eid;
         msg.dst = EID_VOICE;
         msg.flags_request = 1;
         msg.flags.bits.bounceIfNotRunning = 1;
         msg.wordData = cmAcntNum;

         /* TR69 may try to get values while call manager is restarting. If this is the case, we have to
         ** give up the lock to make sure we don't block the call manager restart. */
            cmsLck_releaseLock();

         /* Send message to VOICE asking for call statistics */
         if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not send reset statistics message msg to Call Manager, ret= %d\n", ret);
            return ret;
         }

         if (cmsLck_acquireLockWithTimeout(1000) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get lock.");
         }

         /* Wait for reply containing statistics block with timeout. If time out occurs, exit. If we
            receive other messages, we put them back in queue and increment counter. If we exceed the
            counter, we give up */
         while ( receive )
         {
            if ( recvCount < STL_MSG_RECEIVE_LIMIT )
            {
               ret = cmsMsg_receiveWithTimeout(mdmLibCtx.msgHandle, &reply, STL_MSG_RECEIVE_TIMEOUT);
               if (ret == CMSRET_SUCCESS)
               {
                  /* Check to see if this is the expected message from VOICE, else put it back in message queue */
                  if ( reply->type == CMS_MSG_VOICE_STATISTICS_RESPONSE && reply->src == EID_VOICE )
                  {
                     /* We got the message containing statistics, set them in MDM, but make sure MdmObjectID 
                        and sequence number are not lost */
                     _oid = obj->_oid;
                     _seq = obj->_sequenceNum;
                     *obj = *((_VoiceLineStatsObject *)(reply + 1));
                     obj->_oid = _oid;
                     obj->_sequenceNum = _seq;

                     /* Free reply message and exit loop */
                     CMSMEM_FREE_BUF_AND_NULL_PTR(reply);
                     reply = NULL;
                     receive = FALSE;
                  }
                  else
                  {
                     /* Queue and wait again */
                     cmsMsg_putBack(mdmLibCtx.msgHandle, &reply);
                     reply = NULL;
                     recvCount++;
                  }
               }
               else
               {
                  cmsLog_error("Error receiving stats message, ret= %d\n", ret);
                  return ret;
               }
            }
            else
            {
               ret = CMSRET_INTERNAL_ERROR;
               cmsLog_error("Error receiving stats message, not waiting anymore, ret= %d\n", ret);
               return ret;
            }
         }
      }
      else
      {
         ret = CMSRET_SUCCESS;
      }
   }
   else
   {
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   return ret;
}

CmsRet stl_voiceLineObject(_VoiceLineObject *obj, const InstanceIdStack *iidStack )
{
   CmsRet ret;
   UINT32 cmAcntNum;

   /* Makes sure we are not receiving the same msg just sent out */
   if ( mdmLibCtx.eid != EID_VOICE )
   {
      if ( rutIsVoipRunning() )
      {
         /* Create replybuf */
         char buf[sizeof(CmsMsgHeader) + sizeof(VoiceLineObjStatus)]={0};
         CmsMsgHeader *replyBuf = (CmsMsgHeader *) buf;

         /* Create msg to send to voice */
         CmsMsgHeader msg = EMPTY_MSG_HEADER;

         VoiceLineObjStatus *lineObjStatus = NULL;

         cmAcntNum = obj->X_BROADCOM_COM_CMAcntNum;

         /* Prepare msg to send */
         msg.type = CMS_MSG_VOICE_CM_ENDPT_STATUS;
         msg.src = mdmLibCtx.eid;
         msg.dst = EID_VOICE;
         msg.flags.bits.request = 1;
         msg.flags.bits.bounceIfNotRunning = 1;
         msg.dataLength = 0;
         msg.wordData = cmAcntNum;

         /* TR69 may try to get values while call manager is restarting. If this is the case, we have to
         ** give up the lock to make sure we don't block the call manager restart. */
            cmsLck_releaseLock();

         /* Send msg to sipCli requesting line status and call state */
         if ( (ret = cmsMsg_sendAndGetReplyBufWithTimeout(mdmLibCtx.msgHandle, &msg, &replyBuf, 1000) == CMSRET_SUCCESS ) )
            {
               if (cmsLck_acquireLockWithTimeout(1000) != CMSRET_SUCCESS)
               {
                  cmsLog_error("Could not get lock, ret=%d", ret);
               }

            lineObjStatus = (VoiceLineObjStatus*) (replyBuf + 1);

            /* Note: wordData in the replyBuf indicates whether voice is successful in getting Provis values.
             * Check to see if success in getting Provis values in sipCli */
            if ( replyBuf->wordData )
            {
               /* Set status and call state to mdm */
               if( strlen(lineObjStatus->regStatus ) )
               {
                  CMSMEM_REPLACE_STRING_FLAGS( obj->status, lineObjStatus->regStatus, mdmLibCtx.allocFlags );
               }

               if( strlen( lineObjStatus->callStatus ) )
               {
                  CMSMEM_REPLACE_STRING_FLAGS( obj->callState, lineObjStatus->callStatus, mdmLibCtx.allocFlags );
               }

               ret = CMSRET_SUCCESS;
            }
            else
            {
               /* Error has occurred while getting Provis data in sipCli */
               return CMSRET_SUCCESS_OBJECT_UNCHANGED;
            }
         }
         else
         {
            if (cmsLck_acquireLockWithTimeout(1000) != CMSRET_SUCCESS)
            {
               cmsLog_error("Could not get lock, ret=%d", ret);
            }
         }
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS( obj->status, "Disabled", mdmLibCtx.allocFlags );
         CMSMEM_REPLACE_STRING_FLAGS( obj->callState, "Idle", mdmLibCtx.allocFlags );

         ret = CMSRET_SUCCESS;
      }
   }
   else
   {
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   return ret;
}

CmsRet stl_voicePhyIntfObject(_VoicePhyIntfObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voicePhyIntfTestsObject(_VoicePhyIntfTestsObject *obj , const InstanceIdStack *iidStack )
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


//#ifdef DMP_X_BROADCOM_COM_PSTNENDPOINT_1
CmsRet stl_voicePstnObject(_VoicePstnObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
//#endif

#if defined( DMP_X_BROADCOM_COM_NTR_1 )
CmsRet stl_voiceNtrObject(_VoiceNtrObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceNtrHistoryObject(_VoiceNtrHistoryObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_X_BROADCOM_COM_NTR_1 */


#ifdef DMP_X_BROADCOM_COM_DECTENDPOINT_1

CmsRet stl_voiceDectSystemSettingObject( _VoiceDectSystemSettingObject *obj, const InstanceIdStack *iidStack )
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDECTHandsetObject(_VoiceDECTHandsetObject *obj, const InstanceIdStack *iidStack)
{
   CmsMsgHeader msg = EMPTY_MSG_HEADER;
   CmsMsgHeader *reply = NULL;
   CmsRet ret;
   UBOOL8 receive = TRUE;
   int recvCount = 0;

   /* If stats object is being retreived by VOICE, no need to notify VOICE */
   if ( mdmLibCtx.eid != EID_DECT )
   {
      if ( rutIsDectRunning() )
      {
         msg.type = CMS_MSG_VOICE_DECT_HS_INFO_REQ;
         msg.src = mdmLibCtx.eid;
         msg.dst = EID_DECT;
         msg.flags_request = 1;
         msg.flags.bits.bounceIfNotRunning = 1;
         msg.wordData = obj->X_BROADCOM_COM_ID;

         /* TR69 may try to get values while call manager is restarting. If this is the case, we have to
         ** give up the lock to make sure we don't block the call manager restart. */
         cmsLck_releaseLock();

         /* Send message to VOICE asking for DECT information */
         if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not send DECT handset info message to Call Manager, ret= %d\n", ret);
            return ret;
         }

         if (cmsLck_acquireLockWithTimeout(1000) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get lock.");
         }

         /* Wait for reply containing DECT info block with timeout. If time out occurs, exit. If we
         ** receive other messages, we put them back in queue and increment counter. If we exceed the
         ** counter, we give up.
         */
         while ( receive )
         {
            if ( recvCount < STL_MSG_RECEIVE_LIMIT )
            {
               ret = cmsMsg_receiveWithTimeout(mdmLibCtx.msgHandle, &reply, STL_MSG_RECEIVE_TIMEOUT);
               if (ret == CMSRET_SUCCESS)
               {
                  /* Check to see if this is the expected message from VOICE, else put it back in message queue */
                  if ( ( reply->type == CMS_MSG_VOICE_DECT_HS_INFO_RSP ) &&
                        ( reply->src == EID_DECT ) )
                  {
                     if ( reply->dataLength )
                     {
                        char *pData = (char *) ( reply + 1 );
                        char statusBuf[BUFLEN_32];
                        char *pStatus = NULL;

                        pStatus = (char *)( pData + BUFLEN_4 );

                        memset ( statusBuf, 0, sizeof( statusBuf ) );
                        memcpy(statusBuf, pStatus, BUFLEN_32);

                        /* Cleanup old strings from obj */
                        CMSMEM_REPLACE_STRING_FLAGS( obj->status,
                                                     statusBuf,
                                                     mdmLibCtx.allocFlags );


                        /* Free reply message and exit loop */
                        CMSMEM_FREE_BUF_AND_NULL_PTR(reply);
                        receive = FALSE;
                     }
                     else
                     {
                        /* We received an empty message back. In this case,
                           try to use the default values in MDM, if they exist. */
                        CMSMEM_FREE_BUF_AND_NULL_PTR(reply);
                        return CMSRET_SUCCESS_OBJECT_UNCHANGED;
                     }
                  }
                  else
                  {
                     /* Queue and wait again */
                     cmsMsg_putBack(mdmLibCtx.msgHandle, &reply);
                     recvCount++;
                  }
               }
               else
               {
                  cmsLog_error("Error receiving DECT handset info message, ret= %d\n", ret);
                  return ret;
               }
            }
            else
            {
               ret = CMSRET_INTERNAL_ERROR;
               cmsLog_error("Error receiving DECT handset info message, not waiting anymore, ret= %d\n", ret);
               return ret;
            }
         }
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS( obj->status, "Disabled", mdmLibCtx.allocFlags );

         ret = CMSRET_SUCCESS;
      }
   }
   else
   {
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   return ret;
}

CmsRet stl_voiceLineNameObject(_VoiceLineNameObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineSettingObject(_VoiceLineSettingObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineIntrusionCallObject(_VoiceLineIntrusionCallObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineMultiCallModeObject(_VoiceLineMultiCallModeObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineMelodyObject(_VoiceLineMelodyObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineAttachedHandsetObject(_VoiceLineAttachedHandsetObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineDialingPrefixObject(_VoiceLineDialingPrefixObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectFirmwareVersionObject(_VoiceDectFirmwareVersionObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectHardwareVersionObject(_VoiceDectHardwareVersionObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectEEPROMVersionObject(_VoiceDectEEPROMVersionObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineCLIRObject(_VoiceLineCLIRObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectResetBaseObject(_VoiceDectResetBaseObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectClockMasterObject(_VoiceDectClockMasterObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectPinCodeObject(_VoiceDectPinCodeObject *obj __attribute__((unused)), const InstanceIdStack *iiStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineVolumnObject(_VoiceLineVolumnObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineBlockedNumberObject(_VoiceLineBlockedNumberObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceLineDectLineIdObject(_VoiceLineDectLineIdObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceDectSupportListObject(_VoiceDectSupportListObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_voiceContactNameObject( _VoiceContactNameObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceContactFirstNameObject( _VoiceContactFirstNameObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceContactLineIdObject( _VoiceContactLineIdObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_voiceContactListObject( _VoiceContactListObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceContactMelodyObject( _VoiceContactMelodyObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceContactNumberObject( _VoiceContactNumberObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallTypeObject( _VoiceCallTypeObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallNameObject( _VoiceCallNameObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallNumberObject( _VoiceCallNumberObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallDateTimeObject( _VoiceCallDateTimeObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallLineNameObject( _VoiceCallLineNameObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallLineIdObject( _VoiceCallLineIdObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallNumberOfMissedCallsObject( _VoiceCallNumberOfMissedCallsObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallNewFlagObject( _VoiceCallNewFlagObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallListObject( _VoiceCallListObject *obj __attribute((unused)), const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif /* DMP_X_BROADCOM_COM_DECTENDPOINT_1 */

#endif /* BRCM_VOICE_SUPPORT */
