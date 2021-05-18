/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
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

#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut2_time.h"

CmsRet rcl_dev2TimeObject( _Dev2TimeObject *newObj,
                const _Dev2TimeObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   
   /* add and enable NTP service, or enable existing  NTP service*/
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Add or enable time entry: enable = %d, server1=%s, server2=%s, server3=%s, server4=%s, server5=%s, currTime=%s, zoneTime=%s, zoneName=%s, dsu=%d, dss=%s, dse=%s",
newObj->enable,
newObj->NTPServer1, newObj->NTPServer2, newObj->NTPServer3, newObj->NTPServer4, newObj->NTPServer5,
newObj->currentLocalTime, newObj->localTimeZone, newObj->X_BROADCOM_COM_LocalTimeZoneName, newObj->X_BROADCOM_COM_DaylightSavingsUsed, 
newObj->X_BROADCOM_COM_DaylightSavingsStart, newObj->X_BROADCOM_COM_DaylightSavingsEnd);

      if (!rutTime_isAllRequiredValuesPresent_dev2(newObj))
      {
         return CMSRET_INVALID_ARGUMENTS;
      }

#if defined(SUPPORT_NTPD)
      rutNtpd_restart_dev2();
#elif defined(SUPPORT_SNTP)
      rutSntp_restart_dev2();
#endif
   }


   /* edit existing NTP service */
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj) &&
            rutTime_isValuesChanged_dev2(newObj, currObj))
   {
      cmsLog_debug("Edit time entry: enable = %d, server1=%s, server2=%s, server3=%s, server4=%s, server5=%s, currTime=%s, zoneTime=%s, zoneName=%s, dsu=%d, dss=%s, dse=%s",
newObj->enable,
newObj->NTPServer1, newObj->NTPServer2, newObj->NTPServer3, newObj->NTPServer4, newObj->NTPServer5,
newObj->currentLocalTime, newObj->localTimeZone, newObj->X_BROADCOM_COM_LocalTimeZoneName, newObj->X_BROADCOM_COM_DaylightSavingsUsed, 
newObj->X_BROADCOM_COM_DaylightSavingsStart, newObj->X_BROADCOM_COM_DaylightSavingsEnd);
	  
      if (!rutTime_isAllRequiredValuesPresent_dev2(newObj))
      {
         return CMSRET_INVALID_ARGUMENTS;
      }
	  
#if defined(SUPPORT_NTPD)
      rutNtpd_restart_dev2();
#elif defined(SUPPORT_SNTP)
      rutSntp_restart_dev2();
#endif
   }
   
   /* remove dynamic dns, or disable existing dynamic dns */
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Delete or disable time entry: enable = %d, server1=%s, server2=%s, server3=%s, server4=%s, server5=%s, currTime=%s, zoneTime=%s, zoneName=%s, dsu=%d, dss=%s, dse=%s",
newObj->enable,
newObj->NTPServer1, newObj->NTPServer2, newObj->NTPServer3, newObj->NTPServer4, newObj->NTPServer5,
newObj->currentLocalTime, newObj->localTimeZone, newObj->X_BROADCOM_COM_LocalTimeZoneName, newObj->X_BROADCOM_COM_DaylightSavingsUsed, 
newObj->X_BROADCOM_COM_DaylightSavingsStart, newObj->X_BROADCOM_COM_DaylightSavingsEnd);
	  
#if defined(SUPPORT_NTPD)
      rutNtpd_stop_dev2();
#elif defined(SUPPORT_SNTP)
      rutSntp_stop_dev2();
#endif
   }

   return ret;
}

#endif  /* DMP_DEVICE2_TIME_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */

