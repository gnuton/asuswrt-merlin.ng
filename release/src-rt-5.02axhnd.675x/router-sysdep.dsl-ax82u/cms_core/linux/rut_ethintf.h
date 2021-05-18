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
#ifndef __RUT_ETHINTF_H__
#define __RUT_ETHINTF_H__


/*!\file rut_ethintf.h
 * \brief System level interface functions for ethernet interface functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"


/** find the WanEthIntf object with the given layer 2 ifName.
 *
 * @param ifName (IN) layer 2 ifName of WanEthIntf to find.
 * @param iidStack (OUT) iidStack of the WanEthIntf object found.
 * @param ethIntfCfg (OUT) if not null, this will contain a pointer to the found
 *                         WanEthIntf object.  Caller is responsible for calling
 *                         cmsObj_free() on this object.
 *
 * @return UBOOL8 indicating whether the desired WanEthIntf object was found.
 */
UBOOL8 rutEth_getEthIntfByIfName(char *ifName, InstanceIdStack *iidStack, WanEthIntfObject **ethIntfCfg);

/** if device has ETH wan  
 * 
 * @param none
 *
 * @return UBOOL8 indicating whether ETH wan be set.
 */
UBOOL8 rutEth_isEthWanMode();


/** set eth port as wan port of external switch
 * 
 * @param ifname (IN) layer 2 ifName.
 * @param enabled (IN) enable or disable wan port selection.
 *
 * @return CmsRet       enum.
 */
CmsRet rutEth_setSwitchWanPort(char *ifname, int enabled);


#endif 

