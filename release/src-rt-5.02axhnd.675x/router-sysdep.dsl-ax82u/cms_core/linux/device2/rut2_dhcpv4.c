/***********************************************************************
 *
 *  Copyright (c) 2009-2013  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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


#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_lan.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "rut2_dhcpv4.h"


#ifdef DMP_DEVICE2_DHCPV4_1


UBOOL8 rutLan_isDhcpdEnabled_dev2(void)
{
   UBOOL8 dhcpdEnabled = FALSE;
   Dev2Dhcpv4ServerPoolObject *dhcpv4ServerPoolObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   while (dhcpdEnabled == FALSE &&
          cmsObj_getNext(MDMOID_DEV2_DHCPV4_SERVER_POOL, &iidStack, (void **)&dhcpv4ServerPoolObj) == CMSRET_SUCCESS)
   {
      dhcpdEnabled = dhcpv4ServerPoolObj->enable;

      cmsObj_free((void **)&dhcpv4ServerPoolObj);
   }

#ifdef DMP_DEVICE2_DHCPV4RELAY_1
   {
      Dev2Dhcpv4RelayForwardingObject *forwardingObj=NULL;

      INIT_INSTANCE_ID_STACK(&iidStack);
      while (dhcpdEnabled == FALSE &&
             cmsObj_getNext(MDMOID_DEV2_DHCPV4_RELAY_FORWARDING, &iidStack, (void **) &forwardingObj) == CMSRET_SUCCESS)
      {
         dhcpdEnabled = forwardingObj->enable;
         cmsObj_free((void **) &forwardingObj);
      }
   }
#endif  /* DMP_DEVICE2_DHCPV4RELAY_1 */

   cmsLog_debug("dhcpd eanbled: %s", dhcpdEnabled ? "yes": "no");

   return dhcpdEnabled;
}

