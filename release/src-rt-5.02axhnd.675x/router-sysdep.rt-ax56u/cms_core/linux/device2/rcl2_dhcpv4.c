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

#ifdef DMP_DEVICE2_DHCPV4_1

/** all the TR181 DHCPv4 server and client objects will go into this file,
 *  or split client and server into two files?
 */

#include "cms_core.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_lan.h"
#include "rut_wan.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut2_ip.h"
#include "rut2_dns.h"
#include "rut2_route.h"
#include "rut2_dhcpv4.h"



#ifdef DMP_DEVICE2_DHCPV4RELAY_1
static void modifyDhcpv4ForwardingNumEntry(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DHCPV4_RELAY,
                        MDMOID_DEV2_DHCPV4_RELAY_FORWARDING,
                        iidStack,
                        delta);
}
#endif




CmsRet rcl_dev2Dhcpv4Object( _Dev2Dhcpv4Object *newObj __attribute((unused)),
                const _Dev2Dhcpv4Object *currObj __attribute((unused)),
                const InstanceIdStack *iidStack __attribute((unused)),
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Dhcpv4ClientObject( _Dev2Dhcpv4ClientObject *newObj,
                const _Dev2Dhcpv4ClientObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   /*
    * Update forwardNumberOfEntries on successful add or delete.
    */
   if (ADD_NEW(newObj, currObj))
   {
      /* no action on startup.  just return. */
      rutUtil_modifyNumDhcpv4Client(iidStack, 1);
      cmsLog_debug("start up or add - No action.");
      return ret;
   }


   if (newObj && currObj)
   {
      cmsLog_debug("DHCPStatus %s => %s", currObj->DHCPStatus, newObj->DHCPStatus);
      cmsLog_debug("ipAddr %s => %s", currObj->IPAddress, newObj->IPAddress);
      cmsLog_debug("mask   %s => %s", currObj->subnetMask, newObj->subnetMask);
      cmsLog_debug("dnsServers %s => %s", currObj->subnetMask, newObj->subnetMask);
      cmsLog_debug("IPRouters %s => %s", currObj->IPRouters, newObj->IPRouters);

      if (newObj->sentOptionNumberOfEntries != currObj->sentOptionNumberOfEntries)
      {
         cmsLog_debug("Number of options is increased. Do nothing.");
         return ret;
      }

      /*
       * For LAN side dhcp client, we will add the dhcp client object after
       * the br0 is already in the UP state.  So we need to start the dhcp
       * client here instead of in the intfstack/IPv4 Service state machine.
       */
      if (newObj->enable && !currObj->enable &&
          !IS_EMPTY_STRING(newObj->interface))
      {
         char serviceStatusBuf[BUFLEN_32]={0};

         qdmIpIntf_getIpvxServiceStatusFromFullPathLocked_dev2(
                                                  newObj->interface,
                                                  CMS_AF_SELECT_IPV4,
                                                  serviceStatusBuf,
                                                  sizeof(serviceStatusBuf));
         if (!strcmp(serviceStatusBuf, MDMVS_SERVICESTARTING) ||
             !strcmp(serviceStatusBuf, MDMVS_SERVICEUP))
         {
            char intfNameBuf[CMS_IFNAME_LENGTH]={0};
            char vid[BUFLEN_256], uid[BUFLEN_256];
            char iaid[BUFLEN_16], duid[BUFLEN_256];
            char ipAddr[BUFLEN_16], leasedTime[BUFLEN_16];
            char buf[BUFLEN_16], serverIpAddr[BUFLEN_16];
#ifdef SUPPORT_HOMEPLUG
            UBOOL8 op125=TRUE;
#else
            UBOOL8 op125=FALSE; 
#endif
            
            UBOOL8 op212=FALSE;

            qdmIntf_fullPathToIntfnameLocked(newObj->interface, intfNameBuf);

            memset(vid, 0, BUFLEN_256);
            memset(duid, 0, BUFLEN_256);
            memset(iaid, 0, BUFLEN_16);
            memset(uid, 0, BUFLEN_256);
            memset(buf, 0, BUFLEN_16);
            memset(ipAddr, 0, BUFLEN_16);
            memset(leasedTime, 0, BUFLEN_16);
            memset(serverIpAddr, 0, BUFLEN_16);

            qdmDhcpv4Client_getSentOption_dev2(newObj->interface, 60, vid, BUFLEN_256);
            qdmDhcpv4Client_getSentOption_dev2(newObj->interface, 61, duid, BUFLEN_256);
            qdmDhcpv4Client_getSentOption_dev2(newObj->interface, 61, iaid, BUFLEN_8);
            qdmDhcpv4Client_getSentOption_dev2(newObj->interface, 77, uid, BUFLEN_256);
#ifndef SUPPORT_HOMEPLUG
            if (qdmDhcpv4Client_getSentOption_dev2(newObj->interface, 125, buf, BUFLEN_16) == CMSRET_SUCCESS)
               op125 = atoi(buf);
#endif

            qdmDhcpv4Client_getReqOption_dev2(newObj->interface, 50, ipAddr, BUFLEN_16);
            qdmDhcpv4Client_getReqOption_dev2(newObj->interface, 51, leasedTime, BUFLEN_16);
            qdmDhcpv4Client_getReqOption_dev2(newObj->interface, 54, serverIpAddr, BUFLEN_16);

            cmsLog_debug("start of dhcp client on %s when Ipv4ServiceStatus=%s",
                         intfNameBuf, serviceStatusBuf);

            newObj->X_BROADCOM_COM_Pid = rutWan_startDhcpc(intfNameBuf,
                                                           vid,
                                                           duid,
                                                           iaid,
                                                           uid,
                                                           op125,
                                                           ipAddr,
                                                           serverIpAddr,
                                                           leasedTime,
                                                           op212);
            if (newObj->X_BROADCOM_COM_Pid == CMS_INVALID_PID)
            {
               CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ERROR, mdmLibCtx.allocFlags);
            }
            else
            {
               CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
            }
         }
      }
      /*
       * When DHCPStatus transitions to BOUND state, create the IPv4Address,
       * DNS servers, and routing objects.
       * The state machine in rut_ipservicecfg.c is responsible for deleting
       * these objects.
       */
      else if (cmsUtl_strcmp(newObj->DHCPStatus, currObj->DHCPStatus) &&
               !cmsUtl_strcmp(newObj->DHCPStatus, MDMVS_BOUND))

      {          
         CmsRet r2;
         if ((r2 = rutIp_addIpv4AddressObject_dev2(newObj->interface,
                                                   newObj->IPAddress,
                                                   newObj->subnetMask,
                                                   MDMVS_DHCP)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutIp_addIpv4AddressObject_dev2 failed. ret %d", r2);
            /* complain, but keep going.  There is no point in failing this
             * set operation.  We don't recover or rollback anything.
             */
         }

         if ((r2 = rutDns_addServerObject_dev2(newObj->interface,
                                               newObj->DNSServers,
                                               MDMVS_DHCPV4)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutDns_addServerObject_dev2 failed. ret %d", r2);
            /* complain, but keep going.  There is no point in failing this
             * set operation.  We don't recover or rollback anything.
             */
         }

         if ((r2 = rutRt_addIpv4ForwardingObject_dev2(newObj->interface,
                                                      newObj->IPRouters,
                                                      MDMVS_DHCPV4)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutRt_addIpv4ForwardingObject_dev2 failed. ret %d", r2);
            /* complain, but keep going.  There is no point in failing this
             * set operation.  We don't recover or rollback anything.
             */
         }             
      }
   }


   if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /* regardless of delete or disable, stop the dhcp client */
      if (currObj->X_BROADCOM_COM_Pid != CMS_INVALID_PID)
      {
         rutDhcpv4_stopClientByPid_dev2(currObj->X_BROADCOM_COM_Pid);
      }

      if (DELETE_EXISTING(newObj, currObj))
      {
         rutUtil_modifyNumDhcpv4Client(iidStack, -1);
      }
      else
      {
         /* if not a delete (just disable existing), then update status */
         newObj->X_BROADCOM_COM_Pid = CMS_INVALID_PID;
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->DHCPStatus, MDMVS_INIT,
                                     mdmLibCtx.allocFlags);
      }
   }

   return ret;
}


CmsRet rcl_dev2Dhcpv4ClientSentOptionObject( _Dev2Dhcpv4ClientSentOptionObject *newObj,
                const _Dev2Dhcpv4ClientSentOptionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   /*
    * Update sentOptionNumberOfEntries on successful add or delete.
    */
   if (ADD_NEW(newObj, currObj))
   {
      /* no action on startup.  just return. */
      rutUtil_modifyNumDhcpv4ClientSentOption(iidStack, 1);
      cmsLog_debug("start up or add - No action.");
      return ret;
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumDhcpv4ClientSentOption(iidStack, -1);
   }

   return ret;
}


CmsRet rcl_dev2Dhcpv4ClientReqOptionObject( _Dev2Dhcpv4ClientReqOptionObject *newObj,
                const _Dev2Dhcpv4ClientReqOptionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   /*
    * Update reqOptionNumberOfEntries on successful add or delete.
    */
   if (ADD_NEW(newObj, currObj))
   {
      /* no action on startup.  just return. */
      rutUtil_modifyNumDhcpv4ClientReqOption(iidStack, 1);
      cmsLog_debug("start up or add - No action.");
      return ret;
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumDhcpv4ClientReqOption(iidStack, -1);
   }

   return ret;
}


CmsRet rcl_dev2Dhcpv4ServerObject( _Dev2Dhcpv4ServerObject *newObj __attribute((unused)),
                const _Dev2Dhcpv4ServerObject *currObj __attribute((unused)),
                const InstanceIdStack *iidStack __attribute((unused)),
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   /* XXX TODO: detect changes in enable param */

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2Dhcpv4ServerPoolObject( _Dev2Dhcpv4ServerPoolObject *newObj,
                const _Dev2Dhcpv4ServerPoolObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumDhcpv4ServerPool(iidStack, 1);
   }


   /* activate or change dhcp server */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("ENABLE_NEW_OR_ENABLE_EXISTING");
      ret = rutLan_updateDhcpd();
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj) && 
      rutLan_isDhcpv4ServerPoolChanged_dev2(newObj, currObj))
   {
      cmsLog_debug("POTENTIAL_CHANGE_OF_EXISTING");
      ret = rutLan_updateDhcpd();
      rutLan_reconfigNatForAddressChange_dev2(currObj->IPRouters, currObj->subnetMask, newObj->IPRouters, newObj->subnetMask);
   }         
   if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("DELETE_OR_DISABLE_EXISTING");
      ret = rutLan_updateDhcpd();

      if (DELETE_EXISTING(newObj, currObj))
      {
         rutUtil_modifyNumDhcpv4ServerPool(iidStack, -1);
      }
   }

   return ret;
}


