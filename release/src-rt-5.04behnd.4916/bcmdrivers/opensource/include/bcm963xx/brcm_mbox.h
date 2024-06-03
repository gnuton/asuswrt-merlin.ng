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

#ifndef BRCM_MBOX_H
#define BRCM_MBOX_H

#define BRCM_MBOX_IOC_MG          'M'

#define BRCM_MBOX_IOCTL_STATE _IOR(BRCM_MBOX_IOC_MG, 1, struct brcm_mbox_info)
#define BRCM_MBOX_IOCTL_SET _IOR(BRCM_MBOX_IOC_MG, 1, struct brcm_mbox_set)
#define BRCM_MBOX_IOCTL_PERM _IOR(BRCM_MBOX_IOC_MG, 1, struct brcm_mbox_perm)

#define MAX_NUM_MBOX 16 /* 16 HW MBOX's */

/* Defines for understanding which MBOX is which */
enum brcm_mb_id {
	MBOX_BMU = 0,
	MBOX_CM,
	MBOX_RG,
	MBOX_STB,
	MBOX_CM_REQ,
	MBOX_STB_RESP,
	MBOX_CM_SIZE,
	MBOX_TP1_INFO,
	MBOX_SVM,
	MBOX_ARM_BR,
	MBOX_MAX
};

static inline char *mbox_to_string(int mbox)
{
	switch (mbox) {
	case MBOX_BMU:
		return "BMU state";
	case MBOX_CM:
		return "CM state";
	case MBOX_RG:
		return "RG state";
	case MBOX_STB:
		return "STB state";
	case MBOX_CM_REQ:
		return "CM image request";
	case MBOX_STB_RESP:
		return "STB image response";
	case MBOX_CM_SIZE:
		return "CM memory size";
	case MBOX_TP1_INFO:
		return "CM TP1 memory info";
	case MBOX_SVM:
		return "SVM state";
	case MBOX_ARM_BR:
		return "ARM Boot Rom info";
	default:
		break;
	}
	return "Unknown/Invalid";
}

#define BATT_U2G_SEL_SHIFT	6
#define BATT_U2G_SEL_MASK	(0x1 << BATT_U2G_SEL_SHIFT)
#define BATT_CHIP_IN_BBM_SHIFT	5
#define BATT_CHIP_IN_BBM_MASK	(0x1 << BATT_CHIP_IN_BBM_SHIFT)
#define BATT_STATE_SHIFT	2
#define BATT_STATE_MASK		(0x3 << BATT_STATE_SHIFT)
#define BATT_STATE(x) (((x)&BATT_STATE_MASK)>>BATT_STATE_SHIFT) /* Get BMU state from u32 */
#define BATT_DISCHARGE_SHIFT	0
#define BATT_DISCHARGE_MASK	(0x1 << BATT_DISCHARGE_SHIFT)

/* Power state as reported by the BMU. As defined by spec */
enum battery_state {
	BS_UNKNOWN,
	BS_AC,
	BS_BATTERY,
	BS_LOW_BATT,
};
#define BATT_STATE_AC	(BS_AC << BATT_STATE_SHIFT)

static inline char *bmustate_to_string(enum battery_state state)
{
	switch (state) {
	case BS_UNKNOWN:
		return "Unknown";
	case BS_AC:
		return "AC Power";
	case BS_BATTERY:
		return "Battery";
	case BS_LOW_BATT:
		return "Low Battery";
	default:
		break;
	}
	return "Invalid";
}

#define POWER(x) ((x>>4)&0x3) /* RG/CM/STB power state from u32 */
#define STATE(x) (x&0xf) /* RG/CM/STB run state from u32 */

/* Power state as reported by RG/STB/SVM and CM. As defined by spec */
enum power_state {
	POWER_UNKNOWN,
	POWER_NONE,
	POWER_LOW,
	POWER_HIGH
};

static inline char *power_to_string(enum power_state state)
{
	switch (state) {
	case POWER_UNKNOWN:
		return "Unknown";
	case POWER_NONE:
		return "No Power";
	case POWER_LOW:
		return "Low Power";
	case POWER_HIGH:
		return "High Power";
	default:
		break;
	}
	return "Invalid";
}

/* CM request to RG for immediate powerdown */
#define CM_RG_POWERDOWN_REQ	(1<<12)

/* Run state as reported by CM. As defined by spec */
enum cm_state {
	CM_UNKNOWN,
	CM_ROM_BOOT,
	CM_BOOT_LOADER,
	CM_COPY_SPI,
	CM_CHECK_LOADER,
	CM_BL_INVALID,
	CM_BL_RUNNING,
	CM_WAIT_RUNTIME,
	CM_CHECK_RUNTIME,
	CM_RUNTIME_INVALID,
	CM_WAIT_DATA,
	CM_SHUTDOWN_REQ,
	CM_SHUTDOWN_READY,
	CM_EXCEPTION_SHUTDOWN_REQ,
	CM_RUNNING = 15,
};

static inline char *cmstate_to_string(enum cm_state state)
{
	switch (state) {
	case CM_UNKNOWN:
		return "Unknown";
	case CM_ROM_BOOT:
		return "Boot ROM Running";
	case CM_BOOT_LOADER:
		return "Waiting For Bootloader";
	case CM_COPY_SPI:
		return "Copying Bootloader from SPI flash";
	case CM_CHECK_LOADER:
		return "Verifying Bootloader Image";
	case CM_BL_INVALID:
		return "Bootloader Invalid (Halted)";
	case CM_BL_RUNNING:
		return "Bootloader running";
	case CM_WAIT_RUNTIME:
		return "Waiting For Runtime";
	case CM_CHECK_RUNTIME:
		return "Verifying Runtime Image";
	case CM_RUNTIME_INVALID:
		return "Runtime Image Invalid (Halted)";
	case CM_WAIT_DATA:
		return "Waiting for DATA";
	case CM_SHUTDOWN_REQ:
		return "Shutdown Requested";
	case CM_SHUTDOWN_READY:
		return "Shutdown Granted";
	case CM_EXCEPTION_SHUTDOWN_REQ:
		return "Exception occurred";
	case CM_RUNNING:
		return "Running";
	default:
		break;
	}
	return "Invalid";
}

/* Run state as reported by RG. As defined by spec */
enum rg_state {
	RG_UNKNOWN,
	RG_BOOTING, /* Only valid for A0 */
	RG_KERNEL,
	RG_SHUTDOWN_REQ,
	RG_SHUTDOWN_READY,
	RG_RUNNING = 15,
};

static inline char *rgstate_to_string(enum rg_state state)
{
	switch (state) {
	case RG_UNKNOWN:
		return "Unknown";
	case RG_BOOTING:
		return "Booting DDR";
	case RG_KERNEL:
		return "Kernel Running";
	case RG_SHUTDOWN_REQ:
		return "Shutdown Requested";
	case RG_SHUTDOWN_READY:
		return "Shutdown Granted";
	case RG_RUNNING:
		return "Running";
	default:
		break;
	}
	return "Invalid";
}

/* Run state as reported by RG. As defined by spec */
enum stb_state {
	STB_UNKNOWN,
	STB_FSBL, /* Only valid for A0 */
	STB_KERNEL,
	STB_SHUTDOWN_REQ,
	STB_SHUTDOWN_READY,
	STB_RUNNING = 15
};

static inline char *stbstate_to_string(enum stb_state state)
{
	switch (state) {
	case STB_UNKNOWN:
		return "Unknown";
	case STB_FSBL:
		return "FSBL Running";
	case STB_KERNEL:
		return "Kernel Running";
	case STB_SHUTDOWN_REQ:
		return "Shutdown Requested";
	case STB_SHUTDOWN_READY:
		return "Shutdown Granted";
	case STB_RUNNING:
		return "Running";
	default:
		break;
	}
	return "Invalid";
}

/* Run state as reported by RG. As defined by spec */
enum svm_state {
	SVM_UNKNOWN,
	SVM_FSBL,
	SVM_KERNEL,
	SVM_SHUTDOWN_REQ,
	SVM_SHUTDOWN_READY,
	SVM_RUNNING = 15
};

static inline char *svmstate_to_string(enum stb_state state)
{
	switch (state) {
	case SVM_UNKNOWN:
		return "Unknown";
	case SVM_FSBL:
		return "FSBL Running";
	case SVM_KERNEL:
		return "Kernel Running";
	case SVM_SHUTDOWN_REQ:
		return "Shutdown Requested";
	case SVM_SHUTDOWN_READY:
		return "Shutdown Granted";
	case SVM_RUNNING:
		return "Running";
	default:
		break;
	}
	return "Invalid";
}

/* Structure to set a MBOX value */
struct brcm_mbox_set {
	enum brcm_mb_id mbox;
	__u32 value;
};

/* MBOX/Run states of various parts of the system */
struct brcm_mbox_info {
	__u32 mbox[MAX_NUM_MBOX]; /* MBOX data */
	__u16 mask; /* Which MBOX's are "new" */
};

/* MBOX permission info */
struct brcm_mbox_perm {
	__u16 read; /* Which #'s can generate interrupts */
	__u16 write; /* Which #'s can be written to */
};

/* Notify state change on mbox to other register driver*/
enum brcm_mbox_event {
	MBOX_CHANGE_EVENT,
	MAX_MBOX_EVENTS
};
#ifdef __KERNEL__
int brcm_mbox_register_notifier(struct notifier_block *nb);
int brcm_mbox_unregister_notifier(struct notifier_block *nb);
void brcm_mbox_update_rg_powerdown (void);
int brcm_mbox_get_state(struct brcm_mbox_info *state);
#endif
#endif
