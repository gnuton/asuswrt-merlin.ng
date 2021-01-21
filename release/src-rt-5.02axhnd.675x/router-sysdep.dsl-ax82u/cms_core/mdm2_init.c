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

#ifdef DMP_DEVICE2_BASELINE_1

#include "cms.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"
#include "cms_net.h"
#include "linux/rut_system.h"

/* in mdm2_initlan.c */
CmsRet mdm_addDefaultLanBridgeObjects_dev2(MdmPathDescriptor *mgmtBrPortPathDesc);
CmsRet mdm_addDefaultDhcpv4ServerObjects_dev2(const char *ipIntfFullPath);
CmsRet mdm_addIpv4AddressObject_dev2(const MdmPathDescriptor *ipIntfPathDesc,
                            const char *ipv4Addr,
                            const char *netmask,
                            const char *addressingType);

#ifdef DMP_DEVICE2_DHCPV4RELAY_1
void mdm_configDhcpv4RelayObject_dev2(void);
#endif

/* in mdm2_initlan6.c */
CmsRet mdm_addIpv6AddressObject_dev2(const MdmPathDescriptor *ipIntfPathDesc);

/* in mdm2_initdsl.c */
CmsRet mdm_addDefaultDslObjects_dev2(void);

/* in mdm2_initwifi.c */
CmsRet mdm_initWifiAccessPoint_dev2(void);

#ifdef DMP_DEVICE2_SM_BASELINE_1
CmsRet mdm_addDefaultModSwObjects(void);
#else
void mdm_removeBeepDatabase(void);
#endif


/* forward declarations */
/* local functions */

static CmsRet addEthernetLinkObject(const char *ifname,
                            const char *lowerLayer,
                            MdmPathDescriptor *pathDesc);

#ifdef DMP_DEVICE2_SUPPORTEDDATAMODEL_1
static CmsRet mdm_addDefaultSupportedDataModelObjects_dev2(void);
#endif

#ifdef DMP_DEVICE2_PROCESSORS_1
static CmsRet mdm_addDefaultProcessorObjects_dev2(void);
#endif  /* DMP_DEVICE2_PROCESSORS_1 */

#ifdef DMP_DEVICE2_OPTICAL_1
CmsRet mdm_addDefaultOpticalObject(void);
#endif

#if defined(DMP_X_BROADCOM_COM_RDPA_1) 
CmsRet mdm_addDefaultWanFilterObjects(void);
#endif

CmsRet mdm_adjustForHardwareLocked_dev2(void)
{
   MdmPathDescriptor mgmtBrPortPathDesc;
   MdmPathDescriptor ipIntfPathDesc;
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_notice("Entered===>");

#ifdef DMP_DEVICE2_SUPPORTEDDATAMODEL_1
   ret = mdm_addDefaultSupportedDataModelObjects_dev2();
#endif

#ifdef DMP_DEVICE2_PROCESSORS_1
   ret = mdm_addDefaultProcessorObjects_dev2();
#endif

#ifdef DMP_DEVICE2_DSL_1
   ret = mdm_addDefaultDslObjects_dev2();
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }
#endif

#ifdef DMP_DEVICE2_ROUTING_1
   ret = mdm_initRouterObject_dev2();
   if (ret != CMSRET_SUCCESS)
   {
      return ret;
   }
#endif

   ret = mdm_addDefaultLanBridgeObjects_dev2(&mgmtBrPortPathDesc);
   if ((ret == CMSRET_SUCCESS) && (mgmtBrPortPathDesc.oid == MDMOID_DEV2_BRIDGE_PORT))
   {
      char *mgmtBrPortFullPath=NULL;
      char *ipIntfFullPath=NULL;
      UBOOL8 supportIpv4=TRUE;
      UBOOL8 supportIpv6=FALSE;

      cmsLog_notice("new LAN Bridge object was created, created IP and other objects too");

      if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&mgmtBrPortPathDesc, &mgmtBrPortFullPath)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
         return ret;
      }

      /* set basic IP params */
      /* TODO: if IPv6 only build, then supportIpv4 = FALSE */

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      supportIpv6 = TRUE;
#endif

      ret = mdm_initIpObject_dev2(supportIpv4, supportIpv6);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("initIpObject failed, ret=%d", ret);
         return ret;
      }

      /* create IP.Interface object here */
      ret = mdm_addDefaultLanIpInterfaceObject_dev2("br0", "Default",
                                              supportIpv4, supportIpv6,
                                              mgmtBrPortFullPath,
                                              &ipIntfPathDesc);
      CMSMEM_FREE_BUF_AND_NULL_PTR(mgmtBrPortFullPath);
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

