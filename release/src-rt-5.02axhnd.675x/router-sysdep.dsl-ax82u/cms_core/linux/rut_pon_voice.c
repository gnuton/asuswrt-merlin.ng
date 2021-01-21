/****************************************************************************
*
* <:copyright-BRCM:2011:proprietary:standard
*
*    Copyright (c) 2011 Broadcom
*    All Rights Reserved
*
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
*
*  Except as expressly set forth in the Authorized License,
*
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
*
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
*
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*
****************************************************************************/

#ifdef DMP_X_ITU_ORG_GPON_1
#ifdef DMP_X_ITU_ORG_VOICE_1

#include "cms_core.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "rut_voice.h"
#include "rut_pon_voice.h"
#include "rcl.h"
#include "dal_voice.h"

#define debugPrint(fmt, arg...)


LINE_STATE_REC lineStatusArray[] = {
  {VOICE_STATUS_REGISTER_STR, VOICE_STATUS_REGISTER_VAL},
  {VOICE_STATUS_ERROR_STR, VOICE_STATUS_ERROR_VAL},
  {VOICE_STATUS_DISABLED_STR, VOICE_STATUS_DISABLED_VAL},
  {VOICE_STATUS_UP_STR, VOICE_STATUS_UP_VAL}
};

LINE_STATE_REC lineCallStateArray[] = {
  {VOICE_STATE_IDLE_STR, VOICE_STATE_IDLE_VAL},
  {VOICE_STATE_CALLING_STR, VOICE_STATE_CALLING_VAL},
  {VOICE_STATE_RINGING_STR, VOICE_STATE_RINGING_VAL},
  {VOICE_STATE_CONNECTING_STR, VOICE_STATE_CONNECTING_VAL},
  {VOICE_STATE_INCALL_STR, VOICE_STATE_INCALL_VAL},
  {VOICE_STATE_DISCONNECTING_STR, VOICE_STATE_DISCONNECTING_VAL}
};

#ifdef DMP_X_ITU_ORG_VOICE_SIP_1
#ifdef DMP_VOICE_SERVICE_1
static CmsRet getVoiceLineSipObject
    (const UINT32 portIndex,
     VoiceLineSipObject **ppObjLineSip,
     InstanceIdStack *pIddLineSip)
{
    void *objService = NULL, *objVoice = NULL, *objLine = NULL;
    UBOOL8 found = FALSE;
    UINT32 voiceLineIndex = 0;
    InstanceIdStack iidService = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack iidVoice = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack iidLine = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_OBJECT_NOT_FOUND;

    // Get MDMOID_SERVICES object.
    ret = cmsObj_get(MDMOID_SERVICES, &iidService, 0, (void **)&objService);
    if (ret != CMSRET_SUCCESS)
        goto out;

    // Get MDMOID_VOICE object.
    ret = cmsObj_getNextInSubTree(MDMOID_VOICE, &iidService, &iidVoice, (void **)&objVoice);
    if (ret != CMSRET_SUCCESS)
        goto out;

     // Get MDMOID_VOICE_LINE object.
    while ((!found) &&
           ((ret = cmsObj_getNextInSubTree(MDMOID_VOICE_LINE, &iidVoice, &iidLine,
                                          (void **)&objLine)) == CMSRET_SUCCESS))
    {
        found = (voiceLineIndex == portIndex);
        if (found)
        {
            // Get MDMOID_VOICE_LINE_SIP object.
            ret = cmsObj_getNextInSubTree(MDMOID_VOICE_LINE_SIP,
                                          &iidLine, pIddLineSip,
                                          (void **)ppObjLineSip);
        }
        else
            voiceLineIndex++;
        cmsObj_free((void **)&objLine);
    }

out:
    if (objService != NULL)
        cmsObj_free((void **)&objService);
    if (objVoice != NULL)
        cmsObj_free((void **)&objVoice);

    return ret;
}
#else

/* DMP_VOICE_SERVICE_2 */
static CmsRet getVoiceSipClientObject
    (const UINT32 portIndex,
     SipClientObject **ppObjLineSip,
     InstanceIdStack *pIddLineSip)
{
    void *objService = NULL, *objVoice = NULL, *objLine = NULL;
    UBOOL8 found = FALSE;
    UINT32 voiceLineIndex = 0;
    InstanceIdStack iidService = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack iidVoice = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack iidSipClient = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_OBJECT_NOT_FOUND;

    // Get MDMOID_SERVICES object.
    ret = cmsObj_get(MDMOID_SERVICES, &iidService, 0, (void **)&objService);
    if (ret != CMSRET_SUCCESS)
        goto out;

    // Get MDMOID_VOICE object.
    ret = cmsObj_getNextInSubTree(MDMOID_VOICE, &iidService, &iidVoice, (void **)&objVoice);
    if (ret != CMSRET_SUCCESS)
        goto out;

     // Get MDMOID_SIP_CLIENT object.
    while ((!found) &&
           ((ret = cmsObj_getNextInSubTree(MDMOID_SIP_CLIENT, &iidVoice, &iidSipClient,
                                          (void **)&objLine)) == CMSRET_SUCCESS))
    {
        found = (voiceLineIndex == portIndex);
        if (found)
        {
            *ppObjLineSip = objLine;
        }
        else
        {
            voiceLineIndex++;
            cmsObj_free(&objLine);
        }
    }

out:
    if (objService != NULL)
        cmsObj_free((void **)&objService);
    if (objVoice != NULL)
        cmsObj_free((void **)&objVoice);

    return ret;
}
#endif /* DMP_VOICE_SERVICE_1 */
#endif /* DMP_X_ITU_ORG_VOICE_SIP_1 */

CmsRet setVoipBoundIfNameAddress
    (const char *ifName,
     const char *address)
{
    UBOOL8 change = FALSE;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    VoiceObject *objVoice;
    CmsRet ret = CMSRET_SUCCESS;

    // Get MDMOID_VOICE object.
    ret = cmsObj_getNext(MDMOID_VOICE, &iidStack, (void**)&objVoice);

    // Test for valid MDMOID_VOICE object.
    if (ret == CMSRET_SUCCESS)
    {
        if (objVoice->X_BROADCOM_COM_BoundIfName != NULL)
        {
            if (strcmp(objVoice->X_BROADCOM_COM_BoundIfName, ifName) != 0)
            {
                // Setup bound interface name pointer.
                CMSMEM_REPLACE_STRING_FLAGS(objVoice->X_BROADCOM_COM_BoundIfName, ifName, ALLOC_SHARED_MEM);
                change = TRUE;
            }
        }
        else
        {
            // Setup bound interface name pointer.
            CMSMEM_REPLACE_STRING_FLAGS(objVoice->X_BROADCOM_COM_BoundIfName, ifName, ALLOC_SHARED_MEM);
            change = TRUE;
        }
        if (objVoice->X_BROADCOM_COM_BoundIpAddr != NULL)
        {
            if (strcmp(objVoice->X_BROADCOM_COM_BoundIpAddr, address) != 0)
            {
                // Setup bound interface address pointer.
                CMSMEM_REPLACE_STRING_FLAGS(objVoice->X_BROADCOM_COM_BoundIpAddr, address, ALLOC_SHARED_MEM);
                change = TRUE;
            }
        }
        else
        {
            // Setup bound interface address pointer.
            CMSMEM_REPLACE_STRING_FLAGS(objVoice->X_BROADCOM_COM_BoundIpAddr, address, ALLOC_SHARED_MEM);
            change = TRUE;
        }
        // Write modified-object back to CMS.
        if (change == TRUE)
            ret = cmsObj_set(objVoice, &iidStack);

        // Free MDMOID_VOICE object.
        cmsObj_free((void**)&objVoice);
    }

    debugPrint("===> setVoipBoundIfNameAddress, ifName=%s, address=%s, ret=%d\n",
                   ifName, address, ret);
    return ret;
}

#ifdef DMP_X_ITU_ORG_VOICE_SIP_1
CmsRet setSipAuthUsername
    (const UINT32 port,
     const char* user)
{
#ifdef DMP_VOICE_SERVICE_1
    UBOOL8 change = FALSE;
    VoiceLineSipObject *pObjLineSip = NULL;
    InstanceIdStack iidLineSip = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_REQUEST_DENIED;

    // Get appropriate port-based line object.
    ret = getVoiceLineSipObject(port, &pObjLineSip, &iidLineSip);

    // Test for success.
    if (ret == CMSRET_SUCCESS && pObjLineSip != NULL)
    {
        if (pObjLineSip->authUserName != NULL)
        {
            if (strcmp(pObjLineSip->authUserName, user) != 0)
            {
                // Setup authUserName pointer.
                CMSMEM_REPLACE_STRING_FLAGS(pObjLineSip->authUserName, user, ALLOC_SHARED_MEM);
                change = TRUE;
            }
        }
        else
        {
            // Setup authUserName pointer.
            CMSMEM_REPLACE_STRING_FLAGS(pObjLineSip->authUserName, user, ALLOC_SHARED_MEM);
            change = TRUE;
        }
        // Write modified-object back to CMS.
        if (change == TRUE)
            ret = cmsObj_set(pObjLineSip, &iidLineSip);
        // Free MDMOID_VOICE_LINE_SIP object.
        cmsObj_free((void**)&pObjLineSip);
    }

    debugPrint("===> setSipAuthUsername, port=%d, username=%s, ret=%d\n",
               port, user, ret);

    return ret;
#else
    return CMSRET_SUCCESS;
#endif /* DMP_VOICE_SERVICE_1 */
}

CmsRet setSipAuthPassword
    (const UINT32 port,
     const char* password)
{
#ifdef DMP_VOICE_SERVICE_1
    UBOOL8 change = FALSE;
    VoiceLineSipObject *pObjLineSip = NULL;
    InstanceIdStack iidLineSip = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_REQUEST_DENIED;

    // Get appropriate port-based line object.
    ret = getVoiceLineSipObject(port, &pObjLineSip, &iidLineSip);

    // Test for success.
    if (ret == CMSRET_SUCCESS && pObjLineSip != NULL)
    {
        if (pObjLineSip->authPassword != NULL)
        {
            if (strcmp(pObjLineSip->authPassword, password) != 0)
            {
                // Setup authPassword pointer.
                CMSMEM_REPLACE_STRING_FLAGS(pObjLineSip->authPassword, password, ALLOC_SHARED_MEM);
                change = TRUE;
            }
        }
        else
        {
            // Setup authPassword pointer.
            CMSMEM_REPLACE_STRING_FLAGS(pObjLineSip->authPassword, password, ALLOC_SHARED_MEM);
            change = TRUE;
        }
        // Write modified-object back to CMS.
        if (change == TRUE)
            ret = cmsObj_set(pObjLineSip, &iidLineSip);
        // Free MDMOID_VOICE_LINE_SIP object.
        cmsObj_free((void**)&pObjLineSip);
    }

    debugPrint("===> setSipAuthPassword, port=%d, password=%s, ret = %d\n",
               port, password, ret);

    return ret;
#else
    return CMSRET_SUCCESS;
#endif /* DMP_VOICE_SERVICE_1 */
}

CmsRet setSipUserPartAor
    (const UINT32 port,
     const char* userPartAor)
{
#ifdef DMP_VOICE_SERVICE_1
    UBOOL8 change = FALSE;
    VoiceLineSipObject *pObjLineSip = NULL;
    InstanceIdStack iidLineSip = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_REQUEST_DENIED;

    // Get appropriate port-based line object.
    ret = getVoiceLineSipObject(port, &pObjLineSip, &iidLineSip);

    // Test for success.
    if (ret == CMSRET_SUCCESS && pObjLineSip != NULL)
    {
        if (pObjLineSip->URI != NULL)
        {
            if (strcmp(pObjLineSip->URI, userPartAor) != 0)
            {
                // Setup userPartAor pointer.
                CMSMEM_REPLACE_STRING_FLAGS(pObjLineSip->URI, userPartAor, ALLOC_SHARED_MEM);
                change = TRUE;
            }
        }
        else
        {
            // Setup userPartAor pointer.
            CMSMEM_REPLACE_STRING_FLAGS(pObjLineSip->URI, userPartAor, ALLOC_SHARED_MEM);
            change = TRUE;
        }
        // Write modified-object back to CMS.
        if (change == TRUE)
            ret = cmsObj_set(pObjLineSip, &iidLineSip);
        // Free MDMOID_VOICE_LINE_SIP object.
        cmsObj_free((void**)&pObjLineSip);
    }

    debugPrint("===> setSipUserPartAor, port=%d, userPartAor=%s, ret = %d\n",
               port, userPartAor, ret);

    return ret;
#else
    return CMSRET_SUCCESS;
#endif /* DMP_VOICE_SERVICE_1 */
}

CmsRet setSipReregHeadStartTime
    (const UINT32 port __attribute__((unused)),
     const UINT32 reregVal __attribute__((unused)))
{
#ifdef DMP_VOICE_SERVICE_1
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    VoiceProfSipObject* sip = NULL;
    CmsRet ret = CMSRET_SUCCESS;

    // Get MDMOID_VOICE_PROF_SIP object.
    ret = cmsObj_getNext(MDMOID_VOICE_PROF_SIP, &iidStack, (void**)&sip);

    // Test for valid MDMOID_VOICE_PROF_SIP object.
    if (ret == CMSRET_SUCCESS)
    {
        if (sip->registerRetryInterval != reregVal)
        {
            // Setup SIP rereg head start time.
            sip->registerRetryInterval = reregVal;
            // Write modified-object back to CMS.
            ret = cmsObj_set(sip, &iidStack);
        }
        // Free MDMOID_VOICE_PROF_SIP object.
        cmsObj_free((void**)&sip);
    }

    debugPrint("===> setSipRegisterExpirationTime, port=%d, reregVal=%d, ret = %d\n",
               port, reregVal, ret);

    return ret;
#else
    return CMSRET_SUCCESS;
#endif /* DMP_VOICE_SERVICE_1 */
}

CmsRet setSipRegisterExpirationTime
    (const UINT32 port __attribute__((unused)),
     const UINT32 expireVal __attribute__((unused)))
{
#ifdef DMP_VOICE_SERVICE_1
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    VoiceProfSipObject* sip = NULL;
    CmsRet ret = CMSRET_SUCCESS;

    // Get MDMOID_VOICE_PROF_SIP object.
    ret = cmsObj_getNext(MDMOID_VOICE_PROF_SIP, &iidStack, (void**)&sip);

    // Test for valid MDMOID_VOICE_PROF_SIP object.
    if (ret == CMSRET_SUCCESS)
    {
        if (sip->registerExpires != expireVal)
        {
            // Setup SIP registration expiration time in seconds.
            sip->registerExpires = expireVal;
            // Write modified-object back to CMS.
            ret = cmsObj_set(sip, &iidStack);
        }
        // Free MDMOID_VOICE_PROF_SIP object.
        cmsObj_free((void**)&sip);
    }

    debugPrint("===> setSipRegisterExpirationTime, port=%d, expireVal=%d, ret = %d\n",
               port, expireVal, ret);

    return ret;
#else
    return CMSRET_SUCCESS;
#endif /* DMP_VOICE_SERVICE_1 */
}

CmsRet setSipOutboundProxyAddress
    (const UINT32 port __attribute__((unused)),
     const char *address __attribute__((unused)))
{
#ifdef DMP_VOICE_SERVICE_1
    UBOOL8 change = FALSE;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    VoiceProfSipObject* sip = NULL;
    CmsRet ret = CMSRET_SUCCESS;

    // Get MDMOID_VOICE_PROF_SIP object.
    ret = cmsObj_getNext(MDMOID_VOICE_PROF_SIP, &iidStack, (void**)&sip);

    // Test for valid MDMOID_VOICE_PROF_SIP object.
    if (ret == CMSRET_SUCCESS)
    {
        if (sip->outboundProxy != NULL)
        {
            if (strcmp(sip->outboundProxy, address) != 0)
            {
                // Setup outbound proxy address pointer.
                CMSMEM_REPLACE_STRING_FLAGS(sip->outboundProxy, address, ALLOC_SHARED_MEM);
                change = TRUE;
            }
        }
        else
        {
            // Setup outbound proxy address pointer.
            CMSMEM_REPLACE_STRING_FLAGS(sip->outboundProxy, address, ALLOC_SHARED_MEM);
            change = TRUE;
        }
        // Write modified-object back to CMS.
        if (change == TRUE)
            ret = cmsObj_set(sip, &iidStack);
        // Free MDMOID_VOICE_PROF_SIP object.
        cmsObj_free((void**)&sip);
    }

    debugPrint("===> setSipOutboundProxyAddress, port=%d, address=%s, ret = %d\n",
               port, address, ret);

    return ret;
#else
    return CMSRET_SUCCESS;
#endif /* DMP_VOICE_SERVICE_1 */
}

CmsRet setSipProxyServerAddress
    (const UINT32 port __attribute__((unused)),
     const char *address __attribute__((unused)))
{
#ifdef DMP_VOICE_SERVICE_1
    UBOOL8 change = FALSE;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    VoiceProfSipObject* sip = NULL;
    CmsRet ret = CMSRET_SUCCESS;

    // Get MDMOID_VOICE_PROF_SIP object.
    ret = cmsObj_getNext(MDMOID_VOICE_PROF_SIP, &iidStack, (void**)&sip);

    // Test for valid MDMOID_VOICE_PROF_SIP object.
    if (ret == CMSRET_SUCCESS)
    {
        if (sip->proxyServer != NULL)
        {
            if (strcmp(sip->proxyServer, address) != 0)
            {
                // Setup proxy server address pointer.
                CMSMEM_REPLACE_STRING_FLAGS(sip->proxyServer, address, ALLOC_SHARED_MEM);
                change = TRUE;
            }
        }
        else
        {
            // Setup proxy server address pointer.
            CMSMEM_REPLACE_STRING_FLAGS(sip->proxyServer, address, ALLOC_SHARED_MEM);
            change = TRUE;
        }
        // Write modified-object back to CMS.
        if (change == TRUE)
            ret = cmsObj_set(sip, &iidStack);
        // Free MDMOID_VOICE_PROF_SIP object.
        cmsObj_free((void**)&sip);
    }

    debugPrint("===> setSipProxyServerAddress, port=%d, address=%s, ret = %d\n",
               port, address, ret);

    return ret;
#else
    return CMSRET_SUCCESS;
#endif /* DMP_VOICE_SERVICE_1 */
}

CmsRet setSipRegistrarAddress
    (const UINT32 port __attribute__((unused)),
     const char *address __attribute__((unused)))
{
#ifdef DMP_VOICE_SERVICE_1
    UBOOL8 change = FALSE;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    VoiceProfSipObject* sip = NULL;
    CmsRet ret = CMSRET_SUCCESS;

    // Get MDMOID_VOICE_PROF_SIP object.
    ret = cmsObj_getNext(MDMOID_VOICE_PROF_SIP, &iidStack, (void**)&sip);

    // Test for valid MDMOID_VOICE_PROF_SIP object.
    if (ret == CMSRET_SUCCESS)
    {
        if (sip->registrarServer != NULL)
        {
            if (strcmp(sip->registrarServer, address) != 0)
            {
                // Setup SIP registrar address pointer.
                CMSMEM_REPLACE_STRING_FLAGS(sip->registrarServer, address, ALLOC_SHARED_MEM);
                change = TRUE;
            }
        }
        else
        {
            // Setup SIP registrar address pointer
            CMSMEM_REPLACE_STRING_FLAGS(sip->registrarServer, address, ALLOC_SHARED_MEM);
            change = TRUE;
        }
        // Write modified-object back to CMS.
        if (change == TRUE)
            ret = cmsObj_set(sip, &iidStack);
        // Free MDMOID_VOICE_PROF_SIP object.
        cmsObj_free((void**)&sip);
    }

    debugPrint("===> setSipRegistrarAddress, port=%d, address=%s, ret = %d\n",
               port, address, ret);

    return ret;
#else
    return CMSRET_SUCCESS;
#endif /* DMP_VOICE_SERVICE_1 */
}
#endif // DMP_X_ITU_ORG_VOICE_SIP_1

CmsRet SendUploadComplete(void)
{
  CmsRet cmsResult;
  CmsMsgHeader msgHdr = EMPTY_MSG_HEADER;

  msgHdr.src = mdmLibCtx.eid;
  msgHdr.dst = EID_SMD;
  msgHdr.type = CMS_MSG_CONFIG_UPLOAD_COMPLETE;
  msgHdr.flags_event = 1;

  cmsResult = cmsMsg_send(mdmLibCtx.msgHandle, &msgHdr);
  if (cmsResult != CMSRET_SUCCESS)
  {
    cmsLog_error("Send message failure, cmsResult: %d", cmsResult);
  }

  debugPrint("===> SendUploadComplete\n");

  // Return CMS result.
  return cmsResult;
}


//*******************************************************************
//*******************************************************************
//*******************************************************************
// EPON voice routines start here...

static void SetupSwVersion(char* versStrPtr, iadRec* recPtr)
{
  int stringLength;

  // Clear software version string field.
  memset(&recPtr->iadSwVersion, 0, SW_VERSION_SIZE);

  // Get software version string length to LSB-align.
  stringLength = strlen(versStrPtr);

  // Test for valid software version string.
  if ((stringLength > 0) && (stringLength <= SW_VERSION_SIZE))
  {
    // Copy software version string into field's LSBs.
    memcpy(&recPtr->iadSwVersion[SW_VERSION_SIZE - stringLength], versStrPtr, stringLength);
  }
}


static void SetupSwBuildTime(char* buildTimeStrPtr, iadRec* recPtr)
{
  unsigned int stringLength;
  unsigned int buildDate;
  unsigned int buildTime;
  char tempChar;
  char tempStr[SW_TIME_SIZE];

  // Split build date rom build time.
  sscanf(buildTimeStrPtr, "%d%c%d", &buildDate, &tempChar, &buildTime);

  // Form new time string in OAM format, adding full year to build date and seconds to build time.
  sprintf(tempStr, "20%d%d00", buildDate, buildTime);

  // Clear software build-time string field.
  memset(&recPtr->iadSwTime, 0, SW_TIME_SIZE);

  // Get software version string length to LSB-align.
  stringLength = strlen(tempStr);

  // Test for valid software version string.
  if ((stringLength > 0) && (stringLength <= SW_VERSION_SIZE))
  {
    // Copy software version string into field's LSBs.
    memcpy(&recPtr->iadSwTime[SW_VERSION_SIZE - stringLength], tempStr, stringLength);
  }
}


#ifdef DMP_X_ITU_ORG_VOICE_SIP_1
static UINT32 Form32BitValueFromByteArray(UINT8* charArrayPtr)
{
  // Setup 32-bit value from byte array which may not be word-aligned.
  return ((charArrayPtr[0] << 24) | (charArrayPtr[1] << 16) | (charArrayPtr[2] << 8) | charArrayPtr[3]);
}


static void SetLsbAlignedString(char* srcStr, char* destStr, int strSize)
{
  int stringLength;

  // Clear LSB-aligned destination string.
  memset(destStr, 0, strSize);

  // Get source string length.
  stringLength = strlen(srcStr);

  // Test for valid software version string.
  if ((stringLength > 0) && (stringLength <= strSize))
  {
    // Copy string into field's LSBs.
    memcpy(&destStr[strSize - stringLength], srcStr, stringLength);
  }
}


static void GetLsbAlignedString(char* srcStr, char* destStr, int strSize)
{
  int loopIndex;

  // Loop through source string to find first non-zero character and copy to destination string.
  for (loopIndex = 0;loopIndex < strSize;loopIndex++)
  {
    // Test for non-zero characters.
    if (srcStr[loopIndex] != 0)
    {
      // Copy non-zero character to destination buffer and post-inc index.
      *destStr++ = srcStr[loopIndex];
    }
  }
}
#endif // DMP_X_ITU_ORG_VOICE_SIP_1


