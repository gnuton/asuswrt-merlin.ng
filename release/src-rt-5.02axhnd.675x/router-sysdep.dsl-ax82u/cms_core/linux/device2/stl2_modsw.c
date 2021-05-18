/***********************************************************************
 *
 *  Copyright (c) 2006-2009  Broadcom Corporation
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

#ifdef DMP_DEVICE2_SM_BASELINE_1

#include "cms_core.h"
#include "cms_fil.h"
#include "cms_util.h"
#include "cms_msg_modsw.h"
#include "rcl.h"
#include "rut_util.h"
#include "qdm_modsw_ee.h"

/*!\file stl_modsw.c
 * \brief This file contains generic modular sw functions (not specific to
 *        any specific execution env.)
 *
 */

#ifdef SUPPORT_BEEP

static CmsRet getAvailableDiskSpaceByEe(_ExecEnvObject *obj, SINT32 *availableDiskSpace);
static CmsRet getMemoryInUseByEe(_ExecEnvObject *obj, SINT32 *memoryInUse);
static CmsRet getDiskSpaceInUseByEu(_EUObject *obj, SINT32 *diskSpaceInUse);
static CmsRet getMemoryInUseByEu(_EUObject *euObj, SINT32 *memoryInUse);

#ifdef DMP_DEVICE2_PROCESSSTATUS_1
static UINT32 getParentPid(UINT32 pid);
static CmsRet getAssociatedProcessList(_EUObject *obj);
#endif


static CmsRet getAvailableDiskSpaceByEe(_ExecEnvObject *obj, SINT32 *availableDiskSpace)
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 timeoutMs = 5000;
   char msgBuf[sizeof(CmsMsgHeader)] = {0};
   CmsMsgHeader *msg = (CmsMsgHeader *)msgBuf;
   char replyBuf[sizeof(CmsMsgHeader)+sizeof(EEavailableDiskSpaceReplyBody)] = {0};    
   CmsMsgHeader *reply = (CmsMsgHeader *)replyBuf;
   EEavailableDiskSpaceReplyBody *replyBody = (EEavailableDiskSpaceReplyBody *)(reply+1);
   
   *availableDiskSpace = -1;

   cmsLog_debug("Enter: eeName=%s eeMngrEid=%d", obj->name, obj->X_BROADCOM_COM_MngrEid);

   /* send message to EE manager to get available disk space */
   msg->type = CMS_MSG_GET_EE_AVAILABLE_DISK_SPACE;
   msg->src  = mdmLibCtx.eid;
   msg->dst  = obj->X_BROADCOM_COM_MngrEid;
   msg->flags_event = 1;
   msg->flags_bounceIfNotRunning = 1;

   ret = cmsMsg_sendAndGetReplyBufWithTimeout(mdmLibCtx.msgHandle, msg, &reply, timeoutMs);
   if (ret == CMSRET_SUCCESS)
   {
      if (replyBody->availableDiskSpace > 0)
      {
         *availableDiskSpace = replyBody->availableDiskSpace;
      }
   }
   else
   {
      /* pmd may not be up */
      cmsLog_debug("send msg 0x%x failed, ret=%d", msg->type, ret);
   }
   
   return CMSRET_SUCCESS;

}  /* End of getAvailableDiskSpaceByEe() */


