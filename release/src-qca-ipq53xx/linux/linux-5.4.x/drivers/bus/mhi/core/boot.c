// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
 *
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/dma-direction.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mhi.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/of.h>
#include <linux/qcom_scm.h>
#include <soc/qcom/license_manager.h>
#include "internal.h"

#define QRTR_INSTANCE_MASK	0x0000FFFF
#define QRTR_INSTANCE_SHIFT	0

#define PCIE_PCIE_LOCAL_REG_PCIE_LOCAL_RSV1 	0x3168
#define PCIE_SOC_PCIE_REG_PCIE_SCRATCH_0	0x4040
#define PCIE_REMAP_BAR_CTRL_OFFSET		0x310C
#define PCIE_SCRATCH_0_WINDOW_VAL		0x4000003C
#define MAX_UNWINDOWED_ADDRESS			0x80000
#define WINDOW_ENABLE_BIT 			0x40000000
#define WINDOW_SHIFT 				19
#define WINDOW_VALUE_MASK 			0x3F
#define WINDOW_START 				MAX_UNWINDOWED_ADDRESS
#define WINDOW_RANGE_MASK 			0x7FFFF
#define PCIE_REG_FOR_BOOT_ARGS			PCIE_SOC_PCIE_REG_PCIE_SCRATCH_0

#define NONCE_SIZE 				34

/* Setup RDDM vector table for RDDM transfer and program RXVEC */
void mhi_rddm_prepare(struct mhi_controller *mhi_cntrl,
		      struct image_info *img_info)
{
	struct mhi_buf *mhi_buf = img_info->mhi_buf;
	struct bhi_vec_entry *bhi_vec = img_info->bhi_vec;
	void __iomem *base = mhi_cntrl->bhie;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	u32 sequence_id;
	unsigned int i;

	for (i = 0; i < img_info->entries - 1; i++, mhi_buf++, bhi_vec++) {
		bhi_vec->dma_addr = cpu_to_le64(mhi_buf->dma_addr);
		bhi_vec->size = cpu_to_le64( mhi_buf->len);
	}

	mhi_buf->dma_addr = dma_map_single(mhi_cntrl->cntrl_dev, mhi_buf->buf,
			mhi_buf->len, DMA_TO_DEVICE);
	if (dma_mapping_error(mhi_cntrl->cntrl_dev, mhi_buf->dma_addr)) {
		dev_err(dev, "dma mapping failed, Address: %p and len: 0x%zx\n",
				&mhi_buf->dma_addr, mhi_buf->len);
		return;
	}

	dev_dbg(dev, "BHIe programming for RDDM\n");

	mhi_write_reg(mhi_cntrl, base, BHIE_RXVECADDR_HIGH_OFFS,
		      upper_32_bits(mhi_buf->dma_addr));

	mhi_write_reg(mhi_cntrl, base, BHIE_RXVECADDR_LOW_OFFS,
		      lower_32_bits(mhi_buf->dma_addr));

	mhi_write_reg(mhi_cntrl, base, BHIE_RXVECSIZE_OFFS, mhi_buf->len);
	sequence_id = MHI_RANDOM_U32_NONZERO(BHIE_RXVECSTATUS_SEQNUM_BMSK);

	mhi_write_reg_field(mhi_cntrl, base, BHIE_RXVECDB_OFFS,
			    BHIE_RXVECDB_SEQNUM_BMSK, BHIE_RXVECDB_SEQNUM_SHFT,
			    sequence_id);

	dev_dbg(dev, "Address: %p and len: 0x%zx sequence: %u\n",
		&mhi_buf->dma_addr, mhi_buf->len, sequence_id);
}

