/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
/*
 * ASUS Home Gateway Reference Design
 * Web Page Configuration Support Routines
 *
 * Copyright 2004, ASUSTeK Inc.
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 */

#ifdef WEBS
#include <webs.h>
#include <uemf.h>
#include <ej.h>
#else /* !WEBS */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <httpd.h>
#endif /* WEBS */
#include <typedefs.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <shutils.h>
#include <qca.h>
#include <iwlib.h>
//#include <stapriv.h>
#include <ethutils.h>
#include <shared.h>
#include <sys/mman.h>
#ifndef O_BINARY
#define O_BINARY 	0
#endif
#ifndef MAP_FAILED
#define MAP_FAILED (-1)
#endif

#define wan_prefix(unit, prefix)	snprintf(prefix, sizeof(prefix), "wan%d_", unit)
//static char * rfctime(const time_t *timep);
//static char * reltime(unsigned int seconds);
void reltime(unsigned int seconds, char *buf);
static int wl_status(int eid, webs_t wp, int argc, char_t **argv, int unit);

#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/klog.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <net/if_arp.h>

#include <dirent.h>

typedef struct _WPS_CONFIGURED_VALUE {
	unsigned short 	Configured;	// 1:un-configured/2:configured
	char		BSSID[18];
	char 		SSID[32 + 1];
	char		AuthMode[16];	// Open System/Shared Key/WPA-Personal/WPA2-Personal/WPA-Enterprise/WPA2-Enterprise
	char 		Encryp[8];	// None/WEP/TKIP/AES
	char 		DefaultKeyIdx;
	char 		WPAKey[64 + 1];
} WPS_CONFIGURED_VALUE;

/* shared/sysdeps/api-qca.c */
extern u_int ieee80211_mhz2ieee(u_int freq);
extern int get_channel_list_via_driver(int unit, char *buffer, int len);
extern int get_channel_list_via_country(int unit, const char *country_code, char *buffer, int len);
extern int get_channel(const char *ifname);

#define WL_A		(1U << 0)
#define WL_B		(1U << 1)
#define WL_G		(1U << 2)
#define WL_N		(1U << 3)
#define WL_AC		(1U << 4)
#define WL_AD		(1U << 5)
#define WL_AX		(1U << 6)
#define WL_AY		(1U << 7)
#define WL_BE		(1U << 8)

static const char *g_mac_title = "MAC address";
#if defined(RTCONFIG_MLO)
static const char *g_mld_mac_title = "MLD address";
#else
static const char *g_mld_mac_title = g_mac_title;
#endif

static const struct mode_s {
	unsigned int mask;
	char *mode;
} mode_tbl[] = {
	{ WL_A,	"a" },
	{ WL_B,	"b" },
	{ WL_G, "g" },
	{ WL_N, "n" },
	{ WL_AC, "ac" },
	{ WL_AD, "ad" },
	{ WL_AX, "ax" },
	{ WL_AY, "ay" },
	{ WL_BE, "be" },
	{ 0, NULL },
};

#define OP_MODE_AP		"AP"
#define OP_MODE_WDS_ONLY	"WDS Only"
#define OP_MODE_HYBRID		"Hybrid"
#define OP_MODE_MB		"Media Bridge"
#if defined(RTCONFIG_CONCURRENTREPEATER)
#define OP_MODE_RP		"Express Way 2.4 GHz"
#else
#define OP_MODE_RP		"Repeater"
#endif
#define OP_MODE_MLO_BH		"backhaul"

#if defined(RTCONFIG_MLO)
static int show_mlo_bh_info(webs_t wp);
#else
static inline int show_mlo_bh_info(webs_t wp) { return 0; }
#endif

static int g_max_c1data_len = 0;

static void getWPSConfig(int unit, WPS_CONFIGURED_VALUE *result)
{
	char buf[128];
	FILE *fp;
	char wif[IFNAMSIZ] = "";
	char *wlxy = NULL;
	memset(result, 0, sizeof(*result));

#if defined(RTCONFIG_MULTILAN_MWL)
	wlxy = get_sdntype_iface("MAINFH", unit, wif, sizeof(wif));
	if (wlxy)
		free(wlxy);
	if (!wlxy || *wif == '\0')
		return;
#else
        strlcpy(wif, get_wifname(unit), sizeof(wif));
#endif

        snprintf(buf, sizeof(buf), "hostapd_cli -i%s get_config", wif);
	fp = popen(buf, "r");
	if (fp) {
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			char *pt1, *pt2;

			chomp(buf);
			//BSSID
			if ((pt1 = strstr(buf, "bssid="))) {
				pt2 = pt1 + safe_strlen("bssid=");
				strlcpy(result->BSSID, pt2, sizeof(result->BSSID));
			}
			//SSID
			if ((pt1 = strstr(buf, "ssid="))) {
				pt2 = pt1 + safe_strlen("ssid=");
				strlcpy(result->SSID, pt2, sizeof(result->SSID));
			}
			//Configured
			else if ((pt1 = strstr(buf, "wps_state="))) {
				pt2 = pt1 + safe_strlen("wps_state=");
				if (!strcmp(pt2, "configured") ||
				    (!strcmp(pt2, "disabled") && nvram_get_int("w_Setting"))
				   )
					result->Configured = 2;
				else
					result->Configured = 1;
			}
			//WPAKey
			else if ((pt1 = strstr(buf, "passphrase="))) {
				pt2 = pt1 + safe_strlen("passphrase=");
				strlcpy(result->WPAKey, pt2, sizeof(result->WPAKey));
			}
			//AuthMode
			else if ((pt1 = strstr(buf, "key_mgmt="))) {
				pt2 = pt1 + safe_strlen("key_mgmt=");
				strlcpy(result->AuthMode, pt2, sizeof(result->AuthMode));/* FIXME: NEED TRANSFORM CONTENT */
			}
			//Encryp
			else if ((pt1 = strstr(buf, "rsn_pairwise_cipher="))) {
				pt2 = pt1 + safe_strlen("rsn_pairwise_cipher=");
				if (!strcmp(pt2, "NONE"))
					strlcpy(result->Encryp, "None", sizeof(result->Encryp));
				else if (!strncmp(pt2, "WEP", 3))
					strlcpy(result->Encryp, "WEP", sizeof(result->Encryp));
				else if (!strcmp(pt2, "TKIP"))
					strlcpy(result->Encryp, "TKIP", sizeof(result->Encryp));
				else if (!strncmp(pt2, "CCMP", 4))
					strlcpy(result->Encryp, "AES", sizeof(result->Encryp));
				else if (unit == WL_60G_BAND && !strncmp(pt2, "GCMP", 4))
					strlcpy(result->Encryp, "AES", sizeof(result->Encryp));
			}
		}
		pclose(fp);
	}
	//dbg("%s: SSID[%s], Configured[%d], WPAKey[%s], AuthMode[%s], Encryp[%s]\n", __FUNCTION__, result->SSID, result->Configured, result->WPAKey, result->AuthMode, result->Encryp);
}

/**
 * Convert WL_XXX bit masks to string via mode_tbl[]
 * @mask:
 * @return:
 */
static char *mode_mask_to_str(unsigned int mask)
{
	static char result[sizeof("11b/g/nXXXXXXXXXXXX")] = "";
	const struct mode_s *q;
	char *p, *sep;
	size_t len, l;
	int noax = 0;

#if defined(RTCONFIG_WIFI_QCN5024_QCN5054) \
 || defined(RTCONFIG_QCA_AXCHIP) \
 || defined(RTCONFIG_QCA_BECHIP)
	if (!find_word(nvram_safe_get("rc_support"), "11AX"))
		noax = 1;
#endif

	p = result;
	len = sizeof(result);
	*p = '\0';
	sep = "11";
	for (q = &mode_tbl[0]; len > 0 && mask > 0 && q->mask; ++q) {
		if (!(mask & q->mask))
			continue;

		if (q->mask == WL_AX && noax)
			continue;

		mask &= ~q->mask;
		strlcat(p, sep, len);
		l = safe_strlen(sep);
		p += l;
		len -= l;
		strlcat(p, q->mode, len);
		l = safe_strlen(q->mode);
		p += l;
		len -= l;
		sep = "/";
	}

	return result;
}

/**
 * Get phy mode via Wireless Extension ioctl of QCA WiFi 10.2/10.4 driver.
 * @iface:
 * @return:
 */
char *__getAPPhyModebyIface(const char *iface, int max)
{
	char *mode, *puren, *pure11ax;
	unsigned int m = 0;
	int sta = 0, unit __attribute__((unused)) = -1;
	int dis_legacy = 0;

	if (!iface || *iface == '\0')
		return "";
	mode = iwpriv_get(iface, "get_mode");
	if (!mode)
		return "";

	/* Ref to phymode_strings of qca-wifi driver. */
	if (!strcmp(mode, "11A") || !strcmp(mode, "TA"))
		m = WL_A;
	else if (!strcmp(mode, "11G") || !strcmp(mode, "TG"))
		m = WL_G | WL_B;
	else if (!strcmp(mode, "11B"))
		m = WL_B;
	else if (!strncmp(mode, "11NA", 4))
		m = max? WL_N : WL_N | WL_A;
	else if (!strncmp(mode, "11NG", 4))
		m = max? WL_N : WL_N | WL_G | WL_B;
	else if (!strncmp(mode, "11ACVHT", 7))
		m = max? WL_AC : WL_AC | WL_N | WL_A;
	else if (!strncmp(mode, "11AHE", 5))
		m = max? WL_AX : WL_AX | WL_AC | WL_N | WL_A;
	else if (!strncmp(mode, "11GHE", 5))
		m = max? WL_AX : WL_AX | WL_N | WL_G | WL_B;
	else if (!strncmp(mode, "11GEHT", 6))
		m = max? WL_BE : WL_BE | WL_AX | WL_N | WL_G | WL_B;
	else if (!strncmp(mode, "11AEHT", 6))
		m = max? WL_BE : WL_BE | WL_AX | WL_AC | WL_N | WL_A;
	else if (!strncmp(mode, "AUTO", 4)) {
		if (is_sta_ifname(iface)) {
			int band = get_sta_ifname_unit(iface);

			sta = 1;
			if (band == WL_2G_BAND)
				m = max? WL_N : WL_N | WL_G | WL_B;
			else
				m = max? WL_AC : WL_AC | WL_N | WL_A;

#if defined(RTCONFIG_WIFI_QCN5024_QCN5054) \
 || defined(RTCONFIG_QCA_AXCHIP) \
 || defined(RTCONFIG_QCA_BECHIP)
			m = max? WL_AX : m | WL_AX;
#endif
#if defined(RTCONFIG_QCA_BECHIP)
			m = max? WL_BE : m | WL_BE;
#endif
		} else {
			dbg("%s: Unknown interface [%s] in AUTO mode\n", __func__, iface);
		}
	}
	else {
		dbg("%s: Unknown mode [%s]\n", __func__, mode);
	}

	/* If puren is enabled, remove a/g/b. */
	puren = iwpriv_get(iface, "get_puren");
	if (!sta && puren && safe_atoi(puren) && (m & WL_N))
		m &= ~(WL_A | WL_B | WL_G);

	/* If pure11ax is enabled, remove a/g/b/n/ac. */
	pure11ax = iwpriv_get(iface, "get_pure11ax");
	if (!sta && pure11ax && safe_atoi(pure11ax) && (m & WL_AX))
		m &= ~(WL_A | WL_B | WL_G | WL_N | WL_AC);

	if (is_vap_ifname(iface))
		unit = get_vap_ifname_unit(iface);
	else if (is_sta_ifname(iface))
		unit = get_sta_ifname_unit(iface);

	if (unit == WL_2G_BAND
	 && !iwpriv_get_int(iface, "g_dis_legacy", &dis_legacy)
	 && dis_legacy == 15)
		m &= ~WL_B;

	if (is_6g(unit))
		m &= ~(WL_A | WL_N | WL_AC);

	return mode_mask_to_str(m);
}

static inline char *getAPPhyModebyIface(const char *iface) { return __getAPPhyModebyIface(iface, 0); }

/**
 * Get phy mode via nl80211 of 802.11ad Wigig driver.
 * @ifname:
 * @return:
 */
char *getAPPhyModebyIfaceIW(const char *ifname)
{
	static char result[sizeof("11a/b/g/n/ac/adXXXXXX")] = "";
	unsigned int m = 0;
	char cmd[sizeof("iwinfo wlan0 info") + IFNAMSIZ];
#if defined(RTAD7200)
	const unsigned int default_11ad_mask = WL_AD;
#else
	const unsigned int default_11ad_mask = WL_AD | WL_AY;
#endif

	if (!ifname || *ifname == '\0') {
		dbg("%s: got invalid ifname %p\n", __func__, ifname);
		return 0;
	}

	/* Example:
	 * wlan0     ESSID: "OpenWrt_11ad"
	 *           Access Point: 04:CE:14:0A:21:17
	 *           Mode: Master  Channel: 3 (62.640 GH)
	 *           Tx-Power: unknown  Link Quality: 0/100
	 *           Signal: unknown  Noise: unknown
	 *           Bit Rate: unknown
	 *           Encryption: none
	 *           Type: nl80211  HW Mode(s): 802.11/ad
	 *           Hardware: 1AE9:0310 1AE9:0000 [Generic MAC80211]
	 *           TX power offset: unknown
	 *           Frequency offset: unknown
	 *           Supports VAPs: no
	 *           Beacon Interval: 100
	 */
	snprintf(cmd, sizeof(cmd), "iwinfo %s info", ifname);
	if (exec_and_parse(cmd, "HW Mode", "%*[^:]:%*[^:]: %s", 1, result))
		*result = '\0';

	if (!strcmp(result, "802.11ad"))
		m = default_11ad_mask;
	else
		dbg("%s: unknown phy mode string [%s]\n", __func__, result);

	return mode_mask_to_str(m);
}

char *getAPPhyMode(int unit)
{
	char *r = "";

	if (unit < 0 || unit >= MAX_NR_WL_IF || __absent_band(unit))
		return "";

	switch (unit) {
	case WL_2G_BAND:	/* fall-through */
	case WL_5G_BAND:	/* fall-through */
	case WL_5G_2_BAND:	/* fall-through */
	case WL_6G_BAND:	/* fall-through */
	case WL_6G_2_BAND:
		r = getAPPhyModebyIface(get_wifname(unit));
		break;
	case WL_60G_BAND:
		r = getAPPhyModebyIfaceIW(get_wifname(unit));
		break;
	default:
		dbg("%s: unknown wl%d band!\n", __func__, unit);
	}

	return r;
}

static unsigned int getAPChannelbyIWInfo(const char *ifname)
{
	int r;
	unsigned int freq = 0, ch = 0;
	char buf[256];

	if (!ifname || *ifname == '\0') {
		dbg("%s: got invalid ifname %p\n", __func__, ifname);
		return 0;
	}

	/* FIXME:
	 * I can't find any nl80211 based command that can be used to get channel from 11ad interface.
	 */
	/* Example: /sys/kernel/debug/ieee80211/phy0/wil6210/freq
	 * Freq = 60480
	 */
	r = f_read_string("/sys/kernel/debug/ieee80211/phy2/wil6210/freq", buf, sizeof(buf));
	if (r < safe_strlen("Freq = xxxxx"))
		return 0;

	r = sscanf(buf, "Freq = %u", &freq);
	if (r != 1)
		return 0;

	ch = ieee80211_mhz2ieee(freq);

	return ch;
}

unsigned int getAPChannel(int unit)
{
	int r = 0;

	if (__absent_band(unit))
		return 0;

	switch (unit) {
	case WL_2G_BAND:	/* fall-through */
	case WL_5G_BAND:	/* fall-through */
	case WL_5G_2_BAND:	/* fall-through */
	case WL_6G_BAND:	/* fall-through */
	case WL_6G_2_BAND:
		r = get_channel(get_wifname(unit));
		break;
	case WL_60G_BAND:
		/* FIXME */
		r = getAPChannelbyIWInfo(get_wifname(unit));
		break;
	default:
		dbg("%s: Unknown wl%d band!\n", __func__, unit);
	}

	return r;
}

#if defined(GTAXY16000)
unsigned int getEDMGChannel(void)
{
	int edmg_channel = 0;
	char cmd[sizeof("hostapd_cli -i XXXX status") + IFNAMSIZ];

	/* Example:
	 * / # hostapd_cli -i wlan0 status
	 * state=ENABLED
	 * phy=wlan0
	 * freq=60480
	 * ......
	 * channel=2
	 * edmg_enable=1
	 * edmg_channel=10
	 * ......
	 */
	snprintf(cmd, sizeof(cmd), "hostapd_cli -i %s status", get_wififname(WL_60G_BAND));
	if (exec_and_parse(cmd, "edmg_channel", "%*[^=]=%d", 1, &edmg_channel))
		edmg_channel = 0;

	return edmg_channel;
}
#endif

static int __getAPBitRate(const char *ifname, char *buf, size_t buf_len)
{
	int r;
	FILE *fp;
	char cmd[sizeof("iwconfig athXYYYYYY")], line[256], rate[16] = {0}, unit[16] = {0};

	if (!ifname || *ifname == '\0' || !buf || !buf_len) {
		dbg("%s: got invalid ifname %s buf %p buf_len %zu\n",
			__func__, ifname? : "NULL", buf, buf_len);
		return 0;
	}

	strlcpy(buf, "N/A", buf_len);
	snprintf(cmd, sizeof(cmd), "iwconfig %s", ifname);
	fp = popen(cmd, "r");
	if (!fp)
		return 0;

	/* Example:
	 * / # iwconfig ath0
	 * ath0      IEEE 802.11axa  ESSID:"ASUS_00_5G"
	 *           Mode:Master  Frequency:5.745 GHz  Access Point: 00:03:7F:12:B3:B3
	 *           Bit Rate:4.8039 Gb/s   Tx-Power:40 dBm
	 *           RTS thr:off   Fragment thr:off
	 *           Encryption key:BEA0-1288-6B95-B8A6-BDB4-4AF3-3248-2244   Security mode:restricted
	 *           Power Management:off
	 *           Link Quality=94/94  Signal level=-107 dBm  Noise level=-105 dBm
	 *           Rx invalid nwid:3287  Rx invalid crypt:0  Rx invalid frag:0
	 *           Tx excessive retries:0  Invalid misc:0   Missed beacon:0
	 */
	while (fgets(line, sizeof(line), fp)) {
		if (!strstr(line, "Bit Rate"))
			continue;
		if ((r = sscanf(line, "%*[^:]:%15s %15s", rate, unit)) != 2) {
			_dprintf("%s: Unknown bit rate of ifname [%s]: [%s]\n",
				__func__, ifname, line);
			continue;
		}
		break;
	}
	pclose(fp);

	if (rate[0] == '\0' || unit[0] == '\0') {
		*buf = '\0';
	} else {
		snprintf(buf, buf_len, "%s %s", rate, unit);
	}

	return 0;
}

