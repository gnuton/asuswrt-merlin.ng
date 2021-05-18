/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom 
 *  All Rights Reserved
 *
 * <:label-BRCM:2012:proprietary:standard
 * 
 *  This program is the proprietary software of Broadcom  and/or its
 *  licensors, and may only be used, duplicated, modified or distributed pursuant
 *  to the terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied), right
 *  to use, or waiver of any kind with respect to the Software, and Broadcom
 *  expressly reserves all rights in and to the Software and all intellectual
 *  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 *  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 *  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 * 
 *  Except as expressly set forth in the Authorized License,
 * 
 *  1. This program, including its structure, sequence and organization,
 *     constitutes the valuable trade secrets of Broadcom, and you shall use
 *     all reasonable efforts to protect the confidentiality thereof, and to
 *     use this information only in connection with your use of Broadcom
 *     integrated circuit products.
 * 
 *  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 *     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 *     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
 *     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
 *     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
 *     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
 *     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
 *     PERFORMANCE OF THE SOFTWARE.
 * 
 *  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
 *     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 *     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
 *     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
 *     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
 *     LIMITED REMEDY.
:>
 *
 ************************************************************************/
#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1

#include <linux/version.h>
#include <sys/utsname.h>
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_core.h"
#include "rut_lan.h"
#include "rut_util.h"
#include "rut_system.h"
#include "rut_pmap.h"
#include "mdm.h"
#include "rut_openvswitch.h"

/* hard code openvswtich ovs-vsctl and openvswitch bridge */
#define OVS_VSCTL         "/bin/ovs-vsctl"
#define OVS_BRIDGE        "brsdn"
#define OVS_START_SCRIPT  "/etc/openvswitch/scripts/startopenVS.sh > /dev/null 2>&1"
#define OVS_STOP_SCRIPT   "/etc/openvswitch/scripts/stopopenVS.sh > /dev/null 2>&1"

#ifdef DMP_DEVICE2_BRIDGE_1
extern CmsRet rutBridge_addIntfNameToBridge_dev2(const char *intfName, const char *brIntfName);
extern void rutBridge_deleteIntfNameFromBridge_dev2(const char *intfName);
#endif

