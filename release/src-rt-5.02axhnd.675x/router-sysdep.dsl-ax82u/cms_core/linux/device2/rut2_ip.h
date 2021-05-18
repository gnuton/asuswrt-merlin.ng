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

#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_IPINTERFACE_1



/* in rut2_ipservicecfg.c */
void rutIpv4Service_runStateMachine(const char *newStatus,
         const char *currStatus,
         const char *ipIntfFullPath, const char *ifname,
         UBOOL8 isWan, UBOOL8 isBridge);

void rutIpv4Service_start(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan, UBOOL8 isBridge);
void rutIpv4Service_setup(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan, UBOOL8 isBridge);
void rutIpv4Service_teardown(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan, UBOOL8 isBridge);
void rutIpv4Service_stop(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan, UBOOL8 isBridge);
void rutIpv4Service_down(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan, UBOOL8 isBridge);



/* in rut2_ipservicecfg6.c */
void rutIpv6Service_runStateMachine(const char *newStatus,
         const char *currStatus,
         const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan);
void rutIpv6Service_start(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan);
void rutIpv6Service_setup(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan);
void rutIpv6Service_teardown(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan);
void rutIpv6Service_stop(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan);
void rutIpv6Service_down(const char *ipIntfFullPath, const char *ifname, UBOOL8 isWan);

CmsRet rutIpv6Service_launchDhcp6c(const char *ipIntfFullPath, const char *ifname);

/* in rut2_ip.c */
void rutIp_sendStaticAddrConfigToSsk(const char *ifName, UBOOL8 isIPv4,
                                     UBOOL8 isAdd, UBOOL8 isMod, UBOOL8 isDel);
void rutIp_sendIntfStackPropagateMsgToSsk(char *lowLayerFullPath);                                     
UBOOL8 rutIp_isUpstream(const char *fullPath);
CmsRet rutIp_getIfnameFromLowerLayers(const char *fullPath,
                                      char *ifnameBuf, UINT32 bufLen);

void rutIp_configureIpv4Addr(const char *ipIntfName,
                     const char *ipAddress, const char *subnetMask);
void rutIp_unconfigureIpv4Addr(const char *ipIntfName);

CmsRet rutIp_activateIpv4Interface_dev2(const char *ifName);
CmsRet rutIp_deactivateIpv4Interface_dev2(const char *ifName);

CmsRet rutIp_addIpv4AddressObject_dev2(const char *ipIntfFullPath,
                                       const char *ipAddrStr, 
                                       const char *subnetMask,
                                       const char *addressingType);

CmsRet rutIp_addMacFilterObject_dev2(const char *ipIntfFullPath);
CmsRet rutIp_delMacFilterObject_dev2(const char *ipIntfFullPath);

#ifdef DMP_DEVICE2_BRIDGE_1  /* aka SUPPORT_PORT_MAP */
CmsRet rutPMap_configPortMapping_dev2(const UBOOL8 isAdd, const char *ifName, const char *ipIntfFullPath);
void rutPMap_configIntfGrouping(Dev2IpInterfaceObject *newObj, Dev2IpInterfaceObject *currObj, const char *ipIntfFullPath);
#endif

#endif  /* DMP_DEVICE2_IPINTERFACE_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
