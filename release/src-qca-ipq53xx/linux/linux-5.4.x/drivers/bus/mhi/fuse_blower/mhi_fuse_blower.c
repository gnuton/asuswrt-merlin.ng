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

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/dma-mapping.h>
#include <linux/pci.h>
#include <linux/firmware.h>
#include <linux/delay.h>

#define MAX_PCIE_INSTANCE 4

#define DEFAULT_FW_PATH				"qcn9224/fuse_blower.bin"
#define QCN9224_PCIE_REMAP_BAR_CTRL_OFFSET      0x310C
#define PCIE_SOC_GLOBAL_RESET_ADDRESS 		0x3008
#define QCN9224_TCSR_SOC_HW_VERSION		0x1B00000
#define QCN9224_TCSR_SOC_HW_VERSION_MASK	GENMASK(11,8)
#define QCN9224_TCSR_SOC_HW_VERSION_SHIFT 	8
#define PCIE_SOC_GLOBAL_RESET_VALUE 		0x5
#define MAX_SOC_GLOBAL_RESET_WAIT_CNT 		50 /* x 20msec */

#define QCN9224_TCSR_PBL_LOGGING_REG		0x01B00094
#define QCN9224_SECURE_BOOT0_AUTH_EN 		0x01e20010
#define QCN9224_OEM_MODEL_ID 			0x01e20018
#define QCN9224_ANTI_ROLL_BACK_FEATURE 		0x01e2001C
#define QCN9224_OEM_PK_HASH 			0x01e20060
#define QCN9224_SECURE_BOOT0_AUTH_EN_MASK 	(0x1)
#define QCN9224_OEM_ID_MASK 			GENMASK(31,16)
#define QCN9224_OEM_ID_SHIFT 			16
#define QCN9224_MODEL_ID_MASK 			GENMASK(15,0)
#define QCN9224_ANTI_ROLL_BACK_FEATURE_EN_MASK 	BIT(9)
#define QCN9224_ANTI_ROLL_BACK_FEATURE_EN_SHIFT 9
#define QCN9224_TOTAL_ROT_NUM_MASK 		GENMASK(13,12)
#define QCN9224_TOTAL_ROT_NUM_SHIFT 		12
#define QCN9224_ROT_REVOCATION_MASK 		GENMASK(17,14)
#define QCN9224_ROT_REVOCATION_SHIFT 		14
#define QCN9224_ROT_ACTIVATION_MASK 		GENMASK(21,18)
#define QCN9224_ROT_ACTIVATION_SHIFT 		18
#define QCN9224_OEM_PK_HASH_SIZE 		36

#define MHICTRL (0x38)
#define BHI_STATUS (0x12C)
#define BHI_IMGADDR_LOW (0x108)
#define BHI_IMGADDR_HIGH (0x10C)
#define BHI_IMGSIZE (0x110)
#define BHI_ERRCODE (0x130)
#define BHI_ERRDBG1 (0x134)
#define BHI_ERRDBG2 (0x138)
#define BHI_ERRDBG3 (0x13C)
#define BHI_IMGTXDB (0x118)
#define BHI_EXECENV (0x128)

#define MHICTRL_RESET_MASK (0x2)
#define BHI_STATUS_MASK (0xC0000000)
#define BHI_STATUS_SHIFT (30)
#define BHI_STATUS_SUCCESS (2)

#define NO_MASK (0xFFFFFFFF)

#define QTI_PCI_VENDOR_ID		0x17CB
#define QCN92XX_DEVICE_ID		0x1109

#define PCI_LINK_DOWN                   0
#define PCI_BAR_NUM                     0
#define PCI_DMA_MASK_64_BIT		64

#define MAX_UNWINDOWED_ADDRESS			0x80000
#define WINDOW_ENABLE_BIT			0x40000000
#define WINDOW_SHIFT				19
#define WINDOW_VALUE_MASK			0x3F
#define WINDOW_START				MAX_UNWINDOWED_ADDRESS
#define WINDOW_RANGE_MASK			0x7FFFF
static DEFINE_SPINLOCK(pci_reg_window_lock);

struct pcie_dev_list{
	struct pci_dev *pci_dev;
	const struct pci_device_id *id;
};
struct pcie_dev_list gpci_dev_list[MAX_PCIE_INSTANCE];

char *fw_path;
module_param(fw_path, charp, 0444);
MODULE_PARM_DESC(fw_path, "fw path");

static void mhi_pci_select_window(void __iomem *bar, u32 offset)
{
	u32 window = (offset >> WINDOW_SHIFT) & WINDOW_VALUE_MASK;
	u32 prev_window = 0, curr_window = 0, prev_cleared_window = 0;

	prev_window = readl_relaxed(bar +
					QCN9224_PCIE_REMAP_BAR_CTRL_OFFSET);

	/* Clear out last 6 bits of window register */
	prev_cleared_window = prev_window & ~(0x3f);

	/* Write the new last 6 bits of window register. Only window 1 values
	 * are changed. Window 2 and 3 are unaffected.
	 */
	curr_window = prev_cleared_window | window;

	/* Skip writing into window register if the read value
	 * is same as calculated value.
	 */
	if (curr_window == prev_window)
		return;

	writel_relaxed(WINDOW_ENABLE_BIT | curr_window,
		       QCN9224_PCIE_REMAP_BAR_CTRL_OFFSET +
		       bar);
}

static int mhi_pci_reg_read(void __iomem *bar,
			     u32 offset, u32 *val)
{
	unsigned long flags;

	if (!bar) {
		pr_err("PCI bar is not yet assigned\n");
		return 0;
	}

	if (offset < MAX_UNWINDOWED_ADDRESS) {
		*val = readl_relaxed(bar + offset);
		return 0;
	}

	spin_lock_irqsave(&pci_reg_window_lock, flags);
	mhi_pci_select_window(bar, offset);

	*val = readl_relaxed(bar + WINDOW_START +
			     (offset & WINDOW_RANGE_MASK));
	spin_unlock_irqrestore(&pci_reg_window_lock, flags);

	return 0;
}

int mhifuse_get_exec_env(void __iomem *bar)
{
	u32 exec;
	mhi_pci_reg_read(bar, BHI_EXECENV, &exec);

	return exec;
}

void mhifuse_global_soc_reset(void __iomem *bar)
{
	u32 current_ee;
	u32 count = 0;

	current_ee = mhifuse_get_exec_env(bar);

	do {
		writel_relaxed(PCIE_SOC_GLOBAL_RESET_VALUE,
			       PCIE_SOC_GLOBAL_RESET_ADDRESS +
			       bar);
		msleep(20);
		current_ee = mhifuse_get_exec_env(bar);
		count++;
	} while (current_ee != 0 &&
		 count < MAX_SOC_GLOBAL_RESET_WAIT_CNT);

	if (count >= MAX_SOC_GLOBAL_RESET_WAIT_CNT)
		printk("SoC global reset failed! Reset count : %d\n",
			      count);
}

static void *mhi_fuse_copy_fw(struct pci_dev *pci_dev, dma_addr_t *dma_addr, size_t *size)
{
	struct device *dev = &pci_dev->dev;
	int  ret;
	void *buf;
	const struct firmware *file = NULL;
	const char *filename = DEFAULT_FW_PATH;

	if (fw_path)
		filename = fw_path;

	ret = request_firmware(&file, filename, dev);
	if (ret) {
		dev_err(dev, "Error in loading file (%s) : %d \n"
			, filename, ret);
		return NULL;
	}

	buf = dma_alloc_coherent(dev, file->size, dma_addr, GFP_KERNEL);
	if (!buf) {
		dev_err(dev, "Error in DMA alloc \n");
		goto fail;
	}

	memcpy(buf, file->data, file->size);

	*size = file->size;

fail:
	release_firmware(file);
	return buf;
}

static int poll_reg(void __iomem *addr, u32 *val, u32 mask, u32 shift, u32 compare)
{
	u32 retry = 50;
	u32 delayus = 250000;

	while(retry--) {
		*val = readl(addr);
		if ((*val & mask) >> shift == compare)
			return 0;

		fsleep(delayus);
	}

	return -ETIMEDOUT;
}