/* Collect RDDM buffer during kernel panic */
static int __mhi_download_rddm_in_panic(struct mhi_controller *mhi_cntrl)
{
	int ret;
	u32 rx_status;
	enum mhi_ee_type ee;
	const u32 delayus = 2000;
	u32 retry = (mhi_cntrl->timeout_ms * 1000) / delayus;
	const u32 rddm_timeout_us = 400000;
	int rddm_retry = rddm_timeout_us / delayus;
	void __iomem *base = mhi_cntrl->bhie;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	struct mhi_buf *mhi_buf = &mhi_cntrl->rddm_image->mhi_buf[
		mhi_cntrl->rddm_image->entries - 1];
	u32 val, i;
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

	dev_dbg(dev, "Entered with pm_state:%s dev_state:%s ee:%s\n",
		to_mhi_pm_state_str(mhi_cntrl->pm_state),
		TO_MHI_STATE_STR(mhi_cntrl->dev_state),
		TO_MHI_EXEC_STR(mhi_cntrl->ee));

	/*
	 * This should only be executing during a kernel panic, we expect all
	 * other cores to shutdown while we're collecting RDDM buffer. After
	 * returning from this function, we expect the device to reset.
	 *
	 * Normaly, we read/write pm_state only after grabbing the
	 * pm_lock, since we're in a panic, skipping it. Also there is no
	 * gurantee that this state change would take effect since
	 * we're setting it w/o grabbing pm_lock
	 */
	mhi_cntrl->pm_state = MHI_PM_LD_ERR_FATAL_DETECT;
	/* update should take the effect immediately */
	smp_wmb();

	/*
	 * Make sure device is not already in RDDM. In case the device asserts
	 * and a kernel panic follows, device will already be in RDDM.
	 * Do not trigger SYS ERR again and proceed with waiting for
	 * image download completion.
	 */
	ee = mhi_get_exec_env(mhi_cntrl);
	if (ee == MHI_EE_MAX)
		goto error_exit_rddm;

	if (ee != MHI_EE_RDDM) {
		dev_dbg(dev, "Trigger device into RDDM mode using SYS ERR\n");
		mhi_set_mhi_state(mhi_cntrl, MHI_STATE_SYS_ERR);

		dev_dbg(dev, "Waiting for device to enter RDDM\n");
		while (rddm_retry--) {
			ee = mhi_get_exec_env(mhi_cntrl);
			if (ee == MHI_EE_RDDM)
				break;

			udelay(delayus);
		}

		if (rddm_retry <= 0) {
			/* Hardware reset so force device to enter RDDM */
			dev_dbg(dev,
				"Did not enter RDDM, do a host req reset\n");
			mhi_write_reg(mhi_cntrl, mhi_cntrl->regs,
				      MHI_SOC_RESET_REQ_OFFSET,
				      MHI_SOC_RESET_REQ);
			udelay(delayus);
		}

		ee = mhi_get_exec_env(mhi_cntrl);
	}

	dev_dbg(dev,
		"Waiting for RDDM image download via BHIe, current EE:%s\n",
		TO_MHI_EXEC_STR(ee));

	while (retry--) {
		ret = mhi_read_reg_field(mhi_cntrl, base, BHIE_RXVECSTATUS_OFFS,
					 BHIE_RXVECSTATUS_STATUS_BMSK,
					 BHIE_RXVECSTATUS_STATUS_SHFT,
					 &rx_status);
		if (ret) {
			dma_unmap_single(mhi_cntrl->cntrl_dev,
					mhi_buf->dma_addr, mhi_buf->len,
					DMA_TO_DEVICE);
			return -EIO;
		}

		if (rx_status == BHIE_RXVECSTATUS_STATUS_XFER_COMPL) {
			dma_unmap_single(mhi_cntrl->cntrl_dev,
					mhi_buf->dma_addr, mhi_buf->len,
					DMA_TO_DEVICE);
			return 0;
		}

		udelay(delayus);
	}

	ee = mhi_get_exec_env(mhi_cntrl);
	ret = mhi_read_reg(mhi_cntrl, base, BHIE_RXVECSTATUS_OFFS, &rx_status);

	dev_err(dev, "RXVEC_STATUS: 0x%x\n", rx_status);
	for (i = 0; error_reg[i].name; i++) {
		ret = mhi_read_reg(mhi_cntrl, mhi_cntrl->bhi,
				   error_reg[i].offset, &val);
		if (ret)
			break;
		dev_err(dev, "reg:%s value:0x%x\n",
			error_reg[i].name, val);
	}

error_exit_rddm:
	dma_unmap_single(mhi_cntrl->cntrl_dev, mhi_buf->dma_addr, mhi_buf->len,
			DMA_TO_DEVICE);

	dev_err(dev, "RDDM transfer failed. Current EE: %s\n",
		TO_MHI_EXEC_STR(ee));

	return -EIO;
}

/* Download RDDM image from device */
int mhi_download_rddm_image(struct mhi_controller *mhi_cntrl, bool in_panic)
{
	void __iomem *base = mhi_cntrl->bhie;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	u32 rx_status;
	struct mhi_buf *mhi_buf = NULL;
	rwlock_t *pm_lock = &mhi_cntrl->pm_lock;
	u32 val, ret, i;
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

	/*
	 * Allocate RDDM table if specified, this table is for debugging purpose
	 */
	if (mhi_cntrl->disable_rddm_prealloc && mhi_cntrl->rddm_size) {
		ret = mhi_alloc_bhie_table(mhi_cntrl, &mhi_cntrl->rddm_image,
				     mhi_cntrl->rddm_size, false);
		if (ret) {
			dev_err(dev, "Failed to allocate RDDM table memory\n");
			return ret;
		}

		/* setup the RX vector table */
		mhi_rddm_prepare(mhi_cntrl, mhi_cntrl->rddm_image);
	}

	if (in_panic)
		return __mhi_download_rddm_in_panic(mhi_cntrl);

	dev_dbg(dev, "Waiting for RDDM image download via BHIe\n");

	/* Wait for the image download to complete */
	wait_event_timeout(mhi_cntrl->state_event,
			   mhi_read_reg_field(mhi_cntrl, base,
					      BHIE_RXVECSTATUS_OFFS,
					      BHIE_RXVECSTATUS_STATUS_BMSK,
					      BHIE_RXVECSTATUS_STATUS_SHFT,
					      &rx_status) || rx_status,
			   msecs_to_jiffies(mhi_cntrl->timeout_ms));

	mhi_buf = &mhi_cntrl->rddm_image->mhi_buf[
		mhi_cntrl->rddm_image->entries - 1];
	dma_unmap_single(mhi_cntrl->cntrl_dev, mhi_buf->dma_addr, mhi_buf->len,
			DMA_TO_DEVICE);

	if (rx_status == BHIE_RXVECSTATUS_STATUS_XFER_COMPL)
		return 0;

	dev_err(dev, "Image download completion timed out, rx_status = %d\n",
		rx_status);
	read_lock_bh(pm_lock);
	if (MHI_REG_ACCESS_VALID(mhi_cntrl->pm_state)) {
		for (i = 0; error_reg[i].name; i++) {
			ret = mhi_read_reg(mhi_cntrl, mhi_cntrl->bhi,
					   error_reg[i].offset, &val);
			if (ret)
				break;
			dev_err(dev, "reg:%s value:0x%x\n",
				error_reg[i].name, val);
		}
	}
	read_unlock_bh(pm_lock);

	return -EIO;
}
EXPORT_SYMBOL_GPL(mhi_download_rddm_image);