UBOOL8 rutOpenVS_isEnabled(void)
{
   UBOOL8 openVSEnable = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _OpenvswitchCfgObject *openVSCfg=NULL;
   CmsRet ret;

   cmsLog_debug("Enter");
   if ((ret = cmsObj_get(MDMOID_OPENVSWITCH_CFG, &iidStack, 0, (void **) &openVSCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get OPENVSWITCH_CFG, ret=%d", ret);
   }
   else
   {
      openVSEnable = openVSCfg->enable;
      cmsObj_free((void **) &openVSCfg);
   }

   cmsLog_debug("openvswitch enable status = %d", openVSEnable);
   return openVSEnable;
}

UBOOL8 rutOpenVS_isOpenVSPorts(const char *ifName)
{
   UBOOL8 isOpenVSPort = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _OpenvswitchCfgObject *openVSCfg=NULL;
   char  *ptr,*currIfName,*tmpIfNameList = NULL, *savePtr=NULL;
   CmsRet ret;

   if ((ret = cmsObj_get(MDMOID_OPENVSWITCH_CFG, &iidStack, 0, (void **) &openVSCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get OPENVSWITCH_CFG, ret=%d",ret );
   }
   else
   {
      if ( openVSCfg->ifNameList == NULL)
      {
         cmsObj_free((void **) &openVSCfg);
         return FALSE;
      }
      tmpIfNameList = cmsMem_strdup(openVSCfg->ifNameList);
      if (!tmpIfNameList)
      {
         cmsLog_error("Memory allocation failure");
         cmsObj_free((void **) &openVSCfg);
         cmsMem_free(tmpIfNameList);
         return FALSE;
      }
      ptr = strtok_r(tmpIfNameList, ",", &savePtr);
      /* check those interface deleted */
      while (ptr )
      {
         currIfName=ptr;
         while ((isspace(*currIfName)) && (*currIfName != 0))
         {
            /* skip white space after comma */
            currIfName++;
         }
         /*interfaces is deleted from ports list*/
         if( strcmp(currIfName, ifName) == 0)
         {
            isOpenVSPort = TRUE;
            break;
         }
         ptr = strtok_r(NULL, ",", &savePtr);
      }
      cmsMem_free(tmpIfNameList);
      cmsObj_free((void **) &openVSCfg);
   }
   return isOpenVSPort;
}


CmsRet rutOpenVS_updateOpenvswitch(UBOOL8 change,const char *oldOpenVSports )
{
   CmsRet ret = CMSRET_SUCCESS;
   _OpenvswitchCfgObject *openVSCfg = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   if ((ret = cmsObj_get(MDMOID_OPENVSWITCH_CFG, &iidStack, 0, (void **) &openVSCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get OPENVSWITCH_CFG, ret=%d", ret);
      return ret;
   }
   if(openVSCfg->enable)
   {
      rutOpenVS_startOpenvswitch();
      rutOpenVS_updateOpenvswitchPorts("",openVSCfg->ifNameList);
      rutOpenVS_updateOpenvswitchOFController(openVSCfg->OFControllerIPAddress,openVSCfg->OFControllerPortNumber);
   }
   else if (change)
   {
      /*just delete the bridge will all ports under that bridge*/
      rutOpenVS_updateOpenvswitchPorts(oldOpenVSports,"");
      rutOpenVS_stopOpenvswitch();
   }
   cmsObj_free((void **) &openVSCfg);	
   return ret;
}


CmsRet rutOpenVS_startOpenvswitch(void)
{
   char cmdStr[CMS_MAX_FULLPATH_LENGTH];
   struct utsname kernel;
	
   if (uname(&kernel) == -1) 
   {
      cmsLog_error("Failed to get kernel version");
      return CMSRET_INTERNAL_ERROR;
   }
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,11,0))
   /*from install.md, version early than 3.11.0,should not load ip_gre.ko or build it in*/
   /*here try to remove it first*/
   snprintf(cmdStr, sizeof(cmdStr), "rmmod ip_gre > /dev/null 2>&1");   
   rut_doSystemAction("rut", cmdStr);
#endif	
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/netfilter/nf_conntrack.ko > /dev/null 2>&1", kernel.release);	
   rut_doSystemAction("rut", cmdStr);
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/netfilter/xt_conntrack.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/ipv4/netfilter/nf_defrag_ipv4.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/ipv6/netfilter/nf_defrag_ipv6.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/netfilter/nf_nat.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/ipv4/netfilter/nf_conntrack_ipv4.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/ipv4/netfilter/nf_nat_ipv4.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/ipv6/netfilter/nf_conntrack_ipv6.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/ipv6/netfilter/nf_nat_ipv6.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
   snprintf(cmdStr, sizeof(cmdStr), "insmod /lib/modules/%s/kernel/net/ipv4/gre.ko > /dev/null 2>&1", kernel.release);   
   rut_doSystemAction("rut", cmdStr);
   rut_doSystemAction("rut", OVS_START_SCRIPT);
   return CMSRET_SUCCESS;
}

CmsRet rutOpenVS_stopOpenvswitch(void)
{
   rut_doSystemAction("rut", OVS_STOP_SCRIPT);
   return CMSRET_SUCCESS;
}

CmsRet rutOpenVS_updateOpenvswitchOFController( const char *ofControllerAddr,UINT32 ofControllerPort)
{
   char cmdLine[BUFLEN_128];
 
   snprintf(cmdLine, sizeof(cmdLine), "%s set-controller %s tcp:%s:%d", OVS_VSCTL,OVS_BRIDGE,ofControllerAddr,ofControllerPort);
   rut_doSystemAction("rut", cmdLine);
   return CMSRET_SUCCESS;
}

CmsRet rutOpenVS_updateOpenvswitchPorts(const char *oldOpenVSports,const char *newOpenVSports)
{
   char *tmpOldPortsList = NULL, *tmpNewPortsList=NULL;
   char *currIfName, *ptr, *saveOldPortsPtr=NULL, *saveNewPortsPtr=NULL;
   if( oldOpenVSports != NULL)
      tmpOldPortsList = cmsMem_strdup(oldOpenVSports);
   if( newOpenVSports != NULL)
      tmpNewPortsList = cmsMem_strdup(newOpenVSports);
   if (!tmpOldPortsList || !tmpNewPortsList)
   {
      cmsLog_error("Memory allocation failure");
      cmsMem_free(tmpOldPortsList);
      cmsMem_free(tmpNewPortsList);
      return CMSRET_RESOURCE_EXCEEDED;
   }
 
   ptr = strtok_r(tmpOldPortsList, ",", &saveOldPortsPtr);
   /* check those interface deleted */
   while (ptr )
   {
      currIfName=ptr;
      while ((isspace(*currIfName)) && (*currIfName != 0))
      {
         /* skip white space after comma */
         currIfName++;
      }
      /*interfaces is deleted from ports list*/
      if( strstr(newOpenVSports, currIfName) == NULL)
      {
         rutOpenVS_deleteOpenVSport(currIfName);   
      }
      ptr = strtok_r(NULL, ",", &saveOldPortsPtr);
   }
   cmsMem_free(tmpOldPortsList);
   ptr = strtok_r(tmpNewPortsList, ",", &saveNewPortsPtr);
   /* check those interface new added */
   while (ptr )
   {
      currIfName=ptr;
      while ((isspace(*currIfName)) && (*currIfName != 0))
      {
         /* skip white space after comma */
         currIfName++;
      }
	
      /*interfaces is newly added into openvswitch ports list*/
      if( strstr(oldOpenVSports, currIfName) == NULL)
      {
         rutOpenVS_addOpenVSport(currIfName);	 
      }
      ptr = strtok_r(NULL, ",", &saveNewPortsPtr);
   }
   cmsMem_free(tmpNewPortsList);
   return CMSRET_SUCCESS;
}

UBOOL8 rutOpenVS_isValidBridgeWanInterface(const char *ifName)
{
   InstanceIdStack iidStack;
   _WanIpConnObject  *ipConn = NULL;
   UBOOL8 found = FALSE;
   cmsLog_debug("Enter: ifName=%s", ifName);
   INIT_INSTANCE_ID_STACK(&iidStack);
   /* get the related ipConn obj */
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, ipConn->X_BROADCOM_COM_IfName) && !cmsUtl_strcmp(ipConn->connectionType, MDMVS_IP_BRIDGED))
      {
         found = TRUE;            
      }
      cmsObj_free((void **) &ipConn);
   }

   cmsLog_debug("Exit: ifName=%s found=%d", ifName, found);
   return found;
}


#if defined(SUPPORT_DM_LEGACY98) || defined(SUPPORT_DM_HYBRID) || defined(SUPPORT_DM_DETECT)
CmsRet rutOpenVS_deleteOpenVSport_igd(const char *ifName)
{
   char cmdLine[BUFLEN_128];
   InstanceIdStack iidStack;
   _WanIpConnObject  *ipConn = NULL;
   UBOOL8 isWanIntf = FALSE;
#ifdef DMP_BRIDGING_1
   UINT32 defaultBridgeRef = 0;
#endif
   cmsLog_debug("Enter: ifName=%s", ifName);
   /* for any delete interfaces 
    1. delete it from openvswitch
    2. add the port back to linux default bridge
   */
   /* this function should be called when openswitch is running*/
   snprintf(cmdLine, sizeof(cmdLine), "%s --if-exists del-port %s %s", OVS_VSCTL,OVS_BRIDGE,ifName);
   rut_doSystemAction("rut", cmdLine);
 
   INIT_INSTANCE_ID_STACK(&iidStack);
   /* get the related ipConn obj */
   while (!isWanIntf &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, ipConn->X_BROADCOM_COM_IfName) && !cmsUtl_strcmp(ipConn->connectionType, MDMVS_IP_BRIDGED))
      {
         isWanIntf = TRUE; 		   
         break;
      }
      cmsObj_free((void **) &ipConn);
   }

   /* check if it is wan, if no, assume it is lan*/
