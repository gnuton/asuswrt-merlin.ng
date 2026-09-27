// SPDX-License-Identifier: GPL-2.0+
/*
 * drivers/net/phy/at803x.c
 *
 * Driver for Atheros 803x PHY
 *
 * Author: Matus Ujhelyi <ujhelyi.m@gmail.com>
 */

#include <linux/phy.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/bitfield.h>

#define AT803X_SPECIFIC_FUNCTION_CONTROL        0x10
#define AT803X_SFC_MDI_CROSSOVER_MODE_M		GENMASK(6, 5)
#define AT803X_SFC_AUTOMATIC_CROSSOVER		0x3
#define AT803X_SFC_MANUAL_MDIX			0x1
#define AT803X_SFC_MANUAL_MDI			0x0
#define AT803X_SPECIFIC_STATUS			0x11
#define AT803X_SS_SPEED_MASK			GENMASK(15, 14)
#define AT803X_SS_SPEED_1000			2
#define AT803X_SS_SPEED_100			1
#define AT803X_SS_SPEED_10			0
#define AT803X_SS_DUPLEX			BIT(13)
#define AT803X_SS_SPEED_DUPLEX_RESOLVED		BIT(11)
#define AT803X_SS_MDIX				BIT(6)

#define QCA808X_SS_SPEED_MASK			GENMASK(9, 7)
#define QCA808X_SS_SPEED_2500			4

#define AT803X_INTR_ENABLE			0x12
#define AT803X_INTR_ENABLE_AUTONEG_ERR		BIT(15)
#define AT803X_INTR_ENABLE_SPEED_CHANGED	BIT(14)
#define AT803X_INTR_ENABLE_DUPLEX_CHANGED	BIT(13)
#define AT803X_INTR_ENABLE_PAGE_RECEIVED	BIT(12)
#define AT803X_INTR_ENABLE_LINK_FAIL		BIT(11)
#define AT803X_INTR_ENABLE_LINK_SUCCESS		BIT(10)
#define AT803X_INTR_ENABLE_WIRESPEED_DOWNGRADE	BIT(5)
#define AT803X_INTR_ENABLE_POLARITY_CHANGED	BIT(1)
#define AT803X_INTR_ENABLE_WOL			BIT(0)

#define AT803X_INTR_STATUS			0x13

#define AT803X_SMART_SPEED			0x14
#define AT803X_LED_CONTROL			0x18

#define AT803X_PHY_MMD3_WOL_CTRL		0x8012
#define AT803X_WOL_EN				BIT(5)
#define AT803X_LOC_MAC_ADDR_0_15_OFFSET		0x804C
#define AT803X_LOC_MAC_ADDR_16_31_OFFSET	0x804B
#define AT803X_LOC_MAC_ADDR_32_47_OFFSET	0x804A
#define AT803X_REG_CHIP_CONFIG			0x1f
#define AT803X_BT_BX_REG_SEL			0x8000
#define AT803X_SGMII_ANEG_EN			0x1000

#define AT803X_DEBUG_ADDR			0x1D
#define AT803X_DEBUG_DATA			0x1E

#define AT803X_MODE_CFG_MASK			0x0F
#define AT803X_MODE_CFG_SGMII			0x01

#define AT803X_PSSR			0x11	/*PHY-Specific Status Register*/
#define AT803X_PSSR_MR_AN_COMPLETE	0x0200

#define AT803X_DEBUG_REG_0			0x00
#define AT803X_DEBUG_RX_CLK_DLY_EN		BIT(15)

#define AT803X_DEBUG_REG_5			0x05
#define AT803X_DEBUG_TX_CLK_DLY_EN		BIT(8)

#define QCA8081_PHY_ID 0x004dd101
#define QCA8081_PHY_ID_MASK 0xffffffff

#define ATH8030_PHY_ID 0x004dd076
#define ATH8031_PHY_ID 0x004dd074
#define ATH8032_PHY_ID 0x004dd023
#define ATH8035_PHY_ID 0x004dd072
#define AT803X_PHY_ID_MASK			0xffffffef
#define AT8032_PHY_ID_MASK			0xffffffff

/* ADC threshold */
#define QCA808X_PHY_DEBUG_ADC_THRESHOLD		0x2c80
#define QCA808X_ADC_THRESHOLD_MASK		GENMASK(7, 0)
#define QCA808X_ADC_THRESHOLD_80MV		0
#define QCA808X_ADC_THRESHOLD_100MV		0xf0
#define QCA808X_ADC_THRESHOLD_200MV		0x0f
#define QCA808X_ADC_THRESHOLD_300MV		0xff

/* CLD control */
#define QCA808X_PHY_MMD3_ADDR_CLD_CTRL7		0x8007
#define QCA808X_8023AZ_AFE_CTRL_MASK		GENMASK(8, 4)
#define QCA808X_8023AZ_AFE_EN			0x90

/* AZ control */
#define QCA808X_PHY_MMD3_AZ_TRAINING_CTRL	0x8008
#define QCA808X_MMD3_AZ_TRAINING_VAL		0x1c32

#define QCA808X_PHY_MMD1_MSE_THRESHOLD_20DB	0x8014
#define QCA808X_MSE_THRESHOLD_20DB_VALUE	0x529

#define QCA808X_PHY_MMD1_MSE_THRESHOLD_17DB	0x800E
#define QCA808X_MSE_THRESHOLD_17DB_VALUE	0x341

#define QCA808X_PHY_MMD1_MSE_THRESHOLD_27DB	0x801E
#define QCA808X_MSE_THRESHOLD_27DB_VALUE	0x419

#define QCA808X_PHY_MMD1_MSE_THRESHOLD_28DB	0x8020
#define QCA808X_MSE_THRESHOLD_28DB_VALUE	0x341

#define QCA808X_PHY_MMD7_TOP_OPTION1		0x901c
#define QCA808X_TOP_OPTION1_DATA		0x0

#define QCA808X_PHY_MMD3_DEBUG_1		0xa100
#define QCA808X_MMD3_DEBUG_1_VALUE		0x9203
#define QCA808X_PHY_MMD3_DEBUG_2		0xa101
#define QCA808X_MMD3_DEBUG_2_VALUE		0x48ad
#define QCA808X_PHY_MMD3_DEBUG_3		0xa103
#define QCA808X_MMD3_DEBUG_3_VALUE		0x1698
#define QCA808X_PHY_MMD3_DEBUG_4		0xa105
#define QCA808X_MMD3_DEBUG_4_VALUE		0x8001
#define QCA808X_PHY_MMD3_DEBUG_5		0xa106
#define QCA808X_MMD3_DEBUG_5_VALUE		0x1111
#define QCA808X_PHY_MMD3_DEBUG_6		0xa011
#define QCA808X_MMD3_DEBUG_6_VALUE		0x5f85

/* master/slave seed config */
#define QCA808X_PHY_DEBUG_LOCAL_SEED		9
#define QCA808X_MASTER_SLAVE_SEED_ENABLE	BIT(1)
#define QCA808X_MASTER_SLAVE_SEED_CFG		GENMASK(12, 2)
#define QCA808X_MASTER_SLAVE_SEED_RANGE		0x32

/* QCA808X 1G chip type */
#define QCA808X_PHY_MMD7_CHIP_TYPE		0x901d
#define QCA808X_PHY_CHIP_TYPE_1G		BIT(0)

#define QCA8081_PHY_SERDES_MMD1_FIFO_CTRL	0x9072
#define QCA8081_PHY_FIFO_RSTN			BIT(11)

MODULE_DESCRIPTION("Qualcomm Atheros AR803x and QCA808X PHY driver");
MODULE_AUTHOR("Matus Ujhelyi");
MODULE_LICENSE("GPL");

struct at803x_priv {
	bool phy_reset:1;
};

struct at803x_context {
	u16 bmcr;
	u16 advertise;
	u16 control1000;
	u16 int_enable;
	u16 smart_speed;
	u16 led_control;
};

static int at803x_debug_reg_read(struct phy_device *phydev, u16 reg)
{
	int ret;

	ret = phy_write(phydev, AT803X_DEBUG_ADDR, reg);
	if (ret < 0)
		return ret;

	return phy_read(phydev, AT803X_DEBUG_DATA);
}

static int at803x_debug_reg_mask(struct phy_device *phydev, u16 reg,
				 u16 clear, u16 set)
{
	u16 val;
	int ret;

	ret = at803x_debug_reg_read(phydev, reg);
	if (ret < 0)
		return ret;

	val = ret & 0xffff;
	val &= ~clear;
	val |= set;

	return phy_write(phydev, AT803X_DEBUG_DATA, val);
}

static int at803x_enable_rx_delay(struct phy_device *phydev)
{
	return at803x_debug_reg_mask(phydev, AT803X_DEBUG_REG_0, 0,
				     AT803X_DEBUG_RX_CLK_DLY_EN);
}

static int at803x_enable_tx_delay(struct phy_device *phydev)
{
	return at803x_debug_reg_mask(phydev, AT803X_DEBUG_REG_5, 0,
				     AT803X_DEBUG_TX_CLK_DLY_EN);
}

static int at803x_disable_rx_delay(struct phy_device *phydev)
{
	return at803x_debug_reg_mask(phydev, AT803X_DEBUG_REG_0,
				     AT803X_DEBUG_RX_CLK_DLY_EN, 0);
}

static int at803x_disable_tx_delay(struct phy_device *phydev)
{
	return at803x_debug_reg_mask(phydev, AT803X_DEBUG_REG_5,
				     AT803X_DEBUG_TX_CLK_DLY_EN, 0);
}

/* save relevant PHY registers to private copy */
static void at803x_context_save(struct phy_device *phydev,
				struct at803x_context *context)
{
	context->bmcr = phy_read(phydev, MII_BMCR);
	context->advertise = phy_read(phydev, MII_ADVERTISE);
	context->control1000 = phy_read(phydev, MII_CTRL1000);
	context->int_enable = phy_read(phydev, AT803X_INTR_ENABLE);
	context->smart_speed = phy_read(phydev, AT803X_SMART_SPEED);
	context->led_control = phy_read(phydev, AT803X_LED_CONTROL);
}

/* restore relevant PHY registers from private copy */
static void at803x_context_restore(struct phy_device *phydev,
				   const struct at803x_context *context)
{
	phy_write(phydev, MII_BMCR, context->bmcr);
	phy_write(phydev, MII_ADVERTISE, context->advertise);
	phy_write(phydev, MII_CTRL1000, context->control1000);
	phy_write(phydev, AT803X_INTR_ENABLE, context->int_enable);
	phy_write(phydev, AT803X_SMART_SPEED, context->smart_speed);
	phy_write(phydev, AT803X_LED_CONTROL, context->led_control);
}

static int at803x_set_wol(struct phy_device *phydev,
			  struct ethtool_wolinfo *wol)
{
	struct net_device *ndev = phydev->attached_dev;
	const u8 *mac;
	int ret, irq_enabled;
	unsigned int i, offsets[] = {
		AT803X_LOC_MAC_ADDR_32_47_OFFSET,
		AT803X_LOC_MAC_ADDR_16_31_OFFSET,
		AT803X_LOC_MAC_ADDR_0_15_OFFSET,
	};

	if (!ndev)
		return -ENODEV;

	if (wol->wolopts & WAKE_MAGIC) {
		mac = (const u8 *) ndev->dev_addr;

		if (!is_valid_ether_addr(mac))
			return -EINVAL;

		for (i = 0; i < 3; i++)
			phy_write_mmd(phydev, MDIO_MMD_PCS, offsets[i],
				      mac[(i * 2) + 1] | (mac[(i * 2)] << 8));

		/* Enable WOL function */
		ret = phy_modify_mmd(phydev, MDIO_MMD_PCS, AT803X_PHY_MMD3_WOL_CTRL,
				0, AT803X_WOL_EN);
		if (ret)
			return ret;
		/* Enable WOL interrupt */
		ret = phy_modify(phydev, AT803X_INTR_ENABLE, 0, AT803X_INTR_ENABLE_WOL);
		if (ret)
			return ret;
	} else {
		/* Disable WoL function */
		ret = phy_modify_mmd(phydev, MDIO_MMD_PCS, AT803X_PHY_MMD3_WOL_CTRL,
				AT803X_WOL_EN, 0);
		if (ret)
			return ret;
		/* Disable WOL interrupt */
		ret = phy_modify(phydev, AT803X_INTR_ENABLE, AT803X_INTR_ENABLE_WOL, 0);
		if (ret)
			return ret;
	}

	/* Clear WOL status */
	ret = phy_read(phydev, AT803X_INTR_STATUS);
	if (ret < 0)
		return ret;

	/* Check if there are other interrupts except for WOL triggered when PHY is
	 * in interrupt mode, only the interrupts enabled by AT803X_INTR_ENABLE can
	 * be passed up to the interrupt PIN.
	 */
	irq_enabled = phy_read(phydev, AT803X_INTR_ENABLE);
	if (irq_enabled < 0)
		return irq_enabled;

	irq_enabled &= ~AT803X_INTR_ENABLE_WOL;
	if (ret & irq_enabled && !phy_polling_mode(phydev))
		phy_queue_state_machine(phydev, 0);

	return 0;
}

static void at803x_get_wol(struct phy_device *phydev,
			   struct ethtool_wolinfo *wol)
{
	u32 value;

	wol->supported = WAKE_MAGIC;
	wol->wolopts = 0;

	value = phy_read_mmd(phydev, MDIO_MMD_PCS, AT803X_PHY_MMD3_WOL_CTRL);
	if (value < 0)
		return;

	if (value & AT803X_WOL_EN)
		wol->wolopts |= WAKE_MAGIC;
}

