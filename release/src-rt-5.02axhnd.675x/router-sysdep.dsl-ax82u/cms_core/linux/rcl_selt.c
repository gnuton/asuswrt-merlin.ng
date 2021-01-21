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

#ifdef DMP_BASELINE_1

#ifdef DMP_X_BROADCOM_COM_SELT_1

#include "cms.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "devctl_adsl.h"
#include "rut_dsl.h"
#include "adslctlapi.h"

CmsRet rcl_seltObject( _SeltObject *newObj __attribute__((unused)),
                       const _SeltObject *currObj __attribute__((unused)),
                       const InstanceIdStack *iidStack __attribute__((unused)),
                       char **errorParam __attribute__((unused)),
                       CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_seltCfgObject( _SeltCfgObject *newObj,
                          const _SeltCfgObject *currObj,
                          const InstanceIdStack *iidStack __attribute__((unused)),
                          char **errorParam __attribute__((unused)),
                          CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }

   if (mdmLibCtx.eid != EID_SSK)
   {
      if (cmsUtl_strcmp(newObj->seltTestState, MDMVS_NONE) == 0)
      {
         /* if it's not requested, return CMS_SUCCESS, so other parameters can be changed */
         return CMSRET_SUCCESS;
      }
      else if (cmsUtl_strcmp(newObj->seltTestState, MDMVS_REQUESTED))
      {
         /* not equal REQUESTED nor NONE*/
         cmsLog_debug("Mgmt apps may only write REQUESTED to this object");
         return CMSRET_INVALID_ARGUMENTS;
      }
      else if ((IS_EMPTY_STRING(newObj->CID)) || (strlen(newObj->CID) < 10))
      {
         cmsLog_error("CID is invalid");
         return CMSRET_INVALID_ARGUMENTS;
      }
   }

   if (cmsUtl_strcmp(newObj->seltTestState,MDMVS_REQUESTED) == 0) 
   {
      /* DSL SELT test will bring ADSL link down.  ACS (TR69) expects
       * a set request response after initiating this test.  This RCL needs to return
       * set success before initiating this test to allow time for TR69 client to send
       * the response.
       * 
       * 1. If management apps initiates the test, message is sent SSK and returns.
       * 2. SSK receives the message, it does 2 things:
       *    a. calls objSet again to set seltTestState to START.
       *    This is to avoid having SSK having to initiate the test.  This routine will be called 
       *    again.  ADSL driver will be called to start the test.
       *    b. SSK starts to poll for the result of the test.
       */

      if (mdmLibCtx.eid != EID_SSK)
      {
         UINT32 msgDataLen = sizeof(dslDiagMsgBody);
         char buf[sizeof(CmsMsgHeader) + sizeof(dslDiagMsgBody)]={0};
         CmsMsgHeader *msg = (CmsMsgHeader *) buf;
         /* I do not care about the body now */
         /*dslDiagMsgBody *info = (dslDiagMsgBody *) &(buf[sizeof(CmsMsgHeader)]); */
      
         cmsLog_notice("Sending message to SSK to start doing checking SELT results, line %d",newObj->lineNumber);
         msg->type = CMS_MSG_WATCH_DSL_SELT_DIAG;
         msg->src = mdmLibCtx.eid;
         msg->dst = EID_SSK;
         msg->flags_request = 1;
         msg->dataLength = msgDataLen;
         
         /* for ssk to set to started the second time */
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->seltTestState,MDMVS_NONE,mdmLibCtx.allocFlags);
         if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, msg)) != CMSRET_SUCCESS)
         {
            cmsLog_debug("Could not send out CMS_MSG_DSL_SELT_DIAG_MONITOR ret=%d", ret);
         }
         else
         {
            cmsLog_debug("CMS_MSG_DSL_SELT_DIAG_MONITOR msg sent");
         }
      } /* !ssk */
      else
      {
         /* Give application like TR69c a chance to do a reponse to server
            first before bringing down xDSL link */
         sleep(1);
         cmsLog_debug("Ask ADSL driver to start the test.");

         /* first set the configuration of the SELT Test */
         ret=xdslCtl_SeltConfigureTest((unsigned char)newObj->lineNumber,atof(newObj->maxSeltPSD),
                                       atof(newObj->maxSeltT),atof(newObj->maxSeltF),NULL);

         if (ret == CMSRET_SUCCESS)
         {
            ret = xdslCtl_StartSeltTest(newObj->lineNumber, NULL, NULL);
         }
      }
   } /* requested */
   return (ret);
}

CmsRet rcl_seltResultObject( _SeltResultObject *newObj __attribute__((unused)),
                             const _SeltResultObject *currObj __attribute__((unused)),
                             const InstanceIdStack *iidStack __attribute__((unused)),
                             char **errorParam __attribute__((unused)),
                             CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif /* X_BROADCOM_COM_SELT_1 */


#endif  /* DMP_BASELINE_1 */