static CmsRet GetPotsPortStatus(int portIndex, UINT32* resultPtr)
{
  CmsRet cmsResult = CMSRET_REQUEST_DENIED;
  void* objService;
  VoiceObject* voiceObjPtr;
#ifdef DMP_VOICE_SERVICE_1
  VoiceLineObject* voiceLineObjPtr;
  UBOOL8 stateFound = FALSE;
#else
  SipClientObject* voiceLineObjPtr;
#endif
  UBOOL8 lineFound = FALSE;
  UINT32 voiceLineIndex = 0;
  UINT32 loopIndex;
  InstanceIdStack iidService = EMPTY_INSTANCE_ID_STACK;
  InstanceIdStack iidVoice = EMPTY_INSTANCE_ID_STACK;
  InstanceIdStack iidLine = EMPTY_INSTANCE_ID_STACK;

  // Get MDMOID_SERVICES object & test result.
  if (cmsObj_get(MDMOID_SERVICES, &iidService, 0, (void **)&objService) == CMSRET_SUCCESS)
  {
    // Get MDMOID_VOICE object & test result.
    if (cmsObj_getNextInSubTree(MDMOID_VOICE, &iidService, &iidVoice, (void **)&voiceObjPtr) == CMSRET_SUCCESS)
    {
#ifdef DMP_VOICE_SERVICE_1
      // Get MDMOID_VOICE_LINE object.
      while ((lineFound == FALSE) && (cmsObj_getNextInSubTree(MDMOID_VOICE_LINE, &iidVoice, &iidLine, (void **)&voiceLineObjPtr) == CMSRET_SUCCESS))
#else
      // Get MDMOID_SIP_CLIENT object.
      while ((lineFound == FALSE) && (cmsObj_getNextInSubTree(MDMOID_SIP_CLIENT, &iidVoice, &iidLine, (void **)&voiceLineObjPtr) == CMSRET_SUCCESS))
#endif
      {
        // Set line found flag.
        lineFound = (voiceLineIndex++ == (UINT32)portIndex);

        // Test if specified line has been found.
        if (lineFound == TRUE)
        {
          // Loop through line's status string array attempting to find match.
          for (loopIndex = 0;loopIndex < (sizeof(lineStatusArray) / sizeof(LINE_STATE_REC));loopIndex++)
          {
            // Test for matching string.
            if (strcasecmp(lineStatusArray[loopIndex].stateStrPtr, voiceLineObjPtr->status) == 0)
            {
              // Setup match's state.
              *resultPtr = lineStatusArray[loopIndex].stateVal;

#ifdef DMP_VOICE_SERVICE_1
              // Signal match found.
              stateFound = TRUE;
#endif

              // Signal success.
              cmsResult = CMSRET_SUCCESS;

              // Done with loop.
              break;
            }
          }

#ifdef DMP_VOICE_SERVICE_1
          // Test if the line status has not been found
          if (stateFound == FALSE)
          {
            // Loop through line's call state string array attempting to find match.
            for (loopIndex = 0;loopIndex < (sizeof(lineCallStateArray) / sizeof(LINE_STATE_REC));loopIndex++)
            {
              // Test for matching string.
              if (strcasecmp(lineCallStateArray[loopIndex].stateStrPtr, voiceLineObjPtr->callState) == 0)
              {
                // Setup match's state.
                *resultPtr = lineCallStateArray[loopIndex].stateVal;

                // Signal match found.
                stateFound = TRUE;

                // Signal success.
                cmsResult = CMSRET_SUCCESS;

                // Done with loop.
                break;
              }
            }
          }
#endif /* DMP_VOICE_SERVICE_1 */
        }

        // Release MDMOID_VOICE_LINE object.
        cmsObj_free((void **)&voiceLineObjPtr);
      }

      // Release MDMOID_VOICE object.
      cmsObj_free((void **)&voiceObjPtr);
    }

    // Release MDMOID_SERVICES object.
    cmsObj_free((void **)&objService);
  }

  // Return CMS result.
  return cmsResult;
}


static CmsRet GetPotsPortCodecMode(int portIndex __attribute__((unused)), UINT32* resultPtr __attribute__((unused)))
{
  CmsRet cmsResult = CMSRET_SUCCESS;

  //*******************************************************************
  //*******************************************************************
  //*******************************************************************
  // NEEDS WORK HERE
  //*******************************************************************
  //*******************************************************************
  //*******************************************************************

  // Return CMS result.
  return cmsResult;
}


static CmsRet GetPotsPortServiceState(int portIndex __attribute__((unused)), UINT32* resultPtr __attribute__((unused)))
{
  CmsRet cmsResult = CMSRET_SUCCESS;

  //*******************************************************************
  //*******************************************************************
  //*******************************************************************
  // NEEDS WORK HERE
  //*******************************************************************
  //*******************************************************************
  //*******************************************************************

  // Return CMS result.
  return cmsResult;
}


#ifdef DMP_X_ITU_ORG_VOICE_SIP_1
#ifdef DMP_VOICE_SERVICE_1
static CmsRet SetupSipUser(int portIndex, char* acctStr, char* nameStr, char* passStr)
{
  VoiceLineSipObject* voiceLineSipObjectPtr = NULL;
  InstanceIdStack iidLineSip = EMPTY_INSTANCE_ID_STACK;
  CmsRet cmsResult;

  // Get appropriate port-based line object.
  cmsResult = getVoiceLineSipObject(portIndex, &voiceLineSipObjectPtr, &iidLineSip);

  // Test for success.
  if ((cmsResult == CMSRET_SUCCESS) && (voiceLineSipObjectPtr != NULL))
  {
    // Setup username string.
    CMSMEM_REPLACE_STRING_FLAGS(voiceLineSipObjectPtr->authUserName, nameStr, ALLOC_SHARED_MEM);

    // Setup password string.
    CMSMEM_REPLACE_STRING_FLAGS(voiceLineSipObjectPtr->authPassword, passStr, ALLOC_SHARED_MEM);

    // Setup account string.
    CMSMEM_REPLACE_STRING_FLAGS(voiceLineSipObjectPtr->URI, acctStr, ALLOC_SHARED_MEM);

    // Write modified-object back to CMS.
    cmsResult = cmsObj_set(voiceLineSipObjectPtr, &iidLineSip);

    // Free MDMOID_VOICE_LINE_SIP object.
    cmsObj_free((void**)&voiceLineSipObjectPtr);
  }

  // Return CMS result.
  return cmsResult;
}
#else /* DMP_VOICE_SERVICE_1 */

/* DMP_VOICE_SERVICE_2 */
static CmsRet SetupSipUser(int portIndex, char* acctStr, char* nameStr, char* passStr)
{
  SipClientObject* voiceSipClientObjectPtr = NULL;
  InstanceIdStack iidLineSip = EMPTY_INSTANCE_ID_STACK;
  CmsRet cmsResult;

  // Get appropriate port-based line object.
  cmsResult = getVoiceSipClientObject(portIndex, &voiceSipClientObjectPtr, &iidLineSip);

  // Test for success.
  if ((cmsResult == CMSRET_SUCCESS) && (voiceSipClientObjectPtr != NULL))
  {
    // Setup username string.
    CMSMEM_REPLACE_STRING_FLAGS(voiceSipClientObjectPtr->authUserName, nameStr, ALLOC_SHARED_MEM);

    // Setup password string.
    CMSMEM_REPLACE_STRING_FLAGS(voiceSipClientObjectPtr->authPassword, passStr, ALLOC_SHARED_MEM);

    // Setup account string.
    CMSMEM_REPLACE_STRING_FLAGS(voiceSipClientObjectPtr->registerURI, acctStr, ALLOC_SHARED_MEM);

    // Write modified-object back to CMS.
    cmsResult = cmsObj_set(voiceSipClientObjectPtr, &iidLineSip);

    // Free MDMOID_VOICE_LINE_SIP object.
    cmsObj_free((void**)&voiceSipClientObjectPtr);
  }

  // Return CMS result.
  return cmsResult;
}
#endif /* DMP_VOICE_SERVICE_1 */

#ifdef DMP_VOICE_SERVICE_1
static CmsRet GetSipUser(int portIndex, char* acctStr, char* nameStr, char* passStr, int maxStrSize)
{
  CmsRet cmsResult;
  VoiceLineSipObject* voiceLineSipObjectPtr = NULL;
  InstanceIdStack iidLineSip = EMPTY_INSTANCE_ID_STACK;

  // Get appropriate port-based line object.
  cmsResult = getVoiceLineSipObject(portIndex, &voiceLineSipObjectPtr, &iidLineSip);

  // Test for success.
  if ((cmsResult == CMSRET_SUCCESS) && (voiceLineSipObjectPtr != NULL))
  {
    // Copy username string to destination.
    strncpy(nameStr, voiceLineSipObjectPtr->authUserName, maxStrSize);

    // Copy password string to destination.
    strncpy(passStr, voiceLineSipObjectPtr->authPassword, maxStrSize);

    // Copy account string to destination.
    strncpy(acctStr, voiceLineSipObjectPtr->URI, maxStrSize);

    // Free MDMOID_VOICE_LINE_SIP object.
    cmsObj_free((void**)&voiceLineSipObjectPtr);
  }

  // Return CMS result.
  return cmsResult;
}
#else /* DMP_VOICE_SERVICE_1 */

/* DMP_VOICE_SERVICE_2 */
static CmsRet GetSipUser(int portIndex, char* acctStr, char* nameStr, char* passStr, int maxStrSize)
{
  CmsRet cmsResult;
  SipClientObject* voiceSipClientObjectPtr = NULL;
  InstanceIdStack iidLineSip = EMPTY_INSTANCE_ID_STACK;

  // Get appropriate port-based line object.
  cmsResult = getVoiceSipClientObject(portIndex, &voiceSipClientObjectPtr, &iidLineSip);

  // Test for success.
  if ((cmsResult == CMSRET_SUCCESS) && (voiceSipClientObjectPtr != NULL))
  {
    // Copy username string to destination.
    strncpy(nameStr, voiceSipClientObjectPtr->authUserName, maxStrSize);

    // Copy password string to destination.
    strncpy(passStr, voiceSipClientObjectPtr->authPassword, maxStrSize);

    // Copy account string to destination.
    strncpy(acctStr, voiceSipClientObjectPtr->registerURI, maxStrSize);

    // Free MDMOID_VOICE_LINE_SIP object.
    cmsObj_free((void**)&voiceSipClientObjectPtr);
  }

  // Return CMS result.
  return cmsResult;
}
#endif /* DMP_VOICE_SERVICE_1 */


static CmsRet SetupSipOutboundProxyAddress(int portIndex __attribute__((unused)), char* addrStr, UINT32 portValue)
{
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
#ifdef DMP_VOICE_SERVICE_1
  VoiceProfSipObject* sipObjPtr;
#else
  SIPNetworkObject* sipObjPtr;
#endif /* DMP_VOICE_SERVICE_1 */
  CmsRet cmsResult;

  // Get MDMOID_VOICE_PROF_SIP object.
#ifdef DMP_VOICE_SERVICE_1
  cmsResult = cmsObj_getNext(MDMOID_VOICE_PROF_SIP, &iidStack, (void**)&sipObjPtr);
#else
  cmsResult = cmsObj_getNext(MDMOID_SIP_NETWORK, &iidStack, (void**)&sipObjPtr);
#endif /* DMP_VOICE_SERVICE_1 */

  // Test for valid MDMOID_VOICE_PROF_SIP object.
  if (cmsResult == CMSRET_SUCCESS)
  {
    // Setup outbound proxy address pointer.
    CMSMEM_REPLACE_STRING_FLAGS(sipObjPtr->outboundProxy, addrStr, ALLOC_SHARED_MEM);

    // Setup outbound proxy port.
    sipObjPtr->outboundProxyPort = portValue;

    // Write modified-object back to CMS.
    cmsResult = cmsObj_set(sipObjPtr, &iidStack);

    // Free MDMOID_VOICE_PROF_SIP object.
    cmsObj_free((void**)&sipObjPtr);
  }

  // Return CMS result.
  return cmsResult;
}


static CmsRet SetupSipProxyServerAddress(int portIndex __attribute__((unused)), char* addrStr, UINT32 portValue)
{
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
#ifdef DMP_VOICE_SERVICE_1
  VoiceProfSipObject* sipObjPtr;
#else
  SIPNetworkObject* sipObjPtr;
#endif /* DMP_VOICE_SERVICE_1 */
  CmsRet cmsResult;

  // Get MDMOID_VOICE_PROF_SIP object.
#ifdef DMP_VOICE_SERVICE_1
  cmsResult = cmsObj_getNext(MDMOID_VOICE_PROF_SIP, &iidStack, (void**)&sipObjPtr);
#else
  cmsResult = cmsObj_getNext(MDMOID_SIP_NETWORK, &iidStack, (void**)&sipObjPtr);
#endif /* DMP_VOICE_SERVICE_1 */

  // Test for valid MDMOID_VOICE_PROF_SIP object.
  if (cmsResult == CMSRET_SUCCESS)
  {
    // Setup outbound proxy address pointer.
    CMSMEM_REPLACE_STRING_FLAGS(sipObjPtr->proxyServer, addrStr, ALLOC_SHARED_MEM);

    // Setup proxy server port.
    sipObjPtr->proxyServerPort = portValue;

    // Write modified-object back to CMS.
    cmsResult = cmsObj_set(sipObjPtr, &iidStack);

    // Free MDMOID_VOICE_PROF_SIP object.
    cmsObj_free((void**)&sipObjPtr);
  }

  return cmsResult;
}


static CmsRet SetupSipRegistrarAddress(int portIndex __attribute__((unused)), char* addrStr, UINT32 portValue)
{
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
#ifdef DMP_VOICE_SERVICE_1
  VoiceProfSipObject* sipObjPtr;
#else
  SIPNetworkObject* sipObjPtr;
#endif /* DMP_VOICE_SERVICE_1 */
  CmsRet cmsResult;

  // Get MDMOID_VOICE_PROF_SIP object.
#ifdef DMP_VOICE_SERVICE_1
  cmsResult = cmsObj_getNext(MDMOID_VOICE_PROF_SIP, &iidStack, (void**)&sipObjPtr);
#else
  cmsResult = cmsObj_getNext(MDMOID_SIP_NETWORK, &iidStack, (void**)&sipObjPtr);
#endif /* DMP_VOICE_SERVICE_1 */

  // Test for valid MDMOID_VOICE_PROF_SIP object.
  if (cmsResult == CMSRET_SUCCESS)
  {
    // Setup SIP registrar address pointer.
    CMSMEM_REPLACE_STRING_FLAGS(sipObjPtr->registrarServer, addrStr, ALLOC_SHARED_MEM);

    // Setup registrar port.
    sipObjPtr->registrarServerPort = portValue;

    // Write modified-object back to CMS.
    cmsResult = cmsObj_set(sipObjPtr, &iidStack);

    // Free MDMOID_VOICE_PROF_SIP object.
    cmsObj_free((void**)&sipObjPtr);
  }

  return cmsResult;
}


static CmsRet SetupSipReregHeadStartTime(int portIndex __attribute__((unused)), UINT32 reregVal)
{
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
#ifdef DMP_VOICE_SERVICE_1
  VoiceProfSipObject* sipObjPtr;
#else
  SIPNetworkObject* sipObjPtr;
#endif /* DMP_VOICE_SERVICE_1 */
  CmsRet cmsResult;

  // Get MDMOID_VOICE_PROF_SIP object.
#ifdef DMP_VOICE_SERVICE_1
  cmsResult = cmsObj_getNext(MDMOID_VOICE_PROF_SIP, &iidStack, (void**)&sipObjPtr);
#else
  cmsResult = cmsObj_getNext(MDMOID_SIP_NETWORK, &iidStack, (void**)&sipObjPtr);
#endif /* DMP_VOICE_SERVICE_1 */

  // Test for valid MDMOID_VOICE_PROF_SIP object.
  if (cmsResult == CMSRET_SUCCESS)
  {
    // Setup SIP rereg head start time.
    sipObjPtr->registerRetryInterval = reregVal;

    // Write modified-object back to CMS.
    cmsResult = cmsObj_set(sipObjPtr, &iidStack);

    // Free MDMOID_VOICE_PROF_SIP object.
    cmsObj_free((void**)&sipObjPtr);
  }

  return cmsResult;
}
#endif // DMP_X_ITU_ORG_VOICE_SIP_1


