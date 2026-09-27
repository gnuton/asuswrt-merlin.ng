/*
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
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
 *
 */

#include <linux/cpufreq.h>
#include <linux/module.h>
#include <linux/netstandby.h>

struct soc_standby_ctx {
	char *old_gov;
	netstandby_event_compl_cb_t enter_cmp_cb;
	netstandby_event_compl_cb_t exit_cmp_cb;
	netstandby_trigger_notif_cb_t trigger_cb;
};

struct soc_standby_ctx soc_ctx;

int soc_enter_standby(void *app_data, struct netstandby_entry_info *entry_info)
{
	struct soc_standby_ctx *ctx = (struct soc_standby_ctx *)app_data;

	ctx->old_gov = cpufreq_enter_standby();
	if (IS_ERR(ctx->old_gov)) {
		pr_err("%s: CPU frequency to enter standby failed\n",__func__);
		return PTR_ERR(ctx->old_gov);
	}

	if (ctx->enter_cmp_cb) {
		struct netstandby_event_compl_info event_info = {0};
		event_info.system_type = NETSTANDBY_SUBSYSTEM_TYPE_PLATFORM;
		event_info.event_type = NETSTANDBY_NOTIF_ENTER_COMPLETE;
		ctx->enter_cmp_cb(app_data, &event_info);
	}

	return 0;
}

int soc_exit_standby(void *app_data, struct netstandby_exit_info *exit_info)
{
	struct soc_standby_ctx *ctx = (struct soc_standby_ctx *)app_data;
	int ret;

	ret = cpufreq_exit_standby(ctx->old_gov);
	if (ret < 0) {
		pr_err("%s: CPU frequency to exit standby failed\n",__func__);
		return ret;
	}

	if (ctx->exit_cmp_cb) {
		struct netstandby_event_compl_info event_info = {0};
		event_info.system_type = NETSTANDBY_SUBSYSTEM_TYPE_PLATFORM;
		event_info.event_type = NETSTANDBY_NOTIF_EXIT_COMPLETE;
		ctx->exit_cmp_cb(app_data, &event_info);
	}

	return 0;
}

int netstandby_platform_get_and_register_cb(struct netstandby_reg_info *info)
{
	info->enter_cb = soc_enter_standby;
	info->exit_cb = soc_exit_standby;
	info->app_data = &soc_ctx;

	soc_ctx.enter_cmp_cb = info->enter_cmp_cb;
	soc_ctx.exit_cmp_cb = info->exit_cmp_cb;
	soc_ctx.trigger_cb = info->trigger_cb;

	return 0;
}
EXPORT_SYMBOL(netstandby_platform_get_and_register_cb);

MODULE_LICENSE("Dual BSD/GPL v2");
MODULE_DESCRIPTION("Network Standby Platform API driver");
