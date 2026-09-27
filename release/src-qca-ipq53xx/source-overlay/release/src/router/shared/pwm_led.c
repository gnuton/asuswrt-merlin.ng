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
 *
 * Copyright 2014, ASUSTeK Inc.
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND ASUS GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 */
#include <shared.h>

#define GPIO2PWM_OFF		0	/* turn LED Off */
#define GPIO2PWM_STATIC		1	/* Static light */
#define GPIO2PWM_SB		2	/* Static light and brightness control */
#define GPIO2PWM_BREATHING	3	/* Breathing light  */
#define GPIO2PWM_BB		4	/* Breathing light and brightness control */
#if defined(RTCONFIG_GPIO2PWM)
static void set_gpio2pwm(int pidx);
#else
static inline void set_gpio2pwm(__attribute__ ((unused)) int pidx) { return; }
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(ary) (sizeof(ary) / sizeof((ary)[0]))
#endif

#define SYS_PWMCHIP	"/sys/class/pwm/pwmchip0"

#if defined(RTCONFIG_PWMX1_GPIOX3_RGBLED)
#if defined(RTAX59U)
#define PWMDEV_NO	(1)
#else
#error Define PWM device number!
#endif
#endif

#if defined(RTAX59U) || defined(PRTAX57_GO)
#define PWM_LOW_ACTIVE	"1"
#else
#define PWM_LOW_ACTIVE	"0"
#endif

struct pwm_conf {
	char type[16];
	char config[16];
};

struct gpio_mapping {
	unsigned int bitmask;
	char gpio_nv[16];
};

struct led_mapping {
	unsigned int bitmask;
	unsigned int led_id;
};

