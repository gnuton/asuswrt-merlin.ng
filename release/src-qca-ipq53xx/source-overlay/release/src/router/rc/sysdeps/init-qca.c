/*
	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

*/

#include "rc.h"

#include <termios.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <time.h>
#include <errno.h>
#include <paths.h>
#include <sys/wait.h>
#include <sys/reboot.h>
#include <sys/klog.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <netinet/if_ether.h>		//have to in front of <linux/ethtool.h> and <linux/mii.h> to avoid redefinition of 'struct ethhdr'
#include <linux/mii.h>
#include <wlutils.h>
#include <bcmdevs.h>
#include <string.h>
#include <stdint.h>
#include <sys/sysmacros.h>

#include <shared.h>

#define DBGOUT		NULL			/* "/dev/console" */

extern const char *get_real_productid(void);

static inline void prn_args(char **eval_argvs)
{
	char **p;

	if (!DBGOUT)
		return;

	dbg("%s: Execute [", __func__);
	for (p = &eval_argvs[0]; p && *p; ++p) {
		dbg("%s%s", *p, " ");
	}
	dbg("]\n");
}

#ifdef RTCONFIG_QCA
#include <qca.h>
#include <flash_mtd.h>
#if defined(RTCONFIG_QCA) && defined(RTCONFIG_SOC_IPQ40XX)
extern int bg;
#endif
#endif

#if defined(RTCONFIG_NEW_REGULATION_DOMAIN)
#error !!!!!!!!!!!QCA driver must use country code!!!!!!!!!!!
#endif

#if defined(RTCONFIG_GLOBAL_INI)
static const char *global_ini_params[] = {
	"vow_config", "carrier_vow_config", "fw_vow_stats_enable",
	"OL_ACBKMinfree", "OL_ACBEMinfree", "OL_ACVIMinfree", "OL_ACVOMinfree",
	"fw_dump_options", "enableuartprint", "max_descs", "max_peers",
	"cce_disable", "qwrap_enable", "otp_mod_param", "max_active_peers",
	"enable_smart_antenna", "sa_validate_sw", "nss_wifi_nxthop_cfg",
	"max_clients", "max_vaps", "enable_smart_antenna_da", "lteu_support",
	"tgt_sched_params", "eapol_minrate_set", "eapol_minrate_ac_set",
	"mesh_support", "beacon_offload_disable", "spectral_disable", "twt_enable",
	NULL
};
#endif

#if !defined(RTCONFIG_QCA_WLAN_SCRIPTS)
#if defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_SOC_IPQ40XX) \
 || defined(RTCONFIG_SOC_IPQ60XX) || defined(RTCONFIG_SOC_IPQ53XX) \
 || defined(RTCONFIG_QCN550X) \
 || defined(RPAC51) || defined(MAPAC1750)
/* SPF4.0 (kernel v3.14) or above, including SPF5.2 (kernel v4.4) */
static const char *umac_params[] = {
	/*"atf_mode",*/ "atf_msdu_desc", "atf_peers", "atf_max_vdevs",
	"enable_mesh_peer_cap_update", "enable_pktlog_support",
	"wifiposenable",

	NULL
},
*qca_ol_params[] = {
#if defined(RTCONFIG_GLOBAL_INI) && !defined(RTCONFIG_QSDK10CS) /*DK SPF10*/
	"allocram_track_max", "ar900b_20_targ_clk",
	"bmi", "cfg_iphdr_pad", "dfs_disable", "emu_type",
	"enable_mesh_support", "enable_tx_tcp_cksum", "frac",
	"intval", "low_mem_system", "max_vdevs",
	"nss_wifi_ol_skip_nw_process", "ol_scan_chanlist",
	"war1", "war1_allow_sleep",
#if defined(RTCONFIG_WIFI_QCA9990_QCA9990) || \
    defined(RTCONFIG_WIFI_QCA9994_QCA9994) || \
    defined(RTCONFIG_WIFI_QCN5024_QCN5054)
	"qca9888_20_targ_clk", "preCACEn",
#elif defined(RTCONFIG_QCA_BECHIP)
	"qca9888_20_targ_clk",
#endif
#if defined(RTCONFIG_WIFI_QCN5024_QCN5054)
	/* SPF10 ES QSDK */
	"enable_eapol_minrate", "set_eapol_minrate_ac", "interCACChan",
#endif
#else	/* !RTCONFIG_GLOBAL_INI */
	"OL_ACBEMinfree", "OL_ACBKMinfree", "OL_ACVIMinfree",
	"OL_ACVOMinfree", "allocram_track_max", "ar900b_20_targ_clk",
	"bmi", "cfg_iphdr_pad", "dfs_disable", "emu_type",
	"enable_mesh_support", "enable_smart_antenna",
	"enable_tx_tcp_cksum", "enableuartprint", "frac",
	"fw_dump_options", "intval", "low_mem_system",
	"max_active_peers", "max_clients", "max_descs", "max_peers",
	"max_vaps", "max_vdevs", "nss_wifi_ol_skip_nw_process",
	/*"nss_wifi_olcfg",*/ "ol_scan_chanlist", "otp_mod_param",
	"qwrap_enable", "sa_validate_sw", /*"testmode",*/
	"vow_config", "war1", "war1_allow_sleep",
#endif	/* RTCONFIG_GLOBAL_INI */

	NULL
},
*qdf_params[] = {
	"qdf_dbg_mask", "prealloc_disabled",

	NULL
};
#else
/* SPF3.0 (kernel v3.14)
 * QCA95xx ILQ1.x~2.x (kernel v3.3)
 * IPQ806X ILQ3.0~3.1 (kernel v3.4)
 */
static const char *umac_params[] = {
	"vow_config", "OL_ACBKMinfree", "OL_ACBEMinfree", "OL_ACVIMinfree",
	"OL_ACVOMinfree", "ar900b_emu", "frac", "intval",
	"fw_dump_options", "enableuartprint", "ar900b_20_targ_clk",
	"max_descs", "qwrap_enable", "otp_mod_param", "max_active_peers",
	"enable_smart_antenna", "max_vaps", "enable_smart_antenna_da",
#if defined(RTCONFIG_WIFI_QCA9990_QCA9990) || \
    defined(RTCONFIG_WIFI_QCA9994_QCA9994) || \
    defined(RTCONFIG_WIFI_QCN5024_QCN5054)
	"qca9888_20_targ_clk", "lteu_support",
	"atf_msdu_desc", "atf_peers", "atf_max_vdevs",
#endif
#if defined(RTCONFIG_SOC_IPQ40XX)
	"atf_msdu_desc", "atf_peers", "atf_max_vdevs",
#endif

	NULL
}, *qdf_params[] __attribute__((unused)) = {
	NULL
}, *adf_params[]  __attribute__((unused)) = {
	"prealloc_disabled",
	NULL
};
#endif

enum qca_kmod_flags {
	QCA_WIFI_STICK_BIT = 0,
	QCA_WIFI_DA_BIT,
	QCA_WIFI_OL_BIT,
	QCA_WIFI_ADF_BIT,
	QCA_WIFI_QDF_BIT,
};

#define QWIFI_STICK	(1U << QCA_WIFI_STICK_BIT)
#define QWIFI_DA	(1U << QCA_WIFI_DA_BIT)
#define QWIFI_OL	(1U << QCA_WIFI_OL_BIT)
#define QWIFI_DAOL_TYPE	(QWIFI_DA | QWIFI_OL)
#define QWIFI_ADF	(1U << QCA_WIFI_ADF_BIT)
#define QWIFI_QDF	(1U << QCA_WIFI_QDF_BIT)
#define QWIFI_DF_TYPE	(QWIFI_ADF | QWIFI_QDF)

#define DPARM(v, s, len, fmt, args...)	{	\
	int l = snprintf(s, len, fmt, ##args);	\
	*v++ = s;				\
	len -= l;				\
	s += l + 1;				\
}

#define PTR_DPARM(pv, ps, plen, fmt, args...) {		\
	int l = snprintf(*ps, *plen, fmt, ##args);	\
	*(*pv)++ = *ps;					\
	*plen -= l;					\
	*ps += l + 1;					\
}

#if defined(RTCONFIG_WIFI_QCN5024_QCN5054) || defined(RTCONFIG_QCA_BECHIP)
static int do_cold_boot_calibration(char *mod, int is_ftm);
#endif

#if defined(RTCONFIG_SOC_IPQ53XX) || defined(RTCONFIG_SOC_IPQ60XX) || defined(RTCONFIG_SOC_IPQ8074)
/* No need to implement ath_hal_param_hook(). */
#else
static void ath_hal_param_hook(char ***pv, char **ps, int *plen);
#endif
static void umac_param_hook(char ***pv, char **ps, int *plen);
static void umac_tmode_param_hook(char ***pv, char **ps, int *plen);
#if (SPF_VER >= SPF_VER_ID(11,1))
static void qdf_param_hook(char ***pv, char **ps, int *plen);
static void qdf_tmode_param_hook(char ***pv, char **ps, int *plen);
#endif

/* RTCONFIG_WIFI_IPQ53XX_QCN6274: AP-MI01.2, IPQ53xx + QCN6274 | QCN64XX, SPF12.2 CS, /lib/wifi/qca-wifi-modules
 * mem_manager
 * qdf
 * umac
 * telemetry_agent
 * qca_spectral
 * qca_ol
 * smart_antenna
 * rawmode_sim
 * wifi_3_0
 * monitor
 * ath_pktlog
 */
#if defined(RTCONFIG_SOC_IPQ40XX) \
 || defined(RTCONFIG_WIFI_QCN5024_QCN5054) \
 || defined(RTCONFIG_QCA_BECHIP) \
 || defined(RTCONFIG_QCN550X) \
 || defined(RPAC51) || defined(MAPAC1750)
static void qca_ol_param_hook(char ***pv, char **ps, int *plen);
static void qca_ol_tmode_param_hook(char ***pv, char **ps, int *plen);
#endif
#if defined(RTCONFIG_SOC_IPQ53XX) \
 && defined(RTCONFIG_WIFI_IPQ53XX_QCN6274)
static void ecm_param_hook(char ***pv, char **ps, int *plen);
static inline void ecm_tmode_param_hook(char ***pv, char **ps, int *plen) { return ecm_param_hook(pv, ps, plen); }
#endif
#if defined(RTCONFIG_GLOBAL_INI)
static void wifi_3_0_post_hook(void);
#endif
static void ecm_wifi_plugin_post_hook(void);
static struct load_wifi_kmod_seq_s {
	char *kmod_name;
	/* 0:		Always load this module.
	 * bit0:	stick, never removed.
	 * bit1~4:	Direct-Attach/Offload/adf/qdf module.
	 * 		Load module if same bit is enabled in qca_wifi_type.
	 */
	unsigned int flags;
	unsigned int load_sleep;
	unsigned int remove_sleep;
	/* module-specific parameters and can be adjusted by define qca_XXX nvram variable. */
	const char **params;
	/* module-specific mission mode parameter generator function. */
	void (*mission_mode_param_hook_fn)(char ***pv, char **ps, int *plen);
	/* module-specific test mode parameter generator function. */
	void (*test_mode_param_hook_fn)(char ***pv, char **ps, int *plen);
	/* module-specific post hook function. */
	void (*post_fn)(void);
} load_wifi_kmod_seq[] = {
	/* Reference to /lib/wifi/qca-wifi-modules in QSDK. */
#if defined(RTCONFIG_SOC_IPQ40XX) \
 || defined(RTCONFIG_WIFI_QCN5024_QCN5054) \
 || defined(RTCONFIG_QCN550X) || defined(RPAC51) || defined(MAPAC1750)
	/* SPF3.0, kernel 3.14.x, 10.4 WiFi driver
	 * SPF5.2, kernel 4.4.x, 10.4 WiFi driver
	 * SPF8.0, kernel 4.4.x, 11.0 WiFi driver
	 */
	{ .kmod_name = "mem_manager", .flags = QWIFI_STICK },	/* If QCA WiFi configuration file has WIFI_MEM_MANAGER_SUPPORT=1 */
#if (defined(RTCONFIG_WIFI_QCN5024_QCN5054) || defined(RTCONFIG_SOC_IPQ40XX)) \
 && defined(RTCONFIG_GLOBAL_INI)
	/* IPQ8074A SPF10 */
	{ .kmod_name = "qdf", .flags = QWIFI_QDF,
		.params = qdf_params,
#if (SPF_VER >= SPF_VER_ID(11,1))
		.mission_mode_param_hook_fn = qdf_param_hook,
		.test_mode_param_hook_fn = qdf_tmode_param_hook
#endif
	},
	{ .kmod_name = "asf" },
#else	/* !((RTCONFIG_WIFI_QCN5024_QCN5054 || RTCONFIG_QCA_BECHIP || RTCONFIG_SOC_IPQ40XX) && RTCONFIG_GLOBAL_INI) */
	/* IPQ8074 SPF8 */
	{ .kmod_name = "asf" },
	{ .kmod_name = "adf", .flags = QWIFI_ADF },
	{ .kmod_name = "qdf", .flags = QWIFI_QDF,
		.params = qdf_params
	},
#endif	/* (RTCONFIG_WIFI_QCN5024_QCN5054 || RTCONFIG_QCA_BECHIP || RTCONFIG_SOC_IPQ40XX) && RTCONFIG_GLOBAL_INI */
#if !defined(RTCONFIG_WIFI_QCN5024_QCN5054) && !defined(RTCONFIG_QSDK10CS) /*DK SPF10*/
	{ .kmod_name = "ath_dfs" },
	{ .kmod_name = "ath_spectral" },
#endif
	{ .kmod_name = "umac",
		.remove_sleep = 2, .params = umac_params,
		.mission_mode_param_hook_fn = umac_param_hook,
		.test_mode_param_hook_fn = umac_tmode_param_hook
	},
#if defined(RTCONFIG_WIFI_QCN5024_QCN5054) || defined(RTCONFIG_QSDK10CS) /*DK SPF10*/
	{ .kmod_name = "qca_spectral" },
#elif !defined(RTCONFIG_SOC_IPQ60XX)
	{ .kmod_name = "ath_hal", .flags = QWIFI_DA,
		.mission_mode_param_hook_fn = ath_hal_param_hook
	},
	{ .kmod_name = "ath_rate_atheros", .flags = QWIFI_DA },
	{ .kmod_name = "hst_tx99", .flags = QWIFI_DA  },
	{ .kmod_name = "ath_dev", .flags = QWIFI_DA },
	{ .kmod_name = "qca_da", .flags = QWIFI_DA },
#endif
	{ .kmod_name = "qca_ol", .flags = QWIFI_OL,
		.params = qca_ol_params,
		.mission_mode_param_hook_fn = qca_ol_param_hook,
		.test_mode_param_hook_fn = qca_ol_tmode_param_hook
	},
#if defined(RTCONFIG_GLOBAL_INI)
	{ .kmod_name = "wifi_3_0",
		.post_fn = wifi_3_0_post_hook
	},
#if (SPF_VER >= SPF_VER_ID(11,5))
	{ .kmod_name = "monitor" },
#endif
#endif	/* RTCONFIG_GLOBAL_INI */
#elif defined(RTCONFIG_QCA_BECHIP)
	/* IPQ53XX SPF12.2, kernel 5.4.x, 12.2 WiFi ddriver */
	{ .kmod_name = "mem_manager",	/* If QCA WiFi configuration file has WIFI_MEM_MANAGER_SUPPORT=1 */
		.flags = QWIFI_STICK
	},
	{ .kmod_name = "qdf",
		.flags = QWIFI_QDF,
		.params = qdf_params,
		.mission_mode_param_hook_fn = qdf_param_hook,
		.test_mode_param_hook_fn = qdf_tmode_param_hook
	},
	{ .kmod_name = "umac",
		.remove_sleep = 2,
		.params = umac_params,
		.mission_mode_param_hook_fn = umac_param_hook,
		.test_mode_param_hook_fn = umac_tmode_param_hook
	},
	{ .kmod_name = "telemetry_agent" },
	{ .kmod_name = "qca_spectral" },
	{ .kmod_name = "qca_ol",
		.flags = QWIFI_OL,
		.params = qca_ol_params,
		.mission_mode_param_hook_fn = qca_ol_param_hook,
		.test_mode_param_hook_fn = qca_ol_tmode_param_hook
	},
	{ .kmod_name = "smart_antenna" },
	// { .kmod_name = "rawmode_sim" },	// load it if load_rawsimulation_mod != 0
	{ .kmod_name = "wifi_3_0",
		.post_fn = wifi_3_0_post_hook
	},
	{ .kmod_name = "monitor" },
#if defined(RTCONFIG_SOC_IPQ53XX) \
 && defined(RTCONFIG_WIFI_IPQ53XX_QCN6274)
	{ .kmod_name = "ecm",
		.mission_mode_param_hook_fn = ecm_param_hook,
		.test_mode_param_hook_fn = ecm_tmode_param_hook
	},
#endif
	{ .kmod_name = "ecm_wifi_plugin",
#if (SPF_VER >= SPF_VER_ID(12,5)) && defined(RTCONFIG_SOC_IPQ53XX)
		.post_fn = ecm_wifi_plugin_post_hook
#endif
	},
#else	/* !(RTCONFIG_SOC_IPQ40XX || RTCONFIG_WIFI_QCN5024_QCN5054 || RTCONFIG_QCA_BECHIP || RTCONFIG_QCN550X || RPAC51 || MAPAC1750 */
	/* QCA9558 ILQ2.0, kernel 3.3.x, 10.2 WiFi driver
	 * IPQ8064 ILQ3.1, kernel 3.4.x, 10.4 WiFi driver
	 */
	{ .kmod_name = "asf" },
	{ .kmod_name = "adf",
		.params = adf_params
	},
	{ .kmod_name = "ath_hal",
		.mission_mode_param_hook_fn = ath_hal_param_hook
	},
	{ .kmod_name = "ath_rate_atheros" },
	{ .kmod_name = "ath_dfs" },
	{ .kmod_name = "ath_spectral" },
	{ .kmod_name = "hst_tx99" },
	{ .kmod_name = "ath_dev" },
	{ .kmod_name = "umac",
		.remove_sleep = 2,
		.params = umac_params,
		.mission_mode_param_hook_fn = umac_param_hook,
		.test_mode_param_hook_fn = umac_tmode_param_hook
	},
#endif	/* RTCONFIG_SOC_IPQ40XX || RTCONFIG_WIFI_QCN5024_QCN5054 ||  RTCONFIG_QCN550X || RPAC51 || MAPAC1750 */

//	{ "ath_pktlog", 0, 0, 0, NULL },
#if defined(RTCONFIG_WIFI_QCN5024_QCN5054) || defined(RTCONFIG_QSDK10CS) /*DK SPF10*/
	{ .kmod_name = "smart_antenna" },
#endif
#if defined(RTCONFIG_WIGIG)
	{ .kmod_name = "wil6210", .flags = QWIFI_STICK },
#endif
};

/* Define QCA Wi-Fi modules type here.
 * If a model uses old Wi-Fi driver, e.g., 10.2, specify 0 here.
 * If a model uses new Wi-Fi driver, e.g., 10.4 and kernel v3.14 or above, specify all type of Wi-Fi of all band here.
 */
static unsigned int qca_wifi_type =
#if defined(RTCONFIG_WIFI_QCA9990_QCA9990) \
 || defined(RTCONFIG_WIFI_QCA9994_QCA9994)
	QWIFI_OL | QWIFI_ADF
#elif defined(RTCONFIG_SOC_IPQ40XX) \
   || defined(RTCONFIG_WIFI_QCN5024_QCN5054) \
   || defined(RTCONFIG_QCA_BECHIP)
	QWIFI_OL | QWIFI_QDF	/* All Wi-Fi unit are offload. */
#elif defined(RPAC51)
	QWIFI_DA | QWIFI_ADF	/* qca_ol is not loaded on RP-AC51.  It should use Direct-Attach Wi-Fi modules only. */
#elif defined(MAPAC1750) \
   || (defined(RTCONFIG_QCN550X) && defined(RTCONFIG_HAS_5G))
	QWIFI_DA | QWIFI_OL | QWIFI_QDF
#elif defined(RTCONFIG_QCN550X)
	QWIFI_DA | QWIFI_QDF
#else
	0		/* skip DA/OL module checking and load all QCA Wi-Fi modules */
#endif
;
#endif // end of RTCONFIG_QCA_WLAN_SCRIPTS

static struct load_nat_accel_kmod_seq_s {
	char *kmod_name;
	unsigned int load_sleep;
	unsigned int remove_sleep;
} load_nat_accel_kmod_seq[] = {
#if defined(RTCONFIG_SOC_IPQ8064) || defined(RTCONFIG_SOC_IPQ8074) \
 || defined(RTCONFIG_SOC_IPQ53XX)
#if defined(RTCONFIG_WIFI_QCA9994_QCA9994) || \
    defined(RTCONFIG_WIFI_QCN5024_QCN5054)
	{ "shortcut_fe_drv", 0, 0 },
#endif
	{ "ecm", 0, 0 },
#elif defined(RTCONFIG_SOC_IPQ60XX) || defined(RTCONFIG_SOC_IPQ50XX)
	{ "ecm", 0, 0 },
#else
        { "shortcut_fe_cm", 0, 0 },
#endif
};

static const struct irq_smp_affinity_s {
	int irq;
	unsigned int cpu_mask;
} wifi_irq_smp_affinity_tbl[] = {
#if defined(BRTAC828) || defined(RTAD7200)
#if LINUX_KERNEL_VERSION >= KERNEL_VERSION(4,4,0)
	{ 143, 1 },	/* wifi0 = 2G ==> core 0 */
	{ 176, 2 },	/* wifi1 = 5G ==> core 1 */
#else
	{ 68, 1 },	/* wifi0 = 2G ==> core 0 */
	{ 90, 2 },	/* wifi1 = 5G ==> core 1 */
#endif
#if defined(RTAD7200)
#if LINUX_KERNEL_VERSION >= KERNEL_VERSION(4,4,0)
	{ 178, 2 },	/* wil6210 = 60G ==> core 0 */
#else
	{ 1433, 2 },	/* wil6210 = 60G ==> core 0 */
#endif
#endif
#elif defined(RTCONFIG_SOC_IPQ40XX)
	{ 174, 2 },	/* wifi2 = 5G2 ==> core 2 */
	{ 200, 4 },	/* wifi0 = 2G  ==> core 3 */
	{ 201, 8 },	/* wifi1 = 5G  ==> core 4 */
#if defined(RTAC58U)
	{ 164, 2 },	/* USB2.0 ==> core 2 */
#endif
#endif
	{ -1, 0 }	/* Last item. */
};

#if defined(RTCONFIG_GLOBAL_INI)
/* Translate multiple continuously NULL-terminated strings as newline-terminated strings at run-time,
 * and then write it to file.
 * @fp:
 * @s:	multiple continuously NULL-terminated strings, last string must end with two NULL character.
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
static int write_cont_null_strings(FILE *fp, const char *s)
{
	int ret = 0;
	const char *b = s;

	if (!fp || !s)
		return -2;

	for (b = s; *b != '\0'; ) {
		fprintf(fp, "%s\n", b);
		b += strlen(b);
		if (*b == '\0' && *(b + 1) == '\0')
			break;
		b++;
	}

	return ret;
}

/* Update multiple parameters pointed by @params array to @filename .ini file
 * @params:	char* array, last item must be NULL, newline character will be ignored.
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
int __update_ini_file(const char *filename, char **params)
{
	const unsigned char end_mark[2] = { 0, 0 };
	long flen = 0, params_tlen = 0, alen;
	int ret = 0, i, found, section;
	FILE *fp;
	size_t l, key_len;
	char line[MAX_INI_PARM_LINE_LEN], *p, **v, *data;
	struct buf_ctrl_s {
		char *buf, *b;
		size_t remain, tlen;
	} bc[2], *src, *dst;

	if (!filename || !params)
		return -1;

	/* Calculate buffer size and allocate two buffers. */
	for (v = params; *v != NULL; ++v)
		params_tlen += strlen(*v) + 1;
	if (!params_tlen)
		return 0;

	fp = fopen(filename, "r+");
	if (!fp)
		return -2;
	fseek(fp, 0, SEEK_END);
	flen = ftell(fp);

	alen = flen + params_tlen + 1;
	for (i = 0, src = &bc[0]; i < ARRAY_SIZE(bc); ++i, ++src) {
		src->buf = malloc(alen);
		if (!src->buf) {
			ret = -3;
			break;
		}

		src->remain = src->tlen = alen;
		*src->buf = '\0';
	}

	if (ret)
		goto exit_update_ini_file;

	/* Read @filename to 1-st buffer, comments and blank lines are removed. */
	section = 0;
	src = &bc[0];
	src->b = src->buf;
	fseek(fp, 0, SEEK_SET);
	while (fgets(line, sizeof(line), fp)) {
		if (*line == '\0' || *line == '\r' || *line == '\n' || *line == '=')
			continue;
		/* replace '\n' with '\0' temporary. */
		if ((p = strchr(line, '\n')) != NULL)
			*p = '\0';

		if (!section && *line == '[' && strchr(line + 1, ']'))
			section = 1;
		l = snprintf(src->b, src->remain, "%s", line) + 1;
		src->b += l;
		src->remain -= l;
	}
	*src->b++ = '\0';
	src->remain--;

	dst = &bc[1];
	for (i = 0, v = &params[0]; *v != NULL; ++i, ++v) {
		/* get next legal parameter */
		while (*v != NULL) {
			if ((p = strchr(*v, '=')) && p != *v)
				break;

			dbg("%s: unknown format [%s] in params.\n", __func__, *v);
			v++;
		}
		if (*v == NULL) {
			memcpy(dst->buf, src->buf, src->tlen - src->remain);
			dst->remain = src->remain;
			break;
		}

		if (!(p = strchr(*v, '=')))
			continue;
		key_len = p - *v + 1;
		found = 0;
		src = &bc[0 ^ (i & 1)];
		dst = &bc[1 ^ (i & 1)];
		src->b = src->buf;
		dst->b = dst->buf;
		dst->remain = dst->tlen;
		memcpy(dst->b, end_mark, 2);
		while (*(src->b) != '\0') {
			data = src->b;
			if (!strncmp(src->b, *v, key_len)) {
				found++;
				/* replace parameter */
				data = *v;
			}

			/* If a parameter hasn't been found before we meet first section line, e.g., [256M],
			 * append the parameter before section line.
			 */
			if (!found && *data == '[' && strchr(data + 1, ']')) {
				if ((p = strchr(*v, '\n')) != NULL) {
					l = p - *v;
					memmove(dst->b, *v, l);
					*(dst->b + l) = '\0';
					dst->b += l + 1;
					dst->remain -= l + 1;
				} else {
					l = snprintf(dst->b, dst->remain, "%s", *v) + 1;
					dst->b += l;
					dst->remain -= l;
				}
				found++;
			}

			l = snprintf(dst->b, dst->remain, "%s", data) + 1;
			dst->b += l;
			dst->remain -= l;
			src->b += strlen(src->b) + 1;
		}
		if (!found){
			/* append new parameter */
			if ((p = strchr(*v, '\n')) != NULL) {
				l = p - *v;
				memmove(dst->b, *v, l);
				*(dst->b + l) = '\0';
				dst->b += l + 1;
				dst->remain -= l + 1;
			} else {
				l = snprintf(dst->b, dst->remain, "%s", *v) + 1;
				dst->b += l;
				dst->remain -= l;
			}
		}
		*dst->b++ = '\0';
		dst->remain--;
	}

	fseek(fp, 0, SEEK_SET);
	write_cont_null_strings(fp, dst->buf);
	fclose(fp);
	/* last '\0' is not wrote, same offset in file maybe non-zero value. */
	truncate(filename, dst->tlen - dst->remain - 1);

 exit_update_ini_file:

	for (i = 0, src = &bc[0]; i < ARRAY_SIZE(bc); ++i, ++src)
		if (src->buf)
			free(src->buf);

	return ret;
}
#endif

#if !defined(RTCONFIG_QCA_WLAN_SCRIPTS)
#if defined(RTCONFIG_SOC_IPQ53XX) || defined(RTCONFIG_SOC_IPQ60XX) || defined(RTCONFIG_SOC_IPQ8074)
/* No need to implement ath_hal_param_hook(). */
#else
/* Generate parameters for ath_hal in mission mode. */
static void ath_hal_param_hook(char ***pv, char **ps, int *plen)
{
	int ce_level __attribute__((unused));
#if defined(RPAC51)
	const int default_ce_level = 0xa0;
#else
	const int default_ce_level __attribute__((unused)) = 0xce;
#endif

	if (!pv || !ps || !plen)
		return;

#if defined(RTCONFIG_WIFI_QCA9557_QCA9882) || defined(RTCONFIG_QCA953X) || (defined(RTCONFIG_QCA956X) && !defined(MAPAC1750))
	ce_level = nvram_get_int("ce_level");
	if (ce_level <= 0)
		ce_level = default_ce_level;
	PTR_DPARM(pv, ps, plen, "ce_level=%d", ce_level);
#endif
}
#endif	/* RTCONFIG_SOC_IPQ53XX || RTCONFIG_SOC_IPQ60XX */

/* Generate parameters for umac in mission mode. */
static void umac_param_hook(char ***pv, char **ps, int *plen)
{
	if (!pv || !ps || !plen)
		return;

	if (atf_enabled())
		*(*pv)++ = "atf_mode=1";

#if defined(RTCONFIG_WIFI_QCA9990_QCA9990) \
 || defined(RTCONFIG_WIFI_QCA9994_QCA9994)
#if LINUX_KERNEL_VERSION < KERNEL_VERSION(3,14,0)
	*(*pv)++ = "msienable=0";
#endif
#elif defined(RPAC51)
	*(*pv)++ = "msienable=0";
	*(*pv)++ = "wifi_start_idx=1";	/* FIXME: The internal direct attach radio should always be wifi0 */
	*(*pv)++ = "qca9888_20_targ_clk=300000000";
#endif

#if defined(RTCONFIG_WIFI_QCA9990_QCA9990) \
 || defined(RTCONFIG_WIFI_QCA9994_QCA9994)
#if LINUX_KERNEL_VERSION < KERNEL_VERSION(3,14,0)
	/* IPQ806X ILQ3.1 */
	PTR_DPARM(pv, ps, plen, "nss_wifi_olcfg=%d", nss_wifi_offloading());
#endif
#endif
}

/* Generate parameters for umac in test mode. */
static void umac_tmode_param_hook(char ***pv, char **ps, int *plen)
{
	if (!pv || !ps || !plen)
		return;

#if defined(RTCONFIG_WIFI_QCA9990_QCA9990) \
 || defined(RTCONFIG_WIFI_QCA9994_QCA9994) \
 || defined(RPAC51)
	/* ILQ3.1 (kernel 3.3), SPF3.0 (kernel 3.14), or below. */
	*(*pv)++ = "testmode=1";
	*(*pv)++ = "ahbskip=1";
#elif defined(RTCONFIG_SOC_IPQ40XX) \
   || defined(RTCONFIG_QCN550X) \
   || defined(MAPAC1750)
	*(*pv)++ = "ahbskip=1";
#endif
}

#if (defined(RTCONFIG_SOC_IPQ8074) && (SPF_VER >= SPF_VER_ID(11,1))) \
 || defined(RTCONFIG_SOC_IPQ53XX)
/* Generate parameters for qdf in mission mode. */
static void qdf_param_hook(char ***pv, char **ps, int *plen)
{
	if (!pv || !ps || !plen)
		return;

	// *(*pv)++ = "mem_debug_disabled=1";	// Only available if MEMORY_DEBUG is enabled in config_XXX.wlan.unified.profile
#if (SPF_VER >= SPF_VER_ID(11,3))
	/* is_cnss_diag_logging_enabled() = 0 */
	*(*pv)++ = "qdf_log_dump_at_kernel_enable=0";
#if (SPF_VER >= SPF_VER_ID(12,2))
	*(*pv)++ = "qdf_log_flush_timer_period=50";
#elif (SPF_VER >= SPF_VER_ID(11,4))
	*(*pv)++ = "qdf_log_flush_timer_period=0";
#endif
	update_ini_file(GLOBAL_INI, "logger_enable_mask=0");	/* 14 or another value if is_cnss_diag_logging_enabled() = 1 */
#endif
}

/* Generate parameters for qdf in test mode. */
static void qdf_tmode_param_hook(char ***pv, char **ps, int *plen)
{
	if (!pv || !ps || !plen)
		return;

	// *(*pv)++ = "mem_debug_disabled=1";	// Only available if MEMORY_DEBUG is enabled in config_XXX.wlan.unified.profile
#if (SPF_VER >= SPF_VER_ID(11,3))
	/* is_cnss_diag_logging_enabled() = 0 */
	*(*pv)++ = "qdf_log_dump_at_kernel_enable=0";
#if (SPF_VER >= SPF_VER_ID(12,2))
	*(*pv)++ = "qdf_log_flush_timer_period=50";
#elif (SPF_VER >= SPF_VER_ID(11,4))
	*(*pv)++ = "qdf_log_flush_timer_period=0";
#endif
	update_ini_file(GLOBAL_INI, "logger_enable_mask=0");	/* 14 or another value if is_cnss_diag_logging_enabled() = 1 */
#endif
}
#endif	/* (RTCONFIG_SOC_IPQ8074 && SPF11.1+) || RTCONFIG_SOC_IPQ53XX */

#if defined(RTCONFIG_SOC_IPQ40XX) \
 || defined(RTCONFIG_WIFI_QCN5024_QCN5054) \
 || defined(RTCONFIG_QCA_BECHIP) \
 || defined(RTCONFIG_QCN550X) \
 || defined(RPAC51) || defined(MAPAC1750)
/* Generate parameters for qca_ol in mission mode.  */
static void qca_ol_param_hook(char ***pv, char **ps, int *plen)
{
	int olcfg __attribute__((unused)) = nss_wifi_offloading();

	if (!pv || !ps || !plen)
		return;

#if !defined(RTCONFIG_GLOBAL_INI)
#if defined(RTCONFIG_WIFI_QCA9990_QCA9990) \
 || defined(RTCONFIG_WIFI_QCA9994_QCA9994)
#if LINUX_KERNEL_VERSION >= KERNEL_VERSION(3,14,0)
	/* IPQ806X SPF5.3 */
	PTR_DPARM(pv, ps, plen, "nss_wifi_olcfg=%d", olcfg());
#endif
#elif defined(RTCONFIG_WIFI_QCN5024_QCN5054)
	/* IPQ807X SPF8.0 */
	PTR_DPARM(pv, ps, plen, "nss_wifi_olcfg=%d", !!olcfg);
	PTR_DPARM(pv, ps, plen, "nss_wifili_olcfg=%d", olcfg);
	*(*pv)++ = "rx_hash=0";
	*(*pv)++ = "intr_mitigation_enabled=1";		/* SPF10 obsoleted it */
#endif
#endif	/* !RTCONFIG_GLOBAL_INI */
#if defined(RTCONFIG_CFG80211) && defined(RTCONFIG_SPF8_QSDK)
	*(*pv)++ = "cfg80211_config=1";
#endif


#if defined(RTCONFIG_SOC_IPQ40XX)
	/* For lower memory system, e.g., IPQ40xx + 128MB RAM models. */
	if (get_meminfo_item("MemTotal") <= 131072)
		*(*pv)++ = "low_mem_system=1";
#endif


	/* Keep below statsment at end of this function. */
#if defined(RTCONFIG_WIFI_QCN5024_QCN5054) || defined(RTCONFIG_QCA_BECHIP)
	do_cold_boot_calibration("qca_ol", 0);
#endif
}

/* Generate parameters for qca_ol in test mode.  */
static void qca_ol_tmode_param_hook(char ***pv, char **ps, int *plen)
{
	if (!pv || !ps || !plen)
		return;

#if defined(RTCONFIG_WIFI_QCN5024_QCN5054)
#if defined(RTCONFIG_SPF8_QSDK)
	*(*pv)++ = "soc_probe_module_load=1";
	*(*pv)++ = "fbc_enabled=1";
	*(*pv)++ = "hw_mode_id=1";
	*(*pv)++ = "testmode=1";
	*(*pv)++ = "cfg80211_config=1";
#elif (SPF_VER >= SPF_VER_ID(11,0))
	*(*pv)++ = "hw_mode_id=1";
	*(*pv)++ = "testmode=1";
	*(*pv)++ = "cfg80211_config=1";
#else
#error FIXME
#endif	/* RTCONFIG_SPF8_QSDK */
#elif defined(RTCONFIG_QCA_BECHIP)
	*(*pv)++ = "testmode=1";
	*(*pv)++ = "cfg80211_config=1";
#elif defined(RTCONFIG_SOC_IPQ40XX) \
   || defined(RTCONFIG_QCN550X) \
   || defined(MAPAC1750)
	*(*pv)++ = "testmode=1";
#if defined(RTCONFIG_QSDK10CS)
	*(*pv)++ = "hw_mode_id=1";
	*(*pv)++ = "cfg80211_config=1";
#endif  /*IPQ40XX QSDK10*/
#endif


	/* Keep below statsment at end of this function. */
#if defined(RTCONFIG_WIFI_QCN5024_QCN5054) || defined(RTCONFIG_QCA_BECHIP)
	do_cold_boot_calibration("qca_ol", 1);
#endif
}
#endif
#endif // end of RTCONFIG_QCA_WLAN_SCRIPTS

#if defined(RTCONFIG_SOC_IPQ53XX) \
 && defined(RTCONFIG_WIFI_IPQ53XX_QCN6274)
static void ecm_param_hook(char ***pv, char **ps, int *plen)
{
	int accel_type;

	if (!pv || !ps || !plen)
		return;

	accel_type = nat_accel_type("ecm");
	if (accel_type > 0) {
		PTR_DPARM(pv, ps, plen, "front_end_selection=%d", accel_type);
	}
}
#endif

#if defined(RTCONFIG_GLOBAL_INI)
/* Set correct hw_mode_id to @arg in accordance with /sys/class/net/socXXX/hw_modes.
 * @basedir:
 * @de:		struct dirent readed by readdir() in readdir_wrapper()
 * @arg:	integer pointer, must be initialized as negative integer.
 * @return:
 * 	0:	success or irrevelent file
 *     -1:	invalid parameter
 *  otherwise:	error
 */
static int __update_hw_mode_id(const char *basedir, const struct dirent *de, size_t de_size, void *arg)
{
#if SPF_VER >= SPF_VER_ID(12,0)
	int v = -1, prefer_hw_mode_id = -1;
	char tag[sizeof("hw_mode_id_soc1XXXXXX")] = "";
	char preferred_hw_mode_str[4] = "", hw_modes_str[32] = "";
	char path1[sizeof("/sys/class/net/socXXX/preferred_hw_modeXXXXXX")];
	char path2[sizeof("/sys/class/net/socXXX/hw_modesXXXXXX")];
	const struct str2id_s {
		char *keyword;
		int val;	/* enum wmi_host_hw_mode_config_type */
	} str2id_tbl[] = {
		{ "2G_PHYB:",		7 },
		{ "DBS_SBS:",		4 },
		{ "DBS:",		1 },
		{ "DBS_OR_SBS:",	5 },
		{ "SINGLE:",		0 },
		{ "SBS_PASSIVE:",	2 },
		{ "SBS:",		3 },
		{ NULL, -1 }
	}, *p;

	if (sizeof(*de) != de_size) {
		/* If size of struct dirent mismatch, make sure dirent.h is compiled with same
		 * parameter at readdir_wrapper() and this function both.
		 * e.g., it's different in uclibc if _FILE_OFFSET_BITS=64 is defined or not.
		 */
		dbg("%s: size of struct dirent mismatch (%zu v.s. %zu)!\n", __func__,
			sizeof(*de), de_size);
		return -1;
	}

	if (strncmp(de->d_name, "soc", 3))
		return 0;

	snprintf(path1, sizeof(path1), "%s/%s/preferred_hw_mode", basedir, de->d_name);
	snprintf(path2, sizeof(path2), "%s/%s/hw_modes", basedir, de->d_name);
	if (f_read_string(path1, preferred_hw_mode_str, sizeof(preferred_hw_mode_str)) > 0) {
		prefer_hw_mode_id = safe_atoi(preferred_hw_mode_str);
		_dprintf("%s: prefer_hw_mode_id=%d\n", de->d_name, prefer_hw_mode_id);
	} else if (f_read_string(path2, hw_modes_str, sizeof(hw_modes_str)) > 0) {
		/* Convert strings in hw_modes attribute as integer. see detect_qcawifi() */
		for (p = &str2id_tbl[0]; v < 0 && p->keyword; ++p) {
			if (!strstr(hw_modes_str, p->keyword)
			 || strlen(hw_modes_str) < strlen(p->keyword))
				continue;

			v = p->val;
		}

		if (v < 0) {
			dbg("%s: unknown string [%s] in %s\n", __func__, hw_modes_str, path2);
			return -3;
		}
		prefer_hw_mode_id = v;
		_dprintf("%s: prefer_hw_mode_id=%d, hw_modes_str=%s\n",
			de->d_name, prefer_hw_mode_id, hw_modes_str);
	} else {
		_dprintf("Read %s/%s/{preferred_hw_mode,hw_modes} failed\n", basedir, de->d_name);
		return 0;
	}

	if (prefer_hw_mode_id < 0)
		return 0;

	if (!strcmp(de->d_name, "soc0")) {
		strlcpy(tag, "hw_mode_id=", sizeof(tag));
	} else {
		snprintf(tag, sizeof(tag), "hw_mode_id_%s=", de->d_name);
	}

	if (read_and_parse(GLOBAL_I_INI, tag, "%*[^=]=%d", 1, &v))
		v = -1;

	if (v >= 8) {
		snprintf(hw_modes_str, sizeof(hw_modes_str), "%s%d", tag, prefer_hw_mode_id);
		_dprintf("%s: Set %s\n", de->d_name, hw_modes_str);
		update_ini_file(GLOBAL_I_INI, hw_modes_str);
	}

	return 0;
#else
	/* SPF11.1 */
	int v = -1, *hw_mode_id = arg;
	char val[16];	/* see hw_modes in wifi_soc_hw_modes_show() */
	char path[sizeof("/sys/class/net/socXXX/hw_modesXXXXXX")];
	struct str2id_s {
		char *keyword;
		int val;
	} str2id_tbl[] = {
		{ "DBS_SBS:", 4 },
		{ "DBS:", 1 },
		{ "DBS_OR_SBS:", 5 },
		{ "SINGLE:", 0 },
		{ "SBS_PASSIVE:", 2 },
		{ "SBS:", 3 },
		{ NULL, -1 }
	}, *p;

	if (sizeof(*de) != de_size) {
		/* If size of struct dirent mismatch, make sure readdir_wrapper() and this function see same struct dirent.h.
		 * e.g., it's different in uclibc if _FILE_OFFSET_BITS=64 is defined or not.
		 */
		dbg("%s: size of struct dirent mismatch (%u v.s. %u)!\n", __func__, sizeof(*de), de_size);
		return -1;
	}
	if (!arg)
		return -1;

	if (strncmp(de->d_name, "soc", 3))
		return 0;

	/* If socX/hw_modes absent, skip it. */
	snprintf(path, sizeof(path), "%s/%s/hw_modes", basedir, de->d_name);
	if (!f_exists(path))
		return 0;
	if (f_read_string(path, val, sizeof(val)) <= 0)
		return -2;

	/* Convert strings in hw_modes attribute as integer. see detect_qcawifi() */
	for (p = &str2id_tbl[0]; v < 0 && p->keyword; ++p) {
		if (!strstr(val, p->keyword) || strlen(val) < strlen(p->keyword))
			continue;

		v = p->val;
	}

	if (v < 0) {
		dbg("%s: unknown string [%s] in %s\n", __func__, val, path);
		return -3;
	}
	if (*hw_mode_id >= 0) {
		dbg("%s: hw_mode_id is updated more than one times\n", __func__);
	}
	*hw_mode_id = v;

	return 0;
#endif
}

static void wifi_3_0_post_hook(void)
{
#if (SPF_VER < SPF_VER_ID(12,0))
	int hw_mode_id = -1;
	char hw_mode_id_str[sizeof("hw_mode_id=6XXX")] = { 0 };

	if (readdir_wrapper(SYS_CLASS_NET, "soc", __update_hw_mode_id, &hw_mode_id) || hw_mode_id < 0)
		return;

	snprintf(hw_mode_id_str, sizeof(hw_mode_id_str), "hw_mode_id=%d", hw_mode_id);
	update_ini_file(GLOBAL_I_INI, hw_mode_id_str);
#endif
}
#endif

#if (SPF_VER >= SPF_VER_ID(12,5)) && defined(RTCONFIG_SOC_IPQ53XX)
static void ecm_wifi_plugin_post_hook(void)
{
	/* enable_vifs_qcawificfg80211() & scs=1 */
	f_write_string("/sys/kernel/debug/ecm/ecm_classifier_emesh/sawf_enabled", "3", 0, 0);
	f_write_string("/sys/kernel/debug/ecm/ecm_classifier_dscp/enabled", "0", 0, 0);
	f_write_string("/sys/kernel/debug/ecm/ecm_classifier_mscs/udp_ipsec_port", "5200", 0, 0);
	f_write_string("/sys/kernel/debug/ecm/ecm_classifier_emesh/udp_ipsec_port", "5200", 0, 0);
}
#endif

static void __mknod(char *name, mode_t mode, dev_t dev)
{
	if (mknod(name, mode, dev)) {
		printf("## mknod %s mode 0%o fail! errno %d (%s)", name, mode, errno, strerror(errno));
	}
}

void init_devs(void)
{
	int status;

	__mknod("/dev/nvram", S_IFCHR | 0666, makedev(228, 0));
	__mknod("/dev/dk0", S_IFCHR | 0666, makedev(63, 0));
	__mknod("/dev/dk1", S_IFCHR | 0666, makedev(63, 1));
	__mknod("/dev/armem", S_IFCHR | 0660, makedev(1, 13));
#if defined(RTCONFIG_SWITCH_RTL8370M_PHY_QCA8033_X2) || \
    defined(RTCONFIG_SWITCH_RTL8370MB_PHY_QCA8033_X2)
	__mknod("/dev/rtkswitch", S_IFCHR | 0666, makedev(206, 0));
#endif
#if defined(RTCONFIG_QCA953X) || \
    defined(RTCONFIG_QCA956X) || \
    defined(RTCONFIG_QCN550X)
	eval("ln", "-sf", "/dev/mtdblock2", "/dev/caldata");	/* mtdblock2 = SPI flash, Factory MTD partition */
#else
	eval("ln", "-sf", "/dev/mtdblock3", "/dev/caldata");	/* mtdblock3 = cal in NAND flash, Factory MTD partition */
#endif

	if ((status = WEXITSTATUS(modprobe("nvram_linux"))))
		printf("## modprove(nvram_linux) fail status(%d)\n", status);

#if defined(RTCONFIG_WIFI_SON) && defined(RTCONFIG_AMAS)
	if (nvram_match("wifison_ready", "1"))
		mount("overlayfs", "/www", "overlayfs", MS_MGC_VAL, "lowerdir=/www,upperdir=/www-sys");
#endif
}

void init_others(void)
{
#if defined(RTCONFIG_RT_TUF_UI) && defined(TUFBE6500)
	if (is_tuf_ui()) {
		/* Simulate TUF_UI installation at run-time.
		 * /www/images/New_ui/networkmap/white_04.gif shouldn't exist
		 * in default /www directory.
		 */
		mount("overlay", "/www", "overlay", MS_MGC_VAL, "lowerdir=/TUF_UI:/www");
	} else {
		/* Move /www/images/New_ui/networkmap/white_04.gif to
		 * /RT-BE90U/images/New_ui/networkmap/white_04.gif at compile-time.
		 */
		mount("overlay", "/www", "overlay", MS_MGC_VAL, "lowerdir=/RT-BE90U:/www");
		mount("/rom/dlna.RT-BE90U", "/rom/dlna", "none", MS_BIND, NULL);
	}
#endif
#if defined(RTCONFIG_TS_UI)
	if (nvram_get_int("CoBrand") >= 8 && nvram_get_int("CoBrand") <= 13) {
		mount("overlay", "/www", "overlay", MS_MGC_VAL, "lowerdir=/TS_UI:/www");
		dbG("mount overlay\n");
	}
#endif
	return;
}

void generate_switch_para(void)
{
#if defined(RTCONFIG_DUALWAN)
	int model;
	int wans_cap = get_wans_dualwan() & WANSCAP_WAN;
	int wanslan_cap = get_wans_dualwan() & WANSCAP_LAN;

	// generate nvram nvram according to system setting
	model = get_model();

	switch (model) {
	case MODEL_RTAC55U:
	case MODEL_RTAC55UHP:
	case MODEL_RT4GAC55U:
	case MODEL_RTN19:
	case MODEL_RTAC59U:
	case MODEL_RTAC88N:
		nvram_unset("vlan3hwname");
		if ((wans_cap && wanslan_cap)
		    )
			nvram_set("vlan3hwname", "et0");
		break;
	}
#endif
}

#if defined(RTCONFIG_SOC_IPQ8064) || defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_SOC_IPQ60XX) \
 || defined(RTCONFIG_SOC_IPQ50XX) || defined(RTCONFIG_SOC_IPQ53XX)
void tweak_wifi_ps(const char *wif)
{
	unsigned int all_cores = 3;	/* CPU0 and CPU1 */

#if defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_SOC_IPQ60XX) || defined(RTCONFIG_SOC_IPQ50XX) \
 || defined(RTCONFIG_SOC_IPQ53XX)
	all_cores = 0xF;		/* CPU0~3 */
#endif

	if (!strncmp(wif, WIF_2G, strlen(WIF_2G)) || !strcmp(wif, VPHY_2G))
		set_iface_ps(wif, 1);	/* 2G: CPU0 only */
	else
		set_iface_ps(wif, all_cores);	/* 5G/5G2/60G */
}
#endif

static void tweak_lan_wan_ps(void)
{
#if defined(RTCONFIG_SWITCH_RTL8370M_PHY_QCA8033_X2)
	/* Reference to qca-nss-drv.init */
#if LINUX_KERNEL_VERSION >= KERNEL_VERSION(4,4,0)
	set_irq_smp_affinity_by_name("nss", 1, 2);		/* eth0, LAN, nss */
	set_irq_smp_affinity_by_name("nss_queue1", 1, 2);	/* eth1, LAN, nss_queue1 */
	set_irq_smp_affinity_by_name("nss", 2, 1);		/* eth2, WAN0, nss */
	set_irq_smp_affinity_by_name("nss_queue1", 2, 1);	/* eth3, WAN1, nss_queue1 */
#else
	set_irq_smp_affinity_by_name("nss", 1, 2);	/* 1st nss irq, eth0, LAN */
	set_irq_smp_affinity_by_name("nss", 2, 2);	/* 2nd nss irq, eth1, LAN */
	set_irq_smp_affinity_by_name("nss", 3, 1);	/* 3rd nss irq, eth2, WAN0 */
	set_irq_smp_affinity_by_name("nss", 4, 1);	/* 4th nss irq, eth3, WAN1 */
#endif

	set_iface_ps("eth0", 2);
	set_iface_ps("eth1", 2);
	set_iface_ps("eth2", 1);
	set_iface_ps("eth3", 1);
#elif defined(RTCONFIG_SWITCH_RTL8370MB_PHY_QCA8033_X2)
	/* Reference to qca-nss-drv.init */
#if LINUX_KERNEL_VERSION >= KERNEL_VERSION(4,4,0)
	set_irq_smp_affinity_by_name("nss", 1, 1);		/* eth0, WAN0, nss */
	set_irq_smp_affinity_by_name("nss_queue1", 1, 2);	/* eth1, LAN, nss_queue1 */
	set_irq_smp_affinity_by_name("nss", 2, 2);		/* eth2, LAN, nss */
	set_irq_smp_affinity_by_name("nss_queue2", 2, 1);	/* eth3, WAN1, nss_queue1 */
#else
	set_irq_smp_affinity_by_name("nss", 1, 1);	/* 1st nss irq, eth0, WAN0 */
	set_irq_smp_affinity_by_name("nss", 2, 2);	/* 2nd nss irq, eth1, LAN */
	set_irq_smp_affinity_by_name("nss", 3, 2);	/* 3rd nss irq, eth2, LAN */
	set_irq_smp_affinity_by_name("nss", 4, 1);	/* 4th nss irq, eth3, WAN1 */
#endif

	set_iface_ps("eth0", 1);
	set_iface_ps("eth1", 2);
	set_iface_ps("eth2", 2);
	set_iface_ps("eth3", 1);
#elif defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_SOC_IPQ60XX) || defined(RTCONFIG_SOC_IPQ50XX)
	/* qca-nss-drv.init */
	set_irq_smp_affinity_by_name("nss_queue1", 0, 2);
	set_irq_smp_affinity_by_name("nss_queue2", 0, 4);
	set_irq_smp_affinity_by_name("nss_queue3", 0, 8);
	f_write_string("/proc/sys/dev/nss/rps/enable", "1", 0, 0);
#elif defined(RTCONFIG_SOC_IPQ53XX)
#if defined(RTCONFIG_WIFI_IPQ53XX_QCN64XX)
	set_irq_smp_affinity_by_name("pci0_ppe_wbm_rel", 0 , 4);
	set_irq_smp_affinity_by_name("pci0_reo2ppe", 0 , 4);
	set_irq_smp_affinity_by_name("pci0_ppe2tcl", 0 , 4);
#endif

	/* qca-nss-ppe */
	set_irq_smp_affinity_by_name("edma_txcmpl_4", 0 , 1);
	set_irq_smp_affinity_by_name("edma_txcmpl_5", 0 , 2);
	set_irq_smp_affinity_by_name("edma_txcmpl_6", 0 , 4);
	set_irq_smp_affinity_by_name("edma_txcmpl_7", 0 , 8);

	set_irq_smp_affinity_by_name("edma_txcmpl_8", 0 , 1);
	set_irq_smp_affinity_by_name("edma_txcmpl_9", 0 , 2);
	set_irq_smp_affinity_by_name("edma_txcmpl_10", 0 , 4);
	set_irq_smp_affinity_by_name("edma_txcmpl_11", 0 , 8);

	set_irq_smp_affinity_by_name("edma_txcmpl_12", 0 , 1);
	set_irq_smp_affinity_by_name("edma_txcmpl_13", 0 , 2);
	set_irq_smp_affinity_by_name("edma_txcmpl_14", 0 , 4);
	set_irq_smp_affinity_by_name("edma_txcmpl_15", 0 , 8);

	/* qca-nss-ppe */
	set_irq_smp_affinity_by_name("edma_rxdesc_12", 0 , 1);
	set_irq_smp_affinity_by_name("edma_rxdesc_13", 0 , 2);
	set_irq_smp_affinity_by_name("edma_rxdesc_14", 0 , 4);
	set_irq_smp_affinity_by_name("edma_rxdesc_15", 0 , 8);
#if defined(RTCONFIG_WIFI_IPQ53XX_QCN64XX)
	// Enable RPS to use only 3 cores by default
	f_write_string("/proc/sys/net/edma/rps_num_cores", "3", 0, 0);
#endif

	/* qca-nss-eip */
	set_irq_smp_affinity_by_name("eip_irq_ring_0", 0 , 1);
	set_irq_smp_affinity_by_name("eip_irq_ring_1", 0 , 2);
	set_irq_smp_affinity_by_name("eip_irq_ring_2", 0 , 4);
	set_irq_smp_affinity_by_name("eip_irq_ring_3", 0 , 8);
#endif
}

#if defined(RTCONFIG_SWITCH_QCA8075_QCA8337_PHY_AQR107_AR8035_QCA8033)
/* Set default MAC address of WAN interfaces. */
static void set_wanifaces_hwaddr(void)
{
	int i;
	char *wan_ifaces[3] = { 0 };
	char macbuf[sizeof("00:11:22:33:44:55XXX")];
	unsigned char ea[6];

	switch (get_model()) {
	case MODEL_GTAXY16000:
	case MODEL_RTAX89U:
		wan_ifaces[0] = "eth3";
		wan_ifaces[1] = "eth5";
		wan_ifaces[2] = "eth4";
		break;
	}

	for (i = 0; i < 3; ++i) {
		if (wan_ifaces[i] == NULL)
			break;

		ether_atoe(get_wan_hwaddr(), ea);
		ea[5] += i;
		ether_etoa(ea, macbuf);
		eval("ifconfig", wan_ifaces[i], "hw", "ether", macbuf);
	}
}
#else
static inline void set_wanifaces_hwaddr(void) {}
#endif


static void init_switch_qca(void)
{
	char *qca_module_list[] = {
#if defined(RTCONFIG_SOC_IPQ8064)
#if defined(RTCONFIG_SWITCH_QCA8337N)
		"qca-ssdk",
#endif
		"qca-nss-gmac",
		"qca-nss-drv",
		"qca-nss-qdisc",
#elif defined(RTCONFIG_SOC_IPQ8074)
		/* 09-shortcut-fe */
		"shortcut-fe", "shortcut-fe-ipv6",

		/* 10-shortcut-fe-drv */
		"shortcut-fe-drv",
#if defined(RTCONFIG_SWITCH_QCA8337N) || defined(RTCONFIG_SWITCH_QCA8075_QCA8337_PHY_AQR107_AR8035_QCA8033)
		"qca-ssdk",
#endif
		"qca-nss-dp",
		"qca-nss-drv",
		"hyfi-bridging", "ecm",
#elif defined(RTCONFIG_SOC_IPQ40XX)
		"qrfs",
		"shortcut-fe", "shortcut-fe-ipv6", "shortcut-fe-cm",
		"qca-ssdk",
		"essedma",
/*		"qca-nss-gmac", "qca-nss-drv",	*/
#if defined(RTCONFIG_WIFI_SON)
		"hyfi_qdisc", "hyfi-bridging",
#endif
#elif defined(MAPAC1750)
		"shortcut-fe", "shortcut-fe-ipv6", "shortcut-fe-cm",
		"qca-ssdk",
		"hyfi_qdisc", "hyfi-bridging",
#elif defined(RTCONFIG_QCN550X) && defined(RTCONFIG_SWITCH_QCA8337N)
		"qca-ssdk",
#elif defined(RTCONFIG_SOC_IPQ60XX) || defined(RTCONFIG_SOC_IPQ50XX)
		"qca-ssdk",
		"qca-nss-dp",
		"qca-nss-drv",
		"hyfi-bridging", "ecm",
#elif defined(RTCONFIG_SOC_IPQ53XX)
		"qca-ssdk",
#endif
#ifdef RTCONFIG_QCA_MCSD
		"qca-mcs",
#endif	/* RTCONFIG_QCA_MCSD */
#if defined(RTCONFIG_STRONGSWAN) ||  defined(RTCONFIG_QUICKSEC)
#if LINUX_KERNEL_VERSION >= KERNEL_VERSION(4,4,0)
		"ip_tunnel",
#endif
		"tunnel4", /*"tunnel6",*/
		"ah4", "esp4", "xfrm4_tunnel", "ipcomp",
		/*"ah6", "esp6", "xfrm6_tunnel", "ipcomp6", */
#if defined(RTCONFIG_SOC_IPQ8064)
#if LINUX_KERNEL_VERSION >= KERNEL_VERSION(4,4,0)
		"udp_tunnel", "ip6_udp_tunnel",
		"qca-nss-l2tpv2", "qca-nss-lag-mgr", "qca-nss-map-t",
		"qca-nss-pptp", "qca-nss-crypto", "qca-nss-crypto-tool", 
		"qca-nss-macsec", "qca-nss-ipsecmgr", "qca-nss-qdisc",
		"qca-nss-cfi-ocf", "qca-nss-cfi-cryptoapi",
		"qca-nss-ipsec", /*"qca-nss-tun6rd",*/ "qca-nss-tunipip6",
#else
//		"qca-nss-capwapmgr",
		"qca-nss-cfi-cryptoapi",
//		"qca-nss-crypto-tool", "qca-nss-crypto",
//		"qca-nss-profile-drv", "qca-nss-tun6rd",
//		"qca-nss-tunipip6", "qca-nss-ipsec",
//		"qca-nss-ipsecmgr", "qca-nss-cfi-ocf",
#endif
#elif defined(RTCONFIG_SOC_IPQ8074)
		"qca-nss-bridge-mgr",
		"qca-nss-l2tpv2", "qca-nss-lag-mgr", "qca-nss-map-t",
		"qca-nss-pppoe", "qca-nss-pptp", "qca-nss-vlan-mgr",

		/* 52-diag-char */
		"diagchar",

		/* 52-qca-nss-crypto */
		"qca-nss-crypto", "qca-nss-crypto-tool",

		"qca-nss-ipsecmgr",
		"qca-nss-qdisc",

		/* 59-qca-nss-cfi */
		"qca-nss-cfi-ocf", "qca-nss-cfi-cryptoapi", "qca-nss-ipsec",

		// "qca-nss-tun6rd",
		"qca-nss-tunipip6",
#endif	/* RTCONFIG_SOC_IPQ8064 */
#endif	/* RTCONFIG_STRONGSWAN || RTCONFIG_QUICKSEC */
#if defined(RTCONFIG_SOC_IPQ60XX) || defined(RTCONFIG_SOC_IPQ50XX)
#ifdef RTCONFIG_QSDK_LM256
		"qca-nss-pppoe",
		"qca-nss-macsec",
#else
		"ip_tunnel",
		"tunnel4", /*"tunnel6",*/
		"ah4", "esp4", "xfrm4_tunnel", "ipcomp",
		"qca-nss-bridge-mgr", "qca-nss-gre",
		"qca-nss-l2tpv2", "qca-nss-lag-mgr", "qca-nss-map-t",
		"qca-nss-pppoe", "qca-nss-pptp", "qca-nss-vlan",
		/* 52-diag-char */
		"diagchar",
		/* 52-qca-nss-crypto */
		"qca-nss-crypto",
		"qca-nss-macsec",
		"qca-nss-qdisc",
		"qca-nss-cfi-cryptoapi",
		"qca-nss-ipsecmgr",
		"qca-nss-tun6rd",
		"qca-nss-tunipip6",
		"qca-nss-cfi-ocf",
#endif
#endif	/* RTCONFIG_SOC_IPQ60XX || RTCONFIG_SOC_IPQ50XX */
#if defined(RTCONFIG_SOC_IPQ53XX)
/* ls /etc/modules.d		| lsmod|grep nss
 * _____________________________|______________________
 * ......			|
 * 09-qca-nss-sfe		| qca_nss_dp
 * 31-qca-nss-dp		| qca_nss_eip
 * 31-qca-nss-ppe		| qca_nss_macsec
 * 50-qca-ovsmgr		| qca_nss_ppe
 * 50-usb-lib-composite		| qca_nss_ppe_bridge_mgr
 * 51-qca-hyfi-bridge		| qca_nss_ppe_ds
 * 51-qca-nss-ppe-bridge-mgr	| qca_nss_ppe_gretap
 * 51-qca-nss-ppe-ds		| qca_nss_ppe_lag
 * 51-qca-nss-ppe-lag-mgr	| qca_nss_ppe_mapt
 * 51-qca-nss-ppe-pppoe-mgr	| qca_nss_ppe_pppoe_mgr
 * 51-qca-nss-ppe-vlan-mgr	| qca_nss_ppe_rule
 * 52-diag-char			| qca_nss_ppe_tun
 * 52-qca-mcs			| qca_nss_ppe_tunipip6
 * 52-qca-nss-eip		| qca_nss_ppe_vlan
 * 52-qca-nss-macsec		| qca_nss_ppe_vp
 * 52-qca-nss-ppe-rule		| qca_nss_ppe_vxlanmgr
 * 52-qca-nss-ppe-vp		| qca_nss_sfe
 * 52-usb-f-diag		|
 * 52-usb-gdiag			|
 * 53-qca-nss-ppe-gretap	|
 * 53-qca-nss-ppe-mapt		|
 * 53-qca-nss-ppe-tun		|
 * 53-qca-nss-ppe-vxlanmgr	|
 * 54-qca-nss-ppe-tunipip6	|
 * 56-bootconfig		|
 * ......
 */
		"ip_tunnel",
		"tunnel4", /*"tunnel6",*/

		/* 9 */
		"qca-nss-sfe",
		/* 31 */
		"qca-nss-dp", "qca-nss-ppe",
		/* 50 */
		"qca-ovsmgr",
		/* 51 */
		"qca-nss-ppe-bridge-mgr", "qca-nss-ppe-ds", "qca-nss-ppe-lag",
		"qca-nss-ppe-pppoe-mgr", "qca-nss-ppe-vlan",
		"qca-nss-eip", /*"qca-nss-macsec",*/ "qca-nss-ppe-rule",
		/* 52 */
		"qca-nss-ppe-vp",
		"diagchar", "qca-mcs",
		/* 53 */
		"qca-nss-ppe-gretap", "qca-nss-ppe-mapt", "qca-nss-ppe-tun",
		"qca-nss-ppe-vxlanmgr", "qca-nss-ppe-tunipip6",
#endif	/* RTCONFIG_SOC_IPQ53XX */
		NULL
	}, **qmod;
	char *argv[30] = {
		"modprobe", "-s", NULL
	}, **v;
#if defined(RTCONFIG_SOC_IPQ8074)
	int max_speed = 0;
	int mod_def0_pin = 46;
	char speed[sizeof("max_speed=10000XXX")];
	char str[sizeof("d0_mod_def0_pin=57XXXXX")];
#endif

	for (qmod = &qca_module_list[0]; *qmod != NULL; ++qmod) {
		if (module_loaded(*qmod))
			continue;

		v = &argv[2];
		*v++ = *qmod;
		if (!strcmp(*qmod, "qca-nss-cfi-cryptoapi")) {
			if (nvram_get_int("ipsec_hw_crypto_enable") == 0)
			continue;
		}
#if defined(RTCONFIG_SOC_IPQ8074)
		if (!strcmp(*qmod, "qca-ssdk")) {
			/* 2.5Gbps needs qca-ssdk must >= SPF11.5 */
			max_speed = nvram_get_int("sfpp_max_speed");
			if (max_speed == 1000 || max_speed == 10000
			 || max_speed == 2500
			) {
				snprintf(speed, sizeof(speed), "max_speed=%d", max_speed);
				*v++ = speed;
			}

			if (nvram_match("sfpp_force_on", "1"))
				*v++ = "xgmac_autodet=N";

			if (nvram_match("HwId", "A"))
				mod_def0_pin = 57;
			snprintf(str, sizeof(str), "d0_mod_def0_pin=%d", mod_def0_pin);
			*v++ = str;
		}
#endif
#if defined(RTCONFIG_SOC_IPQ40XX)
		if (!strcmp(*qmod, "shortcut-fe-cm")) {
#if defined(MAPAC1300) || defined(MAPAC2200) || defined(VZWAC1300) || defined(SHAC1300) || defined(RTAC95U)
			if ((sw_mode() != SW_MODE_ROUTER) && !nvram_match("cfg_master", "1"))
				continue;
#endif
			if (IS_NON_AQOS())
#if defined(RTCONFIG_BWDPI)
				*v++ ="skip_sfe=1";
#else
				continue;
#endif
		}
		else if (!strcmp(*qmod, "essedma")) {
			*v++ = *qmod;
			if (nvram_get_int("jumbo_frame_enable")) {
				*v++ = "overwrite_mode=1";
				*v++ = "page_mode=1";
			}

#if defined(RTAC58U) || defined(RT4GAC53U) /* for RAM 128MB */
			if (get_meminfo_item("MemTotal") <= 131072)
				*v++ = "reduce_rx_ring_size=1";
#endif

		}
#if defined(MAPAC1300) || defined(MAPAC2200) || defined(VZWAC1300) || defined(SHAC1300) || defined(RTAC95U)
		else if (!strcmp(*qmod, "shortcut-fe")) {
			if(sw_mode() != SW_MODE_ROUTER)
				continue;
		}
		else if (!strcmp(*qmod, "shortcut-fe-ipv6")) {
			if(sw_mode() != SW_MODE_ROUTER)
				continue;
		}
#endif
#endif	/* RTCONFIG_SOC_IPQ40XX */

		*v++ = NULL;
		_eval(argv, NULL, 0, NULL);
	}

	char *wan0_ifname = nvram_safe_get("wan0_ifname");
	char *lan_ifname, *lan_ifnames, *ifname, *p;

#if defined(RTCONFIG_QCA953X) || defined(RTCONFIG_QCA956X) || defined(RTCONFIG_QCN550X)
	wan0_ifname = MII_IFNAME;
#endif

	tweak_lan_wan_ps();
	generate_switch_para();

	set_wanifaces_hwaddr();

#ifndef RTCONFIG_ETHBACKHAUL
	/* Set LAN MAC address to all LAN ethernet interface. */
	lan_ifname = nvram_safe_get("lan_ifname");
	if (!strncmp(lan_ifname, "br", 2) &&
	    !strstr(nvram_safe_get("lan_ifnames"), "bond"))
	{
		if ((lan_ifnames = strdup(nvram_safe_get("lan_ifnames"))) != NULL) {
			p = lan_ifnames;
			while ((ifname = strsep(&p, " ")) != NULL) {
				char *ptr = NULL;

				while (*ifname == ' ') ++ifname;
				SKIP_ABSENT_FAKE_IFACE(ifname);
				if (!strcmp(ifname, lan_ifname))
					continue;
				if (!strncmp(ifname, WIF_2G, strlen(WIF_2G))
				    || !strncmp(ifname, WIF_5G, strlen(WIF_5G))
				    || !strncmp(ifname, WIF_5G2, strlen(WIF_5G2))
				    || !strncmp(ifname, WIF_60G, strlen(WIF_60G))
				   )
					continue;
				if (*ifname == 0)
					break;

				/* If ifname is dev.vid, then find dev */
				if ((ptr = strstr(ifname, ".")))
					*ptr = '\0';
				eval("ifconfig", ifname, "hw", "ether", get_lan_hwaddr());
			}
			free(lan_ifnames);
		}
	}
#endif

	// TODO: replace to nvram controlled procedure later
#if !(defined(RTCONFIG_DETWAN) && defined(RTCONFIG_ETHBACKHAUL))		// not to change MAC
	if (strlen(wan0_ifname)) {
		eval("ifconfig", wan0_ifname, "hw", "ether", get_wan_hwaddr());
	}
#endif	/* ! RTCONFIG_ETHBACKHAUL */
	config_switch();

#ifdef RTCONFIG_SHP
	if (nvram_get_int("qos_enable") || nvram_get_int("lfp_disable_force")) {
		nvram_set("lfp_disable", "1");
	} else {
		nvram_set("lfp_disable", "0");
	}

	if (nvram_get_int("lfp_disable") == 0) {
		restart_lfp();
	}
#endif

#if defined(RTCONFIG_SOC_IPQ40XX)
	doSystem("ethtool -K eth0 gro off\n");
	doSystem("ethtool -K eth1 gro off\n");
	/* qrfs.init:
	 * enable Qualcomm Receiving Flow Steering (QRFS)
	 */
	f_write_string("/proc/qrfs/enable", "0", 0, 0); //for throughput, disable it
#endif
}

