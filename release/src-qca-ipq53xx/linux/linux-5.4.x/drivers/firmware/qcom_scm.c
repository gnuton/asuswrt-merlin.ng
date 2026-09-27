// SPDX-License-Identifier: GPL-2.0-only
/*
 * Qualcomm SCM driver
 *
 * Copyright (c) 2010,2015, The Linux Foundation. All rights reserved.
 * Copyright (C) 2015 Linaro Ltd.
 */
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/qcom_scm.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/reset-controller.h>
#include <linux/qcom_scm.h>

#include "qcom_scm.h"

static bool download_mode = IS_ENABLED(CONFIG_QCOM_SCM_DOWNLOAD_MODE_DEFAULT);
module_param(download_mode, bool, 0);

#define SCM_HAS_CORE_CLK	BIT(0)
#define SCM_HAS_IFACE_CLK	BIT(1)
#define SCM_HAS_BUS_CLK		BIT(2)

struct qcom_scm {
	struct device *dev;
	struct clk *core_clk;
	struct clk *iface_clk;
	struct clk *bus_clk;
	struct reset_controller_dev reset;

	void __iomem *dload_reg;
	u64 dload_mode_addr;
	u32 hvc_log_cmd_id;
	u32 smmu_state_cmd_id;
};

struct qcom_scm_current_perm_info {
	__le32 vmid;
	__le32 perm;
	__le64 ctx;
	__le32 ctx_size;
	__le32 unused;
};

struct qcom_scm_mem_map_info {
	__le64 mem_addr;
	__le64 mem_size;
};

static struct qcom_scm *__scm;

static int qcom_scm_clk_enable(void)
{
	int ret;

	ret = clk_prepare_enable(__scm->core_clk);
	if (ret)
		goto bail;

	ret = clk_prepare_enable(__scm->iface_clk);
	if (ret)
		goto disable_core;

	ret = clk_prepare_enable(__scm->bus_clk);
	if (ret)
		goto disable_iface;

	return 0;

disable_iface:
	clk_disable_unprepare(__scm->iface_clk);
disable_core:
	clk_disable_unprepare(__scm->core_clk);
bail:
	return ret;
}

static void qcom_scm_clk_disable(void)
{
	clk_disable_unprepare(__scm->core_clk);
	clk_disable_unprepare(__scm->iface_clk);
	clk_disable_unprepare(__scm->bus_clk);
}

int qti_scm_qseecom_remove_xpu()
{
	int ret = 0;

	ret = __qti_scm_qseecom_remove_xpu(__scm->dev);

	return ret;
}
EXPORT_SYMBOL(qti_scm_qseecom_remove_xpu);

int qti_scm_qseecom_notify(struct qsee_notify_app *req, size_t req_size,
			   struct qseecom_command_scm_resp *resp,
			   size_t resp_size)
{
	int ret = 0;

	ret = __qti_scm_qseecom_notify(__scm->dev, req, req_size,
				      resp, resp_size);

	return ret;
}
EXPORT_SYMBOL(qti_scm_qseecom_notify);

int qti_scm_qseecom_load(uint32_t smc_id, uint32_t cmd_id,
			 union qseecom_load_ireq *req, size_t req_size,
			 struct qseecom_command_scm_resp *resp,
			 size_t resp_size)
{
	int ret = 0;

	ret = __qti_scm_qseecom_load(__scm->dev, smc_id, cmd_id, req, req_size,
				    resp, resp_size);

	return ret;
}
EXPORT_SYMBOL(qti_scm_qseecom_load);

int qti_scm_qseecom_send_data(union qseecom_client_send_data_ireq *req,
			      size_t req_size,
			      struct qseecom_command_scm_resp *resp,
			      size_t resp_size)
{
	int ret = 0;

	ret = __qti_scm_qseecom_send_data(__scm->dev, req, req_size,
					 resp, resp_size);

	return ret;
}
EXPORT_SYMBOL(qti_scm_qseecom_send_data);

int qti_scm_qseecom_unload(uint32_t smc_id, uint32_t cmd_id,
			   struct qseecom_unload_ireq *req,
			   size_t req_size,
			   struct qseecom_command_scm_resp *resp,
			   size_t resp_size)
{
	int ret = 0;

	ret = __qti_scm_qseecom_unload(__scm->dev, smc_id, cmd_id, req,
				      req_size, resp, resp_size);

	return ret;
}
EXPORT_SYMBOL(qti_scm_qseecom_unload);

int qti_scm_register_log_buf(struct device *dev,
				struct qsee_reg_log_buf_req *request,
				size_t req_size,
				struct qseecom_command_scm_resp *response,
				size_t resp_size)
{
	int ret = 0;

	ret = __qti_scm_register_log_buf(__scm->dev, request, req_size,
					    response, resp_size);

	return ret;
}
EXPORT_SYMBOL(qti_scm_register_log_buf);

int qti_scm_aes(uint32_t req_addr, uint32_t req_size, u32 cmd_id)
{
	int ret = 0;

	ret = __qcom_scm_is_call_available(__scm->dev, QTI_SVC_CRYPTO,
					   cmd_id);
	if (ret == 1) {
		ret = __qti_scm_aes(__scm->dev, req_addr, req_size, cmd_id);
	} else {
		pr_err("%s : Feature not supported by TZ..!\n", __func__);
		return -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL(qti_scm_aes);

int qti_scm_aes_clear_key_handle(uint32_t key_handle, u32 cmd_id)
{
	int ret = 0;

	ret = __qti_scm_aes_clear_key_handle(__scm->dev, key_handle, cmd_id);

	return ret;
}
EXPORT_SYMBOL(qti_scm_aes_clear_key_handle);

int qti_scm_tls_hardening(uint32_t req_addr, uint32_t req_size,
			  uint32_t resp_addr, uint32_t resp_size, u32 cmd_id)
{
	int ret = 0;

	ret = __qti_scm_tls_hardening(__scm->dev, req_addr, req_size,
				      resp_addr, resp_size, cmd_id);

	return ret;
}
EXPORT_SYMBOL(qti_scm_tls_hardening);

/**
 * qcom_scm_set_cold_boot_addr() - Set the cold boot address for cpus
 * @entry: Entry point function for the cpus
 * @cpus: The cpumask of cpus that will use the entry point
 *
 * Set the cold boot address of the cpus. Any cpu outside the supported
 * range would be removed from the cpu present mask.
 */
int qcom_scm_set_cold_boot_addr(void *entry, const cpumask_t *cpus)
{
	return __qcom_scm_set_cold_boot_addr(entry, cpus);
}
EXPORT_SYMBOL(qcom_scm_set_cold_boot_addr);

/**
 * qcom_scm_set_warm_boot_addr() - Set the warm boot address for cpus
 * @entry: Entry point function for the cpus
 * @cpus: The cpumask of cpus that will use the entry point
 *
 * Set the Linux entry point for the SCM to transfer control to when coming
 * out of a power down. CPU power down may be executed on cpuidle or hotplug.
 */
int qcom_scm_set_warm_boot_addr(void *entry, const cpumask_t *cpus)
{
	return __qcom_scm_set_warm_boot_addr(__scm->dev, entry, cpus);
}
EXPORT_SYMBOL(qcom_scm_set_warm_boot_addr);

/**
 * qcom_scm_cpu_power_down() - Power down the cpu
 * @flags - Flags to flush cache
 *
 * This is an end point to power down cpu. If there was a pending interrupt,
 * the control would return from this function, otherwise, the cpu jumps to the
 * warm boot entry point set for this cpu upon reset.
 */
void qcom_scm_cpu_power_down(u32 flags)
{
	__qcom_scm_cpu_power_down(flags);
}
EXPORT_SYMBOL(qcom_scm_cpu_power_down);

/**
 * qcom_scm_hdcp_available() - Check if secure environment supports HDCP.
 *
 * Return true if HDCP is supported, false if not.
 */
bool qcom_scm_hdcp_available(void)
{
	int ret = qcom_scm_clk_enable();

	if (ret)
		return ret;

	ret = __qcom_scm_is_call_available(__scm->dev, QCOM_SCM_SVC_HDCP,
						QCOM_SCM_CMD_HDCP);

	qcom_scm_clk_disable();

	return ret > 0 ? true : false;
}
EXPORT_SYMBOL(qcom_scm_hdcp_available);

/**
 * qcom_scm_hdcp_req() - Send HDCP request.
 * @req: HDCP request array
 * @req_cnt: HDCP request array count
 * @resp: response buffer passed to SCM
 *
 * Write HDCP register(s) through SCM.
 */
int qcom_scm_hdcp_req(struct qcom_scm_hdcp_req *req, u32 req_cnt, u32 *resp)
{
	int ret = qcom_scm_clk_enable();

	if (ret)
		return ret;

	ret = __qcom_scm_hdcp_req(__scm->dev, req, req_cnt, resp);
	qcom_scm_clk_disable();
	return ret;
}
EXPORT_SYMBOL(qcom_scm_hdcp_req);

/**
 * qcom_scm_pas_supported() - Check if the peripheral authentication service is
 *			      available for the given peripherial
 * @peripheral:	peripheral id
 *
 * Returns true if PAS is supported for this peripheral, otherwise false.
 */
bool qcom_scm_pas_supported(u32 peripheral)
{
	int ret;

	ret = __qcom_scm_is_call_available(__scm->dev, QCOM_SCM_SVC_PIL,
					   QCOM_SCM_PAS_IS_SUPPORTED_CMD);
	if (ret <= 0)
		return false;

	return __qcom_scm_pas_supported(__scm->dev, peripheral);
}
EXPORT_SYMBOL(qcom_scm_pas_supported);

/**
 * qcom_scm_pas_init_image() - Initialize peripheral authentication service
 *			       state machine for a given peripheral, using the
 *			       metadata
 * @peripheral: peripheral id
 * @metadata:	pointer to memory containing ELF header, program header table
 *		and optional blob of data used for authenticating the metadata
 *		and the rest of the firmware
 * @size:	size of the metadata
 *
 * Returns 0 on success.
 */
int qcom_scm_pas_init_image(u32 peripheral, const void *metadata, size_t size)
{
	dma_addr_t mdata_phys;
	void *mdata_buf;
	int ret;

	/*
	 * During the scm call memory protection will be enabled for the meta
	 * data blob, so make sure it's physically contiguous, 4K aligned and
	 * non-cachable to avoid XPU violations.
	 */
	mdata_buf = dma_alloc_coherent(__scm->dev, size, &mdata_phys,
				       GFP_KERNEL);
	if (!mdata_buf) {
		dev_err(__scm->dev, "Allocation of metadata buffer failed.\n");
		return -ENOMEM;
	}
	memcpy(mdata_buf, metadata, size);

	ret = qcom_scm_clk_enable();
	if (ret)
		goto free_metadata;

	if (__qcom_scm_is_call_available(__scm->dev, QCOM_SCM_SVC_PIL,
					QCOM_SCM_PAS_INIT_IMAGE_V2_CMD))
		ret = __qcom_scm_pas_init_image_v2(__scm->dev, peripheral,
							mdata_phys, size);
	else
		ret = __qcom_scm_pas_init_image(__scm->dev, peripheral,
								mdata_phys);

	qcom_scm_clk_disable();

free_metadata:
	dma_free_coherent(__scm->dev, size, mdata_buf, mdata_phys);

	return ret;
}
EXPORT_SYMBOL(qcom_scm_pas_init_image);

/**
 * qcom_scm_pas_mem_setup() - Prepare the memory related to a given peripheral
 *			      for firmware loading
 * @peripheral:	peripheral id
 * @addr:	start address of memory area to prepare
 * @size:	size of the memory area to prepare
 *
 * Returns 0 on success.
 */
int qcom_scm_pas_mem_setup(u32 peripheral, phys_addr_t addr, phys_addr_t size)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qcom_scm_pas_mem_setup(__scm->dev, peripheral, addr, size);
	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qcom_scm_pas_mem_setup);

/**
 * qcom_scm_pas_auth_and_reset() - Authenticate the given peripheral firmware
 *				   and reset the remote processor
 * @peripheral:	peripheral id
 *
 * Return 0 on success.
 */
int qcom_scm_pas_auth_and_reset(u32 peripheral, u32 debug, u32 reset_cmd_id)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qcom_scm_pas_auth_and_reset(__scm->dev, peripheral, debug,
								reset_cmd_id);
	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qcom_scm_pas_auth_and_reset);

