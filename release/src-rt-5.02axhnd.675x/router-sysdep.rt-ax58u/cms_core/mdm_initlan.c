/*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
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
*/


#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"
#include "cms_net.h"
#include "cms_qdm.h"

#if defined(DMP_X_BROADCOM_COM_RDPA_1)
#include "rdpactl_api.h"
#include "ethswctl_api.h"

/* MDMVS_FILTER_ETYPE_UDEF entry will use the following value */
#define INIT_FILTER_INBAND_FILTER_VAL 0x888A
#ifndef G9991
const char *defaultLanIngressFiltersTypeStr[] = {
        MDMVS_FILTER_DHCP,
        MDMVS_FILTER_IGMP,
        MDMVS_FILTER_MLD,
        MDMVS_FILTER_ARP,
        MDMVS_FILTER_802_1AG_CFM,
        MDMVS_FILTER_BCAST,
        MDMVS_FILTER_IP_FRAG,
        MDMVS_FILTER_HDR_ERR,
#if defined(CONFIG_BCM_PTP_1588)
        MDMVS_FILTER_PTP_1588,
        MDMVS_FILTER_L4_PTP_1588,
#endif
};
#else
const char *defaultLanIngressFiltersTypeStr[] = {
        MDMVS_FILTER_IGMP,
        MDMVS_FILTER_ETYPE_UDEF,
};
#endif
CmsRet lanDefaultFiltersConfigByName(const char *ifName);
CmsRet lanDefaultFiltersConfig(int rdpaIf_p);
CmsRet allLanFiltersDefaultConf(void);
#endif

static CmsRet addDefaultLanDeviceObject(void);
static CmsRet addLanEthInterfaceObject(const char *ifName,  
                                const char *pGMACPortList,
                                const char *pWANOnlyPortList,
                                const char *pLANOnlyPortList);

#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1
/* NotUsed static CmsRet addPersistentWanMocaInterfaceObject(const char *ifName);
*/
#endif 

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1   
#ifdef DMP_X_BROADCOM_COM_EPON_1      
static CmsRet addPersistentWanEponInterfaceObject(const char *ifName);
#endif
#endif

#ifdef DMP_X_BROADCOM_COM_EPON_1
static CmsRet addLanEponInterfaceObject(const char *ifName);
#endif

#ifdef DMP_USBLAN_1      
static CmsRet addLanUsbInterfaceObject(const char *ifName);
#endif

#ifdef DMP_X_BROADCOM_COM_MOCALAN_1
CmsRet addLanMocaInterfaceObject(const char *ifName);
#endif

#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
static CmsRet addPersistentWanWifiInterfaceObject(const char *ifName);
#endif /* DMP_X_BROADCOM_COM_WIFIWAN_1 */

#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */
static CmsRet addDefaultL2BridgingEntryObject(void);
static CmsRet addDefaultL2BridgingFilterInterfaceObject(UINT32 intfKey, SINT32 bridgeRef);
CmsRet addDefaultL2BridgingAvailableInterfaceObject(const char *interfaceReference, SINT32 bridgeRef);
#endif



