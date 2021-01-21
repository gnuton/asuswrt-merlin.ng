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
#include "rut_wan.h"
#include "rut_lan.h"
#include "rut_pmap.h"
#include "rut_route.h"
#include "skb_defines.h"


#ifdef DMP_BASELINE_1
/*
 * Most of the functions in this file are only usable in Legacy TR98 mode
 * or Hybrid TR98+TR181 mode.  There is a few functions in here which
 * can be used in all modes, so look carefully at the ifdef's in the middle
 * of the file.
 */




CmsRet rutRt_getActiveDefaultGateway_igd(char *gwIfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   _L3ForwardingObject *L3ForwadingObj=NULL;
   CmsRet ret;

   cmsLog_debug("Enter");
   
   if (gwIfName == NULL)
   {
      cmsLog_error("NULL string.");
      return CMSRET_INVALID_ARGUMENTS;
   }

   *gwIfName='\0';
   if ((ret = cmsObj_get(MDMOID_L3_FORWARDING, &iidStack, 0, (void **)&L3ForwadingObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get MDMOID_L3_FORWARDING, ret=%d", ret);
      return ret;
   }
   if (IS_EMPTY_STRING(L3ForwadingObj->X_BROADCOM_COM_ActiveDefaultGateway))
   {
      cmsLog_debug("L3ForwadingObj->X_BROADCOM_COM_ActiveDefaultGateway is NULL, No system Default gateway.");
   }
   else
   {
      strcpy(gwIfName, L3ForwadingObj->X_BROADCOM_COM_ActiveDefaultGateway);
   }
   
   cmsObj_free((void **)&L3ForwadingObj);

   cmsLog_debug("Exit. ret=%d, active gwIfName=%s", ret, gwIfName);
   
   return ret;        
    
} 



UBOOL8 rutRt_useGatewayIfcNameOnly_igd(const char *inGwIfc, char *outGwIpAddress)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanIpConnObject *ipConn = NULL;
   UBOOL8 useIfc = TRUE;
   UBOOL8 found = FALSE;
   
   cmsLog_debug("Enter inGwIfc=%s", inGwIfc);
   
   if (outGwIpAddress == NULL)
   {
      cmsLog_error("outGwIpAddress is NULL!");
      return FALSE;
   }

   /* get the related ipConn obj */
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(inGwIfc, ipConn->X_BROADCOM_COM_IfName) == 0)
      {
          /* For IpoA, the interface name will be fixed (ipoa0, ipoa1, etc). */
         if (strstr(ipConn->X_BROADCOM_COM_IfName, IPOA_IFC_STR))
         {
            /* for IPoA, always use ifc */
            useIfc = TRUE;
         }
         else
         {
            /* for IPoE, always use gw Ip address */
            cmsUtl_strcpy(outGwIpAddress, ipConn->defaultGateway);
            useIfc = FALSE;
         }
         found = TRUE;            
      }
     cmsObj_free((void **) &ipConn);
   }

   if (!found)
   {
      LanIpIntfObject *lanIpIntfObj=NULL;

      INIT_INSTANCE_ID_STACK(&iidStack);
      while (!found &&
             (cmsObj_getNextFlags(MDMOID_LAN_IP_INTF, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &lanIpIntfObj)) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(lanIpIntfObj->X_BROADCOM_COM_IfName, inGwIfc) &&
             !cmsUtl_strcmp(lanIpIntfObj->IPInterfaceAddressingType, MDMVS_DHCP))
         {
            cmsUtl_strcpy(outGwIpAddress, lanIpIntfObj->X_BROADCOM_COM_DhcpDefaultGateway);
            useIfc = FALSE;
            found = TRUE;
         }
         cmsObj_free((void **) &lanIpIntfObj);
      }
   }

   cmsLog_debug("Exit. inGwIfc=%s, outGwIpAddress=%s, useIfc=%d, found = %d", 
      inGwIfc, outGwIpAddress, useIfc, found);
   
   return useIfc;

}


#endif  /* DMP_BASELINE_1 */



CmsRet rutRt_configActiveDefaultGateway(const char *gwIfName)
{
   char cmd[BUFLEN_128]={0};
   char gwIpAddress[CMS_IPADDR_LENGTH]={0};
   FILE* errFs = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Entered: gwIfName=%s", gwIfName);


   /* delete default gateway first before add  */
   snprintf(cmd, sizeof(cmd), "route del default 2>/dev/null");  
   rut_doSystemAction("rutRt", cmd);
      
   /* rutRt_useGatewayIfcNameOnly return TRUE = use gwIfName only,
    * else use gwIfName and ip address */
   if (rutRt_useGatewayIfcNameOnly(gwIfName, gwIpAddress))
   {
      snprintf(cmd, sizeof(cmd), "route add default dev %s 2>/var/gwerr", gwIfName);
      rut_doSystemAction("rutRt", cmd);
   }
   else
   {
      if (gwIpAddress[0] != '\0')
      {
         snprintf(cmd, sizeof(cmd), "route add default gw %s dev %s 2>/var/gwerr", gwIpAddress, gwIfName);
         rut_doSystemAction("rutRt", cmd);
      }
      else
      {
         cmsLog_debug("No gateway Ip address found");
         ret = CMSRET_INTERNAL_ERROR;
      }
   }

   errFs = fopen("/var/gwerr", "r");
   if (errFs != NULL ) 
   {
      char errStr[BUFLEN_264]={0};
      char *fgetrv;

      fgetrv = fgets(errStr, sizeof(errStr), errFs);
      if (fgetrv)
      {
         cmsLog_error("Failed to add default gateway, cmd=%s, err=%s", cmd, errStr);
         ret = CMSRET_INTERNAL_ERROR;
      }
      fclose(errFs);
      unlink("/var/gwerr");
   }
   
   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_debug("Active default gateway set! (cmd=%s)", cmd);
   }         

   return ret;
   
}


void rutRt_unconfigActiveDefaultGateway(const char *gwIfName)
{
   if (rutRt_isDefaultGatewayIfNameInRouteTable(gwIfName))
   {
      char cmd[BUFLEN_128];
      cmsLog_debug("%s is currently default gw, remove it!", gwIfName);
      snprintf(cmd, sizeof(cmd), "route del default 2>/dev/null");
      rut_doSystemAction("rutRt", cmd);
   }

   return;
}



#ifdef DMP_BASELINE_1
/* this function is only used in TR98 mode.
 * TR181 equivalent function is rutRt_selectActiveIpvxDefaultGateway_dev2.
 */
void rutRt_fetchActiveDefaultGateway(const char *gatewayList, char *defaultGateway)
{
   UBOOL8 found = FALSE;

   defaultGateway[0] = '\0';
   
   if (gatewayList != NULL)
   {
      char *defaultGatewList, *currDefaultGateway, *ptr, *savePtr=NULL;
      UINT32 count=0;
   
      defaultGatewList = cmsMem_strdup(gatewayList);
      ptr = strtok_r(defaultGatewList, ",", &savePtr);

      /* need to find the a WAN interface is in the list and is CONNECTED */
      while (!found && (ptr != NULL) && (count < CMS_MAX_DEFAULT_GATEWAY))
      {
         currDefaultGateway=ptr;
         while ((isspace(*currDefaultGateway)) && (*currDefaultGateway != 0))
         {
            /* skip white space after comma */
            currDefaultGateway++;
         }         

         if (rut_isWanInterfaceUp(currDefaultGateway, TRUE))
         {
            strcpy(defaultGateway, currDefaultGateway);
            found = TRUE;
         }
         
         count++;
         ptr = strtok_r(NULL, ",", &savePtr);

      }

      cmsMem_free(defaultGatewList);
      
   }

   if (!found)
   {
      /* try to find any connected and routed wan interface as system default gateway */
      found = rutWan_findFirstRoutedAndConnected(defaultGateway);
   }

   if (!found)
   {
      /* try to find a LAN interface configured to do DHCP client */
      found = rutLan_findFirstDhcpcAndConnected(defaultGateway);
   }

   cmsLog_debug("Active defaultGateway = %s", defaultGateway);
   
}

#endif  /* DMP_BASELINE_1 */


UBOOL8 rutRt_isDefaultGatewayIfNameInRouteTable(const char *ifName) 
{
   char col[BUFLEN_16][BUFLEN_32];
   char line[BUFLEN_264];
   struct in_addr addr;
   SINT32 count = 0;
   SINT32  flag = 0;
   FILE* fsRoute = fopen("/proc/net/route", "r");
   UBOOL8 found = FALSE;
   
   if (fsRoute != NULL) 
   {
        while (!found && fgets(line, sizeof(line), fsRoute)) 
        {
            /* Skip the first title line */
            if (count++ < 1) 
            {
               continue;
            }     
            
            sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s",
                col[0], col[1], col[2], col[3], col[4], col[5],
                col[6], col[7], col[8], col[9], col[10]);
            flag = strtol(col[3], (char**)NULL, 16);
            
            if ((flag & (RTF_UP)) == RTF_UP) 
            {
                addr.s_addr = strtoul(col[1], (char**)NULL, 16);
                if (!cmsUtl_strcmp("0.0.0.0", inet_ntoa(addr))) 
                {
                    if (!cmsUtl_strcmp(ifName, col[0]))
                    {
                        found = TRUE;
                    }
                }
            }
        }
        
        fclose(fsRoute);
        
   }

   cmsLog_debug("found=%d of %s in the routing table", found, ifName);
   
   return found;
}


void rutRt_removeIfNameFromList(const char *ifName, char *list)
{
   char *tmpList=NULL;
   char *ptr=NULL;
   char *savePtr=NULL;
   UINT32 count=0;
   UINT32 len=0;

   len = cmsUtl_strlen(list);

   cmsLog_debug("Entered: ifName=%s list(%d)=%s", ifName, len, list);

   if (len == 0)
   {
      /* list is already empty, nothing to do */
      return;
   }

   tmpList = cmsMem_alloc(len+1, ALLOC_ZEROIZE);
   if (tmpList == NULL)
   {
      cmsLog_error("alloc of %d bytes failed", len);
      return;
   }


   ptr = strtok_r(list, ",", &savePtr);

   while ((ptr != NULL) && (count < CMS_MAX_DEFAULT_GATEWAY))
   {
      while ((isspace(*ptr)) && (*ptr != 0))
      {
         /* skip white space after comma */
         ptr++;
      }

      if (*ptr != 0)
      {
         /* Only copy it back if not matched with the deleted WAN ifName */
         if (cmsUtl_strcmp(ifName, ptr))
         {
            if (tmpList[0] != '\0')
            {
               strcat(tmpList, ",");
            }
            strcat(tmpList, ptr);
         }
      }

      count++;
      ptr = strtok_r(NULL, ",", &savePtr);
   }

   /* overwrite the original list with the updated one */
   sprintf(list, "%s", tmpList);
   CMSMEM_FREE_BUF_AND_NULL_PTR(tmpList);

   cmsLog_debug("Exit: list=%s", list);

   return;
}



#ifdef DMP_BASELINE_1

