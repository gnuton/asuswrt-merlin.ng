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
#include "rut_wan.h"
#include "rut_lan.h"
#include "rut_wanlayer2.h"

CmsRet rutWan_ipv6PppConnDisable(_WanPppConnObject *newObj __attribute__((unused)),
                                 const _WanPppConnObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   CmsRet ret = CMSRET_SUCCESS;
   cmsLog_debug("enter");

   /* for delete or disable IPv6 wan interface */
   if ( currObj->X_BROADCOM_COM_IPv6Enabled )
   {
      /* Only do action when currObj->connectionStatus is CONNECTED/CONNECTING  */
      if (!cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED) || 
          !cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTING))
      {
         /* Stop all service */
         if ( (ret = rutCfg_tearDownWanPppConnection(iidStack, 
                                                     newObj,
                                                     currObj, FALSE) == CMSRET_SUCCESS) )
         {                                                    
            cmsLog_debug("rutCfg_tearDownWanPppConnection ok.");
         }           
         else
         {
            cmsLog_error("rutCfg_tearDownWanPppConnection failed, ret=%d", ret);
         }

         /* Stop the interface */
         if ( (ret = rutCfg_stopWanPppConnection(iidStack, currObj, FALSE) == CMSRET_SUCCESS) )
         {                                                    
            cmsLog_debug("rutCfg_stopWanPppConnection ok.");
         }           
         else
         {
            cmsLog_error("rutCfg_stopWanPppConnection failed, ret=%d", ret);
         }
      }

      /* release the external ip address, default gateway, etc. if it is not a delete */
      if (newObj)
      {
         if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6AddressingType, MDMVS_DHCP) == 0)
         {
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_ExternalIPv6Address);
         }

         /* TODO: should check static DNS or not */
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6DNSServers);
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6DomainName);

         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6SitePrefix);
         newObj->X_BROADCOM_COM_IPv6SitePrefixPltime = 0;
         newObj->X_BROADCOM_COM_IPv6SitePrefixVltime = 0;

         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6SitePrefixOld);
         newObj->X_BROADCOM_COM_IPv6SitePrefixVltimeOld = 0;

         /* Set PrefixDelegationEnabled and PppUp to FALSE if we bring down the interface */
         newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled = FALSE;
         newObj->X_BROADCOM_COM_IPv6PppUp = FALSE;

         /* 
          *
          * TODO: should update gateway info in the future. 
          * Currently, we force users to statically configure.
         */
//      CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_DefaultIPv6Gateway);
      }
   }

   rutMulti_updateIgmpMldProxyIntfList();

   cmsLog_debug("Exit with ret=%d", ret);

   return ret;
#else
   return CMSRET_SUCCESS;
#endif
}