void enable_jumbo_frame(void)
{
	if (set_jumbo_frame) {
		set_jumbo_frame();
		return;
	}
#if defined(RTCONFIG_SWITCH_RTL8370M_PHY_QCA8033_X2) || \
    defined(RTCONFIG_SWITCH_RTL8370MB_PHY_QCA8033_X2)
#elif defined(RTCONFIG_SOC_IPQ60XX) || defined(RTCONFIG_SOC_IPQ50XX)
#elif defined(RTCONFIG_SOC_IPQ40XX)
	if (nvram_get_int("jumbo_frame_enable")) {
		eval("ifconfig", "eth0", "mtu", "9000");
		eval("ifconfig", "eth1", "mtu", "9000");
		eval("ssdk_sh", "misc", "frameMaxSize", "set", "9018");
	}
#else
	int mtu = 1518;	/* default value */
	char mtu_str[] = "9000XXX";

	if (!nvram_contains_word("rc_support", "switchctrl"))
		return;

	if (nvram_get_int("jumbo_frame_enable"))
		mtu = 9000;

	snprintf(mtu_str, sizeof(mtu_str), "%d", mtu);
	eval("swconfig", "dev", MII_IFNAME, "set", "max_frame_size", mtu_str);
#endif
}

void init_switch(void)
{
	init_switch_qca();

#if defined(RTCONFIG_SOC_IPQ8064) || defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_SOC_IPQ60XX) \
 || defined(RTCONFIG_SOC_IPQ50XX) || defined(RTCONFIG_SOC_IPQ53XX)
	init_ecm();
#endif
}

/**
 * Setup a VLAN.
 * @vid:	VLAN ID
 * @prio:	VLAN PRIO
 * @mask:	bit31~16:	untag mask
 * 		bit15~0:	port member mask
 * @return:
 * 	0:	success
 *  otherwise:	fail
 *
 * bit definition of untag mask/port member mask
 * 0:	Port 0, LANx port which is closed to WAN port in visual.
 * 1:	Port 1
 * 2:	Port 2
 * 3:	Port 3
 * 4:	Port 4, WAN port
 * 9:	Port 9, RGMII/MII port that is used to connect CPU and WAN port.
 * 	a. If you only have one RGMII/MII port and it is shared by WAN/LAN ports,
 * 	   you have to define two VLAN interface for WAN/LAN ports respectively.
 * 	b. If your switch chip choose another port as same feature, convert bit9
 * 	   to your own port in low-level driver.
 */
static int __setup_vlan(int vid, int prio, unsigned int mask)
{
	char vlan_str[] = "4096XXX";
	char prio_str[] = "7XXX";
	char mask_str[] = "0x00000000XXX";
	char *set_vlan_argv[] = { "rtkswitch", "36", vlan_str, NULL };
	char *set_prio_argv[] = { "rtkswitch", "37", prio_str, NULL };
	char *set_mask_argv[] = { "rtkswitch", "39", mask_str, NULL };

	if (vid > 4096) {
		_dprintf("%s: invalid vid %d\n", __func__, vid);
		return -1;
	}

	if (prio > 7)
		prio = 0;

	_dprintf("%s: vid %d prio %d mask 0x%08x\n", __func__, vid, prio, mask);

	if (vid >= 0) {
		snprintf(vlan_str, sizeof(vlan_str), "%d", vid);
		_eval(set_vlan_argv, NULL, 0, NULL);
	}

	if (prio >= 0) {
		snprintf(prio_str, sizeof(prio_str), "%d", prio);
		_eval(set_prio_argv, NULL, 0, NULL);
	}

	snprintf(mask_str, sizeof(mask_str), "0x%08x", mask);
	_eval(set_mask_argv, NULL, 0, NULL);

	return 0;
}

int config_switch_for_first_time = 1;
void config_switch(void)
{
	int model = get_model();
	int stbport;
	int controlrate_unknown_unicast;
	int controlrate_unknown_multicast;
	int controlrate_multicast;
	int controlrate_broadcast;
	int merge_wan_port_into_lan_ports;
#if defined(RTCONFIG_BONDING_WAN)
	char nv_mode[] = "bondXXX_mode", nv_policy[] = "bondXXX_policy";
#endif
#if defined(RTCONFIG_SWITCH_RTL8370M_PHY_QCA8033_X2) || \
    defined(RTCONFIG_SWITCH_RTL8370MB_PHY_QCA8033_X2)
	char *str;
	int wans = get_wans_dualwan();
#endif

	dbG("link down all ports\n");
	eval("rtkswitch", "17");	// link down all ports

	switch (model) {
	case MODEL_RTAC55U:	/* fall through */
	case MODEL_RTAC55UHP:	/* fall through */
	case MODEL_RT4GAC55U:	/* fall through */
	case MODEL_RTN19:	/* fall through */
	case MODEL_RTAC59U:	/* fall through */
	case MODEL_RTAC59CD6R:	/* fall through */
	case MODEL_RTAC59CD6N:	/* fall through */
	case MODEL_RTAC58U:	/* fall through */
	case MODEL_RT4GAC53U:	/* fall through */
	case MODEL_RT4GAC56:	/* fall through */
	case MODEL_RTAC82U:	/* fall through */
	case MODEL_MAPAC1300:	/* fall through */
	case MODEL_VZWAC1300:	/* fall through */
	case MODEL_SHAC1300:	/* fall through */
	case MODEL_MAPAC1750:	/* fall through */
	case MODEL_MAPAC2200:	/* fall through */
	case MODEL_RTAC88N:	/* fall through */
	case MODEL_RTAC95U:	/* fall through */
#ifdef RTCONFIG_ETHBACKHAUL
		merge_wan_port_into_lan_ports = 0;
#else
		merge_wan_port_into_lan_ports = 1;
#endif
		break;
	default:
		merge_wan_port_into_lan_ports = 0;
	}

	if (config_switch_for_first_time)
		config_switch_for_first_time = 0;
	else {
		dbG("software reset\n");
		eval("rtkswitch", "27");	// software reset
	}

	pre_config_switch();

#if defined(RTCONFIG_BONDING_WAN)
	if (sw_mode() == SW_MODE_ROUTER && bond_wan_enabled() && nvram_get("bond1_ifnames")) {
		sprintf(nv_mode, "%s_mode", "bond1");
		sprintf(nv_policy, "%s_policy", "bond1");
		set_bonding("bond1", nvram_get(nv_mode), nvram_get(nv_policy), get_wan_hwaddr());
	}
#endif

#ifdef RTCONFIG_DEFAULT_AP_MODE
	if (!is_router_mode())
		system("rtkswitch 8 7"); // LLLLL
	else
#endif
	system("rtkswitch 8 0"); // init, rtkswitch 114,115,14,15 need it
	if (is_routing_enabled()) {
		char parm_buf[] = "XXX";

		stbport = safe_atoi(nvram_safe_get("switch_stb_x"));
		if (stbport < 0 || stbport > 6) stbport = 0;
		dbG("ISP Profile/STB: %s/%d\n", nvram_safe_get("switch_wantag"), stbport);
		/* stbport:	Model-independent	unifi_malaysia=1	otherwise
		 * 		IPTV STB port		(RT-N56U)		(RT-N56U)
		 * -----------------------------------------------------------------------
		 *	0:	N/A			LLLLW
		 *	1:	LAN1			LLLTW			LLLWW
		 *	2:	LAN2			LLTLW			LLWLW
		 *	3:	LAN3			LTLLW			LWLLW
		 *	4:	LAN4			TLLLW			WLLLW
		 *	5:	LAN1 + LAN2		LLTTW			LLWWW
		 *	6:	LAN3 + LAN4		TTLLW			WWLLW
		 */

		/* portmask in rtkswitch
		 * 	P9	P8	P7	P6	P5	P4	P3	P2	P1	P0
		 * 	MII-W	MII-L	-	-	-	WAN	LAN1	LAN2	LAN3	LAN4
		 */

		if (!nvram_match("switch_wantag", "none")&&!nvram_match("switch_wantag", "")) {
			//2012.03 Yau modify
			char tmp[128];
			char *p;
			int voip_port = 0;
			int t, vlan_val = -1, prio_val = -1;
			unsigned int mask = 0;

//			voip_port = safe_atoi(nvram_safe_get("voip_port"));
			voip_port = 3;
			if (voip_port < 0 || voip_port > 4)
				voip_port = 0;		

			/* Fixed Ports Now*/
			stbport = 4;	
			voip_port = 3;
	
			snprintf(tmp, sizeof(tmp), "rtkswitch 29 %d", voip_port);
			system(tmp);	

			if (!strncmp(nvram_safe_get("switch_wantag"), "unifi", 5)) {
				/* Added for Unifi. Cherry Cho modified in 2011/6/28.*/
				if(strstr(nvram_safe_get("switch_wantag"), "home")) {
					system("rtkswitch 38 1");		/* IPTV: P0 */
					/* Internet:	untag: P9;   port: P4, P9 */
					__setup_vlan(500, 0, 0x02000210);
					/* IPTV:	untag: P0;   port: P0, P4 */
					__setup_vlan(600, 0, 0x00010011);
				}
				else if (strstr(nvram_safe_get("switch_wantag"), "biz_voip")) {
					system("rtkswitch 40 1");		/* admin all frames on all ports */
					system("rtkswitch 38 2");		/* VoIP: P1  2 = 0x10 */
					/* Internet:	untag: P9;   port: P4, P9 */
					__setup_vlan(500, 0, 0x02000210);
					/* VoIP:	untag: P1;  port: P1, P4 */
					//VoIP Port: P1 untag (special case)
					__setup_vlan(400, 0, 0x00020012);
				}
				else {
					/* No IPTV. Business package */
					/* Internet:	untag: P9;   port: P4, P9 */
					system("rtkswitch 38 0");
					__setup_vlan(500, 0, 0x02000210);
				}
			}
			else if (!strncmp(nvram_safe_get("switch_wantag"), "singtel", 7)) {
				/* Added for SingTel's exStream issues. Cherry Cho modified in 2011/7/19. */
				if(strstr(nvram_safe_get("switch_wantag"), "mio")) {
					/* Connect Singtel MIO box to P3 */
					system("rtkswitch 40 1");		/* admin all frames on all ports */
					system("rtkswitch 38 3");		/* IPTV: P0  VoIP: P1 */
					/* Internet:	untag: P9;   port: P4, P9 */
					__setup_vlan(10, 0, 0x02000210);
					/* IPTV:	untag: P0;   port: P0, P4 */
					__setup_vlan(20, 4, 0x00010011);
					/* VoIP:	untag: N/A;  port: P1, P4 */
					//VoIP Port: P1 tag
					__setup_vlan(30, 4, 0x00000012);
				}
				else if (strstr(nvram_safe_get("switch_wantag"), "mstb")) {
					system("rtkswitch 38 3");		/* IPTV: P0 & P1 */
					/* Internet:	untag: P9;       port: P4, P9 */
					__setup_vlan(10, 0, 0x02000210);
					/* IPTV:	untag: P0, P1;   port: P0, P1, P4 */
					__setup_vlan(20, 4, 0x00030013);
				}
				else {
					//Connect user's own ATA to lan port and use VoIP by Singtel WAN side VoIP gateway at voip.singtel.com
					system("rtkswitch 38 1");		/* IPTV: P0 */
					/* Internet:	untag: P9;   port: P4, P9 */
					__setup_vlan(10, 0, 0x02000210);
					/* IPTV:	untag: P0;   port: P0, P4 */
					__setup_vlan(20, 4, 0x00010011);
				}
			}
			else if (!strcmp(nvram_safe_get("switch_wantag"), "m1_fiber")) {
				//VoIP: P1 tag. Cherry Cho added in 2012/1/13.
				system("rtkswitch 40 1");			/* admin all frames on all ports */
				system("rtkswitch 38 2");			/* VoIP: P1  2 = 0x10 */
				/* Internet:	untag: P9;   port: P4, P9 */
				__setup_vlan(1103, 1, 0x02000210);
				/* VoIP:	untag: N/A;  port: P1, P4 */
				//VoIP Port: P1 tag
				__setup_vlan(1107, 1, 0x00000012);
			}
			else if (!strcmp(nvram_safe_get("switch_wantag"), "maxis_fiber")) {
				//VoIP: P1 tag. Cherry Cho added in 2012/11/6.
				system("rtkswitch 40 1");			/* admin all frames on all ports */
				system("rtkswitch 38 2");			/* VoIP: P1  2 = 0x10 */
				/* Internet:	untag: P9;   port: P4, P9 */
				__setup_vlan(621, 0, 0x02000210);
				/* VoIP:	untag: N/A;  port: P1, P4 */
				__setup_vlan(821, 0, 0x00000012);

				__setup_vlan(822, 0, 0x00000012);		/* untag: N/A;  port: P1, P4 */ //VoIP Port: P1 tag
			}
			else if (!strcmp(nvram_safe_get("switch_wantag"), "maxis_fiber_sp")) {
				//VoIP: P1 tag. Cherry Cho added in 2012/11/6.
				system("rtkswitch 40 1");			/* admin all frames on all ports */
				system("rtkswitch 38 2");			/* VoIP: P1  2 = 0x10 */
				/* Internet:	untag: P9;   port: P4, P9 */
				__setup_vlan(11, 0, 0x02000210);
				/* VoIP:	untag: N/A;  port: P1, P4 */
				//VoIP Port: P1 tag
				__setup_vlan(14, 0, 0x00000012);
			}
			else if (!strcmp(nvram_safe_get("switch_wantag"), "maxis_cts")) {
				//VoIP: P1 tag. Cherry Cho added in 2012/11/6.
				system("rtkswitch 40 1");			/* admin all frames on all ports */
				system("rtkswitch 38 2");			/* VoIP: P1  2 = 0x10 */
				/* Internet:	untag: P9;   port: P4, P9 */
				__setup_vlan(41, 0, 0x02000210);
				/* VoIP:	untag: N/A;  port: P1, P4 */
				//VoIP Port: P1 tag
				__setup_vlan(44, 0, 0x00000012);
			}
			else if (!strcmp(nvram_safe_get("switch_wantag"), "maxis_sacofa")) {
				//VoIP: P1 tag. Cherry Cho added in 2012/11/6.
				system("rtkswitch 40 1");			/* admin all frames on all ports */
				system("rtkswitch 38 2");			/* VoIP: P1  2 = 0x10 */
				/* Internet:	untag: P9;   port: P4, P9 */
				__setup_vlan(31, 0, 0x02000210);
				/* VoIP:	untag: N/A;  port: P1, P4 */
				//VoIP Port: P1 tag
				__setup_vlan(34, 0, 0x00000012);
			}
			else if (!strcmp(nvram_safe_get("switch_wantag"), "maxis_tnb")) {
				//VoIP: P1 tag. Cherry Cho added in 2012/11/6.
				system("rtkswitch 40 1");			/* admin all frames on all ports */
				system("rtkswitch 38 2");			/* VoIP: P1  2 = 0x10 */
				/* Internet:	untag: P9;   port: P4, P9 */
				__setup_vlan(51, 0, 0x02000210);
				/* VoIP:	untag: N/A;  port: P1, P4 */
				//VoIP Port: P1 tag
				__setup_vlan(54, 0, 0x00000012);
			}
#ifdef RTCONFIG_MULTICAST_IPTV
			else if (!strcmp(nvram_safe_get("switch_wantag"), "movistar")) {
#if defined(RTCONFIG_SOC_IPQ40XX)
				doSystem("echo 10 > /proc/sys/net/edma/default_group1_vlan_tag");
#elif defined(RTCONFIG_QCA953X) || defined(RTCONFIG_QCA956X) || defined(RTCONFIG_QCN550X)
				nvram_set("vlan_idx", "1");
#else
				/* Software bridge based IPTV implementation need to create VLAN interface
				 * in __setup_vlan() which is called by config_switch().  In this case,
				 * start_vlan() hasn't been executed.  So, software bridge based IPTV
				 * implementation should not enable BFL_ENETVLAN bit in boardflags and has
				 * to call set_wan_tag() here instead.
				 */
				if (sw_based_iptv() && !(nvram_get_int("boardflags") & BFL_ENETVLAN)) {
					char wan_base_if[IFNAMSIZ] = "";

					strlcpy(wan_base_if, get_wan_base_if(), sizeof(wan_base_if));
					set_wan_tag(wan_base_if);
				}
#endif
#if 0	//set in set_wan_tag() since (switch_stb_x > 6) and need vlan interface by vconfig.
				system("rtkswitch 40 1");			/* admin all frames on all ports */
				/* Internet/STB/VoIP:	untag: N/A;   port: P4, P9 */
				__setup_vlan(6, 0, 0x00000210);
				__setup_vlan(2, 0, 0x00000210);
				__setup_vlan(3, 0, 0x00000210);
#endif
			}
			else if (!strcmp(nvram_safe_get("switch_wantag"), "starhub")) {
				system("rtkswitch 38 0");		//No IPTV and VoIP ports
			}
#endif
			else if (!strcmp(nvram_safe_get("switch_wantag"), "meo")) {
				system("rtkswitch 40 1");			/* admin all frames on all ports */
				system("rtkswitch 38 1");			/* VoIP: P0 */
				/* Internet/VoIP:	untag: P9;   port: P0, P4, P9 */
				__setup_vlan(12, 0, 0x02000211);
			}
			else if (!strcmp(nvram_safe_get("switch_wantag"), "vodafone")) {
				system("rtkswitch 40 1");			/* admin all frames on all ports */
				system("rtkswitch 38 3");			/* Vodafone: P0  IPTV: P1 */
				/* Internet:	untag: P9;   port: P4, P9 */
				__setup_vlan(100, 1, 0x02000211);
				/* IPTV:	untag: N/A;  port: P0, P4 */
				__setup_vlan(101, 0, 0x00000011);
				/* Vodafone:	untag: P1;   port: P0, P1, P4 */
				__setup_vlan(105, 1, 0x00020013);
			}
			else if (!strcmp(nvram_safe_get("switch_wantag"), "hinet")) {
				if (sw_bridge_iptv_different_switches()) {
					/* Bridge:	untag: P0, P4, P9;	port: P0, P4, P9
					 * WAN:		no VLAN (hacked in API for SW based IPTV)
					 * STB:		Ctag, return value of get_sw_bridge_iptv_vid().
					 */
					__setup_vlan(get_sw_bridge_iptv_vid(), 0, 0x02110211);
				} else {
					eval("rtkswitch", "8", "4");		/* LAN4 with WAN */
				}
			}
			else if (!strcmp(nvram_safe_get("switch_wantag"), "hinet_mesh")) { /* Hinet MOD Mesh */
				/* Nothing to do. */
			}
			else if (!strcmp(nvram_safe_get("switch_wantag"), "superonline")) {
				system("rtkswitch 38 1");			/* IPTV: P0 */
				/* Internet:	untag: P4, P9;   port: P4, P9 */
				__setup_vlan(2, 0, 0x02100210);
				/* IPTV:	untag: P0;   port: P0, P4 */
				__setup_vlan(103, 4, 0x00010011);
			}
#if defined(RTAC58U) || defined(RTAC59U)
			else if (!strcmp(nvram_safe_get("switch_wantag"), "stuff_fibre")) {
				system("rtkswitch 38 0");			//No IPTV and VoIP ports
				/* Internet:	untag: P9;   port: P4, P9 */
				__setup_vlan(10, 0, 0x02000210);
			}
#endif
			else {
				/* Cherry Cho added in 2011/7/11. */
				/* Initialize VLAN and set Port Isolation */
				if(strcmp(nvram_safe_get("switch_wan1tagid"), "") && strcmp(nvram_safe_get("switch_wan2tagid"), ""))
					system("rtkswitch 38 3");		// 3 = 0x11 IPTV: P0  VoIP: P1
				else if(strcmp(nvram_safe_get("switch_wan1tagid"), ""))
					system("rtkswitch 38 1");		// 1 = 0x01 IPTV: P0
				else if(strcmp(nvram_safe_get("switch_wan2tagid"), ""))
					system("rtkswitch 38 2");		// 2 = 0x10 VoIP: P1
				else
					system("rtkswitch 38 0");		//No IPTV and VoIP ports

				/*++ Get and set Vlan Information */
				if(strcmp(nvram_safe_get("switch_wan0tagid"), "") != 0) {
					// Internet on WAN (port 4)
					if ((p = nvram_get("switch_wan0tagid")) != NULL) {
						t = safe_atoi(p);
						if((t >= 2) && (t <= 4094))
							vlan_val = t;
					}

					if((p = nvram_get("switch_wan0prio")) != NULL && *p != '\0')
						prio_val = safe_atoi(p);

#if defined(RTCONFIG_SOC_IPQ40XX)
					if (vlan_val == 2)
						doSystem("echo 10 > /proc/sys/net/edma/default_group1_vlan_tag");
#endif
					__setup_vlan(vlan_val, prio_val, 0x02000210);
				} else { /* switch_wan0tagid empty case */
#if defined(RTCONFIG_SOC_IPQ40XX)
					__setup_vlan(2, 0, 0x00100210);
#elif defined(RTCONFIG_QCA953X) || defined(RTCONFIG_QCA956X) || defined(RTCONFIG_QCN550X)
					/* need to fix by using ssdk_sh instead of swconfig */
#endif
				}

				if(strcmp(nvram_safe_get("switch_wan1tagid"), "") != 0) {
					// IPTV on LAN4 (port 0)
					if ((p = nvram_get("switch_wan1tagid")) != NULL) {
						t = safe_atoi(p);
						if((t >= 2) && (t <= 4094))
							vlan_val = t;
					}

					if((p = nvram_get("switch_wan1prio")) != NULL && *p != '\0')
						prio_val = safe_atoi(p);

					if(!strcmp(nvram_safe_get("switch_wan1tagid"), nvram_safe_get("switch_wan2tagid")))
						mask = 0x00030013;	//IPTV=VOIP
					else
						mask = 0x00010011;	//IPTV Port: P0 untag 65553 = 0x10 011

					__setup_vlan(vlan_val, prio_val, mask);
				}	

				if(strcmp(nvram_safe_get("switch_wan2tagid"), "") != 0) {
					// VoIP on LAN3 (port 1)
					if ((p = nvram_get("switch_wan2tagid")) != NULL) {
						t = safe_atoi(p);
						if((t >= 2) && (t <= 4094))
							vlan_val = t;
					}

					if((p = nvram_get("switch_wan2prio")) != NULL && *p != '\0')
						prio_val = safe_atoi(p);

					if(!strcmp(nvram_safe_get("switch_wan1tagid"), nvram_safe_get("switch_wan2tagid")))
						mask = 0x00030013;	//IPTV=VOIP
					else
						mask = 0x00020012;	//VoIP Port: P1 untag

					__setup_vlan(vlan_val, prio_val, mask);
				}

			}
		}
		else /* switch_wantag empty case */
		{
			const int sw_br_vid = get_sw_bridge_iptv_vid();
			char *str __attribute__((unused));

			if (stbport) {
				snprintf(parm_buf, sizeof(parm_buf), "%d", stbport);
				eval("rtkswitch", "8", parm_buf);
			}
			if (sw_based_iptv()) {
				/* WAN:	no VLAN (hacked in API for SW based IPTV)
				 * STB:	according to switch_stb_x nvram variable.
				 */
				switch (stbport) {
				case 0:	/* none */
					break;
				case 1:	/* LAN1 */
					/* untag: P3, P4, P9;	port: P3, P4, P9 */
					__setup_vlan(sw_br_vid, 0, 0x02180218);
					break;
				case 2:	/* LAN2 */
					/* untag: P2, P4, P9;	port: P2, P4, P9 */
					__setup_vlan(sw_br_vid, 0, 0x02140214);
					break;
				case 3:	/* LAN3 */
					/* untag: P1, P4, P9;	port: P1, P4, P9 */
					__setup_vlan(sw_br_vid, 0, 0x02120212);
					break;
				case 4:	/* LAN4 */
					/* untag: P0, P4, P9;	port: P0, P4, P9 */
					__setup_vlan(sw_br_vid, 0, 0x02110211);
					break;
				case 5:	/* LAN1 & LAN2 */
					/* untag: P3, P2, P4, P9;	port: P3, P2, P4, P9 */
					__setup_vlan(sw_br_vid, 0, 0x021C021C);
					break;
				case 6:	/* LAN3 & LAN4 */
					/* untag: P1, P0, P4, P9;	port: P1, P0, P4, P9 */
					__setup_vlan(sw_br_vid, 0, 0x02130213);
					break;
				default:
					dbg("%s: unknown stb_stb_x %d\n", __func__, nvram_get_int("switch_stb_x"));
				}
			}
#if defined(RTCONFIG_SWITCH_RTL8370M_PHY_QCA8033_X2) || \
    defined(RTCONFIG_SWITCH_RTL8370MB_PHY_QCA8033_X2)
			str = nvram_get("lan_trunk_0");
			if(str != NULL && str[0] != '\0')
			{
				eval("rtkswitch", "45", "0");
				eval("rtkswitch", "46", str);
			}

			str = nvram_get("lan_trunk_1");
			if(str != NULL && str[0] != '\0')
			{
				eval("rtkswitch", "45", "1");
				eval("rtkswitch", "46", str);
			}
#endif
		}

		/* unknown unicast storm control */
		if (!nvram_get("switch_ctrlrate_unknown_unicast"))
			controlrate_unknown_unicast = 0;
		else
			controlrate_unknown_unicast = safe_atoi(nvram_get("switch_ctrlrate_unknown_unicast"));
		if (controlrate_unknown_unicast < 0 || controlrate_unknown_unicast > 1024)
			controlrate_unknown_unicast = 0;
		if (controlrate_unknown_unicast)
		{
			snprintf(parm_buf, sizeof(parm_buf), "%d", controlrate_unknown_unicast);
			eval("rtkswitch", "22", parm_buf);
		}
	
		/* unknown multicast storm control */
		if (!nvram_get("switch_ctrlrate_unknown_multicast"))
			controlrate_unknown_multicast = 0;
		else
			controlrate_unknown_multicast = safe_atoi(nvram_get("switch_ctrlrate_unknown_multicast"));
		if (controlrate_unknown_multicast < 0 || controlrate_unknown_multicast > 1024)
			controlrate_unknown_multicast = 0;
		if (controlrate_unknown_multicast) {
			snprintf(parm_buf, sizeof(parm_buf), "%d", controlrate_unknown_multicast);
			eval("rtkswitch", "23", parm_buf);
		}
	
		/* multicast storm control */
		if (!nvram_get("switch_ctrlrate_multicast"))
			controlrate_multicast = 0;
		else
			controlrate_multicast = safe_atoi(nvram_get("switch_ctrlrate_multicast"));
		if (controlrate_multicast < 0 || controlrate_multicast > 1024)
			controlrate_multicast = 0;
		if (controlrate_multicast)
		{
			snprintf(parm_buf, sizeof(parm_buf), "%d", controlrate_multicast);
			eval("rtkswitch", "24", parm_buf);
		}
	
		/* broadcast storm control */
		if (!nvram_get("switch_ctrlrate_broadcast"))
			controlrate_broadcast = 0;
		else
			controlrate_broadcast = safe_atoi(nvram_get("switch_ctrlrate_broadcast"));
		if (controlrate_broadcast < 0 || controlrate_broadcast > 1024)
			controlrate_broadcast = 0;
		if (controlrate_broadcast) {
			snprintf(parm_buf, sizeof(parm_buf), "%d", controlrate_broadcast);
			eval("rtkswitch", "25", parm_buf);
		}
	}
#ifdef RTCONFIG_WIFI_SON
	else if ((access_point_mode() && nvram_match("cfg_master", "1")) && nvram_match("wifison_ready", "1"))
		; //not to merge_wan_port_into_lan_ports.
#endif
	else if (access_point_mode())
	{
		if (merge_wan_port_into_lan_ports)
			eval("rtkswitch", "8", "100");
	}
#if defined(RTCONFIG_WIRELESSREPEATER) && defined(RTCONFIG_PROXYSTA)
	else if (mediabridge_mode())
	{
		if (merge_wan_port_into_lan_ports)
			eval("rtkswitch", "8", "100");
	}
#endif

#if defined(RTCONFIG_SWITCH_RTL8370M_PHY_QCA8033_X2) || \
    defined(RTCONFIG_SWITCH_RTL8370MB_PHY_QCA8033_X2)
	if (!(wans & WANSCAP_LAN)) {
		str = nvram_get("lan_hash_algorithm");
		if (nvram_match("lan_trunk_type", "2"))
			str = "0";	/* source port */
		if(str && *str != '\0') {
			eval("rtkswitch", "47", str);
		}
	}
#endif

	dbG("link up wan port(s)\n");
	eval("rtkswitch", "114");	// link up wan port(s)

#if !defined(RTCONFIG_SOC_IPQ40XX)
	enable_jumbo_frame();
#endif

	post_config_switch();

#if defined(RTCONFIG_BLINK_LED)
	if (is_swports_bled("led_lan_gpio")) {
		update_swports_bled("led_lan_gpio", nvram_get_int("lanports_mask"));
	}
	if (is_swports_bled("led_wan_gpio")) {
		update_swports_bled("led_wan_gpio", nvram_get_int("wanports_mask"));
	}
#if defined(RTCONFIG_WANLEDX2)
	if (is_swports_bled("led_wan2_gpio")) {
		update_swports_bled("led_wan2_gpio", nvram_get_int("wan1ports_mask"));
	}
#endif
#endif
}

/*
 * @return:
 * 	0:	switch not found or failure.
 *  otherwise:	switch found and ok.
 */
