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

#ifdef DMP_DEVICE2_BRIDGE_1

#include "odl.h"
#include "cms_core.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_strconv2.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut2_ipv6.h"
#include "rut2_util.h"
#include "rut2_bridging.h"
#include "beep_networking.h"
#include "qdm_route.h"



/*!\file rcl2_bridging.c
 * \brief This file contains Device.Bridging. objects.
 * This file currently contains the vlan objects, but they can be moved
 * out to another file if we want more separation of the code.
 *
 */
CmsRet rcl_dev2BridgingObject( _Dev2BridgingObject *newObj __attribute__((unused)),
                     const _Dev2BridgingObject *currObj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)),
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2BridgeObject( _Dev2BridgeObject *newObj,
                     const _Dev2BridgeObject *currObj,
                     const InstanceIdStack *iidStack,
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;

    if (ADD_NEW(newObj, currObj))
    {
        InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
        Dev2BridgingObject *bridgingObj = NULL;

        ret = cmsObj_get(MDMOID_DEV2_BRIDGING, &brIidStack, OGF_NO_VALUE_UPDATE,
                              (void **)&bridgingObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("cmsObj_get failed. ret=%d MDMOID_DEV2_BRIDGING", ret);
            return ret;
        }

        if (bridgingObj->bridgeNumberOfEntries == bridgingObj->maxBridgeEntries)
        {
            cmsLog_error("Number of bridges exceeds maximum %d", bridgingObj->maxBridgeEntries);
            cmsObj_free((void **)&bridgingObj);
            return CMSRET_RESOURCE_EXCEEDED;
        }

#ifdef DMP_DEVICE2_VLANBRIDGE_1
        if (bridgingObj->bridgeNumberOfEntries == bridgingObj->maxQBridgeEntries)
        {
            cmsLog_error("Number of bridges exceeds maximum %d", bridgingObj->maxQBridgeEntries);
            cmsObj_free((void **)&bridgingObj);
            return CMSRET_RESOURCE_EXCEEDED;
        }
#endif
        cmsObj_free((void **)&bridgingObj);
 
        rutUtil_modifyNumBridge(iidStack, 1);
    }

    if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
    {
        UBOOL8 enableBridge=TRUE;
        SINT32 bridgeNum=-1;
        char ipAddr[CMS_IPADDR_LENGTH];
        char ipMask[CMS_IPADDR_LENGTH];
        char bCast[CMS_IPADDR_LENGTH];
        Dev2IpInterfaceObject *ipIntfObj=NULL;
        InstanceIdStack intfIidStack=EMPTY_INSTANCE_ID_STACK;
        UBOOL8 foundBrIpIntf=FALSE; 

        CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);

        sprintf(ipAddr, "0.0.0.0");
        sprintf(ipMask, "0.0.0.0");
        sprintf(bCast, "0.0.0.0");

        if (newObj != NULL && currObj == NULL && IS_EMPTY_STRING(newObj->X_BROADCOM_COM_IfName))
        {
            cmsLog_error("Bridge does not have a name... generate one");
            /* complain, but keep going */
        }

        if (IS_EMPTY_STRING(newObj->X_BROADCOM_COM_IfName))
        {
            char brIntfNameBuf[CMS_IFNAME_LENGTH]={0};

            if ((bridgeNum = rutLan_getNextAvailableBridgeNumber()) < 0)
            {
                return CMSRET_RESOURCE_EXCEEDED;
            }

            sprintf(brIntfNameBuf, "br%d", bridgeNum);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_IfName, brIntfNameBuf, mdmLibCtx.allocFlags);
        }
        else if (currObj != NULL)
        {
            /* Enable existing bridge. */

            /* Find bridge ip interface. */
            while (cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, 
                                                &intfIidStack,
                                                OGF_NO_VALUE_UPDATE, 
                                                (void **) &ipIntfObj) == CMSRET_SUCCESS)
            {
                if (!cmsUtl_strcmp(ipIntfObj->name, newObj->X_BROADCOM_COM_IfName))
                {
                    /* need this obj, so break now and free it later */
                    foundBrIpIntf = TRUE;
                    break;
                }
                cmsObj_free((void **) &ipIntfObj);
            }

            if (!foundBrIpIntf)
            {
                cmsLog_error("Could not find IP.Interface for %s", newObj->X_BROADCOM_COM_IfName);
            }
            else
            {
                /* Restore bridge interface ipv4 address */
                if (!cmsUtl_strcmp(ipIntfObj->status, MDMVS_UP))
                {
                    Dev2Ipv4AddressObject *ipv4AddrObj=NULL;
                    InstanceIdStack addrIidStack=EMPTY_INSTANCE_ID_STACK;

                    if (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV4_ADDRESS,
                                                                &intfIidStack,
                                                                &addrIidStack,
                                                                OGF_NO_VALUE_UPDATE,
                                                                (void **) &ipv4AddrObj) == CMSRET_SUCCESS)
                    {
                        if (!cmsUtl_strcmp(ipv4AddrObj->status, MDMVS_ENABLED) &&
                             !cmsUtl_strcmp(ipv4AddrObj->addressingType, MDMVS_STATIC))
                        {
                            cmsUtl_strncpy(ipAddr, ipv4AddrObj->IPAddress, sizeof(ipAddr));
                            cmsUtl_strncpy(ipMask, ipv4AddrObj->subnetMask, sizeof(ipMask));
                            rut_getBCastFromIpSubnetMask(ipAddr, ipMask, bCast);
                        }
                        cmsObj_free((void **) &ipv4AddrObj);
                    }
                }
            }
        }

        /*
         * Just enable the bridge here.  IP addrs are enabled from the RCL
         * handler functions for the IPv4 and IPv6 addr objects.
         */
        cmsLog_debug("enable linux bridge %s ip=%s mask=%s bcast=%s",
                         newObj->X_BROADCOM_COM_IfName, ipAddr, ipMask, bCast);
        rutLan_enableBridge(newObj->X_BROADCOM_COM_IfName, enableBridge,
                                  ipAddr, ipMask, bCast);

        if (foundBrIpIntf)
        {
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1 /* aka SUPPORT_IPV6 */
            /* Restore bridge interface ipv6 address. */
            if (!cmsUtl_strcmp(ipIntfObj->status, MDMVS_UP))
            {
                Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
                InstanceIdStack addrIidStack=EMPTY_INSTANCE_ID_STACK;

                if (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_ADDRESS,
                                                            &intfIidStack,
                                                            &addrIidStack,
                                                            OGF_NO_VALUE_UPDATE,
                                                            (void **) &ipv6AddrObj) == CMSRET_SUCCESS)
                {
                    if (!cmsUtl_strcmp(ipv6AddrObj->status, MDMVS_ENABLED) &&
                         !cmsUtl_strcmp(ipv6AddrObj->origin, MDMVS_STATIC))
                    {
                        char prefix[CMS_IPADDR_LENGTH];

                        if (qdmIpv6_fullPathToPefixLocked_dev2(ipv6AddrObj->prefix, prefix) != CMSRET_SUCCESS)
                        {
                            cmsLog_error("cannot get prefix from %s", ipv6AddrObj->prefix);
                        }
                        else
                        {
                            rutIp_configureIpv6Addr(ipIntfObj->name, ipv6AddrObj->IPAddress, prefix);
                        }
                    }
                    cmsObj_free((void **) &ipv6AddrObj);
                }
            }
#endif
            /* free ipIntfObj */
            cmsObj_free((void **) &ipIntfObj);
        }

        if (currObj != NULL)
        {
            /* Enable existing bridge */

            Dev2BridgePortObject *brPortObj=NULL;
            InstanceIdStack brPortIidStack=EMPTY_INSTANCE_ID_STACK;
            UBOOL8 isUpstream;

            /* Add all the non-management ports to bridge. */
            while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT,
                                                            iidStack,
                                                            &brPortIidStack,
                                                            OGF_NO_VALUE_UPDATE,
                                                            (void **) &brPortObj) == CMSRET_SUCCESS)
            {
                if (!brPortObj->managementPort &&
                     !cmsUtl_strcmp(brPortObj->status, MDMVS_UP))
                {
                    char l2IntfName[BUFLEN_32];
                    char *ptr;

                    cmsLog_debug("bridge port %s is UP! add to %s",
                                     brPortObj->name, newObj->X_BROADCOM_COM_IfName);

                    strncpy(l2IntfName, brPortObj->name, sizeof(l2IntfName));
                    ptr = strchr(l2IntfName, '.');
                    if (ptr != NULL)
                        *ptr = '\0';

                    isUpstream = qdmIntf_isLayer2IntfNameUpstreamLocked_dev2(l2IntfName);
            
                    rutLan_addInterfaceToBridge(brPortObj->name,
                                                         isUpstream,
                                                         newObj->X_BROADCOM_COM_IfName);
                    rutMulti_updateIgmpMldProxyIntfList();
                }
                cmsObj_free((void **) &brPortObj);
            }
        }

        /* 
         * BEEP requires interface group to create separated bridge
         * and need to add firewall rules and NAT rules
         */
        if (newObj->X_BROADCOM_COM_Mode == INTFGRP_BR_BEEP_SECONDARY_MODE)
        {
             char activeGwIfName[CMS_IFNAME_LENGTH]={0};
             UBOOL8 isIPv4 = TRUE;

             /* firewall rules similar to guest wifi */
             rutIpt_beepNetworkingSecurity(newObj->X_BROADCOM_COM_IfName, INTFGRP_BR_BEEP_SECONDARY_MODE);

             qdmRt_getActiveDefaultGatewayLocked(activeGwIfName);
             if (!IS_EMPTY_STRING(activeGwIfName))
             {
                 if (qdmIpIntf_isWanInterfaceUpLocked(activeGwIfName, isIPv4) &&
                      qdmIpIntf_isNatEnabledOnIntfNameLocked(activeGwIfName))
                 {
                      rutIpt_beepNetworkingMasqueurade(newObj->X_BROADCOM_COM_IfName, activeGwIfName);
                 }
             }
        }
        else if (newObj->X_BROADCOM_COM_Mode == INTFGRP_BR_BEEP_WANONLY_MODE)
        {
             char activeGwIfName[CMS_IFNAME_LENGTH]={0};
             UBOOL8 isIPv4 = TRUE;

             /* firewall rules to only allow WAN access */
             rutIpt_beepNetworkingSecurity(newObj->X_BROADCOM_COM_IfName, INTFGRP_BR_BEEP_WANONLY_MODE);

             qdmRt_getActiveDefaultGatewayLocked(activeGwIfName);
             if (!IS_EMPTY_STRING(activeGwIfName))
             {
                 if (qdmIpIntf_isWanInterfaceUpLocked(activeGwIfName, isIPv4) &&
                      qdmIpIntf_isNatEnabledOnIntfNameLocked(activeGwIfName))
                 {
                      rutIpt_beepNetworkingMasqueurade(newObj->X_BROADCOM_COM_IfName, activeGwIfName);
                 }
             }
        }
    }


    if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
    {
        cmsLog_debug("disable linux bridge %s", currObj->X_BROADCOM_COM_IfName);
        rutLan_disableBridge(currObj->X_BROADCOM_COM_IfName);

        if (DELETE_EXISTING(newObj, currObj))
        {
            rutUtil_modifyNumBridge(iidStack, -1);
        }
        else
        {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
        }
    }

    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2BridgePortObject( _Dev2BridgePortObject *newObj,
                                          const _Dev2BridgePortObject *currObj,
                                          const InstanceIdStack *iidStack,
                                          char **errorParam __attribute__((unused)),
                                          CmsRet *errorCode __attribute__((unused)))
{
    char brIntfNameBuf[CMS_IFNAME_LENGTH]={0};
    UBOOL8 statusChanged = FALSE;
    CmsRet ret;

    cmsLog_debug("Entered:");

    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumBridgePort(iidStack, 1);
    }

#ifdef DMP_DEVICE2_VLANBRIDGE_1
    /* We don't allow a bridge port be configured with both VLANTermination and
     * VLANBridge objects.
     */
    if (newObj && !newObj->managementPort &&
         cmsUtl_strcasestr(newObj->lowerLayers, "VLANTermination"))
    {
        MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
        InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
        InstanceIdStack brVlanPortIidStack = EMPTY_INSTANCE_ID_STACK;
        Dev2BridgeVlanPortObject *brVlanPortObj = NULL;
        char *brPortFullPath = NULL;
        UBOOL8 foundVlanPort = FALSE;

        /* This port is to be configured on top of VLANTermination object.
         * We want to check that it had not been configured with VLANBridge object.
         */
        pathDesc.oid = MDMOID_DEV2_BRIDGE_PORT;
        pathDesc.iidStack = *iidStack;
        ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &brPortFullPath);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("pathDescriptorToFullPathNoEndDot failed. ret=%d DEV2_BRIDGE_PORT iidStack=%s",
                             ret, cmsMdm_dumpIidStack(iidStack));
            return ret;
        }

        /* The parent Bridge object is 1 level above the port object. */
        brIidStack = *iidStack;
        POP_INSTANCE_ID(&brIidStack);

        /* find the associated vlan port */
        while (!foundVlanPort &&
                 (ret = cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_VLAN_PORT,
                                             &brIidStack,
                                             &brVlanPortIidStack,
                                             OGF_NO_VALUE_UPDATE,
                                             (void **)&brVlanPortObj)) == CMSRET_SUCCESS)
        {
            if (!cmsUtl_strcmp(brVlanPortObj->port, brPortFullPath))
            {
                foundVlanPort = TRUE;
            }
            cmsObj_free((void **)&brVlanPortObj);
        }
        CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);

        if (foundVlanPort)
        {
            cmsLog_error("bridge port had been configured with VLANBridge object.");
            return CMSRET_INVALID_ARGUMENTS;
        }
        if (ret == CMSRET_NO_MORE_INSTANCES) ret = CMSRET_SUCCESS;
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("cmsObj_getNextInSubTreeFlags failed. ret=%d DEV2_BRIDGE_VLAN_PORT iidStack=%s",
                             ret, cmsMdm_dumpIidStack(&brVlanPortIidStack));
            return ret;
        }
    }
#endif

    /*
     * Always force management Port status to UP.  This allows brx which sits
     * on top of Management Port to always be UP.
     */
    if (newObj && newObj->managementPort && cmsUtl_strcmp(newObj->status, MDMVS_UP))
    {
        cmsLog_debug("force mgmtPort %s to status UP", cmsMdm_dumpIidStack(iidStack));
        CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_UP, mdmLibCtx.allocFlags);
        newObj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
    }

    IF_STATUS_HAS_CHANGED_SET_LASTCHANGE(newObj, currObj);

    /*
     * First figure out what my Linux bridge intf name is.
     */
    ret = rutBridge_getParentBridgeIntfName_dev2(iidStack, brIntfNameBuf);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not get parent bridge intfName, newObj=%p currObj=%p iidStack=%s",
                         newObj, currObj, cmsMdm_dumpIidStack(iidStack));
        return ret;
    }


    /*
     * If we are not in a delete situation (newObj != NULL) and
     * we don't have a Linux interface name yet, try to figure it out.
     * (a) if this is a management port, ifName is in parent bridge.
     * (b) if this is not a management port, go down on LowerLayers
     */
    if ((newObj != NULL) && IS_EMPTY_STRING(newObj->name))
    {
        if (newObj->managementPort)
        {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->name, brIntfNameBuf, mdmLibCtx.allocFlags);
        }
        else if (!IS_EMPTY_STRING(newObj->X_BROADCOM_COM_Name))
        {
            /* DAL has provided a port name. Just use it. This is a use case for
             * interface grouping where the port name shall be maintained when
             * the port is moving from the old bridge to the new bridge.
             * Note that this parameter is invisible to ACS.
             */
            CMSMEM_REPLACE_STRING_FLAGS(newObj->name, newObj->X_BROADCOM_COM_Name, mdmLibCtx.allocFlags);

            /* X_BROADCOM_COM_Name is no longer needed.  Free it. */
            CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->X_BROADCOM_COM_Name);
        }
        else if (!IS_EMPTY_STRING(newObj->lowerLayers))
        {
            char lowerIntfName[CMS_IFNAME_LENGTH]={0};

            if ((ret = qdmIntf_fullPathToIntfnameLocked(newObj->lowerLayers, lowerIntfName)) != CMSRET_SUCCESS)
            {
                cmsLog_error("qdmIntf_getIntfnameFromFullPathLocked failed. ret %d", ret);
                return ret;
            }

#ifdef DMP_DEVICE2_VLANBRIDGE_1
            /* If this port is to be on top of a VLANTermination object,
             * then get the interface name from the VLANTermination object.
             * Otherwise, create a virtual interface name using the next
             * available virtual interface index.
             */
            if (cmsUtl_strcasestr(newObj->lowerLayers, "VLANTermination") == NULL &&
                 cmsUtl_strncmp(lowerIntfName, WLAN_IFC_STR, strlen(WLAN_IFC_STR)))
            {
                SINT32 vlanIndex = 0;

                if (rutUtil_getAvailVlanIndex_dev2(lowerIntfName, &vlanIndex) == CMSRET_SUCCESS)
                {
                    char vlanIfname[CMS_IFNAME_LENGTH]={0};

                    strcpy(vlanIfname, lowerIntfName);
                    snprintf(lowerIntfName, sizeof(lowerIntfName), "%s.%d", vlanIfname, vlanIndex);
                }
            }
#endif

            CMSMEM_REPLACE_STRING_FLAGS(newObj->name, lowerIntfName, mdmLibCtx.allocFlags);
        }

        cmsLog_debug("assigned port (mgmt=%d) name %s",
                         newObj->managementPort, newObj->name);
    }

    if (newObj && currObj && !newObj->managementPort)
    {
        if (newObj->enable)
        {
            if (IS_EMPTY_STRING(newObj->name) || IS_EMPTY_STRING(newObj->lowerLayers))
            {
                cmsLog_error("Cannot enable port without name and lowerLayers.");
                return CMSRET_INVALID_ARGUMENTS;
            }

            if (qdmIntf_isStatusUpOnFullPathLocked_dev2(newObj->lowerLayers))
            {
                CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_UP, mdmLibCtx.allocFlags);
            }
            else
            {
                CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
            }
        }
        else
        {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
        }

        if (cmsUtl_strcmp(newObj->status, currObj->status))
        {
            newObj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
            statusChanged = TRUE;
        }

#if defined(EPON_SFU) && defined(BRCM_PKTCBL_SUPPORT)
        if (strncmp(newObj->name, ETH_IFC_STR, strlen(ETH_IFC_STR)) == 0)
        {
            UBOOL8 isUpstream;
            char l2IntfName[BUFLEN_32];
            char *ptr;

            cmsLog_debug("bridge port %s is UP! add to %s",
                                 newObj->name, brIntfNameBuf);
                
            strncpy(l2IntfName, newObj->name, sizeof(l2IntfName));
            ptr = strchr(l2IntfName, '.');
            if (ptr != NULL)
                *ptr = '\0';

            isUpstream = qdmIntf_isLayer2IntfNameUpstreamLocked_dev2(l2IntfName);

            rutLan_addInterfaceToBridge(newObj->name, isUpstream, brIntfNameBuf);
            rutMulti_updateIgmpMldProxyIntfList();
            statusChanged = FALSE;
        }
#endif

#ifdef DMP_DEVICE2_VLANBRIDGE_1
        if (statusChanged ||
             newObj->defaultUserPriority != currObj->defaultUserPriority ||
             newObj->TPID != currObj->TPID ||
             newObj->priorityTagging != currObj->priorityTagging)
#else
        if (statusChanged)
#endif
        {
            if (!cmsUtl_strcmp(newObj->status, MDMVS_UP))
            {
                UBOOL8 isUpstream;
                char l2IntfName[BUFLEN_32];
                char *ptr;

                cmsLog_debug("bridge port %s is UP! add to %s",
                                 newObj->name, brIntfNameBuf);

#ifdef DMP_DEVICE2_VLANBRIDGE_1
#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
                /* Port status changed to UP, we want to configure vlan rules for
                 * the interface provided that the interface:
                 * - is a virtual interface. Per design, VLAN is only supported on
                 *    virtual interface with dotted name.
                 * - is not on top of a VLANTermination object where vlan rules
                 *    had already been configured.
                 * - is not a member of another bridge. If the port is a member
                 *    of another bridge, no need to re-configure vlan rules for it.
                 *    This is likely the case when DAL is moving the port from its
                 *    old bridge to this bridge for interface grouping.
                 */
                if (strchr(newObj->name, '.') != NULL &&
                    cmsUtl_strcasestr(newObj->lowerLayers, "VLANTermination") == NULL &&
                    !rutBridge_isPortInOtherBridge(brIntfNameBuf, newObj->name))
                {
                    char l2Ifname[CMS_IFNAME_LENGTH]={0};

                    ret = qdmIntf_fullPathToIntfnameLocked_dev2(newObj->lowerLayers, l2Ifname);
                    if (ret != CMSRET_SUCCESS)
                    {
                        cmsLog_error("qdmIntf_fullPathToIntfnameLocked_dev2 failed. ret=%d lowerLayers=%s",
                                     ret, newObj->lowerLayers);
                    }
                    else
                    {
                        UBOOL8 untagged;
                        SINT32 vlanId;

                        ret = rutBridge_getVlanPortInfo_dev2(iidStack, &untagged, &vlanId);
                        if (ret != CMSRET_SUCCESS)
                        {
                            cmsLog_error("rutBridge_getVlanPortInfo_dev2 failed. ret=%d", ret);
                        }
                        else
                        {
                            ret = rutBridge_configVlanRules_dev2(newObj->name,
                                                                 l2Ifname,
                                                                 newObj->TPID,
                                                                 newObj->priorityTagging,
                                                                 newObj->defaultUserPriority,
                                                                 untagged,
                                                                 vlanId);
                            if (ret != CMSRET_SUCCESS)
                            {
                                cmsLog_error("rutBridge_configVlanRules_dev2 failed. ret=%d", ret);
                            }
                        }
                    }
                }
#endif            
#endif            
                strncpy(l2IntfName, newObj->name, sizeof(l2IntfName));
                ptr = strchr(l2IntfName, '.');
                if (ptr != NULL)
                    *ptr = '\0';

                isUpstream = qdmIntf_isLayer2IntfNameUpstreamLocked_dev2(l2IntfName);

                rutLan_addInterfaceToBridge(newObj->name, isUpstream, brIntfNameBuf);
                rutMulti_updateIgmpMldProxyIntfList();
            }
        }

        if (statusChanged && !cmsUtl_strcmp(currObj->status, MDMVS_UP))
        {
            /* remove from bridge only when going from UP to any non-UP state */
            /* don't care if going from non-UP to non-UP state */
            cmsLog_debug("bridge port %s is %s, remove from %s",
                         newObj->name, newObj->status, brIntfNameBuf);
            rutLan_removeInterfaceFromBridge(newObj->name, brIntfNameBuf);
            rutMulti_updateIgmpMldProxyIntfList();

#ifdef DMP_DEVICE2_VLANBRIDGE_1
#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
            if (!cmsUtl_strcmp(newObj->status, MDMVS_LOWERLAYERDOWN))
            { 
                /* Port status changed to MDMVS_LOWERLAYERDOWN, we want to delete
                 * the port interface provided that the interface:
                 * - is a virtual interface with the exception of default LANVLAN,
                 *    e.g. eth1.0, eth2.0, which shall not be deleted.
                 * - is not a wlan. e.g. wl0.1, wl0.2
                 * - is not a member of another bridge.
                 */
                if (strchr(newObj->name, '.') != NULL &&
                    cmsUtl_strncmp(newObj->name, WLAN_IFC_STR, strlen(WLAN_IFC_STR)) &&
                    (cmsUtl_strncmp(newObj->name, ETH_IFC_STR, strlen(ETH_IFC_STR)) ||
                     cmsUtl_strstr(newObj->name, ".0") == NULL) &&
                    rut_wanGetIntfIndex(newObj->name) > 0 &&
                    !rutBridge_isPortInOtherBridge(brIntfNameBuf, newObj->name))
                {
        	           /* Delete the virtual interface.
                     * All rules associated with it will be purged.
                     */
                    vlanCtl_init();
                    vlanCtl_deleteVlanInterface(newObj->name);
                    vlanCtl_cleanup();
                }
            }
#endif
#endif
        }
    }

    if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumBridgePort(iidStack, -1);

        if (!currObj->managementPort)
        {
#if defined(EPON_SFU) && defined(BRCM_PKTCBL_SUPPORT)
            if (strncmp(newObj->name, ETH_IFC_STR, strlen(ETH_IFC_STR)) == 0)
            {
                cmsLog_debug("bridge port %s is being deleted, remove from %s",
                             currObj->name, brIntfNameBuf);
                rutLan_removeInterfaceFromBridge(currObj->name, brIntfNameBuf);
            }
            else
            {
#endif
                if (!cmsUtl_strcmp(currObj->status, MDMVS_UP))
                {
                    cmsLog_debug("bridge port %s is being deleted, remove from %s",
                                     currObj->name, brIntfNameBuf);
                    rutLan_removeInterfaceFromBridge(currObj->name, brIntfNameBuf);

                }
#if defined(EPON_SFU) && defined(BRCM_PKTCBL_SUPPORT)
            }
#endif

#ifdef DMP_DEVICE2_VLANBRIDGE_1
            /* Need to modify the vlan port table */
            ret = rutBridge_deRefPortFromVlanPort_dev2(iidStack);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("rutBridge_deRefPortFromVlanPort_dev2 failed. ret=%d", ret);
            }

#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
            /* We need to delete the port interface provided that the interface:
             * - is a virtual interface with the exception of default LANVLAN,
             *    e.g. eth1.0, eth2.0, which shall not be deleted.
             * - is not a wlan. e.g. wl0.1, wl0.2
             * - is not a member of another bridge.
             */
            if (strchr(currObj->name, '.') != NULL &&
                cmsUtl_strncmp(currObj->name, WLAN_IFC_STR, strlen(WLAN_IFC_STR)) &&
                (cmsUtl_strncmp(currObj->name, ETH_IFC_STR, strlen(ETH_IFC_STR)) ||
                 cmsUtl_strstr(currObj->name, ".0") == NULL) &&
                rut_wanGetIntfIndex(currObj->name) > 0 &&
                !rutBridge_isPortInOtherBridge(brIntfNameBuf, currObj->name))
            {
        	       /* Delete the virtual interface.
                 * All rules associated with it will be purged.
                 */
                vlanCtl_init();
                vlanCtl_deleteVlanInterface(currObj->name);
                vlanCtl_cleanup();
            }
#endif
#endif
        }
    }


    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2BridgePortStatsObject( _Dev2BridgePortStatsObject *newObj __attribute__((unused)),
                     const _Dev2BridgePortStatsObject *currObj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)),
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


#ifdef DMP_DEVICE2_VLANBRIDGE_1
CmsRet rcl_dev2BridgeVlanObject( _Dev2BridgeVlanObject *newObj,
                     const _Dev2BridgeVlanObject *currObj,
                     const InstanceIdStack *iidStack,
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;

    if (ADD_NEW(newObj, currObj))
    {
        InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
        InstanceIdStack ancestorIidStack = *iidStack;
        Dev2BridgingObject *bridgingObj = NULL;
        Dev2BridgeObject *brObj = NULL;

        ret = cmsObj_get(MDMOID_DEV2_BRIDGING, &brIidStack, OGF_NO_VALUE_UPDATE,
                              (void **)&bridgingObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("cmsObj_get failed. ret=%d DEV2_BRIDGING", ret);
            return ret;
        }

        ret = cmsObj_getAncestorFlags(MDMOID_DEV2_BRIDGE, MDMOID_DEV2_BRIDGE_VLAN,
                                                &ancestorIidStack, OGF_NO_VALUE_UPDATE,
                                                (void **)&brObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("cmsObj_getAncestorFlags failed. ret=%d ancestor=DEV2_BRIDGE decendent=DEV2_BRIDGE_VLAN iidStack=%s",
                             cmsMdm_dumpIidStack(iidStack));
            cmsObj_free((void **)&bridgingObj);
            return ret;
        }

        if (brObj->VLANNumberOfEntries == bridgingObj->maxVLANEntries)
        {
            cmsLog_error("Number of VLANs exceeds maximum %d", bridgingObj->maxVLANEntries);
            cmsObj_free((void **)&bridgingObj);
            cmsObj_free((void **)&brObj);
            return CMSRET_RESOURCE_EXCEEDED;
        }
        cmsObj_free((void **)&bridgingObj);
        cmsObj_free((void **)&brObj);
 
        rutUtil_modifyNumBridgeVlan(iidStack, 1);

        if (newObj->enable)
        {
            ret = rutBridge_configVlan_dev2(iidStack, newObj->VLANID);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("rutBridge_configVlan_dev2 failed. ret=%d", ret);
            }
        }
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumBridgeVlan(iidStack, -1);

        cmsLog_debug("Delete VLAN object. iidStack=%s", cmsMdm_dumpIidStack(iidStack));

        ret = rutBridge_configVlan_dev2(iidStack, -1);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("rutBridge_configVlan_dev2 failed. ret=%d", ret);
            return ret;
        }

        /* Need to modify the vlan port table */
        ret = rutBridge_deRefVlanFromVlanPort_dev2(iidStack);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("rutBridge_deRefVlanFromVlanPort_dev2 failed. ret=%d", ret);
        }
    }
    else
    {
        /* modify existing object */

        if (newObj->enable &&
             (!currObj->enable || (newObj->VLANID != currObj->VLANID)))
        {
            ret = rutBridge_configVlan_dev2(iidStack, newObj->VLANID);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("rutBridge_configVlan_dev2 failed. ret=%d", ret);
            }
        }
        else if (!newObj->enable && currObj->enable)
        {
            ret = rutBridge_configVlan_dev2(iidStack, -1);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("rutBridge_configVlan_dev2 failed. ret=%d", ret);
            }
        }
    }
    
    return ret;
}

CmsRet rcl_dev2BridgeVlanPortObject( _Dev2BridgeVlanPortObject *newObj,
                     const _Dev2BridgeVlanPortObject *currObj,
                     const InstanceIdStack *iidStack,
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    Dev2BridgeVlanObject *brVlanObj = NULL;
    MdmPathDescriptor pathDesc;
    UBOOL8 untagged = TRUE;
    SINT32 vlanId = -1;

    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumBridgeVlanPort(iidStack, 1);

        if (IS_EMPTY_STRING(newObj->VLAN) || IS_EMPTY_STRING(newObj->port))
        {
            newObj->enable = FALSE;
        }
        return ret;
    }

    if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumBridgeVlanPort(iidStack, -1);

        cmsLog_debug("Delete VLAN port object. iidStack=%s bridge port=%s",
                         cmsMdm_dumpIidStack(iidStack), currObj->port);

#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
        /* update bridge port vlanctl rule for either untagged or priority tagging. */
        ret = rutBridge_configPort_dev2(currObj->port, TRUE, -1);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("rutBridge_configPort_dev2 failed. ret=%d", ret);
        }
#endif
    }
    else
    {
        /* modify existing object */

        if (IS_EMPTY_STRING(newObj->VLAN) || IS_EMPTY_STRING(newObj->port))
        {
            newObj->enable = FALSE;
        }

#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
        if (newObj->enable && 
             (!currObj->enable ||
              (newObj->untagged != currObj->untagged) ||
              cmsUtl_strcmp(newObj->VLAN, currObj->VLAN) ||
              cmsUtl_strcmp(newObj->port, currObj->port)))
        {
            if (IS_EMPTY_STRING(newObj->VLAN))
            {
                /* This vlan port is not a member of any vlan. Disable vlan tagging. */
                untagged = TRUE;
                vlanId    = -1;
            }
            else
            {
                INIT_PATH_DESCRIPTOR(&pathDesc);
                ret = cmsMdm_fullPathToPathDescriptor(newObj->VLAN, &pathDesc);
                if (ret != CMSRET_SUCCESS)
                {
                    cmsLog_error("cmsMdm_fullPathToPathDescriptor failed. ret=%d", ret);
                    return ret;
                }
                ret = cmsObj_get(MDMOID_DEV2_BRIDGE_VLAN, &pathDesc.iidStack,
                                      OGF_NO_VALUE_UPDATE, (void **)&brVlanObj);
                if (ret != CMSRET_SUCCESS)
                {
                    cmsLog_error("cmsObj_get failed. MDMOID_DEV2_BRIDGE_VLAN, ret=%d", ret);
                    return ret;
                }

                untagged = newObj->untagged;
                vlanId    = brVlanObj->VLANID;

                cmsObj_free((void **)&brVlanObj);
            }

            ret = rutBridge_configPort_dev2(newObj->port, untagged, vlanId);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("rutBridge_configPort_dev2 failed. ret=%d", ret);
            }
        }
        else if (!newObj->enable && currObj->enable)
        {
            ret = rutBridge_configPort_dev2(currObj->port, TRUE, -1);
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("rutBridge_configPort_dev2 failed. ret=%d", ret);
            }
        }
#endif
    }

    return ret;
}
#endif

CmsRet rcl_dev2BridgeFilterObject( _Dev2BridgeFilterObject *newObj,
                     const _Dev2BridgeFilterObject *currObj,
                     const InstanceIdStack *iidStack,
                     char **errorParam __attribute__((unused)),
                     CmsRet *errorCode __attribute__((unused)))
{

    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumBridgeFilter(iidStack, 1);
    }

    if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumBridgeFilter(iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_BRIDGE_1 */

#endif     /* DMP_DEVICE2_BASELINE_1 */



