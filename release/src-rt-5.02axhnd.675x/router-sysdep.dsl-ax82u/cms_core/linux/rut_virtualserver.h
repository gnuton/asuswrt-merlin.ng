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
#ifndef __RUT_VIRTUALSERVER_H__
#define __RUT_VIRTUALSERVER_H__


/*!\file rut_virtualserver.h
 * \brief System level interface functions for virtual server functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */




#include "cms.h"

#define FTP_SERVER_PORT_21       21
#define FTP_SERVER_PORT_2121     2121
#define WEB_SERVER_PORT_80       80
#define WEB_SERVER_PORT_8080     8080
#define SNMP_AGENT_PORT_161      161
#define SNMP_AGENT_PORT_16116    16116
#define SSH_SERVER_PORT_22       22
#define SSH_SERVER_PORT_2222     2222
#define TELNET_SERVER_PORT_23    23
#define TELNET_SERVER_PORT_2323  2323
#define TFTP_SERVER_PORT_69      69
#define TFTP_SERVER_PORT_6969    6969

#define LOCK_TIMEOUT  (6 * MSECS_IN_SEC)

#define VRTSRV_ENABLE_NEW_OR_ENABLE_EXISTING(n, c) \
   (((n) != NULL && (n)->portMappingEnabled && (c) == NULL) || \
   ((n) != NULL && (n)->portMappingEnabled && (c) != NULL && !((c)->portMappingEnabled)))

#define VRTSRV_DELETE_OR_DISABLE_EXISTING(n, c) \
   (((n) == NULL) ||                                                    \
    ((n) != NULL && !((n)->portMappingEnabled) && (c) != NULL && (c)->portMappingEnabled))

    
/** Find out if the ip connection object parameters have been changed or not.
 * 
 * @param *newObj       (IN) the new _WanIpConnObject.
 * @param *CurrObj      (IN) the current _WanIpConnObject.
 * @return BOOL  TRUE -- currObj object parameters in the check differ from newObj.  FALSE -- no change
 */
CmsRet rutIpt_vrtsrvCfg(const _WanIpConnObject *wan_ip_con, const _WanPppConnObject *wan_ppp_con, 
                                       const _WanPppConnPortmappingObject *pppObj, const _WanIpConnPortmappingObject *ipObj, const UBOOL8 add);

#endif // __RUT_VIRTUALSERVER_H__

