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

#include "cms.h"
#include "cms_util.h"
#include "cms_mdm.h"
#include "cms_obj.h"
#include "cms_qdm.h"
#include "mdm.h"
#include "rut_dns.h"
#include "rut2_dns.h"
#include "rut_dnsproxy.h"
#include "rut_network.h"


CmsRet rutDns_configIpvxDnsServers_dev2(UINT32 ipvx,
                                        const char *ipIntfFullPath,
                                        UBOOL8 activate)
{
   Dev2DnsClientObject *dnsClientObj=NULL;
   Dev2DnsServerObject *dnsServerObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack savedIidStack=EMPTY_INSTANCE_ID_STACK;
   char activeDnsIfName[CMS_IFNAME_LENGTH]={0};
   char activeDnsServers[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
   char staticDNSServers[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};
   CmsRet ret;

   cmsLog_debug("Enter: ipvx=%d activate=%d ipIntffullPath=%s",
                 ipvx, activate, ipIntfFullPath);

   if ((ipvx & CMS_AF_SELECT_IPVX) == CMS_AF_SELECT_IPVX)
   {
      cmsLog_error("cannot set ipvx to both IPv4 and IPv6 for this func");
      return CMSRET_INVALID_ARGUMENTS;
   }


   ret = cmsObj_get(MDMOID_DEV2_DNS_CLIENT, &iidStack, OGF_NO_VALUE_UPDATE,
                    (void**)&dnsClientObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get MDMOID_DEV2_DNS_CLIENT, ret=%d", ret);
      return ret;
   }

   qdmDns_getStaticIpvxDnsServersLocked_dev2(ipvx, staticDNSServers);

   /*
    * First figure out which DNS server from multiple UP Wan connections
    * is the System DNS server.  This is like the fetchActiveDefaultGateway
    * in the routing code.
    */
   if (ipvx & CMS_AF_SELECT_IPV4)
   {
      ret = rutNtwk_selectActiveIpvxDnsServers(ipvx,
                                       dnsClientObj->X_BROADCOM_COM_DnsIfNames,
                                       staticDNSServers,
                                       activeDnsIfName, activeDnsServers);
      if (ret == CMSRET_SUCCESS)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dnsClientObj->X_BROADCOM_COM_ActiveDnsIfName,
                                    activeDnsIfName, mdmLibCtx.allocFlags);

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dnsClientObj->X_BROADCOM_COM_ActiveDnsServers,
                                    activeDnsServers, mdmLibCtx.allocFlags);
      }
   }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   if (ipvx & CMS_AF_SELECT_IPV6)
   {
      ret = rutNtwk_selectActiveIpvxDnsServers(ipvx,
                                dnsClientObj->X_BROADCOM_COM_Ipv6_DnsIfNames,
                                staticDNSServers,
                                activeDnsIfName, activeDnsServers);
      if (ret == CMSRET_SUCCESS)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsIfName,
                                           activeDnsIfName, mdmLibCtx.allocFlags);
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsServers,
                                           activeDnsServers, mdmLibCtx.allocFlags);
      }
   }
