/* QTI Secure Execution Environment Communicator (QSEECOM) driver
 *
 * Copyright (c) 2012, 2015, 2017-2018 The Linux Foundation. All rights reserved.
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

/* Refer to Documentation/qseecom.txt for detailed usage instructions.
 */

#include "qseecom.h"

struct qseecom_props {
	const int function;
	bool libraries_inbuilt;
	bool logging_support_enabled;
	bool aes_v2;
};

const struct qseecom_props qseecom_props_ipq807x = {
	.function = (MUL | CRYPTO | AES_SEC_KEY | RSA_SEC_KEY | LOG_BITMASK |
					FUSE | MISC | AES_TZAPP | RSA_TZAPP |
					FUSE_WRITE),
	.libraries_inbuilt = false,
	.logging_support_enabled = true,
	.aes_v2 = false,
};

const struct qseecom_props qseecom_props_ipq6018 = {
	.function = (MUL | CRYPTO | AES_SEC_KEY | RSA_SEC_KEY),
	.libraries_inbuilt = false,
	.logging_support_enabled = true,
	.aes_v2 = false,
};

const struct qseecom_props qseecom_props_ipq5018 = {
	.function = (MUL | CRYPTO | AES_SEC_KEY | RSA_SEC_KEY | LOG_BITMASK |
					FUSE | MISC | RSA_TZAPP | FUSE_WRITE),
	.libraries_inbuilt = false,
	.logging_support_enabled = true,
	.aes_v2 = false,
};

const struct qseecom_props qseecom_props_ipq9574 = {
	.function = (MUL | CRYPTO | AES_SEC_KEY | RSA_SEC_KEY | LOG_BITMASK |
					FUSE | MISC | AES_TZAPP | RSA_TZAPP |
					FUSE_WRITE),
	.libraries_inbuilt = false,
	.logging_support_enabled = true,
	.aes_v2 = true,
};

static const struct of_device_id qseecom_of_table[] = {
	{	.compatible = "ipq807x-qseecom",
		.data = (void *) &qseecom_props_ipq807x,
	},
	{	.compatible = "ipq6018-qseecom",
		.data = (void *) &qseecom_props_ipq6018,
	},
	{	.compatible = "ipq5018-qseecom",
		.data = (void *) &qseecom_props_ipq5018,
	},
	{	.compatible = "ipq9574-qseecom",
		.data = (void *) &qseecom_props_ipq9574,
	},
	{}
};
MODULE_DEVICE_TABLE(of, qseecom_of_table);

static int unload_app_libs(void)
{
	struct qseecom_unload_ireq req;
	struct qseecom_command_scm_resp resp;
	int ret = 0;
	uint32_t cmd_id = 0;
	uint32_t smc_id = 0;

	cmd_id = QSEE_UNLOAD_SERV_IMAGE_COMMAND;

	smc_id = QTI_SYSCALL_CREATE_SMC_ID(QTI_OWNER_QSEE_OS, QTI_SVC_APP_MGR,
					   QTI_CMD_UNLOAD_LIB);

	/* SCM_CALL to unload the app */
	ret = qti_scm_qseecom_unload(smc_id, cmd_id, &req, sizeof(uint32_t),
				     &resp, sizeof(resp));

	if (ret) {
		pr_err("scm_call to unload app libs failed, ret val = %d\n",
		      ret);
		return ret;
	}

	pr_info("App libs unloaded successfully\n");

	return 0;
}

static int qtidbg_register_qsee_log_buf(struct device *dev)
{
	uint64_t len = 0;
	int ret = 0;
	void *buf = NULL;
	struct qsee_reg_log_buf_req req;
	struct qseecom_command_scm_resp resp;
	dma_addr_t dma_log_buf = 0;

	len = QSEE_LOG_BUF_SIZE;
	buf = dma_alloc_coherent(dev, len, &dma_log_buf, GFP_KERNEL);
	if (buf == NULL) {
		pr_err("Failed to alloc memory for size %llu\n", len);
		return -ENOMEM;
	}
	g_qsee_log = (struct qtidbg_log_t *)buf;

	req.phy_addr = dma_log_buf;
	req.len = len;

	ret = qti_scm_register_log_buf(dev, &req, sizeof(req),
					  &resp, sizeof(resp));

	if (ret) {
		pr_err("SCM Call failed..SCM Call return value = %d\n", ret);
		dma_free_coherent(dev, len, (void *)g_qsee_log, dma_log_buf);
		return ret;
	}

	if (resp.result) {
		ret = resp.result;
		pr_err("Response status failure..return value = %d\n", ret);
		dma_free_coherent(dev, len, (void *)g_qsee_log, dma_log_buf);
		return ret;
	}

	return 0;
}

static ssize_t
show_qsee_app_log_buf(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	ssize_t count = 0;

	if (app_state) {
		if (g_qsee_log->log_pos.wrap != 0) {
			memcpy(buf, g_qsee_log->log_buf +
			      g_qsee_log->log_pos.offset, QSEE_LOG_BUF_SIZE -
			      g_qsee_log->log_pos.offset - 4);
			count = QSEE_LOG_BUF_SIZE -
				g_qsee_log->log_pos.offset - 4;
			memcpy(buf + count, g_qsee_log->log_buf,
			      g_qsee_log->log_pos.offset);
			count = count + g_qsee_log->log_pos.offset;
		} else {
			memcpy(buf, g_qsee_log->log_buf,
			      g_qsee_log->log_pos.offset);
			count = g_qsee_log->log_pos.offset;
		}
	} else {
		pr_err("load app and then view log..\n");
		return -EINVAL;
	}

	return count;
}

/*
 * store_aes_derive_key()
 * Function to store aes derive key
 */
static ssize_t
store_aes_derive_key(struct device *dev, struct device_attribute *attr,
	 const char *buf, size_t count)
{
	unsigned long long val;

	if (kstrtoull(buf, 10, &val))
		return -EINVAL;

	*key_handle = val;
	if (*key_handle == INVALID_AES_KEY_HANDLE_VAL) {
		pr_info("Invalid aes key handle: %lu\n",
			(unsigned long)*key_handle);
		return -EINVAL;
	}

	return count;
}

/*
 * show_aes_derive_key()
 * Function to derive aes_key and get key_handle
 */
static ssize_t show_aes_derive_key(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int rc = 0, i = 0;
	size_t req_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_req_addr = 0;
	const char *message = NULL;
	int message_len = 0;
	struct qti_storage_service_derive_key_cmd_t_v1 *req_ptr;

	if (!source_data || !context_data_len || !bindings_data) {
		pr_info("Provide the required src data, bindings data and context data before encrypt/decrypt\n");
		return -EINVAL;
	}

	dev = qdev;

	if (context_data_len > MAX_CONTEXT_BUFFER_LEN_V1) {
		pr_err("Context data length must be less than %d bytes\n",
				MAX_CONTEXT_BUFFER_LEN_V1);
		return -EINVAL;
	}
	req_size = sizeof(struct qti_storage_service_derive_key_cmd_t_v1 );
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_derive_key_cmd_t_v1 *)
		dma_alloc_coherent(dev, dma_buf_size,
				&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	req_ptr->policy.key_type = DEFAULT_KEY_TYPE;
	req_ptr->policy.destination = DEFAULT_POLICY_DESTINATION;
	req_ptr->hw_key_bindings.bindings = bindings_data;
	req_ptr->source = source_data;
	req_ptr->key = (u64) dma_key_handle;
	req_ptr->mixing_key = 0;

	for (i = 0; i < MAX_CONTEXT_BUFFER_LEN_V1; i++)
		req_ptr->hw_key_bindings.context[i] = context_data[i];
	req_ptr->hw_key_bindings.context_len = context_data_len;

	rc = qti_scm_aes(dma_req_addr, req_size, QTI_CMD_AES_DERIVE_KEY);
	if (rc == KEY_HANDLE_OUT_OF_SLOT)
		pr_info("Key handle out of slot. Clear a key and try again!\n");
	if (!rc) {
		message = "AES Key derive successful\n\0";
	} else {
		pr_err("SCM call failed..return value = %d\n", rc);
		message = "AES Key derive failed\n\0";
	}

	pr_info("key_handle is: %lu\n", (unsigned long)*key_handle);

	message_len = strlen(message) + 1;
	memcpy(buf, message, message_len);

	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);
	return message_len;
}

/*
 * show_aes_derive_128_key()
 * Function to derive aes_max_ctxt_key and get key_handle
 */
static ssize_t show_aes_derive_128_byte_key(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int rc = 0, i = 0;
	size_t req_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_req_addr = 0;
	const char *message = NULL;
	int message_len = 0;
	struct qti_storage_service_derive_key_cmd_t_v2 *req_ptr;

	if (!source_data || !context_data_len || !bindings_data) {
		pr_info("Provide the required src data, bindings data and "
			"context data before encrypt/decrypt\n");
		return -EINVAL;
	}

	dev = qdev;

	if (context_data_len > MAX_CONTEXT_BUFFER_LEN_V2) {
		pr_err("Context data length must be less than %d bytes\n",
				MAX_CONTEXT_BUFFER_LEN_V2);
		return -EINVAL;
	}

	req_size = sizeof(struct qti_storage_service_derive_key_cmd_t_v2 );
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_derive_key_cmd_t_v2 *)
				dma_alloc_coherent(dev, dma_buf_size,
				&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	req_ptr->policy.key_type = DEFAULT_KEY_TYPE;
	req_ptr->policy.destination = DEFAULT_POLICY_DESTINATION;
	req_ptr->hw_key_bindings.bindings = bindings_data;
	req_ptr->source = source_data;
	req_ptr->key = (u64) dma_key_handle;
	req_ptr->mixing_key = 0;

	for (i = 0; i < MAX_CONTEXT_BUFFER_LEN_V2; i++)
		req_ptr->hw_key_bindings.context[i] = context_data[i];
	req_ptr->hw_key_bindings.context_len = context_data_len;

	rc = qti_scm_aes(dma_req_addr, req_size, QTI_CMD_AES_DERIVE_128_KEY);
	if (rc == KEY_HANDLE_OUT_OF_SLOT)
		pr_info("Key handle out of slot. Clear a key and try again!\n");
	if (!rc) {
		message = "AES Key derive successful\n\0";
	} else {
		pr_err("SCM call failed..return value = %d\n", rc);
		message = "AES Key derive failed\n\0";
	}

	pr_info("key_handle is: %lu\n", (unsigned long)*key_handle);

	message_len = strlen(message) + 1;
	memcpy(buf, message, message_len);

	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);
	return message_len;
}

static ssize_t store_aes_clear_key(struct device *dev, struct device_attribute *attr, const char* buf, size_t count)
{
	int rc = 0;
	uint32_t key_handle;

	if (kstrtouint(buf, 10, &key_handle))
		return -EINVAL;

	rc = qti_scm_aes_clear_key_handle(key_handle, QTI_CMD_AES_CLEAR_KEY);

	if (!rc)
		pr_info("AES key = %u cleared successfully\n",key_handle);
	else
		pr_info("AES key clear failed\n");

	return count;
}

static ssize_t
generate_key_blob(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct qti_storage_service_gen_key_cmd_t *req_ptr = NULL;
	struct qti_storage_service_gen_key_resp_t *resp_ptr = NULL;
	size_t req_size = 0;
	size_t resp_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_req_addr = 0;
	dma_addr_t dma_resp_addr = 0;

	key_blob = memset(key_blob, 0, KEY_BLOB_SIZE);
	key_blob_len = 0;

	dev = qdev;

	req_size = sizeof(struct qti_storage_service_gen_key_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_gen_key_cmd_t *)
			dma_alloc_coherent(dev, dma_buf_size,
			&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	resp_size = sizeof(struct qti_storage_service_gen_key_resp_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	resp_ptr = (struct qti_storage_service_gen_key_resp_t *)
			dma_alloc_coherent(dev, dma_buf_size,
			&dma_resp_addr, GFP_KERNEL);

	if (!resp_ptr)
		return -ENOMEM;

	req_ptr->key_blob.key_material = (u64)dma_key_blob;
	req_ptr->cmd_id = QTI_STOR_SVC_GENERATE_KEY;
	req_ptr->key_blob.key_material_len = KEY_BLOB_SIZE;

	rc = qti_scm_tls_hardening(dma_req_addr, req_size,
				   dma_resp_addr, resp_size,
				   CLIENT_CMD_CRYPTO_AES_64);
	if (rc) {
		pr_err("SCM Call failed..SCM Call return value = %d\n", rc);
		goto err_end;
	}

	if (resp_ptr->status) {
		rc = resp_ptr->status;
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	key_blob_len = resp_ptr->key_blob_size;
	memcpy(buf, key_blob, key_blob_len);

	goto end;

err_end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);
	return rc;

end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);

	return key_blob_len;
}

static ssize_t
store_key(struct device *dev, struct device_attribute *attr,
	 const char *buf, size_t count)
{
	key = memset(key, 0, KEY_SIZE);
	key_len = 0;

	if (count != KEY_SIZE) {
		pr_info("Invalid input\n");
		pr_info("Key length is %lu bytes\n", (unsigned long)count);
		pr_info("Key length must be %u bytes\n",(unsigned int)KEY_SIZE);
		return -EINVAL;
	}

	key_len = count;
	memcpy(key, buf, key_len);

	return count;
}

static ssize_t
import_key_blob(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct qti_storage_service_import_key_cmd_t *req_ptr = NULL;
	struct qti_storage_service_gen_key_resp_t *resp_ptr = NULL;
	size_t req_size = 0;
	size_t resp_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_req_addr = 0;
	dma_addr_t dma_resp_addr = 0;

	key_blob = memset(key_blob, 0, KEY_BLOB_SIZE);
	key_blob_len = 0;

	if (key_len == 0) {
		pr_err("Please provide key to import key blob\n");
		return -EINVAL;
	}

	dev = qdev;

        req_size = sizeof(struct qti_storage_service_import_key_cmd_t);
        dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
        req_ptr = (struct qti_storage_service_import_key_cmd_t *)
                        dma_alloc_coherent(dev, dma_buf_size,
                        &dma_req_addr, GFP_KERNEL);
        if (!req_ptr)
                return -ENOMEM;

        resp_size = sizeof(struct qti_storage_service_gen_key_resp_t);
        dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
        resp_ptr = (struct qti_storage_service_gen_key_resp_t *)
                        dma_alloc_coherent(dev, dma_buf_size,
                        &dma_resp_addr, GFP_KERNEL);

        if (!resp_ptr)
                return -ENOMEM;

	req_ptr->input_key = (u64)dma_key;
	req_ptr->key_blob.key_material = (u64)dma_key_blob;
	req_ptr->cmd_id = QTI_STOR_SVC_IMPORT_KEY;
	req_ptr->input_key_len = KEY_SIZE;
	req_ptr->key_blob.key_material_len = KEY_BLOB_SIZE;

	rc = qti_scm_tls_hardening(dma_req_addr, req_size,
				   dma_resp_addr, resp_size,
				   CLIENT_CMD_CRYPTO_AES_64);
	if (rc) {
		pr_err("SCM Call failed..SCM Call return value = %d\n", rc);
		goto err_end;
	}

	if (resp_ptr->status) {
		rc = resp_ptr->status;
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	key_blob_len = resp_ptr->key_blob_size;
	memcpy(buf, key_blob, key_blob_len);

	goto end;

err_end:
        dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

        dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, resp_ptr, dma_resp_addr);
	return rc;

end:
        dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

        dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, resp_ptr, dma_resp_addr);

	return key_blob_len;
}

static ssize_t
store_key_blob(struct device *dev, struct device_attribute *attr,
	      const char *buf, size_t count)
{
	key_blob = memset(key_blob, 0, KEY_BLOB_SIZE);
	key_blob_len = 0;

	if (count != KEY_BLOB_SIZE) {
		pr_info("Invalid input\n");
		pr_info("Key blob length is %lu bytes\n", (unsigned long)count);
		pr_info("Key blob length must be %u bytes\n", (unsigned int)KEY_BLOB_SIZE);
		return -EINVAL;
	}

	key_blob_len = count;
	memcpy(key_blob, buf, key_blob_len);

	return count;
}

static ssize_t
store_aes_mode(struct device *dev, struct device_attribute *attr,
               const char *buf, size_t count)
{
	unsigned long long val;

	if (kstrtoull(buf, 10, &val))
		return -EINVAL;

	mode = val;
	if (mode >= QTI_CRYPTO_SERVICE_AES_MODE_MAX) {
		pr_info("Invalid aes 256 mode: %llu\n", mode);
		return -EINVAL;
	}

	return count;
}

static ssize_t
store_aes_type(struct device *dev, struct device_attribute *attr,
               const char *buf, size_t count)
{
	unsigned long long val;

	if (kstrtoull(buf, 10, &val))
		return -EINVAL;

	type = val;
	if (!type || type >= QTI_CRYPTO_SERVICE_AES_TYPE_MAX) {
		pr_info("Invalid aes 256 type: %llu\n", type);
		return -EINVAL;
	}

	return count;
}

static ssize_t
store_decrypted_data(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsealed_buf = memset(unsealed_buf, 0, MAX_PLAIN_DATA_SIZE);
	decrypted_len = count;

	if ((decrypted_len % AES_BLOCK_SIZE) ||
			decrypted_len > MAX_PLAIN_DATA_SIZE) {
		pr_info("Invalid input\n");
		pr_info("Plain data length is %lu bytes\n",
		       (unsigned long)decrypted_len);
		pr_info("Plain data length must be multiple of AES block size"
			"of 16 bytes and <= %u bytes\n",
			(unsigned int)MAX_PLAIN_DATA_SIZE);
		return -EINVAL;
	}

	if (!ivdata) {
		pr_info("could not allocate ivdata\n");
		return -EINVAL;
	}

	memcpy(unsealed_buf, buf, decrypted_len);
	return count;
}

