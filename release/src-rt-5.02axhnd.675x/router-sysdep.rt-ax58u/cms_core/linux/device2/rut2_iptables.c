/*
#
#  Copyright 2013, Broadcom Corporation
#
# <:label-BRCM:2013:proprietary:standard
# 
#  This program is the proprietary software of Broadcom and/or its
#  licensors, and may only be used, duplicated, modified or distributed pursuant
#  to the terms and conditions of a separate, written license agreement executed
#  between you and Broadcom (an "Authorized License").  Except as set forth in
#  an Authorized License, Broadcom grants no license (express or implied), right
#  to use, or waiver of any kind with respect to the Software, and Broadcom
#  expressly reserves all rights in and to the Software and all intellectual
#  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
#  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
#  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
# 
#  Except as expressly set forth in the Authorized License,
# 
#  1. This program, including its structure, sequence and organization,
#     constitutes the valuable trade secrets of Broadcom, and you shall use
#     all reasonable efforts to protect the confidentiality thereof, and to
#     use this information only in connection with your use of Broadcom
#     integrated circuit products.
# 
#  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
#     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
#     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
#     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
#     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
#     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
#     PERFORMANCE OF THE SOFTWARE.
# 
#  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
#     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
#     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
#     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
#     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
#     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
#     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
#     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
#     LIMITED REMEDY.
# :>
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/utsname.h>

#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_iptables.h"
#include "rut_lan.h"
#include "rut_upnp.h"
#include "rut_network.h"
#include "rut_virtualserver.h"
#include "rut_pmap.h"
#include "rut_wan.h"
#include "rut_ipsec.h"
#include <bcm_local_kernel_include/linux/netfilter/nf_conntrack_pt.h>
#include <linux/version.h>

#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_NAT_1

extern UBOOL8 isModuleInserted(char *moduleName);

typedef struct
{
    const char*         name;
	const unsigned int  protocol;
	const unsigned int  localPort;
	const unsigned int  redirectPort;
}RedirectVirtualServerEntry;

#define REDIRECT_TCP 1
#define REDIRECT_UDP 2

const RedirectVirtualServerEntry localPortChecklist[] =
{
 	{"Web"   ,REDIRECT_TCP ,WEB_SERVER_PORT_80    ,WEB_SERVER_PORT_8080    },
#ifdef SUPPORT_FTPD
 	{"FTP"   ,REDIRECT_TCP ,FTP_SERVER_PORT_21    ,FTP_SERVER_PORT_2121    },
#endif 	
#ifdef SUPPORT_SNMP
 	{"SNMP" ,REDIRECT_UDP ,SNMP_AGENT_PORT_161   ,SNMP_AGENT_PORT_16116   },
#endif 	
#ifdef SUPPORT_SSHD
 	{"SSH"   ,REDIRECT_TCP ,SSH_SERVER_PORT_22    ,SSH_SERVER_PORT_2222    },
#endif 	
#ifdef SUPPORT_TELNETD 	 	
 	{"TELNET",REDIRECT_TCP ,TELNET_SERVER_PORT_23 ,TELNET_SERVER_PORT_2323 },
#endif
#ifdef SUPPORT_TFTPD
 	{"TFTP"  ,REDIRECT_UDP ,TFTP_SERVER_PORT_69   ,TFTP_SERVER_PORT_6969   },
#endif 	
	{NULL    ,0      ,0                     ,0                  	 },     //Keep on last one 
};

void rutIpt_insertPortTriggeringModules_dev2(void)
{
   struct utsname kernel;
   char cmd[128];

   if (!isModuleInserted("nf_nat_pt")) {
      insertModuleByName("nf_nat");

      if(uname(&kernel) < 0) {
         cmsLog_error("Fail to insert nf_nat_pt.ko - uname() error\n");
         return;
      }
      sprintf(cmd, "insmod /lib/modules/%s/kernel/net/ipv4/netfilter/nf_nat_pt.ko ", kernel.release);
      rut_doSystemAction("rut", cmd);
   }
}

#endif /* DMP_DEVICE2_NAT_1 */

