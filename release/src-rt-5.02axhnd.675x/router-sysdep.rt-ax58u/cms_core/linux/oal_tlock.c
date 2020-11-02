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


#include "cms.h"
#include "cms_util.h"
#include "../oal.h"
#include <errno.h>
#include <sys/sem.h>

static SINT32 semid=-1;

CmsRet oalTlk_init(BOOL attachExisting)
{
   UINT32 flags;

   flags = (attachExisting) ? 0 : IPC_CREAT;

   if ((semid = semget(MDM_TLOCK_SEMAPHORE_KEY, 1, flags|0666)) == -1)
   {
      cmsLog_error("semget failed, errno=%d", errno);
      return CMSRET_INTERNAL_ERROR;
   }

   if (attachExisting)
   {
      return CMSRET_SUCCESS;
   }

   /*
    * We are creating new semaphore, so initialize semaphore to 0.
    */
   if(semctl(semid, 0, SETVAL, 0) == -1)
   {
      cmsLog_error("setctl setval 0 failed, errno=%d", errno);
      oalTlk_cleanup();
      return CMSRET_INTERNAL_ERROR;
   }

   mdmShmCtx->tlockOwner = 0;
   mdmShmCtx->tlockCount = 0;

   return CMSRET_SUCCESS;
}

void oalTlk_cleanup(void)
{
   SINT32 rc;

   if ((rc = semctl(semid, 0, IPC_RMID)) < 0)
   {
      cmsLog_error("IPC_RMID failed, errno=%d", errno);
   }
   else
   {
      if ((mdmShmCtx->tlockOwner != 0) || (mdmShmCtx->tlockCount != 0))
      {
         cmsLog_error("bad state, owner=%d count=%d",
                      mdmShmCtx->tlockOwner,
                      mdmShmCtx->tlockCount);
      }

      cmsLog_notice("Semid %d deleted.", semid);

      mdmShmCtx->tlockOwner = 0;
      mdmShmCtx->tlockCount = 0;
      semid = -1;
   }
}


CmsRet oal_tlock(void)
{
   /*
    * The first set of {0, 0, 0} is a "wait for zero" operation.
    * It causes the calling process to block until the semphore reaches 0.
    *
    * The second set of {0, 1, SEM_UNDO} increments the semaphore by 1,
    * effectively acquiring the lock for the process. The SEM_UNDO will
    * automatically release the semaphore when the  process terminates.
    */
   struct sembuf lockOp[2] = {{0, 0, 0}, {0, 1, SEM_UNDO}}; 
   CmsRet ret=CMSRET_SUCCESS;


   /*
    * Make sure I don't already own the lock, otherwise, I will block
    * forever waiting for myself to release the lock.  If this condition
    * is detected, log error and continue.
    */
   if (mdmLibCtx.pid == mdmShmCtx->tlockOwner)
   {
      cmsLog_error("already has tlock, owner=%d count=%d",
                   mdmShmCtx->tlockOwner,
                   mdmShmCtx->tlockCount);
      /* cmsAst_assert(0); too strict? */
      return ret;
   }


   /*
    * This semop may block.
    * There is also a semtimedop variant that limits the duration
    * of the sleep.
    */
   if (semop(semid, lockOp, sizeof(lockOp)/sizeof(struct sembuf)) == -1)
   {
      /*
       * most likely cause of error is caught signal, we could also
       * get EIDRM if someone deletes the semphore while we are waiting
       * for it (that indicates programming error.)
       */
      cmsLog_error("lock failed, errno=%d", errno);
      if (errno == EINTR)
      {
         ret = CMSRET_OP_INTR;
      }
      else
      {
         ret = CMSRET_INTERNAL_ERROR;
      }
   }
   else
   {
      /*
       * Because of the SEM_UNDO feature, when I acquire the tlock,
       * if I notice that the owner is not 0, then that means the previous
       * owner died suddenly and did not clean up.  Clean up for him.
       */
      if (mdmShmCtx->tlockOwner != 0)
      {
         mdmShmCtx->tlockCount = 0;
      }

      mdmShmCtx->tlockOwner = mdmLibCtx.pid;
      mdmShmCtx->tlockCount++;
   }

   return ret;
}


void oal_tunlock(void)
{
   struct sembuf unlockOp[1] = {{0, -1, IPC_NOWAIT|SEM_UNDO}};


   if (mdmLibCtx.pid != mdmShmCtx->tlockOwner)
   {
      cmsLog_error("not owner of the tlock, current owner=%d (me=%d) count=%d",
                   mdmShmCtx->tlockOwner,
                   mdmLibCtx.pid,
                   mdmShmCtx->tlockCount);
      /* cmsAst_assert(0); too strict? */
      return;
   }

   if (semop(semid, unlockOp, sizeof(unlockOp)/sizeof(struct sembuf)) == -1)
   {
      cmsLog_error("unlock failed, errno=%d", errno);
   }
   else
   {
      mdmShmCtx->tlockOwner = 0;
      mdmShmCtx->tlockCount--;
   }
}