CmsRet rutWan_ipv6PppConnProcess(_WanPppConnObject *newObj __attribute__((unused)),
                                 const _WanPppConnObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   CmsRet ret = CMSRET_SUCCESS;
   
   /*
    * Change in X_BROADCOM_COM_IPv6ConnStatus state machine or some other run-time change.
    */
   cmsLog_debug("new->IPv6ConnStatus=%s, new->pdflag=%d, new->PppUp=%d, curr->IPv6ConnStatus=%s, curr->pdflag=%d, curr->PppUp=%d", newObj->X_BROADCOM_COM_IPv6ConnStatus, 
                   newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled, 
                   newObj->X_BROADCOM_COM_IPv6PppUp, 
                   currObj->X_BROADCOM_COM_IPv6ConnStatus, 
                   currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled, 
                   currObj->X_BROADCOM_COM_IPv6PppUp);

   cmsLog_debug("new->IPv6Addr=%s, new->SitePrefix=%s, curr->IPv6Addr=%s, curr->SitePrefix=%s",
               newObj->X_BROADCOM_COM_ExternalIPv6Address, newObj->X_BROADCOM_COM_IPv6SitePrefix, 
               currObj->X_BROADCOM_COM_ExternalIPv6Address, currObj->X_BROADCOM_COM_IPv6SitePrefix);

   if (newObj->X_BROADCOM_COM_IPv6Enabled == FALSE)
   {
      cmsLog_error("Enter rutWan_ipv6IpConnProcess() but X_BROADCOM_COM_IPv6Enabled is not set");
      return CMSRET_INTERNAL_ERROR;
   }
   
   if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTING))
   {
      if (!cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_UNCONFIGURED) ||
          !cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_DISCONNECTED))
      {
         if (newObj->X_BROADCOM_COM_IPv6PppUp == 0)
         {
            /* Layer 2 interface just comes up. If pppd is not launched in IPv4, we should launch pppd. */

            if ((newObj->X_BROADCOM_COM_PppdPid == CMS_INVALID_PID) && 
                ((ret = rutCfg_startWanPppConnection(iidStack, newObj)) == CMSRET_SUCCESS))
            {
               cmsLog_debug("rutCfg_startWanPppConnection ok.");
            }
            else if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("rutCfg_startWanPppConnection failed, error %d", ret);
            }
         }
      }
      if ((!cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTING) && 
          (newObj->X_BROADCOM_COM_IPv6PppUp == TRUE && currObj->X_BROADCOM_COM_IPv6PppUp == FALSE)) || 
          (!cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_DISCONNECTED) &&
          (newObj->X_BROADCOM_COM_IPv6PppUp == 1 && currObj->X_BROADCOM_COM_IPv6PppUp == FALSE))
         )
      {
         /*
          * Two cases in this statement:
          * 1. L2 intf down to up, PPP is successfully launched.
          * 2. L2 intf is always up but PPP session is down to up
          */
         /* pppd has successfully created an interface. Now launch dhcp6c on this interface */
         if ((ret = rutCfg_startWanPppConnection6(iidStack, newObj)) == CMSRET_SUCCESS)
         {
            cmsLog_debug("rutCfg_startWanPppConnection6 ok.");
         }
         else
         {
            cmsLog_error("rutCfg_startWanPppConnection6 failed, error %d", ret);
         }
      }
      /* If Mflag in RA is changed, restart DHCPv6 client accordingly */
      if (!cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTING) && 
          (newObj->X_BROADCOM_COM_IPv6PppUp == TRUE && currObj->X_BROADCOM_COM_IPv6PppUp == TRUE) &&
          ((newObj->X_BROADCOM_COM_MFlag != currObj->X_BROADCOM_COM_MFlag) &&
           (newObj->X_BROADCOM_COM_MFlag != newObj->X_BROADCOM_COM_Dhcp6cForAddress)))
      {
         if ((ret = rutCfg_startWanPppConnection6(iidStack, newObj)) == CMSRET_SUCCESS)
         {
            cmsLog_debug("rutCfg_startWanPppConnection6 ok.");
         }
         else
         {
            cmsLog_error("rutCfg_startWanPppConnection6 failed, error %d", ret);
         }
      }
   }
   else if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED))
   {
      UBOOL8 skipWanIpSetup = FALSE;

      if (!cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED))
      {
         
         if ( (newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == TRUE) &&
               (currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == FALSE) )
         {
            /* 
             * From "Connected" to "Connected" and Delegation flag is changed: 
            */
            cmsLog_debug("dhcp6c gets PD information.");
         }


         else if ( ((newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == 
                     currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled) &&
                    (newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == TRUE)) ||
                   ((newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == 0) &&
                    (currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == 1)) 
                 )
         {
            /* 
             * From "Connected" to "Connected" and Delegation flag is the same: 
             * May be dhcp6c get a new external ip/default gateway, dns info.
             */
            /* remove address if necessary. FIXME: should we teardown here? */
            if ( (newObj->X_BROADCOM_COM_ExternalIPv6Address == NULL) &&
                 (currObj->X_BROADCOM_COM_ExternalIPv6Address != NULL) )
            {
               rutWan_setIPv6Address(NULL, NULL, 
                                     currObj->X_BROADCOM_COM_ExternalIPv6Address, 
                                     currObj->X_BROADCOM_COM_IfName);
            }
            /* TODO: Our system currently can only take one address at WAN interface */
            else if (!cmsNet_areIp6AddrEqual(newObj->X_BROADCOM_COM_ExternalIPv6Address, 
                                             currObj->X_BROADCOM_COM_ExternalIPv6Address))
            {
               rutWan_setIPv6Address(newObj->X_BROADCOM_COM_ExternalIPv6Address, 
                                     newObj->X_BROADCOM_COM_IfName, 
                                     currObj->X_BROADCOM_COM_ExternalIPv6Address, 
                                     currObj->X_BROADCOM_COM_IfName);
            }

            if((cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6SitePrefix, currObj->X_BROADCOM_COM_IPv6SitePrefix) != 0) ||
               (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6DomainName, currObj->X_BROADCOM_COM_IPv6DomainName) != 0) ||
                (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6DNSServers, currObj->X_BROADCOM_COM_IPv6DNSServers) != 0)) 
            {
               cmsLog_debug("dhcp6c get a new ip, dns, pd info, tear down service");
               if ((ret = rutCfg_tearDownWanPppConnection(iidStack, 
                                                         newObj,
                                                         currObj, FALSE)) == CMSRET_SUCCESS)
               {
                  cmsLog_debug("rutCfg_tearDownWanPppConnection ok");
                  
                  if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6SitePrefix, currObj->X_BROADCOM_COM_IPv6SitePrefix) != 0) 
                  { 
                     rutWan_deleteDelegatedAddrEntry(currObj->X_BROADCOM_COM_IfName, MDMVS_WANDELEGATED);
                  } 
			   
