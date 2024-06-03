/****************************************************************************
 *
 * Broadcom Proprietary and Confidential.
 * (c) 2018 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to
 * you under the terms of the GNU General Public License version 2 (the
 * "GPL"), available at [http://www.broadcom.com/licenses/GPLv2.php], with
 * the following added to such license:
 *
 * As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy
 * and distribute the resulting executable under terms of your choice,
 * provided that you also meet, for each linked independent module, the
 * terms and conditions of the license of that module. An independent
 * module is a module which is not derived from this software. The special
 * exception does not apply to any modifications of the software.
 *
 * Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a
 * license other than the GPL, without Broadcom's express prior written
 * consent.
 *
 * Author: Tim Ross <tross@broadcom.com>
 *****************************************************************************/
#ifndef _FPM_DEV_H_
#define _FPM_DEV_H_

#include <linux/types.h>
#include <linux/skbuff.h>
#include "fpm.h"

extern struct fpmdev *fpm;
extern spinlock_t fpm_reg_lock;

static inline u32 fpm_reg_read(u32 *reg)
{
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&fpm_reg_lock, flags);
	val = __raw_readl(reg);
	pr_debug("read @ 0x%p 0x%08x\n", reg, val);
	spin_unlock_irqrestore(&fpm_reg_lock, flags);
	return val;
}

static inline void fpm_reg_write(u32 *reg, u32 val)
{
	unsigned long flags;

	spin_lock_irqsave(&fpm_reg_lock, flags);
	__raw_writel(val, reg);
	pr_debug("wrote @ 0x%p 0x%08x\n", reg, val);
	spin_unlock_irqrestore(&fpm_reg_lock, flags);
}

static inline void fpm_reg_write_mask(u32 *reg, u32 mask, u32 val)
{
	unsigned long flags;
	u32 v;

	spin_lock_irqsave(&fpm_reg_lock, flags);
	v = __raw_readl(reg);
	pr_debug("read @ 0x%p 0x%08x\n", reg, v);
	v &= ~mask;
	v |= (val & mask);
	__raw_writel(v, reg);
	pr_debug("wrote @ 0x%p 0x%08x\n", reg, v);
	spin_unlock_irqrestore(&fpm_reg_lock, flags);
}

#define FPM_CTRL_CTL				0x00000
#define FPM_CTRL_CTL_INIT_MEM_MASK(pool)	(0x00000010 >> (pool))
#define FPM_CTRL_CTL_INIT_MEM_SHIFT(pool)	(4 - (pool))
#define FPM_CTRL_CTL_POOL_ENABLE_MASK(pool)	(0x00010000 << (pool))
#define FPM_CTRL_CTL_POOL_ENABLE_SHIFT(pool)	(18 + (pool))
#define FPM_CTRL_CTL_FPM_BB_SOFT_RESET_MASK	0x00004000
#define FPM_CTRL_CTL_FPM_BB_SOFT_RESET_SHIFT	14

#define FPM_CTRL_WEIGHT				0x00008
#define FPM_CTRL_WEIGHT_DDR0_ALLOC_WEIGHT_MASK	0x000000ff
#define FPM_CTRL_WEIGHT_DDR0_ALLOC_WEIGHT_SHIFT	0
#define FPM_CTRL_WEIGHT_DDR1_ALLOC_WEIGHT_MASK	0x00ff0000
#define FPM_CTRL_WEIGHT_DDR1_ALLOC_WEIGHT_SHIFT	16
#define FPM_CTRL_WEIGHT_DDR0_FREE_WEIGHT_MASK	0x0000ff00
#define FPM_CTRL_WEIGHT_DDR0_FREE_WEIGHT_SHIFT	8
#define FPM_CTRL_WEIGHT_DDR1_FREE_WEIGHT_MASK	0xff000000
#define FPM_CTRL_WEIGHT_DDR1_FREE_WEIGHT_SHIFT	24

#define FPM_CTRL_POOL_INTR_MSK(pool)			(0x00010 + (pool) * 0x0c)
#define FPM_EXPIRED_TOKEN_RECOV_MASK			0x00004000
#define FPM_EXPIRED_TOKEN_RECOV_SHIFT			14
#define FPM_EXPIRED_TOKEN_DET_MASK			0x00002000
#define FPM_EXPIRED_TOKEN_DET_SHIFT			13
#define FPM_ILLEGAL_ALLOC_REQUEST_MASK			0x00001000
#define FPM_ILLEGAL_ALLOC_REQUEST_SHIFT			12
#define FPM_ILLEGAL_ADDRESS_ACCESS_MASK			0x00000800
#define FPM_ILLEGAL_ADDRESS_ACCESS_SHIFT		11
#define FPM_XON_MASK					0x00000400
#define FPM_XON_SHIFT					10
#define FPM_XOFF_MASK					0x00000200
#define FPM_XOFF_SHIFT					9
#define FPM_MEMORY_CORRUPT_MASK				0x00000100
#define FPM_MEMORY_CORRUPT_SHIFT			8
#define FPM_POOL_DIS_FREE_MULTI_MASK			0x00000080
#define FPM_POOL_DIS_FREE_MULTI_SHIFT			7
#define FPM_MULTI_TOKEN_INDEX_OUT_OF_RANGE_MASK		0x00000040
#define FPM_MULTI_TOKEN_INDEX_OUT_OF_RANGE_SHIFT	6
#define FPM_MULTI_TOKEN_NO_VALID_MASK			0x00000020
#define FPM_MULTI_TOKEN_NO_VALID_SHIFT			5
#define FPM_FREE_TOKEN_INDEX_OUT_OF_RANGE_MASK		0x00000010
#define FPM_FREE_TOKEN_INDEX_OUT_OF_RANGE_SHIFT		4
#define FPM_FREE_TOKEN_NO_VALID_MASK			0x00000008
#define FPM_FREE_TOKEN_NO_VALID_SHIFT			3
#define FPM_POOL_FULL_MASK				0x00000004
#define FPM_POOL_FULL_SHIFT				2
#define FPM_FREE_FIFO_FULL_MASK				0x00000002
#define FPM_FREE_FIFO_FULL_SHIFT			1
#define FPM_ALLOC_FIFO_FULL_MASK			0x00000001
#define FPM_ALLOC_FIFO_FULL_SHIFT			0

#define FPM_CTRL_POOL_INTR_STS(pool)	(0x00014 + (pool)*0x0c)

#define FPM_CTRL_POOL_CFG_FP_BUF_SIZE				0x00040
#define FPM_CTRL_POOL_CFG_FP_BUF_SIZE_MASK			0x07000000
#define FPM_CTRL_POOL_CFG_FP_BUF_SIZE_SHIFT			24

#define FPM_CTRL_POOL_CFG_BASE_ADDRESS(pool)			(0x00044 + (pool) * 0x4)
#define FPM_CTRL_POOL_CFG_BASE_ADDRESS_MASK			0xfffffffc
#define FPM_CTRL_POOL_CFG_BASE_ADDRESS_SHIFT			2

#define FPM_CTRL_POOL_STAT1(pool)				(0x00050 + (pool) * 0x20)
#define FPM_CTRL_POOL_STAT1_UNDRFL_MASK				0x0000ffff
#define FPM_CTRL_POOL_STAT1_UNDRFL_SHIFT			0
#define FPM_CTRL_POOL_STAT1_OVRFL_MASK				0xffff0000
#define FPM_CTRL_POOL_STAT1_OVRFL_SHIFT				16

#define FPM_CTRL_POOL_STAT2(pool)				(0x00054 + (pool) * 0x20)
#define FPM_CTRL_POOL_STAT2_NUM_OF_TOKENS_AVAILABLE_MASK	0x003fffff
#define FPM_CTRL_POOL_STAT2_NUM_OF_TOKENS_AVAILABLE_SHIFT	0
#define FPM_CTRL_POOL_STAT2_ALLOC_FIFO_EMPTY_MASK		0x04000000
#define FPM_CTRL_POOL_STAT2_ALLOC_FIFO_EMPTY_SHIFT		26
#define FPM_CTRL_POOL_STAT2_ALLOC_FIFO_FULL_MASK		0x08000000
#define FPM_CTRL_POOL_STAT2_ALLOC_FIFO_FULL_SHIFT		27
#define FPM_CTRL_POOL_STAT2_FREE_FIFO_EMPTY_MASK		0x10000000
#define FPM_CTRL_POOL_STAT2_FREE_FIFO_EMPTY_SHIFT		28
#define FPM_CTRL_POOL_STAT2_FREE_FIFO_FULL_MASK			0x20000000
#define FPM_CTRL_POOL_STAT2_FREE_FIFO_FULL_SHIFT		29
#define FPM_CTRL_POOL_STAT2_POOL_FULL_MASK			0x80000000
#define FPM_CTRL_POOL_STAT2_POOL_FULL_SHIFT			31

#define FPM_CTRL_POOL_STAT3(pool)				(0x00058 + (pool) * 0x20)
#define FPM_CTRL_POOL_STAT3_NUM_OF_NOT_VALID_TOKEN_FREES_MASK	0x003fffff
#define FPM_CTRL_POOL_STAT3_NUM_OF_NOT_VALID_TOKEN_FREES_SHIFT	0

#define FPM_CTRL_POOL_STAT4(pool)				(0x0005c + (pool) * 0x20)
#define FPM_CTRL_POOL_STAT4_NUM_OF_NOT_VALID_TOKEN_MULTI_MASK	0x003fffff
#define FPM_CTRL_POOL_STAT4_NUM_OF_NOT_VALID_TOKEN_MULTI_SHIFT	0

#define FPM_CTRL_POOL_STAT5(pool)						(0x00060 + (pool) * 0x20)
#define FPM_CTRL_POOL_STAT5_MEM_CORRUPT_STS_RELATED_ALLOC_TOKEN_VALID_MASK	0x80000000
#define FPM_CTRL_POOL_STAT5_MEM_CORRUPT_STS_RELATED_ALLOC_TOKEN_VALID_SHIFT	31
#define FPM_CTRL_POOL_STAT5_MEM_CORRUPT_STS_RELATED_ALLOC_TOKEN_MASK		0x7fffffff
#define FPM_CTRL_POOL_STAT5_MEM_CORRUPT_STS_RELATED_ALLOC_TOKEN_SHIFT		0

#define FPM_CTRL_POOL_STAT6(pool)				(0x00064 + (pool) * 0x20)
#define FPM_CTRL_POOL_STAT6_INVALID_FREE_TOKEN_MASK		0x7fffffff
#define FPM_CTRL_POOL_STAT6_INVALID_FREE_TOKEN_SHIFT		0
#define FPM_CTRL_POOL_STAT6_INVALID_FREE_TOKEN_VALID_MASK	0x80000000
#define FPM_CTRL_POOL_STAT6_INVALID_FREE_TOKEN_VALID_SHIFT	31

#define FPM_CTRL_POOL_STAT7(pool)				(0x00068 + (pool) * 0x20)
#define FPM_CTRL_POOL_STAT7_INVALID_MCAST_TOKEN_MASK		0x7fffffff
#define FPM_CTRL_POOL_STAT7_INVALID_MCAST_TOKEN_SHIFT		0
#define FPM_CTRL_POOL_STAT7_INVALID_MCAST_TOKEN_VALID_MASK	0x80000000
#define FPM_CTRL_POOL_STAT7_INVALID_MCAST_TOKEN_VALID_SHIFT	31

#define FPM_CTRL_POOL_STAT8(pool)				(0x0006c + (pool) * 0x20)

#define FPM_CTRL_POOL1_XON_XOFF_CFG     0x000c0

#define FPM_CTRL_SPARE                  0x00120

