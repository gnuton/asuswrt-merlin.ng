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

#ifdef DMP_DEVICE2_DHCPV6SERVER_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "qdm_ipintf.h"
#include "qdm_intf.h"
#include "qdm_ipv6.h"
#include "rut2_dhcpv6.h"

/*!\file rcl2_dhcpv6server.c
 * \brief This file contains device 2 device.DHCPV6 server objects 
 *
 */

CmsRet rcl_dev2Dhcpv6ServerObject( _Dev2Dhcpv6ServerObject *newObj __attribute__((unused)),
                const _Dev2Dhcpv6ServerObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}



CmsRet rcl_dev2Dhcpv6ServerPoolObject( _Dev2Dhcpv6ServerPoolObject *newObj __attribute__((unused)),
                const _Dev2Dhcpv6ServerPoolObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj != NULL)
   {
      char ifName[CMS_IFNAME_LENGTH];

      if (currObj)
      {
         if (!newObj->enable && currObj->enable)
         {
            rut_stopDhcp6s();
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                        mdmLibCtx.allocFlags);
         }
      }

      if (newObj->enable)
      {
         qdmIntf_fullPathToIntfnameLocked_dev2(newObj->interface, ifName);

         if (qdmIpIntf_isAssociatedWanInterfaceUpLocked_dev2(newObj->interface, CMS_AF_SELECT_IPV6))
         {
            char prefix[CMS_IPADDR_LENGTH];
            char ipAddr[CMS_IPADDR_LENGTH];
            /*
             * 1. get br0 address
             * 2. get delegated prefix for IANA
             * 3. check whether WAN is up or down
             * FIXME: domainName is not supported now
             */
            qdmIpIntf_getDproxyIpv6AddressByNameLocked_dev2(ifName, ipAddr);

            rutDhcpv6_updateIANAPrefixes(newObj->interface, &(newObj->IANAPrefixes), prefix);
            if ((ret = rut_restartDhcp6s(ipAddr, NULL, newObj->IANAEnable,
                                         newObj->X_BROADCOM_COM_MinInterfaceID,
                                         newObj->X_BROADCOM_COM_MaxInterfaceID,
                                         newObj->X_BROADCOM_COM_IANALeaseTime,
                                         prefix)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rut_restartDhcp6s returns error. ret=%d", ret);
            }

            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,
                                        mdmLibCtx.allocFlags);
         }
         else
         {
            char prefix[CMS_IPADDR_LENGTH];

            rutDhcpv6_updateIANAPrefixesByULA(newObj->interface, &(newObj->IANAPrefixes), prefix);

            if (!IS_EMPTY_STRING(prefix))
            {
               if ((ret = rut_restartDhcp6s(NULL, NULL, newObj->IANAEnable,
                                            newObj->X_BROADCOM_COM_MinInterfaceID,
                                            newObj->X_BROADCOM_COM_MaxInterfaceID,
                                            newObj->X_BROADCOM_COM_IANALeaseTime,
                                            prefix)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("rut_restartDhcp6s returns error. ret=%d", ret);
               }

               CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,
                                           mdmLibCtx.allocFlags);
            }
            else
            {
               rut_stopDhcp6s();
               CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                           mdmLibCtx.allocFlags);
            }
         }
      }
   }

   return ret;
}


CmsRet rcl_dev2Dhcpv6ServerPoolClientObject( _Dev2Dhcpv6ServerPoolClientObject *newObj __attribute__((unused)),
                const _Dev2Dhcpv6ServerPoolClientObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2Dhcpv6ServerPoolClientIpv6AddressObject( _Dev2Dhcpv6ServerPoolClientIpv6AddressObject *newObj __attribute__((unused)),
                const _Dev2Dhcpv6ServerPoolClientIpv6AddressObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Dhcpv6ServerPoolClientIpv6PrefixObject( _Dev2Dhcpv6ServerPoolClientIpv6PrefixObject *newObj __attribute__((unused)),
                const _Dev2Dhcpv6ServerPoolClientIpv6PrefixObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Dhcpv6ServerPoolClientOptionObject( _Dev2Dhcpv6ServerPoolClientOptionObject *newObj __attribute__((unused)),
                const _Dev2Dhcpv6ServerPoolClientOptionObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Dhcpv6ServerPoolOptionObject( _Dev2Dhcpv6ServerPoolOptionObject *newObj __attribute__((unused)),
                const _Dev2Dhcpv6ServerPoolOptionObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


#endif  /* DMP_DEVICE2_DHCPV6SERVER_1 */

#endif /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */

