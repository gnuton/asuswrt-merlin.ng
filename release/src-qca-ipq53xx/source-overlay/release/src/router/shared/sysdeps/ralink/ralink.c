#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shutils.h"
#include "shared.h"
#include <ralink.h>

#if defined(RTCONFIG_AMAS_ETHDETECT)
#if defined(RTCONFIG_MT798X)
extern int iface_name_to_vport(const char *iface);
extern int get_mt7986_mt7531_vport_info(unsigned int vport, unsigned int *link, unsigned int *speed, phy_info *info);
#elif defined(RTCONFIG_MT799X)
extern int iface_name_to_vport(const char *iface);
extern int get_mt7988_vport_info(unsigned int vport, unsigned int *link, unsigned int *speed, phy_info *info);
#endif

#if defined(PRTAX57_GO) || defined(GSBE7200X)
#define PORT_UNITS 2
#else
#define PORT_UNITS 1	/* query only WAN port */
#endif

/* Aimesh RE: vport to eth name */
static const char *query_ifname[PORT_UNITS] = {
#if defined(PRTAX57_GO)
	/* LAN   WAN */
	"eth0", "eth1"
#elif defined(TUFAX4200) || defined(TUFAX6000) || defined(RTAX59U) \
   || defined(BT8) || defined(BT8P) || defined(BT6) \
   || defined(GS7) || defined(EBA76) || defined(GS7_MAGIC)
	/* WAN */
	"eth1"
#elif defined(RTBE59)
	/* WAN   LAN4 */
	"eth1", "eth2"
#elif defined(GSBE7200X)
	/* WAN   LAN5 */
	"eth2", "eth1"
#else
#error define query_ifname
#endif
};
#endif /* RTCONFIG_AMAS_ETHDETECT */

struct channel_info {
	unsigned char channel;
	unsigned char bandwidth;
	unsigned char extrach;
};

int wl_get_bw(int unit)
{
	struct iwreq wrq;
	struct channel_info info;

	memset(&info, 0, sizeof(struct channel_info));
	wrq.u.data.length = sizeof(struct channel_info);
	wrq.u.data.pointer = (caddr_t) &info;
	wrq.u.data.flags = ASUS_SUBCMD_GCHANNELINFO;

	if (wl_ioctl(get_staifname(unit), RTPRIV_IOCTL_ASUSCMD, &wrq) < 0) {
		dbg("wl_ioctl failed on %s (%d)\n", __FUNCTION__, __LINE__);
		return -1;
	}

	switch (info.bandwidth) {
		case 0:
			return 20;
			break;
		case 1:
			return 40;
			break;
		case 2:
			return 80;
			break;
		case 3:
			return 160;
			break;
#if defined(RTCONFIG_BW320M)
		case 7:
			return 320;
			break;
#endif
		default:
			break;
	}

	return 20;
}

#ifdef RTCONFIG_AMAS
char *get_pap_bssid(int unit, char *bssid_buf, int buf_len)
{
	struct iwreq wrq;
	unsigned char ether_zero[ETH_ALEN] = { 0x00 };
#if defined(RTCONFIG_MLO) && defined(RTCONFIG_NL80211)
	struct apcli_mlo_info ami;
#endif

	memset(bssid_buf, 0, buf_len);
#if defined(RTCONFIG_MLO) && defined(RTCONFIG_NL80211)
	get_apcli_mlo_info(APCLI_2G, &ami);
	if (ami.link_status && (ami.link_path & (1U << unit))) {
		ether_etoa(&ami.link_addr[unit], bssid_buf);
		return bssid_buf;
	}
	else
#endif
	if (wl_ioctl(get_staifname(unit), SIOCGIWAP, &wrq) < 0) {
		dbg("%s: unable to obtain pap bssid\n", __func__);
		return bssid_buf;
	}
	else if (memcmp(&wrq.u.ap_addr.sa_data[0], &ether_zero[0], ETH_ALEN)) {
		snprintf(bssid_buf, buf_len, "%02X:%02X:%02X:%02X:%02X:%02X",
			(unsigned char)wrq.u.ap_addr.sa_data[0],
			(unsigned char)wrq.u.ap_addr.sa_data[1],
			(unsigned char)wrq.u.ap_addr.sa_data[2],
			(unsigned char)wrq.u.ap_addr.sa_data[3],
			(unsigned char)wrq.u.ap_addr.sa_data[4],
			(unsigned char)wrq.u.ap_addr.sa_data[5]);
	}
	//dbg("%s: unit(%d), bssid(%s)\n", __func__, unit, bssid_buf);

	return bssid_buf;
}

/*
 * wl_get_bw_cap(unit, *bwcap)
 *
 * bwcap
 * 	0x01 = 20 MHz
 * 	0x02 = 40 MHz
 * 	0x04 = 80 MHz
 * 	0x08 = 160 MHz
 *
 * 	ex: 5G support 20,40,80
 * 	*bwcap = 0x01 | 0x02 | 0x04
 */
int wl_get_bw_cap(int unit, int *bwcap)
{
	if (bwcap == NULL)
		return -1;
	if (unit == 0)
		*bwcap = 0x01 | 0x02;		/* 40MHz */
	else if (unit == 1) {
		*bwcap = 0x01 | 0x02 | 0x04;	/* 80MHz */
#if defined(RTCONFIG_BW160M)
		*bwcap |= 0x08;
#endif
	}
	else if (unit == 2) {
		*bwcap = 0x01 | 0x02 | 0x04;	/* 80MHz */
#if defined(RTCONFIG_BW160M)
		*bwcap |= 0x08;
#endif
#if defined(RTCONFIG_BW320M)
		*bwcap |= 0x10;
#endif
	}
	else
		return -1;

	return 0;
}

int wl_set_ch_bw(const char *ifname, int channel, int bw, int nctrlsb)
{
#if defined(RTCONFIG_NL80211)
	int band = get_wifi_unit(ifname);

	if ((band == WL_5G_BAND
#if defined(RTCONFIG_HAS_5G_2)
	  || band == WL_5G_2_BAND
#endif
	    ) && get_dfs_cac_status(ifname)) {
		//dbg("%s: skip... because band(%d) is during CAC\n", __func__, band);
		return -1;
	}
	doSystem("mwctl phy phy%d set channel num=%d bw=%d", band, channel, bw);
#else /* !RTCONFIG_NL80211 */
	if (set_bw_nctrlsb(ifname, bw, nctrlsb) < 0) {
		dbg("set %s bw (%d) or nctrlsb (%d) failed", ifname, bw, nctrlsb);
		return -1;
	}

	if (set_channel(ifname, channel) < 0) {
		dbg("set %s channel (%d) failed", ifname, channel);
		return -1;
	}
#endif /* RTCONFIG_NL80211 */

	return 0;
}

void sync_control_channel(int unit, int channel, int bw, int nctrlsb)
{
	char ifname[IFNAMSIZ];
	int ret __attribute__ ((unused));

	if (unit < 0 || unit >= MAX_NR_WL_IF)
		return;

	__get_wlifname(unit, 0, ifname);
	ret = wl_set_ch_bw(ifname, channel, bw, nctrlsb);
}

void get_control_channel(int unit, int *channel, int *bw, int *nctrlsb)
{
	char ifname[IFNAMSIZ];
	int ret __attribute__ ((unused));

	if (unit < 0 || unit >= MAX_NR_WL_IF)
		return;
	if (channel == NULL || bw == NULL || nctrlsb == NULL)
		return;

	__get_wlifname(unit, 0, ifname);

	ret = get_channel_info(ifname, channel, bw, nctrlsb);
}

int get_psta_status(int unit)
{
#if defined(RTCONFIG_NL80211)
	/* reference from QCA platform */
	int ret;
	const char *sta;

	sta = get_staifname(unit);
	ret = chk_assoc(sta);
	if (ret < 0) return WLC_STATE_STOPPED;
	if (ret > 0) return WLC_STATE_CONNECTED;
	return ret;
#else /* !RTCONFIG_NL80211 */
	const char *ifname;
	char data[32];
	struct iwreq wrq;
	int status;

	ifname = get_staifname(unit);

	memset(data, 0x00, sizeof(data));
	wrq.u.data.length = sizeof(data);
	wrq.u.data.pointer = (caddr_t) data;
	wrq.u.data.flags = ASUS_SUBCMD_CONN_STATUS;

	if (wl_ioctl(ifname, RTPRIV_IOCTL_ASUSCMD, &wrq) < 0) {
		dbg("errors in getting %s CONN_STATUS result\n", ifname);
		return -1;
	}

	status = *(int*)wrq.u.data.pointer;

	if (status == 6)        // APCLI_CTRL_CONNECTED
		return WLC_STATE_CONNECTED;
	else if (status == 4)   // APCLI_CTRL_ASSOC
		return WLC_STATE_CONNECTING;

	return WLC_STATE_INITIALIZING;
#endif /* RTCONFIG_NL80211 */
}

enum {
    VSIE_BEACON = 0x1,
    VSIE_PROBE_REQ = 0x2,
    VSIE_PROBE_RESP = 0x4,
    VSIE_ASSOC_REQ = 0x8,
    VSIE_ASSOC_RESP = 0x10,
    VSIE_AUTH_REQ = 0x20,
    VSIE_AUTH_RESP = 0x40
};

