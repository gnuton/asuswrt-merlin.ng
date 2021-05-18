/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
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
#include <arpa/inet.h> /* for inet_ntop */

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_network.h"
#include "rut_ebtables.h"
#include "rut_wan6.h"
#include "device2/rut2_iptunnel.h"
#include "device2/rut2_ipv6.h"
#include "device2/rut2_ra.h"
#include "qdm_intf.h"
#include "qdm_ipintf.h"


void rutTunnel_control( const char *ifName, const char *mode, UBOOL8 activate )
{
   cmsLog_debug("ifName<%s> mode=%s activate=%d", ifName, mode, activate);

#if defined(DMP_X_BROADCOM_COM_IPV6_1)
   {
   UBOOL8 found=FALSE;
   _IPTunnelObject *tunnelCfg;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   while ( cmsObj_getNextFlags(MDMOID_IP_TUNNEL, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&tunnelCfg) == CMSRET_SUCCESS )
   {
      /*
       * There might be dual stack on the same WAN interface. We need to check the mode to distinguish the correct tunnelObj.
       * If the mode is NULL, it must be triggered by deleting the WAN interface (rutWan_deleteWanIpOrPppConnection).
       */
      if ( (cmsUtl_strcmp(tunnelCfg->associatedWanIfName, ifName) == 0) &&
           ((mode == NULL) || 
            ((mode != NULL) && (cmsUtl_strcmp(tunnelCfg->mode, mode) == 0))) 
         )
      {
         found = TRUE;

         tunnelCfg->activated = activate;
         if ( (ret = cmsObj_set(tunnelCfg, &iidStack)) != CMSRET_SUCCESS )
         {
            cmsLog_error("Failed to set MDMOID_IP_TUNNEL object");
            cmsObj_free((void **) &tunnelCfg);
            return;
         }
      }

      cmsObj_free((void **)&tunnelCfg);
   }

   if (!found)
   {
      cmsLog_debug("No tunnel associated with %s", ifName);
   }
   }

#elif defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
   {
      char *ifpath = NULL;
      UBOOL8 isLayer2 = FALSE;

      if ((mode != NULL) && (cmsUtl_strcmp(mode, MDMVS_IPV6INIPV4) == 0))
      {
         UBOOL8 prevHideObjectsPendingDelete;

         prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
         mdmLibCtx.hideObjectsPendingDelete = FALSE;

      
         /* In hybrid mode, 6rd must be triggered by WANIPConn/WANPppConn */
         if (qdmIntf_intfnameToFullPathLocked(ifName, isLayer2, &ifpath) != CMSRET_SUCCESS)
         {
            cmsLog_error("cannot get %s's full path", ifName);
         }
         else
         {
            cmsLog_debug("ifpath<%s>", ifpath);
            rutTunnel_ipv6rdControl(ifpath);
            CMSMEM_FREE_BUF_AND_NULL_PTR(ifpath);
         }
         
         mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
         
      }
   }
#else
   cmsLog_debug("Do nothing in non-IPv6 case");
#endif
   return;
}


