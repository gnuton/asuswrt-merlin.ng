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
#include "rut_util.h"
#include "rut_wan.h"
#include "rut2_dhcpv6.h"

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
static CmsRet getIpv6DhcpcObjectByFullPath(const char *ipIntfFullPath,
                                           InstanceIdStack *dhcpcIidStack,
                                           Dev2Dhcpv6ClientObject **dhcpcObject)
{
   UBOOL8 found=FALSE;
   Dev2Dhcpv6ClientObject *dhcp6ClientObj=NULL;
   CmsRet ret=CMSRET_INVALID_ARGUMENTS;
   
   if (ipIntfFullPath == NULL)
   {
      cmsLog_error("NULL string.");
      return ret;
   }
   cmsLog_debug("Enter ipIntfFullPath %s.", ipIntfFullPath);
   
   while(!found && 
         (ret = cmsObj_getNextFlags(MDMOID_DEV2_DHCPV6_CLIENT,
                             dhcpcIidStack, 
                             OGF_NO_VALUE_UPDATE,
                             (void **)&dhcp6ClientObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(dhcp6ClientObj->interface, ipIntfFullPath))
      {
         found = TRUE;
         *dhcpcObject = dhcp6ClientObj;
      }
      else
      {
         cmsObj_free((void **)&dhcp6ClientObj);
      }            
   }     

   cmsLog_debug("found %d", found);

   return ret;
}


UBOOL8 rutDhcpv6_isClientEnabled_dev2(const char *ipIntfFullPath, UBOOL8 *iana, UBOOL8 *iapd,
                                        UBOOL8 *um, SINT32 *pid)
{
   
   UBOOL8 isEnabled=FALSE;
   Dev2Dhcpv6ClientObject *dhcpcObject=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   
   if (getIpv6DhcpcObjectByFullPath(ipIntfFullPath,
                                    &iidStack, 
                                    &dhcpcObject) != CMSRET_SUCCESS)
   {
      cmsLog_debug("no Dhcpv6ClientObj for %s", ipIntfFullPath);
      return FALSE;
   }
   
   isEnabled = dhcpcObject->enable;
   *iana = dhcpcObject->requestAddresses;
   *iapd = dhcpcObject->requestPrefixes;
   *um = dhcpcObject->X_BROADCOM_COM_UnnumberedModel;
   *pid = dhcpcObject->X_BROADCOM_COM_Pid;
   cmsObj_free((void **) &dhcpcObject);

   cmsLog_debug("enabled=%d, iana=%d, iapd=%d, pid=%d", isEnabled, *iana, *iapd, *pid);

   return isEnabled;
}


void rutDhcpv6_setClientPidAndStatusByIpIntfFullPath_dev2(const char *ipIntfFullPath,
                                                    const SINT32 pid,
                                                    const char *status)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2Dhcpv6ClientObject *dhcp6ClientObj=NULL;
   CmsRet ret;


   if (getIpv6DhcpcObjectByFullPath(ipIntfFullPath,
                                    &iidStack, 
                                    &dhcp6ClientObj) != CMSRET_SUCCESS)   {
      cmsLog_error("Fail to find the dhcp client info for %s", ipIntfFullPath);
      return;
   }

   dhcp6ClientObj->X_BROADCOM_COM_Pid = pid;
   CMSMEM_REPLACE_STRING_FLAGS(dhcp6ClientObj->status, status, mdmLibCtx.allocFlags);

   if ((ret = cmsObj_set(dhcp6ClientObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_set dhcp6ClientObj returns error. ret=%d", ret);
   }     
   
   cmsObj_free((void **) &dhcp6ClientObj);

   return;
}


void rutDhcpv6_updateIANAPrefixes(const char *ifpath, char **prefixes, char *prefixValue)
{
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6PrefixObject *ipv6Prefix = NULL;
   CmsRet ret;

   if (!ifpath)
   {
      cmsLog_error("null ifpath");
      return;
   }

   cmsLog_debug("ifpath=%s", ifpath);

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
         
   if ((ret = cmsMdm_fullPathToPathDescriptor(ifpath, &pathDesc)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert %s to pathDesc, ret=%d", ifpath, ret);
      return;
   }         

   CMSMEM_FREE_BUF_AND_NULL_PTR(*prefixes);
   *prefixes = (char *)cmsMem_alloc(CMS_DEV2_RA_PREFIX_LEN, ALLOC_SHARED_MEM);

   /*
    * 1. Clear the prefixes
    * 2. Get all delegated prefixes of the IP.Interface.i.IPv6Prefix.
    * FIXME: how about IANA for ULA?
    */
   memset(*prefixes, 0, CMS_DEV2_RA_PREFIX_LEN);
   prefixValue[0] = '\0';

   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_PREFIX, &(pathDesc.iidStack), 
           &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipv6Prefix) == CMSRET_SUCCESS)
   {
      UBOOL8 isPD;

      isPD = ((cmsUtl_strcmp(ipv6Prefix->origin, MDMVS_CHILD) == 0) ||
              ((cmsUtl_strcmp(ipv6Prefix->origin, MDMVS_STATIC) == 0) &&
               (cmsUtl_strcmp(ipv6Prefix->staticType, MDMVS_CHILD) == 0)));

      /* IANAPrefix is only from PD. Maybe ULA prefix in the future? */
      if (cmsUtl_strcmp(ipv6Prefix->status, MDMVS_DISABLED) && isPD)
      {
         MdmPathDescriptor pathDesc_ipv6prefix;
         char *ipv6prefixFullPath=NULL;

         INIT_PATH_DESCRIPTOR(&pathDesc_ipv6prefix);
         pathDesc_ipv6prefix.oid = MDMOID_DEV2_IPV6_PREFIX;
         pathDesc_ipv6prefix.iidStack = iidStack;
         if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc_ipv6prefix, &ipv6prefixFullPath)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_pathDescriptorToFullPathNoEndDot returns error. ret=%d", ret);
            cmsObj_free((void **)&ipv6Prefix);
            return;
         }

         if (IS_EMPTY_STRING(prefixValue) && cmsUtl_isValidIpAddress(AF_INET6, ipv6Prefix->prefix))
         {
            cmsUtl_strcpy(prefixValue, ipv6Prefix->prefix);
         }

         if (cmsUtl_strlen(*prefixes) > 0)
         {
            strcat(*prefixes, ",");
         }

         strcat(*prefixes, ipv6prefixFullPath);

         CMSMEM_FREE_BUF_AND_NULL_PTR(ipv6prefixFullPath);
      }

      cmsObj_free((void **)&ipv6Prefix);
   }

   cmsLog_debug("prefixes<%s> prefix<%s>", *prefixes, prefixValue);
   return;
}


