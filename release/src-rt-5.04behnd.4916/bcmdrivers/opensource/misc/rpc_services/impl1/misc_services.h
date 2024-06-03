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
 * misc_services.h
 * Jun 17 2013
 * Tim Ross
 * Peter Sulc
 *
 *******************************************************************************/
#ifndef _MISC_SERVICES_H_
#define _MISC_SERVICES_H_

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/types.h>

#include <itc_rpc.h>
#include <power_services.h>

#include <linux/types.h>
#include <linux/if_link.h>
/* miscellanious linux control related messages */

/*
 * misc_services are all the various service functions
 * associated with this particular service.
 */

struct dqnet_dev_stats {
	struct rtnl_link_stats64	netdev;
	__u64				tx_multicast;
	__u64				rx_multicast_bytes;
	__u64				tx_multicast_bytes;
	__u64				rx_broadcast;
	__u64				tx_broadcast;
	__u64				rx_broadcast_bytes;
	__u64				tx_broadcast_bytes;
	__u64				rx_nethook_consumed;
	__u64				tx_nethook_consumed;
};

enum misc_services
{
	PASSWORD,
	TIME,
	NETIF,
	PWM,
	LINUX_STATE,
	CM_POWER_STATE,
	RG_POWER_CONFIG,
	XPT_DEMOD,
	LINUX_VERSION,
	LINUX_APPS_VERSION,
	LINUX_TIME,
	LOOPBACK_TEST,
	CM_TEMPERATURE_STATE
};

/*
 * message used in this service
 */
struct misc_msg
{
	uint32_t	dqm_header;
	uint32_t	type;
	uint32_t	data;
	uint32_t	extra;
};

/*
 * msg.type = enum netif_state
 * msg.data = 	sent: FPM token of buffer containing interface name string
 *      	reply: FPM token of buffer containing return data
 * msg.extra =  reply: 0 for success, < 0 for error (Linux ERRNO)
 */
enum netif_op
{
	IF_UP,		/* interface is up */
	IF_DOWN,	/* interface is down */
	IF_STATS	/* get interface stats */
};

/*
 * msg.type = password_op
 * msg.data = FPM token of buffer containing password
 */
enum password_op
{
	PASSWD_SET,	/* set the Linux password */
	PASSWD_GET,	/* get the Linux password */
};

/*
 * msg.type = time_op
 * msg.data = struct timespec.tv_sec
 * msg.extra = struct timezone.tz_minuteswest
 */
enum time_op
{
	TIME_SET,	/* set the Linux time */
};

extern const char *get_linux_password(void);
int send_netif_state(int dqm_tunnel, char *if_name,
		     enum netif_op state);
int get_netif_stats(int dqm_tunnel, char *if_name,
		    struct dqnet_dev_stats *stats);

int pwm_ctrl(int dqm_tunnel, struct misc_msg *msg);
int cm_power_ctrl(int dqm_tunnel, struct misc_msg *msg);
int rg_power_config(int dqm_tunnel, struct misc_msg *msg);
int cm_temperature_state(int dqm_tunnel, struct misc_msg *msg);

enum linux_state
{
	LS_UNKNOWN = 0,
	LS_INIT,
	LS_IP_UP,
	LS_ERROR,
	LS_CRASH,
	LS_HALTED,
	LS_RESTART,
	LS_PM_POWEROFF,
	LS_REDUCED_POWER,
	LS_FULL_POWER,
	LS_BOOT_SUCCESS,
	LS_UNBELIEVABLE,
};


/* for boot success reporting in linux_state */
enum partition_type {
    PartInvalid,
    PartCMeCos,
    PartLinux,
    PartRootfs,
    PartAppsfs,
    PartRGNV
};

static inline int linux_send_state_msg(int dqm_tunnel, enum linux_state state)
{
	rpc_msg	msg;
	rpc_msg_init(&msg, RPC_SERVICE_MISC, LINUX_STATE, 0, state, 0, 0);
	return rpc_send_message(dqm_tunnel, &msg, true);
}

extern void send_halted_msg_to_ecos(void);
extern void send_restart_msg_to_ecos(void);
extern void send_poweroff_msg_to_ecos(void);

extern int misc_file_open(struct inode *inode,
			 struct file *file);
extern int misc_file_release(struct inode *inode,
			    struct file *file);
extern ssize_t misc_file_read(struct file *file,
			     char __user *buf,
			     size_t count,
			     loff_t *ppos);
extern ssize_t misc_file_write(struct file *file,
			      const char __user *buf,
			      size_t count,
			      loff_t *ppos);
#endif
