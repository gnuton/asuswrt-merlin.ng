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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_iptunnel.h"
#include "rut2_iptunnel.h"
#include "rut_iptables.h"


CmsRet rcl_dev2DsliteObject(_Dev2DsliteObject *newObj __attribute__((unused)),
                const _Dev2DsliteObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

static CmsRet getTunnelInterfacePath(const char *ifname, char *ipintfpath, UINT32 pathLen, UBOOL8 istunnel)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2IpInterfaceObject *ipIntf = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("ifname<%s> istunnel<%d>", ifname, istunnel);

   if ((ret = cmsObj_addInstance(MDMOID_DEV2_IP_INTERFACE, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add IP Instance, ret = %d", ret);
      return ret;
   }

   if ((ret = cmsObj_get(MDMOID_DEV2_IP_INTERFACE, &iidStack, 0, (void **) &ipIntf)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get ipIntf, ret = %d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_IP_INTERFACE, &iidStack);
      return ret;
   }

   ipIntf->enable = TRUE;
   ipIntf->IPv4Enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipIntf->status, MDMVS_UP, mdmLibCtx.allocFlags);

   if (istunnel)
   {
      CMSMEM_REPLACE_STRING_FLAGS(ipIntf->type, MDMVS_TUNNEL, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(ipIntf->name, "DSLiteTunnel", mdmLibCtx.allocFlags);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(ipIntf->type, MDMVS_TUNNELED, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(ipIntf->name, "DSLiteTunneled", mdmLibCtx.allocFlags);
   }
   
   ret = cmsObj_set(ipIntf, &iidStack);
   cmsObj_free((void **) &ipIntf);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ipIntf. ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_IP_INTERFACE, &iidStack);
      return ret;
   }

   if ((ret = cmsObj_setNonpersistentInstance(MDMOID_DEV2_IP_INTERFACE, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ipintfobj nonpersistent. ret=%d", ret);
   }
   
   {
      char *fullPath=NULL;
      MdmPathDescriptor ipIntfPathDesc;

      memset(&ipIntfPathDesc, 0, sizeof(MdmPathDescriptor));
      ipIntfPathDesc.oid = MDMOID_DEV2_IP_INTERFACE;
      ipIntfPathDesc.iidStack = iidStack;
      if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&ipIntfPathDesc, &fullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPathNoEndDot returns error. ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_DEV2_IP_INTERFACE, &iidStack);
         return ret;
      }

      cmsUtl_strncpy(ipintfpath, fullPath, pathLen);

      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
   }

   return ret;
}

CmsRet rcl_dev2DsliteInterfaceSettingObject(_Dev2DsliteInterfaceSettingObject *newObj __attribute__((unused)),
                const _Dev2DsliteInterfaceSettingObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if ( ADD_NEW(newObj, currObj) )
   {
      char tunnelintf[MDM_SINGLE_FULLPATH_BUFLEN];

      if ((ret = getTunnelInterfacePath("", tunnelintf, sizeof(tunnelintf), TRUE)) == CMSRET_SUCCESS)
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->tunnelInterface, tunnelintf, mdmLibCtx.allocFlags);

         if ((ret = getTunnelInterfacePath("", tunnelintf, sizeof(tunnelintf), FALSE)) == CMSRET_SUCCESS)
         {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->tunneledInterface, tunnelintf, mdmLibCtx.allocFlags);
         }
      }

      return ret;
   }

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /*
       * If dynamic tunnel, EndpointAddress is empty string
       * So, Origin must be set to static if EndpointAddress is not empty
       *
       * EndpointAddressInUse: 
       *   If dynamic, resolved addr of EndpointName is set.
       *   Otherwise, copy EndpointAddress to EndpointAddressInUse
       */
      if (newObj->endpointAddress)
      {
         if (!cmsUtl_isValidIpAddress(AF_INET6, newObj->endpointAddress))
         {
            cmsLog_error("invalid static aftr addr <%s>", newObj->endpointAddress);
            return CMSRET_INVALID_ARGUMENTS;
         }

         CMSMEM_REPLACE_STRING_FLAGS(newObj->origin, MDMVS_STATIC, mdmLibCtx.allocFlags);
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->origin, MDMVS_DHCPV6, mdmLibCtx.allocFlags);

         /*
          * if configure dynamic, need to re-launch dhcp6c to request aftr
          */
         if (cmsUtl_strcmp(currObj->origin, MDMVS_DHCPV6) != 0)
         {
            rutTunnel_restartDhcpClientForTunnel(newObj->X_BROADCOM_COM_TunneledInterface, FALSE);
            return ret;
         }
      }
   }

   if (newObj != NULL && newObj->enable)
   {
      char wanIp[CMS_IPADDR_LENGTH];
      char ipstr[CMS_IPADDR_LENGTH];
      char ifname[CMS_IFNAME_LENGTH];
      UBOOL8 firewall = 0;

      /* Fetch the IPv6 address of the respective WAN interface */
      if ((ret = rutTunnel_getTunneledWanInfo(newObj->X_BROADCOM_COM_TunnelInterface,
                                      newObj->X_BROADCOM_COM_TunneledInterface,
                                      wanIp, &firewall,
                                      ifname, FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutTunnel_getTunneledWanInfo fail");
         return ret;
      }

      cmsLog_debug( "Configuring 4in6 tunnel: OldStatus<%s> Origin<%s> "
                    "endpointAddress<%s> aftr<%s> lan<%s>", 
                    currObj->status, newObj->origin, 
                    newObj->endpointAddress, newObj->endpointName, 
                    newObj->X_BROADCOM_COM_TunnelInterface);

      /*
       * If WAN service is up, wanIp for tunnel configuration will be available
       * Otherwise, wanIp is empty string
       */
      if (cmsUtl_isValidIpAddress(AF_INET6, wanIp) == TRUE)
      {
         cmsLog_debug("%s: wanIP<%s> firewall<%d>", ifname, wanIp, firewall);

         if ((ret = rutTunnel_getAftrAddress(newObj->endpointAddress, newObj->endpointName, ipstr)) != CMSRET_SUCCESS)
         {
            cmsLog_notice("getAftrAddress fail");
            return ret;
         }

         cmsLog_debug("aftr_addr<%s>", ipstr);

         if (cmsUtl_strcmp(currObj->status, MDMVS_ENABLED) == 0) 
         {
            /* 
             * No need of remote IP address for tunnel deletion. 
             * Therefore, second argument can be anything here
             */
            ret = rutTunnel_4in6Config(wanIp, currObj->endpointAddressInUse, FALSE);
         }

         ret = rutTunnel_4in6Config(wanIp, ipstr, TRUE);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->endpointAddressInUse, ipstr, mdmLibCtx.allocFlags);

         /* 
          * If the respective IPv6 WAN interface is firewall enabled,
          * this DS-Lite tunnel needs to enable IPv4 firewall too.
          */
         if ( firewall )
         {
            rutIpt_insertIpModules();
            rutIpt_initFirewall(PF_INET, "ip6tnl1");
//            rutIpt_TCPMSSforIPTunnel(ifname, FALSE); //FIXME AAA!!!
         }
      }
      else
      {
         if (cmsUtl_strcmp(currObj->status, MDMVS_ENABLED) == 0)
         {
            ret = rutTunnel_4in6Config(wanIp, currObj->endpointAddressInUse, FALSE);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->endpointAddressInUse);
         }
      }
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting 4in6 tunnel");
      if (cmsUtl_strcmp(currObj->status, MDMVS_ENABLED) == 0)
      {
         ret = rutTunnel_4in6Config(NULL, currObj->endpointAddressInUse, FALSE);
      }

      if (newObj) /* case of disable */
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
         CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->endpointAddressInUse);
      }
      else /* case of delete */
      {
         MdmPathDescriptor ipintf = EMPTY_PATH_DESCRIPTOR;

         cmsMdm_fullPathToPathDescriptor(currObj->tunnelInterface, &ipintf);

         if (cmsObj_deleteInstance(MDMOID_DEV2_IP_INTERFACE, &ipintf.iidStack) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not delete IP.Interface at %s", cmsMdm_dumpIidStack(&ipintf.iidStack));
         }

         cmsMdm_fullPathToPathDescriptor(currObj->tunneledInterface, &ipintf);
         if (cmsObj_deleteInstance(MDMOID_DEV2_IP_INTERFACE, &ipintf.iidStack) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not delete IP.Interface at %s", cmsMdm_dumpIidStack(&ipintf.iidStack));
         }          
      }
   }   

   return ret;
}


#endif  /* SUPPORT_IPV6 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
