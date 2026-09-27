/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
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

#include <linux/delay.h>
#include <linux/io.h>

/* CPU power domain register offsets */
#define CPU_PWR_CTL                     0x4
#define CPU_PWR_GATE_CTL                0x14

inline void a53ss_unclamp_cpu(void __iomem *reg)
{
	/* Deassert CPU in sleep state */
	writel_relaxed(0x00000033, reg + CPU_PWR_CTL);
	mb();

	/* Program skew between en_few and en_rest to 16 XO clk cycles,
	   close Core logic head switch*/
	writel_relaxed(0x10000001, reg + CPU_PWR_GATE_CTL);
	mb();
	udelay(2);

	/* De-assert coremem clamp */
	writel_relaxed(0x00000031, reg + CPU_PWR_CTL);
	mb();

	/* De-assert Core memory slp_nret_n */
	writel_relaxed(0x00000039, reg + CPU_PWR_CTL);
	mb();
	udelay(2);

	/* De-assert Core memory slp_ret_n */
	writel_relaxed(0x00000239, reg + CPU_PWR_CTL);
	mb();
	udelay(2);

	/* Assert WL_EN_CLK */
	writel_relaxed(0x00004239, reg + CPU_PWR_CTL);
	mb();
	udelay(2);

	/* De-assert WL_EN_CLK */
	writel_relaxed(0x00000239, reg + CPU_PWR_CTL);
	mb();

	/* Deassert Clamp */
	writel_relaxed(0x00000238, reg + CPU_PWR_CTL);
	mb();
	udelay(2);

	/* Deassert Core-n reset */
	writel_relaxed(0x00000208, reg + CPU_PWR_CTL);
	mb();

	/* Assert PWRDUP; */
	writel_relaxed(0x00000288, reg + CPU_PWR_CTL);
	mb();
}
