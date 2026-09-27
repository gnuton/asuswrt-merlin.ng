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

#include <linux/memblock.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/spinlock.h>
#include <linux/of_device.h>
#include <soc/qcom/ramdump.h>
#include "commonmhitest.h"

#define PCIE_PCIE_LOCAL_REG_PCIE_LOCAL_RSV0	0x3164
#define PCIE_REG_FOR_QRTR_NODE_INSTANCE_ID	\
	PCIE_PCIE_LOCAL_REG_PCIE_LOCAL_RSV0
#define QCN90XX_QRTR_INSTANCE_ID_BASE		0x20
#define QCN92XX_QRTR_INSTANCE_ID_BASE		0x30

#define MHITEST_MHI_SEG_LEN			SZ_512K
#define MHITEST_DUMP_DESC_TOLERANCE		64
#define MAX_RAMDUMP_TABLE_SIZE			6
#define COREDUMP_DESC				"Q6-COREDUMP"
#define Q6_SFR_DESC				"Q6-SFR"


static struct mhi_channel_config mhitest_mhi_channels[] = {
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
		.num = 4,
		.name = "DIAG",
		.num_elements = 32,
		.event_ring = 1,
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
		.num = 5,
		.name = "DIAG",
		.num_elements = 32,
		.event_ring = 1,
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
		.num = 20,
		.name = "IPCR",
		.num_elements = 64,
		.event_ring = 1,
		.dir = DMA_TO_DEVICE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.doorbell = MHI_DB_BRST_DISABLE,
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
		.event_ring = 1,
		.dir = DMA_FROM_DEVICE,
		.ee_mask = 0x4,
		.pollcfg = 0,
		.doorbell = MHI_DB_BRST_DISABLE,
		.lpm_notify = false,
		.offload_channel = false,
		.doorbell_mode_switch = false,
		.auto_queue = true,
		.auto_start = true,
	},
};

static struct mhi_event_config mhitest_mhi_events[] = {
	{
		.num_elements = 32,
		.irq_moderation_ms = 0,
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
		.priority = 1,
		.hardware_event = false,
		.client_managed = false,
		.offload_channel = false,
	},
};

static struct mhi_controller_config mhitest_mhi_config = {
	.max_channels = 128,
	.timeout_ms = 2000,
	.use_bounce_buf = false,
	.buf_len = 0,
	.num_channels = ARRAY_SIZE(mhitest_mhi_channels),
	.ch_cfg = mhitest_mhi_channels,
	.num_events = ARRAY_SIZE(mhitest_mhi_events),
	.event_cfg = mhitest_mhi_events,
};

static struct mhitest_msi_config msi_config = {
	.total_vectors = 16,
	.total_users = 1,
	.users = (struct mhitest_msi_user[]) {
		{ .name = "MHI-TEST", .num_vectors = 16, .base_vector = 0 },
	},
};

struct ramdump_entry {
	__le64 base_address;
	__le64 actual_phys_address;
	__le64 size;
	char description[20];
	char file_name[20];
};

struct ramdump_header {
	__le32 version;
	__le32 header_size;
	struct ramdump_entry ramdump_table[MAX_RAMDUMP_TABLE_SIZE];
};

irqreturn_t mhitest_msi_handlr(int irq_number, void *dev)
{
	printk("mhitest_msi_handlr irq_number==%d\n",irq_number);
	return IRQ_HANDLED;
}

void mhitest_get_crash_reason(struct mhi_controller *mhi_cntrl)
{
	struct ramdump_header *ramdump_header;
	struct ramdump_entry *ramdump_table;
	char *msg = ERR_PTR(-EPROBE_DEFER);
	struct image_info *rddm_image;
	u64 coredump_offset = 0;
	struct mhi_buf *mhi_buf;
	struct pci_dev *pdev;
	struct device *dev;
	int i;

	rddm_image = mhi_cntrl->rddm_image;
	mhi_buf = rddm_image->mhi_buf;

	dev = &mhi_cntrl->mhi_dev->dev;
	pdev = to_pci_dev(mhi_cntrl->cntrl_dev);
	dev_err(dev, "CRASHED - [DID:DOMAIN:BUS:SLOT] - %x:%04u:%02u:%02u\n",
		pdev->device, pdev->bus->domain_nr, pdev->bus->number,
		PCI_SLOT(pdev->devfn));

	/* Get RDDM header size */
	ramdump_header = (struct ramdump_header *)mhi_buf[0].buf;
	ramdump_table = ramdump_header->ramdump_table;
	coredump_offset += le32_to_cpu(ramdump_header->header_size);

	/* Traverse ramdump table to get coredump offset */
	i = 0;
	while (i < MAX_RAMDUMP_TABLE_SIZE) {
		if (!strncmp(ramdump_table->description, COREDUMP_DESC,
			     sizeof(COREDUMP_DESC)) ||
			!strncmp(ramdump_table->description, Q6_SFR_DESC,
			     sizeof(Q6_SFR_DESC))) {
			break;
		}
		coredump_offset += cpu_to_le64(ramdump_table->size);
		ramdump_table++;
		i++;
	}

	if (i == MAX_RAMDUMP_TABLE_SIZE) {
		dev_err(dev, "Cannot find '%s' entry in ramdump\n",
			COREDUMP_DESC);
		return;
	}

	/* Locate coredump data from the ramdump segments */
	for (i = 0; i < rddm_image->entries; i++) {
		if (coredump_offset < mhi_buf[i].len) {
			msg = mhi_buf[i].buf + coredump_offset;
			break;
		}

		coredump_offset -= mhi_buf[i].len;
	}

	if (!IS_ERR(msg) && msg && msg[0])
		dev_err(dev, "Fatal error received from wcss software!\n%s\n",
			msg);
}

