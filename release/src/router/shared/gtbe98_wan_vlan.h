/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __GTBE98_WAN_VLAN_H__
#define __GTBE98_WAN_VLAN_H__

#include <stddef.h>

/* ASUS masks, not SDK or front-panel port numbers. */
#define GTBE98_VLAN_CPU 0x08U
#define GTBE98_VLAN_WAN 0x20U
#define GTBE98_VLAN_LAN 0xd0U
#define GTBE98_VLAN_PORTS 0xf8U
#define GTBE98_VLAN_COUNT 4096U

enum gtbe98_vlan_result {
	GTBE98_VLAN_OK = 0,
	GTBE98_VLAN_DISABLED,
	GTBE98_VLAN_INVALID,
	GTBE98_VLAN_TOPOLOGY,
	GTBE98_VLAN_IPTV_CONFLICT,
	GTBE98_VLAN_LAN_CONFLICT,
	GTBE98_VLAN_UNVERIFIED,
	GTBE98_VLAN_MISMATCH,
	GTBE98_VLAN_IO_ERROR,
	GTBE98_VLAN_QUIESCE_ERROR
};

/* The caller supplies an effective, consistent configuration snapshot.
 * dot1q/vid/dot1p belong to wan0_; wan_* is only the web form projection.
 * Do not fall back to switch_wan0tagid or silently clear an IPTV profile.
 * No NVRAM changes, device access or service restart occur in this module.
 */
struct gtbe98_vlan_request {
	const char *dot1q;
	const char *vid;
	const char *dot1p;
	const char *iptv_profile;
	const char *stb_port;
	int gtbe98_rtl8372;
	int router_mode;
	int wan_unit;
	int manual_wan;
	int single_wan; /* No second Ethernet, USB or wireless WAN, even standby. */
	int extwan;     /* 0: eth0 (10G), 1: LAN/WAN1 on eth1 (2.5G). */
	int wan_aggregation;
	int lan_vlan_state; /* 0: confirmed inactive; 1: active; -1: unknown. */
	const unsigned int *reserved_vids; /* Configured LAN/internal VID inventory. */
	size_t reserved_count;
};

struct gtbe98_vlan_entry {
	unsigned int members;
	unsigned int untag;
	unsigned int fid;
	unsigned int table_flags;
};

struct gtbe98_vlan_plan {
	unsigned int vid;
	unsigned int priority;
	int program_switch;
	char parent[8];
	char ifname[16];
	unsigned int isp_members;
	unsigned int lan_members;
	unsigned int isp_create_mask;
	unsigned int lan_create_mask;
};

/* A hardware adapter must fill this from fresh, checked reads while holding
 * exclusive switch ownership and keeping WAN/LAN1 quiesced. A successful
 * legacy rtkswitch ioctl, cached writes or parsed printk are NOT readback.
 * entries must contain every VID, including zero/unused entries; otherwise
 * an unrelated VLAN could still admit the WAN port into the LAN.
 */
struct gtbe98_vlan_observation {
	int complete;
	const struct gtbe98_vlan_entry *entries;
	size_t count;
	unsigned int pvid[8];
	unsigned int ingress_filter;
	unsigned int tagged_only;
	unsigned int untagged_only;
	unsigned int admit_all;
	unsigned int egress_filter;
	unsigned int nondefault_tag_mode;
};

enum gtbe98_vlan_result gtbe98_wan_vlan_plan(
	const struct gtbe98_vlan_request *request, struct gtbe98_vlan_plan *plan);
enum gtbe98_vlan_result gtbe98_wan_vlan_verify(
	const struct gtbe98_vlan_plan *plan,
	const struct gtbe98_vlan_observation *observed);
const char *gtbe98_wan_vlan_error(enum gtbe98_vlan_result result);

/* Adapter contract: quiesce must verify physical WAN down, not just submit a
 * command. The caller holds exclusive ownership throughout staging and Linux
 * setup. Staging NEVER enables the port, including on success. It must not be
 * released until Linux VLAN/bridge state has also been checked by the caller.
 */
struct gtbe98_vlan_ops {
	int (*quiesce)(void *ctx);
	int (*command)(void *ctx, unsigned int cmd, unsigned int arg);
	int (*readback)(void *ctx, struct gtbe98_vlan_observation *observed);
};
enum gtbe98_vlan_result gtbe98_wan_vlan_stage(
	const struct gtbe98_vlan_plan *plan, const struct gtbe98_vlan_ops *ops, void *ctx);

#endif