UBOOL8 rutTunnel_containDynamicTunnel( const char *ifName, UBOOL8 is6in4 )
{
   cmsLog_debug("ifName<%s> 6in4<%d>", ifName, is6in4);

#if defined(DMP_X_BROADCOM_COM_IPV6_1)
   {
   UBOOL8 enable = FALSE;
   UBOOL8 found = FALSE;
   _IPTunnelObject *tunnelCfg;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;


   while ( !found &&
           (cmsObj_getNextFlags(MDMOID_IP_TUNNEL, &iidStack, OGF_NO_VALUE_UPDATE,
                                (void **)&tunnelCfg) == CMSRET_SUCCESS) )
   {
      if (cmsUtl_strcmp(tunnelCfg->associatedWanIfName, ifName) == 0)
      {
         found = TRUE;
      }

      cmsObj_free((void **)&tunnelCfg);
   }

   if ( found )
   {
      if ( is6in4 )
      {
         _Ipv6inIpv4TunnelObject *ipv6inipv4Obj;
         InstanceIdStack iidStack1=EMPTY_INSTANCE_ID_STACK;

         if( cmsObj_getNextInSubTreeFlags(MDMOID_IPV6IN_IPV4_TUNNEL, &iidStack, &iidStack1, 
                                          OGF_NO_VALUE_UPDATE, (void **)&ipv6inipv4Obj) == CMSRET_SUCCESS )
         {
            enable = ipv6inipv4Obj->dynamic;
            cmsObj_free((void **)&ipv6inipv4Obj);
         }
      }
      else
      {
         _Ipv4inIpv6TunnelObject *ipv4inipv6Obj;
         InstanceIdStack iidStack1=EMPTY_INSTANCE_ID_STACK;

         if( cmsObj_getNextInSubTreeFlags(MDMOID_IPV4IN_IPV6_TUNNEL, &iidStack, &iidStack1, 
                                          OGF_NO_VALUE_UPDATE, (void **)&ipv4inipv6Obj) == CMSRET_SUCCESS )
         {
            enable = ipv4inipv6Obj->dynamic;
            cmsObj_free((void **)&ipv4inipv6Obj);
         }
      }
   }
   return enable;
   }
#elif defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
   {
   UBOOL8 dynamic = FALSE;

   if (is6in4)
   {
      char *fullPath = NULL;
      UBOOL8 isLayer2 = FALSE;

      qdmIntf_intfnameToFullPathLocked(ifName, isLayer2, &fullPath);
      cmsLog_debug("fullpath<%s>", fullPath);
      dynamic = rutTunnel_isDynamicTunnel(fullPath, TRUE);

      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
   }
   else
   {
      cmsLog_error("should not be called in dslite!");
   }

   cmsLog_debug("dynamic<%d>", dynamic);
   return dynamic;
   }
#else
   cmsLog_error("Do nothing in non-IPv6 case");
   return FALSE;
#endif
}


#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
UBOOL8 rutTunnel_activateByLanAddr( const char *ifName )
{
   UBOOL8 found;
   _IPTunnelObject *tunnelCfg;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   char guAddr[CMS_IPADDR_LENGTH]={0};
   UINT32 prefixLen=0;

   /* 
    * This function should be only used for DS-Lite tunnel.
    * Criteria to activate DS-Lite tunnel here are"
    * 1. An inactive DS-Lite tunnel associated with the WAN interface.
    * 2. The WAN interface does not get GUA.
    *
    * The WAN interface must be up at this point??
    */
   cmsLog_debug("ifName<%s>", ifName);

   found = FALSE;
   if (cmsNet_getGloballyUniqueIfAddr6(ifName, guAddr, &prefixLen) == CMSRET_SUCCESS)
   {
      cmsLog_debug("found GUA at %s, return", ifName);
      return found;
   }

   while ( !found && 
           (cmsObj_getNextFlags(MDMOID_IP_TUNNEL, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&tunnelCfg) == CMSRET_SUCCESS)
         )
   {
      if ( (cmsUtl_strcmp(tunnelCfg->associatedWanIfName, ifName) == 0) &&
           (cmsUtl_strcmp(tunnelCfg->mode, MDMVS_IPV4INIPV6) == 0) &&
           (tunnelCfg->activated == FALSE)
         )
      {
         found = TRUE;
      }

      cmsObj_free((void **)&tunnelCfg);
   }

   cmsLog_debug("activate<%d>", found);
   return found;
}


