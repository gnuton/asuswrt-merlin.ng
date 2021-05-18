/***********************************************************************
 *
 *  Copyright (c) 2006-2007  Broadcom Corporation
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

#include <time.h>
#include "cms.h"
#include "cms_util.h"
#include "mdm.h"
#include "oal.h"
#include "cms_lck.h"
#include "cms_obj.h"
#include "prctl.h"
#include "sysutil.h"
#include "sysutil_proc.h"

/* filled in by oalLck_init, used for lock tracing. */
extern char oalLock_name[PROC_THREAD_NAME_LEN];

/* my PID namespace offset (shared with odl.c) */
SINT32 pidNsOffset=0;

#define SAME_PID_NS(x, y) ((x)/CMSLCK_PID_NS_MULTIPLIER == (y)/CMSLCK_PID_NS_MULTIPLIER)


/* forward static function declarations */
static CmsRet lck_acquireLock(const char* callerFuncName,
                              UBOOL8 useTimeout, UINT32 timeoutMilliSeconds);
static CmsRet lck_acquireZoneLocks(UBOOL8 *zones,
                            UBOOL8 useTimeout, UINT32 timeoutMilliSeconds,
                            const char* callerFuncName);
static void dumpLockTimeoutDiag(UINT8 zone, UINT32 actualTimeoutMs);
static void sleepRandom();
static UINT8 getMaxZone(const UBOOL8 *zones);
static void writeLockedInfo(CmsLockThreadInfo *thread, UINT8 zone,
                            MdmObjectId oid, const char *funcName);
static UBOOL8 writeUnlockInfo(CmsLockThreadInfo *thread, UINT8 zone,
                              const char *funcName);
static void condFreeLockThread(CmsLockThreadInfo *thread);
static CmsLockThreadInfo *getLockThread(SINT32 tid);
// allow odl.c to use this func.
CmsLockThreadInfo *lck_getExistingLockThread(SINT32 tid);
static CmsLockThreadInfo *getLockThreadExistingLocked(SINT32 tid);
static CmsLockThreadInfo *getLockThreadNewLocked(SINT32 tid);
static void clearDeadThreadsLocked();
static void clearDeadThreadByTidLocked(SINT32 tid);
static UBOOL8 checkAndClearDeadPthread(UINT8 zone);
static UINT32 getTotalZoneEntryCounts(const CmsLockThreadInfo *thread);
static UINT32 getTotalZoneLockCounts(const CmsLockThreadInfo *thread);
static UINT32 getTotalHasZones(const CmsLockThreadInfo *thread);



CmsRet cmsLck_acquireLockTraced(const char* callerFuncName)
{
   return (lck_acquireLock(callerFuncName, FALSE, 0));
}

CmsRet cmsLck_acquireLockWithTimeoutTraced(const char* callerFuncName,
                                           UINT32 timeoutMilliSeconds)
{
   return (lck_acquireLock(callerFuncName, TRUE, timeoutMilliSeconds));
}

CmsRet lck_acquireLock(const char* callerFuncName,
                       UBOOL8 useTimeout, UINT32 timeoutMilliSeconds)
{
   SINT32 tid;
   CmsLockThreadInfo *thread;

   tid = sysUtl_gettid() + pidNsOffset;
   thread = getLockThread(tid);
   if (thread == NULL)
   {
      return CMSRET_INTERNAL_ERROR;
   }

   // With MDM zone locking, multiple calls to acquireLock is allowed.
   if (thread->globalLockCount == MDM_MAX_ENTRY_COUNT)
   {
      cmsLog_error("thread global lock count is %d, infinite recursion?",
                   thread->globalLockCount);
      cmsLck_dumpInfo();
      mdmShmCtx->lockMeta->stats.internalErrors++;
      return CMSRET_RECURSION_ERROR;
   }
   thread->globalLockCount++;
   thread->useCallerTimeout = useTimeout;
   thread->timeoutMilliSeconds = timeoutMilliSeconds;
   if (thread->globalLockCount == 1)
   {
      // Record the function name of the first call to acquireLock.
      cmsUtl_strncpy(thread->funcName, callerFuncName, sizeof(thread->funcName));
   }

   if (mdmShmCtx->lockMeta->traceLevel > 0)
   {
      cmsTms_get(&(thread->activityTs));
      printf("%s[%d]:%d.%03d:acquire global lock (cnt=%d) func %s\n",
             oalLock_name, thread->tid,
             thread->activityTs.sec%1000, thread->activityTs.nsec/1000000,
             thread->globalLockCount, callerFuncName);
   }

   // No locking is done in this function.  The actual zone will be auto-locked
   // when it is accessed via the OBJ, PHL, or MGM layers.  Once zone lock is
   // acquired, it will be held until the globalLockCount goes back to 0.
   return CMSRET_SUCCESS;
}


void cmsLck_releaseLockTraced(const char* callerFuncName)
{
   UINT32 i;
   SINT32 tid;
   CmsLockThreadInfo *thread;

   tid = sysUtl_gettid() + pidNsOffset;
   thread = getLockThread(tid);
   if (thread == NULL)
   {
      return;
   }

   if (thread->globalLockCount == 0)
   {
      // App programming error.
      cmsLog_error("global lock count is already 0!! tid=%d callerFuncName %s",
                   tid, callerFuncName);
      condFreeLockThread(thread);
      return;
   }

   thread->globalLockCount--;
   if (mdmShmCtx->lockMeta->traceLevel > 0)
   {
      cmsTms_get(&(thread->activityTs));
      printf("%s[%d]:%d.%03d:release global lock (cnt=%d) func %s\n",
             oalLock_name, thread->tid,
             thread->activityTs.sec%1000, thread->activityTs.nsec/1000000,
             thread->globalLockCount, callerFuncName);
   }
   if (thread->globalLockCount > 0)
   {
      return;
   }

   // Global lock count is 0.  Should be able to release all locks now.
   // However, if zoneLockCounts, zoneEntryCounts, or trackedMdmObj count for a
   // zone is not 0, that means the caller is trying to release lock inside MDM 
   // or got locking calls tangled up.  Do not unlock that zone.  When all
   // counts for that zone eventually goes to 0, that zone will be unlocked.
   for (i=0; i < MDM_MAX_LOCK_ZONES; i++)
   {
      if (thread->hasZones[i])
      {
         if (thread->zoneLockCounts[i] > 0)
         {
            cmsLog_debug("zoneLockCount[%d] is still %d.  Don't release yet.",
                         i, thread->zoneLockCounts[i]);
            continue;
         }
         if (thread->zoneEntryCounts[i] > 0)
         {
            cmsLog_debug("zoneEntryCount[%d] is still %d. Don't release yet.",
                         i, thread->zoneEntryCounts[i]);
            continue;
         }
         if (thread->tracked[i].count > 0)
         {
            cmsLog_debug("trackedMdmObj[%d] is still %d. Don't release yet.",
                         i, thread->tracked[i].count);
            continue;
         }
         // OK, we can unlock this zone.
         if (writeUnlockInfo(thread, i, callerFuncName))
         {
            oal_unlock_zone(i);
         }
      }
   }

   condFreeLockThread(thread);
   return;
}


