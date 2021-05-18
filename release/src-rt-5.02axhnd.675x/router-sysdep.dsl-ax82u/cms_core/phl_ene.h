/***********************************************************************
 *
 *  Copyright (c) 2006-2018  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

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
 * phl_ene.h
 *
 *  Created on:  Sep.16, 2018
 *  Author: Fuguo Xu <fuguo.xu@broadcom.com>
 */


/*
 * this file export the declaration of common function in phl_ene.c for
 * MULTIPLE_TR69C_SUPPORT case.
 */


#ifndef __PHL_ENE_H__
#define __PHL_ENE_H__

#include "cms.h"
#include "cms_mdm.h"
#include "mdm_types.h"


#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)

/** Is this object or parameter hidden from the specified TR-069 ACS.
 *
 * This is a similar function as legacy_func(mdm_getPathDescHiddenFromAcs). the 
 * difference is that legacy_func determines whether hide or not according to the node 
 * hiding mask in xml definition, which will hide from all tr69c/ACS. this one is based 
 * on Blacklist Table of each tr69c/ACS.
 *
 * We might not want to send all of our parameters to the ACS.
 * This entry point allows other parameter walking functions,
 * (e.g. phl_getNextPath, mdm_getNextChildObjPathDesc, etc). to determine
 * if this object or parameter should be hidden from specified tr69c and hence the
 * specified TR-069 ACS.  From an architectural perspective, tr69c should do the
 * hiding.  But from a coding perspective, it is much cleaner to do the
 * hiding in the phl and mdm layers.
 *
 * @param      (IN)  Path descriptor pointing to the object or parameter in question.
 * @param      (OUT) TRUE if pathDesc specifies an object or param that is hidden
 *                   from the specified tr69c.
 *
 * @return CmsRet enum.
 */
CmsRet ene_getPathDescHiddenFromAcs(const MdmPathDescriptor *pathDesc,
                                              UBOOL8 *hidden);

#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)


#endif /* __PHL_ENE_H__ */