void vsie_operation(int unit, int subunit, int flag, int opt, char *hexdata)
{
	struct iwreq wrq;
	char cmd_data[512], ifname[16], nv[16];
	int len = 0;

	if (!subunit)
		snprintf(nv, sizeof(nv), "wl%d_ifname", unit);
	else
		snprintf(nv, sizeof(nv), "wl%d.%d_ifname", unit, subunit);

	if (flag == VSIE_PROBE_REQ)
		strlcpy(ifname, get_staifname(unit), sizeof(ifname));
	else
		strlcpy(ifname, nvram_safe_get(nv), sizeof(ifname));

	len = 3 + strlen(hexdata)/2;    /* 3 is oui's len */
	snprintf(cmd_data, sizeof(cmd_data), "vie_op=%d-frm_map:%d-oui:%02X%02X%02X-length:%d-ctnt:%s",
		opt, flag, (uint8_t)OUI_ASUS[0], (uint8_t)OUI_ASUS[1], (uint8_t)OUI_ASUS[2], len, hexdata);

#if defined(RTCONFIG_NL80211)
	eval(IWPRIV, ifname, "set", cmd_data);
#else
	wrq.u.data.length = strlen(cmd_data) + 1;
	wrq.u.data.pointer = cmd_data;
	wrq.u.data.flags = 0;

        if (wl_ioctl(ifname, RTPRIV_IOCTL_SET, &wrq) < 0)
                dbg("wl_ioctl failed on %s (%d)\n", __FUNCTION__, __LINE__);
#endif

    return;
}

/**
 * @brief add beacon vise by unit and subunit
 *
 * @param unit band index
 * @param subunit mssid index
 * @param hexdata vise string
 */
void add_beacon_vsie_by_unit(int unit, int subunit, char *hexdata)
{
    vsie_operation(unit, subunit, VSIE_BEACON | VSIE_PROBE_RESP, 1, hexdata);
}

/**
 * @brief add guest vsie
 *
 * @param hexdata vsie string
 */
void add_beacon_vsie_guest(char *hexdata)
{
	int unit = 0, subunit = 0;
	char word[32], *next;

	foreach (word, nvram_safe_get("wl_ifnames"), next) {
		if (nvram_get_int("re_mode") == 1)  // RE
			subunit = 3;
		else  // CAP/Router
			subunit = 2;
#if defined(RTCONFIG_HAS_6G)
		/* 6G does not have IoT VAP */
		if (unit == WL_6G_BAND)
			subunit -= 1;
#endif

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
				vsie_operation(unit, subunit, VSIE_BEACON | VSIE_PROBE_RESP, 1, hexdata);
		}
		unit++;
	}
}

void add_beacon_vsie(char *hexdata)
{
#ifdef RTCONFIG_BHCOST_OPT
    int unit = 0;
    char word[100], *next;

    foreach (word, nvram_safe_get("wl_ifnames"), next) {
    	vsie_operation(unit, 0, VSIE_BEACON | VSIE_PROBE_RESP, 1, hexdata);
        unit++;
    }
#else
    vsie_operation(0, 0, VSIE_BEACON | VSIE_PROBE_RESP, 1, hexdata);
#endif
}

#if defined(RTCONFIG_ROUTERBOOST) && defined(RTCONFIG_SOC_MT7981)
void vsie_operation_with_rb_oui(char *OUI_STR, int unit, int subunit, int flag, int opt, char *hexdata)
{
	struct iwreq wrq;
	char cmd_data[512], ifname[16];

	int len = 0;

	len = 3 + strlen(hexdata)/2;    /* 3 is oui's len */
	__get_wlifname(unit, subunit, ifname);

	snprintf(cmd_data, sizeof(cmd_data), "vie_op=%d-frm_map:%d-oui:%s-length:%d-ctnt:%s",
		opt, flag, OUI_STR/*ROUTERBOOST_OUI_STR*/, len, hexdata);

	wrq.u.data.length = strlen(cmd_data) + 1;
	wrq.u.data.pointer = cmd_data;
	wrq.u.data.flags = 0;

        if (wl_ioctl(ifname, RTPRIV_IOCTL_SET, &wrq) < 0)
                dbg("wl_ioctl failed on %s (%d)\n", __FUNCTION__, __LINE__);
	
    return;
}
void add_or_del_vendor_elements(int op, char *cap_OUI, char *hexdata)
{
#ifdef RTCONFIG_BHCOST_OPT
	int unit = 0;
	char word[100], *next;

	foreach (word, nvram_safe_get("wl_ifnames"), next) {
		vsie_operation_with_rb_oui(cap_OUI, unit, 0, VSIE_BEACON | VSIE_PROBE_RESP, op, hexdata);
		unit++;
	}
#endif
}
void add_or_del_assocresp_elements(int op, char *cap_OUI, char *bss_OUI, char *capaility, char *aplist)
{
#ifdef RTCONFIG_BHCOST_OPT
	int unit = 0;
	char word[100], *next;

	foreach (word, nvram_safe_get("wl_ifnames"), next) {
		vsie_operation_with_rb_oui(cap_OUI, unit, 0, VSIE_ASSOC_RESP, op, capaility);
		vsie_operation_with_rb_oui(bss_OUI, unit, 0, VSIE_ASSOC_RESP, op, aplist);
		unit++;
	}
#endif
}

void get_rb_ap_list(char *out_val)
{
#ifdef RTCONFIG_BHCOST_OPT
	int i = 0, max = 0;
	char word[100], *next, wl_hwaddr[20], hex_mac[13] ={0}, *mac_str = NULL;
	unsigned char mac[20];

	foreach (word, nvram_safe_get("wl_ifnames"), next) {
		max++;
	}

	if(max < 2)
		return;

	sprintf(out_val, "C80201%02X", max);
	for(i=0; i< max; i++) {
		sprintf(wl_hwaddr, "wl%d_hwaddr", i);
		mac_str = nvram_safe_get(wl_hwaddr);
		if(mac_str[0] == '\0' || !ether_atoe(mac_str, mac)) {
			return;
		}
		sprintf(hex_mac, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		strcat(out_val, hex_mac);
	}

#endif
}

void routerboost_vsie_operation(int op, char* v1_OUI, char *v2_OUI)
{
/* add "vendor_elements" and "assocresp_elements" like in hostapd.conf vsie
   for beacon/prob res and assoc_res/reassoc_res respectively
*/
	char capability[] = "C801010300";
	char cap_OUI[10] = {0};
	char bss_OUI[10] = {0};
	char aplist[50] = {0};
	
	if(v1_OUI == NULL)
		strcpy(cap_OUI, ROUTERBOOST_OUI_STR);
	else 
		strcpy(cap_OUI, v1_OUI);


	if(v2_OUI == NULL)
		strcpy(bss_OUI, ROUTERBOOST_OUI_STR);
	else 
		strcpy(bss_OUI, v2_OUI);
	
	get_rb_ap_list(aplist);
	add_or_del_vendor_elements(op, cap_OUI, capability);
	
	if(strlen(aplist) > 0) {
		add_or_del_assocresp_elements(op, cap_OUI, bss_OUI, capability, aplist);
	}
}

void add_routerboost_vsie(char* v1_OUI, char *v2_OUI)
{
	int op = 1; // add
	routerboost_vsie_operation(op, v1_OUI, v2_OUI);
}

void del_routerboost_vsie(char* v1_OUI, char *v2_OUI)
{
	int op = 3; // del
	routerboost_vsie_operation(op, v1_OUI, v2_OUI);	
}
#endif /* RTCONFIG_ROUTERBOOST & RTCONFIG_SOC_MT7981 */

/**
 * @brief remove beacon vsie by unit and subunit
 *
 * @param unit band index
 * @param subunit mssid index
 * @param hexdata vsie string
 */
void del_beacon_vsie_by_unit(int unit, int subunit, char *hexdata)
{
	vsie_operation(unit, subunit, VSIE_BEACON | VSIE_PROBE_RESP, 3, hexdata);
}

/**
 * @brief remove guest beacon vsie
 *
 * @param hexdata vsie string
 */
void del_beacon_vsie_guest(char *hexdata)
{
	int unit = 0, subunit = 0;
	char word[32], *next;

	foreach (word, nvram_safe_get("wl_ifnames"), next) {
		if (nvram_get_int("re_mode") == 1)  // RE
			subunit = 3;
		else  // CAP/Router
			subunit = 2;
#if defined(RTCONFIG_HAS_6G)
		/* 6G does not have IoT VAP */
		if (unit == WL_6G_BAND)
			subunit -= 1;
#endif

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
				vsie_operation(unit, subunit, VSIE_BEACON | VSIE_PROBE_RESP, 3, hexdata);
		}
		unit++;
	}
}

void del_beacon_vsie(char *hexdata)
{
#ifdef RTCONFIG_BHCOST_OPT
    int unit = 0;
    char word[100], *next;

    foreach (word, nvram_safe_get("wl_ifnames"), next) {
    	vsie_operation(unit, 0, VSIE_BEACON | VSIE_PROBE_RESP, 3, hexdata);
        unit++;
    }
#else
    vsie_operation(0, 0, VSIE_BEACON | VSIE_PROBE_RESP, 3, hexdata);
#endif
}