static int mhi_fw_load_bhie(struct mhi_controller *mhi_cntrl,
			    struct mhi_buf *mhi_buf)
{
	void __iomem *base = mhi_cntrl->bhie;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	rwlock_t *pm_lock = &mhi_cntrl->pm_lock;
	u32 tx_status, sequence_id;
	int ret;
	u32 val, i;
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


	read_lock_bh(pm_lock);
	if (!MHI_REG_ACCESS_VALID(mhi_cntrl->pm_state)) {
		read_unlock_bh(pm_lock);
		return -EIO;
	}

	mhi_buf->dma_addr = dma_map_single(mhi_cntrl->cntrl_dev, mhi_buf->buf,
			mhi_buf->len, DMA_TO_DEVICE);
	if (dma_mapping_error(mhi_cntrl->cntrl_dev, mhi_buf->dma_addr)) {
		dev_err(dev, "dma mapping failed, Address: %p and len: 0x%zx\n",
				&mhi_buf->dma_addr, mhi_buf->len);
		return -EIO;
	}

	sequence_id = MHI_RANDOM_U32_NONZERO(BHIE_TXVECSTATUS_SEQNUM_BMSK);
	dev_dbg(dev, "Starting image download via BHIe. Sequence ID: %u\n",
		sequence_id);
	mhi_write_reg(mhi_cntrl, base, BHIE_TXVECADDR_HIGH_OFFS,
		      upper_32_bits(mhi_buf->dma_addr));

	mhi_write_reg(mhi_cntrl, base, BHIE_TXVECADDR_LOW_OFFS,
		      lower_32_bits(mhi_buf->dma_addr));

	mhi_write_reg(mhi_cntrl, base, BHIE_TXVECSIZE_OFFS, mhi_buf->len);

	mhi_write_reg_field(mhi_cntrl, base, BHIE_TXVECDB_OFFS,
			    BHIE_TXVECDB_SEQNUM_BMSK, BHIE_TXVECDB_SEQNUM_SHFT,
			    sequence_id);
	read_unlock_bh(pm_lock);

	/* Wait for the image download to complete */
	ret = wait_event_timeout(mhi_cntrl->state_event,
				 MHI_PM_IN_ERROR_STATE(mhi_cntrl->pm_state) ||
				 mhi_read_reg_field(mhi_cntrl, base,
						   BHIE_TXVECSTATUS_OFFS,
						   BHIE_TXVECSTATUS_STATUS_BMSK,
						   BHIE_TXVECSTATUS_STATUS_SHFT,
						   &tx_status) || tx_status,
				 msecs_to_jiffies(mhi_cntrl->timeout_ms));

	dma_unmap_single(mhi_cntrl->cntrl_dev, mhi_buf->dma_addr, mhi_buf->len,
			DMA_TO_DEVICE);

	if (MHI_PM_IN_ERROR_STATE(mhi_cntrl->pm_state) ||
	    tx_status != BHIE_TXVECSTATUS_STATUS_XFER_COMPL) {
		pr_err("Upper:0x%x Lower:0x%x len:0x%zx sequence:%u\n",
			upper_32_bits(mhi_buf->dma_addr),
			lower_32_bits(mhi_buf->dma_addr),
			mhi_buf->len, sequence_id);

		pr_err("MHI pm_state: %s tx_status: %d ee: %s\n",
			to_mhi_pm_state_str(mhi_cntrl->pm_state), tx_status,
			TO_MHI_EXEC_STR(mhi_get_exec_env(mhi_cntrl)));

		read_lock_bh(pm_lock);
		if (MHI_REG_ACCESS_VALID(mhi_cntrl->pm_state)) {
			for (i = 0; error_reg[i].name; i++) {
				ret = mhi_read_reg(mhi_cntrl, base,
						   error_reg[i].offset, &val);
				if (ret)
					break;
				dev_err(dev, "Reg: %s value: 0x%x\n",
					error_reg[i].name, val);
			}
		}
		read_unlock_bh(pm_lock);
		return -EIO;
	}