CmsRet mdm_addDefaultLanObjects(void)
{
   void *mdmObj=NULL;
   LanDevObject *lanDevObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char *ifNames=NULL;
   char ifNameBuf[CMS_IFNAME_LENGTH];
   UINT32 end, c=0;
   UINT32 idx;
   CmsRet ret;
   char *pWANOnlyPortList=NULL;
   char *pLANOnlyPortList=NULL;   
   char *pGMACPortList=NULL;  /* WAN Preferred eth port list */
   
#ifdef DMP_BRIDGING_1
   L2BridgingObject *l2BridgingObj=NULL;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
#endif

   ret = mdm_getNextObject(MDMOID_LAN_DEV, &iidStack, &mdmObj);
   if (ret == CMSRET_SUCCESS)
   {
      /*
       * if we detect a LANDevice, assume all devices have been added.
       * We do not go further and make sure every device is in the data model.
       * This means once you have booted the modem, you cannot add hardware
       * or enable extra lan drivers because this code will not detect it.
       *
       * That could be changed if needed though.... For example, notice
       * how we will detect the homeplug interface and add it even if
       * the LAN_DEV is found.
       */
      mdm_freeObject(&mdmObj);
#if defined(DMP_X_BROADCOM_COM_RDPA_1)
      ret = allLanFiltersDefaultConf();
#endif  

#ifdef DMP_DEVICE2_HOMEPLUG_1
      if (isPlcInterfaceExist())
      {
         cmsLog_debug("Found plc0");
         /*plc0 is always a LAN port and is added to br0  if it is found in
         * the system.  Currently IGD.Device.Homeplug.Interface{i} is not saved as
         * part of lan configuration (only in mdm) like others: eth0,... 
         */
         mdmInit_addHomePlugInterfaceObject("plc0", FALSE, NULL);
      }         
#endif /* DMP_DEVICE2_HOMEPLUG_1 */
      
      return CMSRET_SUCCESS;
   }

   if ((ret = addDefaultLanDeviceObject()) != CMSRET_SUCCESS)
   {
      return ret;
   }


   /*
    * We also need to update the various counters in LANDevice.  Get the object first
    * and then update the fields as we go along.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = mdm_getNextObject(MDMOID_LAN_DEV, &iidStack, (void **) &lanDevObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get LAN DEV object, ret=%d", ret);
      return ret;
   }



#ifdef DMP_BRIDGING_1
      INIT_INSTANCE_ID_STACK(&iidStack2);
      if ((ret = mdm_getObject(MDMOID_L2_BRIDGING, &iidStack2, (void **) &l2BridgingObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get LAYER2 BRIDGING object, ret=%d", ret);
         return ret;
      }

   if ((ret = addDefaultL2BridgingEntryObject()) != CMSRET_SUCCESS)
   {
      return ret;
   }

      l2BridgingObj->bridgeNumberOfEntries = 1;
#endif
   
   /*
    * If there are any GMAC ports (called Preferred WAN ports in driver) in the system, just get the list.
    */
   cmsNet_getGMACPortIfNameList(&pGMACPortList);
   cmsLog_debug("GbMAC PortList=%s", pGMACPortList);
   /*
    * If there are WAN only Eth port list in the system, get that first.
    */
   cmsNet_getWANOnlyEthPortIfNameList(&pWANOnlyPortList);
   cmsLog_debug("WANOnlyPortList=%s", pWANOnlyPortList);   


   /*
    * If there are any LANOnly ports in the system, just get the list.
    */
   cmsNet_getLANOnlyEthPortIfNameList(&pLANOnlyPortList);
   cmsLog_debug("LANOnlyPortList=%s", pLANOnlyPortList);   
   
   /*
    * Now add ethernet, usb, moca LAN devices based on what the kernel
    * tells us is there.
    */
   cmsNet_getIfNameList(&ifNames);
   if (ifNames == NULL)
   {
      cmsLog_error("no interfaces found during initialization!");
      return CMSRET_INTERNAL_ERROR;
   }
   end = strlen(ifNames);

   while (c < end)
   {
      idx = 0;
      while (c < end && ifNames[c] != ',')
      {
         ifNameBuf[idx] = ifNames[c];
         c++;
         idx++;
      }
      ifNameBuf[idx] = 0;
      c++;

      if (0 == cmsUtl_strncmp(ifNameBuf, "eth", 3))
      {
#ifndef G9991
         addLanEthInterfaceObject(ifNameBuf, pGMACPortList, pWANOnlyPortList, pLANOnlyPortList);
         lanDevObj->LANEthernetInterfaceNumberOfEntries++;
#ifdef DMP_BRIDGING_1
         l2BridgingObj->filterNumberOfEntries++;
         l2BridgingObj->availableInterfaceNumberOfEntries++;
#endif
#endif  /* G9991 */
      }

#ifdef G9991
      else if (0 == cmsUtl_strncmp(ifNameBuf, "sid", 3))
      {
          addLanEthInterfaceObject(ifNameBuf, pGMACPortList, pWANOnlyPortList, pLANOnlyPortList);

          lanDevObj->LANEthernetInterfaceNumberOfEntries++;
#ifdef DMP_BRIDGING_1
          l2BridgingObj->filterNumberOfEntries++;
          l2BridgingObj->availableInterfaceNumberOfEntries++;
#endif
      }
#endif  /* G9991 */


#ifdef DMP_USBLAN_1
      else if (0 == cmsUtl_strncmp(ifNameBuf, "usb", 3))
      {
         addLanUsbInterfaceObject(ifNameBuf);

         lanDevObj->LANUSBInterfaceNumberOfEntries++;
#ifdef DMP_BRIDGING_1
         l2BridgingObj->filterNumberOfEntries++;
         l2BridgingObj->availableInterfaceNumberOfEntries++;
#endif
      }
#endif  /* DMP_USBLAN */
#ifdef DMP_X_BROADCOM_COM_MOCALAN_1
      else if (0 == cmsUtl_strncmp(ifNameBuf, "moca", 4))
      {

#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1         
         /*
          * If the the moca port is in the persistent wan list,
          * need to add that as the WAN device. 
          * NOTE: Since we only support ONE moca WAN for now,
          *       the first moca found in pWanPortList will be used and 
          *       the rest of moca in the list will be ignored.
          */
#endif  /* DMP_X_BROADCOM_COM_MOCAWAN_1 */ 
         {
      
            addLanMocaInterfaceObject(ifNameBuf);

            lanDevObj->X_BROADCOM_COM_LANMocaInterfaceNumberOfEntries++;
#ifdef DMP_BRIDGING_1
            l2BridgingObj->filterNumberOfEntries++;
            l2BridgingObj->availableInterfaceNumberOfEntries++;
#endif
         }
      }
#endif  /* DMP_X_BROADCOM_COM_MOCALAN_1 */

#ifdef DMP_X_BROADCOM_COM_EPON_1      
      if (0 == cmsUtl_strncmp(ifNameBuf, "epon", 4))
      {
#ifdef DMP_X_BROADCOM_COM_EPONWAN_1         
         /*
          * If the the epon port is in the persistent wan list,
          * need to add that as the WAN device. 
          * NOTE: Since we only support ONE epon WAN for now,
          *       the first epon found in pWanPortList will be used and 
          *       the rest of epon in the list will be ignored.
          */
         if (pWANOnlyPortList && cmsUtl_isSubOptionPresent(pWANOnlyPortList, ifNameBuf))
         {
            addPersistentWanEponInterfaceObject(ifNameBuf);
         }
         else if (!(pWANOnlyPortList && cmsUtl_isSubOptionPresent(pWANOnlyPortList, ifNameBuf)))
         {
            /* If the persistent epon wan port is not found,
               default epon0 Interface should be created by
               mdm_addDefaultWanEponObject() function */

            /* Fake a epon wan interface even if it is not found in the persistent Wan list. 
            * There might be some risk on the leakages if there are traffic on the epon port.
            * TODO:  Need to be removed later on if the boardparams.c has this set to persistent.
            */
            //cmsLog_notice("Just fake a eponWAN layer 2 interface even if the persistent epon wan port is not found!");
            //addPersistentWanEponInterfaceObject(ifNameBuf);
         }
         else
#endif  /* DMP_X_BROADCOM_COM_EPONWAN_1 */ 
         {
            /* epon0 is added to  br0 here for SFU board */
            addLanEponInterfaceObject(ifNameBuf);
            
            lanDevObj->X_BROADCOM_COM_LANMocaInterfaceNumberOfEntries++;
#ifdef DMP_BRIDGING_1
            l2BridgingObj->filterNumberOfEntries++;
            l2BridgingObj->availableInterfaceNumberOfEntries++;
#endif /* DMP_BRIDGING_1 */
         }
      }
#endif  /* DMP_X_BROADCOM_COM_EPON_1 */
#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1   	  
      else if (0 == cmsUtl_strncmp(ifNameBuf, "wl", 2))
      {  
      
         /*
          * If the the Wl is in the persistent wan list,
          * need to add that as the WAN device. 
          * NOTE: Since we only support ONE wifi WAN for now,
          *       the first wl interface found in pWanPortList will be used and 
          *       the rest of wl interfaces in the list will be ignored.
          */
         if (pWANOnlyPortList &&  cmsUtl_isSubOptionPresent(pWANOnlyPortList, ifNameBuf))
         {
            addPersistentWanWifiInterfaceObject(ifNameBuf);
         }
      }
#endif	  

#ifdef DMP_DEVICE2_HOMEPLUG_1   	  
      else if (0 == cmsUtl_strncmp(ifNameBuf, "plc", 3))
      {  
         /*plc0 is always a LAN port and is added to br0  */
         mdmInit_addHomePlugInterfaceObject(ifNameBuf, FALSE, NULL);
      }
#endif	/* DMP_DEVICE2_HOMEPLUG_1 */
  }

#if defined(DMP_X_BROADCOM_COM_RDPA_1)
   allLanFiltersDefaultConf();
#endif

   /* Free the the LAN interface name list */
   cmsMem_free(ifNames);
   
   /* No long needed, free this and set to NULL */
   CMSMEM_FREE_BUF_AND_NULL_PTR(pWANOnlyPortList);    
   CMSMEM_FREE_BUF_AND_NULL_PTR(pGMACPortList);    
   CMSMEM_FREE_BUF_AND_NULL_PTR(pLANOnlyPortList);    

   /*
    * Now do the set to save the updated counts of the various interfaces.
    */
   if ((ret = mdm_setObject((void **) &lanDevObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set LAN Dev object, ret=%d", ret);
   }

#ifdef DMP_BRIDGING_1
   if ((ret = mdm_setObject((void **) &l2BridgingObj, &iidStack2, FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set Layer2 Bridging object, ret=%d", ret);
   }
#endif

   return ret;
}


CmsRet addDefaultLanDeviceObject(void)
{
   MdmPathDescriptor pathDesc;
   _IGDObject *igdObj=NULL;
   _LanHostCfgObject *lanHostCfgObj=NULL;
   _LanIpIntfObject *ipIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

#if defined(SUPPORT_IPV6) && defined(BRCM_PKTCBL_SUPPORT)
   IPv6LanHostCfgObject *ipv6Obj = NULL;
#endif
   
   cmsLog_notice("Creating initial LAN Device / br0 LAN interface");
   

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_LAN_DEV;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to create LANDevice.1, ret=%d", ret);
      return ret;
   }

   /*
    * We also need to update the LANDeviceNumberOfEntries parameter in InternetGatewayDevice.
    */
   if ((ret = mdm_getObject(MDMOID_IGD, &iidStack, (void **) &igdObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get IGD object, ret=%d", ret);
      return ret;
   }

   igdObj->LANDeviceNumberOfEntries = 1;

   if ((ret = mdm_setObject((void **) &igdObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set IGD object, ret=%d", ret);
   }

   
   /*
    * Enable the dhcp server on this initial br0 LAN interface.
    * Also update the IPInterfaceNumberOfEntries since we will add a single IPInterface
    * later in this function.
    */
   if ((ret = mdm_getObject(MDMOID_LAN_HOST_CFG, &(pathDesc.iidStack), (void **) &lanHostCfgObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get LAN_HOST_CFG, ret=%d", ret);
      return ret;
   }

#if defined(DHCP_SERVER_DEFAULT)
   lanHostCfgObj->DHCPServerEnable = TRUE;
#else   
   lanHostCfgObj->DHCPServerEnable = FALSE;
#endif
   lanHostCfgObj->IPInterfaceNumberOfEntries = 1;
   
   if ((ret = mdm_setObject((void **) &lanHostCfgObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set LanHostCfg object, ret=%d", ret);
   }

#if defined(SUPPORT_IPV6) && defined(BRCM_PKTCBL_SUPPORT)
   /* setDns6Info begin */
   if ((ret = mdm_getObject(MDMOID_I_PV6_LAN_HOST_CFG, &(pathDesc.iidStack),
     (void **)&ipv6Obj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get MDMOID_I_PV6_LAN_HOST_CFG, ret=%d", ret);
      return ret;
   }
   CMSMEM_REPLACE_STRING_FLAGS(ipv6Obj->IPv6DNSConfigType, MDMVS_DHCP, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv6Obj->IPv6DNSWANConnection, EPON_VOICE_WAN_IF_NAME, mdmLibCtx.allocFlags);
   CMSMEM_FREE_BUF_AND_NULL_PTR(ipv6Obj->IPv6DNSServers);
   if ((ret = mdm_setObject((void **) &ipv6Obj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      mdm_freeObject((void **)&ipv6Obj);
      cmsLog_error("could not set IPv6LanHostCfg object, ret=%d", ret);
   }
   /* setDns6Info end */
#endif // SUPPORT_IPV6 && BRCM_PKTCBL_SUPPORT

   /*
    * Now add a single IP interface under the LAN device.
    * IP Interface name will be automatically assigned upon object creation,
    * we should get br0.
    */
   pathDesc.oid = MDMOID_LAN_IP_INTF;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to create LANDevice.1.IPInterface.1, ret=%d", ret);
      return ret;
   }

   cmsLog_debug("created default LAN IPInterface at %s", cmsMdm_dumpIidStack(&(pathDesc.iidStack)));
   
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &ipIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get IPInterface object, ret=%d", ret);
      return ret;
   }
   
   /* Enable IP intf */
   ipIntfObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->X_BROADCOM_COM_IfName, "br0", mdmLibCtx.allocFlags);

#if defined(DHCP_CLIENT_DEFAULT)
   /* Enable DHCP client by default */
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->IPInterfaceAddressingType, MDMVS_DHCP, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->IPInterfaceIPAddress, "0.0.0.0", mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->IPInterfaceSubnetMask, "0.0.0.0", mdmLibCtx.allocFlags);
#endif

   if ((ret = mdm_setObject((void **) &ipIntfObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set IPIntf object, ret=%d", ret);
   }

#if defined(DMP_DEVICE2_BASELINE_1) && defined(DMP_X_BROADCOM_COM_DEV2_IPV6_1) //for hybrid IPv6
   {
      char ethLinkLowerLayer[MDM_SINGLE_FULLPATH_BUFLEN]={0};
      char *ipIntfFullPath=NULL;
      MdmPathDescriptor ipIntfPathDesc;
      UBOOL8 supportIpv4=FALSE;
      UBOOL8 supportIpv6=TRUE;

      qdmEthLink_getEthLinkLowerLayerFullPathByName("br0", ethLinkLowerLayer, sizeof(ethLinkLowerLayer));

      ret = mdm_initIpObject_dev2(supportIpv4, supportIpv6);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("initIpObject failed, ret=%d", ret);
         return ret;
      }

      /* create IP.Interface object here */
      ret = mdm_addDefaultLanIpInterfaceObject_dev2("br0", "Default",
                                              supportIpv4, supportIpv6,
                                              ethLinkLowerLayer,
                                              &ipIntfPathDesc);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("addDefaultLanIpIntferfaceObject failed, ret=%d", ret);
         return ret;
      }

      if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&ipIntfPathDesc, &ipIntfFullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
         return ret;
      }

      /* IPv6 needs DHCP server and Router Advertisement on LAN side */
      mdm_addDefaultDhcpv6ServerObjects_dev2(ipIntfFullPath);
      mdm_addDefaultRouterAdvertisementObjects_dev2(ipIntfFullPath);
#ifdef DMP_DEVICE2_HOMEPLUG_1
      /*
       * Create a dhcp client to point to br0 IP.Interface.
       */
      cmsLck_acquireLock();
      mdmInit_addDhcpv6ClientObject_dev2(ipIntfFullPath);
      cmsLck_releaseLock();
#endif

      CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
   }
#endif

   return ret;
}



CmsRet addLanEthInterfaceObject(const char *ifName,  
                                const char *pGMACPortList,
                                const char *pWANOnlyPortList, 
                                const char *pLANOnlyPortList)
{
   MdmPathDescriptor pathDesc;
   _LanEthIntfObject *ethObj=NULL;
   char savedEthPortAttribute[BUFLEN_32]={0};
   CmsRet ret;

   cmsLog_debug("adding LAN %s intf object, pGMACPortList=%s, pWANOnlyPortList=%s,  pLANOnlyPortList=%s", 
      ifName, pGMACPortList, pWANOnlyPortList, pLANOnlyPortList);  
      
   INIT_PATH_DESCRIPTOR(&pathDesc);
   
   pathDesc.oid = MDMOID_LAN_ETH_INTF;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); /* default eth interface always under LANDevice.1 */

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to create default %s interface, ret=%d", ifName, ret);
      return ret;
   }
   
   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &ethObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get eth intf object, ret=%d", ret);
      return ret;
   }

   /* set the intf name and enable */   
   ethObj->enable = TRUE;

   CMSMEM_REPLACE_STRING_FLAGS(ethObj->X_BROADCOM_COM_IfName, ifName, mdmLibCtx.allocFlags);

   if (cmsUtl_isSubOptionPresent(pGMACPortList, ethObj->X_BROADCOM_COM_IfName))
   {
      /* X_BROADCOM_COM_GMAC_Enabled is used for backward compatibilty and will be depreciated later on */
      ethObj->X_BROADCOM_COM_GMAC_Enabled = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(ethObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANPREPERRED,  mdmLibCtx.allocFlags);      
   }

   if (cmsUtl_isSubOptionPresent(pWANOnlyPortList, ethObj->X_BROADCOM_COM_IfName))
   {
      CMSMEM_REPLACE_STRING_FLAGS(ethObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANONLY,  mdmLibCtx.allocFlags);
   }
   else if (cmsUtl_isSubOptionPresent(pLANOnlyPortList, ethObj->X_BROADCOM_COM_IfName))
   {
      CMSMEM_REPLACE_STRING_FLAGS(ethObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_LANONLY,  mdmLibCtx.allocFlags);
   }

   /* after mdm_setObject, ethObj->X_BROADCOM_COM_WanLan_Attribute will not be accessable, so save it */
   cmsUtl_strncpy(savedEthPortAttribute, ethObj->X_BROADCOM_COM_WanLan_Attribute, sizeof(savedEthPortAttribute)-1);
   
   if ((ret = mdm_setObject((void **) &ethObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set eth intf object, ret=%d", ret);
   }

#ifdef DMP_BRIDGING_1
   {
      char *fullPath=NULL;

      cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath);
      /* Annoying TR-98 format: remove the last . */
      fullPath[strlen(fullPath)-1] = '\0';

      ret = addDefaultL2BridgingAvailableInterfaceObject(fullPath, 0);
      cmsMem_free(fullPath);
      
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("addDefaultL2BridgingAvailableInterfaceObject returns error. ret=%d", ret);
         return ret;
      }
   }
#endif

#if 0 //defined(CHIP_63138) || defined(CHIP_63148) || defined(CHIP_4908)
#ifdef SUPPORT_QOS
   /* For not wan only ports */
   if (cmsUtl_strcmp(savedEthPortAttribute, MDMVS_WANONLY))
   {
      char *fullPath=NULL;
      
      cmsLog_notice("Init QoS Queues for %s", ifName);
      
      cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath);
      /* Annoying TR-98 format: remove the last . */
      fullPath[strlen(fullPath)-1] = '\0';

      /* Sets the default Strict Priority queues for this interface */
      ret = addDefaultEthQueueObject(fullPath, FALSE, TRUE);
      cmsMem_free(fullPath);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("addDefaultLanEthQueueObject returns error. ret=%d", ret);
         return ret;
      }
   }
#endif
#endif

   cmsLog_debug("ret %d", ret);

   return ret;
}

CmsRet delLanEthInterfaceObject(const char *ifName)
{
    InstanceIdStack iidStack;
    void *mdmObj  = NULL;

    /* get LAN Ethernet interfaces */
    INIT_INSTANCE_ID_STACK(&iidStack);
    while (cmsObj_getNext(MDMOID_LAN_ETH_INTF, &iidStack, &mdmObj) == CMSRET_SUCCESS)
    {
        if (((LanEthIntfObject *)mdmObj)->enable)
        {
            if (!cmsUtl_strcmp(ifName, ((LanEthIntfObject *)mdmObj)->X_BROADCOM_COM_IfName))
                cmsObj_deleteInstance(MDMOID_LAN_ETH_INTF, &iidStack);
        }
        cmsObj_free(&mdmObj);
    }
   return CMSRET_SUCCESS;
}

#ifdef SUPPORT_QOS
/* Sets the default Strict Priority queues for the Ethernet interface.
 * This function is used by both TR98 and TR181 code.
 */
CmsRet addDefaultEthQueueObject(const char *fullPath, UBOOL8 isWan, UBOOL8 enable)
{
   UINT32 qid;
   UINT32 precedence;
   UINT32 maxQueues;
   char qname[8];
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Entered: fullPath=%s", fullPath);

   /* create default queues for the ETH interface */
   if (isWan)
      maxQueues = MAX_ETHWAN_TRANSMIT_QUEUES;
   else
      maxQueues = MAX_ETH_TRANSMIT_QUEUES;
   
   for (precedence = 1; precedence <= maxQueues; precedence++)
   {
      /* queueId calculation */
      qid = 1 + maxQueues - precedence;

      /* queueName */
      if (isWan)
         sprintf(qname, "WAN Q%u", qid);
      else
         sprintf(qname, "LAN Q%u", qid);

      ret = mdmInit_addQosQueue(precedence, qid, fullPath, qname, enable);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_createQueue failed at precedence %d, ret=%d",
                      precedence, ret);
         break;
      }
   }

   return ret;

}
#endif  /* SUPPORT_QOS */

#ifdef DMP_USBLAN_1      
CmsRet addLanUsbInterfaceObject(const char *ifName)
{
   MdmPathDescriptor pathDesc;
   _LanUsbIntfObject *usbObj=NULL;
   CmsRet ret;   

   cmsLog_notice("adding LAN %s intf object", ifName);
      
   INIT_PATH_DESCRIPTOR(&pathDesc);
   
   pathDesc.oid = MDMOID_LAN_USB_INTF;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); /* default usb interface always under LANDevice.1 */

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to create default USB interface, ret=%d", ret);
      return ret;
   }
   
   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &usbObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get USB intf object, ret=%d", ret);
      return ret;
   }

   /* set the intf name and enable */   
   usbObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(usbObj->X_BROADCOM_COM_IfName, ifName, mdmLibCtx.allocFlags);
   
   if ((ret = mdm_setObject((void **) &usbObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set USB intf object, ret=%d", ret);
   }

#ifdef DMP_BRIDGING_1
   {
      char *fullPath=NULL;

      cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath);
      /* Annoying TR-98 format: remove the last . */
      fullPath[strlen(fullPath)-1] = '\0';

      ret = addDefaultL2BridgingAvailableInterfaceObject(fullPath, 0);
      cmsMem_free(fullPath);
   }
#endif

   cmsLog_debug("done, ret=%d", ret);

   return ret;
}
#endif /* DMP_USBLAN_1 */


#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */

CmsRet addDefaultL2BridgingEntryObject(void)
{
   CmsRet                ret        = CMSRET_SUCCESS;
   MdmPathDescriptor     pathDesc;
   _L2BridgingEntryObject *bridgeObj=NULL;


   cmsLog_notice("Adding default L2BridgingEntry");


   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_L2_BRIDGING_ENTRY;

   /* add new instance of L2BridgingObject */
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create new L2BridgingEntry, ret=%d", ret);
      return ret;
   }

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &bridgeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get L2BridgingEntryObject, ret=%d", ret);
      return ret;
   }


   /*
    * Set the object values here.
    */
   CMSMEM_REPLACE_STRING_FLAGS(bridgeObj->bridgeName, "Default", mdmLibCtx.allocFlags);
   bridgeObj->bridgeEnable = TRUE;

   /* set the L2BridgingEntryObject */
   ret = mdm_setObject((void **)&bridgeObj, &(pathDesc.iidStack), FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set L2BridgingEntryObject, ret = %d", ret);
   }

   return ret;
}

