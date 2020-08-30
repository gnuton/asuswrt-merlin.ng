/*
 * WPS push button
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
 * $Id: wps_pb.h 771467 2019-01-28 10:19:54Z $
 */

#ifndef __WPS_PB_H__
#define __WPS_PB_H__

#include <wps_wps.h>
#include <wlif_utils.h>

#define PBC_OVERLAP_CNT			2
#define WPS_PB_SELECTING_MAX_TIMEOUT	10	/* second */

typedef struct {
	unsigned char	mac[6];
	unsigned int	last_time;
	unsigned char	uuid[16];
} PBC_STA_INFO;

enum {
	WPS_PB_STATE_INIT = 0,
	WPS_PB_STATE_CONFIRM,
	WPS_PB_STATE_SELECTING
} WPS_PB_STATE_T;

#ifdef BCMWPSAPSTA
#define WPS_MAX_PBC_APSTA 3
typedef struct {
	char name[32];
	char ifname[IFNAMSIZ];
} wps_pbc_apsta_intf_t;

extern wps_pbc_apsta_intf_t wps_pbc_ap_ifnames[WPS_MAX_PBC_APSTA];
extern wps_pbc_apsta_intf_t wps_pbc_sta_ifnames[WPS_MAX_PBC_APSTA];
#endif // endif

int wps_pb_check_pushtime(unsigned long time);
void wps_pb_update_pushtime(unsigned char *mac, uint8 *uuid);
void wps_pb_get_uuids(uint8 *buf, int len);
void wps_pb_clear_sta(unsigned char *mac);
wps_hndl_t *wps_pb_check(char *buf, int *buflen);
int wps_pb_find_pbc_ap(char * bssid, char *ssid, uint8 *wsec);
int wps_pb_init();
int wps_pb_deinit();
void wps_pb_reset();
void wps_pb_timeout(int session_opened);
int wps_pb_state_reset();
void wps_pb_ifname_reset();
#endif	/* __WPS_PB_H__ */
