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
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>

#include <shutils.h>
#include <shared.h>
#include <qca.h>

extern int get_ap_mac(const char *ifname, struct iwreq *pwrq);
extern int diff_current_bssid(int unit, char bssid_str[]);



#if defined(RTCONFIG_SOC_IPQ8074)
/* Return value of /sys/firmware/devicetree/base/soc_version_major.
 * If it's not readable, return fixed value based on model.
 * @return:	value of /sys/firmware/devicetree/base/soc_version_major
 */
unsigned char get_soc_version_major(void)
{
#if defined(RTAX89U) || defined(GTAXY16000)
	const unsigned char sver = 2;
#else
	const unsigned char sver = 0;
#endif
	unsigned char v = sver;

	if (f_read("/sys/firmware/devicetree/base/soc_version_major", &v, 1) <= 0) {	/* 1 byte */
		dbg("%s: can't read soc_version_major, assume it's %d\n", __func__, sver);
		v = sver;
	}

	return v;
}
#endif

#if defined(RTCONFIG_GLOBAL_INI)
#if defined(RTCONFIG_SOC_IPQ8074) && defined(RTCONFIG_SPF10_QSDK)
/* Return path and filename of internal ini file.
 * e.g. /etc/Wireless/ini/internal/QCA8074V2_i.ini
 * @ini_fn:		.ini filename buffer
 * @ini_fn_size:	size of @ini_fn
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
int get_internal_ini_filename(char *ini_fn, size_t ini_fn_size)
{
	const char *ini_fn_tbl[] = { "QCA8074_i.ini", "QCA8074V2_i.ini" };
	unsigned char v;

	if (!ini_fn || !ini_fn_size)
		return -1;

	*ini_fn = '\0';

	v = get_soc_version_major();
	if (v <= 0 || v > ARRAY_SIZE(ini_fn_tbl)) {
		dbg("%s: unknown SoC version number [%d]\n", __func__, v);
		return -2;
	}

	snprintf(ini_fn, ini_fn_size, "%s/internal/%s", GLOBAL_INI_TOPDIR, ini_fn_tbl[v - 1]);

	return 0;
}
#endif	/* RTCONFIG_SOC_IPQ8074 && RTCONFIG_SPF10_QSDK */

/* Get one parameter from .ini file and return it's value in string format.
 * If @param_name is replicated multi-times, first one is returned.
 * @param_name:
 * @param_val:
 * @param_val_size:
 * @ini_fn:
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
int get_parameter_from_ini_file(const char *param_name, char *param_val, size_t param_val_size, const char *ini_fn)
{
	FILE *fp;
	int found = 0, key_len;
	char *p, line[MAX_INI_PARM_LINE_LEN];

	if (!param_name || !param_val || !param_val_size || !ini_fn)
		return -1;

	*param_val = '\0';
	if (!(fp = fopen(ini_fn, "r")))
		return -2;

	key_len = strlen(param_name);
	while (fgets(line, sizeof(line), fp)) {
		if (*line == '#' || *line == '\0' || *line == '\r' || *line == '\n' || *line == '=')
			continue;

		if (!strchr(line, '=')) {
			dbg("%s: unknown format [%s] in %s.\n", __func__, line, ini_fn);
			continue;
		}
		if (strncmp(line, param_name, key_len) || *(line + key_len) != '=')
			continue;

		/* replace '\n' with '\0' temporary. */
		if ((p = strchr(line, '\n')) != NULL)
			*p = '\0';

		if (!found) {
			strlcpy(param_val, line + key_len + 1, param_val_size);
			found++;
		}
	}
	fclose(fp);

	return 0;
}

#if (SPF_VER >= SPF_VER_ID(11,0))
/* Get one board/default parameter from .ini file and return it's value in string format.
 * If board-specific parameter absent, return default parameter instead.
 * If @param_name is replicated multi-times, first one is returned.
 * @board_name: e.g. ap-hk_v1
 * @param_name:
 * @param_val:
 * @param_val_size:
 * @ini_fn:
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
int get_board_or_default_parameter_from_ini_file(const char *board_name, const char *param_name, char *param_val, size_t param_val_size, const char *ini_fn)
{
	int c, r;
	char key_name[256];
	char *p, prefix[2][64];
#if defined(RTCONFIG_SOC_IPQ8074)
	unsigned char soc_ver = get_soc_version_major();
	char ver[sizeof("_v?XXX")];
#endif

	if (!board_name || !param_name || !param_val || !param_val_size || !ini_fn)
		return -1;

	strlcpy(prefix[0], board_name, 64);
	strlcpy(prefix[1], board_name, 64);
	for (p = prefix[1]; *p != '\0'; ++p) {
		if (!isdigit(*p))
			continue;

		*p = '\0';
		break;
	}
#if defined(RTCONFIG_SOC_IPQ8074)
	snprintf(ver, sizeof(ver), "_v%d", (soc_ver == 1)? 1 : 2);
	strlcat(prefix[1], ver, 64);
#endif
	strlcat(prefix[1], "_default", 64);

	/* INI file has strings with the below format
	 * <board_name>_<feature>=0/1   or
	 * <board_name>_<PCI_device_id>_<PCI_Slot_number>_<feature>=0/1
	 * Append a "_" to the board_name here so that grep would be able to
	 * differentiate boards with similar names like ap-mp03.1 and
	 * ap-mp03.1-c2
	 */
	for (c = 0; c <= 1; ++c) {
		/* 1st run: ap-hk01-c2_XXX
		 * 2nd run: ap-hk_v?_default_XXX (Hawkeye only)
		 *          ap-cp_default_XXX    (Another SOC)
		 */
		snprintf(key_name, sizeof(key_name), "%s_%s", (c == 0)? prefix[0] : prefix[1], param_name);

		*param_val = '\0';
		r = get_parameter_from_ini_file(key_name, param_val, param_val_size, ini_fn);
		if (!r && *param_val != '\0')
			break;
	}

	return 0;
}
#endif	/* SPF11.0+ */

/* Get one parameter from .ini file and return it's value in integer format.
 * If @param_name is replicated multi-times, first one is returned.
 * @param_name:
 * @param_val:
 * @ini_fn:
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
int get_integer_parameter_from_ini_file(const char *param_name, int *param_val, const char *ini_fn)
{
	int ret = 0;
	char intbuf[11] = { 0 };

	if (!param_val)
		return -1;

	if (!(ret = get_parameter_from_ini_file(param_name, intbuf, sizeof(intbuf), ini_fn)))
		*param_val = safe_atoi(intbuf);

	return ret;
}
#endif	/* RTCONFIG_GLOBAL_INI */

#if defined(RTCONFIG_WIFI_QCA9990_QCA9990) \
 || defined(RTCONFIG_WIFI_QCA9994_QCA9994) \
 || defined(RTCONFIG_WIFI_QCN5024_QCN5054)
int nss_wifi_offloading(void)
{
	int band, shift, olcfg = 0;
	char vphy[IFNAMSIZ] = { 0 }, prefix[sizeof("wlXXXXX_")];

	for (band = 0, olcfg = 0; band < MAX_NR_WL_IF; ++band) {
		SKIP_ABSENT_BAND(band);
		snprintf(prefix, sizeof(prefix), "wl%d_", band);
		strlcpy(vphy, get_vphyifname(band), sizeof(vphy));
		shift = safe_atoi(vphy + strlen(vphy) - 1);
		olcfg |= !!nvram_pf_get_int(prefix, "hwol") << shift;
	}
	return olcfg;
}
#endif

/* Helper of __get_qca_sta_info_by_ifname()
 * @src:	pointer to WLANCONFIG_LIST
 * @arg:
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
static int handler_qca_sta_info(const WLANCONFIG_LIST *src, void *arg)
{
	WIFI_STA_TABLE *sta_info = arg;
	WLANCONFIG_LIST *dst;

	if (!src || !arg || sta_info->Num < 0)
		return -1;

	dst = &sta_info->Entry[sta_info->Num++];
	*dst = *src;
#if 0
	dbg("[%s][%u][%u][%s][%s][%d][%s][%s]\n", dst->addr, dst->aid, dst->chan,
		dst->txrate, dst->rxrate, dst->rssi, dst->mode, dst->conn_time);
#endif


	return 0;
}

#define MAX_NR_STA_ITEMS	(30)
struct sta_info_item_s {
	int idx;		/* < 0: doesn't exist; >= 0: v[] index */
	const char *key;
	const char *fmt;	/* format string that is used to convert v[idx] */
	void *var;		/* target address that is used to store convertion result. */
};

/* Helper of __get_qca_sta_info_by_ifname() that is used to initialize struct sta_info_item_s array,
 * according to header line of output of "wlanconfig athX list".
 * header line maybe truncated due to:
 * 1. ACAPS never has data.
 * 2. IEs maybe empty string, RSN, WME, or RSN WME.
 * @line:
 * @sta_info_items:
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
static int init_sta_info_item(const char *line, struct sta_info_item_s *sta_info_items)
{
	const char f[] = "%s";
	int i, n;
	char fmt[MAX_NR_STA_ITEMS * sizeof(f)];
	char v[MAX_NR_STA_ITEMS][sizeof("MAXRATE(DOT11)XXXXX")];
	struct sta_info_item_s *p;

	if (!line || !sta_info_items)
		return -1;

	for (p = sta_info_items; p->key != NULL; ++p)
		p->idx = -1;

	for (i = 0, *fmt = '\0'; i < MAX_NR_STA_ITEMS; ++i)
		strlcat(fmt, f, sizeof(fmt));

	n = sscanf(line, fmt, v, v + 1, v + 2, v + 3, v + 4, v + 5, v + 6, v + 7, v + 8, v + 9,
		v + 10, v + 11, v + 12, v + 13, v + 14, v + 15, v + 16, v + 17, v + 18, v + 19,
		v + 20, v + 21, v + 22, v + 23, v + 24, v + 25, v + 26, v + 27, v + 28, v + 29);
	for (i = 0; i < n; ++i) {
		for (p = &sta_info_items[0]; p->key != NULL; ++p) {
			if (strcmp(v[i], p->key))
				continue;

			p->idx = i;
			break;
		}
	}

	return 0;
}

/* Helper of __get_qca_sta_info_by_ifname() that is used to fill data to WLANCONFIG_LIST,
 * according to header line of output of "wlanconfig athX list".
 * header line maybe truncated due to:
 * 1. ACAPS never has data.
 * 2. IEs maybe empty string, RSN, WME, or RSN WME.
 * @line:
 * @sta_info_items:
 * @return:
 * 	0:	success
 *     <0:	error
 *     >0:	number of items can't be parsed.
 */
static int fill_sta_info_item(const char *line, const struct sta_info_item_s *sta_info_items)
{
	const char f[] = "%s";
	int i, n, ret = 0;
	char fmt[MAX_NR_STA_ITEMS * sizeof(f)];
	char v[MAX_NR_STA_ITEMS][sizeof("IEEE80211_MODE_11BEA_EHT40MINUSXXXXX")];
	const struct sta_info_item_s *p;

	if (!line || !sta_info_items)
		return -1;

	for (i = 0, *fmt = '\0'; i < MAX_NR_STA_ITEMS; ++i)
		strlcat(fmt, f, sizeof(fmt));

	n = sscanf(line, fmt, v, v + 1, v + 2, v + 3, v + 4, v + 5, v + 6, v + 7, v + 8, v + 9,
		v + 10, v + 11, v + 12, v + 13, v + 14, v + 15, v + 16, v + 17, v + 18, v + 19,
		v + 20, v + 21, v + 22, v + 23, v + 24, v + 25, v + 26, v + 27, v + 28, v + 29);
	for (p = sta_info_items; n > 0 && p->key != NULL; ++p) {
		if (p->idx < 0)
			continue;

		if (sscanf(v[p->idx], p->fmt, p->var) == 1)
			continue;

		ret++;
		dbg("%s: can't parse. argv[%d] = [%s] key [%s] fmt [%s] var [%p]\n",
			__func__, p->idx, v[p->idx]? : "<NULL>", p->key, p->fmt, p->var);
	}

	return ret;
}

/* Return band of @name.
 * @name:	VAP interface name of main VAP. Guest VAP is not supported!
 * return:	enum wl_band_id
 */
int get_wifname_num(const char *name)
{
	int band, ret = -1;

	for (band = 0; ret < 0 && band < WL_NR_BANDS; ++band) {
		SKIP_ABSENT_BAND(band);

		if (strcmp(get_wififname(band), name))
			continue;

		ret = band;
	}

	return ret;
}

/* Return channel noise floor.
 * If @band or @ifname is valid and corresponding interface does exist,
 * adjust channel noise floor based on bandwidth.
 * @band:	enum wl_band_id, can be invalid value.
 * @ifname:	VAP interface name.
 * @return:	channel noise floor.
 */
int get_channf(int band, const char *ifname)
{
	int channf = -96;	/* QCA_DEFAULT_NOISE_FLOOR, SF case#03626623. */
	int nf_offset = 0, bw = 40, nctrlsb;
	char vap[IFNAMSIZ] = { 0 };

	if (!absent_band(band))
		__get_wlifname(band, 0, vap);
	else if (ifname)
		strlcpy(vap, ifname, sizeof(vap));

	if (!iface_exist(vap) || get_bw_nctrlsb(vap, &bw, &nctrlsb) < 0)
		return channf;

	/* SF case#03945423, nf_offset:
	 * 20MHz: -3
	 * 40MHz:  0
	 * 80MHz:  3
	 * 160MHz: 6
	 * SF case#07314548, nf_offset
	 * 240MHz: 7.8
	 */
	if (bw == 20)
		nf_offset = -3;
	else if (bw == 80)
		nf_offset = 3;
	else if (bw == 160)
		nf_offset = 6;
	else if (bw == 240 || bw == 320)
		nf_offset = 7;

	return channf + nf_offset;
}

/* Parsing "wlanconfig athX list" result, fill WLANCONFIG_LIST, and then pass it to @handler() with @arg which is provided by caller.
 *
 * @ifname:	VAP interface name that is used to execute "wlanconfig @ifname list" command.
 * @subunit_id:	if non-zero, copied to WLANCONFIG_LIST.subunit
 * @handler:	handler function that will be execute for each client.
 * return:
 * 	0:	success
 *  otherwise:	error
 *
 * ILQ3.1 example:
 * wlanconfig ath1 list
 * ADDR               AID CHAN TXRATE RXRATE RSSI IDLE  TXSEQ  RXSEQ  CAPS        ACAPS     ERP    STATE MAXRATE(DOT11) HTCAPS ASSOCTIME    IEs   MODE PSMODE
 * 00:10:18:55:cc:08    1  149  55M   1299M   63    0      0   65535               0        807              0              Q 00:10:33 IEEE80211_MODE_11A  0
 * 08:60:6e:8f:1e:e6    2  149 159M    866M   44    0      0   65535     E         0          b              0           WPSM 00:13:32 WME IEEE80211_MODE_11AC_VHT80  0
 * 08:60:6e:8f:1e:e8    1  157 526M    526M   51 4320      0   65535    EP         0          b              0          AWPSM 00:00:10 RSN WME IEEE80211_MODE_11AC_VHT80 0
 *
 * SPF8 CSU2 QSDK example:
 * admin@RT-AX89U:/tmp/home/root# wlanconfig ath0 list
 * ADDR               AID CHAN TXRATE RXRATE RSSI MINRSSI MAXRSSI IDLE  TXSEQ  RXSEQ  CAPS        ACAPS     ERP    STATE MAXRATE(DOT11) HTCAPS ASSOCTIME    IEs   MODE                   PSMODE
 * 12:9d:92:4e:85:bc    1  104 2882M   3026M   73       0      74    0      0   65535   EPs         0          b              0           AWPSM 00:00:35     RSN WME IEEE80211_MODE_11AXA_HE80   0
 * 14:dd:a9:3d:68:65    2  104 433M    433M   69       0      79    0      0   65535    EP         0          b              0            AWPS 00:00:35     RSN WME IEEE80211_MODE_11AC_VHT80   1
 *
 * SPF10 ES QSDK example:
 * admin@GT-AXY16000:/tmp/home/root# wlanconfig ath0 list
 * ADDR               AID CHAN TXRATE RXRATE RSSI MINRSSI MAXRSSI IDLE  TXSEQ  RXSEQ  CAPS XCAPS        ACAPS     ERP    STATE MAXRATE(DOT11) HTCAPS   VHTCAPS ASSOCTIME    IEs   MODE RXNSS TXNSS                   PSMODE
 * 14:dd:a9:3d:68:65    1   60 433M      6M   36      22      40    0      0   65535    EP    OI         0          b              0            AWPS             gGR 00:00:09     RSN WME IEEE80211_MODE_11AC_VHT80  1 1   1
 *  Minimum Tx Power             : 0
 *  Maximum Tx Power             : 0
 *  HT Capability                        : Yes
 *  VHT Capability                       : Yes
 *  MU capable                   : No
 *  SNR                          : 36
 *  Operating band                       : 5GHz
 *  Current Operating class      : 0
 *  Supported Rates              : 12  18  24  36  48  72  96  108
 *
 * SPF11 CSU1 QSDK example:
 * admin@RT-AX89U-4988:/tmp# wlanconfig ath0 list
 * ADDR               AID CHAN TXRATE RXRATE RSSI MINRSSI MAXRSSI IDLE  TXSEQ  RXSEQ  CAPS XCAPS ACAPS     ERP    STATE MAXRATE(DOT11) HTCAPS   VHTCAPS ASSOCTIME    IEs   MODE RXNSS TXNSS                   PSMODE
 * 14:dd:a9:3d:68:65    1   40 325M    433M  -61     -79     -53   24      0   65535    EP    OI NULL    0          b         541666            AWPS             gGR 00:19:28     RSN WME IEEE80211_MODE_11AC_VHT80  1 1   0  
 *  Minimum Tx Power             : 0
 *  Maximum Tx Power             : 0
 *  HT Capability                        : Yes
 *  VHT Capability                       : Yes
 *  MU capable                   : No
 *  SNR                          : 32
 *  Operating band                       : 5GHz
 *  Current Operating class      : 0
 *  Supported Rates              : 12  18  24  36  48  72  96  108 
 *  Max STA phymode              : IEEE80211_MODE_11AC_VHT80 
 *
 *  SPF12.2 CSU1 QSDK example:
 *  admin@TUF-BE6500-0100:/# wlanconfig ath1 list
 *  ADDR               AID CHAN TXRATE RXRATE RSSI MINRSSI MAXRSSI IDLE  TXSEQ  RXSEQ  CAPS XCAPS ACAPS     ERP    STATE MAXRATE(DOT11) HTCAPS   VHTCAPS ASSOCTIME    IEs   MODE RXNSS TXNSS                   PSMODE
 *  38:00:25:53:bc:ef    1   40   6M     18M  -71     -78     -65    0      0   65535   EPR   EQO NULL    0          b         866700           AWRSM          1gGTRs 00:00:08     RSN WME IEEE80211_MODE_11AC_VHT80  2 2   0
 *  1a:7c:61:81:ac:ba    1    6 154M    206M   -7     -13      -7    0      0   65535  EPSs  ETWt NULL    0          f         688200           AWPSM          gGTRsS 00:06:56     RSN WME IEEE80211_MODE_11BEG_EHT40  2 2   0
 *  1a:7c:61:81:ac:bb    1   60 2882M   1729M  -18     -20     -10    1      0   65535   EPs  ETWt NULL    0          b        5764800           AWPSM           gGTRs 00:06:50     RSN WME IEEE80211_MODE_11BEA_EHT160  4 4   0
 *        LM BRP BRA BRT
 *   RSSI is combined over chains in dBm
 *   Minimum Tx Power             : 0
 *   Maximum Tx Power             : 22
 *   HT Capability                        : Yes
 *   VHT Capability                       : Yes
 *   MU capable                   : Yes
 *   SNR                          : 24
 *   Operating band                       : 5GHz
 *   Current Operating class      : 128
 *   Supported Operating classes  : 81  83  84  115  116  117  118  119  120  121  122  123  124  125  126  127
 *   Supported Rates(Mbps)                : 6  9  12  18  24  36  48  54
 *   Max STA phymode              : IEEE80211_MODE_11AC_VHT80
 *   MLO                          : No
 *
 * SPF12.2 CSU3 QSDK, MLD example
 * root@OpenWrt:/etc/config# wlanconfig mld0 list
 * ==== STA Info From Link: ath0 =======
 * ADDR               AID CHAN TXRATE RXRATE RSSI MINRSSI MAXRSSI IDLE  TXSEQ  RXSEQ  CAPS XCAPS ACAPS     ERP    STATE MAXRATE(DOT11) HTCAPS   VHTCAPS ASSOCTIME    IEs   MODE RXNSS TXNSS                   PSMODE
 * 06:94:00:01:01:00   94    1   1M      0M  -19     -99     -18   29      0   65535  EPSs  cETW NULL    0          f         344200           AWPSM          gGTRsS 00:13:11     RSN WME IEEE80211_MODE_11BEG_EHT20  2 2   0
 *
 *  RSSI is combined over chains in dBm
 *  Minimum Tx Power		: 0
 *  Maximum Tx Power		: 0
 *  HT Capability			: Yes
 *  VHT Capability			: Yes
 *  MU capable			: Yes
 *  SNR				: 80
 *  Operating band			: 2.4GHz
 *  Current Operating class	: 81
 *  Supported Operating classes	: 81  83  84
 *  Supported Rates(Mbps)		: 1  2  5  6  9  11  12  18  24  36  48  54
 *  Max STA phymode		: IEEE80211_MODE_11BEG_EHT20
 *  MLO				: Yes
 *  MLD Addr			: 0a:94:00:01:01:00
 *  Num Partner links		: 2
 *        Partner link 0: ADDR :06:94:00:01:01:02, Link ID: 2 Bridge link: 0
 *        Partner link 1: ADDR :06:94:00:01:01:01, Link ID: 1 Bridge link: 0
 *  EMLSR capable			: No
 *  EMLMR capable			: No
 *  STR capable			: Yes
 *
 * ==== STA Info From Link: ath1 =======
 * ADDR               AID CHAN TXRATE RXRATE RSSI MINRSSI MAXRSSI IDLE  TXSEQ  RXSEQ  CAPS XCAPS ACAPS     ERP    STATE MAXRATE(DOT11) HTCAPS   VHTCAPS ASSOCTIME    IEs   MODE RXNSS TXNSS                   PSMODE
 * 06:94:00:01:01:01   94   64 154M    309M  -27     -94     -25   29      0   65535   EPs  ETWt NULL    0          b         344200           AWPSM           gGTRs 00:13:11     RSN WME IEEE80211_MODE_11BEA_EHT20  2 2   0
 *
 *  RSSI is combined over chains in dBm
 *  Minimum Tx Power		: 0
 *  Maximum Tx Power		: 69
 *  HT Capability			: Yes
 *  VHT Capability			: Yes
 *  MU capable			: Yes
 *  SNR				: 67
 *  Operating band			: 5GHz
 *  Current Operating class	: 118
 *  Supported Operating classes	: 115  118  124  121  125  116  119  122  126  126  117  120  123  127  127  128  129  130
 *  Supported Rates(Mbps)		: 6  9  12  18  24  36  48  54
 *  Max STA phymode		: IEEE80211_MODE_11BEA_EHT20
 *  MLO				: Yes
 *  MLD Addr			: 0a:94:00:01:01:00
 *  Num Partner links		: 2
 *        Partner link 0: ADDR :06:94:00:01:01:02, Link ID: 2 Bridge link: 0
 *        Partner link 1: ADDR :06:94:00:01:01:00, Link ID: 0 Bridge link: 0
 *  EMLSR capable			: No
 *  EMLMR capable			: No
 *  STR capable			: Yes
 *
 * ==== STA Info From Link: ath2 =======
 * ADDR               AID CHAN TXRATE RXRATE RSSI MINRSSI MAXRSSI IDLE  TXSEQ  RXSEQ  CAPS XCAPS ACAPS     ERP    STATE MAXRATE(DOT11) HTCAPS   VHTCAPS ASSOCTIME    IEs   MODE RXNSS TXNSS                   PSMODE
 * 06:94:00:01:01:02   94  133 5188M      6M  -39     -40     -37   29      0   65535   EPs  ETWt NULL    0          3        5764800               Q              00 00:13:11     RSN WME IEEE80211_MODE_11BEA_EHT320  2 2   0
 *
 *  RSSI is combined over chains in dBm
 *  Minimum Tx Power		: 0
 *  Maximum Tx Power		: 69
 *  HT Capability			: No
 *  VHT Capability			: No
 *  MU capable			: Yes
 *  SNR				: 54
 *  Operating band			: 6GHz
 *  Current Operating class	: 137
 *  Supported Operating classes	: 131  132  133  134  135  137
 *  Supported Rates(Mbps)		: 6  9  12  18  24  36  48  54
 *  Max STA phymode		: IEEE80211_MODE_11BEA_EHT320
 *  MLO				: Yes
 *  MLD Addr			: 0a:94:00:01:01:00
 *  Num Partner links		: 2
 *        Partner link 0: ADDR :06:94:00:01:01:00, Link ID: 0 Bridge link: 0
 *        Partner link 1: ADDR :06:94:00:01:01:01, Link ID: 1 Bridge link: 0
 *  EMLSR capable			: No
 *  EMLMR capable			: No
 *  STR capable			: Yes
 */
#define STAINFO_FROM_LINK	"STA Info From Link"
static int __get_QCA_sta_info_by_ifname(const char *ifname, char subunit_id, int (*handler)(const WLANCONFIG_LIST *rptr, void *arg), void *arg)
{
#if defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_SOC_IPQ53XX)
	const int l2_offset = 91;
#elif defined(RTCONFIG_SOC_IPQ8064) || defined(RTCONFIG_)
	const int l2_offset = 85;
#else
	const int l2_offset = 79;
#endif
#if defined(RTCONFIG_WIFI7)
	int band = -1;
#endif
	FILE *fp;
	int channf, ret = 0, ax2he = 0, dirty;
	unsigned char tmac[6], *tm = &tmac[0];
	char cmd[sizeof("wlanconfig XXX list") + IFNAMSIZ];
	char *q, buf[64], line_buf[300], *l2 = line_buf + l2_offset;
	WLANCONFIG_LIST result, *r = &result;
	struct sta_info_item_s part1_tbl[] = {
		/* Parse ADDR ~ XCAPS. */
		{ .key = "ADDR",	.fmt = "%s",	.var = &r->addr },
		{ .key = "AID",		.fmt = "%u",	.var = &r->aid },
		{ .key = "CHAN",	.fmt = "%u",	.var = &r->chan },
		{ .key = "TXRATE",	.fmt = "%s",	.var = &r->txrate },
		{ .key = "RXRATE",	.fmt = "%s",	.var = &r->rxrate },
		{ .key = "RSSI",	.fmt = "%d",	.var = &r->rssi },

		{ .key = NULL, .fmt = NULL, .var = NULL },
	}, part2_tbl[] = {
		/* Parse ACAPS ~ IEs (maybe empty string, RSN, WME, or both).
		 * ACAPS is empty on ILQ2.x ~ SPF10, is "NULL" on SPF11
		 */
		{ .key = "HTCAPS",	.fmt = "%s",	.var = &r->htcaps },
		{ .key = "VHTCAPS",	.fmt = "%s",	.var = &r->vhtcaps },
		{ .key = "ASSOCTIME",	.fmt = "%s",	.var = &r->conn_time },

		{ .key = NULL, .fmt = NULL, .var = NULL },
	}, part3_tbl[] = {
		/* Parse MODE ~ PSMODE */
		{ .key = "MODE",	.fmt = "IEEE80211_MODE_%s", .var = r->mode },
		{ .key = "PSMODE",	.fmt = "%u",	.var = &r->psm },
		{ .key = "RXNSS",	.fmt = "%u",	.var = &r->rxnss },
		{ .key = "TXNSS",	.fmt = "%u",	.var = &r->txnss },

		{ .key = NULL, .fmt = NULL, .var = NULL },
	};

	if (!ifname || !handler)
		return -1;

	snprintf(cmd, sizeof(cmd), "wlanconfig %s list", ifname);
	if (!(fp = popen(cmd, "r")))
		return -2;

#if defined(RTCONFIG_WIFI_QCN5024_QCN5054) \
 || defined(RTCONFIG_QCA_AXCHIP) \
 || defined(RTCONFIG_QCA_BECHIP)
	if (!find_word(nvram_safe_get("rc_support"), "11AX"))
		ax2he = 1;
#endif
	channf = get_channf(-1, ifname);

	/* Find and parsing header and initialize related data structure */
	do {
		if (!fgets(line_buf, sizeof(line_buf), fp))
			goto leave;

		if (!strncmp(line_buf, "ADDR", 4))
			break;
	} while (1);

	if ((q = strstr(line_buf, "MODE")) != NULL) {
		*(q - 1) = '\0';
		init_sta_info_item(q, part3_tbl);
	}
	if ((q = strstr(line_buf, "ACAPS")) != NULL) {
		*(q - 1) = '\0';
		l2 = q;
#if SPF_VER >= SPF_VER_ID(11,0)
		init_sta_info_item(q, part2_tbl);
#else
		/* ILQ2.x ~ SPF10 */
		init_sta_info_item(q + strlen("ACAPS"), part2_tbl);	/* skip ACAPS due to it doesn't have data. */
#endif
	}
	init_sta_info_item(line_buf, part1_tbl);

#if defined(RTCONFIG_WIFI7)
	get_wlif_unit(ifname, &band, NULL);
#endif
	/* Parsing client list */
	dirty = 0;
	memset(r, 0, sizeof(*r));
	while (fgets(line_buf, sizeof(line_buf), fp) != NULL) {
		if (!strncmp(line_buf, "===", 3) || !strncmp(line_buf, "ADDR", 4)) {
			/* Output of mld client list may have multiple "===" or "ADDR". */
			continue;
		} else if (*line_buf == ' ') {
			/* sub-fields of last client. */
			if (!strncmp(line_buf + 1, "MLO", 3)
			 && (q = strchr(line_buf + 3, ':')) != NULL) {
				*buf = '\0';
				if (sscanf(q + 1, "%s", buf) == 1 && !strcmp(buf, "Yes")) {
					r->flags |= WLFLAGS_MLO;
				}
			}
			continue;
		} else if (sscanf(line_buf, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx %*[^\n]",
				tm, tm + 1, tm + 2, tm + 3, tm + 4, tm + 5) != 6)
			continue;

		if (dirty) {
			handler(r, arg);
			dirty = 0;
		}
		memset(r, 0, sizeof(*r));
		/* Parsing part3, all data behind IEs (started from IEEE802...) */
		if ((q = strstr(line_buf, "IEEE80211_MODE_")) != NULL) {
			*(q - 1) = '\0';
			fill_sta_info_item(q, part3_tbl);
		}

		/* Parsing part2, ACAPS (omit) ~ IEs */
		*(l2 - 1) = '\0';
		fill_sta_info_item(l2, part2_tbl);

		/* Parsing part1, ADDR ~ IEs */
		fill_sta_info_item(line_buf, part1_tbl);

		/* Post adjustment */
		if (ax2he) {
			if ((q = strstr(r->mode, "11AXA")) != NULL)
				memcpy(q, "11AHE", 5);
			else if ((q = strstr(r->mode, "11AXG")) != NULL)
				memcpy(q, "11GHE", 5);
		}
		if (subunit_id)
			r->subunit_id = subunit_id;
		if (strlen(r->rxrate) >= 6)
			strcpy(r->rxrate, "0M");
		convert_mac_string(r->addr);

		/* If wlanconfig reports QCA_RSSI (0 ~ 115), adjust it with channf.
		 * But it's not accurate as long as client bandwidth different.
		 * If wlanconfig reports normal RSSI, negative value, don't adjust it again.
		 */
		if (r->rssi > 0) {
			r->rssi += channf;
			if (r->rssi >= 0)
				r->rssi = -1;
		}

#if defined(RTCONFIG_WIFI7)
		if (is_5g(band) && (q = strstr(r->mode, "EHT320")) != NULL) {
			strlcpy(q, "EHT240", sizeof(r->mode) - (q - r->mode));
		}
#endif
		dirty = 1;
	}
	if (dirty)
		handler(r, arg);
leave:
	pclose(fp);
	return ret;
}

#if defined(RTCONFIG_WIGIG)
/* Parsing "iw wlan0 station dump" result, fill WLANCONFIG_LIST, and then pass it to @handler() with @arg which is provided by caller.
 * @ifname:	VAP interface name that is used to execute "iw @ifname station dump" command.
 * @subunit_id:	if non-zero, copied to WLANCONFIG_LIST.subunit
 * @handler:	handler function that will be execute for each client.
 * return:
 * 	0:	success
 *  otherwise:	error
 *
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
static int __get_IW_sta_info_by_ifname(const char *ifname, char subunit_id, int (*handler)(const WLANCONFIG_LIST *rptr, void *arg), void *arg)
{
	FILE *fp;
	int c, time_val, hr, min, sec, rssi;
	char rate[6], line_buf[300];
	char cmd[sizeof("iw wlan0 station dump XXXXXX")];
	WLANCONFIG_LIST result, *r = &result;

	if (!ifname || !handler)
		return -1;

	snprintf(cmd, sizeof(cmd), "iw %s station dump", get_wififname(WL_60G_BAND));
	fp = popen(cmd, "r");
	if (!fp)
		return -2;

	/* /sys/kernel/debug/ieee80211/phy0/wil6210/stations has client list too.
	 * But I guess none of any another attributes exist, e.g., connection time, exist.
	 * Thus, parsing result of "iw wlan0 station dump" instead.
	 */
	while (fgets(line_buf, sizeof(line_buf), fp)) {
		if (strncmp(line_buf, "Station", 7)) {
			continue;
		}

next_sta:
		memset(r, 0, sizeof(*r));
		c = sscanf(line_buf, "Station %17[0-9a-f:] %*[^\n]", r->addr);
		if (c != 1) {
			continue;
		}
		convert_mac_string(r->addr);
		if (subunit_id)
			r->subunit_id = subunit_id;
		strlcpy(r->mode, "11ad", sizeof(r->mode));	/* FIXME */
		while (fgets(line_buf, sizeof(line_buf), fp)) {
			if (!strncmp(line_buf, "Station", 7)) {

#if 0
				dbg("[%s][%u][%u][%s][%s][%u][%s]\n",
					r->addr, r->aid, r->chan, r->txrate, r->rxrate, r->rssi, r->mode);
#endif
				handler(r, arg);
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
#endif	/* RTCONFIG_WIGIG */

/* Wrapper function of QCA/IW Wireless client list parser.
 * @ifname:	VAP interface name
 * @subunit_id:
 * @sta_info:	pointer to WIFI_STA_TABLE
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
int __get_qca_sta_info_by_ifname(const char *ifname, char subunit_id, int (*handler)(const WLANCONFIG_LIST *rptr, void *arg), void *arg)
{
#if defined(RTCONFIG_WIGIG)
	char *vap_60g = get_wififname(WL_60G_BAND);

	if (!ifname)
		return -1;

	if (!strncmp(ifname, vap_60g, strlen(vap_60g))) {
		return __get_IW_sta_info_by_ifname(ifname, subunit_id, handler, arg);
	} else
#endif
		return __get_QCA_sta_info_by_ifname(ifname, subunit_id, handler, arg);
}

/* Wrapper function of QCA Wireless client list parser.
 * NOTE: Caller is in charge of initialize @sta_info as zero.
 * @ifname:	VAP interface name
 * @subunit_id:
 * @sta_info:	pointer to WIFI_STA_TABLE
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
int get_qca_sta_info_by_ifname(const char *ifname, char subunit_id, WIFI_STA_TABLE *sta_info)
{
#if defined(RTCONFIG_WIGIG)
	char *vap_60g = get_wififname(WL_60G_BAND);

	if (!ifname)
		return -1;

	if (!strncmp(ifname, vap_60g, strlen(vap_60g))) {
		return __get_IW_sta_info_by_ifname(ifname, subunit_id, handler_qca_sta_info, sta_info);
	} else
#endif
		return __get_QCA_sta_info_by_ifname(ifname, subunit_id, handler_qca_sta_info, sta_info);
}

struct find_vap_by_sta_priv_s {
	char *addr;

	int found;
};

/* Helper of rssi_check_unit()
 * @src:	pointer to WLANCONFIG_LIST
 * @arg:
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
static int handle_find_vap_by_sta(const WLANCONFIG_LIST *src, void *arg)
{
	unsigned char ea1[6] = { 0 }, ea2[6] = { 0 };
	struct find_vap_by_sta_priv_s *priv = arg;

	if (!src || !arg || !priv->addr)
		return -1;
	if (priv->found)
		return 0;

	if (!ether_atoe(priv->addr, ea1) || !ether_atoe(src->addr, ea2) || memcmp(ea1, ea2, sizeof(ea1)))
		return 0;

	priv->found = 1;
	return 0;
}

/* Check whether @sta_addr exist on @vap, if not, find correct VAP and return it by @vap.
 * @sta_addr:
 * @vap:	pointer to char pointer, length IFNAMSIZ.
 * @return:
 *    < 0:	error
 * 	0:	@sta_addr is not found in all VAP at same band
 * 	1:	@sta_addr is found, @vap maybe update
 */
int find_vap_by_sta(char *sta_addr, char *vap)
{
	int band, y, max_subnet;
	char prefix[sizeof("wlX.XXX_")], ifname[IFNAMSIZ];
	struct find_vap_by_sta_priv_s priv;

	if (!sta_addr || *sta_addr == '\0' || !vap || *vap == '\0')
		return -1;

	memset(&priv, 0, sizeof(priv));
	priv.addr = sta_addr;
	__get_qca_sta_info_by_ifname(ifname, 0, handle_find_vap_by_sta, &priv);
	if (priv.found)
		return 1;

	band = -1;
	get_wlif_unit(vap, &band, NULL);
	if (band < 0 || band >= MAX_NR_WL_IF || __absent_band(band))
		return -1;

	/* Find correct VAP for @sta_addr. */
	max_subnet = num_of_mssid_support(band);
	for (y = 0; y < max_subnet; ++y) {
		snprintf(prefix, sizeof(prefix), "wl%d.%d_", band, y);
		if (!nvram_pf_match(prefix, "bss_enabled", "1"))
			continue;

		get_wlxy_ifname(band, y, ifname);
		if (!strcmp(ifname, vap))
			continue;

		memset(&priv, 0, sizeof(priv));
		priv.addr = sta_addr;
		if (!__get_qca_sta_info_by_ifname(ifname, 0, handle_find_vap_by_sta, &priv) && priv.found) {
			strlcpy(vap, ifname, IFNAMSIZ);
			break;
		}
	}

	return priv.found? 1 : 0;
}

#if defined(RTCONFIG_NO_RELOAD_WIFI_DRV_IF_POSSIBLE)
/* wlX_XXX on CAP/RE both. */
static const char *reload_qcawifi_params[] = {
	/* Effect module parameters, reload if changed. */
	"twt", "atf",
#if defined(RTCONFIG_WIFI_QCA9990_QCA9990) \
 || defined(RTCONFIG_WIFI_QCA9994_QCA9994) \
 || defined(RTCONFIG_WIFI_QCN5024_QCN5054)
	"hwol",
#endif
	/* Effect VPHY settings and won't be set if it equal to default value we assumed.
	 * qcawifi modules must be reloaded due to VPHY interface always exist and hold
	 * last settings.
	 */
	"frameburst",
#if defined(RTCONFIG_WIFI_QCN5024_QCN5054)
	"precacen",
#endif
	NULL
};

/* Return true if qca-wifi modules must be reloaded.
 * In general, if WiFi settings that effect module parameters of qca-wifi modules hasn't been changed,
 * all VAP are destroied successful, VPHY parameters that will be set if different from default value,
 * and hostapd configurations are removed from hostapd instance,it should be okay not to reload qca-wifi modules.
 * If you want to always reload qca-wifi modules in ATE mode, check it in rc.
 * @return:
 * 	0:	no need to reload qca-wifi drivers
 *  otherwise:	must reload qca-wifi drivers
 */
int __need_to_reload_wifi_drv(void)
{
	const char **p;
	int i, reload = 0;;
	char main_prefix[sizeof("wlX_XXX")], cache_prefix[sizeof("wlX_cache_XXX")];

	if (nvram_match("reload_wifidrv", "1"))
		return 1;

	for (i = 0; !reload && i < MAX_NR_WL_IF; ++i) {
		SKIP_ABSENT_BAND(i);

		snprintf(main_prefix, sizeof(main_prefix), "wl%d_", i);
		snprintf(cache_prefix, sizeof(cache_prefix), "wl%d_cache_", i);
		for (p = &reload_qcawifi_params[0]; !reload && p && *p; ++p) {
			if (*nvram_pf_safe_get(cache_prefix, *p) == '\0'
			 || *nvram_pf_safe_get(main_prefix, *p) == '\0')
				continue;
			if (!strcmp(nvram_pf_safe_get(main_prefix, *p), nvram_pf_safe_get(cache_prefix, *p)))
				continue;
			reload++;
		}
	}

	return reload;
}

/* Copy wlX_XXX to wlX_cache_XXX. It will be used to test whether qca-wifi modules
 * must be reloaded during restart wireless or not.
 */
int save_wl_params_for_testing_reload_wifi_drv(void)
{
	const char **p;
	int i;
	char main_prefix[sizeof("wlX_XXX")], cache_prefix[sizeof("wlX_cache_XXX")];

	for (i = 0; i < MAX_NR_WL_IF; ++i) {
		SKIP_ABSENT_BAND(i);

		snprintf(main_prefix, sizeof(main_prefix), "wl%d_", i);
		snprintf(cache_prefix, sizeof(cache_prefix), "wl%d_cache_", i);
		for (p = &reload_qcawifi_params[0]; p && *p; ++p) {
			nvram_pf_set(cache_prefix, *p, nvram_pf_get(main_prefix, *p));
		}
	}

	return 0;
}
#endif

#ifdef RTCONFIG_AMAS
/**
 * @brief add beacon vise by unit and subunit
 *
 * @param unit band index
 * @param subunit mssid index
 * @param hexdata vise string
 */
void add_beacon_vsie_by_unit(int unit, int subunit, char *hexdata)
{
	// 0: Beacon
	// 1: ProbeRequest
	// 2: ProbeResponse
	// 3: AuthenticationRequest
	// 4: AuthenticationRespnse
	// 5: AssocationRequest
	// 6: AssociationResponse
	// 7: ReassociationRequest
	// 8: ReassociationResponse
	char cmd[300];
	int pktflag = 0x0;
	int len = 0;
	char ifname[IFNAMSIZ] = "";
	char buf[50] = "wlXX.XX_ifname";
#ifdef RTCONFIG_WIFI_SON
	if (nvram_match("wifison_ready", "1"))
		return;
#endif
	len = 3 + strlen(hexdata)/2;	/* 3 is oui's len */

	if(subunit<=0)
		strlcpy(ifname, get_wififname(unit), sizeof(ifname));	// TODO: Should we get the band from nvram?
	else
	{
		memset(buf, 0, sizeof(buf));
                snprintf(buf, sizeof(buf), "wl%d.%d_ifname", unit, subunit);
		strlcpy(ifname, nvram_safe_get(buf), sizeof(ifname));
		if(!guest_wlif(ifname)) //not guestnetwork
			return;
	}

	//_dprintf("%s: ifname=%s\n", __func__, ifname);

	if (*ifname != '\0') {
		snprintf(cmd, sizeof(cmd), "hostapd_cli -i%s set_vsie %d DD%02X%02X%02X%02X%s",
			ifname, pktflag, (uint8_t)len, (uint8_t)OUI_ASUS[0], (uint8_t)OUI_ASUS[1], (uint8_t)OUI_ASUS[2], hexdata);
		_dprintf("%s: cmd=%s\n", __func__, cmd);
		system(cmd);
	}
}


/**
 * @brief add guest vsie
 *
 * @param hexdata vsie string
 */
void add_beacon_vsie_guest(char *hexdata)
{
	int unit = 0, subunit = 0;
    	char word[100], *next;

    	foreach (word, nvram_safe_get("wl_ifnames"), next) {
			if (nvram_get_int("re_mode") == 1)  // RE
					subunit = 2;
			else  // CAP/Router
					subunit = 1;
			for (; subunit <=  num_of_mssid_support(unit); subunit++)
			{
				char buf[] = "wlXX.XX_ifname";
				memset(buf, 0, sizeof(buf));
				snprintf(buf, sizeof(buf), "wl%d.%d_ifname", unit, subunit);
				if ((is_intf_up(nvram_safe_get(buf)) != -1)  // interface exist
#if defined(RTCONFIG_MLO)
					&& !is_mlo_dwb_mssid(nvram_safe_get(buf))
					&& !is_compatible_network(nvram_safe_get(buf))
#ifdef RTCONFIG_MULTILAN_MWL
					&& !is_mainFH_network(nvram_safe_get(buf))
#endif
#endif
					)
					add_beacon_vsie_by_unit(unit, subunit, hexdata);
        	}
        	unit++;
    	}
}

void add_beacon_vsie(char *hexdata)
{
	// 0: Beacon
	// 1: ProbeRequest
	// 2: ProbeResponse
	// 3: AuthenticationRequest
	// 4: AuthenticationRespnse
	// 5: AssocationRequest
	// 6: AssociationResponse
	// 7: ReassociationRequest
	// 8: ReassociationResponse
	char cmd[300];
	int pktflag = 0x0;
	int len = 0;
	char ifname[IFNAMSIZ] = "";
#ifdef RTCONFIG_BHCOST_OPT
        int unit = 0;
        char word[100], *next;
#endif

#ifdef RTCONFIG_WIFI_SON
	if (nvram_match("wifison_ready", "1"))
		return;
#endif
	len = 3 + strlen(hexdata)/2;	/* 3 is oui's len */
#ifdef RTCONFIG_BHCOST_OPT
	unit=0;
	foreach (word, nvram_safe_get("wl_ifnames"), next)
	{
		strlcpy(ifname, get_wififname(unit), sizeof(ifname));
	//	_dprintf("%s: wl%d_ifname=%s\n", __func__,unit, ifname);
#else
	strlcpy(ifname, get_wififname(0), sizeof(ifname));	// TODO: Should we get the band from nvram?
	//_dprintf("%s: wl0_ifname=%s\n", __func__, ifname);
#endif

	if (*ifname != '\0') {
		snprintf(cmd, sizeof(cmd), "hostapd_cli -i%s set_vsie %d DD%02X%02X%02X%02X%s",
			ifname, pktflag, (uint8_t)len, (uint8_t)OUI_ASUS[0], (uint8_t)OUI_ASUS[1], (uint8_t)OUI_ASUS[2], hexdata);
		_dprintf("%s: cmd=%s\n", __func__, cmd);
		system(cmd);
	}
#ifdef RTCONFIG_BHCOST_OPT
		unit++;
	}
#endif
}

/**
 * @brief remove beacon vsie by unit and subunit
 *
 * @param unit band index
 * @param subunit mssid index
 * @param hexdata vsie string
 */
void del_beacon_vsie_by_unit(int unit, int subunit, char *hexdata)
{
	// 0: Beacon
	// 1: ProbeRequest
	// 2: ProbeResponse
	// 3: AuthenticationRequest
	// 4: AuthenticationRespnse
	// 5: AssocationRequest
	// 6: AssociationResponse
	// 7: ReassociationRequest
	// 8: ReassociationResponse
	char cmd[300] = {0};
	int pktflag = 0x0;
	int len = 0;
	char ifname[IFNAMSIZ] = "";
	char buf[50] = "wlXX.XX_ifname";

#ifdef RTCONFIG_WIFI_SON
	if (nvram_match("wifison_ready", "1"))
		return;
#endif
	len = 3 + strlen(hexdata)/2;	/* 3 is oui's len */

	if(subunit<=0)
		strlcpy(ifname, get_wififname(unit), sizeof(ifname));	// TODO: Should we get the band from nvram?
	else
	{
		memset(buf, 0, sizeof(buf));
                snprintf(buf, sizeof(buf), "wl%d.%d_ifname", unit, subunit);
		strlcpy(ifname, nvram_safe_get(buf), sizeof(ifname));
		if(!guest_wlif(ifname)) //not guestnetwork
			return;
	}

	//_dprintf("%s: ifname=%s\n", __func__, ifname);

	if (*ifname != '\0') {
		snprintf(cmd, sizeof(cmd), "hostapd_cli -i%s del_vsie %d DD%02X%02X%02X%02X%s",
			ifname, pktflag, (uint8_t)len, (uint8_t)OUI_ASUS[0], (uint8_t)OUI_ASUS[1], (uint8_t)OUI_ASUS[2], hexdata);
		_dprintf("%s: cmd=%s\n", __func__, cmd);
		system(cmd);
	}
}


/**
 * @brief remove guest beacon vsie
 *
 * @param hexdata vsie string
 */
void del_beacon_vsie_guest(char *hexdata)
{
    	int unit = 0, subunit = 0;
    	char word[100], *next;

    	foreach (word, nvram_safe_get("wl_ifnames"), next) {
        	if (nvram_get_int("re_mode") == 1)  // RE
            		subunit = 2;
        	else  // CAP/Router
            		subunit = 1;
        	for (; subunit <= num_of_mssid_support(unit); subunit++) {
                        char buf[] = "wlXX.XX_ifname";
                        memset(buf, 0, sizeof(buf));
                        snprintf(buf, sizeof(buf), "wl%d.%d_ifname", unit, subunit);
			if ((is_intf_up(nvram_safe_get(buf)) != -1)  // interface exist
#if defined(RTCONFIG_MLO)
                        && !is_mlo_dwb_mssid(nvram_safe_get(buf))
                        && !is_compatible_network(nvram_safe_get(buf))
#ifdef RTCONFIG_MULTILAN_MWL
                        && !is_mainFH_network(nvram_safe_get(buf))
#endif
#endif
                        )
                        del_beacon_vsie_by_unit(unit, subunit, hexdata);
        	}
        	unit++;
    	}
}

void del_beacon_vsie(char *hexdata)
{
	// 0: Beacon
	// 1: ProbeRequest
	// 2: ProbeResponse
	// 3: AuthenticationRequest
	// 4: AuthenticationRespnse
	// 5: AssocationRequest
	// 6: AssociationResponse
	// 7: ReassociationRequest
	// 8: ReassociationResponse
	char cmd[300] = {0};
	int pktflag = 0x0;
	int len = 0;
	char ifname[IFNAMSIZ] = "";
#ifdef RTCONFIG_BHCOST_OPT
        int unit = 0;
        char word[100], *next;
#endif

#ifdef RTCONFIG_WIFI_SON
	if (nvram_match("wifison_ready", "1"))
		return;
#endif
	len = 3 + strlen(hexdata)/2;	/* 3 is oui's len */

#ifdef RTCONFIG_BHCOST_OPT
	unit=0;
	foreach (word, nvram_safe_get("wl_ifnames"), next)
	{
		strlcpy(ifname, get_wififname(unit), sizeof(ifname));
		//_dprintf("%s: wl%d_ifname=%s\n", __func__,unit, ifname);
#else
	strlcpy(ifname, get_wififname(0), sizeof(ifname));;	// TODO: Should we get the band from nvram?
	//_dprintf("%s: wl0_ifname=%s\n", __func__, ifname);
#endif

	if (*ifname != '\0') {
		snprintf(cmd, sizeof(cmd), "hostapd_cli -i%s del_vsie %d DD%02X%02X%02X%02X%s",
			ifname, pktflag, (uint8_t)len, (uint8_t)OUI_ASUS[0], (uint8_t)OUI_ASUS[1], (uint8_t)OUI_ASUS[2], hexdata);
		_dprintf("%s: cmd=%s\n", __func__, cmd);
		system(cmd);
	}

#ifdef RTCONFIG_BHCOST_OPT
		unit++;
	}
#endif
}

#if defined(RTCONFIG_AMAS_WGN) 
char* get_all_lan_ifnames(void)
{
	char *wgn_ifnames = NULL, *lan_ifnames=NULL;
        char word[64], *next = NULL;
        char ath[64], *tmp = NULL;
	char br[20],result[200];
        if (!(wgn_ifnames = nvram_safe_get("wgn_ifnames")))
                return NULL;
	memset(result,0,sizeof(result));
	strlcat(result,nvram_safe_get("lan_ifnames"),sizeof(result));
	strlcat(result," ",sizeof(result));
        foreach (word, wgn_ifnames, next)
        {
		memset(br,0,sizeof(br));
		snprintf(br, sizeof(br), "%s_ifnames", word);
		if((lan_ifnames = nvram_safe_get(br)) != NULL)
		{
        		foreach (ath, lan_ifnames, tmp)
			{
				if(strstr(ath,"ath"))
				{
				 	if(!strncmp(ath, "ath0.", 5) || !strncmp(ath, "ath1.", 5) || !strncmp(ath, "ath2.", 5))
						continue;
					strlcat(result,ath,sizeof(result));
					strlcat(result," ",sizeof(result));
				}
			}
		}
        }
	result[strlen(result)-1] = '\0';
	return strdup(result);
}

int check_vlan_invalid(char *word,char *iface)
{
        int ret = 0;
        FILE *fp = NULL;
        char buf[1024] = {0};
        char vlan[20] = {0};
        char dev[20] = {0};
        int n = 2;
        if ((fp = fopen("/proc/net/vlan/config", "r")) != NULL)
	{
                // skip rows
		/* 
		 * VLAN Dev name    | VLAN ID
		 * Name-Type: VLAN_NAME_TYPE_PLUS_VID_NO_PAD
		 */
		while(n--)
                	fgets(buf, sizeof(buf), fp);

                // while loop
                while (fgets(buf, sizeof(buf), fp))
                {
                        memset(vlan, 0, sizeof(vlan));
                        memset(dev, 0, sizeof(dev));
                        if (sscanf(buf, "%s %*s %*s %*s %s", vlan, dev) != 2) continue;
                        if (!strcmp(vlan, word))
			{
				_dprintf("%s is a Vlan and we will use other names..\n",word);
				strcpy(iface,dev);
				ret=1;
				break;
                	}
                }
		fclose(fp);
        }
        else
                _dprintf("FAIL to open vlan config\n");

        return ret;
}
#endif

/*
 * int get_psta_status(int unit)
 *
 * return value
 * 	0: init
 * 	1:
 * 	2: connect and auth
 * 	3: stop
 */
int get_psta_status(int unit)
{
	int ret;
	const char *sta;
#if defined(RTCONFIG_MLO_BH) && defined(RTCONFIG_ALT_MLO)
        char ids[10];
        int mlo_mode;
        mlo_mode = isMloConnectionMode();
#endif

	unit = swap_5g_band(unit);

	sta = get_staifname(unit);
	ret = chk_assoc(sta);
	
#if defined(RTCONFIG_MLO_BH) && defined(RTCONFIG_ALT_MLO)
       if(mlo_mode)
       {
               if(!ret)
               {
                       memset(ids,0,sizeof(ids));
                       get_wpacli_status(!unit,"mlo_num_links",ids);
                       if(strlen(ids) && atoi(ids)==2)
                       {
                               if(chk_assoc(get_staifname(!unit))>0)
                                       ret=1;
                       }
               }


       }
       //_dprintf("%s:get_psta_status[%d]=%d\n",mlo_mode?"MLO":"Non-MLO",unit,ret);
#endif

	if (ret < 0) return WLC_STATE_STOPPED;
	if (ret > 0) return WLC_STATE_CONNECTED;
	return ret;
}

void Pty_stop_wlc_connect(int band)
{
	band = swap_5g_band(band);
	set_wpa_cli_cmd(band, "disconnect", 0);
}

#if defined(RTCONFIG_ALT_MLO)
int get_wpacli_status(int unit,char *target,char *result)
{
        char line[2048];
        char ctrl_sk[32];
        char *staifname;
        FILE *fp;
        staifname = get_staifname(unit);
        get_wpa_ctrl_sk(unit, ctrl_sk, sizeof(ctrl_sk));
        snprintf(line, sizeof(line), "/usr/bin/wpa_cli -p %s -i %s status", ctrl_sk, staifname);
        if ((fp = popen(line, "r")) != NULL) {
                char *p,*pt2;
                while(fgets(line, sizeof(line)-1, fp) != NULL) {
                        strip_new_line(line);
                        p = line;
                        pt2 = strstr(p, target);
                        if(pt2)
                        {
                                strcpy(result,pt2+strlen(target)+1);
                                //_dprintf("get %s =%s\n",target,result);
                                break;
                        }
                }
                pclose(fp);
        }
        return 0;
}
#endif

/*
 * get the network id of wpa_supplicant
 * return >=0: network id
 *         -1: error
 *
 * for example:
 * # wpa_cli -p /var/run/wpa_supplicant/ -i sta0 list_network
 * network id / ssid / bssid / flags
 * 3    AEE9E38CB4D40EC2794542567539B4C8        any     [CURRENT]
 */
int get_wsup_nid(const char *ifname, const char *ssid)
{
        FILE *fp;
        char buf[128];
        char nid[4], nssid[33], nbssid[18], flags[16];
        int v = -1;

        sprintf(buf, "wpa_cli -p /var/run/wpa_supplicant-%s -i %s list_networks", ifname, ifname);
        if (!(fp = popen(buf, "r"))) {
                _dprintf("%s: Can't popen %s\n", __func__, buf);
                return -1;
        }

        /* skip header: network id / ssid / bssid / flags */
        if (!fgets(buf, sizeof(buf), fp)) {
                pclose(fp);
                return -1;
        }
        while(fgets(buf,sizeof(buf),fp)!=NULL)
        {
                sscanf(buf, "%1s\t%32s\t%17s\t%15s", nid, nssid, nbssid, flags);
                if (!strcmp(nssid, ssid)) {
                        v = atoi(nid);
                        break;
                }
        }
        pclose(fp);

        return v;
}

#ifdef RTCONFIG_BHCOST_OPT
#if defined(RTAX89U)
#define PORT_UNITS 11
#elif defined(RTAC59_CD6R) || defined(RTAC59_CD6N)
#define PORT_UNITS 6
#elif defined(BD4D5) || defined(BD4_OD)
#define PORT_UNITS 2
#else
#define PORT_UNITS 6
#endif

#ifdef RTCONFIG_AMAS_ETHDETECT
//Aimesh RE: vport to eth name
static const char *query_ifname[PORT_UNITS] = { //Aimesh RE
#if defined(RTAX89U) || (GTAXY16000)
//	L1	L2	L3	L4	L5	L6	L7	L8	WAN1	WAN2	SFP+
	"eth2", "eth1", "eth0", "eth0", "eth0", "eth0", "eth0", "eth0", "eth3", "eth5", "eth4"
#elif defined(RTAC59_CD6R) || defined(RTAC59_CD6N)
//	P0	P1	P2	P3	P4	P5
	NULL,   "vlan1",NULL,   NULL,   "vlan4",NULL
#elif defined(PLAX56_XP4)
//	P0	P1	P2	P3	P4	P5
	"eth2",	"eth3",	"eth1",	NULL,	"eth4",	NULL
#elif defined(BD4D5) || defined(BD4_OD)
//	P0	P1	P2	P3	P4	P5
	"eth1",	"eth0"
#elif defined(RTBE50) || defined(TUFBE6500) || defined(TUFBE9400) || defined(RT4GBE58)
//	P0	P1	P2	P3      P4(WAN) // WAN_PORT is set to 4 (0~4) by vport_to_phy_addr[MAX_WANLAN_PORT], so a fake P3 needs to be added to shift by 1.
	"eth1", "eth1", "eth1", NULL,   "eth0"
#else
//	P0	P1	P2	P3	P4	P5
	NULL,   NULL,   NULL,   NULL,   NULL,   NULL
#endif
};
#endif

void Pty_start_wlc_connect(int band, char *bssid)
{
#ifdef RTCONFIG_MLO_BH
	int num_mlo_links = 0, l, b, nr_wlif;
	unsigned mask = 0, m;
	unsigned char mld_mac[ETHER_ADDR_LEN], mld_mac_str[sizeof("XX:XX:XX:XX:XX:XXYY")];
	char cmd[sizeof(QWPA_CLI " -i XXX dump") + IFNAMSIZ];
#endif
	char nv[16];
	int nid;
	const char *sta;
	int i, mlo_mode = 0;
	band = swap_5g_band(band);
	     
	snprintf(nv, sizeof(nv), "wlc%d_ssid", band);
	nid = get_wsup_nid(get_staifname(band), nvram_safe_get(nv));

	if (bssid != NULL) {
#ifdef RTCONFIG_MLO_BH
		sta = get_staifname(band);
		mlo_mode = isMloConnectionMode();
		strlcpy(mld_mac_str, nvram_safe_get("ss_ap_mld_mac"), sizeof(mld_mac_str));
		if (mlo_mode && *mld_mac_str != '\0' && ether_atoe(mld_mac_str, mld_mac)) {
			snprintf(cmd, sizeof(cmd), QWPA_CLI " -i %s dump", sta);
			if (exec_and_parse(cmd, "num_mlo_links=", "%*[^=]=%d", 1, &num_mlo_links))
				num_mlo_links = -1;
			nr_wlif = num_of_wl_if();
			for (mask = 0, b = 0; num_mlo_links > 0 && b < nr_wlif; b++) {
				sta = get_staifname(b);
				if (b == band) {
					mask |= 1U << band;
				} else {
					snprintf(cmd, sizeof(cmd), QWPA_CLI " -i %s dump", sta);
					if (exec_and_parse(cmd, "num_mlo_links=", "%*[^=]=%d", 1, &l))
						l = 0;
					if (l <= 0)
						continue;
					mask |= 1U << b;
				}
                        	if (nid >= 0) {
					doSystem(QWPA_CLI " -p /var/run/wpa_supplicant-%s disable_network %d", sta,nid);
					doSystem(QWPA_CLI " -p /var/run/wpa_supplicant-%s set_network %d bssid any", sta,nid);
					doSystem(QWPA_CLI " -p /var/run/wpa_supplicant-%s set_network %d preferred_ap_mld_addr %s", sta,nid, mld_mac_str);
				}
			}
			m = mask;
			while ((b = ffs(m)) > 0) {
				b--;
				sta = get_staifname(b);
                        	if (nid >= 0) {
					doSystem(QWPA_CLI " -p /var/run/wpa_supplicant-%s enable_network %d", sta,nid);
					logmessage("AMAS RE", "RE: wpacli set %s's preferred_ap_mld_addr as %s in profile %d\n", sta, mld_mac_str,nid);
				}
				m &= ~(1U << b);
			}
			if (!mask)
				mlo_mode = 0;
		}
#endif
		sta = get_staifname(band);
		if (!mlo_mode && (chk_assoc(sta)==0 || diff_current_bssid(band, bssid))) {	//Restart the network with configured BSSID
                        if (nid >= 0) {
				doSystem(QWPA_CLI " -p /var/run/wpa_supplicant-%s disable_network all", sta);
				doSystem(QWPA_CLI " -p /var/run/wpa_supplicant-%s set_network %d bssid %s", sta,nid,bssid);
				doSystem(QWPA_CLI " -p /var/run/wpa_supplicant-%s enable_network %d", sta,nid);
				logmessage("AMAS RE", "RE: wpacli set %s's bssid as %s in profile %d\n", sta, bssid,nid);
			}
		}
	}

#ifdef RTCONFIG_MLO_BH
	if (mlo_mode) {
		for(i=0; i < num_of_wl_if(); i++)
			set_wpa_cli_cmd(i, "reconnect", 0);
	}
	else
#endif
		set_wpa_cli_cmd(band, "reconnect", 0);

	return;
}

/**
 * @brief Get DFS status
 *
 * @param band Band
 * @return int Status. 1: CAC 0: Idle
 */
int amas_dfs_status(int band)
{
	int cac = 0;
	char vap[IFNAMSIZ];

	if (band != WL_5G_BAND && band != WL_5G_2_BAND)
		return 0;
	strlcpy(vap, get_wififname(band), sizeof(vap));
	if (iwpriv_get_int(vap, "get_cac_state", &cac) < 0)
		return 0;
	return !!cac;
}

#ifdef RTCONFIG_AMAS_ETHDETECT
unsigned int get_uplinkports_linkrate(char *ifname)
{
	unsigned int link_rate = 0;
	int vport = 0;

	for (vport = 0; vport < PORT_UNITS; vport++) {
		if (vport >= ARRAY_SIZE(query_ifname)) {
			dbg("%s: don't know vport %d\n", __func__, vport);
			return 0;
		}
		if (query_ifname[vport] != NULL && strstr(query_ifname[vport],ifname)) {
			if (rtkswitch_Port_phyStatus(1 << vport)) //connect
			{
				link_rate = rtkswitch_Port_phyLinkRate(1 << vport);
				break;
			}
		}
	}

	return link_rate;
}
/**
 * @brief Get the uplinkports status
 *
 * @param ifname ethernet uplink ifname
 * @return int connnected(1) or not(0)
 */

int get_uplinkports_status(char *ifname)
{
	int vport = 0;

	for (vport = 0; vport < PORT_UNITS; vport++) {
		if (vport >= ARRAY_SIZE(query_ifname)) {
			dbg("%s: don't know vport %d\n", __func__, vport);
			return 0;
		}
		if (query_ifname[vport] != NULL && strstr(query_ifname[vport],ifname)) {
			if (rtkswitch_Port_phyStatus(1 << vport))
				return 1;
		}
	}
	return 0;
}
#else
/**
 * @brief Get the uplinkports status
 *
 * @param ifname ethernet uplink ifname
 * @return int connnected(1) or not(0)
 */
int get_uplinkports_status(char *ifname)
{
	int wan_unit = wan_primary_ifunit();

	return get_wanports_status(wan_unit);
}

unsigned int get_uplinkports_linkrate(char *ifname)
{
	int speed;
	char *eth=NULL;
	speed=0;
	eth=nvram_safe_get("eth_ifnames");
	if(eth && strstr(eth,ifname))
		speed=rtkswitch_WanPort_phySpeed();
	return speed;
}

#endif
#else
void Pty_start_wlc_connect(int band)
{
	band = swap_5g_band(band);
	set_wpa_cli_cmd(band, "reconnect", 0);
}
#endif

/*
 * int Pty_get_upstream_rssi(int band)
 *
 * return value
 * 	a rssi value which is a native value. The lower the smaller signal.
 *
 * 	-91: RSSI_NO_SIGNAL
 */
int Pty_get_upstream_rssi(int band)
{
	const char *sta;
	int status, quality, signal, noise, update;
	int ret;

	if (band < 0 || band > MAX_NR_WL_IF || __absent_band(band))
		return 0;

	sta = get_staifname(swap_5g_band(band));
	extern int get_wl_status(const char *ifname, int *status, int *quality, int *signal, int *noise, unsigned int *update);
	ret = get_wl_status(sta, &status, &quality, &signal, &noise, &update);
	if (ret > 0 && status)
		return signal;

	return 0;
}

// softdown support platform //
#if defined(RTCONFIG_SOC_IPQ60XX) \
 || defined(RTCONFIG_SOC_IPQ50XX) \
 || defined(RTCONFIG_SOC_IPQ40XX) \
 || defined(RTCONFIG_SOC_IPQ53XX) \
 || (defined(RTCONFIG_SOC_IPQ8074) && defined(RTCONFIG_CFG80211))
#define QCA_SOFTDOWN_SUPPORT 1
#else
#undef QCA_SOFTDOWN_SUPPORT
#endif

/*
 * int get_wlan_service_status(int bssidx, int vifidx)
 *
 * Get the status of interface that indicate by bssidx and vifidx.
 *
 * return
 * 	-1: invalid
 * 	 0: inactive
 * 	 1: active
 */
int get_wlan_service_status(int bssidx, int vifidx)
{
	int ret;
	char athfix[8],ifname[20];
	int is_sta = 0;
	char wl_radio[] = "wlXXXX_radio";

	if (nvram_get_int("wlready") == 0)
		return -1;

	snprintf(wl_radio, sizeof(wl_radio), "wl%d_radio", bssidx);
	if (nvram_get_int(wl_radio) == 0)
		return -2;

	if (bssidx < 0 || bssidx >= MAX_NR_WL_IF || vifidx >= MAX_NO_MSSID || __absent_band(bssidx))
		return -1;

	/* CAP: main ap
	 * RE:  sta
	 */
	if (vifidx == 0) {
		if (sw_mode() == SW_MODE_AP && nvram_match("re_mode", "1")) {
			strcpy(athfix, get_staifname(swap_5g_band(bssidx)));
			is_sta = 1;
		}
		else
			__get_wlifname(swap_5g_band(bssidx), 0, athfix);
	}
	/* CAP: guest network/vif
	 * RE:  main ap/guest network/vif
	 */
	else {
		snprintf(ifname, sizeof(ifname), "wl%d.%d_ifname", swap_5g_band(bssidx), vifidx);
		if (strlen(nvram_safe_get(ifname)))
			strcpy(athfix, nvram_get(ifname));
		else
			return -1;
	}
	//_dprintf("%s: bssidx=%d, vifidx=%d, ifname=%s\n", __func__, bssidx, vifidx, athfix);

#if defined(QCA_SOFTDOWN_SUPPORT)
// softdown support platform //
	if (!is_sta) {
		if (iwpriv_get_int(athfix, "g_softdown", &ret) < 0) {
			_dprintf("EEEEEEErrr!!! please check [%s] softdown commands!!!!\n", athfix);
			return -1;
		}
		return !ret;
	}
#endif // softdown support platform //
	ret = is_intf_up(athfix);

	return ret;
}

struct calc_nr_softdown_vap_s {
	int count;	/* number of softdown/iface down/hostapd down VAP on the band */
	int total;	/* number of VAP checked */
};

static int count_nr_softdown_vap(const char *basedir, const struct dirent *de, size_t de_size, void *arg)
{
	struct calc_nr_softdown_vap_s *s = (struct calc_nr_softdown_vap_s*) arg;
	int r, c = 0;
	char proc_vlan[sizeof("/proc/net/vlan/XXX") + IFNAMSIZ], vap[IFNAMSIZ];
	char cmd[sizeof("hostapd_cli -i XXX status") + IFNAMSIZ], ans[sizeof("DISABLEDXXX")] = { 0 };

	if (!s)
		return -1;

	if (sizeof(*de) != de_size) {
		/* If size of struct dirent mismatch, make sure readdir_wrapper() and this function see same struct dirent.h.
		 * e.g., it's different in uclibc if _FILE_OFFSET_BITS=64 is defined or not.
		 */
		dbg("%s: size of struct dirent mismatch (%zu v.s. %zu)!\n", __func__, sizeof(*de), de_size);
		return -1;
	}

	snprintf(proc_vlan, sizeof(proc_vlan), "/proc/net/vlan/%s", de->d_name);
	if (f_exists(proc_vlan))
		return 0;
	strlcpy(vap, de->d_name, sizeof(vap));
	s->total++;
	if ((r = iwpriv_get_int(vap, "g_softdown", &c)) != 0) {
		//dbg("%s: Get softdown status of %s failed (return %d)\n", __func__, vap);
		return -2;
	} else if (c == 1) {
		s->count++;
		return 0;
	}

	/* check up/down status of the VAP. */
	if (!get_radio_status(vap)) {
		s->count++;
		return 0;
	}

	/* check hostapd status of the VAP. Example:
	 * # hostapd_cli -i ath1 status|head
	 * state=DISABLED
	 * phy=ath1
	 * ......
	 */
	snprintf(cmd, sizeof(cmd), "hostapd_cli -i %s status", vap);
	if (!(r = exec_and_parse(cmd, "state=", "state=%[^\n]", 1, ans)) && !strcmp(ans, "DISABLED")) {
		s->count++;
		return 0;
	}
	return 0;
}

void set_wlan_service_status(int bssidx, int vifidx, int enabled)
{
#if defined(QCA_SOFTDOWN_SUPPORT)
// softdown support platform //
	char tmp[100], prefix[] = "wlXXXXXXXXXXXXXX", athfix[]="athXXXXXX", *cur_softdown, next_softdown[2] = "";
	int sub, led, onoff;
	struct calc_nr_softdown_vap_s s = { 0 };
#else
	int cfg_stat;
	char athfix[8];
        char tmp[20],tmp2[20];
#endif // softdown support platform //
	char wl_radio[] = "wlXXXX_radio";

	if (nvram_get_int("wlready") == 0)
                return;

	snprintf(wl_radio, sizeof(wl_radio), "wl%d_radio", bssidx);
	if (nvram_get_int(wl_radio) == 0)
		return;

	if(bssidx < 0 || bssidx >= MAX_NR_WL_IF || vifidx >= MAX_NO_MSSID || __absent_band(bssidx))
		return;

	if(sw_mode() == SW_MODE_AP && nvram_match("re_mode", "1")) {
		if(vifidx <= 0) {  //for sta
			strcpy(athfix, get_staifname(swap_5g_band(bssidx)));
			doSystem("ifconfig %s %s", athfix, enabled?"up":"down");
		}
		if(vifidx==1)
			vifidx--;
	}
#if defined(QCA_SOFTDOWN_SUPPORT)
// softdown support platform //
	sub = (vifidx >= 0) ? vifidx: 0;

	if (sub > 0)
		snprintf(prefix, sizeof(prefix), "wl%d.%d_", swap_5g_band(bssidx), sub);
	else
		snprintf(prefix, sizeof(prefix), "wl%d_", swap_5g_band(bssidx));
	strcpy(athfix, nvram_safe_get(strcat_r(prefix, "ifname", tmp)));
	cur_softdown = iwpriv_get(athfix, "g_softdown");
	snprintf(next_softdown, sizeof(next_softdown), "%d", enabled? 0 : 1);
	if (!cur_softdown || strcmp(cur_softdown, next_softdown)) {
		eval(IWPRIV, athfix, "softdown", next_softdown);
	}
	if (enabled) {
#if defined(RTCONFIG_SOC_IPQ8074) || defined(RTCONFIG_SOC_IPQ53XX)
		eval("hostapd_cli", "-i", athfix, "disable");
		eval("hostapd_cli", "-i", athfix, "enable");
#elif defined(RTAC95U)
		eval("hostapd_cli", "-i", athfix, "reload");
#endif
	}
	led = get_wl_led_id(swap_5g_band(bssidx));
	onoff = (inhibit_led_on() || !enabled)? LED_OFF : LED_ON;
	if (onoff == LED_OFF && !inhibit_led_on()) {
		/* If WiFi LED is about to be turn off and it's not turn off by all LED off,
		 * check status of all VAP on the band. If one or more VAP havn't been softdown,
		 * turn on WiFi LED.
		 */
		if (!readdir_wrapper(SYS_CLASS_NET, get_wififname(bssidx), count_nr_softdown_vap, &s)
		 && s.total > 0 && s.total != s.count)
		{
			onoff = LED_ON;
		}
	} else if (onoff == LED_ON) {
		/* If WiFi LED is about to be turn on and status of all VAP have been softdown,
		 * turn off WiFi LED.
		 */
		if (!readdir_wrapper(SYS_CLASS_NET, get_wififname(bssidx), count_nr_softdown_vap, &s)
		 && s.total > 0 && s.total == s.count)
		{
			onoff = LED_OFF;
		}
	}
	led_control(led, onoff);
	// _dprintf("[%s %s softdown %s], enabled:%d\n", IWPRIV, athfix, enabled? "0":"1", enabled);
#else
	cfg_stat = nvram_get_int("cfg_alive");

	if(cfg_stat)
	{
		if (sw_mode() == SW_MODE_AP && nvram_match("re_mode", "1")) {
			snprintf(tmp, sizeof(tmp), "wl%d_qca_sched", bssidx);
			snprintf(tmp2, sizeof(tmp2), "wl%d_timesched", bssidx);
			if(nvram_get_int(tmp2)==1 ) //wifi sched is enabled
			{
				if(nvram_get_int(tmp)==0) //sched is radio-off
				{
					//_dprintf("radio[%d] should be left to wifi-sched\n",bssidx);
					return;
				}
			}
		}
	}
	set_radio(enabled, swap_5g_band(bssidx), vifidx);
#endif // softdown support platform //
}

/**
 *@brief update channel/bw/nctrlsb to
 *
 */
int resolv_bw_nctrlsb(const char *mode, int *bw, int *nctrlsb)
{
	char *p;
	char *p2;
	if(mode == NULL || bw == NULL || nctrlsb == NULL)
		return -1;

	*bw = 0;
	*nctrlsb = -1;

	if((p = strstr(mode, "11")) == NULL)
	{
		*bw=20; /*based*/
		return -3;
	}
	p += 2;

	if ((p2 = strstr(p, "HE")) != NULL) {		/* 11AHE, 11GHE */
	} else if((p2 = strstr(p, "HT")) == NULL) {	/* 11A, 11B, 11G */
		*bw = 20;
		return *bw;
	}
	p = p2 + 2;

	if (!memcmp(p, "20", 2))		/* 11NGHT20, 11AXG_HE20, 11BEG_EHT20, 11NAHT20, 11ACVHT20, 11BEA_EHT20 */
		*bw = 20;
	else if (!memcmp(p, "40", 2))		/* 11NGHT40, 11AXG_HE40, 11BEG_EHT40, 11NAHT40, 11ACVHT40, 11BEA_EHT40 */
		*bw = 40;
	else if (!memcmp(p, "80_80", 5))	/* 11ACVHT80_80, 11AXA_HE80_80 */
		*bw = 160;
	else if (!memcmp(p, "80", 2))		/* 11ACVHT80, 11AXA_HE80, 11BEA_EHT80 */
		*bw = 80;
	else if (!memcmp(p, "160", 3))		/* 11ACVHT160, 11AXA_HE160, 11BEA_EHT160 */
		*bw = 160;
	else if (!memcmp(p, "240", 3))		/* 11BEA_EHT320 */
		*bw = 240;
	else if (!memcmp(p, "320", 3))		/* 11BEA_EHT320 */
#if defined(RTCONFIG_HAS_6G) || defined(RTCONFIG_HAS_6G_2)
		*bw = 320;
#else
		*bw = 240;
#endif
	else					/* 11A, 11B, 11G */
		*bw = 20;

	if (*bw == 40) {
		if(strstr(p, "MINUS") != NULL)			/* HT40MINUS */
			*nctrlsb = 1;	//extension channel is lower,  so the control SB is higher
		else if(strstr(p, "PLUS") != NULL)		/* HT40PLUS */
			*nctrlsb = 0;	//extension channel is higher, so the control SB is lower
		else
			return -4;
	}
	return 0;
}

char acs_string[2][20]={"acs_mode=","acs_ch="};
#define IEEE80211_IOCTL_GETMODE         (SIOCIWFIRSTPRIV+17)
#define IEEE80211_PARAM_ACS_RESULT 691
char ieee80211_phymode[44][40] ={
    "IEEE80211_MODE_AUTO",    /* autoselect */
    "IEEE80211_MODE_11A",    /* 5GHz, OFDM */
    "IEEE80211_MODE_11B",    /* 2GHz, CCK */
    "IEEE80211_MODE_11G",    /* 2GHz, OFDM */
    "IEEE80211_MODE_FH",    /* 2GHz, GFSK */
    "IEEE80211_MODE_TURBO_A",    /* 5GHz, OFDM, 2x clock dynamic turbo */
    "IEEE80211_MODE_TURBO_G",    /* 2GHz, OFDM, 2x clock dynamic turbo */
    "IEEE80211_MODE_11NA_HT20",    /* 5Ghz, HT20 */
    "IEEE80211_MODE_11NG_HT20",    /* 2Ghz, HT20 */
    "IEEE80211_MODE_11NA_HT40PLUS",    /* 5Ghz, HT40 (ext ch +1) */
    "IEEE80211_MODE_11NA_HT40MINUS",   /* 5Ghz, HT40 (ext ch -1) */
    "IEEE80211_MODE_11NG_HT40PLUS",   /* 2Ghz, HT40 (ext ch +1) */
    "IEEE80211_MODE_11NG_HT40MINUS",   /* 2Ghz, HT40 (ext ch -1) */
    "IEEE80211_MODE_11NG_HT40",   /* 2Ghz, Auto HT40 */
    "IEEE80211_MODE_11NA_HT40",   /* 5Ghz, Auto HT40 */
    "IEEE80211_MODE_11AC_VHT20",   /* 5Ghz, VHT20 */
    "IEEE80211_MODE_11AC_VHT40PLUS",  /* 5Ghz, VHT40 (Ext ch +1) */
    "IEEE80211_MODE_11AC_VHT40MINUS",   /* 5Ghz  VHT40 (Ext ch -1) */
    "IEEE80211_MODE_11AC_VHT40",   /* 5Ghz, VHT40 */
    "IEEE80211_MODE_11AC_VHT80",   /* 5Ghz, VHT80 */
    "IEEE80211_MODE_11AC_VHT160",   /* 5Ghz, VHT160 */
    "IEEE80211_MODE_11AC_VHT80_80",   /* 5Ghz, VHT80_80 */
    "IEEE80211_MODE_11AXA_HE20",   /* 5GHz, HE20 */
    "IEEE80211_MODE_11AXG_HE20",   /* 2GHz, HE20 */
    "IEEE80211_MODE_11AXA_HE40PLUS",   /* 5GHz, HE40 (ext ch +1) */
    "IEEE80211_MODE_11AXA_HE40MINUS",   /* 5GHz, HE40 (ext ch -1) */
    "IEEE80211_MODE_11AXG_HE40PLUS",   /* 2GHz, HE40 (ext ch +1) */
    "IEEE80211_MODE_11AXG_HE40MINUS",   /* 2GHz, HE40 (ext ch -1) */
    "IEEE80211_MODE_11AXA_HE40",   /* 5GHz, HE40 */
    "IEEE80211_MODE_11AXG_HE40",   /* 2GHz, HE40 */
    "IEEE80211_MODE_11AXA_HE80",   /* 5GHz, HE80 */
    "IEEE80211_MODE_11AXA_HE160",   /* 5GHz, HE160 */
    "IEEE80211_MODE_11AXA_HE80_80",   /* 5GHz, HE80_80 */
    "IEEE80211_MODE_11BEA_EHT20",	/* 5GHz, EHT20 */
    "IEEE80211_MODE_11BEG_EHT20",	/* 2GHz, EHT20 */
    "IEEE80211_MODE_11BEA_EHT40PLUS",	/* 5GHz, EHT40 (ext ch +1) */
    "IEEE80211_MODE_11BEA_EHT40MINUS",	/* 5GHz, EHT40 (ext ch -1) */
    "IEEE80211_MODE_11BEG_EHT40PLUS",	/* 2GHz, EHT40 (ext ch +1) */
    "IEEE80211_MODE_11BEG_EHT40MINUS",	/* 2GHz, EHT40 (ext ch -1) */
    "IEEE80211_MODE_11BEA_EHT40",	/* 5GHz, EHT40 */
    "IEEE80211_MODE_11BEG_EHT40",	/* 2GHz, EHT40 */
    "IEEE80211_MODE_11BEA_EHT80",	/* 5GHz, EHT80 */
    "IEEE80211_MODE_11BEA_EHT160",	/* 5GHz, EHT160 */
    "IEEE80211_MODE_11BEA_EHT320",	/* 5GHz, EHT320 */
};

int update_band_info(int unit)
{
        int i,bw=0,sb=0;
	int nctrlsb=0; //otherwise
	char prefix[16]="wlxxxxxx",tmp[128],wif[IFNAMSIZ];
	u_int8_t buf[200],value[10];
	char *pt1,*pt2;
	struct iwreq wrq;
	memset(buf,0,sizeof(buf));
	memset(&wrq, 0, sizeof(wrq));
	memset(wif,0,sizeof(wif));
	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
        __get_wlifname(swap_5g_band(unit), 0, wif);

	//if wlx_sel_xxx is unset
   	if(strlen(nvram_safe_get(strcat_r(prefix, "sel_channel", tmp)))==0)
                nvram_set_int(strcat_r(prefix, "sel_channel", tmp),0);
        if(strlen(nvram_safe_get(strcat_r(prefix, "sel_bw", tmp)))==0)
                nvram_set_int(strcat_r(prefix, "sel_bw", tmp),0);
        if(strlen(nvram_safe_get(strcat_r(prefix, "sel_nctrlsb", tmp)))==0)
                nvram_set_int(strcat_r(prefix, "sel_nctrlsb", tmp),0);

	wrq.u.data.flags = IEEE80211_PARAM_ACS_RESULT;
	wrq.u.data.pointer = buf;
	wrq.u.data.length = sizeof(buf);

	if( !strlen(wif) )
                return -1;
	if (wl_ioctl(wif, IEEE80211_IOCTL_GETMODE, &wrq) < 0)
		return -1;
	if (wrq.u.data.length <= 0)
		return -1;

	if(strlen(buf))
	{
		for(i=0;i<2;i++)
		{
			pt1 = strstr(buf, acs_string[i]);
			if (pt1)
			{
				pt2 = strstr(pt1, "]");
				if(pt2)
				{
					memset(value,0,sizeof(value));
					strncpy(value,pt1+strlen(acs_string[i]),pt2-pt1-strlen(acs_string[i]));
					chomp((char*)value);
					if(strstr(acs_string[i],"ch"))
					{
						if (nvram_get_int(strcat_r(prefix, "sel_channel", tmp))!=atoi(value))  //update ch
						{
							//_dprintf("update ch=%d ....\n",atoi(value));
							nvram_set_int(strcat_r(prefix, "sel_channel", tmp),atoi(value));
						}
					}
					else if(strstr(acs_string[i],"mode"))
					{
						resolv_bw_nctrlsb(ieee80211_phymode[atoi(value)],&bw,&sb);
						if(bw==40)
						{
							if(sb==0) //high
								nctrlsb=1;
							else //lower
								nctrlsb=0;
						}
						if (nvram_get_int(strcat_r(prefix, "sel_bw", tmp))!=bw)  //update bw
						{
							//_dprintf("update bw=%d ....\n",bw);
							nvram_set_int(strcat_r(prefix, "sel_bw", tmp),bw);
						}
						if(nvram_get_int(strcat_r(prefix, "sel_nctrlsb", tmp))!=nctrlsb) //update side band
						{
							//_dprintf("update sb=%d ....\n",nctrlsb);
							nvram_set_int(strcat_r(prefix, "sel_nctrlsb", tmp),nctrlsb);
						}

					}

				}
			}
		}
	}
	return 0;
}


/**
 * @brief Post sent action to amas_wlcconnect
 *
 */
void post_sent_action() {}

/**
 * @brief Backhaul changed sysdeps function
 *
 * @param iftype BH defif
 */
void post_bh_changed(int iftype) {

}


/*
 * get_radar_status()
 *
 * Emily:
 * If platform in RE mode can re-associate to PAP after all DFS channels are blocked by radar 30 minutes, it don't need this function.
 *
 * Lencer:
 * In fact, this is not called in asuswrt-router-3004 branch in a 10 hour test.
 *
 */

int get_radar_status(int bssidx)
{
	return 0;
}

/* Pty_procedure_check()
 *
 * a wordaround in DFS environment
 *
 * On dualband platform: RE would not limite DFS channel
 * On triband  platform: RE would use +112/80 +136u +108l +100l +140 +132 +116" with higher pritority when radar is detected.
 *
 */
int Pty_procedure_check(int unit, int wlif_count)
{
	return 0;
}

char *get_pap_bssid(int unit, char *bssid_buf, int buf_len)
{
	struct iwreq wrq;
	const char *sta;
	int ret;
	unsigned char *mac;

	*bssid_buf = '\0';
	sta = get_staifname(swap_5g_band(unit));
	if ((ret = get_ap_mac(sta, &wrq)) >= 0) {
		mac = wrq.u.ap_addr.sa_data;
		snprintf(bssid_buf, buf_len, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	return bssid_buf;
}

int diff_current_bssid(int unit, char bssid_str[])
{
	char cur_bssid[18];
	int i,diff;

	get_pap_bssid(unit, cur_bssid, sizeof(cur_bssid));
	if (strcmp(cur_bssid, "00:00:00:00:00:00") != 0) {
		for(i=0; i<17; i++) {
			diff = abs((int)(*(cur_bssid+i)-*(bssid_str+i)));
			if(diff==0 || diff==32)
				continue;
			else {
				logmessage("AMAS RE", "Change %s's bssid from %s to %s\n", unit?"5G":"2G", cur_bssid, bssid_str);
				return 1;
			}

		}
		logmessage("AMAS RE", "Current serving-ap and the best serving-ap are the same, no restart required.\n");
		return 0;
	}
	else
		logmessage("AMAS RE", "Can not get %s's current pap bssid!!\n", unit?"5G":"2G");

	return 1;
}

int wl_get_bw(int unit)
{
	char athfix[IFNAMSIZ];
	int bw, nctrlsb;

	__get_wlifname(swap_5g_band(unit), 0, athfix);
	get_bw_nctrlsb(athfix, &bw, &nctrlsb);
	return bw;
}

/* Return bitmask, CFG_BW_XXM, of @bw
 * @bw:		one of 20/40/80/160
 * @return:	bitmask of one of CFG_BW_XXM
 */
int get_cfg_bw_mask_by_bw(int bw)
{
	int ret = 0;
	if (bw == 20)
		ret = CFG_BW_20M;
	else if (bw == 40)
		ret = CFG_BW_40M;
	else if (bw == 80)
		ret = CFG_BW_80M;
	else if (bw == 160)
		ret = CFG_BW_160M;
	else if (bw == 240)
		ret = CFG_BW_240M;
	else {
		dbg("%s: Unknown bandwidth %d\n", __func__, bw);
	}

	return ret;
}

/* Get supported bandwidth without checking /proc/athX/iv_config.
 * @bwcap:	pointer to store bitmask of CFG_BW_XXM, e.g., CFG_BW_20M | CFG_BW_40M
 * @return:
 * 	0:	success
 *     -1:	error
 */
int __wl_get_bw_cap(int unit, int *bwcap)
{
	if (unit < 0 || unit >= WL_NR_BANDS || !bwcap || absent_band(unit))
		return -1;

	if (unit == WL_2G_BAND)
		*bwcap = CFG_BW_20M | CFG_BW_40M;		/* 40MHz */
	else if (is_5g(unit)) {
		*bwcap = CFG_BW_20M | CFG_BW_40M | CFG_BW_80M;	/* 11AC 80MHz */
#ifdef RTCONFIG_BW160M
		*bwcap |= CFG_BW_160M;
#endif
#if defined(RTCONFIG_BW240M)
		*bwcap |= CFG_BW_240M;
#endif
	}
#if defined(RTCONFIG_HAS_6G)
	else if (unit == WL_6G_BAND) {
		*bwcap = CFG_BW_20M | CFG_BW_40M | CFG_BW_80M;
#if defined(RTCONFIG_BW160M)
		*bwcap |= CFG_BW_160M;
#if defined(RTCONFIG_BW320M)
		*bwcap |= CFG_BW_320M;
#endif
#endif
	}
#endif
	else {
		dbg("%s: unit %d is not supported yet!\n", __func__, unit);
		return -1;
	}

	return 0;
}

/* Reference to ieee80211_phymode and ieee80211_convert_mode() in qca-wifi. */
static const char *phymode_str_tbl[IEEE80211_MODE_MAX] = {
	"AUTO", "11A", "11B", "11G", "FH",						/* 0 */
	"TA", "TG", "11NAHT20", "11NGHT20", "11NAHT40PLUS",				/* 5 */
	"11NAHT40MINUS", "11NGHT40PLUS", "11NGHT40MINUS", "11NGHT40", "11NAHT40",	/* 10 */
	"11ACVHT20", "11ACVHT40PLUS", "11ACVHT40MINUS", "11ACVHT40", "11ACVHT80",	/* 15 */
	"11ACVHT160", "11ACVHT80_80", "11AHE20", "11GHE20", "11AHE40PLUS",		/* 20 */
	"11AHE40MINUS", "11GHE40PLUS", "11GHE40MINUS", "11AHE40", "11GHE40",		/* 25 */
	"11AHE80", "11AHE160", "11AHE80_80", "11AEHT20", "11GEHT20",			/* 30 */
	"11AEHT40PLUS", "11AEHT40MINUS", "11GEHT40PLUS", "11GEHT40MINUS", "11AEHT40",	/* 35 */
	"11GEHT40", "11AEHT80", "11AEHT160", "11AEHT320"				/* 40 */
};

/* Get mode string of @phymode.
 * @return:	pointer to mode string or "unknown phymode %d".
 */
const char *phymode_str(int phymode)
{
	static char mode[sizeof("Unknown phymode XXXXXX")];
	const char *ret = mode;

	if (phymode >= 0 && phymode < ARRAY_SIZE(phymode_str_tbl)) {
		ret = phymode_str_tbl[phymode];
	} else {
		snprintf(mode, sizeof(mode), "Unknown phymode %d", phymode);
	}
	return ret;
}

/* Get bandwidth of @mode
 * @mode:	parameter of private mode command, e.g., 11GHE20
 * @return:	one of 20/40/80/160/240/320
 */
int get_bw_by_mode_str(const char *mode)
{
	int r = 20;
	if (!mode)
		return r;

	if (strstr(mode, "320")) {
#if defined(RTCONFIG_HAS_6G) || defined(RTCONFIG_HAS_6G_2)
		r = 320;
#else
		r = 240;
#endif
	}
	else if (strstr(mode, "240"))
		r = 240;
	else if (strstr(mode, "160"))
		r = 160;
	else if (strstr(mode, "80"))
		r = 80;
	else if (strstr(mode, "40"))
		r = 40;
	else if (strstr(mode, "20"))
		r = 20;
	else {
		r = 20;
		//dbg("%s: unknown mode [%s]\n", __func__, mode);
	}

	return r;
}

/* Get bandwidth of enum ieee80211_phymode
 * @unit:	enum wl_band_id, not used unless phymode is auto.
 * @phymode:	IEEE80211_MODE_11NA_XXX, e.g. IEEE80211_MODE_11NA_HT40PLUS
 * @return:	one of 20/40/80/160
 */
int get_bw_by_phymode(int unit, int phymode)
{
	int ret = 20;

	if (__absent_band(unit))
		return ret;

	if (phymode == IEEE80211_MODE_AUTO && unit >= 0 && unit < WL_NR_BANDS) {
		if (unit == WL_2G_BAND) {
			return 40;
		}
		else if (is_5g(unit)) {
#if defined(RTCONFIG_BW240M)
			if (w75g_240m_capable(unit))
				return 240;
			else
				return 160;
#elif defined(RTCONFIG_BW160M)
			return 160;
#else
			return 80;
#endif
		}
#if defined(RTCONFIG_HAS_6G)
		else if (is_6g(unit)) {
#if defined(RTCONFIG_BW320M)
			return 320;
#elif defined(RTCONFIG_BW160M)
			return 160;
#else
			return 80;
#endif
		}
#endif
	}

	switch (phymode) {
	case IEEE80211_MODE_11BEA_EHT320:	/* fall-through */
		ret = 320;
		if (is_5g(unit))
			ret = 240;
		break;
	case IEEE80211_MODE_11AC_VHT160:	/* fall-through */
	case IEEE80211_MODE_11AXA_HE160:	/* fall-through */
	case IEEE80211_MODE_11BEA_EHT160:	/* fall-through */
		ret = 160;
		break;
	case IEEE80211_MODE_11AC_VHT80:		/* fall-through */
	case IEEE80211_MODE_11AC_VHT80_80:	/* fall-through */
	case IEEE80211_MODE_11AXA_HE80:		/* fall-through */
	case IEEE80211_MODE_11AXA_HE80_80:	/* fall-through */
	case IEEE80211_MODE_11BEA_EHT80:	/* fall-through */
		ret = 80;
		break;

	case IEEE80211_MODE_11NA_HT40PLUS:	/* fall-through */
	case IEEE80211_MODE_11NA_HT40MINUS:	/* fall-through */
	case IEEE80211_MODE_11NG_HT40PLUS:	/* fall-through */
	case IEEE80211_MODE_11NG_HT40MINUS:	/* fall-through */
	case IEEE80211_MODE_11NG_HT40:		/* fall-through */
	case IEEE80211_MODE_11NA_HT40:		/* fall-through */
	case IEEE80211_MODE_11AC_VHT40PLUS:	/* fall-through */
	case IEEE80211_MODE_11AC_VHT40MINUS:	/* fall-through */
	case IEEE80211_MODE_11AC_VHT40:		/* fall-through */
	case IEEE80211_MODE_11AXA_HE40PLUS:	/* fall-through */
	case IEEE80211_MODE_11AXA_HE40MINUS:	/* fall-through */
	case IEEE80211_MODE_11AXG_HE40PLUS:	/* fall-through */
	case IEEE80211_MODE_11AXG_HE40MINUS:	/* fall-through */
	case IEEE80211_MODE_11AXA_HE40:		/* fall-through */
	case IEEE80211_MODE_11AXG_HE40:		/* fall-through */
	case IEEE80211_MODE_11BEG_EHT40:	/* fall-through */
	case IEEE80211_MODE_11BEA_EHT40:	/* fall-through */
	case IEEE80211_MODE_11BEG_EHT40PLUS:	/* fall-through */
	case IEEE80211_MODE_11BEG_EHT40MINUS:	/* fall-through */
	case IEEE80211_MODE_11BEA_EHT40PLUS:	/* fall-through */
	case IEEE80211_MODE_11BEA_EHT40MINUS:	/* fall-through */
		ret = 40;
		break;

	case IEEE80211_MODE_AUTO:		/* fall-through */
	case IEEE80211_MODE_11A:		/* fall-through */
	case IEEE80211_MODE_11B:		/* fall-through */
	case IEEE80211_MODE_11G:		/* fall-through */
	case IEEE80211_MODE_FH:			/* fall-through */
	case IEEE80211_MODE_TURBO_A:		/* fall-through */
	case IEEE80211_MODE_TURBO_G:		/* fall-through */
	case IEEE80211_MODE_11NA_HT20:		/* fall-through */
	case IEEE80211_MODE_11NG_HT20:		/* fall-through */
	case IEEE80211_MODE_11AC_VHT20:		/* fall-through */
	case IEEE80211_MODE_11AXA_HE20:		/* fall-through */
	case IEEE80211_MODE_11AXG_HE20:		/* fall-through */
	case IEEE80211_MODE_11BEG_EHT20:	/* fall-through */
	case IEEE80211_MODE_11BEA_EHT20:	/* fall-through */
		ret = 20;
		break;
	default:
		dbg("%s: Unknown phymode %d\n", __func__, phymode);
	}

	return ret;
}

static const uint64_t g_bw160ch_m[] = {
	CH36_M | CH40_M | CH44_M | CH48_M | CH52_M | CH56_M | CH60_M | CH64_M,
	CH100_M | CH104_M | CH108_M | CH112_M | CH116_M | CH120_M | CH124_M | CH128_M,
	0
}, g_bw80ch_m[] = {
	CH36_M | CH40_M | CH44_M | CH48_M,
	CH52_M | CH56_M | CH60_M | CH64_M,
	CH100_M | CH104_M | CH108_M | CH112_M,
	CH116_M | CH120_M | CH124_M | CH128_M,
	CH132_M | CH136_M | CH140_M | CH144_M,
	CH149_M | CH153_M | CH157_M | CH161_M,
	0
}, g_bw40ch_m[] = {
	CH36_M | CH40_M,
	CH44_M | CH48_M,
	CH52_M | CH56_M,
	CH60_M | CH64_M,
	CH100_M | CH104_M,
	CH108_M | CH112_M,
	CH116_M | CH120_M,
	CH124_M | CH128_M,
	CH132_M | CH136_M,
	CH140_M | CH144_M,
	CH149_M | CH153_M,
	CH157_M | CH161_M,
	0
#if defined(RTCONFIG_BW240M)
}, g_bw240ch_m[] = {
	CH100_M | CH104_M | CH108_M | CH112_M, CH116_M | CH120_M | CH124_M | CH128_M, CH132_M | CH136_M | CH140_M | CH144_M,
	0
#endif
#if defined(RTCONFIG_HAS_6G)
#if defined(RTCONFIG_BW320M)
}, g_6gbw320ch_m[] = {
	B6GCH1_M   | B6GCH5_M   | B6GCH9_M   | B6GCH13_M  | B6GCH17_M  | B6GCH21_M  | B6GCH25_M  | B6GCH29_M  |
	B6GCH33_M  | B6GCH37_M  | B6GCH41_M  | B6GCH45_M  | B6GCH49_M  | B6GCH53_M  | B6GCH57_M  | B6GCH61_M,
	B6GCH33_M  | B6GCH37_M  | B6GCH41_M  | B6GCH45_M  | B6GCH49_M  | B6GCH53_M  | B6GCH57_M  | B6GCH61_M  |
	B6GCH65_M  | B6GCH69_M  | B6GCH73_M  | B6GCH77_M  | B6GCH81_M  | B6GCH85_M  | B6GCH89_M  | B6GCH93_M,
	B6GCH65_M  | B6GCH69_M  | B6GCH73_M  | B6GCH77_M  | B6GCH81_M  | B6GCH85_M  | B6GCH89_M  | B6GCH93_M  |
	B6GCH97_M  | B6GCH101_M | B6GCH105_M | B6GCH109_M | B6GCH113_M | B6GCH117_M | B6GCH121_M | B6GCH125_M,
	B6GCH97_M  | B6GCH101_M | B6GCH105_M | B6GCH109_M | B6GCH113_M | B6GCH117_M | B6GCH121_M | B6GCH125_M |
	B6GCH129_M | B6GCH133_M | B6GCH137_M | B6GCH141_M | B6GCH145_M | B6GCH149_M | B6GCH153_M | B6GCH157_M,
	B6GCH129_M | B6GCH133_M | B6GCH137_M | B6GCH141_M | B6GCH145_M | B6GCH149_M | B6GCH153_M | B6GCH157_M |
	B6GCH161_M | B6GCH165_M | B6GCH169_M | B6GCH173_M | B6GCH177_M | B6GCH181_M | B6GCH185_M | B6GCH189_M,
	B6GCH161_M | B6GCH165_M | B6GCH169_M | B6GCH173_M | B6GCH177_M | B6GCH181_M | B6GCH185_M | B6GCH189_M |
	B6GCH193_M | B6GCH197_M | B6GCH201_M | B6GCH205_M | B6GCH209_M | B6GCH213_M | B6GCH217_M | B6GCH221_M,
	0
#endif
}, g_6gbw160ch_m[] = {
	B6GCH1_M   | B6GCH5_M   | B6GCH9_M   | B6GCH13_M  | B6GCH17_M  | B6GCH21_M  | B6GCH25_M  | B6GCH29_M,
	B6GCH33_M  | B6GCH37_M  | B6GCH41_M  | B6GCH45_M  | B6GCH49_M  | B6GCH53_M  | B6GCH57_M  | B6GCH61_M,
	B6GCH65_M  | B6GCH69_M  | B6GCH73_M  | B6GCH77_M  | B6GCH81_M  | B6GCH85_M  | B6GCH89_M  | B6GCH93_M,
	B6GCH97_M  | B6GCH101_M | B6GCH105_M | B6GCH109_M | B6GCH113_M | B6GCH117_M | B6GCH121_M | B6GCH125_M,
	B6GCH129_M | B6GCH133_M | B6GCH137_M | B6GCH141_M | B6GCH145_M | B6GCH149_M | B6GCH153_M | B6GCH157_M,
	B6GCH161_M | B6GCH165_M | B6GCH169_M | B6GCH173_M | B6GCH177_M | B6GCH181_M | B6GCH185_M | B6GCH189_M,
	B6GCH193_M | B6GCH197_M | B6GCH201_M | B6GCH205_M | B6GCH209_M | B6GCH213_M | B6GCH217_M | B6GCH221_M,
	0
}, g_6gbw80ch_m[] = {
	B6GCH1_M   | B6GCH5_M   | B6GCH9_M   | B6GCH13_M,
	B6GCH17_M  | B6GCH21_M  | B6GCH25_M  | B6GCH29_M,
	B6GCH33_M  | B6GCH37_M  | B6GCH41_M  | B6GCH45_M,
	B6GCH49_M  | B6GCH53_M  | B6GCH57_M  | B6GCH61_M,
	B6GCH65_M  | B6GCH69_M  | B6GCH73_M  | B6GCH77_M,
	B6GCH81_M  | B6GCH85_M  | B6GCH89_M  | B6GCH93_M,
	B6GCH97_M  | B6GCH101_M | B6GCH105_M | B6GCH109_M,
	B6GCH113_M | B6GCH117_M | B6GCH121_M | B6GCH125_M,
	B6GCH129_M | B6GCH133_M | B6GCH137_M | B6GCH141_M,
	B6GCH145_M | B6GCH149_M | B6GCH153_M | B6GCH157_M,
	B6GCH161_M | B6GCH165_M | B6GCH169_M | B6GCH173_M,
	B6GCH177_M | B6GCH181_M | B6GCH185_M | B6GCH189_M,
	B6GCH193_M | B6GCH197_M | B6GCH201_M | B6GCH205_M,
	B6GCH209_M | B6GCH213_M | B6GCH217_M | B6GCH221_M,
	0
}, g_6gbw40ch_m[] = {
	B6GCH1_M   | B6GCH5_M,
	B6GCH9_M   | B6GCH13_M,
	B6GCH17_M  | B6GCH21_M,
	B6GCH25_M  | B6GCH29_M,
	B6GCH33_M  | B6GCH37_M,
	B6GCH41_M  | B6GCH45_M,
	B6GCH49_M  | B6GCH53_M,
	B6GCH57_M  | B6GCH61_M,
	B6GCH65_M  | B6GCH69_M,
	B6GCH73_M  | B6GCH77_M,
	B6GCH81_M  | B6GCH85_M,
	B6GCH89_M  | B6GCH93_M,
	B6GCH97_M  | B6GCH101_M,
	B6GCH105_M | B6GCH109_M,
	B6GCH113_M | B6GCH117_M,
	B6GCH121_M | B6GCH125_M,
	B6GCH129_M | B6GCH133_M,
	B6GCH137_M | B6GCH141_M,
	B6GCH145_M | B6GCH149_M,
	B6GCH153_M | B6GCH157_M,
	B6GCH161_M | B6GCH165_M,
	B6GCH169_M | B6GCH173_M,
	B6GCH177_M | B6GCH181_M,
	B6GCH185_M | B6GCH189_M,
	B6GCH193_M | B6GCH197_M,
	B6GCH201_M | B6GCH205_M,
	B6GCH209_M | B6GCH213_M,
	B6GCH217_M | B6GCH221_M,
	0
#endif
};

static const struct bw_chlist_s {
	int bw;
	const uint64_t *mask;
} g_bw_5gchlist_tbl[] = {
#if defined(RTCONFIG_BW240M)
	{ 240, g_bw240ch_m },
#endif
	{ 160, g_bw160ch_m },
	{  80, g_bw80ch_m },
	{  40, g_bw40ch_m },
	{   0, NULL },
#if defined(RTCONFIG_HAS_6G)
}, g_bw_6gchlist_tbl[] = {
#if defined(RTCONFIG_BW320M)
	{ 320, g_6gbw320ch_m },
#endif
	{ 160, g_6gbw160ch_m },
	{  80, g_6gbw80ch_m },
	{  40, g_6gbw40ch_m },
	{   0, NULL },
#endif
};

/* Return maximum bandwidth of channel list @avbl_ch_mask.
 * Currently only 5G is supported.
 * return:	20, 40, 80, 160, 240
 */
int get_max_bw_by_chlist(int band, uint64_t avbl_ch_mask)
{
	int bw = 0;
	const uint64_t *m;
	const struct bw_chlist_s *p;

	if (!avbl_ch_mask)
		return 0;

	if (band < 0 || band >= WL_NR_BANDS || __absent_band(band))
		return 0;
	else if (is_5g(band))
		p = g_bw_5gchlist_tbl;
#if defined(RTCONFIG_HAS_6G)
	else if (is_6g(band))
		p = g_bw_6gchlist_tbl;
#endif
	else {
		if (band != WL_2G_BAND) {
			_dprintf("%s: band %d is not supported!\n", __func__, band);
		}
		return 0;
	}

	for (; !bw && p->bw && p->mask; ++p) {
		for (m = p->mask; !bw && m && *m; ++m) {
			if ((avbl_ch_mask & *m) < *m)
				continue;
			bw = p->bw;
		}
	}

	/*dbg("%s: avbl_ch_mask %" PRIu64 " [%s] return bw [%d]\n", __func__,
		avbl_ch_mask, bitmask2chlist(band, avbl_ch_mask, " "), bw);*/
	if (!bw && avbl_ch_mask)
		bw = 20;

	return bw;
}

/* Guess maximum bandwidth by configuration.
 * @band
 * @return:	20/40/80/160/...
 */
int get_max_bw_by_cfg(int band)
{
	int bw, wl_bw;
	char m_pfix[sizeof("wlX_XX")];

	if (__absent_band(band))
		return 0;

	snprintf(m_pfix, sizeof(m_pfix), "wl%d_", band);
	wl_bw = nvram_pf_get_int(m_pfix, "bw");
	bw = 20;
	if (wl_bw == WL_BW_AUTO || wl_bw == WL_BW_40)
		bw = 40;
	if (wl_bw == WL_BW_AUTO || wl_bw == WL_BW_80)
		bw = 80;
#if defined(RTCONFIG_BW160M)
	if ((wl_bw == WL_BW_AUTO || wl_bw == WL_BW_160)
	 && nvram_pf_match(m_pfix, "bw_160", "1"))
		bw = 160;
#endif
#if defined(RTCONFIG_BW240M)
	if ((wl_bw == WL_BW_AUTO || wl_bw == WL_BW_240) && is_5g(band)
	 && nvram_pf_match(m_pfix, "bw_240", "1"))
		bw = 240;
#endif
#if defined(RTCONFIG_BW320M)
	if ((wl_bw == WL_BW_AUTO || wl_bw == WL_BW_320) && is_6g(band)
	 && nvram_pf_match(m_pfix, "bw_320", "1"))
		bw = 320;
#endif
	return bw;
}

/* Get bandwidth from @t_bw
 * @t_bw:	Same format in gen_ath_config().
 * @return:	20/40/80/160/...
 */
int get_bw_from_t_bw(int band, const char *t_bw)
{
	int bw;
	const char *p;
	char m_pfix[sizeof("wlX_XX")];

	if (__absent_band(band))
		return 0;

	if (!t_bw)
		return 20;

	snprintf(m_pfix, sizeof(m_pfix), "wl%d_", band);
	/* get bandwidth from t_bw. */
	bw = 20;
	if ((p = strstr(t_bw, "40")) && (p == t_bw || *(p - 1) != '2'))
		bw = 40;
	if (strstr(t_bw, "80"))
		bw = 80;
#if defined(RTCONFIG_BW160M)
	if (strstr(t_bw, "160") && nvram_pf_match(m_pfix, "bw_160", "1"))
		bw = 160;
#endif
#if defined(RTCONFIG_BW240M)
	if ((strstr(t_bw, "240") || strstr(t_bw, "320")) && is_5g(band)
	 && nvram_pf_match(m_pfix, "bw_240", "1"))
		bw = 240;
#endif
#if defined(RTCONFIG_BW320M)
	if (strstr(t_bw, "320") && is_5g(band)
	 && nvram_pf_match(m_pfix, "bw_320", "1"))
		bw = 320;
#endif
	return bw;
}

/* Return maximum 5G bandwidth of channel list, limited by configuration and @chlist_m, for ACS purpose.
 * all DFS channel:		@bw isn't supported if one of DFS channel is blocked.
 * non-DFS/mixed channel:	@bw isn't supported if all channels are blocked.
 * return:	20, 40, 80, 160, 240
 */
int get_max_5gbw_by_chlist_for_acs(int band, uint64_t chlist_m)
{
	int max_bw, bw = 0;
	const uint64_t *m;
	const struct bw_chlist_s *p;

	if (!chlist_m)
		return 0;

	if (band < 0 || band >= WL_NR_BANDS || __absent_band(band) || !is_5g(band))
		return 0;

	max_bw = get_max_bw_by_cfg(band);
	for (p = g_bw_5gchlist_tbl; !bw && p->bw && p->mask; ++p) {
		for (m = p->mask; !bw && m && *m; ++m) {
			if (p->bw > max_bw)
				continue;
			if ((chlist_m & *m) == (*m & DFS_CH_M)) {
				/* all DFS channels */
				if ((chlist_m & *m) != *m)
					continue;
			} else {
				/* non-DFS channels or mixed */
				if ((!(chlist_m & *m & ~DFS_CH_M) && (*m & ~DFS_CH_M))
				 || (!(chlist_m & *m &  DFS_CH_M) && (*m &  DFS_CH_M)))
					continue;
			}
			bw = p->bw;
		}
	}

	/*dbg("%s: chlist_m %" PRIu64 " [%s] return bw [%d]\n", __func__,
		chlist_m, bitmask2chlist(band, chlist_m, " "), bw);*/
	if (!bw && chlist_m)
		bw = 20;

	return bw;
}

/* Do ACS directly if available channel is able to offser channel for current bandwidth.
 * If not, switch to smaller bandwith first and the do ACS.
 * Because qca-wifi driver choose channel based on mode if it has been setup.
 * Fet mode as AUTO and then ACS rarely success.
 * @band:		enum wl_band_id
 * @vap:		VAP interface name, e.g., athX.
 * @avbl_ch_mask:	available channel bitmask, same as return value of chlist5g2bitmask(),
 * 			bit-wise combination of CHXXX_M
 */
int acs_and_change_bw_if_need(int band, const char *vap, uint64_t avbl_ch_mask)
{
	int orig_bw, new_bw;
	char *p, orig_mode[32] = "", new_mode[32] = "", main_prefix[sizeof("wlXXX_")];

	/*dbg("%s: band %d vap %s avbl_ch_mask 0x%" PRIx64 "\n", __func__,
		band, vap? : "NULL", avbl_ch_mask);*/
	if (band < 0 || band >= WL_NR_BANDS || !vap || !iface_exist(vap))
		return -1;
	if (!avbl_ch_mask) {
		_dprintf("%s: vap %s avbl_ch_mask 0x%" PRIx64 " none of any channel available!\n",
			__func__, vap, avbl_ch_mask);
		logmessage("WIFI", "%s: vap %s avbl_ch_mask 0x%" PRIx64 ", none of any channel available!\n",
			__func__, vap, avbl_ch_mask);
		goto acs_and_reduce_bw_if_need_do_acs;
	}

	strlcpy(orig_mode, iwpriv_get(vap, "get_mode")? : "", sizeof(orig_mode));
	if ((orig_bw = get_bw_by_mode_str(orig_mode)) <= 20)
		goto acs_and_reduce_bw_if_need_do_acs;

	if (orig_bw == 320 && is_5g(band))
		orig_bw = 240;
	new_bw = get_max_bw_by_chlist(band, avbl_ch_mask);
	snprintf(main_prefix, sizeof(main_prefix), "wl%d_", band);
	if (is_5g(band) && new_bw > 80) {
		if (new_bw >= 240 && (
#if defined(RTCONFIG_BW240M)
		     !nvram_pf_match(main_prefix, "bw_240", "1") ||
#endif
		     !w75g_240m_capable(band)))
			new_bw = 160;

		if (new_bw >= 160
#if defined(RTCONFIG_BW160M)
		 && !nvram_pf_match(main_prefix, "bw_160", "1")
#endif
		)
			new_bw = 80;
	}

	if (new_bw > 0 && orig_bw > new_bw && !strncmp(orig_mode, "11", 2)) {
		strlcpy(new_mode, orig_mode, sizeof(new_mode));
		for (p = new_mode + strlen("11"); *p != '\0'; ++p) {
			if (!isdigit(*p))
				continue;
			snprintf(p, (new_mode + sizeof(new_mode) - p), "%d", new_bw);
			break;
		}
		if (strcmp(orig_mode, new_mode)) {
			_dprintf("%s: vap %s mode %s -> %s, avbl_chlist [%s]\n", __func__,
				vap, orig_mode, new_mode, bitmask2chlist(band, avbl_ch_mask, ","));
			logmessage("WIFI", "%s: vap %s mode %s -> %s, avbl_chlist [%s]\n", __func__,
				vap, orig_mode, new_mode, bitmask2chlist(band, avbl_ch_mask, ","));
			eval(IWPRIV, (char*) vap, "mode", new_mode);
			sleep(1);
		}
	}

acs_and_reduce_bw_if_need_do_acs:
	eval("iwconfig", (char*) vap, "channel", "0");
	return 0;
}

/*
 * wl_get_bw_cap(unit, *bwcap)
 *
 * bwcap
 * 	0x01 (CFG_BW_20M) = 20 MHz
 * 	0x02 (CFG_BW_40M) = 40 MHz
 * 	0x04 (CFG_BW_80M) = 80 MHz
 * 	0x08 (CFG_BW_160M) = 160 MHz
 * 	0x10 (CFG_BW_320M) = 320 MHz @ 6GHz
 * 	0x20 (CFG_BW_240M) = 240 MHz @ 5GHz
 *
 * 	ex: 5G support 20,40,80
 * 	*bwcap = 0x01 | 0x02 | 0x04
 */
int wl_get_bw_cap(int unit, int *bwcap)
{
	int r, iv_des_hw_mode = 0, iv_cur_mode = 0, exp_bw, cur_bw, m, mask;
	char mode[32] = "", cmd[sizeof("cat /proc/XXX/iv_config") + IFNAMSIZ];
	char prefix[sizeof("wlXXX_")];

	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	if ((r = __wl_get_bw_cap(unit, bwcap)) != 0)
		return r;

	/* Channel/bandwidth checking is executed per 30 seconds. Don't run full check if possible. */
	if (unit != WL_2G_BAND || !nvram_match(WLREADY, "1"))
		return 0;

	strlcpy(mode, iwpriv_get(get_wififname(unit), "get_mode")? : "", sizeof(mode));
	if (strstr(mode, "HT40") || strstr(mode, "HE40")) {
		return 0;
	}

	snprintf(cmd, sizeof(cmd), "cat /proc/%s/iv_config", get_wififname(unit));
	if (nvram_pf_match(prefix, "radio", "0")
	 && (!strcmp(mode, "11A") || !strcmp(mode, "11B"))) {
		/* For SPF11.5, if radio is turn off, mode of VAP would be 11A or 11B and can't be changed.
		 * In this case, we can't get supported bandwidth from mode. Parsing iv_des_hw_mode and
		 * mask out unsupported higher bandwidth instead.
		 */
		exec_and_parse(cmd, "iv_des_hw_mode", "%*[^:]:%d", 1, &iv_des_hw_mode);
		exp_bw = get_bw_by_phymode(unit, iv_des_hw_mode);
		if ((m = get_cfg_bw_mask_by_bw(exp_bw)) != 0) {
			mask = m | (m - 1);
			*bwcap &= mask;
		}
	} else {
		/* 2G is not 40MHz, remove 40MHz capability if it's failed to set as 40MHz. */
		if (exec_and_parse(cmd, "iv_des_hw_mode", "%*[^:]:%d", 1, &iv_des_hw_mode) || !iv_des_hw_mode
		 || exec_and_parse(cmd, "iv_cur_mode", "%*[^:]:%d", 1, &iv_cur_mode) || !iv_cur_mode) {
			dbg("%s: Failed to parse iv_des_hw_mode %d or iv_cur_mode %d from %s\n",
				__func__, iv_des_hw_mode, iv_cur_mode, cmd + 4);
			/* Return safe value when radio not ready. */
			*bwcap = CFG_BW_20M;	/* 20MHz only */
			if (is_5g(unit))
				*bwcap = CFG_BW_20M | CFG_BW_40M | CFG_BW_80M;/* 20/40/80MHz */
			return r;
		}
		if (iv_cur_mode == iv_des_hw_mode)
			return 0;
		exp_bw = get_bw_by_phymode(unit, iv_des_hw_mode);
		cur_bw = get_bw_by_phymode(unit, iv_cur_mode);
		if (exp_bw > cur_bw)
			*bwcap &= ~(get_cfg_bw_mask_by_bw(exp_bw));
	}

	return 0;
}

int chk_wifi_sched(int band)
{
#if defined(QCA_SOFTDOWN_SUPPORT)
	int ret = 0;
	char vap[IFNAMSIZ] = "";

	if (nvram_get_int("wlready") == 0)
		return 0;

	strlcpy(vap, get_wififname(band)? : "", sizeof(vap));
	if (!iface_exist(vap))
		return 0;

	if (iwpriv_get_int(vap, "g_softdown", &ret) < 0)
		return 0;

	return !ret;
#else
	char tmp[20],tmp2[20];
        snprintf(tmp, sizeof(tmp), "wl%d_qca_sched", band);
        snprintf(tmp2, sizeof(tmp2), "wl%d_timesched", band);
	if(nvram_get_int(tmp2)==1 ) //wifi sched is enabled
	{
		if(nvram_get_int(tmp)==0) //sched is radio-off
		{
			//_dprintf("cfgmnt: radio[%d] should be left to wifi-sched\n",band);
			return 0;
		}
        }
	return 1;
#endif	/* QCA_SOFTDOWN_SUPPORT */
}

/* sec_channel_offset=1  if channel in plus_chlist_m.
 * sec_channel_offset=-1 if channel in minus_chlist_m.
 * center frequency is frequency of first_ch + (bandwidth / 2 - 10)
 */
struct freq2csparams_s {
	int first_ch;			/* first 20M channel of the bw channel. */
	uint64_t plus_chlist_m;		/* chlist mask of all ch. that ext. ch on plus direction */
	uint64_t minus_chlist_m;	/* chlist mask of all ch. that ext. ch on minus direction */
};

static const struct freq2csparams_s
g_b5g_40m_tbl[] = {
	{  36,  CH36_M,  CH40_M },
	{  44,  CH44_M,  CH48_M },
	{  52,  CH52_M,  CH56_M },
	{  60,  CH60_M,  CH64_M },
//	{  68,  CH68_M,  CH72_M },
//	{  76,  CH76_M,  CH80_M },
//	{  84,  CH84_M,  CH88_M },
//	{  92,  CH92_M,  CH96_M },
	{ 100, CH100_M, CH104_M },
	{ 108, CH108_M, CH112_M },
	{ 116, CH116_M, CH120_M },
	{ 124, CH124_M, CH128_M },
	{ 132, CH132_M, CH136_M },
	{ 140, CH140_M, CH144_M },
	{ 149, CH149_M, CH153_M },
	{ 157, CH157_M, CH161_M },
	{ 165, CH165_M, CH169_M },
	{ 173, CH173_M, CH177_M },
	{ 0, 0, 0 },
}, g_b5g_80m_tbl[] = {
	{  36,  CH36_M |  CH44_M,  CH40_M |  CH48_M },
	{  52,  CH52_M |  CH60_M,  CH56_M |  CH64_M },
//	{  68,  CH68_M |  CH76_M,  CH72_M |  CH80_M },
//	{  84,  CH84_M |  CH92_M,  CH88_M |  CH96_M },
	{ 100, CH100_M | CH108_M, CH104_M | CH112_M },
	{ 116, CH116_M | CH124_M, CH120_M | CH128_M },
	{ 132, CH132_M | CH140_M, CH136_M | CH144_M },
	{ 149, CH149_M | CH157_M, CH153_M | CH161_M },
	{ 165, CH165_M | CH173_M, CH169_M | CH177_M },
	{ 0, 0, 0 }
}, g_b5g_160m_tbl[] = {
	{  36,  CH36_M |  CH44_M |  CH52_M |  CH60_M,  CH40_M |  CH48_M |  CH56_M |  CH64_M },
//	{  68,  CH68_M |  CH76_M |  CH84_M |  CH92_M,  CH72_M |  CH80_M |  CH88_M |  CH96_M },
	{ 100, CH100_M | CH108_M | CH116_M | CH124_M, CH104_M | CH112_M | CH120_M | CH128_M },
	{ 149, CH149_M | CH157_M | CH165_M | CH173_M, CH153_M | CH161_M | CH169_M | CH177_M },
	{ 0, 0, 0 }
#if defined(RTCONFIG_BW240M)
}, g_b5g_240m_tbl[] = {
	{ 100, CH100_M | CH108_M | CH116_M | CH124_M | CH132_M | CH140_M,
	       CH104_M | CH112_M | CH120_M | CH128_M | CH136_M | CH144_M },
	{ 0, 0, 0 }
#endif
#if defined(RTCONFIG_HAS_6G)
}, g_b6g_40m_tbl[] = {
	{   1,    B6GCH1_M,    B6GCH5_M },
	{   9,    B6GCH9_M,   B6GCH13_M },
	{  17,   B6GCH17_M,   B6GCH21_M },
	{  25,   B6GCH25_M,   B6GCH29_M },
	{  33,   B6GCH33_M,   B6GCH37_M },
	{  41,   B6GCH41_M,   B6GCH45_M },
	{  49,   B6GCH49_M,   B6GCH53_M },
	{  57,   B6GCH57_M,   B6GCH61_M },
	{  65,   B6GCH65_M,   B6GCH69_M },
	{  73,   B6GCH73_M,   B6GCH77_M },
	{  81,   B6GCH81_M,   B6GCH85_M },
	{  89,   B6GCH89_M,   B6GCH93_M },
	{  97,   B6GCH97_M,  B6GCH101_M },
	{ 105,  B6GCH105_M,  B6GCH109_M },
	{ 113,  B6GCH113_M,  B6GCH117_M },
	{ 121,  B6GCH121_M,  B6GCH125_M },
	{ 129,  B6GCH129_M,  B6GCH133_M },
	{ 137,  B6GCH137_M,  B6GCH141_M },
	{ 145,  B6GCH145_M,  B6GCH149_M },
	{ 153,  B6GCH153_M,  B6GCH157_M },
	{ 161,  B6GCH161_M,  B6GCH165_M },
	{ 169,  B6GCH169_M,  B6GCH173_M },
	{ 177,  B6GCH177_M,  B6GCH181_M },
	{ 185,  B6GCH185_M,  B6GCH189_M },
	{ 193,  B6GCH193_M,  B6GCH197_M },
	{ 201,  B6GCH201_M,  B6GCH205_M },
	{ 209,  B6GCH209_M,  B6GCH213_M },
	{ 217,  B6GCH217_M,  B6GCH221_M },
	{ 225,  B6GCH225_M,  B6GCH229_M },
	{ 0, 0, 0 },
}, g_b6g_80m_tbl[] = {
	{   1,    B6GCH1_M |   B6GCH9_M,    B6GCH5_M |  B6GCH13_M },
	{  17,   B6GCH17_M |  B6GCH25_M,   B6GCH21_M |  B6GCH29_M },
	{  33,   B6GCH33_M |  B6GCH41_M,   B6GCH37_M |  B6GCH45_M },
	{  49,   B6GCH49_M |  B6GCH57_M,   B6GCH53_M |  B6GCH61_M },
	{  65,   B6GCH65_M |  B6GCH73_M,   B6GCH69_M |  B6GCH77_M },
	{  81,   B6GCH81_M |  B6GCH89_M,   B6GCH85_M |  B6GCH93_M },
	{  97,   B6GCH97_M | B6GCH105_M,  B6GCH101_M | B6GCH109_M },
	{ 113,  B6GCH113_M | B6GCH121_M,  B6GCH117_M | B6GCH125_M },
	{ 129,  B6GCH129_M | B6GCH137_M,  B6GCH133_M | B6GCH141_M },
	{ 145,  B6GCH145_M | B6GCH153_M,  B6GCH149_M | B6GCH157_M },
	{ 161,  B6GCH161_M | B6GCH169_M,  B6GCH165_M | B6GCH173_M },
	{ 177,  B6GCH177_M | B6GCH185_M,  B6GCH181_M | B6GCH189_M },
	{ 193,  B6GCH193_M | B6GCH201_M,  B6GCH197_M | B6GCH205_M },
	{ 209,  B6GCH209_M | B6GCH217_M,  B6GCH213_M | B6GCH221_M },
	{ 0, 0, 0 }
}, g_b6g_160m_tbl[] = {
	{   1,    B6GCH1_M |   B6GCH9_M |  B6GCH17_M |  B6GCH25_M,    B6GCH5_M |  B6GCH13_M |  B6GCH21_M |  B6GCH29_M },
	{  33,   B6GCH33_M |  B6GCH41_M |  B6GCH49_M |  B6GCH57_M,   B6GCH37_M |  B6GCH45_M |  B6GCH53_M |  B6GCH61_M },
	{  65,   B6GCH65_M |  B6GCH73_M |  B6GCH81_M |  B6GCH89_M,   B6GCH69_M |  B6GCH77_M |  B6GCH85_M |  B6GCH93_M },
	{  97,   B6GCH97_M | B6GCH105_M | B6GCH113_M | B6GCH121_M,  B6GCH101_M | B6GCH109_M | B6GCH117_M | B6GCH125_M },
	{ 129,  B6GCH129_M | B6GCH137_M | B6GCH145_M | B6GCH153_M,  B6GCH133_M | B6GCH141_M | B6GCH149_M | B6GCH157_M },
	{ 161,  B6GCH161_M | B6GCH169_M | B6GCH177_M | B6GCH185_M,  B6GCH165_M | B6GCH173_M | B6GCH181_M | B6GCH189_M },
	{ 193,  B6GCH193_M | B6GCH201_M | B6GCH209_M | B6GCH217_M,  B6GCH197_M | B6GCH205_M | B6GCH213_M | B6GCH221_M },
	{ 0, 0, 0 }
}, g_b6g_320m_tbl[] = {
	{   1,    B6GCH1_M |   B6GCH9_M |  B6GCH17_M |  B6GCH25_M |  B6GCH33_M |  B6GCH41_M |  B6GCH49_M |  B6GCH57_M,
		  B6GCH5_M |  B6GCH13_M |  B6GCH21_M |  B6GCH29_M |  B6GCH37_M |  B6GCH45_M |  B6GCH53_M |  B6GCH61_M },
	{  33,   B6GCH33_M |  B6GCH41_M |  B6GCH49_M |  B6GCH57_M |  B6GCH65_M |  B6GCH73_M |  B6GCH81_M |  B6GCH89_M,
		 B6GCH37_M |  B6GCH45_M |  B6GCH53_M |  B6GCH61_M |  B6GCH69_M |  B6GCH77_M |  B6GCH85_M |  B6GCH93_M },
	{  65,   B6GCH65_M |  B6GCH73_M |  B6GCH81_M |  B6GCH89_M |  B6GCH97_M | B6GCH105_M | B6GCH113_M | B6GCH121_M,
		 B6GCH69_M |  B6GCH77_M |  B6GCH85_M |  B6GCH93_M | B6GCH101_M | B6GCH109_M | B6GCH117_M | B6GCH125_M },
	{  97,   B6GCH97_M | B6GCH105_M | B6GCH113_M | B6GCH121_M | B6GCH129_M | B6GCH137_M | B6GCH145_M | B6GCH153_M,
		B6GCH101_M | B6GCH109_M | B6GCH117_M | B6GCH125_M | B6GCH133_M | B6GCH141_M | B6GCH149_M | B6GCH157_M },
	{ 129,  B6GCH129_M | B6GCH137_M | B6GCH145_M | B6GCH153_M | B6GCH161_M | B6GCH169_M | B6GCH177_M | B6GCH185_M,
		B6GCH133_M | B6GCH141_M | B6GCH149_M | B6GCH157_M | B6GCH165_M | B6GCH173_M | B6GCH181_M | B6GCH189_M },
	{ 161,  B6GCH161_M | B6GCH169_M | B6GCH177_M | B6GCH185_M | B6GCH193_M | B6GCH201_M | B6GCH209_M | B6GCH217_M,
		B6GCH165_M | B6GCH173_M | B6GCH181_M | B6GCH189_M | B6GCH197_M | B6GCH205_M | B6GCH213_M | B6GCH221_M },
	{ 0, 0, 0 }
#endif
};

struct bw_freq2csparams_s {
	int bw;
	const struct freq2csparams_s *freq2csparams_tbl;
};

static const struct bw_freq2csparams_s g_b5g_bw_freq2csparams_tbl[] = {
	{  40, g_b5g_40m_tbl },
	{  80, g_b5g_80m_tbl },
	{ 160, g_b5g_160m_tbl },
#if defined(RTCONFIG_BW240M)
	{ 240, g_b5g_240m_tbl },
	{ 320, g_b5g_240m_tbl },
#endif
	{ 0, NULL },
#if defined(RTCONFIG_HAS_6G)
}, g_b6g_bw_freq2csparams_tbl[] = {
	{  40, g_b6g_40m_tbl },
	{  80, g_b6g_80m_tbl },
	{ 160, g_b6g_160m_tbl },
	{ 320, g_b6g_320m_tbl },
#endif
	{ 0, NULL },
};

#if defined(RTCONFIG_HAS_6G)
/* Decide center channel of 320MHz channel by direction.
 * @nctrlsb = lower, choose center channel if @ch smaller than it.
 * @nctrlsb = upper, choose center channel if @ch greater than it.
 * @return:
 * 	0:	invalid parameter or center channel not available.
 *    > 0:	center channel.
 */
int get_6g_320m_cch_by_dir(int ch, uint64_t chlist_m, const char *nctrlsb)
{
	int ret = 0, dir = 0, cch;		/* 320M center channel number */
	uint64_t m, ch_m = 0;
	const struct freq2csparams_s *p;

	if (ch <= 0 || !chlist_m || !nctrlsb)
		return 0;

	ch_m = ch6g2bitmask(ch);
	if (!ch_m || !(ch_m & chlist_m))
		return 0;

	if (!strcmp(nctrlsb, "lower"))
		dir = -1;
	else if (!strcmp(nctrlsb, "upper"))
		dir = 1;

	for (p = g_b6g_320m_tbl; dir && p->first_ch; ++p) {
		m = p->plus_chlist_m | p->minus_chlist_m;
		if (!(ch_m & m) || (chlist_m & m) != m)
			continue;

		cch = p->first_ch + 4 * 7 + 2;
		if ((dir < 0 && ch < cch) || (dir > 0 && ch > cch)) {
			ret = cch;
			break;
		}
	}

	return ret;
}
#endif

/* Assume current mode, @orig_mode, is 40MHz, remove tail non-digital string
 * and append either "MINUS" or "PLUS" based on @nctrlsb.
 * @orig_mode:	same as return value of "iwpriv athX get_mode" in 40MHz mode.
 * @bw:		20/40/80/...
 * @nctrlsb:	direction of frequency of control channel if @bw is 40MHz.
 * 		cfg_client may change nctrlsb for RE, e.g.,
 *		CAP support ch1~13 and RE support ch1~11, when CAP is ch9@40+,
 *		it's not possible to set RE as same mode, in this case, nctrlsb
 *		is set as NCTRLSB_CTL_H_EXT_L by caller instead.
 * 	1:	the control sideband is upper (NCTRLSB_CTL_H_EXT_L = 11xxxMINUS)
 * 	0:	the control sideband is lower (NCTRLSB_CTL_L_EXT_H = 11xxxPLUS )
 * @return:	pointer to local buffer or NULL.
 */
static const char *genvapmode(const char *orig_mode, int bw, int nctrlsb)
{
	static char new_mode[32] = "";
	char *p;

	if (!orig_mode || *orig_mode == '\0' || bw < 0
	 || nctrlsb < 0 || nctrlsb >= NCTRLSB_MAX)
		return NULL;

	*new_mode = '\0';
	strlcpy(new_mode, orig_mode, sizeof(new_mode));
	for (p = new_mode + strlen("11"); *p != '\0'; ++p) {
		if (!isdigit(*p))
			continue;
		snprintf(p, (new_mode + sizeof(new_mode) - p), "%d", bw);
		break;
	}

	/* set extension channel when bw==40 and valid nctrlsb. */
	if (bw == 40 && nctrlsb == NCTRLSB_CTL_H_EXT_L)
		strlcat(new_mode, "MINUS", sizeof(new_mode));
	else if (bw == 40 && nctrlsb == NCTRLSB_CTL_L_EXT_H)
		strlcat(new_mode, "PLUS", sizeof(new_mode));

	return new_mode;
}

/* Test if @bw is valid bandwidth, e.g.
 * 2G: 20, 40
 * 5G: 20, 40, 80, 160 if RTCONFIG_BW160M, 240/320 if RTCONFIG_BW240M
 * 6G: 20, 40, 80, 160 if RTCONFIG_BW160M, 320 if 11be mode and RTCONFIG_BW320M
 */
int is_valid_wifi_bw(int band, int bw)
{
	static const int valid_2g_bw_tbl[] = { 20, 40, -1 }
		, valid_5g_bw_tbl[] = { 20, 40, 80
#if defined(RTCONFIG_BW160M)
			, 160
#endif
#if defined(RTCONFIG_BW240M)
			, 240
#endif
#if defined(RTCONFIG_BW320M)
			, 320	/* 320 + puncture = 240 */
#endif
			, -1 }
#if defined(RTCONFIG_HAS_6G)
		, valid_6g_bw_tbl[] = { 20, 40, 80
#if defined(RTCONFIG_BW160M)
			, 160
#endif
#if defined(RTCONFIG_BW320M)
			, 320
#endif
			, -1 }
#endif	/* RTCONFIG_HAS_6G */
		;
	const int *p = NULL;
	int ret = 0;

	if (band < 0 || band >= WL_NR_BANDS || __absent_band(band) || bw <= 0)
		return 0;
	if (band == WL_2G_BAND)
		p = valid_2g_bw_tbl;
	else if (is_5g(band))
		p = valid_5g_bw_tbl;
#if defined(RTCONFIG_HAS_6G)
	else if (is_6g(band))
		p = valid_6g_bw_tbl;
#endif
	else {
		_dprintf("%s: Unknown band %d\n", __func__, band);
		return 0;
	}

	for (; !ret && p && *p > 0; ++p) {
		if (bw == *p)
			ret = 1;
	}

	return ret;
}

struct nr_vap_softdown_s {
	int nr_vap;
	int nr_softdown;
};

/* Called by readdir_wrapper(), select best U-Boot binary provided by next ASUSWRT firmware we are about to write to DUT.
 * @basedir:
 * @de:
 * @arg:	pointer to struct uboot_req_s
 * @return:
 * 	0:	to keep readdir_wrapper working, always return 0
 */
static int calc_nr_vap_softdown(const char *basedir, const struct dirent *de, size_t de_size, void *arg)
{
	struct nr_vap_softdown_s *p = arg;
	int r, c = 0;

	if (sizeof(*de) != de_size) {
		/* If size of struct dirent mismatch, make sure readdir_wrapper()
		 * and this function see same struct dirent.h. e.g., it's different
		 * in uclibc if _FILE_OFFSET_BITS=64 is defined or not.
		 */
		_dprintf("%s: size of struct dirent mismatch (%zu v.s. %zu)!\n",
			__func__, sizeof(*de), de_size);
		return -1;
	}
	if (!arg)
		return 0;

	/* skip VLAN */
	if (strchr(de->d_name, '.'))
		return 0;

	p->nr_vap++;
	r = iwpriv_get_int(de->d_name, "g_softdown", &c);
	if (r || c)
		p->nr_softdown++;

	return 0;
}

/* Return name of @accel_type
 * @return:	Pointer to a string or empty string.
 */
const char *nat_accel_type_name(int accel_type)
{
	const char *accel_type_name = "";

	if (__nat_accel_type_name)
		accel_type_name = __nat_accel_type_name(accel_type);

	if (!accel_type_name)
		accel_type_name = "";
	return accel_type_name;
}

/* Return NAT acceleration type
 * @return:
 *  >= 0	Platform-specific acceleration type
 *    -1	API not defined, assume Auto.
 */
int nat_accel_type(const char *kmod_name)
{
	int accel_type = -1;

	if (__nat_accel_type)
		accel_type = __nat_accel_type(kmod_name);

	return accel_type;
}

/* Return current NAT acceleration type
 * @return:
 *  >= 0	Platform-specific acceleration type
 *    -1	API not defined, assume Auto.
 */
int cur_nat_accel_type(const char *kmod_name)
{
	int cur_accel_type = -1;

	if (__cur_nat_accel_type)
		cur_accel_type = __cur_nat_accel_type(kmod_name);

	return cur_accel_type;
}


/* Set channel/bandwidth immediately/later
 * Usage: wl_set_ch_bw [-d XX] [-f] -b X|ifname channel bw nctrlsb
 * -d XX:	change channel/bandwidth after XX seconds, XX = 0~60.
 * -b X:	band number, if it's valid wl_band_id and exist,
 *		@ifname parameter is ignored and can be anything string.
 * -f:		skip bw_cap checking.
 * @return:
 */
int wl_set_ch_bw_main(int argc, char *argv[])
{
	int c, delay = 0, channel = -1, bw = -1, nctrlsb = -1, save_nv = 0;
	int old_nctrlsb = -1, old_channel = 0, max_ch = 0, force = 0;
	int first_ch = 0, sec_channel_offset = 0, cfreq1 = 0, ret = 0;
	int b, v, unit = -1, subunit, bwcap, max_b, orig_bw, new_bw, wl_bw;
	const char *ifname = NULL, *r;
	char ch_list[3 * 2 + 22 * 3 + 34 * 4 + 4] = "", *d1, *d2;
	char orig_mode[32] = "", prefix[] = "wlXXXXXXXXXXXX_", vap[IFNAMSIZ];
	char cmd[sizeof("hostapd_cli -i XXX status") + IFNAMSIZ], state[16] = { 0 }; /* ref. state string in hostapd_state_text() */
	char mode_cmd[sizeof(IWPRIV " XXX mode ") + IFNAMSIZ + 32] = "", new_mode[32] = "";
	char h_chan_switch[sizeof("hostapd_cli -i XXXXXX chan_switch 5 FFFF sec_channel_offset=-1 center_freq1=XXXX bandwidth=320 vht") + IFNAMSIZ] = "";
	char pidfn[sizeof("/var/run/wl_set_ch_bw.B.pidXXX")], pidstr[10] = "", *ch_type = "";
	char main_prefix[sizeof("wlX_XXX")], nv_buf[7];
	pid_t pid;
	uint64_t chlist_m = 0, ch_m = 0, m;
	unsigned int new_freq = 0;
	const struct freq2csparams_s *q = NULL;
	const struct bw_freq2csparams_s *p = NULL;
	struct nr_vap_softdown_s nr_softdown;

	while((c = getopt(argc, argv, "b:d:fs")) != -1) {
		switch (c) {
		case 'b':
			b = safe_atoi(optarg);
			if (b < 0 || b >= WL_NR_BANDS || __absent_band(b))
				continue;
			unit = b;
			break;
		case 'd':
			delay = safe_atoi(optarg);
			if (delay > 60)
				delay = 60;
			else if (delay < 0)
				delay = 0;
			break;
		case 'f':
			force = 1;
			break;
		case 's':
			save_nv = 1;
			break;
		}
	}
	argc -= optind;
	argv += optind;
	v = 0;
	if (unit >= 0 && unit < WL_NR_BANDS && !__absent_band(unit)) {
		if (argc < 3)
			return -1;

		strlcpy(vap, get_wififname(unit), sizeof(vap));
		if (!iface_exist(vap)) {
			_dprintf("%s: vap %s doesn't exist!\n", __func__, vap);
			return -2;
		}
		/* If next parameter started with "ath" or "sta", assume it's
		 * @ifname parameter and skip it.
		 */
		if ((!strncmp(argv[v], "ath", 3) || !strncmp(argv[v], "sta", 3)))
			v++;
	} else {
		if (argc < 3)
			return -1;

		ifname = argv[v++];
		if (!iface_exist(ifname)) {
			_dprintf("%s: ifname %s doesn't exist!\n", __func__, ifname? : "NULL");
			return -2;
		}
		if (get_wlif_unit(ifname, &unit, &subunit) < 0)
			return -3;
		strlcpy(vap, ifname, sizeof(vap));
	}
	pid = 0;
	snprintf(pidfn, sizeof(pidfn), "/var/run/wl_set_ch_bw.pid");
	if (f_read_string(pidfn, pidstr, sizeof(pidstr)) > 0)
		pid = safe_atoi(pidstr);
	if (pid > 0 && process_exists(pid)) {
		return -4;
	}
	snprintf(pidstr, sizeof(pidstr), "%d", getpid());
	f_write_string(pidfn, pidstr, 0, 0);
	if (delay > 0)
		sleep(delay);

	if (!get_radio_status(vap)) {
		_dprintf("%s: ifname %s isn't up.\n", __func__, vap);
		ret = -5;
		goto exit_wl_set_ch_bw_main;
	}

#if defined(RTCONFIG_LYRA_5G_SWAP)
	snprintf(prefix, sizeof(prefix), "wl%d_", swap_5g_band(unit));
#else
	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
#endif

	if (nvram_pf_match(prefix, "radio", "0")) {
		_dprintf("%s: unit %d radio is off, skip.\n", __func__, unit);
		ret = -6;
		goto exit_wl_set_ch_bw_main;
	}
	snprintf(cmd, sizeof(cmd), "hostapd_cli -i %s status", vap);
	if (!exec_and_parse(cmd, "state=", "%*[^=]=%[^ \n]s", 1, state)
	 && !strcmp(state, "DISABLED")) {
		_dprintf("%s: ifname %s is disabled, skip.\n", __func__, vap);
		ret = -7;
		goto exit_wl_set_ch_bw_main;
	}

	channel = safe_atoi(argv[v++]);
	get_channel_list_via_driver(unit, ch_list, sizeof(ch_list));
	chlist_m = chlist2bitmask(unit, ch_list, ",");
	ch_m = ch2bitmask(unit, channel);
	if (!(ch_m & chlist_m)) {
		_dprintf("%s: unit %d channel %d is not supported! (chlist %s)\n",
			__func__, unit, channel, ch_list);
		ret = -8;
		goto exit_wl_set_ch_bw_main;
	}

	bw = safe_atoi(argv[v++]);
	if (!is_valid_wifi_bw(unit, bw)) {
		_dprintf("%s: Invalid bw %d (band %d)\n", __func__, bw, unit);
		ret = -9;
		goto exit_wl_set_ch_bw_main;
	}

	nctrlsb = safe_atoi(argv[v++]);
	if (unit == WL_2G_BAND && bw == 40
	 && (nctrlsb != NCTRLSB_CTL_H_EXT_L
	  && nctrlsb != NCTRLSB_CTL_L_EXT_H)) {
		ret = -10;
		goto exit_wl_set_ch_bw_main;
	}

	old_channel = get_channel(vap);
	if (channel <= 0) {
		if (old_channel > 0) {
			channel = old_channel;
			ch_m = ch2bitmask(unit, channel);
		} else {
			_dprintf("%s unit %d can't get current channel!\n",
				__func__, unit);
			ret = -11;
			goto exit_wl_set_ch_bw_main;
		}
	}

	if (unit == WL_2G_BAND && bw == 40) {
		for (b = 0, max_b = 0, m = chlist_m; m != 0; ++b, m >>= 1) {
			if (!(m & 1))
				continue;
			max_b = b;
		}
		max_ch = bit2ch(unit, max_b);
		if (((nctrlsb == NCTRLSB_CTL_H_EXT_L) && channel - 4 < 1)
		 || ((nctrlsb == NCTRLSB_CTL_L_EXT_H) && channel + 4 > max_ch)) {
			_dprintf("%s: 2G channel %d@40%sMHz is not supported. (chlist %s)\n",
				__func__, channel, (nctrlsb == NCTRLSB_CTL_H_EXT_L)? "-" : "+",
				ch_list);
			ret = -12;
			goto exit_wl_set_ch_bw_main;
		}
	}

	if (wl_get_bw_cap(unit, &bwcap) < 0) {
		dbg("%s: can't get bw_cap from band %d\n", __func__, unit);
		ret = -13;
		goto exit_wl_set_ch_bw_main;
	}

	strlcpy(orig_mode, iwpriv_get(vap, "get_mode")? : "", sizeof(orig_mode));
	if (get_bw_nctrlsb(vap, &orig_bw, &old_nctrlsb) <= 0) {
		ret = -14;
		goto exit_wl_set_ch_bw_main;
	}
#if defined(RTCONFIG_BW240M)
	if (orig_bw == 320 && is_5g(unit))
		orig_bw = 240;
	if (bw == 320 && is_5g(unit))
		bw = 240;
#endif
	new_bw = orig_bw;
	if (((bw != 0 && orig_bw != bw)
	  || (unit == WL_2G_BAND && bw == 40 && old_nctrlsb != nctrlsb))
	 && ((bw == 320 && (force || (bwcap & CFG_BW_320M)))
	  || (bw == 240 && (force || (bwcap & CFG_BW_240M)) && w75g_240m_capable(unit))
	  || (bw == 160 && (force || (bwcap & CFG_BW_160M)))
	  || (bw ==  80 && (force || (bwcap & CFG_BW_80M)))
	  || (bw ==  40 && (force || (bwcap & CFG_BW_40M)))
	  || (bw ==  20 && (force || (bwcap & CFG_BW_20M))))
	) {
		new_bw = bw;
	}
#if defined(RTCONFIG_BW240M)
	if (new_bw == 320 && is_5g(unit))
		new_bw = 240;
#endif

	cfreq1 = new_freq = ch2freq(channel, is_6g(unit)? 1 : 0);
	if (new_bw >= 40) {
		if (unit == WL_2G_BAND) {
			if (nctrlsb == NCTRLSB_CTL_L_EXT_H)
				sec_channel_offset = 1;
			else if (nctrlsb == NCTRLSB_CTL_H_EXT_L)
				sec_channel_offset = -1;
			first_ch = channel;
		} else if (is_5g(unit)) {
			p = g_b5g_bw_freq2csparams_tbl;
#if defined(RTCONFIG_HAS_6G)
		} else if (is_6g(unit)) {
			p = g_b6g_bw_freq2csparams_tbl;
#endif
		}

		for (; !first_ch && p != NULL && p->bw > 20 && p->freq2csparams_tbl; ++p) {
			if (p->bw != new_bw)
				continue;
			for (q = p->freq2csparams_tbl; q->first_ch; ++q) {
				m = q->plus_chlist_m | q->minus_chlist_m;
				if ((ch_m & m) && (m & chlist_m) != m) {
					/* Not all channels for the @bw bandwidth
					 * at @channel are supported.
					 */
					continue;
				}
				if ((ch_m & q->plus_chlist_m)) {
					sec_channel_offset = 1;
					nctrlsb = NCTRLSB_CTL_L_EXT_H;
				} else if ((ch_m & q->minus_chlist_m)) {
					sec_channel_offset = -1;
					nctrlsb = NCTRLSB_CTL_H_EXT_L;
				} else {
					continue;
				}
				first_ch = q->first_ch;
				cfreq1 = ch2freq(first_ch, is_6g(unit)? 1 : 0) + (new_bw / 2 - 10);
			}
		}
	}
	if (orig_bw == new_bw && old_channel == channel) {
		if (unit == WL_2G_BAND) {
			if (new_bw == 20 || (new_bw == 40 && old_nctrlsb == nctrlsb)) {
				ret = 0;
				goto exit_wl_set_ch_bw_main;
			}
		} else {
			ret = 0;
			goto exit_wl_set_ch_bw_main;
		}
	}

	if (new_bw >= 40
	 && (!sec_channel_offset
	  || (unit != WL_2G_BAND && cfreq1 == new_freq))) {
		/* Wrong sec_channel_offset or cfreq1. */
		_dprintf("%s: %d@%dMHz is not supported. (chlist %s, sec_channel_offset %d, cfreq1 %d)\n",
			__func__, channel, new_bw, ch_list, sec_channel_offset, cfreq1);
		ret = -15;
		goto exit_wl_set_ch_bw_main;
	}

	/* If all VAP on @unit are softdown, don't change channel/bandwidth. */
	memset(&nr_softdown, 0, sizeof(nr_softdown));
	readdir_wrapper(SYS_CLASS_NET, get_wififname(unit), calc_nr_vap_softdown, &nr_softdown);
	if (nr_softdown.nr_vap > 0 && nr_softdown.nr_vap == nr_softdown.nr_softdown) {
		_dprintf("%s: %d/%d softdown VAP on band %d. Don't change channel/bandwidth.\n",
			__func__, nr_softdown.nr_softdown, nr_softdown.nr_vap, unit);
		ret = 0;
		goto exit_wl_set_ch_bw_main;
	}

	/* Switch mode if necessary */
	if ((new_bw >= 240 && (is_5g(unit) || is_6g(unit))
	  && (!strncmp(orig_mode, "11AHE", 5) || !strncmp(orig_mode, "11ACVHT", 7)
	   || !strncmp(orig_mode, "11NA", 4)  || !strcmp(orig_mode, "11A")))
	 || (new_bw >= 80 && new_bw <= 160 && is_5g(unit)
	  && (!strncmp(orig_mode, "11NA", 4) || !strcmp(orig_mode, "11A")))) {
#if defined(RTCONFIG_QCA_BECHIP)
		const char *base_mode = "11AEHT80";
#elif defined(RTCONFIG_QCA_AXCHIP) || defined(RTCONFIG_WIFI_QCN5024_QCN5054)
		const char *base_mode = "11AHE80";
#else
		const char *base_mode = "11ACVHT80";
#endif
		/* Neither "iwpriv athX channel/doth_XXX" nor "hostapd_cli chan_switch"
		 * can switch to 11be bandwidth (320M) from non-11be mode to 11be mode or
		 * can switch to 80/160MHz from 11N/11A mode. Switch mode first.
		 */
		if ((r = genvapmode(base_mode, orig_bw, nctrlsb)) != NULL) {
			strlcpy(new_mode, r, sizeof(new_mode));
			snprintf(mode_cmd, sizeof(mode_cmd),
				IWPRIV " %s mode %s", vap, r);
		} else {
			_dprintf("%s: Generate mode for switching to 11be fail! (orig_mode %s new_bw %d)\n",
				__func__, orig_mode, new_bw);
		}
	} else if ((unit == WL_2G_BAND && new_bw == 40 && orig_bw == new_bw
		 && old_channel == channel && old_nctrlsb != nctrlsb)
		|| (unit != WL_2G_BAND && orig_bw != new_bw && old_channel == channel)
#if SPF_VER < SPF_VER_ID(11,1)
		|| 1
#endif
	) {
		/* doth_ch_chwidth can't switch 40MHz direction on same 2G channel.
		 * hostapd_cli chan_switch can't change bandwidth on same 2/5/6G channel either.
		 * Do it via "iwpriv athX mode NEW_MODE"
		 */
		if ((r = genvapmode(orig_mode, new_bw, nctrlsb)) != NULL) {
			strlcpy(new_mode, r, sizeof(new_mode));
			snprintf(mode_cmd, sizeof(mode_cmd),
				IWPRIV " %s mode %s", vap, r);
		} else {
			_dprintf("%s: Generate mode for changing bandwidth fail! (orig_mode %s new_bw %d)\n",
				__func__, orig_mode, new_bw);
		}
	}
	if (*mode_cmd != '\0') {
		_dprintf("%s: vap %s mode %s -> %s\n", __func__, vap, orig_mode, new_mode);
		logmessage("WIFI", "vap %s mode %s -> %s\n", vap, orig_mode, new_mode);
		doSystem(mode_cmd);
		sleep(1);

		strlcpy(orig_mode, iwpriv_get(vap, "get_mode")? : "", sizeof(orig_mode));
		if (get_bw_nctrlsb(vap, &orig_bw, &old_nctrlsb) <= 0) {
			ret = -16;
			goto exit_wl_set_ch_bw_main;
		}
#if defined(RTCONFIG_BW240M)
		if (orig_bw == 320 && is_5g(unit))
			orig_bw = 240;
#endif
		old_channel = get_channel(vap);
		if (orig_bw == new_bw && old_channel == channel) {
			ret = 0;
			goto exit_wl_set_ch_bw_main;
		}
	}


	/* Switch channel and/or bandwidth if necessary. */
	if (((new_bw == 240 || new_bw == 320) && is_5g(unit))) {
		int doth = 0;

		/* If doth is disabled, enable it. */
		if (iwpriv_get_int(vap, "get_doth", &doth) || !doth) {
			eval(IWPRIV, vap, "doth", "1");
		}
		/* Limitation of hostapd_cli chan_switch:
		 * 1. Can't switch to 240MHz channel
		 *    hostapd_cli -i ath1 chan_switch 5 5500 sec_channel_offset=1 center_freq1=5650 bandwidth=320 eht punct_bitmap=61440
		 *    will be blocked by kernel due to (extension) channel is disabled. Use doth_ch_chwidth instead.
		 * 2. Can't switch from 20MHz to 40MHz on same 2G channel if "eht" is specified. But "ht" works.
		 *    ieee80211_ucfg_set_chanswitch: Destination and current channels are the same. Exiting without error.
		 * 3. Can't switch 40MHz direction on same 2G channel, "iwpriv athX mode XXX" should be used in the case.
		 *
		 * doth_ch_chwidth:
		 * 2G/5G: SPF11.1+
		 * 6G:    SPF12+
		 */
		_dprintf("%s: vap %s mode %s%s%s, channel %d@%dMHz -> %d@%dMHz\n",
			__func__, vap, orig_mode, (*new_mode == '\0')? "" : " -> ", new_mode,
			old_channel, orig_bw, channel, new_bw);
		logmessage("WIFI", "vap %s mode %s%s%s, channel %d@%dMHz -> %d@%dMHz\n",
			vap, orig_mode, (*new_mode == '\0')? "" : " -> ", new_mode,
			old_channel, orig_bw, channel, new_bw);
		snprintf(h_chan_switch, sizeof(h_chan_switch),
			IWPRIV " %s doth_ch_chwidth %d 5 %d %s", vap, channel,
			(new_bw == 240)? 320 : new_bw, band2wlan_band_id(unit));
	} else {
		if (new_bw >= 40) {
			if (strstr(orig_mode, "11GEHT") || strstr(orig_mode, "11AEHT"))
				ch_type = "eht";
			else if (strstr(orig_mode, "11GHE") || strstr(orig_mode, "11AHE"))
				ch_type = "he";
			else if (strstr(orig_mode, "11ACVHT"))
				ch_type = "vht";
			else if (strstr(orig_mode, "11NGHT") || strstr(orig_mode, "11NAHT"))
				ch_type = "ht";
			else
				ch_type = "";

			/* If eht is specified, chan_switch can't change bandwidth at same 2G channel.
			 * But eht is required if chan_switch is about to change bandwidth on 5G.
			 */
			if (unit == WL_2G_BAND && new_bw == 40 && strcmp(ch_type, "ht"))
				ch_type = "ht";
		}

		d1 = (orig_bw != 40)? "" : (old_nctrlsb == NCTRLSB_CTL_H_EXT_L)? "-" : "+";
		d2 = (orig_bw != 40)? "" : (nctrlsb == NCTRLSB_CTL_H_EXT_L)? "-" : "+";
		_dprintf("%s: vap %s mode %s%s%s, channel %d@%d%sMHz -> %d@%d%sMHz, cfreq1 %d\n",
			__func__, vap, orig_mode, (*new_mode == '\0')? "" : " -> ", new_mode,
			old_channel, orig_bw, d1, channel,
			new_bw, d2, cfreq1);
		logmessage("WIFI", "vap %s mode %s%s%s, channel %d@%d%sMHz -> %d@%d%sMHz, cfreq1 %d\n",
			vap, orig_mode, (*new_mode == '\0')? "" : " -> ", new_mode,
			old_channel, orig_bw, d1, channel, new_bw, d2, cfreq1);

		/* Refer to hostapd_ctrl_check_freq_params().
		 * sec_channel_offset:	1 or -1 if bw >= 40, 1 means 11xxxPLUS, -1 means 11xxxMINUS.
		 * center_freq1:	center frequency of the @new_bw channel.
		 * 			optional param. if bw == 20; mandatory param. if bw >= 40.
		 * 			if bw == 20, cfreq1 = frequency of channel,
		 * 			if 5G/6G bw >= 40, cfreq1 = freq. of 1st ch. of the bandwidth ch. + (bandwidth / 2 - 10)
		 * bandwidth:		20,40,80,160,320
		 * center_freq2:	80+80 only and 80MHz channel are not adjacent to the other one.
		 * ht/vht:		"ht" if bw == 40, "vht" if bw >= 80
		 */
		if (unit == WL_2G_BAND) {
			snprintf(h_chan_switch, sizeof(h_chan_switch),
				"hostapd_cli -i %s chan_switch 5 %d sec_channel_offset=%d bandwidth=%d %s",
				vap, new_freq, sec_channel_offset, new_bw, ch_type);
		} else {
			snprintf(h_chan_switch, sizeof(h_chan_switch),
				"hostapd_cli -i %s chan_switch 5 %d sec_channel_offset=%d center_freq1=%d bandwidth=%d %s",
				vap, new_freq, sec_channel_offset, cfreq1, new_bw, ch_type);
		}
	}
	if (*h_chan_switch != '\0')
		doSystem(h_chan_switch);

	/* If 2G @channel supports 40MHz on both side, fix direction if it's wrong. */
	if (unit == WL_2G_BAND && new_bw == 40
	 && (channel - 4) > 0 && (channel + 4) <= max_ch) {
		sleep(1);
		strlcpy(orig_mode, iwpriv_get(vap, "get_mode")? : "", sizeof(orig_mode));
		if (get_bw_nctrlsb(vap, &orig_bw, &old_nctrlsb) <= 0) {
			ret = -17;
			goto exit_wl_set_ch_bw_main;
		}
		if (old_nctrlsb != nctrlsb
		 && (r = genvapmode(orig_mode, new_bw, nctrlsb)) != NULL) {
			snprintf(h_chan_switch, sizeof(h_chan_switch),
				IWPRIV " %s mode %s", vap, r);
			doSystem(h_chan_switch);
		}
	}

exit_wl_set_ch_bw_main:
	if (save_nv && !ret) {
		snprintf(main_prefix, sizeof(main_prefix), "wl%d_", unit);
		snprintf(nv_buf, sizeof(nv_buf), "%d", channel);
		if (!nvram_pf_match(main_prefix, "cap_channel", nv_buf))
			nvram_pf_set(main_prefix, "cap_channel", nv_buf);
		wl_bw = -1;
		if (new_bw == 320)
			wl_bw = WL_BW_320;
		else if (new_bw == 240)
			wl_bw = WL_BW_240;
		else if (new_bw == 160)
			wl_bw = WL_BW_160;
		else if (new_bw == 80)
			wl_bw = WL_BW_80;
		else if (new_bw == 40)
			wl_bw = WL_BW_40;
		else if (new_bw == 20)
			wl_bw = WL_BW_20;

		if (wl_bw >= 0) {
			snprintf(nv_buf, sizeof(nv_buf), "%d", wl_bw);
			if (!nvram_pf_match(main_prefix, "cap_bw", nv_buf))
				nvram_pf_set(main_prefix, "cap_bw", nv_buf);
		}

		if (new_bw == 40) {
			*nv_buf = '\0';
			if (nctrlsb == NCTRLSB_CTL_L_EXT_H)
				strlcpy(nv_buf, "lower", sizeof(nv_buf));
			else if (nctrlsb == NCTRLSB_CTL_H_EXT_L)
				strlcpy(nv_buf, "upper", sizeof(nv_buf));
			if (*nv_buf != '\0'
			 && !nvram_pf_match(main_prefix, "cap_nctrlsb", nv_buf))
				nvram_pf_set(main_prefix, "cap_nctrlsb", nv_buf);
		}
	}
	unlink(pidfn);

	return ret;
}

/* Set channel/bandwidth that is requested by cfg_client.
 * @ifname:
 * @channel:
 * @bw:		20/40/80/...
 * @nctrlsb:	direction of frequency of control channel if @bw is 40MHz.
 * 		cfg_client may change nctrlsb for RE, e.g.,
 *		CAP support ch1~13 and RE support ch1~11, when CAP is ch9@40+,
 *		it's not possible to set RE as same mode, in this case, nctrlsb
 *		is set as NCTRLSB_CTL_H_EXT_L by caller instead.
 * 	1:	the control sideband is upper (NCTRLSB_CTL_H_EXT_L = 11xxxMINUS)
 * 	0:	the control sideband is lower (NCTRLSB_CTL_L_EXT_H = 11xxxPLUS )
 * @return:
 */
int wl_set_ch_bw(const char *ifname, int channel, int bw, int nctrlsb)
{
	int delay = 0, ret = 0;
	char cmd[sizeof("wl_set_ch_bw -d XXX XXX CCC BBB N") + IFNAMSIZ];
	char ifname_argv[IFNAMSIZ], channel_argv[8], bw_argv[5], nctrlsb_argv[3];
	char *argv[] = { "wl_set_ch_bw", "-s", ifname_argv, channel_argv, bw_argv, nctrlsb_argv, NULL };

	if (ifname == NULL || (channel <= 0 && bw <= 0))
		return -1;

	if (delay > 0) {
		snprintf(cmd, sizeof(cmd), "wl_set_ch_bw -d %d %s %d %d %d &",
			delay, ifname, channel, bw, nctrlsb);
		doSystem(cmd);
	} else {
		strlcpy(ifname_argv, ifname, sizeof(ifname_argv));
		snprintf(channel_argv, sizeof(channel_argv), "%d", channel);
		snprintf(bw_argv, sizeof(bw_argv), "%d", bw);
		snprintf(nctrlsb_argv, sizeof(nctrlsb_argv), "%d", nctrlsb);
		ret = wl_set_ch_bw_main(5, argv);
	}

	return ret;
}

void sync_control_channel(int unit, int channel, int bw, int nctrlsb)
{
	char athfix[IFNAMSIZ];
	int ret __attribute__ ((unused));

	if (unit < 0 || unit >= MAX_NR_WL_IF || __absent_band(unit))
		return;

	__get_wlifname(swap_5g_band(unit), 0, athfix);
	int wl_set_ch_bw(const char *ifname, int channel, int bw, int nctrlsb);
	ret = wl_set_ch_bw(athfix, channel, bw, nctrlsb);
}

/*
 * get_control_channel(unit, *channel, *bw, *nctrlsb)
 *
 * *bw:
 * 	return the bandwitdh value, could be 20/40/80/160/240.
 *
 * nctrlsb:
 * 	return the side band when in HT40 mode
 * 	1: the control sideband is upper (NCTRLSB_CTL_H_EXT_L = 11xxxMINUS)
 * 	0: the control sideband is lower (NCTRLSB_CTL_L_EXT_H = 11xxxPLUS )
 * 	-1: invalid
 *
 */
void get_control_channel(int unit, int *channel, int *bw, int *nctrlsb)
{
	char athfix[8];
	int ret __attribute__ ((unused));

	if (unit < 0 || unit >= MAX_NR_WL_IF || __absent_band(unit))
		return;
	if (channel == NULL || bw == NULL || nctrlsb == NULL)
		return;

	__get_wlifname(swap_5g_band(unit), 0, athfix);
	*channel = get_channel(athfix);

	ret = get_bw_nctrlsb(athfix, bw, nctrlsb);
}

#if defined(RTCONFIG_MLO)
char *get_mld_mac_by_sta(char *ap_ifname, char *sta_mac, char *mld_mac, int mld_mac_len, int *mlo_active)
{
	FILE *fp;
        unsigned char sta[6],cand_mac[6],mld[6];
        char cmd[sizeof("wlanconfig XXX list") + IFNAMSIZ];
        char *p,*q, line_buf[300];
	int mac_match;
        *mlo_active=-1;

	if (sscanf(sta_mac,"%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX", &sta[0],&sta[1],&sta[2],&sta[3],&sta[4],&sta[5]) != 6)
                return NULL;

        snprintf(cmd, sizeof(cmd), "wlanconfig %s list", ap_ifname);
        if (!(fp = popen(cmd, "r")))
                return NULL;

	mac_match=0;
        /* Parsing client list */
        while (fgets(line_buf, sizeof(line_buf), fp) != NULL) {
		memset(cand_mac,0,sizeof(cand_mac));
		memset(mld,0,sizeof(mld));
                if (sscanf(line_buf, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX %*[^\n]", &cand_mac[0], &cand_mac[1], &cand_mac[2], &cand_mac[3], &cand_mac[4], &cand_mac[5]) != 6)
		{
		       if(mac_match==1) //parse capability
		       {
				if ((q=strstr(line_buf, "MLD Addr")) != NULL)
				{
					if((p=strstr(q,":"))!=NULL)
					{
						if (sscanf(p+1, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX %*[^\n]", &mld[0], &mld[1], &mld[2], &mld[3], &mld[4], &mld[5]) == 6)
							break; //mlo addr
					}
				}
		       }
		}
		else
		{
			if(mac_match) //target is non-mlo sta / target's mlo parse fail
				break;
			mac_match=0;
			if (!memcmp(sta, cand_mac, ETHER_ADDR_LEN))
				mac_match=1;
		}
        }

        pclose(fp);
	memset(mld_mac, 0, mld_mac_len);
	if(mac_match)
	{
		if(strlen(mld))
		{
			snprintf(mld_mac, mld_mac_len, "%02X:%02X:%02X:%02X:%02X:%02X",mld[0],mld[1],mld[2],mld[3],mld[4],mld[5]);
			*mlo_active=1;
		}
	}
	return mld_mac;
}

int is_mlo_if(char *vif)
{
	char info[512];
        char *pt;
	int i;
        unsigned char mld[6];
        if(get_qca_iw(vif,"info",info, sizeof(info)))
	{
		if ((pt=strstr(info, "mld_addr")) != NULL)
		{
			if (sscanf(pt + strlen("mld_addr") + 1, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX %*[^\n]",
				&mld[0], &mld[1], &mld[2], &mld[3], &mld[4], &mld[5]) == 6)
			{
				for(i=0;i<ETH_ALEN;i++)
				{
					if(mld[i]!=0x0)
					{
						//_dprintf("mld_addr: %02X:%02X:%02X:%02X:%02X:%02X\n",mld[0],mld[1],mld[2],mld[3],mld[4],mld[5]);
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

int is_mlo_map(char *vif)
{
	int res = 0;
	int mlo_connection_mode = isMloConnectionMode();
	if(!mlo_connection_mode)
		return res;
	if(!is_mlo_if(vif))
		return res;
	
	if(!strcmp(nvram_safe_get("mlo_map"),vif))
               res++;

	return res;
}

/**
 * @brief add guest vsie
 *
 * @param hexdata vsie string
 */
void add_beacon_vsie_dwb(char *hexdata)
{
	int dwb_band = nvram_get_int("dwb_band");
	int subunit = nvram_get_int("mlo_dwb_mssid_subunit");
	if(subunit == 0)
		return;
	add_beacon_vsie_by_unit(dwb_band, subunit, hexdata);
}

void del_beacon_vsie_dwb(char *hexdata)
{
	int dwb_band = nvram_get_int("dwb_band");
	int subunit = nvram_get_int("mlo_dwb_mssid_subunit");
	if(subunit == 0)
		return;
	del_beacon_vsie_by_unit(dwb_band, subunit, hexdata);
}

/**
 * @brief add FH vsie
 *
 * @param hexdata vsie string
 */
void add_beacon_vsie_FH(char *hexdata)
{
#ifdef RTCONFIG_BHCOST_OPT
        int unit = 0;
        char word[100], *next;
#endif
	int subunit=0;
#ifdef RTCONFIG_MLO
	char iotFhIfname[32] = {0};
#endif
#ifdef RTCONFIG_MULTILAN_MWL
	char FhIfname[32] = {0};
	char tmp[32] = {0}, wl_prefix[sizeof("wlXXXX_")];
#endif

#ifdef RTCONFIG_WIFI_SON
	if (nvram_match("wifison_ready", "1"))
		return;
#endif
#ifdef RTCONFIG_BHCOST_OPT
#ifdef RTCONFIG_MLO
	if(get_compatible_network(-1, iotFhIfname, sizeof(iotFhIfname)) != NULL)
	{
		foreach (word, iotFhIfname, next) {
			unit = subunit = -1;
			sscanf(word, "wl%d.%d", &unit, &subunit);
			if(subunit > 0) {
#ifdef RTCONFIG_MULTILAN_MWL
				snprintf(wl_prefix, sizeof(wl_prefix), "wl%d_", unit);
				if (nvram_get_int(strcat_r(wl_prefix, "nband", tmp)) == 4) {
					continue;
				}
#endif
				add_beacon_vsie_by_unit(unit, subunit, hexdata);
			}
		}
	}
#endif
#ifdef RTCONFIG_MULTILAN_MWL
	if(get_fh_if_prefix(FhIfname, sizeof(FhIfname)) != NULL)
	{
		foreach (word, FhIfname, next) {
			unit = subunit = -1;
			sscanf(word, "wl%d.%d", &unit, &subunit);
			if(subunit > 0) {
				snprintf(wl_prefix, sizeof(wl_prefix), "wl%d_", unit);
				if (nvram_get_int(strcat_r(wl_prefix, "nband", tmp)) == 4) {
					continue;
				}
				add_beacon_vsie_by_unit(unit, subunit, hexdata);
			}
		}
	}
#endif
#endif
}

void del_beacon_vsie_FH(char *hexdata)
{
#ifdef RTCONFIG_BHCOST_OPT
        int unit = 0;
        char word[100], *next;
	int subunit=0;
#ifdef RTCONFIG_MLO
	char iotFhIfname[32] = {0};
#endif
#ifdef RTCONFIG_MULTILAN_MWL
	char FhIfname[32] = {0};
#endif
#endif

#ifdef RTCONFIG_BHCOST_OPT
#ifdef RTCONFIG_MLO
	if(get_compatible_network(-1, iotFhIfname, sizeof(iotFhIfname)) != NULL)
	{
		foreach (word, iotFhIfname, next) {
			unit = subunit = -1;
			sscanf(word, "wl%d.%d", &unit, &subunit);
			if(subunit > 0)
				del_beacon_vsie_by_unit(unit, subunit, hexdata);
		}
	}
#endif
#ifdef RTCONFIG_MULTILAN_MWL
	if(get_fh_if_prefix(FhIfname, sizeof(FhIfname)) != NULL)
	{
		foreach (word, FhIfname, next) {
			unit = subunit = -1;
			sscanf(word, "wl%d.%d", &unit, &subunit);
			if(subunit > 0)
				del_beacon_vsie_by_unit(unit, subunit, hexdata);
		}
	}
#endif
#endif
}
#endif

#endif

#ifdef RTCONFIG_CFGSYNC

#define check_re_in_macfilter(...) (0)

#define TMP_UPDATE_MACFILTER "/tmp/update_macfilter.sh"
void update_macfilter_relist(void)
{
	char word[16], *next;
	char mac2g[32], mac5g[32], *next_mac;
	int unit = 0;
	char *nv, *nvp, *b;
	char *reMac, *maclist2g, *maclist5g, *timestamp;
	char stamac2g[18] = {0};
	char stamac5g[18] = {0};
	FILE *fp;
	char qca_mac[32];
	char *sec = "";
#ifdef RTCONFIG_HAS_6G
	char *maclist6g, *reserved1, *reserved2;
	char stamac6g[18] = {0};
	char mac6g[32];
#endif

	nvram_unset("relist_ready");
	while (nvram_get_int("wlready") != 1)
		sleep(1);

	unlink(TMP_UPDATE_MACFILTER);
	if ((fp = fopen(TMP_UPDATE_MACFILTER, "w")) == NULL) {
		perror(TMP_UPDATE_MACFILTER);
		return;
	}

	fprintf(fp, "#!/bin/sh\n");
	set_macfilter_all(fp);
	if (is_cfg_relist_exist()) {
#ifdef RTCONFIG_AMAS
		if (nvram_get_int("re_mode") == 1) {
			nv = nvp = get_cfg_relist(0);
			if (nv) {
				while ((b = strsep(&nvp, "<")) != NULL) {
					if ((vstrsep(b, ">", &reMac, &maclist2g, &maclist5g, &timestamp) != 4))
						continue;
					/* first mac for sta 2g of dut */
					foreach_44 (mac2g, maclist2g, next_mac)
						break;
					/* first mac for sta 5g of dut */
					foreach_44 (mac5g, maclist5g, next_mac)
						break;

					if (strcmp(reMac, get_2g_hwaddr()) == 0) {
						snprintf(stamac2g, sizeof(stamac2g), "%s", mac2g);
						dbg("dut 2g sta (%s)\n", stamac2g);
						snprintf(stamac5g, sizeof(stamac5g), "%s", mac5g);
						dbg("dut 5g sta (%s)\n", stamac5g);
						break;
					}
				}
				free(nv);
			}
#ifdef RTCONFIG_HAS_6G
			/* cfg_relist_x */
			nv = nvp = runtime_has_6g()? get_cfg_relist(1) : NULL;
			if (nv) {
				while ((b = strsep(&nvp, "<")) != NULL) {
					if ((vstrsep(b, ">", &reMac, &maclist6g, &reserved1, &reserved2) != 4))
						continue;
					/* first mac for sta 6g of dut */
					foreach_44 (mac6g, maclist6g, next_mac)
						break;

					if (strcmp(reMac, get_2g_hwaddr()) == 0) {
						snprintf(stamac6g, sizeof(stamac6g), "%s", mac6g);
						dbg("dut 6g sta (%s)\n", stamac6g);
						break;
					}
				}
				free(nv);
			}
#endif
		}
#endif

#ifdef RTCONFIG_WIFI_SON
		if (nvram_match("wifison_ready", "1"))
			sec = "_sec";
#endif
#ifdef RTCONFIG_QCA_LBD
		if (nvram_match("smart_connect_x", "1"))
			sec = "_sec";
#endif
		sprintf(qca_mac, "%s%s", QCA_ADDMAC, sec);

		foreach (word, nvram_safe_get("wl_ifnames"), next) {
			char tmp[32], prefix[16] = "wlXXXXXXXXXX_";
			char athfix[8];
			SKIP_ABSENT_BAND_AND_INC_UNIT(unit);

#ifdef RTCONFIG_AMAS
			if (nvram_get_int("re_mode") == 1)
				snprintf(prefix, sizeof(prefix), "wl%d.1_", unit);
			else
#endif
				snprintf(prefix, sizeof(prefix), "wl%d_", unit);

			__get_wlifname(swap_5g_band(unit), 0, athfix);

			if (nvram_match(strcat_r(prefix, "macmode", tmp), "allow")) {
				/* cfg_relist */
				nv = nvp = get_cfg_relist(0);
				if (nv) {
					while ((b = strsep(&nvp, "<")) != NULL) {
						if ((vstrsep(b, ">", &reMac, &maclist2g, &maclist5g, &timestamp) != 4))
							continue;

						if (strcmp(reMac, get_lan_hwaddr()) == 0)
							continue;

						if (unit == 0) {
							foreach_44 (mac2g, maclist2g, next_mac) {
								if (check_re_in_macfilter(unit, mac2g))
									continue;
								printf("relist sta (%s) in %s\n", mac2g, athfix);
								fprintf(fp, IWPRIV " %s %s %s\n", athfix, qca_mac, mac2g);
							}
						}
						else if (unit == 1) {
							foreach_44 (mac5g, maclist5g, next_mac) {
								if (check_re_in_macfilter(unit, mac5g))
									continue;
								printf("relist sta (%s) in %s\n", mac5g, athfix);
								fprintf(fp, IWPRIV " %s %s %s\n", athfix, qca_mac, mac5g);
							}
						}
					}
					free(nv);
				}

#ifdef RTCONFIG_HAS_6G
				/* cfg_relist_x */
				nv = nvp = runtime_has_6g()? get_cfg_relist(1) : NULL;
				if (nv) {
					while ((b = strsep(&nvp, "<")) != NULL) {
						if ((vstrsep(b, ">", &reMac, &maclist6g, &reserved1, &reserved2) != 4))
							continue;

						if (strcmp(reMac, get_lan_hwaddr()) == 0)
							continue;

						if (unit == 2) {
							foreach_44 (mac6g, maclist6g, next_mac) {
								if (check_re_in_macfilter(unit, mac6g))
									continue;
								printf("relist sta (%s) in %s\n", mac6g, athfix);
								fprintf(fp, IWPRIV " %s %s %s\n", athfix, qca_mac, mac6g);
							}
						}
					}
					free(nv);
				}
#endif
			}

			unit++;
		}
	}
	fflush(fp);
	fclose(fp);
	chmod(TMP_UPDATE_MACFILTER, 0777);
	eval(TMP_UPDATE_MACFILTER);
	nvram_set("relist_ready", "1");
}
#endif

#if defined(RTCONFIG_LYRA_5G_SWAP)
int swap_5g_band(int band)
{
#if defined(RTCONFIG_WIFI_SON)
	if (nvram_match("wifison_ready", "1"))
		return band;
#endif
#ifdef RTCONFIG_WIRELESSREPEATER
	if (sw_mode() == SW_MODE_REPEATER)
		return band;
#endif
	switch(band)
	{
		case 1:
			return 2;
			break;
		case 2:
			return 1;
			break;
		default:
			return band;
			break;
	}
	return 0;
}
#endif

#if defined(RTCONFIG_QCA_LBD)
static int make_lbd_conn(char *ip, int port)
{
	int sfd, flags;
	struct sockaddr_in rsock;
	fd_set wmask;
	struct timeval select_timeout;

	sfd = socket(AF_INET,SOCK_STREAM,0);
	if ( sfd == -1 )
		return -1;

	flags = fcntl(sfd, F_GETFL, 0);
	fcntl(sfd, F_SETFL, flags | O_NONBLOCK);

	memset ((char *)&rsock,0,sizeof(rsock));
	rsock.sin_addr.s_addr = inet_addr(ip);
	rsock.sin_family = AF_INET;
	rsock.sin_port = htons(port);
	if ( connect(sfd,(struct sockaddr *)(&rsock),sizeof(rsock)) == -1 ) {
		if ( errno != EINPROGRESS )
			return -2;
	}

	FD_ZERO(&wmask);
	FD_SET(sfd, &wmask);
	select_timeout.tv_sec = 1;
	select_timeout.tv_usec= 500000;

	if (select(sfd+1, NULL, &wmask, NULL, &select_timeout) <= 0) {
		close(sfd);
		return -3;
	}

	fcntl(sfd, F_SETFL, flags&~O_NONBLOCK);
	return sfd;
}

static char *get_lbd_data(int sfd, int *rlen, char *terminate)
{
	char *recv_buf;
	int block_len, buf_len, recv_len;
	int ok = 0;
	int terminate_len;
	fd_set rmask;
	struct timeval select_timeout;

	block_len = 500;
	buf_len = recv_len = 0;
	recv_buf = NULL;
	if (terminate)
		terminate_len = strlen(terminate);
	else
		terminate_len = 0;
	while (1) {
		int len;
		char *tmp;

		if (recv_len + block_len >= buf_len) {
			buf_len += block_len;
			tmp = realloc(recv_buf, buf_len + 1);
			if (!tmp)
				break;
			recv_buf = tmp;
		}

		FD_ZERO(&rmask);
		FD_SET(sfd, &rmask);
		select_timeout.tv_sec = 1;
		select_timeout.tv_usec= 0;
		if  (select(sfd+1, &rmask, NULL, NULL, &select_timeout) <= 0)
			break;

		len = read(sfd, recv_buf+recv_len, block_len);
		if (len < 0)
			break;
		recv_len += len;
		if ((terminate_len) && (recv_len >= terminate_len)) {
			if (memcmp(recv_buf+recv_len-terminate_len, terminate, terminate_len) == 0)
				ok = 1;
		}
		if (ok) {
			recv_buf[recv_len] = '\0';
			break;
		}
	};
	if (!ok) {
		if (recv_buf)
			free(recv_buf);
		return NULL;
	}
	if (rlen)
		*rlen = recv_len;
	return recv_buf;
}

/* nosteer */
char *set_steer(const char *mac,int val)
{
	int sock;
	char outbuf[100];
	char *got_buf=NULL;
	int len;

	sock = make_lbd_conn("127.0.0.1", 7787);
	if (sock < 0)
		return NULL;

	got_buf = get_lbd_data(sock, &len, "@ ");
	if (!got_buf)
		goto fail;
	free(got_buf);

	if (mac==NULL)
		goto fail;
	else
		len = sprintf(outbuf, "stadb nosteer %s %d\r\n", mac,val);
	write(sock, outbuf, len);
	got_buf = get_lbd_data(sock, &len, "@ ");
	if (!got_buf)
		goto fail;

	len = sprintf(outbuf, "q q\r\n");
	write(sock, outbuf, len);
fail:
	close(sock);
	return got_buf;
}
#endif

#ifdef RTCONFIG_AMAS
double get_wifi_maxpower(int band_type)
{
	return 0;
}
double get_wifi_5G_maxpower()
{
	return 0;
}
double get_wifi_5GH_maxpower()
{
	return 0;
}
double get_wifi_6G_maxpower()
{
	return 0;
}
#endif

// nmp
static void akm_suite_selector(char *tmp, char *wl_auth, int auth_len)
{
	if(!strcmp(tmp, "1")) {
		strlcpy(wl_auth, "802.1x", auth_len);
	} else if(!strcmp(tmp, "2")) {
		strlcpy(wl_auth, "WPA-PSK", auth_len);
	} else if(!strcmp(tmp, "3")) {
		strlcpy(wl_auth, "FT-802.1x", auth_len);
	} else if(!strcmp(tmp, "4")) {
		strlcpy(wl_auth, "WPA-PSK-FT", auth_len);
	} else if(!strcmp(tmp, "5")) {
		strlcpy(wl_auth, "802.1x-SHA256", auth_len);
	} else if(!strcmp(tmp, "6")) {
		strlcpy(wl_auth, "WPA-PSK-SHA256", auth_len);
	} else if(!strcmp(tmp, "7")) {
		strlcpy(wl_auth, "TDLS", auth_len);
	} else if(!strcmp(tmp, "8")) {
		strlcpy(wl_auth, "WPA3-SAE", auth_len);
	} else if(!strcmp(tmp, "9")) {
		strlcpy(wl_auth, "FT-SAE", auth_len);
	} else if(!strcmp(tmp, "10")) {
		strlcpy(wl_auth, "AP-PEER-KEY", auth_len);
	} else if(!strcmp(tmp, "11")) {
		strlcpy(wl_auth, "802.1x-suite-B", auth_len);
	} else if(!strcmp(tmp, "12")) {
		strlcpy(wl_auth, "802.1x-suite-B-192", auth_len);
	} else if(!strcmp(tmp, "13")) {
		strlcpy(wl_auth, "FT-802.1x-SHA384", auth_len);
	} else if(!strcmp(tmp, "14")) {
		strlcpy(wl_auth, "FILS-SHA256", auth_len);
	} else if(!strcmp(tmp, "15")) {
		strlcpy(wl_auth, "FILS-SHA384", auth_len);
	} else if(!strcmp(tmp, "16")) {
		strlcpy(wl_auth, "FT-FILS-SHA256", auth_len);
	} else if(!strcmp(tmp, "17")) {
		strlcpy(wl_auth, "FT-FILS-SHA384", auth_len);
	} else if(!strcmp(tmp, "18")) {
		strlcpy(wl_auth, "OWE", auth_len);
	} else if(!strcmp(tmp, "19")) {
		strlcpy(wl_auth, "FT-WPA2-PSK-SHA384", auth_len);
	} else if(!strcmp(tmp, "20")) {
		strlcpy(wl_auth, "WPA2-PSK-SHA384", auth_len);
	} else if(!strcmp(tmp, "21")) {
		strlcpy(wl_auth, "PASN", auth_len);
	} else if(!strcmp(tmp, "22")) {
		strlcpy(wl_auth, "FT-802-1X-SHA384", auth_len);
	} else if(!strcmp(tmp, "23")) {
		strlcpy(wl_auth, "802_1X_SHA384", auth_len);
	} else if(!strcmp(tmp, "24")) {
		strlcpy(wl_auth, "SAE-EXT-KEY", auth_len);
	} else if(!strcmp(tmp, "25")) {
		strlcpy(wl_auth, "FT-SAE-EXT-KEY", auth_len);
	} else {
		strlcpy(wl_auth, "undefined", auth_len);
	}
}

#ifdef RTCONFIG_MULTILAN_CFG
void check_wireless_auth_from_sdn(char *mac, char *ifname, char *wl_auth, int auth_len)
{
	FILE *fp;
	char cmd[128] = {0}, tmp[8] = {0};

	snprintf(cmd, sizeof(cmd), "hostapd_cli -i %s sta %s | grep AKMSuiteSelector 2>/dev/null", ifname, mac);
	if ((fp = popen(cmd, "r")) != NULL) {
		if (fscanf(fp, "AKMSuiteSelector=00-0f-ac-%s", tmp) == 1) {
			akm_suite_selector(tmp, wl_auth, auth_len);
		}
		pclose(fp);
	}
}
#else
void check_wireless_auth(char *mac, char *wl_auth, int auth_len)
{
	FILE *fp;
	char word[256], *next;
	char cmd[128] = {0}, tmp[8] = {0};
	int ret = 0;

	foreach (word, nvram_safe_get("wl_ifnames"), next)
	{
		snprintf(cmd, sizeof(cmd), "hostapd_cli -i %s sta %s | grep AKMSuiteSelector 2>/dev/null", word, mac);
		if ((fp = popen(cmd, "r")) != NULL) {
			if (fscanf(fp, "AKMSuiteSelector=00-0f-ac-%s", tmp) == 1) {
				akm_suite_selector(tmp, wl_auth, auth_len);
			}
			pclose(fp);
			if(ret)
				break;
		}
	}
}
#endif

#if defined(RTCONFIG_MULTILAN_CFG)
char* get_all_lan_ifnames(void)
{
	int band;
        char vif[sizeof("wlXXXXX_vifs")],result[200];

	memset(result,0,sizeof(result));
	snprintf(result, sizeof(result), "%s ", nvram_safe_get("wl_ifnames"));

        for (band = 0; band < MAX_NR_WL_IF; ++band) {
                SKIP_ABSENT_BAND(band);
                snprintf(vif, sizeof(vif), "wl%d_vifs", band);
		strlcat(result,nvram_safe_get(vif),sizeof(result));
		strlcat(result," ",sizeof(result));
	}

	result[strlen(result)-1] = '\0';
	return strdup(result);
}
#endif
