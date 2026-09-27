/* SPDX-License-Identifier: GPL-2.0-only */
/*
* Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
*/

#ifndef _LINUX_EAWTP_H
#define _LINUX_EAWTP_H

#include <linux/netdevice.h>

/*
 * eawtp_port_info
 */
struct eawtp_port_info {
	uint8_t num_active_port;	/**< Number of active ports */
};

/*
 * Callbacks type
 */
typedef int (*eawtp_get_num_active_ports_cb_t) (void *app_data, struct eawtp_port_info *pinfo);
typedef int (*eawtp_notify_active_ports_cb_t) (void *app_data, struct eawtp_port_info *ninfo);
typedef int (*eawtp_port_link_notify_register_cb_t) (void *app_data);

/*
 * struct eawtp_reg_info
 * 	Eawtp registration structure
 */
struct eawtp_reg_info {
	eawtp_get_num_active_ports_cb_t get_active_ports_cb;	/**< Get number of active ports */
	eawtp_notify_active_ports_cb_t ntfy_active_ports_cb;	/**< Notify active ports after any port link state change */
	eawtp_notify_active_ports_cb_t ntfy_port_status_to_wifi_cb;	/**< Notify active ports status to wifi */
	eawtp_get_num_active_ports_cb_t get_nss_active_port_cnt_cb; /**< Get number of active ports from DP */
	eawtp_port_link_notify_register_cb_t port_link_notify_register_cb;/**< Port link notifier register */
	eawtp_port_link_notify_register_cb_t port_link_notify_unregister_cb;/**< Port link notifier unregister */
};

/*
 * Registration API for various subsystems (NSS, Wi-Fi)
 *	API(s) to be implemented and exposed by different subsystem
 *	and called by standby eawtp module.
 */
int eawtp_nss_get_and_register_cb(struct eawtp_reg_info *info);
int eawtp_wifi_get_and_register_cb(struct eawtp_reg_info *info);
int eawtp_nss_unregister_cb(void);
int eawtp_wifi_unregister_cb(void);
#endif
