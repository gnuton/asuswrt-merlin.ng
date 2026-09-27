// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016-2018 Linaro Ltd.
 * Copyright (C) 2014 Sony Mobile Communications AB
 * Copyright (c) 2012-2018, 2021 The Linux Foundation. All rights reserved.
 */
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/soc/qcom/mdt_loader.h>
#include <linux/soc/qcom/smem.h>
#include <linux/soc/qcom/smem_state.h>
#include <linux/qcom_scm.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/dma-mapping.h>
#include <linux/debugfs.h>
#ifdef CONFIG_IPQ_SUBSYSTEM_RAMDUMP
#include <soc/qcom/ramdump.h>
#endif
#include <soc/qcom/socinfo.h>
#include <soc/qcom/license_manager.h>
#include "qcom_common.h"
#include "qcom_q6v5.h"

#include "remoteproc_internal.h"

#define WCSS_CRASH_REASON		421
#define WCSS_SMEM_HOST			1

#define MPD_WCNSS_PAS_ID        0xD

#define BUF_SIZE 35
#define REMOTE_PID	1
#define Q6_BOOT_ARGS_SMEM_SIZE 4096

#define RPD_SWID		MPD_WCNSS_PAS_ID
#define UPD_SWID		0x12

#define UPD_BOOTARGS_HEADER_TYPE	0x2
#define LIC_BOOTARGS_HEADER_TYPE	0x3

static const struct wcss_data q6_ipq5332_res_init;
static int debug_wcss;
static struct dentry *heartbeat_hdl;

/**
 * enum state - state of a wcss (private)
 * @WCSS_NORMAL: subsystem is operating normally
 * @WCSS_CRASHED: subsystem has crashed and hasn't been shutdown
 * @WCSS_RESTARTING: subsystem has been shutdown and is now restarting
 * @WCSS_SHUTDOWN: subsystem has been shutdown
 *
 */
enum q6_wcss_state {
	WCSS_NORMAL,
	WCSS_CRASHED,
	WCSS_RESTARTING,
	WCSS_SHUTDOWN,
};

enum {
	Q6_IPQ,
	WCSS_AHB_IPQ,
	WCSS_PCIE_IPQ,
};

enum q6_version {
	Q6V5,
	Q6V6,
	Q6V7,
};

enum q6_bootargs_version {
	VERSION1 = 1,
	VERSION2,
};

struct q6_wcss {
	struct device *dev;
	u32 reset_cmd_id;

	struct qcom_rproc_glink glink_subdev;
	struct qcom_rproc_ssr ssr_subdev;

	struct qcom_q6v5 q6;

	phys_addr_t mem_phys;
	phys_addr_t mem_reloc;
	void *mem_region;
	size_t mem_size;

	int crash_reason_smem;
	u32 version;
	bool requires_force_stop;
	bool need_mem_protection;
	u8 pd_asid;
	enum q6_wcss_state state;
	bool is_fw_shared;
	int (*mdt_load_sec)(struct device *dev, const struct firmware *fw,
			const char *fw_name, int pas_id, void *mem_region,
			phys_addr_t mem_phys, size_t mem_size,
			phys_addr_t *reloc_base);
};

struct wcss_data {
	int (*init_irq)(struct qcom_q6v5 *q6, struct platform_device *pdev,
				struct rproc *rproc, int remote_id,
				int crash_reason,
				void (*handover)(struct qcom_q6v5 *q6));
	const char *q6_firmware_name;
	int crash_reason_smem;
	int remote_id;
	u32 version;
	u32 reset_cmd_id;

	const char *ssr_name;

	const struct rproc_ops *ops;
	bool requires_force_stop;
	bool need_mem_protection;
	bool need_auto_boot;
	bool glink_subdev_required;
	u8 pd_asid;
	bool reset_seq;
	enum q6_version q6ver;
	bool is_fw_shared;
	int (*mdt_load_sec)(struct device *dev, const struct firmware *fw,
			const char *fw_name, int pas_id, void *mem_region,
			phys_addr_t mem_phys, size_t mem_size,
			phys_addr_t *reloc_base);

	u32 pasid;
	u8 bootargs_version;
	bool clear_smp2p_last_value;
};

struct bootargs_smem_info {
	void *smem_base_ptr;
	void *smem_elem_cnt_ptr;
	void *smem_bootargs_ptr;
};

struct license_params {
	dma_addr_t dma_buf;
	void *buf;
	size_t size;
};

static struct license_params lic_param;

struct bootargs_header {
	u8 type;
	u8 length;
};

struct q6_userpd_bootargs {
	struct bootargs_header header;
	u8 pid;
	u32 bootaddr;
	u32 data_size;
} __packed;

struct license_bootargs {
	struct bootargs_header header;
	u8 license_type;
	u32 addr;
	u32 size;
} __packed;

#ifdef CONFIG_IPQ_SUBSYSTEM_RAMDUMP
static int qcom_get_pd_fw_info(struct q6_wcss *wcss, const struct firmware *fw,
				struct ramdump_segment *segs, int index,
				struct qcom_pd_fw_info *fw_info)
{
	int ret = get_pd_fw_info(wcss->dev, fw, wcss->mem_phys,
			wcss->mem_size, wcss->pd_asid, fw_info);

	if (ret) {
		dev_err(wcss->dev, "couldn't get PD firmware info : %d\n", ret);
		return ret;
	}

	segs[index].address = fw_info->paddr;
	segs[index].size = fw_info->size;
	segs[index].v_address = ioremap(segs[index].address,
			segs[index].size);
	segs[index].vaddr = fw_info->vaddr;
	return ret;
}

static void crashdump_init(struct rproc *rproc,
				struct rproc_dump_segment *segment,
				void *dest)
{
	void *handle;
	struct ramdump_segment *segs;
	int ret, index = 0;
	int num_segs;
	struct qcom_pd_fw_info fw_info = {0};
	struct q6_wcss *wcss = rproc->priv;
	struct device *dev = wcss->dev;
	struct device_node *node = NULL, *np = dev->of_node;
	const struct firmware *fw;
	char dev_name[BUF_SIZE];
	u32 temp;

	if (wcss->pd_asid)
		snprintf(dev_name, BUF_SIZE, "q6v5_wcss_userpd%d_mem",
							wcss->pd_asid);
	else
		snprintf(dev_name, BUF_SIZE, "q6mem");

	handle = create_ramdump_device(dev_name, &rproc->dev);
	if (!handle) {
		dev_err(&rproc->dev, "unable to create ramdump device"
						"for %s\n", rproc->name);
		return;
	}

	if (create_ramdump_device_file(handle)) {
		dev_err(&rproc->dev, "unable to create ramdump device"
						"for %s\n", rproc->name);
		goto free_device;
	}

	num_segs = of_count_phandle_with_args(np, "memory-region", NULL);
	if (num_segs <= 0) {
		if (!wcss->pd_asid) {
			dev_err(&rproc->dev, "Could not find memory regions to dump");
			goto free_device;
		}
		dev_info(&rproc->dev, "memory regions to dump not defined in DTS");
		dev_info(&rproc->dev, "taking dump of FW PIL segment data");
		num_segs = 0;
	}

	if (wcss->pd_asid)
		num_segs++;
	dev_dbg(&rproc->dev, "number of segments to be dumped: %d\n", num_segs);

	segs = kzalloc(num_segs * sizeof(struct ramdump_segment), GFP_KERNEL);
	if (!segs) {
		dev_err(&rproc->dev, "Could not allocate memory for ramdump segments");
		goto free_device;
	}

	while (index < num_segs) {
		node = of_parse_phandle(np, "memory-region", index);
		if (!node)
			break;

		ret = of_property_read_u32_index(node, "reg", 1, &temp);
		if (ret) {
			pr_err("Could not retrieve reg addr %d\n", ret);
			of_node_put(node);
			goto put_node;
		}
		segs[index].address = (u32)temp;

		ret = of_property_read_u32_index(node, "reg", 3, &temp);
		if (ret) {
			pr_err("Could not retrieve reg size %d\n", ret);
			of_node_put(node);
			goto put_node;
		}
		segs[index].size = (u32)temp;

		segs[index].v_address = ioremap(segs[index].address,
						segs[index].size);
		of_node_put(node);
		index++;
	}

	/* Get the PD firmware info */
	ret = request_firmware(&fw, rproc->firmware, dev);
	if (ret < 0) {
		dev_err(dev, "request_firmware failed: %d\n", ret);
		goto free_device;
	}

	if (wcss->pd_asid) {
		ret = qcom_get_pd_fw_info(wcss, fw, segs, index, &fw_info);
		if (ret)
			goto free_device;
		index++;
	}

	release_firmware(fw);
	do_elf_ramdump(handle, segs, index);

	for (index = 0; index < num_segs; index++) {
		if (segs[index].v_address)
			iounmap(segs[index].v_address);
	}
put_node:
	of_node_put(np);
free_device:
	destroy_ramdump_device(handle);
}
#else
static void crashdump_init(struct rproc *rproc,
				struct rproc_dump_segment *segment, void *dest)
{
}
#endif

