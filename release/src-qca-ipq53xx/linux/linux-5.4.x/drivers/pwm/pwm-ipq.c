/* Copyright (c) 2016-2017, 2020 The Linux Foundation. All rights reserved.
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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <asm/div64.h>
#include <linux/of_device.h>

#ifdef CONFIG_SND_SOC_IPQ_ADSS
#include "ipq-adss.h"
#else
#define ADSS_GLB_PWM0_CTRL_REG		0x20
#define ADSS_GLB_PWM1_CTRL_REG		0x24
#define ADSS_GLB_PWM2_CTRL_REG		0x28
#define ADSS_GLB_PWM3_CTRL_REG		0x2C
#endif

#define SRC_FREQ		(200*1000*1000)
#define MAX_PWM_DEVICES		4

/* The default period and duty cycle values to be configured. */
#define DEFAULT_PERIOD_NS	10
#define DEFAULT_DUTY_CYCLE_NS	5

/* The frequency range supported is 762Hz to 100MHz. */
#define MIN_PERIOD_NS		10
#define MAX_PERIOD_NS		1312335

/* The max value specified for each field is based on the number of bits
 * in the pwm control regitser for that filed
 */
#define MAX_PRE_DIV		0xFF
#define MAX_PWM_DIV		0x3FF
#define MAX_HI_DUR		0x7FF

/* Enable bit is set to enable output toggling in pwm device.
 * Update bit is set to reflect the changed divider and high duration
 * values in register. Update bit is auto cleared after the update
 */
#define PWM_ENABLE		0x80000000
#define PWM_UPDATE		0x40000000

#define PWM_CTRL_PRE_DIV_SHIFT	22
#define PWM_CTRL_DIV_SHIFT	0
#define PWM_CTRL_HI_SHIFT	10

/* The frequency range supported is 1Hz to 100MHz in V2. */
#define MIN_PERIOD_NS_V2	10
#define MAX_PERIOD_NS_V2	1000000000

/* The max value specified for each field is based on the number of bits
 * in the pwm control regitser for that filed in V2
 */
#define MAX_PWM_CFG_V2		0xFFFF

#define PWM_CTRL_HI_SHIFT_V2	16

#define PWM0_CTRL_REG0	0x0
#define PWM0_CTRL_REG1	0x4
#define PWM1_CTRL_REG0	0x8
#define PWM1_CTRL_REG1	0xc
#define PWM2_CTRL_REG0	0x10
#define PWM2_CTRL_REG1	0x14
#define PWM3_CTRL_REG0	0x18
#define PWM3_CTRL_REG1	0x1C

#define PWM_CFG_REG0 0 /*PWM_DIV PWM_HI*/
#define PWM_CFG_REG1 1 /*ENABLE UPDATE PWM_PRE_DIV*/

enum pwm_version {
	PWM_V1,
	PWM_V2,
};

struct ipq_pwm_ops {
	void (*pwm_writel)(struct pwm_device*, uint32_t, uint32_t);
	uint32_t (*pwm_readl)(struct pwm_device*, uint32_t);
	u32 max_pre_div;
	u32 max_pwm_div;
	u32 max_period_ns;
	u32 min_period_ns;
	enum pwm_version version;
};

struct ipq_pwm_chip {
	struct pwm_chip chip;
	struct clk *clk;
	void __iomem *mem;
	struct ipq_pwm_ops *ops;
};

static ssize_t count;
static uint32_t used_pwm[MAX_PWM_DEVICES];

static const uint32_t pwm_ctrl_register[] = {
	ADSS_GLB_PWM0_CTRL_REG,
	ADSS_GLB_PWM1_CTRL_REG,
	ADSS_GLB_PWM2_CTRL_REG,
	ADSS_GLB_PWM3_CTRL_REG,
};

static const uint32_t pwm_ctrl_register_v2[] = {
	PWM0_CTRL_REG0,
	PWM0_CTRL_REG1,
	PWM1_CTRL_REG0,
	PWM1_CTRL_REG1,
	PWM2_CTRL_REG0,
	PWM2_CTRL_REG1,
	PWM3_CTRL_REG0,
	PWM3_CTRL_REG1,
};

