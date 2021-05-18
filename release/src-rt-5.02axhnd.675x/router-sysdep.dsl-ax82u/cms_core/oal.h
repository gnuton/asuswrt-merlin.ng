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


/*!\file oal.h
 * \brief This header file contains all the functions exported by the
 *        OS Adaptation Layer (OAL).
 *
 * The OAL functions live under the
 * directory with the name of the OS, e.g. linux, ecos.
 * The Make system will automatically compile the appropriate files
 * and link them in with the final executable based on the TARGET_OS
 * variable, which is set by make menuconfig.
 */

#ifndef __OAL_H__
#define __OAL_H__

#include "cms.h"
#include "mdm.h"
#include "mdm_types.h"
#include "cms_boardcmds.h"


/** creates, attaches, copies and does everything needed to make the
 * shared memory ready for use by the MDM.
 * 
 * If CMS_SHARED_MEM is not defined, this function will do nothing.
 * A shared memory region is created only if shmid is -1.  The newly
 * created shmid will be returned in shmId.
 * If shmid is not -1, this function will attach to the specified shmid.
 *
 * @param shmSize (IN)   Size of the shared memory region to create.
 * @param shmId (IN/OUT) Shared memory id.
 * @param shmAddr (OUT)  Address of the attached shared memory region.
 * @return CmsRet enum.
 */ 
CmsRet oalShm_init(UINT32 shmSize, SINT32 *shmId, void **shmAddr);


/** Detaches, frees memory in, and destroys the shared memory region.
 * 
 * If CMS_SHARED_MEM is not defined, this function will do nothing.
 * If this caller is the last process to detach from the shared memory
 * region, it wil also free all the memory in the shared memory region
 * and destory the shared memory region.
 *
 * @param shmId   (IN) Shared memory id to cleanup.
 * @param shmAddr (IN) Address where shared memory is attached.
 */ 
void oalShm_cleanup(SINT32 shmId, void *shmAddr);



/** Creates and initializes OS dependent lock.
 *
 * This function should only be called once per system startup.
 * This is probably done at the same time as oalShm_init().
 * On Linux, this lock is implemented by a semaphore.
 *
 * @param attachExisting (IN) If true, get existing semaphore, else create new one.
 *
 * @return CmsRet enum.
 */
CmsRet oalLck_init(UBOOL8 attachExisting);


/** Free the OS dependent lock.
 *
 * This function should only be called when the last user of the MDM
 * exits, probably at the same time as oalShm_cleanup().
 *
 */
void oalLck_cleanup(void);

/** Acquire the specified zone lock. */
CmsRet oal_lock_zone(UINT8 zone, UBOOL8 useTimeout, UINT32 timeoutMs);

/** Release the specified zone lock. */
void oal_unlock_zone(UINT8 zone);

/** Reset zone lock state to 0.
 * @return 0 on success, -1 on error. */
SINT32 oal_reset_zone(UINT8 zone);


/** Acquire the lock meta info lock.  Always block forever waiting for this
 *  lock.  This lock is used to protect access to lock meta info in MdmShmCtx.
 *  Code which runs while holding this lock is never blocking, so every
 *  thread should be able to get it quickly.
 */
CmsRet oal_lock_meta();

/** Release the lock meta info lock. */
void oal_unlock_meta();



/** Primary (single) config file identifier.
 *
 * Used in oal_readConfigFlashToBuf().
 * 
 */
#define CMS_CONFIG_PRIMARY  "__primaryPsi"


/** optional backup config file identifier.
 *
 * Used in oal_readConfigFlashToBuf().
 * Space for the backup config file must be allocated in the CFE and this
 * feature must be enabled via make menuconfig.
 */
#define CMS_CONFIG_BACKUP  "__backupPsi"


/** Read the config file in the config area of the flash and put it in the
 *  buffer.
 *
 * @param selector (IN) Which config buffer to read, CMS_CONFIG_PRIMARY or
 *                      CMS_CONFIG_BACKUP.
 * @param buf (OUT)    Caller provided buffer for holding the config, and this
 *                     function will read the config file from the 
 *                     persistent storage into this buffer.
 * @param len (IN/OUT) On entry, len contains the length of the buffer.
 *                     On successful exit, len contains the number of bytes
 *                     actually read.
 *
 * @return CmsRet enum.  If the config area of the flash is empty, this
 *                       function will return CMSRET_SUCCESS with len=0;
 */
CmsRet oal_readConfigFlashToBuf(const char *selector, char *buf, UINT32 *len);


/** Read the config file from the specified file and put it in the buffer.
 *
 * This function is used to read the optional default configuration file
 * that may be burned into the image.
 *
 * @param filename (IN) File to read from
 * @param buf (OUT)    Caller provided buffer for holding the config, and this
 *                     function will read the config file from the 
 *                     specified file into this buffer.
 * @param len (IN/OUT) On entry, len contains the length of the buffer.
 *                     On successful exit, len contains the number of bytes
 *                     actually read.
 *
 * @return CmsRet enum
 */
CmsRet oal_readConfigFileToBuf(const char *filename, char *buf, UINT32 *len);


/** Write 256 bytes of zero's at the beginning of the specified config area.
 *
 * This will cause the config area to appear invalid.
 *
 * @param selector (IN) Which config buffer to read, CMS_CONFIG_PRIMARY or
 *                      CMS_CONFIG_BACKUP.
 */
void oal_invalidateConfigFlash(const char *selector);


/** write buffered data in memory out to media **/
void oal_flushFsToMedia( void );


/** Return TRUE if type is a 64 bit type.  Note some types may be 32 or 64
 *  bits depending on the compile mode.
 */
UBOOL8 oalMdm_isParam64(MdmParamTypes type);

/** Return TRUE if type is a 8 bit (boolean) type. */
UBOOL8 oalMdm_isParam8(MdmParamTypes type);


#endif /* __OAL_H__ */
