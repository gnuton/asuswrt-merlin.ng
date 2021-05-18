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

#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"

#ifdef DMP_X_BROADCOM_COM_SNMP_1

extern void rutSnmp_setTrapCode(void *data, int dataLen);

CmsRet rcl_snmpCfgObject( _SnmpCfgObject *newObj __attribute__((unused)),
                const _SnmpCfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   int pid;
   int configChanged=0;
   int statusChanged = 0;
   int coldStartTrap = 0;
   int warmStartTrap = 1;

   /* at start up, object creation time, if SNMP is enable,
      start SNMPD.  */

   cmsLog_debug("Ended: newObj %p, currObj %p",newObj,currObj);

   if (newObj != NULL && currObj == NULL)
   {
      rutSnmp_setTrapCode((void*)&warmStartTrap,sizeof(int));
      if (cmsUtl_strcmp(newObj->status, MDMVS_ENABLED) == 0)
      {
         pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_SNMPD, NULL, 0);
         if (pid == CMS_INVALID_PID)
         {
            cmsLog_error("failed to start SNMP at boot up");
            return CMSRET_INTERNAL_ERROR;
         }
         else
         {
            cmsLog_debug("started SNMPD at bootup, pid=%d", pid);
         }
      }
   }
   else if (newObj != NULL && currObj != NULL)
   {
      /* detect change in SNMP configuration object */
      if ( (cmsUtl_strcmp(newObj->ROCommunity, currObj->ROCommunity)) ||
           (cmsUtl_strcmp(newObj->RWCommunity, currObj->RWCommunity)) ||
           (cmsUtl_strcmp(newObj->trapIPAddress, currObj->trapIPAddress)) ||
           (cmsUtl_strcmp(newObj->sysName, currObj->sysName)) ||
           (cmsUtl_strcmp(newObj->sysLocation, currObj->sysLocation)) ||
           (cmsUtl_strcmp(newObj->sysContact, currObj->sysContact)) )
      {
         /* cold trap needs to be sent */
         configChanged = 1;
      }
      if (configChanged)
      {
         rutSnmp_setTrapCode((void*)&coldStartTrap,sizeof(int));
      }
      else
      {
         rutSnmp_setTrapCode((void*)&warmStartTrap,sizeof(int));
      }

      if (cmsUtl_strcmp(newObj->status, currObj->status))
      {
         statusChanged = 1;
         if (cmsUtl_strcmp(newObj->status, MDMVS_DISABLED) == 0)
         {
            cmsLog_debug("stopping SNMPD");
            rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_SNMPD, NULL, 0);
            return CMSRET_SUCCESS;
         }
      }
      /* SNMPD can not restart itself. */
      if ((mdmLibCtx.eid != EID_SNMPD) && (configChanged || statusChanged))
      {
         pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_SNMPD, NULL, 0);
         if (pid == CMS_INVALID_PID)
         {
            cmsLog_error("failed to start or restart SNMP");
            return CMSRET_INTERNAL_ERROR;
         }
         else
         {
            cmsLog_debug("restarting SNMPD, pid=%d", pid);
         }
      } /* enabled or changed in configuration */
   } /* change in configuration */

   /* This object cannot be deleted, so no need to handle that case. */
   return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_SNMP_1 */
