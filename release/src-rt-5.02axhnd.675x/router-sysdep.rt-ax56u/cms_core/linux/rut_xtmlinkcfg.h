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
#ifndef __RUT_XTMLINKCFG_H__
#define __RUT_XTMLINKCFG_H__


/*!\file rut_xtmlinkcfg.h
 * \brief System level interface functions for XTM functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */

#ifdef DMP_X_BROADCOM_COM_ATMSTATS_1
#include "bcmxtmcfg.h"
#include "cms.h"

void rut_getAtmIntfStats(WanAtmStatsObject *stats, UINT32 port, UINT32 reset);
void rut_getAtmAal5IntfStats(_WanAal5StatsObject *stats, UINT32 port, UINT32 reset);
void rut_getAtmAal2IntfStats(_WanAal2StatsObject *stats, UINT32 port, UINT32 reset);
void rut_getAtmVccStats(UINT32 vpi, UINT32 vci, UINT32 port, UINT32 *crcError, UINT32 *oversizedSdu,
                        UINT32 *shortPktErr,UINT32 *lenErr);
void rut_clearAtmVccStats(UINT32 vpi, UINT32 vci, UINT32 port);
#endif

#ifdef DMP_X_BROADCOM_COM_XTMSTATS_1
void rut_getXtmIntfStats(XtmInterfaceStatsObject *stats, UINT32 port, UINT32 reset);
#endif



 /** Get the xDSL link status (UP or Down) from WanDSLInterfaceConfig object with dsl/ptmLinkCfg
* iidStack and corresponding oid. 
*
* @param *iidStack      (IN) dslLinkCfg or ptmLinkCfg iidStack
* @param  decendentOid  (IN) either MDMOID_WAN_PTM_LINK_CFG or MDMOID_WAN_DSL_LINK_CFG
*
* @return TRUE if xDSL link is up, FALSE if down.
*/

UBOOL8 rutXtm_isXDSLLinkUp(const InstanceIdStack *iidStack, MdmObjectId decendentOid);

#endif /* __RUT_XTMLINKCFG_H__ */

