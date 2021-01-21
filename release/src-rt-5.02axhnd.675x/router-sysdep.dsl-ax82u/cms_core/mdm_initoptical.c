/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
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
#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "mdm.h"
#include "mdm_private.h"
#include "qdm_intf.h"

extern CmsRet rutBridge_addFullPathToBridge_dev2(const char *fullPath, const char *brIntfName);

#ifdef DMP_DEVICE2_OPTICAL_1

static CmsRet mdm_addOpticalInterfaceObject(const char *ifName)
{
    CmsRet ret = CMSRET_SUCCESS;
    MdmPathDescriptor  pathDesc;
    DeviceOpticalObject *mdmObjDev = NULL;
    OpticalInterfaceObject *mdmObj = NULL;

    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_OPTICAL_INTERFACE;

    if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to add OpticalInterfaceObject, ret=%d", ret);
        return ret;
    }

    if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &mdmObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get OpticalInterfaceObject, ret=%d", ret);
        mdm_deleteObjectInstance(&pathDesc, NULL, NULL);
        return ret;
    }

    CMSMEM_REPLACE_STRING_FLAGS(mdmObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
    CMSMEM_REPLACE_STRING_FLAGS(mdmObj->name, ifName, mdmLibCtx.allocFlags);
    mdmObj->upstream = TRUE;
#if (defined(DMP_X_BROADCOM_COM_EPONWAN_1) && defined(EPON_SFU) || defined(DMP_X_BROADCOM_COM_GPONWAN_1) && defined(GPON_SFU))
    mdmObj->enable = TRUE;
#endif

    ret = mdm_setObject((void **)&mdmObj, &pathDesc.iidStack, FALSE);
    mdm_freeObject((void **) &mdmObj);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set OpticalInterfaceObject. ret=%d", ret);
        return ret;
    }

    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_DEVICE_OPTICAL;

    if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &mdmObjDev)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get DeviceOpticalObject, ret=%d", ret);
        return ret;
    }

    mdmObjDev->interfaceNumberOfEntries++;
    ret = mdm_setObject((void **)&mdmObjDev, &pathDesc.iidStack, FALSE);
    mdm_freeObject((void **) &mdmObjDev);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set DeviceOpticalObject. ret=%d", ret);
        return ret;
    }

    return ret;
}

static int mdm_isOpticalInterfaceObjectExists(const char *ifName)
{
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    OpticalInterfaceObject *mdmObj = NULL;
    int found = 0;

    while (!found && mdm_getNextObject(MDMOID_OPTICAL_INTERFACE, &iidStack, (void **) &mdmObj) == CMSRET_SUCCESS)
    {
        found = (cmsUtl_strcmp(mdmObj->name, ifName) == 0);
        mdm_freeObject((void **) &mdmObj);
    }

    return found;
}

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1   
#ifdef EPON_SFU
static CmsRet addEthernetLink(const char *lowerLayer, char *myPathRef, UINT32 bufLen)
{
   char *fullPathStringPtr = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;   
   Dev2EthernetLinkObject *ethLinkObj = NULL;
   MdmPathDescriptor ethLinkPathDesc;
   CmsRet ret;

   cmsLog_debug("lowerLayer %s", lowerLayer);
   
   /* Create an Ethernet link object */
   if ((ret = cmsObj_addInstance(MDMOID_DEV2_ETHERNET_LINK, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add ethLink Instance, ret = %d", ret);
      return ret;
   } 

   if ((ret = cmsObj_get(MDMOID_DEV2_ETHERNET_LINK, &iidStack, 0, (void **) &ethLinkObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get ethLinkObj, ret = %d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_ETHERNET_LINK, &iidStack);
      return ret;
   }

   CMSMEM_REPLACE_STRING_FLAGS(ethLinkObj->lowerLayers, lowerLayer, mdmLibCtx.allocFlags);
   ethLinkObj->enable = TRUE;

   /* MacAddress need to be set when the layer 2 link is up in rcl/rut */
   ret =  cmsObj_set(ethLinkObj, &iidStack);
   cmsObj_free((void **) &ethLinkObj); 

   if (ret  != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ethLinkObj. ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_ETHERNET_LINK, &iidStack);
      return ret;
   } 

   /* Get the Ethernet Link object full path string to be used as ip interface lower layer */
   INIT_PATH_DESCRIPTOR(&ethLinkPathDesc);
   ethLinkPathDesc.iidStack = iidStack;
   ethLinkPathDesc.oid = MDMOID_DEV2_ETHERNET_LINK;
   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&ethLinkPathDesc, &fullPathStringPtr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_ETHERNET_LINK, &iidStack);
      return ret;
   }
   if (cmsUtl_strlen(fullPathStringPtr) > (SINT32) bufLen)
   {
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsUtl_strncpy(myPathRef, fullPathStringPtr, bufLen);
      ret = CMSRET_SUCCESS;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);

   cmsLog_debug("Exit, ret=%d", ret);

   return ret;

}

static CmsRet addEthernetVlanTermination(const char *lowerLayer, char *myPathRef, UINT32 bufLen)
{
   char *fullPathStringPtr = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;   
   Dev2VlanTerminationObject *ethVlanObj = NULL;
   MdmPathDescriptor ethVlanTerminationPathDesc;
   CmsRet ret;

   cmsLog_debug("lowerLayer %s", lowerLayer);
   
   /* Create an Ethernet VlanTermination object */
   if ((ret = cmsObj_addInstance(MDMOID_DEV2_VLAN_TERMINATION, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add VLAN_TERMINATION Instance, ret = %d", ret);
      return ret;
   } 

   if ((ret = cmsObj_get(MDMOID_DEV2_VLAN_TERMINATION, &iidStack, 0, (void **) &ethVlanObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get VLAN_TERMINATION object, ret = %d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_VLAN_TERMINATION, &iidStack);
      return ret;
   }

   CMSMEM_REPLACE_STRING_FLAGS(ethVlanObj->lowerLayers, lowerLayer, mdmLibCtx.allocFlags);
   ethVlanObj->VLANID = -1;
   ethVlanObj->X_BROADCOM_COM_Vlan8021p = -1;
   ethVlanObj->TPID = -1;
   ethVlanObj->enable = TRUE;

   ret =  cmsObj_set(ethVlanObj, &iidStack);
   cmsObj_free((void **) &ethVlanObj); 

   if (ret  != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ethVlanObj. ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_VLAN_TERMINATION, &iidStack);
      return ret;
   } 

   /* Get the Ethernet VlanTermination object full path string to be used as ip interface lower layer */
   INIT_PATH_DESCRIPTOR(&ethVlanTerminationPathDesc);
   ethVlanTerminationPathDesc.iidStack = iidStack;
   ethVlanTerminationPathDesc.oid = MDMOID_DEV2_VLAN_TERMINATION;
   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&ethVlanTerminationPathDesc, &fullPathStringPtr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_VLAN_TERMINATION, &iidStack);
      return ret;
   }
   if (cmsUtl_strlen(fullPathStringPtr) > (SINT32) bufLen)
   {
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsUtl_strncpy(myPathRef, fullPathStringPtr, bufLen);
      ret = CMSRET_SUCCESS;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);


   cmsLog_debug("Exit, ret=%d", ret);

   return ret;

}

static CmsRet addDefaultIpIntfObject(const char *lowerLayer)
{
   Dev2IpInterfaceObject *ipIntf = NULL;
   MdmPathDescriptor intPathDesc;
   CmsRet ret;
   
   cmsLog_debug("Enter: lowerLayer=%s", lowerLayer);

   INIT_PATH_DESCRIPTOR(&intPathDesc);
   intPathDesc.oid = MDMOID_DEV2_IP_INTERFACE;
   if ((ret = mdm_addObjectInstance(&intPathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
        cmsLog_error("Failed to add IP Instance, ret = %d", ret);
        return ret;
   } 

   if ((ret = mdm_getObject(intPathDesc.oid, &intPathDesc.iidStack, (void **) &ipIntf)) != CMSRET_SUCCESS)
   {
        cmsLog_error("Failed to get IP interface object, ret = %d", ret);
        mdm_deleteObjectInstance(&intPathDesc, NULL, NULL);
        return ret;
   }
   
   ipIntf->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipIntf->alias, "bridge", mdmLibCtx.allocFlags);
   
   ipIntf->IPv4Enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipIntf->lowerLayers, lowerLayer, mdmLibCtx.allocFlags);

   ipIntf->X_BROADCOM_COM_BridgeService = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ipIntf->X_BROADCOM_COM_BridgeName, "br0", mdmLibCtx.allocFlags);

