/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/remoteproc.h>
#include <linux/remoteproc/qcom_rproc.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-direction.h>
#include <linux/mhi.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/msi.h>
#include <linux/irq.h>

#define MHI_MAX_DEVICE 4
#define MHITEST_NUM_META_INFO_SEGMENTS		1
#define MHITEST_RAMDUMP_MAGIC			0x574C414E /* WLAN in ASCII */
#define MHITEST_RAMDUMP_VERSION_V2		2

/* MHI MMIO register mapping */
#define PCI_INVALID_READ(val) (val == U32_MAX)

/* PCI specific macros */
#define PCI_BAR_NUM                     0
#define PCI_DMA_MASK_64_BIT		64
#define MHI_TIMEOUT_DEFAULT		10000
#define PCIE_SOC_GLOBAL_RESET_V		1
#define PCIE_SOC_GLOBAL_RESET_ADDRESS	0x3008
#define PCIE_TXVECDB			0x360
#define PCIE_TXVECSTATUS		0x368
#define PCIE_RXVECDB			0x394
#define PCIE_RXVECSTATUS		0x39C

#define MHISTATUS			0x48
#define MHICTRL				0x38
#define MHICTRL_RESET_MASK		0x2

#define PCIE_SOC_GLOBAL_RESET_VALUE     0x5
#define MAX_SOC_GLOBAL_RESET_WAIT_CNT   50 /* x 20msec */

/* Add DEBUG related here  */
enum MHITEST_DEBUG_KLVL{
	MHITEST_LOG_LVL_VERBOSE,
	MHITEST_LOG_LVL_INFO,
	MHITEST_LOG_LVL_ERR,
};
extern int debug_lvl;

#define pr_mhitest(msg, ...)  pr_err("[mhitest]: " msg,  ##__VA_ARGS__)
#define pr_mhitest2(msg, ...) \
	pr_err("[mhitest]: %s[%d] " msg, __func__, __LINE__,   ##__VA_ARGS__)

#define MHITEST_EMERG(msg, ...) do {\
		pr_err("[mhitest][A]: [%s] " msg, __func__,   ##__VA_ARGS__);\
} while (0)

#define MHITEST_ERR(msg, ...) do {\
	if  (debug_lvl <= MHITEST_LOG_LVL_ERR) \
		pr_err("[mhitest][E]: [%s] " msg, __func__,   ##__VA_ARGS__);\
} while (0)

#define MHITEST_VERB(msg, ...) do {\
	if  (debug_lvl <= MHITEST_LOG_LVL_VERBOSE) \
		pr_err("[mhitest][D]: [%s][%d] " msg, __func__, __LINE__,   ##__VA_ARGS__);\
} while (0)

#define MHITEST_LOG(msg, ...) do {\
	if  (debug_lvl <= MHITEST_LOG_LVL_INFO) \
		pr_err("[mhitest][I]: [%s] " msg, __func__,   ##__VA_ARGS__);\
} while (0)

#define VERIFY_ME(val, announce)\
	do {		\
		if (val) {	\
			pr_mhitest2("%s Error val :%d\n", announce, val);\
		}		\
		else {		\
			pr_mhitest2("%s Pass!\n", announce);	\
		}	\
	} while (0)

#define QTI_PCI_VENDOR_ID		0x17CB
#define QCN90xx_DEVICE_ID		0x1104
#define QCN92XX_DEVICE_ID		0x1109

#define PCI_LINK_DOWN                   0

/*
 *Structure specific to mhitest module
 */
struct mhitest_msi_user {
	char *name;
	int num_vectors;
	u32 base_vector;
};
enum fw_dump_type {
	FW_IMAGE,
	FW_RDDM,
	FW_REMOTE_HEAP,
	FW_DUMP_TYPE_MAX,
};
/* recovery reasons using only default and rddm one for now */
enum mhitest_recovery_reason {
	MHI_DEFAULT,
	MHI_LINK_DOWN,
	MHI_RDDM,
	MHI_TIMEOUT,
};
struct mhitest_msi_config {
	int total_vectors;
	int total_users;
	struct mhitest_msi_user *users;
};
struct mhitest_dump_entry {
	u32 type;
	u32 entry_start;
	u32 entry_num;
};
struct mhitest_dump_meta_info {
	u32 magic;
	u32 version;
	u32 chipset;
	u32 total_entries;
	struct mhitest_dump_entry entry[FW_DUMP_TYPE_MAX];
};
struct mhitest_dump_seg {
	unsigned long address;
	void *v_address;
	unsigned long size;
	u32 type;
};
struct mhitest_dump_data {
	u32 version;
	u32 magic;
	char name[32];
	phys_addr_t paddr;
	int nentries;
	u32 seg_version;
};
struct mhitest_ramdump_info {
	struct ramdump_device *ramdump_dev;
	unsigned long ramdump_size;
	void *dump_data_vaddr;
	u8 dump_data_valid;
	struct mhitest_dump_data dump_data;
};
struct mhitest_platform {
	struct list_head node;
	struct platform_device *plat_dev;
	struct pci_dev *pci_dev;
	unsigned long device_id;
	const struct pci_device_id  *pci_dev_id;
	u16 def_link_speed;
	u16 def_link_width;
	u8 pci_link_state;
	struct pci_saved_state *pci_dev_default_state;
	void __iomem *bar;
	char fw_name[30];
/*mhi  msi */
	struct mhitest_msi_config *msi_config;
	u32 msi_ep_base_data;
	struct mhi_controller *mhi_ctrl;
/* subsystem related */
	char *mhitest_ss_desc_name;
	phandle rproc_handle;
	struct rproc *subsys_handle;
/* ramdump */
	struct mhitest_ramdump_info mhitest_rdinfo;
/* event work queue*/
	struct list_head event_list;
	spinlock_t event_lock; /* spinlock for driver work event handling */
	struct work_struct event_work;
	struct workqueue_struct *event_wq;
/* probed device no. 0 to (MAX-1)*/
	int d_instance;
/* klog level for mhitest driver */
	enum MHITEST_DEBUG_KLVL  mhitest_klog_lvl;
	bool soc_reset_request;
};
enum MHI_STATE {
	MHI_INIT,
	MHI_DEINIT,
	MHI_POWER_ON,
	MHI_POWER_OFF,
	MHI_FORCE_POWER_OFF,
};
enum mhitest_event_type {
	MHITEST_RECOVERY_EVENT,
	MHITEST_MAX_EVENT,
};
struct mhitest_recovery_data {
	enum mhitest_recovery_reason reason;
};
struct mhitest_driver_event {
	struct list_head list;
	enum mhitest_event_type type;
	bool sync;
	struct completion complete;
	int ret;
	void *data;
};

int mhitest_pci_register(void);
int mhitest_subsystem_register(struct mhitest_platform *);
void mhitest_pci_unregister(void);
void mhitest_subsystem_unregister(struct mhitest_platform *);
int mhitest_pci_enable_bus(struct mhitest_platform *);
struct mhitest_platform *get_mhitest_mplat_by_pcidev(struct pci_dev *pci_dev);
int mhitest_pci_en_msi(struct mhitest_platform *);
int mhitest_pci_register_mhi(struct mhitest_platform *);
int mhitest_pci_get_mhi_msi(struct mhitest_platform *);
int mhitest_pci_get_link_status(struct mhitest_platform *);
int mhitest_prepare_pci_mhi_msi(struct mhitest_platform *);
int mhitest_prepare_start_mhi(struct mhitest_platform *);
int mhitest_dump_info(struct mhitest_platform *mplat, bool in_panic);
int mhitest_dev_ramdump(struct mhitest_platform *mplat);
int mhitest_post_event(struct mhitest_platform *,
	struct mhitest_recovery_data *, enum mhitest_event_type, u32 flags);
struct platform_device *get_plat_device(void);
void mhitest_store_mplat(struct mhitest_platform *);
void mhitest_remove_mplat(struct mhitest_platform *);
void mhitest_free_mplat(struct mhitest_platform *);
int mhitest_event_work_init(struct mhitest_platform *);
void mhitest_event_work_deinit(struct mhitest_platform *);
int mhitest_pci_start_mhi(struct mhitest_platform *);
void mhitest_global_soc_reset(struct mhitest_platform *);
int mhitest_ss_powerup(struct rproc *);
int mhitest_pci_set_mhi_state(struct mhitest_platform *, enum MHI_STATE);
void mhitest_pci_disable_bus(struct mhitest_platform *);
int mhitest_unregister_ramdump(struct mhitest_platform *);
int mhitest_pci_remove_all(struct mhitest_platform *);
