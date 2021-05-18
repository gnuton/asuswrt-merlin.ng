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
 * rut_dhcp6.h
 *
 *  Created on:  Sep. 2017
 *      Author: Fuguo Xu <fuguo.xu@broadcom.com>
 */


#ifndef __RUT_DHCP6_H__
#define __RUT_DHCP6_H__

#ifdef SUPPORT_IPV6

#include "rut_dhcp_common.h"


/* DHCP option codes (partial list) */
#define DHCP_V6_VDR_SPECIFIC_INFO   17
#define DHCP_V6_VDR_NTP_SERVER      56



#if defined(BRCM_PKTCBL_SUPPORT)

#define DHCP6_OPTION17_SUBOPTION40   40   /* CL_V6OPTION_ACS_SERVER */

#define DHCP6_OPTION56_SUBOPTION1    1   /* NTP_SUBOPTION_SRV_ADDR */
#define DHCP6_OPTION56_SUBOPTION2    2   /* NTP_SUBOPTION_MC_ADDR */
#define DHCP6_OPTION56_SUBOPTION3    3   /* NTP_SUBOPTION_SRV_FQDN */




/** This function will create option 17 data following DHCPv6 spec[CL-CANN-DHCP-Reg].
 *  Note: the function should be called before lauching "dhcp6c"
 *
 * @param wanType  (IN)  PKTCBL wan type(EMTA/EPTA)
 * @param ifName  (IN)  interface name on which dhcp6c is launched.
 *
 * @return CmsRet enum.
 */
CmsRet rutDhcp6_createOption17(PKTCBL_WAN_TYPE wanType, const char *ifName);


/** This function will get ACS URL from option 17 sub-option 40.
 *
 * @param ifName  (IN)  interface name on which dhcp6c is launched.
 * @param acsURL  (OUT)  ACS URL.
 * @param inLen  (IN)  ACS URL buffer len.
 *
 * @return CmsRet enum.
 */
CmsRet rutDhcp6_getAcsUrlFromOption17(const char *ifName, char *acsURL, int inLen);


/** This function will get Ntp Servers from option 56.
 *
 * @param ifName  (IN)  interface name on which dhcp6c is launched.
 * @param ntpServerList  (OUT)  Ntp Server list.
 * @param inLen  (IN)  Ntp Server list buffer len.
 *
 * @return CmsRet enum.
 */
CmsRet rutDhcp6_getNtpserversFromOption56(const char *ifName, char *ntpServerList, int inLen);

#endif // BRCM_PKTCBL_SUPPORT


#endif // SUPPORT_IPV6

#endif // __RUT_DHCP6_H__

