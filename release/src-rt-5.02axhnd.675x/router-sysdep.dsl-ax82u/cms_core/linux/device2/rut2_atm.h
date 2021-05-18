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

#ifndef __RUT2_ATM_H__
#define __RUT2_ATM_H__

/*!\file rut2_atm.h
 * \brief System level interface functions for ATM functionality and other utility functions
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */

#define ATM_TDTE_ADD    1
#define ATM_TDTE_DELETE 2

/** fills in ATM layer 2 ATM interface name based on ifNameType */
CmsRet rutatm_fillL2IfName_dev2(const Layer2IfNameType ifNameType, char **ifName);


/** Get traffic descriptor index by traffic descriptor parameters
 * @param  *atmLinkQosObj          (IN) the _Dev2AtmLinkQosObject
 *
 * @return CmsRet enum.
 */
UINT32 rutAtm_getTrffDscrIndex_dev2(const _Dev2AtmLinkQosObject *atmLinkQosObj);

/** Add or delete traffic descriptor 
 * @param  *atmLinkQosObj          (IN) the _Dev2AtmLinkQosObject
 * @param  operation               (IN) ATM_TDTE_ADD or ATM_TDTE_DELETE
 * @return CmsRet enum.
 */
CmsRet rutAtm_ctlTrffDscrConfig_dev2(const _Dev2AtmLinkQosObject *atmLinkQosObject, int operation);

/*
 * Description  : add/delete vcc to BcmAtm using atmctl.
 * For correct ATM shaping, the transmit queue size needs to be
 * number_of_lan_receive_buffers / number_of_transmit_queues
 * (80 Ethernet buffers / 8 transmit queues = 10 buffer queue size)
 * @param  *atmLinkObj          (IN) the _Dev2AtmLinkObject
 * @param  *iidStack            (IN) InstanceIdStack of the ATMLinkObject
 * @return CmsRet enum.
 */
CmsRet rutAtm_ctlVccAdd_dev2(const _Dev2AtmLinkObject *atmLinkObject, const InstanceIdStack *iidStack);


/** Create the atm network interface
 * @param  *atmLinkObj          (IN) the _Dev2AtmLinkObject
 *
 * @return CmsRet enum.
 */
CmsRet rutAtm_createInterface_dev2(const _Dev2AtmLinkObject *newObj);


/** Delete the atm network interface 
 * @param *currObj          (IN) the _WanDslLinkCfgObject
 *
 * @return CmsRet enum.
 */
CmsRet rutAtm_deleteInterface_dev2(const _Dev2AtmLinkObject *currObj);


/** Delete the atm VCC 
 * @param *atmLinkObj          (IN) AtmLinkObject
 *
 * @return CmsRet enum.
 */
CmsRet rutAtm_ctlVccDelete_dev2(const Dev2AtmLinkObject *atmLinkObj);

/** Run a loopback test on an atm VCC 
 * @param type          (IN) type of test OAM_F4_LB_SEGMENT_TYPE, OAM_F4_LB_END_TO_END_TYPE, 
 *                                        OAM_F5_LB_SEGMENT_TYPE, OAM_F5_LB_END_TO_END_TYPE
 * @param *new          (IN) ATMDiagnosticsF5LoopbackObject, 
 *
 * @return CmsRet enum.
 */
CmsRet rutAtm_runAtmOamLoopbackTest_dev2(int type, void *new,
                                         const void *curr __attribute__((unused)),
                                         const InstanceIdStack *iidStack);


/** Get ATM link Statistics 
 * @param *atmLinkStatsObject          (IN) AtmLinkStatsObject
 * @param *linkName                    (IN) character string name of this link
 * @param port                         (IN) port ID (interfaceID) of this link
 * @param reset                        (IN) TRUE is reset of statistics desired
 *
 * @return void
 */
void rutatm_getLinkStats_dev2(Dev2AtmLinkStatsObject *stats, char *linkName, int port, UBOOL8 reset);

/** fill in lowerlayer string for the ATM interface.  ATM interface is always
 *  stacked over ATM channel of the primary physical DSL line
 * @param **lowerLayer                 (IN/OUT) character string name ATM channel
 *
 * @return CmsRet
 */
CmsRet rutatm_fillLowerLayer(char **lowerLayer);

#endif /* RUT2_ATM_H__ */