CmsRet addDefaultL2BridgingFilterInterfaceObject(UINT32 intfKey, SINT32 bridgeRef)
{
   CmsRet                 ret = CMSRET_SUCCESS;
   MdmPathDescriptor      pathDesc;
   L2BridgingFilterObject *filterObj=NULL;
   char filterInterface[BUFLEN_32];

   cmsLog_debug("interface key %d", intfKey);

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_L2_BRIDGING_FILTER;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not create instance of bridge filter object");
      return ret;
   }

   /* get the object we just created */
   if ((ret = mdm_getObject(MDMOID_L2_BRIDGING_FILTER, &(pathDesc.iidStack), (void **) &filterObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get L2BridgingEntryObject, ret=%d", ret);
      return ret;
   }

   /* populate the fields of the bridge filter object */

   filterObj->filterEnable = TRUE;
   filterObj->filterBridgeReference = bridgeRef;
   sprintf(filterInterface, "%u", intfKey);
   CMSMEM_REPLACE_STRING_FLAGS(filterObj->filterInterface, filterInterface, mdmLibCtx.allocFlags);

   ret = mdm_setObject((void **)&filterObj, &(pathDesc.iidStack), FALSE);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set L2BridgingFilterObject, ret = %d", ret);
   }

   cmsLog_debug("done, ret=%d", ret);

   return ret;
}


