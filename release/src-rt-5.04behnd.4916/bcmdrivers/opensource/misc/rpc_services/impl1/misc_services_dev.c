 /****************************************************************************
 *
 * Broadcom Proprietary and Confidential. (c) 2017 Broadcom.  All rights reserved.
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
 ****************************************************************************/
 /*
 * misc_services_dev.c
 * Dec 11 2015
 * Peter Sulc
 *
 * support the misc_services device mrpc0
 *
 *******************************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/reboot.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>

#include "misc_services.h"

int misc_file_open(struct inode *inode, struct file *file)
{
	file->private_data = 0;
	return 0;
}

int misc_file_release(struct inode *inode, struct file *file)
{
	return 0;
}

ssize_t misc_file_read(struct file *file,
		       char __user *buf,
		       size_t count,
		       loff_t *ppos)
{
	return 0;
}

/* return number to increment buf if match or 0 */
static int matchstr(const char *buf, const char *match)
{
	int i = 0;

	while (buf[i] == match[i] && buf[i])
		i++;

	if (match[i] == 0) { /* success */
		/* skip to next char or null */
		while (buf[i] && buf[i] <= ' ')
			i++;
		return i;
	}
	return 0;
}

static int send_boot_success_to_ecos(int dqm_tunnel, enum partition_type ptype)
{
	rpc_msg	msg;
	rpc_msg_init(&msg, RPC_SERVICE_MISC,
		     LINUX_STATE, 0, LS_BOOT_SUCCESS, ptype, 0);
	return  rpc_send_message(dqm_tunnel, &msg, true);
}

const char *parse_next_u32(const char *buf, u32 *val)
{
	u32 n = 0;

	while (*buf && *buf == ' ') buf++;
	if (*buf == 0) {
		pr_err("%s %d Parse error\n", __func__, __LINE__);
		return NULL;
	}
	while (*buf > ' ') {
		if (*buf >= '0' && *buf <= '9') {
			n *= 10;
			n += *buf - '0';
		}
		buf++;
	}
	*val = n;

	return buf;
}

void misc_service_test(int dqm_tunnel, const char *buf)
{
	rpc_msg	msg;
	u32 service, func, a1 = 0, a2 = 0, a3 = 0;

	buf = parse_next_u32(buf, &service);
	if (!buf) {
		pr_err("%s %d Parse error\n", __func__, __LINE__);
		return;
	}
	buf = parse_next_u32(buf, &func);
	if (!buf) {
		pr_err("%s %d Parse error\n", __func__, __LINE__);
		return;
	}
	buf = parse_next_u32(buf, &a1);
	if (buf)
		buf = parse_next_u32(buf, &a2);
	if (buf)
		buf = parse_next_u32(buf, &a3);

	rpc_msg_init(&msg, service, func, 0, a1, a2, a3);
	pr_info("sending %d %d %d %d %d\n", service, func, a1, a2, a3);
	rpc_send_message(dqm_tunnel, &msg, true);
}


int misc_service_testreq(int dqm_tunnel, const char *buf)
{
	rpc_msg	msg;
	u32 service, func, a1 = 0, a2 = 0, a3 = 0;

	buf = parse_next_u32(buf, &service);
	if (!buf) {
		pr_err("%s %d Parse error\n", __func__, __LINE__);
		return -1;
	}
	buf = parse_next_u32(buf, &func);
	if (!buf) {
		pr_err("%s %d Parse error\n", __func__, __LINE__);
		return -1;
	}
	buf = parse_next_u32(buf, &a1);
	if (buf)
		buf = parse_next_u32(buf, &a2);
	if (buf)
		buf = parse_next_u32(buf, &a3);

	rpc_msg_init(&msg, service, func, 0, a1, a2, a3);
	pr_debug("sending req %d %d %d %d %d\n", service, func, a1, a2, a3);
	return rpc_send_request_timeout(dqm_tunnel, &msg, 10);
}

ssize_t misc_file_write(struct file *file,
			const char __user *buf,
			size_t count,
			loff_t *ppos)
{
	char buffer[128];
	int cnt = sizeof(buffer) - 1;
	int i, len, dqm_tunnel, ret;

	if (count < cnt)
		cnt = count;

	if (copy_from_user(buffer, buf, cnt))
		return -EFAULT;
	buffer[cnt] = 0;

	i = 0;
	if (i > cnt) {
		pr_err("\nstring index > %d", cnt);
		return cnt;
	}
	len = matchstr(&buffer[i], "rg-cm");
	if (len) {
		i += len;
		dqm_tunnel = rpc_get_fifo_tunnel_id("rg-cm");
	}
	else {
		pr_err("\nUsage: <tunnel> <function> <arg>");
		pr_err("\nexpecting tunnel rg-cm");
		return cnt;
	}
    if (i > cnt) {
        pr_err("\nstring index > %d", cnt);
        return cnt;
    }
	len = matchstr(&buffer[i], "boot-success");
	if (len) {
		i += len;
	}
	else {
		len = matchstr(&buffer[i], "testreq");
		if (len) {
			i += len;
            if (i > cnt) {
                pr_err("\nstring index > %d", cnt);
                return cnt;
            }
			ret = misc_service_testreq(dqm_tunnel, &buffer[i]);
			if (!ret)
				return cnt;
			else
				return ret;
		}
		len = matchstr(&buffer[i], "test");
		if (len) {
			i += len;
            if (i > cnt) {
                pr_err("\nstring index > %d", cnt);
                return cnt;
            }
			misc_service_test(dqm_tunnel, &buffer[i]);
			return cnt;
		}
		else {
			pr_err("\nUsage: <tunnel> <function> <arg>");
			pr_err("\nexpecting function boot-success");
			return cnt;
		}
	}
    if (i > cnt) {
        pr_err("\nstring index > %d", cnt);
        return cnt;
    }
	len = matchstr(&buffer[i], "Linux");
	if (len) {
		i += len;
		send_boot_success_to_ecos(dqm_tunnel, PartLinux);
	}
	else if ((len = matchstr(&buffer[i], "Appsfs"))) {
		i += len;
		send_boot_success_to_ecos(dqm_tunnel, PartAppsfs);
	}
	else if ((len = matchstr(&buffer[i], "Rootfs"))) {
		i += len;
		send_boot_success_to_ecos(dqm_tunnel, PartRootfs);
	}
	else if ((len = matchstr(&buffer[i], "RGNV"))) {
		i += len;
		send_boot_success_to_ecos(dqm_tunnel, PartRGNV);
	}
	else {
		pr_err("\nUsage: <tunnel> <function> <arg>");
		pr_err("\nexpecting arg Linux or Appsfs or Rootfs or RGNV");
		return cnt;
	}
	return cnt;
}