/**
 * qcom_scm_pas_shutdown() - Shut down the remote processor
 * @peripheral: peripheral id
 *
 * Returns 0 on success.
 */
int qcom_scm_pas_shutdown(u32 peripheral)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qcom_scm_pas_shutdown(__scm->dev, peripheral);
	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qcom_scm_pas_shutdown);

static int qcom_scm_pas_reset_assert(struct reset_controller_dev *rcdev,
				     unsigned long idx)
{
	if (idx != 0)
		return -EINVAL;

	return __qcom_scm_pas_mss_reset(__scm->dev, 1);
}

static int qcom_scm_pas_reset_deassert(struct reset_controller_dev *rcdev,
				       unsigned long idx)
{
	if (idx != 0)
		return -EINVAL;

	return __qcom_scm_pas_mss_reset(__scm->dev, 0);
}

static const struct reset_control_ops qcom_scm_pas_reset_ops = {
	.assert = qcom_scm_pas_reset_assert,
	.deassert = qcom_scm_pas_reset_deassert,
};

int qcom_scm_restore_sec_cfg(u32 device_id, u32 spare)
{
	return __qcom_scm_restore_sec_cfg(__scm->dev, device_id, spare);
}
EXPORT_SYMBOL(qcom_scm_restore_sec_cfg);

int qcom_scm_iommu_secure_ptbl_size(u32 spare, size_t *size)
{
	return __qcom_scm_iommu_secure_ptbl_size(__scm->dev, spare, size);
}
EXPORT_SYMBOL(qcom_scm_iommu_secure_ptbl_size);

int qcom_scm_iommu_secure_ptbl_init(u64 addr, u32 size, u32 spare)
{
	return __qcom_scm_iommu_secure_ptbl_init(__scm->dev, addr, size, spare);
}
EXPORT_SYMBOL(qcom_scm_iommu_secure_ptbl_init);

int qcom_scm_io_readl(phys_addr_t addr, unsigned int *val)
{
	return __qcom_scm_io_readl(__scm->dev, addr, val);
}
EXPORT_SYMBOL(qcom_scm_io_readl);

int qcom_scm_io_writel(phys_addr_t addr, unsigned int val)
{
	return __qcom_scm_io_writel(__scm->dev, addr, val);
}
EXPORT_SYMBOL(qcom_scm_io_writel);

static void qcom_scm_set_download_mode(bool enable)
{
	bool avail;
	int ret = 0;

	avail = __qcom_scm_is_call_available(__scm->dev,
					     QCOM_SCM_SVC_BOOT,
					     QCOM_SCM_SET_DLOAD_MODE);
	if (avail) {
		ret = __qcom_scm_set_dload_mode(__scm->dev, enable);
	} else if (__scm->dload_mode_addr) {
		ret = __qcom_scm_io_writel(__scm->dev, __scm->dload_mode_addr,
					   enable ? QCOM_SCM_SET_DLOAD_MODE : 0);
	} else {
		dev_err(__scm->dev,
			"No available mechanism for setting download mode\n");
	}

	if (ret)
		dev_err(__scm->dev, "failed to set download mode: %d\n", ret);
}

