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
#ifdef DMP_DEVICE2_DHCPV6CLIENT_1

#ifdef  DMP_DEVICE2_DHCPV6CLIENTSERVERIDENTITY_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_ipv6.h"
#include "rut2_dns.h"

/*!\file rcl2_dhcpv6client.c
 * \brief This file contains device 2 device.DHCPV6 client objects 
 *
 */
CmsRet  rcl_dev2Dhcpv6ClientObject( _Dev2Dhcpv6ClientObject *newObj __attribute__((unused)),
                const _Dev2Dhcpv6ClientObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


#if 0 //FIXME: NOT SUPPORTED
CmsRet  rcl_dev2Dhcpv6ClientServerObject( _Dev2Dhcpv6ClientServerObject *newObj __attribute__((unused)),
                const _Dev2Dhcpv6ClientServerObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet  rcl_dev2Dhcpv6ClientSentOptionObject( _Dev2Dhcpv6ClientSentOptionObject *newObj __attribute__((unused)),
                const _Dev2Dhcpv6ClientSentOptionObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet  rcl_dev2Dhcpv6ClientReceivedOptionObject( _Dev2Dhcpv6ClientReceivedOptionObject *newObj __attribute__((unused)),
                const _Dev2Dhcpv6ClientReceivedOptionObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}
#endif


CmsRet  rcl_dev2Dhcp6cRcvOptionObject( _Dev2Dhcp6cRcvOptionObject *newObj,
                const _Dev2Dhcp6cRcvOptionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj && currObj)
   {
      Dev2Dhcpv6ClientObject *dhcpcObj = NULL;
      MdmPathDescriptor pathDesc;
      InstanceIdStack iidStacktmp;
      char *ptr;

      cmsLog_debug("addr/prefix/dns/aftr: newObj<%s/%s/%s/%s>,  currObj<%s/%s/%s/%s>", 
        newObj->address, newObj->prefix, newObj->DNSServers, newObj->aftr, 
        currObj->address, currObj->prefix, currObj->DNSServers, currObj->aftr);

      if (cmsObj_get(MDMOID_DEV2_DHCPV6_CLIENT, iidStack, 0, (void **) &dhcpcObj) != CMSRET_SUCCESS)
      {
         cmsLog_error("rcvopt cannot get dhcpv6Client");
         return CMSRET_MDM_TREE_ERROR;
      }

      /* get the iid of corresponding ip.interface.i */
      INIT_PATH_DESCRIPTOR(&pathDesc);
      ret = cmsMdm_fullPathToPathDescriptor(dhcpcObj->interface, &pathDesc);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d",
                      dhcpcObj->interface, ret);
         cmsObj_free((void **) &dhcpcObj);
         return ret;
      }

      if (pathDesc.oid != MDMOID_DEV2_IP_INTERFACE)
      {
         cmsLog_error("dhcpv6Client->interface (%s) must point to IP.Interface", dhcpcObj->interface);
         cmsObj_free((void **) &dhcpcObj);
         return CMSRET_INVALID_PARAM_NAME;
      }

      /* address from dhcp6c message contains prefix len. Remove it for ip.interface.i.ipv6address */
      ptr = cmsUtl_strstr(newObj->address, "/");
      if (ptr)
      {
         *ptr = '\0';
      }

      iidStacktmp = pathDesc.iidStack;

      /* update address if changed: FIXME: assume only one address in IANA */
      if (cmsUtl_strcmp(newObj->address, currObj->address))
      {
         UBOOL8 isAdd;

         isAdd = !IS_EMPTY_STRING(newObj->address);

         if (isAdd)
         {
            InstanceIdStack iidStack_ipv6Addr;

            cmsLog_debug("isAdd addr");

            if (!rutIp_findIpv6Addr(&iidStacktmp, currObj->address, MDMVS_DHCPV6, &iidStack_ipv6Addr))
            {
               rutIp_addIpv6Addr(&iidStacktmp, newObj->address, MDMVS_DHCPV6, 
                                 NULL, newObj->addressPlt, newObj->addressVlt);
            }
            else
            {
               rutIp_deleteIpv6Addr(&iidStacktmp, currObj->address, MDMVS_DHCPV6);
               rutIp_addIpv6Addr(&iidStacktmp, newObj->address, MDMVS_DHCPV6, 
                                 NULL, newObj->addressPlt, newObj->addressVlt);
            }
         }
         else
         {
            /* Delete an address */
            rutIp_deleteIpv6Addr(&iidStacktmp, currObj->address, MDMVS_DHCPV6);
         }
      }

      /* update prefix if changed: FIXME: assume only one prefix in IAPD */
      if (cmsUtl_strcmp(newObj->prefix, currObj->prefix))
      {
         UBOOL8 isAdd;
         InstanceIdStack iidStack_ipv6Prefix;

         isAdd = !IS_EMPTY_STRING(newObj->prefix);
         if (!rutIp_findIpv6Prefix(&iidStacktmp, NULL, MDMVS_STATIC,
                              MDMVS_PREFIXDELEGATION, &iidStack_ipv6Prefix))
         {
            cmsLog_debug("cannot find pre-configured ipv6prefix obj for PD");
         }

         if (isAdd)
         {
            cmsLog_debug("isAdd prefix");

            rutIp_replaceIpv6Prefix(&iidStack_ipv6Prefix, newObj->prefix, MDMVS_STATIC,
                                    MDMVS_PREFIXDELEGATION, NULL, NULL, FALSE, FALSE, 
                                    newObj->prefixPlt, newObj->prefixVlt,
                                    newObj->prefixOld, newObj->prefixVltOld);
         }
	     else
         {
            /* reset static PD object */
            rutIp_replaceIpv6Prefix(&iidStack_ipv6Prefix, NULL, MDMVS_STATIC,
                                    MDMVS_PREFIXDELEGATION, NULL, NULL, FALSE, FALSE,
                                    0, 0,
                                    NULL, 0);
         }
      }

      /* update DNSServers if it not empty AND changed */
      if (!IS_EMPTY_STRING(newObj->DNSServers) &&
         cmsUtl_strcmp(newObj->DNSServers, currObj->DNSServers))
      {
         /* Need to get the ip interface name for adding dns client server object */
         cmsLog_debug("dhcpcObj->interface %s, newObj->DNSServers %s", dhcpcObj->interface, newObj->DNSServers);
         if ((ret = rutDns_addServerObject_dev2(dhcpcObj->interface,
                                                       newObj->DNSServers, 
                                                       MDMVS_DHCPV6)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutDns_addServerObject_dev2 failed. ret %d", ret);
         }      
      }

      /* update aftr if changed */
      if (!IS_EMPTY_STRING(newObj->aftr) &&
          cmsUtl_strcmp(newObj->aftr, currObj->aftr))
      {
         rutDhcpv6_updateAftr(dhcpcObj->interface, newObj->aftr);
      }

      /* update MAPT/MAPE if changed */
      if (!IS_EMPTY_STRING(newObj->mapBRPrefix) &&
          cmsUtl_strcmp(newObj->mapBRPrefix, currObj->mapBRPrefix))
      {
         rutDhcpv6_updateMap(dhcpcObj->interface, newObj->mapTransportMode, newObj->mapBRPrefix, 
                 newObj->mapRuleIPv4Prefix, newObj->mapRuleIPv6Prefix, newObj->mapEALen,
                 newObj->mapPSIDOffset, newObj->mapPSIDLen, newObj->mapPSID,
                 newObj->mapIsFMR);
      }

      cmsObj_free((void **) &dhcpcObj);
   }

   return ret;
}

#endif  /*DMP_DEVICE2_DHCPV6CLIENTSERVERIDENTITY_1 */

#endif  /* DMP_DEVICE2_DHCPV6CLIENT_1 */

#endif /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */


#endif  /* DMP_DEVICE2_BASELINE_1 */