static void parseFirewallExceptionObject_dev2(const void *voidObj, char *src, char *dst, char *sport, char *dport, char *protocol, UBOOL8 *tcpOrUdp,char *target)
{
   const _Dev2FirewallExceptionRuleObject *obj = (_Dev2FirewallExceptionRuleObject *) voidObj;

   /* source address/mask */
   if ( cmsUtl_strcmp(obj->sourceIPAddress, "\0") != 0 ) 
   {
      if (strchr(obj->sourceIPAddress, ':') == NULL)
      {
         /* IPv4 address */
         if ( cmsUtl_strcmp(obj->sourceNetMask, "\0") != 0 )
            sprintf(src, "-s %s/%d", obj->sourceIPAddress, cmsNet_getLeftMostOneBitsInMask(obj->sourceNetMask));
         else
            sprintf(src, "-s %s", obj->sourceIPAddress);
      }
      else
      {
         /* IPv6 address */
         sprintf(src, "-s %s", obj->sourceIPAddress);
      }
   }
   
   /* source port */
   if ( obj->sourcePortStart != 0)
   {
      if ( obj->sourcePortEnd != 0)
      {
         sprintf(sport, "--sport %d:%d", obj->sourcePortStart, obj->sourcePortEnd);
      }
      else
      {
         sprintf(sport, "--sport %d", obj->sourcePortStart);
      }
   }
   
   /* destination address/mask */
   if ( cmsUtl_strcmp(obj->destinationIPAddress, "\0") != 0 ) 
   {
      if (strchr(obj->destinationIPAddress, ':') == NULL)
      {
         /* IPv4 address */
         if ( cmsUtl_strcmp(obj->destinationNetMask, "\0") != 0 )
            sprintf(dst, "-d %s/%d", obj->destinationIPAddress, cmsNet_getLeftMostOneBitsInMask(obj->destinationNetMask));
         else
            sprintf(dst, "-d %s", obj->destinationIPAddress);
      }
      else
      {
         /* IPv6 address */
         sprintf(dst, "-d %s", obj->destinationIPAddress);
      }
   }
   
   /* destination port */
   if ( obj->destinationPortStart != 0)
   {
      if ( obj->destinationPortEnd != 0)
      {
         sprintf(dport, "--dport %d:%d", obj->destinationPortStart, obj->destinationPortEnd);
      }
      else
      {
         sprintf(dport, "--dport %d", obj->destinationPortStart);
      }
   }
   
   /* protocol */
   if ( cmsUtl_strcmp(obj->protocol, MDMVS_TCP) == 0 ) 
   {
      sprintf(protocol, "-p tcp");
   }
   else if ( cmsUtl_strcmp(obj->protocol, MDMVS_UDP) == 0 ) 
   {
      sprintf(protocol, "-p udp");
   }
   else if ( cmsUtl_strcmp(obj->protocol, MDMVS_TCP_OR_UDP) == 0 ) 
   {
      *tcpOrUdp = TRUE;
   }
   else if ( cmsUtl_strcmp(obj->protocol, MDMVS_ICMP) == 0 ) 
   {
      sprintf(protocol, "-p icmp");
   }
   else if ( cmsUtl_strcmp(obj->protocol, MDMVS_ICMPV6) == 0 ) 
   {
      sprintf(protocol, "-p icmpv6");
   }
   else
   {
      // None
      protocol[0] = '\0';
   }

   /* Target */
   if ( cmsUtl_strcmp(obj->target, MDMVS_ACCEPT) == 0 ) 
   {
      sprintf(target, "-j ACCEPT");
   }
   else
   {
	   sprintf(target, "-j DROP");
   }

   return;
}


CmsRet rutIpt_ipFilterInRunIptables_dev2(char *ipver, char action, const char *ifName, char *protocol, char *src, char *sport, char *dst, char *dport,char *target);

extern void insertModuleByName(char *moduleName);

void rutIpt_doFirewallExceptionRule_dev2(const _Dev2FirewallExceptionRuleObject *InObj, const char *ifName, UBOOL8 add)
{
   char src[BUFLEN_64], dst[BUFLEN_64];
   char sport[BUFLEN_40], dport[BUFLEN_40], protocol[BUFLEN_32],target[BUFLEN_32];
   char action;
   char *ipver;
   UBOOL8 tcpOrUdp = FALSE;

   src[0] = dst[0] = sport[0] = dport[0] = protocol[0] = target[0] = '\0';
   
   insertModuleByName("iptable_filter");
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
   insertModuleByName("ip6table_filter");
#endif

   ipver = (InObj)->IPVersion;

   /* action */
   if (!add) 
   {
	   action = 'D';      
   }else
   {
	   action = (cmsUtl_strcmp(InObj->target, MDMVS_ACCEPT) == 0 ) ? 'I' : 'A';
   }

   parseFirewallExceptionObject_dev2(InObj, src, dst, sport, dport, protocol, &tcpOrUdp, target);
   
   /* Delete rule first for avoid duplicate rule add */

   if (tcpOrUdp)
   {
      rutIpt_ipFilterInRunIptables_dev2(ipver, 'D', ifName, "-p tcp", src, sport, dst, dport,target);
	  if(action != 'D')
         rutIpt_ipFilterInRunIptables_dev2(ipver, action, ifName, "-p tcp", src, sport, dst, dport,target);
	  
      rutIpt_ipFilterInRunIptables_dev2(ipver, 'D', ifName, "-p udp", src, sport, dst, dport,target);	  
	  if(action != 'D')
         rutIpt_ipFilterInRunIptables_dev2(ipver, action, ifName, "-p udp", src, sport, dst, dport,target);
   }
   else
   {
      rutIpt_ipFilterInRunIptables_dev2(ipver, 'D', ifName, protocol, src, sport, dst, dport,target);
	  if(action != 'D')
         rutIpt_ipFilterInRunIptables_dev2(ipver, action, ifName, protocol, src, sport, dst, dport,target);
   }
}