void rutTunnel_getLanAddrForDsLite( const char *wanIntf, char *ipaddr )
{
   InstanceIdStack LaniidStack = EMPTY_INSTANCE_ID_STACK;
   _DelegatedAddressObject *delegatedAddr = NULL;
   UBOOL8 found = FALSE;

   while ( (!found) &&
           (cmsObj_getNextFlags(MDMOID_DELEGATED_ADDRESS, &LaniidStack, 
                                OGF_NO_VALUE_UPDATE, (void **)&delegatedAddr) == CMSRET_SUCCESS) )
   {
      if ( (cmsUtl_strcmp(delegatedAddr->mode, MDMVS_WANDELEGATED) == 0) &&
           (cmsUtl_strcmp(delegatedAddr->delegatedConnection, wanIntf) == 0) )
      {
         found = TRUE;
         cmsUtl_strncpy(ipaddr, delegatedAddr->IPv6InterfaceAddress, CMS_IPADDR_LENGTH);
      }

      cmsObj_free((void **) &delegatedAddr);
   }
}


void rutTunnel_getAssociatedWanInfo( const char *wanIntf, char *ipaddr, UBOOL8 *firewall, UBOOL8 isIPv4 )
{
   InstanceIdStack iidStack;
   _WanPppConnObject *pppConn = NULL;
   _WanIpConnObject  *ipConn = NULL;
   UBOOL8 found = FALSE;

   cmsLog_debug("Enter");
   ipaddr[0] = '\0';

   INIT_INSTANCE_ID_STACK(&iidStack);
   /* get the related ipConn obj */
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(wanIntf, ipConn->X_BROADCOM_COM_IfName) )
      {
         found = TRUE;
         *firewall = (ipConn->X_BROADCOM_COM_FirewallEnabled | ipConn->NATEnabled);

         if ( isIPv4 )
         {
            if ( cmsUtl_strcmp(ipConn->connectionStatus, MDMVS_CONNECTED) == 0 )
            {
               cmsUtl_strncpy(ipaddr, ipConn->externalIPAddress, CMS_IPADDR_LENGTH);
            }
            else
            {
               cmsLog_debug("WAN interface is not up yet!");
            }
         }
         else
         {
            if ( cmsUtl_strcmp(ipConn->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED) == 0 )
            {
               UBOOL8 found_addr = FALSE;

               if ( cmsUtl_isValidIpAddress(AF_INET6, ipConn->X_BROADCOM_COM_ExternalIPv6Address) )
               {
                  cmsLog_debug("Fetch IPv6 address from WANIPConnection");
                  cmsUtl_strncpy(ipaddr, ipConn->X_BROADCOM_COM_ExternalIPv6Address, CMS_IPADDR_LENGTH);
                  found_addr = TRUE;
               }
               else
               {
                  InstanceIdStack iidStack1 = iidStack;
                  InstanceIdStack ipv6AddriidStack = EMPTY_INSTANCE_ID_STACK;
                  UBOOL8 found1 = FALSE;
                  void *obj;
                  CmsRet ret;

                  cmsLog_debug("Fetch IPv6 address from WANEthernetInterfaceConfig");
                  if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_ETH_INTF,
                                    MDMOID_WAN_IP_CONN, &iidStack1, OGF_NO_VALUE_UPDATE, &obj)) != CMSRET_SUCCESS)
                  {
                     cmsLog_error("Fail to get cmsObj_getAncestor(MDMOID_WAN_ETH_INTF). ret=%d", ret);
                     cmsObj_free((void **) &ipConn);
                     return;
                  }
                  cmsObj_free(&obj);

                  while ( !found1 && 
                          (cmsObj_getNextInSubTreeFlags(MDMOID_I_PV6_ADDR, &iidStack1, &ipv6AddriidStack, 
                                                        OGF_NO_VALUE_UPDATE, &obj) == CMSRET_SUCCESS) )
                  {
                     _IPv6AddrObject *ipv6AddrObj = (_IPv6AddrObject *)obj;

                     if ( !cmsUtl_strcmp(ipv6AddrObj->scope, MDMVS_GUA) )
                     {
                        cmsUtl_strncpy(ipaddr, ipv6AddrObj->IPv6Address, CMS_IPADDR_LENGTH);
                        found1 = found_addr = TRUE;
                     }
                     cmsObj_free(&obj);
                  }
               }

               /* If there is no address associated with WAN interface, use the delegated address from brX */
               if ( !found_addr )
               {
                  rutTunnel_getLanAddrForDsLite(wanIntf, ipaddr);
               }
            }
            else
            {
               cmsLog_debug("WAN interface is not up yet!");
            }
         }
      }
      cmsObj_free((void **) &ipConn);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(wanIntf, pppConn->X_BROADCOM_COM_IfName) )
      {
         found = TRUE;
         *firewall = (pppConn->X_BROADCOM_COM_FirewallEnabled | pppConn->NATEnabled);

         if ( isIPv4 )
         {
            if ( cmsUtl_strcmp(pppConn->connectionStatus, MDMVS_CONNECTED) == 0 )
            {
               cmsUtl_strncpy(ipaddr, pppConn->externalIPAddress, CMS_IPADDR_LENGTH);
            }
            else
            {
               cmsLog_debug("WAN interface is not up yet!");
            }
         }
         else
         {
            if ( cmsUtl_strcmp(pppConn->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED) == 0 )
            {
               if ( cmsUtl_isValidIpAddress(AF_INET6, pppConn->X_BROADCOM_COM_ExternalIPv6Address) )
               {
                  cmsUtl_strncpy(ipaddr, pppConn->X_BROADCOM_COM_ExternalIPv6Address, CMS_IPADDR_LENGTH);
               }
               else
               {
                  /* If there is no address associated with WAN interface, use the delegated address from brX */
                  rutTunnel_getLanAddrForDsLite(wanIntf, ipaddr);
               }
            }
            else
            {
               cmsLog_debug("WAN interface is not up yet!");
            }
         }
      }
      cmsObj_free((void **) &pppConn);
   }

   return;
}

