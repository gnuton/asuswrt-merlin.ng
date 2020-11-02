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
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_boardcmds.h"
#include "cms_boardioctl.h"
#include "rcl.h"
#include "rut_lan.h"
#include "rut_ethswitch.h"
#include "rut_util.h"
#include "rut_network.h"
#include "rut_iptables.h"
#include "rut_wan.h"
#include "rut_system.h"
#include "rut_dnsproxy.h"
#include "rut_multicast.h"
#include "rut_qos.h"
#include "rut_ethintf.h"
#include "rut_route.h"
#ifdef DMP_X_BROADCOM_COM_WIFILAN_1
#include "rut_wlan.h"
#endif

#ifdef SUPPORT_LANVLAN
#include "rut_pmap.h"
#include "ethswctl_api.h"
#if defined(DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1) 
#include "rut_omci.h"
#endif
#endif


#ifdef DMP_BASELINE_1
/* Most of the functions in this file are only used in Legacy TR98 mode and
 * Hybrid TR98+TR181 mode.  However, the Broadcom defined
 * rcl_igmpSnoopingCfgObject is data model independent.
 */


CmsRet rcl_lanDevObject( _LanDevObject *newObj,
                const _LanDevObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   if (ADD_NEW(newObj, currObj))
   {
       rut_modifyNumLanDev(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rut_modifyNumLanDev(iidStack, -1);
   }
   
   /* no other work needs to be done for Lan Dev Object */

   return CMSRET_SUCCESS;
}


CmsRet rcl_lanHostCfgObject(_LanHostCfgObject *newObj __attribute__((unused)),
                            const _LanHostCfgObject *currObj __attribute__((unused)),
                            const InstanceIdStack *iidStack __attribute__((unused)),
                            char **errorParam __attribute__((unused)),
                            CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

#ifdef SUPPORT_UDHCP
   
   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   /* activate or change dhcp server */
   if ((newObj != NULL && newObj->DHCPServerEnable && 
   	(currObj == NULL || !currObj->DHCPServerEnable)) ||
       rutLan_isHostCfgChanged(newObj, currObj))
   {
      ret = rutLan_updateDhcpd();

   }  /* end of enable new or enable existing */ 

   /* delete or disable existing */
   else if ((newObj == NULL) ||
       (newObj != NULL && !(newObj->DHCPServerEnable) && currObj != NULL && currObj->DHCPServerEnable))
   {
      ret = rutLan_updateDhcpd();
   }

   cmsLog_debug("done, ret=%d", ret);

#else

   /*
    * On 6816R, dhcpd is not compiled in.  So don't try to start it.
    * It is not easy to define out dhcpd using the TR-098 profile mechanism,
    * so we use the old SUPPORT_UDHCP method instead.
    */

   cmsLog_debug("don't start dhcpd because it is not compiled in.");

#endif  /* SUPPORT_UDHCP */

   return ret;
}


CmsRet rcl_lanIpIntfObject(_LanIpIntfObject *newObj,
                           const _LanIpIntfObject *currObj,
                           const InstanceIdStack *iidStack,
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   
   char ipAddrStr[CMS_IPADDR_LENGTH]={0};
   char subnetMaskStr[CMS_IPADDR_LENGTH]={0};
   char bCastStr[CMS_IPADDR_LENGTH]={0};
   _LanDevObject *lanDevObj = NULL;
   _LanIpIntfObject *lanIpIntfObj = NULL;
   InstanceIdStack iidStackAncestor = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackSub      = EMPTY_INSTANCE_ID_STACK;
   UINT32 numOfIntfs = 0;
   char   brNameOfIntf[BUFLEN_64]={0};          

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   
   /* new bridge which is DHCP CLIENT: do not use default ip addresses: */
   if (newObj != NULL && currObj == NULL
     && newObj->IPInterfaceAddressingType != NULL 
     && !cmsUtl_strcmp(newObj->IPInterfaceAddressingType, MDMVS_DHCP)) {
       cmsLog_debug("setting lan side dhcp client to address 0.0.0.0");       
       CMSMEM_REPLACE_STRING_FLAGS(newObj->IPInterfaceIPAddress, "0.0.0.0", mdmLibCtx.allocFlags);
       CMSMEM_REPLACE_STRING_FLAGS(newObj->IPInterfaceSubnetMask, "0.0.0.0", mdmLibCtx.allocFlags );
   }
   
   /*
    * If this is a new bridge creation and there is no major ip address, get an available bridge number.
    * If this is a new bridge creation and this is the secondary ip address, assign "brX:0" to X_BROADCOM_COM_IfName.
    * If this is a new bridge creation and this is the third ip address, return error.
    */
   if (newObj != NULL && currObj == NULL && newObj->X_BROADCOM_COM_IfName == NULL)
   {

      /* copy the original iidStack into our iidStack, which will get modified */
      iidStackAncestor = *iidStack;

      /*find current object ancestor*/
      if (cmsObj_getAncestor(MDMOID_LAN_DEV, MDMOID_LAN_IP_INTF,
                             &iidStackAncestor,
                             (void **) &lanDevObj) != CMSRET_SUCCESS)
      {
         cmsLog_error("Current lan ip intf object have no ancestor.");
         return CMSRET_MDM_TREE_ERROR;
      }
      cmsObj_free((void **) &lanDevObj);
   
      /*look into substree, count the number of lan ip interface and record the major bridge name*/
      while (((ret = cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF,
                                             &iidStackAncestor,
                                             &iidStackSub,
                                             (void **)&lanIpIntfObj)) == CMSRET_SUCCESS))
      {
         if (lanIpIntfObj->X_BROADCOM_COM_IfName != NULL && 
             cmsUtl_strstr(lanIpIntfObj->X_BROADCOM_COM_IfName, ":") == NULL)
         {
            strcpy(brNameOfIntf, lanIpIntfObj->X_BROADCOM_COM_IfName);   
         }
         numOfIntfs ++;
         cmsObj_free((void **)&lanIpIntfObj);
      }
      
      if ((numOfIntfs == 1) && (brNameOfIntf[0] == 0))
      {
         SINT32 bridgeNum;

         /* add first bridge device for this subnet */
         if ((bridgeNum = rutLan_getNextAvailableBridgeNumber()) < 0)
         {
            return CMSRET_RESOURCE_EXCEEDED;
         }
         
         sprintf(brNameOfIntf, "br%d", bridgeNum);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_IfName, brNameOfIntf, mdmLibCtx.allocFlags);
      }
      else if ((numOfIntfs == 2) && (brNameOfIntf[0] != 0))
      {
         /* add secondary ip address for this subnet */
         strcat(brNameOfIntf, ":0");
         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_IfName, brNameOfIntf, mdmLibCtx.allocFlags);   
      }
      else
      {
         return CMSRET_INVALID_ARGUMENTS;
      }

      rut_modifyNumLanIpIntf(iidStack, 1);
   } 

   ret = CMSRET_SUCCESS;

   /* enable or change ip interface */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj) ||
       rutLan_isIpv4IntfChanged(newObj, currObj) ||
       rutLan_isIpv6IntfChanged(newObj, currObj))
   {
      UBOOL8 FirewallOn = FALSE;
      UBOOL8 FirewallOff = FALSE;

#ifdef DMP_X_BROADCOM_COM_SECURITY_1
      /*
       * This stuff is for the LAN side firewall.
       */
      if (newObj->X_BROADCOM_COM_FirewallEnabled &&
          ((currObj == NULL) ||
           ((currObj != NULL) && !currObj->X_BROADCOM_COM_FirewallEnabled)))
      {
         FirewallOn = TRUE;
      }
      else if (!newObj->X_BROADCOM_COM_FirewallEnabled &&
               (currObj != NULL) &&
               currObj->X_BROADCOM_COM_FirewallEnabled)
      {
         FirewallOff = TRUE;
      }
#endif

      if (newObj->X_BROADCOM_COM_IfName == NULL)
      {
         cmsLog_error("bridge IfName must be set prior to enable");
         return CMSRET_INVALID_ARGUMENTS;
      }


      strncpy(ipAddrStr, newObj->IPInterfaceIPAddress, sizeof(ipAddrStr)-1);
      strncpy(subnetMaskStr, newObj->IPInterfaceSubnetMask, sizeof(subnetMaskStr)-1);

      if ((ret = rut_getBCastFromIpSubnetMask(ipAddrStr, subnetMaskStr, bCastStr)) != CMSRET_SUCCESS)
      {
         return ret;
      }

      rutLan_enableBridge(newObj->X_BROADCOM_COM_IfName,
                          (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj)),
                          ipAddrStr, subnetMaskStr, bCastStr);

