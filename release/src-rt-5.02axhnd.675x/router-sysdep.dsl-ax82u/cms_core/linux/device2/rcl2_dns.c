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

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "cms_qdm.h"
#include "rut_network.h"

/*!\file rcl2_dns.c
 * \brief This file contains device 2 device.DNS objects related functions.
 *
 */

static void modifyDnsServerNumEntry(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DNS_CLIENT,
                        MDMOID_DEV2_DNS_SERVER,
                        iidStack,
                        delta);
}

CmsRet rcl_dev2DnsObject( _Dev2DnsObject *newObj __attribute__((unused)),
                const _Dev2DnsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* Nothing to do here, just return */
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DnsClientObject( _Dev2DnsClientObject *newObj,
                const _Dev2DnsClientObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * DNS Client object is activated in mdm_init.c
    * In mdm_activateObjects all RCL functions are called including this one.
    * We assume if the DNS Client feature is compiled in, that it is always
    * enabled by default.
    */
   if (ADD_NEW(newObj, currObj))
   {
      newObj->enable = TRUE;
   }

   /*
    * We do not handle the case where this object is disabled.
    */
   if (newObj && newObj->enable)
   {
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_ENABLED,
                                                      mdmLibCtx.allocFlags);
   }

   /*
    * If during runtime, someone changes the X_BROADCOM_COM_DNSIfNames /X_BROADCOM_COM_ActiveDnsServers or
    * X_BROADCOM_COM_Ipv6_DNSIfNames /X_BROADCOM_COM_Ipv6_ActiveDnsServers,
    * recalculate the active system default DNS servers and trigger
    * reconfiguration of all DNS related stuff.
    */
   if (newObj && currObj && newObj->enable)
   {
      char activeDnsIntfName[CMS_IFNAME_LENGTH]={0};
      char activeDnsServers[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
      char staticDNSServers[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
      UBOOL8 doTrigger=FALSE;
      UBOOL8 dnsIfNamesChanged=FALSE;
      UBOOL8 activeDnsServersChanged=FALSE;

      dnsIfNamesChanged = (0 != cmsUtl_strcmp(newObj->X_BROADCOM_COM_DnsIfNames,
                                             currObj->X_BROADCOM_COM_DnsIfNames));

      activeDnsServersChanged = (0 != cmsUtl_strcmp(newObj->X_BROADCOM_COM_ActiveDnsServers,
                                                   currObj->X_BROADCOM_COM_ActiveDnsServers));

      if (dnsIfNamesChanged || activeDnsServersChanged)
      {
         if (dnsIfNamesChanged)
         {
            cmsLog_debug("DNSIfNames changed %s, recalculate!",
                         newObj->X_BROADCOM_COM_DnsIfNames);
         }
         if (activeDnsServersChanged)
         {
            cmsLog_debug("ActiveDNSServers changed %s, recalculate!",
                         newObj->X_BROADCOM_COM_ActiveDnsServers);
         }

         qdmDns_getStaticIpvxDnsServersLocked_dev2(CMS_AF_SELECT_IPV4, staticDNSServers);

         rutNtwk_selectActiveIpvxDnsServers(CMS_AF_SELECT_IPV4,
                                         newObj->X_BROADCOM_COM_DnsIfNames,
                                         staticDNSServers,
                                         activeDnsIntfName, activeDnsServers);

         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_ActiveDnsIfName,
                                     activeDnsIntfName, mdmLibCtx.allocFlags);

         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_ActiveDnsServers,
                                     activeDnsServers, mdmLibCtx.allocFlags);
         doTrigger = cmsMdm_isDataModelDevice2();
      }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      dnsIfNamesChanged = (0 != cmsUtl_strcmp(newObj->X_BROADCOM_COM_Ipv6_DnsIfNames,
                                             currObj->X_BROADCOM_COM_Ipv6_DnsIfNames));

      activeDnsServersChanged = (0 != cmsUtl_strcmp(newObj->X_BROADCOM_COM_Ipv6_ActiveDnsServers,
                                                   currObj->X_BROADCOM_COM_Ipv6_ActiveDnsServers));

      if (dnsIfNamesChanged || activeDnsServersChanged)
      {
         if (dnsIfNamesChanged)
         {
            cmsLog_debug("IPv6 DNSIfNames changed %s, recalculate!",
                         newObj->X_BROADCOM_COM_Ipv6_DnsIfNames);
         }
         if (activeDnsServersChanged)
         {
            cmsLog_debug("IPv6 ActiveDNSServers changed %s, recalculate!",
                         newObj->X_BROADCOM_COM_Ipv6_ActiveDnsServers);
         }

         memset(activeDnsIntfName, 0, sizeof(activeDnsIntfName));
         memset(activeDnsServers, 0, sizeof(activeDnsServers));

         qdmDns_getStaticIpvxDnsServersLocked_dev2(CMS_AF_SELECT_IPV6, staticDNSServers);

         rutNtwk_selectActiveIpvxDnsServers(CMS_AF_SELECT_IPV6,
                                       newObj->X_BROADCOM_COM_Ipv6_DnsIfNames,
                                       staticDNSServers,
                                       activeDnsIntfName, activeDnsServers);

         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_Ipv6_ActiveDnsIfName,
                                     activeDnsIntfName, mdmLibCtx.allocFlags);

         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_Ipv6_ActiveDnsServers,
                                     activeDnsServers, mdmLibCtx.allocFlags);
         doTrigger = cmsMdm_isDataModelDevice2();
      }
#endif  /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */

      if (doTrigger)
      {
         /* trigger reconfiguration of all DNS related stuff */
         rutNtwk_configActiveDnsIp();
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2DnsServerObject( _Dev2DnsServerObject *newObj,
                const _Dev2DnsServerObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   /*
    * Update NumberOfEntries on successful add or delete.
    */
   if (ADD_NEW(newObj, currObj))
   {
      modifyDnsServerNumEntry(iidStack, 1);
      if (newObj->enable)
      {
         if (IS_EMPTY_STRING(newObj->interface))
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_ENABLED,
                                                            mdmLibCtx.allocFlags);
         }
      }
      else
      {
         /* no action on startup.  just return. */
         cmsLog_debug("start up or add - No action.");
      }
      return ret;
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      modifyDnsServerNumEntry(iidStack, -1);
      cmsLog_debug("Delete - No action");
      return ret;      
   }

   /*
    * We do not support the case where a DNS.Client.Server object is disabled.
    * The only thing this RCL handler does is update the status param
    * based on IP.Interface UP or not.
    */
   if (newObj && newObj->enable)
   {
      if (!IS_EMPTY_STRING(newObj->interface))
      {
         /* this server is associated with a particular interface, so
          * set status based on interface status.
          */
         if (qdmIntf_isStatusUpOnFullPathLocked_dev2(newObj->interface))
         {
            /* my interface is UP, so my status should be Enabled */
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_ENABLED,
                                                        mdmLibCtx.allocFlags);
         }
         else
         {
            /* my interface is down, so my status should be Disabled */
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_DISABLED,
                                                        mdmLibCtx.allocFlags);
         }
      }
      else
      {
         /* server objects with no interface setting are statically
          * configured DNS servers, so just set their status to ENABLED.
          */
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_ENABLED,
                                                     mdmLibCtx.allocFlags);
      }
   }
   
   return ret;
}


#endif    /* DMP_DEVICE2_BASELINE_1 */



