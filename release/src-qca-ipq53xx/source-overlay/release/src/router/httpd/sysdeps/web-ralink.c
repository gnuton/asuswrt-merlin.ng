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
#include <ralink.h>
#include <iwlib.h>
#include <stapriv.h>
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


/************************ CONSTANTS & MACROS ************************/

/*
 * Constants fof WE-9->15
 */
#define IW15_MAX_FREQUENCIES	16
#define IW15_MAX_BITRATES	8
#define IW15_MAX_TXPOWER	8
#define IW15_MAX_ENCODING_SIZES	8
#define IW15_MAX_SPY		8
#define IW15_MAX_AP		8

/****************************** TYPES ******************************/

/*
 *	Struct iw_range up to WE-15
 */
struct	iw15_range
{
	__u32		throughput;
	__u32		min_nwid;
	__u32		max_nwid;
	__u16		num_channels;
	__u8		num_frequency;
	struct iw_freq	freq[IW15_MAX_FREQUENCIES];
	__s32		sensitivity;
	struct iw_quality	max_qual;
	__u8		num_bitrates;
	__s32		bitrate[IW15_MAX_BITRATES];
	__s32		min_rts;
	__s32		max_rts;
	__s32		min_frag;
	__s32		max_frag;
	__s32		min_pmp;
	__s32		max_pmp;
	__s32		min_pmt;
	__s32		max_pmt;
	__u16		pmp_flags;
	__u16		pmt_flags;
	__u16		pm_capa;
	__u16		encoding_size[IW15_MAX_ENCODING_SIZES];
	__u8		num_encoding_sizes;
	__u8		max_encoding_tokens;
	__u16		txpower_capa;
	__u8		num_txpower;
	__s32		txpower[IW15_MAX_TXPOWER];
	__u8		we_version_compiled;
	__u8		we_version_source;
	__u16		retry_capa;
	__u16		retry_flags;
	__u16		r_time_flags;
	__s32		min_retry;
	__s32		max_retry;
	__s32		min_r_time;
	__s32		max_r_time;
	struct iw_quality	avg_qual;
};

/*
 * Union for all the versions of iwrange.
 * Fortunately, I mostly only add fields at the end, and big-bang
 * reorganisations are few.
 */
union	iw_range_raw
{
	struct iw15_range	range15;	/* WE 9->15 */
	struct iw_range		range;		/* WE 16->current */
};

/*
 * Offsets in iw_range struct
 */
#define iwr15_off(f)	( ((char *) &(((struct iw15_range *) NULL)->f)) - \
			  (char *) NULL)
#define iwr_off(f)	( ((char *) &(((struct iw_range *) NULL)->f)) - \
			  (char *) NULL)

/* Disable runtime version warning in ralink_get_range_info() */
int	iw_ignore_version_sp = 0;

/*------------------------------------------------------------------*/
/*
 * Get the range information out of the driver
 */
int
ralink_get_range_info(iwrange *	range, char* buffer, int length)
{
  union iw_range_raw *	range_raw;

  /* Point to the buffer */
  range_raw = (union iw_range_raw *) buffer;

  /* For new versions, we can check the version directly, for old versions
   * we use magic. 300 bytes is a also magic number, don't touch... */
  if (length < 300)
    {
      /* That's v10 or earlier. Ouch ! Let's make a guess...*/
      range_raw->range.we_version_compiled = 9;
    }

  /* Check how it needs to be processed */
  if (range_raw->range.we_version_compiled > 15)
    {
      /* This is our native format, that's easy... */
      /* Copy stuff at the right place, ignore extra */
      memcpy((char *) range, buffer, sizeof(iwrange));
    }
  else
    {
      /* Zero unknown fields */
      bzero((char *) range, sizeof(struct iw_range));

      /* Initial part unmoved */
      memcpy((char *) range,
	     buffer,
	     iwr15_off(num_channels));
      /* Frequencies pushed futher down towards the end */
      memcpy((char *) range + iwr_off(num_channels),
	     buffer + iwr15_off(num_channels),
	     iwr15_off(sensitivity) - iwr15_off(num_channels));
      /* This one moved up */
      memcpy((char *) range + iwr_off(sensitivity),
	     buffer + iwr15_off(sensitivity),
	     iwr15_off(num_bitrates) - iwr15_off(sensitivity));
      /* This one goes after avg_qual */
      memcpy((char *) range + iwr_off(num_bitrates),
	     buffer + iwr15_off(num_bitrates),
	     iwr15_off(min_rts) - iwr15_off(num_bitrates));
      /* Number of bitrates has changed, put it after */
      memcpy((char *) range + iwr_off(min_rts),
	     buffer + iwr15_off(min_rts),
	     iwr15_off(txpower_capa) - iwr15_off(min_rts));
      /* Added encoding_login_index, put it after */
      memcpy((char *) range + iwr_off(txpower_capa),
	     buffer + iwr15_off(txpower_capa),
	     iwr15_off(txpower) - iwr15_off(txpower_capa));
      /* Hum... That's an unexpected glitch. Bummer. */
      memcpy((char *) range + iwr_off(txpower),
	     buffer + iwr15_off(txpower),
	     iwr15_off(avg_qual) - iwr15_off(txpower));
      /* Avg qual moved up next to max_qual */
      memcpy((char *) range + iwr_off(avg_qual),
	     buffer + iwr15_off(avg_qual),
	     sizeof(struct iw_quality));
    }

  /* We are now checking much less than we used to do, because we can
   * accomodate more WE version. But, there are still cases where things
   * will break... */
  if (!iw_ignore_version_sp)
    {
      /* We don't like very old version (unfortunately kernel 2.2.X) */
      if (range->we_version_compiled <= 10)
	{
	  fprintf(stderr, "Warning: Driver for device %s has been compiled with an ancient version\n", "raxx");
	  fprintf(stderr, "of Wireless Extension, while this program support version 11 and later.\n");
	  fprintf(stderr, "Some things may be broken...\n\n");
	}

      /* We don't like future versions of WE, because we can't cope with
       * the unknown */
      if (range->we_version_compiled > WE_MAX_VERSION)
	{
	  fprintf(stderr, "Warning: Driver for device %s has been compiled with version %d\n", "raxx", range->we_version_compiled);
	  fprintf(stderr, "of Wireless Extension, while this program supports up to version %d.\n", WE_VERSION);
	  fprintf(stderr, "Some things may be broken...\n\n");
	}

      /* Driver version verification */
      if ((range->we_version_compiled > 10) &&
	 (range->we_version_compiled < range->we_version_source))
	{
	  fprintf(stderr, "Warning: Driver for device %s recommend version %d of Wireless Extension,\n", "raxx", range->we_version_source);
	  fprintf(stderr, "but has been compiled with version %d, therefore some driver features\n", range->we_version_compiled);
	  fprintf(stderr, "may not be available...\n\n");
	}
      /* Note : we are only trying to catch compile difference, not source.
       * If the driver source has not been updated to the latest, it doesn't
       * matter because the new fields are set to zero */
    }

  /* Don't complain twice.
   * In theory, the test apply to each individual driver, but usually
   * all drivers are compiled from the same kernel. */
  iw_ignore_version_sp = 1;

  return (0);
}


char* GetBW(int MODE, int BW)
{
	switch(BW)
	{
		case BW_10:
#if defined(RTCONFIG_MT799X)
#ifndef is_MODE_EHT
#define is_MODE_EHT(m) ((m) == MODE_EHT)
#endif
			if (is_MODE_EHT(MODE))
				return "320M";	//the BW in eht_mcs_phyrate_mapping_table[BW][MCS] is only 0~4 for 20/40/80/160/320.
#endif
			return "10M";

		case BW_20:
			return "20M";

		case BW_40:
			return "40M";

#if defined(RTAC52U) || defined(RTAC51U) || defined(RTN54U) || defined(RTAC1200HP) || defined(RTAC54U) || defined(RTAC1200) || defined(RTAC1200V2) || defined(RTAC1200GA1) || defined(RTAC1200GU) || defined(RTACRH18) || defined(RT4GAC86U) || defined(RTCONFIG_WLMODULE_MT7915D_AP) || defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
		case BW_80:
			return "80M";
#endif
#if defined(RTCONFIG_BW160M)
		case BW_160:
			return "160M";
		case BW_8080:
			return "80M+80M";
#endif
#if defined(RTCONFIG_BW320M)
		case BW_320:
			return "320M";
#endif

		default:
			return "N/A";
	}
}

char* GetPhyMode(int Mode)
{
	switch(Mode)
	{
		case MODE_CCK:
			return "CCK";

		case MODE_OFDM:
			return "OFDM";
		case MODE_HTMIX:
			return "HTMIX";

		case MODE_HTGREENFIELD:
			return "GREEN";

#if defined(RTAC52U) || defined(RTAC51U)  || defined(RTN54U) || defined(RTAC1200HP) || defined(RTAC54U) || defined(RTAC1200) || defined(RTAC1200V2) || defined(RTAC1200GA1) || defined(RTAC1200GU) || defined(RTACRH18) || defined(RT4GAC86U) || defined(RTCONFIG_WLMODULE_MT7915D_AP) || defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
		case MODE_VHT:
			return "VHT";
#endif
#if defined(RTCONFIG_WLMODULE_MT7915D_AP) || defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
		case MODE_HE:
			return "HE";
#if !defined(RTCONFIG_MT799X)
		case MODE_HE_5G:
			return "HE_5G";
#endif
		case MODE_HE_24G:
			return "HE2G";
		case MODE_HE_SU:
			return "HE_SU";
		case MODE_HE_EXT_SU:
			return "HE_EXT_SU";
		case MODE_HE_TRIG:
			return "HE_TRIG";
		case MODE_HE_MU:
			return "HE_MU";
#endif
#if defined(RTCONFIG_MT799X)
		case MODE_EHT:
			return "EHT";
		case MODE_EHT_ER_SU:
			return "EHT_ER_SU";
		case MODE_EHT_TB:
			return "EHT_TB";
		case MODE_EHT_MU:
			return "EHT_MU";
#endif

		default:
			return "N/A";
	}
}