void rutDhcpv6_updateIANAPrefixesByULA(const char *ifpath, char **prefixes, char *prefixValue)
{
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6PrefixObject *ipv6Prefix = NULL;
   CmsRet ret;

   if (!ifpath)
   {
      cmsLog_error("null ifpath");
      return;
   }

   cmsLog_debug("ifpath=%s", ifpath);

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
         
   if ((ret = cmsMdm_fullPathToPathDescriptor(ifpath, &pathDesc)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert %s to pathDesc, ret=%d", ifpath, ret);
      return;
   }         

   CMSMEM_FREE_BUF_AND_NULL_PTR(*prefixes);
   *prefixes = (char *)cmsMem_alloc(CMS_DEV2_RA_PREFIX_LEN, ALLOC_SHARED_MEM);

   /*
    * 1. Clear the prefixes
    * FIXME: Integrate this function with rutDhcpv6_updateIANAPrefixes
    */
   memset(*prefixes, 0, CMS_DEV2_RA_PREFIX_LEN);
   prefixValue[0] = '\0';

   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_PREFIX, &(pathDesc.iidStack), 
           &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipv6Prefix) == CMSRET_SUCCESS)
   {
      UBOOL8 isULA;

      isULA = (cmsUtl_strcmp(ipv6Prefix->origin, MDMVS_STATIC) == 0) &&
              (cmsUtl_strcmp(ipv6Prefix->staticType, MDMVS_STATIC) == 0);

      /* IANAPrefix is only from PD. Maybe ULA prefix in the future? */
      if (cmsUtl_strcmp(ipv6Prefix->status, MDMVS_DISABLED) && isULA)
      {
         MdmPathDescriptor pathDesc_ipv6prefix;
         char *ipv6prefixFullPath=NULL;

         INIT_PATH_DESCRIPTOR(&pathDesc_ipv6prefix);
         pathDesc_ipv6prefix.oid = MDMOID_DEV2_IPV6_PREFIX;
         pathDesc_ipv6prefix.iidStack = iidStack;
         if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc_ipv6prefix, &ipv6prefixFullPath)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_pathDescriptorToFullPathNoEndDot returns error. ret=%d", ret);
            cmsObj_free((void **)&ipv6Prefix);
            return;
         }

         if (IS_EMPTY_STRING(prefixValue) && cmsUtl_isValidIpAddress(AF_INET6, ipv6Prefix->prefix))
         {
            cmsUtl_strcpy(prefixValue, ipv6Prefix->prefix);
         }

         if (cmsUtl_strlen(*prefixes) > 0)
         {
            strcat(*prefixes, ",");
         }

         strcat(*prefixes, ipv6prefixFullPath);

         CMSMEM_FREE_BUF_AND_NULL_PTR(ipv6prefixFullPath);
      }

      cmsObj_free((void **)&ipv6Prefix);
   }

   cmsLog_debug("prefixes<%s> prefix<%s>", *prefixes, prefixValue);
   return;
}


