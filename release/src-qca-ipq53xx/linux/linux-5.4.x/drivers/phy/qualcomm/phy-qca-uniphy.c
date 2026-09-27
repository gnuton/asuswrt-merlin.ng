/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
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
#include <linux/regulator/consumer.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/qcom_scm.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

#define USB_CALIBRATION_CMD	0x10
#define USB3PHY_SPARE_1		0x7FC
#define RX_LOS_1		0x7C8
#define MISC_SOURCE_REG		0x21c
#define CDR_CONTROL_REG_1	0x80
#define PCS_INTERNAL_CONTROL14	0x364
#define MMD1_REG_REG_MASK	(0x7F << 8)
#define OTP_MASK		(0x7F << 5)
#define MMD1_REG_AUTOLOAD_MASK	(0x1 << 7)
#define SPARE_1_BIT14_MASK	(0x1 << 14)
#define SSCG_CTRL_REG_1		0x9c
#define SSCG_CTRL_REG_2		0xa0
#define SSCG_CTRL_REG_3		0xa4
#define SSCG_CTRL_REG_4		0xa8
#define SSCG_CTRL_REG_5		0xac
#define SSCG_CTRL_REG_6		0xb0

#define PHY_AUTOLOAD_PERIOD	35

#define PCIE_USB_COMBO_PHY_CFG_RX_AFE_2		0x7C4
#define PCIE_USB_COMBO_PHY_CFG_RX_DLF_DEMUX_2	0x7E8
#define PCIE_USB_COMBO_PHY_CFG_MISC1		0x214

#define APB_REG_UPHY_RX_RESCAL_CODE	(16 << 8)
#define APB_REG_UPHY_RX_AFE_CAP1	(7 << 4)
#define APB_REG_UPHY_RX_AFE_RES1	(6 << 0)

#define APB_REG_UPHY_RXD_BIT_WIDTH	(2 << 0)
#define APB_REG_UPHY_RX_PLOOP_GAIN	(4 << 4)
#define APB_REG_UPHY_RX_DLF_RATE	(1 << 8)
#define APB_UPHY_RX_PLOOP_EN		(1 << 12)
#define APB_REG_UPHY_RX_CDR_EN		(1 << 13)

#define APB_REG_FLOOP_GAIN		(3 << 0)

struct qca_uni_ss_phy {
	struct phy phy;
	struct device *dev;

	void __iomem *base;

	struct reset_control *por_rst;

	unsigned int host;
	struct clk *pipe_clk;
	struct clk *phy_cfg_ahb_clk;
	struct clk *phy_ahb_clk;
};

struct qf_read {
	uint32_t value;
};

#define	phy_to_dw_phy(x)	container_of((x), struct qca_uni_ss_phy, phy)

static int qca_uni_ss_phy_shutdown(struct phy *x)
{
	struct qca_uni_ss_phy *phy = phy_get_drvdata(x);
	int ret = 0;

	/* assert SS PHY POR reset */
	reset_control_assert(phy->por_rst);

	return ret;
}

static void phy_autoload(void)
{
	int temp = 0;

	while (temp < PHY_AUTOLOAD_PERIOD) {
		udelay(1);
		temp += 1;
	}
}

static int qca_uni_ss_phy_init(struct phy *x)
{
	int ret;
	struct qca_uni_ss_phy *phy = phy_get_drvdata(x);
	const char *compat_name;

	ret = of_property_read_string(phy->dev->of_node, "compatible",
					&compat_name);
	if (ret) {
		dev_err(phy->dev, "couldn't compatible string: %d\n", ret);
		return ret;
	}

	/* assert SS PHY POR reset */
	reset_control_assert(phy->por_rst);
	usleep_range(1, 5);
	/* deassert SS PHY POR reset */
	reset_control_deassert(phy->por_rst);
	clk_prepare_enable(phy->phy_ahb_clk);
	clk_prepare_enable(phy->phy_cfg_ahb_clk);
	clk_prepare_enable(phy->pipe_clk);
	phy_autoload();

	if (!strcmp(compat_name, "qca,ipq5332-uni-ssphy")) {
		writel(APB_REG_UPHY_RX_RESCAL_CODE |
		APB_REG_UPHY_RX_AFE_CAP1 |
		APB_REG_UPHY_RX_AFE_RES1,
		phy->base + PCIE_USB_COMBO_PHY_CFG_RX_AFE_2);

		writel(APB_REG_UPHY_RXD_BIT_WIDTH |
		APB_REG_UPHY_RX_PLOOP_GAIN |
		APB_REG_UPHY_RX_DLF_RATE |
		APB_UPHY_RX_PLOOP_EN |
		APB_REG_UPHY_RX_CDR_EN,
		phy->base + PCIE_USB_COMBO_PHY_CFG_RX_DLF_DEMUX_2);

		writel(APB_REG_FLOOP_GAIN,
			phy->base + PCIE_USB_COMBO_PHY_CFG_MISC1);
		return 0;
	}

	/*set frequency initial value*/
	writel(0x1cb9, phy->base + SSCG_CTRL_REG_4);
	writel(0x023a, phy->base + SSCG_CTRL_REG_5);
	/*set spectrum spread count*/
	writel(0xd360, phy->base + SSCG_CTRL_REG_3);
	/*set fstep*/
	writel(0x1, phy->base + SSCG_CTRL_REG_1);
	writel(0xeb, phy->base + SSCG_CTRL_REG_2);
	return ret;
}

static int qca_uni_ss_get_resources(struct platform_device *pdev,
		struct qca_uni_ss_phy *phy)
{
	struct resource *res;
	struct device_node *np = NULL;
	const char *compat_name;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	phy->base = devm_ioremap_resource(phy->dev, res);
	if (IS_ERR(phy->base))
		return PTR_ERR(phy->base);

	np = of_node_get(pdev->dev.of_node);
	ret = of_property_read_string(np, "compatible", &compat_name);
	if (ret) {
		dev_err(&pdev->dev, "couldn't compatible string: %d\n", ret);
		return ret;
	}

	phy->por_rst = devm_reset_control_get(phy->dev, "por_rst");
	if (IS_ERR(phy->por_rst))
		return PTR_ERR(phy->por_rst);

	if (!strcmp(compat_name, "qca,ipq5018-uni-ssphy") ||
			!strcmp(compat_name, "qca,ipq5332-uni-ssphy")) {
		phy->pipe_clk = devm_clk_get(phy->dev, "pipe_clk");
		if (IS_ERR(phy->pipe_clk)) {
			dev_err(phy->dev, "can not get phy clock\n");
			return PTR_ERR(phy->pipe_clk);
		}

		phy->phy_cfg_ahb_clk = devm_clk_get(phy->dev,
					"phy_cfg_ahb_clk");
		if (IS_ERR(phy->phy_cfg_ahb_clk)) {
			dev_err(phy->dev, "can not get phy cfg ahb clock\n");
			return PTR_ERR(phy->phy_cfg_ahb_clk);
		}

		phy->phy_ahb_clk = devm_clk_get_optional(phy->dev, "phy_ahb_clk");
		if (IS_ERR(phy->phy_ahb_clk)) {
			dev_err(phy->dev, "cannot get phy ahb clock");
			return PTR_ERR(phy->phy_ahb_clk);
		}
	} else {
		if (of_property_read_u32(np, "qca,host", &phy->host)) {
			pr_err("%s: error reading critical device node properties\n",
					np->name);
			return -EFAULT;
		}
	}
	return 0;
}

static const struct of_device_id qca_uni_ss_id_table[] = {
	{ .compatible = "qca,uni-ssphy" },
	{ .compatible = "qca,ipq5018-uni-ssphy"},
	{ .compatible = "qca,ipq5332-uni-ssphy"},
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, qca_uni_ss_id_table);

static const struct phy_ops ops = {
	.init           = qca_uni_ss_phy_init,
	.exit           = qca_uni_ss_phy_shutdown,
	.owner          = THIS_MODULE,
};
static int qca_uni_ss_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct qca_uni_ss_phy  *phy;
	int ret;
	struct phy *generic_phy;
	struct phy_provider *phy_provider;

	match = of_match_device(qca_uni_ss_id_table, &pdev->dev);
	if (!match)
		return -ENODEV;

	phy = devm_kzalloc(&pdev->dev, sizeof(*phy), GFP_KERNEL);
	if (!phy)
		return -ENOMEM;

	platform_set_drvdata(pdev, phy);
	phy->dev = &pdev->dev;

	ret = qca_uni_ss_get_resources(pdev, phy);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request resources: %d\n", ret);
		return ret;
	}

	generic_phy = devm_phy_create(phy->dev, NULL, &ops);
	if (IS_ERR(generic_phy))
		return PTR_ERR(generic_phy);

	phy_set_drvdata(generic_phy, phy);
	phy_provider = devm_of_phy_provider_register(phy->dev,
			of_phy_simple_xlate);
	if (IS_ERR(phy_provider))
		return PTR_ERR(phy_provider);

	return 0;
}

static struct platform_driver qca_uni_ss_driver = {
	.probe		= qca_uni_ss_probe,
	.driver		= {
		.name	= "qca-uni-ssphy",
		.owner	= THIS_MODULE,
		.of_match_table = qca_uni_ss_id_table,
	},
};

module_platform_driver(qca_uni_ss_driver);

MODULE_ALIAS("platform:qti-uni-ssphy");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("USB3 QTI UNI SSPHY driver");
