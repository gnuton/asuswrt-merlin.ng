/*
 * Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>
#include <sound/pcm.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/spinlock.h>
#include <linux/of_device.h>

#include "ipq-adss.h"

static void __iomem *adss_audio_local_base;
static struct reset_control *audio_blk_rst;
static spinlock_t i2s_ctrl_lock;
static spinlock_t tdm_ctrl_lock;
static spinlock_t glb_mode_lock;

static struct ipq_configs ipq8074_cfgs = {
	.txd_oe = {
		.reg = ADSS_GLB_CLK_I2S_CTRL_REG,
		.mask = GLB_CLK_I2S_CTRL_I2S0_TXD_OE,
	},
	.rxd_oe = {
		.reg = ADSS_GLB_CLK_I2S_CTRL_REG,
		.mask = GLB_CLK_I2S_CTRL_I2S3_RXD_OE,
	},
	.i2s0_fs_oe = {
		.reg = ADSS_GLB_CLK_I2S_CTRL_REG,
		.mask = GLB_CLK_I2S_CTRL_I2S0_FS_OE,
	},
	.i2s3_fs_oe = {
		.reg = ADSS_GLB_CLK_I2S_CTRL_REG,
		.mask = GLB_CLK_I2S_CTRL_I2S3_FS_OE,
	},
	.i2s_reset_val = {
		.reg = ADSS_GLB_I2S_RST_REG,
		.mask = GLB_I2S_RESET_VAL_8074,
	},
};
static struct ipq_configs *ipq_cfgs;

/* API to write ADSS registers */
void ipq_audio_adss_writel(uint32_t val, uint32_t offset)
{
	if (!adss_audio_local_base) {
		pr_err("adss_audio_local_base not mapped\n");
		return;
	}
	writel(val, adss_audio_local_base + offset);
}
EXPORT_SYMBOL(ipq_audio_adss_writel);

/* API to read ADSS regitsers */
uint32_t ipq_audio_adss_readl(uint32_t offset)
{
	if (adss_audio_local_base)
		return readl(adss_audio_local_base + offset);
	pr_err("adss_audio_local_base not mapped\n");
	return 0;
}
EXPORT_SYMBOL(ipq_audio_adss_readl);

/* Channel Number Per Frame for Transmitter/Receiver
 * Real value = val + 1
 */
void ipq_glb_tdm_ctrl_ch_num(uint32_t val, uint32_t dir)
{
	uint32_t cfg;
	unsigned long flags;

	spin_lock_irqsave(&tdm_ctrl_lock, flags);
	cfg = readl(adss_audio_local_base + ADSS_GLB_TDM_CTRL_REG);

	if (dir == PLAYBACK) {
		cfg &= ~(GLB_TDM_CTRL_TX_CHAN_NUM_MASK);
		cfg |= GLB_TDM_CTRL_TX_CHAN_NUM(val);
	} else if (dir == CAPTURE) {
		cfg &= ~(GLB_TDM_CTRL_RX_CHAN_NUM_MASK);
		cfg |= GLB_TDM_CTRL_RX_CHAN_NUM(val);
	}
	writel(cfg, adss_audio_local_base + ADSS_GLB_TDM_CTRL_REG);
	spin_unlock_irqrestore(&tdm_ctrl_lock, flags);
}
EXPORT_SYMBOL(ipq_glb_tdm_ctrl_ch_num);

