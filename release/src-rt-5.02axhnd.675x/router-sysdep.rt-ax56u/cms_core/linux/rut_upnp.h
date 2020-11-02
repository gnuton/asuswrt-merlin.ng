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

#ifndef __RUT_UPNP_H__
#define __RUT_UPNP_H__


/*!\file rut_upnp.h
 * \brief System level interface functions for UPnP functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */



/** Check if upnp is enabled or not
 *
 * @return  TRUE or FALSE
 */
UBOOL8 rut_isUpnpEnabled(void);


/** Stopt upnp process
 *
 * @return CmsRet enum.
 */
CmsRet  rut_stopUpnp(void);

/** ReStart upnp process by stop upnp process if it is in memory and remove all iptables rules 
 *   and start a new upnp process and add iptables rules for all natted wan interface.
 *
 * @deletedIfc       (IN) already deleted Wan interface name and to be excluded
 * @return CmsRet enum.
 */
CmsRet rut_restartUpnp(char *deletedIfc);

/** This function defers with rut_restartUpnp in a way that it is used in changing the default gateway process
 * and since the default gateway hander has not existed, the old default gateway still served as default 
 * gateway for the system. Passing the new default gateway wan interface name to force not to use
 * the old one.
 *
 * @ifcName       (IN) Wan interface name for upnp to attach to.
 * @return CmsRet enum.
 */
CmsRet rut_restartUpnpWithWanIfc(char *ifcName);




/** Configure (add/delete) the upnp iptable rules for all nated wan interfaces
 *
 * @addRules       (IN) BOOLEAN.  TRUE = add rule.  FALSE = delete rule
 *
 */
void rutUpnp_configIptableRuleForAllWanIfcs(UBOOL8 addRules);
void rutUpnp_configIptableRuleForAllWanIfcs_igd(UBOOL8 addRules);
void rutUpnp_configIptableRuleForAllWanIfcs_dev2(UBOOL8 addRules);

#if defined(SUPPORT_DM_LEGACY98)
#define rutUpnp_configIptableRuleForAllWanIfcs(i)   rutUpnp_configIptableRuleForAllWanIfcs_igd((i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutUpnp_configIptableRuleForAllWanIfcs(i)   rutUpnp_configIptableRuleForAllWanIfcs_igd((i))
#elif defined(SUPPORT_DM_PURE181)
#define rutUpnp_configIptableRuleForAllWanIfcs(i)   rutUpnp_configIptableRuleForAllWanIfcs_dev2((i))
#elif defined(SUPPORT_DM_DETECT)
#define rutUpnp_configIptableRuleForAllWanIfcs(i)   (cmsMdm_isDataModelDevice2() ? \
                             rutUpnp_configIptableRuleForAllWanIfcs_dev2((i)) : \
                             rutUpnp_configIptableRuleForAllWanIfcs_igd((i)))
#endif

/* Check Upnp is running with in 1.5 seconds period. */
UBOOL8 rutUpnp_checkRunStatusWithDelay(void);


/* IPv6 related functions */

UBOOL8 rutUpnp_pcpOfLanAddr(const char *ifName);

#endif  /* __RUT_UPNP_H__ */

