/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/clk-provider.h>
#include <linux/regmap.h>

#include <linux/reset-controller.h>
#include <dt-bindings/clock/qca,adss-ipq8074.h>

#include "common.h"
#include "clk-regmap.h"
#include "clk-rcg.h"
#include "clk-branch.h"
#include "clk-regmap-divider.h"
#include "clk-regmap-mux.h"

#define F(f, s, h, m, n) { (f), (s), (2 * (h) - 1), (m), (n) }

enum {
	P_XO,
	P_AUDIO_PLL,
	P_RXMPAD_CLK,
	P_TXMPAD_CLK,
	P_PCMPAD_CLK,
};

static const char * const parents_audio_rxm_clk_src[] = {
	"xo",
	"audio_pll",
	"audio_rxmpad_clk"
};

static const struct parent_map parents_audio_rxm_clk_src_map[] = {
	{ P_XO, 0 },
	{ P_AUDIO_PLL, 1 },
	{ P_RXMPAD_CLK, 2 },
};

struct freq_tbl ftbl_audio_rxm_clk_src[] = {
	F(252,      P_RXMPAD_CLK, 1, 0, 0),
	F(16384000, P_AUDIO_PLL, 12, 0, 0),
	F(19200000, P_XO, 1, 0, 0),
	F(22579200, P_AUDIO_PLL, 8, 0, 0),
	F(24576000, P_AUDIO_PLL, 8, 0, 0),
	F(32768000, P_AUDIO_PLL, 6, 0, 0),
	F(45158400, P_AUDIO_PLL, 4, 0, 0),
	F(49152000, P_AUDIO_PLL, 4, 0, 0),
	F(61440000, P_AUDIO_PLL, 3, 0, 0),
	F(64512000, P_AUDIO_PLL, 3, 0, 0),
	F(65536000, P_AUDIO_PLL, 3, 0, 0),
	F(67737600, P_AUDIO_PLL, 3, 0, 0),
	F(70560000, P_AUDIO_PLL, 3, 0, 0),
	F(71680000, P_AUDIO_PLL, 3, 0, 0),
	F(86016000, P_AUDIO_PLL, 2, 0, 0),
	F(90316800, P_AUDIO_PLL, 2, 0, 0),
	F(92160000, P_AUDIO_PLL, 2, 0, 0),
	F(98304000, P_AUDIO_PLL, 2, 0, 0),
	F(98784000, P_AUDIO_PLL, 2, 0, 0),
	F(100352000, P_AUDIO_PLL, 2, 0, 0),
	F(102400000, P_AUDIO_PLL, 2, 0, 0),
	F(186278400, P_AUDIO_PLL, 1, 0, 0),
	F(196608000, P_AUDIO_PLL, 1, 0, 0),
	F(202752000, P_AUDIO_PLL, 1, 0, 0),
	{ }
};

struct clk_rcg2 adss_audio_rxm_clk_src = {
	.cmd_rcgr = 0x0020,
	.freq_tbl = ftbl_audio_rxm_clk_src,
	.hid_width = 5,
	.parent_map = parents_audio_rxm_clk_src_map,
	.flags = CLK_RCG2_HW_CONTROLLED,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "adss_audio_rxm_clk_src",
		.parent_names = parents_audio_rxm_clk_src,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap_div adss_audio_rxm_postdiv_clk_src = {
	.reg = 0x0028,
	.shift = 0,
	.width = 9,
	.clkr = {
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_rxm_postdiv_clk_src",
			.parent_names = (const char *[]){
				"adss_audio_rxm_clk_src"
			},
			.num_parents = 1,
			.ops = &clk_regmap_div_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_branch adss_audio_rxm_clk = {
	.halt_reg = 0x002c,
	.halt_bit = 31,
	.clkr = {
		.enable_reg = 0x002c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_rxm_clk",
			.parent_names = (const char *[]){
				"adss_audio_rxm_postdiv_clk_src"
			},
			.num_parents = 1,
			.ops = &clk_branch2_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_regmap_div adss_audio_rxb_postdiv_clk_src = {
	.reg = 0x0008,
	.shift = 1,
	.width = 8,
	.clkr = {
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_rxb_postdiv_clk_src",
			.parent_names = (const char *[]){
				"adss_audio_rxm_clk_src"
			},
			.num_parents = 1,
			.ops = &clk_regmap_div_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static const char * const parent_adss_audio_rxb_clk_mux[] = {
	"adss_audio_rxb_postdiv_clk_src",
	"audio_rxbpad_clk",
};

static struct clk_regmap_mux adss_audio_rxb_clk_mux = {
	.reg = 0x0004,
	.shift = 8,
	.width = 3,
	.clkr = {
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_rxb_clk_mux",
			.parent_names = parent_adss_audio_rxb_clk_mux,
			.num_parents = 2,
			.ops = &clk_regmap_mux_closest_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_branch adss_audio_rxb_clk = {
	.halt_reg = 0x000c,
	.halt_bit = 31,
	.clkr = {
		.enable_reg = 0x000c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_rxb_clk",
			.parent_names = (const char *[]){
				"adss_audio_rxb_clk_mux"
			},
			.num_parents = 1,
			.ops = &clk_branch2_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

struct freq_tbl ftbl_audio_txm_clk_src[] = {
	F(252,      P_TXMPAD_CLK, 1, 0, 0),
	F(16384000, P_AUDIO_PLL, 12, 0, 0),
	F(19200000, P_XO, 1, 0, 0),
	F(22579200, P_AUDIO_PLL, 8, 0, 0),
	F(24576000, P_AUDIO_PLL, 8, 0, 0),
	F(32768000, P_AUDIO_PLL, 6, 0, 0),
	F(45158400, P_AUDIO_PLL, 4, 0, 0),
	F(49152000, P_AUDIO_PLL, 4, 0, 0),
	F(61440000, P_AUDIO_PLL, 3, 0, 0),
	F(64512000, P_AUDIO_PLL, 3, 0, 0),
	F(65536000, P_AUDIO_PLL, 3, 0, 0),
	F(67737600, P_AUDIO_PLL, 3, 0, 0),
	F(70560000, P_AUDIO_PLL, 3, 0, 0),
	F(71680000, P_AUDIO_PLL, 3, 0, 0),
	F(86016000, P_AUDIO_PLL, 2, 0, 0),
	F(90316800, P_AUDIO_PLL, 2, 0, 0),
	F(92160000, P_AUDIO_PLL, 2, 0, 0),
	F(98304000, P_AUDIO_PLL, 2, 0, 0),
	F(98784000, P_AUDIO_PLL, 2, 0, 0),
	F(100352000, P_AUDIO_PLL, 2, 0, 0),
	F(102400000, P_AUDIO_PLL, 2, 0, 0),
	F(186278400, P_AUDIO_PLL, 1, 0, 0),
	F(196608000, P_AUDIO_PLL, 1, 0, 0),
	F(202752000, P_AUDIO_PLL, 1, 0, 0),
	{ }
};

static const char * const parents_audio_txm_clk_src[] = {
	"xo",
	"audio_pll",
	"audio_txmpad_clk"
};

static const struct parent_map parents_audio_txm_clk_src_map[] = {
	{ P_XO, 0 },
	{ P_AUDIO_PLL, 1 },
	{ P_TXMPAD_CLK, 2 },
};

struct clk_rcg2 adss_audio_txm_clk_src = {
	.cmd_rcgr = 0x0060,
	.freq_tbl = ftbl_audio_txm_clk_src,
	.hid_width = 5,
	.parent_map = parents_audio_txm_clk_src_map,
	.flags = CLK_RCG2_HW_CONTROLLED,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "adss_audio_txm_clk_src",
		.parent_names = parents_audio_txm_clk_src,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap_div adss_audio_txm_postdiv_clk_src = {
	.reg = 0x0068,
	.shift = 0,
	.width = 9,
	.clkr = {
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_txm_postdiv_clk_src",
			.parent_names = (const char *[]){
				"adss_audio_txm_clk_src"
			},
			.num_parents = 1,
			.ops = &clk_regmap_div_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_branch adss_audio_txm_clk = {
	.halt_reg = 0x006c,
	.halt_bit = 31,
	.clkr = {
		.enable_reg = 0x006c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_txm_clk",
			.parent_names = (const char *[]){
				"adss_audio_txm_postdiv_clk_src"
			},
			.num_parents = 1,
			.ops = &clk_branch2_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_regmap_div adss_audio_txb_postdiv_clk_src = {
	.reg = 0x0048,
	.shift = 1,
	.width = 8,
	.clkr = {
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_txb_postdiv_clk_src",
			.parent_names = (const char *[]){
				"adss_audio_txm_clk_src"
			},
			.num_parents = 1,
			.ops = &clk_regmap_div_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static const char * const parent_adss_audio_txb_clk_mux[] = {
	"adss_audio_txb_postdiv_clk_src",
	"audio_txbpad_clk",
};

static struct clk_regmap_mux adss_audio_txb_clk_mux = {
	.reg = 0x0044,
	.shift = 8,
	.width = 3,
	.clkr = {
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_txb_clk_mux",
			.parent_names = parent_adss_audio_txb_clk_mux,
			.num_parents = 2,
			.ops = &clk_regmap_mux_closest_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_branch adss_audio_txb_clk = {
	.halt_reg = 0x004c,
	.halt_bit = 31,
	.clkr = {
		.enable_reg = 0x004c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_txb_clk",
			.parent_names = (const char *[]){
				"adss_audio_txb_clk_mux"
			},
			.num_parents = 1,
			.ops = &clk_branch2_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

struct freq_tbl ftbl_audio_pcm_clk_src[] = {
	F(19200000, P_XO, 1, 0, 0),
	F(49152000, P_AUDIO_PLL, 4, 0, 0),
	{ }
};

static const char * const parents_audio_pcm_clk_src[] = {
	"xo",
	"audio_pll",
	"audio_pcmpad_clk"
};

static const struct parent_map parents_audio_pcm_clk_src_map[] = {
	{ P_XO, 0 },
	{ P_AUDIO_PLL, 1 },
	{ P_PCMPAD_CLK, 2 },
};

struct clk_rcg2 adss_audio_pcm_clk_src = {
	.cmd_rcgr = 0x00a0,
	.freq_tbl = ftbl_audio_pcm_clk_src,
	.hid_width = 5,
	.parent_map = parents_audio_pcm_clk_src_map,
	.flags = CLK_RCG2_HW_CONTROLLED,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "adss_audio_pcm_clk_src",
		.parent_names = parents_audio_txm_clk_src,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_regmap_div adss_audio_pcm_postdiv_clk_src = {
	.reg = 0x00a8,
	.shift = 0,
	.width = 9,
	.clkr = {
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_pcm_postdiv_clk_src",
			.parent_names = (const char *[]){
				"adss_audio_pcm_clk_src"
			},
			.num_parents = 1,
			.ops = &clk_regmap_div_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_branch adss_audio_pcm_clk = {
	.halt_reg = 0x00ac,
	.halt_bit = 31,
	.clkr = {
		.enable_reg = 0x00ac,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_pcm_clk",
			.parent_names = (const char *[]){
				"adss_audio_pcm_postdiv_clk_src"
			},
			.num_parents = 1,
			.ops = &clk_branch2_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_branch adss_audio_i2s0_clk = {
	.halt_reg = 0x0104,
	.halt_bit = 31,
	.clkr = {
		.enable_reg = 0x0104,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_i2s0_clk",
			.parent_names = (const char *[]){
				"pcnoc_clk_src"
			},
			.num_parents = 1,
			.ops = &clk_branch2_ops,
			.flags = CLK_IGNORE_UNUSED,
		},
	},
};

static struct clk_branch adss_audio_i2s3_clk = {
	.halt_reg = 0x0108,
	.halt_bit = 31,
	.clkr = {
		.enable_reg = 0x0108,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_i2s3_clk",
			.parent_names = (const char *[]){
				"pcnoc_clk_src"
			},
			.num_parents = 1,
			.ops = &clk_branch2_ops,
			.flags = CLK_IGNORE_UNUSED,
		},
	},
};

static struct clk_branch adss_audio_mbox0_clk = {
	.halt_reg = 0x010c,
	.halt_bit = 31,
	.clkr = {
		.enable_reg = 0x010c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_mbox0_clk",
			.parent_names = (const char *[]){
				"pcnoc_clk_src"
			},
			.num_parents = 1,
			.ops = &clk_branch2_ops,
			.flags = CLK_IGNORE_UNUSED,
		},
	},
};

static struct clk_branch adss_audio_mbox3_clk = {
	.halt_reg = 0x0110,
	.halt_bit = 31,
	.clkr = {
		.enable_reg = 0x0110,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "adss_audio_mbox3_clk",
			.parent_names = (const char *[]){
				"pcnoc_clk_src"
			},
			.num_parents = 1,
			.ops = &clk_branch2_ops,
			.flags = CLK_IGNORE_UNUSED,
		},
	},
};

static struct clk_regmap *adss_ipq807x_clks[] = {
	[ADSS_AUDIO_RXM_CLK_SRC] = &adss_audio_rxm_clk_src.clkr,
	[ADSS_AUDIO_RXM_POSTDIV_CLK_SRC] = &adss_audio_rxm_postdiv_clk_src.clkr,
	[ADSS_AUDIO_RXM_CLK] = &adss_audio_rxm_clk.clkr,
	[ADSS_AUDIO_RXB_POSTDIV_CLK_SRC] = &adss_audio_rxb_postdiv_clk_src.clkr,
	[ADSS_AUDIO_RXB_CLK_MUX] = &adss_audio_rxb_clk_mux.clkr,
	[ADSS_AUDIO_RXB_CLK] = &adss_audio_rxb_clk.clkr,
	[ADSS_AUDIO_TXM_CLK_SRC] = &adss_audio_txm_clk_src.clkr,
	[ADSS_AUDIO_TXM_POSTDIV_CLK_SRC] = &adss_audio_txm_postdiv_clk_src.clkr,
	[ADSS_AUDIO_TXM_CLK] = &adss_audio_txm_clk.clkr,
	[ADSS_AUDIO_TXB_POSTDIV_CLK_SRC] = &adss_audio_txb_postdiv_clk_src.clkr,
	[ADSS_AUDIO_TXB_CLK_MUX] = &adss_audio_txb_clk_mux.clkr,
	[ADSS_AUDIO_TXB_CLK] = &adss_audio_txb_clk.clkr,
	[ADSS_AUDIO_PCM_CLK_SRC] = &adss_audio_pcm_clk_src.clkr,
	[ADSS_AUDIO_PCM_POSTDIV_CLK_SRC] = &adss_audio_pcm_postdiv_clk_src.clkr,
	[ADSS_AUDIO_PCM_CLK] = &adss_audio_pcm_clk.clkr,
	[ADSS_AUDIO_I2S0_CLK] = &adss_audio_i2s0_clk.clkr,
	[ADSS_AUDIO_I2S3_CLK] = &adss_audio_i2s3_clk.clkr,
	[ADSS_AUDIO_MBOX0_CLK] = &adss_audio_mbox0_clk.clkr,
	[ADSS_AUDIO_MBOX3_CLK] = &adss_audio_mbox3_clk.clkr,
};

static const struct of_device_id adss_ipq807x_match_table[] = {
	{ .compatible = "qcom,adss-ipq807x" },
	{ }
};
MODULE_DEVICE_TABLE(of, adss_ipq807x_match_table);

static const struct regmap_config adss_ipq807x_regmap_config = {
	.reg_bits       = 32,
	.reg_stride     = 4,
	.val_bits       = 32,
	.max_register   = 0x1fc,
	.fast_io	= true,
};

static const struct qcom_cc_desc adss_ipq807x_desc = {
	.config = &adss_ipq807x_regmap_config,
	.clks = adss_ipq807x_clks,
	.num_clks = ARRAY_SIZE(adss_ipq807x_clks),
};

static int adss_ipq807x_probe(struct platform_device *pdev)
{
	int ret;

	ret = qcom_cc_probe(pdev, &adss_ipq807x_desc);

	return ret;
}

static int adss_ipq807x_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver adss_ipq807x_driver = {
	.probe = adss_ipq807x_probe,
	.remove = adss_ipq807x_remove,
	.driver = {
		.name   = "qcom,adss-ipq807x",
		.owner  = THIS_MODULE,
		.of_match_table = adss_ipq807x_match_table,
	},
};

static int __init adss_ipq807x_init(void)
{
	return platform_driver_register(&adss_ipq807x_driver);
}
core_initcall(adss_ipq807x_init);

static void __exit adss_ipq807x_exit(void)
{
	platform_driver_unregister(&adss_ipq807x_driver);
}
module_exit(adss_ipq807x_exit);

MODULE_DESCRIPTION("QTI APSS IPQ807x Driver");
MODULE_LICENSE("Dual BSD/GPLv2");
MODULE_ALIAS("platform:adss-ipq807x");