CmsRet addDefaultL2BridgingAvailableInterfaceObject(const char *interfaceReference, SINT32 bridgeRef)
{
   CmsRet                 ret = CMSRET_SUCCESS;
   MdmPathDescriptor      pathDesc;
   L2BridgingIntfObject   *availIntfObj=NULL;

   cmsLog_debug("interfaceReference=%s", interfaceReference);
   
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_L2_BRIDGING_INTF;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not create instance of available interfaces object");
      return ret;
   }

   /* get the object we just created */
   if ((ret = mdm_getObject(MDMOID_L2_BRIDGING_INTF, &(pathDesc.iidStack), (void **) &availIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get L2BridgingAvailableInterfaceObject, ret=%d", ret);
      return ret;
   }

   /* During adjust for hardware, all available interfaces are LAN interfaces */
   CMSMEM_REPLACE_STRING_FLAGS(availIntfObj->interfaceType, MDMVS_LANINTERFACE, mdmLibCtx.allocFlags);
   
   CMSMEM_REPLACE_STRING_FLAGS(availIntfObj->interfaceReference, interfaceReference, mdmLibCtx.allocFlags);
   

   /*
    * add the filter entry associated with this available interface entry.
    * We are supposed to pass in the availIntfObj->availableInterfaceKey, but
    * because we are doing this inside the mdm, the availableInterfaceKey has not
    * been set by the rcl_l2BridgingIntfObject() yet.  Since I know that function
    * uses the instance number as the key, I can use the key here also.
    */
   addDefaultL2BridgingFilterInterfaceObject(PEEK_INSTANCE_ID(&(pathDesc.iidStack)), bridgeRef);

   if ((ret = mdm_setObject((void **)&availIntfObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set L2BridgingIntfObject, ret = %d", ret);
   }

   cmsLog_debug("done, ret=%d", ret);
 
   return ret;  
}

#endif  /* DMP_BRIDGING_1 aka SUPPORT_PORT_MAP */



#ifdef DMP_ETHERNETWAN_1

UBOOL8 isEthInterfaceGMACEnabled(const char *ifName, const char *pGMACPortList)
{
   UBOOL8 isGMACEnabled = FALSE;
   
   if (pGMACPortList)
   {
      isGMACEnabled = cmsUtl_isSubOptionPresent(pGMACPortList, ifName);
   }      

   return isGMACEnabled;
}


CmsRet addPersistentWanEthInterfaceObject(const char *ifName, const char *pGMACPortList)
{
   _WanEthIntfObject *wanEthObj = NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   cmsLog_debug("Enabling persistent WAN Eth %s intf object", ifName);
      
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WAN_ETH_INTF;
   /* default WAN eth interface always under WANDevice.3 */
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), CMS_WANDEVICE_ETHERNET); 

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &wanEthObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get eth intf object, ret=%d", ret);
      return ret;
   }

   /* Only do this once */
   if (!wanEthObj->enable)
   {
      /* set the intf name and enable */   
      wanEthObj->enable = TRUE;

      wanEthObj->X_BROADCOM_COM_PersistentDevice = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->duplexMode, MDMVS_AUTO, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->maxBitRate, MDMVS_AUTO, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->X_BROADCOM_COM_ConnectionMode, MDMVS_VLANMUXMODE, mdmLibCtx.allocFlags);
      
      CMSMEM_REPLACE_STRING_FLAGS(wanEthObj->X_BROADCOM_COM_IfName, ifName, mdmLibCtx.allocFlags);

      wanEthObj->X_BROADCOM_COM_GMAC_Enabled = isEthInterfaceGMACEnabled(ifName, pGMACPortList);
      if ((ret = mdm_setObject((void **) &wanEthObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         /* Free the wanEnet obect */
         mdm_freeObject((void **) &wanEthObj);
         cmsLog_error("could not set eth intf object, ret=%d", ret);
      }

#if 0 //defined(CHIP_63138) || defined(CHIP_63148) || defined(CHIP_4908)      
#ifdef SUPPORT_QOS
      {
         char *fullPath=NULL;
      
         cmsLog_notice("Init QoS Queues for %s", ifName);
      
         cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath);
         /* Annoying TR-98 format: remove the last . */
         fullPath[strlen(fullPath)-1] = '\0';

         /* Sets the default Strict Priority queues for this interface */
         ret = addDefaultEthQueueObject(fullPath, TRUE, TRUE);
         cmsMem_free(fullPath);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("addDefaultLanEthQueueObject returns error. ret=%d", ret);
            return ret;
         }
      }
#endif
#endif

      /*
       * Also create a single WANConnectionDevice in this WANDevice.
       */
      pathDesc.oid = MDMOID_WAN_CONN_DEVICE;
      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not create new WanConnectionDevice, ret=%d", ret);
      }
   }
   else
   {
      /* Free the wanEnet obect */
      mdm_freeObject((void **) &wanEthObj);
   }
   
   return ret;
               
}

