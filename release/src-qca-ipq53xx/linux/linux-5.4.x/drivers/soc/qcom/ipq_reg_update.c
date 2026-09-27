/*
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/qcom_scm.h>
#include <soc/qcom/socinfo.h>

/* NoC Control Offsets */
#define MEM_NOC_XM_APP0_QOSGEN_MAINCTL_LOW 0x6088
#define MEM_NOC_XM_APP0_QOSGEN_REGUL0CTL_LOW 0x60C0
#define MEM_NOC_XM_APP0_QOSGEN_REGUL0BW_LOW 0x60C8
#define MEM_NOC_QXM_WCSS_Q6_QOSGEN_MAINCTL_LOW 0x6188
#define MEM_NOC_QXM_WCSS_Q6_QOSGEN_REGUL0CTL_LOW 0x61C0
#define MEM_NOC_QXM_WCSS_Q6_QOSGEN_REGUL0BW_LOW 0x61c8
#define NSS_NOC_XM_PPE_QOSGEN_MAINCTL_LOW 0x3008
#define NSS_NOC_XM_PPE_QOSGEN_REGUL0CTL_LOW 0x3040
#define NSS_NOC_XM_PPE_QOSGEN_REGUL0BW_LOW 0x3048

/* NoC register values */
#define MAINCTL_LOW_VAL 0x70
#define REGUL0CTL_LOW_VAL 0x7703
#define REGUL0BW_LOW_VAL 0x3FF0FFF
#define MEMNOC_53xx_APP0_MAINCTL_LOW_VAL 0x30
#define MEMNOC_53xx_APP0_REGUL0CTL_LOW_VAL 0x3303
#define MEMNOC_53xx_APP0_REGUL0BW_LOW_VAL 0x1000FFF
#define MEMNOC_53xx_WCSS_Q6_MAINCTL_LOW_VAL 0x38
#define MEMNOC_53xx_WCSS_Q6_REGUL0CTL_LOW_VAL 0x1103
#define MEMNOC_53xx_WCSS_Q6_PRIO_3_MAINCTL_LOW_VAL 0x30
#define MEMNOC_53xx_WCSS_Q6_PRIO_3_REGUL0CTL_LOW_VAL 0x3303
#define MEMNOC_53xx_WCSS_Q6_REGUL0BW_LOW_VAL 0x10007FF
#define NSSNOC_53xx_PPE_MAINCTL_LOW_VAL 0x20
#define NSSNOC_53xx_PPE_REGUL0CTL_LOW_VAL 0x2203
#define NSSNOC_53xx_PPE_REGUL0BW_LOW_VAL 0x1000FFF

/* UBI Control Offsets */
#define UBI_C1_GDS_CTRL_REQ 0x4
#define UBI_C2_GDS_CTRL_REQ 0x8
#define UBI_C3_GDS_CTRL_REQ 0xC
#define UBI32_CORE_GDS_COLLAPSE_EN_SW 0x1 << 28

static void __iomem *memnoc_base;
static void __iomem *nssnoc_base;
struct kobject *reg_update_kobj;

static void configure_noc_priority_settings(void)
{
	/* APSS-3, NSS-2, Q6-1 Priority configuration */
	writel(MEMNOC_53xx_APP0_MAINCTL_LOW_VAL, memnoc_base +
		MEM_NOC_XM_APP0_QOSGEN_MAINCTL_LOW);
	writel(MEMNOC_53xx_APP0_REGUL0CTL_LOW_VAL, memnoc_base +
		MEM_NOC_XM_APP0_QOSGEN_REGUL0CTL_LOW);
	writel(MEMNOC_53xx_APP0_REGUL0BW_LOW_VAL, memnoc_base +
		MEM_NOC_XM_APP0_QOSGEN_REGUL0BW_LOW);
	writel(MEMNOC_53xx_WCSS_Q6_MAINCTL_LOW_VAL, memnoc_base +
		MEM_NOC_QXM_WCSS_Q6_QOSGEN_MAINCTL_LOW);
	writel(MEMNOC_53xx_WCSS_Q6_REGUL0CTL_LOW_VAL, memnoc_base +
		MEM_NOC_QXM_WCSS_Q6_QOSGEN_REGUL0CTL_LOW);
	writel(MEMNOC_53xx_WCSS_Q6_REGUL0BW_LOW_VAL, memnoc_base +
		MEM_NOC_QXM_WCSS_Q6_QOSGEN_REGUL0BW_LOW);
	writel(NSSNOC_53xx_PPE_MAINCTL_LOW_VAL, nssnoc_base +
		NSS_NOC_XM_PPE_QOSGEN_MAINCTL_LOW);
	writel(NSSNOC_53xx_PPE_REGUL0CTL_LOW_VAL, nssnoc_base +
		NSS_NOC_XM_PPE_QOSGEN_REGUL0CTL_LOW);
	writel(NSSNOC_53xx_PPE_REGUL0BW_LOW_VAL, nssnoc_base +
		NSS_NOC_XM_PPE_QOSGEN_REGUL0BW_LOW);
}

static void configure_noc_priority_settings_to_por(void)
{
	/* Configure noc priority settings to default PoR values */
	writel(0x0, memnoc_base + MEM_NOC_XM_APP0_QOSGEN_MAINCTL_LOW);
	writel(0x0, memnoc_base + MEM_NOC_XM_APP0_QOSGEN_REGUL0CTL_LOW);
	writel(0x0, memnoc_base + MEM_NOC_XM_APP0_QOSGEN_REGUL0BW_LOW);
	writel(0x8, memnoc_base + MEM_NOC_QXM_WCSS_Q6_QOSGEN_MAINCTL_LOW);
	writel(0x0, memnoc_base + MEM_NOC_QXM_WCSS_Q6_QOSGEN_REGUL0CTL_LOW);
	writel(0x0, memnoc_base + MEM_NOC_QXM_WCSS_Q6_QOSGEN_REGUL0BW_LOW);
	writel(0x0, nssnoc_base + NSS_NOC_XM_PPE_QOSGEN_MAINCTL_LOW);
	writel(0x0, nssnoc_base + NSS_NOC_XM_PPE_QOSGEN_REGUL0CTL_LOW);
	writel(0x0, nssnoc_base + NSS_NOC_XM_PPE_QOSGEN_REGUL0BW_LOW);
}

static ssize_t store_noc_register_configuration(struct kobject *k,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int sfe_mode, ret;

	ret = kstrtouint(buf, 0, &sfe_mode);
	if (ret)
		return ret;

	if (cpu_is_ipq5332() || cpu_is_ipq5322() || cpu_is_ipq5300()) {
		if (sfe_mode == 1)
			configure_noc_priority_settings();
		else if (sfe_mode == 0)
			configure_noc_priority_settings_to_por();
		else
			pr_err("Invalid input!! Please provide 1 for SFE mode"
				"configuration or 0 for resetting to PoR values\n");
	}
	else {
		pr_info("SKU is Nominal, NoC settings are not applied\n");
	}

	return count;
}

static struct kobj_attribute noc_reg_update_attr =
        __ATTR(enable_sfe_mode, 0300, NULL, store_noc_register_configuration);

