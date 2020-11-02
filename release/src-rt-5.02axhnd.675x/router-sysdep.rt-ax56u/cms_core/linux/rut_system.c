/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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

#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <pwd.h>    /* for putpwent */
#include <sys/ioctl.h>
#include <net/if.h>
#include <bcm_local_kernel_include/linux/sockios.h>
#include "cms.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "rut_dns.h"
#include "rut_system.h"
#include "rut_util.h"
#include "../mdm.h"
#include "bcmnet.h"
#include "devctl_atm.h"
#include "devctl_xtm.h"
#include "cms_qdm.h"


/* /var/passwd is linked to /etc/passwd */
#define loginPasswdFile "/var/passwd"
#define loginGroupFile  "/var/group"  

/***************************************************************************
// Function Name: bcmCreateLoginCfg().
// Description  : create password file for login using 'admin' or 'support'.
// Parameters   : cp_admin - clear password of 'admin'.
//                cp_support - clear password of 'support'.
//                cp_user - clear password of 'user'.
// Returns      : status 0 - OK, -1 - ERROR.
****************************************************************************/
CmsRet rut_createLoginCfg(const char *cp_admin __attribute__((unused)),
                          const char *cp_support __attribute__((unused)),
                          const char *cp_user __attribute__((unused)))
{
#if defined(DESKTOP_LINUX) && !defined(BUILD_DESKTOP_BEEP)

   cmsLog_debug("Changing /etc/passwd, admin = %s, support = %s, user = %s", cp_admin, cp_support, cp_user);
   return CMSRET_SUCCESS;

#else   
   struct passwd pw;
   char tmp_file[BUFLEN_32];
   int tmp_fd;
   FILE *tmp_fp = NULL;
   char line[BUFLEN_264];
   char cmd[BUFLEN_128];

   cmsLog_debug("Enter");   
   
   /*
    * To preserve the new lines added on by others, perform the following steps:
    *
    * 1) create a temp passwd and write out the admin, user and support new entries
    * 2) append any other lines from the original passwd to the new temp file
    * 3) delete the old passwd file and rename the new temp passwd to /var/passwd (linked to /etc/passwd)
    */

   strncpy(tmp_file, "/var/passwdXXXXXX", sizeof(tmp_file));    
   tmp_fd = mkstemp(tmp_file);
   if (tmp_fd == -1)
   {
      cmsLog_error("Failed to create passwd tmp file");
      return CMSRET_INTERNAL_ERROR;
   }

   tmp_fp = fdopen(tmp_fd, "w+");
   if (tmp_fp == NULL) 
   {
      cmsLog_error("Failed to create file stream for passwd tmp file");
      close(tmp_fd);
      return CMSRET_INTERNAL_ERROR;
   }

   
   // In future, we may change uid of 'admin' and 'support'
   // uclibc may have a bug on putpwent in terms of uid,gid setup

   /* write out the admin, user, support line to the temp passwd file */
   pw.pw_name = "admin";
#ifdef SUPPORT_HASHED_PASSWORDS
   pw.pw_passwd = (char *)cp_admin;
#else
   pw.pw_passwd = cmsUtil_pwEncrypt(cp_admin, cmsUtil_cryptMakeSalt());
#endif
   pw.pw_uid = 0;
   pw.pw_gid = 0;
   pw.pw_gecos = "Administrator";
   pw.pw_dir = "/";
   pw.pw_shell = "/bin/sh";
   putpwent(&pw, tmp_fp);

   pw.pw_name = "support";
#ifdef SUPPORT_HASHED_PASSWORDS
   pw.pw_passwd = (char *)cp_support;
#else
   pw.pw_passwd = cmsUtil_pwEncrypt(cp_support, cmsUtil_cryptMakeSalt());
#endif
   pw.pw_uid = 0;
   pw.pw_gid = 0;
   pw.pw_gecos = "Technical Support";
   pw.pw_dir = "/";
   pw.pw_shell = "/bin/sh";
   putpwent(&pw, tmp_fp);

   pw.pw_name = "user";
#ifdef SUPPORT_HASHED_PASSWORDS
   pw.pw_passwd = (char *)cp_user;
#else
   pw.pw_passwd = cmsUtil_pwEncrypt(cp_user, cmsUtil_cryptMakeSalt());
#endif
   pw.pw_uid = 0;
   pw.pw_gid = 0;
   pw.pw_gecos = "Normal User";
   pw.pw_dir = "/";
   pw.pw_shell = "/bin/sh";
   putpwent(&pw, tmp_fp);
   pw.pw_name = "nobody";
#ifdef SUPPORT_HASHED_PASSWORDS
   pw.pw_passwd = (char *)cp_admin;
#else
   pw.pw_passwd = cmsUtil_pwEncrypt(cp_admin, cmsUtil_cryptMakeSalt());
#endif
   pw.pw_uid = 0;
   pw.pw_gid = 0;
   pw.pw_gecos = "nobody for ftp";
   pw.pw_dir = "/";
   pw.pw_shell = "/bin/sh";
   putpwent(&pw, tmp_fp);

   /* Now need to add on any none admin,user,support lines from passwd
   * file to the new temp passwd file
   */
   if (cmsFil_isFilePresent(loginPasswdFile))
   {
      FILE *fsPwd = NULL;

      cmsLog_debug("%s is present", loginPasswdFile);
      if ((fsPwd = fopen(loginPasswdFile, "r")) == NULL)
      {
         cmsLog_error("Failed to open %s ?", loginPasswdFile);
         fclose(tmp_fp);
         unlink(tmp_file);
         return CMSRET_INTERNAL_ERROR;
      }

      while ((fgets(line, sizeof(line), fsPwd) != NULL )) 
      {
         if (!cmsUtl_strncmp(line, "admin:", strlen("admin:")) || 
             !cmsUtl_strncmp(line, "user:", strlen("user:")) || 
             !cmsUtl_strncmp(line, "support:", strlen("support:")) ||
             !cmsUtl_strncmp(line, "nobody:", strlen("nobody:")))
            
         {
            cmsLog_debug("skip line %s", line);
         }
         else
         {
            cmsLog_debug("adding line %s", line);
            fputs(line, tmp_fp);
         }
      }
      
      fclose(fsPwd);
   }
   fclose(tmp_fp);

   /* update /var/passwd with temp passwd file */
   sprintf(cmd, "cp -f %s %s", tmp_file, loginPasswdFile);
   if (system(cmd) != 0)
   {
      cmsLog_error("Failed to update %s", loginPasswdFile);
      unlink(tmp_file);
      return CMSRET_INTERNAL_ERROR;
   }
   unlink(tmp_file);

   strncpy(tmp_file, "/var/groupXXXXXX", sizeof(tmp_file));    
   tmp_fd = mkstemp(tmp_file);
   if (tmp_fd == -1)
   {
      cmsLog_error("Failed to create group tmp file");
      return CMSRET_INTERNAL_ERROR;
   }

   tmp_fp = fdopen(tmp_fd, "w+");
   if (tmp_fp == NULL) 
   {
      cmsLog_error("Failed to create file stream for group tmp file");
      close(tmp_fd);
      return CMSRET_INTERNAL_ERROR;
   }

#ifdef SUPPORT_IEEE1905_GOLDENNODE
   fputs("root::0:root,admin,ncap\n", tmp_fp);
#else
   fputs("root::0:root,admin,support,user\n", tmp_fp);
#endif

   /* Now need to add on any none root lines from group
   * file to the new temp passwd file
   */
   if (cmsFil_isFilePresent(loginGroupFile))
   {
      FILE *fsGrp = NULL;

      cmsLog_debug("%s is present", loginGroupFile);
      if ((fsGrp = fopen(loginGroupFile, "r")) == NULL)
      {
         cmsLog_error("Failed to open %s ?", loginGroupFile);
         fclose(tmp_fp);
         unlink(tmp_file);
         return CMSRET_INTERNAL_ERROR;
      }

      while ((fgets(line, sizeof(line), fsGrp) != NULL )) 
      {
         if (!cmsUtl_strncmp(line, "root:", strlen("root:")))
         {
            cmsLog_debug("skip line %s", line);
         }
         else
         {
            cmsLog_debug("adding line %s", line);
            fputs(line, tmp_fp);
         }
      }
      
      fclose(fsGrp);
   }
   fclose(tmp_fp);

   /* update /var/group with temp group file */
   sprintf(cmd, "cp -f %s %s", tmp_file, loginGroupFile);
   if (system(cmd) != 0)
   {
      cmsLog_error("Failed to update %s", loginGroupFile);
      unlink(tmp_file);
      return CMSRET_INTERNAL_ERROR;
   }
   unlink(tmp_file);

   return CMSRET_SUCCESS;

#endif /* DESKTOP_LINUX */
}