#if defined(RTCONFIG_WIGIG)
static int __getAPBitRateIW(int band, const char *ifname, char *buf, size_t buf_len)
{
	int ratio = 1, mcs, edmg_enable, edmg_channel;
	char cmd[sizeof("hostapd_cli -i XXXX status") + IFNAMSIZ];
	float rate[] = {
		27.5, 385, 770, 962.5,						/* MCS0~3  Mb/s */
		1.155, 1.25125, 1.54, 1.925, 2.31, 2.5025, 3.08, 3.85, 4.62	/* MCS4~12 Gb/s */
	};

	if (band < 0 || band >= WL_NR_BANDS || absent_band(band)
	 || !ifname || *ifname == '\0' || !buf || !buf_len)
	{
		dbg("%s: got invalid ifname,buf,buf_len %p,%p,%d\n",
			__func__, ifname, buf, buf_len);
		return 0;
	}

	/* Example:
	 * / # iw phy phy0 info
	 * Wiphy phy0
	 *       max # scan SSIDs: 1
	 *       max scan IEs length: 1024 bytes
	 *       max # sched scan SSIDs: 0
	 *       max # match sets: 0
	 *       max # scan plans: 1
	 *       max scan plan interval: -1
	 *       max scan plan iterations: 0
	 *       Retry short limit: 7
	 *       Retry long limit: 4
	 *       Coverage class: 0 (up to 0m)
	 *       Available Antennas: TX 0 RX 0
	 *       Supported interface modes:
	 *                * managed
	 *                * AP
	 *                * monitor
	 *                * P2P-client
	 *                * P2P-GO
	 *                * P2P-device
	 *       Band 3:
	 *               Capabilities: 0x00
	 *                       HT20
	 *                       Static SM Power Save
	 *                       No RX STBC
	 *                       Max AMSDU length: 3839 bytes
	 *                       No DSSS/CCK HT40
	 *               Maximum RX AMPDU length 65535 bytes (exponent: 0x003)
	 *               Minimum RX AMPDU time spacing: 8 usec (0x06)
	 *               HT TX/RX MCS rate indexes supported: 1-12
	 *               Frequencies:
	 *                       * 58320 MHz [1] (40.0 dBm)
	 *                       * 60480 MHz [2] (40.0 dBm)
	 *                       * 62640 MHz [3] (40.0 dBm)
	 *                       * 64800 MHz [4] (40.0 dBm)
	 *       interface combinations are not supported
	 */
	snprintf(cmd, sizeof(cmd), "iw phy %s info", get_vphyifname(band));
	if (exec_and_parse(cmd, "MCS", "%*[^-]-%d", 1, &mcs))
		mcs = 12;

	if (mcs >= 12) {
		/* Example:
		 * /# hostapd_cli -i wlan0 status
		 * state=ENABLED
		 * phy=wlan0
		 * freq=60480
		 * ......
		 * channel=2
		 * edmg_enable=0
		 * edmg_channel=0
		 * ......
		 */
		snprintf(cmd, sizeof(cmd), "hostapd_cli -i %s status", get_wififname(WL_60G_BAND));
		if (exec_and_parse(cmd, "edmg_enable", "%*[^=]=%d", 1, &edmg_enable))
			edmg_enable = 0;
		if (edmg_enable) {
			if (exec_and_parse(cmd, "edmg_channel", "%*[^=]=%d", 1, &edmg_channel))
				edmg_channel = 0;
			if (edmg_channel >= 9 && edmg_channel <= 13)
				ratio = 2;
			else if (edmg_channel >= 17 && edmg_channel <= 20)
				ratio = 3;
			else if (edmg_channel >= 25 && edmg_channel <= 27)
				ratio = 4;
		}
	}

	*buf = '\0';
	if (mcs >= 0 && mcs < ARRAY_SIZE(rate))
		snprintf(buf, buf_len, "%.2f %s", ratio * rate[mcs], (mcs >= 4)? "Gb/s" : "Mb/s");

	return 0;
}
#endif

static int __getAPBandwidth(const char *ifname, char *buf, size_t buf_len)
{
#if defined(RTCONFIG_WIFI7)
	int band = -1;
#endif
	char *m, mode[32] = "";

	if (!ifname || !buf || !buf_len)
		return -1;

	*buf = '\0';
	m = iwpriv_get(ifname, "get_mode");
	if (!m)
		return -1;

	/* Ref to phymode_strings of qca-wifi driver.
	 * "AUTO", "11A", "11B", "11G", "FH", "TA", "TG",
	 * "11NGHT20", "11NAHT20", "11ACVHT20", "11AXA_HE20", "11AXG_HE20", "11GEHT20",
	 * "11NGHT40PLUS", "11NGHT40MINUS", "11NGHT40", "11AXG_HE40MINUS", "11AXG_HE40",
	 * "11GEHT40PLUS", "11GEHT40MINUS", "11GEHT40",
	 * "11AEHT20",
	 * "11NAHT40PLUS", "11NAHT40MINUS", "11NAHT40",
	 * "11ACVHT40PLUS", "11ACVHT40MINUS", "11ACVHT40",
	 * "11AXA_HE40PLUS", "11AXA_HE40MINUS", "11AXG_HE40PLUS", "11AXA_HE40",
	 * "11AEHT40PLUS", "11AEHT40MINUS", "11AEHT40"
	 * "11ACVHT80", "11AXA_HE80", "11AEHT80"
	 * "11ACVHT160", "11AXA_HE160", "11AEHT160"
	 * "11ACVHT80_80", "11AXA_HE80_80"
	 * "11AEHT320"
	 */
	strlcpy(mode, m, sizeof(mode));
	if (strstr(mode, "EHT320")) {
		get_wlif_unit(ifname, &band, NULL);
		if (is_5g(band))
			strlcpy(buf, "240", buf_len);
		else
			strlcpy(buf, "320", buf_len);
	}
	else if (strstr(mode, "EHT240"))
		strlcpy(buf, "240", buf_len);
	else if (strstr(mode, "HE160") || strstr(mode, "VHT160") || strstr(mode, "EHT160"))
		strlcpy(buf, "160", buf_len);
	else if (strstr(mode, "HE80_80") || strstr(mode, "VHT80_80"))
		strlcpy(buf, "80+80", buf_len);
	else if (strstr(mode, "HE80") || strstr(mode, "VHT80") || strstr(mode, "EHT80"))
		strlcpy(buf, "80", buf_len);
	else if (strstr(mode, "HT40") || strstr(mode, "HE40") || strstr(mode, "EHT40"))
		strlcpy(buf, "40", buf_len);
	else if (strstr(mode, "HT20") || strstr(mode, "HE20") || strstr(mode, "EHT20")
	     ||  !strcmp(mode, "AUTO") || !strcmp(mode, "11A") || !strcmp(mode, "11B")
	     ||  !strcmp(mode, "11G") || !strcmp(mode, "FH") || !strcmp(mode, "TA") || !strcmp(mode, "TG"))
		strlcpy(buf, "20", buf_len);
	else {
		dbg("%s: Unknown mode [%s]\n", __func__, mode);
		strlcpy(buf, "20", buf_len);
	}

	return 0;
}

static void getVAPBandwidth(int unit, const char *ifname, char *buf, size_t buf_len)
{
	char *rate = "N/A";

	if (!buf || !buf_len || __absent_band(unit))
		return;

	strlcpy(buf, rate, buf_len);
	switch (unit) {
	case WL_2G_BAND:	/* fall-through */
	case WL_5G_BAND:	/* fall-through */
	case WL_5G_2_BAND:	/* fall-through */
	case WL_6G_BAND:
		__getAPBandwidth(ifname, buf, buf_len);
		break;
#if defined(RTCONFIG_WIGIG)
	case WL_60G_BAND:
		/* FIXME */
		*buf = '\0';
		break;
#endif
	default:
		dbg("%s: Unknown wl%d band!\n", __func__, unit);
	}
}

/**
 * Return SSID of a interface.
 * @return:	Don't return NULL even interface name is invalid or interface absent.
 */
char* getSSIDbyIFace(int unit, const char *ifname)
{
	static char ssid[33] = "";
	char buf[8192] = "";
	FILE *fp;
	int len;
	char *pt1, *pt2, *pt3;

	if (unit < 0 || unit >= MAX_NR_WL_IF || __absent_band(unit))
		return ssid;

	if (!ifname || *ifname == '\0') {
		dbg("%s: got invalid ifname %p\n", __func__, ifname);
		return ssid;
	}

#if defined(RTCONFIG_WIGIG)
	if (unit == WL_60G_BAND) {
#if defined(RTAD7200)
		snprintf(buf, sizeof(buf), "/sys/kernel/debug/ieee80211/%s/wil6210/ssid", get_vphyifname(unit));
		f_read_string(buf, ssid, sizeof(ssid));
#elif defined(GTAXY16000)
		char cmd[sizeof("iw wlan0 info") + IFNAMSIZ];

		snprintf(cmd, sizeof(cmd), "iw %s info", get_wififname(unit));
		if (exec_and_parse(cmd, "ssid", "%*s %[^\n]", 1, ssid))
			*ssid = '\0';
#else
#error FIXME: Get SSID
#endif
	} else
#endif	/* RTCONFIG_WIGIG */
	{
		snprintf(buf, sizeof(buf), "iwconfig %s", ifname);
		if (!(fp = popen(buf, "r")))
			return ssid;

		len = fread(buf, 1, sizeof(buf), fp);
		pclose(fp);
		if (len <= 0)
			return ssid;

		buf[len] = '\0';
		pt1 = strstr(buf, "ESSID:");
		if (!pt1)
			return ssid;

		pt2 = pt1 + safe_strlen("ESSID:") + 1;	/* skip leading " */
		pt1 = strchr(pt2, '\n');
		if (!pt1 || (pt1 - pt2) <= 1)
			return ssid;

		/* Remove trailing " */
		*pt1 = '\0';
		pt3 = strrchr(pt2, '"');
		if (pt3)
			*pt3 = '\0';

		strlcpy(ssid, pt2, sizeof(ssid));
	}

	return ssid;
}

int
ej_wl_control_channel(int eid, webs_t wp, int argc, char_t **argv)
{
        int ret = 0;
        int channel_24 = 0, channel_50 = 0;
	int channel_5G2, channel_60G;
        
	channel_24 = getAPChannel(0);
#if defined(RTCONFIG_LYRA_5G_SWAP)
#if defined(RTCONFIG_WIFI_SON)
	if(nvram_match("wifison_ready","1"))
	{
		channel_50 = getAPChannel(1);
		channel_5G2 = getAPChannel(2);
	}
	else
#endif
	{
		channel_50 = getAPChannel(2);
		channel_5G2 = getAPChannel(1);
	}
#else
	channel_50 = getAPChannel(1);
	channel_5G2 = getAPChannel(2);
#endif
	channel_60G = getAPChannel(3);

	ret = websWrite(wp, "[\"%d\", \"%d\", \"%d\", \"%d\"]",
		channel_24, channel_50, channel_5G2, channel_60G);
	
        return ret;
}

#if defined(GTAXY16000)
int
ej_wl_edmg_channel(int eid, webs_t wp, int argc, char_t **argv)
{
        int ret = 0, edmg_channel;

	edmg_channel = getEDMGChannel();
	ret = websWrite(wp, "[\"%d\", \"%d\", \"%d\", \"%d\"]", 0, 0, 0, edmg_channel);

        return ret;
}
#endif

long getSTAConnTime(char *ifname, char *bssid)
{
	char buf[8192];
	FILE *fp;
	int len;
	char *pt1,*pt2;

	snprintf(buf, sizeof(buf), "hostapd_cli -i%s sta %s", ifname, bssid);
	fp = popen(buf, "r");
	if (fp) {
		memset(buf, 0, sizeof(buf));
		len = fread(buf, 1, sizeof(buf), fp);
		pclose(fp);
		if (len > 1) {
			buf[len-1] = '\0';
			pt1 = strstr(buf, "connected_time=");
			if (pt1) {
				pt2 = pt1 + safe_strlen("connected_time=");
				chomp(pt2);
				return safe_atol(pt2);
			}
		}
	}
	return 0;
}

#if defined(RTCONFIG_MULTILAN_MWL)
static int __getSTAInfoSDN(int unit, WIFI_STA_TABLE *sta_info, char *ifname)
{
	int subunit;
	char subunit_str[4] = "0";
	const char *main_iface;
	int main_iface_len;

	if (absent_band(unit))
		return -1;
	if (!ifname || *ifname == '\0')
		return -1;

	main_iface = get_wififname(unit);
	main_iface_len = strlen(main_iface);
	if (memcmp(main_iface, ifname, main_iface_len)==0)
		subunit = atoi(ifname+main_iface_len);
	else
		subunit = 0;

	if (subunit >= 0 && subunit < MAX_NO_MSSID)
		snprintf(subunit_str, sizeof(subunit_str), "%d", subunit);

	return get_qca_sta_info_by_ifname(ifname, subunit_str[0], sta_info);
}
#else

/** Get client list via wlanconfig utility.
 * @unit:
 * @sta_info:
 * @ifname:
 * @subunit_id:
 * 	'B':	Facebook Wi-Fi
 * 	'F':	Free Wi-Fi
 * 	'C':	Captive Portal
 * otherwise:	Main or guest network.
 * @return:
 */
static int __getSTAInfo(int unit, WIFI_STA_TABLE *sta_info, char *ifname, char id)
{
	int subunit;
	char subunit_str[4] = "0", wlif[sizeof("wlX.Yxxx")];

	if (absent_band(unit))
		return -1;
	if (!ifname || *ifname == '\0')
		return -1;

	subunit = get_wlsubnet(unit, ifname);
	if (subunit < 0)
		subunit = 0;
	if (subunit >= 10) {
		dbg("%s: invalid subunit %d\n", __func__, subunit);
		return -2;
	}

	snprintf(wlif, sizeof(wlif), "wl%d.%d", unit, subunit);
	if (subunit >= 0 && subunit < MAX_NO_MSSID)
		snprintf(subunit_str, sizeof(subunit_str), "%d", subunit);
	if (id == 'B' || id == 'F' || id == 'C')
		snprintf(subunit_str, sizeof(subunit_str), "%c", id);
#if defined(RTCONFIG_WIFI7)
	if (is_iot_ifname(ifname))
		snprintf(subunit_str, sizeof(subunit_str), "I");
#endif

	return get_qca_sta_info_by_ifname(ifname, subunit_str[0], sta_info);
}
#endif

/** Get client list via iw utility.
 * @unit:
 * @sta_info:
 * @ifname:
 * @subunit_id:
 * 	'B':	Facebook Wi-Fi
 * 	'F':	Free Wi-Fi
 * 	'C':	Captive Portal
 * otherwise:	Main or guest network.
 * @return:
 */
static int __getSTAInfoIW(int unit, WIFI_STA_TABLE *sta_info, char *ifname, char id)
{
	FILE *fp;
	int c, subunit, time_val, hr, min, sec, rssi;
	char rate[6], line_buf[300];
	char subunit_str[4] = "0", wlif[sizeof("wlX.Yxxx")];
	char cmd[sizeof("iw wlan0 station dump XXXXXX")];
	WLANCONFIG_LIST *r;


	if (absent_band(unit))
		return -1;
	if (!ifname || *ifname == '\0')
		return -1;

	subunit = get_wlsubnet(unit, ifname);
	if (subunit < 0)
		subunit = 0;
	if (subunit >= 10) {
		dbg("%s: invalid subunit %d\n", __func__, subunit);
		return -2;
	}

	snprintf(wlif, sizeof(wlif), "wl%d.%d", unit, subunit);
	if (subunit >= 0 && subunit < MAX_NO_MSSID)
		snprintf(subunit_str, sizeof(subunit_str), "%d", subunit);
	if (id == 'B' || id == 'F' || id == 'C')
		snprintf(subunit_str, sizeof(subunit_str), "%c", id);

	snprintf(cmd, sizeof(cmd), "iw %s station dump", get_wififname(unit));
	fp = popen(cmd, "r");
	if (!fp)
		return -2;

	/* /sys/kernel/debug/ieee80211/phy0/wil6210/stations has client list too.
	 * But I guess none of any another attributes exist, e.g., connection time, exist.
	 * ILQ1.3.7 Example: iw wlan0 station dump
	 * Station 04:ce:14:0a:21:17 (on wlan0)
	 *       rx bytes:       0
	 *       rx packets:     0
	 *       tx bytes:       0
	 *       tx packets:     0
	 *       tx failed:      0
	 *       tx bitrate:     27.5 MBit/s MCS 0
	 *       rx bitrate:     27.5 MBit/s MCS 0
	 *       connected time: 292 seconds
	 * SPF10.0 FC Example: iw wlan0 station dump
	 *  Station 04:ce:14:0b:46:12 (on wlan0)
	 *        rx bytes:       0
	 *        rx packets:     0
	 *        tx bytes:       0
	 *        tx packets:     0
	 *        tx failed:      0
	 *        rx drop misc:   0
	 *        signal:         -55 dBm
	 *        tx bitrate:     27.5 MBit/s MCS 0
	 *        rx bitrate:     27.5 MBit/s MCS 0
	 */
	while (fgets(line_buf, sizeof(line_buf), fp)) {
		if (strncmp(line_buf, "Station", 7)) {
			continue;
		}

next_sta:
		r = &sta_info->Entry[sta_info->Num++];
		c = sscanf(line_buf, "Station %17[0-9a-f:] %*[^\n]", r->addr);
		if (c != 1) {
			continue;
		}
		convert_mac_string(r->addr);
		r->subunit_id = subunit_str[0];
		strlcpy(r->mode, "11ad", sizeof(r->mode));
		while (fgets(line_buf, sizeof(line_buf), fp)) {
			if (!strncmp(line_buf, "Station", 7)) {

#if 0
				dbg("[%s][%u][%u][%s][%s][%u][%s]\n",
					r->addr, r->aid, r->chan, r->txrate, r->rxrate, r->rssi, r->mode);
#endif
				goto next_sta;
			} else if (strstr(line_buf, "tx bitrate:")) {
				c = sscanf(line_buf, "%*[ \t]tx bitrate:%*[ \t]%6[0-9.]", rate);
				if (c != 1) {
					continue;
				}
				snprintf(r->txrate, sizeof(r->txrate), "%sM", rate);
			} else if (strstr(line_buf, "rx bitrate:")) {
				c = sscanf(line_buf, "%*[ \t]rx bitrate:%*[ \t]%6[0-9.]", rate);
				if (c != 1) {
					continue;
				}
				snprintf(r->rxrate, sizeof(r->rxrate), "%sM", rate);
			} else if (strstr(line_buf, "connected time:")) {
				c = sscanf(line_buf, "%*[ \t]connected time:%*[ \t]%d seconds", &time_val);
				if (c != 1) {
					continue;
				}
				hr = time_val / 3600;
				time_val %= 3600;
				min = time_val / 60;
				sec = time_val % 60;
				snprintf(r->conn_time, sizeof(r->conn_time), "%02d:%02d:%02d", hr, min, sec);
			} else if (strstr(line_buf, "signal:")) {
				c = sscanf(line_buf, "%*[ \t]signal:%*[ \t]%d dBm", &rssi);
				r->rssi = rssi;
			} else {
				//dbg("%s: skip [%s]\n", __func__, line_buf);
			}
		}
	}
	pclose(fp);

	return 0;
}

