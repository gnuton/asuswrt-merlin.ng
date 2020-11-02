/***********************************************************************
 *
 *  Copyright (c) 2010  Broadcom Corporation
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
#include "cms_core.h"
#include "cms_util.h"
#include "cms_boardioctl.h"
#include "cms_msg.h"
#include "cms_seclog.h"
#include <errno.h>

#ifdef CMS_SECURITY_LOG

char * securityLogStrings[LOG_SECURITY_MAX] =
{
   "",                               /* 0  - custom string */
   "Password change success:",       /* 1  - LOG_SECURITY_PWD_CHANGE_SUCCESS */
   "Password change failure:",       /* 2  - LOG_SECURITY_PWD_CHANGE_FAIL */
   "Authorized login success:",      /* 3  - LOG_SECURITY_AUTH_LOGIN_PASS */
   "Authorized login fail:",         /* 4  - LOG_SECURITY_AUTH_LOGIN_FAIL */
   "Authorized user logged out:",    /* 5  - LOG_SECURITY_AUTH_LOGOUT */
   "Security lockout added:",        /* 6  - LOG_SECURITY_LOCKOUT_START */
   "Security lockout removed:",      /* 7  - LOG_SECURITY_LOCKOUT_END */
   "Authorized resource access:",    /* 8  - LOG_SECURITY_AUTH_RESOURCES */
   "Unauthorized resource access:",  /* 9  - LOG_SECURITY_UNAUTH_RESOURCES */
   "Software update:",               /* 10 - LOG_SECURITY_SOFTWARE_MOD */
};

int cmsLog_createSecurityString(char * buf, int maxLen, CmsSecurityLogIDs id, 
   CmsSecurityLogData * pdata, const char *pFmt, va_list * ap)
{
   CmsRet   ret;
   int      len = 0;
   char *   p_timestamp;

   p_timestamp = &buf[len];
   ret = cmsTms_getXSIDateTime(0, p_timestamp, (maxLen - len));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("getXSIDateTime returned %lu\n", ret);
   }
   else
   {
      len += strlen(p_timestamp);
   }
   
   if (len < maxLen)
   {
       len += snprintf(&(buf[len]), maxLen - len, " ID %u: ", id);
   }

   if (len < maxLen)
   {
       len += snprintf(&(buf[len]), maxLen - len, "%s", securityLogStrings[id]);
   }

   if ((pFmt != NULL) && (len < maxLen))
   {
      /* Custom string */
      if (len < maxLen)
         len += vsnprintf(&buf[len], (maxLen - len), pFmt, *ap);
   }

   if ((len < maxLen) && (pdata->data_flags & CMSLOG_SEC_USER_FLAG))
   {
       len += snprintf(&(buf[len]), maxLen - len, ":U %s", pdata->user);
   }

   if ((len < maxLen) && (pdata->data_flags & CMSLOG_SEC_LEVEL_FLAG))
   {
       len += snprintf(&(buf[len]), maxLen - len, ":L %u", pdata->security_level);
   }

   if ((len < maxLen) && (pdata->data_flags & CMSLOG_SEC_APP_NAME_FLAG))
   {
       len += snprintf(&(buf[len]), maxLen - len, ":N %s", pdata->appName);
   }

   if ((len < maxLen) && (pdata->data_flags & CMSLOG_SEC_PORT_FLAG))
   {
       len += snprintf(&(buf[len]), maxLen - len, ":P %u", pdata->port);
   }

   if ((len < maxLen) && (pdata->data_flags & CMSLOG_SEC_SRC_IP_FLAG))
   {
       len += snprintf(&(buf[len]), maxLen - len, ":IP %s", pdata->ipAddr);
   }

   /* Add the byte for the trailing '\0' */
   len++;

   if (len < maxLen)
      return(len);
   else
      return(maxLen);

}