static void q6_coredump(struct rproc *rproc,
			struct rproc_dump_segment *segment, void *dest)
{
	struct q6_wcss *wcss = rproc->priv;
	struct device_node *upd_np;

	/*
	 * Send ramdump notification to userpd(s) if rootpd
	 * crashed, irrespective of userpd status.
	 */
	for_each_available_child_of_node(wcss->dev->of_node, upd_np) {
		struct device_node *temp;
		struct platform_device *upd_pdev;
		struct rproc *upd_rproc;

		if (!strstr(upd_np->name, "pd"))
			continue;

		upd_pdev = of_find_device_by_node(upd_np);
		upd_rproc = platform_get_drvdata(upd_pdev);
		rproc_subsys_notify(upd_rproc,
				    SUBSYS_RAMDUMP_NOTIFICATION, false);

		for_each_available_child_of_node(upd_np, temp) {
			upd_pdev = of_find_device_by_node(temp);
			upd_rproc = platform_get_drvdata(upd_pdev);
			rproc_subsys_notify(upd_rproc,
					    SUBSYS_RAMDUMP_NOTIFICATION, false);
		}
	}

	wcss->state = WCSS_RESTARTING;

	crashdump_init(rproc, segment, dest);
}

static int handle_upd_in_rpd_crash(void *data)
{
	struct rproc *rpd_rproc = data, *upd_rproc;
	struct q6_wcss *rpd_wcss = rpd_rproc->priv;
	struct device_node *upd_np, *temp;
	struct platform_device *upd_pdev;
	const struct firmware *firmware_p;
	int ret;

	while (1) {
		if (rpd_rproc->state == RPROC_RUNNING)
			break;
		udelay(1);
	}

	for_each_available_child_of_node(rpd_wcss->dev->of_node, upd_np) {
		if (strstr(upd_np->name, "pd") == NULL)
			continue;
		upd_pdev = of_find_device_by_node(upd_np);
		upd_rproc = platform_get_drvdata(upd_pdev);

		mutex_lock(&upd_rproc->lock);
		if (upd_rproc->state != RPROC_SUSPENDED) {
			mutex_unlock(&upd_rproc->lock);
			continue;
		}

		/* load firmware */
		ret = request_firmware(&firmware_p, upd_rproc->firmware,
				&upd_pdev->dev);
		if (ret < 0) {
			dev_err(&upd_pdev->dev, "request_firmware failed: %d\n",
				ret);
			mutex_unlock(&upd_rproc->lock);
			continue;
		}

		/* start the userpd rproc*/
		ret = rproc_start(upd_rproc, firmware_p);
		if (ret)
			dev_err(&upd_pdev->dev, "failed to start %s\n",
					upd_rproc->name);
		release_firmware(firmware_p);
		mutex_unlock(&upd_rproc->lock);

		for_each_available_child_of_node(upd_np, temp) {
			upd_pdev = of_find_device_by_node(temp);
			upd_rproc = platform_get_drvdata(upd_pdev);

			mutex_lock(&upd_rproc->lock);
			if (upd_rproc->state != RPROC_SUSPENDED) {
				mutex_unlock(&upd_rproc->lock);
				continue;
			}

			/* load firmware */
			ret = request_firmware(&firmware_p, upd_rproc->firmware,
					&upd_pdev->dev);
			if (ret < 0) {
				dev_err(&upd_pdev->dev, "request_firmware failed: %d\n",
					ret);
				mutex_unlock(&upd_rproc->lock);
				continue;
			}

			/* start the userpd rproc*/
			ret = rproc_start(upd_rproc, firmware_p);
			if (ret)
				dev_err(&upd_pdev->dev, "failed to start %s\n",
						upd_rproc->name);
			release_firmware(firmware_p);
			mutex_unlock(&upd_rproc->lock);
		}
	}
	rpd_wcss->state = WCSS_NORMAL;
	return 0;
}

static int q6_wcss_start(struct rproc *rproc)
{
	struct q6_wcss *wcss = rproc->priv;
	int ret;
	struct device_node *upd_np, *temp;
	struct platform_device *upd_pdev;
	struct rproc *upd_rproc;
	struct q6_wcss *upd_wcss;
	const struct wcss_data *desc;

	desc = of_device_get_match_data(wcss->dev);
	if (!desc)
		return -EINVAL;

	qcom_q6v5_prepare(&wcss->q6);

	if (desc->clear_smp2p_last_value)
		qcom_clear_smp2p_last_value();

	if (wcss->need_mem_protection) {
		ret = qcom_scm_pas_auth_and_reset(desc->pasid, debug_wcss,
					wcss->reset_cmd_id);
		if (ret) {
			dev_err(wcss->dev, "wcss_reset failed\n");
			return ret;
		}
		goto wait_for_reset;
	}

wait_for_reset:
	ret = qcom_q6v5_wait_for_start(&wcss->q6, msecs_to_jiffies(10000));
	if (ret) {
		if (debug_wcss && ret == -ETIMEDOUT)
			goto wait_for_reset;
		else
			dev_err(wcss->dev, "start failed ret: %d\n", ret);
	}

	/* start userpd's, if root pd getting recovered*/
	if (wcss->state == WCSS_RESTARTING) {
		char thread_name[32];

		snprintf(thread_name, sizeof(thread_name), "rootpd_crash");
		kthread_run(handle_upd_in_rpd_crash, rproc, thread_name);
	} else {
		/* Bring userpd wcss state to default value */
		for_each_available_child_of_node(wcss->dev->of_node, upd_np) {
			if (strstr(upd_np->name, "pd") == NULL)
				continue;
			upd_pdev = of_find_device_by_node(upd_np);
			upd_rproc = platform_get_drvdata(upd_pdev);
			upd_wcss = upd_rproc->priv;
			upd_wcss->state = WCSS_NORMAL;

			for_each_available_child_of_node(upd_np, temp) {
				upd_pdev = of_find_device_by_node(temp);
				upd_rproc = platform_get_drvdata(upd_pdev);
				upd_wcss = upd_rproc->priv;
				upd_wcss->state = WCSS_NORMAL;
			}
		}
	}

	if (lic_param.buf) {
		lm_free_license(lic_param.buf, lic_param.dma_buf, lic_param.size);
		lic_param.buf = NULL;
	}

	return ret;

	if (lic_param.buf) {
		lm_free_license(lic_param.buf, lic_param.dma_buf, lic_param.size);
		lic_param.buf = NULL;
	}

	return ret;
}

static int q6_wcss_spawn_pd(struct rproc *rproc)
{
	int ret;
	struct q6_wcss *wcss = rproc->priv;
	struct qcom_q6v5 *q6v5 = &wcss->q6;

	reinit_completion(&q6v5->start_done);
	reinit_completion(&q6v5->stop_done);
	reinit_completion(&q6v5->spawn_done);

	ret = qcom_q6v5_request_spawn(&wcss->q6);
	if (ret) {
		dev_err(wcss->dev, "Spawn failed, ret = %d\n", ret);
		return ret;
	}

	ret = qcom_q6v5_wait_for_start(&wcss->q6, msecs_to_jiffies(10000));
	if (ret) {
		dev_err(wcss->dev, "Start failed, ret = %d\n", ret);
		wcss->q6.running = false;
		return ret;
	}
	wcss->q6.running = true;
	return ret;
}

static int wcss_ipq5332_text_pd_start(struct rproc *rproc)
{
	struct q6_wcss *wcss = rproc->priv;
	int ret = 0;
	const struct wcss_data *desc;
	u8 pd_asid;
	u32 pasid;

	desc = of_device_get_match_data(wcss->dev);
	if (!desc)
		return -EINVAL;

	if (wcss->need_mem_protection) {
		pd_asid = qcom_get_pd_asid(wcss->dev->of_node);
		pasid = (pd_asid << 8) | UPD_SWID;
		ret = qcom_scm_pas_auth_and_reset(pasid, 0x0, 0x0);
		if (ret) {
			dev_err(wcss->dev, "failed to power up ahb pd\n");
			return ret;
		}
	}

	wcss->state = WCSS_NORMAL;

	return ret;
}

static int wcss_ipq5332_ahb_pd_start(struct rproc *rproc)
{
	struct q6_wcss *wcss = rproc->priv;
	int ret = 0;
	const struct wcss_data *desc;
	u8 pd_asid;
	u32 pasid;

	desc = of_device_get_match_data(wcss->dev);
	if (!desc)
		return -EINVAL;

	if (wcss->need_mem_protection) {
		pd_asid = qcom_get_pd_asid(wcss->dev->of_node);
		pasid = (pd_asid << 8) | UPD_SWID;
		ret = qcom_scm_pas_auth_and_reset(pasid, 0x0, 0x0);
		if (ret) {
			dev_err(wcss->dev, "failed to power up ahb pd\n");
			return ret;
		}
	}

	if (wcss->q6.spawn_bit) {
		ret = q6_wcss_spawn_pd(rproc);
		if (ret)
			return ret;
	}

	wcss->state = WCSS_NORMAL;

	return ret;
}

static int wcss_ahb_pd_start(struct rproc *rproc)
{
	struct q6_wcss *wcss = rproc->priv;
	int ret = 0;
	const struct wcss_data *desc;

	desc = of_device_get_match_data(wcss->dev);
	if (!desc)
		return -EINVAL;

	if (!desc->reset_seq)
		goto spawn_pd;

	if (wcss->need_mem_protection) {
		ret = qti_scm_int_radio_powerup(desc->pasid);
		if (ret) {
			dev_err(wcss->dev, "failed to power up ahb pd\n");
			return ret;
		}
	}

spawn_pd:
	if (wcss->q6.spawn_bit) {
		ret = q6_wcss_spawn_pd(rproc);
		if (ret)
			return ret;
	}

	wcss->state = WCSS_NORMAL;

	return ret;
}

static int wcss_ipq5332_pcie_pd_start(struct rproc *rproc)
{
	int ret;
	struct q6_wcss *wcss = rproc->priv;
	u8 pd_asid;
	u32 pasid;

	pd_asid = qcom_get_pd_asid(wcss->dev->of_node);
	if (wcss->need_mem_protection) {
		pasid = (pd_asid << 8) | UPD_SWID,
		ret = qcom_scm_pas_auth_and_reset(pasid, 0x0, 0x0);
		if (ret) {
			dev_err(wcss->dev, "failed to power up pcie pd\n");
			return ret;
		}
	}

	ret = q6_wcss_spawn_pd(rproc);
	if (!ret)
		wcss->state = WCSS_NORMAL;

	return ret;
}

static int wcss_pcie_pd_start(struct rproc *rproc)
{
	int ret = q6_wcss_spawn_pd(rproc);

	if (!ret) {
		struct q6_wcss *wcss = rproc->priv;

		wcss->state = WCSS_NORMAL;
	}
	return ret;
}

static int q6_wcss_stop(struct rproc *rproc)
{
	struct q6_wcss *wcss = rproc->priv;
	int ret;

	/* stop userpd's, if root pd getting crashed*/
	if (rproc->state == RPROC_CRASHED) {
		struct device_node *upd_np, *temp;
		struct platform_device *upd_pdev;
		struct rproc *upd_rproc;
		struct q6_wcss *upd_wcss;

		/*
		 * Send fatal notification to userpd(s) if rootpd
		 * crashed, irrespective of userpd status.
		 */
		for_each_available_child_of_node(wcss->dev->of_node, upd_np) {
			if (strstr(upd_np->name, "pd") == NULL)
				continue;
			upd_pdev = of_find_device_by_node(upd_np);
			upd_rproc = platform_get_drvdata(upd_pdev);
			upd_wcss = upd_rproc->priv;

			/*Skip completion for textpd since it has no IRQ*/
			if (strstr(upd_np->name, "text") == NULL) {
				complete(&upd_wcss->q6.spawn_done);
				complete(&upd_wcss->q6.start_done);
				complete(&upd_wcss->q6.stop_done);
			}

			rproc_subsys_notify(upd_rproc,
				SUBSYS_PREPARE_FOR_FATAL_SHUTDOWN, true);

			for_each_available_child_of_node(upd_np, temp) {
				upd_pdev = of_find_device_by_node(temp);
				upd_rproc = platform_get_drvdata(upd_pdev);
				upd_wcss = upd_rproc->priv;
				complete(&upd_wcss->q6.spawn_done);
				complete(&upd_wcss->q6.start_done);
				complete(&upd_wcss->q6.stop_done);

				rproc_subsys_notify(upd_rproc,
					SUBSYS_PREPARE_FOR_FATAL_SHUTDOWN, true);
			}
		}

		for_each_available_child_of_node(wcss->dev->of_node, upd_np) {
			if (strstr(upd_np->name, "pd") == NULL)
				continue;
			upd_pdev = of_find_device_by_node(upd_np);
			upd_rproc = platform_get_drvdata(upd_pdev);
			upd_wcss = upd_rproc->priv;

			mutex_lock(&upd_rproc->lock);
			if (upd_rproc->state == RPROC_OFFLINE) {
				mutex_unlock(&upd_rproc->lock);
				continue;
			}

			upd_rproc->state = RPROC_CRASHED;

			/* stop the userpd parent rproc*/
			ret = rproc_stop(upd_rproc, true);
			if (ret)
				dev_err(&upd_pdev->dev, "failed to stop %s\n",
							upd_rproc->name);
			upd_rproc->state = RPROC_SUSPENDED;
			mutex_unlock(&upd_rproc->lock);

			for_each_available_child_of_node(upd_np, temp) {
				upd_pdev = of_find_device_by_node(temp);
				upd_rproc = platform_get_drvdata(upd_pdev);
				upd_wcss = upd_rproc->priv;

				mutex_lock(&upd_rproc->lock);
				if (upd_rproc->state == RPROC_OFFLINE) {
					mutex_unlock(&upd_rproc->lock);
					continue;
				}

				upd_rproc->state = RPROC_CRASHED;

				/* stop the userpd child rproc*/
				ret = rproc_stop(upd_rproc, true);
				if (ret)
					dev_err(&upd_pdev->dev, "failed to stop %s\n",
							upd_rproc->name);

				upd_rproc->state = RPROC_SUSPENDED;
				mutex_unlock(&upd_rproc->lock);
			}
		}
	}

	if (wcss->need_mem_protection) {
		const struct wcss_data *desc =
					of_device_get_match_data(wcss->dev);

		if (!desc)
			return -EINVAL;

		ret = qcom_scm_pas_shutdown(desc->pasid);
		if (ret) {
			dev_err(wcss->dev, "not able to shutdown\n");
			return ret;
		}
		goto pas_done;
	}

	if (wcss->requires_force_stop) {
		ret = qcom_q6v5_request_stop(&wcss->q6);
		if (ret) {
			dev_err(wcss->dev, "Stop failed, ret: %d\n", ret);
			return ret;
		}
	}

pas_done:
	debugfs_remove(heartbeat_hdl);
	qcom_q6v5_unprepare(&wcss->q6);

	return 0;
}

static int wcss_ipq5332_pcie_pd_stop(struct rproc *rproc)
{
	struct q6_wcss *wcss = rproc->priv;
	int ret;
	struct rproc *rpd_rproc = dev_get_drvdata(wcss->dev->parent);
	u32 pasid;
	u8 pd_asid = qcom_get_pd_asid(wcss->dev->of_node);

	if (rproc->state != RPROC_CRASHED) {
		ret = qcom_q6v5_request_stop(&wcss->q6);
		if (ret) {
			dev_err(&rproc->dev, "ahb pd not stopped, ret: %d\n", ret);
			return ret;
		}
	}

	if (wcss->need_mem_protection) {
		pasid = (pd_asid << 8) | UPD_SWID,
		ret = qcom_scm_pas_shutdown(pasid);
		if (ret) {
			dev_err(wcss->dev, "failed to power down ahb pd\n");
			return ret;
		}
	}

	/*Shut down rootpd, if userpd not crashed*/
	if (rproc->state != RPROC_CRASHED)
		rproc_shutdown(rpd_rproc);

	wcss->state = WCSS_SHUTDOWN;
	return 0;
}

static int wcss_pcie_pd_stop(struct rproc *rproc)
{
	struct q6_wcss *wcss = rproc->priv;
	int ret = 0;
	struct rproc *rpd_rproc = dev_get_drvdata(wcss->dev->parent);

	if (rproc->state != RPROC_CRASHED) {
		ret = qcom_q6v5_request_stop(&wcss->q6);
		if (ret) {
			dev_err(&rproc->dev, "ahb pd not stopped, ret: %d\n", ret);
			return ret;
		}
	}

	/*Shut down rootpd, if userpd not crashed*/
	if (rproc->state != RPROC_CRASHED)
		rproc_shutdown(rpd_rproc);

	wcss->state = WCSS_SHUTDOWN;
	return ret;
}

static int wcss_ipq5332_text_pd_stop(struct rproc *rproc)
{
	struct q6_wcss *wcss = rproc->priv;
	int ret = 0;
	struct rproc *rpd_rproc = dev_get_drvdata(wcss->dev->parent);
	const struct wcss_data *desc;
	u8 pd_asid;
	u32 pasid;

	desc = of_device_get_match_data(wcss->dev);
	if (!desc)
		return -EINVAL;

	if (wcss->need_mem_protection) {
		pd_asid = qcom_get_pd_asid(wcss->dev->of_node);
		pasid = (pd_asid << 8) | UPD_SWID;

		ret = qcom_scm_pas_shutdown(pasid);
		if (ret) {
			dev_err(wcss->dev, "failed to power down ahb pd\n");
			return ret;
		}
	}

	if (rproc->state != RPROC_CRASHED)
		rproc_shutdown(rpd_rproc);
	wcss->state = WCSS_SHUTDOWN;
	return ret;
}

static int wcss_ipq5332_ahb_pd_stop(struct rproc *rproc)
{
	struct q6_wcss *wcss = rproc->priv, *rpd_wcss;
	int ret = 0;
	struct rproc *rpd_rproc = dev_get_drvdata(wcss->dev->parent);
	const struct wcss_data *desc;
	bool q6_offload;
	u8 pd_asid;
	u32 pasid;

	rpd_wcss = rpd_rproc->priv;
	desc = of_device_get_match_data(wcss->dev);
	if (!desc)
		return -EINVAL;

	q6_offload = of_property_read_bool(wcss->dev->of_node,
						"qcom,offloaded_to_q6");
	if (rproc->state != RPROC_CRASHED && wcss->q6.stop_bit && q6_offload) {
		ret = qcom_q6v5_request_stop(&wcss->q6);
		if (ret) {
			dev_err(&rproc->dev, "ahb pd not stopped, ret: %d\n", ret);
			return ret;
		}
	}

	if (wcss->need_mem_protection) {
		pd_asid = qcom_get_pd_asid(wcss->dev->of_node);
		pasid = (pd_asid << 8) | UPD_SWID;
		ret = qcom_scm_pas_shutdown(pasid);
		if (ret) {
			dev_err(wcss->dev, "failed to power down ahb pd\n");
			return ret;
		}
	}

	if (rproc->state != RPROC_CRASHED)
		rproc_shutdown(rpd_rproc);
	wcss->state = WCSS_SHUTDOWN;
	return ret;
}

static int wcss_ahb_pd_stop(struct rproc *rproc)
{
	struct q6_wcss *wcss = rproc->priv;
	int ret = 0;
	struct rproc *rpd_rproc = dev_get_drvdata(wcss->dev->parent);
	const struct wcss_data *desc;

	desc = of_device_get_match_data(wcss->dev);
	if (!desc)
		return -EINVAL;

	if (!desc->reset_seq)
		goto shut_dn_rpd;

	if (rproc->state != RPROC_CRASHED && wcss->q6.stop_bit) {
		ret = qcom_q6v5_request_stop(&wcss->q6);
		if (ret) {
			dev_err(&rproc->dev, "ahb pd not stopped, ret: %d\n", ret);
			return ret;
		}
	}

	if (wcss->need_mem_protection) {
		ret = qti_scm_int_radio_powerdown(desc->pasid);
		if (ret) {
			dev_err(wcss->dev, "failed to power down ahb pd\n");
			return ret;
		}
	}

shut_dn_rpd:
	if (rproc->state != RPROC_CRASHED)
		rproc_shutdown(rpd_rproc);
	wcss->state = WCSS_SHUTDOWN;
	return ret;
}

static void *q6_wcss_da_to_va(struct rproc *rproc, u64 da, int len)
{
	struct q6_wcss *wcss = rproc->priv;
	int offset;

	offset = da - wcss->mem_reloc;
	if (offset < 0 || offset + len > wcss->mem_size)
		return NULL;

	return wcss->mem_region + offset;
}

static void load_license_params_to_bootargs(struct device *dev,
					struct bootargs_smem_info *boot_args)
{
	u16 cnt;
	u32 rd_val;
	struct license_bootargs lic_bootargs = {0x0};

	lic_param.buf = lm_get_license(INTERNAL, &lic_param.dma_buf, &lic_param.size, 0);
	if (!lic_param.buf) {
		dev_info(dev, "No license file passed in bootargs\n");
		return;
	}

	/* No of elements */
	cnt = *((u16 *)boot_args->smem_elem_cnt_ptr);
	cnt += sizeof(struct license_bootargs);
	memcpy_toio(boot_args->smem_elem_cnt_ptr, &cnt, sizeof(u16));

	/* TYPE */
	lic_bootargs.header.type = LIC_BOOTARGS_HEADER_TYPE;

	/* LENGTH */
	lic_bootargs.header.length =
			sizeof(lic_bootargs) - sizeof(lic_bootargs.header);

	/* license type */
	if (!of_property_read_u32(dev->of_node, "license-type", &rd_val))
		lic_bootargs.license_type = (u8)rd_val;

	/* ADDRESS */
	lic_bootargs.addr = (u32)lic_param.dma_buf;

	/* License file size */
	lic_bootargs.size = lic_param.size;
	memcpy_toio(boot_args->smem_bootargs_ptr,
					&lic_bootargs, sizeof(lic_bootargs));
	boot_args->smem_bootargs_ptr += sizeof(lic_bootargs);

	dev_info(dev, "License file copied in bootargs\n");
	return;
}

static int copy_userpd_bootargs(struct bootargs_smem_info *boot_args,
				struct rproc *upd_rproc)
{
	struct q6_wcss *upd_wcss = upd_rproc->priv;
	int ret;
	const struct firmware *fw;
	struct q6_userpd_bootargs upd_bootargs = {0};

	/* TYPE */
	upd_bootargs.header.type = UPD_BOOTARGS_HEADER_TYPE;

	/* LENGTH */
	upd_bootargs.header.length =
		sizeof(struct q6_userpd_bootargs) - sizeof(upd_bootargs.header);

	/* PID */
	upd_bootargs.pid = qcom_get_pd_asid(upd_wcss->dev->of_node) + 1;

	ret = request_firmware(&fw, upd_rproc->firmware,
			upd_wcss->dev);
	if (ret < 0) {
		dev_err(upd_wcss->dev, "request_firmware failed: %d\n",
				ret);
		return ret;
	}

	/* Load address */
	upd_bootargs.bootaddr = rproc_get_boot_addr(upd_rproc, fw);

	/* PIL data size */
	upd_bootargs.data_size = qcom_mdt_get_file_size(fw);

	release_firmware(fw);

	/* copy into smem bootargs array*/
	memcpy_toio(boot_args->smem_bootargs_ptr,
			&upd_bootargs, sizeof(struct q6_userpd_bootargs));
	boot_args->smem_bootargs_ptr += sizeof(struct q6_userpd_bootargs);
	return ret;
}

static int load_userpd_params_to_bootargs(struct device *dev,
				struct bootargs_smem_info *boot_args)
{
	int ret = 0;
	struct device_node *upd_np, *temp;
	struct platform_device *upd_pdev;
	struct rproc *upd_rproc;
	u16 cnt;
	u8 upd_cnt = 0;

	if (!of_property_read_bool(dev->of_node, "qcom,userpd-bootargs"))
		return -EINVAL;

	for_each_available_child_of_node(dev->of_node, upd_np) {
		if (strstr(upd_np->name, "pd") == NULL)
			continue;
		upd_cnt++;
		for_each_available_child_of_node(upd_np, temp)
			upd_cnt++;
	}

	/* No of elements */
	cnt = *((u16 *)boot_args->smem_elem_cnt_ptr);
	cnt += (sizeof(struct q6_userpd_bootargs) * upd_cnt);
	memcpy_toio(boot_args->smem_elem_cnt_ptr, &cnt, sizeof(u16));

	for_each_available_child_of_node(dev->of_node, upd_np) {
		if (strstr(upd_np->name, "pd") == NULL)
			continue;
		upd_pdev = of_find_device_by_node(upd_np);
		upd_rproc = platform_get_drvdata(upd_pdev);
		ret = copy_userpd_bootargs(boot_args, upd_rproc);
		if (ret)
			return ret;

		for_each_available_child_of_node(upd_np, temp) {
			upd_pdev = of_find_device_by_node(temp);
			upd_rproc = platform_get_drvdata(upd_pdev);
			ret = copy_userpd_bootargs(boot_args, upd_rproc);
			if (ret)
				return ret;
		}
	}
	return ret;
}

static ssize_t show_smem_addr(struct file *file, char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	void *smem_pa = file->private_data;
	char _buf[16] = {0};

	snprintf(_buf, sizeof(_buf), "0x%lX\n", (uintptr_t)smem_pa);
	return simple_read_from_buffer(user_buf, count, ppos, _buf,
				       strnlen(_buf, 16));
}

static const struct file_operations heartbeat_smem_ops = {
	.open = simple_open,
	.read = show_smem_addr,
};

static int create_heartbeat_smem(struct device *dev)
{
	u32 smem_id;
	void *ptr;
	size_t size;
	int ret;
	const char *key = "qcom,heartbeat_smem";

	ret = of_property_read_u32(dev->of_node, key, &smem_id);
	if (ret) {
		dev_err(dev, "failed to get heartbeat smem id\n");
		return ret;
	}

	ret = qcom_smem_alloc(REMOTE_PID, smem_id,
			      Q6_BOOT_ARGS_SMEM_SIZE);
	if (ret && ret != -EEXIST) {
		dev_err(dev, "failed to allocate heartbeat smem segment\n");
		return ret;
	}

	ptr = qcom_smem_get(REMOTE_PID, smem_id, &size);
	if (IS_ERR(ptr)) {
		dev_err(dev,
			"Unable to acquire smem item(%d) ret:%ld\n",
			smem_id, PTR_ERR(ptr));
		return PTR_ERR(ptr);
	}

	/* Create sysfs entry to expose smem PA */
	heartbeat_hdl = debugfs_create_file("heartbeat_address",
					    0400, NULL,
					    (void *)qcom_smem_virt_to_phys(ptr),
					    &heartbeat_smem_ops);
	if (IS_ERR_OR_NULL(heartbeat_hdl)) {
		ret = PTR_ERR(heartbeat_hdl);
		dev_err(dev,
			"Unable to create heartbeat sysfs entry ret:%ld\n",
			PTR_ERR(ptr));
	}
	return ret;
}

