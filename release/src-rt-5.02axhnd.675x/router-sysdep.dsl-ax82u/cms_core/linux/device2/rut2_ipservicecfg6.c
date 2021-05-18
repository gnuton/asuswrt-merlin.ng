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
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_strconv2.h"
#include "rcl.h"
#include "rut_qos.h"
#include "rut_util.h"
#include "rut_wan6.h"
#include "rut_rip.h"
#include "rut2_ip.h"
#include "rut2_dhcpv6.h"
#include "qdm_ipintf.h"
#include "rut2_ipv6.h"
#include "rut2_dns.h"
#include "rut2_ra.h"
#include "rut2_iptunnel.h"
#include "rut_ipsec.h"
#include "rut_upnp.h"

#ifdef DMP_DEVICE2_IPV6ROUTING_1
#include "rut2_route.h"
#endif

/*!\file rut2_ipservicecfg6.c
 * \brief This file contains functions for the IP.Interface IPv6 service
 * state machine.  This is the TR181 version rut_ipconcfg6.c and
 * rut_pppconcfg6.c.
 * Note in TR181, we tried to make the IPv4 service state machine and
 * the IPv6 service state machine are independent of each other.  Although
 * in reality, the IPv6 state machine may still have some dependencies on
 * the IPv4 state machine.
 *
 */

static UBOOL8 getMflagFromIpIntfFullPath(const char *ipIntfFullPath);

void rutIpv6Service_runStateMachine(const char *newStatus,
         const char *currStatus,
         const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan)
{
   cmsLog_debug("%s >>> %s ifname=%s isWan=%d",
                currStatus, newStatus, ifname, isWan);

   if (!cmsUtl_strcmp(newStatus, MDMVS_SERVICESTARTING))   /*** NEW STATE ***/
   {
      if (!cmsUtl_strcmp(currStatus, MDMVS_SERVICEDOWN))
      {
         /*
          * DOWN    => STARTING
          * Start all services which do not need to know the IP address and
          * also could affect flowcache flows.
          * (call these "group 1" services?)
          */
         rutIpv6Service_start(ipIntfFullPath, ifname, isWan);
      }
      else if (!cmsUtl_strcmp(currStatus, MDMVS_SERVICEUP))
      {
         /*
          * UP => STARTING
          * We lost our IP address.
          * No need to look at transientlayer2linkstatus.  We know layer
          * 2 link is still up.  If layer 2 link is down, we would go to DOWN.
          * Stop the higher level services (the group 2 services)
          */
         rutIpv6Service_teardown(ipIntfFullPath, ifname, isWan);
      }
      else if (!cmsUtl_strcmp(currStatus, MDMVS_SERVICESTARTING))
      {
         /*
          * STARTING    => STARTING
          * Should happen only for re-launching dhcp6c due to change of Mflag
          */
         rutIpv6Service_launchDhcp6c(ipIntfFullPath, ifname);
      }
      else
      {
         cmsLog_error("illegal status transition: newStatus=%s currStatus=%s",
               newStatus, currStatus);
      }
   }
   else if (!cmsUtl_strcmp(newStatus, MDMVS_SERVICEUP))  /*** NEW STATE ***/
   {
      if (!cmsUtl_strcmp(currStatus, MDMVS_SERVICESTARTING))
      {
         /*
          * STARTING => UP
          * The only way to get to UP is via STARTING.
          * We got an IP address.  Start all high level services (call it
          * "Group 2" services?)
          */
         rutIpv6Service_setup(ipIntfFullPath, ifname, isWan);
      }
      else if (!cmsUtl_strcmp(currStatus, MDMVS_SERVICEUP))
      {
         /*
          * UP    => UP
          * Should happen only for re-launching dhcp6c due to change of Mflag
          */
         rutIpv6Service_launchDhcp6c(ipIntfFullPath, ifname);
      }
      else
      {
         cmsLog_error("illegal status transition: newStatus=%s currStatus=%s",
               newStatus, currStatus);
      }
   }
   else if (!cmsUtl_strcmp(newStatus, MDMVS_SERVICEDOWN)) /*** NEW STATE ***/
   {
      /* we enter DOWN if the lower layer link is down. */
      if (!cmsUtl_strcmp(currStatus, MDMVS_SERVICEUP))
      {
         /*
          * UP => DOWN
          * we were fully up, stop/restart all higher/lower level services
          * and stop dhcp client if there is one running.  Due to interface
          * stack, we do not have to stop ppp here because ppp is on the
          * interface stack and it
          * will get stopped from its own RCL handler function when status
          * is set to DORMANT.
          */
         rutIpv6Service_down(ipIntfFullPath, ifname, isWan);
      }
      else if (!cmsUtl_strcmp(currStatus, MDMVS_SERVICESTARTING))
      {
         /*
          * STARTING => DOWN
          * we were not fully up yet.  but we do need to kill dhcp client
          * if there was one running (also remove firewall rules and
          * unload modules?  basically, undo group 1 services).
          */
         rutIpv6Service_stop(ipIntfFullPath, ifname, isWan);
      }
      else
      {
         cmsLog_error("illegal status transition: newStatus=%s currStatus=%s",
               newStatus, currStatus);
      }
   }
   else
   {
      cmsLog_error("Unrecognized new status %s", newStatus);
   }
}


/* This is the TR181 version of rutCfg_startWanIpConnection or
 * startCfg_startWanPppConnection, except this does not start ppp.
 * Do all the prep work though, e.g. Firewall.
 * This is all the stuff that can be done before
 * an IP address is configured, and also before flowcache learns the flow.
 */