static ssize_t
store_iv_data(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
	if (!ivdata) {
		pr_info("could not allocate ivdata\n");
		return -EINVAL;
	}

	ivdata = memset(ivdata, 0, AES_BLOCK_SIZE);
	ivdata_len = count;

	if (ivdata_len != AES_BLOCK_SIZE) {
		pr_info("Invalid input\n");
		pr_info("IV data length is %lu bytes\n",
		       (unsigned long)ivdata_len);
		pr_info("IV data length must be equal to AES block size"
		        " (16) bytes\n");
		return -EINVAL;
	}

	memcpy(ivdata, buf, ivdata_len);
	return count;
}

/*
 * store_source_data()
 * Function to provide source which is required for derive key
 */
static ssize_t
store_source_data(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
	if ((count - 1) == 0) {
		pr_err("Input cannot be NULL!\n");
		return -EINVAL;
	}

	if (kstrtouint(buf, 10, (unsigned int *)&source_data) || source_data > (U32_MAX / 10)) {
		pr_err("Please enter a valid unsigned integer less than %u\n",
			(U32_MAX / 10));
		return -EINVAL;
	}
	pr_debug("source_data = %lu\n", (unsigned long)source_data);

	return count;
}

/*
 * store_bindings_data()
 * Function to provide bindings bit mask which is required for derive key
 */
static ssize_t
store_bindings_data(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
	if ((count - 1) == 0) {
		pr_err("Input cannot be NULL!\n");
		return -EINVAL;
	}

	if (kstrtouint(buf, 10, (unsigned int *)&bindings_data) || bindings_data > (U32_MAX / 10)) {
		pr_err("Please enter a valid unsigned integer less than %u\n",
			(U32_MAX / 10));
		return -EINVAL;
	}
	pr_debug("bindings_data = %lu\n", (unsigned long)bindings_data);

	return count;
}

/*
 * store_context_data()
 * Function provide context(salt) to be mixed which is required for derive key
 */
static ssize_t
store_context_data(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
	int i = 0;
	int num_bytes = count / 2 ;


	for (i = 0; i < MAX_CONTEXT_BUFFER_LEN_V2; i++)
		context_data[i] = 0;

	if(count % 2 != 0) {
		pr_info("Input data should be in terms of bytes, which " \
			"will have even number of digits\n");
		pr_info("Context data length is %lu bytes\n",
			(unsigned long)count);
		context_data_len = 0;
		return -EINVAL;
	}

	context_data_len = num_bytes;

	if (count > (MAX_CONTEXT_BUFFER_LEN_V2 * 2)) {
		pr_info("Invalid input\n");
		pr_info("Context data length is %lu bytes\n",
		       (unsigned long)count);
		pr_info("Context data length must be less than %d bytes\n",
				MAX_CONTEXT_BUFFER_LEN_V2);
		context_data_len = 0;
		return -EINVAL;
	}

	for (i = 0; i < num_bytes; i++) {
		sscanf(buf, "%2hhx", &context_data[i]);
		buf += 2;
	}

	pr_debug("context_data is :\n");
	for (i = 0; i < num_bytes; i++)
		pr_debug("0x%02x\n", (unsigned int)context_data[i]);

	return count;
}

/*
 * store_source_data()
 * Function to provide source which is required for derive key (in QTIAPP)
 */
static ssize_t
store_source_data_qtiapp(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
	if ((count - 1) == 0) {
		pr_err("Input cannot be NULL!\n");
		return -EINVAL;
	}

	if (kstrtouint(buf, 10, (unsigned int *)&aes_source_data)
			|| aes_source_data > (U32_MAX / 10)) {
		pr_err("Please enter a valid unsigned integer less than %u\n",
			(U32_MAX / 10));
		return -EINVAL;
	}
	pr_debug("source_data = %lu\n", (unsigned long)aes_source_data);

	return count;
}

/*
 * store_bindings_data()
 * Function to provide bindings bit mask which is required for derive key (in QTIAPP)
 */
static ssize_t
store_bindings_data_qtiapp(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
	if ((count - 1) == 0) {
		pr_err("Input cannot be NULL!\n");
		return -EINVAL;
	}

	if (kstrtouint(buf, 10, (unsigned int *)&aes_bindings_data)
			|| aes_bindings_data > (U32_MAX / 10)) {
		pr_err("Please enter a valid unsigned integer less than %u\n",
			(U32_MAX / 10));
		return -EINVAL;
	}
	pr_debug("bindings_data = %lu\n", (unsigned long)aes_bindings_data);

	return count;
}

/*
 * store_context_data()
 * Function provide context(salt) to be mixed which is required for derive key (in QTIAPP)
 */
static ssize_t
store_context_data_qtiapp(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
	int i = 0;

	for (i = 0; i < MAX_CONTEXT_BUFFER_LEN_V1; i++)
		aes_context_data[i] = 0;
	aes_context_data_len = MAX_CONTEXT_BUFFER_LEN_V1;

	if (count > ((MAX_CONTEXT_BUFFER_LEN_V1 * 2) + 1)) {
		pr_info("Invalid input\n");
		pr_info("Context data length is %lu bytes\n",
		       (unsigned long)count);
		pr_info("Context data length must be less than 64 bytes\n");
		return -EINVAL;
	}

	for (i = 0; i < MAX_CONTEXT_BUFFER_LEN_V1; i++) {
		sscanf(buf, "%2hhx", &aes_context_data[i]);
		buf += 2;
	}

	pr_debug("context_data is :\n");
	for (i = 0; i < MAX_CONTEXT_BUFFER_LEN_V1; i++)
		pr_debug("0x%02x\n", (unsigned int)aes_context_data[i]);

	return count;
}

static ssize_t
store_aes_mode_qtiapp(struct device *dev, struct device_attribute *attr,
               const char *buf, size_t count)
{
	unsigned long long val;

	if (kstrtoull(buf, 10, &val))
		return -EINVAL;

	if (val >= QTI_CRYPTO_SERVICE_AES_MODE_MAX) {
		pr_info("Invalid aes 256 mode: %llu\n", val);
		return -EINVAL;
	}
	aes_mode = val;

	return count;
}

static ssize_t
store_aes_type_qtiapp(struct device *dev, struct device_attribute *attr,
               const char *buf, size_t count)
{
	unsigned long long val;

	if (kstrtoull(buf, 10, &val))
		return -EINVAL;

	if (!val || val >= QTI_CRYPTO_SERVICE_AES_TYPE_MAX) {
		pr_info("Invalid aes 256 type: %llu\n", val);
		return -EINVAL;
	}
	aes_type = val;

	return count;
}

static ssize_t
store_iv_data_qtiapp(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
	if (!aes_ivdata) {
		pr_info("could not allocate ivdata\n");
		return -EINVAL;
	}

	if (count != AES_BLOCK_SIZE) {
		pr_info("Invalid input\n");
		pr_info("IV data length is %lu bytes\n",
		       (unsigned long)count);
		pr_info("IV data length must be equal to AES block size"
		        "(16) bytes");
		return -EINVAL;
	}

	aes_ivdata = memset(aes_ivdata, 0, AES_BLOCK_SIZE);
	aes_ivdata_len = count;
	memcpy(aes_ivdata, buf, aes_ivdata_len);
	return count;
}
static ssize_t
store_aes_decrypted_data_qtiapp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{

	if ((count % AES_BLOCK_SIZE) ||
			count > MAX_PLAIN_DATA_SIZE) {
		pr_info("Invalid input\n");
		pr_info("Plain data length is %lu bytes\n",
		       (unsigned long)count);
		pr_info("Plain data length must be multiple of AES block size"
			"of 16 bytes and <= %u bytes\n",
			(unsigned int)MAX_PLAIN_DATA_SIZE);
		return -EINVAL;
	}

	if (!aes_ivdata) {
		pr_info("could not allocate ivdata\n");
		return -EINVAL;
	}

	aes_unsealed_buf = memset(aes_unsealed_buf, 0, MAX_PLAIN_DATA_SIZE);
	aes_decrypted_len = count;
	memcpy(aes_unsealed_buf, buf, aes_decrypted_len);
	return count;
}

static ssize_t
store_aes_encrypted_data_qtiapp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	if ((count % AES_BLOCK_SIZE) || count > MAX_PLAIN_DATA_SIZE) {
		pr_info("Invalid input\n");
		pr_info("Encrypted data length is %lu bytes\n",
			(unsigned long)count);
		pr_info("Encrypted data length must be multiple of AES block"
			"size 16  and <= %ubytes\n",
			(unsigned int)MAX_ENCRYPTED_DATA_SIZE);
		return -EINVAL;
	}

	aes_sealed_buf = memset(aes_sealed_buf, 0, MAX_ENCRYPTED_DATA_SIZE);
	aes_encrypted_len = count;
	memcpy(aes_sealed_buf, buf, count);

	return count;
}

