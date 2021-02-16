/*
 * Application-specific portion of EAPD
 * (WAI)
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
 * $Id: wai_eap.c 758209 2018-04-18 02:43:48Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <typedefs.h>
#include <bcmutils.h>
#include <ethernet.h>
#include <eapol.h>
#include <eap.h>
#include <bcmendian.h>
#include <wlutils.h>
#include <eapd.h>
#include <shutils.h>
#include <UdpLib.h>
#include <security_ipc.h>
#include <bcmnvram.h>

#define AUTH_MODE_WAPI 		1
#define AUTH_MODE_WAPI_PSK	2

/* Receive wai message from wapid module
 */
void
wai_app_recv_handler(eapd_wksp_t *nwksp, eapd_cb_t *from, uint8 *pData, int *pLen)
{
	if (!nwksp || !from || !pData) {
		EAPD_ERROR("Wrong argument...\n");
		return;
	}

	/* send message data out. */
	eapd_message_send(nwksp, from->brcmSocket, pData, *pLen);

	return;
}

void
wai_app_set_eventmask(eapd_app_t *app)
{
	memset(app->bitvec, 0, sizeof(app->bitvec));

	setbit(app->bitvec, WLC_E_WAI_MSG);
	setbit(app->bitvec, WLC_E_WAI_STA_EVENT);
	return;
}