static int qcom_scm_find_dload_address(struct device *dev, struct qcom_scm *scm)
{
	struct device_node *tcsr;
	struct device_node *np = dev->of_node;
	struct resource res;
	u32 offset;
	long feat_avail;
	int ret;

	tcsr = of_parse_phandle(np, "qcom,dload-mode", 0);
	if (!tcsr)
		return 0;

	ret = of_address_to_resource(tcsr, 0, &res);
	of_node_put(tcsr);
	if (ret)
		return ret;

	ret = of_property_read_u32_index(np, "qcom,dload-mode", 1, &offset);
	if (ret < 0)
		return ret;

	scm->dload_mode_addr = res.start + offset;

	feat_avail = qti_scm_is_feature_available(QCOM_SCM_SVC_INFO, SCM_SVC_UTIL, QCOM_DLOAD_READ_MODE);
        if (feat_avail != QCOM_DLOAD_SEC_READ) {
		scm->dload_reg = devm_ioremap(dev, res.start, resource_size(&res));
		if (!scm->dload_reg) {
			pr_err("%s: Error mapping memory region!\n", __func__);
			return -ENOMEM;
		}
	}

	return 0;
}

/**
 * qcom_scm_is_available() - Checks if SCM is available
 */
bool qcom_scm_is_available(void)
{
	return !!__scm;
}
EXPORT_SYMBOL(qcom_scm_is_available);

int qcom_scm_set_remote_state(u32 state, u32 id)
{
	return __qcom_scm_set_remote_state(__scm->dev, state, id);
}
EXPORT_SYMBOL(qcom_scm_set_remote_state);

/**
 * qcom_scm_assign_mem() - Make a secure call to reassign memory ownership
 * @mem_addr: mem region whose ownership need to be reassigned
 * @mem_sz:   size of the region.
 * @srcvm:    vmid for current set of owners, each set bit in
 *            flag indicate a unique owner
 * @newvm:    array having new owners and corresponding permission
 *            flags
 * @dest_cnt: number of owners in next set.
 *
 * Return negative errno on failure or 0 on success with @srcvm updated.
 */
int qcom_scm_assign_mem(phys_addr_t mem_addr, size_t mem_sz,
			unsigned int *srcvm,
			const struct qcom_scm_vmperm *newvm,
			unsigned int dest_cnt)
{
	struct qcom_scm_current_perm_info *destvm;
	struct qcom_scm_mem_map_info *mem_to_map;
	phys_addr_t mem_to_map_phys;
	phys_addr_t dest_phys;
	dma_addr_t ptr_phys;
	size_t mem_to_map_sz;
	size_t dest_sz;
	size_t src_sz;
	size_t ptr_sz;
	int next_vm;
	__le32 *src;
	void *ptr;
	int ret, i, b;
	unsigned long srcvm_bits = *srcvm;

	src_sz = hweight_long(srcvm_bits) * sizeof(*src);
	mem_to_map_sz = sizeof(*mem_to_map);
	dest_sz = dest_cnt * sizeof(*destvm);
	ptr_sz = ALIGN(src_sz, SZ_64) + ALIGN(mem_to_map_sz, SZ_64) +
			ALIGN(dest_sz, SZ_64);

	ptr = dma_alloc_coherent(__scm->dev, ptr_sz, &ptr_phys, GFP_KERNEL);
	if (!ptr)
		return -ENOMEM;

	/* Fill source vmid detail */
	src = ptr;
	i = 0;
	for_each_set_bit(b, &srcvm_bits, BITS_PER_LONG)
		src[i++] = cpu_to_le32(b);

	/* Fill details of mem buff to map */
	mem_to_map = ptr + ALIGN(src_sz, SZ_64);
	mem_to_map_phys = ptr_phys + ALIGN(src_sz, SZ_64);
	mem_to_map->mem_addr = cpu_to_le64(mem_addr);
	mem_to_map->mem_size = cpu_to_le64(mem_sz);

	next_vm = 0;
	/* Fill details of next vmid detail */
	destvm = ptr + ALIGN(mem_to_map_sz, SZ_64) + ALIGN(src_sz, SZ_64);
	dest_phys = ptr_phys + ALIGN(mem_to_map_sz, SZ_64) + ALIGN(src_sz, SZ_64);
	for (i = 0; i < dest_cnt; i++, destvm++, newvm++) {
		destvm->vmid = cpu_to_le32(newvm->vmid);
		destvm->perm = cpu_to_le32(newvm->perm);
		destvm->ctx = 0;
		destvm->ctx_size = 0;
		next_vm |= BIT(newvm->vmid);
	}

	ret = __qcom_scm_assign_mem(__scm->dev, mem_to_map_phys, mem_to_map_sz,
				    ptr_phys, src_sz, dest_phys, dest_sz);
	dma_free_coherent(__scm->dev, ptr_sz, ptr, ptr_phys);
	if (ret) {
		dev_err(__scm->dev,
			"Assign memory protection call failed %d\n", ret);
		return -EINVAL;
	}

	*srcvm = next_vm;
	return 0;
}
EXPORT_SYMBOL(qcom_scm_assign_mem);

/**
 * qcom_qfprom_show_authenticate() - Check secure boot fuse is enabled
 */
int qti_qfprom_show_authenticate(void)
{
	int ret;
	char buf;

	ret = __qti_qfprom_show_authenticate(__scm->dev, &buf);

	if (ret) {
		pr_err("%s: Error in QFPROM read : %d\n", __func__, ret);
		return -1;
	}

	return buf == 1 ? 1 : 0;
}
EXPORT_SYMBOL(qti_qfprom_show_authenticate);

int qti_qfprom_write_version(uint32_t sw_type,
			uint32_t value, uint32_t qfprom_ret_ptr)

{
	return __qti_qfprom_write_version(__scm->dev, sw_type, value,
						   qfprom_ret_ptr);
}

int qti_qfprom_read_version(uint32_t sw_type,
			uint32_t value, uint32_t qfprom_ret_ptr)
{
	return __qti_qfprom_read_version(__scm->dev, sw_type, value,
						qfprom_ret_ptr);
}

int qti_sec_upgrade_auth(unsigned int scm_cmd_id, unsigned int sw_type,
				unsigned int img_size, unsigned int load_addr)
{
	return __qti_sec_upgrade_auth(__scm->dev, scm_cmd_id, sw_type,
						img_size, load_addr);
}
EXPORT_SYMBOL(qti_sec_upgrade_auth);

int qti_sec_upgrade_auth_meta_data(unsigned int scm_cmd_id, unsigned int sw_type,
					unsigned int img_size, unsigned int load_addr,
					void* hash_addr, unsigned int hash_size)
{
	return __qti_sec_upgrade_auth_meta_data(__scm->dev, scm_cmd_id, sw_type,
					img_size, load_addr, hash_addr, hash_size);
}
EXPORT_SYMBOL(qti_sec_upgrade_auth_meta_data);

/**
 * qti_scm_sec_auth_available() - Check if SEC_AUTH is supported.
 *
 * Return true if SEC_AUTH is supported, false if not.
 */
bool qti_scm_sec_auth_available(unsigned int scm_cmd_id)
{
	int ret;

	ret = __qcom_scm_is_call_available(__scm->dev, QTI_SCM_SVC_SEC_AUTH,
								scm_cmd_id);

	return ret > 0 ? true : false;
}
EXPORT_SYMBOL(qti_scm_sec_auth_available);

int qti_fuseipq_scm_call(struct device *dev, u32 svc_id, u32 cmd_id,
			  void *cmd_buf, size_t size)
{
	return __qti_fuseipq_scm_call(dev, svc_id, cmd_id,
				       cmd_buf, size);
}
EXPORT_SYMBOL(qti_fuseipq_scm_call);

int qti_scm_dload(u32 svc_id, u32 cmd_id, void *cmd_buf)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qti_scm_dload(__scm->dev, svc_id, cmd_id, cmd_buf,
			      __scm->dload_mode_addr, __scm->dload_reg);

	qcom_scm_clk_disable();

	return ret;

}
EXPORT_SYMBOL(qti_scm_dload);

int qti_scm_set_trybit(u32 svc_id)
{
	int ret;
	unsigned int val;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = qti_read_dload_reg(&val);
	if (ret)
		goto exit;

	val |= QTI_TRYBIT;
	ret = __qti_scm_set_trybit(__scm->dev, svc_id, val, __scm->dload_mode_addr);

exit:
	qcom_scm_clk_disable();

	return ret;

}
EXPORT_SYMBOL(qti_scm_set_trybit);

int qti_read_dload_reg(uint32_t *val)
{
	int ret = 0;
	long feat_avail;

	/* The TCSR dload register is protected for IPQ5332 target in latest TZ
	 * Old TZ will allow direct read
	 * Use the qca_scm_is_feature_available() call to know if TZ supports direct or scm read
	 * Based on the return value read the TCSR dload register appropriately
	 */
	feat_avail = qti_scm_is_feature_available(QCOM_SCM_SVC_INFO, SCM_SVC_UTIL, QCOM_DLOAD_READ_MODE);
	if (feat_avail == QCOM_DLOAD_SEC_READ) {
		ret = qcom_scm_io_readl(__scm->dload_mode_addr, val);
		if (ret)
			return ret;
	}
	else {
		if (!__scm->dload_reg)
                {
			pr_info("Invalid dload reg virtual address\n");
			return -EINVAL;
                }
		*val = readl(__scm->dload_reg);
		ret = 0;
	}

	return ret;
}
EXPORT_SYMBOL(qti_read_dload_reg);

int qti_scm_get_ecdsa_blob(u32 svc_id, u32 cmd_id, dma_addr_t nonce_buf,
			u32 nonce_buf_len, dma_addr_t ecdsa_buf,
			u32 ecdsa_buf_len, u32 *ecdsa_consumed_len)
{
	int ret;
	ret = __qti_scm_get_ecdsa_blob(__scm->dev, svc_id, cmd_id, nonce_buf,
			nonce_buf_len, ecdsa_buf, ecdsa_buf_len, ecdsa_consumed_len);
	return ret;

}
EXPORT_SYMBOL(qti_scm_get_ecdsa_blob);

long qti_scm_is_feature_available(u32 svc_id, u32 cmd_id, u32 feature_id)
{
	int ret;
	ret = __qti_scm_is_feature_available(__scm->dev, svc_id, cmd_id,
							feature_id);
	return ret;
}
EXPORT_SYMBOL(qti_scm_is_feature_available);

int qti_scm_get_device_attestation_ephimeral_key(u32 svc_id, u32 cmd_id,
			void *key_buf, u32 key_buf_len, u32 *key_len)
{
	int ret;
	ret = __qti_scm_get_device_attestation_ephimeral_key(__scm->dev,
			svc_id, cmd_id, key_buf, key_buf_len, key_len);
	return ret;

}
EXPORT_SYMBOL(qti_scm_get_device_attestation_ephimeral_key);

