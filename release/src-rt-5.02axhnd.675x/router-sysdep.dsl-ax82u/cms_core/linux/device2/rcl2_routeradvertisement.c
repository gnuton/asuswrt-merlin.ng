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

/* this file touches TR181 IPv6 objects */
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1 /* aka SUPPORT_IPV6 */

#ifdef DMP_DEVICE2_ROUTERADVERTISEMENT_1


#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_ra.h"
#include "rut_lan.h"

/*!\file rcl2_routeradvertisment.c
 * \brief This file contains device 2 device.RouterAdvertisment objects related functions.
 *
 */

CmsRet rcl_dev2RouterAdvertisementObject( _Dev2RouterAdvertisementObject *newObj __attribute__((unused)),
                const _Dev2RouterAdvertisementObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2RouterAdvertisementInterfaceSettingObject( _Dev2RouterAdvertisementInterfaceSettingObject *newObj,
                const _Dev2RouterAdvertisementInterfaceSettingObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj != NULL && currObj == NULL)
   {
      UINT32 pid;
      cmsLog_debug("startup condition");
      /* for single instance object, this is the startup condition */

      /* launch rastatus6 here at system bootup time */
      if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_RASTATUS6,  NULL, 0)) == CMS_INVALID_PID)
      {
         cmsLog_error("failed to restart rastatus6");
         ret = CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("rastatus6 started with pid=%d", pid);
      }

      if (newObj->enable)
      {
         UBOOL8 foundPD;

         rutRa_updatePrefixes(newObj->interface, &(newObj->prefixes), &foundPD);
         rutRa_createRadvdConf(foundPD);
         rut_restartRadvd();
      }
   }
   else if (newObj != NULL && currObj != NULL)
   {
      cmsLog_debug("new->enable=%d, old->enable=%d", 
                   newObj->enable, currObj->enable);

      /*
       * 1. Update prefix parameter
       * 2. Create config file accordingly
       * 3. Restart radvd
       */
      if (newObj->enable)
      {
         UBOOL8 foundPD;

         rutRa_updatePrefixes(newObj->interface, &(newObj->prefixes), &foundPD);
         rutRa_createRadvdConf(foundPD);
         rut_restartRadvd();
      }
      else if (currObj->enable)
      {
         rut_stopRadvd();
      }
   }

   return ret;
}


#if 0 //FIXME: NOT SUPPORTED
CmsRet rcl_dev2RouterAdvertisementInterfaceSettingOptionObject( _Dev2RouterAdvertisementInterfaceSettingOptionObject *newObj __attribute__((unused)),
                const _Dev2RouterAdvertisementInterfaceSettingOptionObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif



#endif  /* DMP_DEVICE2_ROUTERADVERTISEMENT_1 */

#endif /* SUPPORT_IPV6 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
