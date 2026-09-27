/*
 * Copyright (c) 2016-2017, 2019-2020, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/of_mdio.h>
#include <linux/reset.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/bitfield.h>
#include <soc/qcom/socinfo.h>

#define MDIO_CTRL_0_REG				0x40
#define MDIO_CTRL_1_REG				0x44
#define MDIO_CTRL_2_REG				0x48
#define MDIO_CTRL_3_REG				0x4c
#define MDIO_CTRL_4_REG				0x50
#define MDIO_CTRL_4_ACCESS_BUSY			BIT(16)
#define MDIO_CTRL_4_ACCESS_START		BIT(8)
#define MDIO_CTRL_4_ACCESS_CODE_READ		0
#define MDIO_CTRL_4_ACCESS_CODE_WRITE		1
#define MDIO_CTRL_4_ACCESS_CODE_C45_ADDR	0
#define MDIO_CTRL_4_ACCESS_CODE_C45_WRITE	1
#define MDIO_CTRL_4_ACCESS_CODE_C45_READ	2
#define CTRL_0_REG_DEFAULT_VALUE(div)		(0x15000 | (div & 0xff))
#define CTRL_0_REG_C45_DEFAULT_VALUE(div)	(0x15100 | (div & 0xff))

#define QCA_MDIO_RETRY				1000
#define QCA_MDIO_DELAY				10

#define QCA_MDIO_CLK_RATE			100000000
#define UNIPHY_AHB_CLK_RATE			100000000
#define UNIPHY_SYS_CLK_RATE			24000000

#define TCSR_LDO_ADDR				0x19475C4
#define GCC_GEPHY_ADDR				0x1856004
#define REG_SIZE				4

#define PHY_CLK_REG_ADDR			0x7a00610
#define PHY_CLK_REG_SIZE			0x20000

/* macro for mht chipset start */
#define EPHY_CFG				0xC90F018
#define GEPHY0_TX_CBCR				0xC800058
#define SRDS0_SYS_CBCR				0xC8001A8
#define SRDS1_SYS_CBCR				0xC8001AC
#define EPHY0_SYS_CBCR				0xC8001B0
#define EPHY1_SYS_CBCR				0xC8001B4
#define EPHY2_SYS_CBCR				0xC8001B8
#define EPHY3_SYS_CBCR				0xC8001BC
#define GCC_GEPHY_MISC				0xC800304
#define QFPROM_RAW_PTE_ROW2_MSB			0xC900014
#define QFPROM_RAW_CALIBRATION_ROW4_LSB 	0xC900048
#define QFPROM_RAW_CALIBRATION_ROW7_LSB 	0xC900060
#define QFPROM_RAW_CALIBRATION_ROW8_LSB 	0xC900068
#define QFPROM_RAW_CALIBRATION_ROW6_MSB 	0xC90005C
#define PHY_DEBUG_PORT_ADDR			0x1d
#define PHY_DEBUG_PORT_DATA			0x1e
#define PHY_LDO_EFUSE_REG			0x180
#define PHY_ICC_EFUSE_REG			0x280
#define PHY_10BT_SG_THRESH_REG			0x3380
#define PHY_MMD1_CTRL2ANA_OPTION2_REG		0x40018102
#define PHY_ADDR_LENGTH				5
#define PHY_ADDR_NUM				4
#define UNIPHY_ADDR_NUM				3
#define MII_HIGH_ADDR_PREFIX			0x18
#define MII_LOW_ADDR_PREFIX			0x10
#define SWITCH_REG_TYPE_MASK			GENMASK(31, 28)
#define SWITCH_REG_TYPE_QCA8386			0
#define SWITCH_REG_TYPE_QCA8337			1
#define SWITCH_HIGH_ADDR_DFLT			0x200

#define CMN_PLL_REFERENCE_CLOCK			0x784
#define CMN_PLL_REFCLK_INDEX			GENMASK(3, 0)
#define CMN_PLL_REFCLK_EXTERNAL			BIT(9)

#define CMN_PLL_POWER_ON_AND_RESET		0x780
#define CMN_ANA_EN_SW_RSTN			BIT(6)

#define CMN_PLL_OUTPUT_RELATED_1		0x79c
#define CMN_PLL_CLK25M_EN			BIT(15)
#define CMN_PLL_CMN_PLL_CLK50M_62P5M_EN		BIT(11)
#define CMN_PLL_CMN_PLL_CLK50M_62P5M_EN1	BIT(10)
#define CMN_PLL_CMN_PLL_CLK50M_62P5M_EN2	BIT(14)

struct qca_mdio_data {
	struct mii_bus *mii_bus;
	struct clk *mdio_clk;
	void __iomem *membase;
	int phy_irq[PHY_MAX_ADDR];
	int clk_div;
	bool force_c22;
	void (*preinit)(struct mii_bus *bus);
	u32 (*sw_read)(struct mii_bus *bus, u32 reg);
	void (*sw_write)(struct mii_bus *bus, u32 reg, u32 val);
};

static int qca_mdio_wait_busy(struct qca_mdio_data *am)
{
	int i;

	for (i = 0; i < QCA_MDIO_RETRY; i++) {
		unsigned int busy;

		busy = readl(am->membase + MDIO_CTRL_4_REG);
		busy &= MDIO_CTRL_4_ACCESS_BUSY;
		if (!busy)
			return 0;

		udelay(QCA_MDIO_DELAY);
	}

	pr_err("%s: MDIO operation timed out\n", am->mii_bus->name);

	return -ETIMEDOUT;
}

static int _qca_mdio_read(struct mii_bus *bus, int mii_id, int regnum)
{
	struct qca_mdio_data *am = bus->priv;
	int value = 0;
	unsigned int cmd = 0;

	if (qca_mdio_wait_busy(am))
		return 0xffff;

	if (regnum & MII_ADDR_C45) {
		unsigned int mmd = (regnum >> 16) & 0x1F;
		unsigned int reg = regnum & 0xFFFF;

		writel(CTRL_0_REG_C45_DEFAULT_VALUE(am->clk_div),
		       am->membase + MDIO_CTRL_0_REG);
		/* issue the phy address and mmd */
		writel((mii_id << 8) | mmd, am->membase + MDIO_CTRL_1_REG);
		/* issue reg */
		writel(reg, am->membase + MDIO_CTRL_2_REG);
		cmd = MDIO_CTRL_4_ACCESS_START |
			MDIO_CTRL_4_ACCESS_CODE_C45_ADDR;
	} else {
		writel(CTRL_0_REG_DEFAULT_VALUE(am->clk_div), am->membase + MDIO_CTRL_0_REG);
		/* issue the phy address and reg */
		writel((mii_id << 8) | regnum, am->membase + MDIO_CTRL_1_REG);
		cmd = MDIO_CTRL_4_ACCESS_START | MDIO_CTRL_4_ACCESS_CODE_READ;
	}

	/* issue command */
	writel(cmd, am->membase + MDIO_CTRL_4_REG);

	/* Wait complete */
	if (qca_mdio_wait_busy(am))
		return 0xffff;

	if (regnum & MII_ADDR_C45) {
		cmd = MDIO_CTRL_4_ACCESS_START |
			MDIO_CTRL_4_ACCESS_CODE_C45_READ;
		writel(cmd, am->membase + MDIO_CTRL_4_REG);
		if (qca_mdio_wait_busy(am))
			return 0xffff;
	}

	/* Read data */
	value = readl(am->membase + MDIO_CTRL_3_REG);

	return value;
}

static int _qca_mdio_write(struct mii_bus *bus, int mii_id, int regnum,
			  u16 value)
{
	struct qca_mdio_data *am = bus->priv;
	unsigned int cmd = 0;

	if (qca_mdio_wait_busy(am))
		return 0xffff;

	if (regnum & MII_ADDR_C45) {
		unsigned int mmd = (regnum >> 16) & 0x1F;
		unsigned int reg = regnum & 0xFFFF;

		writel(CTRL_0_REG_C45_DEFAULT_VALUE(am->clk_div),
		       am->membase + MDIO_CTRL_0_REG);
		/* issue the phy address and mmd */
		writel((mii_id << 8) | mmd, am->membase + MDIO_CTRL_1_REG);
		/* issue reg */
		writel(reg, am->membase + MDIO_CTRL_2_REG);
		cmd = MDIO_CTRL_4_ACCESS_START |
			MDIO_CTRL_4_ACCESS_CODE_C45_ADDR;
		writel(cmd, am->membase + MDIO_CTRL_4_REG);
		if (qca_mdio_wait_busy(am))
			return -ETIMEDOUT;
	} else {
		writel(CTRL_0_REG_DEFAULT_VALUE(am->clk_div), am->membase + MDIO_CTRL_0_REG);
		/* issue the phy address and reg */
		writel((mii_id << 8) | regnum, am->membase + MDIO_CTRL_1_REG);
	}

	/* issue write data */
	writel(value, am->membase + MDIO_CTRL_2_REG);

	/* issue write command */
	if (regnum & MII_ADDR_C45)
		cmd = MDIO_CTRL_4_ACCESS_START |
			MDIO_CTRL_4_ACCESS_CODE_C45_WRITE;
	else
		cmd = MDIO_CTRL_4_ACCESS_START | MDIO_CTRL_4_ACCESS_CODE_WRITE;
	writel(cmd, am->membase + MDIO_CTRL_4_REG);

	/* Wait write complete */
	if (qca_mdio_wait_busy(am))
		return -ETIMEDOUT;

	return 0;
}

static int qca_mdio_read(struct mii_bus *bus, int mii_id, int regnum)
{
	struct qca_mdio_data *priv = bus->priv;

	if (priv && priv->force_c22 && (regnum & MII_ADDR_C45)) {
		unsigned int mmd = (regnum >> 16) & 0x1F;
		unsigned int reg = regnum & 0xFFFF;

		_qca_mdio_write(bus, mii_id, MII_MMD_CTRL, mmd);
		_qca_mdio_write(bus, mii_id, MII_MMD_DATA, reg);
		_qca_mdio_write(bus, mii_id, MII_MMD_CTRL, mmd | MII_MMD_CTRL_NOINCR);

		return  _qca_mdio_read(bus, mii_id, MII_MMD_DATA);
	}

	return _qca_mdio_read(bus, mii_id, regnum);
}

static int qca_mdio_write(struct mii_bus *bus, int mii_id, int regnum,
			  u16 value)
{
	struct qca_mdio_data *priv = bus->priv;

	if (priv && priv->force_c22 && (regnum & MII_ADDR_C45)) {
		unsigned int mmd = (regnum >> 16) & 0x1F;
		unsigned int reg = regnum & 0xFFFF;

		_qca_mdio_write(bus, mii_id, MII_MMD_CTRL, mmd);
		_qca_mdio_write(bus, mii_id, MII_MMD_DATA, reg);
		_qca_mdio_write(bus, mii_id, MII_MMD_CTRL, mmd | MII_MMD_CTRL_NOINCR);

		return _qca_mdio_write(bus, mii_id, MII_MMD_DATA, value);
	}

	return _qca_mdio_write(bus, mii_id, regnum, value);
}

static int qca_phy_gpio_set(struct platform_device *pdev, int number)
{
	int ret;

	ret = gpio_request(number, "phy-reset-gpio");
	if (ret) {
		dev_err(&pdev->dev, "Can't get phy-reset-gpio %d\n", ret);
		return ret;
	}

	ret = gpio_direction_output(number, 0x0);
	if (ret) {
		dev_err(&pdev->dev,
			"Can't set direction for phy-reset-gpio %d\n", ret);
		goto phy_reset_out;
	}

	usleep_range(100000, 110000);

	gpio_set_value(number, 0x01);

	usleep_range(100000, 110000);

phy_reset_out:
	gpio_free(number);

	return ret;
}

int qca_phy_reset(struct platform_device *pdev)
{
	struct device_node *mdio_node = pdev->dev.of_node;
	int phy_reset_gpio_number;
	int ret, i, gpio_num;

	gpio_num = of_gpio_named_count(mdio_node, "phy-reset-gpio");
	if (gpio_num == 0 || gpio_num == -ENOENT)
		return 0;
	else if (gpio_num < 0)
		return gpio_num;

	for (i = 0; i < gpio_num; i++) {
		ret = of_get_named_gpio(mdio_node, "phy-reset-gpio", i);
		if (ret < 0) {
			dev_err(&pdev->dev, "Could not find phy-reset-gpio\n");
			return 0;
		}

		phy_reset_gpio_number = ret;

		dev_info(&pdev->dev, "Reset PHY by GPIO %d\n", phy_reset_gpio_number);
		ret = qca_phy_gpio_set(pdev, phy_reset_gpio_number);
		if (ret)
			return ret;
	}

	return 0;
}

static void qca_tcsr_ldo_rdy_set(bool ready)
{
	void __iomem *tcsr_base = NULL;
	u32 val;

	tcsr_base = ioremap_nocache(TCSR_LDO_ADDR, REG_SIZE);
	if (!tcsr_base)
		return;

	val = readl(tcsr_base);
	if (ready)
		val |= 1;
	else
		val &= ~1;
	writel(val, tcsr_base);
	usleep_range(100000, 110000);

	iounmap(tcsr_base);
}

static inline void qca8337_split_addr(u32 regaddr, u16 *r1, u16 *r2, u16 *page)
{
	regaddr >>= 1;
	*r1 = regaddr & 0x1e;

	regaddr >>= 5;
	*r2 = regaddr & 0x7;

	regaddr >>= 3;
	*page = regaddr & 0x3ff;
}

u32 qca8337_read(struct mii_bus *mii_bus, u32 reg)
{
	u16 r1, r2, page;
	u16 lo, hi;

	qca8337_split_addr(reg, &r1, &r2, &page);
	mii_bus->write(mii_bus, MII_HIGH_ADDR_PREFIX, 0, page);
	udelay(100);

	lo = mii_bus->read(mii_bus, MII_LOW_ADDR_PREFIX | r2, r1);
	hi = mii_bus->read(mii_bus, MII_LOW_ADDR_PREFIX | r2, r1 + 1);

	mii_bus->write(mii_bus, MII_HIGH_ADDR_PREFIX, 0, SWITCH_HIGH_ADDR_DFLT);
	return (hi << 16) | lo;
}

void qca8337_write(struct mii_bus *mii_bus, u32 reg, u32 val)
{
	u16 r1, r2, page;

	qca8337_split_addr(reg, &r1, &r2, &page);
	mii_bus->write(mii_bus, MII_HIGH_ADDR_PREFIX, 0, page);
	udelay(100);

	mii_bus->write(mii_bus, MII_LOW_ADDR_PREFIX | r2, r1, val & 0xffff);
	mii_bus->write(mii_bus, MII_LOW_ADDR_PREFIX | r2, r1 + 1, (u16)(val >> 16));

	mii_bus->write(mii_bus, MII_HIGH_ADDR_PREFIX , 0, SWITCH_HIGH_ADDR_DFLT);
}