void add_probe_req_vsie(char *hexdata)
{
    vsie_operation(0, 0, VSIE_PROBE_REQ, 1, hexdata);
}

void del_probe_req_vsie(char *hexdata)
{
    vsie_operation(0, 0, VSIE_PROBE_REQ, 3, hexdata);
}

void wait_connection_finished(int band)
{
    int wait_time = 0;
    int conn_stat = 0;
    int wlc_conn_time = nvram_get_int("wlc_conn_time") ?: 10;

    while (wait_time++ < wlc_conn_time) {
        conn_stat = get_psta_status(band);
        if (conn_stat == WLC_STATE_CONNECTED) break;
        sleep(1);
    }
}

int get_wlan_service_status(int bssidx, int vifidx)
{
    if (nvram_get_int("wlready") == 0) return -1;

    char *ifname = NULL;
    char tmp[128] = {0}, prefix[] = "wlXXXXXXXXXX_";
    char wl_radio[] = "wlXXXX_radio";

    snprintf(wl_radio, sizeof(wl_radio), "wl%d_radio", bssidx);
    if (nvram_get_int(wl_radio) == 0)
        return -2;

    if (vifidx > 0)
        snprintf(prefix, sizeof(prefix), "wl%d.%d_", bssidx, vifidx);
    else
        snprintf(prefix, sizeof(prefix), "wl%d_", bssidx);

    ifname = nvram_safe_get(strcat_r(prefix, "ifname", tmp));

#if defined(RTCONFIG_NL80211)
	return get_no_bcn(ifname) ? 0 : 1;
#else
    if (is_intf_up(ifname) > 0) return get_radio(bssidx, vifidx);

    return 0;
#endif
}

#ifdef RTCONFIG_BHCOST_OPT
#ifdef RTCONFIG_AMAS_ETHDETECT
unsigned int get_uplinkports_linkrate(char *ifname)
{
        unsigned int link_rate = 0;
        int connect=0 ,speed=0;
        int vport = 0;

        for (vport = 0; vport < PORT_UNITS; vport++) 
	{
                if (vport >= ARRAY_SIZE(query_ifname)) {
                        dbg("%s: don't know vport %d\n", __func__, vport);
                        return 0;
                }
                if (query_ifname[vport] != NULL && strstr(query_ifname[vport],ifname)) 
		{
#if defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
#if defined(RTCONFIG_MT799X)
			get_mt7988_vport_info(iface_name_to_vport(ifname), &connect, &speed, NULL);
#else
			get_mt7986_mt7531_vport_info(iface_name_to_vport(ifname), &connect, &speed, NULL);
#endif
			if(connect)
			{	
				link_rate=speed;
				break;
			}
#else
#error port status and linkrate
#endif			
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
        int connect=0 ,speed=0;
        for (vport = 0; vport < PORT_UNITS; vport++) {
                if (vport >= ARRAY_SIZE(query_ifname)) {
                        dbg("%s: don't know vport %d\n", __func__, vport);
                        return 0;
                }
                if (query_ifname[vport] != NULL && strstr(query_ifname[vport],ifname)) 
		{
#if defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
#if defined(RTCONFIG_MT799X)
			get_mt7988_vport_info(iface_name_to_vport(ifname), &connect, &speed, NULL);
#else
			get_mt7986_mt7531_vport_info(iface_name_to_vport(ifname), &connect, &speed, NULL);
#endif
			if(connect)
				return 1;
#else
#error port status and linkrate
#endif			
                }
        }
        return 0;
}
#else /* !RTCONFIG_AMAS_ETHDETECT */
unsigned int get_uplinkports_linkrate(char *ifname)
{
	int speed;
	char *eth=NULL;
	speed=0;
	eth=nvram_safe_get("eth_ifnames");
	if(eth && strstr(eth,ifname))
		speed = rtkswitch_WanPort_phySpeed();
	return speed;
}

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
#endif  /* RTCONFIG_AMAS_ETHDETECT */
#endif	/* RTCONFIG_BHCOST_OPT */
#endif  /* RTCONFIG_AMAS */

void set_wlan_service_status(int bssidx, int vifidx, int enabled)
{
    if (nvram_get_int("wlready") == 0) return;

    char *ifname = NULL;
    char tmp[128] = {0}, prefix[] = "wlXXXXXXXXXX_";
    char wl_radio[] = "wlXXXX_radio";

    snprintf(wl_radio, sizeof(wl_radio), "wl%d_radio", bssidx);
    if (nvram_get_int(wl_radio) == 0)
        return;

    if (vifidx > 0)
        snprintf(prefix, sizeof(prefix), "wl%d.%d_", bssidx, vifidx);
    else
        snprintf(prefix, sizeof(prefix), "wl%d_", bssidx);

    ifname = nvram_safe_get(strcat_r(prefix, "ifname", tmp));
#if defined(RTCONFIG_NL80211)
	set_no_bcn(ifname, enabled ? 0 : 1);
#else
    if (!enabled && bssidx)
        doSystem("iwpriv %s set DfsCacClean=1", ifname);
    eval("ifconfig", ifname, enabled? "up":"down");
#endif
}

#if defined(RTCONFIG_MLO)
// if sta_mac match any mld link address in MTK bss_mngr, mld_mac is mld address
// else, mld_mac is 00:00:00:00:00:00
char *get_mld_mac_by_sta(char *ap_ifname, char *sta_mac, char *mld_mac, int mld_mac_len, int *mlo_active)
{
// mimic from mtk bss_mngr.h in sdk
#define BMGR_MAX_MLD_STA_CNT    256
#define BSS_MNGR_MAX_BAND_NUM     3
#define MAC_ADDR_LEN 6
	struct iwreq wrq;
	// temp_mac[6] is used not only as an input for the MAC address but also as a place to return the MLD address.
	// temp_mac[6] will change after performing wl_ioctl
	unsigned char temp_mac[MAC_ADDR_LEN];
	// sta_mac is stored in string format, transform it into bytes format
	unsigned char mac_bytes[MAC_ADDR_LEN];
	int values[MAC_ADDR_LEN];

	*mlo_active = -1;
	if (ap_ifname == NULL || mld_mac == NULL || mld_mac_len < 18)
		return NULL;

	if (sscanf(sta_mac, "%02X:%02X:%02X:%02X:%02X:%02X", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) == 6)
	{
		for (int i = 0; i < MAC_ADDR_LEN; i++)
			mac_bytes[i] = (unsigned char)values[i];
   	}
	else
		dbg("%s: Invalid MAC address format\n", __func__);
	memset(temp_mac, 0x00, sizeof(temp_mac));
	memcpy(temp_mac, mac_bytes, MAC_ADDR_LEN);
	wrq.u.data.length = sizeof(temp_mac);
	wrq.u.data.pointer = (caddr_t)temp_mac;
	wrq.u.data.flags = ASUS_SUBCMD_GET_MLD_ADDR_BY_STA;
	if (wl_ioctl(ap_ifname, RTPRIV_IOCTL_ASUSCMD, &wrq) < 0) {
		dbg("ap_ifname(%s) ASUS_SUBCMD_GET_MLD_ADDR failure\n", __FUNCTION__, ap_ifname);
		return NULL;
	}

	snprintf(mld_mac, mld_mac_len, "%02X:%02X:%02X:%02X:%02X:%02X", temp_mac[0], temp_mac[1], temp_mac[2], temp_mac[3], temp_mac[4], temp_mac[5]);
	if (!strcmp(mld_mac, "00:00:00:00:00:00"))
		/* MLO device, but single link */
		*mlo_active = 0;
	else if (!strcmp(mld_mac, "FF:FF:FF:FF:FF:FF"))
		/* non-MLO device */
		return NULL;
	else
		*mlo_active = 1;

	return mld_mac;
}

int is_mlo_if(char *vif)
{
#if defined(RTCONFIG_NL80211)
	int band;
	struct apcli_mlo_info ami;

	if (!vif)
		return 0;

	band = get_sta_ifname_unit(vif);
	get_apcli_mlo_info(APCLI_2G, &ami);
	if (ami.valid_path
	 && band >= 0
	 && (ami.valid_path & (1U << band)))
		return 1;
#endif /* RTCONFIG_NL80211 */

	return 0;
}

int is_mlo_map(char *vif)
{
	int amm_band = nvram_get_int("amm_band");
	int mlo = isMloConnectionMode();

	if (!is_mlo_if(vif))
		return 0;
	else if (mlo && amm_band == -1 && (!strcmp(vif, APCLI_2G)
#if defined(RTCONFIG_HAS_6G)
					|| !strcmp(vif, APCLI_6G)
#elif defined(RTCONFIG_HAS_5G_2) && defined(RTCONFIG_WIFI7_NO_6G)
					|| !strcmp(vif, APCLI_5G2)
#endif
					  ))
		return 1;
	else if (mlo && !strcmp(vif, get_staifname(amm_band)))
		return 1;
	else
		return 0;
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
	int subunit=0;
#ifdef RTCONFIG_MLO
	char iotFhIfname[32] = {0};
#endif
#ifdef RTCONFIG_MULTILAN_MWL
	char FhIfname[32] = {0};
	char tmp[32] = {0}, wl_prefix[sizeof("wlXXXX_")];
#endif
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

				vsie_operation(unit, subunit, VSIE_BEACON | VSIE_PROBE_RESP, 1, hexdata);
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
				vsie_operation(unit, subunit, VSIE_BEACON | VSIE_PROBE_RESP, 1, hexdata);
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

#ifdef RTCONFIG_MLO
	if(get_compatible_network(-1, iotFhIfname, sizeof(iotFhIfname)) != NULL)
	{
		foreach (word, iotFhIfname, next) {
			unit = subunit = -1;
			sscanf(word, "wl%d.%d", &unit, &subunit);
			if(subunit > 0)
                vsie_operation(unit, subunit, VSIE_BEACON | VSIE_PROBE_RESP, 3, hexdata);
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
                vsie_operation(unit, subunit, VSIE_BEACON | VSIE_PROBE_RESP, 3, hexdata);
		}
	}
#endif
#else
    vsie_operation(0, 0, VSIE_BEACON | VSIE_PROBE_RESP, 3, hexdata);
#endif
}
#endif

#ifdef RTCONFIG_CFGSYNC
void update_macfilter_relist(void)
{
	char tmp[128], prefix[] = "wlXXXXXXXXXX_";
	char word[256], *next;
	char mac2g[32], mac5g[32], *next_mac;
	int unit = 0;
	char *wlif_name = NULL;
	char *nv, *nvp, *b;
	char *reMac, *maclist2g, *maclist5g, *timestamp;
	char stamac2g[18] = {0};
	char stamac5g[18] = {0};
#ifdef RTCONFIG_HAS_6G
	char *maclist6g, *reserved1, *reserved2;
	char stamac6g[18] = {0};
	char mac6g[32];
#endif

	if (is_cfg_relist_exist())
	{
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

					if (strcmp(reMac, get_lan_hwaddr()) == 0) {
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
			nv = nvp = get_cfg_relist(1);
			if (nv) {
				while ((b = strsep(&nvp, "<")) != NULL) {
					if ((vstrsep(b, ">", &reMac, &maclist6g, &reserved1, &reserved2) != 4))
						continue;
					/* first mac for sta 6g of dut */
					foreach_44 (mac6g, maclist6g, next_mac)
						break;

					if (strcmp(reMac, get_lan_hwaddr()) == 0) {
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

		foreach (word, nvram_safe_get("wl_ifnames"), next) {
			SKIP_ABSENT_BAND_AND_INC_UNIT(unit);

#ifdef RTCONFIG_AMAS
			if (nvram_get_int("re_mode") == 1)
				snprintf(prefix, sizeof(prefix), "wl%d.1_", unit);
			else
#endif
				snprintf(prefix, sizeof(prefix), "wl%d_", unit);

			wlif_name = nvram_safe_get(strcat_r(prefix, "ifname", tmp));

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
								dbg("relist sta (%s) in %s\n", mac2g, wlif_name);
								set_acl_entry(wlif_name, mac2g);
							}
						}
						else if(unit == 1)
						{
							foreach_44 (mac5g, maclist5g, next_mac) {
								if (check_re_in_macfilter(unit, mac5g))
									continue;
								dbg("relist sta (%s) in %s\n", mac5g, wlif_name);
								set_acl_entry(wlif_name, mac5g);
							}
						}
					}
					free(nv);
				}
#ifdef RTCONFIG_HAS_6G
				/* cfg_relist_x */
				nv = nvp = get_cfg_relist(1);
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
								dbg("relist sta (%s) in %s\n", mac6g, wlif_name);
								set_acl_entry(wlif_name, mac6g);
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
}
#endif

#ifdef RTCONFIG_NEW_PHYMAP
extern int get_trunk_port_mapping(int trunk_port_value)
{
	return trunk_port_value;
}

#if defined(RTCONFIG_MT798X)
void mt798x_get_phy_port_mapping(phy_port_mapping *port_mapping);
#endif
#if defined(RTCONFIG_MT799X)
void mt799x_get_phy_port_mapping(phy_port_mapping *port_mapping);
#endif

/* phy port related start */
void get_phy_port_mapping(phy_port_mapping *port_mapping)
{
#if !defined(RTCONFIG_MT798X) && !defined(RTCONFIG_MT799X)
	static phy_port_mapping port_mapping_static = {
#if defined(RT4GAX56)
		.count = 6,
		.is_mobile_router = 1,
		.port[0] = { .phy_port_id = -1, .ext_port_id = 0, .label_name = "W0", .cap = PHY_PORT_CAP_WAN, .max_rate = 1000, .ifname = "eth1", .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[1] = { .phy_port_id = -1, .ext_port_id = 1, .label_name = "L1", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = "eth0", .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[2] = { .phy_port_id = -1, .ext_port_id = 2, .label_name = "L2", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = "eth0", .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[3] = { .phy_port_id = -1, .ext_port_id = 3, .label_name = "L3", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = "eth0", .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[4] = { .phy_port_id = -1, .ext_port_id = 4, .label_name = "L4", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = "eth0", .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[5] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "M1", .cap = PHY_PORT_CAP_MOBILE, .max_rate = 480, .ifname = "usb0", .flag = 0, .seq_no = -1, .ui_display = NULL }
#elif defined(RT4GAC86U)
		.count = 7,
		.is_mobile_router = 1,
		.port[0] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "W0", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[1] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L1", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[2] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L2", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[3] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L3", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[4] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L4", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[5] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "U1", .cap = PHY_PORT_CAP_USB, .max_rate = 480, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[6] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "M1", .cap = PHY_PORT_CAP_MOBILE, .max_rate = 480, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL }
#elif defined(RTAX54)
		.count = 5,
		.port[0] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "W0", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[1] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L1", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[2] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L2", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[3] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L3", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[4] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L4", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
#elif defined(RTAX53U)
		.count = 5,
		.port[0] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "W0", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[1] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L1", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[2] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L2", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[3] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L3", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[4] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "U1", .cap = PHY_PORT_CAP_USB, .max_rate = 480, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
#elif defined(RTACRH18)
		.count = 6,
		.port[0] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "W0", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[1] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L1", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[2] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L2", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[3] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L3", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[4] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L4", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[5] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "U1", .cap = PHY_PORT_CAP_USB, .max_rate = 5000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
#elif defined(XD4S)
		.count = 2,
		.port[0] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "W0", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
		.port[1] = { .phy_port_id = -1, .ext_port_id = -1, .label_name = "L1", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = NULL, .flag = 0, .seq_no = -1, .ui_display = NULL },
#else
		#error "port_mapping is not defined."
#endif
	};

	if (!port_mapping)
		return;

	memcpy(port_mapping, &port_mapping_static, sizeof(phy_port_mapping));

	add_sw_cap(port_mapping);
	swap_wanlan(port_mapping);
	return;
#else // RTCONFIG_MT798X
#if defined(RTCONFIG_MT799X)
	mt799x_get_phy_port_mapping(port_mapping);
#else
	mt798x_get_phy_port_mapping(port_mapping);
#endif
	return;
#endif
}
#endif

#if !defined(RTCONFIG_WLMODULE_MT7915D_AP) && !defined(RTCONFIG_MT798X) && !defined(RTCONFIG_MT799X)
int MCSMappingRateTable[] =
	{2,  4,   11,  22, // CCK
	12, 18,   24,  36, 48, 72, 96, 108, // OFDM
	13, 26,   39,  52,  78, 104, 117, 130, 26,  52,  78, 104, 156, 208, 234, 260, // 20MHz, 800ns GI, MCS: 0 ~ 15
	39, 78,  117, 156, 234, 312, 351, 390,										  // 20MHz, 800ns GI, MCS: 16 ~ 23
	27, 54,   81, 108, 162, 216, 243, 270, 54, 108, 162, 216, 324, 432, 486, 540, // 40MHz, 800ns GI, MCS: 0 ~ 15
	81, 162, 243, 324, 486, 648, 729, 810,										  // 40MHz, 800ns GI, MCS: 16 ~ 23
	14, 29,   43,  57,  87, 115, 130, 144, 29, 59,   87, 115, 173, 230, 260, 288, // 20MHz, 400ns GI, MCS: 0 ~ 15
	43, 87,  130, 173, 260, 317, 390, 433,										  // 20MHz, 400ns GI, MCS: 16 ~ 23
	30, 60,   90, 120, 180, 240, 270, 300, 60, 120, 180, 240, 360, 480, 540, 600, // 40MHz, 400ns GI, MCS: 0 ~ 15
	90, 180, 270, 360, 540, 720, 810, 900,
	13, 26,   39,  52,  78, 104, 117, 130, 156, /* 11ac: 20Mhz, 800ns GI, MCS: 0~8 */
	27, 54,   81, 108, 162, 216, 243, 270, 324, 360, /*11ac: 40Mhz, 800ns GI, MCS: 0~9 */
	59, 117, 176, 234, 351, 468, 527, 585, 702, 780, /*11ac: 80Mhz, 800ns GI, MCS: 0~9 */
	14, 29,   43,  57,  87, 115, 130, 144, 173, /* 11ac: 20Mhz, 400ns GI, MCS: 0~8 */
	30, 60,   90, 120, 180, 240, 270, 300, 360, 400, /*11ac: 40Mhz, 400ns GI, MCS: 0~9 */
	65, 130, 195, 260, 390, 520, 585, 650, 780, 867 /*11ac: 80Mhz, 400ns GI, MCS: 0~9 */
	};
#endif

#if defined(RTCONFIG_WLMODULE_MT7663E_AP) || defined(RTCONFIG_WLMODULE_MT7629_AP) || defined(RTCONFIG_WLMODULE_MT7622_AP)
int MCSMappingRateTable_5G[] = {
	2,  4, 11, 22, 12,  18,  24,  36, 48,  72,  96, 108, 109, 110, 111, 112,/* CCK and OFDM */
	13, 26, 39, 52, 78, 104, 117, 130, 26,  52,  78, 104, 156, 208, 234, 260,
	39, 78, 117, 156, 234, 312, 351, 390, /* BW 20, 800ns GI, MCS 0~23 */
	27, 54, 81, 108, 162, 216, 243, 270, 54, 108, 162, 216, 324, 432, 486, 540,
	81, 162, 243, 324, 486, 648, 729, 810, /* BW 40, 800ns GI, MCS 0~23 */
	14, 29, 43, 57, 87, 115, 130, 144, 29, 59,   87, 115, 173, 230, 260, 288,
	43, 87, 130, 173, 260, 317, 390, 433, /* BW 20, 400ns GI, MCS 0~23 */
	30, 60, 90, 120, 180, 240, 270, 300, 60, 120, 180, 240, 360, 480, 540, 600,
	90, 180, 270, 360, 540, 720, 810, 900, /* BW 40, 400ns GI, MCS 0~23 */

	/*for 11ac:20 Mhz 800ns GI*/
	6,  13, 19, 26,  39,  52,  58,  65,  78,  0,     /*1ss mcs 0~8*/
	13, 26, 39, 52,  78,  104, 117, 130, 156, 0,     /*2ss mcs 0~8*/
	19, 39, 58, 78,  117, 156, 175, 195, 234, 260,   /*3ss mcs 0~9*/
	26, 52, 78, 104, 156, 208, 234, 260, 312, 0,     /*4ss mcs 0~8*/

	/*for 11ac:40 Mhz 800ns GI*/
	13,	27,	40,	54,	 81,  108, 121, 135, 162, 180,   /*1ss mcs 0~9*/
	27,	54,	81,	108, 162, 216, 243, 270, 324, 360,   /*2ss mcs 0~9*/
	40,	81,	121, 162, 243, 324, 364, 405, 486, 540,  /*3ss mcs 0~9*/
	54,	108, 162, 216, 324, 432, 486, 540, 648, 720, /*4ss mcs 0~9*/

	/*for 11ac:80 Mhz 800ns GI*/
	29,	58,	87,	117, 175, 234, 263, 292, 351, 390,   /*1ss mcs 0~9*/
	58,	117, 175, 243, 351, 468, 526, 585, 702, 780, /*2ss mcs 0~9*/
	87,	175, 263, 351, 526, 702, 0,	877, 1053, 1170, /*3ss mcs 0~9*/
	117, 234, 351, 468, 702, 936, 1053, 1170, 1404, 1560, /*4ss mcs 0~9*/

	/*for 11ac:160 Mhz 800ns GI*/
	58,	117, 175, 234, 351, 468, 526, 585, 702, 780, /*1ss mcs 0~9*/
	117, 234, 351, 468, 702, 936, 1053, 1170, 1404, 1560, /*2ss mcs 0~9*/
	175, 351, 526, 702, 1053, 1404, 1579, 1755, 2160, 0, /*3ss mcs 0~8*/
	234, 468, 702, 936, 1404, 1872, 2106, 2340, 2808, 3120, /*4ss mcs 0~9*/

	/*for 11ac:20 Mhz 400ns GI*/
	7,	14,	21,	28,  43,  57,   65,	 72,  86,  0,    /*1ss mcs 0~8*/
	14,	28,	43,	57,	 86,  115,  130, 144, 173, 0,    /*2ss mcs 0~8*/
	21,	43,	65,	86,	 130, 173,  195, 216, 260, 288,  /*3ss mcs 0~9*/
	28,	57,	86,	115, 173, 231,  260, 288, 346, 0,    /*4ss mcs 0~8*/

	/*for 11ac:40 Mhz 400ns GI*/
	15,	30,	45,	60,	 90,  120,  135, 150, 180, 200,  /*1ss mcs 0~9*/
	30,	60,	90,	120, 180, 240,  270, 300, 360, 400,  /*2ss mcs 0~9*/
	45,	90,	135, 180, 270, 360,  405, 450, 540, 600, /*3ss mcs 0~9*/
	60,	120, 180, 240, 360, 480,  540, 600, 720, 800, /*4ss mcs 0~9*/

	/*for 11ac:80 Mhz 400ns GI*/
	32,	65,	97,	130, 195, 260,  292, 325, 390, 433,  /*1ss mcs 0~9*/
	65,	130, 195, 260, 390, 520,  585, 650, 780, 866, /*2ss mcs 0~9*/
	97,	195, 292, 390, 585, 780,  0,	 975, 1170, 1300, /*3ss mcs 0~9*/
	130, 260, 390, 520, 780, 1040,	1170, 1300, 1560, 1733, /*4ss mcs 0~9*/

	/*for 11ac:160 Mhz 400ns GI*/
	65,	130, 195, 260, 390, 520,  585, 650, 780, 866, /*1ss mcs 0~9*/
	130, 260, 390, 520, 780, 1040,	1170, 1300, 1560, 1733, /*2ss mcs 0~9*/
	195, 390, 585, 780, 1170, 1560,	1755, 1950, 2340, 0, /*3ss mcs 0~8*/
	260, 520, 780, 1040, 1560, 2080,	2340, 2600, 3120, 3466, /*4ss mcs 0~9*/

	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37
}; /* 3*3 */

#define FN_GETRATE(_fn_, _st_, _if, _mcstbl)						\
_fn_(_st_ HTSetting)							\
{									\
	unsigned char Antenna = 0;	\
	unsigned char MCS = HTSetting.field.MCS;	\
	int rate_count = sizeof(_mcstbl)/sizeof(int);	\
	int rate_index = 0;						\
	int value = 0;	\
									\
	if (HTSetting.field.MODE >= MODE_VHT)				\
	{								\
		if(_if == 1) {	\
			MCS = HTSetting.field.MCS & 0xf;	\
			Antenna = (HTSetting.field.MCS >> 4) + 1;	\
														\
			if (HTSetting.field.BW == BW_20) {	\
				rate_index = 112 + ((Antenna - 1) * 10) + ((unsigned char)HTSetting.field.ShortGI * 160) + ((unsigned char)MCS);	\
			} else if (HTSetting.field.BW == BW_40) {	\
				rate_index = 152 + ((Antenna - 1) * 10) + ((unsigned char)HTSetting.field.ShortGI * 160) + ((unsigned char)MCS);	\
			} else if (HTSetting.field.BW == BW_80) {	\
				rate_index = 192 + ((Antenna - 1) * 10) + ((unsigned char)HTSetting.field.ShortGI * 160) + ((unsigned char)MCS);	\
			} else if (HTSetting.field.BW == BW_160) {	\
				rate_index = 232 + ((Antenna - 1) * 10) + ((unsigned char)HTSetting.field.ShortGI * 160) + ((unsigned char)MCS);	\
			}	\
		}	\
		else	\
		if (HTSetting.field.BW == BW_20) {			\
			rate_index = 108 +				\
			((unsigned char)HTSetting.field.ShortGI * 29) +	\
			((unsigned char)HTSetting.field.MCS);		\
		}							\
		else if (HTSetting.field.BW == BW_40) {			\
			rate_index = 117 +				\
			((unsigned char)HTSetting.field.ShortGI * 29) +	\
			((unsigned char)HTSetting.field.MCS);		\
		}							\
		else if (HTSetting.field.BW == BW_80) {			\
			rate_index = 127 +				\
			((unsigned char)HTSetting.field.ShortGI * 29) +	\
			((unsigned char)HTSetting.field.MCS);		\
		}							\
	}								\
	else								\
	if (HTSetting.field.MODE >= MODE_HTMIX)				\
	{								\
		if(_if == 1)	\
		{	\
			MCS = HTSetting.field.MCS;	\
			\
			if ((HTSetting.field.MODE == MODE_HTMIX) || (HTSetting.field.MODE == MODE_HTGREENFIELD))	\
				Antenna = (MCS >> 3) + 1;	\
			\
			/* map back to 1SS MCS , multiply by antenna numbers later */		\
			if (MCS > 7)		\
				MCS %= 8;		\
			\
			rate_index = 16 + ((unsigned char)HTSetting.field.BW * 24) + ((unsigned char)HTSetting.field.ShortGI * 48) + ((unsigned char)MCS);		\
		}else	\
			rate_index = 12 + ((unsigned char)HTSetting.field.BW *24) + ((unsigned char)HTSetting.field.ShortGI *48) + ((unsigned char)HTSetting.field.MCS);	\
	}								\
	else								\
		if (HTSetting.field.MODE == MODE_OFDM)				\
			rate_index = (unsigned char)(HTSetting.field.MCS) + 4;	\
		else if (HTSetting.field.MODE == MODE_CCK)			\
			rate_index = (unsigned char)(HTSetting.field.MCS);	\
									\
	if (rate_index < 0)						\
		rate_index = 0;						\
									\
	if (rate_index >= rate_count)					\
		rate_index = rate_count-1;				\
	\
	if(_if == 1)	{		\
		if (HTSetting.field.MODE != MODE_VHT)	\
			value = (_mcstbl[rate_index] * 5) / 10;	\
		else	\
			value =  _mcstbl[rate_index];	\
	}else		\
		value = (_mcstbl[rate_index] * 5) / 10;	\
	\
	return value;		\
}
#elif defined(RTCONFIG_WLMODULE_MT7915D_AP) || defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
/* mt_wifi/embadded/common/cmm_info */
int MCSMappingRateTable[] = {
	2,  4, 11, 22, 12,  18,  24,  36, 48,  72,  96, 108, 109, 110, 111, 112,/* CCK and OFDM */
	13, 26, 39, 52, 78, 104, 117, 130, 26,  52,  78, 104, 156, 208, 234, 260,
	39, 78, 117, 156, 234, 312, 351, 390, /* BW 20, 800ns GI, MCS 0~23 */
	27, 54, 81, 108, 162, 216, 243, 270, 54, 108, 162, 216, 324, 432, 486, 540,
	81, 162, 243, 324, 486, 648, 729, 810, /* BW 40, 800ns GI, MCS 0~23 */
	14, 29, 43, 57, 87, 115, 130, 144, 29, 59,   87, 115, 173, 230, 260, 288,
	43, 87, 130, 173, 260, 317, 390, 433, /* BW 20, 400ns GI, MCS 0~23 */
	30, 60, 90, 120, 180, 240, 270, 300, 60, 120, 180, 240, 360, 480, 540, 600,
	90, 180, 270, 360, 540, 720, 810, 900, /* BW 40, 400ns GI, MCS 0~23 */

	/*for 11ac:20 Mhz 800ns GI*/
	6,  13, 19, 26,  39,  52,  58,  65,  78,  90,     /*1ss mcs 0~8*/
	13, 26, 39, 52,  78,  104, 117, 130, 156, 180,     /*2ss mcs 0~8*/
	19, 39, 58, 78,  117, 156, 175, 195, 234, 260,   /*3ss mcs 0~9*/
	26, 52, 78, 104, 156, 208, 234, 260, 312, 360,     /*4ss mcs 0~8*/

	/*for 11ac:40 Mhz 800ns GI*/
	13,	27,	40,	54,	 81,  108, 121, 135, 162, 180,   /*1ss mcs 0~9*/
	27,	54,	81,	108, 162, 216, 243, 270, 324, 360,   /*2ss mcs 0~9*/
	40,	81,	121, 162, 243, 324, 364, 405, 486, 540,  /*3ss mcs 0~9*/
	54,	108, 162, 216, 324, 432, 486, 540, 648, 720, /*4ss mcs 0~9*/

	/*for 11ac:80 Mhz 800ns GI*/
	29,	58,	87,	117, 175, 234, 263, 292, 351, 390,   /*1ss mcs 0~9*/
	58,	117, 175, 243, 351, 468, 526, 585, 702, 780, /*2ss mcs 0~9*/
	87,	175, 263, 351, 526, 702, 0,	877, 1053, 1170, /*3ss mcs 0~9*/
	117, 234, 351, 468, 702, 936, 1053, 1170, 1404, 1560, /*4ss mcs 0~9*/

	/*for 11ac:160 Mhz 800ns GI*/
	58,	117, 175, 234, 351, 468, 526, 585, 702, 780, /*1ss mcs 0~9*/
	117, 234, 351, 468, 702, 936, 1053, 1170, 1404, 1560, /*2ss mcs 0~9*/
	175, 351, 526, 702, 1053, 1404, 1579, 1755, 2160, 0, /*3ss mcs 0~8*/
	234, 468, 702, 936, 1404, 1872, 2106, 2340, 2808, 3120, /*4ss mcs 0~9*/

	/*for 11ac:20 Mhz 400ns GI*/
	7,	14,	21,	28,  43,  57,   65,	 72,  86,  100,    /*1ss mcs 0~8*/
	14,	28,	43,	57,	 86,  115,  130, 144, 173, 200,    /*2ss mcs 0~8*/
	21,	43,	65,	86,	 130, 173,  195, 216, 260, 288,  /*3ss mcs 0~9*/
	28,	57,	86,	115, 173, 231,  260, 288, 346, 400,    /*4ss mcs 0~8*/

	/*for 11ac:40 Mhz 400ns GI*/
	15,	30,	45,	60,	 90,  120,  135, 150, 180, 200,  /*1ss mcs 0~9*/
	30,	60,	90,	120, 180, 240,  270, 300, 360, 400,  /*2ss mcs 0~9*/
	45,	90,	135, 180, 270, 360,  405, 450, 540, 600, /*3ss mcs 0~9*/
	60,	120, 180, 240, 360, 480,  540, 600, 720, 800, /*4ss mcs 0~9*/

	/*for 11ac:80 Mhz 400ns GI*/
	32,	65,	97,	130, 195, 260,  292, 325, 390, 433,  /*1ss mcs 0~9*/
	65,	130, 195, 260, 390, 520,  585, 650, 780, 866, /*2ss mcs 0~9*/
	97,	195, 292, 390, 585, 780,  0,	 975, 1170, 1300, /*3ss mcs 0~9*/
	130, 260, 390, 520, 780, 1040,	1170, 1300, 1560, 1733, /*4ss mcs 0~9*/

	/*for 11ac:160 Mhz 400ns GI*/
	65,	130, 195, 260, 390, 520,  585, 650, 780, 866, /*1ss mcs 0~9*/
	130, 260, 390, 520, 780, 1040,	1170, 1300, 1560, 1733, /*2ss mcs 0~9*/
	195, 390, 585, 780, 1170, 1560,	1755, 1950, 2340, 0, /*3ss mcs 0~8*/
	260, 520, 780, 1040, 1560, 2080,	2340, 2600, 3120, 3466, /*4ss mcs 0~9*/

	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37
}; /* 3*3 */

#define MAX_NUM_HE_BANDWIDTHS 4
#define MAX_NUM_HE_SPATIAL_STREAMS 4
#define MAX_NUM_HE_MCS_ENTRIES 12
unsigned short he_mcs_phyrate_mapping_table[MAX_NUM_HE_BANDWIDTHS][MAX_NUM_HE_SPATIAL_STREAMS][MAX_NUM_HE_MCS_ENTRIES] = {
	{ /*20 Mhz*/
		/* 1 SS */
		{
			/* DCM 0*/
			 8, 17, 25, 34, 51, 68, 77, 86, 103, 114, 129, 143
		},
		/* 2 SS */
		{
			/* DCM 0 */
			 17, 34, 51, 68, 103, 137, 154, 172, 206, 229, 258, 286
		},
		/* 3 SS */
		{
			/* DCM 0 */
			 25, 51, 77, 103, 154, 206, 232, 258, 309, 344, 387, 430
		},
		/* 4 SS */
		{
			/* DCM 0 */
			 34, 68, 103, 137, 206, 275, 309, 344, 412, 458, 516, 573
		}
	},
	{ /*40 Mhz*/
		/* 1 SS */
		{
			/* DCM 0*/
			 17, 34, 51, 68, 103, 137, 154, 172, 206, 229, 258, 286
		},
		/* 2 SS */
		{
			/* DCM 0 */
			 34, 68, 103, 137, 206, 275, 309, 344, 412, 458, 516, 573
		},
		/* 3 SS */
		{
			/* DCM 0 */
			 51, 103, 154, 206, 309, 412, 464, 516, 619, 688, 774, 860
		},
		/* 4 SS */
		{
			/* DCM 0 */
			 68, 137, 206, 275, 412, 550, 619, 688, 825, 917, 1032, 1147
		}
	},
	{ /*80 Mhz*/
		/* 1 SS */
		{
			/* DCM 0*/
			36, 72, 108, 144, 216, 288, 324, 360, 432, 480, 540, 600
		},
		/* 2 SS */
		{
			/* DCM 0 */
			 72, 144, 216, 288, 432, 576, 648, 720, 864, 960, 1080, 1201
		},
		/* 3 SS */
		{
			/* DCM 0 */
			 108, 216, 324, 432, 648, 864, 972, 1080, 1297, 1441, 1621, 1801
		},
		/* 4 SS */
		{
			/* DCM 0 */
			 144, 288, 432, 576, 864, 1152, 1297, 1141, 1729, 1921, 2161, 2401
		}
	},
	{ /*160 Mhz*/
		/* 1 SS */
		{
			/* DCM 0*/
			 72, 144, 216, 288, 432, 576, 648, 720, 864, 960, 1080, 1201
		},
		/* 2 SS */
		{
			/* DCM 0 */
			 144, 288, 432, 576, 864, 1152, 1297, 1441, 1729, 1921, 2161, 2401
		},
		/* 3 SS */
		{
			/* DCM 0 */
			 216, 432, 648, 864, 1297, 1729, 1945, 2161, 2594, 2882, 3242, 3602
		},
		/* 4 SS */
		{
			/* DCM 0 */
			 288, 576, 864, 1152, 1729, 2305, 2594, 2882, 3458, 3843, 4323, 4803
		},
	}
};

#define MAX_NUM_EHT_BANDWIDTHS 5
#define MAX_NUM_EHT_SPATIAL_STREAMS 16
#define MAX_NUM_EHT_MCS_ENTRIES 14
short eht_mcs_phyrate_mapping_table[MAX_NUM_EHT_BANDWIDTHS][MAX_NUM_EHT_MCS_ENTRIES] = {
	/*20 Mhz 1SS*/
	{8, 17, 25, 34, 51, 68, 77, 86, 103, 114, 129, 143, 154, 172},
	/*40 Mhz 1 SS */
	{17, 34, 51, 68, 103, 137, 154, 172, 206, 229, 258, 286, 309, 344},
	/*80 Mhz 1 SS */
	{36, 72, 108, 144, 216, 288, 324, 360, 432, 480, 540, 600, 648, 720},
	/*160 Mhz 1 SS */
	{72, 144, 216, 288, 432, 576, 648, 720, 864, 960, 1080, 1201, 1297, 1441},
	/*320 Mhz 1 SS */
	{144, 288, 432, 576, 864, 1152, 1297, 1441, 1729, 1921, 2161, 2401, 2594, 2882}
};

#if defined(RTCONFIG_MT799X)
#define is_MODE_EHT(m) ((m) == MODE_EHT)
#else
#define is_MODE_EHT(m) 0
#endif

#define FN_GETRATE(_fn_, _st_, _if, _mcstbl)						\
_fn_(_st_ HTSetting)							\
{									\
	unsigned char Antenna = 0;	\
	unsigned char MCS = HTSetting.field.MCS;	\
	unsigned char BW = HTSetting.field.BW;	\
	unsigned char NSS = ((HTSetting.field.MCS >> 4) & 0x3) + 1;		\
	int rate_count = sizeof(_mcstbl)/sizeof(int);	\
	int rate_index = 0;						\
	unsigned long value = 0;					\
									\
	if (is_MODE_EHT(HTSetting.field.MODE))				\
	{								\
		NSS = ((HTSetting.field.MCS >> 4) & 0x3) + 1;		\
		MCS = HTSetting.field.MCS & 0xf;			\
		if (NSS == 0) {						\
			NSS = 1;					\
		}							\
									\
		if (MCS >= MAX_NUM_EHT_MCS_ENTRIES)			\
			MCS = MAX_NUM_EHT_MCS_ENTRIES - 1;		\
									\
		if (NSS > MAX_NUM_EHT_SPATIAL_STREAMS)			\
			NSS = MAX_NUM_EHT_SPATIAL_STREAMS;		\
									\
		if (BW >= MAX_NUM_EHT_BANDWIDTHS)			\
			BW = MAX_NUM_EHT_BANDWIDTHS - 1;		\
									\
		value = eht_mcs_phyrate_mapping_table[BW][MCS];		\
		value = value * NSS;					\
									\
	}								\
	else if (HTSetting.field.MODE >= MODE_HE)				\
	{								\
		NSS = ((HTSetting.field.MCS >> 4) & 0x3) + 1;		\
		MCS = HTSetting.field.MCS & 0xf;	\
		if (NSS == 0) {								\
			NSS = 1;								\
		}								\
								\
		if (MCS >= MAX_NUM_HE_MCS_ENTRIES)								\
			MCS = MAX_NUM_HE_MCS_ENTRIES - 1;								\
								\
		if (NSS > MAX_NUM_HE_SPATIAL_STREAMS)								\
			NSS = MAX_NUM_HE_SPATIAL_STREAMS;								\
								\
		if (BW >= MAX_NUM_HE_BANDWIDTHS)								\
			BW = MAX_NUM_HE_BANDWIDTHS - 1;								\
								\
		NSS--;								\
								\
		value = he_mcs_phyrate_mapping_table[BW][NSS][MCS];								\
	}								\
	else								\
	{								\
		if (HTSetting.field.MODE >= MODE_VHT)				\
		{				\
			MCS = HTSetting.field.MCS & 0xf;	\
			Antenna = (HTSetting.field.MCS >> 4) + 1;	\
														\
			if (HTSetting.field.BW == BW_20) {	\
				rate_index = 112 + ((Antenna - 1) * 10) + ((unsigned char)HTSetting.field.ShortGI * 160) + ((unsigned char)MCS);	\
			} else if (HTSetting.field.BW == BW_40) {	\
				rate_index = 152 + ((Antenna - 1) * 10) + ((unsigned char)HTSetting.field.ShortGI * 160) + ((unsigned char)MCS);	\
			} else if (HTSetting.field.BW == BW_80) {	\
				rate_index = 192 + ((Antenna - 1) * 10) + ((unsigned char)HTSetting.field.ShortGI * 160) + ((unsigned char)MCS);	\
			} else if (HTSetting.field.BW == BW_160) {	\
				rate_index = 232 + ((Antenna - 1) * 10) + ((unsigned char)HTSetting.field.ShortGI * 160) + ((unsigned char)MCS);	\
			}	\
		}								\
		else								\
		if (HTSetting.field.MODE >= MODE_HTMIX)				\
		{								\
			MCS = HTSetting.field.MCS;	\
			\
			if ((HTSetting.field.MODE == MODE_HTMIX) || (HTSetting.field.MODE == MODE_HTGREENFIELD))	\
				Antenna = (MCS >> 3) + 1;	\
			\
			/* map back to 1SS MCS , multiply by antenna numbers later */		\
			if (MCS > 7)		\
				MCS %= 8;		\
			\
			rate_index = 16 + ((unsigned char)HTSetting.field.BW * 24) + ((unsigned char)HTSetting.field.ShortGI * 48) + ((unsigned char)MCS);		\
		}								\
		else								\
			if (HTSetting.field.MODE == MODE_OFDM)				\
				rate_index = (unsigned char)(HTSetting.field.MCS) + 4;		\
			else if (HTSetting.field.MODE == MODE_CCK)			\
				rate_index = (unsigned char)(HTSetting.field.MCS);	\
									\
		if (rate_index < 0)						\
			rate_index = 0;						\
										\
		if (rate_index >= rate_count)					\
			rate_index = rate_count-1;				\
		\
		if (HTSetting.field.MODE < MODE_VHT)	\
			value = (_mcstbl[rate_index] * 5) / 10;		\
		else	\
			value =  _mcstbl[rate_index];	\
		\
		if (HTSetting.field.MODE >= MODE_HTMIX && HTSetting.field.MODE < MODE_VHT)	\
		value *= Antenna;	\
	}								\
	return value;		\
}
#else

#define FN_GETRATE(_fn_, _st_, _if, _mcstbl)						\
_fn_(_st_ HTSetting)							\
{									\
	int rate_count = sizeof(_mcstbl)/sizeof(int);	\
	int rate_index = 0;						\
	int value = 0;	\
									\
	if (HTSetting.field.MODE >= MODE_VHT)				\
	{								\
		if (HTSetting.field.BW == BW_20) {			\
			rate_index = 108 +				\
			((unsigned char)HTSetting.field.ShortGI * 29) +	\
			((unsigned char)HTSetting.field.MCS);		\
		}							\
		else if (HTSetting.field.BW == BW_40) {			\
			rate_index = 117 +				\
			((unsigned char)HTSetting.field.ShortGI * 29) +	\
			((unsigned char)HTSetting.field.MCS);		\
		}							\
		else if (HTSetting.field.BW == BW_80) {			\
			rate_index = 127 +				\
			((unsigned char)HTSetting.field.ShortGI * 29) +	\
			((unsigned char)HTSetting.field.MCS);		\
		}							\
	}								\
	else								\
	if (HTSetting.field.MODE >= MODE_HTMIX)				\
	{								\
		rate_index = 12 + ((unsigned char)HTSetting.field.BW *24) + ((unsigned char)HTSetting.field.ShortGI *48) + ((unsigned char)HTSetting.field.MCS);	\
	}								\
	else								\
	if (HTSetting.field.MODE == MODE_OFDM)				\
		rate_index = (unsigned char)(HTSetting.field.MCS) + 4;	\
	else if (HTSetting.field.MODE == MODE_CCK)			\
		rate_index = (unsigned char)(HTSetting.field.MCS);	\
									\
	if (rate_index < 0)						\
		rate_index = 0;						\
									\
	if (rate_index >= rate_count)					\
		rate_index = rate_count-1;				\
	\
	if (HTSetting.field.MODE != MODE_VHT)	\
		value = (_mcstbl[rate_index] * 5) / 10;	\
	else	\
		value =  _mcstbl[rate_index];	\
	return value;		\
}

#endif


#if defined(RTCONFIG_HAS_5G)
#if defined(RTCONFIG_WLMODULE_MT7663E_AP) || defined(RTCONFIG_WLMODULE_MT7629_AP) || defined(RTCONFIG_WLMODULE_MT7622_AP)
int FN_GETRATE(getRate,      MACHTTRANSMIT_SETTING_for_5G, 1, MCSMappingRateTable_5G)		//getRate   (MACHTTRANSMIT_SETTING_for_5G)
#else
int FN_GETRATE(getRate,      MACHTTRANSMIT_SETTING_for_5G, 1, MCSMappingRateTable)		//getRate   (MACHTTRANSMIT_SETTING_for_5G)
#endif
#endif	/* RTCONFIG_HAS_5G */
int FN_GETRATE(getRate_2g,   MACHTTRANSMIT_SETTING_for_2G, 0, MCSMappingRateTable)		//getRate_2g(MACHTTRANSMIT_SETTING_for_2G)



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

#if defined(RTCONFIG_NL80211)
/* reference from QCA platform */
int diff_current_bssid(int unit, char bssid_str[])
{
	char cur_bssid[18];
	int i, diff;

	get_pap_bssid(unit, cur_bssid, sizeof(cur_bssid));
	if (strcmp(cur_bssid, "00:00:00:00:00:00") != 0) {
		for (i = 0; i < 17; i++) {
			diff = abs((int)(*(cur_bssid + i) - *(bssid_str + i)));
			if (diff == 0 || diff == 32)
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
#endif /* RTCONFIG_NL80211 */
#endif /* RTCONFIG_AMAS */

#if defined(RTCONFIG_MTK_BSD) && defined(RTCONFIG_MT799X)
/* Add/Del mac addr to/from non_steer_list */
char *set_steer(const char *mac,int val)
{
	if (val == 1) {
		eval("mapd_cli", "/tmp/mapd_ctrl", "non_steer_list", "add", mac);
	} else if (val == 0) {
		eval("mapd_cli", "/tmp/mapd_ctrl", "non_steer_list", "del", mac);
	}

	return NULL;
}
#endif

#if defined(RTCONFIG_MULTILAN_CFG)
/* get all wireless interfaces except onboarding/prelink interface */
char* get_wlan_ifnames(void)
{
	int band;
	char word[16], *next;
	char nv[16], result[256];
#if defined(RTCONFIG_VIF_ONBOARDING)
	char *obvif = NULL;

	snprintf(nv, sizeof(nv), "wl0.%d_ifname",
		(!nvram_get_int("re_mode")) ? nvram_get_int("obvif_cap_subunit") : nvram_get_int("obvif_re_subunit"));
	obvif = nvram_safe_get(nv);
#endif
#if defined(RTCONFIG_PRELINK)
	char *plkif = NULL;

	snprintf(nv, sizeof(nv), "wl%d.%d_ifname",
		(num_of_wl_if() - 1),
		(!nvram_get_int("re_mode")) ? nvram_get_int("plk_cap_subunit") : nvram_get_int("plk_re_subunit"));
	plkif = nvram_safe_get(nv);
#endif

	memset(result, 0, sizeof(result));
	snprintf(result, sizeof(result), "%s ", nvram_safe_get("wl_ifnames"));

	for (band = 0; band < MAX_NR_WL_IF; ++band) {
		SKIP_ABSENT_BAND(band);
		snprintf(nv, sizeof(nv), "wl%d_vifs", band);
		foreach (word, nvram_safe_get(nv), next) {
#if defined(RTCONFIG_VIF_ONBOARDING)
			if (obvif && !strcmp(word, obvif))
				continue;
#endif
#if defined(RTCONFIG_PRELINK)
			if (plkif && !strcmp(word, plkif))
				continue;
#endif
			strlcat(result, word, sizeof(result));
			strlcat(result, " ", sizeof(result));
		}
	}

	return strdup(result);
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

#if defined(RTCONFIG_EXTEND_MTK_RFB_CONFIG)
/*
 * structure and function for merging .dat config for ralink
 */

/* 
 * List of key names which should be assigned <bssid_num> parameters.
 * 	ex: WirelessMode=24;24;24
 * 
 * Set by manually inspecting .dat files, may miss some values.
 * */
static const char* Config_multi_dict[] = 
{
	"FtSupport", 
	"ApEnable", 
	"VHT_BW", 
	"VHT_BW_SIGNAL", 
	"EHT_ApBw", 
	"EHT_ApNsepPriAccess", 
	"EHT_ApTxopSharing", 
	"TxRate", 
//	"Dot11vMbssid", always 16 parameters
	"PweMethod", 
	"TidMapping",
};

int initConfig(Config *config) {
	config->capacity = INIT_DAT_LENGTH;
	config->count = 0;
	config->items = (ConfigItem *)malloc(config->capacity * sizeof(ConfigItem));
	if (config->items == NULL) {
		dbg("[%s]malloc failed.\n", __func__);
		return -1;
	}
	return 0;
}

void freeConfig(Config *config) {
	free(config->items);
	config->items = NULL;
	config->capacity = 0;
	config->count = 0;
}

int isKeyInArray(const char *str, const char *arr[], int arr_size) {
	for (int i = 0; i < arr_size; i++) {
		if (strcmp(str, arr[i]) == 0) {
			return 1;
		}
	}
	return 0;
}

int addConfigItem(Config *config, const char *key, const char *value) {
	if (config->count >= config->capacity) {
		config->capacity *= 2;
		config->items = (ConfigItem *)realloc(config->items, config->capacity * sizeof(ConfigItem));
	}

	ConfigItem *item = &config->items[config->count++];
	snprintf(item->key, MAX_K_LEN, "%s", key);
	item->key[MAX_K_LEN - 1] = '\0';
	snprintf(item->value, MAX_V_LEN, "%s", value);
	item->value[MAX_V_LEN - 1] = '\0';

	return 0;
}

int parseLine(char *line, char *key, char *value) {
	char *equals = strchr(line, '=');
	if (!equals) return -1;

	*equals = '\0';
	snprintf(key, MAX_K_LEN, "%s", line);
	key[MAX_K_LEN - 1] = '\0';

	snprintf(value, MAX_V_LEN, "%s", equals + 1);
	value[MAX_V_LEN - 1] = '\0';

	return 0;
}

int readConfigFile(const char *filename, Config *config) {
	FILE *file = fopen(filename, "r");
	if (!file) return -1;

	char line[MAX_LINE_LENGTH];
	while (fgets(line, MAX_LINE_LENGTH, file)) {
		// Remove newline character
		line[strcspn(line, "\n")] = '\0';

		// Skip empty lines and comments
		if (line[0] == '\0' || line[0] == '#') continue;

		char key[MAX_K_LEN];
		char value[MAX_V_LEN];

		if (parseLine(line, key, value) == 0) {
			addConfigItem(config, key, value);
		}
	}

	fclose(file);
	return 0;
}

int findItemIndex(Config *config, const char *key) {
	for (int i = 0; i < config->count; i++) {
		if (strcmp(config->items[i].key, key) == 0) {
			return i;
		}
	}
	return -1;
}

void mergeConfigs(Config *base, Config *additional, int bssid_num) {
	int multi_dict_size = sizeof(Config_multi_dict) / sizeof(Config_multi_dict[0]);
	for (int i = 0; i < additional->count; i++) {
		const char *key_ori = additional->items[i].key;
		const char *value_ori = additional->items[i].value;
		char key[MAX_K_LEN] = {0};
		char value[MAX_V_LEN] = {0};
		snprintf(key, MAX_K_LEN, "%s", key_ori);
		snprintf(value, MAX_V_LEN, "%s", value_ori);

		if(key[0] == '\0'){
			dbg("[%s] Empty key\n", __func__);
			continue;
		}

		if (findItemIndex(base, key) == -1) {
			// if this key should be applied <bssid_num> times, generate string with sep of ';' instead.
			if(isKeyInArray(key, Config_multi_dict, multi_dict_size) == 1) {
				char value_temp[MAX_V_LEN] = {0};
				char *tok = NULL;
				char *saveptr = NULL;
				tok = strtok_r(value, ";", &saveptr);
				for (int j = 0; j < bssid_num; j++) {
					strncat(value_temp, tok, MAX_V_LEN);
					if (j < (bssid_num - 1))
						strncat(value_temp, ";", MAX_V_LEN);
				}
				snprintf(value, MAX_V_LEN, "%s", value_temp);
				addConfigItem(base, key, value);
			}	
			else{
				addConfigItem(base, key, value);
			}
		}
	}
}

void printConfigtoFile(Config *config) {
	FILE *fp;
	if (!(fp=fopen("/tmp/band0_merge.dat", "w+")))
		return;

	fprintf(fp, "#The word of \"Default\" must not be removed\n");
	fprintf(fp, "Default\n");

	for (int i = 0; i < config->count; i++) {
		fprintf(fp, "%s=%s\n", config->items[i].key, config->items[i].value);
	}
	fclose(fp);

}
#endif