#endif /* DMP_ETHERNETWAN_1_NOT_USED */


#ifdef NotUsed

#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1
CmsRet addPersistentWanMocaInterfaceObject(const char *ifName)
{
   _WanMocaIntfObject *wanMocaObj = NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   cmsLog_debug("Enabling persistent WAN MoCA %s intf object", ifName);
      
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WAN_MOCA_INTF;
   /* default WAN moca interface always under WANDevice.4 */
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), CMS_WANDEVICE_MOCA); 

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &wanMocaObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get moca intf object, ret=%d", ret);
      return ret;
   }

   /* Only do this once */
   if (!wanMocaObj->enable)
   {
      /* set the intf name and enable */   
      wanMocaObj->enable = TRUE;

      wanMocaObj->persistentDevice = TRUE;
      
      CMSMEM_REPLACE_STRING_FLAGS(wanMocaObj->ifName, ifName, mdmLibCtx.allocFlags);

      if ((ret = mdm_setObject((void **) &wanMocaObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set moca intf object, ret=%d", ret);
      }

      /* Free the wanEnet obect */
      mdm_freeObject((void **) &wanMocaObj);

      /*
       * Also create a single WANConnectionDevice in this WANDevice.
       */
      pathDesc.oid = MDMOID_WAN_CONN_DEVICE;
      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not create new WanConnectionDevice, ret=%d", ret);
      }
   }
   
   return ret;
               
}

