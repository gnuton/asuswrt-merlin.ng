// SPDX-License-Identifier: GPL-2.0
/* Readback bridge linked into the unmodified, hash-pinned GT-BE98 prebuilt.
 * Native declarations are deliberately local: these are not portable SDK
 * headers. Offsets/signatures were checked against that exact ELF object.
 */
#include <linux/build_bug.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include "gtbe98_readback.h"

struct gtbe98_sdk_vlan {
	u32 members;
	u32 untag;
	u16 fid;
	u16 table_bit24;
	u16 table_bit25;
	u16 padding;
};

extern void RTK_API_LOCK(void);
extern void RTK_API_UNLOCK(void);
extern int rtk_switch_initialState_get(void);
extern int dal_rtl8373_vlan_get(u32 vid, struct gtbe98_sdk_vlan *vlan);
extern int dal_rtl8373_vlan_portPvid_get(u32 port, u32 *value);
extern int dal_rtl8373_vlan_portIgrFilterEnable_get(u32 port, u32 *value);
extern int dal_rtl8373_vlan_portAcceptFrameType_get(u32 port, u32 *value);
extern int dal_rtl8373_vlan_tagMode_get(u32 port, u32 *value);
extern int dal_rtl8373_vlan_egrFilterEnable_get(u32 *value);
/* The vendor's "rlt" spelling is intentional. */
extern int dal_rlt8373_phy_common_c45_enable_get(u32 phy, u32 *value);
extern int dal_rtl8373_phy_read(u32 phy, u32 mmd, u32 reg, u32 *value);
extern int rtl8373_getAsicReg(u32 reg, u32 *value);

static int gtbe98_read_wan(struct gtbe98_rtl8372_snapshot *s, int after)
{
	u32 enable, a610, link;
	int ret;

	s->failed_index = GTBE98_RTL8372_WAN_PORT;
	s->failed_stage = GTBE98_RTL8372_STAGE_PHY_ENABLE;
	ret = dal_rlt8373_phy_common_c45_enable_get(GTBE98_RTL8372_WAN_PORT,
						 &enable);
	if (ret)
		return ret;
	s->failed_stage = GTBE98_RTL8372_STAGE_PHY_A610;
	ret = dal_rtl8373_phy_read(GTBE98_RTL8372_WAN_PORT, 31, 0xa610, &a610);
	if (ret)
		return ret;
	s->failed_stage = GTBE98_RTL8372_STAGE_LINK;
	/* dal_rtl8373_portStatus_get reads 0x63e8 twice and returns bit port
	 * at offset 8, which the legacy link-status ioctl exposes to userspace.
	 */
	ret = rtl8373_getAsicReg(0x63e8, &link);
	if (!ret)
		ret = rtl8373_getAsicReg(0x63e8, &link);
	if (ret)
		return ret;
	link = (link >> GTBE98_RTL8372_WAN_PORT) & 1U;
	if (after) {
		s->wan_phy_enable_after = enable;
		s->wan_phy_a610_after = a610;
		s->wan_link_after = link;
	} else {
		s->wan_phy_enable = enable;
		s->wan_phy_a610 = a610;
		s->wan_link = link;
	}
	return 0;
}

int gtbe98_rtl8372_capture_snapshot(struct gtbe98_rtl8372_snapshot *s)
{
	struct gtbe98_sdk_vlan vlan;
	struct gtbe98_rtl8372_port *port;
	u32 i;
	int ret;

	BUILD_BUG_ON(sizeof(struct gtbe98_sdk_vlan) != 16);
	BUILD_BUG_ON(offsetof(struct gtbe98_sdk_vlan, members) != 0);
	BUILD_BUG_ON(offsetof(struct gtbe98_sdk_vlan, untag) != 4);
	BUILD_BUG_ON(offsetof(struct gtbe98_sdk_vlan, fid) != 8);
	BUILD_BUG_ON(offsetof(struct gtbe98_sdk_vlan, table_bit24) != 10);
	BUILD_BUG_ON(offsetof(struct gtbe98_sdk_vlan, table_bit25) != 12);
	BUILD_BUG_ON(sizeof(struct gtbe98_rtl8372_vlan) != 16);
	BUILD_BUG_ON(sizeof(struct gtbe98_rtl8372_port) != 16);
	BUILD_BUG_ON(sizeof(struct gtbe98_rtl8372_snapshot) != 65728);
	if (!s)
		return -EINVAL;
	memset(s, 0, sizeof(*s));
	s->abi_version = GTBE98_RTL8372_ABI_VERSION;
	s->size = sizeof(*s);
	s->port_mask = GTBE98_RTL8372_PORT_MASK;

	/* Call DAL getters under one SDK lock. Calling rtk_* wrappers while
	 * holding it would recursively lock the same nonrecursive mutex.
	 * VLAN/MDIO indirect reads issue read commands, never config writes.
	 * Legacy raw register and PHY commands do not use this lock.
	 */
	RTK_API_LOCK();
	s->failed_stage = GTBE98_RTL8372_STAGE_INIT;
	if (rtk_switch_initialState_get() != 1) {
		ret = 15; /* Native RT_ERR_NOT_INIT, as returned by these getters. */
		goto failed;
	}
	ret = gtbe98_read_wan(s, 0);
	if (ret)
		goto failed;
	for (i = 0; i < GTBE98_RTL8372_VLAN_COUNT; ++i) {
		s->failed_stage = GTBE98_RTL8372_STAGE_VLAN;
		s->failed_index = i;
		memset(&vlan, 0, sizeof(vlan));
		ret = dal_rtl8373_vlan_get(i, &vlan);
		if (ret)
			goto failed;
		s->vlans[i].members = vlan.members;
		s->vlans[i].untag = vlan.untag;
		s->vlans[i].fid = vlan.fid;
		s->vlans[i].table_bit24 = vlan.table_bit24;
		s->vlans[i].table_bit25 = vlan.table_bit25;
		if (!(i & 63))
			cond_resched();
	}
	for (i = 0; i < GTBE98_RTL8372_PORT_COUNT; ++i) {
		if (!(GTBE98_RTL8372_PORT_MASK & (1U << i)))
			continue;
		port = &s->ports[i];
		s->failed_index = i;
		s->failed_stage = GTBE98_RTL8372_STAGE_PVID;
		ret = dal_rtl8373_vlan_portPvid_get(i, &port->pvid);
		if (ret)
			goto failed;
		s->failed_stage = GTBE98_RTL8372_STAGE_FILTER;
		ret = dal_rtl8373_vlan_portIgrFilterEnable_get(i,
							    &port->ingress_filter);
		if (ret)
			goto failed;
		s->failed_stage = GTBE98_RTL8372_STAGE_ADMISSION;
		ret = dal_rtl8373_vlan_portAcceptFrameType_get(i, &port->admission);
		if (ret)
			goto failed;
		s->failed_stage = GTBE98_RTL8372_STAGE_TAG_MODE;
		ret = dal_rtl8373_vlan_tagMode_get(i, &port->tag_mode);
		if (ret)
			goto failed;
	}
	s->failed_stage = GTBE98_RTL8372_STAGE_EGRESS_FILTER;
	s->failed_index = 0;
	ret = dal_rtl8373_vlan_egrFilterEnable_get(&s->egress_filter);
	if (ret)
		goto failed;
	ret = gtbe98_read_wan(s, 1);
	if (ret)
		goto failed;
	s->failed_stage = GTBE98_RTL8372_STAGE_NONE;
	s->failed_index = 0;
	s->vlan_count = GTBE98_RTL8372_VLAN_COUNT;
	s->complete = 1;
	RTK_API_UNLOCK();
	return 0;

failed:
	s->sdk_error = ret;
	RTK_API_UNLOCK();
	return -EIO;
}
EXPORT_SYMBOL_GPL(gtbe98_rtl8372_capture_snapshot);