static struct ipq_pwm_chip *to_ipq_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct ipq_pwm_chip, chip);
}

#ifdef CONFIG_SND_SOC_IPQ_ADSS
static void pwm_writel_v1(struct pwm_device *pwm, uint32_t val, uint32_t cfg_reg)
{
	ipq_audio_adss_writel(val, pwm_ctrl_register[pwm->hwpwm]);
}

static uint32_t pwm_readl_v1(struct pwm_device *pwm, uint32_t cfg_reg)
{
	return ipq_audio_adss_readl(pwm_ctrl_register[pwm->hwpwm]);
}
#else
static void pwm_writel_v1(struct pwm_device *pwm, uint32_t val, uint32_t cfg_reg)
{
	struct ipq_pwm_chip *ipq_chip = to_ipq_pwm_chip(pwm->chip);

	writel(val, ipq_chip->mem + pwm_ctrl_register[pwm->hwpwm]);
	return;
}

static uint32_t pwm_readl_v1(struct pwm_device *pwm, uint32_t cfg_reg)
{
	struct ipq_pwm_chip *ipq_chip = to_ipq_pwm_chip(pwm->chip);

	return readl(ipq_chip->mem + pwm_ctrl_register[pwm->hwpwm]);
}
#endif

static void pwm_writel_v2(struct pwm_device *pwm, uint32_t val, uint32_t cfg_reg)
{
	struct ipq_pwm_chip *ipq_chip = to_ipq_pwm_chip(pwm->chip);
	uint32_t offset;

	offset = pwm_ctrl_register_v2[(pwm->hwpwm * 2) + cfg_reg];
	writel(val, ipq_chip->mem + offset);
}

static uint32_t pwm_readl_v2(struct pwm_device *pwm, uint32_t cfg_reg)
{
	struct ipq_pwm_chip *ipq_chip = to_ipq_pwm_chip(pwm->chip);
	uint32_t offset;

	offset = pwm_ctrl_register_v2[(pwm->hwpwm * 2) + cfg_reg];
	return readl(ipq_chip->mem + offset);
}

static void config_div_and_duty(struct pwm_device *pwm, int pre_div,
			unsigned long long pwm_div, unsigned long period_ns,
			unsigned long long duty_ns)
{
	unsigned long hi_dur;
	unsigned long long quotient;
	unsigned long ctrl_reg_val = 0;
	struct ipq_pwm_chip *ipq_chip = to_ipq_pwm_chip(pwm->chip);

	/* high duration = pwm duty * ( pwm div + 1)
	 * pwm duty = duty_ns / period_ns
	 */
	quotient = (pwm_div + 1) * duty_ns;
	do_div(quotient, period_ns);
	hi_dur = quotient;

	if (ipq_chip->ops->version == PWM_V1) {
		ctrl_reg_val |= ((pre_div & ipq_chip->ops->max_pre_div)
						<< PWM_CTRL_PRE_DIV_SHIFT);
		ctrl_reg_val |= ((hi_dur & MAX_HI_DUR) << PWM_CTRL_HI_SHIFT);
		ctrl_reg_val |= ((pwm_div & ipq_chip->ops->max_pwm_div)
						<< PWM_CTRL_DIV_SHIFT);
	} else {
		ctrl_reg_val |= ((hi_dur & MAX_PWM_CFG_V2)
						<< PWM_CTRL_HI_SHIFT_V2);
		ctrl_reg_val |= (pwm_div & ipq_chip->ops->max_pwm_div);
		ipq_chip->ops->pwm_writel(pwm, ctrl_reg_val, PWM_CFG_REG0);
		ctrl_reg_val = pre_div & ipq_chip->ops->max_pre_div;
	}
	ipq_chip->ops->pwm_writel(pwm, ctrl_reg_val, PWM_CFG_REG1);
}