int qti_scm_get_ipq_fuse_list(u32 svc_id, u32 cmd_id, void *fuse, size_t size)
{
	int ret;
	ret = __qti_scm_get_ipq_fuse_list(__scm->dev,
			svc_id, cmd_id, fuse, size);
	return ret;

}
EXPORT_SYMBOL(qti_scm_get_ipq_fuse_list);

int qti_scm_get_device_attestation_response(u32 svc_id, u32 cmd_id, void *req_buf,
			u32 req_buf_len, void *extclaim_buf, u32 extclaim_buf_len,
			void *resp_buf, u32 resp_buf_len, u32 *attest_resp_len)
{
	int ret;

	ret = __qti_scm_get_device_attestation_response(__scm->dev, svc_id, cmd_id,
			req_buf, req_buf_len, extclaim_buf, extclaim_buf_len, resp_buf,
			resp_buf_len, attest_resp_len);

	return ret;

}
EXPORT_SYMBOL(qti_scm_get_device_attestation_response);

int qti_scm_get_device_provision_response(u32 svc_id, u32 cmd_id, void *provreq_buf,
			u32 provreq_buf_len, void *provresp_buf, u32 provresp_buf_len,
			u32 *prov_resp_size)
{
	int ret;

	ret = __qti_scm_get_device_provision_response(__scm->dev, svc_id, cmd_id,
			provreq_buf, provreq_buf_len, provresp_buf,
			provresp_buf_len, prov_resp_size);

	return ret;

}
EXPORT_SYMBOL(qti_scm_get_device_provision_response);

int qti_scm_derive_and_share_key(u32 svc_id, u32 cmd_id, uint32_t key_len,
			uint8_t *sw_context, u32 sw_context_len,
			uint8_t *derived_key, u32 derived_key_len)
{
	int ret;
	ret = __qti_scm_derive_and_share_key(__scm->dev, svc_id, cmd_id,
			key_len, sw_context, sw_context_len, derived_key,
			derived_key_len);
	return ret;
}
EXPORT_SYMBOL_GPL(qti_scm_derive_and_share_key);

int qti_scm_wcss_boot(u32 svc_id, u32 cmd_id, void *cmd_buf)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qti_scm_wcss_boot(__scm->dev, svc_id, cmd_id, cmd_buf);

	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qti_scm_wcss_boot);

int qti_scm_pdseg_memcpy_v2(u32 peripheral, int phno, dma_addr_t dma,
							int seg_cnt)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qti_scm_pdseg_memcpy_v2(__scm->dev, peripheral, phno, dma,
			seg_cnt);

	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qti_scm_pdseg_memcpy_v2);

int qti_scm_pdseg_memcpy(u32 peripheral, int phno, dma_addr_t dma,
							size_t size)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qti_scm_pdseg_memcpy(__scm->dev, peripheral, phno, dma, size);

	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qti_scm_pdseg_memcpy);

int qti_scm_int_radio_powerup(u32 peripheral)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qti_scm_int_radio_powerup(__scm->dev, peripheral);

	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qti_scm_int_radio_powerup);

int qti_scm_int_radio_powerdown(u32 peripheral)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qti_scm_int_radio_powerdown(__scm->dev, peripheral);

	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qti_scm_int_radio_powerdown);

int qti_scm_sdi(u32 svc_id, u32 cmd_id)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;
	ret = __qti_scm_sdi(__scm->dev, svc_id, cmd_id);

	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qti_scm_sdi);

/**
 * qti_scm_tz_log() - Get trustzone diag log
 * ker_buf: kernel buffer to store the diag log
 * buf_len: kernel buffer length
 *
 * Return negative errno on failure or 0 on success. Diag log will
 * be present in the kernel buffer passed.
 */
int qti_scm_tz_log(void *ker_buf, u32 buf_len)
{
	return __qti_scm_tz_hvc_log(__scm->dev, QCOM_SCM_SVC_INFO,
				     QTI_SCM_TZ_DIAG_CMD, ker_buf, buf_len);
}
EXPORT_SYMBOL(qti_scm_tz_log);

/**
 * qti_scm_hvc_log() - Get hypervisor diag log
 * ker_buf: kernel buffer to store the diag log
 * buf_len: kernel buffer length
 *
 * Return negative errno on failure or 0 on success. Diag log will
 * be present in the kernel buffer passed.
 */
int qti_scm_hvc_log(void *ker_buf, u32 buf_len)
{
	return __qti_scm_tz_hvc_log(__scm->dev, QCOM_SCM_SVC_INFO,
				    __scm->hvc_log_cmd_id, ker_buf, buf_len);
}
EXPORT_SYMBOL(qti_scm_hvc_log);

/**
 * qti_scm_get_smmustate () - Get SMMU state
 *
 * Returns 0 - SMMU_DISABLE_NONE
 *	   1 - SMMU_DISABLE_S2
 *	   2 - SMMU_DISABLE_ALL on success.
 *	  -1 - Failure
 */
int qti_scm_get_smmustate(void)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qti_scm_get_smmustate(__scm->dev, QCOM_SCM_SVC_BOOT,
				      __scm->smmu_state_cmd_id);
	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qti_scm_get_smmustate);

/**
 * qti_scm_regsave pass a buffer to tz for saving cpu context
 * Retruns 0 on success
 * Err otherwise
 */