void rutIpv6Service_start(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan)
{
   UBOOL8 isFirewallEnabled=FALSE;

   cmsLog_debug("ipIntfFullPath=%s ifname=%s isWan=%d",
                ipIntfFullPath, ifname, isWan);

   rutIpv6Service_launchDhcp6c(ipIntfFullPath, ifname);
   
   if (!isWan)
   {
      /* XXX for now, if this is a LAN interface, return.  Later, we should
       * try to generalize processing for WAN and LAN.  LAN might have firewall
       * and vlan too.  But definately not NAT.
       */
      return;
   }

   /* XXX Lots of vlan configuration in this step */

   /* todo: need to check for ip intf is routed later by calling
   * qdmIpIntf_isRoutedWanExistedLocked
   */

#if defined(SUPPORT_DPI)
   /* with DPI enabled, always insmod conntrack/netlink modules on WAN up */
   rutIpt_insertIpModules6();
#endif

   isFirewallEnabled = qdmIpIntf_isFirewallEnabledOnIpIntfFullPathLocked_dev2(ipIntfFullPath);

   if (isFirewallEnabled)
   {   
      /* Assume routed for now.  todo: Need to be modified later on */
      rutIpt_insertIpModules6();

      rutIpt_initFirewall(PF_INET6, ifname);
      rutIpt_initFirewallExceptions(ifname);
      rutIpt_setupFirewallForDHCPv6(TRUE, ifname);
      rutIpt_createRoutingChain6();
      rutIpt_insertTCPMSSRules(PF_INET6, ifname);
   }


   cmsLog_debug("Exit");

   return;
}


/* this is the TR181 version of rutCfg_setupWanIpConnection or
 * rutCfg_setupWanPPPConnection.  All the services that must be run after
 * IP address is configured and does not affect flowcache flows.
 *
 */
