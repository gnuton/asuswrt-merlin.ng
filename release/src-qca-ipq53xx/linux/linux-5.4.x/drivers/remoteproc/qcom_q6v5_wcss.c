// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016-2018 Linaro Ltd.
 * Copyright (C) 2014 Sony Mobile Communications AB
 * Copyright (c) 2012-2018, The Linux Foundation. All rights reserved.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>
#include <linux/soc/qcom/mdt_loader.h>
#include <linux/qcom_scm.h>
#include <linux/soc/qcom/smem.h>
#ifdef CONFIG_IPQ_SUBSYSTEM_RAMDUMP
#include <soc/qcom/ramdump.h>
#endif
#include "qcom_common.h"
#include "qcom_q6v5.h"

#define WCSS_CRASH_REASON		421
#define WCSS_SMEM_HOST			1

#define Q6_BOOT_TRIG_SVC_ID		0x5
#define Q6_BOOT_TRIG_CMD_ID		0x2

/* Q6SS Register Offsets */
#define Q6SS_RESET_REG			0x014
#define Q6SS_DBG_CFG                    0x018
#define Q6SS_GFMUX_CTL_REG		0x020
#define Q6SS_PWR_CTL_REG		0x030
#define Q6SS_MEM_PWR_CTL		0x0B0
#define Q6SS_STRAP_ACC			0x110
#define Q6SS_CGC_OVERRIDE		0x034
#define Q6SS_BCR_REG			0x6000
#define Q6SS_BOOT_CORE_START		0x400
#define Q6SS_BOOT_CMD                   0x404
#define Q6SS_BOOT_STATUS		0x408
#define Q6SS_AHB_UPPER			0x104
#define Q6SS_AHB_LOWER			0x108

/* AXI Halt Register Offsets */
#define AXI_HALTREQ_REG			0x0
#define AXI_HALTACK_REG			0x4
#define AXI_IDLE_REG			0x8

#define HALT_ACK_TIMEOUT_MS		100

/* Q6SS_RESET */
#define Q6SS_STOP_CORE			BIT(0)
#define Q6SS_CORE_ARES			BIT(1)
#define Q6SS_BUS_ARES_ENABLE		BIT(2)

/* Q6SS_BRC_RESET */
#define Q6SS_BRC_BLK_ARES		BIT(0)

/* Q6SS_GFMUX_CTL */
#define Q6SS_CLK_ENABLE			BIT(1)
#define Q6SS_SWITCH_CLK_SRC		BIT(8)

/* Q6SS_PWR_CTL */
#define Q6SS_L2DATA_STBY_N		BIT(18)
#define Q6SS_SLP_RET_N			BIT(19)
#define Q6SS_CLAMP_IO			BIT(20)
#define QDSS_BHS_ON			BIT(21)
#define QDSS_Q6_MEMORIES		GENMASK(15, 0)

/* Q6SS parameters */
#define Q6SS_LDO_BYP		BIT(25)
#define Q6SS_BHS_ON		BIT(24)
#define Q6SS_CLAMP_WL		BIT(21)
#define Q6SS_CLAMP_QMC_MEM		BIT(22)
#define HALT_CHECK_MAX_TIMEOUT         20000000
#define Q6SS_XO_CBCR		GENMASK(5, 3)
#define Q6SS_SLEEP_CBCR		GENMASK(5, 2)
#define Q6SS_CORE_CBCR         BIT(5)
#define Q6SS_TIMEOUT_US         1000

/* Q6SS config/status registers */
#define TCSR_GLOBAL_CFG0	0x0
#define TCSR_GLOBAL_CFG1	0x4
#define SSCAON_CONFIG		0x8
#define SSCAON_STATUS		0xc
#define Q6SS_BHS_STATUS		0x78
#define Q6SS_RST_EVB		0x10

#define BHS_EN_REST_ACK		BIT(0)
#define WCSS_HM_RET		BIT(1)
#define SSCAON_ENABLE		BIT(13)
#define SSCAON_BUS_EN		BIT(15)
#define SSCAON_BUS_MUX_MASK	GENMASK(18, 16)
#define SSCAON_MASK             GENMASK(17, 15)

#define MEM_BANKS		19
#define TCSR_WCSS_CLK_MASK	0x1F
#define TCSR_WCSS_CLK_ENABLE	0x15

#define MAX_HALT_REG		4

#define WCNSS_PAS_ID		6
#define QDSP6SS_Q6V7_VERSION	0x30010000
#define QDSP6SS_Q6V6_VERSION	0x20020000
#define QDSP6SS_Q6V5_VERSION	0x100A0000
#define WCSS_VERSION		0x10000000

static int debug_wcss;
static const struct wcss_data wcss_ipq6018_res_init;

enum {
	WCSS_IPQ,
	WCSS_QCS404,
};

enum {
	Q6V5,
	Q6V6,
	Q6V7,
};

struct q6v5_wcss {
	struct device *dev;

	void __iomem *reg_base;
	void __iomem *rmb_base;
	void __iomem *wcmn_base;

	struct regmap *halt_map;
	u32 halt_q6;
	u32 halt_wcss;
	u32 halt_nc;
	u32 reset_cmd_id;

	struct clk *xo;
	struct clk *ahbfabric_cbcr_clk;
	struct clk *gcc_abhs_cbcr;			//"gcc_wcss_ahb_s_clk"
	struct clk *gcc_axim_cbcr;			//"gcc_q6_axim_clk"
	struct clk *lcc_csr_cbcr;
	struct clk *ahbs_cbcr;				//"gcc_q6_ahb_s_clk"
	struct clk *tcm_slave_cbcr;
	struct clk *qdsp6ss_abhm_cbcr;			//"gcc_q6_ahb_clk"
	struct clk *qdsp6ss_sleep_cbcr;
	struct clk *qdsp6ss_axim_cbcr;			//"gcc_wcss_axi_m_clk"
	struct clk *qdsp6ss_xo_cbcr;
	struct clk *qdsp6ss_core_gfmux;
	struct clk *lcc_bcr_sleep;
	struct clk *prng_clk;
	struct clk *eachb_clk;				//"gcc_wcss_ecahb_clk"
	struct clk *acmt_clk;				//"gcc_wcss_acmt_clk"
	struct clk *gcc_axim2_clk;			//"gcc_q6_axim2_clk"
	struct clk *axmis_clk;				//"gcc_q6_axis_clk"
	struct clk *axi_s_clk;				//"gcc_wcss_axi_s_clk"
	struct clk *gcc_sys_noc_wcss_ahb_clk;		//gcc_sys_noc_wcss_ahb_clk
	struct clk *gcc_q6ss_atbm_clk;			//gcc_q6ss_atbm_clk
	struct clk *gcc_q6ss_pclkdbg_clk;		//gcc_q6ss_pclkdbg_clk
	struct clk *gcc_q6_tsctr_1to2_clk;		//gcc_q6_tsctr_1to2_clk
	struct clk_bulk_data *clks;			//clks to get and use
	int num_clks;

	struct regulator *cx_supply;

	struct qcom_rproc_glink glink_subdev;
	struct qcom_rproc_ssr ssr_subdev;
	struct qcom_sysmon *sysmon;

	struct reset_control *wcss_aon_reset;
	struct reset_control *wcss_reset;
	struct reset_control *wcss_q6_reset;
	struct reset_control *wcss_q6_bcr_reset;
	struct reset_control *ce_reset;

	struct qcom_q6v5 q6v5;

	phys_addr_t mem_phys;
	phys_addr_t mem_reloc;
	void *mem_region;
	size_t mem_size;

	int crash_reason_smem;
	u32 version;
	u32 q6_version;
	bool requires_force_stop;
	bool need_mem_protection;
	const char *m3_firmware_name;
	bool backdoor;
};

struct wcss_data {
	int (*init_clock)(struct q6v5_wcss *wcss);
	int (*init_regulator)(struct q6v5_wcss *wcss);
	const char *q6_firmware_name;
	const char *m3_firmware_name;
	int crash_reason_smem;
	int remote_id;
	u32 version;
	u32 q6_version;
	u32 reset_cmd_id;
	bool aon_reset_required;
	bool wcss_q6_reset_required;
	bool bcr_reset_required;
	bool ce_reset_required;
	const char *ssr_name;
	const char *sysmon_name;
	int ssctl_id;
	const struct rproc_ops *ops;
	bool requires_force_stop;
	bool need_mem_protection;
	bool need_auto_boot;
	int q6ss_version;
	int wcss_version;
	bool clear_smp2p_last_value;
};

struct wcss_clk {
	const char* clk;
	unsigned long rate;
};

#ifdef CONFIG_IPQ_SUBSYSTEM_RAMDUMP
static void crashdump_init(struct rproc *rproc, struct rproc_dump_segment *segment, void *dest)
{
	void *handle;
	struct device_node *node = NULL, *np = NULL;
	struct ramdump_segment *segs;
	int ret, index = 0;
	int num_segs;

	handle = create_ramdump_device("q6mem", &rproc->dev);
	if (!handle) {
		dev_err(&rproc->dev, "unable to create ramdump device"
						"for %s\n", rproc->name);
		return;
	}

	if (create_ramdump_device_file(handle)) {
		dev_err(&rproc->dev, "unable to create ramdump device"
						"for %s\n", rproc->name);
		goto free_device;
	}

	np = rproc->dev.parent->of_node;

	num_segs = of_count_phandle_with_args(np, "memory-region", NULL);
	if (num_segs <= 0) {
		dev_err(&rproc->dev, "Could not find memory regions to dump");
		goto free_device;
	}

	dev_dbg(&rproc->dev, "number of segments to be dumped: %d\n", num_segs);

	segs = kzalloc(num_segs * sizeof(struct ramdump_segment), GFP_KERNEL);
	if (!segs) {
		dev_err(&rproc->dev, "Could not allocate memory for ramdump segments");
		goto free_device;
	}

	while (index < num_segs) {
		node = of_parse_phandle(np, "memory-region", index);
		if (!node)
			break;

		ret = of_property_read_u32_index(node, "reg", 1,
						 (u32 *)&segs[index].address);
		if (ret) {
			pr_err("Could not retrieve reg addr %d\n", ret);
			of_node_put(node);
			goto put_node;
		}

		ret = of_property_read_u32_index(node, "reg", 3,
						 (u32 *)&segs[index].size);
		if (ret) {
			pr_err("Could not retrieve reg size %d\n", ret);
			of_node_put(node);
			goto put_node;
		}
		segs[index].v_address = ioremap(segs[index].address,
						segs[index].size);
		of_node_put(node);
		index++;
	}

	do_elf_ramdump(handle, segs, index);

	for (index = 0; index < num_segs; index++) {
		if (segs[index].v_address)
			iounmap(segs[index].v_address);
	}
put_node:
	kfree(segs);
	of_node_put(np);
free_device:
	destroy_ramdump_device(handle);
}
#else
static void crashdump_init(struct rproc *rproc, struct rproc_dump_segment *segment, void *dest)
{
}
#endif

static int ipq6018_clks_prepare_enable(struct q6v5_wcss *wcss)
{
	int ret;

	ret = clk_prepare_enable(wcss->gcc_sys_noc_wcss_ahb_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->gcc_q6ss_atbm_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->gcc_q6ss_pclkdbg_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->gcc_q6_tsctr_1to2_clk);
	if (ret)
		return ret;

	return 0;
}

static int ipq5018_clks_prepare_enable(struct q6v5_wcss *wcss)
{
	int ret;

	ret = clk_prepare_enable(wcss->gcc_abhs_cbcr);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->eachb_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->acmt_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->gcc_axim_cbcr);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->qdsp6ss_axim_cbcr);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->gcc_axim2_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->qdsp6ss_abhm_cbcr);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->ahbs_cbcr);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->axi_s_clk);
	if (ret)
		return ret;

	return 0;
}

static void ipq6018_clks_prepare_disable(struct q6v5_wcss *wcss)
{
	clk_disable_unprepare(wcss->gcc_sys_noc_wcss_ahb_clk);
	clk_disable_unprepare(wcss->gcc_q6ss_atbm_clk);
	clk_disable_unprepare(wcss->gcc_q6ss_pclkdbg_clk);
	clk_disable_unprepare(wcss->gcc_q6_tsctr_1to2_clk);
}

static void ipq5018_clks_prepare_disable(struct q6v5_wcss *wcss)
{
	clk_disable_unprepare(wcss->gcc_abhs_cbcr);
	clk_disable_unprepare(wcss->eachb_clk);
	clk_disable_unprepare(wcss->acmt_clk);
	clk_disable_unprepare(wcss->gcc_axim_cbcr);
	clk_disable_unprepare(wcss->qdsp6ss_axim_cbcr);
	clk_disable_unprepare(wcss->gcc_axim2_clk);
	clk_disable_unprepare(wcss->qdsp6ss_abhm_cbcr);
	clk_disable_unprepare(wcss->ahbs_cbcr);
	clk_disable_unprepare(wcss->axi_s_clk);
}

static int ipq9574_wcss_clks_prepare_enable(struct q6v5_wcss *wcss)
{
	int ret;

	ret = clk_prepare_enable(wcss->gcc_axim2_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->gcc_abhs_cbcr);
	if (ret)
		return ret;

	ret = clk_prepare_enable(wcss->qdsp6ss_axim_cbcr);
	if (ret)
		return ret;

	return 0;
}

static void ipq9574_wcss_clks_prepare_disable(struct q6v5_wcss *wcss)
{
	clk_disable_unprepare(wcss->gcc_axim2_clk);
	clk_disable_unprepare(wcss->gcc_abhs_cbcr);
	clk_disable_unprepare(wcss->qdsp6ss_axim_cbcr);
}

static int ipq9574_enable_dbg_clks(struct device *dev)
{
	int i, ret;
	struct clk* dclk;

	struct wcss_clk dbg_clks[] = {
		{ .clk = "dbg-apb-bdg", .rate = 150000000 },
		{ .clk = "dbg-atb-bdg", .rate = 240000000 },
		{ .clk = "dbg-dapbus-bdg", .rate = 150000000 },
		{ .clk = "dbg-nts-bdg", .rate = 300000000 },
		{ .clk = "dbg-apb", .rate = 150000000 },
		{ .clk = "dbg-atb", .rate = 240000000 },
		{ .clk = "dbg-dapbus", .rate = 150000000 },
		{ .clk = "dbg-nts", .rate = 300000000 },
		{ .clk = "q6_tsctr_1to2_clk", .rate = 300000000 },
		{ .clk = "q6ss_atbm_clk", .rate = 240000000 },
		{ .clk = "q6ss_pclkdbg_clk", .rate = 150000000 },
		{ .clk = "q6ss_trig_clk", .rate = 150000000 },
	};

	for (i = 0; i < ARRAY_SIZE(dbg_clks); i++) {
		dclk = devm_clk_get(dev, dbg_clks[i].clk);
		if (IS_ERR(dclk)) {
			ret = PTR_ERR(dclk);
			if (ret != -EPROBE_DEFER)
				dev_err(dev, "failed to get gcc %s", dbg_clks[i].clk);
			return PTR_ERR(dclk);
		}

		ret = clk_set_rate(dclk, dbg_clks[i].rate);
		if (ret) {
			dev_err(dev, "failed to set rate for %s", dbg_clks[i].clk);
			return ret;
		}

		ret = clk_prepare_enable(dclk);
		if (ret) {
			dev_err(dev, "failed to enable %s", dbg_clks[i].clk);
			return ret;
		}
	}
	return 0;
}

static int q6v7_wcss_reset(struct q6v5_wcss *wcss, struct rproc *rproc)
{
	int ret;
	u32 val;
	int temp = 0;

	/*1. Set TCSR GLOBAL CFG1*/
	ret = regmap_update_bits(wcss->halt_map,
				 wcss->halt_nc + TCSR_GLOBAL_CFG1,
				 0xff00, 0x1100);
	if (ret) {
		dev_err(wcss->dev, "TCSE_GLOBAL_CFG1 failed\n");
		return ret;
	}

	/* Enable Q6 clocks */
	ret = clk_bulk_prepare_enable(wcss->num_clks, wcss->clks);
	if (ret) {
		dev_err(wcss->dev, "failed to enable clocks, err=%d\n", ret);
		return ret;
	};

	if (debug_wcss)
		writel(0x20000001, wcss->reg_base + Q6SS_DBG_CFG);

	/* Write bootaddr to EVB so that Q6WCSS will jump there after reset */
	writel(rproc->bootaddr >> 4, wcss->reg_base + Q6SS_RST_EVB);

	/*2. Deassert AON Reset */
	ret = reset_control_deassert(wcss->wcss_aon_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_aon_reset failed\n");
		clk_bulk_disable_unprepare(wcss->num_clks, wcss->clks);
		return ret;
	}

	/*8. Set mpm configs*/
	/*set CFG[18:15]=1*/
	val = readl(wcss->rmb_base + SSCAON_CONFIG);
	val &= ~SSCAON_MASK;
	val |= SSCAON_BUS_EN;
	writel(val, wcss->rmb_base + SSCAON_CONFIG);

	/*9. Wait for SSCAON_STATUS */
	val = readl(wcss->rmb_base + SSCAON_STATUS);
	ret = readl_poll_timeout(wcss->rmb_base + SSCAON_STATUS,
				 val, (val & 0xffff) == 0x10, 1000,
				 Q6SS_TIMEOUT_US * 1000);
	if (ret) {
		dev_err(wcss->dev, " Boot Error, SSCAON=0x%08X\n", val);
		return ret;
	}

	/*3. BHS require xo cbcr to be enabled */
	val = readl(wcss->reg_base + Q6SS_XO_CBCR);
	val |= 0x1;
	writel(val, wcss->reg_base + Q6SS_XO_CBCR);

	/*4. Enable core cbcr*/
	val = readl(wcss->reg_base + Q6SS_CORE_CBCR);
	val |= 0x1;
	writel(val, wcss->reg_base + Q6SS_CORE_CBCR);

	/*5. Enable sleep cbcr*/
	val = readl(wcss->reg_base + Q6SS_SLEEP_CBCR);
	val |= 0x1;
	writel(val, wcss->reg_base + Q6SS_SLEEP_CBCR);

	/*6. Boot core start */
	writel(0x1, wcss->reg_base + Q6SS_BOOT_CORE_START);
	writel(0x1, wcss->reg_base + Q6SS_BOOT_CMD);

	/*7. Pray god and wait for reset to complete*/
	while (temp < 20) {
		val = readl(wcss->reg_base + Q6SS_BOOT_STATUS);
		if (val & 0x01)
			break;
		mdelay(1);
		temp++;
	}

	/* Enable WCSS clocks */
	ret = ipq9574_wcss_clks_prepare_enable(wcss);
	if (ret) {
		dev_err(wcss->dev, "wcss clk(s) enable failed");
		return ret;
	}

	/* Enable WCSS dbg clocks */
	ret = ipq9574_enable_dbg_clks(wcss->dev);
	if (ret) {
		dev_err(wcss->dev, "wcss dbg clks enable failed");
		return ret;
	}

	return 0;
}

