/*
 * Copyright (c) 2016-2019, 2021 The Linux Foundation. All rights reserved.
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
#include <linux/mfd/syscon.h>

#include <linux/reset-controller.h>
#include <dt-bindings/clock/qca,apss-ipq5018.h>

#include "common.h"
#include "clk-regmap.h"
#include "clk-pll.h"
#include "clk-rcg.h"
#include "clk-branch.h"
#include "clk-alpha-pll.h"
#include "clk-regmap-divider.h"
#include "clk-regmap-mux.h"
#include "reset.h"

#define F(f, s, h, m, n) { (f), (s), (2 * (h) - 1), (m), (n) }

enum {
	P_XO,
	P_GPLL0,
	P_GPLL2,
	P_GPLL4,
	P_APSS_PLL_EARLY,
	P_APSS_PLL
};

/* Stromer PLL configs
 *  ALPHA_WIDTH : 40-bit
 *  CONFIG CTL  : 32-bit
 */
static struct clk_alpha_pll apss_pll_early = {
	.offset = 0x5000,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_STROMER],
	.clkr = {
		.enable_reg = 0x5000,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "apss_pll_early",
			.parent_names = (const char *[]){
				"xo"
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_stromer_ops,
			.flags = CLK_IS_CRITICAL,
		},
	},
};

static struct clk_alpha_pll_postdiv apss_pll = {
	.offset = 0x5000,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_STROMER],
	.width = 4,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "apss_pll",
		.parent_names = (const char *[]){ "apss_pll_early" },
		.num_parents = 1,
		.ops = &clk_alpha_pll_postdiv_ro_ops,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const char * const parents_apcs_alias0_clk_src[] = {
	"xo",
	"gpll0",
	"gpll2",
	"gpll4",
	"apss_pll",
	"apss_pll_early",
};

static const struct parent_map parents_apcs_alias0_clk_src_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 4 },
	{ P_GPLL2, 2 },
	{ P_GPLL4, 1 },
	{ P_APSS_PLL, 3 },
	{ P_APSS_PLL_EARLY, 5 },
};

static const struct freq_tbl ftbl_apcs_alias0_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	F(800000000, P_APSS_PLL_EARLY, 1, 0, 0),
	F(1008000000, P_APSS_PLL_EARLY, 1, 0, 0),
	{ }
};

static struct clk_rcg2 apcs_alias0_clk_src = {
	.cmd_rcgr = 0x0050,
	.freq_tbl = ftbl_apcs_alias0_clk_src,
	.hid_width = 5,
	.parent_map = parents_apcs_alias0_clk_src_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "apcs_alias0_clk_src",
		.parent_names = parents_apcs_alias0_clk_src,
		.num_parents = 6,
		.ops = &clk_rcg2_ops,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_branch apcs_alias0_core_clk = {
	.halt_reg = 0x0058,
	.clkr = {
		.enable_reg = 0x0058,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "apcs_alias0_core_clk",
			.parent_names = (const char *[]){
				"apcs_alias0_clk_src"
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT |
				CLK_IS_CRITICAL,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_regmap *apss_ipq5018_clks[] = {
	[APSS_PLL_EARLY] = &apss_pll_early.clkr,
	[APSS_PLL] = &apss_pll.clkr,
	[APCS_ALIAS0_CLK_SRC] = &apcs_alias0_clk_src.clkr,
	[APCS_ALIAS0_CORE_CLK] = &apcs_alias0_core_clk.clkr,
};

static const struct alpha_pll_config apss_pll_config = {
	.l = 0x2A,
	.alpha = 0x0,
	.alpha_hi = 0x0,
	.config_ctl_val = 0x4001075B,
	.main_output_mask = BIT(0),
	.aux_output_mask = BIT(1),
	.early_output_mask = BIT(3),
	.alpha_en_mask = BIT(24),
	.vco_val = 0x0,
	.vco_mask = GENMASK(21, 20),
	.status_reg_val = 0x3,
	.status_reg_mask = GENMASK(10, 8),
	.lock_det = BIT(2),
	.test_ctl_val = 0x0,
	.test_ctl_hi_val = 0x00400003,
};

static const struct regmap_config apss_ipq5018_regmap_config = {
	.reg_bits       = 32,
	.reg_stride     = 4,
	.val_bits       = 32,
	.max_register   = 0x5ffc,
	.fast_io        = true,
};

static const struct qcom_cc_desc apss_ipq5018_desc = {
	.config = &apss_ipq5018_regmap_config,
	.clks = apss_ipq5018_clks,
	.num_clks = ARRAY_SIZE(apss_ipq5018_clks),
};

static int apss_ipq5018_probe(struct platform_device *pdev)
{
	int ret;
	struct regmap *regmap;

	regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	clk_alpha_pll_configure(&apss_pll_early, regmap, &apss_pll_config);

	ret = qcom_cc_really_probe(pdev, &apss_ipq5018_desc, regmap);
	dev_dbg(&pdev->dev, "Registered ipq5018 apss clock provider\n");

	return ret;
}

static int apss_ipq5018_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver apss_ipq5018_driver = {
	.probe = apss_ipq5018_probe,
	.remove = apss_ipq5018_remove,
	.driver = {
		.name   = "qcom,apss-ipq5018",
		.owner  = THIS_MODULE,
	},
};

static int __init apss_ipq5018_init(void)
{
	return platform_driver_register(&apss_ipq5018_driver);
}
core_initcall(apss_ipq5018_init);

static void __exit apss_ipq5018_exit(void)
{
	platform_driver_unregister(&apss_ipq5018_driver);
}
module_exit(apss_ipq5018_exit);

MODULE_DESCRIPTION("QTI APSS IPQ5018 Driver");
MODULE_LICENSE("Dual BSD/GPLv2");
MODULE_ALIAS("platform:apss-ipq5018");
