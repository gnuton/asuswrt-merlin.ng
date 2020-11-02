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

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_lan.h"
#include "rut_wan.h"
#include "rut_wan6.h"
#include "rut_util.h"
#include "rut_iptunnel.h"
#include "rut_upnp.h"
#include "rut_ebtables.h"

#ifdef DMP_X_BROADCOM_COM_IPV6_1

/* Legacy TR98 IPv6 functions */

CmsRet rcl_ipv6LanIntfAddrObject( _IPv6LanIntfAddrObject *newObj,
                const _IPv6LanIntfAddrObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode  __attribute__((unused)))
{
   _LanIpIntfObject *ipIntfObj = NULL;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   char ifName[BUFLEN_32];
   CmsRet ret = CMSRET_SUCCESS;

   iidStack1 = *iidStack;
   if (cmsObj_getAncestorFlags(MDMOID_LAN_IP_INTF, MDMOID_I_PV6_LAN_INTF_ADDR,
                             &iidStack1, OGF_NO_VALUE_UPDATE, 
                             (void **) &ipIntfObj) != CMSRET_SUCCESS)
   {
      cmsLog_error("Current ULA object have no ancestor.");
      return CMSRET_MDM_TREE_ERROR;
   }

   cmsUtl_strncpy(ifName, ipIntfObj->X_BROADCOM_COM_IfName, BUFLEN_32);
   cmsObj_free((void **) &ipIntfObj);

   if (currObj == NULL)
   {
      /* boot up time */
      if ( !IS_EMPTY_STRING(newObj->uniqueLocalAddress) )
      {
         if ( cmsUtl_isValidIpAddress(AF_INET6, newObj->uniqueLocalAddress) )
         {
            rutLan_setIPv6Address( newObj->uniqueLocalAddress, ifName, NULL, NULL);
         }
         else
         {
            cmsLog_error("invalid ULA address: %s", newObj->uniqueLocalAddress);
            return CMSRET_INVALID_PARAM_VALUE;
         }
      }

      /* TODO: delete delegated address object?? */
   }

   /* assign new address */
   if ( (currObj != NULL) && (newObj != NULL) )
   {
      if (IS_EMPTY_STRING(newObj->uniqueLocalAddress) && 
          !IS_EMPTY_STRING(currObj->uniqueLocalAddress) )
      {
         char cmdLine[BUFLEN_128];

         snprintf(cmdLine, sizeof(cmdLine), "ip -6 addr del %s dev %s 2>/dev/null", 
                  currObj->uniqueLocalAddress, ifName);
         rut_doSystemAction("rut", cmdLine);
      }
      else if ( cmsUtl_isValidIpAddress(AF_INET6, newObj->uniqueLocalAddress) )
      {
         rutLan_setIPv6Address( newObj->uniqueLocalAddress, ifName,
                                           currObj->uniqueLocalAddress, ifName );
      }
      else
      {
         cmsLog_error("invalid ULA address: %s", newObj->uniqueLocalAddress);
         return CMSRET_INVALID_PARAM_VALUE;
      }
   }   

   return ret;
}


CmsRet rcl_delegatedAddressObject( _DelegatedAddressObject *newObj,
                const _DelegatedAddressObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode  __attribute__((unused)))
{
   _LanIpIntfObject *ipIntfObj = NULL;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   char ifName[BUFLEN_32];
   CmsRet ret = CMSRET_SUCCESS;

   iidStack1 = *iidStack;
   if (cmsObj_getAncestorFlags(MDMOID_LAN_IP_INTF, MDMOID_DELEGATED_ADDRESS,
                             &iidStack1, OGF_NO_VALUE_UPDATE, 
                             (void **) &ipIntfObj) != CMSRET_SUCCESS)
   {
      cmsLog_error("Current delegated address object has no ancestor.");
      return CMSRET_MDM_TREE_ERROR;
   }

   cmsUtl_strncpy(ifName, ipIntfObj->X_BROADCOM_COM_IfName, BUFLEN_32);
   cmsObj_free((void **) &ipIntfObj);

   if (ADD_NEW(newObj, currObj))
   {
       return CMSRET_SUCCESS;
   }

   if ( (newObj != NULL) && (currObj != NULL) )
   {
      if (IS_EMPTY_STRING(newObj->IPv6InterfaceAddress) && 
          !IS_EMPTY_STRING(currObj->IPv6InterfaceAddress) )
      {
         char cmdLine[BUFLEN_128];

         snprintf(cmdLine, sizeof(cmdLine), "ip -6 addr del %s dev %s 2>/dev/null", 
                  currObj->IPv6InterfaceAddress, ifName);
         rut_doSystemAction("rut", cmdLine);
      }
      else if ( cmsUtl_isValidIpAddress(AF_INET6, newObj->IPv6InterfaceAddress) )
      {
         rutLan_setIPv6Address( newObj->IPv6InterfaceAddress, ifName,
                                           currObj->IPv6InterfaceAddress, ifName );

         /* 
          * If DS-Lite tunnel is associated with a WAN interface without WAN addr,
          * we need to take LAN addr to configure the local tunnel address.
          */
         if ((cmsUtl_strcmp(newObj->mode, MDMVS_WANDELEGATED) == 0) && 
             rutTunnel_activateByLanAddr(newObj->delegatedConnection))
         {
            rutTunnel_control(newObj->delegatedConnection, MDMVS_IPV4INIPV6, TRUE);
#ifdef SUPPORT_UPNP
            if (rutUpnp_pcpOfLanAddr(newObj->delegatedConnection))
            {
               if (rut_isUpnpEnabled() && rutUpnp_checkRunStatusWithDelay() == FALSE)
               {
                  rut_restartUpnp(NULL);
               }
            }
#endif
         }
      }
      else
      {
         cmsLog_error("invalid delegated address: %s", newObj->IPv6InterfaceAddress);
         return CMSRET_INVALID_PARAM_VALUE;
      }
   }
   else if ( (newObj == NULL) && (currObj != NULL) )
   {
      char cmdLine[BUFLEN_128];

      snprintf(cmdLine, sizeof(cmdLine), "ip -6 addr del %s dev %s 2>/dev/null", 
               currObj->IPv6InterfaceAddress, ifName);
      rut_doSystemAction("rut", cmdLine);
   }

   /* Since the address changes at brx, restart dhcp6s to get the latest address information */
   {
      _LanHostCfgObject *hostCfg = NULL;
      _IPv6LanHostCfgObject *hostCfg6 = NULL;

      iidStack1 = *iidStack;
      if (cmsObj_getAncestor(MDMOID_LAN_HOST_CFG, MDMOID_DELEGATED_ADDRESS,
                             &iidStack1, (void **) &hostCfg) != CMSRET_SUCCESS)
      {
         cmsLog_error("Current delegated address object has no ancestor of lanhostcfg.");
         return CMSRET_MDM_TREE_ERROR;
      }

      cmsObj_free((void **) &hostCfg);

      if (cmsObj_get(MDMOID_I_PV6_LAN_HOST_CFG, &iidStack1, 0, (void **) &hostCfg6) != CMSRET_SUCCESS)
      {
         cmsLog_error("Current delegated address object cannot find ipv6lanhostcfg.");
         return CMSRET_MDM_TREE_ERROR;
      }

      cmsObj_set(hostCfg6, &iidStack1);
      cmsObj_free((void **) &hostCfg6);
   }
   
   return ret;
}


CmsRet rcl_ipv6LanHostCfgObject(_IPv6LanHostCfgObject *newObj,
                                const _IPv6LanHostCfgObject *currObj,
                                const InstanceIdStack *iidStack  __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode  __attribute__((unused)))
{
   char *dnsServer  = NULL;
   char *domainName  = NULL;
   char *sitePrefix = NULL;
   CmsRet ret, r2;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   if (newObj != NULL && currObj == NULL)
   {
      UINT32 pid;
      cmsLog_debug("startup condition");
      /* for single instance object, this is the startup condition */

      /* TODO: launch rastatus6 */
      if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_RASTATUS6,  NULL, 0)) == CMS_INVALID_PID)
      {
         cmsLog_error("failed to restart rastatus6");
         ret = CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsLog_debug("rastatus6 started with pid=%d", pid);
      }

      /*
       * Process DNS info
      */
      if (strcmp(newObj->IPv6DNSConfigType, MDMVS_STATIC) == 0)
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->IPv6DNSWANConnection);
         if (newObj->DHCPv6ServerEnable)
         {
            if ((ret = rut_restartDhcp6s(newObj->IPv6DNSServers, newObj->IPv6DomainName, 
                                         newObj->statefulDHCPv6Server, newObj->minInterfaceID, 
                                         newObj->maxInterfaceID, newObj->DHCPv6LeaseTime, NULL)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rut_restartDhcp6s returns error. ret=%d", ret);
               return ret;
            }
         }
      }
      else  /* IPv6DNSConfigType == MDMVS_DHCP */
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->IPv6DNSServers);
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->IPv6DomainName);
      }
   }
   else if (newObj != NULL && currObj != NULL)
   {
      cmsLog_debug("new->DHCPv6ServerEnable=%d, old->DHCPv6ServerEnable=%d\n"
                      "new->IPv6DNSConfigType=%s, old->IPv6DNSConfigType=%s\n"
                      "new->IPv6DNSWANConnection=%s, old->IPv6DNSWANConnection=%s\n"
                      "new->IPv6DomainName=%s, old->IPv6DomainName=%s\n"
                      "new->IPv6DNSServers=%s, old->IPv6DNSServers=%s\n",
                      newObj->DHCPv6ServerEnable, currObj->DHCPv6ServerEnable,
                      newObj->IPv6DNSConfigType, currObj->IPv6DNSConfigType,
                      newObj->IPv6DNSWANConnection, currObj->IPv6DNSWANConnection,
                      newObj->IPv6DomainName, currObj->IPv6DomainName,
                      newObj->IPv6DNSServers, currObj->IPv6DNSServers);

      cmsLog_debug("new->StatefulDHCPv6Server=%d, old->StatefulDHCPv6Server=%d\n"
                      "new->MinInterfaceID=%s, old->MinInterfaceID=%s\n"
                      "new->MaxInterfaceID=%s, old->MaxInterfaceID=%s\n"
                      "new->DHCPv6LeaseTime=%d, old->DHCPv6LeaseTime=%d\n",
                      newObj->statefulDHCPv6Server, currObj->statefulDHCPv6Server,
                      newObj->minInterfaceID, currObj->minInterfaceID,
                      newObj->maxInterfaceID, currObj->maxInterfaceID,
                      newObj->DHCPv6LeaseTime, currObj->DHCPv6LeaseTime);

      /*
       * Process DNS info 
      */
      if (!newObj->DHCPv6ServerEnable && currObj->DHCPv6ServerEnable)
      {
         rut_stopDhcp6s();
      }

      if (strcmp(newObj->IPv6DNSConfigType, MDMVS_STATIC) == 0)
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->IPv6DNSWANConnection);
         dnsServer = cmsMem_strdup(newObj->IPv6DNSServers);
         domainName = cmsMem_strdup(newObj->IPv6DomainName);
      }
      else  /* IPv6DNSConfigType == MDMVS_DHCP */
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->IPv6DNSServers);
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->IPv6DomainName);

         if ((r2=rutWan_getDns6Server(newObj->IPv6DNSWANConnection, &dnsServer, &domainName)) != CMSRET_SUCCESS)
         {
            cmsLog_debug("rutWan_getDns6Server returns error = %d", r2);
            cmsMem_free(dnsServer);
         }
      }

      /*
       * Process DHCPv6ServerEnable
      */
      if (newObj->DHCPv6ServerEnable)
      {
         if (strcmp(newObj->IPv6DNSConfigType, MDMVS_STATIC) != 0)
         {
            /* dynamic DNS */
            if (rut_isWanInterfaceUp(newObj->IPv6DNSWANConnection, FALSE))
            {
               rut_getDhcp6sPrefixFromInterface(newObj->IPv6DNSWANConnection, &sitePrefix);
               if ((ret = rut_restartDhcp6s(dnsServer, domainName, newObj->statefulDHCPv6Server, newObj->minInterfaceID, 
                                         newObj->maxInterfaceID, newObj->DHCPv6LeaseTime, sitePrefix)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("rut_restartDhcp6s returns error. ret=%d", ret);
               }
            }
            else
            {
               rut_stopDhcp6s();
            }
         }
         else
         {
            /* static DNS: FIXME: how to get sitePrefix for stateful dhcp6s without DNSWANConnection? */
            if ((ret = rut_restartDhcp6s(dnsServer, domainName, newObj->statefulDHCPv6Server, newObj->minInterfaceID, 
                                         newObj->maxInterfaceID, newObj->DHCPv6LeaseTime, NULL)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rut_restartDhcp6s returns error. ret=%d", ret);
               return ret;
            }
         }
      }

      cmsMem_free(sitePrefix);
      cmsMem_free(dnsServer);
      cmsMem_free(domainName);
   }

   return ret;

}  /* End of rcl_ipv6LanHostCfgObject() */


CmsRet rcl_radvdConfigMgtObject( _RadvdConfigMgtObject *newObj,
                const _RadvdConfigMgtObject *currObj,
                const InstanceIdStack *iidStack  __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode  __attribute__((unused)))