	return (!ret) ? -ETIMEDOUT : 0;
}

static int mhi_fw_load_bhi(struct mhi_controller *mhi_cntrl,
			   dma_addr_t dma_addr,
			   size_t size)
{
	u32 tx_status, val, session_id;
	int i, ret;
	void __iomem *base = mhi_cntrl->bhi;
	rwlock_t *pm_lock = &mhi_cntrl->pm_lock;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
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

	read_lock_bh(pm_lock);
	if (!MHI_REG_ACCESS_VALID(mhi_cntrl->pm_state)) {
		read_unlock_bh(pm_lock);
		goto invalid_pm_state;
	}

	session_id = MHI_RANDOM_U32_NONZERO(BHI_TXDB_SEQNUM_BMSK);
	dev_dbg(dev, "Starting image download via BHI. Session ID: %u\n",
		session_id);
	mhi_write_reg(mhi_cntrl, base, BHI_STATUS, 0);
	mhi_write_reg(mhi_cntrl, base, BHI_IMGADDR_HIGH,
		      upper_32_bits(dma_addr));
	mhi_write_reg(mhi_cntrl, base, BHI_IMGADDR_LOW,
		      lower_32_bits(dma_addr));
	mhi_write_reg(mhi_cntrl, base, BHI_IMGSIZE, size);
	mhi_write_reg(mhi_cntrl, base, BHI_IMGTXDB, session_id);
	read_unlock_bh(pm_lock);

	/* Wait for the image download to complete */
	ret = wait_event_timeout(mhi_cntrl->state_event,
			   MHI_PM_IN_ERROR_STATE(mhi_cntrl->pm_state) ||
			   mhi_read_reg_field(mhi_cntrl, base, BHI_STATUS,
					      BHI_STATUS_MASK, BHI_STATUS_SHIFT,
					      &tx_status) || tx_status,
			   msecs_to_jiffies(mhi_cntrl->timeout_ms));
	if (MHI_PM_IN_ERROR_STATE(mhi_cntrl->pm_state))
		goto invalid_pm_state;

	if (tx_status == BHI_STATUS_ERROR) {
		dev_err(dev, "Image transfer failed\n");
		read_lock_bh(pm_lock);
		if (MHI_REG_ACCESS_VALID(mhi_cntrl->pm_state)) {
			for (i = 0; error_reg[i].name; i++) {
				ret = mhi_read_reg(mhi_cntrl, base,
						   error_reg[i].offset, &val);
				if (ret)
					break;
				dev_err(dev, "Reg: %s value: 0x%x\n",
					error_reg[i].name, val);
			}
		}
		read_unlock_bh(pm_lock);
		goto invalid_pm_state;
	}

	return (!ret) ? -ETIMEDOUT : 0;

invalid_pm_state:

	return -EIO;
}

void mhi_free_bhie_table(struct mhi_controller *mhi_cntrl,
			 struct image_info *image_info, bool is_fbc)
{
	int i;
	struct mhi_buf *mhi_buf = image_info->mhi_buf;

	for (i = 0; i < image_info->entries; i++, mhi_buf++) {
		/* For FBC image, element mhi_buf[img_info->entries - 2] points
		 * to Dynamic paging region and it should not be freed.
		 */
		if (is_fbc && i == (image_info->entries - 2))
			continue;

		if (i == (image_info->entries - 1))
			kfree(mhi_buf->buf);
		else if (!is_fbc && mhi_cntrl->disable_rddm_prealloc) {
			dma_unmap_single(mhi_cntrl->cntrl_dev,
					mhi_buf->dma_addr, mhi_buf->len,
					DMA_FROM_DEVICE);
			kfree(mhi_buf->buf);
		} else
			mhi_fw_free_coherent(mhi_cntrl, mhi_buf->len,
					mhi_buf->buf,
					mhi_buf->dma_addr);
	}

	kfree(image_info->mhi_buf);
	kfree(image_info);
}

int mhi_update_bhie_table_for_dyn_paging(struct mhi_controller *mhi_cntrl,
					 void *va, phys_addr_t pa,
					 size_t size)
{
	struct image_info *image_info = mhi_cntrl->fbc_image;
	int i, segments;
	struct mhi_buf *mhi_buf;
	struct bhi_vec_entry *bhi_vec;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;

	if (!image_info) {
		dev_err(dev, "FBC Image is NULL\n");
		return -EINVAL;
	}

	segments = image_info->entries;

	/* Find the free entry in bhi_vec table for dynamic paging region */
	bhi_vec = &image_info->bhi_vec[0];
	for (i = 0; (i < segments - 1); i++) {
		if (!bhi_vec->dma_addr)
			break;

		bhi_vec++;
	}
	if (i == (segments - 1)) {
		dev_err(dev, "No space in Vector Table\n");
		return -ENOMEM;
	}

	bhi_vec->dma_addr = pa;
	bhi_vec->size = size;

	/* mhi_buf[segments - 2] is reserved Dynamic Paging region */
	mhi_buf = &image_info->mhi_buf[segments - 2];
	mhi_buf->buf = va;
	mhi_buf->dma_addr = pa;
	mhi_buf->len = size;

	return 0;
}
EXPORT_SYMBOL(mhi_update_bhie_table_for_dyn_paging);