CmsRet rutIpt_ipFilterInRunIptables_dev2(char *ipver, char action, const char *ifName, char *protocol, char *src, char *sport, char *dst, char *dport,char *target)
{
   char cmd[BUFLEN_256];
   char ipt[BUFLEN_16];

   snprintf(ipt, sizeof(ipt), "%s -w", (atoi(ipver) == 4)? "iptables" : "ip6tables");

   /* Add rule to FORWARD chain for routing and NAT traffic */	 
   snprintf(cmd, sizeof(cmd), "%s -w -%c FORWARD -i %s %s %s %s %s %s %s", ipt, action, ifName, protocol, src, sport, dst, dport,target);

   /* For deletion, the rule might not be there, add redirect to null to
   * avoid console "Bad rule..." display
   */
   if (action == 'D')
   {
      cmsUtl_strncat(cmd, BUFLEN_256, " 2>/dev/null");
   }
   rut_doSystemAction("rutIpt_ipFilterInRunIptables_dev2", cmd);

   /* 
      br incoming drop rule mean it's a "rule for block traffic outgoing to wan". 
      So "outgoing rule" (br incoming drop rule) just add on forward chain only
      Because the traffic destination may be localhost and shouldn't be block via input rule.
      For ex: block web access to wan , but still need access on CPE web
   */   

   if (
           (cmsUtl_strcmp(target, "-j ACCEPT") == 0)
        || ((cmsUtl_strcmp(target, "-j ACCEPT") != 0) && (cmsUtl_strstr(ifName, "br") == NULL))  //non- br incoming drop rule
      )    
   {
      snprintf(cmd, sizeof(cmd), "%s -w -%c INPUT -i %s %s %s %s %s %s %s", ipt, action, ifName, protocol, src, sport, dst, dport,target);
      if (action == 'D')
      {
         cmsUtl_strncat(cmd, BUFLEN_256, " 2>/dev/null");
      }      
      rut_doSystemAction("rutIpt_ipFilterInRunIptables_dev2", cmd);
   }

   return CMSRET_SUCCESS;
}


CmsRet rutIpt_GetfwExceptionbyFullPath_dev2(char *ipIntfFullPath,Dev2FirewallExceptionObject **fwExObj,InstanceIdStack *iidStack)
{
      int found = FALSE;
      CmsRet ret = CMSRET_INTERNAL_ERROR;

      cmsLog_debug("entered, ipIntfFullPath=%s", ipIntfFullPath);
      INIT_INSTANCE_ID_STACK(iidStack);
      while (!found && cmsObj_getNext(MDMOID_DEV2_FIREWALL_EXCEPTION, iidStack, (void **) fwExObj) == CMSRET_SUCCESS)
      {
            if (cmsUtl_strcmp((*fwExObj)->IPInterface, ipIntfFullPath) == 0 )
            {
                  found=TRUE;
                  ret = CMSRET_SUCCESS;
                  break;
            }
            cmsObj_free((void **)fwExObj);
      }
 
      return ret;
 }


