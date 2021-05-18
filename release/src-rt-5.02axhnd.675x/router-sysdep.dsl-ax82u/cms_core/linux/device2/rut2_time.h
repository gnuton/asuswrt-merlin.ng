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

#ifndef __RUT2_TIME_H__
#define __RUT2_TIME_H__


/*!\file rut2_time.h
 * \brief System level interface functions for NTP functionality.
 *
 * The functions in this file should only be called by
 * RCL2, STL2, and other RUT2 functions.
 */


#include "cms.h"

/** Restart sntp with the SNTP entry in the MDM
 *
 * If there is already a sntp running, it will be stopped first.
 * 
 */
void rutSntp_restart_dev2(void);


/** Stop the sntp application.
 *
 */
void rutSntp_stop_dev2(void);

/** Restart ntpd with the Time entry in the MDM
 *
 * If there is already a ntpd running, it will be stopped first.
 * 
 */
void rutNtpd_restart_dev2(void);


/** Stop the ntpd application.
 *
 */
void rutNtpd_stop_dev2(void);


/** Verify all required values are in the object.
 *
 * @param TimeObj (IN) The new Time object to check.
 *
 * @return TRUE if all required fields are non-null, otherwise, return FALSE.
 */
UBOOL8 rutTime_isAllRequiredValuesPresent_dev2(const _Dev2TimeObject *timeObj);


/** Detect changes of any fields between newObj and currObj.
 *
 * @param newObj  (IN) The new Time object.
 * @param currObj (IN) The current Time object.
 *
 * @return TRUE if any of the fields (except for the enabled field) has changed. 
 */
UBOOL8 rutTime_isValuesChanged_dev2(const _Dev2TimeObject *newObj, const _Dev2TimeObject *currObj);


#endif