CmsRet rcl_dev2Dhcpv4ServerPoolStaticAddressObject( _Dev2Dhcpv4ServerPoolStaticAddressObject *newObj,
                const _Dev2Dhcpv4ServerPoolStaticAddressObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 found = FALSE;
   Dev2Dhcpv4ServerPoolStaticAddressObject *dhcpv4StaticAddressObj = NULL;
   InstanceIdStack searchIidStack = EMPTY_INSTANCE_ID_STACK;

   if (ADD_NEW(newObj, currObj))
   {
      cmsLog_debug("ADD_NEW");

      rutUtil_modifyNumDhcpv4ServerPoolStaticAddress(iidStack, 1);
   }
   else if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj) ||
            rutLan_isDhcpv4ServerPoolStaticAddressChanged_dev2(newObj, currObj))
   {
      cmsLog_debug("ENABLE_NEW_OR_ENABLE_EXISTING");

      /* TODO: handle PPPIpExtension not implemented yet */

      if (!cmsUtl_isValidIpAddress(AF_INET, newObj->yiaddr) || !cmsUtl_strcmp(newObj->yiaddr, "0.0.0.0"))
      {
         cmsLog_error("Invalid yiaddr IP address");
         return CMSRET_INVALID_ARGUMENTS;		
      }

      if (cmsUtl_isValidMacAddress(newObj->chaddr) == FALSE)
      {
         cmsLog_error("Invalid chaddr MAC address");
         return CMSRET_INVALID_ARGUMENTS;		
      }

      /* loop through all static IP entries to check for duplicate entry */
      while (!found &&
             cmsObj_getNext(MDMOID_DEV2_DHCPV4_SERVER_POOL_STATIC_ADDRESS,
                            &searchIidStack,
                            (void **) &dhcpv4StaticAddressObj) == CMSRET_SUCCESS)
      {
         /*
          * When we iterate through the objects using cmsObj_getNext, we will
          * also get a copy of the new object that is being set.  Only do the
          * duplicate check if the object is not the same as the new one being set.
          */
         if (cmsMdm_compareIidStacks(&searchIidStack, iidStack))
         {
            if ((dhcpv4StaticAddressObj->enable) && 
                (!cmsUtl_strcmp(newObj->chaddr, dhcpv4StaticAddressObj->chaddr) ||
                 !cmsUtl_strcmp(newObj->yiaddr, dhcpv4StaticAddressObj->yiaddr)))
            {
               found = TRUE;
            }

         }

         cmsObj_free((void **) &dhcpv4StaticAddressObj);
      }

      if (found)
      {
         cmsLog_error("Either MAC or IP address already has been used for static IP lease.");
         return CMSRET_INVALID_ARGUMENTS;
      }

      rutLan_updateDhcpd();
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("DELETE_OR_DISABLE_EXISTING");

      if (DELETE_EXISTING(newObj, currObj))
      {
         rutUtil_modifyNumDhcpv4ServerPoolStaticAddress(iidStack, -1);
      }

      /*
       * The set on the object from dal_lan could have failed.
       * In that case, chaddr would be NULL, so don't bother
       * deleting it from the udhcpd.conf, since it was not put there
       * in the first place.
       */
      if (currObj->chaddr != NULL)
      {
         rutLan_updateDhcpd();
      }
   }

   return ret;
}