static ssize_t
show_aes_v2_encrypted_data(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int rc = 0;
	struct qti_crypto_service_encrypt_data_cmd_t_v2 *req_ptr = NULL;
	uint64_t req_size = 0;
	size_t dma_buf_size = 0;
	uint64_t output_len = 0;
	dma_addr_t dma_req_addr = 0;

	sealed_buf = memset(sealed_buf, 0, MAX_ENCRYPTED_DATA_SIZE);
	output_len = decrypted_len;

	if (decrypted_len <= 0 || decrypted_len % AES_BLOCK_SIZE) {
		pr_err("Invalid input %lld\n", decrypted_len);
		pr_info("Input data length for encryption should be multiple"
			" of AES block size(16)\n");
		return -EINVAL;
	}
	if (!key_handle) {
		pr_info("Derive the required key handle before encrypt/decrypt\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_crypto_service_encrypt_data_cmd_t_v2);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_crypto_service_encrypt_data_cmd_t_v2 *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	req_ptr->key_handle = (unsigned long)*key_handle;
	pr_info("key_handle is: %lu\n", (unsigned long)req_ptr->key_handle);
	req_ptr->v1.type = type;
	req_ptr->v1.mode = mode;
	req_ptr->v1.iv = (req_ptr->v1.mode == 0 ? 0 : (u64)dma_iv_data);
	req_ptr->v1.iv_len = AES_BLOCK_SIZE;
	req_ptr->v1.plain_data = (u64)dma_unsealed_data;
	req_ptr->v1.output_buffer = (u64)dma_sealed_data;
	req_ptr->v1.plain_data_len = decrypted_len;
	req_ptr->v1.output_len = output_len;

	/* dma_resp_addr and resp_size required are passed in req str itself */
	rc = qti_scm_aes(dma_req_addr, req_size,
			CLIENT_CMD_CRYPTO_AES_ENCRYPT);
	if (rc) {
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	if (mode == QTI_CRYPTO_SERVICE_AES_MODE_CBC && ivdata)
		print_hex_dump(KERN_INFO, "IV data(CBC): ", DUMP_PREFIX_NONE, 16, 1,
				ivdata, AES_BLOCK_SIZE, false);
	else
		pr_info("IV data(ECB): NULL\n");

	memcpy(buf, sealed_buf, req_ptr->v1.output_len);
	encrypted_len = req_ptr->v1.output_len;

goto end;
err_end:
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return rc;

end:
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return encrypted_len;
}

static ssize_t
show_encrypted_data(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int rc = 0;
	struct qti_crypto_service_encrypt_data_cmd_t *req_ptr = NULL;
	uint64_t req_size = 0;
	size_t dma_buf_size = 0;
	uint64_t output_len = 0;
	dma_addr_t dma_req_addr = 0;

	if (props->aes_v2) {
		rc = show_aes_v2_encrypted_data(dev, attr, buf);
		return rc;
	}

	sealed_buf = memset(sealed_buf, 0, MAX_ENCRYPTED_DATA_SIZE);
	output_len = decrypted_len;

	if (decrypted_len <= 0 || decrypted_len % AES_BLOCK_SIZE) {
		pr_err("Invalid input %lld\n", decrypted_len);
		pr_info("Input data length for encryption should be multiple"
			" of AES block size(16)\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_crypto_service_encrypt_data_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_crypto_service_encrypt_data_cmd_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	req_ptr->type = type;
	req_ptr->mode = mode;
	req_ptr->iv = (req_ptr->mode == 0 ? 0 : (u64)dma_iv_data);
	req_ptr->iv_len = AES_BLOCK_SIZE;
	req_ptr->plain_data = (u64)dma_unsealed_data;
	req_ptr->output_buffer = (u64)dma_sealed_data;
	req_ptr->plain_data_len = decrypted_len;
	req_ptr->output_len = output_len;

	/* dma_resp_addr and resp_size required are passed in req str itself */
	rc = qti_scm_aes(dma_req_addr, req_size,
			CLIENT_CMD_CRYPTO_AES_ENCRYPT);
	if (rc) {
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	if (mode == QTI_CRYPTO_SERVICE_AES_MODE_CBC && ivdata)
		print_hex_dump(KERN_INFO, "IV data(CBC): ", DUMP_PREFIX_NONE, 16, 1,
				ivdata, AES_BLOCK_SIZE, false);
	else
		pr_info("IV data(ECB): NULL\n");

	memcpy(buf, sealed_buf, req_ptr->output_len);
	encrypted_len = req_ptr->output_len;

goto end;

err_end:
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);
	return rc;

end:
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);
	return encrypted_len;
}

static ssize_t
store_encrypted_data(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	sealed_buf = memset(sealed_buf, 0, MAX_ENCRYPTED_DATA_SIZE);
	encrypted_len = 0;

	if ((count % AES_BLOCK_SIZE) || count > MAX_PLAIN_DATA_SIZE) {
		pr_info("Invalid input\n");
		pr_info("Encrypted data length is %lu bytes\n",
			(unsigned long)count);
		pr_info("Encrypted data length must be multiple of AES block"
			" size 16  and <= %ubytes\n",
			(unsigned int)MAX_ENCRYPTED_DATA_SIZE);
		return -EINVAL;
	}

	encrypted_len = count;
	memcpy(sealed_buf, buf, count);

	return count;
}

static ssize_t
show_aes_v2_decrypted_data(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct qti_crypto_service_decrypt_data_cmd_t_v2 *req_ptr = NULL;
	uint64_t req_size = 0;
	size_t dma_buf_size = 0;
	uint64_t output_len = 0;
	dma_addr_t dma_req_addr = 0;

	unsealed_buf = memset(unsealed_buf, 0, MAX_PLAIN_DATA_SIZE);
	output_len = encrypted_len;

	if (encrypted_len <= 0 || encrypted_len % AES_BLOCK_SIZE) {
		pr_err("Invalid input %lld\n", encrypted_len);
		pr_info("Encrypted data length for decryption should be multiple"
			" of AES block size(16)\n");
		return -EINVAL;
	}
	if (!type) {
		pr_info("aes type needed for decryption\n");
		return -EINVAL;
	}
	if (!key_handle) {
		pr_info("Derive the required key handle before encrypt/decrypt\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_crypto_service_decrypt_data_cmd_t_v2);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_crypto_service_decrypt_data_cmd_t_v2 *)
					dma_alloc_coherent(dev, dma_buf_size,
						  &dma_req_addr, GFP_KERNEL);

	if (!req_ptr)
		return -ENOMEM;

	req_ptr->key_handle = (unsigned long)*key_handle;
	pr_info("key_handle is: %lu\n", (unsigned long)req_ptr->key_handle);
	req_ptr->v1.type = type;
	req_ptr->v1.mode = mode;
	req_ptr->v1.encrypted_data = (u64)dma_sealed_data;
	req_ptr->v1.output_buffer = (u64)dma_unsealed_data;
	req_ptr->v1.iv = (req_ptr->v1.mode == 0 ? 0 : (u64)dma_iv_data);
	req_ptr->v1.iv_len = AES_BLOCK_SIZE;
	req_ptr->v1.encrypted_dlen = encrypted_len;
	req_ptr->v1.output_len = output_len;

	/* dma_resp_addr and resp_size required are passed in req str itself */
	rc = qti_scm_aes(dma_req_addr, req_size,
			CLIENT_CMD_CRYPTO_AES_DECRYPT);

	if (rc) {
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	decrypted_len = req_ptr->v1.output_len;
	memcpy(buf, unsealed_buf, decrypted_len);

goto end;

err_end:
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return rc;

end:
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return decrypted_len;
}

static ssize_t
show_decrypted_data(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct qti_crypto_service_decrypt_data_cmd_t *req_ptr = NULL;
	uint64_t req_size = 0;
	size_t dma_buf_size = 0;
	uint64_t output_len = 0;
	dma_addr_t dma_req_addr = 0;

	if (props->aes_v2) {
		rc = show_aes_v2_decrypted_data(dev, attr, buf);
		return rc;
	}

	unsealed_buf = memset(unsealed_buf, 0, MAX_PLAIN_DATA_SIZE);
	output_len = encrypted_len;

	if (encrypted_len <= 0 || encrypted_len % AES_BLOCK_SIZE) {
		pr_err("Invalid input %lld\n", encrypted_len);
		pr_info("Encrypted data length for decryption should be multiple"
			" of AES block size(16)\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_crypto_service_decrypt_data_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_crypto_service_decrypt_data_cmd_t *)
			dma_alloc_coherent(dev, dma_buf_size,
			&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	req_ptr->type = type;
	req_ptr->mode = mode;
	req_ptr->encrypted_data = (u64)dma_sealed_data;
	req_ptr->output_buffer = (u64)dma_unsealed_data;
	req_ptr->iv = (req_ptr->mode == 0 ? 0 : (u64)dma_iv_data);
	req_ptr->iv_len = AES_BLOCK_SIZE;
	req_ptr->encrypted_dlen = encrypted_len;
	req_ptr->output_len = output_len;

	/* dma_resp_addr and resp_size required are passed in req str itself */
	rc = qti_scm_aes(dma_req_addr, req_size,
			CLIENT_CMD_CRYPTO_AES_DECRYPT);
	if (rc) {
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	decrypted_len = req_ptr->output_len;
	memcpy(buf, unsealed_buf, decrypted_len);

goto end;

err_end:
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return rc;

end:
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return decrypted_len;
}

static ssize_t
store_unsealed_data(struct device *dev, struct device_attribute *attr,
		   const char *buf, size_t count)
{
	unsealed_buf = memset(unsealed_buf, 0, MAX_PLAIN_DATA_SIZE);
	unseal_len = 0;

	if (count == 0 || count > MAX_PLAIN_DATA_SIZE) {
		pr_info("Invalid input\n");
		pr_info("Plain data length is %lu bytes\n",
		       (unsigned long)count);
		pr_info("Plain data length must be > 0 bytes and <= %u bytes\n",
		       (unsigned int)MAX_PLAIN_DATA_SIZE);
		return -EINVAL;
	}

	unseal_len = count;
	memcpy(unsealed_buf, buf, unseal_len);

	return count;
}

static ssize_t
show_sealed_data(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct qti_storage_service_seal_data_cmd_t *req_ptr = NULL;
	struct qti_storage_service_seal_data_resp_t *resp_ptr = NULL;
	size_t req_size = 0;
	size_t resp_size = 0;
	size_t dma_buf_size = 0;
	size_t output_len = 0;
	dma_addr_t dma_req_addr = 0;
	dma_addr_t dma_resp_addr = 0;

	sealed_buf = memset(sealed_buf, 0, MAX_ENCRYPTED_DATA_SIZE);
	output_len = unseal_len + ENCRYPTED_DATA_HEADER;

	if (key_blob_len == 0 || unseal_len == 0) {
		pr_err("Invalid input\n");
		pr_info("Need keyblob and plain data for encryption\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_storage_service_seal_data_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_seal_data_cmd_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	resp_size = sizeof(struct qti_storage_service_seal_data_resp_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	resp_ptr = (struct qti_storage_service_seal_data_resp_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_resp_addr, GFP_KERNEL);

	if (!resp_ptr)
		return -ENOMEM;

	req_ptr->key_blob.key_material = (u64)dma_key_blob;
	req_ptr->plain_data = (u64)dma_unsealed_data;
	req_ptr->output_buffer = (u64)dma_sealed_data;
	req_ptr->cmd_id = QTI_STOR_SVC_SEAL_DATA;
	req_ptr->key_blob.key_material_len = KEY_BLOB_SIZE;
	req_ptr->plain_data_len = unseal_len;
	req_ptr->output_len = output_len;

	rc = qti_scm_tls_hardening(dma_req_addr, req_size,
				   dma_resp_addr, resp_size,
				   CLIENT_CMD_CRYPTO_AES_64);
	if (rc) {
		pr_err("SCM Call failed..SCM Call return value = %d\n", rc);
		goto err_end;
	}

	if (resp_ptr->status != 0) {
		rc = resp_ptr->status;
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	seal_len = resp_ptr->sealed_data_len;
	memcpy(buf, sealed_buf, seal_len);

	goto end;

err_end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);
	return rc;

end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);

	return seal_len;
}

static ssize_t
store_sealed_data(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
	sealed_buf = memset(sealed_buf, 0, MAX_ENCRYPTED_DATA_SIZE);
	seal_len = 0;

	if (count == 0 || count > MAX_ENCRYPTED_DATA_SIZE) {
		pr_info("Invalid input\n");
		pr_info("Encrypted data length is %lu bytes\n",
		       (unsigned long)count);
		pr_info("Encrypted data length must be > 0 bytes and <= %u bytes\n",
		       (unsigned int)MAX_ENCRYPTED_DATA_SIZE);
		return -EINVAL;
	}

	seal_len = count;
	memcpy(sealed_buf, buf, seal_len);

	return count;
}

static ssize_t
show_unsealed_data(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct qti_storage_service_unseal_data_cmd_t *req_ptr = NULL;
	struct qti_storage_service_unseal_data_resp_t *resp_ptr = NULL;
	size_t req_size = 0;
	size_t resp_size = 0;
	size_t output_len = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_req_addr = 0;
	dma_addr_t dma_resp_addr = 0;

	unsealed_buf = memset(unsealed_buf, 0, MAX_PLAIN_DATA_SIZE);
	output_len = seal_len - ENCRYPTED_DATA_HEADER;

	if (key_blob_len == 0 || seal_len == 0) {
		pr_err("Invalid input\n");
		pr_info("Need key and sealed data for decryption\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_storage_service_unseal_data_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_unseal_data_cmd_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	resp_size = sizeof(struct qti_storage_service_unseal_data_resp_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	resp_ptr = (struct qti_storage_service_unseal_data_resp_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_resp_addr, GFP_KERNEL);
	if (!resp_ptr)
		return -ENOMEM;

	req_ptr->key_blob.key_material = (u64)dma_key_blob;
	req_ptr->sealed_data = (u64)dma_sealed_data;
	req_ptr->output_buffer = (u64)dma_unsealed_data;
	req_ptr->cmd_id = QTI_STOR_SVC_UNSEAL_DATA;
	req_ptr->key_blob.key_material_len = KEY_BLOB_SIZE;
	req_ptr->sealed_dlen = seal_len;
	req_ptr->output_len = output_len;

	rc = qti_scm_tls_hardening(dma_req_addr, req_size,
				   dma_resp_addr, resp_size,
				   CLIENT_CMD_CRYPTO_AES_64);
	if (rc) {
		pr_err("SCM Call failed..SCM Call return value = %d\n", rc);
		goto err_end;
	}

	if (resp_ptr->status != 0) {
		rc = resp_ptr->status;
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	unseal_len = resp_ptr->unsealed_data_len;
	memcpy(buf, unsealed_buf, unseal_len);

	goto end;

err_end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);

	return rc;

end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);

	return unseal_len;
}

static ssize_t
generate_rsa_key_blob(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	int rc = 0;
	struct qti_storage_service_rsa_gen_key_cmd_t *req_ptr = NULL;
	struct qti_storage_service_rsa_gen_key_resp_t *resp_ptr = NULL;
	size_t req_size = 0;
	size_t resp_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_req_addr = 0;
	dma_addr_t dma_resp_addr = 0;

	rsa_key_blob = memset(rsa_key_blob, 0, RSA_KEY_BLOB_SIZE);
	rsa_key_blob_len = 0;

	dev = qdev;

	req_size = sizeof(struct qti_storage_service_rsa_gen_key_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_rsa_gen_key_cmd_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	resp_size = sizeof(struct qti_storage_service_rsa_gen_key_resp_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	resp_ptr = (struct qti_storage_service_rsa_gen_key_resp_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_resp_addr, GFP_KERNEL);
	if (!resp_ptr)
		return -ENOMEM;

	req_ptr->key_blob.key_material = (u64)dma_rsa_key_blob;
	req_ptr->cmd_id = QTI_STOR_SVC_RSA_GENERATE_KEY;
	req_ptr->rsa_params.modulus_size = RSA_MODULUS_LEN;
	req_ptr->rsa_params.public_exponent = RSA_PUBLIC_EXPONENT;
	pr_info("rsa pad scheme used = %u\n",cur_rsa_pad_scheme);
	req_ptr->rsa_params.pad_algo = cur_rsa_pad_scheme;
	req_ptr->key_blob.key_material_len = RSA_KEY_BLOB_SIZE;

	rc = qti_scm_tls_hardening(dma_req_addr, req_size,
				   dma_resp_addr, resp_size,
				   CLIENT_CMD_CRYPTO_RSA_64);
	if (rc) {
		pr_err("SCM call failed..SCM Call return value = %d\n", rc);
		goto err_end;
	}

	if (resp_ptr->status) {
		rc = resp_ptr->status;
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	rsa_key_blob_len = resp_ptr->key_blob_size;
	memcpy(buf, rsa_key_blob, rsa_key_blob_len);
	rsa_key_blob_buf_valid = 1;

	goto end;

err_end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);

	return rc;

end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);

	return rsa_key_blob_len;
}

static ssize_t
store_rsa_key(struct device *dev, struct device_attribute *attr,
	     const char *buf, size_t count)
{
	int idx = 0;
	int ret = 0;
	int i = 0;
	int j = 0;
	char hex_len[RSA_PARAM_LEN];

	memset(rsa_import_modulus, 0, RSA_KEY_SIZE_MAX);
	rsa_import_modulus_len = 0;
	memset(rsa_import_public_exponent, 0, RSA_PUB_EXP_SIZE_MAX);
	rsa_import_public_exponent_len = 0;
	memset(rsa_import_pvt_exponent, 0, RSA_KEY_SIZE_MAX);
	rsa_import_pvt_exponent_len = 0;

	if (count != RSA_KEY_MATERIAL_SIZE) {
		pr_info("Invalid input\n");
		pr_info("Key material length is %lu bytes\n",
		       (unsigned long)count);
		pr_info("Key material length must be %u bytes\n",
		       (unsigned int)RSA_KEY_MATERIAL_SIZE);
		return -EINVAL;
	}

	memcpy(rsa_import_modulus, buf, RSA_KEY_SIZE_MAX);
	idx += RSA_KEY_SIZE_MAX;
	memset(hex_len, 0, RSA_PARAM_LEN);
	for (i = idx, j = 0; i < idx + 2; i++, j++)
		snprintf(hex_len + (j * 2), 3, "%02X", buf[i]);
	ret = kstrtoul(hex_len, 16, (unsigned long *)&rsa_import_modulus_len);
	if (ret) {
		pr_info("Cannot read modulus size from input buffer..\n");
		return -EINVAL;
	}
	idx += 2;

	memcpy(rsa_import_public_exponent, buf + idx, RSA_PUB_EXP_SIZE_MAX);
	idx += RSA_PUB_EXP_SIZE_MAX;
	memset(hex_len, 0, RSA_PARAM_LEN);
	for (i = idx, j = 0; i < idx + 1; i++, j++)
		snprintf(hex_len + (j * 2), 3, "%02X", buf[i]);
	ret = kstrtoul(hex_len, 16,
		      (unsigned long *)&rsa_import_public_exponent_len);
	if (ret) {
		pr_info("Cannot read pub exp size from input buffer..\n");
		return -EINVAL;
	}
	idx += 1;

	memcpy(rsa_import_pvt_exponent, buf + idx, RSA_KEY_SIZE_MAX);
	idx += RSA_KEY_SIZE_MAX;
	memset(hex_len, 0, RSA_PARAM_LEN);
	for (i = idx, j = 0; i < idx + 2; i++, j++)
		snprintf(hex_len + (j * 2), 3, "%02X", buf[i]);
	ret = kstrtoul(hex_len, 16,
		      (unsigned long *)&rsa_import_pvt_exponent_len);
	if (ret) {
		pr_info("Cannot read pvt exp size from input buffer..\n");
		return -EINVAL;
	}
	idx += 2;

	return count;
}

static ssize_t
import_rsa_key_blob(struct device *dev, struct device_attribute *attr,
		   char *buf)
{
	int rc = 0;
	size_t req_size = 0;
	size_t resp_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_req_addr = 0;
	dma_addr_t dma_resp_addr = 0;

	struct qti_storage_service_rsa_import_key_cmd_t *req_ptr = NULL;
	struct qti_storage_service_rsa_import_key_resp_t *resp_ptr = NULL;

	memset(rsa_key_blob, 0, RSA_KEY_BLOB_SIZE);
	rsa_key_blob_len = 0;

	if (rsa_import_pvt_exponent_len == 0 ||
	   rsa_import_public_exponent_len == 0 || rsa_import_modulus_len == 0) {
		pr_err("Please provide key to import key blob\n");
		return -EINVAL;
	}

	if (rsa_import_pvt_exponent_len > RSA_KEY_SIZE_MAX ||
	   rsa_import_public_exponent_len > RSA_PUB_EXP_SIZE_MAX ||
	   rsa_import_modulus_len > RSA_KEY_SIZE_MAX) {
		pr_info("Invalid key\n");
		pr_info("Both pvt and pub exponent len less than 512 bytes\n");
		pr_info("Modulus len should be less than 4096 bytes\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_storage_service_rsa_import_key_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_rsa_import_key_cmd_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	resp_size = sizeof(struct qti_storage_service_rsa_import_key_resp_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	resp_ptr = (struct qti_storage_service_rsa_import_key_resp_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_resp_addr, GFP_KERNEL);
	if (!resp_ptr)
		return -ENOMEM;

	req_ptr->key_blob.key_material = (u64)dma_rsa_key_blob;
	req_ptr->cmd_id = QTI_STOR_SVC_RSA_IMPORT_KEY;
	memcpy(req_ptr->modulus, rsa_import_modulus, rsa_import_modulus_len);
	req_ptr->modulus_len = rsa_import_modulus_len;
	memcpy(req_ptr->public_exponent, rsa_import_public_exponent,
	      rsa_import_public_exponent_len);
	req_ptr->public_exponent_len = rsa_import_public_exponent_len;
	pr_info("rsa pad scheme used = %u\n",cur_rsa_pad_scheme);
	req_ptr->digest_pad_type = cur_rsa_pad_scheme;
	memcpy(req_ptr->pvt_exponent, rsa_import_pvt_exponent,
	      rsa_import_pvt_exponent_len);
	req_ptr->pvt_exponent_len = rsa_import_pvt_exponent_len;
	req_ptr->key_blob.key_material_len = RSA_KEY_BLOB_SIZE;

	rc = qti_scm_tls_hardening(dma_req_addr, req_size,
				   dma_resp_addr, resp_size,
				   CLIENT_CMD_CRYPTO_RSA_64);
	if (rc) {
		pr_err("SCM Call failed..SCM Call return value = %d\n", rc);
		goto err_end;
	}


	if (resp_ptr->status) {
		rc = resp_ptr->status;
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	rsa_key_blob_len = RSA_KEY_BLOB_SIZE;
	memcpy(buf, rsa_key_blob, rsa_key_blob_len);
	rsa_key_blob_buf_valid = 1;

	goto end;

err_end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);

	return rc;

end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);

	return rsa_key_blob_len;
}

static ssize_t
store_rsa_key_blob(struct device *dev, struct device_attribute *attr,
		  const char *buf, size_t count)
{
	memset(rsa_key_blob, 0, RSA_KEY_BLOB_SIZE);
	rsa_key_blob_len = 0;

	if (count != RSA_KEY_BLOB_SIZE) {
		pr_info("Invalid input\n");
		pr_info("Key blob length is %lu bytes\n", (unsigned long)count);
		pr_info("Key blob length must be %u bytes\n",
		       (unsigned int)RSA_KEY_BLOB_SIZE);
		return -EINVAL;
	}

	rsa_key_blob_len = count;
	memcpy(rsa_key_blob, buf, rsa_key_blob_len);
	rsa_key_blob_buf_valid = 1;

	return count;
}

static ssize_t
store_rsa_plain_data(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count)
{
	static void __iomem *file_buf;
	struct file *file;
	struct kstat st;
	char *file_name;
	loff_t pos = 0;
	long size;
	int ret;

	rsa_plain_data_buf = memset(rsa_plain_data_buf, 0,
				   MAX_RSA_PLAIN_DATA_SIZE);
	rsa_plain_data_len = 0;

	file_name = kzalloc(count+1, GFP_KERNEL);
	if (file_name == NULL)
		return -ENOMEM;

	strlcpy(file_name, buf, count+1);

	file = filp_open(file_name, O_RDONLY, 0);
	if (IS_ERR(file)) {
		pr_err("%s File open failed\n", file_name);
		ret = -EBADF;
		goto free_mem;
	}

	ret = vfs_getattr(&file->f_path, &st, STATX_SIZE, AT_STATX_SYNC_AS_STAT);
	if (ret) {
		pr_err("get file attributes failed\n");
		goto file_close;
	}
	size = (long)st.size;

	if (size == 0 || size > MAX_RSA_PLAIN_DATA_SIZE) {
		pr_info("Invalid input\n");
		pr_info("Plain data length is %lu bytes\n",
		       (unsigned long)count);
		pr_info("Plain data length must be > 0 bytes and <= %u bytes\n",
		       (unsigned int)MAX_RSA_PLAIN_DATA_SIZE);
		ret = -EINVAL;
		goto file_close;
	}

	rsa_plain_data_len = size;
	ret = kernel_read(file, rsa_plain_data_buf, rsa_plain_data_len, &pos);

	if (ret != rsa_plain_data_len) {
		pr_err("%s file read failed\n", file_name);
		goto un_map;
	}

	ret = count;

un_map:
	iounmap(file_buf);
file_close:
	filp_close(file, NULL);
free_mem:
	kfree(file_name);
	return ret;
}

static ssize_t
show_rsa_signed_data(struct device *dev, struct device_attribute *attr,
		    char *buf)
{
	int rc = 0;
	struct qti_storage_service_rsa_sign_data_cmd_t *req_ptr = NULL;
	struct qti_storage_service_rsa_sign_data_resp_t *resp_ptr = NULL;
	size_t req_size = 0;
	size_t resp_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_req_addr = 0;
	dma_addr_t dma_resp_addr = 0;

	memset(rsa_sign_data_buf, 0, MAX_RSA_SIGN_DATA_SIZE);
	rsa_sign_data_len = 0;

	if (rsa_key_blob_len == 0 || rsa_plain_data_len == 0) {
		pr_err("Invalid input\n");
		pr_info("Need key blob and plain data for RSA Signing\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_storage_service_rsa_sign_data_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_rsa_sign_data_cmd_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	resp_size = sizeof(struct qti_storage_service_rsa_sign_data_resp_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	resp_ptr = (struct qti_storage_service_rsa_sign_data_resp_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_resp_addr, GFP_KERNEL);
	if (!resp_ptr)
		return -ENOMEM;

	req_ptr->key_blob.key_material = (u64)dma_rsa_key_blob;
	req_ptr->plain_data = (u64)dma_rsa_plain_data_buf;
	req_ptr->output_buffer = (u64)dma_rsa_sign_data_buf;
	req_ptr->cmd_id = QTI_STOR_SVC_RSA_SIGN_DATA;
	req_ptr->key_blob.key_material_len = RSA_KEY_BLOB_SIZE;
	req_ptr->plain_data_len = rsa_plain_data_len;
	req_ptr->output_len = MAX_RSA_SIGN_DATA_SIZE;

	rc = qti_scm_tls_hardening(dma_req_addr, req_size,
				   dma_resp_addr, resp_size,
				   CLIENT_CMD_CRYPTO_RSA_64);
	if (rc) {
		pr_err("SCM Call failed..SCM Call return value = %d\n", rc);
		goto err_end;
	}

	if (resp_ptr->status != 0) {
		rc = resp_ptr->status;
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	rsa_sign_data_len = resp_ptr->sealed_data_len;
	memcpy(buf, rsa_sign_data_buf, rsa_sign_data_len);

	goto end;

err_end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);

	return rc;

end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);

	return rsa_sign_data_len;
}

static ssize_t
store_rsa_signed_data(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
	rsa_sign_data_buf = memset(rsa_sign_data_buf, 0,
				  MAX_RSA_SIGN_DATA_SIZE);
	rsa_sign_data_len = 0;

	if (count == 0 || count > MAX_RSA_SIGN_DATA_SIZE) {
		pr_info("Invalid input\n");
		pr_info("Signed data length is %lu\n", (unsigned long)count);
		pr_info("Signed data length must be > 0 bytes and <= %u bytes\n",
		       (unsigned int)MAX_RSA_SIGN_DATA_SIZE);
		return -EINVAL;
	}

	rsa_sign_data_len = count;
	memcpy(rsa_sign_data_buf, buf, rsa_sign_data_len);

	return count;
}

static ssize_t
verify_rsa_signed_data(struct device *dev, struct device_attribute *attr,
		      char *buf)
{
	int rc = 0;
	struct qti_storage_service_rsa_verify_data_cmd_t *req_ptr = NULL;
	struct qti_storage_service_rsa_verify_data_resp_t *resp_ptr = NULL;
	size_t req_size = 0;
	size_t resp_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_req_addr = 0;
	dma_addr_t dma_resp_addr = 0;
	const char *message = NULL;
	int message_len = 0;

	if (rsa_key_blob_len == 0 || rsa_sign_data_len == 0
	   || rsa_plain_data_len == 0) {
		pr_err("Invalid input\n");
		pr_info("Need key blob, signed data and plain data for RSA Verification\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_storage_service_rsa_verify_data_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_rsa_verify_data_cmd_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	resp_size = sizeof(struct qti_storage_service_rsa_verify_data_resp_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	resp_ptr = (struct qti_storage_service_rsa_verify_data_resp_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_resp_addr, GFP_KERNEL);
	if (!resp_ptr)
		return -ENOMEM;

	req_ptr->key_blob.key_material = (u64)dma_rsa_key_blob;
	req_ptr->signed_data = (u64)dma_rsa_sign_data_buf;
	req_ptr->data = (u64)dma_rsa_plain_data_buf;
	req_ptr->cmd_id = QTI_STOR_SVC_RSA_VERIFY_SIGNATURE;
	req_ptr->key_blob.key_material_len = rsa_key_blob_len;
	req_ptr->signed_dlen = rsa_sign_data_len;
	req_ptr->data_len = rsa_plain_data_len;

	rc = qti_scm_tls_hardening(dma_req_addr, req_size,
				   dma_resp_addr, resp_size,
				   CLIENT_CMD_CRYPTO_RSA_64);
	if (rc) {
		pr_err("SCM Call failed..SCM Call return value = %d\n", rc);
		goto err_end;
	}

	if (resp_ptr->status != 0) {
		rc = resp_ptr->status;
		pr_err("Response status failure..return value = %d\n", rc);
		message = "RSA Verification Failed\0\n";
		message_len = strlen(message) + 1;
		memcpy(buf, message, message_len);
		goto err_end;
	} else {
		message = "RSA Verification Successful\0\n";
		message_len = strlen(message) + 1;
		memcpy(buf, message, message_len);
	}

	goto end;

err_end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);

	return rc;

end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, PAGE_SIZE, resp_ptr, dma_resp_addr);

	return message_len;
}

static ssize_t store_rsa_pad_scheme(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	uint32_t pad_scheme;

	if(kstrtouint(buf, 10, &pad_scheme))
		return -EINVAL;

	if (pad_scheme == 1)
		cur_rsa_pad_scheme = QTI_STOR_SVC_RSA_DIGEST_PAD_PKCS115_SHA2_256;
	else if (pad_scheme == 2)
		cur_rsa_pad_scheme = QTI_STOR_SVC_RSA_DIGEST_PAD_PSS_SHA2_256;
	else
		pr_info("Provide a valid padding scheme\n");

	return count;
}

static ssize_t show_rsa_pad_scheme(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	char *msg;
	msg = (cur_rsa_pad_scheme == 2) ? "PSS\n" : "PKCS\n";
	memcpy(buf, msg, strlen(msg)+1);
	return strlen(msg)+1;
}

static ssize_t show_rsa_update_keyblob(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	struct qti_storage_service_rsa_update_keyblob_cmd_t *req_ptr = NULL;
	struct qti_storage_service_rsa_update_keyblob_data_resp_t *resp_ptr = NULL;
	size_t req_size = 0;
	size_t resp_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_req_addr = 0;
	dma_addr_t dma_resp_addr = 0;
	struct qti_storage_service_rsa_key_t *temp = NULL;
	int rc = 0;

	dev = qdev;

	if(!rsa_key_blob_buf_valid || rsa_key_blob_len == 0) {
		pr_info("RSA Key blob is invalid. Input the key blob and try again.\n");
		return -EINVAL;
	}

	temp = (struct qti_storage_service_rsa_key_t *)rsa_key_blob;
	if (temp->pad_algo == cur_rsa_pad_scheme) {
		pr_info("Padding type already matches with keyblob\n");
		memcpy(buf, rsa_key_blob, rsa_key_blob_len);
		return rsa_key_blob_len;
	}

	req_size = sizeof(struct qti_storage_service_rsa_update_keyblob_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_rsa_update_keyblob_cmd_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;
	resp_size = sizeof(struct qti_storage_service_rsa_update_keyblob_data_resp_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	resp_ptr = (struct qti_storage_service_rsa_update_keyblob_data_resp_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_resp_addr, GFP_KERNEL);
	if (!resp_ptr)
		return -ENOMEM;

	req_ptr->cmd_id = CRYPTO_STORAGE_UPDATE_KEYBLOB;
	req_ptr->key_blob.key_material = (u64)dma_rsa_key_blob;
	req_ptr->key_blob.key_material_len = RSA_KEY_BLOB_SIZE;
	req_ptr->pad_algo = cur_rsa_pad_scheme;

	rc = qti_scm_tls_hardening(dma_req_addr, req_size,
				   dma_resp_addr, resp_size,
				   CLIENT_CMD_CRYPTO_RSA_64);

	if (rc) {
		pr_err("SCM call failed..SCM Call return value = %d\n", rc);
		goto err_end;
	}

	if (resp_ptr->status) {
		rc = resp_ptr->status;
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	rsa_key_blob_len = resp_ptr->key_blob_size;
	memcpy(buf, rsa_key_blob, rsa_key_blob_len);

	goto end;

err_end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, dma_buf_size, resp_ptr, dma_resp_addr);

	return rc;

end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	dma_buf_size = PAGE_SIZE * (1 << get_order(resp_size));
	dma_free_coherent(dev, dma_buf_size, resp_ptr, dma_resp_addr);

	return rsa_key_blob_len;
}

static int __init rsa_sec_key_init(struct device *dev)
{
	int err = 0;
	size_t dma_buf_size = 0;

	rsa_sec_kobj = kobject_create_and_add("rsa_sec_key", rsa_sec_kobj);

	if (!rsa_sec_kobj) {
		pr_info("Failed to register rsa_sec_key sysfs\n");
		return -ENOMEM;
	}

	err = sysfs_create_group(rsa_sec_kobj, &rsa_sec_key_attr_grp);

	if (err) {
		kobject_put(rsa_sec_kobj);
		rsa_sec_kobj = NULL;
		return err;
	}

	dma_buf_size = PAGE_SIZE * (1 << get_order(RSA_KEY_BLOB_SIZE));
	buf_rsa_key_blob = dma_alloc_coherent(dev, dma_buf_size,
					&dma_rsa_key_blob, GFP_KERNEL);

	dma_buf_size = PAGE_SIZE * (1 << get_order(RSA_KEY_SIZE_MAX));
	buf_rsa_import_modulus = dma_alloc_coherent(dev, dma_buf_size,
				&dma_rsa_import_modulus, GFP_KERNEL);

	dma_buf_size = PAGE_SIZE * (1 << get_order(RSA_PUB_EXP_SIZE_MAX));
	buf_rsa_import_public_exponent = dma_alloc_coherent(dev, dma_buf_size,
				&dma_rsa_import_public_exponent, GFP_KERNEL);

	dma_buf_size = PAGE_SIZE * (1 << get_order(RSA_KEY_SIZE_MAX));
	buf_rsa_import_pvt_exponent = dma_alloc_coherent(dev, dma_buf_size,
				&dma_rsa_import_pvt_exponent, GFP_KERNEL);

	dma_buf_size = PAGE_SIZE * (1 << get_order(MAX_RSA_SIGN_DATA_SIZE));
	buf_rsa_sign_data_buf = dma_alloc_coherent(dev, dma_buf_size,
					&dma_rsa_sign_data_buf, GFP_KERNEL);

	dma_buf_size = PAGE_SIZE * (1 << get_order(MAX_RSA_PLAIN_DATA_SIZE));
	buf_rsa_plain_data_buf = dma_alloc_coherent(dev, dma_buf_size,
					&dma_rsa_plain_data_buf, GFP_KERNEL);

	if (!buf_rsa_key_blob || !buf_rsa_import_modulus ||
	   !buf_rsa_import_public_exponent || !buf_rsa_import_pvt_exponent ||
	   !buf_rsa_sign_data_buf || !buf_rsa_plain_data_buf) {
		pr_err("Cannot allocate memory for RSA secure-key ops\n");

		if (buf_rsa_key_blob) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(RSA_KEY_BLOB_SIZE));
			dma_free_coherent(dev, dma_buf_size, buf_rsa_key_blob,
							dma_rsa_key_blob);
		}

		if (buf_rsa_import_modulus) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(RSA_KEY_SIZE_MAX));
			dma_free_coherent(dev, dma_buf_size,
					buf_rsa_import_modulus,
					dma_rsa_import_modulus);
		}

		if (buf_rsa_import_public_exponent) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(RSA_PUB_EXP_SIZE_MAX));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_import_public_exponent,
						dma_rsa_import_public_exponent);
		}

		if (buf_rsa_import_pvt_exponent) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(RSA_KEY_SIZE_MAX));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_import_pvt_exponent,
						dma_rsa_import_pvt_exponent);
		}

		if (buf_rsa_sign_data_buf) {
			dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_RSA_SIGN_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_sign_data_buf,
						dma_rsa_sign_data_buf);
		}

		if(buf_rsa_plain_data_buf) {
			dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_RSA_PLAIN_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_plain_data_buf,
						dma_rsa_plain_data_buf);
		}

		sysfs_remove_group(rsa_sec_kobj, &rsa_sec_key_attr_grp);
		kobject_put(rsa_sec_kobj);
		rsa_sec_kobj = NULL;

		return -ENOMEM;
	}

	rsa_key_blob = (uint8_t*) buf_rsa_key_blob;
	rsa_import_modulus = (uint8_t*) buf_rsa_import_modulus;
	rsa_import_public_exponent = (uint8_t*) buf_rsa_import_public_exponent;
	rsa_import_pvt_exponent = (uint8_t*) buf_rsa_import_pvt_exponent;
	rsa_sign_data_buf = (uint8_t*) buf_rsa_sign_data_buf;
	rsa_plain_data_buf = (uint8_t*) buf_rsa_plain_data_buf;
	rsa_key_blob_buf_valid = 0;

	return 0;
}

static int __init sec_key_init(struct device *dev)
{
	int err = 0;
	size_t dma_buf_size = 0;

	dev = qdev;

	sec_kobj = kobject_create_and_add("sec_key", NULL);

	if (!sec_kobj) {
		pr_info("Failed to register sec_key sysfs\n");
		return -ENOMEM;
	}

	err = sysfs_create_group(sec_kobj, &sec_key_attr_grp);

	if (err) {
		kobject_put(sec_kobj);
		sec_kobj = NULL;
		return err;
	}

	if (props->aes_v2) {
		err = sysfs_create_group(sec_kobj, &sec_key_aesv2_attr_grp);
		if (err)
			pr_debug("TZ AES v2 sysfs creation failed with error %d\n",err);
	}

	dma_buf_size = PAGE_SIZE * (1 << get_order(KEY_SIZE));
	buf_key = dma_alloc_coherent(dev, dma_buf_size,
					&dma_key, GFP_KERNEL);

	dma_buf_size = PAGE_SIZE * (1 << get_order(KEY_BLOB_SIZE));
	buf_key_blob = dma_alloc_coherent(dev, dma_buf_size,
					&dma_key_blob, GFP_KERNEL);

	dma_buf_size = PAGE_SIZE * (1 << get_order(MAX_ENCRYPTED_DATA_SIZE));
	buf_sealed_buf = dma_alloc_coherent(dev, dma_buf_size,
					&dma_sealed_data, GFP_KERNEL);

	dma_buf_size = PAGE_SIZE * (1 << get_order(MAX_PLAIN_DATA_SIZE));
	buf_unsealed_buf = dma_alloc_coherent(dev, dma_buf_size,
					&dma_unsealed_data, GFP_KERNEL);

	dma_buf_size = PAGE_SIZE * (1 << get_order(AES_BLOCK_SIZE));
	buf_iv = dma_alloc_coherent(dev, dma_buf_size,
					&dma_iv_data, GFP_KERNEL);

	dma_buf_size = PAGE_SIZE * (1 << get_order(MAX_KEY_HANDLE_SIZE));
	key_handle = dma_alloc_coherent(dev, dma_buf_size,
					&dma_key_handle, GFP_KERNEL);
	dma_buf_size = PAGE_SIZE * (1 << get_order(MAX_KEY_HANDLE_SIZE));
	aes_key_handle = dma_alloc_coherent(dev, dma_buf_size,
					&dma_aes_key_handle, GFP_KERNEL);

	if (!buf_key || !buf_key_blob || !buf_sealed_buf ||
	    !buf_unsealed_buf || !buf_iv || !key_handle || !aes_key_handle) {
		pr_err("Cannot allocate memory for secure-key ops\n");

		if (buf_key) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(KEY_SIZE));
			dma_free_coherent(dev, dma_buf_size, buf_key,
							dma_key);
		}

		if (buf_key_blob) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(KEY_BLOB_SIZE));
			dma_free_coherent(dev, dma_buf_size, buf_key_blob,
							dma_key_blob);
		}

		if (buf_sealed_buf) {
			dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_ENCRYPTED_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
							buf_sealed_buf,
							dma_sealed_data);
		}

		if (buf_unsealed_buf) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(MAX_PLAIN_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
							buf_unsealed_buf,
							dma_unsealed_data);
		}

		if (buf_iv) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(AES_BLOCK_SIZE));
			dma_free_coherent(dev, dma_buf_size,
							buf_iv,
							dma_iv_data);
		}

		if (key_handle) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(MAX_KEY_HANDLE_SIZE));
			dma_free_coherent(dev, dma_buf_size,
					key_handle, dma_key_handle);
		}

		if (aes_key_handle) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(MAX_KEY_HANDLE_SIZE));
			dma_free_coherent(dev, dma_buf_size, aes_key_handle,
					 dma_aes_key_handle);
		}

		sysfs_remove_group(sec_kobj, &sec_key_attr_grp);
		if (props->aes_v2)
			sysfs_remove_group(sec_kobj, &sec_key_aesv2_attr_grp);
		kobject_put(sec_kobj);
		sec_kobj = NULL;

		return -ENOMEM;
	}

	key = (uint8_t*) buf_key;
	key_blob = (uint8_t*) buf_key_blob;
	sealed_buf = (uint8_t*) buf_sealed_buf;
	unsealed_buf = (uint8_t*) buf_unsealed_buf;
	ivdata = (uint8_t*) buf_iv;

	return 0;
}

static ssize_t mdt_write(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr,
	char *buf, loff_t pos, size_t count)
{
	uint8_t *tmp;
	/*
	 * Position '0' means new file being written,
	 * Hence allocate new memory after freeing already allocated mem if any
	 */
	if (pos == 0) {
		kfree(mdt_file);
		mdt_file = kzalloc((count) * sizeof(uint8_t), GFP_KERNEL);
	} else {
		tmp = mdt_file;
		mdt_file = krealloc(tmp,
			(pos + count) * sizeof(uint8_t), GFP_KERNEL);
	}

	if (!mdt_file)
		return -ENOMEM;

	memcpy((mdt_file + pos), buf, count);
	mdt_size = pos + count;
	return count;
}

static ssize_t seg_write(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr,
	char *buf, loff_t pos, size_t count)
{
	uint8_t *tmp;
	if (pos == 0) {
		kfree(seg_file);
		seg_file = kzalloc((count) * sizeof(uint8_t), GFP_KERNEL);
	} else {
		tmp = seg_file;
		seg_file = krealloc(tmp, (pos + count) * sizeof(uint8_t),
					GFP_KERNEL);
	}

	if (!seg_file)
		return -ENOMEM;

	memcpy((seg_file + pos), buf, count);
	seg_size = pos + count;
	return count;
}