int
ej_wl_status(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
	int retval = 0;
	int ii = 0;
	char word[256], *next;

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
	return ej_wl_status(eid, wp, argc, argv, 0);
}

int get_ra_sta_info_by_unit(int unit, int subunit, unsigned char *data, int data_len)
{
	char prefix[16];
	struct iwreq wrq3;
	int iocmd = RTPRIV_IOCTL_GET_MAC_TABLE;
	char *ifname;
	int r;

	if(unit >= MAX_NR_WL_IF || subunit > num_of_mssid_support(unit) || data == NULL || data_len <= 0)
		return -1;

#if defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
	iocmd = RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT;
#endif

	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	if (nvram_pf_get_int(prefix, "radio") <= 0)
		return 0;

	if (subunit > 0) {
		snprintf(prefix, sizeof(prefix), "wl%d.%d_", unit, subunit);
		if (nvram_pf_get_int(prefix, "bss_enabled") <= 0)
			return 0;
	}

	if ((ifname = nvram_pf_get(prefix, "ifname")) == NULL || *ifname == '\0') {
		return -1;
	}
	memset(data, 0, data_len);
	wrq3.u.data.pointer = data;
	wrq3.u.data.length = data_len;
	wrq3.u.data.flags = 0;

	if ((r = wl_ioctl(ifname, iocmd, &wrq3)) < 0) {
		_dprintf("%s: Take MAC table from (%d.%d) i/f %s fail! ret %d, errno %d (%s)\n",
			__func__, unit, subunit, ifname, r, errno, strerror(errno));
		return -1;
	}
	return wrq3.u.data.length;
}

static int
wl_status(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
	int ret = 0, channel, i, radar_cnt = 0, radar_list[32];
	uint64_t m = 0;
	struct iw_range	range;
	double freq;
	struct iwreq wrq0;
	struct iwreq wrq1;
	struct iwreq wrq2;
	unsigned long phy_mode;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_", *ifname;
	int wl_mode_x;
	int r;
#if defined(RTCONFIG_AMAS)
	uint64_t all_ch_m = 0, unavbl_ch_m = 0;
#endif

	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	ifname = nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp)));

#if 0
	if (nvram_match(strlcat_r(prefix, "radio", tmp, sizeof(tmp)), "0"))
	{
		ret+=websWrite(wp, "%s radio is disabled\n",
			wl_nband_name(nvram_pf_get(prefix, "nband")));
		return ret;
	}
#else
	if (!get_radio_status(ifname))
	{
#if defined(BAND_2G_ONLY)
		ret+=websWrite(wp, "2.4 GHz radio is disabled\n");
#else
		ret+=websWrite(wp, "%s radio is disabled\n",
			wl_nband_name(nvram_pf_get(prefix, "nband")));
#endif
		return ret;
	}
#endif

	if (wl_ioctl(ifname, SIOCGIWAP, &wrq0) < 0)
	{
#if defined(BAND_2G_ONLY)
		ret+=websWrite(wp, "2.4 GHz radio is disabled\n");
#else
		ret+=websWrite(wp, "%s radio is disabled\n",
			wl_nband_name(nvram_pf_get(prefix, "nband")));
#endif
		return ret;
	}

	wrq0.u.ap_addr.sa_family = ARPHRD_ETHER;
	ret+=websWrite(wp, "MAC address	: %02X:%02X:%02X:%02X:%02X:%02X\n",
			(unsigned char)wrq0.u.ap_addr.sa_data[0],
			(unsigned char)wrq0.u.ap_addr.sa_data[1],
			(unsigned char)wrq0.u.ap_addr.sa_data[2],
			(unsigned char)wrq0.u.ap_addr.sa_data[3],
			(unsigned char)wrq0.u.ap_addr.sa_data[4],
			(unsigned char)wrq0.u.ap_addr.sa_data[5]);

	if (wl_ioctl(ifname, SIOCGIWFREQ, &wrq1) < 0)
		return ret;

	char buffer[sizeof(iwrange) * 2];
	bzero(buffer, sizeof(buffer));
	wrq2.u.data.pointer = (caddr_t) buffer;
	wrq2.u.data.length = sizeof(buffer);
	wrq2.u.data.flags = 0;

	if (wl_ioctl(ifname, SIOCGIWRANGE, &wrq2) < 0)
		return ret;
	if (ralink_get_range_info(&range, buffer, wrq2.u.data.length) < 0)
		return ret;

#if defined(RTN65U)
	if (unit == 0 && get_model() == MODEL_RTN65U)
	{
		FILE *fp;
		phy_mode = 0;
		if((fp = fopen("/etc/Wireless/iNIC/iNIC_ap.dat", "r")) != NULL)
		{
			while(fgets(tmp, sizeof(tmp), fp) != NULL)
			{
				if(strncmp(tmp, "WirelessMode=", 13) == 0)
				{
					phy_mode = atoi(tmp + 13);
					break;
				}
			}
			fclose(fp);
		}
	}
	else
	{
#endif	/* RTN65U */
	bzero(buffer, sizeof(unsigned long));
	wrq2.u.data.length = sizeof(unsigned long);
	wrq2.u.data.pointer = (caddr_t) buffer;
#if defined(RTCONFIG_MT799X)
	wrq2.u.data.flags = ASUS_SUBCMD_GET_PHY_MODE;

	if (wl_ioctl(ifname, RTPRIV_IOCTL_ASUSCMD, &wrq2) < 0)
		return ret;
#else
	wrq2.u.data.flags = RT_OID_GET_PHY_MODE;

	if (wl_ioctl(ifname, RT_PRIV_IOCTL, &wrq2) < 0)
		return ret;
#endif

	if(wrq2.u.mode == (__u32) buffer) //.u.mode is at the same location as u.data.pointer
	{ //new wifi driver
		phy_mode = 0;
		memcpy(&phy_mode, wrq2.u.data.pointer, wrq2.u.data.length);
	}
	else
		phy_mode=wrq2.u.mode;
#if defined(RTN65U)
	}
#endif	/* RTN65U */

	freq = iw_freq2float(&(wrq1.u.freq));
	if (freq < KILO)
		channel = (int) freq;
	else
	{
		channel = iw_freq_to_channel(freq, &range);
		if (channel < 0)
			return ret;
	}

	wl_mode_x = nvram_get_int(strlcat_r(prefix, "mode_x", tmp, sizeof(tmp)));
	if	(wl_mode_x == 1)
		ret+=websWrite(wp, "OP Mode		: WDS Only\n");
	else if (wl_mode_x == 2)
		ret+=websWrite(wp, "OP Mode		: Hybrid\n");
	else
		ret+=websWrite(wp, "OP Mode		: AP\n");

#if defined(RTAC52U) || defined(RTAC51U) || defined(RTN54U)  || defined(RTAC1200HP) || defined(RTAC54U) || defined(RTAC1200) || defined(RTAC1200GA1) || defined(RTAC1200GU) || defined(RTAC85U) || defined(RTAC85P) || defined(MTK_REP) || defined(RTACRH26) || defined(TUFAC1750)
	if (unit == 1)
	{
		char *p = tmp;
		size_t len = sizeof(tmp);

		*tmp = '\0';
		if(phy_mode & WMODE_A) {
			strlcat(p, "/a", len);
			p += 2;
			len -= 2;
		}
		if(phy_mode & WMODE_B) {
			strlcat(p, "/b", len);
			p += 2;
			len -= 2;
		}
		if(phy_mode & WMODE_G) {
			strlcat(p, "/g", len);
			p += 2;
			len -= 2;
		}
		if(phy_mode & WMODE_GN) {
			strlcat(p, "/n", len);	//N in 2G
			p += 2;
			len -= 2;
		}
		if(phy_mode & WMODE_AN) {
			strlcat(p, "/n", len);	//N in 5G
			p += 2;
			len -= 2;
		}
		if(phy_mode & WMODE_AC) {
			strlcat(p, "/ac", len);
			p += 3;
			len -= 3;
		}
		if(*tmp != '\0')
			ret+=websWrite(wp, "Phy Mode	: 11%s\n", tmp+1); // skip first '/'
	}
	else
#elif defined(RTCONFIG_WLMODULE_MT7663E_AP) || defined(RTCONFIG_WLMODULE_MT7629_AP) || defined(RTCONFIG_WLMODULE_MT7915D_AP) || defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
	if (unit == 1 || unit == 0
#if defined(RTCONFIG_WIFI7)
	  ||unit == 2
#endif
	   )
	{
		char *p = tmp;
		size_t len = sizeof(tmp);

		*tmp = '\0';
		if(phy_mode & WMODE_A) {
			strlcat(p, "/a", len);
			p += 2;
			len -= 2;
		}
		if(phy_mode & WMODE_B) {
			strlcat(p, "/b", len);
			p += 2;
			len -= 2;
		}
		if(phy_mode & WMODE_G) {
			strlcat(p, "/g", len);
			p += 2;
			len -= 2;
		}
		if(phy_mode & WMODE_GN) {
			strlcat(p, "/n", len);	//N in 2G
			p += 2;
			len -= 2;
		}
		if(phy_mode & WMODE_AN) {
			strlcat(p, "/n", len);	//N in 5G
			p += 2;
			len -= 2;
		}
		if(phy_mode & WMODE_AC) {
			strlcat(p, "/ac", len);
			p += 3;
			len -= 3;
		}
#if defined(RTCONFIG_WLMODULE_MT7915D_AP) || defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
		if(phy_mode & (WMODE_AX_24G | WMODE_AX_5G | WMODE_AX_6G)) {
			strlcat(p, "/ax", len);
			p += 3;
			len -= 3;
		}
#endif		
#if defined(RTCONFIG_MT799X)
		if(phy_mode & (WMODE_BE_24G | WMODE_BE_5G | WMODE_BE_6G)) {
			strlcat(p, "/be", len);
			p += 3;
			len -= 3;
		}
#endif

		if(*tmp != '\0')
			ret+=websWrite(wp, "Phy Mode	: 11%s\n", tmp+1); // skip first '/'
	}
	else