int switch_exist(void)
{
#if defined(RTCONFIG_SWITCH_RTL8370M_PHY_QCA8033_X2) || \
    defined(RTCONFIG_SWITCH_RTL8370MB_PHY_QCA8033_X2)
	int ret;
	unsigned int id[2];
	char *wan_ifname[2] = {
#if defined(RTCONFIG_SWITCH_RTL8370M_PHY_QCA8033_X2)
		"eth2", "eth3"	/* BRT-AC828 SR1 ~ SR3 wan0, wan1 interface. */
#elif defined(RTCONFIG_SWITCH_RTL8370MB_PHY_QCA8033_X2)
		"eth0", "eth3"	/* BRT-AC828 SR4 or above wan0, wan1 interface. */
#endif
	};

	ret = eval("rtkswitch", "41");
	_dprintf("eval(rtkswitch, 41) ret(%d)\n", ret);
	id[0] = (mdio_read(wan_ifname[0], MII_PHYSID1) << 16) | mdio_read(wan_ifname[0], MII_PHYSID2);
	id[1] = (mdio_read(wan_ifname[1], MII_PHYSID1) << 16) | mdio_read(wan_ifname[1], MII_PHYSID2);
	_dprintf("phy0/1 id %08x/%08x\n", id[0], id[1]);

	return (!ret && id[0] == 0x004dd074 && id[1] == 0x004dd074);
#elif defined(RTCONFIG_SWITCH_QCA8075_QCA8337_PHY_AQR107_AR8035_QCA8033)
#if defined(GTAXY16000) || defined(RTAX89U)
	static const uint32_t     aqr_id[] = { 0x03a1b4e2, 0x31c31c41, 0x31c31c12, 0 };	/* AQR107, AQR113, AQR113C */
	static const uint32_t qca8075_id[] = { 0x004dd0b1, 0 };
	static const uint32_t qca8337_id[] = { 0x004dd036, 0 };
	static const uint32_t  ar8033_id[] = { 0x004dd074, 0 };
	static const struct phy_id_list_s {
		int phy;
		int phy_type;	/* 0: General; 1: AQR C45 PHY */
		const uint32_t *pval;
		uint32_t mask;
	} phy_id_lists[] = {
		{  7, 1,     aqr_id, 0xfffffff0 },	/*  AQR1xx: WAN2, 10G RJ-45 */
		{ 11, 0, qca8075_id, 0xffffffff },	/* QCA8075: WAN1 */
		{ 10, 0, qca8075_id, 0xffffffff },	/* QCA8075: LAN1 */
		{  9, 0, qca8075_id, 0xffffffff },	/* QCA8075: LAN2 */
		{  4, 0, qca8337_id, 0xffffffff },	/* QCA8337: LAN3 */
		{  3, 0, qca8337_id, 0xffffffff },	/* QCA8337: LAN4 */
		{  0, 0, qca8337_id, 0xffffffff },	/* QCA8337: LAN5 */
		{  1, 0, qca8337_id, 0xffffffff },	/* QCA8337: LAN6 */
		{  0, 0, qca8337_id, 0xffffffff },	/* QCA8337: LAN7 */
		{  6, 0,  ar8033_id, 0xffffffff },	/*  AR8033: LAN8 */
		{ -1, 0, NULL, 0 },
	}, *p;
	static const struct gmac_spd_s {
		int port;			/* IPQ8074 GMAC port. */
		int min_speed;			/* minimal speed of the GMAC. */
	} gmac_spd[] = {
		{ 1, 1000 },			/* port 1, QCA8337 */
		{ -1, -1},
	}, *q;
	time_t t1;
	uint32_t r, id1, id2;
	const uint32_t *pval;
	unsigned int id_reg_addr;
	int i, r1, ret = 1, val, retry, fw, build, right, phy;
	char iface[IFNAMSIZ];
	char speed_cmd[sizeof("ssdk_sh swX port speed get XYYYYYY")];

	/* Check all switch/PHY ports, except SFP+ */
	for (p = &phy_id_lists[0]; p->phy >= 0; ++p) {
		if (p->phy_type) {
			/* Get correct AQR PHY address */
			if (!is_aqr_phy_exist())
				continue;
			phy = aqr_phy_addr();
		} else {
			phy = p->phy;
		}

		if (p->phy_type)
			id_reg_addr = 0x40070002;
		else
			id_reg_addr = 2;

		if ((id1 = read_phy_reg(phy, id_reg_addr)) < 0)
			id1 = 0;
		if ((id2 = read_phy_reg(phy, id_reg_addr + 1)) < 0)
			id2 = 0;
		r = ((id1 & 0xFFFF) << 16) | (id2 & 0xFFFF);
		for (right = 0, pval = p->pval; !right && *pval != 0; ++pval) {
			if ((r & p->mask) != (*pval & p->mask))
				continue;
			right++;
		}

		if (!right) {
			ret = 0;
			dbg("%s: PHY %d wrong ID: %08x\n", __func__, phy, r);
		}
	}

	/* Check all ethX interfaces. */
	for (i = 0; i < 6; ++i) {
		if (i == 5 && !is_aqr_phy_exist())
			continue;
		snprintf(iface, sizeof(iface), "eth%d", i);
		if (!iface_exist(iface)) {
			dbg("%s: eth%d not found!\n", __func__, i);
			ret = 0;
		}
	}

	if (!ret)
		return ret;

	t1 = uptime();
	while (pids("aq-fw-download") && (uptime() - t1) < 20) {
		dbg("%s: waiting aq-fw-download ...\n", __func__);
		sleep(1);
	}

	/* Check GMAC that are connected to QCA8337 switch.
	 * For those ports that are connected to Malibu (PHY) and AQR107,
	 * GMAC speed is negotiated at run-time.  Don't check them.
	 */
	for (q = &gmac_spd[0]; q->port > 0; ++q) {
		retry = 20;
		snprintf(speed_cmd, sizeof(speed_cmd), "ssdk_sh %s port speed get %d", SWID_IPQ807X, q->port);
		while (retry-- > 0) {
			/* Example:
			 * / # ssdk_sh port speed get 1
			 *
			 *  SSDK Init OK![speed]:1000(Mbps)
			 * operation done.
			 */
			if ((r1 = parse_ssdk_sh(speed_cmd, "%*[^:]:%d", 1, &val)) != 0) {
				dbg("%s: cmd [%s] val [%d], return %d\n", __func__, speed_cmd, val, ret);
				ret = 0;
				break;
			}

			if (val >= q->min_speed)
				break;

			sleep(1);
		}

		if (val < q->min_speed) {
			dbg("IPQ8074 port %d speed %d < %d\n", q->port, val, q->min_speed);
			logmessage("ATE", "IPQ8074 port %d speed %d < %d\n", q->port, val, q->min_speed);
			ret = 0;

			if (q->port == 1) {
				int phy_addr[] = { 5, 8, -1 }, *pp;

				/* Check register 0x11 of related PHYs (Malibu, AR8035) and registers of QCA8337 switch.  */
				for (pp = &phy_addr[0]; *pp >= 0; ++pp) {
					char port_str[4];
					char *phy_reg0x11[] = { "ssdk_sh", SWID_IPQ807X, "debug", "phy", "get", port_str, "0x11", NULL };

					snprintf(port_str, sizeof(port_str), "%d", *pp);
					dbg("%s: Checking PHY%d reg 0x11:\n", __func__, *pp);
					_eval(phy_reg0x11, ">/dev/console", 0, NULL);
				}

				for (i = 0; i <= 6; ++i) {
					char grp_str[4];
					char *qca8337_reg[] = { "ssdk_sh", SWID_QCA8337, "debug", "reg", "dump", grp_str, NULL };

					snprintf(grp_str, sizeof(grp_str), "%d", i);
					_eval(qca8337_reg, ">/dev/console", 0, NULL);
				}
			}
		}
	}

	if (!ret)
		return ret;

	/* Read AQR firmware version. (PHY7) */
	if (is_aqr_phy_exist()) {
		int aqr_addr = aqr_phy_addr();
		fw = read_phy_reg(aqr_addr, 0x401e0020);
		build = read_phy_reg(aqr_addr, 0x401ec885);
		if (fw < 0 || build < 0) {
			dbg("%s: Can't get AQR PHY firmware version.\n", __func__);
			ret = 0;
		} else {
			dbg("AQR PHY @ %d firmware %d.%d build %X.%X\n", aqr_addr,
				(fw >> 8) & 0xFF, fw & 0xFF, (build >> 4) & 0xF, build & 0xF);
		}
	}

	return ret;
#endif	/* GTAXY16000 || RTAX89U */
#elif defined(RTCONFIG_SOC_IPQ40XX)
//TBD
	return 0;
#elif defined(RTCONFIG_SOC_IPQ60XX) || defined(RTCONFIG_SOC_IPQ50XX)
	return 1;
#elif defined(RTCONFIG_SWITCH_QCA8386)
#if defined(BD4D5) || defined(BD4_OD)
	int i;
	char iface[IFNAMSIZ];
	for (i = 0; i < 2; i++) {
		snprintf(iface, sizeof(iface), "eth%d", i);
		if (!iface_exist(iface)) {
			dbg("%s: eth%d not found!\n", __func__, i);
			return 0;
		}
	}
	return 1;
#else	
	static const uint32_t phy5_ids[] = { 0x1a241cc7, 0 };	/* two or more PHY ID are accepted, end with 0. */
	static const uint32_t phy6_ids[] = { 0x1a281ac7, 0 };
	static const uint32_t qca8386_ids[] = { 0x004dd180, 0 };
	static const struct phy_id_list_s {
		int phy;
		const uint32_t *pval;
		uint32_t mask;
	} phy_id_lists[] = {
#if defined(TUFBE6500) || defined(TUFBE9400) \
 || defined(RTBE50) || defined(RT4GBE58)
		{  5,    phy5_ids, 0xffffffff },
		{  6,    phy6_ids, 0xffffffff },
		{  4, qca8386_ids, 0xffffffff },	/* QCA8386: WAN */
		{  3, qca8386_ids, 0xffffffff },	/* QCA8386: LAN1 */
		{  2, qca8386_ids, 0xffffffff },	/* QCA8386: LAN2 */
		{  1, qca8386_ids, 0xffffffff },	/* QCA8386: LAN3 */
#endif
		{ -1, NULL, 0 },
	}, *p;
	static const struct gmac_spd_s {
		int port;			/* IPQ53XX GMAC port. */
		int min_speed;			/* minimal speed of the GMAC. */
	} gmac_spd[] = {
		{ 1, 2500 },			/* GMAC1 <=> QCA8386 (UniPHY1) */
		{ -1, -1},
	}, *q;
	uint32_t r, id1, id2;
	const uint32_t *pval;
	unsigned int id_reg_addr;
	int i, r1, ret = 1, val, retry, right;
	char iface[IFNAMSIZ];
	char speed_cmd[sizeof("ssdk_sh swX port speed get XYYYYYY")];

	/* Check all switch/PHY ports */
	for (p = &phy_id_lists[0]; p->phy >= 0; ++p) {
		id_reg_addr = 2;
		if ((id1 = read_phy_reg(p->phy, id_reg_addr)) < 0)
			id1 = 0;
		if ((id2 = read_phy_reg(p->phy, id_reg_addr + 1)) < 0)
			id2 = 0;
		r = ((id1 & 0xFFFF) << 16) | (id2 & 0xFFFF);
		for (right = 0, pval = p->pval; !right && *pval != 0; ++pval) {
			if ((r & p->mask) != (*pval & p->mask))
				continue;
			right++;
		}

		if (!right) {
			ret = 0;
			dbg("%s: PHY %d wrong ID: %08x\n", __func__, p->phy, r);
		}
	}

	/* Check all ethX interfaces. */
	for (i = 0; i < 1; ++i) {
		snprintf(iface, sizeof(iface), "eth%d", i);
		if (!iface_exist(iface)) {
			dbg("%s: eth%d not found!\n", __func__, i);
			ret = 0;
		}
	}

	if (!ret)
		return ret;

	/* Check GMAC that are connected to QCA8386 switch, link-speed should be 2.5Gbps.
	 * For GMAC port that is connected to WAN, link-speed is negotiated at run-time, don't check it.
	 */
	for (q = &gmac_spd[0]; q->port > 0; ++q) {
		retry = 20;
		snprintf(speed_cmd, sizeof(speed_cmd), "ssdk_sh %s port speed get %d", SWID_IPQ53XX, q->port);
		while (retry-- > 0) {
			/* Example:
			 * / # ssdk_sh port speed get 1
			 *
			 *  SSDK Init OK![speed]:2500(Mbps)
			 * operation done.
			 */
			if ((r1 = parse_ssdk_sh(speed_cmd, "%*[^:]:%d", 1, &val)) != 0) {
				dbg("%s: cmd [%s] val [%d], return %d\n", __func__, speed_cmd, val, ret);
				ret = 0;
				break;
			}

			if (val >= q->min_speed)
				break;

			sleep(1);
		}

		if (val < q->min_speed) {
			dbg("IPQ53XX port %d speed %d < %d\n", q->port, val, q->min_speed);
			logmessage("ATE", "IPQ53XX port %d speed %d < %d\n", q->port, val, q->min_speed);
			ret = 0;
		}
	}

	return ret;
#endif	
#else
	FILE *fp;
	char cmd[64], buf[512];
	int rlen;

#ifdef RTCONFIG_QCA8033
	snprintf(cmd, sizeof(cmd), "cat /proc/link_status");
#elif defined(RTCONFIG_QCN550X) && defined(RTCONFIG_SWITCH_QCA8337N)
	snprintf(cmd, sizeof(cmd), "ssdk_sh port linkstatus get 0");
#else
	snprintf(cmd, sizeof(cmd), "swconfig dev %s port 0 get link", MII_IFNAME);
#endif
	if ((fp = popen(cmd, "r")) == NULL) {
		return 0;
	}
	rlen = fread(buf, 1, sizeof(buf), fp);
	pclose(fp);
	if (rlen <= 1)
		return 0;

	buf[rlen-1] = '\0';
#ifdef RTCONFIG_QCA8033
	if (strstr(buf, "link up (1000"))
#elif defined(RTCONFIG_QCN550X) && defined(RTCONFIG_SWITCH_QCA8337N)
	if (strstr(buf, ":ENABLE"))
#else
	if (strstr(buf, "link:up speed:1000"))
#endif
		return 1;
	return 0;
#endif
}

#if !defined(RTCONFIG_QCA_WLAN_SCRIPTS)
static char *get_ref_board_name(void)
{
	char *ret = NULL;
#if defined(RTCONFIG_SOC_IPQ60XX)
	ret = "ap-cp03-c1";
#elif defined(RTCONFIG_SOC_IPQ8074)
	ret = "ap-hk01-c2";
#elif defined(RTCONFIG_SOC_IPQ53XX)
#if defined(RTCONFIG_WIFI_IPQ53XX_QCN64XX)
#if defined(RTBE50) || defined(RT4GBE58) // 8060007
	ret = "ap-mi04.1-c2";
#else // BD4D5, BD4_OD
	ret = "ap-mi01.3-c2";
#endif // RTBE50
#else

	/* board_name:		ap-mi01.2
	 * board_name_prefix:	ap-mi01.2_
	 * board_prefix:	ap-mi
	 */
	ret = "ap-mi01.2";
#endif
#endif

	return ret;
}

int adjust_process_oom_adj(const char *name, int oom_adj)
{
	pid_t *pidList, *pid;
	char oom_adj_str[sizeof("-1000XXX")];
	char oom_score_adj[sizeof("/proc/XXXXX/oom_score_adjXXX")];

	if (!name || *name)
		return -1;

	if (oom_adj < -1000)
		oom_adj = -1000;
	if (oom_adj > 1000)
		oom_adj = 1000;

	if (!pids((char*) name))
		return 0;

	snprintf(oom_adj_str, sizeof(oom_adj_str), "%d", oom_adj);
	pidList = find_pid_by_name(name);
	for (pid = pidList; *pid; pid++) {
		snprintf(oom_score_adj, sizeof(oom_score_adj), "/proc/%d/oom_score_adj", *pid);
		f_write_string(oom_score_adj, oom_adj_str, 0, 0);
	}
	free(pidList);

	return 0;
}

char *get_fw_ini_file(void)
{
	int total_mem __attribute__((unused)) = get_meminfo_item("MemTotal");
	char *fw_ini_file = NULL;
#if defined(RTCONFIG_SOC_IPQ8074)
        const int soc_ver = get_soc_version_major();
#endif

#if defined(RTCONFIG_SOC_IPQ60XX)
	fw_ini_file = "/lib/firmware/IPQ6018/firmware_rdp_feature.ini";
#elif defined(RTCONFIG_SOC_IPQ8074)
#if (SPF_VER >= SPF_VER_ID(11,3))
	if (total_mem <= (512 * 1024)) {
		/* 256MB profile will use the same file as 512MB profile,
		 * but coldboot calibration support will be skipped.
		 */
		if (soc_ver == 2)
			fw_ini_file = "/lib/firmware/IPQ8074A/firmware_rdp_feature_512P.ini";
		else if (soc_ver == 1)
			fw_ini_file = "/lib/firmware/IPQ8074/firmware_rdp_feature_512P.ini";
	} else {
		if (soc_ver == 2)
			fw_ini_file = "/lib/firmware/IPQ8074A/firmware_rdp_feature.ini";
		else if (soc_ver == 1)
			fw_ini_file = "/lib/firmware/IPQ8074/firmware_rdp_feature.ini";
	}
#else
	if (soc_ver == 2)
		fw_ini_file = "/lib/firmware/IPQ8074A/firmware_rdp_feature.ini";
	else if (soc_ver == 1)
		fw_ini_file = "/lib/firmware/IPQ8074/firmware_rdp_feature.ini";
#endif	/* SPF11.3+ */
#elif defined(RTCONFIG_SOC_IPQ53XX)
	/* On AP-MI01.2 ref. board, /lib/firmware/firmware_rdp_feature.ini = 
	 * /lib/firmware/IPQ5332/WIFI_FW/qcn9224/firmware_rdp_feature.ini
	 */
	if (total_mem <= (512 * 1024)) {
		/* 256MB profile will use the same file as 512MB profile,
		 * but coldboot calibration support will be skipped.
		 */
#if defined(RTCONFIG_WIFI_IPQ53XX_QCN64XX)
		fw_ini_file = "/lib/firmware/IPQ5332/firmware_rdp_feature_512P.ini";
#else
		fw_ini_file = "/lib/firmware/qcn9224/firmware_rdp_feature_512P.ini";
#endif
	} else {
#if defined(RTCONFIG_WIFI_IPQ53XX_QCN64XX)
		fw_ini_file = "/lib/firmware/IPQ5332/firmware_rdp_feature.ini";
#else
		fw_ini_file = "/lib/firmware/qcn9224/firmware_rdp_feature.ini";
#endif
	}
#endif	/* RTCONFIG_SOC_IPQ60XX */

	return fw_ini_file;
}

#if defined(RTCONFIG_SOC_IPQ53XX) && (SPF_VER >= SPF_VER_ID(12,2))
/* Implement update_global_daemon_coldboot_qdss_support_variables() of qcawificfg80211.sh
 * based on IPQ53xx SPF12.2 CSU1 QSDK
 * @cold_boot:
 * @return:
 */
int update_global_daemon_coldboot_qdss_support_variables(int *cold_boot)
{
	/* g_cold_boot_support is not used by another functions in qcawificfg80211.sh
	 * Skip the implementation of update_global_daemon_coldboot_qdss_support_variables()
	 */
	return 0;
}
#elif (SPF_VER >= SPF_VER_ID(11,0))
/* SPF11.3, SPF11.4 update_global_daemon_coldboot_qdss_support_variables(). */
static int update_daemon_coldboot_qdss_support_variables(int *cold_boot, int *daemon)
{
	const char *board_name = get_ref_board_name();
	const char *fw_ini_file = get_fw_ini_file();
	int total_mem = get_meminfo_item("MemTotal");
	char val[4] = { 0 };

	if (!cold_boot || !daemon)
		return -1;

#if (SPF_VER <= SPF_VER_ID(11,1))
	/* Low Mem 256 profile does not support these */
	if (total_mem <= (256 * 1024))
		return -2;
#endif

	if (!get_board_or_default_parameter_from_ini_file(board_name, "enable_cold_boot_support", val, sizeof(val), fw_ini_file)) {
		if (*val != '\0')
			*cold_boot = safe_atoi(val);
	}
	if (!get_board_or_default_parameter_from_ini_file(board_name, "enable_daemon_support", val, sizeof(val), fw_ini_file)) {
		if (*val != '\0')
			*daemon = safe_atoi(val);
	}

#if (SPF_VER >= SPF_VER_ID(11,3))
	/* Force disable Coldboot Calibration for 256MB profile.
	 * Daemon support and QDSS are only supported.
	 */
	if (total_mem <= (256 * 1024))
		*cold_boot = 0;
#endif

	return 0;
}
#elif defined(RTCONFIG_SPF10_QSDK)
static int update_daemon_coldboot_qdss_support_variables(int *cold_boot, int *daemon)
{
	char val[4] = { 0 };
	char internal_ini_fn[sizeof(GLOBAL_INI_TOPDIR "/ini/internal/QCA8074V2_i.iniXXXXXX")] = { 0 };

	if (!cold_boot || !daemon)
		return -1;

	/* SPF10 ES QSDK */
	get_internal_ini_filename(internal_ini_fn, sizeof(internal_ini_fn));
	if (!get_parameter_from_ini_file("enable_cold_boot_support", val, sizeof(val), internal_ini_fn)) {
		if (*val != '\0')
			*cold_boot = safe_atoi(val);
	}
	if (!get_parameter_from_ini_file("enable_daemon_support", val, sizeof(val), internal_ini_fn)) {
		if (*val != '\0')
			*daemon = safe_atoi(val);
	}

	return 0;
}
#elif defined(RTCONFIG_SPF8_QSDK)
static int update_daemon_coldboot_qdss_support_variables(int *cold_boot, int *daemon)
{
	char val[4] = {0};
	/* Load cnssdaemon if daemon_support of cnss2 driver is enabled.
	 * daemon_support=Y or N
	 */
	if (!daemon)
		return -1;

	if (f_read_string("/sys/module/cnss2/parameters/daemon_support", val, sizeof(val)) <= 0)
		return -2;
	if (*val != '\0')
		*daemon = (!strcmp(val, "N"))? 0 : 1;

	return 0;
}
#endif	/* SPF11.0+ */

#if defined(RTCONFIG_SOC_IPQ53XX)
/* update_daemon_cold_boot_support_to_plat_priv() is replaced by another function. */
#elif (SPF_VER >= SPF_VER_ID(11,3))
static void update_daemon_cold_boot_support_to_plat_priv(int cold_boot, int daemon)
{
	int total_mem = get_meminfo_item("MemTotal");

	if (daemon == 1) {
		eval("cnsscli", "-i", "integrated", "--enable_daemon_support", "1");
	}

	/* 256MB profile does not support Coldboot Calibration, return here */
	if (total_mem <= (256 * 1024))
		return;

	if (cold_boot == 1) {
		eval("cnsscli", "-i", "integrated", "--enable_cold_boot_support", "1");
	}

	return;
}
#else
static inline void update_daemon_cold_boot_support_to_plat_priv(int cold_boot, int daemon) { }
#endif	/* RTCONFIG_SOC_IPQ53XX */

#if defined(RTCONFIG_SOC_IPQ53XX)
#if defined(TUFBE6500) || defined(TUFBE9400)
static int skip_absent_slot(const char *slot)
{
	int ret = 0;

	if (!slot)
		return 0;

	/* Use same machid of MI01.2 and pci0/pci2 is not wired to any device.
	 * Avoid below warning:
	 * cnss: INFO: cnss_config_param_update_cb: Instance ID: 0x37 Param 3 Value: 0
	 * cnss: ERR: Failed to get plat_priv for instance_id 0x37
	 */
	if (!strcmp(slot, "pci0") || !strcmp(slot, "pci2"))
		ret = 1;

	return ret;
}
#else
static inline int skip_absent_slot(const char *slot) { return 0; }
#endif

/* Reference to IPQ53XX SPF12.2 CSU1 */
/* Implementation do_cold_boot_calibration_qcawificfg80211() and
 * update_platform_features_support_to_plat_priv()
 * @mod:	module name that can handle cold boot calibration.
 * @return:
 * 	-1:	invalid parameter.
 * 	-2:	can't judge daemon_support of cnss2 driver is enabled or not.
 * 	-3:	invalid board_name or fw_ini_file.
 */
static int do_cold_boot_calibration(char *mod, int is_ftm)
{
#define OOM_SCORE_ADJ	"-1000"
	const char **p, *target_process[] = { "cnssdaemon",
#if SPF_VER >= SPF_VER_ID(12,2) && SPF_VER < SPF_VER_ID(12,5)
		"hostapd",
		"wpa_supplicant",
#endif
		NULL
	};
	const char **f, *g_platform_features[] = { "enable_cold_boot_support", "enable_hds_support",
		"enable_regdb_support", "enable_qdss_tracing", NULL };
	const char **t, *g_pci_targets[] = { "qcn9000", "qcn6122", "qcn9224", "qcn9160", "qcn6432", NULL };
	const char **s, *g_pci_slots[] = { "pci0", "pci1", "pci2", "pci3", NULL };
	const char *board_name = get_ref_board_name();
	const char *fw_ini_file = get_fw_ini_file();
	char name[128], val[4] = {0}, cnss_target[sizeof("qcn9224_pci0XXXXXX")], cnss_feature[64];
	char *cnss_set_feature_argv[] = { "cnsscli", "-i", cnss_target, cnss_feature, val, NULL };
	int total_mem = get_meminfo_item("MemTotal");

	if (!mod || *mod == '\0')
		return -1;

	if (!board_name || !fw_ini_file) {
		dbg("%s: Invalid board_name [%s] or fw_ini_file [%s]\n", __func__,
			board_name? : "NULL", fw_ini_file? : "NULL");
		return -3;
	}

	update_global_daemon_coldboot_qdss_support_variables(NULL);
	/* Set cnss-daemon OOM score to -1000 to prevent it from getting killed
	 * Set hostapd OOM score to -1000 to prevent it from getting killed
	 * Set supplicant OOM score to -1000 to prevent it from getting killed
	 */
	for (p = &target_process[0]; p && *p; p++) {
		adjust_process_oom_adj(*p, -1000);
	}

	/* update_platform_features_support_to_plat_priv() */
	/* Search BOARD-NAME_PCI-TARGET_PCI-SLOTS_SETTING-NAME=VALUE, e.g.
	 * ap-mi01.2_qcn9224_pci1_enable_daemon_support=1
	 */
	for (f = &g_platform_features[0]; f && *f; ++f) {
		if (total_mem <= (256 * 1024)
		 && !strcmp(*f, "enable_cold_boot_support"))
			continue;

		snprintf(cnss_feature, sizeof(cnss_feature), "--%s", *f);
		/* Update for PCIe Radios */
		for (t = &g_pci_targets[0]; t && *t; ++t) {
			for (s = &g_pci_slots[0]; s && *s; ++s) {
				snprintf(name, sizeof(name), "%s_%s_%s", *t, *s, *f);

				if (skip_absent_slot(*s))
					continue;
				if (!get_board_or_default_parameter_from_ini_file(board_name,
					name, val, sizeof(val), fw_ini_file))
				{
					if (*val == '\0') {
						continue;
					}
					snprintf(cnss_target, sizeof(cnss_target), "%s_%s", *t, *s);
					_eval(cnss_set_feature_argv, DBGOUT, 0, NULL);
					prn_args(cnss_set_feature_argv);
					if (safe_atoi(val) == 1) {
						/* Update for Integrated Radio */
						strlcpy(cnss_target, "integrated", sizeof(cnss_target));
						_eval(cnss_set_feature_argv, DBGOUT, 0, NULL);
						prn_args(cnss_set_feature_argv);
					}
				}
			}
		}
		/* Skip enable_qdss_tracing. */
	}

	return 0;
}
#else	/* !RTCONFIG_SOC_IPQ53XX) */
/* IPQ8074 or models that use external wifi scripts. */
#if (defined(RTCONFIG_SOC_IPQ8074) && (SPF_VER >= SPF_VER_ID(11,4)))
/* Implementation cold boot calibration.
 * @mod:	module name that can handle cold boot calibration.
 * @return:
 * 	-1:	invalid parameter.
 * 	-2:	can't judge daemon_support of cnss2 driver is enabled or not.
 */
static int do_cold_boot_calibration(char *mod, int is_ftm)
{
#define CNSSDAEMON_OOM_SCORE_ADJ	"-1000"				/* SPF10 */
	char val[4] = {0};
	char testmode[] = "7", ftm_testmode[] = "10";
	char testmode_param[] = "testmode=7", ftm_testmode_param[] = "testmode=10";
	int cold_boot = 0, daemon = 0;

	if (!mod || *mod == '\0')
		return -1;

	update_daemon_coldboot_qdss_support_variables(&cold_boot, &daemon);

	strlcpy(val, cold_boot? "1" : "0", sizeof(val));
	strlcpy(val, daemon? "1" : "0", sizeof(val));
	/* SPF11.4 ~ SPF11.5: update_daemon_cold_boot_support_to_plat_priv()
	 * SPF11.5: update_platform_features_support_to_plat_priv()
	 * is incharge of enabling cold_boot/daemon and must be executed after
	 * cnssdaemon started.
	 */

	if (daemon) {
		/* /etc/init.d/qrtr */
		if (!pids("qrtr-ns")) {
			eval("qrtr-cfg", "1");
			eval("qrtr-ns");
		}

		/* start_cnssdaemon() */
		/* Daemon User Socket uses loopback interface, make sure it is up */
		_ifconfig("lo", IFUP, NULL, NULL, NULL, 0);;

		eval("mkdir", "-p", "/tmp/wifi");	/* /data/vendor/wifi ==> /tmp/wifi */
		if (!pids("cnssdaemon")) {
			pid_t *pidList, *p;
			char oom_score_adj[sizeof("/proc/XXXXX/oom_score_adjXXX")];
			char *cnssdaemon_argv[] = { "cnssdaemon"
				, "-s"
				, "-i", "integraded"
				, "-dddd", "-f", "/tmp/cnssdaemon.log"
				, NULL };

			_eval(cnssdaemon_argv, NULL, 0, NULL);
			/* cnssdaemon opens up sockets which is required for cnsscli.
			 * sleep here is to ensure the sockets are open before cnsscli is used.
			 */
			sleep(1);
			pidList = find_pid_by_name("cnssdaemon");
			for (p = pidList; *p; p++) {
				snprintf(oom_score_adj, sizeof(oom_score_adj), "/proc/%d/oom_score_adj", *p);
				f_write_string(oom_score_adj, CNSSDAEMON_OOM_SCORE_ADJ, 0, 0);
			}
			free(pidList);
		}

		update_daemon_cold_boot_support_to_plat_priv(cold_boot, daemon);
	}	/* daemon */

	if (!cold_boot) {
		dbg("No cold_boot_support\b");
		return 0;
	}

	if (f_exists(WLFW_CAL_01_BIN))
		return 0;

	if (module_loaded(mod))
		return -3;

	/* Set driver_mode as coldboot to the kernel */
	f_write_string("/sys/module/cnss2/parameters/driver_mode", is_ftm? ftm_testmode : testmode, 0, 0);

	/* Set Cold boot mode to 10 for FTM Mode, 7 otherwise */
	eval("modprobe", "-s", mod, is_ftm? ftm_testmode_param : testmode_param);

	if (!module_loaded(mod))
		return -4;
	eval("modprobe", "-s", "wifi_3_0");
	eval("rmmod", "wifi_3_0");
	eval("rmmod",  mod);

	/* Reset driver_mode to 0 for mission mode */
	f_write_string("/sys/module/cnss2/parameters/driver_mode", "0", 0, 0);

	if (module_loaded(mod))
		return -5;

	return 0;
}
#elif (SPF_VER >= SPF_VER_ID(8,0)) && (SPF_VER <= SPF_VER_ID(11,3))
/* Implementation cold boot calibration.
 * @mod:	module name that can handle cold boot calibration.
 * @return:
 * 	-1:	invalid parameter.
 * 	-2:	can't judge daemon_support of cnss2 driver is enabled or not.
 */
static int do_cold_boot_calibration(char *mod, int is_ftm)
{
#if defined(RTCONFIG_GLOBAL_INI)
#define CNSSDAEMON_OOM_SCORE_ADJ	"-1000"				/* SPF10 */
	char val[4] = {0};
#else
#endif
#if (SPF_VER >= SPF_VER_ID(11,0)) && (SPF_VER <= SPF_VER_ID(11,3))
	char testmode[] __attribute__((unused))= "7", ftm_testmode[] __attribute__((unused)) = "10";
	char testmode_param[] = "testmode=7", ftm_testmode_param[] = "testmode=10";
#endif
	int cold_boot = 0, daemon = 0;

	if (!mod || *mod == '\0')
		return -1;

#if (SPF_VER >= SPF_VER_ID(8,0)) && (SPF_VER <= SPF_VER_ID(11,3))
	update_daemon_coldboot_qdss_support_variables(&cold_boot, &daemon);
#endif

#if defined(RTCONFIG_SPF11_3_QSDK)
	if (!cold_boot) {
		dbg("No cold_boot_support\b");
		return 0;
	}

	strlcpy(val, cold_boot? "1" : "0", sizeof(val));
	strlcpy(val, daemon? "1" : "0", sizeof(val));
	/* update_daemon_cold_boot_support_to_plat_priv() is incharge of enabling
	 * cold_boot/daemon and must be executed after cnssdaemon started.
	 */
#elif defined(RTCONFIG_SPF11_1_QSDK)
	if (!cold_boot) {
		dbg("No cold_boot_support\b");
		return 0;
	}

	strlcpy(val, cold_boot? "1" : "0", sizeof(val));
	if (f_write_string(CNSS2_SYSFS_COLDBOOT, val, 0, 0) <= 0)
		return -2;

	strlcpy(val, daemon? "1" : "0", sizeof(val));
	if (f_write_string(CNSS2_SYSFS_DAEMON, val, 0, 0) <= 0)
		return -2;
#elif defined(RTCONFIG_SPF11_QSDK)
	if (!cold_boot) {
		dbg("No cold_boot_support\b");
		return 0;
	}

	strlcpy(val, cold_boot? "1" : "0", sizeof(val));
	if (f_write_string(CNSS2_SYSFS_COLDBOOT, val, 0, 0) <= 0)
		return -2;

	strlcpy(val, daemon? "Y" : "N", sizeof(val));
	if (f_write_string(CNSS2_SYSFS_DAEMON, val, 0, 0) <= 0)
		return -2;
#elif defined(RTCONFIG_SPF10_QSDK)
	strlcpy(val, cold_boot? "1" : "0", sizeof(val));
	if (f_write_string(CNSS2_SYSFS_COLDBOOT, val, 0, 0) <= 0)
		return -2;

	strlcpy(val, daemon? "Y" : "N", sizeof(val));
	if (f_write_string(CNSS2_SYSFS_DAEMON, val, 0, 0) <= 0)
		return -2;

	if (!cold_boot) {
		dbg("No cold_boot_support\b");
		return 0;
	}
#elif defined(RTCONFIG_SPF8_QSDK)
	/* No need to update sysfs attribute of cnss2. */
#endif

	if (!daemon)
		return 0;

#if ((SPF_VER >= SPF_VER_ID(11,1)) && (SPF_VER <= SPF_VER_ID(11,3))) \
 && defined(RTCONFIG_SOC_IPQ8074)
	/* /etc/init.d/qrtr */
	if (!pids("qrtr-ns")) {
		eval("qrtr-cfg", "1");
		eval("qrtr-ns");
	}
#endif

	/* start_cnssdaemon() */
#if defined(RTCONFIG_SPF11_3_QSDK)
	/* Daemon User Socket uses loopback interface, make sure it is up */
	_ifconfig("lo", IFUP, NULL, NULL, NULL, 0);;
#endif

	eval("mkdir", "-p", "/tmp/wifi");	/* /data/vendor/wifi ==> /tmp/wifi */
	if (!pids("cnssdaemon")) {
#if defined(RTCONFIG_GLOBAL_INI)
		pid_t *pidList, *p;
		char oom_score_adj[sizeof("/proc/XXXXX/oom_score_adjXXX")];
		char *cnssdaemon_argv[] = { "cnssdaemon",
#if defined(RTCONFIG_SPF11_3_QSDK)
			"-i", "integraded", /* "-dddd", "-f", "/tmp/cnssdaemon.log", */
#endif
			NULL };

		_eval(cnssdaemon_argv, NULL, 0, NULL);
		/* cnssdaemon opens up sockets which is required for cnsscli.
		 * sleep here is to ensure the sockets are open before cnsscli is used.
		 */
		sleep(1);
		pidList = find_pid_by_name("cnssdaemon");
		for (p = pidList; *p; p++) {
			snprintf(oom_score_adj, sizeof(oom_score_adj), "/proc/%d/oom_score_adj", *p);
			f_write_string(oom_score_adj, CNSSDAEMON_OOM_SCORE_ADJ, 0, 0);
		}
		free(pidList);
#else	/* !RTCONFIG_GLOBAL_INI */
		eval("cnssdaemon");
#endif	/* RTCONFIG_GLOBAL_INI */
	}

	update_daemon_cold_boot_support_to_plat_priv(cold_boot, daemon);

	if (f_exists(WLFW_CAL_01_BIN))
		return 0;

	if (module_loaded(mod))
		return -3;

#if defined(RTCONFIG_SPF11_3_QSDK)
	/* Set driver_mode as coldboot to the kernel */
	f_write_string("/sys/module/cnss2/parameters/driver_mode", is_ftm? ftm_testmode : testmode, 0, 0);
#endif

#if ((SPF_VER >= SPF_VER_ID(11,0)) && (SPF_VER <= SPF_VER_ID(11,3)))
	/* Set Cold boot mode to 10 for FTM Mode, 7 otherwise */
	eval("modprobe", "-s", mod, is_ftm? ftm_testmode_param : testmode_param);
#elif defined(RTCONFIG_SPF10_QSDK)
	eval("modprobe", "-s", mod, "testmode=7");
#endif

	if (!module_loaded(mod))
		return -4;
#if defined(RTCONFIG_GLOBAL_INI)
	eval("modprobe", "-s", "wifi_3_0");
	eval("rmmod", "wifi_3_0");
#endif
	eval("rmmod",  mod);

#if defined(RTCONFIG_SPF11_3_QSDK)
	/* Reset driver_mode to 0 for mission mode */
	f_write_string("/sys/module/cnss2/parameters/driver_mode", "0", 0, 0);
#endif

	if (module_loaded(mod))
		return -5;

	return 0;
}
#endif	/* RTCONFIG_SOC_IPQ8074 && SPF11.4) */
#endif	/* RTCONFIG_SOC_IPQ53XX */

#if defined(RTCONFIG_GLOBAL_INI)
#if defined(RTCONFIG_SOC_IPQ8074) && (defined(RTCONFIG_SPF11_4_QSDK) || defined(RTCONFIG_SPF11_5_QSDK))
static void update_ini_dp_rings_sbs_for_nss_offload()
{
	char *dp_rings_sbs[] = {
		"dp_nss_tcl_data_rings=3",
		"dp_nss_reo_dest_rings=3",
		NULL
	};

	__update_ini_file(QCA8074V2_I_INI, dp_rings_sbs);
}

static void update_ini_dp_rings_dbs_for_nss_offload()
{
	char *dp_rings_dbs[] = {
		"dp_nss_tcl_data_rings=2",
		"dp_nss_reo_dest_rings=2",
		NULL
	};

	__update_ini_file(QCA8074_I_INI, dp_rings_dbs);
	__update_ini_file(QCA8074V2_I_INI, dp_rings_dbs);
}

static void update_ini_dp_rings_for_nss_mode(void)
{
	int dynamic_hw_mode = 0, num_radio = 2;
	char val[4] = { 0 }, hw_mode_str[64] = { 0 };

	if (get_soc_version_major() != 2)
		return;

	if (!get_parameter_from_ini_file("dynamic_hw_mode", val, sizeof(val), GLOBAL_I_INI)) {
		if (*val != '\0')
			dynamic_hw_mode = safe_atoi(val);
	}

	if (f_read_string(SYS_CLASS_NET "/soc0/hw_modes", hw_mode_str, sizeof(hw_mode_str)) > 0) {
		if (strstr(hw_mode_str, "DBS_SBS:"))
			num_radio = 2;
	}

	if (num_radio > 2 || dynamic_hw_mode) {
		update_ini_dp_rings_sbs_for_nss_offload();
	} else {
		update_ini_dp_rings_dbs_for_nss_offload();
	}
}
#else
static inline void update_ini_dp_rings_for_nss_mode(void) { }
#endif	/* RTCONFIG_SOC_IPQ8074 || (RTCONFIG_SPF11_4_QSDK || RTCONFIG_SPF11_5_QSDK) */

