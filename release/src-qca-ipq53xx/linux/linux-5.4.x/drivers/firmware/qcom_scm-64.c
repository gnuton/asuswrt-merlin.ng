// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
 */

#include <linux/io.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/qcom_scm.h>
#include <linux/arm-smccc.h>
#include <linux/dma-mapping.h>

#include "qcom_scm.h"

#define ICE_CRYPTO_ALGO_MODE_AES_XTS    0x3

#define QCOM_SCM_FNID(s, c) ((((s) & 0xFF) << 8) | ((c) & 0xFF))

#define QCOM_SCM_ARGS_IMPL(num, a, b, c, d, e, f, g, h, i, j, ...) (\
			   (((a) & 0x3) << 4) | \
			   (((b) & 0x3) << 6) | \
			   (((c) & 0x3) << 8) | \
			   (((d) & 0x3) << 10) | \
			   (((e) & 0x3) << 12) | \
			   (((f) & 0x3) << 14) | \
			   (((g) & 0x3) << 16) | \
			   (((h) & 0x3) << 18) | \
			   (((i) & 0x3) << 20) | \
			   (((j) & 0x3) << 22) | \
			   ((num) & 0xf))

#define QCOM_SCM_ARGS(...) QCOM_SCM_ARGS_IMPL(__VA_ARGS__, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

/**
 * struct qcom_scm_desc
 * @arginfo:	Metadata describing the arguments in args[]
 * @args:	The array of arguments for the secure syscall
 * @res:	The values returned by the secure syscall
 */
struct qcom_scm_desc {
	u32 arginfo;
	u64 args[MAX_QCOM_SCM_ARGS];
};

static u64 qcom_smccc_convention = -1;
static DEFINE_MUTEX(qcom_scm_lock);

#define N_EXT_QCOM_SCM_ARGS 7
#define FIRST_EXT_ARG_IDX 3
#define N_REGISTER_ARGS (MAX_QCOM_SCM_ARGS - N_EXT_QCOM_SCM_ARGS + 1)

/* This function is used to find whether TZ is in AARCH64 mode.
 * Here we return true since TZ will be in AARCH64 mode for
 * 64bit Linux.
 */
bool is_scm_armv8(void)
{
	return true;
}

/**
 * qcom_scm_call() - Invoke a syscall in the secure world
 * @dev:	device
 * @owner_id:	owner identifier
 * @svc_id:	service identifier
 * @cmd_id:	command identifier
 * @desc:	Descriptor structure containing arguments and return values
 *
 * Sends a command to the SCM and waits for the command to finish processing.
 * This should *only* be called in pre-emptible context.
*/
static int qcom_scm_call(struct device *dev, u32 owner_id, u32 svc_id,
			 u32 cmd_id, const struct qcom_scm_desc *desc,
			 struct arm_smccc_res *res)
{
	int arglen = desc->arginfo & 0xf;
	int retry_count = 0, i;
	u32 fn_id = QCOM_SCM_FNID(svc_id, cmd_id);
	u64 cmd, x5 = desc->args[FIRST_EXT_ARG_IDX];
	dma_addr_t args_phys = 0;
	void *args_virt = NULL;
	size_t alloc_len;
	struct arm_smccc_quirk quirk = {.id = ARM_SMCCC_QUIRK_QCOM_A6};

	if (unlikely(arglen > N_REGISTER_ARGS)) {
		alloc_len = N_EXT_QCOM_SCM_ARGS * sizeof(u64);
		args_virt = kzalloc(PAGE_ALIGN(alloc_len), GFP_KERNEL);

		if (!args_virt)
			return -ENOMEM;

		if (qcom_smccc_convention == ARM_SMCCC_SMC_32) {
			__le32 *args = args_virt;

			for (i = 0; i < N_EXT_QCOM_SCM_ARGS; i++)
				args[i] = cpu_to_le32(desc->args[i +
						      FIRST_EXT_ARG_IDX]);
		} else {
			__le64 *args = args_virt;

			for (i = 0; i < N_EXT_QCOM_SCM_ARGS; i++)
				args[i] = cpu_to_le64(desc->args[i +
						      FIRST_EXT_ARG_IDX]);
		}

		args_phys = dma_map_single(dev, args_virt, alloc_len,
					   DMA_TO_DEVICE);

		if (dma_mapping_error(dev, args_phys)) {
			kfree(args_virt);
			return -ENOMEM;
		}

		x5 = args_phys;
	}

	do {
		mutex_lock(&qcom_scm_lock);

		cmd = ARM_SMCCC_CALL_VAL(ARM_SMCCC_STD_CALL,
					 qcom_smccc_convention,
					 owner_id, fn_id);

		quirk.state.a6 = 0;

		do {
			arm_smccc_smc_quirk(cmd, desc->arginfo, desc->args[0],
				      desc->args[1], desc->args[2], x5,
				      quirk.state.a6, 0, res, &quirk);

			if (res->a0 == QCOM_SCM_INTERRUPTED)
				cmd = res->a0;

		} while (res->a0 == QCOM_SCM_INTERRUPTED);

		mutex_unlock(&qcom_scm_lock);

		if (res->a0 == QCOM_SCM_V2_EBUSY) {
			if (retry_count++ > QCOM_SCM_EBUSY_MAX_RETRY)
				break;
			msleep(QCOM_SCM_EBUSY_WAIT_MS);
		}
	}  while (res->a0 == QCOM_SCM_V2_EBUSY);

	if (args_virt) {
		dma_unmap_single(dev, args_phys, alloc_len, DMA_TO_DEVICE);
		kfree(args_virt);
	}

	if ((long)res->a0 < 0)
		return qcom_scm_remap_error(res->a0);

	return 0;
}

/**
 * qcom_scm_set_cold_boot_addr() - Set the cold boot address for cpus
 * @entry: Entry point function for the cpus
 * @cpus: The cpumask of cpus that will use the entry point
 *
 * Set the cold boot address of the cpus. Any cpu outside the supported
 * range would be removed from the cpu present mask.
 */
int __qcom_scm_set_cold_boot_addr(void *entry, const cpumask_t *cpus)
{
	return -ENOTSUPP;
}

/**
 * qcom_scm_set_warm_boot_addr() - Set the warm boot address for cpus
 * @dev: Device pointer
 * @entry: Entry point function for the cpus
 * @cpus: The cpumask of cpus that will use the entry point
 *
 * Set the Linux entry point for the SCM to transfer control to when coming
 * out of a power down. CPU power down may be executed on cpuidle or hotplug.
 */
int __qcom_scm_set_warm_boot_addr(struct device *dev, void *entry,
				  const cpumask_t *cpus)
{
	return -ENOTSUPP;
}

/**
 * qcom_scm_cpu_power_down() - Power down the cpu
 * @flags - Flags to flush cache
 *
 * This is an end point to power down cpu. If there was a pending interrupt,
 * the control would return from this function, otherwise, the cpu jumps to the
 * warm boot entry point set for this cpu upon reset.
 */
void __qcom_scm_cpu_power_down(u32 flags)
{
}

int __qcom_remove_xpu_scm_call_available(struct device *dev, u32 svc_id, u32 cmd_id)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.arginfo = QCOM_SCM_ARGS(1);
	desc.args[0] = SCM_QSEEOS_FNID(svc_id, cmd_id) |
			(ARM_SMCCC_OWNER_TRUSTED_OS << ARM_SMCCC_OWNER_SHIFT);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_INFO,
			    QCOM_IS_CALL_AVAIL_CMD, &desc, &res);

	return ret ? : res.a1;
}

int __qcom_scm_is_call_available(struct device *dev, u32 svc_id, u32 cmd_id)
{
	int ret;
	int fn_id;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	fn_id = QCOM_SCM_FNID(svc_id, cmd_id);
	if (cmd_id == QCOM_SCM_IS_TZ_LOG_ENCRYPTED)
		fn_id |= (ARM_SMCCC_OWNER_TRUSTED_OS << ARM_SMCCC_OWNER_SHIFT);
	else
		fn_id |= (ARM_SMCCC_OWNER_SIP << ARM_SMCCC_OWNER_SHIFT);

	desc.arginfo = QCOM_SCM_ARGS(1);
	desc.args[0] = fn_id;

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_INFO,
			    QCOM_IS_CALL_AVAIL_CMD, &desc, &res);

	return ret ? : res.a1;
}

int __qcom_scm_hdcp_req(struct device *dev, struct qcom_scm_hdcp_req *req,
			u32 req_cnt, u32 *resp)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	if (req_cnt > QCOM_SCM_HDCP_MAX_REQ_CNT)
		return -ERANGE;

	desc.args[0] = req[0].addr;
	desc.args[1] = req[0].val;
	desc.args[2] = req[1].addr;
	desc.args[3] = req[1].val;
	desc.args[4] = req[2].addr;
	desc.args[5] = req[2].val;
	desc.args[6] = req[3].addr;
	desc.args[7] = req[3].val;
	desc.args[8] = req[4].addr;
	desc.args[9] = req[4].val;
	desc.arginfo = QCOM_SCM_ARGS(10);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_HDCP,
			    QCOM_SCM_CMD_HDCP, &desc, &res);
	*resp = res.a1;

	return ret;
}

void __qcom_scm_init(void)
{
	u64 cmd;
	struct arm_smccc_res res;
	u32 function = QCOM_SCM_FNID(QCOM_SCM_SVC_INFO, QCOM_IS_CALL_AVAIL_CMD);

	/* First try a SMC64 call */
	cmd = ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_64,
				 ARM_SMCCC_OWNER_SIP, function);

	arm_smccc_smc(cmd, QCOM_SCM_ARGS(1), cmd & (~BIT(ARM_SMCCC_TYPE_SHIFT)),
		      0, 0, 0, 0, 0, &res);

	if (!res.a0 && res.a1)
		qcom_smccc_convention = ARM_SMCCC_SMC_64;
	else
		qcom_smccc_convention = ARM_SMCCC_SMC_32;
}

bool __qcom_scm_pas_supported(struct device *dev, u32 peripheral)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.args[0] = peripheral;
	desc.arginfo = QCOM_SCM_ARGS(1);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_PIL,
			    QCOM_SCM_PAS_IS_SUPPORTED_CMD, &desc, &res);

	return ret ? false : !!res.a1;
}

int __qcom_scm_pas_init_image_v2(struct device *dev, u32 peripheral,
			      dma_addr_t metadata_phys, size_t size)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.args[0] = peripheral;
	desc.args[1] = metadata_phys;
	desc.args[2] = size;
	desc.arginfo = QCOM_SCM_ARGS(3, QCOM_SCM_VAL, QCOM_SCM_RW,
							QCOM_SCM_VAL);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_PIL,
			    QCOM_SCM_PAS_INIT_IMAGE_V2_CMD, &desc, &res);

	return ret ? : res.a1;
}

int __qcom_scm_pas_init_image(struct device *dev, u32 peripheral,
			      dma_addr_t metadata_phys)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.args[0] = peripheral;
	desc.args[1] = metadata_phys;
	desc.arginfo = QCOM_SCM_ARGS(2, QCOM_SCM_VAL, QCOM_SCM_RW);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_PIL,
			    QCOM_SCM_PAS_INIT_IMAGE_CMD, &desc, &res);

	return ret ? : res.a1;
}

int __qcom_scm_pas_mem_setup(struct device *dev, u32 peripheral,
			      phys_addr_t addr, phys_addr_t size)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.args[0] = peripheral;
	desc.args[1] = addr;
	desc.args[2] = size;
	desc.arginfo = QCOM_SCM_ARGS(3);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_PIL,
			    QCOM_SCM_PAS_MEM_SETUP_CMD, &desc, &res);

	return ret ? : res.a1;
}

int __qcom_scm_pas_auth_and_reset(struct device *dev, u32 peripheral,
				u32 debug, u32 reset_cmd_id)
{
	int ret;
	int break_support = 0;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	if (debug) {
		ret = __qcom_scm_is_call_available(dev,
					QCOM_SCM_SVC_PIL,
					reset_cmd_id);
		if (ret)
			break_support = 1;
		else
			dev_err(dev, "breat at start not supported\n");
	}

	if (break_support) {
		desc.args[0] = debug;
		desc.arginfo = QCOM_SCM_ARGS(1);
		ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_PIL,
				    reset_cmd_id, &desc, &res);
		if (ret || res.a1)
			goto end;
	}

	desc.args[0] = peripheral;
	desc.arginfo = QCOM_SCM_ARGS(1);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_PIL,
			    QCOM_SCM_PAS_AUTH_AND_RESET_CMD,
			    &desc, &res);

end:
	return ret ? : res.a1;
}

int __qcom_scm_pas_shutdown(struct device *dev, u32 peripheral)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.args[0] = peripheral;
	desc.arginfo = QCOM_SCM_ARGS(1);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_PIL,
			    QCOM_SCM_PAS_SHUTDOWN_CMD, &desc, &res);

	return ret ? : res.a1;
}

int __qcom_scm_pas_mss_reset(struct device *dev, bool reset)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = reset;
	desc.args[1] = 0;
	desc.arginfo = QCOM_SCM_ARGS(2);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_PIL,
			    QCOM_SCM_PAS_MSS_RESET, &desc, &res);

	return ret ? : res.a1;
}

int __qcom_scm_set_remote_state(struct device *dev, u32 state, u32 id)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = state;
	desc.args[1] = id;
	desc.arginfo = QCOM_SCM_ARGS(2);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_BOOT,
			    QCOM_SCM_SET_REMOTE_STATE, &desc, &res);

	return ret ? : res.a1;
}

int __qcom_scm_assign_mem(struct device *dev, phys_addr_t mem_region,
			  size_t mem_sz, phys_addr_t src, size_t src_sz,
			  phys_addr_t dest, size_t dest_sz)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.args[0] = mem_region;
	desc.args[1] = mem_sz;
	desc.args[2] = src;
	desc.args[3] = src_sz;
	desc.args[4] = dest;
	desc.args[5] = dest_sz;
	desc.args[6] = 0;

	desc.arginfo = QCOM_SCM_ARGS(7, QCOM_SCM_RO, QCOM_SCM_VAL,
				     QCOM_SCM_RO, QCOM_SCM_VAL, QCOM_SCM_RO,
				     QCOM_SCM_VAL, QCOM_SCM_VAL);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_MP,
			    QCOM_MEM_PROT_ASSIGN_ID, &desc, &res);

	return ret ? : res.a1;
}

int __qcom_scm_restore_sec_cfg(struct device *dev, u32 device_id, u32 spare)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = device_id;
	desc.args[1] = spare;
	desc.arginfo = QCOM_SCM_ARGS(2);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_MP,
			    QCOM_SCM_RESTORE_SEC_CFG, &desc, &res);

	return ret ? : res.a1;
}

int __qcom_scm_iommu_secure_ptbl_size(struct device *dev, u32 spare,
				      size_t *size)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = spare;
	desc.arginfo = QCOM_SCM_ARGS(1);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_MP,
			    QCOM_SCM_IOMMU_SECURE_PTBL_SIZE, &desc, &res);

	if (size)
		*size = res.a1;

	return ret ? : res.a2;
}

int __qcom_scm_iommu_secure_ptbl_init(struct device *dev, u64 addr, u32 size,
				      u32 spare)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = addr;
	desc.args[1] = size;
	desc.args[2] = spare;
	desc.arginfo = QCOM_SCM_ARGS(3, QCOM_SCM_RW, QCOM_SCM_VAL,
				     QCOM_SCM_VAL);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_MP,
			    QCOM_SCM_IOMMU_SECURE_PTBL_INIT, &desc, &res);

	/* the pg table has been initialized already, ignore the error */
	if (ret == -EPERM)
		ret = 0;

	return ret;
}

int __qti_scm_wcss_boot(struct device *dev, u32 svc_id, u32 cmd_id,
			 void *cmd_buf)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;
	unsigned int enable;

	enable = cmd_buf ? *((unsigned int *)cmd_buf) : 0;
	desc.args[0] = TCSR_Q6SS_BOOT_TRIG_REG;
	desc.args[1] = enable;
	desc.arginfo = SCM_ARGS(2, QCOM_SCM_VAL, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, SCM_SVC_IO_ACCESS,
			    SCM_IO_WRITE, &desc, &res);

	return ret ? : res.a1;
}

int __qti_scm_pdseg_memcpy_v2(struct device *dev, u32 peripheral,
			int phno, dma_addr_t dma, int seg_cnt)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = peripheral;
	desc.args[1] = phno;
	desc.args[2] = dma;
	desc.args[3] = seg_cnt;

	desc.arginfo = SCM_ARGS(4, QCOM_SCM_VAL, QCOM_SCM_VAL, QCOM_SCM_RW,
								QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, PD_LOAD_SVC_ID,
			PD_LOAD_V2_CMD_ID, &desc, &res);
	return ret ? : res.a1;
}

int __qti_scm_pdseg_memcpy(struct device *dev, u32 peripheral,
			int phno, dma_addr_t dma, size_t size)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = peripheral;
	desc.args[1] = phno;
	desc.args[2] = dma;
	desc.args[3] = size;

	desc.arginfo = SCM_ARGS(4, QCOM_SCM_VAL, QCOM_SCM_VAL, QCOM_SCM_RW,
							QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, PD_LOAD_SVC_ID,
			PD_LOAD_CMD_ID, &desc, &res);
	return ret ? : res.a1;
}

int __qti_scm_int_radio_powerup(struct device *dev, u32 peripheral)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = peripheral;

	desc.arginfo = SCM_ARGS(1, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, PD_LOAD_SVC_ID,
			INT_RAD_PWR_UP_CMD_ID, &desc, &res);
	return ret ? : res.a1;
}

int __qti_scm_int_radio_powerdown(struct device *dev, u32 peripheral)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = peripheral;

	desc.arginfo = SCM_ARGS(1, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, PD_LOAD_SVC_ID,
			INT_RAD_PWR_DN_CMD_ID, &desc, &res);
	return ret ? : res.a1;
}

int __qcom_scm_set_dload_mode(struct device *dev, bool enable)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.args[0] = QCOM_SCM_SET_DLOAD_MODE;
	desc.args[1] = enable ? QCOM_SCM_SET_DLOAD_MODE : 0;
	desc.arginfo = QCOM_SCM_ARGS(2);

	return qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_BOOT,
			     QCOM_SCM_SET_DLOAD_MODE, &desc, &res);
}

int __qcom_scm_io_readl(struct device *dev, phys_addr_t addr,
			unsigned int *val)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = addr;
	desc.arginfo = QCOM_SCM_ARGS(1);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_IO,
			    QCOM_SCM_IO_READ, &desc, &res);
	if (ret >= 0)
		*val = res.a1;

	return ret < 0 ? ret : 0;
}

int __qcom_scm_io_writel(struct device *dev, phys_addr_t addr, unsigned int val)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.args[0] = addr;
	desc.args[1] = val;
	desc.arginfo = QCOM_SCM_ARGS(2);

	return qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_IO,
			     QCOM_SCM_IO_WRITE, &desc, &res);
}

int __qti_qfprom_show_authenticate(struct device *dev, char *buf)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};
	dma_addr_t auth_phys;
	void *auth_buf;

	auth_buf = dma_alloc_coherent(dev, sizeof(*buf),
					&auth_phys, GFP_KERNEL);
	if (!auth_buf) {
		dev_err(dev, "Allocation for auth buffer failed\n");
		return -ENOMEM;
	}
	desc.args[0] = (u64)auth_phys;
	desc.args[1] = sizeof(char);
	desc.arginfo = QCOM_SCM_ARGS(2, QCOM_SCM_RO);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QTI_SCM_SVC_FUSE,
				QTI_QFPROM_IS_AUTHENTICATE_CMD, &desc, &res);
	memcpy(buf, auth_buf, sizeof(char));
	dma_free_coherent(dev, sizeof(*buf), auth_buf, auth_phys);
	return ret ? : res.a1;
}

int __qti_qfprom_write_version(struct device *dev, uint32_t sw_type,
				uint32_t value, uint32_t qfprom_ret_ptr)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QTI_SCM_SVC_FUSE,
			    QTI_QFPROM_ROW_WRITE_CMD, &desc, &res);

	return ret ? : res.a1;
}

int __qti_qfprom_read_version(struct device *dev, uint32_t sw_type,
				uint32_t value, uint32_t qfprom_ret_ptr)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	desc.args[0] = sw_type;
	desc.args[1] = (u64)value;
	desc.args[2] = sizeof(uint32_t);
	desc.args[3] = (u64)qfprom_ret_ptr;
	desc.args[4] = sizeof(uint32_t);

	desc.arginfo = QCOM_SCM_ARGS(5, QCOM_SCM_VAL, QCOM_SCM_RW, QCOM_SCM_VAL,
					QCOM_SCM_RW, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QTI_SCM_SVC_FUSE,
			    QTI_QFPROM_ROW_READ_CMD, &desc, &res);

	return ret ? : res.a1;
}

int __qti_sec_upgrade_auth(struct device *dev, unsigned int scm_cmd_id,
							unsigned int sw_type,
							unsigned int img_size,
							unsigned int load_addr)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	desc.args[0] = sw_type;
	desc.args[1] = img_size;
	desc.args[2] = (u64)load_addr;
	desc.arginfo = QCOM_SCM_ARGS(3, QCOM_SCM_VAL, QCOM_SCM_VAL,
					QCOM_SCM_RW);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_BOOT,
			    scm_cmd_id, &desc, &res);

	return ret ? : res.a1;
}

int __qti_sec_upgrade_auth_meta_data(struct device *dev, unsigned int scm_cmd_id,
							unsigned int sw_type,
							unsigned int img_size,
							unsigned int load_addr,
							void* hash_addr,
							unsigned int hash_size)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};
	dma_addr_t hash_address;

	hash_address = dma_map_single(dev, hash_addr, hash_size, DMA_FROM_DEVICE);

	ret = dma_mapping_error(dev, hash_address);
	if (ret != 0) {
		pr_err("%s: DMA Mapping Error : %d\n", __func__, ret);
		return ret;
	}

	desc.args[0] = sw_type;
	desc.args[1] = (u64)load_addr;
	desc.args[2] = img_size;
	desc.args[3] = hash_address;
	desc.args[4] = hash_size;

	desc.arginfo = QCOM_SCM_ARGS(5, QCOM_SCM_VAL, QCOM_SCM_RW, QCOM_SCM_VAL,
							QCOM_SCM_RW, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_BOOT,
			    scm_cmd_id, &desc, &res);
	dma_unmap_single(dev, hash_address, hash_size, DMA_FROM_DEVICE);

	return ret ? : res.a1;
}

int __qcom_config_ice_sec(struct device *dev, void *conf_buf, int size)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};
	dma_addr_t conf_phys;

	conf_phys = dma_map_single(dev, conf_buf, size, DMA_TO_DEVICE);

	ret = dma_mapping_error(dev, conf_phys);
	if (ret) {
		dev_err(dev, "Allocation fail for conf buffer\n");
		return -ENOMEM;
	}
	desc.arginfo = QCOM_SCM_ARGS(2, QTI_SCM_PARAM_BUF_RO,
						QTI_SCM_PARAM_VAL);
	desc.args[0] = (u64)conf_phys;
	desc.args[1] = size;

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QTI_SVC_ICE,
						QTI_SCM_ICE_CMD, &desc, &res);

	dma_unmap_single(dev, conf_phys, size, DMA_TO_DEVICE);
	return ret ? : res.a1;
}

int __qcom_context_ice_sec(struct device *dev, u32 type, u8 key_size,
			u8 algo_mode, u8 *data_ctxt, u32 data_ctxt_len,
			u8 *salt_ctxt, u32 salt_ctxt_len)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};
	void *data_ctxbuf = NULL, *salt_ctxbuf = NULL;
	dma_addr_t data_context_phy, salt_context_phy = 0;

	data_ctxbuf = dma_alloc_coherent(dev, data_ctxt_len,
				&data_context_phy, GFP_KERNEL);
	if (!data_ctxbuf)
		return -ENOMEM;

	if (algo_mode == ICE_CRYPTO_ALGO_MODE_AES_XTS && salt_ctxt != NULL) {
		salt_ctxbuf = dma_alloc_coherent(dev, salt_ctxt_len,
				&salt_context_phy, GFP_KERNEL);
		if (!salt_ctxbuf) {
			ret = -ENOMEM;
			goto dma_unmap_data_ctxbuf;
		}

		memcpy(salt_ctxbuf, salt_ctxt, salt_ctxt_len);
	}
	if (data_ctxt != NULL) {
		memcpy(data_ctxbuf, data_ctxt, data_ctxt_len);
	}
	else {
		ret = -EINVAL;
		goto dma_unmap_data_ctxbuf;
	}
	desc.arginfo = QCOM_SCM_ARGS(7, QTI_SCM_PARAM_VAL, QTI_SCM_PARAM_VAL,
		QTI_SCM_PARAM_VAL, QTI_SCM_PARAM_BUF_RO, QTI_SCM_PARAM_VAL,
		QTI_SCM_PARAM_BUF_RO, QTI_SCM_PARAM_VAL);

	desc.args[0] = type;
	desc.args[1] = key_size;
	desc.args[2] = algo_mode;
	desc.args[3] = data_context_phy;
	desc.args[4] = data_ctxt_len;
	desc.args[5] = salt_context_phy;
	desc.args[6] = salt_ctxt_len;

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QTI_SVC_ICE,
					QTI_SCM_ICE_CONTEXT_CMD, &desc, &res);

	if (algo_mode == ICE_CRYPTO_ALGO_MODE_AES_XTS && salt_ctxt != NULL) {
		memzero_explicit(salt_ctxt, salt_ctxt_len);
		dma_free_coherent(dev, salt_ctxt_len,
					salt_ctxbuf, salt_context_phy);
	}

dma_unmap_data_ctxbuf:
	memzero_explicit(data_ctxbuf, data_ctxt_len);
	dma_free_coherent(dev, data_ctxt_len, data_ctxbuf, data_context_phy);
	return ret ? : res.a1;

}

int __qti_fuseipq_scm_call(struct device *dev, u32 svc_id, u32 cmd_id,
			    void *cmd_buf, size_t size)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};
	uint64_t *status;
	struct fuse_blow *fuse_blow = cmd_buf;

	desc.args[0] = fuse_blow->address;
	if (fuse_blow->size) {
		desc.args[1] = fuse_blow->size;
		desc.arginfo = QCOM_SCM_ARGS(2, QCOM_SCM_RO, QCOM_SCM_VAL);
	} else {
		desc.arginfo = QCOM_SCM_ARGS(1, QCOM_SCM_RO);
	}

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, svc_id, cmd_id,
			    &desc, &res);

	status = (uint64_t *)fuse_blow->status;
	*status = res.a1;
	return ret;
}

int __qti_scm_dload(struct device *dev, u32 svc_id, u32 cmd_id, void *cmd_buf,
		    u64 dload_mode_addr, void __iomem *dload_reg)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;
	unsigned int enable;
	uint32_t val;

	enable = cmd_buf ? *((unsigned int *)cmd_buf) : 0;

	ret = qti_read_dload_reg(&val);
	if (ret)
		return ret;

	desc.args[0] = dload_mode_addr;
	desc.args[1] = val;
	if (enable == SET_MAGIC_WARMRESET)
		desc.args[1] |= DLOAD_MODE_ENABLE_WARMRESET;
	else if (enable == ABNORMAL_MAGIC)
		desc.args[1] |= DLOAD_MODE_DISABLE_ABNORMALRESET;
	else if (enable == CLEAR_ABNORMAL_MAGIC)
		desc.args[1] &= ~(DLOAD_MODE_DISABLE_ABNORMALRESET);
	else if (enable == SET_MAGIC)
		desc.args[1] |= DLOAD_MODE_ENABLE;
	else if (enable == CLEAR_MAGIC)
		desc.args[1] &= DLOAD_MODE_DISABLE;

	desc.arginfo = QCOM_SCM_ARGS(2, QCOM_SCM_VAL, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_IO,
			    QCOM_SCM_IO_WRITE, &desc, &res);

	return ret ? : res.a1;
}

int __qti_scm_sdi(struct device *dev, u32 svc_id, u32 cmd_id)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = 1ull; /* Disable wdog debug */
	desc.args[1] = 0ull; /* SDI Enable */
	desc.arginfo = QCOM_SCM_ARGS(2, QCOM_SCM_VAL, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_BOOT,
				SCM_CMD_TZ_CONFIG_HW_FOR_RAM_DUMP_ID, &desc,
				&res);

	return ret ? : res.a1;
}

int __qti_scm_qseecom_remove_xpu(struct device *dev)
{
	int ret = 0;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	ret = __qcom_remove_xpu_scm_call_available(dev, QTI_SVC_APP_MGR,
					QTI_ARMv8_CMD_REMOVE_XPU);

	if (ret <= 0)
		return -ENOTSUPP;

	ret = qcom_scm_call(dev, QTI_OWNER_QSEE_OS, QTI_SVC_APP_MGR,
			    QTI_ARMv8_CMD_REMOVE_XPU, &desc, &res);

	return ret;
}

int __qti_scm_qseecom_notify(struct device *dev,
			     struct qsee_notify_app *req, size_t req_size,
			     struct qseecom_command_scm_resp *resp,
			     size_t resp_size)
{
	int ret = 0;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.arginfo = QCOM_SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);
	desc.args[0] = req->applications_region_addr;
	desc.args[1] = req->applications_region_size;

	ret = qcom_scm_call(dev, QTI_OWNER_QSEE_OS, QTI_SVC_APP_MGR,
			   QTI_CMD_NOTIFY_REGION_ID, &desc, &res);

	resp->result = res.a1;
	resp->resp_type = res.a2;
	resp->data = res.a3;

	return ret;
}

int __qti_scm_qseecom_load(struct device *dev, uint32_t smc_id,
			   uint32_t cmd_id, union qseecom_load_ireq *req,
			   size_t req_size,
			   struct qseecom_command_scm_resp *resp,
			   size_t resp_size)
{
	int ret = 0;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.arginfo = QCOM_SCM_ARGS(3, QCOM_SCM_VAL, QCOM_SCM_VAL,
				    QCOM_SCM_VAL);
	desc.args[0] = req->load_lib_req.mdt_len;
	desc.args[1] = req->load_lib_req.img_len;
	desc.args[2] = req->load_lib_req.phy_addr;

	if (cmd_id == QSEOS_APP_START_COMMAND)
		ret = qcom_scm_call(dev, QTI_OWNER_QSEE_OS, QTI_SVC_APP_MGR,
				   QTI_CMD_LOAD_APP_ID, &desc, &res);
	else
		ret = qcom_scm_call(dev, QTI_OWNER_QSEE_OS, QTI_SVC_APP_MGR,
				   QTI_CMD_LOAD_LIB, &desc, &res);

	resp->result = res.a1;
	resp->resp_type = res.a2;
	resp->data = res.a3;

	return ret;
}

int __qti_scm_qseecom_send_data(struct device *dev,
				union qseecom_client_send_data_ireq *req,
				size_t req_size,
				struct qseecom_command_scm_resp *resp,
				size_t resp_size)
{
	int ret = 0;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.arginfo = QCOM_SCM_ARGS(5, QCOM_SCM_VAL, QCOM_SCM_RW, QCOM_SCM_VAL,
				     QCOM_SCM_RW, QCOM_SCM_VAL);
	desc.args[0] = req->v1.app_id;
	desc.args[1] = req->v1.req_ptr;
	desc.args[2] = req->v1.req_len;
	desc.args[3] = req->v1.rsp_ptr;
	desc.args[4] = req->v1.rsp_len;

	ret = qcom_scm_call(dev, QTI_OWNER_TZ_APPS, QTI_SVC_APP_ID_PLACEHOLDER,
			   QTI_CMD_SEND_DATA_ID, &desc, &res);

	resp->result = res.a1;
	resp->resp_type = res.a2;
	resp->data = res.a3;

	return ret;
}

int __qti_scm_qseecom_unload(struct device *dev, uint32_t smc_id,
			     uint32_t cmd_id, struct qseecom_unload_ireq *req,
			     size_t req_size,
			     struct qseecom_command_scm_resp *resp,
			     size_t resp_size)
{
	int ret = 0;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	switch (cmd_id) {
	case QSEOS_APP_SHUTDOWN_COMMAND:
		desc.arginfo = QCOM_SCM_ARGS(1);
		desc.args[0] = req->app_id;
		ret = qcom_scm_call(dev, QTI_OWNER_QSEE_OS,
				   QTI_SVC_APP_MGR,
				   QTI_CMD_UNLOAD_APP_ID,
				   &desc, &res);
		break;

	case QSEE_UNLOAD_SERV_IMAGE_COMMAND:
		ret = qcom_scm_call(dev, QTI_OWNER_QSEE_OS, QTI_SVC_APP_MGR,
				   QTI_CMD_UNLOAD_LIB, &desc, &res);
		break;

	default:
		pr_info("\nIncorrect command id has been passed");
		return -EINVAL;
	}

	resp->result = res.a1;
	resp->resp_type = res.a2;
	resp->data = res.a3;

	return ret;
}

int __qti_scm_register_log_buf(struct device *dev,
				  struct qsee_reg_log_buf_req *request,
				  size_t req_size,
				  struct qseecom_command_scm_resp *response,
				  size_t resp_size)
{
	int ret = 0;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.arginfo = QCOM_SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);
	desc.args[0] = request->phy_addr;
	desc.args[1] = request->len;

	ret = qcom_scm_call(dev, QTI_OWNER_QSEE_OS, QTI_SVC_APP_MGR,
			   QTI_CMD_REGISTER_LOG_BUF, &desc, &res);

	response->result = res.a1;
	response->resp_type = res.a2;
	response->data = res.a3;

	return ret;
}

int __qti_scm_tls_hardening(struct device *dev, uint32_t req_addr,
			    uint32_t req_size, uint32_t resp_addr,
			    uint32_t resp_size, u32 cmd_id)
{
	int ret = 0;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.arginfo = SCM_ARGS(4, QCOM_SCM_RW, QCOM_SCM_VAL,
				QCOM_SCM_RW, QCOM_SCM_VAL);
	desc.args[0] = req_addr;
	desc.args[1] = req_size;
	desc.args[2] = resp_addr;
	desc.args[3] = resp_size;

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QTI_SVC_CRYPTO, cmd_id,
			    &desc, &res);

	if (res.a1 == QCOM_SCM_EINVAL_SIZE) {
		pr_err("%s: TZ does not support data larger than 2K bytes: -%ld\n",
					__func__, res.a1);
	}
	return ret ? : res.a1;
}

int __qti_scm_aes(struct device *dev, uint32_t req_addr,
		  uint32_t req_size, u32 cmd_id)
{
	int ret = 0;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.arginfo = SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);

	desc.args[0] = req_addr;
	desc.args[1] = req_size;

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QTI_SVC_CRYPTO, cmd_id,
			    &desc, &res);

	return res.a1;
}

int __qti_scm_aes_clear_key_handle(struct device *dev, uint32_t key_handle, u32 cmd_id)
{
	int ret = 0;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.arginfo = SCM_ARGS(1);

	desc.args[0] = key_handle;

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QTI_SVC_CRYPTO, cmd_id,
				&desc, &res);

	return ret ? : res.a1;
}

/**
 * __qti_scm_tz_hvc_log() - Get trustzone diag log or hypervisor diag log
 * @svc_id: SCM service id
 * @cmd_id: SCM command id
 * ker_buf: kernel buffer to store the diag log
 * buf_len: kernel buffer length
 *
 * This function can be used to get either the trustzone diag log
 * or the hypervisor diag log based on the command id passed to this
 * function.
 */
int __qti_scm_tz_hvc_log(struct device *dev, u32 svc_id, u32 cmd_id,
			 void *ker_buf, u32 buf_len)
{
	int ret;
	dma_addr_t dma_buf;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	dma_buf = dma_map_single(dev, ker_buf, buf_len, DMA_FROM_DEVICE);

	ret = dma_mapping_error(dev, dma_buf);
	if (ret != 0) {
		pr_err("DMA Mapping Error : %d\n", ret);
		return ret;
	}

	desc.args[0] = dma_buf;
	desc.args[1] = buf_len;
	desc.arginfo = QCOM_SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, svc_id, cmd_id,
			    &desc, &res);

	dma_unmap_single(dev, dma_buf, buf_len, DMA_FROM_DEVICE);

	return ret ? : res.a1;
}

/**
 * __qti_scm_get_ecdsa_blob() - Get the ECDSA blob from TME-L by sending NONCE
 *
 * @svc_id: SCM service id
 * @cmd_id: SCM command id
 * nonce_buf: NONCE buffer which contains the NONCE recieved from Q6.
 * nonce_buf_len: Variable for NONCE buffer length
 * ecdsa_buf: ECDSA buffer, used to receive the ECDSA blob from TME
 * ecdsa_buf_len: Variable which holds the total ECDSA buffer lenght
 * *ecdsa_consumed_len: Pointer to get the consumed ECDSA buffer lenght from TME
 *
 * This function can be used to get the ECDSA blob from TME-L by passing the
 * NONCE through nonce_buf. nonce_buf and ecdsa_buf should be DMA alloc
 * coherent and caller should take care of it.
 */
int __qti_scm_get_ecdsa_blob(struct device *dev, u32 svc_id, u32 cmd_id,
		dma_addr_t nonce_buf, u32 nonce_buf_len, dma_addr_t ecdsa_buf,
		u32 ecdsa_buf_len, u32 *ecdsa_consumed_len)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	dma_addr_t dma_ecdsa_consumed_len;

	dma_ecdsa_consumed_len = dma_map_single(dev, ecdsa_consumed_len,
			sizeof(u32), DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, dma_ecdsa_consumed_len);
	if (ret != 0) {
		pr_err("%s: DMA Mapping Error : %d\n", __func__, ret);
		return ret;
	}

	desc.args[0] = nonce_buf;
	desc.args[1] = nonce_buf_len;
	desc.args[2] = ecdsa_buf;
	desc.args[3] = ecdsa_buf_len;
	desc.args[4] = dma_ecdsa_consumed_len;

	desc.arginfo = SCM_ARGS(5, QCOM_SCM_VAL, QCOM_SCM_VAL,
				QCOM_SCM_VAL, QCOM_SCM_VAL, QCOM_SCM_RW);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, svc_id, cmd_id,
				&desc, &res);

	if (res.a1 != 0)
		pr_err("%s: Response error code is : %x\n", __func__,
					(unsigned int)res.a1);

	dma_unmap_single(dev, dma_ecdsa_consumed_len, sizeof(u32), DMA_FROM_DEVICE);

	return ret ? : res.a1;
}

/**
 * __qti_scm_get_ipq_fuse_list() - Get OEM Fuse parameter from TME-L
 *
 * @svc_id: SCM service id
 * @cmd_id: SCM command id
 * @fuse: QFPROM CORR addresses
 * @size: size of fuse structure
 *
 * This function can be used to get the OEM Fuse parameters from TME-L.
 */
int __qti_scm_get_ipq_fuse_list(struct device *dev, u32 svc_id, u32 cmd_id,
				void *fuse, size_t size)
{
	int ret;
	dma_addr_t dma_fuse;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	dma_fuse = dma_map_single(dev, fuse, size, DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, dma_fuse);
	if (ret != 0) {
		pr_err("DMA Mapping Error : %d\n", ret);
		return -EINVAL;
	}
	desc.args[0] = dma_fuse;
	desc.args[1] = size;

	desc.arginfo = SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, svc_id, cmd_id,
			&desc, &res);

	if(res.a1 != 0) {
		pr_err("%s : Response error code is : %#x\n", __func__,
				(unsigned int)res.a1);
	}

	dma_unmap_single(dev, dma_fuse, size, DMA_FROM_DEVICE);

	return ret ? : res.a1;
}

/**
 * __qti_scm_is_feature_available() - Check if a given feature is enabled by TZ,
 *                   and its version if enabled.
 * @feature_id: ID of the feature to check in TZ for availablilty/version.
 *
 * Return: 0 on success and the version of the feature in result.
 *
 * TZ returns 0xFFFFFFFF if this smc call is not supported or
 * if smc call supported but feature ID not supported
 */
long  __qti_scm_is_feature_available(struct device *dev, u32 svc_id, u32 cmd_id,
							u32 feature_id)
{
	long ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	desc.args[0] = feature_id;
	desc.arginfo = SCM_ARGS(1, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, svc_id, cmd_id,
			&desc, &res);

	return ret ? : res.a1;
}

/**
 * __qti_scm_get_device_attestation_ephimeral_key() - Get M3 public ephimeral key from TME-L
 *
 * @svc_id: SCM service id
 * @cmd_id: SCM command id
 * key_buf: key buffer to store the M3 public ephimeral key and this is populated by TME-L
 * key_buf_len: key buffer length
 * key_len : Size of the M3 Ephimeral public key. This is populated by TME-L after
 *           storing the key in the key buffer.
 *
 * This function can be used to get the M3 public ephimeral key from the TME-L.
 */
int __qti_scm_get_device_attestation_ephimeral_key(struct device *dev,
		u32 svc_id, u32 cmd_id, void *key_buf, u32 key_buf_len,
		u32 *key_len)
{
	int ret;
	dma_addr_t dma_key_buf;
	dma_addr_t dma_key_len;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	dma_key_buf = dma_map_single(dev, key_buf, key_buf_len,
				DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, dma_key_buf);
	if (ret != 0) {
		pr_err("DMA Mapping Error : %d\n", ret);
		return ret;
	}
	dma_key_len = dma_map_single(dev, key_len, sizeof(unsigned int),
				DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, dma_key_len);
	if (ret != 0) {
		pr_err("DMA Mapping Error : %d\n", ret);
		goto dma_unmap_key_buf;
	}
	desc.args[0] = dma_key_buf;
	desc.args[1] = key_buf_len;
	desc.args[2] = dma_key_len;

	desc.arginfo = SCM_ARGS(3, QCOM_SCM_VAL, QCOM_SCM_VAL, QCOM_SCM_RW);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, svc_id, cmd_id,
			&desc, &res);

	if(res.a1 != 0) {
		pr_err("%s : Response error code is : %#x\n", __func__,
					(unsigned int)res.a1);
	}

	dma_unmap_single(dev, dma_key_len, sizeof(unsigned int),
				DMA_FROM_DEVICE);

dma_unmap_key_buf:
	dma_unmap_single(dev, dma_key_buf, key_buf_len, DMA_FROM_DEVICE);

	return ret ? : res.a1;

}

/**
 * __qti_scm_get_device_attestation_response() - Get attestation response from TME-L
 *
 * @svc_id: SCM service id
 * @cmd_id: SCM command id
 * req_buf: attestation request buffer, it contains a attestation request.
 * req_buf_len: attestation request buffer length
 * extclaim_buf: External claim buffer, it also contains attestation request when the
                 attestation request is more than 2KB.
 * extclaim_buf_len: size of external buffer.
 * resp_buf: Response Buffer passed to TME to store the Attestation report response.
 *           TME will used this buffer to populate the Attestation report.
 * resp_buf_len: size of the response buffer.
 * attest_resp_len: Length of the Attestation report response. This is populated by TME
 *                  after storing the attestation response.
 *
 * This function can be used to get the attestation response binary from TME-L by
 * passing the attestation report through req_buf and extclaim_buf.
 */
int __qti_scm_get_device_attestation_response(struct device *dev,
		u32 svc_id, u32 cmd_id, void *req_buf, u32 req_buf_len,
		void *extclaim_buf, u32 extclaim_buf_len, void *resp_buf,
		u32 resp_buf_len, u32 *attest_resp_len)
{
	int ret;
	dma_addr_t dma_req_buf;
	dma_addr_t dma_claim_buf = 0;
	dma_addr_t dma_resp_buf;
	dma_addr_t dma_resp_len;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	dma_req_buf = dma_map_single(dev, req_buf, req_buf_len,
						DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, dma_req_buf);
	if (ret != 0) {
		pr_err("DMA Mapping Error : %d\n", ret);
		return ret;
	}

	if(extclaim_buf != NULL) {
		dma_claim_buf = dma_map_single(dev, extclaim_buf,
				extclaim_buf_len, DMA_FROM_DEVICE);
		ret = dma_mapping_error(dev, dma_claim_buf);
		if (ret != 0) {
			pr_err("DMA Mapping Error : %d\n", ret);
			goto dma_unmap_req_buf;
		}
	}

	dma_resp_buf = dma_map_single(dev, resp_buf, resp_buf_len,
				DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, dma_resp_buf);
	if (ret != 0) {
		pr_err("DMA Mapping Error : %d\n", ret);
		goto dma_unmap_extclaim_buf;
	}

	dma_resp_len = dma_map_single(dev, attest_resp_len,
				sizeof(unsigned int), DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, dma_resp_len);
	if (ret != 0) {
		pr_err("DMA Mapping Error : %d\n", ret);
		goto dma_unmap_resp_buf;
	}

	desc.args[0] = dma_req_buf;
	desc.args[1] = req_buf_len;
	desc.args[2] = dma_claim_buf;
	desc.args[3] = extclaim_buf_len;
	desc.args[4] = dma_resp_buf;
	desc.args[5] = resp_buf_len;
	desc.args[6] = dma_resp_len;

	desc.arginfo = SCM_ARGS(7, QCOM_SCM_VAL, QCOM_SCM_VAL,
				QCOM_SCM_VAL, QCOM_SCM_VAL, QCOM_SCM_VAL,
				QCOM_SCM_VAL, QCOM_SCM_RW);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, svc_id, cmd_id,
			&desc, &res);

	if(res.a1 != 0) {
		pr_err("%s: Response error code is : %x\n", __func__,
					(unsigned int)res.a1);
	}

	dma_unmap_single(dev, dma_resp_len, sizeof(unsigned int),
			DMA_FROM_DEVICE);

dma_unmap_resp_buf:
	dma_unmap_single(dev, dma_resp_buf, resp_buf_len, DMA_FROM_DEVICE);

dma_unmap_extclaim_buf:
	if(extclaim_buf != NULL) {
		dma_unmap_single(dev, dma_claim_buf, extclaim_buf_len,
			DMA_FROM_DEVICE);
	}

dma_unmap_req_buf:
	dma_unmap_single(dev, dma_req_buf, req_buf_len, DMA_FROM_DEVICE);

	return ret ? : res.a1;
}

/**
 *__qti_scm_get_device_provision_response() - Get device provisioning response from TME-L
 *
 * @svc_id: SCM service id
 * @cmd_id: SCM command id
 * provreq_buf: Provsion request buffer, it contains a provision request.
 * provreq_buf_len: Provision request buffer length.
 * provresp_buf: Provision response buffer passed to TME to store the Provision response.
 *           TME will used this buffer to populate the provision response.
 * provresp_buf_len: size allocated to provision response buffer.
 * attest_resp_len: Length of the provision response. This is populated by TME
 *                  after storing the provision response.
 *
 * This function can be used to get the provision response from TME-L by
 * passing the provision report through prov_req.bin file.
 */
int __qti_scm_get_device_provision_response(struct device *dev, u32 svc_id,
                u32 cmd_id, void *provreq_buf, u32 provreq_buf_len,
                void *provresp_buf, u32 provresp_buf_len, u32 *prov_resp_size)
{
	int ret;
	dma_addr_t dma_req_buf;
	dma_addr_t dma_resp_buf;
	dma_addr_t dma_prov_resp_size;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	dma_req_buf = dma_map_single(dev, provreq_buf, provreq_buf_len,
			DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, dma_req_buf);
	if (ret != 0) {
		pr_err("DMA Mapping Error : %d\n", ret);
		return ret;
	}

	dma_resp_buf = dma_map_single(dev, provresp_buf, provresp_buf_len,
			DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, dma_resp_buf);
	if (ret != 0) {
		pr_err("DMA Mapping Error : %d\n", ret);
		goto dma_unmap_req_buf;
	}

	dma_prov_resp_size = dma_map_single(dev, prov_resp_size,
			sizeof(unsigned int), DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, dma_prov_resp_size);
	if (ret != 0) {
		pr_err("DMA Mapping Error : %d\n", ret);
		goto dma_unmap_resp_buf;
	}

	desc.args[0] = dma_req_buf;
	desc.args[1] = provreq_buf_len;
	desc.args[2] = dma_resp_buf;
	desc.args[3] = provresp_buf_len;
	desc.args[4] = dma_prov_resp_size;

	desc.arginfo = SCM_ARGS(5, QCOM_SCM_VAL, QCOM_SCM_VAL,
			QCOM_SCM_VAL, QCOM_SCM_VAL, QCOM_SCM_RW);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, svc_id, cmd_id,
			&desc, &res);

	if(res.a1 != 0) {
		pr_err("%s: Response error code is : %#x\n", __func__,
				(unsigned int)res.a1);
	}

	dma_unmap_single(dev, dma_prov_resp_size, sizeof(unsigned int),
			DMA_FROM_DEVICE);

dma_unmap_resp_buf:
	dma_unmap_single(dev, dma_resp_buf, provresp_buf_len, DMA_FROM_DEVICE);

dma_unmap_req_buf:
	dma_unmap_single(dev, dma_req_buf, provreq_buf_len, DMA_FROM_DEVICE);

	return ret ? : res.a1;
}

int __qti_scm_derive_and_share_key(struct device *dev, u32 svc_id, u32 cmd_id,
		u32 key_len, uint8_t *sw_context, u32 sw_context_len,
		uint8_t *derived_key, uint32_t derived_key_len)
{
	dma_addr_t dma_sw_context_buf = 0;
	dma_addr_t dma_derived_key_buf;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};
	char *sw_context_buf = NULL, *derived_key_buf = NULL;
	int ret = -ENOMEM;

	if (sw_context_len != 0) {
		sw_context_buf = dma_alloc_coherent(dev, PAGE_SIZE,
				&dma_sw_context_buf, GFP_KERNEL);
		if (sw_context_buf == NULL) {
			pr_err("DMA Allocation failed for sw_context_buf\n");
			return ret;
		}
		memcpy(sw_context_buf, sw_context, sw_context_len);
	}

	derived_key_buf = dma_alloc_coherent(dev, PAGE_SIZE,
					     &dma_derived_key_buf, GFP_KERNEL);
	if (derived_key_buf == NULL) {
		pr_err("DMA Allocation failed for derived_key_buf\n");
		goto dma_unmap_sw_context_buf;
	}

	desc.args[0] = key_len;
	desc.args[1] = dma_sw_context_buf;
	desc.args[2] = sw_context_len;
	desc.args[3] = dma_derived_key_buf;
	desc.args[4] = derived_key_len;

	desc.arginfo = SCM_ARGS(5, QCOM_SCM_VAL, QCOM_SCM_RO,
			QCOM_SCM_VAL, QCOM_SCM_RW, QCOM_SCM_VAL);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, svc_id, cmd_id,
			&desc, &res);

	if(res.a1 != 0) {
		pr_err("%s: Response error code is : 0x%x\n", __func__,
				(unsigned int)res.a1);
	}

	memcpy(derived_key, derived_key_buf, derived_key_len);
	dma_free_coherent(dev, PAGE_SIZE, derived_key_buf,
			  dma_derived_key_buf);

dma_unmap_sw_context_buf:
	if (sw_context_len != 0) {
		dma_free_coherent(dev, PAGE_SIZE, sw_context_buf,
				  dma_sw_context_buf);
	}

	return ret ? : res.a1;
}

/**
 * __qti_scm_get_smmustate () - Get SMMU state
 * @svc_id: SCM service id
 * @cmd_id: SCM command id
 *
 * Returns 0 - SMMU_DISABLE_NONE
 *	   1 - SMMU_DISABLE_S2
 *	   2 - SMMU_DISABLE_ALL on success.
 *	  -1 - Failure
 */
int __qti_scm_get_smmustate(struct device *dev, u32 svc_id, u32 cmd_id)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.arginfo = QCOM_SCM_ARGS(0);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, svc_id, cmd_id,
			    &desc, &res);

	return ret ? -1 : res.a1;
}

int __qti_scm_regsave(struct device *dev, u32 svc_id, u32 cmd_id,
				void *scm_regsave, unsigned int buf_size)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = (u64)virt_to_phys(scm_regsave);
	desc.args[1] = buf_size;
	desc.arginfo = QCOM_SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, svc_id, cmd_id,
			    &desc, &res);
	return ret ? : res.a1;
}

int __qti_set_qcekey_sec(struct device *dev, void *confBuf, int size)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};
	dma_addr_t conf_phys;

	conf_phys = dma_map_single(dev, confBuf, size, DMA_TO_DEVICE);

	ret = dma_mapping_error(dev, conf_phys);
	if (ret) {
		dev_err(dev, "Allocation fail for conf buffer\n");
		return -ENOMEM;
	}
	desc.arginfo = SCM_ARGS(1, QCOM_SCM_RO);
	desc.args[0] = (u64)conf_phys;
	desc.args[1] = size;

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_QCE_CRYPTO_SIP,
			QCOM_SCM_QCE_CMD, &desc, &res);

	dma_unmap_single(dev, conf_phys, size, DMA_TO_DEVICE);
	return ret ? : res.a1;
}

int __qti_sec_crypt(struct device *dev, void *confBuf, int size)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};
	dma_addr_t conf_phys;

	conf_phys = dma_map_single(dev, confBuf, size, DMA_TO_DEVICE);

	ret = dma_mapping_error(dev, conf_phys);
	if (ret) {
		dev_err(dev, "Allocation fail for conf buffer\n");
		return -ENOMEM;
	}
	desc.arginfo = SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);
	desc.args[0] = (u64)conf_phys;
	desc.args[1] = size;

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_QCE_CRYPTO_SIP,
			QCOM_SCM_QCE_ENC_DEC_CMD, &desc, &res);

	dma_unmap_single(dev, conf_phys, size, DMA_TO_DEVICE);
	return ret ? : res.a1;
}

int __qti_qcekey_release_xpu_prot(struct device *dev)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	desc.arginfo = SCM_ARGS(0, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_QCE_CRYPTO_SIP,
			QCOM_SCM_QCE_UNLOCK_CMD, &desc, &res);

	return ret ? : res.a1;
}

int __qti_seccrypt_clearkey(struct device *dev)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	desc.arginfo = SCM_ARGS(0, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_QCE_CRYPTO_SIP,
			QCOM_SCM_SECCRYPT_CLRKEY_CMD, &desc, &res);

	return ret ? : res.a1;
}

int __qti_scm_set_resettype(struct device *dev, u32 reset_type)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.args[0] = reset_type;
	desc.arginfo = SCM_ARGS(1);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_BOOT,
			    QTI_SCM_SVC_RESETTYPE_CMD, &desc, &res);

	return ret ? false : !!res.a1;

}

int __qti_scm_pshold(struct device *dev)
{
	return -ENOTSUPP;
}

int __qti_scm_extwdt(struct device *dev, u32 svc_id, u32 cmd_id,
			unsigned int regaddr, unsigned int val)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = (u64)regaddr;
	desc.args[1] = val;
	desc.arginfo = SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_IO,
			    QTI_SCM_EXTWDT_CMD, &desc, &res);

	return ret ? : res.a1;
}

int __qti_scm_tcsr_reg_write(struct device *dev, u32 reg_addr, u32 value)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = reg_addr;
	desc.args[1] = value;
	desc.arginfo = SCM_ARGS(2, QCOM_SCM_VAL, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_IO,
			    QCOM_SCM_IO_WRITE, &desc, &res);

	return ret ? : res.a1;
}

int __qti_scm_is_tz_log_encrypted(struct device *dev)
{
	int ret;
	struct arm_smccc_res res;
	struct qcom_scm_desc desc = {0};

	desc.arginfo = SCM_ARGS(0);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_TRUSTED_OS, QTI_SVC_APP_MGR,
			    QCOM_SCM_IS_TZ_LOG_ENCRYPTED, &desc, &res);

	return ret ? : res.a1;
}

int __qti_scm_get_encrypted_tz_log(struct device *dev, void *ker_buf,
				   u32 buf_len, u32 log_id)
{
	int ret;
	dma_addr_t log_buf;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	log_buf = dma_map_single(dev, ker_buf, buf_len, DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, log_buf);

	if (ret) {
		dev_err(dev, "DMA Mapping error: %d\n", ret);
		return ret;
	}

	desc.args[0] = log_buf;
	desc.args[1] = buf_len;
	desc.args[2] = log_id;
	desc.arginfo = SCM_ARGS(3, QCOM_SCM_RW, QCOM_SCM_VAL, QCOM_SCM_VAL);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_TRUSTED_OS, QTI_SVC_APP_MGR,
		            QCOM_SCM_GET_TZ_LOG_ENCRYPTED, &desc, &res);

	dma_unmap_single(dev, log_buf, buf_len, DMA_FROM_DEVICE);
	return ret ? : res.a1;
}

int __qti_scm_load_otp(struct device *dev, u32 peripheral)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.args[0] = peripheral;
	desc.arginfo = QCOM_SCM_ARGS(1);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QTI_SCM_SVC_OTP,
			    QTI_SCM_CMD_OTP, &desc, &res);

	return ret ? false : !!res.a1;
}

int __qti_scm_pil_cfg(struct device *dev, u32 peripheral, u32 arg)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.args[0] = peripheral;
	desc.args[1] = arg;
	desc.arginfo = QCOM_SCM_ARGS(2);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QTI_SCM_SVC_XO_TCXO,
			    QTI_SCM_CMD_XO_TCXO, &desc, &res);

	return ret ? false : !!res.a1;
}

int __qti_scm_toggle_bt_eco(struct device *dev, u32 peripheral, u32 arg)
{
	int ret;
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;

	desc.args[0] = peripheral;
	desc.args[1] = arg;
	desc.arginfo = QCOM_SCM_ARGS(2);

	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QTI_SCM_SVC_BT_ECO,
			    QTI_SCM_CMD_BT_ECO, &desc, &res);

	return ret ? false : !!res.a1;
}

int __qti_scm_set_trybit(struct device *dev, u32 svc_id, u32 val, u64 dload_mode_addr)
{
	struct qcom_scm_desc desc = {0};
	struct arm_smccc_res res;
	int ret;

	desc.args[0] = dload_mode_addr;
	desc.args[1] = val;

	desc.arginfo = QCOM_SCM_ARGS(2, QCOM_SCM_VAL, QCOM_SCM_VAL);
	ret = qcom_scm_call(dev, ARM_SMCCC_OWNER_SIP, QCOM_SCM_SVC_IO,
				QCOM_SCM_IO_WRITE, &desc, &res);

	return ret ? : res.a1;
}