#if defined(RTCONFIG_CAPTIVE_PORTAL)
/**
 * List non standard guest network clients, e.g., Free Wi-Fi and Captive portal
 * @unit:
 * @ifnames:	6-th parameter of captive_portal or captive_portal_adv_profile
 * 		e.g.: "wl0.6wl1.6" minus double quotes
 * @sta_info:
 * @id:		The @id will be passed to __getSTAInfo() function.
 * 	'B':	Facebook Wi-Fi
 * 	'F':	Free Wi-Fi
 * 	'C':	Captive Portal
 *  otherwise:	Main 2G/5G network or guest network.
 * @return:
 */
static int getNonStdGuestSTAInfo(int unit, char *ifnames,
					WIFI_STA_TABLE * sta_info, char id)
{
	int i, u, s;
	char *p, *q, ifname[IFNAMSIZ];

	if (absent_band(unit) || !ifnames || strncmp(ifnames, "wl", 2) ||
	    safe_strlen(ifnames) < 5 || !sta_info)
		return -1;

	/* ifnames example: "wl0.6wl1.6", minus double quotes */
	for (u = -1, p = q = ifnames; u != unit && p != NULL; p = q) {
		q = strstr(p + 1, "wl");
		if (sscanf(p, "wl%d.%d", &u, &s) != 2 || u != unit)
			continue;
		break;
	}

	if (u != unit || p == NULL)
		return -2;
	for (i = 1, *ifname = '\0'; i < MAX_NO_MSSID; ++i) {
		__get_wlifname(unit, i, ifname);
		if (get_wlsubnet(unit, ifname) != s)
			continue;

		__getSTAInfo(unit, sta_info, ifname, id);
		break;
	}

	return 0;
}

/**
 * List Captive Portal clients.
 * @unit:
 * @sta_info:
 * @return:
 */
static int getCPortalSTAInfo(int unit, WIFI_STA_TABLE * sta_info)
{
	char *a[12], *nv, *nvp, *b;

	if (absent_band(unit) || !sta_info)
		return -1;

	/* Captive Portal */
	if (!nvram_match("captive_portal_adv_enable", "on"))
		return 0;

	nv = nvp = strdup(nvram_safe_get("captive_portal_adv_profile"));
	if (!nv)
		return 0;

	while ((b = strsep(&nvp, "<")) != NULL) {
		memset(a, 0, sizeof(a));
		if ((vstrsep
		     (b, ">", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6], &a[7],
		      &a[8], &a[9], &a[10], &a[11]) != 12))
			continue;

		getNonStdGuestSTAInfo(unit, a[5], sta_info, 'C');
	}
	free(nv);

	return 0;
}

/**
 * List Free Wi-Fi clients.
 * @unit:
 * @sta_info:
 * @return:
 */
static int getFreeWiFiSTAInfo(int unit, WIFI_STA_TABLE *sta_info)
{
	char *a[7], *nv, *nvp, *b;

	if (absent_band(unit) || !sta_info)
		return -1;

	/* Free Wi-Fi */
	if (!nvram_match("captive_portal_enable", "on"))
		return 0;

	nv = nvp = strdup(nvram_safe_get("captive_portal"));
	if (!nv)
		return 0;

	while ((b = strsep(&nvp, "<")) != NULL) {
		memset(a, 0, sizeof(a));
		if ((vstrsep
		     (b, ">", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6]) != 7))
			continue;

		getNonStdGuestSTAInfo(unit, a[5], sta_info, 'F');
	}
	free(nv);

	return 0;
}
#else
static inline int getCPortalSTAInfo(int unit, WIFI_STA_TABLE *sta_info) { return 0; }
static inline int getFreeWiFiSTAInfo(int unit, WIFI_STA_TABLE *sta_info) { return 0; }
#endif

#if defined(RTCONFIG_FBWIFI)
/**
 * List Facebook Wi-Fi clients.
 * @unit:
 * @sta_info:
 * @return:
 */
static int getFacebookWiFiSTAInfo(int unit, WIFI_STA_TABLE *sta_info)
{
	char *fbwifi_iface[] = { "fbwifi_2g", "fbwifi_5g", "fbwifi_5g_2" };

	if (absent_band(unit) || unit >= ARRAY_SIZE(fbwifi_iface)|| !sta_info)
		return -1;

	/* Facebook Wi-Fi */
	if (!nvram_match("fbwifi_enable", "on"))
		return 0;

	getNonStdGuestSTAInfo(unit, nvram_safe_get(fbwifi_iface[unit]), sta_info, 'B');

	return 0;
}
#else
static inline int getFacebookWiFiSTAInfo(int unit, WIFI_STA_TABLE *sta_info) { return 0; }
#endif

static int getSTAInfo(int unit, WIFI_STA_TABLE *sta_info)
{
	int ret = 0;
	char *unit_name;
	char *p, *ifname;
	char *wl_ifnames;
#if defined(RTCONFIG_FBWIFI)
	char *fbwifi_iface[] = { "fbwifi_2g", "fbwifi_5g", "fbwifi_5g_2" };
	char wl_ifname[IFNAMSIZ] = "", *wl_if = wl_ifname;
#endif

	if (!sta_info)
		return -1;

	memset(sta_info, 0, sizeof(*sta_info));
	if (unit < 0 || unit >= MAX_NR_WL_IF || absent_band(unit))
		return 0;

#if defined(RTCONFIG_FBWIFI)
	if (nvram_match("fbwifi_enable", "on") &&
	    unit >= 0 && unit < min(MAX_NR_WL_IF, ARRAY_SIZE(fbwifi_iface)))
	{
		int j;

		if (sscanf(nvram_safe_get(fbwifi_iface[unit]), "wl%*d.%d", &j) == 1)
			wl_if = get_wlxy_ifname(unit, j, wl_ifname);
	}
#endif
	unit_name = strdup(get_wifname(unit));
	if (!unit_name)
		return ret;
#if defined(RTCONFIG_AMAS_WGN) || defined(RTCONFIG_MULTILAN_CFG)
        wl_ifnames = get_all_lan_ifnames();
#else
	wl_ifnames = strdup(nvram_safe_get("lan_ifnames"));
#endif	
	if (!wl_ifnames) {
		free(unit_name);
		return ret;
	}
	p = wl_ifnames;
	while ((ifname = strsep(&p, " ")) != NULL) {
		while (*ifname == ' ') ++ifname;
		if (*ifname == 0) break;
		SKIP_ABSENT_FAKE_IFACE(ifname);
		if (strncmp(ifname, unit_name, safe_strlen(unit_name)))
			continue;

#if defined(RTCONFIG_FBWIFI)
		if (!strcmp(ifname, wl_if))
			continue;
#endif

		switch (unit) {
		case WL_2G_BAND:	/* fall-through */
		case WL_5G_BAND:	/* fall-through */
		case WL_5G_2_BAND:	/* fall-through */
		case WL_6G_BAND:
#if defined(RTCONFIG_MULTILAN_MWL)
			__getSTAInfoSDN(unit, sta_info, ifname);
#else
			__getSTAInfo(unit, sta_info, ifname, 0);
#endif
			break;
		case WL_60G_BAND:
			__getSTAInfoIW(unit, sta_info, ifname, 0);
			break;
		default:
			dbg("%s: unknown wl%d band!\n", __func__, unit);
		}
	}
	free(wl_ifnames);
	free(unit_name);

#if !defined(RTCONFIG_MULTILAN_MWL)
	getFacebookWiFiSTAInfo(unit, sta_info);
	getCPortalSTAInfo(unit, sta_info);
	getFreeWiFiSTAInfo(unit, sta_info);
#endif

	return ret;
}

#define MLS1WIDTH	17	/* MAC address */
#define MLS2WIDTH	12	/* PhyMode */
#define MLS3WIDTH	3	/* MLO */
#define MLS4WIDTH	7	/* Channel */
#define MLS5WIDTH	4	/* RSSI */
#define MLS6WIDTH	7	/* TX_RATE */
#define MLS7WIDTH	7	/* RX_RATE */
#define MLS8WIDTH	12	/* Connect Time */

#define VLS1WIDTH	16	/* ESS */
#define VLS2WIDTH	17	/* MAC address */
#define VLS3WIDTH	12	/* PhyMode */
#define VLS4WIDTH	4	/* MLO */
#define VLS5WIDTH	4	/* RSSI */
#define VLS6WIDTH	7	/* TX_RATE */
#define VLS7WIDTH	7	/* RX_RATE */
#define VLS8WIDTH	12	/* Connect Time */

#define STA1WIDTH	17	/* MAC address */
#define STA2WIDTH	18	/* Parent-AP BSSID */
#define STA3WIDTH	14	/* PHY Mode */
#define STA4WIDTH	7	/* Channel */
#define STA5WIDTH	9	/* Bandwidth */
#define STA6WIDTH	12	/* Bit Rate */

#define VAP1WIDTH	24	/* Title: OP Mode, SSID, PHY Mode (AP), Channel */
#define VAP2WIDTH	12	/* Title: MAC address, BSSID, Bit Rate (AP) */

static void calc_c1data_width(void)
{
	const int sw_mode __attribute__((unused)) = sw_mode();
	int l, u, su, max_mssid, mode_chk, mode_x;
	char pfix[sizeof("wlX.XXX_")], mode[64], vap[IFNAMSIZ];

	if (__mediabridge_mode(sw_mode))
		g_max_c1data_len = strlen(OP_MODE_MB);
	else if (__repeater_mode(sw_mode))
		g_max_c1data_len = strlen(OP_MODE_RP);
	else {
		g_max_c1data_len = strlen(OP_MODE_AP);
		for (u = 0; u < MAX_NR_WL_IF; ++u) {
			SKIP_ABSENT_BAND(u);

			l = 0;
			snprintf(pfix, sizeof(pfix), "wl%d_", u);
			mode_x = nvram_pf_get_int(pfix, "mode_x");
			if (mode_x == 1)
				l = strlen(OP_MODE_WDS_ONLY);
			else if (mode_x == 2)
				l = strlen(OP_MODE_HYBRID);
			if (l > g_max_c1data_len)
				g_max_c1data_len = l;
		}
	}

	/* Calculate maximum SSID length of VAPs on all bands. */
	for (u = 0; u < MAX_NR_WL_IF; ++u) {
		SKIP_ABSENT_BAND(u);

		/* athX */
		max_mssid = num_of_mssid_support(u);
		for (su = 0, mode_chk = 0; su <= max_mssid; ++su) {
			if (!mode_chk) {
				strlcpy(mode, getAPPhyMode(u), sizeof(mode));
				mode_chk = 1;
				l = strlen(mode);
				if (l > g_max_c1data_len)
					g_max_c1data_len = l;
			}
#if defined(RTCONFIG_MLO_BH)
			/* Skip SSID if it's backhaul. */
			if (!su && sw_mode != SW_MODE_REPEATER)
				continue;
#endif
#if defined(RTCONFIG_VIF_ONBOARDING)
			/* Skip SSID if it's dedicated to onboarding. */
			if (u == WL_2G_BAND && su == get_vif_subunit())
				continue;
#endif
			if (su == 0)
				snprintf(pfix, sizeof(pfix), "wl%d_", u);
			else
				snprintf(pfix, sizeof(pfix), "wl%d.%d_", u, su);
			if (!nvram_pf_match(pfix, "bss_enabled", "1"))
				continue;

			strlcpy(vap, nvram_pf_safe_get(pfix, "ifname"), sizeof(vap));
			if (*vap == '\0' || !iface_exist(vap))
				continue;
			l = strlen(getSSIDbyIFace(u, vap));
			if (l > g_max_c1data_len)
				g_max_c1data_len = l;
		}

		/* staX */
		strlcpy(vap, get_staifname(u), sizeof(vap));
		if (*vap == '\0' || !iface_exist(vap))
			continue;
		l = strlen(getSSIDbyIFace(u, vap));
		if (l > g_max_c1data_len)
			g_max_c1data_len = l;
	}
}

int
ej_wl_status(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
	int retval = 0;
	int ii = 0;
	char word[256], *next;

	calc_c1data_width();
	retval += show_mlo_bh_info(wp);
	foreach (word, nvram_safe_get("wl_ifnames"), next) {
		SKIP_ABSENT_BAND_AND_INC_UNIT(ii);
		retval += wl_status(eid, wp, argc, argv, ii);
		retval += websWrite(wp, "\n");

		ii++;
	}

	return retval;
}

int
ej_wl_status_2g(int eid, webs_t wp, int argc, char_t **argv)
{
	return ej_wl_status(eid, wp, argc, argv, WL_2G_BAND);
}

/* Show bands of slave interfaces and stations list of downstream MLD interface.
 * @wp:
 * @ifname:		mldX, except MLD_STA.
 * @c2sep:
 * @return:		number of printed characters.
 */
static int __show_mldapiface_info(webs_t wp, const char *ifname, const char *c2sep)
{
	int i, unit, w, ret = 0;
	const char *q;
	char *p, *next;
	char vap[IFNAMSIZ], slave_ifaces[MAX_NR_WL_IF * IFNAMSIZ];
	char bandsname[MAX_NR_WL_IF * sizeof("2.4GHz/") + 4] = "";
	WIFI_STA_TABLE *sta_info;
	WLANCONFIG_LIST *l;
	char sep[MLS1WIDTH + 1 + 32 + 1 + MLS3WIDTH + 1 + MLS4WIDTH + 1 + MLS5WIDTH + 1 + MLS6WIDTH + 1 + MLS7WIDTH + 1 + MLS8WIDTH + 2];

	if (!wp || !ifname || !c2sep || *c2sep == '\0' || !strcmp(ifname, MLD_STA))
		return 0;

	strlcpy(slave_ifaces, mld_slave_ifaces(ifname), sizeof(slave_ifaces));
	foreach (vap, slave_ifaces, next) {
		unit = get_vap_ifname_unit(vap);
		q = get_band_longname(unit);
		if (!q || *q == '\0')
			continue;
		if (*bandsname != '\0')
			strlcat(bandsname, "/", sizeof(bandsname));
		strlcat(bandsname, q, sizeof(bandsname));
	}
	ret += websWrite(wp, "%-*s: %s\n", VAP1WIDTH, "Band", bandsname);

	w = sizeof(sep) - (32 - g_max_c1data_len);
	for (i = 0, p = sep; i < (sizeof(sep) - 1); ++i) {
		if (i >= w)
			break;
		*p++ = '-';
	}
	*p++ = '\0';
	ret += websWrite(wp, "\nStations List\n%s\n", sep);
	ret += websWrite(wp, "%-*s %-*s %-*s %-*s %-*s %-*s %-*s %-*s\n", MLS1WIDTH, "MAC",
		MLS2WIDTH, "PhyMode ", MLS3WIDTH, "MLO", MLS4WIDTH, "Channel", MLS5WIDTH, "RSSI",
		MLS6WIDTH, "TX_RATE", MLS7WIDTH, "RX_RATE", MLS8WIDTH, "Connect Time");

	if (!(sta_info = malloc(sizeof(*sta_info)))) {
		ret += websWrite(wp, "\n");
		return ret;
	}

	memset(sta_info, 0, sizeof(*sta_info));
	get_qca_sta_info_by_ifname(ifname, '\0', sta_info);
	for (i = 0, l = sta_info->Entry; i < sta_info->Num; i++, l++) {
		ret += websWrite(wp, "%-*s %-*s %-*s %-*u %-*d %-*s %-*s %-*s\n",
			MLS1WIDTH, l->addr, MLS2WIDTH, l->mode,
			MLS3WIDTH, (l->flags & WLFLAGS_MLO)? "Yes" : "No", MLS4WIDTH, l->chan,
			MLS5WIDTH, l->rssi, MLS6WIDTH, l->txrate, MLS7WIDTH, l->rxrate,
			MLS8WIDTH, l->conn_time);
	}
	free(sta_info);
	ret += websWrite(wp, "\n");

	return ret;
}

/* Show SSID, BSSID, Phy Mode, Bit Rate, and Channel of upstream interface.
 * If @ifname belongs to a upstream MLD interface, show same info of all slave interfaces.
 * @wp:
 * @ifname:		staX.
 * @mld_slave_ifaces:	all slave interfaces of a MLD interface if @ifname belongs to it.
 * @c2sep:
 * @return:		number of printed characters.
 */