#ifdef DMP_BRIDGING_1
   rutPMap_addAvailableInterface(ifName, isWanIntf);
   rutPMap_addFilter(ifName, isWanIntf, defaultBridgeRef);
#endif
   if((isWanIntf == FALSE) || ( isWanIntf &&   !cmsUtl_strcmp(ipConn->connectionStatus, MDMVS_CONNECTED)))
   {
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */      
      char bridgeIfName[CMS_IFNAME_LENGTH]; 
      /* the filter object is already in mdm, so just the action
       * ie. add the wan interface to the interface group with correct bridge ifName
      */			
      if ((rutPMap_getBridgeIfNameFromIfName(ifName, bridgeIfName, isWanIntf)) == CMSRET_SUCCESS)
      {
         cmsLog_debug("Just the action to add wan intf to the interface group (%).", bridgeIfName);
         rutLan_addInterfaceToBridge(ifName, isWanIntf, bridgeIfName);
      }
#else
      rutLan_addInterfaceToBridge(ifName, isWanIntf, "br0");
#endif 
   }
 
   if( isWanIntf )
      cmsObj_free((void **) &ipConn);
   cmsLog_debug("ifName=%s ", ifName);
   return CMSRET_SUCCESS;
}

/* for any new interfaces added into openvswitch 
 1. delete the port back to linux default bridge
 2. add it as new openvswitch port
 */