UBOOL8 rut_isSyslogCfgChanged(const _SyslogCfgObject *newObj,
                              const _SyslogCfgObject *currObj)
{
   UBOOL8 isChanged=FALSE;

   /* changes in status (enabled/disabled) is checked in rcl_syslogCfgObject() */
   /* localDisplayLevel does not affect syslogd options, so it is not checked. */

   if (strcmp(newObj->option, currObj->option) ||
       strcmp(newObj->localLogLevel, currObj->localLogLevel) ||
       strcmp(newObj->remoteLogLevel, currObj->remoteLogLevel) ||
       strcmp(newObj->serverIPAddress, currObj->serverIPAddress) ||
       (newObj->serverPortNumber != currObj->serverPortNumber))
   {
      isChanged = TRUE;
   }

   return isChanged;
}


CmsRet rut_restartsysklogd(const _SyslogCfgObject *syslogCfg)
{
   char args[BUFLEN_128];
   CmsRet ret = CMSRET_SUCCESS;


   /*
    * First, explictly stop klogd first and then syslogd.
    */
   rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_KLOGD, NULL, 0);
   rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_SYSLOGD, NULL, 0);


   /* Then start syslogd */
   cmsAst_assert(!(strcmp(syslogCfg->status, MDMVS_ENABLED)));

   if (!strcmp(syslogCfg->option, MDMVS_LOCAL_BUFFER))
   {
      snprintf(args, BUFLEN_128, "-C -l %d",
               cmsUtl_syslogLevelToNum(syslogCfg->localLogLevel));
   }
   else if (!strcmp(syslogCfg->option, MDMVS_REMOTE))
   {
      snprintf(args, BUFLEN_128, "-R %s:%i -r %d",
               syslogCfg->serverIPAddress, syslogCfg->serverPortNumber, 
               cmsUtl_syslogLevelToNum(syslogCfg->remoteLogLevel));
   }
   else if (!strcmp(syslogCfg->option, MDMVS_LOCAL_BUFFER_AND_REMOTE))
   {
      snprintf(args, BUFLEN_128, "-C -L -l %d -R %s:%i -r %d",
               cmsUtl_syslogLevelToNum(syslogCfg->localLogLevel),
               syslogCfg->serverIPAddress, syslogCfg->serverPortNumber, 
               cmsUtl_syslogLevelToNum(syslogCfg->remoteLogLevel));
   }
   else if (!strcmp(syslogCfg->option, MDMVS_LOCAL_FILE))
   {
      /* this option not available through webUI, do we really support this? */
      snprintf(args, BUFLEN_128, "-L -l %d",
               cmsUtl_syslogLevelToNum(syslogCfg->localLogLevel));
   }
   else if (!strcmp(syslogCfg->option, MDMVS_LOCAL_BUFFER_AND_REMOTE))
   {
      /* this option not available through webUI, do we really support this? */
      snprintf(args, BUFLEN_128, "-L -l %d -R %s:%i -r %d",
               cmsUtl_syslogLevelToNum(syslogCfg->localLogLevel),
               syslogCfg->serverIPAddress, syslogCfg->serverPortNumber, 
               cmsUtl_syslogLevelToNum(syslogCfg->remoteLogLevel));
   }
   else
   {
      cmsLog_error("Unrecognized syslog mode option, %s", syslogCfg->option);
      ret = CMSRET_INVALID_ARGUMENTS;
   }


   if (ret == CMSRET_SUCCESS)
   {
      SINT32 pid;
      pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_SYSLOGD, args, strlen(args) + 1);
      if (pid == CMS_INVALID_PID)
      {
         /*
          * Uh,oh, if we fail to start syslogd, we need to restart it using
          * the old args!  Realistically though, this will never happen...
          */
         cmsLog_error("failed to start syslogd");
         ret = CMSRET_INTERNAL_ERROR;
      }
   }

   /* finally start klogd */
   if (ret == CMSRET_SUCCESS)
   {
      SINT32 pid;
      pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_KLOGD, NULL, 0);
      if (pid == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start klogd");
         ret = CMSRET_INTERNAL_ERROR;
      }
   }

   return ret;
}


char *rutSys_getDeviceLog(UINT16 *logLen)
{
   int readptr = BCM_SYSLOG_FIRST_READ;
   int dataLen = 0;
   char data[BCM_SYSLOG_MAX_LINE_SIZE];
   char *log = NULL, *cp = NULL;

   *logLen = 0;
   // try reading circular buffer
   while ((readptr != BCM_SYSLOG_READ_BUFFER_ERROR) &&
          (readptr != BCM_SYSLOG_READ_BUFFER_END)) {
      readptr = cmsLog_readPartial(readptr,data);
      if (readptr == BCM_SYSLOG_READ_BUFFER_ERROR)
      {
         *logLen = 0;
         break;
      }
      else
      {
         dataLen = strlen(data);
      }
      if (log == NULL)
      {
          cp = cmsMem_alloc(*logLen + dataLen, ALLOC_ZEROIZE);
      }
      else 
      {
          // reallocate log size
          cp = cmsMem_realloc(log, *logLen + dataLen);
      }         

      if (cp == NULL)
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(log);
         *logLen = 0;
         break;
      }
      else
      {
         log = cp;
      }

      // append chunked data to log 
      strncpy(log + *logLen, data, dataLen);

      *logLen += dataLen;
   }

   return log;
}


void rut_updateLogLevel(CmsEntityId destEid, const char *logLevel)
{
   cmsLog_debug("eid=%d new log level=%s", destEid, logLevel);

   if (destEid == mdmLibCtx.eid)
   {
      /* don't send a message to myself, just set log level directly */
      cmsLog_setLevel(cmsUtl_logLevelStringToEnum(logLevel));
   }
   else if (destEid == EID_SSHD || destEid == EID_TELNETD || destEid == EID_CONSOLED)
   {
      /*
       * These apps do not support dynamic setting of log level/dest via a message.
       * This is very hard to do for sshd and telnetd because they fork children
       * to handle the CLI part.
       * Do nothing, require restart of sshd/telnetd/consoled before new settings
       * take effect (note: if you set the telnetd log level from the telnetd window
       * itself, then the first "if" clause will be true and the settings will
       * take effect dynamically.
       */
   }
   else
   {
      CmsMsgHeader msg = EMPTY_MSG_HEADER;
      CmsRet ret;

      msg.type = CMS_MSG_SET_LOG_LEVEL;
      msg.src = mdmLibCtx.eid;
      msg.dst = destEid;

      msg.flags_request = 1;
      msg.flags_bounceIfNotRunning = 1;
      msg.wordData = cmsUtl_logLevelStringToEnum(logLevel);

      ret = cmsMsg_sendAndGetReplyWithTimeout(mdmLibCtx.msgHandle, &msg, CMSLCK_MAX_HOLDTIME);
      if (ret != CMSRET_SUCCESS && ret != CMSRET_MSG_BOUNCED)
      {
         cmsLog_error("update log level failed, ret=%d", ret);
      }
   }
}


void rut_updateLogDestination(CmsEntityId destEid, const char *logDest)
{
   cmsLog_debug("eid=%d new log dest=%s", destEid, logDest);

   if (destEid == mdmLibCtx.eid)
   {
      /* don't send a message to myself, just set log dest directly */
      cmsLog_setDestination(cmsUtl_logDestinationStringToEnum(logDest));
   }
   else if (destEid == EID_SSHD || destEid == EID_TELNETD || destEid == EID_CONSOLED)
   {
      /*
       * These apps do not support dynamic setting of log level/dest via a message.
       * This is very hard to do for sshd and telnetd because they fork children
       * to handle the CLI part.
       * Do nothing, require restart of sshd/telnetd/consoled before new settings
       * take effect (note: if you set the telnetd log level from the telnetd window
       * itself, then the first "if" clause will be true and the settings will
       * take effect dynamically.
       */
   }
   else
   {
      CmsMsgHeader msg = EMPTY_MSG_HEADER;
      CmsRet ret;

      msg.type = CMS_MSG_SET_LOG_DESTINATION;
      msg.src = mdmLibCtx.eid;
      msg.dst = destEid;

      msg.flags_request = 1;
      msg.flags_bounceIfNotRunning = 1;
      msg.wordData = cmsUtl_logDestinationStringToEnum(logDest);

      ret = cmsMsg_sendAndGetReplyWithTimeout(mdmLibCtx.msgHandle, &msg, CMSLCK_MAX_HOLDTIME);
      if (ret != CMSRET_SUCCESS && ret != CMSRET_MSG_BOUNCED)
      {
         cmsLog_error("update log dest failed, ret=%d", ret);
      }
   }
}