static int __show_staiface_info(webs_t wp, const char *ifname, const char *mld_slave_ifaces, const char *c2sep)
{
#if defined(RTCONFIG_WIGIG)
	const int wigig = 1;
#else
	const int wigig = 0;
#endif
	int i, unit, w, ret = 0;
	unsigned int ch = 0;
	unsigned char mac_addr[6];
	char *p, *next, mac[sizeof("00:11:22:33:44:55XX")];
	char ssid[32 + 1], ssid_uri[sizeof(ssid) * 3] = "";
	char vap[IFNAMSIZ], slave_ifaces[MAX_NR_WL_IF * IFNAMSIZ];
	char cmd[sizeof("iwconfig staXYYYYYY")];
	char ap_bssid[sizeof("00:00:00:00:00:00XXX")];
	char phymode[sizeof("11a/n/ac/ax/be/ad/ay") + 10] = "";
	char bitrate[sizeof("2.8824 Gb/s, 320MHz") + 10] = "";
	char bwstr[sizeof("320MHz") + 4] = "";
	char sep[STA1WIDTH + 2 + 32 + 1 + STA3WIDTH + 1 + STA4WIDTH + 1 + STA5WIDTH + 1 + STA6WIDTH + 3];

	if (!wp || !ifname || !c2sep || *c2sep == '\0' || strncmp(ifname, "sta", 3))
		return 0;

	if (!mld_slave_ifaces || *mld_slave_ifaces == '\0')
		strlcpy(slave_ifaces, ifname, sizeof(slave_ifaces));
	else
		strlcpy(slave_ifaces, mld_slave_ifaces, sizeof(slave_ifaces));

	/* SSID */
	unit = get_sta_ifname_unit(ifname);
	strlcpy(ssid, getSSIDbyIFace(unit, ifname), sizeof(ssid));
	string_to_uriencode(ssid_uri, ssid, sizeof(ssid_uri));
	ret += websWrite(wp, "%-*s: %-*s\n", VAP1WIDTH, "SSID", g_max_c1data_len, ssid_uri);

	w = sizeof(sep) - (32 - g_max_c1data_len);
	for (i = 0, p = sep; i < (sizeof(sep) - 1); ++i) {
		if (i >= w)
			break;
		*p++ = '-';
	}
	*p++ = '\0';
	ret += websWrite(wp, "\nInterfaces List\n%s\n", sep);
	ret += websWrite(wp, "%-*s %-*s %-*s %-*s %-*s %-*s\n", STA1WIDTH, "MAC",
		STA2WIDTH, "Parent-AP BSSID", STA3WIDTH, "PHY Mode", STA4WIDTH, "Channel",
		STA5WIDTH, "Bandwidth", STA6WIDTH, "Bit Rate");

	foreach(vap, slave_ifaces, next) {
		/* Parent-AP BSSID */
		if (wigig && unit == WL_60G_BAND) {
			/* Example: iw wlan0 info
			 * Interface wlan0
			 *         ifindex 24
			 *         wdev 0x1
			 *         addr 04:ce:14:0b:46:12
			 *         type AP
			 *         wiphy 0
			 */
			snprintf(cmd, sizeof(cmd), "iw %s info", vap);
			if (exec_and_parse(cmd, "addr", "%*s %[^\n]", 1, ap_bssid))
				*ap_bssid = '\0';
			convert_mac_string(ap_bssid);
		} else {
			snprintf(cmd, sizeof(cmd), "iwconfig %s", vap);
			if (exec_and_parse_after(cmd, "Access Point:", "%s", 1, ap_bssid))
				*ap_bssid = '\0';
		}

		/* STA BSSID (MAC) */
		get_iface_hwaddr(vap, mac_addr);
		ether_etoa(mac_addr, mac);

		strlcpy(phymode, getAPPhyModebyIface(vap), sizeof(phymode));
		__getAPBitRate(vap, bitrate, sizeof(bitrate));
		ch = get_channel(vap);
		unit = get_sta_ifname_unit(vap);
		getVAPBandwidth(unit, vap, bwstr, sizeof(bwstr));
		if (*bwstr != '\0')
			strlcat(bwstr, "MHz", sizeof(bwstr));

		/* BSSID, max PHY Mode, Channel, Bandwidth, Bit Rate. */
		ret += websWrite(wp, "%-*s %-*s %-*s %-*u %-*s %-*s\n",
			STA1WIDTH, mac, STA2WIDTH, ap_bssid, STA3WIDTH, phymode,
			STA4WIDTH, ch, STA5WIDTH, bwstr, STA6WIDTH, bitrate);
	}
	return ret;
}

/* Show SSID, BSSID, Phy Mode, Bit Rate, and Channel of non-MLD downstream interface.
 * @wp:
 * @unit:
 * @ifname:		athX or staX; don't input mldX.
 * @mld_slave_ifaces:	all slave interfaces of a MLD fronthaul interface of same @unit band.
 * @prefix:
 * @ssid_title:
 * @c2sep:
 * @return:	number of printed characters.
 */
static int __show_wliface_info(webs_t wp, int unit, const char *ifname, const char *mld_slave_ifaces, const char *prefix, const char *ssid_title, const char *c2sep)
{
#if defined(RTCONFIG_MLO)
	int mlo = 0;
#else
	const int mlo = 0;
#endif
#if defined(RTCONFIG_WIGIG)
	const int wigig = 1;
#else
	const int wigig = 0;
#endif
	int ret = 0, c_np = 0, nr_cw, l;
	char ssid[32 + 1], ssid_uri[sizeof(ssid) * 3] = "";
	char *p, mlo_ssid_title[VAP1WIDTH + 1] = "";
	char cmd[sizeof("iwconfig staXYYYYYY")];
	char ap_bssid[sizeof("00:00:00:00:00:00XXX")];
	char slave_ifaces[MAX_NR_WL_IF * IFNAMSIZ] = "";

	if (!wp || __absent_band(unit) || !ifname || !prefix || *prefix == '\0' || !ssid_title || !c2sep)
		return 0;

	if (!mld_slave_ifaces)
		mld_slave_ifaces = "";

	strlcpy(slave_ifaces, mld_slave_ifaces, sizeof(slave_ifaces));
#if defined(RTCONFIG_MLO)
	if (sw_mode() != SW_MODE_REPEATER && find_word(slave_ifaces, ifname)) {
		mlo = 1;
		snprintf(mlo_ssid_title, sizeof(mlo_ssid_title), "%s (MLO)", ssid_title);
	}
#endif

	/* SSID */
	strlcpy(ssid, getSSIDbyIFace(unit, ifname), sizeof(ssid));
	for (p = ssid, c_np = 0; *p != '\0'; ++p) {
		if (!isprint(*p))
			c_np++;
	}
	nr_cw = c_np / 3;
	l = nr_cw? nr_cw + (nr_cw >> 1) : 0;
	if (l > 0) {
		if (!(nr_cw % 3))
			l -= 1;
		else if ((nr_cw % 3) == 1)
			l += 1;
	}
	string_to_uriencode(ssid_uri, ssid, sizeof(ssid_uri));
	ret += websWrite(wp, "%-*s: %-*s", VAP1WIDTH, mlo? mlo_ssid_title : ssid_title,
		g_max_c1data_len, ssid_uri);
	if (l > 0)
		ret += websWrite(wp, "%*s", l, "");

	/* BSSID */
	if (wigig && unit == WL_60G_BAND) {
		/* Example: iw wlan0 info
		 * Interface wlan0
		 *         ifindex 24
		 *         wdev 0x1
		 *         addr 04:ce:14:0b:46:12
		 *         type AP
		 *         wiphy 0
		 */
		snprintf(cmd, sizeof(cmd), "iw %s info", ifname);
		if (exec_and_parse(cmd, "addr", "%*s %[^\n]", 1, ap_bssid))
			*ap_bssid = '\0';
		convert_mac_string(ap_bssid);
	} else {
		snprintf(cmd, sizeof(cmd), "iwconfig %s", ifname);
		if (exec_and_parse_after(cmd, "Access Point:", "%s", 1, ap_bssid))
			*ap_bssid = '\0';
	}
	ret += websWrite(wp, "%s%-*s: %s\n", c2sep, VAP2WIDTH, "BSSID", ap_bssid);

	return ret;
}

static int
show_wliface_info(webs_t wp, int unit, const char *ifname, const char *op_mode)
{
#if defined(RTCONFIG_MLO)
	const int mlo_possible = 1, mlo_sta = !!nvram_get_int("qca_mlo_sta");
#else
	const int mlo_possible = 0, mlo_sta = 0;
#endif
	const char *mac_t = g_mac_title;
	const int sw_mode = sw_mode();
#if defined(GTAXY16000)
	unsigned int edmg_channel;
#endif
	int i, su, l, w, max_mssid, is_sta;
	int ret = 0, cac = 0, radar_cnt = 0, radar_list[32];;
	uint64_t m = 0;
#if defined(RTCONFIG_AMAS)
	uint64_t all_ch_m = 0, unavbl_ch_m = 0;
#endif
	unsigned int ch = 0;
	unsigned char mac_addr[ETHER_ADDR_LEN];
	char *p, prefix[sizeof("wlX_XXX")], pfix[sizeof("wlX.XXX_")];
	char vap[IFNAMSIZ], vphy[IFNAMSIZ], ssid_title[VAP1WIDTH + 1];
	char slave_ifaces[MAX_NR_WL_IF * IFNAMSIZ] = "";
	char phymode[sizeof("11a/n/ac/ax/be/ad/ay") + 10] = "";
	char bitrate[sizeof("2.8824 Gb/s, 320MHz") + 10] = "";
	char bwstr[sizeof("320MHz") + 4] = "", c2sep[50] = "";
	char bsep[VAP1WIDTH + 2 + 32 + 3 + VAP2WIDTH + 2 + 20 + 10] = "";
	char btitle[sizeof(bsep) - 2] = "";
	enum sdntype_id st_id __attribute__((unused));
#if defined(RTCONFIG_MULTILAN_MWL)
	char swap_name[64];
#endif

	if (unit < 0 || !ifname || !op_mode)
		return 0;

#if defined(RTCONFIG_MULTILAN_MWL)
	if (!strcmp(get_wififname(unit), ifname)) {
		/* do not show hidden BACKHAUL infromation */
		if ((p = get_sdntype_iface("MAINFH", unit, swap_name, sizeof(swap_name))) != NULL
		 && *swap_name != '\0') {
			ifname = swap_name;
			free(p);
		}
	}
#endif
	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	w = VAP1WIDTH + 2 + g_max_c1data_len + 3 + VAP2WIDTH + 2 + 20 + sizeof(" (CAC scan)");
	/* make sure length of band seperator greater than length of separator for "Station List" */
	if (w < 85)
		w = 85;
	/* band separator */
	for (i = 0, p = bsep; i < sizeof(bsep) - 1; ++i) {
		if (i == w)
			break;
		*p++ = '_';
	}
	*p = '\0';
	is_sta = is_sta_ifname(ifname);
	if (is_sta) {
		if (mlo_sta) {
			/* RT/AP/RP/MB/RE */
			strlcpy(slave_ifaces, mld_slave_ifaces(MLD_STA), sizeof(slave_ifaces));
		}
		if (find_word(slave_ifaces, ifname)) {
			strlcpy(btitle, " <MLO STA> ", sizeof(btitle));
		} else {
			strlcpy(btitle, " <STA> ", sizeof(btitle));
		}
	} else if (mlo_possible && sw_mode != SW_MODE_REPEATER && !strcmp(ifname, MLD_AP)) {
		/* RT/AP/RE */
		strlcpy(btitle, " <MLO backhaul> ", sizeof(btitle));
	} else {
		snprintf(btitle, sizeof(btitle), " <%s AP> ", get_band_longname(unit));
	}
	l = strlen(btitle);
	if (l < w)
		memcpy(bsep + ((w - l) / 2), btitle, l);
	ret += websWrite(wp, "%s\n", bsep);

	/* sepapator string for 2-nd column */
	l = (w - (VAP1WIDTH + 2 + g_max_c1data_len + 1) - (VAP2WIDTH + 2 + 17) - 3) / 2;
	for (i = 0, p = c2sep; i < sizeof(c2sep) - 1; ++i) {
		if (i >= (l * 2 + 1))
			break;
		*p++ = ' ';
	}
	*p = '\0';

	memset(&mac_addr, 0, sizeof(mac_addr));
	if (is_sta_ifname(ifname)) {
		ret += websWrite(wp, "%-*s: %-*s", VAP1WIDTH, "OP Mode", g_max_c1data_len, op_mode);

		strlcpy(vap, ifname, sizeof(vap));
		mac_t = g_mac_title;
		if (find_word(slave_ifaces, ifname)) {
			/* Show MAC address of mld2 instead of one of slave staX iface. */
			strlcpy(vap, MLD_STA, sizeof(vap));
			mac_t = g_mld_mac_title;
		}
		get_iface_hwaddr(vap, mac_addr);
		ret += websWrite(wp, "%s%-*s: %02X:%02X:%02X:%02X:%02X:%02X\n",
			c2sep, VAP2WIDTH, mac_t, mac_addr[0], mac_addr[1],
			mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

		/* Show SSID, BSSID, PHY Mode, Bit Rate, and channel of
		 * all slave interface of MLD_STA/standlone STA interface.
		 */
		__show_staiface_info(wp, ifname, slave_ifaces, c2sep);
	} else if (mlo_possible && !strcmp(ifname, MLD_AP)) {
		ret += websWrite(wp, "%-*s: %-*s", VAP1WIDTH, "OP Mode", g_max_c1data_len, op_mode);

		/* Show MAC address of mld0 instead of one of slave staX iface. */
		get_iface_hwaddr(MLD_AP, mac_addr);
		ret += websWrite(wp, "%s%-*s: %02X:%02X:%02X:%02X:%02X:%02X\n",
			c2sep, VAP2WIDTH, g_mld_mac_title, mac_addr[0], mac_addr[1],
			mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

		/* Show bands of each slave interfaces and client list of the MLD_AP. */
		__show_mldapiface_info(wp, MLD_AP, c2sep);
	} else {
		ret += websWrite(wp, "%-*s: %-*s", VAP1WIDTH, "OP Mode", g_max_c1data_len, op_mode);

		/* Show MLD address if one of VAP belongs to MLO fronthaul interface. */
		max_mssid = (sw_mode == SW_MODE_REPEATER)? 1 : num_of_mssid_support(unit);
		strlcpy(slave_ifaces, mld_slave_ifaces(MLD_SDN), sizeof(slave_ifaces));
		for (su = 0; su <= max_mssid; ++su) {
#if defined(RTCONFIG_MLO_BH)
			/* Hide SSID if it's backhaul. */
			if (!su && sw_mode != SW_MODE_REPEATER)
				continue;
#endif
#if defined(RTCONFIG_VIF_ONBOARDING)
			/* Hide SSID if it's dedicated to onboarding. */
			if (unit == WL_2G_BAND && su == get_vif_subunit())
				continue;
#endif
			if (su == 0)
				snprintf(pfix, sizeof(pfix), "wl%d_", unit);
			else
				snprintf(pfix, sizeof(pfix), "wl%d.%d_", unit, su);
			if (!nvram_pf_match(pfix, "bss_enabled", "1"))
				continue;

			strlcpy(vap, nvram_pf_safe_get(pfix, "ifname"), sizeof(vap));
			if (*vap == '\0' || !find_word(slave_ifaces, vap))
				continue;

			get_iface_hwaddr(MLD_SDN, mac_addr);
			ret += websWrite(wp, "%s%-*s: %02X:%02X:%02X:%02X:%02X:%02X",
				c2sep, VAP2WIDTH, g_mld_mac_title, mac_addr[0], mac_addr[1],
				mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
			break;
		}
		ret += websWrite(wp, "\n");

		for (su = 0; su <= max_mssid; ++su) {
#if defined(RTCONFIG_MLO_BH)
			/* Hide SSID if it's backhaul. */
			if (!su && sw_mode != SW_MODE_REPEATER)
				continue;
#endif
#if defined(RTCONFIG_VIF_ONBOARDING)
			/* Hide SSID if it's dedicated to onboarding. */
			if (unit == WL_2G_BAND && su == get_vif_subunit())
				continue;
#endif
			if (su == 0)
				snprintf(pfix, sizeof(pfix), "wl%d_", unit);
			else
				snprintf(pfix, sizeof(pfix), "wl%d.%d_", unit, su);
			if (!nvram_pf_match(pfix, "bss_enabled", "1"))
				continue;

			strlcpy(vap, nvram_pf_safe_get(pfix, "ifname"), sizeof(vap));
			if (*vap == '\0' || is_sta_ifname(vap) || !iface_exist(vap)) {
				dbg("%s: invalid iface [%s]. (unit %d subunit %d)\n",
					__func__, vap, unit, su);
				continue;
			}
			st_id = get_sdntype_by_wlxy(pfix);
#if defined(RTCONFIG_MULTILAN_MWL)
			if (st_id == SDNTYPE_MAINBH) {
				/* shouldn't happen */
				continue;
			} else if (st_id == SDNTYPE_MAINFH) {
				snprintf(ssid_title, sizeof(ssid_title), "Main SSID");
			} else if (st_id == SDNTYPE_IOT) {
				strlcpy(ssid_title, "IoT Network SSID", sizeof(ssid_title));
			} else if (st_id == SDNTYPE_KIDS) {
				strlcpy(ssid_title, "Kid's Network SSID", sizeof(ssid_title));
			} else if (st_id == SDNTYPE_GUEST) {
				strlcpy(ssid_title, "Guest Network SSID", sizeof(ssid_title));
			} else {
				strlcpy(ssid_title, "SSID", sizeof(ssid_title));
			}
#else
			strlcpy(ssid_title, "SSID", sizeof(ssid_title));
#endif
			__show_wliface_info(wp, unit, vap, slave_ifaces, prefix, ssid_title, c2sep);
		}
#if defined(RTCONFIG_WIGIG)
		if (unit == WL_60G_BAND) {
			strlcpy(phymode, getAPPhyModebyIfaceIW(get_wifname(unit)), sizeof(phymode));
			__getAPBitRateIW(unit, ifname, bitrate, sizeof(bitrate));
			ch = getAPChannelbyIWInfo(get_wifname(unit));	/* FIXME */
		} else
#endif
		{
			strlcpy(phymode, getAPPhyModebyIface(ifname), sizeof(phymode));
			__getAPBitRate(ifname, bitrate, sizeof(bitrate));
			ch = get_channel(ifname);
		}
		ret += websWrite(wp, "%-*s: %-*s", VAP1WIDTH, "PHY Mode", g_max_c1data_len, phymode);

		/* Bit Rate, CAC status, bandwidth */
		if (is_5g(unit) && !is_sta_ifname(ifname)) {
			cac = safe_atoi(iwpriv_get(ifname, "get_cac_state")? : "0");
			strlcpy(vphy, get_vphyifname(swap_5g_band(unit)), sizeof(vphy));
			radar_cnt = get_radar_channel_list(vphy, radar_list, ARRAY_SIZE(radar_list));
			for (i = 0; i < radar_cnt; ++i) {
				m |= ch5g2bitmask(radar_list[i]);
			}
#if defined(RTCONFIG_AMAS)
			all_ch_m = get_channel_list_mask(unit);
			unavbl_ch_m = chlist5g2bitmask(nvram_pf_get(prefix, "unavbl_ch"), ",");
			if (unavbl_ch_m && unavbl_ch_m == all_ch_m)
				unavbl_ch_m = 0;
			m |= unavbl_ch_m & DFS_CH_M;
#endif
		}
		ret += websWrite(wp, "%s%-*s: %s%s", c2sep, VAP2WIDTH, "Bit Rate",
			bitrate, cac? " (CAC scan)" : "");
		getVAPBandwidth(unit, ifname, bwstr, sizeof(bwstr));
		if (*bwstr != '\0')
			ret += websWrite(wp, ", %sMHz", bwstr);
		ret += websWrite(wp, "\n");

		/* Channel */
		ret += websWrite(wp, "%-*s: %u", VAP1WIDTH, "Channel", ch);
		if (m) {
			ret += websWrite(wp, " (Radar: %s)", bitmask2chlist5g(m, ","));
		}
		ret += websWrite(wp, "\n");
#if defined(GTAXY16000)
		if (unit == WL_60G_BAND) {
			edmg_channel = getEDMGChannel();
			if (edmg_channel != 0) {
				ret += websWrite(wp, "%-*s: %u\n", VAP1WIDTH, "EDMG Channel", edmg_channel);
			}
		}
#endif
	}

	return ret;
}

#if defined(RTCONFIG_MLO)
static int show_mlo_bh_info(webs_t wp)
{
	int ret = 0;

	/* MLD_AP
	 * RT/AP:	MLO backhaul
	 * RP/MB:	MLO fronthaul (AP)
	 */
	if (sw_mode() == SW_MODE_REPEATER || !iface_exist(MLD_AP))
		return 0;

	ret += show_wliface_info(wp, WL_2G_BAND, MLD_AP, OP_MODE_MLO_BH);
	return ret;
}
#endif

#if defined(RTCONFIG_MULTILAN_MWL)
static char *subid_to_sdn_name(char id)
{
	switch (id) {
		case 'B':
			return "BackHaul";
		case 'F':
			return "Main";
		case 'I':
			return "Iot Network";
		case 'G':
			return "Guest Network";
		case 'P':
			return "Portal";
		case 'E':
			return "Employee";
		case 'K':
			return "Kids";
		case 'M':
			return "MLO";
		default:
			return "SDNs";
	}
}
#endif

static int
wl_status(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
	int ret = 0, wl_mode_x, i, w;
	WIFI_STA_TABLE *sta_info;
	WLANCONFIG_LIST *l;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_", *ifname, *op_mode;
	char *p, subunit_str[20];
	char sep[VLS1WIDTH + 2 + 32 + 1 + VLS3WIDTH + 1 + VLS4WIDTH + 1 + VLS5WIDTH + 1 + VLS6WIDTH + 1 + VLS7WIDTH + 1 + VLS8WIDTH + 3];
#if defined(RTCONFIG_CONCURRENTREPEATER)
	char wlc_prefix[] = "wlcXXXXXXXXXX_";
#endif
#if defined(RTCONFIG_MULTILAN_MWL)
	char subid_sdn_type[20];
#endif

#if defined(RTCONFIG_LYRA_5G_SWAP)
       unit=swap_5g_band(unit);
#endif

#if defined(RTCONFIG_WIRELESSREPEATER) && defined(RTCONFIG_PROXYSTA)
	if (mediabridge_mode()) {
#if !defined(RTCONFIG_CONCURRENTREPEATER)
		/* Media bridge mode */
		snprintf(prefix, sizeof(prefix), "wl%d.1_", unit);
		ifname = nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp)));
		if (unit != nvram_get_int("wlc_band")) {
			snprintf(prefix, sizeof(prefix), "wl%d_", unit);
			ret += websWrite(wp, "%s radio is disabled\n",
				wl_nband_name(nvram_pf_get(prefix, "nband")));
			return ret;
		}
		ret += show_wliface_info(wp, unit, ifname, OP_MODE_MB);
#else
		snprintf(prefix, sizeof(prefix), "wl%d.1_", unit);
		ifname = nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp)));
		snprintf(wlc_prefix, sizeof(wlc_prefix), "wlc%d_", unit);
		ret += show_wliface_info(wp, unit, ifname, OP_MODE_MB);
		ret += websWrite(wp, "\n");
#endif	/* #if !defined(RTCONFIG_CONCURRENTREPEATER) */
	} else {
#endif
		/* Router mode, Repeater and AP mode */
#if defined(RTCONFIG_WIRELESSREPEATER)
#if !defined(RTCONFIG_CONCURRENTREPEATER)
		if (!unit && repeater_mode()) {
			/* Show P-AP information first, if we are about to show 2.4G information in repeater mode. */
#if defined(RTCONFIG_REPEATER_STAALLBAND)
			unit = nvram_get_int("wlc_triBand");
			snprintf(prefix, sizeof(prefix), "sta%d", unit);
			ret += show_wliface_info(wp, unit, prefix, OP_MODE_RP);
			ret += websWrite(wp, "\n");
			unit = 0;
#else
			snprintf(prefix, sizeof(prefix), "wl%d.1_", nvram_get_int("wlc_band"));
			ifname = nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp)));
			ret += show_wliface_info(wp, nvram_get_int("wlc_band"), ifname, OP_MODE_RP);
			ret += websWrite(wp, "\n");
#endif
		}
#else	/* RTCONFIG_CONCURRENTREPEATER */
		if (repeater_mode()) {
			if (!unit) {
				if (nvram_get_int("wlc_express") == 0) {	/* concurrent repeater */
					for (i = 0; i <= 1; i++) {
						snprintf(prefix, sizeof(prefix), "wl%d.1_", i);
						ifname = nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp)));
						snprintf(wlc_prefix, sizeof(wlc_prefix), "wlc%d_", i);
						ret += show_wliface_info(wp, i, ifname, OP_MODE_RP);
						ret += websWrite(wp, "\n");
					}
				}
				else {	/* express way (2G or 5G) */
					snprintf(prefix, sizeof(prefix), "wl%d.1_", nvram_get_int("wlc_express") - 1);
					ifname = nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp)));
					snprintf(wlc_prefix, sizeof(wlc_prefix), "wlc%d_", nvram_get_int("wlc_express") - 1);
					ret += show_wliface_info(wp, nvram_get_int("wlc_express") - 1, ifname, nvram_get_int("wlc_express") == 1 ? "Express Way 2.4 GHz" : "Express Way 5 GHz");
					ret += websWrite(wp, "\n");
					//return ret;
				}
			}

			if (nvram_get_int("wlc_express") > 0) {
				if (unit == (nvram_get_int("wlc_express") - 1))
					return ret;
			}
		}
#endif	/* #if !defined(RTCONFIG_CONCURRENTREPEATER) */
#endif

		snprintf(prefix, sizeof(prefix), "wl%d_", unit);
		ifname = nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp)));
		if (!get_radio_status(ifname)) {
#if defined(BAND_2G_ONLY)
			ret += websWrite(wp, "2.4 GHz radio is disabled\n");
#else

#if defined(RTCONFIG_HIDDEN_BACKHAUL)
#if defined(MAPAC2200)
		if(strcmp(ifname,"ath1"))
#endif
#endif

			ret += websWrite(wp, "%s radio is disabled\n",
				wl_nband_name(nvram_pf_get(prefix, "nband")));
#endif
			return ret;
		}

		wl_mode_x = nvram_get_int(strlcat_r(prefix, "mode_x", tmp, sizeof(tmp)));
		op_mode = OP_MODE_AP;
		if (wl_mode_x == 1)
			op_mode = OP_MODE_WDS_ONLY;
		else if (wl_mode_x == 2)
			op_mode = OP_MODE_HYBRID;
#if defined(RTCONFIG_CONCURRENTREPEATER)
		if (repeater_mode()) {
			if (nvram_get_int("wlc_express") == 0) {	/* concurrent repeater */
				snprintf(wlc_prefix, sizeof(wlc_prefix), "wl%d.1_", unit);
				ret += show_wliface_info(wp, unit, ifname, op_mode);
			}
			else
			{
				snprintf(wlc_prefix, sizeof(wlc_prefix), "wl%d.1_", nvram_get_int("wlc_express") == 1 ? 1 : 0);
				ret += show_wliface_info(wp, nvram_get_int("wlc_express") == 1 ? 1 : 0, ifname, op_mode);
			}
		}
		else
#endif
			ret += show_wliface_info(wp, unit, ifname, op_mode);

		w = sizeof(sep) - (32 - g_max_c1data_len);
		for (i = 0, p = sep; i < (sizeof(sep) - 1); ++i) {
			if (i >= w)
				break;
			*p++ = '-';
		}
		*p++ = '\0';
		ret += websWrite(wp, "\nStations List\n%s\n", sep);
		ret += websWrite(wp, "%-*s %-*s %-*s %-*s %-*s %-*s %-*s %-*s\n",
			VLS1WIDTH, "ESS", VLS2WIDTH, "MAC", VLS3WIDTH, "PhyMode", VLS4WIDTH, "MLO",
			VLS5WIDTH, "RSSI", VLS6WIDTH, "TX_RATE", VLS7WIDTH, "RX_RATE",
			VLS8WIDTH, "Connect Time");

		if ((sta_info = malloc(sizeof(*sta_info))) != NULL) {
#if defined(RTCONFIG_WIFI7)
#if !defined(RTCONFIG_MULTILAN_MWL)
			int decrease_idx = iot_exists() ? 1 : 0;
#endif
#else
			int decrease_idx = 0;
#endif
			getSTAInfo(unit, sta_info);
#if defined(RTCONFIG_MULTILAN_MWL)
			enum_sdn_subid_type(unit, subid_sdn_type, sizeof(subid_sdn_type));
#endif
			for(i = 0, l = sta_info->Entry; i < sta_info->Num; i++, l++) {
				*subunit_str = '\0';

#if defined(RTCONFIG_MULTILAN_MWL)
				if (l->subunit_id == 'M') // skip MLO
					continue;
				else {
					int tmp_idx;
					tmp_idx = l->subunit_id - '0';
					if (tmp_idx < sizeof(subid_sdn_type))
						snprintf(subunit_str, sizeof(subunit_str), "%s", subid_to_sdn_name(subid_sdn_type[tmp_idx]));
					else
						snprintf(subunit_str, sizeof(subunit_str), "%s", subid_to_sdn_name('O'));
				}
#else
				if (l->subunit_id == '0')
					strlcpy(subunit_str, "Main", sizeof(subunit_str));
				else if (isdigit(l->subunit_id))
					snprintf(subunit_str, sizeof(subunit_str), "Guest Network-%c", l->subunit_id - decrease_idx);
				else if (l->subunit_id == 'B')
					strlcpy(subunit_str, "Facebook Wi-Fi", sizeof(subunit_str));
				else if (l->subunit_id == 'F')
					strlcpy(subunit_str, "Free Wi-Fi", sizeof(subunit_str));
				else if (l->subunit_id == 'C')
					strlcpy(subunit_str, "Captive Portal", sizeof(subunit_str));
				else if (l->subunit_id == 'I')
					strlcpy(subunit_str, "IoT Network", sizeof(subunit_str));
				else {
					dbg("%s: Unknown subunit_id [%c]\n", l->subunit_id);
				}
#endif
				ret += websWrite(wp, "%-*s %-*s %-*s %-*s %*d %-*s %-*s %-*s\n",
					VLS1WIDTH, subunit_str, VLS2WIDTH, l->addr, VLS3WIDTH, l->mode,
					VLS4WIDTH, (l->flags & WLFLAGS_MLO)? "Yes" : "No", VLS5WIDTH, l->rssi,
					VLS6WIDTH, l->txrate, VLS7WIDTH, l->rxrate, VLS8WIDTH, l->conn_time);
			}
			free(sta_info);
		}
#if defined(RTCONFIG_WIRELESSREPEATER) && defined(RTCONFIG_PROXYSTA)
	}
#endif

	return ret;
}

static int ej_wl_sta_list(int unit, webs_t wp)
{
	WIFI_STA_TABLE *sta_info;
	char *value;
	int firstRow = 1;
	int i;
	int from_app = 0;

	from_app = check_user_agent(user_agent);

	if(hook_get_json == 1)
		websWrite(wp, "{");

	if ((sta_info = malloc(sizeof(*sta_info))) != NULL)
	{
		getSTAInfo(unit, sta_info);
		for(i = 0; i < sta_info->Num; i++)
		{
			if (firstRow == 1)
				firstRow = 0;
			else
				websWrite(wp, ", ");

			if (from_app == 0 && hook_get_json == 0)
				websWrite(wp, "[");

			websWrite(wp, "\"%s\"", sta_info->Entry[i].addr);

			if (from_app != 0 || hook_get_json == 1) {
				websWrite(wp, ":{");
				websWrite(wp, "\"isWL\":");
			}

			value = "Yes";
			if (from_app == 0 && hook_get_json == 0)
				websWrite(wp, ", \"%s\"", value);
			else
				websWrite(wp, "\"%s\"", value);

			value = "";

			if (from_app == 0 && hook_get_json == 0)
				websWrite(wp, ", \"%s\"", value);
	
			if (from_app != 0 || hook_get_json == 1) {
				websWrite(wp, ",\"rssi\":");
			}

			if (from_app == 0 && hook_get_json == 0)
				websWrite(wp, ", \"%d\"", sta_info->Entry[i].rssi);
			else
				websWrite(wp, "\"%d\"", sta_info->Entry[i].rssi);

			if (from_app == 0 && hook_get_json == 0)
				websWrite(wp, "]");
			else
				websWrite(wp, "}");
		}
		free(sta_info);
	}
	if(hook_get_json == 1)
		websWrite(wp, "}");
	return 0;
}

int ej_wl_sta_list_2g(int eid, webs_t wp, int argc, char_t **argv)
{
	ej_wl_sta_list(WL_2G_BAND, wp);
	return 0;
}

int ej_wl_sta_list_5g(int eid, webs_t wp, int argc, char_t **argv)
{
	ej_wl_sta_list(WL_5G_BAND, wp);
	return 0;
}

int ej_wl_sta_list_5g_2(int eid, webs_t wp, int argc, char_t **argv)
{
#if defined(RTCONFIG_HAS_5G_2)
	/* FIXME: I think it's not good to report 2-nd 5G station list in 1-st 5G station list. */
	ej_wl_sta_list(WL_5G_2_BAND, wp);
#endif
	if(hook_get_json == 1)
		websWrite(wp, "{}");
	return 0;
}

#if defined(RTCONFIG_STAINFO)
/**
 * Format:
 * 	[ MAC, TX_RATE, RX_RATE, CONNECT_TIME, IDX ]
 * IDX:	main/GN1/GN2/GN3
 */
static int wl_stainfo_list(int unit, webs_t wp)
{
	WIFI_STA_TABLE *sta_info;
	WLANCONFIG_LIST *r;
	char idx_str[8], s;
	int i, firstRow = 1;

	if ((sta_info = malloc(sizeof(*sta_info))) == NULL){
		if(hook_get_json == 1)
			websWrite(wp, "[]");
		return 0 ;
	}

	if(hook_get_json == 1)
		websWrite(wp, "[");

	getSTAInfo(unit, sta_info);
	for(i = 0, r = &sta_info->Entry[0]; i < sta_info->Num; i++, r++) {
		if (firstRow == 1)
			firstRow = 0;
		else
			websWrite(wp, ", ");

		websWrite(wp, "[");
		websWrite(wp, "\"%s\"", r->addr);
		websWrite(wp, ", \"%s\"", r->txrate);
		websWrite(wp, ", \"%s\"", r->rxrate);
		websWrite(wp, ", \"%s\"", r->conn_time);
		s = r->subunit_id;
		if (s < '0' || s  >= ('0' + MAX_NO_MSSID - 1))
			s = '0';
		if (s == '0')
			strlcpy(idx_str, "main", sizeof(idx_str));
		else if (isdigit(s))
			snprintf(idx_str, sizeof(idx_str), "GN%c", s);
		else
			snprintf(idx_str, sizeof(idx_str), "%c", toupper(s));
		websWrite(wp, ", \"%s\"", idx_str);
		websWrite(wp, "]");
	}
	if(hook_get_json == 1)
		websWrite(wp, "]");
	free(sta_info);
	return 0;
}

int
ej_wl_stainfo_list_2g(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_stainfo_list(0, wp);
}

int
ej_wl_stainfo_list_5g(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_stainfo_list(1, wp);
}

int
ej_wl_stainfo_list_5g_2(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_stainfo_list(2, wp);
}
#endif  /* RTCONFIG_STAINFO */

int ej_get_wlstainfo_list(int eid, webs_t wp, int argc, char_t **argv)
{
	char word[64], *next;
	int unit = 0;
	int haveInfo = 0;

	websWrite(wp, "{");

	foreach (word, nvram_safe_get("wl_ifnames"), next) {
		WIFI_STA_TABLE *sta_info;
		WLANCONFIG_LIST *r;
		int i, j, s;
		char alias[16];

		SKIP_ABSENT_BAND_AND_INC_UNIT(unit);
		if ((sta_info = malloc(sizeof(*sta_info))) == NULL)
			return 0;

		getSTAInfo(unit, sta_info);
		for (i = 0; i < MAX_NO_MSSID; i++) {
			int firstRow = 1;
			SKIP_ABSENT_BAND(i);

			memset(alias, 0, sizeof(alias));
			switch (unit) {
				case WL_2G_BAND:
					if (i == 0)
						strlcpy(alias, "2G", sizeof(alias));
					else
						snprintf(alias, sizeof(alias), "%s_%d", "2G", i);
					break;
				case WL_5G_BAND:	/* fall-through */
#if defined(RTCONFIG_HAS_5G_2)
				case WL_5G_2_BAND:
#endif
					if (i == 0)
						snprintf(alias, sizeof(alias), "%s", unit == WL_5G_2_BAND ? "5G1" : "5G");
					else
						snprintf(alias, sizeof(alias), "%s_%d", unit == WL_5G_2_BAND ? "5G1" : "5G", i);
					break;
#if defined(RTCONFIG_HAS_6G)
				case WL_6G_BAND:	/* fall-through */
#if defined(RTCONFIG_HAS_6G_2)
				case WL_6G_2_BAND:
#endif
					if (i == 0)
						snprintf(alias, sizeof(alias), "%s", (unit == WL_5G_2_BAND || unit == WL_6G_2_BAND)? "6G1" : "6G");
					else
						snprintf(alias, sizeof(alias), "%s_%d", (unit == WL_5G_2_BAND || unit == WL_6G_2_BAND)? "6G1" : "6G", i);
					break;
#endif
#if defined(RTCONFIG_WIGIG)
				case WL_60G_BAND:
					if (i == 0)
						strlcpy(alias, "60G", sizeof(alias));
					else
						snprintf(alias, sizeof(alias), "%s_%d", "60G", i);
					break;
#endif
				default:
					dbg("%s():%d: Unknown unit %d (MAX_NR_WL_IF %d)\n", __func__, __LINE__, unit, MAX_NR_WL_IF);
			}

			for(j = 0, r = &sta_info->Entry[0]; j < sta_info->Num; j++, r++) {
				s = r->subunit_id;
				if (s < 0 || s > 3)
					s = 0;

				if (i != s)
					continue;

				if (firstRow == 1) {
					if (haveInfo)
						websWrite(wp, ",");
					websWrite(wp, "\"%s\":[", alias);
					firstRow = 0;
					haveInfo = 1;
				}
				else
					websWrite(wp, ",");
				websWrite(wp, "{\"mac\":\"%s\",\"rssi\":%d}", r->addr, r->rssi);
			}

			if (!firstRow)
				websWrite(wp, "]");
		}
		free(sta_info);
		unit++;
	}
	websWrite(wp, "}");

	return 0;
}

char *getWscStatus(int unit)
{
	static char buf[256];
	FILE *fp;
	int len;
	char *pt1,*pt2,*wlxy = NULL;
        char wif[IFNAMSIZ] = "";
#if defined(RTCONFIG_MULTILAN_MWL)
	wlxy = get_sdntype_iface("MAINFH", unit, wif, sizeof(wif));
	if (wlxy)
		free(wlxy);
	if (!wlxy || *wif == '\0')
		return "Failed";
#else
        strlcpy(wif, get_wifname(unit), sizeof(wif));
#endif
        snprintf(buf, sizeof(buf), "hostapd_cli -i%s wps_get_status", wif);
	fp = popen(buf, "r");
	if (fp) {
		memset(buf, 0, sizeof(buf));
		len = fread(buf, 1, sizeof(buf), fp);
		pclose(fp);
		if (len > 1) {
			buf[len-1] = '\0';
			pt1 = strstr(buf, "Last WPS result: ");
			if (pt1) {
				pt2 = pt1 + safe_strlen("Last WPS result: ");
				pt1 = strstr(pt2, "Peer Address: ");
				if (pt1) {
					*pt1 = '\0';
					chomp(pt2);
				}
				return pt2;
			}
		}
	}
	return "";
}

char *getAPPIN(int unit)
{
	static char buffer[128];
#if 0
	char cmd[64];
	FILE *fp;
	int len;

	buffer[0] = '\0';
	snprintf(cmd, sizeof(cmd), "hostapd_cli -i%s wps_ap_pin get", get_wifname(unit));
	fp = popen(cmd, "r");
	if (fp) {
		len = fread(buffer, 1, sizeof(buffer), fp);
		pclose(fp);
		if (len > 1) {
			buffer[len] = '\0';
			//dbg("%s: AP PIN[%s]\n", __FUNCTION__, buffer);
			if(!strncmp(buffer,"FAIL",4))
			   strlcpy(buffer,nvram_get("secret_code"), sizeof(buffer));
			return buffer;
		}
	}
	return "";
#else
	snprintf(buffer, sizeof(buffer), "%s", nvram_safe_get("secret_code"));
	return buffer;
#endif
}

int
wl_wps_info(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
	int j = -1, u = unit;
	char tmpstr[128];
	WPS_CONFIGURED_VALUE result;
	int retval=0;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_";
	char *wps_sta_pin;
	char tag1[] = "<wps_infoXXXXXX>", tag2[] = "</wps_infoXXXXXX>";

#if defined(RTCONFIG_WPSMULTIBAND)
	for (j = -1; j < MAX_NR_WL_IF; ++j) {
#endif
		switch (j) {
		case WL_2G_BAND:	/* fall through */
		case WL_5G_BAND:	/* fall through */
		case WL_5G_2_BAND:	/* fall through */
		case WL_6G_BAND:	/* fall through */
		case WL_6G_2_BAND:	/* fall through */
		case WL_60G_BAND:	/* fall through */
			u = j;
			snprintf(tag1, sizeof(tag1), "<wps_info%d>", j);
			snprintf(tag2, sizeof(tag2), "</wps_info%d>", j);
			break;
		case -1: /* fall through */
		default:
			u = unit;
			strlcpy(tag1, "<wps_info>", sizeof(tag1));
			strlcpy(tag2, "</wps_info>", sizeof(tag2));
		}

		snprintf(prefix, sizeof(prefix), "wl%d_", u);

#if defined(RTCONFIG_WPSMULTIBAND)
		SKIP_ABSENT_BAND(u);
		if (!nvram_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp))))
			continue;
#endif

		memset(&result, 0, sizeof(result));
		getWPSConfig(u, &result);

		if (j == -1)
			retval += websWrite(wp, "<wps>\n");

		//0. WSC Status
		memset(tmpstr, 0, sizeof(tmpstr));
		strlcpy(tmpstr, getWscStatus(u), sizeof(tmpstr));
		retval += websWrite(wp, "%s%s%s\n", tag1, tmpstr, tag2);

		//1. WPS Configured
		if (result.Configured==2)
			retval += websWrite(wp, "%s%s%s\n", tag1, "Yes", tag2);
		else
			retval += websWrite(wp, "%s%s%s\n", tag1, "No", tag2);

		//2. WPS SSID
		memset(tmpstr, 0, sizeof(tmpstr));
		char_to_ascii(tmpstr, result.SSID);
		retval += websWrite(wp, "%s%s%s\n", tag1, tmpstr, tag2);

		//3. WPS AuthMode
		retval += websWrite(wp, "%s%s%s\n", tag1, result.AuthMode, tag2);

		//4. WPS Encryp
		retval += websWrite(wp, "%s%s%s\n", tag1, result.Encryp, tag2);

		//5. WPS DefaultKeyIdx
		memset(tmpstr, 0, sizeof(tmpstr));
		snprintf(tmpstr, sizeof(tmpstr), "%d", result.DefaultKeyIdx);/* FIXME: TBD */
		retval += websWrite(wp, "%s%s%s\n", tag1, tmpstr, tag2);

		//6. WPS WPAKey
#if 0	//hide for security
		if (!safe_strlen(result.WPAKey))
			retval += websWrite(wp, "%sNone%s\n", tag1, tag2);
		else
		{
			memset(tmpstr, 0, sizeof(tmpstr));
			char_to_ascii(tmpstr, result.WPAKey);
			retval += websWrite(wp, "%s%s%s\n", tag1, tmpstr, tag2);
		}
#else
		retval += websWrite(wp, "%s%s\n", tag1, tag2);
#endif
		//7. AP PIN Code
		memset(tmpstr, 0, sizeof(tmpstr));
		strlcpy(tmpstr, getAPPIN(u), sizeof(tmpstr));
		retval += websWrite(wp, "%s%s%s\n", tag1, tmpstr, tag2);

		//8. Saved WPAKey
#if 0	//hide for security
		if (!safe_strlen(nvram_safe_get(strlcat_r(prefix, "wpa_psk", tmp, sizeof(tmp)))))
			retval += websWrite(wp, "%s%s%s\n", tag1, "None", tag2);
		else
		{
			char_to_ascii(tmpstr, nvram_safe_get(strlcat_r(prefix, "wpa_psk", tmp, sizeof(tmp))));
			retval += websWrite(wp, "%s%s%s\n", tag1, tmpstr, tag2);
		}
#else
		retval += websWrite(wp, "%s%s\n", tag1, tag2);
#endif
		//9. WPS enable?
		if (!strcmp(nvram_safe_get(strlcat_r(prefix, "wps_mode", tmp, sizeof(tmp))), "enabled"))
			retval += websWrite(wp, "%s%s%s\n", tag1, "None", tag2);
		else
			retval += websWrite(wp, "%s%s%s\n", tag1, nvram_safe_get("wps_enable"), tag2);

		//A. WPS mode
		wps_sta_pin = nvram_safe_get("wps_sta_pin");
		if (safe_strlen(wps_sta_pin) && strcmp(wps_sta_pin, "00000000"))
			retval += websWrite(wp, "%s%s%s\n", tag1, "1", tag2);
		else
			retval += websWrite(wp, "%s%s%s\n", tag1, "2", tag2);

		//B. current auth mode
		if (!safe_strlen(nvram_safe_get(strlcat_r(prefix, "auth_mode_x", tmp, sizeof(tmp)))))
			retval += websWrite(wp, "%s%s%s\n", tag1, "None", tag2);
		else
			retval += websWrite(wp, "%s%s%s\n", tag1, nvram_safe_get(strlcat_r(prefix, "auth_mode_x", tmp, sizeof(tmp))), tag2);

		//C. WPS band
		retval += websWrite(wp, "%s%d%s\n", tag1, u, tag2);
#if defined(RTCONFIG_WPSMULTIBAND)
	}
#endif

	retval += websWrite(wp, "</wps>");

	return retval;
}

int
ej_wps_info(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_wps_info(eid, wp, argc, argv, WL_5G_BAND);
}

int
ej_wps_info_2g(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_wps_info(eid, wp, argc, argv, WL_2G_BAND);
}

int
ej_wps_info_5g(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_wps_info(eid, wp, argc, argv, WL_5G_BAND);
}

int
ej_wps_info_5g_2(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_wps_info(eid, wp, argc, argv, WL_5G_2_BAND);
}

int
ej_wps_info_6g(int eid, webs_t wp, int argc, char_t **argv)
{
#if defined(RTCONFIG_HAS_6G)
	return wl_wps_info(eid, wp, argc, argv, WL_6G_BAND);
#else
	return 0;
#endif
}

int
ej_wps_info_6g_2(int eid, webs_t wp, int argc, char_t **argv)
{
#if defined(RTCONFIG_HAS_6G_2)
	return wl_wps_info(eid, wp, argc, argv, WL_6G_2_BAND);
#else
	return 0;
#endif
}

// Wireless Client List		 /* Start --Alicia, 08.09.23 */

struct ej_wl_auth_list_priv_s {
	int firstRow;
	webs_t wp;
};

