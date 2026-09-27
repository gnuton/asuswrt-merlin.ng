/*
 * Copyright (c) 2019, 2021 The Linux Foundation. All rights reserved.
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
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm_wakeup.h>
#include <linux/rwsem.h>
#include <linux/suspend.h>
#include <linux/platform_device.h>
#include <linux/remoteproc.h>
#include <linux/debugfs.h>

#include "../../remoteproc/remoteproc_internal.h"
/*
 * To call rproc boot and shutdown of WCSS alone,
 * 1) insmod testssr.ko (test_id by default is set to 1)
 * 2) rmmod testssr
 * In case if multipd architectire used in WCSS/QDSP6
 * 1)insmod testssr.ko test_id=1 mpd=1
 * 2)rmmod testssr.ko
 */

static struct notifier_block nb;
static struct notifier_block atomic_nb;

#define WCSS_RPROC	"cd00000.remoteproc"
static int test_id = 1;
module_param(test_id, int, S_IRUGO | S_IWUSR | S_IWGRP);
static ulong mpd;
module_param(mpd, ulong, S_IRUGO | S_IWUSR | S_IWGRP);

#define MAX_USERPD_CNT	3

struct userpd_info {
	struct rproc *rproc;
	/*0-stop, 1-start*/
	bool pd_curr_state;
};

struct userpd_dbgfs_info {
	struct dentry *userpd;
	/*0-stop, 1-start*/
	struct dentry *start_stop;
	struct userpd_info userpd_hdl;
};

static struct dentry *q6_userpd_debug;
static struct userpd_dbgfs_info *userpd_dbgfs_hdl[MAX_USERPD_CNT];

static const char *action_to_string(enum subsys_notif_type action)
{
	switch (action) {

	case	SUBSYS_BEFORE_SHUTDOWN:
		return __stringify(SUBSYS_BEFORE_SHUTDOWN);

	case	SUBSYS_AFTER_SHUTDOWN:
		return __stringify(SUBSYS_AFTER_SHUTDOWN);

	case	SUBSYS_BEFORE_POWERUP:
		return __stringify(SUBSYS_BEFORE_POWERUP);

	case	SUBSYS_AFTER_POWERUP:
		return __stringify(SUBSYS_AFTER_POWERUP);

	case	SUBSYS_RAMDUMP_NOTIFICATION:
		return __stringify(SUBSYS_RAMDUMP_NOTIFICATION);

	case	SUBSYS_POWERUP_FAILURE:
		return __stringify(SUBSYS_POWERUP_FAILURE);

	case	SUBSYS_PROXY_VOTE:
		return __stringify(SUBSYS_PROXY_VOTE);

	case	SUBSYS_PROXY_UNVOTE:
		return __stringify(SUBSYS_PROXY_UNVOTE);

	case	SUBSYS_SOC_RESET:
		return __stringify(SUBSYS_SOC_RESET);

	case	SUBSYS_PREPARE_FOR_FATAL_SHUTDOWN:
		return __stringify(SUBSYS_PREPARE_FOR_FATAL_SHUTDOWN);

	default:
		return "unknown";
	}
}

static int tssr_notifier(struct notifier_block *nb, unsigned long action,
								void *data)
{
	struct platform_device *pdev = data;

	pr_emerg("%s:\tReceived [%s] notification from [%s]subsystem\n",
		THIS_MODULE->name, action_to_string(action), pdev->name);

	return NOTIFY_DONE;
}

static int test_rproc_notif_register(const char *rproc_name)
{
	int ret;

	ret = rproc_register_subsys_notifier(rproc_name, &nb, &atomic_nb);
	if (ret)
		pr_emerg("rproc notif reg failed\n");

	return ret;
}

#if defined(CONFIG_QCOM_Q6V5_WCSS) || defined(CONFIG_QCOM_Q6V5_MPD)
static struct rproc *get_rproc_from_phandle(void)
{
		struct device_node *of_np;
		phandle rproc_phandle;
		struct rproc *q6rproc;

		of_np = of_find_node_with_property(NULL, "qcom,rproc");
		if (!of_np) {
			pr_err("no node with qcom,rproc NULLi\n");
			return NULL;
		}
		if (of_property_read_u32(of_np, "qcom,rproc", &rproc_phandle)) {
			pr_err("could not get rproc phandle\n");
			return  NULL;
		}
		q6rproc  = rproc_get_by_phandle(rproc_phandle);
		if (!q6rproc) {
			pr_err("could not get the rproc handle\n");
			return NULL;
		}
		return q6rproc;
}
#endif

static ssize_t show_start_stop(struct file *file, char __user *user_buf,
			size_t count, loff_t *ppos)
{
	struct userpd_dbgfs_info *hdl = file->private_data;
	char _buf[16] = {0};
	int ret;

	snprintf(_buf, sizeof(_buf), "%d\n", hdl->userpd_hdl.pd_curr_state);
	ret = simple_read_from_buffer(user_buf, count, ppos, _buf,
						strnlen(_buf, 16));
	return ret;
}

static ssize_t store_start_stop(struct file *file, const char __user *user_buf,
			size_t count, loff_t *ppos)
{
	struct userpd_dbgfs_info *hdl = file->private_data;
	bool state;
	int ret;

	ret = kstrtobool_from_user(user_buf, count, &state);
	if (ret) {
		pr_err("Failed to retrieve userbuf value\n");
		return ret;
	}

	if (state == hdl->userpd_hdl.pd_curr_state)
		return -EINVAL;

	if (state) {
		ret = rproc_boot(hdl->userpd_hdl.rproc);
		if (ret) {
			pr_err("couldn't boot q6v5: %d\n", ret);
			return ret;
		}
		/*userpd started*/
		hdl->userpd_hdl.pd_curr_state = true;
	} else {
		rproc_shutdown(hdl->userpd_hdl.rproc);
		/*userpd stopped*/
		hdl->userpd_hdl.pd_curr_state = false;
	}
	return count;
}

static const struct file_operations userpd_ops = {
	.open = simple_open,
	.write = store_start_stop,
	.read = show_start_stop,
};

static int get_userpd_cnt(struct rproc *q6rproc)
{
	struct device_node *upd_np;
	int cnt = 0;

	for_each_available_child_of_node(q6rproc->dev.parent->of_node, upd_np)
		cnt += strstr(upd_np->name, "pd") ? 1 : 0;

	return cnt;
}

struct rproc *get_userpd_rproc(struct rproc *q6rproc, int pd_asid)
{
	struct device_node *upd_np;
	int cnt = 0;
	struct platform_device *upd_pdev;
	struct rproc *rproc;

	for_each_available_child_of_node(q6rproc->dev.parent->of_node, upd_np) {
		if (!strstr(upd_np->name, "pd"))
			continue;

		if (pd_asid == ++cnt) {
			upd_pdev = of_find_device_by_node(upd_np);
			if (!upd_pdev) {
				pr_info("upd pdev is null\n");
				return NULL;
			}
			rproc = platform_get_drvdata(upd_pdev);
			return rproc;
		}
	}
	return NULL;
}

static int register_userpd_rproc_notif(struct rproc *q6rproc)
{
	struct device_node *upd_np;

	for_each_available_child_of_node(q6rproc->dev.parent->of_node, upd_np) {
		struct platform_device *upd_pdev;
		struct rproc *upd_rproc; int ret;

		if (!strstr(upd_np->name, "pd"))
			continue;

		upd_pdev = of_find_device_by_node(upd_np);
		if (!upd_pdev) {
			pr_info("upd pdev is null\n");
			return -1;
		}
		upd_rproc = platform_get_drvdata(upd_pdev);

		ret = test_rproc_notif_register(upd_rproc->name);
		if (ret)
			return ret;
	}
	return 0;
}

static void unregister_userpd_rproc_notif(struct rproc *q6rproc)
{
	struct device_node *upd_np;

	for_each_available_child_of_node(q6rproc->dev.parent->of_node, upd_np) {
		struct platform_device *upd_pdev;
		struct rproc *upd_rproc;

		if (!strstr(upd_np->name, "pd"))
			continue;

		upd_pdev = of_find_device_by_node(upd_np);
		upd_rproc = platform_get_drvdata(upd_pdev);

		rproc_unregister_subsys_notifier(upd_rproc->name, &nb,
&atomic_nb);
	}
}

static int create_userpd_debugfs(struct rproc *q6rproc)
{
	int ret, cnt, tmp;
	char name[40];

	/*Get userpd count*/
	cnt = get_userpd_cnt(q6rproc);
	if (cnt > MAX_USERPD_CNT) {
		pr_err("Current implementation don't support %d userpd's\n",
			cnt);
		return -EINVAL;
	}

	if (!cnt) {
		pr_err("No userpd is registered\n");
		return -EINVAL;
	}


	/*create a sysfs entry to start/stop registered user pd's*/
	q6_userpd_debug = debugfs_create_dir("q6v5_userpd_debug", NULL);
	if (IS_ERR(q6_userpd_debug)) {
		pr_err("Failed to create q6v5 userpd debug directory\n");
		return PTR_ERR(q6_userpd_debug);
	}

	for (tmp = 0; tmp < cnt; tmp++) {

		userpd_dbgfs_hdl[tmp] = kzalloc(sizeof(*userpd_dbgfs_hdl[tmp]),
				GFP_KERNEL);
		if (!userpd_dbgfs_hdl[tmp]) {
			pr_err("Failed to allocate memory\n");
			ret = PTR_ERR(userpd_dbgfs_hdl[tmp]);
			goto err_free_mem;
		}

		/*get rproc handle*/
		userpd_dbgfs_hdl[tmp]->userpd_hdl.rproc =
			get_userpd_rproc(q6rproc, tmp+1);
		if (!userpd_dbgfs_hdl[tmp]->userpd_hdl.rproc) {
			pr_err("failed to get rproc handle\n");
			return -1;
		}

		snprintf(name, sizeof(name), "q6v5_wcss_userpd%d", (tmp + 1));

		/*sysfs entry for each userpd*/
		userpd_dbgfs_hdl[tmp]->userpd = debugfs_create_dir(name,
				q6_userpd_debug);
		if (IS_ERR(userpd_dbgfs_hdl[tmp]->userpd)) {
			pr_err("Failed to create q6v5 userpd%d debug directory\n",
					(tmp + 1));
			ret = PTR_ERR(userpd_dbgfs_hdl[tmp]->userpd);
			goto err_release_userpd;
		}
		/*sysfs entry for userpd start/stop*/
		userpd_dbgfs_hdl[tmp]->start_stop =
			debugfs_create_file("start_stop",
					0600, userpd_dbgfs_hdl[tmp]->userpd,
					userpd_dbgfs_hdl[tmp],
					&userpd_ops);
		if (IS_ERR(userpd_dbgfs_hdl[tmp]->start_stop)) {
			pr_err("Failed to create q6v5\
					userpd%d start/stop\n", (tmp + 1));
			ret = PTR_ERR(userpd_dbgfs_hdl[tmp]->start_stop);
			goto err_release_userpd_start_stop;
		}

	}
	/* register call back for every userpd*/
	ret = register_userpd_rproc_notif(q6rproc);
	if (ret)
		return ret;
	return 0;

	for (; tmp >= 0; tmp--) {
err_release_userpd_start_stop:
		debugfs_remove(userpd_dbgfs_hdl[tmp]->start_stop);
err_release_userpd:
		debugfs_remove(userpd_dbgfs_hdl[tmp]->userpd);
err_free_mem:
		kfree(userpd_dbgfs_hdl[tmp]);
	}
	debugfs_remove(q6_userpd_debug);
	return ret;
}

static int remove_userpd_debugfs(struct rproc *q6rproc)
{
	int tmp, cnt;

	/*Get userpd count*/
	cnt = get_userpd_cnt(q6rproc);
	if (cnt > MAX_USERPD_CNT) {
		pr_err("Current implementation don't support %d userpd's\n",
			cnt);
		return -EINVAL;
	}

	if (!cnt) {
		pr_err("No userpd is registered\n");
		return -EINVAL;
	}

	/*shutdown any userpd's subsequently rootpd
	 * also will be shutted down
	 */
	for (tmp = 0; tmp < cnt; tmp++) {
		if (userpd_dbgfs_hdl[tmp]->userpd_hdl.pd_curr_state) {
			rproc_shutdown(userpd_dbgfs_hdl[tmp]->
					userpd_hdl.rproc);
		}
	}

	for (tmp = 0; tmp < cnt; tmp++) {
		debugfs_remove(userpd_dbgfs_hdl[tmp]->start_stop);
		debugfs_remove(userpd_dbgfs_hdl[tmp]->userpd);
		kfree(userpd_dbgfs_hdl[tmp]);
	}
	debugfs_remove(q6_userpd_debug);
	unregister_userpd_rproc_notif(q6rproc);
	return 0;
}

static int __init testssr_init(void)
{
	struct rproc *q6rproc;
	int ret;

	nb.notifier_call = tssr_notifier;
	atomic_nb.notifier_call = tssr_notifier;

	switch (test_id) {
	case 1:
		q6rproc = get_rproc_from_phandle();
		if (!q6rproc) {
			pr_err("could not get rproc..\n");
			return -ENODEV;
		}

		if (mpd) {
			ret = create_userpd_debugfs(q6rproc);
			if (ret) {
				pr_err("Failed to create sysfs entry for\
						userpd\n");
				return ret;
			}
			return 0;
		}

		ret = test_rproc_notif_register(q6rproc->name);
		if (ret)
			return ret;

		ret = rproc_boot(q6rproc);
		if (ret) {
			pr_err("couldn't boot q6v5: %d\n", ret);
			ret = rproc_unregister_subsys_notifier(q6rproc->name,
							&nb, &atomic_nb);
			return ret;
		}
		break;
	default:
		pr_err("Enter a valid test case id\n");
	}

	return 0;
}

static void __exit testssr_exit(void)
{
	struct rproc *q6rproc;
	int ret;

	switch (test_id) {
	case 1:
		q6rproc = get_rproc_from_phandle();
		if (!q6rproc) {
			pr_err("could not get rproc..\n");
				return;
		}

		if (mpd) {
			remove_userpd_debugfs(q6rproc);
			return;
		}
		rproc_shutdown(q6rproc);
		ret = rproc_unregister_subsys_notifier(q6rproc->name,
							&nb, &atomic_nb);
		break;
	default:
		break;
	}
}

module_init(testssr_init);
module_exit(testssr_exit);
MODULE_LICENSE("Dual BSD/GPL");