/* this function should be called when openswitch is running*/

CmsRet rutOpenVS_addOpenVSport_igd(const char *ifName)
{
   CmsRet ret=CMSRET_SUCCESS;
   char cmdLine[BUFLEN_128];
   InstanceIdStack iidStack;
   _WanIpConnObject  *ipConn = NULL;
   UBOOL8 isWanIntf = FALSE,bAddToVS =FALSE;
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */
   char fullName[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   char availInterfaceReference[BUFLEN_256];
   _L2BridgingIntfObject *availIntfObj=NULL;
#endif	
	
   cmsLog_debug("Enter: ifName=%s", ifName);
   INIT_INSTANCE_ID_STACK(&iidStack);
   /* get the related ipConn obj */
   while (!isWanIntf &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ifName, ipConn->X_BROADCOM_COM_IfName) && !cmsUtl_strcmp(ipConn->connectionType, MDMVS_IP_BRIDGED))
      {
         isWanIntf = TRUE; 		   
         break;
      }
      cmsObj_free((void **) &ipConn);
   }

   /* check if it is wan, if no, assume it is lan*/
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */      
   if (isWanIntf)
      ret = rutPMap_wanIfNameToAvailableInterfaceReference(ifName, fullName);
   else
      ret= rutPMap_lanIfNameToAvailableInterfaceReference(ifName, fullName);
#endif
  	
   if((isWanIntf == FALSE) || ( isWanIntf &&	!cmsUtl_strcmp(ipConn->connectionStatus, MDMVS_CONNECTED)))
   {
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */      
      char bridgeIfName[CMS_IFNAME_LENGTH]; 
      if( ret == CMSRET_SUCCESS)
      {
         if ((rutPMap_getBridgeIfNameFromIfName(ifName,bridgeIfName, isWanIntf)) == CMSRET_SUCCESS)
         {
            rutLan_removeInterfaceFromBridge(ifName,bridgeIfName);
         }
      }
#else
      rutLan_removeInterfaceFromBridge(ifName, "br0");
#endif 
      bAddToVS = TRUE;
   }

#ifdef DMP_BRIDGING_1
   if( ret == CMSRET_SUCCESS)
   {
      strncpy(availInterfaceReference, fullName, sizeof(availInterfaceReference));
      availInterfaceReference[strlen(availInterfaceReference)-1] = '\0';	 
      if ((ret = rutPMap_getAvailableInterfaceByRef(availInterfaceReference, &iidStack, &availIntfObj)) == CMSRET_SUCCESS)
      {
         rutPMap_deleteFilter(fullName, isWanIntf);
         rutPMap_deleteAvailableInterface(fullName, isWanIntf);
      }
   }
#endif
   if( isWanIntf )
      cmsObj_free((void **) &ipConn);
   else
      rutLan_enableInterface(ifName);

   if (bAddToVS)
   {
      snprintf(cmdLine, sizeof(cmdLine), "%s --may-exist add-port %s %s", OVS_VSCTL,OVS_BRIDGE,ifName);
      rut_doSystemAction("rut", cmdLine);
   }
   cmsLog_debug("ifName=%s ", ifName);
   return ret;
}
#endif

#if defined(SUPPORT_DM_PURE181) || defined(SUPPORT_DM_DETECT)
static UBOOL8 rut_OpenVS_getWanIpInterfaceObjByIfName_dev2(const char *ifName,
                                                           Dev2IpInterfaceObject **ipIntfObj,
                                                           InstanceIdStack *iidStack)
{
   UBOOL8 found=FALSE;

   while (!found &&
          cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) ipIntfObj) == CMSRET_SUCCESS)
   {
      if ( ( (*ipIntfObj)->X_BROADCOM_COM_Upstream || (*ipIntfObj)->X_BROADCOM_COM_BridgeService )&&
           !cmsUtl_strcmp((*ipIntfObj)->name, ifName) )  
      {
         found = TRUE;
      }
      else
      {
         cmsObj_free((void **) ipIntfObj);
      }
   }

   return found;
}

