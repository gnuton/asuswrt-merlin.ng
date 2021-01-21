/***********************************************************************
 *
 *  Copyright (c) 2017  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2017:proprietary:standard

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

/*
 * rut_dhcp.h
 *
 *  Created on:  Sep. 2017
 *      Author: Fuguo Xu <fuguo.xu@broadcom.com>
 */


#ifndef __RUT_DHCP_H__
#define __RUT_DHCP_H__

#include "rut_dhcp_common.h"


/* -------------------------------- define A begin   -------------------------------- */

/* Note: the define A should be consistent with the one in files: 
** userspace/gpl/apps/udhcp/x.h
** For private/gpls licens concern, cannot include same file directly.
*/


/* DHCP option codes (partial list) */
#define DHCP_VDR_SPECIFIC_INFO   0x2b    /* 43 */
#define DHCP_VDR                 0x3c    /* 60 */
#define DHCP_VDR_VI_VENDOR       0x7d    /* 125 */
#define DHCP_VDR_NTP_SERVERS     0x2a    /* 42 */


/* -------------------------------- define A  end  -------------------------------- */



#if defined(BRCM_PKTCBL_SUPPORT)

#define DHCP4_OPTION125_SUBOPTION6   6   /* CL_V4OPTION_ACS_SERVER */


/** This function will create option 43 data following DHCP spec.
 *  Note: the function should be called before lauching "dhcpc"
 *
 * @param wanType  (IN)  PKTCBL wan type(EMTA/EPTA)
 * @param ifName  (IN)  interface name on which dhcpc is launched.
 * @param duid  (IN)  DHCP Unique Identifier (DUID),  refer to: (RFC 3315)
 *
 * @return CmsRet enum.
 */
CmsRet rutDhcp_createOption43(PKTCBL_WAN_TYPE wanType, const char *ifName, const char *duid);


/** This function will create option 60 data following DHCP spec.
 *  Note: the function should be called before lauching "dhcpc"
 *
 * @param ifName  (IN)  interface name on which dhcpc is launched.
 *
 * @return CmsRet enum.
 */
CmsRet rutDhcp_createOption60(const char *ifName);


/** This function will create option 125 data following DHCP spec.
 *  Note: the function should be called before lauching "dhcpc"
 *
 * @param ifName  (IN)  interface name on which dhcpc is launched.
 *
 * @return CmsRet enum.
 */
CmsRet rutDhcp_createOption125(const char *ifName);


/** This function will get ACS URL from option 125 sub-option 6.
 *
 * @param ifName  (IN)  interface name on which dhcpc is launched.
 * @param acsURL  (OUT)  ACS URL.
 * @param inLen  (IN)  ACS URL buffer len.
 *
 * @return CmsRet enum.
 */
CmsRet rutDhcp_getAcsUrlFromOption125(const char *ifName, char *acsURL, int inLen);


/** This function will get Ntp Servers from option 42.
 *
 * @param ifName  (IN)  interface name on which dhcpc is launched.
 * @param ntpServerList  (OUT)  Ntp Server list.
 * @param inLen  (IN)  Ntp Server list buffer len.
 *
 * @return CmsRet enum.
 */
CmsRet rutDhcp_getNtpserversFromOption42(const char *ifName, char *ntpServerList, int inLen);

#endif // BRCM_PKTCBL_SUPPORT

#endif // __RUT_DHCP_H__

