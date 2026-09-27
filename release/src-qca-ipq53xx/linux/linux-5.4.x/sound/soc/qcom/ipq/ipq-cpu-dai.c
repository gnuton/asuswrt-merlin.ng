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
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of_device.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>

#include "ipq-mbox.h"
#include "ipq-adss.h"

struct dai_priv_st {
	int stereo_tx;
	int stereo_rx;
	int mbox_tx;
	int mbox_rx;
	int tx_enabled;
	int rx_enabled;
	struct platform_device *pdev;
	enum ipq_hw_type ipq_hw;
};
static struct dai_priv_st dai_priv[MAX_INTF];

static bool slave;

static struct clk *audio_tx_bclk;
static struct clk *audio_tx_mclk;
static struct clk *audio_rx_bclk;
static struct clk *audio_rx_mclk;

/* Get Stereo channel ID based on I2S intf and direction */
int ipq_get_stereo_id(struct snd_pcm_substream *substream, int intf)
{
	switch (substream->stream) {
	case SNDRV_PCM_STREAM_PLAYBACK:
		return dai_priv[intf].stereo_tx;
	case SNDRV_PCM_STREAM_CAPTURE:
		return dai_priv[intf].stereo_rx;
	}
	return -EINVAL;
}
EXPORT_SYMBOL(ipq_get_stereo_id);

/* Get MBOX channel ID based on I2S/TDM/SPDIF intf and direction */
int ipq_get_mbox_id(struct snd_pcm_substream *substream, int intf)
{
	switch (substream->stream) {
	case SNDRV_PCM_STREAM_PLAYBACK:
		return dai_priv[intf].mbox_tx;
	case SNDRV_PCM_STREAM_CAPTURE:
		return dai_priv[intf].mbox_rx;
	}
	return -EINVAL;
}
EXPORT_SYMBOL(ipq_get_mbox_id);

u32 ipq_get_act_bit_width(u32 bit_width)
{
	switch (bit_width) {
	case SNDRV_PCM_FORMAT_S8:
	case SNDRV_PCM_FORMAT_U8:
		return __BIT_8;
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_S16_BE:
	case SNDRV_PCM_FORMAT_U16_LE:
	case SNDRV_PCM_FORMAT_U16_BE:
		return __BIT_16;
	case SNDRV_PCM_FORMAT_S24_3LE:
	case SNDRV_PCM_FORMAT_S24_3BE:
	case SNDRV_PCM_FORMAT_U24_3LE:
	case SNDRV_PCM_FORMAT_U24_3BE:
		return __BIT_32;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_BE:
	case SNDRV_PCM_FORMAT_U24_LE:
	case SNDRV_PCM_FORMAT_U24_BE:
		return __BIT_24;
	case SNDRV_PCM_FORMAT_S32_LE:
	case SNDRV_PCM_FORMAT_S32_BE:
	case SNDRV_PCM_FORMAT_U32_LE:
	case SNDRV_PCM_FORMAT_U32_BE:
		return __BIT_32;
	}
	return __BIT_INVAL;
}
EXPORT_SYMBOL(ipq_get_act_bit_width);

static int ipq_audio_clk_set(struct clk *clk, struct device *dev,
					u32 val)
{
	int ret;

	ret = clk_set_rate(clk, val);
	if (ret != 0) {
		dev_err_ratelimited(dev, "Error in setting %s\n",
						__clk_get_name(clk));
		return ret;
	}

	ret = clk_prepare_enable(clk);
	if (ret != 0) {
		dev_err_ratelimited(dev, "Error in enable %s\n",
					__clk_get_name(clk));
		return ret;
	}
	return 0;
}

static void ipq_audio_clk_disable(struct clk **clk, struct device *dev)
{
	if (__clk_is_enabled(*clk))
		clk_disable_unprepare(*clk);
}

static int ipq_audio_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	u32 intf = dai->driver->id;

	switch (substream->stream) {
	case SNDRV_PCM_STREAM_PLAYBACK:
		/* Check if the direction is enabled */
		if (dai_priv[intf].tx_enabled != ENABLE)
			return -EFAULT;

		ipq_glb_tx_data_port_en(ENABLE);
		ipq_glb_tx_framesync_port_en(ENABLE);
		break;
	case SNDRV_PCM_STREAM_CAPTURE:
		/* Check if the direction is enabled */
		if (dai_priv[intf].rx_enabled != ENABLE)
			return -EFAULT;

		ipq_glb_rx_data_port_en(ENABLE);
		ipq_glb_rx_framesync_port_en(ENABLE);
		break;
	default:
		return -EINVAL;
	}

	if (intf == I2S || intf == I2S1 || intf == I2S2) {
		/* Select I2S mode */
		ipq_glb_audio_mode(I2S, substream->stream);
	} else if (intf == TDM) {
		/* Select TDM mode */
		ipq_glb_audio_mode(TDM, substream->stream);

		/* Set TDM Ctrl register */
		ipq_glb_tdm_ctrl_sync_num(TDM_SYNC_NUM, substream->stream);
		ipq_glb_tdm_ctrl_delay(TDM_DELAY, substream->stream);
	}

	return 0;
}

static int ipq_audio_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params,
					struct snd_soc_dai *dai)
{
	u32 bit_width, channels, rate;
	u32 intf = dai->driver->id;
	u32 stereo_id = ipq_get_stereo_id(substream, intf);
	u32 mbox_id = ipq_get_mbox_id(substream, intf);
	u32 bit_act;
	int ret;
	u32 mclk, bclk;
	struct device *dev = &(dai_priv[intf].pdev->dev);

	bit_width = params_format(params);
	channels = params_channels(params);
	rate = params_rate(params);

	bit_act = ipq_get_act_bit_width(bit_width);

	if (intf == TDM) {
		/* Set TDM number of channels */
		ipq_glb_tdm_ctrl_ch_num((channels-1), substream->stream);
		mclk = bclk = rate * bit_act * channels;
	} else {
		bclk = rate * bit_act * channels;
		mclk = bclk * MCLK_MULTI;
	}

	ipq_glb_clk_enable_oe(substream->stream);

	if (slave) {
		ipq_config_master(DISABLE, stereo_id);
		ipq_config_mclk_sel(stereo_id, 1);
		/*
		 * 252 and 254 are dummy frequency values which will make clock
		 * framework to select external pad clock as source. We need to
		 * configure the mclk and bclk to these fixed values when in
		 * slave mode in order to accept clock from master dev.
		 *
		 * mclk should be set to 252 (mclk_pad_in) and bclk can be set
		 * to 252 (mclk_pad_in) or 254 (bclk_pad_in).
		 */
		mclk = 252;
		bclk = 254;
	} else {
		ipq_config_master(ENABLE, stereo_id);
	}

	ret = ipq_cfg_bit_width(bit_width, stereo_id);
	if (ret) {
		pr_err("BitWidth %d not supported ret: %d\n", bit_width, ret);
		return ret;
	}

	ipq_stereo_config_enable(DISABLE, stereo_id);

	ipq_stereo_config_reset(ENABLE, stereo_id);
	ipq_stereo_config_mic_reset(ENABLE, stereo_id);

	mdelay(5);

	ret = ipq_mbox_fifo_reset(mbox_id);
	if (ret) {
		pr_err("%s: ret: %d Error in dma fifo reset\n",
					__func__, ret);
		return ret;
	}

	ipq_stereo_config_reset(DISABLE, stereo_id);
	ipq_stereo_config_mic_reset(DISABLE, stereo_id);

	switch (substream->stream) {
	case SNDRV_PCM_STREAM_PLAYBACK:
		ret = ipq_audio_clk_set(audio_tx_mclk, dev, mclk);
		if (ret)
			return ret;

		ret = ipq_audio_clk_set(audio_tx_bclk, dev, bclk);
		if (ret)
			return ret;
		break;

	case SNDRV_PCM_STREAM_CAPTURE:
		ret = ipq_audio_clk_set(audio_rx_mclk, dev, mclk);
		if (ret)
			return ret;

		ret = ipq_audio_clk_set(audio_rx_bclk, dev, bclk);
		if (ret)
			return ret;
		break;
	}