#if defined(RTCONFIG_SOC_IPQ60XX) || defined(RTCONFIG_SOC_IPQ8074)
static void update_ini_nss_info(int hk_ol_num)
{
	int total_mem, ring_size;
	char str[sizeof("dp_nss_comp_ring_size=0xFFFFFFFFXXX")];
	char *nss_info[] = { str, NULL };

	total_mem = get_meminfo_item("MemTotal");
	if (total_mem <= 0 || hk_ol_num < 0 || hk_ol_num > 3 || get_soc_version_major() != 2)
		return;

	if (total_mem <= (256 * 1024)) {
		ring_size = 0x200;
	} else if (total_mem <= (512 * 1024)) {
		if (hk_ol_num == 3)
			ring_size = 0x3000;
		else
			ring_size = 0x4000;
	} else {
		if (hk_ol_num == 3)
			ring_size = 0x8000;
		else
			ring_size = 0x8000;
	}
	snprintf(str, sizeof(str), "dp_nss_comp_ring_size=0x%x", ring_size);
#if defined(RTCONFIG_SOC_IPQ60XX)
	__update_ini_file(QCA6018_I_INI, nss_info);
#elif defined(RTCONFIG_SOC_IPQ8074)
	__update_ini_file(QCA8074V2_I_INI, nss_info);
#endif
}
#else
static inline void update_ini_nss_info(int hk_ol_num) { }
#endif

#if defined(RTCONFIG_WIFI_QCN5024_QCN5054)
/* Adjust ring buffer related settings in .ini file
 * @total_mem: Total Memory size, unit: KB
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
#if (SPF_VER >= SPF_VER_ID(11,0))
static int adjust_ring_buffer_in_ini(int total_mem)
{
	/* detect_qcawifi() */
	if (total_mem <= (256 * 1024)) {
		/* update_ini_for_lowmem() */
		char *nr_dp_rings[] = {
			"dp_rxdma_monitor_buf_ring=128",
			"dp_rxdma_monitor_dst_ring=128",
			"dp_rxdma_monitor_desc_ring=128",
			"dp_rxdma_monitor_status_ring=512",
#if (SPF_VER >= SPF_VER_ID(11,1))
			"num_vdevs_pdev0=9",
			"num_vdevs_pdev1=9",
			"num_vdevs_pdev2=9",
			"num_peers_pdev0=128",
			"num_peers_pdev1=128",
			"num_peers_pdev2=128",
			"num_monitor_pdev0=0",
			"num_monitor_pdev1=0",
			"num_monitor_pdev2=0",
#endif
#if (SPF_VER >= SPF_VER_ID(11,3))
			"full_mon_mode=0",
#endif
			NULL
		};

#if defined(RTCONFIG_SOC_IPQ60XX)
		__update_ini_file(QCA6018_I_INI, nr_dp_rings);
#else
		__update_ini_file(QCA8074_I_INI, nr_dp_rings);
		__update_ini_file(QCA8074V2_I_INI, nr_dp_rings);
#endif
	} else {
		/* update_ini_for_monitor_buf_ring() */
		char *dp_rxdma_monitor[] = {
			"dp_rxdma_monitor_buf_ring=4096",
			"dp_rxdma_monitor_dst_ring=4096",
			"dp_rxdma_monitor_desc_ring40960",
			"dp_rxdma_monitor_status_ring=512",
			NULL
		};

		if (0 /* && num_chain <= 4 */) {
#if defined(RTCONFIG_SOC_IPQ60XX)
			__update_ini_file(QCA6018_I_INI, dp_rxdma_monitor);
#else
			__update_ini_file(QCA8074_I_INI, dp_rxdma_monitor);
			__update_ini_file(QCA8074V2_I_INI, dp_rxdma_monitor);
#endif
		}
	}

#if defined(RTCONFIG_SOC_IPQ60XX)
	if (1) {
		char *dp_tx_desc[] = {
			"dp_tx_desc=0x8000",
			NULL
		};
		char *dp_rxdma_monitor[] = {
			"dp_rxdma_monitor_buf_ring=4096",
			"dp_rxdma_monitor_dst_ring=4096",
			"dp_rxdma_monitor_desc_ring=4096",
			"dp_rxdma_monitor_status_ring=1024",
			NULL
		};
		__update_ini_file(QCA6018_I_INI, dp_tx_desc);
		__update_ini_file(QCA6018_I_INI, dp_rxdma_monitor);
	}
#else // @RTCONFIG_SOC_IPQ60XX
	if (total_mem > (256 * 1024) && total_mem <= (512 * 1024)) {
		/* update_ini_for_512MP() */
		char *dp_tx_desc[] = {
			/* update_ini_for_512MP_dp_tx_desc() */
			"dp_tx_desc=0x4000",
			"dp_tx_desc_limit_0=8192",
			"dp_tx_desc_limit_1=8192",
			"dp_tx_desc_limit_2=8192",
			NULL
		};
		/* update_ini_for_512MP_P_build() */
		char *dp_rxdma_monitor[] = {
			"dp_rxdma_monitor_buf_ring=128",
			"dp_rxdma_monitor_dst_ring=128",
			"dp_rxdma_monitor_desc_ring=4096",
			"dp_rxdma_monitor_status_ring=512",
#if (SPF_VER >= SPF_VER_ID(11,1))
			"num_vdevs_pdev0=9",
			"num_vdevs_pdev1=9",
			"num_vdevs_pdev2=9",
			"num_peers_pdev0=128",
			"num_peers_pdev1=128",
			"num_peers_pdev2=128",
			"num_monitor_pdev0=0",
			"num_monitor_pdev1=0",
			"num_monitor_pdev2=0",
#endif
			NULL
		};
		__update_ini_file(QCA8074_I_INI, dp_tx_desc);
		__update_ini_file(QCA8074V2_I_INI, dp_tx_desc);
		if (1 /* || is_e_build != 1 */) {
			__update_ini_file(QCA8074_I_INI, dp_rxdma_monitor);
			__update_ini_file(QCA8074V2_I_INI, dp_rxdma_monitor);
		}
	}
#endif // @RTCONFIG_SOC_IPQ60XX

	return 0;
}
#elif defined(RTCONFIG_SPF10_QSDK)
static int adjust_ring_buffer_in_ini(int total_mem)
{
	/* detect_qcawifi() */
	if (total_mem <= (256 * 1024)) {
		/* update_ini_for_lowmem() */
		char *nr_dp_rings[] = {
			"dp_rxdma_monitor_buf_ring=128",
			"dp_rxdma_monitor_dst_ring=128",
			"dp_rxdma_monitor_desc_ring=128",
			"dp_rxdma_monitor_status_ring=512",
			"enable_daemon_support=0",
			"enable_cold_boot_support=0",
			NULL
		};

		__update_ini_file(QCA8074_I_INI, nr_dp_rings);
		__update_ini_file(QCA8074V2_I_INI, nr_dp_rings);
	} else if (total_mem <= (512 * 1024)) {
		/* update_ini_for_512MP() */
		char *dp_tx_desc[] = {
			"dp_tx_desc=0x4000",
			"dp_rxdma_monitor_buf_ring=128",
			"dp_rxdma_monitor_dst_ring=128",
			"dp_rxdma_monitor_desc_ring=128",
			"dp_rxdma_monitor_status_ring=512",
			NULL
		};
		__update_ini_file(QCA8074_I_INI, dp_tx_desc);
		__update_ini_file(QCA8074V2_I_INI, dp_tx_desc);
	}

	return 0;
}
#endif	/* RTCONFIG_SPF11_QSDK || RTCONFIG_SPF11_1_QSDK || RTCONFIG_SPF11_3_QSDK || RTCONFIG_SPF11_4_QSDK || RTCONFIG_SPF11_5_QSDK */
#endif	/* RTCONFIG_WIFI_QCN5024_QCN5054 */
#endif	/* RTCONFIG_GLOBAL_INI */

/* Decide extra_pbuf_core0, n2h_high_water_core0, and n2h_wifi_pool_buf.
 * @total_mem:			Total Memory size, unit KB.
 * @olcfg:			WiFi offloading of each band.
 * @extra_pbuf_core0:
 * @n2h_high_water_core0:
 * @n2h_wifi_pool_buf:
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
#if defined(RTCONFIG_SOC_IPQ53XX)
/* No need to implement get_nss_buf_size() due to none of any NSS available on the chip. */
#elif (SPF_VER >= SPF_VER_ID(11,0))
static int get_nss_buf_size(int total_mem, int olcfg, const char **extra_pbuf_core0, const char **n2h_high_water_core0, const char **n2h_wifi_pool_buf)
{
	unsigned int hk_ol_num;
	const int soc_version_major __attribute__((unused)) = get_soc_version_major();

	if ((total_mem < (32 * 1024)) || !extra_pbuf_core0 || !n2h_high_water_core0 || !n2h_wifi_pool_buf)
		return -1;

	hk_ol_num = bitCount(olcfg);
	if (total_mem <= (256 * 1024)) {
		/* total pbuf size is 160 bytes,allocate memory for 8712 pbufs */
		*extra_pbuf_core0 = "1400000";;
		*n2h_high_water_core0 = "20432";
		*n2h_wifi_pool_buf = "0";
	} else if (total_mem <= (512 * 1024)) {
		if (hk_ol_num == 3) {
			/* total pbuf size is 160 bytes,allocate memory for 19928 pbufs */
			*extra_pbuf_core0 = "3200000";
			*n2h_high_water_core0 = "31648";
			*n2h_wifi_pool_buf = "0";
		} else {
			/* total pbuf size is 160 bytes,allocate memory for 18904 pbufs */
			*extra_pbuf_core0 = "3100000";
			*n2h_high_water_core0 = "30624";
			*n2h_wifi_pool_buf = "8192";
		}
	} else{
#if defined(GTAXY16000)
		/* ap-hk09 */
		if (soc_version_major == 2) {
			/* total pbuf size is 160 bytes,allocate memory for 55672 pbufs */
			*extra_pbuf_core0 = "9000000";
			*n2h_high_water_core0 = "67392";
			/* initially after init 4k buf for 5G and 4k for 2G will be allocated, later range will be configured */
			*n2h_wifi_pool_buf = "40960";
		} else {
			/* total pbuf size is 160 bytes,allocate memory for 57184 pbufs */
			*extra_pbuf_core0 = "9200000";
			*n2h_high_water_core0 = "68904";
			*n2h_wifi_pool_buf = "32768";
		}
#elif defined(RTAX89U)
		char *dp_tx_device_limit[] = { "dp_tx_device_limit=49152", NULL };

		/* ap-hk01 */
		if (hk_ol_num == 3) {
			/* total pbuf size is 160 bytes,allocate memory for 93560 pbufs
			 * NSS general payload(8000),Rx Buffer per radio(4k),Tx queue buffer per radio(1k), intial TX allocation per radio(4k)
			 * Below table is Tx desc allocation based on number of clients connected
			 * Radio     Range0   Range1     Range2       Range3
			 *            (<=64) (<=128)     (<=256)      (>256)
			 * 5G-Hi        24k	24k	24k		32k
			 * 2G           16k	16k	16k		16k
			 * 5G-Low       24k	24k	24k		32k
			 * Absolute high water=NSS payloads + Rx buf per radio + Tx queue per radio + TxDescRange3(5g-low/5g-hi/2g)
			 * wifi pool buff = Min(Total tx desc at range 3, device_limit) - total intial tx allocation
			 * extra pbuf core0 = (high_water_core0 - (NSS + OCM buffers)) * pbuf_size
			 * where NSS+OCM buffers = 11720 and pbuf_size = 160
			 * The need for correction of n2h_high_water_core0 from 105280 to 72512 is needed since
			 * this high number can cause OOM
			 */
			*extra_pbuf_core0 = "10000000";
			*n2h_high_water_core0 = "72512";
			/* initially after init 4k buf for 5G and 4k for 2G will be allocated, later range will be configured */
			*n2h_wifi_pool_buf = "36864";
		} else {
			/* total pbuf size is 160 bytes,allocate memory for 55672 pbufs */
			*extra_pbuf_core0 = "9000000";
			*n2h_high_water_core0 = "67392";
			/* initially after init 4k buf for 5G and 4k for 2G will be allocated, later range will be configured */
			*n2h_wifi_pool_buf = "40960";
		}

		/* update_ini_for_hk_dbs QCA8074V2_i.ini */
		__update_ini_file(QCA8074V2_I_INI, dp_tx_device_limit);
#elif defined(RTCONFIG_SOC_IPQ60XX)
		*extra_pbuf_core0 = "3100000";
		*n2h_high_water_core0 = "30528";
		*n2h_wifi_pool_buf = "4096";
#else
#error Define NSS buffer size!
#endif
	}

	return 0;
}
#elif defined(RTCONFIG_SPF10_QSDK)
static int get_nss_buf_size(int total_mem, int olcfg, const char **extra_pbuf_core0, const char **n2h_high_water_core0, const char **n2h_wifi_pool_buf)
{
	unsigned int hk_ol_num;
	const int soc_version_major __attribute__((unused)) = get_soc_version_major();

	if ((total_mem < (32 * 1024)) || !extra_pbuf_core0 || !n2h_high_water_core0 || !n2h_wifi_pool_buf)
		return -1;

	hk_ol_num = bitCount(olcfg);
	if (total_mem <= (256 * 1024)) {
		/* total pbuf size is 160 bytes,allocate memory for 8712 pbufs */
		*extra_pbuf_core0 = "1400000";;
		*n2h_high_water_core0 = "20432";
		*n2h_wifi_pool_buf = "0";
	} else if (total_mem <= (512 * 1024)) {
		if (hk_ol_num == 3) {
			/* total pbuf size is 160 bytes,allocate memory for 28120 pbufs */
			*extra_pbuf_core0 = "4500000";
			*n2h_high_water_core0 = "39840";
			*n2h_wifi_pool_buf = "8192";
		} else {
			/* total pbuf size is 160 bytes,allocate memory for 18904 pbufs */
			*extra_pbuf_core0 = "3100000";
			*n2h_high_water_core0 = "30624";
			*n2h_wifi_pool_buf = "8192";
		}
	} else{
#if defined(GTAXY16000)
		/* ap-hk09 */
		if (soc_version_major == 2) {
			/* total pbuf size is 160 bytes,allocate memory for 55672 pbufs */
			*extra_pbuf_core0 = "9000000";
			*n2h_high_water_core0 = "67392";
			/* initially after init 4k buf for 5G and 4k for 2G will be allocated, later range will be configured */
			*n2h_wifi_pool_buf = "40960";
		} else {
			/* total pbuf size is 160 bytes,allocate memory for 57184 pbufs */
			*extra_pbuf_core0 = "9200000";
			*n2h_high_water_core0 = "68904";
			*n2h_wifi_pool_buf = "32768";
		}
#elif defined(RTAX89U)
		/* ap-hk01 */
		if (hk_ol_num == 3) {
			/* total pbuf size is 160 bytes,allocate memory for 77176 pbufs */
			*extra_pbuf_core0 = "13000000";
			*n2h_high_water_core0 = "88896";
			/* initially after init 4k buf for 5G and 4k for 2G will be allocated, later range will be configured */
			*n2h_wifi_pool_buf = "53248";
		} else {
			/* total pbuf size is 160 bytes,allocate memory for 55672 pbufs */
			*extra_pbuf_core0 = "9000000";
			*n2h_high_water_core0 = "67392";
			/* initially after init 4k buf for 5G and 4k for 2G will be allocated, later range will be configured */
			*n2h_wifi_pool_buf = "40960";
		}
#else
#error Define NSS buffer size!
#endif
	}

	return 0;
}
#elif defined(RTCONFIG_SPF8_QSDK)
static int get_nss_buf_size(int total_mem, int olcfg, const char **extra_pbuf_core0, const char **n2h_high_water_core0, const char **n2h_wifi_pool_buf)
{
	if (!extra_pbuf_core0 || !n2h_high_water_core0 || !n2h_wifi_pool_buf)
		return -1;

#if defined(RTAX89U)
	/* ap-hk*, total pbuf size is 160 bytes,allocate memory for 35288 pbufs */
	*extra_pbuf_core0 = "6000000";
	*n2h_high_water_core0 = "48128";
	*n2h_wifi_pool_buf = "24576";
#else
#error Define NSS buffer size!
#endif

	return 0;
}
#elif defined(RTCONFIG_WIFI_QCA9990_QCA9990) || \
      defined(RTCONFIG_WIFI_QCA9994_QCA9994)
/* IPQ806X, BRT-AC828 */
static int get_nss_buf_size(int total_mem, int olcfg, const char **extra_pbuf_core0, const char **n2h_high_water_core0, const char **n2h_wifi_pool_buf)
{
	unsigned int hk_ol_num;

	if (!extra_pbuf_core0 || !n2h_high_water_core0 || !n2h_wifi_pool_buf)
		return -1;

	hk_ol_num = bitCount(olcfg);
	if (hk_ol_num == 2) {
		*extra_pbuf_core0 = "5939200";
		*n2h_high_water_core0 = "59392";
		*n2h_wifi_pool_buf = "36608";
	} else {
		*extra_pbuf_core0 = "4096000";
		*n2h_high_water_core0 = "43008";
		*n2h_wifi_pool_buf = "20224";
	}

	return 0;
}
#endif	/* RTCONFIG_SOC_IPQ53XX */
#endif	/* !RTCONFIG_QCA_WLAN_SCRIPTS */

#if defined(RTCONFIG_SOC_IPQ53XX)
#define MAGIC_CHECK_LEN	4
#if defined(TUFBE6500)
static unsigned char bdf_magic[2][MAGIC_CHECK_LEN] = { {0x01, 0x00, 0x78, 0x03}, {0x01, 0x00, 0x04, 0x04} };
#elif defined(TUFBE9400)
static unsigned char bdf_magic[3][MAGIC_CHECK_LEN] = {
	{0x01, 0x00, 0x78, 0x03},
	{0x01, 0x00, 0x04, 0x04},
	{0x01, 0x00, 0x04, 0x04}
};
#elif defined(BD4D5) || defined(BD4_OD) || defined(RTBE50) || defined(RT4GBE58)
static unsigned char bdf_magic[2][MAGIC_CHECK_LEN] = { {0x01, 0x00, 0x78, 0x03}, {0x01, 0x00, 0x78, 0x03} };
#else
#error define your caldata file magic!!
#endif

static const char *g_caldata_fn[MAX_NR_WL_IF] = {
	[WL_2G_BAND] = "/lib/firmware/IPQ5332/caldata.bin",
#if defined(TUFBE6500)
	[WL_5G_BAND] = "/lib/firmware/qcn9224/caldata_2.bin",
#if defined(RTCONFIG_HAS_6G)
	[WL_6G_BAND] = "/lib/firmware/qcn9224/caldata_2.bin",
#endif
#elif defined(BD4D5) || defined(BD4_OD) || defined(RTBE50) || defined(RT4GBE58)
	[WL_5G_BAND] = "/lib/firmware/qcn6432/caldata_1.b0060",
#endif
};

int is_valid_caldata(int band)
{
	unsigned char buf[MAGIC_CHECK_LEN];
	int ret, i;

	if (band < 0 || band >= ARRAY_SIZE(g_caldata_fn)) {
		_dprintf("%s: Invalid band (%d)!\n", __func__, band);
		return 0;
	}
	if (!g_caldata_fn[band]) {
		_dprintf("%s: filename of band (%d) caldata is not defined!\n", __func__, band);
		return 0;
	}
	ret = f_read(g_caldata_fn[band], buf, sizeof(buf));
	if (ret != sizeof(buf))
		return 0;
	for ( i = 0; i < sizeof(buf); i++) {
		if (buf[i] != bdf_magic[band][i])
			return 0;
	}
	return 1;
}

static void factory_to_caldata_file(void)
{
	int r;
	void *buf;
	struct factory_to_caldata_s {
		unsigned int offset;	/* bdwlan.XXXX offset in Factory */
		unsigned int length;	/* caldata length, >= size of bdwlan.XXXX,
					 * ref. /lib/read_caldata_to_fs.sh
					 */
		char *fn;		/* /PATH/TO/CALDATA_FILE */
	} *p, factory_to_caldata_tbl[] = {
#if defined(TUFBE6500) || defined(TUFBE9400)
		{ 0x1000, 			IPQ53XX_EEPROM_SIZE,
			"/lib/firmware/IPQ5332/caldata.bin"
		},
		/* read_caldata_to_fs.sh use 184320 instead,
		 * it's neither size of bdwlan.XXXX nor slot_1_size in ftm.conf
		 */
		{ 0x1000 + QCA_2G_EEPROM_SIZE,	184320 /* QCN6274_EEPROM_SIZE */,
			"/lib/firmware/qcn9224/caldata_2.bin"
		},
#elif defined(BD4D5) || defined(BD4_OD) || defined(RTBE50) || defined(RT4GBE58)
		{ 0x1000, 			IPQ53XX_EEPROM_SIZE,
			"/lib/firmware/IPQ5332/caldata.bin"
		},
		/* read_caldata_to_fs.sh use 100352, equal to the size of bdwlan.b0060
		 */
		{ 0x1000 + QCA_2G_EEPROM_SIZE,	QCN64XX_EEPROM_SIZE,
			"/lib/firmware/qcn6432/caldata_1.b0060"
		},
#endif

		{ 0, 0 }
	};

	for (p = &factory_to_caldata_tbl[0]; p->fn && p->length; ++p) {
		buf = malloc(p->length);
		if (!buf) {
			dbg("%s: allocate %u bytes for %s failed!\n", __func__, p->length, p->fn);
			continue;
		}
		r = FactoryRead(buf, p->offset, p->length);
		if (r) {
			dbg("%s: read %u bytes from offset 0x%x in Factory failed, return %d\n",
				__func__, p->length, p->offset, r);
			free(buf);
			continue;
		}
		r = f_write(p->fn, buf, p->length, 0, 0644);
		if (r < 0) {
			dbg("%s: write %u bytes to %s failed, return %d\n",
				__func__, p->length, p->fn, r);
			free(buf);
			continue;
		} else if (r < p->length) {
			dbg("%s: write %d/%u bytes to %s\n", __func__, r, p->length, p->fn);
			free(buf);
			continue;
		}

		free(buf);
	}
}
#else
static inline void factory_to_caldata_file(void) { }
#endif	/* RTCONFIG_SOC_IPQ53XX */

static const int g_band2fid[WL_NR_BANDS] = {
	[WL_2G_BAND] = 2,
	[WL_5G_BAND] = 5,
#if defined(RTCONFIG_HAS_5G_2)
	[WL_5G_2_BAND] = 5,
#endif
#if defined(RTCONFIG_HAS_6G)
	[WL_6G_BAND] = 6,
#if defined(RTCONFIG_HAS_6G_2)
	[WL_6G_2_BAND] = 6,
#endif
#endif
#if defined(RTCONFIG_WIGIG)
	[WL_60G_BAND] = 60,
#endif
};

#if defined(RTCONFIG_SOC_IPQ53XX) && (SPF_VER >= SPF_VER_ID(12,2))
/* ap-al02-c16 example      | ap-mi01.2 example
 * -------------------------+------------------------
 * mlo_max_num_groups=1     | mlo_max_num_groups=1
 *                          |
 * [MLO_GROUP_0]            | [MLO_GROUP_0]
 * mlo_max_num_peers=256    | mlo_max_num_peers=256
 * mlo_num_chips=4          | mlo_num_chips=3
 * mlo_soc_chip_ids=1,2,3,4 | mlo_soc_chip_ids=0,1,2
 *                          |
 * [MLO_SOC_CHIP_1]         | [MLO_SOC_CHIP_0]
 * mlo_chip_idx=0           | mlo_chip_idx=0
 * mlo_num_adj_chip=2       | mlo_num_adj_chip=2
 * mlo_adj_chip_idx=1,3     | mlo_adj_chip_idx=1,2
 *                          |
 * [MLO_SOC_CHIP_2]         | [MLO_SOC_CHIP_1]
 * mlo_chip_idx=1           | mlo_chip_idx=1
 * mlo_num_adj_chip=2       | mlo_num_adj_chip=2
 * mlo_adj_chip_idx=0,2     | mlo_adj_chip_idx=2,0
 *                          |
 * [MLO_SOC_CHIP_3]         | [MLO_SOC_CHIP_2]
 * mlo_chip_idx=2           | mlo_chip_idx=2
 * mlo_num_adj_chip=2       | mlo_num_adj_chip=2
 * mlo_adj_chip_idx=3,1     | mlo_adj_chip_idx=0,1
 *                          |
 * [MLO_SOC_CHIP_4]         |
 * mlo_chip_idx=3           |
 * mlo_num_adj_chip=2       |
 * mlo_adj_chip_idx=2,0     |
 */
/* qca-wifi asks mlo_config.ini via hotplug flow.
 * hotplug_firmware() read it from /etc/Wireless/ini if it doesn't exist in /lib/firmware.
 */
#define MLO_CONFIG_INI	"/etc/Wireless/ini/mlo_config.ini"
/* Generate /etc/Wireless/ini/mlo_config.ini
 * @return:
 * 	0:	success
 * otherwise:	error
 */
static int gen_mlo_config_ini(void)
{
	int i, j, first, ret = 0, chip_base = 0, nr_chips = -1;
	FILE *fp;

#if defined(TUFBE6500) || defined(TUFBE9400)
	/* mlo_config.ini is loaded at very earily stage.
	 * If number of MLO chip is three on TUF-BE9400 that switch 2-nd chip
	 * from 5G 4x4 to 5G+6G 2x2 later, it gets
	 * "ERR: plat_env is not found for chip 2" error message and
	 * "cmnos_allocram.c:553 Assertion 0" Q6 crash at very early * stage,
	 * uptime ~17s.
	 */
	if (runtime_has_6g())
		nr_chips = 2; //3;
	else
		nr_chips = 2;
#endif
	unlink(MLO_CONFIG_INI);
	if (nr_chips <= 0)
		return 0;

	if (!(fp = fopen(MLO_CONFIG_INI, "w"))) {
		return -1;
	}

	fprintf(fp, "mlo_max_num_groups=1\n"
		"\n[MLO_GROUP_0]\n"
		"mlo_max_num_peers=256\n"
		"mlo_num_chips=%d\n"
		"mlo_soc_chip_ids=", nr_chips);
	for (i = chip_base; i < (chip_base + nr_chips); ++i) {
		fprintf(fp, "%s%d", (i == chip_base)? "" : ",", i);
	}
	fprintf(fp, "\n");

	for (i = chip_base; i < (chip_base + nr_chips); ++i) {
		fprintf(fp, "\n[MLO_SOC_CHIP_%d]\n"
			"mlo_chip_idx=%d\n"
			"mlo_num_adj_chip=%d\n"
			"mlo_adj_chip_idx=", i, i, nr_chips - 1);
		first = 1;
		for (j = chip_base; j < (chip_base + nr_chips); ++j) {
			if (i == j)
				continue;
			fprintf(fp, "%s%d", first? "" : ",", j);
			first = 0;
		}
		fprintf(fp, "\n");
	}
	fclose(fp);

	return ret;
}
#else
static inline int gen_mlo_config_ini(void) { return 0; }
#endif

static int load_qcawificfg80211(const char *i_ini_tbl[WL_NR_BANDS], int testmode)
{
	const unsigned int daol_type = qca_wifi_type & QWIFI_DAOL_TYPE;
	const unsigned int df_type = qca_wifi_type & QWIFI_DF_TYPE;
	const char **up;
	int i, len, unit;
	struct load_wifi_kmod_seq_s *p = &load_wifi_kmod_seq[0];
	char param[1024], *s = &param[0], qca_nv[64], *val;
	char *g_ini[50], *g_i_ini[50], **g_v, **gi_v, *argv[50] = {
		"modprobe", "-s", NULL
	}, **v;
#if defined(RTCONFIG_CFG80211)
	const int cfg80211 __attribute__((unused)) = 1;
#else
	const int cfg80211 __attribute__((unused)) = 0;
#endif
#if defined(RTCONFIG_WIFI_QCA9990_QCA9990) || defined(RTCONFIG_WIFI_QCA9994_QCA9994) \
 || defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_SOC_IPQ60XX)
	unsigned int olcfg __attribute__((unused)) = nss_wifi_offloading();
	unsigned int hk_ol_num __attribute__((unused)) = 0;
	int total_mem __attribute__((unused)) = get_meminfo_item("MemTotal");
	int r, r0, r1, r2, l0, l1, l2;
	char buf[16];
	const char *extra_pbuf_core0 = "0";
	const char *n2h_high_water_core0 = "8704", *n2h_wifi_pool_buf = "0";
#if (SPF_VER >= SPF_VER_ID(11,3))
	char *spf11_3_fixed_ini_params[] __attribute__((unused)) = {
		"max_peers=0", "wds_ext=0",
		"nss_wifi_radio_scheme_enable=1",	/* Enable the radio scheme flag */
		"externalacs_enable=0",			/* icm_enable */
		NULL };
	char nss_wifi_radio_pri_map[sizeof("nss_wifi_radio_pri_map=XXXXXXXXXXXX")] __attribute__((unused));
#endif
#elif defined(RTCONFIG_SOC_IPQ53XX)
	int sawf_stats __attribute__((unused)) = 0;
	int mldev_mode __attribute__((unused)) = -1, sawf __attribute__((unused)) = 0;
	char dp_sawf_stats_str[sizeof("dp_sawf_stats=XXX")] __attribute__((unused)) = "";
	char cat_global_ini[sizeof("cat " GLOBAL_INI "XXXXXX")] __attribute__((unused)) = "";
	char sawf_str[sizeof("sawf=XXX")] __attribute__((unused)) = "";
#endif


	if (!i_ini_tbl)
		return -1;

	g_v = &g_ini[0];
	gi_v = &g_i_ini[0];
	*param = '\0';
	s = &param[0];
	len = sizeof(param);

#if defined(RTCONFIG_SOC_IPQ53XX)
	for (unit = 0; unit < WL_NR_BANDS; ++unit) {
		SKIP_ABSENT_BAND(unit);

		if (i_ini_tbl[unit] == NULL)
			continue;

		/* load_qcawificfg80211() */
		update_ini_file(i_ini_tbl[unit], "sawf=0");
		update_ini_file(i_ini_tbl[unit], "dp_sawf_stats=0");
#if (SPF_VER >= SPF_VER_ID(12,5))
		*gi_v++ = "dp_sawf_msduq_tid_skid=1";
		update_ini_file(i_ini_tbl[unit], "dp_sawf_mcast=0");
		update_ini_file(i_ini_tbl[unit], "sawf_adm_ctrl=0");
		update_ini_file(i_ini_tbl[unit], "dp_sawf_msduq_reclaim=0");
		*gi_v++ = "dp_sawf_reclaim_timer=20";
#endif
	}

	 /* Load ecm_wifi_plugin if
	  * SPF12.2: umac have been loaded.
	  * SPF12.5: umac and ecm have been loaded both.
	  */
	f_write_string("/sys/kernel/debug/ecm/ecm_classifier_emesh/sawf_enabled", "0", 0, 0);
	f_write_string("/sys/kernel/debug/ecm/ecm_classifier_dscp/enabled", "1", 0, 0);

	DPARM(g_v, s, len, "cfg80211_config=%d", cfg80211);

	*g_v++ = "max_peers=0";

#if (SPF_VER >= SPF_VER_ID(10,0))
	*g_v++ = "qwrap_enable=0";
#endif	/* SPF10 */

#ifdef RTCONFIG_MLO
	/* MLD Single Netdev is ENABLED */
	*g_v++ = "wds_ext=1";				/* FIXME: MLO */
	*g_v++ = "enable_mloadvert_degrade_on_cac=0";	/* FIXME: MLO */
	*g_v++ = "mlme_mlo_reconfig_reassoc_enable=0";	/* FIXME: MLO */
	*g_v++ = "non_mlo_11be_ap_operation_enable=1";	/* FIXME: MLO */
#else	
	*g_v++ = "wds_ext=0";				/* FIXME: MLO */
#endif

#if (SPF_VER >= SPF_VER_ID(12,5))
	update_ini_file(QCN9224_I_INI, "disable_bridge_vap=1");
#endif	/* SPF12.5+ */

#if (SPF_VER >= SPF_VER_ID(12,5))
	/* "Setting peer_ext_stats params -
	 * stats_max_window:$stats_max_window
	 * stats_max_packets_per_window:$stats_max_packets_per_window"
	 */
	*gi_v++ = "dp_stats_max_window=10";
	*gi_v++ = "dp_stats_max_packets_per_window=1000";

	/* sawf is enabled by default since SPF12.5 CSU1 */
	sawf = 1;
	update_ini_file(QCA5332_I_INI, "fw_ast_indication_disable=1");
	update_ini_file(QCN9224_I_INI, "fw_ast_indication_disable=1");

	/* load_qcawificfg80211() */
	snprintf(sawf_str, sizeof(sawf_str), "sawf=%d", sawf);
	for (unit = 0; unit < WL_NR_BANDS; ++unit) {
		SKIP_ABSENT_BAND(unit);

		if (i_ini_tbl[unit] == NULL)
			continue;
		update_ini_file(i_ini_tbl[unit], sawf_str);
	}

	if (sawf == 1) {
		sawf_stats = 0;

		f_write_string("/sys/kernel/debug/ecm/ecm_classifier_emesh/sawf_enabled", "3", 0, 0);
		f_write_string("/sys/kernel/debug/ecm/ecm_classifier_dscp/enabled", "0", 0, 0);

		for (unit = 0; unit < WL_NR_BANDS; ++unit) {
			SKIP_ABSENT_BAND(unit);

			if (i_ini_tbl[unit] == NULL)
				continue;
			/* load_qcawificfg80211() */
			update_ini_file(i_ini_tbl[unit], "sawf_ul_fse=1");
		}
		*gi_v++ = "dp_sawf_msduq_tid_skid=1";

		/* "Setting sawf params -
		 * sawf_msduq_tid_skid:$sawf_msduq_tid_skid
		 * sawf_msduq_reclaim:$sawf_msduq_reclaim
		 * sawf_reclaim_timer:$sawf_reclaim_timer"
		 */
		for (unit = 0; unit < WL_NR_BANDS; ++unit) {
			SKIP_ABSENT_BAND(unit);

			if (i_ini_tbl[unit] == NULL)
				continue;
			/* load_qcawificfg80211() */
			snprintf(dp_sawf_stats_str, sizeof(dp_sawf_stats_str), "dp_sawf_stats=%d", sawf_stats);
			update_ini_file(i_ini_tbl[unit], dp_sawf_stats_str);
			update_ini_file(i_ini_tbl[unit], "dp_sawf_msduq_reclaim=1");
		}
		*gi_v++ = "dp_sawf_reclaim_timer=20";
	}
#endif	/* SPF 12.5+ */

	/* Enable the radio scheme flag */
	*g_v++ = "nss_wifi_radio_scheme_enable=1";

	/* Force all the radios in NSS offload mode on 256M profile */

	/* IPA related INI params, also enable IPA by default for 1x Pine if not configured by user */

	snprintf(cat_global_ini, sizeof(cat_global_ini), "cat %s", GLOBAL_INI);
	exec_and_parse(cat_global_ini, "mldev_mode_ap=", "%d", 1, &mldev_mode);
	/* Disable PPE for unified non-bond and hybrid non-bond mlo models
	 * Set QCN9224_i.ini as below if mldev_mode_ap exist in global.ini and it equals to 0 or 2.
	 * ppe_ds_enable=0
	 * otherwise
	 * ppe_ds_enable=1
	 */
#if defined(RTCONFIG_WIFI_IPQ53XX_QCN6274)
	if (mldev_mode == 0 || mldev_mode == 2) {
		update_ini_file(QCN9224_I_INI, "ppe_ds_enable=0");
	} else {
		update_ini_file(QCN9224_I_INI, "ppe_ds_enable=1");
	}
#endif

	/* Disable HW offload stats in Hybrid MLO model
	 * Set QCN9224_i.ini/QCA5332_i.ini/QCN6432_i.ini as below
	 * if mldev_mode_ap exist in global.ini and it equals to 2.
	 * vdev_stats_hw_offload_config=0
	 */
	for (unit = 0; mldev_mode == 2 && unit < WL_NR_BANDS; ++unit) {
		SKIP_ABSENT_BAND(unit);

		if (i_ini_tbl[unit] == NULL)
			continue;

		update_ini_file(i_ini_tbl[unit], "vdev_stats_hw_offload_config=0");
	}


	/* Enable/disable 3 link mlo tx */

	*g_v++ = "dp_rx_hash=1";

