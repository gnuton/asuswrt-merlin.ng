/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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
#include <sys/utsname.h>

#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rut_util.h"
#include "rut_ethswitch.h"
#include "ethswctl_api.h"
#include "rut_multicast.h"
#include "rut_pmap.h"
#include "rut_lan.h"


#define LAN_DATA_PATH            "/var/lan"
#define IFC_LAN_MAX             4
#define IFC_ENET_ID             1
#define IFC_ENET1_ID            (IFC_ENET_ID+1)
#define IFC_USB_ID              (IFC_ENET_ID + IFC_LAN_MAX)
#define IFC_HPNA_ID             (IFC_USB_ID + IFC_LAN_MAX)
#define IFC_WIRELESS_ID         (IFC_HPNA_ID + IFC_LAN_MAX)
#define IFC_SUBNET_ID           (IFC_WIRELESS_ID + IFC_LAN_MAX) // for sencond LAN subnet
#define IFC_ENET0_VNET_ID       (IFC_SUBNET_ID + IFC_LAN_MAX)
#define IFC_ENET1_VNET_ID       (IFC_ENET0_VNET_ID + IFC_LAN_MAX)
#define START_PMAP_ID           2


char *rut_pmapGetIfcNameById(int ifcId, char *ifcName) {
   if ( ifcName == NULL ) return NULL;

   ifcName[0] = '\0';

   if ( ifcId >= IFC_ENET_ID && ifcId < IFC_USB_ID )
      sprintf(ifcName, "eth%d", ifcId - IFC_ENET_ID);
   else if ( ifcId >= IFC_USB_ID && ifcId < IFC_HPNA_ID )
      sprintf(ifcName, "usb%d", ifcId - IFC_USB_ID);
   else if ( ifcId >= IFC_HPNA_ID && ifcId < IFC_WIRELESS_ID )
      sprintf(ifcName, "il%d", ifcId - IFC_HPNA_ID);
   else if ( ifcId >= IFC_WIRELESS_ID && ifcId < IFC_WIRELESS_ID + IFC_LAN_MAX ) {
      int num = ifcId - IFC_WIRELESS_ID;
      if (num == 0) { // multiple ssid support
         sprintf(ifcName, "wl0");
      }
      else {
         sprintf(ifcName, "wl0.%d", num);
      }
   }
   else if (ifcId >= IFC_ENET0_VNET_ID && ifcId < IFC_ENET0_VNET_ID + IFC_LAN_MAX + 2)
       sprintf(ifcName, "eth0.%d", ifcId - IFC_ENET0_VNET_ID);
   else if (ifcId >= IFC_ENET1_VNET_ID && ifcId < IFC_ENET1_VNET_ID + IFC_LAN_MAX + 2)
       sprintf(ifcName, "eth1.%d", ifcId - IFC_ENET1_VNET_ID);

   return ifcName;
}

#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */
static void rut_pmapCreateDataDirectory(UINT16 lanId) {
    char cmd[BUFLEN_512];
    char file[BUFLEN_264];
    char interface[BUFLEN_16];

    // get interface name
    if ( lanId != IFC_SUBNET_ID )
        rut_pmapGetIfcNameById(lanId, interface);
    else
        strcpy(interface, "br0:0");

    // if /var/lan/%s already exists, do nothing
    sprintf(file, "%s/%s", LAN_DATA_PATH, interface);
    if (!access(file, F_OK))
        return;

    // create /var/lan/%s directory
    sprintf(file, "%s/%s", LAN_DATA_PATH, interface);
    sprintf(cmd, "mkdir -p %s", file);
    rut_doSystemAction("rut_pmapCreateDataDirectory", cmd);

    // init
    sprintf(file, "%s/%s/linkstate", LAN_DATA_PATH, interface);
    sprintf(cmd, "echo 00 > %s", file);
    rut_doSystemAction("rut_pmapCreateDataDirectory", cmd);
}

