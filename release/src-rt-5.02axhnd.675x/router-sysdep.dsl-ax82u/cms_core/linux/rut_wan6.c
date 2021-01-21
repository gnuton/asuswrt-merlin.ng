/***********************************************************************
 *
 *  Copyright (c) 2008  Broadcom Corporation
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
 
#include "rcl.h"
 
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
#include "rut_wan.h"
#include "rut_lan.h" 
#else
#include "rut_wan6.h"
#endif
/* TODO: IPv6 over ATM?? */
//#include "rut_wanlayer2.h"
#include "rut_ipsec.h"
#include "rut_upnp.h"
#include "rut_dhcp6.h"
#include "rut_ebtables.h"

#if defined(BRCM_PKTCBL_SUPPORT)
/* EMTA */
#define DHCP6C_OPTION_REPORT_LIST_EMTA   "16_17"  /* Pls connect multi codes with "_" */
#define DHCP6C_OPTION_REQUEST_LIST_EMTA  "17_24_37_39_56"  /* Pls connect multi codes with "_" */

/* EPTA */
#define DHCP6C_OPTION_REPORT_LIST_EPTA   "17"  /* Pls connect multi codes with "_" */
#define DHCP6C_OPTION_REQUEST_LIST_EPTA  "17_56"  /* Pls connect multi codes with "_" */
#endif

#ifdef SUPPORT_IPV6 
#define NeighTable "/tmp/NeighTable"
#endif

UBOOL8 rutWan_isIpv6ConnStatusChanged(const void *nObj __attribute__((unused)),
                                    const void *cObj __attribute__((unused)),
                                    UBOOL8 isIpObj __attribute__((unused)))
{
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   UBOOL8 changed;

   if ( isIpObj )
   {
      _WanIpConnObject *newObj = (_WanIpConnObject *)nObj;
      _WanIpConnObject *currObj = (_WanIpConnObject *)cObj;

     cmsLog_debug("ifname=%s, ipv6Enable=%d, new->status=%s, curr->status=%s", 
                   newObj->X_BROADCOM_COM_IfName, newObj->X_BROADCOM_COM_IPv6Enabled,
                   newObj->X_BROADCOM_COM_IPv6ConnStatus, currObj->X_BROADCOM_COM_IPv6ConnStatus);

      if ( (newObj->X_BROADCOM_COM_IPv6Enabled == TRUE) && (cmsUtl_strcmp(newObj->connectionType, MDMVS_IP_ROUTED) == 0) &&
            ((cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ConnStatus, currObj->X_BROADCOM_COM_IPv6ConnStatus) != 0) ||
              (newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled != currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled) ||
              (newObj->X_BROADCOM_COM_MFlag != currObj->X_BROADCOM_COM_MFlag) ||
              ( (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ConnStatus, currObj->X_BROADCOM_COM_IPv6ConnStatus) == 0) &&
                 (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED) == 0) &&
                 ( (cmsUtl_strcmp(newObj->X_BROADCOM_COM_ExternalIPv6Address, currObj->X_BROADCOM_COM_ExternalIPv6Address) != 0) ||
                    (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6SitePrefix, currObj->X_BROADCOM_COM_IPv6SitePrefix) != 0) ||
                    (cmsUtl_strcmp(newObj->X_BROADCOM_COM_DefaultIPv6Gateway, currObj->X_BROADCOM_COM_DefaultIPv6Gateway) != 0) ||
                    (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6DNSServers, currObj->X_BROADCOM_COM_IPv6DNSServers) != 0) ||
                    (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6DomainName, currObj->X_BROADCOM_COM_IPv6DomainName) != 0) ||
                    (newObj->X_BROADCOM_COM_IPv6SitePrefixPltime != currObj->X_BROADCOM_COM_IPv6SitePrefixPltime) ||
                    (newObj->X_BROADCOM_COM_IPv6SitePrefixVltime != currObj->X_BROADCOM_COM_IPv6SitePrefixVltime)))) )
      {
         changed = TRUE;
      }
      else
      {
         changed = FALSE;
      }
   }
   else
   {
      _WanPppConnObject *newObj = (_WanPppConnObject *)nObj;
      _WanPppConnObject *currObj = (_WanPppConnObject *)cObj;

      cmsLog_debug("ifname=%s, ipv6Enable=%d, new->status=%s, curr->status=%s", 
                    newObj->X_BROADCOM_COM_IfName, newObj->X_BROADCOM_COM_IPv6Enabled,
                    newObj->X_BROADCOM_COM_IPv6ConnStatus, currObj->X_BROADCOM_COM_IPv6ConnStatus);

      if ( (newObj->X_BROADCOM_COM_IPv6Enabled == TRUE) && (cmsUtl_strcmp(newObj->connectionType, MDMVS_IP_ROUTED) == 0) &&
            ((cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ConnStatus, currObj->X_BROADCOM_COM_IPv6ConnStatus) != 0) ||
              (newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled != currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled) ||
              (newObj->X_BROADCOM_COM_IPv6PppUp != currObj->X_BROADCOM_COM_IPv6PppUp) ||
              (newObj->X_BROADCOM_COM_MFlag != currObj->X_BROADCOM_COM_MFlag) ||
              ( (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ConnStatus, currObj->X_BROADCOM_COM_IPv6ConnStatus) == 0) &&
                 (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED) == 0) &&
                 ( (cmsUtl_strcmp(newObj->X_BROADCOM_COM_ExternalIPv6Address, currObj->X_BROADCOM_COM_ExternalIPv6Address) != 0) ||
                    (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6SitePrefix, currObj->X_BROADCOM_COM_IPv6SitePrefix) != 0) ||
                    (cmsUtl_strcmp(newObj->X_BROADCOM_COM_DefaultIPv6Gateway, currObj->X_BROADCOM_COM_DefaultIPv6Gateway) != 0) ||
                    (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6DNSServers, currObj->X_BROADCOM_COM_IPv6DNSServers) != 0) ||
                    (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6DomainName, currObj->X_BROADCOM_COM_IPv6DomainName) != 0) ||
                    (newObj->X_BROADCOM_COM_IPv6SitePrefixPltime != currObj->X_BROADCOM_COM_IPv6SitePrefixPltime) ||
                    (newObj->X_BROADCOM_COM_IPv6SitePrefixVltime != currObj->X_BROADCOM_COM_IPv6SitePrefixVltime)))) )
      {
         changed = TRUE;
      }
      else
      {
         changed = FALSE;
      }
   }

   cmsLog_debug("changed=%d",changed);

   return changed;
#else
   return FALSE;
#endif
}


