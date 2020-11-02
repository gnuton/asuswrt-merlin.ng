/***********************************************************************
 *
 *  Copyright (c) 2010  Broadcom Corporation
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
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_iptables.h"
#include "rut_ebtables.h"
#include "rut_iptunnel.h"

CmsRet rcl_ipTunnelObject( _IPTunnelObject *newObj,
                             const _IPTunnelObject *currObj,
                             const InstanceIdStack *iidStack,
                             char **errorParam __attribute__((unused)),
                             CmsRet *errorCode  __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));
   
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if ( (newObj != NULL) && (currObj != NULL) )
   {
      void *obj;
      InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;

      cmsLog_debug("newObj: Name<%s> Mode<%s> WAN<%s> LAN<%s> Activated<%d>", 
                            newObj->tunnelName, newObj->mode, newObj->associatedWanIfName, 
                            newObj->associatedLanIfName, newObj->activated);

      if ( cmsUtl_strcmp(newObj->mode, MDMVS_IPV6INIPV4) == 0 )
      {
         if ( (ret = cmsObj_getNextInSubTreeFlags(MDMOID_IPV6IN_IPV4_TUNNEL, iidStack, &iidStack1,
                                                                              OGF_NO_VALUE_UPDATE, &obj) != CMSRET_SUCCESS ) )
         {
            cmsLog_error("Failed at getting 6in4 object, ret=%d", ret);
            return ret;
         }

      }
      else if ( cmsUtl_strcmp(newObj->mode, MDMVS_IPV4INIPV6) == 0 )
      {
         if ( (ret = cmsObj_getNextInSubTreeFlags(MDMOID_IPV4IN_IPV6_TUNNEL, iidStack, &iidStack1,
                                                                              OGF_NO_VALUE_UPDATE, &obj) != CMSRET_SUCCESS ) )
         {
            cmsLog_error("Failed at getting 4in6 object, ret=%d", ret);
            return ret;
         }

      }

      if ( newObj->activated )
      {
         /* connStatus must be the first parameter in both Ipv6inIpv4TunnelObject and Ipv6inIpv4TunnelObject structures */
         CMSMEM_REPLACE_STRING_FLAGS(((_Ipv6inIpv4TunnelObject *)obj)->connStatus, MDMVS_CONNECTED, mdmLibCtx.allocFlags);
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(((_Ipv6inIpv4TunnelObject *)obj)->connStatus, MDMVS_DISCONNECTED, mdmLibCtx.allocFlags);
      }
   
      if ( (ret = cmsObj_set(obj, &iidStack1)) != CMSRET_SUCCESS )
      {
         cmsLog_error("Failed to set MDMOID_IPVxIN_IPVx_TUNNEL object");
      }

      cmsObj_free((void **) &obj);

   }

   return ret;
}


