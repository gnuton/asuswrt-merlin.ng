/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2011:proprietary:standard

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

#include "cms.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "sysutil.h"
#include "mdm.h"
#include "mdm_private.h"

/*
 * This file contains helper functions for mdm.c
 * mdm.c is shipped to customers as a binary only object, which means
 * it cannot be recompiled.  But the customer may need to customize
 * our reference software, which may cause some constants to get redefined
 * or some structures to change in size.  If mdm.c uses these constants
 * or structures, than the mdm.o binary object will not work correctly
 * because it was compiled with the old values.  Hence, mdm.c will use
 * the functions in this file to get those changed definitions
 * so that mdm.c will work correctly even though it was not recompiled.
 * The functions in this file may look trivially simple, but they are needed
 * because this file can be recompiled to get the changed definitions, but
 * mdm.c cannot be recompiled.
 */


/*
 * Info shared by other layers in the cms_core library, but not by
 * any other applications.
 */
MdmLibraryContext mdmLibCtx={FALSE, 0, 0, CMS_INVALID_PID, NULL, UNINITIALIZED_SHM_ID, NULL, ALLOC_ZEROIZE, FALSE, TRUE};


/* An arry of EIDs' that have full write access and object create privileges */
CmsEntityId fullWriteAccessEidArray[] = {EID_SMD, EID_SSK,
                       EID_OSGID, EID_LINMOSD, EID_OPENWRTD, EID_DOCKERMD,
                       EID_UDPECHO, EID_OMCID, EID_OMCIPMD, EID_WLMNGR,
                       EID_VOICE, EID_DECT, EID_DECTDBGD, EID_BBCD,
                       EID_EPON_APP, EID_HOMEPLUGD, EID_HOMEPLUGCTL, EID_FIREWALLD,
                       EID_BMUD, EID_1905, EID_PMD, EID_WLSSK, EID_WLNVRAM, 
                       EID_WLDIAG, EID_INVALID};
                       
UBOOL8 mdm_isFullWriteAccessEid(CmsEntityId eid)
{
   UINT32 i=0;

   while (fullWriteAccessEidArray[i] != EID_INVALID)
   {
      if (eid == fullWriteAccessEidArray[i])
      {
         return TRUE;
      }
      i++;
   }

   return FALSE;
}

MdmPathDescriptor *mdm_allocatePathDescriptor(UINT32 flags)
{
   return ((MdmPathDescriptor *) cmsMem_alloc(sizeof(MdmPathDescriptor), flags));
}

UINT32 mdm_getMaxOid(void)
{
   return MDM_MAX_OID;
}

UINT32 mdm_getMaxParamNameLength(void)
{
   return MAX_MDM_PARAM_NAME_LENGTH;
}

void mdm_initPathDescriptor(MdmPathDescriptor *pathDesc)
{
   INIT_PATH_DESCRIPTOR(pathDesc);
}

void mdm_initParamName(char *paramName)
{
   INIT_MDM_PARAM_NAME(paramName);
}

UINT16 mdm_getDefaultAccessBitMask(void)
{
   return DEFAULT_ACCESS_BIT_MASK;
}


const char *cmsMdm_paramTypeToString(MdmParamTypes paramType)
{
   const char *str;

   switch(paramType)
   {
   case MPT_STRING:
      str = "string";
      break;

   case MPT_INTEGER:
      str = "int";
      break;

   case MPT_UNSIGNED_INTEGER:
      str = "unsignedInt";
      break;

   case MPT_BOOLEAN:
      str = "boolean";
      break;

   case MPT_DATE_TIME:
      str = "dateTime";
      break;

   case MPT_BASE64:
      str = "base64";
      break;

   case MPT_HEX_BINARY:
      str = "hexBinary";
      break;

   case MPT_UNSIGNED_LONG64:
      str = "unsignedLong";
      break;

   case MPT_LONG64:
      str = "long";
      break;

   case MPT_UUID:
      str = "UUID";
      break;

   case MPT_IP_ADDR:
      str = "IPAddress";
      break;

   case MPT_MAC_ADDR:
      str = "MACAddress";
      break;

   case MPT_STATS_COUNTER32:
      str = "StatsCounter32";
      break;

   case MPT_STATS_COUNTER64:
      str = "StatsCounter64";
      break;

   default:
      cmsLog_error("invalid paramType enum %d", paramType);
      str = "invalid";
      break;
   }

   return str;
}


