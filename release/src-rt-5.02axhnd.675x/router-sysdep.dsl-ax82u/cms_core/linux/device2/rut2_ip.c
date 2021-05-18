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

#ifdef DMP_DEVICE2_IPINTERFACE_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_strconv2.h"
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_ip.h"
#include "rut_pmap.h"



/*!\file rut2_ip.c
 * \brief Helper functions for rcl2_ip.c and stl2_ip.c
 *
 */


void rutIp_sendStaticAddrConfigToSsk(const char *ifName, UBOOL8 isIPv4,
                                     UBOOL8 isAdd, UBOOL8 isMod, UBOOL8 isDel)
{
   char buf[sizeof(CmsMsgHeader) + sizeof(IntfStackStaticAddressConfig)]={0};
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   IntfStackStaticAddressConfig *info = (IntfStackStaticAddressConfig *) (msg+1);
   CmsRet ret;

   msg->type = CMS_MSG_INTFSTACK_STATIC_ADDRESS_CONFIG;
   msg->src =  mdmLibCtx.eid;
   msg->dst = EID_SSK;
   msg->flags_event = 1;
   msg->dataLength = sizeof(IntfStackStaticAddressConfig);

   strcpy(info->ifName, ifName);
   info->isIPv4 = isIPv4;
   info->isAdd = isAdd;
   info->isMod = isMod;
   info->isDel = isDel;

   ret = cmsMsg_send(mdmLibCtx.msgHandle, msg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send STATIC_ADDRESS_CONFIG msg, ret=%d", ret);
   }
   else
   {
      cmsLog_debug("sent STATIC_ADDRESS_CONFIG "
                   "(ifName=%s isIPv4=%d isAdd=%d, isMod=%d isDel=%d",
                   ifName, isIPv4, isAdd, isMod, isDel);
   }
}

void rutIp_sendIntfStackPropagateMsgToSsk(char *lowLayerFullPath)
{
   char buf[sizeof(CmsMsgHeader) + sizeof(IntfStackPropagateStaus)]={0};
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   IntfStackPropagateStaus *info = (IntfStackPropagateStaus *) (msg+1);
   CmsRet ret;

   msg->type = CMS_MSG_INTFSTACK_PROPAGATE_STATUS;
   msg->src =  mdmLibCtx.eid;
   msg->dst = EID_SSK;
   msg->flags_event = 1;
   msg->dataLength = sizeof(IntfStackPropagateStaus);

   cmsUtl_strcpy(info->ipLowerLayerFullPath, lowLayerFullPath);

   ret = cmsMsg_send(mdmLibCtx.msgHandle, msg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send CMS_MSG_INTFSTACK_PROPAGATE_STATUS msg, ret=%d", ret);
   }
   else
   {
      cmsLog_debug("sent CMS_MSG_INTFSTACK_PROPAGATE_STATUS with %s", lowLayerFullPath);
   }   
}


UBOOL8 rutIp_isUpstream(const char *fullPath)
{
   char higherLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   char lowerLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   UBOOL8 isUpstream;
   CmsRet ret;

   cmsLog_debug("Entered: fullPath=%s", fullPath);

   cmsUtl_strncpy(lowerLayerBuf, fullPath, sizeof(lowerLayerBuf));

   while (!qdmIntf_isFullPathLayer2Locked_dev2(lowerLayerBuf) &&
          ret == CMSRET_SUCCESS)
   {
      /* go down one more level */
      cmsUtl_strncpy(higherLayerBuf, lowerLayerBuf, sizeof(higherLayerBuf));
      memset(lowerLayerBuf, 0, sizeof(lowerLayerBuf));

      ret = qdmIntf_getFirstLowerLayerFromFullPathLocked_dev2(higherLayerBuf,
                                                      lowerLayerBuf,
                                                      sizeof(lowerLayerBuf));
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not convert %s, ret=%d", higherLayerBuf, ret);
         return FALSE;
      }

      if (IS_EMPTY_STRING(lowerLayerBuf))
      {
         cmsLog_error("did not find layer2 intf before hitting empty lowerLayer (higher=%s)", higherLayerBuf);
         return FALSE;
      }

      cmsLog_debug("%s ==> %s", higherLayerBuf, lowerLayerBuf);
   }

   /* if we get here, we must have hit the layer 2 interface */
   isUpstream = qdmIntf_isLayer2FullPathUpstreamLocked_dev2(lowerLayerBuf);

   return isUpstream;
}


