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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/jack.h>
#include <linux/io.h>
#include <linux/pinctrl/consumer.h>
#include "ipq-adss.h"

SND_SOC_DAILINK_DEFS(i2s,
	DAILINK_COMP_ARRAY(COMP_CPU("qca-i2s-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC("qca_codec.0-0012", "qca-i2s-codec-dai")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("7709000.qca-pcm-i2s")));

SND_SOC_DAILINK_DEFS(tdm,
	DAILINK_COMP_ARRAY(COMP_CPU("qca-tdm-dai")),
	DAILINK_COMP_ARRAY(COMP_CODEC("qca_codec.0-0012", "qca-tdm-codec-dai")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("7709000.qca-pcm-tdm")));

static struct snd_soc_dai_link ipq8074_snd_dai[] = {
	{
		.name		= "IPQ Media1",
		.stream_name	= "I2S",
		.dai_fmt = (SND_SOC_DAIFMT_I2S |
				SND_SOC_DAIFMT_NB_NF |
				SND_SOC_DAIFMT_CBS_CFS),
		SND_SOC_DAILINK_REG(i2s),
	},
	{
		.name		= "IPQ Media2",
		.stream_name	= "TDM",
		SND_SOC_DAILINK_REG(tdm),
	},
};

static struct snd_soc_card snd_soc_card_ipq8074 = {
	.name		= "ipq8074_snd_card",
	.dai_link	= ipq8074_snd_dai,
	.num_links	= ARRAY_SIZE(ipq8074_snd_dai),
};

static const struct of_device_id ipq_audio_id_table[] = {
	{ .compatible = "qca,ipq8074-audio", .data = &snd_soc_card_ipq8074 },
	{},
};
MODULE_DEVICE_TABLE(of, ipq_audio_id_table);

static int ipq_audio_probe(struct platform_device *pdev)
{
	int ret;
	bool slave = false;
	const struct of_device_id *match;
	struct snd_soc_card *card;
	struct dev_pin_info *pins;
	struct pinctrl_state *pin_state;
	struct device_node *np;

	match = of_match_device(ipq_audio_id_table, &pdev->dev);
	if (!match)
		return -ENODEV;

	card = (struct snd_soc_card *)match->data;

	card->dev = &pdev->dev;
	pins = card->dev->pins;

	/*
	 * If the sound card registration fails, then the audio TLMM change
	 * is also reverted. Due to this, the pins are seen to toggle causing
	 * pop noise. To avoid this, the pins are set to GPIO state and moved
	 * to audio functionality only when the sound card registration is
	 * successful.
	 */

	np = of_find_node_by_name(NULL, "audio");
	if (np)
		slave = of_property_read_bool(np, "slave");

	/* If Slave property is not found in DTS, then
	 * Audio will be configured as master by default
	 */
	if (slave)
		pin_state = pinctrl_lookup_state(pins->p, "audio_slave");
	else
		pin_state = pinctrl_lookup_state(pins->p, "audio");

	if (IS_ERR(pin_state)) {
		pr_err("audio pinctrl state not available\n");
		return PTR_ERR(pin_state);
	}

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card() failed:%d\n", ret);
		return ret;
	}

	ipq_audio_adss_init();

	pinctrl_select_state(pins->p, pin_state);

	return ret;
}

static struct platform_driver ipq_audio_driver = {
	.driver = {
		.name = "ipq_audio",
		.of_match_table = ipq_audio_id_table,
	},
	.probe = ipq_audio_probe,
};

module_platform_driver(ipq_audio_driver);

MODULE_ALIAS("platform:ipq_audio");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("ALSA SoC IPQ Machine Driver");
