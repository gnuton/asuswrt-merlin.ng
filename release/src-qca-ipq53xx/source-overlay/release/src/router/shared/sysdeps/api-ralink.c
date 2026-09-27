#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <typedefs.h>
#include <bcmnvram.h>
#include <sys/ioctl.h>
#include <iwlib.h>
#include "shutils.h"
#include <ralink.h>
#include <shared.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <iwlib.h>
#ifndef O_BINARY
#define O_BINARY 	0
#endif
#include <image.h>
#ifndef MAP_FAILED
#define MAP_FAILED (-1)
#endif
#include <linux_gpio.h>

typedef uint32_t __u32;

#if defined(RTN14U) || defined(RTAC52U) || defined(RTAC51U) || defined(RTN11P) || defined(RTN300) || defined(RTN54U) || defined(RTAC1200HP) || defined(RTN56UB1) || defined(RTN56UB2) ||defined(RTAC54U) || defined(RTAC51UP)|| defined(RTAC53) || defined(RTAC1200GA1) || defined(RTAC1200GU) || defined(RTAC1200) || defined(RTAC1200V2) || defined(RTN11P_B1) || defined(RPAC87) || defined(RTAC85U) || defined(RTAC85P) || defined(RTAC65U) || defined(RTN800HP) || defined(RTACRH26) || defined(TUFAC1750) || defined(RTACRH18)  || defined(RT4GAC86U) || defined(RTCONFIG_WLMODULE_MT7915D_AP)
const char WIF_5G[]	= "rai0";
const char WIF_2G[]	= "ra0";
const char WDSIF_5G[]	= "wdsi";
const char APCLI_5G[]	= "apclii0";
const char APCLI_2G[]	= "apcli0";
#elif defined(RTCONFIG_MT798X)
const char WIF_5G[]	= "rax0";
const char WIF_2G[]	= "ra0";
const char WDSIF_5G[]	= "wdsx";
const char APCLI_5G[]	= "apclix0";
const char APCLI_2G[]	= "apcli0";
#elif defined(RTCONFIG_MT799X)
const char WIF_6G[]	= "rax0";
#if defined(RTCONFIG_HAS_5G_2)
const char WIF_5G2[]   = "rax0";
#endif
const char WIF_5G[]	= "rai0";
const char WIF_2G[]	= "ra0";
const char WDSIF_5G[]	= "wdsx";
const char APCLI_6G[]	= "apclix0";
#if defined(RTCONFIG_HAS_5G_2)
const char APCLI_5G2[]   = "apclix0";
#endif
const char APCLI_5G[]	= "apclii0";
const char APCLI_2G[]	= "apcli0";
#else
const char WIF_5G[]	= "ra0";
const char WIF_2G[]	= "rai0";
const char WDSIF_5G[]	= "wds";
const char APCLI_5G[]	= "apcli0";
const char APCLI_2G[]	= "apclii0";
#endif

struct channel_info {
	unsigned char channel;
	unsigned char bandwidth;
	unsigned char extrach;
};

#if defined(RA_ESW)
/* Read TX/RX byte count information from switch's register. */
#if defined(RTCONFIG_RALINK_MT7620)
int get_mt7620_wan_unit_bytecount(int unit, unsigned long long *tx, unsigned long long *rx)
#elif defined(RTCONFIG_RALINK_MT7621)
int get_mt7621_wan_unit_bytecount(int unit, unsigned long long *tx, unsigned long long *rx)
#endif
{
#if defined(RTCONFIG_RALINK_MT7620)
	return __mt7620_wan_bytecount(unit, tx, rx);
#elif defined(RTCONFIG_RALINK_MT7621)
	return __mt7621_wan_bytecount(unit, tx, rx);
#endif
}
#endif

#if defined(RTCONFIG_MT798X) || defined(RTCONFIG_MT799X)
#include <limits.h>		//PATH_MAX, LONG_MIN, LONG_MAX
#define GPIOLIB_DIR	"/sys/class/gpio"
/* Export specified GPIO
 * @return:
 * 	0:	success
 *  otherwise:	fail
 */
static int __export_gpio(uint32_t gpio)
{
	char gpio_path[PATH_MAX], export_path[PATH_MAX], gpio_str[] = "999XXX";

	if (!d_exists(GPIOLIB_DIR)) {
		_dprintf("%s does not exist!\n", __func__);
		return -1;
	}
	snprintf(gpio_path, sizeof(gpio_path),"%s/gpio%d", GPIOLIB_DIR, gpio);
	if (d_exists(gpio_path))
		return 0;

	snprintf(export_path, sizeof(export_path), "%s/export", GPIOLIB_DIR);
	snprintf(gpio_str, sizeof(gpio_str), "%d", gpio);
	f_write_string(export_path, gpio_str, 0, 0);

	return 0;
}

#define PWM_SYS_PREFIX "/sys/class/pwm/pwmchip0"
uint32_t is_pwm_exported(uint8_t channel)
{
	char path[PATH_MAX], tmpbuf[20];
	snprintf(tmpbuf, sizeof(tmpbuf), "%u", channel);
	snprintf(path, sizeof(path), PWM_SYS_PREFIX"/pwm%u", channel);
	if (!d_exists(path))
		return 0;
	return 1;
}

uint32_t pwm_export(uint8_t channel, uint32_t period, uint32_t duty_cycle)
{
	char path[PATH_MAX], tmpbuf[20];

	if (!d_exists(PWM_SYS_PREFIX))
		return -1;

	snprintf(tmpbuf, sizeof(tmpbuf), "%u", channel);
	f_write_string(PWM_SYS_PREFIX"/export", tmpbuf, 0, 0);

	snprintf(path, sizeof(path), PWM_SYS_PREFIX"/pwm%u/period", channel);
	snprintf(tmpbuf, sizeof(tmpbuf), "%u", period);
	f_write_string(path, tmpbuf, 0, 0);

	snprintf(path, sizeof(path), PWM_SYS_PREFIX"/pwm%u/duty_cycle", channel);
	snprintf(tmpbuf, sizeof(tmpbuf), "%u", duty_cycle);
	f_write_string(path, tmpbuf, 0, 0);

	// toggle enable to make the status correct
	snprintf(path, sizeof(path), PWM_SYS_PREFIX"/pwm%u/enable", channel);
	f_write_string(path, "1", 0, 0);
	f_write_string(path, "0", 0, 0);

	return 0;
}

uint32_t gpio_dir(uint32_t gpio, int dir)
{
	char path[PATH_MAX], v[10], *dir_str = "in";

	if (gpio >= GPIO_PWM_DEFSHIFT) return 0; // PWM, only output
	if (dir == GPIO_DIR_OUT) {
		dir_str = "out";		/* output, low voltage */
		*v = '\0';
		snprintf(path, sizeof(path), "%s/gpio%d/value", GPIOLIB_DIR, gpio);
		if (f_read_string(path, v, sizeof(v)) > 0 && safe_atoi(v) == 1)
			dir_str = "high";	/* output, high voltage */
	} else if (dir == GPIO_DIR_OUT_LOW) {
                dir_str = "low";
        } else if (dir == GPIO_DIR_OUT_HIGH) {
                dir_str = "high";
        }

	__export_gpio(gpio);
	snprintf(path, sizeof(path), "%s/gpio%d/direction", GPIOLIB_DIR, gpio);
	f_write_string(path, dir_str, 0, 0);

	return 0;
}

uint32_t get_gpio(uint32_t gpio)
{
	char path[PATH_MAX], value[10];

	if (gpio >= GPIO_PWM_DEFSHIFT) { // PWM
#if defined(RTCONFIG_PWMX2_GPIOX1_RGBLED) || defined(RTCONFIG_PWMX3_RGBLED)
		snprintf(path, sizeof(path), PWM_SYS_PREFIX"/pwm%u/mm_enable", gpio - GPIO_PWM_DEFSHIFT);
#else
		snprintf(path, sizeof(path), PWM_SYS_PREFIX"/pwm%u/enable", gpio - GPIO_PWM_DEFSHIFT);
#endif
	} else {
		snprintf(path, sizeof(path), "%s/gpio%d/value", GPIOLIB_DIR, gpio);
	}
	f_read_string(path, value, sizeof(value));

	return safe_atoi(value);
}

uint32_t set_gpio(uint32_t gpio, uint32_t value)
{
	char path[PATH_MAX], val_str[10];

	snprintf(val_str, sizeof(val_str), "%d", !!value);
	if (gpio >= GPIO_PWM_DEFSHIFT) { // PWM
#if defined(RTCONFIG_PWMX2_GPIOX1_RGBLED) || defined(RTCONFIG_PWMX3_RGBLED)
		snprintf(path, sizeof(path), PWM_SYS_PREFIX"/pwm%u/mm_enable", gpio - GPIO_PWM_DEFSHIFT);
#else
		snprintf(path, sizeof(path), PWM_SYS_PREFIX"/pwm%u/enable", gpio - GPIO_PWM_DEFSHIFT);
#endif
	} else {
		snprintf(path, sizeof(path), "%s/gpio%d/value", GPIOLIB_DIR, gpio);
	}
	f_write_string(path, val_str, 0, 0);

	return 0;
}

#else // of RTCONFIG_MT798X
uint32_t gpio_dir(uint32_t gpio, int dir)
{
	return ralink_gpio_init(gpio, dir);
}
#if defined(RT4GAC86U) || defined(RTCONFIG_WLMODULE_MT7915D_AP)
uint32_t gpio_dir2(uint32_t gpio, int dir)
{
	return ralink_gpio_init2(gpio, dir);
}
#endif
uint32_t get_gpio(uint32_t gpio)
{
	return ralink_gpio_read_bit(gpio);
}

