/***********************************************************************
 * <:copyright-BRCM:2006:proprietary:standard
 * 
 *    Copyright (c) 2006 Broadcom 
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
 ************************************************************************/

#include "cms.h"
#include "cms_util.h"
#include "cms_mgm.h"
#include "cms_msg.h"
#include "mdm.h"
#include "cms_boardioctl.h"
#include "cms_params_modsw.h"
#include "prctl.h"
#include "genutil_crc.h"
#ifdef SUPPORT_BEEP
#include "beep_common.h"
#endif

static CmsRet writeValidatedBufToConfigFlash(const char *buf, UINT32 len);


/** Try really hard to get all locks.  Wait a long time if neccessary. */
#define MGM_LOCK_TIMEOUT (CMSLCK_MAX_HOLDTIME * 8)

#ifdef COMPRESSED_CONFIG_FILE

/** Compress the given buffer and add a compression header.
 *
 * @param buf (IN)     Buffer to be compressed.
 * @param len (IN/OUT) On entry into the function, len is the length of the buffer to 
 *                     be compressed.  On successful exit, len is the length of the
 *                     compressed buffer that is returned.
 *
 * @return the compressed buffer.  Caller is responsible for freeing this buffer.
 */
static UINT8 *compressBuf(const char *buf, UINT32 *len)
{
   UINT32 uncompressedLen = *len;
   UINT32 outbufLen=(uncompressedLen * 3) / 2 + (COMPRESSED_CONFIG_HEADER_LENGTH);
   UINT8 *outbuf;
   LZWEncoderState *encoder=NULL;
   SINT32 rc;
   CmsRet ret;

   outbuf = cmsMem_alloc(outbufLen, ALLOC_ZEROIZE);
   if (outbuf == NULL)
   {
      cmsLog_error("could not allocate %d bytes for compressed buf", outbufLen);
      *len = 0;
      return NULL;
   }

   if ((ret = cmsLzw_initEncoder(&encoder,
                                 &(outbuf[COMPRESSED_CONFIG_HEADER_LENGTH]),
                                 outbufLen-COMPRESSED_CONFIG_HEADER_LENGTH)) != CMSRET_SUCCESS)
   {
      cmsLog_error("initEncoder failed, ret=%d", ret);
      cmsMem_free(outbuf);
      *len = 0;
      return NULL;
   }

   if ((rc = cmsLzw_encode(encoder, (UINT8 *)buf, *len)) < 0)
   {
      cmsLog_error("encode failed");
      cmsMem_free(outbuf);
      *len = 0;
      cmsLzw_cleanupEncoder(&encoder);
      return NULL;
   }
   else
   {
      *len = rc;
   }

   if ((rc = cmsLzw_flushEncoder(encoder)) < 0)
   {
      cmsLog_error("encode flush failed");
      cmsMem_free(outbuf);
      *len = 0;
      cmsLzw_cleanupEncoder(&encoder);
      return NULL;
   }
   else
   {
      *len += rc;
   }

   cmsLzw_cleanupEncoder(&encoder);

   cmsLog_notice("compressed len=%d uncompressed len=%d\n", *len, uncompressedLen);

   /*
    * now add a header to the config file so we know it is
    * compressed and how many compressed bytes there are.
    */
   sprintf((char *)outbuf, "%s%d>", COMPRESSED_CONFIG_HEADER, *len);

   *len += COMPRESSED_CONFIG_HEADER_LENGTH;

   return outbuf;
}

#endif /* COMPRESSED_CONFIG_FILE */


/** Add a CRC header to the config buffer, which may already contain a compression header.
 *
 * Do not calculate the CRC on the compression header.
 *
 * @param buf (IN)     Buffer containing the config file, but may contain the compress
 *                     header.  Do not calculate crc over the compression header.
 * @param len (IN/OUT) On entry into the function, len is the length of the entire buffer.
 *                     On successful exit, len is the length of the new buffer with
 *                     crc header.
 *
 * @return the compressed buffer.  Caller is responsible for freeing this buffer.
 */
