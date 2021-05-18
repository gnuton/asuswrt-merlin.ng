/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
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


#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_IPINTERFACE_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_iptables.h"
#include "rut_network.h"
#include "rut_route.h"
#include "rut_lan.h"
#include "rut_qos.h"
#include "rut2_ip.h"
#include "rut2_ipv6.h"
#include "cms_qdm.h"
#include "rut2_util.h"
#ifdef DMP_DEVICE2_IPV6INTERFACE_1
#include "rut_dnsproxy.h"
#endif


/*!\file rcl2_ip.c
 * \brief This file contains device 2 device.ip objects related functions.
 *
 */

CmsRet rcl_dev2IpObject( _Dev2IpObject *newObj __attribute__((unused)),
                const _Dev2IpObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2IpInterfaceObject( _Dev2IpInterfaceObject *newObj,
                const _Dev2IpInterfaceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   MdmPathDescriptor pathDesc;
   char *ipIntfFullPath=NULL;
   char ifname[CMS_IFNAME_LENGTH]={0};
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_IP_INTERFACE;
   pathDesc.iidStack = *iidStack;
   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &ipIntfFullPath)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
      return ret;
   }

   IF_STATUS_HAS_CHANGED_SET_LASTCHANGE(newObj, currObj);

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumIpInterface(iidStack, 1);
   }

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* if type is tunnel or tunneled, it is only for legacy tunnel mechanism */
      if (!cmsUtl_strcmp(newObj->type, MDMVS_TUNNEL) || !cmsUtl_strcmp(newObj->type, MDMVS_TUNNELED))
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
         return CMSRET_SUCCESS;
      }

      /*
       * We just enabled this IP.Interface.
       * -- Get ifname
       * -- Recalculate the Upstream (WAN) boolean.
       * -- Set status to LOWERLAYERDOWN.
       */
      if (IS_EMPTY_STRING(newObj->lowerLayers))
      {
         return CMSRET_INVALID_ARGUMENTS;
      }

      ret = rutIp_getIfnameFromLowerLayers(newObj->lowerLayers, ifname, sizeof(ifname));
      if (ret != CMSRET_SUCCESS)
      {
         return ret;
      }
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->name, ifname, mdmLibCtx.allocFlags);

      if (!cmsUtl_strncmp(newObj->name, "br", 2))
      {
          /* assume all IP.Interface with name of brx is LAN */
         newObj->X_BROADCOM_COM_Upstream = FALSE;
      }
      else if (newObj->X_BROADCOM_COM_BridgeService)
      {
         /* WAN Bridge Service is always WAN */
         newObj->X_BROADCOM_COM_Upstream = TRUE;

         /* Add Mac filter for Bridge WAN */
         rutIp_addMacFilterObject_dev2(ipIntfFullPath);
      }
      else
      {
         /* traverse the lower layers and figure out */
         newObj->X_BROADCOM_COM_Upstream = rutIp_isUpstream(newObj->lowerLayers);
      }

      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
      /* In the scenario where IP.Interface is disabled,
       * everything else is fully connected and UP, and then as the last
       * step, IP.Interface is enabled or in the edit case, the IP.interface is change
       * from disable to enable.  We need to send a CMS_MSG_INTFSTACK_PROPAGATE_STATUS
       * msg containing the layer below this one to ssk.  Must be the layer below
       * the IP.Interface layer because the interface stack code's special
       * handling of IP.Interface states are triggered when IP.Interface is
       * the higherLayer.
       */

      /* Only propagate the intf stack if ip interface enable flag is from
	  * off to on
	  */
      if (currObj && !currObj->enable && newObj->enable)
      {
         char propageLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
         char lowerLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};  
         
         cmsUtl_strncpy(propageLayerBuf, newObj->lowerLayers, sizeof(propageLayerBuf));
         
         if (cmsUtl_strstr(propageLayerBuf, "Device.PPP."))   /* test to see if it is ppp intf object or not */
         {
            /* Go down one more level if it is ppp intf object 
			* since ppp interface object parameters change requires the 
			* lowerlayer - EthernetVlanTermination object
			* to propagate the interface stacks up. 
			*/         
            memset(lowerLayerBuf, 0, sizeof(lowerLayerBuf));
            ret = qdmIntf_getFirstLowerLayerFromFullPathLocked_dev2(propageLayerBuf, lowerLayerBuf, sizeof(lowerLayerBuf));
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("Could not get the lowerlayer of %s, ret=%d", propageLayerBuf, ret);
               CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
               return ret;
            }            
            cmsUtl_strncpy(propageLayerBuf, lowerLayerBuf, sizeof(propageLayerBuf));
         }

         rutIp_sendIntfStackPropagateMsgToSsk(propageLayerBuf);
         
      }

      /*
       * Whenever we add or delete a WAN IP service, just update dhcpd
       * because we have to stop dhcpd if we are all bridge services.
       */
      if (newObj->X_BROADCOM_COM_Upstream)
      {
         rutLan_updateDhcpd();
      }