void rutLan_createDhcpdCfg_dev2(void)
{
   char dns1[CMS_IPADDR_LENGTH]={0};
   char dns2[CMS_IPADDR_LENGTH]={0};
   UINT32 leasetime=10;  // is this too short?  why is min_lease 30?
   char cmdStr[BUFLEN_128]={0};
   char ifName[CMS_IFNAME_LENGTH]={0};
   FILE* fs = NULL;
   Dev2Dhcpv4ServerPoolObject *dhcpv4ServerPoolObj = NULL;
   Dev2Dhcpv4ServerPoolStaticAddressObject *dhcpv4StaticAddressObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;

   cmsLog_debug("Enter.");

   if ((fs = fopen(UDHCPD_CONFIG_FILENAME, "w")) == NULL)
   {  
      cmsLog_error("Failed to open %s", UDHCPD_CONFIG_FILENAME);
      return;
   }

   /* Global information */

   /* If you change the name of this file, make sure you change it
    * everywhere by searching for UDHCPD_DECLINE macro
    */
   snprintf(cmdStr, sizeof(cmdStr), "decline_file %s\n", UDHCPD_DECLINE);
   fputs(cmdStr, fs);

#ifdef DMP_DEVICE2_DHCPV4RELAY_1
   /*
    * First write out all interfaces which has DHCP relay enabled.  Then write
    * out the interfaces which has normal DHCP server enabled.  This is
    * slightly different than the way TR98 does it (due to data model
    * structure differences).
    */
   {
      Dev2Dhcpv4RelayForwardingObject *forwardingObj=NULL;

      while (cmsObj_getNext(MDMOID_DEV2_DHCPV4_RELAY_FORWARDING, &iidStack, (void **) &forwardingObj) == CMSRET_SUCCESS)
      {
         /* XXX TODO: should also verify WAN interface does not have NAT.
          * For now, assume WebUI and ACS will configure correctly.
          */
         if (forwardingObj->enable)
         {
            qdmIntf_fullPathToIntfnameLocked(forwardingObj->interface, ifName);
            snprintf(cmdStr, sizeof(cmdStr), "interface %s\n", ifName);
            fputs(cmdStr, fs);

            snprintf(cmdStr, sizeof(cmdStr), "relay %s\n", forwardingObj->DHCPServerIPAddress);
            fputs(cmdStr, fs);
         }
         cmsObj_free((void **) &forwardingObj);
      }
   }
#endif  /* DMP_DEVICE2_DHCPV4RELAY_1 */

   /* Per-subnet information */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNext(MDMOID_DEV2_DHCPV4_SERVER_POOL, &iidStack, (void **)&dhcpv4ServerPoolObj) == CMSRET_SUCCESS)
   {
      if (dhcpv4ServerPoolObj->enable == TRUE &&
          qdmIntf_fullPathToIntfnameLocked_dev2(dhcpv4ServerPoolObj->interface, ifName) == CMSRET_SUCCESS)
      {
         /* interface name */
         snprintf(cmdStr, sizeof(cmdStr), "interface %s\n", ifName);
         fputs(cmdStr, fs);

         /* min address */
         if ( !IS_EMPTY_STRING(dhcpv4ServerPoolObj->minAddress) )
         {
            snprintf(cmdStr, sizeof(cmdStr), "start %s\n", dhcpv4ServerPoolObj->minAddress);
            fputs(cmdStr, fs);
         }
         /* max address */
         if ( !IS_EMPTY_STRING(dhcpv4ServerPoolObj->maxAddress) )
         {
            snprintf(cmdStr, sizeof(cmdStr), "end %s\n", dhcpv4ServerPoolObj->maxAddress);
            fputs(cmdStr, fs);
         }

         /* subnet mask */
         if ( !IS_EMPTY_STRING(dhcpv4ServerPoolObj->subnetMask) )
         {
            snprintf(cmdStr, sizeof(cmdStr), "option subnet %s\n", dhcpv4ServerPoolObj->subnetMask);
            fputs(cmdStr, fs);
         }
         /* router */
         if ( !IS_EMPTY_STRING(dhcpv4ServerPoolObj->IPRouters) )
         {
            char *pToken = NULL;
            char buff[BUFLEN_64]={0};
            strncpy(buff, dhcpv4ServerPoolObj->IPRouters, sizeof(buff));
            for (pToken = strtok(buff, "," ); pToken != NULL; pToken = strtok(NULL, "," )) 
            {
               snprintf(cmdStr, sizeof(cmdStr), "option router %s\n", pToken);
               fputs(cmdStr, fs);
            }
         }

         /*
          * DNS section.
          * If DNS server is set in the DHCPv4ServerPool object, use that.
          * If DNS server in the DHCPv4ServerPool object is not set, then
          * (a) if DNSProxy, use LAN bridge IP Addr
          * (b) if no DNSProxy, use the DNS info from the WAN connection
          * use heuristics to determine which DNS server to use.
          */
          
         leasetime = dhcpv4ServerPoolObj->leaseTime;
         
         /* do a parse on the DNSServers string to see whether there is any no zero dns */
         if ((cmsUtl_parseDNS(dhcpv4ServerPoolObj->DNSServers, dns1, dns2, TRUE) == CMSRET_SUCCESS) &&
             (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPV4, dns1) ||!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPV4, dns2)))
         {
            cmsLog_debug("Use DNSServers: dns1 %s, dns2 %s", dns1, dns2);
         }
         else
         {
#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1 /* aka SUPPORT_DNSPROXY */

            /* If DNS Proxy, DNS server is just the IP address of this
             * particular bridge, which is ifName.
             * XXX TODO some weird code about isAnyNatEnable here for
             * DHCP Relay (relay and NAT are incompatible)
             * See rutLan_createDhcpdCfg_igd
             */
            if (qdmIpIntf_getIpv4AddressByNameLocked_dev2(ifName, dns1) == CMSRET_SUCCESS)
            {
               cmsLog_debug("Use interface %s IPv4 address %s as dns", ifName, dns1);
            }               
            else
            {
               cmsLog_error("Cannot get IPv4 address from interface %s", ifName);
            }
#else
            /* XXX should first try to get the WAN connection associated
             * with this bridge group, if there is one.
             * Otherwise, use the default system DNS servers.
             */
            qdmDns_getActiveIpvxDnsIpLocked_dev2(CMS_AF_SELECT_IPV4, dns1, dns2);
            if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPV4, dns1))
            {
               /* there is a DNS server from UP wan connection */
               cmsLog_debug("Use system default dns %s", dns1);
            }
