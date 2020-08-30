/*
 * WPS IE
 *
 * Copyright 2020 Broadcom
 *
 * This program is the proprietary software of Broadcom and/or
 * its licensors, and may only be used, duplicated, modified or distributed
 * pursuant to the terms and conditions of a separate, written license
 * agreement executed between you and Broadcom (an "Authorized License").
 * Except as set forth in an Authorized License, Broadcom grants no license
 * (express or implied), right to use, or waiver of any kind with respect to
 * the Software, and Broadcom expressly reserves all rights in and to the
 * Software and all intellectual property rights therein.  IF YOU HAVE NO
 * AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 * WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 * THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1. This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof, and to
 * use this information only in connection with your use of Broadcom
 * integrated circuit products.
 *
 * 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 * "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 * OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 * SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 * IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 * IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 * ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 * OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 * NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wps_ie.h 525052 2015-01-08 20:18:35Z $
 */

#ifndef __WPS_IE_H__
#define __WPS_IE_H__

int wps_ie_default_ssr_info(CTlvSsrIE *ssrmsg, unsigned char *authorizedMacs,
	int authorizedMacs_len, BufferObj *authorizedMacs_Obj, unsigned char *wps_uuid,
	BufferObj *uuid_R_Obj, uint8 scState);
void wps_ie_set(char *wps_ifname, CTlvSsrIE *ssrmsg);
void wps_ie_clear();

#include <bcmconfig.h>
#ifdef __CONFIG_WFI__
/*
*  Generic Vendor Extension Support:
*/
#define WPSM_VNDR_EXT_MAX_DATA_80211	(3 + 246)

int wps_vndr_ext_obj_free(void *ptToFree);
/*
*  Return: <0, the object not found; >= 0 OK.
*  ptToFree: pointer to vendor object.
*/

void *wps_vndr_ext_obj_alloc(int siBufferSize, char *pcOSName);
/*
*  Return the Vendor Extension Object Pointer allocated with buffer
*     size siBufferSize. NULL if BufferSize is too big or malloc error.
*  siBufferSize: < 0, the WPSM_VNDR_EXT_MAX_DATA_80211 will be allocated.
*  pcOSName: pointer to char that contains OS name of an interface
*     that this vendor object belongs to.
*/

int wps_vndr_ext_obj_copy(void *ptObj, char *pcData, int siLen);
/*
*  Return <0, Error; >0, number of bytes copied.
*  pcData: pointer to char array containing input data.
*  siLen: Number of data to copy.
*/

int wps_vndr_ext_obj_set_mode(void *ptObj, int siType, int boolActivate);
/*
*  Return < 0 if error; >=0 if OK.
*  siType: WPS_IE_TYPE_SET_BEACON_IE or WPS_IE_TYPE_SET_PROBE_RESPONSE_IE.
*  boolActivate: 0, set but do not activate it; 1, set and activate it.
*/
#endif /* __CONFIG_WFI__ */

#endif	/* __WPS_IE_H__ */
