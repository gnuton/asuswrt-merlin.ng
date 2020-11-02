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

#include "cms.h"
#include "cms_util.h"
#include "sysutil.h"
#include "sysutil_proc.h"
#include "../oal.h"
#include <errno.h>
#include <sys/sem.h>

#if defined(DESKTOP_LINUX) || defined(__ARM_EABI__) || defined(__LP64__)
/*
 * In EABI-ARM gcc compiler, it does not support __NR_ipc, so we have to make
 * an exception here.
 */
/*
 * On DESKTOP_LINUX, semtimedop is hidden under the symbol __USE_GNU.
 * Just define it here for symetry with what I have to do below.
 */
extern int semtimedop (int __semid, struct sembuf *__sops, size_t __nsops,
                       __const struct timespec *__timeout);
#else

/* 
 * On the modem, our kernel acutally supports semtimedop, but our uclibc does not.
 * So all I am doing below is providing the path from the user level to the kernel.
 */
#include <sys/syscall.h>
#include <unistd.h> 

#define IPCOP_semtimedop  4
int semtimedop(int semid, struct sembuf *sops, size_t nsops, const struct timespec *timeout)
{
   return syscall(__NR_ipc, IPCOP_semtimedop, semid, (int) nsops, 0, (unsigned long) sops, (unsigned long) timeout);
}

#endif /* DESKTOP_LINUX */


/** My thread name for lock tracing. */
char oalLock_name[PROC_THREAD_NAME_LEN];

/** Linux kernel semaphore id for the zone locks.
 *  This semid points to an array of MDM_MAX_LOCK_ZONES semaphores.
 */
static SINT32 zone_semid=-1;

/** Linux kernel semaphore id for the lock meta-info tables.
 *  This semid points to a single semaphore.
 */
static SINT32 meta_semid=-1;


static CmsRet lock_generic(SINT32 semid, UINT32 semIndex, const UINT32 *timeoutMs);
static void unlock_generic(SINT32 semid, UINT32 semIndex);



CmsRet oalLck_init(UBOOL8 attachExisting)
{
   UINT32 flags;
   UINT32 i;

   flags = (attachExisting) ? 0 : IPC_CREAT;

   /* get the zone lock array */
   if ((zone_semid = semget(MDM_ZONE_LOCK_SEMAPHORE_KEY, MDM_MAX_LOCK_ZONES, flags|0666)) == -1)
   {
      cmsLog_error("semget for zone locks failed, errno=%d", errno);
      return CMSRET_INTERNAL_ERROR;
   }

   /* get the meta-info lock (this is a single lock) */
   if ((meta_semid = semget(MDM_META_LOCK_SEMAPHORE_KEY, 1, flags|0666)) == -1)
   {
      cmsLog_error("semget for meta-info lock failed, errno=%d", errno);
      return CMSRET_INTERNAL_ERROR;
   }
   
   /* initialize my name for lock tracing */
   {
      ProcThreadInfo tInfo;
      int tid = sysUtl_gettid();

      sysUtl_getThreadInfoFromProc(tid, &tInfo);
      strcpy(oalLock_name, tInfo.name);
   }

   if (attachExisting)
   {
      cmsLog_notice("attach existing done, zone_semid=%d meta_semid=%d",
                    zone_semid, meta_semid);
      return CMSRET_SUCCESS;
   }

   /* Initialize all semaphores to 0 (unlocked state) */
   for (i=0; i < MDM_MAX_LOCK_ZONES; i++)
   {
      if(oal_reset_zone(i) == -1)
      {
         cmsLog_error("semctl setval 0 failed on %d, errno=%d", i, errno);
         oalLck_cleanup();
         return CMSRET_INTERNAL_ERROR;
      }
   }
   
   if(semctl(meta_semid, 0, SETVAL, 0) == -1)
   {
      cmsLog_error("semctl setval 0 failed meta-info, errno=%d", errno);
      oalLck_cleanup();
      return CMSRET_INTERNAL_ERROR;
   }

   /*
    * The MDM zone lock meta info tables has not been allocated yet.
    * They will get zero'd out after they are allocated in copyToSharedMem().
    */

   return CMSRET_SUCCESS;
}


void oalLck_cleanup(void)
{
   SINT32 rc;

   if ((rc = semctl(zone_semid, 0, IPC_RMID)) < 0)
   {
      cmsLog_error("zone IPC_RMID failed, errno=%d", errno);
   }
   else
   {
      cmsLog_notice("zone_semid %d deleted.", zone_semid);
      zone_semid = -1;
   }

   if ((rc = semctl(meta_semid, 0, IPC_RMID)) < 0)
   {
      cmsLog_error("meta IPC_RMID failed, errno=%d", errno);
   }
   else
   {
      cmsLog_notice("meta_semid %d deleted.", meta_semid);
      meta_semid = -1;
   }

   return;
}


CmsRet oal_lock_zone(UINT8 zone, UBOOL8 useTimeout, UINT32 timeoutMs)
{
   UINT32 localTimeoutMs = timeoutMs;
   CmsRet ret;

   if (useTimeout)
   {
      ret = lock_generic(zone_semid, zone, &localTimeoutMs);
   }
   else
   {
      ret = lock_generic(zone_semid, zone, NULL);
   }

   return ret;
}