#define FPM_CTRL_TOKEN_RECOVER_CTL      0x00130 /* [RW] Token Recovery Control Register */
#define FPM_CTRL_TOKEN_RECOVER_CTL_CLR_RECOVERED_TOKEN_COUNT_MASK 0x00000040
#define FPM_CTRL_TOKEN_RECOVER_CTL_CLR_RECOVERED_TOKEN_COUNT_SHIFT 6
#define FPM_CTRL_TOKEN_RECOVER_CTL_CLR_EXPIRED_TOKEN_COUNT_MASK 0x00000020
#define FPM_CTRL_TOKEN_RECOVER_CTL_CLR_EXPIRED_TOKEN_COUNT_SHIFT 5
#define FPM_CTRL_TOKEN_RECOVER_CTL_FORCE_TOKEN_RECLAIM_MASK 0x00000010
#define FPM_CTRL_TOKEN_RECOVER_CTL_FORCE_TOKEN_RECLAIM_SHIFT 4
#define FPM_CTRL_TOKEN_RECOVER_CTL_TOKEN_RECLAIM_ENA_MASK 0x00000008
#define FPM_CTRL_TOKEN_RECOVER_CTL_TOKEN_RECLAIM_ENA_SHIFT 3
#define FPM_CTRL_TOKEN_RECOVER_CTL_TOKEN_REMARK_ENA_MASK  0x00000004
#define FPM_CTRL_TOKEN_RECOVER_CTL_TOKEN_REMARK_ENA_SHIFT 2
#define FPM_CTRL_TOKEN_RECOVER_CTL_SINGLE_PASS_ENA_MASK   0x00000002
#define FPM_CTRL_TOKEN_RECOVER_CTL_SINGLE_PASS_ENA_SHIFT  1
#define FPM_CTRL_TOKEN_RECOVER_CTL_TOKEN_RECOVER_ENA_MASK 0x00000001
#define FPM_CTRL_TOKEN_RECOVER_CTL_TOKEN_RECOVER_ENA_SHIFT 0

#define FPM_CTRL_SHORT_AGING_TIMER      0x00134 /* [RW] Short Aging Timer */
#define FPM_CTRL_LONG_AGING_TIMER       0x00138 /* [RW] Long Aging Timer */
#define FPM_CTRL_CACHE_RECYCLE_TIMER    0x0013c /* [RW] Token Cache Recycle Timer */

#define FPM_CTRL_EXPIRED_TOKEN_COUNT_POOL1 0x00140 /* [RO] Expired Token Count */
#define FPM_CTRL_RECOVERED_TOKEN_COUNT_POOL1 0x00144 /* [RO] Recovered Token Count */
#define FPM_CTRL_EXPIRED_TOKEN_COUNT_POOL2 0x00148 /* [RO] Expired Token Count */
#define FPM_CTRL_RECOVERED_TOKEN_COUNT_POOL2 0x0014c /* [RO] Recovered Token Count */
#define FPM_CTRL_TOKEN_RECOVER_START_END_POOL1 0x00150 /* [RW] Token Recovery Start/End Range */
#define FPM_CTRL_TOKEN_RECOVER_START_END_POOL2 0x00154 /* [RW] Token Recovery Start/End Range */

#define FPM_POOL_POOL1_ALLOC_DEALLOC			0x00200
#define FPM_POOL_POOL2_ALLOC_DEALLOC			0x00208
#define FPM_POOL_POOL3_ALLOC_DEALLOC			0x00210
#define FPM_POOL_POOL4_ALLOC_DEALLOC			0x00218
#define FPM_POOL_MULTI_CTL				0x00224
#define FPM_POOL_MULTI_UPDATE_TYPE_MASK			0x00000800
#define FPM_POOL_MULTI_UPDATE_TYPE_SHIFT		11
#define FPM_POOL_MULTI_TOKEN_MULTI_MASK			0x0000007f
#define FPM_POOL_MULTI_TOKEN_MULTI_SHIFT		0


#define FPM_POOL_ALLOC_DEALLOC_TOKEN_VALID_MASK     	0x80000000
#define FPM_POOL_ALLOC_DEALLOC_TOKEN_VALID_SHIFT    	31
#define FPM_POOL_ALLOC_DEALLOC_DDR_MASK             	0x20000000
#define FPM_POOL_ALLOC_DEALLOC_DDR_SHIFT            	29
#define FPM_POOL_ALLOC_DEALLOC_TOKEN_INDEX_MASK     	0x1ffff000
#define FPM_POOL_ALLOC_DEALLOC_TOKEN_INDEX_SHIFT    	12
#define FPM_POOL_ALLOC_DEALLOC_TOKEN_SIZE_MASK      	0x00000fff
#define FPM_POOL_ALLOC_DEALLOC_TOKEN_SIZE_SHIFT     	0


#define FPM_POOL_n_FPM_POOL1_ALLOC_DEALLOC(pool)	(0x00400 + (pool) * 0x200)
#define FPM_POOL_n_FPM_POOL2_ALLOC_DEALLOC(pool)	(0x00408 + (pool) * 0x200)
#define FPM_POOL_n_FPM_POOL3_ALLOC_DEALLOC(pool)	(0x00410 + (pool) * 0x200)
#define FPM_POOL_n_FPM_POOL4_ALLOC_DEALLOC(pool)	(0x00418 + (pool) * 0x200)
#define FPM_POOL_n_FPM_POOL_MULTI(pool)			(0x00424 + (pool) * 0x200)

/* Ctl regs */
#define FPM_CTL		\
	(fpm->reg_vbase + ((FPM_CTRL_CTL) >> 2))
#define FPM_SET_POOL_INIT(pool) \
	fpm_reg_write_mask(FPM_CTL, \
	   FPM_CTRL_CTL_INIT_MEM_MASK(pool), \
	   FPM_CTRL_CTL_INIT_MEM_MASK(pool));
