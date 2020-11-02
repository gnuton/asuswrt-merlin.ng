/***********************************************************************
 *
 *  Copyright (c) 2006-2007  Broadcom Corporation
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

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_network.h"
#include "rut_wan.h"

CmsRet rcl_networkConfigObject( _NetworkConfigObject *newObj ,
                const _NetworkConfigObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
#ifdef BRCM_VOICE_SUPPORT
   int bRestartVoice = 0;
#endif
   
   if (ADD_NEW(newObj, currObj))
   {

#ifdef SUPPORT_ADVANCED_DMZ
      if (newObj->enableAdvancedDMZ == TRUE)
      {
         cmsLog_debug("Create the NonDmz lan device.");
         rutNtwk_addNonDmzLan();
      }
#endif /* SUPPORT_ADVANCED_DMZ */
   }

   if (DELETE_EXISTING(newObj, currObj))
   {
      cmsLog_error("cannot delete a type 0 object");
      return CMSRET_INVALID_ARGUMENTS;
   }

   /*
    * On bootup, or if the DNSIfName list or static dns servers has changed:
    * If static DNSServers has been configured, use those.
    * Otherwise, try to find an active (up) WAN interface from DNSIfName from
    * the list.
    */
   if ((currObj == NULL) ||
       cmsUtl_strcmp(newObj->DNSIfName, currObj->DNSIfName) ||
       cmsUtl_strcmp(newObj->DNSServers, currObj->DNSServers))
   {
      char activeDnsIfName[CMS_IFNAME_LENGTH]={0};
      char activeDNSServers[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};

      /* Select Active IPv4 DNS Servers */
      rutNtwk_selectActiveIpvxDnsServers(CMS_AF_SELECT_IPV4,
                                     newObj->DNSIfName,
                                     newObj->DNSServers,
                                     activeDnsIfName, activeDNSServers);
      
      CMSMEM_REPLACE_STRING_FLAGS(newObj->activeDNSServers, activeDNSServers, mdmLibCtx.allocFlags);

#ifdef BRCM_VOICE_SUPPORT
      bRestartVoice = 1;
#endif

      /* this object is IPv4 only.  We use Device2 data model objects for IPv6 */
   }

   /* Action only - config system dns with the activate dns ip.
   * Can be continue from above code OR from rutNtwk_doSystemDns where
   * the system has a WAN connection change
   */
   if ((ret = rutNtwk_configActiveDnsIp()) == CMSRET_SUCCESS)
   {
      cmsLog_debug("Done setting %s as system dns", newObj->activeDNSServers);
      
#ifdef SUPPORT_ADVANCED_DMZ
      if (newObj->enableAdvancedDMZ == TRUE && currObj->enableAdvancedDMZ == FALSE)
      {
         cmsLog_debug("Create the NonDmz lan device.");
         rutNtwk_addNonDmzLan();
      }
      else if (newObj->enableAdvancedDMZ == FALSE && currObj->enableAdvancedDMZ == TRUE)
      {
         cmsLog_debug("Delete the nonDMZ lan device.");
         rutNtwk_deleteNonDmzLan();
      }
#endif /* SUPPORT_ADVANCED_DMZ */

   }
   else
   {
      cmsLog_error("Fail to config %s as active system dns.", newObj->activeDNSServers);
   }

#ifdef BRCM_VOICE_SUPPORT
   //  only restart voice if there is a runtime change, not during bootup.
   if (currObj != NULL && bRestartVoice )
   {
      ret = rut_sendMsgCommon(EID_SSK, CMS_MSG_RESTART_VOICE_CALLMGR, 0,
                              TRUE, FALSE, FALSE,
                              NULL, 0);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not send voice START/RESTART msg to ssk, ret=%d", ret);
      }
   }
#endif

   cmsLog_debug("Exit, ret=%d", ret);
   return ret;
}