/* Helper of ej_wl_auth_list()
 * @src:	pointer to WLANCONFIG_LIST
 * @arg:
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
static int handle_ej_wl_auth_list(const WLANCONFIG_LIST *src, void *arg)
{
	struct ej_wl_auth_list_priv_s *priv = arg;
	char *value;

	if (!src || !arg)
		return -1;

	if (priv->firstRow == 1)
		priv->firstRow = 0;
	else
		websWrite(priv->wp, ", ");

	websWrite(priv->wp, "[");

	websWrite(priv->wp, "\"%s\"", src->addr);
	value = "YES";
	websWrite(priv->wp, ", \"%s\"", value);
	value = "";
	websWrite(priv->wp, ", \"%s\"", value);
	websWrite(priv->wp, "]");

	return 0;
}

int ej_wl_auth_list(int eid, webs_t wp, int argc, char_t **argv)
{
//only for ath0 & ath1
	int unit, ret = 0;
	char ifname[IFNAMSIZ], *next;
	struct ej_wl_auth_list_priv_s priv = { .firstRow = 1, .wp = wp };

	unit = 0;
	foreach(ifname, nvram_safe_get("wl_ifnames"), next) {
		if (unit >= MAX_NR_WL_IF)
			break;
		SKIP_ABSENT_BAND_AND_INC_UNIT(unit);

		__get_qca_sta_info_by_ifname(ifname, 0, handle_ej_wl_auth_list, &priv);
		++unit;
	}
	
	return ret;
}

#define target 7
char str[target][40]={"Address:","ESSID:","Frequency:","Quality=","Encryption key:","IE:","Authentication Suites"};
static int wl_scan(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
   	int apCount=0,retval=0;
	char header[128];
	char tmp[128], prefix[] = "wlXXXXXXXXXX_";
	char cmd[300];
	FILE *fp;
	char buf[target][200];
	int i,fp_len;
	char *pt1,*pt2;
	char a1[10],a2[10];
	char ssid_str[256];
	char ch[4] = "", ssid[33] = "", address[18] = "", enc[9] = "";
	char auth[16] = "", sig[9] = "", wmode[8] = "";
	int  lock;
#if defined(RTCONFIG_QCA_LBD)
	int restart_lbd = 0;
#endif

	dbg("Please wait...");
#if defined(RTCONFIG_QCA_LBD)
	if (nvram_match("smart_connect_x", "1") && pids("lbd")) {
		eval("rc", "rc_service", "stop_qca_lbd");
		restart_lbd = 1;
	}
#endif
	lock = file_lock("nvramcommit");
	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	snprintf(cmd, sizeof(cmd), "iwlist %s scanning", nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp))));
	fp = popen(cmd, "r");
	file_unlock(lock);
#if defined(RTCONFIG_QCA_LBD)
	if (restart_lbd) {
		eval("rc", "rc_service", "start_qca_lbd");
	}
#endif
	
	if (fp == NULL)
		return -1;
	
	memset(header, 0, sizeof(header));
	snprintf(header, sizeof(header), "%-4s%-33s%-18s%-9s%-16s%-9s%-8s\n", "Ch", "SSID", "BSSID", "Enc", "Auth", "Siganl(%)", "W-Mode");

	dbg("\n%s", header);

	retval += websWrite(wp, "[");
	while(1)
	{
		memset(buf,0,sizeof(buf));
		fp_len=0;
		for(i=0;i<target;i++)
		{
		   	while(fgets(buf[i], sizeof(buf[i]), fp))
			{
				fp_len += safe_strlen(buf[i]);  	
				if(i!=0 && strstr(buf[i],"Cell") && strstr(buf[i],"Address"))
				{
					fseek(fp,-fp_len, SEEK_CUR);
					fp_len=0;
					break;
				}
				else
			  	{ 	   
					if(strstr(buf[i],str[i]))
					{
					 	fp_len =0;  	
						break;
					}	
					else
						memset(buf[i],0,sizeof(buf[i]));
				}	

			}
		        	
	      		//dbg("buf[%d]=%s\n",i,buf[i]);
		}

  		if(feof(fp)) 
		   break;

		apCount++;

		dbg("\napCount=%d\n",apCount);
		//ch
	        pt1 = strstr(buf[2], "Channel ");	
		if(pt1)
		{

			pt2 = strstr(pt1,")");
		   	memset(ch,0,sizeof(ch));
			strlcpy(ch, pt1+safe_strlen("Channel "), min(sizeof(ch), pt2-pt1-safe_strlen("Channel ")+1));
		}   

		//ssid
	        pt1 = strstr(buf[1], "ESSID:");	
		if(pt1)
		{
		   	memset(ssid,0,sizeof(ssid));
			strlcpy(ssid, pt1+safe_strlen("ESSID:")+1, min(sizeof(ssid), safe_strlen(buf[1])-2-(pt1+safe_strlen("ESSID:")+1-buf[1])+1));
		}   


		//bssid
	        pt1 = strstr(buf[0], "Address: ");	
		if(pt1)
		{
		   	memset(address,0,sizeof(address));
			strlcpy(address, pt1+safe_strlen("Address: "), min(sizeof(address), safe_strlen(buf[0])-(pt1+safe_strlen("Address: ")-buf[0])-1+1));
		}   
	

		//enc
		pt1=strstr(buf[4],"Encryption key:");
		if(pt1)
		{   
			if(strstr(pt1+safe_strlen("Encryption key:"),"on"))
			{  	
				strlcpy(enc, "ENC", sizeof(enc));
		
			} 
			else
				strlcpy(enc, "NONE", sizeof(enc));
		}

		//auth
		memset(auth,0,sizeof(auth));
		strlcpy(auth, "N/A", sizeof(auth));

		//sig
	        pt1 = strstr(buf[3], "Quality=");	
		pt2 = NULL;
		if (pt1 != NULL)
			pt2 = strstr(pt1,"/");
		if(pt1 && pt2)
		{
			memset(sig,0,sizeof(sig));
			memset(a1,0,sizeof(a1));
			memset(a2,0,sizeof(a2));
			strlcpy(a1, pt1+safe_strlen("Quality="), min(sizeof(a1), pt2-pt1-safe_strlen("Quality=")+1));
			strlcpy(a2, pt2+1, min(sizeof(a2), strstr(pt2," ")-(pt2+1)+1));
			snprintf(sig, sizeof(sig), "%d",safe_atoi(a1)/safe_atoi(a2));

		}   

		//wmode
		memset(wmode,0,sizeof(wmode));
		strlcpy(wmode, "11b/g/n", sizeof(wmode));


#if 1
		dbg("%-4s%-33s%-18s%-9s%-16s%-9s%-8s\n",ch,ssid,address,enc,auth,sig,wmode);
#endif	


		memset(ssid_str, 0, sizeof(ssid_str));
		char_to_ascii(ssid_str, trim_r(ssid));
		if (apCount==1)
			retval += websWrite(wp, "[\"%s\", \"%s\"]", ssid_str, address);
		else
			retval += websWrite(wp, ", [\"%s\", \"%s\"]", ssid_str, address);

	}

	retval += websWrite(wp, "]");
	pclose(fp);
	return 0;
}   

int
ej_wl_scan(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_scan(eid, wp, argc, argv, 0);
}

int
ej_wl_scan_2g(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_scan(eid, wp, argc, argv, 0);
}

int
ej_wl_scan_5g(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_scan(eid, wp, argc, argv, 1);
}

int
ej_wl_scan_5g_2(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_scan(eid, wp, argc, argv, 2);
}


static int ej_wl_channel_list(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
	int retval = 0;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_";
	char *country_code;
	char chList[256];

	if (absent_band(unit))
		return 0;

#if defined(RTCONFIG_LYRA_5G_SWAP)
#if defined(RTCONFIG_WIFI_SON)
	if(nvram_match("wifison_ready","1"))
		goto bypass;
	else
#endif
	{
		if(unit==1)
			unit=2;
		else if(unit==2)
			unit=1;
	}
bypass:
#endif

	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	country_code = nvram_get(strlcat_r(prefix, "country_code", tmp, sizeof(tmp)));

	if (country_code == NULL || safe_strlen(country_code) != 2) return retval;

	//try getting channel list via wifi driver first
#if defined(RTAC58U) || defined(RTAC59U)
	if (unit == 0 && (!strncmp(nvram_safe_get("territory_code"), "CX/01", 5)
		       || !strncmp(nvram_safe_get("territory_code"), "CX/05", 5)))
		retval += websWrite(wp, "[1,2,3,4,5,6,7,8,9,10,11]");
	else
#endif
	if(get_channel_list_via_driver(unit, chList, sizeof(chList)) > 0)
	{
		retval += websWrite(wp, "[%s]", chList);
	}
	else if(get_channel_list_via_country(unit, country_code, chList, sizeof(chList)) > 0)
	{
		retval += websWrite(wp, "[%s]", chList);
	}
	return retval;
}

static const struct g_bw40chanspec_s {
	uint64_t ch_mask;
	uint64_t bw_mask;
	char *tag;
} g_5gbw40chanspec_tbl[] = {
	{  CH40_M,  CH36_M |  CH40_M, "u" },
	{  CH48_M,  CH44_M |  CH48_M, "u" },
	{  CH56_M,  CH52_M |  CH56_M, "u" },
	{  CH64_M,  CH60_M |  CH64_M, "u" },
	{ CH104_M, CH100_M | CH104_M, "u" },
	{ CH112_M, CH108_M | CH112_M, "u" },
	{ CH120_M, CH116_M | CH120_M, "u" },
	{ CH128_M, CH124_M | CH128_M, "u" },
	{ CH136_M, CH132_M | CH136_M, "u" },
	{ CH144_M, CH140_M | CH144_M, "u" },
	{ CH153_M, CH149_M | CH153_M, "u" },
	{ CH161_M, CH157_M | CH161_M, "u" },
	{ CH169_M, CH165_M | CH169_M, "u" },
	{ CH177_M, CH173_M | CH177_M, "u" },

	{  CH36_M,  CH36_M |  CH40_M, "l" },
	{  CH44_M,  CH44_M |  CH48_M, "l" },
	{  CH52_M,  CH52_M |  CH56_M, "l" },
	{  CH60_M,  CH60_M |  CH64_M, "l" },
	{ CH100_M, CH100_M | CH104_M, "l" },
	{ CH108_M, CH108_M | CH112_M, "l" },
	{ CH116_M, CH116_M | CH120_M, "l" },
	{ CH124_M, CH124_M | CH128_M, "l" },
	{ CH132_M, CH132_M | CH136_M, "l" },
	{ CH140_M, CH140_M | CH144_M, "l" },
	{ CH149_M, CH149_M | CH153_M, "l" },
	{ CH157_M, CH157_M | CH161_M, "l" },
	{ CH165_M, CH165_M | CH169_M, "l" },
	{ CH173_M, CH173_M | CH177_M, "l" },

	{ 0, 0, NULL }
};

static const struct g_bw80pchanspec_s {
	uint64_t bw_mask;
	char *tag;
} g_5gbw80pchanspec_tbl[] = {
	{  CH36_M |  CH40_M |  CH44_M |  CH48_M, "/80" },
	{  CH52_M |  CH56_M |  CH60_M |  CH64_M, "/80" },
	{ CH100_M | CH104_M | CH108_M | CH112_M, "/80" },
	{ CH116_M | CH120_M | CH124_M | CH128_M, "/80" },
	{ CH132_M | CH136_M | CH140_M | CH144_M, "/80" },
	{ CH149_M | CH153_M | CH157_M | CH161_M, "/80" },
	{ CH165_M | CH169_M | CH173_M | CH177_M, "/80" },
#if defined(RTCONFIG_BW160M)
	{  CH36_M |  CH40_M |  CH44_M |  CH48_M |  CH52_M |  CH56_M |  CH60_M |  CH64_M, "/160" },
	{ CH100_M | CH104_M | CH108_M | CH112_M | CH116_M | CH120_M | CH124_M | CH128_M, "/160" },
	{ CH149_M | CH153_M | CH157_M | CH161_M | CH165_M | CH169_M | CH173_M | CH177_M, "/160" },
#endif
#if defined(RTCONFIG_BW240M)
	{ CH100_M | CH104_M | CH108_M | CH112_M | CH116_M | CH120_M | CH124_M | CH128_M | CH132_M | CH136_M | CH140_M | CH144_M, "/240" },
#endif

	{ 0, NULL },
#if defined(RTCONFIG_HAS_6G)
}, g_6gbw40pchanspec_tbl[] = {
	{   B6GCH1_M |   B6GCH5_M, "/40" },
	{   B6GCH9_M |  B6GCH13_M, "/40" },
	{  B6GCH17_M |  B6GCH21_M, "/40" },
	{  B6GCH25_M |  B6GCH29_M, "/40" },
	{  B6GCH33_M |  B6GCH37_M, "/40" },
	{  B6GCH41_M |  B6GCH45_M, "/40" },
	{  B6GCH49_M |  B6GCH53_M, "/40" },
	{  B6GCH57_M |  B6GCH61_M, "/40" },
	{  B6GCH65_M |  B6GCH69_M, "/40" },
	{  B6GCH73_M |  B6GCH77_M, "/40" },
	{  B6GCH81_M |  B6GCH85_M, "/40" },
	{  B6GCH89_M |  B6GCH93_M, "/40" },
	{  B6GCH97_M | B6GCH101_M, "/40" },
	{ B6GCH105_M | B6GCH109_M, "/40" },
	{ B6GCH113_M | B6GCH117_M, "/40" },
	{ B6GCH121_M | B6GCH125_M, "/40" },
	{ B6GCH129_M | B6GCH133_M, "/40" },
	{ B6GCH137_M | B6GCH141_M, "/40" },
	{ B6GCH145_M | B6GCH149_M, "/40" },
	{ B6GCH153_M | B6GCH157_M, "/40" },
	{ B6GCH161_M | B6GCH165_M, "/40" },
	{ B6GCH169_M | B6GCH173_M, "/40" },
	{ B6GCH177_M | B6GCH181_M, "/40" },
	{ B6GCH185_M | B6GCH189_M, "/40" },
	{ B6GCH193_M | B6GCH197_M, "/40" },
	{ B6GCH201_M | B6GCH205_M, "/40" },
	{ B6GCH209_M | B6GCH213_M, "/40" },
	{ B6GCH217_M | B6GCH221_M, "/40" },
	{ B6GCH225_M | B6GCH229_M, "/40" },

	{   B6GCH1_M |   B6GCH5_M |   B6GCH9_M |  B6GCH13_M, "/80" },
	{  B6GCH17_M |  B6GCH21_M |  B6GCH25_M |  B6GCH29_M, "/80" },
	{  B6GCH33_M |  B6GCH37_M |  B6GCH41_M |  B6GCH45_M, "/80" },
	{  B6GCH49_M |  B6GCH53_M |  B6GCH57_M |  B6GCH61_M, "/80" },
	{  B6GCH65_M |  B6GCH69_M |  B6GCH73_M |  B6GCH77_M, "/80" },
	{  B6GCH81_M |  B6GCH85_M |  B6GCH89_M |  B6GCH93_M, "/80" },
	{  B6GCH97_M | B6GCH101_M | B6GCH105_M | B6GCH109_M, "/80" },
	{ B6GCH113_M | B6GCH117_M | B6GCH121_M | B6GCH125_M, "/80" },
	{ B6GCH129_M | B6GCH133_M | B6GCH137_M | B6GCH141_M, "/80" },
	{ B6GCH145_M | B6GCH149_M | B6GCH153_M | B6GCH157_M, "/80" },
	{ B6GCH161_M | B6GCH165_M | B6GCH169_M | B6GCH173_M, "/80" },
	{ B6GCH177_M | B6GCH181_M | B6GCH185_M | B6GCH189_M, "/80" },
	{ B6GCH193_M | B6GCH197_M | B6GCH201_M | B6GCH205_M, "/80" },
	{ B6GCH209_M | B6GCH213_M | B6GCH217_M | B6GCH221_M, "/80" },

#if defined(RTCONFIG_BW160M)
	{   B6GCH1_M |   B6GCH5_M |   B6GCH9_M |  B6GCH13_M |  B6GCH17_M |  B6GCH21_M |  B6GCH25_M |  B6GCH29_M, "/160" },
	{  B6GCH33_M |  B6GCH37_M |  B6GCH41_M |  B6GCH45_M |  B6GCH49_M |  B6GCH53_M |  B6GCH57_M |  B6GCH61_M, "/160" },
	{  B6GCH65_M |  B6GCH69_M |  B6GCH73_M |  B6GCH77_M |  B6GCH81_M |  B6GCH85_M |  B6GCH89_M |  B6GCH93_M, "/160" },
	{  B6GCH97_M | B6GCH101_M | B6GCH105_M | B6GCH109_M | B6GCH113_M | B6GCH117_M | B6GCH121_M | B6GCH125_M, "/160" },
	{ B6GCH129_M | B6GCH133_M | B6GCH137_M | B6GCH141_M | B6GCH145_M | B6GCH149_M | B6GCH153_M | B6GCH157_M, "/160" },
	{ B6GCH161_M | B6GCH165_M | B6GCH169_M | B6GCH173_M | B6GCH177_M | B6GCH181_M | B6GCH185_M | B6GCH189_M, "/160" },
	{ B6GCH193_M | B6GCH197_M | B6GCH201_M | B6GCH205_M | B6GCH209_M | B6GCH213_M | B6GCH217_M | B6GCH221_M, "/160" },
#endif

	{   B6GCH1_M |   B6GCH5_M |   B6GCH9_M |  B6GCH13_M |  B6GCH17_M |  B6GCH21_M |  B6GCH25_M |  B6GCH29_M |  B6GCH33_M |  B6GCH37_M |  B6GCH41_M |  B6GCH45_M |  B6GCH49_M |  B6GCH53_M |  B6GCH57_M |  B6GCH61_M, "/320-1" },
	{  B6GCH33_M |  B6GCH37_M |  B6GCH41_M |  B6GCH45_M |  B6GCH49_M |  B6GCH53_M |  B6GCH57_M |  B6GCH61_M |  B6GCH65_M |  B6GCH69_M |  B6GCH73_M |  B6GCH77_M |  B6GCH81_M |  B6GCH85_M |  B6GCH89_M |  B6GCH93_M, "/320-2" },
	{  B6GCH65_M |  B6GCH69_M |  B6GCH73_M |  B6GCH77_M |  B6GCH81_M |  B6GCH85_M |  B6GCH89_M |  B6GCH93_M |  B6GCH97_M | B6GCH101_M | B6GCH105_M | B6GCH109_M | B6GCH113_M | B6GCH117_M | B6GCH121_M | B6GCH125_M, "/320-1" },
	{  B6GCH97_M | B6GCH101_M | B6GCH105_M | B6GCH109_M | B6GCH113_M | B6GCH117_M | B6GCH121_M | B6GCH125_M | B6GCH129_M | B6GCH133_M | B6GCH137_M | B6GCH141_M | B6GCH145_M | B6GCH149_M | B6GCH153_M | B6GCH157_M, "/320-2" },
	{ B6GCH129_M | B6GCH133_M | B6GCH137_M | B6GCH141_M | B6GCH145_M | B6GCH149_M | B6GCH153_M | B6GCH157_M | B6GCH161_M | B6GCH165_M | B6GCH169_M | B6GCH173_M | B6GCH177_M | B6GCH181_M | B6GCH185_M | B6GCH189_M, "/320-1" },
	{ B6GCH161_M | B6GCH165_M | B6GCH169_M | B6GCH173_M | B6GCH177_M | B6GCH181_M | B6GCH185_M | B6GCH189_M | B6GCH193_M | B6GCH197_M | B6GCH201_M | B6GCH205_M | B6GCH209_M | B6GCH213_M | B6GCH217_M | B6GCH221_M, "/320-2" },

	{ 0, NULL },
#endif
};

/* Get chanspec.
 * GT-AC5300 example:
 * chanspecs_2g:
 * [ "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "5u", "6u", "7u", "8u", "9u", "10u", "11u", "1l", "2l", "3l", "4l", "5l", "6l", "7l" ]
 * chanspecs_5g:
 * [ "36", "40", "44", "48", "40u", "48u", "36l", "44l", "36/80", "40/80", "44/80", "48/80" ]
 * chanspecs_5g_2:
 * [ "149", "153", "157", "161", "165", "153u", "161u", "149l", "157l", "149/80", "153/80", "157/80", "161/80" ]
 * chanspecs_6g:
 * [ "0" ]
 * RT-BE?? example, EU sku:
 * chanspecs_2g:
 * [ "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13",
 *   "1l", "5u", "2l", "6u", "3l", "7u", "4l", "8u", "5l", "9u", "6l",
 *   "10u", "7l", "11u", "8l", "12u", "9l", "13u" ]
 * RT-BE96U example:
 * chanspecs_6g:
 * [ "6g1", "6g5", "6g9", "6g13", "6g17", "6g21", "6g25", "6g29", "6g33", "6g37", "6g41", "6g45", "6g49", "6g53", "6g57", "6g61",
 *   "6g65", "6g69", "6g73", "6g77", "6g81", "6g85", "6g89", "6g93", "6g97", "6g101", "6g105", "6g109", "6g113", "6g117", "6g121",
 *   "6g125", "6g129", "6g133", "6g137", "6g141", "6g145", "6g149", "6g153", "6g157", "6g161", "6g165", "6g169", "6g173", "6g177",
 *   "6g181", "6g185", "6g189", "6g193", "6g197", "6g201", "6g205", "6g209", "6g213", "6g217", "6g221", "6g225", "6g229",
 *   "6g1/40", "6g5/40", "6g9/40", "6g13/40", "6g17/40", "6g21/40", "6g25/40", "6g29/40", "6g33/40", "6g37/40", "6g41/40", "6g45/40",
 *   "6g49/40", "6g53/40", "6g57/40", "6g61/40", "6g65/40", "6g69/40", "6g73/40", "6g77/40", "6g81/40", "6g85/40", "6g89/40", "6g93/40",
 *   "6g97/40", "6g101/40", "6g105/40", "6g109/40", "6g113/40", "6g117/40", "6g121/40", "6g125/40", "6g129/40", "6g133/40", "6g137/40", "6g141/40",
 *   "6g145/40", "6g149/40", "6g153/40", "6g157/40", "6g161/40", "6g165/40", "6g169/40", "6g173/40", "6g177/40", "6g181/40", "6g185/40", "6g189/40",
 *   "6g193/40", "6g197/40", "6g201/40", "6g205/40", "6g209/40", "6g213/40", "6g217/40", "6g221/40", "6g225/40", "6g229/40",
 *   "6g1/80", "6g5/80", "6g9/80", "6g13/80", "6g17/80", "6g21/80", "6g25/80", "6g29/80", "6g33/80", "6g37/80", "6g41/80", "6g45/80",
 *   "6g49/80", "6g53/80", "6g57/80", "6g61/80", "6g65/80", "6g69/80", "6g73/80", "6g77/80", "6g81/80", "6g85/80", "6g89/80", "6g93/80",
 *   "6g97/80", "6g101/80", "6g105/80", "6g109/80", "6g113/80", "6g117/80", "6g121/80", "6g125/80", "6g129/80", "6g133/80", "6g137/80", "6g141/80",
 *   "6g145/80", "6g149/80", "6g153/80", "6g157/80", "6g161/80", "6g165/80", "6g169/80", "6g173/80", "6g177/80", "6g181/80", "6g185/80", "6g189/80",
 *   "6g193/80", "6g197/80", "6g201/80", "6g205/80", "6g209/80", "6g213/80", "6g217/80", "6g221/80",
 *   "6g1/160", "6g5/160", "6g9/160", "6g13/160", "6g17/160", "6g21/160", "6g25/160", "6g29/160", "6g33/160", "6g37/160", "6g41/160", "6g45/160",
 *   "6g49/160", "6g53/160", "6g57/160", "6g61/160", "6g65/160", "6g69/160", "6g73/160", "6g77/160", "6g81/160", "6g85/160", "6g89/160", "6g93/160",
 *   "6g97/160", "6g101/160", "6g105/160", "6g109/160", "6g113/160", "6g117/160", "6g121/160", "6g125/160", "6g129/160", "6g133/160", "6g137/160", "6g141/160",
 *   "6g145/160", "6g149/160", "6g153/160", "6g157/160", "6g161/160", "6g165/160", "6g169/160", "6g173/160", "6g177/160", "6g181/160", "6g185/160", "6g189/160",
 *   "6g193/160", "6g197/160", "6g201/160", "6g205/160", "6g209/160", "6g213/160", "6g217/160", "6g221/160",
 *   "6g1/320-1", "6g5/320-1", "6g9/320-1", "6g13/320-1", "6g17/320-1", "6g21/320-1", "6g25/320-1", "6g29/320-1",
 *   "6g33/320-1", "6g37/320-1", "6g41/320-1", "6g45/320-1", "6g49/320-1", "6g53/320-1","6g57/320-1", "6g61/320-1",
 *   "6g33/320-2", "6g37/320-2", "6g41/320-2", "6g45/320-2", "6g49/320-2", "6g53/320-2", "6g57/320-2", "6g61/320-2",
 *   "6g65/320-2", "6g69/320-2", "6g73/320-2", "6g77/320-2", "6g81/320-2", "6g85/320-2", "6g89/320-2", "6g93/320-2",
 *   "6g65/320-1", "6g69/320-1", "6g73/320-1", "6g77/320-1", "6g81/320-1", "6g85/320-1", "6g89/320-1", "6g93/320-1",
 *   "6g97/320-1", "6g101/320-1", "6g105/320-1", "6g109/320-1", "6g113/320-1", "6g117/320-1", "6g121/320-1", "6g125/320-1",
 *   "6g97/320-2", "6g101/320-2", "6g105/320-2", "6g109/320-2", "6g113/320-2", "6g117/320-2", "6g121/320-2", "6g125/320-2",
 *   "6g129/320-2", "6g133/320-2", "6g137/320-2", "6g141/320-2", "6g145/320-2", "6g149/320-2", "6g153/320-2", "6g157/320-2",
 *   "6g129/320-1", "6g133/320-1", "6g137/320-1", "6g141/320-1", "6g145/320-1", "6g149/320-1", "6g153/320-1", "6g157/320-1",
 *   "6g161/320-1", "6g165/320-1", "6g169/320-1", "6g173/320-1", "6g177/320-1", "6g181/320-1", "6g185/320-1", "6g189/320-1",
 *   "6g161/320-2", "6g165/320-2", "6g169/320-2", "6g173/320-2", "6g177/320-2", "6g181/320-2", "6g185/320-2", "6g189/320-2",
 *   "6g193/320-2", "6g197/320-2", "6g201/320-2", "6g205/320-2", "6g209/320-2", "6g213/320-2", "6g217/320-2", "6g221/320-2"
 * ]
 * chanspecs_6g_2:
 * [ "0" ]
 *
 * WARNING: @wp may be NULL!
 */