/* FSYNC Hi Duration for Transmitter/Receiver */
void ipq_glb_tdm_ctrl_sync_num(uint32_t val, uint32_t dir)
{
	uint32_t cfg;
	unsigned long flags;

	spin_lock_irqsave(&tdm_ctrl_lock, flags);
	cfg = readl(adss_audio_local_base + ADSS_GLB_TDM_CTRL_REG);

	if (dir == PLAYBACK) {
		cfg &= ~(GLB_TDM_CTRL_TX_SYNC_NUM_MASK);
		cfg |= GLB_TDM_CTRL_TX_SYNC_NUM(val);
	} else if (dir == CAPTURE) {
		cfg &= ~(GLB_TDM_CTRL_RX_SYNC_NUM_MASK);
		cfg |= GLB_TDM_CTRL_RX_SYNC_NUM(val);
	}
	writel(cfg, adss_audio_local_base + ADSS_GLB_TDM_CTRL_REG);
	spin_unlock_irqrestore(&tdm_ctrl_lock, flags);
}
EXPORT_SYMBOL(ipq_glb_tdm_ctrl_sync_num);

/* Serial Data Delay for transmitter/receiver */
void ipq_glb_tdm_ctrl_delay(uint32_t delay, uint32_t dir)
{
	uint32_t cfg;
	unsigned long flags;

	spin_lock_irqsave(&tdm_ctrl_lock, flags);
	cfg = readl(adss_audio_local_base + ADSS_GLB_TDM_CTRL_REG);

	if (dir == PLAYBACK) {
		cfg &= ~(GLB_TDM_CTRL_TX_DELAY);
		if (delay)
			cfg |= GLB_TDM_CTRL_TX_DELAY;
	} else if (dir == CAPTURE) {
		cfg &= ~(GLB_TDM_CTRL_RX_DELAY);
		if (delay)
			cfg |= GLB_TDM_CTRL_RX_DELAY;
	}
	writel(cfg, adss_audio_local_base + ADSS_GLB_TDM_CTRL_REG);
	spin_unlock_irqrestore(&tdm_ctrl_lock, flags);
}
EXPORT_SYMBOL(ipq_glb_tdm_ctrl_delay);

/* I2S Interface Enable */
static void ipq_glb_i2s_interface_en(int enable)
{
	u32 cfg;
	unsigned long flags;

	spin_lock_irqsave(&i2s_ctrl_lock, flags);
	cfg = readl(adss_audio_local_base + ADSS_GLB_CHIP_CTRL_I2S_REG);
	cfg &= ~GLB_CHIP_CTRL_I2S_INTERFACE_EN;
	if (enable)
		cfg |= GLB_CHIP_CTRL_I2S_INTERFACE_EN;
	writel(cfg, adss_audio_local_base + ADSS_GLB_CHIP_CTRL_I2S_REG);
	spin_unlock_irqrestore(&i2s_ctrl_lock, flags);
	/*
	 * As per the audio controller susbsytem after writing to
	 * the register wait 5ms for the i2s settle down.
	 */
	mdelay(5);
}
EXPORT_SYMBOL(ipq_glb_i2s_interface_en);

/* Enable Stereo0/Stereo1/Stereo2 channel */
void ipq_glb_stereo_ch_en(int enable, int stereo_ch)
{
	uint32_t cfg;
	unsigned long flags;

	spin_lock_irqsave(&i2s_ctrl_lock, flags);
	cfg = readl(adss_audio_local_base + ADSS_GLB_CHIP_CTRL_I2S_REG);
	if (stereo_ch == STEREO0) {
		cfg &= ~(GLB_CHIP_CTRL_I2S_STEREO0_GLB_EN);
		cfg |= GLB_CHIP_CTRL_I2S_STEREO0_GLB_EN;
	} else if (stereo_ch == STEREO1) {
		cfg &= ~(GLB_CHIP_CTRL_I2S_STEREO1_GLB_EN);
		cfg |= GLB_CHIP_CTRL_I2S_STEREO1_GLB_EN;
	} else if (stereo_ch == STEREO2) {
		cfg &= ~(GLB_CHIP_CTRL_I2S_STEREO2_GLB_EN);
		cfg |= GLB_CHIP_CTRL_I2S_STEREO2_GLB_EN;
	}
	writel(cfg, adss_audio_local_base + ADSS_GLB_CHIP_CTRL_I2S_REG);
	spin_unlock_irqrestore(&i2s_ctrl_lock, flags);
}
EXPORT_SYMBOL(ipq_glb_stereo_ch_en);

/*
 * I2S Module Reset
 */
static void ipq_glb_i2s_reset(void)
{
	writel(ipq_cfgs->i2s_reset_val.mask,
		adss_audio_local_base + ipq_cfgs->i2s_reset_val.reg);
	mdelay(5);
	writel(0x0, adss_audio_local_base + ADSS_GLB_I2S_RST_REG);
	mdelay(5);
}

/*
 * MBOX Module Reset
 */
void ipq_glb_mbox_reset(void)
{
	writel(GLB_I2S_RST_MBOX_RESET_MASK,
		adss_audio_local_base + ipq_cfgs->i2s_reset_val.reg);
	mdelay(5);
	writel(0x0, adss_audio_local_base + ADSS_GLB_I2S_RST_REG);
	mdelay(5);
}
EXPORT_SYMBOL(ipq_glb_mbox_reset);

/*
 * Enable I2S/TDM and Playback/Capture Audio Mode
 */
void ipq_glb_audio_mode(int mode, int dir)
{
	u32 cfg;
	unsigned long flags;

	spin_lock_irqsave(&glb_mode_lock, flags);
	cfg = readl(adss_audio_local_base + ADSS_GLB_AUDIO_MODE_REG);
	if (mode == I2S && dir == PLAYBACK) {
		cfg &= ~GLB_AUDIO_MODE_XMIT_MASK;
		cfg |= GLB_AUDIO_MODE_XMIT_I2S;
	} else if (mode == I2S && dir == CAPTURE) {
		cfg &= ~GLB_AUDIO_MODE_RECV_MASK;
		cfg |= GLB_AUDIO_MODE_RECV_I2S;
	} else if (mode == TDM && dir == PLAYBACK) {
		cfg &= ~GLB_AUDIO_MODE_XMIT_MASK;
		cfg |= GLB_AUDIO_MODE_XMIT_TDM;
	} else if (mode == TDM && dir == CAPTURE) {
		cfg &= ~GLB_AUDIO_MODE_RECV_MASK;
		cfg |= GLB_AUDIO_MODE_RECV_TDM;
	}
	writel(cfg, adss_audio_local_base + ADSS_GLB_AUDIO_MODE_REG);
	spin_unlock_irqrestore(&glb_mode_lock, flags);
}
EXPORT_SYMBOL(ipq_glb_audio_mode);

/*
 * I2S0 TX Data Port Enable
 *
 * Todo :
 * Check if bits 6:4 configures only
 * I2S0 or other channels as well
 */
void ipq_glb_tx_data_port_en(u32 enable)
{
	u32 cfg;
	unsigned long flags;
	uint32_t reg = ipq_cfgs->txd_oe.reg;
	uint32_t val = ipq_cfgs->txd_oe.mask;

	spin_lock_irqsave(&glb_mode_lock, flags);
	cfg = readl(adss_audio_local_base + reg);
	cfg &= ~val;
	if (enable)
		cfg |= val;
	writel(cfg, adss_audio_local_base + reg);
	spin_unlock_irqrestore(&glb_mode_lock, flags);
}
EXPORT_SYMBOL(ipq_glb_tx_data_port_en);

/*
 * I2S3 RX Data Port Enable
 */
void ipq_glb_rx_data_port_en(u32 enable)
{
	u32 cfg;
	unsigned long flags;
	uint32_t reg = ipq_cfgs->rxd_oe.reg;
	uint32_t val = ipq_cfgs->rxd_oe.mask;

	spin_lock_irqsave(&glb_mode_lock, flags);
	cfg = readl(adss_audio_local_base + reg);
	cfg &= ~val;
	if (enable)
		cfg |= val;
	writel(cfg, adss_audio_local_base + reg);
	spin_unlock_irqrestore(&glb_mode_lock, flags);
}
EXPORT_SYMBOL(ipq_glb_rx_data_port_en);

/*
 * Cross 1K Boundary
 */
void ipq_glb_audio_mode_B1K(void)
{
	u32 cfg;
	unsigned long flags;

	spin_lock_irqsave(&glb_mode_lock, flags);
	cfg =  readl(adss_audio_local_base + ADSS_GLB_AUDIO_MODE_REG);
	cfg &= ~GLB_AUDIO_MODE_B1K;
	cfg |= GLB_AUDIO_MODE_B1K;
	writel(cfg, adss_audio_local_base + ADSS_GLB_AUDIO_MODE_REG);
	spin_unlock_irqrestore(&glb_mode_lock, flags);
}
EXPORT_SYMBOL(ipq_glb_audio_mode_B1K);

/*
 * Frame Sync Port Enable for I2S0 TX
 */
void ipq_glb_tx_framesync_port_en(u32 enable)
{
	u32 cfg;
	unsigned long flags;
	uint32_t reg = ipq_cfgs->i2s0_fs_oe.reg;
	uint32_t val = ipq_cfgs->i2s0_fs_oe.mask;

	spin_lock_irqsave(&glb_mode_lock, flags);
	cfg =  readl(adss_audio_local_base + reg);
	cfg &= ~val;
	if (enable)
		cfg |= val;
	writel(cfg, adss_audio_local_base + reg);
	spin_unlock_irqrestore(&glb_mode_lock, flags);
}
EXPORT_SYMBOL(ipq_glb_tx_framesync_port_en);

/*
 * Frame Sync Port Enable for I2S3 RX
 */
void ipq_glb_rx_framesync_port_en(u32 enable)
{
	u32 cfg;
	unsigned long flags;
	uint32_t reg = ipq_cfgs->i2s3_fs_oe.reg;
	uint32_t val = ipq_cfgs->i2s3_fs_oe.mask;

	spin_lock_irqsave(&glb_mode_lock, flags);
	cfg =  readl(adss_audio_local_base + reg);
	cfg &= ~val;
	if (enable)
		cfg |= val;
	writel(cfg, adss_audio_local_base + reg);
	spin_unlock_irqrestore(&glb_mode_lock, flags);
}
EXPORT_SYMBOL(ipq_glb_rx_framesync_port_en);

void ipq_glb_clk_enable_oe(u32 dir)
{
	u32 cfg;
	unsigned long flags;

	spin_lock_irqsave(&i2s_ctrl_lock, flags);
	cfg = readl(adss_audio_local_base + ADSS_GLB_CLK_I2S_CTRL_REG);

	if (dir == PLAYBACK) {
		cfg |= (GLB_CLK_I2S_CTRL_TX_BCLK_OE |
			GLB_CLK_I2S_CTRL_TX_MCLK_OE);
	} else {
		cfg |= (GLB_CLK_I2S_CTRL_RX_BCLK_OE |
			GLB_CLK_I2S_CTRL_RX_MCLK_OE);
	}
	writel(cfg, adss_audio_local_base + ADSS_GLB_CLK_I2S_CTRL_REG);
	spin_unlock_irqrestore(&i2s_ctrl_lock, flags);
}
EXPORT_SYMBOL(ipq_glb_clk_enable_oe);

/* PCM RAW ADSS_GLB_PCM_RST_REG register */
void ipq_glb_pcm_rst(uint32_t enable)
{
	uint32_t reg_val;

	if (enable)
		reg_val = GLB_PCM_RST_CTRL(1);
	else
		reg_val = GLB_PCM_RST_CTRL(0);

	writel(reg_val, adss_audio_local_base + ADSS_GLB_PCM_RST_REG);
}
EXPORT_SYMBOL(ipq_glb_pcm_rst);

/* PCM RAW clock configuration */
void ipq_pcm_clk_cfg(uint32_t rate)
{
	uint32_t reg_val;
	uint32_t div_1, div_2;

	if (rate == 8000) {
		div_1 = 3; div_2 = 1;
	} else if (rate == 16000) {
		div_1 = 3; div_2 = 0;
	} else {
		pr_err("%s invalid rate %d\n", __func__, rate);
		return;
	}

	/* set ADSS_AUDIO_PCM_CFG_RCGR_REG as required */
	reg_val = readl(adss_audio_local_base + ADSS_AUDIO_PCM_CFG_RCGR_REG);
	reg_val |= AUDIO_PCM_CFG_RCGR_SRC_SEL(1)
			| AUDIO_PCM_CFG_RGCR_SRC_DIV(div_1);
	writel(reg_val, adss_audio_local_base + ADSS_AUDIO_PCM_CFG_RCGR_REG);

	/* set ADSS_AUDIO_PCM_MISC_REG  as required */
	reg_val = AUDIO_PCM_MISC_AUTO_SCALE_DIV(div_2);
	writel(reg_val, adss_audio_local_base + ADSS_AUDIO_PCM_MISC_REG);

	/* set ADSS_AUDIO_PCM_CMD_RCGR_REG as required */
	reg_val = 3;
	writel(reg_val, adss_audio_local_base + ADSS_AUDIO_PCM_CMD_RCGR_REG);
}
EXPORT_SYMBOL(ipq_pcm_clk_cfg);

void ipq_pcm_clk_enable(void)
{
	/* write ADSS_AUDIO_PCM_CBCR_REG*/
	writel(0x1, adss_audio_local_base + ADSS_AUDIO_PCM_CBCR_REG);
	writel(0x1, adss_audio_local_base + ADSS_AUDIO_ZSI_CBCR_REG);
}
EXPORT_SYMBOL(ipq_pcm_clk_enable);

static const struct of_device_id ipq_audio_adss_id_table[] = {
	{ .compatible = "qca,ipq8074-audio-adss", .data = &ipq8074_cfgs},
	{},
};
MODULE_DEVICE_TABLE(of, ipq_audio_adss_id_table);

void ipq_audio_adss_init(void)
{
	spin_lock_init(&i2s_ctrl_lock);
	spin_lock_init(&glb_mode_lock);
	spin_lock_init(&tdm_ctrl_lock);

	/*
	 * Reset order is critical here.
	 * First audio block should be out of reset,
	 * followed by I2S block.
	 * Since the audio block is brought out of
	 * reset by hardware by default, it is not
	 * required to be done in software explicitly.
	 */
	ipq_glb_i2s_reset();

	ipq_glb_i2s_interface_en(ENABLE);

	ipq_glb_audio_mode_B1K();
}
EXPORT_SYMBOL(ipq_audio_adss_init);

static int ipq_audio_adss_probe(struct platform_device *pdev)
{
	struct resource *res;
	const struct of_device_id *match;

	match = of_match_device(ipq_audio_adss_id_table, &pdev->dev);
	if (!match)
		return -ENODEV;

	ipq_cfgs = (struct ipq_configs *)match->data;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	adss_audio_local_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(adss_audio_local_base))
		return PTR_ERR(adss_audio_local_base);

	audio_blk_rst = devm_reset_control_get(&pdev->dev, "blk_rst");
	if (IS_ERR(audio_blk_rst))
		return PTR_ERR(audio_blk_rst);

	return 0;
}

static int ipq_audio_adss_remove(struct platform_device *pdev)
{
	ipq_glb_i2s_interface_en(DISABLE);
	return 0;
}

static struct platform_driver ipq_audio_adss_driver = {
	.probe = ipq_audio_adss_probe,
	.remove = ipq_audio_adss_remove,
	.driver = {
		.name = "ipq-adss",
		.of_match_table = ipq_audio_adss_id_table,
	},
};

module_platform_driver(ipq_audio_adss_driver);

MODULE_ALIAS("platform:ipq-adss");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("IPQ Audio subsytem driver");