#endif
	if (phy_mode==PHY_11BG_MIXED)
		ret+=websWrite(wp, "Phy Mode	: 11b/g\n");
	else if (phy_mode==PHY_11B)
		ret+=websWrite(wp, "Phy Mode	: 11b\n");
	else if (phy_mode==PHY_11A)
		ret+=websWrite(wp, "Phy Mode	: 11a\n");
	else if (phy_mode==PHY_11ABG_MIXED)
		ret+=websWrite(wp, "Phy Mode	: 11a/b/g\n");
	else if (phy_mode==PHY_11G)
		ret+=websWrite(wp, "Phy Mode	: 11g\n");
	else if (phy_mode==PHY_11ABGN_MIXED)
		ret+=websWrite(wp, "Phy Mode	: 11a/b/g/n\n");
	else if (phy_mode==PHY_11N)
		ret+=websWrite(wp, "Phy Mode	: 11n\n");
	else if (phy_mode==PHY_11GN_MIXED)
		ret+=websWrite(wp, "Phy Mode	: 11g/n\n");
	else if (phy_mode==PHY_11AN_MIXED)
		ret+=websWrite(wp, "Phy Mode	: 11a/n\n");
	else if (phy_mode==PHY_11BGN_MIXED)
		ret+=websWrite(wp, "Phy Mode	: 11b/g/n\n");
	else if (phy_mode==PHY_11AGN_MIXED)
		ret+=websWrite(wp, "Phy Mode	: 11a/g/n\n");

#if defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
	unsigned long long bitrate;
	int bw;
	bitrate = get_bitrate(ifname);
	ret += websWrite(wp, "Bit Rate	: %llu", bitrate);
	if ((get_ch_cch_bw(ifname, NULL, NULL, &bw)) == 1) {
		ret += websWrite(wp, ", %dMHz", bw);
	}
	ret += websWrite(wp, "\n");
#endif


	ret+=websWrite(wp, "Channel		: %d", channel);
	if (unit < MAX_NR_WL_IF) {
		radar_cnt = get_radar_channel_list(unit, radar_list, ARRAY_SIZE(radar_list));
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
	if (m) {
		ret += websWrite(wp, " (Radar: %s)", bitmask2chlist5g(m, ","));
	}
	ret += websWrite(wp, "\n");

#if defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
	char data[sizeof(RT_802_11_MAC_TABLE_5G)]; // 2G, 5G use same structure
#else
	char data[16384];
#endif
	if (get_ra_sta_info_by_unit(unit, 0, data, sizeof(data)) <= 0)
		return ret;

	RT_802_11_MAC_TABLE_5G* mp =(RT_802_11_MAC_TABLE_5G*)data;
	RT_802_11_MAC_TABLE_2G* mp2=(RT_802_11_MAC_TABLE_2G*)data;

#if defined(RTCONFIG_MLO) && defined(RTCONFIG_MT799X)
#ifndef MAX3
#define MAX3(a,b,c) (a>b?(a>c?a:c):(b>c?b:c))
#endif
#define WL_LOG_FMT "%-4s%-8s%-5s%-4s%-5s"
	ret+=websWrite(wp, "\nStations List			   \n");
	ret+=websWrite(wp, "----------------------------------------\n");
	ret+=websWrite(wp, "%-4s%-18s" WL_LOG_FMT "%-4s%-5s%-7s%-7s%-12s\n",
			   "Inf", "MAC", "PSM", "PhyMode", "BW", "SGI", "STBC", "MLO", "RSSI", "TxRate", "RxRate", "Connect Time");

#define SHOW_STA_INFO(idx,_p,_i,_st, _gr) {											\
		int hr, min, sec;											\
		_st *Entry = ((_st *)(_p)) + _i;									\
		hr = Entry->ConnectedTime/3600;										\
		min = (Entry->ConnectedTime % 3600)/60;									\
		sec = Entry->ConnectedTime - hr*3600 - min*60;						\
		ret+=websWrite(wp, "%-4d%02X:%02X:%02X:%02X:%02X:%02X " WL_LOG_FMT "%-3s %4hhd %5dM %5dM %02d:%02d:%02d\n",  		\
				idx,											\
				Entry->Addr[0], Entry->Addr[1],								\
				Entry->Addr[2], Entry->Addr[3],								\
				Entry->Addr[4], Entry->Addr[5],								\
				Entry->Psm ? "Yes" : "NO ",								\
				GetPhyMode(Entry->TxRate.field.MODE),							\
				GetBW(Entry->TxRate.field.MODE, Entry->TxRate.field.BW),								\
				Entry->TxRate.field.ShortGI ? "Yes" : "NO ",						\
				Entry->TxRate.field.STBC ? "Yes" : "NO ",						\
				Entry->EmlsrBmap?"EML":(Entry->StrBmap?"Yes":"No"),					\
				MAX3(Entry->AvgRssi0,Entry->AvgRssi1,Entry->AvgRssi2),					\
				_gr(Entry->TxRate),									\
				(Entry->LastRxRate),								\
				hr, min, sec										\
		);													\
	}
#else
#define WL_LOG_FMT "%-4s%-8s%-5s%-4s%-5s"
	ret+=websWrite(wp, "\nStations List			   \n");
	ret+=websWrite(wp, "----------------------------------------\n");
	ret+=websWrite(wp, "%-4s%-18s" WL_LOG_FMT "%-7s%-7s%-12s\n",
			   "Inf", "MAC", "PSM", "PhyMode", "BW", "SGI", "STBC", "TxRate", "RxRate", "Connect Time");

#define SHOW_STA_INFO(idx,_p,_i,_st, _gr) {											\
		int hr, min, sec;											\
		_st *Entry = ((_st *)(_p)) + _i;									\
		hr = Entry->ConnectedTime/3600;										\
		min = (Entry->ConnectedTime % 3600)/60;									\
		sec = Entry->ConnectedTime - hr*3600 - min*60;						\
		ret+=websWrite(wp, "%-4d%02X:%02X:%02X:%02X:%02X:%02X " WL_LOG_FMT "%5dM %5dM %02d:%02d:%02d\n",  		\
				idx,											\
				Entry->Addr[0], Entry->Addr[1],								\
				Entry->Addr[2], Entry->Addr[3],								\
				Entry->Addr[4], Entry->Addr[5],								\
				Entry->Psm ? "Yes" : "NO ",								\
				GetPhyMode(Entry->TxRate.field.MODE),							\
				GetBW(Entry->TxRate.field.MODE, Entry->TxRate.field.BW),								\
				Entry->TxRate.field.ShortGI ? "Yes" : "NO ",						\
				Entry->TxRate.field.STBC ? "Yes" : "NO ",						\
				_gr(Entry->TxRate),									\
				(Entry->LastRxRate),								\
				hr, min, sec										\
		);													\
	}
#endif	/* !MLO */

	if (!strcmp(ifname, WIF_2G)) {
	    int idx;
	    for(idx = 0; idx <= num_of_mssid_support(unit); idx++) {
		if (idx) {
			if (get_ra_sta_info_by_unit(unit, idx, data, sizeof(data)) <= 0)
				continue;
		}
		for (i=0;i<mp2->Num;i++) {
			SHOW_STA_INFO(idx, mp2->Entry, i, RT_802_11_MAC_ENTRY_for_2G, getRate_2g);
		}
	    }
	}
#if defined(RTCONFIG_HAS_5G)
	else {
	    int idx;
	    for(idx = 0; idx <= num_of_mssid_support(unit); idx++) {
		if (idx) {
			if (get_ra_sta_info_by_unit(unit, idx, data, sizeof(data)) <= 0)
				continue;
		}
		for (i=0;i<mp->Num;i++) {
			SHOW_STA_INFO(idx, mp->Entry, i, RT_802_11_MAC_ENTRY_for_5G, getRate);
		}
	    }
	}
#endif	/* RTCONFIG_HAS_5G */

	return ret;
}

#ifdef RTCONFIG_NL80211
typedef struct _WPS_CONFIGURED_VALUE {
	unsigned short 	Configured;	// 1:un-configured/2:configured
	char		BSSID[18];
	char 		SSID[32 + 1];
	char		AuthMode[16];	// Open System/Shared Key/WPA-Personal/WPA2-Personal/WPA-Enterprise/WPA2-Enterprise
	char 		Encryp[8];	// None/WEP/TKIP/AES
	char 		DefaultKeyIdx;
	char 		WPAKey[64 + 1];
} WPS_CONFIGURED_VALUE;