int mhitest_dump_info(struct mhitest_platform *mplat, bool in_panic)
{
	struct mhi_controller *mhi_ctrl;
	struct image_info *rddm_img, *fw_img;
	struct mhitest_dump_data *dump_data;
	struct mhitest_dump_seg *dump_seg;
	int ret, i;
	u16 device_id;

	mhi_ctrl = mplat->mhi_ctrl;
	pci_read_config_word(mplat->pci_dev, PCI_DEVICE_ID, &device_id);
	MHITEST_VERB("Read config space again, Device_id:0x%x\n", device_id);
	if (device_id != mplat->pci_dev_id->device) {
		MHITEST_VERB("Device Id does not match with Probe ID..\n");
		return -EIO;
	}

	ret = mhi_download_rddm_image(mhi_ctrl, in_panic);
	if (ret) {
		MHITEST_VERB("Error .. not able to dload rddm img ret:%d\n",
									ret);
		return ret;
	}

	mhitest_get_crash_reason(mhi_ctrl);

	MHITEST_VERB("Let's dump some more things...\n");
	mhi_debug_reg_dump(mhi_ctrl);

	rddm_img = mhi_ctrl->rddm_image;
	fw_img = mhi_ctrl->fbc_image;
	dump_data = &mplat->mhitest_rdinfo.dump_data;
	dump_seg = mplat->mhitest_rdinfo.dump_data_vaddr;

	dump_data->nentries = 0;
	MHITEST_VERB("dump_dname:%s entries:%d\n", dump_data->name,
						dump_data->nentries);
	MHITEST_VERB("----Collect FW image dump segment, nentries %d----\n",
		    fw_img->entries);

	for (i = 0; i < fw_img->entries; i++) {
		dump_seg->address = fw_img->mhi_buf[i].dma_addr;
		dump_seg->v_address = fw_img->mhi_buf[i].buf;
		dump_seg->size = fw_img->mhi_buf[i].len;
		dump_seg->type = FW_IMAGE;
		MHITEST_VERB("seg-%d:Address:0x%lx,v_Address %pK, size 0x%lx\n",
				i, dump_seg->address, dump_seg->v_address,
							dump_seg->size);
		dump_seg++;
	}
	dump_data->nentries += fw_img->entries;

	MHITEST_VERB("----Collect RDDM image dump segment, nentries %d----\n",
		    rddm_img->entries);

	for (i = 0; i < rddm_img->entries; i++) {
		dump_seg->address = rddm_img->mhi_buf[i].dma_addr;
		dump_seg->v_address = rddm_img->mhi_buf[i].buf;
		dump_seg->size = rddm_img->mhi_buf[i].len;
		dump_seg->type = FW_RDDM;
		MHITEST_VERB("seg-%d: address:0x%lx,v_address %pK,size 0x%lx\n",
				i, dump_seg->address, dump_seg->v_address,
								dump_seg->size);
		dump_seg++;
	}
	dump_data->nentries += rddm_img->entries;
	MHITEST_VERB("----TODO/not need to Collect remote heap dump segment--\n");
	if (dump_data->nentries > 0)
		mplat->mhitest_rdinfo.dump_data_valid = true;

	return 0;
}

int mhitest_dev_ramdump(struct mhitest_platform *mplat)
{
	struct mhitest_ramdump_info *rdinfo = &mplat->mhitest_rdinfo;
	struct mhitest_dump_data *dump_data = &rdinfo->dump_data;
	struct mhitest_dump_seg *dump_seg = rdinfo->dump_data_vaddr;
	struct ramdump_segment *ramdump_segs, *s;
	struct mhitest_dump_meta_info *meta_info;
	int i, ret = 0, idx = 0;

	if (!rdinfo->dump_data_valid ||
	    dump_data->nentries == 0)
		return 0;

	/* First segment of the dump_data will have meta info in
	 * cnss_dump_meta_info structure format.
	 * Allocate extra segment for meta info and start filling the dump_seg
	 * entries from ramdump_segs + NUM_META_INFO_SEGMENTS.
	 */
	ramdump_segs = kcalloc(dump_data->nentries +
			       MHITEST_NUM_META_INFO_SEGMENTS,
			       sizeof(*ramdump_segs),
			       GFP_KERNEL);
	if (!ramdump_segs)
		return -ENOMEM;

	meta_info = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!meta_info) {
		kfree(ramdump_segs);
		return -ENOMEM;
	}

	meta_info->magic = MHITEST_RAMDUMP_MAGIC;
	meta_info->version = MHITEST_RAMDUMP_VERSION_V2;
	meta_info->chipset = mplat->device_id;

	ramdump_segs->v_address = meta_info;
	ramdump_segs->size = sizeof(*meta_info);

	s = ramdump_segs + MHITEST_NUM_META_INFO_SEGMENTS;
	for (i = 0; i < dump_data->nentries; i++) {
		if (dump_seg->type >= FW_DUMP_TYPE_MAX) {
			MHITEST_ERR("Unsupported dump type: %d\n",
				    dump_seg->type);
			continue;
		}

		if (dump_seg->type != meta_info->entry[idx].type)
			idx++;

		if (meta_info->entry[idx].entry_start == 0) {
			meta_info->entry[idx].type = dump_seg->type;
			meta_info->entry[idx].entry_start =
						i + MHITEST_NUM_META_INFO_SEGMENTS;
		}
		meta_info->entry[idx].entry_num++;

		s->address = dump_seg->address;
		s->v_address = dump_seg->v_address;
		s->size = dump_seg->size;
		s++;
		dump_seg++;
	}

	meta_info->total_entries = idx + 1;

	MHITEST_LOG("Dumping meta_info: total_entries: %d",
		    meta_info->total_entries);
	for (i = 0; i < meta_info->total_entries; i++)
		MHITEST_LOG("entry %d type %d entry_start %d entry_num %d",
			    i, meta_info->entry[i].type,
			    meta_info->entry[i].entry_start,
			    meta_info->entry[i].entry_num);

	ret = create_ramdump_device_file(rdinfo->ramdump_dev);
	if (ret)
		goto clear_dump_info;

	ret = do_elf_ramdump(rdinfo->ramdump_dev, ramdump_segs,
			     dump_data->nentries + MHITEST_NUM_META_INFO_SEGMENTS);