#endif

#ifdef SUPPORT_IPV6
CmsRet rutTunnel_4in6Config( const char *wanIp, const char *remoteIp, UBOOL8 add )
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmdStr[BUFLEN_256];

   cmsLog_debug("Enter for %s: wanIp<%s> remoteIp<%s>", add?"ADD":"DEL", wanIp, remoteIp);

   if ( add )
   {
      char addr6[CMS_IPADDR_LENGTH];
      UINT32 plen;

      if (cmsUtl_parsePrefixAddress(wanIp, addr6, &plen) != CMSRET_SUCCESS)
      {
         cmsLog_error("Invalid ipv6 address=%s", wanIp);
         return CMSRET_INVALID_PARAM_VALUE;
      }

      snprintf(cmdStr, sizeof(cmdStr), "ip -6 tunnel add ip6tnl1 mode ip4ip6 remote %s local %s tclass inherit dscp inherit", remoteIp, addr6);
      rut_doSystemAction("4in6CfgAdd", cmdStr);

      snprintf(cmdStr, sizeof(cmdStr), "ip link set dev ip6tnl1 up");
      rut_doSystemAction("4in6CfgAdd", cmdStr);

      snprintf(cmdStr, sizeof(cmdStr), "ip -6 addr add %s dev ip6tnl1", wanIp);
      rut_doSystemAction("4in6CfgAdd", cmdStr);

      snprintf(cmdStr, sizeof(cmdStr), "ip ro add default dev ip6tnl1");
      rut_doSystemAction("4in6CfgAdd", cmdStr);

      /* RFC 6333 section 5.7: 192.0.0.2/29 is reserved for B4 */
      snprintf(cmdStr, sizeof(cmdStr), "ip addr add 192.0.0.2/29 dev ip6tnl1");
      rut_doSystemAction("4in6CfgAdd", cmdStr);
   }
   else
   {
      snprintf(cmdStr, sizeof(cmdStr), "ip ro del default dev ip6tnl1");
      rut_doSystemAction("4in6CfgDel", cmdStr);

      snprintf(cmdStr, sizeof(cmdStr), "ip link set dev ip6tnl1 down");
      rut_doSystemAction("4in6CfgDel", cmdStr);

      snprintf(cmdStr, sizeof(cmdStr), "ip -6 tunnel del ip6tnl1");
      rut_doSystemAction("4in6CfgDel", cmdStr);
   }

   return ret;
}