static void print_error_code(void __iomem *addr, bool pbl_log)
{
	int i;
	u32 val;
	struct {
		char *name;
		u32 offset;
	} error_reg[] = {
		{ "ERROR_CODE", BHI_ERRCODE },
		{ "ERROR_DBG1", BHI_ERRDBG1 },
		{ "ERROR_DBG2", BHI_ERRDBG2 },
		{ "ERROR_DBG3", BHI_ERRDBG3 },
		{ NULL },
	};

	for (i = 0; error_reg[i].name; i++) {
		val = readl(addr + error_reg[i].offset);
		printk("Reg: %s value: 0x%x\n", error_reg[i].name, val);
	}

	if (pbl_log) {
		mhi_pci_reg_read(addr, QCN9224_TCSR_PBL_LOGGING_REG, &val);
		printk("Reg: TCSR_PBL_LOGGING: 0x%x\n", val);
	}

}

int mhi_fuse_blow(struct pci_dev *pci_dev)
{
	dma_addr_t dma_addr;
	void __iomem *bar;
	size_t size;
	void *buf;
	u32 val = 0;
	int ret = 0;

	bar = pci_iomap(pci_dev, PCI_BAR_NUM, 0);
	if (!bar) {
		printk("Failed to do PCI IO map ..\n");
		return -EIO;
	}

	mhi_pci_reg_read(bar, QCN9224_TCSR_SOC_HW_VERSION, &val);
	val = (val & QCN9224_TCSR_SOC_HW_VERSION_MASK) >>
					QCN9224_TCSR_SOC_HW_VERSION_SHIFT;
	if (val == 2)
		printk("Detected QNC9224 V2\n");
	else {
		printk("Fusing is not supported for QNC9224 V%d\n", val);
		return -1;
	}

	buf = mhi_fuse_copy_fw(pci_dev, &dma_addr, &size);

	if (!buf)
		return -1;

	writel(0, bar + BHI_STATUS);
	writel(upper_32_bits(dma_addr), bar + BHI_IMGADDR_HIGH);
	writel(lower_32_bits(dma_addr), bar + BHI_IMGADDR_LOW);
	writel(size, bar + BHI_IMGSIZE);
	writel(1, bar + BHI_IMGTXDB);

	printk("Waiting for fuse blower bin download ... ");
	ret = poll_reg(bar + BHI_STATUS, &val, BHI_STATUS_MASK,
				BHI_STATUS_SHIFT, BHI_STATUS_SUCCESS);
	if (ret) {
		printk("Failed, BHI_STATUS 0x%x, ret %d\n",
							val, ret);
		print_error_code(bar, true);
		goto fail;
	}

	ret = poll_reg(bar + BHI_EXECENV, &val, NO_MASK, 0, 1);
	if (ret) {
		printk("EE is not set, BHI EE 0x%x, ret %d\n",
							val, ret);
		print_error_code(bar, true);
		goto fail;
	}
	printk("Fuse blower bin loaded successfully \n");

	printk("Waiting for fuse blower status ... ");
	ret = poll_reg(bar + BHI_ERRCODE, &val, NO_MASK, 0, 0xCAFECACE);
	if (ret) {
		printk("Fusing failed, ret %d\n",ret);
		print_error_code(bar, false);
		goto fail;
	}

	printk("Fusing completed successfully \n");

fail:
	/* Target SoC global reset */
	mhifuse_global_soc_reset(bar);

	msleep(1000);

	/* Target MHI reset */
	val = readl(bar + MHICTRL);
	writel(val | MHICTRL_RESET_MASK, bar + MHICTRL);

	dma_free_coherent(&pci_dev->dev, size, buf, dma_addr);
	pci_iounmap(pci_dev, bar);

	return ret;
}

