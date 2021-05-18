/***********************************************************************
 *
 *  Copyright (c) 2009  Broadcom Corporation
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

#ifdef DMP_X_BROADCOM_COM_PPTPAC_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_wanlayer2.h"

CmsRet rutPptpAC_start(WanPppConnObject *newObj, const char *server, const char *userid, const char *password)
{
   UINT32 pid = 0;
   CmsRet ret = CMSRET_SUCCESS;
   char pptpcmd[BUFLEN_128];
    /* start pptpac connection */
   snprintf(pptpcmd, sizeof(pptpcmd), "-r %s -U %s -P %s -f -c %s", server, userid, password, newObj->X_BROADCOM_COM_IfName);
   /* send message to ssk to launch pptpd */
   if ((pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_PPTPD, pptpcmd, strlen(pptpcmd)+1)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to start or restart dproxy");
      ret = CMSRET_INTERNAL_ERROR;
   }
   
   return ret;
}

CmsRet rutPptpAC_stop(_PptpAcIntfConfigObject *currObj)
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Entered");
   if ((rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_PPTPD, NULL,0)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to stop dnsproxy.");
      return CMSRET_INTERNAL_ERROR;
   }         
   return ret;
}

#endif /* DMP_X_BROADCOM_COM_PPTPAC_1 */
