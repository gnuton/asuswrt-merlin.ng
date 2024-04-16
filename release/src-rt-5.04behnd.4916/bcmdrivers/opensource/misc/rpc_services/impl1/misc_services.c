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
 * misc_services.c
 * Jun 17 2013
 * Tim Ross
 * Peter Sulc
 *
 ******************************************************************************/

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
#include "fpm.h"

/* creat a user space device for sending messages */
#define MISC_CLASS	"mrpc"
#define MISC_DEVNAME	"mrpc%d"
#define MISC_MAX_DEVS	1

static struct class		*misc_class;
static int			misc_major;
static dev_t			misc_dev;
static struct cdev		misc_cdev;
static struct device		*misc_device;

static const struct file_operations misc_fops = {
	.owner =		THIS_MODULE,
	.open =			misc_file_open,
	.release =		misc_file_release,
	.read =			misc_file_read,
	.write =		misc_file_write,
};

static int rpc_net_message(int dqm_tunnel, struct misc_msg *msg);
static int linux_passwd(int dqm_tunnel, struct misc_msg *msg);
static int linux_time(int dqm_tunnel, struct misc_msg *msg);
static int linux_state(int dqm_tunnel, struct misc_msg *msg);

static int linux_version_dummy(int dqm_tunnel, struct misc_msg *msg);
static int linux_app_version_dummy(int dqm_tunnel, struct misc_msg *msg);
static int linux_time_dummy(int dqm_tunnel, struct misc_msg *msg);
static int loopback_test_dummy(int dqm_tunnel, struct misc_msg *msg);
static int xpt_demod_access_dummy(int dqm_tunnel, struct misc_msg *msg);

static rpc_function misc_services_table[] =
{
	{ (rpc_rx_handler)linux_passwd,			0 },
	{ (rpc_rx_handler)linux_time,			0 },
	{ (rpc_rx_handler)rpc_net_message,		0 },
	{ (rpc_rx_handler)pwm_ctrl,			0 },
	{ (rpc_rx_handler)linux_state,			0 },
	{ (rpc_rx_handler)cm_power_ctrl,		0 },
	{ (rpc_rx_handler)rg_power_config,		0 },
	{ (rpc_rx_handler)xpt_demod_access_dummy,	0 },
	{ (rpc_rx_handler)linux_version_dummy,		0 },
	{ (rpc_rx_handler)linux_app_version_dummy,	0 },
	{ (rpc_rx_handler)linux_time_dummy,		0 },
	{ (rpc_rx_handler)loopback_test_dummy,		0 },
	{ (rpc_rx_handler)cm_temperature_state,		0 },
};

int __init misc_services_init(void)
{
	int status = 0;

	status = rpc_register_functions(RPC_SERVICE_MISC,
					misc_services_table,
					sizeof(misc_services_table)/
					sizeof(rpc_function));

	misc_class = class_create(THIS_MODULE, MISC_CLASS);
	if (IS_ERR(misc_class)) {
		pr_err("class_create() failed for misc_class\n");
		return PTR_ERR(misc_class);
	}
	status = alloc_chrdev_region(&misc_dev, 0, MISC_MAX_DEVS, MISC_CLASS);
	misc_major = MAJOR(misc_dev);
	if (status < 0) {
		pr_err("%s: can't alloc chrdev region\n", __func__);
		class_destroy(misc_class);
		return status;
	}
	cdev_init(&misc_cdev, &misc_fops);
	status = cdev_add(&misc_cdev, misc_dev, 1);
	if (status < 0) {
		pr_err("can't register major %d\n", misc_major);
		return status;
	}
	misc_device = device_create(misc_class, NULL, MKDEV(misc_major, 0),
				    NULL, MISC_DEVNAME, 0);
	if (IS_ERR(misc_device)) {
		pr_err("%s: can't register class device\n", __func__);
		misc_device = NULL;
		return PTR_ERR(misc_device);
	}
	return status;
}

void misc_services_cleanup(void)
{
	if (misc_class)
		class_destroy(misc_class);

	if (misc_major)
		unregister_chrdev_region(MKDEV(misc_major, 0), MISC_MAX_DEVS);
}

static char password[128];

static int xpt_demod_access_dummy(int dqm_tunnel, struct misc_msg *msg)
{
	return 0;
}

static int linux_version_dummy(int dqm_tunnel, struct misc_msg *msg)
{
	return 0;
}

static int linux_app_version_dummy(int dqm_tunnel, struct misc_msg *msg)
{
	return 0;
}

static int linux_time_dummy(int dqm_tunnel, struct misc_msg *msg)
{
	return 0;
}

static int loopback_test_dummy(int dqm_tunnel, struct misc_msg *msg)
{
	return 0;
}

static int linux_passwd(int dqm_tunnel, struct misc_msg *msg)
{
	uint8_t *buf;
	union tok_src_dest tok_op_data;

	if (msg->type == PASSWD_SET)
	{
		if (unlikely(!fpm_is_valid_token(msg->data))) {
			pr_err("%s: Invalid token 0x%08x.\n", __func__,
			       msg->data);
			msg->data = -EINVAL;
			goto done;
		}
		fpm_track_token_rx(msg->data);
		tok_op_data.data = 0;
		tok_op_data.rpc_hdr = msg->dqm_header;
		fpm_track_token_src(msg->data, &tok_op_data);
		fpm_invalidate_token(msg->data, 0, 0, 0);
		buf = fpm_token_to_buffer(msg->data);
		if (unlikely(!buf)) {
			pr_err("%s (%d): Unable to translate token 0x%08x to "
			       "buffer.\n", __func__, msg->type, msg->data);
			msg->data = -EINVAL;
			goto done;
		}
		strncpy(password, buf, sizeof(password)-1);
		password[sizeof(password)-1] = 0;
		fpm_invalidate_token(msg->data, 0, 0, 0);
		fpm_free_token(msg->data);
		pr_info("Got Linux password (0x%08x, %s).\n", msg->data, password);
	}
	else if (msg->type == PASSWD_GET)
	{
		msg->data = fpm_alloc_token(sizeof(password));
		if (unlikely(!msg->data)) {
			pr_err("%s (%d): Unable to alloc token.\n",
			       __func__, msg->type);
			msg->data = -ENOMEM;
			goto done;
		}
		fpm_set_token_size(&msg->data, sizeof(password));
		buf = fpm_token_to_buffer(msg->data);
		memcpy(buf, &password[0], sizeof(password));
		tok_op_data.data = 0;
		tok_op_data.rpc_hdr = msg->dqm_header;
		fpm_track_token_dest(msg->data, &tok_op_data);
		fpm_track_token_tx(msg->data);
		fpm_flush_invalidate_token(msg->data, 0, 0, 0);
		pr_info("Sent Linux password (0x%08x, %s).\n", msg->data, password);
	}
	else
	{
		pr_err("%s Unidentified message type %d\n", __func__, msg->type);
		msg->data = -1;
	}

done:
	if (rpc_msg_request((rpc_msg *)msg))
		rpc_send_reply(dqm_tunnel, (rpc_msg *)msg);

	return 0;
}

static int linux_time(int dqm_tunnel, struct misc_msg *msg)
{
	if (msg->type == TIME_SET)
	{
		struct timespec64 tv;
		struct timezone tz;
		int error = 0;

		pr_info("Got UTC time.\n");
		tv.tv_sec = msg->data;
		tv.tv_nsec = 0;
		tz.tz_minuteswest = ((int)(msg->extra))/(-60);
		tz.tz_dsttime = 0;

		error = do_sys_settimeofday64(&tv, &tz);
		if(error)
		{
			pr_err("Got error %d from do_sys_settimeofday()!!!\n", error);
		}
	}
	else
	{
		pr_err("%s Unidentified message type %d\n", __func__, msg->type);
		msg->data = -1;
	}
	if (rpc_msg_request((rpc_msg *)msg))
		rpc_send_reply(dqm_tunnel, (rpc_msg *)msg);

	return 0;
}

static int linux_state(int dqm_tunnel, struct misc_msg *msg)
{
	if (msg->type == LS_HALTED)
		kernel_halt();
	else if (msg->type == LS_RESTART)
		kernel_restart(NULL);
	else if (msg->type == LS_PM_POWEROFF)
		kernel_power_off();
	else {
		pr_err("%s Unidentified message type %d\n", __func__, msg->type);
		msg->data = -1;
	}
	if (rpc_msg_request((rpc_msg *)msg))
		rpc_send_reply(dqm_tunnel, (rpc_msg *)msg);

	return 0;
}

/*********************************************************
 * functions to access misc_services
 ********************************************************/

const char *get_linux_password(void)
{
	return password;
}
EXPORT_SYMBOL(get_linux_password);

int send_netif_state(int dqm_tunnel, char *if_name,
		     enum netif_op op)
{
	struct misc_msg msg;
	u32 token;
	u8 *buf;
	int len;
	int status = 0;
	union tok_src_dest tok_dest;

	if (!if_name) {
		status = -EINVAL;
		goto done;
	}

	if (op != IF_UP && op != IF_DOWN) {
		status = -EINVAL;
		goto done;
	}

	len = strlen(if_name) + 1;
	token = fpm_alloc_token(len);
	if (unlikely(!token)) {
		pr_err("%s: Unable to alloc token, if_name = %s.\n",
		       __func__, if_name);
		status = -ENOMEM;
		goto done;
	}
	fpm_set_token_size(&token, len);
	buf = fpm_token_to_buffer(token);
	memcpy(buf, if_name, len);
	tok_dest.data = 0;
	fpm_track_token_dest(token, &tok_dest);
	fpm_track_token_tx(token);
	fpm_flush_invalidate_token(token, 0, 0, 0);
	rpc_msg_init((rpc_msg *)&msg, RPC_SERVICE_MISC, NETIF, 0, op, token, 0);
	tok_dest.rpc_hdr = msg.dqm_header;
	status = rpc_send_message(dqm_tunnel, (rpc_msg *)&msg, false);

done:
	return status;
}
EXPORT_SYMBOL(send_netif_state);

int get_netif_stats(int dqm_tunnel, char *if_name,
		    struct dqnet_dev_stats *stats)
{
	struct misc_msg msg;
	u32 token;
	u8 *buf;
	int len;
	int status = 0;
	union tok_src_dest tok_op_data;

	if (!if_name || !stats) {
		status = -EINVAL;
		goto done;
	}

	len = strlen(if_name) + 1;
	token = fpm_alloc_token(len);
	if (unlikely(!token)) {
		pr_err("%s: Unable to alloc token, if_name = %s.\n", __func__,
		       if_name);
		status = -ENOMEM;
		goto done;
	}
	fpm_set_token_size(&token, len);
	buf = fpm_token_to_buffer(token);
	memcpy(buf, if_name, len);
	tok_op_data.data = 0;
	fpm_track_token_dest(token, &tok_op_data);
	fpm_track_token_tx(token);
	fpm_flush_invalidate_token(token, 0, 0, 0);
	rpc_msg_init((rpc_msg *)&msg, RPC_SERVICE_MISC, NETIF, 0,
		     IF_STATS, token, 0);
	tok_op_data.rpc_hdr = msg.dqm_header;
	status = rpc_send_request(dqm_tunnel, (rpc_msg *)&msg);
	if (status)
		goto done;
	status = msg.extra;
	if (status)
		goto done;
	token = msg.data;
	if (unlikely(!fpm_is_valid_token(token))) {
		pr_err("%s: Invalid token 0x%08x.\n", __func__, token);
		status = -EINVAL;
		goto done;
	}
	fpm_track_token_rx(token);
	tok_op_data.rpc_hdr = msg.dqm_header;
	fpm_track_token_src(token, &tok_op_data);
	fpm_invalidate_token(token, 0, 0, 0);
	buf = fpm_token_to_buffer(token);
	memcpy((u8 *)stats, buf, sizeof(*stats));
	fpm_invalidate_token(token, 0, 0, 0);
	fpm_free_token(token);

done:
	return status;
}
EXPORT_SYMBOL(get_netif_stats);

int (*rpc_net_handler)(int dqm_tunnel, struct misc_msg *msg);
EXPORT_SYMBOL(rpc_net_handler);

static int rpc_net_message(int dqm_tunnel, struct misc_msg *msg)
{
	union tok_src_dest tok_src;

	if (rpc_net_handler)
		return rpc_net_handler(dqm_tunnel, msg);
	else {
		char *name;
		fpm_track_token_rx(msg->data);
		tok_src.data = 0;
		tok_src.rpc_hdr = msg->dqm_header;
		fpm_track_token_src(msg->data, &tok_src);
		name = (char *)fpm_token_to_buffer(msg->data);
		if (unlikely(!name)) {
			pr_err("%s: handler not initialized - rcvd message type %d with"
			       "uninitialized name buffer\n",
			       __func__, msg->type);
		} else {
			pr_err("%s: handler not initialized - rcvd message type %d for %s\n",
			       __func__, msg->type, name);
		}
		fpm_invalidate_buffer(name, strlen(name));
		fpm_free_buffer((u8 *)name);
		return 0;
	}
}

static inline void send_linux_state_message_to_ecos(enum linux_state state)
{
	linux_send_state_msg(rpc_get_fifo_tunnel_id("rg-cm"), state);
}

void send_halted_msg_to_ecos(void)
{
	send_linux_state_message_to_ecos(LS_HALTED);
}

void send_restart_msg_to_ecos(void)
{
	send_linux_state_message_to_ecos(LS_RESTART);
}

void send_poweroff_msg_to_ecos(void)
{
	send_linux_state_message_to_ecos(LS_PM_POWEROFF);
}

postcore_initcall_sync(misc_services_init);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("misc services driver");