CmsRet rutIp_getIfnameFromLowerLayers(const char *fullPath,
                                      char *ifnameBuf, UINT32 bufLen)
{
   char lowerLayerFullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   CmsRet ret;

   cmsLog_debug("fullPath=%s", fullPath);

   /* check every lower layer object we encounter.  As soon as we
    * get an ifname, we are done.
    */
   ret = qdmIntf_getIntfnameFromFullPathLocked_dev2(fullPath,
                                                    ifnameBuf, bufLen);
   if ((ret == CMSRET_SUCCESS) &&
       (cmsUtl_strlen(ifnameBuf) > 0))
   {
      cmsLog_debug("found %s", ifnameBuf);
      return ret;
   }
   else
   {
      /* recurse to next lower layer down */
      qdmIntf_getFirstLowerLayerFromFullPathLocked_dev2(fullPath,
                                                      lowerLayerFullPathBuf,
                                               sizeof(lowerLayerFullPathBuf));
      if (cmsUtl_strlen(lowerLayerFullPathBuf) > 0)
      {
         return (rutIp_getIfnameFromLowerLayers(lowerLayerFullPathBuf,
                                                ifnameBuf, bufLen));
      }
      else
      {
         /* we've hit the end */
         return CMSRET_NO_MORE_INSTANCES;
      }
   }
}



void rutIp_configureIpv4Addr(const char *ipIntfName,
                     const char *ipAddress, const char *subnetMask)
{
   char bCastStr[CMS_IPADDR_LENGTH]={0};
   CmsRet ret;

   cmsLog_debug("Enter: ipIntfName=%s ipAddr=%s subnet=%s",
                ipIntfName, ipAddress, subnetMask);

   if (cmsUtl_strlen(ipIntfName) == 0)
   {
      /* for whatever reason, the ifname is not yet known.  This function
       * cannot do anything without the ifname.
       */
      cmsLog_debug("no ipIntfName, just return");
      return;
   }

   if ((ret = rut_getBCastFromIpSubnetMask((char *) ipAddress,
                                           (char *) subnetMask,
                                           bCastStr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get bcastStr from %s/%s, ret=%d",
            ipAddress, subnetMask, ret);
   }
   else
   {
      /* if it is ppp, don't need to configure ip address */
      if (cmsUtl_strncmp(ipIntfName, "ppp", cmsUtl_strlen("ppp")))
      {
          char cmdStr[BUFLEN_128];
          snprintf(cmdStr, sizeof(cmdStr),
                   "ifconfig %s %s netmask %s broadcast %s up",
                   ipIntfName,
                   ipAddress, subnetMask, bCastStr);
          rut_doSystemAction("rut2_ip", cmdStr);

          /* is the sendarp always on br0 or on the ifname of this IP.Interface? */
          snprintf(cmdStr, sizeof(cmdStr), "sendarp -s %s -d %s",
                                            ipIntfName, ipIntfName);
          rut_doSystemAction("rut2_ip", cmdStr);
          /* smbd bind br0's ip,must restart after config br0's ip address*/
#ifdef SUPPORT_SAMBA
          if(cmsUtl_strcmp(ipIntfName,"br0") == 0)
          {   
             SINT32 pid;
             UBOOL8 isActive;

             isActive = rut_isApplicationActive(EID_SAMBA);
             if(isActive)
             {	
                if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_SAMBA, NULL,0)) == CMS_INVALID_PID)
                   cmsLog_error("failed to restart SAMBA server.");
             }
             else
             {	 
                if ((pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_SAMBA, NULL,0)) == CMS_INVALID_PID)
                   cmsLog_error("failed to start SAMBA server.");
             }
         }