CmsRet cmsLck_acquireZoneLockTraced(UINT8 zone, const char* callerFuncName)
{
   UBOOL8 zones[MDM_MAX_LOCK_ZONES]={FALSE};

   if (zone >= MDM_MAX_LOCK_ZONES)
   {
      cmsLog_error("Invalid zone %d (max=%d)", zone, MDM_MAX_LOCK_ZONES);
      return CMSRET_INVALID_ARGUMENTS;
   }

   zones[zone] = TRUE;
   return (lck_acquireZoneLocks(zones, FALSE, 0, callerFuncName));
}

CmsRet cmsLck_acquireZoneLockWithTimeoutTraced(UINT8 zone,
                    UINT32 timeoutMilliSeconds, const char* callerFuncName)
{
   UBOOL8 zones[MDM_MAX_LOCK_ZONES]={FALSE};

   if (zone >= MDM_MAX_LOCK_ZONES)
   {
      cmsLog_error("Invalid zone %d (max=%d)", zone, MDM_MAX_LOCK_ZONES);
      return CMSRET_INVALID_ARGUMENTS;
   }

   zones[zone] = TRUE;
   return (lck_acquireZoneLocks(zones, TRUE, timeoutMilliSeconds, callerFuncName));
}

CmsRet cmsLck_acquireZoneLocksTraced(UBOOL8 *zones, const char* callerFuncName)
{
   return (lck_acquireZoneLocks(zones, FALSE, 0, callerFuncName));
}

CmsRet cmsLck_acquireZoneLocksWithTimeoutTraced(UBOOL8 *zones,
                       UINT32 timeoutMilliSeconds, const char* callerFuncName)
{
   return (lck_acquireZoneLocks(zones, TRUE, timeoutMilliSeconds, callerFuncName));
}


// TODO: unify with lck_autoLockAllZonesWithBackoff
CmsRet cmsLck_acquireAllZoneLocksWithBackoffTraced(MdmObjectId oid,
                                                   UINT32 timeoutMilliSeconds,
                                                   const char *callerFuncName)
{
   UBOOL8 allZones[MDM_MAX_LOCK_ZONES];
   SINT32 tid;
   UINT32 elapsedMilliSeconds=0;
   UINT32 tries=0;
   CmsTimestamp startTs, nowTs;
   CmsLockThreadInfo *thread;
   CmsRet ret=CMSRET_INTERNAL_ERROR;

   tid = sysUtl_gettid() + pidNsOffset;
   thread = getLockThread(tid);
   if (thread == NULL)
   {
      return CMSRET_INTERNAL_ERROR;
   }

   memset(allZones, TRUE, sizeof(allZones));
   if (timeoutMilliSeconds <= CMSLCK_MAX_HOLDTIME)
   {
      return (lck_acquireZoneLocks(allZones, TRUE, timeoutMilliSeconds, callerFuncName));
   }

   // Special handling for longer timeouts.  Try to acquire all locks with a
   // short timeout, and if not successful, release all locks, sleep for a
   // slightly randomized amount on the order of a few seconds, and try again.
   cmsTms_get(&startTs);
   while (elapsedMilliSeconds < timeoutMilliSeconds)
   {
      UINT32 customTimeoutMs;
      if (tries > 0)
      {
         sleepRandom();
      }
      // fail quickly at first, but try harder later on.
      customTimeoutMs = tries < 3 ? 1000 : 2500;
      cmsLog_debug("Trying to lock all zones: tries=%d timeout=%d oid=%d (%s)",
                   tries, customTimeoutMs, oid, callerFuncName);
      ret = lck_acquireZoneLocks(allZones, TRUE, customTimeoutMs, callerFuncName);
      if (ret == CMSRET_SUCCESS)
      {
         cmsLog_debug("Got all locks! (tries=%d)", tries);
         return ret;
      }
      tries++;
      cmsTms_get(&nowTs);
      elapsedMilliSeconds = cmsTms_deltaInMilliSeconds(&nowTs, &startTs);
   }

   // Could not get all the locks.
   return ret;
}


CmsRet lck_acquireZoneLocks(UBOOL8 *zones,
                            UBOOL8 useTimeout, UINT32 timeoutMilliSeconds,
                            const char* callerFuncName)
{
   UINT8 maxZone;
   UINT32 i, j, actualTimeoutMs;
   SINT32 tid;
   CmsLockThreadInfo *thread;
   UBOOL8 alreadyHasNeededLocks=TRUE;
   UBOOL8 undo[MDM_MAX_LOCK_ZONES]={FALSE};
   CmsRet ret=CMSRET_SUCCESS;

   if (zones == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   tid = sysUtl_gettid() + pidNsOffset;
   thread = getLockThread(tid);
   if (thread == NULL)
   {
      return CMSRET_INTERNAL_ERROR;
   }
   thread->useCallerTimeout = useTimeout;
   thread->timeoutMilliSeconds = timeoutMilliSeconds;

   // Check for out-of-control recursions.
   maxZone = getMaxZone(zones);
   for (i=0; i <= maxZone; i++)
   {
      if (thread->zoneLockCounts[i] == MDM_MAX_ENTRY_COUNT)
      {
         cmsLog_error("lock count in zone %d is %d, infinite recursion??",
                      i, thread->zoneLockCounts[i]);
         cmsLck_dumpInfo();
         mdmShmCtx->lockMeta->stats.internalErrors++;
         return CMSRET_RECURSION_ERROR;
      }
   }

   // see if this thread already has all the requested zone locks.
   for (i=0; i <= maxZone; i++)
   {
      if (zones[i])
      {
         thread->zoneLockCounts[i]++;
         if (thread->hasZones[i] == FALSE)
         {
            alreadyHasNeededLocks = FALSE;
         }
      }
   }

   if (alreadyHasNeededLocks)
   {
      cmsTms_get(&(thread->activityTs));
      return CMSRET_SUCCESS;
   }

   /* thread does not have all the requested zone locks, get them */
   for (i=0; i <= maxZone; i++)
   {
      if (zones[i] == FALSE)
      {
         continue;
      }
      if (thread->hasZones[i])
      {
         continue;
      }
      if (thread->zoneLockCounts[i] != 1)
      {
         cmsLog_error("zone %d lockCount %d but does not have lock!?!",
                      i, thread->zoneLockCounts[i]);
         cmsLck_dumpInfo();
         mdmShmCtx->lockMeta->stats.internalErrors++;
         continue;
      }

      thread->wantZone = i;
      actualTimeoutMs = thread->useCallerTimeout ?
                        thread->timeoutMilliSeconds : CMSLCK_MAX_HOLDTIME;
      ret = oal_lock_zone(i, TRUE, actualTimeoutMs);
      if (ret != CMSRET_SUCCESS)
      {
         // if this lock owner is dead, reset lock and try again.
         if (checkAndClearDeadPthread(i))
         {
            ret = oal_lock_zone(i, TRUE, actualTimeoutMs);
         }
      }

      if (ret == CMSRET_SUCCESS)
      {
         writeLockedInfo(thread, i, 0, callerFuncName);
         undo[i] = TRUE;
      }
      else
      {
         // Failed to get zone lock i.
         dumpLockTimeoutDiag(i, actualTimeoutMs);

         // Undo everything this function did.
         for (j=0; j <= maxZone; j++)
         {
            if (zones[j])
            {
               thread->zoneLockCounts[j]--;
               if (undo[j])
               {
                  if (writeUnlockInfo(thread, j, callerFuncName))
                  {
                     oal_unlock_zone(j);
                  }
               }
            }
         }
         thread->wantZone = MDM_INVALID_LOCK_ZONE;
         return ret;
      }
   } /* for loop through all requested zones */

   return ret;
}


void cmsLck_releaseZoneLockTraced(UINT8 zone, const char* callerFuncName)
{
   UBOOL8 zones[MDM_MAX_LOCK_ZONES]={FALSE};
   if (zone >= MDM_MAX_LOCK_ZONES)
   {
      cmsLog_error("Invalid zone %d", zone);
      return;
   }

   zones[zone] = TRUE;
   cmsLck_releaseZoneLocksTraced(zones, callerFuncName);
   return;
}


void cmsLck_releaseAllZoneLocksTraced(const char *callerFuncName)
{
   UBOOL8 allZones[MDM_MAX_LOCK_ZONES];

   memset(allZones, TRUE, sizeof(allZones));
   cmsLck_releaseZoneLocksTraced(allZones, callerFuncName);
   return;
}


void cmsLck_releaseZoneLocksTraced(UBOOL8 *zones, const char* callerFuncName)
{
   UINT8 maxZone;
   UINT32 i;
   SINT32 tid;
   CmsLockThreadInfo *thread;

   tid = sysUtl_gettid() + pidNsOffset;
   thread = getLockThread(tid);
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread tid=%d", tid);
      return;
   }

   maxZone = getMaxZone(zones);
   for (i=0; i <= maxZone; i++)
   {
      if (zones[i] == FALSE)
      {
         continue;
      }

      if (thread->zoneLockCounts[i] == 0)
      {
         // App programming error.
         cmsLog_error("zoneLockCount for zone %d is already 0!", i);
         continue;
      }

      thread->zoneLockCounts[i]--;
      if (thread->zoneLockCounts[i] > 0)
      {
         continue;
      }
      if (thread->zoneEntryCounts[i] != 0)
      {
         // Releasing a zone lock while inside MDM.  It could happen.
         continue;
      }
      if (thread->tracked[i].count > 0)
      {
         continue;
      }
      if (thread->globalLockCount > 0)
      {
         // wait for global lock to go to 0 before releasing this lock.
         // This behavior is questionable.  If an app explicitly locked the
         // zone, then explicitly unlocks the zone, maybe we should just do it
         // regardless of the state of the globalLock.  But current behavior
         // is more simple and consistent.
         continue;
      }

      // OK, we can unlock this zone.
      if (writeUnlockInfo(thread, i, callerFuncName))
      {
         oal_unlock_zone(i);
      }
   }

   condFreeLockThread(thread);
   return;
}


