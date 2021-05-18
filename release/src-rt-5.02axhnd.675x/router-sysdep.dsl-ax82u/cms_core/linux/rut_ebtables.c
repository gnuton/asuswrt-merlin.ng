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
#include <linux/if_ether.h>

#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_ebtables.h"

#ifdef DMP_X_BROADCOM_COM_SECURITY_1
static void rutEbt_executeMacFilterRule(char *cmd, char *cmd2, const char *policy);
#endif

#ifdef DMP_BASELINE_1
/*
 * The next few functions look at the TR98 data model, so can only be used
 * in Legacy TR98 and Hybrid TR98+TR181 modes.
 */
UBOOL8 rut_isBridgedWanExisted()
{
   UBOOL8 exist = FALSE;
   CmsRet ret;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanIpConnObject *wanIpConn = NULL;

   while ((ret = cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &wanIpConn)) == CMSRET_SUCCESS)
   {
      if (strcmp(wanIpConn->connectionType, MDMVS_IP_BRIDGED) == 0)
      {
         exist = TRUE;
         cmsObj_free((void **) &wanIpConn);
         break;
      }
      cmsObj_free((void **) &wanIpConn);
   }
   
   return exist;
}

UBOOL8 rut_isRoutedWanExisted()
{
   UBOOL8 exist = FALSE;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
   _WanPppConnObject *wan_ppp_con = NULL;
   _WanIpConnObject *wan_ip_con = NULL;

   while (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack1, OGF_NO_VALUE_UPDATE, (void **) &wan_ip_con) == CMSRET_SUCCESS)
   {
      if ( strcmp(wan_ip_con->connectionType, MDMVS_IP_BRIDGED) != 0 ) 
      {
         cmsObj_free((void **) &wan_ip_con);    
         return TRUE;
      }

      cmsObj_free((void **) &wan_ip_con);        
   }

   if ( cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack2, OGF_NO_VALUE_UPDATE, (void **) &wan_ppp_con) == CMSRET_SUCCESS ) 
   {
      cmsObj_free((void **) &wan_ppp_con);
      return TRUE;
   }

    return exist;
}

#endif  /* DMP_BASELINE_1 */


void rut_accessTimeRestriction(const _AccessTimeRestrictionObject *Obj, const UBOOL8 add)
{
   char eb_options[BUFLEN_1024];
   char cmd[BUFLEN_1024+BUFLEN_32];

   eb_options[0] = cmd[0] = '\0';
   
   cmsLog_debug("setting info: %s/%s/%s/%s/%s with add = %d", Obj->username, Obj->days, Obj->startTime, Obj->endTime, Obj->MACAddress, add);
   sprintf(eb_options, "--timestart %s --timestop %s -s %s --days %s", Obj->startTime, Obj->endTime, Obj->MACAddress, Obj->days);  

   strncat( eb_options, " -j DROP", sizeof( eb_options )-1 );

   /* This needs to be BridgeMode */
   if ( qdmIpIntf_isBridgedWanExistedLocked() )
   {
      sprintf( cmd, "ebtables -%c FORWARD %c %s", add?'I':'D', add?'1':' ', eb_options );     
      rut_doSystemAction( "rut_accessTimeRestriction", cmd );
   }
   
   if ( qdmIpIntf_isRoutedWanExistedLocked() )
   {
      sprintf( cmd, "ebtables -%c INPUT %c %s", add?'I':'D', add?'1':' ', eb_options );   
      rut_doSystemAction( "rut_accessTimeRestriction", cmd );
   }

}


void rutEbt_avoidDhcpAcceleration(void)
{
   char cmd[BUFLEN_256];

   snprintf(cmd, sizeof(cmd), "ebtables -D FORWARD -p ip --ip-protocol 17 --ip-destination-port 68 -j SKIPLOG 2>/dev/null");
   rut_doSystemAction("rutEbt_avoidDhcpAcceleration", cmd);

   snprintf(cmd, sizeof(cmd), "ebtables -A FORWARD -p ip --ip-protocol 17 --ip-destination-port 68 -j SKIPLOG");
   rut_doSystemAction("rutEbt_avoidDhcpAcceleration", cmd);

   snprintf(cmd, sizeof(cmd), "ebtables -D FORWARD -p ip --ip-destination 255.255.255.255 -j SKIPLOG 2>/dev/null");
   rut_doSystemAction("rutEbt_avoidDhcpAcceleration", cmd);

   snprintf(cmd, sizeof(cmd), "ebtables -A FORWARD -p ip --ip-destination 255.255.255.255 -j SKIPLOG");
   rut_doSystemAction("rutEbt_avoidDhcpAcceleration", cmd);

   return;
}