#ifndef DHCP_CLIENT_DEFAULT                                         
      /* TODO: should this be surrouned by some IPv4 ifdef ? */
      ret = mdm_addIpv4AddressObject_dev2(&ipIntfPathDesc,
                                         "192.168.1.1", "255.255.255.0",
                                         MDMVS_STATIC);
#endif 

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_addDefaultLanIpv4AddressObject failed, ret=%d", ret);
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
         return ret;
      }

#ifdef DHCP_SERVER_DEFAULT
      ret = mdm_addDefaultDhcpv4ServerObjects_dev2(ipIntfFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_addDefaultDhcpv4ServerObjects failed, ret=%d", ret);
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
         return ret;
      }
#endif  /* DHCP_SERVER_DEFAULT */

#if defined(DHCP_CLIENT_DEFAULT) && defined(DMP_DEVICE2_DHCPV4_1)
      ret = mdmInit_addDhcpv4ClientObject_dev2(ipIntfFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("mdmInit_addDhcpv4ClientObject_dev2 failed, ret=%d", ret);
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
         return ret;
      }
#endif  /* defined(DHCP_CLIENT_DEFAULT) && defined(DMP_DEVICE2_DHCPV4_1) */


#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      /* TODO: do we need to add IP.Interface.{i}.IPv6Address object ?*/
//      ret = mdm_addIpv6AddressObject_dev2(&ipIntfPathDesc);
//      if (ret != CMSRET_SUCCESS)
//      {
//         cmsLog_error("mdm_addDefaultLanIpv6AddressObject failed, ret=%d", ret);
//         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
//         return ret;
//      }

      /* IPv6 needs DHCP server and Router Advertisement on LAN side */
      mdm_addDefaultDhcpv6ServerObjects_dev2(ipIntfFullPath);
      mdm_addDefaultRouterAdvertisementObjects_dev2(ipIntfFullPath);

#endif

      CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
   }

#ifdef DMP_DEVICE2_OPTICAL_1
   mdm_addDefaultOpticalObject();
#endif

#if defined(DMP_X_BROADCOM_COM_RDPA_1) 
   mdm_addDefaultWanFilterObjects();
#endif

#ifdef DMP_DEVICE2_DHCPV4RELAY_1
   mdm_configDhcpv4RelayObject_dev2();
#endif


#ifdef DMP_DEVICE2_HOMEPLUG_1
   if (isPlcInterfaceExist())
   {
      cmsLog_debug("Found plc0");
      ret = mdmInit_addDownstreamHomePlugPort("plc0");
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to add Homeplug! ret=%d", ret);
         return ret;
      }
   }
#endif  /* DMP_DEVICE2_HOMEPLUG_1 */

#ifdef DMP_DEVICE2_SM_BASELINE_1
   ret = mdm_addDefaultModSwObjects();
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add software module management object! ret=%d", ret);
      return ret;
   }
#else
   mdm_removeBeepDatabase();
#endif

#ifdef DMP_DEVICE2_WIFIACCESSPOINT_1
   /* init Wifi on LAN side, meaning Access Point */
   /* or should it do both LAN side and WAN side at the same time ? */
   if ((ret = mdm_initWifiAccessPoint_dev2()) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_initWifi_dev2 failed, ret=%d", ret);
      return ret;
   }
#endif

   /* XXX later add a DMP_DEVICE2_WIFIENDPOINT for Wifi as WAN */


#ifdef BRCM_VOICE_SUPPORT
   ret = mdm_adjustForVoiceHardware();
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_adjustForVoiceHardware failed, ret=%d", ret);
      return ret;
   }
#endif

#ifdef DMP_STORAGESERVICE_1
   ret = mdm_addDefaultStorageServiceObject();
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addDefaultStorageService failed, ret=%d", ret);
      return ret;
   }
#endif


#ifdef DMP_DEVICE2_USBHOSTSBASIC_1
   ret = mdm_addDefaultUsbHostObject();
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addDefaultUsbHostObject failed, ret=%d", ret);
      return ret;
   }
#endif

   cmsLog_notice("====> returning ret=%d", ret);
   return ret;
}


CmsRet mdm_adjustForHardware_dev2(void)
{
   CmsRet ret;

   /* acquire lock during adjustForHardware so we can call QDM functions.
    * Only do this for _dev2 because people might have written code
    * in the _igd version to get the lock already.
    */
   cmsLck_acquireLock();
   ret = mdm_adjustForHardwareLocked_dev2();
   cmsLck_releaseLock();

   return ret;
}


