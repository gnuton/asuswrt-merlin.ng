/*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
:>
*/

#include "stl.h"
#include "cms_util.h"
#include "rut_lan.h"
#include "rut_wan.h"
#include "rut_dsl.h"
#include "rut_qos.h"
#include "rut_system.h"
#include "rut_wifiwan.h"
#include "rut_wanlayer2.h"

#include "ethctl_api.h"


CmsRet stl_wanDevObject(_WanDevObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


#ifdef DMP_ETHERNETWAN_1 /* aka SUPPORT_ETHWAN */

CmsRet stl_wanEthLinkCfgObject(_WanEthLinkCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS_OBJECT_UNCHANGED;
   _WanEthIntfObject *wanEthIntf=NULL;
   InstanceIdStack parentIid=*iidStack;

   if ((ret = cmsObj_getAncestor(MDMOID_WAN_ETH_INTF, MDMOID_WAN_ETH_LINK_CFG, &parentIid, (void **) &wanEthIntf)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor wanEthIntfObj, ret=%d", ret);
      return ret;
   }

   /* Set up WanEthernetLinkConfig.EthernetLinkStatus status */
   if (!cmsUtl_strcmp(wanEthIntf->status, MDMVS_UP))
   {
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->ethernetLinkStatus, MDMVS_UP, mdmLibCtx.allocFlags);
   }
   else
   {
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->ethernetLinkStatus, MDMVS_DOWN, mdmLibCtx.allocFlags);
   }

   ret = CMSRET_SUCCESS;
   
   cmsObj_free((void **) &wanEthIntf);

   return ret;
   
}

