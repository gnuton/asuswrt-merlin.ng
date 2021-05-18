/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
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

#ifdef SUPPORT_UPNP

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "cms_core.h"
#include "odl.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_iptables.h"
#include "rut_route.h"
#include "rut_upnp.h"
#include "rut_wan.h"
#include "rut_wanlayer2.h"
#include "rut_iptunnel.h"


UBOOL8 rut_getPCPInterfaceName(char *ifcName);
UBOOL8 rut_getPCPInterfaceName_igd(char *ifcName);
UBOOL8 rut_getPCPInterfaceName_dev2(char *ifcName);

#if defined(SUPPORT_DM_LEGACY98)
#define rut_getPCPInterfaceName(w)  rut_getPCPInterfaceName_igd((w))
#elif defined(SUPPORT_DM_HYBRID)
#define rut_getPCPInterfaceName(w)  rut_getPCPInterfaceName_igd((w))
#elif defined(SUPPORT_DM_PURE181)
#define rut_getPCPInterfaceName(w)  rut_getPCPInterfaceName_dev2((w))
#elif defined(SUPPORT_DM_DETECT)
#define rut_getPCPInterfaceName(w)  (cmsMdm_isDataModelDevice2() ? \
                                    rut_getPCPInterfaceName_dev2((w)) :  \
                                    rut_getPCPInterfaceName_igd((w)))
#endif

CmsRet rutUpnp_getPcpInfo(const char *ifName, int *pcpMode,
                          char pcpServer[], char pcpLocal[]);
CmsRet rutUpnp_getPcpInfo_igd(const char *ifName, int *pcpMode,
                              char pcpServer[], char pcpLocal[]);
CmsRet rutUpnp_getPcpInfo_dev2(const char *ifName, int *pcpMode,
                               char pcpServer[], char pcpLocal[]);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUpnp_getPcpInfo(a, b, c, d)  rutUpnp_getPcpInfo_igd((a), (b), (c), (d))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUpnp_getPcpInfo(a, b, c, d)  rutUpnp_getPcpInfo_igd((a), (b), (c), (d))
#elif defined(SUPPORT_DM_PURE181)
#define rutUpnp_getPcpInfo(a, b, c, d)  rutUpnp_getPcpInfo_dev2((a), (b), (c), (d))
#elif defined(SUPPORT_DM_DETECT)
#define rutUpnp_getPcpInfo(a, b, c, d)  (cmsMdm_isDataModelDevice2() ? \
                                 rutUpnp_getPcpInfo_dev2((a), (b), (c), (d)) :  \
                                 rutUpnp_getPcpInfo_igd((a), (b), (c), (d)))
#endif


#ifdef DMP_BASELINE_1
void rutUpnp_configIptableRuleForAllWanIfcs_igd(UBOOL8 addRules)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanPppConnObject *pppConn = NULL;
   WanIpConnObject *ipConn = NULL;

   cmsLog_debug("Enter: addRules=%d", addRules);


   while (cmsObj_getNext(MDMOID_WAN_IP_CONN, &iidStack, (void **) &ipConn) == CMSRET_SUCCESS)
   {
      if (ipConn->NATEnabled)
      {
         rutIpt_upnpConfigStopMulticast(ipConn->X_BROADCOM_COM_IfName, addRules);
      }
      cmsObj_free((void **) &ipConn);
   }
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNext(MDMOID_WAN_PPP_CONN, &iidStack, (void **) &pppConn) == CMSRET_SUCCESS)
   {
      if (pppConn->NATEnabled)
      {
         rutIpt_upnpConfigStopMulticast(pppConn->X_BROADCOM_COM_IfName, addRules);
      }
      cmsObj_free((void **) &pppConn);
   }

   cmsLog_debug("Done");
   
   return;
}