#endif   
      }
      rutMulti_reloadMcpd();
   }

   return;
}


void rutIp_unconfigureIpv4Addr(const char *ipIntfName)
{
   char cmdStr[BUFLEN_128];

   cmsLog_debug("Enter: ipIntfName=%s", ipIntfName);

   if (cmsUtl_strlen(ipIntfName) == 0)
   {
      /* for whatever reason, the ifname is not yet known.  This function
       * cannot do anything without the ifname.
       */
      cmsLog_debug("no ipIntfName, just return");
      return;
   }

   /* hmmm, don't know how to unconfigure just one address specifically.
    * Just slam a 0.0.0.0 onto the interface.
    */
   snprintf(cmdStr, sizeof(cmdStr),
      "ifconfig %s 0.0.0.0 netmask 0.0.0.0 broadcast 0.0.0.0 up 2>/dev/null",
       ipIntfName);
   rut_doSystemAction("rut2_ip", cmdStr);

   /* is the sendarp always on br0 or on the ifname of this IP.Interface? */
   snprintf(cmdStr, sizeof(cmdStr), "sendarp -s %s -d %s",
                                     ipIntfName, ipIntfName);
   rut_doSystemAction("rut2_ip", cmdStr);

   return;
}


static CmsRet configInterfaceIpAddress(const char *ifName, UBOOL8 activate)
{
   InstanceIdStack parentIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2IpInterfaceObject *ipIntf=NULL;
   UBOOL8 found=FALSE; 
   UBOOL8 prevHideObjectsPendingDelete;
   CmsRet ret;
   
   if (cmsUtl_strlen(ifName) == 0)
   {
      cmsLog_error("no ipIntfName, just return");
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_debug("Entered: ifName %s, activate %d", ifName, activate);

   /* this function might be called when IP.Interface is being deleted, so
    * tell MDM to show us the object even though it is pending delete. */
   prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
   mdmLibCtx.hideObjectsPendingDelete = FALSE;

   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, 
                                     &parentIidStack,
                                     OGF_NO_VALUE_UPDATE, 
                                     (void **) &ipIntf)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ipIntf->name, ifName))
      {
         found = TRUE;
      }
      cmsObj_free((void **) &ipIntf);
   }

   mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete; // restore value


   if (!found)
   {
      cmsLog_error("Could not find IP.Interface for %s", ifName);
   }
   else
   {
      Dev2Ipv4AddressObject *ipv4AddrObj=NULL;
      InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
      InstanceIdStack savedIidStack=EMPTY_INSTANCE_ID_STACK;

      /* Walk over all IPv4Address objects in this IP.Interface */
      while (cmsObj_getNextInSubTree(MDMOID_DEV2_IPV4_ADDRESS,
                                     &parentIidStack,
                                     &iidStack,
                                     (void **) &ipv4AddrObj) == CMSRET_SUCCESS)
      {
         cmsLog_debug("Found IPv4 address %s for %s", ipv4AddrObj->IPAddress, ifName);
      
         /*
          * If this is a deactivate and this ipv4AddrObj is not static (i.e.
          * dynamically created), just delete it.  The RCL handler function
          * will unconfig the IP address.
          * Otherwise, do a "set" without modifying any params to allow
          * the RCL handler to take some action
          */
         if (!activate && cmsUtl_strcmp(ipv4AddrObj->addressingType, MDMVS_STATIC))
         {
            cmsObj_deleteInstance(MDMOID_DEV2_IPV4_ADDRESS, &iidStack);
            /* since we did a delete, restore iidStack to last good one */
            iidStack = savedIidStack;
         }
         else
         {
            if ((ret = cmsObj_set(ipv4AddrObj, &iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsObj_set ipv4AddrObj returns error. ret=%d", ret);
            }
         }
         
         cmsObj_free((void **) &ipv4AddrObj);
         /* save current iidStack in case we delete the next one */
         savedIidStack = iidStack;
      }
   }
   
   cmsLog_debug("Exit. ret %d", ret);

   return ret;
}


CmsRet rutIp_activateIpv4Interface_dev2(const char *ifName)
{
   UBOOL8 activate=TRUE;
   cmsLog_debug("Enter: ifname=%s", ifName);
   return (configInterfaceIpAddress(ifName, activate));

}

CmsRet rutIp_deactivateIpv4Interface_dev2(const char *ifName)
{
   UBOOL8 activate=FALSE;
   cmsLog_debug("Enter: ifname=%s", ifName);
   return(configInterfaceIpAddress(ifName, activate));
   
}


CmsRet rutIp_addIpv4AddressObject_dev2(const char *ipIntfFullPath,
                                       const char *ipAddrStr, 
                                       const char *subnetMask,
                                       const char *addressingType)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv4AddressObject *ipv4AddrObj=NULL;
   CmsRet ret;
   MdmPathDescriptor pathDesc;
   UBOOL8 foundExisting=FALSE;

   INIT_PATH_DESCRIPTOR(&pathDesc);
   if ((ret = cmsMdm_fullPathToPathDescriptor(ipIntfFullPath, &pathDesc)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s ", ipIntfFullPath );
      return ret;
   }

   if (pathDesc.oid != MDMOID_DEV2_IP_INTERFACE)
   {
      cmsLog_error("fullPath (%s) must point to IP.Interface", ipIntfFullPath);
      return ret;
   }


   /*
    * Special case hack for dhcp client on br0:
    * When dhcp client gets the IP addr, overwrite the existing static IP addr.
    * XXX TODO: the right way is to create a secondary IP address using
    * the Linux IP alias mechanism (br0:1).  That alias interface would
    * be configured with the dynamic address.  But a lot of our code assumes
    * there is only 1 IP addr per interface.
    */
   if (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV4_ADDRESS,
                                    &pathDesc.iidStack,
                                    &iidStack,
                                    OGF_NO_VALUE_UPDATE,
                                   (void **) &ipv4AddrObj) == CMSRET_SUCCESS)
   {
      cmsLog_debug("found existng ip addr obj (%s)", ipv4AddrObj->IPAddress);

      /* For Homeplug device, br0 ip address object type should be DHCP, the code here changes DHCP 
      * to Static which will cause dns and default gateway ip is not properly obtained after the reboot.
      * So, just skip setting foundExiting to TRUE for homeplug to keep the addressType to DHCP.
      */
#ifndef DMP_DEVICE2_HOMEPLUG_1
      foundExisting = TRUE;
#endif       

   }
   else
   {
      iidStack = pathDesc.iidStack;
      ret = cmsObj_addInstance(MDMOID_DEV2_IPV4_ADDRESS, &iidStack);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not create IPV4_ADDRESS obj, ret=%d", ret);
         return ret;
      }

      ret = cmsObj_get(MDMOID_DEV2_IPV4_ADDRESS, &iidStack,
                       OGF_NO_VALUE_UPDATE, (void **)&ipv4AddrObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get IPv4ADDRESS obj, ret=%d", ret);
         cmsObj_deleteInstance(MDMOID_DEV2_IPV4_ADDRESS, &iidStack);
         return ret;
      }
   }

   ipv4AddrObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipv4AddrObj->IPAddress, ipAddrStr, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv4AddrObj->subnetMask, subnetMask, mdmLibCtx.allocFlags);
   if (!foundExisting)
   {
      CMSMEM_REPLACE_STRING_FLAGS(ipv4AddrObj->addressingType, addressingType, mdmLibCtx.allocFlags);
   }

   if((ret = cmsObj_set (ipv4AddrObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Set of IPV4Address object failed, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_IPV4_ADDRESS, &iidStack);
   }
   else
   {
      if (!foundExisting)
      {
         if ((ret = cmsObj_setNonpersistentInstance(MDMOID_DEV2_IPV4_ADDRESS, &iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Set of cmsObj_setNonpersistentInstance   failed, ret=%d", ret);
         }
      }
   }
   cmsObj_free((void **)&ipv4AddrObj);

   return ret;  

}

CmsRet rutIp_addMacFilterObject_dev2(const char *ipIntfFullPath)
{
   CmsRet ret=CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc;


   /* This is device 2 MAC Fileter object  is only for device 2 data model */
   if (!cmsMdm_isDataModelDevice2())
   {
      cmsLog_notice("Not for hybrid data model");
      return ret;
   }

   /* Get macFilterObj iid */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   if((ret = qdmIpIntf_getMacFilterByFullPathLocked_dev2(ipIntfFullPath, &pathDesc)) == CMSRET_SUCCESS)
   {
      cmsLog_debug("Add exist Dev2MacFilterObject on %s, ret = %d", ipIntfFullPath,ret);      
      return ret; //Success because there already exist MacFilterObject of ipIntfFullPath .
   } 
   else
   {
      Dev2MacFilterObject *macFilterObj=NULL;        
      INIT_INSTANCE_ID_STACK(&iidStack);

      cmsObj_addInstance(MDMOID_DEV2_MAC_FILTER,&iidStack);
      if ((ret = cmsObj_get(MDMOID_DEV2_MAC_FILTER, &iidStack, 0, (void **) &macFilterObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to add Dev2MacFilterObject on %s, ret = %d", ipIntfFullPath,ret);
         cmsObj_deleteInstance(MDMOID_DEV2_MAC_FILTER, &iidStack);
         return ret;
      }

      macFilterObj->enable = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(macFilterObj->IPInterface, ipIntfFullPath, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(macFilterObj->policy, MDMVS_FORWARD, mdmLibCtx.allocFlags);

      cmsObj_set((void *)macFilterObj,&iidStack);
      cmsObj_free((void **)&macFilterObj);
   }
   
   return ret;
}

CmsRet rutIp_delMacFilterObject_dev2(const char *ipIntfFullPath)
{
   CmsRet ret=CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc;

   /* This MAC filter object is only for device 2 data model */
   if (!cmsMdm_isDataModelDevice2())
   {
      cmsLog_notice("Not for hybrid data model");
      return ret;
   }

   /* Get macFilterObj iid */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   if((ret = qdmIpIntf_getMacFilterByFullPathLocked_dev2(ipIntfFullPath, &pathDesc)) == CMSRET_SUCCESS)
   {
      ret = cmsObj_deleteInstance(pathDesc.oid, &(pathDesc.iidStack));      
   } 
   
   return ret;
}

#ifdef DMP_DEVICE2_BRIDGE_1  /* aka SUPPORT_PORT_MAP */

/* For ip interface setup/tear_down */
CmsRet rutPMap_configPortMapping_dev2(const UBOOL8 isAdd, const char *ifName, const char *ipIntfFullPath)
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   char gwIP[CMS_IPADDR_LENGTH]={0};
   CmsRet ret = CMSRET_SUCCESS;
   char wanIntfName[CMS_IFNAME_LENGTH]={0};
  	
   cmsLog_debug("isAdd=%d, ifName=%s, ipIntfFullPath=%s", isAdd, ifName, ipIntfFullPath);      

   qdmRt_getGatewayIpv4AddrByFullPathLocked_dev2(ipIntfFullPath, gwIP);
   
   ipIntfObj = getIpIntfObjByDirection(ifName, QDM_IPINTF_DIR_ANY);
  
   if (ipIntfObj &&
       ipIntfObj->X_BROADCOM_COM_Upstream &&
      !ipIntfObj->X_BROADCOM_COM_BridgeService &&
      !IS_EMPTY_STRING(ipIntfObj->X_BROADCOM_COM_GroupName)	&&	 
       cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_GroupName, "Default"))		 
   {
      cmsLog_debug("gwIP=%s, bridgeName=%s, groupName=%s", gwIP, ifName, ipIntfObj->X_BROADCOM_COM_BridgeName, ipIntfObj->X_BROADCOM_COM_GroupName);      
   
      /* perform the add policy route action */
      ret = rutPMap_doPoliceRoutingOnWanIfc(isAdd, ifName, gwIP,
                                      ipIntfObj->X_BROADCOM_COM_BridgeName,
                                      ipIntfObj->X_BROADCOM_COM_GroupName);
   }

   /* for bridge interface up, check if any wan interface grouped to this bridge*/
   if (ipIntfObj &&
      !ipIntfObj->X_BROADCOM_COM_Upstream &&
      !IS_EMPTY_STRING(ipIntfObj->X_BROADCOM_COM_GroupName)	&&	 
       cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_GroupName, "Default"))		 
   {
      qdmIpIntf_getWanIntfNameByGroupNameLocked_dev2(ipIntfObj->X_BROADCOM_COM_GroupName, wanIntfName);

      if (!IS_EMPTY_STRING(wanIntfName) && qdmIpIntf_isNatEnabledOnIntfNameLocked_dev2(wanIntfName))
      {
         rutIpt_configNatForIntfGroup_dev2(TRUE, wanIntfName, ifName, qdmIpIntf_isFullConeNatEnabledOnIntfNameLocked_dev2(wanIntfName));
      }
   }
   

   /* When first boot ,ETH WAN will up early then br0, so NAT doesn't perform due to br0 not up yet.
      So we need redo nat when "Default" bridge interface up, check all wan interface grouped to this bridge
   */
   if (ipIntfObj &&
      !ipIntfObj->X_BROADCOM_COM_Upstream &&
      !cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_GroupName, "Default")
      )		 
   {
      Dev2IpInterfaceObject *wan_ipIntfObj = NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      CmsRet ret2;
   
      while (
             (ret2 = cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                                  OGF_NO_VALUE_UPDATE,
                                  (void **)&wan_ipIntfObj)) == CMSRET_SUCCESS)
      {
         if (wan_ipIntfObj->X_BROADCOM_COM_Upstream &&
             (    IS_EMPTY_STRING(wan_ipIntfObj->X_BROADCOM_COM_GroupName)   
               || !cmsUtl_strcmp(wan_ipIntfObj->X_BROADCOM_COM_GroupName, "Default")
             )
            )
         {
            if (qdmIpIntf_isNatEnabledOnIntfNameLocked_dev2(wan_ipIntfObj->name))
            {
               rutIpt_initNat(wan_ipIntfObj->name,qdmIpIntf_isFullConeNatEnabledOnIntfNameLocked_dev2(wan_ipIntfObj->name));
            }
         }
         cmsObj_free((void **)&wan_ipIntfObj);
      }
   }

   if (ipIntfObj)
      cmsObj_free((void **)&ipIntfObj);
   
   return ret;
}

