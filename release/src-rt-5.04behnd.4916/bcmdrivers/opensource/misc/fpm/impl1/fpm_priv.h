 /****************************************************************************
 *
 * Broadcom Proprietary and Confidential.
 * (c) 2016 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Limited and/or its subsidiaries.
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
 * Author: Peter Sulc <petersu@broadcom.com>
 *	   Tim Ross <tross@broadcom.com>
 *****************************************************************************/
#ifndef _FPM_PRIV_H_
#define _FPM_PRIV_H_

#include <linux/types.h>
#include <linux/skbuff.h>
#include <uapi/linux/if_ether.h>
#include <linux/seq_file.h>
#include "fpm.h"

/* For debugging. */
#define FPM_CACHED 1
#define FPM_MAX_DDR_POOLS 3

#define MODULE_NAME	"brcm-fpm"
#define MODULE_VER	"2.0"

#define FPM_STATE_BIT_XOFF(pool)		(0 + (pool * 16))
#define FPM_STATE_BIT_XON(pool)			(1 + (pool * 16))
#define FPM_XON_THRESHOLD			(0x800)
#define FPM_XOFF_THRESHOLD			(0x400)
#define FPM_STATE_BIT_LWM_WD_ACTIVE(pool)	FPM_STATE_BIT_XOFF(pool)
#define FPM_WAIT_TIMEOUT_BR 10

/* Token operation */
/*
 * Pack to save memory and carefully order members so that they remain
 * aligned on their type-size boundaries for speed.
 */
struct __attribute__ ((packed)) fpm_tok_op {
	u32 token;		/* token */
	void *called;		/* driver function called */
	void *caller;		/* driver function caller */
	u32 ts;			/* timestamp of op (jiffies) */
	u32 op_data;		/* if_id/if_sub_id (pkt), header (RPC), ... */
	u8 ref_count;		/* ref count at time of op */
};

#define FPM_HISTORY_MEM_SIZE	(CONFIG_BCM_FPM_TOKEN_HIST_MEM * 1024)
#define FPM_NUM_HISTORY_ENTRIES \
	(FPM_HISTORY_MEM_SIZE / sizeof(struct fpm_tok_op))

/* Device struct */
struct fpmdev {
	struct platform_device *pdev;

	bool init;			/* init HW or not? */

	u32 *reg_pbase;			/* HW reg phys base addr */
	u32 *reg_vbase;			/* HW reg virt base addr */
	int irq;			/* HW IRQ # */
	int npools;     		/* # of pools supported by this chip */
	phys_addr_t pool_pbase[FPM_MAX_DDR_POOLS];	/* pool phys base addr */
	u8 *pool_vbase[FPM_MAX_DDR_POOLS];		/* pool virt base addr */
	u32 pool_size[FPM_MAX_DDR_POOLS];		/* pool size in bytes */
	u32 pool_ntokens[FPM_MAX_DDR_POOLS];		/* # of tokens supported by chip */
	u32 pool_alloc_weight[FPM_MAX_DDR_POOLS];	/* HW pool selection algorithm weight */
	u32 pool_free_weight[FPM_MAX_DDR_POOLS];	/* HW pool selection algorithm weight */

	/* map from requested buf size to alloc/free reg to use */
	u32 *buf_size_to_alloc_reg_map[8];
	u32 chunk_size;			/* minimum allocation size */
	bool pad_in_ctrl_spare;		/* net buf padding values stored in CTRL_SPARE reg*/
	u32 net_buf_head_pad;		/* network buffer head padding */
	u32 net_buf_tail_pad;		/* network buffer tail padding */
#ifdef CONFIG_BCM_FPM_TOKEN_FORMAT_VARIABLE
	u32 *chunk_to_alloc_reg_map[4];
	/* tok_idx_to_chunks_map is used to map buffers to tokens */
	u8  *tok_idx_to_chunks_map;
	u32 buf_sizes[4];		/* buf sizes for each alloc/free reg */
#endif

	/* tokens preallocated to work around FPM HW bug in chips < 3390B0 */
	u32 prealloc_tok[2][4];

	bool track_tokens;			/* track token ops */
	bool track_on_err;			/* continue tracking on error */
	s8 *tok_ref_count;			/* token ref count array */
	struct fpm_tok_op *tok_hist_start;	/* token history buffer start ptr */
	struct fpm_tok_op *tok_hist_end;	/* token history buffer end ptr */
	struct fpm_tok_op *tok_hist_head;	/* token history list head ptr */
	struct fpm_tok_op *tok_hist_tail;	/* token history list tail ptr */
	spinlock_t tok_hist_lock;		/* token history list lock */

	struct delayed_work pool_full_work;	/* used to re-enable FPM_POOL_FULL */
	int pool_full_idx;			/* pool index which was full */
	struct work_struct xon_work;		/* delayed work for notification chain */
	struct work_struct xoff_work;		/* delayed work for notification chain */
	unsigned long xon_xoff_state;
	u32 xon_cnt[FPM_MAX_DDR_POOLS];
	u32 xoff_cnt[FPM_MAX_DDR_POOLS];
	unsigned long xon_tstamp[FPM_MAX_DDR_POOLS];
	unsigned long xoff_tstamp[FPM_MAX_DDR_POOLS];

	u32 mem_corrupt_cnt[FPM_MAX_DDR_POOLS];
	struct delayed_work lwm_wd_work;	/* delayed work for LWM watchdog reset */
	unsigned long lwm_wd_state;
	u32 lwm_wd_timeout;			/* LWM watchdog reset timeout */
	bool lwm_wd_rs;			/* LWM watchdog reboot status */
};

extern struct fpmdev *fpm;

static inline u32 __fpm_max_tokens(void)
{
	int i;
	u32 tokens;
	for (i = 0, tokens = 0; i < fpm->npools; i++)
		tokens += fpm->pool_ntokens[i];
	return tokens;
}

#ifdef CONFIG_BCM_FPM_TOKEN_FORMAT_VARIABLE
static inline bool __fpm_is_token_valid(u32 token)
{
	return (token + 1) ? true : false;
}
static inline u32 __fpm_get_token_chunk(u32 token)
{
	return ((token & FPM_TOKEN_VARIABLE_FORMAT_CHUNK_MASK) >> FPM_TOKEN_VARIABLE_FORMAT_CHUNK_SHIFT);
}
static inline u32 __fpm_get_token_ddrpool(u32 token)
{
	if (fpm->npools==1)
		return 0;
	return ((token & FPM_TOKEN_VARIABLE_FORMAT_DDRPOOL_MASK) >> FPM_TOKEN_VARIABLE_FORMAT_DDRPOOL_SHIFT);
}
static inline u32 __fpm_get_token_index(u32 token)
{
	int chunk = __fpm_get_token_chunk(token);
	int index;
	if (fpm->npools>1) {
		/* Clear DDR bits */
		token &= ~FPM_TOKEN_VARIABLE_FORMAT_DDRPOOL_MASK;
	}
	if (fpm->chunk_size == 256)
		index = ((token & FPM_TOKEN_VARIABLE_FORMAT256_MAXINDEX_MASK) >> FPM_TOKEN_VARIABLE_FORMAT256_MAXINDEX_SHIFT);
	else
		index = ((token & FPM_TOKEN_VARIABLE_FORMAT512_MAXINDEX_MASK) >> FPM_TOKEN_VARIABLE_FORMAT512_MAXINDEX_SHIFT);
	index &= ~((1L<<(3-chunk))-1); /* Clear residual length bits based on chunk bits */
	return index; /* Aligned on chunk_size */
}
static inline u32 __fpm_get_token_size(u32 token)
{
	int chunk = __fpm_get_token_chunk(token);
	int size;
	if (fpm->chunk_size == 256)
		size = token & (FPM_TOKEN_VARIABLE_FORMAT256_MAXSIZE_MASK >> chunk);
	else
		size = token & (FPM_TOKEN_VARIABLE_FORMAT512_MAXSIZE_MASK >> chunk);
	return size;
}
static inline u32 __fpm_token_max_index_bits(void)
{
	if (fpm->chunk_size == 256)
		return (FPM_TOKEN_VARIABLE_FORMAT256_MAXINDEX_BITS);
	else
		return (FPM_TOKEN_VARIABLE_FORMAT512_MAXINDEX_BITS);
}
static inline int __fpm_set_token_size(u32 *token, u32 size)
{
	int chunk = __fpm_get_token_chunk(*token);
	if (fpm->chunk_size == 256) {
		*token &= ~(FPM_TOKEN_VARIABLE_FORMAT256_MAXSIZE_MASK >> chunk);
		size  &= (FPM_TOKEN_VARIABLE_FORMAT256_MAXSIZE_MASK >> chunk);
	} else {
		*token &= ~(FPM_TOKEN_VARIABLE_FORMAT512_MAXSIZE_MASK >> chunk);
		size  &= (FPM_TOKEN_VARIABLE_FORMAT512_MAXSIZE_MASK >> chunk);
	}
	*token |= size;
	return 0;
}
static inline int __fpm_set_token_index(u32 *token, u32 idx)
{
	if (fpm->chunk_size == 256) {
		*token &= ~(FPM_TOKEN_VARIABLE_FORMAT256_MAXINDEX_MASK);
		*token |= ((idx << FPM_TOKEN_VARIABLE_FORMAT256_MAXINDEX_SHIFT)
			  & FPM_TOKEN_VARIABLE_FORMAT256_MAXINDEX_MASK);
	} else {
		*token &= ~(FPM_TOKEN_VARIABLE_FORMAT512_MAXINDEX_MASK);
		*token |= ((idx << FPM_TOKEN_VARIABLE_FORMAT512_MAXINDEX_SHIFT)
			  & FPM_TOKEN_VARIABLE_FORMAT512_MAXINDEX_MASK);
	}
	return 0;
}
#else
static inline bool __fpm_is_token_valid(u32 token)
{
	if (token & FPM_TOKEN_FIXED_FORMAT_VALID_MASK)
		return true;
	else
		return false;
}
static inline u32 __fpm_get_token_ddrpool(u32 token)
{
	return ((token & FPM_TOKEN_FIXED_FORMAT_POOL_MASK) >> FPM_TOKEN_FIXED_FORMAT_POOL_SHIFT);
}
static inline u32 __fpm_get_token_index(u32 token)
{
	return ((token & FPM_TOKEN_FIXED_FORMAT_INDEX_MASK) >> FPM_TOKEN_FIXED_FORMAT_INDEX_SHIFT);
}
static inline u32 __fpm_get_token_size(u32 token)
{
	return ((token & FPM_TOKEN_FIXED_FORMAT_SIZE_MASK) >> FPM_TOKEN_FIXED_FORMAT_SIZE_SHIFT);
}
static inline u32 __fpm_token_max_index_bits(void)
{
	return (FPM_POOL_FIXED_FORMAT_MAXINDEX_BITS);
}
static inline int __fpm_set_token_size(u32 *token, u32 size)
{
	*token &= ~FPM_TOKEN_FIXED_FORMAT_SIZE_MASK;
	*token |= (size & FPM_TOKEN_FIXED_FORMAT_SIZE_MASK);
	return 0;
}
static inline int __fpm_set_token_index(u32 *token, u32 idx)
{
	*token &= ~FPM_TOKEN_FIXED_FORMAT_INDEX_MASK;
	*token |= ((idx << FPM_TOKEN_FIXED_FORMAT_INDEX_SHIFT)
	           & FPM_TOKEN_FIXED_FORMAT_INDEX_MASK);
	return 0;
}
#endif

#ifdef CONFIG_BCM_FPM_TOKEN_CHECKING
static inline void fpm_check_token(u32 token)
{
	int status = 0;

	if (!__fpm_is_token_valid(token)) {
		pr_err("Token 0x%08x not valid.\n", token);
		status = -EINVAL;
	}
	if (unlikely(!fpm->pool_pbase[__fpm_get_token_ddrpool(token)])) {
		pr_err("Token 0x%08x from disabled pool.\n", token);
		status = -EINVAL;
	}
	if (unlikely(__fpm_get_token_index(token) * fpm->chunk_size >
	    fpm->pool_size[__fpm_get_token_ddrpool(token)])) {
		pr_err("Token 0x%08x index out of range.\n", token);
		pr_err("chunk_size: %d, pool: %d, pool_size: %d\n", fpm->chunk_size, __fpm_get_token_ddrpool(token), fpm->pool_size[__fpm_get_token_ddrpool(token)]);
		status = -EINVAL;
	}
	if (unlikely(__fpm_get_token_size(token) > fpm->chunk_size * 8)) {
		pr_err("Token 0x%08x size greater than max allowed.\n", token);
		status = -EINVAL;
	}
	if (status)
		dump_stack();
}
#else
#define fpm_check_token(token)
#endif

static inline u32 __fpm_alloc_token(int size)
{
#ifdef CONFIG_BCM_FPM_TOKEN_HIST
	struct fpmdev *fdev = fpm;
	u32 tok_idx;
#endif
	u32 token;
	int idx = (size - 1) / (fpm->chunk_size);

	if (size == 0)
		idx = 0;
	if (unlikely(idx > 7)) {
		pr_err("%s Invalid size %d is too big.\n",
		       __func__, size);
		dump_stack();
		return 0;
	}

	token = fpm_reg_read(fpm->buf_size_to_alloc_reg_map[idx]);
	if (__fpm_is_token_valid(token)) {
#ifdef CONFIG_BCM_FPM_TOKEN_HIST
		if (fdev->track_tokens) {
			tok_idx = __fpm_get_token_index(token);
			tok_idx |= (__fpm_get_token_ddrpool(token) <<
				    __fpm_token_max_index_bits());
#ifdef CONFIG_BCM_FPM_TOKEN_HIST_CHECKING
			if (fdev->tok_ref_count[tok_idx]) {
				pr_err("Allocation of token (0x%08x) ", token);
				pr_cont("with non-zero ref count (%d).\n",
					fdev->tok_ref_count[tok_idx]);
				fdev->track_tokens = fdev->track_on_err;
			}
#endif
			fdev->tok_ref_count[tok_idx]++;
		}
#endif
		return token;
	}

	return 0;
}

static inline void __fpm_free_token(u32 token)
{
#ifdef CONFIG_BCM_FPM_TOKEN_HIST
	struct fpmdev *fdev = fpm;
	u32 tok_idx;
#endif
#ifdef CONFIG_BCM_FPM_TOKEN_FORMAT_VARIABLE
	int chunk = __fpm_get_token_chunk(token);
	fpm_reg_write(fpm->chunk_to_alloc_reg_map[chunk], token);
#else
	fpm_reg_write(FPM_DEALLOC, token);
#endif
#ifdef CONFIG_BCM_FPM_TOKEN_HIST
	if (fdev->track_tokens) {
		tok_idx = __fpm_get_token_index(token);
		tok_idx |= (__fpm_get_token_ddrpool(token) <<
			    __fpm_token_max_index_bits());
#ifdef CONFIG_BCM_FPM_TOKEN_HIST_CHECKING
		if (fdev->tok_ref_count[tok_idx] <= 0) {
			pr_err("Free of token (0x%08x) with zero ", token);
			pr_cont("or negative ref count (%d).\n",
				fdev->tok_ref_count[tok_idx]);
			fdev->track_tokens = fdev->track_on_err;
		}
#endif
		fdev->tok_ref_count[tok_idx]--;
	}
#endif
}

static inline u8 *__fpm_token_to_buffer(u32 token)
{
	u8 *buf;

	buf = fpm->pool_vbase[__fpm_get_token_ddrpool(token)] +
		__fpm_get_token_index(token) * fpm->chunk_size;
#ifdef CONFIG_BCM_FPM_TOKEN_FORMAT_VARIABLE
	/* need this for buffer to token conversion */
	fpm->tok_idx_to_chunks_map[__fpm_get_token_index(token)] =
		__fpm_get_token_chunk(token);
#endif
	return buf;
}

static inline int fpm_buffer_to_pool(u8 *buf)
{
	int i;
	for (i = 0; i < fpm->npools; i++) {
		if (buf >= fpm->pool_vbase[i] &&
		    buf < fpm->pool_vbase[i] + fpm->pool_size[i])
			return i;
	}
	return -1;
}

#ifdef CONFIG_BCM_FPM_TOKEN_FORMAT_VARIABLE
static inline u32 __fpm_buffer_to_token(u8 *buf, u32 size)
{
	int ddrpool;
	u32 token_idx;
	u32 token;
	u32 chunks;

	/*
	 * This will calc the index of the chunk_size chunk <= and nearest
	 * to buf. If buf is not within the 1st chunk of the original group
	 * of chunks allocated then the index will not be correct.
	 */
	ddrpool = fpm_buffer_to_pool(buf);
	if (ddrpool < 0) {
		pr_err("%p is not an FPM buffer ptr.\n", buf);
		dump_stack();
		return 0;
	}
	token_idx	= (buf - fpm->pool_vbase[ddrpool]) / fpm->chunk_size;
	chunks		= fpm->tok_idx_to_chunks_map[token_idx];

	if (fpm->chunk_size == 256) {
		token		= (token_idx << FPM_TOKEN_VARIABLE_FORMAT256_MAXINDEX_SHIFT);
		/* Clear Length bits */
		token          &= ~(FPM_TOKEN_VARIABLE_FORMAT256_MAXSIZE_MASK >> chunks);
	} else {
		token		= (token_idx << FPM_TOKEN_VARIABLE_FORMAT512_MAXINDEX_SHIFT);
		/* Clear Length bits */
		token          &= ~(FPM_TOKEN_VARIABLE_FORMAT512_MAXSIZE_MASK >> chunks);
	}
	token          |= (size);
	token          |= (ddrpool << FPM_TOKEN_VARIABLE_FORMAT_DDRPOOL_SHIFT) &
			   FPM_TOKEN_VARIABLE_FORMAT_DDRPOOL_MASK;
	return token;
}
#else
static inline u32 __fpm_buffer_to_token(u8 *buf, u32 size)
{
	int pool;
	u32 token_idx;
	u32 token;

	/*
	 * This will calc the index of the chunk_size chunk <= and nearest
	 * to buf. If buf is not within the 1st chunk of the original group
	 * of chunks allocated then the index will not be correct.
	 */
	pool = fpm_buffer_to_pool(buf);
	if (pool < 0) {
		pr_err("%p is not an FPM buffer ptr.\n", buf);
		dump_stack();
		return 0;
	}
	token_idx	= (buf - fpm->pool_vbase[pool]) / fpm->chunk_size;
	token		= FPM_TOKEN_FIXED_FORMAT_VALID_MASK |
			  (token_idx << FPM_TOKEN_FIXED_FORMAT_INDEX_SHIFT) |
			  (size & FPM_TOKEN_FIXED_FORMAT_SIZE_MASK);
#if defined(FPM_TOKEN_POOL_MASK)
	token		|= (pool << FPM_TOKEN_FIXED_FORMAT_POOL_SHIFT) &
			   FPM_TOKEN_FIXED_FORMAT_POOL_MASK;
#endif
	return token;
}
#endif

#ifdef CONFIG_BCM_FPM_TOKEN_FORMAT_VARIABLE
/* Without pool ID bits we have no way to get size of entire token buffer. */
static inline u32 fpm_get_buffer_size(u32 token)
{
	return fpm->buf_sizes[__fpm_get_token_chunk(token)];
}
#endif

static inline void fpm_get_sync_start_size(u32 token, u32 head, u32 tail,
					   u32 flags, u8 **start, u32 *size)
{
	*start = __fpm_token_to_buffer(token) + head;
#ifdef CONFIG_BCM_FPM_TOKEN_FORMAT_VARIABLE
	/*
	 * Without pool ID bits we have no way to get size of entire token
	 * buffer, and thus no way to sync the tail.
	 */
	*size = fpm_get_buffer_size(token) - head - tail;
#else
	*size = __fpm_get_token_size(token);
	if (!*size) {
		pr_err("%s: sync with 0 data bytes (head: %d, tail: %d)\n",
		       __func__, head, tail);
		dump_stack();
	}
#endif
	if (flags & FPM_SYNC_HEAD) {
		*start -= head;
		*size += head;
	}
	if (flags & FPM_SYNC_TAIL) {
		*size += tail;
	}
}

#ifdef CONFIG_BCM_FPM_TOKEN_HIST
static inline void fpm_track_token_op(void *called, u32 token, u32 op_data)
{
	struct fpmdev *fdev = fpm;
	struct fpm_tok_op *op = fdev->tok_hist_head;
	unsigned long flags;
	u32 tok_idx;

	pr_debug("-->\n");

	if (!fdev->track_tokens)
		return;

	op->token = token;
	op->called = called;
	op->caller = __builtin_return_address(0);
	op->ts = jiffies;
	op->op_data = op_data;
	tok_idx = __fpm_get_token_index(token);
	tok_idx |= (__fpm_get_token_ddrpool(token) << __fpm_token_max_index_bits());
	op->ref_count = fdev->tok_ref_count[tok_idx];

	spin_lock_irqsave(&fdev->tok_hist_lock, flags);
	fdev->tok_hist_head++;
	if (fdev->tok_hist_head >= fdev->tok_hist_end)
		fdev->tok_hist_head = fdev->tok_hist_start;
	if (fdev->tok_hist_head == fdev->tok_hist_tail)
		fdev->tok_hist_tail++;
	if (fdev->tok_hist_tail >= fdev->tok_hist_end)
		fdev->tok_hist_tail = fdev->tok_hist_start;
	spin_unlock_irqrestore(&fdev->tok_hist_lock, flags);

	pr_debug("<--\n");
}

static inline void fpm_dump_hist_entry(struct fpm_tok_op *op,
				       struct seq_file *seq)
{
	if (seq) {
		seq_printf(seq, "token: 0x%08x\tref count: %d\n", op->token,
			   op->ref_count);
		seq_printf(seq, "op: %pf\tcalled by: %pf\n", op->called,
			   op->caller);
		seq_printf(seq, "timestamp (jiffies): 0x%08x\n", op->ts);
		seq_printf(seq, "op data: 0x%08x\n", op->op_data);
		seq_printf(seq, "------------------------------------------\n");
	} else {
		pr_info("token: 0x%08x\tref count: %d\n", op->token,
			   op->ref_count);
		pr_info("op: %pf\tcalled by: %pf\n", op->called,
			   op->caller);
		pr_info("timestamp (jiffies): 0x%08x\n", op->ts);
		pr_info("op data: 0x%08x\n", op->op_data);
		pr_info("------------------------------------------\n");
	}
}
#else
#define fpm_track_token_op(called, token, op_data)
#define fpm_dump_hist_entry(op, seq)
#endif

#endif