CmsRet cmsLog_security(CmsSecurityLogIDs id, CmsSecurityLogData * pdata, const char *pFmt, ... )
{
   CmsRet ret = CMSRET_SUCCESS;
   char buf[MAX_LOG_LINE_LENGTH] = {0};
   int len=0, maxLen;
   va_list        ap;
   FILE * stream;
   uint32_t read_offset = 0;
   uint32_t write_offset = 0;
   int retval = 0; 
   
   maxLen = sizeof(buf);

   // The feature is not part of the MDM, but just use zone lock 0 for now.
   // Find a better place for this feature.
   if ((ret = cmsLck_acquireZoneLockWithTimeout(0, CMSLOG_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get lock, ret=%d", ret);
   }
   else
   {      
      /* Retrieve the current log from flash */
      stream = fopen(SECURITY_LOG_FILE_NAME, "r+");

      if ((stream == NULL) && (errno == ENOENT))
      {
         /* The file doesn't exist, see if we can create it. */
         stream = fopen(SECURITY_LOG_FILE_NAME, "w+");
      }

      if (stream == NULL)
      {
         /* Don't log an error if this a read-only filesystem */
         if (errno != EROFS)
         {
            cmsLog_error("open of security log failed, %s (%d)", strerror(errno), errno);
         }
         ret = CMSRET_OPEN_FILE_ERROR;
      }
      else
      {
         if (id < LOG_SECURITY_MAX)
         {
            if ( pFmt != NULL )
            {
               va_start(ap, pFmt);
               len = cmsLog_createSecurityString(buf, maxLen, id, pdata, pFmt, &ap);
               va_end(ap);
            }
            else
            {
               len = cmsLog_createSecurityString(buf, maxLen, id, pdata, NULL, NULL);
            }

            if (len >= sizeof(buf))
            {
               cmsLog_error("overflow, log ID %02u", id);
               ret = CMSRET_RESOURCE_EXCEEDED;
            }
         }
         else
         {
            cmsLog_error("invalid security log id %02u", id);
            ret = CMSRET_INVALID_ARGUMENTS;
         }

         if (ret == CMSRET_SUCCESS)
         {
            /* Read the read offset from the file */
            if (fseek(stream, SECURITY_LOG_RD_OFFSET, SEEK_SET) == 0)
            {
               retval = fread(&read_offset, 1, sizeof(read_offset), stream);
            }
            else
            {
               cmsLog_error("fseek failed rd_offs: %s (%d)", strerror(errno), errno);
            }

            /* Read the write offset from the file */
            if (fseek(stream, SECURITY_LOG_WR_OFFSET, SEEK_SET) == 0)
            {
               retval = fread(&write_offset, 1, sizeof(write_offset), stream);
            }
            else
            {
               cmsLog_error("fseek failed wr_offs: %s (%d)", strerror(errno), errno);
            }

            if ((read_offset >= SECURITY_LOG_DATA_SIZE) || 
                (write_offset >= SECURITY_LOG_DATA_SIZE))
            {
               cmsLog_error("Security log seems corrupt, resetting: RD %d WR %d", 
                  read_offset, write_offset);
               read_offset = 0;
               write_offset = 0;
            }

            if ((len + write_offset) < SECURITY_LOG_DATA_SIZE)
            {
               /* We can write the new log in one piece */
               if (fseek(stream, (SECURITY_LOG_DATA_OFFSET + write_offset), SEEK_SET) != 0)
               {
                  cmsLog_error("fseek failed: %s (%d)", strerror(errno), errno);
               }
               else
               {
                  retval = fwrite(buf, 1, len, stream);
                  if (retval != len)
                  {
                     cmsLog_error("fwrite failed: wrote %d chars, expected %d", retval, len);
                  }                     
               }
               
               /* If read_offset == write_offset, that should mean that
                * the log hasn't been written to => no need to increment
                * the read pointer. */
               if ((read_offset > write_offset) &&
                   (read_offset <= write_offset + len))
               {
                  /* Find the end of the log we overwrote */
                  do
                  {
                     retval = fgetc(stream);
                  } while ((retval != EOF) && (retval != 0x0));
                  
                  if (retval == EOF)
                  {
                     fseek(stream, SECURITY_LOG_DATA_OFFSET, SEEK_SET);
                     do
                     {
                        retval = fgetc(stream);
                     } while ((retval != EOF) && (retval != 0x0));                     
                  }

                  if (retval == 0x0)
                  {
                     read_offset = ftell(stream) - SECURITY_LOG_DATA_OFFSET;
                  }
                  else
                  {
                     /* Reset the log to start with the most recent entry */
                     read_offset = write_offset;
                  }
               }

               /* Advance the write offset */
               write_offset += len;
            }
            else
            {
               /* This log will wrap around the end of the buffer, we need to 
                * write it in two chunks. */
               if (fseek(stream, (SECURITY_LOG_DATA_OFFSET + write_offset), SEEK_SET) != 0)
               {
                  cmsLog_error("fseek failed: %s (%d)", strerror(errno), errno);
               }
               else
               {
                  retval = fwrite(buf, 1, (SECURITY_LOG_DATA_SIZE - write_offset), stream);
                  if (retval != (SECURITY_LOG_DATA_SIZE - write_offset))
                  {
                     cmsLog_error("fwrite failed: wrote %d chars, expected %d", 
                        retval, (SECURITY_LOG_DATA_SIZE - write_offset));
                  }
               }

               if (fseek(stream, SECURITY_LOG_DATA_OFFSET, SEEK_SET) != 0)
               {
                  cmsLog_error("fseek failed: %s (%d)", strerror(errno), errno);
               }
               else
               {
                  retval = fwrite(&buf[SECURITY_LOG_DATA_SIZE - write_offset], 
                     1, (write_offset + len - SECURITY_LOG_DATA_SIZE), stream);
                  if (retval != (write_offset + len - SECURITY_LOG_DATA_SIZE))
                  {
                     cmsLog_error("fwrite failed: wrote %d chars, expected %d", 
                        retval, (write_offset + len - SECURITY_LOG_DATA_SIZE));
                  }                     
               }

               /* If read_offset == write_offset, that should mean that
                * the log hasn't been written to => no need to increment
                * the read pointer. */
               if ((read_offset > write_offset) ||
                   (read_offset <= write_offset + len - SECURITY_LOG_DATA_SIZE))
               {
                  do
                  {
                     retval = fgetc(stream);
                  } while ((retval != EOF) && (retval != 0x0));
                  
                  if (retval == 0x0)
                  {
                     read_offset = ftell(stream) - SECURITY_LOG_DATA_OFFSET;
                  }
                  else
                  {
                     /* Reset the log to start with the most recent entry */
                     read_offset = write_offset;
                  }
               }               

               write_offset = write_offset + len - SECURITY_LOG_DATA_SIZE;
            }

            /* Write the read and write offset values to the file */
            if (fseek(stream, SECURITY_LOG_RD_OFFSET, SEEK_SET) != 0)
            {
               cmsLog_error("fseek failed: %s (%d)", strerror(errno), errno);
            }
            retval = fwrite(&read_offset, 1, SECURITY_LOG_RD_SIZE, stream);

            if (fseek(stream, SECURITY_LOG_WR_OFFSET, SEEK_SET) != 0)
            {
               cmsLog_error("fseek failed: %s (%d)", strerror(errno), errno);
            }

            retval = fwrite(&write_offset, 1, SECURITY_LOG_WR_SIZE, stream);
            fflush(stream);

            /* Write new log to flash */
            if (fclose(stream) != 0)
            {
               cmsLog_error("close of security log failed, %s (%d)", strerror(errno), errno);
               ret = CMSRET_REQUEST_DENIED;
            }
            else
            {
               cmsLog_notice("wrote to security log file");
            }

         }
      }

      cmsLck_releaseZoneLock(0);
   }

   return(ret);

}

CmsRet cmsLog_getSecurityLog(CmsSecurityLogFile * log)
{
   CmsRet ret = CMSRET_SUCCESS; 
   int needLock = 1;
   FILE * stream;
   int retval = 0;
   

   if ((ret = cmsLck_acquireZoneLockWithTimeout(0, CMSLOG_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get lock, ret=%d", ret);
   }
   else
   {
      /* Retrieve the current log from flash */
      stream = fopen(SECURITY_LOG_FILE_NAME, "r");

      if (stream == NULL)
      {
         if ((errno == ENOENT) || (errno == EROFS))
         {
            /* This isn't really an error. The file could have been deleted
             * by the user or just not exist yet, or could be R/O filesystem. */
            log->read_offset = 0;
            log->write_offset = 0;
            cmsLog_notice("security log doesn't exist", retval);
         }
         else
         {
            cmsLog_error("open of security log failed, %s (%d)", strerror(errno), errno);
            ret = CMSRET_OPEN_FILE_ERROR;
         }
      }
      else
      {
         retval = fread(log, 1, sizeof(*log), stream);         
         cmsLog_notice("read from security log flash, ret=%d", retval);
         fclose(stream);

      }

      cmsLck_releaseZoneLock(0);
   }

   return(ret);
   
}

void cmsLog_printSecurityLog(CmsSecurityLogFile * log)
{
   UINT32 rd, end;
   
   if (log == NULL)
      return;

   rd = log->read_offset;
   if (rd > log->write_offset)
      end = log->write_offset + SECURITY_LOG_DATA_SIZE;
   else
      end = log->write_offset;

   printf("Security Log (R=%lu W=%lu)\n", log->read_offset, log->write_offset);

   while (rd < end)
   {
      if (log->log[rd % SECURITY_LOG_DATA_SIZE] != 0x0)
         putc(log->log[rd % SECURITY_LOG_DATA_SIZE], stderr);
      else
         fprintf(stderr, "\n");

      rd++;
   };
}


CmsRet cmsLog_resetSecurityLog(void)
{
   CmsRet ret = CMSRET_SUCCESS;
   int retval;
   

   if ((ret = cmsLck_acquireZoneLockWithTimeout(0, CMSLOG_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get lock, ret=%d", ret);
   }
   else
   {
      /* Erase the log */
      retval = remove(SECURITY_LOG_FILE_NAME);
      /* It's not an error on a R/O filesystem or if the file 
       * already doesn't exist */
      if (retval != 0 && (errno != EROFS) && (errno == ENOENT))
      {
         cmsLog_error("unable to remove security log file: %s (%d)",
            strerror(errno), errno);
         ret = CMSRET_INTERNAL_ERROR;
      }

      cmsLog_notice("remove security log file, ret=%d", retval);

      cmsLck_releaseZoneLock();
   }

   return(ret);

}
#else
CmsRet cmsLog_security(CmsSecurityLogIDs id __attribute__((unused)),
                       CmsSecurityLogData * pdata __attribute__((unused)),
                       const char *pFmt __attribute__((unused)), ... )
{
   return(CMSRET_SUCCESS);
}

CmsRet cmsLog_getSecurityLog(CmsSecurityLogFile * log)
{
   if (log != NULL)
   {
      log->read_offset = 0;
      log->write_offset = 0;
   }
   return(CMSRET_SUCCESS);
}

void cmsLog_printSecurityLog(CmsSecurityLogFile * log __attribute__((unused)))
{
   printf("Security Log not supported\n");
   return;
}


CmsRet cmsLog_resetSecurityLog(void)
{
   return(CMSRET_SUCCESS);
}
#endif /* CMS_SECURITY_LOG */
