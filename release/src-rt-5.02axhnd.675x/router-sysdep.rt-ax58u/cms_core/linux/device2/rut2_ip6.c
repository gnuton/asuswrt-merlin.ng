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
/* this file touches TR181 IPv6 objects */
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1 /* aka SUPPORT_IPV6 */

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "cms_strconv2.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_ipv6.h"


/*!\file rut2_ip6.c
 * \brief IPv6 helper functions for rcl2_ip.c and stl2_ip.c
 *
 */


void rutIp_configureStaticIpv6Addrs(const InstanceIdStack *ipIntfIidStack,
                                    const char *ipIntfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
   CmsRet ret;

   if (cmsUtl_strlen(ipIntfName) == 0)
   {
      /* for whatever reason, the ifname is not yet known.  This function
       * cannot do anything without the ifname.
       */
      cmsLog_debug("no ipIntfName, just return");
      return;
   }

   while ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS,
                              ipIntfIidStack,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&ipv6AddrObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ipv6AddrObj->origin, MDMVS_STATIC) &&
          cmsUtl_isValidIpAddress(AF_INET6, ipv6AddrObj->IPAddress) == TRUE)
      {
         char prefix[CMS_IPADDR_LENGTH];

         if (qdmIpv6_fullPathToPefixLocked_dev2(ipv6AddrObj->prefix, prefix) != CMSRET_SUCCESS)
         {
            cmsLog_error("cannot get prefix from %s", ipv6AddrObj->prefix);
         }

         rutIp_configureIpv6Addr(ipIntfName, ipv6AddrObj->IPAddress, prefix);

         CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->status, MDMVS_ENABLED,
                                     mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->IPAddressStatus,
                                     MDMVS_PREFERRED, mdmLibCtx.allocFlags);

         if (cmsObj_set(ipv6AddrObj, &iidStack) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set ipv6AddrObj");
         }
      }

      cmsObj_free((void **) &ipv6AddrObj);
   }

   return;
}


void rutIp_configureIpv6Addr(const char *ifname, const char *addr, 
                             const char *prefix  __attribute__((unused)))
{
   if (cmsUtl_strlen(ifname) == 0)
   {
      /* for whatever reason, the ifname is not yet known.  This function
       * cannot do anything without the ifname.
       */
      cmsLog_debug("no ifname, just return");
      return;
   }

   /* TODO: Verify address with the prefix */
   if (cmsUtl_isValidIpAddress(AF_INET6, addr) == TRUE)
   {
      char cmdStr[BUFLEN_128];
      Dev2IpInterfaceObject *ipIntfObj = NULL;
      InstanceIdStack iidStack;
      InstanceIdStack iidStackChild;
      UBOOL8 found = FALSE;
      UBOOL8 isWan = FALSE;
      UBOOL8 isStateful = FALSE;
      UBOOL8 onLink = TRUE;

      /* 1: Check if it is a WAN interface */
      INIT_INSTANCE_ID_STACK(&iidStack);
      while (!found &&
                 (cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                    OGF_NO_VALUE_UPDATE, (void **)&ipIntfObj) == CMSRET_SUCCESS))
      {
         if (!cmsUtl_strcmp(ipIntfObj->name, ifname))
         {
            isWan = ipIntfObj->X_BROADCOM_COM_Upstream;
            found = TRUE;
         }
         cmsObj_free((void **) &ipIntfObj);
      }

      /* 2: What if the address is stateful */
      if (isWan)
      {
         Dev2Ipv6AddressObject *ipv6AddrObj = NULL;

         found = FALSE;
         INIT_INSTANCE_ID_STACK(&iidStackChild);
         while (!found &&
                    (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS,
                              &iidStack, &iidStackChild,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&ipv6AddrObj) == CMSRET_SUCCESS))
         {
            if (!cmsUtl_strcmp(ipv6AddrObj->IPAddress, addr))
            {
               if (!cmsUtl_strcmp(ipv6AddrObj->origin, MDMVS_AUTOCONFIGURED))
               {
                  cmsLog_debug("no action for SLAAC address");
                  cmsObj_free((void **) &ipv6AddrObj);
                  return;
               }
               else if (!cmsUtl_strcmp(ipv6AddrObj->origin, MDMVS_DHCPV6))
               {
                  isStateful = TRUE;
               }
               found = TRUE;
            }
            cmsObj_free((void **) &ipv6AddrObj);
         }
      }

      /* 3: Follow RFC 7084 Section 4.2 (WAA-2) */
      if (isStateful)
      {
         Dev2Ipv6PrefixObject *ipv6PrefixObj = NULL;

         found = FALSE;
         INIT_INSTANCE_ID_STACK(&iidStackChild);
         while (!found &&
                    (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_PREFIX,
                                  &iidStack, &iidStackChild,
                                  OGF_NO_VALUE_UPDATE,
                                  (void **)&ipv6PrefixObj) == CMSRET_SUCCESS))
				 {
            if (!cmsUtl_strcmp(ipv6PrefixObj->origin, MDMVS_ROUTERADVERTISEMENT) &&
                !cmsUtl_strcmp(ipv6PrefixObj->staticType, MDMVS_INAPPLICABLE) &&
                cmsNet_isHostInSameSubnet(addr, ipv6PrefixObj->prefix))
            {
               /* evaluate L (Onlink) Flag in the RA PIO */
               onLink = ipv6PrefixObj->onLink;
               found = TRUE;
            }
            cmsObj_free((void **) &ipv6PrefixObj);
         }
      }

      snprintf(cmdStr, sizeof(cmdStr), "ip -6 addr add %s/64 dev %s", 
               addr, ifname);

      /* The interface route at WAN should not happen when advertised
       * RA message with the L Flag clear set.
       */
      strncat(cmdStr, onLink? "" : " noprefixroute", sizeof(cmdStr)-1);

      rut_doSystemAction("rut2_ip6", cmdStr);

      rutMulti_reloadMcpd();
   }
   else
   {
      cmsLog_error("invalid addr: %s", addr);
   }

   return;
}


void rutIp_unconfigureStaticIpv6Addrs(const InstanceIdStack *ipIntfIidStack,
                                      const char *ipIntfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
   CmsRet ret;

   if (cmsUtl_strlen(ipIntfName) == 0)
   {
      /* for whatever reason, the ifname is not yet known.  This function
       * cannot do anything without the ifname.
       */
      cmsLog_debug("no ipIntfName, just return");
      return;
   }

   while ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS,
                              ipIntfIidStack,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&ipv6AddrObj)) == CMSRET_SUCCESS)
   {
      /* XXX fill in */
      cmsObj_free((void **) &ipv6AddrObj);
   }

   return;
}


void rutIp_unconfigureIpv6Addr(const char *ipIntfName, const char *addr)
{
   char cmdStr[BUFLEN_128];

   if (cmsUtl_strlen(ipIntfName) == 0)
   {
      /* for whatever reason, the ifname is not yet known.  This function
       * cannot do anything without the ifname.
       */
      cmsLog_debug("no ipIntfName, just return");
      return;
   }

   /* By the time we try to unconfigure the IPv6 address, the interface
    * might have already been deleted, so suppress error message from system.
    */
   snprintf(cmdStr, sizeof(cmdStr), "ip -6 addr delete %s/64 dev %s 2>/dev/null",
            addr, ipIntfName);

   rut_doSystemAction("rut2_ip6", cmdStr);

   return;
}


UBOOL8 rutIp_findIpv6Addr(const InstanceIdStack *iidStackIpIntf, const char *addr, const char *origin, InstanceIdStack *iidStackIpv6Addr)
{
   Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
   UBOOL8 found = FALSE;
   CmsRet ret;

   cmsLog_debug("addr/origin: %s/%s", addr, origin);
   INIT_INSTANCE_ID_STACK(iidStackIpv6Addr);
   while (!found && 
              (ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS,
                              iidStackIpIntf, iidStackIpv6Addr,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&ipv6AddrObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ipv6AddrObj->IPAddress, addr) && 
          (!origin || !cmsUtl_strcmp(ipv6AddrObj->origin, origin)))
      {
         found = TRUE;
      }
      cmsObj_free((void **) &ipv6AddrObj);
   }

   cmsLog_debug("found<%d>", found);
   return found;
}


