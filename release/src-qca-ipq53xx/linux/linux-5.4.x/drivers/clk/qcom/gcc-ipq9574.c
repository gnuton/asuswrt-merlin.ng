// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017,2018,2020 The Linux Foundation. All rights reserved.
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
#include <dt-bindings/clock/qcom,gcc-ipq9574.h>
#include <dt-bindings/reset/qcom,gcc-ipq9574.h>

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
	P_PCIE30_PHY0_PIPE,
	P_PCIE30_PHY1_PIPE,
	P_PCIE30_PHY2_PIPE,
	P_PCIE30_PHY3_PIPE,
	P_USB3PHY_0_PIPE,
	P_GPLL0,
	P_GPLL0_DIV2,
	P_GPLL0_OUT_AUX,
	P_GPLL2,
	P_GPLL4,
	P_PI_SLEEP,
	P_BIAS_PLL_UBI_NC_CLK,
};

static struct clk_alpha_pll gpll0_main = {
	.offset = 0x20000,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_DEFAULT_EVO],
	.clkr = {
		.enable_reg = 0x0b000,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gpll0_main",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "xo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_ops,
			.flags = CLK_IS_CRITICAL,
		},
	},
};

static struct clk_fixed_factor gpll0_out_main_div2 = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "gpll0_out_main_div2",
		.parent_hws = (const struct clk_hw *[]){
				&gpll0_main.clkr.hw },
		.num_parents = 1,
		.ops = &clk_fixed_factor_ops,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_alpha_pll_postdiv gpll0 = {
	.offset = 0x20000,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_DEFAULT_EVO],
	.width = 4,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "gpll0",
		.parent_hws = (const struct clk_hw *[]){
				&gpll0_main.clkr.hw },
		.num_parents = 1,
		.ops = &clk_alpha_pll_postdiv_ro_ops,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_alpha_pll gpll4_main = {
	.offset = 0x22000,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_DEFAULT_EVO],
	.clkr = {
		.enable_reg = 0x0b000,
		.enable_mask = BIT(2),
		.hw.init = &(struct clk_init_data){
			.name = "gpll4_main",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "xo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_ops,
			.flags = CLK_IS_CRITICAL,
		},
	},
};

static struct clk_alpha_pll_postdiv gpll4 = {
	.offset = 0x22000,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_DEFAULT_EVO],
	.width = 4,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "gpll4",
		.parent_hws = (const struct clk_hw *[]){
				&gpll4_main.clkr.hw },
		.num_parents = 1,
		.ops = &clk_alpha_pll_postdiv_ro_ops,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_alpha_pll gpll2_main = {
	.offset = 0x21000,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_DEFAULT_EVO],
	.clkr = {
		.enable_reg = 0x0b000,
		.enable_mask = BIT(1),
		.hw.init = &(struct clk_init_data){
			.name = "gpll2_main",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "xo",
			},
			.num_parents = 1,
			.ops = &clk_alpha_pll_ops,
			.flags = CLK_IS_CRITICAL,
		},
	},
};

static struct clk_alpha_pll_postdiv gpll2 = {
	.offset = 0x21000,
	.regs = clk_alpha_pll_regs[CLK_ALPHA_PLL_TYPE_DEFAULT_EVO],
	.width = 4,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "gpll2",
		.parent_hws = (const struct clk_hw *[]){
				&gpll2_main.clkr.hw },
		.num_parents = 1,
		.ops = &clk_alpha_pll_postdiv_ro_ops,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_branch gcc_sleep_clk_src = {
	.halt_reg = 0x3400c,
	.clkr = {
		.enable_reg = 0x3400c,
		.enable_mask = BIT(1),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_sleep_clk_src",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "sleep_clk",
			},
			.num_parents = 1,
			.ops = &clk_branch2_ops,
			.flags = CLK_IS_CRITICAL,
		},
	},
};

static const struct clk_parent_data gcc_xo_gpll0_gpll0_out_main_div2[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw},
	{ .hw = &gpll0_out_main_div2.hw},
};

static const struct parent_map gcc_xo_gpll0_gpll0_out_main_div2_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL0_DIV2, 4 },
};

static const struct clk_parent_data gcc_xo_gpll0[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
};

static const struct parent_map gcc_xo_gpll0_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
};

static const struct clk_parent_data gcc_xo_gpll0_gpll4[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll4.clkr.hw },
};

static const struct parent_map gcc_xo_gpll0_gpll4_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL4, 2 },
};

static const struct clk_parent_data gcc_xo_gpll0_gpll0_div2_gpll0[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll0_out_main_div2.hw },
	{ .hw = &gpll0.clkr.hw },
};

static const struct parent_map
gcc_xo_gpll0_gpll0_div2_gpll0_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL0_DIV2, 4 },
	{ P_GPLL0, 5 },
};

static const struct clk_parent_data gcc_xo_gpll0_gpll0_sleep_clk[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll0_out_main_div2.hw },
	{ .fw_name = "sleep_clk" },
};

static const struct parent_map gcc_xo_gpll0_gpll0_sleep_clk_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL0_DIV2, 4 },
	{ P_PI_SLEEP, 6 },
};

static const struct clk_parent_data gcc_xo_gpll0_core_pi_sleep_clk[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .fw_name = "sleep_clk" },
};

static const struct parent_map gcc_xo_gpll0_core_pi_sleep_clk_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 2 },
	{ P_PI_SLEEP, 6 },
};

static const struct clk_parent_data gcc_xo_gpll0_gpll4_bias_pll_ubi_nc_clk[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll4.clkr.hw },
	{ .fw_name = "bias_pll_ubi_nc_clk" },
};

static const struct parent_map gcc_xo_gpll0_gpll4_bias_pll_ubi_nc_clk_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL4, 2 },
	{ P_BIAS_PLL_UBI_NC_CLK, 3 },
};

static const struct clk_parent_data gcc_pcie30_phy0_pipe_clk_xo[] = {
	{ .fw_name = "pcie30_phy0_pipe_clk" },
	{ .fw_name = "xo" },
};

static const struct parent_map gcc_pcie30_phy0_pipe_clk_xo_map[] = {
	{ P_PCIE30_PHY0_PIPE, 0 },
	{ P_XO, 2 },
};

static const struct clk_parent_data gcc_pcie30_phy1_pipe_clk_xo[] = {
	{ .fw_name = "pcie30_phy1_pipe_clk" },
	{ .fw_name = "xo" },
};

static const struct parent_map gcc_pcie30_phy1_pipe_clk_xo_map[] = {
	{ P_PCIE30_PHY1_PIPE, 0 },
	{ P_XO, 2 },
};

static const struct clk_parent_data gcc_pcie30_phy2_pipe_clk_xo[] = {
	{ .fw_name = "pcie30_phy2_pipe_clk" },
	{ .fw_name = "xo" },
};

static const struct parent_map gcc_pcie30_phy2_pipe_clk_xo_map[] = {
	{ P_PCIE30_PHY2_PIPE, 0 },
	{ P_XO, 2 },
};

static const struct clk_parent_data gcc_pcie30_phy3_pipe_clk_xo[] = {
	{ .fw_name = "pcie30_phy3_pipe_clk" },
	{ .fw_name = "xo" },
};

static const struct parent_map gcc_pcie30_phy3_pipe_clk_xo_map[] = {
	{ P_PCIE30_PHY3_PIPE, 0 },
	{ P_XO, 2 },
};

static const struct clk_parent_data
			gcc_xo_gpll0_gpll0_aux_core_pi_sleep_clk[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll0.clkr.hw },
	{ .fw_name = "sleep_clk" },
};

static const struct parent_map
			gcc_xo_gpll0_gpll0_aux_core_pi_sleep_clk_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL0_OUT_AUX, 2 },
	{ P_PI_SLEEP, 6 },
};

static const struct clk_parent_data gcc_xo_gpll0_out_main_div2_gpll0[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll0_out_main_div2.hw },
};

static const struct parent_map gcc_xo_gpll0_out_main_div2_gpll0_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL0_DIV2, 4 },
};

static const struct clk_parent_data
			gcc_xo_gpll4_gpll0_gpll0_out_main_div2[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll4.clkr.hw },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll0_out_main_div2.hw },
};

static const struct parent_map gcc_xo_gpll4_gpll0_gpll0_out_main_div2_map[] = {
	{ P_XO, 0 },
	{ P_GPLL4, 1 },
	{ P_GPLL0, 3 },
	{ P_GPLL0_DIV2, 4 },
};

static const struct clk_parent_data gcc_usb3phy_0_cc_pipe_clk_xo[] = {
	{ .fw_name = "usb3phy_0_cc_pipe_clk" },
	{ .fw_name = "xo" },
};

static const struct parent_map gcc_usb3phy_0_cc_pipe_clk_xo_map[] = {
	{ P_USB3PHY_0_PIPE, 0 },
	{ P_XO, 2 },
};

static const struct clk_parent_data
			gcc_xo_gpll0_gpll2_gpll0_out_main_div2[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll2.clkr.hw },
	{ .hw = &gpll0_out_main_div2.hw },
};

static const struct parent_map gcc_xo_gpll0_gpll2_gpll0_out_main_div2_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL2, 2 },
	{ P_GPLL0_DIV2, 4 },
};

static const struct clk_parent_data gcc_xo_gpll0_gpll4_gpll0_div2[] = {
	{ .fw_name = "xo"},
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll4.clkr.hw },
	{ .hw = &gpll0_out_main_div2.hw },
};

static const struct parent_map gcc_xo_gpll0_gpll4_gpll0_div2_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL4, 2 },
	{ P_GPLL0_DIV2, 4 },
};

static const struct clk_parent_data gcc_xo_gpll0_gpll0_div2_gpll4[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll0_out_main_div2.hw },
	{ .hw = &gpll4.clkr.hw },
};

static const struct parent_map gcc_xo_gpll0_gpll0_div2_gpll4_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL0_DIV2, 4 },
	{ P_GPLL4, 5 },
};

static const struct clk_parent_data gcc_xo_gpll0_main_div2[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll0_out_main_div2.hw },
};

static const struct parent_map gcc_xo_gpll0_main_div2_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL0_DIV2, 2 },
};

static const struct clk_parent_data gcc_xo_gpll4_gpll0_gpll0_div2[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll4.clkr.hw },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll0_out_main_div2.hw },
};

static const struct parent_map gcc_xo_gpll4_gpll0_gpll0_div2_map[] = {
	{ P_XO, 0 },
	{ P_GPLL4, 1 },
	{ P_GPLL0, 2 },
	{ P_GPLL0_DIV2, 4 },
};

static const struct clk_parent_data gcc_xo_gpll0_gpll2[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll2.clkr.hw },
};

static const struct parent_map gcc_xo_gpll0_gpll2_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL2, 2 },
};

static const struct clk_parent_data gcc_xo_gpll0_gpll2_gpll4_pi_sleep[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll2.clkr.hw },
	{ .hw = &gpll4.clkr.hw },
	{ .fw_name = "sleep_clk" },
};

static const struct parent_map gcc_xo_gpll0_gpll2_gpll4_pi_sleep_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL2, 2 },
	{ P_GPLL4, 3 },
	{ P_PI_SLEEP, 6 },
};

static const struct clk_parent_data gcc_xo_gpll0_gpll0_aux_gpll2[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll2.clkr.hw },
};

static const struct parent_map gcc_xo_gpll0_gpll0_aux_gpll2_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL0_OUT_AUX, 2 },
	{ P_GPLL2, 3 },
};

static const struct clk_parent_data
			gcc_xo_gpll0_gpll4_gpll2_pi_sleep[] = {
	{ .fw_name = "xo" },
	{ .hw = &gpll0.clkr.hw },
	{ .hw = &gpll4.clkr.hw },
	{ .hw = &gpll2.clkr.hw },
	{ .fw_name = "sleep_clk" },
};

static const struct parent_map
			gcc_xo_gpll0_gpll4_gpll2_pi_sleep_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL4, 3 },
	{ P_GPLL2, 4 },
	{ P_PI_SLEEP, 6 },
};

static const struct clk_parent_data gcc_xo_pi_sleep_clk[] = {
	{ .fw_name = "xo" },
	{ .fw_name = "sleep_clk" },
};

static const struct parent_map gcc_xo_pi_sleep_clk_map[] = {
	{ P_XO, 0 },
	{ P_PI_SLEEP, 6 },
};

static const struct freq_tbl ftbl_apss_ahb_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	F(50000000, P_GPLL0, 16, 0, 0),
	F(100000000, P_GPLL0, 8, 0, 0),
	{ }
};

static struct clk_rcg2 apss_ahb_clk_src = {
	.cmd_rcgr = 0x2400c,
	.freq_tbl = ftbl_apss_ahb_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "apss_ahb_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static const struct freq_tbl ftbl_apss_axi_clk_src[] = {
	F(533000000, P_GPLL0, 1.5, 0, 0),
	{ }
};

static struct clk_rcg2 apss_axi_clk_src = {
	.cmd_rcgr = 0x24004,
	.freq_tbl = ftbl_apss_axi_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_div2_gpll0_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "apss_axi_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_div2_gpll0,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static const struct freq_tbl ftbl_blsp1_qup_i2c_apps_clk_src[] = {
	F(9600000, P_XO, 2.5, 0, 0),
	F(24000000, P_XO, 1, 0, 0),
	F(50000000, P_GPLL0, 16, 0, 0),
	{ }
};

static struct clk_rcg2 blsp1_qup1_i2c_apps_clk_src = {
	.cmd_rcgr = 0x02018,
	.freq_tbl = ftbl_blsp1_qup_i2c_apps_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_qup1_i2c_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static const struct freq_tbl ftbl_blsp1_qup_spi_apps_clk_src[] = {
	F(960000, P_XO, 10, 2, 5),
	F(4800000, P_XO, 5, 0, 0),
	F(9600000, P_XO, 2, 4, 5),
	F(16000000, P_GPLL0, 10, 1, 5),
	F(24000000, P_XO, 1, 0, 0),
	F(25000000, P_GPLL0, 16, 1, 2),
	F(50000000, P_GPLL0, 16, 0, 0),
	{ }
};

static struct clk_rcg2 blsp1_qup1_spi_apps_clk_src = {
	.cmd_rcgr = 0x02004,
	.freq_tbl = ftbl_blsp1_qup_spi_apps_clk_src,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_qup1_spi_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_qup2_i2c_apps_clk_src = {
	.cmd_rcgr = 0x03018,
	.freq_tbl = ftbl_blsp1_qup_i2c_apps_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_qup2_i2c_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_qup2_spi_apps_clk_src = {
	.cmd_rcgr = 0x03004,
	.freq_tbl = ftbl_blsp1_qup_spi_apps_clk_src,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_qup2_spi_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_qup3_i2c_apps_clk_src = {
	.cmd_rcgr = 0x04018,
	.freq_tbl = ftbl_blsp1_qup_i2c_apps_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_qup3_i2c_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_qup3_spi_apps_clk_src = {
	.cmd_rcgr = 0x04004,
	.freq_tbl = ftbl_blsp1_qup_spi_apps_clk_src,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_qup3_spi_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_qup4_i2c_apps_clk_src = {
	.cmd_rcgr = 0x05018,
	.freq_tbl = ftbl_blsp1_qup_i2c_apps_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_qup4_i2c_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_qup4_spi_apps_clk_src = {
	.cmd_rcgr = 0x05004,
	.freq_tbl = ftbl_blsp1_qup_spi_apps_clk_src,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_qup4_spi_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_qup5_i2c_apps_clk_src = {
	.cmd_rcgr = 0x06018,
	.freq_tbl = ftbl_blsp1_qup_i2c_apps_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_qup5_i2c_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_qup5_spi_apps_clk_src = {
	.cmd_rcgr = 0x06004,
	.freq_tbl = ftbl_blsp1_qup_spi_apps_clk_src,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_qup5_spi_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_qup6_i2c_apps_clk_src = {
	.cmd_rcgr = 0x07018,
	.freq_tbl = ftbl_blsp1_qup_i2c_apps_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_qup6_i2c_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_qup6_spi_apps_clk_src = {
	.cmd_rcgr = 0x07004,
	.freq_tbl = ftbl_blsp1_qup_spi_apps_clk_src,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_qup6_spi_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static const struct freq_tbl ftbl_blsp1_uart_apps_clk_src[] = {
	F(3686400, P_GPLL0_DIV2, 1, 144, 15625),
	F(7372800, P_GPLL0_DIV2, 1, 288, 15625),
	F(14745600, P_GPLL0_DIV2, 1, 576, 15625),
	F(24000000, P_XO, 1, 0, 0),
	F(25000000, P_GPLL0, 16, 1, 2),
	F(32000000, P_GPLL0, 1, 1, 25),
	F(40000000, P_GPLL0, 1, 1, 20),
	F(46400000, P_GPLL0, 1, 29, 500),
	F(48000000, P_GPLL0, 1, 3, 50),
	F(51200000, P_GPLL0, 1, 8, 125),
	F(56000000, P_GPLL0, 1, 7, 100),
	F(58982400, P_GPLL0, 1, 1152, 15625),
	F(60000000, P_GPLL0, 1, 3, 40),
	F(64000000, P_GPLL0, 12.5, 0, 0),
	{ }
};

static struct clk_rcg2 blsp1_uart1_apps_clk_src = {
	.cmd_rcgr = 0x0202c,
	.freq_tbl = ftbl_blsp1_uart_apps_clk_src,
	.mnd_width = 16,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_uart1_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_uart2_apps_clk_src = {
	.cmd_rcgr = 0x0302c,
	.freq_tbl = ftbl_blsp1_uart_apps_clk_src,
	.mnd_width = 16,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_uart2_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_uart3_apps_clk_src = {
	.cmd_rcgr = 0x0402c,
	.freq_tbl = ftbl_blsp1_uart_apps_clk_src,
	.mnd_width = 16,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_uart3_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_uart4_apps_clk_src = {
	.cmd_rcgr = 0x0502c,
	.freq_tbl = ftbl_blsp1_uart_apps_clk_src,
	.mnd_width = 16,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_uart4_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_uart5_apps_clk_src = {
	.cmd_rcgr = 0x0602c,
	.freq_tbl = ftbl_blsp1_uart_apps_clk_src,
	.mnd_width = 16,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_uart5_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 blsp1_uart6_apps_clk_src = {
	.cmd_rcgr = 0x0702c,
	.freq_tbl = ftbl_blsp1_uart_apps_clk_src,
	.mnd_width = 16,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "blsp1_uart6_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_apss_ahb_clk = {
	.halt_reg = 0x24018,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x0b004,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_apss_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&apss_ahb_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IS_CRITICAL,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_apss_axi_clk = {
	.halt_reg = 0x2401c,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x0b004,
		.enable_mask = BIT(1),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_apss_axi_clk",
			.parent_hws = (const struct clk_hw *[]){
					&apss_axi_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IS_CRITICAL,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_qup1_i2c_apps_clk = {
	.halt_reg = 0x2024,
	.clkr = {
		.enable_reg = 0x2024,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_qup1_i2c_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_qup1_i2c_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_qup1_spi_apps_clk = {
	.halt_reg = 0x02020,
	.clkr = {
		.enable_reg = 0x02020,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_qup1_spi_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_qup1_spi_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_qup2_i2c_apps_clk = {
	.halt_reg = 0x03024,
	.clkr = {
		.enable_reg = 0x03024,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_qup2_i2c_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_qup2_i2c_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_qup2_spi_apps_clk = {
	.halt_reg = 0x03020,
	.clkr = {
		.enable_reg = 0x03020,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_qup2_spi_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_qup2_spi_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_qup3_i2c_apps_clk = {
	.halt_reg = 0x04024,
	.clkr = {
		.enable_reg = 0x04024,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_qup3_i2c_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_qup3_i2c_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_qup3_spi_apps_clk = {
	.halt_reg = 0x04020,
	.clkr = {
		.enable_reg = 0x04020,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_qup3_spi_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_qup3_spi_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_qup4_i2c_apps_clk = {
	.halt_reg = 0x05024,
	.clkr = {
		.enable_reg = 0x05024,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_qup4_i2c_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_qup4_i2c_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_qup4_spi_apps_clk = {
	.halt_reg = 0x05020,
	.clkr = {
		.enable_reg = 0x05020,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_qup4_spi_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_qup4_spi_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_qup5_i2c_apps_clk = {
	.halt_reg = 0x06024,
	.clkr = {
		.enable_reg = 0x06024,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_qup5_i2c_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_qup5_i2c_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_qup5_spi_apps_clk = {
	.halt_reg = 0x06020,
	.clkr = {
		.enable_reg = 0x06020,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_qup5_spi_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_qup5_spi_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_qup6_i2c_apps_clk = {
	.halt_reg = 0x07024,
	.clkr = {
		.enable_reg = 0x07024,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_qup6_i2c_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_qup6_i2c_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_qup6_spi_apps_clk = {
	.halt_reg = 0x07020,
	.clkr = {
		.enable_reg = 0x07020,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_qup6_spi_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_qup6_spi_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_uart1_apps_clk = {
	.halt_reg = 0x02040,
	.clkr = {
		.enable_reg = 0x02040,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_uart1_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_uart1_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_uart2_apps_clk = {
	.halt_reg = 0x03040,
	.clkr = {
		.enable_reg = 0x03040,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_uart2_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_uart2_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_uart3_apps_clk = {
	.halt_reg = 0x04054,
	.clkr = {
		.enable_reg = 0x04054,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_uart3_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_uart3_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_uart4_apps_clk = {
	.halt_reg = 0x05040,
	.clkr = {
		.enable_reg = 0x05040,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_uart4_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_uart4_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_uart5_apps_clk = {
	.halt_reg = 0x06040,
	.clkr = {
		.enable_reg = 0x06040,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_uart5_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_uart5_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_uart6_apps_clk = {
	.halt_reg = 0x07040,
	.clkr = {
		.enable_reg = 0x07040,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_uart6_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&blsp1_uart6_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_pcie0_axi_m_clk_src[] = {
	F(240000000, P_GPLL4, 5, 0, 0),
	{ }
};

static struct clk_rcg2 pcie0_axi_m_clk_src = {
	.cmd_rcgr = 0x28018,
	.freq_tbl = ftbl_pcie0_axi_m_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll4_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie0_axi_m_clk_src",
		.parent_data = gcc_xo_gpll0_gpll4,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie0_axi_m_clk = {
	.halt_reg = 0x28038,
	.clkr = {
		.enable_reg = 0x28038,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie0_axi_m_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie0_axi_m_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_anoc_pcie0_1lane_m_clk = {
	.halt_reg = 0x2E07C,
	.clkr = {
		.enable_reg = 0x2E07C,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_anoc_pcie0_1lane_m_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie0_axi_m_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_rcg2 pcie1_axi_m_clk_src = {
	.cmd_rcgr = 0x29018,
	.freq_tbl = ftbl_pcie0_axi_m_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll4_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie1_axi_m_clk_src",
		.parent_data = gcc_xo_gpll0_gpll4,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie1_axi_m_clk = {
	.halt_reg = 0x29038,
	.clkr = {
		.enable_reg = 0x29038,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie1_axi_m_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie1_axi_m_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_anoc_pcie1_1lane_m_clk = {
	.halt_reg = 0x2E08C,
	.clkr = {
		.enable_reg = 0x2E08C,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_anoc_pcie1_1lane_m_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie1_axi_m_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_pcie2_axi_m_clk_src[] = {
	F(342857143, P_GPLL4, 3.5, 0, 0),
	{ }
};

static struct clk_rcg2 pcie2_axi_m_clk_src = {
	.cmd_rcgr = 0x2a018,
	.freq_tbl = ftbl_pcie2_axi_m_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll4_bias_pll_ubi_nc_clk_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie2_axi_m_clk_src",
		.parent_data = gcc_xo_gpll0_gpll4_bias_pll_ubi_nc_clk,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie2_axi_m_clk = {
	.halt_reg = 0x2a038,
	.clkr = {
		.enable_reg = 0x2a038,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie2_axi_m_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie2_axi_m_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_anoc_pcie2_2lane_m_clk = {
	.halt_reg = 0x2E080,
	.clkr = {
		.enable_reg = 0x2E080,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_anoc_pcie2_2lane_m_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie2_axi_m_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_rcg2 pcie3_axi_m_clk_src = {
	.cmd_rcgr = 0x2b018,
	.freq_tbl = ftbl_pcie2_axi_m_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll4_bias_pll_ubi_nc_clk_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie3_axi_m_clk_src",
		.parent_data = gcc_xo_gpll0_gpll4_bias_pll_ubi_nc_clk,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie3_axi_m_clk = {
	.halt_reg = 0x2b038,
	.clkr = {
		.enable_reg = 0x2b038,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie3_axi_m_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie3_axi_m_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_anoc_pcie3_2lane_m_clk = {
	.halt_reg = 0x2E090,
	.clkr = {
		.enable_reg = 0x2E090,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_anoc_pcie3_2lane_m_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie3_axi_m_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_rcg2 pcie0_axi_s_clk_src = {
	.cmd_rcgr = 0x28020,
	.freq_tbl = ftbl_pcie0_axi_m_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll4_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie0_axi_s_clk_src",
		.parent_data = gcc_xo_gpll0_gpll4,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie0_axi_s_clk = {
	.halt_reg = 0x2803c,
	.clkr = {
		.enable_reg = 0x2803c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie0_axi_s_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie0_axi_s_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcie0_axi_s_bridge_clk = {
	.halt_reg = 0x28040,
	.clkr = {
		.enable_reg = 0x28040,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie0_axi_s_bridge_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie0_axi_s_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_snoc_pcie0_1lane_s_clk = {
	.halt_reg = 0x2E048,
	.clkr = {
		.enable_reg = 0x2E048,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_snoc_pcie0_1lane_s_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie0_axi_s_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_rcg2 pcie1_axi_s_clk_src = {
	.cmd_rcgr = 0x29020,
	.freq_tbl = ftbl_pcie0_axi_m_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll4_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie1_axi_s_clk_src",
		.parent_data = gcc_xo_gpll0_gpll4,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie1_axi_s_clk = {
	.halt_reg = 0x2903c,
	.clkr = {
		.enable_reg = 0x2903c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie1_axi_s_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie1_axi_s_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcie1_axi_s_bridge_clk = {
	.halt_reg = 0x29040,
	.clkr = {
		.enable_reg = 0x29040,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie1_axi_s_bridge_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie1_axi_s_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_snoc_pcie1_1lane_s_clk = {
	.halt_reg = 0x2E04C,
	.clkr = {
		.enable_reg = 0x2E04C,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_snoc_pcie1_1lane_s_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie1_axi_s_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_rcg2 pcie2_axi_s_clk_src = {
	.cmd_rcgr = 0x2a020,
	.freq_tbl = ftbl_pcie0_axi_m_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll4_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie2_axi_s_clk_src",
		.parent_data = gcc_xo_gpll0_gpll4,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie2_axi_s_clk = {
	.halt_reg = 0x2a03c,
	.clkr = {
		.enable_reg = 0x2a03c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie2_axi_s_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie2_axi_s_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcie2_axi_s_bridge_clk = {
	.halt_reg = 0x2a040,
	.clkr = {
		.enable_reg = 0x2a040,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie2_axi_s_bridge_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie2_axi_s_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_snoc_pcie2_2lane_s_clk = {
	.halt_reg = 0x2E050,
	.clkr = {
		.enable_reg = 0x2E050,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_snoc_pcie2_2lane_s_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie2_axi_s_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_rcg2 pcie3_axi_s_clk_src = {
	.cmd_rcgr = 0x2b020,
	.freq_tbl = ftbl_pcie0_axi_m_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll4_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie3_axi_s_clk_src",
		.parent_data = gcc_xo_gpll0_gpll4,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie3_axi_s_clk = {
	.halt_reg = 0x2b03c,
	.clkr = {
		.enable_reg = 0x2b03c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie3_axi_s_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie3_axi_s_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcie3_axi_s_bridge_clk = {
	.halt_reg = 0x2b040,
	.clkr = {
		.enable_reg = 0x2b040,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie3_axi_s_bridge_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie3_axi_s_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_snoc_pcie3_2lane_s_clk = {
	.halt_reg = 0x2E054,
	.clkr = {
		.enable_reg = 0x2E054,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_snoc_pcie3_2lane_s_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie3_axi_s_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_regmap_mux pcie0_pipe_clk_src = {
	.reg = 0x28064,
	.shift = 8,
	.width = 2,
	.parent_map = gcc_pcie30_phy0_pipe_clk_xo_map,
	.clkr = {
		.hw.init = &(struct clk_init_data){
			.name = "pcie0_pipe_clk_src",
			.parent_data = gcc_pcie30_phy0_pipe_clk_xo,
			.num_parents = 2,
			.ops = &clk_regmap_mux_closest_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_branch gcc_pcie0_pipe_clk = {
	.halt_reg = 0x28044,
	.halt_check = BRANCH_HALT_DELAY,
	.clkr = {
		.enable_reg = 0x28044,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie0_pipe_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie0_pipe_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_regmap_mux pcie1_pipe_clk_src = {
	.reg = 0x29064,
	.shift = 8,
	.width = 2,
	.parent_map = gcc_pcie30_phy1_pipe_clk_xo_map,
	.clkr = {
		.hw.init = &(struct clk_init_data){
			.name = "pcie1_pipe_clk_src",
			.parent_data = gcc_pcie30_phy1_pipe_clk_xo,
			.num_parents = 2,
			.ops = &clk_regmap_mux_closest_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_branch gcc_pcie1_pipe_clk = {
	.halt_reg = 0x29044,
	.halt_check = BRANCH_HALT_DELAY,
	.clkr = {
		.enable_reg = 0x29044,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie1_pipe_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie1_pipe_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_regmap_mux pcie2_pipe_clk_src = {
	.reg = 0x2a064,
	.shift = 8,
	.width = 2,
	.parent_map = gcc_pcie30_phy2_pipe_clk_xo_map,
	.clkr = {
		.hw.init = &(struct clk_init_data){
			.name = "pcie2_pipe_clk_src",
			.parent_data = gcc_pcie30_phy2_pipe_clk_xo,
			.num_parents = 2,
			.ops = &clk_regmap_mux_closest_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_branch gcc_pcie2_pipe_clk = {
	.halt_reg = 0x2a044,
	.halt_check = BRANCH_HALT_DELAY,
	.clkr = {
		.enable_reg = 0x2a044,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie2_pipe_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie2_pipe_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_regmap_mux pcie3_pipe_clk_src = {
	.reg = 0x2b064,
	.shift = 8,
	.width = 2,
	.parent_map = gcc_pcie30_phy3_pipe_clk_xo_map,
	.clkr = {
		.hw.init = &(struct clk_init_data){
			.name = "pcie3_pipe_clk_src",
			.parent_data = gcc_pcie30_phy3_pipe_clk_xo,
			.num_parents = 2,
			.ops = &clk_regmap_mux_closest_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_branch gcc_pcie3_pipe_clk = {
	.halt_reg = 0x2b044,
	.halt_check = BRANCH_HALT_DELAY,
	.clkr = {
		.enable_reg = 0x2b044,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie3_pipe_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie3_pipe_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_pcie_rchng_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	F(100000000, P_GPLL0, 8, 0, 0),
	{ }
};

static struct clk_rcg2 pcie0_rchng_clk_src = {
	.cmd_rcgr = 0x28028,
	.freq_tbl = ftbl_pcie_rchng_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie0_rchng_clk_src",
		.parent_data = gcc_xo_gpll0,
		.num_parents = 2,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie0_rchng_clk = {
	.halt_reg = 0x28028,
	.clkr = {
		.enable_reg = 0x28028,
		.enable_mask = BIT(1),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie0_rchng_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie0_rchng_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_rcg2 pcie1_rchng_clk_src = {
	.cmd_rcgr = 0x29028,
	.freq_tbl = ftbl_pcie_rchng_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie1_rchng_clk_src",
		.parent_data = gcc_xo_gpll0,
		.num_parents = 2,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie1_rchng_clk = {
	.halt_reg = 0x29028,
	.clkr = {
		.enable_reg = 0x29028,
		.enable_mask = BIT(1),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie1_rchng_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie1_rchng_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_rcg2 pcie2_rchng_clk_src = {
	.cmd_rcgr = 0x2a028,
	.freq_tbl = ftbl_pcie_rchng_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie2_rchng_clk_src",
		.parent_data = gcc_xo_gpll0,
		.num_parents = 2,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie2_rchng_clk = {
	.halt_reg = 0x2a028,
	.clkr = {
		.enable_reg = 0x2a028,
		.enable_mask = BIT(1),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie2_rchng_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie2_rchng_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_rcg2 pcie3_rchng_clk_src = {
	.cmd_rcgr = 0x2b028,
	.freq_tbl = ftbl_pcie_rchng_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie3_rchng_clk_src",
		.parent_data = gcc_xo_gpll0,
		.num_parents = 2,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie3_rchng_clk = {
	.halt_reg = 0x2b028,
	.clkr = {
		.enable_reg = 0x2b028,
		.enable_mask = BIT(1),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie3_rchng_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie3_rchng_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_pcie_aux_clk_src[] = {
	F(20000000, P_GPLL0, 10, 1, 4),
	{ }
};

static struct clk_rcg2 pcie_aux_clk_src = {
	.cmd_rcgr = 0x28004,
	.freq_tbl = ftbl_pcie_aux_clk_src,
	.mnd_width = 16,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_aux_core_pi_sleep_clk_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcie_aux_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_aux_core_pi_sleep_clk,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_pcie0_aux_clk = {
	.halt_reg = 0x28034,
	.clkr = {
		.enable_reg = 0x28034,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie0_aux_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie_aux_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcie1_aux_clk = {
	.halt_reg = 0x29034,
	.clkr = {
		.enable_reg = 0x29034,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie1_aux_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie_aux_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcie2_aux_clk = {
	.halt_reg = 0x2a034,
	.clkr = {
		.enable_reg = 0x2a034,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie2_aux_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie_aux_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcie3_aux_clk = {
	.halt_reg = 0x2b034,
	.clkr = {
		.enable_reg = 0x2b034,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie3_aux_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcie_aux_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_usb_aux_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	{ }
};

static struct clk_rcg2 usb0_aux_clk_src = {
	.cmd_rcgr = 0x2c018,
	.freq_tbl = ftbl_usb_aux_clk_src,
	.mnd_width = 16,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_core_pi_sleep_clk_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "usb0_aux_clk_src",
		.parent_data = gcc_xo_gpll0_core_pi_sleep_clk,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_usb0_aux_clk = {
	.halt_reg = 0x2c048,
	.clkr = {
		.enable_reg = 0x2c048,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_usb0_aux_clk",
			.parent_hws = (const struct clk_hw *[]){
					&usb0_aux_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_usb0_master_clk_src[] = {
	F(100000000, P_GPLL0, 8, 0, 0),
	F(200000000, P_GPLL0, 4, 0, 0),
	{ }
};

static struct clk_rcg2 usb0_master_clk_src = {
	.cmd_rcgr = 0x2c004,
	.freq_tbl = ftbl_usb0_master_clk_src,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_out_main_div2_gpll0_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "usb0_master_clk_src",
		.parent_data = gcc_xo_gpll0_out_main_div2_gpll0,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_usb0_master_clk = {
	.halt_reg = 0x2c044,
	.clkr = {
		.enable_reg = 0x2c044,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_usb0_master_clk",
			.parent_hws = (const struct clk_hw *[]){
					&usb0_master_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_snoc_usb_clk = {
	.halt_reg = 0x2E058,
	.clkr = {
		.enable_reg = 0x2E058,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_snoc_usb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&usb0_master_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_anoc_usb_axi_clk = {
	.halt_reg = 0x2E084,
	.clkr = {
		.enable_reg = 0x2E084,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_anoc_usb_axi_clk",
			.parent_hws = (const struct clk_hw *[]){
					&usb0_master_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_usb0_mock_utmi_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	F(60000000, P_GPLL4, 10, 1, 2),
	{ }
};

static struct clk_rcg2 usb0_mock_utmi_clk_src = {
	.cmd_rcgr = 0x2c02c,
	.freq_tbl = ftbl_usb0_mock_utmi_clk_src,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll4_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "usb0_mock_utmi_clk_src",
		.parent_data = gcc_xo_gpll4_gpll0_gpll0_out_main_div2,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_regmap_div usb0_mock_utmi_div_clk_src = {
	.reg = 0x2c040,
	.shift = 0,
	.width = 2,
	.clkr.hw.init = &(struct clk_init_data) {
		.name = "usb0_mock_utmi_div_clk_src",
		.parent_data = &(const struct clk_parent_data){
			.hw = &usb0_mock_utmi_clk_src.clkr.hw,
		},
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_regmap_div_ro_ops,
	},
};


static struct clk_branch gcc_usb0_mock_utmi_clk = {
	.halt_reg = 0x2c04c,
	.clkr = {
		.enable_reg = 0x2c04c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_usb0_mock_utmi_clk",
			.parent_hws = (const struct clk_hw *[]){
					&usb0_mock_utmi_div_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_regmap_mux usb0_pipe_clk_src = {
	.reg = 0x2C074,
	.shift = 8,
	.width = 2,
	.parent_map = gcc_usb3phy_0_cc_pipe_clk_xo_map,
	.clkr = {
		.hw.init = &(struct clk_init_data){
			.name = "usb0_pipe_clk_src",
			.parent_data = gcc_usb3phy_0_cc_pipe_clk_xo,
			.num_parents = 2,
			.ops = &clk_regmap_mux_closest_ops,
			.flags = CLK_SET_RATE_PARENT,
		},
	},
};

static struct clk_branch gcc_usb0_pipe_clk = {
	.halt_reg = 0x2c054,
	.halt_check = BRANCH_HALT_DELAY,
	.clkr = {
		.enable_reg = 0x2c054,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_usb0_pipe_clk",
			.parent_hws = (const struct clk_hw *[]){
					&usb0_pipe_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb0_sleep_clk = {
	.halt_reg = 0x2c058,
	.clkr = {
		.enable_reg = 0x2c058,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_usb0_sleep_clk",
			.parent_hws = (const struct clk_hw *[]){
					&gcc_sleep_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_sdcc_apps_clk_src[] = {
	F(144000, P_XO, 16, 12, 125),
	F(400000, P_XO, 12, 1, 5),
	F(24000000, P_GPLL2, 12, 1, 4),
	F(48000000, P_GPLL2, 12, 1, 2),
	F(96000000, P_GPLL2, 12, 0, 0),
	F(177777778, P_GPLL0, 4.5, 0, 0),
	F(192000000, P_GPLL2, 6, 0, 0),
	F(384000000, P_GPLL2, 3, 0, 0),
	F(400000000, P_GPLL0, 2, 0, 0),
	{ }
};

static struct clk_rcg2 sdcc1_apps_clk_src = {
	.cmd_rcgr = 0x33004,
	.freq_tbl = ftbl_sdcc_apps_clk_src,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll2_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "sdcc1_apps_clk_src",
		.parent_data = gcc_xo_gpll0_gpll2_gpll0_out_main_div2,
		.num_parents = 4,
		.ops = &clk_rcg2_floor_ops,
	},
};

static struct clk_branch gcc_sdcc1_apps_clk = {
	.halt_reg = 0x3302c,
	.clkr = {
		.enable_reg = 0x3302c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_sdcc1_apps_clk",
			.parent_hws = (const struct clk_hw *[]){
					&sdcc1_apps_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_sdcc_ice_core_clk_src[] = {
	F(150000000, P_GPLL4, 8, 0, 0),
	F(300000000, P_GPLL4, 4, 0, 0),
};

static struct clk_rcg2 sdcc1_ice_core_clk_src = {
	.cmd_rcgr = 0x33018,
	.freq_tbl = ftbl_sdcc_ice_core_clk_src,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll4_gpll0_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "sdcc1_ice_core_clk_src",
		.parent_data = gcc_xo_gpll0_gpll4_gpll0_div2,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_sdcc1_ice_core_clk = {
	.halt_reg = 0x33030,
	.clkr = {
		.enable_reg = 0x33030,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_sdcc1_ice_core_clk",
			.parent_hws = (const struct clk_hw *[]){
					&sdcc1_ice_core_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_pcnoc_bfdcd_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	F(50000000, P_GPLL0, 16, 0, 0),
	F(80000000, P_GPLL0, 10, 0, 0),
	F(100000000, P_GPLL0, 8, 0, 0),
	{ }
};

static struct clk_rcg2 pcnoc_bfdcd_clk_src = {
	.cmd_rcgr = 0x31004,
	.freq_tbl = ftbl_pcnoc_bfdcd_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.flags = CLK_RCG2_HW_CONTROLLED,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "pcnoc_bfdcd_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_fixed_factor pcnoc_clk_src = {
	.mult = 1,
	.div = 1,
	.hw.init = &(struct clk_init_data){
		.name = "pcnoc_clk_src",
		.parent_hws = (const struct clk_hw *[]){
				&pcnoc_bfdcd_clk_src.clkr.hw },
		.num_parents = 1,
		.ops = &clk_fixed_factor_ops,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_branch gcc_nsscfg_clk = {
	.halt_reg = 0x1702c,
	.clkr = {
		.enable_reg = 0x1702c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nsscfg_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_nssnoc_nsscc_clk = {
	.halt_reg = 0x17030,
	.clkr = {
		.enable_reg = 0x17030,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nssnoc_nsscc_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_nsscc_clk = {
	.halt_reg = 0x17034,
	.clkr = {
		.enable_reg = 0x17034,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nsscc_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_nssnoc_pcnoc_1_clk = {
	.halt_reg = 0x17080,
	.clkr = {
		.enable_reg = 0x17080,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nssnoc_pcnoc_1_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_qdss_dap_ahb_clk = {
	.halt_reg = 0x2D064,
	.clkr = {
		.enable_reg = 0x2D064,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_dap_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_qdss_cfg_ahb_clk = {
	.halt_reg = 0x2D068,
	.clkr = {
		.enable_reg = 0x2D068,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_cfg_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_snoc_pcnoc_ahb_clk = {
	.halt_reg = 0x2E03C,
	.clkr = {
		.enable_reg = 0x2E03C,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_snoc_pcnoc_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_anoc_pcnoc_ahb_clk = {
	.halt_reg = 0x2E094,
	.clkr = {
		.enable_reg = 0x2E094,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_anoc_pcnoc_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_qpic_ahb_clk = {
	.halt_reg = 0x32010,
	.clkr = {
		.enable_reg = 0x32010,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qpic_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_qpic_clk = {
	.halt_reg = 0x32014,
	.clkr = {
		.enable_reg = 0x32014,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qpic_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_blsp1_ahb_clk = {
	.halt_reg = 0x01004,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x0b004,
		.enable_mask = BIT(4),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_blsp1_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_mdio_ahb_clk = {
	.halt_reg = 0x17040,
	.clkr = {
		.enable_reg = 0x17040,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_mdio_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_prng_ahb_clk = {
	.halt_reg = 0x13024,
	.halt_check = BRANCH_HALT_VOTED,
	.clkr = {
		.enable_reg = 0x0b004,
		.enable_mask = BIT(10),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_prng_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_uniphy0_ahb_clk = {
	.halt_reg = 0x1704c,
	.clkr = {
		.enable_reg = 0x1704c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_uniphy0_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_uniphy1_ahb_clk = {
	.halt_reg = 0x1705c,
	.clkr = {
		.enable_reg = 0x1705c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_uniphy1_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_uniphy2_ahb_clk = {
	.halt_reg = 0x1706c,
	.clkr = {
		.enable_reg = 0x1706c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_uniphy2_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_cmn_12gpll_ahb_clk = {
	.halt_reg = 0x3a004,
	.clkr = {
		.enable_reg = 0x3a004,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_cmn_12gpll_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_cmn_12gpll_apu_clk = {
	.halt_reg = 0x3a00c,
	.clkr = {
		.enable_reg = 0x3a00c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_cmn_12gpll_apu_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcie0_ahb_clk = {
	.halt_reg = 0x28030,
	.clkr = {
		.enable_reg = 0x28030,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie0_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcie1_ahb_clk = {
	.halt_reg = 0x29030,
	.clkr = {
		.enable_reg = 0x29030,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie1_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcie2_ahb_clk = {
	.halt_reg = 0x2a030,
	.clkr = {
		.enable_reg = 0x2a030,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie2_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcie3_ahb_clk = {
	.halt_reg = 0x2b030,
	.clkr = {
		.enable_reg = 0x2b030,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcie3_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb0_phy_cfg_ahb_clk = {
	.halt_reg = 0x2c05c,
	.clkr = {
		.enable_reg = 0x2c05c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_usb0_phy_cfg_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_sdcc1_ahb_clk = {
	.halt_reg = 0x33034,
	.clkr = {
		.enable_reg = 0x33034,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_sdcc1_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&pcnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_system_noc_bfdcd_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	F(133333333, P_GPLL0, 6, 0, 0),
	F(200000000, P_GPLL0, 4, 0, 0),
	F(342850000, P_GPLL4, 3.5, 0, 0),
	{ }
};

static struct clk_rcg2 system_noc_bfdcd_clk_src = {
	.cmd_rcgr = 0x2e004,
	.freq_tbl = ftbl_system_noc_bfdcd_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll4_map,
	.flags = CLK_RCG2_HW_CONTROLLED,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "system_noc_bfdcd_clk_src",
		.parent_data = gcc_xo_gpll0_gpll4,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
		.flags = CLK_IS_CRITICAL,
	},
};

static struct clk_branch gcc_q6ss_boot_clk = {
	.halt_reg = 0x25080,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x25080,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_q6ss_boot_clk",
			.parent_hws = (const struct clk_hw *[]){
					&system_noc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};


static struct clk_branch gcc_nssnoc_snoc_clk = {
	.halt_reg = 0x17028,
	.clkr = {
		.enable_reg = 0x17028,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nssnoc_snoc_clk",
			.parent_hws = (const struct clk_hw *[]){
					&system_noc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_nssnoc_snoc_1_clk = {
	.halt_reg = 0x1707c,
	.clkr = {
		.enable_reg = 0x1707c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nssnoc_snoc_1_clk",
			.parent_hws = (const struct clk_hw *[]){
					&system_noc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_qdss_etr_usb_clk = {
	.halt_reg = 0x2D060,
	.clkr = {
		.enable_reg = 0x2D060,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_etr_usb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&system_noc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_wcss_ahb_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	F(133333333, P_GPLL0, 6, 0, 0),
	{ }
};

static struct clk_rcg2 wcss_ahb_clk_src = {
	.cmd_rcgr = 0x25030,
	.freq_tbl = ftbl_wcss_ahb_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "wcss_ahb_clk_src",
		.parent_data = gcc_xo_gpll0,
		.num_parents = 2,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_q6_ahb_clk = {
	.halt_reg = 0x25014,
	.clkr = {
		.enable_reg = 0x25014,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_q6_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&wcss_ahb_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_q6_ahb_s_clk = {
	.halt_reg = 0x25018,
	.clkr = {
		.enable_reg = 0x25018,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_q6_ahb_s_clk",
			.parent_hws = (const struct clk_hw *[]){
					&wcss_ahb_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_wcss_ecahb_clk = {
	.halt_reg = 0x25058,
	.clkr = {
		.enable_reg = 0x25058,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_ecahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&wcss_ahb_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_wcss_acmt_clk = {
	.halt_reg = 0x2505c,
	.clkr = {
		.enable_reg = 0x2505c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_acmt_clk",
			.parent_hws = (const struct clk_hw *[]){
					&wcss_ahb_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_sys_noc_axi_clk = {
	.halt_reg = 0x2e01c,
	.clkr = {
		.enable_reg = 0x2e01c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_sys_noc_axi_clk",
			.parent_hws = (const struct clk_hw *[]){
					&system_noc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_anoc0_axi_clk = {
	.halt_reg = 0x2e078,
	.clkr = {
		.enable_reg = 0x2e078,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_anoc0_axi_clk",
			.parent_hws = (const struct clk_hw *[]){
					&system_noc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_anoc1_axi_clk = {
	.halt_reg = 0x2e088,
	.clkr = {
		.enable_reg = 0x2e088,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_anoc1_axi_clk",
			.parent_hws = (const struct clk_hw *[]){
					&system_noc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_anoc0_tbu_clk = {
	.halt_reg = 0x12038,
	.clkr = {
		.enable_reg = 0xb00c,
		.enable_mask = BIT(3),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_anoc0_tbu_clk",
			.parent_hws = (const struct clk_hw *[]){
					&system_noc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_anoc1_tbu_clk = {
	.halt_reg = 0x12048,
	.clkr = {
		.enable_reg = 0xb00c,
		.enable_mask = BIT(5),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_anoc1_tbu_clk",
			.parent_hws = (const struct clk_hw *[]){
					&system_noc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_mem_noc_snoc_axi_clk = {
	.halt_reg = 0x19018,
	.clkr = {
		.enable_reg = 0x19018,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_mem_noc_snoc_axi_clk",
			.parent_hws = (const struct clk_hw *[]){
					&system_noc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_imem_axi_clk = {
	.halt_reg = 0xe004,
	.clkr = {
		.enable_reg = 0xb004,
		.enable_mask = BIT(12),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_imem_axi_clk",
			.parent_hws = (const struct clk_hw *[]){
					&system_noc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};



static struct clk_branch gcc_wcss_ahb_s_clk = {
	.halt_reg = 0x25060,
	.clkr = {
		.enable_reg = 0x25060,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_ahb_s_clk",
			.parent_hws = (const struct clk_hw *[]){
					&wcss_ahb_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_sys_noc_wcss_ahb_clk = {
	.halt_reg = 0x2e030,
	.clkr = {
		.enable_reg = 0x2e030,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_sys_noc_wcss_ahb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&wcss_ahb_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_wcss_axi_m_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	F(133333333, P_GPLL0, 6, 0, 0),
	F(266666667, P_GPLL0, 3, 0, 0),
	{ }
};

static struct clk_rcg2 wcss_axi_m_clk_src = {
	.cmd_rcgr = 0x25078,
	.freq_tbl = ftbl_wcss_axi_m_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "wcss_axi_m_clk_src",
		.parent_data = gcc_xo_gpll0,
		.num_parents = 2,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_wcss_axi_m_clk = {
	.halt_reg = 0x25064,
	.clkr = {
		.enable_reg = 0x25064,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_axi_m_clk",
			.parent_hws = (const struct clk_hw *[]){
					&wcss_axi_m_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_anoc_wcss_axi_m_clk = {
	.halt_reg = 0x2e0a8,
	.clkr = {
		.enable_reg = 0x2e0a8,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_anoc_wcss_axi_m_clk",
			.parent_hws = (const struct clk_hw *[]){
					&wcss_axi_m_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_qdss_at_clk_src[] = {
	F(240000000, P_GPLL4, 5, 0, 0),
	{ }
};

static struct clk_rcg2 qdss_at_clk_src = {
	.cmd_rcgr = 0x2d004,
	.freq_tbl = ftbl_qdss_at_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll4_gpll0_gpll0_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "qdss_at_clk_src",
		.parent_data = gcc_xo_gpll4_gpll0_gpll0_div2,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_q6ss_atbm_clk = {
	.halt_reg = 0x2501c,
	.clkr = {
		.enable_reg = 0x2501c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_q6ss_atbm_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_at_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_wcss_dbg_ifc_atb_clk = {
	.halt_reg = 0x2503c,
	.clkr = {
		.enable_reg = 0x2503c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_dbg_ifc_atb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_at_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_wcss_dbg_ifc_atb_bdg_clk = {
	.halt_reg = 0x2504C,
	.clkr = {
		.enable_reg = 0x2504C,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_dbg_ifc_atb_bdg_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_at_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_nssnoc_atb_clk = {
	.halt_reg = 0x17014,
	.clkr = {
		.enable_reg = 0x17014,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nssnoc_atb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_at_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_qdss_at_clk = {
	.halt_reg = 0x2D038,
	.clkr = {
		.enable_reg = 0x2D038,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_at_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_at_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IS_CRITICAL,
			.ops = &clk_branch2_ops,
		},
	},
};
static struct clk_branch gcc_sys_noc_at_clk = {
	.halt_reg = 0x2e038,
	.clkr = {
		.enable_reg = 0x2e038,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_sys_noc_at_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_at_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IS_CRITICAL,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcnoc_at_clk = {
	.halt_reg = 0x31024,
	.clkr = {
		.enable_reg = 0x31024,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcnoc_at_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_at_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IS_CRITICAL,
			.ops = &clk_branch2_ops,
		},
	},
};


static struct clk_fixed_factor gcc_eud_at_div_clk_src = {
	.mult = 1,
	.div = 6,
	.hw.init = &(struct clk_init_data){
		.name = "gcc_eud_at_div_clk_src",
		.parent_hws = (const struct clk_hw *[]){
				&qdss_at_clk_src.clkr.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_fixed_factor_ops,
	},
};


static struct clk_branch gcc_usb0_eud_at_clk = {
	.halt_reg = 0x30004,
	.clkr = {
		.enable_reg = 0x30004,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_usb0_eud_at_clk",
			.parent_hws = (const struct clk_hw *[]){
					&gcc_eud_at_div_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IS_CRITICAL,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_qdss_eud_at_clk = {
	.halt_reg = 0x2D06C,
	.clkr = {
		.enable_reg = 0x2D06C,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_eud_at_clk",
			.parent_hws = (const struct clk_hw *[]){
					&gcc_eud_at_div_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IS_CRITICAL,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_qdss_stm_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	F(200000000, P_GPLL0, 4, 0, 0),
	{ }
};

static struct clk_rcg2 qdss_stm_clk_src = {
	.cmd_rcgr = 0x2D00C,
	.freq_tbl = ftbl_qdss_stm_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_out_main_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "qdss_stm_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_out_main_div2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_qdss_stm_clk = {
	.halt_reg = 0x2D03C,
	.clkr = {
		.enable_reg = 0x2D03C,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_stm_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_stm_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_sys_noc_qdss_stm_axi_clk = {
	.halt_reg = 0x2E034,
	.clkr = {
		.enable_reg = 0x2E034,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_sys_noc_qdss_stm_axi_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_stm_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_qdss_traceclkin_clk_src[] = {
	F(300000000, P_GPLL4, 4, 0, 0),
	{ }
};

static struct clk_rcg2 qdss_traceclkin_clk_src = {
	.cmd_rcgr = 0x2d014,
	.freq_tbl = ftbl_qdss_traceclkin_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll4_gpll0_gpll0_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "qdss_traceclkin_clk_src",
		.parent_data = gcc_xo_gpll4_gpll0_gpll0_div2,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_qdss_traceclkin_clk = {
	.halt_reg = 0x2D040,
	.clkr = {
		.enable_reg = 0x2D040,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_traceclkin_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_traceclkin_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_qdss_tsctr_clk_src[] = {
	F(600000000, P_GPLL4, 2, 0, 0),
	{ }
};

static struct clk_rcg2 qdss_tsctr_clk_src = {
	.cmd_rcgr = 0x2d01c,
	.freq_tbl = ftbl_qdss_tsctr_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll4_gpll0_gpll0_div2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "qdss_tsctr_clk_src",
		.parent_data = gcc_xo_gpll4_gpll0_gpll0_div2,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_fixed_factor qdss_tsctr_div2_clk_src = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "qdss_tsctr_div2_clk_src",
		.parent_hws = (const struct clk_hw *[]){
				&qdss_tsctr_clk_src.clkr.hw },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
		.ops = &clk_fixed_factor_ops,
	},
};

static struct clk_branch gcc_q6_tsctr_1to2_clk = {
	.halt_reg = 0x25020,
	.clkr = {
		.enable_reg = 0x25020,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_q6_tsctr_1to2_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_tsctr_div2_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_wcss_dbg_ifc_nts_clk = {
	.halt_reg = 0x25040,
	.clkr = {
		.enable_reg = 0x25040,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_dbg_ifc_nts_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_tsctr_div2_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_wcss_dbg_ifc_nts_bdg_clk = {
	.halt_reg = 0x25050,
	.clkr = {
		.enable_reg = 0x25050,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_dbg_ifc_nts_bdg_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_tsctr_div2_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_qdss_tsctr_div2_clk = {
	.halt_reg = 0x2d044,
	.clkr = {
		.enable_reg = 0x2d044,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_tsctr_div2_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_tsctr_div2_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct clk_parent_data gcc_xo[] = {
	{ .fw_name = "xo" },
};

static const struct parent_map gcc_xo_map[] = {
	{ P_XO, 0 },
};

static const struct freq_tbl ftbl_uniphy_sys_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	{ }
};

static struct clk_rcg2 uniphy_sys_clk_src = {
	.cmd_rcgr = 0x17090,
	.freq_tbl = ftbl_uniphy_sys_clk_src,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = gcc_xo_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "uniphy_sys_clk_src",
		.parent_data = gcc_xo,
		.num_parents = 1,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_rcg2 nss_ts_clk_src = {
	.cmd_rcgr = 0x17088,
	.freq_tbl = ftbl_uniphy_sys_clk_src,
	.mnd_width = 8,
	.hid_width = 5,
	.parent_map = gcc_xo_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "nss_ts_clk_src",
		.parent_data = gcc_xo,
		.num_parents = 1,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_qdss_ts_clk = {
	.halt_reg = 0x2D078,
	.clkr = {
		.enable_reg = 0x2D078,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_ts_clk",
			.parent_hws = (const struct clk_hw *[]){
					&nss_ts_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_snoc_ts_clk = {
	.halt_reg = 0x2e068,
	.clkr = {
		.enable_reg = 0x2e068,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_snoc_ts_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_tsctr_div2_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_mem_noc_ts_clk = {
	.halt_reg = 0x19028,
	.clkr = {
		.enable_reg = 0x19028,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_mem_noc_ts_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_tsctr_div2_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};


static struct clk_fixed_factor qdss_dap_sync_clk_src = {
	.mult = 1,
	.div = 4,
	.hw.init = &(struct clk_init_data){
		.name = "qdss_dap_sync_clk_src",
		.parent_hws = (const struct clk_hw *[]){
				&qdss_tsctr_clk_src.clkr.hw },
		.num_parents = 1,
		.ops = &clk_fixed_factor_ops,
	},
};


static struct clk_branch gcc_qdss_tsctr_div4_clk = {
	.halt_reg = 0x2d04c,
	.clkr = {
		.enable_reg = 0x2d04c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_tsctr_div4_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_dap_sync_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_fixed_factor qdss_tsctr_div8_clk_src = {
	.mult = 1,
	.div = 8,
	.hw.init = &(struct clk_init_data){
		.name = "qdss_tsctr_div8_clk_src",
		.parent_hws = (const struct clk_hw *[]){
				&qdss_tsctr_clk_src.clkr.hw },
		.num_parents = 1,
		.ops = &clk_fixed_factor_ops,
	},
};

static struct clk_branch gcc_nss_ts_clk = {
	.halt_reg = 0x17018,
	.clkr = {
		.enable_reg = 0x17018,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nss_ts_clk",
			.parent_hws = (const struct clk_hw *[]){
					&nss_ts_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_qdss_tsctr_div8_clk = {
	.halt_reg = 0x2d050,
	.clkr = {
		.enable_reg = 0x2d050,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_tsctr_div8_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_tsctr_div8_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcnoc_ts_clk = {
	.halt_reg = 0x3102c,
	.clkr = {
		.enable_reg = 0x3102c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcnoc_ts_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_tsctr_div8_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};


static struct clk_fixed_factor qdss_tsctr_div16_clk_src = {
	.mult = 1,
	.div = 16,
	.hw.init = &(struct clk_init_data){
		.name = "qdss_tsctr_div16_clk_src",
		.parent_hws = (const struct clk_hw *[]){
				&qdss_tsctr_clk_src.clkr.hw },
		.num_parents = 1,
		.ops = &clk_fixed_factor_ops,
	},
};

static struct clk_branch gcc_qdss_tsctr_div16_clk = {
	.halt_reg = 0x2d054,
	.clkr = {
		.enable_reg = 0x2d054,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_tsctr_div16_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_tsctr_div16_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_q6ss_pclkdbg_clk = {
	.halt_reg = 0x25024,
	.clkr = {
		.enable_reg = 0x25024,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_q6ss_pclkdbg_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_dap_sync_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_q6ss_trig_clk = {
	.halt_reg = 0x25068,
	.clkr = {
		.enable_reg = 0x25068,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_q6ss_trig_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_dap_sync_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_wcss_dbg_ifc_apb_clk = {
	.halt_reg = 0x25038,
	.clkr = {
		.enable_reg = 0x25038,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_dbg_ifc_apb_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_dap_sync_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_wcss_dbg_ifc_dapbus_clk = {
	.halt_reg = 0x25044,
	.clkr = {
		.enable_reg = 0x25044,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_dbg_ifc_dapbus_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_dap_sync_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_wcss_dbg_ifc_apb_bdg_clk = {
	.halt_reg = 0x25048,
	.clkr = {
		.enable_reg = 0x25048,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_dbg_ifc_apb_bdg_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_dap_sync_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_wcss_dbg_ifc_dapbus_bdg_clk = {
	.halt_reg = 0x25054,
	.clkr = {
		.enable_reg = 0x25054,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_dbg_ifc_dapbus_bdg_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_dap_sync_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_ubi32_dbg0_clk = {
	.halt_reg = 0x17010,
	.clkr = {
		.enable_reg = 0x17010,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_ubi32_dbg0_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_dap_sync_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_qdss_dap_clk = {
	.halt_reg = 0x2d058,
	.clkr = {
		.enable_reg = 0x2d058,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_dap_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_dap_sync_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IS_CRITICAL,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_qdss_apb2jtag_clk = {
	.halt_reg = 0x2d05c,
	.clkr = {
		.enable_reg = 0x2d05c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_apb2jtag_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_dap_sync_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcnoc_dcc_clk = {
	.halt_reg = 0x31080,
	.clkr = {
		.enable_reg = 0x31080,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcnoc_dcc_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_dap_sync_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_fixed_factor qdss_tsctr_div3_clk_src = {
	.mult = 1,
	.div = 3,
	.hw.init = &(struct clk_init_data){
		.name = "qdss_tsctr_div3_clk_src",
		.parent_hws = (const struct clk_hw *[]){
				&qdss_tsctr_clk_src.clkr.hw },
		.num_parents = 1,
		.ops = &clk_fixed_factor_ops,
	},
};

static struct clk_branch gcc_qdss_tsctr_div3_clk = {
	.halt_reg = 0x2d048,
	.clkr = {
		.enable_reg = 0x2d048,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qdss_tsctr_div3_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qdss_tsctr_div3_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_qpic_io_macro_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	F(100000000, P_GPLL0, 8, 0, 0),
	F(200000000, P_GPLL0, 4, 0, 0),
	F(320000000, P_GPLL0, 2.5, 0, 0),
	F(400000000, P_GPLL0, 2, 0, 0),
	{ }
};

static struct clk_rcg2 qpic_io_macro_clk_src = {
	.cmd_rcgr = 0x32004,
	.freq_tbl = ftbl_qpic_io_macro_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "qpic_io_macro_clk_src",
		.parent_data = gcc_xo_gpll0_gpll2,
		.num_parents = 3,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_qpic_io_macro_clk = {
	.halt_reg = 0x3200c,
	.clkr = {
		.enable_reg = 0x3200c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_qpic_io_macro_clk",
			.parent_hws = (const struct clk_hw *[]){
					&qpic_io_macro_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_q6_axi_clk_src[] = {
	F(533333333, P_GPLL0, 1.5, 0, 0),
	{ }
};

static struct clk_rcg2 q6_axi_clk_src = {
	.cmd_rcgr = 0x25004,
	.freq_tbl = ftbl_q6_axi_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll2_gpll4_pi_sleep_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "q6_axi_clk_src",
		.parent_data = gcc_xo_gpll0_gpll2_gpll4_pi_sleep,
		.num_parents = 5,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_q6_axim_clk = {
	.halt_reg = 0x2500c,
	.clkr = {
		.enable_reg = 0x2500c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_q6_axim_clk",
			.parent_hws = (const struct clk_hw *[]){
					&q6_axi_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_wcss_q6_tbu_clk = {
	.halt_reg = 0x12050,
	.halt_check = BRANCH_HALT_DELAY,
	.clkr = {
		.enable_reg = 0xb00c,
		.enable_mask = BIT(6),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_wcss_q6_tbu_clk",
			.parent_hws = (const struct clk_hw *[]){
					&q6_axi_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

struct clk_branch gcc_mem_noc_q6_axi_clk = {
	.halt_reg = 0x19010,
	.clkr = {
		.enable_reg = 0x19010,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_mem_noc_q6_axi_clk",
			.parent_hws = (const struct clk_hw *[]){
					&q6_axi_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_q6_axim2_clk_src[] = {
	F(342857143, P_GPLL4, 3.5, 0, 0),
	{ }
};

static const struct parent_map gcc_xo_gpll0_gpll4_bias_pll_ubinc_clk_map[] = {
	{ P_XO, 0 },
	{ P_GPLL0, 1 },
	{ P_GPLL4, 2 },
	{ P_BIAS_PLL_UBI_NC_CLK, 4 },
};

static struct clk_rcg2 q6_axim2_clk_src = {
	.cmd_rcgr = 0x25028,
	.freq_tbl = ftbl_q6_axim2_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll4_bias_pll_ubinc_clk_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "q6_axim2_clk_src",
		.parent_data = gcc_xo_gpll0_gpll4_bias_pll_ubi_nc_clk,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_q6_axim2_clk = {
	.halt_reg = 0x25010,
	.clkr = {
		.enable_reg = 0x25010,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_q6_axim2_clk",
			.parent_hws = (const struct clk_hw *[]){
					&q6_axim2_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_nssnoc_memnoc_bfdcd_clk_src[] = {
	F(533333333, P_GPLL0, 1.5, 0, 0),
	{ }
};

static struct clk_rcg2 nssnoc_memnoc_bfdcd_clk_src = {
	.cmd_rcgr = 0x17004,
	.freq_tbl = ftbl_nssnoc_memnoc_bfdcd_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_aux_gpll2_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "nssnoc_memnoc_bfdcd_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_aux_gpll2,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_nssnoc_memnoc_clk = {
	.halt_reg = 0x17024,
	.clkr = {
		.enable_reg = 0x17024,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nssnoc_memnoc_clk",
			.parent_hws = (const struct clk_hw *[]){
					&nssnoc_memnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_nssnoc_mem_noc_1_clk = {
	.halt_reg = 0x17084,
	.clkr = {
		.enable_reg = 0x17084,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nssnoc_mem_noc_1_clk",
			.parent_hws = (const struct clk_hw *[]){
					&nssnoc_memnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_nss_tbu_clk = {
	.halt_reg = 0x12040,
	.clkr = {
		.enable_reg = 0xb00c,
		.enable_mask = BIT(4),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nss_tbu_clk",
			.parent_hws = (const struct clk_hw *[]){
					&nssnoc_memnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_mem_noc_nssnoc_clk = {
	.halt_reg = 0x19014,
	.clkr = {
		.enable_reg = 0x19014,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_mem_noc_nssnoc_clk",
			.parent_hws = (const struct clk_hw *[]){
					&nssnoc_memnoc_bfdcd_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_lpass_axim_clk_src[] = {
	F(133333333, P_GPLL0, 6, 0, 0),
	{ }
};

static struct clk_rcg2 lpass_axim_clk_src = {
	.cmd_rcgr = 0x2700c,
	.freq_tbl = ftbl_lpass_axim_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "lpass_axim_clk_src",
		.parent_data = gcc_xo_gpll0,
		.num_parents = 2,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_lpass_core_axim_clk = {
	.halt_reg = 0x27018,
	.clkr = {
		.enable_reg = 0x27018,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_lpass_core_axim_clk",
			.parent_hws = (const struct clk_hw *[]){
					&lpass_axim_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_pcnoc_lpass_clk = {
	.halt_reg = 0x31020,
	.clkr = {
		.enable_reg = 0x31020,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_pcnoc_lpass_clk",
			.parent_hws = (const struct clk_hw *[]){
					&lpass_axim_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_rcg2 lpass_sway_clk_src = {
	.cmd_rcgr = 0x27004,
	.freq_tbl = ftbl_lpass_axim_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "lpass_sway_clk_src",
		.parent_data = gcc_xo_gpll0,
		.num_parents = 2,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_lpass_sway_clk = {
	.halt_reg = 0x27014,
	.clkr = {
		.enable_reg = 0x27014,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_lpass_sway_clk",
			.parent_hws = (const struct clk_hw *[]){
					&lpass_sway_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_snoc_lpass_cfg_clk = {
	.halt_reg = 0x2e028,
	.clkr = {
		.enable_reg = 0x2e028,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_snoc_lpass_cfg_clk",
			.parent_hws = (const struct clk_hw *[]){
					&lpass_sway_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_adss_pwm_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	F(100000000, P_GPLL0, 8, 0, 0),
	{ }
};

static struct clk_rcg2 adss_pwm_clk_src = {
	.cmd_rcgr = 0x1c004,
	.freq_tbl = ftbl_adss_pwm_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "adss_pwm_clk_src",
		.parent_data = gcc_xo_gpll0,
		.num_parents = 2,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_adss_pwm_clk = {
	.halt_reg = 0x1c00c,
	.clkr = {
		.enable_reg = 0x1c00c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_adss_pwm_clk",
			.parent_hws = (const struct clk_hw *[]){
					&adss_pwm_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static const struct freq_tbl ftbl_gp1_clk_src[] = {
	F(24000000, P_XO, 1, 0, 0),
	F(200000000, P_GPLL0, 4, 0, 0),
	{ }
};

static struct clk_rcg2 gp1_clk_src = {
	.cmd_rcgr = 0x8004,
	.freq_tbl = ftbl_gp1_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_sleep_clk_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "gp1_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_sleep_clk,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_gp1_clk = {
	.halt_reg = 0x8018,
	.clkr = {
		.enable_reg = 0x8018,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_gp1_clk",
			.parent_hws = (const struct clk_hw *[]){
					&gp1_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_rcg2 gp2_clk_src = {
	.cmd_rcgr = 0x9004,
	.freq_tbl = ftbl_gp1_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_sleep_clk_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "gp2_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_sleep_clk,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_gp2_clk = {
	.halt_reg = 0x9018,
	.clkr = {
		.enable_reg = 0x9018,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_gp2_clk",
			.parent_hws = (const struct clk_hw *[]){
					&gp2_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_rcg2 gp3_clk_src = {
	.cmd_rcgr = 0xa004,
	.freq_tbl = ftbl_gp1_clk_src,
	.hid_width = 5,
	.parent_map = gcc_xo_gpll0_gpll0_sleep_clk_map,
	.clkr.hw.init = &(struct clk_init_data){
		.name = "gp3_clk_src",
		.parent_data = gcc_xo_gpll0_gpll0_sleep_clk,
		.num_parents = 4,
		.ops = &clk_rcg2_ops,
	},
};

static struct clk_branch gcc_gp3_clk = {
	.halt_reg = 0xa018,
	.clkr = {
		.enable_reg = 0xa018,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_gp3_clk",
			.parent_hws = (const struct clk_hw *[]){
					&gp3_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_xo_clk_src = {
	.halt_reg = 0x34004,
	.clkr = {
		.enable_reg = 0x34004,
		.enable_mask = BIT(1),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_xo_clk_src",
			.parent_data = &(const struct clk_parent_data){
				.fw_name = "xo",
			},
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_nssnoc_xo_dcd_clk = {
	.halt_reg = 0x17074,
	.clkr = {
		.enable_reg = 0x17074,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nssnoc_xo_dcd_clk",
			.parent_hws = (const struct clk_hw *[]){
					&gcc_xo_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_xo_clk = {
	.halt_reg = 0x34018,
	.clkr = {
		.enable_reg = 0x34018,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_xo_clk",
			.parent_hws = (const struct clk_hw *[]){
					&gcc_xo_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IS_CRITICAL,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_spdm_ff_clk = {
	.halt_reg = 0x36018,
	.clkr = {
		.enable_reg = 0x36018,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_spdm_ff_clk",
			.parent_hws = (const struct clk_hw *[]){
					&gcc_xo_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_uniphy0_sys_clk = {
	.halt_reg = 0x17048,
	.clkr = {
		.enable_reg = 0x17048,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_uniphy0_sys_clk",
			.parent_hws = (const struct clk_hw *[]){
					&uniphy_sys_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_uniphy1_sys_clk = {
	.halt_reg = 0x17058,
	.clkr = {
		.enable_reg = 0x17058,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_uniphy1_sys_clk",
			.parent_hws = (const struct clk_hw *[]){
					&uniphy_sys_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_uniphy2_sys_clk = {
	.halt_reg = 0x17068,
	.clkr = {
		.enable_reg = 0x17068,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_uniphy2_sys_clk",
			.parent_hws = (const struct clk_hw *[]){
					&uniphy_sys_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_cmn_12gpll_sys_clk = {
	.halt_reg = 0x3a008,
	.clkr = {
		.enable_reg = 0x3a008,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_cmn_12gpll_sys_clk",
			.parent_hws = (const struct clk_hw *[]){
					&uniphy_sys_clk_src.clkr.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_fixed_factor gcc_xo_div4_clk_src = {
	.mult = 1,
	.div = 4,
	.hw.init = &(struct clk_init_data){
		.name = "gcc_xo_div4_clk_src",
		.parent_hws = (const struct clk_hw *[]){
				&gcc_xo_clk_src.clkr.hw },
		.num_parents = 1,
		.ops = &clk_fixed_factor_ops,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_branch gcc_nssnoc_qosgen_ref_clk = {
	.halt_reg = 0x1701c,
	.clkr = {
		.enable_reg = 0x1701c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nssnoc_qosgen_ref_clk",
			.parent_hws = (const struct clk_hw *[]){
					&gcc_xo_div4_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_nssnoc_timeout_ref_clk = {
	.halt_reg = 0x17020,
	.clkr = {
		.enable_reg = 0x17020,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_nssnoc_timeout_ref_clk",
			.parent_hws = (const struct clk_hw *[]){
					&gcc_xo_div4_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_xo_div4_clk = {
	.halt_reg = 0x3401c,
	.clkr = {
		.enable_reg = 0x3401c,
		.enable_mask = BIT(0),
		.hw.init = &(struct clk_init_data){
			.name = "gcc_xo_div4_clk",
			.parent_hws = (const struct clk_hw *[]){
					&gcc_xo_div4_clk_src.hw },
			.num_parents = 1,
			.flags = CLK_SET_RATE_PARENT | CLK_IS_CRITICAL,
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_hw *gcc_ipq9574_hws[] = {
	&gpll0_out_main_div2.hw,
	&gcc_xo_div4_clk_src.hw,
	&qdss_dap_sync_clk_src.hw,
	&qdss_tsctr_div2_clk_src.hw,
	&qdss_tsctr_div8_clk_src.hw,
	&qdss_tsctr_div16_clk_src.hw,
	&qdss_tsctr_div3_clk_src.hw,
	&gcc_eud_at_div_clk_src.hw,
	&pcnoc_clk_src.hw,
};

static int clk_dummy_is_enabled(struct clk_hw *hw)
{
	return 1;
};

static int clk_dummy_enable(struct clk_hw *hw)
{
	return 0;
};

static void clk_dummy_disable(struct clk_hw *hw)
{
	return;
};

static u8 clk_dummy_get_parent(struct clk_hw *hw)
{
	return 0;
};

static int clk_dummy_set_parent(struct clk_hw *hw, u8 index)
{
	return 0;
};

static int clk_dummy_set_rate(struct clk_hw *hw, unsigned long rate,
			      unsigned long parent_rate)
{
	return 0;
};

static int clk_dummy_determine_rate(struct clk_hw *hw,
				struct clk_rate_request *req)
{
	return 0;
};

static unsigned long clk_dummy_recalc_rate(struct clk_hw *hw,
					   unsigned long parent_rate)
{
	return parent_rate;
};

static const struct clk_ops clk_dummy_ops = {
	.is_enabled = clk_dummy_is_enabled,
	.enable = clk_dummy_enable,
	.disable = clk_dummy_disable,
	.get_parent = clk_dummy_get_parent,
	.set_parent = clk_dummy_set_parent,
	.set_rate = clk_dummy_set_rate,
	.recalc_rate = clk_dummy_recalc_rate,
	.determine_rate = clk_dummy_determine_rate,
};

#define DEFINE_DUMMY_CLK(clk_name)				\
(&(struct clk_regmap) {						\
	.hw.init = &(struct clk_init_data){			\
		.name = #clk_name,				\
		.parent_names = (const char *[]){ "xo"},	\
		.num_parents = 1,				\
		.ops = &clk_dummy_ops,				\
	},							\
})

static struct clk_regmap *gcc_ipq9574_clks[] = {
	[GPLL0_MAIN] = &gpll0_main.clkr,
	[GPLL0] = &gpll0.clkr,
	[GPLL4_MAIN] = &gpll4_main.clkr,
	[GPLL4] = &gpll4.clkr,
	[GPLL2_MAIN] = &gpll2_main.clkr,
	[GPLL2] = &gpll2.clkr,
	[GCC_SLEEP_CLK_SRC] = &gcc_sleep_clk_src.clkr,
	[APSS_AHB_CLK_SRC] = &apss_ahb_clk_src.clkr,
	[APSS_AXI_CLK_SRC] = &apss_axi_clk_src.clkr,
	[BLSP1_QUP1_I2C_APPS_CLK_SRC] = &blsp1_qup1_i2c_apps_clk_src.clkr,
	[BLSP1_QUP1_SPI_APPS_CLK_SRC] = &blsp1_qup1_spi_apps_clk_src.clkr,
	[BLSP1_QUP2_I2C_APPS_CLK_SRC] = &blsp1_qup2_i2c_apps_clk_src.clkr,
	[BLSP1_QUP2_SPI_APPS_CLK_SRC] = &blsp1_qup2_spi_apps_clk_src.clkr,
	[BLSP1_QUP3_I2C_APPS_CLK_SRC] = &blsp1_qup3_i2c_apps_clk_src.clkr,
	[BLSP1_QUP3_SPI_APPS_CLK_SRC] = &blsp1_qup3_spi_apps_clk_src.clkr,
	[BLSP1_QUP4_I2C_APPS_CLK_SRC] = &blsp1_qup4_i2c_apps_clk_src.clkr,
	[BLSP1_QUP4_SPI_APPS_CLK_SRC] = &blsp1_qup4_spi_apps_clk_src.clkr,
	[BLSP1_QUP5_I2C_APPS_CLK_SRC] = &blsp1_qup5_i2c_apps_clk_src.clkr,
	[BLSP1_QUP5_SPI_APPS_CLK_SRC] = &blsp1_qup5_spi_apps_clk_src.clkr,
	[BLSP1_QUP6_I2C_APPS_CLK_SRC] = &blsp1_qup6_i2c_apps_clk_src.clkr,
	[BLSP1_QUP6_SPI_APPS_CLK_SRC] = &blsp1_qup6_spi_apps_clk_src.clkr,
	[BLSP1_UART1_APPS_CLK_SRC] = &blsp1_uart1_apps_clk_src.clkr,
	[BLSP1_UART2_APPS_CLK_SRC] = &blsp1_uart2_apps_clk_src.clkr,
	[BLSP1_UART3_APPS_CLK_SRC] = &blsp1_uart3_apps_clk_src.clkr,
	[BLSP1_UART4_APPS_CLK_SRC] = &blsp1_uart4_apps_clk_src.clkr,
	[BLSP1_UART5_APPS_CLK_SRC] = &blsp1_uart5_apps_clk_src.clkr,
	[BLSP1_UART6_APPS_CLK_SRC] = &blsp1_uart6_apps_clk_src.clkr,
	[GCC_APSS_AHB_CLK] = &gcc_apss_ahb_clk.clkr,
	[GCC_APSS_AXI_CLK] = &gcc_apss_axi_clk.clkr,
	[GCC_BLSP1_QUP1_I2C_APPS_CLK] = &gcc_blsp1_qup1_i2c_apps_clk.clkr,
	[GCC_BLSP1_QUP1_SPI_APPS_CLK] = &gcc_blsp1_qup1_spi_apps_clk.clkr,
	[GCC_BLSP1_QUP2_I2C_APPS_CLK] = &gcc_blsp1_qup2_i2c_apps_clk.clkr,
	[GCC_BLSP1_QUP2_SPI_APPS_CLK] = &gcc_blsp1_qup2_spi_apps_clk.clkr,
	[GCC_BLSP1_QUP3_I2C_APPS_CLK] = &gcc_blsp1_qup3_i2c_apps_clk.clkr,
	[GCC_BLSP1_QUP3_SPI_APPS_CLK] = &gcc_blsp1_qup3_spi_apps_clk.clkr,
	[GCC_BLSP1_QUP4_I2C_APPS_CLK] = &gcc_blsp1_qup4_i2c_apps_clk.clkr,
	[GCC_BLSP1_QUP4_SPI_APPS_CLK] = &gcc_blsp1_qup4_spi_apps_clk.clkr,
	[GCC_BLSP1_QUP5_I2C_APPS_CLK] = &gcc_blsp1_qup5_i2c_apps_clk.clkr,
	[GCC_BLSP1_QUP5_SPI_APPS_CLK] = &gcc_blsp1_qup5_spi_apps_clk.clkr,
	[GCC_BLSP1_QUP6_I2C_APPS_CLK] = &gcc_blsp1_qup6_i2c_apps_clk.clkr,
	[GCC_BLSP1_QUP6_SPI_APPS_CLK] = &gcc_blsp1_qup6_spi_apps_clk.clkr,
	[GCC_BLSP1_UART1_APPS_CLK] = &gcc_blsp1_uart1_apps_clk.clkr,
	[GCC_BLSP1_UART2_APPS_CLK] = &gcc_blsp1_uart2_apps_clk.clkr,
	[GCC_BLSP1_UART3_APPS_CLK] = &gcc_blsp1_uart3_apps_clk.clkr,
	[GCC_BLSP1_UART4_APPS_CLK] = &gcc_blsp1_uart4_apps_clk.clkr,
	[GCC_BLSP1_UART5_APPS_CLK] = &gcc_blsp1_uart5_apps_clk.clkr,
	[GCC_BLSP1_UART6_APPS_CLK] = &gcc_blsp1_uart6_apps_clk.clkr,
	[PCIE0_AXI_M_CLK_SRC] = &pcie0_axi_m_clk_src.clkr,
	[GCC_PCIE0_AXI_M_CLK] = &gcc_pcie0_axi_m_clk.clkr,
	[PCIE1_AXI_M_CLK_SRC] = &pcie1_axi_m_clk_src.clkr,
	[GCC_PCIE1_AXI_M_CLK] = &gcc_pcie1_axi_m_clk.clkr,
	[PCIE2_AXI_M_CLK_SRC] = &pcie2_axi_m_clk_src.clkr,
	[GCC_PCIE2_AXI_M_CLK] = &gcc_pcie2_axi_m_clk.clkr,
	[PCIE3_AXI_M_CLK_SRC] = &pcie3_axi_m_clk_src.clkr,
	[GCC_PCIE3_AXI_M_CLK] = &gcc_pcie3_axi_m_clk.clkr,
	[PCIE0_AXI_S_CLK_SRC] = &pcie0_axi_s_clk_src.clkr,
	[GCC_PCIE0_AXI_S_BRIDGE_CLK] = &gcc_pcie0_axi_s_bridge_clk.clkr,
	[GCC_PCIE0_AXI_S_CLK] = &gcc_pcie0_axi_s_clk.clkr,
	[PCIE1_AXI_S_CLK_SRC] = &pcie1_axi_s_clk_src.clkr,
	[GCC_PCIE1_AXI_S_BRIDGE_CLK] = &gcc_pcie1_axi_s_bridge_clk.clkr,
	[GCC_PCIE1_AXI_S_CLK] = &gcc_pcie1_axi_s_clk.clkr,
	[PCIE2_AXI_S_CLK_SRC] = &pcie2_axi_s_clk_src.clkr,
	[GCC_PCIE2_AXI_S_BRIDGE_CLK] = &gcc_pcie2_axi_s_bridge_clk.clkr,
	[GCC_PCIE2_AXI_S_CLK] = &gcc_pcie2_axi_s_clk.clkr,
	[PCIE3_AXI_S_CLK_SRC] = &pcie3_axi_s_clk_src.clkr,
	[GCC_PCIE3_AXI_S_BRIDGE_CLK] = &gcc_pcie3_axi_s_bridge_clk.clkr,
	[GCC_PCIE3_AXI_S_CLK] = &gcc_pcie3_axi_s_clk.clkr,
	[PCIE0_PIPE_CLK_SRC] = &pcie0_pipe_clk_src.clkr,
	[GCC_PCIE0_PIPE_CLK] = &gcc_pcie0_pipe_clk.clkr,
	[PCIE1_PIPE_CLK_SRC] = &pcie1_pipe_clk_src.clkr,
	[GCC_PCIE1_PIPE_CLK] = &gcc_pcie1_pipe_clk.clkr,
	[PCIE2_PIPE_CLK_SRC] = &pcie2_pipe_clk_src.clkr,
	[GCC_PCIE2_PIPE_CLK] = &gcc_pcie2_pipe_clk.clkr,
	[PCIE3_PIPE_CLK_SRC] = &pcie3_pipe_clk_src.clkr,
	[GCC_PCIE3_PIPE_CLK] = &gcc_pcie3_pipe_clk.clkr,
	[PCIE_AUX_CLK_SRC] = &pcie_aux_clk_src.clkr,
	[GCC_PCIE0_AUX_CLK] = &gcc_pcie0_aux_clk.clkr,
	[GCC_PCIE1_AUX_CLK] = &gcc_pcie1_aux_clk.clkr,
	[GCC_PCIE2_AUX_CLK] = &gcc_pcie2_aux_clk.clkr,
	[GCC_PCIE3_AUX_CLK] = &gcc_pcie3_aux_clk.clkr,
	[PCIE0_RCHNG_CLK_SRC] = &pcie0_rchng_clk_src.clkr,
	[GCC_PCIE0_RCHNG_CLK] = &gcc_pcie0_rchng_clk.clkr,
	[PCIE1_RCHNG_CLK_SRC] = &pcie1_rchng_clk_src.clkr,
	[GCC_PCIE1_RCHNG_CLK] = &gcc_pcie1_rchng_clk.clkr,
	[PCIE2_RCHNG_CLK_SRC] = &pcie2_rchng_clk_src.clkr,
	[GCC_PCIE2_RCHNG_CLK] = &gcc_pcie2_rchng_clk.clkr,
	[PCIE3_RCHNG_CLK_SRC] = &pcie3_rchng_clk_src.clkr,
	[GCC_PCIE3_RCHNG_CLK] = &gcc_pcie3_rchng_clk.clkr,
	[GCC_PCIE0_AHB_CLK] = &gcc_pcie0_ahb_clk.clkr,
	[GCC_PCIE1_AHB_CLK] = &gcc_pcie1_ahb_clk.clkr,
	[GCC_PCIE2_AHB_CLK] = &gcc_pcie2_ahb_clk.clkr,
	[GCC_PCIE3_AHB_CLK] = &gcc_pcie3_ahb_clk.clkr,
	[USB0_AUX_CLK_SRC] = &usb0_aux_clk_src.clkr,
	[GCC_USB0_AUX_CLK] = &gcc_usb0_aux_clk.clkr,
	[USB0_MASTER_CLK_SRC] = &usb0_master_clk_src.clkr,
	[GCC_USB0_MASTER_CLK] = &gcc_usb0_master_clk.clkr,
	[GCC_SNOC_USB_CLK] = &gcc_snoc_usb_clk.clkr,
	[GCC_ANOC_USB_AXI_CLK] = &gcc_anoc_usb_axi_clk.clkr,
	[USB0_MOCK_UTMI_CLK_SRC] = &usb0_mock_utmi_clk_src.clkr,
	[USB0_MOCK_UTMI_DIV_CLK_SRC] = &usb0_mock_utmi_div_clk_src.clkr,
	[GCC_USB0_MOCK_UTMI_CLK] = &gcc_usb0_mock_utmi_clk.clkr,
	[USB0_PIPE_CLK_SRC] = &usb0_pipe_clk_src.clkr,
	[GCC_USB0_PIPE_CLK] = &gcc_usb0_pipe_clk.clkr,
	[GCC_USB0_PHY_CFG_AHB_CLK] = &gcc_usb0_phy_cfg_ahb_clk.clkr,
	[GCC_USB0_SLEEP_CLK] = &gcc_usb0_sleep_clk.clkr,
	[SDCC1_APPS_CLK_SRC] = &sdcc1_apps_clk_src.clkr,
	[GCC_SDCC1_APPS_CLK] = &gcc_sdcc1_apps_clk.clkr,
	[SDCC1_ICE_CORE_CLK_SRC] = &sdcc1_ice_core_clk_src.clkr,
	[GCC_SDCC1_ICE_CORE_CLK] = &gcc_sdcc1_ice_core_clk.clkr,
	[GCC_SDCC1_AHB_CLK] = &gcc_sdcc1_ahb_clk.clkr,
	[PCNOC_BFDCD_CLK_SRC] = &pcnoc_bfdcd_clk_src.clkr,
	[GCC_NSSCFG_CLK] = &gcc_nsscfg_clk.clkr,
	[GCC_NSSNOC_NSSCC_CLK] = &gcc_nssnoc_nsscc_clk.clkr,
	[GCC_NSSCC_CLK] = &gcc_nsscc_clk.clkr,
	[GCC_NSSNOC_PCNOC_1_CLK] = &gcc_nssnoc_pcnoc_1_clk.clkr,
	[GCC_QDSS_DAP_AHB_CLK] = &gcc_qdss_dap_ahb_clk.clkr,
	[GCC_QDSS_CFG_AHB_CLK] = &gcc_qdss_cfg_ahb_clk.clkr,
	[GCC_SNOC_PCNOC_AHB_CLK] = &gcc_snoc_pcnoc_ahb_clk.clkr,
	[GCC_ANOC_PCNOC_AHB_CLK] = &gcc_anoc_pcnoc_ahb_clk.clkr,
	[GCC_QPIC_AHB_CLK] = &gcc_qpic_ahb_clk.clkr,
	[GCC_QPIC_CLK] = &gcc_qpic_clk.clkr,
	[GCC_BLSP1_AHB_CLK] = &gcc_blsp1_ahb_clk.clkr,
	[GCC_MDIO_AHB_CLK] = &gcc_mdio_ahb_clk.clkr,
	[GCC_PRNG_AHB_CLK] = &gcc_prng_ahb_clk.clkr,
	[GCC_UNIPHY0_AHB_CLK] = &gcc_uniphy0_ahb_clk.clkr,
	[GCC_UNIPHY1_AHB_CLK] = &gcc_uniphy1_ahb_clk.clkr,
	[GCC_UNIPHY2_AHB_CLK] = &gcc_uniphy2_ahb_clk.clkr,
	[GCC_CMN_12GPLL_AHB_CLK] = &gcc_cmn_12gpll_ahb_clk.clkr,
	[GCC_CMN_12GPLL_APU_CLK] = &gcc_cmn_12gpll_apu_clk.clkr,
	[SYSTEM_NOC_BFDCD_CLK_SRC] = &system_noc_bfdcd_clk_src.clkr,
	[GCC_NSSNOC_SNOC_CLK] = &gcc_nssnoc_snoc_clk.clkr,
	[GCC_NSSNOC_SNOC_1_CLK] = &gcc_nssnoc_snoc_1_clk.clkr,
	[GCC_QDSS_ETR_USB_CLK] = &gcc_qdss_etr_usb_clk.clkr,
	[GCC_SYS_NOC_AXI_CLK] = &gcc_sys_noc_axi_clk.clkr,
	[GCC_ANOC0_AXI_CLK] = &gcc_anoc0_axi_clk.clkr,
	[GCC_ANOC1_AXI_CLK] = &gcc_anoc1_axi_clk.clkr,
	[GCC_ANOC0_TBU_CLK] = &gcc_anoc0_tbu_clk.clkr,
	[GCC_ANOC1_TBU_CLK] = &gcc_anoc1_tbu_clk.clkr,
	[GCC_MEM_NOC_SNOC_AXI_CLK] = &gcc_mem_noc_snoc_axi_clk.clkr,
	[GCC_IMEM_AXI_CLK] = &gcc_imem_axi_clk.clkr,
	[WCSS_AHB_CLK_SRC] = &wcss_ahb_clk_src.clkr,
	[GCC_Q6_AHB_CLK] = &gcc_q6_ahb_clk.clkr,
	[GCC_Q6_AHB_S_CLK] = &gcc_q6_ahb_s_clk.clkr,
	[GCC_WCSS_ECAHB_CLK] = &gcc_wcss_ecahb_clk.clkr,
	[GCC_WCSS_ACMT_CLK] = &gcc_wcss_acmt_clk.clkr,
	[GCC_WCSS_AHB_S_CLK] = &gcc_wcss_ahb_s_clk.clkr,
	[GCC_SYS_NOC_WCSS_AHB_CLK] = &gcc_sys_noc_wcss_ahb_clk.clkr,
	[WCSS_AXI_M_CLK_SRC] = &wcss_axi_m_clk_src.clkr,
	[GCC_WCSS_AXI_M_CLK] = &gcc_wcss_axi_m_clk.clkr,
	[GCC_ANOC_WCSS_AXI_M_CLK] = &gcc_anoc_wcss_axi_m_clk.clkr,
	[QDSS_AT_CLK_SRC] = &qdss_at_clk_src.clkr,
	[GCC_Q6SS_ATBM_CLK] = &gcc_q6ss_atbm_clk.clkr,
	[GCC_WCSS_DBG_IFC_ATB_CLK] = &gcc_wcss_dbg_ifc_atb_clk.clkr,
	[GCC_WCSS_DBG_IFC_ATB_BDG_CLK] = &gcc_wcss_dbg_ifc_atb_bdg_clk.clkr,
	[GCC_NSSNOC_ATB_CLK] = &gcc_nssnoc_atb_clk.clkr,
	[GCC_QDSS_AT_CLK] = &gcc_qdss_at_clk.clkr,
	[GCC_SYS_NOC_AT_CLK] = &gcc_sys_noc_at_clk.clkr,
	[GCC_PCNOC_AT_CLK] = &gcc_pcnoc_at_clk.clkr,
	[GCC_USB0_EUD_AT_CLK] = &gcc_usb0_eud_at_clk.clkr,
	[GCC_QDSS_EUD_AT_CLK] = &gcc_qdss_eud_at_clk.clkr,
	[QDSS_STM_CLK_SRC] = &qdss_stm_clk_src.clkr,
	[GCC_QDSS_STM_CLK] = &gcc_qdss_stm_clk.clkr,
	[GCC_SYS_NOC_QDSS_STM_AXI_CLK] = &gcc_sys_noc_qdss_stm_axi_clk.clkr,
	[QDSS_TRACECLKIN_CLK_SRC] = &qdss_traceclkin_clk_src.clkr,
	[GCC_QDSS_TRACECLKIN_CLK] = &gcc_qdss_traceclkin_clk.clkr,
	[QDSS_TSCTR_CLK_SRC] = &qdss_tsctr_clk_src.clkr,
	[GCC_Q6_TSCTR_1TO2_CLK] = &gcc_q6_tsctr_1to2_clk.clkr,
	[GCC_WCSS_DBG_IFC_NTS_CLK] = &gcc_wcss_dbg_ifc_nts_clk.clkr,
	[GCC_WCSS_DBG_IFC_NTS_BDG_CLK] = &gcc_wcss_dbg_ifc_nts_bdg_clk.clkr,
	[GCC_QDSS_TSCTR_DIV2_CLK] = &gcc_qdss_tsctr_div2_clk.clkr,
	[GCC_QDSS_TS_CLK] = &gcc_qdss_ts_clk.clkr,
	[GCC_SNOC_TS_CLK] = &gcc_snoc_ts_clk.clkr,
	[GCC_MEM_NOC_TS_CLK] = &gcc_mem_noc_ts_clk.clkr,
	[GCC_QDSS_TSCTR_DIV4_CLK] = &gcc_qdss_tsctr_div4_clk.clkr,
	[GCC_NSS_TS_CLK] = &gcc_nss_ts_clk.clkr,
	[GCC_QDSS_TSCTR_DIV8_CLK] = &gcc_qdss_tsctr_div8_clk.clkr,
	[GCC_PCNOC_TS_CLK] = &gcc_pcnoc_ts_clk.clkr,
	[GCC_QDSS_TSCTR_DIV16_CLK] = &gcc_qdss_tsctr_div16_clk.clkr,
	[GCC_Q6SS_PCLKDBG_CLK] = &gcc_q6ss_pclkdbg_clk.clkr,
	[GCC_Q6SS_TRIG_CLK] = &gcc_q6ss_trig_clk.clkr,
	[GCC_WCSS_DBG_IFC_APB_CLK] = &gcc_wcss_dbg_ifc_apb_clk.clkr,
	[GCC_WCSS_DBG_IFC_DAPBUS_CLK] = &gcc_wcss_dbg_ifc_dapbus_clk.clkr,
	[GCC_WCSS_DBG_IFC_APB_BDG_CLK] = &gcc_wcss_dbg_ifc_apb_bdg_clk.clkr,
	[GCC_WCSS_DBG_IFC_DAPBUS_BDG_CLK] = &gcc_wcss_dbg_ifc_dapbus_bdg_clk.clkr,
	[GCC_UBI32_DBG0_CLK] = &gcc_ubi32_dbg0_clk.clkr,
	[GCC_QDSS_DAP_CLK] = &gcc_qdss_dap_clk.clkr,
	[GCC_QDSS_APB2JTAG_CLK] = &gcc_qdss_apb2jtag_clk.clkr,
	[GCC_QDSS_TSCTR_DIV3_CLK] = &gcc_qdss_tsctr_div3_clk.clkr,
	[QPIC_IO_MACRO_CLK_SRC] = &qpic_io_macro_clk_src.clkr,
	[GCC_QPIC_IO_MACRO_CLK] = &gcc_qpic_io_macro_clk.clkr,
	[Q6_AXI_CLK_SRC] = &q6_axi_clk_src.clkr,
	[GCC_Q6_AXIM_CLK] = &gcc_q6_axim_clk.clkr,
	[GCC_WCSS_Q6_TBU_CLK] = &gcc_wcss_q6_tbu_clk.clkr,
	[GCC_MEM_NOC_Q6_AXI_CLK] = &gcc_mem_noc_q6_axi_clk.clkr,
	[Q6_AXIM2_CLK_SRC] = &q6_axim2_clk_src.clkr,
	[GCC_Q6_AXIM2_CLK] = &gcc_q6_axim2_clk.clkr,
	[NSSNOC_MEMNOC_BFDCD_CLK_SRC] = &nssnoc_memnoc_bfdcd_clk_src.clkr,
	[GCC_NSSNOC_MEMNOC_CLK] = &gcc_nssnoc_memnoc_clk.clkr,
	[GCC_NSSNOC_MEM_NOC_1_CLK] = &gcc_nssnoc_mem_noc_1_clk.clkr,
	[GCC_NSS_TBU_CLK] = &gcc_nss_tbu_clk.clkr,
	[GCC_MEM_NOC_NSSNOC_CLK] = &gcc_mem_noc_nssnoc_clk.clkr,
	[LPASS_AXIM_CLK_SRC] = &lpass_axim_clk_src.clkr,
	[GCC_LPASS_CORE_AXIM_CLK] = &gcc_lpass_core_axim_clk.clkr,
	[GCC_PCNOC_LPASS_CLK] = &gcc_pcnoc_lpass_clk.clkr,
	[LPASS_SWAY_CLK_SRC] = &lpass_sway_clk_src.clkr,
	[GCC_LPASS_SWAY_CLK] = &gcc_lpass_sway_clk.clkr,
	[GCC_SNOC_LPASS_CFG_CLK] = &gcc_snoc_lpass_cfg_clk.clkr,
	[ADSS_PWM_CLK_SRC] = &adss_pwm_clk_src.clkr,
	[GCC_ADSS_PWM_CLK] = &gcc_adss_pwm_clk.clkr,
	[GP1_CLK_SRC] = &gp1_clk_src.clkr,
	[GP2_CLK_SRC] = &gp2_clk_src.clkr,
	[GP3_CLK_SRC] = &gp3_clk_src.clkr,
	[GCC_GP1_CLK] = &gcc_gp1_clk.clkr,
	[GCC_GP2_CLK] = &gcc_gp2_clk.clkr,
	[GCC_GP3_CLK] = &gcc_gp3_clk.clkr,
	[GCC_XO_CLK_SRC] = &gcc_xo_clk_src.clkr,
	[GCC_NSSNOC_XO_DCD_CLK] = &gcc_nssnoc_xo_dcd_clk.clkr,
	[GCC_XO_CLK] = &gcc_xo_clk.clkr,
	[GCC_NSSNOC_QOSGEN_REF_CLK] = &gcc_nssnoc_qosgen_ref_clk.clkr,
	[GCC_NSSNOC_TIMEOUT_REF_CLK] = &gcc_nssnoc_timeout_ref_clk.clkr,
	[GCC_XO_DIV4_CLK] = &gcc_xo_div4_clk.clkr,
	[GCC_SPDM_FF_CLK] = &gcc_spdm_ff_clk.clkr,
	[GCC_UNIPHY0_SYS_CLK] = &gcc_uniphy0_sys_clk.clkr,
	[GCC_UNIPHY1_SYS_CLK] = &gcc_uniphy1_sys_clk.clkr,
	[GCC_UNIPHY2_SYS_CLK] = &gcc_uniphy2_sys_clk.clkr,
	[GCC_CMN_12GPLL_SYS_CLK] = &gcc_cmn_12gpll_sys_clk.clkr,
	[GCC_Q6SS_BOOT_CLK] = &gcc_q6ss_boot_clk.clkr,
	[UNIPHY_SYS_CLK_SRC] = &uniphy_sys_clk_src.clkr,
	[NSS_TS_CLK_SRC] = &nss_ts_clk_src.clkr,
	[GCC_ANOC_PCIE0_1LANE_M_CLK] = &gcc_anoc_pcie0_1lane_m_clk.clkr,
	[GCC_ANOC_PCIE1_1LANE_M_CLK] = &gcc_anoc_pcie1_1lane_m_clk.clkr,
	[GCC_ANOC_PCIE2_2LANE_M_CLK] = &gcc_anoc_pcie2_2lane_m_clk.clkr,
	[GCC_ANOC_PCIE3_2LANE_M_CLK] = &gcc_anoc_pcie3_2lane_m_clk.clkr,
	[GCC_SNOC_PCIE0_1LANE_S_CLK] = &gcc_snoc_pcie0_1lane_s_clk.clkr,
	[GCC_SNOC_PCIE1_1LANE_S_CLK] = &gcc_snoc_pcie1_1lane_s_clk.clkr,
	[GCC_SNOC_PCIE2_2LANE_S_CLK] = &gcc_snoc_pcie2_2lane_s_clk.clkr,
	[GCC_SNOC_PCIE3_2LANE_S_CLK] = &gcc_snoc_pcie3_2lane_s_clk.clkr,
	[GCC_PCNOC_DCC_CLK] = &gcc_pcnoc_dcc_clk.clkr,
};

static struct clk_regmap *gcc_ipq9574_dummy_clks[] = {
	[GPLL0_MAIN] = DEFINE_DUMMY_CLK(gpll0_main),
	[GPLL0] = DEFINE_DUMMY_CLK(gpll0),
	[GPLL4_MAIN] = DEFINE_DUMMY_CLK(gpll4_main),
	[GPLL4] = DEFINE_DUMMY_CLK(gpll4),
	[GPLL2_MAIN] = DEFINE_DUMMY_CLK(gpll2_main),
	[GPLL2] = DEFINE_DUMMY_CLK(gpll2),
	[GCC_SLEEP_CLK_SRC] = DEFINE_DUMMY_CLK(gcc_sleep_clk_src),
	[APSS_AHB_CLK_SRC] = DEFINE_DUMMY_CLK(apss_ahb_clk_src),
	[APSS_AXI_CLK_SRC] = DEFINE_DUMMY_CLK(apss_axi_clk_src),
	[BLSP1_QUP1_I2C_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_qup1_i2c_apps_clk_src),
	[BLSP1_QUP1_SPI_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_qup1_spi_apps_clk_src),
	[BLSP1_QUP2_I2C_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_qup2_i2c_apps_clk_src),
	[BLSP1_QUP2_SPI_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_qup2_spi_apps_clk_src),
	[BLSP1_QUP3_I2C_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_qup3_i2c_apps_clk_src),
	[BLSP1_QUP3_SPI_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_qup3_spi_apps_clk_src),
	[BLSP1_QUP4_I2C_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_qup4_i2c_apps_clk_src),
	[BLSP1_QUP4_SPI_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_qup4_spi_apps_clk_src),
	[BLSP1_QUP5_I2C_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_qup5_i2c_apps_clk_src),
	[BLSP1_QUP5_SPI_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_qup5_spi_apps_clk_src),
	[BLSP1_QUP6_I2C_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_qup6_i2c_apps_clk_src),
	[BLSP1_QUP6_SPI_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_qup6_spi_apps_clk_src),
	[BLSP1_UART1_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_uart1_apps_clk_src),
	[BLSP1_UART2_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_uart2_apps_clk_src),
	[BLSP1_UART3_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_uart3_apps_clk_src),
	[BLSP1_UART4_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_uart4_apps_clk_src),
	[BLSP1_UART5_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_uart5_apps_clk_src),
	[BLSP1_UART6_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(blsp1_uart6_apps_clk_src),
	[GCC_APSS_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_apss_ahb_clk),
	[GCC_APSS_AXI_CLK] = DEFINE_DUMMY_CLK(gcc_apss_axi_clk),
	[GCC_BLSP1_QUP1_I2C_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_qup1_i2c_apps_clk),
	[GCC_BLSP1_QUP1_SPI_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_qup1_spi_apps_clk),
	[GCC_BLSP1_QUP2_I2C_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_qup2_i2c_apps_clk),
	[GCC_BLSP1_QUP2_SPI_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_qup2_spi_apps_clk),
	[GCC_BLSP1_QUP3_I2C_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_qup3_i2c_apps_clk),
	[GCC_BLSP1_QUP3_SPI_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_qup3_spi_apps_clk),
	[GCC_BLSP1_QUP4_I2C_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_qup4_i2c_apps_clk),
	[GCC_BLSP1_QUP4_SPI_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_qup4_spi_apps_clk),
	[GCC_BLSP1_QUP5_I2C_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_qup5_i2c_apps_clk),
	[GCC_BLSP1_QUP5_SPI_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_qup5_spi_apps_clk),
	[GCC_BLSP1_QUP6_I2C_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_qup6_i2c_apps_clk),
	[GCC_BLSP1_QUP6_SPI_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_qup6_spi_apps_clk),
	[GCC_BLSP1_UART1_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_uart1_apps_clk),
	[GCC_BLSP1_UART2_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_uart2_apps_clk),
	[GCC_BLSP1_UART3_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_uart3_apps_clk),
	[GCC_BLSP1_UART4_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_uart4_apps_clk),
	[GCC_BLSP1_UART5_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_uart5_apps_clk),
	[GCC_BLSP1_UART6_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_uart6_apps_clk),
	[PCIE0_AXI_M_CLK_SRC] = DEFINE_DUMMY_CLK(pcie0_axi_m_clk_src),
	[GCC_PCIE0_AXI_M_CLK] = DEFINE_DUMMY_CLK(gcc_pcie0_axi_m_clk),
	[PCIE1_AXI_M_CLK_SRC] = DEFINE_DUMMY_CLK(pcie1_axi_m_clk_src),
	[GCC_PCIE1_AXI_M_CLK] = DEFINE_DUMMY_CLK(gcc_pcie1_axi_m_clk),
	[PCIE2_AXI_M_CLK_SRC] = DEFINE_DUMMY_CLK(pcie2_axi_m_clk_src),
	[GCC_PCIE2_AXI_M_CLK] = DEFINE_DUMMY_CLK(gcc_pcie2_axi_m_clk),
	[PCIE3_AXI_M_CLK_SRC] = DEFINE_DUMMY_CLK(pcie3_axi_m_clk_src),
	[GCC_PCIE3_AXI_M_CLK] = DEFINE_DUMMY_CLK(gcc_pcie3_axi_m_clk),
	[PCIE0_AXI_S_CLK_SRC] = DEFINE_DUMMY_CLK(pcie0_axi_s_clk_src),
	[GCC_PCIE0_AXI_S_BRIDGE_CLK] = DEFINE_DUMMY_CLK(gcc_pcie0_axi_s_bridge_clk),
	[GCC_PCIE0_AXI_S_CLK] = DEFINE_DUMMY_CLK(gcc_pcie0_axi_s_clk),
	[PCIE1_AXI_S_CLK_SRC] = DEFINE_DUMMY_CLK(pcie1_axi_s_clk_src),
	[GCC_PCIE1_AXI_S_BRIDGE_CLK] = DEFINE_DUMMY_CLK(gcc_pcie1_axi_s_bridge_clk),
	[GCC_PCIE1_AXI_S_CLK] = DEFINE_DUMMY_CLK(gcc_pcie1_axi_s_clk),
	[PCIE2_AXI_S_CLK_SRC] = DEFINE_DUMMY_CLK(pcie2_axi_s_clk_src),
	[GCC_PCIE2_AXI_S_BRIDGE_CLK] = DEFINE_DUMMY_CLK(gcc_pcie2_axi_s_bridge_clk),
	[GCC_PCIE2_AXI_S_CLK] = DEFINE_DUMMY_CLK(gcc_pcie2_axi_s_clk),
	[PCIE3_AXI_S_CLK_SRC] = DEFINE_DUMMY_CLK(pcie3_axi_s_clk_src),
	[GCC_PCIE3_AXI_S_BRIDGE_CLK] = DEFINE_DUMMY_CLK(gcc_pcie3_axi_s_bridge_clk),
	[GCC_PCIE3_AXI_S_CLK] = DEFINE_DUMMY_CLK(gcc_pcie3_axi_s_clk),
	[PCIE0_PIPE_CLK_SRC] = DEFINE_DUMMY_CLK(pcie0_pipe_clk_src),
	[GCC_PCIE0_PIPE_CLK] = DEFINE_DUMMY_CLK(gcc_pcie0_pipe_clk),
	[PCIE1_PIPE_CLK_SRC] = DEFINE_DUMMY_CLK(pcie1_pipe_clk_src),
	[GCC_PCIE1_PIPE_CLK] = DEFINE_DUMMY_CLK(gcc_pcie1_pipe_clk),
	[PCIE2_PIPE_CLK_SRC] = DEFINE_DUMMY_CLK(pcie2_pipe_clk_src),
	[GCC_PCIE2_PIPE_CLK] = DEFINE_DUMMY_CLK(gcc_pcie2_pipe_clk),
	[PCIE3_PIPE_CLK_SRC] = DEFINE_DUMMY_CLK(pcie3_pipe_clk_src),
	[GCC_PCIE3_PIPE_CLK] = DEFINE_DUMMY_CLK(gcc_pcie3_pipe_clk),
	[PCIE_AUX_CLK_SRC] = DEFINE_DUMMY_CLK(pcie_aux_clk_src),
	[GCC_PCIE0_AUX_CLK] = DEFINE_DUMMY_CLK(gcc_pcie0_aux_clk),
	[GCC_PCIE1_AUX_CLK] = DEFINE_DUMMY_CLK(gcc_pcie1_aux_clk),
	[GCC_PCIE2_AUX_CLK] = DEFINE_DUMMY_CLK(gcc_pcie2_aux_clk),
	[GCC_PCIE3_AUX_CLK] = DEFINE_DUMMY_CLK(gcc_pcie3_aux_clk),
	[PCIE0_RCHNG_CLK_SRC] = DEFINE_DUMMY_CLK(pcie0_rchng_clk_src),
	[GCC_PCIE0_RCHNG_CLK] = DEFINE_DUMMY_CLK(gcc_pcie0_rchng_clk),
	[PCIE1_RCHNG_CLK_SRC] = DEFINE_DUMMY_CLK(pcie1_rchng_clk_src),
	[GCC_PCIE1_RCHNG_CLK] = DEFINE_DUMMY_CLK(gcc_pcie1_rchng_clk),
	[PCIE2_RCHNG_CLK_SRC] = DEFINE_DUMMY_CLK(pcie2_rchng_clk_src),
	[GCC_PCIE2_RCHNG_CLK] = DEFINE_DUMMY_CLK(gcc_pcie2_rchng_clk),
	[PCIE3_RCHNG_CLK_SRC] = DEFINE_DUMMY_CLK(pcie3_rchng_clk_src),
	[GCC_PCIE3_RCHNG_CLK] = DEFINE_DUMMY_CLK(gcc_pcie3_rchng_clk),
	[GCC_PCIE0_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_pcie0_ahb_clk),
	[GCC_PCIE1_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_pcie1_ahb_clk),
	[GCC_PCIE2_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_pcie2_ahb_clk),
	[GCC_PCIE3_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_pcie3_ahb_clk),
	[USB0_AUX_CLK_SRC] = DEFINE_DUMMY_CLK(usb0_aux_clk_src),
	[GCC_USB0_AUX_CLK] = DEFINE_DUMMY_CLK(gcc_usb0_aux_clk),
	[USB0_MASTER_CLK_SRC] = DEFINE_DUMMY_CLK(usb0_master_clk_src),
	[GCC_USB0_MASTER_CLK] = DEFINE_DUMMY_CLK(gcc_usb0_master_clk),
	[GCC_SNOC_USB_CLK] = DEFINE_DUMMY_CLK(gcc_snoc_usb_clk),
	[GCC_ANOC_USB_AXI_CLK] = DEFINE_DUMMY_CLK(gcc_anoc_usb_axi_clk),
	[USB0_MOCK_UTMI_CLK_SRC] = DEFINE_DUMMY_CLK(usb0_mock_utmi_clk_src),
	[USB0_MOCK_UTMI_DIV_CLK_SRC] = DEFINE_DUMMY_CLK(usb0_mock_utmi_div_clk_src),
	[GCC_USB0_MOCK_UTMI_CLK] = DEFINE_DUMMY_CLK(gcc_usb0_mock_utmi_clk),
	[USB0_PIPE_CLK_SRC] = DEFINE_DUMMY_CLK(usb0_pipe_clk_src),
	[GCC_USB0_PIPE_CLK] = DEFINE_DUMMY_CLK(gcc_usb0_pipe_clk),
	[GCC_USB0_PHY_CFG_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_usb0_phy_cfg_ahb_clk),
	[GCC_USB0_SLEEP_CLK] = DEFINE_DUMMY_CLK(gcc_usb0_sleep_clk),
	[SDCC1_APPS_CLK_SRC] = DEFINE_DUMMY_CLK(sdcc1_apps_clk_src),
	[GCC_SDCC1_APPS_CLK] = DEFINE_DUMMY_CLK(gcc_sdcc1_apps_clk),
	[SDCC1_ICE_CORE_CLK_SRC] = DEFINE_DUMMY_CLK(sdcc1_ice_core_clk_src),
	[GCC_SDCC1_ICE_CORE_CLK] = DEFINE_DUMMY_CLK(gcc_sdcc1_ice_core_clk),
	[GCC_SDCC1_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_sdcc1_ahb_clk),
	[PCNOC_BFDCD_CLK_SRC] = DEFINE_DUMMY_CLK(pcnoc_bfdcd_clk_src),
	[GCC_NSSCFG_CLK] = DEFINE_DUMMY_CLK(gcc_nsscfg_clk),
	[GCC_NSSNOC_NSSCC_CLK] = DEFINE_DUMMY_CLK(gcc_nssnoc_nsscc_clk),
	[GCC_NSSCC_CLK] = DEFINE_DUMMY_CLK(gcc_nsscc_clk),
	[GCC_NSSNOC_PCNOC_1_CLK] = DEFINE_DUMMY_CLK(gcc_nssnoc_pcnoc_1_clk),
	[GCC_QDSS_DAP_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_dap_ahb_clk),
	[GCC_QDSS_CFG_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_cfg_ahb_clk),
	[GCC_SNOC_PCNOC_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_snoc_pcnoc_ahb_clk),
	[GCC_ANOC_PCNOC_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_anoc_pcnoc_ahb_clk),
	[GCC_QPIC_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_qpic_ahb_clk),
	[GCC_QPIC_CLK] = DEFINE_DUMMY_CLK(gcc_qpic_clk),
	[GCC_BLSP1_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_blsp1_ahb_clk),
	[GCC_MDIO_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_mdio_ahb_clk),
	[GCC_PRNG_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_prng_ahb_clk),
	[GCC_UNIPHY0_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_uniphy0_ahb_clk),
	[GCC_UNIPHY1_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_uniphy1_ahb_clk),
	[GCC_UNIPHY2_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_uniphy2_ahb_clk),
	[GCC_CMN_12GPLL_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_cmn_12gpll_ahb_clk),
	[GCC_CMN_12GPLL_APU_CLK] = DEFINE_DUMMY_CLK(gcc_cmn_12gpll_apu_clk),
	[SYSTEM_NOC_BFDCD_CLK_SRC] = DEFINE_DUMMY_CLK(system_noc_bfdcd_clk_src),
	[GCC_NSSNOC_SNOC_CLK] = DEFINE_DUMMY_CLK(gcc_nssnoc_snoc_clk),
	[GCC_NSSNOC_SNOC_1_CLK] = DEFINE_DUMMY_CLK(gcc_nssnoc_snoc_1_clk),
	[GCC_QDSS_ETR_USB_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_etr_usb_clk),
	[GCC_SYS_NOC_AXI_CLK] = DEFINE_DUMMY_CLK(gcc_sys_noc_axi_clk),
	[GCC_ANOC0_AXI_CLK] = DEFINE_DUMMY_CLK(gcc_anoc0_axi_clk),
	[GCC_ANOC1_AXI_CLK] = DEFINE_DUMMY_CLK(gcc_anoc1_axi_clk),
	[GCC_ANOC0_TBU_CLK] = DEFINE_DUMMY_CLK(gcc_anoc0_tbu_clk),
	[GCC_ANOC1_TBU_CLK] = DEFINE_DUMMY_CLK(gcc_anoc1_tbu_clk),
	[GCC_MEM_NOC_SNOC_AXI_CLK] = DEFINE_DUMMY_CLK(gcc_mem_noc_snoc_axi_clk),
	[GCC_IMEM_AXI_CLK] = DEFINE_DUMMY_CLK(gcc_imem_axi_clk),
	[WCSS_AHB_CLK_SRC] = DEFINE_DUMMY_CLK(wcss_ahb_clk_src),
	[GCC_Q6_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_q6_ahb_clk),
	[GCC_Q6_AHB_S_CLK] = DEFINE_DUMMY_CLK(gcc_q6_ahb_s_clk),
	[GCC_WCSS_ECAHB_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_ecahb_clk),
	[GCC_WCSS_ACMT_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_acmt_clk),
	[GCC_WCSS_AHB_S_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_ahb_s_clk),
	[GCC_SYS_NOC_WCSS_AHB_CLK] = DEFINE_DUMMY_CLK(gcc_sys_noc_wcss_ahb_clk),
	[WCSS_AXI_M_CLK_SRC] = DEFINE_DUMMY_CLK(wcss_axi_m_clk_src),
	[GCC_WCSS_AXI_M_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_axi_m_clk),
	[GCC_ANOC_WCSS_AXI_M_CLK] = DEFINE_DUMMY_CLK(gcc_anoc_wcss_axi_m_clk),
	[QDSS_AT_CLK_SRC] = DEFINE_DUMMY_CLK(qdss_at_clk_src),
	[GCC_Q6SS_ATBM_CLK] = DEFINE_DUMMY_CLK(gcc_q6ss_atbm_clk),
	[GCC_WCSS_DBG_IFC_ATB_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_dbg_ifc_atb_clk),
	[GCC_WCSS_DBG_IFC_ATB_BDG_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_dbg_ifc_atb_bdg_clk),
	[GCC_NSSNOC_ATB_CLK] = DEFINE_DUMMY_CLK(gcc_nssnoc_atb_clk),
	[GCC_QDSS_AT_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_at_clk),
	[GCC_SYS_NOC_AT_CLK] = DEFINE_DUMMY_CLK(gcc_sys_noc_at_clk),
	[GCC_PCNOC_AT_CLK] = DEFINE_DUMMY_CLK(gcc_pcnoc_at_clk),
	[GCC_USB0_EUD_AT_CLK] = DEFINE_DUMMY_CLK(gcc_usb0_eud_at_clk),
	[GCC_QDSS_EUD_AT_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_eud_at_clk),
	[QDSS_STM_CLK_SRC] = DEFINE_DUMMY_CLK(qdss_stm_clk_src),
	[GCC_QDSS_STM_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_stm_clk),
	[GCC_SYS_NOC_QDSS_STM_AXI_CLK] = DEFINE_DUMMY_CLK(gcc_sys_noc_qdss_stm_axi_clk),
	[QDSS_TRACECLKIN_CLK_SRC] = DEFINE_DUMMY_CLK(qdss_traceclkin_clk_src),
	[GCC_QDSS_TRACECLKIN_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_traceclkin_clk),
	[QDSS_TSCTR_CLK_SRC] = DEFINE_DUMMY_CLK(qdss_tsctr_clk_src),
	[GCC_Q6_TSCTR_1TO2_CLK] = DEFINE_DUMMY_CLK(gcc_q6_tsctr_1to2_clk),
	[GCC_WCSS_DBG_IFC_NTS_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_dbg_ifc_nts_clk),
	[GCC_WCSS_DBG_IFC_NTS_BDG_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_dbg_ifc_nts_bdg_clk),
	[GCC_QDSS_TSCTR_DIV2_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_tsctr_div2_clk),
	[GCC_QDSS_TS_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_ts_clk),
	[GCC_SNOC_TS_CLK] = DEFINE_DUMMY_CLK(gcc_snoc_ts_clk),
	[GCC_MEM_NOC_TS_CLK] = DEFINE_DUMMY_CLK(gcc_mem_noc_ts_clk),
	[GCC_QDSS_TSCTR_DIV4_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_tsctr_div4_clk),
	[GCC_NSS_TS_CLK] = DEFINE_DUMMY_CLK(gcc_nss_ts_clk),
	[GCC_QDSS_TSCTR_DIV8_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_tsctr_div8_clk),
	[GCC_PCNOC_TS_CLK] = DEFINE_DUMMY_CLK(gcc_pcnoc_ts_clk),
	[GCC_QDSS_TSCTR_DIV16_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_tsctr_div16_clk),
	[GCC_Q6SS_PCLKDBG_CLK] = DEFINE_DUMMY_CLK(gcc_q6ss_pclkdbg_clk),
	[GCC_Q6SS_TRIG_CLK] = DEFINE_DUMMY_CLK(gcc_q6ss_trig_clk),
	[GCC_WCSS_DBG_IFC_APB_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_dbg_ifc_apb_clk),
	[GCC_WCSS_DBG_IFC_DAPBUS_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_dbg_ifc_dapbus_clk),
	[GCC_WCSS_DBG_IFC_APB_BDG_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_dbg_ifc_apb_bdg_clk),
	[GCC_WCSS_DBG_IFC_DAPBUS_BDG_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_dbg_ifc_dapbus_bdg_clk),
	[GCC_UBI32_DBG0_CLK] = DEFINE_DUMMY_CLK(gcc_ubi32_dbg0_clk),
	[GCC_QDSS_DAP_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_dap_clk),
	[GCC_QDSS_APB2JTAG_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_apb2jtag_clk),
	[GCC_QDSS_TSCTR_DIV3_CLK] = DEFINE_DUMMY_CLK(gcc_qdss_tsctr_div3_clk),
	[QPIC_IO_MACRO_CLK_SRC] = DEFINE_DUMMY_CLK(qpic_io_macro_clk_src),
	[GCC_QPIC_IO_MACRO_CLK] = DEFINE_DUMMY_CLK(gcc_qpic_io_macro_clk),
	[Q6_AXI_CLK_SRC] = DEFINE_DUMMY_CLK(q6_axi_clk_src),
	[GCC_Q6_AXIM_CLK] = DEFINE_DUMMY_CLK(gcc_q6_axim_clk),
	[GCC_WCSS_Q6_TBU_CLK] = DEFINE_DUMMY_CLK(gcc_wcss_q6_tbu_clk),
	[GCC_MEM_NOC_Q6_AXI_CLK] = DEFINE_DUMMY_CLK(gcc_mem_noc_q6_axi_clk),
	[Q6_AXIM2_CLK_SRC] = DEFINE_DUMMY_CLK(q6_axim2_clk_src),
	[GCC_Q6_AXIM2_CLK] = DEFINE_DUMMY_CLK(gcc_q6_axim2_clk),
	[NSSNOC_MEMNOC_BFDCD_CLK_SRC] = DEFINE_DUMMY_CLK(nssnoc_memnoc_bfdcd_clk_src),
	[GCC_NSSNOC_MEMNOC_CLK] = DEFINE_DUMMY_CLK(gcc_nssnoc_memnoc_clk),
	[GCC_NSSNOC_MEM_NOC_1_CLK] = DEFINE_DUMMY_CLK(gcc_nssnoc_mem_noc_1_clk),
	[GCC_NSS_TBU_CLK] = DEFINE_DUMMY_CLK(gcc_nss_tbu_clk),
	[GCC_MEM_NOC_NSSNOC_CLK] = DEFINE_DUMMY_CLK(gcc_mem_noc_nssnoc_clk),
	[LPASS_AXIM_CLK_SRC] = DEFINE_DUMMY_CLK(lpass_axim_clk_src),
	[GCC_LPASS_CORE_AXIM_CLK] = DEFINE_DUMMY_CLK(gcc_lpass_core_axim_clk),
	[GCC_PCNOC_LPASS_CLK] = DEFINE_DUMMY_CLK(gcc_pcnoc_lpass_clk),
	[LPASS_SWAY_CLK_SRC] = DEFINE_DUMMY_CLK(lpass_sway_clk_src),
	[GCC_LPASS_SWAY_CLK] = DEFINE_DUMMY_CLK(gcc_lpass_sway_clk),
	[GCC_SNOC_LPASS_CFG_CLK] = DEFINE_DUMMY_CLK(gcc_snoc_lpass_cfg_clk),
	[ADSS_PWM_CLK_SRC] = DEFINE_DUMMY_CLK(adss_pwm_clk_src),
	[GCC_ADSS_PWM_CLK] = DEFINE_DUMMY_CLK(gcc_adss_pwm_clk),
	[GP1_CLK_SRC] = DEFINE_DUMMY_CLK(gp1_clk_src),
	[GP2_CLK_SRC] = DEFINE_DUMMY_CLK(gp2_clk_src),
	[GP3_CLK_SRC] = DEFINE_DUMMY_CLK(gp3_clk_src),
	[GCC_GP1_CLK] = DEFINE_DUMMY_CLK(gcc_gp1_clk),
	[GCC_GP2_CLK] = DEFINE_DUMMY_CLK(gcc_gp2_clk),
	[GCC_GP3_CLK] = DEFINE_DUMMY_CLK(gcc_gp3_clk),
	[DDRSS_SMS_SLOW_CLK_SRC] = DEFINE_DUMMY_CLK(ddrss_sms_slow_clk_src),
	[GCC_DDRSS_SMS_SLOW_CLK] = DEFINE_DUMMY_CLK(gcc_ddrss_sms_slow_clk),
	[GCC_XO_CLK_SRC] = DEFINE_DUMMY_CLK(gcc_xo_clk_src),
	[GCC_NSSNOC_XO_DCD_CLK] = DEFINE_DUMMY_CLK(gcc_nssnoc_xo_dcd_clk),
	[GCC_XO_CLK] = DEFINE_DUMMY_CLK(gcc_xo_clk),
	[GCC_NSSNOC_QOSGEN_REF_CLK] = DEFINE_DUMMY_CLK(gcc_nssnoc_qosgen_ref_clk),
	[GCC_NSSNOC_TIMEOUT_REF_CLK] = DEFINE_DUMMY_CLK(gcc_nssnoc_timeout_ref_clk),
	[GCC_XO_DIV4_CLK] = DEFINE_DUMMY_CLK(gcc_xo_div4_clk),
	[GCC_SPDM_FF_CLK] = DEFINE_DUMMY_CLK(gcc_spdm_ff_clk),
	[GCC_UNIPHY0_SYS_CLK] = DEFINE_DUMMY_CLK(gcc_uniphy0_sys_clk),
	[GCC_UNIPHY1_SYS_CLK] = DEFINE_DUMMY_CLK(gcc_uniphy1_sys_clk),
	[GCC_UNIPHY2_SYS_CLK] = DEFINE_DUMMY_CLK(gcc_uniphy2_sys_clk),
	[GCC_CMN_12GPLL_SYS_CLK] = DEFINE_DUMMY_CLK(gcc_cmn_12gpll_sys_clk),
	[GCC_Q6SS_BOOT_CLK] = DEFINE_DUMMY_CLK(gcc_q6ss_boot_clk),
	[UNIPHY_SYS_CLK_SRC] = DEFINE_DUMMY_CLK(uniphy_sys_clk_src),
	[NSS_TS_CLK_SRC] = DEFINE_DUMMY_CLK(nss_ts_clk_src),
	[GCC_ANOC_PCIE0_1LANE_M_CLK] = DEFINE_DUMMY_CLK(gcc_anoc_pcie0_1lane_m_clk),
	[GCC_ANOC_PCIE1_1LANE_M_CLK] = DEFINE_DUMMY_CLK(gcc_anoc_pcie1_1lane_m_clk),
	[GCC_ANOC_PCIE2_2LANE_M_CLK] = DEFINE_DUMMY_CLK(gcc_anoc_pcie2_2lane_m_clk),
	[GCC_ANOC_PCIE3_2LANE_M_CLK] = DEFINE_DUMMY_CLK(gcc_anoc_pcie3_2lane_m_clk),
	[GCC_SNOC_PCIE0_1LANE_S_CLK] = DEFINE_DUMMY_CLK(gcc_snoc_pcie0_1lane_s_clk),
	[GCC_SNOC_PCIE1_1LANE_S_CLK] = DEFINE_DUMMY_CLK(gcc_snoc_pcie1_1lane_s_clk),
	[GCC_SNOC_PCIE2_2LANE_S_CLK] = DEFINE_DUMMY_CLK(gcc_snoc_pcie2_2lane_s_clk),
	[GCC_SNOC_PCIE3_2LANE_S_CLK] = DEFINE_DUMMY_CLK(gcc_snoc_pcie3_2lane_s_clk),
	[GCC_PCNOC_DCC_CLK] = DEFINE_DUMMY_CLK(gcc_pcnoc_dcc_clk),
};

static const struct qcom_reset_map gcc_ipq9574_resets[] = {
	[GCC_ADSS_BCR] = { 0x1c000, 0 },
	[GCC_ANOC0_TBU_BCR] = { 0x1203C, 0 },
	[GCC_ANOC1_TBU_BCR] = { 0x1204C, 0 },
	[GCC_ANOC_BCR] = { 0x2E074, 0 },
	[GCC_APC0_VOLTAGE_DROOP_DETECTOR_BCR] = { 0x38000, 0 },
	[GCC_APSS_TCU_BCR] = { 0x12014, 0 },
	[GCC_BLSP1_BCR] = { 0x01000, 0 },
	[GCC_BLSP1_QUP1_BCR] = { 0x02000, 0 },
	[GCC_BLSP1_QUP2_BCR] = { 0x03000, 0 },
	[GCC_BLSP1_QUP3_BCR] = { 0x04000, 0 },
	[GCC_BLSP1_QUP4_BCR] = { 0x05000, 0 },
	[GCC_BLSP1_QUP5_BCR] = { 0x06000, 0 },
	[GCC_BLSP1_QUP6_BCR] = { 0x07000, 0 },
	[GCC_BLSP1_UART1_BCR] = { 0x02028, 0 },
	[GCC_BLSP1_UART2_BCR] = { 0x03028, 0 },
	[GCC_BLSP1_UART3_BCR] = { 0x04028, 0 },
	[GCC_BLSP1_UART4_BCR] = { 0x05028, 0 },
	[GCC_BLSP1_UART5_BCR] = { 0x06028, 0 },
	[GCC_BLSP1_UART6_BCR] = { 0x07028, 0 },
	[GCC_BOOT_ROM_BCR] = { 0x13028, 0 },
	[GCC_CMN_BLK_BCR] = { 0x3A000, 0 },
	[GCC_CMN_BLK_AHB_ARES] = { 0x3A010, 0 },
	[GCC_CMN_BLK_SYS_ARES] = { 0x3A010, 1 },
	[GCC_CMN_BLK_APU_ARES] = { 0x3A010, 2 },
	[GCC_DCC_BCR] = { 0x35000, 0 },
	[GCC_DDRSS_BCR] = { 0x11000, 0 },
	[GCC_IMEM_BCR] = { 0x0e000, 0 },
	[GCC_LPASS_BCR] = { 0x27000, 0 },
	[GCC_MDIO_BCR] = { 0x1703c, 0 },
	[GCC_MPM_BCR] = { 0x37000, 0 },
	[GCC_MSG_RAM_BCR] = { 0x26000, 0 },
	[GCC_NSS_BCR] = { 0x17000, 0 },
	[GCC_NSS_TBU_BCR] = { 0x12044, 0 },
	[GCC_NSSNOC_MEMNOC_1_ARES] = { 0x17038, 13 },
	[GCC_NSSNOC_PCNOC_1_ARES] = { 0x17038, 12 },
	[GCC_NSSNOC_SNOC_1_ARES] = { 0x17038,  11 },
	[GCC_NSSNOC_XO_DCD_ARES] = { 0x17038,  10 },
	[GCC_NSSNOC_TS_ARES] = { 0x17038, 9 },
	[GCC_NSSCC_ARES] = { 0x17038, 8 },
	[GCC_NSSNOC_NSSCC_ARES] = { 0x17038, 7 },
	[GCC_NSSNOC_ATB_ARES] = { 0x17038, 6 },
	[GCC_NSSNOC_MEMNOC_ARES] = { 0x17038, 5 },
	[GCC_NSSNOC_QOSGEN_REF_ARES] = { 0x17038, 4 },
	[GCC_NSSNOC_SNOC_ARES] = { 0x17038, 3 },
	[GCC_NSSNOC_TIMEOUT_REF_ARES] = { 0x17038, 2 },
	[GCC_NSS_CFG_ARES] = { 0x17038, 1 },
	[GCC_UBI0_DBG_ARES] = { 0x17038, 0 },
	[GCC_PCIE0PHY_PHY_BCR] = { 0x2805c, 0 },
	[GCC_PCIE0_AHB_ARES] = { 0x28058, 7 },
	[GCC_PCIE0_AUX_ARES] = { 0x28058, 6 },
	[GCC_PCIE0_AXI_M_ARES] = { 0x28058, 5 },
	[GCC_PCIE0_AXI_M_STICKY_ARES] = { 0x28058, 4 },
	[GCC_PCIE0_AXI_S_ARES] = { 0x28058, 3 },
	[GCC_PCIE0_AXI_S_STICKY_ARES] = { 0x28058, 2 },
	[GCC_PCIE0_CORE_STICKY_ARES] = { 0x28058, 1 },
	[GCC_PCIE0_PIPE_ARES] = { 0x28058, 0 },
	[GCC_PCIE1_AHB_ARES] = { 0x29058, 7 },
	[GCC_PCIE1_AUX_ARES] = { 0x29058, 6 },
	[GCC_PCIE1_AXI_M_ARES] = { 0x29058, 5 },
	[GCC_PCIE1_AXI_M_STICKY_ARES] = { 0x29058, 4 },
	[GCC_PCIE1_AXI_S_ARES] = { 0x29058, 3 },
	[GCC_PCIE1_AXI_S_STICKY_ARES] = { 0x29058, 2 },
	[GCC_PCIE1_CORE_STICKY_ARES] = { 0x29058, 1 },
	[GCC_PCIE1_PIPE_ARES] = { 0x29058, 0 },
	[GCC_PCIE2_AHB_ARES] = { 0x2A058, 7 },
	[GCC_PCIE2_AUX_ARES] = { 0x2A058, 6 },
	[GCC_PCIE2_AXI_M_ARES] = { 0x2A058, 5 },
	[GCC_PCIE2_AXI_M_STICKY_ARES] = { 0x2A058, 4 },
	[GCC_PCIE2_AXI_S_ARES] = { 0x2A058, 3 },
	[GCC_PCIE2_AXI_S_STICKY_ARES] = { 0x2A058, 2 },
	[GCC_PCIE2_CORE_STICKY_ARES] = { 0x2A058, 1 },
	[GCC_PCIE2_PIPE_ARES] = { 0x2A058, 0 },
	[GCC_PCIE3_AHB_ARES] = { 0x2B058, 7 },
	[GCC_PCIE3_AUX_ARES] = { 0x2B058, 6 },
	[GCC_PCIE3_AXI_M_ARES] = { 0x2B058, 5 },
	[GCC_PCIE3_AXI_M_STICKY_ARES] = { 0x2B058, 4 },
	[GCC_PCIE3_AXI_S_ARES] = { 0x2B058, 3 },
	[GCC_PCIE3_AXI_S_STICKY_ARES] = { 0x2B058, 2 },
	[GCC_PCIE3_CORE_STICKY_ARES] = { 0x2B058, 1 },
	[GCC_PCIE3_PIPE_ARES] = { 0x2B058, 0 },
	[GCC_PCIE0_BCR] = { 0x28000, 0 },
	[GCC_PCIE0_LINK_DOWN_BCR] = { 0x28054, 0 },
	[GCC_PCIE0_PHY_BCR] = { 0x28060, 0 },
	[GCC_PCIE1_BCR] = { 0x29000, 0 },
	[GCC_PCIE1_LINK_DOWN_BCR] = { 0x29054, 0 },
	[GCC_PCIE1_PHY_BCR] = { 0x29060, 0 },
	[GCC_PCIE1PHY_PHY_BCR] = { 0x2905c, 0 },
	[GCC_PCIE2_BCR] = { 0x2a000, 0 },
	[GCC_PCIE2_LINK_DOWN_BCR] = { 0x2a054, 0 },
	[GCC_PCIE2_PHY_BCR] = { 0x2a060, 0 },
	[GCC_PCIE2PHY_PHY_BCR] = { 0x2a05c, 0 },
	[GCC_PCIE3_BCR] = { 0x2b000, 0 },
	[GCC_PCIE3_LINK_DOWN_BCR] = { 0x2b054, 0 },
	[GCC_PCIE3PHY_PHY_BCR] = { 0x2b05c, 0 },
	[GCC_PCIE3_PHY_BCR] = { 0x2B060, 0 },
	[GCC_PCNOC_BCR] = { 0x31000, 0 },
	[GCC_PCNOC_BUS_TIMEOUT0_BCR] = { 0x31030, 0 },
	[GCC_PCNOC_BUS_TIMEOUT1_BCR] = { 0x31038, 0 },
	[GCC_PCNOC_BUS_TIMEOUT2_BCR] = { 0x31040, 0 },
	[GCC_PCNOC_BUS_TIMEOUT3_BCR] = { 0x31048, 0 },
	[GCC_PCNOC_BUS_TIMEOUT4_BCR] = { 0x31050, 0 },
	[GCC_PCNOC_BUS_TIMEOUT5_BCR] = { 0x31058, 0 },
	[GCC_PCNOC_BUS_TIMEOUT6_BCR] = { 0x31060, 0 },
	[GCC_PCNOC_BUS_TIMEOUT7_BCR] = { 0x31068, 0 },
	[GCC_PCNOC_BUS_TIMEOUT8_BCR] = { 0x31070, 0 },
	[GCC_PCNOC_BUS_TIMEOUT9_BCR] = { 0x31078, 0 },
	[GCC_PCNOC_TBU_BCR] = { 0x12034, 0 },
	[GCC_PRNG_BCR] = { 0x13020, 0 },
	[GCC_Q6SS_DBG_ARES] = { 0x2506C, 4 },
	[GCC_Q6_AHB_ARES] = { 0x2506C, 3 },
	[GCC_Q6_AHB_S_ARES] = { 0x2506C, 2 },
	[GCC_Q6_AXIM2_ARES] = { 0x2506C, 1 },
	[GCC_Q6_AXIM_ARES] = { 0x2506C, 0 },
	[GCC_QDSS_BCR] = { 0x2d000, 0 },
	[GCC_QPIC_BCR] = { 0x32000, 0 },
	[GCC_QPIC_AHB_ARES] = { 0x3201c, 1 },
	[GCC_QPIC_ARES] = { 0x3201c, 0 },
	[GCC_QUSB2_0_PHY_BCR] = { 0x2c068, 0 },
	[GCC_RBCPR_BCR] = { 0x39000, 0 },
	[GCC_RBCPR_MX_BCR] = { 0x39014, 0 },
	[GCC_SDCC_BCR] = { 0x33000, 0 },
	[GCC_SEC_CTRL_BCR] = { 0x1a000, 0 },
	[GCC_SMMU_CFG_BCR] = { 0x1202c, 0 },
	[GCC_SNOC_BCR] = { 0x2E000, 0 },
	[GCC_SPDM_BCR] = { 0x36000, 0 },
	[GCC_TCSR_BCR] = { 0x3d000, 0 },
	[GCC_TLMM_BCR] = { 0x3e000, 0 },
	[GCC_TME_BCR] = { 0x10000, 0 },
	[GCC_UNIPHY0_BCR] = { 0x17044, 0 },
	[GCC_UNIPHY0_SYS_RESET] = { 0x17050, 0 },
	[GCC_UNIPHY0_AHB_RESET] = { 0x17050, 1 },
	[GCC_UNIPHY0_XPCS_RESET] = { 0x17050, 2 },
	[GCC_UNIPHY1_SYS_RESET] = { 0x17060, 0 },
	[GCC_UNIPHY1_AHB_RESET] = { 0x17060, 1 },
	[GCC_UNIPHY1_XPCS_RESET] = { 0x17060, 2 },
	[GCC_UNIPHY2_SYS_RESET] = { 0x17070, 0 },
	[GCC_UNIPHY2_AHB_RESET] = { 0x17070, 1 },
	[GCC_UNIPHY2_XPCS_RESET] = { 0x17070, 2 },
	[GCC_UNIPHY1_BCR] = { 0x17054, 0 },
	[GCC_UNIPHY2_BCR] = { 0x17064, 0 },
	[GCC_USB0_PHY_BCR] = { 0x2c06c, 0 },
	[GCC_USB3PHY_0_PHY_BCR] = { 0x2c070, 0 },
	[GCC_USB_BCR] = { 0x2c000, 0 },
	[GCC_USB_MISC_RESET] = { 0x2c064, 0 },
	[GCC_WCSSAON_RESET] = { 0x25074, 0 },
	[GCC_WCSS_ACMT_ARES] = { 0x25070, 5 },
	[GCC_WCSS_AHB_S_ARES] = { 0x25070, 4 },
	[GCC_WCSS_AXI_M_ARES] = { 0x25070, 3 },
	[GCC_WCSS_BCR] = { 0x18004, 0 },
	[GCC_WCSS_DBG_ARES] = { 0x25070, 2 },
	[GCC_WCSS_DBG_BDG_ARES] = { 0x25070, 1 },
	[GCC_WCSS_ECAHB_ARES] = { 0x25070, 0 },
	[GCC_WCSS_Q6_BCR] = { 0x18000, 0 },
	[GCC_WCSS_Q6_TBU_BCR] = { 0x12054, 0 },
};

static const struct of_device_id gcc_ipq9574_match_table[] = {
	{ .compatible = "qcom,gcc-ipq9574" },
	{ }
};
MODULE_DEVICE_TABLE(of, gcc_ipq9574_match_table);

static const struct regmap_config gcc_ipq9574_regmap_config = {
	.reg_bits       = 32,
	.reg_stride     = 4,
	.val_bits       = 32,
	.max_register   = 0x7fffc,
	.fast_io	= true,
};

static const struct qcom_cc_desc gcc_ipq9574_desc = {
	.config = &gcc_ipq9574_regmap_config,
	.clks = gcc_ipq9574_clks,
	.num_clks = ARRAY_SIZE(gcc_ipq9574_clks),
	.resets = gcc_ipq9574_resets,
	.num_resets = ARRAY_SIZE(gcc_ipq9574_resets),
	.clk_hws = gcc_ipq9574_hws,
	.num_clk_hws = ARRAY_SIZE(gcc_ipq9574_hws),
};

static const struct qcom_cc_desc gcc_ipq9574_dummy_desc = {
	.config = &gcc_ipq9574_regmap_config,
	.clks = gcc_ipq9574_dummy_clks,
	.num_clks = ARRAY_SIZE(gcc_ipq9574_dummy_clks),
	.resets = gcc_ipq9574_resets,
	.num_resets = ARRAY_SIZE(gcc_ipq9574_resets),
};

static int gcc_ipq9574_probe(struct platform_device *pdev)
{
	int ret;
	struct regmap *regmap;
	struct device *dev = &pdev->dev;
	struct qcom_cc_desc ipq9574_desc = gcc_ipq9574_desc;
	struct device_node *np = (&pdev->dev)->of_node;

	if (of_property_read_bool(np, "gcc-use-dummy"))
		ipq9574_desc = gcc_ipq9574_dummy_desc;

	regmap = qcom_cc_map(pdev, &ipq9574_desc);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	clk_register_fixed_rate(dev, "pcie30_phy0_pipe_clk", NULL, 0,
							250000000);
	clk_register_fixed_rate(dev, "pcie30_phy1_pipe_clk", NULL, 0,
							250000000);
	clk_register_fixed_rate(dev, "pcie30_phy2_pipe_clk", NULL, 0,
							250000000);
	clk_register_fixed_rate(dev, "pcie30_phy3_pipe_clk", NULL, 0,
							250000000);

	ret = qcom_cc_really_probe(pdev, &ipq9574_desc, regmap);

	dev_info(&pdev->dev, "Registered ipq9574 GCC clock provider");

	return ret;
}

static int gcc_ipq9574_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver gcc_ipq9574_driver = {
	.probe = gcc_ipq9574_probe,
	.remove = gcc_ipq9574_remove,
	.driver = {
		.name   = "qcom,gcc-ipq9574",
		.owner  = THIS_MODULE,
		.of_match_table = gcc_ipq9574_match_table,
	},
};

static int __init gcc_ipq9574_init(void)
{
	return platform_driver_register(&gcc_ipq9574_driver);
}
core_initcall(gcc_ipq9574_init);

static void __exit gcc_ipq9574_exit(void)
{
	platform_driver_unregister(&gcc_ipq9574_driver);
}
module_exit(gcc_ipq9574_exit);

MODULE_DESCRIPTION("Qualcomm Technologies, Inc. GCC IPQ9574 Driver");
MODULE_LICENSE("GPL v2");
