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

#ifdef DMP_DEVICE2_IPINTERFACE_1

#include "mdm.h"
#include "cms_core.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_strconv2.h"
#include "rut_util.h"
#include "rut2_ip.h"
#include "rut_iptables.h"
#include "rut_qos.h"
#include "rut_wan.h"
#include "rut_lan.h"
#include "rut_upnp.h"
#include "rut_rip.h"
#include "rut2_dhcpv4.h"
#include "rut2_dns.h"
#include "rut2_route.h"
#include "rut_ipsec.h"
#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1
#include "rut_openvswitch.h"
#endif



/*!\file rut2_ipservicecfg.c
 * \brief This file contains functions for the IP.Interface IPv4 service
 * state machine.  This is the TR181 version of rut_ipconcfg.c and
 * rut_pppconcfg.c
 *
 */

void rutIpv4Service_runStateMachine(const char *newStatus,
         const char *currStatus,
         const char *ipIntfFullPath, const char *ifname,
         UBOOL8 isWan, UBOOL8 isBridge)
{

   cmsLog_notice("%s >>> %s ifname=%s isWan=%d isBridge=%d",
                currStatus, newStatus, ifname, isWan, isBridge);

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
         rutIpv4Service_start(ipIntfFullPath, ifname, isWan, isBridge);
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
         rutIpv4Service_teardown(ipIntfFullPath, ifname, isWan, isBridge);
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
         rutIpv4Service_setup(ipIntfFullPath, ifname, isWan, isBridge);
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
         rutIpv4Service_down(ipIntfFullPath, ifname, isWan, isBridge);
      }
      else if (!cmsUtl_strcmp(currStatus, MDMVS_SERVICESTARTING))
      {
         /*
          * STARTING => DOWN
          * we were not fully up yet.  but we do need to kill dhcp client
          * if there was one running (also remove firewall rules and
          * unload modules?  basically, undo group 1 services).
          */
         rutIpv4Service_stop(ipIntfFullPath, ifname, isWan, isBridge);
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
void rutIpv4Service_start(const char *ipIntfFullPath, const char *ifname,
                          UBOOL8 isWan, UBOOL8 isBridge)
{

  cmsLog_notice("==Enter: ipIntfFullPath=%s ifname=%s isWan=%d isBridge=%d",
               ipIntfFullPath, ifname, isWan, isBridge);


   /*
    * If there is a dhcpv4 client pointing to this IP.Interface
    * start dhcpv4 client.  Note that we could have a dhcp client on the
    * LAN side.
    */
   if (rutDhcpv4_isClientEnabled_dev2(ipIntfFullPath))
   {
      SINT32 pid=CMS_INVALID_PID;
      char intfNameBuf[CMS_IFNAME_LENGTH]={0};
      char vid[BUFLEN_256], uid[BUFLEN_256];
      char iaid[BUFLEN_16], duid[BUFLEN_256];
      char ipAddr[BUFLEN_16], leasedTime[BUFLEN_16];
      char buf[BUFLEN_16], serverIpAddr[BUFLEN_16];
#ifdef SUPPORT_HOMEPLUG
      UBOOL8 op125=TRUE;
#else
      UBOOL8 op125=FALSE; 
#endif
      UBOOL8 op212=FALSE;

      memset(vid, 0, BUFLEN_256);
      memset(duid, 0, BUFLEN_256);
      memset(iaid, 0, BUFLEN_16);
      memset(uid, 0, BUFLEN_256);
      memset(buf, 0, BUFLEN_16);
      memset(ipAddr, 0, BUFLEN_16);
      memset(leasedTime, 0, BUFLEN_16);
      memset(serverIpAddr, 0, BUFLEN_16);

      qdmDhcpv4Client_getSentOption_dev2(ipIntfFullPath, 60, vid, BUFLEN_256);
      qdmDhcpv4Client_getSentOption_dev2(ipIntfFullPath, 61, duid, BUFLEN_256);
      qdmDhcpv4Client_getSentOption_dev2(ipIntfFullPath, 61, iaid, BUFLEN_8);
      qdmDhcpv4Client_getSentOption_dev2(ipIntfFullPath, 77, uid, BUFLEN_256);
#ifndef SUPPORT_HOMEPLUG
      if (qdmDhcpv4Client_getSentOption_dev2(ipIntfFullPath, 125, buf, BUFLEN_16) == CMSRET_SUCCESS)
         op125 = atoi(buf);
#endif

      qdmDhcpv4Client_getReqOption_dev2(ipIntfFullPath, 50, ipAddr, BUFLEN_16);
      qdmDhcpv4Client_getReqOption_dev2(ipIntfFullPath, 51, leasedTime, BUFLEN_16);
      qdmDhcpv4Client_getReqOption_dev2(ipIntfFullPath, 54, serverIpAddr, BUFLEN_16);

      if ((pid = rutWan_startDhcpc(ifname, 
                                   vid,
                                   duid,
                                   iaid,
                                   uid,
                                   op125,
                                   ipAddr,
                                   serverIpAddr,
                                   leasedTime,
                                   op212)) == CMS_INVALID_PID)
      {
         rutDhcpv4_setClientPidAndStatusByIpIntfFullPath_dev2(ipIntfFullPath,
                                                           pid, MDMVS_ERROR);
         return;
      }
      else
      {
         rutDhcpv4_setClientPidAndStatusByIpIntfFullPath_dev2(ipIntfFullPath,
                                                         pid, MDMVS_ENABLED);
      }
   }
   else
   {
      cmsLog_debug("No dhcpc needed on %s", ifname);
   }

   /* XXX TODO: Lots of vlan configuration in this step */


   /* Only if it is a routed WAN ip interface before
   * doing NAT/Firewall actions here.
   */
   if (!isBridge)
   {
      UBOOL8 isNatEnabled;
      UBOOL8 isFirewallEnabled;

      isNatEnabled = qdmIpIntf_isNatEnabledOnIpIntfFullPathLocked(ipIntfFullPath);
      isFirewallEnabled = qdmIpIntf_isFirewallEnabledOnIpIntfFullPathLocked_dev2(ipIntfFullPath);

      if (isWan &&( isNatEnabled || isFirewallEnabled))
      {
         UBOOL8 isFullCone=FALSE;
         isFullCone = qdmIpIntf_isFullConeNatEnabledOnIpIntfFullPathLocked_dev2(ipIntfFullPath);

         rutIpt_initNatAndFirewallOnWanConnection(ifname, isFullCone,
                         isNatEnabled, isFirewallEnabled);
      }
      else if(!isWan) //For LAN side firewall
      {
          if(isFirewallEnabled)
          {
              rutIpt_initNatAndFirewallOnWanConnection(ifname, FALSE,
                         isNatEnabled, isFirewallEnabled);
          }else
          {

            /* For now, HOMPLUG  has no iptable modules compiled in since  
            * it does not use any LAN side firewall. Jst skip the following calls
            * so that the console will not complains a bunch of iptable models not found messages.
            */
#ifndef DMP_DEVICE2_HOMEPLUG_1          
              /* This's a LAN interface with Firewall disable. Need setup outgoing rule on it */
              rutIpt_insertIpModules();
              rutIpt_initFirewallExceptions(ifname);
#endif        

          }
      }
   }

   cmsLog_debug("Exit");

   return;
}



/* this is the TR181 version of rutCfg_setupWanIpConnection or
 * rutCfg_setupWanPPPConnection.  All the services that must be run after
 * IP address is configured and does not affect flowcache flows.
 *
 */
void rutIpv4Service_setup(const char *ipIntfFullPath, const char *ifname,
                          UBOOL8 isWan, UBOOL8 isBridge)
{
   UBOOL8 isFirewallEnabled=FALSE;
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_notice("====Enter: ifname=%s isWan=%d", ifname, isWan);


#ifdef DMP_DEVICE2_QOS_1
   /*
    * QoS classification config requires the egress L3 intf to be UP, so we
    * have to call here in _setup instead of _start.  But we configure
    * before the IP address is configured so flowcache will still learn
    * the right markings.
    */
   rutQos_reconfigAllClassifications_dev2(NULL);
#endif

   /* I guess for AntiSpoofing, we need the ifname to be somewhat more "up"
    * so we do it here instead of in _start.
    */
   isFirewallEnabled = qdmIpIntf_isFirewallEnabledOnIpIntfFullPathLocked_dev2(ipIntfFullPath);
   if (isFirewallEnabled)
   {
      rutIpt_activateFirewallAntiIPspoofing(ifname);
   }


   /* ======== configure IPv4 (WAN and LAN) Addr now ================= */
   if ((ret = rutIp_activateIpv4Interface_dev2(ifname)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutIp_activateInterface_dev2 failed on %s. ret %d", ifname, ret);
   }

   /*
    * Everything done after this point should not affect the packet flow or
    * packet markings.
    */

   /* rutTunnel_control(newObj->X_BROADCOM_COM_IfName, MDMVS_IPV6INIPV4, TRUE); */

   /* RIP */


#ifdef DMP_DEVICE2_ROUTING_1
   /*
    * Update Routing and Default Gateway for WAN and LAN because of
    * possible dhcp client on LAN
    */
   if (!isBridge)
   {
      if ((ret = rutRt_activateIpv4Routing_dev2(ipIntfFullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutRt_activateIpv4Routing_dev2 failed on %s. ret %d",
                      ipIntfFullPath, ret);
      }
	  
#ifdef SUPPORT_RIP
      /* reevaluate rip process attachment to reflect the current modem state */
      if (rut_isRipEnabled_dev2() && !rut_isApplicationRunning(EID_RIPD))
      {
         rutRip_restart_dev2(NULL, NULL);
      }
#endif	  
   }
#endif

#ifdef DMP_DEVICE2_BRIDGE_1  /* aka SUPPORT_PORT_MAP */
      rutPMap_configPortMapping_dev2(TRUE, ifname, ipIntfFullPath);
#endif
   
#ifdef SUPPORT_UPNP
   /* UPnP depends on default route info, so do it after routing */
   if (isWan && !isBridge)
   {
      /* this logic is copied from TR98 code, so once we start UPnP, do not
       * restart it when the second, third, fourth WAN connection comes up?
       */
      if (rut_isUpnpEnabled() && rutUpnp_checkRunStatusWithDelay() == FALSE)
      {
         rut_restartUpnp(NULL);
      }
   }
#endif

   if (!isBridge)
   {
      /* Update DNS for WAN and LAN because of possible dhcp client on LAN */
      ret = rutDns_activateIpv4DnsServers_dev2(ipIntfFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("rutDns_activateIpv4DnsServers_dev2 failed on %s. ret %d",
               ipIntfFullPath, ret);
      }
   }

   if (isWan)
   {
      /* multicast related stuff (WAN side only) */
      if ( isBridge )
      {
         rutMulti_updateIgmpMldProxyIntfList();
      }
      else
      {
         if (qdmMulti_isIgmpProxyEnabledOnIntfNameLocked(ifname) || 
             qdmMulti_isIgmpSourceEnabledOnIntfNameLocked(ifname))
         {
            rutIpt_igmpRules(TRUE, ifname);
            rutMulti_updateIgmpProxyIntfList();
         }

#if defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
         rutTunnel_ipv6rdControl(ipIntfFullPath);
#endif

#ifdef SUPPORT_IPSEC
         rutIPSec_activateTunnel();
#endif
      }
      cmsLed_setWanConnected();
   }


#ifdef DMP_DEVICE2_HOMEPLUG_1   
   /* homeplug devices do not have "WAN side" */
#else
   /* only print this if the IP.Interface is on the WAN side */
   if (isWan)
#endif
   {
      printf("All services associated with %s is activated.\n", ifname);
      rut_sendEventMsgToSmd(CMS_MSG_WAN_CONNECTION_UP, 0, ifname, strlen(ifname)+1);
   }
#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1
   if( (isWan) && (isBridge))
   { 
      if( (rutOpenVS_isEnabled()) && (rutOpenVS_isOpenVSPorts(ifname)))
      {
         rutOpenVS_startupOpenVSport(ifname);
      }
   }
#endif
}


/* This is the TR181 version of rutCfg_tearDownWanIpConnection or rutCfg_tearDownWanPppConnection.
 * We lost IP address (lease expired), but LowerLayer link is still up.
 * Stop or restart all high level IP services, but keep dhcp client running.
 */
void rutIpv4Service_teardown(const char *ipIntfFullPath, const char *ifname,
                             UBOOL8 isWan, UBOOL8 isBridge)
{
   UBOOL8 isStaticIpIntf=FALSE;
   UBOOL8 isIgmpEnabled=FALSE;

   cmsLog_notice("==Enter: fullpath=%s ifname=%s isWan=%d", ipIntfFullPath, ifname, isWan);


   /* Deactivate the tunnels associated with this IP connection */
   //todo: rutTunnel_control(ifname, MDMVS_IPV6INIPV4, FALSE);
   /* todo: reevaluate upnp process attachment to reflect the current modem state */

   /*
    * Override the default MDM behavior of hiding objects which are pending
    * delete by setting hideObjectsPendingDelete to FALSE.  After we are
    * done with the QDM, restore original value (which should be TRUE).
    */
   {
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;

#ifdef DMP_DEVICE2_QOS_1
      /*
       * Since we configured QoS classifications when we acquired IP Addr
       * in _setup, we now need to unconfig QoS classifications when we lose
       * IP Addr.
       */
      rutQos_reconfigAllClassifications_dev2(ifname);
#endif

      if (isWan)
      {
         isStaticIpIntf = qdmIpIntf_isStaticWanLocked_dev2(ifname);
         isIgmpEnabled = qdmMulti_isIgmpProxyEnabledOnIntfNameLocked(ifname) | 
                         qdmMulti_isIgmpSourceEnabledOnIntfNameLocked(ifname);
      }

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
   }


   if (isStaticIpIntf)
   {
      /* for static IP interface, need to delete next hop -- eg. 
       * route del -net 10.6.33.128 netmask 255.255.255.192 gw 10.6.33.129
       */
      char ipAddress[CMS_IPADDR_LENGTH]={0};
      char subnetMask[CMS_IPADDR_LENGTH]={0};
      char cmdStr[BUFLEN_128 + CMS_IPADDR_LENGTH]={0};
      char bcastStr[CMS_IPADDR_LENGTH]={0};
      char subnetStr[CMS_IPADDR_LENGTH]={0};
      char gwIpAddr[CMS_IPADDR_LENGTH]={0};
      
      if (qdmIpIntf_getIpv4AddrAndSubnetByNameLocked_dev2(ifname,
                                                          ipAddress,
                                                          subnetMask) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmIpIntf_getIpv4AddrAndSubnetByNameLocked_dev2 failed on %s",
                      ifname);
         return;
      }
      if (rutWan_getBcastStrAndSubnetFromIpAndMask(ipAddress,
                                                   subnetMask,
                                                   bcastStr,
                                                   subnetStr) != CMSRET_SUCCESS)
      {
          cmsLog_error("rutWan_getBcastStrAndSubnetFromIpAndMask failed on %s %s",
                       ipAddress, subnetMask);
          return;
      }
      /* Need to get this ip interface gateway in router.forwarding */
      if (qdmRt_getGatewayIpv4AddrByFullPathLocked_dev2(ipIntfFullPath,
                                                  gwIpAddr) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmRt_getGatewayIpv4AddrByFullPathLocked_dev2 failed on %s",
                       ipIntfFullPath);
         return;
      }
      
      snprintf(cmdStr, sizeof(cmdStr), "route del -net %s netmask %s gw %s 2>/dev/null",
               subnetStr, subnetMask, gwIpAddr);
      rut_doSystemAction("rcl", cmdStr);
   }

   if (isWan)
   {
      if ( isBridge )
      {
         rutMulti_updateIgmpMldProxyIntfList();
      }
      else
      {
         /* multicast related stuff (WAN side only) */
         if (isIgmpEnabled)
         {
            rutIpt_igmpRules(FALSE, ifname);
            rutMulti_updateIgmpProxyIntfList();
         }

#if defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
         rutTunnel_ipv6rdControl(ipIntfFullPath);
#endif
      }
      cmsLed_setWanDisconnected();
   }

#ifdef DMP_DEVICE2_BRIDGE_1  /* aka SUPPORT_PORT_MAP */
      rutPMap_configPortMapping_dev2(FALSE, ifname, ipIntfFullPath);
#endif

#ifdef SUPPORT_UPNP
   /* reevaluate upnp process attachment to reflect the current modem state */
   if (isWan && !isBridge && rut_isUpnpEnabled())
   {
      rut_restartUpnp((char *) ifname);
   }
#endif


   /*
    * Update DNS and routing for WAN and LAN (bcause of possible
    * dhcp client on LAN side).
    */
   if (!isBridge)
   {
       rutDns_deactivateIpv4DnsServers_dev2(ipIntfFullPath);

#ifdef DMP_DEVICE2_ROUTING_1
      rutRt_deactivateIpv4Routing_dev2(ipIntfFullPath);

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
#endif
      //XXX TODO:  rut_stopRipd(currObj, isIPv4);
   }


   /* deactivate IP address for LAN and WAN, static or not */
   rutIp_deactivateIpv4Interface_dev2(ifname);


   cmsLog_notice("Deactivated all services associated with %s.", ifname);
#ifdef DMP_DEVICE2_HOMEPLUG_1   
   /* homeplug devices do not have "WAN side" */
#else
   if (isWan)
#endif
   {
      rut_sendEventMsgToSmd(CMS_MSG_WAN_CONNECTION_DOWN, 0, ifname, strlen(ifname)+1);
   }

   return;
}



/* Need to undo the stuff that we did when called rutIpv4Service_start.
 * In other words, stop Group 1 services but don't need to stop Group 2
 * services because we never started them or already stopped by rutIpv4Service_teardown.
 */
void rutIpv4Service_stop(const char *ipIntfFullPath, const char *ifname,
                         UBOOL8 isWan, UBOOL8 isBridge __attribute__((unused)))
{
   UBOOL8 isNatEnabled;
   UBOOL8 isFirewallEnabled;
   UBOOL8 prevHideObjectsPendingDelete;

   cmsLog_notice("==Enter: fullpath=%s ifname=%s isWan=%d", ipIntfFullPath, ifname, isWan);


   /* If there is a dhcpv4 client pointing to this IP.Interface
    * stop dhcpv4 client.  Note this could be WAN or LAN.
    */
   if (rutDhcpv4_isClientEnabled_dev2(ipIntfFullPath))
   {
      rutDhcpv4_stopClientByIpIntfFullPath_dev2(ipIntfFullPath);
      rutDhcpv4_setClientPidAndStatusByIpIntfFullPath_dev2(ipIntfFullPath,
                                           CMS_INVALID_PID, MDMVS_DISABLED);
   }


   /* XXX TODO: Lots of vlan configuration in this step */


   /*
    * Override the default MDM behavior of hiding objects which are pending
    * delete by setting hideObjectsPendingDelete to FALSE.  After we are
    * done with the QDM, restore original value (which should be TRUE).
    */
   prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
   mdmLibCtx.hideObjectsPendingDelete = FALSE;

   isNatEnabled = qdmIpIntf_isNatEnabledOnIpIntfFullPathLocked(ipIntfFullPath);
   isFirewallEnabled = qdmIpIntf_isFirewallEnabledOnIpIntfFullPathLocked_dev2(ipIntfFullPath);

   mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

   if ( isWan || !isFirewallEnabled )       /* Wan interface, or LAN interface with Firewall disable. Need remove outgoing */
   {   
      UBOOL8 isIPv4=TRUE;

      cmsLog_debug("Removing iptables rules for %s", ifname);
      rutIpt_removeInterfaceIptableRules(ifname, isIPv4);
   }

   /*
   * XXX TODO: Tunnel: rutTunnel_control(currObj->X_BROADCOM_COM_IfName, MDMVS_IPV6INIPV4, FALSE);
   */

   cmsLog_debug("==Exit");
   
   return;

}


/* Stop everything.  Link is down or we were disabled or deleted.
 * Do everything we do in rutIpv4Service_start and rutIpv4Service_setup.
 * see rutCfg_stopWanIpConnection or rutCfg_stopWanPppConnection.
 */
void rutIpv4Service_down(const char *ipIntfFullPath, const char *ifname,
                         UBOOL8 isWan, UBOOL8 isBridge)
{
   cmsLog_debug("==Enter: ipIntf=%s ifname=%s isWan=%d",
                ipIntfFullPath, ifname, isWan);

#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1
   if(( isWan) && (isBridge))
   {
      if( (rutOpenVS_isEnabled()) && (rutOpenVS_isOpenVSPorts(ifname)))
         rutOpenVS_shutdownOpenVSport(ifname);
   }
#endif

   rutIpv4Service_teardown(ipIntfFullPath, ifname, isWan, isBridge);
   rutIpv4Service_stop(ipIntfFullPath, ifname, isWan, isBridge);

   cmsLog_debug("==Exit");
}

#endif  /* DMP_DEVICE2_IPINTERFACE_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */
