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
#ifndef __FPM_CLIENT_H_
#define __FPM_CLIENT_H_

#include <linux/notifier.h>

u32 fpm_alloc_token(int size);
u8 *fpm_alloc_buffer(int size);
void fpm_free_token(u32);
void fpm_free_buffer(u8 *);
u32 fpm_incr_multicount(u32);
u8 *fpm_token_to_buffer(u32);
u32 fpm_buffer_to_token(u8 *, u32);
int fpm_is_fpm_buf(void *buf);
u32 fpm_get_token_size(u32);
int fpm_set_token_size(u32 *, u32);
int fpm_set_token_index(u32 *, u32);
u32 fpm_get_token_index(u32);
bool fpm_is_valid_token(u32 token);
void fpm_flush_invalidate_token(u32 token, u32 head, u32 tail, u32 flags);
void fpm_invalidate_token(u32 token, u32 head, u32 tail, u32 flags);
void fpm_flush_invalidate_buffer(void *start, u32 size);
void fpm_invalidate_buffer(void *start, u32 size);
int fpm_get_min_buffer_size(int size);

phys_addr_t fpm_buf_virt_to_phys(void *buf);
void *fpm_buf_phys_to_virt(phys_addr_t paddr);

struct fpm_hw_info {
	/* physical address(es) of pool(s) */
	u32 pool_base[2];
	/* physical addresses of alloc_dealloc registers */
	u32 alloc_dealloc[4];
	/* configured chunk size */
	u32 chunk_size;
	/* network buffer head padding */
	u32 net_buf_head_pad;
	/* network buffer tail padding */
	u32 net_buf_tail_pad;
};
int fpm_get_hw_info(struct fpm_hw_info *);
int fpm_is_fpm_buf(void *buf);
u32 fpm_token_pool(u32 token);

/* FPM sync flags */
#define	FPM_SYNC_HEAD	0x1
#define FPM_SYNC_TAIL	0x2	/* ignored if token format doen't have pool ID bits */

struct fpm_pool_stats {
	u32	underflow_count;
	u32	overflow_count;
	u32	tok_avail;
	u32	alloc_fifo_empty;
	u32	alloc_fifo_full;
	u32	free_fifo_empty;
	u32	free_fifo_full;
	u32	pool_full;
	u32	invalid_tok_frees;
	u32	invalid_tok_multi;
	u32	mem_corrupt_tok;
	u32	mem_corrupt_tok_valid;
	u32	invalid_free_tok;
	u32	invalid_free_tok_valid;
	u32	invalid_mcast_tok;
	u32	invalid_mcast_tok_valid;
};
int fpm_get_pool_stats(int pool, struct fpm_pool_stats *stats);
u32 fpm_get_tok_avail(int pool);
void fpm_reset_bb(bool reset);

/*
 * Token tracking src/dest data is context-specific, but
 * limited to a u32.
 */
union tok_src_dest {
	u32 data;		/* Generic reference to context data */
	u32 rpc_hdr;		/* RPC header when src/dest is RPC */
	struct {		/* interface ID/sub-ID when packet */
#ifdef __LITTLE_ENDIAN__	/* output is a u32 - keep order consistent */
		u8 id;
		u8 sub_id;
		u16 rsvd;
#else
		u16 rsvd;
		u8 sub_id;
		u8 id;
#endif
	} iface;
};

enum fpm_event {
	FPM_EVENT_XON,
	FPM_EVENT_XOFF,
};

struct fpm_notifier_info {
	u8	pool;
	u8	event;
};

int register_fpm_notifier(struct notifier_block *nb);
int unregister_fpm_notifier(struct notifier_block *nb);

#ifdef CONFIG_BCM_FPM_TOKEN_HIST
void fpm_track_token_rx(u32 token);
void fpm_track_token_src(u32 token, union tok_src_dest *src);
void fpm_track_token_tx(u32 token);
void fpm_track_token_dest(u32 token, union tok_src_dest *dest);
void fpm_dump_token_hist(u32 token);
void fpm_dump_hist(u32 entries);
#else
#define fpm_track_token_rx(token)
#define fpm_track_token_src(token, src)
#define fpm_track_token_tx(token)
#define fpm_track_token_dest(token, dest)
#define fpm_dump_token_hist(token)
#define fpm_dump_hist(entries)
#endif

#endif
