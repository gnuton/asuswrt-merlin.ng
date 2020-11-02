/***********************************************************************
 *
 *  Copyright (c) 2008  Broadcom Corporation
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

#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_time.h"

CmsRet rcl_timeServerCfgObject( _TimeServerCfgObject *newObj,
                         const _TimeServerCfgObject *currObj,
                         const InstanceIdStack *iidStack __attribute__((unused)),
                         char **errorParam __attribute__((unused)),
                         CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   
   /* add and enable ntp server, or enable existing ntp server */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Add or enable timeServerCfg entry: enable = %d, server1=%s, server2=%s, server3=%s, server4=%s, server5=%s, currTime=%s, zoneTime=%s, zoneName=%s, dsu=%d, dss=%s, dse=%s",
                   newObj->enable,
                   newObj->NTPServer1, newObj->NTPServer2, newObj->NTPServer3, newObj->NTPServer4, newObj->NTPServer5,
                   newObj->currentLocalTime, newObj->localTimeZone, newObj->localTimeZoneName, newObj->daylightSavingsUsed, 
                   newObj->daylightSavingsStart, newObj->daylightSavingsEnd);

      if (!rutTime_isAllRequiredValuesPresent(newObj))
      {
         return CMSRET_INVALID_ARGUMENTS;
      }

#if defined(SUPPORT_NTPD)
      rutNtpd_restart();
#elif defined(SUPPORT_SNTP)
      rutSntp_restart();
#endif
   }


   /* edit existing timeServerCfg */
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj) &&
            rutTime_isValuesChanged(newObj, currObj))
   {
      cmsLog_debug("Edit timeServerCfg entry: enable = %d, server1=%s, server2=%s, server3=%s, server4=%s, server5=%s, currTime=%s, zoneTime=%s, zoneName=%s, dsu=%d, dss=%s, dse=%s",
                   newObj->enable,
                   newObj->NTPServer1, newObj->NTPServer2, newObj->NTPServer3, newObj->NTPServer4, newObj->NTPServer5,
                   newObj->currentLocalTime, newObj->localTimeZone, newObj->localTimeZoneName, newObj->daylightSavingsUsed, 
                   newObj->daylightSavingsStart, newObj->daylightSavingsEnd);
	  
      if (!rutTime_isAllRequiredValuesPresent(newObj))
      {
         return CMSRET_INVALID_ARGUMENTS;
      }


#if defined(SUPPORT_NTPD)
      rutNtpd_restart();
#elif defined(SUPPORT_SNTP)
      rutSntp_restart();
#endif
   }
   
   /* remove dynamic dns, or disable existing dynamic dns */
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Delete or disable timeServerCfg entry: enable = %d, server1=%s, server2=%s, server3=%s, server4=%s, server5=%s, currTime=%s, zoneTime=%s, zoneName=%s, dsu=%d, dss=%s, dse=%s",
                   newObj->enable,
                   newObj->NTPServer1, newObj->NTPServer2, newObj->NTPServer3, newObj->NTPServer4, newObj->NTPServer5,
                   newObj->currentLocalTime, newObj->localTimeZone, newObj->localTimeZoneName, newObj->daylightSavingsUsed, 
                   newObj->daylightSavingsStart, newObj->daylightSavingsEnd);
	  
#if defined(SUPPORT_NTPD)
      rutNtpd_stop();
#elif defined(SUPPORT_SNTP)
      rutSntp_stop();
#endif
   }

   return ret;
}

#endif  // DMP_TIME_1