#ifdef DMP_DEVICE2_DHCPV4SERVERCLIENTINFO_1

CmsRet rcl_dev2Dhcpv4ServerPoolClientObject( _Dev2Dhcpv4ServerPoolClientObject *newObj,
                const _Dev2Dhcpv4ServerPoolClientObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumDhcpv4ServerPoolClient(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumDhcpv4ServerPoolClient(iidStack, -1);
   }

   return ret;
}


CmsRet rcl_dev2Dhcpv4ServerPoolClientIPv4AddressObject( _Dev2Dhcpv4ServerPoolClientIPv4AddressObject *newObj,
                const _Dev2Dhcpv4ServerPoolClientIPv4AddressObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumDhcpv4ServerPoolClientIPv4Address(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumDhcpv4ServerPoolClientIPv4Address(iidStack, -1);
   }

   return ret;
}


CmsRet rcl_dev2Dhcpv4ServerPoolClientOptionObject( _Dev2Dhcpv4ServerPoolClientOptionObject *newObj,
                const _Dev2Dhcpv4ServerPoolClientOptionObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumDhcpv4ServerPoolClientOption(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumDhcpv4ServerPoolClientOption(iidStack, -1);
   }

   return ret;
}

#endif    /* DMP_DEVICE2_DHCPV4SERVERCLIENTINFO_1 */