#endif /* DMP_X_BROADCOM_COM_MOCAWAN_1 */

#endif /* NotUsed */


#ifdef DMP_X_BROADCOM_COM_EPONWAN_1   
#ifdef EPON_SFU
CmsRet addDefaultWanIpConnection()
{
   void *mdmObj = NULL;
   InstanceIdStack iidStack;
   MdmPathDescriptor pathDesc;
   WanConnDeviceObject *wanConnDev = NULL;
   WanIpConnObject *wanIpConn = NULL; 
   CmsRet ret;
   
   if ((ret = mdm_getNextObject(MDMOID_WAN_IP_CONN, &iidStack, &mdmObj)) == CMSRET_SUCCESS)
   {
      /* Epon WANConnDevice is already present, no action needed */
      mdm_freeObject(&mdmObj);
      cmsLog_debug("epon wan layer3 interface exist");
      return ret;
   }
   
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WAN_IP_CONN;
   PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_EPON);
   PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);
   PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);
   
   /* add WAN_IP_CONN instance */
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add wanIpConnInstance, ret = %d", ret);
      return ret;
   }
   
   if ((ret = mdm_getObject(MDMOID_WAN_IP_CONN, &pathDesc.iidStack, (void **) &wanIpConn)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get wanIpConn, ret = %d", ret);
      return ret;
   }
   
   CMSMEM_REPLACE_STRING_FLAGS(wanIpConn->connectionType, MDMVS_IP_BRIDGED, mdmLibCtx.allocFlags);
#ifdef DMP_X_BROADCOM_COM_IGMP_1
   wanIpConn->X_BROADCOM_COM_IGMP_SOURCEEnabled = 1;
#endif
#ifdef DMP_X_BROADCOM_COM_MLD_1
   wanIpConn->X_BROADCOM_COM_MLD_SOURCEEnabled = 1;
#endif
   wanIpConn->enable = TRUE;
   wanIpConn->X_BROADCOM_COM_VlanMuxID    = -1;
   wanIpConn->X_BROADCOM_COM_VlanMux8021p = -1;
   wanIpConn->X_BROADCOM_COM_ConnectionId = 1;
   CMSMEM_REPLACE_STRING_FLAGS(wanIpConn->name, "br_epon0", mdmLibCtx.allocFlags);
   
   if ((ret = mdm_setObject((void **) &wanIpConn, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set MDMOID_WAN_IP_CONN, ret=%d", ret);
      mdm_freeObject((void **) &wanIpConn);
      return ret;
   }
   

      iidStack = pathDesc.iidStack;
      /* Need to update the WanConnDevice count */
      if ((ret = mdm_getAncestorObject(MDMOID_WAN_CONN_DEVICE, 
                                       MDMOID_WAN_IP_CONN,
                                       &iidStack,
                                       (void **) &wanConnDev)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Fail to get WanDev object, ret=%d", ret);
         return ret;
      }

      /* update wanConnectionDevice counter */
      wanConnDev->WANIPConnectionNumberOfEntries = 1;
         
      if ((ret = mdm_setObject((void **) &wanConnDev, &iidStack, FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set wanConnDev, ret=%d", ret);
         mdm_freeObject((void **) &wanConnDev);               
         return ret;
      }

#ifdef DMP_BRIDGING_1
   {
      char *fullPath=NULL;

      cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath);
      /* Annoying TR-98 format: remove the last . */
      fullPath[strlen(fullPath)-1] = '\0';
      cmsLog_debug("fullpathname is %s", fullPath);

      ret = addDefaultL2BridgingAvailableInterfaceObject(fullPath, 0);
      cmsMem_free(fullPath);
   }
#endif
   
   return ret;
}

#ifdef BRCM_PKTCBL_SUPPORT
CmsRet addDefaultWanIpConnectionForVoice()
{
    InstanceIdStack iidStack;
    MdmPathDescriptor pathDesc;
    WanConnDeviceObject *wanConnDev = NULL;
    WanIpConnObject *wanIpConn = NULL; 
    
    UBOOL8 found = FALSE;
    CmsRet ret = CMSRET_SUCCESS;
    
    cmsLog_debug("Enter addDefaultEponWanIpConnectionForVoice");
    
    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_WAN_IP_CONN;
    
    while (!found && (ret == CMSRET_SUCCESS))
    {
        ret = mdm_getNextObject(pathDesc.oid, &pathDesc.iidStack, (void **) &wanIpConn);
        if (ret == CMSRET_SUCCESS)
        {
            if (!cmsUtl_strncmp(wanIpConn->name, "e_mta", 5))
            {
                found = TRUE;
            }
            mdm_freeObject((void **) &wanIpConn);
        }
    }
    
    if (found)
    {
        cmsLog_error("Found existing IP.Interface with name e_mta, just return");
        return CMSRET_SUCCESS;
    }
        
    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_WAN_IP_CONN;
    PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_EPON);
    PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);
    PUSH_INSTANCE_ID(&pathDesc.iidStack, 2);
        
    /* add WAN_IP_CONN instance */
    if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to add wanIpConnInstance, ret = %d", ret);
        return ret;
    }
    
    if ((ret = mdm_getObject(MDMOID_WAN_IP_CONN, &pathDesc.iidStack, (void **) &wanIpConn)) != CMSRET_SUCCESS)
    {
       cmsLog_error("Failed to get wanIpConn, ret = %d", ret);
       return ret;
    }
    
    CMSMEM_REPLACE_STRING_FLAGS(wanIpConn->connectionType, MDMVS_IP_ROUTED, mdmLibCtx.allocFlags);
    wanIpConn->enable = TRUE;   
    wanIpConn->X_BROADCOM_COM_VlanMuxID    = -1;
    wanIpConn->X_BROADCOM_COM_VlanMux8021p = -1;
    wanIpConn->X_BROADCOM_COM_ConnectionId = 2;
    CMSMEM_REPLACE_STRING_FLAGS(wanIpConn->name, "e_mta", mdmLibCtx.allocFlags);
    CMSMEM_REPLACE_STRING_FLAGS(wanIpConn->X_BROADCOM_COM_IfName, EPON_VOICE_WAN_IF_NAME, mdmLibCtx.allocFlags);
    wanIpConn->X_BROADCOM_COM_Op51LeasedTime = 3600;

    
#ifdef SUPPORT_IPV6
    /* enable IPv6  */
    wanIpConn->X_BROADCOM_COM_IPv6Enabled = TRUE;
#if defined(DMP_X_BROADCOM_COM_IPV6_1)
    CMSMEM_REPLACE_STRING_FLAGS(wanIpConn->X_BROADCOM_COM_IPv6AddressingType, MDMVS_DHCP, mdmLibCtx.allocFlags);
    wanIpConn->X_BROADCOM_COM_Dhcp6cForAddress = TRUE;
    wanIpConn->X_BROADCOM_COM_Dhcp6cForPrefixDelegation = TRUE;
#endif // DMP_X_BROADCOM_COM_IPV6_1

#endif // SUPPORT_IPV6
    
    if ((ret = mdm_setObject((void **) &wanIpConn, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
    {
        cmsLog_error("could not set MDMOID_WAN_IP_CONN, ret=%d", ret);
        mdm_freeObject((void **) &wanIpConn);
        return ret;
    }    
    
    iidStack = pathDesc.iidStack;
    /* Need to update the WanConnDevice count */
    if ((ret = mdm_getAncestorObject(MDMOID_WAN_CONN_DEVICE, 
                                     MDMOID_WAN_IP_CONN,
                                     &iidStack,
                                     (void **) &wanConnDev)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Fail to get WanDev object, ret=%d", ret);
        return ret;
    }
    
    /* update wanConnectionDevice counter */
    wanConnDev->WANIPConnectionNumberOfEntries ++;
       
    if ((ret = mdm_setObject((void **) &wanConnDev, &iidStack, FALSE)) != CMSRET_SUCCESS)
    {
        cmsLog_error("could not set wanConnDev, ret=%d", ret);
        mdm_freeObject((void **) &wanConnDev);
        return ret;
    }    
   
    return ret;
}

#endif
#endif

#ifdef DMP_X_BROADCOM_COM_EPON_1      
CmsRet addPersistentWanEponInterfaceObject(const char *ifName)

{
   _WanEponIntfObject *wanEponObj = NULL;
   _WanDevObject *wanDev = NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   cmsLog_debug("Enabling persistent WAN Epon %s intf object", ifName);
      
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WAN_EPON_INTF;
   /* default WAN Epon interface always under WANDevice.6 */
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), CMS_WANDEVICE_EPON); 

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &wanEponObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get epon intf object, ret=%d", ret);
      return ret;
   }

   /* Only do this once */
   if (!wanEponObj->enable)
   {
      /* set the intf name and enable */   
      wanEponObj->enable = TRUE;

      wanEponObj->persistentDevice = TRUE;
      
      CMSMEM_REPLACE_STRING_FLAGS(wanEponObj->ifName, ifName, mdmLibCtx.allocFlags);

      if ((ret = mdm_setObject((void **) &wanEponObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set epon intf object, ret=%d", ret);
         mdm_freeObject((void **) &wanEponObj);
         return ret;
      }
   

      /*
       * Also create a single WANConnectionDevice in this WANDevice.
       */
      pathDesc.oid = MDMOID_WAN_CONN_DEVICE;
      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not create new WanConnectionDevice, ret=%d", ret);
         return ret;
      }

      /* Need to update the WanConnDevice count */
      if ((ret = mdm_getAncestorObject(MDMOID_WAN_DEV, 
                                       MDMOID_WAN_CONN_DEVICE,
                                       &(pathDesc.iidStack),
                                       (void **) &wanDev)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Fail to get WanDev object, ret=%d", ret);
         return ret;
      }

      /* update wanConnectionDevice counter */
      wanDev->WANConnectionNumberOfEntries = 1;
         
      if ((ret = mdm_setObject((void **) &wanDev, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set wanDev, ret=%d", ret);
         mdm_freeObject((void **) &wanDev);               
         return ret;
      }
#ifdef EPON_SFU
      addDefaultWanIpConnection();
#ifdef BRCM_PKTCBL_SUPPORT
      addDefaultWanIpConnectionForVoice();
#endif
#endif
   }
   else
   {
      mdm_freeObject((void **) &wanEponObj);
   }
   
   return ret;
                  
}
#endif /* DMP_X_BROADCOM_COM_EPON_1 */
#endif /* DMP_X_BROADCOM_COM_EPONWAN_1 */



#ifdef DMP_X_BROADCOM_COM_EPON_1

CmsRet addLanEponInterfaceObject(const char *ifName)
{
   MdmPathDescriptor pathDesc;
   _LanEponIntfObject *eponObj=NULL;
   CmsRet ret;

   cmsLog_notice("adding LAN %s intf object", ifName);
      
   INIT_PATH_DESCRIPTOR(&pathDesc);
   
   pathDesc.oid = MDMOID_LAN_EPON_INTF;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); /* default epon interface always under LANDevice.1 */

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to create default %s interface, ret=%d", ifName, ret);
      return ret;
   }
   
   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &eponObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get epon intf object, ret=%d", ret);
      return ret;
   }

   /* set the intf name and enable */   
   eponObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(eponObj->ifName, ifName, mdmLibCtx.allocFlags);

   
   if ((ret = mdm_setObject((void **) &eponObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set epon intf object, ret=%d", ret);
      mdm_freeObject((void **) &eponObj);
      return ret;
   }

#ifdef DMP_BRIDGING_1
   {
      char *fullPath=NULL;

      cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath);
      /* Annoying TR-98 format: remove the last . */
      fullPath[strlen(fullPath)-1] = '\0';
      cmsLog_debug("fullpathname is %s", fullPath);

      ret = addDefaultL2BridgingAvailableInterfaceObject(fullPath, 0);
      cmsMem_free(fullPath);
   }
#endif

   return ret;
}

#endif /* DMP_X_BROADCOM_COM_EPON_1 */

#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
CmsRet addPersistentWanWifiInterfaceObject(const char *ifName)
{
   _WanWifiIntfObject *wanWifiObj = NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   cmsLog_debug("Enabling persistent WAN Wifi %s intf object", ifName);
      
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WAN_WIFI_INTF;
   /* default WAN Wl interface always under WANDevice.7 */
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), CMS_WANDEVICE_WIFI); 

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &wanWifiObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get wifi intf object, ret=%d", ret);
      return ret;
   }

   /* Only do this once */
   if (!wanWifiObj->enable)
   {
      /* set the intf name and enable */   
      wanWifiObj->enable = TRUE;

      wanWifiObj-> persistentDevice= TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(wanWifiObj->connectionMode, MDMVS_VLANMUXMODE, mdmLibCtx.allocFlags);
      
      CMSMEM_REPLACE_STRING_FLAGS(wanWifiObj->ifName, ifName, mdmLibCtx.allocFlags);

      if ((ret = mdm_setObject((void **) &wanWifiObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set wifi intf object, ret=%d", ret);
      }

      /* Free the wanWifi obect */
      mdm_freeObject((void **) &wanWifiObj);

      /*
       * Also create a single WANConnectionDevice in this WANDevice.
       */
      pathDesc.oid = MDMOID_WAN_CONN_DEVICE;
      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not create new WanConnectionDevice, ret=%d", ret);
      }
   }
   
   return ret;
               
}

#endif /* DMP_X_BROADCOM_COM_WIFIWAN_1 */

#if defined(DMP_X_BROADCOM_COM_RDPA_1) 
CmsRet lanDefaultFiltersConfig(int rdpaIf_p)
{
    CmsRet ret;
    char strPortMask[BUFLEN_64+1];
    UINT32 count;
    MdmPathDescriptor filterDataPathDesc;
    _IngressFiltersDataObject *filterData = NULL;

    memset(strPortMask, '0', BUFLEN_64 + 1);
    strPortMask[BUFLEN_64] = '\0';
    strPortMask[(BUFLEN_64-1)-rdpaIf_p] = '1';

    INIT_PATH_DESCRIPTOR(&filterDataPathDesc);
    PUSH_INSTANCE_ID(&(filterDataPathDesc.iidStack),1); // for filter instance.1
    filterDataPathDesc.oid = MDMOID_INGRESS_FILTERS_DATA;

    /* Check if already created filters on lan side */
    if ((ret = mdm_getObject(filterDataPathDesc.oid, &(filterDataPathDesc.iidStack), (void **) &filterData)) != CMSRET_SUCCESS)
    {
        for (count = 0 ; count < sizeof(defaultLanIngressFiltersTypeStr)/sizeof(char *); count++)
        {
            INIT_PATH_DESCRIPTOR(&filterDataPathDesc);
            filterDataPathDesc.oid = MDMOID_INGRESS_FILTERS_DATA;
            PUSH_INSTANCE_ID(&(filterDataPathDesc.iidStack), (count+1));

            if ((ret = mdm_addObjectInstance(&filterDataPathDesc, NULL, NULL)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Failed to create filter data %s ", defaultLanIngressFiltersTypeStr[count]);
                goto exit;
            }

            /* get the object we just created */
            if ((ret = mdm_getObject(filterDataPathDesc.oid, &(filterDataPathDesc.iidStack), (void **) &filterData)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Could not get filter data object, ret=%d", ret);
                goto exit;
            }

            /* Build the strinf port map */
            CMSMEM_REPLACE_STRING_FLAGS(filterData->ports, strPortMask, mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(filterData->type, defaultLanIngressFiltersTypeStr[count], mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(filterData->action, MDMVS_FILTER_CPU, mdmLibCtx.allocFlags);
            if (!cmsUtl_strcmp(filterData->type, MDMVS_FILTER_ETYPE_UDEF))
                filterData->val = INIT_FILTER_INBAND_FILTER_VAL;

            if ((ret = mdm_setObject((void **) &filterData, &(filterDataPathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Could not set filter data object, ret=%d", ret);
                goto exit;
            }
        }
    }
    else
    {
        for (count = 0 ; count < sizeof(defaultLanIngressFiltersTypeStr)/sizeof(char *); count++)
        {
            if (count!=0)
            {
                /* get the object we just created */
                if ((ret = mdm_getNextObject(filterDataPathDesc.oid, &(filterDataPathDesc.iidStack), (void **) &filterData)) != CMSRET_SUCCESS)
                {
                    cmsLog_error("Could not get filter data object, ret=%d", ret);
                    goto exit;
                }
            }

            /* Update the instance port map */
            cmsUtl_strncpy(strPortMask, filterData->ports, sizeof(strPortMask));
            /* set the last char in the arry to 1 means wan0 port will be marked in the port bit mask */
            strPortMask[(BUFLEN_64-1)-rdpaIf_p] = '1';           
            CMSMEM_REPLACE_STRING_FLAGS(filterData->ports, strPortMask, mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(filterData->type, defaultLanIngressFiltersTypeStr[count], mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(filterData->action, MDMVS_FILTER_CPU, mdmLibCtx.allocFlags);
            if (!cmsUtl_strcmp(filterData->type, MDMVS_FILTER_ETYPE_UDEF))
                filterData->val = INIT_FILTER_INBAND_FILTER_VAL;

            if ((ret = mdm_setObject((void **) &filterData, &(filterDataPathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Could not set filter data object, ret=%d", ret);
                goto exit;
            }
        }
    }

exit:
    if (filterData!=NULL)
        mdm_freeObject((void **) &filterData);    
    return ret;
}

CmsRet lanDefaultFiltersConfigByName(const char *ifName)
{
    CmsRet ret;
    int rdpaIf_p;

    /* Translate if name to rdpa if and build mask */
    ret = mdm_getRpdaIfByIfname(ifName, &rdpaIf_p);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Cannot get rdpa if ret=%d", ret);
        return ret;
    }

    if (rdpaIf_p < rdpa_if_lan0 || rdpaIf_p > rdpa_if_lan_max)
        return ret;

    return lanDefaultFiltersConfig(rdpaIf_p);
}

CmsRet allLanFiltersDefaultConf(void)
{
    CmsRet ret;
    char *ifNames=NULL;
    char ifNameBuf[CMS_IFNAME_LENGTH];
    UINT32 end, c=0;
    UINT32 idx;
    
    /* Return if upgrade flag set, otherwise set flag and continue */
    MdmPathDescriptor filterPathDesc;
    _FiltersCfgObject *filterObj = NULL;

    INIT_PATH_DESCRIPTOR(&filterPathDesc);
    filterPathDesc.oid = MDMOID_FILTERS_CFG;

    if ((ret = mdm_getObject(filterPathDesc.oid, &(filterPathDesc.iidStack), (void **) &filterObj)) == CMSRET_SUCCESS)
    {
        if (filterObj->prevCfg)
            goto exit;
        else
        {
            filterObj->prevCfg =1;
            if ((ret = mdm_setObject((void **) &filterObj, &(filterPathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Could not set filter object, ret=%d", ret);
                goto exit;
            }
        }
    }

    cmsNet_getIfNameList(&ifNames);

    if (ifNames == NULL)
    {
        cmsLog_error("no interfaces found during initialization!");
        return CMSRET_INTERNAL_ERROR;
    }
    end = strlen(ifNames);

    while (c < end)
    {
        cmsLog_debug("current ifNames=%s", &(ifNames[c]));   
        idx = 0;
        while (c < end && ifNames[c] != ',')
        {
            ifNameBuf[idx] = ifNames[c];
            c++;
            idx++;
        }
        ifNameBuf[idx] = 0;
        c++;

        if (
#ifndef G9991
            (0 == cmsUtl_strncmp(ifNameBuf, "eth", 3)) ||
            (0 == cmsUtl_strncmp(ifNameBuf, "moca", 4)) ||            
#endif
            (0 == cmsUtl_strncmp(ifNameBuf, "sid", 3)))
        {
            ret = lanDefaultFiltersConfigByName(ifNameBuf);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("lanDefaultFiltersConfigByName() failed, ret=%d", ret);
                break;
            }
        }

    }

exit:
    if (filterObj != NULL)
    {
        mdm_freeObject((void **) &filterObj);
    }

    cmsMem_free(ifNames);

    return ret;
}
#endif