#if 0
CmsRet rutTunnel_get6rdLanAddr(const char *prefixStr, const char *lanIntf, char *addr)
{
   UINT8 macAddr[MAC_ADDR_LEN];
   CmsRet ret;

   rutLan_getIntfMacAddr(lanIntf, macAddr);

   if ((ret = cmsUtl_prefixMacToAddress(prefixStr, macAddr, addr)) != CMSRET_SUCCESS)
   {
      cmsLog_debug("cannot generate 6rd LAN addr based on EUI64");
   }

   return ret;
}
#endif

CmsRet rutTunnel_6rdDelegatePrefixAddress(const char *tunnelName __attribute__((unused)),
                                          const char *ipv6AddrStr,
                                          const char *ipv6str,
                                          const char *lanIntf)
{
   CmsRet ret = CMSRET_SUCCESS;

#if defined(DMP_X_BROADCOM_COM_IPV6_1)
   /* Configure LAN address by setting the LAN object */
   if ( (ret = rutWan_addDelegatedAddrEntry(tunnelName, ipv6AddrStr, lanIntf, MDMVS_TUNNELDELEGATED)) == CMSRET_SUCCESS )
   {
      if ( (ret = rutWan_addPDEntry(tunnelName, ipv6str, NULL, -1, -1, -1, MDMVS_TUNNELDELEGATED))  != CMSRET_SUCCESS )
      {
         cmsLog_error("rutWan_addPDEntry returns error = %d", ret);
      }
   }
#elif defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
   /* 
    * Delegate 6rd prefix and address:
    * 1. prefix with origin=CHILD
    * 2. ipv6AddrStr is in the following format: prefix::1/64
    * 3. ipv6str may come with prefix length < 64
    * FIXME: create a flag in prefix object to indicate 6rd prefix?
    */
   {
   char ifname[CMS_IFNAME_LENGTH];
   char networkPrefix[CMS_IPADDR_LENGTH];
   char prefixPathRef[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   char *ptr;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack_ipv6Prefix;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   
   qdmIntf_fullPathToIntfnameLocked_dev2(lanIntf, ifname);
   qdmIntf_getPathDescFromIntfnameLocked_dev2(ifname, FALSE, &pathDesc);

   if (cmsNet_subnetIp6SitePrefix(ipv6str, 0, 64, networkPrefix) == CMSRET_SUCCESS)
   {
      cmsUtl_strcat(networkPrefix, "/64");
   }
   else
   {
      cmsLog_error("cmsNet_subnetIp6SitePrefix returns error");
   }
 
   ptr = cmsUtl_strstr(ipv6AddrStr, "/");
   *ptr = '\0';

   if (!rutIp_findIpv6Prefix(&pathDesc.iidStack, networkPrefix, MDMVS_CHILD, 
                             MDMVS_INAPPLICABLE, &iidStack_ipv6Prefix))
   {
      Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
      InstanceIdStack iidStack_ipv6Addr;
      UBOOL8 found = FALSE;

      rutIp_addIpv6Prefix(&pathDesc.iidStack, networkPrefix, MDMVS_CHILD,
                          MDMVS_INAPPLICABLE, NULL, NULL, TRUE, TRUE,
                          -1, -1, prefixPathRef, sizeof(prefixPathRef));

      /* find delegated 6rd address object */
      INIT_INSTANCE_ID_STACK(&iidStack_ipv6Addr);
      while (!found &&
              cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS,
                              &pathDesc.iidStack, &iidStack_ipv6Addr,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&ipv6AddrObj) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(ipv6AddrObj->prefix, prefixPathRef))
         {
            cmsLog_debug("found 6rd delegated address<%s>", ipv6AddrObj->IPAddress);
            found = TRUE;
         }
         cmsObj_free((void **) &ipv6AddrObj);         
      }

      if (!found)
      {
         rutIp_addIpv6Addr(&pathDesc.iidStack, ipv6AddrStr, MDMVS_AUTOCONFIGURED, prefixPathRef, -1, -1);
      }

      /* update raintf.prefixes and trigger radvd */
      rutRa_updateRouterAdvObj(lanIntf);
   }
   else
   {
      cmsLog_debug("6rd prefix already exists");
   }
   }