#if defined(DMP_DEVICE2_BASELINE_1) && defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1) //for hybrid IPv6
   /* For hybrid IPV6, There is no device 2 fwException, since Device.Firewall. object is not created, so cannot 
   * adding Device.Firewall.X_BROADCOM_COM_FirewallException.{i}. here
   */
   cmsLog_notice("Skipping Device.Firewall.X_BROADCOM_COM_FirewallException.{i}. for hybrid mode");
#else
      /* For PURE181, add fwException Object */
      rutIpt_AddfwExceptionforIPDevice_dev2(ipIntfFullPath);
#endif

   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      if (cmsUtl_strcmp(newObj->lowerLayers, currObj->lowerLayers))
      {
         /*
          * If lowerLayer changes, refresh our Upstream (WAN) boolean
          * and our name.  LowerLayers will change from blank to something,
          * and if object underneath is being deleted, from something to blank.
          */
         if (IS_EMPTY_STRING(newObj->lowerLayers))
         {
            return CMSRET_INVALID_ARGUMENTS;
         }

         newObj->X_BROADCOM_COM_Upstream = rutIp_isUpstream(newObj->lowerLayers);
         ret = rutIp_getIfnameFromLowerLayers(newObj->lowerLayers, ifname, sizeof(ifname));
         if (ret != CMSRET_SUCCESS)
         {
            return ret;
         }
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->name, ifname, mdmLibCtx.allocFlags);
      }

      /* check for service state machine changes */
      if (newObj->IPv4Enable &&
          cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv4ServiceStatus,
                        currObj->X_BROADCOM_COM_IPv4ServiceStatus))
      {
         /* IPv4 service status is changed. */
         rutIpv4Service_runStateMachine(newObj->X_BROADCOM_COM_IPv4ServiceStatus,
                                   currObj->X_BROADCOM_COM_IPv4ServiceStatus,
                                   ipIntfFullPath, newObj->name,
                                   newObj->X_BROADCOM_COM_Upstream,
                                   newObj->X_BROADCOM_COM_BridgeService);
      }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      if (newObj->IPv6Enable &&
          (cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ServiceStatus,
                        currObj->X_BROADCOM_COM_IPv6ServiceStatus) ||
           (newObj->X_BROADCOM_COM_Mflag_Upstream != currObj->X_BROADCOM_COM_Mflag_Upstream)
          ))
      {
         /* IPv6 service status is changed. */
         rutIpv6Service_runStateMachine(newObj->X_BROADCOM_COM_IPv6ServiceStatus,
                                   currObj->X_BROADCOM_COM_IPv6ServiceStatus,
                                   ipIntfFullPath, newObj->name,
                                   newObj->X_BROADCOM_COM_Upstream);
      }
