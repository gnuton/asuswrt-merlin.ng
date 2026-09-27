/*
 * Copyright (c) 2020 The Linux Foundation. All rights reserved.
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

#include <linux/elf.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/soc/qcom/mdt_loader.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/qcom_scm.h>
#include <linux/clk.h>
#include <linux/bt.h>

static bool auto_load;
module_param(auto_load, bool, 0644);

unsigned
int m0_btss_load_address(struct rproc *rproc, const struct firmware *fw)
{
	int i;
	struct elf32_hdr *ehdr;
	struct elf32_phdr *phdr;
	struct elf32_phdr *phdrs;
	phys_addr_t min_addr = PHYS_ADDR_MAX;

	ehdr = (struct elf32_hdr *)fw->data;
	phdrs = (struct elf32_phdr *)(ehdr + 1);

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = &phdrs[i];
		if ((phdr->p_type != PT_LOAD) || !phdr->p_memsz)
			continue;

		if (phdr->p_vaddr < min_addr)
			break;
	}

	return phdr->p_vaddr;
}

int m0_btss_start(struct rproc *rproc)
{
	int ret = 0;
	struct bt_descriptor *btDesc = rproc->priv;

	if (rproc->state == RPROC_OFFLINE) {
		ret = bt_ipc_init(btDesc);
		if (ret) {
			dev_err(rproc->dev.parent, "%s err initializing IPC\n",
					__func__);
			return ret;
		}


		if (!btDesc->nosecure) {
			ret = qcom_scm_pas_auth_and_reset(PAS_ID, 0, CMD_ID);
			if (ret) {
				dev_err(rproc->dev.parent,
						"secure reset failed\n");
				return ret;
			}

		} else {
			ret = reset_control_deassert(btDesc->btss_reset);
			if (ret) {
				dev_err(rproc->dev.parent,
						"non secure reset failed\n");
				return ret;
			}

			mdelay(50);

			writel(0x1, btDesc->warm_reset + BT_M0_WARM_RST);
			writel(0x0, btDesc->warm_reset + BT_M0_WARM_RST_ORIDE);
		}

		dev_info(rproc->dev.parent, "%s\n", __func__);
	}

	return ret;
}

int m0_btss_stop(struct rproc *rproc)
{
	int ret = 0;
	struct bt_descriptor *btDesc = rproc->priv;

	if (!atomic_read(&rproc->power) && ((rproc->state == RPROC_RUNNING) |
				(rproc->state == RPROC_CRASHED))) {
		bt_ipc_deinit(btDesc);

		if (!btDesc->nosecure) {
			ret = qcom_scm_pas_shutdown(PAS_ID);
			if (ret) {
				dev_err(rproc->dev.parent, "failed, ret = %d\n",
						ret);
				return ret;
			}
		} else {
			ret = reset_control_assert(btDesc->btss_reset);
			if (ret) {
				dev_err(rproc->dev.parent,
				"non secure assert failed, ret = %d\n", ret);
				return ret;
			}

			mdelay(50);

			writel(0x0, btDesc->warm_reset + BT_M0_WARM_RST);
			writel(0x1, btDesc->warm_reset + BT_M0_WARM_RST_ORIDE);

			mdelay(50);
			ret = reset_control_deassert(btDesc->btss_reset);
			if (ret) {
				dev_err(rproc->dev.parent,
				"non secure deassert failed, ret = %d\n", ret);
				return ret;
			}
		}

		dev_info(rproc->dev.parent, "%s\n", __func__);
	}
	return ret;
}

int m0_btss_load(struct rproc *rproc, const struct firmware *fw)
{
	int ret;
	uint32_t offset;
	struct bt_descriptor *btDesc = rproc->priv;

	offset = m0_btss_load_address(rproc, fw);

	if (!btDesc->nosecure) {
		ret = qcom_mdt_load(rproc->dev.parent, fw, rproc->firmware,
				PAS_ID, btDesc->btmem.virt + offset,
				btDesc->btmem.phys, btDesc->btmem.size,
				&btDesc->btmem.reloc);

	} else {
		ret = qti_scm_load_otp(PAS_ID);
		if (ret)
			dev_info(rproc->dev.parent, "secure OTP copy failed\n");

		ret = qcom_mdt_load_no_init(rproc->dev.parent, fw,
				rproc->firmware, 0, btDesc->btmem.virt + offset,
				btDesc->btmem.phys, btDesc->btmem.size,
				&btDesc->btmem.reloc);
	}


	if (ret)
		dev_err(rproc->dev.parent,
				"Could not load firmware, ret = %d\n", ret);
	else
		dev_info(rproc->dev.parent, "%s\n", __func__);

	return ret;
}

static const struct rproc_ops m0_btss_ops = {
	.start = m0_btss_start,
	.stop = m0_btss_stop,
	.load = m0_btss_load,
	.get_boot_addr = rproc_elf_get_boot_addr,
};

static int bt_rproc_probe(struct platform_device *pdev)
{
	int ret;
	struct bt_descriptor *btDesc = *((struct bt_descriptor **)
						pdev->dev.platform_data);
	struct rproc *rproc;

	ret = clk_prepare_enable(btDesc->lpo_clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to prepare/enable clock\n");
		return ret;
	}

	rproc = rproc_alloc(&pdev->dev, pdev->name, &m0_btss_ops,
							btDesc->fw_name, 0);
	if (!rproc) {
		dev_err(&pdev->dev, "failed to allocate rproc\n");
		return -ENOMEM;
	}

	rproc->priv = btDesc;
	rproc->auto_boot = auto_load;

	if (!rproc->dev.class->p) {
		dev_err(&pdev->dev, "class not registered defering probe\n");
		return -EPROBE_DEFER;
	}

	ret = reset_control_deassert(btDesc->btss_reset);
	if (ret) {
		dev_err(&btDesc->pdev->dev, "btss_reset failed\n");
		rproc_free(rproc);
		return ret;
	}

	ret = rproc_add(rproc);
	if (ret) {
		dev_err(&pdev->dev, "rproc_add failed, ret = %d\n", ret);
		rproc_free(rproc);
		return ret;
	}

	if (of_property_read_bool(btDesc->pdev->dev.of_node,
						"qcom,bt-running")) {
		rproc->state = RPROC_RUNNING;
		atomic_inc(&rproc->power);
		dev_info(&btDesc->pdev->dev, "Started at bootloader\n");
	}

	platform_set_drvdata(pdev, rproc);


	if (of_machine_is_compatible("qcom,ipq5018-mp02.1")) {
		ret = qti_scm_pil_cfg_available();
		if (ret) {
			ret = qti_scm_pil_cfg(PAS_ID, 0x1);
			if (ret) {
				dev_err(rproc->dev.parent,
						"Failed to update XO/TCXO");
				return ret;
			}
			dev_info(rproc->dev.parent, "Updated XO/TCXO config\n");
		} else {
			dev_info(&pdev->dev, "SCM call not available\n");
		}
	}

	dev_info(&pdev->dev, "Probed\n");

	return 0;
}

static int bt_rproc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct bt_descriptor *btDesc = rproc->priv;

	atomic_set(&btDesc->state, 0);
	rproc_del(rproc);
	rproc_free(rproc);
	clk_disable_unprepare(btDesc->lpo_clk);

	return 0;
}

static struct platform_driver bt_rproc_driver = {
	.probe = bt_rproc_probe,
	.remove = bt_rproc_remove,
	.driver = {
		.name = "bt_rproc_driver",
		.owner = THIS_MODULE,
	},
};

static int __init bt_rproc_init(void)
{
	int ret;

	ret = platform_driver_register(&bt_rproc_driver);
	if (ret)
		pr_err("%s: plat_driver registeration  failed\n", __func__);

	return ret;
}

static void __exit bt_rproc_exit(void)
{
	platform_driver_unregister(&bt_rproc_driver);
}

module_init(bt_rproc_init);
module_exit(bt_rproc_exit);

MODULE_DESCRIPTION("QTI Technologies, Inc.");
MODULE_LICENSE("GPL v2");
