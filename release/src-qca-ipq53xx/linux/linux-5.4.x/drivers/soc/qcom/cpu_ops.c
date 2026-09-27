/*
 * Copyright (c) 2016, 2020, The Linux Foundation. All rights reserved.
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
 */

/* IPQ ARMv8 CPU Operations
 * Based on arch/arm64/kernel/smp_spin_table.c
 */

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/smp.h>
#include <linux/types.h>

#include <asm/cacheflush.h>
#include <asm/cpu_ops.h>
#include <asm/cputype.h>
#include <asm/smp_plat.h>
#include <linux/qcom_scm.h>


#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
extern void a53ss_unclamp_cpu(void __iomem *reg);

static DEFINE_SPINLOCK(boot_lock);
static DEFINE_PER_CPU(int, cold_boot_done);

static int a53ss_release_secondary(unsigned int cpu)
{
	int ret = 0;
	struct device_node *cpu_node, *acc_node;
	void __iomem *reg;

	cpu_node = of_get_cpu_node(cpu, NULL);
	if (!cpu_node)
		return -ENODEV;

	acc_node = of_parse_phandle(cpu_node, "qcom,acc", 0);
	if (!acc_node) {
		ret = -ENODEV;
		goto err_cpu_node;
	}
	reg = of_iomap(acc_node, 0);
	if (!reg) {
		ret = -ENOMEM;
		goto err_acc_node;
	}

	a53ss_unclamp_cpu(reg);

	per_cpu(cold_boot_done, cpu) = true;

	/* Secondary CPU-N is now alive */
	iounmap(reg);
err_acc_node:
	of_node_put(acc_node);
err_cpu_node:
	of_node_put(cpu_node);

	return ret;

}

static void qti_wake_cpu(unsigned int cpu)
{
	spin_lock(&boot_lock);

	arch_send_wakeup_ipi_mask(cpumask_of(cpu));

	spin_unlock(&boot_lock);
}

static int qti_boot_cpu(unsigned int cpu, int (*func)(unsigned int))
{
	int ret = 0;

	if (!per_cpu(cold_boot_done, cpu)) {
		ret = func(cpu);
	}

	qti_wake_cpu(cpu);

	return ret;
}

static int __init qti_cpu_init(unsigned int cpu)
{
	return 0;
}

static int __init qti_cpu_prepare(unsigned int cpu)
{
	return 0;
}

static int a53ss_cpu_boot(unsigned int cpu)
{
	return qti_boot_cpu(cpu, a53ss_release_secondary);
}

void qti_cpu_postboot(void)
{

}

#ifdef CONFIG_HOTPLUG_CPU
static void a53ss_wfi_cpu_die(unsigned int cpu)
{

}
#endif

struct cpu_operations smp_a53ss_ops = {
	.name           = "qcom,arm-cortex-acc",
	.cpu_init       = qti_cpu_init,
	.cpu_prepare    = qti_cpu_prepare,
	.cpu_boot       = a53ss_cpu_boot,
	.cpu_postboot   = qti_cpu_postboot,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die        = a53ss_wfi_cpu_die,
#endif
};

CPU_METHOD_OF_DECLARE(qcom_smp_a53ss, "qcom,arm-cortex-acc", &smp_a53ss_ops);

