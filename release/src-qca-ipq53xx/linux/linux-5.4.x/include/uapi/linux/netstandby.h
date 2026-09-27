/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifndef _LINUX_NETSTANDBY_H
#define _LINUX_NETSTANDBY_H

#include <linux/netdevice.h>

#define MAX_INTERFACE 16

#define NETSTANDBY_EXIT_TRIGGER_RULE_SRC_IP_VALID 0x00000001	/**< Indicate if trigger rule uses source ip address */
#define NETSTANDBY_EXIT_TRIGGER_RULE_DES_IP_VALID 0x00000002	/**< Indicate if trigger rule uses destination ip address */
#define NETSTANDBY_EXIT_TRIGGER_RULE_SRC_MAC_VALID 0x00000004	/**< Indicate if trigger rule uses source mac address */
#define NETSTANDBY_EXIT_TRIGGER_RULE_DEST_MAC_VALID 0x00000008	/**< Indicate if trigger rule uses destination mac address */
#define NETSTANDBY_EXIT_TRIGGER_RULE_PROTOCOL_VALID 0x00000010	/**< Indicate if trigger rule uses L4 protocol */
#define NETSTANDBY_EXIT_TRIGGER_RULE_IPV4 0x00000020		/**< Indicate if trigger rule used IP address is IPv4 */
#define NETSTANDBY_EXIT_TRIGGER_RULE_IPV6 0x00000040		/**< Indicate if trigger rule used IP address is IPv6 */

#define NETSTANDBY_ENTER_NSS_FLAG_VALID_PORT_NONE 0x00000001	/**< Indicate ALL MHT port to be shut down */
#define NETSTANDBY_ENTER_NSS_FLAG_VALID_PORT_ALL 0x00000002	/**< Indicate ALL  MHT port be awake */
#define NETSTANDBY_ENTER_NSS_FLAG_VALID_PORT_IDX 0x00000004	/**< Indicate a particular port need to be in UP state */

/*
 * netstandby_subsystem_type
 */
enum netstandby_subsystem_type {
	NETSTANDBY_SUBSYSTEM_TYPE_NSS = 0,	/**< Platform type is NSS */
	NETSTANDBY_SUBSYSTEM_TYPE_WIFI = 1,	/**< Platform type is WI-FI */
	NETSTANDBY_SUBSYSTEM_TYPE_PLATFORM = 2,	/**< Platform type is BSP */
	NETSTANDBY_SUBSYSTEM_TYPE_MAX,
};

/*
 * netstandby_notification
 */
enum netstandby_notif_type {
	NETSTANDBY_NOTIF_ENTER_COMPLETE = 0,	/**< Enter complete notification */
	NETSTANDBY_NOTIF_EXIT_COMPLETE,		/**< Exit complete notification */
	NETSTANDBY_NOTIF_TRIGGER,		/**< Trigger wakeup trigger */
};

/*
 * netstandby_exit_trigger_rule
 *	Trigger rule
 */
struct netstandby_exit_trigger_rule {
	uint32_t valid_flags;			/**< Indicates which field to consider for trigger rule */
	uint32_t src_ip[4];			/**< Source IP address */
	uint8_t smac[6];			/**< Source MAC address */
	uint32_t dest_ip[4];			/**< Destination IP address */
	uint8_t dmac[6];			/**< Destination MAC address */
	int protocol;				/**< Protocol */
};

/*
 * netstandby_nss_priv_info
 *	NSS private information
 */
struct netstandby_nss_priv_info {
	uint8_t port_id;			/**< Manhattan port-id */
	uint16_t flags;				/**< NSS valid flags */
};

/*
 * netstandby_event_compl_info
 */
struct netstandby_event_compl_info {
	enum netstandby_notif_type event_type;		/**< Event type */
	enum netstandby_subsystem_type system_type;	/**< Subsystem type */
};

/*
 * netstandby_trigger_info
 */
struct netstandby_trigger_info {
	struct net_device *dev;				/**< Device which receives notification */
	enum netstandby_subsystem_type system_type;	/**< Subsystem which receives this notification */
	enum netstandby_notif_type event_type;		/**< Event type */
};

/*
 * netstandby_exit_info
 */
struct netstandby_exit_info {
	uint32_t reserved;		/**< Reserved */
};

/*
 * netstandby_entry_info
 */
struct netstandby_entry_info {
	struct net_device *dev[MAX_INTERFACE]; 		/**< List of designated interfaces to exit network standby mode */
	struct netstandby_exit_trigger_rule tuple;	/**< Trigger rule informataion */
	struct netstandby_nss_priv_info nss_info;	/**< NSS specific entry information */
	int iface_cnt;					/**< Number of designated wakeup interface */
};

/*
 * Callbacks type
 */
typedef int (*netstandby_exit_cb_t) (void *app_data, struct netstandby_exit_info *exit_info);
typedef int (*netstandby_enter_cb_t) (void *app_data, struct netstandby_entry_info *enter_info);
typedef void (*netstandby_event_compl_cb_t) (void *app_data, struct netstandby_event_compl_info *event_info);
typedef void (*netstandby_trigger_notif_cb_t) (void *app_data, struct netstandby_trigger_info *trigger_info);

/*
 * struct netstandby_reg_info
 * 	registration structure
 */
struct netstandby_reg_info {
	netstandby_enter_cb_t enter_cb;	/**< Callback to enter network standby mode */
	netstandby_exit_cb_t exit_cb;		/**< Callback to exit network standby mode */
	netstandby_event_compl_cb_t enter_cmp_cb;	/**< Callback enter completion event */
	netstandby_event_compl_cb_t exit_cmp_cb;	/**< Callback exit completion event */
	netstandby_trigger_notif_cb_t trigger_cb;	/** < Callback when wakeup trigger is received */
	void *app_data;				/** < Application data to filled by NSS/WI-FI/BSP and passed by Standby module */
};

/*
 * Registration API for various subsystems (NSS, Wi-Fi, Platform)
 *	API(s) to be implemented and exposed by different subsystem
 *	and called by standby module.
 */
int nss_dp_get_and_register_cb(struct netstandby_reg_info *info);
int netstandby_wifi_get_and_register_cb(struct netstandby_reg_info *info);
int netstandby_platform_get_and_register_cb(struct netstandby_reg_info *info);
#endif
