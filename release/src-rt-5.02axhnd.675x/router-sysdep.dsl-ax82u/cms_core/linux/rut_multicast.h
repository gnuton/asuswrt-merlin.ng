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
#ifndef __RUT_MULTICAST_H__
#define __RUT_MULTICAST_H__


#define MCPD_CONFIG_FILE     "/var/mcpd.conf"



/** create mcpd config file
 *
 * @param isRestartNeeded (IN/OUT) snooping/proxy enabled
 *
 * @return CmsRet enum.
 *
 */
CmsRet rutMulti_CreateMcpdCfg(void);

/** reload/restart mcpd with protocol Type
 *
 * @param protoType  (IN) ipv4/ipv6/all
 * @return CmsRet enum.
 *
 */
CmsRet rutMulti_reloadMcpdWithType(int protoType);
	
/** reload/restart mcpd
 *
 * @return CmsRet enum.
 *
 */
CmsRet rutMulti_reloadMcpd(void);


/**force mcpd to clean up all objects and flush snooping entries 
 *
 * @return CmsRet enum.
 *
 */
CmsRet rutMulti_resetMcpd(void);


/** General function to deal with changes to the mcastCfg object.
 *  Updates switch setting and sends queuePriority to mcpd.
 *  Updates strict WAN setting and DSCP remark
 *
 * @return CmsRet enum.
 */
CmsRet rutMulti_mcastObjCfg(_McastCfgObject *newObj);




/** Return TRUE if mcast QoS is enabled
 *
 * @return TRUE if mcast QoS is enabled
 */
UBOOL8 rutMulti_isMcastQosEnabled(void);




/** Return true if the curently enabled snooping cfg mode has changed.
 *
 * @param newObj  (IN) new IgmpSnoopingCfgObject.
 * @param currObj (IN) current IgmpSnoopingCfgObject.
 *
 * @return true if the curently enabled snooping cfg mode has changed.
 */
UBOOL8 rutMulti_isIgmpSnoopingCfgChanged(const _IgmpSnoopingCfgObject *newObj,
                                       const _IgmpSnoopingCfgObject *currObj);


/** Return true if the curently enabled snooping cfg mode has changed.
 *
 * @param newObj  (IN) new MldSnoopingCfgObject.
 * @param currObj (IN) current MldSnoopingCfgObject.
 *
 * @return true if the curently enabled snooping cfg mode has changed.
 */
UBOOL8 rutMulti_isMldSnoopingCfgChanged(const _MldSnoopingCfgObject *newObj,
                                       const _MldSnoopingCfgObject *currObj);


/** Actual ACTION function to configure IGMP snooping on the specified bridge.
 *
 */
void rutMulti_configIgmpSnooping(const char *brIntfName,
                                 UINT32 mode, SINT32 lanToLanEnable);


/** Actual ACTION function to configure MLD snooping on the specified bridge.
 *
 */
void rutMulti_configMldSnooping(const char *brIntfName,
                                 UINT32 mode, SINT32 lanToLanEnable);


/** This is an internal helper function for updateIgmpSnooping and
 *  updateMldSnooping.  External callers should not call it.
 *
 * @param snoopFullPath (IN) fullPath to IGMP or MLD snooping object
 */
void rutMulti_updateIgmpMldSnoopingObj(const char *snoopFullPath);


void rutMulti_updateIgmpSnooping(const char *brIntfName);

void rutMulti_updateMldSnooping(const char *brIntfName);

void rutMulti_updateIgmpMldSnooping(const char *brIntfName);



/** This is an internal helper function used by the multicast functions.
 * External callers should not call this.
 */
CmsRet rutMulti_addIntfNameToList(const char *caller, const char *intfName,
                                char *intfNamesBuf, UINT32 intfNamesLen);

/** This is an internal helper function used by
 * rutMulti_updateIgmpSnoopingIntfList and rutMulti_updateMldSnoopingIntfList.
 * External callers should not call this function.
 */
CmsRet rutMulti_getAllSnoopingIntfNames(UBOOL8 isMld,
                                  char *intfNamesBuf, UINT32 intfNamesLen);


void rutMulti_updateIgmpSnoopingIntfList(void);

void rutMulti_updateMldSnoopingIntfList(void);

void rutMulti_updateIgmpMldSnoopingIntfList(void);




/** This is an internal helper function.  External callers should not call it.
 *  Return space separate lists of WAN interface where igmp or mld proxy
 *  is enabled, and igmp or mld multicast source is enabled.
 */
void rutMulti_getAllProxyIntfNames(UBOOL8 isMld,
                        char *proxyIntfNames, UINT32 proxyIntfNamesLen,
                        char *sourceIntfNames, UINT32 sourceIntfNamesLen);

void rutMulti_getAllProxyIntfNames_igd(UBOOL8 isMld,
                        char *proxyIntfNames, UINT32 proxyIntfNamesLen,
                        char *sourceIntfNames, UINT32 sourceIntfNamesLen);

void rutMulti_getAllProxyIntfNames_dev2(UBOOL8 isMld,
                        char *proxyIntfNames, UINT32 proxyIntfNamesLen,
                        char *sourceIntfNames, UINT32 sourceIntfNamesLen);

#if defined(SUPPORT_DM_LEGACY98)
#define rutMulti_getAllProxyIntfNames(a, b, c, d, e)   rutMulti_getAllProxyIntfNames_igd((a), (b), (c), (d), (e))
#elif defined(SUPPORT_DM_HYBRID)
#define rutMulti_getAllProxyIntfNames(a, b, c, d, e)   rutMulti_getAllProxyIntfNames_igd((a), (b), (c), (d), (e))
#elif defined(SUPPORT_DM_PURE181)
#define rutMulti_getAllProxyIntfNames(a, b, c, d, e)   rutMulti_getAllProxyIntfNames_dev2((a), (b), (c), (d), (e))
#elif defined(SUPPORT_DM_DETECT)
#define rutMulti_getAllProxyIntfNames(a, b, c, d, e)   (cmsMdm_isDataModelDevice2() ? \
            rutMulti_getAllProxyIntfNames_dev2((a), (b), (c), (d), (e)) : \
            rutMulti_getAllProxyIntfNames_igd((a), (b), (c), (d), (e)))
#endif



void rutMulti_updateIgmpProxyIntfList(void);

void rutMulti_updateMldProxyIntfList(void);

void rutMulti_updateIgmpMldProxyIntfList(void);

CmsRet rutMulti_processHostCtrlChange(UBOOL8 enable);

UBOOL8 rutMulti_getHostCtrlConfig(void);

CmsRet rutMulti_getIgmpRateLimitOnRgBridge(UINT32 *igmpRate);


#endif /* __RUT_MULTICAST_H__ */