static void set_pwm(int pidx, unsigned int color)
{
	char pwmdev[sizeof(SYS_PWMCHIP"/pwmXXX")];
	int pwmX = -1, i = 0;
#if defined(RTCONFIG_PWMX1_GPIOX3_RGBLED)
	struct pwm_conf p[] = {
		{		    "0",       "0-0-0"},	/* on */
		{		    "3", "0xf-250-250"},	/* off:0.5s, on:0.5s */
		{		    "3", "0xf-125-375"},	/* off:0.25s, on:0.75s */
		{		    "3",   "0xf-63-63"},	/* off:0.125s, on:0.125s */
		{"1-128-"PWM_LOW_ACTIVE, "0x3-625-625"}		/* breathing */
	};
	struct gpio_mapping g[] = {
		{RGBLED_WLED, "xxx"}				/* fake */
	};

#elif defined(RTCONFIG_PWMX3_RGBLED)
	struct pwm_conf p[] = {
#if defined(RTCONFIG_AURA_RGBLED)
#if defined(RTCONFIG_SOC_MT7988D) \
 || defined(RTCONFIG_SOC_MT7988A) || defined(RTCONFIG_SOC_MT7987)
		{    "2-128-"PWM_LOW_ACTIVE,   "0x3-751-751"},	/* on */
		{"3-128,500-"PWM_LOW_ACTIVE,   "0x3-763-763"},	/* off:0.5s, on:0.5s */
		{    "1-128-"PWM_LOW_ACTIVE,   "0x3-939-939"},	/* breathing */
		{    "8-128-"PWM_LOW_ACTIVE, "0x3-1127-1127"},	/* AURA: water flow */
		{    "4-128-"PWM_LOW_ACTIVE,   "0x5-526-526"}	/* AURA: rainbow */
#endif /* RTCONFIG_SOC_MTxxxxx */

#else /* RTCONFIG_ZENWIFI_RGBLED */
#if defined(RTCONFIG_SOC_MT7981)
		{    "2-128-"PWM_LOW_ACTIVE, "0x3-192-192"},	/* on */
		{"3-128,500-"PWM_LOW_ACTIVE, "0x3-195-195"},	/* off:0.5s, on:0.5s */
		{"3-128,750-"PWM_LOW_ACTIVE, "0x3-195-195"},	/* off:0.25s, on:0.75s */
		{"3-128,500-"PWM_LOW_ACTIVE,   "0x3-49-49"},	/* off:0.125s, on:0.125s */
		{    "1-128-"PWM_LOW_ACTIVE, "0x3-240-240"}	/* breathing */
#elif defined(RTCONFIG_SOC_MT7988D) \
   || defined(RTCONFIG_SOC_MT7988A) || defined(RTCONFIG_SOC_MT7987)
		{    "2-128-"PWM_LOW_ACTIVE, "0x3-751-751"},	/* on */
		{"3-128,500-"PWM_LOW_ACTIVE, "0x3-763-763"},	/* off:0.5s, on:0.5s */
		{"3-128,750-"PWM_LOW_ACTIVE, "0x3-763-763"},	/* off:0.25s, on:0.75s */
		{"3-128,500-"PWM_LOW_ACTIVE, "0x3-191-191"},	/* off:0.125s, on:0.125s */
		{    "1-128-"PWM_LOW_ACTIVE, "0x3-939-939"}	/* breathing */
#else /* MT7986 */
		{    "2-128-"PWM_LOW_ACTIVE, "0x3-500-500"},	/* on */
		{"3-128,500-"PWM_LOW_ACTIVE, "0x3-508-508"},	/* off:0.5s, on:0.5s */
		{"3-128,750-"PWM_LOW_ACTIVE, "0x3-508-508"},	/* off:0.25s, on:0.75s */
		{"3-128,500-"PWM_LOW_ACTIVE, "0x3-127-127"},	/* off:0.125s, on:0.125s */
		{    "1-128-"PWM_LOW_ACTIVE, "0x3-625-625"}	/* breathing */
#endif /* RTCONFIG_SOC_MTxxxxx */
#endif /* !RTCONFIG_AURA_RGBLED */
	};
	struct gpio_mapping g[] = {
		{RGBLED_RLED, "led_red_gpio"},
		{RGBLED_GLED, "led_green_gpio"},
		{RGBLED_BLED, "led_blue_gpio"}
	};
	int rgb[3] = { -1, -1, -1 };
	int nm = nvram_get("ledg_night_mode") ? nvram_get_int("ledg_night_mode") : 0;
	int nmdr = nvram_get("ledg_night_mode_divide_ratio") ? nvram_get_int("ledg_night_mode_divide_ratio") : 10;

	/* for color mixing */
	if (nvram_invmatch("ledg_scheme", "")) {
		char nv[16], buf[64];
		char *ptr = buf, *str = NULL;
		int ledg_scheme = nvram_get_int("ledg_scheme");

		/* parse nvram ledg_rgbXXX */
		snprintf(nv, sizeof(nv), "ledg_rgb%d", ledg_scheme);
		strcpy(buf, nvram_safe_get(nv));
		while ((str = strsep(&ptr, ","))) {
			rgb[i] = atoi(str);
			/* for night mode, reduce brightness */
			if (nm)
				rgb[i] = rgb[i] / nmdr;

			if (++i > ARRAY_SIZE(rgb))
				break;
		}
	}
#endif /* !RTCONFIG_PWMX1_GPIOX3_RGBLED */

	for (i = 0; i < ARRAY_SIZE(g); i++) {
#if defined(RTCONFIG_PWMX1_GPIOX3_RGBLED)
		pwmX = PWMDEV_NO;
#elif defined(RTCONFIG_PWMX3_RGBLED)
		pwmX = (nvram_get_int(g[i].gpio_nv) & 0xff) - GPIO_PWM_DEFSHIFT;
		if (pwmX < 0)
			continue;
#endif

		snprintf(pwmdev, sizeof(pwmdev), "%s/pwm%d", SYS_PWMCHIP, pwmX);
		if (!d_exists(pwmdev))
			doSystem("echo %d > %s/export", pwmX, SYS_PWMCHIP);
		doSystem("echo 0 > %s/mm_enable", pwmdev);
#if defined(RTCONFIG_PWMX1_GPIOX3_RGBLED)
		if (!strcmp(p[pidx].type, "0"))
			continue;
		else
#elif defined(RTCONFIG_PWMX3_RGBLED)
		/* turn off LED */
		if (!(g[i].bitmask & color)) {
			/* for high-active */
			if (strstr(PWM_LOW_ACTIVE, "0"))
				continue;
			/* for low-active */
			else {
				doSystem("echo 2-0-1 > %s/mm_type", pwmdev);
				doSystem("echo 0x3-500-500 > %s/mm_config", pwmdev);
			}
		}
		/* for color mixing */
		else if (rgb[i] >= 0 && rgb[i] < 128
#if defined(RTCONFIG_AURA_RGBLED)
		      && nvram_get_int("x_Setting")
		      && nvram_get("rgbled_qis_running") == NULL
#endif
		) {
			char buf[16];
			char *sptr = p[pidx].type;
			char *dptr = buf;

			/* re-generate p[pidx].type content */
			memset(buf, 0, sizeof(buf));
			strncpy(dptr, sptr, 2);
			dptr += 2;
			sptr += 5;
			dptr += sprintf(dptr, "%d", rgb[i]);
			if (*sptr != '\0')
				strncpy(dptr, sptr, strlen(sptr));

			doSystem("echo %s > %s/mm_type", buf, pwmdev);
			doSystem("echo %s > %s/mm_config", p[pidx].config, pwmdev);
		}
		else
#endif
		{
			doSystem("echo %s > %s/mm_type", p[pidx].type, pwmdev);
			doSystem("echo %s > %s/mm_config", p[pidx].config, pwmdev);
		}
		doSystem("echo 1 > %s/mm_enable", pwmdev);
	}
}

static void set_color(unsigned int color)
{
#if defined(RTCONFIG_PWMX1_GPIOX3_RGBLED)
	struct led_mapping l[] = {
		{RGBLED_BLED, LED_BLUE},
		{RGBLED_GLED, LED_GREEN},
		{RGBLED_RLED, LED_RED},
		{RGBLED_WLED, LED_WHITE}
	};
#elif defined(RTCONFIG_PWMX3_RGBLED)
	struct led_mapping l[] = {
		{RGBLED_WLED, LED_WHITE}
	};
#endif
	int i;

	for (i = 0; i < ARRAY_SIZE(l); i++) {
		if (l[i].bitmask & color)
			led_control(l[i].led_id, LED_ON);
		else
			led_control(l[i].led_id, LED_OFF);
	}
}

void set_rgbled(unsigned int mode)
{
#if defined(RTCONFIG_AURA_RGBLED)
	int gpio2pwm_pidx = GPIO2PWM_STATIC;

	switch (mode) {
	/* start ATE mode */
	case RGBLED_ATE_MODE:
		/* disable color mixing */
		nvram_set("rgbled_qis_running", "1");

		set_pwm(0, RGBLED_BLED | RGBLED_GLED | RGBLED_RLED);
		set_gpio2pwm(GPIO2PWM_STATIC);
		break;
	/* static Water Blue */
	case RGBLED_DEFAULT_STANDBY:
		if (nvram_match("rgbled_qis_running", "1"))
			break;
		set_pwm(0, RGBLED_BLED | RGBLED_GLED);
		break;
	/* rainbow */
	case RGBLED_QIS_RUN:
		nvram_set("rgbled_qis_running", "1");
		set_pwm(4, RGBLED_BLED | RGBLED_GLED | RGBLED_RLED);
		break;
	/* blinking Water Blue on/off 0.5s */
	case RGBLED_QIS_FINISH:
		set_pwm(1, RGBLED_BLED | RGBLED_GLED);
		sleep(3);
		nvram_unset("rgbled_qis_running");
		/* fall through */
	/* AURA */
	case RGBLED_CONFIGURED_STANDBY:
		if (nvram_invmatch("ledg_scheme", "")) {
			unsigned int c = RGBLED_BLED | RGBLED_GLED | RGBLED_RLED;
			int ledg_scheme = nvram_get_int("ledg_scheme");
			int pidx = 0;

			switch (ledg_scheme) {
			/* static */
			case LEDG_SCHEME_STEADY_RED:
				pidx = 0;
				break;
			/* breathing */
			case LEDG_SCHEME_PULSATING:
				pidx = 2;
				gpio2pwm_pidx = GPIO2PWM_BREATHING;
				break;
			/* water flow */
			case LEDG_SCHEME_WATER_FLOW:
				pidx = 3;
				break;
			/* rainbow */
			case LEDG_SCHEME_RAINBOW:
				pidx = 4;
				break;
			/* disable Aura RGB */
			case LEDG_SCHEME_OFF:
				c = 0;
				gpio2pwm_pidx = GPIO2PWM_OFF;
				break;
			default:
				;
			}

			if (nvram_match("AllLED", "0")) {
				c = 0;
				gpio2pwm_pidx = GPIO2PWM_OFF;
			}

			set_pwm(pidx, c);
			set_gpio2pwm(gpio2pwm_pidx);
		}
		break;
	default:
		;
	}

#else /* RTCONFIG_ZENWIFI_RGBLED */
	unsigned int cmask = RGBLED_COLOR_MESK, bmask = RGBLED_BLINK_MESK;
	unsigned int c = mode & cmask;
	unsigned int b = mode & bmask;
	int pidx = 0;
#if defined(RTCONFIG_SW_BTN)
 	int btn_mode=-1;
        btn_mode=nvram_get_int("btnsw_onoff");
	if((btn_mode == 2) && nvram_match("AllLED", "0"))
		c=0; //OFF
#endif
	if ((c == RGBLED_CONNECTED || c == RGBLED_ETH_BACKHAUL)
	  && b == 0
	  && nvram_match("AllLED", "0")
	)
		c = 0;




	switch (b) {
	case RGBLED_ATE_MODE:
		pidx = 0;
		break;
	case RGBLED_SBLINK:
		pidx = 1;
		break;
	case RGBLED_3ON1OFF:
		pidx = 2;
		break;
	case RGBLED_FBLINK:
		pidx = 3;
		break;
	case RGBLED_BREATHING:
		pidx = 4;
		break;
	default:
		;
	}

	if (b != RGBLED_ATE_MODE)
		set_color(c);
	set_pwm(pidx, c);
#endif /* !RTCONFIG_AURA_RGBLED */
}

