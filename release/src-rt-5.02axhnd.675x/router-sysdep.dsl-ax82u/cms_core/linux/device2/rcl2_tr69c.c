/***********************************************************************
 *
 *  Copyright (c) 2006-2013  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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


#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"

#ifdef SUPPORT_TR69C
/* because this object is a baseline object, it is mostly included.  And if TR69C is disabled, the functions 
 * should be empty.
 */
CmsRet rcl_dev2ManagementServerObject( _Dev2ManagementServerObject *newObj,
                                       const _Dev2ManagementServerObject *currObj,
                                       const InstanceIdStack *iidStack __attribute((unused)),
                                       char **errorParam __attribute((unused)),
                                       CmsRet *errorCode __attribute((unused)))
{
   /* modify supported conn req methods if xmpp is enabled */
#ifdef SUPPORT_XMPP
   if (ADD_NEW(newObj, currObj))
   {
      if (strstr(newObj->supportedConnReqMethods,"XMPP") == NULL)
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->supportedConnReqMethods, "HTTP, XMPP", mdmLibCtx.allocFlags);
      }
      #ifdef SUPPORT_STUN
      if (strstr(newObj->supportedConnReqMethods,"STUN") == NULL)
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->supportedConnReqMethods, "HTTP, XMPP, STUN", mdmLibCtx.allocFlags);
      }
      #endif
   }
#endif

   /* detect change in management server object */
   if (newObj != NULL && currObj != NULL)
   {
      UBOOL8 b = (cmsUtl_strcmp(newObj->URL, currObj->URL) ||
                  cmsUtl_strcmp(newObj->username, currObj->username) ||
                  cmsUtl_strcmp(newObj->password, currObj->password) ||
                  cmsUtl_strcmp(newObj->connectionRequestUsername, currObj->connectionRequestUsername) ||
                  cmsUtl_strcmp(newObj->connectionRequestPassword, currObj->connectionRequestPassword) ||
                  cmsUtl_strcmp(newObj->X_BROADCOM_COM_BoundIfName, currObj->X_BROADCOM_COM_BoundIfName) ||
                  (newObj->periodicInformEnable != currObj->periodicInformEnable) ||
                  (newObj->periodicInformInterval != currObj->periodicInformInterval));

      if (b == TRUE)
      {
         char *acsConfigId = MULTI_TR69C_CONFIG_INDEX_1; /* ACS config#1 changed */
         rut_sendAcsConfigChangedMsgToSmd(acsConfigId);
      }

      if (cmsUtl_strcmp(newObj->URL, currObj->URL) ||
          cmsUtl_strcmp(newObj->X_BROADCOM_COM_BoundIfName, currObj->X_BROADCOM_COM_BoundIfName))
      {
         /*
          * if one of these parameters have changed, then connection request URL
          * will likely change.  Free it and set it to null here.  That will cause
          * it to get filled in again next time somebody does a read of
          * the management server object.
          */
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->connectionRequestURL);
      }

      if (newObj->manageableDeviceNotificationLimit != currObj->manageableDeviceNotificationLimit)
      {
         /*
          * NotificationLimit is changed, send a NotificationLimit event msg to smd, then forwarded to registered process
          */
         CmsMsgHeader msg = EMPTY_MSG_HEADER;
         msg.type = CMS_MSG_MANAGEABLE_DEVICE_NOTIFICATION_LIMIT_CHANGED;
         msg.src =  mdmLibCtx.eid;
         msg.dst = EID_SMD;
         msg.wordData = newObj->manageableDeviceNotificationLimit;
         msg.flags_event = 1;
         if (cmsMsg_send(mdmLibCtx.msgHandle, &msg) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not send out MANAGEABLE_DEVICE_NOTIFICATION_LIMIT_CHANGED event msg");
         }
         else
         {
            cmsLog_debug("Sent out MANAGEABLE_DEVICE_NOTIFICATION_LIMIT_CHANGED event msg.");
         }
      } /* notificationLimit */

