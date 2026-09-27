/* Copyright (c) 2014, 2020 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef MDSS_QPIC_H
#define MDSS_QPIC_H

#include <linux/list.h>

#include <linux/pinctrl/consumer.h>
#include "mdss_panel.h"
#include "mdss_qpic_panel.h"

#define QPIC_REG_QPIC_LCDC_CTRL				0x22000
#define QPIC_REG_LCDC_VERSION				0x22004
#define QPIC_REG_QPIC_LCDC_IRQ_EN			0x22008
#define QPIC_REG_QPIC_LCDC_IRQ_STTS			0x2200C
#define QPIC_REG_QPIC_LCDC_IRQ_CLR			0x22010
#define QPIC_REG_QPIC_LCDC_STTS				0x22014
#define QPIC_REG_QPIC_LCDC_CMD_DATA_CYCLE_CNT	0x22018
#define QPIC_REG_QPIC_LCDC_CFG0				0x22020
#define QPIC_REG_QPIC_LCDC_CFG1				0x22024
#define QPIC_REG_QPIC_LCDC_CFG2				0x22028
#define QPIC_REG_QPIC_LCDC_RESET			0x2202C
#define QPIC_REG_QPIC_LCDC_FIFO_SOF			0x22100
#define QPIC_REG_LCD_DEVICE_CMD0			0x23000
#define QPIC_REG_QPIC_LCDC_FIFO_DATA_PORT0	0x22140
#define QPIC_REG_QPIC_LCDC_FIFO_EOF			0x22180

#define QPIC_OUTP(off, data) \
	writel_relaxed((data), qpic_res->qpic_base + (off))
#define QPIC_OUTPW(off, data) \
	writew_relaxed((data), qpic_res->qpic_base + (off))
#define QPIC_INP(off) \
	readl_relaxed(qpic_res->qpic_base + (off))

#define QPIC_MAX_VSYNC_WAIT_TIME			500
#define QPIC_MAX_CMD_BUF_SIZE				512

#define QPIC_BAM_TIMEOUT				400

int mdss_qpic_init(void);
void mdss_qpic_set_cfg0(void);
int qpic_send_pkt(u32 cmd, u8 *param, u32 len);
u32 qpic_read_data(u32 cmd_index, u32 size);
int mdss_qpic_panel_on(struct mdss_panel_data *pdata,
	struct qpic_panel_io_desc *panel_io);
int mdss_qpic_panel_off(struct mdss_panel_data *pdata,
	struct qpic_panel_io_desc *panel_io);
int qpic_register_panel(struct mdss_panel_data *pdata);

struct qpic_data_type {
	u32 rev;
	struct platform_device *pdev;
	size_t qpic_reg_size;
	u32 qpic_phys;
	char __iomem *qpic_base;
	int irq;
	int bam_irq;
	u32 irq_ena;
	u32 res_init;
	void *fb_virt;
	u32 fb_phys;
	size_t fb_size;
	void *cmd_buf_virt;
	u32 cmd_buf_phys;
	u32 irq_requested;
	struct mdss_panel_data *panel_data;
	struct qpic_panel_io_desc panel_io;
	u32 bus_handle;
	unsigned long bam_timeout;
	struct completion fifo_eof_comp;
	struct completion completion;
	struct dma_chan *chan;
	struct clk *core_clk;
	struct clk *aon_clk;
};

u32 qpic_send_frame(
		u32 x_start,
		u32 y_start,
		u32 x_end,
		u32 y_end,
		u32 *data,
		u32 total_bytes);

u32 qpic_panel_get_framerate(void);

#endif /* MDSS_QPIC_H */