/*
 * logging SOAP oject is changed, 
 * send a TR69C_CONFIG_CHANGED event msg to tr69c/smd with source ConfigId,
 */
void rut_updateLogSOAP(CmsEntityId destEid, const char *ConfigId)
{

   CmsMsgHeader *msgHdr;
   char *dataPtr = NULL;
   UINT32 dataLen = 0;

   if (ConfigId != NULL)
   {
      dataLen = strlen(ConfigId) + 1;
      cmsLog_debug("ConfigId=%s", ConfigId);
   }
   
   msgHdr = (CmsMsgHeader *) cmsMem_alloc(sizeof(CmsMsgHeader) + dataLen, ALLOC_ZEROIZE);
   if (msgHdr == NULL)
   {
      cmsLog_error("message header allocation failed, len of ConfigId=%d", strlen(ConfigId));
      return;
   }

   msgHdr->type = CMS_MSG_TR69C_CONFIG_CHANGED;
   msgHdr->src = mdmLibCtx.eid;
   msgHdr->dst = destEid;
   msgHdr->flags_event = 1;
   msgHdr->dataLength = dataLen;
   if (ConfigId != NULL)
   {
      dataPtr = (char *) (msgHdr+1);
      strncpy(dataPtr, ConfigId, strlen(ConfigId));
   }

   if (cmsMsg_send(mdmLibCtx.msgHandle, msgHdr) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send out TR69C_CONFIG_CHANGED event msg");
   }
   else
   {
      cmsLog_debug("Send out TR69C_CONFIG_CHANGED event msg.");
   }

   cmsMem_free(msgHdr);

   return;
}


/* get interface statistics: Ethernet, USB, WLAN based on device name *devName. */
void rut_getIntfStats(const char *devName, 
                      UINT32 *byteRx, UINT32 *packetRx, 
                      UINT32 *byteMultiRx, UINT32 *packetMultiRx, UINT32 *packetUniRx, UINT32 *packetBcastRx,
                      UINT32 *errRx, UINT32 *dropRx,
                      UINT32 *byteTx, UINT32 *packetTx, 
                      UINT32 *byteMultiTx, UINT32 *packetMultiTx, UINT32 *packetUniTx, UINT32 *packetBcastTx,
                      UINT32 *errTx, UINT32 *dropTx)
{
   int count = 0;
   char *pChar = NULL;
   char line[BUFLEN_512], buf[BUFLEN_512];
   char dummy[BUFLEN_32];
   char rxByte[BUFLEN_32];
   char rxPacket[BUFLEN_32];   
   char rxMultiByte[BUFLEN_32];
   char rxPacketMulti[BUFLEN_32];
   char rxPacketUni[BUFLEN_32];
   char rxPacketBcast[BUFLEN_32];   
   char rxErr[BUFLEN_32];
   char rxDrop[BUFLEN_32];
   char txByte[BUFLEN_32];
   char txPacket[BUFLEN_32];   
   char txMultiByte[BUFLEN_32];
   char txPacketMulti[BUFLEN_32];
   char txPacketUni[BUFLEN_32];
   char txPacketBcast[BUFLEN_32];
   char txErr[BUFLEN_32];
   char txDrop[BUFLEN_32];
   char *pcDevNameStart;


   if (devName == NULL)
   {
      *byteRx = 0;
      *packetRx = 0;
      *byteMultiRx = 0;
      *packetMultiRx = 0;
      *packetUniRx = 0;
      *packetBcastRx = 0;      
      *errRx = 0;
      *dropRx = 0;
      *byteTx = 0;
      *packetTx = 0;      
      *byteMultiTx = 0;
      *packetMultiTx = 0;
      *packetUniTx = 0;
      *packetBcastTx = 0;      
      *errTx = 0;
      *dropTx = 0;
      return;
   }

   
   /* getstats put device statistics into this file, read the stats */
   /* Be sure to read the page with the extended stats */
   FILE* fs = fopen("/proc/net/dev_extstats", "r");
   if ( fs == NULL ) 
   {
      return;
   }

   // find interface
   while ( fgets(line, sizeof(line), fs) ) 
   {
      /* read pass 2 header lines */
      if ( count++ < 2 ) 
      {
         continue;
      }

      /* normally line will have the following example value
       * "eth0: 19140785 181329 0 0 0 0 0 0 372073009 454948 0 0 0 0 0 0"
       * but when the number is too big then line will have the following example value
       * "eth0:19140785 181329 0 0 0 0 0 0 372073009 454948 0 0 0 0 0 0"
       * so to make the parsing correctly, the following codes are added
       * to insert space between ':' and number
       */
      pChar = strchr(line, ':');
      if ( pChar != NULL )
      {
         pChar++;
      }
      if ( pChar != NULL && isdigit(*pChar) ) 
      {
         strcpy(buf, pChar);
         *pChar = ' ';
         strcpy(++pChar, buf);
      }
	  
      /* Find and test the interface name to see if it's the one we want.
         If so, then store statistic values.	  */
      pcDevNameStart = strstr(line, devName);
      if ( (pcDevNameStart != NULL) && *(pcDevNameStart + strlen(devName)) == ':' )
      {
         sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
                dummy, rxByte, rxPacket, rxErr, rxDrop, dummy, dummy, dummy, rxPacketMulti,
                txByte, txPacket, txErr, txDrop, dummy, dummy, dummy, dummy,
                txPacketMulti, rxMultiByte, txMultiByte, rxPacketUni, txPacketUni, rxPacketBcast, txPacketBcast, dummy);  
         *byteRx= (UINT32)strtoull(rxByte, NULL, 10);
         *packetRx = (UINT32)strtoull(rxPacket, NULL, 10);
         *byteMultiRx = (UINT32)strtoull(rxMultiByte, NULL, 10);
         *packetMultiRx = (UINT32)strtoull(rxPacketMulti, NULL, 10);
         *packetUniRx = (UINT32)strtoull(rxPacketUni, NULL, 10);
         *packetBcastRx = (UINT32)strtoull(rxPacketBcast, NULL, 10); 
         *errRx = (UINT32)strtoull(rxErr, NULL, 10);
         *dropRx = (UINT32)strtoull(rxDrop, NULL, 10);
         *byteTx = (UINT32)strtoull(txByte, NULL, 10);
         *packetTx = (UINT32)strtoull(txPacket, NULL, 10);
         *byteMultiTx = (UINT32)strtoull(txMultiByte, NULL, 10);
         *packetMultiTx = (UINT32)strtoull(txPacketMulti, NULL, 10);
         *packetUniTx = (UINT32)strtoull(txPacketUni, NULL, 10);
         *packetBcastTx = (UINT32)strtoull(txPacketBcast, NULL, 10);
         *errTx = (UINT32)strtoull(txErr, NULL, 10);
         *dropTx = (UINT32)strtoull(txDrop, NULL, 10);
         
         /* Interface found - break out of while() loop */
         break;
      } /* devName */
   } /* while */
   
   fclose(fs);

   /* if any byte counter hits ULONG_MAX then reset counters */
   if (*byteRx == UINT32_MAX || *byteMultiRx == UINT32_MAX ||
       *errRx == UINT32_MAX || *dropRx == UINT32_MAX ||
       *byteTx == UINT32_MAX || *byteMultiTx == UINT32_MAX ||
       *errTx == UINT32_MAX || *dropTx == UINT32_MAX)
   {
      rut_clearIntfStats(devName);
   }
}


