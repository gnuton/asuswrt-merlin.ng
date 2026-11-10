/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef GTBE98_VLAN_HW_H
#define GTBE98_VLAN_HW_H

#include "gtbe98_wan_vlan.h"
/* One ABI source, shared with the GT-BE98-only kernel companion. */
#include "../../../src-rt-5.04behnd.4916/kernel/linux-4.19/include/uapi/linux/gtbe98_rtl8372.h"

int gtbe98_vlan_hw_identity(void);
int gtbe98_vlan_hw_capture(struct gtbe98_rtl8372_snapshot *snapshot);
int gtbe98_vlan_hw_observe(const struct gtbe98_rtl8372_snapshot *snapshot,
	struct gtbe98_vlan_observation *observed, struct gtbe98_vlan_entry *entries);
int gtbe98_vlan_hw_down(const struct gtbe98_rtl8372_snapshot *snapshot);

#endif