static ssize_t auth_write(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr,
	char *buf, loff_t pos, size_t count)
{
	uint8_t *tmp = NULL;

	if (pos == 0) {
		kfree(auth_file);
		auth_file = kzalloc((count) * sizeof(uint8_t), GFP_KERNEL);
	} else {
		tmp = auth_file;
		auth_file = krealloc(tmp, (pos + count) * sizeof(uint8_t),
					GFP_KERNEL);
	}

	if (!auth_file) {
		kfree(tmp);
		return -ENOMEM;
	}

	memcpy((auth_file + pos), buf, count);
	auth_size = pos + count;

	return count;
}

static int qseecom_unload_app(void)
{
	struct qseecom_unload_ireq req;
	struct qseecom_command_scm_resp resp;
	int ret = 0;
	uint32_t cmd_id = 0;
	uint32_t smc_id = 0;

	cmd_id = QSEOS_APP_SHUTDOWN_COMMAND;

	smc_id = QTI_SYSCALL_CREATE_SMC_ID(QTI_OWNER_QSEE_OS, QTI_SVC_APP_MGR,
					   QTI_CMD_UNLOAD_APP_ID);

	req.app_id = qsee_app_id;

	/* SCM_CALL to unload the app */
	ret = qti_scm_qseecom_unload(smc_id, cmd_id, &req,
				     sizeof(struct qseecom_unload_ireq),
				     &resp, sizeof(resp));
	if (ret) {
		pr_err("scm_call to unload app (id = %d) failed\n", req.app_id);
		pr_info("scm call ret value = %d\n", ret);
		return ret;
	}

	pr_info("App id %d now unloaded\n", req.app_id);

	return 0;
}

static int qtiapp_test(struct device *dev, void *input,
		      void *output, int input_len, int option)
{
	int ret = 0;
	int ret1, ret2;

	union qseecom_client_send_data_ireq send_data_req;
	struct qseecom_command_scm_resp resp;
	struct qsee_send_cmd_rsp *msgrsp; /* response data sent from QSEE */
	struct qsee_64_send_cmd *msgreq;

	void *buf = NULL;
	dma_addr_t dma_buf = 0;
	dma_addr_t dma_msgreq;
	dma_addr_t dma_msgresp;

	dev = qdev;

	/*
	 * Using dma_alloc_coherent to avoid colliding with input pointer's
	 * allocated page, since qsee_register_shared_buffer() in sampleapp
	 * checks if input ptr is in secure area. Page where msgreq/msgrsp
	 * is allocated is added to blacklisted area by sampleapp and added
	 * as secure memory region, hence input data (shared buffer)
	 * cannot be in that secure memory region
	 */
	buf = dma_alloc_coherent(dev, PAGE_SIZE, &dma_buf, GFP_KERNEL);
	if (!buf) {
		pr_err("Failed to allocate page\n");
		return -ENOMEM;
	}
	/*
	 * Getting virtual page address. pg_tmp will be pointing to
	 * first page structure
	 */

	msgreq = (struct qsee_64_send_cmd *)buf;
	msgrsp = (struct qsee_send_cmd_rsp *)((uint8_t *) msgreq +
					sizeof(struct qsee_64_send_cmd));
	dma_msgreq = dma_buf;
	dma_msgresp = (dma_addr_t)((uint8_t *)dma_msgreq +
				sizeof(struct qsee_64_send_cmd));

	/*
	 * option = 1 -> Basic Multiplication, option = 2 -> Encryption,
	 * option = 3 -> Decryption, option = 4 -> Crypto Function
	 * option = 5 -> Authorized OTP fusing
	 * option = 6 -> Log Bitmask function, option = 7 -> Fuse test
	 * option = 8 -> Miscellaneous function
	 * option = 9 -> AES Encryption, option = 10 -> AES Decryption
	 * option = 11 -> RSA Crypto
	 */

	switch (option) {
	case QTI_APP_BASIC_DATA_TEST_ID:
		msgreq->cmd_id = CLIENT_CMD1_BASIC_DATA;
		msgreq->data = *((dma_addr_t *)input);
		break;
	case QTI_APP_ENC_TEST_ID:
		msgreq->cmd_id = CLIENT_CMD8_RUN_CRYPTO_ENCRYPT;
		break;
	case QTI_APP_DEC_TEST_ID:
		msgreq->cmd_id = CLIENT_CMD9_RUN_CRYPTO_DECRYPT;
		break;
	case QTI_APP_CRYPTO_TEST_ID:
		msgreq->cmd_id = CLIENT_CMD8_RUN_CRYPTO_TEST;
		break;
	case QTI_APP_AUTH_OTP_TEST_ID:
		if (!auth_file) {
			pr_err("No OTP file provided\n");
			return -ENOMEM;
		}

		msgreq->cmd_id = CLIENT_CMD_AUTH;
		msgreq->data = dma_map_single(dev, auth_file,
					      auth_size, DMA_TO_DEVICE);
		ret = dma_mapping_error(dev, msgreq->data);
		if (ret) {
			pr_err("DMA Mapping Error: otp_buffer %d",
				ret);
			return ret;
		}

		break;
	case QTI_APP_LOG_BITMASK_TEST_ID:
		msgreq->cmd_id = CLIENT_CMD53_RUN_LOG_BITMASK_TEST;
		break;
	case QTI_APP_FUSE_TEST_ID:
		msgreq->cmd_id = CLIENT_CMD18_RUN_FUSE_TEST;
		break;
	case QTI_APP_MISC_TEST_ID:
		msgreq->cmd_id = CLIENT_CMD13_RUN_MISC_TEST;
		break;
	case QTI_APP_FUSE_BLOW_ID:
		msgreq->cmd_id = CLIENT_CMD43_RUN_FUSE_BLOW;
		msgreq->data = (dma_addr_t)input;
		msgreq->len = input_len;
		break;
	case QTI_APP_AES_ENCRYPT_ID:
		msgreq->cmd_id = CLIENT_CMD40_RUN_AES_ENCRYPT;
		msgreq->data = (dma_addr_t)input;
		msgreq->len = input_len;
		break;
	case QTI_APP_AES_DECRYPT_ID:
		msgreq->cmd_id = CLIENT_CMD41_RUN_AES_DECRYPT;
		msgreq->data = (dma_addr_t)input;
		msgreq->len = input_len;
		break;
	case QTI_APP_RSA_ENC_DEC_ID:
		msgreq->cmd_id = CLIENT_CMD42_RUN_RSA_CRYPT;
		msgreq->data = (dma_addr_t)input;
		msgreq->len = input_len;
		break;
	case QTI_APP_KEY_DERIVE_TEST:
		msgreq->cmd_id = CLIENT_CMD121_RUN_KEY_DERIVE_TEST;
		msgreq->data = (dma_addr_t)input;
		msgreq->len = input_len;
		break;
	case QTI_APP_CLEAR_KEY:
		msgreq->cmd_id = CLIENT_CMD122_CLEAR_KEY;
		msgreq->data = *((dma_addr_t *)input);
		break;
	default:
		pr_err("Invalid Option\n");
		goto fn_exit;
	}
	if (option == QTI_APP_ENC_TEST_ID || option == QTI_APP_DEC_TEST_ID) {
		msgreq->data = dma_map_single(dev, input,
				input_len, DMA_TO_DEVICE);
		msgreq->data2 = dma_map_single(dev, output,
				input_len, DMA_FROM_DEVICE);
		ret1 = dma_mapping_error(dev, msgreq->data);
		ret2 = dma_mapping_error(dev, msgreq->data2);

		if (ret1 || ret2) {
			pr_err("\nDMA Mapping Error:input:%d output:%d",
			      ret1, ret2);
			if (!ret1) {
				dma_unmap_single(dev, msgreq->data,
					input_len, DMA_TO_DEVICE);
			}

			if (!ret2) {
				dma_unmap_single(dev, msgreq->data2,
					input_len, DMA_FROM_DEVICE);
			}
			return ret1 ? ret1 : ret2;
		}
		msgreq->test_buf_size = input_len;
		msgreq->len = input_len;
	}
	send_data_req.v1.app_id = qsee_app_id;

	send_data_req.v1.req_ptr = dma_msgreq;
	send_data_req.v1.rsp_ptr = dma_msgresp;

	send_data_req.v1.req_len = sizeof(struct qsee_64_send_cmd);
	send_data_req.v1.rsp_len = sizeof(struct qsee_send_cmd_rsp);
	ret = qti_scm_qseecom_send_data(&send_data_req,
			sizeof(send_data_req.v2), &resp, sizeof(resp));

	if (option == QTI_APP_ENC_TEST_ID || option == QTI_APP_DEC_TEST_ID) {
		dma_unmap_single(dev, msgreq->data,
					input_len, DMA_TO_DEVICE);
		dma_unmap_single(dev, msgreq->data2,
					input_len, DMA_FROM_DEVICE);

	}

	if (ret) {
		pr_err("qseecom_scm_call failed with err: %d\n", ret);
		goto fn_exit;
	}

	if (resp.result == QSEOS_RESULT_INCOMPLETE) {
		pr_err("Result incomplete\n");
		ret = -EINVAL;
		goto fn_exit;
	} else {
		if (resp.result != QSEOS_RESULT_SUCCESS) {
			pr_err("Response result %lu not supported\n",
							resp.result);
			ret = -EINVAL;
			goto fn_exit;
		} else {
			if (option == QTI_APP_CRYPTO_TEST_ID) {
				if (!msgrsp->status) {
					pr_info("Crypto operation success\n");
				} else {
					pr_info("Crypto operation failed\n");
					goto fn_exit;
				}
			}
		}
	}

	if (option == QTI_APP_BASIC_DATA_TEST_ID) {
		if (msgrsp->status) {
			pr_err("Input size exceeded supported range\n");
			ret = -EINVAL;
		}
		basic_output = msgrsp->data;
	} else if (option == QTI_APP_AUTH_OTP_TEST_ID) {
		if (msgrsp->status) {
			pr_err("Auth OTP failed with response %d\n",
							msgrsp->status);
			ret = -EIO;
		} else
			pr_info("Auth and Blow Success");
	} else if (option == QTI_APP_LOG_BITMASK_TEST_ID) {
		if (!msgrsp->status) {
			pr_info("Log Bitmask test Success\n");
		} else {
			pr_info("Log Bitmask test failed\n");
			goto fn_exit;
		}
	} else if (option == QTI_APP_FUSE_TEST_ID) {
		if (!msgrsp->status) {
			pr_info("Fuse test success\n");
		} else {
			pr_info("Fuse test failed\n");
			goto fn_exit;
		}
	} else if (option == QTI_APP_MISC_TEST_ID) {
		if (!msgrsp->status) {
			pr_info("Misc test success\n");
		} else {
			pr_info("Misc test failed\n");
			goto fn_exit;
		}
	} else if (option == QTI_APP_RSA_ENC_DEC_ID ||
			option == QTI_APP_FUSE_BLOW_ID) {
		if (msgrsp->status) {
			ret = -EINVAL;
		}
	} else if (option == QTI_APP_KEY_DERIVE_TEST ||
			option == QTI_APP_AES_ENCRYPT_ID ||
			option == QTI_APP_AES_DECRYPT_ID) {
		if (msgrsp->status == KEY_HANDLE_OUT_OF_SLOT) {
			pr_info("Key handle out of slot. Clear a key and try again!\n");
		}
		if (msgrsp->status) {
			pr_info("QTIAPP DERIVE/ENC/DEC Failed with status = %d\n",
				msgrsp->status);
			ret = -EINVAL;
		}
	} else if (option == QTI_APP_CLEAR_KEY) {
		if(msgrsp->status) {
			ret = -EINVAL;
		}
	}

fn_exit:
	dma_free_coherent(dev, PAGE_SIZE, buf, dma_buf);
	if (option == QTI_APP_AUTH_OTP_TEST_ID) {
		dma_unmap_single(dev, msgreq->data, auth_size,
							DMA_TO_DEVICE);
	}
	return ret;
}

/*
 * store_aes_derive_key_qtiapp()
 * Function to store aes derive key handle (in QTIAPP)
 */
static ssize_t
store_aes_derive_key_qtiapp(struct device *dev, struct device_attribute *attr,
	 const char *buf, size_t count)
{
	unsigned long long val;

	if (kstrtoull(buf, 10, &val))
		return -EINVAL;

	*aes_key_handle = val;
	if (*aes_key_handle == INVALID_AES_KEY_HANDLE_VAL) {
		pr_info("Invalid aes key handle: %lu\n",
			(unsigned long)*aes_key_handle);
		return -EINVAL;
	}

	return count;
}

/*
 * show_aes_derive_key()
 * Function to derive aes_key and get key_handle (in QTIAPP)
 */
