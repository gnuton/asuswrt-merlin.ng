// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.*/

#include <asm/arch_timer.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-direction.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/memblock.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mhi.h>
#include <linux/msi.h>
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/io.h>
#include <linux/gpio/consumer.h>
#include <linux/of_gpio.h>
#include "mhi_qti.h"
#include "../core/internal.h"
#include "../../../pci/controller/dwc/pcie-designware.h"

#define WDOG_TIMEOUT	30
#define MHI_PANIC_TIMER_STEP	1000
volatile int mhi_panic_timeout;
volatile int force_graceful = 0;
volatile int force_pcie_ssr = 0;
volatile int ssr_mode = 0;

static int ap2mdm_gpio, mdm2ap_gpio;

#define	SDX55	0x0306
#define	SDX65	0x0308

bool mhi_ssr_negotiate;

static int mhi_nr_tre_update[50] = { -1 };
static int mhi_nr_tre_update_argc = 0;
module_param_array(mhi_nr_tre_update, int, &mhi_nr_tre_update_argc, 0644);
MODULE_PARM_DESC(mhi_nr_tre_update, "MHI nr_tre update array");

void __iomem *wdt;

static struct kobject *mhi_kobj;

struct notifier_block *global_mhi_panic_notifier;

static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf);
static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t count);

static struct kobj_attribute mhi_attr =
	__ATTR(mhi_panic_timeout, 0660, sysfs_show, sysfs_store);

static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", mhi_panic_timeout);
}

static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	if (sscanf(buf, "%du", &mhi_panic_timeout) != 1) {
		pr_err("failed to read timeout value from string\n");
		return -EINVAL;
	}
	return count;
}

static ssize_t force_graceful_sysfs_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", force_graceful);
}

static ssize_t force_graceful_sysfs_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	if (sscanf(buf, "%du", &force_graceful) != 1) {
		pr_err("failed to read force graceful value from string\n");
		return -EINVAL;
	}
	return count;
}
static struct kobj_attribute force_graceful_attr =
	__ATTR(force_graceful, 0660, force_graceful_sysfs_show, force_graceful_sysfs_store);

struct firmware_info {
	unsigned int dev_id;
	const char *fw_image;
	const char *edl_image;
};

/* set ptr to control private data */
static inline void mhi_controller_set_devdata(struct mhi_controller *mhi_cntrl,
					      void *priv)
{
	mhi_cntrl->priv_data = priv;
}

static struct mhi_channel_config mhi_sdx_mhi_channels[] = {
	{
		.num = 0,
		.name = "LOOPBACK",
		.num_elements = 32,
		.event_ring = 0,
		.dir = DMA_TO_DEVICE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.doorbell = MHI_DB_BRST_DISABLE,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 1,
		.name = "LOOPBACK",
		.num_elements = 32,
		.event_ring = 0,
		.dir = DMA_FROM_DEVICE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.doorbell = MHI_DB_BRST_DISABLE,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 2,
		.name = "SAHARA",
		.num_elements = 64,
		.event_ring = 1,
		.dir = DMA_TO_DEVICE,
		.ee_mask = BIT(MHI_EE_SBL),
		.pollcfg = 0,
		.doorbell = MHI_DB_BRST_DISABLE,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 3,
		.name = "SAHARA",
		.num_elements = 64,
		.event_ring = 1,
		.dir = DMA_FROM_DEVICE,
		.ee_mask = BIT(MHI_EE_SBL),
		.pollcfg = 0,
		.doorbell = MHI_DB_BRST_DISABLE,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 4,
		.name = "DIAG",
		.num_elements = 128,
		.event_ring = 2,
		.dir = DMA_TO_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 5,
		.name = "DIAG",
		.num_elements = 128,
		.event_ring = 2,
		.dir = DMA_FROM_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 14,
		.name = "QMI0",
		.num_elements = 64,
		.event_ring = 1,
		.dir = DMA_TO_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 15,
		.name = "QMI0",
		.num_elements = 64,
		.event_ring = 2,
		.dir = DMA_FROM_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 16,
		.name = "QMI1",
		.num_elements = 64,
		.event_ring = 3,
		.dir = DMA_TO_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 17,
		.name = "QMI1",
		.num_elements = 64,
		.event_ring = 3,
		.dir = DMA_FROM_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 18,
		.name = "IP_CTRL",
		.num_elements = 64,
		.event_ring = 1,
		.dir = DMA_TO_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 19,
		.name = "IP_CTRL",
		.num_elements = 64,
		.event_ring = 1,
		.dir = DMA_FROM_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = true,
		.auto_start = false,
	},
	{
		.num = 20,
		.name = "IPCR",
		.num_elements = 64,
		.event_ring = 2,
		.dir = DMA_TO_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = true,
	},
	{
		.num = 21,
		.name = "IPCR",
		.num_elements = 64,
		.event_ring = 2,
		.dir = DMA_FROM_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = true,
		.auto_start = true,
	},
	{
		.num = 32,
		.name = "DUN",
		.num_elements = 32,
		.event_ring = 3,
		.dir = DMA_TO_DEVICE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.doorbell = MHI_DB_BRST_DISABLE,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 33,
		.name = "DUN",
		.num_elements = 32,
		.event_ring = 3,
		.dir = DMA_FROM_DEVICE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.doorbell = MHI_DB_BRST_DISABLE,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 34,
		.name = "FIREHOSE",
		.num_elements = 32,
		.event_ring = 1,
		.dir = DMA_TO_DEVICE,
		.ee_mask = BIT(MHI_EE_FP),
		.pollcfg = 0,
		.doorbell = MHI_DB_BRST_DISABLE,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 35,
		.name = "FIREHOSE",
		.num_elements = 32,
		.event_ring = 1,
		.dir = DMA_FROM_DEVICE,
		.ee_mask = BIT(MHI_EE_FP),
		.pollcfg = 0,
		.doorbell = MHI_DB_BRST_DISABLE,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 46,
		.name = "IP_SW0",
		.num_elements = 512,
		.event_ring = 4,
		.dir = DMA_TO_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 47,
		.name = "IP_SW0",
		.num_elements = 512,
		.event_ring = 5,
		.dir = DMA_FROM_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 100,
		.name = "IP_HW0",
		.num_elements = 512,
		.event_ring = 6,
		.dir = DMA_TO_DEVICE,
		.doorbell = MHI_DB_BRST_ENABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = true,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 101,
		.name = "IP_HW0",
		.num_elements =  512,
		.event_ring = 7,
		.dir = DMA_FROM_DEVICE,
		.doorbell = MHI_DB_BRST_ENABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 103,
		.name = "IP_HW_QDSS",
		.num_elements =  128,
		.event_ring = 8,
		.dir = DMA_FROM_DEVICE,
		.doorbell = MHI_DB_BRST_ENABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},