int qti_scm_regsave(u32 svc_id, u32 cmd_id, void *scm_regsave, u32 buf_size)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;
	ret = __qti_scm_regsave(__scm->dev, svc_id, cmd_id,
						scm_regsave, buf_size);
	qcom_scm_clk_disable();
	return ret;
}
EXPORT_SYMBOL(qti_scm_regsave);

/*
 * qcom_set_qcekey_sec() - Configure key securely
 */
int qti_set_qcekey_sec(void *buf, int size)
{
	return __qti_set_qcekey_sec(__scm->dev, buf, size);
}
EXPORT_SYMBOL(qti_set_qcekey_sec);

int qti_sec_crypt(void *buf, int size)
{
	return __qti_sec_crypt(__scm->dev, buf, size);
}
EXPORT_SYMBOL(qti_sec_crypt);

/*
 * qti_qcekey_release_xpu_prot() - release XPU protection
 */
int qti_qcekey_release_xpu_prot(void)
{
	return __qti_qcekey_release_xpu_prot(__scm->dev);
}
EXPORT_SYMBOL(qti_qcekey_release_xpu_prot);

int qti_seccrypt_clearkey(void)
{
	return __qti_seccrypt_clearkey(__scm->dev);
}
EXPORT_SYMBOL(qti_seccrypt_clearkey);

/**
 * qti_scm_resettype () - cold or warm reset
 * @reset type: 0 for cold 1 for warm
 *
 * Returns 0 on success.
 */
int qti_scm_set_resettype(u32 reset_type)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qti_scm_set_resettype(__scm->dev, reset_type);
	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qti_scm_set_resettype);

int qti_scm_tcsr_reg_write(u32 reg_addr, u32 value)
{
	return __qti_scm_tcsr_reg_write(__scm->dev, reg_addr, value);
}
EXPORT_SYMBOL(qti_scm_tcsr_reg_write);

/*
 * qti_config_sec_ice() - Configure ICE block securely
 */
int qcom_config_sec_ice(void *buf, int size)
{
	return __qcom_config_ice_sec(__scm->dev, buf, size);
}
EXPORT_SYMBOL(qcom_config_sec_ice);

int qcom_context_sec_ice(u32 type, u8 key_size, u8 algo_mode, u8 *data_ctxt,
		u32 data_ctxt_len, u8 *salt_ctxt, u32 salt_ctxt_len)
{
	return __qcom_context_ice_sec(__scm->dev, type, key_size, algo_mode,
		 data_ctxt, data_ctxt_len, salt_ctxt, salt_ctxt_len);
}
EXPORT_SYMBOL(qcom_context_sec_ice);

/*
 * qti_scm_pshold() - TZ performs the PSHOLD operation
 */
int qti_scm_pshold(void)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;
	ret = __qti_scm_pshold(__scm->dev);

	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qti_scm_pshold);

int qti_scm_extwdt(u32 svc_id, u32 cmd_id, unsigned int regaddr,
						unsigned int val)
{
	return __qti_scm_extwdt(__scm->dev, svc_id, cmd_id,
						regaddr, val);
}
EXPORT_SYMBOL(qti_scm_extwdt);

int qti_scm_is_tz_log_encryption_supported(void)
{
	int ret;

	ret = __qcom_scm_is_call_available(__scm->dev, QTI_SVC_APP_MGR,
					   QCOM_SCM_IS_TZ_LOG_ENCRYPTED);

	return (ret == 1) ? 1 : 0;
}
EXPORT_SYMBOL(qti_scm_is_tz_log_encryption_supported);

int qti_scm_is_tz_log_encrypted(void)
{
	int ret;

	ret = __qti_scm_is_tz_log_encrypted(__scm->dev);

	return (ret == 1) ? 1 : 0;
}
EXPORT_SYMBOL(qti_scm_is_tz_log_encrypted);

int qti_scm_get_encrypted_tz_log(void *ker_buf, u32 buf_len, u32 log_id)
{
	return __qti_scm_get_encrypted_tz_log(__scm->dev, ker_buf, buf_len, log_id);
}
EXPORT_SYMBOL(qti_scm_get_encrypted_tz_log);

/**
 * qti_scm_load_otp () - Load OTP to device memory
 * @peripheral:	peripheral id
 *
 * Return 0 on success.
 */
int qti_scm_load_otp(u32 peripheral)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qti_scm_load_otp(__scm->dev, peripheral);
	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qti_scm_load_otp);

bool qti_scm_pil_cfg_available(void)
{
	int ret;

	ret = __qcom_scm_is_call_available(__scm->dev, QTI_SCM_SVC_XO_TCXO,
                                                QTI_SCM_CMD_XO_TCXO);

	return ret > 0 ? true : false;
}
EXPORT_SYMBOL(qti_scm_pil_cfg_available);

int qti_scm_pil_cfg(u32 peripheral, u32 arg)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qti_scm_pil_cfg(__scm->dev, peripheral, arg);
	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qti_scm_pil_cfg);

/**
 * qti_scm_toggle_bt_eco () - Toggle ECO bit
 * @peripheral:	peripheral id
 *
 * Return 0 on success.
 */
int qti_scm_toggle_bt_eco(u32 peripheral, u32 arg)
{
	int ret;

	ret = qcom_scm_clk_enable();
	if (ret)
		return ret;

	ret = __qti_scm_toggle_bt_eco(__scm->dev, peripheral, arg);
	qcom_scm_clk_disable();

	return ret;
}
EXPORT_SYMBOL(qti_scm_toggle_bt_eco);

static int qcom_scm_probe(struct platform_device *pdev)
{
	struct device_node *np = (&pdev->dev)->of_node;
	struct qcom_scm *scm;
	unsigned long clks;
	int ret;

	scm = devm_kzalloc(&pdev->dev, sizeof(*scm), GFP_KERNEL);
	if (!scm)
		return -ENOMEM;

	ret = of_property_read_u32(np, "hvc-log-cmd-id", &scm->hvc_log_cmd_id);
	if (ret)
		scm->hvc_log_cmd_id = QTI_SCM_HVC_DIAG_CMD;

	ret = of_property_read_u32(pdev->dev.of_node, "smmu-state-cmd-id",
				   &scm->smmu_state_cmd_id);
	if (ret)
		scm->smmu_state_cmd_id = QTI_SCM_SMMUSTATE_CMD;

	clks = (unsigned long)of_device_get_match_data(&pdev->dev);

	scm->core_clk = devm_clk_get(&pdev->dev, "core");
	if (IS_ERR(scm->core_clk)) {
		if (PTR_ERR(scm->core_clk) == -EPROBE_DEFER)
			return PTR_ERR(scm->core_clk);

		if (clks & SCM_HAS_CORE_CLK) {
			dev_err(&pdev->dev, "failed to acquire core clk\n");
			return PTR_ERR(scm->core_clk);
		}

		scm->core_clk = NULL;
	}

	scm->iface_clk = devm_clk_get(&pdev->dev, "iface");
	if (IS_ERR(scm->iface_clk)) {
		if (PTR_ERR(scm->iface_clk) == -EPROBE_DEFER)
			return PTR_ERR(scm->iface_clk);

		if (clks & SCM_HAS_IFACE_CLK) {
			dev_err(&pdev->dev, "failed to acquire iface clk\n");
			return PTR_ERR(scm->iface_clk);
		}

		scm->iface_clk = NULL;
	}

	scm->bus_clk = devm_clk_get(&pdev->dev, "bus");
	if (IS_ERR(scm->bus_clk)) {
		if (PTR_ERR(scm->bus_clk) == -EPROBE_DEFER)
			return PTR_ERR(scm->bus_clk);

		if (clks & SCM_HAS_BUS_CLK) {
			dev_err(&pdev->dev, "failed to acquire bus clk\n");
			return PTR_ERR(scm->bus_clk);
		}

		scm->bus_clk = NULL;
	}

	scm->reset.ops = &qcom_scm_pas_reset_ops;
	scm->reset.nr_resets = 1;
	scm->reset.of_node = pdev->dev.of_node;
	ret = devm_reset_controller_register(&pdev->dev, &scm->reset);
	if (ret)
		return ret;

	/* vote for max clk rate for highest performance */
	ret = clk_set_rate(scm->core_clk, INT_MAX);
	if (ret)
		return ret;

	__scm = scm;
	__scm->dev = &pdev->dev;

	ret = qcom_scm_find_dload_address(&pdev->dev, scm);
	if (ret < 0)
		return ret;

	__qcom_scm_init();

	/*
	 * If requested enable "download mode", from this point on warmboot
	 * will cause the the boot stages to enter download mode, unless
	 * disabled below by a clean shutdown/reboot.
	 */
	if (download_mode)
		qcom_scm_set_download_mode(true);

	return 0;
}

static void qcom_scm_shutdown(struct platform_device *pdev)
{
	/* Clean shutdown, disable download mode to allow normal restart */
	qcom_scm_set_download_mode(false);
}

static const struct of_device_id qcom_scm_dt_match[] = {
	{ .compatible = "qcom,scm-apq8064",
	  /* FIXME: This should have .data = (void *) SCM_HAS_CORE_CLK */
	},
	{ .compatible = "qcom,scm-apq8084", .data = (void *)(SCM_HAS_CORE_CLK |
							     SCM_HAS_IFACE_CLK |
							     SCM_HAS_BUS_CLK)
	},
	{ .compatible = "qcom,scm-ipq4019" },
	{ .compatible = "qcom,scm-msm8660", .data = (void *) SCM_HAS_CORE_CLK },
	{ .compatible = "qcom,scm-msm8960", .data = (void *) SCM_HAS_CORE_CLK },
	{ .compatible = "qcom,scm-msm8916", .data = (void *)(SCM_HAS_CORE_CLK |
							     SCM_HAS_IFACE_CLK |
							     SCM_HAS_BUS_CLK)
	},
	{ .compatible = "qcom,scm-msm8974", .data = (void *)(SCM_HAS_CORE_CLK |
							     SCM_HAS_IFACE_CLK |
							     SCM_HAS_BUS_CLK)
	},
	{ .compatible = "qcom,scm-msm8996" },
	{ .compatible = "qcom,scm" },
	{}
};

static struct platform_driver qcom_scm_driver = {
	.driver = {
		.name	= "qcom_scm",
		.of_match_table = qcom_scm_dt_match,
	},
	.probe = qcom_scm_probe,
	.shutdown = qcom_scm_shutdown,
};

static int __init qcom_scm_init(void)
{
	return platform_driver_register(&qcom_scm_driver);
}
subsys_initcall(qcom_scm_init);