void rutIpv6Service_setup(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan)
{
   CmsRet ret=CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc;

   cmsLog_debug("path/ifname/isWan: %s/%s/%d", ipIntfFullPath, ifname, isWan);

#ifdef DMP_DEVICE2_QOS_1
   /*
    * QoS classification config requires the egress L3 intf to be UP, so we
    * have to call here in _setup instead of _start.  But we configure
    * before the IP address is configured so flowcache will still learn
    * the right markings.
    */
   rutQos_reconfigAllClassifications_dev2(NULL);
#elif defined(DMP_QOS_1)
   /* In hybrid data model, IPv6 is TR181, but QoS is TR98.
      To fix QoS queue not configured for IPv6 only XTM WAN,
      we use TR98 functions to setup queue, default policy, and port shaping. */
   if (isWan)
   {
      if ((ret = rutQos_qMgmtQueueReconfig(ifname, FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtQueueReconfig returns error. ret=%d", ret);
         return ret;
      }
      rutQos_doDefaultPolicy();
      rutQos_portShapingConfigAll();
   }
#endif

   /* Also fold lan side case here(?):
    * If there is a DHCP server pointed to this intf, start it.
    */

   /* set static IPv6 address. FIXME: LAN address?? */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   ret = cmsMdm_fullPathToPathDescriptor(ipIntfFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", ipIntfFullPath, ret);
   }

   rutIp_configureStaticIpv6Addrs(&pathDesc.iidStack, ifname);

   if (!isWan)
   {
      cmsLog_debug("XXX (LAN side) start DHCP server for %s LATER ON ?", ifname);
#ifdef DMP_DEVICE2_HOMEPLUG_1
      /* Configure DNS for HomePlug interface (which is LAN side) */
      ret = rutDns_activateIpv6DnsServers_dev2(ipIntfFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("rutDns_activateIpv6DnsServers_dev2 failed on %s. ret %d",
               ipIntfFullPath, ret);
         return;
      }
#endif 
      return;
   }

#ifdef DMP_DEVICE2_IPV6ROUTING_1
   /* This does Default Gateway and all static routes */
   if ((ret = rutRt_activateIpv6Routing_dev2(ipIntfFullPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutRt_activateIpv6Routing_dev2 failed on %s. ret %d",
                    ipIntfFullPath, ret);
   }
#endif


   /* delegate prefix */
   {
      char prefix[CMS_IPADDR_LENGTH];
      char prefixOld[CMS_IPADDR_LENGTH];
      SINT32 plt=0;
      SINT32 vlt=0;
      SINT32 vltOld=0;
      MdmPathDescriptor prefixPathDesc;
      char *prefixFullPath=NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      Dev2Ipv6PrefixObject *prefixObj=NULL;
      UBOOL8 found = FALSE;
      UBOOL8 foundOld = FALSE;

      cmsLog_debug("Do prefix delegation!");

      while (!found && (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_PREFIX,
                                       &pathDesc.iidStack,
                                       &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **)&prefixObj) == CMSRET_SUCCESS))
      {
         if (prefixObj->enable && !cmsUtl_strcmp(prefixObj->origin, MDMVS_STATIC) &&
              !cmsUtl_strcmp(prefixObj->staticType, MDMVS_PREFIXDELEGATION) &&
              cmsUtl_isValidIpAddress(AF_INET6, prefixObj->prefix))
         {
            INIT_PATH_DESCRIPTOR(&prefixPathDesc);
            prefixPathDesc.oid = MDMOID_DEV2_IPV6_PREFIX;
            prefixPathDesc.iidStack = iidStack;

            if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&prefixPathDesc, &prefixFullPath)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
               cmsObj_free((void **) &prefixObj);
               return;
            }

            cmsUtl_strncpy(prefix, prefixObj->prefix, CMS_IPADDR_LENGTH);
            plt = prefixObj->X_BROADCOM_COM_Plt;
            vlt = prefixObj->X_BROADCOM_COM_Vlt;

            if (cmsUtl_isValidIpAddress(AF_INET6, prefixObj->X_BROADCOM_COM_Prefix_Old))
            {
               cmsUtl_strncpy(prefixOld, prefixObj->X_BROADCOM_COM_Prefix_Old, CMS_IPADDR_LENGTH);
               vltOld = prefixObj->X_BROADCOM_COM_Vlt_Old;
               foundOld = TRUE;
            }

            found = TRUE;
         }

         cmsObj_free((void **) &prefixObj);
      }

      if (found)
      {
         UBOOL8 foundChild = FALSE;
         InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
         Dev2Ipv6PrefixObject *prefixObjChild=NULL;

         while (!foundChild && (cmsObj_getNextFlags(MDMOID_DEV2_IPV6_PREFIX, &iidStackChild,
                    OGF_NO_VALUE_UPDATE, (void **)&prefixObjChild) == CMSRET_SUCCESS))
         {
            if (!cmsUtl_strcmp(prefixObjChild->parentPrefix, prefixFullPath))
            {
               foundChild = TRUE;
            }
            else
            {
               cmsObj_free((void **)&prefixObjChild);
            }
         }

         CMSMEM_FREE_BUF_AND_NULL_PTR(prefixFullPath);

         if (foundChild)
         {
            char networkPrefix[CMS_IPADDR_LENGTH];
            Dev2IpInterfaceObject *ipIntfObj=NULL;
            MdmPathDescriptor pathDescTmp;
            char *childIntfFullPath=NULL;
            InstanceIdStack iidStack_ipv6Addr;
            char lanIntfAddress[CMS_IPADDR_LENGTH];
            MdmPathDescriptor pathDescPrefixChild;      
            char *fullPathPrefixChild=NULL;
            char timestring[BUFLEN_128];
          
            if (cmsNet_subnetIp6SitePrefix(prefix, 0, 64, networkPrefix) == CMSRET_SUCCESS)
            {
               cmsUtl_strncpy(lanIntfAddress, networkPrefix, CMS_IPADDR_LENGTH);
               cmsUtl_strcat(networkPrefix, "/64");
            }
            else
            {
               cmsLog_error("cmsNet_subnetIp6SitePrefix returns error");
            }

            prefixObjChild->enable = TRUE;
            prefixObjChild->X_BROADCOM_COM_Plt = plt;
            prefixObjChild->X_BROADCOM_COM_Vlt = vlt;
            CMSMEM_REPLACE_STRING_FLAGS(prefixObjChild->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(prefixObjChild->prefixStatus, MDMVS_PREFERRED, mdmLibCtx.allocFlags);

            /* prefix itself retains a Route Information for radvd to advertise */
            CMSMEM_REPLACE_STRING_FLAGS(prefixObjChild->prefix, prefix, mdmLibCtx.allocFlags);

            if (foundOld)
            {
               prefixObjChild->X_BROADCOM_COM_Vlt_Old = vltOld;
               CMSMEM_REPLACE_STRING_FLAGS(prefixObjChild->X_BROADCOM_COM_Prefix_Old, prefixOld, mdmLibCtx.allocFlags);
            }

            cmsTms_getXSIDateTime(plt, timestring, sizeof(timestring));
            CMSMEM_REPLACE_STRING_FLAGS(prefixObjChild->preferredLifetime, timestring, mdmLibCtx.allocFlags);
            cmsTms_getXSIDateTime(vlt, timestring, sizeof(timestring));
            CMSMEM_REPLACE_STRING_FLAGS(prefixObjChild->validLifetime, timestring, mdmLibCtx.allocFlags);

            if ((ret = cmsObj_set(prefixObjChild, &iidStackChild)) != CMSRET_SUCCESS)
            {
               cmsLog_error("fail set prefix_child: ret=%d", ret);
            }

            cmsObj_free((void **)&prefixObjChild);

            /* Get the full path of prefix_child for address object */
            memset(&pathDescPrefixChild, 0, sizeof(MdmPathDescriptor));
            pathDescPrefixChild.oid = MDMOID_DEV2_IPV6_PREFIX;
            pathDescPrefixChild.iidStack = iidStackChild;
            if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDescPrefixChild, &fullPathPrefixChild)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsMdm_pathDescriptorToFullPathNoEndDot returns error. ret=%d", ret);
            }
            
            /* If PD is length of 60, all the traffic within the /60 must stay in LAN */
            rutWan_configPDRoute(prefix, TRUE);

            if ((ret = cmsObj_getAncestor(MDMOID_DEV2_IP_INTERFACE,
                                          MDMOID_DEV2_IPV6_PREFIX,
                                          &iidStackChild,
                                          (void **) &ipIntfObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("could not get parent IP.Interface, ret=%d", ret);
            }
            else
            {
               /* Create ip6tables rules to verify source addr within the delegated prefixes */
               if (qdmIpIntf_isFirewallEnabledOnIpIntfFullPathLocked_dev2(ipIntfFullPath))
               {
                  rutIpt_configRoutingChain6(networkPrefix, ipIntfObj->name, TRUE);
               }

               cmsObj_free((void **)&ipIntfObj);

               /* Set address (from the delegated prefix) to brX, pick the first address!! */
               cmsUtl_strcat(lanIntfAddress, "1");
               cmsLog_debug("set address<%s> to brX with iid<%s>", lanIntfAddress, cmsMdm_dumpIidStack(&iidStackChild));

               if (!rutIp_findIpv6Addr(&iidStackChild, lanIntfAddress, MDMVS_AUTOCONFIGURED, &iidStack_ipv6Addr))
               {
                  rutIp_addIpv6Addr(&iidStackChild, lanIntfAddress, MDMVS_AUTOCONFIGURED, fullPathPrefixChild, plt, vlt);
               }

               /* get fullpath of child intf obj for RAObj */
               INIT_PATH_DESCRIPTOR(&pathDescTmp);
               pathDescTmp.oid = MDMOID_DEV2_IP_INTERFACE;
               pathDescTmp.iidStack = iidStackChild;
   
               if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDescTmp, &childIntfFullPath)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
               }
            }

            CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathPrefixChild);

            /* update raintf.prefixes and trigger radvd */
            rutRa_updateRouterAdvObj(childIntfFullPath);

            /* update dhcp6s */
            {
               InstanceIdStack iidStackDhcp6sIntf = EMPTY_INSTANCE_ID_STACK;
               Dev2Dhcpv6ServerPoolObject *dhcp6sIntfObj = NULL;
               UBOOL8 foundDhcp6s = FALSE;

               cmsLog_debug("for dhcp6s:ip.intf<%s>", childIntfFullPath);
         
               while (!foundDhcp6s && cmsObj_getNext(MDMOID_DEV2_DHCPV6_SERVER_POOL, 
                                  &iidStackDhcp6sIntf, (void **) &dhcp6sIntfObj) == CMSRET_SUCCESS)
               {
                  if (!cmsUtl_strcmp(dhcp6sIntfObj->interface, childIntfFullPath))
                  {
                     foundDhcp6s = TRUE;
                  }
                  else
                  {
                     cmsObj_free((void **) &dhcp6sIntfObj);
                  }
               }

               if (foundDhcp6s)
               {
                  ret = cmsObj_set(dhcp6sIntfObj, &iidStackDhcp6sIntf);
                  cmsObj_free((void **) &dhcp6sIntfObj);
         
                  if (ret != CMSRET_SUCCESS)
                  {
                      cmsLog_error("Failed setting Dhcp6sPool. ret %d", ret);
                  }
               }
               else
               {
                  cmsLog_notice("Failed getting Dhcp6sPool.");
               }
            }

            CMSMEM_FREE_BUF_AND_NULL_PTR(childIntfFullPath);
         }
         else
         {
            cmsLog_error("cannot find pre-configure IPv6Prefix child object for PD!");
         }
      }
      else
      {
         cmsLog_debug("cannot find pre-configure IPv6Prefix object for PD, maybe no PD available?");
      }
   }

