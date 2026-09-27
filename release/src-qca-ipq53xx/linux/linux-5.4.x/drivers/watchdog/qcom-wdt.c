// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
 */
#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/of_device.h>
#include <linux/sched/clock.h>
#include <linux/qcom_scm.h>
#include <linux/mhi.h>

#define TCSR_WONCE_REG 0x193d010

bool mhi_wdt_panic_enable;
enum wdt_reg {
	WDT_RST,
	WDT_EN,
	WDT_STS,
	WDT_BARK_TIME,
	WDT_BITE_TIME,
};

#define QCOM_WDT_ENABLE		BIT(0)

static const u32 reg_offset_data_apcs_tmr[] = {
	[WDT_RST] = 0x38,
	[WDT_EN] = 0x40,
	[WDT_STS] = 0x44,
	[WDT_BARK_TIME] = 0x4C,
	[WDT_BITE_TIME] = 0x5C,
};

static const u32 reg_offset_data_kpss[] = {
	[WDT_RST] = 0x4,
	[WDT_EN] = 0x8,
	[WDT_STS] = 0xC,
	[WDT_BARK_TIME] = 0x10,
	[WDT_BITE_TIME] = 0x14,
};

struct qti_wdt_props {
	const u32 *layout;
	u32 max_tick_count;
};

const struct qti_wdt_props qti_wdt_props_apcs_tmr = {
	.layout = reg_offset_data_apcs_tmr,
	.max_tick_count = 0x10000000U,
};

const struct qti_wdt_props qti_wdt_props_kpss = {
	.layout = reg_offset_data_kpss,
	.max_tick_count = 0xFFFFFU,
};

struct qcom_wdt {
	struct watchdog_device	wdd;
	unsigned long		rate;
	void __iomem		*base;
	const u32		*layout;
};

static void __iomem *wdt_addr(struct qcom_wdt *wdt, enum wdt_reg reg)
{
	return wdt->base + wdt->layout[reg];
}

static inline
struct qcom_wdt *to_qcom_wdt(struct watchdog_device *wdd)
{
	return container_of(wdd, struct qcom_wdt, wdd);
}

static inline int qcom_get_enable(struct watchdog_device *wdd)
{
	return QCOM_WDT_ENABLE;
}

static void qcom_wdt_bite(struct qcom_wdt *wdt, unsigned int ticks)
{
	unsigned long nanosec_rem;
	unsigned long long t = sched_clock();

#ifdef CONFIG_DUMP_PREV_OOPS_MSG
	enable_oopsbuf(1);
#endif

	nanosec_rem = do_div(t, 1000000000);
	pr_info("Watchdog bark! Now = %lu.%06lu\n", (unsigned long) t,
							nanosec_rem / 1000);

#ifdef CONFIG_MHI_BUS
	if (mhi_wdt_panic_enable)
		mhi_wdt_panic_handler();
#endif

	pr_info("Causing a watchdog bite!");
	writel(0, wdt_addr(wdt, WDT_EN));
	writel(1, wdt_addr(wdt, WDT_RST));
	mb(); /* Avoid unpredictable behaviour in concurrent executions */
	pr_info("Configuring Watchdog Timer\n");
	writel(ticks, wdt_addr(wdt, WDT_BARK_TIME));
	writel(ticks, wdt_addr(wdt, WDT_BITE_TIME));
	writel(QCOM_WDT_ENABLE, wdt_addr(wdt, WDT_EN));
	mb(); /* Make sure the above sequence hits hardware before Reboot. */
	pr_info("Waiting for Reboot\n");

	/*
	 * Actually make sure the above sequence hits hardware before sleeping.
	 */
	wmb();

	mdelay(150);
}

static irqreturn_t qcom_wdt_isr(int irq, void *arg)
{
	struct watchdog_device *wdd = arg;

	watchdog_notify_pretimeout(wdd);
	qcom_wdt_bite(to_qcom_wdt(wdd), 1);

	return IRQ_HANDLED;
}

