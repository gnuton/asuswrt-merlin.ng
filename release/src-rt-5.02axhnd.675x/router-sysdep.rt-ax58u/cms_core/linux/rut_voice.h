/***********************************************************************
 *
 *  Copyright (c) 2006 - 2009  Broadcom Corporation
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

#ifndef __RUT_VOICE_H__
#define __RUT_VOICE_H__


/*!\file rut_voice.h
 * \brief System level interface functions for voice functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */



#include "cms.h"

/* Note, those #define must match the ones present in vrgEndpt.h as they are
** used for offseting the information passed on the message from VOICE to STL.
**
** Long term, we should have only one #define for those used in all modules.
*/
#ifdef DMP_X_BROADCOM_COM_DECTENDPOINT_1
#   define DECT_MAX_HANDSET_COUNT     6
#endif

#define MAX_TRANSPORTS_LIST_LENGTH   24
#define MAX_IPFAMILIES_LIST_LENGTH   16

/* Common Codec Identifiers */
#define CODEC_PCMU_STR        "G.711MuLaw"
#define CODEC_PCMA_STR        "G.711ALaw"
#define CODEC_G729_STR        "G.729a"
#define CODEC_G7231_63_STR    "G.723.1"
#define CODEC_G726_16_STR     "G.726_16"
#define CODEC_G726_24_STR     "G.726_24"
#define CODEC_G726_32_STR     "G.726_32"
#define CODEC_G726_40_STR     "G.726_40"
#define CODEC_G728_STR        "G.728"
#define CODEC_BV32_STR        "X_BROADCOM_COM_BV32"
#define CODEC_LINPCM256_STR   "X_BROADCOM_COM_PCMWIDEBAND"
#define CODEC_G722_MODE_1_STR "G.722"
#define CODEC_ILBC_30_STR     "iLBC_30"
#define CODEC_GSMAMR_12K_STR  "X_BROADCOM_COM_GSM_AMR_12K"
#define CODEC_GSMAMR_10K_STR  "X_BROADCOM_COM_GSM_AMR_10K"
#define CODEC_GSMAMR_795_STR  "X_BROADCOM_COM_GSM_AMR_795"
#define CODEC_GSMAMR_740_STR  "X_BROADCOM_COM_GSM_AMR_740"
#define CODEC_GSMAMR_670_STR  "X_BROADCOM_COM_GSM_AMR_670"
#define CODEC_GSMAMR_590_STR  "X_BROADCOM_COM_GSM_AMR_590"
#define CODEC_GSMAMR_515_STR  "X_BROADCOM_COM_GSM_AMR_515"
#define CODEC_GSMAMR_475_STR  "X_BROADCOM_COM_GSM_AMR_475"
#define CODEC_GSMAMR_OA_12K_STR  "X_BROADCOM_COM_GSM_AMR_OA_12K"
#define CODEC_GSMAMR_OA_10K_STR  "X_BROADCOM_COM_GSM_AMR_OA_10K"
#define CODEC_GSMAMR_OA_795_STR  "X_BROADCOM_COM_GSM_AMR_OA_795"
#define CODEC_GSMAMR_OA_740_STR  "X_BROADCOM_COM_GSM_AMR_OA_740"
#define CODEC_GSMAMR_OA_670_STR  "X_BROADCOM_COM_GSM_AMR_OA_670"
#define CODEC_GSMAMR_OA_590_STR  "X_BROADCOM_COM_GSM_AMR_OA_590"
#define CODEC_GSMAMR_OA_515_STR  "X_BROADCOM_COM_GSM_AMR_OA_515"
#define CODEC_GSMAMR_OA_475_STR  "X_BROADCOM_COM_GSM_AMR_OA_475"
#define CODEC_GSMEFR_STR      "GSM-EFR"
#define CODEC_AMRWB_66_STR    "X_BROADCOM_COM_AMR_WB_66"
#define CODEC_AMRWB_885_STR   "X_BROADCOM_COM_AMR_WB_885"
#define CODEC_AMRWB_1265_STR  "X_BROADCOM_COM_AMR_WB_1265"
#define CODEC_AMRWB_1425_STR  "X_BROADCOM_COM_AMR_WB_1425"
#define CODEC_AMRWB_1585_STR  "X_BROADCOM_COM_AMR_WB_1585"
#define CODEC_AMRWB_1825_STR  "X_BROADCOM_COM_AMR_WB_1825"
#define CODEC_AMRWB_1985_STR  "X_BROADCOM_COM_AMR_WB_1985"
#define CODEC_AMRWB_2305_STR  "X_BROADCOM_COM_AMR_WB_2305"
#define CODEC_AMRWB_2385_STR  "X_BROADCOM_COM_AMR_WB_2385"
#define CODEC_AMRWB_OA_66_STR    "X_BROADCOM_COM_AMR_WB_OA_66"
#define CODEC_AMRWB_OA_885_STR   "X_BROADCOM_COM_AMR_WB_OA_885"
#define CODEC_AMRWB_OA_1265_STR  "X_BROADCOM_COM_AMR_WB_OA_1265"
#define CODEC_AMRWB_OA_1425_STR  "X_BROADCOM_COM_AMR_WB_OA_1425"
#define CODEC_AMRWB_OA_1585_STR  "X_BROADCOM_COM_AMR_WB_OA_1585"
#define CODEC_AMRWB_OA_1825_STR  "X_BROADCOM_COM_AMR_WB_OA_1825"
#define CODEC_AMRWB_OA_1985_STR  "X_BROADCOM_COM_AMR_WB_OA_1985"
#define CODEC_AMRWB_OA_2305_STR  "X_BROADCOM_COM_AMR_WB_OA_2305"
#define CODEC_AMRWB_OA_2385_STR  "X_BROADCOM_COM_AMR_WB_OA_2385"
#define CODEC_OPUS_NB_5_STR      "X_BROADCOM_COM_OPUS_NB_5"
#define CODEC_OPUS_NB_10_STR     "X_BROADCOM_COM_OPUS_NB_10"
#define CODEC_OPUS_NB_20_STR     "X_BROADCOM_COM_OPUS_NB_20"
#define CODEC_OPUS_NB_40_STR     "X_BROADCOM_COM_OPUS_NB_40"
#define CODEC_OPUS_NB_60_STR     "X_BROADCOM_COM_OPUS_NB_60"
#define CODEC_OPUS_WB_5_STR      "X_BROADCOM_COM_OPUS_WB_5"
#define CODEC_OPUS_WB_10_STR     "X_BROADCOM_COM_OPUS_WB_10"
#define CODEC_OPUS_WB_20_STR     "X_BROADCOM_COM_OPUS_WB_20"
#define CODEC_OPUS_WB_40_STR     "X_BROADCOM_COM_OPUS_WB_40"
#define CODEC_OPUS_WB_60_STR     "X_BROADCOM_COM_OPUS_WB_60"
#define CODEC_ISAC_30_STR        "X_BROADCOM_COM_ISAC_30"
#define CODEC_ISAC_60_STR        "X_BROADCOM_COM_ISAC_60"
#define CODEC_T38_STR         "X_BROADCOM_COM_T38"
#define CODEC_NTE_STR         "X_BROADCOM_COM_NTE"
#define CODEC_RED_STR         "X_BROADCOM_COM_RED"
#define CODEC_UNKNOWN_STR     "unknown"