#define FPM_GET_POOL_INIT(pool) \
	((fpm_reg_read(FPM_CTL) & \
	  FPM_CTRL_CTL_INIT_MEM_MASK(pool)) >> \
	 FPM_CTRL_CTL_INIT_MEM_SHIFT(pool))
#define FPM_SET_POOL_ENABLE(pool) \
	fpm_reg_write_mask(FPM_CTL, \
	   FPM_CTRL_CTL_POOL_ENABLE_MASK(pool), \
	   FPM_CTRL_CTL_POOL_ENABLE_MASK(pool));
#define FPM_SET_BB_RESET(reset) \
	fpm_reg_write_mask(FPM_CTL, \
		FPM_CTRL_CTL_FPM_BB_SOFT_RESET_MASK, \
		((reset) << FPM_CTRL_CTL_FPM_BB_SOFT_RESET_SHIFT));

#define FPM_CTRL_POOL_FP_BUF_SIZE		\
	(fpm->reg_vbase + ((FPM_CTRL_POOL_CFG_FP_BUF_SIZE) >> 2))
#define FPM_SET_CHUNK_SIZE(val)	\
	fpm_reg_write_mask(FPM_CTRL_POOL_FP_BUF_SIZE, \
	 FPM_CTRL_POOL_CFG_FP_BUF_SIZE_MASK, \
	 (val) << FPM_CTRL_POOL_CFG_FP_BUF_SIZE_SHIFT);
#define FPM_GET_CHUNK_SIZE()	\
	((fpm_reg_read(FPM_CTRL_POOL_FP_BUF_SIZE) & \
	  FPM_CTRL_POOL_CFG_FP_BUF_SIZE_MASK) >> \
	 FPM_CTRL_POOL_CFG_FP_BUF_SIZE_SHIFT)
#define FPM_CHUNK_SIZE_512	0
#define FPM_CHUNK_SIZE_256	1

/* Base regs */
#define FPM_CTL_POOL_BASE_ADDR(pool)	\
	(fpm->reg_vbase + ((FPM_CTRL_POOL_CFG_BASE_ADDRESS(pool)) >> 2))
#define FPM_SET_POOL_BASE_ADDR(pool, val) \
	fpm_reg_write(FPM_CTL_POOL_BASE_ADDR(pool), (val));
#define FPM_GET_POOL_BASE_ADDR(pool) \
	(fpm_reg_read(FPM_CTL_POOL_BASE_ADDR(pool)) & \
	 FPM_CTRL_POOL_CFG_BASE_ADDRESS_MASK)


/* Pool weight regs */
#define FPM_CTL_FPM_WEIGHT		\
	(fpm->reg_vbase + ((FPM_CTRL_WEIGHT) >> 2))
#define FPM_SET_ALLOC_WEIGHT(pool, weight) \
{ \
	if ((pool) == 0) \
		fpm_reg_write_mask(FPM_CTL_FPM_WEIGHT, \
		FPM_CTRL_WEIGHT_DDR0_ALLOC_WEIGHT_MASK, \
		(weight) << FPM_CTRL_WEIGHT_DDR0_ALLOC_WEIGHT_SHIFT); \
	else \
		fpm_reg_write_mask(FPM_CTL_FPM_WEIGHT, \
		FPM_CTRL_WEIGHT_DDR1_ALLOC_WEIGHT_MASK, \
		(weight) << FPM_CTRL_WEIGHT_DDR1_ALLOC_WEIGHT_SHIFT); \
}
#define FPM_GET_ALLOC_WEIGHT(pool) \
	(((pool) == 0) ? ((fpm_reg_read(FPM_CTL_FPM_WEIGHT) & \
			   FPM_CTRL_WEIGHT_DDR0_ALLOC_WEIGHT_MASK) >> \
			   FPM_CTRL_WEIGHT_DDR0_ALLOC_WEIGHT_SHIFT) : \
			 ((fpm_reg_read(FPM_CTL_FPM_WEIGHT) & \
			   FPM_CTRL_WEIGHT_DDR1_ALLOC_WEIGHT_MASK) >> \
			   FPM_CTRL_WEIGHT_DDR1_ALLOC_WEIGHT_SHIFT))
#define FPM_SET_FREE_WEIGHT(pool, weight) \
{ \
	if ((pool) == 0) \
		fpm_reg_write_mask(FPM_CTL_FPM_WEIGHT, \
		FPM_CTRL_WEIGHT_DDR0_FREE_WEIGHT_MASK, \
		(weight) << FPM_CTRL_WEIGHT_DDR0_FREE_WEIGHT_SHIFT); \
	else \
		fpm_reg_write_mask(FPM_CTL_FPM_WEIGHT, \
		FPM_CTRL_WEIGHT_DDR1_FREE_WEIGHT_MASK, \
		(weight) << FPM_CTRL_WEIGHT_DDR1_FREE_WEIGHT_SHIFT); \
}
#define FPM_GET_FREE_WEIGHT(pool) \
	(((pool) == 0) ? ((fpm_reg_read(FPM_CTL_FPM_WEIGHT) & \
			   FPM_CTRL_WEIGHT_DDR0_FREE_WEIGHT_MASK) >> \
			   FPM_CTRL_WEIGHT_DDR0_FREE_WEIGHT_SHIFT) : \
			 ((fpm_reg_read(FPM_CTL_FPM_WEIGHT) & \
			   FPM_CTRL_WEIGHT_DDR1_FREE_WEIGHT_MASK) >> \
			   FPM_CTRL_WEIGHT_DDR1_FREE_WEIGHT_SHIFT))

/* IRQ regs */
#define FPM_POOL_INTR_MASK(pool) \
	(fpm->reg_vbase + ((FPM_CTRL_POOL_INTR_MSK(pool)) >> 2))
#define FPM_POOL_INTR_STATUS(pool) \
	(fpm->reg_vbase + ((FPM_CTRL_POOL_INTR_STS(pool)) >> 2))