static char *addCrcHeader(const char *buf, UINT32 *len)
{
   UINT32 crc;
   UINT32 newLen = *len + CRC_CONFIG_HEADER_LENGTH;
   char *newBuf;

   if ((newBuf = cmsMem_alloc(newLen, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("could not allocate %d bytes for crc header buf", newLen);
      *len = 0;
      return NULL;
   }

   if (!cmsUtl_strncmp(buf, COMPRESSED_CONFIG_HEADER, strlen(COMPRESSED_CONFIG_HEADER)))
   {
      /* there is a compression header already. */

      /* copy over the compression header */
      memcpy(newBuf, buf, COMPRESSED_CONFIG_HEADER_LENGTH);


      /* calculate crc and insert crc header */
      crc = genUtl_getCrc32((UINT8 *) &(buf[COMPRESSED_CONFIG_HEADER_LENGTH]),
                            *len - COMPRESSED_CONFIG_HEADER_LENGTH,
                            CRC_INITIAL_VALUE);


      sprintf(&(newBuf[COMPRESSED_CONFIG_HEADER_LENGTH]), "%s0x%x>", CRC_CONFIG_HEADER, crc);

      /* copy over the compressed buffer */
      memcpy(&(newBuf[COMPRESSED_CONFIG_HEADER_LENGTH + CRC_CONFIG_HEADER_LENGTH]),
             &(buf[COMPRESSED_CONFIG_HEADER_LENGTH]),
             *len - COMPRESSED_CONFIG_HEADER_LENGTH);
   }
   else
   {
      /* there is no compression header */
      /* calculate crc and insert crc header */
      crc = genUtl_getCrc32((UINT8 *) buf, *len, CRC_INITIAL_VALUE);

      sprintf(newBuf, "%s0x%x>", CRC_CONFIG_HEADER, crc);

      /* copy over the real config buffer */
      memcpy(&(newBuf[CRC_CONFIG_HEADER_LENGTH]), buf, *len);
   }

   *len += CRC_CONFIG_HEADER_LENGTH;

   return newBuf;
}


CmsRet cmsMgm_saveConfigToFlash(void)
{
   CmsRet ret;
   char *buf;
   UINT32 len;

   if ((len = cmsImg_getConfigFlashSize()) == 0)
   {
      return CMSRET_INTERNAL_ERROR;
   }

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((buf = cmsMem_alloc(len, 0)) == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", len);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(buf);
      return ret;
   }

   if ((ret = mdm_serializeToBuf(buf, &len)) == CMSRET_SUCCESS)
   {
      /*
       * The buffer must contain a valid config because it came straight from the MDM,
       * so just write it out.
       */
      cmsLog_debug("writing serialized buf (len=0x%x) to config flash", len);
      ret = writeValidatedBufToConfigFlash(buf, len);      
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(buf);
   
   lck_autoUnlockAllZones(__FUNCTION__);

   return ret;
}


CmsRet cmsMgm_writeConfigToBuf(char *buf, UINT32 *len)
{
   CmsRet ret;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   ret = mdm_serializeToBuf(buf, len);

   lck_autoUnlockAllZones(__FUNCTION__);
   
   return ret;
}


CmsRet cmsMgm_writeMdmToBuf(char *buf, UINT32 *len)
{
   CmsRet ret;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   mdmLibCtx.dumpAll = TRUE;

   ret = mdm_serializeToBuf(buf, len);

   mdmLibCtx.dumpAll = FALSE;

   lck_autoUnlockAllZones(__FUNCTION__);
   
   return ret;
}


CmsRet cmsMgm_writeObjectToBuf(const MdmObjectId oid, char *buf, UINT32 *len)
{
   CmsRet ret;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((ret = lck_autoLockZone(oid, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   mdmLibCtx.dumpAll = TRUE;

   ret = mdm_serializeObjectToBuf(oid, buf, len);

   mdmLibCtx.dumpAll = FALSE;

   lck_autoUnlockZone(oid, __FUNCTION__);

   return ret;
}

#ifdef SUPPORT_BEEP
static void umountBeepDUStorage(void)
{
   FILE *fd = fopen("/proc/mounts", "r");
   char line[2000];
   char dev[BUFLEN_128], mountPoint[BUFLEN_512], fsType[BUFLEN_32];
   int  found = 0;

   if (fd == NULL)
   {
      cmsLog_error("Failed to Open /proc/mounts !!");
      return;
   }

   /* find mount point "/local" */
   while (fgets(line, 2000, fd))
   {
      if (sscanf(line, "%s %s %s", dev, mountPoint, fsType) == 3)
      {
         if (!strcmp(CMS_DATA_STORAGE_DIR, mountPoint))
         {
            found = 1;
            break;
         } 
      }
   }
   fclose(fd);

   if (!found)
   {
      cmsLog_error("Can't find mount point \"%s\" in /proc/mounts !!", CMS_DATA_STORAGE_DIR);
      return;
   }

   /* umount all under /local */
   sprintf(line, "umount -l %s", CMS_DATA_STORAGE_DIR);
   prctl_runCommandInShellBlocking(line);
   /* mount back /local for later */ 
   sprintf(line, "mount -t %s %s %s", fsType, dev, CMS_DATA_STORAGE_DIR);
   prctl_runCommandInShellBlocking(line);
}
#endif

void cmsMgm_invalidateConfigFlash(void)
{
   CmsRet ret;

   CHECK_MDM_EXTERNAL_CALLER_V(__FUNCTION__);

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return;
   }
#ifdef SUPPORT_WIRELESS
   unlink("/data/.kernel_nvram.setting");
#endif
#ifdef SUPPORT_BEEP
   unlink(BEEP_DB_FILE);
#endif

   cmsLog_notice("invalidating primary config flash");
   oal_invalidateConfigFlash(CMS_CONFIG_PRIMARY);

#ifdef SUPPORT_BACKUP_PSI
#ifdef SUPPORT_BACKUP_PSI_MIRROR_MODE
   /*
    * Only invalidate the backup if we are in Mirror Mode.  Otherwise,
    * the backup psi contains the per-device defaults which we want to
    * preserve.  (SUPPORT_BACKUP_PSI_DEVICE_DEFAULT)
    */
   cmsLog_notice("invalidating backup config flash");
   oal_invalidateConfigFlash(CMS_CONFIG_BACKUP);
#endif
#endif

#ifdef DMP_DEVICE2_HOMEPLUG_1
    cmsImg_setPLCconfigState("0");
#endif

#ifdef SUPPORT_BEEP
   {
      char cmd[BUFLEN_128];

      /* umount DU storage */
      umountBeepDUStorage();
      /* delete all busgate config files and log files */
      sprintf(cmd, "rm -rf %s/dbus-1 %s/busgate %s %s",
              CMS_DATA_STORAGE_DIR, CMS_DATA_STORAGE_DIR,
              CMS_MODSW_TMP_DIR, CMS_MODSW_DU_DIR);
      prctl_runCommandInShellBlocking(cmd);

      /* delete openwrt/merged_rootfs/share */
      sprintf(cmd, "rm -rf %s/openwrt/merged_rootfs/share", CMS_DATA_STORAGE_DIR);
      prctl_runCommandInShellBlocking(cmd);

      /*
       * CMS_DATA_STORAGE_DIR/openwrt/merged_rootfs is already unmounted 
       * in umountBeepDUStorage
       */

      /* delete all openwrt files */
      sprintf(cmd, "rm -rf %s/openwrt", CMS_DATA_STORAGE_DIR);
      prctl_runCommandInShellBlocking(cmd);
   }
#endif

   lck_autoUnlockAllZones(__FUNCTION__);
   
   return;
}


CmsRet cmsMgm_readConfigFlashToBuf(char *buf, UINT32 *len)
{
   UINT32 localLen = *len;
   CmsRet ret;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   ret = oal_readConfigFlashToBuf(CMS_CONFIG_PRIMARY, buf, &localLen);
   cmsLog_debug("oal_readConfigFlashToBuf primary returned %d len=0x%x", ret, localLen);

#ifdef SUPPORT_BACKUP_PSI
   if ((ret != CMSRET_SUCCESS) || (localLen == 0))
   {
      localLen = *len;
      ret = oal_readConfigFlashToBuf(CMS_CONFIG_BACKUP, buf, &localLen);
      cmsLog_debug("oal_readConfigFlashToBuf backup returned %d len=0x%x", ret, localLen);
   }
#endif

   *len = localLen;

   lck_autoUnlockAllZones(__FUNCTION__);

   return ret;
}


CmsRet writeValidatedBufToConfigFlash(const char *buf, UINT32 len)
{
   char *buf2=NULL;
   char *configBuf=NULL;
   char *crcBuf=NULL;
   UINT32 realConfigBufSize;
   CmsMsgHeader msg = EMPTY_MSG_HEADER;
   CmsRet ret;
   
   /*
    * In the flash, the config file must end with a 0 byte.
    * When I send out the config file, I always include a 0 byte, but when the
    * config file comes back into CMS, it might have gotten the 0 byte
    * stripped off.  So check for that, and append a 0 byte if neccessary.
    */
   if (buf[len-1] != 0)
   {
      cmsLog_debug("final zero byte not detected, adding it.");
      if ((buf2 = cmsMem_alloc(len+1, ALLOC_ZEROIZE)) == NULL)
      {
         cmsLog_error("realloc of %u bytes for config buf failed", len);
         return CMSRET_RESOURCE_EXCEEDED;
      }
      memcpy(buf2, buf, len);
      len++;
      configBuf = buf2;
   }
   else
   {
      cmsLog_debug("final zero byte detected, do nothing.");
      configBuf = (char *) buf;
   }

#ifdef COMPRESSED_CONFIG_FILE
   configBuf = (char *) compressBuf(configBuf, &len);
#endif

   crcBuf = addCrcHeader(configBuf, &len);

   /*
    * Make sure the (compressed) buffer can fit into the flash.
    */
   realConfigBufSize = cmsImg_getRealConfigFlashSize();
   if (len < realConfigBufSize)
   {
      ret = devCtl_boardIoctl(BOARD_IOCTL_FLASH_WRITE, PERSISTENT, crcBuf, len, 0, NULL);
      cmsLog_notice("wrote config to primary config flash, ret=%d", ret);

#ifdef SUPPORT_BACKUP_PSI
#ifdef SUPPORT_BACKUP_PSI_MIRROR_MODE
      /* MIRROR MODE: write the same thing to both config flash areas */
      if (cmsImg_isBackupConfigFlashAvailable())
      {
         CmsRet r2;
         r2 = devCtl_boardIoctl(BOARD_IOCTL_FLASH_WRITE, BACKUP_PSI, crcBuf, len, 0, NULL);
         cmsLog_notice("wrote config to backup config flash, ret=%d", r2);

         /*
          * not sure how to handle failures to write to the backup psi, just log it for now.
          * If write to primary succeeded and write to backup failed, that is less
          * serious than if both writes failed.
          */
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("write to backup psi failed, r2=%d", r2);
         }
      }
#endif
#endif

      /* flush to the file system*/
      oal_flushFsToMedia();
   }
   else
   {
      cmsLog_error("cannot fit config buf into flash, max=%d got=%d", realConfigBufSize, len);
      ret = CMSRET_RESOURCE_EXCEEDED;
   }



   CMSMEM_FREE_BUF_AND_NULL_PTR(buf2);
   CMSMEM_FREE_BUF_AND_NULL_PTR(crcBuf);

#ifdef COMPRESSED_CONFIG_FILE
   CMSMEM_FREE_BUF_AND_NULL_PTR(configBuf);
#endif


   if ((ret == CMSRET_SUCCESS) && (mdmLibCtx.msgHandle != NULL))
   {
      /*
       * Send out an event msg for any interested app.
       * The event message only goes out when a manangement app saves
       * the current MDM to config, but not when an app uploads a config file
       * via the network.  In the latter scenario, the app sends the whole
       * config file to smd, which validates and writes the config file to flash.
       * But the problem is, smd does not set a msgHandle in mdmLibCtx.msgHandle.
       * So it cannot send a message.  This is probably OK, beacuse right
       * after we upload a config file and write it to flash, we reboot the modem.
       */
      msg.src = mdmLibCtx.eid;
      msg.dst = EID_SMD;
      msg.type = CMS_MSG_CONFIG_WRITTEN;
      msg.flags_event = 1;
      msg.wordData = mdmLibCtx.eid;  /* this is who wrote out the config file */
      cmsMsg_send(mdmLibCtx.msgHandle, &msg);
   }

   return ret;
}


#ifdef SUPPORT_BEEP

#define START_TAG_SW_MODULES    "    <SoftwareModules>"
#define END_TAG_SW_MODULES      "    </SoftwareModules>"

#ifdef SUPPORT_DM_PURE181
#define END_TAG_ROOT_MDM        "  </Device>"
#else
#define END_TAG_ROOT_MDM        "  </InternetGatewayDevice>"
#endif

/* replace <SoftwareModules>...</SoftwareModules> section in cfgIn
 * with <SoftwareModules>..</SoftwareModules> section in cfgFlash
 * and write to cfgOut with this section at the end of cfgOut */
static CmsRet writeBeepValidatedBufToConfigFlash(const char *cfgIn,
                                                 UINT32 lenCfgIn)
{
   CmsRet ret = CMSRET_SUCCESS;
   char *cfgFlash = NULL, *cfgOut = NULL;
   char *startSmCfgIn = NULL, *endSmCfgIn = NULL, *endCfgIn = NULL;
   char *startSmCfgFlash = NULL, *endSmCfgFlash = NULL;
   UINT32 len = 0, lenEndSm = 0, lenCfgFlash = 0;
   UINT32 lenSmCfgFlash = 0, lenSmCfgIn = 0;
   UINT32 lenCfgOut = 0;

   if ((lenCfgFlash = cmsImg_getConfigFlashSize()) == 0)
   {
      cmsLog_error("cmsImg_getConfigFlashSize returned 0!");
      return CMSRET_INTERNAL_ERROR;
   }

   /* malloc a buffer for holding the config file from flash. */
   if ((cfgFlash = cmsMem_alloc(lenCfgFlash, 0)) == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", lenCfgFlash);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /*
    * load primary config file from flash to buffer.
    */
   if ((ret = oal_readConfigFlashToBuf(CMS_CONFIG_PRIMARY,
                                       cfgFlash,
                                       &lenCfgFlash)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Fail to loadd primary config file from flash to buffer, ret=%d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(cfgFlash);
      return CMSRET_INTERNAL_ERROR;
   }

   lenEndSm = strlen(END_TAG_SW_MODULES);

   /* calculate length of Software Modules in flash. */
   if ((startSmCfgFlash = strstr(cfgFlash, START_TAG_SW_MODULES)) != NULL &&
       (endSmCfgFlash = strstr(cfgFlash, END_TAG_SW_MODULES)) != NULL)
   {
      lenSmCfgFlash = endSmCfgFlash - startSmCfgFlash + lenEndSm;
   }
   else
   {
      cmsLog_notice("Software Modules does not exist in flash");
      CMSMEM_FREE_BUF_AND_NULL_PTR(cfgFlash);
      return writeValidatedBufToConfigFlash(cfgIn, lenCfgIn);
   }

   /* calculate length of Software Modules in configuration. */
   if ((startSmCfgIn = strstr(cfgIn, START_TAG_SW_MODULES)) != NULL &&
       (endSmCfgIn = strstr(cfgIn, END_TAG_SW_MODULES)) != NULL)
   {
      lenSmCfgIn = endSmCfgIn - startSmCfgIn + lenEndSm;
   }

   /* do nothing if Software Modules in configuration file is
    * the same with Software Modules in flash */
   if (lenSmCfgIn == lenSmCfgFlash &&
       memcmp(startSmCfgIn, startSmCfgFlash, lenSmCfgIn) == 0)
   {
      cmsLog_notice("Software Modules in configuration file is the same with Software Modules in flash");
      CMSMEM_FREE_BUF_AND_NULL_PTR(cfgFlash);
      return writeValidatedBufToConfigFlash(cfgIn, lenCfgIn);
   }

   /* calculate length of configuration output. */
   lenCfgOut = lenCfgIn - lenSmCfgIn + lenSmCfgFlash;

   /* malloc a buffer for holding the update config. */
   if ((cfgOut = cmsMem_alloc(lenCfgOut, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("malloc of %d bytes failed", lenCfgOut);
      CMSMEM_FREE_BUF_AND_NULL_PTR(cfgFlash);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   endCfgIn = strstr(cfgIn, END_TAG_ROOT_MDM);

   /* copy cfgIn to cfgOut without SoftwareModules section
    * up to and exclude END_TAG_ROOT_MDM */
   if (startSmCfgIn != NULL && endSmCfgIn != NULL)
   {
      len = startSmCfgIn - &(cfgIn[0]);
      memcpy(cfgOut, cfgIn, len);

      memcpy(&(cfgOut[len]),
             endSmCfgIn + lenEndSm,
             endCfgIn - (endSmCfgIn + lenEndSm));
   }
   else
   {
	  len = endCfgIn - &(cfgIn[0]);
      memcpy(cfgOut, cfgIn, len);
   }

   /* copy SoftwareModules section in cfgFlash to cfgOut */
   if (startSmCfgFlash != NULL && endSmCfgFlash != NULL)
   {
      len = strlen(cfgOut);
      if ((len + lenSmCfgFlash) < lenCfgOut)
      {
         memcpy(&(cfgOut[len]), startSmCfgFlash, lenSmCfgFlash);
      }
   }   

   /* copy from END_TAG_ROOT_MDM to end of cfgIn to cfgOut*/
   len = strlen(cfgOut);
   if ((len + lenCfgIn - (endCfgIn - &(cfgIn[0]))) <= lenCfgOut)
   {
      memcpy(&(cfgOut[len]), endCfgIn, lenCfgIn - (endCfgIn - &(cfgIn[0])));
   }

   ret = writeValidatedBufToConfigFlash(cfgOut, lenCfgOut);

   CMSMEM_FREE_BUF_AND_NULL_PTR(cfgFlash);

   CMSMEM_FREE_BUF_AND_NULL_PTR(cfgOut);
   
   return ret;
}

#endif


CmsRet cmsMgm_writeValidatedBufToConfigFlash(const char *buf, UINT32 len)
{
   CmsRet ret = CMSRET_SUCCESS;

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

#ifdef SUPPORT_BEEP

   ret = writeBeepValidatedBufToConfigFlash(buf, len);

#else

   ret = writeValidatedBufToConfigFlash(buf, len); 

#endif

   lck_autoUnlockAllZones(__FUNCTION__);

   return ret;
}


CmsRet cmsMgm_validateConfigBuf(const char *buf, UINT32 len)
{
   char *buf2=NULL;
   UBOOL8 freeBuf2=FALSE;
   CmsRet ret;

   cmsLog_debug("buf=%p len=%d", buf, len);

   CHECK_MDM_EXTERNAL_CALLER(__FUNCTION__);

   if ((ret = lck_autoLockAllZonesWithBackoff(0, MGM_LOCK_TIMEOUT, __FUNCTION__)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   // Validating a config buf can take a while, increase lock hold time
   // warning thresh.
   cmsLck_setHoldTimeWarnThresh(CMSLCK_MAX_HOLDTIME * 2);

   /*
    * mdm_validateConfigBuf expects the config file must end with a 0 byte.
    * This config file might have gotten its last 0 byte stripped off by
    * wordpad or something.  So check for that, and append a 0 byte if neccessary.
    */
   if (buf[len-1] != 0)
   {
      cmsLog_debug("final zero byte not detected, adding it.");
      if ((buf2 = cmsMem_alloc(len+1, ALLOC_ZEROIZE)) == NULL)
      {
         cmsLog_error("realloc of %u bytes for config buf failed", len);
         ret = CMSRET_RESOURCE_EXCEEDED;
      }
      else
      {
         freeBuf2 = TRUE;
         memcpy(buf2, buf, len);
         len++;
      }
   }
   else
   {
      cmsLog_debug("final zero byte detected, do nothing.");
      buf2 = (char *) buf;
   }


   ret = mdm_validateConfigBuf(buf2, len);

   if (freeBuf2)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(buf2);
   }

   lck_autoUnlockAllZones(__FUNCTION__);

   return ret;
}