#ifdef DMP_X_BROADCOM_COM_SECURITY_1
void rutEbt_changeMacFilterPolicy(const char *ifName, UBOOL8 isForward)
{
   char cmd[BUFLEN_256];

   /* If the policy is BLOCKED, we need to block all traffic of the interface */
   if (!isForward)
   {
      snprintf(cmd, sizeof(cmd), "ebtables -D FORWARD -i %s -j DROP 2>/dev/null", ifName);
      rut_doSystemAction("rutEbt_changeMacFilterPolicy", cmd);
      snprintf(cmd, sizeof(cmd), "ebtables -A FORWARD -i %s -j DROP", ifName);
      rut_doSystemAction("rutEbt_changeMacFilterPolicy", cmd);
      snprintf(cmd, sizeof(cmd), "ebtables -D FORWARD -o %s -j DROP 2>/dev/null", ifName);
      rut_doSystemAction("rutEbt_changeMacFilterPolicy", cmd);
      snprintf(cmd, sizeof(cmd), "ebtables -A FORWARD -o %s -j DROP", ifName);
      rut_doSystemAction("rutEbt_changeMacFilterPolicy", cmd);
   }
   else
   {
      snprintf(cmd, sizeof(cmd), "ebtables -D FORWARD -i %s -j DROP 2>/dev/null", ifName);
      rut_doSystemAction("rutEbt_changeMacFilterPolicy", cmd);
      snprintf(cmd, sizeof(cmd), "ebtables -D FORWARD -o %s -j DROP 2>/dev/null", ifName);
      rut_doSystemAction("rutEbt_changeMacFilterPolicy", cmd);
   }
}