#endif
         }

         if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPV4, dns1))
         {
            snprintf(cmdStr, sizeof(cmdStr), "option dns %s\n", dns1);
            fputs(cmdStr, fs);
         }

         if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPV4, dns2))
         {
            snprintf(cmdStr, sizeof(cmdStr), "option dns %s\n", dns2);
            fputs(cmdStr, fs);
         }


         /*
          * Domain name section.
          * Use same idea as DNS server?
          */
         if ( !IS_EMPTY_STRING(dhcpv4ServerPoolObj->domainName) )
         {
            snprintf(cmdStr, sizeof(cmdStr), "option domain %s\n", dhcpv4ServerPoolObj->domainName);
            fputs(cmdStr, fs);
         }
         else
         {
            /* Get domain name from DNS Proxy object. */
         }

         /*
          * Lease time section.
          * If using DNSProxy, lease time can be long: use configured value.
          * If not DNSProxy, we will be using info from WAN side directly,
          * but WAN side might not be up yet, so keep lease time short
          * (10 seconds) so that LAN side clients are forced to refresh their
          * lease often.   Once CPE has received the info from the WAN side,
          * the lease time can be long again.
          */
         snprintf(cmdStr, sizeof(cmdStr), "option lease %d\n", leasetime);
         fputs(cmdStr, fs);

         /* min lease time */
         snprintf(cmdStr, sizeof(cmdStr), "min_lease 30\n");
         fputs(cmdStr, fs);

      }  // end of if (dhcpv4ServerPoolObj->enable == TRUE ...

      /* loop through all static IP entries to */
      /* write out static IP lease info */
      while (cmsObj_getNextInSubTree(MDMOID_DEV2_DHCPV4_SERVER_POOL_STATIC_ADDRESS,
                                     &iidStack, &iidStackChild,
                                     (void **) &dhcpv4StaticAddressObj) == CMSRET_SUCCESS)
      {
         if (dhcpv4StaticAddressObj->enable == TRUE &&
             !IS_EMPTY_STRING(dhcpv4StaticAddressObj->chaddr) &&
             !IS_EMPTY_STRING(dhcpv4StaticAddressObj->yiaddr)) 
         {
            snprintf(cmdStr, sizeof(cmdStr), "static_lease %s %s\n",
                     dhcpv4StaticAddressObj->chaddr, dhcpv4StaticAddressObj->yiaddr);
            fputs(cmdStr, fs);
         }

         cmsObj_free((void **) &dhcpv4StaticAddressObj);
      }

      cmsObj_free((void **)&dhcpv4ServerPoolObj);

   } // end of while cmsObj_getNext(MDMOID_DEV2_DHCPV4_SERVER_POOL, ...

   fclose(fs);

   cmsLog_debug("Exit.");
   
}


#ifdef DMP_DEVICE2_DHCPV4RELAY_1