UBOOL8 rut_getPCPInterfaceName_igd(char *ifcName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanIpConnObject *ipConn = NULL;
   WanPppConnObject *pppConn = NULL;
   UBOOL8 found = FALSE;
   
   cmsLog_debug("Enter");

   if (ifcName == NULL)
   {
      cmsLog_error("Null ifcName.");
      return FALSE;
   }

   while (!found && cmsObj_getNext(MDMOID_WAN_IP_CONN, &iidStack, (void **) &ipConn) == CMSRET_SUCCESS)
   {
      if ( ((ipConn->X_BROADCOM_COM_PCPMode == PCP_MODE_NAT444) && !cmsUtl_strcmp(ipConn->connectionStatus, MDMVS_CONNECTED)) 
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
           || ((ipConn->X_BROADCOM_COM_PCPMode == PCP_MODE_DSLITE) && !cmsUtl_strcmp(ipConn->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED)) 
#endif
         )
      {
         strcpy(ifcName, ipConn->X_BROADCOM_COM_IfName);
         found = TRUE;
      }
      cmsObj_free((void **) &ipConn);
   }
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found && cmsObj_getNext(MDMOID_WAN_PPP_CONN, &iidStack, (void **) &pppConn) == CMSRET_SUCCESS)
   {
      if ( ((pppConn->X_BROADCOM_COM_PCPMode == PCP_MODE_NAT444) && !cmsUtl_strcmp(pppConn->connectionStatus, MDMVS_CONNECTED)) 
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
           || ((pppConn->X_BROADCOM_COM_PCPMode == PCP_MODE_DSLITE) && !cmsUtl_strcmp(pppConn->X_BROADCOM_COM_IPv6ConnStatus, MDMVS_CONNECTED)) 
#endif
         )
      {
         strcpy(ifcName, pppConn->X_BROADCOM_COM_IfName);
         found = TRUE;
      }
      cmsObj_free((void **) &pppConn);
   }

   return found;
}

CmsRet rutUpnp_getPcpInfo_igd(const char *ifName, int *pcpMode, char pcpServer[], char pcpLocal[])
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanIpConnObject *ipConn = NULL;
   WanPppConnObject *pppConn = NULL;
   UBOOL8 found = FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Enter: ifname<%s>", ifName);

   while (!found && cmsObj_getNext(MDMOID_WAN_IP_CONN, &iidStack, (void **) &ipConn) == CMSRET_SUCCESS)
   {
      if (strcmp(ipConn->X_BROADCOM_COM_IfName, ifName) == 0)
      {
         found = TRUE;
         *pcpMode = ipConn->X_BROADCOM_COM_PCPMode;

         if (*pcpMode != PCP_MODE_DISABLE) 
         {
            if (*pcpMode == PCP_MODE_DSLITE)
            {
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
               char addr[CMS_IPADDR_LENGTH];
               char *ptr;

               addr[0] = '\0';
               if (cmsUtl_isValidIpAddress(AF_INET6, 
                                 ipConn->X_BROADCOM_COM_ExternalIPv6Address))
               {
                   strcpy(addr, ipConn->X_BROADCOM_COM_ExternalIPv6Address);
               }
               /* 
                * If there is no address associated with WAN interface, 
                * use the delegated address from brX 
                */
               else
               {
                   cmsLog_debug("no WAN addr, try brX");
                   rutTunnel_getLanAddrForDsLite(ifName, addr);
               }

               if (cmsUtl_isValidIpAddress(AF_INET6, addr) && 
                   cmsUtl_isValidIpAddress(AF_INET6, ipConn->X_BROADCOM_COM_PCPServer))
               {
                  /* remove prefix info of X_BROADCOM_COM_ExternalIPv6Address */
                  ptr = strstr(addr, "/");
                  *ptr = '\0';
                  strcpy(pcpLocal, addr);
                  strcpy(pcpServer, ipConn->X_BROADCOM_COM_PCPServer);
               }
               else
               {
                  cmsLog_notice("pcpMode<%d> but no valid pcpLocal/pcpServer", *pcpMode);
                  ret = CMSRET_INTERNAL_ERROR;
               }
#else
               cmsObj_free((void **) &ipConn);
               cmsLog_error("pcpMode DSLITE but IPv6 is not enabled");
               return CMSRET_INVALID_PARAM_VALUE;
#endif
            }
            else /* PCP_MODE_NAT444 */
            {
               if (cmsUtl_isValidIpAddress(AF_INET, ipConn->externalIPAddress) &&
                   cmsUtl_isValidIpAddress(AF_INET, ipConn->X_BROADCOM_COM_PCPServer))
               {
                  strcpy(pcpServer, ipConn->X_BROADCOM_COM_PCPServer);
                  strcpy(pcpLocal, ipConn->externalIPAddress);
               }
               else
               {
                  cmsLog_notice("pcpMode<%d> but no valid pcpLocal/pcpServer", *pcpMode);
                  ret = CMSRET_INTERNAL_ERROR;
               }
            }
         }
      }
      cmsObj_free((void **) &ipConn);
   }
   
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found && cmsObj_getNext(MDMOID_WAN_PPP_CONN, &iidStack, (void **) &pppConn) == CMSRET_SUCCESS)
   {
      if (strcmp(pppConn->X_BROADCOM_COM_IfName, ifName) == 0)
      {
         found = TRUE;
         *pcpMode = pppConn->X_BROADCOM_COM_PCPMode;

         if (*pcpMode != PCP_MODE_DISABLE) 
         {
            if (*pcpMode == PCP_MODE_DSLITE)
            {
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
               char addr[CMS_IPADDR_LENGTH];
               char *ptr;

               addr[0] = '\0';
               if (cmsUtl_isValidIpAddress(AF_INET6, 
                                 pppConn->X_BROADCOM_COM_ExternalIPv6Address))
               {
                   strcpy(addr, pppConn->X_BROADCOM_COM_ExternalIPv6Address);
               }
               /* 
                * If there is no address associated with WAN interface, 
                * use the delegated address from brX 
                */
               else
               {
                   cmsLog_debug("no WAN addr, try brX");
                   rutTunnel_getLanAddrForDsLite(ifName, addr);
               }

               if (cmsUtl_isValidIpAddress(AF_INET6, addr) && 
                   cmsUtl_isValidIpAddress(AF_INET6, pppConn->X_BROADCOM_COM_PCPServer))
               {
                  /* remove prefix info of X_BROADCOM_COM_ExternalIPv6Address */
                  ptr = strstr(addr, "/");
                  *ptr = '\0';
                  strcpy(pcpLocal, addr);
                  strcpy(pcpServer, pppConn->X_BROADCOM_COM_PCPServer);
               }
               else
               {
                  cmsLog_notice("pcpMode<%d> but no valid pcpLocal/pcpServer", *pcpMode);
                  ret = CMSRET_INTERNAL_ERROR;
               }
#else
               cmsObj_free((void **) &pppConn);
               cmsLog_error("pcpMode DSLITE but IPv6 is not enabled");
               return CMSRET_INVALID_PARAM_VALUE;
#endif
            }
            else /* PCP_MODE_NAT444 */
            {
               if (cmsUtl_isValidIpAddress(AF_INET, pppConn->externalIPAddress) &&
                   cmsUtl_isValidIpAddress(AF_INET, pppConn->X_BROADCOM_COM_PCPServer))
               {
                  strcpy(pcpServer, pppConn->X_BROADCOM_COM_PCPServer);
                  strcpy(pcpLocal, pppConn->externalIPAddress);
               }
               else
               {
                  cmsLog_notice("pcpMode<%d> but no valid pcpLocal/pcpServer", *pcpMode);
                  ret = CMSRET_INTERNAL_ERROR;
               }
            }
         }
      }
      cmsObj_free((void **) &pppConn);
   }

   cmsLog_debug("found<%d>, pcpMode<%d> pcpSrv<%s> pcpLocal<%s>", found, *pcpMode, pcpServer, pcpLocal);

   return ret;
}
#endif  /* DMP_BASELINE_1 */


#ifdef DMP_DEVICE2_BASELINE_1
void rutUpnp_configIptableRuleForAllWanIfcs_dev2(UBOOL8 addRules)
{
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   cmsLog_debug("Enter: addRules=%d", addRules);

   while (cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                         (void **)&ipIntfObj) == CMSRET_SUCCESS)
   {
      if (qdmIpIntf_isNatEnabledOnIntfNameLocked(ipIntfObj->name))
      {
         rutIpt_upnpConfigStopMulticast(ipIntfObj->name, addRules);
      }

      cmsObj_free((void **) &ipIntfObj);
   }

   cmsLog_debug("Done");

   return;
}

UBOOL8 rut_getPCPInterfaceName_dev2(char *ifcName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2IpInterfaceObject *obj = NULL;
   UBOOL8 found = FALSE;
   
   cmsLog_debug("Enter");

   if (ifcName == NULL)
   {
      cmsLog_error("Null ifcName.");
      return FALSE;
   }

   while (!found && cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack, 
                    OGF_NO_VALUE_UPDATE, (void **) &obj) == CMSRET_SUCCESS)
   {
      if ( ((obj->X_BROADCOM_COM_PCPMode == PCP_MODE_NAT444) && 
            !cmsUtl_strcmp(obj->X_BROADCOM_COM_IPv4ServiceStatus, MDMVS_SERVICEUP))
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
           || ((obj->X_BROADCOM_COM_PCPMode == PCP_MODE_DSLITE) &&
               !cmsUtl_strcmp(obj->X_BROADCOM_COM_IPv6ServiceStatus, MDMVS_SERVICEUP)) 
#endif
         )
      {
         strcpy(ifcName, obj->name);
         found = TRUE;
      }
      cmsObj_free((void **) &obj);
   }
   
   return found;
}

CmsRet rutUpnp_getPcpInfo_dev2(const char *ifName, int *pcpMode, char pcpServer[], char pcpLocal[])
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2IpInterfaceObject *obj = NULL;
   UBOOL8 found = FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Enter: ifname<%s>", ifName);

   while (!found && cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                    OGF_NO_VALUE_UPDATE, (void **) &obj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(obj->name, ifName) == 0)
      {
         found = TRUE;
         *pcpMode = obj->X_BROADCOM_COM_PCPMode;

         if (*pcpMode != PCP_MODE_DISABLE) 
         {
            if (*pcpMode == PCP_MODE_DSLITE)
            {
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
               char addr[CMS_IPADDR_LENGTH];
               char addrtmp[CMS_IPADDR_LENGTH];

               qdmIpIntf_getIpv6AddressByNameLocked_dev2(obj->name, addrtmp);

               addr[0] = '\0';
               if (cmsUtl_isValidIpAddress(AF_INET6, addrtmp))
               {
                  char *ptr;

                  ptr = strchr(addrtmp, '/');
                  *ptr = '\0';
                  strcpy(addr, addrtmp);
               }
               /* 
                * If there is no address associated with WAN interface, 
                * use the delegated address from brX 
                */
               else
               {
                   cmsLog_debug("no WAN addr, try brX");
                   rutTunnel_getLanAddrForDsLite_dev2(ifName, addr);
               }

               if (cmsUtl_isValidIpAddress(AF_INET6, addr) && 
                   cmsUtl_isValidIpAddress(AF_INET6, obj->X_BROADCOM_COM_PCPServer))
               {
                  strcpy(pcpLocal, addr);
                  strcpy(pcpServer, obj->X_BROADCOM_COM_PCPServer);
               }
               else
               {
                  cmsLog_notice("pcpMode<%d> but no valid pcpLocal/pcpServer", *pcpMode);
                  ret = CMSRET_INTERNAL_ERROR;
               }
#else
               cmsObj_free((void **) &obj);
               cmsLog_error("pcpMode DSLITE but IPv6 is not enabled");
               return CMSRET_INVALID_PARAM_VALUE;
#endif
            }
            else /* PCP_MODE_NAT444 */
            {
               char addr[CMS_IPADDR_LENGTH]={0};

               qdmIpIntf_getIpv4AddressByNameLocked_dev2(obj->name, addr);

               if (cmsUtl_isValidIpAddress(AF_INET, addr) &&
                   cmsUtl_isValidIpAddress(AF_INET, obj->X_BROADCOM_COM_PCPServer))
               {
                  strcpy(pcpServer, obj->X_BROADCOM_COM_PCPServer);
                  strcpy(pcpLocal, addr);
               }
               else
               {
                  cmsLog_notice("pcpMode<%d> but no valid pcpLocal/pcpServer", *pcpMode);
                  ret = CMSRET_INTERNAL_ERROR;
               }
            }
         }
      }
      cmsObj_free((void **) &obj);
   }
   
   cmsLog_debug("found<%d>, pcpMode<%d> pcpSrv<%s> pcpLocal<%s>", found, *pcpMode, pcpServer, pcpLocal);

   return ret;
}
#endif /* DMP_DEVICE2_BASELINE_1 */



UBOOL8 rut_isUpnpEnabled(void)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UpnpCfgObject *upnpCfg=NULL;
   CmsRet ret;
   UBOOL8 upnpEnable = FALSE;

   cmsLog_debug("Enter");
   if ((ret = cmsObj_get(MDMOID_UPNP_CFG, &iidStack, 0, (void **) &upnpCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get UPNP_CFG, ret=%d", ret);
   }
   else
   {
      upnpEnable = upnpCfg->enable;
      cmsObj_free((void **) &upnpCfg);
   }

   cmsLog_debug("upnp enable status = %d", upnpEnable);

   return upnpEnable;
   
}


/** Start upnp process
 *
 * @ifcName       (IN) Wan interface name.  If default gateway is Nated, the default gateway will be used instead.
 * @excludeIfc    (IN) Wan interface name to be exclude -- still in mdm but is already deleted.
 *
 * @return CmsRet enum.
 */
CmsRet  rut_startUpnp(char *ifcName, char *excludeIfc, UBOOL8 pcpEnable)
{
   char cmd[BUFLEN_256]={0};
   SINT32 pid;
   CmsRet ret = CMSRET_SUCCESS;
   char upnpIfcName[CMS_IFNAME_LENGTH]={0};
   char upnpl2IfcName[CMS_IFNAME_LENGTH]={0};
   char dlftGwIfName[CMS_IFNAME_LENGTH*3]={0};
   char pcpServer[CMS_IPADDR_LENGTH]={0};
   char pcpLocal[CMS_IPADDR_LENGTH]={0};
   SINT32 pcpMode = PCP_MODE_DISABLE;
   
   cmsLog_debug("Entered: ifcName=%s exclude=%s pcpEnable=%d",
                ifcName, excludeIfc, pcpEnable);

   if (pcpEnable == FALSE)
   {
      if ((ret = qdmRt_getActiveDefaultGatewayLocked(dlftGwIfName)) != CMSRET_SUCCESS)
      {
         return ret;
      }
      else if (*dlftGwIfName == '\0')
      {
         /* Default gateway is not up yet. */
         return ret;
      }
      cmsLog_debug("got dlftGwIfName=%s", dlftGwIfName);

      /* 1)  try to attach upnp to default gateway if it is natted (nat enabled).
      *  2)  if default gateway is not natted just use this natted wan interface.
        */   
      if (cmsUtl_strcmp(excludeIfc, dlftGwIfName) &&
          qdmIpIntf_isNatEnabledOnIntfNameLocked(dlftGwIfName))
      {
         cmsLog_debug("Use default gateway ifc %s for upnp.", dlftGwIfName);
         strncpy(upnpIfcName, dlftGwIfName, sizeof(upnpIfcName));
      }
      else
      {
         strncpy(upnpIfcName, ifcName, sizeof(upnpIfcName));
         cmsLog_debug("Use NONE default gateway NATed ifc %s for upnp.", upnpIfcName);
      }
   }
   else
   {
      strncpy(upnpIfcName, ifcName, sizeof(upnpIfcName));
      cmsLog_debug("Use ifcName %s for upnp if pcpEnable.", upnpIfcName);
   }
   cmsLog_debug("upnpIfcName=%s", upnpIfcName);

   ret = qdmIpIntf_getLayer2IntfNameByLayer3IntfNameLocked(upnpIfcName,
                                                           upnpl2IfcName);
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }  

   ret = rutUpnp_getPcpInfo(upnpIfcName, &pcpMode, pcpServer, pcpLocal);

   if (pcpMode == PCP_MODE_DISABLE)
   {
      snprintf(cmd,  sizeof(cmd), "-L br0 -W %s -W2 %s", upnpIfcName, upnpl2IfcName);
   }
   else
   {
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_debug("fail to get pcp info of %s", upnpIfcName);
      }
      else
      {
         snprintf(cmd,  sizeof(cmd), "-L br0 -W %s -W2 %s -mode %d -pcpsrv %s -pcplocal %s", 
                  upnpIfcName, upnpl2IfcName, pcpMode, pcpServer, pcpLocal);
      }
   }
   
   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_debug("send restart upnp msg to smd, cmd=%s", cmd);

      if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_UPNP, cmd, strlen(cmd)+1)) == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start or restart upnp");
         return CMSRET_INTERNAL_ERROR;
      }

      /* add all upnp iptables rules */
      rutUpnp_configIptableRuleForAllWanIfcs(TRUE);
   }
   
   cmsLog_debug("Exit: ret=%d", ret);

   return ret;
}


CmsRet  rut_stopUpnp(void)
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Enter");
   if (rut_isApplicationRunning(EID_UPNP) == FALSE)
   {
      cmsLog_debug("Upnp has not been started yet.");
      return ret;
   }
   
   if ((rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_UPNP, NULL,0)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to stop upnp.");
      return CMSRET_INTERNAL_ERROR;
   }         

   /* remove all upnp iptables rules */
   rutUpnp_configIptableRuleForAllWanIfcs(FALSE);
   
   return ret;
}


CmsRet rut_restartUpnp(char *deletedIfc)
{
   char upnpIfcName[CMS_IFNAME_LENGTH]={0};
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 pcpEnable = FALSE;
   
   cmsLog_debug("Enter: deletedIfc=%s", deletedIfc);

   if ((ret = rut_stopUpnp()) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to remove upnp process.");
      return ret;         
   }
      
   if (rutWan_findFirstNattedAndConnected(upnpIfcName, deletedIfc) ||
       ((pcpEnable = rut_getPCPInterfaceName(upnpIfcName)) == TRUE))
   {
      cmsLog_debug("upnpIfcName=%s", upnpIfcName);        
      if ((ret = rut_startUpnp(upnpIfcName, deletedIfc, pcpEnable)) != CMSRET_SUCCESS)
      {
         cmsLog_notice("Failed to add upnp process.");
      }
   }
   else
   {
      cmsLog_notice("No Nated wan ifc exsit. Upnp will not be started.");
   }

   cmsLog_debug("Exit: ret = %d", ret);
   
   return ret;
}


/* XXX TODO: consolidate with rut_startUpnp */
CmsRet rut_restartUpnpWithWanIfc(char *ifcName)
{
   char upnpIfcName[CMS_IFNAME_LENGTH]={0};
   char upnpl2IfcName[CMS_IFNAME_LENGTH]={0};
   CmsRet ret = CMSRET_SUCCESS;
   char cmd[BUFLEN_256]={0};
   SINT32 pid;
   char pcpServer[CMS_IPADDR_LENGTH]={0};
   char pcpLocal[CMS_IPADDR_LENGTH]={0};
   SINT32 pcpMode = PCP_MODE_DISABLE;
   
   cmsLog_debug("Enter: ifcName=%s", ifcName);

   if (IS_EMPTY_STRING(ifcName))
   {
      cmsLog_debug("ifcName is NULL");
      return CMSRET_INTERNAL_ERROR;
   }
   
   if ((ret = rut_stopUpnp()) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to remove upnp process.");
      return ret;         
   }

   if (qdmIpIntf_isNatEnabledOnIntfNameLocked(ifcName))
   {
      strncpy(upnpIfcName, ifcName, sizeof(upnpIfcName));   
      cmsLog_debug("Use default gateway %s for upnp.", upnpIfcName);        
   }
   else if (rutWan_findFirstNattedAndConnected(upnpIfcName, NULL))
   {
      cmsLog_debug("Use Non default gateway %s for upnp.", upnpIfcName);        
   }
   else if (rut_getPCPInterfaceName(upnpIfcName))
   {
      cmsLog_debug("Use ifname with PCP %s for upnp.", upnpIfcName);        
   }
   else
   {
      cmsLog_notice("No Nated/PCP wan ifc exsit. Upnp will not be started.");
      return ret;
   }
   
   ret = qdmIpIntf_getLayer2IntfNameByLayer3IntfNameLocked(upnpIfcName, upnpl2IfcName);
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }  

   ret = rutUpnp_getPcpInfo(upnpIfcName, &pcpMode, pcpServer, pcpLocal);

   if (pcpMode == PCP_MODE_DISABLE)
   {
      snprintf(cmd,  sizeof(cmd), "-L br0 -W %s -W2 %s", upnpIfcName, upnpl2IfcName);
   }
   else
   {
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_debug("fail to get pcp info of %s", upnpIfcName);
      }
      else
      {
         snprintf(cmd,  sizeof(cmd), "-L br0 -W %s -W2 %s -mode %d -pcpsrv %s -pcplocal %s", 
                  upnpIfcName, upnpl2IfcName, pcpMode, pcpServer, pcpLocal);
      }
   }

   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_notice("send restart upnp msg to smd (%s)", cmd);

      if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_UPNP, cmd, strlen(cmd)+1)) == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start or restart upnp");
         return CMSRET_INTERNAL_ERROR;
      }         

      /* add all upnp iptables rules */
      rutUpnp_configIptableRuleForAllWanIfcs(TRUE);
   }

   cmsLog_debug("Exit: ret=%d", ret);

   return ret;
}


UBOOL8 rutUpnp_checkRunStatusWithDelay(void)
{
   int i = 0;
   while (i++ < 15)
   {
      /* check UPnP is active */
      if (rut_isApplicationActive(EID_UPNP) == TRUE)
      {
         /* check UPnP is running (well initialized)
          * if not, wait 100 ms for next check. */
         if (rut_isApplicationRunning(EID_UPNP) == TRUE)
            return TRUE;
         else
            usleep(100000); // 100 ms
      }
      else
      {
         /* UPnP is not even activated, so just return false now. */
         return FALSE;
      }
   }
   /* waited 1.5 seconds and UPnP is still not running, give up and return false */
   return FALSE;

}


#ifdef DMP_X_BROADCOM_COM_IPV6_1
UBOOL8 rutUpnp_pcpOfLanAddr( const char *ifName )
{
   UBOOL8 found=FALSE;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   char guAddr[CMS_IPADDR_LENGTH]={0};
   UINT32 prefixLen=0;
   _WanPppConnObject *pppConn = NULL;
   _WanIpConnObject  *ipConn = NULL;
   UBOOL8 pcpEnable=FALSE;

   /* 
    * This function should be only used for DS-Lite tunnel.
    * Criteria to activate DS-Lite tunnel here are"
    * 1. An inactive DS-Lite tunnel associated with the WAN interface.
    * 2. The WAN interface does not get GUA.
    *
    * The WAN interface must be up at this point??
    */
   cmsLog_debug("ifName<%s>", ifName);

   if (cmsNet_getGloballyUniqueIfAddr6(ifName, guAddr, &prefixLen) == CMSRET_SUCCESS)
   {
      cmsLog_debug("found GUA at %s, return", ifName);
      return pcpEnable;
   }

   while (!found &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, ipConn->X_BROADCOM_COM_IfName))
      {
         found = TRUE;

         if (ipConn->X_BROADCOM_COM_PCPMode == PCP_MODE_DSLITE)
         {
            pcpEnable = TRUE;
         }
      }
      cmsObj_free((void **) &ipConn);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, pppConn->X_BROADCOM_COM_IfName))
      {
         found = TRUE;

         if (pppConn->X_BROADCOM_COM_PCPMode == PCP_MODE_DSLITE)
         {
            pcpEnable = TRUE;
         }
      }
      cmsObj_free((void **) &pppConn);
   }

   cmsLog_debug("pcpEnable<%d>", pcpEnable);
   return pcpEnable;
}
#endif  /* DMP_X_BROADCOM_COM_IPV6_1  */

#endif  /* SUPPORT_UPNP */