static ssize_t show_aes_derive_key_qtiapp(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int rc = 0, i = 0;
	struct qti_storage_service_derive_key_cmd_t_v1 *req_ptr;
	size_t req_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_req_addr = 0;
	const char *message = NULL;
	int message_len = 0;

	if (!aes_source_data || !aes_context_data_len || !aes_bindings_data) {
		pr_info("Provide the required src data, bindings data and context data before encrypt/decrypt\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_storage_service_derive_key_cmd_t_v1);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_derive_key_cmd_t_v1 *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	req_ptr->policy.key_type = DEFAULT_KEY_TYPE;
	req_ptr->policy.destination = DEFAULT_POLICY_DESTINATION;
	req_ptr->hw_key_bindings.bindings = aes_bindings_data;
	req_ptr->source = aes_source_data;
	req_ptr->key = (u64) dma_aes_key_handle;
	req_ptr->mixing_key = 0;

	for (i = 0; i < MAX_CONTEXT_BUFFER_LEN_V1; i++)
		req_ptr->hw_key_bindings.context[i] = aes_context_data[i];
	req_ptr->hw_key_bindings.context_len = aes_context_data_len;

	rc = qtiapp_test(dev, (uint8_t *)dma_req_addr, NULL, req_size,
						QTI_APP_KEY_DERIVE_TEST);
	if (!rc) {
		message = "AES Key derive successful\n\0";
	} else {
		pr_err("SCM call failed..return value = %d\n", rc);
		message = "AES Key derive failed\n\0";
	}

	pr_info("key_handle is: %lu\n", (unsigned long)*aes_key_handle);

	message_len = strlen(message) + 1;
	memcpy(buf, message, message_len);

	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);
	return message_len;
}

static ssize_t store_aes_clear_key_qtiapp(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	uint32_t rc = 0;
	uint32_t key_handle;

	if (kstrtouint(buf, 10, &key_handle))
		return -EINVAL;

	rc = qtiapp_test(dev, &key_handle, NULL, 0, QTI_APP_CLEAR_KEY);

	if (!rc)
		pr_info("AES key =  %u cleared successfully\n",key_handle);
	else
		pr_info("AES key clear failed\n");

	return rc ? rc : count;
}

static ssize_t
show_aes_v2_decrypted_data_qtiapp(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct qti_crypto_service_decrypt_data_cmd_t_v2 *req_ptr = NULL;
	uint64_t req_size = 0;
	size_t dma_buf_size = 0;
	uint64_t output_len = 0;
	dma_addr_t dma_req_addr = 0;

	aes_unsealed_buf = memset(aes_unsealed_buf, 0, MAX_PLAIN_DATA_SIZE);
	output_len = aes_encrypted_len;

	if (aes_encrypted_len <= 0 || aes_encrypted_len % AES_BLOCK_SIZE) {
		pr_err("Invalid input %lld\n", aes_encrypted_len);
		pr_info("Encrypted data length for decryption should be multiple "
			"of AES block size(16)\n");
		return -EINVAL;
	}
	if (!aes_type || aes_type >= QTI_CRYPTO_SERVICE_AES_TYPE_MAX) {
		pr_info(" aes type needed for decryption\n");
		return -EINVAL;
	}
	if (!aes_key_handle) {
		pr_info("Derive the required key handle before encrypt/decrypt\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_crypto_service_decrypt_data_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_crypto_service_decrypt_data_cmd_t_v2 *)
					dma_alloc_coherent(dev, dma_buf_size,
						&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	req_ptr->key_handle = (unsigned long)*aes_key_handle;
	pr_info("key_handle is: %lu\n", (unsigned long)req_ptr->key_handle);
	req_ptr->v1.type = aes_type;
	req_ptr->v1.mode = aes_mode;
	req_ptr->v1.encrypted_data = (u64)dma_aes_sealed_buf;
	req_ptr->v1.output_buffer = (u64)dma_aes_unsealed_buf;
	req_ptr->v1.iv = (req_ptr->v1.mode == 0 ? 0 : (u64)dma_aes_ivdata);
	req_ptr->v1.iv_len = AES_BLOCK_SIZE;
	req_ptr->v1.encrypted_dlen = aes_encrypted_len;
	req_ptr->v1.output_len = output_len;

	rc = qtiapp_test(dev, (uint8_t *)dma_req_addr, NULL, req_size,
						QTI_APP_AES_DECRYPT_ID);
	if (rc) {
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	aes_decrypted_len = req_ptr->v1.output_len;
	memcpy(buf, aes_unsealed_buf, aes_decrypted_len);

goto end;

err_end:
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return rc;

end:
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return aes_decrypted_len;
}

static ssize_t
show_aes_decrypted_data_qtiapp(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct qti_crypto_service_decrypt_data_cmd_t *req_ptr = NULL;
	uint64_t req_size = 0;
	size_t dma_buf_size = 0;
	uint64_t output_len = 0;
	dma_addr_t dma_req_addr = 0;

	if (props->aes_v2) {
		rc = show_aes_v2_decrypted_data_qtiapp(dev, attr, buf);
		return rc;
	}

	aes_unsealed_buf = memset(aes_unsealed_buf, 0, MAX_PLAIN_DATA_SIZE);
	output_len = aes_encrypted_len;

	if (aes_encrypted_len <= 0 || aes_encrypted_len % AES_BLOCK_SIZE) {
		pr_err("Invalid input %lld\n", aes_encrypted_len);
		pr_info("Encrypted data length for decryption should be multiple "
			"of AES block size(16)\n");
		return -EINVAL;
	}
	if (!aes_type || aes_type >= QTI_CRYPTO_SERVICE_AES_TYPE_MAX) {
		pr_info(" aes type needed for decryption\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_crypto_service_decrypt_data_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_crypto_service_decrypt_data_cmd_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	req_ptr->type = aes_type;
	req_ptr->mode = aes_mode;
	req_ptr->encrypted_data = (u64)dma_aes_sealed_buf;
	req_ptr->output_buffer = (u64)dma_aes_unsealed_buf;
	req_ptr->iv = (req_ptr->mode == 0 ? 0 : (u64)dma_aes_ivdata);
	req_ptr->iv_len = AES_BLOCK_SIZE;
	req_ptr->encrypted_dlen = aes_encrypted_len;
	req_ptr->output_len = output_len;

	rc = qtiapp_test(dev, (uint8_t *)dma_req_addr, NULL, req_size,
						QTI_APP_AES_DECRYPT_ID);
	if (rc) {
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	aes_decrypted_len = req_ptr->output_len;
	memcpy(buf, aes_unsealed_buf, aes_decrypted_len);

goto end;

err_end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return rc;

end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return aes_decrypted_len;
}

static ssize_t
show_aes_v2_encrypted_data_qtiapp(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int rc = 0;
	struct qti_crypto_service_encrypt_data_cmd_t_v2 *req_ptr = NULL;
	uint64_t req_size = 0;
	size_t dma_buf_size = 0;
	uint64_t output_len = 0;
	dma_addr_t dma_req_addr = 0;

	aes_sealed_buf = memset(aes_sealed_buf, 0, MAX_ENCRYPTED_DATA_SIZE);
	output_len = aes_decrypted_len;

	if (aes_decrypted_len <= 0 || aes_decrypted_len % AES_BLOCK_SIZE) {
		pr_err("Invalid input %lld\n", aes_decrypted_len);
		pr_info("Input data length for encryption should be multiple"
			"of AES block size(16)\n");
		return -EINVAL;
	}
	if (!aes_type || aes_type >= QTI_CRYPTO_SERVICE_AES_TYPE_MAX) {
		pr_info(" aes type needed for encryption\n");
		return -EINVAL;
	}
	if (!aes_key_handle) {
		pr_info("Derive the required key handle before encrypt/decrypt\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_crypto_service_encrypt_data_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_crypto_service_encrypt_data_cmd_t_v2 *)
					dma_alloc_coherent(dev, dma_buf_size,
						&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	req_ptr->key_handle = (unsigned long)*aes_key_handle;
	pr_info("key_handle is: %lu\n", (unsigned long)req_ptr->key_handle);
	req_ptr->v1.type = aes_type;
	req_ptr->v1.mode = aes_mode;
	req_ptr->v1.iv = (req_ptr->v1.mode == 0 ? 0 : (u64)dma_aes_ivdata);
	req_ptr->v1.iv_len = AES_BLOCK_SIZE;
	req_ptr->v1.plain_data = (u64)dma_aes_unsealed_buf;
	req_ptr->v1.output_buffer = (u64)dma_aes_sealed_buf;
	req_ptr->v1.plain_data_len = aes_decrypted_len;
	req_ptr->v1.output_len = output_len;

	rc = qtiapp_test(dev, (uint8_t *)dma_req_addr, NULL, req_size,
						QTI_APP_AES_ENCRYPT_ID);
	if (rc) {
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	if (aes_mode == QTI_CRYPTO_SERVICE_AES_MODE_CBC && aes_ivdata)
		print_hex_dump(KERN_INFO, "IV data(CBC): ", DUMP_PREFIX_NONE, 16, 1,
				aes_ivdata, AES_BLOCK_SIZE, false);
	else
		pr_info("IV data(ECB): NULL\n");

	memcpy(buf, aes_sealed_buf, req_ptr->v1.output_len);
	aes_encrypted_len = req_ptr->v1.output_len;

goto end;

err_end:
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return rc;

end:
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return aes_encrypted_len;
}

static ssize_t
show_aes_encrypted_data_qtiapp(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int rc = 0;
	struct qti_crypto_service_encrypt_data_cmd_t *req_ptr = NULL;
	uint64_t req_size = 0;
	size_t dma_buf_size = 0;
	uint64_t output_len = 0;
	dma_addr_t dma_req_addr = 0;

	if (props->aes_v2) {
		rc = show_aes_v2_encrypted_data_qtiapp(dev, attr, buf);
		return rc;
	}

	aes_sealed_buf = memset(aes_sealed_buf, 0, MAX_ENCRYPTED_DATA_SIZE);
	output_len = aes_decrypted_len;

	if (aes_decrypted_len <= 0 || aes_decrypted_len % AES_BLOCK_SIZE) {
		pr_err("Invalid input %lld\n", aes_decrypted_len);
		pr_info("Input data length for encryption should be multiple"
			"of AES block size(16)\n");
		return -EINVAL;
	}
	if (!aes_type || aes_type >= QTI_CRYPTO_SERVICE_AES_TYPE_MAX) {
		pr_info(" aes type needed for encryption\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_crypto_service_encrypt_data_cmd_t);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_crypto_service_encrypt_data_cmd_t *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	req_ptr->type = aes_type;
	req_ptr->mode = aes_mode;
	req_ptr->iv = (req_ptr->mode == 0 ? 0 : (u64)dma_aes_ivdata);
	req_ptr->iv_len = AES_BLOCK_SIZE;
	req_ptr->plain_data = (u64)dma_aes_unsealed_buf;
	req_ptr->output_buffer = (u64)dma_aes_sealed_buf;
	req_ptr->plain_data_len = aes_decrypted_len;
	req_ptr->output_len = output_len;

	rc = qtiapp_test(dev, (uint8_t *)dma_req_addr, NULL, req_size,
						QTI_APP_AES_ENCRYPT_ID);
	if (rc) {
		pr_err("Response status failure..return value = %d\n", rc);
		goto err_end;
	}

	if (aes_mode == QTI_CRYPTO_SERVICE_AES_MODE_CBC && aes_ivdata)
		print_hex_dump(KERN_INFO, "IV data(CBC): ", DUMP_PREFIX_NONE, 16, 1,
				aes_ivdata, AES_BLOCK_SIZE, false);
	else
		pr_info("IV data(ECB): NULL\n");

	memcpy(buf, aes_sealed_buf, req_ptr->output_len);
	aes_encrypted_len = req_ptr->output_len;

goto end;

err_end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return rc;

end:
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return aes_encrypted_len;
}

static int32_t copy_files(int *img_size)
{
	uint8_t *buf;

	if (mdt_file && seg_file) {
		*img_size = mdt_size + seg_size;

		qsee_sbuffer = kzalloc(*img_size, GFP_KERNEL);
		if (!qsee_sbuffer) {
			pr_err("Error: qsee_sbuffer alloc failed\n");
			return -ENOMEM;
		}
		buf = qsee_sbuffer;

		memcpy(buf, mdt_file, mdt_size);
		buf += mdt_size;
		memcpy(buf, seg_file, seg_size);
		buf += seg_size;
	} else {
		pr_err("Sampleapp file Inputs not provided\n");
		return -EINVAL;
	}
	return 0;
}

static int load_request(struct device *dev, uint32_t smc_id,
		       uint32_t cmd_id, size_t req_size)
{
	union qseecom_load_ireq load_req;
	struct qseecom_command_scm_resp resp;
	int ret, ret1;
	int img_size;

	kfree(qsee_sbuffer);
	ret = copy_files(&img_size);
	if (ret) {
		pr_err("Copying Files failed\n");
		return ret;
	}

	dev = qdev;

	load_req.load_lib_req.mdt_len = mdt_size;
	load_req.load_lib_req.img_len = img_size;
	load_req.load_lib_req.phy_addr = dma_map_single(dev, qsee_sbuffer,
						       img_size, DMA_TO_DEVICE);
	ret1 = dma_mapping_error(dev, load_req.load_lib_req.phy_addr);
	if (ret1 == 0) {
		ret = qti_scm_qseecom_load(smc_id, cmd_id, &load_req,
					   req_size, &resp, sizeof(resp));
		dma_unmap_single(dev, load_req.load_lib_req.phy_addr,
				img_size, DMA_TO_DEVICE);
	}
	if (ret1) {
		pr_err("DMA Mapping error (qsee_sbuffer)\n");
		return ret1;
	}
	if (ret) {
		pr_err("SCM_CALL to load app and services failed\n");
		return ret;
	}

	if (resp.result == QSEOS_RESULT_FAILURE) {
		pr_err("SCM_CALL rsp.result is QSEOS_RESULT_FAILURE\n");
		return -EFAULT;
	}

	if (resp.result == QSEOS_RESULT_INCOMPLETE)
		pr_err("Process_incomplete_cmd ocurred\n");

	if (resp.result != QSEOS_RESULT_SUCCESS) {
		pr_err("scm_call failed resp.result unknown, %lu\n",
				resp.result);
		return -EFAULT;
	}

	pr_info("Successfully loaded app and services!!!!!\n");

	qsee_app_id = resp.data;
	return 0;
}

/* To show basic multiplication output */
static ssize_t
show_basic_output(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, (basic_data_len + 1), "%lu\n", basic_output);
}

/* Basic multiplication App*/
static ssize_t
store_basic_input(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	dma_addr_t __aligned(sizeof(dma_addr_t) * 8) basic_input = 0;
	uint32_t ret = 0;
	basic_data_len = count;
	if ((count - 1) == 0) {
		pr_err("Input cannot be NULL!\n");
		return -EINVAL;
	}
	if (kstrtouint(buf, 10, (unsigned int *)&basic_input) || basic_input > (U32_MAX / 10))
		pr_err("Please enter a valid unsigned integer less than %u\n",
			(U32_MAX / 10));
	else
		ret = qtiapp_test(dev, &basic_input, NULL, 0, QTI_APP_BASIC_DATA_TEST_ID);

	return ret ? ret : count;
}

/* To show encrypted plain text*/
static ssize_t
show_encrypt_output(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	memcpy(buf, encrypt_text, enc_len);
	return enc_len;
}

/* To Encrypt input plain text */
static ssize_t
store_encrypt_input(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int32_t ret = -EINVAL;
	uint8_t *input_pt;
	uint8_t *output_pt;

	enc_len = count;
	if (enc_len == 0) {
		pr_err("Input cannot be NULL!\n");
		return -EINVAL;
	}
	if ((enc_len % 16 != 0) || (enc_len > MAX_INPUT_SIZE)) {
		pr_info("Input Length must be multiple of 16 & < 4096 bytes\n");
		return -EINVAL;
	}

	input_pt = kzalloc(enc_len * sizeof(uint8_t *), GFP_KERNEL);
	if (!input_pt)
		return -ENOMEM;
	memcpy(input_pt, buf, count);

	output_pt = kzalloc(enc_len * sizeof(uint8_t *), GFP_KERNEL);
	if (!output_pt) {
		kfree(input_pt);
		return -ENOMEM;
	}

	ret = qtiapp_test(dev, (uint8_t *)input_pt,
			 (uint8_t *)output_pt, enc_len, QTI_APP_ENC_TEST_ID);

	if (!ret)
		memcpy(encrypt_text, output_pt, enc_len);

	kfree(input_pt);
	kfree(output_pt);
	return count;
}

/* To show decrypted cipher text */
static ssize_t
show_decrypt_output(struct device *dev, struct device_attribute *attr,
		 char *buf)
{
	memcpy(buf, decrypt_text, dec_len);
	return dec_len;
}

/* To decrypt input cipher text */
static ssize_t
store_decrypt_input(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int32_t ret = -EINVAL;
	uint8_t *input_pt;
	uint8_t *output_pt;

	dec_len = count;
	if (dec_len == 0) {
		pr_err("Input cannot be NULL!\n");
		return -EINVAL;
	}

	if ((dec_len % 16 != 0) || (dec_len > MAX_INPUT_SIZE)) {
		pr_info("Input Length must be multiple of 16 & < 4096 bytes\n");
		return -EINVAL;
	}

	input_pt = kzalloc(dec_len * sizeof(uint8_t *), GFP_KERNEL);
	if (!input_pt)
		return -ENOMEM;
	memcpy(input_pt, buf, dec_len);

	output_pt = kzalloc(dec_len * sizeof(uint8_t *), GFP_KERNEL);
	if (!output_pt) {
		kfree(input_pt);
		return -ENOMEM;
	}

	ret = qtiapp_test(dev, (uint8_t *)input_pt,
			 (uint8_t *)output_pt, dec_len, QTI_APP_DEC_TEST_ID);
	if (!ret)
		memcpy(decrypt_text, output_pt, dec_len);

	kfree(input_pt);
	kfree(output_pt);
	return count;
}

static ssize_t
store_load_start(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int load_cmd;
	uint32_t smc_id = 0;
	uint32_t cmd_id = 0;
	size_t req_size = 0;

	dev = qdev;

	if (kstrtouint(buf, 10, &load_cmd)) {
		pr_err("Provide valid integer input!\n");
		pr_err("Echo 0 to load app libs\n");
		pr_err("Echo 1 to load app\n");
		pr_err("Echo 2 to unload app\n");
		return -EINVAL;
	}
	if (load_cmd == 0) {
		if (!(props->libraries_inbuilt || app_libs_state)) {
			smc_id = QTI_SYSCALL_CREATE_SMC_ID(QTI_OWNER_QSEE_OS,
					QTI_SVC_APP_MGR, QTI_CMD_LOAD_LIB);
			cmd_id = QSEE_LOAD_SERV_IMAGE_COMMAND;
			req_size = sizeof(struct qseecom_load_lib_ireq);
			if (load_request(dev, smc_id, cmd_id, req_size))
				pr_info("Loading app libs failed\n");
			else
				app_libs_state = 1;
			if (props->logging_support_enabled) {
				if (qtidbg_register_qsee_log_buf(dev))
					pr_info("Registering log buf failed\n");
			}
		} else {
			pr_info("Libraries are either already loaded or are inbuilt in this platform\n");
		}
	} else if (load_cmd == 1) {
		if (props->libraries_inbuilt || app_libs_state) {
			if (!app_state) {
				smc_id = QTI_SYSCALL_CREATE_SMC_ID(
						QTI_OWNER_QSEE_OS, QTI_SVC_APP_MGR,
						QTI_CMD_LOAD_APP_ID);
				cmd_id = QSEOS_APP_START_COMMAND;
				req_size = sizeof(struct qseecom_load_app_ireq);
				if (load_request(dev, smc_id, cmd_id, req_size))
					pr_info("Loading app failed\n");
				else
					app_state = 1;
			} else {
				pr_info("App already loaded...\n");
			}
		} else {
			if (!app_libs_state)
				pr_info("App libs must be loaded first\n");
			if (!props->libraries_inbuilt)
				pr_info("App libs are not inbuilt in this platform\n");
		}
	} else if (load_cmd == 2) {
		if (app_state) {
			if (qseecom_unload_app())
				pr_info("App unload failed\n");
			else
				app_state = 0;
		} else {
			pr_info("App already unloaded...\n");
		}
	} else {
		pr_info("Echo 0 to load app libs if its not inbuilt\n");
		pr_info("Echo 1 to load app if its not already loaded\n");
		pr_info("Echo 2 to unload app if its already loaded\n");
	}

	return count;
}

static ssize_t
store_crypto_input(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	qtiapp_test(dev, NULL, NULL, 0, QTI_APP_CRYPTO_TEST_ID);
	return count;
}

static ssize_t
store_fuse_otp_input(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	qtiapp_test(dev, (void *)buf, NULL, 0, QTI_APP_AUTH_OTP_TEST_ID);
	return count;
}

static ssize_t
store_log_bitmask_input(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	qtiapp_test(dev, NULL, NULL, 0, QTI_APP_LOG_BITMASK_TEST_ID);
	return count;
}

static ssize_t
store_fuse_input(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	qtiapp_test(dev, NULL, NULL, 0, QTI_APP_FUSE_TEST_ID);
	return count;
}

static ssize_t
store_misc_input(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	qtiapp_test(dev, NULL, NULL, 0, QTI_APP_MISC_TEST_ID);
	return count;
}

static ssize_t show_qsee_app_id(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return snprintf(buf, sizeof(uint32_t) + 1, "%u\n", qsee_app_id);
}

static ssize_t
store_addr_fuse_write_qtiapp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned long long val;

	if (kstrtoull(buf, 0, &val))
		return -EINVAL;

	fuse_addr = val;

	return count;
}
static ssize_t
store_value_fuse_write_qtiapp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned long long val;

	if (kstrtoull(buf, 0, &val))
		return -EINVAL;

	if(val > MAX_FUSE_WRITE_VALUE) {
		pr_err("Invalid input: %llu\n", val);
		return -EINVAL;
	}
	fuse_value = val;

	return count;
}
static ssize_t
store_fec_enable_fuse_write_qtiapp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned long long val;

	if (kstrtoull(buf, 10, &val))
		return -EINVAL;

	if(val != 0 && val !=1) {
		pr_err("\nInvalid input: %llu\n", val);
		pr_err("fec enable should be either 0 or 1\n");
		return -EINVAL;
	}
	is_fec_enable = val;

	return count;
}
static ssize_t
store_blow_fuse_write_qtiapp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	uint32_t ret = 0;
	struct qti_storage_service_fuse_blow_req *req_ptr = NULL;
	uint64_t req_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_req_addr = 0;

	unsigned long long val;

	if (kstrtoull(buf, 10, &val))
		return -EINVAL;

	if(val !=1) {
		pr_err("\nInvalid input: %llu\n", val);
		pr_err("echo 1 to blow the fuse\n");
		return -EINVAL;
	}

	dev = qdev;

	req_size = sizeof(struct qti_storage_service_fuse_blow_req);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_fuse_blow_req *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_req_addr, GFP_KERNEL);
	if (!req_ptr)
		return -ENOMEM;

	req_ptr->addr = fuse_addr;
	req_ptr->value = fuse_value;
	req_ptr->is_fec_enable = is_fec_enable;

	ret = qtiapp_test(dev, (void *)dma_req_addr, NULL, req_size,
					QTI_APP_FUSE_BLOW_ID);
	if (ret) {
		pr_err("Fuse Blow failed from QTI app with error code %d\n",ret);
	}
	else
		ret = count;

	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req_addr);

	return ret;

}

static ssize_t
store_decrypted_rsa_data_qtiapp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	if (!count || (count > MAX_RSA_PLAIN_DATA_SIZE)) {
		pr_err("Invalid length\n");
		pr_err("Given length: %lu, Max Plain msg length: %u\n",
			(unsigned long)count, (unsigned int)MAX_RSA_PLAIN_DATA_SIZE);
		return -EINVAL;
	}

	memset(rsa_unsealed_buf, 0, MAX_RSA_PLAIN_DATA_SIZE);
	rsa_decrypted_len = count;
	memcpy(rsa_unsealed_buf, buf, rsa_decrypted_len);
	return count;
}
static ssize_t
store_encrypted_rsa_data_qtiapp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	if (!count || (count > MAX_RSA_SIGN_DATA_SIZE)) {
		pr_err("Invalid length\n");
		pr_err("Given length: %lu, Max Encrypted Msg length: %u\n",
			(unsigned long)count, (unsigned int)MAX_RSA_SIGN_DATA_SIZE);
		return -EINVAL;
	}

	memset(rsa_sealed_buf, 0, MAX_RSA_SIGN_DATA_SIZE);
	rsa_encrypted_len = count;
	memcpy(rsa_sealed_buf, buf, rsa_encrypted_len);
	return count;
}
static ssize_t
store_label_rsa_data_qtiapp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	if (!count || (count > MAX_RSA_PLAIN_DATA_SIZE)) {
		pr_err("Invalid length\n");
		pr_err("Given length: %lu, Max Lebel length: %u\n",
			(unsigned long)count, (unsigned int)MAX_RSA_PLAIN_DATA_SIZE);
		return -EINVAL;
	}

	memset(rsa_label, 0, MAX_RSA_PLAIN_DATA_SIZE);
	rsa_label_len = count;
	memcpy(rsa_label, buf, rsa_label_len);
	return count;
}
static ssize_t
store_n_key_rsa_data_qtiapp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	if (!count || (count > RSA_KEY_SIZE_MAX)) {
		pr_err("Invalid length\n");
		pr_err("Given length: %lu, Max Key Modulus length: %u\n",
			(unsigned long)count, (unsigned int)RSA_KEY_SIZE_MAX);
		return -EINVAL;
	}

	memset(rsa_n_key, 0, RSA_KEY_SIZE_MAX);
	rsa_n_key_len = count;
	memcpy(rsa_n_key, buf, rsa_n_key_len);
	if (rsa_n_key[count-1] == 0x0a)
		rsa_n_key[count-1] = 0x00;
	return count;
}
static ssize_t
store_e_key_rsa_data_qtiapp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	if (!count || (count > RSA_KEY_SIZE_MAX)) {
		pr_err("Invalid length\n");
		pr_err("Given length: %lu, Max Public exponent length: %u\n",
			(unsigned long)count, (unsigned int)RSA_KEY_SIZE_MAX);
		return -EINVAL;
	}

	memset(rsa_e_key, 0, RSA_KEY_SIZE_MAX);
	rsa_e_key_len = count;
	memcpy(rsa_e_key, buf, rsa_e_key_len);
	if (rsa_e_key[count-1] == 0x0a)
		rsa_e_key[count-1] = 0x00;
	return count;
}
static ssize_t
store_d_key_rsa_data_qtiapp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	if (!count || (count > RSA_KEY_SIZE_MAX)) {
		pr_err("Invalid length\n");
		pr_err("Given length: %lu, Max Private exponent length: %u\n",
			(unsigned long)count, (unsigned int)RSA_KEY_SIZE_MAX);
		return -EINVAL;
	}

	memset(rsa_d_key, 0, RSA_KEY_SIZE_MAX);
	rsa_d_key_len = count;
	memcpy(rsa_d_key, buf, rsa_d_key_len);
	if (rsa_d_key[count-1] == 0x0a)
		rsa_d_key[count-1] = 0x00;
	return count;
}
static ssize_t
store_nbits_key_rsa_data_qtiapp(struct device *dev, struct device_attribute *attr,
               const char *buf, size_t count)
{
	unsigned long long val;

	if (kstrtoull(buf, 10, &val))
		return -EINVAL;

	if(val <=0 || val % RSA_KEY_ALIGN) {
		pr_err("Invalid key nbits: %llu\n", val);
		pr_err("Number of bits in key should be RSA_KEY_ALIGN %d aligned\n",RSA_KEY_ALIGN);
		return -EINVAL;
	}
	rsa_nbits_key = val;

	return count;
}
static ssize_t
store_hashidx_rsa_data_qtiapp(struct device *dev, struct device_attribute *attr,
               const char *buf, size_t count)
{
	unsigned long long val;

	if (kstrtoull(buf, 10, &val))
		return -EINVAL;

	if (val <=0 || val >= QSEE_HASH_IDX_MAX) {
		pr_err("Invalid rsa hashidx: %llu\n", val);
		return -EINVAL;
	}
	rsa_hashidx = val;

	return count;
}

static ssize_t
store_padding_type_rsa_data_qtiapp(struct device *dev, struct device_attribute *attr,
               const char *buf, size_t count)
{
	unsigned long long val;

	if (kstrtoull(buf, 10, &val))
		return -EINVAL;

	if (val < 0 || val >= QSEE_RSA_PADDING_TYPE_MAX) {
		pr_err("Invalid padding type: %llu\n", val);
		return -EINVAL;
	}
	rsa_padding_type = val;

	return count;
}

static ssize_t
show_encrypted_rsa_data_qtiapp(struct device *dev, struct device_attribute *attr,
						 char *buf)
{
	uint32_t ret = 0;
	struct qti_storage_service_rsa_message_type *msg_vector_ptr = NULL;
	struct qti_storage_service_rsa_key_type *key = NULL;
	struct qti_storage_service_rsa_message_req *req_ptr = NULL;
	uint64_t msg_vector_size = 0, key_size = 0, req_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_encrypted_msg_len = 0;
	dma_addr_t dma_msg_vector = 0;
	dma_addr_t dma_key = 0;
	dma_addr_t dma_req = 0;
	uint64_t *output_len = NULL;

	dev = qdev;

	if (rsa_decrypted_len <= 0) {
		pr_err("Invalid msg size %lld\n", rsa_decrypted_len);
		return ret;
	} else if (rsa_d_key_len <=0) {
		pr_err("Invalid private exponent, size %lld\n", rsa_d_key_len);
		return ret;
	} else if (rsa_e_key_len <=0) {
		pr_err("Invalid public exponent, size %lld\n", rsa_e_key_len);
		return ret;
	} else if (rsa_n_key_len <=0) {
		pr_err("Invalid modulus key, size %lld\n", rsa_n_key_len);
		return ret;
	} else if (rsa_hashidx <=0) {
		pr_err("Invalid rsa hashidx, size %lld\n", rsa_hashidx);
		return ret;
	} else if (rsa_nbits_key <=0) {
		pr_err("Invalid key nbits, size %lld\n", rsa_nbits_key);
		return ret;
	}

	memset(rsa_sealed_buf, 0, MAX_RSA_SIGN_DATA_SIZE);
	rsa_encrypted_len = 0;

	output_len = (uint64_t*)dma_alloc_coherent(dev, sizeof(uint64_t),
				&dma_encrypted_msg_len, GFP_KERNEL);
	if (!output_len) {
		pr_err("Error: alloc failed\n");
		return -ENOMEM;
	}

	msg_vector_size = sizeof(struct qti_storage_service_rsa_message_type);
	dma_buf_size = PAGE_SIZE * (1 << get_order(msg_vector_size));
	msg_vector_ptr = (struct qti_storage_service_rsa_message_type *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_msg_vector, GFP_KERNEL|GFP_DMA);
	if (!msg_vector_ptr) {
		pr_err("Mem allocation failed for msg vector\n");
		goto err_mem_msg_vector;
	}

	msg_vector_ptr->input = (u64)dma_rsa_unsealed_buf;
	msg_vector_ptr->input_len = rsa_decrypted_len;
	msg_vector_ptr->label = (u64)dma_rsa_label;
	msg_vector_ptr->label_len = rsa_label_len;
	msg_vector_ptr->output = (u64)dma_rsa_sealed_buf;
	msg_vector_ptr->output_len = (u64)dma_encrypted_msg_len;
	msg_vector_ptr->padding_type = rsa_padding_type;
	msg_vector_ptr->hashidx = rsa_hashidx;

	key_size = sizeof(struct qti_storage_service_rsa_key_type);
	dma_buf_size = PAGE_SIZE * (1 << get_order(key_size));
	key = (struct qti_storage_service_rsa_key_type *)
		dma_alloc_coherent(dev, dma_buf_size, &dma_key,
					GFP_KERNEL|GFP_DMA);
	if(!key) {
		pr_err("Mem allocation failed for key vector\n");
		goto err_mem_key;
	}

	key->n = (u64)dma_rsa_n_key;
	key->e = (u64)dma_rsa_e_key;
	key->d = (u64)dma_rsa_d_key;
	key->nbits = rsa_nbits_key;

	req_size = sizeof(struct qti_storage_service_rsa_message_req);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_rsa_message_req *)
			dma_alloc_coherent(dev, dma_buf_size, &dma_req,
						GFP_KERNEL|GFP_DMA);
	if (!req_ptr) {
		pr_err("Mem allocation failed for request buffer\n");
		goto err_mem_req;
	}

	req_ptr->msg_req = (u64)dma_msg_vector;
	req_ptr->key_req = (u64)dma_key;
	req_ptr->operation = QTI_APP_RSA_ENCRYPTION_ID;

	ret = qtiapp_test(dev, (void *)dma_req, NULL, req_size,
						QTI_APP_RSA_ENC_DEC_ID);

	if (ret) {
		pr_err("RSA encryption failed from QTI app with error code %d\n",ret);
		rsa_encrypted_len = 0;
	} else {
		rsa_encrypted_len = *output_len;
		memcpy(buf, rsa_sealed_buf, rsa_encrypted_len);
	}

	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req);

