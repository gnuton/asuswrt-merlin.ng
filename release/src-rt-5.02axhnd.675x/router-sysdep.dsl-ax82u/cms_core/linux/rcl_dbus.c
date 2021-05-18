/***********************************************************************
 *
 *  Copyright (c) 2016  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2016:proprietary:standard 

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

#include "cms_util.h" 
#include "rcl.h"
#include "rut_util.h"

#ifdef DMP_X_BROADCOM_COM_DBUSREMOTE_1

#define DBUS_REMOTE_CONFIG_FILE  "/share/dbus-1/system.d/dfeetenbl.conf"

void rutDbus_setListenTcpPort(UBOOL8 remoteEnabled, UINT32 tcpPort)
{
   FILE *fp;
   char cmdLine[BUFLEN_128] = {0};

   const char *dbusConf = "\
<!-- This configuration file controls the systemwide message bus, support more TCP address. -->\n\
<busconfig>\n\
<listen>tcp:host=localhost,bind=*,port=%d,family=ipv4</listen> \n\
<auth>ANONYMOUS</auth>\n\
  <allow_anonymous/>\n\n\
  <policy context=\"default\">\n\
    <allow send_type=\"method_call\"/>\n\
  </policy>\n\
</busconfig>\n\
";

   cmsLog_debug("Enter remoteEnabled=%d, tcpPort=%d", remoteEnabled, tcpPort);
   if (remoteEnabled)
   {
      if ((fp = fopen(DBUS_REMOTE_CONFIG_FILE, "w")) == NULL)
      {
         /* error */
         cmsLog_error("failed to create %s\n", DBUS_REMOTE_CONFIG_FILE);
         return;
      }
      else
      {
         /* create dfeetenbl.conf */
         fprintf(fp, dbusConf, tcpPort);
         fclose(fp);
      }
   }
   else
   {
      snprintf(cmdLine, sizeof(cmdLine), "rm -f %s 2>/dev/null", DBUS_REMOTE_CONFIG_FILE);
      rut_doSystemAction("rutDbus_setListenTcpPort", cmdLine);
   }
   cmsLog_debug("Leave");
}

CmsRet rcl_dbusRemoteCfgObject( _DbusRemoteCfgObject *newObj,
      const _DbusRemoteCfgObject *currObj,
      const InstanceIdStack *iidStack __attribute__((unused)),
      char **errorParam __attribute__((unused)),
      CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   if (newObj != NULL && currObj != NULL)
   {
      if ((newObj->enable == currObj->enable) && 
            (newObj->port == currObj->port))
      {
         cmsLog_error("no changes in parameters");
         return CMSRET_SUCCESS;
      }
      else
      {
         cmsLog_debug("parameters changed, new.enable=%d, new.port=%d, curr.enable=%d, curr.port=%d", newObj->enable, newObj->port, currObj->enable, currObj->port);
         rutDbus_setListenTcpPort(newObj->enable, newObj->port);
      }
   }

   return ret;
}

#endif

