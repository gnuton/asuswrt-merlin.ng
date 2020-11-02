/***********************************************************************
 *
 *  Copyright (c) 2006-2010  Broadcom Corporation
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
#ifndef __RUT_PON_VOICE_H__
#define __RUT_PON_VOICE_H__

/*!\file rut_gpon_voice.h
 * \brief System level interface functions for GPON voice functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */

#include "cms.h"

//=======================  Public GPON voice functions ========================

#ifdef DMP_X_ITU_ORG_GPON_1
#ifdef DMP_X_ITU_ORG_VOICE_1

enum
{
  POTS_PORT_0 = 0,
  POTS_PORT_1,
  POTS_PORT_2,
  POTS_PORT_3,
  POTS_PORT_MAX
};


#define VOICE_STATUS_REGISTER_STR     "Registering"
#define VOICE_STATUS_REGISTER_VAL     0
#define VOICE_STATUS_ERROR_STR        "Error"
#define VOICE_STATUS_ERROR_VAL        9
#define VOICE_STATUS_DISABLED_STR     "Disabled"
#define VOICE_STATUS_DISABLED_VAL     10
#define VOICE_STATUS_UP_STR           "Up"
#define VOICE_STATUS_UP_VAL           2

#define VOICE_STATE_IDLE_STR          "Idle"
#define VOICE_STATE_IDLE_VAL          1
#define VOICE_STATE_CALLING_STR       "Calling"
#define VOICE_STATE_CALLING_VAL       3
#define VOICE_STATE_RINGING_STR       "Ringing"
#define VOICE_STATE_RINGING_VAL       4
#define VOICE_STATE_CONNECTING_STR    "Connecting"
#define VOICE_STATE_CONNECTING_VAL    6
#define VOICE_STATE_INCALL_STR        "InCall"
#define VOICE_STATE_INCALL_VAL        7
#define VOICE_STATE_DISCONNECTING_STR "Disconnecting"
#define VOICE_STATE_DISCONNECTING_VAL 8

typedef struct
{
  char* stateStrPtr;
  int stateVal;
} LINE_STATE_REC;


#define LEAF_SIZE 2
#define MAC_ADDRESS_SIZE 6
#define SW_VERSION_SIZE 32
#define SW_TIME_SIZE 32
#define IP_ADDRESS_SIZE 4
#define IAD_USERNAME_SIZE 32
#define IAD_PASSWORD_SIZE 32
#define CVLAN_SIZE 2
#define SVLAN_SIZE 2
#define PORT_SIZE 2
#define REGISTRATION_SIZE 4
#define HEARTBEAT_SIZE 2
#define PORT_STATUS_SIZE 4
#define PORT_STATE_SIZE 4
#define PORT_CODEC_MODE_SIZE 4
#define SIP_DIGITAL_MAP_SIZE 1024
#define USER_ACCT_SIZE 16
#define USER_NAME_SIZE 32
#define USER_PASS_SIZE 16
#define OAM_MAX_STR_SIZE 32

#define OAM_SIP_SUPPORTED 1


typedef struct
{
  UINT8 branch;
  UINT8 leaf[LEAF_SIZE];
  UINT8 variableWidth;
  UINT8 macAddress[MAC_ADDRESS_SIZE];
  UINT8 protocolSupported;
  UINT8 iadSwVersion[SW_VERSION_SIZE];
  UINT8 iadSwTime[SW_TIME_SIZE];
  UINT8 voipUserCount;
} iadRec;


typedef struct
{
  UINT8 branch;
  UINT8 leaf[LEAF_SIZE];
  UINT8 variableWidth;
  UINT8 voiceIpMode;
  UINT32 iadIpAddress;
  UINT32 iadNetMask;
  UINT32 iadDefaultGW;
  UINT8 pppoeMode;
  UINT8 pppoeUsername[IAD_USERNAME_SIZE];
  UINT8 pppoePassword[IAD_PASSWORD_SIZE];
  UINT8 taggedFlag;
  UINT8 voiceCVlan[CVLAN_SIZE];
  UINT8 voiceSVlan[SVLAN_SIZE];
  UINT8 voicePriority;
  UINT8 status;
  
} globalConfigRec;


typedef struct
{
  UINT8 branch;
  UINT8 leaf[LEAF_SIZE];
  UINT8 variableWidth;
  UINT8 mgPortNumber[PORT_SIZE];
  UINT8 sipProxyAddress[IP_ADDRESS_SIZE];
  UINT8 sipProxyComPort[PORT_SIZE];
  UINT8 bkupSipProxyAddress[IP_ADDRESS_SIZE];
  UINT8 bkupSipProxyComPort[PORT_SIZE];
  UINT8 activeSipProxyAddress[IP_ADDRESS_SIZE];
  UINT8 sipRegistrarAddress[IP_ADDRESS_SIZE];
  UINT8 sipRegistrarComPort[PORT_SIZE];
  UINT8 bkupSipRegistrarAddress[IP_ADDRESS_SIZE];
  UINT8 bkupSipRegistrarComPort[PORT_SIZE];
  UINT8 outboundServerAddress[IP_ADDRESS_SIZE];
  UINT8 outboundServerComPort[PORT_SIZE];
  UINT8 regRefreshCycle[REGISTRATION_SIZE];
  UINT8 heartbeatSwitch;
  UINT8 heartbeatCycle[HEARTBEAT_SIZE];
  UINT8 heartbeatCount[HEARTBEAT_SIZE];
} sipConfigRec;


typedef struct
{
  UINT8 branch;
  UINT8 leaf[LEAF_SIZE];
  UINT8 variableWidth;
  UINT8 voiceT38Enable;
  UINT8 voiceFaxModemControl;
} faxConfigRec;


typedef struct
{
  UINT8 branch;
  UINT8 leaf[LEAF_SIZE];
  UINT8 variableWidth;
  UINT8 iadPortStatus[PORT_STATUS_SIZE];
  UINT8 iadPortServiceState[PORT_STATE_SIZE];
  UINT8 iadPortCodecMode[PORT_CODEC_MODE_SIZE];
} potsStatusRec;


typedef struct
{
  UINT8 branch;
  UINT8 leaf[LEAF_SIZE];
  UINT8 variableWidth;
  UINT8 userAcct[USER_ACCT_SIZE];
  UINT8 userName[USER_NAME_SIZE];
  UINT8 userPass[USER_PASS_SIZE];
} sipUserConfigRec;


typedef struct
{
  UINT8 branch;
  UINT8 leaf[LEAF_SIZE];
  UINT8 variableWidth;
  UINT8 sipDigitalMap[SIP_DIGITAL_MAP_SIZE];
} sipDigitalMapRec;


CmsRet setVoipBoundIfNameAddress(const char* ifName, const char* address);
CmsRet GetIadInfo(int portIndex, iadRec* recPtr);
CmsRet GetGlobalConfig(int portIndex, globalConfigRec* recPtr);
CmsRet SetGlobalConfig(int portIndex, globalConfigRec* recPtr);
CmsRet GetSipConfig(int portIndex, sipConfigRec* recPtr);
CmsRet SetSipConfig(int portIndex, sipConfigRec* recPtr);
CmsRet GetFaxConfig(int portIndex, faxConfigRec* recPtr);
CmsRet SetFaxConfig(int portIndex, faxConfigRec* recPtr);
CmsRet GetPotsStatus(int portIndex, potsStatusRec* recPtr);
CmsRet GetSipUserConfig(int portIndex, sipUserConfigRec* recPtr);
CmsRet SetSipUserConfig(int portIndex, sipUserConfigRec* recPtr);
CmsRet CommandVoiceStart(const char* ifName, const char* address);
CmsRet SetSipDigitalMap(int portIndex, sipDigitalMapRec* recPtr);
CmsRet SendUploadComplete(void);
CmsRet SendRebootVoice(void);
CmsRet setVoipBoundIfNameAddress(const char *ifName, const char *address);
CmsRet setSipAuthUsername(const UINT32 port, const char* user);
CmsRet setSipAuthPassword(const UINT32 port, const char* password);
CmsRet setSipUserPartAor(const UINT32 port, const char* userPartAor);
CmsRet setSipReregHeadStartTime(const UINT32 port, const UINT32 reregVal);
CmsRet setSipRegisterExpirationTime(const UINT32 port, const UINT32 expireVal);
CmsRet setSipOutboundProxyAddress(const UINT32 port, const char *address);
CmsRet setSipProxyServerAddress(const UINT32 port, const char *address);
CmsRet setSipRegistrarAddress(const UINT32 port, const char *address);


#endif // #ifdef DMP_X_ITU_ORG_VOICE_1
#endif // #ifdef DMP_X_ITU_ORG_GPON_1
#endif /* __RUT_GPON_VOICE_H__ */
