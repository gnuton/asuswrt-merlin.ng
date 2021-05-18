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

#ifndef __CMS_RUT_ATM_H__
#define __CMS_RUT_ATM_H__


/*!\file rut_atm.h
 * \brief System level interface functions for ATM functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */



typedef struct
{
   char *cmpStr;
   char *retStr;
} atmQosDefs;

#define atmctlName         "xtmctl"
#define atmVccString       atmctlName " operate conn --"
#define atmTdteString      atmctlName " operate tdte --"

/*
 * Description  : add/delete vcc to BcmAtm using atmctl.
 * For correct ATM shaping, the transmit queue size needs to be
 * number_of_lan_receive_buffers / number_of_transmit_queues
 * (80 Ethernet buffers / 8 transmit queues = 10 buffer queue size)
 */
CmsRet rutAtm_ctlVccAdd(const WanDslLinkCfgObject *dslLinkCfg);

/** Create  the atm interface
 * @param *newObj          (IN) the _WanDslLinkCfgObject
 *
 * @return CmsRet enum.
 */
CmsRet rutAtm_createInterface(const _WanDslLinkCfgObject *newObj);

/** Delete the atm network interface 
 * @param *currObj          (IN) the _WanDslLinkCfgObject
 *
 * @return CmsRet enum.
 */
CmsRet rutAtm_deleteInterface(const _WanDslLinkCfgObject *currObj);

/** Delete the atm network connection (VCC)
 * @param *currObj          (IN) the _WanDslLinkCfgObject
 *
 * @return CmsRet enum.
 */
CmsRet rutAtm_ctlVccDelete(const WanDslLinkCfgObject *dslLinkCfg);

CmsRet rutAtm_categoryConvertion(const char *inCmpStr, char *outResultStr, int outStrLen);

#endif // __CMS_RUT_ATM_H__                     
