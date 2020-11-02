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

#include "cms_core.h"
#include "cms_util.h"
#include "cms_eid.h"
#include "cms_dal.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_pmap.h"
#include "rut_lan.h"
#include "rut_iptables.h"
#include "rut_dnsproxy.h"
#include "rut_route.h"
#include "rut_multicast.h"
#include "rut_upnp.h"


CmsRet rutPMap_moveLanInterface(const char *filterInterface, SINT32 srcBridgeRef, SINT32 destBridgeRef)
{
   UINT32 key;
   char ifName[BUFLEN_32];
   char srcBridgeIfName[BUFLEN_32], destBridgeIfName[BUFLEN_32];
   char availableInterfaceReference[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   InstanceIdStack iidStack;
   _L2BridgingEntryObject *srcBridgeObj=NULL;
   _L2BridgingEntryObject *destBridgeObj=NULL;
   _L2BridgingIntfObject *availIntfObj=NULL;
   CmsRet ret;
   
   if ((ret = rutPMap_getBridgeByKey((UINT32) srcBridgeRef, &iidStack, &srcBridgeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Error retrieving the src bridge ref %d", srcBridgeRef);
      return ret;
   }
   
   if ((ret = rutPMap_getBridgeByKey((UINT32) destBridgeRef, &iidStack, &destBridgeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Error retrieving the dest bridge ref %d", destBridgeRef);
      cmsObj_free((void **) &srcBridgeObj);
      return ret;
   }
   
   if ((ret = cmsUtl_strtoul(filterInterface, NULL, 0, &key)) != CMSRET_SUCCESS)
   {
      cmsLog_error("not a valid LAN filterInterface value %s", filterInterface);
      cmsObj_free((void **) &srcBridgeObj);
      cmsObj_free((void **) &destBridgeObj);
      return ret;
   }
   
   if ((ret = rutPMap_getAvailableInterfaceByKey(key, &iidStack, &availIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find available interface obj with key %u", key);
      cmsObj_free((void **) &srcBridgeObj);
      cmsObj_free((void **) &destBridgeObj);
      return ret;
   }
   
   if ((ret = rutPMap_availableInterfaceReferenceToIfName(availIntfObj->interfaceReference, ifName)) != CMSRET_SUCCESS)
   {
      cmsObj_free((void **) &srcBridgeObj);
      cmsObj_free((void **) &destBridgeObj);
      cmsObj_free((void **) &availIntfObj);
      cmsLog_error("could not convert %s to ifName", availIntfObj->interfaceReference);
      return ret;
   }
   
   sprintf(srcBridgeIfName, "br%d", srcBridgeObj->bridgeKey);
   sprintf(destBridgeIfName, "br%d", destBridgeObj->bridgeKey);
   cmsLog_debug("moving %s from %s to %s", ifName, srcBridgeIfName, destBridgeIfName);
   
#ifdef DMP_BRIDGING_1
   /* miwang: this entire function should be surrounded by TR98 BRIDGING, right?  Revisit. */
   if (cmsUtl_strncmp(ifName, "eth", 3) == 0)
   {
      ret = rutLan_moveEthInterface(ifName, srcBridgeIfName, destBridgeIfName);
   }
#ifdef DMP_USBLAN_1
   else if (cmsUtl_strncmp(ifName, "usb", 3) == 0)
   {
      ret = rutLan_moveUsbInterface(ifName, srcBridgeIfName, destBridgeIfName);
   }
#endif
#ifdef BRCM_WLAN
   else if (cmsUtl_strncmp(ifName, "wl", 2) == 0)
   {
      ret = rutLan_moveWlanInterface(ifName, srcBridgeIfName, destBridgeIfName);
   }
#endif

#ifdef SUPPORT_MOCA
   else if (cmsUtl_strncmp(ifName, "moca", 4) == 0)
   {
      ret = rutLan_moveMocaInterface(ifName, srcBridgeIfName, destBridgeIfName);
   }
#endif

   else
   {
      cmsLog_error("moving of interface %s not supported (yet?)", ifName);
      ret = CMSRET_INVALID_ARGUMENTS;
   }
#endif /* DMP_BRIDGING_1 */
   
   /*
    * finally, need to update the AvailableInterface.interfaceReference because
    * the LAN eth/usb/wlan interface moved.
    */
   if (ret == CMSRET_SUCCESS)
   {
      if ((ret = rutPMap_lanIfNameToAvailableInterfaceReference(ifName, availableInterfaceReference)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not find %s after move", ifName);
          /* Its too hard to undo now, this should never happen anyways. */
      }
      else
      {
         /*
          * very anoying, the TR98 spec says the fullname does not end in dot,
          * but our internal conversion routines put in the final dot, so strip
          * it out before putting into the MDM.
          */
         availableInterfaceReference[strlen(availableInterfaceReference)-1] = '\0';   
         CMSMEM_REPLACE_STRING_FLAGS(availIntfObj->interfaceReference, availableInterfaceReference, mdmLibCtx.allocFlags);
         if ((ret = cmsObj_set(availIntfObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("set of new availinterfacereference failed, ret=%d", ret);
            /* its too hard to undo now, this should never happen anyways. */
         }
      }
   } 


   cmsObj_free((void **) &srcBridgeObj);
   cmsObj_free((void **) &destBridgeObj);
   cmsObj_free((void **) &availIntfObj);
   
   return ret;
}


CmsRet rutPMap_associateWanIntfWithBridge(const char *filterInterface, SINT32 bridgeRef)
{
   _L2BridgingEntryObject *bridgeObj = NULL;
   L2BridgingIntfObject *availIntfObj = NULL;
   WanPppConnObject *wanPppConnObj;
   WanIpConnObject *wanIpConnObj;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char bridgeIfName[BUFLEN_32];
   char IntfGroupName[BUFLEN_64];
   void *obj=NULL;
   MdmObjectId oid;
   UBOOL8 isWanIntf=TRUE;
   UINT32 availIntfKey;
   CmsRet ret;
   

   cmsLog_debug("Entered: filterInterface=%s, bridgeRef=%d", filterInterface, bridgeRef);

   /*
    * First get the (LAN) bridge interface name.
    */
   if ((ret = rutPMap_getBridgeByKey((UINT32) bridgeRef, &iidStack, &bridgeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Error retrieving the bridge ref %d", bridgeRef);
      return ret;
   }

   sprintf(bridgeIfName, "br%d", bridgeObj->bridgeKey);
   strncpy(IntfGroupName, bridgeObj->bridgeName, sizeof(IntfGroupName));
   cmsObj_free((void **) &bridgeObj);
   
   /*
    * Now get the WAN interface info.
    */
   availIntfKey = (UINT32) atoi(filterInterface);
   INIT_INSTANCE_ID_STACK(&iidStack);
   ret = rutPMap_getAvailableInterfaceByKey(availIntfKey, &iidStack, &availIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find avail intf object %s, ret=%d", filterInterface, ret);
      return ret;
   }
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   ret = rutPMap_availableInterfaceReferenceToMdmObject(availIntfObj->interfaceReference, &iidStack, &obj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find network obj of %s, ret=%d", availIntfObj->interfaceReference, ret);
      cmsObj_free((void **) &availIntfObj);
      return ret;
   }
   
   cmsObj_free((void **) &availIntfObj);

   

   /*
    * Do different actions based on the type of WAN interface that
    * we are adding to the port group/LAN bridge/LAN subnet.
    */   
   oid = GET_MDM_OBJECT_ID(obj);
   wanIpConnObj = (WanIpConnObject *) obj;
   wanPppConnObj = (WanPppConnObject *) obj;
  
   if (oid == MDMOID_WAN_IP_CONN && !cmsUtl_strcmp(wanIpConnObj->connectionType, MDMVS_IP_BRIDGED))
   {
      /*
       * associate bridge WAN interface to LAN bridge, this is the original
       * portmapping case.
       */
      if (cmsUtl_strcmp(wanIpConnObj->connectionStatus, MDMVS_CONNECTED) == 0)
      {
         rutLan_addInterfaceToBridge(wanIpConnObj->X_BROADCOM_COM_IfName, isWanIntf, bridgeIfName);
      }
   }
   else if (oid == MDMOID_WAN_PPP_CONN && !cmsUtl_strcmp(wanPppConnObj->connectionType, MDMVS_PPPOE_BRIDGED))
   {
      /*
       * Associate bridge WAN interface to bridge
       */
      if (cmsUtl_strcmp(wanPppConnObj->connectionStatus, MDMVS_CONNECTED) == 0)
      {
         cmsLog_debug("PppoE_bridged, Binding %s to %s", wanPppConnObj->X_BROADCOM_COM_IfName, bridgeIfName);
         rutLan_addInterfaceToBridge(wanPppConnObj->X_BROADCOM_COM_IfName, isWanIntf, bridgeIfName);
      }
   }
   else if (oid == MDMOID_WAN_IP_CONN && !cmsUtl_strcmp(wanIpConnObj->connectionType, MDMVS_IP_ROUTED))
   {

      /*   
      * The policy routing stuff will be set here  for live interface group creation
      * when the WAN interface is already up. 
      */
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */    
      if ((cmsUtl_strcmp(wanIpConnObj->connectionStatus, MDMVS_CONNECTED) == 0) &&
         (rutPMap_isWanUsedForIntfGroup(wanIpConnObj->X_BROADCOM_COM_IfName) == TRUE))
      {
         UBOOL8 add=TRUE;
         
         if ((ret = rutPMap_doPoliceRoutingOnWanIfc(add,
                                                    wanIpConnObj->X_BROADCOM_COM_IfName, 
                                                    wanIpConnObj->defaultGateway,
                                                    bridgeIfName,
                                                    IntfGroupName)) == CMSRET_SUCCESS)
         {
            /* this is for live creation.  TODO: if NAT enable, need to remove the old NAT rules */
            rutIpt_initNatForIntfGroup(wanIpConnObj->X_BROADCOM_COM_IfName, bridgeIfName, wanIpConnObj->X_BROADCOM_COM_FullconeNATEnabled);
   
            /* When adding the interface group, need to update the dns info in the var/dnsinfo.conf */
            rutDpx_updateDnsproxy();
         }            
      }
#endif      

   }
   else if (oid == MDMOID_WAN_PPP_CONN  && !cmsUtl_strcmp(wanPppConnObj->connectionType, MDMVS_IP_ROUTED))
   {

      /*   
      * The policy routing stuff will be set here  for live interface group creation
      * when the WAN interface is already up. 
      */
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */    
      if ((cmsUtl_strcmp(wanPppConnObj->connectionStatus, MDMVS_CONNECTED) == 0) &&
         (rutPMap_isWanUsedForIntfGroup(wanPppConnObj->X_BROADCOM_COM_IfName) == TRUE))
      {
         UBOOL8 add=TRUE;
         
         if ((ret = rutPMap_doPoliceRoutingOnWanIfc(add,
                                                    wanPppConnObj->X_BROADCOM_COM_IfName, 
                                                    wanPppConnObj->X_BROADCOM_COM_DefaultGateway,
                                                    bridgeIfName,
                                                    IntfGroupName)) == CMSRET_SUCCESS)
         {
            /* this is for live creation.  TODO: if NAT enable, need to remove the old NAT rules */
            rutIpt_initNatForIntfGroup(wanPppConnObj->X_BROADCOM_COM_IfName, bridgeIfName, wanPppConnObj->X_BROADCOM_COM_FullconeNATEnabled);
            
            /* When adding the interface group, need to update the dns info in the var/dnsinfo.conf */
            rutDpx_updateDnsproxy();
         }            
      }
#endif      

   }
   
      
   cmsObj_free(&obj);   

   /*update latest information to udhcpd.conf and inform dhcpd*/
   rutLan_updateDhcpd();

   rutMulti_reloadMcpd();

   cmsLog_debug("done, ret=%d", ret);
   return ret;
}


CmsRet rutPMap_disassociateWanIntfFromBridge(const char *filterInterface, SINT32 bridgeRef, UBOOL8 isEditWanFilter __attribute__((unused)))
{
   _L2BridgingEntryObject *bridgeObj = NULL;
   InstanceIdStack       iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   L2BridgingIntfObject *availIntfObj = NULL;
   WanPppConnObject *wanPppConnObj;
   WanIpConnObject *wanIpConnObj;
   char bridgeIfName[BUFLEN_32];
   void *obj=NULL;
   MdmObjectId oid;
   UINT32 availIntfKey;
   UBOOL8 prevHideObjectsPendingDelete;

   cmsLog_debug("filterInterface=%s bridgeRef=%d", filterInterface, bridgeRef);

   if ((ret = rutPMap_getBridgeByKey((UINT32) bridgeRef, &iidStack, &bridgeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Error retrieving the bridge ref %d", bridgeRef);
      return ret;
   }


   /*
    * Now get the WAN interface info.
    */
   availIntfKey = (UINT32) atoi(filterInterface);
   INIT_INSTANCE_ID_STACK(&iidStack);
   ret = rutPMap_getAvailableInterfaceByKey(availIntfKey, &iidStack, &availIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find avail intf object %s, ret=%d", filterInterface, ret);
      return ret;
   }
   

   /*
    * This function is typically called when the WANIPConnection object or 
    * WANPPPConnection object is getting deleted.
    * The rutPMap_availableInterfaceReferenceToMdmObject function below
    * will use cmsObj_getNextInSubtree to find the WANIPConnection
    * or WANPPPConnection object below this interfaceReference.
    * The problem is the MDM will normally hide this object because it is
    * pending a delete.  Set the special mdmLibCtx.hideObjectsPendingDelete
    * variable to FALSE to override this behavior.
    */
   prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
   mdmLibCtx.hideObjectsPendingDelete = FALSE;

   INIT_INSTANCE_ID_STACK(&iidStack);
   ret = rutPMap_availableInterfaceReferenceToMdmObject(availIntfObj->interfaceReference, &iidStack, &obj);

   mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find network obj of %s, ret=%d", availIntfObj->interfaceReference, ret);
      cmsObj_free((void **) &availIntfObj);
      return ret;
   }
   
   cmsObj_free((void **) &availIntfObj);
   
   /*
    * Do different actions based on the type of WAN interface that
    * we are adding to the port group/LAN bridge/LAN subnet.
    */   
   oid = GET_MDM_OBJECT_ID(obj);
   wanIpConnObj = (WanIpConnObject *) obj;
   wanPppConnObj = (WanPppConnObject *) obj;
   sprintf(bridgeIfName, "br%d", bridgeObj->bridgeKey);

   if (oid == MDMOID_WAN_IP_CONN && !cmsUtl_strcmp(wanIpConnObj->connectionType, MDMVS_IP_BRIDGED))
   {
      /*dissociate (remove) the WAN intf from the default bridge */
      rutLan_removeInterfaceFromBridge(wanIpConnObj->X_BROADCOM_COM_IfName, bridgeIfName);
   }
   else if (oid == MDMOID_WAN_PPP_CONN && !cmsUtl_strcmp(wanPppConnObj->connectionType, MDMVS_PPPOE_BRIDGED))
   {
      /*dissociate (remove) the WAN intf from the the bridge */
      cmsLog_debug("PPPoE_bridged, dissociate %s from %s", wanPppConnObj->X_BROADCOM_COM_IfName, bridgeIfName);
      rutLan_removeInterfaceFromBridge(wanPppConnObj->X_BROADCOM_COM_IfName, bridgeIfName);
   }
   else if (oid == MDMOID_WAN_IP_CONN)
   {
      /* disassociate IPoE from LAN bridge, using policy routing */
      
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */
      UBOOL8 isWanUsedForIntfGroup;

      /*   
      * The policy routing stuff need to be deleted here for live interface group remove
      * only when the wan intf is already removed from interface group (rutPMap_isWanUsedForIntfGroup is FALSE)
      */
      prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      isWanUsedForIntfGroup = rutPMap_isWanUsedForIntfGroup(wanIpConnObj->X_BROADCOM_COM_IfName);

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

      cmsLog_debug("ifName %s in the intfGroup = %d, connectionStatus=%s",
                   wanIpConnObj->X_BROADCOM_COM_IfName,
                   isWanUsedForIntfGroup,
                   wanIpConnObj->connectionStatus);
      
      if ((cmsUtl_strcmp(wanIpConnObj->connectionStatus, MDMVS_CONNECTED) == 0) &&
          (isWanUsedForIntfGroup == TRUE))
      {
         UBOOL8 add=FALSE;
         
         rutPMap_doPoliceRoutingOnWanIfc(add,
                                         wanIpConnObj->X_BROADCOM_COM_IfName, 
                                         wanIpConnObj->defaultGateway,
                                         bridgeIfName,
                                         bridgeObj->bridgeName);
         
         /* remove all iptable rules associated with this interface */
         rutIpt_removeInterfaceIptableRules(wanIpConnObj->X_BROADCOM_COM_IfName, TRUE);
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
//         rutIpt_removeInterfaceIptableRules(wanIpConnObj->X_BROADCOM_COM_IfName, FALSE);  //TODO: what should the action be for IPv6 here?
#endif

         /* remove the dns forwarding the PREROUTING chain for
          * non default bridge (br1..)
          */
         rutIpt_removeBridgeIfNameIptableRules(bridgeIfName);
   

         /* TODO: remove igmp and some other rules ? */
         /* We are redoing initNAT and initFirewall on these objects based on the default (br0) WAN connection rules */
         cmsLog_debug("re-enabling NAT and firewall for deleted object? %s", wanIpConnObj->X_BROADCOM_COM_IfName);
         if (wanIpConnObj->NATEnabled)
         {
            rutIpt_initNat(wanIpConnObj->X_BROADCOM_COM_IfName, wanIpConnObj->X_BROADCOM_COM_FullconeNATEnabled);
         }
         if (wanIpConnObj->X_BROADCOM_COM_FirewallEnabled)
         {
            rutIpt_initFirewall(PF_INET, wanIpConnObj->X_BROADCOM_COM_IfName);
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
            if (wanIpConnObj->X_BROADCOM_COM_IPv6Enabled)
            {
               rutIpt_initFirewall(PF_INET6, wanIpConnObj->X_BROADCOM_COM_IfName);
            }
#endif
            rutIpt_initFirewallExceptions(wanIpConnObj->X_BROADCOM_COM_IfName);
         }
      }
      else if ((isWanUsedForIntfGroup == FALSE) && (isEditWanFilter == TRUE))/*restore iptable rules from interface group*/
      {
         /* remove all iptable rules associated with this interface */
         cmsLog_debug("remove IptableRules with Interface: %s, %s", wanIpConnObj->X_BROADCOM_COM_IfName, bridgeIfName);
         rutIpt_removeInterfaceIptableRules(wanIpConnObj->X_BROADCOM_COM_IfName, TRUE);
         rutIpt_removeInterfaceIptableRules(bridgeIfName, TRUE);
         /*re-enable NAT*/
         if (wanIpConnObj->NATEnabled)
         {
            cmsLog_debug("re-enabling NAT for interface: %s", wanIpConnObj->X_BROADCOM_COM_IfName);
            rutIpt_initNat(wanIpConnObj->X_BROADCOM_COM_IfName, wanIpConnObj->X_BROADCOM_COM_FullconeNATEnabled);
         }
         if (wanIpConnObj->X_BROADCOM_COM_FirewallEnabled)
         {
            rutIpt_initFirewall(PF_INET, wanIpConnObj->X_BROADCOM_COM_IfName);
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
            if (wanIpConnObj->X_BROADCOM_COM_IPv6Enabled)
            {
               rutIpt_initFirewall(PF_INET6, wanIpConnObj->X_BROADCOM_COM_IfName);
            }
#endif
            rutIpt_initFirewallExceptions(wanIpConnObj->X_BROADCOM_COM_IfName);
         }
#ifdef SUPPORT_UPNP
         if (rut_isUpnpEnabled())
         {
            /* add all upnp iptables rules */
            rutUpnp_configIptableRuleForAllWanIfcs(TRUE);
         }
#endif /* SUPPORT_UPNP */
         /* add back the TCPMSS rules*/
         rutIpt_insertTCPMSSRules(PF_INET, wanIpConnObj->X_BROADCOM_COM_IfName);
      }
#endif /* DMP_BRIDGING_1   aka SUPPORT_PORT_MAP */
         
   }
   else if (oid == MDMOID_WAN_PPP_CONN)
   {
      /* disassociate PPP from LAN bridge, using policy routing */
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */
      UBOOL8 isWanUsedForIntfGroup;

      /*   
      * The policy routing stuff need to be deleted here for live interface group remove
      * only when the wan intf is already removed from interface group (rutPMap_isWanUsedForIntfGroup is FALSE)
      */
      prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
      mdmLibCtx.hideObjectsPendingDelete = FALSE;

      isWanUsedForIntfGroup = rutPMap_isWanUsedForIntfGroup(wanPppConnObj->X_BROADCOM_COM_IfName);

      mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

      cmsLog_debug("ifName %s in the intfGroup = %d, connectionStatus=%s",
                   wanPppConnObj->X_BROADCOM_COM_IfName,
                   isWanUsedForIntfGroup,
                   wanPppConnObj->connectionStatus);
      
      if ((cmsUtl_strcmp(wanPppConnObj->connectionStatus, MDMVS_CONNECTED) == 0) &&
          (isWanUsedForIntfGroup == TRUE))
      {
         UBOOL8 add=FALSE;
         
         rutPMap_doPoliceRoutingOnWanIfc(add,
                                         wanPppConnObj->X_BROADCOM_COM_IfName, 
                                         wanPppConnObj->X_BROADCOM_COM_DefaultGateway,
                                         bridgeIfName,
                                         bridgeObj->bridgeName);
         
         /* remove all iptable rules associated with this interface */
         rutIpt_removeInterfaceIptableRules(wanPppConnObj->X_BROADCOM_COM_IfName, TRUE);
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
//         rutIpt_removeInterfaceIptableRules(wanPppConnObj->X_BROADCOM_COM_IfName, FALSE);  //TODO: what should the action be for IPv6 here?
#endif

         /* remove the dns forwarding the PREROUTING chain for
          * non default bridge (br1..)
          */
         rutIpt_removeBridgeIfNameIptableRules(bridgeIfName);
   

         /* TODO: remove igmp and some other rules ? */
         /* We are redoing initNAT and initFirewall on these objects based on the default (br0) WAN connection rules */
         cmsLog_debug("re-enabling NAT and firewall for deleted object? %s", wanPppConnObj->X_BROADCOM_COM_IfName);
         rutIpt_initNat(wanPppConnObj->X_BROADCOM_COM_IfName, wanPppConnObj->X_BROADCOM_COM_FullconeNATEnabled);
         rutIpt_initFirewall(PF_INET, wanPppConnObj->X_BROADCOM_COM_IfName);
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
         if (wanPppConnObj->X_BROADCOM_COM_IPv6Enabled)
         {
            rutIpt_initFirewall(PF_INET6, wanPppConnObj->X_BROADCOM_COM_IfName);
         }
#endif
         rutIpt_initFirewallExceptions(wanPppConnObj->X_BROADCOM_COM_IfName);
      }
      else if ((isWanUsedForIntfGroup == FALSE) && (isEditWanFilter == TRUE))/*restore iptable rules from interface group*/
      {
         /* remove all iptable rules associated with this interface */
         cmsLog_debug("remove IptableRules with Interface: %s, %s", wanPppConnObj->X_BROADCOM_COM_IfName, bridgeIfName);
         rutIpt_removeInterfaceIptableRules(wanPppConnObj->X_BROADCOM_COM_IfName, TRUE);
         rutIpt_removeInterfaceIptableRules(bridgeIfName, TRUE);
         /*re-enable NAT*/
         if (wanPppConnObj->NATEnabled)
         {
            cmsLog_debug("re-enabling NAT for interface: %s", wanPppConnObj->X_BROADCOM_COM_IfName);
            rutIpt_initNat(wanPppConnObj->X_BROADCOM_COM_IfName, wanPppConnObj->X_BROADCOM_COM_FullconeNATEnabled);
         }
         if (wanPppConnObj->X_BROADCOM_COM_FirewallEnabled)
         {
            rutIpt_initFirewall(PF_INET, wanPppConnObj->X_BROADCOM_COM_IfName);
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
            if (wanPppConnObj->X_BROADCOM_COM_IPv6Enabled)
            {
               rutIpt_initFirewall(PF_INET6, wanPppConnObj->X_BROADCOM_COM_IfName);
            }
#endif
            rutIpt_initFirewallExceptions(wanPppConnObj->X_BROADCOM_COM_IfName);
         }
#ifdef SUPPORT_UPNP
         if (rut_isUpnpEnabled())
         {
            /* add all upnp iptables rules */
            rutUpnp_configIptableRuleForAllWanIfcs(TRUE);
         }
#endif /* SUPPORT_UPNP */
         /* add back the TCPMSS rules*/
         rutIpt_insertTCPMSSRules(PF_INET, wanPppConnObj->X_BROADCOM_COM_IfName);
      }
#endif /* DMP_BRIDGING_1   aka SUPPORT_PORT_MAP */
   }

   cmsObj_free(&obj);
   
   cmsObj_free((void **) &bridgeObj);

   rutMulti_reloadMcpd();

   return ret;
}


CmsRet rutPMap_getBridgeByKey(UINT32 bridgeKey __attribute__((unused)),
                              InstanceIdStack *iidStack,
                              L2BridgingEntryObject **bridgeObj)
{
   CmsRet ret=CMSRET_NO_MORE_INSTANCES;

   if (bridgeObj == NULL || iidStack == NULL)
   {
      cmsLog_error("one or more input params is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

#ifdef DMP_BRIDGING_1
   INIT_INSTANCE_ID_STACK(iidStack);
   while ((ret = cmsObj_getNext(MDMOID_L2_BRIDGING_ENTRY, iidStack, (void **) bridgeObj)) == CMSRET_SUCCESS)
   {
      if ((*bridgeObj)->bridgeKey == bridgeKey)
      {
         break;
      }
      cmsObj_free((void **) bridgeObj);
   }
#endif /* DMP_BRIDGING_1 */

   return ret;
}


CmsRet rutPMap_getBridgeByName(const char *bridgeName, InstanceIdStack *iidStack, L2BridgingEntryObject **bridgeObj)
{
   CmsRet ret=CMSRET_NO_MORE_INSTANCES;
   
   if (bridgeName == NULL || bridgeObj == NULL || iidStack == NULL)
   {
      cmsLog_error("one or more input params is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

#ifdef DMP_BRIDGING_1
   INIT_INSTANCE_ID_STACK(iidStack);
   while ((ret = cmsObj_getNext(MDMOID_L2_BRIDGING_ENTRY, iidStack, (void **) bridgeObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp((*bridgeObj)->bridgeName, bridgeName) == 0)      {
         break;
      }
      cmsObj_free((void **) bridgeObj);
   }
#endif /* DMP_BRIDGING_1 */

   return ret;
}


CmsRet rutPMap_setLanBridgeEnable(const char *bridgeIfName, UBOOL8 enable)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _LanIpIntfObject *ipIntfObj=NULL;
   UBOOL8 found=FALSE;
   CmsRet ret;
   
   while ((ret = cmsObj_getNext(MDMOID_LAN_IP_INTF, &iidStack, (void **) &ipIntfObj)) == CMSRET_SUCCESS)
   {
      if (0 == cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IfName, bridgeIfName))
      {
         found = TRUE;
         break;
      }
      cmsObj_free((void **) &ipIntfObj);
   }
      
   if (!found)
   {
      cmsLog_error("could not find bridge %s", bridgeIfName);
      return CMSRET_NO_MORE_INSTANCES;
   }

   ipIntfObj->enable = enable;
   
   if ((ret = cmsObj_set(ipIntfObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set ip intf state, ret=%d", ret);
   }
   
   cmsObj_free((void **) &ipIntfObj);

   return ret;     
}

#ifdef SUPPORT_LANVLAN
CmsRet rutPMap_getFilterWithVlan(const char *filterInterface, SINT32 VLANIDFilter, InstanceIdStack *iidStack, L2BridgingFilterObject **filterObj)
{
   CmsRet ret;

   if (filterInterface == NULL || filterObj == NULL || iidStack == NULL)
   {
      cmsLog_error("one or more input arguments is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(iidStack);
   while ((ret = cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, iidStack, (void **) filterObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp((*filterObj)->filterInterface, filterInterface) && ((*filterObj)->X_BROADCOM_COM_VLANIDFilter == VLANIDFilter))
      {
         break;
      }
      cmsObj_free((void **) filterObj);
   }

   return ret;
}


CmsRet rutPMap_availableVlanInterfaceToIntfList(const char *filterInterface, char *realIfName, char *intfList, UINT32 *numOfIntf)
{
   CmsRet ret;
   InstanceIdStack iidStack;
   L2BridgingFilterObject *filterObj;
   char vlanIfName[BUFLEN_16];	
   
   if (filterInterface == NULL || realIfName == NULL || intfList == NULL)
   {
      cmsLog_error("one or more input arguments is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, &iidStack, (void **) &filterObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(filterObj->filterInterface, filterInterface))
      {
         if (intfList[0] != '\0')
         {
            strcat(intfList, "|");
         }
         snprintf(vlanIfName, sizeof(vlanIfName), "%s.%d", realIfName, filterObj->X_BROADCOM_COM_VLANIDFilter);	
         strcat(intfList, vlanIfName);
         (*numOfIntf)++;
      }
      cmsObj_free((void **) &filterObj);
   }

   return ret;
}

CmsRet rutPMap_addL2BridgingFilterInterfaceObject(UINT32 intfKey, SINT32 bridgeRef, SINT32 VLANIDFilter)
{
   CmsRet                 ret = CMSRET_SUCCESS;
   MdmPathDescriptor      pathDesc;
   L2BridgingFilterObject *filterObj=NULL;
   char filterInterface[BUFLEN_32];

   cmsLog_debug("interface key %d", intfKey);

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_L2_BRIDGING_FILTER;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not create instance of bridge filter object");
      return ret;
   }

   /* get the object we just created */
   if ((ret = mdm_getObject(MDMOID_L2_BRIDGING_FILTER, &(pathDesc.iidStack), (void **) &filterObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get L2BridgingEntryObject, ret=%d", ret);
      return ret;
   }

   /* populate the fields of the bridge filter object */

   filterObj->filterEnable = TRUE;
   filterObj->filterBridgeReference = bridgeRef;
   sprintf(filterInterface, "%u", intfKey);
   CMSMEM_REPLACE_STRING_FLAGS(filterObj->filterInterface, filterInterface, mdmLibCtx.allocFlags);
   filterObj->X_BROADCOM_COM_VLANIDFilter = VLANIDFilter;

   ret = mdm_setObject((void **)&filterObj, &(pathDesc.iidStack), FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      mdm_freeObject((void **) &filterObj);
      cmsLog_error("Failed to set L2BridgingFilterObject, ret = %d", ret);
   }

   cmsLog_debug("done, ret=%d", ret);

   return ret;
}

void rutPMap_deleteFilterWithVlan(char *l2IfName, UINT32 vid)
{
   char fullPathName[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   InstanceIdStack        iidStack = EMPTY_INSTANCE_ID_STACK;
   _L2BridgingFilterObject *filterObj=NULL;
   _L2BridgingIntfObject *availIntfObj=NULL;
   char filterInterface[BUFLEN_32];
   char availInterfaceReference[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   
   if (rutPMap_lanIfNameToAvailableInterfaceReference(l2IfName, fullPathName) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutPMap_lanIfNameToAvailableInterfaceReference Fail");
      return;
   }

   /*
    * very anoying, the TR98 spec says the fullname does not end in dot,
    * but our internal conversion routines put in the final dot, so strip
    * it out before putting into the MDM.
    */
   strncpy(availInterfaceReference, fullPathName, sizeof(availInterfaceReference));
   availInterfaceReference[strlen(availInterfaceReference)-1] = '\0';   
   
   if (rutPMap_getAvailableInterfaceByRef(availInterfaceReference, &iidStack, &availIntfObj) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find %s", availInterfaceReference);
      return;
   }
   
   sprintf(filterInterface, "%u", availIntfObj->availableInterfaceKey);
   cmsObj_free((void **) &availIntfObj);
   
   cmsLog_debug("deleting filter interface %s (%s)", availInterfaceReference, filterInterface);
   

   if ( rutPMap_getFilterWithVlan(filterInterface, vid, &iidStack, &filterObj) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not find availInterfaceReference %s (FilterInterface %s)", availInterfaceReference, filterInterface);
   }
   else
   {
      cmsObj_free((void **)&filterObj);
   }

   if (cmsObj_deleteInstance(MDMOID_L2_BRIDGING_FILTER, &iidStack) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not delete filter interface");
   }
   else
   {
      cmsLog_debug("filter interface %s deleted", availInterfaceReference);
   }
   
   return;
}

#endif

CmsRet rutPMap_addFilter(const char *ifName, UBOOL8 isWanIntf, SINT32 bridgeRef)
{
   InstanceIdStack        iidStack    = EMPTY_INSTANCE_ID_STACK;
   _L2BridgingFilterObject *filterObj=NULL;
   _L2BridgingIntfObject *availIntfObj=NULL;
   char filterInterface[BUFLEN_32];
   char availInterfaceReference[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   
   CmsRet ret;
#ifdef SUPPORT_LANVLAN
   char lanIfName[BUFLEN_32], *p;
   UINT32 vlanId = 0;
#endif

   if (isWanIntf)
   {   
      if ((ret = rutPMap_wanIfNameToAvailableInterfaceReference(ifName, availInterfaceReference)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not find WAN ifName=%s, cannot add to availIntf", ifName);
         return ret;
      }
   }
   else
   {
#ifdef SUPPORT_LANVLAN
   strncpy(lanIfName, ifName, sizeof(lanIfName));
   if (strstr(lanIfName, "eth") && ((p = strchr(lanIfName, '.')) != NULL)) 
   	{
      *p = '\0';
      vlanId = atoi(p+1);
   	}
      if ((ret = rutPMap_lanIfNameToAvailableInterfaceReference(lanIfName, availInterfaceReference)) != CMSRET_SUCCESS)
#else   
      if ((ret = rutPMap_lanIfNameToAvailableInterfaceReference(ifName, availInterfaceReference)) != CMSRET_SUCCESS)
#endif
      {
         cmsLog_error("could not find LAN ifName=%s, cannot add to availIntf", ifName);
         return ret;
      }
   }
   
   /*
    * very anoying, the TR98 spec says the fullname does not end in dot,
    * but our internal conversion routines put in the final dot, so strip
    * it out before putting into the MDM.
    */
   availInterfaceReference[strlen(availInterfaceReference)-1] = '\0';    
   if ((ret = rutPMap_getAvailableInterfaceByRef(availInterfaceReference, &iidStack, &availIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find %s (avail InterfaceReference %s)", ifName, availInterfaceReference);
      return CMSRET_INVALID_ARGUMENTS;
   }
   
   sprintf(filterInterface, "%u", availIntfObj->availableInterfaceKey);
   cmsObj_free((void **) &availIntfObj);
   
   cmsLog_debug("adding filter interface %s (%s) to bridgeRef=%d", ifName, filterInterface, bridgeRef);

   /* check for duplicate */
#ifdef SUPPORT_LANVLAN
   if ((ret = rutPMap_getFilterWithVlan(filterInterface, vlanId, &iidStack, &filterObj)) == CMSRET_SUCCESS)
#else
   if ((ret = rutPMap_getFilter(filterInterface, &iidStack, &filterObj)) == CMSRET_SUCCESS)
#endif
   {
      cmsObj_free((void **) &filterObj);
      cmsLog_debug("filter intf %s (%s) already exists", ifName, filterInterface);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* add new instance of Layer2Bridging.Filter.{i} */
   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = cmsObj_addInstance(MDMOID_L2_BRIDGING_FILTER, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create new bridging filter interface %s, ret=%d", ifName, ret);
      return ret;
   }

   /* read it back */
   if ((ret = cmsObj_get(MDMOID_L2_BRIDGING_FILTER, &iidStack, 0, (void **) &filterObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get newly created filter interface %s, ret=%d", ifName, ret);
      cmsObj_deleteInstance(MDMOID_L2_BRIDGING_FILTER, &iidStack);
      return ret;
   }

   /* populate the object and set it */
   filterObj->filterEnable = TRUE;
   filterObj->filterBridgeReference = bridgeRef;
   CMSMEM_REPLACE_STRING_FLAGS(filterObj->filterInterface, filterInterface, mdmLibCtx.allocFlags);
#ifdef SUPPORT_LANVLAN
   filterObj->X_BROADCOM_COM_VLANIDFilter = vlanId;
#endif
   
   if ((ret = cmsObj_set(filterObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set L2BridgingFilterObject, ret = %d", ret);
      cmsObj_deleteInstance(MDMOID_L2_BRIDGING_FILTER, &iidStack);
   }

   cmsObj_free((void **) &filterObj);
 
   return ret;
}


CmsRet rutPMap_getFilter(const char *filterInterface, InstanceIdStack *iidStack, L2BridgingFilterObject **filterObj)
{
   CmsRet ret;

   if (filterInterface == NULL || filterObj == NULL || iidStack == NULL)
   {
      cmsLog_error("one or more input arguments is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(iidStack);
   while ((ret = cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, iidStack, (void **) filterObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp((*filterObj)->filterInterface, filterInterface)
#ifdef SUPPORT_LANVLAN
         && ((*filterObj)->X_BROADCOM_COM_VLANIDFilter == 0)
#endif
         )
      {
         break;
      }
      cmsObj_free((void **) filterObj);
   }

   return ret;
}


void rutPMap_deleteFilter(const char *fullPathName, UBOOL8 isWanIntf __attribute__((unused)))
{
   InstanceIdStack        iidStack = EMPTY_INSTANCE_ID_STACK;
   _L2BridgingFilterObject *filterObj=NULL;
   _L2BridgingIntfObject *availIntfObj=NULL;
   char filterInterface[BUFLEN_32];
   char availInterfaceReference[BUFLEN_256];
   CmsRet ret;

   /*
    * very anoying, the TR98 spec says the fullname does not end in dot,
    * but our internal conversion routines put in the final dot, so strip
    * it out before putting into the MDM.
    */
   strncpy(availInterfaceReference, fullPathName, sizeof(availInterfaceReference));
   availInterfaceReference[strlen(availInterfaceReference)-1] = '\0';   
   
   if ((ret = rutPMap_getAvailableInterfaceByRef(availInterfaceReference, &iidStack, &availIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find %s", availInterfaceReference);
      return;
   }
   
   sprintf(filterInterface, "%u", availIntfObj->availableInterfaceKey);
   cmsObj_free((void **) &availIntfObj);
   
   cmsLog_debug("deleting filter interface %s (%s)", availInterfaceReference, filterInterface);
   

   if ((ret = rutPMap_getFilter(filterInterface, &iidStack, &filterObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not find availInterfaceReference %s (FilterInterface %s)", availInterfaceReference, filterInterface);
   }
   else
   {
      cmsObj_free((void **)&filterObj);
   }

   if ((ret = cmsObj_deleteInstance(MDMOID_L2_BRIDGING_FILTER, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not delete filter interface, ret=%d", ret);
   }
   else
   {
      cmsLog_debug("filter interface %s deleted", availInterfaceReference);
   }
   
   return;
}


UBOOL8 rutPMap_isLanInterfaceFilter(const char *filterInterface)
{
   InstanceIdStack iidStack    = EMPTY_INSTANCE_ID_STACK;
   _L2BridgingIntfObject *availIntfObj=NULL;
   UINT32 key;
   UBOOL8 isLanIntf;
   CmsRet ret;
   
   
   if (!cmsUtl_strcmp(filterInterface, MDMVS_LANINTERFACES))
   {
      /* this is a DHCP vendor id filter, so answer is FALSE */
      return FALSE;
   }


   cmsUtl_strtoul(filterInterface, NULL, 0, &key);
   
   if ((ret = rutPMap_getAvailableInterfaceByKey(key, &iidStack, &availIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find filter interface %s", filterInterface);
      return FALSE;
   }
   
   isLanIntf = (0 == cmsUtl_strcmp(availIntfObj->interfaceType, MDMVS_LANINTERFACE));
   
   cmsObj_free((void **) &availIntfObj);
   
   return isLanIntf;
}


UBOOL8 rutPMap_isWanInterfaceFilter(const char *filterInterface)
{
   InstanceIdStack iidStack    = EMPTY_INSTANCE_ID_STACK;
   _L2BridgingIntfObject *availIntfObj=NULL;
   UINT32 key;
   UBOOL8 isWanIntf;
   CmsRet ret;
   
   
   if (!cmsUtl_strcmp(filterInterface, MDMVS_LANINTERFACES))
   {
      /* this is a DHCP vendor id filter, so answer is FALSE */
      return FALSE;
   }
   
   cmsUtl_strtoul(filterInterface, NULL, 0, &key);

   if ((ret = rutPMap_getAvailableInterfaceByKey(key, &iidStack, &availIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find filter interface %s", filterInterface);
      return FALSE;
   }
   
   isWanIntf = (0 == cmsUtl_strcmp(availIntfObj->interfaceType, MDMVS_WANINTERFACE));
   
   cmsObj_free((void **) &availIntfObj);
   
   return isWanIntf;
}


CmsRet rutPMap_addAvailableInterface(const char *ifName, UBOOL8 isWanIntf)
{
   InstanceIdStack        iidStack    = EMPTY_INSTANCE_ID_STACK;
   _L2BridgingIntfObject   *availIntfObj=NULL;
   char availInterfaceReference[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   CmsRet ret;

   if (isWanIntf)
   {   
      if ((ret = rutPMap_wanIfNameToAvailableInterfaceReference(ifName, availInterfaceReference)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not find WAN ifName=%s, cannot add to availIntf", ifName);
         return ret;
      }
   }
   else
   {
      if ((ret = rutPMap_lanIfNameToAvailableInterfaceReference(ifName, availInterfaceReference)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not find LAN ifName=%s, cannot add to availIntf", ifName);
         return ret;
      }
   }

   /*
    * very anoying, the TR98 spec says the fullname does not end in dot,
    * but our internal conversion routines put in the final dot, so strip
    * it out before putting into the MDM.
    */
   availInterfaceReference[strlen(availInterfaceReference)-1] = '\0';   
   /* check for duplicate */
   if ((ret = rutPMap_getAvailableInterfaceByRef(availInterfaceReference, &iidStack, &availIntfObj)) == CMSRET_SUCCESS)
   {
      cmsObj_free((void **) &availIntfObj);
      cmsLog_debug("avail InterfaceReference %s already exists", availInterfaceReference);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* add new instance of Layer2Bridging.AvailableInterface.{i} */
   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = cmsObj_addInstance(MDMOID_L2_BRIDGING_INTF, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create new bridging available interface %s, ret=%d", ifName, ret);
      return ret;
   }

   /* read it back */
   if ((ret = cmsObj_get(MDMOID_L2_BRIDGING_INTF, &iidStack, 0, (void **) &availIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get newly created available interface %s, ret=%d", ifName, ret);
      cmsObj_deleteInstance(MDMOID_L2_BRIDGING_INTF, &iidStack);
      return ret;
   }


   /* Fill the information and set it back. */
   CMSMEM_REPLACE_STRING_FLAGS(availIntfObj->interfaceType, 
                               (isWanIntf ? MDMVS_WANINTERFACE : MDMVS_LANINTERFACE),
                               mdmLibCtx.allocFlags);

   CMSMEM_REPLACE_STRING_FLAGS(availIntfObj->interfaceReference, availInterfaceReference, mdmLibCtx.allocFlags);

   if ((ret = cmsObj_set(availIntfObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set L2BridgingIntfObject, ret = %d", ret);
      cmsObj_deleteInstance(MDMOID_L2_BRIDGING_INTF, &iidStack);
   }
   else
   {
      cmsLog_debug("%s added as a %s available interface", ifName, availIntfObj->interfaceType);
   }
   
   cmsObj_free((void **) &availIntfObj); 

   rutMulti_reloadMcpd();

   return ret;  
}


void rutPMap_deleteAvailableInterface(const char *fullPathName,
                                 UBOOL8 isWanIntf __attribute__((unused)))
{
   InstanceIdStack        iidStack = EMPTY_INSTANCE_ID_STACK;
   L2BridgingIntfObject   *availIntfObj = NULL;
   char availInterfaceReference[BUFLEN_256];
   CmsRet ret;

   /*
    * very anoying, the TR98 spec says the fullname does not end in dot,
    * but our internal conversion routines put in the final dot, so strip
    * it out before putting into the MDM.
    */
   strncpy(availInterfaceReference, fullPathName, sizeof(availInterfaceReference));
   availInterfaceReference[strlen(availInterfaceReference)-1] = '\0';   
   
   if ((ret = rutPMap_getAvailableInterfaceByRef(availInterfaceReference, &iidStack, &availIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not find avail InterfaceReference %s", availInterfaceReference);
   }
   else
   {
      cmsObj_free((void **)&availIntfObj);
   }

   if ((ret = cmsObj_deleteInstance(MDMOID_L2_BRIDGING_INTF, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not delete available interface, ret=%d", ret);
   }
   else
   {
      cmsLog_debug("available interface %s deleted", availInterfaceReference);
   }

   rutMulti_reloadMcpd();
   
   return;
}


CmsRet rutPMap_getAvailableInterfaceByRef(const char *availInterfaceReference, InstanceIdStack *iidStack, L2BridgingIntfObject **availIntfObj)
{
   CmsRet ret;
   UBOOL8 found=FALSE;

   if (availInterfaceReference == NULL || availIntfObj == NULL || iidStack == NULL)
   {
      cmsLog_error("one or more input arguments is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(iidStack);
   while ((ret = cmsObj_getNext(MDMOID_L2_BRIDGING_INTF, iidStack, (void **) availIntfObj)) == CMSRET_SUCCESS)
   {
      cmsLog_debug("(*availIntfObj)->interfaceReference=%s, availInterfaceReference=%s", (*availIntfObj)->interfaceReference, availInterfaceReference);
      if (!cmsUtl_strcmp((*availIntfObj)->interfaceReference, availInterfaceReference))
      {
         found = TRUE;
         break;
      }
      cmsObj_free((void **) availIntfObj);
   }
   
   if (found)
   {
      cmsLog_debug("found avail InterfaceReference %s", availInterfaceReference);
      ret = CMSRET_SUCCESS;
   }
   else
   {
      cmsLog_debug("could not find avail InterfaceReference %s", availInterfaceReference);
   }

   return ret;
}


CmsRet rutPMap_getAvailableInterfaceByKey(UINT32 availableInterfaceKey, InstanceIdStack *iidStack, L2BridgingIntfObject **availIntfObj)
{
   CmsRet ret;
   UBOOL8 found=FALSE;

   if (availIntfObj == NULL || iidStack == NULL)
   {
      cmsLog_error("one or more input arguments is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(iidStack);
   while ((ret = cmsObj_getNext(MDMOID_L2_BRIDGING_INTF, iidStack, (void **) availIntfObj)) == CMSRET_SUCCESS)
   {
      if ((*availIntfObj)->availableInterfaceKey == availableInterfaceKey)
      {
         found = TRUE;
         break;
      }
      cmsObj_free((void **) availIntfObj);
   }
   
   if (found)
   {
      cmsLog_debug("found AvailableInterfaceKey %u", availableInterfaceKey);
      ret = CMSRET_SUCCESS;
   }
   else
   {
      cmsLog_debug("could not find AvailableInterfacekey %u", availableInterfaceKey);
   }

   return ret;
}


CmsRet rutPMap_wanIfNameToAvailableInterfaceReference(const char *ifName, char *availInterfaceReference)
{
   InstanceIdStack iidStack;
   _WanIpConnObject *wanIpObj=NULL;
   _WanPppConnObject *wanPppObj=NULL;
   MdmPathDescriptor pathDesc;
   char *fullPathString=NULL;
   UBOOL8 found=FALSE;
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("looking for ifName=%s", ifName);
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &wanIpObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(wanIpObj->X_BROADCOM_COM_IfName, ifName))
      {
         found = TRUE;
         
         
         /*
          * need to create a full param path
          * The new TR98 spec says the InterfaceReference should point to the WANIP/PPPConnection
          * so now we will have that in available interface of layer2briding.
          */
         memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
         pathDesc.oid = MDMOID_WAN_IP_CONN;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPathString);
         /* technically, len of 256 is OK, but keep as is for compatibility */
         if (strlen(fullPathString) >= BUFLEN_256)
         {
            cmsLog_error("fullpath %s too long to fit in param", fullPathString);
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            strcpy(availInterfaceReference, fullPathString);
            ret = CMSRET_SUCCESS;
         }
         
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
      }
      
      cmsObj_free((void **) &wanIpObj);
   }
   
   if (found)
   {
      return ret;
   }
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &wanPppObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(wanPppObj->X_BROADCOM_COM_IfName, ifName))
      {
         found = TRUE;
         
         /* need to create a full param path */
         memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
         pathDesc.oid = MDMOID_WAN_PPP_CONN;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPathString);
         /* technically, len of 256 is OK, but keep as is for compatibility */
         if (strlen(fullPathString) >= BUFLEN_256)
         {
            cmsLog_error("fullpath %s too long to fit in param", fullPathString);
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            strcpy(availInterfaceReference, fullPathString);
            ret = CMSRET_SUCCESS;
         }
         
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
      }
      
      cmsObj_free((void **) &wanPppObj);
   }
   
   if (found)
   {
      cmsLog_debug("conversion successful %s -> %s", ifName, availInterfaceReference);
      return ret;
   }
   
   return CMSRET_NO_MORE_INSTANCES;
}


CmsRet rutPMap_lanIfNameToAvailableInterfaceReference(const char *ifName, char *availInterfaceReference)
{
   InstanceIdStack iidStack;
   _LanEthIntfObject *ethIntfObj=NULL;
#ifdef DMP_USBLAN_1
   _LanUsbIntfObject *usbIntfObj=NULL;
#endif
#ifdef SUPPORT_MOCA
   _LanMocaIntfObject *mocaIntfObj=NULL;
#endif
#ifdef BRCM_WLAN
   _WlVirtIntfCfgObject *wlVirtIntfCfgObj=NULL;
#endif
   MdmPathDescriptor pathDesc;
   char *fullPathString=NULL;
   UBOOL8 found=FALSE;
   CmsRet ret=CMSRET_SUCCESS;

   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (ret = cmsObj_getNext(MDMOID_LAN_ETH_INTF, &iidStack, (void**) &ethIntfObj)) == CMSRET_SUCCESS)
   {
#ifdef SUPPORT_LANVLAN
      /* For VLAN LAN, only compare the part without vlan extension; ie. only the "eth1" part since ifName 
      * is "eth1.0" here
      */
      if (!cmsUtl_strncmp(ethIntfObj->X_BROADCOM_COM_IfName, ifName, cmsUtl_strlen(ethIntfObj->X_BROADCOM_COM_IfName)))
#else   
      if (!cmsUtl_strcmp(ethIntfObj->X_BROADCOM_COM_IfName, ifName))
#endif      
      {
         found = TRUE;
         
         /* need to create a full param path */
         memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
         pathDesc.oid = MDMOID_LAN_ETH_INTF;
         pathDesc.iidStack = iidStack;
         ret = cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPathString);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_pathDescriptorToFullPath failed on %s. ret %d", ifName, ret);
            return ret;
         }
         
         /* technically, len of 256 is OK, but keep as is for compatibility */
         if (strlen(fullPathString) >= BUFLEN_256)
         {
            cmsLog_error("fullpath %s too long to fit in param", fullPathString);
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            strcpy(availInterfaceReference, fullPathString);
            ret = CMSRET_SUCCESS;
         }
         
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
      }
      
      cmsObj_free((void **) &ethIntfObj);
   }
   
   if (found)
   {
      cmsLog_debug("conversion successful %s -> %s", ifName, availInterfaceReference);
      return ret;
   }
    

#ifdef DMP_USBLAN_1    
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (ret = cmsObj_getNext(MDMOID_LAN_USB_INTF, &iidStack, (void**) &usbIntfObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(usbIntfObj->X_BROADCOM_COM_IfName, ifName))
      {
         found = TRUE;
         
         /* need to create a full param path */
         memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
         pathDesc.oid = MDMOID_LAN_USB_INTF;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPathString);
         /* technically, len of 256 is OK, but keep as is for compatibility */
         if (strlen(fullPathString) >= BUFLEN_256)
         {
            cmsLog_error("fullpath %s too long to fit in param", fullPathString);
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            strcpy(availInterfaceReference, fullPathString);
            ret = CMSRET_SUCCESS;
         }
         
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
      }
      
      cmsObj_free((void **) &usbIntfObj);
   }
   
   if (found)
   {
      cmsLog_debug("conversion successful %s -> %s", ifName, availInterfaceReference);
      return ret;
   }
#endif /* DMP_USBLAN_1 */

#ifdef SUPPORT_MOCA
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (ret = cmsObj_getNext(MDMOID_LAN_MOCA_INTF, &iidStack, (void**) &mocaIntfObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(mocaIntfObj->ifName, ifName))
      {
         found = TRUE;
         
         /* need to create a full param path */
         memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
         pathDesc.oid = MDMOID_LAN_MOCA_INTF;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPathString);
         /* technically, len of 256 is OK, but keep as is for compatibility */
         if (strlen(fullPathString) >= BUFLEN_256)
         {
            cmsLog_error("fullpath %s too long to fit in param", fullPathString);
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            strcpy(availInterfaceReference, fullPathString);
            ret = CMSRET_SUCCESS;
         }
         
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
      }
      
      cmsObj_free((void **) &mocaIntfObj);
   }
   
   if (found)
   {
      cmsLog_debug("conversion successful %s -> %s", ifName, availInterfaceReference);
      return ret;
   }
#endif /* SUPPORT_MOCA */

   
 #ifdef BRCM_WLAN
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (ret = cmsObj_getNext(MDMOID_WL_VIRT_INTF_CFG, &iidStack, (void**) &wlVirtIntfCfgObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(wlVirtIntfCfgObj->wlIfcname, ifName))
      {
         found = TRUE;
         
         /* need to create a full param path */
         memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
         pathDesc.oid = MDMOID_WL_VIRT_INTF_CFG;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPathString);

         /* technically, len of 256 is OK, but keep as is for compatibility */
         if (strlen(fullPathString) >= BUFLEN_256)
         {
            cmsLog_error("fullpath %s too long to fit in param", fullPathString);
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            strcpy(availInterfaceReference, fullPathString);
            ret = CMSRET_SUCCESS;
         }
         
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
      }
      
      cmsObj_free((void **) &wlVirtIntfCfgObj);
   }

   if (found)
   {
      cmsLog_debug("conversion successful %s -> %s", ifName, availInterfaceReference);
      return ret;
   }
#endif
   return CMSRET_NO_MORE_INSTANCES;
}


CmsRet rutPMap_availableInterfaceReferenceToMdmObject(const char *availableInterfaceReference,
                                                   InstanceIdStack *iidStack,
                                                   void **obj)
{
   char fullPath[BUFLEN_512];
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   /*
    * Very annoying, TR98 says the fullpath ends without a ., so we have
    * to append a . before we can send the fullpath into our conversion function.
    */
   snprintf(fullPath, sizeof(fullPath), "%s.", availableInterfaceReference);
   if ((ret = cmsMdm_fullPathToPathDescriptor(fullPath, &pathDesc)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert %s to pathDesc", availableInterfaceReference);
      return ret;
   }
   
   INIT_INSTANCE_ID_STACK(iidStack);

   if (pathDesc.oid == MDMOID_WAN_IP_CONN)
   {
      /* ifname could be under WAN_IP_CONN or WAN_PPP_CONN */
      /* get the first one we find.  New TR98 Layer2Bridging can have ip/ppp connection object 
       * which will be compatible with Vlan Mux and MSC */
      if ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_WAN_IP_CONN, &(pathDesc.iidStack), iidStack, OGF_NO_VALUE_UPDATE, obj)) == CMSRET_SUCCESS)
      {
         return CMSRET_SUCCESS;
      }
   }
   else if (pathDesc.oid == MDMOID_WAN_PPP_CONN)
   {
      if ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_WAN_PPP_CONN, &(pathDesc.iidStack), iidStack, OGF_NO_VALUE_UPDATE, obj)) == CMSRET_SUCCESS)
      {
         return CMSRET_SUCCESS;
      }
   }
   else if (pathDesc.oid == MDMOID_LAN_ETH_INTF)
   {
      if ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_LAN_ETH_INTF, &(pathDesc.iidStack), iidStack, OGF_NO_VALUE_UPDATE, obj)) == CMSRET_SUCCESS)
      {
         return CMSRET_SUCCESS;
      }
   }
   else if (pathDesc.oid == MDMOID_LAN_USB_INTF)
   {
      if ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_LAN_USB_INTF, &(pathDesc.iidStack), iidStack, OGF_NO_VALUE_UPDATE, obj)) == CMSRET_SUCCESS)
      {
         return CMSRET_SUCCESS;
      }
   }
   else if (pathDesc.oid == MDMOID_LAN_MOCA_INTF)
   {
      if ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_LAN_MOCA_INTF, &(pathDesc.iidStack), iidStack, OGF_NO_VALUE_UPDATE, obj)) == CMSRET_SUCCESS)
      {
         return CMSRET_SUCCESS;
      }
   }
   else if (pathDesc.oid == MDMOID_WL_VIRT_INTF_CFG )
   {
      if ((ret = cmsObj_getNextInSubTree(MDMOID_WL_VIRT_INTF_CFG, &(pathDesc.iidStack), iidStack, obj)) == CMSRET_SUCCESS)
      {
         return CMSRET_SUCCESS;
      }
   }
   else if (pathDesc.oid == MDMOID_LAN_EPON_INTF)
   {
      if ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_LAN_EPON_INTF, &(pathDesc.iidStack), iidStack, OGF_NO_VALUE_UPDATE, obj)) == CMSRET_SUCCESS)
      {
         return CMSRET_SUCCESS;
      }
   }   
   else
   {
      cmsLog_error("unexpected oid %d", pathDesc.oid);
      return CMSRET_INVALID_ARGUMENTS;
   }

   return CMSRET_NO_MORE_INSTANCES;
}


CmsRet rutPMap_availableInterfaceReferenceToIfName(const char *availableInterfaceReference, char *ifName)
{
   MdmObjectId oid;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   void *obj=NULL;
   WanIpConnObject *wanIpConnObj=NULL;
   WanPppConnObject *wanPppConnObj=NULL;
   LanEthIntfObject *lanEthIntfObj=NULL;
   LanMocaIntfObject *lanMocaIntfObj=NULL;
   LanUsbIntfObject *lanUsbIntfObj=NULL;
   LanEponIntfObject *lanEponIntfObj=NULL;
   WlVirtIntfCfgObject *wlVirtIntfCfgObject=NULL;
   CmsRet ret;

   ret = rutPMap_availableInterfaceReferenceToMdmObject(availableInterfaceReference, &iidStack, &obj);
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }
   
   oid = GET_MDM_OBJECT_ID(obj);

   if (oid == MDMOID_WAN_IP_CONN)
   {
      wanIpConnObj = (WanIpConnObject *) obj;
      sprintf(ifName, "%s", wanIpConnObj->X_BROADCOM_COM_IfName);
   }
   else if (oid == MDMOID_WAN_PPP_CONN)
   {
      wanPppConnObj = (WanPppConnObject *) obj;
      sprintf(ifName, "%s", wanPppConnObj->X_BROADCOM_COM_IfName);
   }
   else if (oid == MDMOID_LAN_ETH_INTF)
   {
      lanEthIntfObj = (LanEthIntfObject *) obj;
      sprintf(ifName, "%s", lanEthIntfObj->X_BROADCOM_COM_IfName);
   }
   else if (oid == MDMOID_LAN_MOCA_INTF)
   {
      lanMocaIntfObj = (LanMocaIntfObject *) obj;
      sprintf(ifName, "%s", lanMocaIntfObj->ifName);
   }
   else if (oid == MDMOID_LAN_USB_INTF)
   {
      lanUsbIntfObj = (LanUsbIntfObject *) obj;
      sprintf(ifName, "%s", lanUsbIntfObj->X_BROADCOM_COM_IfName);
   }
   else if (oid == MDMOID_LAN_EPON_INTF)
   {
      lanEponIntfObj = (LanEponIntfObject *) obj;
      sprintf(ifName, "%s", lanEponIntfObj->ifName);
   }
   else if (oid == MDMOID_WL_VIRT_INTF_CFG )
   {
      wlVirtIntfCfgObject = (WlVirtIntfCfgObject *) obj;
      sprintf(ifName, "%s", wlVirtIntfCfgObject->wlIfcname);
   }
   else
   {
      cmsLog_error("unexpected oid %d", oid);
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsObj_free(&obj);
   return ret;
}


CmsRet rutPMap_filterInterfaceToIfName(const char *filterInterface, char *ifName)
{
   UINT32 key;
   InstanceIdStack iidStack;
   _L2BridgingIntfObject *availIntfObj=NULL;
   CmsRet ret;
   
   if ((ret = cmsUtl_strtoul(filterInterface, NULL, 0, &key)) != CMSRET_SUCCESS)
   {
      cmsLog_error("not a valid filterInterface value %s", filterInterface);
      return ret;
   }
   
   if ((ret = rutPMap_getAvailableInterfaceByKey(key, &iidStack, &availIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find available interface obj with key %u", key);
      return ret;
   }
   
   if ((ret = rutPMap_availableInterfaceReferenceToIfName(availIntfObj->interfaceReference, ifName)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert %s to ifName", availIntfObj->interfaceReference);
   }
   
   cmsObj_free((void **)&availIntfObj);

   return ret;      
}


CmsRet rutPMap_ifNameToFilterObject(const char *ifName, InstanceIdStack *iidStack, L2BridgingFilterObject **filterObj)
{
   char fullPath[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   char keyBuf[BUFLEN_32];
   L2BridgingIntfObject *availIntfObj=NULL;
   InstanceIdStack iidStack2=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("converting ifName=%s", ifName);

   ret = rutPMap_lanIfNameToAvailableInterfaceReference(ifName, fullPath);
   if (ret != CMSRET_SUCCESS)
   {
      ret = rutPMap_wanIfNameToAvailableInterfaceReference(ifName, fullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not convert ifName=%s", ifName);
         return ret;
      }
   }

   /*
    * very anoying, the TR98 spec says the fullname does not end in dot,
    * but our internal conversion routines put in the final dot, so strip
    * it out before putting into the MDM.
    */
   fullPath[strlen(fullPath)-1] = '\0';
   ret = rutPMap_getAvailableInterfaceByRef(fullPath, &iidStack2, &availIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_debug("could not find availableInterface %s", fullPath);
      return ret;
   }

   sprintf(keyBuf, "%u", availIntfObj->availableInterfaceKey);
   cmsObj_free((void **) &availIntfObj);


   ret = rutPMap_getFilter(keyBuf, iidStack, filterObj);

   return ret;
}


CmsRet rutPMap_associateDhcpVendorIdWithBridge(const char *dhcpVendorIds __attribute__((unused)),
                             SINT32 bridgeRef __attribute__((unused)))
{
   /*
    * mwang: not clear if it even makes sense to have this function.
    * Dhcp vendor id to bridge mapping is done by dhcpd and ssk, no need for an rcl function?
    */
   return CMSRET_SUCCESS;  
}


CmsRet rutPMap_disassociateDhcpVendorIdFromBridge(const char *dhcpVendorIds __attribute__((unused)),
                           SINT32 bridgeRef __attribute__((unused)))
{
   return CMSRET_SUCCESS;  
}


UBOOL8 rutPMap_isWanUsedForIntfGroupFullPathName(const char *availInterfaceReference)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _L2BridgingIntfObject *availIntfObj=NULL;
   UBOOL8 found=FALSE;
   
   while (!found && cmsObj_getNext(MDMOID_L2_BRIDGING_INTF, &iidStack, (void **) &availIntfObj) == CMSRET_SUCCESS)
   {
      cmsLog_debug("availIntfObj->interfaceReference=%s, availInterfaceReference=%s", availIntfObj->interfaceReference, availInterfaceReference);
      if (cmsUtl_strcmp(availIntfObj->interfaceReference, availInterfaceReference) == 0)
      {
         InstanceIdStack  filterObjIidStack = EMPTY_INSTANCE_ID_STACK;
         _L2BridgingFilterObject *filterObj = NULL;
         char filterInterface[BUFLEN_32];
         /* get filter object to from InterfacReference to check 
          * FilterBridgeRef -- if > 0, it is in the interface group.
          */
         sprintf(filterInterface, "%u", availIntfObj->availableInterfaceKey);
         
         if (rutPMap_getFilter(filterInterface, &filterObjIidStack, &filterObj) != CMSRET_SUCCESS)
         {
            cmsLog_error("filter  intf %s not exists.", availIntfObj->availableInterfaceKey);
            cmsObj_free((void **) &availIntfObj);
            return found;
         }
         else  if (filterObj->filterBridgeReference > 0)
         {
            found = TRUE;
         }
         cmsObj_free((void **) &filterObj);
      }
      cmsObj_free((void **) &availIntfObj);
   }

   cmsLog_debug("found = %d and availInterfaceReference  is %s", found, availInterfaceReference);

   return found;

}


UBOOL8 rutPMap_isWanUsedForIntfGroup(const char *ifName)
{
   char availInterfaceReference[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   UBOOL8 prevHideObjectsPendingDelete;
   UBOOL8 isWanUsedForIntfGroup = FALSE;
   
   /* override default MDM behavior of hiding objects that are pending delete */
   prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
   mdmLibCtx.hideObjectsPendingDelete = FALSE;

   if (rutPMap_wanIfNameToAvailableInterfaceReference(ifName, availInterfaceReference) == CMSRET_SUCCESS)
   {
      /* get rid of '.' since the availIntfObj->interfaceReference has no '.' at the end */
      availInterfaceReference[strlen(availInterfaceReference)-1] = '\0';

      isWanUsedForIntfGroup = rutPMap_isWanUsedForIntfGroupFullPathName(availInterfaceReference);
   }
   else
   {
       cmsLog_error("could not find WAN ifName=%s", ifName);
   }

   mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

   return isWanUsedForIntfGroup;
   
}


CmsRet rutPMap_getBridgeIfNameFromFullPathName(const char *availInterfaceReference, char *bridgeIfName)
{
   InstanceIdStack  iidStack = EMPTY_INSTANCE_ID_STACK;
   _L2BridgingFilterObject *filterObj = NULL;
    _L2BridgingIntfObject *availIntfObj=NULL;
   char filterInterface[BUFLEN_32];            
   CmsRet ret;
   
   if ((ret = rutPMap_getAvailableInterfaceByRef(availInterfaceReference, &iidStack, &availIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find avail InterfaceReference %s",  availInterfaceReference);
      return ret;
   }
   
   sprintf(filterInterface, "%u", availIntfObj->availableInterfaceKey);
   cmsObj_free((void **) &availIntfObj);
   
   cmsLog_debug("found filter interface (%s) for availInterfaceReference=%s",  filterInterface, availInterfaceReference);

   
   if ((ret = rutPMap_getFilter(filterInterface, &iidStack, &filterObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("filter intf %s not exists. ret=%d", filterInterface, ret);
      return ret;
   }    
   cmsLog_debug("filterObj->Filt  =%d", filterObj->filterBridgeReference );
   sprintf(bridgeIfName, "br%d", filterObj->filterBridgeReference );
      
   cmsObj_free((void **) &filterObj);

   return ret;
   
}

CmsRet rutPMap_getBridgeIfNameFromIfName(const char *ifName, char *bridgeIfName, UBOOL8 isWan)
{
   char availInterfaceReference[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   CmsRet ret;

   if (isWan)
   {
      if ((ret = rutPMap_wanIfNameToAvailableInterfaceReference(ifName, availInterfaceReference)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not find WAN ifName=%s,", ifName);
         return ret;
      }
   }
   else
   {
      if ((ret = rutPMap_lanIfNameToAvailableInterfaceReference(ifName, availInterfaceReference)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not find LAN ifName=%s,", ifName);
         return ret;
      }
   }

   /*
    * very anoying, the TR98 spec says the fullname does not end in dot,
    * but our internal conversion routines put in the final dot, so strip
    * it out before putting into the MDM.
    */
   availInterfaceReference[strlen(availInterfaceReference)-1] = '\0';   

   return (rutPMap_getBridgeIfNameFromFullPathName(availInterfaceReference, bridgeIfName));
   
}            


#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */

CmsRet rutPMap_configPolicyRoute(const UBOOL8 add, const char *ifName, const char *defaultGateway)
{
   InstanceIdStack iidStack;
   _L2BridgingEntryObject *bridgeObj=NULL;
   UINT32 bridgeKey;
   char bridgeIfName[BUFLEN_32]; 
   CmsRet ret;
   
   /* the filter object is already in mdm, so just the correct bridge ifName */
   if ((ret = rutPMap_getBridgeIfNameFromIfName(ifName, bridgeIfName, TRUE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get bridgeIfname for wanIfName=%s", ifName);
      return ret;
   }
  
   /* get bridge from "br?" */
   bridgeKey = atoi((bridgeIfName+2));
   
   /* get the bridge Object from bridgeKey*/
   if ((ret = rutPMap_getBridgeByKey(bridgeKey, &iidStack,  &bridgeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Error retrieving the bridge object, ret=%d", ret);
      return ret;
   }
   cmsLog_debug("bridgeIfName=%s, intfGroupName=%s", bridgeIfName, bridgeObj->bridgeName);
  
   /* perform the add policy route action */
   rutPMap_doPoliceRoutingOnWanIfc(add, 
                                   ifName, 
                                   defaultGateway,
                                   bridgeIfName,
                                   bridgeObj->bridgeName);
   /* free bridge object */
   cmsObj_free((void **) &bridgeObj);

   return ret;
   
}
#endif /* DMP_BRIDGING_1  aka SUPPORT_PORT_MAP */

#if defined(DMP_BRIDGING_1) || defined(DMP_DEVICE2_BRIDGE_1) /* aka SUPPORT_PORT_MAP */
CmsRet rutPMap_doPoliceRoutingOnWanIfc(const UBOOL8 add,
                                       const char *wanIfName, 
                                       const char *defaultGateway,
                                       const char *bridgeIfName,
                                       const char *intfGroupName)
{
   UBOOL8 fromPolicyRoute = FALSE;
   CmsRet ret = CMSRET_SUCCESS;
   
   /* Only do the policy route rule if it is not "Default" group */
   if (!cmsUtl_strcmp("Default", intfGroupName))
   {
      cmsLog_notice("No policy route on Default group");
      return ret;
   }
   
   if (add)
   {
      if ((ret = 
         rutRt_addPolicyRouting(fromPolicyRoute, intfGroupName, bridgeIfName, NULL, defaultGateway, wanIfName)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Fail to add policy route for %s.  Error=%d", wanIfName, ret);
      }
   }
   else
   {
      if ((ret = rutRt_deletePolicyRouting(fromPolicyRoute, intfGroupName)) != CMSRET_SUCCESS)
      {
         cmsLog_notice("Fail to delete policy route for %s.  Error=%d", intfGroupName, ret);
      }     
   }

   return ret;
   
}
#endif /* DMP_BRIDGING_1 || DMP_DEVICE2_BRIDGE_1 */ /* aka SUPPORT_PORT_MAP */

CmsRet rutPMap_getBridgeKey(const char *bridgeName, UINT32 *bridgeKey)
{
   L2BridgingEntryObject *bridgeObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   
   ret = rutPMap_getBridgeByName(bridgeName, &iidStack, &bridgeObj);
   if (ret == CMSRET_SUCCESS)
   {
      *bridgeKey = bridgeObj->bridgeKey;
      cmsObj_free((void **) &bridgeObj);
   }   
   
   return ret;
}


CmsRet rutPMap_getFilterDhcpVendorIdByBridgeName(const char *bridgeName, InstanceIdStack *iidStack, L2BridgingFilterObject **filterObj)
{
   CmsRet ret;
   UINT32 bridgeKey=0;

   if ((ret = rutPMap_getBridgeKey(bridgeName, &bridgeKey)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cannot find bridge key for bridge %s", bridgeName);
      return ret;
   }

   INIT_INSTANCE_ID_STACK(iidStack);
   while ((ret = cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, iidStack, (void **) filterObj)) == CMSRET_SUCCESS)
   {
      if ((!cmsUtl_strcmp((*filterObj)->filterInterface, MDMVS_LANINTERFACES)) &&
          ((*filterObj)->filterBridgeReference == (SINT32) bridgeKey) &&
          ((*filterObj)->sourceMACFromVendorClassIDFilter != NULL) &&
          (strlen((*filterObj)->sourceMACFromVendorClassIDFilter) > 0))
      {
         cmsLog_debug("found filterDhcpVendorId, filterInterface=%s vendorId=%s", (*filterObj)->filterInterface, (*filterObj)->sourceMACFromVendorClassIDFilter); 
         /* break now and don't free filterObj, return to caller */
         break;
      }
      cmsObj_free((void **) filterObj);
   }

   return ret;
}



void rutPmap_deleteFilterDhcpVendorId(const char *bridgeName)
{
   L2BridgingFilterObject *filterObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   ret = rutPMap_getFilterDhcpVendorIdByBridgeName(bridgeName, &iidStack, &filterObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_debug("no dhcp vendor id filter to delete for bridge %s", bridgeName);
      return;
   }

   cmsObj_free((void **) &filterObj);

   /* now our iidStack is pointing to the right instance, delete it */
   if ((ret = cmsObj_deleteInstance(MDMOID_L2_BRIDGING_FILTER, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not delete bridging filter dhcp vendor id, ret=%d", ret);
   }

   return;
}


CmsRet rutPMap_disassocAllFilterIntfFromBridge(const char *bridgeName)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack filterIidStack = EMPTY_INSTANCE_ID_STACK;
   L2BridgingFilterObject *filterObj=NULL;
   SINT32 srcBridgeRef, defaultBridgeRef;


   cmsLog_debug("Disassociating all filter interfaces from bridge %s (and associating them to default bridge)", bridgeName);

   if (bridgeName == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* lookup the default bridge key */
   if ((ret = rutPMap_getBridgeKey("Default", (UINT32 *) &defaultBridgeRef)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cannot find default bridge entry, ret=%d", ret);
      return ret;
   }

   /* look up the source bridge key */
   if ((ret = rutPMap_getBridgeKey(bridgeName, (UINT32 *) &srcBridgeRef)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cannot find bridge %s, ret=%d", bridgeName, ret);
      return ret;
   }

   /* now go through all the filter interface objects */
   while ((ret = cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, &filterIidStack,
                                (void **) &filterObj)) == CMSRET_SUCCESS)
   {
      if ((filterObj->filterBridgeReference == srcBridgeRef) &&
          (cmsUtl_strcmp(filterObj->filterInterface, MDMVS_LANINTERFACES)))
      {
         cmsLog_debug("changing filter interface (%s) bridge ref from %d to %d", filterObj->filterInterface, srcBridgeRef, defaultBridgeRef);
#ifdef SUPPORT_LANVLAN
         if (filterObj->X_BROADCOM_COM_VLANIDFilter > 0)	
            filterObj->filterBridgeReference = -1;
         else
#endif
         filterObj->filterBridgeReference = defaultBridgeRef;
         if ((ret = cmsObj_set(filterObj, &filterIidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Error deassociating %s", filterObj->filterInterface);
         }
      }
      cmsObj_free((void **) &filterObj);
   }

   if (ret == CMSRET_NO_MORE_INSTANCES) {
      return CMSRET_SUCCESS;
   }

   rutMulti_reloadMcpd();

   return ret;
}


void rutPMap_deleteBridge(const char *bridgeName)
{
   CmsRet                ret        = CMSRET_SUCCESS;
   InstanceIdStack       iidStack   = EMPTY_INSTANCE_ID_STACK;
   L2BridgingEntryObject *bridgeObj;

   cmsLog_debug("Deleting %s", bridgeName);

   /* You cannot delete the Default bridge */
   if (bridgeName == NULL || strcmp(bridgeName, "Default") == 0)
   {
      return;
   }

   if ((ret = rutPMap_getBridgeByName(bridgeName, &iidStack, &bridgeObj)) == CMSRET_SUCCESS)
   {
      cmsObj_free((void **) &bridgeObj);
      if ((ret = cmsObj_deleteInstance(MDMOID_L2_BRIDGING_ENTRY, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to delete L2BridgingEntry Object, ret = %d", ret);
      }
   }
   else
   {
      cmsLog_debug("Could not find bridge %s", bridgeName);
   }

   rutMulti_reloadMcpd();

   return;
}



void rutPMap_removeInterfaceGroup(const char *fullPathName)
{
   InstanceIdStack iidStack;
   _L2BridgingEntryObject *bridgeObj=NULL;
   UINT32 bridgeKey;
   char bridgeIfName[BUFLEN_32]; 
   CmsRet ret;
   char availInterfaceReference[BUFLEN_256];
   UBOOL8 isWanUsedForIntfGroup;
   UBOOL8 prevHideObjectsPendingDelete;

   /*
    * very anoying, the TR98 spec says the fullname does not end in dot,
    * but our internal conversion routines put in the final dot, so strip
    * it out before putting into the MDM.
    */
   strncpy(availInterfaceReference, fullPathName, sizeof(availInterfaceReference));
   availInterfaceReference[strlen(availInterfaceReference)-1] = '\0';   

   /* override default MDM behavior of hiding objects that are pending delete */
   prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
   mdmLibCtx.hideObjectsPendingDelete = FALSE;

   isWanUsedForIntfGroup = rutPMap_isWanUsedForIntfGroupFullPathName(availInterfaceReference);

   mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

   if (isWanUsedForIntfGroup == FALSE)
   {
      cmsLog_debug("Not part of interface group.");
      return;
   }

   cmsLog_debug("Remove interface group entry for %s", availInterfaceReference);
   
   /* the filter object is already in mdm, so just the correct bridge ifName */
   if ((ret = rutPMap_getBridgeIfNameFromFullPathName(availInterfaceReference, bridgeIfName)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get bridgeIfname for fullPathName=%s, ret=%d", availInterfaceReference, ret);
      return;
   }
  
   /* get bridge from "br?" */
   bridgeKey = atoi((bridgeIfName+2));
   
   /* get the bridge Object from bridgeKey*/
   if ((ret = rutPMap_getBridgeByKey(bridgeKey, &iidStack,  &bridgeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Error retrieving the bridge object, ret=%d", ret);
      return;
   }
   cmsLog_debug("bridgeIfName=%s, intfGroupName=%s", bridgeIfName, bridgeObj->bridgeName);
  
   cmsLog_debug("delete all dhcp vendor id filters from bridge %s", bridgeObj->bridgeName);
   rutPmap_deleteFilterDhcpVendorId(bridgeObj->bridgeName);

   cmsLog_debug("disassociate all filter intf from bridge %s", bridgeObj->bridgeName);
   rutPMap_disassocAllFilterIntfFromBridge(bridgeObj->bridgeName); 
   
   cmsLog_debug("delete bridge %s", bridgeObj->bridgeName);
   rutPMap_deleteBridge(bridgeObj->bridgeName);

   /* free bridge object */
   cmsObj_free((void **) &bridgeObj);

   cmsLog_debug("Done remove intf group.");

   rutMulti_reloadMcpd();   
 }



UBOOL8 rutPMap_isBridgedInterfaceGroup(UINT32 BridgeKey)
{
   L2BridgingFilterObject *l2bridgefilter = NULL;
   InstanceIdStack FilterIidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 ret = FALSE;

   /* search all Filter objects with FilterBridgeRef==BridgeKey until the Filter object of WANInterface is found */
   while( cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, &FilterIidStack, (void **) &l2bridgefilter) == CMSRET_SUCCESS )
   {
      if( l2bridgefilter->filterBridgeReference == (SINT32) BridgeKey )
      {
         InstanceIdStack AvailableIntfIidStack = EMPTY_INSTANCE_ID_STACK;
         L2BridgingIntfObject *availIntfObj=NULL;
         UBOOL8 isDhcpVendorIdFilter;
         UINT32 key;

         /* extract the info that we need from the l2bridgeFilter object before freeing it. */
         cmsUtl_strtoul(l2bridgefilter->filterInterface, NULL, 0, &key);
         isDhcpVendorIdFilter = (!cmsUtl_strcmp(l2bridgefilter->filterInterface, MDMVS_LANINTERFACES));

         cmsObj_free((void **) &l2bridgefilter);

         if (isDhcpVendorIdFilter)
         {
            /* this is a DHCP vendor id filter */
            continue;
         }


         /* get the respective AvailableInterface object */
         if (rutPMap_getAvailableInterfaceByKey(key, &AvailableIntfIidStack, &availIntfObj) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not find filter interface %d", key);
            return FALSE;
         }

         /* get the WAN IPConn of the bridge and check if it's bridged interface */
         if( cmsUtl_strcmp(availIntfObj->interfaceType, MDMVS_WANINTERFACE) == 0 )
         {
            MdmObjectId oid;
            InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
            void *obj=NULL;
            WanIpConnObject *wanIpConnObj=NULL;

            if( rutPMap_availableInterfaceReferenceToMdmObject(availIntfObj->interfaceReference, &iidStack, &obj) != CMSRET_SUCCESS )
            {
               cmsLog_error("could not get interface reference: %s", availIntfObj->interfaceReference);
               cmsObj_free((void **) &availIntfObj);
               return FALSE;
            }
            cmsObj_free((void **) &availIntfObj);
            oid = GET_MDM_OBJECT_ID(obj);

            if (oid == MDMOID_WAN_IP_CONN)
            {
               wanIpConnObj = (WanIpConnObject *) obj;
               if( !cmsUtl_strcmp(wanIpConnObj->connectionType, MDMVS_IP_BRIDGED) )
               {
                  ret = TRUE;
               }
            }
            cmsObj_free((void **) &obj);
            return ret;
         }
         
         cmsObj_free((void **) &availIntfObj);
         continue;

      }
      cmsObj_free((void **) &l2bridgefilter);
   }
   cmsLog_debug("could not find the Filter object respective to WANInterface %d", BridgeKey);
   return ret;
}


CmsRet rutPMap_getSubnetFromWanIfName(const char *ifName, char *subnetCidr)
{
   CmsRet ret = CMSRET_SUCCESS;
   _L2BridgingFilterObject *pBridgeFltObj = NULL;
   _LanIpIntfObject *lanIpObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack ipIntfIid = EMPTY_INSTANCE_ID_STACK;
   char bridgeIfName[CMS_IFNAME_LENGTH];
   UBOOL8 found=FALSE;   

   if (ifName == NULL || subnetCidr == NULL)
   {
      cmsLog_error("Invalid ifName/subnetCidr %p/%p", ifName, subnetCidr);
      return CMSRET_INVALID_ARGUMENTS;
   }    
   
   /* Need to get filter object to get the bridge name ("brx") 
    * For PPPoA, IPoA, there is NO filter Object so the bridge name will be default "br0"
   */
   if ((ret = rutPMap_ifNameToFilterObject(ifName, &iidStack, &pBridgeFltObj)) == CMSRET_SUCCESS)
   {
      snprintf(bridgeIfName, sizeof(bridgeIfName), "br%d", pBridgeFltObj->filterBridgeReference);
      cmsObj_free((void **) & pBridgeFltObj);
   }
   else
   {
      if (qdmIpIntf_isWanInterfaceBridgedLocked((char *) ifName))
      {
         cmsLog_notice("%s is a bridge and has no subnet", ifName);
         return ret;
      }
      else
      {
         cmsLog_notice("%s is a pppoa or ipoa, just use the subnet from br0", ifName);
         strcpy(bridgeIfName, "br0");
      }
   }
   
   /* Search for the ip Interface object on the brx  to get LAN subnet */
   while (!found && 
      ((ret = cmsObj_getNextFlags(MDMOID_LAN_IP_INTF, &ipIntfIid, OGF_NO_VALUE_UPDATE, (void **) &lanIpObj)) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(lanIpObj->X_BROADCOM_COM_IfName, bridgeIfName))
      {
         /* IPv4 address */
         struct in_addr tmp_ip, tmp_mask, tmp_subnet;

         /* convert ipAddr/subnetMask to cidr format. eg.  192.168.1.1 and 255.255.255.0 to 192.168.1.0/24 */
         if (!inet_aton(lanIpObj->IPInterfaceIPAddress, &tmp_ip) || !inet_aton(lanIpObj->IPInterfaceSubnetMask, &tmp_mask))
         {
            cmsLog_error("ip address conversion failed on %s or %s", lanIpObj->IPInterfaceIPAddress, lanIpObj->IPInterfaceSubnetMask);
            return CMSRET_INTERNAL_ERROR;
         }
         else
         {
            tmp_subnet.s_addr = tmp_ip.s_addr & tmp_mask.s_addr;
         }
         sprintf(subnetCidr, "%s/%d", inet_ntoa(tmp_subnet), cmsNet_getLeftMostOneBitsInMask(lanIpObj->IPInterfaceSubnetMask));
         
         /* TODO: ipv6 */
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
      cmsLog_error("No LAN port in this Interface group with WAN interface %s?", ifName);
      ret = CMSRET_INTERNAL_ERROR;
   }

   cmsLog_debug("subnetCidr=%s", subnetCidr);
   
   return ret;
   
}


#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */

CmsRet rutPMap_getGroupAndBridgeNameFromWanIfName(const char *wanIfName, char *groupName,  char *bridgeName)
{
   CmsRet ret = CMSRET_SUCCESS;
   _L2BridgingFilterObject *pBridgeFltObj = NULL;
   _L2BridgingEntryObject *pBridgeObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   if (wanIfName == NULL || groupName == NULL || bridgeName == NULL)
   {
      cmsLog_error("Invalid wanIfName/groupName/bridgeName %p/%p/%p", wanIfName, groupName, bridgeName);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* Need to get filter object to get the bridge name ("brx") */
   if ((ret = rutPMap_ifNameToFilterObject(wanIfName, &iidStack, &pBridgeFltObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if ((ret = rutPMap_getBridgeByKey(pBridgeFltObj->filterBridgeReference, &iidStack, &pBridgeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Error retrieving the bridge ref %d", pBridgeFltObj->filterBridgeReference);
   }
   else
   {
      sprintf(bridgeName, "br%d", pBridgeFltObj->filterBridgeReference);
      strcpy(groupName, pBridgeObj->bridgeName);
   }

   cmsObj_free((void **) &pBridgeObj);
   cmsObj_free((void **) &pBridgeFltObj);

   return ret;

}
UBOOL8 rutPMap_getIgmpSnoopingForBridgedWanIf(const char *wanIfName)
{
   CmsRet ret = CMSRET_SUCCESS;
   char bridgeName[CMS_IFNAME_LENGTH]={0};
   char IntfGroupName[BUFLEN_64]={0};
   UBOOL8 igmpEnabled = FALSE; 

   if (wanIfName == NULL)
   {
      cmsLog_error("NULL wanIfName");
      return igmpEnabled;
   }

   /* Find the bridge name this WAN interface is attached to */
   if ((ret = rutPMap_getGroupAndBridgeNameFromWanIfName(wanIfName, IntfGroupName, bridgeName)) != CMSRET_SUCCESS)
   {
      cmsLog_debug("Could not find bridge name for <%s>\n",wanIfName);
      return igmpEnabled;
   }

   /* Get the IGMP Snooping Object for this Bridge */
   igmpEnabled = qdmMulti_isIgmpSnoopingEnabledOnBridgeLocked(bridgeName);

   cmsLog_debug("IGMP Snooping %s for <%s>:<%s>",
                (igmpEnabled?"Enabled":"Disabled"), wanIfName,bridgeName);

   return igmpEnabled;

}

#endif /* DMP_BRIDGING_1 */
