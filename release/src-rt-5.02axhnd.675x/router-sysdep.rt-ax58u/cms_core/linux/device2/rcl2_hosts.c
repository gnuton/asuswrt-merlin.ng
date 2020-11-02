/***********************************************************************
 *
 *  Copyright (c) 2009-2013  Broadcom Corporation
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

#ifdef DMP_DEVICE2_HOSTS_2

/** all the TR181 hosts objects will go into this file */

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"


CmsRet rcl_dev2HostsObject(_Dev2HostsObject *newObj __attribute((unused)),
                           const _Dev2HostsObject *currObj __attribute((unused)),
                           const InstanceIdStack *iidStack __attribute((unused)),
                           char **errorParam __attribute((unused)),
                           CmsRet *errorCode __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2HostObject(_Dev2HostObject *newObj,
                          const _Dev2HostObject *currObj,
                          const InstanceIdStack *iidStack,
                          char **errorParam __attribute((unused)),
                          CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 updateHosts = FALSE;

   if (ADD_NEW(newObj, currObj))
   {
      cmsLog_debug("ADD_NEW");

      rutUtil_modifyNumHost(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("DELETE_OR_DISABLE_EXISTING");

      rutUtil_modifyNumHost(iidStack, -1);

      updateHosts = TRUE;
   }

   /*
    * ssk will first create an object, which will have all default values.
    * Then it will modify the object, filling in the IPAddr and hostname.
    * When the modified object entry has interesting data, we want to write
    * out the hosts file and tell dnsproxy to reload it.
    */
   if (newObj != NULL && currObj != NULL && newObj->IPAddress != NULL && newObj->hostName != NULL &&
       (cmsUtl_strcmp(newObj->hostName, currObj->hostName) || cmsUtl_strcmp(newObj->IPAddress, currObj->IPAddress)))
   {
      updateHosts = TRUE;
   }

   if (updateHosts == TRUE)
   {
#ifdef DESKTOP_LINUX
     cmsLog_debug("skipping rutSys_createHostFile and rutDpx_updateDnsproxy for DESKTOP");
#else
      /* TODO: handle DNS */
      // rutSys_createHostsFile();
      // rutDpx_updateDnsproxy();
#endif
   }

   return ret;
}


CmsRet rcl_dev2HostIpv4AddressObject(_Dev2HostIpv4AddressObject *newObj,
                                     const _Dev2HostIpv4AddressObject *currObj,
                                     const InstanceIdStack *iidStack,
                                     char **errorParam __attribute((unused)),
                                     CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      cmsLog_debug("ADD_NEW");

      rutUtil_modifyNumHostIPv4Address(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("DELETE_EXISTING");

      rutUtil_modifyNumHostIPv4Address(iidStack, -1);
   }

   return ret;
}


CmsRet rcl_dev2HostIpv6AddressObject(_Dev2HostIpv6AddressObject *newObj,
                                     const _Dev2HostIpv6AddressObject *currObj,
                                     const InstanceIdStack *iidStack,
                                     char **errorParam __attribute((unused)),
                                     CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      cmsLog_debug("ADD_NEW");

      rutUtil_modifyNumHostIPv6Address(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("DELETE_EXISTING");

      rutUtil_modifyNumHostIPv6Address(iidStack, -1);
   }

   return ret;
}


#endif  /* DMP_DEVICE2_HOSTS_2 */

#endif /* DMP_DEVICE2_BASELINE_1 */

