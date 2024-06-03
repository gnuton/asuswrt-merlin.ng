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

#ifndef BRCM_BA_H
#define BRCM_BA_H

#include "brcm_mbox.h"

#define BA_IOC_MAGIC          'B'

#define BA_IOCTL_GET_VERSION _IOR(BA_IOC_MAGIC, 1, __u32)
#define BA_IOCTL_POWER _IOW(BA_IOC_MAGIC, 2, struct power_request)
#define BA_IOCTL_C2INIT _IOW(BA_IOC_MAGIC, 3, struct c2_req)
#define BA_IOCTL_C2FINISH _IO(BA_IOC_MAGIC, 4)
#define BA_IOCTL_GETMAP _IOR(BA_IOC_MAGIC, 5, struct ba_map)
#define BA_IOCTL_SETMAP _IOWR(BA_IOC_MAGIC, 6, struct ba_map)
#define BA_IOCTL_GETARC _IOR(BA_IOC_MAGIC, 7, struct ba_map)
#define BA_IOCTL_SETARC _IO(BA_IOC_MAGIC, 8)
#define BA_IOCTL_GET_DTB _IOWR(BA_IOC_MAGIC, 9, struct dtb_req)
#define BA_IOCTL_GET_OTP _IOR(BA_IOC_MAGIC, 10, __u32)
#define BA_IOCTL_BOOT_ERROR _IOR(BA_IOC_MAGIC, 11, __u32)
#define BA_IOCTL_GET_RG_BATT_MODE _IOR(BA_IOC_MAGIC, 12, enum rg_battery_mode)

#define BA_KVERSION 0x00020002
#define BA_KVER_MINOR(x) (x & 0x0000FFFF)
#define BA_KVER_MAJOR(x) (x >> 16)

#define NUM_ATW_CLIENTS 4 /* The number of client bitmask registers.  4 for 3390 */
#define MAX_ATW 8 /* TBD... can change per chip? */
#define MAX_ARC 8 /* TBD... can change per chip? */
#define MAX_ARC_MEMC 2 /* TBD... can change per chip? */
#define MAX_ARC_BUS 2 /* TBD... DOES change per chip */
#define MAX_ARC_CLIENTS 8  /* TBD... can change per chip? */

/* Devices that boot assist will need to configure/allocate memory for */
enum ba_device {
	DEV_CM,
	DEV_RG,
	DEV_FPM,
	DEV_LEAP,
	DEV_MAX,
};

/* Devices that will require an ATW */
enum ba_dev_atw {
	ATW_CM,
	ATW_CM_DSP,
	ATW_CM_BOOT,
	ATW_CM_EXTRA,
	ATW_CM_BOOT_EXTRA,
	ATW_DEBUG,
	ATW_MAX,
};

/* Devices that will require an ARC (excluding STB ARC's) */
enum ba_dev_arc {
	ARC_RG,
	ARC_CM,
	ARC_STB,
	ARC_MAX,
};

static inline char *device_to_string(enum ba_dev_arc device)
{
	switch (device) {
	case DEV_CM:
		return "CM";
	case DEV_RG:
		return "RG";
	case DEV_FPM:
		return "FPM";
	case DEV_LEAP:
		return "LEAP";
	default:
		break;
	}
	return "Invalid";
}

/* Specifics for ARC/ATW windows */
struct ba_map_arc_atw {
	__u32 atw[ATW_MAX][NUM_ATW_CLIENTS]; /* ATW client list per device */
	__u32 arc_r[ARC_MAX][8]; /* Read ARC client list per device */
	__u32 arc_w[ARC_MAX][8]; /* Write ARC client list per device */
};

/* Request to power on or off a device */
struct power_request {
	enum ba_device device; /* RG or CM */
	__u8 on; /* true, power on, false, power off */
};

/* Initiate post CM operation (C2) */
struct c2_req {
	enum battery_state battery; /* Allowed for debug purposes */
	__u32 ap_lost_major_dly; /* Default value for AP lost major delay reg */
	enum battery_state boot_battery;
};

struct dtb_req {
	void *buffer;
	__u32 size;
};

struct ba_atw {
	__u8 index;
	__u64 source; /* Physical source address */
	__u64 dest; /* "Virtual" address seen by client */
	__u32 size;
	__u32 clients[NUM_ATW_CLIENTS];
};

struct ba_arc {
	__u8 exclusive;
	__u64 start;
	__u64 end;
	__u32 read[MAX_ARC_CLIENTS];
	__u32 write[MAX_ARC_CLIENTS];
};

struct ba_vm {
	__u64 start;
	__u32 size;
	__u8 cpus;
	unsigned int priority;
	unsigned int latency;
};

struct ba_rg_info {
	struct ba_vm vm;
#ifdef CONFIG_BCM7145A0
	struct ba_atw atw;
#endif
};

struct ba_stb_info {
	struct ba_vm vm;
};

struct ba_tp1 {
	__u64 start;
	__u32 size;
};

struct ba_cm_viper {
	struct ba_atw cm_atw;
	struct ba_atw cm_atw_extra;
	struct ba_atw boot_atw;
	struct ba_atw boot_atw_extra;
};

struct ba_leap {
	__u32 size;
};

struct ba_cm_dsp {
	struct ba_atw atw;
};

struct ba_cm_xfer {
	__u64 start;
	__u32 size;
	__u8 count;
};

struct ba_docsis {
	struct ba_tp1 tp1;
	struct ba_cm_viper cm;
	struct ba_leap leap;
	struct ba_cm_dsp dsp;
	struct ba_cm_xfer xfer;
};

struct ba_arm_boot_rom {
	__u32 start;
	__u32 size;
};

struct ba_arm {
	struct ba_arm_boot_rom boot_rom;
	struct ba_atw btrace_atw;
};

struct ba_map {
	struct ba_rg_info rg;
	struct ba_stb_info stb;
	struct ba_docsis docsis;
	struct ba_arc arc[MAX_ARC_MEMC][MAX_ARC_BUS][MAX_ARC];
	struct ba_arm arm;
};

enum rg_battery_mode {
	RG_CPU_OFF_UTILITY 	= 0,
	RG_CPU_ON_UTILITY,
	RG_CPU_OFF_BATTERY,
	RG_CPU_ON_BATTERY
};

enum rg_battery_mode brcm_ba_rg_batt_mode(void);

#endif