CmsRet rutOpenVS_deleteOpenVSport_dev2(const char *ifName)
{
   CmsRet ret=CMSRET_SUCCESS;
   char cmdLine[BUFLEN_128];
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack ipIntfIidStack = EMPTY_INSTANCE_ID_STACK;

   /* this function should be called when openswitch is running*/
   snprintf(cmdLine, sizeof(cmdLine), "%s --if-exists del-port %s %s", OVS_VSCTL,OVS_BRIDGE,ifName);
   rut_doSystemAction("rut", cmdLine);
   ret =  rutBridge_addIntfNameToBridge_dev2(ifName,"br0");
   //for wan bridge interface
   if (rut_OpenVS_getWanIpInterfaceObjByIfName_dev2(ifName, &ipIntfObj, &ipIntfIidStack))
   {
      if (ipIntfObj->X_BROADCOM_COM_BridgeService)
      {
         CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->X_BROADCOM_COM_BridgeName, "br0", 
           mdmLibCtx.allocFlags);
      }
      else
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfObj->X_BROADCOM_COM_BridgeName);
      }
      CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfObj->X_BROADCOM_COM_GroupName);
      ret = cmsObj_set((void *) ipIntfObj, &ipIntfIidStack);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Set of IP.Interface bridgeName failed,ret=%d", ret);
      }
      cmsObj_free((void **) &ipIntfObj);
   }
   return ret;
}

CmsRet rutOpenVS_addOpenVSport_dev2(const char *ifName)
{
   char cmdLine[BUFLEN_128];
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack ipIntfIidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 foundWan=FALSE;
   UBOOL8 isBridgeService=FALSE;
   CmsRet ret=CMSRET_SUCCESS;
   UBOOL8 bAddToVS =TRUE;
   /* See if this intfName is a WAN interface, we will need to update the
    * X_BROADCOM_COM_BridgeName as part of this move
    */
   foundWan = rut_OpenVS_getWanIpInterfaceObjByIfName_dev2(ifName, &ipIntfObj, &ipIntfIidStack);
   if (foundWan)
   {
      bAddToVS = FALSE;	
      isBridgeService = ipIntfObj->X_BROADCOM_COM_BridgeService;
      if ( isBridgeService)
      {
         if( !cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus,MDMVS_SERVICEUP) )
           bAddToVS =TRUE;
 
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfObj->X_BROADCOM_COM_BridgeName);
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfObj->X_BROADCOM_COM_GroupName);
         ret = cmsObj_set((void *) ipIntfObj, &ipIntfIidStack);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Setting new bridgeName failed, ret=%d", ret);
         }
      }
      cmsObj_free((void **) &ipIntfObj);
   }
 
   if (!foundWan || isBridgeService)	
   {
      /* remove this interface from its current bridge */
      rutBridge_deleteIntfNameFromBridge_dev2(ifName);
      if (bAddToVS)
      {
         snprintf(cmdLine, sizeof(cmdLine), "%s --may-exist add-port %s %s", OVS_VSCTL,OVS_BRIDGE,ifName);
         rut_doSystemAction("rut", cmdLine);
      }
   }
 
   cmsLog_debug("Exit: ret=%d", ret);
   return ret;
}
#endif

void rutOpenVS_startupOpenVSport(const char *ifName)
{
   char cmdLine[BUFLEN_128];
   snprintf(cmdLine, sizeof(cmdLine), "%s --may-exist add-port %s %s", OVS_VSCTL,OVS_BRIDGE,ifName);
   rut_doSystemAction("rut", cmdLine);
   return;
}

void rutOpenVS_shutdownOpenVSport(const char *ifName)
{
   char cmdLine[BUFLEN_128];
   snprintf(cmdLine, sizeof(cmdLine), "%s --if-exists del-port %s %s", OVS_VSCTL,OVS_BRIDGE,ifName);
   rut_doSystemAction("rut", cmdLine);
   return;
}
#endif
