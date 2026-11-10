/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "gtbe98_vlan_hw.h"

static int missing, bad_identity, io_failure, capture_calls, closes;
static struct gtbe98_rtl8372_snapshot hardware;
static int test_open(const char *path, int flags)
{
	assert(!strcmp(path, GTBE98_RTL8372_DEVICE));
	assert(flags == (O_RDONLY | O_CLOEXEC));
	return missing ? -1 : 42;
}
static int test_close(int fd) { assert(fd == 42); closes++; return 0; }
static int test_ioctl(int fd, unsigned long cmd, void *arg)
{
	struct gtbe98_rtl8372_identity *id = arg;
	struct gtbe98_rtl8372_capture *r = arg;
	assert(fd == 42);
	if (cmd == GTBE98_RTL8372_IOC_IDENTITY) {
		memset(id, 0, sizeof(*id));
		id->abi_version = GTBE98_RTL8372_ABI_VERSION;
		id->size = sizeof(*id); id->profile = GTBE98_RTL8372_PROFILE;
		id->snapshot_size = sizeof(hardware); id->vlan_count = 4096; id->port_mask = 0xf8;
		memcpy(id->prebuilt_sha256, GTBE98_RTL8372_PREBUILT_SHA256, 64);
		if (bad_identity == 1) id->abi_version++;
		if (bad_identity == 2) id->snapshot_size--;
		if (bad_identity == 3) id->profile++;
		if (bad_identity == 4) id->port_mask++;
		if (bad_identity == 5) id->prebuilt_sha256[0] ^= 1;
		return 0;
	}
	assert(cmd == GTBE98_RTL8372_IOC_CAPTURE);
	assert(r->abi_version == 1 && r->size == sizeof(*r) && !r->reserved);
	assert(r->snapshot_size == sizeof(hardware));
	capture_calls++;
	memcpy((void *)(uintptr_t)r->snapshot, &hardware, sizeof(hardware));
	return io_failure ? -1 : 0;
}
#define open test_open
#define close test_close
#define ioctl test_ioctl
#include "../release/src/router/shared/gtbe98_vlan_hw.c"

int main(void)
{
	static struct gtbe98_rtl8372_snapshot s;
	static struct gtbe98_vlan_entry entries[4096];
	struct gtbe98_vlan_observation o;
	int i;
	hardware.abi_version = 1; hardware.size = sizeof(hardware); hardware.complete = 1;
	hardware.vlan_count = 4096; hardware.port_mask = 0xf8;
	hardware.wan_phy_a610 = hardware.wan_phy_a610_after = 0x2858;
	assert(!gtbe98_vlan_hw_identity() && closes == 1);
	missing = 1; assert(gtbe98_vlan_hw_capture(&s) == -1 && !s.complete); missing = 0;
	for (i = 1; i <= 5; ++i) {
		bad_identity = i;
		assert(gtbe98_vlan_hw_capture(&s) == -1 && !s.complete && !capture_calls);
	}
	bad_identity = 0;
	assert(!gtbe98_vlan_hw_capture(&s) && gtbe98_vlan_hw_down(&s));
	assert(!gtbe98_vlan_hw_observe(&s, &o, entries) && o.complete);
	s.wan_link_after = 1; assert(!gtbe98_vlan_hw_down(&s));
	s = hardware; s.wan_phy_a610 = 0x2058; assert(!gtbe98_vlan_hw_down(&s));
	s = hardware; s.failed_stage = 1;
	assert(!gtbe98_vlan_hw_down(&s) && gtbe98_vlan_hw_observe(&s, &o, entries) == -1 && !o.complete);
	s = hardware; s.ports[5].admission = 3;
	assert(gtbe98_vlan_hw_observe(&s, &o, entries) == -1 && !o.complete);
	s = hardware; s.vlans[1].table_bit25 = 2;
	assert(gtbe98_vlan_hw_observe(&s, &o, entries) == -1 && !o.complete);
	io_failure = 1; assert(gtbe98_vlan_hw_capture(&s) == -1 && !s.complete);
	io_failure = 0; hardware.complete = 0;
	assert(gtbe98_vlan_hw_capture(&s) == -1 && !s.complete);
	hardware.complete = 1; hardware.sdk_error = 7;
	assert(gtbe98_vlan_hw_capture(&s) == -1 && !s.complete);
	puts("GT-BE98 userspace readback identity, errors, completeness and PHY guards passed");
	return 0;
}
