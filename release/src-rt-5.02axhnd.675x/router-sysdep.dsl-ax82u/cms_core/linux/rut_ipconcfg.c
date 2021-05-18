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
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_atm.h"
#include "rut_util.h"
#include "rut_iptables.h"
#include "rut_wan.h"
#include "rut_pmap.h"
#include "rut_upnp.h"
#include "rut_ethswitch.h"
#include "rut_lan.h"
#include "rut_rip.h"
#include "rut_network.h"
#include "rut_ipconcfg.h"
#include "rut_route.h"
#include "rut_xtmlinkcfg.h"
#include "rut_qos.h"
#include "rut_dsl.h"
#include "rut_ipsec.h"
#include "rut_wanlayer2.h"
#include "rut_multicast.h"
#include "rut_iptunnel.h"
#include "rut_ebtables.h"
#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
#include "rut_l2tpac.h"
#endif

#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1
#include "rut_openvswitch.h"
#endif

#ifdef DMP_BASELINE_1
static CmsRet removeInterfaceFromBridge(char *ifName);
#endif


CmsRet rutCfg_launchDhcpv4Client(WanIpConnObject *ipObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   SINT32 pid;
   UBOOL8 dyn6rd;
   char leasedTime[BUFLEN_16];

   if (!cmsUtl_strcmp(ipObj->addressingType, MDMVS_STATIC)) 
   {
      return ret;
   }

   if (ipObj->X_BROADCOM_COM_DhcpcPid != CMS_INVALID_PID)
   {
      UINT32 specificEid = MAKE_SPECIFIC_EID(ipObj->X_BROADCOM_COM_DhcpcPid, EID_DHCPC);

      cmsLog_debug("stop original dhcpc");
      rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0);
   }

   dyn6rd = rutTunnel_containDynamicTunnel(ipObj->X_BROADCOM_COM_IfName, TRUE);

   memset(leasedTime, 0, BUFLEN_16);
   snprintf(leasedTime, BUFLEN_16, "%d", ipObj->X_BROADCOM_COM_Op51LeasedTime);

   if ((pid = rutWan_startDhcpc(ipObj->X_BROADCOM_COM_IfName, 
                                ipObj->X_BROADCOM_COM_Op60VenderID,
                                ipObj->X_BROADCOM_COM_Op61DUID,
                                ipObj->X_BROADCOM_COM_Op61IAID,
                                ipObj->X_BROADCOM_COM_Op77UserID,
                                ipObj->X_BROADCOM_COM_Op125Enabled,
                                ipObj->X_BROADCOM_COM_Op50IpAddress,
                                ipObj->X_BROADCOM_COM_Op54ServerIpAddress,
                                leasedTime,
                                dyn6rd)) == CMS_INVALID_PID)
   {
      ret = CMSRET_INTERNAL_ERROR;
   }

   ipObj->X_BROADCOM_COM_DhcpcPid = pid;

   return ret;
}


#ifdef DMP_BASELINE_1

CmsRet rutCfg_startWanIpConnection(const InstanceIdStack *iidStack,
                                   _WanIpConnObject *newObj)
{
   CmsRet ret=CMSRET_SUCCESS;
   
   cmsLog_debug("Enter.");

   if (!rutWl2_isIPoA(iidStack))
   {
      /* For bridge and IPoE connection */
      UBOOL8 isBridge = !cmsUtl_strcmp(newObj->connectionType, MDMVS_IP_BRIDGED);
      ConnectionModeType connMode;
      char l2IfName[CMS_IFNAME_LENGTH]={0};
      char baseL3IfName[CMS_IFNAME_LENGTH]={0};

      /* Need layer 2 and baseL3IfName and connMode to start a layer 3 interface */
      if ((ret = rutWan_getL3InterfaceInfo(MDMOID_WAN_IP_CONN, 
                                           iidStack,
                                           newObj->X_BROADCOM_COM_VlanMuxID,
                                           newObj->X_BROADCOM_COM_ConnectionId,
                                           newObj->X_BROADCOM_COM_IfName,
                                           l2IfName,
                                           baseL3IfName,
                                           &connMode)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutWan_getL3InterfaceInfo failed. ret=%d", ret);
         return ret;
      }
      
      /* start the layer 3 interface only needed for PPPoE */
      if ((ret = rutWan_startL3Interface(
                                         newObj->X_BROADCOM_COM_VlanMuxID,
                                         newObj->X_BROADCOM_COM_VlanMux8021p,
                                         newObj->X_BROADCOM_COM_VlanTpid,
                                         l2IfName,
                                         baseL3IfName,
                                         connMode,
                                         isBridge)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutWan_startL3Interface failed. error=%d", ret);
         return ret;
      }
      else
      {
         cmsLog_debug("rutWan_startL3Interface ok");

         if (isBridge)
         {
            cmsLog_debug("Config Bridge.");
         }
         else if (!cmsUtl_strcmp(newObj->addressingType, MDMVS_STATIC)) 
         {
            cmsLog_debug("Config Static IPoE.");
         }
         else if (newObj->X_BROADCOM_COM_IPv4Enabled)
         {
            cmsLog_debug("Config Dynamic IPoE.");

            /*
             * The externalIPAddress may have been saved to the config flash.
             * We have to write out the externalIPAddress because it could be
             * a static IPoE connection.  But now that we are starting a dynamic 
             * IPoE connection, blank out the externalIPAddress and other associated
             * fields until we get that info back from DHCP server.
             */
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->externalIPAddress, "0.0.0.0", mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->subnetMask, "0.0.0.0", mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->defaultGateway, "0.0.0.0", mdmLibCtx.allocFlags);
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->DNSServers, "", mdmLibCtx.allocFlags);

            /*
            * When dynamic IPoE is first configured, we don't have an external IP address yet.
            * Just start dhcpc and return.  When dhcpc gets an IP address, it will send out
            * an event msg to ssk, which will do a cmsObj_set on
            * this object, which will then cause us to call rut_activateIpEthInterface with the 
            * newly assigned externalIPAddress.
            */
            ret = rutCfg_launchDhcpv4Client(newObj);
         }
      } 
   }
   else
   {
      /* IPoA */
      if ((ret = rutWan_startIPoA(iidStack, newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Fail to start IPoA connection.");
      }
   }
   
   cmsLog_debug("Exit. ret=%d", ret);
   
   return ret;
   
}


CmsRet rutCfg_setupWanIpConnection(const InstanceIdStack *iidStack,
                                   _WanIpConnObject *newObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 isRouted = FALSE;

   cmsLog_debug("Enter.");
   
   if (!cmsUtl_strcmp(newObj->connectionType, MDMVS_IP_BRIDGED))
   {
      UBOOL8 isWanIntf=TRUE;

      /* It's a bridge and do some configuration on it. */
      
      /* setup Qos queues associated with this interface */
      if ((ret = rutQos_qMgmtQueueReconfig(newObj->X_BROADCOM_COM_IfName, FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtQueueReconfig returns error. ret=%d", ret);
         return ret;
      }
      rutQos_doDefaultPolicy();

      /* Reconfig QoS port shaping for all Ethernet interfaces */
      rutQos_portShapingConfigAll();

#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1
      if( (rutOpenVS_isEnabled()) && (rutOpenVS_isOpenVSPorts(newObj->X_BROADCOM_COM_IfName)))
      {
         rutOpenVS_startupOpenVSport(newObj->X_BROADCOM_COM_IfName);
         return CMSRET_SUCCESS;
      }
      else
      {
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */      
         char bridgeIfName[CMS_IFNAME_LENGTH]; 

         if ((ret = rutPMap_getBridgeIfNameFromIfName(newObj->X_BROADCOM_COM_IfName, 
                                                      bridgeIfName, TRUE)) == CMSRET_SUCCESS)
         {
            cmsLog_debug("Just the action to add wan intf to the interface group (%).", bridgeIfName);
            rutLan_addInterfaceToBridge(newObj->X_BROADCOM_COM_IfName, isWanIntf, bridgeIfName);
         }
#else
         rutLan_addInterfaceToBridge(newObj->X_BROADCOM_COM_IfName, isWanIntf, "br0");
#endif 
      }   		
#else
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */      
      char bridgeIfName[CMS_IFNAME_LENGTH]; 
  
      /* the filter object is already in mdm, so just the action
      * ie. add the wan interface to the interface group with correct bridge ifName
      */            
      if ((ret = rutPMap_getBridgeIfNameFromIfName(newObj->X_BROADCOM_COM_IfName, 
                                                   bridgeIfName, TRUE)) == CMSRET_SUCCESS)
      {
         cmsLog_debug("Just the action to add wan intf to the interface group (%).", bridgeIfName);
         rutLan_addInterfaceToBridge(newObj->X_BROADCOM_COM_IfName, isWanIntf, bridgeIfName);
      }
  #else
      /* 
       * Unlike the briding case, we have to add the WAN bridge intf to
       * the LAN bridge at both bootup and runtime.  At bootup, this is 
       * the only thing in the configuration that tells the system the
       * WAN bridge should be configured.
       */
      rutLan_addInterfaceToBridge(newObj->X_BROADCOM_COM_IfName, isWanIntf, "br0");

#endif 
#endif
      /* Avoid DHCP messages being accelerated */
      rutEbt_avoidDhcpAcceleration();
   }
   else
   {
      /* IPoE or IPoA connection */
      isRouted = TRUE;
      
      /* call common function to do NAT related stuff before activating the interface. */
      rutIpt_initNatAndFirewallOnWanConnection(newObj->X_BROADCOM_COM_IfName,
                                               newObj->X_BROADCOM_COM_FullconeNATEnabled,
                                               newObj->NATEnabled,
                                               newObj->X_BROADCOM_COM_FirewallEnabled);

      /* setup Qos queues associated with this interface */
      if ((ret = rutQos_qMgmtQueueReconfig(newObj->X_BROADCOM_COM_IfName, FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtQueueReconfig returns error. ret=%d", ret);
         return ret;
      }
      rutQos_doDefaultPolicy();

      /* Reconfig QoS port shaping for all Ethernet interfaces */
      rutQos_portShapingConfigAll();

      if (!rutWl2_isIPoA(iidStack))
      {
         cmsLog_debug("IPoE connection up, activate intf with IPAddress = %s", newObj->externalIPAddress);
         if ((ret = rutWan_activateIpEthInterface(newObj)) != CMSRET_SUCCESS)
         {
            return ret;
         }

#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */
         /* Add the policy routing (action only) if this WAN is part of the interface group */
         if (rutPMap_isWanUsedForIntfGroup(newObj->X_BROADCOM_COM_IfName) == TRUE)
         {
            UBOOL8 add=TRUE;
            if ((ret = rutPMap_configPolicyRoute(add, 
                                                 newObj->X_BROADCOM_COM_IfName,
                                                 newObj->defaultGateway)) != CMSRET_SUCCESS)
            {
               return ret;
            }
         }
#endif
      }
   }

#ifdef SUPPORT_UPNP
   if (rut_isUpnpEnabled() && rutUpnp_checkRunStatusWithDelay() == FALSE)
   {
      rut_restartUpnp(NULL);
   }
#endif

   /* advanced DMZ for IPoE will be here if needed. See old version of this file some code examples (not working) */

   /* do dns and system default gateway, etc. only on routed connection */
   if (isRouted)
   {
      /* reactivate system default gateway and dns */
      if (rutWan_doDefaultSystemGatewayAndDNS(TRUE) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set up default system gateway and dns on %s.", newObj->X_BROADCOM_COM_IfName);
      }
#ifdef DMP_BASELINE_1
      /* miwang: temporarily mark this as BASELINE only.  When we do PURE181,
       * wan connection and route activation will be different.  */
      /* setup L3 forwarding object if any for this interface */
      rutRt_activateL3ForwardingEntry(newObj->X_BROADCOM_COM_IfName);
#endif

      /* Activate tunnels associated with this IPoE connection */
      rutTunnel_control(newObj->X_BROADCOM_COM_IfName, MDMVS_IPV6INIPV4, TRUE);
   }

#ifdef SUPPORT_RIP
   /*
    * IPoE interface is now UP.  If this interface has RIP enabled,
    * write out config file and restart.  Note we do not check for NAT
    * enabled or not.  That checking must be done when RIP was initially
    * enabled and configured.
    */
   if (cmsUtl_strcmp(newObj->routeProtocolRx, MDMVS_OFF))
   {
      rutRip_writeConfigFile();
      rutRip_restart();
   }
#endif

#ifdef DMP_X_BROADCOM_COM_IGMP_1
   if (newObj->X_BROADCOM_COM_IGMPEnabled)
   {
      rutIpt_igmpRules(TRUE, newObj->X_BROADCOM_COM_IfName);
   }
#endif

#ifdef SUPPORT_IPSEC
   if(isRouted)
   {
      rutIPSec_config();
      rutIPSec_restart();
   }
#endif

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
   rutL2tp_refreshL2tp();
#endif 
   rutMulti_updateIgmpMldProxyIntfList();

   printf("All services associated with %s is activated.\n", newObj->X_BROADCOM_COM_IfName);
   
   return ret;
}



CmsRet rutCfg_tearDownWanIpConnection(const InstanceIdStack *iidStack __attribute((unused)),
                                      const _WanIpConnObject *currObj, UBOOL8 isIPv4)
{
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("Enter.");

   /* remove related ip modules if not bridge mode (IPoE/IPoA) */
   if (cmsUtl_strcmp(currObj->connectionType, MDMVS_IP_BRIDGED))
   {

#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */
      /* If this routed IPoE WAN is part of the interface group,
      * need to remove  the policy routing (action only).
      */
      if (!rutWl2_isIPoA(iidStack) && rutPMap_isWanUsedForIntfGroup(currObj->X_BROADCOM_COM_IfName))
      {
         UBOOL8 add = FALSE;
         rutPMap_configPolicyRoute(add, currObj->X_BROADCOM_COM_IfName, currObj->defaultGateway);
      }
#endif

      cmsLog_debug("Removing iptables rules for %s", currObj->X_BROADCOM_COM_IfName);
      rutIpt_removeInterfaceIptableRules(currObj->X_BROADCOM_COM_IfName, isIPv4);

#ifdef DMP_X_BROADCOM_COM_IGMP_1
      if ( isIPv4 && currObj->X_BROADCOM_COM_IGMPEnabled)
      {
         rutIpt_igmpRules(FALSE, currObj->X_BROADCOM_COM_IfName);
      }
#endif

      /* Deactivate the tunnels associated with this IP connection */
      rutTunnel_control(currObj->X_BROADCOM_COM_IfName, MDMVS_IPV6INIPV4, FALSE);
   }

#ifdef DMP_X_BROADCOM_COM_IGMP_1
   if ( isIPv4 && currObj->X_BROADCOM_COM_IGMPEnabled)
   {
      rutIpt_igmpRules(FALSE, currObj->X_BROADCOM_COM_IfName);
   }
#endif

#ifdef SUPPORT_UPNP
   /* reevaluate upnp process attachment to reflect the current modem state */
   if ( isIPv4 && rut_isUpnpEnabled())   
   {
      rut_restartUpnp(currObj->X_BROADCOM_COM_IfName);
   }    
#endif

   /* for static IPoE, need to delete next hop -- eg.  route del -net 10.6.33.128 netmask 255.255.255.192 gw 10.6.33.129 */
   if (isIPv4 && cmsUtl_strcmp(currObj->addressingType, MDMVS_STATIC) == 0)
   {
      
      char cmdStr[BUFLEN_128 + CMS_IPADDR_LENGTH];
      char bCastStr[CMS_IPADDR_LENGTH];
      char subnetStr[CMS_IPADDR_LENGTH];

      if ((rutWan_getBcastStrAndSubnetFromIpAndMask(currObj->externalIPAddress,
                                                    currObj->subnetMask,
                                                    bCastStr,
                                                    subnetStr)) != CMSRET_SUCCESS)
      {
         ret = CMSRET_INVALID_ARGUMENTS;
      }
                                                    
      snprintf(cmdStr, sizeof(cmdStr), "route del -net %s netmask %s gw %s 2>/dev/null", subnetStr, currObj->subnetMask, currObj->defaultGateway);
      rut_doSystemAction("rcl", cmdStr);     
   }

   /* reactivate system default gateway and dns */
   if (rutWan_doDefaultSystemGatewayAndDNS(isIPv4) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set up default system gateway and dns on %s.", currObj->X_BROADCOM_COM_IfName);
   }

   if ( !isIPv4 )
   {
      rutCfg_tearDownWanCon6(currObj, TRUE);
	  rutMulti_updateMldProxyIntfList();
   }
   else
   {
	  rutMulti_updateIgmpProxyIntfList();
   }

   cmsLog_debug("Exit ret %d", ret);
   
   return ret;
   
}



CmsRet rutCfg_stopWanIpConnection(const InstanceIdStack *iidStack, 
                                  const _WanIpConnObject *currObj, UBOOL8 isIPv4)
               
{
   CmsRet ret=CMSRET_SUCCESS;
   cmsLog_debug("Enter.");

   /* Delete the bridge interface from br0 */
   if (!cmsUtl_strcmp(currObj->connectionType, MDMVS_IP_BRIDGED))
   {
#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1
      if( (rutOpenVS_isEnabled()) && (rutOpenVS_isOpenVSPorts(currObj->X_BROADCOM_COM_IfName)))
      {
         rutOpenVS_shutdownOpenVSport(currObj->X_BROADCOM_COM_IfName);
         return CMSRET_SUCCESS;
      }
      else
      {
         ret = removeInterfaceFromBridge(currObj->X_BROADCOM_COM_IfName);
      }
#else
      ret = removeInterfaceFromBridge(currObj->X_BROADCOM_COM_IfName);
#endif
   }

   if (!rutWl2_isIPoA(iidStack))
   {
      CmsRet r2;

      if (isIPv4)
      {
#ifdef SUPPORT_RIP
         char ifName[BUFLEN_32] = {0};

         /* reevaluate rip process attachment to reflect the current modem state */
         if (cmsUtl_strcmp(currObj->routeProtocolRx, MDMVS_OFF) && rut_isApplicationRunning(EID_RIPD))
         {
            if (!rutWan_findFirstIpvxRoutedAndConnected(CMS_AF_SELECT_IPVX, ifName))
            {
               rutRip_stop();
            }
         }
#endif

         /* It's a dynamic IPoE and need to stop dhcpc */
         if (!cmsUtl_strcmp(currObj->addressingType, MDMVS_DHCP)) 
         {     
            UINT32 specificEid = MAKE_SPECIFIC_EID(currObj->X_BROADCOM_COM_DhcpcPid, EID_DHCPC);

            cmsLog_debug("stop dhcpc pid=%d on IfName=%s", currObj->X_BROADCOM_COM_DhcpcPid, currObj->X_BROADCOM_COM_IfName);
            if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0) != CMSRET_SUCCESS)
            {
               cmsLog_error("failed to send msg to stop dhcpc");
            }
            else
            {
               cmsLog_debug("dhcpc stopped");
            }
         }
      }
      else
      {
         rutCfg_stopWanCon6(currObj, TRUE);
      }

      /* Need to stop the following IPoE or bridge interfaces if they are vlan or msc */
      if ((r2 = rutWan_stopL3Interface(MDMOID_WAN_IP_CONN, 
                                       iidStack,
                                       currObj->X_BROADCOM_COM_VlanMuxID,
                                       currObj->X_BROADCOM_COM_ConnectionId,
                                       currObj->X_BROADCOM_COM_IfName)) != CMSRET_SUCCESS)
      {                               
         cmsLog_error("rutWan_stopInterfacer failed. ret=%d", r2);
      }
   }

   return ret;
   
}




CmsRet rutCfg_deleteWanIpConnection(const InstanceIdStack *iidStack __attribute((unused)),
                                    const _WanIpConnObject *currObj)
{
    CmsRet ret=CMSRET_SUCCESS;
    
   if (!currObj->X_BROADCOM_COM_IfName)
   {
      /* if WanIfName is NULL, the adding WanIPConn object failed and just return CMSRET_SUCESS to
      * to delete the object instance
      */
      cmsLog_debug("NULL wan ifName. Just return.");
      return ret;
   }

#ifdef SUPPORT_RIP
   /*
    * This IP connection is being deleted.  Count the remaining number of
    * RIP interfaces on the system.  If there are none left, then stop ripd.
    * Note that we leave ripd up even if we lose IP address or link because
    * when link comes back up and we get IP address again, we restart
    * ripd anyways.
    */
   if (rutRip_getNumberOfRipInterfaces() == 0)
   {
      cmsLog_debug("No more RIP interfaces, stop rip");
      rutRip_stop();
   }
   else
   {
      rutRip_writeConfigFile();
      rutRip_restart();
   }
#endif

#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */
   if (!rutWl2_isIPoA(iidStack))
   {
      /* IPoA has no PORT_MAP entry.  Bridge, IPoE has them and need to be deleted */   

      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      UBOOL8 isWanIntf=TRUE;
      char fullName[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      
      /* need to get full path name for this interface for removing the available interface 
      * system default gateway in layer 3 forwording object.  Make the delete object 
      * visible first.
      */
      mdmLibCtx.hideObjectsPendingDelete = FALSE;
      ret = rutPMap_wanIfNameToAvailableInterfaceReference(currObj->X_BROADCOM_COM_IfName, fullName);
      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
      
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("rutPMap_wanIfNameToAvailableInterfaceReference failed, ret=%d", ret);
      }

      /* Only IPoE can be in the interface group and if it is part of it, remove the subnet (bridge) */
      if (!cmsUtl_strcmp(currObj->connectionType, MDMVS_IP_ROUTED))

      {
         rutPMap_removeInterfaceGroup(fullName);   
      }
      rutPMap_deleteFilter(fullName, isWanIntf);
      rutPMap_deleteAvailableInterface(fullName, isWanIntf);

   }
#endif /* DMP_BRIDGING_1  aka SUPPORT_PORT_MAP */

   return (rutWan_deleteWanIpOrPppConnection(currObj->X_BROADCOM_COM_IfName));
   
}

static CmsRet removeInterfaceFromBridge(char *ifName)
{
    CmsRet ret=CMSRET_SUCCESS;

#ifdef DMP_BRIDGING_1
    char bridgeIfName[CMS_IFNAME_LENGTH];
    char availInterfaceReference[MDM_SINGLE_FULLPATH_BUFLEN]={0};
	  
    if (rutPMap_wanIfNameToAvailableInterfaceReference(ifName, availInterfaceReference) == CMSRET_SUCCESS)
    {
        ret = rutPMap_getBridgeIfNameFromIfName(ifName,  bridgeIfName, TRUE);
        if (ret != CMSRET_SUCCESS)
            return ret;
    }
    else 
    {
        strcpy(bridgeIfName, "br0");
    }
    rutLan_removeInterfaceFromBridge(ifName, bridgeIfName);
#else
    rutLan_removeInterfaceFromBridge(ifName, "br0");
#endif
    return ret;
}

#endif  /* DMP_BASELINE_1 */

