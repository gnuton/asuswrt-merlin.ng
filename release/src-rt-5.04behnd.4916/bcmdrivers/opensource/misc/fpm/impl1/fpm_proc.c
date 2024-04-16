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
  * Author: Tim Ross <tross@broadcom.com>
  *****************************************************************************/
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
#include <linux/panic_notifier.h>
#endif

#include <proc_cmd.h>
#include "fpm.h"
#include "fpm_dev.h"
#include "fpm_priv.h"

#define PROC_DIR		"driver/fpm"
#define CMD_PROC_FILE		"cmd"
#define HEAD_PROC_FILE		"head"
#define TAIL_PROC_FILE		"tail"
#define PKT_PROC_FILE		"pkt"
#define HP_PROC_FILE		"hp"
#define HPT_PROC_FILE		"hpt"
#define FPM_PROC_FILE		"fpm"
#define HIST_PROC_FILE		"hist"
#define STATS_PROC_FILE		"stats"

static u32 token;
static u32 head_size;
static u32 tail_size;
struct seq_buf {
	u8	*buf;
	u32	remain;
	loff_t	offset;
	u8	line[5 + 32 * 3 + 2 + 32 + 1];
};
static struct seq_buf sbuf;

static int tot_num_tok;
struct tokens_list {
	struct list_head list;
	u32 *tokens;
	int num_tok;
};
static struct list_head tokens_lh;

static int fpm_stats_hw_show(struct seq_file *seq)
{
	struct fpm_hw_info info;

	if (fpm_get_hw_info(&info))
		return 0;
	pr_seq(seq, "\n");
	pr_seq(seq, "Buffer Size (bytes)..............%d\n",
		info.chunk_size);
	pr_seq(seq, "Offset (Head) (bytes)............%d\n",
		info.net_buf_head_pad);
	pr_seq(seq, "Pad (Tail) (bytes)...............%d\n",
		info.net_buf_tail_pad);
	pr_seq(seq, "Low token watchdog timeout (ms)..%d (0 = disabled)\n",
		fpm->lwm_wd_timeout);
	pr_seq(seq, "Low token watchdog reboot status %d\n",
		fpm->lwm_wd_rs);
	pr_seq(seq, "xon_xoff_state 0x%08lx\n", fpm->xon_xoff_state);
	return 0;
}
static int fpm_stats_pool_show(struct seq_file *seq, int pool)
{
	struct fpm_pool_stats stats;
	struct timespec64 tv;
	struct fpm_hw_info info;

	if (fpm_get_hw_info(&info))
		return 0;
	fpm_get_pool_stats(pool, &stats);
	pr_seq(seq, "\n");
	pr_seq(seq, "Pool %d:\n", pool);
	pr_seq(seq, "Base Address (Phys)..............0x%llx\n",
		fpm->pool_pbase[pool]);
	pr_seq(seq, "Base Address (Virt)..............0x%p\n",
		fpm->pool_vbase[pool]);
	pr_seq(seq, "Pool Size (bytes)................%d\n",
		fpm->pool_size[pool]);
	pr_seq(seq, "Total Tokens.....................%d\n",
		fpm->pool_size[pool] / info.chunk_size);
	pr_seq(seq, "Available Tokens.................%d\n",
		stats.tok_avail);
	pr_seq(seq, "Pool Alloc Weight................%d\n",
		fpm->pool_alloc_weight[pool]);
	pr_seq(seq, "Pool Free Weight.................%d\n",
		fpm->pool_free_weight[pool]);
	pr_seq(seq, "Underflow Count..................%d\n",
		stats.underflow_count);
	pr_seq(seq, "Overflow Count...................%d\n",
		stats.overflow_count);
	pr_seq(seq, "Alloc FIFO Empty.................%d\n",
		stats.alloc_fifo_empty);
	pr_seq(seq, "Alloc FIFO Full..................%d\n",
		stats.alloc_fifo_full);
	pr_seq(seq, "Free FIFO Empty..................%d\n",
		stats.free_fifo_empty);
	pr_seq(seq, "Free FIFO Full...................%d\n",
		stats.free_fifo_full);
	pr_seq(seq, "Pool Full........................%d\n",
		stats.pool_full);
	pr_seq(seq, "Not Valid Token Frees............%d\n",
		stats.invalid_tok_frees);
	pr_seq(seq, "Not Valid Token Multicount.......%d\n",
		stats.invalid_tok_multi);
	pr_seq(seq, "Xon Interrupts...................0x%x\n",
		fpm->xon_cnt[pool]);
	pr_seq(seq, "Xoff Interrupts..................0x%x\n",
		fpm->xoff_cnt[pool]);
	jiffies_to_timespec64(jiffies - fpm->xon_tstamp[pool], &tv);
	pr_seq(seq, "Xon Timestamp....................%llds %ldus\n",
		tv.tv_sec, tv.tv_nsec*1000);
	jiffies_to_timespec64(jiffies - fpm->xoff_tstamp[pool], &tv);
	pr_seq(seq, "Xoff Timestamp...................%llds %ldus\n",
		tv.tv_sec, tv.tv_nsec*1000);
	pr_seq(seq, "Mem Corrupt Interrupts...........0x%x\n",
		fpm->mem_corrupt_cnt[pool]);
	pr_seq(seq, "\n");
	return 0;
}

static void fpm_proc_cmd_set_help(char *str)
{
	pr_alert("%s set: Set Token fields\n", str);
	pr_alert("%s  set [tok | idx | size | head | tail] {val}\n", str);
	pr_alert("%s   tok    set the token to {val}\n", str);
	pr_alert("%s   idx    set the token index to {val}\n", str);
	pr_alert("%s   size   set the token size to {val}\n", str);
	pr_alert("%s   head   set the head size (offset) to {val}\n", str);
	pr_alert("%s   tail   set the tail size (padding) to {val}\n", str);
}

static void fpm_proc_cmd_get_help(char *str)
{
	pr_alert("%s get: Get Token fields\n", str);
	pr_alert("%s  get [tok | idx | size | head | tail]\n", str);
	pr_alert("%s   tok    get the token val\n", str);
	pr_alert("%s   idx    get the token index val\n", str);
	pr_alert("%s   size   get the token size val\n", str);
	pr_alert("%s   head   get the head size (offset) val\n", str);
	pr_alert("%s   tail   get the tail size (padding) val\n", str);
}

static void fpm_proc_cmd_alloc_help(char *str)
{
	pr_alert("%s alloc: Allocate N tokens\n", str);
	pr_alert("%s  alloc [tok]\n", str);
	pr_alert("%s   tok    allocate tok tokens from the FPM pool\n", str);
	pr_alert("%s          if tok=0 then all remaining tokens will\n", str);
	pr_alert("%s          be allocated\n", str);
}

static void fpm_proc_cmd_free_help(char *str)
{
	pr_alert("%s free: Free N tokens\n", str);
	pr_alert("%s  free [tok]\n", str);
	pr_alert("%s   tok    free tok tokens back to the FPM pool\n", str);
	pr_alert("%s          if tok=0 then all tokens previously\n", str);
	pr_alert("%s          allocated here will be freed\n", str);
}
#ifdef CONFIG_BCM_FPM_TOKEN_HIST
static void fpm_proc_cmd_hist_help(char *str)
{
	pr_alert("%s hist: Set token tracking history on/off\n", str);
	pr_alert("%s  hist {on|off|stop_on_err}\n", str);
	pr_alert("%s   on|off       turn token tracking history on/off\n", str);
	pr_alert("%s   stop_on_err  stop tracking when error detected\n", str);
	pr_alert("%s   cont_on_err  continue tracking when error detected\n",
			 str);
}
#endif
static void fpm_proc_cmd_xon_xoff_help(char *str)
{
	pr_alert("%s xon_xoff: Set xon and xoff threshold\n", str);
	pr_alert("%s  xon_xoff xon_val  xoff_val\n", str);
	pr_alert("%s   xon_val  xon threshold (16bit)\n", str);
	pr_alert("%s   xoff_val xoff threshold (16bit)\n", str);
}

static void fpm_proc_cmd_show_help(char *str)
{
	pr_alert("%s show: Show FPM Stats\n", str);
}

static void fpm_proc_cmd_help_help(char *str)
{
	pr_alert("%s help: Help on individual command\n", str);
}

static int fpm_proc_cmd_help(int argc, char *argv[])
{
	int status = 0;

	if (argc != 2) {
		pr_err("Invalid # of arguments.\n");
		status = -EINVAL;
		goto done;
	}

	if (!strncmp(argv[1], "set", sizeof("set"))) {
		fpm_proc_cmd_set_help("");
	} else if (!strncmp(argv[1], "get", sizeof("get"))) {
		fpm_proc_cmd_get_help("");
	} else if (!strncmp(argv[1], "alloc", sizeof("alloc"))) {
		fpm_proc_cmd_alloc_help("");
	} else if (!strncmp(argv[1], "free", sizeof("free"))) {
		fpm_proc_cmd_free_help("");
#ifdef CONFIG_BCM_FPM_TOKEN_HIST
	} else if (!strncmp(argv[1], "hist", sizeof("hist"))) {
		fpm_proc_cmd_hist_help("");
#endif
	} else if (!strncmp(argv[1], "xon_xoff", sizeof("xon_xoff"))) {
		fpm_proc_cmd_xon_xoff_help("");
	} else if (!strncmp(argv[1], "show", sizeof("show"))) {
		fpm_proc_cmd_show_help("");
	} else {
		pr_err("Unrecognized command: %s\n", argv[1]);
		status = -EINVAL;
	}

done:
	return status;
}