uint32_t set_gpio(uint32_t gpio, uint32_t value)
{
	ralink_gpio_write_bit(gpio, value);
	return 0;
}
#endif // of RTCONFIG_MT798X

int get_switch_model(void)
{
#if defined(RTCONFIG_RALINK_RT3052)
	return SWITCH_RT3052;
#elif defined(RTCONFIG_RALINK_MT7620)
	return SWITCH_MT7620;
#elif defined(RTCONFIG_RALINK_MT7621)
	return SWITCH_MT7621;
#elif defined(RTCONFIG_RALINK_MT7628)
	return SWITCH_MT7628;
#endif
	return SWITCH_UNKNOWN;
}

uint32_t get_phy_status(uint32_t portmask)
{
	// TODO
	return 1;
}

uint32_t get_phy_speed(uint32_t portmask)
{
	// TODO
	return 1;
}

uint32_t set_phy_ctrl(uint32_t portmask, int ctrl)
{
	// TODO
	return 1;
}

#if defined(RTCONFIG_FITFDT)
int get_imageheader_size(void)
{
	return sizeof(image_header_t);
}
#endif	/* RTCONFIG_FITFDT */

#define SWAP_LONG(x) \
	((__u32)( \
		(((__u32)(x) & (__u32)0x000000ffUL) << 24) | \
		(((__u32)(x) & (__u32)0x0000ff00UL) <<  8) | \
		(((__u32)(x) & (__u32)0x00ff0000UL) >>  8) | \
		(((__u32)(x) & (__u32)0xff000000UL) >> 24) ))



int wl_ioctl(const char *ifname, int cmd, struct iwreq *pwrq)
{
	int ret = 0;
 	int s;
	unsigned short subcmd = 0;
	void *orig_data, *new_data = NULL;

	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return errno;
	}

	/* do it */
	strncpy(pwrq->ifr_name, ifname, IFNAMSIZ);
#if defined(RTCONFIG_MT799X)
	if(cmd == RTPRIV_IOCTL_GET_MAC_TABLE || cmd == RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT) {
		cmd = RTPRIV_IOCTL_ASUSCMD;
		pwrq->u.data.flags = ASUS_SUBCMD_GET_MAC_TABLE_STRUCT;
	} else if (cmd == RTPRIV_IOCTL_SET) {
		cmd = RTPRIV_IOCTL_ASUSCMD;
		pwrq->u.data.flags = ASUS_SUBCMD_IOCTL_SET;
	} 
	else if (cmd == RT_PRIV_IOCTL) {
		unsigned short new_len = sizeof(pwrq->u.data.flags) + pwrq->u.data.length;

		subcmd = pwrq->u.data.flags;
		orig_data = pwrq->u.data.pointer;
		
		// Redirecting the pointer to a new data pointer.
		new_data = (void *)malloc(new_len * 1);
		memcpy(new_data, &subcmd, sizeof(pwrq->u.data.flags));
		memcpy(new_data+sizeof(pwrq->u.data.flags), orig_data, pwrq->u.data.length);
		pwrq->u.data.pointer = new_data;

		// new length
		pwrq->u.data.length = new_len;

		// new flags
		cmd = RTPRIV_IOCTL_ASUSCMD;
		pwrq->u.data.flags = ASUS_SUBCMD_IOCTL;
	}
	else if (cmd == RTPRIV_IOCTL_WSC_PROFILE) {
		cmd = RTPRIV_IOCTL_ASUSCMD;
		pwrq->u.data.flags = ASUS_SUBCMD_WSC_PROFILE;
	}
#endif

	if ((ret = ioctl(s, cmd, pwrq)) < 0)
		dbg("ioctl(%s, %d) errno(%d : %s)\n", pwrq->ifr_name, cmd, errno, strerror(errno));

	if (new_data) {
		pwrq->u.data.length = pwrq->u.data.length - sizeof(pwrq->u.data.flags);
		pwrq->u.data.flags = subcmd;
		memcpy(orig_data, new_data+sizeof(pwrq->u.data.flags), pwrq->u.data.length);
		pwrq->u.data.pointer = orig_data;
		free(new_data);
	}

	/* cleanup */
	close(s);
	return ret;
}

