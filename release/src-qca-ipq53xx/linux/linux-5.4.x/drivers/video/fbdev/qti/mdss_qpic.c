/* Copyright (c) 2013-2014, 2020 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>

#include "mdss_fb.h"
#include "mdss_qpic.h"
#include "mdss_qpic_panel.h"

static int mdss_qpic_probe(struct platform_device *pdev);
static int mdss_qpic_remove(struct platform_device *pdev);
static void qpic_interrupt_en(u32 en);

struct qpic_data_type *qpic_res;

/* for debugging */
static u32 use_bam = true;
static u32 use_irq = true;
static u32 use_vsync = true;

static const struct of_device_id mdss_qpic_dt_match[] = {
	{ .compatible = "qti,mdss_qpic",},
	{}
};
MODULE_DEVICE_TABLE(of, mdss_qpic_dt_match);

static struct platform_driver mdss_qpic_driver = {
	.probe = mdss_qpic_probe,
	.remove = mdss_qpic_remove,
	.shutdown = NULL,
	.driver = {
		/*
		 * Simulate mdp hw
		 */
		.name = "mdp",
		.of_match_table = mdss_qpic_dt_match,
	},
};

int qpic_on(struct msm_fb_data_type *mfd)
{
	int ret;
	ret = mdss_qpic_panel_on(qpic_res->panel_data, &qpic_res->panel_io);
	return ret;
}

int qpic_off(struct msm_fb_data_type *mfd)
{
	int ret;
	ret = mdss_qpic_panel_off(qpic_res->panel_data, &qpic_res->panel_io);
	if (use_irq)
		qpic_interrupt_en(false);
	return ret;
}


static void mdss_qpic_pan_display(struct msm_fb_data_type *mfd)
{

	struct fb_info *fbi;
	u32 offset, fb_offset, size, data;
	int bpp;

	if (!mfd) {
		pr_err("%s: mfd is NULL!", __func__);
		return;
	}

	fbi = mfd->fbi;

	bpp = fbi->var.bits_per_pixel / 8;
	offset = fbi->var.xoffset * bpp +
		 fbi->var.yoffset * fbi->fix.line_length;

	if (offset > fbi->fix.smem_len) {
		pr_err("invalid fb offset=%u total length=%u\n",
		       (unsigned int)offset, fbi->fix.smem_len);
		return;
	}
	if (use_bam)
		fb_offset = (u32)fbi->fix.smem_start + offset;
	else
		fb_offset = (u32)(uintptr_t)(mfd->fbi->screen_base + offset);

	mdss_qpic_panel_on(qpic_res->panel_data, &qpic_res->panel_io);
	size = fbi->var.xres * fbi->var.yres * bpp;

	if (size < (1024 * 1024)) {
		qpic_send_frame(0, 0, fbi->var.xres - 1, fbi->var.yres - 1,
		(u32 *)((uintptr_t)fb_offset), size);
	} else {
		/* LCDC BAM can't trnasfer more than 1MB */
		qpic_send_frame(0, 0, fbi->var.xres - 1, fbi->var.yres - 1,
		(u32 *)((uintptr_t)fb_offset), size / 2);

		/* Disable vsync interrupt for second packet */
		if (use_vsync) {
			data = QPIC_INP(QPIC_REG_QPIC_LCDC_CTRL);
			data &= ~(1 << 0);
			QPIC_OUTP(QPIC_REG_QPIC_LCDC_CTRL, data);
		}

		qpic_send_frame(0,  fbi->var.yres / 2, fbi->var.xres - 1,
			 fbi->var.yres - 1, (u32 *)((uintptr_t)(fb_offset +
(size / 2))), size / 2);

		/* Enable vsync interrupt after sending second packet */
		if (use_vsync) {
			data = QPIC_INP(QPIC_REG_QPIC_LCDC_CTRL);
			data |= (1 << 0);
			QPIC_OUTP(QPIC_REG_QPIC_LCDC_CTRL, data);
		}
	}
}

static void qpic_bam_cb(void *param)
{
	complete(&qpic_res->completion);
}

void mdss_qpic_dma_pages_ops(dma_addr_t base, size_t size,
			     void page_ops(struct page *page))
{
	dma_addr_t cur;
	struct page *page;

	for (cur = base; cur < base + size; cur += PAGE_SIZE) {
		page = pfn_to_page(cur >> PAGE_SHIFT);
		page_ops(page);
	}
}