CmsRet GetIadInfo(int portIndex __attribute__((unused)), iadRec* recPtr)
{
  CmsRet cmsResult;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  LanEthIntfObject* lanEthIntfObjPtr;
  IGDDeviceInfoObject* igdDeviceInfoObjPtr;
  VoiceCapObject* voiceCapObjPtr;
  unsigned char tempchar;
  unsigned int intArray[6];

  // Get first MDMOID_LAN_ETH_INTF object.
  cmsResult = cmsObj_getNext(MDMOID_LAN_ETH_INTF, &iidStack, (void**)&lanEthIntfObjPtr);

  // Test for valid MDMOID_LAN_ETH_INTF object.
  if (cmsResult == CMSRET_SUCCESS)
  {
    // Format MAC address string like '12:34:56:78:9A:BC'.
    sscanf(lanEthIntfObjPtr->MACAddress, "%x%c%x%c%x%c%x%c%x%c%x", &intArray[0], &tempchar, &intArray[1], &tempchar,
                                                                   &intArray[2], &tempchar, &intArray[3], &tempchar,
                                                                   &intArray[4], &tempchar, &intArray[5]);

    // Transfer MAC address into record.
    recPtr->macAddress[0] = (UINT8)intArray[0];
    recPtr->macAddress[1] = (UINT8)intArray[1];
    recPtr->macAddress[2] = (UINT8)intArray[2];
    recPtr->macAddress[3] = (UINT8)intArray[3];
    recPtr->macAddress[4] = (UINT8)intArray[4];
    recPtr->macAddress[5] = (UINT8)intArray[5];

    // Free MDMOID_LAN_ETH_INTF object.
    cmsObj_free((void**)&lanEthIntfObjPtr);

    // Setup voice protocol type.
    recPtr->protocolSupported = OAM_SIP_SUPPORTED;

    // Clear stack.
    INIT_INSTANCE_ID_STACK(&iidStack);

    // Get first MDMOID_IGD_DEVICE_INFO object.
    cmsResult = cmsObj_getNext(MDMOID_IGD_DEVICE_INFO, &iidStack, (void**)&igdDeviceInfoObjPtr);

    // Test for valid MDMOID_IGD_DEVICE_INFO object.
    if (cmsResult == CMSRET_SUCCESS)
    {
      // Copy software version string into field's LSBs.
      SetupSwVersion(igdDeviceInfoObjPtr->softwareVersion, recPtr);

      // Copy software build-time string into field's LSBs.
      SetupSwBuildTime(igdDeviceInfoObjPtr->X_BROADCOM_COM_SwBuildTimestamp, recPtr);

      // Clear stack.
      INIT_INSTANCE_ID_STACK(&iidStack);

      // Get first MDMOID_VOICE_CAP object.
      cmsResult = cmsObj_getNext(MDMOID_VOICE_CAP, &iidStack, (void**)&voiceCapObjPtr);

      // Test for valid MDMOID_VOICE_CAP object.
      if (cmsResult == CMSRET_SUCCESS)
      {
        // Setup VoIP POTS count.
        recPtr->voipUserCount = (UINT8)voiceCapObjPtr->maxLineCount;

        // Free MDMOID_VOICE_CAP object.
        cmsObj_free((void**)&voiceCapObjPtr);
      }

      // Free MDMOID_IGD_DEVICE_INFO object.
      cmsObj_free((void**)&igdDeviceInfoObjPtr);
    }
  }

  // Return cumulative CMS result.
  return cmsResult;
}


CmsRet GetGlobalConfig(int portIndex __attribute__((unused)), globalConfigRec* recPtr __attribute__((unused)))
{
  CmsRet cmsResult = CMSRET_SUCCESS;

  //*******************************************************************
  //*******************************************************************
  //*******************************************************************
  // NEEDS WORK HERE
  //*******************************************************************
  //*******************************************************************
  //*******************************************************************

  // Return CMS result.
  return cmsResult;
}


CmsRet SetGlobalConfig(int portIndex __attribute__((unused)), globalConfigRec* recPtr __attribute__((unused)))
{
  CmsRet cmsResult = CMSRET_SUCCESS;

  //*******************************************************************
  //*******************************************************************
  //*******************************************************************
  // NEEDS WORK HERE
  //*******************************************************************
  //*******************************************************************
  //*******************************************************************

  // Return CMS result.
  return cmsResult;
}


#ifdef DMP_X_ITU_ORG_VOICE_SIP_1
CmsRet GetSipConfig(int portIndex __attribute__((unused)), sipConfigRec* recPtr)
{
  CmsRet cmsResult;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
#ifdef DMP_VOICE_SERVICE_1
  VoiceProfSipObject* sipObjPtr;
#else
  SIPNetworkObject* sipObjPtr;
#endif /* DMP_VOICE_SERVICE_1 */
  unsigned char tempChar;
  unsigned int intArray[4];

  // Init SIP configuration record.
  memset(recPtr, 0, sizeof(sipConfigRec));

  // Get MDMOID_VOICE_PROF_SIP object.
#ifdef DMP_VOICE_SERVICE_1
  cmsResult = cmsObj_getNext(MDMOID_VOICE_PROF_SIP, &iidStack, (void**)&sipObjPtr);
#else
  cmsResult = cmsObj_getNext(MDMOID_SIP_NETWORK, &iidStack, (void**)&sipObjPtr);
#endif /* DMP_VOICE_SERVICE_1 */

  // Test for valid MDMOID_VOICE_PROF_SIP object.
  if (cmsResult == CMSRET_SUCCESS)
  {
    // Test for valid SIP proxy server string.
    if (strlen(sipObjPtr->proxyServer) > 0)
    {
      // Format SIP proxy server address like '12.34.56.78'.
      sscanf(sipObjPtr->proxyServer, "%d%c%d%c%d%c%d", &intArray[0], &tempChar, &intArray[1], &tempChar,
                                                       &intArray[2], &tempChar, &intArray[3]);
    }

    // Transfer SIP proxy server address into record.
    recPtr->sipProxyAddress[0] = (UINT8)intArray[0];
    recPtr->sipProxyAddress[1] = (UINT8)intArray[1];
    recPtr->sipProxyAddress[2] = (UINT8)intArray[2];
    recPtr->sipProxyAddress[3] = (UINT8)intArray[3];

    // Get SIP proxy server port.
    recPtr->sipProxyComPort[0] = (UINT8)((sipObjPtr->proxyServerPort >> 8) & 0xFF);
    recPtr->sipProxyComPort[1] = (UINT8)(sipObjPtr->proxyServerPort & 0xFF);

    // Test for valid SIP registrar string.
    if (strlen(sipObjPtr->registrarServer) > 0)
    {
      // Format SIP registrar address like '12.34.56.78'.
      sscanf(sipObjPtr->registrarServer, "%d%c%d%c%d%c%d", &intArray[0], &tempChar, &intArray[1], &tempChar,
                                                           &intArray[2], &tempChar, &intArray[3]);
    }

    // Transfer SIP registrar address into record.
    recPtr->sipRegistrarAddress[0] = (UINT8)intArray[0];
    recPtr->sipRegistrarAddress[1] = (UINT8)intArray[1];
    recPtr->sipRegistrarAddress[2] = (UINT8)intArray[2];
    recPtr->sipRegistrarAddress[3] = (UINT8)intArray[3];

    // Get SIP registrar port.
    recPtr->sipRegistrarComPort[0] = (UINT8)((sipObjPtr->registrarServerPort >> 8) & 0xFF);
    recPtr->sipRegistrarComPort[1] = (UINT8)(sipObjPtr->registrarServerPort & 0xFF);

    // Test for valid SIP outbound server string.
    if (strlen(sipObjPtr->outboundProxy) > 0)
    {
      // Format SIP outbound server address like '12.34.56.78'.
      sscanf(sipObjPtr->outboundProxy, "%d%c%d%c%d%c%d", &intArray[0], &tempChar, &intArray[1], &tempChar,
                                                         &intArray[2], &tempChar, &intArray[3]);
    }

    // Transfer SIP outbound server address into record.
    recPtr->outboundServerAddress[0] = (UINT8)intArray[0];
    recPtr->outboundServerAddress[1] = (UINT8)intArray[1];
    recPtr->outboundServerAddress[2] = (UINT8)intArray[2];
    recPtr->outboundServerAddress[3] = (UINT8)intArray[3];

    // Get SIP outbound server port.
    recPtr->outboundServerComPort[0] = (UINT8)((sipObjPtr->outboundProxyPort >> 8) & 0xFF);
    recPtr->outboundServerComPort[1] = (UINT8)(sipObjPtr->outboundProxyPort & 0xFF);

    // Get SIP registration refresh cycle.
    memcpy(recPtr->regRefreshCycle, &sipObjPtr->registerRetryInterval, REGISTRATION_SIZE);

    // Free MDMOID_VOICE_PROF_SIP object.
    cmsObj_free((void**)&sipObjPtr);
  }

  // Return CMS result.
  return cmsResult;
}