CmsRet mdm_addDefaultLanIpInterfaceObject_dev2(const char *ifname,
                                         const char *groupName,
                                         UBOOL8 supportIpv4,
                                         UBOOL8 supportIpv6,
                                         const char *lowerLayer,
                                         MdmPathDescriptor *ipIntfPathDesc)
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   UBOOL8 found=FALSE;
   CmsRet ret=CMSRET_SUCCESS;

   /*
    * If we find an IP.Interface that starts with "br", then this is
    * not a blank MDM, so we should do nothing.
    */
   INIT_PATH_DESCRIPTOR(ipIntfPathDesc);
   ipIntfPathDesc->oid = MDMOID_DEV2_IP_INTERFACE;
   while (!found && (ret == CMSRET_SUCCESS))
   {
      ret = mdm_getNextObject(ipIntfPathDesc->oid, &ipIntfPathDesc->iidStack, (void **) &ipIntfObj);
      if (ret == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strncmp(ipIntfObj->name, "br", 2))
         {
            found = TRUE;
         }
         mdm_freeObject((void **) &ipIntfObj);
      }
   }

   if (found)
   {
      cmsLog_notice("Found existing IP.Interface with name brx, just return");
      INIT_PATH_DESCRIPTOR(ipIntfPathDesc);
      return CMSRET_SUCCESS;
   }

   return (mdmInit_addIpInterfaceObject_dev2(ifname, groupName,
                                             supportIpv4, supportIpv6,
                                             FALSE, FALSE, FALSE, NULL,
                                             lowerLayer, ipIntfPathDesc));
}