static int at803x_suspend(struct phy_device *phydev)
{
	int value;
	int wol_enabled;

	value = phy_read(phydev, AT803X_INTR_ENABLE);
	wol_enabled = value & AT803X_INTR_ENABLE_WOL;

	if (wol_enabled)
		value = BMCR_ISOLATE;
	else
		value = BMCR_PDOWN;

	phy_modify(phydev, MII_BMCR, 0, value);

	return 0;
}

static int at803x_resume(struct phy_device *phydev)
{
	return phy_modify(phydev, MII_BMCR, BMCR_PDOWN | BMCR_ISOLATE, 0);
}

static int at803x_probe(struct phy_device *phydev)
{
	struct device *dev = &phydev->mdio.dev;
	struct at803x_priv *priv;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	phydev->priv = priv;

	return 0;
}

static int at803x_get_features(struct phy_device *phydev)
{
	int err;

	err = genphy_read_abilities(phydev);
	if (err)
		return err;

	if (phydev->drv->phy_id == QCA8081_PHY_ID) {
		err = phy_read_mmd(phydev, MDIO_MMD_AN, QCA808X_PHY_MMD7_CHIP_TYPE);
		if (err < 0)
			return err;

		/* QCA808X does not support 2.5G capability if the chip type is 1G from
		 * the register MMD7 QCA808X_PHY_MMD7_CHIP_TYPE.
		 */
		if (!(QCA808X_PHY_CHIP_TYPE_1G & err)) {
			err = phy_read_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_PMA_NG_EXTABLE);
			if (err < 0)
				return err;

			linkmode_mod_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->supported,
					err & MDIO_PMA_NG_EXTABLE_2_5GBT);
		}
	}

	if (phydev->drv->phy_id != ATH8031_PHY_ID && phydev->drv->phy_id != QCA8081_PHY_ID)
		return 0;

	/* AR8031/AR8033 have different status registers
	 * for copper and fiber operation. However, the
	 * extended status register is the same for both
	 * operation modes.
	 *
	 * As a result of that, ESTATUS_1000_XFULL is set
	 * to 1 even when operating in copper TP mode.
	 *
	 * Remove this mode from the supported link modes,
	 * as this driver currently only supports copper
	 * operation.
	 *
	 * QCA808X also need to remove the mode as only
	 * support copper operation.
	 */
	linkmode_clear_bit(ETHTOOL_LINK_MODE_1000baseX_Full_BIT,
			   phydev->supported);
	return 0;
}

static int at803x_config_init(struct phy_device *phydev)
{
	int ret;
	u32 v;

	if (phydev->drv->phy_id == ATH8031_PHY_ID &&
		phydev->interface == PHY_INTERFACE_MODE_SGMII)
	{
		v = phy_read(phydev, AT803X_REG_CHIP_CONFIG);
		/* select SGMII/fiber page */
		ret = phy_write(phydev, AT803X_REG_CHIP_CONFIG,
						v & ~AT803X_BT_BX_REG_SEL);
		if (ret)
			return ret;
		/* enable SGMII autonegotiation */
		ret = phy_write(phydev, MII_BMCR, AT803X_SGMII_ANEG_EN);
		if (ret)
			return ret;
		/* select copper page */
		ret = phy_write(phydev, AT803X_REG_CHIP_CONFIG,
						v | AT803X_BT_BX_REG_SEL);
		if (ret)
			return ret;
	}

	/* The RX and TX delay default is:
	 *   after HW reset: RX delay enabled and TX delay disabled
	 *   after SW reset: RX delay enabled, while TX delay retains the
	 *   value before reset.
	 */
	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    phydev->interface == PHY_INTERFACE_MODE_RGMII_RXID)
		ret = at803x_enable_rx_delay(phydev);
	else
		ret = at803x_disable_rx_delay(phydev);
	if (ret < 0)
		return ret;

	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID)
		ret = at803x_enable_tx_delay(phydev);
	else
		ret = at803x_disable_tx_delay(phydev);

	return ret;
}

static int at803x_ack_interrupt(struct phy_device *phydev)
{
	int err;

	err = phy_read(phydev, AT803X_INTR_STATUS);

	return (err < 0) ? err : 0;
}

static int at803x_config_intr(struct phy_device *phydev)
{
	int err;
	int value;

	value = phy_read(phydev, AT803X_INTR_ENABLE);

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED) {
		value |= AT803X_INTR_ENABLE_AUTONEG_ERR;
		value |= AT803X_INTR_ENABLE_SPEED_CHANGED;
		value |= AT803X_INTR_ENABLE_DUPLEX_CHANGED;
		value |= AT803X_INTR_ENABLE_LINK_FAIL;
		value |= AT803X_INTR_ENABLE_LINK_SUCCESS;

		err = phy_write(phydev, AT803X_INTR_ENABLE, value);
	}
	else
		err = phy_write(phydev, AT803X_INTR_ENABLE, 0);

	return err;
}

static void at803x_link_change_notify(struct phy_device *phydev)
{
	/*
	 * Conduct a hardware reset for AT8030/2 every time a link loss is
	 * signalled. This is necessary to circumvent a hardware bug that
	 * occurs when the cable is unplugged while TX packets are pending
	 * in the FIFO. In such cases, the FIFO enters an error mode it
	 * cannot recover from by software.
	 */
	if (phydev->state == PHY_NOLINK && phydev->mdio.reset_gpio) {
		struct at803x_context context;

		at803x_context_save(phydev, &context);

		phy_device_reset(phydev, 1);
		msleep(1);
		phy_device_reset(phydev, 0);
		msleep(1);

		at803x_context_restore(phydev, &context);

		phydev_dbg(phydev, "%s(): phy was reset\n", __func__);
	}
}

static int at803x_aneg_done(struct phy_device *phydev)
{
	int ccr;

	int aneg_done = genphy_aneg_done(phydev);
	if (aneg_done != BMSR_ANEGCOMPLETE)
		return aneg_done;

	/*
	 * in SGMII mode, if copper side autoneg is successful,
	 * also check SGMII side autoneg result
	 */
	ccr = phy_read(phydev, AT803X_REG_CHIP_CONFIG);
	if ((ccr & AT803X_MODE_CFG_MASK) != AT803X_MODE_CFG_SGMII)
		return aneg_done;

	/* switch to SGMII/fiber page */
	phy_write(phydev, AT803X_REG_CHIP_CONFIG, ccr & ~AT803X_BT_BX_REG_SEL);

	/* check if the SGMII link is OK. */
	if (!(phy_read(phydev, AT803X_PSSR) & AT803X_PSSR_MR_AN_COMPLETE)) {
		phydev_warn(phydev, "803x_aneg_done: SGMII link is not ok\n");
		aneg_done = 0;
	}
	/* switch back to copper page */
	phy_write(phydev, AT803X_REG_CHIP_CONFIG, ccr | AT803X_BT_BX_REG_SEL);

	return aneg_done;
}

static int at803x_read_specific_status(struct phy_device *phydev)
{
	int ss;

	/* Read the AT8035 PHY-Specific Status register, which indicates the
	 * speed and duplex that the PHY is actually using, irrespective of
	 * whether we are in autoneg mode or not.
	 */
	ss = phy_read(phydev, AT803X_SPECIFIC_STATUS);
	if (ss < 0)
		return ss;

	if (ss & AT803X_SS_SPEED_DUPLEX_RESOLVED) {
		int sfc, speed;

		sfc = phy_read(phydev, AT803X_SPECIFIC_FUNCTION_CONTROL);
		if (sfc < 0)
			return sfc;

		/* qca8081 takes the different bits for speed value from at803x */
		if (phydev->drv->phy_id == QCA8081_PHY_ID)
			speed = FIELD_GET(QCA808X_SS_SPEED_MASK, ss);
		else
			speed = FIELD_GET(AT803X_SS_SPEED_MASK, ss);

		switch (speed) {
		case AT803X_SS_SPEED_10:
			phydev->speed = SPEED_10;
			break;
		case AT803X_SS_SPEED_100:
			phydev->speed = SPEED_100;
			break;
		case AT803X_SS_SPEED_1000:
			phydev->speed = SPEED_1000;
			break;
		case QCA808X_SS_SPEED_2500:
			phydev->speed = SPEED_2500;
			break;
		}
		if (ss & AT803X_SS_DUPLEX)
			phydev->duplex = DUPLEX_FULL;
		else
			phydev->duplex = DUPLEX_HALF;
		if (ss & AT803X_SS_MDIX)
			phydev->mdix = ETH_TP_MDI_X;
		else
			phydev->mdix = ETH_TP_MDI;
	}

	return 0;
}

static int at803x_read_status(struct phy_device *phydev)
{
	int err, old_link = phydev->link;

	/* Update the link, but return if there was an error */
	err = genphy_update_link(phydev);
	if (err)
		return err;

	/* why bother the PHY if nothing can have changed */
	if (phydev->autoneg == AUTONEG_ENABLE && old_link && phydev->link)
		return 0;

	phydev->speed = SPEED_UNKNOWN;
	phydev->duplex = DUPLEX_UNKNOWN;
	phydev->pause = 0;
	phydev->asym_pause = 0;

	err = genphy_read_lpa(phydev);
	if (err < 0)
		return err;

	err = at803x_read_specific_status(phydev);
	if (err < 0)
		return err;

	if (phydev->autoneg == AUTONEG_ENABLE && phydev->autoneg_complete)
		phy_resolve_aneg_pause(phydev);

	return 0;
}

static int at803x_config_mdix(struct phy_device *phydev, u8 ctrl)
{
	u16 val;

	switch (ctrl) {
	case ETH_TP_MDI:
		val = AT803X_SFC_MANUAL_MDI;
		break;
	case ETH_TP_MDI_X:
		val = AT803X_SFC_MANUAL_MDIX;
		break;
	case ETH_TP_MDI_AUTO:
		val = AT803X_SFC_AUTOMATIC_CROSSOVER;
		break;
	default:
		return 0;
	}

	return phy_modify_changed(phydev, AT803X_SPECIFIC_FUNCTION_CONTROL,
			  AT803X_SFC_MDI_CROSSOVER_MODE_M,
			  FIELD_PREP(AT803X_SFC_MDI_CROSSOVER_MODE_M, val));
}

static int at803x_config_aneg(struct phy_device *phydev)
{
	int ret;

	ret = at803x_config_mdix(phydev, phydev->mdix_ctrl);
	if (ret < 0)
		return ret;

	/* Changes of the midx bits are disruptive to the normal operation;
	 * therefore any changes to these registers must be followed by a
	 * software reset to take effect.
	 */
	if (ret == 1) {
		ret = genphy_soft_reset(phydev);
		if (ret < 0)
			return ret;
	}

	/* Do not restart auto-negotiation by setting ret to 0 defautly,
	 * when calling __genphy_config_aneg later.
	 */
	ret = 0;

	if (phydev->drv->phy_id == QCA8081_PHY_ID) {
		int phy_ctrl = 0, duplex = 0;

		/* The reg MII_BMCR also needs to be configured for force mode, the
		 * genphy_config_aneg is also needed.
		 */
		if (phydev->autoneg == AUTONEG_DISABLE) {
			/*QCA8081 PHY support force duplex half, but genphy_c45_pma_setup_forced
			 *only support duplex full, so need to set duplex as full to configure speed
			 *when duplex is half
			 */
			duplex = phydev->duplex;
			if(phydev->duplex == DUPLEX_HALF)
				phydev->duplex = DUPLEX_FULL;
			genphy_c45_pma_setup_forced(phydev);
			phydev->duplex = duplex;

		}

		if (linkmode_test_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->advertising))
			phy_ctrl = MDIO_AN_10GBT_CTRL_ADV2_5G;

		ret = phy_modify_mmd_changed(phydev, MDIO_MMD_AN, MDIO_AN_10GBT_CTRL,
				MDIO_AN_10GBT_CTRL_ADV2_5G, phy_ctrl);
		if (ret < 0)
			return ret;
	}

	return __genphy_config_aneg(phydev, ret);
}

static int qca808x_phy_fast_retrain_config(struct phy_device *phydev)
{
	int ret;

	/* Enable fast retrain */
	ret = genphy_c45_fast_retrain(phydev, true);
	if (ret)
		return ret;

	phy_write_mmd(phydev, MDIO_MMD_AN, QCA808X_PHY_MMD7_TOP_OPTION1,
			QCA808X_TOP_OPTION1_DATA);
	phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QCA808X_PHY_MMD1_MSE_THRESHOLD_20DB,
			QCA808X_MSE_THRESHOLD_20DB_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QCA808X_PHY_MMD1_MSE_THRESHOLD_17DB,
			QCA808X_MSE_THRESHOLD_17DB_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QCA808X_PHY_MMD1_MSE_THRESHOLD_27DB,
			QCA808X_MSE_THRESHOLD_27DB_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QCA808X_PHY_MMD1_MSE_THRESHOLD_28DB,
			QCA808X_MSE_THRESHOLD_28DB_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_DEBUG_1,
			QCA808X_MMD3_DEBUG_1_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_DEBUG_4,
			QCA808X_MMD3_DEBUG_4_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_DEBUG_5,
			QCA808X_MMD3_DEBUG_5_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_DEBUG_3,
			QCA808X_MMD3_DEBUG_3_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_DEBUG_6,
			QCA808X_MMD3_DEBUG_6_VALUE);
	phy_write_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_DEBUG_2,
			QCA808X_MMD3_DEBUG_2_VALUE);

	return 0;
}

static int qca808x_phy_ms_random_seed_set(struct phy_device *phydev)
{
	u16 seed_value = (prandom_u32() % QCA808X_MASTER_SLAVE_SEED_RANGE);

	return at803x_debug_reg_mask(phydev, QCA808X_PHY_DEBUG_LOCAL_SEED,
			QCA808X_MASTER_SLAVE_SEED_CFG,
			FIELD_PREP(QCA808X_MASTER_SLAVE_SEED_CFG, seed_value));
}

static int qca808x_phy_ms_seed_enable(struct phy_device *phydev, bool enable)
{
	u16 seed_enable = 0;

	if (enable)
		seed_enable = QCA808X_MASTER_SLAVE_SEED_ENABLE;

	return at803x_debug_reg_mask(phydev, QCA808X_PHY_DEBUG_LOCAL_SEED,
			QCA808X_MASTER_SLAVE_SEED_ENABLE, seed_enable);
}

static int qca808x_config_init(struct phy_device *phydev)
{
	int ret;

	/* Active adc&vga on 802.3az for the link 1000M and 100M */
	ret = phy_modify_mmd(phydev, MDIO_MMD_PCS, QCA808X_PHY_MMD3_ADDR_CLD_CTRL7,
			QCA808X_8023AZ_AFE_CTRL_MASK, QCA808X_8023AZ_AFE_EN);
	if (ret)
		return ret;

	/* Adjust the threshold on 802.3az for the link 1000M */
	ret = phy_write_mmd(phydev, MDIO_MMD_PCS,
			QCA808X_PHY_MMD3_AZ_TRAINING_CTRL, QCA808X_MMD3_AZ_TRAINING_VAL);
	if (ret)
		return ret;

	if (linkmode_test_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->supported)) {
		/* Config the fast retrain for the link 2500M */
		ret = qca808x_phy_fast_retrain_config(phydev);
		if (ret)
			return ret;

		/* Configure lower ramdom seed to make phy linked as slave mode */
		ret = qca808x_phy_ms_random_seed_set(phydev);
		if (ret)
			return ret;

		/* Enable seed */
		ret = qca808x_phy_ms_seed_enable(phydev, true);
		if (ret)
			return ret;
	}

	/* Configure adc threshold as 100mv for the link 10M */
	return at803x_debug_reg_mask(phydev, QCA808X_PHY_DEBUG_ADC_THRESHOLD,
			QCA808X_ADC_THRESHOLD_MASK, QCA808X_ADC_THRESHOLD_100MV);
}

static int qca808x_fifo_reset(struct phy_device *phydev, bool reset)
{
	int val, ret;
	u32 addr;

	if (phydev->phy_id != QCA8081_PHY_ID)
		return 0;

	addr = MII_ADDR_C45 | (MDIO_MMD_PMAPMD << 16) |
		(QCA8081_PHY_SERDES_MMD1_FIFO_CTRL & 0xffff);

	/* Reset serdes fifo, the serdes address is phy address added by 1 */
	val = mdiobus_read(phydev->mdio.bus, phydev->mdio.addr + 1, addr);

	if (reset)
		val &= ~QCA8081_PHY_FIFO_RSTN;
	else
		val |= QCA8081_PHY_FIFO_RSTN;

	ret = mdiobus_write(phydev->mdio.bus, phydev->mdio.addr + 1, addr, val);

	return ret;
}

static int qca808x_read_status(struct phy_device *phydev)
{
	int ret;

	ret = phy_read_mmd(phydev, MDIO_MMD_AN, MDIO_AN_10GBT_STAT);
	if (ret < 0)
		return ret;

	linkmode_mod_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->lp_advertising,
			ret & MDIO_AN_10GBT_STAT_LP2_5G);

	ret = genphy_read_status(phydev);
	if (ret)
		return ret;

	ret = at803x_read_specific_status(phydev);
	if (ret < 0)
		return ret;

	/* Need to reset fifo to avoid garbge packet generated when link is changed */
	if (phydev->link)
		qca808x_fifo_reset(phydev, false);
	else
		qca808x_fifo_reset(phydev, true);

	if (phydev->link && phydev->speed == SPEED_2500)
		phydev->interface = PHY_INTERFACE_MODE_2500BASEX;
	else
		phydev->interface = PHY_INTERFACE_MODE_SMII;

	/* generate seed as a lower random value to make PHY linked as SLAVE easily,
	 * except for master/slave configuration fault detected.
	 * the reason for not putting this code into the function link_change_notify is
	 * the corner case where the link partner is also the qca8081 PHY and the seed
	 * value is configured as the same value, the link can't be up and no link change
	 * occurs.
	 */
	if (linkmode_test_bit(ETHTOOL_LINK_MODE_2500baseT_Full_BIT, phydev->supported) &&
			!phydev->link) {
		int val = phy_read(phydev, MII_STAT1000);
		if (val & LPA_1000MSFAIL) {
			qca808x_phy_ms_seed_enable(phydev, false);
		} else {
			qca808x_phy_ms_random_seed_set(phydev);
			qca808x_phy_ms_seed_enable(phydev, true);
		}
	}

	return 0;
}

static int qca808x_soft_reset(struct phy_device *phydev)
{
	int ret;

	ret = genphy_soft_reset(phydev);
	if (ret < 0)
		return ret;

	return qca808x_phy_ms_seed_enable(phydev, true);
}

static struct phy_driver at803x_driver[] = {
{
	/* ATHEROS 8035 */
	.phy_id			= ATH8035_PHY_ID,
	.name			= "Atheros 8035 ethernet",
	.phy_id_mask		= AT803X_PHY_ID_MASK,
	.probe			= at803x_probe,
	.config_init		= at803x_config_init,
	.set_wol		= at803x_set_wol,
	.get_wol		= at803x_get_wol,
	.suspend		= at803x_suspend,
	.resume			= at803x_resume,
	/* PHY_GBIT_FEATURES */
	.read_status		= at803x_read_status,
	.ack_interrupt		= at803x_ack_interrupt,
	.config_intr		= at803x_config_intr,
}, {
	/* ATHEROS 8030 */
	.phy_id			= ATH8030_PHY_ID,
	.name			= "Atheros 8030 ethernet",
	.phy_id_mask		= AT803X_PHY_ID_MASK,
	.probe			= at803x_probe,
	.config_init		= at803x_config_init,
	.link_change_notify	= at803x_link_change_notify,
	.set_wol		= at803x_set_wol,
	.get_wol		= at803x_get_wol,
	.suspend		= at803x_suspend,
	.resume			= at803x_resume,
	/* PHY_BASIC_FEATURES */
	.ack_interrupt		= at803x_ack_interrupt,
	.config_intr		= at803x_config_intr,
}, {
	/* ATHEROS 8031 */
	.phy_id			= ATH8031_PHY_ID,
	.name			= "Atheros 8031 ethernet",
	.phy_id_mask		= AT803X_PHY_ID_MASK,
	.probe			= at803x_probe,
	.config_init		= at803x_config_init,
	.set_wol		= at803x_set_wol,
	.get_wol		= at803x_get_wol,
	.suspend		= at803x_suspend,
	.resume			= at803x_resume,
	/* PHY_GBIT_FEATURES */
	.read_status		= at803x_read_status,
	.aneg_done		= at803x_aneg_done,
	.ack_interrupt		= &at803x_ack_interrupt,
	.config_intr		= &at803x_config_intr,
}, {
	/* ATHEROS 8032 */
	.phy_id			= ATH8032_PHY_ID,
	.name			= "Atheros 8032 ethernet",
	.phy_id_mask		= AT8032_PHY_ID_MASK,
	.probe			= at803x_probe,
	.config_init		= at803x_config_init,
	.link_change_notify	= at803x_link_change_notify,
	.set_wol		= at803x_set_wol,
	.get_wol		= at803x_get_wol,
	.suspend		= at803x_suspend,
	.resume			= at803x_resume,
	/* PHY_BASIC_FEATURES */
	.read_status		= at803x_read_status,
	.config_aneg		= genphy_config_aneg,
	.read_status		= genphy_read_status,
	.ack_interrupt		= at803x_ack_interrupt,
	.config_intr		= at803x_config_intr,
}, {
	/* Qualcomm QCA8081 */
	.phy_id                 = QCA8081_PHY_ID,
	.phy_id_mask		= QCA8081_PHY_ID_MASK,
	.name			= "QCA808X ethernet",
	.config_intr		= at803x_config_intr,
	.ack_interrupt		= at803x_ack_interrupt,
	.set_wol		= at803x_set_wol,
	.get_wol		= at803x_get_wol,
	.get_features		= at803x_get_features,
	.config_aneg		= at803x_config_aneg,
	.suspend		= genphy_suspend,
	.resume			= genphy_resume,
	.read_status		= qca808x_read_status,
	.config_init		= qca808x_config_init,
	.soft_reset		= qca808x_soft_reset,
}, };

module_phy_driver(at803x_driver);

static struct mdio_device_id __maybe_unused atheros_tbl[] = {
	{ ATH8030_PHY_ID, AT803X_PHY_ID_MASK },
	{ ATH8031_PHY_ID, AT803X_PHY_ID_MASK },
	{ ATH8032_PHY_ID, AT8032_PHY_ID_MASK },
	{ ATH8035_PHY_ID, AT803X_PHY_ID_MASK },
	{ QCA8081_PHY_ID, QCA8081_PHY_ID_MASK},
	{ }
};

MODULE_DEVICE_TABLE(mdio, atheros_tbl);