#endif

      if ((!currObj->X_BROADCOM_COM_FirewallEnabled  && newObj->X_BROADCOM_COM_FirewallEnabled)) 
      { /* Enable firewall  */
         if(    cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv4ServiceStatus, MDMVS_SERVICEDOWN) 
             || cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ServiceStatus, MDMVS_SERVICEDOWN) )
         {
              cmsLog_notice("Starting firewall on %s", newObj->name);
              rutIpt_insertIpModules();
			  /* For default drop rule */
              rutIpt_initFirewall(PF_INET, newObj->name);
              rutIpt_initFirewallExceptions(newObj->name);
         }
      }
      else if ((currObj->X_BROADCOM_COM_FirewallEnabled  && !newObj->X_BROADCOM_COM_FirewallEnabled)) 
      { /* Disable Firewall */
         cmsLog_notice("remove firewall on %s", newObj->name);
         rutIpt_removeInterfaceIptableRules(newObj->name, TRUE);
         /*  Delete default rule and incoming rule obj for this interface, keep outgoing rule for bridge interface */
         rutIpt_RemovefwExceptionRule_dev2(ipIntfFullPath,MDMVS_ACCEPT);

         /* for LAN bridge ,need put Outgoing (DROP) rule back .. */
         if(newObj->X_BROADCOM_COM_Upstream == FALSE)
            rutIpt_initFirewallExceptions(newObj->name);
      }

#ifdef DMP_DEVICE2_BRIDGE_1 /* aka SUPPORT_PMAP */
      if ( cmsUtl_strcmp(newObj->X_BROADCOM_COM_GroupName, currObj->X_BROADCOM_COM_GroupName))
      {
         rutPMap_configIntfGrouping(newObj, currObj, ipIntfFullPath);
      }
#endif	  
	  
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /*
       * Whenever we add or delete a WAN IP service, just update dhcpd
       * because we have to stop dhcpd if we are all bridge services.
       */
      if (currObj->X_BROADCOM_COM_Upstream)
      {
         rutLan_updateDhcpd();
      }

      if (currObj->IPv4Enable)
      {
         if (newObj)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_IPv4ServiceStatus,
                                       MDMVS_SERVICEDOWN, mdmLibCtx.allocFlags);
         }
         if (cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv4ServiceStatus, MDMVS_SERVICEDOWN))
         {
            rutIpv4Service_runStateMachine(MDMVS_SERVICEDOWN,
                                  currObj->X_BROADCOM_COM_IPv4ServiceStatus,
                                  ipIntfFullPath, currObj->name,
                                  currObj->X_BROADCOM_COM_Upstream,
                                  currObj->X_BROADCOM_COM_BridgeService);
         }
      }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      if (currObj->IPv6Enable)
      {
         if (newObj)
         {
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->X_BROADCOM_COM_IPv6ServiceStatus,
                                       MDMVS_SERVICEDOWN, mdmLibCtx.allocFlags);
         }
         if (cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ServiceStatus, MDMVS_SERVICEDOWN))
         {
            rutIpv6Service_runStateMachine(MDMVS_SERVICEDOWN,
                               currObj->X_BROADCOM_COM_IPv6ServiceStatus,
                               ipIntfFullPath, currObj->name,
                               currObj->X_BROADCOM_COM_Upstream);
         }
      }
#endif

      if (DELETE_EXISTING(newObj, currObj))
      {

         /* update DLNA dms */
#ifdef DMP_X_BROADCOM_COM_DLNA_1
         rutLan_updateDlna();
#endif
         /*
          * Remove this ifName from DNS.Client.X_BROADCOM_COM_DnsIfNames
          */
         if (!IS_EMPTY_STRING(currObj->name))
         {
            rutNtwk_removeIpvxDnsIfNameFromList(CMS_AF_SELECT_IPVX, currObj->name);
         }

         /* Remove this ifName from list of potential default gateways */
         rutRt_removeDefaultGatewayIfUsed(currObj->name);

         /* Disable Firewall */
         cmsLog_notice("remove firewall on %s", currObj->name);
         rutIpt_removeInterfaceIptableRules(currObj->name, TRUE);
         /* Delete fwException object and all rules associate with interface */
         rutIpt_RemovefwExceptionforIPDevice_dev2(ipIntfFullPath);

         /* Del Mac filter */
         rutIp_delMacFilterObject_dev2(ipIntfFullPath);

         /* delete QoS Classifiers with ingress or egress on this L3 intf */
         rutQos_qMgmtClassDelete(currObj->name);

#ifdef DMP_DEVICE2_DHCPV4RELAY_1
         rutDhcpv4Relay_deleteForwardingEntry_dev2(ipIntfFullPath);
#endif

         rutUtil_modifyNumIpInterface(iidStack, -1);
      }
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);

   return ret;
}


