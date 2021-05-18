/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
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


#include <dlfcn.h>
#include <sys/shm.h>  /* for shmat */
#include <sys/stat.h> /* for stat */
#include <fcntl.h>
#include <errno.h>
#include "../oal.h"
#include "cms_util.h"
#include "cms_boardioctl.h"
#include "bcm_flashutil.h"
#include "genutil_crc.h"

/** This structure is used to track mapping of valid strings arrays from
 * the libmdm.so library to the shared memory region.   It is only used
 * during the copy over from libmdm.so to shared memory region.
 */
struct vsa_entry
{
   const char *libAddr;
   char *shmAddr;
   struct vsa_entry *next;
};


/** This structure is used to track strings that were copied from libmdm.so
 * to the shared memory region.  It is only used
 * during the copy over from libmdm.so to shared memory region.
 */
struct str_entry
{
   char *shmStr;
   struct str_entry *next;
};


/** round a char * pointer to the next word boundary */
#ifdef __LP64__
#define ROUNDUP_WORD(s) ((char *) ((((UINT64) (s)) + 0x7) & ((UINT64) 0xfffffffffffffffc)))
#else
#define ROUNDUP_WORD(s) ((char *) ((((UINT32) (s)) + 0x3) & ((UINT32) 0xfffffffc)))
#endif

/* from mdm.h */
extern MdmSharedMemContext *mdmShmCtx;
void freeObjectMemory(MdmObjectNode *objNode);


static UINT32 copyToSharedMem(MdmSharedMemContext *sharedCtx, const MdmObjectNode *rootObj);
static UINT32 getMdmSize(const MdmObjectNode *objNode);
static char *copyNode(MdmObjectNode *objNode,
                      MdmObjectNode **oidNodePtrTable,
                      char **nodeCopyAddr,
                      char **strCopyAddr,
                      struct vsa_entry **vsaMap,
                      struct str_entry **strMap,
                      const char *shmEnd);

static char *getSharedVsa(struct vsa_entry **vsaMap,
                          struct str_entry **strMap,
                          const char *libAddr,
                          char **strCopyAddr,
                          const char *shmEnd);
static char *copyVsa(struct str_entry **strMap,
                     const char **libArray,
                     char **strCopyAddr,
                     const char *shmEnd);
static void freeVsaMap(struct vsa_entry **vsaMap);

static char *getSharedStr(struct str_entry **strMap,
                          const char *libStr,
                          char **strCopyAddr,
                          const char *shmEnd);
static void freeStrMap(struct str_entry **strMap);

static void detachShm(void *shmAddr);
static SINT32 initDataModelSelection(void);



CmsRet oalShm_init(UINT32 shmSize, SINT32 *shmId, void **shmAddr)
{
   void *addr;
   void *libmdmHandle;
   char *mdmFilename;
#if defined(DESKTOP_LINUX) && !defined(BUILD_DESKTOP_BEEP)
   char mdmPathBuf[CMS_MAX_FULLPATH_LENGTH*2]={0};
#else
   char mdmPathBuf[CMS_MAX_FULLPATH_LENGTH]={0};
#endif
   SINT32 id;
   UINT32 shmflg, consumed;
   MdmObjectNode *igdRootObjNode;
   CmsRet ret=CMSRET_SUCCESS;

   if ((*shmId) != UNINITIALIZED_SHM_ID)
   {
      cmsLog_notice("attaching to existing shmId=%d", *shmId);
      addr = shmat(*shmId, (void *)MDM_SHM_ATTACH_ADDR, 0);
      if (addr == (void *) -1)
      {
         cmsLog_error("Could not attach to shmId=%d at 0x%x", *shmId, MDM_SHM_ATTACH_ADDR);
         *shmAddr = NULL;
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         mdmShmCtx = (MdmSharedMemContext *) addr;

         /*
          * Every process needs to call oalLck_init, but oalLck_cleanup
          * only needs to be called once.
          */
         if ((ret = oalLck_init(TRUE)) != CMSRET_SUCCESS)
         {
            cmsLog_error("oalLck_init failed, ret=%d", ret);
            return ret;
         }

         /*
          * Caller's copy of shared memory allocator just needs to know
          * the address of the allocatable region.  The shared memory
          * allocator has already been fully initialized by the first
          * caller who created the shared memory region.
          */
         cmsMem_initSharedMemPointer(mdmShmCtx->mallocStart,
                          mdmShmCtx->shmEnd - ((char *) mdmShmCtx->mallocStart));

         *shmAddr = addr;



         return CMSRET_SUCCESS;
      }
   }
    

   /*
    * OK, if we get here, then shmId must be -1, which means we
    * have to create the shared memory region and do all the hard
    * work of copying the MDM data structures into the shared memory region.
    */

   /*
    * Create the shared memory region.
    */
   cmsLog_notice("creating shared memory region, size=%d (0x%x)", shmSize, shmSize);
   shmflg = 0666; /* allow everyone to read/write */
   shmflg |= (IPC_CREAT | IPC_EXCL);

   if ((id = shmget(0, shmSize, shmflg)) == -1)
   {
      cmsLog_error("Could not create shared memory.");
      return CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_debug("created shared memory, shmid=%d", id);
   }

   if ((addr = shmat(id, (void *)MDM_SHM_ATTACH_ADDR, 0)) == (void *) -1)
   {
      cmsLog_error("could not attach %d at 0x%x", id, MDM_SHM_ATTACH_ADDR);
      oalShm_cleanup(id, NULL);
   }

   mdmShmCtx = (MdmSharedMemContext *) addr;
   mdmShmCtx->shmEnd = (char *) (addr + shmSize);

   /*
    * Initialize the MDM xlock earlier because it is easy to undo if we
    * encounter errors later on.
    */
   if ((ret = oalLck_init(FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("oalLck_init failed, ret=%d", ret);
      return ret;
   }

   /* figure out which data model shared lib we need to open and load */
   {
      SINT32 dataModelRetVal;

      dataModelRetVal = initDataModelSelection();
      if (dataModelRetVal == 0)
      {
         mdmFilename = "libmdm.so";
         mdmShmCtx->isDataModelDevice2 = 0;
      }
      else if (dataModelRetVal == 1)
      {
         mdmFilename = "libmdm2.so";
         mdmShmCtx->isDataModelDevice2 = 1;
      }
      else
      {
         cmsLog_error("initDataModelSelection failed, retval=%d", dataModelRetVal);
         return CMSRET_INTERNAL_ERROR;
      }
   }

   /* On real system and BEEP, just filename is sufficient. */
   snprintf(mdmPathBuf, sizeof(mdmPathBuf)-1, "%s", mdmFilename);
#if defined(DESKTOP_LINUX) && !defined(BUILD_DESKTOP_BEEP)
   {
       char tmpBuf[CMS_MAX_FULLPATH_LENGTH]={0};
       ret = cmsUtl_getRunTimeRootDir(tmpBuf, sizeof(tmpBuf));
       if (ret != CMSRET_SUCCESS)
       {
           cmsLog_error("Could not get RunTime root dir!");
           return ret;
       }
       snprintf(mdmPathBuf, sizeof(mdmPathBuf)-1,
                "%s/lib/%s",tmpBuf, mdmFilename);
   }
#endif /* DESKTOP_LINUX */

   /*
    * dlopen the mdm shared library.
    * On modem, must have RTLD_LAZY or the dlopen will fail.
    */
   libmdmHandle = dlopen(mdmPathBuf, RTLD_LAZY);
   if (libmdmHandle == NULL)
   {
      cmsLog_error("could not open %s, errno=%d", mdmFilename, errno);
      oalLck_cleanup();
      oalShm_cleanup(id, addr);
      return CMSRET_INTERNAL_ERROR;
   }

   igdRootObjNode = (MdmObjectNode *) dlsym(libmdmHandle, "igdRootObjNode");
   if (igdRootObjNode == NULL)
   {
      cmsLog_error("could not find symbol igdRootObjNode");
      dlclose(libmdmHandle);
      oalLck_cleanup();
      oalShm_cleanup(id, addr);
      return CMSRET_INTERNAL_ERROR;
   }

   cmsLog_debug("got dlsym idgRootObjNode at %p", igdRootObjNode);


   /*
    * Copy contents of libmdm.so to shared memory.
    * Once we've done that, we can close libmdm.so
    */
   consumed = copyToSharedMem(mdmShmCtx, igdRootObjNode);
   dlclose(libmdmHandle);

   if (consumed == 0)
   {
      oalLck_cleanup();
      oalShm_cleanup(id, addr);
      return CMSRET_RESOURCE_EXCEEDED;
   }



   /*
    * Let the shared memory allocator know where it can allocate shared
    * memory from.  As a heuristic, the shared memory allocator should
    * have at least half the shared memory region available to it.
    * If we become heavily invested in the shared mem approach, the
    * shared memory allocator (bget) does have the ability to accept more
    * memory into its pool, so we could slowly grow it on an as needed basis.
    * This will cause more complexity in the code, of course.
    */
   mdmShmCtx->mallocStart = addr + consumed;

   if (shmSize - consumed < (shmSize/3))
   {
      cmsLog_error("insufficient memory for shared memory allocator, got %d, need %d",
                   shmSize - consumed, (shmSize/3));
      oalLck_cleanup();
      oalShm_cleanup(id, addr);
      return CMSRET_RESOURCE_EXCEEDED;
   }
   else
   {
      cmsLog_debug("init shared mem allocator with %p, size=%d",
                   mdmShmCtx->mallocStart, shmSize - consumed);
      cmsMem_initSharedMem(mdmShmCtx->mallocStart, shmSize - consumed);
   }


   *shmId = id;
   *shmAddr = addr;

   return ret;
}


void oalShm_cleanup(SINT32 shmId, void *shmAddr)
{
   struct shmid_ds shmbuf;

   /*
    * stat the shared memory to see how many processes are attached.
    */
   memset(&shmbuf, 0, sizeof(shmbuf));
   if (shmctl(shmId, IPC_STAT, &shmbuf) < 0)
   {
      cmsLog_error("shmctl IPC_STAT failed");
      return;
   }
   else
   {
      cmsLog_debug("nattached=%d", shmbuf.shm_nattch);
   }



   if (shmbuf.shm_nattch > 1)
   {
      /* other proceeses are still attached, just detach myself and return now. */
      detachShm(shmAddr);
      return;
   }

   /*
    * No other processes attached to memory region, first free all the dynamically
    * allocated shared memory (this will make the memory leak checkers happy),
    * then detach myself, and then destroy the shared memory region.
    */
   cmsLog_debug("freeing all MdmObjects");
   freeObjectMemory(mdmShmCtx->rootObjNode);

   cmsMem_cleanup();

   oalLck_cleanup();

   detachShm(shmAddr);

   memset(&shmbuf, 0, sizeof(shmbuf));
   if (shmctl(shmId, IPC_RMID, &shmbuf) < 0)
   {
      cmsLog_error("shm destory of shmId=%d failed.", shmId);
   }
   else
   {
      cmsLog_debug("shared mem (shmId=%d) destroyed.", shmId);
   }

   return;
}


/** Detach the shared memory address. */
void detachShm(void *shmAddr)
{
   if (shmAddr == NULL)
   {
      cmsLog_error("got uninitialized shmAddr, no detach needed");
   }
   else
   {
      if (shmdt(shmAddr) != 0)
      {
         cmsLog_error("shmdt of shmAddr=%p failed", shmAddr);
      }
      else
      {
         cmsLog_debug("detached shmAddr=%p", shmAddr);
      }
   }
}


UINT32 getMdmSize(const MdmObjectNode *objNode)
{
   UINT32 i, s=0;

   /* first count my own size */
   s = sizeof(MdmObjectNode);

   /* get the size of my parameters */
   s += objNode->numParamNodes * sizeof(MdmParamNode);

   /* get the size of my children/sub-trees */
   for (i=0; i < objNode->numChildObjNodes; i++)
   {
      /* recurse */
      s += getMdmSize(&(objNode->childObjNodes[i]));
   }

   return s;
}


UINT32 copyToSharedMem(MdmSharedMemContext *shmCtx, const MdmObjectNode *rootObjNode)
{
   UINT32 s, consumed;
   char *nodeCopyAddr, *strCopyAddr, *strEnd;
   struct str_entry *strMap=NULL;
   struct vsa_entry *vsaMap=NULL;

   s = getMdmSize(rootObjNode);
   cmsLog_debug("mdm size = %d MDM_MAX_OID=%d", s, MDM_MAX_OID);

   /* set the various pointers in the shmCtx */
   /* TODO: the oidNodePtrTable was designed for sequential OIDs.  But now
    * the OIDs are sparse, so we end up setting aside an array of
    * MDM_MAX_OID in the thousands, but only using a few hundred of the slots.
    * This needs to be made more memory efficient.
    */
   shmCtx->lockMeta = (void *) (MDM_SHM_ATTACH_ADDR + sizeof(MdmSharedMemContext));
   shmCtx->oidNodePtrTable = (void *) (shmCtx->lockMeta + 1);
   shmCtx->rootObjNode = (MdmObjectNode *) (shmCtx->oidNodePtrTable + (MDM_MAX_OID + 1));
   shmCtx->stringsStart = (char *) (((char *)shmCtx->rootObjNode) + s);

   cmsLog_debug("start          = 0x%x", MDM_SHM_ATTACH_ADDR);
   cmsLog_debug("LockMetaInfo   = 0x%x (+0x%x)", shmCtx->lockMeta, sizeof(MdmSharedMemContext));
   cmsLog_debug("oidNodePtrTable= 0x%x (+0x%x)", shmCtx->oidNodePtrTable, sizeof(MdmLockMetaInfo));
   cmsLog_debug("rootObjNode    = 0x%x (+0x%x)", shmCtx->rootObjNode, (MDM_MAX_OID+1)*sizeof(void *));
   cmsLog_debug("stringStart    = 0x%x (+0x%x)", shmCtx->stringsStart, s);

   if (shmCtx->stringsStart > shmCtx->shmEnd)
   {
      /* not even enough space to hold the fixed sized data */
      cmsLog_error("not enough space: start %d > end %d",
                     shmCtx->stringsStart,  shmCtx->shmEnd);
      return 0;
   }

   // Due to various call sequence issues, we need to zero out lockMeta here
   // instead of in the lock init function.
   memset(shmCtx->lockMeta, 0, sizeof(MdmLockMetaInfo));

   /*
    * Using a recursive function, I can copy over the MdmObjNode and MdmParamNode
    * structures, update the oidNodePtrTable, and copy over the validStrings
    * in one pass through the tree.
    */
   nodeCopyAddr = (char *) shmCtx->rootObjNode;
   strCopyAddr = shmCtx->stringsStart;

   strEnd = copyNode((MdmObjectNode *) rootObjNode,
                     shmCtx->oidNodePtrTable,
                     &nodeCopyAddr,
                     &strCopyAddr,
                     &vsaMap,
                     &strMap,
                     shmCtx->shmEnd);

   freeVsaMap(&vsaMap);
   freeStrMap(&strMap);

   if (strEnd == NULL)
   {
      /* not enough space to copy the strings */
      cmsLog_error("not enough space for strings");
      return 0;
   }

   /* round up strEnd to 4 byte boundary */
   strEnd = ROUNDUP_WORD(strEnd);

   consumed = ((uintptr_t) strEnd) - MDM_SHM_ATTACH_ADDR;

   cmsLog_debug("strEnd         = 0x%x (consumed=0x%x)", strEnd, consumed);
   cmsLog_debug("shmEnd         = 0x%x",         shmCtx->shmEnd);

   return consumed;
}


#define ALIGN_OFFSET(f, a) (((f) + (a)) / (a) * (a))

/** This is a recursive function which will be called for every MdmObjectNode in the
 *  data model.
 *
 *  For each node:
 *    1. update the oidNodePtrTable with shared memory addr of MdmObjectNode
 *    2. copy the MdmObjNode and all of its MdmParamNodes to shared memory
 *    3. copy over any valid strings array used by the MdmParamNode, taking care not
 *       to go over shmEnd.
 *
 * @return pointer to the next byte after the end of valid strings, or NULL if
 *         valid strings did not fit inside shmend.
 */
char *copyNode(MdmObjectNode *objNode,
               MdmObjectNode **oidNodePtrTable,
               char **nodeCopyAddr,
               char **strCopyAddr,
               struct vsa_entry **vsaMap,
               struct str_entry **strMap,
               const char *shmEnd)
{
   UINT32 i;
   SINT32 offsetInObject=0;
   MdmObjectNode *shmObjNode;
   char *strEnd;
   MdmParamNode *paramNode;
   MdmParamNode *shmParamNode;
   UBOOL8 isPrevParam64=FALSE;
   UBOOL8 isPrevParamBool=FALSE;
   SINT32 align64=8;
#ifdef DESKTOP_LINUX
#if !defined(__LP64__)
   /* the only time 64 bit types are 4 byte aligned is on Intel -m32 mode */
   align64 = 4;
#endif
#endif

   if (*nodeCopyAddr == (char *) mdmShmCtx->rootObjNode)
   {
      /* this is the root object node */
      /* nodeCopyAddr points to the address in shmem where the root node should go */
      shmObjNode = (MdmObjectNode *) (*nodeCopyAddr);

      /* copy the MdmObjectNode to shared mem */
      memcpy((void *) shmObjNode, (void *) objNode, sizeof(MdmObjectNode));

      /* copy the object name to shared mem */
      if ((shmObjNode->name = getSharedStr(strMap, objNode->name, strCopyAddr, shmEnd)) == NULL)
      {
         return NULL;
      }

      /* copy the object profile name to shared mem */
      if ((shmObjNode->profile = getSharedStr(strMap, objNode->profile, strCopyAddr, shmEnd)) == NULL)
      {
         return NULL;
      }

      /* update oidNodePtrTable with MdmObjectNode's shared mem addr */
      oidNodePtrTable[objNode->oid] = shmObjNode;

      /* advance nodeCopyAddr */
      (*nodeCopyAddr) += sizeof(MdmObjectNode);
   }
   else
   {
      /*
       * For all object nodes other than the root object node, the parent
       * has already copied all the children obj nodes over and updated
       * the oidNodePtrTable.  Just get the new shmObjNode pointer from
       * the oidNodePtrTable.
       */
      shmObjNode = oidNodePtrTable[objNode->oid];
   }

   /* the params of this objNode will start at nodeCopyAddr */
   shmObjNode->params = (objNode->numParamNodes > 0) ? 
                        ((MdmParamNode *) (*nodeCopyAddr)) : NULL;

   /* copy the param nodes, the param nodes will be contiguous */
   for (i=0; i < objNode->numParamNodes; i++)
   {
      paramNode = &(objNode->params[i]);
      shmParamNode = (MdmParamNode *) (*nodeCopyAddr);

      /* copy the MdmParamNode to shared mem */
      memcpy((void *) shmParamNode, (void *) paramNode, sizeof(MdmParamNode));

      /*
       * Fill in the offsetInObject field.  This calculation must be done
       * after the preprocessor has compiled out the undefined profiles.
       * Note that all MDM objects start with the following fields:
       * MdmObjectId (UINT16)
       * SequenceNum (UINT16)
       */
      if (oalMdm_isParam8(shmParamNode->type))
      {
         if (isPrevParamBool)
         {
            /* just another bool following a bool, advance 1 byte */
            offsetInObject += 1;
         }
         else if (isPrevParam64)
         {
             offsetInObject += 8;
         }
         else
         {
             /* prevParam must be int.  Advance 4 bytes. */
             offsetInObject += 4;
         }

         isPrevParamBool = TRUE;
         isPrevParam64 = FALSE;
      }
      else if (oalMdm_isParam64(shmParamNode->type))
      {
          if (isPrevParamBool)
          {
              offsetInObject = ALIGN_OFFSET(offsetInObject, align64);
          }
          else if (isPrevParam64)
          {
              offsetInObject += 8;
          }
          else
          {
              /* prevParam must be int.  Advance to next aligned boundary. */
              offsetInObject = ALIGN_OFFSET(offsetInObject, align64);
          }

          isPrevParamBool = FALSE;
          isPrevParam64 = TRUE;
      }
      else
      {
         /* current parameter must be a 32 bit type */
         if (isPrevParamBool)
         {
             /* align to the next 4 byte boundary */
             offsetInObject = ALIGN_OFFSET(offsetInObject, 4);
         }
         else if (isPrevParam64)
         {
             offsetInObject += 8;
         }
         else
         {
             /* prev param was also 32 bits, so just advance 4 */
             offsetInObject += 4;
         }

         isPrevParamBool = FALSE;
         isPrevParam64 = FALSE;
      }

      shmParamNode->offsetInObject = offsetInObject;


      /* copy name to shared mem */
      if ((shmParamNode->name = getSharedStr(strMap, paramNode->name, strCopyAddr, shmEnd)) == NULL)
      {
         return NULL;
      }
/*    cmsLog_debug("paramNode %p name=%s(%p)", shmParamNode, shmParamNode->name, shmParamNode->name); */

      /* copy profile name to shared mem */
      if ((shmParamNode->profile = getSharedStr(strMap, paramNode->profile, strCopyAddr, shmEnd)) == NULL)
      {
         return NULL;
      }

      /* copy default value, if there is one, to shared mem. */
      if (paramNode->defaultValue != NULL)
      {
         if ((shmParamNode->defaultValue = getSharedStr(strMap, paramNode->defaultValue, strCopyAddr, shmEnd)) == NULL)
         {
            return NULL;
         }
      }

      /* check for valid strings */
      if ((paramNode->type == MPT_STRING) && (paramNode->vData.min != NULL))
      {
         shmParamNode->vData.min = getSharedVsa(vsaMap,
                                                strMap,
                                                (const char *) paramNode->vData.min,
                                                strCopyAddr,
                                                shmEnd);

         if (shmParamNode->vData.min == NULL)
         {
            /* out of space */
            return NULL;
         }
      }

      (*nodeCopyAddr) += sizeof(MdmParamNode);
   }


   /*
    * all child MdmObjectNodes of the parent node must be contiguous, so
    * copy them over in a single block first.
    */
   shmObjNode->childObjNodes = (MdmObjectNode *) (*nodeCopyAddr);

   for (i=0; i < objNode->numChildObjNodes; i++)
   {
      oidNodePtrTable[objNode->childObjNodes[i].oid] = (MdmObjectNode *) (*nodeCopyAddr);
      memcpy((*nodeCopyAddr), &(objNode->childObjNodes[i]), sizeof(MdmObjectNode));

      /* copy the object name to shared mem */
      shmObjNode = (MdmObjectNode *) (*nodeCopyAddr);
      if ((shmObjNode->name = getSharedStr(strMap, objNode->childObjNodes[i].name, strCopyAddr, shmEnd)) == NULL)
      {
         return NULL;
      }

      /* copy the object profile name to shared mem */
      if ((shmObjNode->profile = getSharedStr(strMap, objNode->childObjNodes[i].profile, strCopyAddr, shmEnd)) == NULL)
      {
         return NULL;
      }

      (*nodeCopyAddr) += sizeof(MdmObjectNode);
   }


   /* recurse to child objects */
   for (i=0; i < objNode->numChildObjNodes; i++)
   {
      strEnd = copyNode(&(objNode->childObjNodes[i]),
                        oidNodePtrTable,
                        nodeCopyAddr,
                        strCopyAddr,
                        vsaMap,
                        strMap,
                        shmEnd);

      if (strEnd == NULL)
      {
         /* ran out of space */
         return NULL;
      }
   }

   return (*strCopyAddr);
}



/** Find a vsa_entry entry with the specified libAddr, if there is no such entry, allocate
 *  a new vsa_entry entry and link it into the list.
 * 
 * @param vsaMap (IN/OUT) pointer to head of list ov vsa_entries.
 * @param libAddr (IN)    address of valid strings array in the libmdm.so.
 * @return pointer to vsa_entry which could be an existing or new entry.  Or may return
 *         NULL if memory allocation failed.
 */
char *getSharedVsa(struct vsa_entry **vsaMap,
                   struct str_entry **strMap,
                   const char *libAddr,
                   char **strCopyAddr,
                   const char *shmEnd)
{
   struct vsa_entry *tmpEntry = (*vsaMap);


   while ((tmpEntry != NULL) && (tmpEntry->libAddr != libAddr))
   {
      tmpEntry = tmpEntry->next;
   }

   if (tmpEntry != NULL)
   {
      /* we must have found an entry, return it. */
      return tmpEntry->shmAddr;
   }


   /*
    * If we get here, then this is a valid strings array we have not seen before.
    * Allocate a new vsa_entry and link it into the list so we can re-use references
    * to the same valid strings array.  Then we need to copy over the array of 
    * pointers.  Then we need to copy over the strings.
    */
   tmpEntry = cmsMem_alloc(sizeof(struct vsa_entry), ALLOC_ZEROIZE);
   if (tmpEntry == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", sizeof(struct vsa_entry));
      return NULL;
   }
   
   tmpEntry->libAddr = libAddr;
   *strCopyAddr = ROUNDUP_WORD(*strCopyAddr); /* array of pointers must be on word boundary */
   tmpEntry->shmAddr = (*strCopyAddr);
   tmpEntry->next = (*vsaMap);
   (*vsaMap) = tmpEntry;

   if (copyVsa(strMap, (const char **) libAddr, strCopyAddr, shmEnd) == NULL)
   {
      return NULL;
   }

   return tmpEntry->shmAddr;
}


/** Copy a valid string array (array of pointers and strings) to shared memory addr.
 */
char *copyVsa(struct str_entry **strMap, const char **libArray, char **strCopyAddr, const char *shmEnd)
{
   UINT32 j=0;
   UINT32 numEntries=1; /* also count last NULL as an entry */
   char **shmArray;

   while (libArray[j] != NULL)
   {
      numEntries++;
      j++;
   }
   
   if ((*strCopyAddr) + (numEntries * sizeof(char *)) > shmEnd)
   {
      return NULL;
   }

   shmArray = (char **) (*strCopyAddr);
   memcpy(shmArray, libArray, numEntries * sizeof(char *));
   (*strCopyAddr) += numEntries * sizeof(char *);

/*  cmsLog_debug("vsa %p, numEntries=%d", shmArray, numEntries); */

   /* go through the vsArray again, this time copying the strings over */
   j=0;
   while (libArray[j] != NULL)
   {
      shmArray[j] = getSharedStr(strMap, libArray[j], strCopyAddr, shmEnd);
      if (shmArray[j] == NULL)
      {
         return NULL;
      }

      j++;
   }


   return (char *) shmArray;
}



/** Find a the specified string in the shared memory; if not in shared memory, copy
 *  the specified string to shared memory.
 * 
 * @param strMap (IN/OUT) pointer to head of list of str__entries.
 * @param libStr (IN)     pointer to string in the libmdm.so.
 * @return pointer to string in shared memory, or NULL if memory allocation failed
 *         or if there is not enough shared memory to hold the string.
 */
char *getSharedStr(struct str_entry **strMap, const char *libStr, char **strCopyAddr, const char *shmEnd)
{
   UINT32 len;
   struct str_entry *tmpEntry = (*strMap);

   while ((tmpEntry != NULL) && strcmp(tmpEntry->shmStr, libStr))
   {
      tmpEntry = tmpEntry->next;
   }

   if (tmpEntry != NULL)
   {
      /* we must have found an entry, return it. */
      return tmpEntry->shmStr;
   }


   /*
    * If we get here, we need to copy the string from libmdm.so to shared mem.
    * First create a str_entry so we can re-use references to the same string.
    */
   tmpEntry = cmsMem_alloc(sizeof(struct str_entry), ALLOC_ZEROIZE);
   if (tmpEntry == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", sizeof(struct str_entry));
      return NULL;
   }
   
   tmpEntry->next = (*strMap);
   (*strMap) = tmpEntry;


   len = strlen(libStr) + 1;
   if ((*strCopyAddr) + len > shmEnd)
   {
      /* not enough space to copy over string */
      return NULL;
   }


   tmpEntry->shmStr = (*strCopyAddr);
   memcpy(tmpEntry->shmStr, libStr, len);
   (*strCopyAddr) += len;

/* cmsLog_debug("new string (%p) %s", tmpEntry->shmStr, libStr); */

   return tmpEntry->shmStr;
}


/** Free all entries in the vsaMap list.
 */
void freeVsaMap(struct vsa_entry **vsaMap)
{
   struct vsa_entry *tmpEntry;

   while ((*vsaMap) != NULL)
   {
      tmpEntry = (*vsaMap)->next;
      (*vsaMap)->next = NULL;
      cmsMem_free(*vsaMap);
      (*vsaMap) = tmpEntry;
   }

   return;
}


/** Free all entries in the strMap list.
 */
void freeStrMap(struct str_entry **strMap)
{
   struct str_entry *tmpEntry;

   while ((*strMap) != NULL)
   {
      tmpEntry = (*strMap)->next;
      (*strMap)->next = NULL;
      cmsMem_free(*strMap);
      (*strMap) = tmpEntry;
   }

   return;
}


/*************************************************************************
 *
 * Everything below here deals with config files.  This should probably
 * get moved to its own file.
 *
 *************************************************************************/


/** Return the length field in the compressed config buf header.
 *
 * @param buf (IN) the compressed config header.
 *
 * @return the length field in the compressed config header.
 */
static UINT32 getCompressedConfigFileLength(const char *buf)
{
   char tmpbuf[COMPRESSED_CONFIG_HEADER_LENGTH];
   UINT32 headerLen=strlen(COMPRESSED_CONFIG_HEADER);
   UINT32 i, compressedLen;
   UBOOL8 found=FALSE;


   memcpy(tmpbuf, buf, COMPRESSED_CONFIG_HEADER_LENGTH);

   /* look for end marker and insert a null terminator */
   for (i=0; i < COMPRESSED_CONFIG_HEADER_LENGTH && !found; i++)
   {
      if (tmpbuf[i] == '>')
      {
         tmpbuf[i] = 0;
         found = TRUE;
      }
   }

   compressedLen = atoi(&(tmpbuf[headerLen]));
   cmsLog_debug("compressedLen=%d", compressedLen);

   /* sanity check, compressLen cannot be greater than real len */
   if (compressedLen > cmsImg_getRealConfigFlashSize())
   {
      cmsLog_error("invalid length %d in compressed config file header", compressedLen);
      return -1;
   }

   return compressedLen;
}


/** Return the crc field in the crc config buf header.
 *
 * @param buf (IN) the crc config header.
 *
 * @return the crc field in the crc config header.
 */
static UINT32 getConfigCrc(const char *buf)
{
   char tmpbuf[CRC_CONFIG_HEADER_LENGTH];
   UINT32 headerLen=strlen(CRC_CONFIG_HEADER);
   UINT32 i, crc=0;
   UBOOL8 found=FALSE;
   CmsRet ret;


   memcpy(tmpbuf, buf, CRC_CONFIG_HEADER_LENGTH);

   /* look for end marker and insert a null terminator */
   for (i=0; i < CRC_CONFIG_HEADER_LENGTH && !found; i++)
   {
      if (tmpbuf[i] == '>')
      {
         tmpbuf[i] = 0;
         found = TRUE;
      }
   }

   ret = cmsUtl_strtoul(&(tmpbuf[headerLen]), 0, 0, &crc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert crc header ret=%d, header=%s", ret, tmpbuf);
   }
   else
   {
      cmsLog_debug("crc in header 0x%x", crc);
   }

   return crc;
}


/** Get a bunch of information related to the config file headers in one call.
 *
 * Config files may start with a compression header and a CRC header,
 * or just a compression header, or just a CRC header, or no header.
 * Get information from those headers.
 *
 * @param buf (IN) Buffer read from the config flash.
 * @param len (IN) Length of buffer.
 * @param isCompressed  (OUT) TRUE if this is a compressed config file.
 * @param compressedLen (OUT) Length of compressed data from the header.
 * @param isCrc         (OUT) TRUE if there is a CRC header.
 * @param crc           (OUT) crc value from the header.
 *
 * @return number of bytes to start of real data, past all the headers.
 */
static UINT32 getConfigHeaderInfo(const char *buf, UINT32 len, UBOOL8 *isCompressed, UINT32 *compressedLen, UBOOL8 *isCrc, UINT32 *crc)
{
   cmsLog_debug("len=%d", len);

   *isCompressed = FALSE;
   *compressedLen = 0;
   *isCrc = FALSE;
   *crc = 0;

   if ((len > COMPRESSED_CONFIG_HEADER_LENGTH) &&
       (!cmsUtl_strncmp(buf, COMPRESSED_CONFIG_HEADER, strlen(COMPRESSED_CONFIG_HEADER))))
   {
      /* compressed header detected, get length of data */
      *isCompressed = TRUE;
      *compressedLen = getCompressedConfigFileLength(buf);
      cmsLog_debug("compressed header detected, len=%d", *compressedLen);

      if ((len > COMPRESSED_CONFIG_HEADER_LENGTH + CRC_CONFIG_HEADER_LENGTH) &&
          (!cmsUtl_strncmp(&(buf[COMPRESSED_CONFIG_HEADER_LENGTH]), CRC_CONFIG_HEADER, strlen(CRC_CONFIG_HEADER))))
      {
         /* both headers present */
         *isCrc = TRUE;
         *crc = getConfigCrc(&(buf[COMPRESSED_CONFIG_HEADER_LENGTH]));
         cmsLog_debug("crc header detected (after compression header), crc=0x%x", *crc);

         return COMPRESSED_CONFIG_HEADER_LENGTH + CRC_CONFIG_HEADER_LENGTH;
      }
      else
      {
         /* only compression header present */
         return COMPRESSED_CONFIG_HEADER_LENGTH;
      }
   }
   else if ((len > CRC_CONFIG_HEADER_LENGTH) &&
            (!cmsUtl_strncmp(buf, CRC_CONFIG_HEADER, strlen(CRC_CONFIG_HEADER))))
   {
      /* only CRC header present */
      *isCrc = TRUE;
      *crc = getConfigCrc(buf);
      cmsLog_debug("crc header detected, crc=0x%x", *crc);

      return CRC_CONFIG_HEADER_LENGTH;
   }

   return 0;
}


CmsRet oal_readConfigFlashToBuf(const char *selector, char *buf, UINT32 *len)
{
   CmsRet ret=CMSRET_SUCCESS;
   char *flashBuf;
   UINT32 flashLen;
   UINT32 offset;
   UINT32 ioctlCode;
   UBOOL8 isCompressed=FALSE;
   UBOOL8 isCrc=FALSE;
   UINT32 crc;
   UINT32 cdataLen;


   if (!cmsUtl_strcmp(selector, CMS_CONFIG_PRIMARY))
   {
      ioctlCode = PERSISTENT;
   }
   else if (!cmsUtl_strcmp(selector, CMS_CONFIG_BACKUP))
   {
      ioctlCode = BACKUP_PSI;
   }
   else
   {
      cmsLog_error("unregnized selector %s", selector);
      *len = 0;
      return CMSRET_INTERNAL_ERROR;
   }


   /* always read the contents of the flash into our own buffer first */
   flashLen = cmsImg_getRealConfigFlashSize();
   if ((flashBuf = cmsMem_alloc(flashLen, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("could not allocate %d bytes for compressed buffer", flashLen);
      return CMSRET_RESOURCE_EXCEEDED;
   }


   ret = devCtl_boardIoctl(BOARD_IOCTL_FLASH_READ, ioctlCode, flashBuf, flashLen, 0, NULL);
   if (ret != CMSRET_SUCCESS)
   {
      *len = 0;
      cmsMem_free(flashBuf);
      return ret;
   }

   cmsLog_debug("read %d bytes from flash %s", flashLen, selector);

   {
      char z = (char) 0xff;

      /* 
       * When the config flash is erased with the cfe "i" command, we
       * get all 0xff.  Look for that pattern and set valid length to 0.
       */
      if (flashBuf[0] == z && flashBuf[1] == z && flashBuf[2] == z && flashBuf[3] == z &&
          flashBuf[4] == z && flashBuf[5] == z && flashBuf[6] == z && flashBuf[7] == z)
      {
         cmsLog_debug("looks like freshly initialized config area, return valid length=0");
            
         *len = 0;
         cmsMem_free(flashBuf);
         return ret;
      }
   }


   /*
    * Now check CRC.  If the CRC check fails, we do not return the buffer
    * to the user.
    */
   offset = getConfigHeaderInfo(flashBuf, flashLen, &isCompressed, &cdataLen, &isCrc, &crc);

#ifndef COMPRESSED_CONFIG_FILE
   if (isCompressed)
   {
      cmsLog_error("compressed config file detected, but this image does not support compressed config file.");
      *len = 0;
      cmsMem_free(flashBuf);
      return CMSRET_INVALID_CONFIG_FILE;
   }
#endif

   if (isCrc)
   {
      UINT32 calculatedCrc;
      UINT32 crcLen;

#ifdef COMPRESSED_CONFIG_FILE
      crcLen = cdataLen;
#else
      {
         /*
          * If there is no compression header, I need to figure out the actual
          * length of the data to do the CRC over.
          */
         for (crcLen=offset; (crcLen < flashLen) && (flashBuf[crcLen] != 0); crcLen++);

         /* include the first 0 byte by incrementing crcLen by 1 */
         crcLen++;

         /* subtract off the header offset to get true crcLen */
         if (crcLen >= offset)
         {
            crcLen -= offset;
         }
      }
#endif

      calculatedCrc = genUtl_getCrc32((UINT8 *) &(flashBuf[offset]), crcLen, CRC_INITIAL_VALUE);

      cmsLog_debug("calculated=0x%x headerCrc=0x%x", calculatedCrc, crc);
      if (calculatedCrc != crc)
      {
         cmsLog_error("crc check for config %s failed, calculated=0x%x headerCrc=0x%x",
                      selector, calculatedCrc, crc);
         *len = 0;
         cmsMem_free(flashBuf);
         return CMSRET_INVALID_CONFIG_FILE;
      }
   }


#ifdef COMPRESSED_CONFIG_FILE
   if (isCompressed)
   {
      SINT32 dlen;
      LZWDecoderState *decoder=NULL;
      CmsRet r2;

      cmsLog_debug("offset to config data is %d", offset);

      r2 = cmsLzw_initDecoder(&decoder, (UINT8 *) &(flashBuf[offset]), cdataLen);
      if (r2 != CMSRET_SUCCESS)
      {
         cmsMem_free(flashBuf);
         *len = 0;
         return r2;
      }

      /* decode into the user's buffer */
      dlen = (UINT32) cmsLzw_decode(decoder, (UINT8 *) buf, *len);
      cmsLog_debug("decode returned %d", dlen);

      cmsLzw_cleanupDecoder(&decoder);

      if (dlen < 0)
      {
         *len = 0;
         cmsMem_free(flashBuf);
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         *len = (UINT32) dlen;
         CMSMEM_FREE_BUF_AND_NULL_PTR(flashBuf);
         ret = CMSRET_SUCCESS;
      }
   }
   else
#endif
   {
      /*
       * This is not a compressed config file.  Copy the contents
       * of the flash buffer into the user's buffer.
       */

      UINT32 smaller = (flashLen < *len) ? flashLen : *len;
      UINT32 i=0;

      memcpy(buf, &(flashBuf[offset]), smaller);
      CMSMEM_FREE_BUF_AND_NULL_PTR(flashBuf);

      /*
       * Look for the last 0 byte to determine the true length of the config file.
       */
      while (i < smaller)
      {
         if (buf[i] == 0)
         {
            *len = (i == 0) ? 0 : i+1;
            break;
         }
         i++;
      }
   }

   cmsLog_debug("returning %s ret=%d len=%d", selector, ret, *len);

   return ret;
}


CmsRet oal_readConfigFileToBuf(const char *filename, char *buf, UINT32 *len)
{
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("Reading config file from %s", filename);


   if (cmsFil_isFilePresent(filename))
   {
      ret = cmsFil_copyToBuffer(filename, (UINT8 *)buf, len);
      cmsLog_debug("read file %s, len=%d ret=%d", filename, *len, ret);
   }
   else
   {
      cmsLog_debug("file %s not found", filename);
      /* Failed to read the config file and make len  0 to indicate that */
      *len = 0;
   }

   return ret;
}


void oal_invalidateConfigFlash(const char *selector)
{
   CmsRet ret = CMSRET_INTERNAL_ERROR;
   UINT32 ioctlCode;
   char *flashBuf;
   UINT32 flashLen;
   char *flashname;
   FILE *fp;
   unsigned int flags;

   if (!cmsUtl_strcmp(selector, CMS_CONFIG_PRIMARY))
   {
      ioctlCode = PERSISTENT;
      flashname = PSI_FILE_NAME;
   }
   else if (!cmsUtl_strcmp(selector, CMS_CONFIG_BACKUP))
   {
      ioctlCode = BACKUP_PSI;
      flashname = PSI_BACKUP_FILE_NAME;
   }
   else
   {
      cmsLog_error("unrecognized selector %s", selector);
      return;
   }

   /* Allocate a buffer that is the size of the configuration. */
   flashLen = cmsImg_getRealConfigFlashSize();
   if ((flashBuf = cmsMem_alloc(flashLen, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("could not allocate %d bytes for compressed buffer", flashLen);
      return;
   }

   /* Zero the buffer. */
   memset(flashBuf, 0x00, flashLen);

   /* Overwrite the configuration with zeros. */
#ifndef DISABLE_NOR_RAW_PARTITION
   getFlashInfo(&flags);
   if (FLASH_INFO_FLAG_NOR == flags) {
      ret = devCtl_boardIoctl(BOARD_IOCTL_FLASH_WRITE, ioctlCode, flashBuf, flashLen, 0, NULL);
   } else 
#endif
   {
      fp = fopen(flashname,"w");
      if (NULL != fp) {
         if ((0 != fwrite(flashBuf, 1, flashLen, fp)) && (EOF != fclose(fp))) {
            ret = CMSRET_SUCCESS;
         }
      }
   }
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("configuration is not invalidated, oal_invalidateConfigFlash failed, ret=%d", ret);
   }
   sync();

   cmsMem_free(flashBuf);
}


void oal_flushFsToMedia( void )
{
   sync();
}


/** Initialize data model selection stuff.
 *
 * @return -1 on error, 1 if we should load Device2 (Pure181) data model,
 *         0 otherwise (meaning IGD data model, which includes legacy98 and Hybrid)
 */
SINT32 initDataModelSelection()
{
   /*
    * Legacy98, Hybrid, and Pure181 are easy.  No need to create or look at
    * a PSP entry.  The answer is known at compile time.
    */
#if defined(SUPPORT_DM_LEGACY98)
   return 0;
#elif defined(SUPPORT_DM_HYBRID)
   return 0;
#elif defined(SUPPORT_DM_PURE181)
   return 1;
#elif defined(SUPPORT_DM_DETECT)
   SINT32 rv;
   UINT8 dmc[CMS_DATA_MODEL_PSP_VALUE_LEN]={0};

   rv = cmsPsp_get(CMS_DATA_MODEL_PSP_KEY, dmc, sizeof(dmc));
   if (rv != CMS_DATA_MODEL_PSP_VALUE_LEN)
   {
      /* No PSP file, create one with default entry of 1 (meaning Pure181) */
      /* Write PSP entry of 4 bytes.  Only use the first byte right now,
       * the other 3 are reserved */
      dmc[0] = 1;
      cmsLog_debug("create new CMS Data Model PSP key with value of %d", dmc[0]);
      if (cmsPsp_set(CMS_DATA_MODEL_PSP_KEY, dmc, sizeof(dmc)) != CMSRET_SUCCESS)
      {
         cmsLog_error("initial set of CMS Data Model PSP key failed");
         /* not a fatal error, just return 1 so we can keep running */
         return 1;
      }

      return 1;
   }
   else
   {
      cmsLog_debug("found existing CMS Data Model PSP key=%d", dmc[0]);
      if (dmc[0] == 0)
      {
         return 0;
      }
      else if (dmc[0] == 1)
      {
         return 1;
      }
      else
      {
         cmsLog_error("Unexpected value in CMS Data Model PSP key=%d", dmc[0]);
         /* not a fatal error, just return 1 so we can keep running */
         return 1;
      }
   }
#endif
}

UBOOL8 oalMdm_isParam8(MdmParamTypes type)
{
   /* there is only one 8-bit type */
   return (type == MPT_BOOLEAN);
}

UBOOL8 oalMdm_isParam64(MdmParamTypes type)
{
   UBOOL8 ret=0;

   switch(type)
   {
      case MPT_LONG64:
      case MPT_UNSIGNED_LONG64:
#ifdef __LP64__
      case MPT_STRING:
      case MPT_DATE_TIME:
      case MPT_BASE64:
      case MPT_HEX_BINARY:
      case MPT_UUID:
      case MPT_IP_ADDR:
      case MPT_MAC_ADDR:
#endif
         {
            ret=1;
            break;
         }
      default:
         break;
   }
   return ret;
}