static CmsRet getMemoryInUseByEe(_ExecEnvObject *obj, SINT32 *memoryInUse)
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmd[BUFLEN_128]={0}, buf[BUFLEN_256]={0};
   char name[BUFLEN_64], val1[BUFLEN_64], val2[BUFLEN_64], val3[BUFLEN_64];
   FILE *fp = NULL;

   *memoryInUse = -1;

   sprintf(cmd, "lxc-info -n BEEP_%s_%s --stats", obj->name, obj->version);

   if ((fp = popen(cmd, "r")) == NULL)
   {
      cmsLog_error("Error opening pipe! cmd=%s", cmd);
      return CMSRET_INTERNAL_ERROR;
   }

   while (fgets(buf, BUFLEN_256, fp) != NULL)
   {
      memset(name, 0, BUFLEN_64);
      memset(val1, 0, BUFLEN_64);
      memset(val2, 0, BUFLEN_64);
      memset(val3, 0, BUFLEN_64);

      sscanf(buf, "%s %s %s %s", name, val1, val2, val3);

      if ((val2[0] != '\0') && (cmsUtl_strcmp(name, "Memory") == 0))
      {
         double inUse;

         cmsLog_debug("buf=%s val1=%s val2=%s val3=%s", buf, val1, val2, val3);

         if ((inUse = strtod(val2, NULL)))
         {
            if (cmsUtl_strcmp(val3, "MiB") == 0)
            {
               inUse *= 1024;    /* convert to KiB */
            }
            *memoryInUse = inUse;
         }

         break;
      }
   }

   pclose(fp);
   return ret;

}  /* End of getMemoryInUseByEe() */


static CmsRet getDiskSpaceInUseByEu(_EUObject *obj, SINT32 *diskSpaceInUse)
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 timeoutMs = 5000;
   CmsEntityId  mngrEid;
   char euFullPath[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   char duName[BUFLEN_32+1]={0};
   char duVer[BUFLEN_16+1]={0};
   UINT32 duInstance = 0;
   char msgBuf[sizeof(CmsMsgHeader)+sizeof(EUdiskSpaceInUseMsgBody)] = {0};
   CmsMsgHeader *msg = (CmsMsgHeader *)msgBuf;
   EUdiskSpaceInUseMsgBody *msgBody = (EUdiskSpaceInUseMsgBody *)(msg+1);
   char replyBuf[sizeof(CmsMsgHeader)+sizeof(EUdiskSpaceInUseReplyBody)] = {0};
   DUObject *duObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;

   CmsMsgHeader *reply = (CmsMsgHeader *)replyBuf;
   EUdiskSpaceInUseReplyBody *replyBody=(EUdiskSpaceInUseReplyBody *)(reply+1);
   
   *diskSpaceInUse = -1;

   cmsLog_debug("Enter: euid=%s euName=%s", obj->EUID, obj->name);

   ret = qdmModsw_getMngrEidByExecEnvFullPathLocked(obj->executionEnvRef,
                                                    &mngrEid);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get mngrEid of %s from execEnvRef=%s",
                   obj->name, obj->executionEnvRef);
      return ret;
   }

   ret = qdmModsw_getExecUnitFullPathByEuidLocked(obj->EUID,
                                                  euFullPath,
                                                  sizeof(euFullPath)-1);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed get euFullPath by EUID=%s, ret=%d", obj->EUID, ret);
      return ret;
   }

   while (!found &&
          (ret = cmsObj_getNext(MDMOID_DU, &iidStack,
                                (void **) &duObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_isFullPathInCSL(euFullPath, duObj->executionUnitList) == TRUE)
      {
         found = TRUE;
         cmsUtl_strncpy(duName, duObj->name, sizeof(duName)-1);
         cmsUtl_strncpy(duVer, duObj->version, sizeof(duVer)-1);
         if (iidStack.currentDepth)
         {
            duInstance = iidStack.instance[iidStack.currentDepth-1];
         }
      }
      cmsObj_free((void **)&duObj);
   }

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed get duName/duVer/duInstance from euFullPath=%s",
                   euFullPath);
      return ret;
   }

   /* send message to mngrEid to get disk space in use */
   if (mngrEid == EID_PMD)
   {
      snprintf(msgBody->euDir, sizeof(msgBody->euDir), "/du/%s-%d/app_%s",
               duName, duInstance, obj->name);
   }
   else if (mngrEid == EID_BBCD)
   {
      snprintf(msgBody->euDir, sizeof(msgBody->euDir), "%s-%s/app_%s",
               duName, duVer, obj->name);
   }
   else
   {
      cmsLog_notice("send query diskspace usage to mngrEid<%d>", mngrEid);
   }

   msg->type = CMS_MSG_GET_EU_DISK_SPACE_IN_USE;
   msg->src  = mdmLibCtx.eid;
   msg->dst  = mngrEid;
   msg->flags_event = 1;
   msg->flags_bounceIfNotRunning = 1;
   msg->dataLength = sizeof(EUdiskSpaceInUseMsgBody);

   ret = cmsMsg_sendAndGetReplyBufWithTimeout(mdmLibCtx.msgHandle, msg,
                                              &reply, timeoutMs);
   if (ret == CMSRET_SUCCESS)
   {
      if (replyBody->diskSpaceInUse > 0)
      {
         *diskSpaceInUse = replyBody->diskSpaceInUse;
      }
   }
   else
   {
      /* pmd may not be up */
      cmsLog_debug("send msg 0x%x failed, ret=%d", msg->type, ret);
   }

   return CMSRET_SUCCESS;

}  /* End of getDiskSpaceInUseByEu() */


