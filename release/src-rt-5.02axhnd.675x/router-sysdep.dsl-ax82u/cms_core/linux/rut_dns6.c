/*
#
#  Copyright 2011, Broadcom Corporation
#
# <:label-BRCM:2011:proprietary:standard
# 
#  This program is the proprietary software of Broadcom and/or its
#  licensors, and may only be used, duplicated, modified or distributed pursuant
#  to the terms and conditions of a separate, written license agreement executed
#  between you and Broadcom (an "Authorized License").  Except as set forth in
#  an Authorized License, Broadcom grants no license (express or implied), right
#  to use, or waiver of any kind with respect to the Software, and Broadcom
#  expressly reserves all rights in and to the Software and all intellectual
#  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
#  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
#  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
# 
#  Except as expressly set forth in the Authorized License,
# 
#  1. This program, including its structure, sequence and organization,
#     constitutes the valuable trade secrets of Broadcom, and you shall use
#     all reasonable efforts to protect the confidentiality thereof, and to
#     use this information only in connection with your use of Broadcom
#     integrated circuit products.
# 
#  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
#     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
#     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
#     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
#     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
#     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
#     PERFORMANCE OF THE SOFTWARE.
# 
#  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
#     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
#     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
#     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
#     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
#     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
#     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
#     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
#     LIMITED REMEDY.
# :>
*/

#include "cms_util.h"
#include "cms_msg.h"
#include "cms_core.h"
#include "cms_qdm.h"
#include "rut_lan.h"
#include "rut_util.h"
#include "rut_system.h"
#include "mdm.h"
#include "rut_pmap.h"
#include "rut_route.h"
#include "rut_network.h"
#include "rut_wan.h"

UBOOL8 fetchPrefix(const char *wanIfName, char *prefix)
{
   UBOOL8 found = FALSE;

   cmsLog_debug("wanIfName=%s prefix=%s", wanIfName, prefix);

#ifdef DMP_X_BROADCOM_COM_IPV6_1 
   if (prefix == NULL)
   {
      cmsLog_error("prefix is NULL.");
   }   
   else
   {
      char *tmp = NULL;

      rut_getDhcp6sPrefixFromInterface(wanIfName, &tmp);
      cmsLog_debug("*prefix=%p", tmp);
      if (tmp != NULL)
      {
         strcpy(prefix, tmp);
         cmsMem_free(tmp);
         found = TRUE;
      }
   }
#endif /* DMP_X_BROADCOM_COM_IPV6_1 */

   cmsLog_debug("found=%d", found);
   
   return found;
   
}