CmsRet rutIp_addIpv6Addr(const InstanceIdStack *iidStackIpIntf,
                         const char *addr, const char *origin,
                         const char *prefix, int plt, int vlt)
{
   Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
   InstanceIdStack iidStack;
   char timestring[BUFLEN_128];
   CmsRet ret = CMSRET_SUCCESS;

   iidStack = *iidStackIpIntf;

   if ((ret = cmsObj_addInstance(MDMOID_DEV2_IPV6_ADDRESS, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add MDMOID_DEV2_IPV6_ADDRESS Instance, ret = %d", ret);
      return ret;
   } 

   if ((ret = cmsObj_get(MDMOID_DEV2_IPV6_ADDRESS, &iidStack, 0, (void **) &ipv6AddrObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get ipv6AddrObj, ret = %d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_IPV6_ADDRESS, &iidStack);
      return ret;
   }

   ipv6AddrObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->IPAddress, addr, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->origin, origin, mdmLibCtx.allocFlags);
   if (prefix)
   {
      CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->prefix, prefix, mdmLibCtx.allocFlags);
   }
   ipv6AddrObj->X_BROADCOM_COM_Plt = plt;
   ipv6AddrObj->X_BROADCOM_COM_Vlt = vlt;
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->IPAddressStatus, MDMVS_PREFERRED, mdmLibCtx.allocFlags);
   cmsTms_getXSIDateTime(plt, timestring, sizeof(timestring));
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->preferredLifetime, timestring, mdmLibCtx.allocFlags);
   cmsTms_getXSIDateTime(vlt, timestring, sizeof(timestring));
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->validLifetime, timestring, mdmLibCtx.allocFlags);

   cmsLog_debug("addr/orig/prefix/plt/vlt: %s/%s/%s/%d/%d/%s/%s", 
                      ipv6AddrObj->IPAddress, ipv6AddrObj->origin, ipv6AddrObj->prefix, 
                      ipv6AddrObj->X_BROADCOM_COM_Plt, ipv6AddrObj->X_BROADCOM_COM_Vlt, 
                      ipv6AddrObj->preferredLifetime, ipv6AddrObj->validLifetime);
   
   if ((ret = cmsObj_set(ipv6AddrObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ipv6AddrObj. ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_IPV6_ADDRESS, &iidStack);
   }
   else
   {
      if (cmsUtl_strcmp(origin, MDMVS_STATIC) && (ret = cmsObj_setNonpersistentInstance(MDMOID_DEV2_IPV6_ADDRESS, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set non-persistent address. ret=%d", ret);
      }

   }
   cmsObj_free((void **) &ipv6AddrObj);

   return ret;
}


CmsRet rutIp_replaceIpv6Addr(const InstanceIdStack *iidStackIpv6Addr, 
                             const char *addr, const char *origin, 
                             const char *prefix, int plt, int vlt)
{
   Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
   char timestring[BUFLEN_128];
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsObj_get(MDMOID_DEV2_IPV6_ADDRESS, iidStackIpv6Addr, 0, (void **) &ipv6AddrObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get ipv6AddrObj, ret = %d", ret);
      return ret;
   }

   ipv6AddrObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->IPAddress, addr, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->origin, origin, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->prefix, prefix, mdmLibCtx.allocFlags);
   ipv6AddrObj->X_BROADCOM_COM_Plt = plt;
   ipv6AddrObj->X_BROADCOM_COM_Vlt = vlt;
   cmsTms_getXSIDateTime(plt, timestring, sizeof(timestring));
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->preferredLifetime, timestring, mdmLibCtx.allocFlags);
   cmsTms_getXSIDateTime(vlt, timestring, sizeof(timestring));
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->validLifetime, timestring, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6AddrObj->IPAddressStatus, MDMVS_PREFERRED, mdmLibCtx.allocFlags);
   cmsLog_debug("addr/orig/prefix/plt/vlt: %s/%s/%s/%d/%d", 
              ipv6AddrObj->IPAddress, ipv6AddrObj->origin, ipv6AddrObj->prefix, 
              ipv6AddrObj->X_BROADCOM_COM_Plt, ipv6AddrObj->X_BROADCOM_COM_Vlt);
   
   if ((ret = cmsObj_set(ipv6AddrObj, iidStackIpv6Addr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ipv6AddrObj. ret=%d", ret);
   }
   else
   {
      if (cmsUtl_strcmp(origin, MDMVS_STATIC) && (ret = cmsObj_setNonpersistentInstance(MDMOID_DEV2_IPV6_ADDRESS, &iidStackIpv6Addr)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set non-persistent address. ret=%d", ret);
      }

   }
   cmsObj_free((void **) &ipv6AddrObj);

   return ret;
}


CmsRet rutIp_deleteIpv6Addr(const InstanceIdStack *iidStackIpIntf, const char *addr, const char *origin)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack savedIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("addr/origin: %s/%s", addr, origin);
   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS,
                              iidStackIpIntf, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&ipv6AddrObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ipv6AddrObj->IPAddress, addr) && 
          (!origin || !cmsUtl_strcmp(ipv6AddrObj->origin, origin)))
      {
         ret = cmsObj_deleteInstance(MDMOID_DEV2_IPV6_ADDRESS, &iidStack);
         /* since we did a delete, restore iidStack to last good one */
         iidStack = savedIidStack;
      }

      cmsObj_free((void **) &ipv6AddrObj);
      /* save current iidStack in case we delete the next one */
      savedIidStack = iidStack;
   }

   return ret;
}