static CmsRet getMemoryInUseByEu(_EUObject *obj, SINT32 *memoryInUse)
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmd[BUFLEN_128]={0}, buf[BUFLEN_256]={0};
   char name[BUFLEN_64], val1[BUFLEN_64], val2[BUFLEN_64], val3[BUFLEN_64];
   FILE *fp = NULL;

   *memoryInUse = -1;

   sprintf(cmd, "lxc-info -n %s --stats", obj->EUID);

   if ((fp = popen(cmd, "r")) == NULL)
   {
      cmsLog_error("Error opening pipe! cmd=%s", cmd);
      return CMSRET_INTERNAL_ERROR;
   }

   while (fgets(buf, BUFLEN_256, fp) != NULL)
   {
      memset(name, 0, BUFLEN_64);
      memset(val1, 0, BUFLEN_64);
      memset(val2, 0, BUFLEN_64);
      memset(val3, 0, BUFLEN_64);

      sscanf(buf, "%s %s %s %s", name, val1, val2, val3);

      if ((val2[0] != '\0') && (cmsUtl_strcmp(name, "Memory") == 0))
      {
         double inUse;

         cmsLog_debug("buf=%s val1=%s val2=%s val3=%s", buf, val1, val2, val3);

         if ((inUse = strtod(val2, NULL)))
         {
            if (cmsUtl_strcmp(val3, "MiB") == 0)
            {
               inUse *= 1024;    /* convert to KiB */
            }
            *memoryInUse = inUse;
         }

         break;
      }
   }

   pclose(fp);
   return ret;

}  /* End of getMemoryInUseByEu() */


#ifdef DMP_DEVICE2_PROCESSSTATUS_1
static UINT32 getParentPid(UINT32 pid)
{
   UINT32   ppid = 0;
   char     *ptr;
   char     buf[81];
   FILE     *fp = NULL;
   
   /* open /proc/pid/status file */
   snprintf(buf, sizeof(buf), "/proc/%d/status", pid);
     
   if ((fp = fopen(buf, "r")) == NULL)
   {
      cmsLog_notice("fopen %s failed", buf);
      return ppid;
   }

   while (fgets(buf, sizeof(buf), fp))
   {
      /* strip eol character */
      ptr = strchr(buf, 0xa);
      if (ptr)
      {
         *ptr = '\0';
      }

      if ((ptr = strstr(buf, "PPid:")))
      {
         ptr += sizeof("PPid:");
         ppid = strtol(ptr, NULL, 10);
         break;
      }
   }

   fclose(fp);
   return ppid;

}  /* End of getParentPid() */


static CmsRet getAssociatedProcessList(_EUObject *obj)
{
   CmsRet ret = CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc;
   InstanceIdStack   iidStack, iidStackSave;
   Dev2ProcessStatusObject       *processStatusObj      = NULL;
   Dev2ProcessStatusEntryObject  *processStatusEntryObj = NULL;
   char   *buf, *fullStr, *ptr;
   UINT32 size;
   UINT32 pid = 0, ppid = 0;
   UBOOL8 found = FALSE;      

   /* get DeviceInfo.ProcessStatus object to update the
    * DeviceInfo.ProcessStatus.Process.{i}. table
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = cmsObj_get(MDMOID_DEV2_PROCESS_STATUS, &iidStack, 0,
                         (void **)&processStatusObj)) == CMSRET_SUCCESS)
   {
      cmsObj_free((void **)&processStatusObj);
   }
   else
   {
      cmsLog_error("cmsObj_get MDMOID_DEV2_PROCESS_STATUS failed, ret=%d", ret);
      return ret;
   }

   /* first find the application process, then its parents if any */
   iidStackSave = iidStack;
   while ((ret = cmsObj_getNext(MDMOID_DEV2_PROCESS_STATUS_ENTRY, &iidStack,
                        (void **)&processStatusEntryObj)) == CMSRET_SUCCESS)
   {
      if (!found)
      {
         if (processStatusEntryObj->command)
         {
            if ((ptr = strrchr(processStatusEntryObj->command, '/')))
               ptr++;
            else
               ptr = processStatusEntryObj->command;

            if (cmsUtl_strcmp(obj->name, ptr) == 0)
            {
               /* found the application process */
               found = TRUE;
               /* free the associatedProcessList before update */
               CMSMEM_FREE_BUF_AND_NULL_PTR(obj->associatedProcessList);
            }
         }

         if (!found)
         {
            cmsObj_free((void **)&processStatusEntryObj);
            continue;
         }
      }
      else
      {
         /* the application process had been found. Now we look
          * for its parent processes.
          */
         if (processStatusEntryObj->PID != ppid)
         {
            cmsObj_free((void **)&processStatusEntryObj);
            continue;
         }
      }

      pid = processStatusEntryObj->PID;
      cmsObj_free((void **)&processStatusEntryObj);

      memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
      pathDesc.oid      = MDMOID_DEV2_PROCESS_STATUS_ENTRY;
      pathDesc.iidStack = iidStack;
      fullStr = NULL;
      cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullStr);

      /* allocate buffer for the associatedProcessList */
      size = cmsUtl_strlen(obj->associatedProcessList) +
             cmsUtl_strlen(fullStr) +
             2;  /* plus comma and null terminator */

      if ((buf = cmsMem_alloc(size, ALLOC_ZEROIZE)) == NULL)
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullStr);
         cmsLog_error("cmsMem_alloc failed. size=%d", size);
         ret = CMSRET_RESOURCE_EXCEEDED;
         break;
      }
      
      if (obj->associatedProcessList == NULL)
         sprintf(buf, "%s", fullStr);
      else
         sprintf(buf, "%s,%s", fullStr, obj->associatedProcessList);

      CMSMEM_REPLACE_STRING_FLAGS(obj->associatedProcessList, buf, ALLOC_SHARED_MEM);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullStr);
      CMSMEM_FREE_BUF_AND_NULL_PTR(buf);

      /* Look for the parent processes */
      if ((ppid = getParentPid(pid)) > 1)
         iidStack = iidStackSave;
      else
         break;   /* done */
   }

   if (ret == CMSRET_NO_MORE_INSTANCES)
      ret = CMSRET_SUCCESS;
   return ret;

}  /* End of getAssociatedProcessList() */
#endif
#endif   /* SUPPORT_BEEP */