CmsRet SetSipConfig(int portIndex, sipConfigRec* recPtr)
{
  CmsRet cmsResult = CMSRET_REQUEST_DENIED;
  char tempStr[20];
  UINT32 temp32;

  // Form SIP proxy IP address string from binary values.
  sprintf(tempStr, "%d.%d.%d.%d", recPtr->sipProxyAddress[0], recPtr->sipProxyAddress[1],
          recPtr->sipProxyAddress[2], recPtr->sipProxyAddress[3]);

  // Setup 32-bit value from byte array which may not be word-aligned.
  temp32 = Form32BitValueFromByteArray(recPtr->sipProxyComPort);

  // Configure SIP proxy server and port.
  if (SetupSipProxyServerAddress(portIndex, tempStr, temp32) == CMSRET_SUCCESS)
  {
    // Form SIP registrar IP address string from binary values.
    sprintf(tempStr, "%d.%d.%d.%d", recPtr->sipRegistrarAddress[0], recPtr->sipRegistrarAddress[1],
            recPtr->sipRegistrarAddress[2], recPtr->sipRegistrarAddress[3]);

    // Setup 32-bit value from byte array which may not be word-aligned.
    temp32 = Form32BitValueFromByteArray(recPtr->sipRegistrarComPort);

    // Configure SIP registrar server and port.
    if (SetupSipRegistrarAddress(portIndex, tempStr, temp32) == CMSRET_SUCCESS)
    {
      // Form Outbound server IP address string from binary values.
      sprintf(tempStr, "%d.%d.%d.%d", recPtr->outboundServerAddress[0], recPtr->outboundServerAddress[1],
              recPtr->outboundServerAddress[2], recPtr->outboundServerAddress[3]);

      // Setup 32-bit value from byte array which may not be word-aligned.
      temp32 = Form32BitValueFromByteArray(recPtr->outboundServerComPort);

      // Configure Outbound server and port.
      if (SetupSipOutboundProxyAddress(portIndex, tempStr, temp32) == CMSRET_SUCCESS)
      {
        // Setup 32-bit value from byte array which may not be word-aligned.
        temp32 = Form32BitValueFromByteArray(recPtr->regRefreshCycle);

        // Setup SIP registration refresh cycle.
        cmsResult = SetupSipReregHeadStartTime(portIndex, temp32);
      }
    }
  }

  // Return cumulative CMS result.
  return cmsResult;
}
#endif // DMP_X_ITU_ORG_VOICE_SIP_1

#ifdef DMP_VOICE_SERVICE_1
CmsRet GetFaxConfig(int portIndex __attribute__((unused)), faxConfigRec* recPtr)
{
  CmsRet cmsResult;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  VoiceProfFaxT38Object* faxObjPtr;

  // Init SIP configuration record.
  memset(recPtr, 0, sizeof(faxConfigRec));

  // Get MDMOID_VOICE_PROF_FAX_T38 object.
  cmsResult = cmsObj_getNext(MDMOID_VOICE_PROF_FAX_T38, &iidStack, (void**)&faxObjPtr);

  // Test for valid MDMOID_VOICE_PROF_FAX_T38 object.
  if (cmsResult == CMSRET_SUCCESS)
  {
    // Test CMS FAX record.
    if (faxObjPtr->enable == FALSE)
    {
      // Clear FAX enabled flag.
      recPtr->voiceT38Enable = FALSE;
    }
    else
    {
      // Set FAX enabled flag.
      recPtr->voiceT38Enable = TRUE;
    }

    // Set FAX enabled flag.
    recPtr->voiceFaxModemControl = 0;

    // Free MDMOID_VOICE_PROF_FAX_T38 object.
    cmsObj_free((void**)&faxObjPtr);
  }

  // Return CMS result.
  return cmsResult;
}
#else

/* DMP_VOICE_SERVICE_2 */ 
CmsRet GetFaxConfig(int portIndex, faxConfigRec* recPtr)
{
    CmsRet cmsResult;
    UINT32 voiceLineIndex = 0;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack iidSipClient = EMPTY_INSTANCE_ID_STACK;
    SipClientObject* objLine = NULL;
    UBOOL8 found = FALSE;

    // Init SIP configuration record.
    memset(recPtr, 0, sizeof(faxConfigRec));
    while ((!found) &&
         ((cmsResult = cmsObj_getNextInSubTree(MDMOID_SIP_CLIENT, &iidStack, &iidSipClient,
                                        (void **)&objLine)) == CMSRET_SUCCESS))
    {
        found = (voiceLineIndex++ == portIndex);
        if (found && cmsResult == CMSRET_SUCCESS)
        {
            if (objLine->T38Enable )
            {
                // Clear FAX enabled flag.
                recPtr->voiceT38Enable = TRUE;
            }
            else
            {
                // Set FAX enabled flag.
                recPtr->voiceT38Enable = TRUE;
            }

            // Set FAX enabled flag.
            recPtr->voiceFaxModemControl = 0;
        }
        cmsObj_free((void**)&objLine);
    }

    // Return CMS result.
    return cmsResult;
}
#endif /* DMP_VOICE_SERVICE_1 */

#ifdef DMP_VOICE_SERVICE_1
CmsRet SetFaxConfig(int portIndex __attribute__((unused)), faxConfigRec* recPtr)
{
  CmsRet cmsResult;
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  VoiceProfFaxT38Object* faxObjPtr;

  // Get MDMOID_VOICE_PROF_FAX_T38 object.
  cmsResult = cmsObj_getNext(MDMOID_VOICE_PROF_FAX_T38, &iidStack, (void**)&faxObjPtr);

  // Test for valid MDMOID_VOICE_PROF_FAX_T38 object.
  if (cmsResult == CMSRET_SUCCESS)
  {
    // Test CMS FAX record.
    if (recPtr->voiceT38Enable == FALSE)
    {
      // Clear FAX enabled flag.
      faxObjPtr->enable = FALSE;
    }
    else
    {
      // Set FAX enabled flag.
      faxObjPtr->enable = TRUE;
    }

    // Write modified-object back to CMS.
    cmsResult = cmsObj_set(faxObjPtr, &iidStack);

    // Free MDMOID_VOICE_PROF_FAX_T38 object.
    cmsObj_free((void**)&faxObjPtr);
  }

  // Return CMS result.
  return cmsResult;
}
#else

/* DMP_VOICE_SERVICE_2 */ 
CmsRet SetFaxConfig(int portIndex, faxConfigRec* recPtr)
{
    CmsRet cmsResult;
    UINT32 voiceLineIndex = 0;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack iidSipClient = EMPTY_INSTANCE_ID_STACK;
    SipClientObject* objLine = NULL;
    UBOOL8 found = FALSE;

    // Get MDMOID_SIP_CLIENT object.
    while ((!found) &&
         ((cmsResult = cmsObj_getNextInSubTree(MDMOID_SIP_CLIENT, &iidStack, &iidSipClient,
                                        (void **)&objLine)) == CMSRET_SUCCESS))
    {
        found = (voiceLineIndex++ == portIndex);
        if ( found )
        {
            if (recPtr->voiceT38Enable == FALSE)
            {
                // Clear FAX enabled flag.
                objLine->T38Enable = FALSE;
            }
            else
            {
                // Set FAX enabled flag.
                objLine->T38Enable = TRUE;
            }

            // Write modified-object back to CMS.
            cmsResult = cmsObj_set(objLine, &iidSipClient);
        }
        cmsObj_free((void**)&objLine);
    }

    // Return CMS result.
    return cmsResult;
}
#endif /* DMP_VOICE_SERVICE_1 */

CmsRet GetPotsStatus(int portIndex, potsStatusRec* recPtr)
{
  CmsRet cmsResult;
  UINT32 portStatus;
  UINT32 portServiceState;
  UINT32 portCodecMode;

  // Init POTS status record.
  memset(recPtr, 0, sizeof(potsStatusRec));

  // Get port's status.
  cmsResult = GetPotsPortStatus(portIndex, &portStatus);

  // Test for success.
  if (cmsResult == CMSRET_SUCCESS)
  {
    // Copy status into output record.
    memcpy(&recPtr->iadPortStatus, &portStatus, PORT_STATUS_SIZE);

    // Get port's service state.
    cmsResult = GetPotsPortServiceState(portIndex, &portServiceState);

    // Test for success.
    if (cmsResult == CMSRET_SUCCESS)
    {
      // Copy status into output record.
      memcpy(&recPtr->iadPortServiceState, &portServiceState, PORT_STATE_SIZE);

      // Get port's codec mode.
      cmsResult = GetPotsPortCodecMode(portIndex, &portCodecMode);

      // Test for success.
      if (cmsResult == CMSRET_SUCCESS)
      {
        // Copy status into output record.
        memcpy(&recPtr->iadPortCodecMode, &portCodecMode, PORT_CODEC_MODE_SIZE);
      }
    }
  }

  // Return CMS result.
  return cmsResult;
}