UINT8 cmsLck_getLockZone(MdmObjectId oid)
{
   MdmObjectNode *objNode;

   if ((objNode = mdm_getObjectNode(oid)) == NULL)
   {
      cmsLog_error("Invalid oid %d", oid);
      return MDM_INVALID_LOCK_ZONE;
   }

   return objNode->lockZone;
}

UBOOL8 cmsLck_isTopLevelLockZone(UINT8 zone)
{
   return (zone == 0 || zone == 1);
}

void cmsLck_setHoldTimeWarnThresh(UINT32 thresh)
{
   SINT32 tid;
   CmsLockThreadInfo *thread;

   tid = sysUtl_gettid() + pidNsOffset;
   thread = getLockThread(tid);
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread tid=%d", tid);
      return;
   }

   if (thresh > thread->holdTimeWarnThresh)
   {
      thread->holdTimeWarnThresh = thresh;
      cmsLog_debug("setting thresh to %dms", thread->holdTimeWarnThresh);
   }
   else
   {
      cmsLog_debug("ignore thresh of %dms, curr thresh %dms",
                   thresh, thread->holdTimeWarnThresh);
   }

   return;
}

CmsRet cmsLck_setPidNsOffset(SINT32 offset)
{
   if (offset % CMSLCK_PID_NS_MULTIPLIER != 0)
   {
      cmsLog_error("offset must be a multiple of %d", CMSLCK_PID_NS_MULTIPLIER);
      return CMSRET_INVALID_ARGUMENTS;
   }
   if (pidNsOffset != 0)
   {
      cmsLog_error("Cannot set offset more than once (offset=%d)", pidNsOffset);
      return CMSRET_INVALID_ARGUMENTS;
   }

   pidNsOffset = offset;
   cmsLog_debug("Set pid namespace offset to %d", pidNsOffset);
   return CMSRET_SUCCESS;
}

void cmsLck_toggleTracing()
{
   mdmShmCtx->lockMeta->traceLevel = (mdmShmCtx->lockMeta->traceLevel == 0) ? 1 : 0;
   printf("Toggled MDM lock tracing, new value=%d\n", mdmShmCtx->lockMeta->traceLevel);
   return;
}

void cmsLck_dumpInfo()
{
   CmsLockOwnerInfo owners[MDM_MAX_LOCK_ZONES];
   CmsLockThreadInfo threads[MDM_MAX_LOCK_THREADS];
   CmsLockStats stats;
   UINT32 i, active=0;

   printf("===== Dumping all MDM lock info =====\n");
   memset(owners, 0, sizeof(owners));
   cmsLck_getLockOwnerInfo(owners);
   cmsLck_dumpLockOwnerInfo(owners);

   printf("===== Thread slots =====\n");
   memset(threads, 0, sizeof(threads));
   cmsLck_getLockThreadInfo(threads);
   for (i=0; i < MDM_MAX_LOCK_THREADS; i++)
   {
      if (threads[i].tid != CMS_INVALID_PID)
      {
         active++;
         printf("Slot %d:\n", i);
         cmsLck_dumpLockThreadInfo(&(threads[i]));
         printf("==========\n");
      }
   }
   printf("Thread slots in use %d (max=%d)\n", active, MDM_MAX_LOCK_THREADS);

   printf("===== Stats =====\n");
   memset(&stats, 0, sizeof(stats));
   cmsLck_getLockStats(&stats);
   cmsLck_dumpLockStats(&stats);
   printf("====================================\n\n");
   return;
}

void cmsLck_getLockOwnerInfo(CmsLockOwnerInfo *owners)
{
   oal_lock_meta();

   /* owners must be an array of MDM_MAX_LOCK_ZONES CmsLockOwnerInfo's */
   memcpy(owners, mdmShmCtx->lockMeta->owners, MDM_MAX_LOCK_ZONES * sizeof(CmsLockOwnerInfo));

   oal_unlock_meta();
   return;
}

void cmsLck_getLockThreadInfo(CmsLockThreadInfo *threads)
{
   oal_lock_meta();

   /* threads must be an array of MDM_MAX_LOCK_THREADS CmsLockThreadInfo's */
   memcpy(threads, mdmShmCtx->lockMeta->threads, MDM_MAX_LOCK_THREADS * sizeof(CmsLockThreadInfo));

   oal_unlock_meta();
   return;
}

void cmsLck_getLockStats(CmsLockStats *stats)
{
   oal_lock_meta();

   memcpy(stats, &(mdmShmCtx->lockMeta->stats), sizeof(CmsLockStats));

   oal_unlock_meta();
   return;
}

void cmsLck_dumpLockOwnerInfo(const CmsLockOwnerInfo *owners)
{
   CmsTimestamp ts;
   UINT32 heldMs, i;

   for (i=0; i < MDM_MAX_LOCK_ZONES; i++)
   {
      const CmsLockOwnerInfo *owner = &(owners[i]);
      if (owner->tid == CMS_INVALID_PID)
      {
         printf("Zone %d: unlocked\n", i);
      }
      else
      {
         cmsTms_get(&ts);
         heldMs = cmsTms_deltaInMilliSeconds(&ts, &owner->acquiredTs);
         printf("Zone %d: owner pid/tid=%d held %d ms (since %d.%03d)\n",
                 i, owner->tid, heldMs,
                 owner->acquiredTs.sec, owner->acquiredTs.nsec/1000000);
         printf("    firstOid=%d func=%s currOid=%d currFuncCode=%c\n",
                owner->firstOid, owner->funcName, owner->currOid, owner->currFuncCode);
      }
   }
}

void cmsLck_dumpLockThreadInfo(const CmsLockThreadInfo *thread)
{
   UINT32 len, i, anyCount=0;
   char zoneBuf[MDM_MAX_LOCK_ZONES*30]={0};
   UINT32 maxLen=sizeof(zoneBuf);
   CmsTimestamp nowTs;
   UINT32 deltaMs;

   cmsTms_get(&nowTs);
   deltaMs = cmsTms_deltaInMilliSeconds(&nowTs, &(thread->activityTs));

   if (SAME_PID_NS(thread->tid, pidNsOffset))
   {
      ProcThreadInfo tInfo;
      sysUtl_getThreadInfoFromProc(thread->tid, &tInfo);
      printf("%s ", tInfo.name);
   }
   printf("pid/tid=%d (last activity %d ms ago at %d.%03d)\n",
           thread->tid, deltaMs,
           thread->activityTs.sec, thread->activityTs.nsec/1000000);
   printf("global lock count %d\n", thread->globalLockCount);

   for (len=0, i=0; i < MDM_MAX_LOCK_ZONES; i++)
   {
      anyCount += thread->zoneLockCounts[i];
      len += snprintf(&zoneBuf[len], maxLen-len, "[%d]%d ",
                      i, thread->zoneLockCounts[i]);
   }
   printf("zone lock counts: %s\n", zoneBuf);
   
   for (len=0, i=0; i < MDM_MAX_LOCK_ZONES; i++)
   {
      anyCount += thread->zoneEntryCounts[i];
      len += snprintf(&zoneBuf[len], maxLen-len, "[%d]%d ",
                      i, thread->zoneEntryCounts[i]);
   }
   printf("zone entry counts: %s\n", zoneBuf);
   
   for (len=0, i=0; i < MDM_MAX_LOCK_ZONES; i++)
   {
      anyCount += thread->hasZones[i] ? 1 : 0;
      len += snprintf(&zoneBuf[len], maxLen-len, "[%d]%s ",
                      i, thread->hasZones[i] ? "LOCKED" : "no");
   }
   printf("has zones: %s\n", zoneBuf);
   if (thread->wantZone != MDM_INVALID_LOCK_ZONE)
   {
      printf("want zone: %d\n", thread->wantZone);
   }
   else
   {
      printf("want zone: none\n");
   }
   printf("totalTrackedMdmObjs: %d\n", thread->totalTrackedMdmObjs);
   printf("funcName: %s\n", thread->funcName);
   return;
}

void cmsLck_dumpLockStats(const CmsLockStats *stats)
{
   printf("Successful locks: %d\n", stats->successes);
   printf("Failed locks:     %d\n", stats->failures);
   printf("Soft failures:    %d\n", stats->softFailures);
   printf("Dead threads detected: %d\n", stats->deadThreads);
   printf("Lock UNDO detected:    %d\n", stats->undos);
   printf("Lock resets:           %d\n", stats->resets);
   printf("No thread slots:       %d\n", stats->threadSlotsFull);
   printf("Tracked MdmObj errors: %d\n", stats->trackedMdmObjErrors);
   printf("Internal errors:       %d\n", stats->internalErrors);
}

/** This is called when we fail to get a lock.  Will grab meta-lock and
 *  print out diagnostics. */
void dumpLockTimeoutDiag(UINT8 zone, UINT32 actualTimeoutMs)
{
   UINT32 holdTime;
   CmsTimestamp nowTs;
   CmsLockOwnerInfo *owner;
   ProcThreadInfo tInfo;

   oal_lock_meta();

   owner = &(mdmShmCtx->lockMeta->owners[zone]);
   cmsTms_get(&nowTs);
   holdTime = cmsTms_deltaInMilliSeconds(&nowTs, &(owner->acquiredTs));
   sysUtl_getThreadInfoFromProc(owner->tid, &tInfo);

   if (actualTimeoutMs < CMSLCK_MAX_HOLDTIME)
   {
      // Sometimes callers will "probe" the lock with a short timeout to see if
      // it is available.  The locking code also does this during
      // "backoff-retry".  Classify these failures (with short timeout) as soft
      // failures.
      mdmShmCtx->lockMeta->stats.softFailures++;
      cmsLog_debug("timed out after %d ms waiting for zone lock %d held by "
                   "%s[%d] for %d ms (since %d.%03d) first_oid %d func %s",
                   actualTimeoutMs, zone,
                   tInfo.name, owner->tid, holdTime,
                   owner->acquiredTs.sec, owner->acquiredTs.nsec/1000000,
                   owner->firstOid, owner->funcName);
   }
   else
   {
      mdmShmCtx->lockMeta->stats.failures++;
      cmsLog_error("timed out after %d ms waiting for zone lock %d held by "
                   "%s[%d] for %d ms (since %d.%03d) "
                   "first_oid %d func %s curr_oid %d func %c",
                   actualTimeoutMs, zone,
                   tInfo.name, owner->tid, holdTime,
                   owner->acquiredTs.sec, owner->acquiredTs.nsec/1000000,
                   owner->firstOid, owner->funcName,
                   owner->currOid, owner->currFuncCode);
   }

   oal_unlock_meta();
   return;
}


/** Sleep for a slightly randomized amount of time, between 1-4 seconds. */
static void sleepRandom()
{
   struct timespec sleepSpec;
   CmsTimestamp nowTs;

   cmsTms_get(&nowTs);
   sleepSpec.tv_sec = 1 + (nowTs.nsec / 1000 % 3);
   sleepSpec.tv_nsec = nowTs.nsec;
   nanosleep(&sleepSpec, NULL);
}

/** Return the max zone index in zones which is TRUE.  This helps to 
 *  shorten for loops in other functions. */
UINT8 getMaxZone(const UBOOL8 *zones)
{
   UINT8 i;
   UINT8 maxZone=0;

   for (i=0; i < MDM_MAX_LOCK_ZONES; i++)
   {
      if (zones[i])
      {
         maxZone = i;
      }
   }
   return maxZone;
}

CmsRet lck_autoLockZone(MdmObjectId oid, const char *where)
{
   UBOOL8 zones[MDM_MAX_LOCK_ZONES]={FALSE};
   UINT8 zone;

   if ((zone = cmsLck_getLockZone(oid)) == MDM_INVALID_LOCK_ZONE)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   zones[zone] = TRUE;
   return (lck_autoLockZones(zones, oid, where));
}