#ifdef SUPPORT_LANVLAN
      {
#ifdef DMP_BRIDGING_1
         L2BridgingFilterObject *bObj = NULL;
         char bridgeIfName[CMS_IFNAME_LENGTH];
         char vlanIfName[CMS_IFNAME_LENGTH];
         InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
         
         while (cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, &iidStack2, (void **)&bObj) == CMSRET_SUCCESS)
         {
            if (bObj->X_BROADCOM_COM_VLANIDFilter > 0 && bObj->filterBridgeReference >= 0)
            {
               sprintf(bridgeIfName, "br%d", bObj->filterBridgeReference);
               if (!cmsUtl_strcmp(bridgeIfName, newObj->X_BROADCOM_COM_IfName))
               {
                  if ((ret = rutPMap_filterInterfaceToIfName(bObj->filterInterface, vlanIfName)) != CMSRET_SUCCESS)
                  {
                     cmsLog_error("could not convert filterInterface %s to lanIfName", bObj->filterInterface);
                  }
                  else
                  {
                     char vlanIfName2[BUFLEN_8]={0};
                     snprintf(vlanIfName2, sizeof(vlanIfName2), ".%d", bObj->X_BROADCOM_COM_VLANIDFilter);
                     cmsUtl_strncat(vlanIfName, CMS_IFNAME_LENGTH, vlanIfName2);
                     rutLan_addInterfaceToBridge(vlanIfName, FALSE, bridgeIfName);
                  }
               }
            }
            cmsObj_free((void **)&bObj);
         }
#endif /* DMP_BRIDGING_1 */

         /*
          * In general, this is not exactly the right place to do LANVLAN,
          * but since we are dealing with this stuff here, update the
          * Real HW switching setting which is affected by LANVLAN.
          */
         rutEsw_updateRealHwSwitchingSetting(QOS_CLS_INVALID_INDEX);
      }
#endif
      if ((newObj->X_BROADCOM_COM_DhcpcPid != 0) &&
          (cmsUtl_strcmp(newObj->IPInterfaceAddressingType, MDMVS_DHCP) ||
          (cmsUtl_strcmp(newObj->X_BROADCOM_COM_DhcpConnectionStatus, MDMVS_CONNECTED))))
      {
         /*
          * DHCP client is running but either
          * - we are not in DHCP mode anymore, or
          * - all ethernet interfaces on this bridge are down.
          * stop dhcpc.
          */
         UINT32 eid;
         cmsLog_debug("stopping dhcpc on %s (pid=%d)",
                      newObj->X_BROADCOM_COM_IfName,
                      newObj->X_BROADCOM_COM_DhcpcPid);
         eid = MAKE_SPECIFIC_EID(newObj->X_BROADCOM_COM_DhcpcPid, EID_DHCPC);
         if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, eid, NULL, 0) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to send msg to stop dhcpc (eid=0x%x", eid);
         }
         else
         {
            cmsLog_debug("dhcpc (eid=0x%x) stopped", eid);
         }
         newObj->X_BROADCOM_COM_DhcpcPid = 0;
      }

      if (!cmsUtl_strcmp(newObj->IPInterfaceAddressingType, MDMVS_DHCP) &&
          !cmsUtl_strcmp(newObj->X_BROADCOM_COM_DhcpConnectionStatus, MDMVS_CONNECTING) &&
          (newObj->X_BROADCOM_COM_DhcpcPid == 0))
      {
         /*
          * LAN side is in DHCP mode and we have link up and have not started
          * dhcp client yet, start it now.
          */
         cmsLog_debug("starting dhcpc on %s", newObj->X_BROADCOM_COM_IfName);
#ifdef SUPPORT_HOMEPLUG
         newObj->X_BROADCOM_COM_DhcpcPid = rutWan_startDhcpc(
                     newObj->X_BROADCOM_COM_IfName, NULL, NULL, NULL, NULL, TRUE, NULL, NULL, NULL, FALSE);
#else
         newObj->X_BROADCOM_COM_DhcpcPid = rutWan_startDhcpc(
                     newObj->X_BROADCOM_COM_IfName, NULL, NULL, NULL, NULL, FALSE, NULL, NULL, NULL, FALSE);
#endif
      }

      if (newObj && currObj &&
          cmsUtl_strcmp(newObj->X_BROADCOM_COM_DNSServers,
                        currObj->X_BROADCOM_COM_DNSServers))
      {
         /* DNSServers have changed, reconfig system IPv4 DNS */
         rutNtwk_doSystemDns(TRUE);
      }

      if (newObj && currObj &&
          cmsUtl_strcmp(newObj->X_BROADCOM_COM_DhcpDefaultGateway,
                        currObj->X_BROADCOM_COM_DhcpDefaultGateway))
      {
         /* Gateway has changed, reconfig system IPv4 default gateway */
         rutRt_doSystemDefaultGateway(TRUE);
      }

#ifndef BUILD_BRCM_UNFWLCFG
#ifdef DMP_X_BROADCOM_COM_WIFILAN_1
      if (rutLan_isIpv4AddressChanged(newObj, currObj) && !ADD_NEW(newObj, currObj)) {
		rutWlan_requestRestart("Modify",0);    	  	
      }
#endif 
#endif /* BUILD_BRCM_UNFWLCFG */
      if (rutLan_isIpv4IntfChanged(newObj, currObj) ||
         ADD_NEW(newObj, currObj))
      {
         /* The LAN side firewall needs to be applied if
         * 1). there is a change in interface ie. rutLan_isIpv4IntfChanged is TRUE
         * 2).  the device is in the reboot process ADD_NEW(newObj, currObj) is TRUE
         */
         if (FirewallOn)
         {
            cmsLog_notice("Starting firewall on %s", newObj->X_BROADCOM_COM_IfName);
            rutIpt_insertIpModules();
            rutIpt_initFirewall(PF_INET, newObj->X_BROADCOM_COM_IfName);
            rutIpt_initFirewallExceptions(newObj->X_BROADCOM_COM_IfName);
         }
         else if (FirewallOff)
         {
            cmsLog_notice("remove firewall on %s", newObj->X_BROADCOM_COM_IfName);
            rutIpt_removeInterfaceIptableRules(newObj->X_BROADCOM_COM_IfName, TRUE);
         }
   
         if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj) &&
             (cmsUtl_strcmp(currObj->IPInterfaceIPAddress, newObj->IPInterfaceIPAddress) ||
              cmsUtl_strcmp(currObj->IPInterfaceSubnetMask, newObj->IPInterfaceSubnetMask)))
         {
            /*
             * If the IP address changed, we have to re-install all the NAT rules
             * for the WAN to LAN address masquerading.
             */
            rutLan_reconfigNatForAddressChange(currObj->IPInterfaceIPAddress, currObj->IPInterfaceSubnetMask,
                                               newObj->IPInterfaceIPAddress, newObj->IPInterfaceSubnetMask);
   
            /* let dhcpd know of updated info */
            rutLan_updateDhcpd();
            
#ifdef SUPPORT_ADVANCED_DMZ
            char localLanIp[CMS_IPADDR_LENGTH];
            UBOOL8 found = FALSE;
            InstanceIdStack ppp_iidStack = EMPTY_INSTANCE_ID_STACK;
            WanPppConnObject *pppConn=NULL;		
            	                      
            if(!cmsUtl_strcmp(newObj->X_BROADCOM_COM_IfName, "br1") && (rutNtwk_isAdvancedDmzEnabled() == TRUE))
            {
                while ((!found) && ((cmsObj_getNext(MDMOID_WAN_PPP_CONN, &ppp_iidStack, (void **) &pppConn)) == CMSRET_SUCCESS))
                {
                    if ((!cmsUtl_strcmp(pppConn->connectionType, MDMVS_IP_ROUTED)))
                        found = TRUE;      			
                    else
                        cmsObj_free((void **) &pppConn);            
                }
            
                if (!found)
                    cmsLog_debug("No pppoe connection found!");
                else
                {	
                    cmsLog_debug("pppConn->externalIPAddress = %s\n", pppConn->externalIPAddress);
                    rut_getIfAddr("br0", localLanIp);
                    rutIpt_delAdvancedDmzIpRules(localLanIp, pppConn->externalIPAddress, currObj->IPInterfaceIPAddress, currObj->IPInterfaceSubnetMask);
                    rutIpt_addAdvancedDmzIpRules(localLanIp, pppConn->externalIPAddress, newObj->IPInterfaceIPAddress, newObj->IPInterfaceSubnetMask);				
                    cmsObj_free((void **) &pppConn);	
                }
            }		
#endif
         }             
         
      }

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
      if (rutLan_isIpv6IntfChanged(newObj, currObj) ||
         ADD_NEW(newObj, currObj))
      {
         /* The LAN side firewall needs to be applied if
         * 1). there is a change in interface ie. rutLan_isIpv4IntfChanged is TRUE
         * 2).  the device is in the reboot process ADD_NEW(newObj, currObj) is TRUE
         */
         if (FirewallOn)
         {
            cmsLog_notice("Starting IPv6 firewall on %s", newObj->X_BROADCOM_COM_IfName);
            rutIpt_insertIpModules6();
            rutIpt_initFirewall(PF_INET6, newObj->X_BROADCOM_COM_IfName);
            rutIpt_initFirewallExceptions(newObj->X_BROADCOM_COM_IfName);
         }
         else if (FirewallOff)
         {
            cmsLog_notice("remove IPv6 firewall on %s", newObj->X_BROADCOM_COM_IfName);
            rutIpt_removeInterfaceIptableRules(newObj->X_BROADCOM_COM_IfName, FALSE);
         }
      }
#endif

#if defined(DMP_X_BROADCOM_COM_IGMP_1) || defined(DMP_X_BROADCOM_COM_MLD_1)
      rutMulti_reloadMcpd();
#endif
   }

   if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if (DELETE_EXISTING(newObj, currObj))
      {
         rut_modifyNumLanIpIntf(iidStack, -1);
      }

      rutLan_disableBridge(currObj->X_BROADCOM_COM_IfName);

#ifdef DMP_X_BROADCOM_COM_SECURITY_1
      if (currObj->X_BROADCOM_COM_FirewallEnabled)
      {
         cmsLog_notice("remove firewall on %s", currObj->X_BROADCOM_COM_IfName);
         rutIpt_removeInterfaceIptableRules(currObj->X_BROADCOM_COM_IfName, TRUE);
         /* Only take action if IPv6 is enabled. */
         rutIpt_removeInterfaceIptableRules(currObj->X_BROADCOM_COM_IfName, FALSE);
      }
#endif

      /* let dhcpd know of updated info */
      rutLan_updateDhcpd();
   }

   return ret;
}


CmsRet rcl_lanEthIntfObject(_LanEthIntfObject *newObj,
                            const _LanEthIntfObject *currObj,
                            const InstanceIdStack *iidStack,
                            char **errorParam __attribute__((unused)),
                            CmsRet *errorCode __attribute__((unused)))
{
   char bridgeIfName[BUFLEN_32]={0};
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   /* disable eth device */
   if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if ((ret = rutLan_getParentBridgeIfName(MDMOID_LAN_ETH_INTF, iidStack, bridgeIfName)) == CMSRET_SUCCESS)
      {
#if defined(DMP_X_BROADCOM_COM_EPON_1) && (defined(EPON_SFU) || defined(EPON_SBU)) && !defined(SUPPORT_LANVLAN)
        rutLan_configShadowVlanInterface(newObj->X_BROADCOM_COM_IfName, bridgeIfName, FALSE);
#else
         rutLan_disableInterface(currObj->X_BROADCOM_COM_IfName);
         
         /* If this ethernet interface is not WANONLY, it's part of a bridge and  
          * should be removed from bridge.
          */        
         if (cmsUtl_strcmp(currObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANONLY))
         {

            rutLan_removeInterfaceFromBridge(currObj->X_BROADCOM_COM_IfName, bridgeIfName);
         }    
         
#endif /* DMP_X_BROADCOM_COM_EPON_1 */
      }
   }

   if (ADD_NEW(newObj, currObj))
   {
      /*
       * add new object instance usually will succeed, so just update
       * the count at the beginning of the function.
       */
       rut_modifyNumEthIntf(iidStack, 1);
#ifdef SUPPORT_LANVLAN
       if (newObj->X_BROADCOM_COM_IfName)
       {
#if defined(DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1) 
          OmciEthPortType type;

          if (rutOmci_getEthPortTypeByName(newObj->X_BROADCOM_COM_IfName, &type) != CMSRET_SUCCESS ||
              type != OMCI_ETH_PORT_TYPE_ONT)
#endif
          rutLan_AddDefaultLanVlanInterface(newObj->X_BROADCOM_COM_IfName);
       }
#endif	   
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      /*
       * This is a delete instance, update count.  Deletes always update the MDM,
       * even if the RCL handler function could not do the action.
       */
      rut_modifyNumEthIntf(iidStack, -1);
#ifdef SUPPORT_LANVLAN
      rutLan_RemoveDefaultLanVlanInterface(currObj->X_BROADCOM_COM_IfName);  
#endif	   
   }

#ifdef SUPPORT_LANVLAN
   /* If interface name is changed then update vlan interface,
      this can happen when doing interface grouping */
   if (newObj && currObj && 
      cmsUtl_strcmp(currObj->X_BROADCOM_COM_IfName, newObj->X_BROADCOM_COM_IfName))
   {
      if (currObj->X_BROADCOM_COM_IfName)
         rutLan_RemoveDefaultLanVlanInterface(currObj->X_BROADCOM_COM_IfName);
      if (newObj->X_BROADCOM_COM_IfName)
         rutLan_AddDefaultLanVlanInterface(newObj->X_BROADCOM_COM_IfName);
   }
#endif

   /* enable eth device */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      if ((ret = rutLan_getParentBridgeIfName(MDMOID_LAN_ETH_INTF, iidStack, bridgeIfName)) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANONLY))
         {
            rutQos_tmPortInit(newObj->X_BROADCOM_COM_IfName, FALSE);
         }
         /*
          * Special case check: on bootup, if virtual ports is enabled,
          * and this interface is the base interface of the virtual ports,
          * do not add to the bridge.
          */
         if (ADD_NEW(newObj, currObj) &&
             rutEsw_isVirtualPortsEnabled() &&
             rutEsw_isEthernetSwitchIfNameMatch(newObj->X_BROADCOM_COM_IfName))
         {
            rutLan_enableInterface(newObj->X_BROADCOM_COM_IfName);
         }
         else
         {
#if defined(DMP_X_BROADCOM_COM_EPON_1) && (defined(EPON_SFU) || defined(EPON_SBU)) && !defined(SUPPORT_LANVLAN)
             rutLan_configShadowVlanInterface(newObj->X_BROADCOM_COM_IfName, bridgeIfName, TRUE);
             {
                 char vlanIfName[CMS_IFNAME_LENGTH];
                 sprintf(vlanIfName, "%s.v0", newObj->X_BROADCOM_COM_IfName);
                 rutLan_configEthIfMcastRule(newObj->X_BROADCOM_COM_IfName, vlanIfName);
             }
#else
             UBOOL8 isWanIntf = FALSE;
             rutLan_addInterfaceToBridge(newObj->X_BROADCOM_COM_IfName, isWanIntf, bridgeIfName);
#endif /* DMP_X_BROADCOM_COM_EPON_1 */

         }
#if defined(DMP_ETHERNETWAN_1)
#ifdef BCM_PON
         /* Temporary workaround for Oren EMAC5 which does not operate as a LAN port, is first created as an MDM LAN
            port object, until moved to a WAN port object (bcmenet configuration is always as a WAN device) */
         if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANONLY))
#endif
         {
             /* make sure that interface is configured as LAN */
             if ((ret = rutEth_setSwitchWanPort(newObj->X_BROADCOM_COM_IfName, FALSE)) != CMSRET_SUCCESS)
             {
                return ret;
             }
         }
#endif
         rutQos_qMgmtQueueReconfig(newObj->X_BROADCOM_COM_IfName, TRUE);
         rutQos_doDefaultPolicy();
         rutQos_tmPortShaperCfg(newObj->X_BROADCOM_COM_IfName,
                                newObj->X_BROADCOM_COM_ShapingRate,
                                newObj->X_BROADCOM_COM_ShapingBurstSize,
                                newObj->status,
                                FALSE);
      }
   }

#ifdef SUPPORT_LANVLAN
  if (ADD_NEW(newObj, currObj) || DELETE_EXISTING(newObj, currObj) ||
            (currObj->X_BROADCOM_COM_VLAN_Enable != newObj->X_BROADCOM_COM_VLAN_Enable || 
            cmsUtl_strcmp(currObj->X_BROADCOM_COM_VLAN_TagList, newObj->X_BROADCOM_COM_VLAN_TagList)))
   {
       if (currObj && currObj->X_BROADCOM_COM_VLAN_Enable)
       {
          rutLan_RemoveLanVlanInterface(currObj->X_BROADCOM_COM_IfName, currObj->X_BROADCOM_COM_VLAN_TagList);
       }

       if (newObj && newObj->X_BROADCOM_COM_VLAN_Enable)
       {
          rutLan_CreateLanVlanIf(newObj->X_BROADCOM_COM_IfName, newObj->X_BROADCOM_COM_VLAN_TagList, iidStack, ADD_NEW(newObj, currObj));
          {
             char vlanIfName[CMS_IFNAME_LENGTH];
             sprintf(vlanIfName, "%s.v0", newObj->X_BROADCOM_COM_IfName);
             rutLan_configEthIfMcastRule(newObj->X_BROADCOM_COM_IfName, vlanIfName);
          }
       }
   }
#endif

   /* QoS port shaping */
   if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj)
      && (currObj->X_BROADCOM_COM_ShapingRate != newObj->X_BROADCOM_COM_ShapingRate))
   {
      rutQos_tmPortShaperCfg(newObj->X_BROADCOM_COM_IfName,
                             newObj->X_BROADCOM_COM_ShapingRate,
                             newObj->X_BROADCOM_COM_ShapingBurstSize,
                             newObj->status,
                             FALSE);
   }

   return ret;
}


CmsRet rcl_lanEthIntfStatsObject( _LanEthIntfStatsObject *newObj __attribute__((unused)),
                const _LanEthIntfStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}


#ifdef DMP_USBLAN_1
CmsRet rcl_lanUsbIntfObject(_LanUsbIntfObject *newObj, 
                                  const LanUsbIntfObject *currObj,
                                  const InstanceIdStack *iidStack, 
                                  char **errorParam __attribute__((unused)),
                                  CmsRet *errorCode __attribute__((unused)))
{
   char bridgeIfName[BUFLEN_32]={0};
   UBOOL8 isWanIntf = FALSE;
   CmsRet ret = CMSRET_SUCCESS;
   
   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   if (ADD_NEW(newObj, currObj))
   {
      /*
       * add new object instance usually will succeed, so just update
       * the count at the beginning of the function.
       */
       rut_modifyNumUsbIntf(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      /*
       * This is a delete instance, update count.  Deletes always update the MDM,
       * even if the RCL handler function could not do the action.
       */
      rut_modifyNumUsbIntf(iidStack, -1);
   }

   /* enable usb device */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      if ((ret = rutLan_getParentBridgeIfName(MDMOID_LAN_USB_INTF, iidStack, bridgeIfName)) == CMSRET_SUCCESS)
      {
         rutQos_tmPortInit(newObj->X_BROADCOM_COM_IfName, FALSE);
         rutLan_enableInterface(newObj->X_BROADCOM_COM_IfName);
         rutLan_addInterfaceToBridge(newObj->X_BROADCOM_COM_IfName, isWanIntf, bridgeIfName);
      }
   }

   /* disable usb device */
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if ((ret = rutLan_getParentBridgeIfName(MDMOID_LAN_USB_INTF, iidStack, bridgeIfName)) == CMSRET_SUCCESS)
      {
         rutLan_disableInterface(currObj->X_BROADCOM_COM_IfName);
         rutLan_removeInterfaceFromBridge(currObj->X_BROADCOM_COM_IfName, bridgeIfName);
      }
   }

   return ret;
}

CmsRet rcl_lanUsbIntfStatsObject( _LanUsbIntfStatsObject *newObj __attribute__((unused)),
                const _LanUsbIntfStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

#endif /* DMP_USBLAN_1 */



#ifdef DMP_X_BROADCOM_COM_EPON_1

CmsRet rcl_lanEponIntfObject( _LanEponIntfObject *newObj,
                const _LanEponIntfObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   char bridgeIfName[BUFLEN_32]={0};
   CmsRet ret = CMSRET_SUCCESS;
   unsigned int fe_ports, ge_ports;

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   if (ADD_NEW(newObj, currObj))
   {
      /*
       * add new object instance usually will succeed, so just update
       * the count at the beginning of the function.
       */
       rut_modifyNumEponIntf(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      /*
       * This is a delete instance, update count.  Deletes always update the MDM,
       * even if the RCL handler function could not do the action.
       */
      rut_modifyNumEponIntf(iidStack, -1);
   }

   ret = devCtl_getNumFePorts(&fe_ports);
   if (ret != CMSRET_SUCCESS) {
        printf("ERROR: eponapp not able to get the Number of EPON FE ports. Assuming 4 \n");
        fe_ports = 4;
   }
   ret = devCtl_getNumGePorts(&ge_ports);

   if (ret != CMSRET_SUCCESS) {
        printf("ERROR: eponapp not able to get the Number of EPON GE ports. Assuming 0 \n");
        ge_ports = 0;
   }

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      if ((ret = rutLan_getParentBridgeIfName(MDMOID_LAN_EPON_INTF, iidStack, bridgeIfName)) == CMSRET_SUCCESS)
      {
         rutQos_tmPortInit(newObj->ifName, FALSE);
#if defined(EPON_SFU) && !defined(SUPPORT_LANVLAN)
         rutLan_configShadowVlanInterface(newObj->ifName, bridgeIfName, TRUE);
#else
         UBOOL8 isWanIntf = FALSE;
         rutLan_enableInterface(newObj->ifName);
         rutLan_addInterfaceToBridge(newObj->ifName, isWanIntf, bridgeIfName);
#endif
      }
   }

   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if ((ret = rutLan_getParentBridgeIfName(MDMOID_LAN_EPON_INTF, iidStack, bridgeIfName)) == CMSRET_SUCCESS)
      {
#if defined(EPON_SFU) && !defined(SUPPORT_LANVLAN)
         rutLan_configShadowVlanInterface(newObj->ifName, bridgeIfName, FALSE);
#else
         rutLan_disableInterface(currObj->ifName);
         rutLan_removeInterfaceFromBridge(currObj->ifName, bridgeIfName);
#endif
      }
   }

   return ret;

}


CmsRet rcl_lanEponIntfStatsObject( _LanEponIntfStatsObject *newObj __attribute__((unused)),
                const _LanEponIntfStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#endif  /* DMP_X_BROADCOM_COM_EPON_1 */


CmsRet rcl_dHCPConditionalServingObject( _DHCPConditionalServingObject *newObj,
                            const _DHCPConditionalServingObject *currObj,
                            const InstanceIdStack *iidStack,
                            char **errorParam __attribute__((unused)),
                            CmsRet *errorCode __attribute__((unused)))
{

   CmsRet ret = CMSRET_SUCCESS;

#ifdef SUPPORT_UDHCP

   UBOOL8 found=FALSE;
   _DHCPConditionalServingObject *obj = NULL;
   InstanceIdStack searchIidStack = EMPTY_INSTANCE_ID_STACK;
   
   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   /* enable a static lease */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      if (rutWan_IsPPPIpExtension())
      {
         return CMSRET_INVALID_ARGUMENTS;
      }

      if (!cmsUtl_isValidIpAddress(AF_INET, newObj->reservedAddresses) || !cmsUtl_strcmp(newObj->reservedAddresses, "0.0.0.0"))
      {
         cmsLog_error("Invalid reservedAddresses IP address");
         return CMSRET_INVALID_ARGUMENTS;		
      }

      if (cmsUtl_isValidMacAddress(newObj->chaddr) == FALSE)
      {
         cmsLog_error("Invalid chaddr MAC address");
         return CMSRET_INVALID_ARGUMENTS;		
      }

      while (!found &&
             (cmsObj_getNext(MDMOID_DHCP_CONDITIONAL_SERVING, &searchIidStack, (void **) &obj) == CMSRET_SUCCESS))
      {
         /*
          * Check for duplicate entry.
          * When we iterate through the objects using cmsObj_getNext, we will
          * also get a copy of the new object that is being set.  Only do the
          * duplicate check if the object is not the same as the new one being set.
          */
         if (cmsMdm_compareIidStacks(&searchIidStack, iidStack))
         {
            if((obj->enable) && 
               (!cmsUtl_strcmp(newObj->chaddr, obj->chaddr) || !cmsUtl_strcmp(newObj->reservedAddresses, obj->reservedAddresses)))
            {
               found = TRUE;
            }
         }

         cmsObj_free((void **) &obj);
      }

      if (found)
      {
         cmsLog_error("Either MAC or IP address already has been used for static IP lease.");
         return CMSRET_INVALID_ARGUMENTS;
      }

      rutLan_updateDhcpd();
   }  /* end of enable new or enable existing */ 

   /* delete or disable existing */
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("delete or disable existing object");

      /*
       * The set on the object from dal_lan could have failed.
       * In that case, chaddr would be NULL, so don't bother
       * deleting it from the udhcpd.conf, since it was not put there
       * in the first place.
       */
      if(currObj->chaddr != NULL)
      {
         rutLan_updateDhcpd();
      }
   }

   cmsLog_debug("done, ret=%d", ret);

#endif /* SUPPORT_UDHCP */

   return ret;

}

#ifdef NOT_SUPPORTED
CmsRet rcl_dHCPOptionObject( _DHCPOptionObject *newObj __attribute((unused)),
                            const _DHCPOptionObject *currObj __attribute((unused)),
                            const InstanceIdStack *iidStack __attribute((unused)),
                            char **errorParam __attribute__((unused)),
                            CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif


CmsRet rcl_lanHostsObject( _LanHostsObject *newObj __attribute__((unused)),
                const _LanHostsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}


CmsRet rcl_lanHostEntryObject( _LanHostEntryObject *newObj,
                const _LanHostEntryObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   UBOOL8 updateHosts=FALSE;

   if (ADD_NEW(newObj, currObj))
   {
      rut_modifyNumLanHosts(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rut_modifyNumLanHosts(iidStack, -1);
      updateHosts = TRUE;
   }

   /*
    * ssk will first create an object, which will have all default values.
    * Then it will modify the object, filling in the IPAddr and hostname.
    * When the modified object entry has interesting data, we want to write
    * out the hosts file and tell dnsproxy to reload it.
    */
   if (newObj != NULL && currObj != NULL && newObj->IPAddress != NULL && newObj->hostName != NULL &&
       (cmsUtl_strcmp(newObj->hostName, currObj->hostName) || cmsUtl_strcmp(newObj->IPAddress, currObj->IPAddress)))
   {
      updateHosts = TRUE;
   }

   if (updateHosts)
   {
#ifdef DESKTOP_LINUX
     cmsLog_debug("skipping rutSys_createHostFile and rutDpx_updateDnsproxy for DESKTOP");
#else
      rutSys_createHostsFile();
      rutDpx_updateDnsproxy();
#endif
   }

   return CMSRET_SUCCESS;
}


#endif  /*  DMP_BASELINE_1 */