UBOOL8 rutWan_isIpv4ConnStatusChanged( const void *nObj __attribute__((unused)),
                               const void *cObj __attribute__((unused)),
                               UBOOL8 isIpObj __attribute__((unused)))
{
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   UBOOL8 changed;

   if ( isIpObj )
   {
      _WanIpConnObject *newObj = (_WanIpConnObject *)nObj;
      _WanIpConnObject *currObj = (_WanIpConnObject *)cObj;

      cmsLog_debug("ifname=%s, ipv6Enable=%d", newObj->X_BROADCOM_COM_IfName, newObj->X_BROADCOM_COM_IPv6Enabled);

      if ( (newObj->X_BROADCOM_COM_IPv6Enabled == FALSE) ||
            (cmsUtl_strcmp(newObj->connectionType, MDMVS_IP_BRIDGED) == 0) ||
            (cmsUtl_strcmp(newObj->connectionStatus, currObj->connectionStatus) != 0) ||
            ( (cmsUtl_strcmp(newObj->connectionStatus, currObj->connectionStatus) == 0) &&
               (cmsUtl_strcmp(newObj->connectionStatus, MDMVS_CONNECTED) == 0) &&
               ( (cmsUtl_strcmp(newObj->externalIPAddress, currObj->externalIPAddress) != 0) ||
                  (cmsUtl_strcmp(newObj->subnetMask, currObj->subnetMask) != 0) ||
                  (cmsUtl_strcmp(newObj->defaultGateway, currObj->defaultGateway) != 0) ||
                  (newObj->X_BROADCOM_COM_FirewallEnabled != currObj->X_BROADCOM_COM_FirewallEnabled) ||
                  (cmsUtl_strcmp(newObj->DNSServers, currObj->DNSServers) != 0) )) )
      {
         changed = TRUE;
      }
      else
      {
         changed = FALSE;
      }
   }
   else
   {
      _WanPppConnObject *newObj = (_WanPppConnObject *)nObj;
      _WanPppConnObject *currObj = (_WanPppConnObject *)cObj;

      cmsLog_debug("ifname=%s, ipv6Enable=%d", newObj->X_BROADCOM_COM_IfName, newObj->X_BROADCOM_COM_IPv6Enabled);

      if ( (newObj->X_BROADCOM_COM_IPv6Enabled == FALSE) ||
            (cmsUtl_strcmp(newObj->connectionType, MDMVS_IP_BRIDGED) == 0) ||
            (cmsUtl_strcmp(newObj->connectionStatus, currObj->connectionStatus) != 0) ||
            ( (cmsUtl_strcmp(newObj->connectionStatus, currObj->connectionStatus) == 0) &&
               (cmsUtl_strcmp(newObj->connectionStatus, MDMVS_CONNECTED) == 0) &&
               ( (cmsUtl_strcmp(newObj->externalIPAddress, currObj->externalIPAddress) != 0) ||
                  (cmsUtl_strcmp(newObj->X_BROADCOM_COM_DefaultGateway, currObj->X_BROADCOM_COM_DefaultGateway) != 0) ||
                  (cmsUtl_strcmp(newObj->remoteIPAddress, currObj->remoteIPAddress) != 0) ||
                  (cmsUtl_strcmp(newObj->DNSServers, currObj->DNSServers) != 0) )) )
      {
         changed = TRUE;
      }
      else
      {
         changed = FALSE;
      }
   }

   cmsLog_debug("changed=%d",changed);

   return changed;
#else
   return TRUE;
#endif
}