CmsRet mdmInit_addIpInterfaceObject_dev2(const char *ifname,
                            const char *groupName,
                            UBOOL8 supportIpv4 __attribute((unused)),
                            UBOOL8 supportIpv6 __attribute((unused)),
                            UBOOL8 isUpstream,
                            UBOOL8 isBridgeService,
                            UBOOL8 isBridgeIpAddrNeeded,
                            const char *referedBridgeName,
                            const char *lowerLayer,
                            MdmPathDescriptor *ipIntfPathDesc)
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   char *ethLinkFullPath=NULL;
   CmsRet ret;

   /*
    * First we need a Ethernet.Link object under the IP.Interface object
    */
   {
      MdmPathDescriptor ethLinkPathDesc=EMPTY_PATH_DESCRIPTOR;

      ret = addEthernetLinkObject(ifname, lowerLayer, &ethLinkPathDesc);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not create EthernetLink object, ret=%d", ret);
         return ret;
      }

      /* Create fullPath string needed by IP.Interface lowerlayer */
      ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&ethLinkPathDesc, &ethLinkFullPath);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
         return ret;
      }
   }

   /*
    * Now we can add the IP.Interface object
    */
   INIT_PATH_DESCRIPTOR(ipIntfPathDesc);
   ipIntfPathDesc->oid = MDMOID_DEV2_IP_INTERFACE;
   if ((ret = mdm_addObjectInstance(ipIntfPathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add IP.Interface Instance, ret=%d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(ethLinkFullPath);
      return ret;
   }

   if ((ret = mdm_getObject(ipIntfPathDesc->oid, &ipIntfPathDesc->iidStack, (void **) &ipIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get IP.Interface object, ret=%d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(ethLinkFullPath);
      return ret;
   }

   /* set the params in the newly created IP.Interface object */
   ipIntfObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->name, ifname, mdmLibCtx.allocFlags);
   if (groupName)
   {
      CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->X_BROADCOM_COM_GroupName, groupName, mdmLibCtx.allocFlags);
   }

   ipIntfObj->X_BROADCOM_COM_Upstream = isUpstream;
   ipIntfObj->X_BROADCOM_COM_BridgeService = isBridgeService;
   ipIntfObj->X_BROADCOM_COM_BridgeNeedsIpAddr = isBridgeIpAddrNeeded;
   if (referedBridgeName)
   {
      CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->X_BROADCOM_COM_BridgeName, referedBridgeName, mdmLibCtx.allocFlags);
   }

   /* what about IPv6 only, should we only conditionally enable IPv4?  */
   ipIntfObj->IPv4Enable = supportIpv4;

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   ipIntfObj->IPv6Enable = supportIpv6;
#endif

   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipIntfObj->lowerLayers, ethLinkFullPath, mdmLibCtx.allocFlags);
   CMSMEM_FREE_BUF_AND_NULL_PTR(ethLinkFullPath);

   if ((ret = mdm_setObject((void **) &ipIntfObj, &ipIntfPathDesc->iidStack,  FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ipIntfObj. ret=%d", ret);
   }
   mdm_freeObject((void **)&ipIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ethLinkObj. ret=%d", ret);
      return ret;
   }


   /* need to manually update the count when adding objects during mdm_init */
   {
      Dev2IpObject *ipObj=NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

      if ((ret = mdm_getObject(MDMOID_DEV2_IP, &iidStack, (void **) &ipObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get ipObj. ret=%d", ret);
         return ret;
      }

      ipObj->interfaceNumberOfEntries++;
      ret = mdm_setObject((void **) &ipObj, &iidStack,  FALSE);
      mdm_freeObject((void **)&ipObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set ipObj. ret=%d", ret);
      }
   }

   return ret;
}


CmsRet mdm_initIpObject_dev2(UBOOL8 supportIpv4 __attribute((unused)),
                             UBOOL8 supportIpv6 __attribute((unused)))
{
   Dev2IpObject *ipObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = mdm_getObject(MDMOID_DEV2_IP, &iidStack, (void **) &ipObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get ipObj. ret=%d", ret);
      return ret;
   }

   if (supportIpv4)
   {
      ipObj->IPv4Capable = TRUE;
      ipObj->IPv4Enable = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(ipObj->IPv4Status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
   }


#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   if (supportIpv6)
   {
      ipObj->IPv6Capable = TRUE;
      ipObj->IPv6Enable = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(ipObj->IPv6Status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
   }
#endif

   ret = mdm_setObject((void **) &ipObj, &iidStack,  FALSE);
   mdm_freeObject((void **)&ipObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ipObj. ret=%d", ret);
   }
   return ret;
}


CmsRet addEthernetLinkObject(const char *ifname,
                             const char *lowerLayer,
                             MdmPathDescriptor *pathDesc)
{
   Dev2EthernetLinkObject *ethLinkObj=NULL;
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_notice("Adding for ifname=%s lowerLayer=%s", ifname, lowerLayer);

   INIT_PATH_DESCRIPTOR(pathDesc);
   pathDesc->oid = MDMOID_DEV2_ETHERNET_LINK;
   if ((ret = mdm_addObjectInstance(pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for DEV2_ETHERNET_LINK failed, ret = %d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc->oid, &pathDesc->iidStack, (void **) &ethLinkObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get ethLinkObj object, ret=%d", ret);
      return ret;
   }

   ethLinkObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ethLinkObj->name, ifname, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ethLinkObj->lowerLayers, lowerLayer, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ethLinkObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);

   ret = mdm_setObject((void **) &ethLinkObj, &pathDesc->iidStack,  FALSE);
   mdm_freeObject((void **)&ethLinkObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ethLinkObj. ret=%d", ret);
      return ret;
   }


   /* need to manually update the count when adding objects during mdm_init */
   {
      Dev2EthernetObject *ethObj=NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

      if ((ret = mdm_getObject(MDMOID_DEV2_ETHERNET, &iidStack, (void **) &ethObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get ethObj. ret=%d", ret);
         return ret;
      }

      ethObj->linkNumberOfEntries++;
      ret = mdm_setObject((void **) &ethObj, &iidStack,  FALSE);
      mdm_freeObject((void **)&ethObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set ethObj. ret=%d", ret);
      }
   }

   return ret;
}


#ifdef DMP_DEVICE2_ROUTING_1
CmsRet mdm_initRouterObject_dev2()
{
   Dev2RoutingObject *routingObj=NULL;
   Dev2RouterObject *routerObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   cmsLog_debug("entered");

   ret = mdm_getObject(MDMOID_DEV2_ROUTING, &iidStack, (void **)&routingObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get DEV2_ROUTING obj");
      return ret;
   }

   if (routingObj->routerNumberOfEntries > 0)
   {
      /*
       * Don't know why TR181 has multiple router entries.  But if we have
       * at least 1 router entry, that is good enough.
       */
      mdm_freeObject((void **)&routingObj);
      return CMSRET_SUCCESS;
   }

   cmsLog_notice("add first instance of router!!");
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_ROUTER;
   pathDesc.iidStack = iidStack;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) == CMSRET_SUCCESS)
   {
      ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **)&routerObj);
      if (ret == CMSRET_SUCCESS)
      {
         routerObj->enable = TRUE;
         ret = mdm_setObject((void **)&routerObj, &pathDesc.iidStack,  FALSE);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to set DEV2_ROUTER obj, ret=%d", ret);
         }
         mdm_freeObject((void **)&routerObj);
      }
      else
      {
         cmsLog_error("Failed to get DEV2_ROUTER obj, ret=%d", ret);
      }
   }
   else
   {
      cmsLog_error("Failed to add DEV2_ROUTER instance, ret=%d", ret);
   }

   /* must manually update the count when adding objects during mdm_init */
   if (ret == CMSRET_SUCCESS)
   {
      routingObj->routerNumberOfEntries++;
      ret = mdm_setObject((void **) &routingObj, &iidStack,  FALSE);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set routingObj. ret=%d", ret);
      }
   }

   mdm_freeObject((void **)&routingObj);

   cmsLog_debug("done ");

   return ret;
}

#endif  /* DMP_DEVICE2_ROUTING_1 */


#ifdef DMP_DEVICE2_SUPPORTEDDATAMODEL_1

/* By default, the object needed to be add is Device.DeviceInfo.SupportedDataModel.{i}. */
static CmsRet mdm_addDefaultSupportedDataModelObjects_dev2(void)
{
   char uuid[BUFLEN_48], features[BUFLEN_64];
   CmsRet ret = CMSRET_SUCCESS;
   Dev2SupportedDataModelObject *supportedDataModelObj = NULL;
   Dev2DeviceInfoObject *deviceInfoObj = NULL;
   MdmPathDescriptor pathDesc;

   cmsLog_debug("Adding default supported data model objects (enter)");

   /* first check if there is a Device.DeviceInfo.SupportedDataModel.{i}. instance  first */
   ret = mdm_getNextObject(MDMOID_DEV2_SUPPORTED_DATA_MODEL,
                           &pathDesc.iidStack,
                           (void **) &supportedDataModelObj);

   /* if Dev2SupportedDataModelObject cannot be found then add and get it */
   if (ret != CMSRET_SUCCESS)
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_SUPPORTED_DATA_MODEL;

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_addObjectInstance for MDMOID_DEV2_SUPPORTED_DATA_MODEL failed, ret=%d", ret);
         return ret;
      }

      if ((ret = mdm_getObject(pathDesc.oid,
                               &pathDesc.iidStack,
                               (void **) &supportedDataModelObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get Dev2SupportedDataModelObject, ret=%d", ret);
         return ret;
      }
   }

   /* URL */
   CMSMEM_REPLACE_STRING_FLAGS(supportedDataModelObj->URL,
                               "http://localhost/data-model/bbf-data-model-2.xml",
                               mdmLibCtx.allocFlags);

   /* UUID */
   memset(uuid, 0, BUFLEN_48);
   if (rutSys_getUuidFromBbfDataModel(BUFLEN_48-1, uuid) == CMSRET_SUCCESS)
   {
      CMSMEM_REPLACE_STRING_FLAGS(supportedDataModelObj->UUID,
                                  uuid,
                                  mdmLibCtx.allocFlags);
   }

   /* URN */
   CMSMEM_REPLACE_STRING_FLAGS(supportedDataModelObj->URN,
                               "urn:broadband-forum-org:tr-181-2-10-0",
                               mdmLibCtx.allocFlags);

   /* features */
   memset(features, 0, BUFLEN_64);
   if (rutSys_getFeaturesFromDataModel(BUFLEN_64-1, features) == CMSRET_SUCCESS)
   {
      CMSMEM_REPLACE_STRING_FLAGS(supportedDataModelObj->features,
                                  features,
                                  mdmLibCtx.allocFlags);
   }

   cmsLog_debug("Before set Dev2ProcessorObject, URL='%s', UUID='%s', URN='%s', features='%s'",
                supportedDataModelObj->URL, supportedDataModelObj->UUID,
                supportedDataModelObj->URN, supportedDataModelObj->features);

   /* set supportedDataModelObj */
   ret = mdm_setObject((void **) &supportedDataModelObj, &pathDesc.iidStack, FALSE);

   /* free supportedDataModelObj */
   mdm_freeObject((void **)&supportedDataModelObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Dev2SupportedDataModelObject. ret=%d", ret);
      return ret;
   }

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_DEVICE_INFO;

   /* get Dev2DeviceInfoObject */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **)&deviceInfoObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get Dev2DeviceInfoObject, ret=%d", ret);
      return ret;
   }
   
   /* increment the supported data model count */   
   deviceInfoObj->supportedDataModelNumberOfEntries++;

   /* set Dev2DeviceInfoObject */
   if ((ret = mdm_setObject((void **)&deviceInfoObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set Dev2DeviceInfoObject. ret=%d", ret);
   }

   /* free Dev2DeviceInfoObject */
   mdm_freeObject((void **)&deviceInfoObj);

   return ret;
}

#endif  /* DMP_DEVICE2_SUPPORTEDDATAMODEL_1 */


#ifdef DMP_DEVICE2_PROCESSORS_1

/* By default, the object needed to be add is Device.DeviceInfo.Processor.{i}. */
static CmsRet mdm_addDefaultProcessorObjects_dev2(void)
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 i = 0, processorNum = 0, frequency = 0;
   char architecture[BUFLEN_32];
   Dev2ProcessorObject *processorObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc;

   cmsLog_debug("Adding default processor objects (enter)");

   /* first check if there is a Device.DeviceInfo.Processor.{i}. instance  first */
   ret = mdm_getNextObject(MDMOID_DEV2_PROCESSOR, &iidStack, (void **) &processorObj);

   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_debug("Existing processor object %s found, just return", processorObj->architecture);
      mdm_freeObject((void **) &processorObj);
      return ret;
   }

   processorNum = rutSys_getNumCpuThreads();

   for (i = 0; i < processorNum; i++)
   {
      frequency = 0;
      architecture[0] = '\0';

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_PROCESSOR;

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_addObjectInstance for MDMOID_DEV2_PROCESSOR failed, ret=%d", ret);
         return ret;
      }

      if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &processorObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to get Dev2ProcessorObject object, ret=%d", ret);
         return ret;
      }

      if (rutSys_getCpuInfo(i, &frequency, architecture) == CMSRET_SUCCESS)
      {
         CMSMEM_REPLACE_STRING_FLAGS(processorObj->architecture, architecture, mdmLibCtx.allocFlags);
         processorObj->X_BROADCOM_COM_Frequency = frequency;
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(processorObj->architecture, "", mdmLibCtx.allocFlags);
         processorObj->X_BROADCOM_COM_Frequency = 0;
      }

      cmsLog_debug("Before set Dev2ProcessorObject, arch=%s, freq=%d",
                   processorObj->architecture, processorObj->X_BROADCOM_COM_Frequency);

      ret = mdm_setObject((void **) &processorObj, &pathDesc.iidStack, FALSE);
      // clean up - processorObj should be freed here
      mdm_freeObject((void **)&processorObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set Dev2ProcessorObject. ret=%d", ret);
         return ret;
      }
   }

   return ret;
}

