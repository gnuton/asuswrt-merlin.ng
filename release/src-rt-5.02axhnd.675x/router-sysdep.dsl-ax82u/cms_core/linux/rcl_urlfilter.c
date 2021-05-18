/***********************************************************************
 *
 *  Copyright (c) 2006-2010  Broadcom Corporation
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_iptables.h"

CmsRet rcl_urlFilterCfgObject( _UrlFilterCfgObject *newObj __attribute__((unused)),
                                 const _UrlFilterCfgObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

#ifdef SUPPORT_URLFILTER
   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));
   
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if (ADD_NEW(newObj, currObj))
   {
      /*
       * New object instance.  This only happens at startup time.
       */
      cmsLog_debug("system boots up");
      if (newObj->enable == TRUE)
      {
         rutIpt_activeUrlFilter();
      }
   }
   else if (newObj != NULL && currObj != NULL)
   {
      cmsLog_debug("Enabling url filter feature: type=%s", newObj->excludeMode);

      if (currObj->enable == FALSE)
      {
         rutIpt_activeUrlFilter();
      }
      else
      {
         rutIpt_configUrlFilterMode(newObj->excludeMode);
      }

   }

   /* remove ifc, or disable ifc*/
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting url filter feature");

      rutIpt_deactiveUrlFilter();
   }   

#endif
   return ret;

}


CmsRet rcl_urlFilterListObject( _UrlFilterListObject *newObj __attribute__((unused)),
                                 const _UrlFilterListObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

#ifdef SUPPORT_URLFILTER
//   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;   
//   _UrlFilterListObject *Obj = NULL;

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));
   
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Adding url filter list entry: url=%s, port=%u", newObj->urlAddress, newObj->portNumber);

      rutIpt_urlFilterConfig(newObj, TRUE);
   }

   /* remove ifc, or disable ifc*/
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting url filter list entry: url=%s, port=%u", currObj->urlAddress, currObj->portNumber);

      rutIpt_urlFilterConfig((void *)currObj, FALSE);
   }   

#endif
   return ret;
}


