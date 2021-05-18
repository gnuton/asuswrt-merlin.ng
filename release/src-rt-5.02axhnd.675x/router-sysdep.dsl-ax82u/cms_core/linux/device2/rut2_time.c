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

#ifdef DMP_DEVICE2_BASELINE_1


#ifdef DMP_DEVICE2_TIME_1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "cms_core.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "rut2_time.h"

#ifdef SUPPORT_SNTP
void rutSntp_restart_dev2(void)
{
   Dev2TimeObject *timeObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char servers[BUFLEN_1024*2], cmd[BUFLEN_1024*4];

   if (cmsObj_get(MDMOID_DEV2_TIME, &iidStack, 0, (void **) &timeObj) == CMSRET_SUCCESS)
   {
      servers[0] = '\0';

      if(timeObj->NTPServer1 != NULL && timeObj->NTPServer1[0] != '\0' )
      {
         sprintf(servers, "-s %s", timeObj->NTPServer1);
         if(timeObj->NTPServer2 != NULL && timeObj->NTPServer2[0] != '\0' )
         {
            strcat(servers, " -s " );
            strcat(servers, timeObj->NTPServer2);
         }
         if(timeObj->NTPServer3 != NULL && timeObj->NTPServer3[0] != '\0' )
         {
            strcat(servers, " -s " );
            strcat(servers, timeObj->NTPServer3);
         }
         if(timeObj->NTPServer4 != NULL && timeObj->NTPServer4[0] != '\0' )
         {
            strcat(servers, " -s " );
            strcat(servers, timeObj->NTPServer4);
         }
         if(timeObj->NTPServer5 != NULL && timeObj->NTPServer5[0] != '\0' )
         {
            strcat(servers, " -s " );
            strcat(servers, timeObj->NTPServer5);
         }

         sprintf(cmd, "%s -t %s", servers, timeObj->X_BROADCOM_COM_LocalTimeZoneName);
         if (rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_SNTP, cmd, strlen(cmd)+1) == CMS_INVALID_PID)
         {
            cmsLog_error("failed to start or restart sntp");
         }
      }
      cmsLog_error("TZ:%s timezone:%s", timeObj->localTimeZone, timeObj->X_BROADCOM_COM_LocalTimeZoneName);

      cmsObj_free((void **) &timeObj);
   }
}


void rutSntp_stop_dev2(void)
{
   rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_SNTP, NULL, 0);
}

#endif  /* SUPPORT_SNTP */

#ifdef SUPPORT_NTPD
void rutNtpd_restart_dev2(void)
{
   Dev2TimeObject *timeObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char servers[BUFLEN_1024*2], cmd[BUFLEN_1024*4];

   if (cmsObj_get(MDMOID_DEV2_TIME, &iidStack, 0, (void **) &timeObj) == CMSRET_SUCCESS)
   {
      servers[0] = '\0';

      if(timeObj->NTPServer1 != NULL && timeObj->NTPServer1[0] != '\0' )
      {
         sprintf(servers, "-p %s", timeObj->NTPServer1);
         if(timeObj->NTPServer2 != NULL && timeObj->NTPServer2[0] != '\0' )
         {
            strcat(servers, " -p " );
            strcat(servers, timeObj->NTPServer2);
         }
         if(timeObj->NTPServer3 != NULL && timeObj->NTPServer3[0] != '\0' )
         {
            strcat(servers, " -p " );
            strcat(servers, timeObj->NTPServer3);
         }
         if(timeObj->NTPServer4 != NULL && timeObj->NTPServer4[0] != '\0' )
         {
            strcat(servers, " -p " );
            strcat(servers, timeObj->NTPServer4);
         }
         if(timeObj->NTPServer5 != NULL && timeObj->NTPServer5[0] != '\0' )
         {
            strcat(servers, " -p " );
            strcat(servers, timeObj->NTPServer5);
         }

         sprintf(cmd, "-n %s", servers);
         if (rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_NTPD, cmd, strlen(cmd)+1) == CMS_INVALID_PID)
         {
            cmsLog_error("failed to start or restart ntpd");
         }
      }

      if (timeObj->X_BROADCOM_COM_LocalTimeZoneName)
      {
         int i = 0;
         char fileName[64] = {0}, *ch = timeObj->X_BROADCOM_COM_LocalTimeZoneName;
         while (*ch != '\0' && *ch != ',')
         {
            if (*ch != ' ')
               fileName[i] = *ch;
            else
               fileName[i] = '_';
            ch ++;
            i ++;
         }
         sprintf(cmd, "cp /etc/zoneinfo/%s /var/localtime", fileName);
         rut_doSystemAction("rut", cmd);
      }
      else if (timeObj->localTimeZone)
      {
         sprintf(cmd, "cp /etc/zoneinfo/%s /var/localtime", timeObj->localTimeZone);
         rut_doSystemAction("rut", cmd);
      }
      cmsObj_free((void **) &timeObj);
   }
}


void rutNtpd_stop_dev2(void)
{
   rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_NTPD, NULL, 0);
}

#endif  /* SUPPORT_NTPD */


UBOOL8 rutTime_isAllRequiredValuesPresent_dev2(const _Dev2TimeObject *timeObj)
{
   UBOOL8 ret = FALSE;

   if (timeObj->X_BROADCOM_COM_LocalTimeZoneName != NULL && timeObj->NTPServer1 != NULL)
   {
      ret = TRUE;
   }

   return (ret);
}


UBOOL8 rutTime_isValuesChanged_dev2(const _Dev2TimeObject *newObj, const _Dev2TimeObject *currObj)
{
   UBOOL8 ret = FALSE;

   if (cmsUtl_strcmp(newObj->NTPServer1, currObj->NTPServer1) ||
       cmsUtl_strcmp(newObj->NTPServer2, currObj->NTPServer2) ||
       cmsUtl_strcmp(newObj->NTPServer3, currObj->NTPServer3) ||
       cmsUtl_strcmp(newObj->NTPServer4, currObj->NTPServer4) ||
       cmsUtl_strcmp(newObj->NTPServer5, currObj->NTPServer5) ||
       cmsUtl_strcmp(newObj->localTimeZone, currObj->localTimeZone) ||
       cmsUtl_strcmp(newObj->X_BROADCOM_COM_LocalTimeZoneName, currObj->X_BROADCOM_COM_LocalTimeZoneName) ||
       cmsUtl_strcmp(newObj->X_BROADCOM_COM_DaylightSavingsStart, currObj->X_BROADCOM_COM_DaylightSavingsStart) ||
       cmsUtl_strcmp(newObj->X_BROADCOM_COM_DaylightSavingsEnd, currObj->X_BROADCOM_COM_DaylightSavingsEnd) ||
       newObj->X_BROADCOM_COM_DaylightSavingsUsed != currObj->X_BROADCOM_COM_DaylightSavingsUsed)
   {
      ret = TRUE;
   }

   return (ret);
}

#endif  /* DMP_DEVICE2_TIME_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */

