/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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

#ifndef __RUT2_PTM_H__
#define __RUT2_PTM_H__


/*!\file rut2_ptm.h
 * \brief System level interface functions for ATM functionality and other utility functions
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */

/** fills in ATM layer 2 PTM interface name based on ifNameType */
CmsRet rutptm_fillL2IfName_dev2(const Layer2IfNameType ifNameType, char **ifName);

/** create a PVC over PTM 
 * @param  *ptmLinkObj          (IN) the _Dev2PtmLinkObject
 *
 * @return CmsRet enum.
 */
CmsRet rutptm_setConnCfg_dev2(const _Dev2PtmLinkObject *newObj);

/** Delete a PVC over PTM 
 * @param  *ptmLinkObj          (IN) the _Dev2PtmLinkObject
 *
 * @return CmsRet enum.
 */
CmsRet rutptm_deleteConnCfg_dev2(const _Dev2PtmLinkObject *newObj);

/** Create the ptm network interface
 * @param  *newObj          (IN) the _Dev2PtmLinkObject
 *
 * @return CmsRet enum.
 */
CmsRet rutptm_createInterface_dev2(const _Dev2PtmLinkObject *newObj);


/** Delete the ptm network interface 
 * @param  *currObj          (IN) the _Dev2PtmLinkObject
 *
 * @return CmsRet enum.
 */
CmsRet rutptm_deleteInterface_dev2(const _Dev2PtmLinkObject *currObj);

/** Get PTM link Statistics 
 * @param *ptmLinkStatsObject          (IN) Dev2PtmLinkStatsObject
 * @param *linkName                    (IN) character string name of this link
 * @param port                         (IN) port ID (interfaceID) of this link
 * @param reset                        (IN) TRUE is reset of statistics desired
 *
 * @return void
 */
void rutptm_getLinkStats_dev2(Dev2PtmLinkStatsObject *stats, char *linkName, int port, UBOOL8 reset);

/** fill in lowerlayer string for the PTM interface.  PTM interface is always
 *  stacked over PTM channel of the primary physical DSL line
 * @param **lowerLayer                 (IN/OUT) character string name PTM channel
 *
 * @return CmsRet
 */
CmsRet rutptm_fillLowerLayer(char **lowerLayer);

#endif /* RUT2_PTM_H__ */
