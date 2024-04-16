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
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <asm/cacheflush.h>
#include <msgfifo.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/rtnetlink.h>
#include <linux/reboot.h>

#include <bcmcache.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/bug.h>
#include "fpm.h"
#include "fpm_dev.h"
#include "fpm_priv.h"
#include "fpm_dt.h"
#include "fpm_proc.h"

/*
 * Definitions & Clarifications
 *
 * The FPM hardware register definitions seem to overload the meaning of
 * "pool" to the point of complete confusion. "Pool" is used to refer to:
 * a) all of the memory allotted to the FPM hardware, b) a portion of that
 * memory used to allocate buffers of a particular size (256, 512, 1024,
 * 2048, 4096) even though buffers of a particular size may be located
 * anywhere within the FPM memory, and c) beginning with the 3390, the
 * portion of the memory allotted to the FPM that is within a particular
 * DDR bank, even though the memory may actually be in the same DDR bank.
 * In an attempt to eliminate this confusion I have deviated from the
 * hardware definitions of "pool" and use the following terms throughout
 * the code instead:
 *
 * pool:
 * A contiguous area of DDR from which the FPM hardware may allocate.
 * Chips prior to the 3390 had 1 pool, while the 3390 introduced 2 pools
 * which may be in the same or different DDR hardware interfaces.
 *
 * chunk:
 * The smallest allocation size available from the pool. Currently this
 * can be either 256 or 512 bytes.
 *
 * buffer:
 * Memory is allocated by the FPM as one or more contiguous chunks within
 * a given pool, hereafter called a buffer. Currently a buffer consists
 * of 2^n chunks where n = 0, 1, 2, or 3. With the introduction of multiple
 * pools the FPM hardware allows allocations to be made from a specific pool
 * or from any pool. If a pool is not specified then an internal algorithm
 * is used to pick the pool.
 *
 * token size:
 * Confusion also seems to arise when referring to the size field in the
 * token. While the size of a buffer is always a multiple of chunks, the
 * token size field may or may not be the full size of the buffer. When the
 * buffer is first allocated the token size field will equal the size of the
 * buffer (NOTE: There is a bug in all chips prior to the 3390B0 that results
 * in the size field of the token returned upon allocation to indicate the
 * size of the buffer as a multiple of 256 byte chunks regardless of whether
 * the FPM has been configured for a 256 or 512 byte chunk size). When
 * interfacing with hardware the token size field indicates the size of
 * the data in the buffer.
 *
 */

#define MAX_NO_OF_ONLINE_CPU num_online_cpus()
#define WDMD_TASKS_FILE "/var/run/wdmd/wdmd_tasks"
/* Global lock for all register accesses. */
spinlock_t fpm_reg_lock;
struct fpmdev *fpm = NULL;
/* Global lock for lockup to initiate watchdog reset */
static spinlock_t fpm_lockup;
static struct task_struct *fpm_lockup_thread[8];

static ATOMIC_NOTIFIER_HEAD(fpm_chain);

u32 fpm_alloc_token(int size)
{
	u32 token;

	pr_debug("-->\n");

	token = __fpm_alloc_token(size);
	fpm_check_token(token);
	fpm_track_token_op(fpm_alloc_token, token, 0);
	pr_debug("%s(0x%08x)<-%pf\n", __func__, token,
		 __builtin_return_address(0));

	pr_debug("<--\n");
	return token;
}
EXPORT_SYMBOL(fpm_alloc_token);

u8 *fpm_alloc_buffer(int size)
{
	u8 *buf = NULL;
	u32 token;

	pr_debug("-->\n");
	token = __fpm_alloc_token(size);
	fpm_check_token(token);
	fpm_track_token_op(fpm_alloc_buffer, token, 0);
	pr_debug("%s(0x%08x)<-%pf\n", __func__, token,
		 __builtin_return_address(0));
	if (token)
		buf = __fpm_token_to_buffer(token);

	pr_debug("<--\n");
	return buf;
}
EXPORT_SYMBOL(fpm_alloc_buffer);

u32 fpm_incr_multicount(u32 token)
{
#ifdef CONFIG_BCM_FPM_TOKEN_HIST
	struct fpmdev *fdev = fpm;
	u32 tok_idx;
#endif
	int incr = 0;
	u32 fpm_local_multi;

	pr_debug("-->\n");

	fpm_check_token(token);
	fpm_track_token_op(fpm_incr_multicount, token, 0);
	pr_debug("%s(0x%08x)<-%pf\n", __func__, token,
		 __builtin_return_address(0));
	fpm_local_multi = (token & ~FPM_TOKEN_MULTIVAL_MASK);
	fpm_local_multi |= (1 << FPM_TOKEN_MULTIINCR_SHIFT);
	fpm_local_multi++;

	if (__fpm_is_token_valid(fpm_local_multi)) {
		fpm_reg_write(FPM_POOL_MULTI, fpm_local_multi);
		incr = 1;
	}

#ifdef CONFIG_BCM_FPM_TOKEN_HIST
	if (fdev->track_tokens) {
		tok_idx = __fpm_get_token_index(token);
		tok_idx |= (__fpm_get_token_ddrpool(token) <<
			    __fpm_token_max_index_bits());
#ifdef CONFIG_BCM_FPM_TOKEN_HIST_CHECKING
		if (!fdev->tok_ref_count[tok_idx]) {
			pr_err("Multicount increment of token (0x%08x)", token);
			pr_cont(" with zero ref count (%d).\n",
				fdev->tok_ref_count[tok_idx]);
			fdev->track_tokens = fdev->track_on_err;
		}
#endif
		fdev->tok_ref_count[tok_idx] += incr;
	}
#endif

	pr_debug("<--\n");
	return incr;
}
EXPORT_SYMBOL(fpm_incr_multicount);

void fpm_free_token(u32 token)
{
	pr_debug("-->\n");

	fpm_check_token(token);
	fpm_track_token_op(fpm_free_token, token, 0);
	pr_debug("%s(0x%08x)<-%pf\n", __func__, token,
		 __builtin_return_address(0));
	__fpm_free_token(token);

	pr_debug("<--\n");
}
EXPORT_SYMBOL(fpm_free_token);

void fpm_free_buffer(u8 *buf)
{
	u32 token;

	pr_debug("-->\n");

	token = __fpm_buffer_to_token(buf, 0);
	fpm_check_token(token);
	fpm_track_token_op(fpm_free_buffer, token, 0);
	pr_debug("%s(0x%08x)<-%pf\n", __func__, token,
		 __builtin_return_address(0));
	__fpm_free_token(token);

	pr_debug("<--\n");
}
EXPORT_SYMBOL(fpm_free_buffer);

u8 *fpm_token_to_buffer(u32 token)
{
	pr_debug("-->\n");

	fpm_check_token(token);
	fpm_track_token_op(fpm_token_to_buffer, token, 0);
	pr_debug("%s(0x%08x)<-%pf\n", __func__, token,
		 __builtin_return_address(0));

	pr_debug("<--\n");
	return __fpm_token_to_buffer(token);
}
EXPORT_SYMBOL(fpm_token_to_buffer);

u32 fpm_buffer_to_token(u8 *buf, u32 size)
{
	u32 token;

	pr_debug("-->\n");

	token = __fpm_buffer_to_token(buf, size);
	fpm_check_token(token);
	fpm_track_token_op(fpm_buffer_to_token, token, 0);
	pr_debug("%s(0x%08x)<-%pf\n", __func__, token,
		 __builtin_return_address(0));

	pr_debug("<--\n");
	return token;
}
EXPORT_SYMBOL(fpm_buffer_to_token);

u32 fpm_get_token_size(u32 token)
{
	pr_debug("-->\n");

	fpm_check_token(token);
	fpm_track_token_op(fpm_get_token_size, token, 0);
	pr_debug("%s(0x%08x)<-%pf\n", __func__, token,
		 __builtin_return_address(0));

	pr_debug("<--\n");
	return __fpm_get_token_size(token);
}
EXPORT_SYMBOL(fpm_get_token_size);

int fpm_set_token_size(u32 *token, u32 size)
{
	int status = 0;

	pr_debug("-->\n");

	if (!token) {
		pr_err("%s: NULL token ptr\n", __func__);
		dump_stack();
		status = -EINVAL;
		goto done;
	}
	fpm_check_token(*token);
	fpm_track_token_op(fpm_set_token_size, *token, 0);
	__fpm_set_token_size(token, size);
	pr_debug("%s(0x%08x)<-%pf\n", __func__, *token,
		 __builtin_return_address(0));

done:
	pr_debug("<--\n");
	return status;
}
EXPORT_SYMBOL(fpm_set_token_size);

int fpm_set_token_index(u32 *token, u32 idx)
{
	__fpm_set_token_index(token, idx);
	return 0;
}
EXPORT_SYMBOL(fpm_set_token_index);

u32 fpm_get_token_index(u32 token)
{
	return __fpm_get_token_index(token);
}
EXPORT_SYMBOL(fpm_get_token_index);

bool fpm_is_valid_token(u32 token)
{
	return (__fpm_is_token_valid(token));
}
EXPORT_SYMBOL(fpm_is_valid_token);

int fpm_get_min_buffer_size(int size)
{
	int chunks = (size - 1) / fpm->chunk_size;
	int min_size = (chunks + 1) * fpm->chunk_size;
	if (chunks < 0 || chunks > 7)
		min_size = -ERANGE;
	return min_size;
}
EXPORT_SYMBOL(fpm_get_min_buffer_size);

void fpm_invalidate_token(u32 token, u32 head, u32 tail, u32 flags)
{
	u8 *start;
	u32 size;

	pr_debug("-->\n");

	fpm_check_token(token);
	fpm_track_token_op(fpm_invalidate_token, token, 0);
	pr_debug("%s(0x%08x)<-%pf\n", __func__, token,
		 __builtin_return_address(0));
	fpm_get_sync_start_size(token, head, tail, flags, &start, &size);
	/* dma_addr_t paddr = fpm->pool_pbase + ((u32)buf - fpm->pool_vbase); */
	/* dma_sync_single_for_cpu(NULL, paddr, size, DMA_FROM_DEVICE);	      */
	cache_invalidate_buffer((u8 *)start, size);

	pr_debug("<--\n");
}
EXPORT_SYMBOL(fpm_invalidate_token);

void fpm_flush_invalidate_token(u32 token, u32 head, u32 tail, u32 flags)
{
	u8 *start;
	u32 size;

	pr_debug("-->\n");

	fpm_check_token(token);
	fpm_track_token_op(fpm_flush_invalidate_token, token, 0);
	pr_debug("%s(0x%08x)<-%pf\n", __func__, token,
		 __builtin_return_address(0));
	fpm_get_sync_start_size(token, head, tail, flags, &start, &size);
	/* dma_addr_t paddr = fpm->pool_pbase + ((u32)buf - fpm->pool_vbase); */
	/* dma_sync_single_for_device(NULL, paddr, size, DMA_TO_DEVICE);      */
	cache_flush_invalidate_buffer((u8 *)start, size);

	pr_debug("<--\n");
}
EXPORT_SYMBOL(fpm_flush_invalidate_token);

void fpm_flush_invalidate_buffer(void *start, u32 size)
{
	pr_debug("-->\n");

	if (!fpm_is_fpm_buf(start)) {
		pr_err("%s called on non-FPM buffer %p!\n", __func__,
		       start);
		pr_err("%s should only be called on FPM buffers!\n", __func__);
		dump_stack();
	}
#if defined(CONFIG_ARM)
	dmac_flush_range(start, start + size);
#elif defined(CONFIG_ARM64)
#elif defined(CONFIG_MIPS)
	dma_cache_wback_inv((u32)start, size);
#else
#error No arch-specific DMA invalidate specified.
#endif

	pr_debug("<--\n");
}
EXPORT_SYMBOL(fpm_flush_invalidate_buffer);

void fpm_invalidate_buffer(void *start, u32 size)
{
	pr_debug("-->\n");

	if (!fpm_is_fpm_buf(start)) {
		pr_err("%s called on non-FPM buffer %p!\n", __func__,
		       start);
		pr_err("%s should only be called on FPM buffers!\n", __func__);
		dump_stack();
	}
#if defined(CONFIG_ARM)
	dmac_unmap_area(start, size, DMA_FROM_DEVICE);
#elif defined(CONFIG_ARM64)
#elif defined(CONFIG_MIPS)
	dma_cache_inv((u32)start, size);
#else
#error No arch-specific DMA invalidate specified.
#endif

	pr_debug("<--\n");
}
EXPORT_SYMBOL(fpm_invalidate_buffer);

int fpm_get_hw_info(struct fpm_hw_info *hw_info)
{
	pr_debug("-->\n");

	if (!fpm) {
		pr_err("Free Pool Manager is uninitialized\n");
		return -ENXIO;
	}

	hw_info->pool_base[0] = fpm->pool_pbase[0];
	hw_info->pool_base[1] = fpm->pool_pbase[1];
	hw_info->alloc_dealloc[0] = (uintptr_t)FPM_ALLOC_8_CHUNKS_PHYS;
	hw_info->alloc_dealloc[1] = (uintptr_t)FPM_ALLOC_4_CHUNKS_PHYS;
	hw_info->alloc_dealloc[2] = (uintptr_t)FPM_ALLOC_2_CHUNKS_PHYS;
	hw_info->alloc_dealloc[3] = (uintptr_t)FPM_ALLOC_1_CHUNKS_PHYS;
	hw_info->chunk_size = fpm->chunk_size;
	hw_info->net_buf_head_pad = fpm->net_buf_head_pad;
	hw_info->net_buf_tail_pad = fpm->net_buf_tail_pad;

	pr_debug("<--\n");

	return 0;
}
EXPORT_SYMBOL(fpm_get_hw_info);

int fpm_is_fpm_buf(void *buf)
{
	int is_fpm = 1;

	pr_debug("-->\n");

	if (fpm_buffer_to_pool(buf) < 0)
		is_fpm = 0;

	pr_debug("<--\n");
	return is_fpm;
}
EXPORT_SYMBOL(fpm_is_fpm_buf);

u32 fpm_token_pool(u32 token)
{
	u32 tok_idx;

	pr_debug("-->\n");
	tok_idx = __fpm_get_token_ddrpool(token);
	pr_debug("<--\n");
	return tok_idx;
}
EXPORT_SYMBOL(fpm_token_pool);

int fpm_get_pool_stats(int pool, struct fpm_pool_stats *stats)
{
	pr_debug("-->\n");

	stats->underflow_count		= FPM_GET_UNDERFLOW(pool);
	stats->overflow_count		= FPM_GET_OVERFLOW(pool);
	stats->tok_avail		= FPM_GET_TOK_AVAIL(pool);
	stats->alloc_fifo_empty		= FPM_GET_ALLOC_FIFO_EMPTY(pool);
	stats->alloc_fifo_full		= FPM_GET_ALLOC_FIFO_FULL(pool);
	stats->free_fifo_empty		= FPM_GET_FREE_FIFO_EMPTY(pool);
	stats->free_fifo_full		= FPM_GET_FREE_FIFO_FULL(pool);
	stats->pool_full		= FPM_GET_POOL_FULL(pool);
	stats->invalid_tok_frees	= FPM_GET_INVAL_TOK_FREES(pool);
	stats->invalid_tok_multi	= FPM_GET_INVAL_TOK_MULTI(pool);
	stats->mem_corrupt_tok		= FPM_GET_MEM_CORRUPT_TOK(pool);
	stats->mem_corrupt_tok_valid	=
		FPM_GET_MEM_CORRUPT_TOK_VALID(pool) == 1;
	stats->invalid_free_tok		= FPM_GET_INVALID_FREE_TOK(pool);
	stats->invalid_free_tok_valid	=
		FPM_GET_INVALID_FREE_TOK_VALID(pool) == 1;
	stats->invalid_mcast_tok	= FPM_GET_INVALID_MCAST_TOK(pool);
	stats->invalid_mcast_tok_valid	=
		FPM_GET_INVALID_MCAST_TOK_VALID(pool) == 1;

	pr_debug("<--\n");
	return 0;
}
EXPORT_SYMBOL(fpm_get_pool_stats);

u32 fpm_get_tok_avail(int pool)
{
	return FPM_GET_TOK_AVAIL(pool);
}
EXPORT_SYMBOL(fpm_get_tok_avail);

void fpm_reset_bb(bool reset)
{
	pr_debug("-->\n");

	FPM_SET_BB_RESET(reset ? 1 : 0);

	pr_debug("<--\n");
}
EXPORT_SYMBOL(fpm_reset_bb);

phys_addr_t fpm_buf_virt_to_phys(void *buf)
{
	u8 *vaddr = buf;
	phys_addr_t offset = vaddr - fpm->pool_vbase[0];
	if (offset < fpm->pool_size[0])
		return (phys_addr_t)(fpm->pool_pbase[0] + offset);
	if (fpm->pool_vbase[1]) {
		offset = vaddr - fpm->pool_vbase[1];
		if (offset < fpm->pool_size[1])
			return (phys_addr_t)(fpm->pool_pbase[1] + offset);
	}
	return -1;
}
EXPORT_SYMBOL(fpm_buf_virt_to_phys);

void *fpm_buf_phys_to_virt(phys_addr_t paddr)
{
	phys_addr_t offset = paddr - fpm->pool_pbase[0];
	if (offset < fpm->pool_size[0])
		return (void *)(fpm->pool_vbase[0] + offset);
	if (fpm->pool_vbase[1]) {
		offset = paddr - fpm->pool_pbase[1];
		if (offset < fpm->pool_size[1])
			return (void *)(fpm->pool_vbase[1] + offset);
	}
	return (void *)-1;
}
EXPORT_SYMBOL(fpm_buf_phys_to_virt);

int register_fpm_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&fpm_chain, nb);
}
EXPORT_SYMBOL(register_fpm_notifier);

int unregister_fpm_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&fpm_chain, nb);
}
EXPORT_SYMBOL(unregister_fpm_notifier);

static void fpm_pool_full_work_handler(struct work_struct *work)
{
	struct fpmdev *fdev = container_of(work, struct fpmdev,
					   pool_full_work.work);
	int *pool_full_idx = &fdev->pool_full_idx;

	if (pool_full_idx)
		FPM_SET_IRQ_MASK(*pool_full_idx,
			FPM_GET_IRQ_MASK(*pool_full_idx) | FPM_POOL_FULL_MASK);
}

static void fpm_xon_work_handler(struct work_struct *work)
{
	struct fpmdev *fdev = container_of(work, struct fpmdev,
					   xon_work);
	struct fpm_notifier_info info;
	if (test_and_clear_bit(FPM_STATE_BIT_XON(0), &fdev->xon_xoff_state)) {
		info.pool = 0;
		info.event = FPM_EVENT_XON;
		atomic_notifier_call_chain(&fpm_chain, FPM_EVENT_XON, &info);
	}
	if (test_and_clear_bit(FPM_STATE_BIT_XON(1), &fdev->xon_xoff_state)) {
		info.pool = 1;
		info.event = FPM_EVENT_XON;
		atomic_notifier_call_chain(&fpm_chain, FPM_EVENT_XON, &info);
	}
}

static void fpm_xoff_work_handler(struct work_struct *work)
{
	struct fpmdev *fdev = container_of(work, struct fpmdev,
					   xoff_work);
	struct fpm_notifier_info info;
	if (test_and_clear_bit(FPM_STATE_BIT_XOFF(0), &fdev->xon_xoff_state)) {
		info.pool = 0;
		info.event = FPM_EVENT_XOFF;
		atomic_notifier_call_chain(&fpm_chain, FPM_EVENT_XOFF, &info);
	}
	if (test_and_clear_bit(FPM_STATE_BIT_XOFF(1), &fdev->xon_xoff_state)) {
		info.pool = 1;
		info.event = FPM_EVENT_XOFF;
		atomic_notifier_call_chain(&fpm_chain, FPM_EVENT_XOFF, &info);
	}
}

static int fpm_hard_lockup_thread(void *data)
{
	msleep (1*1000);
	pr_info("%s: Locked up on CPU%d initiated... \n", __func__, smp_processor_id());
	spin_lock_irq(&fpm_lockup);

	/* We should not reach here */
	BUG();

	return 0;
}

static void fpm_hard_lockup_initiate(void)
{
	int cpu_cnt;
	char thread_name[25] = {0};

	spin_lock_init(&fpm_lockup);

	spin_lock(&fpm_lockup);

	/* Create kthreads per CPU */
	for (cpu_cnt=0; cpu_cnt<MAX_NO_OF_ONLINE_CPU; cpu_cnt++) {
		snprintf(thread_name, 8, "fhlt%d", cpu_cnt);
		fpm_lockup_thread[cpu_cnt] = kthread_create(fpm_hard_lockup_thread, NULL, thread_name);
		if (fpm_lockup_thread[cpu_cnt]) {
			kthread_bind(fpm_lockup_thread[cpu_cnt], cpu_cnt);
		}
	}

	/* Start kthreads */
	for (cpu_cnt=0; cpu_cnt<MAX_NO_OF_ONLINE_CPU; cpu_cnt++) {
		if (fpm_lockup_thread[cpu_cnt]) {
			wake_up_process(fpm_lockup_thread[cpu_cnt]);
		}
	}
}

static int fpm_is_watchdog_enabled(void)
{
	struct file *fp;

	fp = filp_open(WDMD_TASKS_FILE, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		fp = NULL;
		return 0;
	}
	filp_close(fp, NULL);
	return 1;
}

static void fpm_lwm_wd_work_handler(struct work_struct *work)
{
	struct fpmdev *fdev = container_of(work, struct fpmdev,
					   lwm_wd_work.work);
	pr_emerg("Too few tokens in FPM pool(s) for %d msec - rebooting.\n",
		jiffies_to_msecs(fdev->lwm_wd_timeout));
	fdev->lwm_wd_rs = true;
	if(fpm_is_watchdog_enabled()) {
		pr_info("Watchdog is enabled, hard lockup will do reboot...\n");
		fpm_hard_lockup_initiate();
		msleep (FPM_WAIT_TIMEOUT_BR*1000);
	}
	kernel_restart(NULL);
}

static int fpm_probe(struct platform_device *pdev)
{
	int status = 0;
	struct fpmdev *fdev;
	int p;
#ifdef CONFIG_BCM_FPM_TOKEN_HIST
	u32 num_tokens;
#endif

	pr_debug("-->\n");

	if (fpm) {
		pr_err("%s Exceeded max FPM devices.\n", __func__);
		status = -EFAULT;
		goto done;
	}

	fdev = kzalloc(sizeof(struct fpmdev), GFP_KERNEL);
	if (!fdev) {
		status = -ENOMEM;
		goto done;
	}

	fpm = fdev;
	pdev->dev.platform_data = fdev;
	fdev->pdev = pdev;

	status = fpm_parse_dt_node(pdev);
	if (status)
		goto err_free_fdev;

	if (FPM_GET_CHUNK_SIZE() == FPM_CHUNK_SIZE_512)
		fdev->chunk_size = 512;
	else
		fdev->chunk_size = 256;

	for (p = 0; p < fpm->npools; p++) {
		fdev->pool_ntokens[p] = fdev->pool_size[p] /
						fdev->chunk_size;
	}

#if FPM_CACHED
	for (p = 0; p < fpm->npools; p++) {
		fdev->pool_vbase[p] =
			memremap(fdev->pool_pbase[p],
				 fdev->pool_size[p],
				 MEMREMAP_WB);
	}
#else
	for (p = 0; p < fpm->npools; p++) {
		fdev->pool_vbase[p] = (u32)ioremap(fdev->pool_pbase[p],
						   fdev->pool_size[p]);
	}
#endif
	for (p = fpm->npools; p < FPM_MAX_DDR_POOLS; p++)
		fdev->pool_vbase[p] = 0;
	for (p = 0; p < fpm->npools; p++) {
		if (!fdev->pool_vbase[p]) {
			pr_err("%s Unable to memremap FPM pool %d @ 0x%llx physical.\n",
			       __func__, p, fdev->pool_pbase[p]);
			status = -EFAULT;
			goto err_free_fdev;
		}
	}

	fdev->buf_size_to_alloc_reg_map[0]  =
		FPM_ALLOC_1_CHUNKS;	/*  512 ||  256 */
	fdev->buf_size_to_alloc_reg_map[1]  =
		FPM_ALLOC_2_CHUNKS;	/* 1024 ||  512 */
	fdev->buf_size_to_alloc_reg_map[2]  =
		FPM_ALLOC_4_CHUNKS;	/* 2048 || 1024 */
	fdev->buf_size_to_alloc_reg_map[3]  =
		FPM_ALLOC_4_CHUNKS;	/* 2048 || 1024 */
	fdev->buf_size_to_alloc_reg_map[4]  =
		FPM_ALLOC_8_CHUNKS;	/* 4096 || 2048 */
	fdev->buf_size_to_alloc_reg_map[5]  =
		FPM_ALLOC_8_CHUNKS;	/* 4096 || 2048 */
	fdev->buf_size_to_alloc_reg_map[6]  =
		FPM_ALLOC_8_CHUNKS;	/* 4096 || 2048 */
	fdev->buf_size_to_alloc_reg_map[7]  =
		FPM_ALLOC_8_CHUNKS;	/* 4096 || 2048 */

#ifdef CONFIG_BCM_FPM_TOKEN_FORMAT_VARIABLE
	fdev->buf_sizes[0] = fdev->chunk_size << 3;
	fdev->buf_sizes[1] = fdev->chunk_size << 2;
	fdev->buf_sizes[2] = fdev->chunk_size << 1;
	fdev->buf_sizes[3] = fdev->chunk_size << 0;
	fdev->tok_idx_to_chunks_map = kzalloc(__fpm_max_tokens(), GFP_KERNEL);
	if (!fdev->tok_idx_to_chunks_map)
		goto err_free_fdev;
	fdev->chunk_to_alloc_reg_map[3]  =
		FPM_ALLOC_1_CHUNKS;	/*  512 ||  256 */
	fdev->chunk_to_alloc_reg_map[2]  =
		FPM_ALLOC_2_CHUNKS;	/* 1024 ||  512 */
	fdev->chunk_to_alloc_reg_map[1]  =
		FPM_ALLOC_4_CHUNKS;	/* 2048 || 1024 */
	fdev->chunk_to_alloc_reg_map[0]  =
		FPM_ALLOC_8_CHUNKS;	/* 4096 || 2048 */
#endif

	if (fpm->pad_in_ctrl_spare) {
		fpm->net_buf_head_pad = FPM_GET_CTL_SPARE() >> 16;
		fpm->net_buf_tail_pad = FPM_GET_CTL_SPARE() & 0xffff;
	}

	fdev->pool_alloc_weight[0] = FPM_GET_ALLOC_WEIGHT(0);
	fdev->pool_free_weight[0]  = FPM_GET_FREE_WEIGHT(0);
	fdev->pool_alloc_weight[1] = FPM_GET_ALLOC_WEIGHT(1);
	fdev->pool_free_weight[1]  = FPM_GET_FREE_WEIGHT(1);

#ifdef CONFIG_BCM_FPM_TOKEN_HIST
	num_tokens = __fpm_max_tokens( );
	if (fpm->npools > 1)
		num_tokens <<= 1;
	fdev->tok_ref_count = kzalloc(num_tokens, GFP_KERNEL);
	if (!fdev->tok_ref_count) {
		pr_err("%s Unable to alloc token history array.\n",
		       __func__);
		status = -ENOMEM;
		goto err_free_fdev;
	}
	pr_info("Allocating %d bytes (%d ops) for token history buffer.\n",
		FPM_HISTORY_MEM_SIZE, FPM_NUM_HISTORY_ENTRIES);
	fdev->tok_hist_start = kzalloc(FPM_NUM_HISTORY_ENTRIES *
		sizeof(struct fpm_tok_op), GFP_KERNEL);
	if (!fdev->tok_hist_start) {
		pr_err("%s Unable to alloc token history buffer.\n",
		       __func__);
		status = -ENOMEM;
		goto err_free_hist;
	}
	fdev->tok_hist_end = fdev->tok_hist_start +
		FPM_NUM_HISTORY_ENTRIES;
	fdev->tok_hist_head = fdev->tok_hist_start;
	fdev->tok_hist_tail = fdev->tok_hist_start;
	spin_lock_init(&fdev->tok_hist_lock);
	fdev->track_on_err = false;
#endif

	pr_info("%s: FPM with %d pool(s)\n", MODULE_NAME, fpm->npools);
	for (p = 0; p < fpm->npools; p++) {
		pr_debug("%s: FPM pool %d phys 0x%llx virt %p size 0x%08x (%d)\n",
			 MODULE_NAME, p, fdev->pool_pbase[p], fdev->pool_vbase[p],
			 fpm->pool_size[p], fpm->pool_size[p]);
	}

	INIT_DELAYED_WORK(&fpm->pool_full_work, fpm_pool_full_work_handler);
	INIT_WORK(&fpm->xon_work, fpm_xon_work_handler);
	INIT_WORK(&fpm->xoff_work, fpm_xoff_work_handler);
	FPM_SET_XON_XOFF_THRESHOLD(0, (FPM_XON_THRESHOLD << 16) | FPM_XOFF_THRESHOLD);
	INIT_DELAYED_WORK(&fpm->lwm_wd_work, fpm_lwm_wd_work_handler);

	fpm_proc_init();

	goto done;

#ifdef CONFIG_BCM_FPM_TOKEN_HIST
err_free_hist:
	if (fdev->tok_ref_count)
		kfree(fdev->tok_ref_count);
#endif

err_free_fdev:
	fpm = NULL;
	kfree(fdev);

done:
	pr_debug("<--\n");
	return status;
}

static int fpm_remove(struct platform_device *pdev)
{
	int status = 0;
	struct fpmdev *fdev = pdev->dev.platform_data;

	pr_debug("-->\n");

	if (!fdev) {
		pr_err("%s Release called with uninitialized ",
		       __func__);
		pr_err("platform_data.\n");
		status = -EINVAL;
		goto done;
	}
	memunmap((void *)fdev->pool_vbase[0]);
	if (fdev->pool_vbase[1])
		memunmap((void *)fdev->pool_vbase[1]);
	iounmap((void *)fdev->reg_vbase);

#ifdef CONFIG_BCM_FPM_TOKEN_HIST
	if (fdev->tok_ref_count)
		kfree(fdev->tok_ref_count);
	if (fdev->tok_hist_start)
		kfree(fdev->tok_hist_start);
#endif
#ifdef CONFIG_BCM_FPM_TOKEN_FORMAT_VARIABLE
	kfree(fdev->tok_idx_to_chunks_map);
#endif

	flush_work(&fdev->pool_full_work.work);

	fpm = NULL;
	kfree(fdev);

done:
	pr_debug("<--\n");
	return status;
}

static const struct of_device_id fpm_of_match[] = {
	{.compatible = "brcm,fpm"},
	{}
};

MODULE_DEVICE_TABLE(of, fpm_of_match);

static struct platform_driver fpm_driver = {
	.probe  = fpm_probe,
	.remove = fpm_remove,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = fpm_of_match
	}
};

static int __init fpm_init(void)
{
	int status = 0;

	pr_debug("%s driver v%s", MODULE_NAME, MODULE_VER);

	spin_lock_init(&fpm_reg_lock);
	status = platform_driver_register(&fpm_driver);
	if (status)
		goto done;

done:
	return status;
}

static void __exit fpm_exit(void)
{
	platform_driver_unregister(&fpm_driver);
	fpm_proc_exit();
}

subsys_initcall(fpm_init);
module_exit(fpm_exit);
MODULE_LICENSE("GPL v2");
