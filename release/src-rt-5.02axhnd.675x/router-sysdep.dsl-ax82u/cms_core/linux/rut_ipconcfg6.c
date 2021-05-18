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
#include "rut_iptunnel.h"

CmsRet rutWan_ipv6IpConnProcess(_WanIpConnObject *newObj __attribute__((unused)),
                                const _WanIpConnObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   CmsRet ret = CMSRET_SUCCESS;

   /*
    * Change in X_BROADCOM_COM_IPv6ConnStatus state machine or some other run-time change.
    */
   cmsLog_debug("new->IPv6ConnStatus=%s, new->pdflag=%d, curr->IPv6ConnStatus=%s, curr->pdflag=%d",
               newObj->X_BROADCOM_COM_IPv6ConnStatus, newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled, 
               currObj->X_BROADCOM_COM_IPv6ConnStatus, currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled);

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
         /* from "Unconfigured"/Disconnected" to "Connecting" */
         if ((ret = rutCfg_startWanIpConnection6(iidStack, newObj)) == CMSRET_SUCCESS)
         {
            cmsLog_debug("rutCfg_startWanIpConnection6 ok.");
         }
         else
         {
            cmsLog_error("rutCfg_startWanIpConnection6 failed, error %d", ret);
         }
      }
      /* from connecting to connecting: the only case is RA changes MFlag at bootup time */
      else if ((!cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTING) &&
               (newObj->X_BROADCOM_COM_MFlag != currObj->X_BROADCOM_COM_MFlag)))
      {
         if (newObj->X_BROADCOM_COM_MFlag != newObj->X_BROADCOM_COM_Dhcp6cForAddress )
         {
            if ((ret = rutCfg_startWanIpConnection6(iidStack, newObj)) == CMSRET_SUCCESS)
            {
               cmsLog_debug("rutCfg_startWanIpConnection6 ok.");
            }
            else
            {
               cmsLog_error("rutCfg_startWanIpConnection6 failed, error %d", ret);
            }
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
            /* From "Connected" to "Connected" and Delegation flag is changed: 
            *   Static IPoE successfully launches dhcp6c and get PD information. Do nothing!
            */
            cmsLog_debug("Static IPoE successfully launches dhcp6c and get PD information.");
         }
         else if ( ((newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == 
                     currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled) &&
                    (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6AddressingType, 
                                   MDMVS_STATIC) != 0)) ||
                   ((newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == 0) &&
                    (currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == 1)) 
                 )
         {
            /* From "Connected" to "Connected" and Delegation flag is the same or changed: 
            *   Must be dhcp6c get a new external ip, pd, and dns info or timeout.
            *   Just teardown first and will be setup again below.
            */
            cmsLog_debug("dhcp6c may get a new ip, dns, pd info");

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
               cmsLog_debug("dhcp6c may get a dns, pd, domainName info, teardown");

               if ((ret = rutCfg_tearDownWanIpConnection(iidStack,
                                                      currObj, FALSE)) == CMSRET_SUCCESS)
               {
                  cmsLog_debug("rutCfg_tearDownWanIpConnection ok");
                  
                  if(cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6SitePrefix, currObj->X_BROADCOM_COM_IPv6SitePrefix) != 0) 
	              { 
                     rutWan_deleteDelegatedAddrEntry(currObj->X_BROADCOM_COM_IfName, MDMVS_WANDELEGATED);
	              }
                  
//                  cmsLed_setWanDisconnected();
               }
               else
               {
                  cmsLog_error("rutCfg_tearDownWanIpConnection failed. ret=%d", ret);
               }
            }
            else
            {
               /*dont restrart WAN services if the change is not related to connectivity */
               skipWanIpSetup = TRUE;

               /* 
                * if the default gateway disappers, no need to tear down but 
                * the router lifetime at LAN must be 0
                */
               if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_DefaultIPv6Gateway, 
                               currObj->X_BROADCOM_COM_DefaultIPv6Gateway) != 0)
               {
                  cmsLog_debug("Default gateway disapper only. No need to tear down service");
                  rut_restartRadvdForBridge("br0");  //FIXME: hardcode br0!!

                  if (!newObj->X_BROADCOM_COM_DefaultIPv6Gateway)
                  {
                     rutWan_removeZeroLifeGtwy6(currObj->X_BROADCOM_COM_DefaultIPv6Gateway,
                                                currObj->X_BROADCOM_COM_IfName);
                  }
               }

               /* If Mflag in RA is changed, restart DHCPv6 client accordingly */
               if ((newObj->X_BROADCOM_COM_MFlag != currObj->X_BROADCOM_COM_MFlag) &&
                   (newObj->X_BROADCOM_COM_MFlag != newObj->X_BROADCOM_COM_Dhcp6cForAddress))
               {
                  if ((ret = rutCfg_startWanIpConnection6(iidStack, newObj)) == CMSRET_SUCCESS)
                  {
                     cmsLog_debug("rutCfg_startWanIpConnection6 ok.");
                  }
                  else
                  {
                     cmsLog_error("rutCfg_startWanIpConnection6 failed, error %d", ret);
                  }
               }
            }
         }
         else if ( (newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == FALSE) &&
                      (currObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled == FALSE) )
         {
               cmsLog_debug("IPv6ConnStatus is CONNECTED and PDflag is FALSE => Static IPoE");
         }

      }
      else if (!cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_UNCONFIGURED) ||
               !cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_DISCONNECTED))
      {
         /* 
          * For static IPoE, when layer 2 is up, ssk sets X_BROADCOM_COM_IPv6ConnStatus 
          * to "Connected".  Need to start the interface.
         */

         /* from "Unconfigured"/Disconnected" to "Connecting" */
         if ((ret = rutCfg_startWanIpConnection6(iidStack, newObj)) == CMSRET_SUCCESS)
         {
            cmsLog_debug("rutCfg_startWanIpConnection6 ok.");
         }
         else
         {
            cmsLog_error("rutCfg_startWanIpConnection6 failed, error %d", ret);
         }

         cmsLog_debug("Static IPoE, from UNCONFIG/DISCONNECTED to CONNECTED, ret=%d", ret);
      }

      /* 
       * From "Unconfigured", "Connecting" or "Disconnected" to "Connected", OR
       * from "Connected" to "Connected" where Wan connection is already tearDown above.
       * Just setup the Wan Ip connection.
      */
      if ((skipWanIpSetup == FALSE) && (ret == CMSRET_SUCCESS))
      {
         if ((ret = rutCfg_setupWanConnection6(newObj, TRUE)) == CMSRET_SUCCESS)
         {
            cmsLog_debug("rutCfg_setupWanConnection6 ok");
         }
         else
         {
            cmsLog_error("rutCfg_setupWanConnection6 failed. ret=%d", ret);

            if ((ret = rutCfg_tearDownWanIpConnection(iidStack, currObj, FALSE)) == CMSRET_SUCCESS)
               cmsLog_debug("rutCfg_tearDownWanIpConnection ok.");
            else
            {
               cmsLog_error("rutCfg_tearDownWanIpConnection failed, error %d", ret);
            }
            if ((ret = rutCfg_stopWanIpConnection(iidStack, currObj, FALSE)) == CMSRET_SUCCESS)
            {
               cmsLog_debug("rutCfg_stopWanIpConnection ok.");
            }
            else
            {
               cmsLog_error("rutCfg_stopWanIpConnection failed, error %d", ret);
            }

         }
      }
   }
   else if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_DISCONNECTED))
   {
      if (!cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED) || 
          !cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTING))
      {
         /* 
          * From CONNECTED/CONNECTING -> DISCONNECED 
          * always tear down Wan services associated with this WAN connection
         */

//         cmsLed_setWanDisconnected();

         if ((ret = rutCfg_tearDownWanIpConnection(iidStack, 
                                                   currObj, FALSE)) == CMSRET_SUCCESS)
         {
            cmsLog_debug("rutCfg_tearDownWanIpConnection ok (Connected to Disconnected).");
         }
         else
         {
            cmsLog_error("rutCfg_tearDownWanIpConnection failed, error %d", ret);
         }

         /* Set PrefixDelegationEnabled to FALSE if the link is down */
         newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled = FALSE;

         if ((ret = rutCfg_stopWanIpConnection(iidStack, currObj, FALSE)) == CMSRET_SUCCESS)
         {
            newObj->X_BROADCOM_COM_Dhcp6cPid = CMS_INVALID_PID;
            cmsLog_debug("rutCfg_stopWanIpConnection ok.");
         }
         else
         {
            cmsLog_error("rutCfg_stopWanIpConnection failed, error %d", ret);
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


CmsRet rutWan_ipv6IpConnDisable(_WanIpConnObject *newObj __attribute__((unused)),
                                const _WanIpConnObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("enter");

   /* for delete or disable wan interface */
   if ( currObj->X_BROADCOM_COM_IPv6Enabled )
   {
      if (!cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED) || 
          !cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTING))
      {
         /* Only tearDown when currObj->connectionStatus is CONNECTED/CONNECTING  */

         if ( (ret = rutCfg_tearDownWanIpConnection(iidStack, 
                                                currObj, FALSE) == CMSRET_SUCCESS) )
         {                                                    
            cmsLog_debug("rutCfg_tearDownWanIpConnection ok.");
         }           
         else
         {
            cmsLog_error("rutCfg_tearDownWanIpConnection failed, ret=%d", ret);
         }

         if ( (ret = rutCfg_stopWanIpConnection(iidStack, currObj, FALSE) == CMSRET_SUCCESS) )
         {                                                    
            cmsLog_debug("rutCfg_stopWanIpConnection ok.");
         }           
         else
         {
            cmsLog_error("rutCfg_stopWanIpConnection failed, ret=%d", ret);
         }
      }

      /* release the external ip address, default gateway, etc. if it is not a delete */
      if (newObj)
      {
         if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6AddressingType, MDMVS_DHCP) == 0)
         {
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_ExternalIPv6Address);
         }

         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6DNSServers); /* TODO: should check static DNS or not */
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6DomainName);
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6SitePrefix);
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6SitePrefixOld);
         newObj->X_BROADCOM_COM_IPv6SitePrefixPltime = 0;
         newObj->X_BROADCOM_COM_IPv6SitePrefixVltime = 0;
         newObj->X_BROADCOM_COM_IPv6SitePrefixVltimeOld = 0;

         /* Set PrefixDelegationEnabled to FALSE if we bring down the interface */
         newObj->X_BROADCOM_COM_IPv6PrefixDelegationEnabled = FALSE;

         /* TODO:  should update gateway info in the future. Currently, we force users to statically configure.*/
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


#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
CmsRet rutCfg_startWanIpConnection6(const InstanceIdStack *iidStack,
                                    _WanIpConnObject *newObj)
{
   SINT32 pid;
   CmsRet ret=CMSRET_SUCCESS;
   ConnectionModeType connMode;
   char l2IfName[CMS_IFNAME_LENGTH]={0};
   char baseL3IfName[CMS_IFNAME_LENGTH]={0};
   UBOOL8 isBridge=FALSE;
   
   cmsLog_debug("Enter.");

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
   }

   /* make sure that all dynamically assigned parameters are properly cleared */
   if (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6AddressingType, MDMVS_STATIC)) 
   {
      cmsLog_debug("Config Static IPoE.");
   }
   else 
   {
      UBOOL8 dynamicTunnel;
      UBOOL8 requestAddress = FALSE;

      cmsLog_debug("Config Dynamic IPv6 IPoE.");
      
      CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_ExternalIPv6Address);
      CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6DNSServers); /* TODO: should check static DNS or not */
      CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6SitePrefix);
      CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_IPv6SitePrefixOld);
      newObj->X_BROADCOM_COM_IPv6SitePrefixPltime = 0;
      newObj->X_BROADCOM_COM_IPv6SitePrefixVltime = 0;
      newObj->X_BROADCOM_COM_IPv6SitePrefixVltimeOld = 0;

      dynamicTunnel = rutTunnel_containDynamicTunnel(newObj->X_BROADCOM_COM_IfName, FALSE);

      if (!newObj->X_BROADCOM_COM_UnnumberedModel)
      {
         requestAddress = (newObj->X_BROADCOM_COM_Dhcp6cForAddress || newObj->X_BROADCOM_COM_MFlag);
      }

      if (newObj->X_BROADCOM_COM_Dhcp6cPid != CMS_INVALID_PID)
      {
         rutWan_stopDhcp6c(newObj->X_BROADCOM_COM_IfName, newObj->X_BROADCOM_COM_Dhcp6cPid);
      }

      /* MAP-T (or MAP-E) is only available in pure181 */
      if ((pid = rutWan_restartDhcp6c(newObj->X_BROADCOM_COM_IfName, 
                                      requestAddress, 
                                      newObj->X_BROADCOM_COM_Dhcp6cForPrefixDelegation,
                                      dynamicTunnel, FALSE, FALSE)) == CMS_INVALID_PID)
      {
         cmsLog_error("rutWan_restartDhcp6c returns error.");
         return CMSRET_INTERNAL_ERROR;
      }
      
      newObj->X_BROADCOM_COM_Dhcp6cPid = pid;
   }
   cmsLog_debug("Exit. ret=%d", ret);
   
   return ret;
}
#endif
