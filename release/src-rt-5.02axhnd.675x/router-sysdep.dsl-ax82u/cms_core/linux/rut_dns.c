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
#include "rut_lan.h"
#include "rut_util.h"
#include "rut_system.h"
#include "mdm.h"
#include "cms_qdm.h"
#include "rut_pmap.h"
#include "rut_route.h"
#include "rut_network.h"
#include "rut_dns.h"
#include "rut_dns6.h"
#include "rut_wan.h"


#ifdef SUPPORT_TR69C
static CmsRet fillInTr69cProcessName(const char *wanIfName, char **processList)
{
   char boundIfNameBuf[CMS_IFNAME_LENGTH]={0};
   CmsRet ret;
   
   ret = qdmTr69c_getBoundIfNameLocked(boundIfNameBuf);
   if (ret != CMSRET_SUCCESS)
   {
      /* maybe tr69c is not enabled in this build, just return. */
      return CMSRET_SUCCESS;
   }

   cmsLog_debug("wanIfName=%s BoundIfName=%s;", wanIfName, boundIfNameBuf);

   if (!cmsUtl_strcmp(boundIfNameBuf, wanIfName))
   {
      if (*processList == NULL)
      {
         *processList = cmsMem_strdup("tr69c");
      }
      else
      {
         char procName[BUFLEN_32];
         strcpy(procName, ",tr69c"); 
         *processList = cmsMem_realloc(*processList, strlen(procName)+1);
         if (*processList)
         {
            strcat(*processList, procName);
         }
      }

      if (*processList)
      {
         cmsLog_debug("*processList=%s", *processList);
      }         
      else
      {
         cmsLog_error("Failed to allocate memory");
         ret = CMSRET_INTERNAL_ERROR;
      }
   }

   return ret;
}
#endif /* SUPPORT_TR69C */

#ifdef DMP_BASELINE_1
static CmsRet getActiveDnsServers(char *activeDNSServers,
                              char *activeDNServers6 __attribute__((unused)),
                              UBOOL8 *isStaticDNS,
                              UBOOL8 *isStaticDNS6 __attribute__((unused)),
                              char *ifName,
                              char *ifName6 __attribute__((unused)))
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _NetworkConfigObject *networkCfg=NULL;
   char activeDnsIfName[CMS_IFNAME_LENGTH]={0};
   CmsRet ret = CMSRET_SUCCESS;
   
   /* For IPv4, use system default dns for the first line in /var/dnsinfo.conf. */
   if ((ret = cmsObj_get(MDMOID_NETWORK_CONFIG, &iidStack, OGF_NO_VALUE_UPDATE, (void *) &networkCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get NETWORK_CONFIG. ret=%d", ret);
      return ret;
   }   
   /* Need to fetch an active DNSServers  for later cmpare for the first line of dnsinfo.conf */
   {
      /* Select Active IPv4 DNS Servers */
      rutNtwk_selectActiveIpvxDnsServers(CMS_AF_SELECT_IPV4,
                                         networkCfg->DNSIfName,
                                         networkCfg->DNSServers,
                                         activeDnsIfName, activeDNSServers);

      *isStaticDNS = (0 == cmsUtl_strcmp(activeDnsIfName, CMS_FAKE_STATIC_DNS_IFNAME));

      cmsUtl_strcpy(ifName, *isStaticDNS ? CMS_FAKE_STATIC_DNS_IFNAME : activeDnsIfName);
   }

   cmsObj_free((void **)&networkCfg);

#ifdef DMP_X_BROADCOM_COM_IPV6_1
   /* TODO. Support for ipv6 static dns */
   fetchActiveDnsServers6(activeDNServers6);
#endif
   
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1 
   {
      _Dev2DnsClientObject *dnsClientObj=NULL;
      char staticDNSServers[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
      char dns1[CMS_IPADDR_LENGTH]={0};
      char dns2[CMS_IPADDR_LENGTH]={0};

      INIT_INSTANCE_ID_STACK(&iidStack);
      if ((ret = cmsObj_get(MDMOID_DEV2_DNS_CLIENT, &iidStack, OGF_NO_VALUE_UPDATE, (void *) &dnsClientObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get MDMOID_DEV2_DNS_CLIENT. ret=%d", ret);
         return ret;
      }

      qdmDns_getStaticIpvxDnsServersLocked_dev2(CMS_AF_SELECT_IPV6, staticDNSServers);

      rutNtwk_selectActiveIpvxDnsServers(CMS_AF_SELECT_IPV6,
                                         dnsClientObj->X_BROADCOM_COM_Ipv6_DnsIfNames,
                                         staticDNSServers,
                                         activeDnsIfName, activeDNServers6);

      *isStaticDNS6 = (0 == cmsUtl_strcmp(activeDnsIfName, CMS_FAKE_STATIC_DNS_IFNAME));

      cmsUtl_strcpy(ifName6, *isStaticDNS6 ? CMS_FAKE_STATIC_DNS_IFNAME : activeDnsIfName);

      /* got actiDNSServer6 and calling parseDNS for IPv6 addresses */
      cmsUtl_parseDNS(activeDNServers6, dns1, dns2, FALSE);
      cmsLog_debug("Active IPv6 from _dev2, dns1=%s, dns2=%s", dns1, dns2);

      cmsObj_free((void **)&dnsClientObj);
   }     
#endif /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */

   return ret;
}


static CmsRet getSubnetForWanConn(const char *wanIfName, char *subnetCidr4)
{
   char activeGwIfName[CMS_IFNAME_LENGTH]={0};
   CmsRet ret = CMSRET_SUCCESS;
   
   if ((ret = qdmRt_getActiveDefaultGatewayLocked(activeGwIfName)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get active default gateway?.  ret=%d", ret);
      return ret;
   }
   
   /* Need to get the lan subnet if this WAN service is part of the interface group OR
   * it is an active default gateway.  todo: IPv6 interface group ...
   */
   if (rutPMap_isWanUsedForIntfGroup(wanIfName) || !cmsUtl_strcmp(wanIfName, activeGwIfName))
   {
      if ((ret =  rutPMap_getSubnetFromWanIfName(wanIfName, subnetCidr4)) != CMSRET_SUCCESS)
      {
         return ret;
      }
   }
   else
   {
      /* this WAN has localloop back as the subnet */
      strcpy(subnetCidr4, "127.0.0.1/32");
   }

   return ret;
   
}


static CmsRet getDnsParam4(const char *wanIfName,  
                           const char *activeDNSServers, 
                           char *ifName4,
                           char *subnetCidr4, 
                           char *DNSServer4)
{
   CmsRet ret = CMSRET_SUCCESS;

   if (wanIfName == NULL ||  activeDNSServers == NULL ||ifName4 == NULL ||
      subnetCidr4 == NULL || DNSServer4 == NULL)
   {
      cmsLog_error("Null parameters.");
      return CMSRET_INVALID_ARGUMENTS;
   }
         
   if ((ret = getSubnetForWanConn(wanIfName, subnetCidr4)) == CMSRET_SUCCESS)
   {
      strcpy(ifName4, wanIfName);
      strcpy(DNSServer4, activeDNSServers);
   }       
   else
   {
      cmsLog_error("Failed to get subnet for %s", wanIfName);
      ret =  CMSRET_INTERNAL_ERROR;
   } 
   
   return ret;
}
#endif  /* #ifdef DMP_BASELINE_1 */


CmsRet rutDns_writeDnsInfoLine(FILE *fp,
                               const char *ifName4, 
                               const char *ifName6, 
                               const char *subnetCidr4, 
                               const char *subnetCidr6, 
                               const char *DNSServer4, 
                               const char *DNSServer6)
{
   char wanIfName[CMS_IFNAME_LENGTH]={0};
   char *processesList = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   if (!IS_EMPTY_STRING(ifName4))
   {
      strncpy(wanIfName, ifName4, sizeof(wanIfName));
   }
   else if (!IS_EMPTY_STRING(ifName6))
   {
      strncpy(wanIfName, ifName6, sizeof(wanIfName));
   }

#ifdef SUPPORT_TR69C
   if ((ret  = fillInTr69cProcessName(wanIfName, &processesList)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get process list  ret=%d", ret);
   }    
#endif

   /* Note:  other apps such as voip, etc need to add a similar function as fillInXXXXProcessName.
   * See fillInTr69cProcessName above.
   */

   /* do nothing for 0.0.0.0,0.0.0.0 in response to no DNS servers found or no WAN services up */

   cmsLog_debug( "%s,%s;%s,%s;%s,%s;%s\n", 
      ifName4 == NULL || !cmsUtl_strcmp(DNSServer4, "0.0.0.0,0.0.0.0") ? "" : ifName4,
      ifName6 == NULL || !cmsUtl_strcmp(DNSServer6, "0.0.0.0,0.0.0.0") ? "" : ifName6,
      subnetCidr4 == NULL || !cmsUtl_strcmp(DNSServer4, "0.0.0.0,0.0.0.0") ? "" : subnetCidr4,
      subnetCidr6 == NULL || !cmsUtl_strcmp(DNSServer6, "0.0.0.0,0.0.0.0") ? "" : subnetCidr6, 
      DNSServer4 == NULL || !cmsUtl_strcmp(DNSServer4, "0.0.0.0,0.0.0.0") ? "" : DNSServer4,
      DNSServer6 == NULL || !cmsUtl_strcmp(DNSServer6, "0.0.0.0,0.0.0.0") ? "" : DNSServer6, 
      processesList == NULL ?  "" : processesList);           

   fprintf(fp, "%s,%s;%s,%s;%s,%s;%s\n", 
      ifName4 == NULL || !cmsUtl_strcmp(DNSServer4, "0.0.0.0,0.0.0.0") ? "" : ifName4,
      ifName6 == NULL || !cmsUtl_strcmp(DNSServer6, "0.0.0.0,0.0.0.0") ? "" : ifName6,
      subnetCidr4 == NULL || !cmsUtl_strcmp(DNSServer4, "0.0.0.0,0.0.0.0") ? "" : subnetCidr4,
      subnetCidr6 == NULL || !cmsUtl_strcmp(DNSServer6, "0.0.0.0,0.0.0.0") ? "" : subnetCidr6, 
      DNSServer4 == NULL || !cmsUtl_strcmp(DNSServer4, "0.0.0.0,0.0.0.0") ? "" : DNSServer4,
      DNSServer6 == NULL || !cmsUtl_strcmp(DNSServer6, "0.0.0.0,0.0.0.0") ? "" : DNSServer6, 
      processesList == NULL ?  "" : processesList);

   CMSMEM_FREE_BUF_AND_NULL_PTR(processesList);

   return ret;

}


#ifdef DMP_BASELINE_1
static CmsRet getDefaultSubnetForStaticDNS(char *subnetCidr4, char *subnetCidr6)
{
   CmsRet ret;
   _LanIpIntfObject *lanIpObj = NULL;
   InstanceIdStack ipIntfIid = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;   
   
   if (subnetCidr4 == NULL || subnetCidr6 == NULL)
   {
      cmsLog_error("Invalid subnetCidr4/6 %p/%p", subnetCidr4, subnetCidr6);
      return CMSRET_INVALID_ARGUMENTS;
   }    

   /* Search for the ip Interface object on the br0 to get LAN subnet */
   while (!found && 
      ((ret = cmsObj_getNextFlags(MDMOID_LAN_IP_INTF, &ipIntfIid, OGF_NO_VALUE_UPDATE, (void **) &lanIpObj)) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(lanIpObj->X_BROADCOM_COM_IfName, "br0"))
      {
         /* IPv4 address */

         /* convert ipAddr/subnetMask to cidr format.
          * eg. 192.168.1.1 and 255.255.255.0 to 192.168.1.0/24
          */
         cmsNet_inet_ipv4AddrStrtoCidr4(lanIpObj->IPInterfaceIPAddress,
                                 lanIpObj->IPInterfaceSubnetMask,
                                 subnetCidr4);

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
         /* ipv6: refer to getDefaultLanSubnetInfo() */
         if (!qdmIpIntf_getIpv6DelegatedPrefixByNameLocked_dev2(lanIpObj->X_BROADCOM_COM_IfName, subnetCidr6))
         {
            char ipAddr[CMS_IPADDR_LENGTH]={0};

            if (qdmIpIntf_getIpv6AddressByNameLocked_dev2(lanIpObj->X_BROADCOM_COM_IfName, ipAddr) == CMSRET_SUCCESS)
            {
               char subnetLen[BUFLEN_8]={0};
               char *ptr = strchr(ipAddr, '/');
               int len = atoi(++ptr);

               cmsNet_subnetIp6SitePrefix(ipAddr, 0, len, subnetCidr6);
	       snprintf(subnetLen, sizeof(subnetLen), "/%d", len);
               cmsUtl_strncat(subnetCidr6, CMS_IPADDR_LENGTH, subnetLen);
            }
            else
            {
               cmsUtl_strcpy(subnetCidr6, "fe80::/64");
            }
         }
#endif /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */

         found = TRUE;            
     }
      cmsObj_free((void **) &lanIpObj);
   }

   if (found)
   {
      ret = CMSRET_SUCCESS;
   }
   else
   {
      ret = CMSRET_INTERNAL_ERROR;
   }

   cmsLog_debug("subnetCidr4=%s, subnetCidr6=%s, ret %d", subnetCidr4, subnetCidr6, ret);
   
   return ret;
   
}


CmsRet rutDns_dumpDnsInfo_igd(FILE *fp, UBOOL8 needDefaultDns)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanPppConnObject *pppCon=NULL;   
   _WanIpConnObject *ipCon=NULL;
   LanIpIntfObject *lanIpObj=NULL;
   char activeDNSServers4[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
   char activeDNSServers6[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
   char ifName4[CMS_IFNAME_LENGTH]={0};
   char subnetCidr4[CMS_IPADDR_LENGTH+3]={0};  // Need to add 3 to IPV4_ADDRLEN for CIDR
   char DNSServer4[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
   char ifName6[CMS_IFNAME_LENGTH]={0};
   char subnetCidr6[CMS_IPADDR_LENGTH]={0};
   char DNSServer6[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
   char IPoEGateway[CMS_IPADDR_LENGTH]={0};
#ifdef DMP_X_BROADCOM_COM_IPV6_1
   char IPoEGateway6[CMS_IPADDR_LENGTH]={0};
#endif
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 done = FALSE;
   UBOOL8 gotOneDns  = FALSE;
   UBOOL8 isStaticDNS = FALSE;
   UBOOL8 isStaticDNS6 = FALSE;
   UBOOL8 isStaticIPoE = FALSE;
#ifdef DMP_X_BROADCOM_COM_IPV6_1
   UBOOL8 isStaticIPoE6 = FALSE;
#endif
   
   cmsLog_debug("Enter, needDefaultDns=%d", needDefaultDns);
   
   if ((ret = getActiveDnsServers(activeDNSServers4, activeDNSServers6, &isStaticDNS, &isStaticDNS6, ifName4, ifName6)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   /* Static DNS is always the default system dns (first line in the dnsinfo.conf) */
   if (needDefaultDns && (isStaticDNS || isStaticDNS6))
   {
      if (getDefaultSubnetForStaticDNS(subnetCidr4, subnetCidr6) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get br0 lan subnet. ret %d", ret);
         return ret;         
      }
      if ((ret = rutDns_writeDnsInfoLine(fp, ifName4, ifName6, subnetCidr4, subnetCidr6, activeDNSServers4, activeDNSServers6)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Default system DNS is static: IPv4 % -- IPv6 %s", activeDNSServers4, activeDNSServers6);
         return ret;
      }     
      /* done for the default system static dns */
      return CMSRET_SUCCESS;
   }
   
   /* The default group need to be the first line in the dnsinfo.conf.  This is the first round to find that */
   while (!done &&
      (ret = cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppCon)) == CMSRET_SUCCESS)
   {
      gotOneDns = FALSE;
      
      /* Get IPv4 info first, if this IPv4 wan connection is up and activeDNSServer matches 
      * with this wan connection
      */
      if (rut_isWanInterfaceUp(pppCon->X_BROADCOM_COM_IfName, TRUE)) 
      {
         /* 2 cases to get IPv4 parameter for the dns line:
         * 1) if needDefaultDns is TRUE and this IPv4 WAN DNSServers is same as active DNSServers
         * 2) if needDefaultDns is FASLE and this IPv4 WAN DNSServers differs active DNSServers
         */
         if (needDefaultDns == !cmsUtl_strcmp(pppCon->DNSServers, activeDNSServers4))
         {            
            ret = getDnsParam4(pppCon->X_BROADCOM_COM_IfName, pppCon->DNSServers, ifName4, subnetCidr4, DNSServer4);
            gotOneDns = TRUE;
         }
      }

      /* Get IPv6 info next, if this IPv6 wan connection is up and then get activeDNSServer6.
      *  Currently, interface grouping is not supported in IPv6 yet.
      */
      if (ret == CMSRET_SUCCESS && qdmIpIntf_isWanInterfaceUpLocked(pppCon->X_BROADCOM_COM_IfName, FALSE) )
      {
         /* Only need to get IPv6 parameter for the dns line if this ipv6 wan is an activeDNSServer */
         if (isActiveDNSServer6(pppCon->X_BROADCOM_COM_IfName, activeDNSServers6))
         {
            /* 2 conditions also need to be met to write out dns6 line:
            *   1) if needDefaultDns is TRUE and this IPv6 WAN DNSServers is same as active DNSServers4
            *   2) if needDefaultDns is FASLE and this IPv6 WAN DNSServers not ipv4 WAN DNSServers4
            */         
            if ((needDefaultDns && !cmsUtl_strcmp(pppCon->DNSServers, activeDNSServers4)) ||
                (!needDefaultDns && cmsUtl_strcmp(pppCon->DNSServers, activeDNSServers4)))
            {
               ret = getDnsParam6(pppCon->X_BROADCOM_COM_IfName,  activeDNSServers6, ifName6, subnetCidr6, DNSServer6);
               gotOneDns = TRUE;
            }
         }
      }

      cmsObj_free((void **)&pppCon);
      
      if (ret != CMSRET_SUCCESS)
      {
         return ret;
      }
      
      if (gotOneDns)
      {
         if ((ret = rutDns_writeDnsInfoLine(fp, ifName4, ifName6, subnetCidr4, subnetCidr6, DNSServer4, DNSServer6)) != CMSRET_SUCCESS)
         {
            return ret;
         }  
         if (needDefaultDns)
         {
            done = TRUE;      
         }
         else
         {
            char dns1[CMS_IFNAME_LENGTH]={0};
            char dns2[CMS_IFNAME_LENGTH];
            char cmd[BUFLEN_128]={0};

            /* If not  default dns (not first line), need to add the static route for the WAN dns.  
            * Always use 32 bit as mask for IPv4. 
            */
            rutRt_addSystemStaticRoute(DNSServer4, "255.255.255.255", "", ifName4);

            if ((cmsUtl_parseDNS(DNSServer6, dns1, dns2, FALSE) == CMSRET_SUCCESS) &&
                !cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPV6, dns1) &&
                cmsUtl_isValidIpAddress(AF_INET6, dns1))
            {
               snprintf(cmd, sizeof(cmd), "route -A inet6 add %s dev %s 2>/dev/null", dns1, ifName6);
               rut_doSystemAction("Add IPv6 DNS static route", cmd);
            }
         }         
      }
      
   }
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!done && 
      (ret = cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipCon)) == CMSRET_SUCCESS)
   {
      gotOneDns = FALSE;
      
      /* Get IPv4 info first, if this IPv4 wan connection is up and activeDNSServer matches 
      * with this wan connection
      */
      if (rut_isWanInterfaceUp(ipCon->X_BROADCOM_COM_IfName, TRUE)) 
      {
         /* Skip bridge wan interface since no dns entry needed */
         if (cmsUtl_strcmp(ipCon->connectionType, MDMVS_IP_BRIDGED))
         {
            /* 2 cases to get IPv4 parameter for the dns line:
            * 1) if needDefaultDns is TRUE and this IPv4 WAN DNSServers is same as active DNSServers
            * 2) if needDefaultDns is FASLE and this IPv4 WAN DNSServers differs active DNSServers
            */
            if (needDefaultDns == !cmsUtl_strcmp(ipCon->DNSServers, activeDNSServers4))
            {            
               /* For static routed WAN, the active DNSServer is not in ipCon->DNSServers, just copy it over for later compare
               * if it is empty string (should be) 
               */
               if (!cmsUtl_strcmp(ipCon->connectionType, MDMVS_IP_ROUTED) && 
                  !cmsUtl_strcmp(ipCon->addressingType, MDMVS_STATIC) && 
                  IS_EMPTY_STRING(ipCon->DNSServers))
               {
                  CMSMEM_REPLACE_STRING_FLAGS(ipCon->DNSServers, activeDNSServers4, mdmLibCtx.allocFlags);      
               }         
               
               ret = getDnsParam4(ipCon->X_BROADCOM_COM_IfName, ipCon->DNSServers, ifName4, subnetCidr4, DNSServer4);
               gotOneDns = TRUE;
            }
         }            
      }
      
      /* Get IPv6 info next, if this IPv6 wan connection is up and then get activeDNSServer6.
      *  Currently, interface grouping is not supported in IPv6 yet.
      */

      if (ret == CMSRET_SUCCESS && qdmIpIntf_isWanInterfaceUpLocked(ipCon->X_BROADCOM_COM_IfName, FALSE) )
      {
         /* Only need to get IPv6 parameter for the dns line if this ipv6 wan is an activeDNSServer6 */
         if (isActiveDNSServer6(ipCon->X_BROADCOM_COM_IfName, activeDNSServers6))
         {
            /* 2 conditions also need to be met to write out dns6 line:
            *   1) if needDefaultDns is TRUE and this IPv6 WAN DNSServers is same as active DNSServers4
            *   2) if needDefaultDns is FASLE and this IPv6 WAN DNSServers not ipv4 WAN DNSServers4
            */         
            if ((needDefaultDns && !cmsUtl_strcmp(ipCon->DNSServers, activeDNSServers4)) ||
                (!needDefaultDns && cmsUtl_strcmp(ipCon->DNSServers, activeDNSServers4)))

            {
               ret = getDnsParam6(ipCon->X_BROADCOM_COM_IfName, activeDNSServers6, ifName6, subnetCidr6, DNSServer6);
               gotOneDns = TRUE;
            }
         }
      }
      /* Need this for static route for WAN dns below */
      cmsUtl_strncpy(IPoEGateway, ipCon->defaultGateway, sizeof(IPoEGateway));
      /* Need this for static IPoE checking later on */
      isStaticIPoE = !cmsUtl_strcmp(ipCon->addressingType, MDMVS_STATIC);
#ifdef DMP_X_BROADCOM_COM_IPV6_1
      cmsUtl_strncpy(IPoEGateway6, ipCon->X_BROADCOM_COM_DefaultIPv6Gateway, sizeof(IPoEGateway6));
      isStaticIPoE6 = !cmsUtl_strcmp(ipCon->X_BROADCOM_COM_IPv6AddressingType, MDMVS_STATIC);
#endif

      /* FIXME: for Hybrid98+181 or Pure181 are there any specific clues to address default IPv6 gateway? */

      cmsObj_free((void **)&ipCon);
      
      if (ret != CMSRET_SUCCESS)
      {
         return ret;
      }
      
      if (gotOneDns)
      {
         if ((ret = rutDns_writeDnsInfoLine(fp, ifName4, ifName6, subnetCidr4, subnetCidr6, DNSServer4, DNSServer6)) != CMSRET_SUCCESS)
         {
            return ret;
         } 
         if (needDefaultDns)
         {
            done = TRUE;      
         }
         else
         {
            /* If not  default dns (not first line), need to add the static route for the WAN dns.  
            * Always use 32 bit as mask for IPv4.  todo: IPv6 
            */
            if (isStaticIPoE)
            {
               /* For static non-default dns IPoE , the static route is from the default dns interface */
               _NetworkConfigObject *networkCfg=NULL;
               InstanceIdStack NetworkCfgIid = EMPTY_INSTANCE_ID_STACK;
               char defaultDNSIfName[CMS_IFNAME_LENGTH]={0};
               _WanIpConnObject *defaultDnsIp =NULL;
               InstanceIdStack defaultDnsIpIid = EMPTY_INSTANCE_ID_STACK;
               UBOOL8 found = FALSE;

               if ((ret = cmsObj_get(MDMOID_NETWORK_CONFIG, &NetworkCfgIid, OGF_NO_VALUE_UPDATE, (void *) &networkCfg)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("Could not get NETWORK_CONFIG. ret=%d", ret);
                  return ret;
               }  
               else
               {
                  cmsUtl_strncpy(defaultDNSIfName, networkCfg->DNSIfName, sizeof(defaultDNSIfName)-1);
               }
               while (!found && 
                  cmsObj_getNext(MDMOID_WAN_IP_CONN, &defaultDnsIpIid, (void **) &defaultDnsIp) == CMSRET_SUCCESS)
               {
                  if (!cmsUtl_strcmp(defaultDnsIp->X_BROADCOM_COM_IfName, defaultDNSIfName))
                  {
                     char dns1[CMS_IFNAME_LENGTH]={0};
                     char dns2[CMS_IFNAME_LENGTH];                  
                     char cmd[BUFLEN_128]={0};
                     
                     found = TRUE;
                     /* Add the static route for the system default dns for this static IPoE wan */
                     if ((cmsUtl_parseDNS(defaultDnsIp->DNSServers, dns1, dns2, TRUE) == CMSRET_SUCCESS) &&
                         !cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPV4, dns1) &&
                         cmsUtl_isValidIpv4Address(dns1))

                     {
                        snprintf(cmd,  sizeof(cmd), "route add %s gw %s dev %s 2>/dev/null", 
                                                     dns1, defaultDnsIp->defaultGateway, defaultDNSIfName);
                        rut_doSystemAction("Add DNS static route", cmd);
                     }                        

                  }
                  cmsObj_free((void **) &defaultDnsIp);
               }
            }
            else
            {
               /* For Dynamic non-default dns IPoE */
               rutRt_addSystemStaticRoute(DNSServer4, "255.255.255.255", IPoEGateway, ifName4);
            }
#ifdef DMP_X_BROADCOM_COM_IPV6_1
            if (isStaticIPoE6)
            {
               _IPv6LanHostCfgObject *ipv6LanCfgObj=NULL;
               InstanceIdStack Ipv6LanCfgIid = EMPTY_INSTANCE_ID_STACK;
               char defaultDNSIfName[CMS_IFNAME_LENGTH]={0};
               _WanIpConnObject *defaultDnsIp =NULL;
               InstanceIdStack defaultDnsIpIid = EMPTY_INSTANCE_ID_STACK;
               UBOOL8 found = FALSE;

               if ((ret = cmsObj_getNext(MDMOID_I_PV6_LAN_HOST_CFG, &Ipv6LanCfgIid, (void *) &ipv6LanCfgObj)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("Could not get MDMOID_I_PV6_LAN_HOST_CFG. ret=%d", ret);
                  return ret;
               }
               else
               {
                  cmsUtl_strncpy(defaultDNSIfName, ipv6LanCfgObj->IPv6DNSWANConnection, sizeof(defaultDNSIfName) - 1);
               }
               while (!found &&
                  cmsObj_getNext(MDMOID_WAN_IP_CONN, &defaultDnsIpIid, (void **) &defaultDnsIp) == CMSRET_SUCCESS)
               {
                  if (!cmsUtl_strcmp(defaultDnsIp->X_BROADCOM_COM_IfName, defaultDNSIfName))
                  {
                     char dns1[CMS_IFNAME_LENGTH]={0};
                     char dns2[CMS_IFNAME_LENGTH];
                     char cmd[BUFLEN_128]={0};

                     found = TRUE;
                     if ((cmsUtl_parseDNS(defaultDnsIp->X_BROADCOM_COM_IPv6DNSServers, dns1, dns2, FALSE) == CMSRET_SUCCESS) &&
                         !cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPV6, dns1) &&
                         cmsUtl_isValidIpAddress(AF_INET6, dns1))
                     {
                        if (!IS_EMPTY_STRING(defaultDnsIp->X_BROADCOM_COM_DefaultIPv6Gateway)) 
                            snprintf(cmd,  sizeof(cmd), "route -A inet6 add %s gw %s dev %s 2>/dev/null",
                                                     dns1, defaultDnsIp->X_BROADCOM_COM_DefaultIPv6Gateway, defaultDNSIfName);
                        else 
                            snprintf(cmd,  sizeof(cmd), "route -A inet6 add %s dev %s 2>/dev/null",
                                                     dns1, defaultDNSIfName);
                        rut_doSystemAction("Add IPv6 DNS static route", cmd);
                     }                        
                  }
                  cmsObj_free((void **) &defaultDnsIp);
               }
            }
            else
            {
               char dns1[CMS_IFNAME_LENGTH]={0};
               char dns2[CMS_IFNAME_LENGTH];
               char cmd[BUFLEN_128]={0};

               if ((cmsUtl_parseDNS(DNSServer6, dns1, dns2, FALSE) == CMSRET_SUCCESS) &&
                   !cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPV6, dns1) &&
                   cmsUtl_isValidIpAddress(AF_INET6, dns1))
               {
                  if (!IS_EMPTY_STRING(IPoEGateway6)) 
                      snprintf(cmd, sizeof(cmd), "route -A inet6 add %s gw %s dev %s 2>/dev/null", dns1, IPoEGateway6, ifName6);
                  else 
                      snprintf(cmd, sizeof(cmd), "route -A inet6 add %s dev %s 2>/dev/null", dns1, ifName6);

                  rut_doSystemAction("Add IPv6 DNS static route", cmd);
               }
            }
#endif
         }
      }

   }

   /*
    * this is needed for homeplug using dhcpc on LAN side.  In this
    * scenario, we might use br0 as the default DNS server and it is not
    * specified in the list of intf where we are supposed to get DNS servers
    * from.  So only look on LAN side if we are writing out the first line
    * (needDefaultDns == TRUE)
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!done &&
          (needDefaultDns == TRUE) &&
          (ret = cmsObj_getNextFlags(MDMOID_LAN_IP_INTF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &lanIpObj)) == CMSRET_SUCCESS)
   {
      gotOneDns = FALSE;

      /* IPv4 */
      if (!cmsUtl_strcmp(lanIpObj->IPInterfaceAddressingType, MDMVS_DHCP) &&
          !cmsUtl_strcmp(lanIpObj->X_BROADCOM_COM_DhcpConnectionStatus, MDMVS_CONNECTED) &&
          !IS_EMPTY_STRING(lanIpObj->X_BROADCOM_COM_DNSServers))
      {
         cmsUtl_strcpy(ifName4, lanIpObj->X_BROADCOM_COM_IfName);
         cmsUtl_strcpy(DNSServer4, lanIpObj->X_BROADCOM_COM_DNSServers);
         sprintf(subnetCidr4, "%s/%d",
                              lanIpObj->IPInterfaceSubnetMask,
                              cmsNet_getLeftMostOneBitsInMask(lanIpObj->IPInterfaceSubnetMask));
         cmsLog_debug("LAN side: %s %s %s",
                   lanIpObj->X_BROADCOM_COM_IfName,
               lanIpObj->X_BROADCOM_COM_DNSServers,
               subnetCidr4);
         gotOneDns = TRUE;
      }

      /*
       * IPv6 DNS Server names on LAN side are not supported on the old
       * Broadcom proprietary IPv6.
       */

      cmsObj_free((void **)&lanIpObj);

      if (ret != CMSRET_SUCCESS)
      {
         return ret;
      }

      if (gotOneDns)
      {
         if ((ret = rutDns_writeDnsInfoLine(fp, ifName4, ifName6, subnetCidr4, subnetCidr6, DNSServer4, DNSServer6)) != CMSRET_SUCCESS)
         {
            return ret;
         }
         if (needDefaultDns)
         {
            done = TRUE;
         }
      }
   }

   if (ret == CMSRET_NO_MORE_INSTANCES)
   {
      ret = CMSRET_SUCCESS;
   }

   cmsLog_debug("Exit. ret=%d", ret);

   return ret;
   
}
#endif  /* DMP_BASELINE_1 */


CmsRet rutDns_createDnsInfoConf(void)
{
   CmsRet ret = CMSRET_SUCCESS;
   FILE *fsDns = NULL;
   UBOOL8 isDefaultDns = TRUE;
   char dnsPath[CMS_MAX_FULLPATH_LENGTH]={0};

   cmsLog_debug("Enter");

   ret = cmsUtl_getRunTimePath(DNSINFO_CONF, dnsPath, sizeof(dnsPath));
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }
   
   cmsLog_debug("opening file %s", dnsPath);

   if ((fsDns = fopen(dnsPath, "w")) == NULL)
   {
      cmsLog_error ("Failed to create %s", dnsPath);
      return CMSRET_OPEN_FILE_ERROR;
   }

   /*
    * DumpDnsInfo is called twice: first time, isDefaultDns == TRUE,
    * meaning dump out exactly one line which describes the system
    * default DNS server info.
    * Second time, isDefaultDns == FALSE, now dump out one line per
    * UP and connected WAN connection, but exclude the WAN connection that
    * was used as the default.  See rut_dns.h for format of the file.
    */
   if ((ret = rutDns_dumpDnsInfo(fsDns, isDefaultDns)) == CMSRET_SUCCESS)
   {
      isDefaultDns = FALSE;
      ret = rutDns_dumpDnsInfo(fsDns, isDefaultDns);
   }
   
   fclose(fsDns);

   if (ret != CMSRET_SUCCESS)
   {
      unlink(dnsPath);
      cmsLog_notice("Failed to create %s", dnsPath);
   }
   else
   {
      cmsLog_debug("%s is created", dnsPath);
   }
   
   return ret;
}


void rutDns_writeStaticHosts_igd(FILE *fp)
{
   LanHostEntryObject *hostObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   while ((ret = cmsObj_getNextFlags(MDMOID_LAN_HOST_ENTRY,
                                     &iidStack, OGF_NO_VALUE_UPDATE,
                                     (void **) &hostObj)) == CMSRET_SUCCESS)
   {
      if (!IS_EMPTY_STRING(hostObj->IPAddress) &&
          !IS_EMPTY_STRING(hostObj->hostName))
      {
         fprintf(fp, "%s\t%s\n", hostObj->IPAddress, hostObj->hostName);
      }
      cmsObj_free((void **) &hostObj);
   }

   return;
}