CmsRet rutIpt_AddfwExceptionforIPDevice_dev2(char *ipIntfFullPath)
{
   Dev2FirewallExceptionObject *fwExObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   int found = FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered, ipIntfFullPath=%s", ipIntfFullPath);

   if(rutIpt_GetfwExceptionbyFullPath_dev2(ipIntfFullPath, &fwExObj, &iidStack) == CMSRET_SUCCESS)
   {
      found = TRUE;		
      cmsObj_free((void **)&fwExObj);
   }

   if(!found)
   {
      INIT_INSTANCE_ID_STACK(&iidStack);
      cmsObj_addInstance(MDMOID_DEV2_FIREWALL_EXCEPTION,&iidStack);
      if ((ret = cmsObj_get(MDMOID_DEV2_FIREWALL_EXCEPTION, &iidStack, 0, (void **) &fwExObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to add Dev2FirewallExceptionObject on %s, ret = %d", ipIntfFullPath,ret);
         cmsObj_deleteInstance(MDMOID_DEV2_FIREWALL_EXCEPTION, &iidStack);
         return ret;
      }

      CMSMEM_REPLACE_STRING_FLAGS(fwExObj->IPInterface, ipIntfFullPath, mdmLibCtx.allocFlags);

      ret = cmsObj_set((void *)fwExObj,&iidStack);
      cmsObj_free((void **)&fwExObj);
   }
   else
   {
      cmsLog_notice("Duplicate to add Dev2FirewallExceptionObject on %s, ret = %d", ipIntfFullPath,ret);
   }

   cmsLog_debug("Exit. ret %d", ret);
   
   return ret;
}

CmsRet rutIpt_RemovefwExceptionforIPDevice_dev2(char *ipIntfFullPath)
{
   _Dev2FirewallExceptionObject *fwExObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   int found = FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered, ipIntfFullPath=%s", ipIntfFullPath);
   
   /* This firewall rule is only for device 2 data model */
   if (!cmsMdm_isDataModelDevice2())
   {
      cmsLog_notice("Not for hybrid data model");
      return ret;
   }

   if(rutIpt_GetfwExceptionbyFullPath_dev2(ipIntfFullPath, &fwExObj, &iidStack) == CMSRET_SUCCESS)
   {
   	  found = TRUE;		
   	  cmsObj_free((void **)&fwExObj);        
   }

   if(found)
   {
      if((ret = cmsObj_deleteInstance(MDMOID_DEV2_FIREWALL_EXCEPTION,&iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to del Dev2FirewallExceptionObject, ret = %d", ret);
         return ret;
      }
   }

   return ret;
}

CmsRet rutIpt_RemovefwExceptionRule_dev2(char *ipIntfFullPath,char *target)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack parentIid = EMPTY_INSTANCE_ID_STACK;
   Dev2FirewallExceptionObject *fwExObj=NULL;	
   Dev2FirewallExceptionRuleObject *obj=NULL;
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 found=FALSE;

   /* This firewall rule is only for device 2 data model */
   if (!cmsMdm_isDataModelDevice2())
   {
      cmsLog_notice("Not for hybrid data model");
      return ret;
   }
   
   cmsLog_debug("entered, ipIntfFullPath=%s", ipIntfFullPath);
		 
   INIT_INSTANCE_ID_STACK(&parentIid);
   while (!found && cmsObj_getNext(MDMOID_DEV2_FIREWALL_EXCEPTION, &parentIid, (void **) &fwExObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(fwExObj->IPInterface, ipIntfFullPath) == 0 )
      {
         found=TRUE;
         ret = CMSRET_SUCCESS;		   
         break;
      }
      cmsObj_free((void **)&fwExObj);
   }

   if(found)
   {
      INIT_INSTANCE_ID_STACK(&iidStack);
      while  ((ret = cmsObj_getNextInSubTree
      (MDMOID_DEV2_FIREWALL_EXCEPTION_RULE, &parentIid ,&iidStack, (void **)&obj)) == CMSRET_SUCCESS)
      {
         /* Target */
         if ( cmsUtl_strcmp(obj->target, target) == 0 ) 
         {
				
            if((ret = cmsObj_deleteInstance(MDMOID_DEV2_FIREWALL_EXCEPTION_RULE,&iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to del Dev2FirewallExceptionObject, ret = %d", ret);
            }else{
               INIT_INSTANCE_ID_STACK(&iidStack);
            }
         }
         cmsObj_free((void **)&obj);			
      }

      ret = CMSRET_SUCCESS;	   
      cmsObj_free((void **)&fwExObj);
    }
	
    return ret;   
}


void rutIpt_initFirewallExceptions_dev2(const char *ifName) 
{
   char *ipIntfFullPath=NULL;
   Dev2FirewallExceptionObject *fwExObj=NULL;
   Dev2FirewallExceptionRuleObject *fwExRuleObj=NULL;   	
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;   
   int found = FALSE;
   CmsRet ret;

   cmsLog_debug("entered, ifName=%s", ifName);

   ret = qdmIntf_intfnameToFullPathLocked_dev2(ifName,FALSE,&ipIntfFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert %s to fullpath, ret=%d", ifName, ret);
   }
   else
   {
      while (!found &&
             cmsObj_getNext(MDMOID_DEV2_FIREWALL_EXCEPTION, &iidStack,
                            (void **) &fwExObj) == CMSRET_SUCCESS)
      {
          if (cmsUtl_strcmp(fwExObj->IPInterface, ipIntfFullPath) == 0 )
          {
             found = TRUE;
             while (cmsObj_getNextInSubTree(MDMOID_DEV2_FIREWALL_EXCEPTION_RULE,
                                    &iidStack, &iidStack1,
                                    (void **) &fwExRuleObj) == CMSRET_SUCCESS)
             {
                /* the a set without changing the obj.  Let RCL do the work */
               cmsObj_set((void *)fwExRuleObj, &iidStack1);
               cmsObj_free((void **) &fwExRuleObj);
             }
          }
          cmsObj_free((void **)&fwExObj);
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
   }

   cmsLog_debug("done");
}  /* End of rutIpt_initFirewallExceptions_dev2() */    


UBOOL8 rutIpt_isFirewallExceptionValid_dev2(const Dev2FirewallExceptionRuleObject *commonExceptionObj)
{
   if (cmsUtl_strcmp(commonExceptionObj->filterName, "\0") == 0)
   {
      cmsLog_error("Invalid filter name");
      return FALSE;
   }

   if (cmsUtl_strcmp(commonExceptionObj->sourceIPAddress, "\0") != 0)
   {
      if (atoi(commonExceptionObj->IPVersion) == 4)
      {
         if (cmsUtl_isValidIpAddress(AF_INET, commonExceptionObj->sourceIPAddress) == FALSE)
         {
            cmsLog_error("Invalid IPv4 source address");
            return FALSE;
         }
      }
      else
      {
         if (cmsUtl_isValidIpAddress(AF_INET6, commonExceptionObj->sourceIPAddress) == FALSE)
         {
            cmsLog_error("Invalid IPv6 source address");
            return FALSE;
         }
      }
   }

   if (cmsUtl_strcmp(commonExceptionObj->destinationIPAddress, "\0") != 0)
   {
      if (atoi(commonExceptionObj->IPVersion) == 4)
      {
         if (cmsUtl_isValidIpAddress(AF_INET, commonExceptionObj->destinationIPAddress) == FALSE)
         {
            cmsLog_error("Invalid IPv4 destination address");
            return FALSE;
         }
      }
      else
      {
         if (cmsUtl_isValidIpAddress(AF_INET6, commonExceptionObj->destinationIPAddress) == FALSE)
         {
            cmsLog_error("Invalid IPv6 destination address");
            return FALSE;
         }
      }
   }

   /*
    * Simple port number range checking can be done in the MDM.
    * The relationship between portStart and portEnd must be done here.
    */
   if ((commonExceptionObj->sourcePortEnd != 0 && commonExceptionObj->sourcePortEnd < commonExceptionObj->sourcePortStart) ||
       (commonExceptionObj->destinationPortEnd != 0 && commonExceptionObj->destinationPortEnd < commonExceptionObj->destinationPortStart))
   {
      cmsLog_error("Invalid port number or start/end relationship");
      return FALSE;
   }

   return TRUE;
}


UBOOL8 rutIpt_isFirewallExceptionSame_dev2(const Dev2FirewallExceptionRuleObject *obj1,
                                      const Dev2FirewallExceptionRuleObject *obj2)
{
   if ( cmsUtl_strcmp(obj1->filterName, obj2->filterName) == 0 )
   {
      /* filter names must be unique */
      return TRUE;
   }
   else if (cmsUtl_strcmp(obj1->sourceIPAddress, obj2->sourceIPAddress) == 0 &&
            cmsUtl_strcmp(obj1->sourceNetMask, obj2->sourceNetMask) == 0 &&
            cmsUtl_strcmp(obj1->destinationIPAddress, obj2->destinationIPAddress) == 0 &&
            cmsUtl_strcmp(obj1->destinationNetMask, obj2->destinationNetMask) == 0 &&
            cmsUtl_strcmp(obj1->protocol, obj2->protocol) == 0 &&
            cmsUtl_strcmp(obj1->IPVersion, obj2->IPVersion) == 0 &&
            cmsUtl_strcmp(obj1->target, obj2->target) == 0 &&
            obj1->sourcePortStart == obj2->sourcePortStart &&
            obj1->sourcePortEnd == obj2->sourcePortEnd &&
            obj1->destinationPortStart == obj2->destinationPortStart &&
            obj1->destinationPortEnd == obj2->destinationPortEnd
            )
   {
      /* two filters with different names cannot have same rule */
      return TRUE;
   }

   return FALSE;
}


UBOOL8 rutIpt_isDuplicateFirewallException_dev2(const Dev2FirewallExceptionRuleObject *obj,
                                       const InstanceIdStack *iidStack)
{
   UBOOL8 duplicate=FALSE;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   Dev2FirewallExceptionRuleObject *firewallobj = NULL;

   while (!duplicate &&
          (cmsObj_getNext(MDMOID_DEV2_FIREWALL_EXCEPTION_RULE, &iidStack1,
                                 (void **)&firewallobj)  == CMSRET_SUCCESS))
   {
      if(cmsMdm_compareIidStacks(&iidStack1, iidStack))
      {
         /* this is a different object instance than me, does it have same values as me? */
         if (rutIpt_isFirewallExceptionSame_dev2(obj, firewallobj))
         {
            cmsLog_error("Rejecting duplicate firewall exception %s", obj->filterName);
            duplicate = TRUE;
         }
      }

      cmsObj_free((void **) &firewallobj);
   }

   return duplicate;
}


#ifdef DMP_DEVICE2_NAT_1

CmsRet rutIpt_vrtsrvRunIptables(char action, char *device,  char *protocol, char *inPort, char *srvAddress, char *srvPort, char *remoteHost);

CmsRet rutIpt_vrtsrvCfg_dev2(const _Dev2NatPortMappingObject *portmapObj, const UBOOL8 add ,char *portmap_ifName)
{
   CmsRet ret = CMSRET_SUCCESS;
   char interface[CMS_IFNAME_LENGTH];
   char action = 'A';
   char cmd[BUFLEN_128]={0};
   char extPort[BUFLEN_16]={0};
   char intPort[BUFLEN_16]={0};
   char dmzHost[BUFLEN_16]={0};
   UINT32 externalPort=0;
   UINT32 X_BROADCOM_COM_ExternalPortEnd=0;
   UINT32 internalPort=0;
   UINT32 X_BROADCOM_COM_InternalPortEnd=0;
   UINT32 portMappingProtocolnum=0;
   char portMappingProtocol[BUFLEN_4]={0};
   char internalClient[CMS_IPADDR_LENGTH]={0};
   char remoteHost[CMS_IPADDR_LENGTH]={0};
   char portMappingDescription[BUFLEN_256]={0};
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack nat_iidStack=EMPTY_INSTANCE_ID_STACK;
   RedirectVirtualServerEntry *entry_ptr=NULL;
   UBOOL8 allInterfaces=FALSE;
   UBOOL8 isDMZobject=FALSE;

   if(portmapObj != NULL)
   {
      if(portmapObj->allInterfaces == FALSE)
      {
         if( ret=qdmIntf_fullPathToIntfnameLocked(portmapObj->interface,(char *)interface))
         {
             cmsLog_error("Failed to convert interface to ifName, ret = %d",ret);
             return ret;
         }
      }

      allInterfaces = portmapObj->allInterfaces;
      externalPort = portmapObj->externalPort;
      X_BROADCOM_COM_ExternalPortEnd = portmapObj->externalPortEndRange;
      internalPort = portmapObj->internalPort;
      X_BROADCOM_COM_InternalPortEnd = portmapObj->X_BROADCOM_COM_InternalPortEndRange;
      if (cmsUtl_strcmp(portmapObj->protocol, "TCP") == 0)
      {
         strcpy(portMappingProtocol, "1");
      }
      else if (cmsUtl_strcmp(portmapObj->protocol, "UDP") == 0)
      {
         strcpy(portMappingProtocol, "2");
      }
      else
      {
         strcpy(portMappingProtocol, "0");
      }

      cmsUtl_strncpy(internalClient, portmapObj->internalClient, sizeof(internalClient));

      if(portmapObj->remoteHost!= NULL)
      {
         cmsUtl_strncpy(remoteHost, portmapObj->remoteHost, sizeof(remoteHost));
      }
      else
      {
         remoteHost[0] = '\0';
      }

      cmsUtl_strncpy(portMappingDescription, portmapObj->description, sizeof(portMappingDescription));

   }
   else
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   do{

         if(allInterfaces)
         {
            Dev2NatIntfSettingObject *natIntf=NULL;
            UBOOL8 found = FALSE;
            while(cmsObj_getNextFlags(MDMOID_DEV2_NAT_INTF_SETTING,
                                       &nat_iidStack, OGF_NO_VALUE_UPDATE,
                                       (void **) &natIntf) == CMSRET_SUCCESS)
            {
               if (natIntf->enable)
               {
                   qdmIntf_fullPathToIntfnameLocked(natIntf->interface,(char *)interface);
                   found = TRUE;
               }

               // Free the mem allocated this object by the get API.
               cmsObj_free((void **)&natIntf);

               if(found)
                  break;
            }

            if(found == FALSE)
            { // No more avaliable NAT interface, break out 
              break;
            }
         }


         if(add)
         {
            action = 'A';
         }
         else
         {
            action = 'D';
         }

         if (externalPort == X_BROADCOM_COM_ExternalPortEnd)
         {
            sprintf(extPort, "%u", externalPort); 
         }
         else
         {
            sprintf(extPort, "%u:%u", externalPort, X_BROADCOM_COM_ExternalPortEnd);
         }

         /* if internal ports are same as enternal, leave intPort to "" */
         if (internalPort == externalPort && X_BROADCOM_COM_InternalPortEnd == X_BROADCOM_COM_ExternalPortEnd)
         {
            intPort[0] = '\0';
         }
         else 
         {
            if (internalPort == X_BROADCOM_COM_InternalPortEnd)
            {
               sprintf(intPort, "%u", internalPort); 
            }
            else
            {
               sprintf(intPort, "%u:%u", internalPort, X_BROADCOM_COM_InternalPortEnd);
            }
         }

         // handle DMZ host : external and internal port are 0 ,this one is a DMZ port mapping !!
         if( externalPort==0 && internalPort==0)
         {
             // This is a DMZ object, DMZ must be last rule of all DNAT rule and bind on all nat interface
 
             if(cmsUtl_strlen(interface) != 0)
             {
                 sprintf(cmd, "iptables -w -t nat -D PREROUTING -i %s -j DNAT --to-destination %s 2>/dev/null", interface, internalClient);
                 rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
                 sprintf(cmd, "iptables -w -D FORWARD -i %s -d %s -j ACCEPT  2>/dev/null", interface, internalClient);
                 rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);   

                 if( action == 'A')   
                 {
                    sprintf(cmd, "iptables -w -t nat -A PREROUTING -i %s -j DNAT --to-destination %s 2>/dev/null",interface, internalClient);
                    rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);

                    sprintf(cmd, "iptables -w -I FORWARD 1 -i %s -d %s -j ACCEPT 2>/dev/null", interface, internalClient);
                    rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);  
                 }
             }

         }else
         {
            // This is a PortMapping object.

            portMappingProtocolnum=atoi(portMappingProtocol);

            switch ( portMappingProtocolnum ) 
            {
               case 1: // TCP
               {
                  ret = rutIpt_vrtsrvRunIptables(action, interface, "tcp", extPort, internalClient, intPort, remoteHost);
               }
               break;
               case 2: // UDP
               {
                  ret = rutIpt_vrtsrvRunIptables(action, interface, "udp", extPort, internalClient, intPort, remoteHost);
               }
               break; 
               default:
               {
                  ret = rutIpt_vrtsrvRunIptables(action, interface, "tcp", extPort, internalClient, intPort, remoteHost);
                  ret = rutIpt_vrtsrvRunIptables(action, interface, "udp", extPort, internalClient, intPort, remoteHost);
               }
               break; 
            }

            //TODO: Tunneling
#if LATER
            if ( cmsUtl_strcmp(portMappingDescription, "IPSEC") == 0 ) 
            {
               sprintf(cmd, "iptables -w -t nat -%c PREROUTING -i %s -p 50 -j DNAT --to %s",
                       action, interface, internalClient);
               rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
               if ( action == 'A' )
               {
                  sprintf(cmd, "iptables -w -I FORWARD 1 -i %s -p 50 -d %s -j ACCEPT",
                          interface, internalClient);
                  rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
               } 
               else 
               {
                  sprintf(cmd, "iptables -w -D FORWARD -i %s -p 50 -d %s -j ACCEPT",
                          interface, internalClient);
                  rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
               }
            }
            else if ( cmsUtl_strcmp(portMappingDescription, "PPTP") == 0 ) 
            {
               sprintf(cmd, "iptables -w -t nat -%c PREROUTING -i %s -p 47 -j DNAT --to %s",
                       action, interface, internalClient);
               rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
               if ( action == 'A' )
               {
                  sprintf(cmd, "iptables -w -I FORWARD 1 -i %s -p 47 -d %s -j ACCEPT",
                          interface, internalClient);
                  rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
               }
               else 
               {
                  sprintf(cmd, "iptables -w -D FORWARD -i %s -p 47 -d %s -j ACCEPT",
                          interface, internalClient);
                  rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
               }
            }
#endif

            /*
              ** if virtual server is configured and wan access is enabled for this service then
              ** add the prerouting rule to redirect port so that system can support this service by itself
             */

            entry_ptr=localPortChecklist;

            while( entry_ptr->name != NULL )
            {
                if( ((portMappingProtocolnum == 0) || (portMappingProtocolnum == entry_ptr->protocol))&& 
                        (
                            (externalPort == entry_ptr->localPort) ||
                            ((externalPort < entry_ptr->localPort) && (X_BROADCOM_COM_ExternalPortEnd >= entry_ptr->localPort)) 
                        )
                  )
                {
                    sprintf(cmd, "iptables -w -t nat -%c PREROUTING -i %s -p %s --dport %d -j REDIRECT --to-ports %d 2>/dev/null"
                            ,action
                            ,interface
                            ,(entry_ptr->protocol == REDIRECT_TCP) ? "tcp" : "udp"
                            ,entry_ptr->redirectPort
                            ,entry_ptr->localPort);
                    rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
                    break;
                }

                entry_ptr++;
            }

         }
      }while(allInterfaces);

      // Handle DMZ rule, DMZ must be last rule of all DNAT rule.
      if(add)
      {
         _Dev2NatPortMappingObject *DMZ_portmapObj=NULL;
         InstanceIdStack iidStack_2 = EMPTY_INSTANCE_ID_STACK;
         CmsRet ret_2 = CMSRET_INTERNAL_ERROR;
         INIT_INSTANCE_ID_STACK(&nat_iidStack);

         while ( (ret_2 = cmsObj_getNext
               (MDMOID_DEV2_NAT_PORT_MAPPING, &iidStack_2, (void **) &DMZ_portmapObj)) == CMSRET_SUCCESS)
         {
            //if (!cmsUtl_strncmp((DMZ_portmapObj)->X_BROADCOM_COM_AppName,"DMZ",3)) 
            if(DMZ_portmapObj->internalPort==0 &&  DMZ_portmapObj->externalPort==0)
            {
                strcpy(dmzHost, DMZ_portmapObj->internalClient);

                Dev2NatIntfSettingObject *natIntf=NULL;
                while(cmsObj_getNextFlags(MDMOID_DEV2_NAT_INTF_SETTING,
                                           &nat_iidStack, OGF_NO_VALUE_UPDATE,
                                           (void **) &natIntf) == CMSRET_SUCCESS)
                {
                   if (natIntf->enable)
                   {
                       qdmIntf_fullPathToIntfnameLocked(natIntf->interface,(char *)interface);

                      sprintf(cmd, "iptables -w -t nat -D PREROUTING -i %s -j DNAT --to-destination %s", interface, dmzHost);
                      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
                      sprintf(cmd, "iptables -w -D FORWARD -i %s -d %s -j ACCEPT", interface, dmzHost);
                      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);   

                      sprintf(cmd, "iptables -w -t nat -A PREROUTING -i %s -j DNAT --to-destination %s 2>/dev/null", interface, dmzHost);
                      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);
                      sprintf(cmd, "iptables -w -I FORWARD 1 -i %s -d %s -j ACCEPT 2>/dev/null", interface, dmzHost);
                      rut_doSystemAction("rutIpt_vrtsrvCfg", cmd);  

                   }
                   // Free the mem allocated this object by the get API.
                   cmsObj_free((void **)&natIntf);
                }
            }
            // Free the mem allocated this object by the get API.
            cmsObj_free((void **) &DMZ_portmapObj);
         }
      }


   return ret;
}

void rutIpt_activatePortMappingEntries_dev2(const char *ifName __attribute((unused)))
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2NatPortMappingObject *portmappingObj=NULL;
   char portmap_ifName[CMS_IFNAME_LENGTH]={0};
   CmsRet ret=CMSRET_SUCCESS;

   //check ifName 
   while ((ret = cmsObj_getNext(MDMOID_DEV2_NAT_PORT_MAPPING, &iidStack, (void **) &portmappingObj)) == CMSRET_SUCCESS)
   {
      if(portmappingObj->allInterfaces || qdmIntf_getIntfnameFromFullPathLocked_dev2(portmappingObj->interface,portmap_ifName,CMS_IFNAME_LENGTH) == CMSRET_SUCCESS)
      {
         if(portmappingObj->allInterfaces || !cmsUtl_strcmp(ifName,portmap_ifName))
         {
            rutIpt_vrtsrvCfg_dev2(portmappingObj, TRUE, ifName);
         }
      }
      cmsObj_free((void **) &portmappingObj);	   
   }

   return;
}