static int qcom_wdt_start(struct watchdog_device *wdd)
{
	struct qcom_wdt *wdt = to_qcom_wdt(wdd);
	unsigned int bark = wdd->timeout - wdd->pretimeout;

	writel(0, wdt_addr(wdt, WDT_EN));
	writel(1, wdt_addr(wdt, WDT_RST));

	/*
	 * mhi-wdt-panic-enable if set, BARK and BITE time should have
	 * enough difference for the MDM to collect crash dump and
	 * reboot i.e BITE time should be set twice as BARK time.
	 *
	 * Openwrt sets the BITE and BARK time to be 30 and 29. In order
	 * to have time for MDM dump collection, the following command must
	 * be used to set BARK and BITE time to 15 and 30.
	 *
	 *	ubus call system watchdog '{ "timeout":15 }'
	 *
	 * The max timeout value that can be configured is only 32 and so
	 * the above command must be run to give sufficient time difference
	 * between bark and bite. Failing to run above command will lead to
	 * timeout round off as a result of which bite time will be less than
	 * bark time.
	 *
	 */
	if (!mhi_wdt_panic_enable) {
		writel(bark * wdt->rate, wdt_addr(wdt, WDT_BARK_TIME));
		writel(wdd->timeout * wdt->rate, wdt_addr(wdt, WDT_BITE_TIME));
	} else {
		writel(wdd->timeout * wdt->rate, wdt_addr(wdt, WDT_BARK_TIME));
		writel((wdd->timeout * wdt->rate) * 2, wdt_addr(wdt, WDT_BITE_TIME));
	}
	udelay(100);

	writel(qcom_get_enable(wdd), wdt_addr(wdt, WDT_EN));
	return 0;
}

static int qcom_wdt_stop(struct watchdog_device *wdd)
{
	struct qcom_wdt *wdt = to_qcom_wdt(wdd);

	writel(0, wdt_addr(wdt, WDT_EN));
	return 0;
}

static int qcom_wdt_ping(struct watchdog_device *wdd)
{
	struct qcom_wdt *wdt = to_qcom_wdt(wdd);

	writel(1, wdt_addr(wdt, WDT_RST));
	return 0;
}

static int qcom_wdt_set_timeout(struct watchdog_device *wdd,
				unsigned int timeout)
{
	wdd->timeout = timeout;
	return qcom_wdt_start(wdd);
}

static int qcom_wdt_set_pretimeout(struct watchdog_device *wdd,
				   unsigned int timeout)
{
	wdd->pretimeout = timeout;
	return qcom_wdt_start(wdd);
}

static int qcom_wdt_restart(struct watchdog_device *wdd, unsigned long action,
			    void *data)
{
	struct qcom_wdt *wdt = to_qcom_wdt(wdd);
	u32 timeout;

	/*
	 * Trigger watchdog bite:
	 *    Setup BITE_TIME to be 128ms, and enable WDT.
	 */
	timeout = 128 * wdt->rate / 1000;
	qcom_wdt_bite(wdt, timeout);

	return 0;
}

static const struct watchdog_ops qcom_wdt_ops = {
	.start		= qcom_wdt_start,
	.stop		= qcom_wdt_stop,
	.ping		= qcom_wdt_ping,
	.set_timeout	= qcom_wdt_set_timeout,
	.set_pretimeout	= qcom_wdt_set_pretimeout,
	.restart        = qcom_wdt_restart,
	.owner		= THIS_MODULE,
};

static const struct watchdog_info qcom_wdt_info = {
	.options	= WDIOF_KEEPALIVEPING
			| WDIOF_MAGICCLOSE
			| WDIOF_SETTIMEOUT
			| WDIOF_CARDRESET,
	.identity	= KBUILD_MODNAME,
};

static const struct watchdog_info qcom_wdt_pt_info = {
	.options	= WDIOF_KEEPALIVEPING
			| WDIOF_MAGICCLOSE
			| WDIOF_SETTIMEOUT
			| WDIOF_PRETIMEOUT
			| WDIOF_CARDRESET,
	.identity	= KBUILD_MODNAME,
};

static void qcom_clk_disable_unprepare(void *data)
{
	clk_disable_unprepare(data);
}

static int qti_fiq_extwdt(unsigned int regaddr, unsigned int value)
{
	int ret;

	ret = qti_scm_extwdt(SCM_SVC_EXTWDT, SCM_CMD_EXTWDT, regaddr, value);
	if (ret)
		pr_err("Setting value to TCSR_WONCE register failed\n");

	return ret;
}