#endif 

   return ret;
}

#if defined(DMP_X_BROADCOM_COM_IPV6_1)
CmsRet rutTunnel_6rdDeleteDelegatePrefixAddress(const char *tunnelName __attribute__((unused)),
                                                const char *lanIntf __attribute__((unused)))
#elif defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
CmsRet rutTunnel_6rdDeleteDelegatePrefixAddress(const char *tunnelName __attribute__((unused)),
                                                const char *lanIntf)
#endif
{
   CmsRet ret = CMSRET_SUCCESS;

#if defined(DMP_X_BROADCOM_COM_IPV6_1)
   if ( (ret = rutWan_deleteDelegatedAddrEntry(tunnelName, MDMVS_TUNNELDELEGATED)) != CMSRET_SUCCESS )
   {
      cmsLog_error("rutWan_deleteDelegatedAddrEntry returns error = %d", ret);
   }
   if ( (ret = rutWan_deletePDEntry(tunnelName, MDMVS_TUNNELDELEGATED)) != CMSRET_SUCCESS )
   {
      cmsLog_error("rutWan_deletePDEntry returns error = %d", ret);
   }
#elif defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
   /*
    * Currently, only 6rd delegated prefix is origin==CHILD.
    * So search prefix object with origin==CHILD  under lanIntf
    * and delete the associated address object too
    *
    * FIXME: should we use a flag instead of origin==CHILD to identify 6rd?
    */
   {
   char ifname[CMS_IFNAME_LENGTH];
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack_ipv6Prefix;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   
   qdmIntf_fullPathToIntfnameLocked_dev2(lanIntf, ifname);
   qdmIntf_getPathDescFromIntfnameLocked_dev2(ifname, FALSE, &pathDesc);

   if (rutIp_findIpv6Prefix(&pathDesc.iidStack, NULL, MDMVS_CHILD, 
                             MDMVS_INAPPLICABLE, &iidStack_ipv6Prefix))
   {
      MdmPathDescriptor prefixPD;
      char *fullPathStringPtr=NULL;

      memset(&prefixPD, 0, sizeof(MdmPathDescriptor));
      prefixPD.iidStack = iidStack_ipv6Prefix;
      prefixPD.oid = MDMOID_DEV2_IPV6_PREFIX;

      if ((ret=cmsMdm_pathDescriptorToFullPathNoEndDot(&prefixPD, 
                                       &fullPathStringPtr)) == CMSRET_SUCCESS)
      {
         Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
         InstanceIdStack iidStack_ipv6Addr;
         UBOOL8 found = FALSE;

         /* find delegated 6rd address object */
         INIT_INSTANCE_ID_STACK(&iidStack_ipv6Addr);
         while (!found &&
                 cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS,
                                 &pathDesc.iidStack, &iidStack_ipv6Addr,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&ipv6AddrObj) == CMSRET_SUCCESS)
         {
            if (!cmsUtl_strcmp(ipv6AddrObj->prefix, fullPathStringPtr))
            {
               cmsLog_debug("found 6rd delegated address<%s>", ipv6AddrObj->IPAddress);
               found = TRUE;
            }
            cmsObj_free((void **) &ipv6AddrObj);         
         }

         if (found)
         {
            if ((ret=cmsObj_deleteInstance(MDMOID_DEV2_IPV6_ADDRESS,
                                      &iidStack_ipv6Addr)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to delete ipv6AddrObj");
            }
         }
         else
         {
            cmsLog_notice("no 6rd delegated address found");
         }

         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);
      }
      else
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPathNoEndDot returns error");
      }

      if ((ret=cmsObj_deleteInstance(MDMOID_DEV2_IPV6_PREFIX,
                                &iidStack_ipv6Prefix)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to delete ipv6AddrObj");
      }
      /* update raintf.prefixes and trigger radvd */
      rutRa_updateRouterAdvObj(lanIntf);
   }
   else
   {
      cmsLog_notice("no 6rd delegated prefix found");
   }
   }
#endif

   return ret;
}

CmsRet rutTunnel_6rdConfig(const char *wanIp, const char *prefix, const char *brAddr, SINT32 ipv4MaskLen,
                           const char *tunnelName, const char *lanIntf, UBOOL8 add)
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmdStr[BUFLEN_256];

   cmsLog_debug("Enter for %s", add?"ADD":"DEL");

   if ( add )
   {
      char ipv6AddrStr[CMS_IPADDR_LENGTH+BUFLEN_8];
      char ipv4str[CMS_IPADDR_LENGTH];
      char ipv6str[CMS_IPADDR_LENGTH];
      char addr6[CMS_IPADDR_LENGTH];
      SINT32 i, plen, tmpMask=0;
      struct in6_addr in6Addr;
      struct in_addr in4Addr, tmpAddr4;
   
      if (inet_pton(AF_INET, wanIp, &in4Addr) <= 0)
      {
         cmsLog_error("Invalid WAN IPv4 address=%s", wanIp);
         return CMSRET_INVALID_PARAM_VALUE;
      }
   
      for ( i=0;i<ipv4MaskLen;i++ )
      {
         tmpMask |= (1<<i);
      }

      tmpMask = (int)htonl(tmpMask << (32-ipv4MaskLen));
      cmsLog_debug("tmpMask= %x\n", tmpMask);

      /* get 6rd-relay_prefix, ex:10.0.0.0 */

      /* 
       * in4Addr from inet_pton is in network order, so tmpMask needs to be in
       * network order too. so tmpAddr4 will be in network order.
       */
      tmpAddr4.s_addr = ((int)in4Addr.s_addr & tmpMask);
      inet_ntop(AF_INET, &tmpAddr4, ipv4str, BUFLEN_16);
      cmsLog_debug("A: tmpAddr4= %s %x\n", ipv4str, (int)tmpAddr4.s_addr);
   
      /* Generate the IPv4 part of the 6rd prefix for the LAN */
      tmpMask = ~tmpMask;
   
      /* Create prefix information for the LAN address */
      if (cmsUtl_parsePrefixAddress(prefix, addr6, (UINT32 *)&plen) != CMSRET_SUCCESS)
      {
         cmsLog_error("Invalid ipv6 address=%s", prefix);
         return CMSRET_INVALID_PARAM_VALUE;
      }
    
      if (inet_pton(AF_INET6, addr6, &in6Addr) <= 0)
      {
         cmsLog_error("Invalid ipv6 address=%s", prefix);
         return CMSRET_INVALID_PARAM_VALUE;
      }

      /*
       * Given v4Addr/v4MaskLen and v6Prefix/v6PrefixLen, we need to calculate the Delegation info as the following.
       * Append IPv4 non-masked part right after IPv6 prefix non-masked part.
       *
       * Example 1, IPv6 prefix info: 2001:db00::/24, IPv4 address info: 10.100.100.1/8
       * The delegated prefix would be 2001:db64:6401::/48
       * Example 2, IPv6 prefix info: 2001:db8::/37, IPv4 address info: 10.100.100.1/8
       * The delegated prefix would be 2001:db8:323:2008::/61
       *
       * Here is the algorithm: We treat IPv6 prefix as four uint32_t elements.
       * Length of non-masked IPv4 address: v4Need
       * Length of non-masked IPv6 prefix in the (v6PrefixLen/32)th element: v6Used=v6PrefixLen%32
       *
       * We first make sure the non-masked IPv4 address align: (v4Addr <<= v4MaskLen)
       * Then we can easily append the v4Addr to the IPv6 prefix.
       */
      {
         int v4Need = 32 - ipv4MaskLen;
         int v6Used = plen%32;
         int v4Space = 32 - v6Used;
         struct in_addr v4Addr;

         /*
          * To do bit shifting operation, convert in4Addr to host order.
          * So v4Addr will be in host order. 
          * Before applying v4Addr to in6Addr (from inet_pton in network order),
          * we need to convert v4Addr to network order again.
          */
         v4Addr.s_addr = (ntohl(in4Addr.s_addr) << ipv4MaskLen);

         in6Addr.s6_addr32[plen/32] |= htonl(v4Addr.s_addr >> v6Used);
         if ( v4Space < v4Need )
         {
            v4Addr.s_addr <<= v4Space;
            in6Addr.s6_addr32[(plen/32)+1] |= htonl(v4Addr.s_addr);
         }
      }

      inet_ntop(AF_INET6, &in6Addr, ipv6str, BUFLEN_48);
      cmsLog_debug("tmpAddr6= %s\n", ipv6str);

#if 0  //always use ::1 as address instead of SLAAC
      if (rutTunnel_get6rdLanAddr(ipv6str, lanIntf, ipv6AddrStr) == CMSRET_SUCCESS)
      {
         snprintf(ipv6AddrStr, sizeof(ipv6AddrStr), "%s/64", ipv6AddrStr); /* Address assignment */
      }
      else
#endif
      {
         snprintf(ipv6AddrStr, sizeof(ipv6AddrStr), "%s1/64", ipv6str); /* Address assignment */
      }

      {
         char tmp[BUFLEN_8];
         snprintf(tmp, sizeof(tmp), "/%d", plen+(32-ipv4MaskLen)); /* Prefix delegation */
         cmsUtl_strncat(ipv6str, CMS_IPADDR_LENGTH, tmp);
      }

      if ((ret = rutTunnel_6rdDelegatePrefixAddress(tunnelName, ipv6AddrStr, ipv6str, lanIntf)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to delegate 6rd prefix or address");
      }

      /* Execute the commands */
      snprintf(cmdStr, sizeof(cmdStr), "ip tunnel add sit1 mode sit local %s ttl 64 tos 1", wanIp);
      rut_doSystemAction("6rdConfigAdd", cmdStr);
      
      snprintf(cmdStr, sizeof(cmdStr), "ip tunnel 6rd dev sit1 6rd-prefix %s 6rd-relay_prefix %s/%d 6rd-br_addr %s", prefix, ipv4str, ipv4MaskLen, brAddr);
      rut_doSystemAction("6rdConfigAdd", cmdStr);
      
      snprintf(cmdStr, sizeof(cmdStr), "ip link set dev sit1 up");
      rut_doSystemAction("6rdConfigAdd", cmdStr);
      
      snprintf(cmdStr, sizeof(cmdStr), "ip -6 ro add default via ::%s dev sit1 metric 1", brAddr);
      rut_doSystemAction("6rdConfigAdd", cmdStr);
      
      snprintf(cmdStr, sizeof(cmdStr), "ip -6 ro add %s dev sit1", prefix);
      rut_doSystemAction("6rdConfigAdd", cmdStr);

      rutEbt_configICMPv6Reply(prefix, TRUE);
   }
   else
   {
      if ((ret = rutTunnel_6rdDeleteDelegatePrefixAddress(tunnelName, lanIntf)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to delete delegate 6rd prefix or address");
      }

      snprintf(cmdStr, sizeof(cmdStr), "ip -6 ro del %s dev sit1", prefix);
      rut_doSystemAction("6rdConfigDel", cmdStr);

      snprintf(cmdStr, sizeof(cmdStr), "ip -6 ro del default via ::%s dev sit1", brAddr);
      rut_doSystemAction("6rdConfigDel", cmdStr);

      snprintf(cmdStr, sizeof(cmdStr), "ip link set dev sit1 down");
      rut_doSystemAction("6rdConfigDel", cmdStr);

      snprintf(cmdStr, sizeof(cmdStr), "ip tunnel del sit1");
      rut_doSystemAction("6rdConfigDel", cmdStr);

      rutEbt_configICMPv6Reply(prefix, FALSE);
   }

   return ret;
}

#endif