void rutEbt_addMacFilter_raw(char* protocol,char* direction ,char* sourceMAC
                   ,char* destinationMAC,const char *ifName, const char *policy, UBOOL8 add) 
{
   char action = 'A', rulenum = ' ';
   char cmd[BUFLEN_128], cmd2[BUFLEN_128], cmdStr[BUFLEN_128];
   char directionStr[BUFLEN_128];

   if ( add ) 
   {
      action = 'I';
      rulenum = '1';
   }
   else
      action = 'D';

   cmd[0] = cmd2[0] = directionStr[0] = cmdStr[0] = '\0';   
      
   if (!cmsUtl_strcmp(protocol, MDMVS_NONE))
   {
      sprintf(cmd, "ebtables -%c FORWARD %c ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_PPPOE))
   {
      sprintf(cmd, "ebtables -%c FORWARD %c -p PPP_DISC ", action, rulenum);
      sprintf(cmd2, "ebtables -%c FORWARD %c -p PPP_SES ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_IPV4))
   {
      sprintf(cmd, "ebtables -%c FORWARD %c -p ARP ", action, rulenum);
      sprintf(cmd2, "ebtables -%c FORWARD %c -p IPv4 ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_IPV6))
   {
      sprintf(cmd, "ebtables -%c FORWARD %c -p ARP ", action, rulenum);
      sprintf(cmd2, "ebtables -%c FORWARD %c -p IPv6 ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_APPLETALK))
   {
      sprintf(cmd, "ebtables -%c FORWARD %c -p ATALK ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_IPX))
   {
      sprintf(cmd, "ebtables -%c FORWARD %c -p IPX ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_NETBEUI))
   {
      sprintf(cmd, "ebtables -%c FORWARD %c -p NetBEUI ", action, rulenum);
   }
   else if (!cmsUtl_strcmp(protocol, MDMVS_IGMP))
   {
      sprintf(cmd, "ebtables -%c FORWARD %c -p IPv4 --ip-proto 2 ", action, rulenum);
   }

   if (cmsUtl_strcmp(destinationMAC, "\0")) 
   {
      strcat(cmd, "-d ");
      strcat(cmd, destinationMAC);
      strcat(cmd, " ");
      if (cmd2[0] != '\0') 
      {
         strcat(cmd2, "-d ");
         strcat(cmd2, destinationMAC);
         strcat(cmd2, " ");
      }
   }
   
   if (cmsUtl_strcmp(sourceMAC, "\0")) 
   {
      strcat(cmd, "-s ");
      strcat(cmd, sourceMAC);
      strcat(cmd, " ");
      if (cmd2[0] != '\0') 
      {
         strcat(cmd2, "-s ");
         strcat(cmd2, sourceMAC);
         strcat(cmd2, " ");
      }
   }

   if (!cmsUtl_strcmp(direction, MDMVS_LAN_TO_WAN))
   {
      strcpy(directionStr, "-o ");
   }
   else if (!cmsUtl_strcmp(direction, MDMVS_WAN_TO_LAN))
   {
      strcpy(directionStr, "-i ");
   }
   else if (!cmsUtl_strcmp(direction, MDMVS_BOTH))
   {
      char cmdSave[BUFLEN_128], cmd2Save[BUFLEN_128];
      strcpy(cmdSave, cmd);
      strcpy(cmd2Save, cmd2);
      strcat(cmd, "-o ");
      strcat(cmd, ifName);
      if (cmd2[0] != '\0') 
      {
         strcat(cmd2, "-o ");
         strcat(cmd2, ifName);
      }
      rutEbt_executeMacFilterRule(cmd, cmd2, policy);
      strcat(cmdSave, "-i ");
      strcat(cmdSave, ifName);
      if (cmd2Save[0] != '\0') 
      {
         strcat(cmd2Save, "-i ");
         strcat(cmd2Save, ifName);
      }
      rutEbt_executeMacFilterRule(cmdSave, cmd2Save, policy);
      return;
   }

   strcat(cmd, directionStr);
   if (cmd2[0] != '\0')
      strcat(cmd2, directionStr);

   strcat(cmd, ifName);
   if (cmd2[0] != '\0')
      strcat(cmd2, ifName);
   rutEbt_executeMacFilterRule(cmd, cmd2, policy);
}


void rutEbt_addMacFilter(const _MacFilterCfgObject *InObj, const char *ifName, const char *policy, UBOOL8 add) 
{
   rutEbt_addMacFilter_raw(
       InObj->protocol
      ,InObj->direction
      ,InObj->sourceMAC
      ,InObj->destinationMAC
      ,ifName, policy, add);
}


void rutEbt_executeMacFilterRule(char *cmd, char *cmd2, const char *policy)
{
   char policyStr[BUFLEN_264];

   if ( cmsUtl_strcmp(policy, MDMVS_FORWARD) == 0 )
      strcpy(policyStr, " -j DROP");
   else
      strcpy(policyStr, " -j ACCEPT");

   strcat(cmd, policyStr);
   rut_doSystemAction("rutEbt_executeMacFilterRule", cmd);
   if (cmd2[0] != '\0') 
   {
      strcat(cmd2, policyStr);  
      rut_doSystemAction("rutEbt_executeMacFilterRule", cmd2);
   }
}

#endif  /* DMP_X_BROADCOM_COM_SECURITY_1 */



void rutEbt_addPppIntfToBridge(char *cmdLine, UINT32 cmdLen, const char *baseL3IfName)
{
      snprintf(cmdLine, cmdLen, "brctl addif br0 %s", baseL3IfName);
      rut_doSystemAction("rut", cmdLine);

      /* set up ebtable rules for receive path to filter out non-pppoe frames */
      snprintf(cmdLine, cmdLen, "ebtables -t broute -D BROUTING -i %s -p ! 0x%x -j poebr%s 2>/dev/null",
               baseL3IfName, ETH_P_PPP_DISC, baseL3IfName);
      rut_doSystemAction("rut", cmdLine);

      snprintf(cmdLine, cmdLen, "ebtables -t broute -X poebr%s 2>/dev/null", baseL3IfName);
      rut_doSystemAction("rut", cmdLine);

      snprintf(cmdLine, cmdLen, "ebtables -t broute -N poebr%s >/dev/null", baseL3IfName);
      rut_doSystemAction("rut", cmdLine);

      snprintf(cmdLine, cmdLen, "ebtables -t broute -P poebr%s RETURN >/dev/null", baseL3IfName);
      rut_doSystemAction("rut", cmdLine);

      snprintf(cmdLine, cmdLen, "ebtables -t broute -I poebr%s 1 -i %s -p ! 0x%x -j DROP >/dev/null",
               baseL3IfName, baseL3IfName, ETH_P_PPP_SES);
      rut_doSystemAction("rut", cmdLine);

      snprintf(cmdLine, cmdLen, "ebtables -t broute -I BROUTING 1 -i %s -p ! 0x%x -j poebr%s >/dev/null",
               baseL3IfName, ETH_P_PPP_DISC, baseL3IfName);
      rut_doSystemAction("rut", cmdLine);

      /* set up ebtable rules for transmit path to filter out non-pppoe frames */
      snprintf(cmdLine, cmdLen, "ebtables -t nat -D POSTROUTING -o %s -p ! 0x%x -j poebr%s 2>/dev/null",
               baseL3IfName, ETH_P_PPP_DISC, baseL3IfName);
      rut_doSystemAction("rut", cmdLine);

      snprintf(cmdLine, cmdLen, "ebtables -t nat -X poebr%s 2>/dev/null", baseL3IfName);
      rut_doSystemAction("rut", cmdLine);

      snprintf(cmdLine, cmdLen, "ebtables -t nat -N poebr%s >/dev/null", baseL3IfName);
      rut_doSystemAction("rut", cmdLine);

      snprintf(cmdLine, cmdLen, "ebtables -t nat -P poebr%s RETURN >/dev/null", baseL3IfName);
      rut_doSystemAction("rut", cmdLine);

      snprintf(cmdLine, cmdLen, "ebtables -t nat -I poebr%s 1 -o %s -p ! 0x%x -j DROP >/dev/null",
               baseL3IfName, baseL3IfName, ETH_P_PPP_SES);
      rut_doSystemAction("rut", cmdLine);

      snprintf(cmdLine, cmdLen, "ebtables -t nat -I POSTROUTING 1 -o %s -p ! 0x%x -j poebr%s >/dev/null",
               baseL3IfName, ETH_P_PPP_DISC, baseL3IfName);
      rut_doSystemAction("rut", cmdLine);

}



void rutEbt_removePppIntfFromBridge(const char *baseL3IfName)
{
   char cmdLine[BUFLEN_128];

   snprintf(cmdLine, sizeof(cmdLine), "brctl delif br0 %s", baseL3IfName);
   rut_doSystemAction("rut", cmdLine);
   snprintf(cmdLine, sizeof(cmdLine), "ebtables -t broute -D BROUTING -i %s -p ! 0x%x -j poebr%s 2>/dev/null",
            baseL3IfName, ETH_P_PPP_DISC, baseL3IfName);
   rut_doSystemAction("rut", cmdLine);

   snprintf(cmdLine, sizeof(cmdLine), "ebtables -t broute -X poebr%s 2>/dev/null", baseL3IfName);
   rut_doSystemAction("rut", cmdLine);

   snprintf(cmdLine, sizeof(cmdLine), "ebtables -t nat -D POSTROUTING -o %s -p ! 0x%x -j poebr%s 2>/dev/null",
            baseL3IfName, ETH_P_PPP_DISC, baseL3IfName);
   rut_doSystemAction("rut", cmdLine);
   snprintf(cmdLine, sizeof(cmdLine), "ebtables -t nat -X poebr%s 2>/dev/null", baseL3IfName);
   rut_doSystemAction("rut", cmdLine);

}

void rutEbt_defaultLANSetup6(void)
{
    char line[BUFLEN_512];
    static UBOOL8 lanSetup6 = FALSE;
    
    if (lanSetup6 == TRUE)
        return;
    
    lanSetup6 = TRUE;
    snprintf(line, sizeof(line), "ebtables -t nat -N brchain 2>/dev/null");
    rut_doSystemAction("rutEbt_defaultLANSetup6", line);
    
    snprintf(line, sizeof(line), "ebtables -t nat -I PREROUTING -j brchain 2>/dev/null");
    rut_doSystemAction("rutEbt_defaultLANSetup6", line);
    
    
    snprintf(line, sizeof(line), "ebtables -t nat -I brchain -p ipv6 --ip6-protocol 58 --ip6-icmp-type 128 -j REJECT --reject-with 0 2>/dev/null");
    rut_doSystemAction("rutEbt_defaultLANSetup6", line);

    snprintf(line, sizeof(line), "ebtables -t nat -I brchain -p ipv6 --ip6-protocol 58 --ip6-icmp-type 128 --ip6-src fe80::/16 -j RETURN 2>/dev/null");
    rut_doSystemAction("rutEbt_defaultLANSetup6", line);
}

void rutEbt_configICMPv6Reply(const char *prefix, UBOOL8 add)
{
    char line[BUFLEN_512];
        
    cmsLog_debug("prefix=%s, add<%d>", prefix, add);
    if (prefix == NULL)
		return;
	
    if (add)
        snprintf(line, sizeof(line), "ebtables -t nat -I brchain -p ipv6 --ip6-protocol 58 --ip6-icmp-type 128 --ip6-src %s -j RETURN 2>/dev/null", prefix);
    else
        snprintf(line, sizeof(line), "ebtables -t nat -D brchain -p ipv6 --ip6-protocol 58 --ip6-icmp-type 128 --ip6-src %s -j RETURN 2>/dev/null", prefix);
    
    rut_doSystemAction("rutEbt_configICMPv6Reply", line);
}


