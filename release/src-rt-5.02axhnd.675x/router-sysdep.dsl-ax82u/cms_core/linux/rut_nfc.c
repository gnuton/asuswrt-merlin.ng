// <:copyright-BRCM:2015:proprietary:standard 
// 
//    Copyright (c) 2015 Broadcom 
//    All Rights Reserved
// 
//  This program is the proprietary software of Broadcom and/or its
//  licensors, and may only be used, duplicated, modified or distributed pursuant
//  to the terms and conditions of a separate, written license agreement executed
//  between you and Broadcom (an "Authorized License").  Except as set forth in
//  an Authorized License, Broadcom grants no license (express or implied), right
//  to use, or waiver of any kind with respect to the Software, and Broadcom
//  expressly reserves all rights in and to the Software and all intellectual
//  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
//  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
//  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
// 
//  Except as expressly set forth in the Authorized License,
// 
//  1. This program, including its structure, sequence and organization,
//     constitutes the valuable trade secrets of Broadcom, and you shall use
//     all reasonable efforts to protect the confidentiality thereof, and to
//     use this information only in connection with your use of Broadcom
//     integrated circuit products.
// 
//  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
//     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
//     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
//     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
//     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
//     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
//     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
//     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
//     PERFORMANCE OF THE SOFTWARE.
// 
//  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
//     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
//     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
//     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
//     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
//     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
//     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
//     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
//     LIMITED REMEDY.
// :>

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_pmap.h"
#include "rut_lan.h"
#include "rut_wlan.h"

/** Send a message to nfc process
 *
 * @return CmsRet enum.
 */


#ifdef DMP_X_BROADCOM_COM_NFC_1

CmsRet sendMsgToNfc(unsigned int msgType)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader reqMsg=EMPTY_MSG_HEADER;

   reqMsg.type = msgType;
   reqMsg.src = mdmLibCtx.eid;
   reqMsg.dst = EID_NFCD;
   reqMsg.flags_request = 1;
   reqMsg.dataLength = 0;

   ret = cmsMsg_send(mdmLibCtx.msgHandle, &reqMsg);
   if (ret){
      cmsLog_error("could not send msg(%x) to nfc, ret=%d", msgType, ret);
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_debug("msg(%x) sent successfully", msgType);
   }

   return ret;
}

CmsRet sendMsgGetReplyToNfcWithTimeout(unsigned int msgType, unsigned int timeoutMs)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader reqMsg = EMPTY_MSG_HEADER;

   reqMsg.type = msgType;
   reqMsg.src = mdmLibCtx.eid;
   reqMsg.dst = EID_NFCD;
   reqMsg.flags_request = 1;
   reqMsg.dataLength = 0;

   ret = cmsMsg_sendAndGetReplyWithTimeout(mdmLibCtx.msgHandle, &reqMsg, timeoutMs);
   if (ret == CMSRET_TIMED_OUT) {
      cmsLog_error("could not send msg(%x) to nfc, ret=%d(timeout)", msgType, ret);
      ret = CMSRET_INTERNAL_ERROR;
   }
   else if (ret == FALSE) {
      cmsLog_error("msg(%x) is sent to nfc, but failure in response", msgType);
      ret = CMSRET_INTERNAL_ERROR;
   }
   else if (ret == TRUE) {
      cmsLog_debug("msg(%x) sent successfully", msgType);
      ret = CMSRET_SUCCESS;
   }
   else
   {
      cmsLog_error("could not send msg(%x) to nfc, ret=%d", msgType, ret);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
}

#endif  /* DMP_X_BROADCOM_COM_NFC_1 */

#ifdef DMP_X_BROADCOM_COM_NFC_1
CmsRet rut_switchNfc(UBOOL8 enable)
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("rut_switchNfc with %d", enable);
   if (enable == TRUE)
   {
      /*
       * If nfc is enabled, it should be running.  Even if it is not
       * running, sending a message to it will cause smd to launch it.
       */
      if ((ret = sendMsgGetReplyToNfcWithTimeout(CMS_MSG_NFCD_START, 10*1000)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to start nfc app, ret=%d", ret);
         return ret;
      }
   }
   else
   {
      if ((ret = sendMsgGetReplyToNfcWithTimeout(CMS_MSG_NFCD_TERMINATE, 10*1000)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to terminate nfc app, ret=%d", ret);
         return ret;
      }
   }

   return ret;
}
#else
CmsRet rut_switchNfc(UBOOL8 enable __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif

CmsRet rut_updateNfc()
{
   CmsRet ret = CMSRET_SUCCESS;

#ifdef DMP_X_BROADCOM_COM_NFC_1

   if ((ret = sendMsgToNfc(CMS_MSG_NFCD_UPDATE_WLAN_INFO)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to update nfc app, ret=%d", ret);
      return ret;
   }

#endif /* DMP_X_BROADCOM_COM_NFC_1 */

   return ret;
}