#ifdef DMP_DEVICE2_BRIDGE_1 /* aka SUPPORT_PMAP */
void rutIpt_configNatForIntfGroup_dev2(UBOOL8 isAdd, const char *ifName, const char *bridgeIfName, UBOOL8 isFullCone)
{
   char localSubnet[BUFLEN_64];
   char localSubnetmask[BUFLEN_64];
   char ipAddress[BUFLEN_64];
   char ifcIpAddress[BUFLEN_64];
   char cmd[BUFLEN_256];
   
   cmsLog_debug("wan isAdd=%d, ifName=%s bridgeIfName=%s fullConeEnabled=%d", 
                isAdd, ifName, bridgeIfName, isFullCone);
   
#ifdef DESKTOP_LINUX
   strcpy(localSubnet, "192.168.2.0");
   strcpy(localSubnetmask, "255.255.255.0");
#else   
   if (rut_getIfSubnet(bridgeIfName, localSubnet) && rut_getIfMask(bridgeIfName, localSubnetmask)) 
#endif
   {
      /* NatMasq: config nat for the local private subnet only. */   
      if(isAdd)
         rutIpt_insertNatMasquerade(ifName,localSubnet,localSubnetmask,isFullCone);
      else
         rutIpt_deleteNatMasquerade(ifName,localSubnet,localSubnetmask);

      /* DnsForward: */
      /* Sarah: dont see why need to do dns forward here, leave it for now */
#if 0	  
      if (rut_getIfAddr(bridgeIfName, ipAddress) && rut_getIfAddr(bridgeIfName, ifcIpAddress))
      {
         sprintf(cmd, "iptables -w -t nat -%c PREROUTING -i %s -d %s -p udp --dport 53 -j DNAT --to %s", 
                 cmdType, bridgeIfName, ifcIpAddress, ipAddress);
         rut_doSystemAction("configNatForIntfGroup -- DnsForward", cmd);
      }
#endif

      /* Remove  the binding to br0 here (was done int rutIpt_initNat)
       * for the problem maybe caused by MASQUERADE.
       */
      if(isAdd && cmsUtl_strcmp("br0", bridgeIfName))
      {
         if (rut_getIfSubnet("br0", localSubnet) && rut_getIfMask("br0", localSubnetmask)) 
         {
            rutIpt_deleteNatMasquerade(ifName, localSubnet, localSubnetmask);      
         }
      }

   }
}
#endif /* DMP_DEVICE2_BRIDGE_1 */

#endif /* DMP_DEVICE2_NAT_1 */

#endif /*DMP_DEVICE2_BASELINE_1 */