#if defined(SUPPORT_DM_PURE181) && defined(SUPPORT_STUN)
      b = ( cmsUtl_strcmp(newObj->STUNServerAddress, currObj->STUNServerAddress) ||
            cmsUtl_strcmp(newObj->STUNUsername, currObj->STUNUsername) ||
            cmsUtl_strcmp(newObj->STUNPassword, currObj->STUNPassword) ||
            (newObj->STUNServerPort != currObj->STUNServerPort) ||
            (newObj->STUNEnable != currObj->STUNEnable) ||
            (newObj->STUNMaximumKeepAlivePeriod != currObj->STUNMaximumKeepAlivePeriod) ||
            (newObj->STUNMinimumKeepAlivePeriod != currObj->STUNMinimumKeepAlivePeriod));      
      if (b == TRUE)
      {
         /*
          * Since STUN parameters changed, 
          * send a STUN_CONFIG_CHANGED event msg to smd, then forwarded to registered process
          */
         CmsMsgHeader msg = EMPTY_MSG_HEADER;
         msg.type = CMS_MSG_STUN_CONFIG_CHANGED;
         msg.src =  mdmLibCtx.eid;
         msg.dst = EID_SMD;
         msg.flags_event = 1;
         if (cmsMsg_send(mdmLibCtx.msgHandle, &msg) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not send out STUN_CONFIG_CHANGED event msg");
         }
         else
         {
            cmsLog_debug("Send out STUN_CONFIG_CHANGED event msg.");
         }
      }      
#endif
   }
   /* This object cannot be deleted, so no need to handle that case. */

   return CMSRET_SUCCESS;
}

#ifdef DMP_DEVICE2_DEVICEASSOCIATION_1
CmsRet rcl_dev2ManagementServerManageableDeviceObject(_Dev2ManagementServerManageableDeviceObject *newObj,
                                                      const _Dev2ManagementServerManageableDeviceObject *currObj,
                                                      const InstanceIdStack *iidStack,
                                                      char **errorParam __attribute((unused)),
                                                      CmsRet *errorCode __attribute((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rut_modifyNumManageableDevices_dev2(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rut_modifyNumManageableDevices_dev2(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}
#endif  /* DMP_DEVICE2_DEVICEASSOCIATION_1 */

#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE
CmsRet rcl_dev2AutonXferCompletePolicyObject( _Dev2AutonXferCompletePolicyObject *newObj __attribute__((unused)), 
                                              const _Dev2AutonXferCompletePolicyObject *currObj __attribute__((unused)),
                                              const InstanceIdStack *iidStack __attribute__((unused)),
                                              char **errorParam __attribute__((unused)),
                                              CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#endif /* SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE */

#ifdef DMP_DEVICE2_DUSTATECHNGCOMPLPOLICY_1
CmsRet rcl_dev2DuStateChangeComplPolicyObject( _Dev2DuStateChangeComplPolicyObject *newObj __attribute__((unused)),
                                               const _Dev2DuStateChangeComplPolicyObject *currObj __attribute__((unused)),
                                               const InstanceIdStack *iidStack __attribute__((unused)),
                                               char **errorParam __attribute__((unused)),
                                               CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

#else 
/* TR69 is not supported */
#ifndef SUPPORT_RETAIL_DM
/* retail data model has even the management object stripped from baseline profile */
CmsRet rcl_dev2ManagementServerObject( _Dev2ManagementServerObject *newObj,
                                       const _Dev2ManagementServerObject *currObj,
                                       const InstanceIdStack *iidStack __attribute((unused)),
                                       char **errorParam __attribute((unused)),
                                       CmsRet *errorCode __attribute((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

#ifdef DMP_DEVICE2_DEVICEASSOCIATION_1
CmsRet rcl_dev2ManagementServerManageableDeviceObject(_Dev2ManagementServerManageableDeviceObject *newObj,
                                                      const _Dev2ManagementServerManageableDeviceObject *currObj,
                                                      const InstanceIdStack *iidStack,
                                                      char **errorParam __attribute((unused)),
                                                      CmsRet *errorCode __attribute((unused)))
{
   return CMSRET_SUCCESS;
}
#endif  /* DMP_DEVICE2_DEVICEASSOCIATION_1 */

#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE
CmsRet rcl_dev2AutonXferCompletePolicyObject( _Dev2AutonXferCompletePolicyObject *newObj __attribute__((unused)), 
                                              const _Dev2AutonXferCompletePolicyObject *currObj __attribute__((unused)),
                                              const InstanceIdStack *iidStack __attribute__((unused)),
                                              char **errorParam __attribute__((unused)),
                                              CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#endif /* SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE */

#ifdef DMP_DEVICE2_DUSTATECHNGCOMPLPOLICY_1
CmsRet rcl_dev2DuStateChangeComplPolicyObject( _Dev2DuStateChangeComplPolicyObject *newObj __attribute__((unused)),
                                               const _Dev2DuStateChangeComplPolicyObject *currObj __attribute__((unused)),
                                               const InstanceIdStack *iidStack __attribute__((unused)),
                                               char **errorParam __attribute__((unused)),
                                               CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

#endif /* SUPPORT_TR69C */


#endif    /* DMP_DEVICE2_BASELINE_1 */