unsigned int get_radio_status(const char *ifname)
{
#if defined(RTCONFIG_NL80211)
	struct ifreq ifr;
	int sfd;
	int ret;

	if ((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) {
		strcpy(ifr.ifr_name, ifname);
		ret = ioctl(sfd, SIOCGIFFLAGS, &ifr);
		close(sfd);
		if (ret == 0)
			return !!(ifr.ifr_flags & IFF_UP);
	}
	return 0;
#else /* !RTCONFIG_NL80211 */
	struct iwreq wrq;
	unsigned int data = 0;

	wrq.u.data.length = sizeof(data);
	wrq.u.data.pointer = (caddr_t) &data;
	wrq.u.data.flags = ASUS_SUBCMD_RADIO_STATUS;
	if (wl_ioctl(ifname, RTPRIV_IOCTL_ASUSCMD, &wrq) < 0)
		printf("ioctl error\n");

	return data;
#endif /* RTCONFIG_NL80211 */
}

int get_radio(int unit, int subunit)
{
	char tmp[100], prefix[] = "wlXXXXXXXXXXXXXX";

	if (subunit > 0)
		snprintf(prefix, sizeof(prefix), "wl%d.%d_", unit, subunit);
	else
		snprintf(prefix, sizeof(prefix), "wl%d_", unit);

	// TODO: handle subunit
	if (subunit > 0)
		return nvram_match(strcat_r(prefix, "radio", tmp), "1");
	else
		return get_radio_status(nvram_safe_get(strcat_r(prefix, "ifname", tmp)));
}

void set_radio(int on, int unit, int subunit)
{
	char tmp[100], prefix[] = "wlXXXXXXXXXXXXXX";
	const char *ifname;

	if (subunit > 0)
		snprintf(prefix, sizeof(prefix), "wl%d.%d_", unit, subunit);
	else
		snprintf(prefix, sizeof(prefix), "wl%d_", unit);

	ifname = nvram_safe_get(strcat_r(prefix, "ifname", tmp));
	//if (nvram_match(strcat_r(prefix, "radio", tmp), "0")) return;
	// TODO: replace hardcoded 
	// TODO: handle subunit
#if defined(RTCONFIG_WLMODULE_MT7629_AP) \
   || defined(RTCONFIG_WLMODULE_MT7915D_AP) \
   || defined(RTCONFIG_WLMODULE_MT7628_AP) \
   || defined(RTCONFIG_WLMODULE_MT7663E_AP) \
   || defined(RTCONFIG_MT798X) \
   || defined(RTCONFIG_NL80211)
	// MTK suggested MT7629/MT7915D use ifconfig down/up to instead RadioOn=0/1
	doSystem("ifconfig %s %s", ifname, on ? "up":"down");
#else
	doSystem("iwpriv %s set RadioOn=%d", ifname, on);
#endif

#if defined(RTAC1200HP) || defined(RTN56UB1) || defined(RTN56UB2) || defined(RTAC1200GA1) || defined(RTAC1200GU) || defined(RTAC85U) || defined(RTAC85P) || defined(RTAC65U)  || defined(RTN800HP)  || defined(RTACRH26) || defined(TUFAC1750) || defined(RTACRH18) || defined(RTCONFIG_WLMODULE_MT7915D_AP) //5G:7612E 2G:7603E
	if (subunit <= 0) {
		led_onoff(unit);
	}
#endif	
}

char *wif_to_vif(char *wif)
{
	static char vif[32];
	int unit = 0, subunit = 0;
	char tmp[100], prefix[] = "wlXXXXXXXXXXXXXX";

	vif[0] = '\0';

	for (unit = 0; unit < MAX_NR_WL_IF; unit++)
	{
		SKIP_ABSENT_BAND(unit);
		for (subunit = 1; subunit < MAX_NO_MSSID; subunit++)
		{
			snprintf(prefix, sizeof(prefix), "wl%d.%d", unit, subunit);
			
			if (nvram_match(strcat_r(prefix, "_ifname", tmp), wif))
			{
				sprintf(vif, "%s", prefix);
				goto RETURN_VIF;
			}
		}
	}

RETURN_VIF:
	return vif;
}


/* get channel list via currently setting in wifi driver */
int get_channel_list_via_driver(int unit, char *buffer, int len)
{
	struct iwreq wrq;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_", *ifname;

	if(buffer == NULL || len <= 0)
		return -1;

	memset(buffer, 0, len);
	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	ifname = nvram_safe_get(strcat_r(prefix, "ifname", tmp));

	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.pointer = buffer;
	wrq.u.data.length  = len;
	wrq.u.data.flags   = ASUS_SUBCMD_CHLIST;
	if (wl_ioctl(ifname, RTPRIV_IOCTL_ASUSCMD, &wrq) < 0)
		return -1;

	return wrq.u.data.length;
}

int get_mtk_wifi_driver_version(char *buffer, int len)
{
	struct iwreq wrq;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_", *ifname;
	int unit = 0;

	if (buffer == NULL || len <= 0)
		return -1;
	memset(buffer, 0, len);
	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	ifname = nvram_safe_get(strcat_r(prefix, "ifname", tmp));
	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.pointer = buffer;
	wrq.u.data.length  = len;
	wrq.u.data.flags   = ASUS_SUBCMD_DRIVERVER;
	if (wl_ioctl(ifname, RTPRIV_IOCTL_ASUSCMD, &wrq) < 0)
		return -1;

	return wrq.u.data.length;
}

/* get channel list via value of countryCode */
unsigned char A_BAND_REGION_0_CHANNEL_LIST[]={36, 40, 44, 48, 149, 153, 157, 161, 165};
unsigned char A_BAND_REGION_1_CHANNEL_LIST[]={36, 40, 44, 48};
#ifdef RTCONFIG_LOCALE2012
unsigned char A_BAND_REGION_2_CHANNEL_LIST[]={36, 40, 44, 48, 52, 56, 60, 64, 149, 153, 157, 161, 165};
unsigned char A_BAND_REGION_3_CHANNEL_LIST[]={52, 56, 60, 64, 149, 153, 157, 161, 165};
#else
unsigned char A_BAND_REGION_2_CHANNEL_LIST[]={36, 40, 44, 48};
unsigned char A_BAND_REGION_3_CHANNEL_LIST[]={149, 153, 157, 161};
#endif
unsigned char A_BAND_REGION_4_CHANNEL_LIST[]={149, 153, 157, 161, 165};
unsigned char A_BAND_REGION_5_CHANNEL_LIST[]={149, 153, 157, 161};
#ifdef RTCONFIG_LOCALE2012
unsigned char A_BAND_REGION_6_CHANNEL_LIST[]={36, 40, 44, 48, 132, 136, 140, 149, 153, 157, 161, 165};
#else
unsigned char A_BAND_REGION_6_CHANNEL_LIST[]={36, 40, 44, 48};
#endif
unsigned char A_BAND_REGION_7_CHANNEL_LIST[]={36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 149, 153, 157, 161, 165, 169, 173};
unsigned char A_BAND_REGION_8_CHANNEL_LIST[]={52, 56, 60, 64};
#ifdef RTCONFIG_LOCALE2012
unsigned char A_BAND_REGION_9_CHANNEL_LIST[]={36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132};
#else
unsigned char A_BAND_REGION_9_CHANNEL_LIST[]={36, 40, 44, 48};
#endif
unsigned char A_BAND_REGION_10_CHANNEL_LIST[]={36, 40, 44, 48, 149, 153, 157, 161, 165};
unsigned char A_BAND_REGION_11_CHANNEL_LIST[]={36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 149, 153, 157, 161};
unsigned char A_BAND_REGION_12_CHANNEL_LIST[]={36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140};
unsigned char A_BAND_REGION_13_CHANNEL_LIST[]={52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 149, 153, 157, 161};
unsigned char A_BAND_REGION_14_CHANNEL_LIST[]={36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 136, 140, 149, 153, 157, 161, 165};
unsigned char A_BAND_REGION_15_CHANNEL_LIST[]={149, 153, 157, 161, 165, 169, 173};
unsigned char A_BAND_REGION_16_CHANNEL_LIST[]={52, 56, 60, 64, 149, 153, 157, 161, 165};
unsigned char A_BAND_REGION_17_CHANNEL_LIST[]={36, 40, 44, 48, 149, 153, 157, 161};
unsigned char A_BAND_REGION_18_CHANNEL_LIST[]={36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 132, 136, 140};
unsigned char A_BAND_REGION_19_CHANNEL_LIST[]={56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 149, 153, 157, 161};
unsigned char A_BAND_REGION_20_CHANNEL_LIST[]={36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 149, 153, 157, 161};
unsigned char A_BAND_REGION_21_CHANNEL_LIST[]={36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 149, 153, 157, 161};
unsigned char A_BAND_REGION_22_CHANNEL_LIST[]={36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 132, 136, 140, 149, 153, 157, 161 ,165};
unsigned char A_BAND_REGION_24_CHANNEL_LIST[]={36, 40, 44, 48, 52, 56, 60, 64, 132, 136, 140, 144, 149, 153, 157, 161 ,165};
unsigned char A_BAND_REGION_25_CHANNEL_LIST[]={36, 40, 44, 48, 52, 56, 60, 64};

unsigned char G_BAND_REGION_0_CHANNEL_LIST[]={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
unsigned char G_BAND_REGION_1_CHANNEL_LIST[]={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
unsigned char G_BAND_REGION_5_CHANNEL_LIST[]={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

#define A_BAND_REGION_0				0
#define A_BAND_REGION_1				1
#define A_BAND_REGION_2				2
#define A_BAND_REGION_3				3
#define A_BAND_REGION_4				4
#define A_BAND_REGION_5				5
#define A_BAND_REGION_6				6
#define A_BAND_REGION_7				7
#define A_BAND_REGION_8				8
#define A_BAND_REGION_9				9
#define A_BAND_REGION_10			10
#define A_BAND_REGION_11			11
#define A_BAND_REGION_12			12
#define A_BAND_REGION_13			13
#define A_BAND_REGION_14			14
#define A_BAND_REGION_15			15
#define A_BAND_REGION_16			16
#define A_BAND_REGION_17			17
#define A_BAND_REGION_18			18
#define A_BAND_REGION_19			19
#define A_BAND_REGION_20			20
#define A_BAND_REGION_21			21
#define A_BAND_REGION_22			22
#define A_BAND_REGION_24			24
#define A_BAND_REGION_25			25

#define G_BAND_REGION_0				0
#define G_BAND_REGION_1				1
#define G_BAND_REGION_2				2
#define G_BAND_REGION_3				3
#define G_BAND_REGION_4				4
#define G_BAND_REGION_5				5
#define G_BAND_REGION_6				6

typedef struct CountryCodeToCountryRegion {
	unsigned char	IsoName[3];
	unsigned char	RegDomainNum11A;
	unsigned char	RegDomainNum11G;
} COUNTRY_CODE_TO_COUNTRY_REGION;

COUNTRY_CODE_TO_COUNTRY_REGION allCountry[] = {
	/* {Country Number, ISO Name, Country Name, Support 11A, 11A Country Region, Support 11G, 11G Country Region} */
	{"DB", A_BAND_REGION_7, G_BAND_REGION_5},
	{"AL", A_BAND_REGION_0, G_BAND_REGION_1},
	{"DZ", A_BAND_REGION_0, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"AR", A_BAND_REGION_0, G_BAND_REGION_1},
	{"AM", A_BAND_REGION_1, G_BAND_REGION_1},
#else
	{"AR", A_BAND_REGION_3, G_BAND_REGION_1},
	{"AM", A_BAND_REGION_2, G_BAND_REGION_1},
#endif
	{"AU", A_BAND_REGION_0, G_BAND_REGION_1},
	{"AT", A_BAND_REGION_1, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"AZ", A_BAND_REGION_1, G_BAND_REGION_1},
#else
	{"AZ", A_BAND_REGION_2, G_BAND_REGION_1},
#endif
	{"BH", A_BAND_REGION_0, G_BAND_REGION_1},
	{"BY", A_BAND_REGION_0, G_BAND_REGION_1},
	{"BE", A_BAND_REGION_1, G_BAND_REGION_1},
	{"BZ", A_BAND_REGION_4, G_BAND_REGION_1},
	{"BO", A_BAND_REGION_4, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"BR", A_BAND_REGION_4, G_BAND_REGION_1},
#else
	{"BR", A_BAND_REGION_1, G_BAND_REGION_1},
#endif
	{"BN", A_BAND_REGION_4, G_BAND_REGION_1},
	{"BG", A_BAND_REGION_1, G_BAND_REGION_1},
	{"CA", A_BAND_REGION_0, G_BAND_REGION_0},
	{"CL", A_BAND_REGION_0, G_BAND_REGION_1},
	{"CN", A_BAND_REGION_4, G_BAND_REGION_1},
	{"CO", A_BAND_REGION_0, G_BAND_REGION_0},
	{"CR", A_BAND_REGION_0, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"HR", A_BAND_REGION_1, G_BAND_REGION_1},
#else
	{"HR", A_BAND_REGION_2, G_BAND_REGION_1},
#endif
	{"CY", A_BAND_REGION_1, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"CZ", A_BAND_REGION_1, G_BAND_REGION_1},
#else
	{"CZ", A_BAND_REGION_2, G_BAND_REGION_1},
#endif
	{"DK", A_BAND_REGION_1, G_BAND_REGION_1},
	{"DO", A_BAND_REGION_0, G_BAND_REGION_0},
	{"EC", A_BAND_REGION_0, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"EG", A_BAND_REGION_1, G_BAND_REGION_1},
#else
	{"EG", A_BAND_REGION_2, G_BAND_REGION_1},
#endif
	{"SV", A_BAND_REGION_0, G_BAND_REGION_1},
	{"EE", A_BAND_REGION_1, G_BAND_REGION_1},
	{"FI", A_BAND_REGION_1, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"FR", A_BAND_REGION_1, G_BAND_REGION_1},
	{"GE", A_BAND_REGION_1, G_BAND_REGION_1},
#else
	{"FR", A_BAND_REGION_2, G_BAND_REGION_1},
	{"GE", A_BAND_REGION_2, G_BAND_REGION_1},
#endif
	{"DE", A_BAND_REGION_1, G_BAND_REGION_1},
	{"GR", A_BAND_REGION_1, G_BAND_REGION_1},
	{"GT", A_BAND_REGION_0, G_BAND_REGION_0},
	{"HN", A_BAND_REGION_0, G_BAND_REGION_1},
	{"HK", A_BAND_REGION_0, G_BAND_REGION_1},
	{"HU", A_BAND_REGION_1, G_BAND_REGION_1},
	{"IS", A_BAND_REGION_1, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"IN", A_BAND_REGION_2, G_BAND_REGION_1},
#else
	{"IN", A_BAND_REGION_0, G_BAND_REGION_1},
#endif
	{"ID", A_BAND_REGION_4, G_BAND_REGION_1},
	{"IR", A_BAND_REGION_4, G_BAND_REGION_1},
	{"IE", A_BAND_REGION_1, G_BAND_REGION_1},
	{"IL", A_BAND_REGION_0, G_BAND_REGION_1},
	{"IT", A_BAND_REGION_1, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"JP", A_BAND_REGION_1, G_BAND_REGION_1},
#else
	{"JP", A_BAND_REGION_9, G_BAND_REGION_1},
#endif
	{"JO", A_BAND_REGION_0, G_BAND_REGION_1},
	{"KZ", A_BAND_REGION_0, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"KP", A_BAND_REGION_1, G_BAND_REGION_1},
	{"KR", A_BAND_REGION_1, G_BAND_REGION_1},
#else
	{"KP", A_BAND_REGION_5, G_BAND_REGION_1},
	{"KR", A_BAND_REGION_5, G_BAND_REGION_1},
#endif
	{"KW", A_BAND_REGION_0, G_BAND_REGION_1},
	{"LV", A_BAND_REGION_1, G_BAND_REGION_1},
	{"LB", A_BAND_REGION_0, G_BAND_REGION_1},
	{"LI", A_BAND_REGION_1, G_BAND_REGION_1},
	{"LT", A_BAND_REGION_1, G_BAND_REGION_1},
	{"LU", A_BAND_REGION_1, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"MO", A_BAND_REGION_4, G_BAND_REGION_1},
#else
	{"MO", A_BAND_REGION_0, G_BAND_REGION_1},
#endif
	{"MK", A_BAND_REGION_0, G_BAND_REGION_1},
	{"MY", A_BAND_REGION_0, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"MX", A_BAND_REGION_2, G_BAND_REGION_0},
	{"MC", A_BAND_REGION_1, G_BAND_REGION_1},
#else
	{"MX", A_BAND_REGION_0, G_BAND_REGION_0},
	{"MC", A_BAND_REGION_2, G_BAND_REGION_1},
#endif
	{"MA", A_BAND_REGION_0, G_BAND_REGION_1},
	{"NL", A_BAND_REGION_1, G_BAND_REGION_1},
	{"NZ", A_BAND_REGION_0, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"NO", A_BAND_REGION_1, G_BAND_REGION_0},
#else
	{"NO", A_BAND_REGION_0, G_BAND_REGION_0},
#endif
	{"OM", A_BAND_REGION_0, G_BAND_REGION_1},
	{"PK", A_BAND_REGION_0, G_BAND_REGION_1},
	{"PA", A_BAND_REGION_0, G_BAND_REGION_0},
	{"PE", A_BAND_REGION_4, G_BAND_REGION_1},
	{"PH", A_BAND_REGION_4, G_BAND_REGION_1},
	{"PL", A_BAND_REGION_1, G_BAND_REGION_1},
	{"PT", A_BAND_REGION_1, G_BAND_REGION_1},
	{"PR", A_BAND_REGION_0, G_BAND_REGION_0},
	{"QA", A_BAND_REGION_0, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"RO", A_BAND_REGION_1, G_BAND_REGION_1},
	{"RU", A_BAND_REGION_6, G_BAND_REGION_1},
#else
	{"RO", A_BAND_REGION_0, G_BAND_REGION_1},
#if defined(RTAC85P)
	{"RU", A_BAND_REGION_24, G_BAND_REGION_1},
	{"IL", A_BAND_REGION_25, G_BAND_REGION_1},
#else
	{"RU", A_BAND_REGION_0, G_BAND_REGION_1},
#endif
#endif
	{"SA", A_BAND_REGION_0, G_BAND_REGION_1},
	{"SG", A_BAND_REGION_0, G_BAND_REGION_1},
	{"SK", A_BAND_REGION_1, G_BAND_REGION_1},
	{"SI", A_BAND_REGION_1, G_BAND_REGION_1},
	{"ZA", A_BAND_REGION_1, G_BAND_REGION_1},
	{"ES", A_BAND_REGION_1, G_BAND_REGION_1},
	{"SE", A_BAND_REGION_1, G_BAND_REGION_1},
	{"CH", A_BAND_REGION_1, G_BAND_REGION_1},
	{"SY", A_BAND_REGION_0, G_BAND_REGION_1},
	{"TW", A_BAND_REGION_3, G_BAND_REGION_0},
	{"TH", A_BAND_REGION_0, G_BAND_REGION_1},
#ifdef RTCONFIG_LOCALE2012
	{"TT", A_BAND_REGION_1, G_BAND_REGION_1},
	{"TN", A_BAND_REGION_1, G_BAND_REGION_1},
	{"TR", A_BAND_REGION_1, G_BAND_REGION_1},
	{"UA", A_BAND_REGION_9, G_BAND_REGION_1},
#else
	{"TT", A_BAND_REGION_2, G_BAND_REGION_1},
	{"TN", A_BAND_REGION_2, G_BAND_REGION_1},
	{"TR", A_BAND_REGION_2, G_BAND_REGION_1},
	{"UA", A_BAND_REGION_0, G_BAND_REGION_1},
#endif
	{"AE", A_BAND_REGION_0, G_BAND_REGION_1},
	{"GB", A_BAND_REGION_1, G_BAND_REGION_1},
	{"US", A_BAND_REGION_0, G_BAND_REGION_0},
#ifdef RTCONFIG_LOCALE2012
	{"UY", A_BAND_REGION_0, G_BAND_REGION_1},
#else
	{"UY", A_BAND_REGION_5, G_BAND_REGION_1},
#endif
	{"UZ", A_BAND_REGION_1, G_BAND_REGION_0},
#ifdef RTCONFIG_LOCALE2012
	{"VE", A_BAND_REGION_4, G_BAND_REGION_1},
#else
	{"VE", A_BAND_REGION_5, G_BAND_REGION_1},
#endif
	{"VN", A_BAND_REGION_0, G_BAND_REGION_1},
	{"YE", A_BAND_REGION_0, G_BAND_REGION_1},
	{"ZW", A_BAND_REGION_0, G_BAND_REGION_1},
	{"",	0,	0}
};

#define NUM_OF_COUNTRIES	(sizeof(allCountry)/sizeof(COUNTRY_CODE_TO_COUNTRY_REGION))

int get_channel_list_via_country(int unit, const char *country_code, char *buffer, int len)
{
	unsigned char *pChannelListTemp = NULL;
	int index, num, i;
	char *p = buffer;
	int band = unit;

	if(buffer == NULL || len <= 0)
		return -1;

	memset(buffer, 0, len);

	if (band != 0 && band != 1) return -1;

	for (index = 0; index < NUM_OF_COUNTRIES; index++)
	{
		if (strncmp((char *) allCountry[index].IsoName, country_code, 2) == 0)
			break;
	}

	if (index >= NUM_OF_COUNTRIES) return 0;

	if (band == 1)
	switch (allCountry[index].RegDomainNum11A)
	{
		case A_BAND_REGION_0:
			num = sizeof(A_BAND_REGION_0_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_0_CHANNEL_LIST;
			break;
		case A_BAND_REGION_1:
			num = sizeof(A_BAND_REGION_1_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_1_CHANNEL_LIST;
			break;
		case A_BAND_REGION_2:
			num = sizeof(A_BAND_REGION_2_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_2_CHANNEL_LIST;
			break;
		case A_BAND_REGION_3:
			num = sizeof(A_BAND_REGION_3_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_3_CHANNEL_LIST;
			break;
		case A_BAND_REGION_4:
			num = sizeof(A_BAND_REGION_4_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_4_CHANNEL_LIST;
			break;
		case A_BAND_REGION_5:
			num = sizeof(A_BAND_REGION_5_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_5_CHANNEL_LIST;
			break;
		case A_BAND_REGION_6:
			num = sizeof(A_BAND_REGION_6_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_6_CHANNEL_LIST;
			break;
		case A_BAND_REGION_7:
			num = sizeof(A_BAND_REGION_7_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_7_CHANNEL_LIST;
			break;
		case A_BAND_REGION_8:
			num = sizeof(A_BAND_REGION_8_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_8_CHANNEL_LIST;
			break;
		case A_BAND_REGION_9:
			num = sizeof(A_BAND_REGION_9_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_9_CHANNEL_LIST;
			break;
		case A_BAND_REGION_10:
			num = sizeof(A_BAND_REGION_10_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_10_CHANNEL_LIST;
			break;
		case A_BAND_REGION_11:
			num = sizeof(A_BAND_REGION_11_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_11_CHANNEL_LIST;
			break;
		case A_BAND_REGION_12:
			num = sizeof(A_BAND_REGION_12_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_12_CHANNEL_LIST;
			break;
		case A_BAND_REGION_13:
			num = sizeof(A_BAND_REGION_13_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_13_CHANNEL_LIST;
			break;
		case A_BAND_REGION_14:
			num = sizeof(A_BAND_REGION_14_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_14_CHANNEL_LIST;
			break;
		case A_BAND_REGION_15:
			num = sizeof(A_BAND_REGION_15_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_15_CHANNEL_LIST;
			break;
		case A_BAND_REGION_16:
			num = sizeof(A_BAND_REGION_16_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_16_CHANNEL_LIST;
			break;
		case A_BAND_REGION_17:
			num = sizeof(A_BAND_REGION_17_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_17_CHANNEL_LIST;
			break;
		case A_BAND_REGION_18:
			num = sizeof(A_BAND_REGION_18_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_18_CHANNEL_LIST;
			break;
		case A_BAND_REGION_19:
			num = sizeof(A_BAND_REGION_19_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_19_CHANNEL_LIST;
			break;
		case A_BAND_REGION_20:
			num = sizeof(A_BAND_REGION_20_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_20_CHANNEL_LIST;
			break;
		case A_BAND_REGION_21:
			num = sizeof(A_BAND_REGION_21_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_21_CHANNEL_LIST;
			break;
		case A_BAND_REGION_22:
			num = sizeof(A_BAND_REGION_22_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_22_CHANNEL_LIST;
			break;
		case A_BAND_REGION_24:
			num = sizeof(A_BAND_REGION_24_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_24_CHANNEL_LIST;
			break;
		case A_BAND_REGION_25:
			num = sizeof(A_BAND_REGION_25_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = A_BAND_REGION_25_CHANNEL_LIST;
			break;
		default:	// Error. should never happen
			dbg("countryregionA=%d not support", allCountry[index].RegDomainNum11A);
			break;
	}
	else if (band == 0)
	switch (allCountry[index].RegDomainNum11G)
	{
		case G_BAND_REGION_0:
			num = sizeof(G_BAND_REGION_0_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = G_BAND_REGION_0_CHANNEL_LIST;
			break;
		case G_BAND_REGION_1:
			num = sizeof(G_BAND_REGION_1_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = G_BAND_REGION_1_CHANNEL_LIST;
			break;
		case G_BAND_REGION_5:
			num = sizeof(G_BAND_REGION_5_CHANNEL_LIST)/sizeof(unsigned char);
			pChannelListTemp = G_BAND_REGION_5_CHANNEL_LIST;
			break;
		default:	// Error. should never happen
			dbg("countryregionG=%d not support", allCountry[index].RegDomainNum11G);
			break;
	}

	if (pChannelListTemp != NULL)
	{
		for (i = 0; i < num; i++)
		{
#if 0
			if (i == 0)
				p += sprintf(p, "\"%d\"", pChannelListTemp[i]);
			else
				p += sprintf(p,  ", \"%d\"", pChannelListTemp[i]);
#else
			if (i == 0)
				p += sprintf(p, "%d", pChannelListTemp[i]);
			else
				p += sprintf(p,  ",%d", pChannelListTemp[i]);
#endif
		}
	}

	return (p - buffer);
}


#if defined(RTAC1200HP) || defined(RTN56UB1) || defined(RTN56UB2) || defined(RTAC1200GA1) || defined(RTAC1200GU) || defined(RTAC85U) || defined(RTAC85P) || defined(RTAC65U) || defined(RTN800HP) || defined(RTACRH26) || defined(TUFAC1750) || defined(RTACRH18) || defined(RTCONFIG_WLMODULE_MT7915D_AP)
void led_onoff(int unit)
{   
#if defined(RTAC1200HP)
	if(unit==1)
#endif		
		if(get_radio(unit, 0))
			led_control(get_wl_led_id(unit), LED_ON);
		else
			led_control(get_wl_led_id(unit), LED_OFF);
}
#endif

static char wan_base_if[IFNAMSIZ] = "";
int set_wan_base_if(char *wan_ifaces)
{
	strlcpy(wan_base_if, wan_ifaces, sizeof(wan_base_if));

	return 0;
}

/* Return wan_base_if for start_vlan().
 * @return:	pointer to base interface name for start_vlan().
 */
char *get_wan_base_if(void)
{
	//static char wan_base_if[IFNAMSIZ] = "";

	if (__get_wan_base_if) {
		__get_wan_base_if(wan_base_if);
		return wan_base_if;
	}

#if defined(RTCONFIG_RALINK_MT7620) || defined(RTCONFIG_RALINK_MT7628) /* RT-N14U, RT-AC52U, RT-AC51U, RT-N11P, RT-N54U, RT-AC1200HP, RT-AC54U */
	strlcpy(wan_base_if, "eth2", sizeof(wan_base_if));
#elif (defined(RTCONFIG_RALINK_MT7621) && !defined(RTCONFIG_WLMODULE_MT7915D_AP)) /* RT-N56UB1, RT-N56UB2 */
	strlcpy(wan_base_if, "eth3", sizeof(wan_base_if));
#endif

	return wan_base_if;
}

/* Return nvram variable name, e.g. et0macaddr, which is used to repented as LAN MAC.
 * @return:
 */
char *get_lan_mac_name(void)
{
	/* TODO: handle exceptional model */
	return "et0macaddr";
}

/* Return nvram variable name, e.g. et1macaddr, which is used to repented as WAN MAC.
 * @return:
 */
char *get_wan_mac_name(void)
{
	/* TODO: handle exceptional model */
	return "et1macaddr";
}

char *get_2g_hwaddr(void)
{
#if defined(RTCONFIG_MT799X)
        return get_lan_hwaddr();
#else
        return get_wan_hwaddr();
#endif
}

char *get_label_mac()
{
	return get_2g_hwaddr();
}

char *get_lan_hwaddr(void)
{
	/* TODO: handle exceptional model */
        return nvram_safe_get("et0macaddr");
}

char *get_wan_hwaddr(void)
{
	/* TODO: handle exceptional model */
        return nvram_safe_get("et1macaddr");
}

/**
 * Generate interface name based on @band and @subunit. (@subunit is NOT y in wlX.Y)
 * @band:
 * @subunit:
 * @buf:
 * @return:
 */
char *__get_wlifname(int band, int subunit, char *buf)
{
	if (!buf)
		return buf;

	strcpy(buf, get_wififname(band));
	if (subunit) {
		sprintf(buf + strlen(buf) - 1, "%d", subunit);
	}

	return buf;
}

char *get_wlifname(int unit, int subunit, int subunit_x, char *buf)
{
	char wifbuf[32];
	char prefix[]="wlXXXXXX_", tmp[100];
#if defined(RTCONFIG_WIRELESSREPEATER)
	if (sw_mode() == SW_MODE_REPEATER
#if !defined(RTCONFIG_CONCURRENTREPEATER)
	 && nvram_get_int("wlc_band") == unit
#endif
	 && subunit==1)
	{
		sprintf(buf, "%s", get_staifname(unit));
		return buf;
	}	
	else
#endif /* RTCONFIG_WIRELESSREPEATER */
#if defined(RTCONFIG_AMAS)
	if (sw_mode() == SW_MODE_AP && nvram_match("re_mode", "1") && (subunit <= 1)) {
		strcpy(buf, "");
		return buf;
	}
#endif  /* RTCONFIG_AMAS */
	{
		int len;
		memset(wifbuf, 0, sizeof(wifbuf));

		strlcpy(wifbuf, get_wififname(unit), sizeof(wifbuf));
		len = strlen(wifbuf);
		wifbuf[len - 1] = '\0';		//remove the last number

		snprintf(prefix, sizeof(prefix), "wl%d.%d_", unit, subunit);
		if (nvram_match(strcat_r(prefix, "bss_enabled", tmp), "1"))
		{
#if defined(RTCONFIG_RALINK_BUILDIN_WIFI)
#if defined(RTCONFIG_AMAS)
			if (aimesh_re_node() && subunit >=2) {
				sprintf(buf, "%s%d", wifbuf, subunit-1);
			} else
#endif
			sprintf(buf, "%s%d", wifbuf, subunit);
#else
			sprintf(buf, "%s%d", wifbuf, subunit_x);
#endif
		}
		else
			sprintf(buf, "%s", "");
	}	
	return buf;
}

/**
 * Generate VAP interface name of wlX.Y for Guest network, Free Wi-Fi, and Facebook Wi-Fi
 * @x:		X of wlX.Y, aka unit
 * @y:		Y of wlX.Y
 * @buf:	Pointer to buffer of VAP interface name. Must greater than or equal to IFNAMSIZ
 * @return:
 * 	NULL	Invalid @buf
 * 	""	Invalid parameters
 *  otherwise	VAP interface name of wlX.Y
 */
char *get_wlxy_ifname(int x, int y, char *buf)
{
	int i, sidx;
	char prefix[sizeof("wlX.Yxxx")];

	if (!buf)
		return buf;

	if (x < 0 || y < 0 || y >= MAX_NO_MSSID)
		return "";

	if (y == 0) {
		__get_wlifname(x, 0, buf);
		return buf;
	}

	*buf = '\0';
	for (i = 1, sidx = 1; i < MAX_NO_MSSID; ++i) {
		if (i == y) {
			__get_wlifname(x, sidx, buf);
			break;
		}

		snprintf(prefix, sizeof(prefix), "wl%d.%d_", x, i);
		if (is_bss_enabled(prefix))
			sidx++;
	}

	return buf;
}

int get_channel_list(int unit, int ch_list[], int size)
{
	struct iwreq wrq;
	char buffer[256], *data, *p = NULL, *tmplist = NULL, tmp[128], prefix[] = "wlXXXXXXXXXX_", *ifname;
	int ch_cnt = 0;

	memset(buffer, 0, sizeof(buffer));
	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	ifname = nvram_safe_get(strcat_r(prefix, "ifname", tmp));

	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.pointer = buffer;
	wrq.u.data.length  = sizeof(buffer);
	wrq.u.data.flags   = ASUS_SUBCMD_CHLIST;
	if (wl_ioctl(ifname, RTPRIV_IOCTL_ASUSCMD, &wrq) < 0) {
		dbg("wl_ioctl failed on %s (%d)\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (strlen(buffer) > 0) {
		p = tmplist = strdup(buffer);
		if (p) {
			while((data = strsep(&tmplist, ",")) != NULL) {
				if (ch_cnt >= size) {
					dbg("ch_cnt >= size\n");
					ch_cnt = -1;
					break;
				}

				ch_list[ch_cnt] = atoi(data);
				ch_cnt++;
			}
			free(p);
		}
	}

	return ch_cnt;
}

uint64_t get_channel_list_mask(enum wl_band_id band)
{
	uint64_t m = 0;
	int i, ch_list[64] = { 0 };

	if (band < 0 || band >= WL_NR_BANDS)
		return 0;

	get_channel_list(band, ch_list, ARRAY_SIZE(ch_list));
	for (i = 0; i < ARRAY_SIZE(ch_list) && ch_list[i] != 0; ++i)
		m |= ch2bitmask(band, ch_list[i]);

	return m;
}

int get_radar_channel_list(int unit, int radar_list[], int size)
{
	struct iwreq wrq;
	char buffer[256], *data, *p = NULL, *tmplist = NULL, tmp[128], prefix[] = "wlXXXXXXXXXX_", *ifname;
	int r, radar_cnt = 0;

	if (!nvram_match("wlready", "1"))
		return 0;

	memset(buffer, 0, sizeof(buffer));
	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	ifname = nvram_safe_get(strcat_r(prefix, "ifname", tmp));

	memset(&wrq, 0, sizeof(wrq));
	wrq.u.data.pointer = buffer;
	wrq.u.data.length  = sizeof(buffer);
	wrq.u.data.flags   = ASUS_SUBCMD_GDFSNOPCHANNEL;
	if ((r = wl_ioctl(ifname, RTPRIV_IOCTL_ASUSCMD, &wrq)) < 0) {
		dbg("%s failed, ret %d\n", __func__, r);
		return -1;
	}

	if (strlen(buffer) > 0) {
		p = tmplist = strdup(buffer);
		if (p) {
			while((data = strsep(&tmplist, ",")) != NULL) {
				if (radar_cnt >= size) {
					dbg("radar_cnt >= size\n");
					radar_cnt = -1;
					break;
				}

				radar_list[radar_cnt] = atoi(data);
				radar_cnt++;
			}
			free(p);
		}
	}

	return radar_cnt;
}

int set_acl_entry(const char *ifname, char *addr)
{
	struct iwreq wrq;
	char data[256];

	snprintf(data, sizeof(data), "ACLAddEntry=%s", addr);

	wrq.u.data.length = strlen(data) + 1;
	wrq.u.data.pointer = data;
	wrq.u.data.flags = 0;

	if (wl_ioctl(ifname, RTPRIV_IOCTL_SET, &wrq) < 0) {
		dbg("wl_ioctl failed on %s (%d)\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 1;
}

int set_channel(const char* ifname, int channel)
{
	struct iwreq wrq;
	char data[32];

	snprintf(data, sizeof(data), "Channel=%d", channel);

	wrq.u.data.length = strlen(data) + 1;
	wrq.u.data.pointer = data;
	wrq.u.data.flags = 0;

	if (wl_ioctl(ifname, RTPRIV_IOCTL_SET, &wrq) < 0) {
		dbg("wl_ioctl failed on %s (%d)\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}

int set_bandwidth(const char* ifname, int ht_bw, int vht_bw)
{
	struct iwreq wrq;
	char data[32];

	if (ht_bw >= 0) {
		snprintf(data, sizeof(data), "HtBw=%d", ht_bw);

		wrq.u.data.length = strlen(data) + 1;
		wrq.u.data.pointer = data;
		wrq.u.data.flags = 0;

		if (wl_ioctl(ifname, RTPRIV_IOCTL_SET, &wrq) < 0) {
			dbg("wl_ioctl failed on %s (%d)\n", __FUNCTION__, __LINE__);
			return -1;
		}
	}

	if (vht_bw >= 0) {
		snprintf(data, sizeof(data), "VhtBw=%d", vht_bw);

		wrq.u.data.length = strlen(data) + 1;
		wrq.u.data.pointer = data;
		wrq.u.data.flags = 0;

		if (wl_ioctl(ifname, RTPRIV_IOCTL_SET, &wrq) < 0) {
			dbg("wl_ioctl failed on %s (%d)\n", __FUNCTION__, __LINE__);
			return -1;
		}
	}

        return 0;
}

int set_extra_channel(const char* ifname, int val)
{
        struct iwreq wrq;
        char data[32];

	snprintf(data, sizeof(data), "HtExtcha=%d", (val == 0 ? 1: 0));

        wrq.u.data.length = strlen(data) + 1;
        wrq.u.data.pointer = data;
        wrq.u.data.flags = 0;

        if (wl_ioctl(ifname, RTPRIV_IOCTL_SET, &wrq) < 0) {
                dbg("wl_ioctl failed on %s (%d)\n", __FUNCTION__, __LINE__);
                return -1;
        }

        return 0;
}

int set_bw_nctrlsb(const char* ifname, int bw, int nctrlsb)
{
	int ht_bw = -1, vht_bw = -1;
	int unit = -1;

	/* set bandwidth */
	//HtBw (0: 20M, 1: 20/40M)
	//vhtBw (0: disable, 1: 80M, 2: 160M, 3:80M+80M)
	if (bw == 20)
		ht_bw = 0;
	else if (bw == 40)
		ht_bw = 1;
	else if (bw == 80)
		vht_bw = 1;
	else if (bw == 160)
		vht_bw = 2;
	else
	{
		dbg("bw (%d) is invalid\n", bw);
		return -1;
	}

	/* get unit */
	if ((unit = get_wifi_unit((char *)ifname)) < 0) {
		dbg("unit (%d) is invalid\n", unit);
	}

	/* re-assing ht_bw/vht_bw */
	if (bw == 20 || bw == 40) {
		if (unit > 0)
			vht_bw = 0;
	}
        else if (bw == 80 || bw == 160) {
		if (unit > 0)
			ht_bw = 1;
	}

	/* set bandwidth */
	if (set_bandwidth(ifname, ht_bw, vht_bw)) {
		dbg("set_bandwidth(%s, %d, %d) failed\n", ifname, ht_bw, vht_bw);
		return -1;
	}

	/* set extra channel if bw is 40Mhz */
	if (bw == 40 && nctrlsb >= 0) {
		if (set_extra_channel(ifname, nctrlsb) < 0) {
			dbg("set_extra_channel() failed\n");
			return -1;
		}
	}

	return 0;
}

int get_channel_info(const char *ifname, int *channel, int *bw, int *nctrlsb)
{
	struct iwreq wrq;
	struct channel_info info;

	memset(&info, 0, sizeof(struct channel_info));
	wrq.u.data.length = sizeof(struct channel_info);
	wrq.u.data.pointer = (caddr_t) &info;
	wrq.u.data.flags = ASUS_SUBCMD_GCHANNELINFO;

	if (wl_ioctl(ifname, RTPRIV_IOCTL_ASUSCMD, &wrq) < 0) {
		dbg("wl_ioctl failed on %s (%d)\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (channel)
		*channel = (int)info.channel;

	if (bw) {
		switch (info.bandwidth) {
		case 0:
			*bw = 20;
			break;
		case 1:
			*bw = 40;
			break;
		case 2:
			*bw = 80;
			break;
		case 3:
			*bw = 160;
			break;
#if defined(RTCONFIG_BW320M)
		case 7:
			*bw = 320;
			break;
#endif
		default:
			*bw = 0;
			break;
		}
	}

	if (nctrlsb && info.bandwidth == 1) {
		switch (info.extrach) {
		case 1:
			*nctrlsb = 0;
			break;
		case 3:
			*nctrlsb = 1;
			break;
		default:
			*nctrlsb = -1;
			break;
		}
	}

        return 0;
}

int get_regular_class(const char* ifname)
{
	char data[32];
	struct iwreq wrq;

	memset(data, 0x00, sizeof(data));
	wrq.u.data.length = sizeof(data);
	wrq.u.data.pointer = (caddr_t) data;
	wrq.u.data.flags = ASUS_SUBCMD_GET_RCLASS;

	if (wl_ioctl(ifname, RTPRIV_IOCTL_ASUSCMD, &wrq) < 0)
	{
		dbg("errors in getting rclass result\n");
		return 0;
	}

	if (wrq.u.data.length > 0)
	{
		return atoi(wrq.u.data.pointer);
	}

	return 0;
}

#if defined(RTCONFIG_HAS_6G)
static const char *wif[] = { WIF_2G, WIF_5G, WIF_6G };
static const char *sta[] = { APCLI_2G, APCLI_5G, APCLI_6G };
#elif defined(RTCONFIG_HAS_5G_2)
static const char *wif[] = { WIF_2G, WIF_5G, WIF_5G2 };
static const char *sta[] = { APCLI_2G, APCLI_5G, APCLI_5G2 };
#else
static const char *wif[] = { WIF_2G, WIF_5G };
static const char *sta[] = { APCLI_2G, APCLI_5G };
#endif

char *get_wififname(int band)
{
	if (band < 0 || band >= ARRAY_SIZE(wif)) {
		printf("%s: Invalid wl%d band!\n", __func__, band);
		band = 0;
	}
	return (char*) wif[band];
}

int get_wififname_unit(const char *ifname)
{
        int band;
        if (!ifname)
                return -1;
        for (band = 0; band < min(MAX_NR_WL_IF, ARRAY_SIZE(wif)); ++band) {
                SKIP_ABSENT_BAND(band);
                if (!strncmp(ifname, wif[band], strlen(wif[band])))
                        return band;
        }
        return -1;
}

char *get_staifname(int band)
{
	if (band < 0 || band >= ARRAY_SIZE(sta)) {
		printf("%s: Invalid wl%d band!\n", __func__, band);
		band = 0;
	}
	return (char*) sta[band];
}

int get_sta_ifname_unit(const char *ifname)
{
        int band;
        if (!ifname)
                return -1;
        for (band = 0; band < min(MAX_NR_WL_IF, ARRAY_SIZE(sta)); ++band) {
                SKIP_ABSENT_BAND(band);
                if (!strncmp(ifname, sta[band], strlen(sta[band])))
                        return band;
        }
        return -1;
}

#ifdef RTCONFIG_BONDING_WAN
/** Return speed of a bonding interface.
 * @bond_if:	name of bonding interface. LAN bond_if = bond0; WAN bond_if = bond1.
 * @return:
 *  <= 0	error
 *  otherwise	link speed
 */
int get_bonding_speed(char *bond_if)
{
	int speed;
	char confbuf[sizeof(SYS_CLASS_NET) + IFNAMSIZ + sizeof("/speedXXXXXX")];
	char buf[32] = { 0 };

	snprintf(confbuf, sizeof(confbuf), SYS_CLASS_NET "/%s/speed", bond_if);
	if (f_read_string(confbuf, buf, sizeof(buf)) <= 0)
		return 0;

	speed = safe_atoi(buf);
	if (speed <= 0)
		speed = 0;

	return speed;
}

/** Return link speed of a bonding slave port if it's connected or 0 if it's disconnected.
 * @port:	0: WAN, 1~8: LAN1~8, 30: 10G base-T (RJ-45), 31: 10G SFP+
 * @return:
 *  <= 0:	disconnected
 *  otherwise:	link speed
 */
int get_bonding_port_status(int port)
{
	int ret = 0;

	if (__get_bonding_port_status)
		ret = __get_bonding_port_status(port);

	return ret;
}
#endif /* RTCONFIG_BONDING_WAN */

int get_ap_mac(const char *ifname, struct iwreq *pwrq)
{
	return wl_ioctl(ifname, SIOCGIWAP, pwrq);
}

const unsigned char ether_zero[6]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const unsigned char ether_bcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

int chk_assoc(const char *ifname)
{
	struct iwreq wrq;
	int ret;

	if((ret = get_ap_mac(ifname, &wrq)) < 0)
		return ret;

#if 0
cprintf("## %s(): ret(%d) ap_addr(%02x:%02x:%02x:%02x:%02x:%02x)\n", __func__, ret
, wrq.u.ap_addr.sa_data[0], wrq.u.ap_addr.sa_data[1], wrq.u.ap_addr.sa_data[2]
, wrq.u.ap_addr.sa_data[3], wrq.u.ap_addr.sa_data[4], wrq.u.ap_addr.sa_data[5]);
#endif
	if(memcmp(&(wrq.u.ap_addr.sa_data), ether_zero, 6) == 0)
		return 0;	// Not-Associated
	else if(memcmp(&(wrq.u.ap_addr.sa_data), ether_bcast, 6) == 0)
		return -1;	// Invalid

	return 1;
}

int get_ch_cch_bw(const char *wlif_name, int *ch, int *cch, int *bw)
{
	struct iwreq wrq;
	char data[256];
	char *p;
	int cnt = 0;

	if (wlif_name == NULL || wlif_name[0] == '\0')
		return -1;

	data[0] = '\0';
	wrq.u.data.length = sizeof(data);
	wrq.u.data.pointer = (caddr_t) data;
	wrq.u.data.flags = ASUS_SUBCMD_GET_CH_BW;
	if (wl_ioctl(wlif_name, RTPRIV_IOCTL_ASUSCMD, &wrq) < 0) { 
		dbg("%s: wl_ioctl(%s, ASUS_SUBCMD_GET_CH_BW) fail\n", __func__, wlif_name);
		return -1;
	}
	if (  ch != NULL && (p = strstr(data, "\nchannel: ")) != NULL ) {
		p += 10;
		*ch = atoi(p);
		cnt++;
	}
	if ( cch != NULL && (p = strstr(data, "\ncen_ch1: ")) != NULL ) {
		p += 10;
		*cch = atoi(p);
		cnt++;
	}
	if (  bw != NULL && (p = strstr(data, "\nbw: ")) != NULL ) {
		p += 5;
		*bw = atoi(p);
		cnt++;
	}
	//cprintf("%s: wlif_name(%s) ch(%d) cch(%d) bw(%d)\n", __func__, wlif_name, *ch, *cch, *bw);
	return cnt;
}

unsigned long long get_bitrate(const char *ifname)
{
	struct iwreq wrq;

	if (ifname == NULL)
		return -1;

	if (wl_ioctl(ifname, SIOCGIWRATE, &wrq))
		return -1;

	return wrq.u.bitrate.value;
}

/*
 * Get wireless client info via ioctl ASUS_SUBCMD_GROAM_COUNT and ASUS_SUBCMD_GROAM
 * NOTICE: The return pointer must be released via free() when not in use.
 */
char *get_sta_info(char *ifname, char *data, int *data_len)
{
	struct iwreq wrq;
	char num[4];
	int len, cnt;

	if (!nvram_match("wlready", "1"))
		return NULL;
	*data_len = -1;

	/* get number of station */
	memset(num, 0x0, sizeof(num));
	wrq.u.data.length = sizeof(num);
	wrq.u.data.pointer = (caddr_t)num;
	wrq.u.data.flags = ASUS_SUBCMD_GROAM_COUNT;
	if (wl_ioctl(ifname, RTPRIV_IOCTL_ASUSCMD, &wrq) < 0) {
		//dbg("%s: WI[%s] ASUS_SUBCMD_GROAM_COUNT failure\n", __func__, ifname);
		cnt = 32;
		goto cnt_32;
	}

	/* allocate memory for data buffer */
	if ((cnt = atoi(wrq.u.data.pointer)) == 0) {
		//dbg("%s: WI[%s] no client!\n", __func__, ifname);
		*data_len = 0;
		return NULL;
	}
cnt_32:
	len = sizeof(sta_info) * cnt;
	data = (char *)malloc(len);
	if (!data) {
		//dbg("%s: WI[%s] malloc failure\n", __func__, ifname);
		return NULL;
	}

	/* get MAC of station */
	memset(data, 0x0, len);
	wrq.u.data.length = len;
	wrq.u.data.pointer = (caddr_t)data;
	wrq.u.data.flags = ASUS_SUBCMD_GROAM;
	if (wl_ioctl(ifname, RTPRIV_IOCTL_ASUSCMD, &wrq) < 0) {
		//dbg("%s: WI[%s] ASUS_SUBCMD_GROAM failure\n", __func__, ifname);
		free(data);
		return NULL;
	}
	*data_len = wrq.u.data.length;

	return (char *)wrq.u.data.pointer;
}
#ifdef RTCONFIG_MT799X
int get_wifi_temperature(enum wl_band_id band)
{
	int t = 0;
	char cmd[sizeof("mwctl dev XXXX stat")];

	if (band >= MAX_NR_WL_IF)
		return -1;
	
	/* cmd: mwctl dev ra0 stat
	 * Example:
	 * CurrentTemperature = 62
	 * LVTS0 = 65358
	 * LVTS1 = 65282
	 * LVTS2 = 66443
	 * LVTS3 = 65664
	 * Tx success = 0
	 * Tx fail count = 0, PER=0.0%
	 * Current BW Tx count = 0
	 * Other BW Tx count = 0
	 * Rx success = 205216
	 * Rx with CRC = 57263, PER=21.8%
	 * Rx drop due to out of resource = 0
	 * Rssi: 0 0 0 0 
	 * CN Info: = 127
	 *       ...
	 *       ...
	 */

	snprintf(cmd, sizeof(cmd), "mwctl dev %s stat", get_wififname(band));
	if (exec_and_parse(cmd, "CurrentTemperature", "%*[^=]=%d%*[^\n]", 1, &t))
		return 0;
	
	return t;
}
#endif

#if defined(RTCONFIG_SW_BTN)
int set_sdn_active_by_unit(int sdn_idx, int onoff)
{
	char *nv = NULL;
        char *nvp = NULL;
        char *b = NULL,*p=NULL;
        char *idx = NULL;
        char *sdn_type = NULL;
        char *sdn_enable = NULL;
        char *vlan_idx = NULL;
        char *sub_idx = NULL;
        char *apg_idx = NULL;
        int err;
	int offset;
	int i=0;
	char apg_enable[20];
	char sdn[MTLAN_MAXINUM][200],temp[200];
        err=-1;
        nv = nvp = strdup(nvram_safe_get("sdn_rl"));
        if (!nv)
                return err;
	//_dprintf("sdn_rl=[%s]\n",nv);
	err=-2;
	memset(sdn,0,sizeof(sdn));
        while ((b = strsep(&nvp, "<")) != NULL) {
		memset(temp,0,sizeof(temp));
		strlcat(temp,b,sizeof(temp));
		i++;
                if (vstrsep(b, ">", &idx, &sdn_type, &sdn_enable, &vlan_idx, &sub_idx, &apg_idx) < 6)
                        continue;

                if(atoi(idx) == sdn_idx){
                        if(atoi(sdn_enable)!=onoff)
			{
				offset=strlen(idx)+1+strlen(sdn_type)+1;
				temp[offset]=onoff+48; //'0' or '1'
				snprintf(apg_enable,sizeof(apg_enable),"apg%d_enable",atoi(apg_idx));
				nvram_set_int(apg_enable,onoff);
				//_dprintf("==> %s=%d and update sdn rule=[%s]\n", apg_enable, onoff,temp);
                                err=0;
			}
			else
				err=-3; //no need to change
                }
		strcat(sdn,"<");
		strlcat(sdn,temp,sizeof(sdn));
        }	
	if(err==0)
	{
		//_dprintf("update sdn=[%s]\n",sdn);
		nvram_set("sdn_rl",sdn);
		nvram_commit();
	}	
        free(nv);
        return err;

}	
#endif

#if defined(RTCONFIG_NL80211)
/* reference from QCA platform */
#define WPA_CLI_REPLY_SIZE		32
#define QUERY_WPA_CLI_REPLY_TIMEOUT	10
#define QUERY_WPA_STATE_TIMEOUT		25
char *wpa_cli_reply(const char *fcmd, char *reply)
{
	FILE *fp;
	int rlen;

	fp = popen(fcmd, "r");
	if (fp) {
		rlen = fread(reply, 1, WPA_CLI_REPLY_SIZE, fp);
		pclose(fp);
		if (rlen > 1) {
			reply[rlen-1] = '\0';
			return reply;
		}
	}

	return "";
}

void set_wpa_cli_cmd(int band, const char *cmd, int chk_reply)
{
	char *sta;
	char fcmd[128];
	char reply[WPA_CLI_REPLY_SIZE];
	int timeout = QUERY_WPA_CLI_REPLY_TIMEOUT;
	int scan_and_with_scan_events = 0;

	if (band < 0 || band >= MAX_NR_WL_IF || cmd == NULL || cmd[0] == '\0')
		return;

	sta = get_staifname(band);
	if (chk_reply) {
		if (strcmp(cmd, "scan") == 0) { // check if scan_events is supported
			char *rpt;
			snprintf(fcmd, sizeof(fcmd), MWPA_CLI" -p "MWPA_CTRL_PATH" -i %s scan_events", sta);
			rpt = wpa_cli_reply(fcmd, reply);
			if ((strcmp(rpt, "YES")==0) || (strcmp(rpt, "NO")==0))
				scan_and_with_scan_events = 1;
		}
		if (scan_and_with_scan_events) {
			eval(MWPA_CLI, "-p", MWPA_CTRL_PATH, "-i", sta, (char*) cmd); // just issue scan command & wait scan_events
			snprintf(fcmd, sizeof(fcmd), MWPA_CLI" -p "MWPA_CTRL_PATH" -i %s scan_events", sta);
			timeout = QUERY_WPA_STATE_TIMEOUT;
			while (strcmp(wpa_cli_reply(fcmd, reply), "YES") && timeout--) {
				//dbg("%s(%d): reply [%s] ...(%d/%d)\n", __func__, band, reply, timeout, QUERY_WPA_STATE_TIMEOUT);
				sleep(1);
			};
		} else { // non-scan cmd or no scan_events supported
			snprintf(fcmd, sizeof(fcmd), MWPA_CLI" -p "MWPA_CTRL_PATH" -i %s %s", sta, cmd);
			while (strcmp(wpa_cli_reply(fcmd, reply), "OK") && timeout--) {
				//dbg("%s(%d): reply [%s] ...(%d/%d)\n", __func__, band, reply, timeout, QUERY_WPA_CLI_REPLY_TIMEOUT);
				sleep(1);
			};

			if (strcmp(cmd, "scan") == 0) {
				snprintf(fcmd, sizeof(fcmd), MWPA_CLI" -p "MWPA_CTRL_PATH" -i %s status | grep wpa_state=", sta);
				timeout = QUERY_WPA_STATE_TIMEOUT;
				while (!strcmp(wpa_cli_reply(fcmd, reply), "wpa_state=SCANNING") && timeout--) {
					//dbg("%s(%d): reply [%s] ...(%d/%d)\n", __func__, band, reply, timeout, QUERY_WPA_STATE_TIMEOUT);
					sleep(1);
				};
			}
		}
	}
	else
		eval(MWPA_CLI, "-p", MWPA_CTRL_PATH, "-i", sta, (char*) cmd);
}
#endif /* RTCONFIG_NL80211 */

#if defined (RTCONFIG_MLO)
int init_mld_macaddr(void)
{
	int i;
	char mld_addr[18];
	int m[6],addr;
	const char *mld[] = { "mld_ap_addr", "mld_sdn_addr", "mld_sta_addr" };
	if(sscanf(get_2g_hwaddr(), "%02X:%02X:%02X:%02X:%02X:%02X",&m[0],&m[1],&m[2],&m[3],&m[4],&m[5]) != 6) 
	{
		_dprintf("invalid init mld mac address\n");
		return -1;	
	}
	addr=0x2; //local admin bit
	for(i=0;i<3;i++)
	{	
		memset(mld_addr,0,sizeof(mld_addr));
		snprintf(mld_addr,sizeof(mld_addr),"%02X:%02X:%02X:%02X:%02X:%02X",addr,m[1],m[2],m[3],m[4],m[5]);
		nvram_set(mld[i],mld_addr);
		addr+=0x4;
	}
	return 0;
}

#ifdef RTCONFIG_MULTILAN_MWL
char *get_sdntype_iface(const char *sdn_type_search, int band, char *out_buf, int buf_len)
{
	char *nv = NULL;
        char *nvp = NULL;
        char *b = NULL;
        char *idx = NULL;
        char *sdn_type = NULL;
        char *sdn_enable = NULL;
        char *vlan_idx = NULL;
        char *sub_idx = NULL;
        char *apg_idx = NULL;
	char word[64], *next = NULL, tmp[100],prefix[]="wlxxxxxxxx";
        ap_wifi_rule_st *ap_wifi_rl;
	int i,total=0,sdnidx;
	char iface[100];
	sdnidx=-1;
	memset(iface, 0, sizeof(iface));
        nv = nvp = strdup(nvram_safe_get("sdn_rl"));
        if (!nv)
                return NULL;

	while ((b = strsep(&nvp, "<")) != NULL) {
                if (vstrsep(b, ">", &idx, &sdn_type, &sdn_enable, &vlan_idx, &sub_idx, &apg_idx) < 6)
                        continue;
		if(strcmp(sdn_type, sdn_type_search)==0){
			sdnidx=atoi(idx);
			break;
		}	
        }

        free(nv);
	if(sdnidx==-1)
		return NULL;

	ap_wifi_rl = (ap_wifi_rule_st *) malloc(sizeof(ap_wifi_rule_st) * MAX_AP_RULE_LIST);
	if (!ap_wifi_rl)
		return NULL;

        memset(ap_wifi_rl, 0, (sizeof(ap_wifi_rule_st) * MAX_AP_RULE_LIST));
        if (!get_ap_wifi_rl_from_nvram(ap_wifi_rl, MAX_AP_RULE_LIST, &total) || total <= 0) {
		free(ap_wifi_rl);
                return NULL;
	}

	snprintf(prefix, sizeof(prefix), "wl%d", band);
        for (i=0; i<total; i++) {
		if(ap_wifi_rl[i].sdn_idx==sdnidx)
		{
                	foreach_44 (word, ap_wifi_rl[i].wlif_set, next) 
			{
				if(strncmp(word,prefix,3)==0)
				{	
					strcpy(iface,nvram_safe_get(strcat_r(word, "_ifname", tmp)));
					break;
				}	
			}	
		}		
	}
	free(ap_wifi_rl);
	if (out_buf && strlcpy(out_buf, iface, buf_len) == strlen(iface))
		return strdup(word);
	else
		return NULL;
}
#endif
#endif

int get_sta_count(int bssidx, int vifidx)
{
	unsigned char data[128];	//only pMacTab->Num is required
	char wlif_name[8];
	struct iwreq wrq;
	int sta_count = 0;

	__get_wlifname(bssidx, vifidx, wlif_name);

	if (is_intf_up(wlif_name) != 1) { //not up
		return sta_count;
	}

	memset(data, 0x00, sizeof(data));
	wrq.u.data.length = sizeof(data);
	wrq.u.data.pointer = (caddr_t) data;
	wrq.u.data.flags = 0;
	if (wl_ioctl(wlif_name, RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT, &wrq) < 0) {
		dbg("WI[%s] Access to MacTableStruct failure\n", wlif_name);
		return sta_count;
	}

	if(bssidx == 0) {
		RT_802_11_MAC_TABLE_2G* mp2=(RT_802_11_MAC_TABLE_2G*)wrq.u.data.pointer;
		sta_count = mp2->Num;
	} else {
		RT_802_11_MAC_TABLE_5G* mp =(RT_802_11_MAC_TABLE_5G*)wrq.u.data.pointer;
		sta_count = mp->Num;
	}
	//dbg("%s: sta_count(%d)\n", wlif_name, sta_count);
	return sta_count;
}

int get_all_sta_count(void)
{
	int sta_count, band, sband, max_sband;

	sta_count = 0;
	for (band = 0; band < MAX_NR_WL_IF; ++band) {
		SKIP_ABSENT_BAND(band);
		max_sband = num_of_mssid_support(band);
		for(sband = 0; sband <= max_sband; sband++) {
			sta_count += get_sta_count(band, sband);
		}
	}
	return sta_count;
}

#ifdef RTCONFIG_BCN_RPT
char *get_rax_bssid(char *ifname, char *bssid_buf, int buf_len)
{
	struct iwreq wrq;
	unsigned char ether_zero[ETH_ALEN] = { 0x00 };

	memset(bssid_buf, 0, buf_len);
	if (wl_ioctl(ifname, SIOCGIWAP, &wrq) < 0) {
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
	//dbg("%s: [%s] bssid(%s)\n", __func__, ifname, bssid_buf);

	return bssid_buf;
}

void update_wlxy_hwaddr(void)
{
	char word[8], *next;
	char tmp[100], prefix[] = "wlXX.YY_";
	char vif[8], hwaddr[18];
	int unit = 0, i;

	foreach (word, nvram_safe_get("wl_ifnames"), next) {
		for (i = 0; i < MAX_NO_MSSID; i++) {
			if (i)
				snprintf(prefix, sizeof(prefix), "wl%d.%d_", unit, i);
			else
				snprintf(prefix, sizeof(prefix), "wl%d_", unit);

			snprintf(vif, sizeof(vif), "%s", nvram_safe_get(strcat_r(prefix, "ifname", tmp)));
			if (strlen(vif)) {
				get_rax_bssid(vif, hwaddr, sizeof(hwaddr));
				nvram_set(strcat_r(prefix, "hwaddr", tmp), hwaddr);
			}
		}
		unit++;
	}
}
#endif /* RTCONFIG_BCN_RPT */
