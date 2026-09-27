// SPDX-License-Identifier: GPL-2.0
/*
 * Qualcomm PCIe root complex driver
 *
 * Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 * Copyright 2015 Linaro Limited.
 *
 * Author: Stanimir Varbanov <svarbanov@mm-sol.com>
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_pci.h>
#include <linux/pci.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <linux/phy/phy.h>
#include <linux/reboot.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/nvmem-consumer.h>
#include <linux/of_clk.h>
#include <soc/qcom/socinfo.h>
#include "../../pci.h"

#include "pcie-designware.h"

#define PCIE20_PARF_SYS_CTRL			0x00
#define MST_WAKEUP_EN				BIT(13)
#define SLV_WAKEUP_EN				BIT(12)
#define MSTR_ACLK_CGC_DIS			BIT(10)
#define SLV_ACLK_CGC_DIS			BIT(9)
#define CORE_CLK_CGC_DIS			BIT(6)
#define AUX_PWR_DET				BIT(4)
#define L23_CLK_RMV_DIS				BIT(2)
#define L1_CLK_RMV_DIS				BIT(1)

#define PCIE_ATU_CR1_OUTBOUND_6_GEN3		0xC00
#define PCIE_ATU_CR2_OUTBOUND_6_GEN3		0xC04
#define PCIE_ATU_LOWER_BASE_OUTBOUND_6_GEN3	0xC08
#define PCIE_ATU_UPPER_BASE_OUTBOUND_6_GEN3	0xC0C
#define PCIE_ATU_LIMIT_OUTBOUND_6_GEN3		0xC10
#define PCIE_ATU_LOWER_TARGET_OUTBOUND_6_GEN3	0xC14
#define PCIE_ATU_UPPER_TARGET_OUTBOUND_6_GEN3	0xC18

#define PCIE_ATU_CR1_OUTBOUND_7_GEN3		0xE00
#define PCIE_ATU_CR2_OUTBOUND_7_GEN3		0xE04
#define PCIE_ATU_LOWER_BASE_OUTBOUND_7_GEN3	0xE08
#define PCIE_ATU_UPPER_BASE_OUTBOUND_7_GEN3	0xE0C
#define PCIE_ATU_LIMIT_OUTBOUND_7_GEN3		0xE10
#define PCIE_ATU_LOWER_TARGET_OUTBOUND_7_GEN3	0xE14
#define PCIE_ATU_UPPER_TARGET_OUTBOUND_7_GEN3 	0xE18

#define PCIE20_COMMAND_STATUS			0x04
#define CMD_BME_VAL				0x4
#define BUS_MASTER_EN				0x7
#define PCIE20_DEVICE_CONTROL2_STATUS2		0x98
#define PCIE_CAP_CPL_TIMEOUT_DISABLE		0x10
#define PCIE30_GEN3_RELATED_OFF			0x890

#define PCIE_DEVICE_CONTROL_STATUS		0x78
#define PCIE_CAP_MAX_PAYLOAD_SIZE_CS_MASK 	GENMASK(7, 5)
#define PCIE_CAP_MAX_PAYLOAD_SIZE_CS_OFFSET(x) 	((x) << 5)

#define RXEQ_RGRDLESS_RXTS			BIT(13)
#define GEN3_ZRXDC_NONCOMPL			BIT(0)

#define PCIE20_PARF_PHY_CTRL			0x40
#define PHY_CTRL_PHY_TX0_TERM_OFFSET_MASK	GENMASK(20, 16)
#define PHY_CTRL_PHY_TX0_TERM_OFFSET(x)		((x) << 16)

#define PCIE20_PARF_PHY_REFCLK			0x4C
#define PHY_REFCLK_SSP_EN			BIT(16)
#define PHY_REFCLK_USE_PAD			BIT(12)

#define PCIE20_PARF_DBI_BASE_ADDR		0x168
#define PCIE20_PARF_SLV_ADDR_SPACE_SIZE		0x16C
#define PCIE20_PARF_MHI_CLOCK_RESET_CTRL	0x174

#define AHB_CLK_EN				BIT(0)
#define MSTR_AXI_CLK_EN				BIT(1)
#define BYPASS					BIT(4)

#define __mask(a, b)		(((1 << ((a) + 1)) - 1) & ~((1 << (b)) - 1))

#define PARF_BDF_TO_SID_TABLE			0x2000
#define PCIE20_LNK_CONTROL2_LINK_STATUS2	0xA0
#define PCIE_CAP_TARGET_LINK_SPEED_MASK		__mask(3, 0)
#define PCIE_CAP_CURR_DEEMPHASIS		BIT(16)
#define SPEED_GEN1				0x1
#define SPEED_GEN2				0x2
#define SPEED_GEN3				0x3
#define PCIE_V2_PARF_SIZE			0x2000
#define AXI_CLK_RATE				200000000
#define RCHNG_CLK_RATE				100000000
#define AXI_CLK_RATE_IPQ9574 			240000000
#define AXI_M_2LANE_CLK_RATE_IPQ9574 		342857143

#define PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT	0x178
#define PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2	0x1A8
#define PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2_MASK	0x1F
#define PCIE20_PARF_LTSSM			0x1B0
#define PCIE20_PARF_SID_OFFSET			0x234
#define PCIE20_PARF_BDF_TRANSLATE_CFG		0x24C
#define PCIE20_PARF_DEVICE_TYPE			0x1000

#define PCIE20_ELBI_SYS_CTRL			0x04
#define PCIE20_ELBI_SYS_CTRL_LT_ENABLE		BIT(0)

#define PCIE20_AXI_MSTR_RESP_COMP_CTRL0		0x818
#define CFG_REMOTE_RD_REQ_BRIDGE_SIZE_2K	0x4
#define CFG_REMOTE_RD_REQ_BRIDGE_SIZE_4K	0x5
#define PCIE20_AXI_MSTR_RESP_COMP_CTRL1		0x81c
#define CFG_BRIDGE_SB_INIT			BIT(0)

#define PCIE20_CAP				0x70
#define PCIE20_CAP_LINK_CAPABILITIES		(PCIE20_CAP + 0xC)
#define PCIE20_CAP_ACTIVE_STATE_LINK_PM_SUPPORT	(BIT(10) | BIT(11))
#define PCIE20_CAP_LINK_1			(PCIE20_CAP + 0x14)
#define PCIE_CAP_LINK1_VAL			0x2FD7F

#define PCIE20_PARF_Q2A_FLUSH			0x1AC

#define PCIE20_MISC_CONTROL_1_REG		0x8BC
#define DBI_RO_WR_EN				1

#define PERST_DELAY_US				1000
/* PARF registers */
#define PCIE20_PARF_PCS_DEEMPH			0x34
#define PCS_DEEMPH_TX_DEEMPH_GEN1(x)		((x) << 16)
#define PCS_DEEMPH_TX_DEEMPH_GEN2_3_5DB(x)	((x) << 8)
#define PCS_DEEMPH_TX_DEEMPH_GEN2_6DB(x)	((x) << 0)

#define PCIE20_PARF_PCS_SWING			0x38
#define PCS_SWING_TX_SWING_FULL(x)		((x) << 8)
#define PCS_SWING_TX_SWING_LOW(x)		((x) << 0)

#define PCIE20_PARF_CONFIG_BITS		0x50
#define PHY_RX0_EQ(x)				((x) << 24)

#define PCIE20_v3_PARF_SLV_ADDR_SPACE_SIZE	0x358
#define SLV_ADDR_SPACE_SZ			0x10000000

/* DBI registers */
#define PCIE20_PORT_LINK_CTRL_OFF		0x710
#define PCIE20_PORT_LINK_CTRL_OFF_MASK		0x3F0000
#define LINK_CAPABLE_OFFSET(x)			((x) << 16)
#define PCIE20_LANE_SKEW_OFF			0x714
#define PCIE20_LANE_SKEW_OFF_MASK		0xFF000000
#define PCIE20_MULTI_LANE_CONTROL_OFF		0x8C0
#define PCIE20_LINK_CONTROL_LINK_STATUS_REG	0x80
#define PCIE20_PARF_LTSSM_MASK			0x3F

/* RATEADAPT_VAL = 256 / ((NOC frequency / PCIe AXI frequency) - 1) */
/* RATEADAPT_VAL = 256 / ((342M / 240M) - 1) */
#define AGGR_NOC_PCIE_1LANE_RATEADAPT_VAL	0x200

/*RATEADAPT_VAL = 256 / ((266M / 200M) - 1) = 775 */
#define SYSTEM_NOC_PCIE_RATEADAPT_VAL		0x307

/* RATEADAPT_VAL = 256 / ((266M / 240M) - 1) = 2363 > Max Value 1023*/
#define SYSTEM_NOC_PCIE_RATEADAPT_VAL_MAX	0x3FF

#define SYSTEM_NOC_PCIE_RATEADAPT_BYPASS	0x1

#define DEVICE_TYPE_RC				0x4

#define PARF_INT_ALL_STATUS			0x224
#define PARF_INT_ALL_CLEAR			0x228
#define PARF_INT_ALL_MASK			0x22c

/* PARF_INT_ALL_{STATUS/CLEAR/MASK} register fields */
#define PARF_INT_ALL_LINK_DOWN                  BIT(1)
#define PARF_INT_ALL_LINK_UP                    BIT(13)

/* Virtual Channel registers and fields */
#define PCIE_TYPE0_VC_CAPABILITIES_REG_1 	0x14c
#define PCIE_TYPE0_RESOURCE_CON_REG_VC0 	0x15c
#define PCIE_TYPE0_RESOURCE_CON_REG_VC1 	0x168
#define PCIE_TYPE0_RESOURCE_STATUS_REG_VC1 	0x16c

#define EP_REG_BASE 				0x1E1DFFF
#define WINDOW_RANGE_MASK 			0x7FFFE

#define VC_EXT_VC_CNT_MASK 			GENMASK(2, 0)
#define VC_TC_MAP_VC_BIT1_MASK 			GENMASK(7, 1)
#define VC_ID_VC1_MASK 				GENMASK(26, 24)
#define VC_ENABLE_VC1_MASK 			BIT(31)
#define VC_NEGO_PENDING_VC1_MASK 		BIT(17)

#define VC_EXT_VC_CNT_OFFSET(x) 		((x) << 0)
#define VC_TC_MAP_VC_BIT1_OFFSET(x) 		((x) << 1)
#define VC_ID_VC1_OFFSET(x) 			((x) << 24)
#define VC_ENABLE_VC1_OFFSET(x) 		((x) << 31)
#define VC_NEGO_PENDING_VC1_OFFSET(x) 		((x) << 17)

#define QCOM_PCIE_2_1_0_MAX_SUPPLY	3

#define PCIE_CAP_LINK_SPEED_MASK		GENMASK(19, 16)
#define PCIE_CAP_NEGO_LINK_WIDTH_MASK		GENMASK(25, 20)
#define PCIE_CAP_RETRAIN_LINK_MASK		GENMASK(5, 5)
#define PCIE_CAP_LINK_SPEED_SHIFT(x)		((x) >> 16)
#define PCIE_CAP_NEGO_LINK_WIDTH_SHIFT(x)	((x) >> 20)
#define PCIE_CAP_RETRAIN_LINK_OFFSET(x)		((x) << 5)
#define PCIE_CAP_TARGET_LINK_SPD_MASK		GENMASK(3, 0)
#define QCOM_IPQ9574_DEVICE_ID			0x1108
#define QCOM_IPQ5332_DEVICE_ID			0x1005
#define QCOM_PCIE_MAX_RETRY			3

struct qcom_pcie_resources_2_1_0 {
	struct clk *iface_clk;
	struct clk *core_clk;
	struct clk *phy_clk;
	struct clk *aux_clk;
	struct clk *ref_clk;
	struct reset_control *pci_reset;
	struct reset_control *axi_reset;
	struct reset_control *ahb_reset;
	struct reset_control *por_reset;
	struct reset_control *phy_reset;
	struct reset_control *ext_reset;
	struct regulator_bulk_data supplies[QCOM_PCIE_2_1_0_MAX_SUPPLY];
};

struct qcom_pcie_resources_1_0_0 {
	struct clk *iface;
	struct clk *aux;
	struct clk *master_bus;
	struct clk *slave_bus;
	struct reset_control *core;
	struct regulator *vdda;
};

#define QCOM_PCIE_2_3_2_MAX_SUPPLY	2
struct qcom_pcie_resources_2_3_2 {
	struct clk *aux_clk;
	struct clk *master_clk;
	struct clk *slave_clk;
	struct clk *cfg_clk;
	struct clk *pipe_clk;
	struct regulator_bulk_data supplies[QCOM_PCIE_2_3_2_MAX_SUPPLY];
};

#define QCOM_PCIE_2_4_0_MAX_CLOCKS	4
struct qcom_pcie_resources_2_4_0 {
	struct clk_bulk_data clks[QCOM_PCIE_2_4_0_MAX_CLOCKS];
	int num_clks;
	struct reset_control *axi_m_reset;
	struct reset_control *axi_s_reset;
	struct reset_control *pipe_reset;
	struct reset_control *axi_m_vmid_reset;
	struct reset_control *axi_s_xpu_reset;
	struct reset_control *parf_reset;
	struct reset_control *phy_reset;
	struct reset_control *axi_m_sticky_reset;
	struct reset_control *pipe_sticky_reset;
	struct reset_control *pwr_reset;
	struct reset_control *ahb_reset;
	struct reset_control *phy_ahb_reset;
};

struct qcom_pcie_resources_2_5_0 {
	struct clk *iface;
	struct clk *axi_m_clk;
	struct clk *axi_s_clk;
	struct reset_control *rst[7];
};

struct qcom_pcie_resources_2_9_0 {
	struct clk *iface;
	struct clk *axi_m_clk;
	struct clk *axi_s_clk;
	struct clk *axi_bridge_clk;
	struct clk *rchng_clk;
	struct clk *ahb_clk;
	struct clk *aux_clk;
	struct reset_control *rst[8];
};

union qcom_pcie_resources {
	struct qcom_pcie_resources_1_0_0 v1_0_0;
	struct qcom_pcie_resources_2_1_0 v2_1_0;
	struct qcom_pcie_resources_2_3_2 v2_3_2;
	struct qcom_pcie_resources_2_4_0 v2_4_0;
	struct qcom_pcie_resources_2_5_0 v2_5_0;
	struct qcom_pcie_resources_2_9_0 v2_9_0;
};

struct qcom_pcie;

struct qcom_pcie_ops {
	int (*get_resources)(struct qcom_pcie *pcie);
	int (*init)(struct qcom_pcie *pcie);
	int (*post_init)(struct qcom_pcie *pcie);
	void (*deinit)(struct qcom_pcie *pcie);
	void (*post_deinit)(struct qcom_pcie *pcie);
	void (*ltssm_enable)(struct qcom_pcie *pcie);
};

struct qcom_pcie_of_data {
	const struct qcom_pcie_ops *ops;
	unsigned int version;
};

struct qcom_pcie {
	struct dw_pcie *pci;
	void __iomem *parf;			/* DT parf */
	void __iomem *elbi;			/* DT elbi */
	void __iomem *aggr_noc;
	resource_size_t parf_size;
	void __iomem *system_noc;
	union qcom_pcie_resources res;
	struct phy *phy;
	struct gpio_desc *reset;
	const struct qcom_pcie_ops *ops;
	u32 max_speed;
	uint32_t slv_addr_space_sz;
	uint32_t num_lanes;
	uint32_t domain;
	uint32_t compliance;
	uint32_t slot_id;
	uint32_t axi_wr_addr_halt;
	uint32_t max_payload_size;
	bool enable_vc;
	bool enable_retry;
	struct work_struct handle_wake_work;
	struct work_struct handle_e911_work;
	int wake_irq;
	int mdm2ap_e911_irq;
	int global_irq;
	bool enumerated;
	uint32_t rc_idx;
	struct notifier_block pci_reboot_notifier;
};

#define to_qcom_pcie(x)		dev_get_drvdata((x)->dev)

struct qcom_pcie_info{
	struct qcom_pcie *pcie;
	struct list_head node;
};

LIST_HEAD(qcom_pcie_list);

extern struct pci_ops dw_pcie_ops;
struct gpio_desc *mdm2ap_e911;
static void pcie_slot_remove(int val);

static void qcom_ep_reset_assert(struct qcom_pcie *pcie)
{
	gpiod_set_value_cansleep(pcie->reset, 1);
	usleep_range(PERST_DELAY_US, PERST_DELAY_US + 500);
}

static void qcom_ep_reset_deassert(struct qcom_pcie *pcie)
{
	/* Ensure that PERST has been asserted for at least 100 ms */
	msleep(100);
	gpiod_set_value_cansleep(pcie->reset, 0);
	usleep_range(PERST_DELAY_US, PERST_DELAY_US + 500);
}

static int qcom_pcie_establish_link(struct qcom_pcie *pcie)
{
	struct dw_pcie *pci = pcie->pci;

	if (dw_pcie_link_up(pci))
		return 0;

	/* Enable Link Training state machine */
	if (pcie->ops->ltssm_enable)
		pcie->ops->ltssm_enable(pcie);

	return dw_pcie_wait_for_link(pci);
}

int pci_create_scan_root_bus(struct pcie_port *pp)
{
	int ret;
	LIST_HEAD(res);
	struct pci_bus *child;
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct device *dev = pci->dev;
	struct pci_host_bridge *hbrg;
	struct pci_bus *root_bus;

	pci_add_resource(&res, pp->busn);
	pci_add_resource(&res, pp->io);
	pci_add_resource(&res, pp->mem);

	if (pp->ops->host_init) {
		ret = pp->ops->host_init(pp);
		if (ret)
			return ret;
	}

	root_bus = pci_scan_root_bus(dev,
			pp->busn->start, &dw_pcie_ops, pp, &res);

	if (!root_bus) {
		dev_err(pci->dev, "root_bus is not created\n");
		return -ENOMEM;
	}

	pp->root_bus_nr = pp->busn->start;
	pp->root_bus = root_bus;

	if (pp->ops->scan_bus)
		pp->ops->scan_bus(pp);

	hbrg = pci_find_host_bridge(pp->root_bus);
	hbrg->map_irq = of_irq_parse_and_map_pci;

	pci_bus_size_bridges(pp->root_bus);
	pci_bus_assign_resources(pp->root_bus);

	list_for_each_entry(child, &pp->root_bus->children, node)
		pcie_bus_configure_settings(child);

	pci_bus_add_devices(pp->root_bus);
	return 0;
}

/* PCIe wake-irq handler */
static void handle_wake_func(struct work_struct *work)
{
	int ret;
	struct qcom_pcie *pcie = container_of(work, struct qcom_pcie,
					      handle_wake_work);
	struct pcie_port *pp = &(pcie->pci)->pp;

	dev_info(pcie->pci->dev, "PCIE wake recieved\n");
	pci_lock_rescan_remove();
	if (pcie->enumerated) {
		pr_info("PCIe: RC has been already enumerated\n");
		pci_unlock_rescan_remove();
		return;
	}

	if (pcie->mdm2ap_e911_irq >= 0) {
		if (!gpiod_get_value(mdm2ap_e911)) {
			pr_debug("[%s]No data call.", __func__);
			ret = pci_create_scan_root_bus(pp);
			if (ret) {
				pr_err("PCIe: failed to enable RC upon wake request from the device\n");
			} else {
				pcie->enumerated = true;
				pr_info("PCIe: enumerated RC successfully upon wake request from the device\n");
			}
		}
	}

	pci_unlock_rescan_remove();
}

static irqreturn_t qcom_pcie_wake_irq_handler(int irq, void *data)
{
	struct qcom_pcie *pcie = data;

	schedule_work(&pcie->handle_wake_work);

	return IRQ_HANDLED;
}

static irqreturn_t qcom_pcie_global_irq_thread_fn(int irq, void *data)
{
	struct qcom_pcie *pcie = data;
	u32 status, mask;

	status = readl_relaxed(pcie->parf + PARF_INT_ALL_STATUS);
	mask = readl_relaxed(pcie->parf + PARF_INT_ALL_MASK);

	writel_relaxed(status, pcie->parf + PARF_INT_ALL_CLEAR);

	status &= mask;

	if (status & PARF_INT_ALL_LINK_DOWN) {
		dev_info(pcie->pci->dev, "Received Link down event for RC %u\n",
								pcie->rc_idx);
		pcie_slot_remove(pcie->slot_id);
	} else if (status & PARF_INT_ALL_LINK_UP) {
		dev_info(pcie->pci->dev, "Received Link up event for RC %u\n",
								pcie->rc_idx);
	}

	return IRQ_HANDLED;
}

static void qcom_pcie_2_1_0_ltssm_enable(struct qcom_pcie *pcie)
{
	u32 val;

	/* enable link training */
	val = readl(pcie->elbi + PCIE20_ELBI_SYS_CTRL);
	val |= PCIE20_ELBI_SYS_CTRL_LT_ENABLE;
	writel(val, pcie->elbi + PCIE20_ELBI_SYS_CTRL);
}

static int qcom_pcie_get_resources_2_1_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_1_0 *res = &pcie->res.v2_1_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	int ret;

	res->supplies[0].supply = "vdda";
	res->supplies[1].supply = "vdda_phy";
	res->supplies[2].supply = "vdda_refclk";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(res->supplies),
				      res->supplies);
	if (ret)
		return ret;

	res->iface_clk = devm_clk_get(dev, "iface");
	if (IS_ERR(res->iface_clk))
		return PTR_ERR(res->iface_clk);

	res->core_clk = devm_clk_get(dev, "core");
	if (IS_ERR(res->core_clk))
		return PTR_ERR(res->core_clk);

	res->phy_clk = devm_clk_get(dev, "phy");
	if (IS_ERR(res->phy_clk))
		return PTR_ERR(res->phy_clk);

	res->aux_clk = devm_clk_get_optional(dev, "aux");
	if (IS_ERR(res->aux_clk))
		return PTR_ERR(res->aux_clk);

	res->ref_clk = devm_clk_get_optional(dev, "ref");
	if (IS_ERR(res->ref_clk))
		return PTR_ERR(res->ref_clk);

	res->pci_reset = devm_reset_control_get_exclusive(dev, "pci");
	if (IS_ERR(res->pci_reset))
		return PTR_ERR(res->pci_reset);

	res->axi_reset = devm_reset_control_get_exclusive(dev, "axi");
	if (IS_ERR(res->axi_reset))
		return PTR_ERR(res->axi_reset);

	res->ahb_reset = devm_reset_control_get_exclusive(dev, "ahb");
	if (IS_ERR(res->ahb_reset))
		return PTR_ERR(res->ahb_reset);

	res->por_reset = devm_reset_control_get_exclusive(dev, "por");
	if (IS_ERR(res->por_reset))
		return PTR_ERR(res->por_reset);

	res->ext_reset = devm_reset_control_get_optional_exclusive(dev, "ext");
	if (IS_ERR(res->ext_reset))
		return PTR_ERR(res->ext_reset);

	res->phy_reset = devm_reset_control_get_exclusive(dev, "phy");
	return PTR_ERR_OR_ZERO(res->phy_reset);
}

static void qcom_pcie_deinit_2_1_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_1_0 *res = &pcie->res.v2_1_0;

	clk_disable_unprepare(res->phy_clk);
	reset_control_assert(res->pci_reset);
	reset_control_assert(res->axi_reset);
	reset_control_assert(res->ahb_reset);
	reset_control_assert(res->por_reset);
	reset_control_assert(res->ext_reset);
	reset_control_assert(res->phy_reset);
	clk_disable_unprepare(res->iface_clk);
	clk_disable_unprepare(res->core_clk);
	clk_disable_unprepare(res->aux_clk);
	clk_disable_unprepare(res->ref_clk);

	writel(1, pcie->parf + PCIE20_PARF_PHY_CTRL);

	regulator_bulk_disable(ARRAY_SIZE(res->supplies), res->supplies);
}

static int qcom_pcie_init_2_1_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_1_0 *res = &pcie->res.v2_1_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	struct device_node *node = dev->of_node;
	u32 val;
	int ret;

	/* reset the PCIe interface as uboot can leave it undefined state */
	reset_control_assert(res->pci_reset);
	reset_control_assert(res->axi_reset);
	reset_control_assert(res->ahb_reset);
	reset_control_assert(res->por_reset);
	reset_control_assert(res->ext_reset);
	reset_control_assert(res->phy_reset);

	writel(1, pcie->parf + PCIE20_PARF_PHY_CTRL);

	ret = regulator_bulk_enable(ARRAY_SIZE(res->supplies), res->supplies);
	if (ret < 0) {
		dev_err(dev, "cannot enable regulators\n");
		return ret;
	}

	ret = reset_control_assert(res->ahb_reset);
	if (ret) {
		dev_err(dev, "cannot assert ahb reset\n");
		goto err_assert_ahb;
	}

	ret = clk_prepare_enable(res->iface_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable iface clock\n");
		goto err_assert_ahb;
	}

	ret = clk_prepare_enable(res->core_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable core clock\n");
		goto err_clk_core;
	}

	ret = clk_prepare_enable(res->aux_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable aux clock\n");
		goto err_clk_aux;
	}

	ret = clk_prepare_enable(res->ref_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable ref clock\n");
		goto err_clk_ref;
	}

	ret = reset_control_deassert(res->ahb_reset);
	if (ret) {
		dev_err(dev, "cannot deassert ahb reset\n");
		goto err_deassert_ahb;
	}

	ret = reset_control_deassert(res->ext_reset);
	if (ret) {
		dev_err(dev, "cannot deassert ext reset\n");
		goto err_deassert_ahb;
	}

	/* enable PCIe clocks and resets */
	val = readl(pcie->parf + PCIE20_PARF_PHY_CTRL);
	val &= ~BIT(0);
	writel(val, pcie->parf + PCIE20_PARF_PHY_CTRL);

	if (of_device_is_compatible(node, "qcom,pcie-ipq8064")) {
		writel(PCS_DEEMPH_TX_DEEMPH_GEN1(24) |
			       PCS_DEEMPH_TX_DEEMPH_GEN2_3_5DB(24) |
			       PCS_DEEMPH_TX_DEEMPH_GEN2_6DB(34),
		       pcie->parf + PCIE20_PARF_PCS_DEEMPH);
		writel(PCS_SWING_TX_SWING_FULL(120) |
			       PCS_SWING_TX_SWING_LOW(120),
		       pcie->parf + PCIE20_PARF_PCS_SWING);
		writel(PHY_RX0_EQ(4), pcie->parf + PCIE20_PARF_CONFIG_BITS);
	}

	if (of_device_is_compatible(node, "qcom,pcie-ipq8064")) {
		/* set TX termination offset */
		val = readl(pcie->parf + PCIE20_PARF_PHY_CTRL);
		val &= ~PHY_CTRL_PHY_TX0_TERM_OFFSET_MASK;
		val |= PHY_CTRL_PHY_TX0_TERM_OFFSET(7);
		writel(val, pcie->parf + PCIE20_PARF_PHY_CTRL);
	}

	/* enable external reference clock */
	val = readl(pcie->parf + PCIE20_PARF_PHY_REFCLK);
	/* USE_PAD is required only for ipq806x */
	if (!of_device_is_compatible(node, "qcom,pcie-apq8064"))
		val &= ~PHY_REFCLK_USE_PAD;
	val |= PHY_REFCLK_SSP_EN;
	writel(val, pcie->parf + PCIE20_PARF_PHY_REFCLK);

	ret = reset_control_deassert(res->phy_reset);
	if (ret) {
		dev_err(dev, "cannot deassert phy reset\n");
		return ret;
	}

	ret = reset_control_deassert(res->pci_reset);
	if (ret) {
		dev_err(dev, "cannot deassert pci reset\n");
		return ret;
	}

	ret = reset_control_deassert(res->por_reset);
	if (ret) {
		dev_err(dev, "cannot deassert por reset\n");
		return ret;
	}

	ret = reset_control_deassert(res->axi_reset);
	if (ret) {
		dev_err(dev, "cannot deassert axi reset\n");
		return ret;
	}

	ret = clk_prepare_enable(res->phy_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable phy clock\n");
		goto err_deassert_ahb;
	}

	/* wait for clock acquisition */
	usleep_range(1000, 1500);


	/* Set the Max TLP size to 2K, instead of using default of 4K */
	writel(CFG_REMOTE_RD_REQ_BRIDGE_SIZE_2K,
	       pci->dbi_base + PCIE20_AXI_MSTR_RESP_COMP_CTRL0);
	writel(CFG_BRIDGE_SB_INIT,
	       pci->dbi_base + PCIE20_AXI_MSTR_RESP_COMP_CTRL1);

	return 0;

err_deassert_ahb:
	clk_disable_unprepare(res->ref_clk);
err_clk_ref:
	clk_disable_unprepare(res->aux_clk);
err_clk_aux:
	clk_disable_unprepare(res->core_clk);
err_clk_core:
	clk_disable_unprepare(res->iface_clk);
err_assert_ahb:
	regulator_bulk_disable(ARRAY_SIZE(res->supplies), res->supplies);

	return ret;
}

static int qcom_pcie_get_resources_1_0_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_1_0_0 *res = &pcie->res.v1_0_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;

	res->vdda = devm_regulator_get(dev, "vdda");
	if (IS_ERR(res->vdda))
		return PTR_ERR(res->vdda);

	res->iface = devm_clk_get(dev, "iface");
	if (IS_ERR(res->iface))
		return PTR_ERR(res->iface);

	res->aux = devm_clk_get(dev, "aux");
	if (IS_ERR(res->aux))
		return PTR_ERR(res->aux);

	res->master_bus = devm_clk_get(dev, "master_bus");
	if (IS_ERR(res->master_bus))
		return PTR_ERR(res->master_bus);

	res->slave_bus = devm_clk_get(dev, "slave_bus");
	if (IS_ERR(res->slave_bus))
		return PTR_ERR(res->slave_bus);

	res->core = devm_reset_control_get_exclusive(dev, "core");
	return PTR_ERR_OR_ZERO(res->core);
}

static void qcom_pcie_deinit_1_0_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_1_0_0 *res = &pcie->res.v1_0_0;

	reset_control_assert(res->core);
	clk_disable_unprepare(res->slave_bus);
	clk_disable_unprepare(res->master_bus);
	clk_disable_unprepare(res->iface);
	clk_disable_unprepare(res->aux);
	regulator_disable(res->vdda);
}

