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
#include <linux/io.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <linux/delay.h>
#include <linux/spinlock.h>

#include "ipq-adss.h"

struct stereo_priv_data {
	void __iomem *stereo_base;
	spinlock_t stereo_lock;
};

static struct stereo_priv_data stereo_priv[MAX_STEREO_ENTRIES];

/*
 *
 * Stereo buffers and I2S state reset
 */
void ipq_stereo_config_reset(u32 reset, u32 stereo_id)
{
	u32 cfg;
	unsigned long flags;

	spin_lock_irqsave(&stereo_priv[stereo_id].stereo_lock, flags);
	cfg = readl(stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	cfg &= ~STEREOn_CONFIG_RESET;
	if (reset)
		cfg |= STEREOn_CONFIG_RESET;
	writel(cfg, stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	spin_unlock_irqrestore(&stereo_priv[stereo_id].stereo_lock, flags);
}
EXPORT_SYMBOL(ipq_stereo_config_reset);

/*
 * MIC buffers reset
 */
void ipq_stereo_config_mic_reset(u32 reset, u32 stereo_id)
{
	u32 cfg;
	unsigned long flags;

	spin_lock_irqsave(&stereo_priv[stereo_id].stereo_lock, flags);
	cfg = readl(stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	cfg &= ~STEREOn_CONFIG_MIC_RESET;
	if (reset)
		cfg |= STEREOn_CONFIG_MIC_RESET;
	writel(cfg, stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	spin_unlock_irqrestore(&stereo_priv[stereo_id].stereo_lock, flags);
}
EXPORT_SYMBOL(ipq_stereo_config_mic_reset);

/*
 * Enable the I2S Stereo block for operation
 */
void ipq_stereo_config_enable(u32 enable, u32 stereo_id)
{
	u32 cfg;
	unsigned long flags;

	spin_lock_irqsave(&stereo_priv[stereo_id].stereo_lock, flags);
	cfg = readl(stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	cfg &= ~STEREOn_CONFIG_ENABLE;
	if (enable)
		cfg |= STEREOn_CONFIG_ENABLE;
	writel(cfg, stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	spin_unlock_irqrestore(&stereo_priv[stereo_id].stereo_lock, flags);
}
EXPORT_SYMBOL(ipq_stereo_config_enable);

/* Configure
 * Data word size : Word size loaded into the PCM
 *			register from the MBOX FIFO.
 * I2S word size : Word size sent to the external I2S DAC.
 *			When set to 32 bit words the PCM data
 *			will be left justified in the I2S word.
 */
int ipq_cfg_bit_width(u32 bit_width, u32 stereo_id)
{
	u32 cfg, mask = 0;
	unsigned long flags;

	switch (bit_width) {
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_S16_BE:
		mask |= (STEREOn_CONFIG_DATA_WORD_SIZE(1) |
			STEREOn_CONFIG_I2S_WORD_SIZE_16 |
			STEREOn_CONFIG_MIC_WORD_SIZE_16);
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
	case SNDRV_PCM_FORMAT_S24_3BE:
		mask |= (STEREOn_CONFIG_DATA_WORD_SIZE(2) |
			STEREOn_CONFIG_I2S_WORD_SIZE_32 |
			STEREOn_CONFIG_MIC_WORD_SIZE_16);
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
	case SNDRV_PCM_FORMAT_S32_BE:
		mask |= (STEREOn_CONFIG_DATA_WORD_SIZE(3) |
			STEREOn_CONFIG_I2S_WORD_SIZE_32 |
			STEREOn_CONFIG_MIC_WORD_SIZE_32);
		break;
	default:
		return -ENOTSUPP;
	}

	spin_lock_irqsave(&stereo_priv[stereo_id].stereo_lock, flags);
	cfg = readl(stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	cfg &= ~STEREOn_CONFIG_DATA_WORD_SIZE_MASK;
	cfg &= ~STEREOn_CONFIG_I2S_WORD_SIZE_32;
	cfg &= ~STEREOn_CONFIG_MIC_WORD_SIZE_32;
	cfg |= mask;
	writel(cfg, stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	spin_unlock_irqrestore(&stereo_priv[stereo_id].stereo_lock, flags);

	return 0;
}
EXPORT_SYMBOL(ipq_cfg_bit_width);

/*
 * Configure stereo/mono mode
 */
void ipq_config_stereo_mode(u32 mode, u32 stereo_id)
{
	u32 cfg;
	unsigned long flags;

	spin_lock_irqsave(&stereo_priv[stereo_id].stereo_lock, flags);
	cfg = readl(stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	cfg &= ~STEREOn_CONFIG_STEREO_MONO_MASK;
	if (mode == CH_STEREO)
		cfg |= STEREOn_CONFIG_STEREO_MODE;
	writel(cfg, stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	spin_unlock_irqrestore(&stereo_priv[stereo_id].stereo_lock, flags);
}
EXPORT_SYMBOL(ipq_config_stereo_mode);

/*
 * Configure master mode
 */
void ipq_config_master(u32 enable, u32 stereo_id)
{
	u32 cfg;
	unsigned long flags;

	spin_lock_irqsave(&stereo_priv[stereo_id].stereo_lock, flags);
	cfg = readl(stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	cfg &= ~STEREOn_CONFIG_MASTER;
	if (enable)
		cfg |= STEREOn_CONFIG_MASTER;
	writel(cfg, stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	spin_unlock_irqrestore(&stereo_priv[stereo_id].stereo_lock, flags);
}
EXPORT_SYMBOL(ipq_config_master);

/* Selects the raw clock source between
 * divided audio clock and input master clock
 * Val 0: Raw master clock is divided audio PLL clock
 * Val 1: Raw master clock is MCLK IN
 */
void ipq_config_mclk_sel(u32 stereo_id, u32 val)
{
	u32 cfg;
	unsigned long flags;

	spin_lock_irqsave(&stereo_priv[stereo_id].stereo_lock, flags);
	cfg = readl(stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	cfg &= ~STEREOn_CONFIG_MCK_SEL;
	if (val)
		cfg |= STEREOn_CONFIG_MCK_SEL;
	writel(cfg, stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	spin_unlock_irqrestore(&stereo_priv[stereo_id].stereo_lock, flags);

}
EXPORT_SYMBOL(ipq_config_mclk_sel);

/*
 * Strategy to clear the sample counter TX and RX registers
 */
void ipq_config_sample_cnt_clear_type(u32 stereo_id)
{
	u32 cfg;
	unsigned long flags;

	spin_lock_irqsave(&stereo_priv[stereo_id].stereo_lock, flags);
	cfg = readl(stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	/* 0 - write an explicit zero data through software
	 *	to the TX and RX sample counter registers
	 * 1 - software read of the TX and RX sample counter
	 *	registers clears the counter registers
	 */
	cfg |= STEREOn_CONFIG_SAMPLE_CNT_CLEAR_TYPE; /* Write 1 */
	writel(cfg, stereo_priv[stereo_id].stereo_base
			+ ADSS_STEREOn_STEREO0_CONFIG_REG);
	spin_unlock_irqrestore(&stereo_priv[stereo_id].stereo_lock, flags);
}
EXPORT_SYMBOL(ipq_config_sample_cnt_clear_type);

static const struct of_device_id ipq_audio_stereo_id_table[] = {
	{ .compatible = "qca,ipq8074-stereo" },
	{},
};
MODULE_DEVICE_TABLE(of, ipq_audio_stereo_id_table);

static int ipq_audio_stereo_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct stereo_priv_data *spd;
	struct device_node *np = pdev->dev.of_node;
	u32 stereo_port_id = 0;

	if (of_property_read_u32(np, "stereo-index", &stereo_port_id)) {
		dev_err(&pdev->dev, "Error reading stereo-index\n");
		return -EINVAL;
	}
	if (stereo_port_id >= MAX_STEREO_ENTRIES)
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	spd = &stereo_priv[stereo_port_id];
	spd->stereo_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(spd->stereo_base))
		return PTR_ERR(spd->stereo_base);

	spin_lock_init(&spd->stereo_lock);
	return 0;
}

static struct platform_driver ipq_audio_stereo_driver = {
	.probe = ipq_audio_stereo_probe,
	.driver = {
		.name = "ipq-stereo",
		.of_match_table = ipq_audio_stereo_id_table,
	},
};

module_platform_driver(ipq_audio_stereo_driver);

MODULE_ALIAS("platform:ipq-stereo");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("IPQ AUDIO Stereo driver");