CmsRet stl_wanEthIntfObject(_WanEthIntfObject *obj,
                   const InstanceIdStack *iidStack __attribute__((unused)))
{
   char currentStatus[BUFLEN_16]={0};
   char hwAddr[MAC_STR_LEN+1]={0};
   CmsRet ret=CMSRET_SUCCESS_OBJECT_UNCHANGED;

   cmsLog_debug("Entered: ifname=%s", obj->X_BROADCOM_COM_IfName);

   /* TODO: We should avoid to use ifname */
   /* we enable it after we set wan connection. 
   * Check ifname for layer2 iface be created */
   /* if (obj->enable) */
   if (obj->X_BROADCOM_COM_IfName)
   {
      /* check for status change */
      rutLan_getIntfStatus(obj->X_BROADCOM_COM_IfName, currentStatus, hwAddr);

      /* always update the mac addr if it has changed */
      if (cmsUtl_strcmp(obj->MACAddress, hwAddr))
      {
         CMSMEM_REPLACE_STRING_FLAGS(obj->MACAddress, hwAddr, mdmLibCtx.allocFlags);
         ret = CMSRET_SUCCESS;
      }

      if (cmsUtl_strcmp(obj->status, currentStatus))
      {
         /* link status has changed */

         if (!strcmp(currentStatus, MDMVS_UP) || !strcmp(obj->status, MDMVS_NOLINK))
         {
            CmsRet r2;

            rutLan_enableInterface(obj->X_BROADCOM_COM_IfName);
            if ((r2 = rutQos_tmPortInit(obj->X_BROADCOM_COM_IfName, TRUE)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_tmPortInit() returns error. ret=%d", r2);
            }
            else
            {
               rutQos_qMgmtQueueReconfig(obj->X_BROADCOM_COM_IfName, TRUE);
            }
         }
//         else if (!strcmp(obj->status, MDMVS_UP))
//         {
//            CmsRet r2;
//            if ((r2 = rutQos_tmPortUninit(obj->X_BROADCOM_COM_IfName, TRUE)) != CMSRET_SUCCESS)
//            {
//               cmsLog_error("rutQos_tmPortUninit() returns error. ret=%d", r2);
//            }
//         }

         CMSMEM_REPLACE_STRING_FLAGS(obj->status, currentStatus, mdmLibCtx.allocFlags);
         ret = CMSRET_SUCCESS;
      }
   }


#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   /* 
    * IPv6 part: Assumption => There will be maximum one GUA and one LLA (no ULA) on WAN interface.
    */
   if (obj->X_BROADCOM_COM_IfName)
   {
      _IPv6AddrObject *ipv6AddrObj = NULL;
      char ipAddr[CMS_IPADDR_LENGTH];
      UINT32 ifIndex   = 0;
      UINT32 addrIdx   = 0;
      UINT32 prefixLen = 0;
      UINT32 scope     = 0;
      UINT32 ifaFlags  = 0;

      cmsLog_debug("IPv6 Enter");

      while (cmsNet_getIfAddr6(obj->X_BROADCOM_COM_IfName, addrIdx,
                               ipAddr, &ifIndex, &prefixLen, &scope, &ifaFlags) == CMSRET_SUCCESS)
      {
         UBOOL8 found = FALSE;
         char scope_text[BUFLEN_4];
         char address[CMS_IPADDR_LENGTH];
         InstanceIdStack ipv6AddriidStack = EMPTY_INSTANCE_ID_STACK;

         sprintf(address, "%s/%d", ipAddr, prefixLen);

         if ( scope == 0x0020U /*IPV6_ADDR_LINKLOCAL*/ )
         {
            strcpy(scope_text, MDMVS_LLA);
         }
         else
         {
            strcpy(scope_text, MDMVS_GUA);
         }

         cmsLog_debug("address<%s>   scope<%x %s>", address, scope, scope_text);
         
         while ( !found && 
                    (cmsObj_getNextInSubTreeFlags(MDMOID_I_PV6_ADDR, iidStack, &ipv6AddriidStack, 
                                                             OGF_NO_VALUE_UPDATE, (void **)&ipv6AddrObj) == CMSRET_SUCCESS) )
         {
            cmsLog_debug("ipv6AddrObj: addr<%s>   scope<%s>", ipv6AddrObj->IPv6Address, ipv6AddrObj->scope);

            if ( !cmsUtl_strcmp(scope_text, ipv6AddrObj->scope) )
            {
               found = TRUE;

               /* scope is the same but address is not. We need to update the address */
               if ( cmsUtl_strcmp(address, ipv6AddrObj->IPv6Address) )
               {
                  CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->IPv6Address, address, mdmLibCtx.allocFlags);

                  if ((ret = cmsObj_set(ipv6AddrObj, &ipv6AddriidStack)) != CMSRET_SUCCESS)
                  {
                     cmsLog_error("failed to set IPv6AddrObject");
                     cmsObj_free((void **) &ipv6AddrObj);
                     return ret;
                  }
               }
            }

            cmsObj_free((void **) &ipv6AddrObj);
         }

         /* if this is a new scoped address, add it */
         if ( !found )
         {
            ipv6AddriidStack = *iidStack;
            cmsLog_debug("It's new, add it");

            if ((ret = cmsObj_addInstance(MDMOID_I_PV6_ADDR, &ipv6AddriidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("could not create new ipv6 address entry (L2), ret=%d", ret);
               return ret;
            }

            if ((ret = cmsObj_get(MDMOID_I_PV6_ADDR, &ipv6AddriidStack, OGF_NO_VALUE_UPDATE, (void **) &ipv6AddrObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to get IPv6AddrObject, ret=%d", ret);
               cmsObj_deleteInstance(MDMOID_I_PV6_ADDR, &ipv6AddriidStack);
               return ret;
            }

            CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->IPv6Address, address, mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->scope, scope_text, mdmLibCtx.allocFlags);

            ret = cmsObj_set(ipv6AddrObj, &ipv6AddriidStack);
            cmsObj_free((void **) &ipv6AddrObj);            
         }

         addrIdx++;
      }
   }
#endif

   return ret;
}


CmsRet stl_ipv6AddrObject(_IPv6AddrObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;   
}


CmsRet stl_wanEthIntfStatsObject(_WanEthIntfStatsObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   _WanEthIntfObject *wanEthIntf=NULL;
   if ((cmsObj_get(MDMOID_WAN_ETH_INTF, iidStack,  0, (void **) &wanEthIntf)) == CMSRET_SUCCESS)
   {
      if (obj == NULL)
      {
         rut_clearWanIntfStats(wanEthIntf->X_BROADCOM_COM_IfName);
      }
      else
      {
         rut_getIntfStats(wanEthIntf->X_BROADCOM_COM_IfName,
                          &obj->bytesReceived,&obj->packetsReceived,
                          &(obj->X_BROADCOM_COM_MulticastBytesReceived), &(obj->X_BROADCOM_COM_MulticastPacketsReceived), &(obj->X_BROADCOM_COM_UnicastPacketsReceived), &(obj->X_BROADCOM_COM_BroadcastPacketsReceived), 
                          &obj->X_BROADCOM_COM_RxErrors,&obj->X_BROADCOM_COM_RxDrops,
                          &obj->bytesSent,&obj->packetsSent,
                          &(obj->X_BROADCOM_COM_MulticastBytesSent), &(obj->X_BROADCOM_COM_MulticastPacketsSent), &(obj->X_BROADCOM_COM_UnicastPacketsSent), &(obj->X_BROADCOM_COM_BroadcastPacketsSent),
                          &obj->X_BROADCOM_COM_TxErrors,&obj->X_BROADCOM_COM_TxDrops);
      }
      cmsObj_free((void **) &wanEthIntf);
      return CMSRET_SUCCESS;
   }

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif  /* DMP_ETHERNETWAN_1 aka SUPPORT_ETHWAN */


CmsRet stl_wanCommonIntfCfgObject(_WanCommonIntfCfgObject *obj, const InstanceIdStack *iidStack)
{
   CmsRet ret = CMSRET_SUCCESS;

   if (obj == NULL)
   {
      /* this is the delete case, do nothing */
      return ret;
   }

   if( !cmsUtl_strcmp(obj->WANAccessType, MDMVS_DSL) )
   {
#ifdef DMP_ADSLWAN_1   
      _WanDslIntfCfgObject *dslIntfCfgObj=NULL;


      if ((ret = cmsObj_get(MDMOID_WAN_DSL_INTF_CFG, iidStack, 0, (void **) &dslIntfCfgObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get DSL_INTF_CFG object, ret=%d", ret);
         return ret;
      }

      if (!cmsUtl_strcmp(dslIntfCfgObj->status, MDMVS_UP))
      {
         cmsLog_debug("BCM_XDSL_LINK_UP (0)");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->physicalLinkStatus, MDMVS_UP, mdmLibCtx.allocFlags);
      }
      else if (!cmsUtl_strcmp(dslIntfCfgObj->status, MDMVS_DOWN))
      {
         cmsLog_debug("BCM_XDSL_LINK_DOWN (1)");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->physicalLinkStatus, MDMVS_DOWN, mdmLibCtx.allocFlags);
      }
      else if (!cmsUtl_strcmp(dslIntfCfgObj->status, MDMVS_INITIALIZING) ||
               !cmsUtl_strcmp(dslIntfCfgObj->status,  MDMVS_ESTABLISHINGLINK))
      {
         cmsLog_debug("xdsl initialzing");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->physicalLinkStatus, MDMVS_INITIALIZING, mdmLibCtx.allocFlags);
      }
      else
      {
         cmsLog_debug("xdsl unavailable");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->physicalLinkStatus, MDMVS_UNAVAILABLE, mdmLibCtx.allocFlags);
      }


      obj->layer1UpstreamMaxBitRate = 0;
      obj->layer1DownstreamMaxBitRate = 0;
      
      if ( dslIntfCfgObj->enable && !cmsUtl_strcmp(dslIntfCfgObj->status, MDMVS_UP))
      {
          obj->layer1UpstreamMaxBitRate = dslIntfCfgObj->upstreamMaxRate * 1000;
          obj->layer1DownstreamMaxBitRate = dslIntfCfgObj->downstreamMaxRate * 1000;
      }
#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
      // [JIRA SWBCACPE-20960]: Add up Bonding Peer's rate if EnableBonding
      if ( dslIntfCfgObj->X_BROADCOM_COM_EnableBonding )
      {
          WanDslIntfCfgObject *pBondingDslIntfObj = NULL;
          InstanceIdStack bondingIidStack = EMPTY_INSTANCE_ID_STACK;

          ret = rutWl2_getBondingDslIntfObjectByPeerName(dslIntfCfgObj->X_BROADCOM_COM_BondingPeerName, 
                                                         &bondingIidStack, 
                                                         &pBondingDslIntfObj);
          if (ret == CMSRET_SUCCESS)
          {
              if (pBondingDslIntfObj && pBondingDslIntfObj->X_BROADCOM_COM_EnableBonding &&
                      !cmsUtl_strcmp(pBondingDslIntfObj->status, MDMVS_UP)) {
                  obj->layer1UpstreamMaxBitRate += (pBondingDslIntfObj->upstreamMaxRate * 1000);
                  obj->layer1DownstreamMaxBitRate += (pBondingDslIntfObj->downstreamMaxRate * 1000);
              } 
              cmsObj_free((void **) &pBondingDslIntfObj);
          }
      }
#endif

#ifdef later
      /* most stats params come from the stats object, not the DslIntfCfg object */

      obj->totalBytesSent = dslIntfCfgObj->X_BROADCOM_COM_UpstreamTotalCells;
      obj->totalBytesReceived = dslIntfCfgObj->X_BROADCOM_COM_DownstreamDataCells;
      obj->totalPacketsSent = dslIntfCfg->transmitBlocks;
      obj->totalPacketsReceived = dslIntfCfg->receiveBlocks;

      obj->X_BROADCOM_COM_TxErrors = adslMib.adslStat.xmtStat.cntSFErr;
      obj->X_BROADCOM_COM_RxErrors = adslMib.adslStat.rcvStat.cntSFErr;
      obj->X_BROADCOM_COM_TxDrops = adslMib.atmStat.xmtStat.cntCellDrop;
      obj->X_BROADCOM_COM_RxDrops = adslMib.atmStat.rcvStat.cntCellDrop;
#endif

      cmsObj_free((void **) &dslIntfCfgObj);

#endif /* DMP_ADSLWAN_1 */

   }
   else if (!cmsUtl_strcmp(obj->WANAccessType, MDMVS_ETHERNET))
   {
#ifdef DMP_ETHERNETWAN_1
      _WanEthIntfObject *ethIntfObj=NULL;

      if ((ret = cmsObj_get(MDMOID_WAN_ETH_INTF, iidStack, 0, (void **) &ethIntfObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get ETH_INTF object, ret=%d", ret);
         return ret;
      }

      if (!cmsUtl_strcmp(ethIntfObj->status, MDMVS_UP))
      {
         cmsLog_debug("wan eth up");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->physicalLinkStatus, MDMVS_UP, mdmLibCtx.allocFlags);
      }
      else
      {
         cmsLog_debug("wan eth down");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->physicalLinkStatus, MDMVS_DOWN, mdmLibCtx.allocFlags);
      }

      /* Use hard-coded value once the maxBitRate is not configured or in auto mode */
      if (ethIntfObj->maxBitRate != NULL && (cmsUtl_strcmp(ethIntfObj->maxBitRate, MDMVS_AUTO)))
      {
         obj->layer1UpstreamMaxBitRate = atoi(ethIntfObj->maxBitRate);
         obj->layer1DownstreamMaxBitRate = atoi(ethIntfObj->maxBitRate);
      }
      else if (!cmsUtl_strcmp(ethIntfObj->maxBitRate, MDMVS_AUTO))
      {
         int speed = 0, duplex = 0, subport = 0;
         enum phy_cfg_flag phycfg;
         if (ethIntfObj->X_BROADCOM_COM_IfName)
         {
            bcm_get_linkspeed(ethIntfObj->X_BROADCOM_COM_IfName, &speed, &duplex, &phycfg, &subport);
         }
         obj->layer1UpstreamMaxBitRate = speed * 1000000;
         obj->layer1DownstreamMaxBitRate = speed * 1000000;
      }
      else
      {
         obj->layer1UpstreamMaxBitRate = 0;
         obj->layer1DownstreamMaxBitRate = 0;
      }

      /* don't know about the other statistics.  They are available on the LAN side.
       * wan side eth intf object should have them too.
       */

      cmsObj_free((void **) &ethIntfObj);
#endif
   }
   else if (!cmsUtl_strcmp(obj->WANAccessType, MDMVS_X_BROADCOM_COM_MOCA))
   {
#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1
      _WanMocaIntfObject *mocaIntfObj=NULL;

      if ((ret = cmsObj_get(MDMOID_WAN_MOCA_INTF, iidStack, 0, (void **) &mocaIntfObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get MOCA_INTF object, ret=%d", ret);
         return ret;
      }

      if (!cmsUtl_strcmp(mocaIntfObj->status, MDMVS_UP))
      {
         cmsLog_debug("wan moca up");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->physicalLinkStatus, MDMVS_UP, mdmLibCtx.allocFlags);
      }
      else
      {
         cmsLog_debug("wan moca down");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->physicalLinkStatus, MDMVS_DOWN, mdmLibCtx.allocFlags);
      }

      /* fill in other statistics later */
      cmsObj_free((void **) &mocaIntfObj);
#endif
   }
   else if (!cmsUtl_strcmp(obj->WANAccessType, MDMVS_X_BROADCOM_COM_L2TPAC))
   {
#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
      /* cwu_todo: linkStatus */
#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */
   }
   else if (!cmsUtl_strcmp(obj->WANAccessType, MDMVS_X_BROADCOM_COM_PON))
   {
#ifdef DMP_X_BROADCOM_COM_PONWAN_1
      _WanPonIntfObject *ponIntfObj=NULL;

      if ((ret = cmsObj_get(MDMOID_WAN_PON_INTF, iidStack, 0, (void **) &ponIntfObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get PON_INTF object, ret=%d", ret);
         return ret;
      }

      if (!cmsUtl_strcmp(ponIntfObj->status, MDMVS_UP))
      {
         cmsLog_debug("PON wan up");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->physicalLinkStatus, MDMVS_UP, mdmLibCtx.allocFlags);
      }
      else
      {
         cmsLog_debug("PON wan down");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->physicalLinkStatus, MDMVS_DOWN, mdmLibCtx.allocFlags);
      }

      cmsObj_free((void **) &ponIntfObj);
#endif /* DMP_X_BROADCOM_COM_PONWAN_1 */
   }
   else if (!cmsUtl_strcmp(obj->WANAccessType, MDMVS_X_BROADCOM_COM_WIFI))
   {
#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
      _WanWifiIntfObject *wifiIntfObj=NULL;
     
      if ((ret = cmsObj_get(MDMOID_WAN_WIFI_INTF, iidStack, 0, (void **) &wifiIntfObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get WIFI_INTF object, ret=%d", ret);
         return ret;
      }
     
      if (!cmsUtl_strcmp(wifiIntfObj->status, MDMVS_UP))
      {
         cmsLog_debug("wan wifi up");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->physicalLinkStatus, MDMVS_UP, mdmLibCtx.allocFlags);
      }
      else
      {
         cmsLog_debug("wan wifi down");
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->physicalLinkStatus, MDMVS_DOWN, mdmLibCtx.allocFlags);
      }

      /* don't know about the other statistics.  They are available on the LAN side.
       * wan side wl intf object should have them too.
       */

      cmsObj_free((void **) &wifiIntfObj);
#endif
   }      
   else
   {
      cmsLog_error("unsupported or unrecognized WANAccessType %s", obj->WANAccessType);

   }


   obj->numberOfActiveConnections = rutWan_getNumberOfActiveConnections(iidStack);
   
   return ret;
}

CmsRet stl_wanConnDeviceObject(_WanConnDeviceObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}



CmsRet stl_wanIpConnObject(_WanIpConnObject *obj, const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS_OBJECT_UNCHANGED;

   if (cmsUtl_strcmp(obj->connectionStatus, MDMVS_CONNECTED) != 0)   
   {
      /* reset uptime  only if it not 0.  Note: when first time modem initailizing, connectionStatus == 
       * "Unconfigured" and uptime is 0. The ret should still be  CMSRET_SUCCESS_OBJECT_UNCHANGED
       * instead of being setted to CMSRET_SUCCESS.
       */
      if (obj->uptime != 0)
      {
         obj->uptime = 0;
         ret = CMSRET_SUCCESS;
      }
   }
   else
   {
      obj->uptime = cmsTms_getSeconds() - obj->X_BROADCOM_COM_ConnectionEstablishedTime;

      ret = CMSRET_SUCCESS;
   }
   return ret;
}

CmsRet stl_wanIpConnPortmappingObject(_WanIpConnPortmappingObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wanIpConnPortTriggeringObject(_WanIpConnPortTriggeringObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#ifdef DMP_X_BROADCOM_COM_SECURITY_1
CmsRet stl_wanIpConnFirewallExceptionObject(_WanIpConnFirewallExceptionObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE.  This handler function does not need to do anything. */
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_wanIpConnStatsObject(_WanIpConnStatsObject *obj, const InstanceIdStack *iidStack)
{
   WanIpConnObject  *wanIpConn = NULL;

   if (cmsObj_get(MDMOID_WAN_IP_CONN, iidStack, OGF_NO_VALUE_UPDATE, (void **) &wanIpConn) == CMSRET_SUCCESS)
   {
      if (obj == NULL)
      {
         rut_clearWanIntfStats(wanIpConn->X_BROADCOM_COM_IfName);
      }
      else
      {
         rut_getIntfStats(wanIpConn->X_BROADCOM_COM_IfName,
                          &obj->ethernetBytesReceived,&obj->ethernetPacketsReceived,
                          &(obj->X_BROADCOM_COM_MulticastBytesReceived), &(obj->X_BROADCOM_COM_MulticastPacketsReceived), &(obj->X_BROADCOM_COM_UnicastPacketsReceived), &(obj->X_BROADCOM_COM_BroadcastPacketsReceived), 
                          &obj->X_BROADCOM_COM_RxErrors,&obj->X_BROADCOM_COM_RxDrops,
                          &obj->ethernetBytesSent,&obj->ethernetPacketsSent,
                          &(obj->X_BROADCOM_COM_MulticastBytesSent), &(obj->X_BROADCOM_COM_MulticastPacketsSent), &(obj->X_BROADCOM_COM_UnicastPacketsSent), &(obj->X_BROADCOM_COM_BroadcastPacketsSent),
                          &obj->X_BROADCOM_COM_TxErrors,&obj->X_BROADCOM_COM_TxDrops);
      }
      cmsObj_free((void **) &wanIpConn);
      return CMSRET_SUCCESS;
   }
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wanPppConnObject(_WanPppConnObject *obj , const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS_OBJECT_UNCHANGED;

   if (cmsUtl_strcmp(obj->connectionStatus, MDMVS_CONNECTED) != 0)   
   {
      /* reset uptime  only if it not 0.  Note: when first time modem initailizing, connectionStatus == 
       * "Unconfigured" and uptime is 0. The ret should still be  CMSRET_SUCCESS_OBJECT_UNCHANGED
       * instead of being setted to CMSRET_SUCCESS.
       */
      if (obj->uptime != 0)
      {
         obj->uptime = 0;
         ret = CMSRET_SUCCESS;
      }
   }
   else
   {
      obj->uptime = cmsTms_getSeconds() - obj->X_BROADCOM_COM_ConnectionEstablishedTime;

      ret = CMSRET_SUCCESS;
   }
   return ret;
}

CmsRet stl_wanPppConnPortmappingObject(_WanPppConnPortmappingObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wanPppConnPortTriggeringObject(_WanPppConnPortTriggeringObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#ifdef DMP_X_BROADCOM_COM_SECURITY_1
CmsRet stl_wanPppConnFirewallExceptionObject(_WanPppConnFirewallExceptionObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE.  This handler function does not need to do anything. */
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_wanPppConnStatsObject(_WanPppConnStatsObject *obj, const InstanceIdStack *iidStack)
{
   WanPppConnObject  *wanPppConn = NULL;

   if (cmsObj_get(MDMOID_WAN_PPP_CONN, iidStack, OGF_NO_VALUE_UPDATE, (void **) &wanPppConn) == CMSRET_SUCCESS)
   {
      if (obj == NULL)
      {
         rut_clearWanIntfStats(wanPppConn->X_BROADCOM_COM_IfName);
      }
      else
      {
         rut_getIntfStats(wanPppConn->X_BROADCOM_COM_IfName,
                          &obj->ethernetBytesReceived,&obj->ethernetPacketsReceived,
                          &(obj->X_BROADCOM_COM_MulticastBytesReceived), &(obj->X_BROADCOM_COM_MulticastPacketsReceived), &(obj->X_BROADCOM_COM_UnicastPacketsReceived), &(obj->X_BROADCOM_COM_BroadcastPacketsReceived), 
                          &obj->X_BROADCOM_COM_RxErrors,&obj->X_BROADCOM_COM_RxDrops,
                          &obj->ethernetBytesSent,&obj->ethernetPacketsSent,
                          &(obj->X_BROADCOM_COM_MulticastBytesSent), &(obj->X_BROADCOM_COM_MulticastPacketsSent), &(obj->X_BROADCOM_COM_UnicastPacketsSent), &(obj->X_BROADCOM_COM_BroadcastPacketsSent),
                          &obj->X_BROADCOM_COM_TxErrors,&obj->X_BROADCOM_COM_TxDrops);
      }
      cmsObj_free((void **) &wanPppConn);
      return CMSRET_SUCCESS;
   }
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}



#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
CmsRet stl_l2tpAcIntfConfigObject(_L2tpAcIntfConfigObject *obj __attribute__((unused)),
                              const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_l2tpAcLinkConfigObject(_L2tpAcLinkConfigObject *obj __attribute__((unused)),
                              const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_l2tpAcClientCfgObject(_L2tpAcClientCfgObject *obj __attribute__((unused)),
                              const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */

#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1 

CmsRet stl_wanWifiLinkCfgObject(_WanWifiLinkCfgObject *obj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_wanWifiIntfObject(_WanWifiIntfObject *obj,
                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   char currentStatus[BUFLEN_16]={0};
   char hwAddr[BUFLEN_32]={0};
   CmsRet ret=CMSRET_SUCCESS_OBJECT_UNCHANGED;

   cmsLog_debug("Entering...");
   if (obj->ifName)
   {
      /*
       * Get latest Wifi status.  Note for Wifi WAN, we cannot use
       * rutNet_isInterfaceLinkUp.  Need to know the association (actually
       * authentication) state.
       */
      rutWifiWan_getIntfStatus(obj->ifName, currentStatus);

      /* change object only if the status has changed */
      if (cmsUtl_strcmp(obj->status, currentStatus))
      {
         cmsLog_debug("status %s -> %s", obj->status, currentStatus);
         CMSMEM_REPLACE_STRING_FLAGS(obj->status, currentStatus, mdmLibCtx.allocFlags);
         ret = CMSRET_SUCCESS; /* let mdm know we changed the string */

         /* if status is not up (i.e. down), free and null the mac addr */
         if (cmsUtl_strcmp(obj->status, MDMVS_UP))
         {
            CMSMEM_FREE_BUF_AND_NULL_PTR(obj->MACAddress);
         }

         /* if status is UP and we don't have a mac address yet, set it */
         if (!cmsUtl_strcmp(obj->status, MDMVS_UP) &&
             (obj->MACAddress == NULL))
         {
            CmsRet r2;
            r2 = cmsNet_getMacAddrStringByIfname(obj->ifName, hwAddr);
            if (r2 != CMSRET_SUCCESS)
            {
               cmsLog_error("Could not get mac address for %s, r2=%d",
                            obj->ifName, r2);
            }
            else
            {
               CMSMEM_REPLACE_STRING_FLAGS(obj->MACAddress, hwAddr, mdmLibCtx.allocFlags);
            }
         }

         /* special hack for wifi0 as WAN: during dynamic config we need to
          * ifconfig wifi0 up after the underlying wl0 interface is
          * up/associated.
          */
         if (!cmsUtl_strcmp(obj->status, MDMVS_UP))
         {
            char intfNameBuf[CMS_IFNAME_LENGTH]={0};
            char cmdStr[BUFLEN_128];
            CmsRet r2;

            r2 = rutWifiWan_getLayer3IntfNameByIidStack(iidStack, intfNameBuf);
            if (r2 == CMSRET_SUCCESS)
            {
               if (cmsNet_isInterfaceExist(intfNameBuf))
               {
                  snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s up", intfNameBuf);
                  rut_doSystemAction("stl_wanWifi", cmdStr);
               }
            }
         }

         /* enable/disable QoS queues and classification rules on this intf*/
         if (!cmsUtl_strcmp(obj->status, MDMVS_UP))
         {
            rutQos_setDefaultWlQueues(obj->ifName, TRUE);
         }
         else
         {
            rutQos_setDefaultWlQueues(obj->ifName, FALSE);
         }
      }
   }

#ifdef DMP_X_BROADCOM_COM_IPV6_1
   /* 
    * IPv6 part: Assumption => There will be maximum one GUA and one LLA (no ULA) on WAN interface.
    */
   if (obj->ifName)
   {
      _IPv6AddrObject *ipv6AddrObj = NULL;
      InstanceIdStack ipv6AddriidStack = EMPTY_INSTANCE_ID_STACK;
      char ipAddr[CMS_IPADDR_LENGTH];
      UINT32 ifIndex   = 0;
      UINT32 addrIdx   = 0;
      UINT32 prefixLen = 0;
      UINT32 scope     = 0;
      UINT32 ifaFlags  = 0;

      cmsLog_debug("IPv6 Enter");

      while (cmsNet_getIfAddr6(obj->ifName, addrIdx,
                               ipAddr, &ifIndex, &prefixLen, &scope, &ifaFlags) == CMSRET_SUCCESS)
      {
         UBOOL8 found = FALSE;
         char scope_text[BUFLEN_4];
         char address[CMS_IPADDR_LENGTH];
         InstanceIdStack ipv6AddriidStack = EMPTY_INSTANCE_ID_STACK;

         sprintf(address, "%s/%d", ipAddr, prefixLen);

         if ( scope == 0x0020U /*IPV6_ADDR_LINKLOCAL*/ )
         {
            strcpy(scope_text, MDMVS_LLA);
         }
         else
         {
            strcpy(scope_text, MDMVS_GUA);
         }

         cmsLog_debug("address<%s>   scope<%x %s>", address, scope, scope_text);
         
         while ( !found && 
                    (cmsObj_getNextInSubTreeFlags(MDMOID_I_PV6_ADDR, iidStack, &ipv6AddriidStack, 
                                                             OGF_NO_VALUE_UPDATE, (void **)&ipv6AddrObj) == CMSRET_SUCCESS) )
         {
            cmsLog_debug("ipv6AddrObj: addr<%s>   scope<%s>", ipv6AddrObj->IPv6Address, ipv6AddrObj->scope);

            if ( !cmsUtl_strcmp(scope_text, ipv6AddrObj->scope) )
            {
               found = TRUE;

               /* scope is the same but address is not. We need to update the address */
               if ( cmsUtl_strcmp(address, ipv6AddrObj->IPv6Address) )
               {
                  CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->IPv6Address, address, mdmLibCtx.allocFlags);

                  if ((ret = cmsObj_set(ipv6AddrObj, &ipv6AddriidStack)) != CMSRET_SUCCESS)
                  {
                     cmsLog_error("failed to set IPv6AddrObject");
                     cmsObj_free((void **) &ipv6AddrObj);
                     return ret;
                  }
               }
            }

            cmsObj_free((void **) &ipv6AddrObj);
         }

         /* if this is a new scoped address, add it */
         if ( !found )
         {
            ipv6AddriidStack = *iidStack;
            cmsLog_debug("It's new, add it");

            if ((ret = cmsObj_addInstance(MDMOID_I_PV6_ADDR, &ipv6AddriidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("could not create new ipv6 address entry (L2), ret=%d", ret);
               return ret;
            }

            if ((ret = cmsObj_get(MDMOID_I_PV6_ADDR, &ipv6AddriidStack, OGF_NO_VALUE_UPDATE, (void **) &ipv6AddrObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to get IPv6AddrObject, ret=%d", ret);
               cmsObj_deleteInstance(MDMOID_I_PV6_ADDR, &ipv6AddriidStack);
               return ret;
            }

            CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->IPv6Address, address, mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->scope, scope_text, mdmLibCtx.allocFlags);

            ret = cmsObj_set(ipv6AddrObj, &ipv6AddriidStack);
            cmsObj_free((void **) &ipv6AddrObj);            
         }

         addrIdx++;
      }
   }
#endif  /* DMP_X_BROADCOM_COM_IPV6_1 */

   return ret;
}

CmsRet stl_wifiIPv6AddrObject(_WifiIPv6AddrObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;   
}


CmsRet stl_wanWifiIntfStatsObject(_WanWifiIntfStatsObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif  /* DMP_X_BROADCOM_COM_WIFIWAN_1 */

