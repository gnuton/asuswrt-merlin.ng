/*
 * WPS upnp
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
 * $Id: wps_upnp.h 676559 2016-12-22 17:02:55Z $
 */

#ifndef __WPS_UPNP_H__
#define __WPS_UPNP_H__

#include <wps_wps.h>

typedef struct upnp_attached_if_list {
	struct upnp_attached_if_list *next;
	int ess_id;
	char ifname[IFNAMSIZ]; /* for libupnp */
	int instance;
	wps_hndl_t upnp_hndl;
	char mac[6];
	char wl_name[IFNAMSIZ];
	char *m1_buf;
	int m1_len;
	unsigned int m1_built_time;
	char *enr_nonce;
	char *private_key;
} upnp_attached_if;

void wps_upnp_init();
void wps_upnp_deinit();
void wps_upnp_clear_ssr();
void wps_upnp_clear_ssr_timer();
int wps_upnp_ssr_expire();
void wps_upnp_device_uuid(unsigned char *uuid);
char *wps_upnp_parse_msg(char *upnpmsg, int upnpmsg_len, int *len, int *type, char *addr);
int wps_upnp_process_msg(char *upnpmsg, int upnpmsg_len);
int wps_upnp_send_msg(int if_instance, char *buf, int len, int type);

void wps_upnp_update_wlan_event(int if_instance, unsigned char *macaddr,
	char *databuf, int datalen, int init, char event_type);
void wps_upnp_update_init_wlan_event(int if_instance, char *mac, int init);
void wps_upnp_forward_preb_req(int if_instance, unsigned char *macaddr,
	char *databuf, int datalen);
char *wps_upnp_type_name(int type);

#ifdef WPS_UPNP_DEVICE
int wps_libupnp_ProcessMsg(char *ifname, char *upnpmsg, int upnpmsg_len);
int wps_libupnp_GetOutMsgLen(char *ifname);
char *wps_libupnp_GetOutMsg(char *ifname);
#endif /* WPS_UPNP_DEVICE */

#endif	/* __WPS_UPNP_H__ */
