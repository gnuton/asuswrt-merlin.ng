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

#ifdef DMP_X_BROADCOM_COM_IPPRINTING_1

#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"


CmsRet rcl_ippCfgObject( _IppCfgObject *newObj,
                const _IppCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   const char *ippConfFile  = "/var/printers.ini";
   IppCfgObject *ippObj = NULL;
   CmsRet ret;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   if (newObj != NULL && currObj == NULL)
   {
      /* for single instance object, this is the startup condition */
      if (!newObj->enable)
      {
         /* printer server is not enabled. do nothing. */
         return CMSRET_SUCCESS;
      }

      ippObj = newObj;
   }
   else if (newObj != NULL && currObj != NULL)
   {
      /* edit curr instance */
      if (newObj->enable == currObj->enable &&
          cmsUtl_strcmp(newObj->make, currObj->make) == 0 &&
          cmsUtl_strcmp(newObj->name, currObj->name) == 0)
      {
         cmsLog_debug("no changes in parameters");
         return CMSRET_SUCCESS;
      }

      if (!newObj->enable && currObj->enable)
      {
         /* printer server is disabled. stop ippd. */
         rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_IPPD, NULL, 0);
         remove(ippConfFile);
         return CMSRET_SUCCESS;
      }

      ippObj = newObj;   
   }
   else
   {
      /* unknown case */
      return CMSRET_SUCCESS;
   }

   if (ippObj->enable && !IS_EMPTY_STRING(ippObj->make) && !IS_EMPTY_STRING(ippObj->name))
   {
      char cmdLine[BUFLEN_32];
      FILE *fp;
      UINT32 pid;

      /* open a new printers.ini file or truncate the existing file */
      if ((fp = fopen(ippConfFile, "w")) == NULL)
      {
         /* error */
         cmsLog_error("failed to open %s", ippConfFile);
         return CMSRET_INTERNAL_ERROR;
      }

      fprintf(fp, "[%s]\nmake=%s\ndevice=%s\n", ippObj->name, ippObj->make, ippObj->device);
      fclose(fp);

      snprintf(cmdLine, sizeof(cmdLine), "%s", ippConfFile);
      if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_IPPD, cmdLine, strlen(cmdLine)+1)) == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start or restart ippd");
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("restarting ippd, pid=%d", pid);
      }
   }

   return CMSRET_SUCCESS;

}  /* End of rcl_ippCfgObject() */

#endif /* DMP_X_BROADCOM_COM_IPPRINTING_1 */