CmsRet lck_autoLockAllZonesWithBackoff(MdmObjectId oid,
                                       UINT32 timeoutMilliSeconds,
                                       const char *where)
{
   UBOOL8 allZones[MDM_MAX_LOCK_ZONES];
   SINT32 tid;
   UINT32 elapsedMilliSeconds=0;
   UINT32 tries=0;
   CmsTimestamp startTs, nowTs;
   CmsLockThreadInfo *thread;
   CmsRet ret=CMSRET_INTERNAL_ERROR;

   tid = sysUtl_gettid() + pidNsOffset;
   thread = getLockThread(tid);
   if (thread == NULL)
   {
      return CMSRET_INTERNAL_ERROR;
   }

   memset(allZones, TRUE, sizeof(allZones));
   thread->useCallerTimeout = TRUE;
   if (timeoutMilliSeconds <= CMSLCK_MAX_HOLDTIME)
   {
      thread->timeoutMilliSeconds = timeoutMilliSeconds;
      return (lck_autoLockZones(allZones, oid, where));
   }

   // Special handling for longer timeouts.  Try to acquire all locks with a
   // short timeout, and if not successful, release all locks, sleep for a
   // slightly randomized amount on the order of a few seconds, and try again.
   cmsTms_get(&startTs);
   while (elapsedMilliSeconds < timeoutMilliSeconds)
   {
      if (tries > 0)
      {
         sleepRandom();
      }
      // fail quickly at first, but try harder later on.
      thread->timeoutMilliSeconds = tries < 3 ? 1000 : 2500;
      cmsLog_debug("Trying to lock all zones: tries=%d timeout=%d",
                   tries, thread->timeoutMilliSeconds);
      if ((ret = lck_autoLockZones(allZones, oid, where)) == CMSRET_SUCCESS)
      {
         cmsLog_debug("Got all locks! (tries=%d)", tries);
         return ret;
      }
      tries++;
      cmsTms_get(&nowTs);
      elapsedMilliSeconds = cmsTms_deltaInMilliSeconds(&nowTs, &startTs);
   }

   // Could not get all the locks.
   return ret;
}

CmsRet lck_autoLockZones(const UBOOL8 *zones, MdmObjectId oid, const char *where)
{
   UINT8 maxZone;
   UINT32 i, j, actualTimeoutMs;
   SINT32 tid;
   CmsLockThreadInfo *thread;
   UBOOL8 alreadyHasNeededLocks=TRUE;
   UBOOL8 undo[MDM_MAX_LOCK_ZONES]={FALSE};
   CmsRet ret=CMSRET_SUCCESS;

   tid = sysUtl_gettid() + pidNsOffset;
   thread = getLockThread(tid);
   if (thread == NULL)
   {
      return CMSRET_INTERNAL_ERROR;
   }

   // Check for out-of-control recursions.
   maxZone = getMaxZone(zones);
   for (i=0; i <= maxZone; i++)
   {
      if (thread->zoneEntryCounts[i] == MDM_MAX_ENTRY_COUNT)
      {
         cmsLog_error("entry count in zone %d is %d, infinite recursion??",
                      i, thread->zoneEntryCounts[i]);
         cmsLck_dumpInfo();
         mdmShmCtx->lockMeta->stats.internalErrors++;
         return CMSRET_RECURSION_ERROR;
      }
   }

   // see if this thread already has all the requested zone locks.
   for (i=0; i <= maxZone; i++)
   {
      if (zones[i])
      {
         thread->zoneEntryCounts[i]++;
         if (thread->hasZones[i] == FALSE)
         {
            alreadyHasNeededLocks = FALSE;
         }
      }
   }

   if (alreadyHasNeededLocks)
   {
      cmsTms_get(&(thread->activityTs));
      return CMSRET_SUCCESS;
   }

   /* thread does not have all the requested zone locks, get them */
   for (i=0; i <= maxZone; i++)
   {
      if (zones[i] == FALSE)
      {
         continue;
      }
      if (thread->hasZones[i])
      {
         continue;
      }
      if (thread->zoneEntryCounts[i] != 1)
      {
         cmsLog_error("zone %d entryCount %d but does not have lock!?!",
                      i, thread->zoneEntryCounts[i]);
         cmsLck_dumpInfo();
         mdmShmCtx->lockMeta->stats.internalErrors++;
         continue;
      }

      thread->wantZone = i;
      actualTimeoutMs = thread->useCallerTimeout ?
                        thread->timeoutMilliSeconds : CMSLCK_MAX_HOLDTIME;
      ret = oal_lock_zone(i, TRUE, actualTimeoutMs);
      if (ret != CMSRET_SUCCESS)
      {
         // if this lock owner is dead, reset lock and try again.
         if (checkAndClearDeadPthread(i))
         {
            ret = oal_lock_zone(i, TRUE, actualTimeoutMs);
         }
      }

      if (ret == CMSRET_SUCCESS)
      {
         writeLockedInfo(thread, i, oid, where);
         undo[i] = TRUE;
      }
      else
      {
         // Failed to get zone lock i.
         dumpLockTimeoutDiag(i, actualTimeoutMs);

         // Undo everything this function did.
         for (j=0; j <= maxZone; j++)
         {
            if (zones[j])
            {
               thread->zoneEntryCounts[j]--;
               if (undo[j])
               {
                  if (writeUnlockInfo(thread, j, where))
                  {
                     oal_unlock_zone(j);
                  }
               }
            }
         }
         thread->wantZone = MDM_INVALID_LOCK_ZONE;
         return ret;
      }
   } /* for loop through all requested zones */

   return ret;
}


void lck_autoUnlockZone(MdmObjectId oid, const char *where)
{
   UBOOL8 zones[MDM_MAX_LOCK_ZONES]={FALSE};
   UINT8 zone;

   if ((zone = cmsLck_getLockZone(oid)) == MDM_INVALID_LOCK_ZONE)
   {
      cmsLog_error("Could not get zone for oid %d", oid);
      return;
   }

   zones[zone] = TRUE;
   lck_autoUnlockZones(zones, oid, where);
   return;
}

void lck_autoUnlockAllZones(const char *where)
{
   UBOOL8 allZones[MDM_MAX_LOCK_ZONES];

   memset(allZones, TRUE, sizeof(allZones));
   lck_autoUnlockZones(allZones, 0, where);
   return;
}

void lck_autoUnlockZones(const UBOOL8 *zones, MdmObjectId oid, const char *where)
{
   UINT8 maxZone;
   UINT32 i;
   SINT32 tid;
   CmsLockThreadInfo *thread;

   tid = sysUtl_gettid() + pidNsOffset;
   thread = getLockThread(tid);
   if (thread == NULL)
   {
      cmsLog_error("Could not find lock thread tid=%d", tid);
      return;
   }

   maxZone = getMaxZone(zones);
   for (i=0; i <= maxZone; i++)
   {
      if (zones[i] == FALSE)
      {
         continue;
      }

      if (thread->zoneEntryCounts[i] == 0)
      {
         cmsLog_error("entryCount is already 0! zone=%d oid=%d where=%s",
                      i, oid, where);
         cmsLck_dumpInfo();
         mdmShmCtx->lockMeta->stats.internalErrors++;
         continue;
      }

      thread->zoneEntryCounts[i]--;
      if (thread->zoneEntryCounts[i] == 0)
      {
         // only auto-unlock this zone if the explicit global lock and
         // explicit zone lock counts are 0 and there are no trackedMdmObjs.
         if ((thread->globalLockCount == 0) &&
             (thread->zoneLockCounts[i] == 0) &&
             (thread->tracked[i].count == 0))
         {
            // OK, we can unlock this zone.
            if (writeUnlockInfo(thread, i, where))
            {
               oal_unlock_zone(i);
            }
         }
      }
   }

   condFreeLockThread(thread);
   return;
}