CmsRet rutCfg_tearDownWanCon6(const void *pObj __attribute__((unused)),
                              UBOOL8 isIpObj __attribute__((unused)))
{
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
#ifdef DMP_X_BROADCOM_COM_MLD_1
   UBOOL8 mldEnabled;
#endif
#ifdef CMS_LOG3
   UBOOL8 pdEnabled;
#endif /* CMS_LOG3 */
   char intfName[BUFLEN_32];
   _IPv6LanHostCfgObject *IPv6LanCfgObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char *wanPrefix = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   if ( isIpObj )
   {
      _WanIpConnObject *currObj = (_WanIpConnObject *)pObj;
#ifdef DMP_X_BROADCOM_COM_MLD_1
      mldEnabled = currObj->X_BROADCOM_COM_MLDEnabled;
#endif
#ifdef CMS_LOG3
      pdEnabled = currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled;
#endif
      cmsUtl_strncpy(intfName, currObj->X_BROADCOM_COM_IfName, sizeof(intfName));
      wanPrefix = cmsMem_strdup(currObj->X_BROADCOM_COM_IPv6SitePrefix);
   }
   else
   {
      _WanPppConnObject *currObj = (_WanPppConnObject *)pObj;
#ifdef DMP_X_BROADCOM_COM_MLD_1
      mldEnabled = currObj->X_BROADCOM_COM_MLDEnabled;
#endif
#ifdef CMS_LOG3
      pdEnabled = currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled;
#endif
      cmsUtl_strncpy(intfName, currObj->X_BROADCOM_COM_IfName, sizeof(intfName));
      wanPrefix = cmsMem_strdup(currObj->X_BROADCOM_COM_IPv6SitePrefix);
   }

#ifdef DMP_X_BROADCOM_COM_MLD_1
   cmsLog_debug("isIpObj<%d> mldEnable<%d> pdEnable<%d> wanPrefix<%s>", isIpObj, mldEnabled, pdEnabled, wanPrefix);
#else
   cmsLog_debug("isIpObj<%d> pdEnable<%d> wanPrefix<%s>", isIpObj, pdEnabled, wanPrefix);
#endif

   /*
    * This function is only to stop all service of the WAN interface.
    * We do not update any information to the data model.
    */
#ifdef DMP_X_BROADCOM_COM_MLD_1
   if ( mldEnabled )
   {
      rutIpt_mldRules(FALSE, intfName);
   }
#endif

   cmsLog_debug("Tear down prefix delegation/dns server without modifying data model information!");

   if ((ret = cmsObj_getNextFlags(MDMOID_I_PV6_LAN_HOST_CFG, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&IPv6LanCfgObj)) != CMSRET_SUCCESS)
   {
      if (wanPrefix)
      {
         cmsMem_free((void *)wanPrefix);
      }
      cmsLog_error("Could not get MDMOID_I_PV6_LAN_HOST_CFG, ret=%d", ret);
      return ret;
   }

   if ((ret = cmsObj_set(IPv6LanCfgObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_set <MDMOID_I_PV6_LAN_HOST_CFG> returns error. ret=%d", ret);
   }

   cmsObj_free((void **) &IPv6LanCfgObj);

   cmsLog_debug("prefix delegation part!");
   rutWan_deletePDEntry(intfName, MDMVS_WANDELEGATED);
   if (wanPrefix)
   {
      rutWan_configPDRoute(wanPrefix, FALSE);
   }

   /* Delete ip6tables rules which verify source addr within the delegated prefixes */
   if (wanPrefix)
   {
      char addr[CMS_IPADDR_LENGTH];
      char prefix[CMS_IPADDR_LENGTH];

      if (cmsNet_subnetIp6SitePrefix(wanPrefix, 0, 64, addr) == CMSRET_SUCCESS)
      {
         sprintf(prefix, "%s/64", addr);
         rutIpt_configRoutingChain6(prefix, "br0", FALSE);
		 rutEbt_configICMPv6Reply(prefix, FALSE);
      }
      else
      {
         cmsLog_error("cmsNet_subnetIp6SitePrefix returns error");
      }
   }

   cmsMem_free((void *)wanPrefix);

   /* Deactivate the tunnels associated with this IP connection */
   cmsLog_debug("tunnel related part!");
   rutTunnel_control(intfName, MDMVS_IPV4INIPV6, FALSE);

   /* TODO:
    * UPNP
    * Multiple WAN Connection -> default gw and Dns
    * RIP
   */
   return ret;
#else
   return CMSRET_SUCCESS;
#endif
}


CmsRet rutCfg_stopWanCon6(const void *pObj __attribute__((unused)),
                          UBOOL8 isIpObj __attribute__((unused)))
{
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   CmsRet ret = CMSRET_SUCCESS;

   if ( isIpObj )
   {
      _WanIpConnObject *currObj = (_WanIpConnObject *)pObj;
      ret = rutCfg_stopWanConnection6(currObj->X_BROADCOM_COM_Dhcp6cPid, 
                                                         currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled, 
                                                         currObj->X_BROADCOM_COM_IfName);
   }
   else
   {
      _WanPppConnObject *currObj = (_WanPppConnObject *)pObj;
      ret = rutCfg_stopWanConnection6(currObj->X_BROADCOM_COM_Dhcp6cPid, 
                                                         currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled, 
                                                         currObj->X_BROADCOM_COM_IfName);
   }

   return ret;
#else
   return CMSRET_SUCCESS;
#endif
}


#ifdef DMP_X_BROADCOM_COM_IPV6_1
CmsRet rutCfg_setupWanConnection6(const void *pObj __attribute__((unused)),
                                           UBOOL8 isIpObj __attribute__((unused)))
{
   CmsRet ret             = CMSRET_SUCCESS;
   const char *ifName     = NULL;
   const char *addr       = NULL;
   const char *dnsServers = NULL;
   const char *domainName = NULL;
   const char *raPrefix   = NULL;
   UBOOL8 firewallEnabled = FALSE;
   UBOOL8 pdEnabled       = FALSE;
   UBOOL8 unnumbered      = FALSE;
   UBOOL8 dhcp6cIana      = FALSE;
   UBOOL8 mldEnabled      = FALSE;
   UBOOL8 isDnsWan        = FALSE;
   UBOOL8 MFlag           = FALSE;
   UBOOL8 LFlag           = FALSE; 
   
   if (isIpObj)
   {
      _WanIpConnObject *currObj = (_WanIpConnObject *)pObj;
	  ifName = currObj->X_BROADCOM_COM_IfName;
      addr = currObj->X_BROADCOM_COM_ExternalIPv6Address;
      firewallEnabled = currObj->X_BROADCOM_COM_FirewallEnabled;
      pdEnabled =  currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled;
      unnumbered = currObj->X_BROADCOM_COM_UnnumberedModel;
      dhcp6cIana = currObj->X_BROADCOM_COM_Dhcp6cForAddress;
	  domainName = currObj->X_BROADCOM_COM_IPv6DomainName;
	  dnsServers = currObj->X_BROADCOM_COM_IPv6DNSServers;
      raPrefix   = currObj->X_BROADCOM_COM_IPv6RaPrefix;
      MFlag      = currObj->X_BROADCOM_COM_MFlag;
      LFlag      = currObj->X_BROADCOM_COM_LFlag;
#ifdef DMP_X_BROADCOM_COM_MLD_1
      mldEnabled = currObj->X_BROADCOM_COM_MLDEnabled;
#endif
   }
   else
   {
      _WanPppConnObject *currObj = (_WanPppConnObject *)pObj;
      ifName = currObj->X_BROADCOM_COM_IfName;
      addr = currObj->X_BROADCOM_COM_ExternalIPv6Address;
      firewallEnabled = currObj->X_BROADCOM_COM_FirewallEnabled;
      pdEnabled =  currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled;
      unnumbered = currObj->X_BROADCOM_COM_UnnumberedModel;
      dhcp6cIana = currObj->X_BROADCOM_COM_Dhcp6cForAddress;
	  domainName = currObj->X_BROADCOM_COM_IPv6DomainName;
	  dnsServers = currObj->X_BROADCOM_COM_IPv6DNSServers;
      raPrefix   = currObj->X_BROADCOM_COM_IPv6RaPrefix;
      MFlag      = currObj->X_BROADCOM_COM_MFlag;
      LFlag      = currObj->X_BROADCOM_COM_LFlag;
#ifdef DMP_X_BROADCOM_COM_MLD_1
      mldEnabled = currObj->X_BROADCOM_COM_MLDEnabled;
#endif
   }

   cmsLog_debug("Enter: ifname=%s, ipv6Address=%s \n firewall=%d, pdflag=%d, unnumbered=%d, iana=%d, mld=%d", 
                 ifName, addr, firewallEnabled, pdEnabled, unnumbered, dhcp6cIana, mldEnabled);

   /* TODO: IPv6 over ATM?? */
//   if (!rutWl2_isIPoA(iidStack))
//   {
      /* configure the IPv6 address of the WAN interface */
      if ((ret = rutWan_setIPv6Address(addr, ifName, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutWan_setIPv6Address returns error. ret=%d", ret);
         return ret;
      }
   
      if (!IS_EMPTY_STRING(addr) || unnumbered || !dhcp6cIana)
      {
         /* set the system default gateway only after the external IPv6 address has been set */
         /* TODO: if want to support multiple WAN connections, REVISIT!! */
         if (rutWan_doDefaultSystemGatewayAndDNS(FALSE) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutWan_doDefaultSystemGatewayAndDNS returns error. ret=%d", ret);
         }

         /* activate static routes only after the external IPv6 address has been set */
         if ((ret = rutWan_activateIPv6StaticRoute(ifName, addr)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutWan_activateIPv6StaticRoute returns error. ret=%d", ret);
            return ret;
         }
 
         /*for CERouter.1.6.2b test case */
         /* if the new IANA is a subnet of the Ra-Prfix which had been advertised as a off-link by RA, del the route*/
         if (MFlag && !LFlag)
         {
            char   prefixAddr[CMS_IPADDR_LENGTH] = {0};
            char           tmpPrefixAddr[CMS_IPADDR_LENGTH] = {0};
            char           wanPrefixAddr[CMS_IPADDR_LENGTH] = {0};
            UINT32  raPrefixLen = 0;
 
            if ((ret = cmsUtl_parsePrefixAddress(raPrefix, prefixAddr, &raPrefixLen))== CMSRET_SUCCESS)
            {
               if (cmsNet_subnetIp6SitePrefix(addr, 0, raPrefixLen, tmpPrefixAddr) == CMSRET_SUCCESS)
               {
                  sprintf(wanPrefixAddr, "%s/%d", tmpPrefixAddr, raPrefixLen);
                  if (cmsUtl_strcmp(wanPrefixAddr, raPrefix) == 0)
                  {
                     rutWan_updateRaRoute(raPrefix, ifName);
                  }
               }
            }                                  
         }             
      }
#if defined(SUPPORT_DPI)
      /* with DPI enabled, always insmod conntrack/netlink modules on WAN up */
      rutIpt_insertIpModules6();
#endif

      if (firewallEnabled)
      {
         rutIpt_insertIpModules6();

         rutIpt_initFirewall(PF_INET6, ifName);
         rutIpt_initFirewallExceptions(ifName);
         rutIpt_setupFirewallForDHCPv6(TRUE, ifName);
         rutIpt_insertTCPMSSRules(PF_INET6, ifName);
      }
         
      /* Process prefix delegation */
      if (pdEnabled)
      {
         char *wanPrefix=NULL;
         char *wanPrefixOld=NULL;
         SINT32 pltime=0;
         SINT32 vltime=0;
         SINT32 vltimeOld=0;

         cmsLog_debug("Do prefix delegation!");

         if ( (ret = rutWan_getDelegatedPrefix(ifName, &wanPrefix, &wanPrefixOld, &pltime, &vltime, &vltimeOld))  != CMSRET_SUCCESS )
         {
            cmsLog_error("rutWan_getDelegatedPrefix returns error = %d", ret);
         }

         /* TODO: We only support br0 now */
         if ( (ret = rutWan_addPDEntry(ifName, wanPrefix, wanPrefixOld, pltime, vltime, vltimeOld, MDMVS_WANDELEGATED))  != CMSRET_SUCCESS )
         {
            cmsLog_error("rutWan_addPDEntry returns error = %d", ret);
         }

         /* If PD is length of 60, all the traffic within the /60 must stay in LAN */
         rutWan_configPDRoute(wanPrefix, TRUE);

         /* Create ip6tables rules to verify source addr within the delegated prefixes */
         if (wanPrefix)
         {
            char address[CMS_IPADDR_LENGTH];
            char prefix[CMS_IPADDR_LENGTH];

            if (cmsNet_subnetIp6SitePrefix(wanPrefix, 0, 64, address) == CMSRET_SUCCESS)
            {
               sprintf(prefix, "%s/64", address);
               rutIpt_configRoutingChain6(prefix, "br0", TRUE);
			   rutEbt_configICMPv6Reply(prefix, TRUE);
            }
            else
            {
               cmsLog_error("cmsNet_subnetIp6SitePrefix returns error");
            }
         }

         cmsMem_free((void *)wanPrefix);
      }
      else
      {
         cmsLog_debug("No prefix delegation!");
      }

      /* Trigger LAN object to setup DNS information, if any */
      {
         _IPv6LanHostCfgObject *IPv6LanCfgObj=NULL;
         InstanceIdStack iidStacktmp = EMPTY_INSTANCE_ID_STACK;

         cmsLog_debug("Do DNS part!");

         if ((ret = cmsObj_getNextFlags(MDMOID_I_PV6_LAN_HOST_CFG, &iidStacktmp, OGF_NO_VALUE_UPDATE, (void **)&IPv6LanCfgObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get MDMOID_I_PV6_LAN_HOST_CFG, ret=%d", ret);
            return ret;
         }
		 
		 if ((cmsUtl_strcmp(IPv6LanCfgObj->IPv6DNSWANConnection, ifName) == 0))
		 {
			 isDnsWan = TRUE;
		 }	 

         if ((ret = cmsObj_set(IPv6LanCfgObj, &iidStacktmp)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set <MDMOID_I_PV6_LAN_HOST_CFG> returns error. ret=%d", ret);
         }

         cmsObj_free((void **) &IPv6LanCfgObj);
      }

	  if (isDnsWan)
	  {
		 RadvdOtherInfoObject *radvdOtherObj = NULL;
   		 InstanceIdStack iidStackRadvdCfg = EMPTY_INSTANCE_ID_STACK;

		 if ((ret = cmsObj_getNextFlags(MDMOID_RADVD_OTHER_INFO, &iidStackRadvdCfg, OGF_NO_VALUE_UPDATE, (void **)&radvdOtherObj)) == CMSRET_SUCCESS)
		 {
			CMSMEM_REPLACE_STRING(radvdOtherObj->recursiveDns, dnsServers);
			CMSMEM_REPLACE_STRING(radvdOtherObj->dnssSearchList, domainName);

			if ((ret = cmsObj_set(radvdOtherObj, &iidStackRadvdCfg)) != CMSRET_SUCCESS)
			{
			   cmsLog_error("cmsObj_set <MDMOID_RADVD_OTHER_INFO> returns error. ret=%d", ret);
			}

			cmsObj_free((void **)&radvdOtherObj);
		 }
	  }
	  
      /* Activate tunnels associated with this connection if the WAN address is available */
      if (cmsUtl_isValidIpAddress(AF_INET6, addr))
      {
         rutTunnel_control(ifName, MDMVS_IPV4INIPV6, TRUE);
      }
//   }

   /* TODO: 
     * UPNP is not supported for IPv6 
     * QoS??
     * RIP??
     * IPSec??
     * Policy Routing??
     */

#ifdef DMP_X_BROADCOM_COM_MLD_1
   if (mldEnabled)
   {
      rutIpt_mldRules(TRUE, ifName);
   }
#endif /* DMP_X_BROADCOM_COM_MLD_1 */

   if(CMSRET_SUCCESS != rutMulti_reloadMcpd())
   {
      cmsLog_error("failed to reload mcpd");
   }

#ifdef SUPPORT_IPSEC
   rutIPSec_config();
   rutIPSec_restart();
#endif

#ifdef SUPPORT_UPNP
      if (rut_isUpnpEnabled() && rutUpnp_checkRunStatusWithDelay() == FALSE)
      {
         rut_restartUpnp(NULL);
      }
#endif
   cmsLog_debug("IPv6 services associated with %s is activated.\n", ifName);
   
   return ret;
}


CmsRet rutCfg_stopWanConnection6(UINT32 dhcp6cPid, UBOOL8 pdEnabled, 
                                 const char *intfName)
{
   /*
    * This function is to stop the WAN interface.
    * We have to update some information to the data model.
    */
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 specificEid = MAKE_SPECIFIC_EID(dhcp6cPid, EID_DHCP6C);

   /* Stop dhcp6c for both static and dynamic */
   cmsLog_debug("stop dhcp6c pid=%d on IfName=%s", dhcp6cPid, intfName);
   if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to send msg to stop dhcp6c");
   }
   else
   {
      cmsLog_debug("dhcp6c stopped");
   }

   /* Release the IPv6 address of br0 which is obtained from PD */         
   if ( pdEnabled )
   {
      _IPv6LanHostCfgObject *ipv6LanCfgObj=NULL;
      InstanceIdStack iidStack1=EMPTY_INSTANCE_ID_STACK;

      rutWan_deleteDelegatedAddrEntry(intfName, MDMVS_WANDELEGATED);

      /* update the DHCPv6 server */
      if ((ret = cmsObj_getNextFlags(MDMOID_I_PV6_LAN_HOST_CFG, &iidStack1, OGF_NO_VALUE_UPDATE, (void **)&ipv6LanCfgObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get MDMOID_I_PV6_LAN_HOST_CFG, ret=%d", ret);
         return ret;
      }

      if ((ret = cmsObj_set(ipv6LanCfgObj, &iidStack1)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_set <MDMOID_I_PV6_LAN_HOST_CFG> returns error. ret=%d", ret);
      }      

      cmsObj_free((void **) &ipv6LanCfgObj);

      /* update the prefix delegation info */
      rutWan_deletePDEntry(intfName, MDMVS_WANDELEGATED);

   }

   return ret;
}


CmsRet rutWan_addDelegatedAddrEntry(const char *srvName, const char *ipv6str, const char *lanIntf, const char * mode)
{
   UBOOL8 found = FALSE;
   char addr[CMS_IPADDR_LENGTH];
   CmsRet ret = CMSRET_SUCCESS;
   _LanIpIntfObject *lanIpIntf = NULL;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackDelegateAddr = EMPTY_INSTANCE_ID_STACK;

   cmsLog_debug("srvName<%s> addr<%s> lanIntf<%s> mode<%s>", srvName, ipv6str, lanIntf, mode);

   while ( (!found) && 
               ((ret = cmsObj_getNextFlags(MDMOID_LAN_IP_INTF, &iidStack2, OGF_NO_VALUE_UPDATE, (void **)&lanIpIntf)) == CMSRET_SUCCESS) )
   {
      if (cmsUtl_strcmp(lanIpIntf->X_BROADCOM_COM_IfName, lanIntf) == 0)
      {
         found = TRUE;
      }

      cmsObj_free((void **)&lanIpIntf);
   }
  
   if (!found)
   {
      cmsLog_error("LAN interface %s not found", lanIntf);
   }
   else
   {
      DelegatedAddressObject *delegatedAddr = NULL;
  
      found = FALSE;

      /* Currently, we only allow ONE delegated address from a single WAN interface or a single tunnel */
      while ( (!found) && 
                  (cmsObj_getNextInSubTreeFlags(MDMOID_DELEGATED_ADDRESS, &iidStack2, &iidStackDelegateAddr, OGF_NO_VALUE_UPDATE, (void **)&delegatedAddr) == CMSRET_SUCCESS) )
      {
         if ( (cmsUtl_strcmp(delegatedAddr->mode, mode) == 0) && 
               (cmsUtl_strcmp(delegatedAddr->delegatedConnection, srvName) == 0) )
         {
            found = TRUE;
            cmsUtl_strncpy(addr, delegatedAddr->IPv6InterfaceAddress, sizeof(addr));
         }

         cmsObj_free((void **) &delegatedAddr);
      }
   
      if ( !found )
      {
         cmsLog_debug("delegate LAN IPv6 address: %s", ipv6str);
         iidStackDelegateAddr = iidStack2;

         if ((ret = cmsObj_addInstance(MDMOID_DELEGATED_ADDRESS, &iidStackDelegateAddr)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not add MDMOID_DELEGATED_ADDRESS, ret=%d", ret);
            return ret;
         }
     
         if ((ret = cmsObj_get(MDMOID_DELEGATED_ADDRESS, &iidStackDelegateAddr, 0, (void **) &delegatedAddr)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get _DelegatedAddressObject, ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_DELEGATED_ADDRESS, &iidStackDelegateAddr);
            return ret;
         }
     
         CMSMEM_REPLACE_STRING_FLAGS(delegatedAddr->delegatedConnection, srvName, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(delegatedAddr->mode, mode, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(delegatedAddr->IPv6InterfaceAddress, ipv6str, mdmLibCtx.allocFlags);
     
         if ((ret = cmsObj_set(delegatedAddr, &iidStackDelegateAddr)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set <MDMOID_DELEGATED_ADDRESS> returns error. ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_DELEGATED_ADDRESS, &iidStackDelegateAddr);
            cmsObj_free((void **) &delegatedAddr);
            return ret;
         }
     
         cmsObj_free((void **) &delegatedAddr);
      }
      else
      {
         cmsLog_debug("srvName<%s> try to delegate more than one addr to br0: orig<%s> new<%s>", addr, ipv6str);
      }
   }

   return ret;
}


CmsRet rutWan_deleteDelegatedAddrEntry(const char *srvName, const char * mode)
{
   UBOOL8 found;
   _LanIpIntfObject *lanIpIntf = NULL;
   _DelegatedAddressObject *addrObj = NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack2=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   found = FALSE;

   cmsLog_debug("srvName<%s> mode<%s>", srvName, mode);

    /* TODO: br0 only!! */
   while ((!found) && 
          (cmsObj_getNextFlags(MDMOID_LAN_IP_INTF, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&lanIpIntf) == CMSRET_SUCCESS))
   {
      if (cmsUtl_strcmp(lanIpIntf->X_BROADCOM_COM_IfName, "br0") == 0)
      {
         found = TRUE;
      }
      cmsObj_free((void **)&lanIpIntf);
   }

   if (!found)
   {
      cmsLog_error("LAN interface br0 not found");
   }

   found = FALSE;
   
   /* find the corresponding DelegatedAddressObject and delete it  */
   while ( (!found) && 
               (cmsObj_getNextInSubTreeFlags(MDMOID_DELEGATED_ADDRESS, &iidStack, &iidStack2, OGF_NO_VALUE_UPDATE, (void **)&addrObj) == CMSRET_SUCCESS) )
   {
      if ( (cmsUtl_strcmp(addrObj->mode, mode) == 0) && 
            (cmsUtl_strcmp(addrObj->delegatedConnection, srvName) == 0) )
      {
         found = TRUE;
   
         if ((ret = cmsObj_deleteInstance(MDMOID_DELEGATED_ADDRESS, &iidStack2)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_deleteInstance <MDMOID_DELEGATED_ADDRESS> returns error. ret=%d", ret);
            cmsObj_free((void **) &addrObj);
            return ret;
         }
      }
      cmsObj_free((void **) &addrObj);
   }

   if ( !found )
   {
      cmsLog_debug("cannot find corresponding DelegatedAddressObject");
   }

   return ret;
}


CmsRet rutWan_activateIPv6StaticRoute(const char *ifcName, const char *ifcAddr)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   IPv6L3ForwardingEntryObject *routeCfg = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   
   cmsLog_debug("Activate ipv6 static route with ifcName=%s ifcAddr=%s", ifcName, ifcAddr);

   while ((ret = cmsObj_getNextFlags
         (MDMOID_I_PV6_L3_FORWARDING_ENTRY, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &routeCfg)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(ifcName, routeCfg->interface) == 0)
      {
         /* set and activate L3ForwardingEntryObject */
         if ((ret = cmsObj_set(routeCfg, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set returns error. ret=%d", ret);
            cmsObj_free((void **) &routeCfg);
            return ret;
         }
      }
      else if (ifcAddr)
      {
         /* the route may not have the interface specified.  Let's see if its gateway address
          * is on the same subnet prefix as this interface.
          */
         if (cmsNet_isHostInSameSubnet(routeCfg->gatewayIPv6Address, ifcAddr))
         {
            /* set and activate L3ForwardingEntryObject */
            if ((ret = cmsObj_set(routeCfg, &iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsObj_set returns error. ret=%d", ret);
               cmsObj_free((void **) &routeCfg);
               return ret;
            }
         }
      }
      cmsObj_free((void **) &routeCfg);
   }

   return CMSRET_SUCCESS;

}  /* End of rutWan_activateIPv6StaticRoute() */


CmsRet rutWan_getDns6Server(const char *wanDnsConn, char **dnsServer, char **domainName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanIpConnObject *ipConnObj=NULL;
   WanPppConnObject *pppConnObj=NULL;
   UBOOL8 found=FALSE;

   CmsRet ret;

   *dnsServer = NULL;
   *domainName = NULL;

   if (IS_EMPTY_STRING(wanDnsConn))
   {
      /* do nothing */
      return CMSRET_SUCCESS;
   }

   cmsLog_debug("wanDnsConn=%s", wanDnsConn);

   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConnObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(ipConnObj->X_BROADCOM_COM_IfName, wanDnsConn) == 0)
      {
         found = TRUE;
         *dnsServer = cmsMem_strdup(ipConnObj->X_BROADCOM_COM_IPv6DNSServers);
         *domainName = cmsMem_strdup(ipConnObj->X_BROADCOM_COM_IPv6DomainName);
      }

      cmsObj_free((void **) &ipConnObj);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppConnObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(pppConnObj->X_BROADCOM_COM_IfName, wanDnsConn) == 0)
      {
         found = TRUE;
         *dnsServer = cmsMem_strdup(pppConnObj->X_BROADCOM_COM_IPv6DNSServers);
         *domainName = cmsMem_strdup(pppConnObj->X_BROADCOM_COM_IPv6DomainName);
      }

      cmsObj_free((void **) &pppConnObj);
   }

   if ( *dnsServer )
   {
      cmsLog_debug("dnsServer=%s", *dnsServer);
   }
   else
   {
      cmsLog_notice("Cannot find dnsServer");
   }

   if ( *domainName )
   {
      cmsLog_debug("domainName=%s", *domainName);
   }
   else
   {
      cmsLog_notice("Cannot find domainName");
   }

   return ret;

}  /* End of rutWan_getDns6Server() */


CmsRet rutWan_getDelegatedPrefix(const char *wanPdConn, char **sitePrefix, char **sitePrefixOld, SINT32 *pltime, SINT32 *vltime, SINT32 *vltimeOld)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanIpConnObject *ipConnObj=NULL;
   _WanPppConnObject *pppConnObj=NULL;
   UBOOL8 found=FALSE;
   CmsRet ret;

   cmsLog_debug("wanPdConn=%s",wanPdConn);

   if (IS_EMPTY_STRING(wanPdConn) || (sitePrefix == NULL) || (sitePrefixOld == NULL) || (pltime == NULL) || (vltime == NULL) || (vltimeOld == NULL))
   {
      /* do nothing */
      return CMSRET_SUCCESS;
   }

   *sitePrefix = NULL;
   *sitePrefixOld = NULL;
   *pltime = 0;
   *vltime = 0;
   *vltimeOld = 0;

   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConnObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(ipConnObj->X_BROADCOM_COM_IfName, wanPdConn) == 0)
      {
         found = TRUE;

         *sitePrefix = cmsMem_strdup(ipConnObj->X_BROADCOM_COM_IPv6SitePrefix);
         *sitePrefixOld = cmsMem_strdup(ipConnObj->X_BROADCOM_COM_IPv6SitePrefixOld);
         *pltime = ipConnObj->X_BROADCOM_COM_IPv6SitePrefixPltime;
         *vltime = ipConnObj->X_BROADCOM_COM_IPv6SitePrefixVltime;
         *vltimeOld = ipConnObj->X_BROADCOM_COM_IPv6SitePrefixVltimeOld;
      }

      cmsObj_free((void **) &ipConnObj);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppConnObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(pppConnObj->X_BROADCOM_COM_IfName, wanPdConn) == 0)
      {
         found = TRUE;

         *sitePrefix = cmsMem_strdup(pppConnObj->X_BROADCOM_COM_IPv6SitePrefix);
         *sitePrefixOld = cmsMem_strdup(pppConnObj->X_BROADCOM_COM_IPv6SitePrefixOld);
         *pltime = pppConnObj->X_BROADCOM_COM_IPv6SitePrefixPltime;
         *vltime = pppConnObj->X_BROADCOM_COM_IPv6SitePrefixVltime;
         *vltimeOld = pppConnObj->X_BROADCOM_COM_IPv6SitePrefixVltimeOld;
      }

      cmsObj_free((void **) &pppConnObj);
   }

   if ( *sitePrefix )
   {
      cmsLog_debug("Exit: prefix=%s, pltime=%d, vltime=%d",*sitePrefix, *pltime, *vltime);
   }
   else
   {
      cmsLog_notice("Cannot find sitePrefix");
   }

   return ret;

}  /* End of rutWan_getDelegatedPrefix() */


CmsRet rutWan_addPDEntry(const char *srvName, const char *wanPrefix, const char *wanPrefixOld, SINT32 pltime, SINT32 vltime, SINT32 vltimeOld, const char * mode)
{
   _PrefixInfoObject *prefixObj = NULL;
   _RadvdConfigMgtObject *radvdObj = NULL;
   char existPrefix[BUFLEN_48];
   InstanceIdStack iidStackRadvdCfg = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackPrefixAdd = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 found = FALSE;

   cmsLog_debug("srvName<%s> wanPrefix<%s> pltime<%d> vltime<%d> mode<%s>", srvName, wanPrefix, pltime, vltime, mode);

   if (wanPrefix == NULL)
   {
      cmsLog_error("wanPrefix is NULL");
      return ret;
   }

   if ((ret = cmsObj_getNextFlags(MDMOID_RADVD_CONFIG_MGT, &iidStackRadvdCfg, OGF_NO_VALUE_UPDATE, (void **)&radvdObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get MDMOID_RADVD_CONFIG_MGT, ret=%d", ret);
      return ret;
   }

   /* Currently, we only allow ONE delegated prefix from a single WAN interface or a single tunnel */
   while ( (!found) && 
               (cmsObj_getNextInSubTreeFlags(MDMOID_PREFIX_INFO, &iidStackRadvdCfg, &iidStackPrefixAdd, OGF_NO_VALUE_UPDATE, (void **)&prefixObj) == CMSRET_SUCCESS) )
   {
      if ( (cmsUtl_strcmp(prefixObj->mode, mode) == 0) && 
            (cmsUtl_strcmp(prefixObj->delegatedConnection, srvName) == 0) )
      {
         found = TRUE;   
         cmsUtl_strncpy(existPrefix, prefixObj->prefix, sizeof(existPrefix));
      }
      cmsObj_free((void **) &prefixObj);
   }

   if ( !found )
   {
      cmsLog_debug("create prefix object");
      iidStackPrefixAdd = iidStackRadvdCfg;

      if ((ret = cmsObj_addInstance(MDMOID_PREFIX_INFO, &iidStackPrefixAdd)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not add MDMOID_PREFIX_INFO, ret=%d", ret);
         cmsObj_free((void **) &radvdObj);
         return ret;
      }
   
      if ((ret = cmsObj_get(MDMOID_PREFIX_INFO, &iidStackPrefixAdd, 0, (void **) &prefixObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get PrefixInfoObject, ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_PREFIX_INFO, &iidStackPrefixAdd);
         cmsObj_free((void **) &radvdObj);
         return ret;
      }  
   
      CMSMEM_REPLACE_STRING_FLAGS(prefixObj->mode, mode, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(prefixObj->delegatedConnection, srvName, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(prefixObj->prefix, wanPrefix, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(prefixObj->prefixOld, wanPrefixOld, mdmLibCtx.allocFlags);
      prefixObj->preferredLifeTime = pltime;
      prefixObj->validLifeTime = vltime;
      prefixObj->validLifeTimeOld = vltimeOld;
   
      if ((ret = cmsObj_set(prefixObj, &iidStackPrefixAdd)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_set <MDMOID_PREFIX_INFO> returns error. ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_PREFIX_INFO, &iidStackPrefixAdd);
         cmsObj_free((void **) &prefixObj);
         cmsObj_free((void **) &radvdObj);
         return ret;
      }
   
      cmsObj_free((void **) &prefixObj);   
   }
   else
   {
      cmsLog_debug("srvName<%s> tries to add more than one pd: orig<%s> new<%s>", srvName, existPrefix, wanPrefix);
   }

   /* At this point, prefixInfo object should already fetch the pd information from current WAN interface */
   if ((ret = cmsObj_set(radvdObj, &iidStackRadvdCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_set <MDMOID_RADVD_CONFIG_MGT> returns error. ret=%d", ret);
   }

   cmsObj_free((void **) &radvdObj);

   return ret;
}


CmsRet rutWan_deletePDEntry(const char *srvName, const char * mode)
{
   UBOOL8 found = FALSE;
   _PrefixInfoObject *prefixObj = NULL;
   _RadvdConfigMgtObject *radvdObj = NULL;
   InstanceIdStack iidStackRadvdCfg = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackPrefix = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("srvName<%s> mode<%s>", srvName, mode);

   if ((ret = cmsObj_getNextFlags(MDMOID_RADVD_CONFIG_MGT, &iidStackRadvdCfg, OGF_NO_VALUE_UPDATE, (void **)&radvdObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get MDMOID_RADVD_CONFIG_MGT, ret=%d", ret);
      return ret;
   }
   
   /* find the corresponding PrefixInfoObj and trigger it to clear its information. Then restart radvd */
   while ( (!found) && 
               (cmsObj_getNextInSubTreeFlags(MDMOID_PREFIX_INFO, &iidStackRadvdCfg, &iidStackPrefix, OGF_NO_VALUE_UPDATE, (void **)&prefixObj) == CMSRET_SUCCESS) )
   {
      if ( (cmsUtl_strcmp(prefixObj->mode, mode) == 0) && 
            (cmsUtl_strcmp(prefixObj->delegatedConnection, srvName) == 0) )
      {
         found = TRUE;
   
         if ((ret = cmsObj_deleteInstance(MDMOID_PREFIX_INFO, &iidStackPrefix)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_deleteInstance <MDMOID_PREFIX_INFO> returns error. ret=%d", ret);
            cmsObj_free((void **) &prefixObj);
            cmsObj_free((void **) &radvdObj);
            return ret;
         }
      }
      cmsObj_free((void **) &prefixObj);
   }

   if ( !found )
   {
      cmsLog_debug("cannot find corresponding PrefixInfoObj");
   }
   else
   {
      if ((ret = cmsObj_set(radvdObj, &iidStackRadvdCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_set <MDMOID_RADVD_CONFIG_MGT> returns error. ret=%d", ret);
      }
   
      cmsObj_free((void **) &radvdObj);
   }

   return ret;
}


CmsRet rutWan_configIPv6DfltGateway(UBOOL8 select, const char *ifname)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanIpConnObject *ipConnObj=NULL;
   _WanPppConnObject *pppConnObj=NULL;
   UBOOL8 found=FALSE;
   CmsRet ret;

   cmsLog_debug("select=%d, ifname=%s", select, ifname);

   if (IS_EMPTY_STRING(ifname))
   {
      cmsLog_error("ifname is empty!");
      return CMSRET_INTERNAL_ERROR;
   }

   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConnObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(ipConnObj->X_BROADCOM_COM_IfName, ifname) == 0)
      {
         found = TRUE;
         if (!IS_EMPTY_STRING(ipConnObj->X_BROADCOM_COM_DefaultIPv6Gateway) &&
             !IS_EMPTY_STRING(ipConnObj->X_BROADCOM_COM_ExternalIPv6Address) )
         {
            char cmdLine[BUFLEN_128];
   
            /*
             * FIXME: Configure the default route only for the static configuration
             * If the default route info is from RA, the kernel will configure the route.
             */
            if (cmsUtl_strcmp(ipConnObj->X_BROADCOM_COM_IPv6AddressingType, MDMVS_STATIC) == 0)
            {
               snprintf(cmdLine, sizeof(cmdLine), "ip -6 ro %s default via %s dev %s 2>/dev/null",
                        select? "add" : "del",
                        ipConnObj->X_BROADCOM_COM_DefaultIPv6Gateway,
                        ipConnObj->X_BROADCOM_COM_IfName);
               rut_doSystemAction("rut", cmdLine);
            }
         }   
      }

      cmsObj_free((void **) &ipConnObj);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppConnObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(pppConnObj->X_BROADCOM_COM_IfName, ifname) == 0)
      {
         char cmdLine[BUFLEN_128];
         
         found = TRUE;
         if (IS_EMPTY_STRING(pppConnObj->X_BROADCOM_COM_DefaultIPv6Gateway))
         {
            snprintf(cmdLine, sizeof(cmdLine), "ip -6 ro %s default dev %s 2>/dev/null",
                     select? "add" : "del",
                     pppConnObj->X_BROADCOM_COM_IfName);
            rut_doSystemAction("rut", cmdLine);
         }
         /*
          * FIXME: Configure the default route only for the static configuration
          * If the default route info is from RA, the kernel will configure the route.
          */
         else if (!IS_EMPTY_STRING(pppConnObj->X_BROADCOM_COM_ExternalIPv6Address) &&
                  !cmsUtl_strcmp(pppConnObj->X_BROADCOM_COM_IPv6AddressingType, MDMVS_STATIC))
         {
            snprintf(cmdLine, sizeof(cmdLine), "ip -6 ro del default dev %s 2>/dev/null",
                     pppConnObj->X_BROADCOM_COM_IfName);
            rut_doSystemAction("rut", cmdLine);

            snprintf(cmdLine, sizeof(cmdLine), "ip -6 ro %s default via %s dev %s 2>/dev/null",
                     select? "add" : "del",
                     pppConnObj->X_BROADCOM_COM_DefaultIPv6Gateway,
                     pppConnObj->X_BROADCOM_COM_IfName);
            rut_doSystemAction("rut", cmdLine);
         }
      }

      cmsObj_free((void **) &pppConnObj);
   }

   return ret;

}  /* End of rutWan_configIPv6DfltGateway() */


#endif  /* DMP_X_BROADCOM_COM_IPV6_1 */



/* This function accesses TR98 objects but is used by common IPv6 code (createRadvdConf)
 * Probably need _igd and _dev2 versions of this function.
 */
UBOOL8 rutWan_isDfltGtwyExist(void)
{
#ifdef DMP_X_BROADCOM_COM_IPV6_1
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
   WanPppConnObject *wan_ppp_con = NULL;
   WanIpConnObject *wan_ip_con = NULL;
   UBOOL8 found = FALSE;

   while (!found &&
          cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack1, 
                  OGF_NO_VALUE_UPDATE, (void **) &wan_ip_con) == CMSRET_SUCCESS)
   {
      if ( wan_ip_con->X_BROADCOM_COM_DefaultIPv6Gateway )
      {
         found = TRUE;
      }
      cmsObj_free((void **) &wan_ip_con);
   }

   while (!found &&
          cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack2, 
                 OGF_NO_VALUE_UPDATE, (void **) &wan_ppp_con) == CMSRET_SUCCESS)
   {
      if ( wan_ppp_con->X_BROADCOM_COM_DefaultIPv6Gateway )
      {
         found = TRUE;
      }
      cmsObj_free((void **) &wan_ppp_con);
   }

   return found;
#else
   return FALSE;
#endif  /* DMP_X_BROADCOM_COM_IPV6_1 */
}


#ifdef SUPPORT_IPV6
UINT32 rutWan_restartDhcp6c(const char *ifName_info, UBOOL8 dynamicIpEnabled,
                            UBOOL8 pdEnabled, UBOOL8 aftrName, UBOOL8 mapt, UBOOL8 mape)
{
   char cmdLine[BUFLEN_128];
   char ifName[BUFLEN_32];
   char *ptr;
   UINT32 pid;
   char dhcp6cConfFile[BUFLEN_64];
   FILE *fp;

const char *dhcp6cConf = "\
interface %s\n\
{\n\
   request domain-name-servers;\n\
   request domain-name;\n\
   request ntp-servers;\n\
   send rapid-commit;\n\
   %s\n\
   %s\n\
   %s\n\
   %s\n\
   %s\n\
};\n\
%s\
%s\
";
const char *iapd = "send ia-pd 0;";
const char *iapdAssoc = "\
id-assoc pd 0\n\
{\n\
   prefix-interface br0\n\
   {\n\
      sla-id 0;\n\
   };\n\
};\n\
";
const char *iana = "send ia-na 1;";
const char *ianaAssoc = "\
id-assoc na 1\n\
{\n\
};\n\
";
const char *aftrReq = "request aftr;";
const char *maptReq = "request mapt;";
const char *mapeReq = "request mape;";

   cmsUtl_strncpy(ifName, ifName_info, sizeof(ifName));
   if ((ptr = cmsUtl_strstr(ifName_info, "__")) != NULL)
   {
      ifName[ptr-ifName_info] = '\0';
   }

   sprintf(dhcp6cConfFile, "/var/dhcp6c_%s.conf", ifName);
   if ((fp = fopen(dhcp6cConfFile, "w")) == NULL)
   {
      /* error */
      cmsLog_error("failed to create %s\n", dhcp6cConfFile);
      return CMS_INVALID_PID;
   }
   else
   {
      /* create dhcp6c.conf */
      fprintf(fp, dhcp6cConf, ifName,
              aftrName?aftrReq:"", 
              mapt?maptReq:"", 
              mape?mapeReq:"", 
              dynamicIpEnabled?iana:"", 
              pdEnabled?iapd:"", 
              dynamicIpEnabled?ianaAssoc:"", 
              pdEnabled?iapdAssoc:"");

      fclose(fp);
   }

//   rutIpt_setupFirewallForDHCPv6(TRUE, ifName);

   snprintf(cmdLine, sizeof(cmdLine), "-c %s", dhcp6cConfFile);

   cmsDhcp_mkCfgDir(DHCP_V6, ifName);
#if defined(BRCM_PKTCBL_SUPPORT)
   char voiceIf[BUFLEN_64] = { 0 };
   rutWan_getVoiceBoundIfName(voiceIf, sizeof(voiceIf));
   if (strcmp(ifName, voiceIf) == 0)
   {
      rutDhcp6_createOption17(PKTCBL_WAN_EMTA, ifName);
      snprintf(&cmdLine[strlen(cmdLine)], sizeof(cmdLine), " -P");
      snprintf(&cmdLine[strlen(cmdLine)], sizeof(cmdLine), " -R %s", DHCP6C_OPTION_REPORT_LIST_EMTA);
      snprintf(&cmdLine[strlen(cmdLine)], sizeof(cmdLine), " -Q %s", DHCP6C_OPTION_REQUEST_LIST_EMTA);
   }
   else if (strcmp(ifName, EPON_EPTA_WAN_IF_NAME) == 0)
   {
      rutDhcp6_createOption17(PKTCBL_WAN_EPTA, ifName);
      snprintf(&cmdLine[strlen(cmdLine)], sizeof(cmdLine), " -P");
      snprintf(&cmdLine[strlen(cmdLine)], sizeof(cmdLine), " -R %s", DHCP6C_OPTION_REPORT_LIST_EPTA);
      snprintf(&cmdLine[strlen(cmdLine)], sizeof(cmdLine), " -Q %s", DHCP6C_OPTION_REQUEST_LIST_EPTA);
   }
#endif
    
   snprintf(&cmdLine[strlen(cmdLine)], sizeof(cmdLine), " %s", ifName_info);

   if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_DHCP6C, cmdLine, strlen(cmdLine)+1)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to start or restart dhcp6c on %s", ifName);
      rutIpt_setupFirewallForDHCPv6(FALSE, ifName);
   }
   else
   {
      cmsLog_debug("restarting dhcp6c, pid=%d on %s", pid, ifName);
   }
   
   return pid;

}  /* End of rutWan_restartDhcp6c() */


CmsRet rutWan_stopDhcp6c(const char *ifName, UINT32 pid)
{
   char dhcp6cConfFile[BUFLEN_64];
   UINT32 specificEid;

   sprintf(dhcp6cConfFile, "/var/dhcp6c_%s.conf", ifName);

   specificEid = MAKE_SPECIFIC_EID(pid, EID_DHCP6C);
   rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0);
   remove(dhcp6cConfFile);
//   rutIpt_setupFirewallForDHCPv6(FALSE, ifName);
   return CMSRET_SUCCESS;

}  /* End of rutWan_stopDhcp6c() */


CmsRet rutWan_setIPv6Address(const char *newAddr, const char *newIfName,
                             const char *oldAddr, const char *oldIfName)
{
   char cmdLine[BUFLEN_128];
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Addr: new=%s old=%s IfName: new=%s old=%s",
                             newAddr, oldAddr, newIfName, oldIfName);

   if (!IS_EMPTY_STRING(oldAddr) && !IS_EMPTY_STRING(oldIfName))
   {
      if (!cmsNet_areIp6AddrEqual(newAddr, oldAddr) || cmsUtl_strcmp(newIfName, oldIfName))
      {
         /* delete the current address for this connection */
         snprintf(cmdLine, sizeof(cmdLine), "ip -6 addr del %s dev %s 2>/dev/null", oldAddr, oldIfName);
         rut_doSystemAction("rut", cmdLine);

         /* have to manually delete the interface route */
         snprintf(cmdLine, sizeof(cmdLine), "ip -6 ro del %s dev %s 2>/dev/null", oldAddr, oldIfName);
         rut_doSystemAction("rut", cmdLine);
      }
   }
   if ( !IS_EMPTY_STRING(newAddr) && !IS_EMPTY_STRING(newIfName))
   {
      /* add the new address for this connection */
      snprintf(cmdLine, sizeof(cmdLine), "ip -6 addr add %s dev %s 2>/dev/null", newAddr, newIfName);
      rut_doSystemAction("rut", cmdLine);
   }
   
   return ret; 

}  /* End of rutWan_setIPv6Address() */

void rutWan_flushHwByGtwy6(const char *gateway, const char *ifName)
{  
   FILE *fs;
   char buf[BUFLEN_128] = {0};
   UBOOL8 found = FALSE;
   char *chPtr = NULL;
   char macStr[BUFLEN_18] = {0};
   char cmdLine[BUFLEN_128] = {0};
   
   if ((ifName) && (gateway))
   {
      snprintf(cmdLine, sizeof(cmdLine), "ip -6 neigh show dev %s | grep %s >%s", ifName, gateway, NeighTable);
      rut_doSystemAction("rut", cmdLine);

      if ((fs = fopen(NeighTable, "r")) != NULL)
      {
         while(fgets(buf, sizeof(buf), fs) != NULL)
         {
			if ((chPtr = cmsUtl_strstr(buf, "lladdr")) != NULL)
			{
				chPtr = chPtr + strlen("lladdr ");
				cmsUtl_strncpy(macStr, chPtr, sizeof(macStr));
				found = TRUE;
				break;
			}
         }
		 
		 fclose(fs);
      }
	  
	  if (found && !IS_EMPTY_STRING(macStr))
	  {
		  snprintf(cmdLine, sizeof(cmdLine), "fc flush --if %s --dstmac %s >/dev/null", ifName, macStr);
		  rut_doSystemAction("rut", cmdLine);
	  }
   }
}


void rutWan_removeZeroLifeGtwy6(const char *gateway, const char *ifName)
{
   cmsLog_debug("gateway<%s> ifName<%s>", gateway, ifName);

   if (ifName)
   {
      char cmdLine[BUFLEN_128];

      snprintf(cmdLine, sizeof(cmdLine), "ip -6 ro del default dev %s 2>/dev/null", ifName);
      rut_doSystemAction("rut", cmdLine);

	  rutWan_flushHwByGtwy6(gateway, ifName);
   }
}
 
void rutWan_updateRaRoute(const char *wanPrefix, const char *ifName)
{
   cmsLog_debug("wanPrefix<%s> if<%s>", wanPrefix, ifName);
   
   if ((wanPrefix) && (ifName))
   {
      char cmdLine[BUFLEN_128];
      snprintf(cmdLine, sizeof(cmdLine), "ip -6 ro del %s dev %s 2>/dev/null", wanPrefix, ifName);
      rut_doSystemAction("rut", cmdLine);
   }
}
 
CmsRet rutWan_configPDRoute(const char *wanPrefix, UBOOL8 add)
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmdLine[BUFLEN_128];

   cmsLog_debug("wanPrefix<%s> add<%d>", wanPrefix, add);
   
   if ( wanPrefix )
   {
      snprintf(cmdLine, sizeof(cmdLine), "ip -6 ro %s %s dev lo",
               add? "add" : "del", wanPrefix);

      rut_doSystemAction("rut", cmdLine);
   }

   return ret;
}


void rutWan_configIpv6DefaultGateway(const char *op,
                                     const char *gwIpAddr,
                                     const char *gwIntfName)
{
   char cmdLine[BUFLEN_128]={0};

   if (cmsUtl_strcmp(op, "add") && cmsUtl_strcmp(op, "del"))
   {
      cmsLog_error("op must be either add or del");
   }

   if (IS_EMPTY_STRING(gwIntfName))
   {
      cmsLog_error("gwIntfName is empty");
      return;
   }

   if (!cmsUtl_strcmp(op, "del"))
   {
      snprintf(cmdLine, sizeof(cmdLine),
               "ip -6 ro del default dev %s 2>/dev/null",
               gwIntfName);
   }
   else
   {
      /* this must be an add.  gwIpAddr is mandatory */
      if (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, gwIpAddr))
      {
         cmsLog_error("gwIpAddr is %s", gwIpAddr);
         return;
      }

      snprintf(cmdLine, sizeof(cmdLine),
              "ip -6 ro %s default via %s dev %s 2>/dev/null",
              op, gwIpAddr, gwIntfName);
   }

   rut_doSystemAction("rut", cmdLine);
}

#endif