err_mem_req:
	dma_buf_size = PAGE_SIZE * (1 << get_order(key_size));
	dma_free_coherent(dev, dma_buf_size, key, dma_key);
err_mem_key:
	dma_buf_size = PAGE_SIZE * (1 << get_order(msg_vector_size));
	dma_free_coherent(dev, dma_buf_size, msg_vector_ptr, dma_msg_vector);
err_mem_msg_vector:
	dma_free_coherent(dev, sizeof(uint64_t*), output_len, dma_encrypted_msg_len);

	return rsa_encrypted_len;
}

static ssize_t
show_decrypted_rsa_data_qtiapp(struct device *dev, struct device_attribute *attr,
						 char *buf)
{
	uint32_t ret = 0;
	struct qti_storage_service_rsa_message_type *msg_vector_ptr = NULL;
	struct qti_storage_service_rsa_key_type *key = NULL;
	struct qti_storage_service_rsa_message_req *req_ptr = NULL;
	uint64_t msg_vector_size = 0, key_size = 0, req_size = 0;
	size_t dma_buf_size = 0;
	dma_addr_t dma_decrypted_msg_len = 0;
	dma_addr_t dma_msg_vector = 0;
	dma_addr_t dma_key = 0;
	dma_addr_t dma_req = 0;
	uint64_t *output_len = NULL;

	dev = qdev;
	if (rsa_encrypted_len <= 0) {
		pr_err("Invalid encrypted msg size %lld\n", rsa_encrypted_len);
		return ret;
	} else if (rsa_d_key_len <=0) {
		pr_err("Invalid private exponent, size %lld\n", rsa_d_key_len);
		return ret;
	} else if (rsa_e_key_len <=0) {
		pr_err("Invalid public exponent, size %lld\n", rsa_e_key_len);
		return ret;
	} else if (rsa_n_key_len <=0) {
		pr_err("Invalid modulus key, size %lld\n", rsa_n_key_len);
		return ret;
	} else if (rsa_hashidx <=0) {
		pr_err("Invalid rsa hashidx, size %lld\n", rsa_hashidx);
		return ret;
	} else if (rsa_nbits_key <=0) {
		pr_err("Invalid key nbits, size %lld\n", rsa_nbits_key);
		return ret;
	}

	output_len = (uint64_t*)dma_alloc_coherent(dev, sizeof(uint64_t),
				&dma_decrypted_msg_len, GFP_KERNEL);
	if (!output_len) {
		pr_err("Error: alloc failed\n");
		return -ENOMEM;
	}

	memset(rsa_unsealed_buf, 0, MAX_RSA_PLAIN_DATA_SIZE);
	rsa_decrypted_len = 0;

	msg_vector_size = sizeof(struct qti_storage_service_rsa_message_type);
	dma_buf_size = PAGE_SIZE * (1 << get_order(msg_vector_size));
	msg_vector_ptr = (struct qti_storage_service_rsa_message_type *)
					dma_alloc_coherent(dev, dma_buf_size,
					&dma_msg_vector, GFP_KERNEL);
	if (!msg_vector_ptr) {
		pr_err("Mem allocation failed for msg vector\n");
		goto err_mem_msg_vector;
	}

	msg_vector_ptr->input = (u64)dma_rsa_sealed_buf;
	msg_vector_ptr->input_len = rsa_encrypted_len;
	msg_vector_ptr->label = (u64)dma_rsa_label;
	msg_vector_ptr->label_len = rsa_label_len;
	msg_vector_ptr->output = (u64)dma_rsa_unsealed_buf;
	msg_vector_ptr->output_len = (u64)dma_decrypted_msg_len;
	msg_vector_ptr->padding_type = rsa_padding_type;
	msg_vector_ptr->hashidx = rsa_hashidx;

	key_size = sizeof(struct qti_storage_service_rsa_key_type);
	dma_buf_size = PAGE_SIZE * (1 << get_order(key_size));
	key = (struct qti_storage_service_rsa_key_type *)
		dma_alloc_coherent(dev, dma_buf_size, &dma_key,
					GFP_KERNEL);
	if(!key) {
		pr_err("Mem allocation failed for key vector\n");
		goto err_mem_key;
	}

	key->n = (u64)dma_rsa_n_key;
	key->e = (u64)dma_rsa_e_key;
	key->d = (u64)dma_rsa_d_key;
	key->nbits = rsa_nbits_key;

	req_size = sizeof(struct qti_storage_service_rsa_message_req);
	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	req_ptr = (struct qti_storage_service_rsa_message_req *)
			dma_alloc_coherent(dev, dma_buf_size, &dma_req,
						GFP_KERNEL);
	if (!req_ptr) {
		pr_err("Mem allocation failed for request buffer\n");
		goto err_mem_req;
	}

	req_ptr->msg_req = (u64)dma_msg_vector;
	req_ptr->key_req = (u64)dma_key;
	req_ptr->operation = QTI_APP_RSA_DECRYPTION_ID;

	ret = qtiapp_test(dev, (void *)dma_req, NULL, req_size,
						QTI_APP_RSA_ENC_DEC_ID);

	if (ret) {
		pr_err("RSA decryption failed with %d from QTI app\n",ret);
		rsa_decrypted_len = 0;
	} else {
		rsa_decrypted_len = *output_len;
		memcpy(buf, rsa_unsealed_buf, rsa_decrypted_len);
	}

	dma_buf_size = PAGE_SIZE * (1 << get_order(req_size));
	dma_free_coherent(dev, dma_buf_size, req_ptr, dma_req);

err_mem_req:
	dma_buf_size = PAGE_SIZE * (1 << get_order(key_size));
	dma_free_coherent(dev, dma_buf_size, key, dma_key);
err_mem_key:
	dma_buf_size = PAGE_SIZE * (1 << get_order(msg_vector_size));
	dma_free_coherent(dev, dma_buf_size, msg_vector_ptr, dma_msg_vector);
err_mem_msg_vector:
	dma_free_coherent(dev, sizeof(uint64_t*), output_len,
							dma_decrypted_msg_len);

	return rsa_decrypted_len;
}