	{
		.num = 105,
		.name = "RMNET_CTL",
		.num_elements =  128,
		.event_ring = 9,
		.dir = DMA_TO_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},
	{
		.num = 106,
		.name = "RMNET_CTL",
		.num_elements =  128,
		.event_ring = 10,
		.dir = DMA_FROM_DEVICE,
		.doorbell = MHI_DB_BRST_DISABLE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = false,
		.auto_start = false,
	},

};

static struct mhi_event_config mhi_sdx_mhi_events[] = {
	{
		.num_elements = 32,
		.irq_moderation_ms = 1,
		.irq = 1,
		.mode = MHI_DB_BRST_DISABLE,
		.data_type = MHI_ER_CTRL,
		.hardware_event = false,
		.client_managed = false,
		.offload_channel = false,
	},
	{
		.num_elements = 256,
		.irq_moderation_ms = 1,
		.irq = 2,
		.mode = MHI_DB_BRST_DISABLE,
		.hardware_event = false,
		.client_managed = false,
		.offload_channel = false,
	},
	{
		.num_elements = 256,
		.irq_moderation_ms = 1,
		.irq = 3,
		.mode = MHI_DB_BRST_DISABLE,
		.hardware_event = false,
		.client_managed = false,
		.offload_channel = false,
	},
	{
		.num_elements = 256,
		.irq_moderation_ms = 1,
		.irq = 4,
		.mode = MHI_DB_BRST_DISABLE,
		.hardware_event = false,
		.client_managed = false,
		.offload_channel = false,
	},
	{
		.num_elements = 1024,
		.irq_moderation_ms = 5,
		.irq = 5,
		.channel = 46,
		.mode = MHI_DB_BRST_DISABLE,
		.hardware_event = false,
		.client_managed = false,
		.offload_channel = false,
	},
	{
		.num_elements = 1024,
		.irq_moderation_ms = 5,
		.irq = 6,
		.channel = 47,
		.mode = MHI_DB_BRST_DISABLE,
		.hardware_event = false,
		.client_managed = true,
		.offload_channel = false,
	},
	{
		.num_elements = 1024,
		.irq_moderation_ms = 5,
		.irq = 5,
		.channel = 100,
		.mode = MHI_DB_BRST_ENABLE,
		.hardware_event = true,
		.client_managed = false,
		.offload_channel = false,
	},
	{
		.num_elements = 1024,
		.irq_moderation_ms = 5,
		.irq = 6,
		.channel = 101,
		.mode = MHI_DB_BRST_ENABLE,
		.hardware_event = true,
		.client_managed = true,
		.offload_channel = false,
	},

	{
		.num_elements = 1024,
		.irq_moderation_ms = 5,
		.irq = 7,
		.channel = 103,
		.mode = MHI_DB_BRST_ENABLE,
		.hardware_event = true,
		.client_managed = false,
		.offload_channel = false,
	},

	{
		.num_elements = 1024,
		.irq_moderation_ms = 1,
		.irq = 8,
		.channel = 105,
		.mode = MHI_DB_BRST_DISABLE,
		.hardware_event = true,
		.client_managed = false,
		.offload_channel = false,
	},
	{
		.num_elements = 1024,
		.irq_moderation_ms = 0,
		.irq = 9,
		.channel = 106,
		.mode = MHI_DB_BRST_DISABLE,
		.hardware_event = true,
		.client_managed = false,
		.offload_channel = false,
	},
};

static struct mhi_controller_config mhi_sdx_mhi_config = {
	.max_channels = 128,
	.timeout_ms = 6000,
	.use_bounce_buf = false,
	.buf_len = 0,
	.num_channels = ARRAY_SIZE(mhi_sdx_mhi_channels),
	.ch_cfg = mhi_sdx_mhi_channels,
	.num_events = ARRAY_SIZE(mhi_sdx_mhi_events),
	.event_cfg = mhi_sdx_mhi_events,
};