{
   CmsRet ret;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   if (newObj != NULL && currObj == NULL)
   {
      cmsLog_debug("startup condition");
      /* for single instance object, this is the startup condition */

      if (newObj->enable)
      {
         rut_createRadvdConf();
         rut_restartRadvd();
      }
   }
   else if (newObj != NULL && currObj != NULL)
   {
      cmsLog_debug("new->enable=%d, old->enable=%d\n", 
                               newObj->enable, currObj->enable);

      /*
       * Process Prefix Delegation
      */
      if (newObj->enable)
      {
         rut_createRadvdConf();
         rut_restartRadvd();
      }
      else if (currObj->enable)
      {
         rut_stopRadvd();
      }
   }

   return ret;
}


CmsRet rcl_uLAPrefixInfoObject( _ULAPrefixInfoObject *newObj  __attribute__((unused)),
                const _ULAPrefixInfoObject *currObj  __attribute__((unused)),
                const InstanceIdStack *iidStack  __attribute__((unused)),
                char **errorParam  __attribute__((unused)),
                CmsRet *errorCode  __attribute__((unused)))
{
   CmsRet ret;
   char   bridgeName[BUFLEN_8] = {"br0"}; /*radvd only work on br0 */
   char   ULAddress[BUFLEN_48];
   char   cmdLine[BUFLEN_128];
   UINT32 prefixLen;
   
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   if (newObj != NULL && currObj == NULL)
   {
      cmsLog_debug("startup condition");
      if (cmsUtl_isUlaPrefix(newObj->prefix))
         rutEbt_configICMPv6Reply(newObj->prefix, TRUE);
      /*br0 MAC may be changed later, skip it*/
      return CMSRET_SUCCESS;
   }
   
   if (newObj != NULL && currObj != NULL)
   {
      if ((currObj->enable != newObj->enable) || cmsUtl_strcmp(currObj->prefix, newObj->prefix))
      {
         if (currObj->enable && !IS_EMPTY_STRING(currObj->prefix))
         {
             /* remove old UL Address which get from UL Prefix */
             *ULAddress = '\0';
             cmsUtl_getULAddressByPrefix(currObj->prefix, bridgeName, ULAddress, &prefixLen);
             snprintf(cmdLine, sizeof(cmdLine), "ip -6 addr del %s/%u dev %s 2>/dev/null", ULAddress, prefixLen, bridgeName);
             rutIpt_configRoutingChain6(currObj->prefix, bridgeName, FALSE);
             rutEbt_configICMPv6Reply(currObj->prefix, FALSE);
             rut_doSystemAction("rut", cmdLine);
         }

         if (newObj->enable && !IS_EMPTY_STRING(newObj->prefix) )
         {
             /* add new UL Address which get from UL Prefix to the interface*/
             *ULAddress = '\0';
             cmsUtl_getULAddressByPrefix(newObj->prefix, bridgeName, ULAddress, &prefixLen);
             snprintf(cmdLine, sizeof(cmdLine), "ip -6 addr add %s/%u dev %s 2>/dev/null", ULAddress, prefixLen, bridgeName);
             rutIpt_configRoutingChain6(newObj->prefix, bridgeName, TRUE);
             rutEbt_configICMPv6Reply(newObj->prefix, TRUE);
             rut_doSystemAction("rut", cmdLine);
         }
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_radvdOtherInfoObject( _RadvdOtherInfoObject *newObj   __attribute__((unused)),
                const _RadvdOtherInfoObject *currObj   __attribute__((unused)),
                const InstanceIdStack *iidStack   __attribute__((unused)),
                char **errorParam   __attribute__((unused)),
				CmsRet *errorCode  __attribute__((unused)))
{
	_RadvdConfigMgtObject *radvdObj = NULL;
	InstanceIdStack iidStackRadvdCfg = EMPTY_INSTANCE_ID_STACK;
	CmsRet ret = CMSRET_SUCCESS;
   
	if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
	{
	   cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
	   return ret;
	}
    /*other info changed, update radvd conf*/
	if ((ret = cmsObj_getNextFlags(MDMOID_RADVD_CONFIG_MGT, &iidStackRadvdCfg, OGF_NO_VALUE_UPDATE, (void **)&radvdObj)) == CMSRET_SUCCESS)
	{
		if (radvdObj->enable)
		{
			rut_createRadvdConf();
			rut_restartRadvd();
		}
	}
	
	return ret;
}

CmsRet rcl_prefixInfoObject( _PrefixInfoObject *newObj  __attribute__((unused)),
                const _PrefixInfoObject *currObj  __attribute__((unused)),
                const InstanceIdStack *iidStack  __attribute__((unused)),
                char **errorParam  __attribute__((unused)),
                CmsRet *errorCode  __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

#endif  /* DMP_X_BROADCOM_COM_IPV6_1 */


