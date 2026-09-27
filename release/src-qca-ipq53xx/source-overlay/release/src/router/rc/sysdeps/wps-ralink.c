/*
 * Miscellaneous services
 *
 * Copyright (C) 2009, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: services.c,v 1.100 2010/03/04 09:39:18 Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <bcmnvram.h>
#include <shutils.h>
#include <rc.h>
#include <shared.h>

/*
 * input variables:
 *	nvram: wps_sta_pin:
 *
 */
int
start_wps_method(void)
{
	if (getpid() != 1) {
		notify_rc("start_wps_method");
		return 0;
	}

	nvram_set("wps_proc_status", "0");

#if defined(RTN11P_B1)
	system("reg s 0xB0000000; reg w 0x64 0x30035555"); // Set WLED GPIO mode.
#endif

#if defined(RTCONFIG_CONCURRENTREPEATER) || defined(RTCONFIG_AMAS)
	if (0
#if defined(RTCONFIG_CONCURRENTREPEATER)
	 || (sw_mode() == SW_MODE_REPEATER)
#endif
#if defined(RTCONFIG_AMAS)
	 || nvram_get_int("wps_enrollee") == 1
#endif
	) {
#if defined(RTCONFIG_NL80211)
		start_wsc_enrollee();
#else
		if (check_if_file_exist(REWPSC_PID_FILE)) {
			return 0;
		}
		stop_wps_method();
		start_re_wpsc();
#endif /* RTCONFIG_NL80211 */
	}
	else
#endif /* RTCONFIG_CONCURRENTREPEATER || RTCONFIG_AMAS */
		start_wsc();

	return 0;
}

int
stop_wps_method(void)
{
	if (getpid() != 1) {
		notify_rc("stop_wps_method");
		return 0;
	}

#if defined(RTCONFIG_NL80211)
#ifdef RTCONFIG_WPS_ENROLLEE
	if (nvram_match("wps_enrollee", "1"))
		stop_wsc_enrollee();
	else
#endif
		stop_wsc();
#else /* RTCONFIG_NL80211 */
	char prefix[] = "wlXXXXXXXXXX_", word[256] __attribute__((unused)), *next, ifnames[128];
	char wif[16];
	int i, wps_band = nvram_get_int("wps_band_x"), multiband __attribute__((unused)) = get_wps_multiband();
#if defined(RTCONFIG_CONCURRENTREPEATER) || defined(RTCONFIG_AMAS)
	int wlc_express = nvram_get_int("wlc_express");
#endif
#ifdef RTCONFIG_VIF_ONBOARDING
	int wps_via_vif = nvram_get_int("wps_via_vif");
#endif

#if defined(RTN11P_B1)
	system("reg s 0xB0000000; reg w 0x64 0x30035554"); // Set WLED hardware mode.
#endif

#ifdef RTCONFIG_AMAS
	if (nvram_get_int("wps_enrollee") == 1)
		wlc_express = 1; // Force use 2.4G only.
#endif

	i = 0;
	strcpy(ifnames, nvram_safe_get("wl_ifnames"));
	foreach (word, ifnames, next) {
		if (i >= MAX_NR_WL_IF)
			break;
		SKIP_ABSENT_BAND_AND_INC_UNIT(i);
#if defined(RTCONFIG_CONCURRENTREPEATER) || defined(RTCONFIG_AMAS)

#if defined(RTCONFIG_AMAS)
		snprintf(prefix, sizeof(prefix), "wl%d_", i);
		if (wps_band == i || multiband) {
#ifdef RTCONFIG_VIF_ONBOARDING
			if (wps_via_vif && wps_band == i) {
				snprintf(prefix, sizeof(prefix), "wl%d.%d_ifname", wps_band,
					(!nvram_get_int("re_mode")) ? nvram_get_int("obvif_cap_subunit"): nvram_get_int("obvif_re_subunit"));
				strlcpy(wif, nvram_safe_get(prefix), sizeof(wif));
			}
			else
#endif
				strlcpy(wif, word, sizeof(wif));
		}
		doSystem("iwpriv %s set WscStop=1", wif);	// Stop WPS Process.

		if (nvram_get_int("wps_enrollee") == 1)
#endif /* RTCONFIG_AMAS */
		{
			char tmp[64];
			char *aif = NULL;

			aif = nvram_safe_get(strcat_r(prefix, "vifs", tmp));
			/* Make sure WPS on all band are turned off */
			if ((wlc_express == 0 || (wlc_express == 1  && i == 0) || (wlc_express == 2  && i == 1)) && (strcmp(aif, "") != 0)) {
				doSystem("iwpriv %s set WscStop=1", aif);	// WPS disabled
				doSystem("ifconfig %s up", aif);
			}
		}
#else	/* !(RTCONFIG_CONCURRENTREPEATER || RTCONFIG_AMAS) */
		if (!multiband && wps_band != i) {
			++i;
			continue;
		}

		doSystem("iwpriv %s set WscStop=1", get_wifname(i));	// Stop WPS Process.
#endif	/* RTCONFIG_CONCURRENTREPEATER || RTCONFIG_AMAS */

		++i;
	}

#if defined(RTCONFIG_CONCURRENTREPEATER) || defined(RTCONFIG_AMAS)
	if (nvram_match("wps_ign_btn", "1")) {
		nvram_unset("wps_ign_btn");
		kill_pidfile_s("/var/run/watchdog.pid", SIGUSR2);
	}
#else	/* !(RTCONFIG_CONCURRENTREPEATER || RTCONFIG_AMAS) */
#if defined(RTCONFIG_MT798X) || defined(RTCONFIG_RALINK_RT3883) || defined(RTCONFIG_RALINK_RT3052)
	/* don't delay 10s */
#else
	sleep(10);
#endif
	doSystem("iwpriv %s set WscConfMode=%d", get_wifname(0), 7);	// trigger Windows OS to give a popup about WPS PBC AP
#if defined(RTCONFIG_HAS_5G)
	doSystem("iwpriv %s set WscConfMode=%d", get_wifname(1), 7);	// trigger Windows OS to give a popup about WPS PBC AP
#endif
#endif	/* RTCONFIG_CONCURRENTREPEATER || RTCONFIG_AMAS */
#endif /* RTCONFIG_NL80211 */

	return 0;
}

extern int g_isEnrollee[MAX_NR_WL_IF];
int count[MAX_NR_WL_IF] = { 0, };

int is_wps_stopped(void)
{
	int ret = 1;
	int i = 0;
#if defined(RTCONFIG_NL80211)
	char status[64];
	int wps_proc_status = 0;
#else	
	int status = 0;
#endif
	char tmp[128], prefix[] = "wlXXXXXXXXXX_", word[256], *next, ifnames[128];
	int wps_band = nvram_get_int("wps_band_x"), multiband = get_wps_multiband();
#if defined(RTCONFIG_NL80211)
	char tmpbuf[512];

#elif defined(RTCONFIG_AMAS)
	/* Trigger by obd daemon */
	if (nvram_get_int("wps_enrollee") == 1) {
		if (nvram_get_int("wps_cli_state") == 1)
			ret = 0;
		return ret;
	}
#endif

	i = 0;
	strcpy(ifnames, nvram_safe_get("wl_ifnames"));
	foreach (word, ifnames, next) {
		if (i >= MAX_NR_WL_IF)
			break;
		if (!multiband && wps_band != i) {
			++i;
			continue;
		}
		SKIP_ABSENT_BAND_AND_INC_UNIT(i);
		snprintf(prefix, sizeof(prefix), "wl%d_", i);
		if (!__need_to_start_wps_band(prefix) || nvram_match(strcat_r(prefix, "radio", tmp), "0")) {
			ret = 0;
			++i;
			continue;
		}

#if defined(RTCONFIG_NL80211)
#ifdef RTCONFIG_WPS_ENROLLEE
		if (nvram_match("wps_enrollee", "1"))
			strlcpy(status, getWscStatus_enrollee(i, tmpbuf, sizeof(tmpbuf)), sizeof(status));
		else
#endif
			strlcpy(status, getWscStatus_unit(i, tmpbuf, sizeof(tmpbuf)), sizeof(status));
		//dbG("band %d wps status: %s\n", i, status);
		if (!strcmp(status, "Success") 
#ifdef RTCONFIG_WPS_ENROLLEE
				|| !strcmp(status, "COMPLETED")
#endif
		) {
			dbG("\nWPS %s\n", status);
			wps_proc_status = 34;	//configured
#ifdef RTCONFIG_WPS_LED
			nvram_set("wps_success", "1");
#endif
#if (defined(RTCONFIG_WPS_ENROLLEE))
			if (nvram_match("wps_enrollee", "1")) {
				nvram_set("wps_e_success", "1");
#if defined(RTCONFIG_AMAS)
					amas_save_wifi_para();
#endif	/* RTCONFIG_AMAS */
			}
#endif
			ret = 1;
		}
		else if (!strcmp(status, "Failed") 
				|| !strcmp(status, "Timed-out")
				|| !strcmp(status, "Overlap")
#ifdef RTCONFIG_WPS_ENROLLEE
				|| !strcmp(status, "INACTIVE")
#endif
		) {
			dbG("\nWPS %s\n", status);
			if (strcmp(status, "Overlap") == 0)
				wps_proc_status = 0x109;	//PBC_SESSION_OVERLAP
			else if (strcmp(status, "Timed-out") == 0)
				wps_proc_status = 1;		//Timeout
			ret = 1;
		}
		else if (!strcmp(status, "Idle")
			) {
			ret = 1;
		}
		else
			ret = 0;
#else
		status = getWscStatus(i);
		if (status != 1)
			count[i] = 0;
		else
			count[i]++;

//		dbG("band %d wps status: %d, count: %d\n", i, status, count[i]);
		switch (status) {
			case 1:		/* Idle */
				if (strstr(nvram_safe_get("rc_service"), "start_wps_method") != NULL)
					count[i] = 0;
				if (count[i] < 15) ret = 0; // 15 would delay 750ms to avoid error since WPS not start yet.
				break;
			case 34:	/* Configured */
				dbG("\nWPS Configured\n");
				ret = 1;
				break;
			case 0x109:	/* PBC_SESSION_OVERLAP */
				dbG("\nWPS PBC SESSION OVERLAP\n");
				ret = 1;
				break;
			case 2:		/* Failed */
				if (!g_isEnrollee[i]) {
					dbG("\nWPS Failed\n");
					break;
				}
			default:
				ret = 0;
				break;
		}
#endif /* RTCONFIG_NL80211 */

		if (ret)
			break;

		++i;
	}

#if defined(RTCONFIG_NL80211)
	nvram_set_int("wps_proc_status", wps_proc_status);
#else
	nvram_set_int("wps_proc_status", status);
#endif

	return ret;
	// TODO: handle enrollee
}