static void getWPSConfig(int unit, WPS_CONFIGURED_VALUE *result)
{
	char buf[128], wif[32] = { 0 };
	char *wlxy = NULL;
	FILE *fp;

	memset(result, 0, sizeof(*result));

#if defined(RTCONFIG_MULTILAN_MWL)
	wlxy = get_sdntype_iface("MAINFH", unit, wif, sizeof(wif));
	if (wlxy)
		free(wlxy);
	if (!wlxy || !strlen(wif))
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

char *getWscStatus(int unit)
{
	static char buf[256];
	char wif[8] = { 0 };
	FILE *fp;
	int len;
	char *pt1,*pt2;
	char *wlxy = NULL;

#if defined(RTCONFIG_MULTILAN_MWL)
	wlxy = get_sdntype_iface("MAINFH", unit, wif, sizeof(wif));
	if (wlxy)
		free(wlxy);
	if (!wlxy || !strlen(wif))
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
			buf[len - 1] = '\0';
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
	static char buffer[16];

	snprintf(buffer, sizeof(buffer), "%s", nvram_safe_get("secret_code"));

	return buffer;
}

#else /* !RTCONFIG_NL80211 */
typedef struct PACKED _WSC_CONFIGURED_VALUE {
    unsigned short WscConfigured;	// 1 un-configured; 2 configured
    unsigned char WscSsid[32 + 1];
    unsigned short WscAuthMode;		// mandatory, 0x01: open, 0x02: wpa-psk, 0x04: shared, 0x08:wpa, 0x10: wpa2, 0x
    unsigned short WscEncrypType;	// 0x01: none, 0x02: wep, 0x04: tkip, 0x08: aes
    unsigned char DefaultKeyIdx;
    unsigned char WscWPAKey[64 + 1];
} WSC_CONFIGURED_VALUE;

void getWPSAuthMode(WSC_CONFIGURED_VALUE *result, char *ret_str, int len)
{
	if (result->WscAuthMode & 0x1)
		strlcat(ret_str, "Open System", len);
	if (result->WscAuthMode & 0x2)
		strlcat(ret_str, "WPA-Personal", len);
	if (result->WscAuthMode & 0x4)
		strlcat(ret_str, "Shared Key", len);
	if (result->WscAuthMode & 0x8)
		strlcat(ret_str, "WPA-Enterprise", len);
	if (result->WscAuthMode & 0x10)
		strlcat(ret_str, "WPA2-Enterprise", len);
	if (result->WscAuthMode & 0x20)
		strlcat(ret_str, "WPA2-Personal", len);
}

void getWPSEncrypType(WSC_CONFIGURED_VALUE *result, char *ret_str, int len)
{
	if (result->WscEncrypType & 0x1)
		strlcat(ret_str, "None", len);
	if (result->WscEncrypType & 0x2)
		strlcat(ret_str, "WEP", len);
	if (result->WscEncrypType & 0x4)
		strlcat(ret_str, "TKIP", len);
	if (result->WscEncrypType & 0x8)
		strlcat(ret_str, "AES", len);
}

/*
 * these definitions are from rt2860v2 driver include/wsc.h
 */
char *getWscStatusStr(int status)
{
	switch(status) {
	case 0:
		return "Not used";
	case 1:
		return "Idle";
	case 2:
#if 0
		return "WPS Fail(Ignore this if Intel/Marvell registrar used)";
#else
		return "Idle";
#endif
	case 3:
		return "Start WPS Process";
	case 4:
		return "Received EAPOL-Start";
	case 5:
		return "Sending EAP-Req(ID)";
	case 6:
		return "Receive EAP-Rsp(ID)";
	case 7:
		return "Receive EAP-Req with wrong WPS SMI Vendor Id";
	case 8:
		return "Receive EAP-Req with wrong WPS Vendor Type";
	case 9:
		return "Sending EAP-Req(WPS_START)";
	case 10:
		return "Send M1";
	case 11:
		return "Received M1";
	case 12:
		return "Send M2";
	case 13:
		return "Received M2";
	case 14:
		return "Received M2D";
	case 15:
		return "Send M3";
	case 16:
		return "Received M3";
	case 17:
		return "Send M4";
	case 18:
		return "Received M4";
	case 19:
		return "Send M5";
	case 20:
		return "Received M5";
	case 21:
		return "Send M6";
	case 22:
		return "Received M6";
	case 23:
		return "Send M7";
	case 24:
		return "Received M7";
	case 25:
		return "Send M8";
	case 26:
		return "Received M8";
	case 27:
		return "Processing EAP Response (ACK)";
	case 28:
		return "Processing EAP Request (Done)";
	case 29:
		return "Processing EAP Response (Done)";
	case 30:
		return "Sending EAP-Fail";
	case 31:
		return "WPS_ERROR_HASH_FAIL";
	case 32:
		return "WPS_ERROR_HMAC_FAIL";
	case 33:
		return "WPS_ERROR_DEV_PWD_AUTH_FAIL";
	case 34:
//		return "Configured";
		return "Success";
	case 35:
		return "SCAN AP";
	case 36:
		return "EAPOL START SENT";
	case 37:
		return "WPS_EAP_RSP_DONE_SENT";
	case 38:
		return "WAIT PINCODE";
	case 39:
		return "WSC_START_ASSOC";
	case 0x101:
		return "PBC:TOO MANY AP";
	case 0x102:
		return "PBC:NO AP";
	case 0x103:
		return "EAP_FAIL_RECEIVED";
	case 0x104:
		return "EAP_NONCE_MISMATCH";
	case 0x105:
		return "EAP_INVALID_DATA";
	case 0x106:
		return "PASSWORD_MISMATCH";
	case 0x107:
		return "EAP_REQ_WRONG_SMI";
	case 0x108:
		return "EAP_REQ_WRONG_VENDOR_TYPE";
	case 0x109:
		return "PBC_SESSION_OVERLAP";
	default:
		return "Unknown";
	}
}

int getWscStatus(int unit)
{
	int data = 0;
	struct iwreq wrq;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_";

	snprintf(prefix, sizeof(prefix), "wl%d_", unit);

	wrq.u.data.length = sizeof(data);
	wrq.u.data.pointer = (caddr_t) &data;
	wrq.u.data.flags = RT_OID_WSC_QUERY_STATUS;

	if (wl_ioctl(nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp))), RT_PRIV_IOCTL, &wrq) < 0)
		fprintf(stderr, "errors in getting WSC status\n");

	return data;
}

unsigned int getAPPIN(int unit)
{
	unsigned int data = 0;
	struct iwreq wrq;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_";

	snprintf(prefix, sizeof(prefix), "wl%d_", unit);

	wrq.u.data.length = sizeof(data);
	wrq.u.data.pointer = (caddr_t) &data;
	wrq.u.data.flags = RT_OID_WSC_PIN_CODE;

	if (wl_ioctl(nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp))), RT_PRIV_IOCTL, &wrq) < 0)
		fprintf(stderr, "errors in getting AP PIN\n");

	return data;
}
#endif /* RTCONFIG_NL80211 */

int
wl_wps_info(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
	int /*i,*/ j = -1, u = unit;
	char tmpstr[128]; //, tmpstr2[256];
#ifdef RTCONFIG_NL80211
	WPS_CONFIGURED_VALUE result;
#else
	WSC_CONFIGURED_VALUE result;
	struct iwreq wrq;
#endif
	int retval = 0;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_";
	char *wps_sta_pin;
	char tag1[] = "<wps_infoXXXXXX>", tag2[] = "</wps_infoXXXXXX>";

#if defined(RTCONFIG_WPSMULTIBAND)
	for (j = -1; j < MAX_NR_WL_IF; ++j) {
		SKIP_ABSENT_BAND(unit);
#endif
		switch (j) {
		case WL_2G_BAND:	/* fall through */
		case WL_5G_BAND:	/* fall through */
#if !(defined(RTCONFIG_WIFI6E) || (defined(RTCONFIG_WIFI7) && !defined(RTCONFIG_WIFI7_NO_6G)))
		case WL_5G_2_BAND:	/* fall through */
#else
		case WL_6G_BAND:	/* fall through */
#endif
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
		if (!nvram_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp))))
			continue;
#endif

#ifdef RTCONFIG_NL80211
		memset(&result, 0, sizeof(result));
		getWPSConfig(u, &result);
#else
		wrq.u.data.length = sizeof(WSC_CONFIGURED_VALUE);
		wrq.u.data.pointer = (caddr_t) &result;
		wrq.u.data.flags = 0;
		strlcpy((char *)&result, "get_wsc_profile", sizeof(result));	/* FIXME */
		if (wl_ioctl(nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp))), RTPRIV_IOCTL_WSC_PROFILE, &wrq) < 0) {
			fprintf(stderr, "errors in getting WSC profile\n");
			return 0;
		}
#endif

		if (j == -1)
			retval += websWrite(wp, "<wps>\n");

		//0. WSC Status
		memset(tmpstr, 0, sizeof(tmpstr));
#ifdef RTCONFIG_NL80211
		strlcpy(tmpstr, getWscStatus(u), sizeof(tmpstr));
#else
		strlcpy(tmpstr, getWscStatusStr(getWscStatus(u)), sizeof(tmpstr));
#endif
		retval += websWrite(wp, "%s%s%s\n", tag1, tmpstr, tag2);

		//1. WPSConfigured
#ifdef RTCONFIG_NL80211
		if (result.Configured == 2)
#else
		if (result.WscConfigured == 2)
#endif
			retval += websWrite(wp, "%s%s%s\n", tag1, "Yes", tag2);
		else
			retval += websWrite(wp, "%s%s%s\n", tag1, "No", tag2);

		//2. WPSSSID
		memset(tmpstr, 0, sizeof(tmpstr));
#ifdef RTCONFIG_NL80211
		char_to_ascii(tmpstr, result.SSID);
#else
		char_to_ascii(tmpstr, result.WscSsid);
#endif
		retval += websWrite(wp, "%s%s%s\n", tag1, tmpstr, tag2);

		//3. WPSAuthMode
		memset(tmpstr, 0, sizeof(tmpstr));
#ifdef RTCONFIG_NL80211
		strlcpy(tmpstr, result.AuthMode, sizeof(tmpstr));
#else
		getWPSAuthMode(&result, tmpstr, sizeof(tmpstr));
#endif
		retval += websWrite(wp, "%s%s%s\n", tag1, tmpstr, tag2);

		//4. EncrypType
		memset(tmpstr, 0, sizeof(tmpstr));
#ifdef RTCONFIG_NL80211
		strlcpy(tmpstr, result.Encryp, sizeof(tmpstr));
#else
		getWPSEncrypType(&result, tmpstr, sizeof(tmpstr));
#endif
		retval += websWrite(wp, "%s%s%s\n", tag1, tmpstr, tag2);

		//5. DefaultKeyIdx
		memset(tmpstr, 0, sizeof(tmpstr));
		snprintf(tmpstr, sizeof(tmpstr), "%d", result.DefaultKeyIdx);
		retval += websWrite(wp, "%s%s%s\n", tag1, tmpstr, tag2);

		//6. WPAKey
#if 0	//hide for security
		memset(tmpstr, 0, sizeof(tmpstr));
		for (i=0; i<64; i++)	// WPA key default length is 64 (defined & hardcode in driver)
		{
			snprintf(tmpstr, sizeof(tmpstr), "%s%c", tmpstr, result.WscWPAKey[i]);
		}
		if (!safe_strlen(tmpstr))
			retval += websWrite(wp, "%sNone%s\n", tag1, tag2);
		else
		{
			char_to_ascii(tmpstr2, tmpstr);
			retval += websWrite(wp, "%s%s%s\n", tag1, tmpstr2, tag2);
		}
#else
		retval += websWrite(wp, "%s%s\n", tag1, tag2);
#endif
		//7. AP PIN Code
		memset(tmpstr, 0, sizeof(tmpstr));
#ifdef RTCONFIG_NL80211
		strlcpy(tmpstr, getAPPIN(u), sizeof(tmpstr));
#else
		snprintf(tmpstr, sizeof(tmpstr), "%08d", getAPPIN(u));
#endif
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

#if defined(RTCONFIG_WIFI7)
int
ej_wps_info_6g(int eid, webs_t wp, int argc, char_t **argv)
{
	// TBD.
	return wl_wps_info(eid, wp, argc, argv, WL_5G_BAND);
}

int
ej_wl_rate_6g_2(int eid, webs_t wp, int argc, char_t **argv)
{
	// TBD.
	return websWrite(wp, "");
}

int
ej_wps_info_6g_2(int eid, webs_t wp, int argc, char_t **argv)
{
	// TBD.
	return wl_wps_info(eid, wp, argc, argv, WL_5G_BAND);
}
#endif

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
ej_wps_info_5g(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_wps_info(eid, wp, argc, argv, WL_5G_BAND);
}

int
ej_wps_info_5g_2(int eid, webs_t wp, int argc, char_t **argv)
{
	return wl_wps_info(eid, wp, argc, argv, WL_5G_2_BAND);
}


// Wireless Client List		 /* Start --Alicia, 08.09.23 */

int ej_wl_sta_list_2g(int eid, webs_t wp, int argc, char_t **argv)
{
	struct iwreq wrq;
	int i, firstRow;
#if defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
	char data[sizeof(RT_802_11_MAC_TABLE_5G)]; // 2G, 5G use same structure
#else
	char data[16384];
#endif
	char mac[ETHER_ADDR_STR_LEN];
	RT_802_11_MAC_TABLE_2G *mp2;
	char *value;
	int rssi, cnt, xTxR;
	int from_app = 0;

	from_app = check_user_agent(user_agent);
	xTxR = nvram_get_int("wl0_HT_RxStream");

	memset(mac, 0, sizeof(mac));

	if(hook_get_json == 1)
		websWrite(wp, "{");

#if defined(RTAC85U) || defined(RTAC85P) || defined(RTACRH26) || defined(TUFAC1750)
	if (!nvram_get_int("wlready"))
		goto exit;
#endif

	/* query wl for authenticated sta list */
	memset(data, 0, sizeof(data));
	wrq.u.data.pointer = data;
	wrq.u.data.length = sizeof(data);
	wrq.u.data.flags = 0;
	if (wl_ioctl(WIF_2G, RTPRIV_IOCTL_GET_MAC_TABLE, &wrq) < 0)
		goto exit;

	/* build wireless sta list */
	firstRow = 1;
	mp2 = (RT_802_11_MAC_TABLE_2G *)wrq.u.data.pointer;
	for (i = 0; i<mp2->Num; i++)
	{
		rssi = cnt = 0;
		if (mp2->Entry[i].AvgRssi0 && cnt < xTxR) {
			rssi += mp2->Entry[i].AvgRssi0;
			cnt++;
		}
		if (mp2->Entry[i].AvgRssi1 && cnt < xTxR) {
			rssi += mp2->Entry[i].AvgRssi1;
			cnt++;
		}
		if (mp2->Entry[i].AvgRssi2 && cnt < xTxR) {
			rssi += mp2->Entry[i].AvgRssi2;
			cnt++;
		}
		if (cnt == 0)
			continue;	//skip this sta info

		if (firstRow == 1)
			firstRow = 0;
		else
			websWrite(wp, ", ");

		if (from_app == 0 && hook_get_json == 0)
			websWrite(wp, "[");

		snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
				mp2->Entry[i].Addr[0], mp2->Entry[i].Addr[1],
				mp2->Entry[i].Addr[2], mp2->Entry[i].Addr[3],
				mp2->Entry[i].Addr[4], mp2->Entry[i].Addr[5]);
		websWrite(wp, "\"%s\"", mac);

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

		if (from_app != 0 || hook_get_json == 1)
			websWrite(wp, ",\"rssi\":");

		rssi = rssi / cnt;
		if (from_app == 0 && hook_get_json == 0)
			websWrite(wp, ", \"%d\"", rssi);
		else
			websWrite(wp, "\"%d\"", rssi);


		if (from_app == 0 && hook_get_json == 0)
			websWrite(wp, "]");
		else
			websWrite(wp, "}");
	}

	/* error/exit */
exit:
	if(hook_get_json == 1)
		websWrite(wp, "}");
	return 0;
}

int ej_wl_sta_list_5g(int eid, webs_t wp, int argc, char_t **argv)
{
#if defined(RTCONFIG_HAS_5G)
	struct iwreq wrq;
	int i, firstRow;
#if defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
	char data[sizeof(RT_802_11_MAC_TABLE_5G)]; // 2G, 5G use same structure
#else
	char data[16384];
#endif
	char mac[ETHER_ADDR_STR_LEN];
	RT_802_11_MAC_TABLE_5G *mp;
	char *value;
	int rssi, cnt, xTxR;
	int from_app = 0;

	from_app = check_user_agent(user_agent);
	xTxR = nvram_get_int("wl1_HT_RxStream");

	memset(mac, 0, sizeof(mac));

	if(hook_get_json == 1)
		websWrite(wp, "{");

#if defined(RTAC85U) || defined(RTAC85P) || defined(RTACRH26) || defined(TUFAC1750)
	if (!nvram_get_int("wlready"))
		goto exit;
#endif

	/* query wl for authenticated sta list */
	memset(data, 0, sizeof(data));
	wrq.u.data.pointer = data;
	wrq.u.data.length = sizeof(data);
	wrq.u.data.flags = 0;
	if (wl_ioctl(WIF_5G, RTPRIV_IOCTL_GET_MAC_TABLE, &wrq) < 0)
		goto exit;

	/* build wireless sta list */
	firstRow = 1;
	mp = (RT_802_11_MAC_TABLE_5G *)wrq.u.data.pointer;
	for (i = 0; i<mp->Num; i++)
	{
		rssi = cnt = 0;
		if (mp->Entry[i].AvgRssi0 && cnt < xTxR) {
			rssi += mp->Entry[i].AvgRssi0;
			cnt++;
		}
		if (mp->Entry[i].AvgRssi1 && cnt < xTxR) {
			rssi += mp->Entry[i].AvgRssi1;
			cnt++;
		}
		if (mp->Entry[i].AvgRssi2 && cnt < xTxR) {
			rssi += mp->Entry[i].AvgRssi2;
			cnt++;
		}
		if (cnt == 0)
			continue;	//skip this sta info

		if (firstRow == 1)
			firstRow = 0;
		else
			websWrite(wp, ", ");

		if (from_app == 0 && hook_get_json == 0)
			websWrite(wp, "[");

		snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
				mp->Entry[i].Addr[0], mp->Entry[i].Addr[1],
				mp->Entry[i].Addr[2], mp->Entry[i].Addr[3],
				mp->Entry[i].Addr[4], mp->Entry[i].Addr[5]);
		websWrite(wp, "\"%s\"", mac);

		if (from_app != 0) {
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

		if (from_app != 0 || hook_get_json == 1)
			websWrite(wp, ",\"rssi\":");

		rssi = rssi / cnt;
		if (from_app == 0 && hook_get_json == 0)
			websWrite(wp, ", \"%d\"", rssi);
		else
			websWrite(wp, "\"%d\"", rssi);


		if (from_app == 0 && hook_get_json == 0)
			websWrite(wp, "]");
		else
			websWrite(wp, "}");
	}

	/* error/exit */
#endif	/* RTCONFIG_HAS_5G */
exit:
	if(hook_get_json == 1)
		websWrite(wp, "}");
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
	dbg("%s: unid %d, FIXME\n", __func__, unit);
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
		char tmp[128], prefix[] = "wlXXXXXXXXXX_";
		struct iwreq wrq;
		int i, j, s, firstRow;
#if defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
		char data[sizeof(RT_802_11_MAC_TABLE_5G)]; // 2G, 5G use same structure
#else
		char data[16384];
#endif
		char mac[ETHER_ADDR_STR_LEN];
		RT_802_11_MAC_TABLE_5G *mp;
		RT_802_11_MAC_TABLE_2G *mp2;
		char *name;
		int rssi, cnt, xTxR;
		char alias[16];

		SKIP_ABSENT_BAND_AND_INC_UNIT(unit);
		snprintf(prefix, sizeof(prefix), "wl%d_", unit);
		name = nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp)));
		xTxR = nvram_get_int(strlcat_r(prefix, "HT_RxStream", tmp, sizeof(tmp)));

		/* query wl for authenticated sta list */
		memset(data, 0, sizeof(data));
		wrq.u.data.pointer = data;
		wrq.u.data.length = sizeof(data);
		wrq.u.data.flags = 0;
		if (wl_ioctl(name, RTPRIV_IOCTL_GET_MAC_TABLE, &wrq) < 0)
			goto exit;

		/* build wireless sta list */
		if (unit == 0) {
			mp2 = (RT_802_11_MAC_TABLE_2G *)wrq.u.data.pointer;

			for (i = 0; i < MAX_NO_MSSID; i++) {
				firstRow = 1;
				memset(alias, 0, sizeof(alias));

				if (i == 0)
					snprintf(alias, sizeof(alias), "%s", unit ? (unit == 2 ? "5G1" : "5G") : "2G");
				else
					snprintf(alias, sizeof(alias), "%s_%d", unit ? (unit == 2 ? "5G1" : "5G") : "2G", i);

				for (j = 0; j<mp2->Num; j++) {
					s = mp2->Entry[j].ApIdx;
					if (i != s)
						continue;

					rssi = cnt = 0;
					if (mp2->Entry[j].AvgRssi0 && cnt < xTxR) {
						rssi += mp2->Entry[j].AvgRssi0;
						cnt++;
					}
					if (mp2->Entry[j].AvgRssi1 && cnt < xTxR) {
						rssi += mp2->Entry[j].AvgRssi1;
						cnt++;
					}
					if (mp2->Entry[j].AvgRssi2 && cnt < xTxR) {
						rssi += mp2->Entry[j].AvgRssi2;
						cnt++;
					}
					if (cnt == 0)
						continue;	//skip this sta info

					if (firstRow == 1) {
						if (haveInfo)
							websWrite(wp, ",");
						websWrite(wp, "\"%s\":[", alias);
						firstRow = 0;
						haveInfo = 1;
					}
					else
						websWrite(wp, ",");

					memset(mac, 0, sizeof(mac));
					snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
							mp2->Entry[j].Addr[0], mp2->Entry[j].Addr[1],
							mp2->Entry[j].Addr[2], mp2->Entry[j].Addr[3],
							mp2->Entry[j].Addr[4], mp2->Entry[j].Addr[5]);
					rssi = rssi / cnt;
					websWrite(wp, "{\"mac\":\"%s\",\"rssi\":%d}", mac, rssi);
				}

				if (!firstRow)
					websWrite(wp, "]");
			}
		}
		else
		{
			mp = (RT_802_11_MAC_TABLE_5G *)wrq.u.data.pointer;

			for (i = 0; i < MAX_NO_MSSID; i++) {
				firstRow = 1;
				memset(alias, 0, sizeof(alias));

				if (i == 0)
					snprintf(alias, sizeof(alias), "%s", unit ? (unit == 2 ? "5G1" : "5G") : "2G");
				else
					snprintf(alias, sizeof(alias), "%s_%d", unit ? (unit == 2 ? "5G1" : "5G") : "2G", i);

				for (j = 0; j<mp->Num; j++) {
					s = mp->Entry[j].ApIdx;
					if (i != s)
						continue;

					rssi = cnt = 0;
					if (mp->Entry[j].AvgRssi0 && cnt < xTxR) {
						rssi += mp->Entry[j].AvgRssi0;
						cnt++;
					}
					if (mp->Entry[j].AvgRssi1 && cnt < xTxR) {
						rssi += mp->Entry[j].AvgRssi1;
						cnt++;
					}
					if (mp->Entry[j].AvgRssi2 && cnt < xTxR) {
						rssi += mp->Entry[j].AvgRssi2;
						cnt++;
					}
					if (cnt == 0)
						continue;	//skip this sta info

					if (firstRow == 1) {
						if (haveInfo)
							websWrite(wp, ",");
						websWrite(wp, "\"%s\":[", alias);
						firstRow = 0;
						haveInfo = 1;
					}
					else
						websWrite(wp, ",");

					memset(mac, 0, sizeof(mac));
					snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
							mp->Entry[j].Addr[0], mp->Entry[j].Addr[1],
							mp->Entry[j].Addr[2], mp->Entry[j].Addr[3],
							mp->Entry[j].Addr[4], mp->Entry[j].Addr[5]);
					rssi = rssi / cnt;
					websWrite(wp, "{\"mac\":\"%s\",\"rssi\":%d}", mac, rssi);
				}

				if (!firstRow)
					websWrite(wp, "]");
			}
		}

		unit++;
	}

	/* error/exit */
exit:

	websWrite(wp, "}");

	return 0;
}

int ej_wl_auth_list(int eid, webs_t wp, int argc, char_t **argv)
{
	struct iwreq wrq;
	int i, firstRow;
#if defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
	char data[sizeof(RT_802_11_MAC_TABLE_5G)]; // 2G, 5G use same structure
#else
	char data[16384];
#endif
	char mac[ETHER_ADDR_STR_LEN];
	RT_802_11_MAC_TABLE_5G *mp;
	RT_802_11_MAC_TABLE_2G *mp2;
	char *value;

	memset(mac, 0, sizeof(mac));

	/* query wl for authenticated sta list */
	memset(data, 0, sizeof(data));
	wrq.u.data.pointer = data;
	wrq.u.data.length = sizeof(data);
	wrq.u.data.flags = 0;

	if(hook_get_json == 1)
		websWrite(wp, "[");

	if (wl_ioctl(WIF_2G, RTPRIV_IOCTL_GET_MAC_TABLE, &wrq) < 0)
		goto exit;

	/* build wireless sta list */
	firstRow = 1;
	mp2 = (RT_802_11_MAC_TABLE_2G *)wrq.u.data.pointer;
	for (i = 0; i<mp2->Num; i++)
	{
		if (firstRow == 1)
			firstRow = 0;
		else
			websWrite(wp, ", ");
		websWrite(wp, "[");

		snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
				mp2->Entry[i].Addr[0], mp2->Entry[i].Addr[1],
				mp2->Entry[i].Addr[2], mp2->Entry[i].Addr[3],
				mp2->Entry[i].Addr[4], mp2->Entry[i].Addr[5]);
		websWrite(wp, "\"%s\"", mac);

		value = "Yes";
		websWrite(wp, ", \"%s\"", value);

		value = "";
		websWrite(wp, ", \"%s\"", value);

		websWrite(wp, "]");
	}

#if defined(RTCONFIG_HAS_5G)
	/* query wl for authenticated sta list */
	memset(data, 0, sizeof(data));
	wrq.u.data.pointer = data;
	wrq.u.data.length = sizeof(data);
	wrq.u.data.flags = 0;
	if (wl_ioctl(WIF_5G, RTPRIV_IOCTL_GET_MAC_TABLE, &wrq) < 0)
		goto exit;

	/* build wireless sta list */
	mp = (RT_802_11_MAC_TABLE_5G *)wrq.u.data.pointer;
	for (i = 0; i<mp->Num; i++)
	{
		if (firstRow == 1)
			firstRow = 0;
		else
			websWrite(wp, ", ");
		websWrite(wp, "[");

		snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
				mp->Entry[i].Addr[0], mp->Entry[i].Addr[1],
				mp->Entry[i].Addr[2], mp->Entry[i].Addr[3],
				mp->Entry[i].Addr[4], mp->Entry[i].Addr[5]);
		websWrite(wp, "\"%s\"", mac);

		value = "Yes";
		websWrite(wp, ", \"%s\"", value);

		value = "";
		websWrite(wp, ", \"%s\"", value);

		websWrite(wp, "]");
	}
#endif	/* RTCONFIG_HAS_5G */

#if defined(RTCONFIG_HAS_6G)
	/* query wl for authenticated sta list */
	memset(data, 0, sizeof(data));
	wrq.u.data.pointer = data;
	wrq.u.data.length = sizeof(data);
	wrq.u.data.flags = 0;
	if (wl_ioctl(WIF_6G, RTPRIV_IOCTL_GET_MAC_TABLE, &wrq) < 0)
		goto exit;

	/* build wireless sta list */
	mp = (RT_802_11_MAC_TABLE_5G *)wrq.u.data.pointer;
	for (i = 0; i<mp->Num; i++)
	{
		if (firstRow == 1)
			firstRow = 0;
		else
			websWrite(wp, ", ");
		websWrite(wp, "[");

		snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
				mp->Entry[i].Addr[0], mp->Entry[i].Addr[1],
				mp->Entry[i].Addr[2], mp->Entry[i].Addr[3],
				mp->Entry[i].Addr[4], mp->Entry[i].Addr[5]);
		websWrite(wp, "\"%s\"", mac);

		value = "Yes";
		websWrite(wp, ", \"%s\"", value);

		value = "";
		websWrite(wp, ", \"%s\"", value);

		websWrite(wp, "]");
	}
#endif	/* RTCONFIG_HAS_5G */


	/* error/exit */
exit:
	if(hook_get_json == 1)
		websWrite(wp, "]");
	return 0;
}
#if defined(RTN65U)
static void convertToUpper(char *str)
{
	if(str == NULL)
		return;
	while(*str)
	{
		if(*str >= 'a' && *str <= 'z')
		{
			*str &= (unsigned char)~0x20;
		}
		str++;
	}
}
#endif
static int wl_scan(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
	int retval = 0, i = 0, apCount = 0;
	char data[8192];
	char ssid_str[256];
	char header[128];
	struct iwreq wrq;
	SSA *ssap;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_";
	int lock;
#if defined(RTCONFIG_MTK_BSD)
	int restart_bs20 = 0;
#endif

	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	memset(data, 0x00, 255);
	strlcpy(data, "SiteSurvey=1", sizeof(data));
	wrq.u.data.length = safe_strlen(data)+1;
	wrq.u.data.pointer = data;
	wrq.u.data.flags = 0;
#if defined(RTCONFIG_MTK_BSD)
	if (nvram_match("smart_connect_x", "1") && pids("bs20")) {
		eval("rc", "rc_service", "stop_mtk_bs20");
		restart_bs20 = 1;
	}
#endif
	lock = file_lock("nvramcommit");
	if (wl_ioctl(nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp))), RTPRIV_IOCTL_SET, &wrq) < 0)
	{
		file_unlock(lock);
		dbg("Site Survey fails\n");
		return 0;
	}
	file_unlock(lock);
	
	dbg("Please wait");
	sleep(1);
	dbg(".");
	sleep(1);
	dbg(".");
	sleep(1);
	dbg(".");
	sleep(1);
	dbg(".\n\n");
	memset(data, 0, 8192);
	strlcpy(data, "", sizeof(data));
	wrq.u.data.length = 8192;
	wrq.u.data.pointer = data;
	wrq.u.data.flags = 0;
	if (wl_ioctl(nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp))), RTPRIV_IOCTL_GSITESURVEY, &wrq) < 0)
	{
		dbg("errors in getting site survey result\n");
		return 0;
	}
#if defined(RTCONFIG_MTK_BSD)
	if (restart_bs20) {
		eval("rc", "rc_service", "start_mtk_bs20");
	}
#endif
	memset(header, 0, sizeof(header));
	//snprintf(header, sizeof(header), "%-3s%-33s%-18s%-8s%-15s%-9s%-8s%-2s\n", "Ch", "SSID", "BSSID", "Enc", "Auth", "Siganl(%)", "W-Mode", "NT");
#if 0// defined(RTN14U)
	snprintf(header, sizeof(header), "%-4s%-33s%-18s%-9s%-16s%-9s%-8s%-4s%-5s\n", "Ch", "SSID", "BSSID", "Enc", "Auth", "Siganl(%)", "W-Mode"," WPS", " DPID");
#else
	snprintf(header, sizeof(header), "%-4s%-33s%-18s%-9s%-16s%-9s%-8s\n", "Ch", "SSID", "BSSID", "Enc", "Auth", "Siganl(%)", "W-Mode");
#endif
	dbg("\n%s", header);
	if (wrq.u.data.length > 0)
	{
#if defined(RTN65U)
		if (unit == 0 && get_model() == MODEL_RTN65U)
		{
			char *encryption;
			SITE_SURVEY_RT3352_iNIC *pSsap, *ssAP;

			pSsap = ssAP = (SITE_SURVEY_RT3352_iNIC *) (1 /* '\n' */ + wrq.u.data.pointer +  sizeof(SITE_SURVEY_RT3352_iNIC) /* header */);
			while(((unsigned int)wrq.u.data.pointer + wrq.u.data.length) > (unsigned int) ssAP)
			{
				ssAP->channel   [sizeof(ssAP->channel)    -1] = '\0';
				ssAP->ssid      [32                         ] = '\0';
				ssAP->bssid     [17                         ] = '\0';
				ssAP->encryption[sizeof(ssAP->encryption) -1] = '\0';
				if((encryption = strchr(ssAP->authmode, '/')) != NULL)
				{
					memmove(ssAP->encryption, encryption +1, sizeof(ssAP->encryption) -1);
					memset(encryption, ' ', sizeof(ssAP->authmode) - (encryption - ssAP->authmode));
					*encryption = '\0';
				}
				ssAP->authmode  [sizeof(ssAP->authmode)   -1] = '\0';
				ssAP->signal    [sizeof(ssAP->signal)     -1] = '\0';
				ssAP->wmode     [sizeof(ssAP->wmode)      -1] = '\0';
				ssAP->extch     [sizeof(ssAP->extch)      -1] = '\0';
				ssAP->nt        [sizeof(ssAP->nt)         -1] = '\0';
				ssAP->wps       [sizeof(ssAP->wps)        -1] = '\0';
				ssAP->dpid      [sizeof(ssAP->dpid)       -1] = '\0';

				convertToUpper(ssAP->bssid);
				ssAP++;
				apCount++;
			}

			if (apCount)
			{
				retval += websWrite(wp, "[");
				for (i = 0; i < apCount; i++)
				{
					dbg("%-4s%-33s%-18s%-9s%-16s%-9s%-8s\n",
						pSsap[i].channel,
						pSsap[i].ssid,
						pSsap[i].bssid,
						pSsap[i].encryption,
						pSsap[i].authmode,
						pSsap[i].signal,
						pSsap[i].wmode
					);

					memset(ssid_str, 0, sizeof(ssid_str));
					char_to_ascii(ssid_str, trim_r(pSsap[i].ssid));

					if (!i)
						retval += websWrite(wp, "[\"%s\", \"%s\"]", ssid_str, pSsap[i].bssid);
					else
						retval += websWrite(wp, ", [\"%s\", \"%s\"]", ssid_str, pSsap[i].bssid);
				}
				retval += websWrite(wp, "]");
				dbg("\n");
			}
			else
				retval += websWrite(wp, "[]");
			return retval;
		}
#endif
		ssap=(SSA *)(wrq.u.data.pointer+safe_strlen(header)+1);
		int len = safe_strlen(wrq.u.data.pointer+safe_strlen(header))-1;
		char *sp, *op;
 		op = sp = wrq.u.data.pointer+safe_strlen(header)+1;
		while (*sp && ((len - (sp-op)) >= 0))
		{
			ssap->SiteSurvey[i].channel[3] = '\0';
			ssap->SiteSurvey[i].ssid[32] = '\0';
			ssap->SiteSurvey[i].bssid[17] = '\0';
			ssap->SiteSurvey[i].encryption[8] = '\0';
			ssap->SiteSurvey[i].authmode[15] = '\0';
			ssap->SiteSurvey[i].signal[8] = '\0';
			ssap->SiteSurvey[i].wmode[7] = '\0';
#if 0//defined(RTN14U)
			ssap->SiteSurvey[i].wps[3] = '\0';
			ssap->SiteSurvey[i].dpid[4] = '\0';
#endif
			sp+=safe_strlen(header);
			apCount=++i;
		}
		if (apCount)
		{
			retval += websWrite(wp, "[");
			for (i = 0; i < apCount; i++)
			{
			   	dbg("\napCount=%d\n",i);
				dbg(
#if 0//defined(RTN14U)
				"%-4s%-33s%-18s%-9s%-16s%-9s%-8s%-4s%-5s\n",
#else
				"%-4s%-33s%-18s%-9s%-16s%-9s%-8s\n",
#endif
					ssap->SiteSurvey[i].channel,
					(char*)ssap->SiteSurvey[i].ssid,
					ssap->SiteSurvey[i].bssid,
					ssap->SiteSurvey[i].encryption,
					ssap->SiteSurvey[i].authmode,
					ssap->SiteSurvey[i].signal,
					ssap->SiteSurvey[i].wmode
#if 0//defined(RTN14U)
					, ssap->SiteSurvey[i].wps
					, ssap->SiteSurvey[i].dpid
#endif
				);

				memset(ssid_str, 0, sizeof(ssid_str));
				char_to_ascii(ssid_str, trim_r(ssap->SiteSurvey[i].ssid));

				if (!i)
//					retval += websWrite(wp, "\"%s\"", ssap->SiteSurvey[i].bssid);
					retval += websWrite(wp, "[\"%s\", \"%s\"]", ssid_str, ssap->SiteSurvey[i].bssid);
				else
//					retval += websWrite(wp, ", \"%s\"", ssap->SiteSurvey[i].bssid);
					retval += websWrite(wp, ", [\"%s\", \"%s\"]", ssid_str, ssap->SiteSurvey[i].bssid);
			}
			retval += websWrite(wp, "]");
			dbg("\n");
		}
		else
			retval += websWrite(wp, "[]");
	}
	return retval;
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
getChannel(int band)
{
	int channel;
	struct iw_range	range;
	double freq;
	struct iwreq wrq1;
	struct iwreq wrq2;
	if (wl_ioctl(get_wifname(band), SIOCGIWFREQ, &wrq1) < 0)
		return 0;
	char buffer[sizeof(iwrange) * 2];
	bzero(buffer, sizeof(buffer));
	wrq2.u.data.pointer = (caddr_t) buffer;
	wrq2.u.data.length = sizeof(buffer);
	wrq2.u.data.flags = 0;

	if (wl_ioctl(get_wifname(band), SIOCGIWRANGE, &wrq2) < 0)
		return 0;

	if (ralink_get_range_info(&range, buffer, wrq2.u.data.length) < 0)
		return 0;

	freq = iw_freq2float(&(wrq1.u.freq));
	if (freq < KILO)
		channel = (int) freq;
	else
	{
		channel = iw_freq_to_channel(freq, &range);
		if (channel < 0)
			return 0;
	}
	return channel;
}
int
ej_wl_control_channel(int eid, webs_t wp, int argc, char_t **argv)
{
	int ret = 0;
	int channel_24 = 0, channel_50 = 0;
#if defined(RTCONFIG_HAS_6G) || defined(RTCONFIG_HAS_5G_2)
	int channel_tri = 0;
#endif

 	if (get_radio_status(get_wifname(0)))
  		channel_24=getChannel(0);
	if (get_radio_status(get_wifname(1)))
 		channel_50=getChannel(1);

#if defined(RTCONFIG_HAS_6G) || defined(RTCONFIG_HAS_5G_2)
	if (get_radio_status(get_wifname(2)))
 		channel_tri=getChannel(2);
	ret = websWrite(wp, "[\"%d\", \"%d\", \"%d\"]", channel_24, channel_50, channel_tri);
#else
	ret = websWrite(wp, "[\"%d\", \"%d\"]", channel_24, channel_50);
#endif

	return ret;
}