#if LINUX_KERNEL_VERSION >= KERNEL_VERSION(6,6,0)
	/* Disable 11az in 6.6 Platform for now */
	update_ini_file(QCN9224_I_INI, "enable_responder_secure_ltf_support=0");
	update_ini_file(QCN9224_I_INI, "enable_responder_11az_support=0");
#endif

	/* qdf_args: qdf_log_dump_at_kernel_enable=0 */
	/* qdf_args: qdf_log_flush_timer_period=50 */

	*g_v++ = "logger_enable_mask=0";

	/* update the ini nss info */
	/* update_ini_nss_info() */

	/* icm_enable */
	*g_v++ = "externalacs_enable=0";

#elif defined(RTCONFIG_SOC_IPQ8074)
	/******************************************************************************/
	hk_ol_num = bitCount(olcfg);

	/* /sys/kernel/debug/WMI_SOC* doesn't exist, no need to handle
	 * filtered_wmi_cmds, filtered_wmi_evts.
	 */
#if defined(RTCONFIG_WIFI_QCN5024_QCN5054)
	/* update_ini_for_tx_vdev_id_check() */
#if (SPF_VER >= SPF_VER_ID(11,3))
	update_ini_file(QCA8074_I_INI, "dp_tx_allow_per_pkt_vdev_id_check=0");
	update_ini_file(QCA8074V2_I_INI, "dp_tx_allow_per_pkt_vdev_id_check=0");
#endif	/* SPF11.3+ */

	adjust_ring_buffer_in_ini(total_mem);
	update_ini_file(GLOBAL_I_INI, "hw_mode_id=1");

	/* load_qcawifi() */
	*g_v++ = "qwrap_enable=0";	/* SPF10.0 FC */
#if (SPF_VER >= SPF_VER_ID(11,3))
	DPARM(g_v, s, len, "nss_wifi_olcfg=%d", olcfg);
	if (olcfg) {
		*g_v++ = "dp_rx_hash=0";
	} else {
		*g_v++ = "dp_rx_hash=1";
	}
#else
	DPARM(g_v, s, len, "nss_wifi_olcfg=%d", olcfg);
	if (olcfg) {
		*g_v++ = "rx_hash=0";
	}
#endif	/* SPF11.3+ */
	DPARM(g_v, s, len, "nss_wifi_olcfg=%d", !!olcfg);
#if (SPF_VER >= SPF_VER_ID(11,3))
	__update_ini_file(GLOBAL_I_INI, spf11_3_fixed_ini_params);
#endif
#endif	/* RTCONFIG_WIFI_QCN5024_QCN5054 */

	update_ini_dp_rings_for_nss_mode();

	/* wifiX/nssoffload doesn't exist, nss_olcfg = nss_ol_num = 0, thus
	 * /lib/wifi/{wifi_nss_hk_olnum,wifi_nss_olcfg,wifi_nss_olnum} = 0
	 */

#elif defined(RTCONFIG_SOC_IPQ60XX)
	/******************************************************************************/
	*g_v++ = "qwrap_enable=0";	/* SPF10.0 FC */
	DPARM(g_v, s, len, "cfg80211_config=%d", cfg80211);
	DPARM(g_v, s, len, "nss_wifi_olcfg=%d", olcfg);
	DPARM(g_v, s, len, "allow_mon_vaps_in_sr=");
#if (SPF_VER >= SPF_VER_ID(11,3))
	__update_ini_file(GLOBAL_I_INI, spf11_3_fixed_ini_params);
#endif
#endif	/* RTCONFIG_SOC_IPQ53XX */


#if (SPF_VER >= SPF_VER_ID(11,3))
#if defined(RTCONFIG_SOC_IPQ8074)
	/* Each radio is mapped to 4 bit priority. Higher value has high priority.
	 * Based on number of radios in board, priority is aggregated and
	 * populated into global ini.
	 * Eg: Board ap-hk10-c2 - 0x01. First radio is higher priority than second.
	 * config_nss_wifi_radio_pri_map()
	 * It's executed if /sys/class/net/wifiX/nssoffload = capable
	 */
	snprintf(nss_wifi_radio_pri_map, sizeof(nss_wifi_radio_pri_map),
		"nss_wifi_radio_pri_map=%d", 0x1101);
	update_ini_file(GLOBAL_I_INI, nss_wifi_radio_pri_map);
#endif
#endif	/* SPF11.3+ */

#if defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_SOC_IPQ60XX)
	/* update the ini nss info */
	update_ini_nss_info(hk_ol_num);
#endif

#if defined(RTCONFIG_WIFI_QCA9990_QCA9990) \
 || defined(RTCONFIG_WIFI_QCA9994_QCA9994) \
 || defined(RTCONFIG_WIFI_QCN5024_QCN5054)
	/* Always wait NSS ready whether NSS WiFi offloading is enabled or not. */
	for (i = 0, *buf = '\0'; i < 10; ++i) {
		r0 = f_read_string(N2H_HIGH_WATER_CORE0_FN, buf, sizeof(buf));
		if (r0 > 0 && strlen(buf) > 0)
			break;
		else {
			dbg(".");
			sleep(1);
		}
	}

	/* Always use maximum extra_pbuf_core0.
	 * Because it can't be changed if it is allocated, write non-zero value.
	 */
	get_nss_buf_size(total_mem, olcfg, &extra_pbuf_core0, &n2h_high_water_core0, &n2h_wifi_pool_buf);
	l0 = strlen(extra_pbuf_core0);
	l1 = strlen(n2h_high_water_core0);
	l2 = strlen(n2h_wifi_pool_buf);
	for (i = 0; olcfg && i < 10; ++i) {
		f_read_string(N2H_HIGH_WATER_CORE0_FN, buf, sizeof(buf));

		*buf = '\0';
		r0 = l0;
		r = f_read_string(EXTRA_PBUF_CORE0_FN, buf, sizeof(buf));
		if (r > 0 && atol(buf)) {
			//dbg("%s: extra_pbuf_core0 is allocated!!! [%s]\n", __func__, buf);
		}
		if (r <= 0 || (r > 0 && atol(buf) < atol(extra_pbuf_core0)))
			r0 = f_write_string(EXTRA_PBUF_CORE0_FN, extra_pbuf_core0, 0, 0);

		r1 = f_write_string(N2H_HIGH_WATER_CORE0_FN, n2h_high_water_core0, 0, 0);
		r2 = f_write_string(N2H_WIFI_POOL_BUF_FN, n2h_wifi_pool_buf, 0, 0);
		if (r0 < l0 || r1 < l1 || r2 < l2) {
			dbg(".");
			sleep(1);
			continue;
		}
		break;
	}
#endif

#if defined(RTCONFIG_WIFI_QCN5024_QCN5054) || defined(RTCONFIG_SOC_IPQ53XX)
	*gi_v++ = "ap_bss_color_collision_detection=0";	/* 11ax client IoT issue */
#endif

	/* umac_args: enable_pktlog_support=0 */

#if defined(RTCONFIG_WIFI_QCN5024_QCN5054) \
 || defined(RTCONFIG_QCA_AXCHIP) \
 || defined(RTCONFIG_QCA_BECHIP)
	/* Target-Wake Time
	 * Always use wl0_twt and make sure wlX_twt equal to each other due to all bands share same global.ini.
	 * This controls "TWT Responder Support" of octet 10 of Extended Capabilities.
	 */

	if (nvram_match("wl0_twt", "1")) {
		update_ini_file(GLOBAL_INI, "twt_enable=1");
		update_ini_file(GLOBAL_INI, "bcast_twt_enable=1");
	} else {
		update_ini_file(GLOBAL_INI, "twt_enable=0");
		update_ini_file(GLOBAL_INI, "bcast_twt_enable=0");
	}
#endif

	/* Append customize parameters. */
	for (up = global_ini_params; up != NULL && *up != NULL; up++) {
		snprintf(qca_nv, sizeof(qca_nv), "qca_%s", *up);
		if (!(val = nvram_get(qca_nv)))
			continue;
		DPARM(g_v, s, len, "%s=%s", *up, val);
	}

	/* Apply changes to global.ini and global_i.ini */
	*g_v++ = *gi_v++ = NULL;
	__update_ini_file(GLOBAL_INI, g_ini);
	__update_ini_file(GLOBAL_I_INI, g_i_ini);

	/* Load WiFi drivers. */
	for (i = 0, len = sizeof(param), p = &load_wifi_kmod_seq[i];
	     len > 0 && i < ARRAY_SIZE(load_wifi_kmod_seq);
	     ++i, ++p)
	{
		/* Don't load a DA/OL or adf/qdf module if it is not used. */
		if (qca_wifi_type) {
			if (((p->flags & QWIFI_DAOL_TYPE) && !(daol_type & p->flags))
			 || ((p->flags & QWIFI_DF_TYPE) && !(df_type & p->flags)))
			{
				// dbg(" %s: ignore %s. (flags %x/%x)\n", __func__,
				// 	p->kmod_name, p->flags, qca_wifi_type);
				continue;
			}
		}
		if (module_loaded(p->kmod_name))
			continue;

		v = &argv[2];
		*v++ = p->kmod_name;
		*param = '\0';
		s = &param[0];

		/* If module-specific hook function for parameters adjustment is defined, execute it. */
		if (!testmode) {
			if (p->mission_mode_param_hook_fn)
				p->mission_mode_param_hook_fn(&v, &s, &len);
		} else {
			if (p->test_mode_param_hook_fn)
				p->test_mode_param_hook_fn(&v, &s, &len);
		}

		/* Extra parameters defined in qca_XXX nvram variables for experiment purpose. */
		for (up = p->params; up != NULL && *up != NULL; up++) {
			snprintf(qca_nv, sizeof(qca_nv), "qca_%s", *up);
			if (!(val = nvram_get(qca_nv)))
				continue;
			DPARM(v, s, len, "%s=%s", *up, val);
		}

		*v++ = NULL;
		_eval(argv, NULL, 0, NULL);

		if (p->load_sleep)
			sleep(p->load_sleep);

		if (p->post_fn)
			p->post_fn();
	}

	return 0;
}

/**
 * Low level function to load QCA WiFi driver.
 * @testmode:	if true, load WiFi driver as test mode which is required in ATE mode.
 */
static void __load_wifi_driver(int testmode)
{
	char country[FACTORY_COUNTRY_CODE_LEN + 1], code_str[6], prefix[sizeof("wlXXXXX_")];
	int unit;
	FILE *fp_wifi;
	const struct irq_smp_affinity_s *irqp;
	char vphy[IFNAMSIZ] = { 0 }, path_wifi[sizeof("/tmp/wifiXXXX.sh")];
	char *stats_cmd;
#if defined(RTCONFIG_WIFI_QCN5024_QCN5054)
	const int wifi_stats = 1;	/* enable_ol_stats must be enabled on Hawkeye platform. */
#elif defined(RTCONFIG_QCA_AXCHIP) \
   || defined(RTCONFIG_QCA_BECHIP)
	const int wifi_stats = 1;	/* enable_ol_stats must be enabled on HK, CP, MP new platform */
#else
	const int wifi_stats = 0;
#endif
#if !defined(RTCONFIG_QCA_WLAN_SCRIPTS)
	char *board_name __attribute__((unused)) = get_ref_board_name();
	char *fw_ini_file __attribute__((unused)) = get_fw_ini_file();
#if defined(RTCONFIG_GLOBAL_INI)
	const char *i_ini_tbl[WL_NR_BANDS];
#endif
#if defined(RTCONFIG_SOC_IPQ53XX)
	int txchains;
	char txchains_str[4];
	char txchains_path[sizeof(SYS_CLASS_NET) + IFNAMSIZ + sizeof("/txchainsXXXXXX")];
#endif

	factory_to_caldata_file();
#if defined(RTCONFIG_GLOBAL_INI)
	// Copy /ini/* to /etc/Wireless/ini
	eval("mkdir", "-p", GLOBAL_INI_TOPDIR);
	system("cp -a /ini " GLOBAL_INI_TOPDIR "/..");

	memset(&i_ini_tbl, 0, sizeof(i_ini_tbl));
#if defined(RTCONFIG_SOC_IPQ53XX)
	/* SPF12.2 detect_qcawificfg80211() -> create_mlo_config() */
	gen_mlo_config_ini();

	/* SPF12.2 CSU1 detect_qcawificfg80211() */
	if (!board_name || !fw_ini_file) {
		dbg("%s: Invalid board_name [%s] or fw_ini_file [%s]\n",
			__func__, board_name, fw_ini_file);
		return;
	}

	i_ini_tbl[WL_2G_BAND] = QCA5332_I_INI;
#if defined(RTCONFIG_WIFI_IPQ53XX_QCN6274)
	i_ini_tbl[WL_5G_BAND] = QCN9224_I_INI;
#if defined(TUFBE6500)
	if (runtime_has_6g()) {
		i_ini_tbl[WL_6G_BAND] = QCN9224_I_INI;
	}
#endif
#elif defined(RTCONFIG_WIFI_IPQ53XX_QCN64XX)
	i_ini_tbl[WL_5G_BAND] = QCN64XX_I_INI;
#else
#error No 5G INI file?
#endif
	/* detect_qcawificfg80211() */
	for (unit = 0; unit < WL_NR_BANDS; ++unit) {
		SKIP_ABSENT_BAND(unit);

		if (i_ini_tbl[unit] == NULL)
			continue;
		/* update_ini_reo_remap() */
		update_ini_file(i_ini_tbl[unit], "dp_reo_rings_map=0x7");
		/* update_ini_napi_scale_factor() */
		update_ini_file(i_ini_tbl[unit], "dp_napi_scale_factor=1");
		/* update_ini_refill_ring_size() */
		/* update_ini_target_dp_rx_hash_reset() */
		/* update_ini_target_dp_default_reo_reset() */
		/* update_ini_ppe_vp_core_mask() */
		update_ini_file(i_ini_tbl[unit], "ppe_vp_core_mask=0x7");
	}

#if (SPF_VER >= SPF_VER_ID(12,5))
	/* Makes qca-wifi remember NOL list even all QCA WiFi modules,
	 * except mem_manager, are reloaded.
	 */
	update_ini_file(GLOBAL_INI, "dfs_retain_nol_across_driver_reload=1");
#endif

	/* detect_qcawificfg80211()
	 * => load_qcawificfg80211()
	 */
	load_qcawificfg80211(i_ini_tbl, testmode);
#endif	/* RTCONFIG_SOC_IPQ53XX */
#endif	/* RTCONFIG_GLOBAL_INI */

#else	/* RTCONFIG_QCA_WLAN_SCRIPTS */
	eval("mkdir", "-p", "/tmp/wifi");	/* /data/vendor/wifi ==> /tmp/wifi */
	doSystem("loadwifi.sh %d %d\n", testmode, atf_enabled()? 1:0);
#endif // RTCONFIG_QCA_WLAN_SCRIPTS

	/* detect_qcawificfg80211(), load_qcawificfg80211() has been finish,
	 * WiFi modules have been loaded.
	 */

#if (SPF_VER >= SPF_VER_ID(12,0))
#if defined(RTCONFIG_SOC_IPQ53XX)
	readdir_wrapper(SYS_CLASS_NET, "soc", __update_hw_mode_id, NULL);

	/* Set dp_mon_2chain_ring=1 or dp_mon_4chain_ring=1 based on number of TX/RX chains.
	 * Get nr. of TX/RX chains from /sys/class/net/wifiX/{txchains,rxchains}
	 */
	for (unit = 0; unit < WL_NR_BANDS; ++unit) {
		if (unit == WL_2G_BAND)
			continue;
		SKIP_ABSENT_BAND(unit);

		snprintf(txchains_path, sizeof(txchains_path),
			"%s/%s/txchains", SYS_CLASS_NET, get_vphyifname(unit));
		txchains = -1;
		if (f_read_string(txchains_path, txchains_str, sizeof(txchains_str)) > 0)
			txchains = safe_atoi(txchains_str);
		if (txchains <= 2) {
			update_ini_file(i_ini_tbl[unit], "dp_mon_2chain_ring=1");
		} else if (txchains <= 4) {
			update_ini_file(i_ini_tbl[unit], "dp_mon_4chain_ring=1");
		} else {
			_dprintf("Unknown txchains of %s: %d (%s)!\n", get_vphyifname(unit),
				txchains, txchains_str);
		}
	}
#endif	/* RTCONFIG_SOC_IPQ53XX */
#endif	/* SPF12.0+ */

	if (testmode)
		return;

	eval("mkdir", "-p", "/etc/Wireless/sh");
	for (unit = 0; unit < MAX_NR_WL_IF; ++unit) {
		SKIP_ABSENT_BAND(unit);

		snprintf(prefix, sizeof(prefix), "wl%d_", unit);
		strlcpy(vphy, get_vphyifname(unit), sizeof(vphy));
		snprintf(path_wifi, sizeof(path_wifi), "/tmp/%s.sh", vphy);
		if (!(fp_wifi = fopen(path_wifi, "w"))) {
			dbg("%s: Can't open %s for writing!\n", __func__, path_wifi);
			continue;
		}

		fprintf(fp_wifi, "#!/bin/sh\n");
		switch (unit) {
#if defined(RTCONFIG_WIGIG)
		case WL_60G_BAND:
			/* Pure iw + nl80211 based band. */
			/* country code of 802.11ad Wigig can be handled by hostapd too. */
			strlcpy(country, nvram_safe_get("wl3_country_code"), sizeof(country));
			if (country_to_code(country, g_band2fid[unit], code_str, sizeof(code_str)) < 0)
				country_to_code("DB", g_band2fid[unit], code_str, sizeof(code_str));
			eval("iw", "reg", "set", code_str);

			tweak_wifi_ps(WIF_60G);
			break;
#endif
		default:
			/* Wireless Extension or cfg80211 + nl80211 based band. */

			/* Country */
			strlcpy(country, nvram_pf_safe_get(prefix, "country_code"), sizeof(country));
			if (country_to_code(country, g_band2fid[unit], code_str, sizeof(code_str)) < 0)
				country_to_code("DB", g_band2fid[unit], code_str, sizeof(code_str));
			fprintf(fp_wifi, IWPRIV " %s setCountryID %s\n", vphy, code_str);

			/* OL stats */
			stats_cmd = "enable_ol_stats";
#if defined(RTCONFIG_WIFI_QCA9557_QCA9882) || defined(RTCONFIG_QCA953X) || defined(RTCONFIG_QCA956X) || defined(RTCONFIG_QCN550X)
			if (unit == WL_2G_BAND)
				stats_cmd = "disablestats";
#endif
			fprintf(fp_wifi, IWPRIV " %s %s %d\n", vphy, stats_cmd, wifi_stats);

			/* TX power adjustment. */
			if (find_word(nvram_safe_get("rc_support"), "pwrctrl")) {
				fprintf(fp_wifi, IWPRIV " %s txpwrpc %s\n",
					vphy, nvram_pf_safe_get(prefix, "txpower"));
			}

			if (0
#if defined(RTCONFIG_AMAS_WGN)
			 || is_wgn_enabled()
#elif defined(RTCONFIG_MULTILAN_CFG)
			 || sdn_enable()
#endif
			) {
				int band;

				for (band = 0; band < WL_NR_BANDS; ++band) {
					SKIP_ABSENT_BAND(band);

					strlcpy(vphy, get_vphyifname(band), sizeof(vphy));
					eval(IWPRIV, vphy, "no_vlan", "1");
				}
			}

#if defined(RTCONFIG_WIFI_SON)
			if (sw_mode()!=SW_MODE_REPEATER && nvram_match("wifison_ready", "1")) {
#if defined(RTCONFIG_HIDDEN_BACKHAUL)
		        	if(strlen(nvram_safe_get("cfg_group")))
#else
				if(nvram_get_int("x_Setting"))
#endif
				{
					int band;

					for (band = 0; band < WL_NR_BANDS; ++band) {
						SKIP_ABSENT_BAND(band);

						strlcpy(vphy, get_vphyifname(band), sizeof(vphy));
						eval(IWPRIV, vphy, "no_vlan", "1");
					}
					//send RCSA to uplink/CAP/PAP when detect radar
					if (nvram_get_int("dfs_check_period"))
						eval(IWPRIV, (char*) VPHY_5G, "CSwOpts", "0x30");
#if defined(RTCONFIG_HAS_5G_2)
					if (nvram_get_int("ncb_enable"))
						doSystem("iwpriv %s ncb_enable %s", VPHY_5G, nvram_get("ncb_enable"));
#endif
				}
			}
#endif /* WIFI_SON */
			tweak_wifi_ps(vphy);
		}

		fclose(fp_wifi);
		chmod(path_wifi, 0777);
	}

	for (irqp = &wifi_irq_smp_affinity_tbl[0]; irqp->irq >= 0; ++irqp) {
		set_irq_smp_affinity(irqp->irq, irqp->cpu_mask);
	}

#if !defined(RTCONFIG_CFG80211)
	/* add acs channel weight */
	acs_ch_weight_param();
#endif	/* !RTCONFIG_CFG80211 */

#if defined(RTCONFIG_SOC_IPQ40XX)
	f_write_string("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "performance", 0, 0); // for throughput
	f_write_string("/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq", "716000", 0, 0); // for throughput
#endif

	save_wl_params_for_testing_reload_wifi_drv();
}

void load_wifi_driver(void)
{
#if defined(RTCONFIG_SOC_IPQ8074)
	char val[4];

	snprintf(val, sizeof(val), "%d", min(get_pagecache_ratio(), 25));
	f_write_string("/proc/sys/vm/pagecache_ratio", val, 0, 0);
	f_write_string("/proc/net/skb_recycler/flush", "1", 0, 0);	/* remove skb and pause skb recycler. */
#elif defined(RTCONFIG_SOC_IPQ8064)
	f_write_string("/proc/sys/vm/pagecache_ratio", "25", 0, 0);
	f_write_string("/proc/net/skb_recycler/max_skbs", "2176", 0, 0);
#elif defined(RTCONFIG_SOC_IPQ40XX)
	/* For lower memory system, e.g., IPQ40xx + 128MB RAM models. */
	if (get_meminfo_item("MemTotal") <= 131072) {
		f_write_string("/proc/net/skb_recycler/flush", "1", 0, 0);
		f_write_string("/proc/net/skb_recycler/max_skbs", "10", 0, 0);
		f_write_string("/proc/net/skb_recycler/max_spare_skbs", "10", 0, 0);
	}
#elif defined(RTCONFIG_QSDK6PLUS) && (defined(RTCONFIG_QCA953X) || \
				      defined(RTCONFIG_QCA956X) || \
				      defined(RTCONFIG_QCN550X))
	/* For QCA95XX/QCN550X newer SDK with CONFIG_SKB_RECYCLER enabled */
	f_write_string("/proc/net/skb_recycler/flush", "1", 0, 0);
	f_write_string("/proc/net/skb_recycler/max_skbs", "256", 0, 0);
#elif defined(RTCONFIG_SOC_IPQ53XX)
	f_write_string("/proc/net/skb_recycler/flush", "1", 0, 0);	/* remove skb and pause skb recycler. */
#endif
	__load_wifi_driver(0);
}

void load_testmode_wifi_driver(void)
{
	__load_wifi_driver(1);
}

void set_uuid(void)
{
	int len;
	char *p, uuid[60];
	FILE *fp;

	fp = popen("cat /proc/sys/kernel/random/uuid", "r");
	 if (fp) {
	    memset(uuid, 0, sizeof(uuid));
	    fread(uuid, 1, sizeof(uuid), fp);
	    for (len = strlen(uuid), p = uuid; len > 0; len--, p++) {
		    if (isxdigit(*p) || *p == '-')
			    continue;
		    *p = '\0';
		    break;
	    }
	    nvram_set("uuid",uuid);
	    pclose(fp);
	 }   
}

/**
 * Down all VAP interfaces, kill all related hostapd instance and
 * delete VAP interface if @unregister_vap is true.
 * @unregister_vap:	Unregister VAP interface if true, except WiGig interface.
 */
void deinit_all_vaps(const int unregister_vap)
{
	int unit, sunit, max_sunit;
	char wif[IFNAMSIZ], prefix[sizeof("wlX_YYY")];
	char hconf_path[sizeof("/etc/Wireless/conf/hostapd_X.confXXX") + IFNAMSIZ];
#if defined(RTCONFIG_SINGLE_HOSTAPD)
	char sock_path[sizeof("/var/run/hostapd/XXXXXX") + IFNAMSIZ];
#else
	char pid_path[sizeof("/var/run/hostapd_athXXX.pidYYYYYY")];
#endif
	
#if defined(RTCONFIG_SOC_IPQ53XX)
	eval("killall", "hostapd_mon");
#endif

	for (unit = 0; unit < MAX_NR_WL_IF; ++unit) {
		SKIP_ABSENT_BAND(unit);
		max_sunit = num_of_mssid_support(unit);
		snprintf(prefix, sizeof(prefix), "wl%d_", unit);
		for (sunit = 0; sunit <= max_sunit; ++sunit) {
			__get_wlifname(unit, sunit, wif);
			snprintf(hconf_path, sizeof(hconf_path), "/etc/Wireless/conf/hostapd_%s.conf", wif);
#if defined(RTCONFIG_SINGLE_HOSTAPD)
			snprintf(sock_path, sizeof(sock_path), "/var/run/hostapd/%s", wif);
			if (f_exists(sock_path))
				eval(QWPA_CLI, "-g", QHOSTAPD_CTRL_IFACE, "raw", "REMOVE", wif);
#else
			snprintf(pid_path, sizeof(pid_path), "/var/run/hostapd_%s.pid", wif);
			if (f_exists(pid_path))
				kill_pidfile_tk(pid_path);
#endif
			if (unregister_vap && f_exists(hconf_path))
				unlink(hconf_path);
			if (!iface_exist(wif))
				continue;
			ifconfig(wif, 0, NULL, NULL);
			if (!unregister_vap || unit == WL_60G_BAND)
				continue;
			destroy_vap(wif);
		}
	}

#if defined(RTCONFIG_SINGLE_HOSTAPD)
	kill_pidfile_tk(QHOSTAPD_PID_PATH);
#endif

	/* in case of pid file is gone...*/
	eval("killall", "hostapd");
}

/**
 * Rebuild main VAP of each bands.
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
int rebuild_main_vap(void)
{
	int band;
	char vap[IFNAMSIZ];
#if defined(RTCONFIG_SINGLE_HOSTAPD)
	char sock_path[sizeof("/var/run/hostapd/XXXXXX") + IFNAMSIZ];
#else
	char pid_path[sizeof("/var/run/hostapd_athXXX.pidYYYYYY")];
#endif

	for (band = 0; band < MAX_NR_WL_IF; ++band) {
		SKIP_ABSENT_BAND(band);
		__get_wlifname(band, 0, vap);
#if defined(RTCONFIG_SINGLE_HOSTAPD)
		snprintf(sock_path, sizeof(sock_path), "/var/run/hostapd/%s", vap);
		if (f_exists(sock_path))
			eval(QWPA_CLI, "-g", QHOSTAPD_CTRL_IFACE, "raw", "REMOVE", vap);
#else
		snprintf(pid_path, sizeof(pid_path), "/var/run/hostapd_%s.pid", vap);
		if (f_exists(pid_path))
			kill_pidfile_tk(pid_path);
#endif
		if (band == WL_60G_BAND)
			continue;
		if (iface_exist(vap))
			destroy_vap(vap);
		create_vap(vap, band, "ap");
	}

	return 0;
}

struct vap2band_s {
	const char *vap;	/* main ap vap or sta vap if concurrent repeater enabled. */
	enum wl_band_id band;
};

static const struct vap2band_s vap2band_tbl[] = {
	{ WIF_2G,	WL_2G_BAND },
	{ WIF_5G,	WL_5G_BAND },
#if defined(RTCONFIG_HAS_5G_2)
	{ WIF_5G2,	WL_5G_2_BAND },
#endif
#if defined(RTCONFIG_HAS_6G)
	{ WIF_6G,	WL_6G_BAND },
#if defined(RTCONFIG_HAS_6G_2)
	{ WIF_6G2,	WL_6G_2_BAND },
#endif
#endif
#if defined(RTCONFIG_WIGIG)
	{ WIF_60G,	WL_60G_BAND },
#endif
#if defined(RTCONFIG_CONCURRENTREPEATER)
	{ STA_2G,	WL_2G_BAND },
	{ STA_5G,	WL_5G_BAND },
#endif
	{ NULL, -1 }
};

#if defined(TUFBE6500)
/* If HwId hasn't been set, guess HwId by wifi2 board_id.
 * In this case, U-Boot has to provide .dts with correct board_id.
 */
void guess_hwid_by_wifi2_board_id(void)
{
	uint32_t v = 0, id;

	if (strlen(nvram_safe_get("HwId")))
		return;

	f_read(WIFI2_BOARD_ID, &v, sizeof(v));
	id = ntohl(v);
	_dprintf("WiFi2 board_id 0x%x\n", v);
	if (id == 0x1019)
		nvram_set("HwId", "B");
	else if (id == 0x15)
		nvram_set("HwId", "A");
	else {
		_dprintf("Read WiFi2 board_id failed or unknown id: 0x%x\n", id);
	}
}
#endif