UBOOL8 rutIp_findIpv6Prefix(const InstanceIdStack *iidStackIpIntf, const char *prefix,
                                                  const char *origin, const char *staticType, InstanceIdStack *iidStackIpv6Prefix)
{
   Dev2Ipv6PrefixObject *ipv6PrefixObj=NULL;
   UBOOL8 found = FALSE;
   CmsRet ret;

   cmsLog_debug("prefix/origin/staticT: %s/%s/%s", prefix, origin, staticType);
   INIT_INSTANCE_ID_STACK(iidStackIpv6Prefix);
   while (!found && 
              (ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_PREFIX,
                              iidStackIpIntf, iidStackIpv6Prefix,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&ipv6PrefixObj)) == CMSRET_SUCCESS)
   {
      /*
       * find prefix is different from address because of PD implementation.
       * If it's related to PD, we only match origin and staticType. So prefix argument MUST be NULL in this case.
       */
      if ((!prefix || !cmsUtl_strcmp(ipv6PrefixObj->prefix, prefix)) && 
          (!cmsUtl_strcmp(ipv6PrefixObj->origin, origin) && !cmsUtl_strcmp(ipv6PrefixObj->staticType, staticType)))
      {
         found = TRUE;
      }
      cmsObj_free((void **) &ipv6PrefixObj);
   }

   cmsLog_debug("found<%d>", found);
   return found;
}