#ifdef DMP_DEVICE2_NEIGHBORDISCOVERY_1
   /* this incert default neighbor discovery and router solicitation setting */
   if ((ret = rutNd_activateNeighborDiscovery_dev2(ipIntfFullPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutNd_activateNeighborDiscovery_dev2 failed on %s. ret %d",
                    ipIntfFullPath, ret);
   }
#endif  /* DMP_DEVICE2_NEIGHBORDISCOVERY_1 */

   if (isWan)
   {
      /* only WAN side interfaces affect DNS.  LAN side does not affect it */
      ret = rutDns_activateIpv6DnsServers_dev2(ipIntfFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("rutDns_activateIpv6DnsServers_dev2 failed on %s. ret %d",
               ipIntfFullPath, ret);
         return;
      }

      /* multicast related stuff (WAN side only) */
      if (qdmMulti_isMldProxyEnabledOnIntfNameLocked_dev2(ifname) ||
          qdmMulti_isMldSourceEnabledOnIntfNameLocked_dev2(ifname))
      {
         rutIpt_mldRules(TRUE, ifname);
         rutMulti_updateMldProxyIntfList();
      }

      rutTunnel_dsliteControl(ipIntfFullPath);
#ifdef SUPPORT_MAP
      rutMap_mapControl(ipIntfFullPath);
#endif

#ifdef SUPPORT_IPSEC
      rutIPSec_activateTunnel();
#endif
      cmsLed_setWanConnected();

      printf("All IPv6 services associated with %s is activated.\n", ifname);
      /*
       * historically, CMS_MSG_WAN_CONNECTION_UP has been used for both
       * IPv4 and IPv6.  But this could be a little confusing because the
       * recipient of the msg would not know whether IPv4 and/or IPv6 is UP.
       * (But maybe he doesn't care).  We could define an additional msg type:
       * CMS_MSG_WAN_IPV6_CONNECTION_UP /DOWN just for IPv6 WAN conn status.
       */
      rut_sendEventMsgToSmd(CMS_MSG_WAN_CONNECTION_UP, 0, ifname, strlen(ifname)+1);
   }

#ifdef SUPPORT_RIP
   /* reevaluate rip process attachment to reflect the current modem state */
   if (rut_isRipEnabled_dev2() && !rut_isApplicationRunning(EID_RIPD))
   {
      rutRip_restart_dev2();
   }
#endif
   
#ifdef SUPPORT_UPNP
   if (rut_isUpnpEnabled() && rutUpnp_checkRunStatusWithDelay() == FALSE)
   {
       rut_restartUpnp(NULL);
   }
#endif
#if 0
   /* TODO: 
     * Policy Routing??
     */

#endif

}


/* This is the TR181 version of rutCfg_tearDownWanIpConnection6 or rutCfg_tearDownWanPppConnection6.
 * We lost L3 above configuration, but LowerLayer link is still up.
 * Stop or restart all high level IP services, but keep dhcp client running.
 */
