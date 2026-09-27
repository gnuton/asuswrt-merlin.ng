// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.*/

#include <linux/async.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/dma-direction.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/memblock.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/mhi.h>
#include "mhi_qti.h"

struct arch_info {
	struct mhi_dev *mhi_dev;
	u32 bus_client;
	struct pci_saved_state *pcie_state;
	struct pci_saved_state *ref_pcie_state;
	struct dma_iommu_mapping *mapping;
};

static int mhi_arch_set_bus_request(struct mhi_controller *mhi_cntrl, int index)
{
#ifdef CONFIG_MSM_BUS_SCALING
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct arch_info *arch_info = mhi_dev->arch_info;

	MHI_LOG("Setting bus request to index %d\n", index);

	if (arch_info->bus_client)
		return msm_bus_scale_client_update_request(
							arch_info->bus_client,
							index);
#endif

	/* default return success */
	return 0;
}

int mhi_arch_pcie_init(struct mhi_controller *mhi_cntrl)
{
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct arch_info *arch_info = mhi_dev->arch_info;

	if (!arch_info) {
		arch_info = devm_kzalloc(&mhi_dev->pci_dev->dev,
					 sizeof(*arch_info), GFP_KERNEL);
		if (!arch_info)
			return -ENOMEM;

		mhi_dev->arch_info = arch_info;

		/* save reference state for pcie config space */
		arch_info->ref_pcie_state = pci_store_saved_state(
							mhi_dev->pci_dev);

	}

	return mhi_arch_set_bus_request(mhi_cntrl, 1);
}

void mhi_arch_pcie_deinit(struct mhi_controller *mhi_cntrl)
{
}

int mhi_arch_link_off(struct mhi_controller *mhi_cntrl, bool graceful)
{
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct arch_info *arch_info = mhi_dev->arch_info;
	struct pci_dev *pci_dev = mhi_dev->pci_dev;
	int ret;

	MHI_LOG("Entered\n");

	if (graceful) {
		pci_clear_master(pci_dev);
		ret = pci_save_state(mhi_dev->pci_dev);
		if (ret) {
			MHI_ERR("Failed with pci_save_state, ret:%d\n", ret);
			return ret;
		}

		arch_info->pcie_state = pci_store_saved_state(pci_dev);
		pci_disable_device(pci_dev);
	}

	/*
	 * We will always attempt to put link into D3hot, however
	 * link down may have happened due to error fatal, so
	 * ignoring the return code
	 */
	pci_set_power_state(pci_dev, PCI_D3hot);

	/* release the resources */
	mhi_arch_set_bus_request(mhi_cntrl, 0);

	MHI_LOG("Exited\n");

	return 0;
}

int mhi_arch_link_on(struct mhi_controller *mhi_cntrl)
{
	struct mhi_dev *mhi_dev = mhi_controller_get_devdata(mhi_cntrl);
	struct arch_info *arch_info = mhi_dev->arch_info;
	struct pci_dev *pci_dev = mhi_dev->pci_dev;
	int ret;

	MHI_LOG("Entered\n");

	/* request resources and establish link trainning */
	ret = mhi_arch_set_bus_request(mhi_cntrl, 1);
	if (ret)
		MHI_LOG("Could not set bus frequency, ret:%d\n", ret);

	ret = pci_set_power_state(pci_dev, PCI_D0);
	if (ret) {
		MHI_ERR("Failed to set PCI_D0 state, ret:%d\n", ret);
		return ret;
	}

	ret = pci_enable_device(pci_dev);
	if (ret) {
		MHI_ERR("Failed to enable device, ret:%d\n", ret);
		return ret;
	}

	ret = pci_load_and_free_saved_state(pci_dev, &arch_info->pcie_state);
	if (ret)
		MHI_LOG("Failed to load saved cfg state\n");

	pci_restore_state(pci_dev);
	pci_set_master(pci_dev);

	MHI_LOG("Exited\n");

	return 0;
}
