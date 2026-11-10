/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_GTBE98_RTL8372_H
#define _UAPI_LINUX_GTBE98_RTL8372_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define GTBE98_RTL8372_ABI_VERSION 1U
#define GTBE98_RTL8372_PROFILE 1U
#define GTBE98_RTL8372_DEVICE "/dev/gtbe98_rtl8372"
#define GTBE98_RTL8372_PREBUILT_SHA256 \
	"3e1948756cec3f05e217c637dc9debe7a9addfc93749feb3653fb9a3006e38f7"
#define GTBE98_RTL8372_VLAN_COUNT 4096U
#define GTBE98_RTL8372_PORT_COUNT 8U
#define GTBE98_RTL8372_PORT_MASK 0xf8U
#define GTBE98_RTL8372_WAN_PORT 5U

struct gtbe98_rtl8372_identity {
	__u32 abi_version;
	__u32 size;
	__u32 profile;
	__u32 snapshot_size;
	__u32 vlan_count;
	__u32 port_mask;
	/* Exactly 64 ASCII hex characters, without a terminating NUL. */
	__u8 prebuilt_sha256[64];
};

struct gtbe98_rtl8372_vlan {
	__u32 members;
	__u32 untag;
	__u16 fid;
	/* Literal hardware table flags; no inferred semantics in this ABI. */
	__u8 table_bit24;
	__u8 table_bit25;
	__u32 reserved;
};

struct gtbe98_rtl8372_port {
	__u32 pvid;
	__u32 ingress_filter;
	__u32 admission;
	__u32 tag_mode;
};

#define GTBE98_RTL8372_STAGE_NONE 0U
#define GTBE98_RTL8372_STAGE_INIT 1U
#define GTBE98_RTL8372_STAGE_VLAN 2U
#define GTBE98_RTL8372_STAGE_PVID 3U
#define GTBE98_RTL8372_STAGE_FILTER 4U
#define GTBE98_RTL8372_STAGE_ADMISSION 5U
#define GTBE98_RTL8372_STAGE_TAG_MODE 6U
#define GTBE98_RTL8372_STAGE_EGRESS_FILTER 7U
#define GTBE98_RTL8372_STAGE_PHY_ENABLE 8U
#define GTBE98_RTL8372_STAGE_PHY_A610 9U
#define GTBE98_RTL8372_STAGE_LINK 10U

/* A successful capture reads the whole table under the SDK API mutex.
 * Raw legacy register/PHY ioctls bypass that mutex: the caller must exclude
 * those writers separately. This is configuration readback, not evidence of
 * traffic isolation. PHY/link fields bracket the other reads, not a promise
 * that the link remained down between observations.
 * On -EIO, only the header's diagnostics are valid; complete stays zero.
 */
struct gtbe98_rtl8372_snapshot {
	__u32 abi_version;
	__u32 size;
	__u32 complete;
	__s32 sdk_error;
	__u32 failed_stage;
	__u32 failed_index;
	__u32 port_mask;
	__u32 egress_filter;
	__u32 wan_phy_enable;
	__u32 wan_phy_a610;
	__u32 wan_link;
	__u32 wan_phy_enable_after;
	__u32 wan_phy_a610_after;
	__u32 wan_link_after;
	__u32 vlan_count;
	__u32 reserved;
	struct gtbe98_rtl8372_port ports[GTBE98_RTL8372_PORT_COUNT];
	struct gtbe98_rtl8372_vlan vlans[GTBE98_RTL8372_VLAN_COUNT];
};

/* The snapshot exceeds _IOC_SIZEBITS. This fixed-size request points to the
 * exact-size output buffer. Aligned u64 keeps the layout identical on ILP32
 * and LP64; no userspace pointer or native long occurs in the wire format.
 */
struct gtbe98_rtl8372_capture {
	__u32 abi_version;
	__u32 size;
	__aligned_u64 snapshot;
	__u32 snapshot_size;
	__u32 reserved;
};

#define GTBE98_RTL8372_IOC_IDENTITY \
	_IOR('G', 0x71, struct gtbe98_rtl8372_identity)
#define GTBE98_RTL8372_IOC_CAPTURE \
	_IOW('G', 0x72, struct gtbe98_rtl8372_capture)

#endif