int
wai_app_init(eapd_wksp_t *nwksp)
{
	int ret, reuse = 1;
	eapd_wai_t *wai;
	eapd_cb_t *cb;
	struct sockaddr_in addr;

	if (nwksp == NULL)
		return -1;

	wai = &nwksp->wai;
	wai->appSocket = -1;

	cb = wai->cb;
	if (cb == NULL) {
		EAPD_INFO("No any WAI application need to run.\n");
		return 0;
	}

	while (cb) {
		EAPD_INFO("wai: init brcm interface %s \n", cb->ifname);
		cb->brcmSocket = eapd_add_brcm(nwksp, cb->ifname);
		if (!cb->brcmSocket)
			return -1;
		/* set this brcmSocket have WAI capability */
		cb->brcmSocket->flag |= EAPD_CAP_WAI;
		cb = cb->next;
	}

	/* appSocket for wapid */
	wai->appSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (wai->appSocket < 0) {
		EAPD_ERROR("UDP Open failed.\n");
		return -1;
	}
	if (setsockopt(wai->appSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse,
		sizeof(reuse)) < 0) {
		EAPD_ERROR("UDP setsockopt failed.\n");
		close(wai->appSocket);
		wai->appSocket = -1;
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = EAPD_UDP_SIN_ADDR(nwksp);
	addr.sin_port = htons(EAPD_WKSP_WAI_UDP_RPORT);
	if (bind(wai->appSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		EAPD_ERROR("UDP Bind failed, close wai appSocket %d\n", wai->appSocket);
		close(wai->appSocket);
		wai->appSocket = -1;
		return -1;
	}
	EAPD_INFO("WAI appSocket %d opened\n", wai->appSocket);

	return 0;
}

int
wai_app_deinit(eapd_wksp_t *nwksp)
{
	eapd_wai_t *wai;
	eapd_cb_t *cb, *tmp_cb;

	if (nwksp == NULL) {
		EAPD_ERROR("Wrong argument...\n");
		return -1;
	}

	wai = &nwksp->wai;
	cb = wai->cb;
	while (cb) {
		/* close  brcm drvSocket */
		if (cb->brcmSocket) {
			EAPD_INFO("close wai brcmSocket %d\n", cb->brcmSocket->drvSocket);
			eapd_del_brcm(nwksp, cb->brcmSocket);
		}

		tmp_cb = cb;
		cb = cb->next;
		free(tmp_cb);
	}

	/* close  appSocke */
	if (wai->appSocket >= 0) {
		EAPD_INFO("close wai appSocket %d\n", wai->appSocket);
		close(wai->appSocket);
		wai->appSocket = -1;
	}

	return 0;
}

/* wai_sendup will send wai and brcm packet */
int
wai_app_sendup(eapd_wksp_t *nwksp, uint8 *pData, int pLen, char *from)
{
	eapd_wai_t *wai;

	if (nwksp == NULL) {
		EAPD_ERROR("Wrong argument...\n");
		return -1;
	}

	wai = &nwksp->wai;
	if (wai->appSocket >= 0) {
		/* send to wai */
		int sentBytes = 0;
		struct sockaddr_in to;

		to.sin_addr.s_addr = inet_addr(EAPD_WKSP_UDP_ADDR);
		to.sin_family = AF_INET;
		to.sin_port = htons(EAPD_WKSP_WAI_UDP_SPORT);

		sentBytes = sendto(wai->appSocket, pData, pLen, 0,
			(struct sockaddr *)&to, sizeof(struct sockaddr_in));

		if (sentBytes != pLen) {
			EAPD_ERROR("UDP send failed; sentBytes = %d\n", sentBytes);
		}
		else {
			/* EAPD_INFO("Send %d bytes to wai\n", sentBytes); */
		}
	}
	else {
		EAPD_ERROR("wai appSocket not created\n");
	}
	return 0;
}

#if EAPD_WKSP_AUTO_CONFIG
int
wai_app_enabled(char *name)
{
	char value[128], comb[32], akm[16], prefix[8];
	char akms[128], os_name[IFNAMSIZ];
	char *akmnext;
	uint wapid;
	int unit;

	memset(os_name, 0, sizeof(os_name));

	if (nvifname_to_osifname(name, os_name, sizeof(os_name)))
		return 0;
	if (wl_probe(os_name) ||
		wl_ioctl(os_name, WLC_GET_INSTANCE, &unit, sizeof(unit)))
		return 0;
	if (osifname_to_nvifname(name, prefix, sizeof(prefix)))
		return 0;

	strcat(prefix, "_");
	/* ignore if disabled */
	eapd_safe_get_conf(value, sizeof(value), strcat_r(prefix, "radio", comb));
	if (atoi(value) == 0) {
		EAPD_INFO("WAI:ignored interface %s. radio disabled\n", os_name);
		return 0;
	}
	/* ignore shared */
	eapd_safe_get_conf(value, sizeof(value), strcat_r(prefix, "auth", comb));
	if (atoi(value) == 1) {
		EAPD_INFO("WAI: ignored interface %s. Shared 802.11 auth\n", os_name);
		return 0;
	}
	/* ignore if BSS is disabled */
	eapd_safe_get_conf(value, sizeof(value), strcat_r(prefix, "bss_enabled", comb));
	if (atoi(value) == 0) {
		EAPD_INFO("WAI: ignored interface %s, %s is disabled \n", os_name, comb);
		return 0;
	}
	/* wai mode */
	wapid = 0;
	eapd_safe_get_conf(akms, sizeof(akms), strcat_r(prefix, "akm", comb));
	foreach(akm, akms, akmnext) {
		if (!strcmp(akm, "wapi"))
			wapid |= AUTH_MODE_WAPI;
		if (!strcmp(akm, "wapi_psk"))
			wapid |= AUTH_MODE_WAPI_PSK;
	}
	if (!wapid) {
		EAPD_INFO("WAI:ignored interface %s. Invalid WAI mode\n", os_name);
		return 0;
	}

	/* if come to here return enabled */
	return 1;
}
#endif /* EAPD_WKSP_AUTO_CONFIG */

int
wai_app_handle_event(eapd_wksp_t *nwksp, uint8 *pData, int Len, char *from)
{
	int type;
	eapd_wai_t *wai;
	eapd_cb_t *cb;
	bcm_event_t *dpkt = (bcm_event_t *) pData;
	wl_event_msg_t *event;

	event = &(dpkt->event);
	type = ntohl(event->event_type);

	wai = &nwksp->wai;
	cb = wai->cb;
	while (cb) {
		if (isset(wai->bitvec, type) && !strcmp(cb->ifname, from)) {
			/* prepend ifname,  we reserved IFNAMSIZ length already */
			pData -= IFNAMSIZ;
			Len += IFNAMSIZ;
			memcpy(pData, event->ifname, IFNAMSIZ);

			/* send to wai use cb->ifname */
			wai_app_sendup(nwksp, pData, Len, cb->ifname);
			break;
		}
		cb = cb->next;
	}

	return 0;
}
