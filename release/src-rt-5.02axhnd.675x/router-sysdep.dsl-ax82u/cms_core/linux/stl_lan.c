/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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

#include "stl.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_obj.h"
#include "rut_lan.h"
#include "rut_system.h"
#include "rut_qos.h"
#include "ethswctl_api.h"




#ifdef DMP_BASELINE_1

CmsRet stl_lanDevObject(_LanDevObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dHCPConditionalServingObject(_DHCPConditionalServingObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#ifdef NOT_SUPPORTED
CmsRet stl_dHCPOptionObject(_DHCPOptionObject *obj, const InstanceIdStack *iidStack)
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_lanHostCfgObject(_LanHostCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS_OBJECT_UNCHANGED;

#ifdef DHCP_RELAY
   UBOOL8 newValue;

   if (obj->DHCPServerEnable)
   {
      newValue = (obj->X_BROADCOM_COM_DhcpRelayServer != NULL);
      if (newValue != obj->DHCPRelay)
      {
         obj->DHCPRelay = newValue;
         ret = CMSRET_SUCCESS;
      }
   }
#endif

   return ret;
}

CmsRet stl_lanIpIntfObject(_LanIpIntfObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


#ifdef DMP_X_BROADCOM_COM_SECURITY_1

CmsRet stl_lanIpIntfFirewallExceptionObject(_LanIpIntfFirewallExceptionObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE.  This handler function does not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif /* DMP_X_BROADCOM_COM_SECURITY_1 */


CmsRet stl_lanEthIntfObject(_LanEthIntfObject *obj, const InstanceIdStack *iidStack __attribute__((unused)))
{
   char currentStatus[BUFLEN_16]={0};
   char hwAddr[MAC_STR_LEN+1]={0};
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   cmsLog_debug("Entered: ifname=%s", obj->X_BROADCOM_COM_IfName);

   if (cmsUtl_strlen(obj->X_BROADCOM_COM_IfName) == 0)
   {
      /* NULL or empty IntfName, can't do anything yet. */
      return ret;
   }

   rutLan_getIntfStatus(obj->X_BROADCOM_COM_IfName, currentStatus, hwAddr);

   cmsLog_debug("obj->X_BROADCOM_COM_IfName %s, status %s, currentStatus %s, hwAddr %s",
                obj->X_BROADCOM_COM_IfName,obj->status,currentStatus,hwAddr);


   /* always update the mac addr if it has changed */
   if (cmsUtl_strcmp(obj->MACAddress, hwAddr))
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->MACAddress, hwAddr, mdmLibCtx.allocFlags);
      ret = CMSRET_SUCCESS;
   }

   if (cmsUtl_strcmp(obj->status, currentStatus))
   {
      /* link status has changed */
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, currentStatus, mdmLibCtx.allocFlags);
      ret = CMSRET_SUCCESS;

      /* if we went from link down to up, update speed and duplex info */
      if (!cmsUtl_strcmp(currentStatus, MDMVS_UP))
      {
         int speed = 0, duplex = 0;
         bcm_phy_mode_getV(obj->X_BROADCOM_COM_IfName, &speed, &duplex);
         switch (speed)
         {
            case 1000:
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->maxBitRate, MDMVS_1000, mdmLibCtx.allocFlags);
               break;
            case 100:
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->maxBitRate, MDMVS_100, mdmLibCtx.allocFlags);
               break;
            case 10:
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->maxBitRate, MDMVS_10, mdmLibCtx.allocFlags);
               break;
            default:
               REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->maxBitRate, MDMVS_AUTO, mdmLibCtx.allocFlags);
               break;
         }

         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->duplexMode,
                                     (duplex ? MDMVS_FULL : MDMVS_HALF),
                                     mdmLibCtx.allocFlags);

         /* set up QoS rules for this eth interface */
         rutQos_qMgmtQueueReconfig(obj->X_BROADCOM_COM_IfName, TRUE);

         /* Add the default classifiers back at the end of the list */
         rutQos_doDefaultPolicy();

         rutQos_tmPortShaperCfg(obj->X_BROADCOM_COM_IfName,
                                obj->X_BROADCOM_COM_ShapingRate,
                                obj->X_BROADCOM_COM_ShapingBurstSize,
                                obj->status, FALSE);
      }
   }

   return ret;
}

CmsRet stl_lanEthIntfStatsObject(_LanEthIntfStatsObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   LanEthIntfObject *ethObj=NULL;

   if (cmsObj_get(MDMOID_LAN_ETH_INTF, iidStack, 0,(void **) &ethObj) == CMSRET_SUCCESS)
   {
      if (obj == NULL)
      {
         rut_clearIntfStats(ethObj->X_BROADCOM_COM_IfName);
      }
      else
      {
         rut_getIntfStats(ethObj->X_BROADCOM_COM_IfName,&obj->bytesReceived,&obj->packetsReceived,
                          &(obj->X_BROADCOM_COM_MulticastBytesReceived), &(obj->X_BROADCOM_COM_MulticastPacketsReceived), &(obj->X_BROADCOM_COM_UnicastPacketsReceived), &(obj->X_BROADCOM_COM_BroadcastPacketsReceived), 
                          &obj->X_BROADCOM_COM_RxErrors,&obj->X_BROADCOM_COM_RxDrops,
                          &obj->bytesSent,&obj->packetsSent,
                          &(obj->X_BROADCOM_COM_MulticastBytesSent), &(obj->X_BROADCOM_COM_MulticastPacketsSent), &(obj->X_BROADCOM_COM_UnicastPacketsSent), &(obj->X_BROADCOM_COM_BroadcastPacketsSent),
                          &obj->X_BROADCOM_COM_TxErrors,&obj->X_BROADCOM_COM_TxDrops);
      }
      cmsObj_free((void **) &ethObj);
   }
   return CMSRET_SUCCESS;
}


#ifdef DMP_USBLAN_1
CmsRet stl_lanUsbIntfObject(_LanUsbIntfObject *obj, const InstanceIdStack *iidStack __attribute__((unused)))
{
   char currentStatus[BUFLEN_16];
   char hwAddr[BUFLEN_18];
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   rutLan_getIntfStatus(obj->X_BROADCOM_COM_IfName, currentStatus, hwAddr);

   cmsLog_debug("obj->X_BROADCOM_COM_IfName %s, status %s, currentStatus %s, hwAddr %s",
                obj->X_BROADCOM_COM_IfName,obj->status,currentStatus,hwAddr);


   if (cmsUtl_strcmp(obj->status, currentStatus))
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, currentStatus, mdmLibCtx.allocFlags);
      ret = CMSRET_SUCCESS;
   }

   if (cmsUtl_strcmp(obj->MACAddress, hwAddr))
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->MACAddress, hwAddr, mdmLibCtx.allocFlags);
      ret = CMSRET_SUCCESS;
   }

   return ret;
}

CmsRet stl_lanUsbIntfStatsObject(_LanUsbIntfStatsObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
  LanUsbIntfObject *usbObj=NULL;

   if (cmsObj_get(MDMOID_LAN_USB_INTF, iidStack, 0, (void **) &usbObj) == CMSRET_SUCCESS)
   {
      if (obj == NULL)
      {
         rut_clearIntfStats(usbObj->X_BROADCOM_COM_IfName);
      }
      else
      {
         rut_getIntfStats(usbObj->X_BROADCOM_COM_IfName,&obj->bytesReceived,&obj->X_BROADCOM_COM_PacketsReceived,
                          &(obj->X_BROADCOM_COM_MulticastBytesReceived), &(obj->X_BROADCOM_COM_MulticastPacketsReceived), &(obj->X_BROADCOM_COM_UnicastPacketsReceived), &(obj->X_BROADCOM_COM_BroadcastPacketsReceived), 
                          &obj->X_BROADCOM_COM_RxErrors,&obj->X_BROADCOM_COM_RxDrops,
                          &obj->bytesSent,&obj->X_BROADCOM_COM_PacketsSent,
                          &(obj->X_BROADCOM_COM_MulticastBytesSent), &(obj->X_BROADCOM_COM_MulticastPacketsSent), &(obj->X_BROADCOM_COM_UnicastPacketsSent), &(obj->X_BROADCOM_COM_BroadcastPacketsSent),
                          &obj->X_BROADCOM_COM_TxErrors,&obj->X_BROADCOM_COM_TxDrops);
      }
      cmsObj_free((void **) &usbObj);
   }
   return CMSRET_SUCCESS;
}

#endif /* DMP_USBLAN_1 */

#endif  /* DMP_BASELINE_1 */




#ifdef DMP_X_BROADCOM_COM_EPON_1

CmsRet stl_lanEponIntfObject(_LanEponIntfObject *obj, const InstanceIdStack *iidStack __attribute__((unused)))
{
   char currentStatus[BUFLEN_16];
   char hwAddr[BUFLEN_18];
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   rutLan_getIntfStatus(obj->ifName, currentStatus, hwAddr);

   cmsLog_debug("obj->ifName %s, status %s, currentStatus %s, hwAddr %s",
                obj->ifName,obj->status,currentStatus,hwAddr);


   if (cmsUtl_strcmp(obj->status, currentStatus))
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, currentStatus, mdmLibCtx.allocFlags);
      ret = CMSRET_SUCCESS;
   }

   if (cmsUtl_strcmp(obj->MACAddress, hwAddr))
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->MACAddress, hwAddr, mdmLibCtx.allocFlags);
      ret = CMSRET_SUCCESS;
   }

   return ret;
}

CmsRet stl_lanEponIntfStatsObject(_LanEponIntfStatsObject *obj, const InstanceIdStack *iidStack)
{
   _LanEponIntfObject *eponObj=NULL;

   if (cmsObj_get(MDMOID_LAN_EPON_INTF, iidStack, 0, (void **) &eponObj) == CMSRET_SUCCESS)
   {
      if (obj == NULL)
      {
         rut_clearIntfStats(eponObj->ifName);
      }
      else
      {
         rut_getIntfStats(eponObj->ifName,&obj->bytesReceived,&obj->packetsReceived,
                          &(obj->X_BROADCOM_COM_MulticastBytesReceived), &(obj->X_BROADCOM_COM_MulticastPacketsReceived), &(obj->X_BROADCOM_COM_UnicastPacketsReceived), &(obj->X_BROADCOM_COM_BroadcastPacketsReceived), 
                          &obj->rxErrors,&obj->rxDrops,
                          &obj->bytesSent,&obj->packetsSent,
                          &(obj->X_BROADCOM_COM_MulticastBytesSent), &(obj->X_BROADCOM_COM_MulticastPacketsSent), &(obj->X_BROADCOM_COM_UnicastPacketsSent), &(obj->X_BROADCOM_COM_BroadcastPacketsSent),
                          &obj->txErrors,&obj->txDrops);
      }
      cmsObj_free((void **) &eponObj);
   }
   return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_EPON_1 */



#ifdef DMP_BASELINE_1

CmsRet stl_lanHostsObject(_LanHostsObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE.  This handler function does not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_lanHostEntryObject(_LanHostEntryObject *obj, const InstanceIdStack *iidStack)
{
   _LanDevObject *lanDevObj=NULL;
   _LanIpIntfObject *ipIntfObj=NULL;
   InstanceIdStack parentIidStack = *iidStack;
   InstanceIdStack searchIidStack = EMPTY_INSTANCE_ID_STACK;
   char buf[sizeof(CmsMsgHeader) + sizeof(GetLeaseTimeRemainingMsgBody)] = {0};
   CmsMsgHeader *hdr = (CmsMsgHeader *) buf;
   GetLeaseTimeRemainingMsgBody *body = (GetLeaseTimeRemainingMsgBody *) (hdr + 1);
   CmsRet ret;

   /* or alternatively, set bounceiIfNotRunningFlag in the CMS message header */
   if (rutLan_isDhcpdEnabled() == FALSE)
   {
      cmsLog_debug("No query of time remaining for IPAddr=%s macAddr=%s.  DHCP disabled", obj->IPAddress, obj->MACAddress);
      obj->leaseTimeRemaining = 0;
      return CMSRET_SUCCESS;
   }

   cmsLog_debug("querying time remaining for IPAddr=%s macAddr=%s", obj->IPAddress, obj->MACAddress);

   if (obj->IPAddress == NULL || obj->MACAddress == NULL)
   {
      /* we are being read immediately after the obj create, so no IP&MAC address yet */
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }


   if ((ret = cmsObj_getAncestor(MDMOID_LAN_DEV, MDMOID_LAN_HOST_ENTRY, &parentIidStack, (void **) &lanDevObj)) != CMSRET_SUCCESS)
   {
      /* very unlikely */
      cmsLog_error("could not find ancestor lan dev, ret=%d", ret);
      return ret;
   }
    
   cmsObj_free((void **) &lanDevObj);


   /* just get the first IP Intf object in the LAN Device */
   if ((ret = cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF, &parentIidStack, &searchIidStack, (void **) &ipIntfObj)) != CMSRET_SUCCESS)
   {
      /* very unlikely */
      cmsLog_error("could not find ip intf ret=%d", ret);
      return ret;
   }


   hdr->type = CMS_MSG_GET_LEASE_TIME_REMAINING;
   hdr->src = mdmLibCtx.eid;
   hdr->dst = EID_DHCPD;
   hdr->flags_request = 1;
   hdr->dataLength = sizeof(GetLeaseTimeRemainingMsgBody);

   snprintf(body->ifName, sizeof(body->ifName), "%s", ipIntfObj->X_BROADCOM_COM_IfName);
   snprintf(body->macAddr, sizeof(body->macAddr), "%s", obj->MACAddress);

   cmsObj_free((void **) &ipIntfObj);

#if defined(DESKTOP_LINUX) && !defined(BUILD_DESKTOP_BEEP) 
   /* dhcpd is not supported on the desktop yet, so can't send a request and expect a reply */
   obj->leaseTimeRemaining = 80123;
#else
   {
      UINT32 timeout = CMSLCK_MAX_HOLDTIME;
      obj->leaseTimeRemaining = cmsMsg_sendAndGetReplyWithTimeout(mdmLibCtx.msgHandle, hdr, timeout);
   }
#endif

   return CMSRET_SUCCESS;
}

#endif  /* DMP_BASELINE_1 */




#ifdef DMP_X_BROADCOM_COM_IGMPSNOOP_1
CmsRet stl_igmpSnoopingCfgObject(_IgmpSnoopingCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE.  This handler function does not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif


#ifdef DMP_X_BROADCOM_COM_MLDSNOOP_1
CmsRet stl_mldSnoopingCfgObject(_MldSnoopingCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE.  This handler function does not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