CmsRet oal_lock_meta()
{
   CmsRet ret;

   // There is only 1 meta-info lock, so index is always 0.
   // Getting meta-info should be fast, so don't need timeout.
   ret = lock_generic(meta_semid, 0, NULL);
   return ret;
}

CmsRet lock_generic(SINT32 semid, UINT32 semIndex, const UINT32 *timeoutMs)
{
   struct sembuf lockOp[2];
   SINT32 rc=-1;
   UINT32 timeRemainingMs=0;
   CmsTimestamp startTms, stopTms;
   CmsRet ret=CMSRET_SUCCESS;

   lockOp[0].sem_num = semIndex;
   lockOp[0].sem_op = 0; /* wait for zero: block until write count goes to 0. */
   lockOp[0].sem_flg = 0;

   lockOp[1].sem_num = semIndex;
   lockOp[1].sem_op = 1; /* incr sem count by 1 */
   lockOp[1].sem_flg = SEM_UNDO; /* automatically undo this op if process terminates. */

   if (timeoutMs != NULL)
   {
      timeRemainingMs = *timeoutMs;
   }

   while (TRUE)
   {
      /*
       * If user specified a timeout, initialize pTimeout and pass it to semtimedop.
       * If fourth arg to semtimedop is NULL, then it blocks indefinately.
       */
      if (timeoutMs != NULL)
      {
         struct timespec timeout;

         cmsTms_get(&startTms);
         timeout.tv_sec = timeRemainingMs / MSECS_IN_SEC;
         timeout.tv_nsec = (timeRemainingMs % MSECS_IN_SEC) * NSECS_IN_MSEC;
         rc = semtimedop(semid, lockOp, sizeof(lockOp)/sizeof(struct sembuf), &timeout);
      }
      else
      {
         rc = semop(semid, lockOp, sizeof(lockOp)/sizeof(struct sembuf));
      }

      /*
       * Our new 2.6.21 MIPS kernel returns the errno in the rc, but my Fedora 7 
       * with 2.6.22 kernel still returns -1 and sets the errno.  So check for both.
       */
      if ((rc == -1 && errno == EINTR) ||
          (rc > 0 && rc == EINTR))
      {
         /*
          * Our semaphore operation was interrupted by a signal or something,
          * go back to the top of while loop and keep trying.
          * But if user has specified a timeout, we have to calculate how long
          * we have waited already, and how much longer we need to wait.
          */
         if (timeoutMs != NULL)
         {
            UINT32 elapsedMs;

            cmsTms_get(&stopTms);
            elapsedMs = cmsTms_deltaInMilliSeconds(&stopTms, &startTms);

            if (elapsedMs >= timeRemainingMs)
            {
               /* even though we woke up because of EINTR, we have waited long enough */
               rc = EAGAIN;
               break;
            }
            else
            {
               /* subtract the time we already waited and wait some more */
               timeRemainingMs -= elapsedMs;
            }
         }
      }
      else
      {
         /* If we get any error other than EINTR, break out of the loop */
         break;
      }
   }

   if (rc != 0)
   {
      /*
       * most likely cause of error is caught signal, we could also
       * get EIDRM if someone deletes the semphore while we are waiting
       * for it (that indicates programming error.)
       */
      if (errno == EINTR || rc == EINTR)
      {
         cmsLog_notice("lock interrupted by signal");
         ret = CMSRET_OP_INTR;
      }
      else if (errno == EAGAIN || rc == EAGAIN)
      {
         /* the new 2.6.21 kernel seems to return the errno in the rc */
         cmsLog_debug("timed out, errno=%d rc=%d", errno, rc);
         return CMSRET_TIMED_OUT;
      }
      else
      {
         cmsLog_error("lock failed, errno=%d rc=%d", errno, rc);
         ret = CMSRET_INTERNAL_ERROR;
      }
   }

   return ret;
}


void oal_unlock_zone(UINT8 zone)
{
   unlock_generic(zone_semid, zone);
   return;
}

void oal_unlock_meta()
{
   UINT32 semIndex=0;
   unlock_generic(meta_semid, semIndex);
   return;
}

void unlock_generic(SINT32 semid, UINT32 semIndex)
{
   struct sembuf unlockOp[1];
   SINT32 semval;

   unlockOp[0].sem_num = semIndex;
   unlockOp[0].sem_op = -1; /* decr sem count by 1 */
   unlockOp[0].sem_flg = SEM_UNDO; /* undo the undo state built up in the kernel during the lockOp */


   /* kernel should have semval of 1 */
   if ((semval = semctl(semid, semIndex, GETVAL, 0)) != 1)
   {
      cmsLog_error("semid=%d index=%d: kernel has semval=%d (should be 1)",
                   semid, semIndex, semval);
      cmsAst_assert(0);
   }

   /* now do the actual release */
   if (semop(semid, unlockOp, sizeof(unlockOp)/sizeof(struct sembuf)) == -1)
   {
      cmsLog_error("semid=%d index=%d: release lock failed, errno=%d",
                   semid, semIndex, errno);
   }

   return;
}

SINT32 oal_reset_zone(UINT8 zone)
{
   SINT32 rc;
   
   rc = semctl(zone_semid, zone, SETVAL, 0);
   return rc;
}