#if defined(RTCONFIG_GPIO2PWM)
/*
 * Use pwmX interrupt to control GPIO to produce PWM effect
 */

#if defined(GS7)
#define GPIO2PWM_PWMDEV_NO	(0)
#define GPIO2PWM_LOW_ACTIVE	"0"
#else
#error Define GPIO2PWM_PWMDEV_NO for GPIO2PWM function!
#define GPIO2PWM_LOW_ACTIVE	"0"
#endif

struct gpio2pwm_conf {
	char type[16];
	char fint[8];
	char config[16];
};

static void set_gpio2pwm(int pidx)
{
	char pwmdev[sizeof(SYS_PWMCHIP"/pwmXXX")];
	struct gpio2pwm_conf p[] = {
#if defined(GS7)
		   [GPIO2PWM_STATIC] = { "6-128,128-"GPIO2PWM_LOW_ACTIVE, "2-1", "0x0-30-30" },	/* static */
		       [GPIO2PWM_SB] = { "6-12,12-"GPIO2PWM_LOW_ACTIVE,   "2-1", "0x0-30-30" },	/* static (night mode) */
		[GPIO2PWM_BREATHING] = { "5-128,128-"GPIO2PWM_LOW_ACTIVE, "2-1", "0x0-60-60" },	/* breathing */
		       [GPIO2PWM_BB] = { "5-12,12-"GPIO2PWM_LOW_ACTIVE,   "2-1", "0x0-60-60" },	/* breathing (night mode) */
#else
#error Define gpio2pwm_conf for GPIO2PWM function!
#endif
	};
	int nm = nvram_get("ledg_night_mode") ? nvram_get_int("ledg_night_mode") : 0;

#if defined(GS7)
	if (!nvram_match("HwId", "B"))
		return;
#endif

	snprintf(pwmdev, sizeof(pwmdev), "%s/pwm%d", SYS_PWMCHIP, GPIO2PWM_PWMDEV_NO);
	if (!d_exists(pwmdev))
		doSystem("echo %d > %s/export", GPIO2PWM_PWMDEV_NO, SYS_PWMCHIP);
	doSystem("echo 0 > %s/mm_enable", pwmdev);

	/* for night mode, reduce brightness */
	if (nm && pidx > 0)
		pidx++;

	switch (pidx) {
	/* only LED off */
	case 0:
		set_color(0x0);
		return;
	/* only LED on */
	case 1:
		set_color(0xf);
		return;
	/* brightness control */
	default:
		;
	}

	doSystem("echo %s > %s/mm_type", p[pidx].type, pwmdev);
	doSystem("echo %s > %s/mm_fint", p[pidx].fint, pwmdev);
	doSystem("echo %s > %s/mm_config", p[pidx].config, pwmdev);
	doSystem("echo 1 > %s/mm_enable", pwmdev);
}
#endif /* RTCONFIG_GPIO2PWM */