int is_wps_success(void)
{
#ifdef RTCONFIG_AMAS
    if (nvram_get_int("wps_enrollee") == 1) { // Trigger by obd daemon
        if (nvram_get_int("wps_e_success") == 1)
            return 1;
    }
#endif
	return 0;
}

void runtime_onoff_wps(int onoff)
{
	int unit = 0;
	char wif[8], *next, prefix[] = "wlXXXXXXXXXX_";
	int wps_band = nvram_get_int("wps_band_x");
#ifdef RTCONFIG_VIF_ONBOARDING
	char prefix_vif[] = "wlXXXXXXXXXX_";
	if (nvram_get_int("wps_via_vif")) {
		snprintf(prefix_vif, sizeof(prefix_vif), "wl%d.%d_ifname", wps_band,
			(!nvram_get_int("re_mode")) ? nvram_get_int("obvif_cap_subunit"): nvram_get_int("obvif_re_subunit"));
	}
#endif

	foreach(wif, nvram_safe_get("wl_ifnames"), next) {
		if (wps_band == unit)
		{
			snprintf(prefix, sizeof(prefix), "wl%d_", unit);
			if (!nvram_pf_match(prefix, "auth_mode_x", "sae")
			&& !nvram_pf_match(prefix, "auth_mode_x", "wpa3")) {
				doSystem(IWPRIV" %s set WscConfMode=%d", wif, onoff ? 7 : 0);
			}
#ifdef RTCONFIG_VIF_ONBOARDING
			else
				doSystem(IWPRIV" %s set WscConfMode=%d", nvram_safe_get(prefix_vif), onoff ? 7 : 0);
#endif
		}
		unit++;
	}
}

int
start_wps_method_ob(void)
{
	if (nvram_get_int("wps_enable") == 0)
		runtime_onoff_wps(1);
	start_wps_method();
	return 0;
}

int
stop_wps_method_ob(void)
{
	stop_wps_method();
	if (nvram_get_int("wps_enable") == 0)
		runtime_onoff_wps(0);
	return 0;
}