static int qcom_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct qcom_wdt *wdt;
	struct resource *res;
	struct device_node *np = dev->of_node;
	const u32 *regs;
	u32 percpu_offset;
	int irq, ret;
	struct clk *clk;
	unsigned int retn, extwdt_val = 0, regaddr;
	u32 val;
	const struct qti_wdt_props *wdt_dev_props;

	wdt_dev_props = of_device_get_match_data(dev);
	if (!wdt_dev_props) {
		dev_err(dev, "Unsupported QCOM WDT module\n");
		return -ENODEV;
	}

	regs = wdt_dev_props->layout;

	wdt = devm_kzalloc(dev, sizeof(*wdt), GFP_KERNEL);
	if (!wdt)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENOMEM;

	/* We use CPU0's DGT for the watchdog */
	if (of_property_read_u32(np, "cpu-offset", &percpu_offset))
		percpu_offset = 0;

	res->start += percpu_offset;
	res->end += percpu_offset;

	mhi_wdt_panic_enable = of_property_read_bool(np, "mhi-wdt-panic-enable");

	wdt->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(wdt->base))
		return PTR_ERR(wdt->base);

	clk = devm_clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		dev_err(dev, "failed to get input clock\n");
		return PTR_ERR(clk);
	}

	ret = clk_prepare_enable(clk);
	if (ret) {
		dev_err(dev, "failed to setup clock\n");
		return ret;
	}
	ret = devm_add_action_or_reset(dev, qcom_clk_disable_unprepare, clk);
	if (ret)
		return ret;

	/*
	 * We use the clock rate to calculate the max timeout, so ensure it's
	 * not zero to avoid a divide-by-zero exception.
	 *
	 * WATCHDOG_CORE assumes units of seconds, if the WDT is clocked such
	 * that it would bite before a second elapses it's usefulness is
	 * limited.  Bail if this is the case.
	 */
	wdt->rate = clk_get_rate(clk);
	if (wdt->rate == 0 ||
	    wdt->rate > wdt_dev_props->max_tick_count) {
		dev_err(dev, "invalid clock rate\n");
		return -EINVAL;
	}

	/* check if there is pretimeout support */
	irq = platform_get_irq_optional(pdev, 0);
	if (irq > 0) {
		ret = devm_request_irq(dev, irq, qcom_wdt_isr,
				       IRQF_TRIGGER_RISING,
				       "wdt_bark", &wdt->wdd);
		if (ret)
			return ret;

		wdt->wdd.info = &qcom_wdt_pt_info;
		wdt->wdd.pretimeout = 1;
	} else {
		if (irq == -EPROBE_DEFER)
			return -EPROBE_DEFER;

		wdt->wdd.info = &qcom_wdt_info;
	}

	wdt->wdd.ops = &qcom_wdt_ops;
	wdt->wdd.min_timeout = 1;
	wdt->wdd.max_timeout = wdt_dev_props->max_tick_count / wdt->rate;

	wdt->wdd.parent = dev;
	wdt->layout = regs;

	if (readl(wdt_addr(wdt, WDT_STS)) & 1)
		wdt->wdd.bootstatus = WDIOF_CARDRESET;

	/*
	 * Default to 32 seconds timeout, unless the max timeout
	 * is less than 32 seconds, then use the max instead.
	 */
	wdt->wdd.timeout = min(wdt->wdd.max_timeout, 32U);
	watchdog_init_timeout(&wdt->wdd, 0, dev);

	ret = devm_watchdog_register_device(dev, &wdt->wdd);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, wdt);

	if (!of_property_read_u32(np, "extwdt-val", &val)) {
		extwdt_val = val;

		regaddr = TCSR_WONCE_REG;
		retn = qti_fiq_extwdt(regaddr, extwdt_val);
		if (retn)
			dev_err(&pdev->dev, "FIQ scm_call failed\n");
	}

	return 0;
}

static int __maybe_unused qcom_wdt_suspend(struct device *dev)
{
	struct qcom_wdt *wdt = dev_get_drvdata(dev);

	if (watchdog_active(&wdt->wdd))
		qcom_wdt_stop(&wdt->wdd);

	return 0;
}

static int __maybe_unused qcom_wdt_resume(struct device *dev)
{
	struct qcom_wdt *wdt = dev_get_drvdata(dev);

	if (watchdog_active(&wdt->wdd))
		qcom_wdt_start(&wdt->wdd);

	return 0;
}

static SIMPLE_DEV_PM_OPS(qcom_wdt_pm_ops, qcom_wdt_suspend, qcom_wdt_resume);

static const struct of_device_id qcom_wdt_of_table[] = {
	{ .compatible = "qcom,kpss-timer",
	  .data = (void *) &qti_wdt_props_apcs_tmr },
	{ .compatible = "qcom,scss-timer",
	  .data = (void *) &qti_wdt_props_apcs_tmr },
	{ .compatible = "qcom,kpss-wdt",
	  .data = (void *) &qti_wdt_props_kpss },
	{ },
};
MODULE_DEVICE_TABLE(of, qcom_wdt_of_table);

static struct platform_driver qcom_watchdog_driver = {
	.probe	= qcom_wdt_probe,
	.driver	= {
		.name		= KBUILD_MODNAME,
		.of_match_table	= qcom_wdt_of_table,
		.pm		= &qcom_wdt_pm_ops,
	},
};
module_platform_driver(qcom_watchdog_driver);

MODULE_DESCRIPTION("QCOM KPSS Watchdog Driver");
MODULE_LICENSE("GPL v2");