void rutIpv6Service_teardown(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan)
{
   CmsRet ret=CMSRET_SUCCESS;
   char prefix[CMS_IPADDR_LENGTH];
   MdmPathDescriptor pathDesc, prefixPathDesc;
   char *prefixFullPath=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack savedIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6PrefixObject *prefixObj=NULL;
   Dev2Ipv6AddressObject * addrObj = NULL;
   UBOOL8 found = FALSE;

   cmsLog_notice("fullpath=%s ifname=%s isWan=%d", ipIntfFullPath, ifname, isWan);
   

#ifdef DMP_DEVICE2_QOS_1
   /*
    * Since we configured QoS classifications when we acquired IP Addr
    * in _setup, we now need to unconfig QoS classifications when we lose
    * IP Addr.
    */
   {
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      rutQos_reconfigAllClassifications_dev2(ifname);

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
   }
#endif


   if (!isWan)
   {
      /* XXX TOOD: try to generalize this function to handle LAN and WAN.
       * For example, there could be a firewall on LAN side, so we might
       * need to delete iptables rules for LAN side going down.
       *
       */
      cmsLog_debug("Do nothing for lan for now.");
      return;
   }

   INIT_PATH_DESCRIPTOR(&pathDesc);
   ret = cmsMdm_fullPathToPathDescriptor(ipIntfFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", ipIntfFullPath, ret);
   }

   /* FIXME: remove interface group (policy route) */


   /* deal with addresses */
   /* XXX This address logic should be moved to rut2_ip6.c and modeled after
    * the logic for the IPv4 case.
    */
   cmsLog_debug("Do address!");

   INIT_INSTANCE_ID_STACK(&iidStack);
   INIT_INSTANCE_ID_STACK(&savedIidStack);
   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS,
                                    &pathDesc.iidStack,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&addrObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(addrObj->origin, MDMVS_STATIC))
      {
         /* dynamic address: delete it, which will also unconfigure it */
         if ((ret = cmsObj_deleteInstance(MDMOID_DEV2_IPV6_ADDRESS, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to delete ipv6AddrObj. ret=%d", ret);
         }
         /* since we did a delete, restore iidStack to last good one */
         iidStack = savedIidStack;
      }

      else
      {
         /* static address: do not delete, just unconfigure */
         rutIp_unconfigureIpv6Addr(ifname, addrObj->IPAddress);

         CMSMEM_REPLACE_STRING_FLAGS(addrObj->status, MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(addrObj->IPAddressStatus,
                                     MDMVS_INVALID, mdmLibCtx.allocFlags);

         if (cmsObj_set(addrObj, &iidStack) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set addrObj");
         }
      }

      cmsObj_free((void **) &addrObj);

      /* save current iidStack in case we delete the next one */
      savedIidStack = iidStack;
   }

   /* Deconfigure prefixes and PD related: prefix/addr/PDRoute */
   cmsLog_debug("Do prefix delegation!");

   INIT_INSTANCE_ID_STACK(&iidStack);
   INIT_INSTANCE_ID_STACK(&savedIidStack);
   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_PREFIX,
                                    &pathDesc.iidStack,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&prefixObj) == CMSRET_SUCCESS)
   {
      /* prefix delegation */
      if (prefixObj->enable && !cmsUtl_strcmp(prefixObj->origin, MDMVS_STATIC) &&
           !cmsUtl_strcmp(prefixObj->staticType, MDMVS_PREFIXDELEGATION))
      {
         INIT_PATH_DESCRIPTOR(&prefixPathDesc);
         prefixPathDesc.oid = MDMOID_DEV2_IPV6_PREFIX;
         prefixPathDesc.iidStack = iidStack;

         if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&prefixPathDesc, &prefixFullPath)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
            cmsObj_free((void **) &prefixObj);
            return;
         }

         if (cmsUtl_isValidIpAddress(AF_INET6, prefixObj->prefix))
         {
            cmsUtl_strncpy(prefix, prefixObj->prefix, CMS_IPADDR_LENGTH);
            found = TRUE;
         }

         CMSMEM_FREE_BUF_AND_NULL_PTR(prefixObj->prefix);
         CMSMEM_FREE_BUF_AND_NULL_PTR(prefixObj->X_BROADCOM_COM_Prefix_Old);
         prefixObj->X_BROADCOM_COM_Plt = 0;
         prefixObj->X_BROADCOM_COM_Vlt = 0;
         prefixObj->X_BROADCOM_COM_Vlt_Old = 0;
         CMSMEM_REPLACE_STRING_FLAGS(prefixObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(prefixObj->prefixStatus, MDMVS_INVALID, mdmLibCtx.allocFlags);

         if ((ret = cmsObj_set(prefixObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set ipv6PrefixObj. ret=%d", ret);
         }
      }
      /* prefix of static address */
      else if (prefixObj->enable && !cmsUtl_strcmp(prefixObj->origin, MDMVS_STATIC) &&
               !cmsUtl_strcmp(prefixObj->staticType, MDMVS_STATIC))
      {
         cmsLog_debug("do nothing for prefix of static address");
      }
      /* others like RA */
      else
      {
         if ((ret = cmsObj_deleteInstance(MDMOID_DEV2_IPV6_PREFIX, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to delete ipv6PrefixObj. ret=%d", ret);
         }
         /* since we did a delete, restore iidStack to last good one */
         iidStack = savedIidStack;
      }

      cmsObj_free((void **) &prefixObj);

      /* save current iidStack in case we delete the next one */
      savedIidStack = iidStack;
   }

   if (found)
   {
      UBOOL8 foundChild = FALSE;
      InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
      Dev2Ipv6PrefixObject *prefixObjChild=NULL;

      while (!foundChild && (cmsObj_getNextFlags(MDMOID_DEV2_IPV6_PREFIX, &iidStackChild,
                 OGF_NO_VALUE_UPDATE, (void **)&prefixObjChild) == CMSRET_SUCCESS))
      {
         if (!cmsUtl_strcmp(prefixObjChild->parentPrefix, prefixFullPath))
         {
            foundChild = TRUE;
         }
         else
         {
            cmsObj_free((void **)&prefixObjChild);
         }
      }

      if (foundChild)
      {
         char networkPrefix[CMS_IPADDR_LENGTH];
         Dev2IpInterfaceObject *ipIntfObj=NULL;
         MdmPathDescriptor pathDescTmp;
         char *childIntfFullPath=NULL;

         cmsUtl_strncpy(networkPrefix, prefixObjChild->prefix, CMS_IPADDR_LENGTH);

         CMSMEM_FREE_BUF_AND_NULL_PTR(prefixObjChild->prefix);
         CMSMEM_FREE_BUF_AND_NULL_PTR(prefixObjChild->X_BROADCOM_COM_Prefix_Old);
         prefixObjChild->X_BROADCOM_COM_Plt = 0;
         prefixObjChild->X_BROADCOM_COM_Vlt = 0;
         prefixObjChild->X_BROADCOM_COM_Vlt_Old = 0;
         CMSMEM_REPLACE_STRING_FLAGS(prefixObjChild->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(prefixObjChild->prefixStatus, MDMVS_INVALID, mdmLibCtx.allocFlags);

         if ((ret = cmsObj_set(prefixObjChild, &iidStackChild)) != CMSRET_SUCCESS)
         {
            cmsLog_error("fail set prefix_child: ret=%d", ret);
         }

         cmsObj_free((void **)&prefixObjChild);

         /* If PD is length of 60, all the traffic within the /60 must stay in LAN */
         rutWan_configPDRoute(prefix, FALSE);

         if ((ret = cmsObj_getAncestor(MDMOID_DEV2_IP_INTERFACE,
                                       MDMOID_DEV2_IPV6_PREFIX,
                                       &iidStackChild,
                                       (void **) &ipIntfObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not get parent IP.Interface, ret=%d", ret);
         }
         else
         {
            char *ptr;

            /* Delete firewall rules for source IP range of PD */
            if (qdmIpIntf_isFirewallEnabledOnIpIntfFullPathLocked_dev2(ipIntfFullPath))
            {
               rutIpt_configRoutingChain6(networkPrefix, ipIntfObj->name, FALSE);
            }

            cmsObj_free((void **)&ipIntfObj);

            /* delete corresponding address object at brX */
            ptr = cmsUtl_strstr(networkPrefix, "/");
            *ptr = '1';
            *(ptr+1) = '\0';

            cmsLog_debug("delete address<%s> to brX", networkPrefix);

            rutIp_deleteIpv6Addr(&iidStackChild, networkPrefix, MDMVS_AUTOCONFIGURED);

            /* get fullpath of child intf obj for RAObj */
            INIT_PATH_DESCRIPTOR(&pathDescTmp);
            pathDescTmp.oid = MDMOID_DEV2_IP_INTERFACE;
            pathDescTmp.iidStack = iidStackChild;

            if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDescTmp, &childIntfFullPath)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
            }
         }

         /* update raintf.prefixes and trigger radvd */
         rutRa_updateRouterAdvObj(childIntfFullPath);

         /* update dhcp6s */
         {
            InstanceIdStack iidStackDhcp6sIntf = EMPTY_INSTANCE_ID_STACK;
            Dev2Dhcpv6ServerPoolObject *dhcp6sIntfObj = NULL;
            UBOOL8 foundDhcp6s = FALSE;

            cmsLog_debug("for dhcp6s:ip.intf<%s>", childIntfFullPath);
         
            while (!foundDhcp6s && cmsObj_getNext(MDMOID_DEV2_DHCPV6_SERVER_POOL, 
                               &iidStackDhcp6sIntf, (void **) &dhcp6sIntfObj) == CMSRET_SUCCESS)
            {
               if (!cmsUtl_strcmp(dhcp6sIntfObj->interface, childIntfFullPath))
               {
                  foundDhcp6s = TRUE;
               }
               else
               {
                  cmsObj_free((void **) &dhcp6sIntfObj);
               }
            }

            if (foundDhcp6s)
            {
               ret = cmsObj_set(dhcp6sIntfObj, &iidStackDhcp6sIntf);
               cmsObj_free((void **) &dhcp6sIntfObj);
      
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed setting Dhcp6sPool. ret %d", ret);
               }
            }
            else
            {
               cmsLog_notice("Failed getting Dhcp6sPool.");
            }
         }

         CMSMEM_FREE_BUF_AND_NULL_PTR(childIntfFullPath);
      }
      else
      {
         cmsLog_error("cannot find pre-configure IPv6Prefix child object for PD!");
      }
   }
   else
   {
      cmsLog_notice("cannot find pre-configure IPv6Prefix object for PD, maybe no PD available!");
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(prefixFullPath);

#ifdef DMP_DEVICE2_IPV6ROUTING_1
   /* Select new default gateway, delete dynamic entries, unconfig static routes */
   rutRt_deactivateIpv6Routing_dev2(ipIntfFullPath);
#endif

#ifdef DMP_DEVICE2_NEIGHBORDISCOVERY_1
   /* Delete dynamic entries */
   rutNd_deactivateNeighborDiscovery_dev2(ipIntfFullPath);
#endif  /* DMP_DEVICE2_NEIGHBORDISCOVERY_1 */


   /* Clear DHCPv6.Client.RcvOption */
   {
      Dev2Dhcpv6ClientObject *dhcpClientObj=NULL;
      InstanceIdStack dhcpClientIidStack=EMPTY_INSTANCE_ID_STACK;
      UBOOL8 foundDhcp6c = FALSE;
   
      cmsLog_debug("clear rcvOption");
   
      while (!foundDhcp6c &&
             ((ret = cmsObj_getNextFlags(MDMOID_DEV2_DHCPV6_CLIENT,
                                &dhcpClientIidStack, OGF_NO_VALUE_UPDATE,
                                (void **)&dhcpClientObj)) == CMSRET_SUCCESS))
      {
         if (!cmsUtl_strcmp(dhcpClientObj->interface, ipIntfFullPath))
         {
            foundDhcp6c = TRUE;
         }

         cmsObj_free((void **)&dhcpClientObj);
      }
      
      if (foundDhcp6c)
      {
         Dev2Dhcp6cRcvOptionObject *dhcp6cRcvObj=NULL;
         InstanceIdStack dhcp6cRcvIidStack;
         dhcp6cRcvIidStack = dhcpClientIidStack;
         if ((ret = cmsObj_get(MDMOID_DEV2_DHCP6C_RCV_OPTION, &dhcp6cRcvIidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&dhcp6cRcvObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get DHCPv6.Client.{i}.X_BROADCOM_COM_RcvOption");
         }
         else
         {
            CMSMEM_FREE_BUF_AND_NULL_PTR(dhcp6cRcvObj->address);
            dhcp6cRcvObj->addressPlt = 0;
            dhcp6cRcvObj->addressVlt = 0;
            CMSMEM_FREE_BUF_AND_NULL_PTR(dhcp6cRcvObj->prefix);
            dhcp6cRcvObj->prefixPlt = 0;
            dhcp6cRcvObj->prefixVlt = 0;
            CMSMEM_FREE_BUF_AND_NULL_PTR(dhcp6cRcvObj->prefixOld);
            dhcp6cRcvObj->prefixVltOld = 0;
            CMSMEM_FREE_BUF_AND_NULL_PTR(dhcp6cRcvObj->DNSServers);
            CMSMEM_FREE_BUF_AND_NULL_PTR(dhcp6cRcvObj->domainName);
            CMSMEM_FREE_BUF_AND_NULL_PTR(dhcp6cRcvObj->aftr);
   
            if ((ret = cmsObj_set(dhcp6cRcvObj, &dhcp6cRcvIidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("set of dhcp6cRcvObj failed, ret=%d", ret);
            }

            cmsObj_free((void **)&dhcp6cRcvObj);
         }
      }
      else
      {
         cmsLog_debug("no dhcp6c associated");
      }
   }

   if (isWan)
   {
      /* only WAN side interfaces affect DNS.  LAN side does not affect it */
      rutDns_deactivateIpv6DnsServers_dev2(ipIntfFullPath);

      if (qdmMulti_isMldProxyEnabledOnIntfNameLocked_dev2(ifname) ||
          qdmMulti_isMldSourceEnabledOnIntfNameLocked_dev2(ifname))
      {
         rutIpt_mldRules(FALSE, ifname);
         rutMulti_updateMldProxyIntfList();
      }

      rutTunnel_dsliteControl(ipIntfFullPath);
#ifdef SUPPORT_MAP
      rutMap_mapControl(ipIntfFullPath);
#endif
      cmsLed_setWanDisconnected();
   }

#ifdef SUPPORT_RIP
{
   char ifName[BUFLEN_32] = {0};
   
   /* reevaluate rip process attachment to reflect the current modem state */
   if (rut_isRipEnabled_dev2() && 
      !rutWan_findFirstIpvxRoutedAndConnected_dev2(CMS_AF_SELECT_IPVX, ifName))
   {
      rutRip_stop();
   }
}
#endif


   cmsLog_notice("Deactivated all IPv6 services associated with %s.", ifname);
   if (isWan)
   {
      rut_sendEventMsgToSmd(CMS_MSG_WAN_CONNECTION_DOWN, 0, ifname, strlen(ifname)+1);
   }

   cmsLog_debug("Exit");
}



/* Need to undo the stuff that we did when called rutIpv6Service_start.
 * In other words, stop Group 1 services but don't need to stop Group 2
 * services because we never started them or already stopped by rutIpv6Service_teardown.
 */
void rutIpv6Service_stop(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan)
{
   SINT32 pid=CMS_INVALID_PID;
   UBOOL8 iana;
   UBOOL8 iapd;
   UBOOL8 um;
   CmsRet ret;

   cmsLog_debug("fullpath=%s ifname=%s isWan=%d", ipIntfFullPath, ifname, isWan);

   /*
    * If there is a dhcpv6 client pointing to this IP.Interface, stop it
    */
   if (rutDhcpv6_isClientEnabled_dev2(ipIntfFullPath, &iana, &iapd, &um, &pid))
   {
      rutWan_stopDhcp6c(ifname, pid);
      rutDhcpv6_setClientPidAndStatusByIpIntfFullPath_dev2(ipIntfFullPath,
                                           CMS_INVALID_PID, MDMVS_DISABLED);
   }
   
   if (!isWan)
   {
      return;
   }

   rutIpt_removeInterfaceIptableRules(ifname, FALSE);

#if 0
   /* Stop layer 2 and 3 interface */
   
   /* cmsLog_debug("l2IfName=%s, baseL3IfName=%s ", l2IfName, baseL3IfName);
     if (rutWl2_isWanLayer2DSL(wanConnOid, iidStack)) 
      if (cmsUtl_strcmp(l2IfName, baseL3IfName)) 
      {
         snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s down", baseL3IfName);
         rut_doSystemAction("rutWan_stopL3Interface: ifconfig L3IfName down", (cmdStr));      
      }         
   */
   
   /* Need to remove the network from the routing table by
   * doing  "ifconfig L3IfName 0 0.0.0.0"
   */
#endif

   return;
}


/* Stop everything.  Link is down or we were disabled or deleted.
 * Do everything we do in rutIpv6Service_start and rutIpv6Service_setup.
 * see rutCfg_stopWanIpConnection or rutCfg_stopWanPppConnection.
 */
void rutIpv6Service_down(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan)
{
   cmsLog_debug("ipIntf=%s ifname=%s isWan=%d",
                ipIntfFullPath, ifname, isWan);

   rutIpv6Service_teardown(ipIntfFullPath, ifname, isWan);   
   rutIpv6Service_stop(ipIntfFullPath, ifname, isWan);      

   cmsLog_debug("Exit");
}




CmsRet rutIpv6Service_launchDhcp6c(const char *ipIntfFullPath, const char *ifname)
{
   CmsRet ret = CMSRET_SUCCESS;
   SINT32 pid=CMS_INVALID_PID;
   UBOOL8 iana;
   UBOOL8 iapd;
   UBOOL8 um;

   cmsLog_debug("ipIntfFullPath=%s ifname=%s", ipIntfFullPath, ifname);

   /*
    * If there is a dhcpv6 client pointing to this IP.Interface
    * start dhcpv6 client.  Note that we could have a dhcp client on the
    * LAN side.
    */
   if (rutDhcpv6_isClientEnabled_dev2(ipIntfFullPath, &iana, &iapd, &um, &pid))
   {
      UBOOL8 dynamicTunnel=FALSE;
      UBOOL8 requestAddress = FALSE;
      UBOOL8 mapt=FALSE;
      UBOOL8 mape=FALSE;
      UBOOL8 mflag;
#ifdef SUPPORT_MAP
      UBOOL8 mapDynamic;
      char mechanism[BUFLEN_24];
#endif
      char ifnameInfo[CMS_IFNAME_LENGTH];

      cmsLog_debug("launching dhcp6c");
      
      dynamicTunnel = rutTunnel_isDynamicTunnel(ipIntfFullPath, FALSE);
#ifdef SUPPORT_MAP
      mapDynamic = rutTunnel_isMapDynamic(ipIntfFullPath, mechanism);

      if (mapDynamic)
      {
         if (!cmsUtl_strcmp(mechanism, MDMVS_TRANSLATION))
         {
            mapt = mapDynamic;
         }
         else if (!cmsUtl_strcmp(mechanism, MDMVS_ENCAPSULATION))
         {
            mape = mapDynamic;
         }
         else
         {
            cmsLog_error("Unrecognized mechanism %s", mechanism);
            return CMSRET_INVALID_PARAM_VALUE;
         }
      }
#endif

//      if (!newObj->X_BROADCOM_COM_UnnumberedModel)
//      {
//         requestAddress = (newObj->X_BROADCOM_COM_Dhcp6cForAddress || newObj->X_BROADCOM_COM_MFlag);
//      }

      mflag = getMflagFromIpIntfFullPath(ipIntfFullPath);

      /* FIXME IPv6: check tunnel */
      if (!um)
      {
         requestAddress = (iana || mflag);
      }

      if (pid != CMS_INVALID_PID)
      {
         rutWan_stopDhcp6c(ifname, pid);
      }

      if (cmsUtl_strstr(ifname, "ppp"))
      {
         char l2IfName[CMS_IFNAME_LENGTH]={0};

         if ((ret = qdmIpIntf_getLayer2IntfNameByLayer3IntfNameLocked(ifname, l2IfName)) == CMSRET_SUCCESS)
         {
            snprintf(ifnameInfo, CMS_IFNAME_LENGTH, "%s__%s", ifname, l2IfName);
         }
         else
         {
            cmsLog_error("cannot get l2ifname from l3ifname %s (ret=%d)",
                         ifname, ret);
            return ret;
         }
      }
      else
      {
         cmsUtl_strncpy(ifnameInfo, ifname, CMS_IFNAME_LENGTH);
      }

      if ((pid = rutWan_restartDhcp6c(ifnameInfo, 
                                      requestAddress, 
                                      iapd,
                                      dynamicTunnel,
                                      mapt,
                                      mape)) == CMS_INVALID_PID)
      {
         cmsLog_error("rutWan_restartDhcp6c returns error.");
         rutDhcpv6_setClientPidAndStatusByIpIntfFullPath_dev2(ipIntfFullPath,
                                                           pid, MDMVS_ERROR);

         return CMSRET_INTERNAL_ERROR;
      }
      else
      {
         rutDhcpv6_setClientPidAndStatusByIpIntfFullPath_dev2(ipIntfFullPath,
                                                         pid, MDMVS_ENABLED);
      }
   }
   else
   {
      cmsLog_debug("No dhcpc needed on %s", ifname);
   }
   
   return ret;
}


UBOOL8 getMflagFromIpIntfFullPath(const char *ipIntfFullPath)
{
   MdmPathDescriptor pathDesc;
   UBOOL8 mflag=FALSE;
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   ret = cmsMdm_fullPathToPathDescriptor(ipIntfFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d",
                   ipIntfFullPath, ret);
      return mflag;
   }

   if(cmsObj_get(pathDesc.oid, &pathDesc.iidStack, OGF_NO_VALUE_UPDATE,
                           (void *)&ipIntfObj)==CMSRET_SUCCESS)
   {
      mflag = ipIntfObj->X_BROADCOM_COM_Mflag_Upstream;
      cmsObj_free((void **)&ipIntfObj);
   }

   return mflag;
}

#endif  /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */

