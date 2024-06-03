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
 ****************************************************************************
 * Author: Tim Ross <tross@broadcom.com>
 *****************************************************************************/
#ifndef __DQM_HW_H
#define __DQM_HW_H

#include <linux/types.h>
#include <linux/io.h>

static inline u32 dqm_reg_read(u32 *reg)
{
	u32 val;

	val = __raw_readl(reg);
	return val;
}

static inline void dqm_reg_write(u32 *reg, u32 val)
{
	__raw_writel(val, reg);
}

static inline void dqm_rx_msgs_noburst(u32 *reg, int msgsz, int count,
				       struct msgstruct *msg)
{
	int i, j;

	for (i = 0; i < count; i++) {
		for (j = 0; j < msgsz; j++)
			msg[i].msgdata[j] = dqm_reg_read(&reg[j]);
		msg[i].msglen = msgsz;
	}
}

#if defined(CONFIG_ARM)

#include "dqm_hw_arm32.h"

#else

static inline void dqm_rx_msgs(u32 *reg, int msgsz, int count,
			       struct msgstruct *msg)
{
	dqm_rx_msgs_noburst(reg, msgsz, count, msg);
}

#if defined(CONFIG_ARM64)

#include "dqm_hw_arm64.h"

#endif

#endif

static inline void dqm_reg_write_mask(u32 *reg, u32 mask, u32 val)
{
	u32 v;

	v = __raw_readl(reg);
	v &= ~mask;
	v |= (val & mask);
	__raw_writel(v, reg);
}

struct dqmtok {
	u32 buf_size;
	u32 buf_base;
	u32 idx2ptr_idx;
	u32 idx2ptr_ptr;
	u32 ptr2idx_ptr;
	u32 ptr2idx_idx;
	u32 buf_size2;
	u32 rsvd0;
	u32 buf_base1;
};

struct dqmctl {
	u32 size;
	u32 cfga;
	u32 cfgb;
	u32 cfgc;
};

struct dqmctl_ol {
	u32 size;
	u32 cfga;
	u32 cfgb;
	u32 cfgc;
	u32 push_tok;
	u32 push_tok_next;
	u32 pop_tok;
	u32 pop_tok_next;
};

struct dqmtmr {
	u32 ctl;
	u32 cnt;
};

#define DQM_Q_WORD_COUNT	4
#define DQM_OL_Q_WORD_COUNT	8

#define DQM_OL_Q_DMA_SIZE	128
#define DQM_OL_Q_DMA_CACHE_SIZE	4
#define DQM_OL_Q_QSM_BYTES	(DQM_OL_Q_DMA_SIZE * DQM_OL_Q_DMA_CACHE_SIZE)

#define DQM_Q_QSM_ALIGN		8

#ifndef CONFIG_BCM_DQM_256M_OL_DEPTH
#define DQM_OL_Q_MAX_DEPTH(msg_size) \
	((msg_size) == 1 ? 16288 : (msg_size) == 2 ? 8144 : \
	 (msg_size) == 3 ? 5429 : 4072)
#else
#define DQM_OL_Q_MAX_DEPTH(msg_size) \
	((msg_size) == 1 ? 286435360 : (msg_size) == 2 ? 134217632 : \
	 (msg_size) == 3 ? 89478389 : 67108768)
#endif

