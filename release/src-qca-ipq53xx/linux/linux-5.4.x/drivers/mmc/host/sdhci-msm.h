/*
 * Copyright (c) 2015, 2020 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SDHCI_MSM_H__
#define __SDHCI_MSM_H__

#include <linux/mmc/mmc.h>
#include "sdhci-pltfm.h"

struct sdhci_msm_variant_ops {
	u32 (*msm_readl_relaxed)(struct sdhci_host *host, u32 offset);
	void (*msm_writel_relaxed)(u32 val, struct sdhci_host *host,
			u32 offset);
};

/*
 * From V5, register spaces have changed. Wrap this info in a structure
 * and choose the data_structure based on version info mentioned in DT.
 */
struct sdhci_msm_variant_info {
	bool mci_removed;
	bool restore_dll_config;
	const struct sdhci_msm_variant_ops *var_ops;
	const struct sdhci_msm_offset *offset;
};

struct sdhci_msm_ice_data {
        struct qcom_ice_variant_ops *vops;
        struct platform_device *pdev;
        int state;
};

struct sdhci_msm_host {
	struct platform_device *pdev;
	void __iomem *core_mem;	/* MSM SDCC mapped address */
	void __iomem *cryptoio;    /* ICE HCI mapped address */
	int pwr_irq;		/* power irq */
	bool ice_hci_support;
	struct clk *bus_clk;	/* SDHC bus voter clock */
	struct clk *xo_clk;	/* TCXO clk needed for FLL feature of cm_dll*/
	struct clk_bulk_data bulk_clks[4]; /* core, iface, cal, sleep clocks */
	struct clk *ice_clk; /* SDHC peripheral ICE clock */
	unsigned long clk_rate;
	struct mmc_host *mmc;
	bool use_14lpp_dll_reset;
	bool tuning_done;
	bool calibration_done;
	u8 saved_tuning_phase;
	bool use_cdclp533;
	u32 curr_pwr_state;
	u32 curr_io_level;
	wait_queue_head_t pwr_irq_wait;
	bool pwr_irq_flag;
	u32 caps_0;
	bool mci_removed;
	bool restore_dll_config;
	const struct sdhci_msm_variant_ops *var_ops;
	const struct sdhci_msm_offset *offset;
	struct sdhci_msm_ice_data ice;
	u32 ice_clk_rate;
	bool use_cdr;
	u32 transfer_mode;
	bool updated_ddr_cfg;
	u32 ice_clk_min;
	u32 ice_clk_max;
};
#endif /* __SDHCI_MSM_H__ */