CmsRet rutRt_doSystemDefaultGateway(UBOOL8 isIPv4)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   
   cmsLog_debug("Enter: isIPv4=%d", isIPv4);

   if ( isIPv4 )
   {
      _L3ForwardingObject *L3ForwadingObj=NULL;
      char defaultGateway[CMS_IFNAME_LENGTH]={0};

      if ((ret = cmsObj_get(MDMOID_L3_FORWARDING, &iidStack, 0, (void **)&L3ForwadingObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get MDMOID_L3_FORWARDING, ret=%d", ret);
         return ret;
      }
   
      /* Need to fetch a default gateway */
      rutRt_fetchActiveDefaultGateway(L3ForwadingObj->X_BROADCOM_COM_DefaultConnectionServices, defaultGateway);
   

      /* 2 cases to change the system default gateway: 
      *
      * 1). When current active default gateway in MDM (L3ForwadingObj->X_BROADCOM_COM_ActiveDefaultGateway) differs from
      *     the actual live system defaultGateway (defaultGateway string from rutRt_fetchActiveDefaultGateway).
      *
      * 2). if they are same but the interface is not in the routing table.
      *
      */
      if ((cmsUtl_strcmp(L3ForwadingObj->X_BROADCOM_COM_ActiveDefaultGateway, defaultGateway)) || /* case 1 */
          (!rutRt_isDefaultGatewayIfNameInRouteTable(defaultGateway)))                            /* case 2 */
      {
   
         if (defaultGateway[0] == '\0')
         {
            CMSMEM_FREE_BUF_AND_NULL_PTR(L3ForwadingObj->X_BROADCOM_COM_ActiveDefaultGateway);
         }
         else
         {
            CMSMEM_REPLACE_STRING_FLAGS(L3ForwadingObj->X_BROADCOM_COM_ActiveDefaultGateway, 
               defaultGateway, mdmLibCtx.allocFlags);
         }   
      
         /* set system active default gateway if differs with the previous one */
         if ((ret = cmsObj_set(L3ForwadingObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set <MDMOID_L3_FORWARDING> returns error. ret=%d", ret);
         }   
      }
      
      cmsObj_free((void **) &L3ForwadingObj);
   }
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   else
   {
      _IPv6L3ForwardingObject *IPv6L3ForwadingObj=NULL;

      if ((ret = cmsObj_get(MDMOID_I_PV6_L3_FORWARDING, &iidStack, 0, (void **)&IPv6L3ForwadingObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get MDMOID_I_PV6_L3_FORWARDING, ret=%d", ret);
         return ret;
      }

      /* TODO: We should follow the same logic as IPv4 default gateway */
      if ( !IS_EMPTY_STRING(IPv6L3ForwadingObj->defaultConnectionService) )
      {
         if ((ret = cmsObj_set(IPv6L3ForwadingObj, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set <MDMOID_I_PV6_L3_FORWARDING> returns error. ret=%d", ret);
         }
      }

      cmsObj_free((void **) &IPv6L3ForwadingObj);
   }
#endif
   
   cmsLog_debug("Exit. ret=%d", ret);
   
   return ret;
   
}



void rutRt_removeDefaultGatewayIfUsed_igd(const char* ifName)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   _L3ForwardingObject *L3ForwadingObj=NULL;

   cmsLog_debug("Enter: ifName=%s", ifName);

   if ((ret = cmsObj_get(MDMOID_L3_FORWARDING, &iidStack, 0, (void **)&L3ForwadingObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get MDMOID_L3_FORWARDING, ret=%d", ret);
      return;
   }

   /* delete this ifName from list of potential default gateways */
   rutRt_removeIfNameFromList(ifName, L3ForwadingObj->X_BROADCOM_COM_DefaultConnectionServices);

   /* if this ifName is currently the default gateway, NULL it out.
    * When we do the cmsObj_set, a new one will be selected.
    */
   if (!cmsUtl_strcmp(ifName, L3ForwadingObj->X_BROADCOM_COM_ActiveDefaultGateway))
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(L3ForwadingObj->X_BROADCOM_COM_ActiveDefaultGateway);
   }

   if ((ret = cmsObj_set(L3ForwadingObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_set <MDMOID_L3_FORWARDING> returns error. ret=%d", ret);
   }

   cmsObj_free((void **) &L3ForwadingObj);


#if defined(DMP_X_BROADCOM_COM_IPV6_1)
   {
      InstanceIdStack iidStack1=EMPTY_INSTANCE_ID_STACK;
      _IPv6L3ForwardingObject *ipv6L3ForwadingObj=NULL;

      if ( ret ==CMSRET_SUCCESS )
      {
         if ((ret = cmsObj_get(MDMOID_I_PV6_L3_FORWARDING, &iidStack1, 0, (void **)&ipv6L3ForwadingObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get MDMOID_I_PV6_L3_FORWARDING, ret=%d", ret);
            return;
         }

         if (IS_EMPTY_STRING(ipv6L3ForwadingObj->defaultConnectionService))
         {
            cmsLog_debug("X_BROADCOM_COM_DefaultConnectionServices is NULL - No action.");
         }
         else
         {
            CMSMEM_FREE_BUF_AND_NULL_PTR(ipv6L3ForwadingObj->defaultConnectionService);

            if ((ret = cmsObj_set(ipv6L3ForwadingObj, &iidStack1)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsObj_set <MDMOID_I_PV6_L3_FORWARDING> returns error. ret=%d", ret);
            }      
         }

         cmsObj_free((void **) &ipv6L3ForwadingObj);
      }
   }
#elif defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1)
   {
      InstanceIdStack iidStack1=EMPTY_INSTANCE_ID_STACK;
      _Dev2RouterObject *routerObj=NULL;

      if ( ret ==CMSRET_SUCCESS )
      {
         if ((ret = cmsObj_getNext(MDMOID_DEV2_ROUTER, &iidStack1, (void **)&routerObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get MDMOID_DEV2_ROUTER, ret=%d", ret);
            return;
         }

         rutRt_removeIfNameFromList(ifName, routerObj->X_BROADCOM_COM_DefaultIpv6ConnectionServices);

         if ((ret = cmsObj_set(routerObj, &iidStack1)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_set <MDMOID_DEV2_ROUTER> returns error. ret=%d", ret);
         }

         cmsObj_free((void **) &routerObj);
      }
   }
#endif
   cmsLog_debug("Exit");
   
   return;
}



CmsRet rutRt_activateL3ForwardingEntry(const char* ifName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _L3ForwardingEntryObject *routeCfg = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 done = FALSE;

   cmsLog_debug("Activate static route with interface name %s", ifName);

   while (!done && (ret = cmsObj_getNext
         (MDMOID_L3_FORWARDING_ENTRY, &iidStack, (void **) &routeCfg)) == CMSRET_SUCCESS)
   {
      /* also activate routing entry without interface */
      if( IS_EMPTY_STRING(routeCfg->interface) || !cmsUtl_strcmp(ifName, routeCfg->interface))
      {
         /* set and activate L3ForwardingEntryObject */
         if ((ret = cmsObj_set(routeCfg, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set static route on %s, error=%d", routeCfg->interface, ret);
            done = TRUE;
         }
      }
      cmsObj_free((void **) &routeCfg);
   }

   return ret;
}


CmsRet rutRt_removeL3ForwardingEntry(const char* ifName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _L3ForwardingEntryObject *routeCfg = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 done = FALSE;

   
   cmsLog_debug("Try to remove any static routes associated with %s interface ", ifName);

   while (!done && 
      cmsObj_getNext(MDMOID_L3_FORWARDING_ENTRY, &iidStack, (void **) &routeCfg) == CMSRET_SUCCESS)
   {
      if(!cmsUtl_strcmp(ifName, routeCfg->interface))
      {
         /* Remove and deactivate L3ForwardingEntryObject */
         if ((ret = cmsObj_deleteInstance(MDMOID_L3_FORWARDING_ENTRY, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to delete L3ForwardingEntryObject, ret = %d", ret);
            done = TRUE;
         }
         else
         {
            INIT_INSTANCE_ID_STACK(&iidStack);
            cmsLog_debug("Remove static route %s", routeCfg->destIPAddress);
         }
      }
      cmsObj_free((void **) &routeCfg);
   }

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   {
      InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
      _IPv6L3ForwardingEntryObject *ipv6rtCfg = NULL;

      if (ret == CMSRET_SUCCESS)
      {
         while (!done && 
            cmsObj_getNext (MDMOID_I_PV6_L3_FORWARDING_ENTRY, &iidStack1, (void **) &ipv6rtCfg) == CMSRET_SUCCESS)
         {
            if(!cmsUtl_strcmp(ifName, ipv6rtCfg->interface))
            {

               /* Remove and deactivate L3ForwardingEntryObject */
               if ((ret = cmsObj_deleteInstance(MDMOID_L3_FORWARDING_ENTRY, &iidStack1)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to delete L3ForwardingEntryObject, ret = %d", ret);
                  done = TRUE;
               }
               else
               {
                  INIT_INSTANCE_ID_STACK(&iidStack1);
                  cmsLog_debug("Remove static route %s", ipv6rtCfg->destIPv6Address);
               }
            }
            cmsObj_free((void **) &ipv6rtCfg);
         }

      }
   }
#endif
   return ret;
}

#endif  /* DMP_BASELINE_1 */


CmsRet rutRt_addSystemStaticRoute(const char* DNSServers, 
                                  const char *mask, 
                                  const char *gateway, 
                                  const char *ifName)
{
   CmsRet ret = CMSRET_SUCCESS;
   SINT32 i = 0;   
   char destination[CMS_IPADDR_LENGTH] = {0};

   if (DNSServers == NULL || mask == NULL || gateway == NULL || ifName == NULL)
   {
      cmsLog_error("Invalid parameters");
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* Only use the primary dns - the first one in the list if multiple dns existed with ',' as the seperator */
   while (DNSServers[i] != ',' && i < CMS_IPADDR_LENGTH)
   {
      destination[i] = DNSServers[i];
      i++;
   }     

   ret = rutRt_addStaticRouteAction(destination, mask, gateway, ifName, -1);
   
   return ret;
}


CmsRet rutRt_addStaticRouteAction(const char *destIPAddress,
                                  const char *destSubnetMask,
                                  const char *gatewayIPAddress,
                                  const char *interface,
                                  SINT32 forwardingMetric)
{
   char cmd[BUFLEN_264]={0};
   char str[BUFLEN_264]={0};
   FILE* errFs = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   
   cmsLog_debug("Add static route entry: addr=%s, mask=%s, gtwy=%s, wanif=%s, metric=%d", 
      destIPAddress, destSubnetMask, gatewayIPAddress, interface, forwardingMetric);

   if (cmsUtl_isValidIpAddress(AF_INET, destIPAddress) == FALSE || !cmsUtl_strcmp(destIPAddress, "0.0.0.0"))
   {
      cmsLog_notice("Invalid destination IP address");
      return CMSRET_INVALID_ARGUMENTS;		
   }
   
   if (cmsUtl_isValidIpAddress(AF_INET, destSubnetMask) == FALSE || !cmsUtl_strcmp(destSubnetMask, "0.0.0.0"))
   {
      cmsLog_error("Invalid destination subnet mask");
      return CMSRET_INVALID_ARGUMENTS;		
   }
   
   if ( cmsUtl_strncmp(interface, "ppp", 3) &&  cmsUtl_strncmp(interface, "ipoa", 4) &&
       (cmsUtl_isValidIpAddress(AF_INET, gatewayIPAddress) == FALSE || !cmsUtl_strcmp(gatewayIPAddress, "0.0.0.0")))
   {
      cmsLog_error("Please give correct gateway IP for static route over IPoE interface");
      return CMSRET_INVALID_ARGUMENTS;		
   }


   /*
    * start static route command only if the WAN interface is up 
    * or if it is a LAN side static route.  Assume for now that all
    * lan side bridges are up.  This is needed by the PHY group 
    * to do tests across multiple LAN subnets.
    */
   if (IS_EMPTY_STRING(interface) ||
       strncmp(interface, "br", 2) == 0 ||
       qdmIpIntf_isWanInterfaceUpLocked(interface, TRUE))
   {
      if ( forwardingMetric == -1 )
      {
         sprintf(cmd, "route add -net %s netmask %s ", destIPAddress, destSubnetMask);
      }
      else
      {
         sprintf(cmd, "route add -net %s netmask %s metric %d", destIPAddress, destSubnetMask, forwardingMetric);
      }

      if ( cmsUtl_isValidIpAddress(AF_INET, gatewayIPAddress) == TRUE &&
             cmsUtl_strcmp(gatewayIPAddress, "0.0.0.0") != 0 ) 
      {
         strcat(cmd, " gw ");
         strcat(cmd, gatewayIPAddress);
      }
      if ( !IS_EMPTY_STRING(interface)) 
      {
         strcat(cmd, " dev ");
         strcat(cmd, interface);
      }
      
      strcat(cmd, " 2> /var/addrt");
      rut_doSystemAction("rutRt_addStaticRouteAction", cmd);
      errFs = fopen("/var/addrt", "r");
      if (errFs != NULL ) 
      {
         char *fgetrv;
         fgetrv = fgets(str, sizeof(str), errFs);
         if (fgetrv)
         {
            /* we actually got some kind of output from the cmd,
             * probably not good. */
             cmsLog_debug("got output =>%s<=", str);
         }
         fclose(errFs);
         unlink("/var/addrt");
      }
       /* if error is not the existence of route ("file exists") */
      if ( str[0] != '\0' && strstr(str, "exist") == NULL )
      {
         cmsLog_error("Error = %s", str);
         ret = CMSRET_REQUEST_DENIED;
      }
   }
   else
   {
      cmsLog_debug("Skip adding the route - WAN interface is not up yet.");         
   }

   return ret;
   
}




CmsRet rutRt_deleteStaticRouteAction(const char *destIPAddress,
                                     const char *destSubnetMask,
                                     const char *gatewayIPAddress,
                                     const char *interface)
{
   char cmd[BUFLEN_264];
   CmsRet ret = CMSRET_SUCCESS;
   
   if (destIPAddress == NULL ||  destSubnetMask == NULL)
   {
      cmsLog_error("NULL detIpAddress or sunnet mask?");
      return ret;
   }

   sprintf(cmd, "route del -net %s netmask %s ", destIPAddress, destSubnetMask);
   if ( cmsUtl_isValidIpAddress(AF_INET, gatewayIPAddress) == TRUE &&
      cmsUtl_strcmp(gatewayIPAddress, "0.0.0.0") != 0 ) 
   {
      strcat(cmd, " gw ");
      strcat(cmd, gatewayIPAddress);
   }
   if ( !IS_EMPTY_STRING(interface)) 
   {
      strcat(cmd, " dev ");
      strcat(cmd, interface);
   }
   strcat(cmd, " 2> /var/addrt");
   rut_doSystemAction("rutRt_deleteStaticRouteAction", cmd);

   return ret;
   
}

#ifdef DMP_BASELINE_1
#ifdef SUPPORT_POLICYROUTING
UBOOL8 getActionInfoFromPolicyRoutingRuleName_igd(const char *ruleName, char *srcIfName, char *srcIP, char *gatewayIP, char *outIfName)
{
   L3ForwardingEntryObject *obj = NULL;
   UBOOL8 found = FALSE;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;

   /*  Loop through all policy routing  */
   while (!found && 
      cmsObj_getNextFlags(MDMOID_L3_FORWARDING_ENTRY, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &obj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ruleName, obj->X_BROADCOM_COM_PolicyRoutingName))
      {
         if (cmsUtl_strcmp(obj->X_BROADCOM_COM_SourceIfName, ""))
         {
            strcpy(srcIfName, obj->X_BROADCOM_COM_SourceIfName);
         }            
         if (cmsUtl_strcmp(obj->sourceIPAddress, ""))
         {
            strcpy(srcIP, obj->sourceIPAddress);
         }
         if (cmsUtl_strcmp(obj->gatewayIPAddress, ""))
         {
            strcpy(gatewayIP, obj->gatewayIPAddress);
         }            
         if (cmsUtl_strcmp(obj->interface, ""))
         {
            strcpy(outIfName, obj->interface);
         }           
         
         found = TRUE;
      }
      cmsObj_free((void **) &obj);
   }
   
   return found;
}
#endif /* SUPPORT_POLICYROUTING */


#ifdef DMP_BRIDGING_1
UBOOL8 getActionInfoFromPortMappingRuleName_igd(const char *ruleName, char *srcIfName, char *srcIP, char *gatewayIP, char *outIfName)
{
   _WanPppConnObject *pppCon=NULL;   
   _WanIpConnObject *ipCon=NULL;
   char groupName[BUFLEN_32]={0};
   char bridgeName[BUFLEN_16]={0};   
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;
   
   /* Loop through all routed interface group entries */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
      cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppCon) == CMSRET_SUCCESS)
   {
      /* Need find out if  this WAN service is part of the interface group */
      if (rutPMap_isWanUsedForIntfGroup(pppCon->X_BROADCOM_COM_IfName))
      {
         if (rutPMap_getGroupAndBridgeNameFromWanIfName(pppCon->X_BROADCOM_COM_IfName, groupName, bridgeName)
            != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get group/bridge name from wanIfname %s", pppCon->X_BROADCOM_COM_IfName);
            cmsObj_free((void **)&pppCon);
            return found;
         }
         else
         {        
            if (!cmsUtl_strcmp(groupName, ruleName))
            {
               strcpy(srcIfName, bridgeName);
               strcpy(srcIP, "");
               strcpy(gatewayIP, pppCon->X_BROADCOM_COM_DefaultGateway);
               strcpy(outIfName, pppCon->X_BROADCOM_COM_IfName);
               found = TRUE;
            }
         }
      }
      cmsObj_free((void **)&pppCon);
   }
 
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
      cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipCon) == CMSRET_SUCCESS)
   {
      /* Need find out if  this WAN service is part of the interface group */
      if (rutPMap_isWanUsedForIntfGroup(ipCon->X_BROADCOM_COM_IfName))
      {
         if (rutPMap_getGroupAndBridgeNameFromWanIfName(ipCon->X_BROADCOM_COM_IfName, groupName, bridgeName) 
            != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get group/bridge name from wanIfname %s", ipCon->X_BROADCOM_COM_IfName);
            cmsObj_free((void **)&ipCon);
            return found;
         }
         else
        {   
            if (!cmsUtl_strcmp(groupName, ruleName))
            {
               strcpy(srcIfName, bridgeName);
               strcpy(srcIP, "");
               strcpy(gatewayIP, ipCon->defaultGateway);
               strcpy(outIfName, ipCon->X_BROADCOM_COM_IfName);
               found = TRUE;
            }
         }
      }
      cmsObj_free((void **)&ipCon);
   }

   return found;   
}
#endif /* DMP_BRIDGING_1 */

#endif /* DMP_BASELINE_1 */

#if defined(SUPPORT_POLICYROUTING) || defined(DMP_BRIDGING_1) || defined(DMP_DEVICE2_BRIDGE_1)

static UBOOL8 isNewIPRoute2( void )
{
   UBOOL8 ret = FALSE;
   char line[BUFLEN_128];
   FILE* fs;
   char cmdStr[BUFLEN_128];   
   
   /* if /var/iproute2/ directory does not exist then create it */
   if (access(POLICY_ROUTING_TABLE_FOLDER,  F_OK) !=  0)
   {
      snprintf(cmdStr, sizeof(cmdStr), "mkdir -p %s 2> /dev/null", POLICY_ROUTING_TABLE_FOLDER);
      rut_doSystemAction("rut_pr", cmdStr);
   }

   snprintf(cmdStr, sizeof(cmdStr), "ip -V > %s", POLICY_ROUTING_VERSION_FILE);
   rut_doSystemAction("rut_pr", cmdStr);

   if ((fs = fopen(POLICY_ROUTING_VERSION_FILE, "r")) == NULL)
   {  
      cmsLog_debug("No version exist yet.");
   }
   else
   {
      if( fgets(line, sizeof(line), fs))
         cmsLog_debug("iproute2 version: %s", line);

      if (!cmsUtl_strstr(line, IPROUTE2_OLD_VERSION))
      {
         ret = TRUE;
      }

      fclose(fs);
   }

   cmsLog_debug("new iproute2: %d", ret);
   return ret;
}

void policyRoutingAction( const char *prefixedRuleName, const char *srcIfName,
                              const char *srcIP, const char *gatewayIP,
                              const char *outIfName, SINT16 idx, UBOOL8 add )
{
   char eb_cmd[BUFLEN_256]={0};
   char ip_rule_cmd[BUFLEN_128]={0};
   char ip_ro_cmd[BUFLEN_128]={0};
   char ip_ro_subnet_cmd[BUFLEN_512]={0};
   char wan_subnet[BUFLEN_128]={0};
   char wan_netmask[BUFLEN_32];
   char num[BUFLEN_16];
   UINT32 flowId = 0;
   struct in_addr netmask_int;
   int    netmask_bits;
   UBOOL8 newIPRoute2 = isNewIPRoute2();

   cmsLog_debug("Add=%d, prefixedRuleName=%s, srcIfName=%s, srcIP=%s, gatewayIP=%s, outIfName=%s, idx=%d", 
                add, prefixedRuleName, srcIfName, srcIP, gatewayIP, outIfName,  idx);

   /* ebtables configuration */
   flowId = SKBMARK_SET_POLICY_RTNG(flowId, (idx + 1));
   snprintf(num, sizeof(num), "0x%x", flowId);
   /*Add policy routing rules before ip qos rule */
   if (add)
   	/* ebtables will complain no rule nr if not specify any */
      snprintf(eb_cmd, sizeof( eb_cmd), "ebtables -t broute -I BROUTING 1 ");
   else
      snprintf(eb_cmd, sizeof( eb_cmd), "ebtables -t broute -D BROUTING ");

   if ( cmsUtl_strcmp(srcIfName, "") )
   {
      if (strstr(srcIfName, "br") == NULL)
      {
         strncat(eb_cmd, " -i ", sizeof(eb_cmd)-1);
      }
      else
      {
         strncat(eb_cmd, " --logical-in ", sizeof(eb_cmd)-1);
      }
      strncat(eb_cmd, srcIfName, sizeof(eb_cmd)-strlen(eb_cmd)-1);
   }

   if ( cmsUtl_strcmp(srcIP, "") )
   {
      strncat(eb_cmd, " -p ipv4 --ip-src ", sizeof(eb_cmd)-strlen(eb_cmd)-1);
      strncat(eb_cmd, srcIP, sizeof(eb_cmd)-strlen(eb_cmd)-1);
   }  

   strncat(eb_cmd, " -j mark --mark-or ", sizeof(eb_cmd)-strlen(eb_cmd)-1);
   strncat(eb_cmd, num, sizeof(eb_cmd)-strlen(eb_cmd)-1);

   strncat(eb_cmd, " --mark-target CONTINUE ", sizeof(eb_cmd)-strlen(eb_cmd)-1);

   /* ip rule configuration */
   snprintf(ip_rule_cmd, sizeof( ip_rule_cmd), "ip rule %s fwmark ", add?"add":"del");  
   strncat(ip_rule_cmd, num, sizeof(ip_rule_cmd)-strlen(ip_rule_cmd)-1);
   snprintf(num, sizeof( num), "/0x%x ", SKBMARK_POLICY_RTNG_M);  
   strncat(ip_rule_cmd, num, sizeof(ip_rule_cmd)-strlen(ip_rule_cmd)-1);

   strncat(ip_rule_cmd, " table ", sizeof(ip_rule_cmd)-strlen(ip_rule_cmd)-1);

   if (newIPRoute2)
   {
      char id[BUFLEN_8];
      snprintf(id, sizeof(id), "%d ", RT_TABLE_BASE+idx);  
      cmsUtl_strncat(ip_rule_cmd, BUFLEN_128-strlen(ip_rule_cmd)-1, id);
   }
   else
   {
      cmsUtl_strncat(ip_rule_cmd, BUFLEN_128-strlen(ip_rule_cmd)-1, prefixedRuleName);
   }

   /* ip route configuration */
   snprintf(ip_ro_cmd, sizeof( ip_ro_cmd), "ip route %s default ", add?"add":"del");  


   /* ip route for wan subnet */
   rut_getIfSubnet(outIfName,wan_subnet);
   rut_getIfMask(outIfName,wan_netmask);
   inet_aton(wan_netmask, &netmask_int);
   netmask_bits = __builtin_popcount(netmask_int.s_addr);

   snprintf(ip_ro_subnet_cmd, sizeof( ip_ro_subnet_cmd), "ip route %s %s/%d ", add?"add":"del",wan_subnet,netmask_bits);

   if ( cmsUtl_strcmp(gatewayIP, "") )
   {
      strncat(ip_ro_cmd, " via ", sizeof(ip_ro_cmd)-strlen(ip_ro_cmd)-1);
      strncat(ip_ro_cmd, gatewayIP, sizeof(ip_ro_cmd)-strlen(ip_ro_cmd)-1);
   }


   strncat(ip_ro_cmd, " dev ", sizeof(ip_ro_cmd)-strlen(ip_ro_cmd)-1);
   strncat(ip_ro_cmd, outIfName, sizeof(ip_ro_cmd)-strlen(ip_ro_cmd)-1);

   strncat(ip_ro_subnet_cmd, " dev ", sizeof(ip_ro_subnet_cmd)-strlen(ip_ro_subnet_cmd)-1);
   strncat(ip_ro_subnet_cmd, outIfName, sizeof(ip_ro_subnet_cmd)-strlen(ip_ro_subnet_cmd)-1);

   strncat(ip_ro_cmd, " table ", sizeof(ip_ro_cmd)-strlen(ip_ro_cmd)-1);
   if (newIPRoute2)
   {
      char id[BUFLEN_8];
      snprintf(id, sizeof(id), "%d ", RT_TABLE_BASE+idx);  
      cmsUtl_strncat(ip_ro_cmd, BUFLEN_128-strlen(ip_ro_cmd)-1, id);
   }
   else
   {
      cmsUtl_strncat(ip_ro_cmd, BUFLEN_128-strlen(ip_ro_cmd)-1, prefixedRuleName);
   }

   strncat(ip_ro_subnet_cmd, " table ", sizeof(ip_ro_subnet_cmd)-strlen(ip_ro_subnet_cmd)-1);
   if (newIPRoute2)
   {
      char id[BUFLEN_8];
      snprintf(id, sizeof(id), "%d ", RT_TABLE_BASE+idx);  
      cmsUtl_strncat(ip_ro_subnet_cmd, BUFLEN_128-strlen(ip_ro_subnet_cmd)-1, id);
   }
   else
   {
      cmsUtl_strncat(ip_ro_subnet_cmd, BUFLEN_128-strlen(ip_ro_subnet_cmd)-1, prefixedRuleName);
   }

   /* Disable rp_filter on policy route interface for avoid block reverse packet */
   if(add)
   {
       char rp_cmd[BUFLEN_128]={0};
       snprintf(rp_cmd, sizeof(rp_cmd), "echo '0' > /proc/sys/net/ipv4/conf/%s/rp_filter", outIfName);
       rut_doSystemAction("rut_pr", rp_cmd);
   }

   rut_doSystemAction("rut_pr", eb_cmd);
   rut_doSystemAction("rut_pr", ip_rule_cmd);
   rut_doSystemAction("rut_pr", ip_ro_cmd);
   rut_doSystemAction("rut_pr", ip_ro_subnet_cmd);

}


static UBOOL8 getActionInfoFromRuleName(const char *prefixedRuleName,
                                        char *srcIfName,
                                        char *srcIP, 
                                        char *gatewayIP,
                                        char *outIfName)
{
   UBOOL8 found = FALSE;
   UBOOL8 prevHideObjectsPendingDelete;
   char ruleName[BUFLEN_16]={0};
   UBOOL8 fromPolicyRoute = TRUE;

   if (prefixedRuleName == NULL || srcIfName == NULL || srcIP == NULL || gatewayIP == NULL || outIfName == NULL)
   {
      cmsLog_error("Illegal parameters");
      return found;
   }

   srcIfName[0] = srcIP[0] = gatewayIP[0] = outIfName[0] = '\0';
   
   fromPolicyRoute = !cmsUtl_strncmp(prefixedRuleName, POLICY_ROUTING_PREFIX, strlen(POLICY_ROUTING_PREFIX));

   /* get rid of prefix  (same size for both policy route and interface group) */
   cmsUtl_strncpy(ruleName, &prefixedRuleName[strlen(POLICY_ROUTING_PREFIX)], sizeof(ruleName)-1);
   
   prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
   mdmLibCtx.hideObjectsPendingDelete = FALSE;

   if (fromPolicyRoute)
   {
#ifdef SUPPORT_POLICYROUTING
      found = getActionInfoFromPolicyRoutingRuleName(ruleName, srcIfName, srcIP, gatewayIP, outIfName);
#endif
   }      
#if defined(DMP_BRIDGING_1) || defined(DMP_DEVICE2_BRIDGE_1) /* aka SUPPORT_PORT_MAP */
   else /* for interface group */   
   {
      found = getActionInfoFromPortMappingRuleName(ruleName, srcIfName, srcIP, gatewayIP, outIfName);
   }
#endif

   if (!found)
   {
      cmsLog_error("Failed to find action info for %s", ruleName);
   }
   
   mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;

   return found;
   
}


CmsRet rutRt_addPolicyRouting(UBOOL8 fromPolicyRoute,
                              const char *ruleName,
                              const char *srcIfName,
                              const char *srcIP, 
                              const char *gatewayIP,
                              const char *outIfName)
{
   SINT32 newIndex=0;
   char line[BUFLEN_128];
   FILE* fs;
   char prefixedRuleName[BUFLEN_32];
   CmsRet ret = CMSRET_SUCCESS;
   
   /* if /var/iproute2/ directory does not exist then create it */
   if (access(POLICY_ROUTING_TABLE_FOLDER,  F_OK) !=  0)
   {
      char cmdStr[BUFLEN_64];   
      snprintf(cmdStr, sizeof(cmdStr), "mkdir -p %s 2> /dev/null", POLICY_ROUTING_TABLE_FOLDER);
      rut_doSystemAction("rut_pr", cmdStr);
   }
      
   /* If the file exists, need to get the next index to append the new rule. */
   if ((fs = fopen(POLICY_ROUTING_TABLE_FILE, "r")) == NULL)
   {  
      cmsLog_debug("No %s exist yet. ", POLICY_ROUTING_TABLE_FILE);
   }
   else
   {
      while((newIndex < RT_TABLE_MAX) && fgets(line, sizeof(line), fs))
      {
         newIndex++;
      }

      fclose(fs);

      if (newIndex >= RT_TABLE_MAX)
      {
         cmsLog_error("Entry count %d is over %d. Corrupted %s ?", newIndex, RT_TABLE_MAX, POLICY_ROUTING_TABLE_FILE);
         return CMSRET_INTERNAL_ERROR;
      }
   }

   /* The rule name is prefix with "pr_" for policy route and  "ig_" for routed interface group in the 
   * "/var/iproute2/rt_tables" file.  This prefix is used to prevent the same rule name and is used only 
   * in rt_tables file
   */
   strcpy(prefixedRuleName, (fromPolicyRoute ? POLICY_ROUTING_PREFIX : INTERFACE_GROUP_PREFIX));
   strncat(prefixedRuleName, ruleName, sizeof(prefixedRuleName)-1);

   /* Need to open for append.  The new rule is always added at the end of the file */
   if ((fs = fopen(POLICY_ROUTING_TABLE_FILE, "a")) == NULL)
   {  
      cmsLog_error("Failed to open %s to append policy routing info", POLICY_ROUTING_TABLE_FILE);
      return CMSRET_OPEN_FILE_ERROR;
   }
   else
   {
      snprintf(line, sizeof(line), "%d %s\n", RT_TABLE_BASE+newIndex, prefixedRuleName);
      fputs(line, fs);
      fclose(fs);
   }

   /* Do  Add rule action */
   policyRoutingAction(prefixedRuleName, srcIfName, srcIP, gatewayIP, outIfName, newIndex, TRUE);
   
   return ret;
}

CmsRet rutRt_deletePolicyRouting(UBOOL8 fromPolicyRoute, const char *delRuleName)
{
   FILE* fs;
   SINT32 oldIndex = 0;
   SINT32 entryCt = 0;
   UBOOL8 found = FALSE;
   char line[BUFLEN_128];
   char prefixedDelRuleName[BUFLEN_32];
   char indexWithBaseStr[BUFLEN_16]={0};
   char prefixedEntryRuleName[BUFLEN_32]={0};
   char srcIfName[CMS_IFNAME_LENGTH]={0};
   char srcIP[CMS_IPADDR_LENGTH]={0};
   char gatewayIP[CMS_IPADDR_LENGTH]={0};
   char outIfName[CMS_IFNAME_LENGTH]={0};
   char entryToAddBackBuf[RT_TABLE_MAX][BUFLEN_32];
   CmsRet ret = CMSRET_SUCCESS;
   SINT32 i = 0;

   if (!fromPolicyRoute && !cmsUtl_strcmp("Default", delRuleName))
   {
      cmsLog_notice("Skip Default group");
      return ret;
   }
   if ((fs = fopen(POLICY_ROUTING_TABLE_FILE, "r")) == NULL)
   {  
      cmsLog_notice("Failed to open %s for read.", POLICY_ROUTING_TABLE_FILE);
      return CMSRET_OPEN_FILE_ERROR;
   }


   /* The rule name is prefix with "pr_" for policy route and  "ig_" for routed interface group in the 
   * "/var/iproute2/rt_tables" file.  This prefix is used to prevent the same rule name and is used only 
   * in rt_tables file; not in the mdm.
   */
   strcpy(prefixedDelRuleName, (fromPolicyRoute ? POLICY_ROUTING_PREFIX : INTERFACE_GROUP_PREFIX));
   strncat(prefixedDelRuleName, delRuleName, sizeof(prefixedDelRuleName)-1);

   /* Need to find the index (in oldIndex) of the rule to be delete from the the file and
   * save other rules in the entryToAddBackBuf for adding them back later on
   */
   while((entryCt  < RT_TABLE_MAX)  && fgets(line, sizeof(line), fs))
   {
      sscanf(line, "%s %s", indexWithBaseStr, prefixedEntryRuleName);
      if (!cmsUtl_strcmp(prefixedEntryRuleName, prefixedDelRuleName))
      {
         found = TRUE;
         /* get the index of rule to be deleted*/
         oldIndex = atoi(indexWithBaseStr) - RT_TABLE_BASE;      
      }
      else
      {
         /* save the entry into the buffer which will be used below */
         cmsUtl_strncpy(entryToAddBackBuf[entryCt], line, BUFLEN_32-1);
         entryCt++;
      }         
   }
   
   fclose(fs);

   if (entryCt >= RT_TABLE_MAX)
   {
      cmsLog_error("corrupted %s ?", POLICY_ROUTING_TABLE_FILE);
      return CMSRET_INTERNAL_ERROR;
   }
   
   if (!found && !fromPolicyRoute)
   {
      cmsLog_notice("Failed to find the rule %s in %s.  Could be a bridge WAN in the group and thus no policy route.", 
         prefixedDelRuleName, POLICY_ROUTING_TABLE_FILE);
      return ret;
   }

   if (!getActionInfoFromRuleName(prefixedDelRuleName, srcIfName, srcIP, gatewayIP, outIfName))
   {
      return CMSRET_INTERNAL_ERROR;
   }
   
   /* remove this rule first, action only */
   policyRoutingAction(prefixedDelRuleName, srcIfName, srcIP, gatewayIP, outIfName, oldIndex, FALSE);

  
   /* Need to remove rest of rules;  action only. */
   for (i = 0; i < entryCt; i++)
   {
      sscanf(entryToAddBackBuf[i], "%s %s", indexWithBaseStr, prefixedEntryRuleName);
      if (!getActionInfoFromRuleName(prefixedEntryRuleName, srcIfName, srcIP, gatewayIP, outIfName))
      {
         return CMSRET_INTERNAL_ERROR;
      }
      
      /* get the index of rule to be deleted*/
      oldIndex = atoi(indexWithBaseStr) - RT_TABLE_BASE;      
   
      /* remove this prefixedEntryRuleName; action only */
      policyRoutingAction(prefixedEntryRuleName, srcIfName, srcIP, gatewayIP, outIfName, oldIndex, FALSE);
   }

   /* remove the old POLICY_ROUTING_TABLE_FILE since rutRt_addPolicyRouting will create a
   * new file with rule index in order in the next for loop
   */
   unlink(POLICY_ROUTING_TABLE_FILE);
   
   /* Now need to add them back in order with the info in entryToAddBackBuf */
   for (i = 0; i < entryCt; i++)
   {
      UBOOL8 entryFromPolicyRoute;
      
      sscanf(entryToAddBackBuf[i], "%s %s", indexWithBaseStr, prefixedEntryRuleName);
      if (!getActionInfoFromRuleName(prefixedEntryRuleName, srcIfName, srcIP, gatewayIP, outIfName))
      {
         return CMSRET_INTERNAL_ERROR;
      }

      /* If the output interface (layer 3 wan interface here) is not up, skip adding the rule back */
      if (!qdmIpIntf_isWanInterfaceUpLocked(outIfName, TRUE))
      {
         return CMSRET_SUCCESS;
      }

      /* rutRt_addPolicyRouting will update the /var/iproute2/rt_tables file
      * and do the action with correct index order. NOTE: Need to pass in non prefixed rule name since rutRt_addPolicyRouting will
      * add the prefix to the rule name
      */
      entryFromPolicyRoute = !cmsUtl_strncmp(prefixedEntryRuleName, POLICY_ROUTING_PREFIX, strlen(POLICY_ROUTING_PREFIX));
      ret = rutRt_addPolicyRouting(entryFromPolicyRoute, 
                                   &prefixedEntryRuleName[strlen(POLICY_ROUTING_PREFIX)], 
                                   srcIfName, 
                                   srcIP, 
                                   gatewayIP, 
                                   outIfName);
      if (ret != CMSRET_SUCCESS)
      {
         return ret;
      }
   }

   return ret;
   
}
#endif /* defined(SUPPORT_POLICYROUTING) || defined(DMP_BRIDGING_1) */