#ifdef DMP_X_BROADCOM_COM_IPV6_1
/* this function is only used in deprecated proprietary Broadcom IPv6 */
UBOOL8 fetchActiveDnsServers6(char *dnsServers6)
{
   UBOOL8 found = FALSE;
   
   if (dnsServers6 == NULL)
   {
      cmsLog_error("NULL string.");
      return FALSE;
   }   

   {
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      IPv6LanHostCfgObject *ipv6Obj = NULL;
      char *dnsServer  = NULL;
      char *domainName  = NULL;
      CmsRet ret = CMSRET_SUCCESS;
      
      if ((ret = cmsObj_getNext(MDMOID_I_PV6_LAN_HOST_CFG, &iidStack, (void **)&ipv6Obj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_getNext <MDMOID_I_PV6_LAN_HOST_CFG> returns error. ret=%d", ret);
         return found;
      }   

      if (!cmsUtl_strcmp(ipv6Obj->IPv6DNSConfigType, MDMVS_STATIC))
      {
         /* static ipv6 dns ip is in this object */

         if (!IS_EMPTY_STRING(ipv6Obj->IPv6DNSServers))
         {
            strcpy(dnsServers6, ipv6Obj->IPv6DNSServers); 
            found = TRUE;
         }         
      }
      else
      {
         /* dynamic ipv6 dns ip is in the wan ip/ppp object */
         
         if ((ret=rutWan_getDns6Server(ipv6Obj->IPv6DNSWANConnection, &dnsServer, &domainName)) != CMSRET_SUCCESS)
         {
            cmsLog_debug("rutWan_getDns6Server returns error = %d", ret);
            cmsMem_free(dnsServer);
            cmsMem_free(domainName);
         }

         if (!IS_EMPTY_STRING(dnsServer))
         {
            strcpy(dnsServers6, dnsServer); 
            found = TRUE;
         }
      }
      cmsObj_free((void **)&ipv6Obj);
      cmsMem_free(dnsServer);
      cmsMem_free(domainName);
      
      cmsLog_debug("found=%d, dnsServers6=%s", found, dnsServers6);
      
   }      

   return found;
}
#endif /* DMP_X_BROADCOM_COM_IPV6_1 */


UBOOL8 isActiveDNSServer6(const char *wanIfName, const char *activeDNServer6)
{

   UBOOL8 isActiveDns6 = FALSE;

#ifdef DMP_X_BROADCOM_COM_IPV6_1    
   char *dnsServer  = NULL;
   char *domainName  = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   
   if ((ret =rutWan_getDns6Server(wanIfName, &dnsServer, &domainName)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutWan_getDns6Server returns error = %d", ret);
   }
   else
   {
      if (!cmsUtl_strcmp(dnsServer, activeDNServer6))
      {
         isActiveDns6 = TRUE;
      }
      cmsMem_free(dnsServer);
      cmsMem_free(domainName);
   } 


#elif  DMP_X_BROADCOM_COM_DEV2_IPV6_1 
   {
      char dnsServers6[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};

      rutNtwk_getIpv6DnsServersFromIfName(wanIfName, dnsServers6);
      if (!cmsUtl_strcmp(dnsServers6, activeDNServer6))
      {
         isActiveDns6 = TRUE;
      }      
   } 
   
#endif /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */

   cmsLog_debug("wanIfName=%s, activeDNServer6=%s, isActiveDns6=%d", wanIfName, activeDNServer6, isActiveDns6);
   
   return isActiveDns6;
   
}


/* this function is only used with old proprietary Broadcom IPv6 impl */
#ifdef DMP_X_BROADCOM_COM_IPV6_1
CmsRet rutNtwk_getIpv6DnsServersFromIfName_igd(const char *ifName, char *DNSServers)
{
   return (rutNtwk_getIpvxDnsServersFromIfName_igd(CMS_AF_SELECT_IPV6,
                                                   ifName, DNSServers));
}
#endif  /* DMP_X_BROADCOM_COM_IPV6_1 */


CmsRet getDnsParam6(const char *wanIfName __attribute__((unused)),
                    const char *activeDNSServers6 __attribute__((unused)),
                    char *ifName6 __attribute__((unused)),
                    char *subnetCidr6 __attribute__((unused)),
                    char *DNSServer6 __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

#ifdef DMP_X_BROADCOM_COM_IPV6_1      
   /* todo: IPv6 interface group ...  */
   
   if (!fetchPrefix(wanIfName, subnetCidr6))
   {
      /* If this WAN interface is not configured for PD, just put loopback "::1/128" for PD */
      cmsLog_debug("No prefix for %s. Just use loopback as prefix.", wanIfName);
      strcpy(DNSServer6, activeDNSServers6);
      strcpy(ifName6, wanIfName);
      strcpy(subnetCidr6, "::1/128");
   }          
   else
   {
      InstanceIdStack iidStack =EMPTY_INSTANCE_ID_STACK;
      _IPv6L3ForwardingObject *ipv6L3ForwadingObj=NULL;
      char activeDefaultGateway[CMS_IFNAME_LENGTH]={0};
      
      strcpy(DNSServer6, activeDNSServers6);
      strcpy(ifName6, wanIfName);

      if ((ret = qdmRt_getActiveDefaultGatewayLocked(activeDefaultGateway)) == CMSRET_SUCCESS)
      {
         cmsLog_debug("Use ipv4 active default gateway");
      }
      else
      {
         cmsLog_debug("No pv4 active default gateway found (ret=%s).  Try to get ipv6 default wan interface if ipv4 does not exist.", ret);
         
         if ((ret = cmsObj_get(MDMOID_I_PV6_L3_FORWARDING, &iidStack, 0, (void **)&ipv6L3ForwadingObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get MDMOID_I_PV6_L3_FORWARDING, ret=%d", ret);
            return ret;
         }
         strncpy(activeDefaultGateway, ipv6L3ForwadingObj->defaultConnectionService, sizeof(activeDefaultGateway));
         cmsObj_free((void **) &ipv6L3ForwadingObj);
      }
      
      /* Three cases for subnetCidr6: 
       * 1) if this wan interface is a IPv6 default Gateway interface, the subnetCidr6 from fetchPrefix will be used.
       * 2) if the wan is not a IPv6 default gateway and this is not an IPv4 wan interface group, the subnetCidr6 will be a loopback ::1/128
       * 3) if the wan is not a IPv6 default gateway and this is an IPv4 wan interface group,, all IPv6 information will not be used in the dns line. 
       */
      cmsLog_debug("activeDefaultGateway =%s, wanIfName=%s", activeDefaultGateway, wanIfName);
      if (cmsUtl_strcmp(activeDefaultGateway, wanIfName))
      {
         /* Not a IPv6 default gateway interface.  For case 2 and 3 */
         if (!rutPMap_isWanUsedForIntfGroup(wanIfName))
         {
            /* case 2:  need IPv6 loopback  */
            strcpy(subnetCidr6, "::1/128");
         }
         else
         {
            /* case 3: Need to null all IPv6 dns info since they are not needed. */
            *subnetCidr6 = '\0';
            *ifName6 = '\0';
            *DNSServer6 = '\0';
         }
      }
      else
      {
         /* case 1). */
         cmsLog_debug("Use the prefix from fetchPrefix %s", subnetCidr6);
      }
   }      


#elif  DMP_X_BROADCOM_COM_DEV2_IPV6_1 

   {
      char activeIpv6DnsServers[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
      char prefix[CMS_IPADDR_LENGTH];
      
      strcpy(ifName6, wanIfName);

      if (qdmIpv6_getIpPrefixInfo_dev2("br0", MDMVS_STATIC, MDMVS_CHILD, prefix, sizeof(prefix)) == CMSRET_SUCCESS)
      {
         cmsLog_debug("Found prefix %s.", prefix);
         strcpy(subnetCidr6, prefix);
      }
      if (rutNtwk_getIpv6DnsServersFromIfName(ifName6, activeIpv6DnsServers) == CMSRET_SUCCESS)
      {
         cmsLog_debug("Found active dns ip %s.", activeIpv6DnsServers);
         strcpy(DNSServer6, activeIpv6DnsServers);
      }

      cmsLog_debug("prefix %s,  DNSServer6 %s", prefix, DNSServer6);
   }      

#endif /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */


  return ret;
  
}     