static const struct firmware_info firmware_table[] = {
	{.dev_id = 0x306, .fw_image = "sdx55m/sbl1.mbn"},
	{.dev_id = 0x305, .fw_image = "sdx50m/sbl1.mbn"},
	{.dev_id = 0x304, .fw_image = "sbl.mbn", .edl_image = "edl.mbn"},
	{.dev_id = 0x308, .edl_image = "edl.mbn"},
	/* default, set to debug.mbn */
	{.fw_image = "debug.mbn"},
};

static int debug_mode;
module_param_named(debug_mode, debug_mode, int, 0644);

int mhi_debugfs_trigger_m0(void *data, u64 val)
{
#ifdef CONFIG_PM
	struct mhi_controller *mhi_cntrl = data;
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);

	MHI_LOG("Trigger M3 Exit\n");
	pm_runtime_get(&mhi_dev->pci_dev->dev);
	pm_runtime_put(&mhi_dev->pci_dev->dev);
#endif

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(debugfs_trigger_m0_fops, NULL,
			mhi_debugfs_trigger_m0, "%llu\n");

int mhi_debugfs_trigger_m3(void *data, u64 val)
{
#ifdef CONFIG_PM
	struct mhi_controller *mhi_cntrl = data;
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);

	MHI_LOG("Trigger M3 Entry\n");
	pm_runtime_mark_last_busy(&mhi_dev->pci_dev->dev);
	pm_request_autosuspend(&mhi_dev->pci_dev->dev);
#endif

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(debugfs_trigger_m3_fops, NULL,
			mhi_debugfs_trigger_m3, "%llu\n");

void mhi_deinit_pci_dev(struct mhi_controller *mhi_cntrl)
{
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct pci_dev *pci_dev = mhi_dev->pci_dev;

#ifdef CONFIG_PM
	pm_runtime_mark_last_busy(&pci_dev->dev);
	pm_runtime_dont_use_autosuspend(&pci_dev->dev);
	pm_runtime_disable(&pci_dev->dev);
#endif
	pci_free_irq_vectors(pci_dev);
	kfree(mhi_cntrl->irq);
	mhi_cntrl->irq = NULL;
	iounmap(mhi_cntrl->regs);
	iounmap(wdt);
	mhi_cntrl->regs = NULL;
	pci_clear_master(pci_dev);
	pci_release_region(pci_dev, mhi_dev->resn);
	pci_disable_device(pci_dev);
}

static int mhi_init_pci_dev(struct mhi_controller *mhi_cntrl)
{
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct pci_dev *pci_dev = mhi_dev->pci_dev;
	int ret;
	resource_size_t len;
	int i, nr_vectors;

	mhi_dev->resn = MHI_PCI_BAR_NUM;
	ret = pci_assign_resource(pci_dev, mhi_dev->resn);
	if (ret) {
		MHI_ERR("Error assign pci resources, ret:%d\n", ret);
		return ret;
	}

	ret = pci_enable_device(pci_dev);
	if (ret) {
		MHI_ERR("Error enabling device, ret:%d\n", ret);
		goto error_enable_device;
	}

	ret = pci_request_region(pci_dev, mhi_dev->resn, "mhi");
	if (ret) {
		MHI_ERR("Error pci_request_region, ret:%d\n", ret);
		goto error_request_region;
	}

	pci_set_master(pci_dev);

	mhi_cntrl->base_addr = pci_resource_start(pci_dev, mhi_dev->resn);
	len = pci_resource_len(pci_dev, mhi_dev->resn);
	mhi_cntrl->regs = ioremap_nocache(mhi_cntrl->base_addr, len);
	if (!mhi_cntrl->regs) {
		MHI_ERR("Error ioremap region\n");
		goto error_ioremap;
	}

	/*
	 * Alloc one MSI vector for BHI + one vector per event ring, ideally...
	 */
	mhi_cntrl->nr_irqs = mhi_sdx_mhi_config.num_events + 1;
	mhi_cntrl->nr_irqs = 1U << get_count_order(mhi_cntrl->nr_irqs);

	nr_vectors = pci_alloc_irq_vectors(pci_dev, 1, mhi_cntrl->nr_irqs,
				    PCI_IRQ_MSI);
	if (nr_vectors < 0) {
		MHI_ERR("Error allocating MSI vectors, ret:%d\n", nr_vectors);
		goto error_req_msi;
	}

	if (nr_vectors < mhi_cntrl->nr_irqs) {
		MHI_LOG("Not enough MSI vectors (%d/%d), use shared MHI\n",
			nr_vectors, mhi_sdx_mhi_config.num_events);
	}

	mhi_cntrl->irq = kmalloc_array(mhi_cntrl->nr_irqs,
				       sizeof(*mhi_cntrl->irq), GFP_KERNEL);
	if (!mhi_cntrl->irq) {
		ret = -ENOMEM;
		goto error_alloc_msi_vec;
	}

	for (i = 0; i < mhi_cntrl->nr_irqs; i++) {
		int vector = i >= nr_vectors ? (nr_vectors - 1) : i;
		mhi_cntrl->irq[i] = pci_irq_vector(pci_dev, vector);
		if (mhi_cntrl->irq[i] < 0) {
			ret = mhi_cntrl->irq[i];
			goto error_get_irq_vec;
		}
	}

	dev_set_drvdata(&pci_dev->dev, mhi_cntrl);

#ifdef CONFIG_PM
	/* configure runtime pm */
	pm_runtime_set_autosuspend_delay(&pci_dev->dev, MHI_RPM_SUSPEND_TMR_MS);
	pm_runtime_use_autosuspend(&pci_dev->dev);
	pm_suspend_ignore_children(&pci_dev->dev, true);

	/*
	 * pci framework will increment usage count (twice) before
	 * calling local device driver probe function.
	 * 1st pci.c pci_pm_init() calls pm_runtime_forbid
	 * 2nd pci-driver.c local_pci_probe calls pm_runtime_get_sync
	 * Framework expect pci device driver to call
	 * pm_runtime_put_noidle to decrement usage count after
	 * successful probe and and call pm_runtime_allow to enable
	 * runtime suspend.
	 */
	pm_runtime_mark_last_busy(&pci_dev->dev);
	pm_runtime_put_noidle(&pci_dev->dev);
#endif

	return 0;

error_get_irq_vec:
	kfree(mhi_cntrl->irq);
	mhi_cntrl->irq = NULL;

error_alloc_msi_vec:
	pci_free_irq_vectors(pci_dev);

error_req_msi:
	iounmap(mhi_cntrl->regs);

error_ioremap:
	pci_clear_master(pci_dev);

error_request_region:
	pci_disable_device(pci_dev);

error_enable_device:
	pci_release_region(pci_dev, mhi_dev->resn);

	return ret;
}

#ifdef CONFIG_PM
static int mhi_runtime_suspend(struct device *dev)
{
	int ret = 0;
	struct mhi_controller *mhi_cntrl = dev_get_drvdata(dev);
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);

	MHI_LOG("Enter\n");

	mutex_lock(&mhi_cntrl->pm_mutex);

	if (!mhi_dev->powered_on) {
		MHI_LOG("Not fully powered, return success\n");
		mutex_unlock(&mhi_cntrl->pm_mutex);
		return 0;
	}

	ret = mhi_pm_suspend(mhi_cntrl);
	if (ret) {
		MHI_LOG("Abort due to ret:%d\n", ret);
		goto exit_runtime_suspend;
	}

	ret = mhi_arch_link_off(mhi_cntrl, true);
	if (ret)
		MHI_ERR("Failed to Turn off link ret:%d\n", ret);

exit_runtime_suspend:
	mutex_unlock(&mhi_cntrl->pm_mutex);
	MHI_LOG("Exited with ret:%d\n", ret);

	return ret;
}

static int mhi_runtime_idle(struct device *dev)
{
	MHI_LOG("Entered returning -EBUSY\n");

	/*
	 * RPM framework during runtime resume always calls
	 * rpm_idle to see if device ready to suspend.
	 * If dev.power usage_count count is 0, rpm fw will call
	 * rpm_idle cb to see if device is ready to suspend.
	 * if cb return 0, or cb not defined the framework will
	 * assume device driver is ready to suspend;
	 * therefore, fw will schedule runtime suspend.
	 * In MHI power management, MHI host shall go to
	 * runtime suspend only after entering MHI State M2, even if
	 * usage count is 0.  Return -EBUSY to disable automatic suspend.
	 */
	return -EBUSY;
}

static int mhi_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct mhi_controller *mhi_cntrl = dev_get_drvdata(dev);
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);

	MHI_LOG("Enter\n");

	mutex_lock(&mhi_cntrl->pm_mutex);

	if (!mhi_dev->powered_on) {
		MHI_LOG("Not fully powered, return success\n");
		mutex_unlock(&mhi_cntrl->pm_mutex);
		return 0;
	}

	/* turn on link */
	ret = mhi_arch_link_on(mhi_cntrl);
	if (ret)
		goto rpm_resume_exit;

	/* enter M0 state */
	ret = mhi_pm_resume(mhi_cntrl);

rpm_resume_exit:
	mutex_unlock(&mhi_cntrl->pm_mutex);
	MHI_LOG("Exited with :%d\n", ret);

	return ret;
}

static int mhi_system_resume(struct device *dev)
{
	int ret = 0;

	ret = mhi_runtime_resume(dev);
	if (ret) {
		MHI_ERR("Failed to resume link\n");
	} else {
		pm_runtime_set_active(dev);
		pm_runtime_enable(dev);
	}

	return ret;
}

int mhi_system_suspend(struct device *dev)
{
	int ret;

	MHI_LOG("Entered\n");

	/* if rpm status still active then force suspend */
	if (!pm_runtime_status_suspended(dev)) {
		ret = mhi_runtime_suspend(dev);
		if (ret) {
			MHI_LOG("suspend failed ret:%d\n", ret);
			return ret;
		}
	}

	pm_runtime_set_suspended(dev);
	pm_runtime_disable(dev);

	MHI_LOG("Exit\n");
	return 0;
}
#endif

