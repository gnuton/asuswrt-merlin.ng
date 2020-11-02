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
#include "mdm.h"
#include "rut_lan.h"
#include "cms_qdm.h"
#include "rut_iptables.h"

CmsRet rutLan_setLanIPv4Info_dev2(const char *ifName, const char *addr, const char *subnetmask)
{
   CmsRet ret = CMSRET_OBJECT_NOT_FOUND;
   UBOOL8 found = FALSE;
   /* this object is Device.IP.Interface.{i}.IPv4Address.{i}. */
   Dev2Ipv4AddressObject *ipv4Addr = NULL;
   /* this object is Device.IP.Interface.{i}. */
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;

   /* XXX TODO: if we change the statically configured IPAddr on br0, we
    * need to create a SERVICE DOWN->SERVICE UP transition in the service
    * state machine.
    */
   while (found == FALSE &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                              OGF_NO_VALUE_UPDATE, (void **) &ipIntfObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(ipIntfObj->name, ifName) == 0)
      {
         found = TRUE;
         if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_IPV4_ADDRESS,
                                     &iidStack, &iidStackChild,
                                     (void **) &ipv4Addr)) == CMSRET_SUCCESS)
         {
            CMSMEM_REPLACE_STRING_FLAGS(ipv4Addr->IPAddress, addr, mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(ipv4Addr->subnetMask, subnetmask, mdmLibCtx.allocFlags);

            if ((ret = cmsObj_set(ipv4Addr, &iidStackChild)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to set Dev2Ipv4AddressObject, ret=%d", ret);
            }

            cmsObj_free((void **) &ipv4Addr);
         }
      }

      cmsObj_free((void **) &ipIntfObj);
   }

   return ret;
}

UBOOL8 rutLan_isDhcpv4ServerPoolChanged_dev2
   (const _Dev2Dhcpv4ServerPoolObject *newObj,
    const _Dev2Dhcpv4ServerPoolObject *currObj)
{
   UBOOL8 changed = FALSE;

   if (newObj == NULL || currObj == NULL || !newObj->enable || !currObj->enable)
   {
      /* this function is not applicable if the objects do not already exist and
       * are enabled. */
      return FALSE;
   }
   
   if (cmsUtl_strcmp(newObj->minAddress, currObj->minAddress) ||
       cmsUtl_strcmp(newObj->maxAddress, currObj->maxAddress) ||
       cmsUtl_strcmp(newObj->subnetMask,currObj->subnetMask) ||
       cmsUtl_strcmp(newObj->DNSServers, currObj->DNSServers) ||
       cmsUtl_strcmp(newObj->domainName, currObj->domainName) ||
       cmsUtl_strcmp(newObj->IPRouters, currObj->IPRouters) ||
       newObj->leaseTime != currObj->leaseTime)         
   {            
      changed = TRUE;
   }         

   return changed;
}

UBOOL8 rutLan_isDhcpv4ServerPoolStaticAddressChanged_dev2
   (const _Dev2Dhcpv4ServerPoolStaticAddressObject *newObj,
    const _Dev2Dhcpv4ServerPoolStaticAddressObject *currObj)
{
   UBOOL8 changed = FALSE;

   if (newObj == NULL || currObj == NULL || !newObj->enable || !currObj->enable)
   {
      /* this function is not applicable if the objects do not already exist and
       * are enabled. */
      return FALSE;
   }
   
   if (cmsUtl_strcmp(newObj->chaddr, currObj->chaddr) ||
       cmsUtl_strcmp(newObj->yiaddr, currObj->yiaddr))
   {            
      changed = TRUE;
   }         

   return changed;
}

void rutLan_reconfigNatForAddressChange_dev2(const char *oldIpAddr, const char *oldMask,
                                        const char *newIpAddr, const char *newMask)
{
   char oldSubnet[CMS_IPADDR_LENGTH];
   char newSubnet[CMS_IPADDR_LENGTH];
   Dev2NatIntfSettingObject *NatIpIntf = NULL;
   struct in_addr ip, mask, subnet;
   InstanceIdStack iidStack;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("oldIpAddr/mask=%s/%s newIpAddr/mask=%s/%s", oldIpAddr, oldMask, newIpAddr, newMask);

   /* got this from rut_getIfSubnet */
   inet_aton(oldIpAddr, &ip);
   inet_aton(oldMask, &mask);
   subnet.s_addr = ip.s_addr & mask.s_addr;
   sprintf(oldSubnet, "%s", inet_ntoa(subnet));

   inet_aton(newIpAddr, &ip);
   inet_aton(newMask, &mask);
   subnet.s_addr = ip.s_addr & mask.s_addr;
   sprintf(newSubnet, "%s", inet_ntoa(subnet));

   /*
    * mwang_todo: This algorithm needs to check if the WAN device is part of an
    * interface group.  We don't support that feature right now, so this is OK.
    * When we do add support for routed WAN on interface group, then this needs to
    * be fixed also.
    */
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_NAT_INTF_SETTING, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &NatIpIntf)) == CMSRET_SUCCESS)
   {
      if (NatIpIntf->enable)
      {  
         char ifName[CMS_IFNAME_LENGTH];
         
         qdmIntf_fullPathToIntfnameLocked_dev2(NatIpIntf->interface, ifName);
         
         if (qdmIpIntf_isIpv4ServiceUpLocked_dev2(ifName, QDM_IPINTF_DIR_WAN))
         {
            cmsLog_debug("reconfig NAT on %s", ifName);
            rutIpt_deleteNatMasquerade(ifName, oldSubnet, oldMask);
            rutIpt_insertNatMasquerade(ifName, newSubnet, newMask, NatIpIntf->X_BROADCOM_COM_FullconeNATEnabled);
         }
      }
      cmsObj_free((void **) &NatIpIntf);
   }

   return;
}


#endif /* DMP_DEVICE2_BASELINE_1 */