#ifdef DMP_X_BROADCOM_COM_IGMP_1
   ipIntf->X_BROADCOM_COM_IGMPEnabled = TRUE;
   ipIntf->X_BROADCOM_COM_IGMP_SOURCEEnabled = TRUE;
#endif

#ifdef DMP_X_BROADCOM_COM_MLD_1
   ipIntf->X_BROADCOM_COM_MLDEnabled = TRUE;
   ipIntf->X_BROADCOM_COM_MLD_SOURCEEnabled = TRUE;
#endif

   ret = mdm_setObject((void **)&ipIntf, &intPathDesc.iidStack, FALSE);
   mdm_freeObject((void **) &ipIntf);
   if (ret != CMSRET_SUCCESS)
   {
        cmsLog_error("Failed to set ipIntf. ret=%d", ret);
        return ret;      
   } 

   cmsLog_debug("Exit: ret=%d", ret);

   return ret;
}

#ifdef BRCM_PKTCBL_SUPPORT
static CmsRet modifyDhcpv4LeasedtimeOption(const char *value,
                                       const char *ipIntfFullPath)
{
    char alias[BUFLEN_48];
    UBOOL8 found = FALSE;
    UINT32 tagNum = 51;
    Dev2Dhcpv4ClientObject *dhcp4Client = NULL;
    Dev2Dhcpv4ClientReqOptionObject *reqOption = NULL;
    InstanceIdStack parentIidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack clientIidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack reqOptionIidStack = EMPTY_INSTANCE_ID_STACK;
    MdmPathDescriptor optionPathDesc;
    CmsRet ret = CMSRET_SUCCESS;

    cmsLog_debug("enter");

    if (ipIntfFullPath == NULL)
    {
        cmsLog_error("Full path of DHCPv4 client interface is NULL");
        return CMSRET_INVALID_ARGUMENTS;
    }

    if (value == NULL)
    {
        cmsLog_error("Option value is NULL");
        return CMSRET_INVALID_ARGUMENTS;
    }

    while(found == FALSE && 
         (ret = mdm_getNextObjectInSubTree(MDMOID_DEV2_DHCPV4_CLIENT,
                                    &parentIidStack, 
                                    &clientIidStack,
                                    (void **)&dhcp4Client)) == CMSRET_SUCCESS)
    {
        if (!cmsUtl_strcmp(dhcp4Client->interface, ipIntfFullPath))
        {
         found = TRUE;
        }
        mdm_freeObject((void **)&dhcp4Client);
    }   

    if (found == FALSE)
    {
        cmsLog_error("Fail to find the dhcp client info for %s", ipIntfFullPath);
        return CMSRET_INTERNAL_ERROR;
    }

    found = FALSE;

    /* Loop through Dev2Dhcpv4ClientReqOptionObject
    * to find object that has the same tag */
    while (found == FALSE &&
          mdm_getNextObjectInSubTree(MDMOID_DEV2_DHCPV4_CLIENT_REQ_OPTION,
                                       &clientIidStack,
                                       &reqOptionIidStack,
                                       (void **)&reqOption) == CMSRET_SUCCESS)
    {
        if (reqOption->tag == tagNum)
        {
            found = TRUE;
            /* do not free reqOption when it's found
            since it's used and freed later */
            break;
        }
        mdm_freeObject((void **)&reqOption);
    }

    if (found == FALSE)
    {
        /* need to create a new entry */
        memcpy(&reqOptionIidStack, &clientIidStack, sizeof(InstanceIdStack));
        INIT_PATH_DESCRIPTOR(&optionPathDesc);
        optionPathDesc.oid = MDMOID_DEV2_DHCPV4_CLIENT_REQ_OPTION;
        optionPathDesc.iidStack = reqOptionIidStack;

        if ((ret = mdm_addObjectInstance(&optionPathDesc, NULL, NULL)) != CMSRET_SUCCESS)
        {
         cmsLog_error("Failed to add Dev2Dhcpv4ClientReqOptionObject instance, ret = %d", ret);
         return ret;
        } 

        if ((ret = mdm_getObject(optionPathDesc.oid,
                            &optionPathDesc.iidStack,
                            (void **) &reqOption)) != CMSRET_SUCCESS)
        {
         cmsLog_error("Failed to get Dev2Dhcpv4ClientReqOptionObject instance, ret = %d", ret);
         mdm_deleteObjectInstance(&optionPathDesc, NULL, NULL);
         return ret;
        }

        memset(alias, 0, BUFLEN_48);
        sprintf(alias, "cpe-dhcp-option-%d", tagNum);

        CMSMEM_REPLACE_STRING_FLAGS(reqOption->alias, alias, mdmLibCtx.allocFlags);
        reqOption->tag = tagNum;
        reqOption->enable = TRUE;
    }

    CMSMEM_REPLACE_STRING_FLAGS(reqOption->X_BROADCOM_COM_Value, value, mdmLibCtx.allocFlags);
    /* modify entry */
    ret = mdm_setObject((void **)&reqOption, &optionPathDesc.iidStack, FALSE);
    /* free memory */
    mdm_freeObject((void **) &reqOption);

    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set Dev2Dhcpv4ClientReqOptionObject instance. ret=%d", ret);
        if (found == FALSE)
        {
            /* only delete a new entry */
            mdm_deleteObjectInstance(&optionPathDesc, NULL, NULL);
        }
    }

    cmsLog_debug("Exit, ret=%d", ret);

    return ret;
}


static
CmsRet mdm_addDhcpv6ClientObject_dev2(const char *ipIntfFullPath)
{
    MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
    Dev2Dhcpv6ClientObject *dhcp6ClientObj = NULL;
    CmsRet ret;

    pathDesc.oid = MDMOID_DEV2_DHCPV6_CLIENT;
    if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
    {
        cmsLog_error("mdm_addObjectInstance for DEV2_DHCPV6_CLIENT failed, ret=%d", ret);
        return ret;
    }

    if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &dhcp6ClientObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("failed to get dhcpv6ClientObj object, ret=%d", ret);
        mdm_deleteObjectInstance(&pathDesc, NULL, NULL);
        return ret;
    }

    CMSMEM_REPLACE_STRING_FLAGS(dhcp6ClientObj->interface, ipIntfFullPath, mdmLibCtx.allocFlags);
    dhcp6ClientObj->enable = TRUE;
    dhcp6ClientObj->requestAddresses = TRUE;
    dhcp6ClientObj->requestPrefixes = TRUE;

    ret = mdm_setObject((void **) &dhcp6ClientObj, &pathDesc.iidStack,  FALSE);
	mdm_freeObject((void **)&dhcp6ClientObj);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set dhcp6ClientObj. ret=%d", ret);
        return ret;
    }

    cmsLog_debug("Exit, ret=%d", ret);
    return ret;
}

static CmsRet addVoiceVlanTermination(const char *lowerLayer, char *myPathRef, UINT32 bufLen)
{
   char *fullPathStringPtr = NULL;
   Dev2VlanTerminationObject *voiceVlanObj = NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   cmsLog_debug("lowerLayer %s", lowerLayer);
   
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_VLAN_TERMINATION;
   /* Create an Ethernet VlanTermination object */
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
        cmsLog_error("Failed to add VLAN_TERMINATION Instance, ret = %d", ret);
        return ret;
   } 

   if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &voiceVlanObj)) != CMSRET_SUCCESS)
   {
        cmsLog_error("Failed to get VLAN_TERMINATION object, ret = %d", ret);
        mdm_deleteObjectInstance(&pathDesc, NULL, NULL);
        return ret;
   }

   CMSMEM_REPLACE_STRING_FLAGS(voiceVlanObj->lowerLayers, lowerLayer, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(voiceVlanObj->name, EPON_VOICE_WAN_IF_NAME, mdmLibCtx.allocFlags);
   voiceVlanObj->VLANID = -1;
   voiceVlanObj->X_BROADCOM_COM_Vlan8021p = -1;
   voiceVlanObj->TPID = -1;
   voiceVlanObj->enable = TRUE;

   ret =  mdm_setObject((void **)&voiceVlanObj, &pathDesc.iidStack, FALSE);
   mdm_freeObject((void **) &voiceVlanObj);

   if (ret  != CMSRET_SUCCESS)
   {
        cmsLog_error("Failed to set voiceVlanObj. ret=%d", ret);
        return ret;
   }

   /* Get the Ethernet VlanTermination object full path string to be used as ip interface lower layer */
   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathStringPtr)) != CMSRET_SUCCESS)
   {
        cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
        return ret;
   }
   if (cmsUtl_strlen(fullPathStringPtr) > (SINT32) bufLen)
   {
        ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
        cmsUtl_strncpy(myPathRef, fullPathStringPtr, bufLen);
        ret = CMSRET_SUCCESS;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);

   cmsLog_debug("Exit, ret=%d", ret);

   return ret;
}

static CmsRet addVoiceIpIntfObject(const char *lowerLayer, MdmPathDescriptor *ipIntfPathDesc)
{
    Dev2IpInterfaceObject *ipIntf = NULL;
    CmsRet ret;

    INIT_PATH_DESCRIPTOR(ipIntfPathDesc);
    ipIntfPathDesc->oid = MDMOID_DEV2_IP_INTERFACE;

    cmsLog_debug("Enter: lowerLayer=%s", lowerLayer);

    if ((ret = mdm_addObjectInstance(ipIntfPathDesc, NULL, NULL)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to add IP Instance, ret = %d", ret);
        return ret;
    } 

    if ((ret = mdm_getObject(ipIntfPathDesc->oid, &ipIntfPathDesc->iidStack, (void **) &ipIntf)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get IP interface object, ret = %d", ret);
        mdm_deleteObjectInstance(ipIntfPathDesc, NULL, NULL);
        return ret;
    }

    ipIntf->enable = TRUE;
    CMSMEM_REPLACE_STRING_FLAGS(ipIntf->alias, "emta", mdmLibCtx.allocFlags);
    
    ipIntf->IPv4Enable = TRUE;
    #ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
    ipIntf->IPv6Enable = TRUE;
    #endif
    CMSMEM_REPLACE_STRING_FLAGS(ipIntf->lowerLayers, lowerLayer, mdmLibCtx.allocFlags);
    ipIntf->X_BROADCOM_COM_BridgeService = FALSE;

    #ifdef DMP_X_BROADCOM_COM_IGMP_1
    ipIntf->X_BROADCOM_COM_IGMPEnabled = TRUE;
    ipIntf->X_BROADCOM_COM_IGMP_SOURCEEnabled = TRUE;
    #endif

    #ifdef DMP_X_BROADCOM_COM_MLD_1
    ipIntf->X_BROADCOM_COM_MLDEnabled = TRUE;
    ipIntf->X_BROADCOM_COM_MLD_SOURCEEnabled = TRUE;
    #endif

    ret = mdm_setObject((void **)&ipIntf, &ipIntfPathDesc->iidStack, FALSE);
    mdm_freeObject((void **) &ipIntf);

    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set ipIntf. ret=%d", ret);
        return ret;
    } 

    cmsLog_debug("Exit: ret=%d", ret);

    return ret;
}

static CmsRet addEptaVlanTermination(const char *lowerLayer, char *myPathRef, UINT32 bufLen)
{
   char *fullPathStringPtr = NULL;
   Dev2VlanTerminationObject *eptaVlanObj = NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   cmsLog_debug("lowerLayer %s", lowerLayer);
   
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_VLAN_TERMINATION;
   /* Create an Ethernet VlanTermination object */
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
        cmsLog_error("Failed to add VLAN_TERMINATION Instance, ret = %d", ret);
        return ret;
   } 

   if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &eptaVlanObj)) != CMSRET_SUCCESS)
   {
        cmsLog_error("Failed to get VLAN_TERMINATION object, ret = %d", ret);
        mdm_deleteObjectInstance(&pathDesc, NULL, NULL);
        return ret;
   }

   CMSMEM_REPLACE_STRING_FLAGS(eptaVlanObj->lowerLayers, lowerLayer, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(eptaVlanObj->name, EPON_EPTA_WAN_IF_NAME, mdmLibCtx.allocFlags);
   eptaVlanObj->VLANID = -1;
   eptaVlanObj->X_BROADCOM_COM_Vlan8021p = -1;
   eptaVlanObj->TPID = -1;
   eptaVlanObj->enable = TRUE;

   ret =  mdm_setObject((void **)&eptaVlanObj, &pathDesc.iidStack, FALSE);
   mdm_freeObject((void **) &eptaVlanObj);

   if (ret  != CMSRET_SUCCESS)
   {
        cmsLog_error("Failed to set eptaVlanObj. ret=%d", ret);
        return ret;
   }

   /* Get the Ethernet VlanTermination object full path string to be used as ip interface lower layer */
   if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathStringPtr)) != CMSRET_SUCCESS)
   {
        cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
        return ret;
   }
   if (cmsUtl_strlen(fullPathStringPtr) > (SINT32) bufLen)
   {
        ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
        cmsUtl_strncpy(myPathRef, fullPathStringPtr, bufLen);
        ret = CMSRET_SUCCESS;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathStringPtr);

   cmsLog_debug("Exit, ret=%d", ret);

   return ret;
}

static CmsRet addEptaIpIntfObject(const char *lowerLayer, MdmPathDescriptor *ipIntfPathDesc)
{
    Dev2IpInterfaceObject *ipIntf = NULL;
    CmsRet ret;

    INIT_PATH_DESCRIPTOR(ipIntfPathDesc);
    ipIntfPathDesc->oid = MDMOID_DEV2_IP_INTERFACE;

    cmsLog_debug("Enter: lowerLayer=%s", lowerLayer);

    if ((ret = mdm_addObjectInstance(ipIntfPathDesc, NULL, NULL)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to add IP Instance, ret = %d", ret);
        return ret;
    } 

    if ((ret = mdm_getObject(ipIntfPathDesc->oid, &ipIntfPathDesc->iidStack, (void **) &ipIntf)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get IP interface object, ret = %d", ret);
        mdm_deleteObjectInstance(ipIntfPathDesc, NULL, NULL);
        return ret;
    }

    ipIntf->enable = TRUE;
    CMSMEM_REPLACE_STRING_FLAGS(ipIntf->alias, "epta", mdmLibCtx.allocFlags);
    
    ipIntf->IPv4Enable = FALSE;
    #ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
    ipIntf->IPv6Enable = TRUE;
    #endif
    CMSMEM_REPLACE_STRING_FLAGS(ipIntf->lowerLayers, lowerLayer, mdmLibCtx.allocFlags);
    ipIntf->X_BROADCOM_COM_BridgeService = FALSE;

    #ifdef DMP_X_BROADCOM_COM_IGMP_1
    ipIntf->X_BROADCOM_COM_IGMPEnabled = FALSE;
    ipIntf->X_BROADCOM_COM_IGMP_SOURCEEnabled = FALSE;
    #endif

    #ifdef DMP_X_BROADCOM_COM_MLD_1
    ipIntf->X_BROADCOM_COM_MLDEnabled = FALSE;
    ipIntf->X_BROADCOM_COM_MLD_SOURCEEnabled = FALSE;
    #endif

    ret = mdm_setObject((void **)&ipIntf, &ipIntfPathDesc->iidStack, FALSE);
    mdm_freeObject((void **) &ipIntf);

    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set epta ipIntf. ret=%d", ret);
        return ret;
    } 

    cmsLog_debug("Exit: ret=%d", ret);

    return ret;
}

static CmsRet bindTr69cWithWanService(void)
{
    Dev2ManagementServerObject *mgntSeverObj = NULL;
    E2E_Dev2ManagementServerObject *e2eMgntSeverObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    cmsLog_debug("In");

    /* bind tr69c_1 with EMTA Wan Service */
    if ((ret = mdm_getObject(MDMOID_DEV2_MANAGEMENT_SERVER, &iidStack, (void **) &mgntSeverObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get ManagementServer object, ret=%d", ret);
        return ret;
    }

    CMSMEM_REPLACE_STRING_FLAGS(mgntSeverObj->X_BROADCOM_COM_BoundIfName, EPON_VOICE_WAN_IF_NAME, mdmLibCtx.allocFlags);

    ret = mdm_setObject((void **)&mgntSeverObj, &iidStack, FALSE);
    mdm_freeObject((void **) &mgntSeverObj);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set ManagementServer object. ret=%d", ret);
        return ret;
    }

    /* bind tr69c_2 with EPTA Wan Service */
    INIT_INSTANCE_ID_STACK(&iidStack);
    if ((ret = mdm_getObject(MDMOID_E2_DEV2_MANAGEMENT_SERVER, &iidStack, (void **) &e2eMgntSeverObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get E2E ManagementServer object, ret=%d", ret);
        return ret;
    }

    CMSMEM_REPLACE_STRING_FLAGS(e2eMgntSeverObj->X_BROADCOM_COM_BoundIfName, EPON_EPTA_WAN_IF_NAME, mdmLibCtx.allocFlags);

    ret = mdm_setObject((void **)&e2eMgntSeverObj, &iidStack, FALSE);
    mdm_freeObject((void **) &e2eMgntSeverObj);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set E2E ManagementServer object. ret=%d", ret);
        return ret;
    }

    cmsLog_debug("Exit: ret=%d", ret);
    return ret;
}

static CmsRet enableNtpClient(void)
{
#ifdef DMP_DEVICE2_TIME_1
    Dev2TimeObject *timeObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    cmsLog_debug("In");

    if ((ret = mdm_getObject(MDMOID_DEV2_TIME, &iidStack, (void **) &timeObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get Time object, ret=%d", ret);
        return ret;
    }

    timeObj->enable = TRUE;
    CMSMEM_REPLACE_STRING_FLAGS(timeObj->X_BROADCOM_COM_LocalTimeZoneName, EPON_NTP_DEFAULT_TIMEZONE, mdmLibCtx.allocFlags);

    ret = mdm_setObject((void **)&timeObj, &iidStack, FALSE);
    mdm_freeObject((void **) &timeObj);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set Time object. ret=%d", ret);
        return ret;
    }

    cmsLog_debug("Exit: ret=%d", ret);
    return ret;
#else // DMP_DEVICE2_TIME_1
    return CMSRET_SUCCESS;
#endif
}


#endif // ifdef BRCM_PKTCBL_SUPPORT


static CmsRet addEPONWanService(const char *wanL2IfName)
{
    CmsRet ret = CMSRET_SUCCESS;
    UBOOL8 layer2 = TRUE;
    char *layer2FullPath = NULL;
    char ethLinkPathRef[MDM_SINGLE_FULLPATH_BUFLEN] = { 0 };
    char ethVlanPathRef[MDM_SINGLE_FULLPATH_BUFLEN] = { 0 };
#ifdef BRCM_PKTCBL_SUPPORT
    MdmPathDescriptor ipIntfPathDesc = EMPTY_PATH_DESCRIPTOR;
    char *ipIntfFullPath=NULL;
#endif

    /* First get the fullpath to the layer 2 interface name */
    if ((ret = qdmIntf_intfnameToFullPathLocked(wanL2IfName, layer2, &layer2FullPath)) != CMSRET_SUCCESS)
    {
        cmsLog_error("failed to get fullpath of %s, ret=%d", wanL2IfName, ret);
        return ret;
    }
    cmsLog_debug("wanL2IfName %s ==> L2FullPath %s", wanL2IfName, layer2FullPath);

    /* Create Ethernet.Link object */
    ret = addEthernetLink(layer2FullPath, ethLinkPathRef, sizeof(ethLinkPathRef));
    CMSMEM_FREE_BUF_AND_NULL_PTR(layer2FullPath);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("addEthernetLink failed, ret=%d", ret);
        return ret;      
    }

    /* Create Ethernet.VlanTermination object */
    ret = addEthernetVlanTermination(ethLinkPathRef, ethVlanPathRef, sizeof(ethVlanPathRef));
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("addEthernetVlanTermination failed, ret=%d", ret);
        return ret;      
    }

    /* Create IP interface object */
    ret = addDefaultIpIntfObject(ethVlanPathRef);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("addDefaultIpIntfObject failed. ret=%d", ret);
        return ret;
    }

    /* Add a bridge port and join br0 */
    ret = rutBridge_addFullPathToBridge_dev2(ethVlanPathRef, "br0");

#ifdef BRCM_PKTCBL_SUPPORT
    /* Create Ethernet.VlanTermination object */
    ret = addVoiceVlanTermination(ethLinkPathRef, ethVlanPathRef, sizeof(ethVlanPathRef));
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("addEthernetVlanTermination failed, ret=%d", ret);
        return ret;      
    }

    /* Create Voice IP interface object */
    ret = addVoiceIpIntfObject(ethVlanPathRef, &ipIntfPathDesc);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("addDefaultIpIntfObject failed. ret=%d", ret);
        return ret;
    }

    ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&ipIntfPathDesc, &ipIntfFullPath);
    if (ret != CMSRET_SUCCESS)
    {
         cmsLog_error("Could not convert IP.Intf pathDesc to fullPath,ret=%d", ret);
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
         return ret;
    }
#ifdef DMP_DEVICE2_DHCPV4CLIENT_1
   /* Here just add into MDM */
    mdmInit_addDhcpv4ClientObject_dev2(ipIntfFullPath);
    modifyDhcpv4LeasedtimeOption("3600", ipIntfFullPath);
#endif
#ifdef DMP_DEVICE2_DHCPV6CLIENT_1
    mdm_addDhcpv6ClientObject_dev2(ipIntfFullPath);
#endif

    memset(ethVlanPathRef, 0, sizeof(MDM_SINGLE_FULLPATH_BUFLEN));
    /* Create EPTA Ethernet.VlanTermination object */
    ret = addEptaVlanTermination(ethLinkPathRef, ethVlanPathRef, sizeof(ethVlanPathRef));
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("addEptaEthernetVlanTermination failed, ret=%d", ret);
        return ret;      
    }

    /* Create EPTA IP interface object */
    ret = addEptaIpIntfObject(ethVlanPathRef, &ipIntfPathDesc);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("addEptaIpIntfObject failed. ret=%d", ret);
        return ret;
    }

    ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&ipIntfPathDesc, &ipIntfFullPath);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not convert epta IP.Intf pathDesc to fullPath,ret=%d", ret);
        CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
        return ret;
    }

#ifdef DMP_DEVICE2_DHCPV6CLIENT_1
    mdm_addDhcpv6ClientObject_dev2(ipIntfFullPath);
#endif

   /* Bind tr69c with Wan Service by default */
   ret = bindTr69cWithWanService();
   if (ret != CMSRET_SUCCESS)
   {
       cmsLog_error("bindTr69cWithWanService failed. ret=%d", ret);
       return ret;
   }

   /* Enable ntp client */
   ret = enableNtpClient();
   if (ret != CMSRET_SUCCESS)
   {
       cmsLog_error("enableNtpClient failed. ret=%d", ret);
       return ret;
   }

#endif // ifdef BRCM_PKTCBL_SUPPORT

    return ret;
}
#endif /* EPON_SFU */
#endif

CmsRet mdm_addDefaultOpticalObject(void)
{
    CmsRet ret = CMSRET_SUCCESS;

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
    if (!mdm_isOpticalInterfaceObjectExists(EPON_WAN_IF_NAME))
    {
        if ((ret = mdm_addOpticalInterfaceObject(EPON_WAN_IF_NAME)) != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to add EPON optical interface %s, ret=%d", EPON_WAN_IF_NAME, ret);
            goto Error;
        }
#ifdef EPON_SFU
        if ((ret = addEPONWanService(EPON_WAN_IF_NAME)) != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to add WAN service for %s, ret=%d", EPON_WAN_IF_NAME, ret);
            goto Error;
        }
#endif
    }
#endif

#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
    if (!mdm_isOpticalInterfaceObjectExists(GPON_WAN_IF_NAME))
    {
        if ((ret = mdm_addOpticalInterfaceObject(GPON_WAN_IF_NAME)) != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to add GPON optical interface %s, ret=%d", GPON_WAN_IF_NAME, ret);
            goto Error;
        }
    }
#endif

Error:
    return ret;
}

#endif /* DMP_DEVICE2_OPTICAL_1 */



