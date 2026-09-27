/*
 * Copyright (c) 2015, 2020 The Linux Foundation. All rights reserved.
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
 */

#include <linux/module.h>
#include <linux/delay.h>

#include "mdss_qpic.h"
#include "mdss_qpic_panel.h"
#include "qpic_panel_er_ssd1963.h"

void ertft_off(struct qpic_panel_io_desc *qpic_panel_io)
{
	/* TODO : add shutdown related code if needed */
}

int ertft_on(struct qpic_panel_io_desc *qpic_panel_io)
{
	u8 param[8];

	/* Set PLL MN */
	param[0] = 0x23;
	param[1] = 0x02;
	param[2] = 0x54;
	qpic_send_pkt(OP_SSD1963_SET_PLL_MN, param, 3);

	/* Use PLL output as system clock */
	param[0] = 0x1;
	qpic_send_pkt(OP_SSD1963_SET_PLL, param, 1);
	msleep(120);

	/* Enable PLL */
	param[0] = 0x03;
	qpic_send_pkt(OP_SSD1963_SET_PLL, param, 1);
	msleep(120);

	/* Software Reset */
	qpic_send_pkt(OP_SOFT_RESET, NULL, 0);
	/* wait for 120 ms after reset as panel spec suggests */
	msleep(120);

	/* Set the LSHIFT (pixel clock) frequency */
	param[0] = 0x04;
	param[1] = 0x66;
	param[2] = 0x65;
	qpic_send_pkt(OP_SSD1963_SET_LSHIFT_FREQ, param, 3);
	msleep(20);

	/* Set the LCD panel mode and resolution */
	param[0] = 0x20;
	param[1] = 0x00;
	param[2] = 0x03;
	param[3] = 0x1f;
	param[4] = 0x01;
	param[5] = 0xdf;
	param[6] = 0x00;
	qpic_send_pkt(OP_SSD1963_SET_LCD_MODE, param, 7);
	msleep(20);

	/* Set front porch */
	param[0] = 0x04;
	param[1] = 0x1f;
	param[2] = 0x00;
	param[3] = 0xd2;
	param[4] = 0x00;
	param[5] = 0x00;
	param[6] = 0x00;
	param[7] = 0x00;
	qpic_send_pkt(OP_SSD1963_SET_HOR_PERIOD, param, 8);

	/* Set the vertical blanking interval between last scan line and
	 * next LFRAME pulse
	 */
	param[0] = 0x02;
	param[1] = 0x0c;
	param[2] = 0x00;
	param[3] = 0x22;
	param[4] = 0x00;
	param[5] = 0x00;
	param[6] = 0x00;
	qpic_send_pkt(OP_SSD1963_SET_VER_PERIOD, param, 7);
	msleep(20);

	/* Set GPIO config */
	param[0] = 0x0f;
	param[1] = 0x01;
	qpic_send_pkt(OP_SSD1963_SET_GPIO_CONFIG, param, 2);
	msleep(20);

	/* Set GPIO Value */
	param[0] = 0x1;
	qpic_send_pkt(OP_SSD1963_SET_GPIO_VAL, param, 1);

	/* set memory access control */
	param[0] = 0x08;
	qpic_send_pkt(OP_SET_ADDRESS_MODE, param, 1);
	/* wait for 20 ms after command sent as panel spec suggests */
	msleep(20);

	param[0] = 0x60;
	qpic_send_pkt(OP_SET_PIXEL_FORMAT, param, 1);

	/* Set the pixel data format of the parallel host processor
	 * interface
	 */
	param[0] = 0x06;
	qpic_send_pkt(OP_SSD1963_SET_PIXEL_DATA_IFACE, param, 1);
	/* wait for 20 ms after command sent as panel spec suggests */
	msleep(20);

	/* set post proc */
	param[0] = 0x40;
	param[1] = 0x80;
	param[2] = 0x40;
	param[3] = 0x01;
	qpic_send_pkt(OP_SSD1963_SET_POST_PROC, param, 4);
	/* wait for 20 ms after command sent */
	msleep(20);

	/* display on */
	qpic_send_pkt(OP_SET_DISPLAY_ON, NULL, 0);
	/* wait for 20 ms after command sent as panel spec suggests */
	msleep(20);

	param[0] = 0x06;
	param[1] = 0x80;
	param[2] = 0x01;
	param[3] = 0xf0;
	param[4] = 0x00;
	param[5] = 0x00;
	qpic_send_pkt(OP_SSD1963_SET_PWM_CONFIG, param, 6);
	/* wait for 20 ms after command sent as panel spec suggests */
	msleep(20);

	param[0] = 0x0d;
	qpic_send_pkt(OP_SSD1963_SET_DBC_CONFIG, param, 1);
	/* wait for 20 ms after command sent as panel spec suggests */
	msleep(20);

	param[0] = 0x00;
	qpic_send_pkt(OP_SSD1963_TEARING_EFFECT_LINE_ON, param, 1);

	param[0] = qpic_read_data(OP_GET_PIXEL_FORMAT, 1);
	pr_info("Pixel format =%x", param[0]);

	return 0;
}

static int __init mdss_qpic_ertft_panel_init(void)
{
	qpic_panel_on = ertft_on;
	qpic_panel_off = ertft_off;
	return 0;
}

static void __exit mdss_qpic_ertft_panel_exit(void)
{
	qpic_panel_on = NULL;
	qpic_panel_off = NULL;
}
module_init(mdss_qpic_ertft_panel_init);
module_exit(mdss_qpic_ertft_panel_exit);

MODULE_LICENSE("Dual BSD/GPL");
