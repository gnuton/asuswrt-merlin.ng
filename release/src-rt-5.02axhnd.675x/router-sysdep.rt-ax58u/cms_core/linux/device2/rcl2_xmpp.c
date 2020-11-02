/***********************************************************************
 *
 *  Copyright (c) 2014  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2014:proprietary:standard

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


#ifdef DMP_DEVICE2_XMPPBASIC_1

#include "cms_msg.h"
#include "cms_util.h"

#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut2_xmpp.h"
 

CmsRet rcl_dev2XmppObject( _Dev2XmppObject *newObj,
                const _Dev2XmppObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2XmppConnObject( _Dev2XmppConnObject *newObj,
                const _Dev2XmppConnObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;

    if(ADD_NEW(newObj, currObj))
    {
        /* rcl was called while the object is being added, so we do nothing */
        rutUtil_modifyNumXmppConn(iidStack, 1);
        ret = CMSRET_SUCCESS;
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
       XmppConnMsgBody msgData;
       
       memset(&msgData, 0, sizeof(msgData));
       cmsUtl_strcpy(msgData.jabberID, currObj->jabberID);
       rut_sendEventMsgToSmd(CMS_MSG_XMPP_CONNECTION_DELETE,
                             0,
                             (void *) &msgData,
                             sizeof(msgData));
       rutUtil_modifyNumXmppConn(iidStack, -1);
    }
    else
    {
        if ((newObj->enable != currObj->enable) ||
            (newObj->enable == TRUE && currObj->enable == TRUE && 
             (newObj->keepAliveInterval != currObj->keepAliveInterval)))
        {
           XmppConnMsgBody msgData;
           char jabberID[BUFLEN_1024];

           rutXmpp_generate_jabber_id(iidStack,jabberID);
           newObj->jabberID = cmsMem_strdupFlags((char*)jabberID,mdmLibCtx.allocFlags);
           memset(&msgData, 0, sizeof(msgData));
           msgData.enable = newObj->enable;
           msgData.useTLS = newObj->useTLS;
           msgData.keepAlive = newObj->keepAliveInterval;
           cmsUtl_strcpy(msgData.jabberID, newObj->jabberID);
           cmsUtl_strcpy(msgData.password, newObj->password);

#ifdef DMP_DEVICE2_XMPPADVANCED_1
           if (cmsUtl_strcmp(newObj->serverConnectAlgorithm, MDMVS_SERVERTABLE) == 0)
           {
              rutXmpp_get_server_info(iidStack,msgData.serverAddress,&(msgData.serverPort));
           }
#endif /* DMP_DEVICE2_XMPPADVANCED_1 */

           if (newObj->enable != currObj->enable)
           {
               rut_sendEventMsgToSmd(CMS_MSG_XMPP_CONNECTION_ENABLE,
                                     0,
                                     (void *) &msgData,
                                     sizeof(msgData));
           }
           else
           {
               rut_sendEventMsgToSmd(CMS_MSG_XMPP_CONNECTION_UPDATE,
                                     0,
                                     (void *) &msgData,
                                     sizeof(msgData));
           }
        }
    }
    return ret;
}


#ifdef DMP_DEVICE2_XMPPADVANCED_1


CmsRet rcl_dev2XmppConnServerObject( _Dev2XmppConnServerObject *newObj __attribute__((unused)),
                const _Dev2XmppConnServerObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret;

   cmsLog_debug("Enter");
   ret = rut_validateObjects(newObj, currObj); 
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }
      
   /*
    * Update numberOfEntries on successful add or delete.
    */
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumXmppConnServer(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumXmppConnServer(iidStack, -1);
   }

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* the default is enable, what needs to be done here? */
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /* I guess if the server is disable, and it is used by some connection reference by
       *  ManagementServer.ConnReqXMPPConnection, something should be done where.
       *  Also, the status should be changed according.
       */
   }   
   return CMSRET_SUCCESS;
}

/*
CmsRet rcl_dev2XmppConnStatsObject( _Dev2XmppConnStatsObject *newObj __attribute__((unused)),
                const _Dev2XmppConnStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
*/


#endif /* DMP_DEVICE2_XMPPADVANCED_1 */


#endif /* DMP_DEVICE2_XMPPBASIC_1 */