CmsRet rutIp_addIpv6Prefix(const InstanceIdStack *iidStackIpIntf, 
                           const char *prefix, const char *origin, 
                           const char *staticType, const char *parent,
                           const char *child, UBOOL8 onLink, UBOOL8 Autonomous,
                           int plt, int vlt, char *myPath, UINT32 pathLen)
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2Ipv6PrefixObject *ipv6PrefixObj=NULL;
   InstanceIdStack iidStack;
   MdmPathDescriptor pathDesc;      
   char timestring[BUFLEN_128];
   char *fullPathStringPtr=NULL;

   iidStack = *iidStackIpIntf;

   if ((ret = cmsObj_addInstance(MDMOID_DEV2_IPV6_PREFIX, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add MDMOID_DEV2_IPV6_PREFIX Instance, ret = %d", ret);
      return ret;
   } 

   if ((ret = cmsObj_get(MDMOID_DEV2_IPV6_PREFIX, &iidStack, 0, (void **) &ipv6PrefixObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get ipv6PrefixObj, ret = %d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_IPV6_PREFIX, &iidStack);
      return ret;
   }

   ipv6PrefixObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->prefix, prefix, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->origin, origin, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->staticType, staticType, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->parentPrefix, parent, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->childPrefixBits, child, mdmLibCtx.allocFlags);
   ipv6PrefixObj->onLink = onLink;
   ipv6PrefixObj->autonomous = Autonomous;
   ipv6PrefixObj->X_BROADCOM_COM_Plt = plt;
   ipv6PrefixObj->X_BROADCOM_COM_Vlt = vlt;
   cmsTms_getXSIDateTime(plt, timestring, sizeof(timestring));
   CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->preferredLifetime, timestring, mdmLibCtx.allocFlags);
   cmsTms_getXSIDateTime(vlt, timestring, sizeof(timestring));
   CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->validLifetime, timestring, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->prefixStatus, MDMVS_PREFERRED, mdmLibCtx.allocFlags);
   cmsLog_debug("prefix/orig/staticType/plt/vlt/A: %s/%s/%s/%d/%d/%d",
                ipv6PrefixObj->prefix, ipv6PrefixObj->origin, ipv6PrefixObj->staticType, 
                ipv6PrefixObj->X_BROADCOM_COM_Plt, ipv6PrefixObj->X_BROADCOM_COM_Vlt,
                ipv6PrefixObj->autonomous);
   
   if ((ret = cmsObj_set(ipv6PrefixObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ipv6PrefixObj. ret=%d", ret);
   }
   else
   {
      if (cmsUtl_strcmp(origin, MDMVS_STATIC) && (ret = cmsObj_setNonpersistentInstance(MDMOID_DEV2_IPV6_PREFIX, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set non-persistent prefix. ret=%d", ret);
      }

   }
   cmsObj_free((void **) &ipv6PrefixObj);

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
   pathDesc.oid = MDMOID_DEV2_IPV6_PREFIX;
   pathDesc.iidStack = iidStack;
   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathStringPtr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_pathDescriptorToFullPathNoEndDot returns error. ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_IPV6_PREFIX, &iidStack);
      return ret;
   }
   if (cmsUtl_strlen(fullPathStringPtr)+1  > (SINT32) pathLen)
   {
      cmsLog_error("fullpath %s too long to fit in param", fullPathStringPtr);
   }
   else
   {
      strncpy(myPath, fullPathStringPtr, pathLen);
   }
   
   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);

   return ret;
}