int mhi_fuse_show(struct pci_dev *pci_dev)
{
	u32 val = 0;
	void __iomem *bar;
	int i;

	bar = pci_iomap(pci_dev, PCI_BAR_NUM, 0);
	if (!bar) {
		printk("Failed to do PCI IO map ..\n");
		return -EIO;
	}

	printk("\nFuse Name \t\t Address \t Value\n");
	printk("------------------------------------------------------\n");

	mhi_pci_reg_read(bar, QCN9224_SECURE_BOOT0_AUTH_EN, &val);
	printk("SECURE_BOOT0_AUTH_EN\t 0x%x \t 0x%x \n", QCN9224_SECURE_BOOT0_AUTH_EN,
			(val & QCN9224_SECURE_BOOT0_AUTH_EN_MASK));

	mhi_pci_reg_read(bar, QCN9224_OEM_MODEL_ID, &val);
	printk("OEM ID\t\t\t 0x%x \t 0x%lx \n",QCN9224_OEM_MODEL_ID,
			(val & QCN9224_OEM_ID_MASK) >> QCN9224_OEM_ID_SHIFT);

	printk("MODEL ID\t\t\t 0x%x \t 0x%lx \n",QCN9224_OEM_MODEL_ID,
			(val & QCN9224_MODEL_ID_MASK));

	mhi_pci_reg_read(bar, QCN9224_ANTI_ROLL_BACK_FEATURE, &val);
	printk("ANTI_ROLL_BACK_FEATURE_EN 0x%x \t 0x%lx \n", QCN9224_ANTI_ROLL_BACK_FEATURE,
			(val & QCN9224_ANTI_ROLL_BACK_FEATURE_EN_MASK) >> QCN9224_ANTI_ROLL_BACK_FEATURE_EN_SHIFT);
	printk("TOTAL_ROT_NUM\t\t 0x%x \t 0x%lx \n", QCN9224_ANTI_ROLL_BACK_FEATURE,
			(val & QCN9224_TOTAL_ROT_NUM_MASK) >> QCN9224_TOTAL_ROT_NUM_SHIFT);
	printk("ROT_REVOCATION\t\t 0x%x \t 0x%lx \n", QCN9224_ANTI_ROLL_BACK_FEATURE,
			(val & QCN9224_ROT_REVOCATION_MASK) >> QCN9224_ROT_REVOCATION_SHIFT);
	printk("ROT_ACTIVATION\t\t 0x%x \t 0x%lx \n", QCN9224_ANTI_ROLL_BACK_FEATURE,
			(val & QCN9224_ROT_ACTIVATION_MASK) >> QCN9224_ROT_ACTIVATION_SHIFT);

	for (i = 0; i <= QCN9224_OEM_PK_HASH_SIZE ; i+=4) {
		mhi_pci_reg_read(bar, QCN9224_OEM_PK_HASH + i, &val);

		printk("OEM PK hash \t\t 0x%x \t 0x%x\n",QCN9224_OEM_PK_HASH + i, val);
	}
	printk("------------------------------------------------------\n\n");

	pci_iounmap(pci_dev, bar);

	return 0;
}

int mhi_fuse_pci_enable_bus(struct pci_dev *pci_dev, const struct pci_device_id *id, bool fuse)
{
	u16 device_id;
	int ret;
	u32 pci_dma_mask = PCI_DMA_MASK_64_BIT;

	printk("Going for PCI Enable bus\n");
	pci_read_config_word(pci_dev, PCI_DEVICE_ID, &device_id);
	printk("Read config space, Device_id:0x%x\n", device_id);

	if (device_id != id->device) {
		printk("Device Id does not match with Probe ID..\n");
		return -EIO;
	}

	ret = pci_assign_resource(pci_dev, PCI_BAR_NUM);
	if (ret) {
		printk("Failed to assign PCI resource  Error:%d\n", ret);
		goto out;
	}

	ret = pci_enable_device(pci_dev);
	if (ret) {
		printk("Failed to Enable PCI device  Error:%d\n", ret);
		goto out;
	}

	ret = pci_request_region(pci_dev, PCI_BAR_NUM, "mhi_fuse_region");
	if (ret) {
		printk("Failed to req. region Error:%d\n", ret);
		goto out2;
	}

	ret = pci_set_dma_mask(pci_dev, DMA_BIT_MASK(pci_dma_mask));
	if (ret) {
		printk("Failed to set dma mask:(%d) ret:%d\n",
					pci_dma_mask, ret);
		goto out3;
	}

	pci_set_master(pci_dev);

	pci_save_state(pci_dev);

	if (fuse)
		ret = mhi_fuse_blow(pci_dev);
	else
		ret = mhi_fuse_show(pci_dev);

	if (ret) {
		printk("MHI fuse %s failed, ret %d \n",fuse?"blow":"print", ret);
		goto out4;
	}

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

static int idx;
static ssize_t fuse_blower_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int i;

	for (i = 0; i < idx; i++) {
		printk("PCIe Bus ID: %d\n",i);
		if (gpci_dev_list[i].pci_dev->is_busmaster != true) {
			mhi_fuse_pci_enable_bus(gpci_dev_list[i].pci_dev,
							gpci_dev_list[i].id, false);
		} else {
			mhi_fuse_show(gpci_dev_list[i].pci_dev);
		}
	}

	return 0;
}

static ssize_t fuse_blower_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t n)
{
	int ret, index;

	ret = kstrtoint(buf, 0, &index);
	if (ret || index >= idx) {
		printk("invalid pci idx %d ret %d \n", index, ret);
		return n;
	}

	if (gpci_dev_list[index].pci_dev->is_busmaster != true) {
		mhi_fuse_pci_enable_bus(gpci_dev_list[index].pci_dev,
						gpci_dev_list[index].id, true);
	} else {
		mhi_fuse_blow(gpci_dev_list[index].pci_dev);
	}

	return n;
}

static const DEVICE_ATTR(fuse_blower, 0644, fuse_blower_show,
			fuse_blower_store);

struct module *mod = THIS_MODULE;
int mhi_fuse_pci_probe(struct pci_dev *pci_dev, const struct pci_device_id *id)
{

	printk("--->\n");

	printk("PCI: 0x%x Vendor ID:0x%x Device ID:0x%x idx %d\n",
			(u32) pci_dev->resource[0].start & 0xff000000,
			pci_dev->vendor, pci_dev->device, idx);

	gpci_dev_list[idx].pci_dev = pci_dev;
	gpci_dev_list[idx].id = id;

	if (!idx) {
		sysfs_create_file(&mod->mkobj.kobj,
					&dev_attr_fuse_blower.attr);
	}
	idx++;

	printk("<---done\n");
	return 0;

}

void mhi_fuse_pci_remove(struct pci_dev *pci_dev)
{
	dev_info(&pci_dev->dev,"removing\n");

	pci_clear_master(pci_dev);
	pci_release_region(pci_dev, PCI_BAR_NUM);
	if (pci_is_enabled(pci_dev))
		pci_disable_device(pci_dev);
}

static const struct pci_device_id mhi_fuse_pci_id_table[] = {
	{QTI_PCI_VENDOR_ID, QCN92XX_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID},
};

struct pci_driver mhi_fuse_pci_driver = {
	.name	  = "mhi_fuse_pci",
	.probe	  = mhi_fuse_pci_probe,
	.remove	  = mhi_fuse_pci_remove,
	.id_table = mhi_fuse_pci_id_table,
};

int mhi_fuse_pci_register(void)
{
	int ret;

	ret = pci_register_driver(&mhi_fuse_pci_driver);
	if (ret) {
		printk("Error ret:%d\n", ret);
		goto out;
	}
out:
	return ret;
}

void mhi_fuse_pci_unregister(void)
{
	printk("\n");
	pci_unregister_driver(&mhi_fuse_pci_driver);
}

int __init mhi_fuse_init(void)
{
	int ret;
	ret = mhi_fuse_pci_register();
	if (ret)
		printk("Error ret:%d\n", ret);
	return ret;
}

void __exit mhi_fuse_exit(void)
{
	printk("Unregister mhi_fuse_blower\n");
	mhi_fuse_pci_unregister();
	printk("done\n");
}

module_init(mhi_fuse_init);
module_exit(mhi_fuse_exit);

MODULE_DESCRIPTION("MHIFUSEBLOWER");
MODULE_LICENSE("GPL v2");