CmsRet rcl_ipv6inIpv4TunnelObject( _Ipv6inIpv4TunnelObject *newObj,
                             const _Ipv6inIpv4TunnelObject *currObj,
                             const InstanceIdStack *iidStack,
                             char **errorParam __attribute__((unused)),
                             CmsRet *errorCode  __attribute__((unused)))
{
   _IPTunnelObject *tunnelObj = NULL;
   InstanceIdStack iidStack1;
   char wanIntf[BUFLEN_32];
   char lanIntf[BUFLEN_32];
   char tunnelName[BUFLEN_32];
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));
   
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if ( ADD_NEW(newObj, currObj) )
   {
      rutEbt_configICMPv6Reply(newObj->prefix, TRUE);
      return ret;
   }

   iidStack1 = *iidStack;
   if (cmsObj_getAncestorFlags( MDMOID_IP_TUNNEL, MDMOID_IPV6IN_IPV4_TUNNEL,
                                                    &iidStack1, OGF_NO_VALUE_UPDATE, 
                                                    (void **) &tunnelObj) != CMSRET_SUCCESS )
   {
      cmsLog_error("6in4Tunnel object cannot get its IPTunnel object.");
      return CMSRET_MDM_TREE_ERROR;
   }

   if ( (tunnelObj->associatedWanIfName == NULL) || (tunnelObj->associatedLanIfName == NULL) )
   {
      cmsLog_debug("IPTunnel object is not configured yet");
      cmsObj_free((void **) &tunnelObj);
      return ret;
   }

   cmsUtl_strncpy(wanIntf, tunnelObj->associatedWanIfName, BUFLEN_32);
   cmsUtl_strncpy(lanIntf, tunnelObj->associatedLanIfName, BUFLEN_32);
   cmsUtl_strncpy(tunnelName, tunnelObj->tunnelName, BUFLEN_32);
   cmsObj_free((void **) &tunnelObj);

   if (newObj != NULL && currObj != NULL)
   {
      char wanIp[CMS_IPADDR_LENGTH];
      UBOOL8 firewall = 0;

      /* Fetch the IPv4 address of the respective WAN interface */
      rutTunnel_getAssociatedWanInfo( wanIntf, wanIp, &firewall, TRUE );

      cmsLog_debug( "Configuring 6in4 tunnel: ConnStatus<%s> Mechanism<%s> Dynamic<%d> Ipv4MaskLen<%d> 6rdPrefix<%s> BRAddr<%s> wan<%s:%s:%d> lan<%s>", 
  	                 newObj->connStatus, newObj->mechanism, newObj->dynamic, newObj->ipv4MaskLen, newObj->prefix, newObj->borderRelayAddress, wanIntf, wanIp, firewall, lanIntf);

      if ( cmsUtl_strcmp(newObj->connStatus, MDMVS_CONNECTED) == 0 )
      {
         if ( (cmsUtl_isValidIpAddress(AF_INET, wanIp) == FALSE) ||
               (cmsUtl_isValidIpAddress(AF_INET6, newObj->prefix) == FALSE) ||
               (cmsUtl_isValidIpAddress(AF_INET, newObj->borderRelayAddress) == FALSE) )

         {
            cmsLog_error("Invalid IP address: prefix<%s> wanIp%s> brAddr<%s>", newObj->prefix, wanIp, newObj->borderRelayAddress);
            return CMSRET_INVALID_PARAM_VALUE;
         }

         if ( cmsUtl_strcmp(currObj->connStatus, MDMVS_CONNECTED) == 0 )
         {
            ret = rutTunnel_6rdConfig(wanIp, currObj->prefix, currObj->borderRelayAddress, currObj->ipv4MaskLen, tunnelName, lanIntf, FALSE);
            ret = rutTunnel_6rdConfig(wanIp, newObj->prefix, newObj->borderRelayAddress, newObj->ipv4MaskLen, tunnelName, lanIntf, TRUE);
         }
         else
         {
            ret = rutTunnel_6rdConfig(wanIp, newObj->prefix, newObj->borderRelayAddress, newObj->ipv4MaskLen, tunnelName, lanIntf, TRUE);
         }

         if ( firewall )
         {
            rutIpt_insertIpModules6();
            rutIpt_initFirewall(PF_INET6, "sit1");
            rutIpt_TCPMSSforIPTunnel(wanIntf, TRUE);
         }
      }
      else
      {
         if ( cmsUtl_strcmp(currObj->connStatus, MDMVS_CONNECTED) == 0 )
         {
            ret = rutTunnel_6rdConfig(wanIp, currObj->prefix, currObj->borderRelayAddress, currObj->ipv4MaskLen, tunnelName, lanIntf, FALSE);
         }
      }
   }
   else if ( DELETE_EXISTING(newObj, currObj) )
   {
      cmsLog_debug("Deleting 6in4 tunnel");
      ret = rutTunnel_6rdConfig(NULL, currObj->prefix, currObj->borderRelayAddress, currObj->ipv4MaskLen, tunnelName, lanIntf, FALSE);
   }   

   return ret;
}


