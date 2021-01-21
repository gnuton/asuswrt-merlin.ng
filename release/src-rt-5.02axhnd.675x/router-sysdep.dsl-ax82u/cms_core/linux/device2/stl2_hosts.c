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

#ifdef DMP_DEVICE2_HOSTS_2

/** all the TR181 hosts objects will go into this file */

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"

CmsRet stl_dev2HostsObject(_Dev2HostsObject *obj __attribute((unused)),
                           const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2HostObject(_Dev2HostObject *obj __attribute((unused)),
                          const InstanceIdStack *iidStack __attribute((unused)))
{
   char ifName[CMS_IFNAME_LENGTH] = {0};
   char buf[sizeof(CmsMsgHeader) + sizeof(GetLeaseTimeRemainingMsgBody)] = {0};
   CmsMsgHeader *hdr = (CmsMsgHeader *) buf;
   GetLeaseTimeRemainingMsgBody *body = (GetLeaseTimeRemainingMsgBody *) (hdr + 1);
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   /* or alternatively, set bounceiIfNotRunningFlag in the CMS message header */
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   if (cmsUtl_isValidIpAddress(AF_INET6, obj->IPAddress))
   {
      cmsLog_debug("No query of time remaining for IPv6Addr=%s macAddr=%s.  Unsupported", obj->IPAddress, obj->physAddress);
      obj->leaseTimeRemaining = 0;
      return CMSRET_SUCCESS;
   }
   else
#endif
   if (rutLan_isDhcpdEnabled() == FALSE)
   {
      cmsLog_debug("No query of time remaining for IPAddr=%s macAddr=%s.  DHCP disabled", obj->IPAddress, obj->physAddress);
      obj->leaseTimeRemaining = 0;
      return CMSRET_SUCCESS;
   }

   cmsLog_debug("querying time remaining for IPAddr=%s macAddr=%s", obj->IPAddress, obj->physAddress);

   if (obj->IPAddress == NULL || obj->physAddress == NULL)
   {
      /* we are being read immediately after the obj create, so no IP&MAC address yet */
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   if ((ret = qdmIntf_fullPathToIntfnameLocked_dev2(obj->layer3Interface, ifName)) != CMSRET_SUCCESS)
   {
      /* we are being read immediately after the obj create, so no IP&MAC address yet */
      cmsLog_error("Could not find interface name associated with layer3Interface=%s, ret=%d",
                   obj->layer3Interface, ret);
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   hdr->type = CMS_MSG_GET_LEASE_TIME_REMAINING;
   hdr->src = mdmLibCtx.eid;
   hdr->dst = EID_DHCPD;
   hdr->flags_request = 1;
   hdr->dataLength = sizeof(GetLeaseTimeRemainingMsgBody);

   snprintf(body->ifName, sizeof(body->ifName), "%s", ifName);
   snprintf(body->macAddr, sizeof(body->macAddr), "%s", obj->physAddress);

#if defined(DESKTOP_LINUX) && !defined(BUILD_DESKTOP_BEEP) 
   /* dhcpd is not supported on the desktop yet, so can't send a request and expect a reply */
   obj->leaseTimeRemaining = 80123;
#else
   obj->leaseTimeRemaining = cmsMsg_sendAndGetReplyWithTimeout(mdmLibCtx.msgHandle, hdr, CMSLCK_MAX_HOLDTIME);
#endif

   return CMSRET_SUCCESS;
}

CmsRet stl_dev2HostIpv4AddressObject(_Dev2HostIpv4AddressObject *obj __attribute((unused)),
                                     const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2HostIpv6AddressObject(_Dev2HostIpv6AddressObject *obj __attribute((unused)),
                                     const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


#endif  /* DMP_DEVICE2_HOSTS_2 */

#endif /* DMP_DEVICE2_BASELINE_1 */