static int reg_update_probe(struct platform_device *pdev)
{
	struct resource *res;
	void __iomem *base;
	void __iomem *ubi_c0_gds;
	struct clk *nss_csr_clk;
	struct clk *nssnoc_nss_csr_clk;
	int ret;
	struct device_node *np = (&pdev->dev)->of_node;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "memnoc");
	if(res) {
		base = devm_ioremap_resource(&pdev->dev, res);
		if(IS_ERR(base))
			return PTR_ERR(base);
		writel(MAINCTL_LOW_VAL, base + MEM_NOC_XM_APP0_QOSGEN_MAINCTL_LOW);
		writel(REGUL0CTL_LOW_VAL, base + MEM_NOC_XM_APP0_QOSGEN_REGUL0CTL_LOW);
		writel(REGUL0BW_LOW_VAL, base + MEM_NOC_XM_APP0_QOSGEN_REGUL0BW_LOW);
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "memnoc_53xx");
	if(res) {
		memnoc_base = devm_ioremap_resource(&pdev->dev, res);
		if(IS_ERR(memnoc_base))
			return PTR_ERR(memnoc_base);
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "nssnoc");
	if(res) {
		nssnoc_base = devm_ioremap_resource(&pdev->dev, res);
		if(IS_ERR(nssnoc_base))
			return PTR_ERR(nssnoc_base);
	}

	if ((of_device_is_compatible(np, "ipq,reg_update")) &&
			(!of_property_read_bool(np, "ubi_core_enable"))) {
		/* Enabling NSS CSR clocks to access the UBI Power collapse registers */
		nss_csr_clk = devm_clk_get(&pdev->dev, "nss-csr-clk");
		if (IS_ERR(nss_csr_clk)) {
			ret = PTR_ERR(nss_csr_clk);
			pr_debug("Failed to get nss-csr-clk\n");
			goto err_out;
		}
		nssnoc_nss_csr_clk = devm_clk_get(&pdev->dev, "nss-nssnoc-csr-clk");
		if (IS_ERR(nssnoc_nss_csr_clk)) {
			ret = PTR_ERR(nssnoc_nss_csr_clk);
			pr_debug("Failed to get nss-nssnoc-csr-clk\n");
			goto err_out;
		}
		ret = clk_prepare_enable(nss_csr_clk);
		if(ret)
			goto err_out;
		ret = clk_prepare_enable(nssnoc_nss_csr_clk);
		if (ret)
			goto err_out;
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ubicore");
		if(res) {
			ubi_c0_gds = devm_ioremap_resource(&pdev->dev, res);
			if(IS_ERR(ubi_c0_gds)) {
				pr_debug("ubicore ioremap failed\n");
				goto err_out;
			}
			/* Power collapsing the 4 UBI32 Cores as it is not used
			   in IPQ9574 except for AL02-C10 RDP */
			writel(readl(ubi_c0_gds) | UBI32_CORE_GDS_COLLAPSE_EN_SW, ubi_c0_gds);
			writel(readl(ubi_c0_gds + UBI_C1_GDS_CTRL_REQ) |
				UBI32_CORE_GDS_COLLAPSE_EN_SW, ubi_c0_gds + UBI_C1_GDS_CTRL_REQ);
			writel(readl(ubi_c0_gds + UBI_C2_GDS_CTRL_REQ) |
				UBI32_CORE_GDS_COLLAPSE_EN_SW, ubi_c0_gds + UBI_C2_GDS_CTRL_REQ);
			writel(readl(ubi_c0_gds + UBI_C3_GDS_CTRL_REQ) |
				UBI32_CORE_GDS_COLLAPSE_EN_SW, ubi_c0_gds + UBI_C3_GDS_CTRL_REQ);
			pr_info("UBI cores power collapsed successfully\n");
		}
	}
	else if (of_device_is_compatible(np, "ipq,53xx-reg-update")) {
		if (!reg_update_kobj)
			reg_update_kobj = kobject_create_and_add("noc_reg_update", kernel_kobj);
		if (sysfs_create_file(reg_update_kobj, &noc_reg_update_attr.attr))
			pr_err("Failed to create sysfs entry for configuring noc registers\n");
		/* Configuring the Q6 Priority to 3 for IPQ5321 SKU */
		if (cpu_is_ipq5321()) {
			pr_info("Bumping up the Q6 Priority to 3\n");
			writel(MEMNOC_53xx_WCSS_Q6_PRIO_3_MAINCTL_LOW_VAL,
				memnoc_base + MEM_NOC_QXM_WCSS_Q6_QOSGEN_MAINCTL_LOW);
			writel(MEMNOC_53xx_WCSS_Q6_PRIO_3_REGUL0CTL_LOW_VAL,
				memnoc_base + MEM_NOC_QXM_WCSS_Q6_QOSGEN_REGUL0CTL_LOW);
			writel(MEMNOC_53xx_WCSS_Q6_REGUL0BW_LOW_VAL,
				memnoc_base + MEM_NOC_QXM_WCSS_Q6_QOSGEN_REGUL0BW_LOW);
		}
	}
	else {
		pr_info("Skipping UBI power collapse\n");
	}
	return 0;

err_out:
	pr_err("Failed to power collapse UBI\n");
	return (ret);

}

static const struct of_device_id reg_update_dt_match[] = {
	{ .compatible = "ipq,reg_update", },
	{ .compatible = "ipq,53xx-reg-update", },
	{ },
};

MODULE_DEVICE_TABLE(of, reg_update_dt_match);

static struct platform_driver reg_update_driver = {
	.driver = {
		.name	= "reg_update",
		.owner	= THIS_MODULE,
		.of_match_table	= reg_update_dt_match,
	},
	.probe = reg_update_probe,
};

module_platform_driver(reg_update_driver);
