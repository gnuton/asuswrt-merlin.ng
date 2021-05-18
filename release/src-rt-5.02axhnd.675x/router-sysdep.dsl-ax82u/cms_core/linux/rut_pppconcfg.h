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
#ifndef __RUT_PPPCONCFG_H__
#define __RUT_PPPCONCFG_H__


/*!\file rut_pppconcfg.h
 * \brief System level interface functions for ppp connection functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */



#include "cms.h"

#include "devctl_xtm.h"  /* needed for XTM_ADDR */


/** Setup ppp connection.  
 *
 * @param iidStack   (IN) iidStack of the WanPPPConnObject.  This iidStack is
 *                        used to find the ancestor WanLinkCfg object
 * @param *newObj    (IN) the new object
 *
 * @return CmsRet enum.
 */
CmsRet rutCfg_startWanPppConnection(const InstanceIdStack *iidStack, _WanPppConnObject *newObj);

/** After the ppp is up, configures the related iptables rule and other applications
 * such as upnp, rid, igmp, etc..
 *
 * @param iidStack   (IN) iidStack of the WanPPPConnObject.  This iidStack is
 *                        used to find the ancestor WanLinkCfg object
 * @param *newObj    (IN) the new object
 *
 * @return CmsRet enum.
 */
CmsRet rutCfg_setupWanPppConnection(const InstanceIdStack *iidStack, _WanPppConnObject *newObj);


/** If the dsl link is down or the wan interface is removed,
 * remove iptables related to the interface and readjust the
 * applications in the system.
 *
 * @param iidStack   (IN) iidStack of the WanPPPConnObject.  This iidStack is
 *                        used to find the ancestor WanLinkCfg object
 * @param *newObj    (IN) the new object
 * @param *currObj   (IN) the old object
 * @param isIPv4       (IN) indicate if IPv4 or IPv6
 *
 * @return CmsRet enum.
 */
CmsRet rutCfg_tearDownWanPppConnection(const InstanceIdStack *iidStack,
                                       _WanPppConnObject *newObj,
                                       const _WanPppConnObject *currObj, UBOOL8 isIPv4);

/** When pppd is not needed, shut down the ppp service by taking pppd off the system
 *
 * @param iidStack   (IN) iidStack of the WanPPPConnObject.  This iidStack is
 *                        used to find the ancestor WanLinkCfg object
 * @param *currObj   (IN) the old object
 * @param isIPv4       (IN) indicate if IPv4 or IPv6
 *
 *
 * @return CmsRet enum.
 */
CmsRet rutCfg_stopWanPppConnection(const InstanceIdStack *iidStack, const _WanPppConnObject *currObj, UBOOL8 isIPv4);


/** Delete WanPppConnection object from configuration
 *
 * @param iidStack   (IN) iidStack of the WanPPPConnObject.  This iidStack is
 *                        used to find the ancestor WanLinkCfg object
 * @param *currObj   (IN) the old object
 *
 *
 * @return CmsRet enum.
 */
CmsRet rutCfg_deleteWanPppConnection(const InstanceIdStack *iidStack, const _WanPppConnObject *currObj);

#endif  /* __RUT_PPPCONCFG_H__ */
