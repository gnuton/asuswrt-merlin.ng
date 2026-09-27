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

#include <linux/kernel.h>
#include "commonmhitest.h"
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/sched.h>

#define QCN9000_DEFAULT_FW_FILE_NAME	"qcn9000/amss.bin"
#define QCN9224_DEFAULT_FW_FILE_NAME	"qcn9224/amss.bin"

int mhitest_ss_powerup(struct rproc *subsys_desc)
{
	int ret;
	struct mhitest_platform *temp;

	MHITEST_LOG("Enter\n");

	temp = subsys_desc->priv;
	if (!temp) {
		MHITEST_ERR("Error not dev data\n");
		return -EINVAL;
	}
	MHITEST_VERB("temp:[%p]\n", temp);

	if (!temp->pci_dev) {
		pr_mhitest2("temp->pci_Dev is NULL\n");
		return -ENODEV;
	}

	if (temp->device_id == QCN92XX_DEVICE_ID)
		snprintf(temp->fw_name, sizeof(temp->fw_name),
					QCN9224_DEFAULT_FW_FILE_NAME);
	else
		snprintf(temp->fw_name, sizeof(temp->fw_name),
					QCN9000_DEFAULT_FW_FILE_NAME);


	ret = mhitest_prepare_pci_mhi_msi(temp);
	if (ret) {
		MHITEST_ERR("Error prep. pci_mhi_msi  ret:%d\n", ret);
		return ret;
	}

	ret = mhitest_prepare_start_mhi(temp);
	if (ret) {
		MHITEST_ERR("Error preapare start mhi  ret:%d\n", ret);
		return ret;
	}

	MHITEST_LOG("Exit\n");

	return 0;
}

int mhitest_ss_shutdown(struct rproc *subsys_desc)
{

	struct mhitest_platform *mplat;

	MHITEST_VERB("Going for shutdown\n");

	mplat = subsys_desc->priv;

	mhitest_pci_set_mhi_state(mplat, MHI_POWER_OFF);
	mhitest_pci_set_mhi_state(mplat, MHI_DEINIT);
	mhitest_pci_remove_all(mplat);

	return 0;
}

int mhitest_ss_dummy_load(struct rproc *subsys_desc,
					const struct firmware *fw)
{
	/* no firmware load it will taken care by pci and mhi */
	return 0;
}

int mhitest_ss_add_ramdump_callback(struct rproc *subsys_desc,
			const struct firmware *firmware)
{
	MHITEST_LOG("Dummy callback, returning 0...\n");
	return 0;
}

const struct rproc_ops mhitest_rproc_ops = {
	.start = mhitest_ss_powerup,
	.stop = mhitest_ss_shutdown,
	.load = mhitest_ss_dummy_load,
	.parse_fw = mhitest_ss_add_ramdump_callback,
};

int mhitest_subsystem_register(struct mhitest_platform *mplat)
{
	MHITEST_VERB("Going for ss_reg.\n");

	if (mplat->d_instance == 0)
		mplat->mhitest_ss_desc_name = "mhitest-ss-0";
	else
		mplat->mhitest_ss_desc_name = "mhitest-ss-1";

	MHITEST_VERB("SS name :%s\n", mplat->mhitest_ss_desc_name);

	MHITEST_VERB("Doing rproc alloc..\n");

	if (mplat->device_id == QCN92XX_DEVICE_ID)
		mplat->subsys_handle = rproc_alloc(&mplat->plat_dev->dev,
						  mplat->mhitest_ss_desc_name,
						  &mhitest_rproc_ops,
						  QCN9224_DEFAULT_FW_FILE_NAME, 0);
	else
		mplat->subsys_handle = rproc_alloc(&mplat->plat_dev->dev,
						  mplat->mhitest_ss_desc_name,
						  &mhitest_rproc_ops,
						  QCN9000_DEFAULT_FW_FILE_NAME, 0);

	if (!mplat->subsys_handle) {
		MHITEST_ERR("rproc_alloc returned NULL..\n");
		return -EINVAL;
	}

	mplat->subsys_handle->priv = mplat;
	mplat->subsys_handle->auto_boot = false;

	MHITEST_VERB("Doing rproc add..\n");
	if (rproc_add(mplat->subsys_handle))
		return -EINVAL;

	if (!mplat->subsys_handle->dev.parent) {
		MHITEST_ERR("dev is null\n");
		return -ENODEV;
	}

	MHITEST_VERB("Doing rproc boot..\n");
	if (rproc_boot(mplat->subsys_handle))
		return -EINVAL;

	return 0;
}

void mhitest_subsystem_unregister(struct mhitest_platform *mplat)
{
	if (!mplat) {
		MHITEST_ERR("mplat is null, no subsystem unregister\n");
		return;
	}

	if (!mplat->subsys_handle) {
		MHITEST_ERR("mplat->subsys_handle is NULL\n");
		return;
	}

	rproc_shutdown(mplat->subsys_handle);
	rproc_del(mplat->subsys_handle);
	rproc_free(mplat->subsys_handle);
	mplat->subsys_handle = NULL;
}