static int fpm_proc_cmd_set(int argc, char *argv[])
{
	int status = 0;
	u32 idx, size;

	if (argc != 3) {
		pr_err("Invalid # of arguments.\n");
		status = -EINVAL;
		goto done;
	}

	if (!strncmp(argv[1], "tok", strlen("tok"))) {
		status = kstrtou32(argv[2], 0, &token);
	} else if (!strncmp(argv[1], "idx", strlen("idx"))) {
		status = kstrtou32(argv[2], 0, &idx);
		fpm_set_token_index(&token, idx);
	} else if (!strncmp(argv[1], "size", strlen("size"))) {
		status = kstrtou32(argv[2], 0, &size);
		fpm_set_token_size(&token, size);
	} else if (!strncmp(argv[1], "head", strlen("head"))) {
		status = kstrtou32(argv[2], 0, &head_size);
	} else if (!strncmp(argv[1], "tail", strlen("tail"))) {
		status = kstrtou32(argv[2], 0, &tail_size);
	} else {
		pr_err("Unrecognized variable: %s\n", argv[1]);
		status = -EINVAL;
		goto done;
	}
	if (status) {
		pr_err("Bad value: %s\n", argv[2]);
		goto done;
	}
done:
	return status;
}

static int fpm_proc_cmd_get(int argc, char *argv[])
{
	int status = 0;

	if (argc != 2) {
		pr_err("Invalid # of arguments.\n");
		status = -EINVAL;
		goto done;
	}

	if (!strncmp(argv[1], "tok", strlen("tok")))
		pr_info("token: 0x%08x\n", token);
	else if (!strncmp(argv[1], "idx", strlen("idx")))
		pr_info("token index: 0x%05x\n",
			__fpm_get_token_index(token));
	else if (!strncmp(argv[1], "size", strlen("size")))
		pr_info("token size: 0x%03x\n",
			fpm_get_token_size(token));
	else if (!strncmp(argv[1], "head", strlen("head")))
		pr_info("head size: %d bytes\n", head_size);
	else if (!strncmp(argv[1], "tail", strlen("tail")))
		pr_info("tail size: %d bytes\n", tail_size);
	else {
		pr_err("Unrecognized variable: %s\n", argv[1]);
		status = -EINVAL;
		goto done;
	}
done:
	return status;
}

static int fpm_proc_cmd_alloc(int argc, char *argv[])
{
	int status = 0;
	u32 n, nt, t;
	u32 i = 0, acc_i = 0;
	struct tokens_list *t_list;
	u32 *new_tokens;
	u32 cnt = 0, dyn_alloc_cnt = 0;
	u32 max_alloc_n = 0;

	if (argc != 2) {
		pr_err("Invalid # of arguments.\n");
		status = -EINVAL;
		goto done;
	}

	status = kstrtou32(argv[1], 0, &n);
	if (status) {
		pr_err("Bad value: %s\n", argv[1]);
		goto done;
	}

	if (!n) {
		pr_info("Allocating all available tokens.\n");
		n = __fpm_max_tokens();
	} else if (n > __fpm_max_tokens()) {
		pr_warn("Specified number of tokens greater than available"
			" so allocating only max available tokens.\n");
		n = __fpm_max_tokens();
	}
	if (n <= 0)
		goto info;

	/* Calculate Max Number of Dynamic allocation loop */
	dyn_alloc_cnt = ((n * sizeof(n)) / (PAGE_SIZE * 1024)) + 1;
	/* Allocate Max of one Page at a time */
	max_alloc_n = ((PAGE_SIZE * 1024)/sizeof(n));
	nt = n;
	for (cnt = 0; cnt < dyn_alloc_cnt; cnt++) {
		if (nt >= max_alloc_n) {
			nt -= max_alloc_n;
			n = max_alloc_n;
		} else
			n = nt;

		if (n <= 0)
			goto info;

		t_list = kzalloc(sizeof(struct tokens_list), GFP_KERNEL);
		if (!t_list) {
			status = -ENOMEM;
			goto done;
		}
		t_list->tokens = kcalloc(n, sizeof(n), GFP_KERNEL);
		if (!t_list->tokens) {
			pr_err("Unable to allocate token stack.\n");
			kfree(t_list);
			status = -ENOMEM;
			goto done;
		}
		t_list->num_tok = n;
		for (i = 0; i < n; i++, tot_num_tok++) {
			t = fpm_alloc_token(1);
			if (!t) {
				if (i > 0) {
					/* Reallocate Tokens when available
					 * token is less than allocated
					 */
					new_tokens = krealloc(t_list->tokens,
						i * sizeof(u32), GFP_KERNEL);
					if (!new_tokens) {
						pr_err("Unable to reallocate token stack.\n");
						kfree(t_list->tokens);
						kfree(t_list);
						status = -ENOMEM;
						goto done;
					}
					t_list->tokens = new_tokens;
					t_list->num_tok = i;
				} else {
					kfree(t_list->tokens);
					kfree(t_list);
					t_list = NULL;
					goto info;
				}
				break;
			} else
				t_list->tokens[i] = t;
		}
		if (t_list)
			list_add(&t_list->list, &tokens_lh);

		acc_i += i;
		i = 0;
	}

info:
	pr_info("Allocated %d tokens - total avilable tokens %d.\n",
			acc_i, tot_num_tok);
done:
	return status;
}

static int fpm_proc_cmd_free(int argc, char *argv[])
{
	int status = 0;
	u32 n;
	u32 i = 0;
	u32 num_tok = 0, prev_tot_num_token = tot_num_tok;
	struct tokens_list *t_list, *tmp;
	u32 *new_tokens;

	if (argc != 2) {
		pr_err("Invalid # of arguments.\n");
		status = -EINVAL;
		goto done;
	}

	status = kstrtou32(argv[1], 0, &n);
	if (status) {
		pr_err("Bad value: %s\n", argv[1]);
		goto done;
	}
	if (!n) {
		pr_info("Freeing all tokens held by FPM driver.\n");
		n = __fpm_max_tokens();
	}
	if (n <= 0)
		goto info;

	list_for_each_entry_safe(t_list, tmp, &tokens_lh, list) {
		if (!t_list || !t_list->tokens) {
			status = -EINVAL;
			goto done;
		}
		num_tok = t_list->num_tok;
		for (i = 0; i < n && tot_num_tok > 0 &&
			  t_list->num_tok > 0; i++,
			  tot_num_tok--, t_list->num_tok--)
			fpm_free_token(t_list->tokens[t_list->num_tok - 1]);

		if (t_list->num_tok <= 0) {
			list_del(&t_list->list);
			kfree(t_list->tokens);
			kfree(t_list);
			n -= num_tok;
		} else if (num_tok > n) {
			if (n <= 0)
				break;
			new_tokens = krealloc(t_list->tokens,
				(num_tok - n) * sizeof(u32), GFP_KERNEL);
			if (!new_tokens) {
				pr_err("Unable to reallocate token stack.\n");
				status = -ENOMEM;
				goto done;
			}
			t_list->tokens = new_tokens;
			break;
		}
	}

info:
	pr_info("Freed %u tokens - total avilable tokens %d.\n",
			(prev_tot_num_token - tot_num_tok), tot_num_tok);
done:
	return status;
}

static int fpm_proc_cmd_xon_xoff(int argc, char *argv[])
{
	int status = 0;
	u16 xon, xoff;
	u32 thresh;

	if (argc == 1) {
		thresh = FPM_GET_XON_XOFF_THRESHOLD(0);
		xon = (thresh>>16) & 0xFFFF;
		xoff = thresh&0xFFFF;
		pr_info("Pool 0 - Xon: 0x%x Xoff: 0x%0x\n", xon, xoff);
	} else if (argc == 3) {
		status = kstrtou16(argv[1], 0, &xon);
		if (status) {
			pr_err("Bad Xon value: %s\n", argv[1]);
			goto done;
		}
		status = kstrtou16(argv[2], 0, &xoff);
		if (status) {
			pr_err("Bad Xoff value: %s\n", argv[2]);
			goto done;
		}
		if (xoff > xon) {
			pr_err("Xon %s should be > Xoff %s\n",
			       argv[1], argv[2]);
			status = EINVAL;
			goto done;
		}
		thresh = xon << 16 | xoff;
		FPM_SET_XON_XOFF_THRESHOLD(0, thresh);
		fpm->xon_xoff_state = 0;
	} else {
		pr_info("%s xon_threshold(16bit) xoff_threshold(16bit)\n",
			argv[0]);
	}
done:
	return status;
}

static int fpm_proc_cmd_show(int argc, char *argv[])
{
	int pool;

	fpm_stats_hw_show(NULL);
	for (pool = 0; pool < fpm->npools; pool++)
		fpm_stats_pool_show(NULL, pool);
	return 0;
}

static void *fpm_head_proc_start(struct seq_file *seq, loff_t *pos)
{
	struct seq_buf *sb = NULL;

	if (*pos == 0) {
		sb = &sbuf;
		sb->offset = 0;
		sb->buf = fpm_token_to_buffer(token) + sb->offset;
		sb->remain = head_size;
		seq_puts(seq, "-----------------------------head----------");
		seq_puts(seq, "--------------------------\n");
	}

	return sb;
}

static void *fpm_tail_proc_start(struct seq_file *seq, loff_t *pos)
{
	struct seq_buf *sb = NULL;

	if (*pos == 0) {
		sb = &sbuf;
		sb->offset = head_size + fpm_get_token_size(token);
		sb->buf = fpm_token_to_buffer(token) + sb->offset;
		sb->remain = tail_size;
		seq_puts(seq, "-----------------------------tail----------");
		seq_puts(seq, "--------------------------\n");
	}

	return sb;
}

static void *fpm_pkt_proc_start(struct seq_file *seq, loff_t *pos)
{
	struct seq_buf *sb = NULL;

	if (*pos == 0) {
		sb = &sbuf;
		sb->offset = head_size;
		sb->buf = fpm_token_to_buffer(token) + sb->offset;
		sb->remain = fpm_get_token_size(token);
		seq_puts(seq, "-----------------------------pkt-----------");
		seq_puts(seq, "--------------------------\n");
	}

	return sb;
}

static void *fpm_hp_proc_start(struct seq_file *seq, loff_t *pos)
{
	struct seq_buf *sb = NULL;

	if (*pos == 0) {
		sb = &sbuf;
		sb->offset = 0;
		sb->buf = fpm_token_to_buffer(token) + sb->offset;
		sb->remain = head_size;
		seq_puts(seq, "-----------------------------head----------");
		seq_puts(seq, "--------------------------\n");
	} else if (*pos == 1) {
		sb = &sbuf;
		sb->buf = fpm_token_to_buffer(token) + sb->offset;
		sb->remain = fpm_get_token_size(token);
		seq_puts(seq, "-----------------------------pkt-----------");
		seq_puts(seq, "--------------------------\n");
	}

	return sb;
}

static void *fpm_hpt_proc_start(struct seq_file *seq, loff_t *pos)
{
	struct seq_buf *sb = NULL;

	if (*pos == 0) {
		sb = &sbuf;
		sb->offset = 0;
		sb->buf = fpm_token_to_buffer(token) + sb->offset;
		sb->remain = head_size;
		seq_puts(seq, "-----------------------------head----------");
		seq_puts(seq, "--------------------------\n");
	} else if (*pos == 1) {
		sb = &sbuf;
		sb->buf = fpm_token_to_buffer(token) + sb->offset;
		sb->remain = fpm_get_token_size(token);
		seq_puts(seq, "-----------------------------pkt-----------");
		seq_puts(seq, "--------------------------\n");
	} else if (*pos == 2) {
		sb = &sbuf;
		sb->buf = fpm_token_to_buffer(token) + sb->offset;
		sb->remain = tail_size;
		seq_puts(seq, "-----------------------------tail----------");
		seq_puts(seq, "--------------------------\n");
	}

	return sb;
}

static void fpm_buf_proc_stop(struct seq_file *seq, void *v)
{
}

static void *fpm_buf_proc_next(struct seq_file *seq, void *v,
			       loff_t *pos)
{
	struct seq_buf *sb = v;

	if (!sb->remain)
		sb = NULL;

	return sb;
}

static int fpm_buf_proc_show(struct seq_file *seq, void *v)
{
	struct seq_buf *sb = v;
	int len;

	len = sb->remain < 16 ? sb->remain : 16;
	snprintf(sb->line, 6, "%03llx: ", sb->offset);
	hex_dump_to_buffer(sb->buf, len, 16, 1,
			   &sb->line[5], sizeof(sb->line)-5, true);
	seq_printf(seq, "%s\n", sb->line);
	sb->remain -= len;
	sb->buf += len;
	sb->offset += len;

	return 0;
}

static const struct seq_operations pkt_seq_ops = {
	.start	= fpm_pkt_proc_start,
	.stop	= fpm_buf_proc_stop,
	.next	= fpm_buf_proc_next,
	.show	= fpm_buf_proc_show,
};
static int fpm_pkt_proc_open(struct inode *inode, struct file *file)
{
	int fid;

	fid = seq_open(file, &pkt_seq_ops);
	return fid;
}


static const struct seq_operations head_seq_ops = {
	.start	= fpm_head_proc_start,
	.stop	= fpm_buf_proc_stop,
	.next	= fpm_buf_proc_next,
	.show	= fpm_buf_proc_show,
};
static int fpm_head_proc_open(struct inode *inode, struct file *file)
{
	int fid;

	fid = seq_open(file, &head_seq_ops);
	return fid;
}

static const struct seq_operations tail_seq_ops = {
	.start	= fpm_tail_proc_start,
	.stop	= fpm_buf_proc_stop,
	.next	= fpm_buf_proc_next,
	.show	= fpm_buf_proc_show,
};
static int fpm_tail_proc_open(struct inode *inode, struct file *file)
{
	int fid;

	fid = seq_open(file, &tail_seq_ops);
	return fid;
}
static const struct seq_operations hp_seq_ops = {
	.start	= fpm_hp_proc_start,
	.stop	= fpm_buf_proc_stop,
	.next	= fpm_buf_proc_next,
	.show	= fpm_buf_proc_show,
};
static int fpm_hp_proc_open(struct inode *inode, struct file *file)
{
	int fid;

	fid = seq_open(file, &hp_seq_ops);
	return fid;
}
static const struct seq_operations hpt_seq_ops = {
	.start	= fpm_hpt_proc_start,
	.stop	= fpm_buf_proc_stop,
	.next	= fpm_buf_proc_next,
	.show	= fpm_buf_proc_show,
};
static int fpm_hpt_proc_open(struct inode *inode, struct file *file)
{
	int fid;

	fid = seq_open(file, &hpt_seq_ops);
	return fid;
}

static void *fpm_stats_proc_start(struct seq_file *seq, loff_t *pos)
{
	void *start = NULL;

	if (*pos == 0)
		start = SEQ_START_TOKEN;

	return start;
}
static void fpm_stats_proc_stop(struct seq_file *seq, void *v)
{
}
static void *fpm_stats_proc_next(struct seq_file *seq, void *v,
				loff_t *pos)
{
	int pool;
	void *next;

	pool = v - SEQ_START_TOKEN;
	if (pool >= fpm->npools) {
		next = NULL;
	} else {
		if (fpm->pool_pbase[pool])
			next = v + 1;
		else
			next = NULL;
	}
	return next;
}
static int fpm_stats_proc_show(struct seq_file *seq, void *v)
{
	int pool;

	if (v == SEQ_START_TOKEN) {
		fpm_stats_hw_show(seq);
	} else {
		pool = v - SEQ_START_TOKEN - 1;
		fpm_stats_pool_show(seq, pool);
	}
	return 0;
}
static const struct seq_operations stats_seq_ops = {
	.start	= fpm_stats_proc_start,
	.stop	= fpm_stats_proc_stop,
	.next	= fpm_stats_proc_next,
	.show	= fpm_stats_proc_show,
};
static int fpm_stats_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &stats_seq_ops);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
static const struct proc_ops pkt_fops = {
	.proc_open		= fpm_pkt_proc_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= seq_release,
};
static const struct proc_ops head_fops = {
	.proc_open		= fpm_head_proc_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= seq_release,
};
static const struct proc_ops tail_fops = {
	.proc_open		= fpm_tail_proc_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= seq_release,
};
static const struct proc_ops hp_fops = {
	.proc_open		= fpm_hp_proc_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= seq_release,
};
static const struct proc_ops hpt_fops = {
	.proc_open		= fpm_hpt_proc_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= seq_release,
};
static const struct proc_ops stats_fops = {
	.proc_open		= fpm_stats_proc_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= seq_release,
};
#else
static const struct file_operations pkt_fops = {
	.owner		= THIS_MODULE,
	.open		= fpm_pkt_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
static const struct file_operations head_fops = {
	.owner		= THIS_MODULE,
	.open		= fpm_head_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
static const struct file_operations tail_fops = {
	.owner		= THIS_MODULE,
	.open		= fpm_tail_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
static const struct file_operations hp_fops = {
	.owner		= THIS_MODULE,
	.open		= fpm_hp_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
static const struct file_operations hpt_fops = {
	.owner		= THIS_MODULE,
	.open		= fpm_hpt_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
static const struct file_operations stats_fops = {
	.owner		= THIS_MODULE,
	.open		= fpm_stats_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
#endif

#ifdef CONFIG_BCM_FPM_TOKEN_HIST
static int fpm_proc_cmd_hist(int argc, char *argv[])
{
	struct fpmdev *fdev = fpm;
	int status = 0;

	if (argc != 2) {
		pr_err("Invalid # of arguments.\n");
		status = -EINVAL;
		goto done;
	}

	if (!strncmp(argv[1], "on", strlen("on")))
		fdev->track_tokens = true;
	else if (!strncmp(argv[1], "off", strlen("off")))
		fdev->track_tokens = false;
	else if (!strncmp(argv[1], "stop_on_err", strlen("stop_on_err")))
		fdev->track_on_err = false;
	else if (!strncmp(argv[1], "cont_on_err", strlen("cont_on_err")))
		fdev->track_on_err = true;
	else {
		pr_err("Unrecognized command: %s\n", argv[1]);
		status = -EINVAL;
		goto done;
	}

done:
	return status;
}

static void *fpm_hist_proc_start(struct seq_file *seq, loff_t *pos)
{
	struct fpmdev *fdev = fpm;
	struct fpm_tok_op *op;
	unsigned long flags;

	if (*pos == 0)
		seq_puts(seq, "===============Token History==============\n");
	spin_lock_irqsave(&fdev->tok_hist_lock, flags);
	op = &fdev->tok_hist_tail[*pos];
	if (op >= fdev->tok_hist_end)
		op = &fdev->tok_hist_start[op - fdev->tok_hist_end];
	if (op == fdev->tok_hist_head || *pos >= FPM_NUM_HISTORY_ENTRIES)
		op = NULL;
	spin_unlock_irqrestore(&fdev->tok_hist_lock, flags);

	return op;
}

static void fpm_hist_proc_stop(struct seq_file *seq, void *v)
{
	if (!v)
		seq_puts(seq, "==========================================\n");
}

static void *fpm_hist_proc_next(struct seq_file *seq, void *v,
				loff_t *pos)
{
	struct fpmdev *fdev = fpm;
	struct fpm_tok_op *op = v;
	unsigned long flags;

	(*pos)++;
	op++;
	if (op >= fdev->tok_hist_end)
		op = fdev->tok_hist_start;
	spin_lock_irqsave(&fdev->tok_hist_lock, flags);
	if (op == fdev->tok_hist_head || *pos >= FPM_NUM_HISTORY_ENTRIES)
		op = NULL;
	spin_unlock_irqrestore(&fdev->tok_hist_lock, flags);

	return op;
}

static int fpm_hist_proc_show(struct seq_file *seq, void *v)
{
	struct fpm_tok_op *op = v;

	fpm_dump_hist_entry(op, seq);
	return 0;
}

static const struct seq_operations hist_seq_ops = {
	.start	= fpm_hist_proc_start,
	.stop	= fpm_hist_proc_stop,
	.next	= fpm_hist_proc_next,
	.show	= fpm_hist_proc_show,
};
static int fpm_hist_proc_open(struct inode *inode, struct file *file)
{
	int fid;

	fid = seq_open(file, &hist_seq_ops);
	return fid;
}
static const struct file_operations hist_fops = {
	.owner		= THIS_MODULE,
	.open		= fpm_hist_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
#endif

static struct proc_cmd_ops command_entries[] = {
	PROC_CMD_INIT("help", fpm_proc_cmd_help),
	PROC_CMD_INIT("set", fpm_proc_cmd_set),
	PROC_CMD_INIT("get", fpm_proc_cmd_get),
	PROC_CMD_INIT("alloc", fpm_proc_cmd_alloc),
	PROC_CMD_INIT("free", fpm_proc_cmd_free),
	PROC_CMD_INIT("xon_xoff", fpm_proc_cmd_xon_xoff),
#ifdef CONFIG_BCM_FPM_TOKEN_HIST
	PROC_CMD_INIT("hist", fpm_proc_cmd_hist),
#endif
	PROC_CMD_INIT("show", fpm_proc_cmd_show),
};

struct proc_cmd_table fpm_command_table = {
	.module_name = "FPM",
	.size = ARRAY_SIZE(command_entries),
	.ops = command_entries
};

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *head_proc_file;
static struct proc_dir_entry *tail_proc_file;
static struct proc_dir_entry *pkt_proc_file;
static struct proc_dir_entry *hp_proc_file;
static struct proc_dir_entry *hpt_proc_file;
static struct proc_dir_entry *hist_proc_file;
static struct proc_dir_entry *cmd_proc_file;
static struct proc_dir_entry *stats_proc_file;

static int panic_callback(struct notifier_block *self,
			  unsigned long event, void *ctx)
{
	int pool;
	struct timespec64 now;
	struct tm tm_val;

	ktime_get_real_ts64(&now);
	time64_to_tm(now.tv_sec, 0, &tm_val);

	pr_emerg("---[ FPM dump: Start %d/%d/%ld %02d:%02d:%02d ]---\n",
		 tm_val.tm_mon + 1, tm_val.tm_mday, 1900 + tm_val.tm_year,
		 tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec);
	fpm_stats_hw_show(NULL);
	for (pool = 0; pool < fpm->npools; pool++)
		fpm_stats_pool_show(NULL, pool);
	pr_emerg("---[ FPM dump: End ]---\n");
	return NOTIFY_OK;
}

static struct notifier_block nb_panic = {
	.notifier_call  = panic_callback,
};

void fpm_proc_exit(void)
{
	struct tokens_list *t_list, *tmp;

	list_for_each_entry_safe(t_list, tmp, &tokens_lh, list) {
		list_del(&t_list->list);
		kfree(t_list->tokens);
		kfree(t_list);
	}
	if (cmd_proc_file) {
		remove_proc_entry(CMD_PROC_FILE, proc_dir);
		cmd_proc_file = NULL;
	}
	if (head_proc_file) {
		remove_proc_entry(HEAD_PROC_FILE, proc_dir);
		head_proc_file = NULL;
	}
	if (tail_proc_file) {
		remove_proc_entry(TAIL_PROC_FILE, proc_dir);
		tail_proc_file = NULL;
	}
	if (pkt_proc_file) {
		remove_proc_entry(PKT_PROC_FILE, proc_dir);
		pkt_proc_file = NULL;
	}
	if (hp_proc_file) {
		remove_proc_entry(HP_PROC_FILE, proc_dir);
		hp_proc_file = NULL;
	}
	if (hpt_proc_file) {
		remove_proc_entry(HPT_PROC_FILE, proc_dir);
		hpt_proc_file = NULL;
	}
	if (hist_proc_file) {
		remove_proc_entry(HIST_PROC_FILE, proc_dir);
		hist_proc_file = NULL;
	}
	if (proc_dir) {
		remove_proc_entry(PROC_DIR, NULL);
		proc_dir = NULL;
	}
	atomic_notifier_chain_unregister(&panic_notifier_list,
					 &nb_panic);
}

int fpm_proc_init(void)
{
	int status = 0;

	head_size = fpm->net_buf_head_pad;
	tail_size = fpm->net_buf_tail_pad;

	INIT_LIST_HEAD(&tokens_lh);

	proc_dir = proc_mkdir(PROC_DIR, NULL);
	if (!proc_dir) {
		pr_err("Failed to create PROC directory %s.\n",
		       PROC_DIR);
		status = -EIO;
		goto done;
	}
	head_proc_file = proc_create(HEAD_PROC_FILE, 0444,
				     proc_dir, &head_fops);
	if (!head_proc_file) {
		pr_err("Failed to create %s\n", HEAD_PROC_FILE);
		status = -EIO;
		fpm_proc_exit();
		goto done;
	}
	tail_proc_file = proc_create(TAIL_PROC_FILE, 0444,
				     proc_dir, &tail_fops);
	if (!tail_proc_file) {
		pr_err("Failed to create %s\n", TAIL_PROC_FILE);
		status = -EIO;
		fpm_proc_exit();
		goto done;
	}
	pkt_proc_file = proc_create(PKT_PROC_FILE, 0444,
				    proc_dir, &pkt_fops);
	if (!pkt_proc_file) {
		pr_err("Failed to create %s\n", PKT_PROC_FILE);
		status = -EIO;
		fpm_proc_exit();
		goto done;
	}
	hp_proc_file = proc_create(HP_PROC_FILE, 0444,
				   proc_dir, &hp_fops);
	if (!hp_proc_file) {
		pr_err("Failed to create %s\n", HP_PROC_FILE);
		status = -EIO;
		fpm_proc_exit();
		goto done;
	}
	hpt_proc_file = proc_create(HPT_PROC_FILE, 0444,
				    proc_dir, &hpt_fops);
	if (!hpt_proc_file) {
		pr_err("Failed to create %s\n", HPT_PROC_FILE);
		status = -EIO;
		fpm_proc_exit();
		goto done;
	}
	stats_proc_file = proc_create(STATS_PROC_FILE, 0444,
					proc_dir, &stats_fops);
	if (!stats_proc_file) {
		pr_err("Failed to create %s\n", STATS_PROC_FILE);
		status = -EIO;
		fpm_proc_exit();
		goto done;
	}
#ifdef CONFIG_BCM_FPM_TOKEN_HIST
	hist_proc_file = proc_create(HIST_PROC_FILE, 0444,
				    proc_dir, &hist_fops);
	if (!hist_proc_file) {
		pr_err("Failed to create %s\n", HIST_PROC_FILE);
		status = -EIO;
		fpm_proc_exit();
		goto done;
	}
#endif
	cmd_proc_file = proc_create_cmd(CMD_PROC_FILE, proc_dir,
					&fpm_command_table);
	if (!cmd_proc_file) {
		pr_err("Failed to create %s\n", CMD_PROC_FILE);
		status = -EIO;
		fpm_proc_exit();
		goto done;
	}
	atomic_notifier_chain_register(&panic_notifier_list,
				       &nb_panic);

done:
	return status;
}