int mhi_alloc_bhie_table(struct mhi_controller *mhi_cntrl,
			 struct image_info **image_info,
			 size_t alloc_size, bool is_fbc)
{
	size_t seg_size = mhi_cntrl->seg_len;
	int segments = 0;
	int i;
	struct image_info *img_info;
	struct mhi_buf *mhi_buf;
	/* Maksed __GFP_DIRECT_RECLAIM flag for non-interrupt context
	   to avoid rcu context sleep issue in kmalloc during panic scenario */
	gfp_t gfp = (in_interrupt() ? GFP_ATOMIC :
		((GFP_KERNEL | __GFP_NORETRY) & ~__GFP_DIRECT_RECLAIM));

	/* Allocate one extra entry for Dynamic Pageable in FBC */
	if (is_fbc)
		segments++;
	else if (mhi_cntrl->disable_rddm_prealloc)
		seg_size = mhi_cntrl->rddm_seg_len;

	segments += DIV_ROUND_UP(alloc_size, seg_size) + 1;

	img_info = kzalloc(sizeof(*img_info), gfp);
	if (!img_info)
		return -ENOMEM;

	/* Allocate memory for entries */
	img_info->mhi_buf = kcalloc(segments, sizeof(*img_info->mhi_buf),
				    gfp);
	if (!img_info->mhi_buf)
		goto error_alloc_mhi_buf;

	/* Allocate and populate vector table */
	mhi_buf = img_info->mhi_buf;
	for (i = 0; i < segments; i++, mhi_buf++) {
		size_t vec_size = seg_size;

		if (is_fbc && (i == segments - 2)) {
			/* Initialize an entry for Dynamic paging region which
			 * would be updated later in
			 * mhi_update_bhie_table_for_dyn_paging
			 */
			vec_size = 0;
			mhi_buf->buf = NULL;
			mhi_buf->dma_addr = 0;
		} else if (i == segments - 1) {
			/* last entry is for vector table */
			vec_size = sizeof(struct bhi_vec_entry) * i;
			mhi_buf->buf = kzalloc(PAGE_ALIGN(vec_size), gfp);
			if (!mhi_buf->buf)
				goto error_alloc_segment;
		} else {
			if (!is_fbc && mhi_cntrl->disable_rddm_prealloc) {
				mhi_buf->buf = kmalloc(vec_size, gfp);
				if (!mhi_buf->buf)
					goto error_alloc_segment;

				mhi_buf->dma_addr = dma_map_single(
						mhi_cntrl->cntrl_dev,
						mhi_buf->buf, vec_size,
						DMA_FROM_DEVICE);
				if (dma_mapping_error(mhi_cntrl->cntrl_dev,
							mhi_buf->dma_addr)) {
					kfree(mhi_buf->buf);
					mhi_buf->buf = NULL;
				}
			} else
				mhi_buf->buf = mhi_fw_alloc_coherent(mhi_cntrl,
							vec_size,
							&mhi_buf->dma_addr,
							GFP_KERNEL);

			if (!mhi_buf->buf)
				goto error_alloc_segment;
		}

		mhi_buf->len = vec_size;
	}

	img_info->bhi_vec = img_info->mhi_buf[segments - 1].buf;
	img_info->entries = segments;
	*image_info = img_info;

	return 0;

error_alloc_segment:
	for (--i, --mhi_buf; i >= 0; i--, mhi_buf--) {
		if (!is_fbc && mhi_cntrl->disable_rddm_prealloc) {
			dma_unmap_single(mhi_cntrl->cntrl_dev,
					mhi_buf->dma_addr, mhi_buf->len,
					DMA_FROM_DEVICE);
			kfree(mhi_buf->buf);
		} else
			mhi_fw_free_coherent(mhi_cntrl, mhi_buf->len,
					mhi_buf->buf, mhi_buf->dma_addr);
	}

error_alloc_mhi_buf:
	kfree(img_info);

	return -ENOMEM;
}

static void mhi_firmware_copy(struct mhi_controller *mhi_cntrl,
			      const struct firmware *firmware,
			      struct image_info *img_info)
{
	size_t remainder = firmware->size;
	size_t to_cpy;
	const u8 *buf = firmware->data;
	struct mhi_buf *mhi_buf = img_info->mhi_buf;
	struct bhi_vec_entry *bhi_vec = img_info->bhi_vec;

	while (remainder) {
		to_cpy = min(remainder, mhi_buf->len);
		memcpy(mhi_buf->buf, buf, to_cpy);
		bhi_vec->dma_addr = cpu_to_le64(mhi_buf->dma_addr);
		bhi_vec->size = cpu_to_le64(mhi_buf->len);

		buf += to_cpy;
		remainder -= to_cpy;
		bhi_vec++;
		mhi_buf++;
	}
}