/* Data Structures */

typedef struct
{
   unsigned int   entryId;
   unsigned int   bitRate;
   char           pTime[64];
   unsigned int   silSup;
   unsigned int   priority;
   char           name[64];
} RUT_VOIP_CODEC_INIT;

typedef struct
{
   int vrgId;
   char *cmTxt;
   char *tr104Txt;
   char name[64];
} RUT_VRG_CM_TR104_LOCALE_MAP;


CmsRet sendCfgChangeMsgToVoice(void);
CmsRet sendLoggingChangeMsgToVoice(int module);
CmsRet sendRouteChangeMsgToVoice(void);
#ifdef DMP_X_BROADCOM_COM_NTR_1
CmsRet sendNtrCfgChangeMsgToVoice(void);
#endif /* DMP_X_BROADCOM_COM_NTR_1 */
UBOOL8 rutIsDectRunning(void);
UBOOL8 rutIsVoipRunning(void);
UBOOL8 rutIsLineAttachedWithDectHS( int handsetId );
int getLineInstFromCmAcntNum( int acntNum );
int getNumberOfLineInstances(void);
int assignSpNumToVpInst ( int vpInst );
int getLineInst( int iteration );
int getNumberOfSrvProv( void );
int getNumAccPerSrvProv( int spNum );
int getVpInstFromSpNum( int spNum );
int getVPInst( int iteration );
int getSpNumFromVpInst ( int vpInst );
int getLineInstFromSpAcntNum( int spNum, int acntNum );
int getVpInstFromCmAcntNum( int cmNum );
void rutVoice_getNumDectEndpt( int * value );
CmsRet rutVoice_getCodecPtime( int iid, char * pTime, unsigned int length );
CmsRet rutVoice_getCodecName( int iid, char * name, unsigned int length );
CmsRet rutVoice_getCodecEntryId( int iid, int * entryId );
CmsRet rutVoice_getCodecSilSup( int iid, int * silSup );
CmsRet rutVoice_getCodecBitRate( int iid, int * bitRate );
CmsRet rutVoice_getCodecPriority( int iid, int *priority);
CmsRet rutVoice_getMaxCodecs( int * maxCodec );
CmsRet rutVoice_getMaxVoipEndpt( int * maxVoipEndpt );
CmsRet rutVoice_getMaxPhysEndpt( int * maxPhysEndpt );
CmsRet rutVoice_getPhysEndptType( int PhyEndptId, int *type );
CmsRet rutVoice_getMaxLine( int * maxLine );
CmsRet rutVoice_getMaxLineInstances( int * maxLine );
CmsRet rutVoice_getNumLines( int vp, int * numLine );
CmsRet rutVoice_getMaxVoiceProfiles( int * maxVp );
CmsRet rutVoice_getMaxSessPerLine( int * maxSess );
CmsRet rutVoice_getNumFxsEndpt( int * numFxoEndpt );
CmsRet rutVoice_getNumFxoEndpt( int * numFxoEndpt );
CmsRet rutVoice_getSupportedLocales ( char * localeList, unsigned int length );
CmsRet rutVoice_getSupportedAlpha2Locales ( char * localeList, unsigned int length );
CmsRet rutVoice_getSupportedAlpha3Locales ( char * localeList, unsigned int length );
CmsRet rutVoice_getSupportedTransports ( char * transports, unsigned int length );
CmsRet rutVoice_getSupportedSrtpOptions ( char * options, unsigned int length );
CmsRet rutVoice_getSupportedBackToPrimOptions ( char * options, unsigned int length );
CmsRet rutVoice_getSupportedRedOptions ( char * options, unsigned int length );
CmsRet rutVoice_getSupportedConfOptions ( char * options, unsigned int length );
CmsRet rutVoice_getSupportedCodecs ( char * codecList, unsigned int length  );
CmsRet rutVoice_getNumSupportedCodecs( int * numCodecs );
CmsRet rutVoice_getDefaultAlpha3Locale ( char * locale, unsigned int length );
CmsRet rutVoice_validateAlpha3Locale ( char * locale, UBOOL8 * found );
CmsRet rutVoice_validateAlpha2Locale ( char * locale, UBOOL8 * found );
CmsRet rutVoice_mapAlpha3toAlpha2Locale ( char * locale, UBOOL8 * found, unsigned int length );
CmsRet rutVoice_validateCodec ( char * codec, UBOOL8 * found  );
CmsRet rutVoice_getMaxCnx( int * maxCnx );
CmsRet rutVoice_getSigProt( char * sigProt, unsigned int length );
CmsRet rutVoice_getSipRole( char *role, unsigned int length );
CmsRet rutVoice_getSipExtensions( char *extensions, unsigned int length );
CmsRet rutVoice_getSipTransports( char *transports, unsigned int length );
CmsRet rutVoice_getSipUriSchemes( char *uriSchemes, unsigned int length );
CmsRet rutVoice_mapAlpha2toVrg( const char *locale, int *id, UBOOL8 *found, unsigned int length );
CmsRet rutVoice_mapAlpha2toAlpha3( const char *locale, char *alpha3, UBOOL8 *found, unsigned int length );
CmsRet rutVoice_getMaxPrefCodecs( int * maxPrefCodecs );
CmsRet rutVoice_mapAcntNumToLineInst( int vpInst, int acntNum, int * lineInst );
CmsRet rutVoice_assignSpNumToVpInst( int vpInst, int * spNum );
CmsRet rutVoice_mapCmLineToVpInstLineInst( int cmNum, int * vpInst, int * lineInst );
CmsRet rutVoice_mapVpInstLineInstToCMAcnt( int vpInst, int lineInst, int * cmAcntNum );
CmsRet rutVoice_updateCMAcntNum( int vp, int lineToDelete );
CmsRet rutVoice_assignCMAcnt( void );
CmsRet rutVoice_getNumAccPerSrvProv( int spNum, int * numAcnt );
CmsRet rutVoice_updateSpNum( int vpToDelete );
CmsRet rutVoice_getNumSrvProv( int * numSp );
CmsRet rutVoice_mapSpNumToVpInst( int spNum, int * vpInst );
CmsRet rutVoice_mapFxoInterfaceIDToInst( int fxoId, int *inst );
CmsRet rutVoice_getSupportedIpFamilyList ( char * ipFamilies, unsigned int length );
CmsRet rutVoice_updateIfAddr ( const char *ifName, const char *address );