CmsRet rutIp_replaceIpv6Prefix(const InstanceIdStack *iidStackIpv6Prefix, 
                               const char *prefix, const char *origin, 
                               const char *staticType, const char *parent,
                               const char *child, UBOOL8 onLink, 
                               UBOOL8 Autonomous, int plt, int vlt,
                               const char *prefixOld, int vltOld)
{
   Dev2Ipv6PrefixObject *ipv6PrefixObj=NULL;
   char timestring[BUFLEN_128];
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsObj_get(MDMOID_DEV2_IPV6_PREFIX, iidStackIpv6Prefix, 0, (void **) &ipv6PrefixObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get ipv6PrefixObj, ret = %d", ret);
      return ret;
   }

   ipv6PrefixObj->enable = TRUE;

   if (!IS_EMPTY_STRING(prefix))
   {
      CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->prefix, prefix, mdmLibCtx.allocFlags);
      ipv6PrefixObj->X_BROADCOM_COM_Plt = plt;
      ipv6PrefixObj->X_BROADCOM_COM_Vlt = vlt;
      cmsTms_getXSIDateTime(plt, timestring, sizeof(timestring));
      CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->preferredLifetime, timestring, mdmLibCtx.allocFlags);
      cmsTms_getXSIDateTime(vlt, timestring, sizeof(timestring));
      CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->validLifetime, timestring, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->prefixStatus, MDMVS_PREFERRED, mdmLibCtx.allocFlags);
      if (!IS_EMPTY_STRING(prefixOld))
      {
         CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->X_BROADCOM_COM_Prefix_Old, prefixOld, mdmLibCtx.allocFlags);
         ipv6PrefixObj->X_BROADCOM_COM_Vlt_Old = vltOld;
      }
   }
   else
   {
      /* case of disabling a prefix object, should be PD expiring */
      CMSMEM_FREE_BUF_AND_NULL_PTR(ipv6PrefixObj->prefix);
      CMSMEM_FREE_BUF_AND_NULL_PTR(ipv6PrefixObj->X_BROADCOM_COM_Prefix_Old);
      ipv6PrefixObj->X_BROADCOM_COM_Plt = 0;
      ipv6PrefixObj->X_BROADCOM_COM_Vlt = 0;
      ipv6PrefixObj->X_BROADCOM_COM_Vlt_Old = 0;
      CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->prefixStatus, MDMVS_INVALID, mdmLibCtx.allocFlags);
   }

   if (cmsUtl_strcmp(ipv6PrefixObj->origin, MDMVS_STATIC) ||
        cmsUtl_strcmp(ipv6PrefixObj->staticType, MDMVS_PREFIXDELEGATION))
   {
      CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->parentPrefix, parent, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->childPrefixBits, child, mdmLibCtx.allocFlags);
      ipv6PrefixObj->onLink = onLink;
      ipv6PrefixObj->autonomous= Autonomous;
   }

   cmsLog_debug("prefix/orig/staticT/plt/vlt: %s/%s/%s/%d/%d", 
                ipv6PrefixObj->prefix, ipv6PrefixObj->origin, ipv6PrefixObj->staticType, 
                ipv6PrefixObj->X_BROADCOM_COM_Plt, ipv6PrefixObj->X_BROADCOM_COM_Vlt);
   
   if ((ret = cmsObj_set(ipv6PrefixObj, iidStackIpv6Prefix)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ipv6PrefixObj. ret=%d", ret);
   } 
   else
   {
      if (cmsUtl_strcmp(origin, MDMVS_STATIC) && (ret = cmsObj_setNonpersistentInstance(MDMOID_DEV2_IPV6_PREFIX, &iidStackIpv6Prefix)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set non-persistent prefix. ret=%d", ret);
      }

   }
   cmsObj_free((void **) &ipv6PrefixObj);

   return ret;
}


CmsRet rutIp_deleteIpv6Prefix(const InstanceIdStack *iidStackIpIntf, 
                              const char *prefix, const char *origin, 
                              const char *staticType)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack savedIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6PrefixObject *ipv6PrefixObj=NULL;
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("prefix/origin/staticT: %s/%s/%s", prefix, origin, staticType);
   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_PREFIX,
                              iidStackIpIntf, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&ipv6PrefixObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ipv6PrefixObj->prefix, prefix) && 
          (!cmsUtl_strcmp(ipv6PrefixObj->origin, origin) && !cmsUtl_strcmp(ipv6PrefixObj->staticType, staticType)))
      {
         if (!cmsUtl_strcmp(ipv6PrefixObj->origin, MDMVS_STATIC) && 
             (!cmsUtl_strcmp(ipv6PrefixObj->staticType, MDMVS_PREFIXDELEGATION) || !cmsUtl_strcmp(ipv6PrefixObj->staticType, MDMVS_CHILD)))
         {
            CMSMEM_FREE_BUF_AND_NULL_PTR(ipv6PrefixObj->prefix);
            ipv6PrefixObj->X_BROADCOM_COM_Plt = 0;
            ipv6PrefixObj->X_BROADCOM_COM_Vlt = 0;
            CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(ipv6PrefixObj->prefixStatus, MDMVS_INVALID, mdmLibCtx.allocFlags);
         }
         else
         {
            ret = cmsObj_deleteInstance(MDMOID_DEV2_IPV6_PREFIX, &iidStack);
            /* since we did a delete, restore iidStack to last good one */
            iidStack = savedIidStack;
         }
      }
      cmsObj_free((void **) &ipv6PrefixObj);
      /* save current iidStack in case we delete the next one */
      savedIidStack = iidStack;
   }

   return ret;
}


