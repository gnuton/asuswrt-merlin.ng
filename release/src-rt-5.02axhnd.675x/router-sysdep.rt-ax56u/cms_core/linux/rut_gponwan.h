/***********************************************************************
 *
 *  Copyright (c) 2010  Broadcom Corporation
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

#ifndef __RUT_GPONWAN_H__
#define __RUT_GPONWAN_H__


/*!\file rut_gponwan.h
 * \brief System level interface functions for gpon as wan functionality.
 *
 * In most cases, the functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"
#include "cms_core.h"


/** find the WanGponLinkCfg iidStack or object with the given layer 2 ifName.
 *
 * @param ifName (IN) layer 2 ifName of layer 2 gpon ifName to find.
 * @param iidStack (OUT) iidStack of the gpon link object found.
 * @param ethIntfCfg (OUT) if not null, this will contain a pointer to the found
 *                         gpon link object.  Caller is responsible for calling
 *                         cmsObj_free() on this object.
 *
 * @return UBOOL8 indicating whether the desired gpon link iidStack or/and object was found.
 */
UBOOL8 rutGpon_getGponLinkByIfName(char *ifName, InstanceIdStack *iidStack, _WanGponLinkCfgObject **gponLinkCfg);

/** send message to ssk to get GPON WAN layer 2 interface info
 *
 * @param GponNthWanLinkInfo->linkEntryIdx (IN) GPON WAN index 
 *                          to find layer 2 GPON interface info
 * @param gponLinkInfoP (OUT) layer 2 GPON interface info
 *
 * @return CmsRet.
 */
CmsRet rutGpon_getNthGponWanLinkL2Info(GponNthWanLinkInfo *gponLinkInfoP);

/** get VEIP layer 2 interface name
 *
 * @param pL2Ifname (OUT) 
 * @param linkEntryIdx (IN)                 
 *
 * @return CmsRet.
 */
CmsRet rutGpon_getGponWanLinkL2IfName(const UINT32 linkEntryIdx, char *pL2Ifname);

/** send GPON WAN service information to SSK
 *
 * @param pService (IN) pointer to GPON WAN service information
 *
 * @return CmsRet.
 */
CmsRet rutGpon_sendServiceInfoMsg (const OmciServiceMsgBody *pService); 

/**  Get the WAN service Oid and iidStack
 *
 * @param InstanceIdStack *gponLinkCfgIid (IN) GPON Link Config 
 *                        IidStack
 * @param MdmObjectId *oid (OUT) 
 * @param InstanceIdStack *iidStack (OUT)
 * @return CmsRet
 */
CmsRet rutGpon_getServiceOidAndIidStack(const InstanceIdStack *gponLinkCfgIid,
                                        MdmObjectId *oid, InstanceIdStack *iidStack);

/**  Get the WAN service Parameters
 *
 * @param GponWanServiceParams *pServiceParams (OUT)
 * @param MdmObjectId *oid (IN) 
 * @param InstanceIdStack *iidStack (IN)
 * @return CmsRet
 */
CmsRet rutGpon_getWanServiceParams(const MdmObjectId oid, const InstanceIdStack *iidStack,
                                   GponWanServiceParams *pServiceParams);

/**  Get the WAN service Layer-2 If name
 *
 * @param char *pL2Ifname (OUT)
 * @param MdmObjectId *oid (IN) 
 * @param InstanceIdStack *iidStack (IN)
 * @return CmsRet
 */
CmsRet rutGpon_getWanServiceL2IfName(const MdmObjectId oid, 
                                     const InstanceIdStack *iidStack,
                                     char *pL2Ifname);
#endif  /* __RUT_GPONWAN_H__ */