CmsRet rcl_dev2Ipv4AddressObject( _Dev2Ipv4AddressObject *newObj,
                const _Dev2Ipv4AddressObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   InstanceIdStack ipIntfIidStack = *iidStack;
   CmsRet ret;

   cmsLog_debug("Entered:");

   /* first get parent ipIntfObj.  Will be needed later */
   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_IP_INTERFACE,
                                 MDMOID_DEV2_IPV4_ADDRESS,
                                 &ipIntfIidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **) &ipIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get parent IP.Interface, ret=%d", ret);
      return ret;
   }

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumIpv4Address(iidStack, 1);

      if (!cmsUtl_strcmp(newObj->addressingType, MDMVS_STATIC) &&
          newObj->enable)
      {
         /*
          * Notify ssk so it can update IP.Interface.Status and IPv4
          * Service state machine
          */
         if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, newObj->IPAddress))
         {
            rutIp_sendStaticAddrConfigToSsk(ipIntfObj->name, TRUE,
                                            TRUE, FALSE, FALSE);
         }
      }
   }

   /*
    * The normal rules for transitions of enable false->true or true->false
    * still apply but in order to accommodate a "set" without any parameter
    * changes (done by rutIp_activateInterface), the
    * logic is a little different here:
    *
    * Regardless of whether the enable param has changed, if it is TRUE, AND
    * parent IP.Interface.IPv4ServiceStatus == SERVICEUP, AND
    * current status != Enabled,
    * then configure IP address.
    *
    * Regardless of whether the enable param has changed, if it is TRUE, AND
    * parent IP.Interface.IPv4ServiceStatus != SERVICEUP, AND
    * current status == Enabled,
    * then unconfigure IP address.
    *
    */
   if (newObj && newObj->enable)
   {
      if ((!cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus, MDMVS_SERVICEUP)) &&
          (cmsUtl_strcmp(newObj->status, MDMVS_ENABLED)))
      {
         /* IP.Interface is IPv4 SERVICEUP, but we have not configured IPv4
          * addr yet, configure it now.
          */
         if (ipIntfObj->X_BROADCOM_COM_BridgeService)
         {
            /* In some unusual configs (homeplug), a Bridged WAN service
             * will get an IP addr.  Configure the actual bridge (e.g. br0)
             * not the IP.Interface.name (plc0).
             */
            rutIp_configureIpv4Addr(ipIntfObj->X_BROADCOM_COM_BridgeName,
                                    newObj->IPAddress, newObj->subnetMask);
         }
         else
         {
            rutIp_configureIpv4Addr(ipIntfObj->name,
                                    newObj->IPAddress, newObj->subnetMask);
         }
         /* configureIpv4Addr does not return code, so assume it worked */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,
                                     mdmLibCtx.allocFlags);
      }

      if ((cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus, MDMVS_SERVICEUP)) &&
          (!cmsUtl_strcmp(newObj->status, MDMVS_ENABLED)))
      {
         /* IP.Interface is not IPv4 SERVICEUP, and we have configured IPv4
          * addr on this intf, unconfigure it now.
          */
         if (ipIntfObj->X_BROADCOM_COM_BridgeService)
         {
            rutIp_unconfigureIpv4Addr(ipIntfObj->X_BROADCOM_COM_BridgeName);
         }
         else
         {
            rutIp_unconfigureIpv4Addr(ipIntfObj->name);
         }
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);
      }
   }

   if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      /*
       * Somebody has changed the static IP address (this happens when user
       * changes br0 IP address on WebUI).
       */
      if (!cmsUtl_strcmp(newObj->addressingType, MDMVS_STATIC) &&
          (cmsUtl_strcmp(newObj->IPAddress, currObj->IPAddress) ||
           cmsUtl_strcmp(newObj->subnetMask, currObj->subnetMask)))
      {
         UBOOL8 isAdd=FALSE;
         UBOOL8 isMod=FALSE;
         UBOOL8 isDel=FALSE;

         if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, newObj->IPAddress) &&
             cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, currObj->IPAddress))
         {
            /* IPaddress is now a non-zero form, but previously was, so this is an add */
            isAdd = TRUE;
         }
         else if (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, newObj->IPAddress) &&
                  !cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, currObj->IPAddress))
         {
            /* IPaddress is now a zero form, but previously was not, so this is a delete */
            isDel = TRUE;
         }
         else if (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, newObj->IPAddress) &&
                  cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, currObj->IPAddress))
         {
            /* IPaddress changed from 1 zero form to another, so basically no change */
            cmsLog_debug("Basically no change");
         }
         else
         {
            /* everything else is a modification of existing non-zero addr */
            isMod = TRUE;
         }

         /*
          * Notify ssk so it can update IP.Interface.Status and IPv4
          * Service state machine
          */
         if (isAdd || isMod || isDel)
         {
            rutIp_sendStaticAddrConfigToSsk(ipIntfObj->name, TRUE,
                                            isAdd, isMod, isDel);
         }
      }
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      rutIp_unconfigureIpv4Addr(ipIntfObj->name);

      if (DELETE_EXISTING(newObj, currObj))
      {
         rutUtil_modifyNumIpv4Address(iidStack, -1);
      }
      else
      {
         /* if not a delete (just disable existing), then update status */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);
		 
         if (!cmsUtl_strcmp(currObj->addressingType, MDMVS_STATIC))
         {
            /*
             * Notify ssk so it can update IP.Interface.Status and IPv4
             * Service state machine
             */
            if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, currObj->IPAddress))
            {
               rutIp_sendStaticAddrConfigToSsk(ipIntfObj->name, TRUE,
                                               FALSE, FALSE, TRUE);
            }
         }
      }
   }

   cmsObj_free((void **)&ipIntfObj);

   return ret;
}