int ej_wl_chanspecs(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
#if defined(RTCONFIG_BW240M)
	int no240m = 0;
#else
	const int no240m = 1;
#endif
	int retval = 0;
	char *ch_prefix = "", prefix[sizeof("wlX_XX")];
	char ch_list[2 * 4 + 3 * 22 + 4 * 34 + 8] = "";
	char tmp_ch_list[sizeof("6gXXX\", \"") * 59] = "";
	char tmp_chansps[sizeof("6g221/320-2XX")];
	char chansps_buf[2970] = "";	/* All 6G channel, except ch2, needs 2959 bytes. */
	uint64_t m, c_m, bw_m, chlist_mask = 0;
	const struct g_bw40chanspec_s *p = NULL;
	const struct g_bw80pchanspec_s *q = NULL;

	if (unit < 0 || unit >= WL_NR_BANDS || absent_band(unit))
		goto exit_ej_wl_chanspecs;

	if (get_channel_list_via_driver(unit, ch_list, sizeof(ch_list)) <= 0)
		goto exit_ej_wl_chanspecs;

	if (!(chlist_mask = chlist2bitmask(unit, ch_list, ",")))
		goto exit_ej_wl_chanspecs;

	if (unit == WL_6G_BAND) {
		ch_prefix = "6g";
	}

	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
#if defined(RTCONFIG_BW240M)
	/* Don't show "Enable 240MHz" if not FCC sku. */
	if (!nvram_pf_match(prefix, "country_code", "US")
#if defined(RTCONFIG_TCODE2CC)
	 && !nvram_pf_match(prefix, "country_code", "TW")
#endif
	)
		no240m = 1;
#if defined(BD4D5)
	if (nvram_match("odmpid","ZenWiFi_BD4")
	 || nvram_match("odmpid","ZenWiFi_BE3600"))
		no240m = 1;
#endif
#if defined(BD4_OD)
	if (nvram_match("odmpid","ZenWiFi_BD4_Outdoor"))
		no240m = 1;
#endif
#endif	/* RTCONFIG_BW240M */

	/* Generate 20MHz channelspecslist. */
	if (is_6g(unit)) {
		__bitmask2chlist(unit, chlist_mask, "\", \"6g", tmp_ch_list, sizeof(tmp_ch_list));
		if (wp) {
			retval += websWrite(wp, "[ \"%s%s", ch_prefix, tmp_ch_list);
		}
		__bitmask2chlist(unit, chlist_mask, " 6g", tmp_ch_list, sizeof(tmp_ch_list));
	} else {
		__bitmask2chlist(unit, chlist_mask, "\", \"", tmp_ch_list, sizeof(tmp_ch_list));
		if (wp) {
			retval += websWrite(wp, "[ \"%s%s", ch_prefix, tmp_ch_list);
		}
		__bitmask2chlist(unit, chlist_mask, " ", tmp_ch_list, sizeof(tmp_ch_list));
	}
	snprintf(chansps_buf, sizeof(chansps_buf), "%s%s", ch_prefix, tmp_ch_list);

	/* Generate 40MHz,u 40MHz,l 80MHz, 160MHz channelspecs. */
	if (unit == WL_2G_BAND) {
		/* If ch-4 exist, print ch and append "u".
		 * If ch+4 exist, print ch and append "l".
		 */
		for (m = (1 << 4), c_m = chlist_mask; c_m && m; m <<= 1) {
			if (!(chlist_mask & m) || !(chlist_mask & (m >> 4)))
				continue;

			if (wp) {
				retval += websWrite(wp, "\", \"%su", bitmask2chlist(unit, m, ""));
			}
			snprintf(tmp_chansps, sizeof(tmp_chansps), " %su", bitmask2chlist(unit, m, ""));
			strlcat(chansps_buf, tmp_chansps, sizeof(chansps_buf));
			c_m &= ~m;
		}
		for (m = 1, c_m = chlist_mask; c_m && m; m <<= 1) {
			if (!(chlist_mask & m) || !(chlist_mask & (m << 4)))
				continue;

			if (wp) {
				retval += websWrite(wp, "\", \"%sl", bitmask2chlist(unit, m, ""));
			}
			snprintf(tmp_chansps, sizeof(tmp_chansps), " %sl", bitmask2chlist(unit, m, ""));
			strlcat(chansps_buf, tmp_chansps, sizeof(chansps_buf));
			c_m &= ~m;
		}
	} else if (is_5g(unit)) {
		p = g_5gbw40chanspec_tbl;
#if defined(RTCONFIG_HAS_6G)
	} else if (is_6g(unit)) {
		p = NULL;	/* u,l format is replaced by /40 */
#endif
	} else {
		_dprintf("%s: 40MHz chanspec of unit %d hasn't been defined!\n", __func__, unit);
	}

	for (; p && p->ch_mask && p->bw_mask && p->tag; ++p) {
		if (!(p->ch_mask & chlist_mask))
			continue;
		if ((p->bw_mask & chlist_mask) != p->bw_mask)
			continue;

		if (wp) {
			retval += websWrite(wp, "\", \"%s%s%s", ch_prefix,
				bitmask2chlist(unit, p->ch_mask, ""), p->tag);
		}
		snprintf(tmp_chansps, sizeof(tmp_chansps), " %s%s%s", ch_prefix,
			bitmask2chlist(unit, p->ch_mask, ""), p->tag);
		strlcat(chansps_buf, tmp_chansps, sizeof(chansps_buf));
	}

	/* Generate 80MHz, 160MHz, 240MHz, 320MHz chanspecs. */
	if (unit == WL_2G_BAND)
		q = NULL;
	else if (is_5g(unit)) {
		q = g_5gbw80pchanspec_tbl;
#if defined(RTCONFIG_HAS_6G)
	} else if (is_6g(unit)) {
		q = g_6gbw40pchanspec_tbl;
#endif
	} else {
		_dprintf("%s: 80+MHz chanspec of unit %d hasn't been defined!\n", __func__, unit);
	}

	for (; q && q->bw_mask && q->tag; ++q) {
		if ((chlist_mask & q->bw_mask) != q->bw_mask)
			continue;

		for (m = 1, bw_m = q->bw_mask; bw_m && m; m <<= 1) {
			if (!(bw_m & m))
				continue;
			if (no240m && strstr(q->tag,"240"))
				continue;

			if (wp) {
				retval += websWrite(wp, "\", \"%s%s%s", ch_prefix,
					bitmask2chlist(unit, m, ""), q->tag);
			}
			snprintf(tmp_chansps, sizeof(tmp_chansps), " %s%s%s", ch_prefix,
				bitmask2chlist(unit, m, ""), q->tag);
			strlcat(chansps_buf, tmp_chansps, sizeof(chansps_buf));
			bw_m &= ~m;
		}
	}

exit_ej_wl_chanspecs:
	if (wp && !retval)
		retval += websWrite(wp, "[ \"0\" ]");
	else if (wp && retval)
		retval += websWrite(wp, "\" ]");
	nvram_pf_set(prefix, "chansps", chansps_buf);

	return retval;
}

int
ej_wl_channel_list_2g(int eid, webs_t wp, int argc, char_t **argv)
{
	return ej_wl_channel_list(eid, wp, argc, argv, WL_2G_BAND);
}

int
ej_wl_channel_list_5g(int eid, webs_t wp, int argc, char_t **argv)
{
	return ej_wl_channel_list(eid, wp, argc, argv, WL_5G_BAND);
}

int
ej_wl_channel_list_5g_2(int eid, webs_t wp, int argc, char_t **argv)
{
	return ej_wl_channel_list(eid, wp, argc, argv, WL_5G_2_BAND);
}

int
ej_wl_channel_list_6g(int eid, webs_t wp, int argc, char_t **argv)
{
	return ej_wl_channel_list(eid, wp, argc, argv, WL_6G_BAND);
}

int
ej_wl_channel_list_6g_2(int eid, webs_t wp, int argc, char_t **argv)
{
	return ej_wl_channel_list(eid, wp, argc, argv, WL_6G_2_BAND);
}

int
ej_wl_channel_list_60g(int eid, webs_t wp, int argc, char_t **argv)
{
	return ej_wl_channel_list(eid, wp, argc, argv, WL_60G_BAND);
}

int ej_wl_chanspecs_2g(int eid, webs_t wp, int argc, char_t **argv)
{
	return ej_wl_chanspecs(eid, wp, argc, argv, WL_2G_BAND);
}

int ej_wl_chanspecs_5g(int eid, webs_t wp, int argc, char_t **argv)
{
	return ej_wl_chanspecs(eid, wp, argc, argv, WL_5G_BAND);
}

int ej_wl_chanspecs_5g_2(int eid, webs_t wp, int argc, char_t **argv)
{
#if defined(RTCONFIG_HAS_5G_2)
	int band = WL_5G_2_BAND;
#else
	int band = -1;
#endif
	return ej_wl_chanspecs(eid, wp, argc, argv, band);
}

int ej_wl_chanspecs_6g(int eid, webs_t wp, int argc, char_t **argv)
{
	return ej_wl_chanspecs(eid, wp, argc, argv, runtime_has_6g()? WL_6G_BAND : -1);
}

int ej_wl_chanspecs_6g_2(int eid, webs_t wp, int argc, char_t **argv)
{
#if defined(RTCONFIG_HAS_6G_2)
	int band = WL_6G_2_BAND;
#else
	int band = -1;
#endif

	return ej_wl_chanspecs(eid, wp, argc, argv, band);
}

static int ej_wl_rate(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
#define ASUS_IOCTL_GET_STA_DATARATE (SIOCDEVPRIVATE+15) /* from qca-wifi/os/linux/include/ieee80211_ioctl.h */
        struct iwreq wrq;
	int retval = 0;
	char tmp[256], prefix[sizeof("wlXXXXXXXXXX_")];
	const char *name;
	unsigned int rate[2];
	char rate_buf[32] = "0 Mbps";
	int sw_mode = sw_mode();
	int wlc_band = nvram_get_int("wlc_band");
	int from_app = 0;

	if (sw_mode != SW_MODE_REPEATER && sw_mode != SW_MODE_HOTSPOT)
		goto ERROR;
	if (absent_band(unit))
		goto ERROR;

	from_app = check_user_agent(user_agent);

	if (wlc_band < 0 || !nvram_match("wlc_state", "2"))
		goto ERROR;

#ifdef RTCONFIG_CONCURRENTREPEATER
	wlc_band = -1;
	snprintf(prefix, sizeof(prefix), "wlc%d_", unit);
	if (!nvram_pf_match(prefix, "state", "2"))
		goto ERROR;
#endif

	if (wlc_band >= WL_2G_BAND && wlc_band != unit)
		goto ERROR;

#if defined(RTCONFIG_REPEATER_STAALLBAND)
	name = get_staifname(nvram_get_int("wlc_triBand"));
#else
	snprintf(prefix, sizeof(prefix), "wl%d.1_", unit);
	name = nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp)));
#endif

	wrq.u.data.pointer = rate;
	wrq.u.data.length = sizeof(rate);

	if (wl_ioctl(name, ASUS_IOCTL_GET_STA_DATARATE, &wrq) < 0)
	{
		dbg("%s: errors in getting %s ASUS_IOCTL_GET_STA_DATARATE result\n", __func__, name);
		goto ERROR;
	}

	if (rate[0] > rate[1])
		snprintf(rate_buf, sizeof(rate_buf), "%d Mbps", rate[0]);
	else
		snprintf(rate_buf, sizeof(rate_buf), "%d Mbps", rate[1]);

ERROR:
	if(from_app == 0 && hook_get_json == 0)
		retval += websWrite(wp, "%s", rate_buf);
	else
		retval += websWrite(wp, "\"%s\"", rate_buf);
	return retval;
}


int
ej_wl_rate_2g(int eid, webs_t wp, int argc, char_t **argv)
{
	if(sw_mode() == SW_MODE_REPEATER)
		return ej_wl_rate(eid, wp, argc, argv, WL_2G_BAND);
	else if(check_user_agent(user_agent) != FROM_BROWSER)
		return websWrite(wp, "\"\"");
	else
		return websWrite(wp, "%s", "");
}

int
ej_wl_rate_5g(int eid, webs_t wp, int argc, char_t **argv)
{
	if(sw_mode() == SW_MODE_REPEATER)
		return ej_wl_rate(eid, wp, argc, argv, WL_5G_BAND);
	else if(check_user_agent(user_agent) != FROM_BROWSER)
		return websWrite(wp, "\"\"");
	else
		return websWrite(wp, "%s", "");
}

int
ej_wl_rate_5g_2(int eid, webs_t wp, int argc, char_t **argv)
{
#if defined(RTCONFIG_HAS_6G) && !defined(RTCONFIG_HAS_5G_2)
	/* Workaround. Because GUI get rate from the hook when wlc_band is two,
	 * whether band two is 6G or 5G2.
	 */
	int band = WL_6G_BAND;
#else
	int band = WL_5G_2_BAND;
#endif

	if(sw_mode() == SW_MODE_REPEATER)
		return ej_wl_rate(eid, wp, argc, argv, band);
	else if(check_user_agent(user_agent) != FROM_BROWSER)
		return websWrite(wp, "\"\"");
	else
		return websWrite(wp, "%s", "");
}

#if defined(RTCONFIG_HAS_6G)
int
ej_wl_rate_6g(int eid, webs_t wp, int argc, char_t **argv)
{
	if(sw_mode() == SW_MODE_REPEATER)
		return ej_wl_rate(eid, wp, argc, argv, WL_6G_BAND);
	else if(check_user_agent(user_agent) != FROM_BROWSER)
		return websWrite(wp, "\"\"");
	else
		return websWrite(wp, "%s", "");
}
#else
int
ej_wl_rate_6g(int eid, webs_t wp, int argc, char_t **argv)
{
	if(check_user_agent(user_agent) != FROM_BROWSER)
		return websWrite(wp, "\"\"");
	else
		return websWrite(wp, "%s", "");
}
#endif	/* RTCONFIG_HAS_6G */

#if defined(RTCONFIG_HAS_6G_2)
int
ej_wl_rate_6g_2(int eid, webs_t wp, int argc, char_t **argv)
{
	if (sw_mode() == SW_MODE_REPEATER)
		return ej_wl_rate(eid, wp, argc, argv, WL_6G_2_BAND);
	else if(check_user_agent(user_agent) != FROM_BROWSER)
		return websWrite(wp, "\"\"");
	else
		return websWrite(wp, "%s", "");
}
#else
int
ej_wl_rate_6g_2(int eid, webs_t wp, int argc, char_t **argv)
{
	if(check_user_agent(user_agent) != FROM_BROWSER)
		return websWrite(wp, "\"\"");
	else
		return websWrite(wp, "%s", "");
}

#endif	/* RTCONFIG_HAS_6G_2 */

int
ej_nat_accel_status(int eid, webs_t wp, int argc, char_t **argv)
{
	return websWrite(wp, "%d", nat_acceleration_status());
}

#if defined(RTCONFIG_WIFI_QCN5024_QCN5054) \
 || defined(RTCONFIG_QCA_AXCHIP) \
 || defined(RTCONFIG_QCA_BECHIP)
/* Hook validate_apply().
 * Sync wl[0~2]_yyy with wlx_yyy if yyy in global_params[].
 */
static const char *global_params[] = { "twt", NULL };
void __validate_apply_set_wl_var(char *nv, char *val)
{
	const char **p;
	int band = -1;
	char prefix[sizeof("wlxxx_")];

	if (!nv)
		return;

	strlcpy(prefix, nv, sizeof("wl0_") + 1);
	if (sscanf(prefix, "wl%d_", &band) != 1)
		return;
	if (band < 0 || band >= WL_NR_BANDS || absent_band(band))
		return;

	for (p = &global_params[0]; *p != NULL; ++p) {
		if (strcmp(nv + 4, *p))
			continue;

		for (band = 0; band < MAX_NR_WL_IF; ++band) {
			SKIP_ABSENT_BAND(band);
			snprintf(prefix, sizeof(prefix), "wl%d_", band);
			if (!strncmp(nv, prefix, safe_strlen(prefix)))
				continue;
			nvram_pf_set(prefix, *p, val);
			_dprintf("%s: set %s%s=%s\n", __func__, prefix, *p, val? : "NULL");
		}
		break;
	}
}
#endif

#ifdef RTCONFIG_PROXYSTA
int
ej_wl_auth_psta(int eid, webs_t wp, int argc, char_t **argv)
{
	int retval = 0;
	int psta = 0, psta_auth = 0;

	if(nvram_match("wlc_state", "2")){	//connected
		psta = 1;
		psta_auth = 0;
	//else if(?)				//authorization failed
	//	retval += websWrite(wp, "wlc_state=2;wlc_state_auth=1;");
	}else{					//disconnected
		psta = 0;
		psta_auth = 0;
	}

	if(json_support){
		retval += websWrite(wp, "{");
		retval += websWrite(wp, "\"wlc_state\":\"%d\"", psta);
		retval += websWrite(wp, ",\"wlc_state_auth\":\"%d\"", psta_auth);
		retval += websWrite(wp, "}");
	}else{
		retval += websWrite(wp, "wlc_state=%d;", psta);
		retval += websWrite(wp, "wlc_state_auth=%d;", psta_auth);
	}

	return retval;
}
#endif

const char *syslog_msg_filter[] = {
	"net_ratelimit",
#if defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_SOC_IPQ53XX)
	"[AUTH] vap", "[MLME] vap", "[ASSOC] vap", "[INACT] vap", "LBDR ", "npu_corner", "apc_corner", "Sync active EEPROM set",
	"wlan_send_mgmt", "hapdevent_proc_event", "HAPD:", "WSUP:", "APSTATS:", "THERMAL:", "skb recycler",
#elif defined(RTCONFIG_SOC_IPQ8064)
	"[AUTH] vap", "[MLME] vap", "[ASSOC] vap", "[INACT] vap",
#endif
	"exist in UDB, can't", "is used by someone else, can't use it", "not mesh client, can't update it", "not mesh client, can't delete it",
	"ERROR: [send_redir_page",
	NULL
};