int mdss_qpic_alloc_fb_mem(struct msm_fb_data_type *mfd)
{
	size_t size;
	u32 yres = mfd->fbi->var.yres_virtual;

	size = PAGE_ALIGN(mfd->fbi->fix.line_length * yres);

	if (!qpic_res->res_init)
		return -EINVAL;

	if (mfd->index != 0) {
		mfd->fbi->fix.smem_start = 0;
		mfd->fbi->screen_base = NULL;
		mfd->fbi->fix.smem_len = 0;
		return 0;
	}

	if (!qpic_res->cmd_buf_virt) {
		qpic_res->cmd_buf_virt = dma_alloc_wc(
			&qpic_res->pdev->dev, QPIC_MAX_CMD_BUF_SIZE,
			(dma_addr_t *)&qpic_res->cmd_buf_phys, GFP_KERNEL);
		if (!qpic_res->cmd_buf_virt) {
			pr_err("%s cmd buf allocation failed", __func__);
			return -ENOMEM;
		}

		pr_debug("%s cmd_buf virt=%p phys=%x", __func__,
			qpic_res->cmd_buf_virt,
			qpic_res->cmd_buf_phys);
	}

	if (!qpic_res->fb_virt) {
		qpic_res->fb_virt = (void *)dmam_alloc_coherent(
						&qpic_res->pdev->dev,
						size,
						(dma_addr_t *)&qpic_res->fb_phys,
						GFP_KERNEL);
		if (!qpic_res->fb_virt) {
			pr_err("%s fb allocation failed", __func__);
			dma_free_wc(&qpic_res->pdev->dev,
					      QPIC_MAX_CMD_BUF_SIZE,
					      qpic_res->cmd_buf_virt,
					      qpic_res->cmd_buf_phys);
			qpic_res->cmd_buf_virt = NULL;
			return -ENOMEM;
		}

		pr_debug("%s size=%d vir_addr=%p phys_addr=0x%x",
			__func__, (int)size, qpic_res->fb_virt,
			qpic_res->fb_phys);

		qpic_res->fb_size = size;
		mdss_qpic_dma_pages_ops(qpic_res->fb_phys, size, get_page);
	}

	mfd->fbi->fix.smem_start = qpic_res->fb_phys;
	mfd->fbi->screen_base = qpic_res->fb_virt;
	mfd->fbi->fix.smem_len = size;

	return 0;
}

u32 mdss_qpic_fb_stride(u32 fb_index, u32 xres, int bpp)
{
	return xres * bpp;
}

int mdss_qpic_overlay_init(struct msm_fb_data_type *mfd)
{
	struct msm_mdp_interface *qpic_interface = &mfd->mdp;
	qpic_interface->on_fnc = qpic_on;
	qpic_interface->off_fnc = qpic_off;
	qpic_interface->cursor_update = NULL;
	qpic_interface->dma_fnc = mdss_qpic_pan_display;
	qpic_interface->ioctl_handler = NULL;
	return 0;
}

int qpic_register_panel(struct mdss_panel_data *pdata)
{
	struct platform_device *mdss_fb_dev = NULL;
	int rc;

	mdss_fb_dev = platform_device_alloc("mdss_fb", pdata->panel_info.pdest);
	if (!mdss_fb_dev) {
		pr_err("unable to allocate mdss_fb device\n");
		return -ENOMEM;
	}

	mdss_fb_dev->dev.platform_data = pdata;

	rc = platform_device_add(mdss_fb_dev);
	if (rc) {
		platform_device_put(mdss_fb_dev);
		pr_err("unable to probe mdss_fb device (%d)\n", rc);
		return rc;
	}

	qpic_res->panel_data = pdata;

	return rc;
}

void mdss_qpic_set_cfg0(void)
{
	/*
	 * RD_CS_HOLD=1,RD_ACTIVE=8,CS_WR_RD_SETUP=0,
	 * WR_CS_HOLD,WR_ACTIVE,ADDR_CS_SETUP=0,
	 * CMD_BUS_ALIGNMENT=0.
	 */
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_CFG0, 0x2000101);
}

