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

#ifndef __RUT_IPSEC_H__
#define __RUT_IPSEC_H__


/*!\file rut_ipsec.h
 * \brief System level interface functions for IPSec functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"



/** Create setkey and strongSwan configuration file.
 * 
 * This is a convenience function made available to other RCL handler functions.
 * This function is also called by the DAL.
 * 
 * @param pWebVar (IN) global resource structure with IPSec info.
 * 
 * @return CmsRet enum.
 */     
CmsRet rutIPSec_config(void);
CmsRet rutIPSec_config_igd(void);
CmsRet rutIPSec_config_dev2(void);
#if defined(SUPPORT_DM_LEGACY98)
#define rutIPSec_config()  rutIPSec_config_igd()
#elif defined(SUPPORT_DM_HYBRID)
#define rutIPSec_config()  rutIPSec_config_igd()
#elif defined(SUPPORT_DM_PURE181)
#define rutIPSec_config()  rutIPSec_config_dev2()
#elif defined(SUPPORT_DM_DETECT)
#define rutIPSec_config()   (cmsMdm_isDataModelDevice2() ?   \
                             rutIPSec_config_dev2()   : \
                             rutIPSec_config_igd())
#endif


/** Restart strongSwan with new configuration. 
 * 
 * This is a convenience function made available to other RCL handler functions.
 * This function is also called by the DAL.
 * 
 * @return CmsRet enum.
 */
CmsRet rutIPSec_restart(void);


CmsRet rutIPSec_getWanIP(const char *wanIntf, char *ipaddr, int *firewall, UBOOL8 isIPv4);


int rutIPSec_calPrefixLen(char *ipAddr);


CmsRet rutIPSec_activateTunnel(void);
CmsRet rutIPSec_activateTunnel_igd(void);
CmsRet rutIPSec_activateTunnel_dev2(void);
#if defined(SUPPORT_DM_LEGACY98)
#define rutIPSec_activateTunnel()  rutIPSec_activateTunnel_igd()
#elif defined(SUPPORT_DM_HYBRID)
#define rutIPSec_activateTunnel()  rutIPSec_activateTunnel_igd()
#elif defined(SUPPORT_DM_PURE181)
#define rutIPSec_activateTunnel()  rutIPSec_activateTunnel_dev2()
#elif defined(SUPPORT_DM_DETECT)
#define rutIPSec_activateTunnel()   (cmsMdm_isDataModelDevice2() ?   \
                                  rutIPSec_activateTunnel_dev2()   : \
                                  rutIPSec_activateTunnel_igd())
#endif


#endif // __RUT_IPSEC_H__