#ifdef DMP_DEVICE2_IPV6INTERFACE_1
CmsRet rcl_dev2Ipv6AddressObject( _Dev2Ipv6AddressObject *newObj,
                const _Dev2Ipv6AddressObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   InstanceIdStack ipIntfIidStack = *iidStack;
   UBOOL8 isSpecialCase = FALSE;
   char tmpBuf[BUFLEN_40] = {0};
   char *ipIntfFullPath = NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   /* XXX TOOD: update the numberofentries param in IP object */
   /* first get parent ipIntfObj.  Will be needed later */
   ret = cmsObj_getAncestorFlags(MDMOID_DEV2_IP_INTERFACE,
                                 MDMOID_DEV2_IPV6_ADDRESS,
                                 &ipIntfIidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **) &ipIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get parent IP.Interface, ret=%d", ret);
      return ret;
   }

   /*
    * if an object is created/enabled that is not STATIC, this
    * handler function must configure the IP address.
    * (For STATIC address objects, the address is configured by the
    * IP.Interface object when its status goes to UP).
    */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      if (cmsUtl_strcmp(newObj->origin, MDMVS_STATIC))
      {
         if (cmsUtl_isValidIpAddress(AF_INET6, newObj->IPAddress) == TRUE)
         {
            char prefix[CMS_IPADDR_LENGTH];

            /* prefix may not exist for IANA case */
            if (newObj->prefix && qdmIpv6_fullPathToPefixLocked_dev2(newObj->prefix, prefix) != CMSRET_SUCCESS)
            {
               cmsLog_error("cannot get prefix from %s", newObj->prefix);
            }

            rutIp_configureIpv6Addr(ipIntfObj->name, newObj->IPAddress, prefix);

            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,
                                        mdmLibCtx.allocFlags);
         }
      }
      else
      {
         /* this IPv6Address object is origin STATIC */
         /* If the IP.Interface is already UP, then we need to configure
          * the static IP address here.
          */
         if (!cmsUtl_strcmp(ipIntfObj->status, MDMVS_UP))
         {
            if (cmsUtl_isValidIpAddress(AF_INET6, newObj->IPAddress) == TRUE)
            {
               char prefix[CMS_IPADDR_LENGTH];

               if (qdmIpv6_fullPathToPefixLocked_dev2(newObj->prefix, prefix) != CMSRET_SUCCESS)
               {
                  cmsLog_error("cannot get prefix from %s", newObj->prefix);
               }

               rutIp_configureIpv6Addr(ipIntfObj->name, newObj->IPAddress, prefix);

               CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,
                                           mdmLibCtx.allocFlags);
            }
         }

         /*
          * Notify ssk so it can update IP.Interface.Status and IPv6
          * Service state machine
          */
         if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, newObj->IPAddress))
         {
            rutIp_sendStaticAddrConfigToSsk(ipIntfObj->name, FALSE,
                                            TRUE, FALSE, FALSE);

            if (!ADD_NEW(newObj, currObj) && (ipIntfObj->X_BROADCOM_COM_Upstream == FALSE))
            {
               cmsUtl_strcpy(tmpBuf, "ENABLE_NEW_OR_ENABLE_EXISTING");
               isSpecialCase = TRUE;
            }
         }

      }
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      if (cmsUtl_strcmp(newObj->origin, MDMVS_STATIC))
      {
         /* This is a dynamic IP address object.  If there are any changes
          * ssk2_connstatus.c should always add a new addr object.
          * And the original address should timeout by its plt/vlt??
          * So there should be no dynamic addr object got deleted?
          */
         if (cmsUtl_strcmp(newObj->IPAddress, currObj->IPAddress))
         {
#if 0
            char prefix[CMS_IPADDR_LENGTH];

            if (cmsUtl_isValidIpAddress(AF_INET6, currObj->IPAddress) == TRUE)
            {
               rutIp_unconfigureIpv6Addr(ipIntfObj->name, currObj->IPAddress);
            }

            if (qdmIpv6_fullPathToPefixLocked_dev2(newObj->prefix, prefix) != CMSRET_SUCCESS)
            {
               cmsLog_error("cannot get prefix from %s", newObj->prefix);
            }

            /* now configure the new one */
            rutIp_configureIpv6Addr(ipIntfObj->name, newObj->IPAddress, prefix);
#else
            cmsLog_notice("changing dynamic IP address??");
#endif
         }
      }
      else
      {
         /* 
          * This IPv6Address object is origin STATIC:
          * If this is LAN brx, it is the ULA address which could change 
          * If this is WAN, it should not be changed
          */

         /*
          * If IP.Interface of brx is already UP, change the ULA address
          */
         if (!cmsUtl_strcmp(ipIntfObj->status, MDMVS_UP) &&
             cmsUtl_strcmp(newObj->IPAddress, currObj->IPAddress) &&
             (ipIntfObj->X_BROADCOM_COM_Upstream == FALSE))
         {
            char prefix[CMS_IPADDR_LENGTH];

            if (cmsUtl_isValidIpAddress(AF_INET6, currObj->IPAddress) == TRUE)
            {
               rutIp_unconfigureIpv6Addr(ipIntfObj->name, currObj->IPAddress);
            }

            if (qdmIpv6_fullPathToPefixLocked_dev2(newObj->prefix, prefix) != CMSRET_SUCCESS)
            {
               cmsLog_error("cannot get prefix from %s", newObj->prefix);
            }

            /* now configure the new one */
            rutIp_configureIpv6Addr(ipIntfObj->name, newObj->IPAddress, prefix);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,
                                        mdmLibCtx.allocFlags);
         }

         /*
          * Notify ssk so it can update IP.Interface.Status and IPv6
          * Service state machine
          */
         if (cmsUtl_strcmp(newObj->IPAddress, currObj->IPAddress))
         {
            UBOOL8 isAdd=FALSE;
            UBOOL8 isMod=FALSE;
            UBOOL8 isDel=FALSE;

            if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, newObj->IPAddress) &&
                cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, currObj->IPAddress))
            {
               /* IPaddress is now a non-zero form, but previously was, so this is an add */
               isAdd = TRUE;
            }
            else if (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, newObj->IPAddress) &&
                     !cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, currObj->IPAddress))
            {
               /* IPaddress is now a zero form, but previously was not, so this is a delete */
               isDel = TRUE;
            }
            else if (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, newObj->IPAddress) &&
                     cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, currObj->IPAddress))
            {
               /* IPaddress changed from 1 zero form to another, so basically no change */
               cmsLog_debug("Basically no change");
            }
            else
            {
               /* everything else is a modification of existing non-zero addr */
               isMod = TRUE;
            }

            if (isAdd || isMod || isDel)
            {
               rutIp_sendStaticAddrConfigToSsk(ipIntfObj->name, FALSE,
                                               isAdd, isMod, isDel);

               if (ipIntfObj->X_BROADCOM_COM_Upstream == FALSE)
               {
                  cmsUtl_strcpy(tmpBuf, "POTENTIAL_CHANGE_OF_EXISTING");
                  isSpecialCase = TRUE;
               }
            }
         }
      }
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      rutIp_unconfigureIpv6Addr(ipIntfObj->name, currObj->IPAddress);

      if (!cmsUtl_strcmp(currObj->origin, MDMVS_STATIC))
      {
         /*
          * Notify ssk so it can update IP.Interface.Status and IPv6
          * Service state machine
          */
         if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, currObj->IPAddress))
         {
            rutIp_sendStaticAddrConfigToSsk(ipIntfObj->name, FALSE,
                                            FALSE, FALSE, TRUE);

            if (ipIntfObj->X_BROADCOM_COM_Upstream == FALSE)
            {
               cmsUtl_strcpy(tmpBuf, "DELETE_OR_DISABLE_EXISTING");
               isSpecialCase = TRUE;
            }
         }
      }

      if (newObj)
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);
      }
   }

   if (isSpecialCase)
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_IP_INTERFACE;
      pathDesc.iidStack = ipIntfIidStack;

      ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc,
                                                    &ipIntfFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
         return ret;
      }

      if (!qdmIpIntf_isAssociatedWanInterfaceUpLocked_dev2(ipIntfFullPath, CMS_AF_SELECT_IPV6))
      {
         cmsLog_notice("in <%s> special case", tmpBuf);
         rutDpx_updateDnsproxy();
      }
   }

   cmsObj_free((void **)&ipIntfObj);

   return ret;
}

