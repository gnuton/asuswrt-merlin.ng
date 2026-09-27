// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 */
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>

#include <linux/mmc/host.h>

#include "pwrseq.h"

struct mmc_pwrseq_ipq {
	struct mmc_pwrseq pwrseq;
	struct gpio_desc *reset_gpio;
};

#define to_pwrseq_ipq(p) container_of(p, struct mmc_pwrseq_ipq, pwrseq)

static void mmc_pwrseq_ipq_pre_power_on(struct mmc_host *host)
{
	struct mmc_pwrseq_ipq *pwrseq = to_pwrseq_ipq(host->pwrseq);

	gpiod_set_value_cansleep(pwrseq->reset_gpio, 1);
	msleep(100);
}

static void mmc_pwrseq_ipq_power_off(struct mmc_host *host)
{
	struct mmc_pwrseq_ipq *pwrseq = to_pwrseq_ipq(host->pwrseq);

	gpiod_set_value_cansleep(pwrseq->reset_gpio, 0);
}

static const struct mmc_pwrseq_ops mmc_pwrseq_ipq_ops = {
	.pre_power_on = mmc_pwrseq_ipq_pre_power_on,
	.power_off = mmc_pwrseq_ipq_power_off,
};

static const struct of_device_id mmc_pwrseq_ipq_of_match[] = {
	{ .compatible = "mmc-pwrseq-ipq",},
	{/* sentinel */},
};
MODULE_DEVICE_TABLE(of, mmc_pwrseq_ipq_of_match);

static int mmc_pwrseq_ipq_probe(struct platform_device *pdev)
{
	struct mmc_pwrseq_ipq *pwrseq;
	struct device *dev = &pdev->dev;

	pwrseq = devm_kzalloc(dev, sizeof(*pwrseq), GFP_KERNEL);
	if (!pwrseq)
		return -ENOMEM;

	pwrseq->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(pwrseq->reset_gpio))
		return PTR_ERR(pwrseq->reset_gpio);

	pwrseq->pwrseq.dev = dev;
	pwrseq->pwrseq.ops = &mmc_pwrseq_ipq_ops;
	pwrseq->pwrseq.owner = THIS_MODULE;
	platform_set_drvdata(pdev, pwrseq);

	return mmc_pwrseq_register(&pwrseq->pwrseq);
}

static int mmc_pwrseq_ipq_remove(struct platform_device *pdev)
{
	struct mmc_pwrseq_ipq *pwrseq = platform_get_drvdata(pdev);

	mmc_pwrseq_unregister(&pwrseq->pwrseq);

	return 0;
}

static struct platform_driver mmc_pwrseq_ipq_driver = {
	.probe = mmc_pwrseq_ipq_probe,
	.remove = mmc_pwrseq_ipq_remove,
	.driver = {
		.name = "pwrseq_ipq",
		.of_match_table = mmc_pwrseq_ipq_of_match,
	},
};

module_platform_driver(mmc_pwrseq_ipq_driver);
MODULE_LICENSE("GPL v2");
