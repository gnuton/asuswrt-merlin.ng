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

#ifdef DMP_TIME_1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "cms_msg.h"
#include "rut_time.h"

#ifdef SUPPORT_SNTP
void rutSntp_restart(void)
{
   TimeServerCfgObject *timeObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char servers[BUFLEN_1024*2], cmd[BUFLEN_1024*4];

   if (cmsObj_get(MDMOID_TIME_SERVER_CFG, &iidStack, 0, (void **) &timeObj) == CMSRET_SUCCESS)
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

         sprintf(cmd, "%s -t %s", servers, timeObj->localTimeZoneName);
         if (rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_SNTP, cmd, strlen(cmd)+1) == CMS_INVALID_PID)
         {
            cmsLog_error("failed to start or restart sntp");
         }
     }

      cmsObj_free((void **) &timeObj);
   }
}


void rutSntp_stop(void)
{
   rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_SNTP, NULL, 0);
}
#endif /* SUPPORT_SNTP */

#ifdef SUPPORT_NTPD
void rutNtpd_restart(void)
{
   TimeServerCfgObject *timeObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char servers[BUFLEN_1024*2], cmd[BUFLEN_1024*4];

   if (cmsObj_get(MDMOID_TIME_SERVER_CFG, &iidStack, 0, (void **) &timeObj) == CMSRET_SUCCESS)
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
            cmsLog_error("failed to start or restart sntp");
         }
      }

      if (timeObj->localTimeZoneName)
      {
         int i = 0;
         char fileName[64] = {0}, *ch = timeObj->localTimeZoneName;
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


void rutNtpd_stop(void)
{
   rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_NTPD, NULL, 0);
}
#endif /* SUPPORT_NTPD */

UBOOL8 rutTime_isAllRequiredValuesPresent(const _TimeServerCfgObject *timeObj)
{
   UBOOL8 ret = FALSE;

   if (timeObj->localTimeZoneName != NULL && timeObj->NTPServer1 != NULL)
   {
      ret = TRUE;
   }

   return (ret);
}


UBOOL8 rutTime_isValuesChanged(const _TimeServerCfgObject *newObj, const _TimeServerCfgObject *currObj)
{
   UBOOL8 ret = FALSE;

   if (cmsUtl_strcmp(newObj->NTPServer1, currObj->NTPServer1) ||
       cmsUtl_strcmp(newObj->NTPServer2, currObj->NTPServer2) ||
       cmsUtl_strcmp(newObj->NTPServer3, currObj->NTPServer3) ||
       cmsUtl_strcmp(newObj->NTPServer4, currObj->NTPServer4) ||
       cmsUtl_strcmp(newObj->NTPServer5, currObj->NTPServer5) ||
       cmsUtl_strcmp(newObj->localTimeZone, currObj->localTimeZone) ||
       cmsUtl_strcmp(newObj->localTimeZoneName, currObj->localTimeZoneName) ||
       cmsUtl_strcmp(newObj->daylightSavingsStart, currObj->daylightSavingsStart) ||
       cmsUtl_strcmp(newObj->daylightSavingsEnd, currObj->daylightSavingsEnd) ||
       newObj->daylightSavingsUsed != currObj->daylightSavingsUsed)
   {
      ret = TRUE;
   }

   return (ret);
}

#endif   // DMP_TIME_1 

