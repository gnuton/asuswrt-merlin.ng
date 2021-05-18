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

#ifdef DMP_BASELINE_1


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
#include "rut_lan.h"
#include "rut_pmap.h"
#include "rut_upnp.h"
#include "rut_rip.h"
#include "rut_network.h"
#include "rut_pppconcfg.h"
#include "rut_route.h"
#include "rut_xtmlinkcfg.h"
#include "rut_qos.h"
#include "rut_dsl.h"
#include "rut_ipsec.h"
#include "rut_wanlayer2.h"
#include "rut_multicast.h"
#include "rut_iptunnel.h"
#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
#include "rut_l2tpac.h"
#endif
CmsRet rutCfg_startWanPppConnection(const InstanceIdStack *iidStack,
                                    _WanPppConnObject *newObj)
{
   UBOOL8 isBridge=FALSE;
   CmsRet ret=CMSRET_SUCCESS;
   ConnectionModeType connMode;
   char l2IfName[CMS_IFNAME_LENGTH]={0};
   char baseL3IfName[CMS_IFNAME_LENGTH]={0};

   cmsLog_debug("Enter.");

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
   if (rutWl2_isPPPoL2tp(iidStack))
   {
	  return CMSRET_SUCCESS;
   }
#endif   

   /* if ppp interface is created, skip initPPP part */
   if (rut_wanGetIntfIndex(newObj->X_BROADCOM_COM_IfName) <= 0)
   {
      /* Note: isBridge is referenced in rutWan_startL3Interface() when creating
       * vlan interface for VLANMUX connection mode.
       * When Bridge PPPoE Frames Between WAN and Local Ports is enabled, in
       * VLLANMUX mode, the vlan interface will be added to the bridge br0.
       * If the vlan interface is created as routed, ppp pass-thru won't work.
       * We need to create the vlan interface as bridged, instead of routed.
       */
      if (newObj->X_BROADCOM_COM_AddPppToBridge)
      {
         isBridge = TRUE;
      }
   
      /* Need layer 2 and baseL3IfName and connMode to start a layer 3 interface */
      if ((ret = rutWan_getL3InterfaceInfo(MDMOID_WAN_PPP_CONN, 
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
      } 
   
      if (!rutWl2_isPPPoA(iidStack))
      {
         if ((ret = rutWan_initPPPoE(iidStack, newObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Init PPPoE failed.");
         }
      }
      else
      {
         if ((ret = rutWan_startPPPoA(iidStack, newObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Fail to start IPoA connection.");
         }
      }
   }
      
   return ret;
   
}



CmsRet rutCfg_setupWanPppConnection(const InstanceIdStack *iidStack  __attribute((unused)),
                                    _WanPppConnObject *newObj)
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Enter.");

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
   if (rutWl2_isPPPoL2tp(iidStack))
   {
	 return CMSRET_SUCCESS;
   }
#endif   

   /* after ppp connected, ip extension requires its own ip rules  */
   if (newObj->X_BROADCOM_COM_IPExtension) 
   {
      if ((ret = rutWan_IpExtensionRelay(newObj->X_BROADCOM_COM_IfName,
                                         newObj->X_BROADCOM_COM_DefaultGateway,
                                         newObj->externalIPAddress,
                                         TRUE)) != CMSRET_SUCCESS)
      {
         
         cmsLog_error("Failed to init ip extension");
         return ret;
      }
#ifdef SUPPORT_ADVANCED_DMZ
      if (rutNtwk_isAdvancedDmzEnabled() == TRUE)
      {
         if ((ret = rutNtwk_startNonDmzDhcpd(newObj->externalIPAddress)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to active advanced DMZ");
            return ret;
         }
      }
#endif /* SUPPORT_ADVANCED_DMZ */

   }

#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */
   /* only pppoe can be in the interface group. TODO: ask Dan on test CMS_WAN_TYPE_ETHERNET_PPPOE*/
   if (!rutWl2_isPPPoA(iidStack))
   {
       /* Add the policy routing (action on) if this WAN is part of the interface group */
      if (rutPMap_isWanUsedForIntfGroup(newObj->X_BROADCOM_COM_IfName) == TRUE)
      {
         UBOOL8 add=TRUE;
         if ((ret = rutPMap_configPolicyRoute(add, 
                                              newObj->X_BROADCOM_COM_IfName,
                                              newObj->X_BROADCOM_COM_DefaultGateway))!= CMSRET_SUCCESS)
         {
            return ret;
         }
      }
   }
#endif

   /* call common function to do NAT related stuff */
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

#ifdef SUPPORT_UPNP
   if (rut_isUpnpEnabled() && rutUpnp_checkRunStatusWithDelay() == FALSE)
   {
      rut_restartUpnp(NULL);
   }
#endif

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

   /* Activate tunnels associate with this PPP connection */
   rutTunnel_control(newObj->X_BROADCOM_COM_IfName, MDMVS_IPV6INIPV4, TRUE);

#ifdef DMP_X_BROADCOM_COM_IGMP_1
   if (newObj->X_BROADCOM_COM_IGMPEnabled)
   {
      rutIpt_igmpRules(TRUE, newObj->X_BROADCOM_COM_IfName);
   }
#endif
   
#ifdef SUPPORT_IPSEC
     rutIPSec_config();
     rutIPSec_restart();
#endif /* SUPPORT_IPSEC */

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
	rutL2tp_refreshL2tp();
#endif 

   rutMulti_updateIgmpMldProxyIntfList();

   printf("All services associated with %s is activated.\n", newObj->X_BROADCOM_COM_IfName);
   
   return ret;
   
}



CmsRet rutCfg_tearDownWanPppConnection(const InstanceIdStack *iidStack,
                                       _WanPppConnObject *newObj,
                                       const _WanPppConnObject *currObj, UBOOL8 isIPv4)

{
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("Enter.");
   
/* UPnP host may command to bring down the dsl link which causes the connection
 * to go down. In this case, we want to keep the UPnP process to communicate with
 * the host UPnP.  So no rut_stopUpnp(); here
 */ 

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
   if (rutWl2_isPPPoL2tp(iidStack))
   {
	 return CMSRET_SUCCESS;
   }
#endif 

#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */
   /* Only pppoe can be in the interface group and if this WAN is part of the interface group,
   * need to remove  the policy routing (action only).
   */
   if (!rutWl2_isPPPoA(iidStack) && rutPMap_isWanUsedForIntfGroup(currObj->X_BROADCOM_COM_IfName))
   {
      UBOOL8 add = FALSE;
      rutPMap_configPolicyRoute(add, currObj->X_BROADCOM_COM_IfName, currObj->X_BROADCOM_COM_DefaultGateway);
   }
#endif

   cmsLog_debug("Removing iptables rules for %s", currObj->X_BROADCOM_COM_IfName);

   rutIpt_removeInterfaceIptableRules(currObj->X_BROADCOM_COM_IfName, isIPv4);

#ifdef DMP_X_BROADCOM_COM_IGMP_1
   if (isIPv4 && currObj->X_BROADCOM_COM_IGMPEnabled)
   {
      rutIpt_igmpRules(FALSE, currObj->X_BROADCOM_COM_IfName);
   }
#endif

   /* Deactivate the tunnels associated with this IP connection */
   rutTunnel_control(currObj->X_BROADCOM_COM_IfName, MDMVS_IPV6INIPV4, FALSE);

   if (isIPv4 && currObj->X_BROADCOM_COM_IPExtension)
   {
      if (newObj)
      {
         /* not delete case, just wipeout the pppObj->DNSServers info and restore dhcp 
          * info to the default instead of special dhcpd relay
          */
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->DNSServers);
         rutNtwk_RestoreDefaultDhcpConfig();
      }
   }
   
   
#ifdef SUPPORT_UPNP
   /* reevaluate upnp process attachment to reflect the current modem state */
   if (isIPv4 && rut_isUpnpEnabled())   
   {
      rut_restartUpnp(currObj->X_BROADCOM_COM_IfName);
   }      
#endif 

   /* reactivate system default gateway and dns */
   if (rutWan_doDefaultSystemGatewayAndDNS(isIPv4) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set up default system gateway and dns on %s.", currObj->X_BROADCOM_COM_IfName);
   }
   
   if ( !isIPv4 )
   {
      rutCfg_tearDownWanCon6(currObj, FALSE);
   }

   if (!rutWl2_isPPPoA(iidStack))
   {
      if ((ret = rutWan_cleanUpPPPoE(iidStack, currObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Clean up PPPoE failed.");
      }
   }

   if (isIPv4)
   {
      rutMulti_updateIgmpProxyIntfList();
   }
   else
   {
      rutMulti_updateMldProxyIntfList();
   }

   cmsLog_debug("Exit ret %d", ret);
   
   return ret;
   
}

CmsRet rutCfg_stopWanPppConnection(const InstanceIdStack *iidStack, 
                                   const _WanPppConnObject *currObj, UBOOL8 isIPv4)
{
   UINT32 specificEid;
   CmsRet ret = CMSRET_SUCCESS;
   
   cmsLog_debug("Enter");

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
   if (rutWl2_isPPPoL2tp(iidStack))
   {
 	 return CMSRET_SUCCESS;
   }
#endif   

   if (!isIPv4 )
   {
      rutCfg_stopWanCon6(currObj, FALSE);
   }
   /*
   * Currently, IPv6 at PPPoE relies on IPv4 configuration to bring up ppp0 interface. 
   * So only IPv4 configuration kills pppd 
   */
   else
   {
      specificEid = MAKE_SPECIFIC_EID(currObj->X_BROADCOM_COM_PppdPid, EID_PPP); 
      if ((ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to stop pppd");
      }
      else
      {
         cmsLog_debug("pppd stopped");
      }
   }

   /* only for pppoe wan interface */
   if (!rutWl2_isPPPoA(iidStack))
   {
      CmsRet r2;
      
      /* Need to stop the following IPoE or bridge interfaces if they are vlan or msc */
      if ((r2 = rutWan_stopL3Interface(MDMOID_WAN_PPP_CONN, 
                                       iidStack,
                                       currObj->X_BROADCOM_COM_VlanMuxID,
                                       currObj->X_BROADCOM_COM_ConnectionId,
                                       currObj->X_BROADCOM_COM_IfName)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutWan_stopL3Interface failed. ret=%d", r2);
      }

   }

   return ret;
   
}


CmsRet rutCfg_deleteWanPppConnection(const InstanceIdStack *iidStack __attribute((unused)),
                                     const _WanPppConnObject *currObj)
{
    CmsRet ret=CMSRET_SUCCESS;
    
   if (!currObj->X_BROADCOM_COM_IfName)
   {
      /* if WanIfName is NULL, the adding WanPPPConn object failed and just return CMSRET_SUCESS to
      * to delete the object instance
      */
      cmsLog_debug("NULL wan ifName. Just return.");
      return ret;
   }

      
#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */
   /* need to remove interface group if PPPoE connection type */
   if (!rutWl2_isPPPoA(iidStack))
   {
      UBOOL8 isWanIntf=TRUE;
      char fullName[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      
      mdmLibCtx.hideObjectsPendingDelete = FALSE;
      ret = rutPMap_wanIfNameToAvailableInterfaceReference(currObj->X_BROADCOM_COM_IfName, fullName);
      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("rutPMap_wanIfNameToAvailableInterfaceReference failed, ret=%d", ret);
      }
      else
      {
         /* Only pppoe can be in the interface group and if it is part of it, remove the subnet (bridge) 
          * todo: ask Dan on adding eth_pppoe as interface group
          */
         if (!rutWl2_isPPPoA(iidStack))
         {
            rutPMap_removeInterfaceGroup(fullName);   
         }
         rutPMap_deleteFilter(fullName, isWanIntf);
         rutPMap_deleteAvailableInterface(fullName, isWanIntf);
      }
   }
#endif /* DMP_BRIDGING_1 */   

      
   return (rutWan_deleteWanIpOrPppConnection(currObj->X_BROADCOM_COM_IfName));
   
}


#endif  /* DMP_BASELINE_1 */