//                  cmsLed_setWanDisconnected();
               }
               else
               {
                  cmsLog_error("rutCfg_tearDownWanPppConnection failed. ret=%d", ret);
               }
            }
            else
            {
               skipWanIpSetup = TRUE;

               /* 
                * if the default gateway disappers, no need to tear down but 
                * the router lifetime at LAN must be 0
                */
               if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_DefaultIPv6Gateway, 
                               currObj->X_BROADCOM_COM_DefaultIPv6Gateway) != 0)
               {
                  if (!newObj->X_BROADCOM_COM_DefaultIPv6Gateway)
                  {
                     cmsLog_debug("Default gateway disapper only. No need to tear down service");
                     rut_restartRadvdForBridge("br0");  //FIXME: hardcode br0!!
                     rutWan_removeZeroLifeGtwy6(currObj->X_BROADCOM_COM_DefaultIPv6Gateway,
                                                currObj->X_BROADCOM_COM_IfName);
                  }
               }
            }
         }
         else if ( (newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == FALSE) &&
                   (currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == FALSE) )
         {
               cmsLog_error("IPv6ConnStatus is CONNECTED and PDflag is FALSE. Static PPPoE??");
         }

      }

      /* 
       * From "Unconfigured", "Connecting" or "Disconnected" to "Connected", OR
       * from "Connected" to "Connected" where Wan connection is already tearDown above.
       * Just setup the Wan Ip connection.
      */
      if ( (skipWanIpSetup == FALSE) && (ret == CMSRET_SUCCESS) )
      {
		 if ((ret = rutCfg_setupWanConnection6(newObj, FALSE)) == CMSRET_SUCCESS)
         {
            cmsLog_debug("rutCfg_setupWanConnection6 ok");
         }
         else
         {
            cmsLog_error("rutCfg_setupWanConnection6 failed. ret=%d", ret);

            if ((ret = rutCfg_tearDownWanPppConnection(iidStack, newObj, 
                                                       currObj, FALSE)) == CMSRET_SUCCESS)
               cmsLog_debug("rutCfg_tearDownWanPppConnection ok.");
            else
            {
               cmsLog_error("rutCfg_tearDownWanPppConnection failed, error %d", ret);
            }
            if ((ret = rutCfg_stopWanPppConnection(iidStack, currObj, FALSE)) == CMSRET_SUCCESS)
            {
               cmsLog_debug("rutCfg_stopWanPppConnection ok.");
            }
            else
            {
               cmsLog_error("rutCfg_stopWanPppConnection failed, error %d", ret);
            }
         }
      }
   }
   else if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_DISCONNECTED))
   {
      if (!cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED) || 
          !cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTING))
      {
         UINT32 dhcp6cEid = MAKE_SPECIFIC_EID(currObj->X_BROADCOM_COM_Dhcp6cPid, EID_DHCP6C);
         /* 
          * From CONNECTED/CONNECTING -> DISCONNECED 
          * Tear down Wan services associated with this WAN connection and 
          * bring the interface down
         */

//         cmsLed_setWanDisconnected();

         if ((ret = rutCfg_tearDownWanPppConnection(iidStack, 
                                                   newObj,
                                                   currObj, FALSE)) == CMSRET_SUCCESS)
         {
            rutWan_deleteDelegatedAddrEntry(currObj->X_BROADCOM_COM_IfName, MDMVS_WANDELEGATED);
            cmsLog_debug("rutCfg_tearDownWanIpConnection ok (Connected to Disconnected).");
         }
         else
         {
            cmsLog_error("rutCfg_tearDownWanIpConnection failed, error %d", ret);
         }

         /* Set PrefixDelegationEnabled to FALSE if the link is down */
         newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled = FALSE;
         newObj->X_BROADCOM_COM_IPv6PppUp = FALSE;

         /* Stop dhcp6c */
         if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, dhcp6cEid, NULL, 0) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to send msg to stop dhcp6c");
         }
         else
         {
            newObj->X_BROADCOM_COM_Dhcp6cPid = CMS_INVALID_PID;
            cmsLog_debug("dhcp6c stopped");
         }

         cmsLog_debug("link is %s", newObj->X_BROADCOM_COM_TransientLayer2LinkStatus);
               
        /* Only stop pppd when layer 2 link is down (X_BROADCOM_COM_TransientLayer2LinkStatus from ssk)
         * Otherwise, keep pppd running when ConnectionStatus = "Disconnected" for following case:
         * 1). ppp connection is down - lastConnectionError = "ERROR_UNKNOWN",
         *  
         *  OR in auto detect mode and ppp cannot be up in the specified time frame, stop pppd as well
         *  X_BROADCOM_COM_StopPppD is set to TRUE during in ssk when setting connectionStatus
         * to "Connecting" OR disabled by manual selection.
         */
         cmsLog_debug("STOP_PPPD(newObj)=%d", STOP_PPPD(newObj));
         if (!rutWan_isTransientLayer2LinkUp(newObj->X_BROADCOM_COM_TransientLayer2LinkStatus) || STOP_PPPD(newObj))
         {
            if ((ret = rutCfg_stopWanPppConnection(iidStack, currObj, TRUE)) == CMSRET_SUCCESS)
            {
               cmsLog_debug("rutCfg_stopWanPppConnection ok.");
            }
            else
            {
               cmsLog_error("rutCfg_stopWanPppConnection failed, error %d", ret);
            }
            
#ifdef DMP_X_BROADCOM_COM_AUTODETECTION_1          
            /* reset it to FALSE */
            newObj->X_BROADCOM_COM_StopPppD = FALSE;
#endif /* DMP_X_BROADCOM_COM_AUTODETECTION_1 */   
         }
      }
   }

   rutMulti_updateIgmpMldProxyIntfList();

   cmsLog_debug("ret=%d", ret);
   return ret;
#else
   return CMSRET_SUCCESS;
#endif
}


#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
CmsRet rutCfg_startWanPppConnection6(const InstanceIdStack *iidStack  __attribute__((unused)),
                                     _WanPppConnObject *newObj)
{
   SINT32 pid;
   char l2IfName[BUFLEN_32];
   char intf_info[BUFLEN_32];
   UBOOL8 dynamicIp = FALSE;
   UBOOL8 dynamicTunnel;
   CmsRet ret=CMSRET_SUCCESS;
   
   cmsLog_debug("Enter.");

   /* Reset the dynamic parameters and launch dhcp6c */
   CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_ExternalIPv6Address);
   /* TODO: should check static DNS or not */
   CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6DNSServers);
   CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6DomainName);

   /* 
    * TODO: Should update gateway info in the future. 
    * Currently, we force users to statically configure.
    */
//   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_DefaultIPv6Gateway, "", mdmLibCtx.allocFlags);

   CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6SitePrefix);
   newObj->X_BROADCOM_COM_IPv6SitePrefixPltime = 0;
   newObj->X_BROADCOM_COM_IPv6SitePrefixVltime = 0;

   CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6SitePrefixOld);
   newObj->X_BROADCOM_COM_IPv6SitePrefixVltimeOld = 0;

   if ((ret = rutWl2_getL2IfnameFromL3Ifname(newObj->X_BROADCOM_COM_IfName, l2IfName)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutWl2_getL2IfnameFromL3Ifname() returns error. ret=%d", ret);
      return ret;
   }

   /* 
    * In order to launch dhcp6c on a PPPoE interface, we need to know the MAC address of 
    * the ethernet at which PPPoE running. Therefore, layer 2 interface name is needed.
    * Before "__" is layer 3 interface name (ex: ppp0). After "__" is layer 2 interface name (ex: eth0).
    */
   snprintf(intf_info, sizeof(intf_info), "%s__%s", newObj->X_BROADCOM_COM_IfName, l2IfName);

   if (!newObj->X_BROADCOM_COM_UnnumberedModel &&
       cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6AddressingType, MDMVS_STATIC))
   {
      dynamicIp = (newObj->X_BROADCOM_COM_Dhcp6cForAddress || newObj->X_BROADCOM_COM_MFlag);
   }
           
   dynamicTunnel = rutTunnel_containDynamicTunnel(newObj->X_BROADCOM_COM_IfName, FALSE);

   if (newObj->X_BROADCOM_COM_Dhcp6cPid != CMS_INVALID_PID)
   {
      rutWan_stopDhcp6c(newObj->X_BROADCOM_COM_IfName, newObj->X_BROADCOM_COM_Dhcp6cPid);
   }

   /* MAP-T (or MAP-E) is only available in pure181 */
   if ((pid = rutWan_restartDhcp6c(intf_info, dynamicIp, 
                                   newObj->X_BROADCOM_COM_Dhcp6cForPrefixDelegation,
                                   dynamicTunnel, FALSE, FALSE)) == CMS_INVALID_PID)
   {
      cmsLog_error("rutWan_restartDhcp6c returns error.");
      return CMSRET_INTERNAL_ERROR;
   }
   
   newObj->X_BROADCOM_COM_Dhcp6cPid = pid;

   cmsLog_debug("Exit. ret=%d", ret);

   return ret;
}
#endif