static int ipq_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ipq_pwm_chip *ipq_chip = to_ipq_pwm_chip(pwm->chip);
	unsigned long ctrl_reg_val;

	ctrl_reg_val = ipq_chip->ops->pwm_readl(pwm, PWM_CFG_REG1);
	ctrl_reg_val |= (PWM_ENABLE | PWM_UPDATE);
	ipq_chip->ops->pwm_writel(pwm, ctrl_reg_val, PWM_CFG_REG1);

	return 0;
}

static void ipq_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ipq_pwm_chip *ipq_chip = to_ipq_pwm_chip(pwm->chip);
	unsigned long ctrl_reg_val;

	ctrl_reg_val = ipq_chip->ops->pwm_readl(pwm, PWM_CFG_REG1);
	ctrl_reg_val &= ~PWM_ENABLE;
	ipq_chip->ops->pwm_writel(pwm, ctrl_reg_val, PWM_CFG_REG1);
}

static int ipq_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
						int duty_ns, int period_ns)
{
	struct ipq_pwm_chip *ipq_chip = to_ipq_pwm_chip(chip);
	unsigned long freq;
	int pre_div, close_pre_div, close_pwm_div;
	int pwm_div;
	long long diff;
	unsigned long rate = clk_get_rate(ipq_chip->clk);
	unsigned long min_diff = rate;
	uint64_t fin_ps;

	if (period_ns > ipq_chip->ops->max_period_ns ||
			period_ns < ipq_chip->ops->min_period_ns) {
		pr_err("PWM Frequency range supported is %dHz to %dHz\n"
			"Switching to default configuration values\n",
		       (int)NSEC_PER_SEC / ipq_chip->ops->max_period_ns,
		       (int)NSEC_PER_SEC / ipq_chip->ops->min_period_ns);
		period_ns = DEFAULT_PERIOD_NS;
		duty_ns = DEFAULT_DUTY_CYCLE_NS;
		pwm->state.period = period_ns;
		pwm->state.duty_cycle = duty_ns;
	}

	/* freq in Hz for period in nano second*/
	freq = NSEC_PER_SEC / period_ns;
	fin_ps = (uint64_t)NSEC_PER_SEC * 1000;
	do_div(fin_ps, rate);
	close_pre_div = ipq_chip->ops->max_pre_div;
	close_pwm_div = ipq_chip->ops->max_pwm_div;

	for (pre_div = 0; pre_div <= ipq_chip->ops->max_pre_div ; pre_div++) {
		pwm_div = DIV_ROUND_CLOSEST_ULL((uint64_t)period_ns * 1000,
							fin_ps * (pre_div + 1));
		pwm_div--;
		if (pwm_div <= ipq_chip->ops->max_pwm_div) {
			diff = (uint64_t)rate - ((uint64_t)freq * (pre_div + 1) * (pwm_div + 1));

			if (diff < 0)
				diff = -diff;
			if (!diff) {
				close_pre_div = pre_div;
				close_pwm_div = pwm_div;
				break;
			}
			if (diff < min_diff) {
				min_diff = diff;
				close_pre_div = pre_div;
				close_pwm_div = pwm_div;
			}
		}
	}

	ipq_pwm_disable(chip, pwm);

	/* config divider values for the closest possible frequency */
	config_div_and_duty(pwm, close_pre_div, close_pwm_div,
					period_ns, duty_ns);
	ipq_pwm_enable(chip, pwm);

	return 0;
}

static int ipq_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	if (!used_pwm[pwm->hwpwm])
		return -EINVAL;

	pwm->state.period = DEFAULT_PERIOD_NS;
	pwm->state.duty_cycle = DEFAULT_DUTY_CYCLE_NS;

	ipq_pwm_config(chip, pwm, pwm->state.duty_cycle, pwm->state.period);

	return 0;
}

static void ipq_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
	ipq_pwm_disable(chip, pwm);
}


static struct pwm_ops ipq_pwm_ops = {
	.request = ipq_pwm_request,
	.free = ipq_pwm_free,
	.config = ipq_pwm_config,
	.enable = ipq_pwm_enable,
	.disable = ipq_pwm_disable,
	.owner = THIS_MODULE,
};

static int ipq_pwm_probe(struct platform_device *pdev)
{
	struct ipq_pwm_chip *pwm;
	struct device *dev;
	unsigned int base_index;
	int ret;
	const void *dev_data;
	unsigned long src_freq = SRC_FREQ;
	struct resource *res = NULL;

	dev = &pdev->dev;
	pwm = devm_kzalloc(dev, sizeof(*pwm), GFP_KERNEL);
	if (!pwm) {
		dev_err(dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, pwm);
	dev_data = of_device_get_match_data(dev);
	if (!dev_data) {
		dev_err(&pdev->dev, "failed to get device data\n");
		return -ENODEV;
	}

	pwm->ops = (struct ipq_pwm_ops*) dev_data;
	of_property_read_u64(dev->of_node, "src-freq", (u64 *)src_freq);

	pwm->clk = devm_clk_get(dev, "core");
	if (!IS_ERR(pwm->clk)) {
		ret = clk_set_rate(pwm->clk, src_freq);
		if (ret)
			return ret;
		ret = clk_prepare_enable(pwm->clk);
		if (ret)
			return ret;
	}

	if (pwm->ops->version == PWM_V2 || !IS_ENABLED(CONFIG_SND_SOC_IPQ_ADSS)) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		pwm->mem = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(pwm->mem))
			return PTR_ERR(pwm->mem);
	}

	if (of_property_read_u32(dev->of_node, "pwm-base-index", &base_index))
		base_index = 0;

	count = of_property_count_u32_elems(dev->of_node, "used-pwm-indices");

	if (of_property_read_u32_array(dev->of_node, "used-pwm-indices",
			used_pwm, count))
		return -EINVAL;

	pwm->chip.dev = dev;
	pwm->chip.ops = &ipq_pwm_ops;
	pwm->chip.base = base_index;
	pwm->chip.npwm = count;

	ret = pwmchip_add(&pwm->chip);
	if (ret < 0) {
		dev_err(dev, "pwmchip_add() failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static int ipq_pwm_remove(struct platform_device *pdev)
{
	struct ipq_pwm_chip *pwm = platform_get_drvdata(pdev);

	return pwmchip_remove(&pwm->chip);
}

static const struct ipq_pwm_ops ops_v1 = {
	.pwm_writel = pwm_writel_v1,
	.pwm_readl = pwm_readl_v1,
	.max_pre_div = MAX_PRE_DIV,
	.max_pwm_div = MAX_PWM_DIV,
	.max_period_ns = MAX_PERIOD_NS,
	.min_period_ns = MIN_PERIOD_NS,
	.version = PWM_V1
};

static const struct ipq_pwm_ops ops_v2 = {
	.pwm_writel = pwm_writel_v2,
	.pwm_readl = pwm_readl_v2,
	.max_pre_div = MAX_PWM_CFG_V2,
	.max_pwm_div = MAX_PWM_CFG_V2,
	.max_period_ns = MAX_PERIOD_NS_V2,
	.min_period_ns = MIN_PERIOD_NS_V2,
	.version = PWM_V2
};

static const struct of_device_id pwm_msm_dt_match[] = {
	{ .compatible = "qti,ipq-pwm", .data = &ops_v1 },
	{ .compatible = "qti,ipq5018-pwm", .data = &ops_v2 },
	{ .compatible = "qti,ipq5332-pwm", .data = &ops_v2 },
	{ .compatible = "qti,ipq6018-pwm", .data = &ops_v2 },
	{ .compatible = "qti,ipq9574-pwm", .data = &ops_v2 },
	{}
};
MODULE_DEVICE_TABLE(of, pwm_msm_dt_match);

static struct platform_driver ipq_pwm_driver = {
	.driver = {
		.name = "qti,ipq-pwm",
		.owner = THIS_MODULE,
		.of_match_table = pwm_msm_dt_match,
	},
	.probe = ipq_pwm_probe,
	.remove = ipq_pwm_remove,
};

module_platform_driver(ipq_pwm_driver);

MODULE_LICENSE("Dual BSD/GPL");