clear_dump_info:
	kfree(meta_info);
	kfree(ramdump_segs);

	return ret;
}

static int mhitest_get_msi_user(struct mhitest_platform *mplat, char *u_name,
		int *num_vectors, u32 *user_base_data, u32 *base_vector)
{
	int idx;
	struct mhitest_msi_config *m_config = mplat->msi_config;

	if (!m_config) {
		MHITEST_ERR("MSI config is NULL\n");
		return -ENODEV;
	}

	for (idx = 0; idx < m_config->total_users; idx++) {
		if (strcmp(u_name, m_config->users[idx].name) == 0) {
			*num_vectors = m_config->users[idx].num_vectors;
			*user_base_data = m_config->users[idx].base_vector
				+ mplat->msi_ep_base_data;
			*base_vector = m_config->users[idx].base_vector;
			MHITEST_VERB("Assign MSI to user:%s,num_vectors:%d,user_base_data:%u, base_vector: %u\n",
				u_name, *num_vectors, *user_base_data,
								*base_vector);

			return 0;
		}
	}
	return -ENODEV;
}

static int mhitest_get_msi_irq(struct device  *device, unsigned int vector)
{
	int irq_num;
	struct pci_dev *pci_dev = to_pci_dev(device);

	irq_num = pci_irq_vector(pci_dev, vector);
	MHITEST_VERB("Got irq_num :%d  for vector : %d\n", irq_num, vector);

	return irq_num;
}

int mhitest_suspend_pci_link(struct mhitest_platform *mplat)
{
	/*
	 * no suspend resume as of now, return 0
	 */
	MHITEST_LOG("No suspend resume now return 0\n");
	return 0;
}

int mhitest_resume_pci_link(struct mhitest_platform *mplat)
{
	/*
	 * no suspend resume as of now, return 0
	 */
	MHITEST_LOG("No suspend resume now return 0\n");
	return 0;
}

int  mhitest_power_off_device(struct mhitest_platform *mplat)
{
	/*
	 * add pinctrl code here if needed !
	 */
	MHITEST_LOG("Powering OFF dummy!\n");
	return 0;
}

int  mhitest_power_on_device(struct mhitest_platform *mplat)
{

	/*
	 * add pinctrl code here if needed !
	 */
	MHITEST_LOG("Powering ON dummy!\n");
	return 0;
}

int mhitest_pci_get_link_status(struct mhitest_platform *mplat)
{
	u16 link_stat;
	int ret;

	ret = pcie_capability_read_word(mplat->pci_dev, PCI_EXP_LNKSTA,
							&link_stat);
	if (ret) {
		MHITEST_ERR("PCIe link is not active !!ret:%d\n", ret);
		return ret;
	}
	MHITEST_VERB("Get PCI link status register: %u\n", link_stat);

	mplat->def_link_speed = link_stat & PCI_EXP_LNKSTA_CLS;
	mplat->def_link_width =
		(link_stat & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT;

	MHITEST_VERB("Default PCI link speed is 0x%x, link width is 0x%x\n",
		    mplat->def_link_speed, mplat->def_link_width);

	return ret;
}

int mhitest_pci_get_mhi_msi(struct mhitest_platform *mplat)
{
	int ret, *irq, num_vectors, i;
	u32 user_base_data, base_vector;

	/*
	 * right now we have only one user in mhitest i.e MHI
	 */
	ret = mhitest_get_msi_user(mplat, "MHI-TEST", &num_vectors,
					&user_base_data, &base_vector);
	if (ret) {
		MHITEST_ERR("Not able to get msi user ret:%d\n", ret);
		return ret;
	}

	MHITEST_LOG("MSI user:%s has num_vectors:%d and base_vector:%d\n",
					"MHI-TEST", num_vectors, base_vector);

	irq = kcalloc(num_vectors, sizeof(int), GFP_KERNEL);
	if (!irq) {
		MHITEST_ERR("Error not able to allocate vectors\n");
		return -ENOMEM;
	}
	for (i = 0; i < num_vectors; i++)
		irq[i] = mhitest_get_msi_irq(&mplat->pci_dev->dev,
							base_vector + i);

	mplat->mhi_ctrl->irq = irq;
	mplat->mhi_ctrl->nr_irqs = num_vectors;

	MHITEST_VERB("irq:[%p] msi_allocated :%d\n", mplat->mhi_ctrl->irq,
				mplat->mhi_ctrl->nr_irqs);

	return 0;
}

char *mhitest_get_reson_str(enum mhi_callback reason)
{
	switch (reason) {
	case MHI_CB_IDLE:
		return "IDLE";
	case MHI_CB_EE_RDDM:
		return "RDDM";
	case MHI_CB_EE_MISSION_MODE:
		return "EE_MISSION_MODE";
	case MHI_CB_BW_REQ:
		return "BW_REQ";
	case MHI_CB_SYS_ERROR:
		return "SYS_ERROR";
	case MHI_CB_FATAL_ERROR:
		return "FATAL_ERROR";
	default:
		return "UNKNOWN";
	}
}

char *mhitest_event_to_str(enum mhitest_event_type etype)
{
	switch (etype) {
	case MHITEST_RECOVERY_EVENT:
		return "MHITEST_RECOVERY_EVENT";
	default:
		return "UNKNOWN EVENT";
	}
}

int mhitest_post_event(struct mhitest_platform *mplat,
		       struct mhitest_recovery_data *data,
		       enum mhitest_event_type etype,
		       u32 flags)
{
	struct mhitest_driver_event *event;
	int gfp = GFP_KERNEL;
	unsigned long irq_flags;

	if (in_interrupt() || irqs_disabled())
		gfp = GFP_ATOMIC;

	event = kzalloc(sizeof(*event), gfp);
	if (!event)
		return -ENOMEM;

	event->type = etype;
	event->data = data;
	init_completion(&event->complete);
	event->ret = -1;
	event->sync = !!(flags);

	spin_lock_irqsave(&mplat->event_lock, irq_flags);
	list_add_tail(&event->list, &mplat->event_list);
	spin_unlock_irqrestore(&mplat->event_lock, irq_flags);

	queue_work(mplat->event_wq, &mplat->event_work);
	if (flags) {
		MHITEST_ERR("Waiting here to complete (%s) event ...\n",
			mhitest_event_to_str(etype));
		wait_for_completion(&event->complete);
	}
	MHITEST_LOG("No waiting/Completed (%s) event ...ret:%d\n",
			mhitest_event_to_str(etype), event->ret);

	return 0;
}

void mhitest_sch_do_recovery(struct mhitest_platform *mplat,
					enum mhitest_recovery_reason reason)
{
	int gfp = GFP_KERNEL;
	struct mhitest_recovery_data *data;

	if (in_interrupt() || irqs_disabled())
		gfp = GFP_ATOMIC;

	data = kzalloc(sizeof(*data), gfp);
	if (!data)
		return;

	data->reason = reason;

	mhitest_post_event(mplat, data, MHITEST_RECOVERY_EVENT, 0);
}

int __must_check mhitest_read_reg(struct mhi_controller *mhi_cntrl,
				  void __iomem *addr, u32 *out)
{
	u32 tmp = readl(addr);

	/* If the value is invalid, the link is down */
	if (PCI_INVALID_READ(tmp))
		return -EIO;

	*out = tmp;

	return 0;
}

void mhitest_write_reg(struct mhi_controller *mhi_cntrl, void __iomem *addr,
		       u32 val)
{
	writel(val, addr);
}

void mhitest_mhi_notify_status(struct mhi_controller *mhi_cntrl,
						enum mhi_callback reason)
{
	struct mhitest_platform *temp;

	temp = dev_get_drvdata(mhi_cntrl->cntrl_dev);

	MHITEST_VERB("Enter\n");
	if (reason > MHI_CB_FATAL_ERROR) {
		MHITEST_ERR("Unsupported reason :%d\n", reason);
		return;
	}
	MHITEST_EMERG(":[%s]- %d\n", mhitest_get_reson_str(reason), reason);

	switch (reason) {
	case MHI_CB_IDLE:
	case MHI_CB_SYS_ERROR:
		return;
	case MHI_CB_FATAL_ERROR:
		reason = MHI_DEFAULT;
		return;
	case MHI_CB_EE_RDDM:
		reason = MHI_RDDM;
		if (temp->soc_reset_request) {
			MHITEST_LOG("Ignoring RDDM CB for SOC_RESET_REQUEST\n");
			return;
		}
		break;
	case MHI_CB_EE_MISSION_MODE:
		MHITEST_VERB("MHI_CB_EE_MISSION_MODE\n");
		return;
	case MHI_CB_BW_REQ:
		MHITEST_VERB("MHI_CB_BW_REQ\n");
		return;
	default:
		MHITEST_ERR("Unsupported reason --reason:[%s]-(%d)\n",
				mhitest_get_reson_str(reason), reason);
		return;
	}
	mhitest_sch_do_recovery(temp, reason);
	MHITEST_VERB("Exit\n");
}

int mhitest_mhi_pm_runtime_get(struct mhi_controller *mhi_cntrl)
{
	return 0;
}

void mhitest_mhi_pm_runtime_put_noidle(struct mhi_controller *mhi_cntrl)
{
}

int mhitest_pci_register_mhi(struct mhitest_platform *mplat)
{
	struct pci_dev *pci_dev = mplat->pci_dev;
	struct mhi_controller *mhi_ctrl;
	struct device_node *np;
	int ret, len, sw, aw;
	unsigned int *reg, *reg_end;
	unsigned long start, size;

	mhi_ctrl = mhi_alloc_controller();
	if (!mhi_ctrl) {
		MHITEST_ERR("Error: not able to allocate mhi_ctrl\n");
		return -ENOMEM;
	}
	MHITEST_LOG("MHI CTRL :%p\n", mhi_ctrl);

	mplat->mhi_ctrl = mhi_ctrl;
	mhi_ctrl->dev_id = mplat->device_id;
	dev_set_drvdata(&pci_dev->dev, mplat);
	mhi_ctrl->cntrl_dev = &pci_dev->dev;

	if (!mplat->fw_name[0]) {
		MHITEST_ERR("fw_name is NULLL\n");
		return -EINVAL;
	}
	mhi_ctrl->fw_image = mplat->fw_name;

	mhi_ctrl->regs = mplat->bar;
	MHITEST_EMERG("BAR start at :%pa\n", &pci_resource_start(pci_dev,
								PCI_BAR_NUM));

	ret  =  mhitest_pci_get_mhi_msi(mplat);
	if (ret) {
		MHITEST_ERR("PCI get MHI MSI Failed ret:%d\n", ret);
		goto out;
	}

	np = of_find_node_by_type(NULL, "memory");
	if (!np) {
		MHITEST_ERR("memory node not found !!\n");
		return 1;
	}

	aw = of_n_addr_cells(np);
	sw = of_n_size_cells(np);

	reg = (unsigned int *)of_get_property(np, "reg", &len);
	if (!reg) {
		pr_mhitest2("Couldn't get reg from mem node\n");
		return -ENOMEM;
	}
	reg_end = reg + len/4;
	do {
		start = of_read_number(reg, aw);
		reg += aw;
		size = of_read_number(reg, sw);
		reg += sw;
	} while (reg < reg_end);


	mhi_ctrl->iova_start = (dma_addr_t)start;
	mhi_ctrl->iova_stop = (dma_addr_t)(start + size - 1);

	MHITEST_VERB("iova_start:%x iova_stop:%x\n",
			(unsigned int)mhi_ctrl->iova_start,
			(unsigned int)mhi_ctrl->iova_stop);

	mhi_ctrl->status_cb = mhitest_mhi_notify_status;
	mhi_ctrl->runtime_get = mhitest_mhi_pm_runtime_get;
	mhi_ctrl->runtime_put = mhitest_mhi_pm_runtime_put_noidle;
	mhi_ctrl->read_reg = mhitest_read_reg;
	mhi_ctrl->write_reg = mhitest_write_reg;

	mhi_ctrl->rddm_size = mplat->mhitest_rdinfo.ramdump_size;
	mhi_ctrl->sbl_size = SZ_512K;
	mhi_ctrl->seg_len = SZ_512K;
	mhi_ctrl->fbc_download = true;

	ret = mhi_register_controller(mhi_ctrl, &mhitest_mhi_config);
	if (ret) {
		MHITEST_ERR("Failed to register mhi controller ret:%d\n", ret);
		goto out;
	}
	MHITEST_VERB("GOOD!\n");
	return  0;

out:
	mhi_free_controller(mhi_ctrl);
	return ret;
}

int mhitest_pci_en_msi(struct mhitest_platform *temp)
{
	struct pci_dev *pci_dev = temp->pci_dev;
	int num_vectors, ret = 0;
	struct msi_desc *msi_desc;

	temp->msi_config = &msi_config;
	if (!temp->msi_config) {
		MHITEST_ERR("MSI config is NULL\n");
		return -EINVAL;
	}

	num_vectors = pci_alloc_irq_vectors(pci_dev,
					   temp->msi_config->total_vectors,
					   temp->msi_config->total_vectors,
					   PCI_IRQ_MSI | PCI_IRQ_LEGACY);
	if (num_vectors != temp->msi_config->total_vectors) {
		MHITEST_ERR("No Enough MSI vectors req:%d and allocated:%d\n",
				temp->msi_config->total_vectors, num_vectors);
		if (num_vectors >= 0)
			ret = -EINVAL;
		temp->msi_config = NULL;
		goto out;
	}

	msi_desc = irq_get_msi_desc(pci_dev->irq);
	if (!msi_desc) {
		MHITEST_ERR("MSI desc is NULL\n");
		goto free_irq_vectors;
	}

	return 0;

free_irq_vectors:
	pci_free_irq_vectors(pci_dev);
out:
	return ret;
}

int mhitest_pci_enable_bus(struct mhitest_platform *temp)
{
	struct pci_dev *pci_dev = temp->pci_dev;
	u16 device_id;
	int ret;
	u32 pci_dma_mask = PCI_DMA_MASK_64_BIT;

	MHITEST_VERB("Going for PCI Enable bus\n");
	pci_read_config_word(pci_dev, PCI_DEVICE_ID, &device_id);
	MHITEST_EMERG("Read config space, Device_id:0x%x\n", device_id);

	if (device_id != temp->pci_dev_id->device) {
		MHITEST_ERR("Device Id does not match with Probe ID..\n");
		return -EIO;
	}

	ret = pci_assign_resource(pci_dev, PCI_BAR_NUM);
	if (ret) {
		MHITEST_ERR("Failed to assign PCI resource  Error:%d\n", ret);
		goto out;
	}

	ret = pci_enable_device(pci_dev);
	if (ret) {
		MHITEST_ERR("Failed to Enable PCI device  Error:%d\n", ret);
		goto out;
	}

	ret = pci_request_region(pci_dev, PCI_BAR_NUM, "mhitest_region");
	if (ret) {
		MHITEST_ERR("Failed to req. region Error:%d\n", ret);
		goto out2;
	}

	ret = pci_set_dma_mask(pci_dev, DMA_BIT_MASK(pci_dma_mask));
	if (ret) {
		MHITEST_ERR("Failed to set dma mask:(%d) ret:%d\n",
					pci_dma_mask, ret);
		goto out3;
	}

	pci_set_master(pci_dev);

	temp->bar = pci_iomap(pci_dev, PCI_BAR_NUM, 0);
	if (!temp->bar) {
		MHITEST_ERR("Failed to do PCI IO map ..\n");
		ret = -EIO;
		goto out4;
	}

	pci_save_state(pci_dev);
	temp->pci_dev_default_state = pci_store_saved_state(pci_dev);

	return 0;

out4:
	pci_clear_master(pci_dev);
out3:
	pci_release_region(pci_dev, PCI_BAR_NUM);
out2:
	pci_disable_device(pci_dev);
out:
	return ret;
}

void mhitest_global_soc_reset(struct mhitest_platform *mplat)
{
	u32 current_ee;
	u32 count = 0;

	current_ee = mhi_get_exec_env(mplat->mhi_ctrl);
	MHITEST_EMERG("Soc Globle Reset issued\n");

	do {
		writel_relaxed(PCIE_SOC_GLOBAL_RESET_VALUE,
			       PCIE_SOC_GLOBAL_RESET_ADDRESS +
			       mplat->bar);
		msleep(20);
		current_ee = mhi_get_exec_env(mplat->mhi_ctrl);
		count++;
	} while (current_ee != MHI_EE_PBL &&
		 count < MAX_SOC_GLOBAL_RESET_WAIT_CNT);

	if (current_ee != MHI_EE_PBL && count >= MAX_SOC_GLOBAL_RESET_WAIT_CNT)
		MHITEST_EMERG("SoC global reset failed! Reset count : %d\n",
			      count);
	else
		MHITEST_ERR("SOC Global reset count: %d\n", count);
}

void mhitest_pci_disable_bus(struct mhitest_platform *mplat)
{
	struct pci_dev *pci_dev = mplat->pci_dev;
	u32 in_reset = -1, temp = -1, retries = 3;

	mhitest_global_soc_reset(mplat);

	msleep(2000);

	mhi_set_mhi_state(mplat->mhi_ctrl, MHI_STATE_RESET);

        while (retries--) {
                temp = readl_relaxed(mplat->mhi_ctrl->regs  + 0x38);
                in_reset = (temp & 0x2) >> 0x1;
                if (in_reset){
                        MHITEST_LOG("Number of retry left:%d- trying again\n",
                                                                retries);
                        udelay(10);
                        continue;
                }
                break;
        }

	if (in_reset) {
		MHITEST_ERR("Device failed to exit RESET state\n");
		return;
	}
	MHITEST_EMERG("MHI Reset good!\n");

	if (mplat->bar) {
		pci_iounmap(pci_dev, mplat->bar);
		mplat->bar = NULL;
	}

	pci_clear_master(pci_dev);
	pci_release_region(pci_dev, PCI_BAR_NUM);
	if (pci_is_enabled(pci_dev))
		pci_disable_device(pci_dev);
}

int mhitest_unregister_ramdump(struct mhitest_platform *mplat)
{
	struct mhitest_ramdump_info *mhitest_rdinfo = &mplat->mhitest_rdinfo;

	if (mhitest_rdinfo->ramdump_dev)
		destroy_ramdump_device(mhitest_rdinfo->ramdump_dev);
	kfree(mhitest_rdinfo->dump_data_vaddr);
	mhitest_rdinfo->dump_data_vaddr = NULL;
	mhitest_rdinfo->dump_data_valid = false;

	return 0;
}

static u32 mhitest_get_dump_desc_size(struct mhitest_platform *mplat)
{
	u32 descriptor_size = 0;
	u32 segment_len = MHITEST_MHI_SEG_LEN;
	u32 wlan_sram_size = mplat->mhitest_rdinfo.ramdump_size;

	of_property_read_u32(mplat->pci_dev->dev.of_node, "qti,rddm-seg-len",
			     &segment_len);

	descriptor_size = (((wlan_sram_size / segment_len) +
			    MHITEST_DUMP_DESC_TOLERANCE) *
			    sizeof(struct mhitest_dump_seg));

	return descriptor_size;
}

int mhitest_register_ramdump(struct mhitest_platform *mplat)
{
	struct mhitest_ramdump_info *mhitest_rdinfo;
	struct mhitest_dump_data *dump_data;
	struct device *dev = &mplat->plat_dev->dev;
	u32 ramdump_size = 0;
	int ret;

	mhitest_rdinfo = &mplat->mhitest_rdinfo;
	dump_data = &mhitest_rdinfo->dump_data;
	if (!dev->of_node) {
		MHITEST_ERR("of node is null\n");
		return -ENOMEM;
	}
	if (!dev->of_node->name) {
		MHITEST_ERR("of node->name  is NULL\n");
		return -ENOMEM;
	}

	ret = of_property_read_u32(dev->of_node, "qcom,wlan-ramdump-dynamic",
					 &ramdump_size);
	if (ret == 0)
		mhitest_rdinfo->ramdump_size = ramdump_size;

	mhitest_rdinfo->dump_data_vaddr = kzalloc(
			mhitest_get_dump_desc_size(mplat), GFP_KERNEL);
	if (!mhitest_rdinfo->dump_data_vaddr)
		return -ENOMEM;

	dump_data->paddr = virt_to_phys(mhitest_rdinfo->dump_data_vaddr);

	/*
	 * TODO: used ramdom version and magic..etc check and correct it.
	 */
	dump_data->version = 0x00;
	dump_data->magic = 0xAA55AA55;
	dump_data->seg_version = 0x1;
	strlcpy(dump_data->name, "mhitest_mod",
		sizeof(dump_data->name));
	mhitest_rdinfo->ramdump_dev =
		create_ramdump_device(mplat->mhitest_ss_desc_name,
				      mplat->subsys_handle->dev.parent);
	if (!mhitest_rdinfo->ramdump_dev) {
		MHITEST_ERR("Failed to create ramdump device!\n");
		ret = -ENOMEM;
		goto free_ramdump;
	}

	MHITEST_LOG("Ramdump registered ramdump_size:0x%x\n", ramdump_size);

	return 0;

free_ramdump:
	kfree(mhitest_rdinfo->dump_data_vaddr);
	mhitest_rdinfo->dump_data_vaddr = NULL;

	return ret;
}

int mhitest_prepare_pci_mhi_msi(struct mhitest_platform *temp)
{
	int ret;

	MHITEST_VERB("Enter\n");
	if (!temp->pci_dev) {
		MHITEST_ERR("pci_dev is NULLL\n");
		return -EINVAL;
	}

	ret = mhitest_register_ramdump(temp);
	if (ret) {
		MHITEST_ERR("Error not able to reg ramdump. ret :%d\n", ret);
		goto unreg_rdump;
	}

	/*
	 * 1. pci enable bus
	 */
	ret = mhitest_pci_enable_bus(temp);
	if (ret) {
		MHITEST_ERR("Error mhitest_pci_enable. ret :%d\n", ret);
		goto out;
	}

	/*
	 * go with some condition for specific device for msi en
	 * 2. pci enable msi
	 */
	ret = mhitest_pci_en_msi(temp);
	if (ret) {
		MHITEST_ERR("Error mhitest_pci_enable_msi. ret :%d\n", ret);
		goto disable_bus;
	}

	/*
	 * 3. pci register mhi -of_controller
	 */
	ret = mhitest_pci_register_mhi(temp);
	if (ret) {
		MHITEST_ERR("Error pci register mhi. ret :%d\n", ret);
		goto disable_bus;
	}

	ret = mhitest_pci_get_link_status(temp);
	if (ret) {
		MHITEST_ERR("Error not able to get pci link status:%d\n", ret);
		goto out;
	}

	ret = mhitest_suspend_pci_link(temp);
	if (ret) {
		MHITEST_ERR("Error not able to suspend pci:%d\n", ret);
		goto out;
	}

	mhitest_power_off_device(temp);
	MHITEST_VERB("Exit\n");

	return 0;

disable_bus:
	mhitest_pci_disable_bus(temp);
unreg_rdump:
	mhitest_unregister_ramdump(temp);
out:
	return ret;
}

char *mhitest_get_mhi_state_str(enum mhi_state state)
{
	switch (state) {
	case MHI_INIT:
		return "MHI_INIT";
	case MHI_DEINIT:
		return "MHI_DEINIT";
	case MHI_POWER_ON:
		return "MHI_POWER_ON";
	case MHI_POWER_OFF:
		return "MHI_POWER_OFF";
	case MHI_FORCE_POWER_OFF:
		return "MHI_FORCE_POWER_OFF";
	default:
		return "UNKNOWN";
	}
}

int mhitest_pci_set_mhi_state(struct mhitest_platform *mplat,
						enum MHI_STATE state)
{
	int ret = 0;
	int i = 0;

	if (state < 0) {
		MHITEST_ERR("Invalid MHI state : %d\n", state);
		return -EINVAL;
	}

	MHITEST_EMERG("Set MHI_STATE- [%s]-(%d)\n",
				mhitest_get_mhi_state_str(state), state);

	switch (state) {
	case MHI_INIT:
		ret = mhi_prepare_for_power_up(mplat->mhi_ctrl);

		/* Registering dummy interrupt handler for vectors
		 * 3 to 16 to demonstrate the usage of multiple
		 * GIC-MSI interrupts
		 */
		if (!ret && mplat->msi_config->total_vectors > 3) {
			for (i = 3; i < mplat->msi_config->total_vectors; i++) {
				ret = request_irq(mplat->mhi_ctrl->irq[i],
						  mhitest_msi_handlr,
						  IRQF_SHARED,
						  "mhi_rem_vec",
						  mplat->mhi_ctrl);
				if (ret) {
					MHITEST_ERR("Error requesting irq:%d for vector:%d----error_code-%d\n",
						    mplat->mhi_ctrl->irq[i], i, ret);
				}
			}

			/* Updating ret to 0.
			 * Since vectors 3 t0 16 are unused any failure
			 * in registering interrupt handler should not
			 * affect the flow of FBC.
			 */
			ret = 0;
		}
		break;
	case MHI_POWER_ON:
		ret = mhi_sync_power_up(mplat->mhi_ctrl);
		break;
	case MHI_DEINIT:
		mhi_unprepare_after_power_down(mplat->mhi_ctrl);
		if (mplat->msi_config->total_vectors > 3) {
			for (i = 3; i < mplat->msi_config->total_vectors; i++) {
				free_irq(mplat->mhi_ctrl->irq[i], mplat->mhi_ctrl);
			}
		}
		ret = 0;
		break;
	case MHI_POWER_OFF:
		mhi_power_down(mplat->mhi_ctrl, true);
		ret = 0;
		break;
	case MHI_FORCE_POWER_OFF:
		mhi_power_down(mplat->mhi_ctrl, false);
		ret = 0;
		break;
	default:
		MHITEST_ERR("I dont know the state:%d!!\n", state);
		ret = -EINVAL;
	}
	return ret;
}

extern int timeout_ms;
int mhitest_pci_start_mhi(struct mhitest_platform *mplat)
{
	int ret;

	MHITEST_VERB("Enter\n");

	if (!mplat->mhi_ctrl) {
		MHITEST_ERR("mhi_ctrl is NULL .. returning..\n");
		return -EINVAL;
	}

	mplat->mhi_ctrl->timeout_ms = timeout_ms;

	ret = mhitest_pci_set_mhi_state(mplat, MHI_INIT);
	if (ret) {
		MHITEST_ERR("Error not able to set mhi init. returning..\n");
		goto out1;
	}
	/**
	 * in the single wlan chipset case, plat_priv->qrtr_node_id always is 0,
	 * wlan fw will use the hardcode 7 as the qrtr node id.
	 * in the dual Hastings case, we will read qrtr node id
	 * from device tree and pass to get plat_priv->qrtr_node_id,
	 * which always is not zero. And then store this new value
	 * to pcie register, wlan fw will read out this qrtr node id
	 * from this register and overwrite to the hardcode one
	 * while do initialization for ipc router.
	 * without this change, two Hastings will use the same
	 * qrtr node instance id, which will mess up qmi message
	 * exchange. According to qrtr spec, every node should
	 * have unique qrtr node id
	 */
	if (mplat->device_id == QCN90xx_DEVICE_ID || mplat->device_id == QCN92XX_DEVICE_ID) {
		u32 val;
		u32 qrtr_id;

		qrtr_id = (mplat->device_id == QCN90xx_DEVICE_ID)? QCN90XX_QRTR_INSTANCE_ID_BASE:QCN92XX_QRTR_INSTANCE_ID_BASE;
		qrtr_id += mplat->d_instance;

		MHITEST_VERB("write 0x%x to PCIE_REG_FOR_QRTR_NODE_INSTANCE_ID\n", qrtr_id);
		writel(qrtr_id, mplat->bar + PCIE_REG_FOR_QRTR_NODE_INSTANCE_ID);
		if (ret) {
			MHITEST_ERR("Failed to write register offset 0x%x, err = %d\n",
				    PCIE_REG_FOR_QRTR_NODE_INSTANCE_ID, ret);
			goto out1;
		}
		val = readl(mplat->bar + PCIE_REG_FOR_QRTR_NODE_INSTANCE_ID);

		if (val != qrtr_id) {
			MHITEST_ERR("qrtr node id write to register doesn't match with readout value 0x%x", val);
			goto out1;
		}
	}

	ret = mhitest_pci_set_mhi_state(mplat, MHI_POWER_ON);
	if (ret) {
		MHITEST_ERR("Error not able to POWER ON\n");
		goto out1;
	}

	MHITEST_VERB("Exit\n");
	return ret;

out1:
	MHITEST_VERB("Exit-Error\n");
	return ret;
}

int mhitest_prepare_start_mhi(struct mhitest_platform *mplat)
{
	int ret;

	/*
	 * 1. power on, resume link if needed
	 */
	ret = mhitest_power_on_device(mplat);
	if (ret) {
		MHITEST_ERR("Error ret:%d\n", ret);
		goto out;
	}
	ret = mhitest_resume_pci_link(mplat);
	if (ret) {
		MHITEST_ERR("Error ret: %d\n", ret);
		goto out;
	}

	/*
	 * 2. start mhi
	 */
	ret = mhitest_pci_start_mhi(mplat);
	if (ret) {
		MHITEST_ERR("Error ret: %d\n", ret);
		goto out;
	}

out:
	return ret;
}

extern int debug_lvl;
extern int domain;
int mhitest_pci_probe(struct pci_dev *pci_dev, const struct pci_device_id *id)
{
	struct mhitest_platform *mplat;
	struct platform_device *plat_dev = get_plat_device();
	int ret;

	if ((domain != -1) && pci_domain_nr(pci_dev->bus) != domain) {
		MHITEST_LOG("Skipping MHI Device on: %04x:%02x:%02x\n",
				pci_domain_nr(pci_dev->bus),
				pci_dev->bus->number, PCI_SLOT(pci_dev->devfn));
		return 0;
	}

	MHITEST_EMERG("--->\n");
	mplat = devm_kzalloc(&plat_dev->dev, sizeof(*mplat), GFP_KERNEL);
	if (!mplat) {
		MHITEST_ERR("Error: not able to allocate memory\n");
		ret = -ENOMEM;
		goto fail_probe;
	}

	mplat->mhitest_klog_lvl = debug_lvl;

	if (plat_dev != NULL)
		mplat->plat_dev = plat_dev;
	else
		MHITEST_ERR("Error: platform dev is broken\n");

	platform_set_drvdata(plat_dev, mplat);

	mplat->pci_dev = pci_dev;
	mplat->device_id = pci_dev->device;
	mplat->pci_dev_id = id;
	mplat->d_instance = pci_domain_nr(pci_dev->bus);

	MHITEST_LOG("Vendor ID:0x%x Device ID:0x%x Probed Device ID:0x%x Instance ID:0x%x\n",
		    pci_dev->vendor, pci_dev->device,
		    id->device, mplat->d_instance);

	ret = mhitest_event_work_init(mplat);
	if (ret)
		goto free_mplat;

	mhitest_store_mplat(mplat);

	ret = mhitest_subsystem_register(mplat);
	if (ret) {
		MHITEST_ERR("Error subsystem register: ret:%d\n", ret);
		goto pci_deinit;
	}

	MHITEST_EMERG("<---done\n");
	return 0;

pci_deinit:
	mhitest_pci_set_mhi_state(mplat, MHI_DEINIT);
	mhitest_pci_remove_all(mplat);
	pci_load_and_free_saved_state(pci_dev, &mplat->pci_dev_default_state);
	mhitest_remove_mplat(mplat);
	mhitest_event_work_deinit(mplat);
free_mplat:
	devm_kfree(&mplat->plat_dev->dev, mplat);
fail_probe:
	return ret;
}

void mhitest_pci_remove(struct pci_dev *pci_dev)
{
	struct mhitest_platform *mplat;

	if ((domain != -1) && pci_domain_nr(pci_dev->bus) != domain) {
		MHITEST_LOG("Skipping MHI Device on: %04x:%02x:%02x\n",
				pci_domain_nr(pci_dev->bus),
				pci_dev->bus->number, PCI_SLOT(pci_dev->devfn));
		return;
	}
	MHITEST_LOG("mhitest PCI removing\n");

	mplat = get_mhitest_mplat_by_pcidev(pci_dev);
	if (mplat) {
		mplat->soc_reset_request = true;
		mhi_soc_reset(mplat->mhi_ctrl);
		mhitest_subsystem_unregister(mplat);
		mhitest_event_work_deinit(mplat);
		pci_load_and_free_saved_state(pci_dev, &mplat->pci_dev_default_state);
		mhitest_free_mplat(mplat);
	}
}

static const struct pci_device_id mhitest_pci_id_table[] = {
	{QTI_PCI_VENDOR_ID, QCN90xx_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID},
	{QTI_PCI_VENDOR_ID, QCN92XX_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID},
};

struct pci_driver mhitest_pci_driver = {
	.name	  = "mhitest_pci",
	.probe	  = mhitest_pci_probe,
	.remove	  = mhitest_pci_remove,
	.id_table = mhitest_pci_id_table,
};

int mhitest_pci_register(void)
{
	int ret;

	ret = pci_register_driver(&mhitest_pci_driver);
	if (ret) {
		MHITEST_ERR("Error ret:%d\n", ret);
		goto out;
	}
out:
	return ret;
}

void mhitest_pci_unregister(void)
{
	MHITEST_VERB("\n");
	pci_unregister_driver(&mhitest_pci_driver);
}