CmsRet rutIp_disableOrDeleteChildPrefix(const char *prefixFullPath, UBOOL8 isDisable)
{
   UBOOL8 foundChild = FALSE;
   InstanceIdStack iidStackChild = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6PrefixObject *prefixObjChild=NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("parent<%s>", prefixFullPath);
   /*
    * FIXME: Assume one parent maps to one child.
    */
   while (!foundChild && (cmsObj_getNextFlags(MDMOID_DEV2_IPV6_PREFIX, &iidStackChild,
              OGF_NO_VALUE_UPDATE, (void **)&prefixObjChild) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(prefixObjChild->parentPrefix, prefixFullPath))
      {
         foundChild = TRUE;
      }
      else
      {
         cmsObj_free((void **)&prefixObjChild);
      }
   }

   if (foundChild)
   {
      cmsLog_debug("found child prefix");

      if (isDisable)
      {
         prefixObjChild->enable = FALSE;
         if ((ret = cmsObj_set(prefixObjChild, &iidStackChild)) != CMSRET_SUCCESS)
         {
            cmsLog_error("fail set prefix_child: ret=%d", ret);
         }
      }
      else
      {
         if ((ret = cmsObj_deleteInstance(MDMOID_DEV2_IPV6_PREFIX, &iidStackChild)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to delete ipv6PrefixObj. ret=%d", ret);
         }
      }

      cmsObj_free((void **)&prefixObjChild);
   }

   return ret;
}


CmsRet rutIp_updateSystemForPrefixChange(const char *prefixFullPath, UBOOL8 isDisable)
{
   Dev2Ipv6AddressObject *ipv6AddrObj = NULL;
   Dev2Dhcpv6ServerPoolObject *serverPoolObj=NULL;
   Dev2RouterAdvertisementInterfaceSettingObject *raIntfObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   SINT32 len = cmsUtl_strlen(prefixFullPath);
   CmsRet ret = CMSRET_SUCCESS;

   if (IS_EMPTY_STRING(prefixFullPath))
   {
      return ret;
   }

   cmsLog_debug("prefixPath<%s>", prefixFullPath);

   while (cmsObj_getNext(MDMOID_DEV2_DHCPV6_SERVER_POOL,
                   &iidStack, (void **) &serverPoolObj) == CMSRET_SUCCESS)
   {
      char *ptr;

      if ((ptr = cmsUtl_strstr(serverPoolObj->IANAPrefixes, prefixFullPath)) != NULL)
      {
         if ((ptr[len] == ',') || (ptr[len] == '\0'))
         {
            cmsLog_debug("found associated dhcp6s");

            if ((ret = cmsObj_set(serverPoolObj, &iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("fail set serverPoolObj: ret=%d", ret);
               cmsObj_free((void **) &serverPoolObj);
               return ret;
            }
         }
      }  

      cmsObj_free((void **) &serverPoolObj);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNext(MDMOID_DEV2_ROUTER_ADVERTISEMENT_INTERFACE_SETTING,
                   &iidStack, (void **) &raIntfObj) == CMSRET_SUCCESS)
   {
      char *ptr;

      if ((ptr = cmsUtl_strstr(raIntfObj->prefixes, prefixFullPath)) != NULL)
      {
         if ((ptr[len] == ',') || (ptr[len] == '\0'))
         {
            cmsLog_debug("found associated radvd");

            if ((ret = cmsObj_set(raIntfObj, &iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("fail set raIntfObj: ret=%d", ret);
               cmsObj_free((void **) &raIntfObj);
               return ret;
            }
         }
      }  

      cmsObj_free((void **) &raIntfObj);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNext(MDMOID_DEV2_IPV6_ADDRESS,
                   &iidStack, (void **) &ipv6AddrObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ipv6AddrObj->prefix, prefixFullPath))
      {
         cmsLog_debug("found associated address");

         /*
          * FIXME: Regarding address update, now only handles delete
          */
         if (!isDisable &&
             ((ret = cmsObj_deleteInstance(MDMOID_DEV2_IPV6_ADDRESS, &iidStack)) != CMSRET_SUCCESS))
         {
            cmsLog_error("fail delete ipv6AddrObj: ret=%d", ret);
            cmsObj_free((void **) &ipv6AddrObj);
            return ret;
         }
      }  

      cmsObj_free((void **) &ipv6AddrObj);
   }

   return ret;
}


#endif  /* SUPPORT_IPV6 */

#endif  /* DMP_DEVICE2_BASELINE_1 */