void mdss_qpic_reset(void)
{
	u32 time_end;

	QPIC_OUTP(QPIC_REG_QPIC_LCDC_RESET, 1 << 0);
	/* wait 100 us after reset as suggested by hw */
	usleep_range(100, 100);
	time_end = (u32)ktime_to_ms(ktime_get()) +
		QPIC_MAX_VSYNC_WAIT_TIME;
	while (((QPIC_INP(QPIC_REG_QPIC_LCDC_STTS) & (1 << 8)) == 0)) {
		if ((u32)ktime_to_ms(ktime_get()) > time_end) {
			pr_err("%s reset not finished", __func__);
			break;
		}
		/* yield 100 us for next polling by experiment*/
		usleep_range(100, 100);
	}
}

static void qpic_interrupt_en(u32 en)
{
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_IRQ_CLR, 0xff);
	if (en) {
		if (!qpic_res->irq_ena) {
			init_completion(&qpic_res->fifo_eof_comp);
			qpic_res->irq_ena = true;
			enable_irq(qpic_res->irq);
		}
	} else {
		QPIC_OUTP(QPIC_REG_QPIC_LCDC_IRQ_EN, 0);
		disable_irq(qpic_res->irq);
		qpic_res->irq_ena = false;
	}
}

static irqreturn_t qpic_irq_handler(int irq, void *ptr)
{
	u32 data;
	data = QPIC_INP(QPIC_REG_QPIC_LCDC_IRQ_STTS);
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_IRQ_CLR, 0xff);
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_IRQ_EN, 0);

	if (data & ((1 << 2) | (1 << 4)))
		complete(&qpic_res->fifo_eof_comp);
	return IRQ_HANDLED;
}

static int qpic_send_pkt_bam(u32 cmd, u32 len, u8 *param)
{
	int ret = 0;
	struct dma_async_tx_descriptor *dma_desc;
	dma_cookie_t cookie = 0;

	u32 cfg2;
	dma_addr_t phys_addr;

	if ((cmd != OP_WRITE_MEMORY_START) &&
		(cmd != OP_WRITE_MEMORY_CONTINUE)) {
		memcpy((u8 *)qpic_res->cmd_buf_virt, param, len);
		phys_addr = qpic_res->cmd_buf_phys;
	} else {
		phys_addr = (dma_addr_t)param;
	}
	cfg2 = QPIC_INP(QPIC_REG_QPIC_LCDC_CFG2);
	cfg2 &= ~0xFF;
	cfg2 |= cmd;
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_CFG2, cfg2);

	dma_desc = dmaengine_prep_slave_single(qpic_res->chan, phys_addr, len,
				DMA_MEM_TO_DEV, DMA_PREP_INTERRUPT);
	if (!dma_desc) {
		pr_err("failed to prepare dma desc for command %x\n",
				cmd);
		return -EINVAL;
	}

	dma_desc->callback = qpic_bam_cb;
	dma_desc->callback_param = qpic_res;

	cookie = dmaengine_submit(dma_desc);
	if (dma_submit_error(cookie)) {
		pr_err("failed to submit command %x\n",
			cmd);
		ret = -EINVAL;
	}

	reinit_completion(&qpic_res->completion);
	dma_async_issue_pending(qpic_res->chan);

	ret = wait_for_completion_timeout(
		&qpic_res->completion,
		qpic_res->bam_timeout);

	if (ret <= 0)
		pr_err("%s timeout %x", __func__, ret);
	else
		ret = 0;

	return ret;
}

void qpic_dump_reg(void)
{
	pr_info("%s\n", __func__);
	pr_info("QPIC_REG_QPIC_LCDC_CTRL = %x\n",
		QPIC_INP(QPIC_REG_QPIC_LCDC_CTRL));
	pr_info("QPIC_REG_QPIC_LCDC_CMD_DATA_CYCLE_CNT = %x\n",
		QPIC_INP(QPIC_REG_QPIC_LCDC_CMD_DATA_CYCLE_CNT));
	pr_info("QPIC_REG_QPIC_LCDC_CFG0 = %x\n",
		QPIC_INP(QPIC_REG_QPIC_LCDC_CFG0));
	pr_info("QPIC_REG_QPIC_LCDC_CFG1 = %x\n",
		QPIC_INP(QPIC_REG_QPIC_LCDC_CFG1));
	pr_info("QPIC_REG_QPIC_LCDC_CFG2 = %x\n",
		QPIC_INP(QPIC_REG_QPIC_LCDC_CFG2));
	pr_info("QPIC_REG_QPIC_LCDC_IRQ_EN = %x\n",
		QPIC_INP(QPIC_REG_QPIC_LCDC_IRQ_EN));
	pr_info("QPIC_REG_QPIC_LCDC_IRQ_STTS = %x\n",
		QPIC_INP(QPIC_REG_QPIC_LCDC_IRQ_STTS));
	pr_info("QPIC_REG_QPIC_LCDC_STTS = %x\n",
		QPIC_INP(QPIC_REG_QPIC_LCDC_STTS));
	pr_info("QPIC_REG_QPIC_LCDC_FIFO_SOF = %x\n",
		QPIC_INP(QPIC_REG_QPIC_LCDC_FIFO_SOF));
}

static int qpic_wait_for_fifo(void)
{
	u32 data, time_end;
	int ret = 0;

	if (use_irq) {
		data = QPIC_INP(QPIC_REG_QPIC_LCDC_STTS);
		data &= 0x3F;
		if (data == 0)
			return ret;
		reinit_completion(&qpic_res->fifo_eof_comp);
		QPIC_OUTP(QPIC_REG_QPIC_LCDC_IRQ_EN, (1 << 4));
		ret = wait_for_completion_timeout(&qpic_res->fifo_eof_comp,
				msecs_to_jiffies(QPIC_MAX_VSYNC_WAIT_TIME));
		if (ret > 0) {
			ret = 0;
		} else {
			pr_err("%s timeout %x\n", __func__, ret);
			ret = -ETIMEDOUT;
		}
		QPIC_OUTP(QPIC_REG_QPIC_LCDC_IRQ_EN, 0);
	} else {
		time_end = (u32)ktime_to_ms(ktime_get()) +
			QPIC_MAX_VSYNC_WAIT_TIME;
		while (1) {
			data = QPIC_INP(QPIC_REG_QPIC_LCDC_STTS);
			data &= 0x3F;
			if (data == 0)
				break;
			/* yield 10 us for next polling by experiment*/
			usleep_range(10, 10);
			if (ktime_to_ms(ktime_get()) > time_end) {
				pr_err("%s time out", __func__);
				ret = -EBUSY;
				break;
			}
		}
	}
	return ret;
}

static int qpic_wait_for_eof(void)
{
	u32 data, time_end;
	int ret = 0;
	if (use_irq) {
		data = QPIC_INP(QPIC_REG_QPIC_LCDC_IRQ_STTS);
		if (data & (1 << 2))
			return ret;
		reinit_completion(&qpic_res->fifo_eof_comp);
		QPIC_OUTP(QPIC_REG_QPIC_LCDC_IRQ_EN, (1 << 2));
		ret = wait_for_completion_timeout(&qpic_res->fifo_eof_comp,
				msecs_to_jiffies(QPIC_MAX_VSYNC_WAIT_TIME));
		if (ret > 0) {
			ret = 0;
		} else {
			pr_err("%s timeout %x\n", __func__, ret);
			ret = -ETIMEDOUT;
		}
		QPIC_OUTP(QPIC_REG_QPIC_LCDC_IRQ_EN, 0);
	} else {
		time_end = (u32)ktime_to_ms(ktime_get()) +
			QPIC_MAX_VSYNC_WAIT_TIME;
		while (1) {
			data = QPIC_INP(QPIC_REG_QPIC_LCDC_IRQ_STTS);
			if (data & (1 << 2))
				break;
			/* yield 10 us for next polling by experiment*/
			usleep_range(10, 10);
			if (ktime_to_ms(ktime_get()) > time_end) {
				pr_err("%s wait for eof time out\n", __func__);
				qpic_dump_reg();
				ret = -EBUSY;
				break;
			}
		}
	}
	return ret;
}

static int qpic_send_pkt_sw(u32 cmd, u32 len, u8 *param)
{
	u32 bytes_left, data, cfg2;
	int i, ret = 0;
	if (len <= 4) {
		data = 0;
		if (param) {
			for (i = 0; i < len; i++)
				data |= (u32)param[i] << (8 * i);
		}
		QPIC_OUTP(QPIC_REG_QPIC_LCDC_CMD_DATA_CYCLE_CNT, len);
		QPIC_OUTP(QPIC_REG_LCD_DEVICE_CMD0 + (4 * cmd), data);
		return 0;
	}

	if ((len & 0x1) != 0) {
		pr_debug("%s: number of bytes needs be even", __func__);
		len = (len + 1) & (~0x1);
	}
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_IRQ_CLR, 0xff);
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_CMD_DATA_CYCLE_CNT, 0);
	cfg2 = QPIC_INP(QPIC_REG_QPIC_LCDC_CFG2);
	if ((cmd != OP_WRITE_MEMORY_START) &&
		(cmd != OP_WRITE_MEMORY_CONTINUE))
		cfg2 |= (1 << 24); /* transparent mode */
	else
		cfg2 &= ~(1 << 24);

	cfg2 &= ~0xFF;
	cfg2 |= cmd;
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_CFG2, cfg2);
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_FIFO_SOF, 0x0);
	bytes_left = len;

	i = 0;
	while (bytes_left > 0) {
		ret = qpic_wait_for_fifo();
		if (ret)
			goto exit_send_cmd_sw;

		data = (u32)param[i];

		QPIC_OUTP(QPIC_REG_QPIC_LCDC_FIFO_DATA_PORT0, data);
		bytes_left--;
		i++;
	}
	/* finished */
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_FIFO_EOF, 0x0);
	ret = qpic_wait_for_eof();
exit_send_cmd_sw:
	cfg2 &= ~(1 << 24);
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_CFG2, cfg2);
	return ret;
}

int qpic_send_pkt(u32 cmd, u8 *param, u32 len)
{
	if (!use_bam || ((cmd != OP_WRITE_MEMORY_CONTINUE) &&
		(cmd != OP_WRITE_MEMORY_START)))
		return qpic_send_pkt_sw(cmd, len, param);
	else
		return qpic_send_pkt_bam(cmd, len, param);
}
EXPORT_SYMBOL(qpic_send_pkt);

int mdss_qpic_init(void)
{
	int ret = 0;
	u32 data;
	mdss_qpic_reset();

	pr_info("%s version=%x", __func__, QPIC_INP(QPIC_REG_LCDC_VERSION));
	data = QPIC_INP(QPIC_REG_QPIC_LCDC_CTRL);
	/* clear vsync wait , bam mode = 0*/
	data &= ~(3 << 0);
	data &= ~(0x1f << 3);
	data |= (1 << 3); /* threshold */
	data |= (1 << 8); /* lcd_en */
	data &= ~(0x1f << 9);
	data |= (1 << 9); /* threshold */
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_CTRL, data);

	if (use_irq && (!qpic_res->irq_requested)) {
		ret = devm_request_irq(&qpic_res->pdev->dev,
			qpic_res->irq, qpic_irq_handler,
			0,	"QPIC", qpic_res);
		if (ret) {
			pr_err("qpic request_irq() failed!\n");
			use_irq = false;
		} else {
			disable_irq(qpic_res->irq);
		}
		qpic_res->irq_requested = true;
	}

	qpic_interrupt_en(use_irq);

	data = QPIC_INP(QPIC_REG_QPIC_LCDC_CFG2);
	data &= ~(0xFFF);
	data |= 0x200; /* XRGB */
	data |= 0x2C;
	QPIC_OUTP(QPIC_REG_QPIC_LCDC_CFG2, data);

	if (use_bam) {
		if (!qpic_res->chan) {
			qpic_res->chan = dma_request_slave_channel(
						&qpic_res->pdev->dev, "chan");
			if (!qpic_res->chan) {
				dev_err(&qpic_res->pdev->dev,
						"failed to request channel\n");
				return -ENODEV;
			}
		}

		data = QPIC_INP(QPIC_REG_QPIC_LCDC_CTRL);
		data |= (1 << 1);
		QPIC_OUTP(QPIC_REG_QPIC_LCDC_CTRL, data);
	}

	/* TE enable */
	if (use_vsync) {
		data = QPIC_INP(QPIC_REG_QPIC_LCDC_CTRL);
		data |= (1 << 0);
		QPIC_OUTP(QPIC_REG_QPIC_LCDC_CTRL, data);
	}

	return ret;
}

u32 qpic_read_data(u32 cmd_index, u32 size)
{
	u32 data = 0;
	if (size <= 4) {
		QPIC_OUTP(QPIC_REG_QPIC_LCDC_CMD_DATA_CYCLE_CNT, size);
		data = QPIC_INP(QPIC_REG_LCD_DEVICE_CMD0 + (cmd_index * 4));
	}
	return data;
}
EXPORT_SYMBOL(qpic_read_data);

static int mdss_qpic_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int rc = 0;
	u32 bam_timeout;
	static struct msm_mdp_interface qpic_interface = {
		.init_fnc = mdss_qpic_overlay_init,
		.fb_mem_alloc_fnc = mdss_qpic_alloc_fb_mem,
		.fb_stride = mdss_qpic_fb_stride,
	};


	if (!pdev->dev.of_node) {
		pr_err("qpic driver only supports device tree probe\n");
		return -ENOTSUPP;
	}

	if (!qpic_res)
		qpic_res = devm_kzalloc(&pdev->dev,
			sizeof(*qpic_res), GFP_KERNEL);

	if (!qpic_res)
		return -ENOMEM;

	if (qpic_res->res_init) {
		pr_err("qpic already initialized\n");
		return -EINVAL;
	}

	qpic_res->core_clk = devm_clk_get(dev, "core");
	if (IS_ERR(qpic_res->core_clk))
		return PTR_ERR(qpic_res->core_clk);

	qpic_res->aon_clk = devm_clk_get(dev, "aon");
	if (IS_ERR(qpic_res->aon_clk))
		return PTR_ERR(qpic_res->aon_clk);

	rc = clk_prepare_enable(qpic_res->core_clk);
	if (rc)
		return rc;

	rc = clk_prepare_enable(qpic_res->aon_clk);
	if (rc)
		goto err_aon_clk;

	pdev->id = 0;

	qpic_res->pdev = pdev;
	platform_set_drvdata(pdev, qpic_res);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("unable to get QPIC reg base address\n");
		rc = -ENOMEM;
		goto clk_disable;
	}

	qpic_res->qpic_reg_size = resource_size(res);
	qpic_res->qpic_base = devm_ioremap(&pdev->dev, res->start,
					qpic_res->qpic_reg_size);
	if (unlikely(!qpic_res->qpic_base)) {
		pr_err("unable to map MDSS QPIC base\n");
		rc = -ENOMEM;
		goto clk_disable;
	}
	qpic_res->qpic_phys = res->start;
	pr_info("MDSS QPIC HW Base phy_Address=0x%x virt=%p\n",
		(int) res->start,
		(void *)qpic_res->qpic_base);

	qpic_res->irq = platform_get_irq(pdev, 0);
	if (qpic_res->irq < 0) {
		dev_warn(&pdev->dev, "missing 'lcdc_irq' resource entry");
		rc = -EINVAL;
		goto clk_disable;
	}

	if (of_property_read_u32(pdev->dev.of_node,
				 "qti,bam_timeout", &bam_timeout))
		bam_timeout = QPIC_BAM_TIMEOUT;

	qpic_res->bam_timeout = msecs_to_jiffies(bam_timeout);

	qpic_res->res_init = true;
	init_completion(&qpic_res->completion);

	rc = mdss_fb_register_mdp_instance(&qpic_interface);
	if (rc) {
		pr_err("unable to register QPIC instance\n");
		goto clk_disable;
	}

	return 0;

clk_disable:
	clk_disable_unprepare(qpic_res->aon_clk);
err_aon_clk:
	clk_disable_unprepare(qpic_res->core_clk);

	return rc;
}

static int mdss_qpic_remove(struct platform_device *pdev)
{
	if (qpic_res->chan)
		dma_release_channel(qpic_res->chan);

	if (qpic_res->fb_virt)
		mdss_qpic_dma_pages_ops(qpic_res->fb_phys,
					qpic_res->fb_size, put_page);

	if (!qpic_res->cmd_buf_virt)
		dma_free_wc(&qpic_res->pdev->dev,
				      QPIC_MAX_CMD_BUF_SIZE,
				      qpic_res->cmd_buf_virt,
				      qpic_res->cmd_buf_phys);

	clk_disable_unprepare(qpic_res->aon_clk);
	clk_disable_unprepare(qpic_res->core_clk);

	return 0;
}

static int __init mdss_qpic_driver_init(void)
{
	int ret;

	ret = platform_driver_register(&mdss_qpic_driver);
	if (ret)
		pr_err("mdss_qpic_register_driver() failed!\n");
	return ret;
}

module_init(mdss_qpic_driver_init);