void rutPMap_configIntfGrouping(Dev2IpInterfaceObject *newObj, Dev2IpInterfaceObject *currObj, const char *ipIntfFullPath)
{
   char gwIP[CMS_IPADDR_LENGTH]={0};
 		
   qdmRt_getGatewayIpv4AddrByFullPathLocked_dev2(ipIntfFullPath, gwIP);

   if ( currObj->X_BROADCOM_COM_Upstream &&
        !currObj->X_BROADCOM_COM_BridgeService &&
       (!cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv4ServiceStatus, MDMVS_SERVICEUP) ||
        !cmsUtl_strcmp(currObj->X_BROADCOM_COM_IPv6ServiceStatus, MDMVS_SERVICEUP)))
   
   {
      cmsLog_debug("Disassociate wan interface %s from group %s", 
                   currObj->name, currObj->X_BROADCOM_COM_GroupName);
 
      if (!IS_EMPTY_STRING(currObj->X_BROADCOM_COM_GroupName) && cmsUtl_strcmp(currObj->X_BROADCOM_COM_GroupName, "Default"))      
      {
         char   *savedGroupName;
         char   *savedBridgeName;
 
         /*Sarah: TODO: 
          *In rutPMap_doPoliceRoutingOnWanIfc, rutRt_deletePolicyRouting->getActionInfoFromPortMappingRuleName_dev2
          *will look for origin policy routing info saved in currObj, so we do a simple restore old value here.
          *To eliminate this hack code require a re-fabric of policy route function for both tr181 and tr98.
         */			   
         savedGroupName = newObj->X_BROADCOM_COM_GroupName;
         savedBridgeName = newObj->X_BROADCOM_COM_BridgeName;
         newObj->X_BROADCOM_COM_GroupName = currObj->X_BROADCOM_COM_GroupName;
         newObj->X_BROADCOM_COM_BridgeName = currObj->X_BROADCOM_COM_BridgeName;
 		   
         rutPMap_doPoliceRoutingOnWanIfc(FALSE, currObj->name, gwIP,
                                         currObj->X_BROADCOM_COM_BridgeName, 
                                         currObj->X_BROADCOM_COM_GroupName);
 		   
         newObj->X_BROADCOM_COM_GroupName = savedGroupName;
         newObj->X_BROADCOM_COM_BridgeName = savedBridgeName;
     }

#ifdef DMP_DEVICE2_NAT_1
     if (qdmIpIntf_isNatEnabledOnIpIntfFullPathLocked_dev2(ipIntfFullPath))
     {
        rutIpt_configNatForIntfGroup_dev2(FALSE, currObj->name, 
                                          IS_EMPTY_STRING(currObj->X_BROADCOM_COM_BridgeName)?"br0":currObj->X_BROADCOM_COM_BridgeName, 
                                          qdmIpIntf_isFullConeNatEnabledOnIpIntfFullPathLocked_dev2(ipIntfFullPath));
     }
#endif     			
   }		   
 
   if ( newObj->X_BROADCOM_COM_Upstream &&
        !newObj->X_BROADCOM_COM_BridgeService &&
       (!cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv4ServiceStatus, MDMVS_SERVICEUP) ||
        !cmsUtl_strcmp(newObj->X_BROADCOM_COM_IPv6ServiceStatus, MDMVS_SERVICEUP)))
   
   {
      cmsLog_debug("Associate wan interface %s to group %s", 
                   newObj->name, newObj->X_BROADCOM_COM_GroupName);
 		
      if (!IS_EMPTY_STRING(newObj->X_BROADCOM_COM_GroupName) && cmsUtl_strcmp(newObj->X_BROADCOM_COM_GroupName, "Default"))      
      {
        rutPMap_doPoliceRoutingOnWanIfc(TRUE, newObj->name, gwIP,
                                         newObj->X_BROADCOM_COM_BridgeName, 
                                         newObj->X_BROADCOM_COM_GroupName);
      }

#ifdef DMP_DEVICE2_NAT_1
      if (qdmIpIntf_isNatEnabledOnIpIntfFullPathLocked_dev2(ipIntfFullPath))
      {
        rutIpt_configNatForIntfGroup_dev2(TRUE, newObj->name, 
                                          IS_EMPTY_STRING(newObj->X_BROADCOM_COM_BridgeName)?"br0":newObj->X_BROADCOM_COM_BridgeName, 
                                          qdmIpIntf_isFullConeNatEnabledOnIpIntfFullPathLocked_dev2(ipIntfFullPath));
      }
#endif     			
   }		  
 
   /* Sarah: TODO: not sure how dns should work in this stage, implement it later...... */
   /* When adding the interface group, need to update the dns info in the var/dnsinfo.conf */
   //rutDpx_updateDnsproxy();
 
   /* Sarah: dont see why need to update dhcpd here, leave it for now */
   /*update latest information to udhcpd.conf and inform dhcpd*/
   //rutLan_updateDhcpd();
   
   /* make sure MCPD is updated with the latest interface grouping information */
   rutMulti_reloadMcpd();
}

#endif /* DMP_DEVICE2_BRIDGE_1 */ /* aka SUPPORT_PORT_MAP */

#endif  /* DMP_DEVICE2_IPINTERFACE_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */



