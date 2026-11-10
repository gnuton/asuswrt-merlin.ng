/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "gtbe98_vlan_hw.h"

static int snapshot_valid(const struct gtbe98_rtl8372_snapshot *s)
{
	return s && s->abi_version == GTBE98_RTL8372_ABI_VERSION &&
		s->size == sizeof(*s) && s->complete == 1 && !s->sdk_error &&
		!s->failed_stage && !s->failed_index && !s->reserved &&
		s->vlan_count == GTBE98_VLAN_COUNT && s->port_mask == GTBE98_VLAN_PORTS;
}

static int readback_open(void)
{
	struct gtbe98_rtl8372_identity id;
	int fd = open(GTBE98_RTL8372_DEVICE, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return -1;
	memset(&id, 0, sizeof(id));
	if (ioctl(fd, GTBE98_RTL8372_IOC_IDENTITY, &id) ||
	    id.abi_version != GTBE98_RTL8372_ABI_VERSION || id.size != sizeof(id) ||
	    id.profile != GTBE98_RTL8372_PROFILE ||
	    id.snapshot_size != sizeof(struct gtbe98_rtl8372_snapshot) ||
	    id.vlan_count != GTBE98_VLAN_COUNT || id.port_mask != GTBE98_VLAN_PORTS ||
	    memcmp(id.prebuilt_sha256, GTBE98_RTL8372_PREBUILT_SHA256, 64)) {
		close(fd);
		errno = EPROTO;
		return -1;
	}
	return fd;
}

int gtbe98_vlan_hw_identity(void)
{
	int fd = readback_open();
	if (fd < 0)
		return -1;
	close(fd);
	return 0;
}

int gtbe98_vlan_hw_capture(struct gtbe98_rtl8372_snapshot *snapshot)
{
	struct gtbe98_rtl8372_capture capture;
	int fd, ret;
	if (!snapshot)
		return -1;
	memset(snapshot, 0, sizeof(*snapshot));
	fd = readback_open();
	if (fd < 0)
		return -1;
	memset(&capture, 0, sizeof(capture));
	capture.abi_version = GTBE98_RTL8372_ABI_VERSION;
	capture.size = sizeof(capture);
	capture.snapshot = (uintptr_t)snapshot;
	capture.snapshot_size = sizeof(*snapshot);
	ret = ioctl(fd, GTBE98_RTL8372_IOC_CAPTURE, &capture);
	close(fd);
	if (ret || !snapshot_valid(snapshot)) {
		snapshot->complete = 0;
		return -1;
	}
	return 0;
}

int gtbe98_vlan_hw_observe(const struct gtbe98_rtl8372_snapshot *s,
	struct gtbe98_vlan_observation *o, struct gtbe98_vlan_entry *entries)
{
	unsigned int i;
	if (!o)
		return -1;
	memset(o, 0, sizeof(*o));
	if (!entries || !snapshot_valid(s))
		return -1;
	for (i = 0; i < GTBE98_VLAN_COUNT; ++i) {
		if (s->vlans[i].reserved || s->vlans[i].table_bit24 > 1 || s->vlans[i].table_bit25 > 1)
			return -1;
		entries[i].members = s->vlans[i].members;
		entries[i].untag = s->vlans[i].untag;
		entries[i].fid = s->vlans[i].fid;
		entries[i].table_flags = s->vlans[i].table_bit24 | (s->vlans[i].table_bit25 << 1);
	}
	for (i = 0; i < 8; ++i) {
		if (!(GTBE98_VLAN_PORTS & (1U << i)))
			continue;
		o->pvid[i] = s->ports[i].pvid;
		if (s->ports[i].ingress_filter > 1 || s->ports[i].admission > 2)
			return -1;
		if (s->ports[i].ingress_filter)
			o->ingress_filter |= 1U << i;
		if (s->ports[i].admission == 0)
			o->admit_all |= 1U << i;
		else if (s->ports[i].admission == 1)
			o->tagged_only |= 1U << i;
		else
			o->untagged_only |= 1U << i;
		if (s->ports[i].tag_mode)
			o->nondefault_tag_mode |= 1U << i;
	}
	o->egress_filter = s->egress_filter;
	o->entries = entries;
	o->count = GTBE98_VLAN_COUNT;
	o->complete = 1;
	return 0;
}

int gtbe98_vlan_hw_down(const struct gtbe98_rtl8372_snapshot *s)
{
	/* Vendor PHY power sequence used by the pinned driver's command 11.
	 * Clause-45 MMD1 enable alone does not describe this vendor operation.
	 */
	return snapshot_valid(s) && s->wan_phy_a610 == 0x2858 &&
		s->wan_phy_a610_after == 0x2858 && s->wan_link == 0 && s->wan_link_after == 0;
}