#endif  /* DMP_DEVICE2_PROCESSORS_1 */

#ifdef DMP_DEVICE2_QOS_1
CmsRet mdmInit_addQosQueue_dev2(UINT32 precedence, UINT32 qid,
                                const char *fullPath, const char *qname,
                                UBOOL8 enable)
{
   MdmPathDescriptor  pathDesc;
   Dev2QosObject      *mdmObj = NULL;
   Dev2QosQueueObject *qObj   = NULL;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_QOS_QUEUE;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance returns error. ret=%d", ret);
      return ret;
   }

   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **)&qObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_getObject returns error. ret=%d", ret);
      return ret;
   }

    qObj->enable                 = enable;
    qObj->precedence             = precedence;
    qObj->X_BROADCOM_COM_QueueId = qid;
    CMSMEM_REPLACE_STRING_FLAGS(qObj->interface, fullPath, mdmLibCtx.allocFlags);
    CMSMEM_REPLACE_STRING_FLAGS(qObj->X_BROADCOM_COM_QueueName, qname, mdmLibCtx.allocFlags);

   if ((ret = mdm_setObject((void **)&qObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_setObject returns error. ret=%d", ret);
   }

   mdm_freeObject((void **)&qObj);

   /* increment the queue count */   
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_QOS;

   /* get the queue management object */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **)&mdmObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_getObject returns error. ret=%d", ret);
      return ret;
   }
   
   mdmObj->queueNumberOfEntries++;
   if ((ret = mdm_setObject((void **)&mdmObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_setObject returns error. ret=%d", ret);
   }

   mdm_freeObject((void **)&mdmObj);
   return ret;
}
#endif  /* DMP_DEVICE2_QOS_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
