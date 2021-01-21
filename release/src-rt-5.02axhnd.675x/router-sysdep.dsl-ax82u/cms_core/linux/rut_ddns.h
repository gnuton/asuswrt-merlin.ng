/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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
#ifndef __RUT_DDNSD_H__
#define __RUT_DDNSD_H__


/*!\file rut_ddns.h
 * \brief System level interface functions for Dynamic DNS functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */



#include "cms.h"


/** Restart ddns with all the enabled DDNS entries in the MDM.
 *
 * If there is already a ddnsd running, it will be stopped first.
 * The existing configuration file will be overwritten with the new values.
 * 
 */
void rutDDns_restart(void);


/** Remove the DDNS config file and stop the application.
 *
 */
void rutDDns_stop(void);


/** Verify all required values are in the object.
 *
 * @param ddnsObj (IN) The new DDNS object to check.
 *
 * @return TRUE if all required fields are non-null, otherwise, return FALSE.
 */
UBOOL8 rutDDns_isAllRequiredValuesPresent(const _DDnsCfgObject *ddnsObj);


/** Check for duplicate fully qualified domain name against all DDNS entries in 
 *  the MDM except for the one at skipIidStack.
 *
 * @param fqdn (IN) This must be set to the value to check against.
 * @param skipIidStack (IN) This may be NULL.  If non-NULL, the entry pointed to by
 *                          skipIidStack will not be checked against fqdn because
 *                          the fqdn that is being checked is the one at skipIidStack,
 *                          so we don't want to check for duplicate against myself.
 *
 * @return TRUE if a duplicate is found.
 */
UBOOL8 rutDDns_isDuplicateFQDN(const char *fqdn, const InstanceIdStack *skipIidStack);


/** Detect changes of any fields between newObj and currObj.
 *
 * @param newObj  (IN) The new DDNS object.
 * @param currObj (IN) The current DDNS object.
 *
 * @return TRUE if any of the fields (except for the enabled field) has changed. 
 */
UBOOL8 rutDDns_isValuesChanged(const _DDnsCfgObject *newObj, const _DDnsCfgObject *currObj);


/** Count number of enabled DDNS entries in the MDM.
 *
 * @return Number of enabled DDNS entries in the MDM.
 */
UINT32 rutDDns_getNumberOfEnabledEntries(void);


#endif