#if DMP_X_BROADCOM_COM_DECTENDPOINT_1
/** Retrieves the maximum number of DECT handset allowed to be supported
 *  on the software when the DECT interface is enabled.
 *
 * @param value (IN/OUT) the information to be retrieved.
 *
 * @return Nothing.
 */
void rutVoice_getMaxDectHset( int * value );

/** Retrieves total number of the current registered DECT handset
 *
 * @param value (IN/OUT) the information to be retrieved.
 *
 * @return Nothing.
 */
void rutVoice_getCurrDectHset( int * value );

/** Queues a delayed list update message for any changed DECT list
 *
 * @param delayId (IN) the id of the delay message, from the list in  dectctl.h
 * @param lineId (IN) the line id to which this update applies
 *
 * @return Nothing.
 */
void rutVoice_dectListUpdate( unsigned int delayId, int lineId );

#endif /* DMP_X_BROADCOM_COM_DECTENDPOINT_1 */

/** Validate faxT38Object BitRate value.
 *
 * Validate faxT38Object bitRate value.
 * Since mdm only validates string enum parameter, this function will help to validate unsigned
 * integer parameter
 *
 *
 * @param bitRate (IN) BitRate value.
 *
 * @return UBOOL8 enum. TRUE or FALSE.
 */
UBOOL8 rut_validateFaxT38BitRate(UINT32 bitRate);


/** Validate faxT38Object HighSpeedPacketRate value.
 *
 * Validate faxT38Object HighSpeedPacketRate value.
 * Since mdm only validates string enum parameter, this function will help to validate unsigned
 * integer parameter
 *
 *
 * @param bitRate (IN) HighSpeedPacketRate value.
 *
 * @return UBOOL8 enum. TRUE or FALSE.
 */
UBOOL8 rut_validateFaxT38HighSpeedPacketRate(UINT32 highSpeedPacketRate);

#endif /*RUT_VOICE_H_*/
