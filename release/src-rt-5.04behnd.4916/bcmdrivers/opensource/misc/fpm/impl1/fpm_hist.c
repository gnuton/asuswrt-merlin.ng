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
#include <linux/if_ether.h>

#include "fpm_dev.h"
#include "fpm_priv.h"

void fpm_track_token_rx(u32 token)
{
	struct fpmdev *fdev = fpm;
	u32 tok_idx;

	if (!fdev->track_tokens)
		return;

	fpm_check_token(token);
	fpm_track_token_op(fpm_track_token_rx, token, 0);
	tok_idx = __fpm_get_token_index(token);
	tok_idx |= (__fpm_get_token_ddrpool(token) << __fpm_token_max_index_bits());
	if (fdev->tok_ref_count[tok_idx]) {
		pr_err("RX token (0x%08x) but internal ref count (%d) != 0\n",
		       token, fdev->tok_ref_count[tok_idx]);
		fdev->track_tokens = fdev->track_on_err;
	}
	fdev->tok_ref_count[tok_idx]++;
}
EXPORT_SYMBOL(fpm_track_token_rx);

void fpm_track_token_src(u32 token, union tok_src_dest *src)
{
	struct fpmdev *fdev = fpm;

	if (!fdev->track_tokens)
		return;
	if (!src) {
		pr_err("%s: Received void ptr for token op src data.\n",
		       __func__);
		return;
	}

	fpm_check_token(token);
	fpm_track_token_op(fpm_track_token_src, token, src->data);
}
EXPORT_SYMBOL(fpm_track_token_src);

void fpm_track_token_tx(u32 token)
{
	struct fpmdev *fdev = fpm;
	u32 tok_idx;

	if (!fdev->track_tokens)
		return;

	fpm_check_token(token);
	fpm_track_token_op(fpm_track_token_tx, token, 0);
	tok_idx = __fpm_get_token_index(token);
	tok_idx |= (__fpm_get_token_ddrpool(token) << __fpm_token_max_index_bits());
	fdev->tok_ref_count[tok_idx]--;
	/*
	 * The ref count should go to 0 when all TX's for this FPM
	 * have occurred. If the multi-count was incremented somewhere
	 * in our drivers we don't know if this is the last TX or not
	 * so we can't check for 0 here.
	 *
	 * TODO: Start timer here and check ref count went to 0 at timeout
	 * when all TX's should have occurred.
	 */
}
EXPORT_SYMBOL(fpm_track_token_tx);

void fpm_track_token_dest(u32 token, union tok_src_dest *dest)
{
	struct fpmdev *fdev = fpm;

	if (!fdev->track_tokens)
		return;
	if (!dest) {
		pr_err("%s: Received void ptr for token op dest data.\n",
		       __func__);
		return;
	}

	fpm_check_token(token);
	fpm_track_token_op(fpm_track_token_dest, token, dest->data);
}
EXPORT_SYMBOL(fpm_track_token_dest);

void fpm_dump_token_hist(u32 token)
{
	struct fpmdev *fdev = fpm;
	struct fpm_tok_op *op;
	int skipped;

	pr_info("===============Token 0x%08x History==============\n", token);
	op = fdev->tok_hist_tail;
	skipped = 0;
	while (op != fdev->tok_hist_head) {
		if (op->token == token) {
			if (skipped) {
				pr_info("%d non-matching entries\n", skipped);
				pr_info("-----------------------------------");
				pr_cont("-------\n");
				skipped = 0;
			}
			fpm_dump_hist_entry(op, NULL);
		} else {
			skipped++;
		}
		op++;
		if (op >= fdev->tok_hist_end)
			op = fdev->tok_hist_start;
	}
	pr_info("%d non-matching entries\n", skipped);
	pr_info("------------------------------------------\n");
	pr_info("==========================================\n");
}
EXPORT_SYMBOL(fpm_dump_token_hist);

void fpm_dump_hist(u32 entries)
{
	struct fpmdev *fdev = fpm;
	struct fpm_tok_op *op;
	int count;
	unsigned long flags;

	pr_err("===============Token History==============\n");
	spin_lock_irqsave(&fdev->tok_hist_lock, flags);
	count = fdev->tok_hist_head - fdev->tok_hist_tail;
	if (count < 0)
		count = fdev->tok_hist_tail - fdev->tok_hist_head;
	if (entries >= count) {
		op = fdev->tok_hist_tail;
	} else {
		op = fdev->tok_hist_head - entries;
		if (op < fdev->tok_hist_start)
			op = fdev->tok_hist_end - (fdev->tok_hist_start - op);
	}
	spin_unlock_irqrestore(&fdev->tok_hist_lock, flags);

	while (op != fdev->tok_hist_head) {
		fpm_dump_hist_entry(op, NULL);
		op++;
		if (op >= fdev->tok_hist_end)
			op = fdev->tok_hist_start;
	}
	pr_err("==========================================\n");
}
EXPORT_SYMBOL(fpm_dump_hist);