#define FPM_GET_IRQ_MASK(pool) \
	fpm_reg_read(FPM_POOL_INTR_MASK(pool))
#define FPM_SET_IRQ_MASK(pool, mask) \
	fpm_reg_write(FPM_POOL_INTR_MASK(pool), (mask));
#define FPM_GET_IRQ_STATUS(pool) \
	(fpm_reg_read(FPM_POOL_INTR_STATUS(pool)))
#define FPM_CLEAR_IRQ_STATUS(pool, mask) \
	fpm_reg_write(FPM_POOL_INTR_STATUS(pool), (mask));

/* Spare regs */
#define FPM_CTL_SPARE		\
	(fpm->reg_vbase + ((FPM_CTRL_SPARE) >> 2))
#define FPM_SET_CTL_SPARE(val)	(fpm_reg_write(FPM_CTL_SPARE, val))
#define FPM_GET_CTL_SPARE()	(fpm_reg_read(FPM_CTL_SPARE))

/* Pool management regs */
#define FPM_ALLOC_8_CHUNKS_BASE(base)	\
	((base) + ((FPM_POOL_POOL1_ALLOC_DEALLOC) >> 2))
#define FPM_ALLOC_4_CHUNKS_BASE(base)	\
	((base) + ((FPM_POOL_POOL2_ALLOC_DEALLOC) >> 2))
#define FPM_ALLOC_2_CHUNKS_BASE(base)	\
	((base) + ((FPM_POOL_POOL3_ALLOC_DEALLOC) >> 2))
#define FPM_ALLOC_1_CHUNKS_BASE(base)	\
	((base) + ((FPM_POOL_POOL4_ALLOC_DEALLOC) >> 2))
#define FPM_DEALLOC_BASE(base)	FPM_ALLOC_8_CHUNKS_BASE(base)
#define FPM_POOL_MULTI_BASE(base)	\
	((base) + ((FPM_POOL_MULTI_CTL) >> 2))
#define FPM_ALLOC_8_CHUNKS	\
	FPM_ALLOC_8_CHUNKS_BASE(fpm->reg_vbase)
#define FPM_ALLOC_4_CHUNKS	\
	FPM_ALLOC_4_CHUNKS_BASE(fpm->reg_vbase)
#define FPM_ALLOC_2_CHUNKS	\
	FPM_ALLOC_2_CHUNKS_BASE(fpm->reg_vbase)
#define FPM_ALLOC_1_CHUNKS	\
	FPM_ALLOC_1_CHUNKS_BASE(fpm->reg_vbase)
#define FPM_DEALLOC		\
	FPM_DEALLOC_BASE(fpm->reg_vbase)
#define FPM_POOL_MULTI	\
	FPM_POOL_MULTI_BASE(fpm->reg_vbase)
#define FPM_ALLOC_8_CHUNKS_PHYS	\
	(FPM_ALLOC_8_CHUNKS_BASE(fpm->reg_pbase))
#define FPM_ALLOC_4_CHUNKS_PHYS	\
	(FPM_ALLOC_4_CHUNKS_BASE(fpm->reg_pbase))
#define FPM_ALLOC_2_CHUNKS_PHYS	\
	(FPM_ALLOC_2_CHUNKS_BASE(fpm->reg_pbase))
#define FPM_ALLOC_1_CHUNKS_PHYS	\
	(FPM_ALLOC_1_CHUNKS_BASE(fpm->reg_pbase))
#define FPM_POOL_MULTI_PHYS	\
	(FPM_POOL_MULTI_BASE(fpm->reg_pbase))
#define FPM_TOKEN_MULTIINCR_SHIFT	\
	FPM_POOL_MULTI_UPDATE_TYPE_SHIFT
#define FPM_TOKEN_MULTIVAL_MASK		\
	FPM_POOL_MULTI_TOKEN_MULTI_MASK

#define FPM_POOL_n_ALLOC_8_CHUNKS_BASE(pool, base)	\
	((base) + ((FPM_POOL_n_FPM_POOL1_ALLOC_DEALLOC(pool)) >> 2))
#define FPM_POOL_n_ALLOC_4_CHUNKS_BASE(pool, base)	\
	((base) + ((FPM_POOL_n_FPM_POOL2_ALLOC_DEALLOC(pool)) >> 2))
#define FPM_POOL_n_ALLOC_2_CHUNKS_BASE(pool, base)	\
	((base) + ((FPM_POOL_n_FPM_POOL3_ALLOC_DEALLOC(pool)) >> 2))
#define FPM_POOL_n_ALLOC_1_CHUNKS_BASE(pool, base)	\
	((base) + ((FPM_POOL_n_FPM_POOL4_ALLOC_DEALLOC(pool)) >> 2))
#define FPM_POOL_n_DEALLOC_BASE(pool, base) FPM_POOL_n_ALLOC_8_CHUNKS_BASE(pool, base)
#define FPM_POOL_n_POOL_MULTI_BASE(pool, base)	\
	((base) + ((FPM_POOL_n_FPM_POOL_MULTI(pool)) >> 2))
#define FPM_ALLOC_8_CHUNKS_FROM_POOL(pool) \
	fpm_reg_read(FPM_POOL_n_ALLOC_8_CHUNKS_BASE(pool, fpm->reg_vbase))
#define FPM_ALLOC_4_CHUNKS_FROM_POOL(pool) \
	fpm_reg_read(FPM_POOL_n_ALLOC_4_CHUNKS_BASE(pool, fpm->reg_vbase))
#define FPM_ALLOC_2_CHUNKS_FROM_POOL(pool) \
	fpm_reg_read(FPM_POOL_n_ALLOC_2_CHUNKS_BASE(fpm->reg_vbase))
#define FPM_ALLOC_1_CHUNKS_FROM_POOL(pool) \
	fpm_reg_read(FPM_POOL_n_ALLOC_1_CHUNKS_BASE(pool, fpm->reg_vbase))
#define FPM_DEALLOC_TO_POOL(pool, token) \
	fpm_reg_write(FPM_POOL_n_DEALLOC_BASE(pool, fpm->reg_vbase, token));

/* Token format */
#ifdef CONFIG_BCM_FPM_TOKEN_FORMAT_VARIABLE
/*
Chunk Size: 256
       ---------------------------------
  256B | P:11 | Index:22b | Length:8b  | 4M pkts
       ---------------------------------
  512B | P:10 | Index:21b | Length:9b  | 2M pkts
       ---------------------------------
 1024B | P:01 | Index:20b | Length:10b | 1M pkts
       ---------------------------------
 2048B | P:00 | Index:19b | Length:11b | 512k pkts
       ---------------------------------

Chunk Size: 512
       ---------------------------------
  512B | P:11 | Index:21b | Length:9b  | 2M pkts
       ---------------------------------
 1024B | P:10 | Index:20b | Length:10b | 1M pkts
       ---------------------------------
 2048B | P:01 | Index:19b | Length:11b | 512k pkts
       ---------------------------------
 4096B | P:00 | Index:18b | Length:12b | 256k pkts
       ---------------------------------
*/
#define FPM_TOKEN_VARIABLE_FORMAT_INVALID_MASK	\
        0XFFFFFFFF
#define FPM_TOKEN_VARIABLE_FORMAT_CHUNK_MASK	\
        0xC0000000
#define FPM_TOKEN_VARIABLE_FORMAT_CHUNK_SHIFT	\
        30
#define FPM_TOKEN_VARIABLE_FORMAT_DDRPOOL_MASK	\
        0x20000000
#define FPM_TOKEN_VARIABLE_FORMAT_DDRPOOL_SHIFT	\
        29
#define FPM_TOKEN_VARIABLE_FORMAT256_MAXINDEX_MASK	\
        0x3FFFFF00
#define FPM_TOKEN_VARIABLE_FORMAT256_MAXINDEX_SHIFT	\
        8
#define FPM_TOKEN_VARIABLE_FORMAT256_MAXSIZE_MASK	\
        0x7FF
#define FPM_TOKEN_VARIABLE_FORMAT512_MAXINDEX_MASK	\
        0x3FFFFE00
#define FPM_TOKEN_VARIABLE_FORMAT512_MAXINDEX_SHIFT	\
        9
#define FPM_TOKEN_VARIABLE_FORMAT512_MAXSIZE_MASK	\
        0xFFF
#define FPM_TOKEN_VARIABLE_FORMAT256_MAXINDEX_BITS 22
#define FPM_TOKEN_VARIABLE_FORMAT512_MAXINDEX_BITS 21
#else
#define FPM_TOKEN_FIXED_FORMAT_VALID_MASK	\
	FPM_POOL_ALLOC_DEALLOC_TOKEN_VALID_MASK
#define FPM_TOKEN_FIXED_FORMAT_POOL_MASK	\
	FPM_POOL_ALLOC_DEALLOC_DDR_MASK
#define FPM_TOKEN_FIXED_FORMAT_POOL_SHIFT	\
	FPM_POOL_ALLOC_DEALLOC_DDR_SHIFT
#define FPM_TOKEN_FIXED_FORMAT_INDEX_MASK	\
	FPM_POOL_ALLOC_DEALLOC_TOKEN_INDEX_MASK
#define	FPM_TOKEN_FIXED_FORMAT_INDEX_SHIFT	\
	FPM_POOL_ALLOC_DEALLOC_TOKEN_INDEX_SHIFT
#define FPM_MAX_TOKEN_INDEX	\
	((FPM_TOKEN_FIXED_FORMAT_INDEX_MASK >> FPM_TOKEN_FIXED_FORMAT_INDEX_SHIFT) + 1)
#define FPM_TOKEN_FIXED_FORMAT_SIZE_MASK	\
	FPM_POOL_ALLOC_DEALLOC_TOKEN_SIZE_MASK
#define FPM_TOKEN_FIXED_FORMAT_SIZE_SHIFT	\
	FPM_POOL_ALLOC_DEALLOC_TOKEN_SIZE_SHIFT
#define FPM_POOL_FIXED_FORMAT_MAXINDEX_BITS 17
#endif

/* Status regs */
#define FPM_POOL_STAT1(pool)		\
	(fpm->reg_vbase + (FPM_CTRL_POOL_STAT1(pool) >> 2))
#define FPM_GET_UNDERFLOW(pool) \
	((fpm_reg_read(FPM_POOL_STAT1(pool)) & \
	  FPM_CTRL_POOL_STAT1_UNDRFL_MASK) >> \
	  FPM_CTRL_POOL_STAT1_UNDRFL_SHIFT)
#define FPM_GET_OVERFLOW(pool) \
	((fpm_reg_read(FPM_POOL_STAT1(pool)) & \
	  FPM_CTRL_POOL_STAT1_OVRFL_MASK) >> \
	  FPM_CTRL_POOL_STAT1_OVRFL_SHIFT)
#define FPM_POOL_STAT2(pool)		\
	(fpm->reg_vbase + (FPM_CTRL_POOL_STAT2(pool) >> 2))
#define FPM_GET_TOK_AVAIL(pool) \
	((fpm_reg_read(FPM_POOL_STAT2(pool)) & \
	  FPM_CTRL_POOL_STAT2_NUM_OF_TOKENS_AVAILABLE_MASK) >> \
	  FPM_CTRL_POOL_STAT2_NUM_OF_TOKENS_AVAILABLE_SHIFT)
#define FPM_GET_ALLOC_FIFO_EMPTY(pool) \
	((fpm_reg_read(FPM_POOL_STAT2(pool)) & \
	  FPM_CTRL_POOL_STAT2_ALLOC_FIFO_EMPTY_MASK) >> \
	  FPM_CTRL_POOL_STAT2_ALLOC_FIFO_EMPTY_SHIFT)
#define FPM_GET_ALLOC_FIFO_FULL(pool) \
	((fpm_reg_read(FPM_POOL_STAT2(pool)) & \
	  FPM_CTRL_POOL_STAT2_ALLOC_FIFO_FULL_MASK) >> \
	  FPM_CTRL_POOL_STAT2_ALLOC_FIFO_FULL_SHIFT)
#define FPM_GET_FREE_FIFO_EMPTY(pool) \
	((fpm_reg_read(FPM_POOL_STAT2(pool)) & \
	  FPM_CTRL_POOL_STAT2_FREE_FIFO_EMPTY_MASK) >> \
	  FPM_CTRL_POOL_STAT2_FREE_FIFO_EMPTY_SHIFT)
#define FPM_GET_FREE_FIFO_FULL(pool) \
	((fpm_reg_read(FPM_POOL_STAT2(pool)) & \
	  FPM_CTRL_POOL_STAT2_FREE_FIFO_FULL_MASK) >> \
	  FPM_CTRL_POOL_STAT2_FREE_FIFO_FULL_SHIFT)
#define FPM_GET_POOL_FULL(pool) \
	((fpm_reg_read(FPM_POOL_STAT2(pool)) & \
	  FPM_CTRL_POOL_STAT2_POOL_FULL_MASK) >> \
	 FPM_CTRL_POOL_STAT2_POOL_FULL_SHIFT)
#define FPM_POOL_STAT3(pool)		\
	(fpm->reg_vbase + (FPM_CTRL_POOL_STAT3(pool) >> 2))
#define FPM_GET_INVAL_TOK_FREES(pool) \
	((fpm_reg_read(FPM_POOL_STAT3(pool)) & \
	  FPM_CTRL_POOL_STAT3_NUM_OF_NOT_VALID_TOKEN_FREES_MASK) >> \
	  FPM_CTRL_POOL_STAT3_NUM_OF_NOT_VALID_TOKEN_FREES_SHIFT)
#define FPM_POOL_STAT4(pool)		\
	(fpm->reg_vbase + (FPM_CTRL_POOL_STAT4(pool) >> 2))
#define FPM_GET_INVAL_TOK_MULTI(pool) \
	((fpm_reg_read(FPM_POOL_STAT4(pool)) & \
	  FPM_CTRL_POOL_STAT4_NUM_OF_NOT_VALID_TOKEN_MULTI_MASK) >> \
	 FPM_CTRL_POOL_STAT4_NUM_OF_NOT_VALID_TOKEN_MULTI_SHIFT)
#define FPM_POOL_STAT5(pool)		\
	(fpm->reg_vbase + (FPM_CTRL_POOL_STAT5(pool) >> 2))
#define FPM_GET_MEM_CORRUPT_TOK(pool) \
	((fpm_reg_read(FPM_POOL_STAT5(pool)) & \
	  FPM_CTRL_POOL_STAT5_MEM_CORRUPT_STS_RELATED_ALLOC_TOKEN_MASK) >> \
	  FPM_CTRL_POOL_STAT5_MEM_CORRUPT_STS_RELATED_ALLOC_TOKEN_SHIFT)
#define FPM_GET_MEM_CORRUPT_TOK_VALID(pool) \
	((fpm_reg_read(FPM_POOL_STAT5(pool)) & \
	  FPM_CTRL_POOL_STAT5_MEM_CORRUPT_STS_RELATED_ALLOC_TOKEN_VALID_MASK) >> \
	  FPM_CTRL_POOL_STAT5_MEM_CORRUPT_STS_RELATED_ALLOC_TOKEN_VALID_SHIFT)
#define FPM_POOL_STAT6(pool)		\
	(fpm->reg_vbase + (FPM_CTRL_POOL_STAT6(pool) >> 2))
#define FPM_GET_INVALID_FREE_TOK(pool) \
	((fpm_reg_read(FPM_POOL_STAT6(pool)) & \
	  FPM_CTRL_POOL_STAT6_INVALID_FREE_TOKEN_MASK) >> \
	  FPM_CTRL_POOL_STAT6_INVALID_FREE_TOKEN_SHIFT)
#define FPM_GET_INVALID_FREE_TOK_VALID(pool) \
	((fpm_reg_read(FPM_POOL_STAT6(pool)) & \
	  FPM_CTRL_POOL_STAT6_INVALID_FREE_TOKEN_VALID_MASK) >> \
	  FPM_CTRL_POOL_STAT6_INVALID_FREE_TOKEN_VALID_SHIFT)
#define FPM_CLEAR_INVALID_FREE_TOK(pool) \
	fpm_reg_write(FPM_POOL_STAT6(pool), 0);
#define FPM_POOL_STAT7(pool)		\
	(fpm->reg_vbase + (FPM_CTRL_POOL_STAT7(pool) >> 2))
#define FPM_GET_INVALID_MCAST_TOK(pool) \
	((fpm_reg_read(FPM_POOL_STAT7(pool)) & \
	  FPM_CTRL_POOL_STAT7_INVALID_MCAST_TOKEN_MASK) >> \
	  FPM_CTRL_POOL_STAT7_INVALID_MCAST_TOKEN_SHIFT)
#define FPM_GET_INVALID_MCAST_TOK_VALID(pool) \
	((fpm_reg_read(FPM_POOL_STAT7(pool)) & \
	  FPM_CTRL_POOL_STAT7_INVALID_MCAST_TOKEN_VALID_MASK) >> \
	  FPM_CTRL_POOL_STAT7_INVALID_MCAST_TOKEN_VALID_SHIFT)
#define FPM_CLEAR_INVALID_MCAST_TOK(pool) \
	fpm_reg_write(FPM_POOL_STAT7(pool), 0);

/* Token recovery regs */
#define FPM_TOK_RECOVER_CTRL		\
	(fpm->reg_vbase + ((FPM_CTRL_TOKEN_RECOVER_CTL) >> 2))
#define FPM_SET_TOK_RECOVER_ENABLE(enable) \
	fpm_reg_write_mask(FPM_TOK_RECOVER_CTRL, \
	   FPM_CTRL_TOKEN_RECOVER_CTL_TOKEN_RECOVER_ENA_MASK, \
	   enable ? FPM_CTRL_TOKEN_RECOVER_CTL_TOKEN_RECOVER_ENA_MASK : 0);
#define FPM_SET_TOK_RECOVER_SINGLE_PASS(enable) \
	fpm_reg_write_mask(FPM_TOK_RECOVER_CTRL, \
	   FPM_CTRL_TOKEN_RECOVER_CTL_SINGLE_PASS_ENA_MASK, \
	   enable ? FPM_CTRL_TOKEN_RECOVER_CTL_SINGLE_PASS_ENA_MASK : 0);
#define FPM_SET_TOK_RECOVER_REMARK(enable) \
	fpm_reg_write_mask(FPM_TOK_RECOVER_CTRL, \
	   FPM_CTRL_TOKEN_RECOVER_CTL_TOKEN_REMARK_ENA_MASK, \
	   enable ? FPM_CTRL_TOKEN_RECOVER_CTL_TOKEN_REMARK_ENA_MASK : 0);
#define FPM_SET_TOK_RECOVER_AUTO_RECLAIM(enable) \
	   fpm_reg_write_mask(FPM_TOK_RECOVER_CTRL, \
	      FPM_CTRL_TOKEN_RECOVER_CTL_TOKEN_RECLAIM_ENA_MASK, \
	      enable ? FPM_CTRL_TOKEN_RECOVER_CTL_TOKEN_RECLAIM_ENA_MASK : 0);
#define FPM_SET_TOK_RECOVER_FORCE_RECLAIM(enable) \
	      fpm_reg_write_mask(FPM_TOK_RECOVER_CTRL, \
		 FPM_CTRL_TOKEN_RECOVER_CTL_FORCE_TOKEN_RECLAIM_MASK, \
		 enable ? FPM_CTRL_TOKEN_RECOVER_CTL_FORCE_TOKEN_RECLAIM_MASK : 0);
#define FPM_CLEAR_EXPIRED_TOK_COUNT() \
	fpm_reg_write_mask(FPM_TOK_RECOVER_CTRL, \
	   FPM_CTRL_TOKEN_RECOVER_CTL_CLR_EXPIRED_TOKEN_COUNT_MASK, \
	   FPM_CTRL_TOKEN_RECOVER_CTL_CLR_EXPIRED_TOKEN_COUNT_MASK);
#define FPM_CLEAR_RECOVERED_TOK_COUNT() \
	fpm_reg_write_mask(FPM_TOK_RECOVER_CTRL, \
	   FPM_CTRL_TOKEN_RECOVER_CTL_CLR_RECOVERED_TOKEN_COUNT_MASK, \
	   FPM_CTRL_TOKEN_RECOVER_CTL_CLR_RECOVERED_TOKEN_COUNT_MASK);
#define FPM_SHORT_AGING_TIMER		\
	(fpm->reg_vbase + ((FPM_CTRL_SHORT_AGING_TIMER) >> 2))
#define FPM_SET_SHORT_AGING_TIMER(value) \
	fpm_reg_write(FPM_SHORT_AGING_TIMER, (value));
#define FPM_LONG_AGING_TIMER		\
	(fpm->reg_vbase + ((FPM_CTRL_LONG_AGING_TIMER) >> 2))
#define FPM_SET_LONG_AGING_TIMER(value) \
	fpm_reg_write(FPM_LONG_AGING_TIMER, (value));
#define FPM_CACHE_RECYCLE_TIMER		\
	(fpm->reg_vbase + ((FPM_CTRL_CACHE_RECYCLE_TIMER) >> 2))
#define FPM_SET_CACHE_RECYCLE_TIMER(value) \
	fpm_reg_write(FPM_CACHE_RECYCLE_TIMER, (value));
#define FPM_POOL1_EXPIRED_TOK_COUNT		\
	(fpm->reg_vbase + ((FPM_CTRL_EXPIRED_TOKEN_COUNT_POOL1) >> 2))
#define FPM_POOL2_EXPIRED_TOK_COUNT		\
	(fpm->reg_vbase + ((FPM_CTRL_EXPIRED_TOKEN_COUNT_POOL2) >> 2))
#define FPM_GET_EXPIRED_TOK_COUNT(pool) \
	(((pool) == 0) ? fpm_reg_read(FPM_POOL1_EXPIRED_TOK_COUNT) : \
			 fpm_reg_read(FPM_POOL2_EXPIRED_TOK_COUNT))
#define FPM_POOL1_RECOVERED_TOK_COUNT		\
	(fpm->reg_vbase + ((FPM_CTRL_RECOVERED_TOKEN_COUNT_POOL1) >> 2))
#define FPM_POOL2_RECOVERED_TOK_COUNT		\
	(fpm->reg_vbase + ((FPM_CTRL_RECOVERED_TOKEN_COUNT_POOL2) >> 2))
#define FPM_GET_RECOVERED_TOK_COUNT(pool) \
	(((pool) == 0) ? fpm_reg_read(FPM_POOL1_RECOVERED_TOK_COUNT) : \
			 fpm_reg_read(FPM_POOL2_RECOVERED_TOK_COUNT))

#define FPM_POOL1_XON_XOFF_THRESHOLD		\
	(fpm->reg_vbase + ((FPM_CTRL_POOL1_XON_XOFF_CFG) >> 2))
#define FPM_GET_XON_XOFF_THRESHOLD(pool) \
	(((pool) == 0) ? fpm_reg_read(FPM_POOL1_XON_XOFF_THRESHOLD) : 0)
#define FPM_SET_XON_XOFF_THRESHOLD(pool, val) \
	(((pool) == 0) ? fpm_reg_write(FPM_POOL1_XON_XOFF_THRESHOLD, val) : 0)

#endif
