// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <soc/qcom/socinfo.h>

#include "cpr3-regulator.h"

/**
 * struct gpio_regulator_data - gpio regulator data structure
 * @regulator_name:	Regulator name which needs to be controlled
 * @volt_fuse_param:	Voltage tick fuse details of the regulator
 * @cpr_rev:		CPR revision fuse details
 * @reference_volt:	Reference voltage used to calculate fused volatage
 * @step_volt:		Step size of fused voltage ticks
 * @min_volt:		Voltage used for Fast parts
 * @max_volt:		Voltage used for Slow parts
 * @threshold_volt:	Voltage threshold steps needs to be compared with
 * 			fuse voltage from nvmem
 */
struct gpio_regulator_data {
	const char *regulator_name;
	struct cpr3_fuse_param *volt_fuse_param;
	struct cpr3_fuse_param *cpr_rev;
	int reference_volt;
	int step_volt;
	int min_volt;
	int max_volt;
	int threshold_volt;
};

static struct cpr3_fuse_param ipq9574_apc_fuse[] = {
	{104, 18, 23},
	{},
};

static struct cpr3_fuse_param ipq9574_cx_fuse[] = {
	{103, 15, 19},
	{},
};

static struct cpr3_fuse_param ipq9574_mx_fuse[] = {
	{103, 37, 41},
	{},
};

static struct cpr3_fuse_param ipq9574_cpr_rev[] = {
	{107, 8, 10},
	{},
};

static const struct gpio_regulator_data ipq9574_gpio_regulator_data[] = {
	{"apc", ipq9574_apc_fuse, ipq9574_cpr_rev, 862500, 10000, 850000, 925000, 800000},
	{"cx", ipq9574_cx_fuse, ipq9574_cpr_rev, 800000, 10000, 800000, 863000, 800000},
	{"mx", ipq9574_mx_fuse, ipq9574_cpr_rev, 850000, 10000, 850000, 925000, 850000},
	{ }
};

static struct cpr3_fuse_param ipq9574_4state_apc_sturbo[] = {
	{104, 12, 17},
	{},
};

static struct cpr3_fuse_param ipq9574_4state_cx_nom[] = {
	{103, 37, 41},
	{},
};

static const struct gpio_regulator_data ipq9574_4state_gpio_regulator_data[] = {
	{"apc", ipq9574_4state_apc_sturbo, ipq9574_cpr_rev, 1062500, 10000, 1004000, 1068000, 1000000},
	{"cx", ipq9574_4state_cx_nom, ipq9574_cpr_rev, 850000, 10000, 850000, 910000, 850000},
	{ }
};

static struct of_device_id gpio_regulator_match_table[] = {
	{
		.compatible = "qcom,ipq9574-gpio-regulator",
		.data = &ipq9574_gpio_regulator_data
	},
	{
		.compatible = "qcom,ipq9574-4state-gpio-regulator",
		.data = &ipq9574_4state_gpio_regulator_data
	},
	{}
};

static int gpio_regulator_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct of_device_id *match;
	struct resource *res;
	void __iomem *fuse_base;
	struct gpio_regulator_data *reg_data;
	struct regulator *gpio_regulator;
	u64 volt_ticks;
	u64 cpr_fuse;
	int fused_volt;
	int volt_select;
	bool fix_volt_max = false;
	int ret = -EINVAL;;

	match = of_match_device(gpio_regulator_match_table, dev);
	if (!match)
		return -ENODEV;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "fuse_base");
	if (!res || !res->start) {
		dev_err(dev, "fuse base address is missing\n");
		return -ENXIO;
	}

	if (device_property_read_bool(dev, "skip-voltage-scaling-turboL1-sku-quirk")) {
		if(cpu_is_ipq9574() || cpu_is_ipq9570())
			fix_volt_max = true;
	}

	fuse_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(fuse_base))
		return PTR_ERR(fuse_base);

	for(reg_data = (struct gpio_regulator_data *)match->data;
					reg_data->regulator_name; reg_data++)
	{
		ret = cpr3_read_fuse_param(fuse_base, reg_data->volt_fuse_param, &volt_ticks);
		if (ret < 0) {
			dev_err(dev, "%s fuse read failed, ret %d\n", reg_data->regulator_name, ret);
			continue;
		}

		fused_volt = cpr3_convert_open_loop_voltage_fuse(reg_data->reference_volt,
					reg_data->step_volt,
					volt_ticks,
					(reg_data->volt_fuse_param->bit_end -
					reg_data->volt_fuse_param->bit_start + 1));

		ret = cpr3_read_fuse_param(fuse_base, reg_data->cpr_rev, &cpr_fuse);
		if (ret < 0) {
			dev_err(dev, "%s CPR fuse revision read failed, ret %d\n", reg_data->regulator_name, ret);
			continue;
		}

		gpio_regulator = devm_regulator_get(dev, reg_data->regulator_name);
		if (IS_ERR(gpio_regulator)) {
			ret = PTR_ERR(gpio_regulator);
			dev_err(dev, "%s regulator get failed, ret %d\n",reg_data->regulator_name, ret);
			continue;
		}

		if (!cpr_fuse || fix_volt_max)
			volt_select = reg_data->max_volt;
		else
			volt_select = (fused_volt > reg_data->threshold_volt) ?
						reg_data->max_volt : reg_data->min_volt;

		ret = regulator_set_voltage(gpio_regulator, volt_select, volt_select);
		if (ret < 0) {
			dev_err(dev, "%s regulator voltage %u set failed, ret %d",
					reg_data->regulator_name, volt_select, ret);
			continue;
		}
	}

	return ret;
}

static struct platform_driver gpio_regulator_driver = {
	.driver		= {
		.name		= "qcom,gpio-regulator",
		.of_match_table	= gpio_regulator_match_table,
	},
	.probe		= gpio_regulator_probe,
};

module_platform_driver(gpio_regulator_driver);

MODULE_DESCRIPTION("QTI GPIO regulator driver");
MODULE_LICENSE("GPL v2");