static int mhi_select_window(struct mhi_controller *mhi_cntrl, u32 addr)
{
	u32 window = (addr >> WINDOW_SHIFT) & WINDOW_VALUE_MASK;
	u32 prev_window = 0, curr_window = 0;
	u32 read_val = 0;
	int retry = 0;

	mhi_read_reg(mhi_cntrl, mhi_cntrl->regs, PCIE_REMAP_BAR_CTRL_OFFSET, &prev_window);

	/* Using the last 6 bits for Window 1. Window 2 and 3 are unaffected */
	curr_window = (prev_window & ~(WINDOW_VALUE_MASK)) | window;

	if (curr_window == prev_window)
		return 0;

	curr_window |= WINDOW_ENABLE_BIT;

	mhi_write_reg(mhi_cntrl, mhi_cntrl->regs, PCIE_REMAP_BAR_CTRL_OFFSET, curr_window);

	mhi_read_reg(mhi_cntrl, mhi_cntrl->regs, PCIE_REMAP_BAR_CTRL_OFFSET, &read_val);

	/* Wait till written value reflects */
	while((read_val != curr_window) && (retry < 10)) {
		mdelay(1);
		mhi_read_reg(mhi_cntrl, mhi_cntrl->regs, PCIE_REMAP_BAR_CTRL_OFFSET, &read_val);
		retry++;
	}

	if(read_val != curr_window)
		return -EINVAL;

	return 0;
}

void mhi_free_nonce_buffer(struct mhi_controller *mhi_cntrl)
{
	if (mhi_cntrl->nonce_buf != NULL) {
		mhi_fw_free_coherent(mhi_cntrl, NONCE_SIZE, mhi_cntrl->nonce_buf,
				mhi_cntrl->nonce_dma_addr);
		mhi_cntrl->nonce_buf = NULL;
	}
}

static int mhi_get_nonce(struct mhi_controller *mhi_cntrl)
{
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	unsigned int sram_addr, rd_addr;
	unsigned int rd_val;
	int ret, i;

	dev_info(dev, "Reading NONCE from Endpoint\n");

	mhi_read_reg(mhi_cntrl, mhi_cntrl->regs, PCIE_PCIE_LOCAL_REG_PCIE_LOCAL_RSV1,
			&sram_addr);
	if (sram_addr != 0) {
		mhi_cntrl->nonce_buf = mhi_fw_alloc_coherent(mhi_cntrl, NONCE_SIZE,
					&mhi_cntrl->nonce_dma_addr, GFP_KERNEL);
		if (!mhi_cntrl->nonce_buf) {
			dev_err(dev, "Error Allocating memory buffer for NONCE\n");
			return -ENOMEM;
		}

		/* Select window to read the NONCE from Q6 SRAM address */
		ret = mhi_select_window(mhi_cntrl, sram_addr);
		if (ret)
			return ret;

		for (i=0; i < NONCE_SIZE; i+=4) {
			/* Calculate read address based on the Window range and read it */
			rd_addr = ((sram_addr + i) & WINDOW_RANGE_MASK) + WINDOW_START;
			mhi_read_reg(mhi_cntrl, mhi_cntrl->regs, rd_addr, &rd_val);

			/* Copy the read value to nonce_buf */
			memcpy(mhi_cntrl->nonce_buf + i, &rd_val, 4);
		}
	}
	else {
		dev_err(dev, "No NONCE from device\n");
		mhi_cntrl->nonce_buf = NULL;
		mhi_cntrl->nonce_dma_addr = 0;
	}

	return 0;
}

static void mhi_download_fw_license(struct mhi_controller *mhi_cntrl)
{
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	int  ret;

	ret = mhi_get_nonce(mhi_cntrl);
	if (ret) {
		mhi_cntrl->nonce_dma_addr = 0;
		mhi_free_nonce_buffer(mhi_cntrl);
	}

	mhi_cntrl->license_buf = lm_get_license(EXTERNAL, &mhi_cntrl->license_dma_addr,
			&mhi_cntrl->license_buf_size, mhi_cntrl->nonce_dma_addr);

	if (!mhi_cntrl->license_buf) {
		mhi_free_nonce_buffer(mhi_cntrl);
		mhi_write_reg(mhi_cntrl, mhi_cntrl->regs,
				PCIE_PCIE_LOCAL_REG_PCIE_LOCAL_RSV1, (u32)0x0);
		dev_info(dev, "No license file passed in RSV1\n");
		return;
	}

	/* Let device know the about license data. Assuming 32 bit only*/
	mhi_write_reg(mhi_cntrl, mhi_cntrl->regs,
				PCIE_PCIE_LOCAL_REG_PCIE_LOCAL_RSV1,
					      lower_32_bits(mhi_cntrl->license_dma_addr));
	dev_dbg(dev, "DMA address 0x%x is copied to EP's RSV1\n",lower_32_bits(mhi_cntrl->license_dma_addr));

	dev_info(dev, "License file address copied to PCIE_PCIE_LOCAL_REG_PCIE_LOCAL_RSV1\n");

	return;
}