void rutDhcpv4Relay_deleteForwardingEntry_dev2(const char *ipIntfFullPath)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Dhcpv4RelayForwardingObject *forwardingObj=NULL;
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 found=FALSE;

   cmsLog_debug("Entered: ipIntfFullPath=%s", ipIntfFullPath);

   /*
    * Go through existing forwarding entries and delete the
    * entry with matching IpIntfFullPath (if it exists).
    */
   while (!found &&
          cmsObj_getNext(MDMOID_DEV2_DHCPV4_RELAY_FORWARDING, &iidStack, (void **) &forwardingObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(forwardingObj->interface, ipIntfFullPath) == 0 )
      {
         found=TRUE;
         if ((ret = cmsObj_deleteInstance(MDMOID_DEV2_DHCPV4_RELAY_FORWARDING, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to delete DHCPV4_RELAY_FORWARDING Object, ret = %d", ret);
         }
      }

      cmsObj_free((void **)&forwardingObj);
   }

    return;
}

#endif  /* DMP_DEVICE2_DHCPV4RELAY_1 */

#endif  /* DMP_DEVICE2_DHCPV4_1 */

/* will be included in DMP_DEVICE2_DHCPV4_1 when hybrid IPv6 is working since 
* 38GWO and 500GWO have IPV6 enabled by default
*/


static CmsRet getIpv4DhcpcObjectByFullPath(const char *ipIntfFullPath,
                                           InstanceIdStack *dhcpcIidStack,
                                           Dev2Dhcpv4ClientObject **dhcpcObject)
{
   UBOOL8 found=FALSE;
   Dev2Dhcpv4ClientObject *dhcp4ClientObj=NULL;
   CmsRet ret=CMSRET_INVALID_ARGUMENTS;
   
   if (ipIntfFullPath == NULL)
   {
      cmsLog_error("NULL string.");
      return ret;
   }
   cmsLog_debug("Enter ipIntfFullPath %s.", ipIntfFullPath);
   
   while(!found && 
         (ret = cmsObj_getNextFlags(MDMOID_DEV2_DHCPV4_CLIENT,
                             dhcpcIidStack, 
                             OGF_NO_VALUE_UPDATE,
                             (void **)&dhcp4ClientObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(dhcp4ClientObj->interface, ipIntfFullPath))
      {
         found = TRUE;
         *dhcpcObject = dhcp4ClientObj;
      }
      else
      {
         cmsObj_free((void **)&dhcp4ClientObj);
      }            
   }     

   cmsLog_debug("found %d", found);

   return ret;
   
}


UBOOL8 rutDhcpv4_isClientEnabled_dev2(const char *ipIntfFullPath)
{
   
   UBOOL8 isEnabled=FALSE;
   Dev2Dhcpv4ClientObject *dhcpcObject=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   
   if (getIpv4DhcpcObjectByFullPath(ipIntfFullPath,
                                    &iidStack, 
                                    &dhcpcObject) != CMSRET_SUCCESS)
   {
      cmsLog_debug("no Dhcpv4ClientObj for %s", ipIntfFullPath);
      return FALSE;
   }
   
   isEnabled = dhcpcObject->enable;
   cmsObj_free((void **) &dhcpcObject);

   cmsLog_debug("enabled=%d", isEnabled);

   return isEnabled;

}


void rutDhcpv4_setClientPidAndStatusByIpIntfFullPath_dev2(const char *ipIntfFullPath,
                                                    const SINT32 pid,
                                                    const char *status)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2Dhcpv4ClientObject *dhcp4ClientObj=NULL;
   CmsRet ret;

   cmsLog_debug("fullpath=%s pid=%d status=%s", ipIntfFullPath, pid, status);

   if (getIpv4DhcpcObjectByFullPath(ipIntfFullPath,
                                    &iidStack, 
                                    &dhcp4ClientObj) != CMSRET_SUCCESS)   {
      cmsLog_error("Fail to find the dhcp client info for %s", ipIntfFullPath);
      return;
   }

   dhcp4ClientObj->X_BROADCOM_COM_Pid = pid;
   CMSMEM_REPLACE_STRING_FLAGS(dhcp4ClientObj->status, status, mdmLibCtx.allocFlags);

   /* this means we are just starting the DHCP client */
   if (!cmsUtl_strcmp(status, MDMVS_ENABLED))
   {
      CMSMEM_REPLACE_STRING_FLAGS(dhcp4ClientObj->DHCPStatus, MDMVS_SELECTING,
                                  mdmLibCtx.allocFlags);
   }

   /* this means we are stopped or have an error */
   if (!cmsUtl_strcmp(status, MDMVS_DISABLED) ||
       !cmsUtl_strcmp(status, MDMVS_ERROR))
   {
      CMSMEM_REPLACE_STRING_FLAGS(dhcp4ClientObj->DHCPStatus, MDMVS_INIT,
                                  mdmLibCtx.allocFlags);
      CMSMEM_FREE_BUF_AND_NULL_PTR(dhcp4ClientObj->IPAddress);
      CMSMEM_FREE_BUF_AND_NULL_PTR(dhcp4ClientObj->subnetMask);
      CMSMEM_FREE_BUF_AND_NULL_PTR(dhcp4ClientObj->DNSServers);
      CMSMEM_FREE_BUF_AND_NULL_PTR(dhcp4ClientObj->IPRouters);
   }

   if ((ret = cmsObj_set(dhcp4ClientObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_set dhcp4ClientObj returns error. ret=%d", ret);
   }     
   
   cmsObj_free((void **) &dhcp4ClientObj);

   return;
}


CmsRet rutDhcpv4_getClientPidByIpIntfFullPath_dev2(const char *ipIntfFullPath,  SINT32 *pid)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2Dhcpv4ClientObject *dhcp4ClientObj=NULL;
   CmsRet ret = CMSRET_SUCCESS;

 
   if (getIpv4DhcpcObjectByFullPath(ipIntfFullPath,
                                    &iidStack, 
                                    &dhcp4ClientObj) != CMSRET_SUCCESS)   
   {
      cmsLog_error("Fail to find the dhcp client info for %s", ipIntfFullPath);
      return CMSRET_INTERNAL_ERROR;
   }
   
   *pid = dhcp4ClientObj->X_BROADCOM_COM_Pid;

   cmsObj_free((void **) &dhcp4ClientObj);
   
   cmsLog_debug("Exit. ret=%d pid %d", ret, pid);
   
   return ret;
   
}


void rutDhcpv4_stopClientByIpIntfFullPath_dev2(const char *ipIntfFullPath)
{
   SINT32 pid;
   CmsRet ret;

   if ((ret = rutDhcpv4_getClientPidByIpIntfFullPath_dev2(ipIntfFullPath, &pid)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutDhcp4_getClientPidByIpIntfFullPath_dev2 failed, ret=%d", ret);
      return;
   }

   if (pid != CMS_INVALID_PID)
   {
      rutDhcpv4_stopClientByPid_dev2(pid);
   }
}


void rutDhcpv4_stopClientByPid_dev2(SINT32 pid)
{
   UINT32 specificEid = MAKE_SPECIFIC_EID(pid, EID_DHCPC);

   cmsLog_debug("stopping dhcpc pid=%d", pid);

   if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to send msg to stop dhcpc");
   }
   else
   {
      cmsLog_debug("dhcpc stopped");
   }
}



CmsRet rutDhcpv4_configClientByIfName_dev2(const char *ifName, UBOOL8 enableFlag)
{
   UBOOL8 found=FALSE;
   Dev2Dhcpv4ClientObject *dhcp4ClientObj=NULL;
   CmsRet ret=CMSRET_INVALID_ARGUMENTS;
   char *brIntfFullPath=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   if ((ret = qdmIntf_intfnameToFullPathLocked(ifName, FALSE, &brIntfFullPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed qdmIntf_intfnameToFullPathLocked on %s", ifName);
      return ret;
   }
   while(!found && 
         (ret = cmsObj_getNextFlags(MDMOID_DEV2_DHCPV4_CLIENT,
                             &iidStack, 
                             OGF_NO_VALUE_UPDATE,
                             (void **)&dhcp4ClientObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(dhcp4ClientObj->interface, brIntfFullPath))
      {
         dhcp4ClientObj->enable = enableFlag;
         if ((ret = cmsObj_set(dhcp4ClientObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set dhcp4ClientObj returns error. ret=%d", ret);
         }     
         found = TRUE;
      }
      cmsObj_free((void **)&dhcp4ClientObj);
   }     

   CMSMEM_FREE_BUF_AND_NULL_PTR(brIntfFullPath);
   
   cmsLog_debug("found %d, ret %d", found, ret);

   return ret;
}



CmsRet rutDhcpv4_restartDhcpv4Client_dev2(const char *bridgeIfName)
{
   UBOOL8 enableDhcpc = FALSE;
   CmsRet ret = CMSRET_SUCCESS;
   
   ret = rutDhcpv4_configClientByIfName_dev2(bridgeIfName, enableDhcpc);
   if (ret == CMSRET_SUCCESS)
   {
      enableDhcpc = TRUE;
      ret = rutDhcpv4_configClientByIfName_dev2(bridgeIfName, enableDhcpc);
   }

   return ret;
}




#endif /* DMP_DEVICE2_BASELINE_1 */

