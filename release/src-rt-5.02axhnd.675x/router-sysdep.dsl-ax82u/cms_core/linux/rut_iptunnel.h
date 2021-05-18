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
#ifndef __RUT_IPTUNNEL_H__
#define __RUT_IPTUNNEL_H__

/*!\file rut_iptunnel.h
 * \brief System level interface functions for IPv6 tunnel functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */

#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"


/** Start the tunnels associated with the WAN interface
 * 
 * @param ifName   (IN) Name of WAN interface
 * @param ifName   (IN) mode
 * @param activate  (IN) activate or deactivate
 * 
 * @return CmsRet enum.
 */
void rutTunnel_control( const char *ifName, const char *mode, UBOOL8 activate );


/** Check if any dynamic tunnel associated with the WAN interface
 * 
 * @param ifName   (IN) Name of WAN interface
 * @param is6in4   (IN) Tunnel type
 * 
 * @return UBOOL8
 */
UBOOL8 rutTunnel_containDynamicTunnel( const char *ifName, UBOOL8 is6in4 );


#ifdef DMP_X_BROADCOM_COM_IPV6_1
/** Check if DS-Lite tunnel should be triggered by LAN address
 * 
 */
UBOOL8 rutTunnel_activateByLanAddr( const char *ifName );


/** For DS-Lite address config, get address from br0 for unnumbered model
 * 
 */
void rutTunnel_getLanAddrForDsLite( const char *wanIntf, char *ipaddr );

#endif

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
void rutTunnel_getLanAddrForDsLite_dev2( const char *wanIntf, char *ipaddr );
#ifdef SUPPORT_MAP
void rutMap_unbindUpstreamL3Interface( const char *wanIntf );
#endif
#endif

#ifdef SUPPORT_IPV6
/** Get the info of the associated WAN interface.
 * 
 * @param wanIntf (IN) Name of the WAN interface
 * @param ipaddr   (OUT) The IP address.
 * @param firewall   (OUT) if firewall is on.
 * @param isIPv4   (IN) Indicate the version of IP address
 * 
 * @return CmsRet enum.
 */
void rutTunnel_getAssociatedWanInfo( const char *wanIntf, char *ipaddr, UBOOL8 *firewall, UBOOL8 isIPv4 );


/** Execute the configuration of a 4in6 tunnel.
 * 
 */
CmsRet rutTunnel_4in6Config( const char *wanIp, const char *remoteIp, UBOOL8 add );


/** Execute the configuration of a 6rd tunnel.
 * 
 */
CmsRet rutTunnel_6rdConfig( const char *wanIp, const char *prefix, const char *brAddr, SINT32 ipv4MaskLen,
                            const char *tunnelName, const char *lanIntf, UBOOL8 add );


#endif

#endif// __RUT_WAN6_H__

