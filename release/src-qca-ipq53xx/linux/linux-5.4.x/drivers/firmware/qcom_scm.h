/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (c) 2010-2015, The Linux Foundation. All rights reserved.
 */
#ifndef __QCOM_SCM_INT_H
#define __QCOM_SCM_INT_H

#define QCOM_SCM_SVC_BOOT		0x1
#define QCOM_SCM_BOOT_ADDR		0x1
#define QCOM_SCM_SET_DLOAD_MODE		0x10
#define QCOM_SCM_BOOT_ADDR_MC		0x11
#define QCOM_SCM_SET_REMOTE_STATE	0xa
#define QCOM_SCM_IS_TZ_LOG_ENCRYPTED	0xb
#define QCOM_SCM_GET_TZ_LOG_ENCRYPTED	0xc
extern int __qcom_scm_set_remote_state(struct device *dev, u32 state, u32 id);
extern int __qcom_scm_set_dload_mode(struct device *dev, bool enable);

#define QCOM_SCM_FLAG_HLOS		0x01
#define QCOM_SCM_FLAG_COLDBOOT_MC	0x02
#define QCOM_SCM_FLAG_WARMBOOT_MC	0x04
extern int __qcom_scm_set_warm_boot_addr(struct device *dev, void *entry,
		const cpumask_t *cpus);
extern int __qcom_scm_set_cold_boot_addr(void *entry, const cpumask_t *cpus);

#define QCOM_SCM_CMD_TERMINATE_PC	0x2
#define QCOM_SCM_FLUSH_FLAG_MASK	0x3
#define QCOM_SCM_CMD_CORE_HOTPLUGGED	0x10
extern void __qcom_scm_cpu_power_down(u32 flags);

#define QCOM_DLOAD_READ_MODE		0x6
#define QCOM_DLOAD_SEC_READ		0x401000

#define QCOM_SCM_SVC_IO			0x5
#define QCOM_SCM_IO_READ		0x1
#define QCOM_SCM_IO_WRITE		0x2
extern int __qcom_scm_io_readl(struct device *dev, phys_addr_t addr, unsigned int *val);
extern int __qcom_scm_io_writel(struct device *dev, phys_addr_t addr, unsigned int val);

#define QCOM_SCM_SVC_INFO		0x6
#define QCOM_IS_CALL_AVAIL_CMD		0x1
#define QTI_SCM_TZ_DIAG_CMD		0x2
#define QTI_SCM_HVC_DIAG_CMD		0x7
#define QTI_SCM_SMMUSTATE_CMD		0x19

extern int __qcom_scm_is_call_available(struct device *dev, u32 svc_id,
		u32 cmd_id);
extern int __qcom_remove_xpu_scm_call_available(struct device *dev, u32 svc_id,
		u32 cmd_id);

#define QCOM_SCM_SVC_HDCP		0x11
#define QCOM_SCM_CMD_HDCP		0x01
extern int __qcom_scm_hdcp_req(struct device *dev,
		struct qcom_scm_hdcp_req *req, u32 req_cnt, u32 *resp);

extern void __qcom_scm_init(void);

#define QCOM_SCM_SVC_PIL		0x2
#define QCOM_SCM_PAS_INIT_IMAGE_CMD	0x1
#define QCOM_SCM_PAS_INIT_IMAGE_V2_CMD	0x1a
#define QCOM_SCM_PAS_MEM_SETUP_CMD	0x2
#define QCOM_SCM_PAS_AUTH_AND_RESET_CMD	0x5
#define QCOM_SCM_PAS_SHUTDOWN_CMD	0x6
#define QCOM_SCM_PAS_IS_SUPPORTED_CMD	0x7
#define QCOM_SCM_PAS_MSS_RESET		0xa

#define QTI_SCM_PARAM_BUF_RO		0x1
#define QTI_SCM_PARAM_VAL		0x0
#define QTI_SVC_ICE			23
#define QTI_SCM_ICE_CMD		0x1
#define QTI_SCM_ICE_CONTEXT_CMD		0x3

extern bool __qcom_scm_pas_supported(struct device *dev, u32 peripheral);
extern int  __qcom_scm_pas_init_image(struct device *dev, u32 peripheral,
		dma_addr_t metadata_phys);
extern int  __qcom_scm_pas_init_image_v2(struct device *dev, u32 peripheral,
		dma_addr_t metadata_phys, size_t size);
extern int  __qcom_scm_pas_mem_setup(struct device *dev, u32 peripheral,
		phys_addr_t addr, phys_addr_t size);
extern int  __qcom_scm_pas_auth_and_reset(struct device *dev, u32 peripheral,
		u32 debug, u32 reset_cmd_id);
extern int  __qcom_scm_pas_shutdown(struct device *dev, u32 peripheral);
extern int  __qcom_scm_pas_mss_reset(struct device *dev, bool reset);
extern int  __qcom_config_ice_sec(struct device *dev, void *confBuf, int size);
extern int  __qcom_context_ice_sec(struct device *dev, u32 type, u8 key_size,
		u8 algo_mode, u8 *data_ctxt, u32 data_ctxt_len,
		u8 *salt_ctxt, u32 salt_ctxt_len);
/* common error codes */
#define QCOM_SCM_V2_EBUSY	-12
#define QCOM_SCM_ENOMEM		-5
#define QCOM_SCM_EOPNOTSUPP	-4
#define QCOM_SCM_EINVAL_ADDR	-3
#define QCOM_SCM_EINVAL_ARG	-2
#define QCOM_SCM_ERROR		-1
#define QCOM_SCM_INTERRUPTED	1
#define QCOM_SCM_EINVAL_SIZE	18

static inline int qcom_scm_remap_error(int err)
{
	switch (err) {
	case QCOM_SCM_ERROR:
		return -EIO;
	case QCOM_SCM_EINVAL_ADDR:
	case QCOM_SCM_EINVAL_ARG:
		return -EINVAL;
	case QCOM_SCM_EOPNOTSUPP:
		return -EOPNOTSUPP;
	case QCOM_SCM_ENOMEM:
		return -ENOMEM;
	case QCOM_SCM_V2_EBUSY:
		return -EBUSY;
	}
	return -EINVAL;
}

#define QCOM_SCM_SVC_MP			0xc
#define QCOM_SCM_RESTORE_SEC_CFG	2
extern int __qcom_scm_restore_sec_cfg(struct device *dev, u32 device_id,
				      u32 spare);
#define QCOM_SCM_IOMMU_SECURE_PTBL_SIZE	3
#define QCOM_SCM_IOMMU_SECURE_PTBL_INIT	4
extern int __qcom_scm_iommu_secure_ptbl_size(struct device *dev, u32 spare,
					     size_t *size);
extern int __qcom_scm_iommu_secure_ptbl_init(struct device *dev, u64 addr,
					     u32 size, u32 spare);
#define QCOM_MEM_PROT_ASSIGN_ID	0x16
extern int  __qcom_scm_assign_mem(struct device *dev,
				  phys_addr_t mem_region, size_t mem_sz,
				  phys_addr_t src, size_t src_sz,
				  phys_addr_t dest, size_t dest_sz);

#define SCM_SIP_FNID(s, c) (((((s) & 0xFF) << 8) | ((c) & 0xFF)) | 0x02000000)
#define QTI_SMC_ATOMIC_MASK		0x80000000
#define SCM_QSEEOS_FNID(s, c) (((((s) & 0xFF) << 8) | ((c) & 0xFF)) | \
			      0x32000000)
#define SCM_ARGS_IMPL(num, a, b, c, d, e, f, g, h, i, j, ...) (\
			(((a) & 0xff) << 4) | \
			(((b) & 0xff) << 6) | \
			(((c) & 0xff) << 8) | \
			(((d) & 0xff) << 10) | \
			(((e) & 0xff) << 12) | \
			(((f) & 0xff) << 14) | \
			(((g) & 0xff) << 16) | \
			(((h) & 0xff) << 18) | \
			(((i) & 0xff) << 20) | \
			(((j) & 0xff) << 22) | \
			(num & 0xffff))

#define SCM_ARGS(...) SCM_ARGS_IMPL(__VA_ARGS__, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

#define MAX_QCOM_SCM_ARGS 10
#define MAX_QCOM_SCM_RETS 3

enum qcom_scm_arg_types {
	QCOM_SCM_VAL,
	QCOM_SCM_RO,
	QCOM_SCM_RW,
	QCOM_SCM_BUFVAL,
};


/**
 * struct scm_desc
 * @arginfo: Metadata describing the arguments in args[]
 * @args: The array of arguments for the secure syscall
 * @ret: The values returned by the secure syscall
 * @extra_arg_buf: The buffer containing extra arguments
		   (that don't fit in available registers)
 * @x5: The 4rd argument to the secure syscall or physical address of
	extra_arg_buf
 */
struct scm_desc {
	u32 arginfo;
	u64 args[MAX_QCOM_SCM_ARGS];
	u64 ret[MAX_QCOM_SCM_RETS];

	/* private */
	void *extra_arg_buf;
	u64 x5;
};

#define QCOM_SCM_EBUSY_WAIT_MS 30
#define QCOM_SCM_EBUSY_MAX_RETRY 20

#define QTI_SCM_SVC_FUSE		0x8
#define QTI_SCM_SVC_SEC_AUTH		0x1
#define QTI_QFPROM_IS_AUTHENTICATE_CMD	0x7
#define QTI_QFPROM_ROW_READ_CMD	0x8
#define QTI_QFPROM_ROW_WRITE_CMD	0x9
extern int __qti_qfprom_show_authenticate(struct device *dev, char *buf);
extern int __qti_qfprom_write_version(struct device *dev, uint32_t sw_type,
					uint32_t value,
					uint32_t qfprom_ret_ptr);
extern int __qti_qfprom_read_version(struct device *dev, uint32_t sw_type,
					uint32_t value,
					uint32_t qfprom_ret_ptr);
extern int __qti_sec_upgrade_auth(struct device *dev, unsigned int scm_cmd_id,
							unsigned int sw_type,
							unsigned int img_size,
							unsigned int load_addr);
extern int __qti_sec_upgrade_auth_meta_data(struct device *dev, unsigned int scm_cmd_id,
							unsigned int sw_type,
							unsigned int img_size,
							unsigned int load_addr,
							void* hash_addr,
							unsigned int hash_size);
extern int __qti_fuseipq_scm_call(struct device *dev, u32 svc_id, u32 cmd_id,
					void *cmd_buf, size_t size);

/*
 * SCM command ids used in qti_scm_dload / qti_scm_sdi calls
 *
 * SCM_CMD_TZ_FORCE_DLOAD_ID - Set the magic cookie(See Below)
 * SCM_CMD_TZ_CONFIG_HW_FOR_RAM_DUMP_ID - Used for SDI related calls
 * SCM_CMD_TZ_SET_DLOAD_FOR_SECURE_BOOT - Set the magic cookie in secure boot
 */
#define SCM_CMD_TZ_FORCE_DLOAD_ID		0x10
#define SCM_CMD_TZ_CONFIG_HW_FOR_RAM_DUMP_ID	0x9
#define SCM_CMD_TZ_SET_DLOAD_FOR_SECURE_BOOT	0x14

/*
 * Based on the magic cookies CLEAR_MAGIC, SET_MAGIC, SET_MAGIC_WARMRESET,
 * corresponding value DLOAD_MODE_DISABLE, DLOAD_MODE_ENABLE,
 * DLOAD_MODE_ENABLE_WARMRESET will be written into the TCSR register.
 *
 * SET_MAGIC_WARMRESET is a unique case, where IMEM content will be preserved
 * in the crash dump disabled case.
 */
#define CLEAR_MAGIC				0x0
#define SET_MAGIC				0x1
#define SET_MAGIC_WARMRESET			0x2
#define ABNORMAL_MAGIC				0x3
#define CLEAR_ABNORMAL_MAGIC		        0x4

#define DLOAD_MODE_DISABLE			(~BIT(4))
#define DLOAD_MODE_ENABLE			BIT(4)
#define DLOAD_MODE_ENABLE_WARMRESET		BIT(5)
#define DLOAD_MODE_DISABLE_ABNORMALRESET	BIT(6)

#define TCSR_Q6SS_BOOT_TRIG_REG			0x193d204ull

#define PD_LOAD_SVC_ID          0x2
#define PD_LOAD_CMD_ID          0x16
#define PD_LOAD_V2_CMD_ID       0x19
#define INT_RAD_PWR_UP_CMD_ID   0x17
#define INT_RAD_PWR_DN_CMD_ID   0x18
extern int __qti_scm_wcss_boot(struct device *, u32 svc_id, u32 cmd_id,
				void *cmd_buf);
extern int qti_scm_wcss_boot(u32 svc_id, u32 cmd_id, void *cmd_buf);
extern int __qti_scm_dload(struct device *dev, u32 svc_id, u32 cmd_id,
				void *cmd_buf, u64 dload_mode_addr,
				void __iomem *dload_reg);
extern int __qti_scm_pdseg_memcpy_v2(struct device *dev, u32 peripheral,
				int phno, dma_addr_t dma, int seg_cnt);
extern int qti_scm_pdseg_memcpy_v2(u32 peripheral, int phno, dma_addr_t dma,
							int seg_cnt);
extern int __qti_scm_pdseg_memcpy(struct device *dev, u32 peripheral,
				int phno, dma_addr_t dma, size_t size);
extern int qti_scm_pdseg_memcpy(u32 peripheral, int phno, dma_addr_t dma,
								size_t size);
extern int __qti_scm_int_radio_powerup(struct device *dev, u32 peripheral);
extern int qti_scm_int_radio_powerup(u32 peripheral);
extern int __qti_scm_int_radio_powerdown(struct device *dev, u32 peripheral);
extern int qti_scm_int_radio_powerdown(u32 peripheral);

extern int __qti_scm_sdi(struct device *dev, u32 svc_id, u32 cmd_id);
extern int __qti_scm_tz_hvc_log(struct device *dev, u32 svc_id, u32 cmd_id,
				void *ker_buf, u32 buf_len);
extern int  __qti_scm_get_smmustate(struct device *dev, u32 svc_id, u32 cmd_id);

int __qti_scm_regsave(struct device *dev, u32 svc_id, u32 cmd_id,
				void *scm_regsave, unsigned int buf_size);

/*
 * QCOM_SCM_QCE_SVC - commands related to secure key for secure nand
 */
#define QCOM_SCM_QCE_SVC		0x2
#define QCOM_SCM_QCE_CMD		0x3
#define QCOM_SCM_QCE_ENC_DEC_CMD	0xB
#define QCOM_SCM_QCE_PARAM		0x2
#define QCOM_SCM_QCE_CRYPTO_SIP		0xA
#define QCOM_SCM_QCE_UNLOCK_CMD		0x4
#define QCOM_SCM_SECCRYPT_CLRKEY_CMD	0xC
extern int __qti_set_qcekey_sec(struct device *dev, void *confBuf, int size);
extern int __qti_sec_crypt(struct device *dev, void *confBuf, int size);
extern int __qti_qcekey_release_xpu_prot(struct device *dev);
extern int __qti_seccrypt_clearkey(struct device *dev);
extern int __qti_scm_qseecom_remove_xpu(struct device *);
extern int __qti_scm_qseecom_notify(struct device *dev,
				    struct qsee_notify_app *req,
				    size_t req_size,
				    struct qseecom_command_scm_resp *resp,
				    size_t resp_size);
extern int __qti_scm_qseecom_load(struct device *dev,
				  uint32_t smc_id, uint32_t cmd_id,
				  union qseecom_load_ireq *req,
				  size_t req_size,
				  struct qseecom_command_scm_resp *resp,
				  size_t resp_size);
extern int __qti_scm_qseecom_send_data(struct device *dev,
				       union qseecom_client_send_data_ireq *req,
				       size_t req_size,
				       struct qseecom_command_scm_resp *resp,
				       size_t resp_size);
extern int __qti_scm_qseecom_unload(struct device *dev,
				    uint32_t smc_id, uint32_t cmd_id,
				    struct qseecom_unload_ireq *req,
				    size_t req_size,
				    struct qseecom_command_scm_resp *resp,
				    size_t resp_size);
extern int __qti_scm_register_log_buf(struct device *dev,
					 struct qsee_reg_log_buf_req *request,
					 size_t req_size,
					 struct qseecom_command_scm_resp
					 *response, size_t resp_size);
extern int __qti_scm_tls_hardening(struct device *dev, uint32_t req_addr,
				   uint32_t req_size, uint32_t resp_addr,
				   uint32_t resp_size, u32 cmd_id);
extern int __qti_scm_get_ipq_fuse_list(struct device *dev, u32 svc_id,
				       u32 cmd_id, void *, size_t size);
extern int __qti_scm_aes(struct device *dev, uint32_t req_addr,
			 uint32_t req_size, u32 cmd_id);

extern int __qti_scm_aes_clear_key_handle(struct device *dev, uint32_t key_handle, u32 cmd_id);

#define QTI_SCM_SVC_RESETTYPE_CMD	0x18
extern int  __qti_scm_set_resettype(struct device *dev, u32 reset_type);

#define QTI_SCM_CMD_PSHOLD	0x16
extern int __qti_scm_pshold(struct device *);
#endif
#define QTI_SCM_EXTWDT_CMD            0x2
extern int __qti_scm_extwdt(struct device *, u32 svc_id, u32 cmd_id,
			     unsigned int regaddr, unsigned int val);
extern int __qti_scm_tcsr_reg_write(struct device *dev, u32 reg_addr, u32 value);

extern int __qti_scm_is_tz_log_encrypted(struct device *dev);
extern int __qti_scm_get_encrypted_tz_log(struct device *dev, void *ker_buf,
					  u32 buf_len, u32 log_id);

#define QTI_SCM_SVC_OTP	0x2
#define QTI_SCM_CMD_OTP	0x15
extern int  __qti_scm_load_otp(struct device *dev, u32 peripheral);

#define QTI_SCM_SVC_XO_TCXO 0x2
#define QTI_SCM_CMD_XO_TCXO 0x20
extern int __qti_scm_pil_cfg(struct device *dev, u32 peripheral, u32 arg);

#define QTI_SCM_SVC_BT_ECO 0x2
#define QTI_SCM_CMD_BT_ECO 0x21

extern int __qti_scm_toggle_bt_eco(struct device *dev, u32 peripheral,
							u32 arg);
