// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2014-2016, 2020, The Linux Foundation. All rights reserved.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/usb/phy.h>
#include <linux/reset.h>
#include <linux/of_device.h>

enum clk_reset_action {
	CLK_RESET_DEASSERT	= 0,
	CLK_RESET_ASSERT	= 1
};

#define USB2PHY_PORT_POWERDOWN		0xA4
#define POWER_UP			BIT(0)
#define POWER_DOWN			0

#define USB2PHY_PORT_UTMI_CTRL1	0x40

#define USB2PHY_PORT_UTMI_CTRL2	0x44
#define UTMI_ULPI_SEL			BIT(7)
#define UTMI_TEST_MUX_SEL		BIT(6)

#define HS_PHY_CTRL_REG			0x10
#define UTMI_OTG_VBUS_VALID             BIT(20)
#define SW_SESSVLD_SEL                  BIT(28)

#define USB_PHY_CFG0			0x94
#define USB_PHY_UTMI_CTRL5		0x50
#define USB_PHY_FSEL_SEL		0xB8
#define USB_PHY_HS_PHY_CTRL_COMMON0	0x54
#define USB_PHY_REFCLK_CTRL		0xA0
#define USB_PHY_HS_PHY_CTRL2		0x64
#define USB_PHY_UTMI_CTRL0		0x3c
#define USB2PHY_USB_PHY_M31_XCFGI_1	0xBC
#define USB2PHY_USB_PHY_M31_XCFGI_4	0xC8
#define USB2PHY_USB_PHY_M31_XCFGI_5	0xCC
#define USB2PHY_USB_PHY_M31_XCFGI_9	0xDC
#define USB2PHY_USB_PHY_M31_XCFGI_11	0xE4

#define USB2_0_TX_ENABLE		BIT(2)
#define HSTX_SLEW_RATE_400PS		7
#define PLL_CHARGING_PUMP_CURRENT_35UA	(3 << 3)
#define ODT_VALUE_38_02_OHM		(3 << 6)
#define HSTX_PRE_EMPHASIS_LEVEL_0_55MA	(1)
#define HSTX_CURRENT_17_1MA_385MV	BIT(1)

#define UTMI_PHY_OVERRIDE_EN		BIT(1)
#define POR_EN				BIT(1)
#define FREQ_SEL			BIT(0)
#define COMMONONN			BIT(7)
#define FSEL				BIT(4)
#define RETENABLEN			BIT(3)
#define USB2_SUSPEND_N_SEL		BIT(3)
#define USB2_SUSPEND_N			BIT(2)
#define USB2_UTMI_CLK_EN		BIT(1)
#define CLKCORE				BIT(1)
#define ATERESET			~BIT(0)
#define FREQ_24MHZ			(5 << 4)
#define XCFG_COARSE_TUNE_NUM		(2 << 0)
#define XCFG_FINE_TUNE_NUM		(1 << 3)

static void m31usb_write_readback(void *base, u32 offset,
					const u32 mask, u32 val);

struct m31usb_phy {
	struct usb_phy		phy;
	void __iomem		*base;
	void __iomem		*qscratch_base;

	struct reset_control	*phy_reset;

	bool			cable_connected;
	bool			suspended;
	bool			ulpi_mode;
};

static void m31usb_reset(struct m31usb_phy *qphy, u32 action)
{
	if (action == CLK_RESET_ASSERT)
		reset_control_assert(qphy->phy_reset);
	else
		reset_control_deassert(qphy->phy_reset);
	wmb(); /* ensure data is written to hw register */
}

static void m31usb_phy_enable_clock(struct m31usb_phy *qphy)
{
	/* Enable override ctrl */
	writel(UTMI_PHY_OVERRIDE_EN, qphy->base + USB_PHY_CFG0);
	/* Enable POR*/
	writel(POR_EN, qphy->base + USB_PHY_UTMI_CTRL5);
	udelay(15);
	/* Configure frequency select value*/
	writel(FREQ_SEL, qphy->base + USB_PHY_FSEL_SEL);
	/* Configure refclk frequency */
	writel(COMMONONN | FSEL | RETENABLEN,
		qphy->base + USB_PHY_HS_PHY_CTRL_COMMON0);
	/* select refclk source */
	writel(CLKCORE, qphy->base + USB_PHY_REFCLK_CTRL);
	/* select ATERESET*/
	writel(POR_EN & ATERESET, qphy->base + USB_PHY_UTMI_CTRL5);
	writel(USB2_SUSPEND_N_SEL | USB2_SUSPEND_N | USB2_UTMI_CLK_EN,
		qphy->base + USB_PHY_HS_PHY_CTRL2);
	writel(0x0, qphy->base + USB_PHY_UTMI_CTRL5);
	writel(USB2_SUSPEND_N | USB2_UTMI_CLK_EN,
		qphy->base + USB_PHY_HS_PHY_CTRL2);
	/* Disable override ctrl */
	writel(0x0, qphy->base + USB_PHY_CFG0);
}

static void ipq5332_m31usb_phy_enable_clock(struct m31usb_phy *qphy)
{
	/* Enable override ctrl */
	writel(UTMI_PHY_OVERRIDE_EN, qphy->base + USB_PHY_CFG0);
	/* Enable POR*/
	writel(POR_EN, qphy->base + USB_PHY_UTMI_CTRL5);
	udelay(15);
	/* Configure frequency select value*/
	writel(FREQ_SEL, qphy->base + USB_PHY_FSEL_SEL);
	/* Configure refclk frequency */
	writel(COMMONONN | FREQ_24MHZ | RETENABLEN,
		qphy->base + USB_PHY_HS_PHY_CTRL_COMMON0);
	/* select ATERESET*/
	writel(POR_EN & ATERESET, qphy->base + USB_PHY_UTMI_CTRL5);
	writel(USB2_SUSPEND_N_SEL | USB2_SUSPEND_N | USB2_UTMI_CLK_EN,
		qphy->base + USB_PHY_HS_PHY_CTRL2);
	writel(XCFG_COARSE_TUNE_NUM  | XCFG_FINE_TUNE_NUM,
				qphy->base + USB2PHY_USB_PHY_M31_XCFGI_11);
	/* Adjust HSTX slew rate to 400 ps*/
	/* Adjust PLL lock Time counter for release clock to 35uA */
	/* Adjust Manual control ODT value to 38.02 Ohm */
	writel(HSTX_SLEW_RATE_400PS | PLL_CHARGING_PUMP_CURRENT_35UA |
		ODT_VALUE_38_02_OHM, qphy->base + USB2PHY_USB_PHY_M31_XCFGI_4);
	/*
	* Enable to always turn on USB 2.0 TX driver
	* when system is in USB 2.0 HS mode
	*/
	writel(USB2_0_TX_ENABLE, qphy->base + USB2PHY_USB_PHY_M31_XCFGI_1);

	/* Adjust HSTX Pre-emphasis level to 0.55mA */
	writel(HSTX_PRE_EMPHASIS_LEVEL_0_55MA,
		qphy->base + USB2PHY_USB_PHY_M31_XCFGI_5);
	/*
	* Adjust HSTX Current of current-mode driver,
	* default 18.5mA * 22.5ohm = 416mV
	* 17.1mA * 22.5ohm = 385mV
	*/
	writel(HSTX_CURRENT_17_1MA_385MV,
		qphy->base + USB2PHY_USB_PHY_M31_XCFGI_9);
	udelay(4);
	writel(0x0, qphy->base + USB_PHY_UTMI_CTRL5);
	writel(USB2_SUSPEND_N | USB2_UTMI_CLK_EN,
		qphy->base + USB_PHY_HS_PHY_CTRL2);
}

static int m31usb_phy_init(struct usb_phy *phy)
{
	struct m31usb_phy *qphy = container_of(phy, struct m31usb_phy, phy);

	dev_dbg(phy->dev, "%s\n", __func__);

	/* Perform phy reset */
	m31usb_reset(qphy, CLK_RESET_ASSERT);
	usleep_range(1, 5);
	m31usb_reset(qphy, CLK_RESET_DEASSERT);

	/* configure for ULPI mode if requested */
	if (qphy->ulpi_mode)
		writel_relaxed(0x0, qphy->base + USB2PHY_PORT_UTMI_CTRL2);

	/* Enable the PHY */
	writel_relaxed(POWER_UP, qphy->base + USB2PHY_PORT_POWERDOWN);

	/* Make sure above write completed */
	wmb();

	/* Turn on phy ref clock*/
	if (of_device_is_compatible(phy->dev->of_node,
					"qca,ipq5332-m31-usb-hsphy"))
		ipq5332_m31usb_phy_enable_clock(qphy);
	else
		m31usb_phy_enable_clock(qphy);

	/* Set OTG VBUS Valid from HSPHY to controller */
	m31usb_write_readback(qphy->qscratch_base, HS_PHY_CTRL_REG,
				UTMI_OTG_VBUS_VALID, UTMI_OTG_VBUS_VALID);
	/* Indicate value is driven by UTMI_OTG_VBUS_VALID bit */
	m31usb_write_readback(qphy->qscratch_base, HS_PHY_CTRL_REG,
				SW_SESSVLD_SEL, SW_SESSVLD_SEL);

	return 0;
}

static void m31usb_phy_shutdown(struct usb_phy *phy)
{
	struct m31usb_phy *qphy = container_of(phy, struct m31usb_phy, phy);

	dev_dbg(phy->dev, "%s\n", __func__);

	/* Disable the PHY */
	writel_relaxed(POWER_DOWN, qphy->base + USB2PHY_PORT_POWERDOWN);
	/* Make sure above write completed */
	wmb();
}

static void m31usb_write_readback(void *base, u32 offset,
					const u32 mask, u32 val)
{
	u32 write_val, tmp = readl_relaxed(base + offset);

	tmp &= ~mask; /* retain other bits */
	write_val = tmp | val;

	writel_relaxed(write_val, base + offset);
	/* Make sure above write completed */
	wmb();

	/* Read back to see if val was written */
	tmp = readl_relaxed(base + offset);
	tmp &= mask; /* clear other bits */

	if (tmp != val)
		pr_err("%s: write: %x to QSCRATCH: %x FAILED\n",
			__func__, val, offset);
}

static int m31usb_phy_notify_connect(struct usb_phy *phy,
					enum usb_device_speed speed)
{
	struct m31usb_phy *qphy = container_of(phy, struct m31usb_phy, phy);

	qphy->cable_connected = true;

	dev_dbg(phy->dev, " cable_connected=%d\n", qphy->cable_connected);

	/* Set OTG VBUS Valid from HSPHY to controller */
	m31usb_write_readback(qphy->qscratch_base, HS_PHY_CTRL_REG,
				UTMI_OTG_VBUS_VALID,
				UTMI_OTG_VBUS_VALID);

	/* Indicate value is driven by UTMI_OTG_VBUS_VALID bit */
	m31usb_write_readback(qphy->qscratch_base, HS_PHY_CTRL_REG,
				SW_SESSVLD_SEL, SW_SESSVLD_SEL);

	dev_dbg(phy->dev, "M31USB2 phy connect notification\n");
	return 0;
}

static int m31usb_phy_notify_disconnect(struct usb_phy *phy,
					enum usb_device_speed speed)
{
	struct m31usb_phy *qphy = container_of(phy, struct m31usb_phy, phy);

	qphy->cable_connected = false;

	dev_dbg(phy->dev, " cable_connected=%d\n", qphy->cable_connected);

	/* Set OTG VBUS Valid from HSPHY to controller */
	m31usb_write_readback(qphy->qscratch_base, HS_PHY_CTRL_REG,
				UTMI_OTG_VBUS_VALID, 0);

	/* Indicate value is driven by UTMI_OTG_VBUS_VALID bit */
	m31usb_write_readback(qphy->qscratch_base, HS_PHY_CTRL_REG,
				SW_SESSVLD_SEL, 0);

	dev_dbg(phy->dev, "M31USB2 phy disconnect notification\n");
	return 0;
}

static int m31usb_phy_probe(struct platform_device *pdev)
{
	struct m31usb_phy *qphy;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;
	const char *phy_type;

	qphy = devm_kzalloc(dev, sizeof(*qphy), GFP_KERNEL);
	if (!qphy)
		return -ENOMEM;

	qphy->phy.dev = dev;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"m31usb_phy_base");
	qphy->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(qphy->base))
		return PTR_ERR(qphy->base);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"qscratch_base");
	if (res) {
		qphy->qscratch_base = devm_ioremap(dev, res->start,
							resource_size(res));
		if (IS_ERR(qphy->qscratch_base)) {
			dev_dbg(dev, "couldn't ioremap qscratch_base\n");
			qphy->qscratch_base = NULL;
		}
	}

	qphy->phy_reset = devm_reset_control_get(dev, "usb2_phy_reset");
	if (IS_ERR(qphy->phy_reset))
		return PTR_ERR(qphy->phy_reset);

	qphy->ulpi_mode = false;
	ret = of_property_read_string(dev->of_node, "phy_type", &phy_type);

	if (!ret) {
		if (!strcasecmp(phy_type, "ulpi"))
			qphy->ulpi_mode = true;
	} else {
		dev_err(dev, "error reading phy_type property\n");
		return ret;
	}

	platform_set_drvdata(pdev, qphy);

	qphy->phy.label			= "qca-m31usb-phy";
	qphy->phy.init			= m31usb_phy_init;
	qphy->phy.shutdown		= m31usb_phy_shutdown;
	qphy->phy.type			= USB_PHY_TYPE_USB2;

	if (qphy->qscratch_base) {
		qphy->phy.notify_connect        = m31usb_phy_notify_connect;
		qphy->phy.notify_disconnect     = m31usb_phy_notify_disconnect;
	}

	ret = usb_add_phy_dev(&qphy->phy);
	return ret;
}

static int m31usb_phy_remove(struct platform_device *pdev)
{
	struct m31usb_phy *qphy = platform_get_drvdata(pdev);

	usb_remove_phy(&qphy->phy);

	return 0;
}

static const struct of_device_id m31usb_phy_id_table[] = {
	{ .compatible = "qca,m31-usb-hsphy",},
	{ .compatible = "qca,ipq5332-m31-usb-hsphy",},
	{ },
};
MODULE_DEVICE_TABLE(of, m31usb_phy_id_table);

static struct platform_driver m31usb_phy_driver = {
	.probe		= m31usb_phy_probe,
	.remove		= m31usb_phy_remove,
	.driver = {
		.name	= "qca-m31usb-phy",
		.of_match_table = of_match_ptr(m31usb_phy_id_table),
	},
};

module_platform_driver(m31usb_phy_driver);

MODULE_DESCRIPTION("USB2 QTI M31 HSPHY driver");
MODULE_LICENSE("GPL v2");