	return 0;
}

static void ipq_audio_shutdown(struct snd_pcm_substream *substream,
					struct snd_soc_dai *dai)
{
	u32 intf = dai->driver->id;
	struct device *dev = &(dai_priv[intf].pdev->dev);

	switch (substream->stream) {
	case SNDRV_PCM_STREAM_PLAYBACK:
		ipq_glb_tx_data_port_en(DISABLE);
		ipq_glb_tx_framesync_port_en(DISABLE);

		/* Disable the clocks */
		ipq_audio_clk_disable(&audio_tx_bclk, dev);
		ipq_audio_clk_disable(&audio_tx_mclk, dev);
		break;
	case SNDRV_PCM_STREAM_CAPTURE:
		ipq_glb_rx_data_port_en(DISABLE);
		ipq_glb_rx_framesync_port_en(DISABLE);

		/* Disable the clocks */
		ipq_audio_clk_disable(&audio_rx_bclk, dev);
		ipq_audio_clk_disable(&audio_rx_mclk, dev);
		break;
	}
}

static struct snd_soc_dai_ops ipq_audio_ops = {
	.startup	= ipq_audio_startup,
	.hw_params	= ipq_audio_hw_params,
	.shutdown	= ipq_audio_shutdown,
};

static struct snd_soc_dai_driver ipq8074_cpu_dais[] = {
	{
		.playback = {
			.rates		= RATE_16000_96000,
			.formats	= SNDRV_PCM_FMTBIT_S16 |
					SNDRV_PCM_FMTBIT_S32,
			.channels_min	= CH_STEREO,
			.channels_max	= CH_STEREO,
			.rate_min	= FREQ_16000,
			.rate_max	= FREQ_96000,
		},
		.capture = {
			.rates		= RATE_16000_96000,
			.formats	= SNDRV_PCM_FMTBIT_S16 |
					SNDRV_PCM_FMTBIT_S32,
			.channels_min	= CH_STEREO,
			.channels_max	= CH_STEREO,
			.rate_min	= FREQ_16000,
			.rate_max	= FREQ_96000,
		},
		.ops = &ipq_audio_ops,
		.id = I2S,
		.name = "qca-i2s-dai"
	},
	{
		.playback = {
			.rates		= RATE_16000_96000,
			.formats	= SNDRV_PCM_FMTBIT_S16 |
					SNDRV_PCM_FMTBIT_S32,
			.channels_min	= CH_STEREO,
			.channels_max	= CH_7_1,
			.rate_min	= FREQ_16000,
			.rate_max	= FREQ_96000,
		},
		.capture = {
			.rates		= RATE_16000_96000,
			.formats	= SNDRV_PCM_FMTBIT_S16 |
					SNDRV_PCM_FMTBIT_S32,
			.channels_min	= CH_STEREO,
			.channels_max	= CH_7_1,
			.rate_min	= FREQ_16000,
			.rate_max	= FREQ_96000,
		},
		.ops = &ipq_audio_ops,
		.id = TDM,
		.name = "qca-tdm-dai"
	},
};

static const struct snd_soc_component_driver ipq_i2s_component = {
	.name           = "qca-cpu-dai",
};

struct ipq_intf_pdata ipq8074_i2s_pdata = {
	.data = I2S,
	.hw = IPQ8074,
};

struct ipq_intf_pdata ipq8074_tdm_pdata = {
	.data = TDM,
	.hw = IPQ8074,
};

static const struct of_device_id ipq_cpu_dai_id_table[] = {
	{ .compatible = "qca,ipq8074-i2s", .data = &ipq8074_i2s_pdata },
	{ .compatible = "qca,ipq8074-tdm", .data = &ipq8074_tdm_pdata },
	{},
};
MODULE_DEVICE_TABLE(of, ipq_cpu_dai_id_table);

static int ipq_dai_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct device_node *np = NULL;
	struct ipq_intf_pdata *pdata = NULL;
	int ret;
	int intf;

	match = of_match_device(ipq_cpu_dai_id_table, &pdev->dev);
	if (!match)
		return -ENODEV;

	pdata = (struct ipq_intf_pdata *)match->data;
	intf = pdata->data;
	dai_priv[intf].ipq_hw = pdata->hw;

	/* If Slave property is not found in DTS, then
	 * Audio will be configured as master by default
	 */
	np = of_find_node_by_name(NULL, "audio");
	if (np)
		slave = of_property_read_bool(np, "slave");

	np = pdev->dev.of_node;

	/* TX is enabled only when both DMA and Stereo TX channel
	 * is specified in the DTSi
	 */
	if (!(of_property_read_u32(np, "dma-tx-channel",
					&dai_priv[intf].mbox_tx)
		|| of_property_read_u32(np, "stereo-tx-port",
					&dai_priv[intf].stereo_tx))) {
		dai_priv[intf].tx_enabled = ENABLE;
	}

	/* RX is enabled only when both DMA and Stereo RX channel
	 * is specified in the DTSi.
	 */
	if (!(of_property_read_u32(np, "dma-rx-channel",
					&dai_priv[intf].mbox_rx))) {
		if (intf == SPDIF) {
			dai_priv[intf].rx_enabled = ENABLE;
			dai_priv[intf].stereo_rx = MAX_STEREO_ENTRIES;
		} else if (!(of_property_read_u32(np, "stereo-rx-port",
					&dai_priv[intf].stereo_rx))) {
			dai_priv[intf].rx_enabled = ENABLE;
		}
	}

	/* Either TX or Rx should have been enabled for a DMA/Stereo Channel */
	if (!(dai_priv[intf].tx_enabled || dai_priv[intf].rx_enabled)) {
		dev_err(&pdev->dev, "%s: error reading node properties\n",
								np->name);
		return -EFAULT;
	}

	/* Get Clks */
	audio_tx_mclk = devm_clk_get(&pdev->dev, "audio_tx_mclk");

	if (IS_ERR(audio_tx_mclk)) {
		dev_err(&pdev->dev, "Could not get tx_mclk\n");
		return PTR_ERR(audio_tx_mclk);
	}

	audio_tx_bclk = devm_clk_get(&pdev->dev, "audio_tx_bclk");

	if (IS_ERR(audio_tx_bclk)) {
		dev_err(&pdev->dev, "Could not get tx_bclk\n");
		return PTR_ERR(audio_tx_bclk);
	}

	audio_rx_mclk = devm_clk_get(&pdev->dev,
					"audio_rx_mclk");
	if (IS_ERR(audio_rx_mclk)) {
		dev_err(&pdev->dev, "Could not get rx_mclk\n");
		return PTR_ERR(audio_rx_mclk);
	}

	audio_rx_bclk = devm_clk_get(&pdev->dev, "audio_rx_bclk");
	if (IS_ERR(audio_rx_bclk)) {
		dev_err(&pdev->dev, "Could not get rx_bclk\n");
		return PTR_ERR(audio_rx_bclk);
	}

	dai_priv[intf].pdev = pdev;

	ret = snd_soc_register_component(&pdev->dev, &ipq_i2s_component,
		 ipq8074_cpu_dais, ARRAY_SIZE(ipq8074_cpu_dais));
	if (ret)
		dev_err(&pdev->dev,
			"ret: %d error registering soc dais\n", ret);

	return ret;
}

static int ipq_dai_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static struct platform_driver ipq_dai_driver = {
	.probe = ipq_dai_probe,
	.remove = ipq_dai_remove,
	.driver = {
		.name = "qca-cpu-dai",
		.of_match_table = ipq_cpu_dai_id_table,
	},
};

module_platform_driver(ipq_dai_driver);

MODULE_ALIAS("platform:qca-cpu-dai");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("IPQ CPU DAI DRIVER");