static void q6v6_wcss_reset(struct q6v5_wcss *wcss)
{
	u32 val;
	int ret;
	int temp = 0;
	unsigned int cookie;

	/*Assert Q6 BLK Reset*/
	ret = reset_control_assert(wcss->wcss_q6_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_q6_reset failed\n");
		return;
	}

	/* AON Reset */
	ret = reset_control_deassert(wcss->wcss_aon_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_aon_reset failed\n");
		return;
	}

	/*Enable Q6 AXIS CLOCK RESET*/
	ret = clk_prepare_enable(wcss->axmis_clk);
	if (ret) {
		dev_err(wcss->dev, "wcss clk(s) enable failed");
		return;
	}

	/*Disable Q6 AXIS CLOCK RESET*/
	clk_disable_unprepare(wcss->axmis_clk);

	/*De assert Q6 BLK reset*/
	ret = reset_control_deassert(wcss->wcss_q6_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_q6_reset failed\n");
		return;
	}

	/*Prepare Q6 clocks*/
	ret = ipq5018_clks_prepare_enable(wcss);
	if (ret) {
		dev_err(wcss->dev, "wcss clk(s) enable failed");
		return;
	}

	/*Secure access to WIFI phy register*/
	regmap_update_bits(wcss->halt_map,
			wcss->halt_nc + TCSR_GLOBAL_CFG1,
			TCSR_WCSS_CLK_MASK,
			0x18);

	/*Disable Q6 AXI2 select*/
	regmap_update_bits(wcss->halt_map,
			wcss->halt_nc + TCSR_GLOBAL_CFG0, 0x40, 0xF0);

	/*wcss axib ib status*/
	regmap_update_bits(wcss->halt_map,
			wcss->halt_nc + TCSR_GLOBAL_CFG0, 0x100, 0x100);


	/*Q6 AHB upper & lower address*/
	writel(0x00cdc000, wcss->reg_base + Q6SS_AHB_UPPER);
	writel(0x00ca0000, wcss->reg_base + Q6SS_AHB_LOWER);

	/*set CFG[18:15]=1*/
	val = readl(wcss->rmb_base + SSCAON_CONFIG);
	val &= ~SSCAON_MASK;
	val |= SSCAON_BUS_EN;
	writel(val, wcss->rmb_base + SSCAON_CONFIG);

	val = readl(wcss->rmb_base + SSCAON_CONFIG);
	val &= ~(1<<1);
	writel(val, wcss->rmb_base + SSCAON_CONFIG);

	/* Wait for SSCAON_STATUS */
	val = readl(wcss->rmb_base + SSCAON_STATUS);
	ret = readl_poll_timeout(wcss->rmb_base + SSCAON_STATUS,
				 val, (val & 0xffff) == 0x10, 1000,
				 Q6SS_TIMEOUT_US * 1000);
	if (ret) {
		dev_err(wcss->dev, " Boot Error, SSCAON=0x%08X\n", val);
		return;
	}

	/*Deassert ce reset*/
	ret = reset_control_deassert(wcss->ce_reset);
	if (ret) {
		dev_err(wcss->dev, "ce_reset failed\n");
		return;
	}

	if (debug_wcss)
		writel(0x20000001, wcss->reg_base + Q6SS_DBG_CFG);
	else
		writel(0x0, wcss->reg_base + Q6SS_DBG_CFG);

	cookie = 1;
	ret = qti_scm_wcss_boot(Q6_BOOT_TRIG_SVC_ID,
				 Q6_BOOT_TRIG_CMD_ID, &cookie);
	if (ret) {
		dev_err(wcss->dev, "q6-boot trigger scm failed\n");
		return;
	}

	/* Boot core start */
	writel(0x1, wcss->reg_base + Q6SS_BOOT_CORE_START);

	while (temp < 20) {
		val = readl(wcss->reg_base + Q6SS_BOOT_STATUS);
		if (val & 0x01)
			break;
		mdelay(1);
		temp += 1;
	}

	pr_err("%s: start %s\n", wcss->q6v5.rproc->name,
					val == 1 ? "successful" : "failed");
	wcss->q6v5.running = val == 1 ? true : false;
}

static int q6v5_wcss_reset(struct rproc *rproc)
{
	int ret;
	u32 val;
	int i;
	struct q6v5_wcss *wcss = rproc->priv;
	const struct wcss_data *desc;

	desc = of_device_get_match_data(wcss->dev);
	if (!desc)
		return -EINVAL;

	if (desc->aon_reset_required) {
		/* Deassert wcss aon reset */
		ret = reset_control_deassert(wcss->wcss_aon_reset);
		if (ret) {
			dev_err(wcss->dev, "wcss_aon_reset failed\n");
			return ret;
		}
		mdelay(1);
	}

	if (desc == &wcss_ipq6018_res_init) {
		ret = ipq6018_clks_prepare_enable(wcss);
		if (ret) {
			dev_err(wcss->dev, "failed to enable clock\n");
			return ret;
		}
	}

	val = readl(wcss->rmb_base + SSCAON_CONFIG);
	val |= BIT(0);
	writel(val, wcss->rmb_base + SSCAON_CONFIG);
	mdelay(1);

	/*set CFG[18:15]=1* and clear CFG[1]=0*/
	val = readl(wcss->rmb_base + SSCAON_CONFIG);
	val &= ~(SSCAON_BUS_MUX_MASK | WCSS_HM_RET);
	val |= SSCAON_BUS_EN;
	writel(val, wcss->rmb_base + SSCAON_CONFIG);
	mdelay(1);

	writel(rproc->bootaddr >> 4, wcss->reg_base + Q6SS_RST_EVB);

	/* Assert resets, stop core */
	val = readl(wcss->reg_base + Q6SS_RESET_REG);
	val |= Q6SS_CORE_ARES | Q6SS_BUS_ARES_ENABLE | Q6SS_STOP_CORE;
	writel(val, wcss->reg_base + Q6SS_RESET_REG);

	/* BHS require xo cbcr to be enabled */
	val = readl(wcss->reg_base + Q6SS_XO_CBCR);
	val |= 0x1;
	writel(val, wcss->reg_base + Q6SS_XO_CBCR);

	/* Read CLKOFF bit to go low indicating CLK is enabled */
	ret = readl_poll_timeout(wcss->reg_base + Q6SS_XO_CBCR,
				 val, !(val & BIT(31)), 1,
				 Q6SS_TIMEOUT_US);
	if (ret) {
		dev_err(wcss->dev,
			"xo cbcr enabling timed out (rc:%d)\n", ret);
		return ret;
	}
	/* Enable power block headswitch and wait for it to stabilize */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val |= Q6SS_BHS_ON;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);
	udelay(1);

	/* 10 - Wait till BHS Reset is done */
	ret = readl_poll_timeout(wcss->reg_base + Q6SS_BHS_STATUS,
			val, (val & BHS_EN_REST_ACK), 1000,
			Q6SS_TIMEOUT_US * 50);
	if (ret) {
		dev_err(wcss->dev, "BHS_STATUS not ON (rc:%d) val:0x%X\n", ret, val);
		return ret;
	}

	/* Put LDO in bypass mode */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val |= Q6SS_LDO_BYP;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* Deassert Q6 compiler memory clamp */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val &= ~Q6SS_CLAMP_QMC_MEM;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* Deassert memory peripheral sleep and L2 memory standby */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val |= Q6SS_L2DATA_STBY_N | Q6SS_SLP_RET_N;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* Turn on L1, L2, ETB and JU memories 1 at a time */
	val = readl(wcss->reg_base + Q6SS_MEM_PWR_CTL);
	for (i = MEM_BANKS; i >= 0; i--) {
		val |= BIT(i);
		writel(val, wcss->reg_base + Q6SS_MEM_PWR_CTL);
		/*
		 * Read back value to ensure the write is done then
		 * wait for 10ms for both memory peripheral and data
		 * array to turn on.
		 */
		val |= readl(wcss->reg_base + Q6SS_MEM_PWR_CTL);
		mdelay(10);
	}
	/* Remove word line clamp */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val &= ~Q6SS_CLAMP_WL;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* Remove IO clamp */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val &= ~Q6SS_CLAMP_IO;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* Bring core out of reset */
	val = readl(wcss->reg_base + Q6SS_RESET_REG);
	val &= ~Q6SS_CORE_ARES;
	writel(val, wcss->reg_base + Q6SS_RESET_REG);

	/* Turn on core clock */
	val = readl(wcss->reg_base + Q6SS_GFMUX_CTL_REG);
	val |= Q6SS_CLK_ENABLE;
	writel(val, wcss->reg_base + Q6SS_GFMUX_CTL_REG);

	/* Start core execution */
	val = readl(wcss->reg_base + Q6SS_RESET_REG);
	val &= ~Q6SS_STOP_CORE;
	writel(val, wcss->reg_base + Q6SS_RESET_REG);

	/* Wait for SSCAON_STATUS */
	val = readl(wcss->rmb_base + SSCAON_STATUS);
	ret = readl_poll_timeout(wcss->rmb_base + SSCAON_STATUS,
				 val, (val & 0xffff) == 0x10, 1000,
				 Q6SS_TIMEOUT_US * 1000);
	if (ret) {
		dev_err(wcss->dev, " Boot Error, SSCAON=0x%08X\n", val);
		return ret;
	}

	return 0;
}

static int q6v5_wcss_start(struct rproc *rproc)
{
	struct q6v5_wcss *wcss = rproc->priv;
	int ret;
	uint32_t val;
	const struct wcss_data *desc;

	desc = of_device_get_match_data(wcss->dev);
	if (!desc)
		return -EINVAL;

	ret = clk_prepare_enable(wcss->prng_clk);
	if (ret) {
		dev_err(wcss->dev, "prng clock enable failed\n");
		return ret;
	}

	qcom_q6v5_prepare(&wcss->q6v5);

	if (desc->clear_smp2p_last_value)
		qcom_clear_smp2p_last_value();

	if (wcss->need_mem_protection) {
		ret = qcom_scm_pas_auth_and_reset(WCNSS_PAS_ID, debug_wcss,
					wcss->reset_cmd_id);
		if (ret) {
			dev_err(wcss->dev, "wcss_reset failed\n");
			return ret;
		}
		goto wait_for_reset;
	}

	/* Release Q6 and WCSS reset */
	ret = reset_control_deassert(wcss->wcss_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_reset failed\n");
		return ret;
	}

	ret = reset_control_deassert(wcss->wcss_q6_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_q6_reset failed\n");
		goto wcss_reset;
	}

	/* Lithium configuration - clock gating and bus arbitration */
	ret = regmap_update_bits(wcss->halt_map,
				 wcss->halt_nc + TCSR_GLOBAL_CFG0,
				 TCSR_WCSS_CLK_MASK,
				 TCSR_WCSS_CLK_ENABLE);
	if (ret)
		goto wcss_q6_reset;

	ret = regmap_update_bits(wcss->halt_map,
				 wcss->halt_nc + TCSR_GLOBAL_CFG1,
				 1, 0);
	if (ret)
		goto wcss_q6_reset;

	if (wcss->q6_version != Q6V7 && wcss->q6_version != Q6V6) {
		if (debug_wcss)
			writel(0x20000001, wcss->reg_base + Q6SS_DBG_CFG);
	}

	if (wcss->q6_version == Q6V6) {
		/* Write bootaddr to EVB so that
		 * Q6WCSS will jump there after reset
		 */
		writel(rproc->bootaddr >> 4, wcss->reg_base + Q6SS_RST_EVB);
		q6v6_wcss_reset(wcss);
	} else if (wcss->q6_version == Q6V7)
		ret = q6v7_wcss_reset(wcss, rproc);
	else
		ret = q6v5_wcss_reset(rproc);

	if (ret)
		goto wcss_q6_reset;

wait_for_reset:
	ret = qcom_q6v5_wait_for_start(&wcss->q6v5, 5 * HZ);
	if (ret == -ETIMEDOUT) {
		if (debug_wcss)
			goto wait_for_reset;
		else
			dev_err(wcss->dev, "start timed out\n");
	}

	/*reset done clear the debug register*/
	if (debug_wcss && wcss->q6_version != Q6V6)
		writel(0x0, wcss->reg_base + Q6SS_DBG_CFG);

	if (wcss->wcmn_base) {
		/* Read the version registers to make sure WCSS is out of reset
		 */
		val = readl(wcss->reg_base);
		if (val != desc->q6ss_version) {
			dev_err(wcss->dev, "Invalid QDSP6SS Version : 0x%x\n", val);
			return -EINVAL;
		}
		dev_info(wcss->dev, "QDSP6SS Version : 0x%x\n", val);

		val = readl(wcss->wcmn_base);
		if (val != desc->wcss_version) {
			dev_err(wcss->dev, "Invalid WCSS Version : 0x%x\n", val);
			return -EINVAL;
		}
		dev_info(wcss->dev, "WCSS Version : 0x%x\n", val);
	}

	return ret;

wcss_q6_reset:
	reset_control_assert(wcss->wcss_q6_reset);

wcss_reset:
	reset_control_assert(wcss->wcss_reset);
	if (debug_wcss)
		writel(0x0, wcss->reg_base + Q6SS_DBG_CFG);

	return ret;
}

static int q6v5_wcss_qcs404_power_on(struct q6v5_wcss *wcss)
{
	unsigned long val;
	int ret, idx;

	/* Toggle the restart */
	reset_control_assert(wcss->wcss_reset);
	usleep_range(200, 300);
	reset_control_deassert(wcss->wcss_reset);
	usleep_range(200, 300);

	/* Enable GCC_WDSP_Q6SS_AHBS_CBCR clock */
	ret = clk_prepare_enable(wcss->gcc_abhs_cbcr);
	if (ret)
		return ret;

	/* Remove reset to the WCNSS QDSP6SS */
	reset_control_deassert(wcss->wcss_q6_bcr_reset);

	/* Enable Q6SSTOP_AHBFABRIC_CBCR clock */
	ret = clk_prepare_enable(wcss->ahbfabric_cbcr_clk);
	if (ret)
		goto disable_gcc_abhs_cbcr_clk;

	/* Enable the LCCCSR CBC clock, Q6SSTOP_Q6SSTOP_LCC_CSR_CBCR clock */
	ret = clk_prepare_enable(wcss->lcc_csr_cbcr);
	if (ret)
		goto disable_ahbfabric_cbcr_clk;

	/* Enable the Q6AHBS CBC, Q6SSTOP_Q6SS_AHBS_CBCR clock */
	ret = clk_prepare_enable(wcss->ahbs_cbcr);
	if (ret)
		goto disable_csr_cbcr_clk;

	/* Enable the TCM slave CBC, Q6SSTOP_Q6SS_TCM_SLAVE_CBCR clock */
	ret = clk_prepare_enable(wcss->tcm_slave_cbcr);
	if (ret)
		goto disable_ahbs_cbcr_clk;

	/* Enable the Q6SS AHB master CBC, Q6SSTOP_Q6SS_AHBM_CBCR clock */
	ret = clk_prepare_enable(wcss->qdsp6ss_abhm_cbcr);
	if (ret)
		goto disable_tcm_slave_cbcr_clk;

	/* Enable the Q6SS AXI master CBC, Q6SSTOP_Q6SS_AXIM_CBCR clock */
	ret = clk_prepare_enable(wcss->qdsp6ss_axim_cbcr);
	if (ret)
		goto disable_abhm_cbcr_clk;

	/* Enable the Q6SS XO CBC */
	val = readl(wcss->reg_base + Q6SS_XO_CBCR);
	val |= BIT(0);
	writel(val, wcss->reg_base + Q6SS_XO_CBCR);
	/* Read CLKOFF bit to go low indicating CLK is enabled */
	ret = readl_poll_timeout(wcss->reg_base + Q6SS_XO_CBCR,
				 val, !(val & BIT(31)), 1,
				 HALT_CHECK_MAX_TIMEOUT);
	if (ret) {
		dev_err(wcss->dev,
			"xo cbcr enabling timed out (rc:%d)\n", ret);
		return ret;
	}

	writel(0, wcss->reg_base + Q6SS_CGC_OVERRIDE);

	/* Enable QDSP6 sleep clock clock */
	val = readl(wcss->reg_base + Q6SS_SLEEP_CBCR);
	val |= BIT(0);
	writel(val, wcss->reg_base + Q6SS_SLEEP_CBCR);

	/* Enable the Enable the Q6 AXI clock, GCC_WDSP_Q6SS_AXIM_CBCR*/
	ret = clk_prepare_enable(wcss->gcc_axim_cbcr);
	if (ret)
		goto disable_sleep_cbcr_clk;

	/* Assert resets, stop core */
	val = readl(wcss->reg_base + Q6SS_RESET_REG);
	val |= Q6SS_CORE_ARES | Q6SS_BUS_ARES_ENABLE | Q6SS_STOP_CORE;
	writel(val, wcss->reg_base + Q6SS_RESET_REG);

	/* Program the QDSP6SS PWR_CTL register */
	writel(0x01700000, wcss->reg_base + Q6SS_PWR_CTL_REG);

	writel(0x03700000, wcss->reg_base + Q6SS_PWR_CTL_REG);

	writel(0x03300000, wcss->reg_base + Q6SS_PWR_CTL_REG);

	writel(0x033C0000, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/*
	 * Enable memories by turning on the QDSP6 memory foot/head switch, one
	 * bank at a time to avoid in-rush current
	 */
	for (idx = 28; idx >= 0; idx--) {
		writel((readl(wcss->reg_base + Q6SS_MEM_PWR_CTL) |
			(1 << idx)), wcss->reg_base + Q6SS_MEM_PWR_CTL);
	}

	writel(0x031C0000, wcss->reg_base + Q6SS_PWR_CTL_REG);
	writel(0x030C0000, wcss->reg_base + Q6SS_PWR_CTL_REG);

	val = readl(wcss->reg_base + Q6SS_RESET_REG);
	val &= ~Q6SS_CORE_ARES;
	writel(val, wcss->reg_base + Q6SS_RESET_REG);

	/* Enable the Q6 core clock at the GFM, Q6SSTOP_QDSP6SS_GFMUX_CTL */
	val = readl(wcss->reg_base + Q6SS_GFMUX_CTL_REG);
	val |= Q6SS_CLK_ENABLE | Q6SS_SWITCH_CLK_SRC;
	writel(val, wcss->reg_base + Q6SS_GFMUX_CTL_REG);

	/* Enable sleep clock branch needed for BCR circuit */
	ret = clk_prepare_enable(wcss->lcc_bcr_sleep);
	if (ret)
		goto disable_core_gfmux_clk;

	return 0;

disable_core_gfmux_clk:
	val = readl(wcss->reg_base + Q6SS_GFMUX_CTL_REG);
	val &= ~(Q6SS_CLK_ENABLE | Q6SS_SWITCH_CLK_SRC);
	writel(val, wcss->reg_base + Q6SS_GFMUX_CTL_REG);
	clk_disable_unprepare(wcss->gcc_axim_cbcr);
disable_sleep_cbcr_clk:
	val = readl(wcss->reg_base + Q6SS_SLEEP_CBCR);
	val &= ~Q6SS_CLK_ENABLE;
	writel(val, wcss->reg_base + Q6SS_SLEEP_CBCR);
	val = readl(wcss->reg_base + Q6SS_XO_CBCR);
	val &= ~Q6SS_CLK_ENABLE;
	writel(val, wcss->reg_base + Q6SS_XO_CBCR);
	clk_disable_unprepare(wcss->qdsp6ss_axim_cbcr);
disable_abhm_cbcr_clk:
	clk_disable_unprepare(wcss->qdsp6ss_abhm_cbcr);
disable_tcm_slave_cbcr_clk:
	clk_disable_unprepare(wcss->tcm_slave_cbcr);
disable_ahbs_cbcr_clk:
	clk_disable_unprepare(wcss->ahbs_cbcr);
disable_csr_cbcr_clk:
	clk_disable_unprepare(wcss->lcc_csr_cbcr);
disable_ahbfabric_cbcr_clk:
	clk_disable_unprepare(wcss->ahbfabric_cbcr_clk);
disable_gcc_abhs_cbcr_clk:
	clk_disable_unprepare(wcss->gcc_abhs_cbcr);

	return ret;
}

static inline int q6v5_wcss_qcs404_reset(struct q6v5_wcss *wcss)
{
	unsigned long val;

	writel(0x80800000, wcss->reg_base + Q6SS_STRAP_ACC);

	/* Start core execution */
	val = readl(wcss->reg_base + Q6SS_RESET_REG);
	val &= ~Q6SS_STOP_CORE;
	writel(val, wcss->reg_base + Q6SS_RESET_REG);

	return 0;
}

static int q6v5_qcs404_wcss_start(struct rproc *rproc)
{
	struct q6v5_wcss *wcss = rproc->priv;
	int ret;

	ret = clk_prepare_enable(wcss->xo);
	if (ret)
		return ret;

	ret = regulator_enable(wcss->cx_supply);
	if (ret)
		goto disable_xo_clk;

	qcom_q6v5_prepare(&wcss->q6v5);

	ret = q6v5_wcss_qcs404_power_on(wcss);
	if (ret) {
		dev_err(wcss->dev, "wcss clk_enable failed\n");
		goto disable_cx_supply;
	}

	writel(rproc->bootaddr >> 4, wcss->reg_base + Q6SS_RST_EVB);

	q6v5_wcss_qcs404_reset(wcss);

	ret = qcom_q6v5_wait_for_start(&wcss->q6v5, 5 * HZ);
	if (ret == -ETIMEDOUT) {
		dev_err(wcss->dev, "start timed out\n");
		goto disable_cx_supply;
	}

	return 0;

disable_cx_supply:
	regulator_disable(wcss->cx_supply);
disable_xo_clk:
	clk_disable_unprepare(wcss->xo);

	return ret;
}

static void q6v6_wcss_halt_axi_port(struct q6v5_wcss *wcss,
				    struct regmap *halt_map,
				    u32 offset)
{
	unsigned long timeout;
	unsigned int val;
	int ret;

	/* Assert halt request */
	regmap_write(halt_map, offset + AXI_HALTREQ_REG, 1);

	/* Wait for halt */
	timeout = jiffies + msecs_to_jiffies(HALT_ACK_TIMEOUT_MS);
	for (;;) {
		ret = regmap_read(halt_map, offset + AXI_HALTACK_REG, &val);
		if (ret || val || time_after(jiffies, timeout))
			break;

		msleep(1);
	}

	if (offset == wcss->halt_q6) {
		ret = regmap_read(halt_map, offset + AXI_IDLE_REG, &val);
		if (ret || !val)
			dev_err(wcss->dev, "port failed halt\n");
	}

	/* Clear halt request (port will remain halted until reset) */
	regmap_write(halt_map, offset + AXI_HALTREQ_REG, 0);
}

static void q6v5_wcss_halt_axi_port(struct q6v5_wcss *wcss,
				    struct regmap *halt_map,
				    u32 offset)
{
	unsigned long timeout;
	unsigned int val;
	int ret;

	if (wcss->q6_version != Q6V5) {
		/* Check if we're already idle */
		ret = regmap_read(halt_map, offset + AXI_IDLE_REG, &val);
		if (!ret && val)
			return;
	}

	/* Assert halt request */
	regmap_read(halt_map, offset + AXI_HALTREQ_REG, &val);
	val |= BIT(0);
	regmap_write(halt_map, offset + AXI_HALTREQ_REG, 1);

	/* Wait for halt */
	timeout = jiffies + msecs_to_jiffies(HALT_ACK_TIMEOUT_MS);
	for (;;) {
		ret = regmap_read(halt_map, offset + AXI_HALTACK_REG, &val);
		if (ret || val || time_after(jiffies, timeout))
			break;

		msleep(1);
	}

	if (wcss->q6_version != Q6V5) {
		ret = regmap_read(halt_map, offset + AXI_IDLE_REG, &val);
		if (ret || !val)
			dev_err(wcss->dev, "port failed halt\n");

		/* Clear halt request (port will remain halted until reset) */
		regmap_write(halt_map, offset + AXI_HALTREQ_REG, 0);
	}
}

static int q6v5_qcs404_wcss_shutdown(struct q6v5_wcss *wcss)
{
	unsigned long val;
	int ret;

	q6v5_wcss_halt_axi_port(wcss, wcss->halt_map, wcss->halt_wcss);

	/* assert clamps to avoid MX current inrush */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val |= (Q6SS_CLAMP_IO | Q6SS_CLAMP_WL | Q6SS_CLAMP_QMC_MEM);
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* Disable memories by turning off memory foot/headswitch */
	writel((readl(wcss->reg_base + Q6SS_MEM_PWR_CTL) &
		~QDSS_Q6_MEMORIES),
		wcss->reg_base + Q6SS_MEM_PWR_CTL);

	/* Clear the BHS_ON bit */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val &= ~Q6SS_BHS_ON;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	clk_disable_unprepare(wcss->ahbfabric_cbcr_clk);
	clk_disable_unprepare(wcss->lcc_csr_cbcr);
	clk_disable_unprepare(wcss->tcm_slave_cbcr);
	clk_disable_unprepare(wcss->qdsp6ss_abhm_cbcr);
	clk_disable_unprepare(wcss->qdsp6ss_axim_cbcr);

	val = readl(wcss->reg_base + Q6SS_SLEEP_CBCR);
	val &= ~BIT(0);
	writel(val, wcss->reg_base + Q6SS_SLEEP_CBCR);

	val = readl(wcss->reg_base + Q6SS_XO_CBCR);
	val &= ~BIT(0);
	writel(val, wcss->reg_base + Q6SS_XO_CBCR);

	clk_disable_unprepare(wcss->ahbs_cbcr);
	clk_disable_unprepare(wcss->lcc_bcr_sleep);

	val = readl(wcss->reg_base + Q6SS_GFMUX_CTL_REG);
	val &= ~(Q6SS_CLK_ENABLE | Q6SS_SWITCH_CLK_SRC);
	writel(val, wcss->reg_base + Q6SS_GFMUX_CTL_REG);

	clk_disable_unprepare(wcss->gcc_abhs_cbcr);

	ret = reset_control_assert(wcss->wcss_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_reset failed\n");
		return ret;
	}
	usleep_range(200, 300);

	ret = reset_control_deassert(wcss->wcss_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_reset failed\n");
		return ret;
	}
	usleep_range(200, 300);

	clk_disable_unprepare(wcss->gcc_axim_cbcr);

	return 0;
}

static int q6v5_wcss_powerdown(struct q6v5_wcss *wcss)
{
	int ret;
	u32 val;

	/* 1 - Assert WCSS/Q6 HALTREQ */
	if (wcss->q6_version == Q6V6)
		q6v6_wcss_halt_axi_port(wcss, wcss->halt_map, wcss->halt_wcss);
	else
		q6v5_wcss_halt_axi_port(wcss, wcss->halt_map, wcss->halt_wcss);

	if (wcss->q6_version == Q6V6 || wcss->q6_version == Q6V7) {
		val = readl(wcss->rmb_base + SSCAON_CONFIG);
		val &= ~SSCAON_MASK;
		val |= SSCAON_BUS_EN;
		writel(val, wcss->rmb_base + SSCAON_CONFIG);
	} else {
		/* 2 - Enable WCSSAON_CONFIG */
		val = readl(wcss->rmb_base + SSCAON_CONFIG);
		val |= SSCAON_ENABLE;
		writel(val, wcss->rmb_base + SSCAON_CONFIG);

		/* 3 - Set SSCAON_CONFIG */
		val |= SSCAON_BUS_EN;
		val &= ~SSCAON_BUS_MUX_MASK;
		writel(val, wcss->rmb_base + SSCAON_CONFIG);
	}

	/* 4 - SSCAON_CONFIG 1 */
	val = readl(wcss->rmb_base + SSCAON_CONFIG);
	val |= BIT(1);
	writel(val, wcss->rmb_base + SSCAON_CONFIG);

	/* 5 - wait for SSCAON_STATUS */
	ret = readl_poll_timeout(wcss->rmb_base + SSCAON_STATUS,
				 val, (val & 0xffff) == 0x400, 1000,
				 Q6SS_TIMEOUT_US * 10);
	if (ret) {
		dev_err(wcss->dev,
			"can't get SSCAON_STATUS rc:%d)\n", ret);
		return ret;
	}

	mdelay(2);

	/* 6 - De-assert WCSS_AON reset */
	reset_control_assert(wcss->wcss_aon_reset);
	if (wcss->q6_version == Q6V5)
		mdelay(1);

	if (wcss->q6_version == Q6V6 || wcss->q6_version == Q6V7) {
		val = readl(wcss->rmb_base + SSCAON_CONFIG);
		val &= ~(1<<1);
		writel(val, wcss->rmb_base + SSCAON_CONFIG);
	} else {
		/* 7 - Disable WCSSAON_CONFIG 13 */
		val = readl(wcss->rmb_base + SSCAON_CONFIG);
		val &= ~SSCAON_ENABLE;
		writel(val, wcss->rmb_base + SSCAON_CONFIG);
	}

	/* 8 - De-assert WCSS/Q6 HALTREQ */
	if (wcss->q6_version != Q6V7)
		reset_control_assert(wcss->wcss_reset);

	if (wcss->q6_version == Q6V5) {
		/* Clear halt request (port will remain halted until reset) */
		regmap_read(wcss->halt_map, wcss->halt_wcss + AXI_HALTREQ_REG, &val);
		val &= ~0x1;
		regmap_write(wcss->halt_map, wcss->halt_wcss + AXI_HALTREQ_REG, val);
	}

	return 0;
}

static void q6v6_q6_powerdown(struct q6v5_wcss *wcss)
{
	int ret;
	unsigned int cookie;

	/*Assert ce reset*/
	reset_control_assert(wcss->ce_reset);
	mdelay(2);

	/*Disable clocks*/
	ipq5018_clks_prepare_disable(wcss);

	cookie = 0;
	ret = qti_scm_wcss_boot(Q6_BOOT_TRIG_SVC_ID,
				 Q6_BOOT_TRIG_CMD_ID, &cookie);
	if (ret) {
		dev_err(wcss->dev, "q6-stop trigger scm failed\n");
		return;
	}
}

static int q6v5_q6_powerdown(struct q6v5_wcss *wcss)
{
	u32 val;
	int ret, i;
	const struct wcss_data *desc = of_device_get_match_data(wcss->dev);

	/* To retain power domain after q6 powerdown */
	if (wcss->q6_version == Q6V5)
		writel(0x1, wcss->reg_base + Q6SS_DBG_CFG);

	/* 1 - Halt Q6 bus interface */
	if (wcss->q6_version == Q6V6)
		q6v6_wcss_halt_axi_port(wcss, wcss->halt_map, wcss->halt_q6);
	else
		q6v5_wcss_halt_axi_port(wcss, wcss->halt_map, wcss->halt_q6);

	if (wcss->q6_version != Q6V7) {
		/* 2 - Disable Q6 Core clock */
		val = readl(wcss->reg_base + Q6SS_GFMUX_CTL_REG);
		val &= ~Q6SS_CLK_ENABLE;
		writel(val, wcss->reg_base + Q6SS_GFMUX_CTL_REG);
	}

	if (wcss->q6_version == Q6V6) {
		q6v6_q6_powerdown(wcss);
		goto reset;
	} else if (wcss->q6_version == Q6V7) {
		/* 2 - Disable Q6 Core clock */
		val = readl(wcss->reg_base + Q6SS_GFMUX_CTL_REG);
		val &= ~BIT(0);
		writel(val, wcss->reg_base + Q6SS_GFMUX_CTL_REG);

		ipq9574_wcss_clks_prepare_disable(wcss);
		clk_bulk_disable_unprepare(wcss->num_clks, wcss->clks);
		goto reset;
	}

	/* 3 - Clamp I/O */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val |= Q6SS_CLAMP_IO;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* 4 - Clamp WL */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val |= QDSS_BHS_ON;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* 5 - Clear Erase standby */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val &= ~Q6SS_L2DATA_STBY_N;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* 6 - Clear Sleep RTN */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val &= ~Q6SS_SLP_RET_N;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* 7 - turn off Q6 memory foot/head switch one bank at a time */
	for (i = 0; i < 20; i++) {
		val = readl(wcss->reg_base + Q6SS_MEM_PWR_CTL);
		val &= ~BIT(i);
		writel(val, wcss->reg_base + Q6SS_MEM_PWR_CTL);
		mdelay(1);
	}

	/* 8 - Assert QMC memory RTN */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val |= Q6SS_CLAMP_QMC_MEM;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* 9 - Turn off BHS */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val &= ~Q6SS_BHS_ON;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);
	udelay(1);

	/* 10 - Wait till BHS Reset is done */
	ret = readl_poll_timeout(wcss->reg_base + Q6SS_BHS_STATUS,
				 val, !(val & BHS_EN_REST_ACK), 1000,
				 Q6SS_TIMEOUT_US * 10);
	if (ret) {
		dev_err(wcss->dev, "BHS_STATUS not OFF (rc:%d)\n", ret);
		return ret;
	}

reset:
	/* 11 -  Assert WCSS reset */
	reset_control_assert(wcss->wcss_reset);
	if (wcss->q6_version == Q6V5)
		mdelay(1);

	/* 12 - Assert Q6 reset */
	reset_control_assert(wcss->wcss_q6_reset);
	if (wcss->q6_version == Q6V5) {
		mdelay(2);

		/* Clear halt request (port will remain halted until reset) */
		regmap_read(wcss->halt_map, wcss->halt_q6 + AXI_HALTREQ_REG, &val);
		val &= ~0x1;
		regmap_write(wcss->halt_map, wcss->halt_q6 + AXI_HALTREQ_REG, val);
		mdelay(1);
	}
	if (desc == &wcss_ipq6018_res_init) {
		/* Disable clocks*/
		ipq6018_clks_prepare_disable(wcss);
	}

	return 0;
}

static int q6v5_wcss_stop(struct rproc *rproc)
{
	struct q6v5_wcss *wcss = rproc->priv;
	int ret;

	if (wcss->need_mem_protection) {
		ret = qcom_scm_pas_shutdown(WCNSS_PAS_ID);
		if (ret) {
			dev_err(wcss->dev, "not able to shutdown\n");
			return ret;
		}
		goto pas_done;
	}

	/* WCSS powerdown */
	if (wcss->requires_force_stop) {
		ret = qcom_q6v5_request_stop(&wcss->q6v5);
		if (ret == -ETIMEDOUT) {
			dev_info(wcss->dev, "force stop ack timed out, proceeding shutdown\n");
		}
	}

	if (wcss->version == WCSS_QCS404) {
		ret = q6v5_qcs404_wcss_shutdown(wcss);
		if (ret)
			return ret;
	} else {
		ret = q6v5_wcss_powerdown(wcss);
		if (ret)
			return ret;

		/* Q6 Power down */
		ret = q6v5_q6_powerdown(wcss);
		if (ret)
			return ret;
	}

pas_done:
	clk_disable_unprepare(wcss->prng_clk);
	qcom_q6v5_unprepare(&wcss->q6v5);

	return 0;
}

static void *q6v5_wcss_da_to_va(struct rproc *rproc, u64 da, int len)
{
	struct q6v5_wcss *wcss = rproc->priv;
	int offset;

	offset = da - wcss->mem_reloc;
	if (offset < 0 || offset + len > wcss->mem_size)
		return NULL;

	return wcss->mem_region + offset;
}

static int q6v5_wcss_load(struct rproc *rproc, const struct firmware *fw)
{
	struct q6v5_wcss *wcss = rproc->priv;
	const struct firmware *m3_fw;
	int ret;

	if (wcss->backdoor)
		return 0;

	if (wcss->m3_firmware_name) {
		ret = request_firmware(&m3_fw, wcss->m3_firmware_name,
				       wcss->dev);
		if (ret)
			goto skip_m3;

		ret = qcom_mdt_load_no_init(wcss->dev, m3_fw,
					    wcss->m3_firmware_name, 0,
					    wcss->mem_region, wcss->mem_phys,
					    wcss->mem_size, &wcss->mem_reloc);

		release_firmware(m3_fw);

		if (ret) {
			dev_err(wcss->dev, "can't load m3_fw.bXX\n");
			return ret;
		}
	}

skip_m3:
	if (wcss->need_mem_protection)
		return qcom_mdt_load(wcss->dev, fw, rproc->firmware,
				     WCNSS_PAS_ID, wcss->mem_region,
				     wcss->mem_phys, wcss->mem_size,
				     &wcss->mem_reloc);

	return qcom_mdt_load_no_init(wcss->dev, fw, rproc->firmware,
				     0, wcss->mem_region, wcss->mem_phys,
				     wcss->mem_size, &wcss->mem_reloc);
}

int q6v5_wcss_register_dump_segments(struct rproc *rproc,
					const struct firmware *fw)
{
	/*
	 * Registering custom coredump function with a dummy dump segment
	 * as the dump regions are taken care by the dump function itself
	 */
	return rproc_coredump_add_custom_segment(rproc, 0, 0, crashdump_init,
									NULL);
}

static void q6v5_wcss_panic(struct rproc *rproc)
{
	struct q6v5_wcss *wcss = rproc->priv;

	qcom_q6v5_panic_handler(&wcss->q6v5);
}

static const struct rproc_ops q6v5_wcss_ipq8074_ops = {
	.start = q6v5_wcss_start,
	.stop = q6v5_wcss_stop,
	.da_to_va = q6v5_wcss_da_to_va,
	.load = q6v5_wcss_load,
	.get_boot_addr = rproc_elf_get_boot_addr,
	.parse_fw = q6v5_wcss_register_dump_segments,
	.report_panic = q6v5_wcss_panic,
};

static const struct rproc_ops q6v5_wcss_qcs404_ops = {
	.start = q6v5_qcs404_wcss_start,
	.stop = q6v5_wcss_stop,
	.da_to_va = q6v5_wcss_da_to_va,
	.load = q6v5_wcss_load,
	.get_boot_addr = rproc_elf_get_boot_addr,
	.parse_fw = qcom_register_dump_segments,
};

static int q6v5_wcss_init_reset(struct q6v5_wcss *wcss,
				const struct wcss_data *desc)
{
	struct device *dev = wcss->dev;

	if (desc->aon_reset_required) {
		wcss->wcss_aon_reset = devm_reset_control_get_exclusive(dev, "wcss_aon_reset");;
		if (IS_ERR(wcss->wcss_aon_reset)) {
			dev_err(wcss->dev, "fail to acquire wcss_aon_reset\n");
			return PTR_ERR(wcss->wcss_aon_reset);
		}
	}

	wcss->wcss_reset = devm_reset_control_get_exclusive(dev, "wcss_reset");
	if (IS_ERR(wcss->wcss_reset)) {
		dev_err(wcss->dev, "unable to acquire wcss_reset\n");
		return PTR_ERR(wcss->wcss_reset);
	}

	if (desc->wcss_q6_reset_required) {
		wcss->wcss_q6_reset = devm_reset_control_get_exclusive(dev, "wcss_q6_reset");
		if (IS_ERR(wcss->wcss_q6_reset)) {
			dev_err(wcss->dev, "unable to acquire wcss_q6_reset\n");
			return PTR_ERR(wcss->wcss_q6_reset);
		}
	}

	if (desc->bcr_reset_required) {
		wcss->wcss_q6_bcr_reset = devm_reset_control_get_exclusive(dev,
									   "wcss_q6_bcr_reset");
		if (IS_ERR(wcss->wcss_q6_bcr_reset)) {
			dev_err(wcss->dev, "unable to acquire wcss_q6_bcr_reset\n");
			return PTR_ERR(wcss->wcss_q6_bcr_reset);
		}
	}

	if (desc->ce_reset_required) {
		wcss->ce_reset = devm_reset_control_get(dev, "ce_reset");
		if (IS_ERR(wcss->ce_reset)) {
			dev_err(wcss->dev, "unable to acquire ce_reset\n");
			return PTR_ERR(wcss->ce_reset);
		}
	}


	return 0;
}

static int q6v5_wcss_init_mmio(struct q6v5_wcss *wcss,
			       struct platform_device *pdev)
{
	unsigned int halt_reg[MAX_HALT_REG] = {0};
	struct device_node *syscon;
	struct resource *res;
	int ret;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "qdsp6");
	if (!res) {
		dev_err(&pdev->dev, "qdsp6 resource not available\n");
		return -EINVAL;
	}
	wcss->reg_base = devm_ioremap(&pdev->dev, res->start,
				      resource_size(res));
	if (IS_ERR(wcss->reg_base))
		return PTR_ERR(wcss->reg_base);

	if (wcss->version == WCSS_IPQ) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "rmb");
		wcss->rmb_base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(wcss->rmb_base))
			return PTR_ERR(wcss->rmb_base);

		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "wcmn");
		if (res) {
			wcss->wcmn_base = ioremap(res->start, resource_size(res));
			if (!wcss->wcmn_base) {
				dev_err(&pdev->dev, "wcmn ioremap failed\n");
				return -ENOMEM;
			}
		}
	}

	syscon = of_parse_phandle(pdev->dev.of_node,
				  "qcom,halt-regs", 0);
	if (!syscon) {
		dev_err(&pdev->dev, "failed to parse qcom,halt-regs\n");
		return -EINVAL;
	}

	wcss->halt_map = syscon_node_to_regmap(syscon);
	of_node_put(syscon);
	if (IS_ERR(wcss->halt_map))
		return PTR_ERR(wcss->halt_map);

	ret = of_property_read_variable_u32_array(pdev->dev.of_node,
						  "qcom,halt-regs",
						  halt_reg, 0,
						  MAX_HALT_REG);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to parse qcom,halt-regs\n");
		return -EINVAL;
	}

	wcss->halt_q6 = halt_reg[1];
	wcss->halt_wcss = halt_reg[2];
	wcss->halt_nc = halt_reg[3];

	return 0;
}

static int q6v5_alloc_memory_region(struct q6v5_wcss *wcss)
{
	struct reserved_mem *rmem = NULL;
	struct device_node *node;
	struct device *dev = wcss->dev;

	node = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (node)
		rmem = of_reserved_mem_lookup(node);

	of_node_put(node);

	if (!rmem) {
		dev_err(dev, "unable to acquire memory-region\n");
		return -EINVAL;
	}

	wcss->mem_phys = rmem->base;
	wcss->mem_reloc = rmem->base;
	wcss->mem_size = rmem->size;
	wcss->mem_region = devm_ioremap_wc(dev, wcss->mem_phys, wcss->mem_size);
	if (!wcss->mem_region) {
		dev_err(dev, "unable to map memory region: %pa+%pa\n",
			&rmem->base, &rmem->size);
		return -EBUSY;
	}

	return 0;
}

static int ipq9574_init_clock(struct q6v5_wcss *wcss)
{
	int i;
	int ret;
	const char* clks[] = { "anoc_wcss_axi_m", "q6_axim", "q6_ahb", "q6_ahb_s",
				"q6ss_boot", "wcss_ecahb", "wcss_acmt", "mem_noc_q6_axi", "wcss_q6_tbu", "sys_noc_wcss_ahb" };
	int num_clks = ARRAY_SIZE(clks);

	wcss->clks = devm_kcalloc(wcss->dev, num_clks, sizeof(*wcss->clks),
				  GFP_KERNEL);
	if (!wcss->clks)
		return -ENOMEM;

	for (i = 0; i < num_clks; i++)
		wcss->clks[i].id = clks[i];

	wcss->num_clks = num_clks;

	wcss->gcc_axim2_clk = devm_clk_get(wcss->dev, "q6_axim2");
	if (IS_ERR(wcss->gcc_axim2_clk)) {
		ret = PTR_ERR(wcss->gcc_axim2_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get gcc q6 axim2 clock");
		return PTR_ERR(wcss->gcc_axim2_clk);
	}

	wcss->gcc_abhs_cbcr = devm_clk_get(wcss->dev, "wcss_ahb_s");
	if (IS_ERR(wcss->gcc_abhs_cbcr)) {
		ret = PTR_ERR(wcss->gcc_abhs_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get gcc wcss ahb_s clock");
		return PTR_ERR(wcss->gcc_abhs_cbcr);
	}

	wcss->qdsp6ss_axim_cbcr = devm_clk_get(wcss->dev, "wcss_axi_m");
	if (IS_ERR(wcss->qdsp6ss_axim_cbcr)) {
		ret = PTR_ERR(wcss->qdsp6ss_axim_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get gcc wcss axi_m clock");
		return PTR_ERR(wcss->qdsp6ss_axim_cbcr);
	}

	return devm_clk_bulk_get(wcss->dev, num_clks, wcss->clks);
}

static int ipq5018_init_clock(struct q6v5_wcss *wcss)
{
	int ret;

	wcss->gcc_abhs_cbcr = devm_clk_get(wcss->dev, "gcc_wcss_ahb_s_clk");
	if (IS_ERR(wcss->gcc_abhs_cbcr)) {
		ret = PTR_ERR(wcss->gcc_abhs_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get gcc ahbs clock");
		return PTR_ERR(wcss->gcc_abhs_cbcr);
	}

	wcss->gcc_axim_cbcr = devm_clk_get(wcss->dev, "gcc_wcss_axi_m_clk");
	if (IS_ERR(wcss->gcc_axim_cbcr)) {
		ret = PTR_ERR(wcss->gcc_axim_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get gcc axim clock\n");
		return PTR_ERR(wcss->gcc_axim_cbcr);
	}

	wcss->ahbs_cbcr = devm_clk_get(wcss->dev, "gcc_q6_ahb_s_clk");
	if (IS_ERR(wcss->ahbs_cbcr)) {
		ret = PTR_ERR(wcss->ahbs_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get ahbs clock\n");
		return PTR_ERR(wcss->ahbs_cbcr);
	}

	wcss->qdsp6ss_abhm_cbcr = devm_clk_get(wcss->dev, "gcc_q6_ahb_clk");
	if (IS_ERR(wcss->qdsp6ss_abhm_cbcr)) {
		ret = PTR_ERR(wcss->qdsp6ss_abhm_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get csr cbcr clk\n");
		return PTR_ERR(wcss->qdsp6ss_abhm_cbcr);
	}

	wcss->eachb_clk = devm_clk_get(wcss->dev, "gcc_wcss_ecahb_clk");
	if (IS_ERR(wcss->eachb_clk)) {
		ret = PTR_ERR(wcss->eachb_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get ahbs_cbcr clk\n");
		return PTR_ERR(wcss->eachb_clk);
	}

	wcss->acmt_clk = devm_clk_get(wcss->dev, "gcc_wcss_acmt_clk");
	if (IS_ERR(wcss->acmt_clk)) {
		ret = PTR_ERR(wcss->acmt_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get acmt clk\n");
		return PTR_ERR(wcss->acmt_clk);
	}

	wcss->gcc_axim2_clk = devm_clk_get(wcss->dev, "gcc_q6_axim2_clk");
	if (IS_ERR(wcss->gcc_axim2_clk)) {
		ret = PTR_ERR(wcss->gcc_axim2_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get gcc_axim2_clk\n");
		return PTR_ERR(wcss->gcc_axim2_clk);
	}

	wcss->axmis_clk = devm_clk_get(wcss->dev, "gcc_q6_axis_clk");
	if (IS_ERR(wcss->axmis_clk)) {
		ret = PTR_ERR(wcss->axmis_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get axmis  clk\n");
		return PTR_ERR(wcss->axmis_clk);
	}

	wcss->axi_s_clk = devm_clk_get(wcss->dev, "gcc_wcss_axi_s_clk");
	if (IS_ERR(wcss->axi_s_clk)) {
		ret = PTR_ERR(wcss->axi_s_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get axi_s_clk clk\n");
		return PTR_ERR(wcss->axi_s_clk);
	}

	wcss->qdsp6ss_axim_cbcr = devm_clk_get(wcss->dev, "gcc_q6_axim_clk");
	if (IS_ERR(wcss->qdsp6ss_axim_cbcr)) {
		ret = PTR_ERR(wcss->qdsp6ss_axim_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to axim clk\n");
		return PTR_ERR(wcss->qdsp6ss_axim_cbcr);
	}

	return 0;
}

static int ipq8074_init_clock(struct q6v5_wcss *wcss)
{
	int ret;

	wcss->prng_clk = devm_clk_get(wcss->dev, "prng");
	if (IS_ERR(wcss->prng_clk)) {
		ret = PTR_ERR(wcss->prng_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "Failed to get prng clock\n");
		return ret;
	}
	return 0;
}

static int ipq6018_init_clock(struct q6v5_wcss *wcss)
{
	int ret;

	wcss->prng_clk = devm_clk_get(wcss->dev, "prng");
	if (IS_ERR(wcss->prng_clk)) {
		ret = PTR_ERR(wcss->prng_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "Failed to get prng clock\n");
		return ret;
	}

	wcss->gcc_sys_noc_wcss_ahb_clk =
			devm_clk_get(wcss->dev, "gcc_sys_noc_wcss_ahb_clk");
	if (IS_ERR(wcss->gcc_sys_noc_wcss_ahb_clk)) {
		ret = PTR_ERR(wcss->gcc_sys_noc_wcss_ahb_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "Failed to get sys_noc_wcss_ahb clock\n");
		return ret;
	}

	wcss->gcc_q6ss_atbm_clk =
			devm_clk_get(wcss->dev, "gcc_q6ss_atbm_clk");
	if (IS_ERR(wcss->gcc_q6ss_atbm_clk)) {
		ret = PTR_ERR(wcss->gcc_q6ss_atbm_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "Failed to get q6ss_atbm clock\n");
		return ret;
	}

	wcss->gcc_q6ss_pclkdbg_clk =
			devm_clk_get(wcss->dev, "gcc_q6ss_pclkdbg_clk");
	if (IS_ERR(wcss->gcc_q6ss_pclkdbg_clk)) {
		ret = PTR_ERR(wcss->gcc_q6ss_pclkdbg_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "Failed to get q6ss_pclkdbg clock\n");
		return ret;
	}

	wcss->gcc_q6_tsctr_1to2_clk =
			devm_clk_get(wcss->dev, "gcc_q6_tsctr_1to2_clk");
	if (IS_ERR(wcss->gcc_q6_tsctr_1to2_clk)) {
		ret = PTR_ERR(wcss->gcc_q6_tsctr_1to2_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "Failed to get q6_tsctr_1to2 clock\n");
		return ret;
	}

	return 0;
}

static int qcs404_init_clock(struct q6v5_wcss *wcss)
{
	int ret;

	wcss->xo = devm_clk_get(wcss->dev, "xo");
	if (IS_ERR(wcss->xo)) {
		ret = PTR_ERR(wcss->xo);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get xo clock");
		return ret;
	}

	wcss->gcc_abhs_cbcr = devm_clk_get(wcss->dev, "gcc_abhs_cbcr");
	if (IS_ERR(wcss->gcc_abhs_cbcr)) {
		ret = PTR_ERR(wcss->gcc_abhs_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get gcc abhs clock");
		return PTR_ERR(wcss->gcc_abhs_cbcr);
	}

	wcss->gcc_axim_cbcr = devm_clk_get(wcss->dev, "gcc_axim_cbcr");
	if (IS_ERR(wcss->gcc_axim_cbcr)) {
		ret = PTR_ERR(wcss->gcc_axim_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get gcc axim clock\n");
		return PTR_ERR(wcss->gcc_axim_cbcr);
	}

	wcss->ahbfabric_cbcr_clk = devm_clk_get(wcss->dev,
						"lcc_ahbfabric_cbc");
	if (IS_ERR(wcss->ahbfabric_cbcr_clk)) {
		ret = PTR_ERR(wcss->ahbfabric_cbcr_clk);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get ahbfabric clock\n");
		return PTR_ERR(wcss->ahbfabric_cbcr_clk);
	}

	wcss->lcc_csr_cbcr = devm_clk_get(wcss->dev, "tcsr_lcc_cbc");
	if (IS_ERR(wcss->lcc_csr_cbcr)) {
		ret = PTR_ERR(wcss->lcc_csr_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get csr cbcr clk\n");
		return PTR_ERR(wcss->lcc_csr_cbcr);
	}

	wcss->ahbs_cbcr = devm_clk_get(wcss->dev,
				       "lcc_abhs_cbc");
	if (IS_ERR(wcss->ahbs_cbcr)) {
		ret = PTR_ERR(wcss->ahbs_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get ahbs_cbcr clk\n");
		return PTR_ERR(wcss->ahbs_cbcr);
	}

	wcss->tcm_slave_cbcr = devm_clk_get(wcss->dev,
					    "lcc_tcm_slave_cbc");
	if (IS_ERR(wcss->tcm_slave_cbcr)) {
		ret = PTR_ERR(wcss->tcm_slave_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get tcm cbcr clk\n");
		return PTR_ERR(wcss->tcm_slave_cbcr);
	}

	wcss->qdsp6ss_abhm_cbcr = devm_clk_get(wcss->dev, "lcc_abhm_cbc");
	if (IS_ERR(wcss->qdsp6ss_abhm_cbcr)) {
		ret = PTR_ERR(wcss->qdsp6ss_abhm_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get abhm cbcr clk\n");
		return PTR_ERR(wcss->qdsp6ss_abhm_cbcr);
	}

	wcss->qdsp6ss_axim_cbcr = devm_clk_get(wcss->dev, "lcc_axim_cbc");
	if (IS_ERR(wcss->qdsp6ss_axim_cbcr)) {
		ret = PTR_ERR(wcss->qdsp6ss_axim_cbcr);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get axim cbcr clk\n");
		return PTR_ERR(wcss->qdsp6ss_abhm_cbcr);
	}

	wcss->lcc_bcr_sleep = devm_clk_get(wcss->dev, "lcc_bcr_sleep");
	if (IS_ERR(wcss->lcc_bcr_sleep)) {
		ret = PTR_ERR(wcss->lcc_bcr_sleep);
		if (ret != -EPROBE_DEFER)
			dev_err(wcss->dev, "failed to get bcr cbcr clk\n");
		return PTR_ERR(wcss->lcc_bcr_sleep);
	}

	return 0;
}

static int qcs404_init_regulator(struct q6v5_wcss *wcss)
{
	wcss->cx_supply = devm_regulator_get(wcss->dev, "cx");
	if (IS_ERR(wcss->cx_supply))
		return PTR_ERR(wcss->cx_supply);

	regulator_set_load(wcss->cx_supply, 100000);

	return 0;
}

static int q6v5_wcss_probe(struct platform_device *pdev)
{
	const struct wcss_data *desc;
	struct q6v5_wcss *wcss;
	struct rproc *rproc;
	int ret;

	desc = of_device_get_match_data(&pdev->dev);
	if (!desc)
		return -EINVAL;

	if (desc->need_mem_protection && !qcom_scm_is_available())
		return -EPROBE_DEFER;

	rproc = rproc_alloc(&pdev->dev, pdev->name, desc->ops,
			    desc->q6_firmware_name, sizeof(*wcss));
	if (!rproc) {
		dev_err(&pdev->dev, "failed to allocate rproc\n");
		return -ENOMEM;
	}

	wcss = rproc->priv;
	wcss->dev = &pdev->dev;
	wcss->version = desc->version;

	wcss->version = desc->version;
	wcss->requires_force_stop = desc->requires_force_stop;
	wcss->need_mem_protection = desc->need_mem_protection;
	wcss->m3_firmware_name = desc->m3_firmware_name;
	wcss->reset_cmd_id = desc->reset_cmd_id;
	wcss->q6_version = desc->q6_version;

	if (of_property_read_bool(pdev->dev.of_node, "qcom,nosecure"))
		wcss->need_mem_protection = false;

	ret = q6v5_wcss_init_mmio(wcss, pdev);
	if (ret)
		goto free_rproc;

	ret = q6v5_alloc_memory_region(wcss);
	if (ret)
		goto free_rproc;

	if (desc->init_clock) {
		ret = desc->init_clock(wcss);
		if (ret)
			goto free_rproc;
	}

	if (desc->init_regulator) {
		ret = desc->init_regulator(wcss);
		if (ret)
			goto free_rproc;
	}

	ret = q6v5_wcss_init_reset(wcss, desc);
	if (ret)
		goto free_rproc;

	wcss->backdoor = of_property_read_bool(pdev->dev.of_node,
					       "qcom,emulation");
	if (wcss->backdoor) {
		ret = of_property_read_u32(pdev->dev.of_node, "bootaddr",
					    &rproc->bootaddr);
		if (ret) {
			dev_err(&pdev->dev, "boot addr required for emulation\n");
			goto free_rproc;
		}
	}

	ret = qcom_q6v5_init(&wcss->q6v5, pdev, rproc, desc->remote_id,
			     desc->crash_reason_smem, NULL);
	if (ret)
		goto free_rproc;

	qcom_add_glink_subdev(rproc, &wcss->glink_subdev);
	qcom_add_ssr_subdev(rproc, &wcss->ssr_subdev, desc->ssr_name);
	if (desc->sysmon_name)
		wcss->sysmon = qcom_add_sysmon_subdev(rproc,
						      desc->sysmon_name,
						      desc->ssctl_id);

	rproc->auto_boot = desc->need_auto_boot;
	rproc->backdoor = wcss->backdoor;
	ret = rproc_add(rproc);
	if (ret)
		goto free_rproc;

	platform_set_drvdata(pdev, rproc);

	return 0;

free_rproc:
	rproc_free(rproc);

	return ret;
}

static int q6v5_wcss_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct q6v5_wcss *wcss = rproc->priv;

	if (wcss->wcmn_base)
		iounmap(wcss->wcmn_base);
	rproc_del(rproc);
	rproc_free(rproc);

	return 0;
}

static const struct wcss_data wcss_ipq8074_res_init = {
	.init_clock = ipq8074_init_clock,
	.q6_firmware_name = "IPQ8074/q6_fw.mdt",
	.m3_firmware_name = "IPQ8074/m3_fw.mdt",
	.crash_reason_smem = WCSS_CRASH_REASON,
	.remote_id = QCOM_SMEM_HOST_ANY,
	.aon_reset_required = true,
	.wcss_q6_reset_required = true,
	.bcr_reset_required = false,
	.ssr_name = "q6wcss",
	.reset_cmd_id = 0x14,
	.ops = &q6v5_wcss_ipq8074_ops,
	.requires_force_stop = true,
	.need_mem_protection = true,
	.need_auto_boot = false,
	.q6_version = Q6V5,
	.q6ss_version = QDSP6SS_Q6V5_VERSION,
	.wcss_version = WCSS_VERSION,
};

static const struct wcss_data wcss_ipq6018_res_init = {
	.init_clock = ipq6018_init_clock,
	.q6_firmware_name = "IPQ6018/q6_fw.mdt",
	.m3_firmware_name = "IPQ6018/m3_fw.mdt",
	.crash_reason_smem = WCSS_CRASH_REASON,
	.remote_id = QCOM_SMEM_HOST_ANY,
	.aon_reset_required = true,
	.wcss_q6_reset_required = true,
	.bcr_reset_required = false,
	.ce_reset_required = false,
	.ssr_name = "q6wcss",
	.reset_cmd_id = 0x18,
	.ops = &q6v5_wcss_ipq8074_ops,
	.requires_force_stop = false,
	.need_mem_protection = true,
	.need_auto_boot = false,
	.q6_version = Q6V5,
	.q6ss_version = QDSP6SS_Q6V5_VERSION,
	.wcss_version = WCSS_VERSION,
};

static const struct wcss_data wcss_ipq5018_res_init = {
	.init_clock = ipq5018_init_clock,
	.q6_firmware_name = "IPQ5018/q6_fw.mdt",
	.m3_firmware_name = "IPQ5018/m3_fw.mdt",
	.crash_reason_smem = WCSS_CRASH_REASON,
	.remote_id = QCOM_SMEM_HOST_ANY,
	.aon_reset_required = true,
	.wcss_q6_reset_required = true,
	.bcr_reset_required = false,
	.ce_reset_required = true,
	.ssr_name = "q6wcss",
	.reset_cmd_id = 0x14,
	.ops = &q6v5_wcss_ipq8074_ops,
	.requires_force_stop = true,
	.need_mem_protection = true,
	.need_auto_boot = false,
	.q6_version = Q6V6,
	.q6ss_version = QDSP6SS_Q6V6_VERSION,
	.wcss_version = WCSS_VERSION,
};

static const struct wcss_data wcss_ipq9574_res_init = {
	.init_clock = ipq9574_init_clock,
	.q6_firmware_name = "IPQ9574/q6_fw.mdt",
	.m3_firmware_name = "IPQ9574/m3_fw.mdt",
	.crash_reason_smem = WCSS_CRASH_REASON,
	.remote_id = WCSS_SMEM_HOST,
	.aon_reset_required = true,
	.wcss_q6_reset_required = true,
	.bcr_reset_required = false,
	.ce_reset_required = false,
	.ssr_name = "q6wcss",
	.reset_cmd_id = 0x18,
	.ops = &q6v5_wcss_ipq8074_ops,
	.requires_force_stop = true,
	.need_mem_protection = true,
	.need_auto_boot = false,
	.q6_version = Q6V7,
	.q6ss_version = QDSP6SS_Q6V7_VERSION,
	.wcss_version = WCSS_VERSION,
	.clear_smp2p_last_value = true,
};

static const struct wcss_data wcss_qcs404_res_init = {
	.init_clock = qcs404_init_clock,
	.init_regulator = qcs404_init_regulator,
	.crash_reason_smem = WCSS_CRASH_REASON,
	.remote_id = QCOM_SMEM_HOST_ANY,
	.q6_firmware_name = "wcnss.mdt",
	.version = WCSS_QCS404,
	.aon_reset_required = false,
	.wcss_q6_reset_required = false,
	.bcr_reset_required = true,
	.ce_reset_required = false,
	.ssr_name = "mpss",
	.sysmon_name = "wcnss",
	.ssctl_id = 0x12,
	.ops = &q6v5_wcss_qcs404_ops,
	.requires_force_stop = false,
	.reset_cmd_id = 0x14,
	.need_auto_boot = true,
	.q6_version = Q6V5,
};

static const struct of_device_id q6v5_wcss_of_match[] = {
	{ .compatible = "qcom,ipq8074-wcss-pil", .data = &wcss_ipq8074_res_init },
	{ .compatible = "qcom,ipq6018-wcss-pil", .data = &wcss_ipq6018_res_init },
	{ .compatible = "qcom,ipq9574-wcss-pil", .data = &wcss_ipq9574_res_init },
	{ .compatible = "qcom,qcs404-wcss-pil", .data = &wcss_qcs404_res_init },
	{ .compatible = "qcom,qcs5018-wcss-pil", .data = &wcss_ipq5018_res_init },

	{ },
};
MODULE_DEVICE_TABLE(of, q6v5_wcss_of_match);

static struct platform_driver q6v5_wcss_driver = {
	.probe = q6v5_wcss_probe,
	.remove = q6v5_wcss_remove,
	.driver = {
		.name = "qcom-q6v5-wcss-pil",
		.of_match_table = q6v5_wcss_of_match,
	},
};
module_platform_driver(q6v5_wcss_driver);
module_param(debug_wcss, int, 0644);

MODULE_DESCRIPTION("Hexagon WCSS Peripheral Image Loader");
MODULE_LICENSE("GPL v2");