#define DQM_TOK_SIZE_MASK	0x00000003
#define DQM_TOK_SIZE_SHIFT	0
#define DQM_Q_SIZE_MASK		0xffff0000
#define DQM_Q_SIZE_SHIFT	16
#define DQM_Q_ADDR_MASK		0x0000ffff
#define DQM_Q_ADDR_SHIFT	0
#define DQM_NUM_TOK_MASK	0x3fff0000
#define DQM_NUM_TOK_SHIFT	16
#define DQM_LWM_MASK		0x00003fff
#define DQM_LWM_SHIFT		0
#define DQM_HWM_MASK		0x00003fff
#define DQM_HWM_SHIFT		0
#define DQM_Q_SPACE_MASK	0x00003fff
#define DQM_Q_SPACE_SHIFT	0
#ifndef CONFIG_BCM_DQM_256M_OL_DEPTH
#define DQM_OL_NUM_TOK_MASK	DQM_NUM_TOK_MASK
#define DQM_OL_NUM_TOK_SHIFT	DQM_NUM_TOK_SHIFT
#define DQM_OL_LWM_MASK		DQM_LWM_MASK
#define DQM_OL_LWM_SHIFT	DQM_LWM_SHIFT
#define DQM_OL_HWM_MASK		DQM_HWM_MASK
#define DQM_OL_HWM_SHIFT	DQM_HWM_SHIFT
#define DQM_OL_Q_SPACE_MASK	DQM_Q_SPACE_MASK
#define DQM_OL_Q_SPACE_SHIFT	DQM_Q_SPACE_SHIFT
#else
#define DQM_OL_NUM_TOK_MASK	0xfffffff0
#define DQM_OL_NUM_TOK_SHIFT	4
#define DQM_OL_LWM_MASK		0x0fffffff
#define DQM_OL_LWM_SHIFT	0
#define DQM_OL_HWM_MASK		0x0fffffff
#define DQM_OL_HWM_SHIFT	0
#define DQM_OL_Q_SPACE_MASK	0x0fffffff
#define DQM_OL_Q_SPACE_SHIFT	0
#endif
#define DQM_OL_DISABLE_MASK	0x00000008
#define DQM_OL_DISABLE_SHIFT	3
#define DQM_TMR_ENABLE_MASK	0x80000000
#define DQM_TMR_ENABLE_SHIFT	31
#define DQM_TMR_MODE_MASK	0x40000000
#define DQM_TMR_MODE_SHIFT	30
#define DQM_TMR_MODE_REPETITIVE	1
#define DQM_TMR_MODE_ONESHOT	0
#define DQM_TMR_CLK_SEL_MASK	0x20000000
#define DQM_TMR_CLK_SEL_SHIFT	29
#define DQM_TMR_CLK_SEL_924NS	0x20000000	/* 924ns clock  */
#define DQM_TMR_CLK_SEL_9NS	0x00000000	/* 9.24ns clock */
#define DQM_TMR_CLK_NS		924
#define DQM_TMR_COUNT_MASK	0x0000ffff
#define DQM_TMR_COUNT_SHIFT	0

#define DQM_SET_TOK_SIZE(q, siz) \
	(dqm_reg_write_mask(&((q)->ctl->size), DQM_TOK_SIZE_MASK, \
		(siz) << DQM_TOK_SIZE_SHIFT))
#define DQM_GET_TOK_SIZE(q) \
	((q)->dqmdev->restricted_access ? (q)->msg_size - 1 :		   \
	 ((dqm_reg_read(&((q)->ctl->size)) & DQM_TOK_SIZE_MASK) >> \
	  DQM_TOK_SIZE_SHIFT))
#define DQM_SET_Q_SIZE(q, size) \
	(dqm_reg_write_mask(&((q)->ctl->cfga), DQM_Q_SIZE_MASK, \
		(size) << DQM_Q_SIZE_SHIFT))
#define DQM_GET_Q_SIZE(q) \
	((q)->dqmdev->restricted_access ? ((q)->msg_size * (q)->depth) : \
	 ((dqm_reg_read(&((q)->ctl->cfga)) & DQM_Q_SIZE_MASK) >> \
	  DQM_Q_SIZE_SHIFT))
#define DQM_SET_Q_ADDR(q, addr) \
	(dqm_reg_write_mask(&((q)->ctl->cfga), DQM_Q_ADDR_MASK, \
		(addr) << DQM_Q_ADDR_SHIFT))
#define DQM_GET_Q_ADDR(q) \
	((q)->dqmdev->restricted_access ? (0) : \
	((dqm_reg_read(&((q)->ctl->cfga)) & DQM_Q_ADDR_MASK) >> \
		DQM_Q_ADDR_SHIFT))

#ifndef CONFIG_BCM_DQM_256M_OL_DEPTH
#define DQM_SET_NUM_TOK(q, num) \
	(dqm_reg_write_mask(&((q)->ctl->cfgb), DQM_NUM_TOK_MASK, \
		(num) << DQM_NUM_TOK_SHIFT))
#define DQM_GET_NUM_TOK(q) \
	((dqm_reg_read(&((q)->ctl->cfgb)) & DQM_NUM_TOK_MASK) >> \
		DQM_NUM_TOK_SHIFT)
#define DQM_SET_LWM(q, lwm) \
	(dqm_reg_write_mask(&((q)->ctl->cfgb), DQM_LWM_MASK, \
		(lwm) << DQM_LWM_SHIFT))
#define DQM_GET_LWM(q) \
	((dqm_reg_read(&((q)->ctl->cfgb)) & DQM_LWM_MASK) >> \
		DQM_LWM_SHIFT)
#define DQM_SET_HWM(q, hwm) \
	(dqm_reg_write_mask(&((q)->ctl->cfgc), DQM_HWM_MASK, \
		(hwm) << DQM_HWM_SHIFT))
#define DQM_GET_HWM(q) \
	((dqm_reg_read(&((q)->ctl->cfgc)) & DQM_HWM_MASK) >> \
		DQM_HWM_SHIFT)
#define DQM_GET_Q_SPACE(q) \
	((dqm_reg_read((q)->status) & DQM_Q_SPACE_MASK) >> \
		DQM_Q_SPACE_SHIFT)
#else
#define DQM_SET_NUM_TOK(q, num) \
	do { \
	if ((q)->offload) \
		(dqm_reg_write_mask(&((q)->ctl->size), DQM_OL_NUM_TOK_MASK, \
			(num) << DQM_OL_NUM_TOK_SHIFT)); \
	else \
		(dqm_reg_write_mask(&((q)->ctl->cfgb), DQM_NUM_TOK_MASK, \
			(num) << DQM_NUM_TOK_SHIFT)); \
	} while (0)
#define DQM_GET_NUM_TOK(q) \
	((q)->dqmdev->restricted_access ? (q)->depth :				\
	 ((q)->offload ?						\
	  ((dqm_reg_read(&((q)->ctl->size)) & DQM_OL_NUM_TOK_MASK) >>	\
	   DQM_OL_NUM_TOK_SHIFT) :					\
	  ((dqm_reg_read(&((q)->ctl->cfgb)) & DQM_NUM_TOK_MASK) >>	\
	   DQM_NUM_TOK_SHIFT)))
#define DQM_SET_LWM(q, lwm) \
	do { \
	if ((q)->offload) \
		(dqm_reg_write_mask(&((q)->ctl->cfgb), DQM_OL_LWM_MASK, \
			(lwm) << DQM_OL_LWM_SHIFT)); \
	else \
		(dqm_reg_write_mask(&((q)->ctl->cfgb), DQM_LWM_MASK, \
			(lwm) << DQM_LWM_SHIFT)); \
	} while (0)
#define DQM_GET_LWM(q) \
	((q)->offload ? \
	 ((dqm_reg_read(&((q)->ctl->cfgb)) & DQM_OL_LWM_MASK) >> \
		DQM_OL_LWM_SHIFT) : \
	 ((dqm_reg_read(&((q)->ctl->cfgb)) & DQM_LWM_MASK) >> \
		DQM_LWM_SHIFT))
#define DQM_SET_HWM(q, hwm) \
	do { \
	if ((q)->offload) \
		(dqm_reg_write_mask(&((q)->ctl->cfgc), DQM_OL_HWM_MASK, \
			(hwm) << DQM_OL_HWM_SHIFT)); \
	else \
		(dqm_reg_write_mask(&((q)->ctl->cfgc), DQM_HWM_MASK, \
			(hwm) << DQM_HWM_SHIFT)); \
	} while (0)
#define DQM_GET_HWM(q) \
	((q)->offload ? \
	 ((dqm_reg_read(&((q)->ctl->cfgc)) & DQM_OL_HWM_MASK) >> \
		DQM_OL_HWM_SHIFT) : \
	 ((dqm_reg_read(&((q)->ctl->cfgc)) & DQM_HWM_MASK) >> \
		DQM_HWM_SHIFT))
#define DQM_GET_Q_SPACE(q) \
	((q)->offload ? \
	 ((dqm_reg_read((q)->status) & DQM_OL_Q_SPACE_MASK) >> \
		DQM_OL_Q_SPACE_SHIFT) : \
	 ((dqm_reg_read((q)->status) & DQM_Q_SPACE_MASK) >> \
		DQM_Q_SPACE_SHIFT))
#endif

#define DQM_SET_OL_DISABLE(q, disable) \
	(dqm_reg_write_mask(&((q)->ctl->size), DQM_OL_DISABLE_MASK, \
		(disable) << DQM_OL_DISABLE_SHIFT))
#define DQM_GET_OL_DISABLE(q) \
	((dqm_reg_read(&((q)->ctl->size)) & DQM_OL_DISABLE_MASK) >> \
		DQM_OL_DISABLE_SHIFT)

#define DQM_ALLOC0_CHUNKS_MASK	0x00000003
#define DQM_ALLOC0_CHUNKS_SHIFT	0
#define DQM_ALLOC1_CHUNKS_MASK	0x0000000c
#define DQM_ALLOC1_CHUNKS_SHIFT	2
#define DQM_ALLOC2_CHUNKS_MASK	0x00000030
#define DQM_ALLOC2_CHUNKS_SHIFT	4
#define DQM_ALLOC3_CHUNKS_MASK	0x000000c0
#define DQM_ALLOC3_CHUNKS_SHIFT	6
#define DQM_ALLOC_CHUNKS_MASK(idx) \
	(((idx) == 0) ? DQM_ALLOC0_CHUNKS_MASK : \
	 ((idx) == 1) ? DQM_ALLOC1_CHUNKS_MASK : \
	 ((idx) == 2) ? DQM_ALLOC2_CHUNKS_MASK : \
	 DQM_ALLOC3_CHUNKS_MASK)
#define DQM_ALLOC_CHUNKS_SHIFT(idx) \
	(((idx) == 0) ? DQM_ALLOC0_CHUNKS_SHIFT : \
	 ((idx) == 1) ? DQM_ALLOC1_CHUNKS_SHIFT : \
	 ((idx) == 2) ? DQM_ALLOC2_CHUNKS_SHIFT : \
	 DQM_ALLOC3_CHUNKS_SHIFT)
#define DQM_ALLOC_8_CHUNKS	3
#define DQM_ALLOC_4_CHUNKS	2
#define DQM_ALLOC_2_CHUNKS	1
#define	DQM_ALLOC_1_CHUNKS	0

#define DQM_CHUNK_SIZE_MASK	0x00000001
#define DQM_CHUNK_SIZE_SHIFT	0

#define DQM_SET_TIMEOUT(q, us) \
	do { \
	u32 cnt; \
	dqm_reg_write_mask(&((q)->tmr->ctl), DQM_TMR_CLK_SEL_MASK, \
		DQM_TMR_CLK_SEL_924NS); \
	cnt = (us) * 1000 / DQM_TMR_CLK_NS; \
	if (cnt & ~DQM_TMR_COUNT_MASK) { \
		pr_err("DQM %d timeout exceeds HW capabilities.\n", (q)->num); \
		cnt = DQM_TMR_COUNT_MASK; \
	} \
	dqm_reg_write_mask(&((q)->tmr->ctl), DQM_TMR_COUNT_MASK, \
		cnt << DQM_TMR_COUNT_SHIFT); \
	} while (0)
#define DQM_GET_TIMEOUT(q) \
	(((dqm_reg_read(&((q)->tmr->ctl)) & DQM_TMR_COUNT_MASK) >> \
		DQM_TMR_COUNT_SHIFT) * DQM_TMR_CLK_NS / 1000)
#define DQM_SET_TMR_MODE(q, mode) \
	dqm_reg_write_mask(&((q)->tmr->ctl), DQM_TMR_MODE_MASK, \
		(mode) << DQM_TMR_MODE_SHIFT)
#define DQM_ENABLE_TMR(q, enable) \
	do { \
	if (enable) \
		dqm_reg_write_mask(&((q)->tmr->ctl), DQM_TMR_ENABLE_MASK, \
			1 << DQM_TMR_ENABLE_SHIFT); \
	else \
		dqm_reg_write_mask(&((q)->tmr->ctl), DQM_TMR_ENABLE_MASK, \
			0); \
	} while (0)
#define DQM_LIVE_IRQ_NE_MASK   0x1
#define DQM_LIVE_IRQ_NE_SHIFT  0
#define DQM_LIVE_IRQ_TMR_MASK  0x2
#define DQM_LIVE_IRQ_TMR_SHIFT 1
#define DQM_LIVE_IRQ_LWM_MASK  0x4
#define DQM_LIVE_IRQ_LWM_SHIFT 2
#define DQM_LIVE_IRQ_HWM_MASK  0x8
#define DQM_LIVE_IRQ_HWM_SHIFT 3
#define DQM_LIVE_IRQ_RX_MASK   (DQM_LIVE_IRQ_NE_MASK |\
				DQM_LIVE_IRQ_TMR_MASK |\
				DQM_LIVE_IRQ_HWM_MASK)
#define DQM_LIVE_IRQ_TX_MASK   (DQM_LIVE_IRQ_LWM_MASK)

#define DQM_EXT_IRQ_NE_MASK    0x2
#define DQM_EXT_IRQ_NE_SHIFT   1
#define DQM_EXT_IRQ_TMR_MASK   0x4
#define DQM_EXT_IRQ_TMR_SHIFT  2
#define DQM_EXT_IRQ_LWM_MASK   0x10
#define DQM_EXT_IRQ_LWM_SHIFT  4
#define DQM_EXT_IRQ_HWM_MASK   0x20
#define DQM_EXT_IRQ_HWM_SHIFT  5
#define DQM_EXT_IRQ_RX_MASK    (DQM_EXT_IRQ_NE_MASK |\
				DQM_EXT_IRQ_TMR_MASK |\
				DQM_EXT_IRQ_HWM_MASK)
#define DQM_EXT_IRQ_TX_MASK    (DQM_EXT_IRQ_LWM_MASK)
#endif