static int mhi_update_scratch_reg(struct mhi_controller *mhi_cntrl, u32 val)
{
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	u32 rd_val;

	/* Program Window register to update boot args pointer */
	mhi_read_reg(mhi_cntrl, mhi_cntrl->regs, PCIE_REMAP_BAR_CTRL_OFFSET,
			&rd_val);

	rd_val = rd_val & ~(0x3f);

	mhi_write_reg(mhi_cntrl, mhi_cntrl->regs, PCIE_REMAP_BAR_CTRL_OFFSET,
				PCIE_SCRATCH_0_WINDOW_VAL | rd_val);

	mhi_write_reg(mhi_cntrl, mhi_cntrl->regs + MAX_UNWINDOWED_ADDRESS,
			PCIE_REG_FOR_BOOT_ARGS, val);

	mhi_read_reg(mhi_cntrl, mhi_cntrl->regs + MAX_UNWINDOWED_ADDRESS,
			PCIE_REG_FOR_BOOT_ARGS,	&rd_val);

	if (rd_val != val) {
		dev_err(dev, "Write to PCIE_REG_FOR_BOOT_ARGS register failed\n");
		return -EFAULT;
	}

	return 0;
}

static int mhi_handle_boot_args(struct mhi_controller *mhi_cntrl)
{
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	int i, cnt, ret;
	u32 val;

	if (!mhi_cntrl->cntrl_dev->of_node)
		return -EINVAL;

	cnt = of_property_count_u32_elems(mhi_cntrl->cntrl_dev->of_node,
					  "boot-args");
	if (cnt < 0) {
		dev_err(dev, "boot-args not defined in DTS. ret:%d\n", cnt);
		mhi_update_scratch_reg(mhi_cntrl, 0);
		return 0;
	}

	mhi_cntrl->bootargs_buf = mhi_fw_alloc_coherent(mhi_cntrl, PAGE_SIZE,
			&mhi_cntrl->bootargs_dma, GFP_KERNEL);

	if (!mhi_cntrl->bootargs_buf) {
		mhi_update_scratch_reg(mhi_cntrl, 0);
		return -ENOMEM;
	}

	for (i = 0; i < cnt; i++) {
		ret = of_property_read_u32_index(mhi_cntrl->cntrl_dev->of_node,
							"boot-args", i, &val);
		if (ret) {
			dev_err(dev, "failed to read boot args\n");
			mhi_fw_free_coherent(mhi_cntrl, PAGE_SIZE,
				mhi_cntrl->bootargs_buf, mhi_cntrl->bootargs_dma);
			mhi_cntrl->bootargs_buf = NULL;
			mhi_update_scratch_reg(mhi_cntrl, 0);
			return ret;
		}
		mhi_cntrl->bootargs_buf[i] = (u8)val;
	}

	ret = mhi_update_scratch_reg(mhi_cntrl, lower_32_bits(mhi_cntrl->bootargs_dma));

	dev_dbg(dev, "boot-args address copied to PCIE_REG_FOR_BOOT_ARGS\n");

	return ret;
}

void mhi_free_boot_args(struct mhi_controller *mhi_cntrl)
{
	if (mhi_cntrl->bootargs_buf != NULL) {
		mhi_fw_free_coherent(mhi_cntrl, PAGE_SIZE, mhi_cntrl->bootargs_buf, mhi_cntrl->bootargs_dma);
		mhi_cntrl->bootargs_buf = NULL;
	}
}

