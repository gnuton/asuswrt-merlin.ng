/*
 * WPS include
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
 * $Id: wps_wps.h 772297 2019-02-20 06:50:21Z $
 */
#ifndef __WPS_H__
#define __WPS_H__

#include <typedefs.h>

#include <bcmconfig.h>

/* Packet handle socket */
typedef struct wps_hndl {
	struct wps_hndl *next;
	int type;
	int handle;
	char ifname[16];
#ifdef	WPS_UPNP_DEVICE
	void *private;
#endif // endif
} wps_hndl_t;

void wps_hndl_add(wps_hndl_t *hndl);
void wps_hndl_del(wps_hndl_t *hndl);

char *wps_get_conf(char *name);
char *wps_safe_get_conf(char *name);
int wps_set_conf(char *name, char *value);

/* Include OSL portion definitions */
#include <wps_osl.h>
#include <wps_hal.h>

#define WL_MAX_UNIT			2

#define RANDOM_SSID_LENGTH		6
#define WPSBTN_EVTI_NULL		0
#define WPSBTN_EVTI_PUSH		1
#define WPS_MAX_TIMEOUT			120
#define WPS_EAPD_READ_MAX_LEN		2048
#define WPS_EAPD_READ_TIMEOUTSEC	1	/* second */

#define WPSM_WKSP_FLAG_SHUTDOWN		1
#define WPSM_WKSP_FLAG_SET_RESTART	2
#define WPSM_WKSP_FLAG_RESTART_WL	4
#define WPSM_WKSP_FLAG_SUCCESS_RESTART	(WPSM_WKSP_FLAG_SHUTDOWN | \
					WPSM_WKSP_FLAG_SET_RESTART | \
					WPSM_WKSP_FLAG_RESTART_WL)

#define WPSM_CHILD_MAX_WAIT_SEC		5

enum {
	WPS_EAP_ID_ENROLLEE = 0,
	WPS_EAP_ID_REGISTRAR,
	WPS_EAP_ID_NONE
} WPS_EAP_ID_T;

enum {
	WPS_RECEIVE_PKT_UI = 1,
	WPS_RECEIVE_PKT_PB,
	WPS_RECEIVE_PKT_EAP,
#ifdef MULTIAP
	WPS_RECEIVE_PKT_1905,
#endif /* MULTIAP */
	WPS_RECEIVE_PKT_UPNP,
	WPS_RECEIVE_PKT_NFC
} WPS_RECEIVE_PKT_T;

typedef struct {
	void *wksp; /* it may wps_ap or wps_enr */
	int sc_mode; /* it used both for wps_ap or wps_enr case */
	int (*open)(void *, void *);
	int (*close)(void *);
	int (*process)(void *, char *, int, int);
	int (*check_timeout)(void *);
#if defined(MULTIAP)
	int (*map_timeout)(void *);
#endif	/* MULTIAP */
} wps_app_t;

/* Macros */
#define WPS_SMODE2STR(smode)	((smode) == SCMODE_AP_ENROLLEE? "ap enrollee" : \
				(smode) == SCMODE_AP_REGISTRAR ? "ap registrar" : \
				"Error!! known mode")

#define	WPS_IS_PROXY(mode)	((mode == SCMODE_AP_REGISTRAR) && \
				strcmp(wps_ui_get_env("wps_sta_pin"), "") == 0)

#define WPS_WLAKM_BOTH(akm) ((akm & WPA_AUTH_PSK) && (akm & WPA2_AUTH_PSK))
#define WPS_WLAKM_PSK2(akm) ((akm & WPA2_AUTH_PSK))
#define WPS_WLAKM_PSK(akm) ((akm & WPA_AUTH_PSK))
#define WPS_WLAKM_NONE(akm) (!(WPS_WLAKM_BOTH(akm) | WPS_WLAKM_PSK2(akm) | WPS_WLAKM_PSK(akm)))

#define WPS_WLENCR_BOTH(wsec) ((wsec & TKIP_ENABLED) && (wsec & AES_ENABLED))
#define WPS_WLENCR_TKIP(wsec) (wsec & TKIP_ENABLED)
#define WPS_WLENCR_AES(wsec) (wsec & AES_ENABLED)

#if defined(IL_BIGENDIAN)
#include <bcmendian.h>
#define htod32(i) (bcmswap32(i))
#define htod16(i) (bcmswap16(i))
#define dtoh32(i) (bcmswap32(i))
#define dtoh16(i) (bcmswap16(i))
#define htodchanspec(i) htod16(i)
#define dtohchanspec(i) dtoh16(i)
#else
#define htod32(i) i
#define htod16(i) i
#define dtoh32(i) i
#define dtoh16(i) i
#define htodchanspec(i) i
#define dtohchanspec(i) i
#endif // endif

/* NVRAM values for different types of multiap bss */
#if defined(MULTIAP)
#define	WPS_NVVAL_MAP_FH_BSS	1	/* Fronthaul BSS */
#define	WPS_NVVAL_MAP_BH_BSS	2	/* Backhaul BSS */
#define	WPS_NVVAL_MAP_BH_STA	4	/* Backhaul STA */
#endif	/* MULTIAP */

void wps_osl_restart_wl();

/* Common APIs */
void wps_stophandler(int sig);
void wps_restarthandler(int sig);
int wps_mainloop(int num, char **list);
void wps_conf_upd(int num, char **list);
void wps_close_session();
bool wps_is_wps_sta(char *wps_ifname);
wps_app_t *get_wps_app();
unsigned char *wps_get_uuid();
int wps_get_ess_num();
#ifdef WPS_ADDCLIENT_WWTP
void wps_close_addclient_window();
#endif // endif

void wps_setWPSSuccessMode(int state);

/* Common interface to ap wksp, WSC 2.0 */
int wpsap_open_session(wps_app_t *wps_app, int sc_mode, unsigned char *mac, unsigned char *mac_sta,
	char *osifname, char *enr_nonce, char *priv_key, uint8 *authorizedMacs,
	uint32 authorizedMacs_len, bool b_reqToEnroll, bool b_nwKeyShareable);

/* Common interface to sta wksp */
int wpssta_open_session(wps_app_t *wps_app, char*ifname);
#if defined(MULTIAP)
unsigned long wpssta_get_start_time();
void wpssta_set_start_time(unsigned long time);
unsigned char *wps_map_get_settings();
void wps_map_do_configure();
int wps_map_eap_init();
void wps_map_eap_dinit();
int wps_map_escan_handler();
#endif	/* MULTIAP */
#ifdef BCMWPSAPSTA
void wps_init_escan_bss_results(char *ifname, int idx);
#endif // endif
void wps_custom_init();
void wps_custom_deinit();
#endif /* __WPS_H__ */