CmsRet rcl_dev2Ipv6PrefixObject( _Dev2Ipv6PrefixObject *newObj,
                const _Dev2Ipv6PrefixObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* We use origin=AutoConfigured to indicate ULA prefix */
      if (newObj->X_BROADCOM_COM_UniqueLocalFlag)
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->origin, MDMVS_AUTOCONFIGURED, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->staticType, MDMVS_INAPPLICABLE, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
      }

      /* prefix of ULA address (origin and staticType are STATIC) is always with status ENABLED */
      if (!cmsUtl_strcmp(newObj->origin, MDMVS_STATIC) && !cmsUtl_strcmp(newObj->staticType, MDMVS_STATIC))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
      }
   }
#if 0
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      if (cmsUtl_strcmp(newObj->origin, MDMVS_STATIC))
      {

      }
      else
      {

      }
   }
#endif
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      char *prefixFullPath=NULL;
      MdmPathDescriptor pathDesc;
      UBOOL8 isDisable = newObj?TRUE:FALSE;

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_IPV6_PREFIX;
      pathDesc.iidStack = *iidStack;

      if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &prefixFullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
         return ret;
      }

      if (newObj)
      {
         cmsLog_debug("disable %s", prefixFullPath);
      }
      else
      {
         cmsLog_debug("delete %s", prefixFullPath);
      }

      /*
       * 1. if delete/disable parent prefix, need to delete/disable child prefix too
       * 2. if delete/disable prefix, need to do the following.
       *    - delete/disable associated address if any
       *    - if the prefix is used for radvd, trigger radvd to reload
       *    - if the prefix is used for dhcp6s, trigger dhcp6s to reload
       */
      ret = rutIp_disableOrDeleteChildPrefix(prefixFullPath, isDisable);

      if (ret == CMSRET_SUCCESS)
      {
         cmsLog_debug("case of child prefix");

         ret = rutIp_updateSystemForPrefixChange(prefixFullPath, isDisable);

         if (isDisable)
         {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
         }
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(prefixFullPath);
   }

   return ret;
}

#endif  /* DMP_DEVICE2_IPV6INTERFACE_1 */




CmsRet rcl_dev2IpStatsObject( _Dev2IpStatsObject *newObj __attribute__((unused)),
                const _Dev2IpStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_IPINTERFACE_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */

