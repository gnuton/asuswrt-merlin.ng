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

#ifdef DMP_X_BROADCOM_COM_DLNA_1

#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"

#define DMS_CONFIG_FILENAME  "/var/dms.conf"


CmsRet rcl_dlnaObject( _DlnaObject *newObj __attribute__((unused)),
                const _DlnaObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dmsCfgObject( _DmsCfgObject *newObj,
                const _DmsCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   DmsCfgObject *dmsObj = NULL;
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
         /* digital media server is not enabled. do nothing. */
         return CMSRET_SUCCESS;
      }

      dmsObj = newObj;
   }
   else if (newObj != NULL && currObj != NULL)
   {
      /* edit curr instance */
      if (newObj->enable == currObj->enable &&
          (cmsUtl_strcmp(newObj->mediaPath, currObj->mediaPath) == 0) && (newObj->brKey == currObj->brKey))
      {
         cmsLog_debug("no changes in parameters");
         return CMSRET_SUCCESS;
      }

      if (!newObj->enable && currObj->enable)
      {
         /* digital media server is disabled. stop dms. */
         rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_DMSD, NULL, 0);
         return CMSRET_SUCCESS;
      }

      dmsObj = newObj;   
   }
   else
   {
      /* unknown case */
      return CMSRET_SUCCESS;
   }

   if (dmsObj->enable && !IS_EMPTY_STRING(dmsObj->mediaPath))
   {
      UINT32 pid;
      FILE* fs;
      char cmdStr[BUFLEN_256];
      if ((fs = fopen(DMS_CONFIG_FILENAME, "w")) == NULL)
      {  
          cmsLog_error("Failed to open %s", DMS_CONFIG_FILENAME);
      }
      else
      {
/* /etc/dms.conf
* netInterface is the interface which the DMS needs to bind to
* extMediaDir is the content directory for file streaming
* UPnP_UCTT: The default is 0. To set it to 1 for UPnP certification(UCTT v2.0)
* CTT_TEST:  The default is 0. To set it to 1 for DLNA certification(DLNA v1.5) or UPnP certification(UCTT v2.0)
* uuid is the device unique GUID for the DMS
* friendlyName is the short description for end user. Should be localized and < 64 characters.
* manufacturer is the manufacturer's name. May be localized. Should be < 64 characters.
* manufacturerUrl is the web site for Manufacturer. May be localized. May be relative to base URL.
* modelDescription is the long description for end user. Should be localized and < 128 characters.
* modelName is the model name. May be localized. Should be < 32 characters.
* modelNumber is the model number. May be localized. Should be < 32 characters.
* modelUrl is the web site for model. May be localized. May be relative to base URL.
* serialNumber is the serial number. May be localized. Should be < 64 characters.
* upc is the Universal Product Code. 12-digit, all-numeric code
* presentationUrl is the URL to presentation for device. May be relative to base URL.
*/
          snprintf(cmdStr, sizeof(cmdStr), "netInterface=br%d\n", dmsObj->brKey);
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "extMediaDir=%s\n", dmsObj->mediaPath);
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "ssdpRefresh=1800\n");
          fputs(cmdStr, fs);
          // snprintf(cmdStr, sizeof(cmdStr), "debug_log=/tmp/dms.log\ndebug_param=DMS.c:*;dirdbase.c:rescan_dummy_dbase,create_dummy_dbase\nmultiProcessEnv=n\n");
          // fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "UPnP_UCTT=0\nCTT_TEST=0\nSupportXbox=1\n");
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "fileExtension=jpg:jpeg:jpe:png:wav:mp3:wma:m1a:m2a:m4a:mpa:mp4:ts:wmv:mpg:mpv:mpeg:asf:avi:divx:flv:mkv:m2ts:vob:m1v:m2v:m4v:mov:3gp:pcm:tts\n");
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "friendlyName=Broadcom Digital Media Server\n");
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "manufacturer=Broadcom\n");
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "manufacturerUrl=http://www.broadcom.com\n");
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "modelDescription=Broadcom Digital Media Server Ver 2.1.2\n");
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "modelName=Settop Box Revision\n");
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "modelNumber=BcmXXXX\n");
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "modelUrl=http://www.broadcom.com\n");
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "serialNumber=12345678\n");
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "upc=123456789012\n");
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "presentationUrl=/webs/html_root/index.html\n");
          fputs(cmdStr, fs);
          snprintf(cmdStr, sizeof(cmdStr), "ServicePort=%d\n", dmsObj->servicePort);
          fputs(cmdStr, fs);
          /* Please add your own uuid generator here. It MUST be per device difference.*/
          // snprintf(cmdStr, sizeof(cmdStr), "uuid=uuid:1c722efe-1dd2-11b2-9f1e-9469546d66ff\n");
          // fputs(cmdStr, fs);
          fclose(fs);
      }

      if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_DMSD, NULL, 0)) == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start or restart dms");
         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("restarting dms, pid=%d", pid);
      }
   }

   return CMSRET_SUCCESS;

}  /* End of rcl_dmsCfgObject() */

#endif  /* DMP_X_BROADCOM_COM_DLNA_1 */
