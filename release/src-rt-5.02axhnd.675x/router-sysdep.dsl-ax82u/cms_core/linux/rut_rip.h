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

#ifndef __RUT_RIP_H__
#define __RUT_RIP_H__


/*!\file rut_rip.h
 * \brief System level interface functions for RIP functionality.
 *        ripd is the daemon that handles the RIP protocol.
 *        zebra is the manager for ripd (and ospf and bfp and etc).
 *        Right now zebra is mixed in with ripd, but in the future may
 *        need to separate it if we want to support ospf and not rip.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */

#include "cms.h"


/** Check if rip config is changed or not
 *
 * @return  TRUE or FALSE
 */
UBOOL8 rutRip_isConfigChanged_igd(const _WanIpConnObject *newObj,
                                  const _WanIpConnObject *currObj);

/** Check if rip config is valid
 *
 * @return  TRUE or FALSE
 */
UBOOL8 rutRip_isConfigValid_igd(const _WanIpConnObject *newObj);
UBOOL8 rutRip_isConfigValid_dev2(const Dev2RipIntfSettingObject *newObj);


/** Return the number of RIP enabled interfaces (regardless of their
 *  connection state.)  We will need igd and dev2 versions of this func.
 *  Move to QDM later.
 */
UINT32 rutRip_getNumberOfRipInterfaces(void);
UINT32 rutRip_getNumberOfRipInterfaces_igd(void);

#define rutRip_getNumberOfRipInterfaces() rutRip_getNumberOfRipInterfaces_igd()




/** Write out a block of config for a single interface.
 *
 */
void rutRip_addRipEntryToConfigFile(FILE *fs, char *ifcName, char *version, char *operation);


/** Look for all WAN interfaces which has RIP enabled and add them
 * to config file.
 */
void rutRip_addAllRipInterfacesToConfigFile(FILE *fs);
void rutRip_addAllRipInterfacesToConfigFile_igd(FILE *fs);
void rutRip_addAllRipInterfacesToConfigFile_dev2(FILE *fs);

#if defined(SUPPORT_DM_LEGACY98)
#define rutRip_addAllRipInterfacesToConfigFile(f)  rutRip_addAllRipInterfacesToConfigFile_igd((f))
#elif defined(SUPPORT_DM_HYBRID)
#define rutRip_addAllRipInterfacesToConfigFile(f)  rutRip_addAllRipInterfacesToConfigFile_igd((f))
#elif defined(SUPPORT_DM_PURE181)
#define rutRip_addAllRipInterfacesToConfigFile(f)  rutRip_addAllRipInterfacesToConfigFile_dev2((f))
#elif defined(SUPPORT_DM_DETECT)
#define rutRip_addAllRipInterfacesToConfigFile(f)  (cmsMdm_isDataModelDevice2() ? \
                                rutRip_addAllRipInterfacesToConfigFile_dev2((f)) : \
                                rutRip_addAllRipInterfacesToConfigFile_igd((f)))
#endif




/** Write the config file for ripd and zebra.
 *  Side Effect: add firewall exceptions for ripd
 *
 */
void rutRip_writeConfigFile(void);


/** Interface is UP or config has changed, restart ripd and zebra.
 *  (If not already running, restart will just start ripd and zebra).
 *
 */
void rutRip_restart(void);
void rutRip_restart_dev2();


/** stop ripd and zebra processes.
 *  Side Effect: remove the firewall exceptions for ripd
 *
 */
void rutRip_stop(void);

UBOOL8 rut_isRipEnabled_dev2(void);

#endif

