/* Copyright (c) 2015, 2017, 2020, The Linux Foundation. All rights reserved.
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
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>

#define PIPE_CLK_DELAY_MIN_US			5000
#define PIPE_CLK_DELAY_MAX_US			5100
#define CDR_CTRL_REG_1		0x80
#define CDR_CTRL_REG_2		0x84
#define CDR_CTRL_REG_3		0x88
#define CDR_CTRL_REG_4		0x8C
#define CDR_CTRL_REG_5		0x90
#define CDR_CTRL_REG_6		0x94
#define CDR_CTRL_REG_7		0x98
#define SSCG_CTRL_REG_1		0x9c
#define SSCG_CTRL_REG_2		0xa0
#define SSCG_CTRL_REG_3		0xa4
#define SSCG_CTRL_REG_4		0xa8
#define SSCG_CTRL_REG_5		0xac
#define SSCG_CTRL_REG_6		0xb0
#define PCS_INTERNAL_CONTROL_2	0x2d8

#define PHY_MODE_FIXED		0x1

/* IPQ5332 specific registers */
#define PHY_CFG_PLLCFG			0x220
#define PHY_CFG_EIOS_DTCT_REG		0x3E4
#define PHY_CFG_GEN3_ALIGN_HOLDOFF_TIME	0x3E8

/* PCIe mode control register values */
#define TCSR_2LANE_MODE			0x0
#define TCSR_2PORT_MODE			0x1

enum qca_uni_pcie_phy_type {
	PHY_TYPE_PCIE,
	PHY_TYPE_PCIE_GEN2,
	PHY_TYPE_PCIE_GEN3,
};

struct qca_uni_pcie_phy {
	struct phy phy;
	struct device *dev;
	unsigned int phy_type;
	struct clk *pipe_clk;
	struct clk *lane_m_clk;
	struct clk *lane_s_clk;
	struct clk *phy_ahb_clk;
	struct reset_control *res_phy;
	struct reset_control *res_phy_phy;
	struct reset_control *res_phy_ahb;
	u32 is_phy_gen3;
	u32 mode;
	u32 is_x2;
	void __iomem *reg_base;
        struct regmap *phy_mux_map;
        u32 phy_mux_reg;
	bool phy_ahb_shared_reset;
};

#define	phy_to_dw_phy(x)	container_of((x), struct qca_uni_pcie_phy, phy)

static int qca_uni_pcie_phy_power_off(struct phy *x)
{
	struct qca_uni_pcie_phy *phy = phy_get_drvdata(x);

	reset_control_assert(phy->res_phy);
	reset_control_assert(phy->res_phy_phy);
	reset_control_assert(phy->res_phy_ahb);

	return 0;
}

static int qca_uni_pcie_phy_reset(struct qca_uni_pcie_phy *phy)
{
	/* phy_ahb reset is shared between pcie1 and pcie2 */
	if(phy->phy_ahb_shared_reset)
		reset_control_deassert(phy->res_phy_ahb);

	reset_control_assert(phy->res_phy);
	reset_control_assert(phy->res_phy_phy);
	reset_control_assert(phy->res_phy_ahb);

	usleep_range(100, 150);

	reset_control_deassert(phy->res_phy);
	reset_control_deassert(phy->res_phy_phy);
	reset_control_deassert(phy->res_phy_ahb);

	return 0;
}

static void qca_uni_pcie_phy_init(struct qca_uni_pcie_phy *phy)
{
	int loop = 0;
	void __iomem *reg = phy->reg_base;

	while (loop < 2) {
		reg += (loop * 0x800);
		if (phy->is_phy_gen3) {
			writel(0x30, reg + PHY_CFG_PLLCFG);
			writel(0x53EF, reg + PHY_CFG_EIOS_DTCT_REG);
			writel(0xCF, reg + PHY_CFG_GEN3_ALIGN_HOLDOFF_TIME);
		} else {
			/*set frequency initial value*/
			writel(0x1cb9, reg + SSCG_CTRL_REG_4);
			writel(0x023a, reg + SSCG_CTRL_REG_5);
			/*set spectrum spread count*/
			writel(0xd360, reg + SSCG_CTRL_REG_3);
			if (phy->mode == PHY_MODE_FIXED) {
				/*set fstep*/
				writel(0x0, reg + SSCG_CTRL_REG_1);
				writel(0x0, reg + SSCG_CTRL_REG_2);
			} else {
				/*set fstep*/
				writel(0x1, reg + SSCG_CTRL_REG_1);
				writel(0xeb, reg + SSCG_CTRL_REG_2);
				/*set FLOOP initial value*/
				writel(0x3f9, reg + CDR_CTRL_REG_4);
				writel(0x1c9, reg + CDR_CTRL_REG_5);
				/*set upper boundary level*/
				writel(0x419, reg + CDR_CTRL_REG_2);
				/*set fixed offset*/
				writel(0x200, reg + CDR_CTRL_REG_1);
				writel(0xf101, reg + PCS_INTERNAL_CONTROL_2);
			}
		}

		if (phy->is_x2)
			loop += 1;
		else
			break;
	}
}

static int qca_uni_pcie_phy_power_on(struct phy *x)
{
	struct qca_uni_pcie_phy *phy = phy_get_drvdata(x);

	qca_uni_pcie_phy_reset(phy);
	if (phy->is_phy_gen3)
		clk_set_rate(phy->pipe_clk, 250000000);
	else
		clk_set_rate(phy->pipe_clk, 125000000);

	usleep_range(PIPE_CLK_DELAY_MIN_US, PIPE_CLK_DELAY_MAX_US);
	clk_prepare_enable(phy->pipe_clk);
	clk_prepare_enable(phy->lane_m_clk);
	clk_prepare_enable(phy->lane_s_clk);
	clk_prepare_enable(phy->phy_ahb_clk);
	usleep_range(30, 50);
	qca_uni_pcie_phy_init(phy);
	return 0;
}

static int phy_mux_sel(struct qca_uni_pcie_phy *phy, unsigned int mode)
{
	struct of_phandle_args args;
	int ret;

	ret = of_parse_phandle_with_fixed_args(phy->dev->of_node,
			"qti,phy-mux-regs", 1, 0, &args);
	if (ret)
		return ret;

	phy->phy_mux_map = syscon_node_to_regmap(args.np);
	of_node_put(args.np);
	if (IS_ERR(phy->phy_mux_map)) {
		pr_err("phy mux regs map failed: %ld\n",
						PTR_ERR(phy->phy_mux_map));
		return PTR_ERR(phy->phy_mux_map);
	}

	phy->phy_mux_reg = args.args[0];

	/* Select mode. Two lane or Two port */
	ret = regmap_write(phy->phy_mux_map, phy->phy_mux_reg, mode);

	if (ret) {
		dev_err(phy->dev,
			"Not able to configure phy mux selection: %d\n", ret);
		return ret;
	}

	return 0;
}

static int qca_uni_pcie_get_resources(struct platform_device *pdev,
		struct qca_uni_pcie_phy *phy)
{
	int ret;
	const char *name;
	struct resource *res;

	phy->phy_ahb_shared_reset = device_property_read_bool(phy->dev, "phy-ahb-shared-reset");

	ret = of_property_read_u32(phy->dev->of_node, "x2", &phy->is_x2);
	if (ret)
		phy->is_x2 = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	phy->reg_base = devm_ioremap_resource(phy->dev, res);
	if (IS_ERR(phy->reg_base)) {
		dev_err(phy->dev, "cannot get phy registers\n");
		return PTR_ERR(phy->reg_base);
	}

	phy->pipe_clk = devm_clk_get(phy->dev, "pipe_clk");
	if (IS_ERR(phy->pipe_clk)) {
		dev_err(phy->dev, "cannot get pipe clock");
		return PTR_ERR(phy->pipe_clk);
	}

	phy->lane_m_clk = devm_clk_get_optional(phy->dev, "lane_m_clk");
	if (IS_ERR(phy->lane_m_clk)) {
		dev_err(phy->dev, "cannot get lane_m clock");
		return PTR_ERR(phy->lane_m_clk);
	}

	phy->lane_s_clk = devm_clk_get_optional(phy->dev, "lane_s_clk");
	if (IS_ERR(phy->lane_s_clk)) {
		dev_err(phy->dev, "cannot get lane_s clock");
		return PTR_ERR(phy->lane_s_clk);
	}

	phy->phy_ahb_clk = devm_clk_get_optional(phy->dev, "phy_ahb_clk");
	if (IS_ERR(phy->phy_ahb_clk)) {
		dev_err(phy->dev, "cannot get phy_ahb clock");
		return PTR_ERR(phy->phy_ahb_clk);
	}

	phy->res_phy = devm_reset_control_get(phy->dev, "phy");
	if (IS_ERR(phy->res_phy)) {
		dev_err(phy->dev, "cannot get phy reset controller");
		return PTR_ERR(phy->res_phy);
	}

	phy->res_phy_phy = devm_reset_control_get_optional(phy->dev, "phy_phy");
	if (IS_ERR(phy->res_phy_phy)) {
		dev_err(phy->dev, "cannot get phy_phy reset controller");
		return PTR_ERR(phy->res_phy_phy);
	}

	if(phy->phy_ahb_shared_reset)
		phy->res_phy_ahb = devm_reset_control_get_optional_shared(phy->dev, "phy_ahb");
	else
		phy->res_phy_ahb = devm_reset_control_get_optional(phy->dev, "phy_ahb");

	if (IS_ERR(phy->res_phy_ahb)) {
		dev_err(phy->dev, "cannot get phy_ahb reset controller");
		return PTR_ERR(phy->res_phy_ahb);
	}

	ret = of_property_read_string(phy->dev->of_node, "phy-type", &name);
	if (!ret) {
		if (!strcmp(name, "gen3")) {
			phy->phy_type = PHY_TYPE_PCIE_GEN3;
			phy->is_phy_gen3 = 1;
		} else if (!strcmp(name, "gen2"))
			phy->phy_type = PHY_TYPE_PCIE_GEN2;
		else if (!strcmp(name, "gen1"))
			phy->phy_type = PHY_TYPE_PCIE;
	} else {
		dev_err(phy->dev, "%s, unknown gen type\n", __func__);
		return ret;
	}

	ret = of_property_read_u32(phy->dev->of_node, "mode-fixed", &phy->mode);
	if (ret) {
		dev_err(phy->dev, "%s, cannot get mode\n", __func__);
		return ret;
	}

	if (device_property_read_bool(phy->dev, "qti,multiplexed-phy"))
		phy_mux_sel(phy, TCSR_2PORT_MODE);
	else
		phy_mux_sel(phy, TCSR_2LANE_MODE);

	return 0;
}

static const struct of_device_id qca_uni_pcie_id_table[] = {
	{ .compatible = "qca,uni-pcie-phy", .data = (void *)PHY_TYPE_PCIE},
	{ .compatible = "qca,uni-pcie-phy-gen2",
		.data = (void *)PHY_TYPE_PCIE_GEN2},
	{ .compatible = "qca,uni-pcie-phy-gen3",
		.data = (void *)PHY_TYPE_PCIE_GEN3},
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, qca_uni_pcie_id_table);

static const struct phy_ops pcie_ops = {
	.power_on	= qca_uni_pcie_phy_power_on,
	.power_off	= qca_uni_pcie_phy_power_off,
	.owner          = THIS_MODULE,
};

static int qca_uni_pcie_probe(struct platform_device *pdev)
{
	struct qca_uni_pcie_phy  *phy;
	int ret;
	struct phy *generic_phy;
	struct phy_provider *phy_provider;

	phy = devm_kzalloc(&pdev->dev, sizeof(*phy), GFP_KERNEL);
	if (!phy)
		return -ENOMEM;

	platform_set_drvdata(pdev, phy);
	phy->dev = &pdev->dev;

	ret = qca_uni_pcie_get_resources(pdev, phy);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get resources: %d\n", ret);
		return ret;
	}

	generic_phy = devm_phy_create(phy->dev, NULL, &pcie_ops);
	if (IS_ERR(generic_phy))
		return PTR_ERR(generic_phy);

	phy_set_drvdata(generic_phy, phy);
	phy_provider = devm_of_phy_provider_register(phy->dev,
			of_phy_simple_xlate);
	if (IS_ERR(phy_provider))
		return PTR_ERR(phy_provider);

	return 0;
}

static int qca_uni_pcie_remove(struct platform_device *pdev)
{
	struct qca_uni_pcie_phy  *phy = platform_get_drvdata(pdev);

	clk_disable_unprepare(phy->pipe_clk);
	clk_disable_unprepare(phy->lane_m_clk);
	clk_disable_unprepare(phy->lane_s_clk);
	clk_disable_unprepare(phy->phy_ahb_clk);

	return 0;
}

static struct platform_driver qca_uni_pcie_driver = {
	.probe		= qca_uni_pcie_probe,
	.remove		= qca_uni_pcie_remove,
	.driver		= {
		.name	= "qca-uni-pcie-phy",
		.owner	= THIS_MODULE,
		.of_match_table = qca_uni_pcie_id_table,
	},
};

module_platform_driver(qca_uni_pcie_driver);

MODULE_ALIAS("platform:qti-uni-pcie-phy");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("PCIE QTI UNIPHY driver");