static inline void split_addr(u32 regaddr, u16 *r1, u16 *r2, u16 *page, u16 *switch_phy_id)
{
	*r1 = regaddr & 0x1c;

	regaddr >>= 5;
	*r2 = regaddr & 0x7;

	regaddr >>= 3;
	*page = regaddr & 0xffff;

	regaddr >>= 16;
	*switch_phy_id = regaddr & 0xff;
}

u32 qca8386_read(struct mii_bus *mii_bus, u32 reg)
{
	u16 r1, r2, page, switch_phy_id;
	int lo, hi;

	split_addr(reg, &r1, &r2, &page, &switch_phy_id);

	mii_bus->write(mii_bus, MII_HIGH_ADDR_PREFIX | (switch_phy_id >> 5),
			switch_phy_id & 0x1f, page);
	udelay(100);
	lo = mii_bus->read(mii_bus, MII_LOW_ADDR_PREFIX | r2, r1);
	hi = mii_bus->read(mii_bus, MII_LOW_ADDR_PREFIX | r2, r1 | BIT(1));

	return (hi << 16) | lo;
}

void qca8386_write(struct mii_bus *mii_bus, u32 reg, u32 val)
{
	u16 r1, r2, page, switch_phy_id;
	u16 lo, hi;

	split_addr(reg, &r1, &r2, &page, &switch_phy_id);
	lo = val & 0xffff;
	hi = val >> 16;

	mii_bus->write(mii_bus, MII_HIGH_ADDR_PREFIX | (switch_phy_id >> 5),
			switch_phy_id & 0x1f, page);
	udelay(100);
	mii_bus->write(mii_bus, MII_LOW_ADDR_PREFIX | r2, r1, lo);
	mii_bus->write(mii_bus, MII_LOW_ADDR_PREFIX | r2, r1 | BIT(1), hi);
}

u32 qca_mii_read(struct mii_bus *mii_bus, u32 reg)
{
	u32 val = 0xffffffff;
	switch (FIELD_GET(SWITCH_REG_TYPE_MASK, reg)) {
		case SWITCH_REG_TYPE_QCA8337:
			val = qca8337_read(mii_bus, reg);
			break;
		case SWITCH_REG_TYPE_QCA8386:
		default:
			val = qca8386_read(mii_bus, reg);
			break;
	}

	return val;
}
EXPORT_SYMBOL_GPL(qca_mii_read);

void qca_mii_write(struct mii_bus *mii_bus, u32 reg, u32 val)
{
	switch (FIELD_GET(SWITCH_REG_TYPE_MASK, reg)) {
		case SWITCH_REG_TYPE_QCA8337:
			qca8337_write(mii_bus, reg, val);
			break;
		case SWITCH_REG_TYPE_QCA8386:
		default:
			qca8386_write(mii_bus, reg, val);
			break;
	}
}
EXPORT_SYMBOL_GPL(qca_mii_write);

static inline void qca_mht_clk_enable(struct mii_bus *mii_bus, u32 reg)
{
	u32 val;

	val = qca_mii_read(mii_bus, reg);
	val |= BIT(0);
	qca_mii_write(mii_bus, reg, val);
}

static inline void qca_mht_clk_disable(struct mii_bus *mii_bus, u32 reg)
{
	u32 val;

	val = qca_mii_read(mii_bus, reg);
	val &= ~BIT(0);
	qca_mii_write(mii_bus, reg, val);
}

static inline void qca_mht_clk_reset(struct mii_bus *mii_bus, u32 reg)
{
	u32 val;

	val = qca_mii_read(mii_bus, reg);
	val |= BIT(2);
	qca_mii_write(mii_bus, reg, val);

	usleep_range(20000, 21000);

	val &= ~BIT(2);
	qca_mii_write(mii_bus, reg, val);
}

static u16
qca_phy_debug_read(struct mii_bus *mii_bus, u32 phy_addr, u32 reg_id)
{
	mii_bus->write(mii_bus, phy_addr, PHY_DEBUG_PORT_ADDR, reg_id);

	return mii_bus->read(mii_bus, phy_addr, PHY_DEBUG_PORT_DATA);
}

static void
qca_phy_debug_write(struct mii_bus *mii_bus, u32 phy_addr, u32 reg_id, u16 reg_val)
{

	mii_bus->write(mii_bus, phy_addr, PHY_DEBUG_PORT_ADDR, reg_id);

	mii_bus->write(mii_bus, phy_addr, PHY_DEBUG_PORT_DATA, reg_val);
}

static void
qca_mht_efuse_loading(struct mii_bus *mii_bus, u8 ethphy)
{
	u32 val = 0, ldo_efuse = 0, icc_efuse = 0, phy_addr = 0;
	u16 reg_val = 0;

	phy_addr = qca_mii_read(mii_bus, EPHY_CFG) >> (ethphy * PHY_ADDR_LENGTH)
		& GENMASK(4, 0);
	switch(ethphy) {
		case 0:
			val = qca_mii_read(mii_bus, QFPROM_RAW_CALIBRATION_ROW4_LSB);
			ldo_efuse = (val & GENMASK(21, 18)) >> 18;
			icc_efuse = (val & GENMASK(26, 22)) >> 22;
			break;
		case 1:
			val = qca_mii_read(mii_bus, QFPROM_RAW_CALIBRATION_ROW7_LSB);
			ldo_efuse = (val & GENMASK(26, 23)) >> 23;
			icc_efuse = (val & GENMASK(31, 27)) >> 27;
			break;
		case 2:
			val = qca_mii_read(mii_bus, QFPROM_RAW_CALIBRATION_ROW8_LSB);
			ldo_efuse = (val & GENMASK(26, 23)) >> 23;
			icc_efuse = (val & GENMASK(31, 27)) >> 27;
			break;
		case 3:
			val = qca_mii_read(mii_bus, QFPROM_RAW_CALIBRATION_ROW6_MSB);
			ldo_efuse = (val & GENMASK(17, 14)) >> 14;
			icc_efuse = (val & GENMASK(22, 18)) >> 18;
			break;
	}
	reg_val = qca_phy_debug_read(mii_bus, phy_addr, PHY_LDO_EFUSE_REG);
	reg_val = (reg_val & ~GENMASK(7, 4)) | ldo_efuse << 4;
	qca_phy_debug_write(mii_bus, phy_addr, PHY_LDO_EFUSE_REG, reg_val);

	reg_val = qca_phy_debug_read(mii_bus, phy_addr, PHY_ICC_EFUSE_REG);
	reg_val = (reg_val & ~GENMASK(4, 0)) | icc_efuse;
	qca_phy_debug_write(mii_bus, phy_addr, PHY_ICC_EFUSE_REG, reg_val);
}

static void
qca_mht_ethphy_ana_fixup(struct mii_bus *mii_bus, u8 ethphy)
{
	u32 phy_addr = 0;
	u16 reg_val = 0;

	phy_addr = qca_mii_read(mii_bus, EPHY_CFG) >> (ethphy * PHY_ADDR_LENGTH)
		& GENMASK(4, 0);
	/*increase 100BT tx amplitude*/
	reg_val = mii_bus->read(mii_bus, phy_addr, PHY_MMD1_CTRL2ANA_OPTION2_REG);
	mii_bus->write(mii_bus, phy_addr, PHY_MMD1_CTRL2ANA_OPTION2_REG,
		reg_val | 0x7f);
	/*increase 10BT signal detect threshold*/
	reg_val = qca_phy_debug_read(mii_bus, phy_addr, PHY_10BT_SG_THRESH_REG);
	qca_phy_debug_write(mii_bus, phy_addr, PHY_10BT_SG_THRESH_REG, reg_val | 0x1);
}

static void qca_mht_clock_init(struct mii_bus *mii_bus)
{
	u32 val = 0;
	int i;

	/* Enable serdes */
	qca_mht_clk_enable(mii_bus, SRDS0_SYS_CBCR);
	qca_mht_clk_enable(mii_bus, SRDS1_SYS_CBCR);

	/* Reset serdes */
	qca_mht_clk_reset(mii_bus, SRDS0_SYS_CBCR);
	qca_mht_clk_reset(mii_bus, SRDS1_SYS_CBCR);

	/* Disable EPHY GMII clock */
	i = 0;
	while (i < 2 * PHY_ADDR_NUM) {
		qca_mht_clk_disable(mii_bus, GEPHY0_TX_CBCR + i*0x20);
		i++;
	}

	/* Enable ephy */
	qca_mht_clk_enable(mii_bus, EPHY0_SYS_CBCR);
	qca_mht_clk_enable(mii_bus, EPHY1_SYS_CBCR);
	qca_mht_clk_enable(mii_bus, EPHY2_SYS_CBCR);
	qca_mht_clk_enable(mii_bus, EPHY3_SYS_CBCR);

	/* Reset ephy */
	qca_mht_clk_reset(mii_bus, EPHY0_SYS_CBCR);
	qca_mht_clk_reset(mii_bus, EPHY1_SYS_CBCR);
	qca_mht_clk_reset(mii_bus, EPHY2_SYS_CBCR);
	qca_mht_clk_reset(mii_bus, EPHY3_SYS_CBCR);

	/* Deassert EPHY DSP */
	val = qca_mii_read(mii_bus, GCC_GEPHY_MISC);
	val &= ~GENMASK(4, 0);
	qca_mii_write(mii_bus, GCC_GEPHY_MISC, val);
	/*for ES chips, need to load efuse manually*/
	val = qca_mii_read(mii_bus, QFPROM_RAW_PTE_ROW2_MSB);
	val = (val & GENMASK(23, 16)) >> 16;
	if(val == 1 || val == 2) {
		for(i = 0; i < 4; i++)
			qca_mht_efuse_loading(mii_bus, i);
	}
	/*fix 100BT template issue and 10BT threshold issue*/
	for(i = 0; i < 4; i++)
		qca_mht_ethphy_ana_fixup(mii_bus, i);
	/* Enable efuse loading into analog circuit */
	val = qca_mii_read(mii_bus, EPHY_CFG);
	/* BIT20 for PHY0 and PHY1, BIT21 for PHY2 and PHY3 */
	val &= ~GENMASK(21, 20);
	qca_mii_write(mii_bus, EPHY_CFG, val);

	/* Sleep 10ms */
	usleep_range(10000, 11000);
}

static void qca_phy_addr_fixup(struct mii_bus *mii_bus, struct device_node *np)
{
	void __iomem *ephy_cfg_base;
	struct device_node *child;
	int phy_index, addr, len;
	const __be32 *phy_cfg, *uniphy_cfg;
	u32 val;
	bool mdio_access = false;
	unsigned long phyaddr_mask = 0;

	phy_cfg = of_get_property(np, "phyaddr_fixup", &len);
	uniphy_cfg = of_get_property(np, "uniphyaddr_fixup", NULL);

	/*
	 * For MDIO access, phyaddr_fixup only provides the register address,
	 * as for local bus, the register length also needs to be provided
	 */
	if(!phy_cfg || (len != (2 * sizeof(__be32)) && len != sizeof(__be32)))
		return;

	if (len == sizeof(__be32))
		mdio_access = true;

	if (!mdio_access) {
		ephy_cfg_base = ioremap_nocache(be32_to_cpup(phy_cfg), be32_to_cpup(phy_cfg + 1));
		if (!ephy_cfg_base)
			return;
		val = readl(ephy_cfg_base);
	} else
		val = qca_mii_read(mii_bus, be32_to_cpup(phy_cfg));

	phy_index = 0;
	addr = 0;
	for_each_available_child_of_node(np, child) {
		if (phy_index >= PHY_ADDR_NUM)
			break;

		addr = of_mdio_parse_addr(&mii_bus->dev, child);
		if (addr < 0) {
			continue;
		}
		phyaddr_mask |= BIT(addr);

		if (!of_find_property(child, "fixup", NULL))
			continue;

		addr &= GENMASK(4, 0);
		val &= ~(GENMASK(4, 0) << (phy_index * PHY_ADDR_LENGTH));
		val |= addr << (phy_index * PHY_ADDR_LENGTH);
		phy_index++;
	}

	/* Programe the PHY address */
	dev_info(mii_bus->parent, "programe EPHY reg 0x%x with 0x%x\n",
			be32_to_cpup(phy_cfg), val);

	if (!mdio_access) {
		writel(val, ephy_cfg_base);
		iounmap(ephy_cfg_base);
	} else {
		qca_mii_write(mii_bus, be32_to_cpup(phy_cfg), val);

		/* Programe the UNIPHY address if uniphyaddr_fixup specified.
		 * the UNIPHY address will select three MDIO address from
		 * unoccupied MDIO address space. */
		if (uniphy_cfg) {
			val = qca_mii_read(mii_bus, be32_to_cpup(uniphy_cfg));

			/* For qca8386, the switch occupies the other 16 MDIO address,
			 * for example, if the phy address is in the range of 0 to 15,
			 * the switch will occupy the MDIO address from 16 to 31. */
			if (addr > 15)
				phyaddr_mask |= GENMASK(15, 0);
			else
				phyaddr_mask |= GENMASK(31, 16);

			phy_index = 0;
			for_each_clear_bit_from(addr, &phyaddr_mask, PHY_MAX_ADDR) {
				if (phy_index >= UNIPHY_ADDR_NUM)
					break;
				val &= ~(GENMASK(4, 0) << (phy_index * PHY_ADDR_LENGTH));
				val |= addr << (phy_index * PHY_ADDR_LENGTH);
				phy_index++;
			}

			if (phy_index < UNIPHY_ADDR_NUM) {
				for_each_clear_bit(addr, &phyaddr_mask, PHY_MAX_ADDR) {
					if (phy_index >= UNIPHY_ADDR_NUM)
						break;
					val &= ~(GENMASK(4, 0) << (phy_index * PHY_ADDR_LENGTH));
					val |= addr << (phy_index * PHY_ADDR_LENGTH);
					phy_index++;
				}
			}

			dev_info(mii_bus->parent, "programe UNIPHY reg 0x%x with 0x%x\n",
					be32_to_cpup(uniphy_cfg), val);

			qca_mii_write(mii_bus, be32_to_cpup(uniphy_cfg), val);
		}
	}
}

static void qca_phy_clock_enable(void)
{
	void __iomem *base = NULL;
	u32 val;

	base = ioremap_nocache(PHY_CLK_REG_ADDR, PHY_CLK_REG_SIZE);
	if (!base)
		return;

	val = readl(base);
	val |= BIT(0);
	writel(val, base);
	usleep_range(100000, 110000);

	if (cpu_is_uniphy1_enabled()) {
		val = readl(base+0x10000);
		val |= BIT(0);
		writel(val, base+0x10000);
		usleep_range(100000, 110000);
	}

	iounmap(base);
}

static int qca_mdio_clock_set_and_enable(struct device *dev,
		const char *clk_id, unsigned long rate)
{
	struct clk *clk;

	clk = devm_clk_get(dev, clk_id);
	if (IS_ERR(clk))
		return -1;

	if (rate && clk_set_rate(clk, rate))
		return -1;

	return clk_prepare_enable(clk);
}

void qca_mht_preinit(struct mii_bus *mii_bus)
{
	struct device_node *np = mii_bus->parent->of_node;

	if (!mii_bus || !np)
		return;

	qca_phy_addr_fixup(mii_bus, np);

	if (of_property_read_bool(np, "mdio_clk_fixup"))
		qca_mht_clock_init(mii_bus);

	return;
}

void qca_cmn_clk_config(struct platform_device *pdev)
{
	struct resource *res;
	void __iomem *cmn_clk_base;
	u32 reg_val;
	const char *cmn_ref_clk;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res) {
		struct device_node *child;
		u32 clk_en;

		cmn_clk_base = devm_ioremap_resource(&pdev->dev, res);
		if (!IS_ERR(cmn_clk_base)) {
			/* Select reference clock source */
			reg_val = readl(cmn_clk_base + CMN_PLL_REFERENCE_CLOCK);
			reg_val &= ~(CMN_PLL_REFCLK_EXTERNAL | CMN_PLL_REFCLK_INDEX);

			cmn_ref_clk = of_get_property(pdev->dev.of_node, "cmn_clk", NULL);
			if (!cmn_ref_clk) {
				/* Internal 48MHZ selected by default */
				reg_val |= FIELD_PREP(CMN_PLL_REFCLK_INDEX, 7);
			} else {
				if (!strcmp(cmn_ref_clk, "external_25MHz"))
					reg_val |= (CMN_PLL_REFCLK_EXTERNAL |
						FIELD_PREP(CMN_PLL_REFCLK_INDEX, 3));
				else if (!strcmp(cmn_ref_clk, "external_31250KHz"))
					reg_val |= (CMN_PLL_REFCLK_EXTERNAL |
						FIELD_PREP(CMN_PLL_REFCLK_INDEX, 4));
				else if (!strcmp(cmn_ref_clk, "external_40MHz"))
					reg_val |= (CMN_PLL_REFCLK_EXTERNAL |
						FIELD_PREP(CMN_PLL_REFCLK_INDEX, 6));
				else if (!strcmp(cmn_ref_clk, "external_48MHz"))
					reg_val |= (CMN_PLL_REFCLK_EXTERNAL |
						FIELD_PREP(CMN_PLL_REFCLK_INDEX, 7));
				else if (!strcmp(cmn_ref_clk, "external_50MHz"))
					reg_val |= (CMN_PLL_REFCLK_EXTERNAL |
						FIELD_PREP(CMN_PLL_REFCLK_INDEX, 8));
				else
					reg_val |= FIELD_PREP(CMN_PLL_REFCLK_INDEX, 7);
			}

			writel(reg_val, cmn_clk_base + CMN_PLL_REFERENCE_CLOCK);

			/* Do the cmn clock reset */
			reg_val = readl(cmn_clk_base + CMN_PLL_POWER_ON_AND_RESET);
			reg_val &= ~CMN_ANA_EN_SW_RSTN;
			writel(reg_val, cmn_clk_base + CMN_PLL_POWER_ON_AND_RESET);
			msleep(1);

			reg_val |= CMN_ANA_EN_SW_RSTN;
			writel(reg_val, cmn_clk_base + CMN_PLL_POWER_ON_AND_RESET);
			msleep(1);

			dev_info(&pdev->dev, "CMN clock reset done\n");

			clk_en = 0;
			for_each_available_child_of_node(pdev->dev.of_node, child) {
				if (of_find_property(child, "ref_clk_25m", NULL))
					clk_en |= CMN_PLL_CLK25M_EN;
				else if (of_find_property(child, "ref_clk_50m", NULL))
					clk_en |= CMN_PLL_CMN_PLL_CLK50M_62P5M_EN;
				else if (of_find_property(child, "ref_clk_50m_1", NULL))
					clk_en |= CMN_PLL_CMN_PLL_CLK50M_62P5M_EN1;
				else if (of_find_property(child, "ref_clk_50m_2", NULL))
					clk_en |= CMN_PLL_CMN_PLL_CLK50M_62P5M_EN2;
			}

			if (clk_en) {
				reg_val = readl(cmn_clk_base + CMN_PLL_OUTPUT_RELATED_1);
				reg_val &= ~(CMN_PLL_CLK25M_EN | CMN_PLL_CMN_PLL_CLK50M_62P5M_EN |
						CMN_PLL_CMN_PLL_CLK50M_62P5M_EN1 |
						CMN_PLL_CMN_PLL_CLK50M_62P5M_EN2);

				reg_val |= clk_en;
				writel(reg_val, cmn_clk_base + CMN_PLL_OUTPUT_RELATED_1);

				dev_info(&pdev->dev, "CMN output clock select %x\n", clk_en);
			}

			devm_iounmap(&pdev->dev, cmn_clk_base);
		}
	}
}

static int qca_mdio_probe(struct platform_device *pdev)
{
	struct qca_mdio_data *am;
	int ret, i;
	struct reset_control *rst = ERR_PTR(-EINVAL);

	qca_cmn_clk_config(pdev);

	if (of_machine_is_compatible("qcom,ipq5018")) {
		qca_tcsr_ldo_rdy_set(true);
		rst = of_reset_control_get(pdev->dev.of_node, "gephy_mdc_rst");
		if (!IS_ERR(rst)) {
			reset_control_deassert(rst);
			usleep_range(100000, 110000);
		}
	}

	if (of_machine_is_compatible("qcom,ipq5332")) {
		if (cpu_is_uniphy1_enabled()) {
			qca_mdio_clock_set_and_enable(&pdev->dev,
					"uniphy1_ahb_clk", UNIPHY_AHB_CLK_RATE);
			qca_mdio_clock_set_and_enable(&pdev->dev,
					"uniphy1_sys_clk", UNIPHY_SYS_CLK_RATE);
		}

		qca_mdio_clock_set_and_enable(&pdev->dev,
				"uniphy0_ahb_clk", UNIPHY_AHB_CLK_RATE);
		qca_mdio_clock_set_and_enable(&pdev->dev,
				"uniphy0_sys_clk", UNIPHY_SYS_CLK_RATE);
		qca_phy_clock_enable();
	}

	ret = qca_phy_reset(pdev);
	if (ret)
		dev_err(&pdev->dev, "Could not find reset gpio\n");

	am = devm_kzalloc(&pdev->dev, sizeof(*am), GFP_KERNEL);
	if (!am)
		return -ENOMEM;

	am->mdio_clk = devm_clk_get(&pdev->dev, "gcc_mdio_ahb_clk");
	if (!IS_ERR(am->mdio_clk)) {
		ret = clk_set_rate(am->mdio_clk, QCA_MDIO_CLK_RATE);
		if (ret)
			goto err_out;
		ret = clk_prepare_enable(am->mdio_clk);
		if (ret)
			goto err_out;
	}

	am->membase = devm_platform_ioremap_resource(pdev, 0);
	if (!am->membase) {
		dev_err(&pdev->dev, "unable to ioremap registers\n");
		ret = -ENOMEM;
		goto err_disable_clk;
	}

	am->mii_bus = mdiobus_alloc();
	if (!am->mii_bus) {
		ret = -ENOMEM;
		goto err_disable_clk;
	}

	am->clk_div = 0xf;
	am->force_c22 = of_property_read_bool(pdev->dev.of_node, "force_clause22");

	writel(CTRL_0_REG_DEFAULT_VALUE(am->clk_div), am->membase + MDIO_CTRL_0_REG);

	am->mii_bus->name = "qca_mdio";
	am->mii_bus->read = &qca_mdio_read;
	am->mii_bus->write = &qca_mdio_write;
	am->mii_bus->priv = am;
	am->mii_bus->parent = &pdev->dev;
	am->preinit = qca_mht_preinit;
	am->sw_read = qca_mii_read;
	am->sw_write = qca_mii_write;
	snprintf(am->mii_bus->id, MII_BUS_ID_SIZE, "%s", dev_name(&pdev->dev));

	for (i = 0; i < PHY_MAX_ADDR; i++) {
		am->phy_irq[i] = PHY_POLL;
		am->mii_bus->irq[i] = PHY_POLL;
	}

	platform_set_drvdata(pdev, am);

	am->preinit(am->mii_bus);
	ret = of_mdiobus_register(am->mii_bus, pdev->dev.of_node);
	if (ret)
		goto err_free_bus;

	dev_info(&pdev->dev, "qca-mdio driver was registered\n");

	if (of_machine_is_compatible("qcom,ipq5018")) {
		qca_tcsr_ldo_rdy_set(false);
		if (!IS_ERR(rst))
			reset_control_assert(rst);
	}

	return 0;

err_free_bus:
	mdiobus_free(am->mii_bus);
err_disable_clk:
	if (!IS_ERR(am->mdio_clk))
		clk_disable_unprepare(am->mdio_clk);
err_out:
	if (of_machine_is_compatible("qcom,ipq5018")) {
		qca_tcsr_ldo_rdy_set(false);
		if (!IS_ERR(rst))
			reset_control_assert(rst);
	}
	return ret;
}

static int qca_mdio_remove(struct platform_device *pdev)
{
	struct qca_mdio_data *am = platform_get_drvdata(pdev);

	if (am) {
		if (!IS_ERR(am->mdio_clk))
			clk_disable_unprepare(am->mdio_clk);
		mdiobus_unregister(am->mii_bus);
		mdiobus_free(am->mii_bus);
		platform_set_drvdata(pdev, NULL);
	}

	return 0;
}

static const struct of_device_id qca_mdio_dt_ids[] = {
	{ .compatible = "qcom,ipq40xx-mdio" },
	{ .compatible = "qcom,qca-mdio" },
	{ }
};
MODULE_DEVICE_TABLE(of, qca_mdio_dt_ids);

static struct platform_driver qca_mdio_driver = {
	.probe = qca_mdio_probe,
	.remove = qca_mdio_remove,
	.driver = {
		.name = "qca-mdio",
		.of_match_table = qca_mdio_dt_ids,
	},
};

module_platform_driver(qca_mdio_driver);

MODULE_DESCRIPTION("QCA MDIO interface driver");
MODULE_LICENSE("Dual BSD/GPL");