CmsRet stl_swModulesObject(_SwModulesObject *obj __attribute__((unused)),
                  const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dUObject(_DUObject *obj __attribute__((unused)),
                  const InstanceIdStack *iidStack __attribute__((unused)))
{
/*
 * In the "classical" architecture of CMS, when the STL handler function is
 * called, the function will do an ioctl or send a message to a daemon to
 * get the latest up-to-date info for this object.  However, in the case
 * of modular software, osgid and linmosd will always update the DUstatus
 * object with the latest info, so the data in this object is always
 * up-to-date.  So no need to do anything in this function.
 */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
}


#ifdef SUPPORT_BEEP
CmsRet stl_execEnvObject(_ExecEnvObject *obj,
                         const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   SINT32 availableDiskSpace = -1;
   SINT32 availableMemory    = -1;
   SINT32 memoryInUse        = -1;

   cmsLog_debug("Enter: obj->name=%s obj->status=%s", obj->name, obj->status);

   if (IS_EMPTY_STRING(obj->name))
   {
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   if (cmsUtl_strcmp(obj->status, MDMVS_UP) != 0)
   {
      /* if pmd is not up, we cannot get availableDiskSpace and availableMemory. */
      obj->availableDiskSpace = -1;
      obj->availableMemory    = -1;
      return CMSRET_SUCCESS;
   }

   if (obj->allocatedDiskSpace > 0)
   {
      getAvailableDiskSpaceByEe(obj, &availableDiskSpace);
   }

   if (obj->allocatedMemory > 0)
   {
      getMemoryInUseByEe(obj, &memoryInUse);
      if (memoryInUse > 0)
      {
         availableMemory = obj->allocatedMemory - memoryInUse;
      }
   }

   if (obj->availableDiskSpace == availableDiskSpace &&
       obj->availableMemory    == availableMemory)
   {
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   else
   {
      obj->availableDiskSpace = availableDiskSpace;
      obj->availableMemory    = availableMemory;      
   }

   return ret;

}  /* End of stl_execEnvObject() */


CmsRet stl_eUObject(_EUObject *obj,
                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   SINT32 diskSpaceInUse = -1;
   SINT32 memoryInUse    = -1;

   cmsLog_debug("Enter: obj->EUID=%s obj->name=%s obj->status=%s",
                obj->EUID, obj->name, obj->status);

   if (IS_EMPTY_STRING(obj->EUID) || IS_EMPTY_STRING(obj->name))
   {
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   getDiskSpaceInUseByEu(obj, &diskSpaceInUse);

   if (cmsUtl_strcmp(obj->status, MDMVS_ACTIVE) == 0)
   {
      getMemoryInUseByEu(obj, &memoryInUse);
   }

   if (obj->diskSpaceInUse == diskSpaceInUse &&
       obj->memoryInUse    == memoryInUse)
   {
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   else
   {
      obj->diskSpaceInUse = diskSpaceInUse;
      obj->memoryInUse    = memoryInUse;      
   }
   
#ifdef DMP_DEVICE2_PROCESSSTATUS_1
   if (cmsUtl_strcmp(obj->status, MDMVS_ACTIVE) != 0)
   {
      /* free the associatedProcessList */
      if (obj->associatedProcessList)
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(obj->associatedProcessList);
         ret = CMSRET_SUCCESS;
      }
   }
   else
   {
      CmsRet ret1;

      ret1 = getAssociatedProcessList(obj);
      if (ret1 != CMSRET_SUCCESS_OBJECT_UNCHANGED)
      {
         ret = ret1;
      }
   }   
#endif

   return ret;

}  /* End of stl_eUObject() */
#else
CmsRet stl_execEnvObject(_ExecEnvObject *obj __attribute__((unused)),
                         const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* see comments in stl_dUObject */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_eUObject(_EUObject *obj __attribute__((unused)),
                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* see comments in stl_dUObject */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
}
#endif

CmsRet stl_extensionsObject(_ExtensionsObject *obj __attribute__((unused)),
                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busObject(_BusObject *obj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busObjectPathObject(_BusObjectPathObject *obj __attribute__((unused)),
                               const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busInterfaceObject(_BusInterfaceObject *obj __attribute__((unused)),
                              const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busMethodObject(_BusMethodObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busSignalObject(_BusSignalObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busPropertyObject(_BusPropertyObject *obj __attribute__((unused)),
                             const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busClientObject(_BusClientObject *obj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_busClientPrivilegeObject(_BusClientPrivilegeObject *obj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_manifestObject(_ManifestObject *obj __attribute__((unused)),
                          const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dmAccessObject(_DmAccessObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


#endif /* DMP_DEVICE2_SM_BASELINE_1 */