#endif

   if (ret != CMSRET_SUCCESS)
   {
      cmsObj_free((void **) &dnsClientObj);
      return ret;
   }

   /*
    * record the default system DNS servers in the DNS.Client object.
    * This cmsObj_set will not trigger any actions.  Just bookkeeping.
    */
   ret = cmsObj_set(dnsClientObj, &iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to set MDMOID_DEV2_DNS_CLIENT, ret=%d", ret);
       /* the set should not fail.  Try to keep going anyways. */
   }
   cmsObj_free((void **) &dnsClientObj);


   /* ==> this triggers the reconfiguration of all DNS related stuff */
   rutNtwk_configActiveDnsIp();


   /*
    * Now traverse all the individual server entries for bookkeeping
    * purposes only.  No action.
    *
    * If deactivate, delete any Server objects which point to the
    * ipIntfFullPath and is not type Static.
    *
    * If activate, do a "set" on all Server objects which point to the
    * interface which just came up.  This allows them to update their
    * status.  A bit silly to call the RCL just to update the status,
    * but makes it symmetrical with rutRt_activateLayer3Ipv4Forwarding.
    */
   while(cmsObj_getNextFlags(MDMOID_DEV2_DNS_SERVER,
                             &iidStack, OGF_NO_VALUE_UPDATE,
                             (void **) &dnsServerObj) == CMSRET_SUCCESS)
   {
      /*
       * A matching entry is:
       * match on interface fullpath AND
       *       user selects IPv4 and this is a dynamic IPv4 entry, or
       *       user selects IPv6 and this is a dynamic IPv6 entry.
       */
      if ((!cmsUtl_strcmp(dnsServerObj->interface, ipIntfFullPath)) &&
             (((ipvx & CMS_AF_SELECT_IPV4) &&
                 (!cmsUtl_strcmp(dnsServerObj->type, MDMVS_DHCPV4) ||
                  !cmsUtl_strcmp(dnsServerObj->type, MDMVS_IPCP)))       ||
             ((ipvx & CMS_AF_SELECT_IPV6) &&
                 (!cmsUtl_strcmp(dnsServerObj->type, MDMVS_DHCPV6) ||
                  !cmsUtl_strcmp(dnsServerObj->type, MDMVS_ROUTERADVERTISEMENT)))))
     {
         if (activate)
         {
            if ((ret = cmsObj_set(dnsServerObj, &iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsObj_set <MDMOID_DEV2_DNS_SERVER> returns error. ret=%d", ret);
            }
         }
         else
         {
            cmsObj_deleteInstance(MDMOID_DEV2_DNS_SERVER, &iidStack);
            /* since we deleted instance here, restore iidStack
             * to the last saved (non-deleted) one.
             */
            iidStack = savedIidStack;
         }
      }

      cmsObj_free((void **) &dnsServerObj);

      /* save iidStack in case we do a delete in the next iteration */
      savedIidStack = iidStack;
   }

   cmsLog_debug("Exit. ret=%d", ret);
   
   return ret;
}


CmsRet rutDns_activateIpv4DnsServers_dev2(const char *ipIntfFullPath)
{
   UBOOL8 activate=TRUE;

   return (rutDns_configIpvxDnsServers_dev2(CMS_AF_SELECT_IPV4,
                                            ipIntfFullPath, activate));
   
}


CmsRet rutDns_deactivateIpv4DnsServers_dev2(const char *ipIntfFullPath)
{
   UBOOL8 activate=FALSE;

   return (rutDns_configIpvxDnsServers_dev2(CMS_AF_SELECT_IPV4,
                                            ipIntfFullPath, activate));
   
}


CmsRet rutDns_addServerObject_dev2(const char *ipIntfFullPath, const char *DNSServers, const char *srcType)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2DnsServerObject *dnsServerObj=NULL;
   char tmpDNSServers[BUFLEN_48] = {0};
   CmsRet ret;

   cmsLog_debug("fullPath=%s servers=%s srcType=%s",
                ipIntfFullPath, DNSServers, srcType);

   if ((ret = cmsObj_addInstance(MDMOID_DEV2_DNS_SERVER, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add MDMOID_DEV2_DNS_SERVER Instance, ret = %d", ret);
      return ret;
   } 

   if ((ret = cmsObj_get(MDMOID_DEV2_DNS_SERVER, &iidStack, 0, (void **) &dnsServerObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get dnsServerObj, ret = %d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_DNS_SERVER, &iidStack);
      return ret;
   }

   dnsServerObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(dnsServerObj->interface, ipIntfFullPath, mdmLibCtx.allocFlags);
   /* We use DNSServers to store multiple DNS Servers information. It might be overflowed! */
   if (DNSServers)
   {
      cmsUtl_strncpy(tmpDNSServers, DNSServers, sizeof(tmpDNSServers)-1);
      if (strlen(DNSServers) >= sizeof(tmpDNSServers))
      {
         char *ptr = NULL;
         ptr = strrchr(tmpDNSServers, ',');
         if (ptr) *ptr = '\0';
      }
   }
   CMSMEM_REPLACE_STRING_FLAGS(dnsServerObj->DNSServer, tmpDNSServers, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(dnsServerObj->type, srcType, mdmLibCtx.allocFlags);

   if((ret = cmsObj_set (dnsServerObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Set of dnsServerObj object failed, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_DNS_SERVER, &iidStack);
   }
   else
   {
      /*
       * This function should only be called when srcType != MDMVS_STATIC, but
       * check anyways just to make this code absolutely correct.
       */
      if (cmsUtl_strcmp(srcType, MDMVS_STATIC))
      {
         if ((ret = cmsObj_setNonpersistentInstance(MDMOID_DEV2_DNS_SERVER, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set dnsServerObj. ret=%d", ret);
         }
      }
   }
   cmsObj_free((void **) &dnsServerObj);

   return ret;
}


CmsRet rutNtwk_getIpv4DnsServersFromIfName_dev2(const char *ifName, char *DNSServers)
{
   return (rutNtwk_getIpvxDnsServersFromIfName_dev2(CMS_AF_SELECT_IPV4,
                                                    ifName, DNSServers));
}


CmsRet rutNtwk_getIpvxDnsServersFromIfName_dev2(UINT32 ipvx, const char *ifName, char *DNSServers)
{
   Dev2DnsServerObject *dnsServerObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   char *ipIntfFullPath=NULL;
   UBOOL8 found=FALSE;
   UBOOL8 isLayer2=FALSE;
   CmsRet ret;

   cmsLog_debug("Entered: ipvx=%d ifName=%s", ipvx, ifName);

   if (IS_EMPTY_STRING(ifName) || !DNSServers)
   {
      cmsLog_error("NULL string.");
      return CMSRET_INTERNAL_ERROR;
   }
   DNSServers[0] = '\0';

   /*
    * This function may get called in Hybrid mode, so we must call the _dev2
    * version of qdmIntf_intfnameToFullPathLocked in order to get the
    * path to the TR181 object.
    */
   ret = qdmIntf_intfnameToFullPathLocked_dev2(ifName, isLayer2, &ipIntfFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get fullpath_dev2 for %s, ret=%d", ifName, ret);
      return ret;
   }

   cmsLog_debug("ifName %s => %s", ifName, ipIntfFullPath);

   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_DNS_SERVER,
                             &iidStack, OGF_NO_VALUE_UPDATE,
                             (void **) &dnsServerObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(dnsServerObj->interface, ipIntfFullPath) && 
          !IS_EMPTY_STRING(dnsServerObj->DNSServer))
      {
         UBOOL8 isIpv4Addr;
         char dns1[CMS_IPADDR_LENGTH]={0};
         char dns2[CMS_IPADDR_LENGTH]={0};

         /* parse servers to see if they are IPv4 or IPv6 */
         memset(dns1, 0, sizeof(dns1));
         if ((cmsUtl_parseDNS(dnsServerObj->DNSServer, dns1, dns2, TRUE) == CMSRET_SUCCESS) &&
             !cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPV4, dns1) &&
             cmsUtl_isValidIpv4Address(dns1))
         {
            isIpv4Addr = TRUE;
         }
         else
         {
            isIpv4Addr = FALSE;
         }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
         if (!isIpv4Addr && (ipvx & CMS_AF_SELECT_IPV6))
         {
            found = TRUE;
            cmsUtl_strncpy(DNSServers, dnsServerObj->DNSServer,
                           (CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH));
         }
#endif

         if (!found && isIpv4Addr && (ipvx & CMS_AF_SELECT_IPV4))
         {
            found = TRUE;
            cmsUtl_strncpy(DNSServers, dnsServerObj->DNSServer,
                           (CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH));
         }
      }

      cmsObj_free((void **) &dnsServerObj);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);

   if (found)
   {
      cmsLog_debug("Got DNSServers %s on %s (ret=%d)", DNSServers, ifName, ret);
   }
   else
   {
      /* this is possible when the DNS servers are statically configured.
       * Statically configured DNS servers are not tied to any interface.
       */
      cmsLog_debug("No DNSServers on %s (ret=%d)", ifName, ret);
   }

   return ret;
}


void rutNtwk_removeIpvxDnsIfNameFromList_dev2(UINT32 ipvx, const char *ifName)
{
   Dev2DnsClientObject *dnsClientObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("Enter: ipvx=%d ifName=%s", ipvx, ifName);

   ret = cmsObj_get(MDMOID_DEV2_DNS_CLIENT, &iidStack, OGF_NO_VALUE_UPDATE,
                    (void**)&dnsClientObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get MDMOID_DEV2_DNS_CLIENT, ret=%d", ret);
      return;
   }

   if (!IS_EMPTY_STRING(ifName))
   {
      if ((ipvx & CMS_AF_SELECT_IPV4) &&
          !IS_EMPTY_STRING(dnsClientObj->X_BROADCOM_COM_DnsIfNames) &&
          strstr(dnsClientObj->X_BROADCOM_COM_DnsIfNames, ifName))
      {
         rutNtwk_removeIfNameFromList(ifName, dnsClientObj->X_BROADCOM_COM_DnsIfNames);
      }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      if ((ipvx & CMS_AF_SELECT_IPV4) &&
          !IS_EMPTY_STRING(dnsClientObj->X_BROADCOM_COM_Ipv6_DnsIfNames) &&
          strstr(dnsClientObj->X_BROADCOM_COM_Ipv6_DnsIfNames, ifName))
      {
         rutNtwk_removeIfNameFromList(ifName, dnsClientObj->X_BROADCOM_COM_Ipv6_DnsIfNames);
      }
#endif
      
      ret = cmsObj_set(dnsClientObj, &iidStack);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to set MDMOID_DEV2_DNS_CLIENT, ret=%d", ret);
      }
   }

   cmsObj_free((void **) &dnsClientObj);

   return;
}


/* basically the same as getDefaultSubnetForStaticDNS in rut_dns.c */
static void getDefaultLanSubnetInfo(char *subnetCidr4, char *subnetCidr6)
{
   char ifNameBuf[CMS_IFNAME_LENGTH]={0};
   char ipv4AddrBuf[CMS_IPADDR_LENGTH]={0};
   char ipv4SubnetMaskBuf[CMS_IPADDR_LENGTH]={0};
   UBOOL8 isWan=FALSE;
   UBOOL8 isStatic=FALSE;


   qdmIpIntf_getDefaultLanIntfNameLocked_dev2(ifNameBuf);

   qdmIpIntf_getIpv4AddrInfoByNameLocked_dev2(ifNameBuf,
                                      ipv4AddrBuf, ipv4SubnetMaskBuf,
                                      &isWan, &isStatic);

   /* convert ipv4addr and subnet to cidr4 */
   cmsNet_inet_ipv4AddrStrtoCidr4(ipv4AddrBuf, ipv4SubnetMaskBuf, subnetCidr4);
   cmsLog_debug("ipv4AddrBuf=%s ipv4SubnetMaskBuf=%s cidr4=%s",
                ipv4AddrBuf, ipv4SubnetMaskBuf, subnetCidr4);


#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   /* FIXME: if no delegated prefix is found, should we use LLA here? */
   if (subnetCidr6 && !qdmIpIntf_getIpv6DelegatedPrefixByNameLocked_dev2(ifNameBuf, subnetCidr6))
   {
      char ipv6AddrBuf[CMS_IPADDR_LENGTH]={0};

      /* give ourselves one more chance to see whether there is a manually configured address */
      if (qdmIpIntf_getIpv6AddressByNameLocked_dev2(ifNameBuf, ipv6AddrBuf) == CMSRET_SUCCESS)
      {
         char *ptr = strchr(ipv6AddrBuf, '/');
         int len = atoi(++ptr);

         /*
          * Special case where we host DNS server on the LAN side,
          * but with NO up WAN connection.
          */
         cmsNet_subnetIp6SitePrefix(ipv6AddrBuf, 0, len, subnetCidr6);
         sprintf(subnetCidr6, "%s/%d", subnetCidr6, len);
      }
      else
      {
         strcpy(subnetCidr6, "fe80::/64");
      }
   }
#endif

   return;
}


CmsRet rutDns_dumpDnsInfo_dev2(FILE *fp, UBOOL8 needDefaultDns)
{
   Dev2DnsClientObject *dnsClientObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   char subnetCidr4[CMS_IPADDR_LENGTH+3]={0};  // Need to add 3 to IPV4_ADDRLEN for CIDR
   char ifName6[CMS_IFNAME_LENGTH]={0};  // XXX IPv6 not handled for now
   char subnetCidr6[CMS_IPADDR_LENGTH]={0};  // XXX IPv6 not handled for now
   char dnsServers6[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};    // XXX IPv6 not handled for now
   CmsRet ret;

   cmsLog_debug("Enter: needDefaultDns=%d", needDefaultDns);

   ret = cmsObj_get(MDMOID_DEV2_DNS_CLIENT, &iidStack, OGF_NO_VALUE_UPDATE,
                    (void**)&dnsClientObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get MDMOID_DEV2_DNS_CLIENT, ret=%d", ret);
      return ret;
   }


   if (needDefaultDns)
   {
      /*
       * First pass: dump the default system DNS (if there is one).
       * This is either the static system DNS or the dynamically acquired DNS
       * server info from some WAN connection.
       */
      if (!IS_EMPTY_STRING(dnsClientObj->X_BROADCOM_COM_ActiveDnsIfName) &&
          !IS_EMPTY_STRING(dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsIfName))
      {
         getDefaultLanSubnetInfo(subnetCidr4, subnetCidr6);

         cmsLog_debug("first pass: ActiveDnsIfName=%s ActiveDnsServers=%s ActiveDnsv6IfName=%s ActiveDnsv6Servers=%s",
                      dnsClientObj->X_BROADCOM_COM_ActiveDnsIfName,
                      dnsClientObj->X_BROADCOM_COM_ActiveDnsServers,
                      dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsIfName,
                      dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsServers);

         rutDns_writeDnsInfoLine(fp,
                   dnsClientObj->X_BROADCOM_COM_ActiveDnsIfName,
                   dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsIfName,
                   subnetCidr4, subnetCidr6,
                   dnsClientObj->X_BROADCOM_COM_ActiveDnsServers,
                   dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsServers);
      }
      else if (!IS_EMPTY_STRING(dnsClientObj->X_BROADCOM_COM_ActiveDnsIfName)) /* only IPv4 */
      {
         getDefaultLanSubnetInfo(subnetCidr4, subnetCidr6);

         cmsLog_debug("first pass: ActiveDnsIfName=%s ActiveDnsServers=%s",
                      dnsClientObj->X_BROADCOM_COM_ActiveDnsIfName,
                      dnsClientObj->X_BROADCOM_COM_ActiveDnsServers);

         rutDns_writeDnsInfoLine(fp,
                   dnsClientObj->X_BROADCOM_COM_ActiveDnsIfName, NULL,
                   subnetCidr4, NULL,
                   dnsClientObj->X_BROADCOM_COM_ActiveDnsServers, NULL);
      }
      else if (!IS_EMPTY_STRING(dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsIfName)) /* only IPv6 */
      {
         getDefaultLanSubnetInfo(subnetCidr4, subnetCidr6);

         cmsLog_debug("first pass: ActiveDnsv6IfName=%s ActiveDnsv6Servers=%s",
                      dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsIfName,
                      dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsServers);

         rutDns_writeDnsInfoLine(fp, NULL,
                   dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsIfName,
                   NULL, subnetCidr6, NULL,
                   dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsServers);
      }
   }
   else
   {
      /* second pass: dump out any WAN connection or DHCP on LAN(br0)
       * that was not dumped out in the first pass.
       */
      Dev2IpInterfaceObject *ipIntfObj=NULL;
      char activeDnsServers[CMS_MAX_ACTIVE_DNS_IP * CMS_IPADDR_LENGTH]={0};

      INIT_INSTANCE_ID_STACK(&iidStack);
      while (cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE,
                            &iidStack,
                            (void **) &ipIntfObj) == CMSRET_SUCCESS)
      {
#ifdef DHCP_CLIENT_DEFAULT
         char ipv4AddrBuf[CMS_IPADDR_LENGTH] = {0};
         UBOOL8 isDhcpOnLanAndGetIP = FALSE;
         UBOOL8 isWan = FALSE;
         UBOOL8 isStatic = FALSE;

         qdmIpIntf_getIpv4AddrInfoByNameLocked_dev2(ipIntfObj->name,
                                                  ipv4AddrBuf, NULL,
                                                  &isWan, &isStatic);
         isDhcpOnLanAndGetIP = (!isWan && !isStatic && strlen(ipv4AddrBuf) > 0);
#endif

         if ((((ipIntfObj->X_BROADCOM_COM_Upstream) &&
              qdmIpIntf_isWanInterfaceUpLocked_dev2(ipIntfObj->name, TRUE))
#ifdef DHCP_CLIENT_DEFAULT
              || isDhcpOnLanAndGetIP
#endif
              ) && cmsUtl_strcmp(ipIntfObj->name, dnsClientObj->X_BROADCOM_COM_ActiveDnsIfName))
         {
            /* we need to write this one out. */

            /* Get the DNSservers that are configured on this WAN or LAN(br0) intf */
            memset(activeDnsServers, 0, sizeof(activeDnsServers));
            if (rutNtwk_getIpv4DnsServersFromIfName(ipIntfObj->name, activeDnsServers) != CMSRET_SUCCESS)
            {
               /* no dynamically configured DNS on intf, use static ones */
               qdmDns_getStaticIpvxDnsServersLocked_dev2(CMS_AF_SELECT_IPV4, activeDnsServers);
            }

            /* XXX gather IPv6 info */

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
            /* FIXME: simply the same logic as what rutDns_dumpDnsInfo_igd() does in rut_dns.c */
            if (qdmIpIntf_isWanInterfaceUpLocked_dev2(ipIntfObj->name, FALSE))
            {
               if (isActiveDNSServer6(ipIntfObj->name, dnsClientObj->X_BROADCOM_COM_Ipv6_ActiveDnsServers))
               {
                  strcpy(ifName6, ipIntfObj->name);
                  if (rutNtwk_getIpv6DnsServersFromIfName(ifName6, dnsServers6) != CMSRET_SUCCESS)
                  {
                     qdmDns_getStaticIpvxDnsServersLocked_dev2(CMS_AF_SELECT_IPV6, dnsServers6);
                  }
               }
            }
#endif

            /*
             * This WAN interface could be part of a Layer 3 interface
             * group, if so, we need to get the subnetCidr for the LAN
             * interface which is associated with the WAN interface.
             * (TR98 code used getSubnetForWanConn).
             */
            if (!IS_EMPTY_STRING(ipIntfObj->X_BROADCOM_COM_GroupName)
#ifdef DHCP_CLIENT_DEFAULT
                && !isDhcpOnLanAndGetIP
#endif
                )
            {
               char ipv4AddrBuf[CMS_IPADDR_LENGTH]={0};
               char ipv4SubnetMaskBuf[CMS_IPADDR_LENGTH]={0};
               UBOOL8 isWan=FALSE;
               UBOOL8 isStatic=FALSE;

               qdmIpIntf_getIpv4AddrInfoByNameLocked_dev2(ipIntfObj->X_BROADCOM_COM_BridgeName,
                                                  ipv4AddrBuf, ipv4SubnetMaskBuf,
                                                  &isWan, &isStatic);

               /* convert ipv4addr and subnet to cidr4 */
               cmsNet_inet_ipv4AddrStrtoCidr4(ipv4AddrBuf, ipv4SubnetMaskBuf, subnetCidr4);
            }
            else
            {
               getDefaultLanSubnetInfo(subnetCidr4, subnetCidr6);
            }

            /* We've gathered the IPv4 and IPv6 info, write out one line */
            cmsLog_debug("second pass: ifName=%s activeDnsServers=%s cidr4=%s",
                         ipIntfObj->name, activeDnsServers, subnetCidr4);

            rutDns_writeDnsInfoLine(fp,
                                    ipIntfObj->name, ifName6,
                                    subnetCidr4, subnetCidr6,
                                    activeDnsServers, dnsServers6);
         }
         cmsObj_free((void **) &ipIntfObj);
      }
   }

   cmsObj_free((void **) &dnsClientObj);

   return ret;
}


void rutDns_writeStaticHosts_dev2(FILE *fp)
{
   Dev2HostObject *hostObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_HOST,
                                     &iidStack, OGF_NO_VALUE_UPDATE,
                                     (void **) &hostObj)) == CMSRET_SUCCESS)
   {
      /* doesn't seem like there is a good clean way to determine if this
       * is a static entry.  Use leaseTimeRemaining == -1 as a heuristic.
       * Real DHCP leases should have some finite time associated with them.
       */
      if (!IS_EMPTY_STRING(hostObj->IPAddress) &&
          !IS_EMPTY_STRING(hostObj->hostName) &&
          hostObj->leaseTimeRemaining == -1)
      {
         fprintf(fp, "%s\t%s\n", hostObj->IPAddress, hostObj->hostName);
      }
      cmsObj_free((void **) &hostObj);
   }

   return;
}
#endif /* DMP_DEVICE2_BASELINE_1 */