#ifdef DMP_X_ITU_ORG_VOICE_SIP_1
CmsRet GetSipUserConfig(int portIndex, sipUserConfigRec* recPtr)
{
  CmsRet cmsResult;
  char acctStr[OAM_MAX_STR_SIZE];
  char nameStr[OAM_MAX_STR_SIZE];
  char passStr[OAM_MAX_STR_SIZE];

  // Initialize user string arrays.
  memset(acctStr, 0, sizeof(acctStr));
  memset(nameStr, 0, sizeof(nameStr));
  memset(passStr, 0, sizeof(passStr));

  // Get SIP user account, user name, and user password strings.
  cmsResult = GetSipUser(portIndex, acctStr, nameStr, passStr, OAM_MAX_STR_SIZE);

  // Test for valid strings.
  if (cmsResult == CMSRET_SUCCESS)
  {
    // Set LSB-aligned user account string.
    SetLsbAlignedString(acctStr, (char*)recPtr->userAcct, USER_ACCT_SIZE);

    // Set LSB-aligned user name string.
    SetLsbAlignedString(nameStr, (char*)recPtr->userName, USER_NAME_SIZE);

    // Set LSB-aligned user password string.
    SetLsbAlignedString(passStr, (char*)recPtr->userPass, USER_PASS_SIZE);
  }

  // Return CMS result.
  return cmsResult;
}


CmsRet SetSipUserConfig(int portIndex, sipUserConfigRec* recPtr)
{
  CmsRet cmsResult;
  char acctStr[USER_ACCT_SIZE + 2];
  char nameStr[USER_NAME_SIZE + 2];
  char passStr[USER_PASS_SIZE + 2];

  // Get LSB-aligned user account string.
  memset(acctStr, 0, sizeof(acctStr));
  GetLsbAlignedString((char*)&recPtr->userAcct, acctStr, USER_ACCT_SIZE);

  // Get LSB-aligned user name string.
  memset(nameStr, 0, sizeof(nameStr));
  GetLsbAlignedString((char*)&recPtr->userName, nameStr, USER_NAME_SIZE);

  // Get LSB-aligned user password string.
  memset(passStr, 0, sizeof(passStr));
  GetLsbAlignedString((char*)&recPtr->userPass, passStr, USER_PASS_SIZE);

  // Setup SIP user account, user name, and user password strings.
  cmsResult = SetupSipUser(portIndex, acctStr, nameStr, passStr);

  // Return CMS result.
  return cmsResult;
}
#endif // DMP_X_ITU_ORG_VOICE_SIP_1


CmsRet CommandVoiceStart(const char* ifName, const char* address)
{
  CmsRet cmsResult = CMSRET_SUCCESS;

  // Start voice stack.
  setVoipBoundIfNameAddress(ifName, address);

  // Return CMS result.
  return cmsResult;
}


#ifdef DMP_X_ITU_ORG_VOICE_SIP_1

#ifdef DMP_VOICE_SERVICE_1
CmsRet SetSipDigitalMap(int portIndex __attribute__((unused)), sipDigitalMapRec* recPtr)
{
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  VoiceProfObject* voiceProfObjectPtr;
  CmsRet cmsResult;

  // Get MDMOID_VOICE_PROF object.
  cmsResult = cmsObj_getNext(MDMOID_VOICE_PROF, &iidStack, (void**)&voiceProfObjectPtr);

  // Test for valid MDMOID_VOICE_PROF object.
  if (cmsResult == CMSRET_SUCCESS)
  {
     // Setup digit map from OAM input array.
     CMSMEM_REPLACE_STRING_FLAGS(voiceProfObjectPtr->digitMap, (void*)&recPtr->sipDigitalMap, ALLOC_SHARED_MEM);

     // Enable digit map.
     voiceProfObjectPtr->digitMapEnable = TRUE;

     // Write modified-object back to CMS.
     cmsResult = cmsObj_set(voiceProfObjectPtr, &iidStack);

     // Free MDMOID_VOICE_PROF_SIP object.
     cmsObj_free((void**)&voiceProfObjectPtr);
  }

  // Return CMS result.
  return cmsResult;
}
#else

/* DMP_VOICE_SERVICE_2 */ 
CmsRet SetSipDigitalMap(int portIndex, sipDigitalMapRec* recPtr)
{
  InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
  CallControlObject* voiceCallCtlObjectPtr;
  CmsRet cmsResult;

  // Get MDMOID_SIP_NETWORK object.
  cmsResult = cmsObj_getNext(MDMOID_CALL_CONTROL, &iidStack, (void**)&voiceCallCtlObjectPtr);

  // Test for valid MDMOID_VOICE_PROF object.
  if (cmsResult == CMSRET_SUCCESS)
  {
     // Setup digit map from OAM input array.
     CMSMEM_REPLACE_STRING_FLAGS(voiceCallCtlObjectPtr->X_BROADCOM_COM_DialPlan, (void*)&recPtr->sipDigitalMap, ALLOC_SHARED_MEM);
   
     // Write modified-object back to CMS.
     cmsResult = cmsObj_set(voiceCallCtlObjectPtr, &iidStack);
   
     // Free MDMOID_VOICE_PROF_SIP object.
     cmsObj_free((void**)&voiceCallCtlObjectPtr);
  }

  // Return CMS result.
  return cmsResult;
}
#endif /* DMP_VOICE_SERVICE_1 */

#endif // DMP_X_ITU_ORG_VOICE_SIP_1


#define VOICE_REBOOT_WAIT_SECONDS 10
static UBOOL8 isVoiceRunning(void)
{
   CmsMsgHeader msgHdr = EMPTY_MSG_HEADER;
   CmsRet ret;

   cmsLog_debug("%s", __FUNCTION__);

   msgHdr.dst = EID_SSK;
   msgHdr.type = CMS_MSG_IS_APP_RUNNING;
   msgHdr.flags_request = 1;
   msgHdr.wordData = EID_VOICE;
         
   ret = cmsMsg_sendAndGetReply(mdmLibCtx.msgHandle, &msgHdr);
   if( ret == CMSRET_SUCCESS )
   {
      return TRUE;
   }
   else if( ret == CMSRET_OBJECT_NOT_FOUND )
   {
      return FALSE;
   }
   else
   {
      cmsLog_error("could not send CMS_MSG_IS_APP_RUNNING msg to smd, ret=%d", ret);
      return FALSE;
   }
}

CmsRet SendRebootVoice(void)
{
   CmsRet cmsResult = CMSRET_SUCCESS;
   int i=0;
   CmsMsgHeader msgHdr = EMPTY_MSG_HEADER;

   msgHdr.src = mdmLibCtx.eid;
   msgHdr.dst = EID_SSK;
   msgHdr.flags_event = 1;
   
   if( isVoiceRunning() )
   {
      /* Stop voice */
      msgHdr.type = CMS_MSG_DEINIT_VOICE;
      cmsResult = cmsMsg_send(mdmLibCtx.msgHandle, &msgHdr);
      if (cmsResult != CMSRET_SUCCESS)
      {
         cmsLog_error("Send message failure, cmsResult: %d", cmsResult);
         cmsResult = CMSRET_INTERNAL_ERROR;
      }
   }
   
   while( isVoiceRunning() && i < VOICE_REBOOT_WAIT_SECONDS )
   {
      sleep(1000);
      i++;
   }      
   
   if( !isVoiceRunning() )
   {
      /* Start voice */
      msgHdr.type = CMS_MSG_INIT_VOICE;
      cmsResult = cmsMsg_send(mdmLibCtx.msgHandle, &msgHdr);
      if (cmsResult != CMSRET_SUCCESS)
      {
         cmsLog_error("Send message failure, cmsResult: %d", cmsResult);
         cmsResult = CMSRET_INTERNAL_ERROR;
      }
   }
   else
   {
      cmsLog_error("Could not stop voice -  failure, cmsResult: %d", cmsResult); 
      cmsResult = CMSRET_INTERNAL_ERROR;    
   }

   return cmsResult;
}


#endif // DMP_X_ITU_ORG_VOICE_SIP_1
#endif // DMP_X_ITU_ORG_GPON_1