static int create_node=0;
void init_wl(void)
{
   	int unit;
	char *p, *ifname, *wl_ifnames;
	char path_wifi[sizeof("/tmp/wifiXXXX.sh")];
	const struct vap2band_s *v2b;
#ifdef RTCONFIG_WIRELESSREPEATER
	int wlc_band;
#endif
#if (SPF_VER >= SPF_VER_ID(12,5))
	char bs[32];
	char *dd_argv[] = { "dd", "if=/dev/caldata", "of=/tmp/virtual_art.bin", bs, "count=1", NULL };
#endif

#if defined(RTCONFIG_WIFI_DRV_DISABLE)
	if (nvram_match("lyra_disable_wifi_drv", "1"))
		return;
#endif

#if defined(RTCONFIG_SOC_IPQ53XX)
#if (SPF_VER >= SPF_VER_ID(12,5))
	/* raw_art_read() */
	snprintf(bs, sizeof(bs), "bs=%d", FTRY_PARM_SHIFT);
	_eval(dd_argv, NULL, 0, NULL);
	_dprintf("Copy ART caldata from /dev/caldata to /tmp/virtual_art.bin\n");
#endif
	gen_sysinfo();
	/* Since SPF12.5, CNSS2 need /tmp/ftm.conf. But existence of the file
	 * causes ipq_cnss2 read caldata via different filename, e.g.,
	 * caldata_2.b0015 on TUF-BE6500 and caldata_2.b1019 on TUF-BE9400,
	 * instead of caldata_2.bin. Reference to cnss_wlfw_bdf_dnld_send_sync()
	 * of qca-cnss module. If MAC address of VAP wrong and ftm.conf
	 * exist when ipq_cnss2 is loaded, check below message in console and add
	 * proper symbolic link in qca-wifi-fw package:
	 *
	 * cnss[38]: WARN: Caldata not present. Skipping caldata download: qcn9224/caldata_2.b0015
	 */
	gen_ftm_conf();

	/* /etc/rc.d/S00load_cnss2 */
	if (!module_loaded("ipq_cnss2")) {
		/* log_level=3 (CNSS_LOG_LEVEL_INFO) */
		modprobe("ipq_cnss2", "log_level=3", "enable_mlo_support=1");
	}
	if (!pids("cnssdaemon")) {
		char *cnssdaemon_argv[] = { "cnssdaemon", "-s", NULL };
		_eval(cnssdaemon_argv, DBGOUT, 0, NULL);
	}
#endif	/* RTCONFIG_SOC_IPQ53XX */

	handle_location_code_for_wl();

	if(!create_node)
	{ 
		load_wifi_driver();
		sleep(2);

		for (unit = 0; unit < MAX_NR_WL_IF; ++unit) {
			SKIP_ABSENT_BAND(unit);

			snprintf(path_wifi, sizeof(path_wifi), "/tmp/%s.sh", get_vphyifname(unit));
			eval(path_wifi);
			unlink(path_wifi);
		}
#ifdef RTCONFIG_MLO
		create_mld_vap();
#endif
		dbG("init_wl:create wi node\n");
#if defined(RTCONFIG_HIDDEN_BACKHAUL)
		if(sw_mode() != SW_MODE_REPEATER)
			add_bh_network();
#endif
		if ((wl_ifnames = strdup(nvram_safe_get("lan_ifnames"))) != NULL) 
		{
			p = wl_ifnames;
			while ((ifname = strsep(&p, " ")) != NULL) {
				while (*ifname == ' ') ++ifname;
				if (*ifname == 0) break;
				SKIP_ABSENT_FAKE_IFACE(ifname);

#if defined(RTCONFIG_BONDING_WAN) || defined(RTCONFIG_LACP)
				if (!strncmp(ifname, "bond", 4))
					continue;
#endif

				for (unit = -99, v2b = &vap2band_tbl[0];
				     unit < 0 && v2b->vap && v2b->band >= 0;
				     ++v2b)
				{
					if (strncmp(ifname, v2b->vap, strlen(v2b->vap)))
						continue;
					unit = v2b->band;
				}

				//create ath00x & ath10x
				switch (unit) {
				case WL_2G_BAND:	/* fall-through */
				case WL_5G_BAND:	/* fall-through */
				case WL_5G_2_BAND:	/* fall-through */
#if defined(RTCONFIG_HAS_6G)
				case WL_6G_BAND:	/* fall-through */
#endif
#if defined(RTCONFIG_HAS_6G_2)
				case WL_6G_2_BAND:	/* fall-through */
#endif
#if defined(RTCONFIG_CONCURRENTREPEATER)
					if (!strncmp(ifname, STA_2G, strlen(STA_2G)) || !strncmp(ifname, STA_5G, strlen(STA_5G))) {
						if(sw_mode() == SW_MODE_REPEATER) {
							create_vap(ifname, unit, "sta");
						}
					} else {
						create_vap(ifname, unit, "ap");
						sleep(1);
					}
#else					
					create_vap(ifname, unit, "ap");
					sleep(1);
#if defined(RTCONFIG_MLO)
					if(is_mlo_if(ifname))
						add_mld_nvram(ifname,"ap");
#endif
#if defined(RTCONFIG_REPEATER_STAALLBAND)
					if (sw_mode() == SW_MODE_REPEATER && nvram_get_int("x_Setting")) {
						dbG("\ncreate a STA node %s from %s\n", get_staifname(unit), get_vphyifname(unit));
						create_vap(get_staifname(i), i, "sta");
						sleep(1);
					}
#endif
#endif	    
					break;
#if defined(RTCONFIG_WIGIG)
				case WL_60G_BAND:
					/* Nothing to do. Both VAP and VPHY interfaces are created by driver automatically. */
					break;
#endif
				default:
					if (!strncmp(ifname, "eth", 3))
						break;
					dbg("%s: Unknown wl%d band, ifname [%s]!\n", __func__, unit, ifname);
				}
			}
			free(wl_ifnames);
		}
		create_node=1;
#if defined(RTCONFIG_HIDDEN_BACKHAUL)
		if(sw_mode() != SW_MODE_REPEATER)
			del_bh_network();
#endif

#ifdef RTCONFIG_WIRELESSREPEATER
#if !defined(RTCONFIG_CONCURRENTREPEATER) && !defined(RTCONFIG_REPEATER_STAALLBAND)
		if ((sw_mode() == SW_MODE_REPEATER || wisp_mode()) && nvram_get_int("x_Setting")) {
		  	wlc_band=nvram_get_int("wlc_band");
			if (wlc_band != WL_60G_BAND) {
				int band;
				char sta[IFNAMSIZ];

				strlcpy(sta, get_staifname(wlc_band), sizeof(sta));
				create_vap(sta, wlc_band, "sta");
#if defined(RTCONFIG_MLO)
				if (nvram_get_int("qca_mlo_mb")) {
					for (band = 0; band < MAX_NR_WL_IF; ++band) {
						SKIP_ABSENT_BAND(band);
						if (band == wlc_band)
							continue;
						strlcpy(sta, get_staifname(band), sizeof(sta));
						create_vap(sta, band, "sta");
					}
				}
#endif
			}
		}
#endif
#endif

		/* Calculate 40/80/160MHz and 4.32/6.48/8.64GHz bandwidth capability based on channel list.
		 * Main VAP must ready before doing this.
		 */
		for (unit = 0; unit < MAX_NR_WL_IF; ++unit) {
			SKIP_ABSENT_BAND(unit);
			calculate_bw_of_each_channel(unit);
		}

#ifdef RTCONFIG_WIFI_SON
		if(sw_mode()!=SW_MODE_REPEATER && nvram_match("wifison_ready", "1")) {
			if((sw_mode() == SW_MODE_AP && !nvram_match("cfg_master", "1")) || nvram_match("wps_e_success", "1"))
			{
				if(nvram_get_int("x_Setting"))
				{
					_dprintf("=>init_wl: create sta vaps\n");
					create_vap(get_staifname(i), i, "sta");
					sleep(1);
					ifconfig(get_staifname(1), IFUP, NULL, NULL);
				}
			}
		}
#endif
#ifdef RTCONFIG_AMAS
		if((sw_mode() == SW_MODE_AP && nvram_match("re_mode", "1")))
		{
			int i;
			char *sta_ifnames = strdup(nvram_safe_get("sta_ifnames"));
			char *skip_ifnames = strdup(nvram_safe_get("skip_ifnames"));
			_dprintf("=>init_wl: create sta vaps: %d\n", MAX_NR_WL_IF);
			for (i = 0; i < MAX_NR_WL_IF; i++) {
				char sta[IFNAMSIZ];

				SKIP_ABSENT_BAND(i);
				strlcpy(sta, get_staifname(i), sizeof(sta));
				if (sta_ifnames && strstr(sta_ifnames, sta) == NULL)
					continue;
				if (skip_ifnames && strstr(skip_ifnames, sta) != NULL)
					continue;
				create_vap(sta, i, "sta");
#if defined(RTCONFIG_MLO)
				if (is_mlo_if(sta))
					add_mld_nvram(sta,"sta");
#endif
			}
			free(sta_ifnames);
			free(skip_ifnames);
#if defined(RTCONFIG_AMAS_WDS) && defined(RTCONFIG_BHCOST_OPT)
			//restart or boot
			nvram_set("amas_wds","0");
			nvram_set("amas_qca_mode","-1");
#endif			
		}
#endif	/* RTCONFIG_AMAS */
	}

#ifdef RTCONFIG_WIFI_SON
#ifdef RTCONFIG_AMAS
	if(!nvram_match("wifison_ready", "1"))
		goto skip_wifison;
#endif	/* RTCONFIG_AMAS */

	if(sw_mode() == SW_MODE_REPEATER)
		goto skip_wifison;

	int i;
	if(sw_mode() == SW_MODE_AP && !nvram_match("cfg_master", "1")) //router->ap
	{
		if(nvram_get_int("x_Setting"))
		{
			char stamac[512];
			char *tmp_pt;
			i=1;
			{
				tmp_pt = get_stamac(i, stamac, sizeof(stamac));
				if(!tmp_pt)
				{
					char *sta = get_staifname(i);
					if(nvram_match("wl1_country_code", "GB"))
					{
						_dprintf("=> RE: hold for cac timeout...\n");
						while(get_cac_state())
						{
							_dprintf(".");
						}
						_dprintf("exit!!!\n");
					}
					_dprintf("=> switch router to ap : create %s\n", sta);
					create_vap(sta, i, "sta");
					sleep(1);
					ifconfig(sta, IFUP, NULL, NULL);
				}
			}
		}
	}

	if(nvram_get_int("wl0.1_bss_enabled"))
	{
		eval("vconfig","set_name_type","DEV_PLUS_VID_NO_PAD");
#if defined(RTCONFIG_HIDDEN_BACKHAUL)
		char vif[10];		
 		sprintf(vif,"%s.%s",WIF_5G_BH,"55");
                eval("vconfig", "add",WIF_5G_BH,"55");
                eval("ifconfig",vif,"up");
               	eval("brctl","addif",BR_GUEST,vif);
#else
		eval("vconfig", "add","ath1","55");
		eval("ifconfig","ath1.55","up");
		eval("brctl","addif",BR_GUEST,"ath1.55");
#endif
#ifdef RTCONFIG_ETHBACKHAUL
#if defined(RTCONFIG_DETWAN)
 		if (get_role()==0) // CAP
                {
                        char vif[15];
			char *def_nic;
                        if (strcmp(CONFIGURED_WAN_NIC, DEFAULT_WAN_NIC)==0)
                                def_nic = DEFAULT_LAN_NIC;
                        else
                                def_nic = DEFAULT_WAN_NIC;
                
			eval("vconfig", "add",vif,"55");
			sprintf(vif,"%s.55",def_nic);
			eval("ifconfig",vif,"up");
			eval("brctl","addif",BR_GUEST,vif);
		}
		else
#endif
		{
			eval("vconfig", "add", MII_IFNAME, "55");
			eval("ifconfig", MII_IFNAME".55", "up");
			eval("brctl", "addif", BR_GUEST, MII_IFNAME".55");
		}
#endif
	}
	
#if defined(RTCONFIG_HIDDEN_BACKHAUL)
        if(strlen(nvram_safe_get("cfg_group")))
        {
		char vif[10],cipher[100];
		if(nvram_match("wl1.1_bss_enabled","0"))
		{
                	nvram_set("wl1.1_bss_enabled","1");
                	nvram_set("wl1.1_expire_tmp","0");
                	nvram_set("wl1.1_auth_mode_x","psk2");
			memset(cipher,0,sizeof(cipher));
			addXOR(nvram_get("cfg_group"),cipher);
                	setting_hash_ap(cipher,strlen(nvram_get("cfg_group")));
                	nvram_commit();
		}
        	eval("vconfig","set_name_type","DEV_PLUS_VID_NO_PAD");
        	if(check_bh(WIF_5G_BH))
        	{
			sprintf(vif,"%s.%s",WIF_5G_BH,"1");
			eval("vconfig", "add",WIF_5G_BH,"1");
			eval("ifconfig",vif,"up");
			eval("brctl","addif",nvram_safe_get("lan_ifname"),vif);
        	}
       		if(!nvram_match("cfg_master","1"))
        	{
               		if(check_bh(STA_5G))
                	{
				sprintf(vif,"%s.%s",STA_5G,"1");
				eval("vconfig", "add",STA_5G,"1");
				eval("ifconfig",vif,"up");
				eval("brctl","addif",nvram_safe_get("lan_ifname"),vif);
                	}
		}
		
        }
#endif
skip_wifison: 
#endif
	return ;
}

#if defined(RTCONFIG_NO_RELOAD_WIFI_DRV_IF_POSSIBLE)
static inline int need_to_reload_wifi_drv(void)
{
	if (__need_to_reload_wifi_drv() || IS_ATE_FACTORY_MODE())
		return 1;
	return 0;
}
#endif

#if SPF_VER >= SPF_VER_ID(12,5)
static void configure_service_class_0(void)
{
	static const int svc_class[] = { 1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10,
		11 ,12 ,13 ,14 ,15 ,16 ,17 ,18 ,19 ,20,
		121 ,122 ,123 ,124 ,125 ,126 ,127 ,128,
		-1
	};
	const int *p;
	char svc[8], *argv[] = { "wlanconfig", "wifi0", "service_class", "disable", svc, NULL };

	for (p = svc_class; *p > 0; ++p) {
		snprintf(svc, sizeof(svc), "%d", *p);
		_eval(argv, NULL, 0, NULL);
	}
}
#else
static inline void configure_service_class_0(void) { }
#endif

void fini_wl(void)
{
	int i;
	int b, radar_cnt = 0, radar_list[32];
	uint64_t radar_m;
#if !defined(RTCONFIG_QCA_WLAN_SCRIPTS)
	struct load_wifi_kmod_seq_s *wp;
#endif
	char ifname[IFNAMSIZ], vphy[IFNAMSIZ];
#if defined(RTCONFIG_CFG80211)
	char path_pid[sizeof("/var/run/hostapd_cli-athX.pid") + IFNAMSIZ];
#endif

	nvram_set("wlready", "0");	//reset wlreay=0 to avoid access in wifi driver and crash
	configure_service_class_0();

	/* Backup NOL list and use it when WiFi reload.
	 * It's reset at system start-up.
	 */
	for (b = 0, radar_m = 0; b < MAX_NR_WL_IF; ++b) {
		SKIP_ABSENT_BAND(b);

		if (!is_5g(b))
			continue;
		strlcpy(vphy, get_vphyifname(b), sizeof(vphy));
		radar_cnt = get_radar_channel_list(vphy, radar_list, ARRAY_SIZE(radar_list));
		for (i = 0; i < radar_cnt; ++i) {
			radar_m |= ch5g2bitmask(radar_list[i]);
		}
	}
	if (radar_m) {
		nvram_set("5g_nol_list", bitmask2chlist5g(radar_m, ","));
	} else {
		nvram_unset("5g_nol_list");
	}

#if !defined(RTCONFIG_WIFI_SON)
#if defined(RTCONFIG_QCA) && defined(RTCONFIG_SOC_IPQ40XX)
        if(nvram_match("x_Setting", "1") && nvram_match("restwifi_qis", "0")) //reduce the finish time of QIS 
        {
#if defined(RTCONFIG_BT_CONN)
		stop_bluetooth_service();
#endif
		eval("killall", "hostapd");
#if defined(RTCONFIG_AMAS)
		kill_wifi_wpa_supplicant(-1); // under AMAS, wpa_supplicant is enabled in default state
#endif
		nvram_set("restwifi_qis","1");
		nvram_commit_bg();
		bg=0;

#if defined(RTCONFIG_PRELINK)
		if (nvram_invmatch("smart_connect_x", "0"))
#endif
		{
                	return ;
		}
        }
	else
		bg=0;
#endif
#endif 

#if defined(RTCONFIG_SOC_IPQ53XX) && (SPF_VER >= SPF_VER_ID(12,5))
	killall("scs_tool", SIGTERM);
#endif

#if defined(RTCONFIG_QCA_LBD)
	stop_qca_lbd();
#endif

	stop_mcsd();

	dbG("fini_wl:destroy wi node\n");

#if (SPF_VER >= SPF_VER_ID(11,0))
	/* in MBSS case, send notification to allow deletion of transmitting VAP */
	for (i = 0; i < MAX_NR_WL_IF; ++i) {
		SKIP_ABSENT_BAND(i);

		strlcpy(ifname, get_vphyifname(i), sizeof(ifname));
		eval(IWPRIV, ifname, "wifi_down_ind", "1");
	}
#endif
#if defined(RTCONFIG_CFG80211)
	for (i = 0; i < MAX_NR_WL_IF; ++i) {
		SKIP_ABSENT_BAND(i);

		__get_wlifname(i, 0, ifname);
		snprintf(path_pid, sizeof(path_pid), "/var/run/hostapd_cli-%s.pid", ifname);
		if (f_exists(path_pid))
			kill_pidfile(path_pid);
	}
#endif

	deinit_all_vaps(1);

#if (SPF_VER >= SPF_VER_ID(11,0))
	for (i = 0; i < MAX_NR_WL_IF; ++i) {
		SKIP_ABSENT_BAND(i);

		strlcpy(ifname, get_vphyifname(i), sizeof(ifname));
		eval(IWPRIV, ifname, "wifi_down_ind", "0");
	}
#endif

	/* Delete all STA interfaces. */
	kill_wifi_wpa_supplicant(-1);
	for (i = 0; i < MAX_NR_WL_IF; ++i) {
		SKIP_ABSENT_BAND(i);

		strlcpy(ifname, get_staifname(i), sizeof(ifname));
		if (!iface_exist(ifname) || i == WL_60G_BAND)
			continue;

#if defined(RTCONFIG_AMAS)
		if (aimesh_re_node()) {
			remove_netdev_bled_if(get_wl_led_gpio_nv(i), get_staifname(i));
		}
#endif
		ifconfig(ifname, 0, NULL, NULL);
		destroy_vap(ifname);
	}

#ifdef RTCONFIG_MLO
        destroy_mld_vap();
#endif

	create_node=0;

	for (i = 0; i < WL_NR_BANDS; ++i) {
		SKIP_ABSENT_BAND(i);

		ifconfig(get_vphyifname(i), 0, NULL, NULL);
	}

	if (need_to_reload_wifi_drv()) {
#if !defined(RTCONFIG_QCA_WLAN_SCRIPTS)
		for (i = ARRAY_SIZE(load_wifi_kmod_seq)-1, wp = &load_wifi_kmod_seq[i]; i >= 0; --i, --wp) {
			if (!module_loaded(wp->kmod_name))
				continue;

			if (!(wp->flags & QWIFI_STICK)) {
				eval("rmmod", wp->kmod_name);
				if (wp->remove_sleep)
					sleep(wp->remove_sleep);
				if (module_loaded(wp->kmod_name)) {
					dbg("Unload %s failed!\n", wp->kmod_name);
				}
			}
		}
#else
		eval("unloadwifi.sh");
#endif
	}
}

static void chk_valid_country_code(char *country_code)
{
	if ((unsigned char)country_code[0]!=0xff)
	{
		//
	}
	else
	{
		strcpy(country_code, "DB");
	}
}

int get_mac_2g(unsigned char dst[])
{
	int bytes = 6;
	if (FRead(dst, OFFSET_MAC_ADDR_2G, bytes) < 0) {  // ET0/WAN is same as 2.4G
		_dprintf("%s: Fread Out of scope\n", __func__);
		return -1;
	}
	return 0;
}

int get_mac_5g(unsigned char dst[])
{
	int bytes = 6;
	if (FRead(dst, OFFSET_MAC_ADDR, bytes) < 0) { // ET1/LAN is same as 5G
		_dprintf("%s: Fread Out of scope\n", __func__);
		return -1;
	}
	return 0;
}

#if defined(RTCONFIG_HAS_5G_2)
int get_mac_5g_2(unsigned char dst[])
{
	int bytes = 6;
	if (FRead(dst, OFFSET_MAC_ADDR_5G_2, bytes) < 0) { // ET1/LAN is same as 5G
		_dprintf("%s: Fread Out of scope\n", __func__);
		return -1;
	}
	return 0;
}
#endif

#if defined(VZWAC1300) /* bad eeprom fix, temp workaround */
extern int update_qca_eeprom(unsigned int eeprom_size, unsigned int eeprom_csum_offset, unsigned int eeprom_offset, unsigned int data_offset, unsigned char *data, unsigned int len);
#endif
/**
 * Check @buf before setting it to @nv_name.
 * @return:
 * 	0:	success
 *     -1:	invalid parameter
 *   otherwise:	error
 */
static int sfunc_datecode(char *nv_name, unsigned char *buf)
{
	int year, month, day;
	char tmp[DATECODE_LENGTH + 1];

	if (!nv_name || *nv_name == '\0' || !buf)
		return -1;

	strlcpy(tmp, buf, 4 + 1);
	year = safe_atoi(tmp);
	strlcpy(tmp, buf + 4, 2 + 1);
	month = safe_atoi(tmp);
	strlcpy(tmp, buf + 6, 2 + 1);
	day = safe_atoi(tmp);

	if (year < 2018 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31) {
		nvram_set(nv_name, "");
		return 0;
	}

	nvram_set(nv_name, buf);
	return 0;
}

#if defined(RTCONFIG_COBRAND)
/**
 * Set a 8-bit unsigned integer to @nv_name.
 * @return:
 * 	0:	success
 *     -1:	invalid parameter
 *   otherwise:	error
 */
static int sfunc_u8(char *nv_name, unsigned char *buf)
{
	if (!nv_name || !buf)
		return -1;

	if (*buf != 0xFF)
		nvram_set_int(nv_name, *buf);
	else
		nvram_unset(nv_name);

	return 0;
}
#endif	/* RTCONFIG_COBRAND */

/* Set MAC address of band0, band1, ..., bandN to below nvram variables respectively.
 * et0macaddr, et1macaddr, ..., etNmacaddr
 * wl0macaddr, wl1macaddr, ..., wlNmacaddr
 */
void set_et0macaddr(char *mac_tbl[])
{
	int band;
	char *macaddr = mac_tbl[WL_2G_BAND], *macaddr5g = mac_tbl[WL_5G_BAND];
	char nv[sizeof("etXXXmacaddr")];
	unsigned char buffer[18] __attribute__((unused));
#if defined(RTCONFIG_SOC_IPQ8064) || defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_QCA_VAP_LOCALMAC)
#if defined(RTCONFIG_QCA_VAP_LOCALMAC)
	if (macaddr)
		nvram_set("wl0macaddr", macaddr);
	if (macaddr5g)
		nvram_set("wl1macaddr", macaddr5g);
#endif
	/* Hack et0macaddr/et1macaddr after MAC address checking of wl_mssid. */
	if (macaddr) {
		ether_atoe(macaddr, buffer);
		buffer[5] += 1;
		ether_etoa(buffer, macaddr);
	}
	if (macaddr5g) {
		ether_atoe(macaddr5g, buffer);
		buffer[5] += 1;
		ether_etoa(buffer, macaddr5g);
	}
#endif

#if defined(RTCONFIG_QCA953X) || defined(RTCONFIG_QCA956X) || defined(RTCONFIG_QCN550X)
	/* set et1macaddr the same as et0macaddr (for cpu connect to switch only use single RGMII) */
	strlcpy((char *)buffer, macaddr, sizeof(buffer));
	macaddr5g = (char *)buffer;
#endif

#if 0	// single band
	nvram_set("et0macaddr", macaddr);
	nvram_set("et1macaddr", macaddr);
#elif defined(RTAC59_CD6R) || defined(RTAC59_CD6N)
	/* SYNC WAN/LAN as 2.4G */
	nvram_set("et0macaddr", nvram_get("wl0macaddr"));
	nvram_set("et1macaddr", nvram_get("wl0macaddr"));
#elif defined(RTAC95U)
	/* SYNC LAN as 2.4G */
	if (macaddr)
		nvram_set("et0macaddr", macaddr);
	nvram_set("et1macaddr", nvram_get("wl0macaddr"));
#else
	//TODO: separate for different chipset solution
	if (macaddr && *macaddr != '\0')
		nvram_set("et0macaddr", macaddr);
	if (macaddr5g && *macaddr5g != '\0')
		nvram_set("et1macaddr", macaddr5g);
	for (band = 0; band < WL_NR_BANDS; ++band) {
		__absent_band(band);

		snprintf(nv, sizeof(nv), "wl%dmacaddr", band);
		nvram_set(nv, mac_tbl[band]);
		/* handled by old macaddr/macaddr5g logic. */
		if (band == WL_2G_BAND || band == WL_5G_BAND)
			continue;

		if (!mac_tbl[band] || *mac_tbl[band] == '\0')
			continue;

		snprintf(nv, sizeof(nv), "et%dmacaddr", band);
		nvram_set(nv, mac_tbl[band]);
	}
#endif
}

static const struct factory_var_s {
	char *nv_name;
	unsigned int factory_offset;
	unsigned int length;
	int (*set_func)(char *nv_name, unsigned char *buf);
} g_factory_var_tbl[] = {
	{ "HwId",	OFFSET_HWID, HWID_LENGTH, NULL },
	{ "HwVer",	OFFSET_HWVERSION, HWVERSION_LENGTH, NULL },
	{ "HwBom",	OFFSET_HWBOM, HWBOM_LENGTH, NULL },
	{ "DCode",	OFFSET_DATECODE, DATECODE_LENGTH, sfunc_datecode },
#if defined(RTCONFIG_COBRAND)
	{ "CoBrand",	OFFSET_HWCOBRAND, HWCOBRAND_LENGTH, sfunc_u8 },
#endif

	{ NULL, 0, 0, NULL }
};

static const unsigned int g_mac_offset_tbl[MAX_NR_WL_IF] = {
	[WL_2G_BAND]	= OFFSET_MAC_ADDR_2G,
	[WL_5G_BAND]	= OFFSET_MAC_ADDR,
#if defined(RTCONFIG_HAS_5G_2)
	[WL_5G_2_BAND]	= OFFSET_MAC_ADDR_5G_2,
#endif
#if defined(RTCONFIG_HAS_6G)
	[WL_6G_BAND]	= OFFSET_MAC_ADDR_6G,
#if defined(RTCONFIG_HAS_6G_2)
	[WL_6G_2_BAND]	= OFFSET_MAC_ADDR_6G_2,
#endif
#endif
};

void init_syspara(void)
{
#if defined(TUFBE6500) || defined(TUFBE9400)
	const char *real_productid = "";
#endif
	int band;
	unsigned char buffer[16];
	unsigned char *dst, *p;
	unsigned int bytes;
	char mac_buf[MAX_NR_WL_IF][sizeof("00:11:22:33:44:55")], *mac_tbl[MAX_NR_WL_IF];
	char *macaddr __attribute__((unused)) = mac_buf[WL_2G_BAND];
	char *macaddr2 __attribute__((unused)) = mac_buf[WL_5G_BAND];
	char country_code[FACTORY_COUNTRY_CODE_LEN+1];
	char pin[9];
	char productid[13];
	char fwver[8];
	char blver[32];
#ifdef RTCONFIG_ODMPID
#ifdef RTCONFIG_32BYTES_ODMPID
	char modelname[32];
#else
	char modelname[16];
#endif
#endif
#if defined(RTCONFIG_WIFI_DRV_DISABLE) /* for IPQ40XX */
	char disableWifiDrv;
#endif
	char ipaddr_lan[16];
	unsigned char factory_var_buf[256];
	const struct factory_var_s *pfv;

#if defined(RTCONFIG_ASUSCTRL)
	fix_location_code();
#endif

#if defined(RT4GAC53U) || defined(MAPAC1750)
	boot_version_ck();
#endif
	fix_caldata();
	set_basic_fw_name();

	/* /dev/mtd/2, RF parameters, starts from 0x40000 */
	dst = buffer;
	bytes = 6;
	memset(buffer, 0, sizeof(buffer));
	memset(country_code, 0, sizeof(country_code));
	memset(pin, 0, sizeof(pin));
	memset(productid, 0, sizeof(productid));
	memset(fwver, 0, sizeof(fwver));

	for (pfv = &g_factory_var_tbl[0]; pfv->nv_name && pfv->length; ++pfv) {
		if (pfv->length >= sizeof(factory_var_buf))
			continue;
		*factory_var_buf = 0xFF;
		if (FRead(factory_var_buf, pfv->factory_offset, pfv->length)) {
			nvram_set(pfv->nv_name, "");
			continue;
		}

		*(factory_var_buf + pfv->length) = '\0';
		if ((p = strchr(factory_var_buf, 0xFF)) != NULL)
			*p = '\0';
		if (*factory_var_buf == '\0' || *factory_var_buf == 0xFF) {
			nvram_set(pfv->nv_name, "");
			continue;
		}

		if (pfv->set_func) {
			pfv->set_func(pfv->nv_name, factory_var_buf);
		} else {
			nvram_set(pfv->nv_name, factory_var_buf);
		}
	}

	guess_hwid_by_wifi2_board_id();	/* TUF-BE6500/TUF-BE9400 */

	memset(mac_buf, 0, sizeof(mac_buf));
	for (band = 0; band < MAX_NR_WL_IF; ++band) {
		mac_tbl[band] = mac_buf[band];
		*mac_buf[band] = '\0';
		/* wlX_nband was not ready if DUT had been reset to default,
		 * SKIP_ABSENT_BAND()/absent_band() rely on those variables
		 * mustn't be used here.
		 */
		if (__absent_band(band))
			continue;
#if defined(RTCONFIG_WIGIG)
		if (band == WL_60G_BAND)
			continue;
#endif

		if (g_mac_offset_tbl[band] < MTD_FACTORY_BASE_ADDRESS) {
			_dprintf("%s: Invalid band %d MAC address offset 0x%x!\n",
				__func__, band, g_mac_offset_tbl[band]);
			continue;
		}
		// ET0/WAN is same as 2.4G
		// ET1/LAN is same as 5G
		if (FRead(dst, g_mac_offset_tbl[band], bytes) < 0) {
			_dprintf("READ band %d MAC address: Out of scope\n", band);
			continue;
		}
		if (buffer[0] != 0xff)
			ether_etoa(buffer, mac_tbl[band]);
		//_dprintf("band %d mac [%s]\n", band, mac_buf[band]);
	}

#if defined(RTCONFIG_QCA_VAP_LOCALMAC) || defined(RTCONFIG_SOC_IPQ53XX)
	nvram_set("wl_mssid", "1");
#else
	if (!mssid_mac_validate(macaddr) || !mssid_mac_validate(macaddr2))
		nvram_set("wl_mssid", "0");
	else
		nvram_set("wl_mssid", "1");
#if defined(RTAC58U)
	if (check_mid("Hydra")) {
		nvram_set("wl_mssid", "1"); // Hydra's MAC may not be multible of 4
	}
#endif
#endif

	set_et0macaddr(mac_tbl);

#if defined(VZWAC1300) /* bad eeprom fix, temp workaround */
	{
		unsigned char reg_dmn[4];
		FRead(reg_dmn, OFFSET_MAC_ADDR_2G+6, 4);
		if (reg_dmn[0]!=0 || reg_dmn[1]!=0 || reg_dmn[2]!=0 || reg_dmn[3]!=0) {
			_dprintf("******Fix non-zero reg_dmn of 2G!!!\n");
			memset(reg_dmn, 0x00, 4);
			update_qca_eeprom(QCA_2G_EEPROM_SIZE, QCA_2G_EEPROM_CSUM_OFFSET,
			ETH0_MAC_OFFSET & 0xF000, (ETH0_MAC_OFFSET & 0xFFF)+6, reg_dmn, 4);
		}
		FRead(reg_dmn, OFFSET_MAC_ADDR+6, 4);
		if (reg_dmn[0]!=0 || reg_dmn[1]!=0 || reg_dmn[2]!=0 || reg_dmn[3]!=0) {
			_dprintf("******Fix non-zero reg_dmn of 5G!!!\n");
			memset(reg_dmn, 0x00, 4);
			update_qca_eeprom(QCA_5G_EEPROM_SIZE, QCA_5G_EEPROM_CSUM_OFFSET,
			ETH1_MAC_OFFSET & 0xF000, (ETH1_MAC_OFFSET & 0xFFF)+6, reg_dmn, 4);
		}
	}
#endif
	country_code[0] = '\0';
	dst = (unsigned char*) country_code;
	bytes = FACTORY_COUNTRY_CODE_LEN;
	if (FRead(dst, OFFSET_COUNTRY_CODE, bytes)<0)
	{
		_dprintf("READ ASUS country code: Out of scope\n");
		nvram_set("wl_country_code", "DB");
	}
	else
	{
		int band;
		char prefix[sizeof("wlXXX_")];

		dst[FACTORY_COUNTRY_CODE_LEN]='\0';
		chk_valid_country_code(country_code);
		nvram_set("wl_country_code", country_code);
		for (band = 0; band < WL_NR_BANDS; ++band) {
			if (__absent_band(band))
				continue;

			snprintf(prefix, sizeof(prefix), "wl%d_", band);
			nvram_pf_set(prefix, "country_code", country_code);
		}
	}

	/* reserved for Ralink. used as ASUS pin code. */
	dst = (char *)pin;
	bytes = 8;
	if (FRead(dst, OFFSET_PIN_CODE, bytes) < 0) {
		_dprintf("READ ASUS pin code: Out of scope\n");
		nvram_set("wl_pin_code", "");
	} else {
		if (((unsigned char)pin[0] == 0xff)
		 || !strcmp(pin, "12345670")
		 || !strcmp(pin, "12345678")) {
			char devPwd[9];
			nvram_set("secret_code", wps_gen_pin(devPwd, sizeof(devPwd)) ? devPwd : "12345670");
		}
		else
			nvram_set("secret_code", pin);
	}

#if defined(RTCONFIG_FITFDT)
	nvram_set("firmver", rt_version);
	nvram_set("productid", rt_buildname);
	strncpy(productid, rt_buildname, 12);
#else
	dst = buffer;
	bytes = 16;
	if (linuxRead(dst, 0x20, bytes) < 0) {	/* The "linux" MTD partition, offset 0x20. */
		fprintf(stderr, "READ firmware header: Out of scope\n");
		nvram_set("productid", "unknown");
		nvram_set("firmver", "unknown");
	} else {
		strncpy(productid, buffer + 4, 12);
		productid[12] = 0;
		snprintf(fwver, sizeof(fwver), "%d.%d.%d.%d", buffer[0], buffer[1], buffer[2],
			buffer[3]);
#if defined(TUFBE6500) || defined(TUFBE9400)
		real_productid = get_real_productid();
		if (*real_productid != '\0' && strcmp(productid, real_productid))
			strlcpy(productid, real_productid, sizeof(productid));
#endif
		nvram_set("productid", trim_r(productid));
		nvram_set("firmver", trim_r(fwver));
	}
#endif

#if defined(OFFSET_MODEL_DESC)
	if (1) {
		unsigned char buf[MAX_MODEL_DESC_SIZE+1];

		nvram_unset("ispctrl_desc");
		if (!nvram_match("isp_wanup_check", "0") || nvram_match("x_Setting", "1"))
			nvram_unset("ispled_shot");
		if (FRead(buf, OFFSET_MODEL_DESC, MAX_MODEL_DESC_SIZE) == 0) {
			int i;
			for (i = 0; i < MAX_MODEL_DESC_SIZE; i++) {
				if (buf[i] == 0xff || buf[i] == 0x00)
					break;
			}
			buf[i] = '\0';
			if (buf[0] != '\0')
				nvram_set("ispctrl_desc", buf);
		}
	}
#endif

#if defined(RTCONFIG_TCODE)
	/* Territory code */
	memset(buffer, 0, sizeof(buffer));
	if (FRead(buffer, OFFSET_TERRITORY_CODE, 5) < 0) {
		_dprintf("READ ASUS territory code: Out of scope\n");
		nvram_unset("territory_code");
	} else {
		/* [A-Z][A-Z]/[0-9][0-9] */
		if (buffer[2] != '/' ||
		    !isupper(buffer[0]) || !isupper(buffer[1]) ||
		    !isdigit(buffer[3]) || !isdigit(buffer[4]))
		{
			nvram_unset("territory_code");
		} else {
#if defined(MAPAC2200) || defined(MAPAC1300)
			if(strcmp(country_code, "CA") == 0 && strcmp(buffer, "US/01") == 0)
				strcpy(buffer, "CA/01");
#endif
#if defined(BD4_OD) /* workaround for old outdoor AU */
			if(strcmp(country_code, "AU") == 0 && strcmp(buffer, "AU/01") == 0) {
				char *newCtry = "US";
				nvram_set("wl_country_code", newCtry);
				nvram_set("wl0_country_code", newCtry);
				nvram_set("wl1_country_code", newCtry);
				FWrite(newCtry, OFFSET_COUNTRY_CODE, 2);
				strcpy(buffer, "AA/01");
				FWrite(buffer, OFFSET_TERRITORY_CODE, 5);
			}
#endif
			nvram_set("territory_code", buffer);
		}
	}

#if defined(RTCONFIG_ASUSCTRL)
	asus_ctrl_sku_check();
#endif

	/* PSK */
	memset(buffer, 0, sizeof(buffer));
	if (FRead(buffer, OFFSET_PSK, 14) < 0) {
		_dprintf("READ ASUS PSK: Out of scope\n");
		nvram_set("wifi_psk", "");
	} else {
		if ((buffer[0] == 0xff)|| !strcmp(buffer,"NONE"))
			nvram_set("wifi_psk", "");
		else
			nvram_set("wifi_psk", buffer);
	}
#if defined(RTAC58U) || defined(RTAC59U)
	if (!strncmp(nvram_safe_get("territory_code"), "CX/01", 5)
	 || !strncmp(nvram_safe_get("territory_code"), "CX/05", 5))
		nvram_set("wifi_psk", nvram_safe_get("secret_code"));
#endif
#endif

	memset(buffer, 0, sizeof(buffer));
	FRead(buffer, OFFSET_BOOT_VER, 4);
	snprintf(blver, sizeof(blver), "%s-0%c-0%c-0%c-0%c", trim_r(productid), buffer[0],
		buffer[1], buffer[2], buffer[3]);
	nvram_set("blver", trim_r(blver));

	_dprintf("bootloader version: %s\n", nvram_safe_get("blver"));
	_dprintf("firmware version: %s\n", nvram_safe_get("firmver"));

	nvram_set("wl1_txbf_en", "0");

#ifdef RTCONFIG_ODMPID
#ifdef RTCONFIG_32BYTES_ODMPID
	FRead(modelname, OFFSET_32BYTES_ODMPID, 32);
	modelname[31] = '\0';
	if (modelname[0] != 0 && (unsigned char)(modelname[0]) != 0xff
	    && is_valid_hostname(modelname)
	    && strcmp(modelname, "ASUS")) {
		nvram_set("odmpid", modelname);
	} else
#endif
	{
		FRead(modelname, OFFSET_ODMPID, 16);
		modelname[15] = '\0';
		if (modelname[0] != 0 && (unsigned char)(modelname[0]) != 0xff
		    && is_valid_hostname(modelname)
		    && strcmp(modelname, "ASUS")) {
			nvram_set("odmpid", modelname);
		} else
			nvram_unset("odmpid");
	}
#else
	nvram_unset("odmpid");
#endif

#if defined(MAPAC2200) /* for Lyra */
	nvram_set("odmpid", "Lyra");
#elif defined(MAPAC1300)
	nvram_set("odmpid", "Lyra_Mini");
#elif defined(VZWAC1300)
	nvram_set("odmpid", "ASUSMESH-AC1300");
#endif

#if defined(TUFBE6500) || defined(TUFBE9400)
	real_productid = get_real_productid();
	if (*real_productid != '\0' && strcmp(rt_buildname, real_productid))
		rt_buildname = real_productid;
#endif
	nvram_set("firmver", rt_version);
	nvram_set("productid", rt_buildname);

#if !defined(RTCONFIG_TCODE) || defined(RPAC66)  || defined(RPAC51) // move the verification later bcz TCODE/LOC
	verify_ctl_table();
#endif

#ifdef RTCONFIG_QCA_PLC_UTILS
	getPLC_MAC(macaddr);
	nvram_set("plc_macaddr", macaddr);
#elif defined(RTCONFIG_QCA_PLC2)
#if defined(PLAX56_XP4)
	strcpy(macaddr2, nvram_get("wl1macaddr"));
	if(ether_atoe(macaddr2, buffer)) {
		buffer[5] += 2;		/* PLC MAC = 5G MAC + 2 */
		ether_etoa(buffer, macaddr2);
		nvram_set("plc_macaddr", macaddr2);
	}
#endif	/* PLAX56_XP4 */
#endif

#ifdef RTCONFIG_DEFAULT_AP_MODE
	char dhcp = '0';

	if (FRead(&dhcp, OFFSET_FORCE_DISABLE_DHCP, 1) < 0) {
		_dprintf("READ Disable DHCP: Out of scope\n");
	} else {
		if (dhcp == '1')
			nvram_set("ate_flag", "1");
		else
			nvram_set("ate_flag", "0");
	}
#endif

#ifdef RTCONFIG_AMAS
	char bdl;
#ifdef RTCONFIG_PRELINK
	char bdlkey_buf[CFGSYNC_GROUPID_LEN+1];
#endif
	if (FRead(&bdl, OFFSET_AMAS_BUNDLE_FLAG, 1) < 0) {
		_dprintf("READ AiMesh bundle flag: Out of scope\n");
	} else {
		if ((bdl > AB_FLAG_NONE) && (bdl < AB_FLAG_MAX))
			nvram_set_int("amas_bdl", bdl);
		else
			nvram_unset("amas_bdl");
	}
#ifdef RTCONFIG_PRELINK
	if (FRead(bdlkey_buf, OFFSET_AMAS_BUNDLE_KEY, CFGSYNC_GROUPID_LEN) < 0) {
		_dprintf("READ AiMesh bundle key: Out of scope\n");
	} else {
		if (!is_valid_group_id(bdlkey_buf)) {
			bdlkey_buf[0]='\0';
			nvram_unset("amas_bdlkey");
		}
		else {
			bdlkey_buf[CFGSYNC_GROUPID_LEN]='\0';
			nvram_set("amas_bdlkey", bdlkey_buf);
		}
	}
#endif
#endif

#if defined(RTCONFIG_WIFI_DRV_DISABLE) /* for IPQ40XX */
	if (FRead(&disableWifiDrv, OFFSET_DISABLE_WIFI_DRV, 1) < 0) {
		_dprintf("Out of scope\n");
	} else {
		if ((disableWifiDrv == 'Y') || (disableWifiDrv == 'y'))
			nvram_set("disableWifiDrv_fac", "1");
	}
#endif

	FRead(ipaddr_lan, OFFSET_IPADDR_LAN, sizeof(ipaddr_lan));
	ipaddr_lan[sizeof(ipaddr_lan)-1] = '\0';
	if ((unsigned char)(ipaddr_lan[0]) != 0xff && !illegal_ipv4_address(ipaddr_lan))
		nvram_set("IpAddr_Lan", ipaddr_lan);
	else
		nvram_unset("IpAddr_Lan");

	getSN();
#if defined(RTCONFIG_HAS_5G)
	/* Clear NOL list if it was saved. */
	nvram_unset("5g_nol_list");
#endif
}