static int qcom_pcie_init_1_0_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_1_0_0 *res = &pcie->res.v1_0_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	int ret;

	ret = reset_control_deassert(res->core);
	if (ret) {
		dev_err(dev, "cannot deassert core reset\n");
		return ret;
	}

	ret = clk_prepare_enable(res->aux);
	if (ret) {
		dev_err(dev, "cannot prepare/enable aux clock\n");
		goto err_res;
	}

	ret = clk_prepare_enable(res->iface);
	if (ret) {
		dev_err(dev, "cannot prepare/enable iface clock\n");
		goto err_aux;
	}

	ret = clk_prepare_enable(res->master_bus);
	if (ret) {
		dev_err(dev, "cannot prepare/enable master_bus clock\n");
		goto err_iface;
	}

	ret = clk_prepare_enable(res->slave_bus);
	if (ret) {
		dev_err(dev, "cannot prepare/enable slave_bus clock\n");
		goto err_master;
	}

	ret = regulator_enable(res->vdda);
	if (ret) {
		dev_err(dev, "cannot enable vdda regulator\n");
		goto err_slave;
	}

	/* change DBI base address */
	writel(0, pcie->parf + PCIE20_PARF_DBI_BASE_ADDR);

	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		u32 val = readl(pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT);

		val |= BIT(31);
		writel(val, pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT);
	}

	return 0;
err_slave:
	clk_disable_unprepare(res->slave_bus);
err_master:
	clk_disable_unprepare(res->master_bus);
err_iface:
	clk_disable_unprepare(res->iface);
err_aux:
	clk_disable_unprepare(res->aux);
err_res:
	reset_control_assert(res->core);

	return ret;
}

static void qcom_pcie_2_3_2_ltssm_enable(struct qcom_pcie *pcie)
{
	u32 val;

	/* enable link training */
	val = readl(pcie->parf + PCIE20_PARF_LTSSM);
	val |= BIT(8);
	writel(val, pcie->parf + PCIE20_PARF_LTSSM);
}

static int qcom_pcie_get_resources_2_3_2(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_3_2 *res = &pcie->res.v2_3_2;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	int ret;

	res->supplies[0].supply = "vdda";
	res->supplies[1].supply = "vddpe-3v3";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(res->supplies),
				      res->supplies);
	if (ret)
		return ret;

	res->aux_clk = devm_clk_get(dev, "aux");
	if (IS_ERR(res->aux_clk))
		return PTR_ERR(res->aux_clk);

	res->cfg_clk = devm_clk_get(dev, "cfg");
	if (IS_ERR(res->cfg_clk))
		return PTR_ERR(res->cfg_clk);

	res->master_clk = devm_clk_get(dev, "bus_master");
	if (IS_ERR(res->master_clk))
		return PTR_ERR(res->master_clk);

	res->slave_clk = devm_clk_get(dev, "bus_slave");
	if (IS_ERR(res->slave_clk))
		return PTR_ERR(res->slave_clk);

	res->pipe_clk = devm_clk_get(dev, "pipe");
	return PTR_ERR_OR_ZERO(res->pipe_clk);
}

static void qcom_pcie_deinit_2_3_2(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_3_2 *res = &pcie->res.v2_3_2;

	clk_disable_unprepare(res->slave_clk);
	clk_disable_unprepare(res->master_clk);
	clk_disable_unprepare(res->cfg_clk);
	clk_disable_unprepare(res->aux_clk);

	regulator_bulk_disable(ARRAY_SIZE(res->supplies), res->supplies);
}

static void qcom_pcie_post_deinit_2_3_2(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_3_2 *res = &pcie->res.v2_3_2;

	clk_disable_unprepare(res->pipe_clk);
}

static int qcom_pcie_init_2_3_2(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_3_2 *res = &pcie->res.v2_3_2;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	u32 val;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(res->supplies), res->supplies);
	if (ret < 0) {
		dev_err(dev, "cannot enable regulators\n");
		return ret;
	}

	ret = clk_prepare_enable(res->aux_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable aux clock\n");
		goto err_aux_clk;
	}

	ret = clk_prepare_enable(res->cfg_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable cfg clock\n");
		goto err_cfg_clk;
	}

	ret = clk_prepare_enable(res->master_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable master clock\n");
		goto err_master_clk;
	}

	ret = clk_prepare_enable(res->slave_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable slave clock\n");
		goto err_slave_clk;
	}

	/* enable PCIe clocks and resets */
	val = readl(pcie->parf + PCIE20_PARF_PHY_CTRL);
	val &= ~BIT(0);
	writel(val, pcie->parf + PCIE20_PARF_PHY_CTRL);

	/* change DBI base address */
	writel(0, pcie->parf + PCIE20_PARF_DBI_BASE_ADDR);

	/* MAC PHY_POWERDOWN MUX DISABLE  */
	val = readl(pcie->parf + PCIE20_PARF_SYS_CTRL);
	val &= ~BIT(29);
	writel(val, pcie->parf + PCIE20_PARF_SYS_CTRL);

	val = readl(pcie->parf + PCIE20_PARF_MHI_CLOCK_RESET_CTRL);
	val |= BIT(4);
	writel(val, pcie->parf + PCIE20_PARF_MHI_CLOCK_RESET_CTRL);

	val = readl(pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2);
	val |= BIT(31);
	writel(val, pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2);

	return 0;

err_slave_clk:
	clk_disable_unprepare(res->master_clk);
err_master_clk:
	clk_disable_unprepare(res->cfg_clk);
err_cfg_clk:
	clk_disable_unprepare(res->aux_clk);

err_aux_clk:
	regulator_bulk_disable(ARRAY_SIZE(res->supplies), res->supplies);

	return ret;
}

static int qcom_pcie_post_init_2_3_2(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_3_2 *res = &pcie->res.v2_3_2;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	int ret;

	ret = clk_prepare_enable(res->pipe_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable pipe clock\n");
		return ret;
	}

	return 0;
}

static int qcom_pcie_get_resources_2_4_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_4_0 *res = &pcie->res.v2_4_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	bool is_ipq = of_device_is_compatible(dev->of_node, "qcom,pcie-ipq4019");
	int ret;

	res->clks[0].id = "aux";
	res->clks[1].id = "master_bus";
	res->clks[2].id = "slave_bus";
	res->clks[3].id = "iface";

	/* qcom,pcie-ipq4019 is defined without "iface" */
	res->num_clks = is_ipq ? 3 : 4;

	ret = devm_clk_bulk_get(dev, res->num_clks, res->clks);
	if (ret < 0)
		return ret;

	res->axi_m_reset = devm_reset_control_get_exclusive(dev, "axi_m");
	if (IS_ERR(res->axi_m_reset))
		return PTR_ERR(res->axi_m_reset);

	res->axi_s_reset = devm_reset_control_get_exclusive(dev, "axi_s");
	if (IS_ERR(res->axi_s_reset))
		return PTR_ERR(res->axi_s_reset);

	if (is_ipq) {
		/*
		 * These resources relates to the PHY or are secure clocks, but
		 * are controlled here for IPQ4019
		 */
		res->pipe_reset = devm_reset_control_get_exclusive(dev, "pipe");
		if (IS_ERR(res->pipe_reset))
			return PTR_ERR(res->pipe_reset);

		res->axi_m_vmid_reset = devm_reset_control_get_exclusive(dev,
									 "axi_m_vmid");
		if (IS_ERR(res->axi_m_vmid_reset))
			return PTR_ERR(res->axi_m_vmid_reset);

		res->axi_s_xpu_reset = devm_reset_control_get_exclusive(dev,
									"axi_s_xpu");
		if (IS_ERR(res->axi_s_xpu_reset))
			return PTR_ERR(res->axi_s_xpu_reset);

		res->parf_reset = devm_reset_control_get_exclusive(dev, "parf");
		if (IS_ERR(res->parf_reset))
			return PTR_ERR(res->parf_reset);

		res->phy_reset = devm_reset_control_get_exclusive(dev, "phy");
		if (IS_ERR(res->phy_reset))
			return PTR_ERR(res->phy_reset);
	}

	res->axi_m_sticky_reset = devm_reset_control_get_exclusive(dev,
								   "axi_m_sticky");
	if (IS_ERR(res->axi_m_sticky_reset))
		return PTR_ERR(res->axi_m_sticky_reset);

	res->pipe_sticky_reset = devm_reset_control_get_exclusive(dev,
								  "pipe_sticky");
	if (IS_ERR(res->pipe_sticky_reset))
		return PTR_ERR(res->pipe_sticky_reset);

	res->pwr_reset = devm_reset_control_get_exclusive(dev, "pwr");
	if (IS_ERR(res->pwr_reset))
		return PTR_ERR(res->pwr_reset);

	res->ahb_reset = devm_reset_control_get_exclusive(dev, "ahb");
	if (IS_ERR(res->ahb_reset))
		return PTR_ERR(res->ahb_reset);

	if (is_ipq) {
		res->phy_ahb_reset = devm_reset_control_get_exclusive(dev, "phy_ahb");
		if (IS_ERR(res->phy_ahb_reset))
			return PTR_ERR(res->phy_ahb_reset);
	}

	return 0;
}

static void qcom_pcie_deinit_2_4_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_4_0 *res = &pcie->res.v2_4_0;

	reset_control_assert(res->axi_m_reset);
	reset_control_assert(res->axi_s_reset);
	reset_control_assert(res->pipe_reset);
	reset_control_assert(res->pipe_sticky_reset);
	reset_control_assert(res->phy_reset);
	reset_control_assert(res->phy_ahb_reset);
	reset_control_assert(res->axi_m_sticky_reset);
	reset_control_assert(res->pwr_reset);
	reset_control_assert(res->ahb_reset);
	clk_bulk_disable_unprepare(res->num_clks, res->clks);
}

static int qcom_pcie_init_2_4_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_4_0 *res = &pcie->res.v2_4_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	u32 val;
	int ret;

	ret = reset_control_assert(res->axi_m_reset);
	if (ret) {
		dev_err(dev, "cannot assert axi master reset\n");
		return ret;
	}

	ret = reset_control_assert(res->axi_s_reset);
	if (ret) {
		dev_err(dev, "cannot assert axi slave reset\n");
		return ret;
	}

	usleep_range(10000, 12000);

	ret = reset_control_assert(res->pipe_reset);
	if (ret) {
		dev_err(dev, "cannot assert pipe reset\n");
		return ret;
	}

	ret = reset_control_assert(res->pipe_sticky_reset);
	if (ret) {
		dev_err(dev, "cannot assert pipe sticky reset\n");
		return ret;
	}

	ret = reset_control_assert(res->phy_reset);
	if (ret) {
		dev_err(dev, "cannot assert phy reset\n");
		return ret;
	}

	ret = reset_control_assert(res->phy_ahb_reset);
	if (ret) {
		dev_err(dev, "cannot assert phy ahb reset\n");
		return ret;
	}

	usleep_range(10000, 12000);

	ret = reset_control_assert(res->axi_m_sticky_reset);
	if (ret) {
		dev_err(dev, "cannot assert axi master sticky reset\n");
		return ret;
	}

	ret = reset_control_assert(res->pwr_reset);
	if (ret) {
		dev_err(dev, "cannot assert power reset\n");
		return ret;
	}

	ret = reset_control_assert(res->ahb_reset);
	if (ret) {
		dev_err(dev, "cannot assert ahb reset\n");
		return ret;
	}

	usleep_range(10000, 12000);

	ret = reset_control_deassert(res->phy_ahb_reset);
	if (ret) {
		dev_err(dev, "cannot deassert phy ahb reset\n");
		return ret;
	}

	ret = reset_control_deassert(res->phy_reset);
	if (ret) {
		dev_err(dev, "cannot deassert phy reset\n");
		goto err_rst_phy;
	}

	ret = reset_control_deassert(res->pipe_reset);
	if (ret) {
		dev_err(dev, "cannot deassert pipe reset\n");
		goto err_rst_pipe;
	}

	ret = reset_control_deassert(res->pipe_sticky_reset);
	if (ret) {
		dev_err(dev, "cannot deassert pipe sticky reset\n");
		goto err_rst_pipe_sticky;
	}

	usleep_range(10000, 12000);

	ret = reset_control_deassert(res->axi_m_reset);
	if (ret) {
		dev_err(dev, "cannot deassert axi master reset\n");
		goto err_rst_axi_m;
	}

	ret = reset_control_deassert(res->axi_m_sticky_reset);
	if (ret) {
		dev_err(dev, "cannot deassert axi master sticky reset\n");
		goto err_rst_axi_m_sticky;
	}

	ret = reset_control_deassert(res->axi_s_reset);
	if (ret) {
		dev_err(dev, "cannot deassert axi slave reset\n");
		goto err_rst_axi_s;
	}

	ret = reset_control_deassert(res->pwr_reset);
	if (ret) {
		dev_err(dev, "cannot deassert power reset\n");
		goto err_rst_pwr;
	}

	ret = reset_control_deassert(res->ahb_reset);
	if (ret) {
		dev_err(dev, "cannot deassert ahb reset\n");
		goto err_rst_ahb;
	}

	usleep_range(10000, 12000);

	ret = clk_bulk_prepare_enable(res->num_clks, res->clks);
	if (ret)
		goto err_clks;

	/* enable PCIe clocks and resets */
	val = readl(pcie->parf + PCIE20_PARF_PHY_CTRL);
	val &= ~BIT(0);
	writel(val, pcie->parf + PCIE20_PARF_PHY_CTRL);

	/* change DBI base address */
	writel(0, pcie->parf + PCIE20_PARF_DBI_BASE_ADDR);

	/* MAC PHY_POWERDOWN MUX DISABLE  */
	val = readl(pcie->parf + PCIE20_PARF_SYS_CTRL);
	val &= ~BIT(29);
	writel(val, pcie->parf + PCIE20_PARF_SYS_CTRL);

	val = readl(pcie->parf + PCIE20_PARF_MHI_CLOCK_RESET_CTRL);
	val |= BIT(4);
	writel(val, pcie->parf + PCIE20_PARF_MHI_CLOCK_RESET_CTRL);

	val = readl(pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2);
	val |= BIT(31);
	writel(val, pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2);

	return 0;

err_clks:
	reset_control_assert(res->ahb_reset);
err_rst_ahb:
	reset_control_assert(res->pwr_reset);
err_rst_pwr:
	reset_control_assert(res->axi_s_reset);
err_rst_axi_s:
	reset_control_assert(res->axi_m_sticky_reset);
err_rst_axi_m_sticky:
	reset_control_assert(res->axi_m_reset);
err_rst_axi_m:
	reset_control_assert(res->pipe_sticky_reset);
err_rst_pipe_sticky:
	reset_control_assert(res->pipe_reset);
err_rst_pipe:
	reset_control_assert(res->phy_reset);
err_rst_phy:
	reset_control_assert(res->phy_ahb_reset);
	return ret;
}


static void qcom_pcie_set_link_speed(void __iomem *dbi_base, u32 speed,
				     u32 max_supported_speed)
{
	/*
	 * Fallback to default speed for this controller if
	 * max-link-speed is mentioned as 0 or as negative value or
	 * as higher value than supported link speed for the
	 * controller.
	 *
	 * 2_5_0 -> max-link-speed supported is 2
	 * 2_9_0 -> max-link-speed supported is 3
	 */
	if (speed <= 0 || speed > max_supported_speed)
		speed = max_supported_speed;

	if (speed == SPEED_GEN3) {
		writel(PCIE_CAP_CURR_DEEMPHASIS | SPEED_GEN3,
			dbi_base + PCIE20_LNK_CONTROL2_LINK_STATUS2);
	} else if (speed == SPEED_GEN2) {
		writel(PCIE_CAP_CURR_DEEMPHASIS | SPEED_GEN2,
			dbi_base + PCIE20_LNK_CONTROL2_LINK_STATUS2);
	} else if (speed == SPEED_GEN1) {
		writel(((readl(
			dbi_base + PCIE20_LNK_CONTROL2_LINK_STATUS2)
			& (~PCIE_CAP_TARGET_LINK_SPEED_MASK)) | SPEED_GEN1),
			dbi_base + PCIE20_LNK_CONTROL2_LINK_STATUS2);
	}
}

static int qcom_pcie_get_resources_2_5_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_5_0 *res = &pcie->res.v2_5_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	int i;
	const char *rst_names[] = { "pipe", "sleep", "sticky", "axi_m",
				    "axi_s", "ahb", "axi_m_sticky", };

	res->iface = devm_clk_get(dev, "iface");
	if (IS_ERR(res->iface))
		return PTR_ERR(res->iface);

	res->axi_m_clk = devm_clk_get(dev, "axi_m");
	if (IS_ERR(res->axi_m_clk))
		return PTR_ERR(res->axi_m_clk);

	res->axi_s_clk = devm_clk_get(dev, "axi_s");
	if (IS_ERR(res->axi_s_clk))
		return PTR_ERR(res->axi_s_clk);

	for (i = 0; i < ARRAY_SIZE(rst_names); i++) {
		res->rst[i] = devm_reset_control_get(dev, rst_names[i]);
		if (IS_ERR(res->rst[i])) {
			return PTR_ERR(res->rst[i]);
		}
	}

	return 0;
}

static void qcom_pcie_deinit_2_5_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_5_0 *res = &pcie->res.v2_5_0;

	clk_disable_unprepare(res->axi_m_clk);
	clk_disable_unprepare(res->axi_s_clk);
	clk_disable_unprepare(res->iface);
}

static int qcom_pcie_init_2_5_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_5_0 *res = &pcie->res.v2_5_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(res->rst); i++) {
		ret = reset_control_assert(res->rst[i]);
		if (ret) {
			dev_err(dev, "reset #%d assert failed (%d)\n", i, ret);
			return ret;
		}
	}

	usleep_range(2000, 2500);

	for (i = 0; i < ARRAY_SIZE(res->rst); i++) {
		ret = reset_control_deassert(res->rst[i]);
		if (ret) {
			dev_err(dev, "reset #%d deassert failed (%d)\n", i,
				ret);
			return ret;
		}
	}

	/*
	 * Don't have a way to see if the reset has completed.
	 * Wait for some time.
	 */
	usleep_range(2000, 2500);

	ret = clk_prepare_enable(res->iface);
	if (ret) {
		dev_err(dev, "cannot prepare/enable core clock\n");
		goto err_clk_iface;
	}

	ret = clk_prepare_enable(res->axi_m_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable core clock\n");
		goto err_clk_axi_m;
	}

	ret = clk_set_rate(res->axi_m_clk, AXI_CLK_RATE);
	if (ret) {
		dev_err(dev, "MClk rate set failed (%d)\n", ret);
		goto err_clk_axi_m;
	}

	ret = clk_prepare_enable(res->axi_s_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable axi slave clock\n");
		goto err_clk_axi_s;
	}

	ret = clk_set_rate(res->axi_s_clk, AXI_CLK_RATE);
	if (ret) {
		dev_err(dev, "SClk rate set failed (%d)\n", ret);
		goto err_clk_axi_s;
	}

	return 0;

err_clk_axi_s:
	clk_disable_unprepare(res->axi_s_clk);
err_clk_axi_m:
	clk_disable_unprepare(res->axi_m_clk);
err_clk_iface:
	clk_disable_unprepare(res->iface);
	/*
	 * Not checking for failure, will anyway return
	 * the original failure in 'ret'.
	 */
	for (i = 0; i < ARRAY_SIZE(res->rst); i++)
		reset_control_assert(res->rst[i]);

	return ret;
}

static int qcom_pcie_post_init_2_5_0(struct qcom_pcie *pcie)
{
	struct dw_pcie *pci = pcie->pci;
	u32 val;

	writel(SLV_ADDR_SPACE_SZ,
		pcie->parf + PCIE20_v3_PARF_SLV_ADDR_SPACE_SIZE);

	val = readl(pcie->parf + PCIE20_PARF_PHY_CTRL);
	val &= ~BIT(0);
	writel(val, pcie->parf + PCIE20_PARF_PHY_CTRL);

	writel(0, pcie->parf + PCIE20_PARF_DBI_BASE_ADDR);

	writel(MST_WAKEUP_EN | SLV_WAKEUP_EN | MSTR_ACLK_CGC_DIS
		| SLV_ACLK_CGC_DIS | CORE_CLK_CGC_DIS |
		AUX_PWR_DET | L23_CLK_RMV_DIS | L1_CLK_RMV_DIS,
		pcie->parf + PCIE20_PARF_SYS_CTRL);

	writel(0, pcie->parf + PCIE20_PARF_Q2A_FLUSH);

	if (pcie->axi_wr_addr_halt) {
		val = readl(pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2);
		val &= ~PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2_MASK;
		writel(val | pcie->axi_wr_addr_halt,
			pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2);
	}

	writel(CMD_BME_VAL, pci->dbi_base + PCIE20_COMMAND_STATUS);

	writel(DBI_RO_WR_EN, pci->dbi_base + PCIE20_MISC_CONTROL_1_REG);
	writel(PCIE_CAP_LINK1_VAL, pci->dbi_base + PCIE20_CAP_LINK_1);

	/* Configure PCIe link capabilities for ASPM */
	val = readl(pci->dbi_base + PCIE20_CAP_LINK_CAPABILITIES);
	val &= ~PCIE20_CAP_ACTIVE_STATE_LINK_PM_SUPPORT;
	writel(val, pci->dbi_base + PCIE20_CAP_LINK_CAPABILITIES);

	writel(PCIE_CAP_CPL_TIMEOUT_DISABLE, pci->dbi_base +
		PCIE20_DEVICE_CONTROL2_STATUS2);

	qcom_pcie_set_link_speed(pci->dbi_base, pcie->max_speed, SPEED_GEN2);

	return 0;
}

static int qcom_pcie_get_resources_2_9_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_9_0 *res = &pcie->res.v2_9_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	int i;
	const char *rst_names[] = { "pipe", "sleep", "sticky",
				    "axi_m", "axi_s", "ahb",
				    "axi_m_sticky", "axi_s_sticky", };

	res->iface = devm_clk_get(dev, "iface");
	if (IS_ERR(res->iface))
		return PTR_ERR(res->iface);

	res->ahb_clk = devm_clk_get(dev, "ahb");
	if (IS_ERR(res->ahb_clk))
		res->ahb_clk = NULL;

	res->aux_clk = devm_clk_get(dev, "aux");
	if (IS_ERR(res->aux_clk))
		res->aux_clk = NULL;

	res->axi_m_clk = devm_clk_get(dev, "axi_m");
	if (IS_ERR(res->axi_m_clk))
		return PTR_ERR(res->axi_m_clk);

	res->axi_s_clk = devm_clk_get(dev, "axi_s");
	if (IS_ERR(res->axi_s_clk))
		return PTR_ERR(res->axi_s_clk);

	res->axi_bridge_clk = devm_clk_get(dev, "axi_bridge");
	if (IS_ERR(res->axi_bridge_clk))
		return PTR_ERR(res->axi_bridge_clk);

	res->rchng_clk = devm_clk_get(dev, "rchng");
	if (IS_ERR(res->rchng_clk))
		res->rchng_clk = NULL;

	for (i = 0; i < ARRAY_SIZE(rst_names); i++) {
		res->rst[i] = devm_reset_control_get(dev, rst_names[i]);
		if (IS_ERR(res->rst[i])) {
			return PTR_ERR(res->rst[i]);
		}
	}

	return 0;
}

static int qti_pcie_get_resources_2_9_0_9574(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_9_0 *res = &pcie->res.v2_9_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	int i;

	const char *rst_names[] = { "pipe", "sticky", "axi_s_sticky",
				    "axi_s", "axi_m_sticky", "axi_m",
				    "aux", "ahb" };

	res->ahb_clk = devm_clk_get(dev, "ahb");
	if (IS_ERR(res->ahb_clk))
		res->ahb_clk = NULL;

	res->aux_clk = devm_clk_get(dev, "aux");
	if (IS_ERR(res->aux_clk))
		res->aux_clk = NULL;

	res->axi_m_clk = devm_clk_get(dev, "axi_m");
	if (IS_ERR(res->axi_m_clk))
		return PTR_ERR(res->axi_m_clk);

	res->axi_s_clk = devm_clk_get(dev, "axi_s");
	if (IS_ERR(res->axi_s_clk))
		return PTR_ERR(res->axi_s_clk);

	res->axi_bridge_clk = devm_clk_get(dev, "axi_bridge");
	if (IS_ERR(res->axi_bridge_clk))
		return PTR_ERR(res->axi_bridge_clk);

	res->rchng_clk = devm_clk_get(dev, "rchng");
	if (IS_ERR(res->rchng_clk))
		res->rchng_clk = NULL;

	for (i = 0; i < ARRAY_SIZE(rst_names); i++) {
		res->rst[i] = devm_reset_control_get(dev, rst_names[i]);
		if (IS_ERR(res->rst[i])) {
			return PTR_ERR(res->rst[i]);
		}
	}

	return 0;
}

static void qcom_pcie_deinit_2_9_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_9_0 *res = &pcie->res.v2_9_0;

	clk_disable_unprepare(res->axi_m_clk);
	clk_disable_unprepare(res->axi_s_clk);
	clk_disable_unprepare(res->axi_bridge_clk);
	if (res->rchng_clk)
		clk_disable_unprepare(res->rchng_clk);
	clk_disable_unprepare(res->aux_clk);
	clk_disable_unprepare(res->ahb_clk);
	if (res->iface)
		clk_disable_unprepare(res->iface);
}

static int qcom_pcie_init_2_9_0_5018(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_9_0 *res = &pcie->res.v2_9_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	int i, ret;
	u32 aux_clk_rate;
	u32 axi_m_clk_rate = AXI_CLK_RATE;
	u32 axi_s_clk_rate = AXI_CLK_RATE;

	device_property_read_u32(dev, "axi-m-clk-rate", &axi_m_clk_rate);
	device_property_read_u32(dev, "axi-s-clk-rate", &axi_s_clk_rate);

	for (i = 0; i < ARRAY_SIZE(res->rst); i++) {
		ret = reset_control_assert(res->rst[i]);
		if (ret) {
			dev_err(dev, "reset #%d assert failed (%d)\n", i, ret);
			return ret;
		}
	}

	usleep_range(2000, 2500);

	for (i = 0; i < ARRAY_SIZE(res->rst); i++) {
		ret = reset_control_deassert(res->rst[i]);
		if (ret) {
			dev_err(dev, "reset #%d deassert failed (%d)\n", i,
				ret);
			return ret;
		}
	}

	/*
	 * Don't have a way to see if the reset has completed.
	 * Wait for some time.
	 */
	usleep_range(2000, 2500);

	if (res->iface) {
		ret = clk_prepare_enable(res->iface);
		if (ret) {
			dev_err(dev, "cannot prepare/enable core clock\n");
			goto err_clk_iface;
		}
	}

	ret = clk_prepare_enable(res->ahb_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable ahb clock\n");
		goto err_clk_ahb;
	}

	ret = clk_prepare_enable(res->aux_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable aux clock\n");
		goto err_clk_aux;
	}

	if (!device_property_read_u32(dev, "aux-clk-rate", &aux_clk_rate)) {
		ret = clk_set_rate(res->aux_clk, aux_clk_rate);
		if (ret) {
			dev_err(dev, "acx_clk rate set failed (%d)\n", ret);
			goto err_clk_aux;
		}
	}

	ret = clk_prepare_enable(res->axi_m_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable core clock\n");
		goto err_clk_axi_m;
	}

	ret = clk_set_rate(res->axi_m_clk, axi_m_clk_rate);
	if (ret) {
		dev_err(dev, "MClk rate set failed (%d)\n", ret);
		goto err_clk_axi_s;
	}

	ret = clk_prepare_enable(res->axi_s_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable axi slave clock\n");
		goto err_clk_axi_s;
	}

	ret = clk_set_rate(res->axi_s_clk, axi_s_clk_rate);
	if (ret) {
		dev_err(dev, "SClk rate set failed (%d)\n", ret);
		goto err_clk_axi_bridge;
	}

	ret = clk_prepare_enable(res->axi_bridge_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable axi bridge clock\n");
		goto err_clk_axi_bridge;
	}

	if (res->rchng_clk) {
		ret = clk_prepare_enable(res->rchng_clk);
		if (ret) {
			dev_err(dev, "cannot prepare/enable rchng clock\n");
			goto err_clk_rchng;
		}

		ret = clk_set_rate(res->rchng_clk, RCHNG_CLK_RATE);
		if (ret) {
			dev_err(dev, "rchng_clk rate set failed (%d)\n", ret);
			goto err_clk_rchng;
		}
	}

	if (pcie->slv_addr_space_sz)
		writel(pcie->slv_addr_space_sz,
			pcie->parf + PCIE20_v3_PARF_SLV_ADDR_SPACE_SIZE);
	else
		writel(SLV_ADDR_SPACE_SZ,
			pcie->parf + PCIE20_v3_PARF_SLV_ADDR_SPACE_SIZE);

	return 0;

	clk_disable_unprepare(res->rchng_clk);
err_clk_rchng:
	clk_disable_unprepare(res->axi_bridge_clk);
err_clk_axi_bridge:
	clk_disable_unprepare(res->axi_s_clk);
err_clk_axi_s:
	clk_disable_unprepare(res->axi_m_clk);
err_clk_axi_m:
	clk_disable_unprepare(res->aux_clk);
err_clk_aux:
	clk_disable_unprepare(res->ahb_clk);
err_clk_ahb:
	if (res->iface)
		clk_disable_unprepare(res->iface);
err_clk_iface:
	/*
	 * Not checking for failure, will anyway return
	 * the original failure in 'ret'.
	 */
	for (i = 0; i < ARRAY_SIZE(res->rst); i++)
		reset_control_assert(res->rst[i]);

	return ret;
}

static int qcom_pcie_post_init_2_9_0_5018(struct qcom_pcie *pcie)
{
	int i;
	u32 val;
	struct dw_pcie *pci = pcie->pci;
	u32 max_speed = SPEED_GEN2;
	u32 rate_adapter_val = 0;

	if (of_device_is_compatible(pci->dev->of_node, "qcom,pcie-ipq5018")) {
		if (pcie->num_lanes == 1)
			rate_adapter_val = SYSTEM_NOC_PCIE_RATEADAPT_VAL;
		else
			rate_adapter_val = SYSTEM_NOC_PCIE_RATEADAPT_VAL_MAX;
	}

	if (of_device_is_compatible(pci->dev->of_node, "qti,pcie-ipq5332")) {
		max_speed = SPEED_GEN3;
		if (pcie->num_lanes == 2)
			rate_adapter_val = SYSTEM_NOC_PCIE_RATEADAPT_BYPASS;
		/* For sigle lane, default value(0) to be used */
	}

	val = readl(pcie->parf + PCIE20_PARF_PHY_CTRL);
	val &= ~BIT(0);
	writel(val, pcie->parf + PCIE20_PARF_PHY_CTRL);

	writel(0, pcie->parf + PCIE20_PARF_DBI_BASE_ADDR);

	writel(DEVICE_TYPE_RC, pcie->parf + PCIE20_PARF_DEVICE_TYPE);
	writel(BYPASS | MSTR_AXI_CLK_EN | AHB_CLK_EN,
		pcie->parf + PCIE20_PARF_MHI_CLOCK_RESET_CTRL);
	writel(RXEQ_RGRDLESS_RXTS | GEN3_ZRXDC_NONCOMPL,
		pci->dbi_base + PCIE30_GEN3_RELATED_OFF);

	writel(MST_WAKEUP_EN | SLV_WAKEUP_EN | MSTR_ACLK_CGC_DIS
		| SLV_ACLK_CGC_DIS | CORE_CLK_CGC_DIS |
		AUX_PWR_DET | L23_CLK_RMV_DIS | L1_CLK_RMV_DIS,
		pcie->parf + PCIE20_PARF_SYS_CTRL);

	writel(0, pcie->parf + PCIE20_PARF_Q2A_FLUSH);

	if (pcie->axi_wr_addr_halt) {
		val = readl(pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2);
		val &= ~PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2_MASK;
		writel(val | pcie->axi_wr_addr_halt,
			pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2);
	}

	if (pcie->system_noc != NULL && !IS_ERR(pcie->system_noc))
		writel(rate_adapter_val, pcie->system_noc);

	writel(BUS_MASTER_EN, pci->dbi_base + PCIE20_COMMAND_STATUS);

	writel(DBI_RO_WR_EN, pci->dbi_base + PCIE20_MISC_CONTROL_1_REG);
	writel(PCIE_CAP_LINK1_VAL, pci->dbi_base + PCIE20_CAP_LINK_1);

	/* Configure PCIe link capabilities for ASPM */
	val = readl(pci->dbi_base + PCIE20_CAP_LINK_CAPABILITIES);
	val &= ~PCIE20_CAP_ACTIVE_STATE_LINK_PM_SUPPORT;
	writel(val, pci->dbi_base + PCIE20_CAP_LINK_CAPABILITIES);

	writel(PCIE_CAP_CPL_TIMEOUT_DISABLE, pci->dbi_base +
		PCIE20_DEVICE_CONTROL2_STATUS2);

	if (pcie->max_payload_size) {
		val = readl(pci->dbi_base + PCIE_DEVICE_CONTROL_STATUS);
		val &= ~PCIE_CAP_MAX_PAYLOAD_SIZE_CS_MASK;
		val |= PCIE_CAP_MAX_PAYLOAD_SIZE_CS_OFFSET(pcie->max_payload_size);
		writel(val, pci->dbi_base + PCIE_DEVICE_CONTROL_STATUS);
	}

	qcom_pcie_set_link_speed(pci->dbi_base, pcie->max_speed, max_speed);

	for (i = 0; i < 255; i++)
		writel(0x0, pcie->parf + PARF_BDF_TO_SID_TABLE + (4 * i));

	writel(0x4, pci->atu_base + PCIE_ATU_CR1_OUTBOUND_6_GEN3);
	writel(0x90000000, pci->atu_base + PCIE_ATU_CR2_OUTBOUND_6_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_LOWER_BASE_OUTBOUND_6_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_UPPER_BASE_OUTBOUND_6_GEN3);
	writel(0x00107FFFF, pci->atu_base + PCIE_ATU_LIMIT_OUTBOUND_6_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_LOWER_TARGET_OUTBOUND_6_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_UPPER_TARGET_OUTBOUND_6_GEN3);
	writel(0x5, pci->atu_base + PCIE_ATU_CR1_OUTBOUND_7_GEN3);
	writel(0x90000000, pci->atu_base + PCIE_ATU_CR2_OUTBOUND_7_GEN3);
	writel(0x200000, pci->atu_base + PCIE_ATU_LOWER_BASE_OUTBOUND_7_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_UPPER_BASE_OUTBOUND_7_GEN3);
	writel(0x7FFFFF, pci->atu_base + PCIE_ATU_LIMIT_OUTBOUND_7_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_LOWER_TARGET_OUTBOUND_7_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_UPPER_TARGET_OUTBOUND_7_GEN3);

	return 0;
}

static int qti_pcie_init_2_9_0_9574(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_9_0 *res = &pcie->res.v2_9_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(res->rst); i++) {
		ret = reset_control_assert(res->rst[i]);
		if (ret) {
			dev_err(dev, "reset #%d assert failed (%d)\n", i, ret);
			return ret;
		}
	}

	usleep_range(2000, 2500);

	for (i = 0; i < ARRAY_SIZE(res->rst); i++) {
		ret = reset_control_deassert(res->rst[i]);
		if (ret) {
			dev_err(dev, "reset #%d deassert failed (%d)\n", i,
				ret);
			return ret;
		}
	}

	/*
	 * Don't have a way to see if the reset has completed.
	 * Wait for some time.
	 */
	usleep_range(2000, 2500);

	ret = clk_prepare_enable(res->ahb_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable ahb clock\n");
		goto err_clk_ahb;
	}

	ret = clk_prepare_enable(res->aux_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable aux clock\n");
		goto err_clk_aux;
	}

	ret = clk_prepare_enable(res->axi_m_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable core clock\n");
		goto err_clk_axi_m;
	}

	if (pcie->num_lanes == 1)
		ret = clk_set_rate(res->axi_m_clk, AXI_CLK_RATE_IPQ9574);
	else if (pcie->num_lanes == 2)
		ret = clk_set_rate(res->axi_m_clk, AXI_M_2LANE_CLK_RATE_IPQ9574);

	if (ret) {
		dev_err(dev, "MClk rate set failed (%d)\n", ret);
		goto err_clk_axi_m;
	}

	ret = clk_prepare_enable(res->axi_s_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable axi slave clock\n");
		goto err_clk_axi_s;
	}

	ret = clk_set_rate(res->axi_s_clk, AXI_CLK_RATE_IPQ9574);
	if (ret) {
		dev_err(dev, "SClk rate set failed (%d)\n", ret);
		goto err_clk_axi_s;
	}

	ret = clk_prepare_enable(res->axi_bridge_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable axi bridge clock\n");
		goto err_clk_axi_bridge;
	}

	if (res->rchng_clk) {
		ret = clk_prepare_enable(res->rchng_clk);
		if (ret) {
			dev_err(dev, "cannot prepare/enable rchng clock\n");
			goto err_clk_rchng;
		}

		ret = clk_set_rate(res->rchng_clk, RCHNG_CLK_RATE);
		if (ret) {
			dev_err(dev, "rchng_clk rate set failed (%d)\n", ret);
			goto err_clk_rchng;
		}
	}

	return 0;

err_clk_rchng:
	clk_disable_unprepare(res->rchng_clk);
err_clk_axi_bridge:
	clk_disable_unprepare(res->axi_bridge_clk);
err_clk_axi_s:
	clk_disable_unprepare(res->axi_s_clk);
err_clk_axi_m:
	clk_disable_unprepare(res->axi_m_clk);
err_clk_aux:
	clk_disable_unprepare(res->aux_clk);
err_clk_ahb:
	clk_disable_unprepare(res->ahb_clk);
	/*
	 * Not checking for failure, will anyway return
	 * the original failure in 'ret'.
	 */
	for (i = 0; i < ARRAY_SIZE(res->rst); i++)
		reset_control_assert(res->rst[i]);

	return ret;
}

static int qti_pcie_post_init_2_9_0_9574(struct qcom_pcie *pcie)
{
	struct dw_pcie *pci = pcie->pci;
	u32 val;
	int i;

	if (pcie->slv_addr_space_sz)
		writel(pcie->slv_addr_space_sz,
			pcie->parf + PCIE20_v3_PARF_SLV_ADDR_SPACE_SIZE);
	else
		writel(SLV_ADDR_SPACE_SZ,
			pcie->parf + PCIE20_v3_PARF_SLV_ADDR_SPACE_SIZE);

	val = readl(pcie->parf + PCIE20_PARF_PHY_CTRL);
	val &= ~BIT(0);
	writel(val, pcie->parf + PCIE20_PARF_PHY_CTRL);

	writel(0, pcie->parf + PCIE20_PARF_DBI_BASE_ADDR);

	writel(DEVICE_TYPE_RC, pcie->parf + PCIE20_PARF_DEVICE_TYPE);
	writel(BYPASS | MSTR_AXI_CLK_EN | AHB_CLK_EN,
		pcie->parf + PCIE20_PARF_MHI_CLOCK_RESET_CTRL);
	writel(RXEQ_RGRDLESS_RXTS | GEN3_ZRXDC_NONCOMPL,
		pci->dbi_base + PCIE30_GEN3_RELATED_OFF);

	writel(MST_WAKEUP_EN | SLV_WAKEUP_EN | MSTR_ACLK_CGC_DIS
		| SLV_ACLK_CGC_DIS | CORE_CLK_CGC_DIS |
		AUX_PWR_DET | L23_CLK_RMV_DIS | L1_CLK_RMV_DIS,
		pcie->parf + PCIE20_PARF_SYS_CTRL);

	writel(0, pcie->parf + PCIE20_PARF_Q2A_FLUSH);

	if (pcie->axi_wr_addr_halt) {
		val = readl(pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2);
		val &= ~PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2_MASK;
		writel(val | pcie->axi_wr_addr_halt,
			pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2);
	}

	if (pcie->aggr_noc != NULL && !IS_ERR(pcie->aggr_noc))
		writel(AGGR_NOC_PCIE_1LANE_RATEADAPT_VAL, pcie->aggr_noc);

	writel(BUS_MASTER_EN, pci->dbi_base + PCIE20_COMMAND_STATUS);

	writel(DBI_RO_WR_EN, pci->dbi_base + PCIE20_MISC_CONTROL_1_REG);
	writel(PCIE_CAP_LINK1_VAL, pci->dbi_base + PCIE20_CAP_LINK_1);

	/* Configure PCIe link capabilities for ASPM */
	val = readl(pci->dbi_base + PCIE20_CAP_LINK_CAPABILITIES);
	val &= ~PCIE20_CAP_ACTIVE_STATE_LINK_PM_SUPPORT;
	writel(val, pci->dbi_base + PCIE20_CAP_LINK_CAPABILITIES);

	writel(PCIE_CAP_CPL_TIMEOUT_DISABLE, pci->dbi_base +
		PCIE20_DEVICE_CONTROL2_STATUS2);

	if (pcie->max_payload_size) {
		val = readl(pci->dbi_base + PCIE_DEVICE_CONTROL_STATUS);
		val &= ~PCIE_CAP_MAX_PAYLOAD_SIZE_CS_MASK;
		val |= PCIE_CAP_MAX_PAYLOAD_SIZE_CS_OFFSET(pcie->max_payload_size);
		writel(val, pci->dbi_base + PCIE_DEVICE_CONTROL_STATUS);
	}

	qcom_pcie_set_link_speed(pci->dbi_base, pcie->max_speed, SPEED_GEN3);

	for (i = 0;i < 256;i++)
		writel(0x0, pcie->parf + PARF_BDF_TO_SID_TABLE + (4 * i));

	writel(0x4, pci->atu_base + PCIE_ATU_CR1_OUTBOUND_6_GEN3);
	writel(0x90000000, pci->atu_base + PCIE_ATU_CR2_OUTBOUND_6_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_LOWER_BASE_OUTBOUND_6_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_UPPER_BASE_OUTBOUND_6_GEN3);
	writel(0x00107FFFF, pci->atu_base + PCIE_ATU_LIMIT_OUTBOUND_6_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_LOWER_TARGET_OUTBOUND_6_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_UPPER_TARGET_OUTBOUND_6_GEN3);
	writel(0x5, pci->atu_base + PCIE_ATU_CR1_OUTBOUND_7_GEN3);
	writel(0x90000000, pci->atu_base + PCIE_ATU_CR2_OUTBOUND_7_GEN3);
	writel(0x200000, pci->atu_base + PCIE_ATU_LOWER_BASE_OUTBOUND_7_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_UPPER_BASE_OUTBOUND_7_GEN3);
	writel(0x7FFFFF, pci->atu_base + PCIE_ATU_LIMIT_OUTBOUND_7_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_LOWER_TARGET_OUTBOUND_7_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_UPPER_TARGET_OUTBOUND_7_GEN3);

	return 0;
}

static int qcom_pcie_init_2_9_0(struct qcom_pcie *pcie)
{
	struct qcom_pcie_resources_2_9_0 *res = &pcie->res.v2_9_0;
	struct dw_pcie *pci = pcie->pci;
	struct device *dev = pci->dev;
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(res->rst); i++) {
		ret = reset_control_assert(res->rst[i]);
		if (ret) {
			dev_err(dev, "reset #%d assert failed (%d)\n", i, ret);
			return ret;
		}
	}

	usleep_range(2000, 2500);

	for (i = 0; i < ARRAY_SIZE(res->rst); i++) {
		ret = reset_control_deassert(res->rst[i]);
		if (ret) {
			dev_err(dev, "reset #%d deassert failed (%d)\n", i,
				ret);
			return ret;
		}
	}

	/*
	 * Don't have a way to see if the reset has completed.
	 * Wait for some time.
	 */
	usleep_range(2000, 2500);

	if (res->iface) {
		ret = clk_prepare_enable(res->iface);
		if (ret) {
			dev_err(dev, "cannot prepare/enable core clock\n");
			goto err_clk_iface;
		}
	}

	ret = clk_prepare_enable(res->ahb_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable ahb clock\n");
		goto err_clk_ahb;
	}

	ret = clk_prepare_enable(res->aux_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable aux clock\n");
		goto err_clk_aux;
	}

	ret = clk_prepare_enable(res->axi_m_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable core clock\n");
		goto err_clk_axi_m;
	}

	ret = clk_set_rate(res->axi_m_clk, AXI_CLK_RATE);
	if (ret) {
		dev_err(dev, "MClk rate set failed (%d)\n", ret);
		goto err_clk_axi_m;
	}

	ret = clk_prepare_enable(res->axi_s_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable axi slave clock\n");
		goto err_clk_axi_s;
	}

	ret = clk_set_rate(res->axi_s_clk, AXI_CLK_RATE);
	if (ret) {
		dev_err(dev, "SClk rate set failed (%d)\n", ret);
		goto err_clk_axi_s;
	}

	ret = clk_prepare_enable(res->axi_bridge_clk);
	if (ret) {
		dev_err(dev, "cannot prepare/enable axi bridge clock\n");
		goto err_clk_axi_bridge;
	}

	if (res->rchng_clk) {
		ret = clk_prepare_enable(res->rchng_clk);
		if (ret) {
			dev_err(dev, "cannot prepare/enable rchng clock\n");
			goto err_clk_rchng;
		}

		ret = clk_set_rate(res->rchng_clk, RCHNG_CLK_RATE);
		if (ret) {
			dev_err(dev, "rchng_clk rate set failed (%d)\n", ret);
			goto err_clk_rchng;
		}
	}

	return 0;

err_clk_rchng:
	clk_disable_unprepare(res->rchng_clk);
err_clk_axi_bridge:
	clk_disable_unprepare(res->axi_bridge_clk);
err_clk_axi_s:
	clk_disable_unprepare(res->axi_s_clk);
err_clk_axi_m:
	clk_disable_unprepare(res->axi_m_clk);
err_clk_aux:
	clk_disable_unprepare(res->aux_clk);
err_clk_ahb:
	clk_disable_unprepare(res->ahb_clk);
err_clk_iface:
	if (res->iface)
		clk_disable_unprepare(res->iface);
	/*
	 * Not checking for failure, will anyway return
	 * the original failure in 'ret'.
	 */
	for (i = 0; i < ARRAY_SIZE(res->rst); i++)
		reset_control_assert(res->rst[i]);

	return ret;
}

static int qcom_pcie_post_init_2_9_0(struct qcom_pcie *pcie)
{
	struct dw_pcie *pci = pcie->pci;
	u32 val;
	int i;

	if (pcie->slv_addr_space_sz)
		writel(pcie->slv_addr_space_sz,
			pcie->parf + PCIE20_v3_PARF_SLV_ADDR_SPACE_SIZE);
	else
		writel(SLV_ADDR_SPACE_SZ,
			pcie->parf + PCIE20_v3_PARF_SLV_ADDR_SPACE_SIZE);

	val = readl(pcie->parf + PCIE20_PARF_PHY_CTRL);
	val &= ~BIT(0);
	writel(val, pcie->parf + PCIE20_PARF_PHY_CTRL);

	writel(0, pcie->parf + PCIE20_PARF_DBI_BASE_ADDR);

	writel(DEVICE_TYPE_RC, pcie->parf + PCIE20_PARF_DEVICE_TYPE);
	writel(BYPASS | MSTR_AXI_CLK_EN | AHB_CLK_EN,
		pcie->parf + PCIE20_PARF_MHI_CLOCK_RESET_CTRL);
	writel(RXEQ_RGRDLESS_RXTS | GEN3_ZRXDC_NONCOMPL,
		pci->dbi_base + PCIE30_GEN3_RELATED_OFF);

	writel(MST_WAKEUP_EN | SLV_WAKEUP_EN | MSTR_ACLK_CGC_DIS
		| SLV_ACLK_CGC_DIS | CORE_CLK_CGC_DIS |
		AUX_PWR_DET | L23_CLK_RMV_DIS | L1_CLK_RMV_DIS,
		pcie->parf + PCIE20_PARF_SYS_CTRL);

	writel(0, pcie->parf + PCIE20_PARF_Q2A_FLUSH);

	if (pcie->axi_wr_addr_halt) {
		val = readl(pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2);
		val &= ~PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2_MASK;
		writel(val | pcie->axi_wr_addr_halt,
			pcie->parf + PCIE20_PARF_AXI_MSTR_WR_ADDR_HALT_V2);
	}

	writel(BUS_MASTER_EN, pci->dbi_base + PCIE20_COMMAND_STATUS);

	writel(DBI_RO_WR_EN, pci->dbi_base + PCIE20_MISC_CONTROL_1_REG);
	writel(PCIE_CAP_LINK1_VAL, pci->dbi_base + PCIE20_CAP_LINK_1);

	/* Configure PCIe link capabilities for ASPM */
	val = readl(pci->dbi_base + PCIE20_CAP_LINK_CAPABILITIES);
	val &= ~PCIE20_CAP_ACTIVE_STATE_LINK_PM_SUPPORT;
	writel(val, pci->dbi_base + PCIE20_CAP_LINK_CAPABILITIES);

	writel(PCIE_CAP_CPL_TIMEOUT_DISABLE, pci->dbi_base +
		PCIE20_DEVICE_CONTROL2_STATUS2);

	qcom_pcie_set_link_speed(pci->dbi_base, pcie->max_speed, SPEED_GEN3);

	for (i = 0;i < 256;i++)
		writel(0x0, pcie->parf + PARF_BDF_TO_SID_TABLE + (4 * i));

	writel(0x4, pci->atu_base + PCIE_ATU_CR1_OUTBOUND_6_GEN3);
	writel(0x90000000, pci->atu_base + PCIE_ATU_CR2_OUTBOUND_6_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_LOWER_BASE_OUTBOUND_6_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_UPPER_BASE_OUTBOUND_6_GEN3);
	writel(0x00107FFFF, pci->atu_base + PCIE_ATU_LIMIT_OUTBOUND_6_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_LOWER_TARGET_OUTBOUND_6_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_UPPER_TARGET_OUTBOUND_6_GEN3);
	writel(0x5, pci->atu_base + PCIE_ATU_CR1_OUTBOUND_7_GEN3);
	writel(0x90000000, pci->atu_base + PCIE_ATU_CR2_OUTBOUND_7_GEN3);
	writel(0x200000, pci->atu_base + PCIE_ATU_LOWER_BASE_OUTBOUND_7_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_UPPER_BASE_OUTBOUND_7_GEN3);
	writel(0x7FFFFF, pci->atu_base + PCIE_ATU_LIMIT_OUTBOUND_7_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_LOWER_TARGET_OUTBOUND_7_GEN3);
	writel(0x0, pci->atu_base + PCIE_ATU_UPPER_TARGET_OUTBOUND_7_GEN3);

	return 0;
}

static int qcom_pcie_link_up(struct dw_pcie *pci)
{
	u16 val = readw(pci->dbi_base + PCIE20_CAP + PCI_EXP_LNKSTA);

	return !!(val & PCI_EXP_LNKSTA_DLLLA);
}

static void qcom_pcie_ltssm_read(struct dw_pcie *pci, u32 *val)
{
	struct qcom_pcie *pcie;

	pcie = to_qcom_pcie(pci);

	*val = readl(pcie->parf + PCIE20_PARF_LTSSM);
}

static int qcom_pcie_host_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct qcom_pcie *pcie = to_qcom_pcie(pci);
	int ret, retry = 0;

	if (gpiod_get_value(mdm2ap_e911))
		return -EBUSY;

retry_link:
	if (retry >= QCOM_PCIE_MAX_RETRY)
		return ret;

	if (retry) {
		dev_info(pci->dev, "Enumeration retry\n");
		ret = phy_init(pcie->phy);
		if (ret) {
			dev_info(pci->dev, "PHY init failed on retry\n");
			return ret;
		}
		msleep(100);
	}

	qcom_ep_reset_assert(pcie);

	ret = pcie->ops->init(pcie);
	if (ret)
		return ret;

	ret = phy_power_on(pcie->phy);
	if (ret)
		goto err_deinit;

	if (pcie->ops->post_init) {
		ret = pcie->ops->post_init(pcie);
		if (ret)
			goto err_disable_phy;
	}

	dw_pcie_setup_rc(pp);

	qcom_ep_reset_deassert(pcie);

	ret = qcom_pcie_establish_link(pcie);
	if (ret)
		goto err;

	return 0;
err:
	if (pcie->compliance == 1)
		return 0;

	qcom_ep_reset_assert(pcie);
	if (pcie->ops->post_deinit)
		pcie->ops->post_deinit(pcie);
err_disable_phy:
	phy_power_off(pcie->phy);
err_deinit:
	if (pcie->compliance == 1)
		return 0;

	pcie->ops->deinit(pcie);

	if (pcie->enable_retry) {
		phy_exit(pcie->phy);
		retry++;
		goto retry_link;
	}

	return ret;
}

static const struct dw_pcie_host_ops qcom_pcie_dw_ops = {
	.host_init = qcom_pcie_host_init,
};

/* Qcom IP rev.: 2.1.0	Synopsys IP rev.: 4.01a */
static const struct qcom_pcie_ops ops_2_1_0 = {
	.get_resources = qcom_pcie_get_resources_2_1_0,
	.init = qcom_pcie_init_2_1_0,
	.deinit = qcom_pcie_deinit_2_1_0,
	.ltssm_enable = qcom_pcie_2_1_0_ltssm_enable,
};

static const struct qcom_pcie_of_data qcom_pcie_2_1_0 = {
	.ops = &ops_2_1_0,
	.version = 0x401A,
};

/* Qcom IP rev.: 1.0.0	Synopsys IP rev.: 4.11a */
static const struct qcom_pcie_ops ops_1_0_0 = {
	.get_resources = qcom_pcie_get_resources_1_0_0,
	.init = qcom_pcie_init_1_0_0,
	.deinit = qcom_pcie_deinit_1_0_0,
	.ltssm_enable = qcom_pcie_2_1_0_ltssm_enable,
};

static const struct qcom_pcie_of_data qcom_pcie_1_0_0 = {
	.ops = &ops_1_0_0,
	.version = 0x411A,
};

/* Qcom IP rev.: 2.3.2	Synopsys IP rev.: 4.21a */
static const struct qcom_pcie_ops ops_2_3_2 = {
	.get_resources = qcom_pcie_get_resources_2_3_2,
	.init = qcom_pcie_init_2_3_2,
	.post_init = qcom_pcie_post_init_2_3_2,
	.deinit = qcom_pcie_deinit_2_3_2,
	.post_deinit = qcom_pcie_post_deinit_2_3_2,
	.ltssm_enable = qcom_pcie_2_3_2_ltssm_enable,
};

static const struct qcom_pcie_of_data qcom_pcie_2_3_2 = {
	.ops = &ops_2_3_2,
	.version = 0x421A,
};

/* Qcom IP rev.: 2.4.0	Synopsys IP rev.: 4.20a */
static const struct qcom_pcie_ops ops_2_4_0 = {
	.get_resources = qcom_pcie_get_resources_2_4_0,
	.init = qcom_pcie_init_2_4_0,
	.deinit = qcom_pcie_deinit_2_4_0,
	.ltssm_enable = qcom_pcie_2_3_2_ltssm_enable,
};

static const struct qcom_pcie_of_data qcom_pcie_2_4_0 = {
	.ops = &ops_2_4_0,
	.version = 0x420A,
};

/* Qcom IP rev.: 2.5.0	Synopsys IP rev.: 4.30a */
static const struct qcom_pcie_ops ops_2_5_0 = {
	.get_resources = qcom_pcie_get_resources_2_5_0,
	.init = qcom_pcie_init_2_5_0,
	.post_init = qcom_pcie_post_init_2_5_0,
	.deinit = qcom_pcie_deinit_2_5_0,
	.ltssm_enable = qcom_pcie_2_3_2_ltssm_enable,
};

static const struct qcom_pcie_of_data qcom_pcie_2_5_0 = {
	.ops = &ops_2_5_0,
	.version = 0x430A,
};

/* Qcom IP rev.: 2.9.0	Synopsys IP rev.: 5.00a */
static const struct qcom_pcie_ops ops_2_9_0 = {
	.get_resources = qcom_pcie_get_resources_2_9_0,
	.init = qcom_pcie_init_2_9_0,
	.post_init = qcom_pcie_post_init_2_9_0,
	.deinit = qcom_pcie_deinit_2_9_0,
	.ltssm_enable = qcom_pcie_2_3_2_ltssm_enable,
};

/* QTI IP rev.: 2.9.0	Synopsys IP rev.: 5.00a */
static const struct qcom_pcie_ops ops_2_9_0_ipq5018 = {
	.get_resources = qcom_pcie_get_resources_2_9_0,
	.init = qcom_pcie_init_2_9_0_5018,
	.post_init = qcom_pcie_post_init_2_9_0_5018,
	.deinit = qcom_pcie_deinit_2_9_0,
	.ltssm_enable = qcom_pcie_2_3_2_ltssm_enable,
};

static const struct qcom_pcie_ops ops_2_9_0_ipq5332 = {
	.get_resources = qti_pcie_get_resources_2_9_0_9574,
	.init = qcom_pcie_init_2_9_0_5018,
	.post_init = qcom_pcie_post_init_2_9_0_5018,
	.deinit = qcom_pcie_deinit_2_9_0,
	.ltssm_enable = qcom_pcie_2_3_2_ltssm_enable,
};

/* QTI IP rev.: 2.9.0	Synopsys IP rev.: 5.00a */
static const struct qcom_pcie_ops ops_2_9_0_ipq9574 = {
	.get_resources = qti_pcie_get_resources_2_9_0_9574,
	.init = qti_pcie_init_2_9_0_9574,
	.post_init = qti_pcie_post_init_2_9_0_9574,
	.deinit = qcom_pcie_deinit_2_9_0,
	.ltssm_enable = qcom_pcie_2_3_2_ltssm_enable,
};

static const struct qcom_pcie_of_data qcom_pcie_2_9_0 = {
	.ops = &ops_2_9_0,
	.version = 0x500A,
};

static const struct qcom_pcie_of_data qcom_pcie_2_9_0_ipq5018 = {
	.ops = &ops_2_9_0_ipq5018,
	.version = 0x500A,
};

static const struct qcom_pcie_of_data qti_pcie_2_9_0_ipq5332 = {
	.ops = &ops_2_9_0_ipq5332,
	.version = 0x500A,
};

static const struct qcom_pcie_of_data qti_pcie_2_9_0_ipq9574 = {
	.ops = &ops_2_9_0_ipq9574,
	.version = 0x500A,
};

static const struct dw_pcie_ops qti_dw_pcie_ops = {
	.link_up = qcom_pcie_link_up,
	.ltssm_read = qcom_pcie_ltssm_read,
};

int pcie_rescan(void)
{
	int ret;
	struct pcie_port *pp;
	struct qcom_pcie *pcie;
	struct qcom_pcie_info *pcie_info, *tmp;

	if (!list_empty(&qcom_pcie_list)) {
		list_for_each_entry_safe(pcie_info, tmp, &qcom_pcie_list, node) {
			pcie = pcie_info->pcie;
		/* reset and enumerate the pcie devices */
			if (pcie) {
				pr_notice("---> Initializing %d\n", pcie->rc_idx);
				if (pcie->enumerated)
					continue;

				pp = &(pcie->pci)->pp;
				ret = pci_create_scan_root_bus(pp);
				if (!ret)
					pcie->enumerated = true;
				pr_notice(" ... done<---\n");
			}
		}
	}
	return 0;
}

void pcie_remove_bus(void)
{
	struct pcie_port *pp;
	struct qcom_pcie *pcie;
	struct qcom_pcie_info *pcie_info, *tmp;

	if (!list_empty(&qcom_pcie_list)) {
		list_for_each_entry_safe_reverse(pcie_info, tmp, &qcom_pcie_list, node) {
			pcie = pcie_info->pcie;
			if (pcie) {
				pr_notice("---> Removing %d\n", pcie->rc_idx);
				if (!pcie->enumerated)
					continue;

				pp = &(pcie->pci)->pp;
				pci_stop_root_bus(pp->root_bus);
				pci_remove_root_bus(pp->root_bus);

				qcom_ep_reset_assert(pcie);
				phy_power_off(pcie->phy);

				pcie->ops->deinit(pcie);
				pp->root_bus = NULL;
				pcie->enumerated = false;
				pr_notice(" ... done<---\n");
			}
		}
	}
}

static ssize_t rcrescan_store(struct bus_type *bus, const char *buf,
					size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 0, &val) < 0)
		return -EINVAL;

	if (gpiod_get_value(mdm2ap_e911))
		return -EBUSY;

	if (val) {
		pci_lock_rescan_remove();
		pr_debug("rcrescan from sysfs\n");
		pcie_rescan();
		pci_unlock_rescan_remove();
	}
	return count;
}
static BUS_ATTR_WO(rcrescan);

static ssize_t rcremove_store(struct bus_type *bus, const char *buf,
					size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 0, &val) < 0)
		return -EINVAL;

	if (val) {
		pci_lock_rescan_remove();
		pr_debug("rcremove from sysfs\n");
		pcie_remove_bus();
		pci_unlock_rescan_remove();
	}
	return count;
}
static BUS_ATTR_WO(rcremove);

static void pcie_slot_remove(int val)
{
	struct pcie_port *pp;
	struct qcom_pcie *pcie;
	struct qcom_pcie_info *pcie_info, *tmp;

	pci_lock_rescan_remove();

	if (!list_empty(&qcom_pcie_list)) {
		list_for_each_entry_safe(pcie_info, tmp, &qcom_pcie_list, node) {
			pcie = pcie_info->pcie;
			if (pcie && (pcie->rc_idx == val)) {
				if (!pcie->enumerated) {
					pr_notice("\nPCIe: RC%d already removed\n", val);
				}
				else {
					pr_notice("---> Removing %d\n", val);
					pp = &(pcie->pci)->pp;
					pci_stop_root_bus(pp->root_bus);
					pci_remove_root_bus(pp->root_bus);

					qcom_ep_reset_assert(pcie);
					phy_power_off(pcie->phy);

					pcie->ops->deinit(pcie);
					pp->root_bus = NULL;
					pcie->enumerated = false;
					pr_notice(" ... done<---\n");
				}
			}
		}
	}
	pci_unlock_rescan_remove();
}

static void pcie_slot_rescan(int val)
{
	struct pcie_port *pp;
	struct qcom_pcie *pcie;
	struct qcom_pcie_info *pcie_info, *tmp;
	int ret;

	pci_lock_rescan_remove();

	if (!list_empty(&qcom_pcie_list)) {
		list_for_each_entry_safe(pcie_info, tmp, &qcom_pcie_list, node) {
			pcie = pcie_info->pcie;
			if (pcie && (pcie->rc_idx == val)) {
				if (pcie->enumerated) {
					pr_notice("PCIe: RC%d already enumerated\n", val);
				} else {
					pp = &(pcie->pci)->pp;
					ret = pci_create_scan_root_bus(pp);
					if (!ret)
						pcie->enumerated = true;
				}
			}
		}
	}
	pci_unlock_rescan_remove();
}

static ssize_t slot_rescan_store(struct bus_type *bus, const char *buf,
		size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 0, &val) < 0)
		return -EINVAL;

	pr_debug("Slot rescan from sysfs\n");
	pcie_slot_rescan(val);

	return count;
}
static BUS_ATTR_WO(slot_rescan);

static ssize_t slot_remove_store(struct bus_type *bus, const char *buf,
		size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 0, &val) < 0)
		return -EINVAL;

	pr_debug("Slot remove from sysfs\n");
	pcie_slot_remove(val);

	return count;
}
static BUS_ATTR_WO(slot_remove);

static void handle_e911_func(struct work_struct *work)
{
	int slot_id;
	struct qcom_pcie *pcie;

	pcie = container_of(work, struct qcom_pcie, handle_e911_work);
	slot_id = pcie->slot_id;

	if (gpiod_get_value(mdm2ap_e911)) {
		pr_debug("E911 call ON\n");
		pcie_slot_remove(slot_id);
	} else {
		pr_debug("E911 call OFF\n");
		pcie_slot_rescan(slot_id);
	}
}

static irqreturn_t handle_mdm2ap_e911_irq(int irq, void *data)
{
	struct qcom_pcie *pcie = data;

	schedule_work(&pcie->handle_e911_work);

	return IRQ_HANDLED;
}

static int pci_reboot_handler(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	pci_lock_rescan_remove();

	pcie_remove_bus();

	pci_unlock_rescan_remove();

	return 0;
}

int pcie_parf_read(struct pci_dev *dev, u32 offset, u32 *val)
{
	struct pcie_port *pp;
	struct dw_pcie *pci;
	struct qcom_pcie *pcie;

	if(!dev)
		goto err;

	pp = dev->bus->sysdata;
	pci = to_dw_pcie_from_pp(pp);
	pcie = to_qcom_pcie(pci);

	if (offset > pcie->parf_size ||
			!IS_ALIGNED((uintptr_t)pcie->parf + offset, 4))
		goto err;

	*val = readl(pcie->parf + offset);

	return 0;

err:
	*val = 0;
	return -EINVAL;
}
EXPORT_SYMBOL(pcie_parf_read);

int pcie_set_link_speed(struct pci_dev *dev, u16 target_link_speed)
{
	struct pcie_port *pp;
	struct dw_pcie *pci;
	u32 val;

	if (dev->device != QCOM_IPQ9574_DEVICE_ID && dev->device != QCOM_IPQ5332_DEVICE_ID)
		return -EINVAL;

	if (target_link_speed < 1 || target_link_speed > 3)
		return -EINVAL;

	pp = dev->bus->sysdata;
	pci = to_dw_pcie_from_pp(pp);

	val = readl(pci->dbi_base + PCIE20_LNK_CONTROL2_LINK_STATUS2);
	val &= ~PCIE_CAP_TARGET_LINK_SPD_MASK;
	val |= target_link_speed;
	writel(val, pci->dbi_base + PCIE20_LNK_CONTROL2_LINK_STATUS2);

	val = readl(pci->dbi_base + PCIE20_LINK_CONTROL_LINK_STATUS_REG);
	val &= ~PCIE_CAP_RETRAIN_LINK_MASK;
	val |= PCIE_CAP_RETRAIN_LINK_OFFSET(0x1);
	writel(val, pci->dbi_base + PCIE20_LINK_CONTROL_LINK_STATUS_REG);

	msleep(1);

	val = readl(pci->dbi_base + PCIE20_LINK_CONTROL_LINK_STATUS_REG);
	val &= PCIE_CAP_LINK_SPEED_MASK;
	if (PCIE_CAP_LINK_SPEED_SHIFT(val) != target_link_speed) {
		dev_err(pci->dev, "Speed change failed. Current speed 0x%x\n",
			PCIE_CAP_LINK_SPEED_SHIFT(val));
		return -EAGAIN;
	}

	return 0;
}
EXPORT_SYMBOL(pcie_set_link_speed);

int pcie_set_link_width(struct pci_dev *dev, u16 target_link_width)
{
	struct pcie_port *pp;
	struct dw_pcie *pci;
	struct qcom_pcie *pcie;
	u32 val;

	if (dev->device != QCOM_IPQ9574_DEVICE_ID && dev->device != QCOM_IPQ5332_DEVICE_ID)
		return -EINVAL;

	if (target_link_width < 1 || target_link_width > 2)
		return -EINVAL;

	pp = dev->bus->sysdata;
	pci = to_dw_pcie_from_pp(pp);
	pcie = to_qcom_pcie(pci);

	dw_pcie_read(pci->dbi_base + PCIE20_PORT_LINK_CTRL_OFF, 4, &val);
	val &= ~PCIE20_PORT_LINK_CTRL_OFF_MASK;
	val |= LINK_CAPABLE_OFFSET(target_link_width);
	dw_pcie_write(pci->dbi_base + PCIE20_PORT_LINK_CTRL_OFF, 4, val);

	dw_pcie_read(pci->dbi_base + PCIE20_LANE_SKEW_OFF, 4, &val);
	val = (val & PCIE20_LANE_SKEW_OFF_MASK) | 0x20;
	dw_pcie_write(pci->dbi_base + PCIE20_LANE_SKEW_OFF, 4, val);

	val = 0xc0 | target_link_width;
	dw_pcie_write(pci->dbi_base + PCIE20_MULTI_LANE_CONTROL_OFF, 4, val);

	msleep(50);

	dw_pcie_read(pci->dbi_base + PCIE20_MULTI_LANE_CONTROL_OFF, 4, &val);
	if((val & 0x40) != 0) {
		dev_err(pci->dev, "LANE_CONTROL_OFF failed: val 0x%x\n", val);
		return -EAGAIN;
	}

	dw_pcie_read(pcie->parf + PCIE20_PARF_LTSSM, 4, &val);
	if ((val & PCIE20_PARF_LTSSM_MASK) != 0x11) {
		dev_err(pci->dev, "After lane switch, link is not in L0: val 0x%x\n", val);
		return -EAGAIN;
	}

	val = readl(pci->dbi_base + PCIE20_LINK_CONTROL_LINK_STATUS_REG);
	val &= PCIE_CAP_NEGO_LINK_WIDTH_MASK;
	if (PCIE_CAP_NEGO_LINK_WIDTH_SHIFT(val) != target_link_width) {
		dev_err(pci->dev, "Lane switch failed: 0x%x\n", val);
		return -EAGAIN;
	}

	return 0;
}
EXPORT_SYMBOL(pcie_set_link_width);

static int qcom_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct pcie_port *pp;
	struct dw_pcie *pci;
	struct qcom_pcie *pcie;
	struct qcom_pcie_of_data *data;
	struct qcom_pcie_info *pcie_info;
	int soc_version_major;
	int ret;
	u32 link_retries_count = 0;
	uint32_t slv_addr_space_sz = 0;
	uint32_t num_lanes = 0;
	uint32_t compliance = 0;
	uint32_t slot_id = -1;
	static int rc_idx;
	struct nvmem_cell *pcie_nvmem;
	u8 *disable_status;
	size_t len;
	int x65_attached = 0;

	/* If nvmem-cells present on PCIe node in DTSI, then check the QFPROM
	 * fuses for PCIe is disabled */
	pcie_nvmem = of_nvmem_cell_get(pdev->dev.of_node, NULL);
	if (IS_ERR(pcie_nvmem)) {
		if (PTR_ERR(pcie_nvmem) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
	} else {
		disable_status = nvmem_cell_read(pcie_nvmem, &len);
		nvmem_cell_put(pcie_nvmem);
		if ( !IS_ERR(disable_status) && ((unsigned int)(*disable_status) == 1) ) {
			dev_info(dev,"Disabled in qfprom efuse\n");
			kfree(disable_status);
			return -ENODEV;
		}
		kfree(disable_status);
	}

	pcie = devm_kzalloc(dev, sizeof(*pcie), GFP_KERNEL);
	if (!pcie)
		return -ENOMEM;

	pci = devm_kzalloc(dev, sizeof(*pci), GFP_KERNEL);
	if (!pci)
		return -ENOMEM;

	pm_runtime_enable(dev);
	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		pm_runtime_disable(dev);
		return ret;
	}

	pci->dev = dev;
	pci->ops = &qti_dw_pcie_ops;
	pp = &pci->pp;

	pcie->pci = pci;

	pcie->max_speed = of_pci_get_max_link_speed(pdev->dev.of_node);

	data = (struct qcom_pcie_of_data *)(of_device_get_match_data(dev));
	if (!data)
		return -EINVAL;

	pcie->ops = data->ops;
	pci->version = data->version;

	pcie->reset = devm_gpiod_get_optional(dev, "perst", GPIOD_OUT_HIGH);
	if (IS_ERR(pcie->reset)) {
		ret = PTR_ERR(pcie->reset);
		goto err_pm_runtime_put;
	}

	of_property_read_u32(pdev->dev.of_node, "slot_id", &slot_id);
	pcie->slot_id = slot_id;

	of_property_read_u32(pdev->dev.of_node, "compliance", &compliance);
	pcie->compliance = compliance;

	of_property_read_u32(pdev->dev.of_node, "link_retries_count",
			     &link_retries_count);
	pci->link_retries_count = link_retries_count;

	of_property_read_u32(pdev->dev.of_node, "slv-addr-space-sz",
			     &slv_addr_space_sz);
	pcie->slv_addr_space_sz = slv_addr_space_sz;

	of_property_read_u32(pdev->dev.of_node, "num-lanes",
			     &num_lanes);
	pcie->num_lanes = num_lanes;

	of_property_read_u32(pdev->dev.of_node, "axi-halt-val",
			     &pcie->axi_wr_addr_halt);

	of_property_read_u32(pdev->dev.of_node, "linux,pci-domain",&pcie->domain);

	of_property_read_u32(pdev->dev.of_node, "max-payload-size",
			     &pcie->max_payload_size);

	pcie->enable_vc = of_property_read_bool(pdev->dev.of_node,
					"enable-virtual-channel");

	pcie->enable_retry = of_property_read_bool(pdev->dev.of_node,
						   "enable-enum-retry");

	if (of_device_is_compatible(pdev->dev.of_node,
					"qcom,pcie-gen3-ipq8074")) {
		/*
		 * ipq8074 has 2 pcie ports. pcie port 1 is a gen3 port in
		 * ipq8074 V2 and is a gen2 port in ipq8074 v1. Here we will
		 * probe accordingly based on soc_version_major. Same DTS is
		 * used for both V1 and V2 and so both gen2 and gen3 phys are
		 * enabled by default for port 1 in DTS. pcie port 2 is a
		 * gen2 port in both V1 and V2.
		 */
		soc_version_major = read_ipq_soc_version_major();
		BUG_ON(soc_version_major <= 0);
		if (soc_version_major == 2) {
			pcie->phy = devm_phy_optional_get(dev, "pciephy-gen3");
			if (IS_ERR(pcie->phy)) {
				ret = PTR_ERR(pcie->phy);
				goto err_pm_runtime_put;
			}
		} else if (soc_version_major == 1) {
			/*
			 * Probe the pcie port as gen2 port if it is ipq8074 V1
			 * since there are no gen3 ports in ipq8074 V1. The QCOM
			 * IP core version for pcie gen2 ports in ipq8074 V1 is
			 * 2.3.3 and its configuration matches with gen2 port in
			 * ipq8074 V2 whose QCOM IP core version for pcie gen2
			 * port is 2.5.0.
			 */
			pcie->phy = devm_phy_optional_get(dev, "pciephy-gen2");
			if (IS_ERR(pcie->phy)) {
				ret = PTR_ERR(pcie->phy);
				goto err_pm_runtime_put;
			}
			pci->version = 0x430A;
			pcie->ops = &ops_2_5_0;
		} else {
			dev_err(dev, "missing phy-names\n");
			ret = -EIO;
			goto err_pm_runtime_put;
		}
	} else {
		pcie->phy = devm_phy_optional_get(dev, "pciephy");
		if (IS_ERR(pcie->phy)) {
			ret = PTR_ERR(pcie->phy);
			goto err_pm_runtime_put;
		}
	}

	if (pci->version >= 0x480A) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						   "atu");
		pci->atu_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(pci->atu_base)) {
			ret = PTR_ERR(pci->atu_base);
			goto err_pm_runtime_put;
		}

		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "parf");
		if (!res)
			goto err_pm_runtime_put;
		else
			res->end += PCIE_V2_PARF_SIZE;
		pcie->parf = devm_ioremap_resource(dev, res);
		if (IS_ERR(pcie->parf)) {
			ret = PTR_ERR(pcie->parf);
			goto err_pm_runtime_put;
		}
	} else {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "parf");
		pcie->parf = devm_ioremap_resource(dev, res);
		if (IS_ERR(pcie->parf)) {
			ret = PTR_ERR(pcie->parf);
			goto err_pm_runtime_put;
		}
	}

	/* get the parf size which is needed for pcie_parf_read() */
	if (res)
		pcie->parf_size = resource_size(res);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dbi");
	pci->dbi_base = devm_pci_remap_cfg_resource(dev, res);
	if (IS_ERR(pci->dbi_base)) {
		ret = PTR_ERR(pci->dbi_base);
		goto err_pm_runtime_put;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "elbi");
	pcie->elbi = devm_ioremap_resource(dev, res);
	if (IS_ERR(pcie->elbi)) {
		ret = PTR_ERR(pcie->elbi);
		goto err_pm_runtime_put;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "aggr_noc");
	if (res != NULL) {
		pcie->aggr_noc = devm_ioremap_resource(dev, res);
		if (IS_ERR(pcie->aggr_noc)) {
			ret = PTR_ERR(pcie->aggr_noc);
			goto err_pm_runtime_put;
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "system_noc");
	if (res != NULL) {
		pcie->system_noc = devm_ioremap_resource(dev, res);
		if (IS_ERR(pcie->system_noc)) {
			ret = PTR_ERR(pcie->system_noc);
			goto err_pm_runtime_put;
		}
	}

	ret = pcie->ops->get_resources(pcie);
	if (ret)
		goto err_pm_runtime_put;

	pp->ops = &qcom_pcie_dw_ops;

	of_property_read_u32(pdev->dev.of_node, "x65_attached", &x65_attached);

	if (x65_attached) {
		pcie->mdm2ap_e911_irq = platform_get_irq_byname(pdev,
							"mdm2ap_e911_x65");
	} else {
		pcie->mdm2ap_e911_irq = platform_get_irq_byname(pdev,
							"mdm2ap_e911");
	}

	if (pcie->mdm2ap_e911_irq >= 0) {
		mdm2ap_e911 = devm_gpiod_get_optional(&pdev->dev,
				(x65_attached ? "e911_x65" : "e911"), GPIOD_IN);
		if (IS_ERR(mdm2ap_e911)) {
			pr_err("requesting for e911 gpio failed %ld\n",
			       PTR_ERR(mdm2ap_e911));
			return PTR_ERR(mdm2ap_e911);
		}

		INIT_WORK(&pcie->handle_e911_work, handle_e911_func);

		ret = devm_request_irq(&pdev->dev, pcie->mdm2ap_e911_irq,
				       handle_mdm2ap_e911_irq,
				       IRQ_TYPE_EDGE_BOTH, "mdm2ap_e911",
				       pcie);
		if (ret) {
			dev_err(&pdev->dev, "Unable to request mdm2ap_e911 irq\n");
			return ret;
		}

		pcie->pci_reboot_notifier.notifier_call = pci_reboot_handler;
		ret = register_reboot_notifier(&pcie->pci_reboot_notifier);
		if (ret) {
			pr_warn("%s: Failed to register notifier (%d)\n",
				__func__, ret);
			return ret;
		}
	}

	ret = phy_init(pcie->phy);
	if (ret)
		goto err_pm_runtime_put;

	platform_set_drvdata(pdev, pcie);

	pcie->wake_irq = platform_get_irq_byname_optional(pdev, "wake_gpio");
	if (pcie->wake_irq >= 0) {
		INIT_WORK(&pcie->handle_wake_work, handle_wake_func);
		ret = devm_request_irq(&pdev->dev, pcie->wake_irq,
				       qcom_pcie_wake_irq_handler,
				       IRQF_TRIGGER_FALLING, "qcom-pcie-wake",
				       pcie);
		if (ret) {
			dev_err(&pdev->dev, "Unable to request wake irq\n");
			pm_runtime_disable(&pdev->dev);
			goto err_phy_exit;
		}
	}

	ret = dw_pcie_host_init(pp);
	if (ret) {
		if (pcie->wake_irq < 0 || pcie->mdm2ap_e911_irq < 0) {
			dev_err(dev, "cannot initialize host\n");
			pm_runtime_disable(&pdev->dev);
			goto err_phy_exit;
		}
		pr_info("[%s] PCIe: RC%d is not enabled during bootup: "
			"It will be enumerated upon client request\n", __func__, rc_idx);

	} else {
		pcie->enumerated = true;
		pr_info("[%s ] PCIe: RC enabled during bootup\n", __func__);
	}

	pcie->global_irq = platform_get_irq_byname(pdev, "global_irq");
	if (pcie->global_irq >= 0) {
		ret = devm_request_threaded_irq(&pdev->dev, pcie->global_irq,
					NULL,
					qcom_pcie_global_irq_thread_fn,
					IRQF_TRIGGER_RISING | IRQF_ONESHOT,
					"pcie-global", pcie);
		if (ret) {
			dev_err(&pdev->dev, "Unable to request global irq\n");
			pm_runtime_disable(&pdev->dev);
			goto err_phy_exit;
		}
	}

	if (!rc_idx) {
		ret = bus_create_file(&pci_bus_type, &bus_attr_rcrescan);
		if (ret != 0) {
			dev_err(&pdev->dev,
				"Failed to create sysfs rcrescan file\n");
			return ret;
		}

		ret = bus_create_file(&pci_bus_type, &bus_attr_rcremove);
		if (ret != 0) {
			dev_err(&pdev->dev,
				"Failed to create sysfs rcremove file\n");
			return ret;
		}

		ret = bus_create_file(&pci_bus_type, &bus_attr_slot_rescan);
		if (ret != 0) {
			dev_err(&pdev->dev,
					"Failed to create sysfs rcrescan file\n");
			return ret;
		}

		ret = bus_create_file(&pci_bus_type, &bus_attr_slot_remove);
		if (ret != 0) {
			dev_err(&pdev->dev,
					"Failed to create sysfs rcremove file\n");
			return ret;
		}
	}

	pcie->rc_idx = rc_idx;
	rc_idx++;

	/* Add qcom_pcie pointer to the list */
	pcie_info = kzalloc(sizeof(struct qcom_pcie_info), GFP_KERNEL);
	if (pcie_info) {
		pcie_info->pcie = pcie;
		list_add_tail(&pcie_info->node, &qcom_pcie_list);
	}

	return 0;

err_phy_exit:
	phy_exit(pcie->phy);
err_pm_runtime_put:
	pm_runtime_put(dev);
	pm_runtime_disable(dev);

	return ret;
}

static const struct of_device_id qcom_pcie_match[] = {
	{ .compatible = "qcom,pcie-apq8084", .data = &qcom_pcie_1_0_0 },
	{ .compatible = "qcom,pcie-ipq8064", .data = &qcom_pcie_2_1_0 },
	{ .compatible = "qcom,pcie-apq8064", .data = &qcom_pcie_2_1_0 },
	{ .compatible = "qcom,pcie-msm8996", .data = &qcom_pcie_2_3_2 },
	{ .compatible = "qcom,pcie-ipq8074", .data = &qcom_pcie_2_5_0 },
	{ .compatible = "qcom,pcie-gen3-ipq8074", .data = &qcom_pcie_2_9_0 },
	{ .compatible = "qcom,pcie-ipq6018", .data = &qcom_pcie_2_9_0},
	{ .compatible = "qcom,pcie-ipq4019", .data = &qcom_pcie_2_4_0 },
	{ .compatible = "qcom,pcie-qcs404", .data = &qcom_pcie_2_4_0 },
	{ .compatible = "qcom,pcie-ipq5018", .data = &qcom_pcie_2_9_0_ipq5018 },
	{ .compatible = "qti,pcie-ipq5332", .data = &qti_pcie_2_9_0_ipq5332 },
	{ .compatible = "qti,pcie-ipq9574", .data = &qti_pcie_2_9_0_ipq9574 },
	{ }
};

static void qcom_fixup_class(struct pci_dev *dev)
{
	dev->class = PCI_CLASS_BRIDGE_PCI << 8;
}

static void qcom_ipq_tc_vc_mapping(struct pci_dev *dev)
{
	struct pcie_port *pp;
	struct dw_pcie *pci;
	struct qcom_pcie *pcie;
	int timeout = 50;
	u32 val;

	pp = dev->bus->sysdata;
	pci = to_dw_pcie_from_pp(pp);
	pcie = to_qcom_pcie(pci);

	if (!pcie->enable_vc)
		return;

	dev_dbg(&dev->dev, "Enabling Virtual channel for 0x%x:0x%x\n",dev->vendor, dev->device);

	/* Read device VC capabilities */
	pci_read_config_dword(dev, PCIE_TYPE0_VC_CAPABILITIES_REG_1, &val);
	if((val & VC_EXT_VC_CNT_MASK) != 0x1) {
		dev_err(&dev->dev,"device 0x%x does not support Virtual Channel\n", dev->device);
		return;
	}

	/* Program Q6 VC */
	pci_read_config_dword(dev, PCIE_TYPE0_RESOURCE_CON_REG_VC0, &val);
	val &= ~VC_TC_MAP_VC_BIT1_MASK;
	val |= VC_TC_MAP_VC_BIT1_OFFSET(0x0);
	pci_write_config_dword(dev, PCIE_TYPE0_RESOURCE_CON_REG_VC0, val);

	pci_read_config_dword(dev, PCIE_TYPE0_RESOURCE_CON_REG_VC1, &val);
	val &= ~VC_TC_MAP_VC_BIT1_MASK;
	val |= VC_TC_MAP_VC_BIT1_OFFSET(0x7F);
	pci_write_config_dword(dev, PCIE_TYPE0_RESOURCE_CON_REG_VC1, val);

	pci_read_config_dword(dev, PCIE_TYPE0_RESOURCE_CON_REG_VC1, &val);
	val &= ~VC_ID_VC1_MASK;
	val |= VC_ID_VC1_OFFSET(0x1);
	pci_write_config_dword(dev, PCIE_TYPE0_RESOURCE_CON_REG_VC1, val);

	pci_read_config_dword(dev, PCIE_TYPE0_RESOURCE_CON_REG_VC1, &val);
	val &= ~VC_ENABLE_VC1_MASK;
	val |= VC_ENABLE_VC1_OFFSET(0x1);
	pci_write_config_dword(dev, PCIE_TYPE0_RESOURCE_CON_REG_VC1, val);

	/* Program Host VC */
	val = readl(pci->dbi_base + PCIE_TYPE0_RESOURCE_CON_REG_VC0);
	val &= ~VC_TC_MAP_VC_BIT1_MASK;
	val |= VC_TC_MAP_VC_BIT1_OFFSET(0x0);
	writel(val, pci->dbi_base + PCIE_TYPE0_RESOURCE_CON_REG_VC0);

	val = readl(pci->dbi_base + PCIE_TYPE0_RESOURCE_CON_REG_VC1);
	val &= ~VC_TC_MAP_VC_BIT1_MASK;
	val |= VC_TC_MAP_VC_BIT1_OFFSET(0x7F);
	writel(val, pci->dbi_base + PCIE_TYPE0_RESOURCE_CON_REG_VC1);

	val = readl(pci->dbi_base + PCIE_TYPE0_RESOURCE_CON_REG_VC1);
	val &= ~VC_ID_VC1_MASK;
	val |= VC_ID_VC1_OFFSET(0x1);
	writel(val, pci->dbi_base + PCIE_TYPE0_RESOURCE_CON_REG_VC1);

	val = readl(pci->dbi_base + PCIE_TYPE0_RESOURCE_CON_REG_VC1);
	val &= ~VC_ENABLE_VC1_MASK;
	val |= VC_ENABLE_VC1_OFFSET(0x1);
	writel(val, pci->dbi_base + PCIE_TYPE0_RESOURCE_CON_REG_VC1);

	do {
		/* Poll for negotiation */
		val = readl(pci->dbi_base + PCIE_TYPE0_RESOURCE_STATUS_REG_VC1);
		if(!(val & VC_NEGO_PENDING_VC1_MASK)) {
			dev_info(&dev->dev,"Virtual channel is enabled for 0x%x:0x%x\n",
					dev->vendor, dev->device);
			break;
		}
		timeout--;
	}while(timeout);
}

static void qcom_ipq_switch_lane(struct pci_dev *dev)
{
	struct pcie_port *pp;
	struct dw_pcie *pci;
        struct qcom_pcie *pcie;
	struct device *devp;
        struct device_node *np;

	u32 val;
	int size = 4;
	pp = dev->bus->sysdata;
        pci = to_dw_pcie_from_pp(pp);
        pcie = to_qcom_pcie(pci);
	devp = pci->dev;
	np = devp->of_node;

	/* Switching PCIE Nodes 2/3 to single lane if force_to_single_lane property is defined in dts */
	if ((of_property_read_bool(np, "force_to_single_lane")) && (pcie->domain == 3 || pcie->domain == 4)) {

		dev_info(devp,"Forcing PCIE to single lane\n");

		/* check if Link is in L0 state */
		dw_pcie_read(pcie->parf + PCIE20_PARF_LTSSM, size, &val);
		if ((val & PCIE20_PARF_LTSSM_MASK) != 0x11)
			dev_info(devp,"Before lane switch: Link is not in L0 state: %u\n",val);

		/* set link width */
		dw_pcie_read(pci->dbi_base + PCIE20_PORT_LINK_CTRL_OFF, size, &val);
		val &= ~(PCIE20_PORT_LINK_CTRL_OFF_MASK);
		val |= 0x10000;
		dw_pcie_write(pci->dbi_base + PCIE20_PORT_LINK_CTRL_OFF, size, val);

		/* config lane skew */
		dw_pcie_read(pci->dbi_base + PCIE20_LANE_SKEW_OFF, size, &val);
		val = (val & PCIE20_LANE_SKEW_OFF_MASK)|0x20;
		dw_pcie_write(pci->dbi_base + PCIE20_LANE_SKEW_OFF, size, val);

		/* set target lane width & direct link width change */
		dw_pcie_read(pci->dbi_base + PCIE20_MULTI_LANE_CONTROL_OFF, size, &val);
		val |= 0xc1;
		dw_pcie_write(pci->dbi_base + PCIE20_MULTI_LANE_CONTROL_OFF, size, val);

		/* wait until the link width change is complete */
		dw_pcie_read(pci->dbi_base + PCIE20_MULTI_LANE_CONTROL_OFF, size, &val);
		mdelay(50);
		dw_pcie_read(pci->dbi_base + PCIE20_MULTI_LANE_CONTROL_OFF, size, &val);

		/* check if Link is in L0 state */
		dw_pcie_read(pcie->parf + PCIE20_PARF_LTSSM, size, &val);
		if ((val & PCIE20_PARF_LTSSM_MASK) != 0x11)
			dev_info(devp,"After lane switch: Link is not in L0 state: %u\n",val);

		dw_pcie_read(pci->dbi_base + PCIE20_LINK_CONTROL_LINK_STATUS_REG, size, &val);

		dev_info(devp,"Link width is: %u\n",(val&0x3F00000)>>20);
	}

}

DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_QCOM, 0x0101, qcom_fixup_class);
DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_QCOM, 0x0104, qcom_fixup_class);
DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_QCOM, 0x0106, qcom_fixup_class);
DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_QCOM, 0x0107, qcom_fixup_class);
DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_QCOM, 0x0302, qcom_fixup_class);
DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_QCOM, 0x1000, qcom_fixup_class);
DECLARE_PCI_FIXUP_EARLY(PCI_VENDOR_ID_QCOM, 0x1001, qcom_fixup_class);
DECLARE_PCI_FIXUP_FINAL(PCI_VENDOR_ID_QCOM, 0x1109, qcom_ipq_tc_vc_mapping);
DECLARE_PCI_FIXUP_FINAL(PCI_VENDOR_ID_QCOM, 0x1108, qcom_ipq_switch_lane);

static struct platform_driver qcom_pcie_driver = {
	.probe = qcom_pcie_probe,
	.driver = {
		.name = "qcom-pcie",
		.suppress_bind_attrs = true,
		.of_match_table = qcom_pcie_match,
	},
};
builtin_platform_driver(qcom_pcie_driver);