static void rut_pmapSetLinkState(UINT16 lanId, UINT16 linkState) {
   char interface[BUFLEN_32];
   char  file[BUFLEN_128];
   char data[BUFLEN_128];
   char buff[BUFLEN_512];

    interface[0] = '\0'; file[0] = '\0'; data[0] = '\0';

    // get interface name
    if ( lanId != IFC_SUBNET_ID )
        rut_pmapGetIfcNameById(lanId, interface);
    else
        strcpy(interface, "br0:0");

    sprintf(file, "%s/%s/linkstate", LAN_DATA_PATH, interface);
    sprintf(data, "%d", linkState);
    sprintf(buff, "echo %s > %s", data, file);
    rut_doSystemAction("rut_pmapSetLinkState", buff);

}
#endif


#ifdef DMP_BASELINE_1
/* Is this virtualPorts stuff still supported?  Keep it in TR98 only for now */
void rutEsw_createVirtualPorts(const char *ifName, UINT32 numIfc)
{
#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */
   int vnet_base;
#endif
   UINT32 i = 0;
   char fullName[BUFLEN_32];

   /*
    * First delete the base interface (eth1).
    */
#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */
   /*
    * Remove the base ethernet interface (e.g. eth1) from the list
    * of available bridging interfaces.
    */
   {
      UBOOL8 isWanIntf=FALSE;
      char fullPathName[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      CmsRet ret;

      if ((ret= rutPMap_lanIfNameToAvailableInterfaceReference(ifName, fullPathName)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutPMap_lanIfNameToAvailableInterfaceReference Fail. ret=%d", ret);
      }
      else
      {
         rutPMap_deleteFilter(fullPathName, isWanIntf);
         rutPMap_deleteAvailableInterface(fullPathName, isWanIntf);
      }
   }
#endif


   /*
    * We don't actually want to delete the base interface completely.
    * We just want to remove it from the bridge, but leave it ifconfig'd up.
    */
   rutLan_removeInterfaceFromBridge(ifName, "br0"); 

   /*
    * Now add the virtual ports/interfaces.
    */
#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */
   vnet_base = strcmp(ifName, "eth1") ? IFC_ENET0_VNET_ID: IFC_ENET1_VNET_ID;
#endif

   for (i = 0; i < numIfc; i++)
   {   
      /*
       * Virtual port/interface names have the form eth1.2, eth1.3, etc.
       */
      snprintf(fullName, sizeof(fullName), "%s.%d", ifName, START_PMAP_ID + i);
    
      rutLan_addEthInterface(fullName);
      
#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */
      {
         UBOOL8 isWanIntf=FALSE;
         SINT32 defaultBridgeRef=0;
         
         rutPMap_addAvailableInterface(fullName, isWanIntf);      
         rutPMap_addFilter(fullName, isWanIntf, defaultBridgeRef);
         
      
         /* mwang: the next two functions create directories and files for
          * linkstate.  legacy cfm stuff.  Keep it until I figure out how
          * to replace it with CMS stuff.
          */
         rut_pmapCreateDataDirectory(vnet_base + i + 2);
         rut_pmapSetLinkState(vnet_base + i + 2, 1);
      }
#endif
   }

   return;
}


void rutEsw_deleteVirtualPorts(const char *ifName, UINT32 numIfc)
{
   UINT32 i;
   UBOOL8 isWanIntf=FALSE;
   char fullName[BUFLEN_32];
   
   for (i=0; i < numIfc; i++)
   {
      snprintf(fullName, sizeof(fullName), "%s.%d", ifName, START_PMAP_ID + i);
      
#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */
      {
         UBOOL8 isWanIntf=FALSE; 
         char fullPathName[MDM_SINGLE_FULLPATH_BUFLEN]={0};
         CmsRet ret;
            
         if ((ret= rutPMap_lanIfNameToAvailableInterfaceReference(fullName, fullPathName)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutPMap_lanIfNameToAvailableInterfaceReference Fail. ret=%d", ret);
         }
         else
         {
            rutPMap_deleteFilter(fullPathName, isWanIntf);
            rutPMap_deleteAvailableInterface(fullPathName, isWanIntf);
         }
      }
#endif

      rutLan_deleteEthInterface(fullName);      
   }
   
   /*
    * Now add the base ethernet interface (e.g. eth1) back to the
    * default bridge.
    */
   rutLan_addInterfaceToBridge(ifName, isWanIntf, "br0");
   
#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */
   {
      UBOOL8 isWanIntf=FALSE;
      SINT32 defaultBridgeRef=0;
      
      rutPMap_addAvailableInterface(ifName, isWanIntf);
      rutPMap_addFilter(ifName, isWanIntf, defaultBridgeRef);
   }
#endif

   return;      
}
#endif  /*  DMP_BASELINE_1 */


/* what is the relationship of this function and the board ioctl getNumberOfEnetMacs
 * and getNumberofEnet ports?
 */
UINT32 rutEsw_getNumberOfVirtualPorts(const char *interfaceName)
{
#ifdef DESKTOP_LINUX
   /*
    * On desktop, fake it so eth1 has 3 ports.
    */
   if (!strcmp(interfaceName, "eth1"))
   {
      return 3;
   }
   else
   {
      return 1;
   }
#else
   FILE *errFs = NULL;
   char cmd[BUFLEN_264];
   char str[BUFLEN_264];
   UINT32 ports;


   sprintf(cmd, "ethctl %s vport query 2>/var/vcfgerr\n", interfaceName);
   rut_doSystemAction("getNumberOfVirtualPorts", cmd);
   /* Check the status of the previous command */
   errFs = fopen("/var/vcfgerr", "r");
   if (errFs == NULL )
   {
      cmsLog_error("Could not open /var/vcfgerr to get number of virtual ports, default to 1");
      ports = 1;
   }
   else
   {
      fgets(str, sizeof(str), errFs);
      ports = atoi(str);
      fclose(errFs);
      unlink("/var/vcfgerr");
   }

   return ports;
#endif
}


void rutEsw_insertEthernetSwitchKernelModule(void)
{
#ifdef DESKTOP_LINUX
   cmsLog_debug("pretend to insert kernel module");
#else
   struct utsname utsInfo;
   char cmd[BUFLEN_256];

   if (uname(&utsInfo) == -1)
   {
      sprintf(cmd, "insmod /lib/modules/%s/extra/vnet.ko", "2.6.8.1");
   }
   else
   {
      sprintf(cmd, "insmod /lib/modules/%s/extra/vnet.ko", utsInfo.release);
   }

   rut_doSystemAction("insertEthernetSwitchKernelModule",cmd);
#endif
}


void rutEsw_removeEthernetSwitchKernelModule(void)
{
#ifdef DESKTOP_LINUX
   cmsLog_debug("pretend to remove kernel module");
#else
   rut_doSystemAction("removeEthernetSwitchKernelModule","rmmod vnet.ko");
#endif
}


UBOOL8 rutEsw_isVirtualPortsEnabled(void)
{
   EthernetSwitchObject *ethSwitchObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 flags=0;
   UBOOL8 isEnabled=FALSE;
   CmsRet ret;   

   if ((ret = cmsObj_get(MDMOID_ETHERNET_SWITCH, &iidStack, flags, (void **) &ethSwitchObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get ethSwitch object, ret=%d", ret);
   }
   else
   {
      isEnabled = ethSwitchObj->enableVirtualPorts;
      cmsObj_free((void **) &ethSwitchObj);
   }
   
   return isEnabled;
}


UBOOL8 rutEsw_isEthernetSwitchIfNameMatch(const char *ifName)
{
   EthernetSwitchObject *ethSwitchObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 flags=0;
   UBOOL8 isMatch=FALSE;
   CmsRet ret;   
   
   if ((ret = cmsObj_get(MDMOID_ETHERNET_SWITCH, &iidStack, flags, (void **) &ethSwitchObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get ethSwitch object, ret=%d", ret);
   }
   else
   {
      isMatch = (0 == cmsUtl_strcmp(ethSwitchObj->ifName, ifName));
      cmsObj_free((void **) &ethSwitchObj);
   }
   
   return isMatch;
}


UBOOL8 rutEsw_isEthernetSwitchOnEth0(void)
{
   return (rutEsw_isEthernetSwitchIfNameMatch("eth0"));
}


void rutEsw_updatePortPauseFlowCtrlSetting(UINT32 excludeClassKey)
{
   UBOOL8 setAuto=TRUE;

   cmsLog_debug("Entered: excludeClassKey=0x%x", excludeClassKey);

   if (rutMulti_isMcastQosEnabled())
   {
      cmsLog_debug("port pause NONE because mcast Qos is enabled");
      setAuto = FALSE;
   }

   if (setAuto)
   {
      if (qdmQos_isEgressEthPortClassificationPresentLocked(excludeClassKey))
      {
         cmsLog_debug("port pause NONE because Egress Eth port class found");
         setAuto = FALSE;
      }
   }

#if (!(defined(CHIP_63138) || defined(CHIP_63148) || defined(CHIP_63158) || defined(CHIP_4908) || defined(CHIP_63178) || defined(CHIP_47622)))
   if (setAuto)
   {
      cmsLog_notice("Set port pause to FLOW_CTRL_AUTO!!");
      bcm_port_pause_capability_set(0, bcm_enet_map_oam_idx_to_phys_port(8), PAUSE_FLOW_CTRL_AUTO);
   }
   else
   {
      cmsLog_notice("Set port pause to FLOW_CTRL_NONE!!");
      bcm_port_pause_capability_set(0, bcm_enet_map_oam_idx_to_phys_port(8), PAUSE_FLOW_CTRL_NONE);
   }
#endif //! SF2 based platforms
}

/*
 * this is a private helper function.
 * External callers should call rutEsw_updateRealHwSwitchingSetting.
 */
static UBOOL8 isRealHwSwitchingAllowed(UINT32 excludeClassKey)
{
   UBOOL8 isAllowedByConfig=FALSE;

#ifdef SUPPORT_CMS_ALLOW_REAL_HW_SWITCHING
   isAllowedByConfig = TRUE;
#endif

   if (isAllowedByConfig)
   {
      UBOOL8 isPresent;

      isPresent = qdmQos_isLanSwitchPortToLanSwitchPortClassificationPresentLocked(excludeClassKey);
      if (isPresent)
      {
         /*
          * There is a classifier for packets that come in on LAN eth switch
          * port and goes out LAN eth switch port.  So cannot do real HW
          * switching.
          */
         cmsLog_debug("Real HW Switching NOT allowed because switch LAN port to LAN port class found");
         return FALSE;
      }

#ifdef SUPPORT_LANVLAN
      isPresent = qdmVlan_isLanVlanPresentLocked();
      if (isPresent)
      {
         cmsLog_debug("Real HW switching NOT allowed because of LANVLAN");
         return FALSE;
      }
#endif

      /*
       * If we get here, then we are allowed to do Real HW switching by config,
       * we did not detect any Lan to Lan classifiers, and we did not detect
       * any LAN VLANs, so we _CAN_ do Real HW switching!
       */
      cmsLog_debug("Real HW switching allowed");
      return TRUE;
   }
   else
   {
      cmsLog_debug("Real HW switching NOT allowed because not enabled in menuconfig");
      return FALSE;
   }
}


void rutEsw_updateRealHwSwitchingSetting(UINT32 excludeClassKey)
{
   UBOOL8 allowed=FALSE;

   cmsLog_debug("Entered: excludeClassKey=%d", excludeClassKey);

   allowed = isRealHwSwitchingAllowed(excludeClassKey);

   if (allowed)
   {
      cmsLog_notice("ENABLE Real HW switching!!");
      ethswctl_enable_switching();

   }
   else
   {
      cmsLog_notice("DISABLE Real HW switching!!");
      ethswctl_disable_switching();
   }
}