static int mhi_power_up(struct mhi_controller *mhi_cntrl)
{
	enum mhi_state dev_state = mhi_get_mhi_state(mhi_cntrl);
	const u32 delayus = 10;
	int itr = DIV_ROUND_UP(mhi_cntrl->timeout_ms * 1000, delayus);
	int ret;

	/*
	 * It's possible device did not go thru a cold reset before
	 * power up and still in error state. If device in error state,
	 * we need to trigger a soft reset before continue with power
	 * up
	 */
	if (dev_state == MHI_STATE_SYS_ERR) {
		mhi_set_mhi_state(mhi_cntrl, MHI_STATE_RESET);
		while (itr--) {
			dev_state = mhi_get_mhi_state(mhi_cntrl);
			if (dev_state != MHI_STATE_SYS_ERR)
				break;
			mhi_set_mhi_state(mhi_cntrl, MHI_STATE_RESET);
			usleep_range(delayus, delayus << 1);
		}
		/* device still in error state, abort power up */
		if (dev_state == MHI_STATE_SYS_ERR)
			return -EIO;
	}

	ret = mhi_async_power_up(mhi_cntrl);

	return ret;
}

int __must_check mhi_sdx_read_reg(struct mhi_controller *mhi_cntrl,
                                  void __iomem *addr, u32 *out)
{
        u32 tmp = readl(addr);

        /* If the value is invalid, the link is down */
        if (tmp == U32_MAX)
                return -EIO;

        *out = tmp;

        return 0;
}

void mhi_sdx_write_reg(struct mhi_controller *mhi_cntrl, void __iomem *addr,
                       u32 val)
{
        writel(val, addr);
}

#ifdef CONFIG_PM
static int mhi_runtime_get(struct mhi_controller *mhi_cntrl)
{
	struct mhi_dev *mhi_dev = mhi_cntrl->priv_data;
	struct device *dev = &mhi_dev->pci_dev->dev;

	return pm_runtime_get(dev);
}

static void mhi_runtime_put(struct mhi_controller *mhi_cntrl)
{
	struct mhi_dev *mhi_dev = mhi_cntrl->priv_data;
	struct device *dev = &mhi_dev->pci_dev->dev;

	pm_runtime_put_noidle(dev);
}

static void mhi_status_cb(struct mhi_controller *mhi_cntrl,
			  enum mhi_callback reason)
{
	struct mhi_dev *mhi_dev = mhi_cntrl->priv_data;
	struct device *dev = &mhi_dev->pci_dev->dev;

	if (reason == MHI_CB_IDLE) {
		MHI_LOG("Schedule runtime suspend 1\n");
		pm_runtime_mark_last_busy(dev);
		pm_request_autosuspend(dev);
	}
}
#endif

static ssize_t timeout_ms_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct mhi_device *mhi_dev = to_mhi_device(dev);
	struct mhi_controller *mhi_cntrl = mhi_dev->mhi_cntrl;

	/* buffer provided by sysfs has a minimum size of PAGE_SIZE */
	return snprintf(buf, PAGE_SIZE, "%u\n", mhi_cntrl->timeout_ms);
}

static ssize_t timeout_ms_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	struct mhi_device *mhi_dev = to_mhi_device(dev);
	struct mhi_controller *mhi_cntrl = mhi_dev->mhi_cntrl;
	u32 timeout_ms;

	if (kstrtou32(buf, 0, &timeout_ms) < 0)
		return -EINVAL;

	mhi_cntrl->timeout_ms = timeout_ms;

	return count;
}
static DEVICE_ATTR_RW(timeout_ms);

static ssize_t power_up_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf,
			      size_t count)
{
	int ret;
	struct mhi_device *mhi_dev = to_mhi_device(dev);
	struct mhi_controller *mhi_cntrl = mhi_dev->mhi_cntrl;

	ret = mhi_power_up(mhi_cntrl);
	if (ret)
		return ret;

	return count;
}
static DEVICE_ATTR_WO(power_up);

static struct attribute *mhi_attrs[] = {
	&dev_attr_timeout_ms.attr,
	&dev_attr_power_up.attr,
	NULL
};

static const struct attribute_group mhi_group = {
	.attrs = mhi_attrs,
};

/* allocate mhi controller to register */
struct mhi_controller *mhi_alloc_dev_and_controller(size_t size)
{
	struct mhi_controller *mhi_cntrl;

	mhi_cntrl = kzalloc(size + sizeof(*mhi_cntrl), GFP_KERNEL);

	if (mhi_cntrl && size)
		mhi_controller_set_devdata(mhi_cntrl, mhi_cntrl + 1);

	return mhi_cntrl;
}

static struct mhi_controller *dt_register_mhi_controller(struct pci_dev *pci_dev)
{
	struct mhi_controller *mhi_cntrl;
	struct mhi_dev *mhi_dev;
	struct device_node *of_node = pci_dev->dev.of_node;
	bool use_bb;
	u64 addr_win[2];
	int ret;

	if (!of_node)
		return ERR_PTR(-ENODEV);

	mhi_cntrl = mhi_alloc_dev_and_controller(sizeof(*mhi_dev));
	if (!mhi_cntrl)
		return ERR_PTR(-ENOMEM);

	mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	mhi_cntrl->cntrl_dev = &pci_dev->dev;

	use_bb = of_property_read_bool(of_node, "mhi,use-bb");

	/*
	 * if s1 translation enabled or using bounce buffer pull iova addr
	 * from dt
	 */
	if (use_bb || (mhi_dev->smmu_cfg & MHI_SMMU_ATTACH &&
		       !(mhi_dev->smmu_cfg & MHI_SMMU_S1_BYPASS))) {
		ret = of_property_count_elems_of_size(of_node, "qti,addr-win",
						      sizeof(addr_win));
		if (ret != 1)
			return ERR_PTR(-EINVAL);
		ret = of_property_read_u64_array(of_node, "qti,addr-win",
						 addr_win, 2);
		if (ret)
			return ERR_PTR(-EINVAL);
	} else {
		addr_win[0] = memblock_start_of_DRAM();
		addr_win[1] = memblock_end_of_DRAM();
	}

	mhi_dev->iova_start = addr_win[0];
	mhi_dev->iova_stop = addr_win[1];

	/*
	 * If S1 is enabled, set MHI_CTRL start address to 0 so we can use low
	 * level mapping api to map buffers outside of smmu domain
	 */
	if (mhi_dev->smmu_cfg & MHI_SMMU_ATTACH &&
	    !(mhi_dev->smmu_cfg & MHI_SMMU_S1_BYPASS))
		mhi_cntrl->iova_start = 0;
	else
		mhi_cntrl->iova_start = addr_win[0];

	mhi_cntrl->iova_stop = mhi_dev->iova_stop;
	mhi_cntrl->of_node = of_node;

	mhi_dev->pci_dev = pci_dev;

#ifdef CONFIG_PM
	/* setup power management apis */
	mhi_cntrl->status_cb = mhi_status_cb;
	mhi_cntrl->runtime_get = mhi_runtime_get;
	mhi_cntrl->runtime_put = mhi_runtime_put;
#endif
	mhi_cntrl->read_reg = mhi_sdx_read_reg;
	mhi_cntrl->write_reg = mhi_sdx_write_reg;

	return mhi_cntrl;
}

static int mhi_panic_handler(struct notifier_block *this,
			     unsigned long event, void *ptr)
{
	int mdmreboot = 0, i;
	struct gpio_desc *ap2mdm = NULL, *mdm2ap = NULL;
	struct mhi_controller *mhi_cntrl = container_of(this, struct mhi_controller, mhi_panic_notifier);

	pr_debug("SSR_mode = %d \n", ssr_mode);
	if (ssr_mode) {
		ap2mdm = gpio_to_desc(ap2mdm_gpio);
		if (IS_ERR(ap2mdm))
			return PTR_ERR(ap2mdm);

		mdm2ap = gpio_to_desc(mdm2ap_gpio);
		if (IS_ERR(mdm2ap))
			return PTR_ERR(mdm2ap);

		/*
		 * ap2mdm_status is set to 0 to indicate the SDX
		 * that IPQ has crashed. Now the SDX has to take
		 * dump.
		 */
		gpiod_set_value(ap2mdm, 0);
		pr_debug("HOST is going down.\n");
	}
	if (mhi_panic_timeout) {
		if (mhi_panic_timeout > WDOG_TIMEOUT) {
			pr_debug("Panic timeout set = %d \n", mhi_panic_timeout);
			writel(0, wdt);
		}

		for (i = 0; i < mhi_panic_timeout; i++) {
			/*
			 * Waiting for the mdm2ap status to be 0
			 * which indicates that SDX is rebooting and entering
			 * the crashdump path.
			 */
			if (!mdmreboot &&
			    (ssr_mode ? !gpiod_get_value(mdm2ap) : mhi_cntrl->ee == MHI_EE_RDDM)) {
				MHI_LOG("MDM is rebooting and entering the crashdump path\n");
				pr_debug("value(mdm2ap) = %d\n", gpiod_get_value(mdm2ap));
				mdmreboot = 1;
			}

			/*
			 * Waiting for the mdm2ap status to be 1
			 * which indicates that SDX has completed crashdump
			 * collection and booted successfully.
			 */
			if (mdmreboot &&
			    (ssr_mode ? gpiod_get_value(mdm2ap) : mhi_cntrl->ee == MHI_EE_AMSS)) {
				MHI_LOG("MDM has completed crashdump collection and booted successfully\n");
				pr_debug("value(mdm2ap) = %d\n", gpiod_get_value(mdm2ap));
				break;
			}


			mdelay(MHI_PANIC_TIMER_STEP);
		}

		if (mhi_panic_timeout > WDOG_TIMEOUT) {
			pr_debug("Enabling Watchdog once SDX cdump is collection done and Modem is ON \n");
			writel(1, wdt);
		}
	}

	return NOTIFY_DONE;
}

void mhi_wdt_panic_handler(void)
{
	mhi_panic_handler(global_mhi_panic_notifier,
			0, NULL);
}
EXPORT_SYMBOL(mhi_wdt_panic_handler);

static int __init pcie_ssr(char *str)
{
	force_pcie_ssr = 1;
	pr_info("forcing pcie_ssr\n");
	return 0;
}

early_param("force_pcie_ssr", pcie_ssr);

int mhi_pci_probe(struct pci_dev *pci_dev,
		  const struct pci_device_id *device_id)
{
	struct mhi_controller *mhi_cntrl;
	const struct firmware_info *firmware_info;
	struct mhi_dev *mhi_dev;
	int i, ret;

	/* update nr_tre for ch & event cfg via bootargs */
	if (mhi_nr_tre_update[0] != -1) {
		int idx = 0;
		for (i = 0; i < mhi_sdx_mhi_config.num_channels; i++) {
			if (mhi_sdx_mhi_config.ch_cfg[i].num ==
					mhi_nr_tre_update[idx]) {
				mhi_sdx_mhi_channels[i].num_elements =
					mhi_nr_tre_update[idx+1];
				idx += 2;
				if (idx == mhi_nr_tre_update_argc)
					break;
			}
		}

		idx = 0;
		for (i = 0; i < mhi_sdx_mhi_config.num_events; i++) {
			if (mhi_sdx_mhi_config.event_cfg[i].channel ==
					mhi_nr_tre_update[idx]) {
				mhi_sdx_mhi_events[i].num_elements =
					(mhi_nr_tre_update[idx+1] << 1);

				idx += 2;
				if (idx == mhi_nr_tre_update_argc)
					break;
			}
		}
	}

	/* Fix me: Add check to see if already registered */
	mhi_cntrl = dt_register_mhi_controller(pci_dev);
	if (IS_ERR(mhi_cntrl))
		return PTR_ERR(mhi_cntrl);

	mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	mhi_dev->powered_on = true;

	ret = mhi_arch_pcie_init(mhi_cntrl);
	if (ret)
		return ret;

	mhi_cntrl->cntrl_dev = &pci_dev->dev;
	ret = mhi_init_pci_dev(mhi_cntrl);
	if (ret)
		goto error_init_pci;

	mhi_cntrl->dev_id = pci_dev->device;
	ret = mhi_register_controller(mhi_cntrl, &mhi_sdx_mhi_config);
	if (ret)
		goto error_register;

	for (i = 0; i < ARRAY_SIZE(firmware_table); i++) {
		firmware_info = firmware_table + i;
		/* debug mode always use default */
		if (!debug_mode && pci_dev->device == firmware_info->dev_id)
			break;
	}
	mhi_cntrl->fw_image = firmware_info->fw_image;
	mhi_cntrl->edl_image = firmware_info->edl_image;

	if (sysfs_create_group(&mhi_cntrl->mhi_dev->dev.kobj, &mhi_group))
		MHI_ERR("Error while creating the sysfs group\n");

	/* start power up sequence */
	if (!debug_mode) {
		ret = mhi_power_up(mhi_cntrl);
		if (ret)
			goto error_power_up;
	}

#ifdef CONFIG_PM
	pm_runtime_mark_last_busy(&pci_dev->dev);
	pm_runtime_allow(&pci_dev->dev);
#endif

	mhi_ssr_negotiate = of_property_read_bool(mhi_cntrl->of_node, "mhi,ssr-negotiate");

	pr_debug("mhi_ssr_negotiate status for GPIO based SSR = %d\n", mhi_ssr_negotiate);
	ap2mdm_gpio = of_get_named_gpio(mhi_cntrl->of_node, "ap2mdm-gpio", 0);
	mdm2ap_gpio = of_get_named_gpio(mhi_cntrl->of_node, "mdm2ap-gpio", 0);

	if (of_property_read_bool(mhi_cntrl->of_node, "ap2mdm-x65-gpio") &&
	    of_property_read_bool(mhi_cntrl->of_node, "mdm2ap-x65-gpio") &&
	    SDX65 == pci_dev->device) {
		ap2mdm_gpio = of_get_named_gpio(mhi_cntrl->of_node, "ap2mdm-x65-gpio", 0);
		mdm2ap_gpio = of_get_named_gpio(mhi_cntrl->of_node, "mdm2ap-x65-gpio", 0);
	}

	if (ap2mdm_gpio < 0 && mdm2ap_gpio < 0) {
		pr_err("AP2MDM and MDMD2AP GPIO not configured\n");
	} else {
		pr_debug("Acquired ssr gpios ap2mdm_gpio = %d mdm2ap_gpio = %d" ,
			 ap2mdm_gpio, mdm2ap_gpio);
		/*
		 * If ap2mdm and mdm2ap are available,
		 * then check whether to proceed in PCIE or
		 * GPIO based SSR. Else default is PCIE based
		 * SSR.
		 * ssr_mode - 0-PCIE, 1-GPIO
		 */
		ssr_mode = (ap2mdm_gpio && mdm2ap_gpio) &&
			   (force_pcie_ssr ? 0 : 1);
	}

	MHI_LOG(" SSR mode selected = %d", ssr_mode);

	if (mhi_ssr_negotiate) {
		mhi_cntrl->mhi_panic_notifier.notifier_call = mhi_panic_handler;

		global_mhi_panic_notifier = &(mhi_cntrl->mhi_panic_notifier);

		ret = atomic_notifier_chain_register(&panic_notifier_list,
				&mhi_cntrl->mhi_panic_notifier);
		MHI_LOG("MHI panic notifier registered\n");

		wdt = ioremap(0x0B017008, 4);

		/* Creating a directory in /sys/kernel/ */
		mhi_kobj = kobject_create_and_add("mhi", kernel_kobj);

		if (mhi_kobj) {
			/* Creating sysfs file for mhi_panic_timeout */
			if (sysfs_create_file(mhi_kobj, &mhi_attr.attr)) {
				MHI_ERR("Cannot create sysfs file for mhi_panic_timeout\n");
				kobject_put(mhi_kobj);
			}
			/* Creating sysfs file for force_graceful */
			if (sysfs_create_file(mhi_kobj, &force_graceful_attr.attr)) {
				MHI_ERR("Cannot create sysfs file for force_graceful\n");
				kobject_put(mhi_kobj);
			}
		} else {
			MHI_ERR("Unable to create mhi sysfs entry\n");
		}
	}
	MHI_LOG("Return successful\n");

	return 0;

error_power_up:
	mhi_deinit_pci_dev(mhi_cntrl);

error_register:
	mhi_free_controller(mhi_cntrl);

error_init_pci:
	mhi_arch_pcie_deinit(mhi_cntrl);

	return ret;
}

void mhi_pci_device_removed(struct pci_dev *pci_dev)
{
	struct mhi_controller *mhi_cntrl;
	struct device *pdev = &pci_dev->bus->parent->dev;
	struct gpio_desc *mdm2ap;
	static int e911_gpio, e911;
	bool graceful;
	struct pcie_port *pp;
	struct dw_pcie *dw_pci;

	pp = pci_dev->bus->sysdata;
	dw_pci = to_dw_pcie_from_pp(pp);
	graceful = SDX55 == pci_dev->device ? 0 : 1;
	pr_debug("Graceful removal based on modem initialised = %d\n", graceful);

	mhi_cntrl = dev_get_drvdata(&pci_dev->dev);

	if (pci_dev->device == SDX65) {
		e911_gpio = of_get_named_gpio(pdev->of_node, "e911-gpio", 0);
		if (of_property_read_bool(pdev->of_node, "e911_x65-gpio"))
			e911_gpio = of_get_named_gpio(pdev->of_node,
						      "e911_x65-gpio", 0);
		if (e911_gpio < 0) {
			MHI_LOG("e911-gpio not configured");
		} else {
			e911 = gpio_get_value(e911_gpio);
			pr_debug("e911 gpio value = %d\n",e911);
			if(ssr_mode == 1 && !e911 && dw_pcie_link_up(dw_pci) == 0)
				graceful = 0;
			pr_debug("X65 modem - graceful removal = %d,"
				"mhi_cntrl->dev_state = %d"
				"dw_pcie_link_up(dw_pci)==%d\n", graceful,
				mhi_cntrl->dev_state, dw_pcie_link_up(dw_pci));
		}
	}

	if (mhi_cntrl) {

		struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);

#ifdef CONFIG_PM
		pm_stay_awake(&mhi_cntrl->mhi_dev->dev);

		/* if link is in drv suspend, wake it up */
		pm_runtime_get_sync(&pci_dev->dev);
#endif

		mutex_lock(&mhi_cntrl->pm_mutex);
		if (!mhi_dev->powered_on) {
			MHI_LOG("Not in active state\n");
			mutex_unlock(&mhi_cntrl->pm_mutex);
#ifdef CONFIG_PM
			pm_runtime_put_noidle(&pci_dev->dev);
#endif
			return;
		}
		mhi_dev->powered_on = false;
		mutex_unlock(&mhi_cntrl->pm_mutex);

#ifdef CONFIG_PM
		pm_runtime_put_noidle(&pci_dev->dev);
#endif

		if (mhi_ssr_negotiate) {
			MHI_LOG("mhi_ssr_negotite status = %d \n",
				mhi_ssr_negotiate);
			if (SDX55 == pci_dev->device) {
				mdm2ap = gpio_to_desc(mdm2ap_gpio);
				if (IS_ERR(mdm2ap))
					MHI_ERR("Unable to acquire mdm2ap_gpio");

				graceful = gpiod_get_value(mdm2ap);
			}
			if(force_graceful)
				graceful = 1;
		}

		MHI_LOG("Triggering shutdown process\n");
		mhi_power_down(mhi_cntrl, graceful);

		/* turn the link off */
		mhi_deinit_pci_dev(mhi_cntrl);
		mhi_arch_link_off(mhi_cntrl, false);

		mhi_arch_pcie_deinit(mhi_cntrl);

#ifdef CONFIG_PM
		pm_relax(&mhi_cntrl->mhi_dev->dev);
#endif

		kobject_put(mhi_kobj);

		mhi_unregister_controller(mhi_cntrl);
	}
}

#ifdef CONFIG_PM
static const struct dev_pm_ops pm_ops = {
	SET_RUNTIME_PM_OPS(mhi_runtime_suspend,
			   mhi_runtime_resume,
			   mhi_runtime_idle)
	SET_SYSTEM_SLEEP_PM_OPS(mhi_system_suspend, mhi_system_resume)
};
#endif

static struct pci_device_id mhi_pcie_device_id[] = {
	{PCI_DEVICE(MHI_PCIE_VENDOR_ID, 0x0300)},
	{PCI_DEVICE(MHI_PCIE_VENDOR_ID, 0x0301)},
	{PCI_DEVICE(MHI_PCIE_VENDOR_ID, 0x0303)},
	{PCI_DEVICE(MHI_PCIE_VENDOR_ID, 0x0304)},
	{PCI_DEVICE(MHI_PCIE_VENDOR_ID, 0x0305)},
	{PCI_DEVICE(MHI_PCIE_VENDOR_ID, 0x0306)},
	{PCI_DEVICE(MHI_PCIE_VENDOR_ID, 0x0308)},
	{PCI_DEVICE(MHI_PCIE_VENDOR_ID, MHI_PCIE_DEBUG_ID)},
	{0},
};

static struct pci_driver mhi_pcie_driver = {
	.name = "mhi",
	.id_table = mhi_pcie_device_id,
	.probe = mhi_pci_probe,
	.remove = mhi_pci_device_removed,
#ifdef CONFIG_PM
	.driver = {
		.pm = &pm_ops
	}
#endif
};

module_pci_driver(mhi_pcie_driver);