#ifdef RTCONFIG_ATEUSB3_FORCE
void post_syspara(void)
{
	unsigned char buffer[16];
	buffer[0]='0';
	if (FRead(&buffer[0], OFFSET_FORCE_USB3, 1) < 0) {
		fprintf(stderr, "READ FORCE_USB3 address: Out of scope\n");
	}
	if (buffer[0]=='1')
		nvram_set("usb_usb3", "1");
}
#endif

void generate_wl_para(int unit, int subunit)
{
}

#if defined(RTCONFIG_SOC_QCA9557) || defined(RTCONFIG_QCA953X) || defined(RTCONFIG_QCA956X) || defined(RTCONFIG_QCN550X) || defined(RTCONFIG_SOC_IPQ40XX)
// only qca solution can reload it dynamically
// only happened when qca_sfe=1
// only loaded when unloaded, and unloaded when loaded
// in restart_firewall for fw_pt_l2tp/fw_pt_ipsec
// in restart_qos for qos_enable
// in restart_wireless for wlx_mrate_x, etc
void reinit_sfe(int unit)
{
	int prim_unit = wan_primary_ifunit();
	int act = 1,i;	/* -1/0/otherwise: ignore/remove sfe/load sfe */
	struct load_nat_accel_kmod_seq_s *p = &load_nat_accel_kmod_seq[0];
#if defined(RTCONFIG_DUALWAN)
	int nat_x = -1;
	char nat_x_str[] = "wanX_nat_xXXXXXX";
#endif
	int sfe_dev;
	char dev_num[6];
#if defined(RTCONFIG_SOC_IPQ40XX) && defined(RTCONFIG_BWDPI)
	int handle_bwdpi = 0;
#endif

	act = nvram_get_int("qca_sfe");	

	/* If non-A.QoS is enabled, disable sfe. */
	if (IS_NON_AQOS())
		act = 0;

#if defined(RTCONFIG_QCA956X) && defined(RTCONFIG_BWDPI)
	/* For MAP-AC1750, not to integrate fast-path in stage 1 */
	if (check_bwdpi_nvram_setting() == 1)
		act = 0;
#endif

	/* URL filter and keyword filter are not compatible to IPQ806x NSS NAT acceleration and IPQ40XX shortcut-fe.
	 * But they do works with QCA955X shortcut-fe.
	 */
	if ((nvram_match("url_enable_x", "1") && !nvram_match("url_rulelist", "")) ||
	    (nvram_match("keyword_enable_x", "1") && !nvram_match("keyword_rulelist", "")))
		act = 0;

	if (act > 0) {
#if defined(RTCONFIG_DUALWAN)
		/* Load Balance */
		if (nvram_match("wans_mode", "lb"))
			act = 0;
#if !defined(RT4GAC53U) && !defined(RT4GAC56) /* for Gobi */
		/* Fail Over to 3G/4G */
		else if (/*(unit == -1 && get_wans_dualwan() & WANSCAP_USB) ||*/
			 (unit == prim_unit && get_dualwan_by_unit(unit) == WANS_DUALWAN_IF_USB))
			act = 0;
#endif
		/* Enable NAT */
		else {
			snprintf(nat_x_str, sizeof(nat_x_str), "wan%d_nat_x", unit);
			nat_x = nvram_get_int(nat_x_str);
			if (unit == prim_unit && !nat_x)
				act = 0;
			else if (unit != prim_unit)
				act = -1;
		}
#else
		if (!is_nat_enabled())
			act = 0;
		if (dualwan_unit__usbif(prim_unit))
			act = 0;
#endif
	}

#ifdef RTCONFIG_WIFI_SON
	if (nvram_match("wifison_ready", "1") &&
	   ((sw_mode() != SW_MODE_ROUTER) && !nvram_match("cfg_master", "1")))
		act = 0;
#endif

#if defined(RTCONFIG_DUALWAN)
	_dprintf("%s:DUALWAN: unit %d,%d type %d nat_x %d qos %d wans_mode %s: action %d.\n",
		__func__, unit, prim_unit, get_dualwan_by_unit(unit),
		nat_x, nvram_get_int("qos_enable"), nvram_safe_get("wans_mode"), act);
#else
	_dprintf("%s:WAN: unit %d,%d type %d nat_x %d qos %d: action %d.\n",
		__func__, unit, prim_unit, get_dualwan_by_unit(unit),
		nvram_get_int("wan0_nat_x"), nvram_get_int("qos_enable"), act);
#endif

	if (act < 0)
		return;

#if defined(RTCONFIG_SOC_IPQ40XX) && defined(RTCONFIG_BWDPI)
	if ((act == 0) && (check_bwdpi_nvram_setting() == 1)) {
		handle_bwdpi = 1;
		act = 1;
	}
#endif

	for (i = 0, p = &load_nat_accel_kmod_seq[i]; i < ARRAY_SIZE(load_nat_accel_kmod_seq); ++i, ++p) {
		if (!act) {
			/* remove sfe */
			if (!module_loaded(p->kmod_name))
				continue;

			modprobe_r(p->kmod_name);

			if (p->remove_sleep)
				sleep(p->load_sleep);

		} else {
#if defined(RTCONFIG_SOC_IPQ40XX) && defined(RTCONFIG_BWDPI)
			if (!strcmp(p->kmod_name,"shortcut_fe_cm") && handle_bwdpi) {
					stop_dpi_engine_service(1); /* DPI use SFE symbol, we must remove DPI at first */
					modprobe_r("shortcut_fe_cm");
			}
#endif
			/* load sfe */
			if (module_loaded(p->kmod_name))
				continue;
			
#if defined(RTCONFIG_SOC_IPQ40XX) && defined(RTCONFIG_BWDPI)
			if (!strcmp(p->kmod_name,"shortcut_fe_cm") && handle_bwdpi) {
				modprobe(p->kmod_name, "skip_sfe=1");
				start_dpi_engine_service();
			}
			else
#endif
				modprobe(p->kmod_name);
			if (p->load_sleep)
				sleep(p->load_sleep);
		}
	}

	if (!act) {
		unlink("/dev/sfe4");
		unlink("/dev/sfe6");
	} else {
		if (( f_read_string("/sys/sfe_ipv4/debug_dev", dev_num, sizeof(dev_num)) && \
				(sfe_dev = atoi(dev_num)) > 0)) {
			unlink("/dev/sfe4");
			__mknod("/dev/sfe4", S_IFCHR | 0660, makedev(sfe_dev, 0));
		}
		if (( f_read_string("/sys/sfe_ipv6/debug_dev", dev_num, sizeof(dev_num)) && \
				(sfe_dev = atoi(dev_num)) > 0)) {
			unlink("/dev/sfe6");
			__mknod("/dev/sfe6", S_IFCHR | 0660, makedev(sfe_dev, 0));
		}
	}
}
#endif	/* RTCONFIG_SOC_QCA9557 || RTCONFIG_QCA953X || RTCONFIG_QCA956X || RTCONFIG_QCN550X || RTCONFIG_SOC_IPQ40XX */

#if defined(RTCONFIG_SOC_IPQ8064) || defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_SOC_IPQ60XX) \
 || defined(RTCONFIG_SOC_IPQ50XX) || defined(RTCONFIG_SOC_IPQ53XX)
#define IPV46_CONN	4096
/**
 * Tell caller whether ecm should be loaded (non-zero value) or unloaded (zero value).
 * @return
 * 	0:	ecm should be unloaded
 *  otherwise:	ecm should be loaded
 */
int ecm_selection(void)
{
	int act = nvram_get_int("qca_sfe");	/* -1/0/otherwise: ignore/remove ecm/load ecm */

#if defined(RTCONFIG_SOC_IPQ53XX) && defined(RTCONFIG_VPN_FUSION_MERLIN)
	/* Keep experimental VPN policy and kill-switch traffic in the Linux path. */
	act = 0;
#endif

	/* Don't load ecm if NAT is not enabled. */
	if (!is_nat_enabled())
		act = 0;

	/* If QoS is enabled, disable ecm.
	 * Including AiProtection due to BWDPI dep. module is compatible to IPQ806x NSS NAT acceleration.
	 * dpi engine doesn't integrate QCA Hardware QoS, so A.QoS needs to "echo 1 >  ecm_nss_ipv[4/6]/stop"
	 */
	if (nvram_get_int("qos_enable") == 1)
		act = 0;

#if defined(RTCONFIG_SOC_IPQ53XX) && defined(RTCONFIG_BWDPI)
	/* For IPQ5332, not to integrate fast-path in stage 1 */
	if (check_bwdpi_nvram_setting() == 1)
		act = 0;
#endif

	/* If IPSec is enabled, disable ecm. */
	if (!nvram_match("qca_hwnat_ipsec", "1")) {
		if (nvram_get_int("ipsec_server_enable") == 1 || nvram_get_int("ipsec_client_enable") == 1
#ifdef RTCONFIG_INSTANT_GUARD
		 || nvram_get_int("ipsec_ig_enable") == 1
#endif
	 )
			act = 0;
	}

	/* URL filter and keyword filter are not compatible to IPQ806x NSS NAT acceleration and IPQ40XX shortcut-fe.
	 * But they do works with QCA955X shortcut-fe.
	 */
	if ((nvram_match("url_enable_x", "1") && !nvram_match("url_rulelist", "")) ||
	    (nvram_match("keyword_enable_x", "1") && !nvram_match("keyword_rulelist", "")))
		act = 0;


	if (act > 0) {
		/* FIXME: IPTV, ISP profile, USB modem, etc. */
	}

	dbg("%s: nat_x %d qos %d: action %d.\n", __func__,
		nvram_get_int("wan0_nat_x"), nvram_get_int("qos_enable"), act);

	return act? 1 : 0;
}

void init_ecm(void)
{
	/* Always enable NSS RPS */
	f_write_string("/proc/sys/dev/nss/general/rps", "1", 0, 0);

	/* Turn off bridge firewall first. */
	f_write_string("/proc/sys/net/bridge/bridge-nf-call-ip6tables", "0", 0, 0);
	f_write_string("/proc/sys/net/bridge/bridge-nf-call-iptables", "0", 0, 0);
}

// ecm kernel module must be loaded before bonding interface creation!
// only qca solution can reload it dynamically
// only happened when qca_sfe=1
// only loaded when unloaded, and unloaded when loaded
// in restart_firewall for fw_pt_l2tp/fw_pt_ipsec
// in restart_qos for qos_enable
// in restart_wireless for wlx_mrate_x, etc
void reinit_ecm(int unit)
{
	int i, act, r1, r2, accel_type __attribute__((unused));
	int cur_accel_type __attribute__((unused)), reload_ecm_wifi_plugin;
	char *modprobe_argv[10] = { "modprobe", NULL }, **v;
	char val[4] = "0";
	struct load_nat_accel_kmod_seq_s *p = &load_nat_accel_kmod_seq[0];
#if defined(RTCONFIG_SOC_IPQ8064)
	const char *v4_stop_fn = "/sys/kernel/debug/ecm/ecm_nss_ipv4/stop", *v6_stop_fn = "/sys/kernel/debug/ecm/ecm_nss_ipv6/stop";
#elif defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_SOC_IPQ60XX) || defined(RTCONFIG_SOC_IPQ50XX) \
   || defined(RTCONFIG_SOC_IPQ53XX)
	const char *v4_stop_fn = "/sys/kernel/debug/ecm/front_end_ipv4_stop", *v6_stop_fn = "/sys/kernel/debug/ecm/front_end_ipv6_stop";
#endif
#if defined(RTCONFIG_SOC_IPQ53XX) \
 && defined(RTCONFIG_WIFI_IPQ53XX_QCN6274)
	char fe_sel[sizeof("front_end_selection=XXXXXX")];
#endif
	act = !!ecm_selection();

	/* Always load ecm and related kernel modules.
	 * If hardware NAT should not enabled, stop ecm instead.
	 */
	for (i = 0, p = &load_nat_accel_kmod_seq[i]; i < ARRAY_SIZE(load_nat_accel_kmod_seq); ++i, ++p) {
		accel_type = nat_accel_type(p->kmod_name);
		cur_accel_type = cur_nat_accel_type(p->kmod_name);
		reload_ecm_wifi_plugin = 0;
		if (module_loaded(p->kmod_name)) {
#if defined(RTCONFIG_SOC_IPQ53XX) \
 && defined(RTCONFIG_WIFI_IPQ53XX_QCN6274)
			if ((accel_type <= 0 && cur_accel_type <= 0)
			 || (accel_type >  0 && accel_type == cur_accel_type))
				continue;

			/* Switch NAT accel. type by sysfs attr. */
			if (f_exists(FRONT_END_SELECTION_ATTR)) {
				snprintf(val, sizeof(val), "%d", accel_type);
				f_write_string(FRONT_END_SELECTION_ATTR, val, 0, 0);
				if (accel_type == cur_nat_accel_type(p->kmod_name))
					continue;
			}

			/* Switch NAT accel. type by reloading modules. */
			dbg("%s: Unload %s (accel_type %d, cur_accel_type %d)\n",
				__func__, p->kmod_name, accel_type, cur_accel_type);
			if (module_loaded("ecm_wifi_plugin"))
				reload_ecm_wifi_plugin = 1;
			/* If ecm_wifi_plug or ecm is removed by "modprobe -r"
			 * command, qca_nss_ppe_vxlanmgr is removed automatically
			 * too. But it doesn't deconfigure some resource. If the
			 * module was removed, it can't be loaded again due to
			 * initialization of the resource failed.
			 */
			if (reload_ecm_wifi_plugin)
				eval("rmmod", "ecm_wifi_plugin");
			eval("rmmod", p->kmod_name);
			if (p->load_sleep)
				sleep(p->load_sleep);
#else
			continue;
#endif
		}

		if (accel_type > 0) {
			dbg("%s: Load %s, accel_type %s\n", __func__,
				p->kmod_name, nat_accel_type_name(accel_type));
		} else {
			dbg("%s: Load %s\n", __func__, p->kmod_name);
		}
		v = &modprobe_argv[1];
		*v++ = p->kmod_name;
#if defined(RTCONFIG_SOC_IPQ53XX) \
 && defined(RTCONFIG_WIFI_IPQ53XX_QCN6274)
		if (accel_type > 0) {
			snprintf(fe_sel, sizeof(fe_sel), "front_end_selection=%d",
				accel_type);
			*v++ = fe_sel;
		}
#endif

		*v++ = NULL;
		_eval(modprobe_argv, DBGOUT, 0, NULL);
		if (p->load_sleep)
			sleep(p->load_sleep);
		if (reload_ecm_wifi_plugin)
			modprobe("ecm_wifi_plugin");
	}
	snprintf(val, sizeof(val), "%d", !act);


	/* resume/stop ecm. */
	r1 = f_write_string(v4_stop_fn, val, 0, 0);
	r2 = f_write_string(v6_stop_fn, val, 0, 0);

	if (!act) {
		f_write_string("/sys/kernel/debug/ecm/ecm_db/defunct_all", "1", 0, 0);
	}
	if (r1 <= 0 || r2 <= 0) {
		dbg("%s ecm failed. (return %d/%d)\n", act? "Resume" : "Stop", r1, r2);
	}

	post_ecm();
}

void post_ecm(void)
{
	int act, redir, r;
	char val[4], tmp[16], ipv46_conn[16];

	snprintf(ipv46_conn, sizeof(ipv46_conn), "%d", IPV46_CONN);
	*tmp = '\0';
	r = f_read_string("/proc/sys/dev/nss/ipv4cfg/ipv4_conn", tmp, sizeof(tmp));
	if (r > 0 && safe_atoi(tmp) != IPV46_CONN)
		r = f_write_string("/proc/sys/dev/nss/ipv4cfg/ipv4_conn", ipv46_conn, 0, 0);
	*tmp = '\0';
	r = f_read_string("/proc/sys/dev/nss/ipv6cfg/ipv6_conn", tmp, sizeof(tmp));
	if (r > 0 && safe_atoi(tmp) != IPV46_CONN)
		r = f_write_string("/proc/sys/dev/nss/ipv6cfg/ipv6_conn", ipv46_conn, 0, 0);

	redir = act = ecm_selection();
#if defined(RTCONFIG_COOVACHILLI)
	if (nvram_match("captive_portal_enable","on") || nvram_match("captive_portal_adv_enable", "on"))
		redir = 0;
#endif
	snprintf(val, sizeof(val), "%d", !!redir);
	f_write_string("/proc/sys/dev/nss/general/redirect", val, 0, 0);

#if defined(RTCONFIG_SOC_IPQ8064)
	/* Limit ecm db usage */
	if (act) {
		f_write_string("/sys/kernel/debug/ecm/ecm_nss_ipv4/db_limit_mode", "1", 0, 0);
		f_write_string("/sys/kernel/debug/ecm/ecm_nss_ipv6/db_limit_mode", "1", 0, 0);
	}
#endif

	if (__post_ecm)
		__post_ecm();
}
#endif	/* IPQ8064 || IPQ8074 || IPQ60XX || IPQ50XX || IPQ53XX */

/** Whether wireless interface works.
 * @ifname:
 * @ate_wl_band:	ATE band number.  1:2G, 2:5G-1, 3:5G-2, etc
 * @return:
 * 	0:	Wireless interface absent or not work.
 *  otherwizse:	Wireless interface exist and works.
 */
int wl_exist(char *ifname, int ate_wl_band)
{
	int ret = 0, band = ate_wl_band - 1;
	char phy[sizeof("/sys/class/ieee80211/XXX") + IFNAMSIZ];

	if (!ifname || band < WL_2G_BAND || band >= WL_NR_BANDS || __absent_band(band)) {
		dbg("%s: invalid parameter? (ifname %p, band %d)\n", __func__, ifname, band);
		return 0;
	}

	if (band == WL_60G_BAND) {
		snprintf(phy, sizeof(phy), "/sys/class/ieee80211/%s", get_vphyifname(band));
		if (!f_exists(phy) && !d_exists(phy))
			return 0;
	} else if (!iface_exist(get_vphyifname(band))) {
		dbg("%s: %s not found!\n", __func__, get_vphyifname(band));
		return 0;
	}

	if (!iface_exist(get_wififname(band))) {
		dbg("%s: %s not found!\n", __func__, get_wififname(band));
		return 0;
	}

	/* "band" ranges from 1~3 (ate.c), but WL_2G_BAND is 0 */
	switch (band) {
	case WL_2G_BAND:	/* fall-through */
	case WL_5G_BAND:	/* fall-through */
	case WL_5G_2_BAND:	/* fall-through */
	case WL_6G_BAND:	/* fall-through */
	case WL_6G_2_BAND:	/* fall-through */
		ret = eval(IWPRIV, ifname, "get_driver_caps");
		dbg("eval(%s, %s, get_driver_caps) ret(%d)\n", IWPRIV, ifname, ret);
		break;
	case WL_60G_BAND:
		ret = eval("iw", ifname, "info");
		dbg("eval(%s, %s, info) ret(%d)\n", "iw", ifname, ret);
		break;
	};

	return (ret == 0);
}

void
set_wan_tag(char *interface)
{
#if defined(RTCONFIG_MULTICAST_IPTV)
	const int sw_iptv = sw_based_iptv();
#endif
	int model, wan_vid;
	char wan_dev[10], port_id[7];

	model = get_model();
	wan_vid = nvram_get_int("switch_wan0tagid");

	snprintf(wan_dev, sizeof(wan_dev), "vlan%d", wan_vid);

	switch(model) {
	case MODEL_BRTAC828:
	case MODEL_RTAD7200:
		set_hwaddr(interface, nvram_safe_get("wan0_hwaddr"));
		/* fall-through */
	case MODEL_RTAC55U:
	case MODEL_RTAC55UHP:
	case MODEL_RT4GAC55U:
	case MODEL_RTN19:
	case MODEL_RTAC59U:
	case MODEL_RTAC59CD6R:
	case MODEL_RTAC59CD6N:
	case MODEL_RTAC58U:
	case MODEL_RT4GAC53U:
	case MODEL_RT4GAC56:
	case MODEL_RTAC82U:
	case MODEL_RTAC88N:
	case MODEL_MAPAC1750:
	case MODEL_RTAC95U:
		ifconfig(interface, IFUP, 0, 0);
		if(wan_vid) { /* config wan port */
			eval("vconfig", "rem", "vlan2");
			snprintf(port_id, sizeof(port_id), "%d", wan_vid);
			eval("vconfig", "add", interface, port_id);
		}
		/* Set Wan port PRIO */
		if(nvram_invmatch("switch_wan0prio", "0"))
			eval("vconfig", "set_egress_map", wan_dev, "0", nvram_get("switch_wan0prio"));
#if defined(RTAC58U) || defined(RTAC59U)
		if (nvram_match("switch_wantag", "stuff_fibre"))
			eval("vconfig", "set_egress_map", wan_dev, "3", "5");
#endif
		break;
	case MODEL_MAPAC1300:
	case MODEL_VZWAC1300:
	case MODEL_SHAC1300:
	case MODEL_MAPAC2200:
#if defined(RTCONFIG_SOC_IPQ40XX)
		if (!strcmp(nvram_safe_get("switch_wantag"), "movistar")) {
			ifconfig(interface, IFUP, 0, 0);
			snprintf(port_id, sizeof(port_id), "%d", wan_vid);
			eval("vconfig", "add", interface, port_id);
			detwan_set_def_vid(interface, 10, 0, 0); // sync with Barton's modification
		} else {
			detwan_set_def_vid(interface, wan_vid, 1, 0);
		}
#endif	/* RTCONFIG_SOC_IPQ40XX */
		break;
	}

#ifdef RTCONFIG_MULTICAST_IPTV
	{
		int iptv_vid, voip_vid, iptv_prio, voip_prio, switch_stb;
		int mang_vid, mang_prio;
		char prio_str[16];

		iptv_vid  = nvram_get_int("switch_wan1tagid") & 0x0fff;
		voip_vid  = nvram_get_int("switch_wan2tagid") & 0x0fff;
		iptv_prio = nvram_get_int("switch_wan1prio") & 0x7;
		voip_prio = nvram_get_int("switch_wan2prio") & 0x7;
		mang_vid  = nvram_get_int("switch_wan3tagid") & 0x0fff;
		mang_prio = nvram_get_int("switch_wan3prio") & 0x7;

		switch_stb = nvram_get_int("switch_stb_x");
		if (switch_stb >= 7) {
			system("rtkswitch 40 1");			/* admin all frames on all ports */
#if defined(RTCONFIG_SWITCH_RTL8370MB_PHY_QCA8033_X2) || \
    defined(RTCONFIG_SWITCH_RTL8370M_PHY_QCA8033_X2)
			/* Make sure the "admin all frames on all ports" is applied to Realtek switch. */
			system("rtkswitch 38 0");
#endif
			if(wan_vid) { /* config wan port */
				__setup_vlan(wan_vid, 0, 0x00000210);	/* config WAN & WAN_MAC port */
			}

			if (iptv_vid) { /* config IPTV on wan port */
				snprintf(wan_dev, sizeof(wan_dev), "vlan%d", iptv_vid);
				nvram_set("wan10_ifname", wan_dev);
				if (sw_iptv) {
					__setup_vlan(iptv_vid, iptv_prio, 0x00000210);	/* config WAN & WAN_MAC port */
				} else {
					snprintf(port_id, sizeof(port_id), "%d", iptv_vid);
					eval("vconfig", "add", interface, port_id);

					__setup_vlan(iptv_vid, iptv_prio, 0x00000210);	/* config WAN & WAN_MAC port */

					if (iptv_prio) { /* config priority */
						sprintf(prio_str, "%d", iptv_prio);
						eval("vconfig", "set_egress_map", wan_dev, "0", prio_str);
					}
				}
			}
		}
		if (switch_stb >= 8) {
			if (voip_vid) { /* config voip on wan port */
				snprintf(wan_dev, sizeof(wan_dev), "vlan%d", voip_vid);
				nvram_set("wan11_ifname", wan_dev);
				if (sw_iptv) {
					__setup_vlan(voip_vid, voip_prio, 0x00000210);	/* config WAN & WAN_MAC port */
				} else {
					snprintf(port_id, sizeof(port_id), "%d", voip_vid);
					eval("vconfig", "add", interface, port_id);

					__setup_vlan(voip_vid, voip_prio, 0x00000210);	/* config WAN & WAN_MAC port */

					if (voip_prio) { /* config priority */
						sprintf(prio_str, "%d", voip_prio);
						eval("vconfig", "set_egress_map", wan_dev, "0", prio_str);
					}
				}
			}
		}
		if (switch_stb >=9 ) {
			if (mang_vid) { /* config tr069 on wan port */
				snprintf(wan_dev, sizeof(wan_dev), "vlan%d", mang_vid);
				nvram_set("wan12_ifname", wan_dev);
				if (sw_iptv) {
					__setup_vlan(mang_vid, mang_prio, 0x00000210);	/* config WAN & WAN_MAC port */
				} else {
					snprintf(port_id, sizeof(port_id), "%d", mang_vid);
					eval("vconfig", "add", interface, port_id);

					__setup_vlan(mang_vid, mang_prio, 0x00000210);	/* config WAN & WAN_MAC port */

					if (mang_prio) { /* config priority */
						sprintf(prio_str, "%d", iptv_prio);
						eval("vconfig", "set_egress_map", wan_dev, "0", prio_str);
					}
				}
			}
		}
#if defined(RTCONFIG_DETWAN)
		if (nvram_get("switch_wantag") && nvram_match("switch_wantag", "movistar")) {
			char buf[128];
			sprintf(buf, "vlan%s", nvram_safe_get("switch_wan0tagid"));
			nvram_set("wan_ifnames", buf);
			nvram_set("wan0_ifname", buf);
		}
#endif
	}
#endif
}

int start_thermald(void)
{
#if defined(RTCONFIG_SOC_IPQ8064)
	char *conf = "/etc/thermal/ipq-thermald-8064.conf";
#elif defined(RTBE50)
	char *conf = "/etc/thermal/ipq-thermald-5312.conf";
#else
	char *conf = "/etc/thermal/thermald.conf";
#endif
	char **v, *thermald_argv[] = { "thermald", NULL, NULL, NULL, NULL };
	pid_t pid;

	v = &thermald_argv[1];
	if (conf && f_exists(conf)) {
		*v++ = "-c";
		*v++ = conf;
	}
	*v++ = NULL;

	return _eval(thermald_argv, NULL, 0, &pid);
}

#ifdef RTCONFIG_TAGGED_BASED_VLAN

/**/
void vlan_switch_accept_tagged(unsigned int port)
{
	char lanportset_str[16]={0};

	snprintf(lanportset_str, sizeof(lanportset_str), "%d", port);
	eval("rtkswitch","298",lanportset_str);
}
void vlan_switch_accept_untagged(unsigned int port)
{
	char lanportset_str[16]={0};

	snprintf(lanportset_str, sizeof(lanportset_str), "%d", port);
	eval("rtkswitch","299",lanportset_str);
}

/* set all ports accept all packets(tagged or untagged) */
void vlan_switch_accept_all(unsigned int port)
{
	char lanportset_str[16]={0};

	snprintf(lanportset_str, sizeof(lanportset_str), "%d", port);
	eval("rtkswitch","300",lanportset_str);
}

void vlan_switch_setup(int vlan_id, int vlan_prio, int lanportset)
{
	char vlan_id_str[12]={0},vlan_prio_str[12]={0};
	char lanportset_str[16]={0};

	snprintf(vlan_id_str, sizeof(vlan_id_str), "%d", vlan_id);
	snprintf(vlan_prio_str, sizeof(vlan_prio_str), "%d", vlan_prio);
	//lanportset |= ( 1<<15 );
	snprintf(lanportset_str, sizeof(lanportset_str), "0x%x", lanportset);

	eval("rtkswitch","301",vlan_id_str);
	eval("rtkswitch","302",vlan_prio_str);
	eval("rtkswitch","303",lanportset_str);
}

int vlan_switch_pvid_setup(int *pvid_list, int *pprio_list, int size)
{
	int i=0;
	char port_str[8]={0};
	char pvid_str[8]={0};
	char pprio_str[8]={0};

	if(!pvid_list && !pprio_list)
		return -1;

	for(i=0;i<size;i++)
	{
		memset(port_str,0,8);
		memset(pvid_str,0,8);
		memset(pprio_str,0,8);
		snprintf(port_str, sizeof(port_str), "%d", i);
		snprintf(pvid_str, sizeof(pvid_str), "%d", pvid_list[i]);
		snprintf(pprio_str, sizeof(pprio_str), "%d", pprio_list[i]);

		if(pvid_list[i] == 0 || pvid_list[i] == 1)
			continue;

		eval("rtkswitch","305",pvid_str);
		eval("rtkswitch","306",pprio_str);
		eval("rtkswitch","307",port_str);
	}

	return 0;
}

#if 0
void set_tagged_based_vlan_config(char *interface)
{
	char *nv, *nvp, *b;
	//char *enable, *vid, *priority, *portset, *wlmap, *subnet_name;
	//char *portset, *wlmap, *subnet_name;
	char *enable, *wanportset, *lanportset, *wl2gset, *wl5gset, *subnet_name, *vlan_name;
	int set_flag = (interface != NULL) ? 1 : 0;

	/* Clean some parameters for vlan */
	//clean_vlan_ifnames();

	printf("%s %d\n",__FUNCTION__,__LINE__);

	if (vlan_enable()) {
		nv = nvp = strdup(nvram_safe_get("vlan_rulelist"));

		if (nv) {
			//int vlan_tag = 4;
			int model;
			int br_index = 3;

			model = get_model();

			if (model != MODEL_BRTAC828){
				printf("model != MODEL_BRTAC828\n");
				return;
			}

			while ((b = strsep(&nvp, "<")) != NULL) {
				//int real_portset = 0;
				char tag_reg_val[7]={0}, vlan_id[5]={0},vlan_prio[2]={0}, lanportset_str[12]={0};
				//unsigned int vlan_entry_tmp = 0, tag_reg_val_tmp = 0;
				unsigned int wanportset_tmp=0,lanportset_tmp=0,wl2gset_tmp=0,wl5gset_tmp=0,wlset_tmp=0;
				int i = 0, vlan_id_tmp = 0,vlan_prio_tmp=0;
				int cpu_port = 0;
				

				if ((vstrsep(b, ">", &enable, &wanportset, &lanportset, &wl2gset, &wl5gset, &subnet_name, &vlan_name) != 7))
					continue;

				//_dprintf("%s: %s %s %s %s %s %s\n", __FUNCTION__, enable, vid, priority, portset, wlmap, subnet_name);
				printf("%s: %s %s %s %s %s %s %s\n", __FUNCTION__, enable, wanportset, lanportset
											, wl2gset, wl5gset, subnet_name, vlan_name);
				_dprintf("%s: %s %s %s %s %s %s %s\n", __FUNCTION__, enable, wanportset, lanportset
											, wl2gset, wl5gset, subnet_name, vlan_name);

				if (!strcmp(enable, "0") || strlen(enable) == 0)
					continue;
				if (!strcmp(subnet_name, "0") || strlen(subnet_name) == 0)
					continue;
				if (!strcmp(vlan_name, "0") || strlen(vlan_name) == 0)
					continue;
				wanportset_tmp = (unsigned int) strtol(wanportset,NULL,16);
				if( wanportset_tmp != 0 )
				{
					/* total:8 bits, bit0: WAN1 */
				}
				lanportset_tmp = (unsigned int) strtol(lanportset,NULL,16);
				if( (lanportset_tmp != 0) && (interface !=NULL) )
				{
					/* total:16 bits, bit0: LAN1, ..., bit15: reserve for the CPU port(user can not set) */
					/* 	rtkswitch 301 14
						rtkswitch 302 0
						rtkswitch 303 0x000000FF */

					lanportset_tmp |= ( 1<<15 );
					snprintf(lanportset_str, sizeof(lanportset_str), "0x%x", lanportset_tmp);

					get_vlan_info(vlan_name,vlan_id,vlan_prio);
					vlan_id_tmp = safe_atoi(vlan_id);
					vlan_prio_tmp = safe_atoi(vlan_prio);
					if( vlan_id_tmp < 1 || vlan_id_tmp > 4095 || vlan_prio_tmp < 0 || vlan_prio_tmp > 7 )
					{
						printf("VLAN vaule error vlan %d, prio %d\n",vlan_id_tmp,vlan_prio_tmp);
						continue;
					}
					eval("rtkswitch","301",vlan_id);
					eval("rtkswitch","302",vlan_prio);
					eval("rtkswitch","303",lanportset_str);

					printf("vconfig add %s %s\n", interface, vlan_id);
					eval("vconfig", "set_name_type", "VLAN_PLUS_VID_NO_PAD");
					eval("vconfig", "add", interface, vlan_id);

				}

				wlset_tmp = (unsigned int) strtol(wl2gset,NULL,16) | 
							( (unsigned int) strtol(wl5gset,NULL,16) << 16 );

				printf("wlset_tmp %x\n",wlset_tmp);

				set_vlan_ifnames(br_index, wlset_tmp, subnet_name, vlan_name);
				br_index ++;
			}

			free(nv);
		}
	}
	return;
}
#endif

#endif
