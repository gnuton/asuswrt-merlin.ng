/****************************************************************************
*
*  Copyright (c) 2007-2012 Broadcom Corporation
*
<:label-BRCM:2016:proprietary:standard

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
*  Filename: stl2_voice.c
*
****************************************************************************
*  Description:
*
*
****************************************************************************/

#ifdef BRCM_VOICE_SUPPORT
#ifdef DMP_VOICE_SERVICE_2
#include "stl.h"
#include "cms.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "cms_mem.h"
#include "mdm.h"
#include "rut_voice.h"

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

CmsRet stl_voiceObject(_VoiceObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCapObject(_VoiceCapObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCapSipObject(_VoiceCapSipObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCapSipClientObject(_VoiceCapSipClientObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCapPotsObject(_VoiceCapPotsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCapCodecsObject(_VoiceCapCodecsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_qualityIndicatorObject(_QualityIndicatorObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceReservedPortsObject(_VoiceReservedPortsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceServicePotsObject(_VoiceServicePotsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_potsRingerObject(_PotsRingerObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_pOTSFxoObject(_POTSFxoObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_pOTSFxsObject(_POTSFxsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceProcessingObject(_VoiceProcessingObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_diagTestsObject(_DiagTestsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_fXODiagTestsObject(_FXODiagTestsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dECTObject(_DECTObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dECTPortableObject(_DECTPortableObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dECTBaseObject(_DECTBaseObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceServiceSipObject(_VoiceServiceSipObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_sipClientObject(_SipClientObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_sIPClientContactObject(_SIPClientContactObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_sIPNetworkObject(_SIPNetworkObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_sIPNetworkResponseMapObject(_SIPNetworkResponseMapObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callControlObject(_CallControlObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callControlCallingFeaturesObject(_CallControlCallingFeaturesObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callingFeaturesSetObject(_CallingFeaturesSetObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callControlNumberingPlanObject(_CallControlNumberingPlanObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callControlPrefixInfoObject(_CallControlPrefixInfoObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callControlIncomingMapObject(_CallControlIncomingMapObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callControlOutgoingMapObject(_CallControlOutgoingMapObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callControlExtensionObject(_CallControlExtensionObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_extensionStatsObject(_ExtensionStatsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_extensionIncomingCallsObject(_ExtensionIncomingCallsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_extensionOutgoingCallsObject(_ExtensionOutgoingCallsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_extensionRtpStatsObject(_ExtensionRtpStatsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_extensionDspStatsObject(_ExtensionDspStatsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callControlLineObject(_CallControlLineObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_lineStatsObject(_LineStatsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_lineIncomingCallsObject(_LineIncomingCallsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_lineOutgoingCallsObject(_LineOutgoingCallsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_lineRtpStatsObject(_LineRtpStatsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_lineDspStatsObject(_LineDspStatsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceCallLogObject(_VoiceCallLogObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callLogSessionObject(_CallLogSessionObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callLogSessionStatsObject(_CallLogSessionStatsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callLogSessionDestinationObject(_CallLogSessionDestinationObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_destinationVoiceQualityObject(_DestinationVoiceQualityObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_sessionDestinationDspObject(_SessionDestinationDspObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_destinationDspReceiveCodecObject(_DestinationDspReceiveCodecObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_destinationDspTransmitCodecObject(_DestinationDspTransmitCodecObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_sessionDestinationRtpObject(_SessionDestinationRtpObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_sessionSourceObject(_SessionSourceObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_sourceVoiceQualityObject(_SourceVoiceQualityObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_sessionSourceDspObject(_SessionSourceDspObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_sourceDSPReceiveCodecObject(_SourceDSPReceiveCodecObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_sourceDSPTransmitCodecObject(_SourceDSPTransmitCodecObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_sessionSourceRtpObject(_SessionSourceRtpObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_callLogSignalingPerformanceObject(_CallLogSignalingPerformanceObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voIPProfileObject(_VoIPProfileObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voIPProfileRTPObject(_VoIPProfileRTPObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voIPProfileFaxT38Object(_VoIPProfileFaxT38Object *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voIPProfileRTPRedundancyObject(_VoIPProfileRTPRedundancyObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voIPProfileSRTPObject(_VoIPProfileSRTPObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voIPProfileRTCPObject(_VoIPProfileRTCPObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_codecProfileObject(_CodecProfileObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_voiceServiceContactObject(_VoiceServiceContactObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_contactNumberObject(_ContactNumberObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}



#endif /* DMP_VOICE_SERVICE_2 */
#endif /* BRCM_VOICE_SUPPORT */