void lck_autoTrackMdmObj(const void *mdmObj)
{
   MdmObjectId oid;
   UINT8 zone;
   SINT32 tid;
   CmsLockThreadInfo *thread;
   TrackedMdmObjs *trackedZone;

   if (mdmObj == NULL)
   {
      return;
   }

   tid = sysUtl_gettid() + pidNsOffset;
   if ((thread = getLockThread(tid)) == NULL)
   {
      cmsLog_error("Could not find lock thread tid=%d", tid);
      return;
   }

   // If app acquired global or zone lock, then no need to track MdmObjs.
   if (thread->globalLockCount > 0)
   {
      return;
   }

   oid = GET_MDM_OBJECT_ID(mdmObj);
   zone = cmsLck_getLockZone(oid);
   if (zone == MDM_INVALID_LOCK_ZONE)
   {
      return;
   }

   if (thread->zoneLockCounts[zone] > 0)
   {
      return;
   }

   // Track this MdmObj.
   trackedZone = &(thread->tracked[zone]);
   if (trackedZone->single == NULL)
   {
      trackedZone->single = mdmObj;
   }
   else
   {
      UINT32 i;
      if (trackedZone->array == NULL)
      {
         trackedZone->array = cmsMem_alloc(
                                     MAX_TRACKED_MDMOBJS * sizeof(void *),
                                     mdmLibCtx.allocFlags);
         if (trackedZone->array == NULL)
         {
            // Highly unlikely malloc will fail.
            cmsLog_error("Could not allocate tracking array!");
            mdmShmCtx->lockMeta->stats.trackedMdmObjErrors++;
            return;
         }
      }
      // find empty slot
      for (i=0; i < MAX_TRACKED_MDMOBJS; i++)
      {
         if (trackedZone->array[i] == NULL)
         {
            trackedZone->array[i] = mdmObj;
            break;
         }
      }
      if (i == MAX_TRACKED_MDMOBJS)
      {
         cmsLog_error("Could not find empy slot for mdmObj (slots=%d)",
                      MAX_TRACKED_MDMOBJS);
         mdmShmCtx->lockMeta->stats.trackedMdmObjErrors++;
         return;
      }
   }

   thread->totalTrackedMdmObjs++;
   trackedZone->count += 1;
   trackedZone->max = (trackedZone->count > trackedZone->max) ?
                        trackedZone->count : trackedZone->max;
   return;
}

void lck_autoUntrackMdmObj(const void *mdmObj)
{
   MdmObjectId oid;
   UINT8 zone;
   SINT32 tid;
   CmsLockThreadInfo *thread;
   TrackedMdmObjs *trackedZone;
   UBOOL8 found=FALSE;

   if (mdmObj == NULL)
   {
      return;
   }

   // This function is always called even if full auto-lock is not active.
   // So if there is no existing lock thread for this tid (meaning there is no
   // outstanding locking activity for this thread), don't create a new one.
   // And if totalTrackedMdmObjs == 0, obviously there is nothing to untrack.
   tid = sysUtl_gettid() + pidNsOffset;
   if ((thread = lck_getExistingLockThread(tid)) == NULL)
   {
      return;
   }
   if (thread->totalTrackedMdmObjs == 0)
   {
      return;
   }

   oid = GET_MDM_OBJECT_ID(mdmObj);
   zone = cmsLck_getLockZone(oid);
   if (zone == MDM_INVALID_LOCK_ZONE)
   {
      // Is mdmObj corrupted?  We should always be able to find the lock zone
      // this for oid.
      cmsLog_error("Could not get lock zone for oid %d (mdmObj=%p)",
                   oid, mdmObj);
      mdmShmCtx->lockMeta->stats.internalErrors++;
      return;
   }

   trackedZone = &(thread->tracked[zone]);
   if (trackedZone->count == 0)
   {
      // This function is always called even if full auto-lock is not active.
      // It is possible we are not tracking any objects in this zone.  So this
      // is not an error.
      return;
   }

   if (trackedZone->single == mdmObj)
   {
      trackedZone->single = NULL;
      found = TRUE;
   }
   else if (trackedZone->array != NULL)
   {
      UINT32 i;
      for (i=0; i < MAX_TRACKED_MDMOBJS; i++)
      {
         if (trackedZone->array[i] == mdmObj)
         {
            trackedZone->array[i] = NULL;
            found = TRUE;
            break;
         }
      }
   }
   
   if (!found)
   {
      // Even this is possible!  Thread initially in full auto-lock mode and
      // tracks some objects, then grabs the global lock and does some more
      // cmsObj_get's.  Those done inside the global lock are not tracked, but
      // when any mdmObj is freed, this function is called.
      return;
   }

   thread->totalTrackedMdmObjs--;
   trackedZone->count--;
   if (trackedZone->count == 0)
   {
      // If all other lock types are clear, we can unlock this zone.
      if ((thread->globalLockCount == 0) &&
          (thread->zoneLockCounts[zone] == 0) &&
          (thread->zoneEntryCounts[zone] == 0))
      {
         // OK, we can unlock this zone.
         if (writeUnlockInfo(thread, zone, "full-auto-unlock-free"))
         {
            oal_unlock_zone(zone);
         }
      }
   }

   condFreeLockThread(thread);
   return;
}

