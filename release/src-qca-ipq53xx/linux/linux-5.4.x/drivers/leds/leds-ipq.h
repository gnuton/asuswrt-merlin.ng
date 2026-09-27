/* Copyright (c) 2015-2016, 2020 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef LEDS_IPQ_H
#define LEDS_IPQ_H

#define NUM_LED_IN_REG	6
#define LED_WIDTH_BITS	5
#define SET_LED(led_num, led_src)  (led_src << ((led_num % NUM_LED_IN_REG) * \
					LED_WIDTH_BITS))
#define LED_MASK(led_num) (~(0x1f << (led_num % NUM_LED_IN_REG) * \
				LED_WIDTH_BITS))

enum led_source {
	WIFI_2G_0,
	WIFI_2G_1,
	WIFI_5G_0,
	WIFI_5G_1,
	LAN0_1000_LNK_ACTIVITY,
	LAN0_100_LNK_ACTIVITY,
	LAN0_10_LNK_ACTIVITY,
	LAN1_1000_LNK_ACTIVITY,
	LAN1_100_LNK_ACTIVITY,
	LAN1_10_LNK_ACTIVITY,
	LAN2_1000_LNK_ACTIVITY,
	LAN2_100_LNK_ACTIVITY,
	LAN2_10_LNK_ACTIVITY,
	LAN3_1000_LNK_ACTIVITY,
	LAN3_100_LNK_ACTIVITY,
	LAN3_10_LNK_ACTIVITY,
	WAN_1000_LNK_ACTIVITY,
	WAN_100_LNK_ACTIVITY,
	WAN_10_LNK_ACTIVITY,
	LEDC_ST_BLINK_0,
	LEDC_ST_BLINK_1,
	LEDC_ST_BLINK_2,
	LEDC_ST_BLINK_3
};

int ipq_led_source_select(int led_num, enum led_source src_type);
int ipq_led_set_blink(int led_idx,
		       unsigned long delay_on, unsigned long delay_off);
int ipq_led_set_brightness(int led_idx, unsigned int brightness);
#endif