CmsRet rcl_ipv4inIpv6TunnelObject( _Ipv4inIpv6TunnelObject *newObj,
                             const _Ipv4inIpv6TunnelObject *currObj,
                             const InstanceIdStack *iidStack,
                             char **errorParam __attribute__((unused)),
                             CmsRet *errorCode  __attribute__((unused)))
{
   _IPTunnelObject *tunnelObj = NULL;
   InstanceIdStack iidStack1;
   char wanIntf[BUFLEN_32];
   char lanIntf[BUFLEN_32];
   char tunnelName[BUFLEN_32];
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));
   
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if ( ADD_NEW(newObj, currObj) )
   {
      return ret;
   }

   iidStack1 = *iidStack;
   if (cmsObj_getAncestorFlags( MDMOID_IP_TUNNEL, MDMOID_IPV4IN_IPV6_TUNNEL,
                                                    &iidStack1, OGF_NO_VALUE_UPDATE, 
                                                    (void **) &tunnelObj) != CMSRET_SUCCESS )
   {
      cmsLog_error("4in6Tunnel object cannot get its IPTunnel object.");
      return CMSRET_MDM_TREE_ERROR;
   }

   if ( (tunnelObj->associatedWanIfName == NULL) || (tunnelObj->associatedLanIfName == NULL) )
   {
      cmsLog_debug("IPTunnel object is not configured yet");
      cmsObj_free((void **) &tunnelObj);
      return ret;
   }

   cmsUtl_strncpy(wanIntf, tunnelObj->associatedWanIfName, BUFLEN_32);
   cmsUtl_strncpy(lanIntf, tunnelObj->associatedLanIfName, BUFLEN_32);
   cmsUtl_strncpy(tunnelName, tunnelObj->tunnelName, BUFLEN_32);
   cmsObj_free((void **) &tunnelObj);

   if (newObj != NULL && currObj != NULL)
   {
      char wanIp[CMS_IPADDR_LENGTH];
      char ipstr[CMS_IPADDR_LENGTH];
      char aftr_info[CMS_AFTR_NAME_LENGTH];
      UBOOL8 firewall = 0;

      /* Fetch the IPv6 address of the respective WAN interface */
      rutTunnel_getAssociatedWanInfo( wanIntf, wanIp, &firewall, FALSE );

      cmsLog_debug( "Configuring 4in6 tunnel: ConnStatus<%s> Mechanism<%s> Dynamic<%d> "
                    "remoteIp<%s> aftr<%s> wan<%s:%s:%d> lan<%s>", 
                    newObj->connStatus, newObj->mechanism, newObj->dynamic, 
                    newObj->remoteIpv6Address, newObj->currentAftrName, wanIntf, wanIp, firewall, lanIntf);


      if ( cmsUtl_strcmp(newObj->connStatus, MDMVS_CONNECTED) == 0 )
      {
         if ( cmsUtl_isValidIpAddress(AF_INET6, wanIp) == FALSE )
         {
            cmsLog_error("Invalid IP address: wanIp<%s>", wanIp);
            return CMSRET_INVALID_PARAM_VALUE;
         }

         /* Choose the AFTR info according to dynamic flag */
         if ( newObj->dynamic )
         {
            if ( !IS_EMPTY_STRING(newObj->currentAftrName) )
            {
               cmsUtl_strncpy(aftr_info, newObj->currentAftrName, CMS_AFTR_NAME_LENGTH);
            }
            else
            {
               cmsLog_error("No AFTR NAME from DHCPv6 for dynamic DS-Lite tunnel.");
               return CMSRET_INVALID_PARAM_VALUE;
            }
         }
         else
         {
            if ( !IS_EMPTY_STRING(newObj->remoteIpv6Address) )
            {
               cmsUtl_strncpy(aftr_info, newObj->remoteIpv6Address, CMS_AFTR_NAME_LENGTH);
            }
            else
            {
               cmsLog_error("No AFTR NAME from configuration for static DS-Lite tunnel.");
               return CMSRET_INVALID_PARAM_VALUE;
            }
         }

         /* If the AFTR info is domain name, we need to get address resolved */
         if ( cmsUtl_isValidIpAddress(AF_INET6, aftr_info) == FALSE )
         {
            struct addrinfo hints, *res;
            int status;

            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_INET6;
            hints.ai_socktype = SOCK_STREAM;
 
            if ((status = getaddrinfo(aftr_info, NULL, &hints, &res)) != 0) 
            {
               cmsLog_error("getaddrinfo: %s", gai_strerror(status));
               return CMSRET_INVALID_PARAM_VALUE;
            }
            else
            {
               struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
 
               inet_ntop(res->ai_family, &(ipv6->sin6_addr), ipstr, INET6_ADDRSTRLEN);
               freeaddrinfo(res);
            }
         }
         else
         {
            cmsUtl_strncpy(ipstr, aftr_info, CMS_IPADDR_LENGTH);
         }

         cmsLog_debug("ipstr<%s>", ipstr);

         if ( cmsUtl_strcmp(currObj->connStatus, MDMVS_CONNECTED) == 0 )
         {
            /* 
             * No need of remote IP address for tunnel deletion. 
             * Therefore, second argument can be anything here
             */
            ret = rutTunnel_4in6Config(wanIp, currObj->remoteIpv6Address, FALSE);
            ret = rutTunnel_4in6Config(wanIp, ipstr, TRUE);
         }
         else
         {
            ret = rutTunnel_4in6Config(wanIp, ipstr, TRUE);
         }

         /* 
          * If the respective IPv6 WAN interface is firewall enabled,
          * this DS-Lite tunnel needs to enable IPv4 firewall too.
          */
         if ( firewall )
         {
            rutIpt_insertIpModules();
            rutIpt_initFirewall(PF_INET, "ip6tnl1");
            rutIpt_TCPMSSforIPTunnel(wanIntf, FALSE);
         }
      }
      else
      {
         if ( cmsUtl_strcmp(currObj->connStatus, MDMVS_CONNECTED) == 0 )
         {
            ret = rutTunnel_4in6Config(wanIp, currObj->remoteIpv6Address, FALSE);
         }
      }
   }
   else if ( newObj == NULL )
   {
      cmsLog_debug("Deleting 4in6 tunnel");
      ret = rutTunnel_4in6Config(NULL, currObj->remoteIpv6Address, FALSE);
   }   

   return ret;}
#endif