/* write meta info AFTER a thread gets a zone lock */
void writeLockedInfo(CmsLockThreadInfo *thread, UINT8 zone,
                     MdmObjectId oid, const char *funcName)
{
   CmsLockOwnerInfo *owner;

   oal_lock_meta();

   owner = &(mdmShmCtx->lockMeta->owners[zone]);
   if (owner->tid != CMS_INVALID_PID)
   {
      if (SAME_PID_NS(thread->tid, owner->tid))
      {
         ProcThreadInfo tInfo;
         if ((sysUtl_getThreadInfoFromProc(owner->tid, &tInfo) == -1) ||
             IS_PROC_THREAD_INFO_ZOMBIE(&tInfo))
         {
            cmsLog_debug("acquired zone lock %d from dead process %d",
                         zone, owner->tid);
            mdmShmCtx->lockMeta->stats.undos++;
            clearDeadThreadByTidLocked(owner->tid);
         }
         else
         {
            CmsLockThreadInfo threads[MDM_MAX_LOCK_THREADS];
            UINT32 i;

            mdmShmCtx->lockMeta->stats.internalErrors++;
            cmsLog_error("tid %d got lock %d from LIVE thread %d (%s)",
                         thread->tid, zone, owner->tid, tInfo.name);
            // Due to lock_meta conflict, must manually dump out threads.
            memset(threads, 0, sizeof(threads));
            memcpy(threads, mdmShmCtx->lockMeta->threads,
                   MDM_MAX_LOCK_THREADS * sizeof(CmsLockThreadInfo));
            for (i=0; i < MDM_MAX_LOCK_THREADS; i++)
            {
               if (threads[i].tid != CMS_INVALID_PID)
               {
                  printf("Slot %d:\n", i);
                  cmsLck_dumpLockThreadInfo(&(threads[i]));
                  printf("==========\n");
               }
            }
         }
      }
      else
      {
         // The lock owner was in a different PID Namespace.  Cannot do any
         // extra checking.  Just trust the linux kernel.
         cmsLog_debug("acquired zone lock %d from dead process %d in different pid ns",
                      zone, owner->tid);
         mdmShmCtx->lockMeta->stats.undos++;
         clearDeadThreadByTidLocked(owner->tid);
      }
   }
   owner->tid = thread->tid;
   owner->firstOid = oid;
   owner->currOid = 0;
   cmsTms_get(&(owner->acquiredTs));
   cmsUtl_strncpy(owner->funcName, funcName, sizeof(owner->funcName));
   mdmShmCtx->lockMeta->stats.successes++;

   oal_unlock_meta();

   thread->wantZone = MDM_INVALID_LOCK_ZONE;
   thread->hasZones[zone] = TRUE;
   thread->activityTs = owner->acquiredTs;

   if (mdmShmCtx->lockMeta->traceLevel > 0)
   {
      printf("%s[%d]:%d.%03d:locked zone %d oid %d func %s\n",
             oalLock_name, thread->tid,
             thread->activityTs.sec%1000, thread->activityTs.nsec/1000000,
             zone, oid, funcName);
   }

   return;
}

/** Write meta info BEFORE a thread unlocks a zone lock.
 *  @return TRUE if sanity checks pass and caller should release lock,
 *          FALSE if sanity checks fail and caller should not release lock.
 */
UBOOL8 writeUnlockInfo(CmsLockThreadInfo *thread, UINT8 zone, const char *funcName)
{
   UINT32 holdTime;
   CmsLockOwnerInfo *owner;
   UBOOL8 dumpInfo=FALSE;
   UBOOL8 rc;

   if (thread->hasZones[zone] == FALSE)
   {
      cmsLog_error("tid %d trying to release a zone which it does not own",
                   thread->tid);
      cmsLck_dumpInfo();
      mdmShmCtx->lockMeta->stats.internalErrors++;
      return FALSE;
   }

   thread->hasZones[zone] = FALSE;
   cmsTms_get(&(thread->activityTs));

   oal_lock_meta();

   owner = &(mdmShmCtx->lockMeta->owners[zone]);
   if (owner->tid == thread->tid)
   {
      holdTime = cmsTms_deltaInMilliSeconds(&(thread->activityTs),
                                            &(owner->acquiredTs));
      if (holdTime > thread->holdTimeWarnThresh)
      {
         cmsLog_error("unlocked zone %d: oid %d lock held %d milliseconds; "
                      "exceeds max guideline of %d ms; "
                      "lock func %s; unlock func %s",
                      zone, owner->firstOid,
                      holdTime, thread->holdTimeWarnThresh,
                      owner->funcName, funcName);
      }
      memset(owner, 0, sizeof(CmsLockOwnerInfo));
      rc = TRUE;
   }
   else
   {
      cmsLog_error("tid %d trying to release a zone owned by %d",
                   thread->tid, owner->tid);
      dumpInfo = TRUE;  // do it after meta lock is released.
      mdmShmCtx->lockMeta->stats.internalErrors++;
      rc = FALSE;
   }

   oal_unlock_meta();

   if (dumpInfo)
   {
      cmsLck_dumpInfo();
   }

   if (rc && mdmShmCtx->lockMeta->traceLevel > 0)
   {
      printf("%s[%d]:%d.%03d:unlocked zone %d held %dms\n",
             oalLock_name, thread->tid,
             thread->activityTs.sec%1000, thread->activityTs.nsec/1000000,
             zone, holdTime);
   }

   return rc;
}

/* Return total zone entry counts of the calling thread. */
UINT32 lck_getTotalZoneEntryCounts()
{
   CmsLockThreadInfo *thread;
   SINT32 tid;
   UINT32 count=0;

   oal_lock_meta();

   // Only look for existing thread info entries.  If no entry exists for this
   // thread, do not create one and the count is obviously 0.
   tid = sysUtl_gettid() + pidNsOffset;
   thread = getLockThreadExistingLocked(tid);
   if (thread != NULL)
   {
      count = getTotalZoneEntryCounts(thread);
   }

   oal_unlock_meta();
   return count;
}

CmsLockThreadInfo *lck_getExistingLockThread(SINT32 tid)
{
   CmsLockThreadInfo *thread;

   oal_lock_meta();
   thread = getLockThreadExistingLocked(tid);
   oal_unlock_meta();

   return thread;
}

/** Return ptr to CmsLockThreadInfo for tid.  It could be already in the table,
 *  or we have to find an empty slot for it. */
CmsLockThreadInfo *getLockThread(SINT32 tid)
{
   CmsLockThreadInfo *thread;

   oal_lock_meta();

   // See if this thread is already in the table.
   thread = getLockThreadExistingLocked(tid);
   if (thread != NULL)
   {
      oal_unlock_meta();
      return thread;
   }

   // Could not find existing thread.  Find an available slot for tid.
   thread = getLockThreadNewLocked(tid);
   if (thread != NULL)
   {
      oal_unlock_meta();
      return thread;
   }

   cmsLog_error("No thread slot available! Clear out dead threads...");
   clearDeadThreadsLocked();

   // Try one more time to find an available slot.
   thread = getLockThreadNewLocked(tid);
   if (thread != NULL)
   {
      oal_unlock_meta();
      return thread;
   }

   cmsLog_error("No thread slot even after dead thread collection!!");
   mdmShmCtx->lockMeta->stats.threadSlotsFull++;
   oal_unlock_meta();
   cmsLck_dumpInfo();  // do this after meta lock is released.
   return NULL;
}

/** Helper function to find an existing lock thread. 
 *  Caller must have meta-lock. */
CmsLockThreadInfo *getLockThreadExistingLocked(SINT32 tid)
{
   UINT32 i;
   CmsLockThreadInfo *thread;

   for (i=0; i < MDM_MAX_LOCK_THREADS; i++)
   {
      thread = &(mdmShmCtx->lockMeta->threads[i]);
      if (thread->tid == tid)
      {
         return thread;
      }
   }

   return NULL;
}

/** Helper function to find an available slot for tid.
 *  Caller must have meta-lock. */
CmsLockThreadInfo *getLockThreadNewLocked(SINT32 tid)
{
   UINT32 i;
   CmsLockThreadInfo *thread;

   for (i=0; i < MDM_MAX_LOCK_THREADS; i++)
   {
      thread = &(mdmShmCtx->lockMeta->threads[i]);
      if (thread->tid == CMS_INVALID_PID)
      {
         // Initialize all fields before returning it to caller.
         memset(thread, 0, sizeof(CmsLockThreadInfo));
         cmsTms_get(&(thread->activityTs));
         thread->wantZone = MDM_INVALID_LOCK_ZONE;
         thread->holdTimeWarnThresh = CMSLCK_MAX_HOLDTIME;
         thread->tid = tid;
         return thread;
      }
   }

   return NULL;
}

/** Look for dead threads in the thread table and clear out their slot.
 *  Caller must have meta-lock. */
void clearDeadThreadsLocked()
{
   UINT32 i;
   CmsLockThreadInfo *thread;

   for (i=0; i < MDM_MAX_LOCK_THREADS; i++)
   {
      thread = &(mdmShmCtx->lockMeta->threads[i]);
      if ((thread->tid != CMS_INVALID_PID) &&
           SAME_PID_NS(thread->tid, pidNsOffset))
      {
         ProcThreadInfo tInfo;
         if ((sysUtl_getThreadInfoFromProc(thread->tid, &tInfo) == -1) ||
             IS_PROC_THREAD_INFO_ZOMBIE(&tInfo))
         {
            // Could not find this thread in /proc, must be dead.
            cmsLog_error("found dead thread %d in slot %d, clear it!",
                         thread->tid, i);
            mdmShmCtx->lockMeta->stats.deadThreads++;

            // The mdmObj in getCache is in the dead threads private heap memory. This caller should
            // not free it.
            if(thread->getCache.mdmObj)
            {
               cmsLog_debug("getCache.mdmObj:%p, set to NULL",thread->getCache.mdmObj);
               thread->getCache.mdmObj = NULL;
            }

            thread->tid = CMS_INVALID_PID;
            // Note that we do not clean up the lock owners table here.
            // We will do that when another thread gets that lock.
         }
      }
   }

   return;
}

/** Find the specified (dead) thread id in the threads table and clear it.
 *  Caller must have meta-lock. */
void clearDeadThreadByTidLocked(SINT32 tid)
{
   UINT32 i;
   CmsLockThreadInfo *thread;

   for (i=0; i < MDM_MAX_LOCK_THREADS; i++)
   {
      thread = &(mdmShmCtx->lockMeta->threads[i]);
      if (thread->tid == tid)
      {
          mdmShmCtx->lockMeta->stats.deadThreads++;

          // The mdmObj in getCache is in the dead threads private heap memory. This caller should
          // not free it.
          if(thread->getCache.mdmObj)
          {
             cmsLog_debug("getCache.mdmObj:%p, set to NULL",thread->getCache.mdmObj);
             thread->getCache.mdmObj = NULL;
          }

          thread->tid = CMS_INVALID_PID;
          break;
      }
   }

   return;
}

/** Special case where a pthread exits while holding a zone lock but the
 *  process is still alive.  Linux SEM_UNDO feature is not triggered
 *  in this scenario, so we have to reset the semaphore state in the Linux
 *  kernel and also clean up our data structures.
 *  This function will acquire meta-lock.
 *  @return TRUE if cleanup actually happened.
 */
UBOOL8 checkAndClearDeadPthread(UINT8 zone)
{
   CmsLockOwnerInfo *owner;
   ProcThreadInfo tInfo;
   UBOOL8 found=FALSE;

   oal_lock_meta();

   owner = &(mdmShmCtx->lockMeta->owners[zone]);
   if (owner->tid == CMS_INVALID_PID)
   {
      // The owner could have released the lock right before we came into this
      // function.  Tell caller to try to get lock again.
      found = TRUE;
   }
   else if (!SAME_PID_NS(owner->tid, pidNsOffset))
   {
      // Owner is in a different PID namespace.  Do not have correct info
      // about whether it is running or not.
      found = FALSE;
   }
   else
   {
      if ((sysUtl_getThreadInfoFromProc(owner->tid, &tInfo) == -1) ||
          IS_PROC_THREAD_INFO_ZOMBIE(&tInfo))
      {
         CmsLockThreadInfo threads[MDM_MAX_LOCK_THREADS];
         UINT32 i;

         cmsLog_error("Dead pthread %d detected, reset zone lock %d",
                      owner->tid, zone);
         // Due to lock_meta conflict, must manually dump out threads.
         memset(threads, 0, sizeof(threads));
         memcpy(threads, mdmShmCtx->lockMeta->threads,
                MDM_MAX_LOCK_THREADS * sizeof(CmsLockThreadInfo));
         for (i=0; i < MDM_MAX_LOCK_THREADS; i++)
         {
            if (threads[i].tid != CMS_INVALID_PID)
            {
               printf("Slot %d:\n", i);
               cmsLck_dumpLockThreadInfo(&(threads[i]));
               printf("==========\n");
            }
         }

         found = TRUE;
         // This dead thread might be holding other zone locks.  But just
         // free/clear the thread and this zone lock.  The other zones locks
         // will get cleared when another thread tries lock access them.
         clearDeadThreadByTidLocked(owner->tid);
         mdmShmCtx->lockMeta->stats.resets++;
         memset(owner, 0, sizeof(CmsLockOwnerInfo));
         oal_reset_zone(zone);  // reset kernel lock state back to unlocked
      }
   }

   oal_unlock_meta();
   return found;
}

/** If this thread is not doing any locking stuff, clear it (make its slot
 *  in the thread table available)
 */
void condFreeLockThread(CmsLockThreadInfo *thread)
{
   if ((getTotalZoneEntryCounts(thread) == 0) &&
       (getTotalZoneLockCounts(thread) == 0) &&
       (thread->totalTrackedMdmObjs == 0) &&
       (thread->globalLockCount == 0))
   {
      UINT32 lockCount;
      lockCount = getTotalHasZones(thread);
      if (lockCount != 0)
      {
         cmsLog_error("thread %d has 0 counts for everything but still holding %d locks!",
                      thread->tid, lockCount);
         cmsLck_dumpInfo();
         mdmShmCtx->lockMeta->stats.internalErrors++;
      }
      else
      {
         UINT32 i;
         for (i=0; i < MDM_MAX_LOCK_ZONES; i++)
         {
            if (thread->tracked[i].array != NULL)
            {
               CMSMEM_FREE_BUF_AND_NULL_PTR(thread->tracked[i].array);
            }
         }
         // This function is called without the meta-lock, so be careful how
         // the fields are cleared.  tid is how other functions tell if this
         // slot is available.  Just clear the tid.  This slot will be memset
         // to 0 when it is assigned to the next thread.
         INVALIDATE_ODL_GET_CACHE;
         thread->tid = CMS_INVALID_PID;
      }
   }
   return;
}

UINT32 getTotalZoneEntryCounts(const CmsLockThreadInfo *thread)
{
   UINT32 i;
   UINT32 total=0;

   for (i=0; i < MDM_MAX_LOCK_ZONES; i++)
   {
      total += thread->zoneEntryCounts[i];
   }

   return total;
}

UINT32 getTotalZoneLockCounts(const CmsLockThreadInfo *thread)
{
   UINT32 i;
   UINT32 total=0;

   for (i=0; i < MDM_MAX_LOCK_ZONES; i++)
   {
      total += thread->zoneLockCounts[i];
   }

   return total;
}

UINT32 getTotalHasZones(const CmsLockThreadInfo *thread)
{
   UINT32 i;
   UINT32 total=0;

   for (i=0; i < MDM_MAX_LOCK_ZONES; i++)
   {
      // hasZones is a boolean, so just add 1 to total if hasZone[i] == TRUE.
      total += thread->hasZones[i] ? 1 : 0;
   }

   return total;
}
