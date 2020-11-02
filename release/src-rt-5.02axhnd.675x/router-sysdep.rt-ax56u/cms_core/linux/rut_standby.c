#ifdef DMP_X_BROADCOM_COM_STANDBY_1
/***********************************************************************
 *
 *  Copyright (c) 2008-2010  Broadcom Corporation
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

#include "rcl.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "cms_boardioctl.h"
#include "mdm_validstrings.h"

CmsRet rut_standby(_StandbyCfgObject *newObj, const _StandbyCfgObject *currObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   unsigned int secondsStandbyTime;
   unsigned int secondsWakeupTime;

   // Only go to standby if this was a clean transition from Waiting for Standby to Waiting for Wakeup
   if ((0 == cmsUtl_strcmp(currObj->standbyStatusString, MDMVS_WAITING_FOR_STANDBY_TIME)) &&
       (0 == cmsUtl_strcmp( newObj->standbyStatusString, MDMVS_WAITING_FOR_WAKE_UP_TIME))) {

      // Convert time to seconds to make calculations easy
      secondsStandbyTime = newObj->standbyHour*24*60 + newObj->standbyMinutes*60;
      secondsWakeupTime = newObj->wakeupHour*24*60 + newObj->wakeupMinutes*60;

      // Adjust time such that they can be compared
      if (secondsWakeupTime < secondsStandbyTime) {
         secondsWakeupTime += 24*60*60;
      }

      // Program standby duration
      if (CMSRET_SUCCESS == (ret = devCtl_boardIoctl(BOARD_IOCTL_SET_STANDBY_TIMER, 0, NULL, 0, (secondsWakeupTime - secondsStandbyTime)/10, NULL))) {

         // Program shutdown to go to standby
         if (CMSRET_SUCCESS == (ret = devCtl_boardIoctl(BOARD_IOCTL_SET_SHUTDOWN_MODE, 0, NULL, 0, 0, NULL))) {
            // Do a clean shutdown
            cmsUtil_sendRequestRebootMsg(mdmLibCtx.msgHandle);
         }
      }
   }
   return ret;
} /* rcl_standbyCfgObject */


#endif 