void mhi_fw_load_handler(struct mhi_controller *mhi_cntrl)
{
	const struct firmware *firmware = NULL;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	const char *fw_name;
	void *buf;
	dma_addr_t dma_addr;
	size_t size;
	int i, ret;
	u32 instance;

	if (MHI_PM_IN_ERROR_STATE(mhi_cntrl->pm_state)) {
		dev_err(dev, "Device MHI is not in valid state\n");
		return;
	}

	/* save hardware info from BHI */
	ret = mhi_read_reg(mhi_cntrl, mhi_cntrl->bhi, BHI_SERIALNU,
			   &mhi_cntrl->serial_number);
	if (ret)
		dev_err(dev, "Could not capture serial number via BHI\n");

	for (i = 0; i < ARRAY_SIZE(mhi_cntrl->oem_pk_hash); i++) {
		ret = mhi_read_reg(mhi_cntrl, mhi_cntrl->bhi, BHI_OEMPKHASH(i),
				   &mhi_cntrl->oem_pk_hash[i]);
		if (ret) {
			dev_err(dev, "Could not capture OEM PK HASH via BHI\n");
			break;
		}
	}

	/* wait for ready on pass through or any other execution environment */
	if (mhi_cntrl->ee != MHI_EE_EDL && mhi_cntrl->ee != MHI_EE_PBL)
		goto fw_load_ready_state;

	fw_name = (mhi_cntrl->ee == MHI_EE_EDL) ?
		mhi_cntrl->edl_image : mhi_cntrl->fw_image;

	if (!fw_name || (mhi_cntrl->fbc_download && (!mhi_cntrl->sbl_size ||
						     !mhi_cntrl->seg_len))) {
		dev_err(dev,
			"No firmware image defined or !sbl_size || !seg_len\n");
		goto error_fw_load;
	}

	ret = request_firmware(&firmware, fw_name, dev);
	if (ret) {
		dev_err(dev, "Error loading firmware: %d\n", ret);
		goto error_fw_load;
	}

	size = (mhi_cntrl->fbc_download) ? mhi_cntrl->sbl_size : firmware->size;

	/* SBL size provided is maximum size, not necessarily the image size */
	if (size > firmware->size)
		size = firmware->size;

	buf = mhi_fw_alloc_coherent(mhi_cntrl, size, &dma_addr, GFP_KERNEL);
	if (!buf) {
		release_firmware(firmware);
		goto error_fw_load;
	}

	/* Download image using BHI */
	memcpy(buf, firmware->data, size);
	ret = mhi_fw_load_bhi(mhi_cntrl, dma_addr, size);
	mhi_fw_free_coherent(mhi_cntrl, size, buf, dma_addr);

	/* Error or in EDL mode, we're done */
	if (ret) {
		dev_err(dev, "MHI did not load image over BHI, ret: %d\n", ret);
		release_firmware(firmware);
		goto error_fw_load;
	}

	/* Wait for ready since EDL image was loaded */
	if (fw_name == mhi_cntrl->edl_image) {
		release_firmware(firmware);
		mhi_uevent_notify(mhi_cntrl, MHI_EE_EDL);
		goto fw_load_ready_state;
	}

	if (!ret && mhi_cntrl->cntrl_dev->of_node) {
		ret = of_property_read_u32(mhi_cntrl->cntrl_dev->of_node,
					   "qrtr_instance_id", &instance);
		if (!ret) {
			instance &= QRTR_INSTANCE_MASK;
			mhi_write_reg_field(mhi_cntrl, mhi_cntrl->bhi,
					    BHI_ERRDBG2, QRTR_INSTANCE_MASK,
					    QRTR_INSTANCE_SHIFT, instance);
		} else {
			dev_err(dev,
				"qrtr_instance_id not defined in DT, ret:%d\n",
				ret);
		}
	}

	write_lock_irq(&mhi_cntrl->pm_lock);
	mhi_cntrl->dev_state = MHI_STATE_RESET;
	write_unlock_irq(&mhi_cntrl->pm_lock);

	/*
	 * If we're doing fbc, populate vector tables while
	 * device transitioning into MHI READY state
	 */
	if (mhi_cntrl->fbc_download) {
		ret = mhi_alloc_bhie_table(mhi_cntrl, &mhi_cntrl->fbc_image,
					   firmware->size, true);
		if (ret) {
			release_firmware(firmware);
			goto error_fw_load;
		}

		/* Load the firmware into BHIE vec table */
		mhi_firmware_copy(mhi_cntrl, firmware, mhi_cntrl->fbc_image);
	}

	release_firmware(firmware);

fw_load_ready_state:
	/* Transitioning into MHI RESET->READY state */
	ret = mhi_ready_state_transition(mhi_cntrl);
	if (ret) {
		dev_err(dev, "MHI did not enter READY state\n");
		goto error_ready_state;
	}

	dev_info(dev, "Wait for device to enter SBL or Mission mode\n");
	return;

error_ready_state:
	if (mhi_cntrl->fbc_download) {
		mhi_free_bhie_table(mhi_cntrl, mhi_cntrl->fbc_image, true);
		mhi_cntrl->fbc_image = NULL;
	}

error_fw_load:
	mhi_cntrl->pm_state = MHI_PM_FW_DL_ERR;
	wake_up_all(&mhi_cntrl->state_event);
}

int mhi_download_amss_image(struct mhi_controller *mhi_cntrl)
{
	struct image_info *image_info = mhi_cntrl->fbc_image;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	int ret;

	if (!image_info)
		return -EIO;

	ret = mhi_handle_boot_args(mhi_cntrl);
	if(ret) {
		dev_err(dev, "Failed to handle the boot-args, ret: %d\n",ret);
		return ret;
	}

	if (mhi_cntrl->dev_id == QCN9224_DEVICE_ID) {
		/* Download the License */
		mhi_download_fw_license(mhi_cntrl);
	}

	ret = mhi_fw_load_bhie(mhi_cntrl,
			       /* Vector table is the last entry */
			       &image_info->mhi_buf[image_info->entries - 1]);
	if (ret) {
		dev_err(dev, "MHI did not load AMSS, ret:%d\n", ret);
		mhi_cntrl->pm_state = MHI_PM_FW_DL_ERR;
		wake_up_all(&mhi_cntrl->state_event);
	}

	return ret;
}