static int share_bootargs_to_q6(struct device *dev)
{
	int ret;
	u32 smem_id, rd_val;
	const char *key = "qcom,bootargs_smem";
	size_t size;
	u16 cnt, tmp, version;
	void *ptr;
	u8 *bootargs_arr;
	struct device_node *np = dev->of_node;
	struct bootargs_smem_info boot_args;
	const struct wcss_data *desc =
				of_device_get_match_data(dev);

	if (!desc)
		return -EINVAL;

	ret = of_property_read_u32(np, key, &smem_id);
	if (ret) {
		pr_err("failed to get smem id\n");
		return ret;
	}

	ret = qcom_smem_alloc(REMOTE_PID, smem_id,
					Q6_BOOT_ARGS_SMEM_SIZE);
	if (ret && ret != -EEXIST) {
		pr_err("failed to allocate q6 bootargs smem segment\n");
		return ret;
	}

	boot_args.smem_base_ptr = qcom_smem_get(REMOTE_PID, smem_id, &size);
	if (IS_ERR(boot_args.smem_base_ptr)) {
		pr_err("Unable to acquire smp2p item(%d) ret:%ld\n",
				smem_id, PTR_ERR(boot_args.smem_base_ptr));
		return PTR_ERR(boot_args.smem_base_ptr);
	}
	ptr = boot_args.smem_base_ptr;

	/*get physical address*/
	pr_info("smem phyiscal address:0x%lX\n",
				(uintptr_t)qcom_smem_virt_to_phys(ptr));

	/*Version*/
	version = desc->bootargs_version;
	if (!of_property_read_u32(dev->of_node, "qcom,bootargs_version",
								&rd_val))
		version = (u16)rd_val;
	memcpy_toio(ptr, &version, sizeof(version));
	ptr += sizeof(version);
	boot_args.smem_elem_cnt_ptr = ptr;

	cnt = ret = of_property_count_u32_elems(np, "boot-args");
	if (ret < 0) {
		if (ret == -ENODATA) {
			pr_err("failed to read boot args ret:%d\n", ret);
			return ret;
		}
		cnt = 0;
	}

	/* No of elements */
	memcpy_toio(ptr, &cnt, sizeof(u16));
	ptr += sizeof(u16);

	bootargs_arr = kzalloc(cnt, GFP_KERNEL);
	if (!bootargs_arr) {
		pr_err("failed to allocate memory\n");
		return PTR_ERR(bootargs_arr);
	}

	for (tmp = 0; tmp < cnt; tmp++) {
		ret = of_property_read_u32_index(np, "boot-args", tmp, &rd_val);
		if (ret) {
			pr_err("failed to read boot args\n");
			kfree(bootargs_arr);
			return ret;
		}
		bootargs_arr[tmp] = (u8)rd_val;
	}

	/* Copy bootargs */
	memcpy_toio(ptr, bootargs_arr, cnt);
	ptr += (cnt);
	boot_args.smem_bootargs_ptr = ptr;

	of_node_put(np);
	kfree(bootargs_arr);

	ret = load_userpd_params_to_bootargs(dev, &boot_args);
	if (ret < 0) {
		pr_err("failed to read userpd boot args ret:%d\n", ret);
		return ret;
	}

	ret = create_heartbeat_smem(dev);
	if (ret && ret != -EEXIST) {
		pr_err("failed to create heartbeat smem ret:0x%X\n", ret);
		return ret;
	}

	load_license_params_to_bootargs(dev, &boot_args);

	return 0;
}

static int load_m3_firmware(struct device_node *np, struct q6_wcss *wcss)
{
	int ret;
	const struct firmware *m3_fw;
	const char *m3_fw_name;

	ret = of_property_read_string(np, "m3_firmware", &m3_fw_name);
	if (ret == -EINVAL)
		ret = of_property_read_string(np, "iu_firmware", &m3_fw_name);

	if (ret)
		return 0;

	ret = request_firmware(&m3_fw, m3_fw_name, wcss->dev);
	if (ret)
		return 0;

	ret = qcom_mdt_load_no_init(wcss->dev, m3_fw,
				m3_fw_name, 0,
				wcss->mem_region, wcss->mem_phys,
				wcss->mem_size, &wcss->mem_reloc);
	release_firmware(m3_fw);

	if (ret) {
		dev_err(wcss->dev,
				"can't load %s ret:%d\n", m3_fw_name, ret);
		return ret;
	}

	dev_info(wcss->dev, "m3 firmware %s loaded to DDR\n", m3_fw_name);
	return ret;
}

static int q6_wcss_load(struct rproc *rproc, const struct firmware *fw)
{
	struct q6_wcss *wcss = rproc->priv;
	const struct firmware *m3_fw;
	int ret;
	struct device *dev = wcss->dev;
	const char *m3_fw_name;
	struct device_node *upd_np, *temp;
	struct platform_device *upd_pdev;

	/* Share boot args to Q6 remote processor */
	ret = share_bootargs_to_q6(wcss->dev);
	if (ret && ret != -EINVAL) {
		dev_err(wcss->dev,
				"boot args sharing with q6 failed %d\n",
				ret);
		return ret;
	}

	/* load m3 firmware of userpd's */
	for_each_available_child_of_node(wcss->dev->of_node, upd_np) {
		if (strstr(upd_np->name, "pd") == NULL)
			continue;
		upd_pdev = of_find_device_by_node(upd_np);
		ret = load_m3_firmware(upd_np, wcss);
		if (ret)
			return ret;

		for_each_available_child_of_node(upd_np, temp) {
			upd_pdev = of_find_device_by_node(temp);
			ret = load_m3_firmware(temp, wcss);
			if (ret)
				return ret;
		}
	}

	ret = of_property_read_string(dev->of_node, "m3_firmware", &m3_fw_name);
	if (!ret && m3_fw_name) {
		ret = request_firmware(&m3_fw, m3_fw_name,
				       wcss->dev);
		if (ret)
			goto skip_m3;

		ret = qcom_mdt_load_no_init(wcss->dev, m3_fw,
					    m3_fw_name, 0,
					    wcss->mem_region, wcss->mem_phys,
					    wcss->mem_size, &wcss->mem_reloc);

		release_firmware(m3_fw);

		if (ret) {
			dev_err(wcss->dev, "can't load m3_fw.bXX ret:%d\n",
									ret);
			return ret;
		}
	}

skip_m3:
	if (wcss->need_mem_protection) {
		const struct wcss_data *desc =
					of_device_get_match_data(wcss->dev);

		if (!desc)
			return -EINVAL;

		return qcom_mdt_load(wcss->dev, fw, rproc->firmware,
				     desc->pasid, wcss->mem_region,
				     wcss->mem_phys, wcss->mem_size,
				     &wcss->mem_reloc);
	}
	return qcom_mdt_load_no_init(wcss->dev, fw, rproc->firmware,
				     0, wcss->mem_region, wcss->mem_phys,
				     wcss->mem_size, &wcss->mem_reloc);
}

static int wcss_ahb_pcie_pd_load(struct rproc *rproc, const struct firmware *fw)
{
	struct q6_wcss *wcss = rproc->priv, *wcss_rpd;
	struct rproc *rpd_rproc = dev_get_drvdata(wcss->dev->parent);
	u8 pd_asid;
	u32 pasid;
	int ret;

	wcss_rpd = rpd_rproc->priv;

	/* Simply Return in case of root pd recovery and fw shared*/
	if (wcss_rpd->state == WCSS_RESTARTING && wcss->is_fw_shared)
		return 0;

	/* Don't boot rootpd rproc incase user/root pd recovering after crash */
	if (wcss->state != WCSS_RESTARTING &&
			wcss_rpd->state != WCSS_RESTARTING) {
		/* Boot rootpd rproc*/
		ret = rproc_boot(rpd_rproc);
		if (ret || (wcss->state == WCSS_NORMAL && wcss->is_fw_shared))
			return ret;
	}

	if (wcss->need_mem_protection) {
		const struct wcss_data *desc =
				of_device_get_match_data(wcss->dev);

		if (!desc)
			return -EINVAL;

		pasid = desc->pasid;
		if (!pasid) {
			/* Dynamically compute pasid */
			pd_asid = qcom_get_pd_asid(wcss->dev->of_node);
			pasid = (pd_asid << 8) | UPD_SWID;
		}

		return wcss->mdt_load_sec(wcss->dev, fw, rproc->firmware,
				     pasid, wcss->mem_region,
				     wcss->mem_phys, wcss->mem_size,
				     &wcss->mem_reloc);
	}

	return 0;
}

int q6_wcss_register_dump_segments(struct rproc *rproc,
					const struct firmware *fw)
{
	/*
	 * Registering custom coredump function with a dummy dump segment
	 * as the dump regions are taken care by the dump function itself
	 */
	return rproc_coredump_add_custom_segment(rproc, 0, 0, q6_coredump,
									NULL);
}

static void q6_wcss_panic(struct rproc *rproc)
{
	struct q6_wcss *wcss = rproc->priv;

	qcom_q6v5_panic_handler(&wcss->q6);
}

static const struct rproc_ops wcss_pcie_ipq5018_ops = {
	.start = wcss_pcie_pd_start,
	.stop = wcss_pcie_pd_stop,
	.load = wcss_ahb_pcie_pd_load,
	.parse_fw = q6_wcss_register_dump_segments,
};

static const struct rproc_ops wcss_ahb_ipq5018_ops = {
	.start = wcss_ahb_pd_start,
	.stop = wcss_ahb_pd_stop,
	.load = wcss_ahb_pcie_pd_load,
	.parse_fw = q6_wcss_register_dump_segments,
};

static const struct rproc_ops q6_wcss_ipq5018_ops = {
	.start = q6_wcss_start,
	.stop = q6_wcss_stop,
	.da_to_va = q6_wcss_da_to_va,
	.load = q6_wcss_load,
	.get_boot_addr = rproc_elf_get_boot_addr,
	.parse_fw = q6_wcss_register_dump_segments,
	.report_panic = q6_wcss_panic,
};

static const struct rproc_ops wcss_text_ipq5332_ops = {
	.start = wcss_ipq5332_text_pd_start,
	.stop = wcss_ipq5332_text_pd_stop,
	.load = wcss_ahb_pcie_pd_load,
	.get_boot_addr = rproc_elf_get_boot_addr,
};

static const struct rproc_ops wcss_ahb_ipq5332_ops = {
	.start = wcss_ipq5332_ahb_pd_start,
	.stop = wcss_ipq5332_ahb_pd_stop,
	.load = wcss_ahb_pcie_pd_load,
	.get_boot_addr = rproc_elf_get_boot_addr,
	.parse_fw = q6_wcss_register_dump_segments,
};

static const struct rproc_ops wcss_pcie_ipq5332_ops = {
	.start = wcss_ipq5332_pcie_pd_start,
	.stop = wcss_ipq5332_pcie_pd_stop,
	.load = wcss_ahb_pcie_pd_load,
	.get_boot_addr = rproc_elf_get_boot_addr,
	.parse_fw = q6_wcss_register_dump_segments,
};

static int q6_alloc_memory_region(struct q6_wcss *wcss)
{
	struct reserved_mem *rmem = NULL;
	struct device_node *node;
	struct device *dev = wcss->dev;

	if (wcss->version == Q6_IPQ) {
		node = of_parse_phandle(dev->of_node, "memory-region", 0);
		if (node)
			rmem = of_reserved_mem_lookup(node);

		of_node_put(node);

		if (!rmem) {
			dev_err(dev, "unable to acquire memory-region\n");
			return -EINVAL;
		}
	} else {
		struct rproc *rpd_rproc = dev_get_drvdata(dev->parent);
		struct q6_wcss *rpd_wcss = rpd_rproc->priv;

		wcss->mem_phys = rpd_wcss->mem_phys;
		wcss->mem_reloc = rpd_wcss->mem_reloc;
		wcss->mem_size = rpd_wcss->mem_size;
		wcss->mem_region = rpd_wcss->mem_region;
		return 0;
	}

	wcss->mem_phys = rmem->base;
	wcss->mem_reloc = rmem->base;
	wcss->mem_size = rmem->size;
	wcss->mem_region = devm_ioremap_wc(dev, wcss->mem_phys, wcss->mem_size);
	if (!wcss->mem_region) {
		dev_err(dev, "unable to map memory region: %pa+%pa\n",
			&rmem->base, &rmem->size);
		return -EBUSY;
	}

	return 0;
}

static int q6_get_inbound_irq(struct qcom_q6v5 *q6,
			struct platform_device *pdev, const char *int_name,
			irqreturn_t (*handler)(int irq, void *data))
{
	int ret, irq;
	char *interrupt, *tmp = (char *)int_name;
	struct q6_wcss *wcss = q6->rproc->priv;

	irq = ret = platform_get_irq_byname(pdev, int_name);
	if (ret < 0) {
		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev,
				"failed to retrieve %s IRQ: %d\n",
					int_name, ret);
		return ret;
	}

	if (!strcmp(int_name, "fatal"))
		q6->fatal_irq = irq;
	else if (!strcmp(int_name, "stop-ack")) {
		q6->stop_irq = irq;
		tmp = "stop_ack";
	} else if (!strcmp(int_name, "ready"))
		q6->ready_irq = irq;
	else if (!strcmp(int_name, "handover"))
		q6->handover_irq  = irq;
	else if (!strcmp(int_name, "spawn-ack")) {
		q6->spawn_irq = irq;
		tmp = "spawn_ack";
	} else {
		dev_err(&pdev->dev, "unknown interrupt\n");
		return -EINVAL;
	}

	interrupt = devm_kzalloc(&pdev->dev, BUF_SIZE, GFP_KERNEL);
	if (!interrupt)
		return -ENOMEM;

	snprintf(interrupt, BUF_SIZE, "q6v5_wcss_userpd%d", wcss->pd_asid);
	strlcat(interrupt, "_", BUF_SIZE);
	strlcat(interrupt, tmp, BUF_SIZE);

	ret = devm_request_threaded_irq(&pdev->dev, irq,
			NULL, handler,
			IRQF_TRIGGER_RISING | IRQF_ONESHOT,
			interrupt, q6);
	if (ret) {
		dev_err(&pdev->dev, "failed to acquire %s irq\n",
				interrupt);
		return ret;
	}
	return 0;
}

static int q6_get_outbound_irq(struct qcom_q6v5 *q6,
			struct platform_device *pdev, const char *int_name)
{
	struct qcom_smem_state *tmp_state;
	unsigned  bit;

	tmp_state = qcom_smem_state_get(&pdev->dev, int_name, &bit);
	if (IS_ERR(tmp_state)) {
		dev_err(&pdev->dev, "failed to acquire %s state\n", int_name);
		return PTR_ERR(tmp_state);
	}

	if (!strcmp(int_name, "stop")) {
		q6->state = tmp_state;
		q6->stop_bit = bit;
	} else if (!strcmp(int_name, "spawn")) {
		q6->spawn_state = tmp_state;
		q6->spawn_bit = bit;
	}

	return 0;
}

static int init_irq(struct qcom_q6v5 *q6,
				struct platform_device *pdev,
				struct rproc *rproc, int remote_id,
				int crash_reason,
				void (*handover)(struct qcom_q6v5 *q6))
{
	int ret;

	q6->rproc = rproc;
	q6->dev = &pdev->dev;
	q6->crash_reason = crash_reason;
	q6->remote_id = remote_id;
	q6->handover = handover;

	init_completion(&q6->start_done);
	init_completion(&q6->stop_done);
	init_completion(&q6->spawn_done);

	ret = q6_get_inbound_irq(q6, pdev, "fatal",
					q6v5_fatal_interrupt);
	if (ret)
		return ret;

	ret = q6_get_inbound_irq(q6, pdev, "ready",
					q6v5_ready_interrupt);
	if (ret)
		return ret;

	ret = q6_get_inbound_irq(q6, pdev, "stop-ack",
					q6v5_stop_interrupt);
	if (ret)
		return ret;

	ret = q6_get_inbound_irq(q6, pdev, "spawn-ack",
					q6v5_spawn_interrupt);
	if (ret)
		return ret;

	ret = q6_get_outbound_irq(q6, pdev, "stop");
	if (ret)
		return ret;

	ret = q6_get_outbound_irq(q6, pdev, "spawn");
	if (ret)
		return ret;

	return 0;
}

static int q6_wcss_probe(struct platform_device *pdev)
{
	const struct wcss_data *desc;
	struct q6_wcss *wcss;
	struct rproc *rproc;
	int ret;
	char *subdev_name;
	bool nosec;
	const char *fw_name;

	desc = of_device_get_match_data(&pdev->dev);
	if (!desc)
		return -EINVAL;

	nosec = of_property_read_bool(pdev->dev.of_node, "qcom,nosecure");
	if (desc->need_mem_protection && !qcom_scm_is_available() && !nosec)
		return -EPROBE_DEFER;

	fw_name = desc->q6_firmware_name;
	if (!desc->q6_firmware_name)
		of_property_read_string(pdev->dev.of_node, "firmware",
					&fw_name);

	rproc = rproc_alloc(&pdev->dev, pdev->name, desc->ops,
				fw_name, sizeof(*wcss));
	if (!rproc) {
		dev_err(&pdev->dev, "failed to allocate rproc\n");
		return -ENOMEM;
	}
	wcss = rproc->priv;
	wcss->dev = &pdev->dev;
	wcss->version = desc->version;

	wcss->requires_force_stop = desc->requires_force_stop;
	wcss->need_mem_protection = desc->need_mem_protection;
	wcss->reset_cmd_id = desc->reset_cmd_id;
	wcss->is_fw_shared = desc->is_fw_shared;
	wcss->mdt_load_sec = desc->mdt_load_sec;
	if (nosec)
		wcss->need_mem_protection = false;

	ret = q6_alloc_memory_region(wcss);
	if (ret)
		goto free_rproc;

	wcss->pd_asid = qcom_get_pd_asid(wcss->dev->of_node);

	if (desc->init_irq) {
		ret = desc->init_irq(&wcss->q6, pdev, rproc, desc->remote_id,
				desc->crash_reason_smem, NULL);
		if (ret)
			goto free_rproc;
	}

	if (desc->glink_subdev_required)
		qcom_add_glink_subdev(rproc, &wcss->glink_subdev);

	subdev_name = (char *)(desc->ssr_name ? desc->ssr_name : pdev->name);
	qcom_add_ssr_subdev(rproc, &wcss->ssr_subdev, subdev_name);

	rproc->auto_boot = desc->need_auto_boot;
	ret = rproc_add(rproc);
	if (ret)
		goto free_rproc;

	platform_set_drvdata(pdev, rproc);

	ret = of_platform_populate(wcss->dev->of_node, NULL, NULL, wcss->dev);
	if (ret) {
		dev_err(&pdev->dev, "failed to populate wcss pd nodes\n");
		goto free_rproc;
	}

	return 0;

free_rproc:
	rproc_free(rproc);

	return ret;
}

static int q6_wcss_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	rproc_del(rproc);
	rproc_free(rproc);

	return 0;
}

static const struct wcss_data q6_ipq5332_res_init = {
	.init_irq = qcom_q6v5_init,
	.crash_reason_smem = WCSS_CRASH_REASON,
	.remote_id = WCSS_SMEM_HOST,
	.ssr_name = "q6wcss",
	.reset_cmd_id = 0x18,
	.ops = &q6_wcss_ipq5018_ops,
	.need_mem_protection = true,
	.need_auto_boot = false,
	.q6ver = Q6V7,
	.glink_subdev_required = true,
	.pasid = RPD_SWID,
	.bootargs_version = VERSION2,
	.clear_smp2p_last_value = true,
};

static const struct wcss_data q6_ipq5018_res_init = {
	.init_irq = qcom_q6v5_init,
	.q6_firmware_name = "IPQ5018/q6_fw.mdt",
	.crash_reason_smem = WCSS_CRASH_REASON,
	.ssr_name = "q6wcss",
	.reset_cmd_id = 0x14,
	.ops = &q6_wcss_ipq5018_ops,
	.need_mem_protection = true,
	.need_auto_boot = false,
	.q6ver = Q6V6,
	.glink_subdev_required = true,
	.pasid = MPD_WCNSS_PAS_ID,
	.bootargs_version = VERSION1,
};

static const struct wcss_data wcss_ahb_ipq5332_res_init = {
	.crash_reason_smem = WCSS_CRASH_REASON,
	.remote_id = WCSS_SMEM_HOST,
	.init_irq = init_irq,
	.ops = &wcss_ahb_ipq5332_ops,
	.need_mem_protection = true,
	.need_auto_boot = false,
	.q6ver = Q6V7,
	.version = WCSS_AHB_IPQ,
	.reset_seq = true,
	.mdt_load_sec = qcom_mdt_load,
};

static const struct wcss_data wcss_ahb_ipq5018_res_init = {
	.init_irq = init_irq,
	.q6_firmware_name = "IPQ5018/q6_fw.mdt",
	.crash_reason_smem = WCSS_CRASH_REASON,
	.ops = &wcss_ahb_ipq5018_ops,
	.need_mem_protection = true,
	.need_auto_boot = false,
	.q6ver = Q6V6,
	.version = WCSS_AHB_IPQ,
	.reset_seq = true,
	.is_fw_shared = true,
	.mdt_load_sec = qcom_mdt_load_pd_seg,
	.pasid = MPD_WCNSS_PAS_ID,
};

static const struct wcss_data wcss_pcie_ipq5332_res_init = {
	.init_irq = init_irq,
	.crash_reason_smem = WCSS_CRASH_REASON,
	.remote_id = WCSS_SMEM_HOST,
	.ops = &wcss_pcie_ipq5332_ops,
	.need_mem_protection = true,
	.need_auto_boot = false,
	.q6ver = Q6V7,
	.version = WCSS_PCIE_IPQ,
	.is_fw_shared = false,
	.mdt_load_sec = qcom_mdt_load,
};

static const struct wcss_data wcss_pcie_ipq5018_res_init = {
	.init_irq = init_irq,
	.q6_firmware_name = "IPQ5018/q6_fw.mdt",
	.crash_reason_smem = WCSS_CRASH_REASON,
	.ops = &wcss_pcie_ipq5018_ops,
	.need_mem_protection = true,
	.need_auto_boot = false,
	.q6ver = Q6V6,
	.version = WCSS_PCIE_IPQ,
	.is_fw_shared = true,
	.mdt_load_sec = qcom_mdt_load_pd_seg,
	.pasid = MPD_WCNSS_PAS_ID,
};

static const struct wcss_data wcss_text_ipq5332_res_init = {
	.ops = &wcss_text_ipq5332_ops,
	.need_mem_protection = true,
	.need_auto_boot = false,
	.q6ver = Q6V7,
	.version = WCSS_AHB_IPQ,
	.mdt_load_sec = qcom_mdt_load,
};

static const struct of_device_id q6_wcss_of_match[] = {
	{ .compatible = "qcom,ipq5018-q6-mpd", .data = &q6_ipq5018_res_init },
	{ .compatible = "qcom,ipq5332-q6-mpd", .data = &q6_ipq5332_res_init },
	{ .compatible = "qcom,ipq5018-wcss-ahb-mpd",
		.data = &wcss_ahb_ipq5018_res_init },
	{ .compatible = "qcom,ipq5332-wcss-ahb-mpd",
		.data = &wcss_ahb_ipq5332_res_init },
	{ .compatible = "qcom,ipq5332-wcss-pcie-mpd",
		.data = &wcss_pcie_ipq5332_res_init },
	{ .compatible = "qcom,ipq5018-wcss-pcie-mpd",
		.data = &wcss_pcie_ipq5018_res_init },
	{ .compatible = "qcom,ipq5332-mpd-upd-text",
		.data = &wcss_text_ipq5332_res_init },
	{ },
};
MODULE_DEVICE_TABLE(of, q6_wcss_of_match);

static struct platform_driver q6_wcss_driver = {
	.probe = q6_wcss_probe,
	.remove = q6_wcss_remove,
	.driver = {
		.name = "qcom-q6-mpd",
		.of_match_table = q6_wcss_of_match,
	},
};
module_platform_driver(q6_wcss_driver);
module_param(debug_wcss, int, 0644);
MODULE_DESCRIPTION("Hexagon WCSS Multipd Peripheral Image Loader");
MODULE_LICENSE("GPL v2");