/* get interface statistics: Ethernet, USB, WLAN based on device name *devName. */
void rut_getIntfStats_uint64(const char *devName, 
                             UINT64 *byteRx, UINT64 *packetRx, 
                             UINT64 *byteMultiRx, UINT64 *packetMultiRx,
                             UINT64 *packetUniRx, UINT64 *packetBcastRx,
                             UINT64 *errRx, UINT64 *dropRx,
                             UINT64 *byteTx, UINT64 *packetTx, 
                             UINT64 *byteMultiTx, UINT64 *packetMultiTx,
                             UINT64 *packetUniTx, UINT64 *packetBcastTx,
                             UINT64 *errTx, UINT64 *dropTx)
{
   int count = 0;
   char *pChar = NULL;
   char line[BUFLEN_512], buf[BUFLEN_512];
   char dummy[BUFLEN_32];
   char rxByte[BUFLEN_32];
   char rxPacket[BUFLEN_32];   
   char rxMultiByte[BUFLEN_32];
   char rxPacketMulti[BUFLEN_32];
   char rxPacketUni[BUFLEN_32];
   char rxPacketBcast[BUFLEN_32];   
   char rxErr[BUFLEN_32];
   char rxDrop[BUFLEN_32];
   char txByte[BUFLEN_32];
   char txPacket[BUFLEN_32];   
   char txMultiByte[BUFLEN_32];
   char txPacketMulti[BUFLEN_32];
   char txPacketUni[BUFLEN_32];
   char txPacketBcast[BUFLEN_32];
   char txErr[BUFLEN_32];
   char txDrop[BUFLEN_32];
   char *pcDevNameStart;


   if (devName == NULL)
   {
      *byteRx = 0;
      *packetRx = 0;
      *byteMultiRx = 0;
      *packetMultiRx = 0;
      *packetUniRx = 0;
      *packetBcastRx = 0;      
      *errRx = 0;
      *dropRx = 0;
      *byteTx = 0;
      *packetTx = 0;      
      *byteMultiTx = 0;
      *packetMultiTx = 0;
      *packetUniTx = 0;
      *packetBcastTx = 0;      
      *errTx = 0;
      *dropTx = 0;
      return;
   }

   
   /* getstats put device statistics into this file, read the stats */
   /* Be sure to read the page with the extended stats */
   FILE* fs = fopen("/proc/net/dev_extstats", "r");
   if ( fs == NULL ) 
   {
      return;
   }

   // find interface
   while ( fgets(line, sizeof(line), fs) ) 
   {
      /* read pass 2 header lines */
      if ( count++ < 2 ) 
      {
         continue;
      }

      /* normally line will have the following example value
       * "eth0: 19140785 181329 0 0 0 0 0 0 372073009 454948 0 0 0 0 0 0"
       * but when the number is too big then line will have the following example value
       * "eth0:19140785 181329 0 0 0 0 0 0 372073009 454948 0 0 0 0 0 0"
       * so to make the parsing correctly, the following codes are added
       * to insert space between ':' and number
       */
      pChar = strchr(line, ':');
      if ( pChar != NULL )
      {
         pChar++;
      }
      if ( pChar != NULL && isdigit(*pChar) ) 
      {
         strcpy(buf, pChar);
         *pChar = ' ';
         strcpy(++pChar, buf);
      }

      /* Find and test the interface name to see if it's the one we want.
         If so, then store statistic values.	  */
      pcDevNameStart = strstr(line, devName);
      if ( (pcDevNameStart != NULL) && *(pcDevNameStart + strlen(devName)) == ':' )
      {
         sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
                dummy, rxByte, rxPacket, rxErr, rxDrop, dummy, dummy, dummy, rxPacketMulti,
                txByte, txPacket, txErr, txDrop, dummy, dummy, dummy, dummy,
                txPacketMulti, rxMultiByte, txMultiByte, rxPacketUni, txPacketUni, rxPacketBcast, txPacketBcast, dummy);  
         *byteRx= strtoull(rxByte, NULL, 10);
         *packetRx = strtoull(rxPacket, NULL, 10);
         *byteMultiRx = strtoull(rxMultiByte, NULL, 10);
         *packetMultiRx = strtoull(rxPacketMulti, NULL, 10);
         *packetUniRx = strtoull(rxPacketUni, NULL, 10);
         *packetBcastRx = strtoull(rxPacketBcast, NULL, 10); 
         *errRx = strtoull(rxErr, NULL, 10);
         *dropRx = strtoull(rxDrop, NULL, 10);
         *byteTx = strtoull(txByte, NULL, 10);
         *packetTx = strtoull(txPacket, NULL, 10);
         *byteMultiTx = strtoull(txMultiByte, NULL, 10);
         *packetMultiTx = strtoull(txPacketMulti, NULL, 10);
         *packetUniTx = strtoull(txPacketUni, NULL, 10);
         *packetBcastTx = strtoull(txPacketBcast, NULL, 10);
         *errTx = strtoull(txErr, NULL, 10);
         *dropTx = strtoull(txDrop, NULL, 10);
         
         /* Interface found - break out of while() loop */
         break;
      } /* devName */
   } /* while */
   
   fclose(fs);

   /* if any byte counter hits ULLONG_MAX then reset counters */
   if (*byteRx == ULLONG_MAX || *byteMultiRx == ULLONG_MAX ||
       *errRx == ULLONG_MAX || *dropRx == ULLONG_MAX ||
       *byteTx == ULLONG_MAX || *byteMultiTx == ULLONG_MAX ||
       *errTx == ULLONG_MAX || *dropTx == ULLONG_MAX)
   {
      rut_clearIntfStats(devName);
   }
}

void rut_clearIntfStats(const char *devname)
{
   int  socketFd;
   struct ifreq intf;

   if (cmsUtl_strlen(devname) == 0)
   {
      cmsLog_error("Cannot clear statistic since device name is NULL or empty.");
      return;
   }

   if ( (socketFd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) 
   {
      strncpy(intf.ifr_name, devname, sizeof(intf.ifr_name));
      ioctl(socketFd, SIOCSCLEARMIBCNTR, &intf);
      close(socketFd);
   }
   else
   {
      cmsLog_debug("Open socket error.");
   }
}

void rut_clearWanIntfStats(char *devname)
{
   int  socketFd;
   struct ifreq intf;

   if ( (socketFd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) 
   {
      strncpy(intf.ifr_name, devname, sizeof(intf.ifr_name));
      ioctl(socketFd, SIOCCIFSTATS, &intf);
      close(socketFd);
   } 
   else
   {
      cmsLog_debug("Open socket error.");
   }
}



void rutSys_createHostsFile(void)
{
   FILE *pFile;
   UBOOL8 ipv6Enabled = FALSE;

   cmsLog_debug("entered");

   /* mwang_todo: look into using the same write-to-temp file
    * and rename procedure used by resolv.conf.
    */
   if (NULL == (pFile = fopen("/var/hosts", "w+")))
   {
      cmsLog_error("Cannot access file /var/hosts!");
      return;
   }

   fprintf(pFile,"%s\t%s\n","127.0.0.1","localhost");

#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1
   {
      CmsRet ret;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      DnsProxyCfgObject *dnsProxyObj=NULL;
      char ifName[CMS_IFNAME_LENGTH]={0};
      char ipAddr[CMS_IPADDR_LENGTH]={0};

      ret = cmsObj_get(MDMOID_DNS_PROXY_CFG, &iidStack, 0, (void **) &dnsProxyObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get dnsproxycfg object, ret=%d", ret);
         fclose(pFile);
         return;
      }

      if ((ret = qdmIpIntf_getDefaultLanIntfNameLocked(ifName)) == CMSRET_SUCCESS)
      {
         if ((ret = qdmIpIntf_getIpv4AddressByNameLocked(ifName, ipAddr)) == CMSRET_SUCCESS)
         {
            fprintf(pFile, "%s\t%s\n", ipAddr, dnsProxyObj->deviceHostName);
            fprintf(pFile, "%s\t%s.%s\n",
                    ipAddr,
                    dnsProxyObj->deviceHostName, dnsProxyObj->deviceDomainName);
         }
      }

      cmsObj_free((void **) &dnsProxyObj);
   }
#endif

   rutDns_writeStaticHosts(pFile);

#ifdef DMP_X_BROADCOM_COM_IPV6_1
   {
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      InstanceIdStack wanDevIid = EMPTY_INSTANCE_ID_STACK;
      WanDevObject *wanDev = NULL;
      WanPppConnObject *pppCon = NULL;
      WanIpConnObject *ipCon = NULL;

      while (!ipv6Enabled &&
             (cmsObj_getNextFlags(MDMOID_WAN_DEV, &wanDevIid, OGF_NO_VALUE_UPDATE, (void **) &wanDev) == CMSRET_SUCCESS))
      {
         INIT_INSTANCE_ID_STACK(&iidStack);
         while (!ipv6Enabled &&
                (cmsObj_getNextInSubTree(MDMOID_WAN_IP_CONN, &wanDevIid, &iidStack, (void **) &ipCon) == CMSRET_SUCCESS))
         {
            ipv6Enabled = qdmIpIntf_isIpv6EnabledOnIntfNameLocked(ipCon->X_BROADCOM_COM_IfName);
            cmsObj_free((void **) &ipCon);
         }

         INIT_INSTANCE_ID_STACK(&iidStack);
         while (!ipv6Enabled &&
                (cmsObj_getNextInSubTree(MDMOID_WAN_PPP_CONN, &wanDevIid, &iidStack, (void **) &pppCon) == CMSRET_SUCCESS))
         {
            ipv6Enabled = qdmIpIntf_isIpv6EnabledOnIntfNameLocked(pppCon->X_BROADCOM_COM_IfName);
            cmsObj_free((void **) &pppCon);
         }

         cmsObj_free((void **) &wanDev);
      }
   }
#elif  DMP_X_BROADCOM_COM_DEV2_IPV6_1
   {
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      Dev2IpInterfaceObject *ipIntfObj = NULL;

      INIT_INSTANCE_ID_STACK(&iidStack);
      while (!ipv6Enabled &&
             (cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &iidStack, (void **) &ipIntfObj) == CMSRET_SUCCESS))
      {
         if (!ipIntfObj->X_BROADCOM_COM_Upstream)
         {
           cmsObj_free((void **)&ipIntfObj);
           continue;
         }

         ipv6Enabled = ipIntfObj->IPv6Enable;
         cmsObj_free((void **) &ipIntfObj);
      }
   }
#endif /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */


   if (ipv6Enabled)
   {
      fprintf(pFile,"%s\t%s\n","::1","ip6-localhost");
      fprintf(pFile,"%s\t%s\n","ff02::1","ip6-allnodes");
      fprintf(pFile,"%s\t%s\n","ff02::2","ip6-allrouters");
   }


   fclose(pFile);

   return;
}

/* Read current running configuration file */
char* rutSys_getRunningConfigFile(void)
{
   char *pBuf;
   UINT32 bufLen = 0;
   CmsRet ret;

   bufLen = cmsImg_getConfigFlashSize();
   cmsLog_debug("enter, configLen=%d maxSizeOf this parameter = %d\n", bufLen,MAX_DEVICE_CONFIG_CONFIGFILE_SIZE);

   if ((bufLen == 0) || (bufLen >= MAX_DEVICE_CONFIG_CONFIGFILE_SIZE))
   {
      return NULL;
   }
   if ((pBuf = cmsMem_alloc(bufLen,ALLOC_ZEROIZE)) == NULL)
   {
      return NULL;
   }

   ret = oal_readConfigFlashToBuf(CMS_CONFIG_PRIMARY,pBuf,&bufLen);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("read config failed, ret=%d bufLen=%d", ret, bufLen);
   }
   return (pBuf);
}

CmsRet rutSys_setRunningConfigFile(char *configBuf, int bufLen __attribute__((unused)))
{
   CmsRet ret;
   char *unescapedString=NULL;
   int unescapedLen=0;

   cmsXml_unescapeString(configBuf,&unescapedString);

   if (unescapedString != NULL)
   {
      unescapedLen = strlen(unescapedString);

      /* send message to SMD to flash the config file.  SMD validate buffer first before flashing */
      /* tr69c is currently holding the lock, release it for SMD, and then acquire the lock back. */
      cmsLck_releaseLock();
      ret = rut_sendMsgToSmd(CMS_MSG_VALIDATE_CONFIG_FILE,0,unescapedString,unescapedLen);
      if (ret != CMSRET_SUCCESS)
      {
         cmsMem_free(unescapedString);
         return (ret);
      }

      ret = rut_sendMsgToSmd(CMS_MSG_WRITE_CONFIG_FILE,0,unescapedString,unescapedLen);
      
      cmsLck_acquireLockWithTimeout(6*MSECS_IN_SEC);
      cmsMem_free(unescapedString);
      return CMSRET_SUCCESS;
   }
   return (CMSRET_INVALID_PARAM_VALUE);
}

char *rutSys_getDevicePersistentData(void)
{
   char *pData;
   int count;
   int maxLen = BUFLEN_256;

   if ((pData = cmsMem_alloc(maxLen,ALLOC_ZEROIZE)) == NULL)
   {
      return NULL;
   }

   count = cmsPsp_get(DEVICE_PERSISTENT_DATA_TOKEN, pData, maxLen);

   if (count == 0)
   {
      cmsLog_debug("No existing device persistent data found in scratch pad.");
   }
   else if (count > maxLen)
   {
      cmsLog_error("error while reading tr69c vendor config data from scratch pad, count=%d", count);
   }

   cmsLog_debug("data read from scratch pad %s, len = %d\n",pData,count);
   return (pData);
}

void rutSys_setDevicePersistentData(char *data, int dataLen)
{
   if ((data == NULL) || (data[0] == 0))
   {
      cmsPsp_set(DEVICE_PERSISTENT_DATA_TOKEN,NULL,0);
   }
   else 
   {
      cmsPsp_set(DEVICE_PERSISTENT_DATA_TOKEN,data,dataLen);
   }
}


UINT32 rutSys_getNumCpuThreads(void)
{
   UINT32 count=0;
   char line[BUFLEN_512];
   FILE *fs;

   fs = fopen("/proc/cpuinfo", "r");
   if ( fs == NULL )
   {
      cmsLog_error("Could not open /proc/cpuinfo, return 1");
      return 1;
   }

   while ( fgets(line, sizeof(line), fs) )
   {
      if (0 == strncmp(line, "processor", 9))
      {
         count++;
      }
   } /* while */

   fclose(fs);

   if (count == 0)
   {
      cmsLog_error("could not find any processors, return 1");
      return 1;
   }

   return count;
}

CmsRet rutSys_getCpuInfo(UINT32 id, UINT32 *frequency, char   *architecture)
{
   CmsRet ret = CMSRET_OBJECT_NOT_FOUND;
   UINT32 num = 0, i = 0;
   char line[BUFLEN_512];
   char *pChar = NULL;
   FILE *fs = NULL;

   if (architecture == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   *frequency = 0;

   fs = fopen("/proc/cpuinfo", "r");
   if (fs == NULL)
   {
      cmsLog_error("Could not open /proc/cpuinfo");
      return ret;
   }

   while ( fgets(line, sizeof(line), fs) )
   {
      if (ret != CMSRET_SUCCESS)
      {
         // processor line mark the beginning of one processor
         if (strncasecmp(line, "processor", 9) == 0 &&
             (pChar = strstr(line, ":")) != NULL &&
             (pChar + 2) != NULL)
         {
            // pChar+2: read pass ": "
            num = (UINT32) strtoul(pChar+2, (char **)NULL, 10);
            // process id is started from 0
            if (num == id)
            {
               ret = CMSRET_SUCCESS;
            }
         }
      }
      else
      {
         // convert line to uppercase to avoid using strcasestr
         for (i = 0; line[i] != '\0'; i++)
            line[i] = toupper(line[i]);

         if (strstr(line, "BMIPS") != NULL)
            strncpy(architecture, MDMVS_MIPSEB, BUFLEN_32);
         else if (strstr(line, "LMIPS") != NULL)
            strncpy(architecture, MDMVS_MIPSEL, BUFLEN_32);
         else if (strstr(line, "ARM") != NULL)
            strncpy(architecture, MDMVS_ARM, BUFLEN_32);
         else if (strstr(line, "I386") != NULL)
            strncpy(architecture, MDMVS_I386, BUFLEN_32);
         else if ((strstr(line, "BOGOMIPS") != NULL) &&
                  ((pChar = strstr(line, ":")) != NULL))
         {
            // pChar+2: read pass ": "
            if ((pChar + 2) != NULL)
            {
               *frequency = (UINT32) strtoul(pChar+2, (char **)NULL, 10);
               break;
            }
         }
      }
   } /* while */
         
   fclose(fs);

   return ret;
}


CmsRet rutSys_getUuidFromBbfDataModel(UINT32 len, char *uuid)
{
   char line[BUFLEN_512];
   char *pc = NULL;
   UINT32 count = 0, UUID_LENGTH = 36;
   FILE *fs = NULL;
   CmsRet ret = CMSRET_OBJECT_NOT_FOUND;

   if (uuid == NULL)
   {
      cmsLog_error("UUID is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (len < UUID_LENGTH)
   {
      cmsLog_error("Length of UUID %d is smaller than %d", len, UUID_LENGTH);
      return CMSRET_INVALID_ARGUMENTS;
   }

   fs = fopen(BBF_DATA_MODEL_FILE, "r");
   if ( fs == NULL )
   {
      cmsLog_error("Could not open %s", BBF_DATA_MODEL_FILE);
      return CMSRET_FILE_TRANSFER_UNABLE_ACCESS_FILE;
   }

   // uuid attribute should be found in
   // document element at line 2 (use 5 generously)
   while ( count < 5 &&
           fgets(line, sizeof(line), fs) )
   {
      if ((pc = strstr(line, "uuid=\"")) != NULL)
      {
         strncpy(uuid, pc+6, UUID_LENGTH);
         ret = CMSRET_SUCCESS;
         break;
      }

      count++;
   } /* while */

   fclose(fs);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not find UUID information");
   }

   return ret;
}


CmsRet rutSys_getFeaturesFromDataModel(UINT32 len, char *features)
{
   UINT32 size = 0;
   char buf[BUFLEN_1024];
   CmsRet ret = CMSRET_SUCCESS;

   if (features == NULL)
   {
      cmsLog_error("Features is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   memset(buf, 0, BUFLEN_1024);

#if defined(SUPPORT_DNSPROXY) || defined(SUPPORT_DNSPROBE) || defined(SUPPORT_DNSPROXYWITHPROBE)
   strcat(buf, "DNSServer,");
#endif

#ifdef DMP_DEVICE2_SIMPLEFIREWALL_1
   strcat(buf, "Firewall,");
#endif

#ifdef SUPPORT_IPV6
   strcat(buf, "IPV6,");
#endif

#ifdef DMP_DEVICE2_NAT_1
   strcat(buf, "NAT,");
#endif

#ifdef DMP_DEVICE2_ROUTING_1
   strcat(buf, "Routing,");
#endif

   size = strlen(buf);

   if (size > 0)
   {
      // remove the last ','
      buf[--size] = '\0';
      size = (len > size) ? size : len;
      strncpy(features, buf, size);
   }

   return ret;
}
