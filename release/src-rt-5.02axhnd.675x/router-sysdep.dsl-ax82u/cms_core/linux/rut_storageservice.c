/***********************************************************************
 *
 *  Copyright (c) 2009  Broadcom Corporation
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

#ifdef DMP_STORAGESERVICE_1

#include <stdlib.h>
#include <sys/vfs.h>
#include <pwd.h>    /* for putpwent */
#include <time.h>   /* for time */
#include <crypt.h>  /* for crypt */

#include "odl.h"
#include "rcl.h"
#include "cms_util.h"
#include "cms_core.h"
#include "rut_util.h"



CmsRet rutStorage_updateLogicalVolumeStats(char *path, _LogicalVolumeObject *Obj)
{
   struct statfs fsStats;

   if(!path || !Obj)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   if(statfs(path,&fsStats) == 0)
   {
      Obj->capacity  = cmsFil_scaleSizetoKB(fsStats.f_blocks, fsStats.f_bsize)/KILOBYTE ;/*Mega Bytes*/
      Obj->usedSpace = Obj->capacity - (cmsFil_scaleSizetoKB(fsStats.f_bfree, fsStats.f_bsize)/KILOBYTE);/*Mega Bytes*/

      return CMSRET_SUCCESS;
   }
   else
   {
      return CMSRET_INTERNAL_ERROR;
   }

}

#ifdef SUPPORT_SAMBA
CmsRet rutStorage_addUserAccount(char *username, const char *password, char *homeDir)
{

   struct passwd pw;
   FILE *fsPwd = NULL;
   char cmd[BUFLEN_256];

   /* create a linux account for the user */

   fsPwd = fopen("/etc/passwd", "a");
   if ( fsPwd == NULL ) 
   {

      cmsLog_error("failed to open passwd file");
      return CMSRET_INTERNAL_ERROR;

   }
   pw.pw_name = username;
   pw.pw_passwd = cmsUtil_pwEncrypt(password, cmsUtil_cryptMakeSalt());
   pw.pw_uid = 0;
   pw.pw_gid = 0;
   pw.pw_gecos = "Normal User";
   pw.pw_dir = homeDir;
   pw.pw_shell = "/bin/sh";

   if(putpwent(&pw, fsPwd) < 0)
   {
      cmsLog_error("putpwent failed");
      fclose(fsPwd); 
      return CMSRET_INTERNAL_ERROR;
   }

   fclose(fsPwd); 
   
   /* add user to sambapasswd */
   //sprintf(cmd,"smbpasswd -s -a %s <<%s <<%s ",username,password,password);
   sprintf(cmd,"(echo %s; echo %s) | smbpasswd -as %s >/dev/null ",password,password,username);
   rut_doSystemAction("storage:useraccount", cmd);

   /* create a soft link in /var/samba/homes/ */
   sprintf(cmd,"ln -s %s /var/samba/homes/%s",homeDir,username);
   rut_doSystemAction("storage:useraccount", cmd);

   return CMSRET_SUCCESS;
}

CmsRet rutStorage_deleteUserAccount(const char *username, const char *homeDir)
{
   char cmd[BUFLEN_256];
   /*remove users linux account */
   sprintf(cmd,"deluser %s",username);
   rut_doSystemAction("storage:useraccount", cmd);

   /*remove user from sambapasswd */
   sprintf(cmd,"smbpasswd -x %s",username);
   rut_doSystemAction("storage:useraccount", cmd);

   /*remove users home dir */
   sprintf(cmd,"rm -rf %s",homeDir);
   rut_doSystemAction("storage:useraccount", cmd);

   /* remove  soft link in /var/samba/homes/ */
   sprintf(cmd,"rm /var/samba/homes/%s",username);
   rut_doSystemAction("storage:useraccount", cmd);

   return CMSRET_SUCCESS;
}
#endif /*SUPPORT_SAMBA*/

#endif