static int __init qtiapp_init(struct device *dev)
{
	int err;
	int i = 0;
	size_t dma_buf_size = 0;
	struct attribute **qtiapp_attrs = kzalloc((hweight_long(props->function)
				+ 1) * sizeof(*qtiapp_attrs), GFP_KERNEL);

	if (!qtiapp_attrs) {
		pr_err("Cannot allocate memory..qtiapp\n");
		return -ENOMEM;
	}

	qtiapp_attrs[i++] = &dev_attr_load_start.attr;
	qtiapp_attrs[i++] = &dev_attr_qsee_app_id.attr;

	if (props->function & MUL)
		qtiapp_attrs[i++] = &dev_attr_basic_data.attr;

	if (props->function & ENC)
		qtiapp_attrs[i++] = &dev_attr_encrypt.attr;

	if (props->function & DEC)
		qtiapp_attrs[i++] = &dev_attr_decrypt.attr;

	if (props->function & CRYPTO)
		qtiapp_attrs[i++] = &dev_attr_crypto.attr;

	if (props->function & AUTH_OTP)
		qtiapp_attrs[i++] = &dev_attr_fuse_otp.attr;

	if (props->function & LOG_BITMASK)
		qtiapp_attrs[i++] = &dev_attr_log_bitmask.attr;

	if (props->function & FUSE)
		qtiapp_attrs[i++] = &dev_attr_fuse.attr;

	if (props->function & MISC)
		qtiapp_attrs[i++] = &dev_attr_misc.attr;

	if (props->logging_support_enabled)
		qtiapp_attrs[i++] = &dev_attr_log_buf.attr;

	qtiapp_attrs[i] = NULL;

	qtiapp_attr_grp.attrs = qtiapp_attrs;

	qtiapp_kobj = kobject_create_and_add("tzapp", firmware_kobj);

	err = sysfs_create_group(qtiapp_kobj, &qtiapp_attr_grp);

	if (err) {
		kobject_put(qtiapp_kobj);
		return err;
	}

	if (props->function & AES_TZAPP) {

		dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_ENCRYPTED_DATA_SIZE));
		buf_aes_sealed_buf = dma_alloc_coherent(dev, dma_buf_size,
					&dma_aes_sealed_buf, GFP_KERNEL);

		dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_PLAIN_DATA_SIZE));
		buf_aes_unsealed_buf = dma_alloc_coherent(dev, dma_buf_size,
					&dma_aes_unsealed_buf, GFP_KERNEL);

		dma_buf_size = PAGE_SIZE *
				(1 << get_order(AES_BLOCK_SIZE));
		buf_aes_iv = dma_alloc_coherent(dev, dma_buf_size,
					&dma_aes_ivdata, GFP_KERNEL);

		if (!buf_aes_sealed_buf || !buf_aes_unsealed_buf || !buf_aes_iv) {
			pr_err("Cannot allocate memory for aes crypt ops\n");

			if (buf_aes_sealed_buf) {
				dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_ENCRYPTED_DATA_SIZE));
				dma_free_coherent(dev, dma_buf_size,
					buf_aes_sealed_buf,
					dma_aes_sealed_buf);
			}

			if (buf_aes_unsealed_buf) {
				dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_PLAIN_DATA_SIZE));
				dma_free_coherent(dev, dma_buf_size,
					buf_aes_unsealed_buf,
					dma_aes_unsealed_buf);
			}

			if (buf_aes_iv) {
				dma_buf_size = PAGE_SIZE *
				(1 << get_order(AES_BLOCK_SIZE));
				dma_free_coherent(dev, dma_buf_size,
					buf_aes_iv,
					dma_aes_ivdata);
			}

		} else {

			aes_sealed_buf = (uint8_t*)buf_aes_sealed_buf;
			aes_unsealed_buf = (uint8_t*)buf_aes_unsealed_buf;
			aes_ivdata = (uint8_t*)buf_aes_iv;

			qtiapp_aes_kobj = kobject_create_and_add("aes", qtiapp_kobj);

			err = sysfs_create_group(qtiapp_aes_kobj, &qtiapp_aes_attr_grp);

			if (err) {
				kobject_put(qtiapp_aes_kobj);
			}

			if (props->aes_v2) {
				err = sysfs_create_group(qtiapp_aes_kobj, &qtiapp_aesv2_attr_grp);
				if (err)
					pr_debug("TZapp AES v2 sysfs creation failed with error %d\n",err);
			}
		}

	}

	if (props->function & RSA_TZAPP) {

		dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_RSA_PLAIN_DATA_SIZE));
		buf_rsa_unsealed_buf = dma_alloc_coherent(dev, dma_buf_size,
					&dma_rsa_unsealed_buf, GFP_KERNEL);
		buf_rsa_label = dma_alloc_coherent(dev, dma_buf_size,
					&dma_rsa_label, GFP_KERNEL);

		dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_RSA_SIGN_DATA_SIZE));
		buf_rsa_sealed_buf = dma_alloc_coherent(dev, dma_buf_size,
					&dma_rsa_sealed_buf, GFP_KERNEL);

		dma_buf_size = PAGE_SIZE *
				(1 << get_order(RSA_KEY_SIZE_MAX));
		buf_rsa_n_key = dma_alloc_coherent(dev, dma_buf_size,
					&dma_rsa_n_key, GFP_KERNEL);
		buf_rsa_e_key = dma_alloc_coherent(dev, dma_buf_size,
					&dma_rsa_e_key, GFP_KERNEL);
		buf_rsa_d_key = dma_alloc_coherent(dev, dma_buf_size,
					&dma_rsa_d_key, GFP_KERNEL);

		if (!buf_rsa_unsealed_buf || !buf_rsa_sealed_buf
			|| !buf_rsa_label || !buf_rsa_n_key
			|| !buf_rsa_e_key || !buf_rsa_d_key) {

			pr_err("Cannot allocate memory for rsa crypt ops\n");

			if (buf_rsa_unsealed_buf) {
				dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_RSA_PLAIN_DATA_SIZE));
				dma_free_coherent(dev, dma_buf_size,
							buf_rsa_unsealed_buf,
							dma_rsa_unsealed_buf);
			}

			if (buf_rsa_sealed_buf) {
				dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_RSA_SIGN_DATA_SIZE));
				dma_free_coherent(dev, dma_buf_size,
							buf_rsa_sealed_buf,
							dma_rsa_sealed_buf);
			}

			if (buf_rsa_label) {
				dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_RSA_PLAIN_DATA_SIZE));
				dma_free_coherent(dev, dma_buf_size,
							buf_rsa_label,
							dma_rsa_label);
			}

			if (buf_rsa_n_key) {
				dma_buf_size = PAGE_SIZE *
					(1 << get_order(RSA_KEY_SIZE_MAX));
				dma_free_coherent(dev, dma_buf_size,
							buf_rsa_n_key,
							dma_rsa_n_key);
			}

			if (buf_rsa_e_key) {
				dma_buf_size = PAGE_SIZE *
					(1 << get_order(RSA_KEY_SIZE_MAX));
				dma_free_coherent(dev, dma_buf_size,
							buf_rsa_e_key,
							dma_rsa_e_key);
			}

			if (buf_rsa_d_key) {
				dma_buf_size = PAGE_SIZE *
					(1 << get_order(RSA_KEY_SIZE_MAX));
				dma_free_coherent(dev, dma_buf_size,
							buf_rsa_d_key,
							dma_rsa_d_key);
			}

		} else {

			rsa_unsealed_buf = (uint8_t*) buf_rsa_unsealed_buf;
			rsa_sealed_buf = (uint8_t*) buf_rsa_sealed_buf;
			rsa_label = (uint8_t*) buf_rsa_label;
			rsa_n_key = (uint8_t*) buf_rsa_n_key;
			rsa_e_key = (uint8_t*) buf_rsa_e_key;
			rsa_d_key = (uint8_t*) buf_rsa_d_key;

			qtiapp_rsa_kobj = kobject_create_and_add("rsa", qtiapp_kobj);

			err = sysfs_create_group(qtiapp_rsa_kobj, &qtiapp_rsa_attr_grp);

			if (err) {
				kobject_put(qtiapp_rsa_kobj);
			}
		}
	}

	if(props->function & FUSE_WRITE) {

		qtiapp_fuse_write_kobj = kobject_create_and_add("fuse_write", qtiapp_kobj);

		err = sysfs_create_group(qtiapp_fuse_write_kobj, &qtiapp_fuse_write_attr_grp);

		if (err) {
			kobject_put(qtiapp_fuse_write_kobj);
		}
	}

	return 0;
}

static int __init qseecom_probe(struct platform_device *pdev)
{
	struct device_node *of_node = pdev->dev.of_node;
	struct device_node *node;
	struct resource qseecom_res;
	phys_addr_t node_addr;
	size_t node_size;
	const struct of_device_id *id;
	struct reserved_mem *rmem = NULL;
	struct qsee_notify_app notify_app;
	struct qseecom_command_scm_resp resp;
	int ret = 0;

	if (!of_node)
		return -ENODEV;

	qdev = &pdev->dev;
	id = of_match_device(qseecom_of_table, &pdev->dev);
	if (!id)
		return -ENODEV;

	node = of_parse_phandle(of_node, "memory-region", 0);
	if (node)
		rmem = of_reserved_mem_lookup(node);

	of_node_put(node);

	if (!rmem) {
		/* Note returning error here because other functionalities
		 * outside tzapp in qseecom can still be used */
		pr_err("QSEECom: Unable to acquire memory-region\n");
		pr_err("QSEECom: TZApp cannot be used\n");
		goto load;
	}

	node = of_parse_phandle(of_node, "memory-region", 1);
	if (node && !of_address_to_resource(node, 0, &qseecom_res)) {
		node_addr = qseecom_res.start;
		node_size = resource_size(&qseecom_res);

		ret = dma_declare_coherent_memory(qdev, node_addr,
				node_addr, node_size);
		if(ret) {
			pr_err("QSEECom: Unable to acquire tzapp_data memory-region\n");
			pr_err("QSEECom: TZApp cannot be used\n");
			goto load;
		}
	}

	notify_app.applications_region_addr = rmem->base;
	notify_app.applications_region_size = rmem->size;

	/* 4KB adjustment is to ensure QTIApp does not overlap
	 * the region alloted for SMMU WAR
	 */
	if (of_property_read_bool(of_node, "notify-align")) {
		notify_app.applications_region_addr += PAGE_SIZE;
		notify_app.applications_region_size -= PAGE_SIZE;
	}

	ret = qti_scm_qseecom_notify(&notify_app,
				     sizeof(struct qsee_notify_app),
				     &resp, sizeof(resp));
	if (ret) {
		pr_err("Notify App failed\n");
		return -1;
	}
	pr_info("QSEECom: Notify App Region Successful\n");
	pr_info("QSEECom: TZApp using Memory Region of size 0x%llx from:0x%llx to 0x%llx\n",
		(long long unsigned int) notify_app.applications_region_size,
		(long long unsigned int) notify_app.applications_region_addr,
		(long long unsigned int) notify_app.applications_region_addr +
		(long long unsigned int) notify_app.applications_region_size);

load:
	props = ((struct qseecom_props *)id->data);

	sysfs_create_bin_file(firmware_kobj, &mdt_attr);
	sysfs_create_bin_file(firmware_kobj, &seg_attr);

	if (props->function & AUTH_OTP)
		sysfs_create_bin_file(firmware_kobj, &auth_attr);
	if (!qtiapp_init(qdev))
		pr_info("Loaded tzapp successfully!\n");
	else
		pr_info("Failed to load tzapp module\n");

	if (props->function & AES_SEC_KEY) {
		if (!sec_key_init(qdev))
			pr_info("Init. AES sec key feature successful!\n");
		else
			pr_info("Failed to load AES Sec Key feature\n");
	}

	if (props->function & RSA_SEC_KEY) {
		if (!rsa_sec_key_init(qdev))
			pr_info("Init. RSA sec key feature successful!\n");
		else
			pr_info("Failed to load RSA Sec Key feature\n");
	}

	return 0;
}

static int __exit qseecom_remove(struct platform_device *pdev)
{
	int ret = -1;
	size_t dma_buf_size = 0;
	struct device *dev = &pdev->dev;

	if (app_state) {
		if (qseecom_unload_app())
			pr_err("App unload failed\n");
		else
			app_state = 0;
	}

	sysfs_remove_bin_file(firmware_kobj, &mdt_attr);
	sysfs_remove_bin_file(firmware_kobj, &seg_attr);

	if (props->function & AUTH_OTP)
		sysfs_remove_bin_file(firmware_kobj, &auth_attr);
	sysfs_remove_group(qtiapp_kobj, &qtiapp_attr_grp);
	kobject_put(qtiapp_kobj);

	if (props->function & AES_SEC_KEY) {

		if (buf_key) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(KEY_SIZE));
			dma_free_coherent(dev, dma_buf_size, buf_key,
							dma_key);
		}

		if (buf_key_blob) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(KEY_BLOB_SIZE));
			dma_free_coherent(dev, dma_buf_size, buf_key_blob,
							dma_key_blob);
		}

		if (buf_sealed_buf) {
			dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_ENCRYPTED_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
							buf_sealed_buf,
							dma_sealed_data);
		}

		if (buf_unsealed_buf) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(MAX_PLAIN_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
							buf_unsealed_buf,
							dma_unsealed_data);
		}

		if (buf_iv) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(AES_BLOCK_SIZE));
			dma_free_coherent(dev, dma_buf_size,
							buf_iv,
							dma_iv_data);
		}

		sysfs_remove_group(sec_kobj, &sec_key_attr_grp);
		kobject_put(sec_kobj);
	}

	if (props->function & RSA_SEC_KEY) {

		if (buf_rsa_key_blob) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(RSA_KEY_BLOB_SIZE));
			dma_free_coherent(dev, dma_buf_size, buf_rsa_key_blob,
							dma_rsa_key_blob);
		}

		if (buf_rsa_import_modulus) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(RSA_KEY_SIZE_MAX));
			dma_free_coherent(dev, dma_buf_size,
					buf_rsa_import_modulus,
					dma_rsa_import_modulus);
		}

		if (buf_rsa_import_public_exponent) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(RSA_PUB_EXP_SIZE_MAX));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_import_public_exponent,
						dma_rsa_import_public_exponent);
		}

		if (buf_rsa_import_pvt_exponent) {
			dma_buf_size = PAGE_SIZE *
					(1 << get_order(RSA_KEY_SIZE_MAX));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_import_pvt_exponent,
						dma_rsa_import_pvt_exponent);
		}

		if (buf_rsa_sign_data_buf) {
			dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_RSA_SIGN_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_sign_data_buf,
						dma_rsa_sign_data_buf);
		}

		if(buf_rsa_plain_data_buf) {
			dma_buf_size = PAGE_SIZE *
				(1 << get_order(MAX_RSA_PLAIN_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_plain_data_buf,
						dma_rsa_plain_data_buf);
		}

		sysfs_remove_group(rsa_sec_kobj, &rsa_sec_key_attr_grp);
		kobject_put(rsa_sec_kobj);
	}

	if (props->function & AES_TZAPP) {

		if (buf_aes_sealed_buf) {
			dma_buf_size = PAGE_SIZE *
			(1 << get_order(MAX_ENCRYPTED_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
				buf_aes_sealed_buf,
				dma_aes_sealed_buf);
		}

		if (buf_aes_unsealed_buf) {
			dma_buf_size = PAGE_SIZE *
			(1 << get_order(MAX_PLAIN_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
				buf_aes_unsealed_buf,
				dma_aes_unsealed_buf);
		}

		if (buf_aes_iv) {
			dma_buf_size = PAGE_SIZE *
			(1 << get_order(AES_BLOCK_SIZE));
			dma_free_coherent(dev, dma_buf_size,
				buf_aes_iv,
				dma_aes_ivdata);
		}

		sysfs_remove_group(qtiapp_aes_kobj, &qtiapp_aes_attr_grp);
		if (props->aes_v2)
			sysfs_remove_group(qtiapp_aes_kobj, &qtiapp_aesv2_attr_grp);
		kobject_put(qtiapp_aes_kobj);

	}

	if (props->function & RSA_TZAPP) {
		if (buf_rsa_unsealed_buf) {
			dma_buf_size = PAGE_SIZE *
			(1 << get_order(MAX_RSA_PLAIN_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_unsealed_buf,
						dma_rsa_unsealed_buf);
		}

		if (buf_rsa_sealed_buf) {
			dma_buf_size = PAGE_SIZE *
			(1 << get_order(MAX_RSA_SIGN_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_sealed_buf,
						dma_rsa_sealed_buf);
		}

		if (buf_rsa_label) {
			dma_buf_size = PAGE_SIZE *
			(1 << get_order(MAX_RSA_PLAIN_DATA_SIZE));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_label,
						dma_rsa_label);
		}

		if (buf_rsa_n_key) {
			dma_buf_size = PAGE_SIZE *
				(1 << get_order(RSA_KEY_SIZE_MAX));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_n_key,
						dma_rsa_n_key);
		}

		if (buf_rsa_e_key) {
			dma_buf_size = PAGE_SIZE *
				(1 << get_order(RSA_KEY_SIZE_MAX));
				dma_free_coherent(dev, dma_buf_size,
						buf_rsa_e_key,
						dma_rsa_e_key);
		}

		if (buf_rsa_d_key) {
			dma_buf_size = PAGE_SIZE *
				(1 << get_order(RSA_KEY_SIZE_MAX));
			dma_free_coherent(dev, dma_buf_size,
						buf_rsa_d_key,
						dma_rsa_d_key);
		}

		sysfs_remove_group(qtiapp_rsa_kobj, &qtiapp_rsa_attr_grp);
		kobject_put(qtiapp_rsa_kobj);
	}


	if (props->function & FUSE_WRITE) {

		sysfs_remove_group(qtiapp_fuse_write_kobj,
						&qtiapp_fuse_write_attr_grp);
		kobject_put(qtiapp_fuse_write_kobj);

	}

	kfree(mdt_file);
	kfree(seg_file);

	if (props->function & AUTH_OTP)
		kfree(auth_file);

	kfree(qsee_sbuffer);

	if (app_libs_state) {
		if (unload_app_libs())
			pr_err("App libs unload failed\n");
		else
			app_libs_state = 0;
	}

	ret = qti_scm_qseecom_remove_xpu();
	if (ret && (ret != -ENOTSUPP))
		pr_err("scm call failed with error %d\n", ret);

	return 0;
}

static struct platform_driver qseecom_driver = {
	.remove = qseecom_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = qseecom_of_table,
	},
};
module_platform_driver_probe(qseecom_driver, qseecom_probe);

MODULE_DESCRIPTION("QSEECOM Driver");
MODULE_LICENSE("GPL v2");