static int ej_wl_channel_list(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
	int retval = 0;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_";
	char *country_code;
	char chList[256];
	int band;

	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	country_code = nvram_get(strlcat_r(prefix, "country_code", tmp, sizeof(tmp)));
	band = unit;

	if (country_code == NULL || safe_strlen(country_code) != 2) return retval;

	if (band != 0 && band != 1
#if defined(RTCONFIG_HAS_6G) || defined(RTCONFIG_HAS_5G_2)
		&& band != 2
#endif
	)
		return retval;

#if defined(RTAC85U) || defined(RTAC85P) || defined(RTACRH26) || defined(TUFAC1750)
	if (!nvram_get_int("wlready")) return retval;
#endif

	chList[0] = '\0';
#if defined(XD4S)
  	if (!get_radio_status(get_wifname(band)))
  	{
  		retval += websWrite(wp, "[]");
  	}
  	else
#endif
	//try getting channel list via wifi driver first
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
	int retval = 0;
	char *ch_prefix = "", prefix[sizeof("wlX_XX")];
	char ch_list[2 * 4 + 3 * 22 + 4 * 34 + 8] = "";
	char tmp_ch_list[sizeof("6gXXX\", \"") * 59] = "";
	char tmp_chansps[sizeof("6g221/320-2XX")];
	char chansps_buf[2970] = "";	/* All 6G channel, except ch2, needs 2959 bytes. */
	uint64_t m, c_m, bw_m, chlist_mask = 0;
	const struct g_bw40chanspec_s *p = NULL;
	const struct g_bw80pchanspec_s *q = NULL;

	if (unit < 0 || unit >= WL_NR_BANDS)
		goto exit_ej_wl_chanspecs;

	if (get_channel_list_via_driver(unit, ch_list, sizeof(ch_list)) <= 0)
		goto exit_ej_wl_chanspecs;

	if (!(chlist_mask = chlist2bitmask(unit, ch_list, ",")))
		goto exit_ej_wl_chanspecs;

	if (unit == WL_6G_BAND) {
		ch_prefix = "6g";
	}

	snprintf(prefix, sizeof(prefix), "wl%d_", unit);

	/* Generate 20MHz channelspecslist. */
#if defined(RTCONFIG_HAS_6G)
	if (unit == WL_6G_BAND) {
		__bitmask2chlist(unit, chlist_mask, "\", \"6g", tmp_ch_list, sizeof(tmp_ch_list));
		if (wp) {
			retval += websWrite(wp, "[ \"%s%s", ch_prefix, tmp_ch_list);
		}
		__bitmask2chlist(unit, chlist_mask, " 6g", tmp_ch_list, sizeof(tmp_ch_list));
	} else
#endif
	{
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
	} else if (unit == WL_5G_BAND
#if defined(RTCONFIG_HAS_5G_2)
		|| unit == WL_5G_2_BAND
#endif
	) {
		p = g_5gbw40chanspec_tbl;
#if defined(RTCONFIG_HAS_6G)
	} else if (unit == WL_6G_BAND) {
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
	else if (unit == WL_5G_BAND
#if defined(RTCONFIG_HAS_5G_2)
	      || unit == WL_5G_2_BAND
#endif
	) {
		q = g_5gbw80pchanspec_tbl;
#if defined(RTCONFIG_HAS_6G)
	} else if (unit == WL_6G_BAND) {
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
ej_wl_channel_list_60g(int eid, webs_t wp, int argc, char_t **argv)
{
	return ej_wl_channel_list(eid, wp, argc, argv, WL_6G_BAND);
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
	return ej_wl_chanspecs(eid, wp, argc, argv, WL_5G_2_BAND);
#else
	return ej_wl_chanspecs(eid, wp, argc, argv, -1);
#endif
}

int ej_wl_chanspecs_6g(int eid, webs_t wp, int argc, char_t **argv)
{
#if defined(RTCONFIG_HAS_6G)
	return ej_wl_chanspecs(eid, wp, argc, argv, WL_6G_BAND);
#else
	return ej_wl_chanspecs(eid, wp, argc, argv, -1);
#endif
}

int ej_wl_chanspecs_6g_2(int eid, webs_t wp, int argc, char_t **argv)
{
#if defined(RTCONFIG_HAS_6G_2)
	return ej_wl_chanspecs(eid, wp, argc, argv, WL_6G_2_BAND);
#else
	return ej_wl_chanspecs(eid, wp, argc, argv, -1);
#endif
}

static int ej_wl_rate(int eid, webs_t wp, int argc, char_t **argv, int unit)
{
	struct iwreq wrq;
	int retval = 0;
	char tmp[256], prefix[] = "wlXXXXXXXXXX_";
	char *name;
	int rate=0;
	int status;
	char rate_buf[32];
	int sw_mode = sw_mode();
	int wlc_band = nvram_get_int("wlc_band");
	int from_app = 0;

	from_app = check_user_agent(user_agent);

#ifdef RTCONFIG_CONCURRENTREPEATER
	strlcpy(rate_buf, "auto", sizeof(rate_buf));
#else
	strlcpy(rate_buf, "0 Mbps", sizeof(rate_buf));	
#endif

	if (absent_band(unit))
		goto ERROR;
#ifdef RTCONFIG_CONCURRENTREPEATER
	if (sw_mode == SW_MODE_REPEATER || sw_mode == SW_MODE_HOTSPOT)
#else	
	if (wlc_band == unit && (sw_mode == SW_MODE_REPEATER || sw_mode == SW_MODE_HOTSPOT || wisp_mode()))
#endif		
		snprintf(prefix, sizeof(prefix), "wl%d.1_", unit);
	else
#if 0
		snprintf(prefix, sizeof(prefix), "wl%d_", unit);

	name = nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp)));

	wrq.u.bitrate.value=-1;
	if (wl_ioctl(name, SIOCGIWRATE, &wrq))
	{
		dbG("can not get rate info of %s\n", name);
		goto ERROR;
	}

	rate = wrq.u.bitrate.value;
	if ((rate == -1) || (rate == 0))
		strlcpy(rate_buf, "auto", sizeof(rate_buf));
	else
		snprintf(rate_buf, sizeof(rate_buf), "%d Mbps", (rate / 1000000));
#else
		goto ERROR;
#if defined(RTCONFIG_WISP)
	if(wisp_mode())
		name = get_staifname(unit);
	else
#endif		
	name = nvram_safe_get(strlcat_r(prefix, "ifname", tmp, sizeof(tmp)));

	memset(tmp, 0x00, sizeof(tmp));
	wrq.u.data.length = sizeof(tmp);
	wrq.u.data.pointer = (caddr_t) tmp;
	wrq.u.data.flags = ASUS_SUBCMD_CONN_STATUS;

	if (wl_ioctl(name, RTPRIV_IOCTL_ASUSCMD, &wrq) < 0)
	{
		dbg("%s: errors in getting %s CONN_STATUS result\n", __func__, name);
		goto ERROR;
	}
	status = ((int*)tmp)[0];
	rate   = ((int*)tmp)[1];

	if(status == 6)
		snprintf(rate_buf, sizeof(rate_buf), "%d Mbps", rate);
#endif

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
	if(sw_mode() == SW_MODE_REPEATER || wisp_mode())
		return ej_wl_rate(eid, wp, argc, argv, 0);
	else if(check_user_agent(user_agent) != FROM_BROWSER)
		return websWrite(wp, "\"\"");
	else
		return websWrite(wp, "");
}

int
ej_wl_rate_5g(int eid, webs_t wp, int argc, char_t **argv)
{
	if(sw_mode() == SW_MODE_REPEATER || wisp_mode())
		return ej_wl_rate(eid, wp, argc, argv, 1);
	else if(check_user_agent(user_agent) != FROM_BROWSER)
		return websWrite(wp, "\"\"");
	else
		return websWrite(wp, "");
}

int
ej_wl_rate_5g_2(int eid, webs_t wp, int argc, char_t **argv)
{
	if(sw_mode() == SW_MODE_REPEATER || wisp_mode())
		return ej_wl_rate(eid, wp, argc, argv, 2);
	else if(check_user_agent(user_agent) != FROM_BROWSER)
		return websWrite(wp, "\"\"");
	else
		return websWrite(wp, "");
}

int
ej_wl_rate_6g(int eid, webs_t wp, int argc, char_t **argv)
{
	if(sw_mode() == SW_MODE_REPEATER || wisp_mode())
		return ej_wl_rate(eid, wp, argc, argv, 3);
	else if(check_user_agent(user_agent) != FROM_BROWSER)
		return websWrite(wp, "\"\"");
	else
		return websWrite(wp, "");
}

int
ej_nat_accel_status(int eid, webs_t wp, int argc, char_t **argv)
{
	char val[4];
	int hwnat = 0;

	if (is_hwnat_loaded())
		hwnat = 1;

	if (hwnat && f_exists("/sys/kernel/debug/hnat/hook_toggle")) {
		if (f_read_string("/sys/kernel/debug/hnat/hook_toggle", val, sizeof(val)) > 0)
			hwnat = !!safe_atoi(val);
	}

	return websWrite(wp, "%d", hwnat);
}

#if defined(RTCONFIG_MT798X) || defined(RTCONFIG_WLMODULE_MT7915D_AP) || defined(RTCONFIG_MT799X)
/* Hook validate_apply().
 * Sync wl[0~2]_yyy with wlx_yyy if yyy in global_params[].
 */
static const char *global_params[] = { "atf", "plcphdr", "frameburst", "PktAggregate", "wme_no_ack", "mumimo", NULL };
void __validate_apply_set_wl_var(char *nv, char *val)
{
	const char **p;
	int band;
	char prefix[sizeof("wlxxx_")];

	if (!nv || (strncmp(nv, "wl0_", 4) && strncmp(nv, "wl1_", 4) && strncmp(nv, "wl2_", 4)))
		return;

	for (p = &global_params[0]; *p != NULL; ++p) {
		if (strcmp(nv + 4, *p))
			continue;

		for (band = 0; band < min(MAX_NR_WL_IF, WL_5G_2_BAND + 1); ++band) {
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