#ifdef DMP_DEVICE2_DHCPV4RELAY_1

CmsRet rcl_dev2Dhcpv4RelayObject( _Dev2Dhcpv4RelayObject *newObj __attribute((unused)),
                const _Dev2Dhcpv4RelayObject *currObj __attribute((unused)),
                const InstanceIdStack *iidStack __attribute((unused)),
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2Dhcpv4RelayForwardingObject( _Dev2Dhcpv4RelayForwardingObject *newObj __attribute((unused)),
                const _Dev2Dhcpv4RelayForwardingObject *currObj __attribute((unused)),
                const InstanceIdStack *iidStack __attribute((unused)),
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("Entered:");

   if (ADD_NEW(newObj, currObj))
   {
      modifyDhcpv4ForwardingNumEntry(iidStack, 1);

      /* startup with existing config file, IP.Interface is already set,
       * so just set status=ENABLED now.
       */
      if (!IS_EMPTY_STRING(newObj->interface))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,
                                              mdmLibCtx.allocFlags);
      }
      return ret;
   }


   if (newObj && currObj && newObj->enable)
   {
      if (cmsUtl_strcmp(newObj->interface, currObj->interface) ||
          cmsUtl_strcmp(newObj->DHCPServerIPAddress, currObj->DHCPServerIPAddress) ||
          (newObj->enable != currObj->enable))
      {
         rutLan_updateDhcpd();
      }
      CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,
                                  mdmLibCtx.allocFlags);
   }

   if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /* regardless of delete or disable, re-write the dhcpd config file */
      rutLan_updateDhcpd();

      if (DELETE_EXISTING(newObj, currObj))
      {
         modifyDhcpv4ForwardingNumEntry(iidStack, -1);
      }
      else
      {
         /* if not a delete (just disable existing), then update status */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);
      }
   }

   return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_DHCPV4RELAY_1 */

#endif  /* DMP_DEVICE2_DHCPV4_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */

