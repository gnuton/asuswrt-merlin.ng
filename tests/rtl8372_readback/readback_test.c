/* SPDX-License-Identifier: GPL-2.0 */
#include "kernel_test.h"
#include "../../release/src-rt-5.04behnd.4916/kernel/linux-4.19/drivers/char/rtl8372/gtbe98_readback_adapter.c"
#include "../../release/src-rt-5.04behnd.4916/kernel/linux-4.19/drivers/char/rtl8372/gtbe98_rtl8372_readback.c"

void RTK_API_LOCK(void) { assert(!test_locked); test_locked = 1; }
void RTK_API_UNLOCK(void) { assert(test_locked); test_locked = 0; }
int rtk_switch_initialState_get(void)
{
	assert(test_locked);
	return !test_not_initialized;
}
static int sdk_read(void)
{
	assert(test_locked);
	return ++test_reads == test_fail_read ? 0x12345 : 0;
}
int dal_rtl8373_vlan_get(u32 vid, struct gtbe98_sdk_vlan *vlan)
{
	int ret = sdk_read();
	assert(vid < 4096);
	if (!ret) {
		vlan->members = vid & 0x3ff;
		vlan->untag = vid & 0x1ff;
		vlan->fid = vid & 15;
		vlan->table_bit24 = vid & 1;
		vlan->table_bit25 = (vid >> 1) & 1;
	}
	return ret;
}
static int port_read(u32 port, u32 *value, u32 result)
{
	int ret = sdk_read();
	assert(port < 8 && (0xf8 & (1U << port)));
	if (!ret)
		*value = result;
	return ret;
}
int dal_rtl8373_vlan_portPvid_get(u32 p, u32 *v) { return port_read(p, v, 99 + p); }
int dal_rtl8373_vlan_portIgrFilterEnable_get(u32 p, u32 *v) { return port_read(p, v, 1); }
int dal_rtl8373_vlan_portAcceptFrameType_get(u32 p, u32 *v) { return port_read(p, v, p % 3); }
int dal_rtl8373_vlan_tagMode_get(u32 p, u32 *v) { return port_read(p, v, p % 4); }
int dal_rtl8373_vlan_egrFilterEnable_get(u32 *v)
{
	int ret = sdk_read();
	if (!ret)
		*v = 1;
	return ret;
}
int dal_rlt8373_phy_common_c45_enable_get(u32 phy, u32 *v)
{
	assert(phy == 5);
	return port_read(phy, v, 1);
}
int dal_rtl8373_phy_read(u32 phy, u32 mmd, u32 reg, u32 *v)
{
	assert(phy == 5 && mmd == 31 && reg == 0xa610);
	return port_read(phy, v, 0x2858);
}
int rtl8373_getAsicReg(u32 reg, u32 *v)
{
	int ret = sdk_read();
	assert(reg == 0x63e8);
	if (!ret)
		*v = 0x3df; /* WAN bit 5 down; other bits must not contaminate it. */
	return ret;
}

static struct gtbe98_rtl8372_snapshot snapshot;

static void test_capture(void)
{
	unsigned int i;
	int reads;
	assert(gtbe98_rtl8372_capture_snapshot(NULL) == -EINVAL);
	assert(gtbe98_rtl8372_capture_snapshot(&snapshot) == 0);
	reads = test_reads;
	assert(reads == 4096 + 5 * 4 + 8 + 1);
	assert(snapshot.complete == 1 && !snapshot.sdk_error && !test_locked);
	assert(snapshot.vlan_count == 4096 && snapshot.port_mask == 0xf8);
	assert(snapshot.egress_filter == 1);
	assert(snapshot.wan_phy_enable == 1 && snapshot.wan_phy_enable_after == 1);
	assert(snapshot.wan_phy_a610 == 0x2858 && snapshot.wan_phy_a610_after == 0x2858);
	assert(snapshot.wan_link == 0 && snapshot.wan_link_after == 0);
	for (i = 0; i < 4096; ++i) {
		assert(snapshot.vlans[i].members == (i & 0x3ff));
		assert(snapshot.vlans[i].untag == (i & 0x1ff));
		assert(snapshot.vlans[i].fid == (i & 15));
		assert(snapshot.vlans[i].table_bit24 == (i & 1));
		assert(snapshot.vlans[i].table_bit25 == ((i >> 1) & 1));
		assert(snapshot.vlans[i].reserved == 0);
	}
	for (i = 0; i < 8; ++i)
		assert(snapshot.ports[i].pvid == (i < 3 ? 0 : 99 + i));
	/* Every SDK failure, including the last read, must prevent success and
	 * release the shared lock. Starting with an old successful snapshot
	 * verifies that stale complete/table-count flags cannot survive.
	 */
	for (test_fail_read = 1; test_fail_read <= reads; ++test_fail_read) {
		test_reads = 0;
		snapshot.complete = 1;
		snapshot.vlan_count = 4096;
		assert(gtbe98_rtl8372_capture_snapshot(&snapshot) == -EIO);
		assert(test_reads == test_fail_read && !test_locked);
		assert(!snapshot.complete && !snapshot.vlan_count);
		assert(snapshot.sdk_error == 0x12345 && snapshot.failed_stage);
	}
	test_fail_read = 0;
	test_not_initialized = 1;
	test_reads = 0;
	assert(gtbe98_rtl8372_capture_snapshot(&snapshot) == -EIO);
	assert(!test_reads && !test_locked && !snapshot.complete);
	assert(snapshot.sdk_error == 15 && snapshot.failed_stage == GTBE98_RTL8372_STAGE_INIT);
	test_not_initialized = 0;
}

static void test_ioctl(void)
{
	struct gtbe98_rtl8372_identity id;
	struct gtbe98_rtl8372_capture request = {
		.abi_version = 1, .size = 24,
		.snapshot = (uintptr_t)&snapshot, .snapshot_size = sizeof(snapshot),
	};
	unsigned long arg = (unsigned long)&request;

	assert(gtbe98_readback_init() == 0);
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_IDENTITY,
				      (unsigned long)&id) == 0);
	assert(id.abi_version == 1 && id.size == 88 && id.snapshot_size == 65728);
	assert(memcmp(id.prebuilt_sha256, GTBE98_RTL8372_PREBUILT_SHA256, 64) == 0);
	test_privileged = 0;
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_IDENTITY, 0) == -EPERM);
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_CAPTURE, arg) == -EPERM);
	test_privileged = 1;
	assert(gtbe98_readback_ioctl(NULL, 0, arg) == -ENOTTY);
	request.abi_version = 2;
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_CAPTURE, arg) == -EINVAL);
	request.abi_version = 1;
	request.reserved = 1;
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_CAPTURE, arg) == -EINVAL);
	request.reserved = 0;
	request.snapshot_size--;
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_CAPTURE, arg) == -EINVAL);
	request.snapshot_size++;
	request.size--;
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_CAPTURE, arg) == -EINVAL);
	request.size++;
	request.snapshot = 0;
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_CAPTURE, arg) == -EFAULT);
	request.snapshot = (uintptr_t)&snapshot;
#ifdef CONFIG_COMPAT
	test_compat = 1;
	request.snapshot = UINT64_C(1) << 32;
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_CAPTURE, arg) == -EFAULT);
	request.snapshot = (uintptr_t)&snapshot;
	test_compat = 0;
#endif
	test_copy_in_fail = 1;
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_CAPTURE, arg) == -EFAULT);
	test_copy_in_fail = 0;
	test_alloc_fail = 1;
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_CAPTURE, arg) == -ENOMEM);
	test_alloc_fail = 0;
	test_reads = 0;
	test_fail_read = 42;
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_CAPTURE, arg) == -EIO);
	assert(!snapshot.complete && snapshot.sdk_error == 0x12345);
	test_reads = 0;
	test_fail_read = 0;
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_CAPTURE, arg) == 0);
	assert(snapshot.complete);
	test_copy_out_fail = 1;
	assert(gtbe98_readback_ioctl(NULL, GTBE98_RTL8372_IOC_CAPTURE, arg) == -EFAULT);
	test_copy_out_fail = 0;
	gtbe98_readback_exit();
}

int main(void)
{
	test_capture();
	test_ioctl();
	puts("RTL8372 readback host tests: complete capture, 4125 SDK faults, ioctl guards passed");
	return 0;
}