CmsRet mdm_sendLowerLayersChangedMsg(MdmObjectId oid __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)),
                     const char *newLowerLayersStr __attribute__((unused)),
                     const char *currLowerLayersStr __attribute__((unused)))
{
#ifdef DMP_DEVICE2_BASELINE_1
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(IntfStackLowerLayersChangedMsgBody)] = {0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   IntfStackLowerLayersChangedMsgBody *llChangedMsg = (IntfStackLowerLayersChangedMsgBody *) (msgHdr + 1);
   CmsRet ret;

   /* fill in header */
   msgHdr->src = mdmLibCtx.eid;
   msgHdr->dst = EID_SSK;
   msgHdr->flags_event = 1;
   msgHdr->type = CMS_MSG_INTFSTACK_LOWERLAYERS_CHANGED;
   msgHdr->dataLength = sizeof(IntfStackLowerLayersChangedMsgBody);

   /* fill in msg body */
   llChangedMsg->oid = oid;
   llChangedMsg->iidStack = *iidStack;

   ret = cmsUtl_diffFullPathCSLs(newLowerLayersStr, currLowerLayersStr,
                                 llChangedMsg->deltaLowerLayers,
                                 sizeof(llChangedMsg->deltaLowerLayers));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsUtl_diffFullPathCSLs failed, newStr=%s currStr=%s ret=%d",
                   newLowerLayersStr, currLowerLayersStr, ret);
      return ret;
   }
   else if (llChangedMsg->deltaLowerLayers[0]=='\0')
   {
      cmsLog_debug("empty deltaLowerLayer!!");
      return ret;
   }

   cmsLog_debug("sending INTFSTACK_LOWERLAYERS_CHANGED");
   if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, msgHdr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to send INTFSTACK_LOWERLAYERS_CHANGED msg, ret=%d",
                   ret);
   }

   return ret;
#else
   return CMSRET_SUCCESS;
#endif
}


CmsRet mdm_sendObjectDeletedMsg(MdmObjectId oid __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DMP_DEVICE2_BASELINE_1
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(IntfStackObjectDeletedMsgBody)] = {0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   IntfStackObjectDeletedMsgBody *objDeletedMsg = (IntfStackObjectDeletedMsgBody *) (msgHdr + 1);
   CmsRet ret;

   /* fill in header */
   msgHdr->src = mdmLibCtx.eid;
   msgHdr->dst = EID_SSK;
   msgHdr->flags_event = 1;
   msgHdr->type = CMS_MSG_INTFSTACK_OBJECT_DELETED;
   msgHdr->dataLength = sizeof(IntfStackObjectDeletedMsgBody);

   /* fill in msg body */
   objDeletedMsg->oid = oid;
   objDeletedMsg->iidStack = *iidStack;

   cmsLog_debug("sending INTFSTACK_OBJECT_DELETED");
   if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, msgHdr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to send INTFSTACK_OBJ_DELETED msg, ret=%d",
                   ret);
   }

   return ret;
#else
   return CMSRET_SUCCESS;
#endif
}


CmsRet mdm_sendAliasChangedMsg(MdmObjectId oid __attribute__((unused)),
              const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DMP_DEVICE2_BASELINE_1
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(IntfStackAliasChangedMsgBody)] = {0};
   CmsMsgHeader *msgHdr = (CmsMsgHeader *) msgBuf;
   IntfStackAliasChangedMsgBody *aliasChangedMsg = (IntfStackAliasChangedMsgBody *) (msgHdr + 1);
   CmsRet ret;

   /* fill in header */
   msgHdr->src = mdmLibCtx.eid;
   msgHdr->dst = EID_SSK;
   msgHdr->flags_event = 1;
   msgHdr->type = CMS_MSG_INTFSTACK_ALIAS_CHANGED;
   msgHdr->dataLength = sizeof(IntfStackAliasChangedMsgBody);

   /* fill in msg body */
   aliasChangedMsg->oid = oid;
   aliasChangedMsg->iidStack = *iidStack;

   cmsLog_debug("sending INTFSTACK_ALIAS_CHANGED");
   if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, msgHdr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to send INTFSTACK_ALIAS_CHANGED msg, ret=%d",
                   ret);
   }

   return ret;
#else
   return CMSRET_SUCCESS;
#endif
}


UBOOL8 mdm_allowSequenceNumError()
{
    return FALSE;
}

CmsRet mdm_validateSequenceNum(MdmObjectId oid, const InstanceIdStack *iidStack,
                            UINT16 currSeqNum, UINT16 newSeqNum,
                            SINT32 lastWritePid, const CmsTimestamp *lastWriteTs)
{
    CmsTimestamp nowTs;
    UINT32 deltaMs;

    if (newSeqNum == currSeqNum)
    {
        return CMSRET_SUCCESS;
    }
    /*
     * Allow the same thread to write any sequence number. 
     * Needed by OMCI (and maybe other code) which has complicated read/write
     * sequences.  See Jira 33919. 
     */
    if (lastWritePid == sysUtl_gettid())
    {
        return CMSRET_SUCCESS;
    }
    /* MDM startup, allow first write to go through */
    if (lastWritePid == CMS_INVALID_PID)
    {
        return CMSRET_SUCCESS;
    }

    cmsTms_get(&nowTs);
    deltaMs = cmsTms_deltaInMilliSeconds(&nowTs, lastWriteTs);
    cmsLog_error("MDM object sequence num error by tid %d on %s (oid %d) "
                 "iidStack %s seq %d->%d (writer tid=%d %dms ago at %d.%03d)",
                 sysUtl_gettid(),
                 mdm_oidToGenericPath(oid), oid, cmsMdm_dumpIidStack(iidStack),
                 newSeqNum, currSeqNum,
                 lastWritePid, deltaMs,
                 lastWriteTs->sec % 1000, lastWriteTs->nsec / NSECS_IN_MSEC);

    /* if we allow sequenceNumError, then we return TRUE here even though
     * the sequence number check failed. */
    if (mdm_allowSequenceNumError())
    {
        return CMSRET_SUCCESS;
    }
    
    return CMSRET_OBJECT_SEQUENCE_ERROR;
}