void rutDhcpv6_updateAftr(const char *ifpath, const char *aftr)
{
   /* if associated dslite obj is dynamic tunnel, update aftr */
   Dev2DsliteInterfaceSettingObject *obj;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;

   cmsLog_debug("ifpath<%s> aftr<%s>", ifpath, aftr);

   while (!found && (cmsObj_getNextFlags(MDMOID_DEV2_DSLITE_INTERFACE_SETTING, &iidStack,
                 OGF_NO_VALUE_UPDATE, (void **)&obj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(obj->X_BROADCOM_COM_TunneledInterface, ifpath))
      {
         found = TRUE;

         if (!cmsUtl_strcmp(obj->origin, MDMVS_DHCPV6))
         {
            cmsLog_debug("update aftr");

            CMSMEM_REPLACE_STRING_FLAGS(obj->endpointName, aftr, mdmLibCtx.allocFlags);
            if (cmsObj_set(obj, &iidStack) != CMSRET_SUCCESS)
            {
               cmsLog_error("set dslite obj fail");
            }
         }
      }

      cmsObj_free((void **)&obj);
   }

   return;
}

void rutDhcpv6_updateMap(const char *ifpath,  const char *mechanism, const char *brprefix, const char *ipv4prefix,
        const char *ipv6prefix, UINT32 ealen, UINT32 psidoffset, UINT32 psidlen,
        UINT32 psid, UBOOL8 isFMR)
{
#ifdef SUPPORT_MAP
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2MapDomainObject *domain = NULL;
   UBOOL8 found = FALSE;
   
   while (!found && (cmsObj_getNextFlags(MDMOID_DEV2_MAP_DOMAIN, &iidStack,
                 OGF_NO_VALUE_UPDATE, (void **)&domain) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(domain->WANInterface, ifpath))
      {
         found = TRUE;

         CMSMEM_REPLACE_STRING_FLAGS(domain->transportMode, mechanism, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(domain->BRIPv6Prefix, brprefix, mdmLibCtx.allocFlags);
         domain->PSIDOffset = psidoffset;
         domain->PSIDLength = psidlen;
         domain->PSID = psid;

         if (cmsObj_set(domain, &iidStack) != CMSRET_SUCCESS)
         {
            cmsLog_error("set map%c domain fail", (*mechanism + 32));
         }
         else
         {
            Dev2MapDomainRuleObject *rule = NULL;
            InstanceIdStack iidStack_rule = EMPTY_INSTANCE_ID_STACK;

            if (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_MAP_DOMAIN_RULE,
                           &iidStack, &iidStack_rule,
                           OGF_NO_VALUE_UPDATE,
                           (void **)&rule) == CMSRET_SUCCESS)
            {
               CMSMEM_REPLACE_STRING_FLAGS(rule->IPv4Prefix, ipv4prefix, mdmLibCtx.allocFlags);
               CMSMEM_REPLACE_STRING_FLAGS(rule->IPv6Prefix, ipv6prefix, mdmLibCtx.allocFlags);
               rule->EABitsLength = ealen;
               rule->isFMR = isFMR;

               if ( cmsObj_set(rule, &iidStack_rule) != CMSRET_SUCCESS )
               {
                  cmsLog_error("Failed to set MDMOID_DEV2_MAP_DOMAIN_RULE object");
               }

               cmsObj_free((void **)&rule);
            }
            else
            {
               cmsLog_notice("No rule obj associated with domain!");
            }
         }
      }

      cmsObj_free((void **)&domain);
   }
#endif

   return;
}

#endif /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */


#endif /* DMP_DEVICE2_BASELINE_1 */

