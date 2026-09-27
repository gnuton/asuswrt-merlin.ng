/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/string.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/dma-direction.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/qcom_scm.h>
#include "soc/qcom/license_manager.h"

#define LICENSE_BUF_MAX 	(512 * 1024) //512KB
#define LICENSE_INFO_CONF_PATH 	"./license/license_info.conf"
#define ECDSA_MAGIC 		"SSED"
#define LICENSE_MAGIC 		"SSLD"
#define LICENSE_META_DATA_MAGIC "SSLM"
#define MAGIC_SIZE 		4
#define TLV_LENGTH_SIZE 	4
#define HEADER_SIZE 		MAGIC_SIZE + TLV_LENGTH_SIZE
#define NONCE_SIZE 		34
#define ECDSA_DATA_SIZE 	2048 //2KB
#define ECDSA_BUF_MAX 		2048 + 8 + 12 //2KB + ECDSA header + License meta data
#define LICENSE_META_DATA_SIZE 	HEADER_SIZE + 8 // Header + Address + Length
#define QWES_SVC_ID 		0x1E
#define QWES_ECDSA_REQUEST 	0x4
#define LICENSE_INFO_START 	"[licensefile]"
#define FILE_COUNT_STRING 	"filecount"
#define FILE_STRING 		"file"
#define MAX_LICENSE_INFO_SIZE	(sizeof(LICENSE_INFO_START) + \
				 sizeof(FILE_COUNT_STRING) + sizeof(FILE_STRING) + \
				 ((sizeof(FILE_STRING) + FILE_NAME_MAX + 5) * QMI_LM_MAX_LICENSE_FILES_V01))
struct qmi_handle *lm_clnt_hdl;

static struct lm_svc_ctx *lm_svc;

static struct kobject *lm_kobj;

dev_t chr_dev;
static struct class *dev_class;
static struct cdev lm_cdev;

static unsigned int use_license_partition = 0;

module_param(use_license_partition, uint, 0644);
MODULE_PARM_DESC(use_license_partition, "Use license files from rootfs: 0,1");

static DEFINE_MUTEX(license_valid_lock);
static atomic_t buf_use_count = ATOMIC_INIT(0);

struct qmi_elem_info qmi_lm_feature_list_req_msg_v01_ei[] = {
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u32),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x01,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   reserved),
	},
	{
		.data_type      = QMI_OPT_FLAG,
		.elem_len       = 1,
		.elem_size      = sizeof(u8),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x10,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   feature_list_valid),
	},
	{
		.data_type      = QMI_DATA_LEN,
		.elem_len       = 1,
		.elem_size      = sizeof(u8),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x10,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   feature_list_len),
	},
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = QMI_LM_MAX_FEATURE_LIST_V01,
		.elem_size      = sizeof(u32),
		.array_type       = VAR_LEN_ARRAY,
		.tlv_type       = 0x10,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   feature_list),
	},
	{
		.data_type      = QMI_OPT_FLAG,
		.elem_len       = 1,
		.elem_size      = sizeof(u8),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x11,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   file_result_valid),
	},
	{
		.data_type      = QMI_DATA_LEN,
		.elem_len       = 1,
		.elem_size      = sizeof(u8),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x11,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   file_result_len),
	},
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = QMI_LM_MAX_LICENSE_FILES_V01,
		.elem_size      = sizeof(u32),
		.array_type       = VAR_LEN_ARRAY,
		.tlv_type       = 0x11,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   file_result),
	},
	{
		.data_type      = QMI_OPT_FLAG,
		.elem_len       = 1,
		.elem_size      = sizeof(u8),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x12,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   lic_common_error_valid),
	},
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u32),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x12,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   lic_common_error),
	},
	{
		.data_type      = QMI_OPT_FLAG,
		.elem_len       = 1,
		.elem_size      = sizeof(u8),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x13,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   oem_id_valid),
	},
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u32),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x13,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   oem_id),
	},
	{
		.data_type      = QMI_OPT_FLAG,
		.elem_len       = 1,
		.elem_size      = sizeof(u8),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x14,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   jtag_id_valid),
	},
	{
		.data_type      = QMI_UNSIGNED_4_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u32),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x14,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   jtag_id),
	},
	{
		.data_type      = QMI_OPT_FLAG,
		.elem_len       = 1,
		.elem_size      = sizeof(u8),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x15,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   serial_number_valid),
	},
	{
		.data_type      = QMI_UNSIGNED_8_BYTE,
		.elem_len       = 1,
		.elem_size      = sizeof(u64),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x15,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_req_msg_v01,
					   serial_number),
	},
	{
		.data_type      = QMI_EOTI,
		.array_type       = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};
EXPORT_SYMBOL(qmi_lm_feature_list_req_msg_v01_ei);

struct qmi_elem_info qmi_lm_feature_list_resp_msg_v01_ei[] = {
	{
		.data_type      = QMI_STRUCT,
		.elem_len       = 1,
		.elem_size      = sizeof(struct qmi_response_type_v01),
		.array_type       = NO_ARRAY,
		.tlv_type       = 0x02,
		.offset         = offsetof(struct
					   qmi_lm_feature_list_resp_msg_v01,
					   resp),
		.ei_array      = qmi_response_type_v01_ei,
	},
	{
		.data_type      = QMI_EOTI,
		.array_type       = NO_ARRAY,
		.tlv_type       = QMI_COMMON_TLV_TYPE,
	},
};
EXPORT_SYMBOL(qmi_lm_feature_list_resp_msg_v01_ei);

static int lm_read_license_file(struct lm_svc_ctx *svc, const char *filename)
{
	const struct firmware *license = NULL;
	struct device *dev = svc->dev;
	size_t lic_size_aligned;
	char *magic = NULL;
	void *buf;
	int ret;

	dev_dbg(dev,"License file: %s\n",filename);

	ret = request_firmware(&license, filename, dev);
	if(ret || !license->data || !license->size) {
		dev_err(svc->dev,"%s file is not present\n", filename);
		/* if ret is zero, then call release_firmware */
		if (!ret)
			release_firmware(license);
		return -ENOENT;
	}

	/* Copy license data in TLV format at license_buf. Each
	 * license data size are 4 bytes aligned */
	lic_size_aligned = ALIGN(license->size, 4);

	if (svc->license_buf_len + lic_size_aligned + HEADER_SIZE > LICENSE_BUF_MAX) {
		dev_err(svc->dev, "License files exceeded the MAX %d Bytes\n",
					LICENSE_BUF_MAX);
		svc->license_files->num_of_file = 0;
		release_firmware(license);
		return -ENOMEM;
	}

	/* Take a copy license file names */
	memcpy(svc->license_files->file_name[svc->license_files->num_of_file],
			filename, FILE_NAME_MAX);
	svc->license_files->num_of_file = svc->license_files->num_of_file + 1;

	buf = svc->license_buf + svc->license_buf_len;

	magic = LICENSE_MAGIC;

	memcpy(buf, magic, MAGIC_SIZE);
	memcpy(buf + MAGIC_SIZE,
			(void *)&license->size, TLV_LENGTH_SIZE);
	memcpy(buf + HEADER_SIZE,
			license->data, license->size);

	svc->license_buf_len = svc->license_buf_len + lic_size_aligned + HEADER_SIZE;

	release_firmware(license);

	return 0;
}

static int lm_get_license_in_tlv(struct lm_svc_ctx *svc, bool rescan) {
	int ret = 0, file_count = 0, files_accounted = 0;
	const struct firmware *licenseinfo = NULL;
	struct device *dev = svc->dev;
	const char *filename = NULL;
	void *lic_info_buf;
	char *token = NULL;
	char *ptr = NULL;

	/* Check the buffer status again */
	if (svc->license_buf_valid && !rescan)
		return 0;

	/* Reset the License data length and file names copies*/
	svc->license_buf_len = 0;
	svc->license_files->num_of_file = 0;

	/* Check if license file name present in DTS */
	if (of_property_read_string(dev->of_node, "license-file", &filename) == 0) {
		if (filename != NULL && (strlen(filename) != 0)) {
			dev_dbg(dev, "License file name present in DTS\n");
			ret = lm_read_license_file(svc, filename);
			/* set license_buffer is valid */
			if (svc->license_buf_len)
				svc->license_buf_valid = true;
			return ret;
		}
	}

	lic_info_buf = kmalloc(MAX_LICENSE_INFO_SIZE, GFP_KERNEL);
	if(!lic_info_buf)
		return -ENOMEM;

	/* Request the license_info.conf file */
	ret = request_firmware_into_buf(&licenseinfo, LICENSE_INFO_CONF_PATH, dev,
					lic_info_buf, MAX_LICENSE_INFO_SIZE);

	if(ret || !licenseinfo->data || !licenseinfo->size) {
		dev_err(svc->dev, "%s file is not valid\n",LICENSE_INFO_CONF_PATH);
		/* if ret is zero, then call release_firmware */
		if (!ret)
			release_firmware(licenseinfo);
		kfree(lic_info_buf);
		return -ENOENT;
	}

	/* Check the license_info.conf has minimum size */
	if (licenseinfo->size < (sizeof(LICENSE_INFO_START) + sizeof(FILE_COUNT_STRING))) {
		dev_err(svc->dev, "%s file not meeting the minimum size\n", LICENSE_INFO_CONF_PATH);
		ret = -EINVAL;
		goto err_licenseinfo;
	}

	/* Check the license_info.conf start */
	if (strncmp(licenseinfo->data, LICENSE_INFO_START, sizeof(LICENSE_INFO_START)-1)) {
		dev_err(svc->dev, "%s : %s not present\n", LICENSE_INFO_CONF_PATH, LICENSE_INFO_START);
		ret = -EINVAL;
		goto err_licenseinfo;
	}

	/* Check the filecount string and get the value */
	ptr = (char *)licenseinfo->data + sizeof(LICENSE_INFO_START);
	token = strsep(&ptr, " ");
	if (!token || !*token || strncmp(token, FILE_COUNT_STRING, sizeof(FILE_COUNT_STRING))) {
		dev_err(svc->dev, "%s: %s string not present\n", LICENSE_INFO_CONF_PATH, FILE_COUNT_STRING);
		ret = -EINVAL;
		goto err_licenseinfo;
	}

	token = strsep(&ptr, "\n");
	if (!token || !*token) {
		dev_err(svc->dev, "%s file count not valid\n", LICENSE_INFO_CONF_PATH);
		ret = -EINVAL;
		goto err_licenseinfo;
	}

	ret = kstrtoint(token, 0, &file_count);
	if (ret || file_count <= 0 || file_count > QMI_LM_MAX_LICENSE_FILES_V01)  {
		dev_err(svc->dev, "%s file count is invalid %d\n", LICENSE_INFO_CONF_PATH, file_count);
		ret = -EINVAL;
		goto err_licenseinfo;
	}

	dev_dbg(dev,"%s file is present with %d license file names\n",
			LICENSE_INFO_CONF_PATH, file_count);

	while (((token = strsep(&ptr, " ")) != NULL) && (files_accounted < file_count)) {
		/* Increment the accounted file count */
		files_accounted++;

		/* Check file string is present */
		if (!*token || strncmp(token, FILE_STRING, sizeof(FILE_STRING))) {
			dev_err(svc->dev, "%s: File name syntax is wrong\n", LICENSE_INFO_CONF_PATH);
			token = strsep(&ptr, "\n");
			continue;
		}

		/* Get the file name */
		token = strsep(&ptr, "\n");
		if (!token || !*token) {
			dev_err(svc->dev, "%s: File name is wrong\n",LICENSE_INFO_CONF_PATH);
			continue;
		}

		ret = lm_read_license_file(svc, token);
		if (ret < 0 && ret!= -ENOENT)
			goto err_licenseinfo;
	}

	/* set license_buffer is valid */
	if (svc->license_buf_len)
		svc->license_buf_valid = true;

err_licenseinfo:
	release_firmware(licenseinfo);
	kfree(lic_info_buf);

	return ret;
}

static int lm_get_license_buffer(struct lm_svc_ctx *svc, bool rescan) {
	int ret;

	/* Check if the license buffer is valid */
	if (svc->license_buf_valid && !rescan)
		return 0;

	mutex_lock(&license_valid_lock);

	ret = lm_get_license_in_tlv(svc, rescan);

	if(ret || !svc->license_buf_len)
		svc->license_buf_valid = false;

	mutex_unlock(&license_valid_lock);

	return ret;
}

static void *lm_get_license_meta_buffer(dma_addr_t *dma_addr) {
	struct lm_svc_ctx *svc = lm_svc;
	char *magic;
	u32 license_metadata_len = 8;
	void *lic_buf;

	/* Allocate buffer for ECDSA blob */
	lic_buf = dma_alloc_coherent(svc->dev, LICENSE_META_DATA_SIZE, dma_addr, GFP_KERNEL);
	if (!lic_buf) {
		dev_err(svc->dev, "Failed to create buffer for license meta data (SSLM) \n");
		return ERR_PTR(-ENOMEM);
	}

	/* Magic */
	magic = LICENSE_META_DATA_MAGIC;
	memcpy(lic_buf, magic, MAGIC_SIZE);
	/* Length 8 bytes, Addr + Length */
	memcpy(lic_buf + MAGIC_SIZE, (void *)&license_metadata_len,
			TLV_LENGTH_SIZE);
	/* Copy License addr and length */
	memcpy(lic_buf + HEADER_SIZE,
			(void *)&svc->license_dma_addr, 4);
	memcpy(lic_buf + HEADER_SIZE + 4, (void *)&svc->license_buf_len,
			TLV_LENGTH_SIZE);

	return lic_buf;
}

static void *lm_get_ecdsa_buffer(dma_addr_t *dma_addr, dma_addr_t nonce_dma_addr) {
	struct lm_svc_ctx *svc = lm_svc;
	u32 ecdsa_consumed;
	char *magic;
	int ret;
	u32 license_metadata_len = 8;
	void *ecdsa_buf, *lic_buf;

	/* Allocate buffer for ECDSA blob */
	ecdsa_buf = dma_alloc_coherent(svc->dev, ECDSA_BUF_MAX, dma_addr, GFP_KERNEL);
	if (!ecdsa_buf) {
		dev_err(svc->dev, "License is SoC bounded and failed to create ECDSA buffer\n");
		return ERR_PTR(-ENOMEM);
	}

	/* Get the ECDSA blob from TZ/TME-L. Pass the ECDSA start + HEADER_SIZE */
	ret = qti_scm_get_ecdsa_blob(QWES_SVC_ID, QWES_ECDSA_REQUEST, nonce_dma_addr,
					NONCE_SIZE, *dma_addr + HEADER_SIZE, ECDSA_DATA_SIZE,
					&ecdsa_consumed);
	if (ret) {
		dev_err(svc->dev, "Failed to get the ECDSA blob from TZ/TME-L, ret %d\n", ret);
		return ERR_PTR(ret);
	}

	/* Copy ECDSA magic and length */
	magic = ECDSA_MAGIC;
	memcpy(ecdsa_buf, magic, MAGIC_SIZE);
	memcpy(ecdsa_buf + MAGIC_SIZE, (void *)&ecdsa_consumed, TLV_LENGTH_SIZE);

	/* Copy License meta data at end of ECDSA TLV */
	lic_buf = ecdsa_buf + HEADER_SIZE + ALIGN(ecdsa_consumed, 4);
	/* Magic */
	magic = LICENSE_META_DATA_MAGIC;
	memcpy(lic_buf, magic, MAGIC_SIZE);
	/* Length 8 bytes, Addr + Length */
	memcpy(lic_buf + MAGIC_SIZE, (void *)&license_metadata_len,
			TLV_LENGTH_SIZE);
	/* Copy License addr and length */
	memcpy(lic_buf + HEADER_SIZE,
			(void *)&svc->license_dma_addr, 4);
	memcpy(lic_buf + HEADER_SIZE + 4, (void *)&svc->license_buf_len,
			TLV_LENGTH_SIZE);

	return ecdsa_buf;
}

void *lm_get_license(enum req_type type, dma_addr_t *dma_addr, size_t *buf_len,
		dma_addr_t nonce_dma_addr) {
	struct lm_svc_ctx *svc = lm_svc;
	void *buf;
	int ret;

	if (!svc || !svc->license_feature || !svc->license_buf) {
		ret = -EIO;
		goto err;
	}

	if (type >= TYPE_MAX) {
		dev_dbg(svc->dev, "Invalid req_type given\n");
		ret = -EINVAL;
		goto err;
	}

	/* Get the license buffer */
	ret = lm_get_license_buffer(svc, false);
	if(ret || !svc->license_buf_valid)
		goto err;

	/* SSLD format for Internal Q6 */
	if (type == INTERNAL) {
		*dma_addr = svc->license_dma_addr;
		*buf_len = svc->license_buf_len;

		/* increment atomic counter to keep track of buf use */
		atomic_inc(&buf_use_count);

		return svc->license_buf;

	} else if (type == EXTERNAL) {
		/* Check if license is endpoint bounded, then send SSLM */
		if (!svc->soc_bounded) {
			buf = lm_get_license_meta_buffer(dma_addr);
			if (IS_ERR(buf)) {
				dev_err(svc->dev, "SSLM buffer not prepared\n");
				goto err;
			}

			/* Copy the License meta data buffer size */
			*buf_len = LICENSE_META_DATA_SIZE;
		} else {
			/* If license is soc bounded, then send ECDSA + SSLM */
			if (!nonce_dma_addr) {
				dev_err(svc->dev, "NONCE is not present. It is expected as the license is SoC bounded\n");
				goto err;
			}

			buf = lm_get_ecdsa_buffer(dma_addr, nonce_dma_addr);
			if (IS_ERR(buf)) {
				dev_err(svc->dev, "ECDSA buffer not prepared\n");
				goto err;
			}

			/* Copy the ECDSA buffer size */
			*buf_len = ECDSA_BUF_MAX;
		}

		/* increment atomic counter to keep track of buf use */
		atomic_inc(&buf_use_count);

		return buf;
	}
err:
	*buf_len = 0;
	*dma_addr = 0;
	return NULL;
}
EXPORT_SYMBOL_GPL(lm_get_license);

void lm_free_license(void *buf, dma_addr_t dma_addr, size_t buf_len) {
	struct lm_svc_ctx *svc = lm_svc;

	/* Decrement the atomic counter */
	if (atomic_read(&buf_use_count) > 0)
		atomic_dec(&buf_use_count);

	/* Free the ECDSA buffer alone */
	if (buf != NULL && buf != svc->license_buf) {
		dev_dbg(svc->dev, "Freeing ECDSA buffer \n");
		dma_free_coherent(svc->dev, buf_len, buf, dma_addr);
	}
}
EXPORT_SYMBOL_GPL(lm_free_license);

static int lm_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int lm_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long lm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct lm_svc_ctx *svc = lm_svc;
	struct client_target_info *client_info = NULL;
	struct feature_info *itr, *tmp;
	dma_addr_t nonce_dma_addr, ecdsa_dma_addr;
	void *nonce_buf, *ecdsa_buf;
	struct bindings_resp br;
	u32 ecdsa_consumed;
	int i, len = 0, ret = 0;

	switch(cmd) {
		case LICENSE_RESCAN:
			if (svc->license_feature && svc->license_buf && !atomic_read(&buf_use_count)) {
				ret = lm_get_license_buffer(svc, true);
				if (ret)
					dev_err(svc->dev, "License file rescan failed\n");
			} else {
				dev_err(svc->dev, "License feature not enabled / resource busy \n");
				ret = -EIO;
			}
		break;

		case GET_FID_INFO:
			client_info = kzalloc(sizeof(struct client_target_info), GFP_KERNEL);
			if (!client_info) {
				dev_err(svc->dev, "Unable to allocate memory for client info\n");
				return -ENOMEM;
			}

			if (!list_empty(&svc->clients_feature_list)) {
				len = 0;
				list_for_each_entry_safe(itr, tmp,
						&svc->clients_feature_list, node) {
					client_info->info[len].sq_node = itr->sq_node;
					client_info->info[len].sq_port = itr->sq_port;
					client_info->info[len].list_len = itr->len;
					for(i = 0; i < itr->len; i++)
						client_info->info[len].list[i] = itr->list[i];
					len++;
				}

				client_info->info_len = len;
			}

			ret = copy_to_user(argp, client_info, sizeof(struct client_target_info));
			if (ret) {
				dev_err(svc->dev, "copy to user error\n");
			}

			kfree(client_info);
		break;

		case GET_BINDINGS:
			ret = copy_from_user(&br, argp, sizeof(struct bindings_resp));
			if (ret) {
				dev_err(svc->dev, "IOCTL: bindings_resp copy from user error\n");
				return ret;
			}
			if ((br.nonce_buf_len != NONCE_SIZE) || (br.ecdsa_buf_len != ECDSA_DATA_SIZE)) {
				dev_err(svc->dev, "IOCTL: nonce/ECDSA buffer size invalid\n");
				return -EINVAL;
			}

			nonce_buf = dma_alloc_coherent(svc->dev, NONCE_SIZE,
						       &nonce_dma_addr, GFP_KERNEL);
			if (!nonce_buf) {
				dev_err(svc->dev, "IOCTL: NONCE coherent mem alloc failed\n");
				return -ENOMEM;
			}

			ecdsa_buf = dma_alloc_coherent(svc->dev, ECDSA_DATA_SIZE,
						       &ecdsa_dma_addr, GFP_KERNEL);
			if (!ecdsa_buf) {
				dev_err(svc->dev, "IOCTL: ECDSA coherent mem alloc failed\n");
				ret = -ENOMEM;
				goto ecdsa_dma_alloc_err;
			}

			ret = copy_from_user(nonce_buf, br.nonce_buf, br.nonce_buf_len);
			if (ret) {
				dev_err(svc->dev, "IOCTL: NONCE copy from user error\n");
				goto err;
			}

			ret = qti_scm_get_ecdsa_blob(QWES_SVC_ID, QWES_ECDSA_REQUEST,
						     nonce_dma_addr, NONCE_SIZE,
						     ecdsa_dma_addr, ECDSA_DATA_SIZE,
						     &ecdsa_consumed);
			if (ret) {
				dev_err(svc->dev, "IOCTL: Failed to get the ECDSA blob from TZ, ret %d\n", ret);
				goto err;
			}

			br.ecdsa_consumed_len = ecdsa_consumed;
			ret = copy_to_user(br.ecdsa_buf, ecdsa_buf, ecdsa_consumed);
			if (ret) {
				dev_err(svc->dev, "IOCTL: ECDSA copy to user error\n");
				goto err;
			}

			ret = copy_to_user(argp, &br, sizeof(br));
			if (ret)
				dev_err(svc->dev, "IOCTL: ecdsa_resp copy to user error\n");

		err:
			dma_free_coherent(svc->dev, ECDSA_DATA_SIZE, ecdsa_buf, ecdsa_dma_addr);

		ecdsa_dma_alloc_err:
			dma_free_coherent(svc->dev, NONCE_SIZE, nonce_buf, nonce_dma_addr);

		break;

		default:
			dev_err(svc->dev, "%x cmd not handled", cmd);
			ret = -ENODEV;
	}

	return ret;
}

static struct file_operations fops =
{
	.owner          = THIS_MODULE,
	.open           = lm_open,
	.unlocked_ioctl = lm_ioctl,
	.release        = lm_release,
};

static int lm_ioctl_init(struct lm_svc_ctx *svc)
{
	int ret;
	struct device *device;

	ret = alloc_chrdev_region(&chr_dev, 0, 1, "lm_dev");
	if (ret) {
		dev_err(svc->dev, "IOCTL: major number allocation failure\n");
		return ret;
	}

	/*Creating cdev structure*/
	cdev_init(&lm_cdev,&fops);

	/*Adding character device to the system*/
	ret = cdev_add(&lm_cdev,chr_dev,1);
	if (ret) {
		dev_err(svc->dev, "IOCTL: adding device failed\n");
		goto out_chrdev;
	}

	/* Creating struct class */
	dev_class = class_create(THIS_MODULE,"lm_class");
	if (IS_ERR(dev_class)){
		dev_err(svc->dev, "IOCTL: struct class creation failed\n");
		ret = PTR_ERR(dev_class);
		goto out_cdev;
	}

	/* Creating device */
	device = device_create(dev_class,NULL,chr_dev,NULL,"lm_device");
	if (IS_ERR(device)) {
		dev_err(svc->dev, "IOCTL: device creation failed\n");
		ret = PTR_ERR(device);
		goto out_class;
	}

	return 0;

out_class:
	class_destroy(dev_class);
out_cdev:
	cdev_del(&lm_cdev);
out_chrdev:
	unregister_chrdev_region(chr_dev,1);

	return ret;
}

static void lm_ioctl_free(void) {
	device_destroy(dev_class, chr_dev);
	class_destroy(dev_class);
	cdev_del(&lm_cdev);
	unregister_chrdev_region(chr_dev,1);
}


static void qmi_handle_feature_list_req(struct qmi_handle *handle,
			struct sockaddr_qrtr *sq,
			struct qmi_txn *txn,
			const void *decoded_msg)
{
	struct qmi_lm_feature_list_req_msg_v01 *req;
	struct qmi_lm_feature_list_resp_msg_v01 *resp;
	struct feature_info *licensed_features, *itr, *tmp;
	int i, ret;

	req = (struct qmi_lm_feature_list_req_msg_v01 *)decoded_msg;

	pr_debug("Licensed Features: Request rcvd, node_id: 0x%x", sq->sq_node);

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		pr_err("%s: Memory allocation failed for resp buffer\n",
							__func__);
		goto free_resp_buf;
	}
	resp->resp.result = QMI_RESULT_FAILURE_V01;

	if(!req->feature_list_valid) {
		pr_err("No Features are licensed in node 0x%x\n",sq->sq_node);
	}

	licensed_features = kzalloc(sizeof(*licensed_features), GFP_KERNEL);
	if (!licensed_features) {
		pr_err("%s: Memory allocation failed for feature list\n",
							__func__);
		goto send_resp;
	}

	licensed_features->sq_node = sq->sq_node;
	licensed_features->sq_port = sq->sq_port;
	licensed_features->reserved = req->reserved;
	if(!req->feature_list_valid) {
		licensed_features->len = 0;
	} else {
		licensed_features->len = req->feature_list_len;
		if(licensed_features->len > QMI_LM_MAX_FEATURE_LIST_V01) {
			pr_err("Feature_list larger than QMI_MAX_FEATURE_LIST_V01"
					"%d, so assiging it to max \n",
					QMI_LM_MAX_FEATURE_LIST_V01);
			licensed_features->len = QMI_LM_MAX_FEATURE_LIST_V01;
		}

		for(i =0; i<licensed_features->len; i++)
			licensed_features->list[i] = req->feature_list[i];

	}

	/* Copy license file status */
	if (!req->file_result_valid) {
		licensed_features->file_result_len = 0;
	} else {
		licensed_features->file_result_len = req->file_result_len;
		if(licensed_features->file_result_len > QMI_LM_MAX_LICENSE_FILES_V01) {
			pr_err("file_result larger than QMI_LM_MAX_LICENSE_FILES_V01"
					"%d, so assiging it to max \n",
					QMI_LM_MAX_LICENSE_FILES_V01);
			licensed_features->file_result_len = QMI_LM_MAX_LICENSE_FILES_V01;
		}
		for(i=0; i<licensed_features->file_result_len; i++)
			licensed_features->file_result[i] = req->file_result[i];
	}

	/* Copy common error, OEM ID, JTAG ID and Serial Number */
	if (!req->lic_common_error_valid)
		licensed_features->lic_common_error = 0;
	else
		licensed_features->lic_common_error = req->lic_common_error;

	if (!req->oem_id_valid)
		licensed_features->oem_id = 0;
	else
		licensed_features->oem_id = req->oem_id;

	if (!req->jtag_id_valid)
		licensed_features->jtag_id = 0;
	else
		licensed_features->jtag_id = req->jtag_id;

	if (!req->serial_number_valid)
		licensed_features->serial_number = 0;
	else
		licensed_features->serial_number = req->serial_number;

	if (!list_empty(&lm_svc->clients_feature_list)) {
		list_for_each_entry_safe(itr, tmp, &lm_svc->clients_feature_list,
								node) {
			if (itr->sq_node == sq->sq_node) {
				list_del(&itr->node);
				kfree(itr);
			}
		}
	}

	list_add_tail(&licensed_features->node, &lm_svc->clients_feature_list);
	resp->resp.result = QMI_RESULT_SUCCESS_V01;

send_resp:
	ret = qmi_send_response(handle, sq, txn,
			QMI_LM_FEATURE_LIST_RESP_V01,
			QMI_LM_FEATURE_LIST_RESP_MSG_V01_MAX_MSG_LEN,
			qmi_lm_feature_list_resp_msg_v01_ei, resp);
	if (ret < 0)
		pr_err("%s: Sending license termination response failed"
					"with error_code:%d\n",__func__,ret);
	else
		pr_debug("Licensed Features: Response sent, Result code "
			"%d\n", resp->resp.result);
free_resp_buf:
	kfree(resp);

}

static ssize_t show_licensed_features(struct kobject *k,
				struct kobj_attribute *attr, char *buf)
{
	uint32_t i, len = 0, max_buf_len = PAGE_SIZE;
	struct feature_info *itr, *tmp;

	if (!list_empty(&lm_svc->clients_feature_list)) {
		list_for_each_entry_safe(itr, tmp,
					&lm_svc->clients_feature_list, node) {
			if(itr->len == 0) {
				len += scnprintf(buf+len, max_buf_len-len,
					"\nClient Node:0x%x Port:%d,"
					" No feature licensed\n",itr->sq_node,
					itr->sq_port);
			} else {
				len += scnprintf(buf+len, max_buf_len-len,
					"\nClient Node:0x%x Port:%d,"
					" %d features licensed\n"
					" Feature List:\n", itr->sq_node,
					itr->sq_port, itr->len);
				for(i=0;i<itr->len;i++)
					 len += scnprintf(buf+len,
						max_buf_len-len,
						" %d\n",itr->list[i]);
			}

			if(itr->file_result_len == 0) {
				len += scnprintf(buf+len, max_buf_len-len,
					"\nNo license file status\n");
			} else {
				len += scnprintf(buf+len, max_buf_len-len,
					"\n%d license file status received\n",
					itr->file_result_len);
				for(i=0; i < itr->file_result_len; i++) {
					if (lm_svc->license_files->num_of_file &&
						i < lm_svc->license_files->num_of_file)
						len += scnprintf(buf+len, max_buf_len-len,
							" %s : %u\n",lm_svc->license_files->file_name[i],
							itr->file_result[i]);
					else
						len += scnprintf(buf+len, max_buf_len-len,
							" Invalid status : %u\n",
							itr->file_result[i]);
				}
			}

			len += scnprintf(buf+len, max_buf_len-len,
					"License common error: %u\n",
					itr->lic_common_error);

			len += scnprintf(buf+len, max_buf_len-len,
					"\nAdditional Info: 0x%08x\n",
					itr->reserved);

			if (itr->jtag_id || itr->oem_id || itr->serial_number)
				len += scnprintf(buf+len, max_buf_len-len,
					"JTAG ID: 0x%x\nOEM ID: 0x%x\n"
					"Serial Number: 0x%llx\n", itr->jtag_id,
					itr->oem_id, itr->serial_number);
		}
	} else
		len += scnprintf(buf+len, max_buf_len-len,
			"Client's licensed feature list not available\n");

	return len;
}

static struct kobj_attribute lm_licensed_features_attr =
	__ATTR(licensed_features, 0400,show_licensed_features,  NULL);

static ssize_t store_license_rescan(struct kobject *k, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	struct lm_svc_ctx *svc = lm_svc;
	int ret;

	if (svc->license_feature && svc->license_buf && !atomic_read(&buf_use_count)) {
		ret = lm_get_license_buffer(svc, true);
		if (ret)
			dev_err(svc->dev, "License file rescan failed\n");
	}

	return count;
}

static struct kobj_attribute lm_license_rescan_attr =
	__ATTR(license_rescan, 0200, NULL, store_license_rescan);

static void lm_qmi_svc_bye_cb(struct qmi_handle *qmi, unsigned int node)
{
	struct feature_info *itr, *tmp;

	/* Clear feature list based on node ID */
	if (!list_empty(&lm_svc->clients_feature_list)) {
		list_for_each_entry_safe(itr, tmp, &lm_svc->clients_feature_list,
								node) {
			if (itr->sq_node == node) {
				pr_debug("Received LM QMI Bye from node: 0x%x \n", node);
				list_del(&itr->node);
				kfree(itr);
			}
		}
	}
}

static struct qmi_ops lm_server_ops = {
	.bye = lm_qmi_svc_bye_cb,
};
static struct qmi_msg_handler lm_req_handlers[] = {
	{
		.type = QMI_REQUEST,
		.msg_id = QMI_LM_FEATURE_LIST_REQ_V01,
		.ei = qmi_lm_feature_list_req_msg_v01_ei,
		.decoded_size = sizeof(struct qmi_lm_feature_list_req_msg_v01),
		.fn = qmi_handle_feature_list_req,
	},
	{}
};

static int license_manager_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	int ret = 0;
	struct lm_svc_ctx *svc;

	svc = kzalloc(sizeof(struct lm_svc_ctx), GFP_KERNEL);
	if (!svc)
		return -ENOMEM;

	svc->license_feature = of_property_read_bool(node, "license-feature");

	if (svc->license_feature) {
		svc->license_buf = dma_alloc_coherent(dev, LICENSE_BUF_MAX,
				&svc->license_dma_addr, GFP_KERNEL);
		if (!svc->license_buf) {
			dev_err(dev, "Failed to allocated DMA memory for License files\n");
			ret = -ENOMEM;
			goto free_lm_svc;
		}

		svc->soc_bounded = of_property_read_bool(node, "soc-bounded");

		svc->license_files = kzalloc(sizeof(struct lm_files), GFP_KERNEL);
		if (!svc->license_files) {
			dev_err(dev, "Failed to allocate memory for license file names\n");
			ret = -ENOMEM;
			goto free_lm_lic_buf;
		}

	}

	/* Create IOCTL for userspace */
	ret = lm_ioctl_init(svc);
	if (ret) {
		dev_err(dev, "Failed to create IOCTL\n");
		goto free_lm_lic_names;
	}

	svc->lm_svc_hdl = kzalloc(sizeof(struct qmi_handle), GFP_KERNEL);
	if (!svc->lm_svc_hdl) {
		ret = -ENOMEM;
		dev_err(dev, "Mem allocation failed for LM svc handle %d\n", ret);
		goto deinit_ioctl;
	}
	ret = qmi_handle_init(svc->lm_svc_hdl,
				QMI_LICENSE_MANAGER_SERVICE_MAX_MSG_LEN,
				&lm_server_ops,
				lm_req_handlers);
	if (ret < 0) {
		dev_err(dev, "Error registering license manager svc %d\n", ret);
		goto free_lm_svc_handle;
	}
	ret = qmi_add_server(svc->lm_svc_hdl, QMI_LM_SERVICE_ID_V01,
					QMI_LM_SERVICE_VERS_V01,
					0);
	if (ret < 0) {
		dev_err(dev, "Failed to add license manager svc server :%d\n", ret);
		goto release_lm_svc_handle;
	}

	INIT_LIST_HEAD(&svc->clients_feature_list);

	svc->dev = dev;
	lm_svc = svc;

	/* Creating a directory in /sys/kernel/ */
	lm_kobj = kobject_create_and_add("license_manager", kernel_kobj);
	if (lm_kobj) {
		if (sysfs_create_file(lm_kobj, &lm_licensed_features_attr.attr)) {
			dev_err(dev, "Cannot create licensed_features sysfs file for lm\n");
			kobject_put(lm_kobj);
		}
		if (sysfs_create_file(lm_kobj, &lm_license_rescan_attr.attr)) {
			dev_err(dev, "Cannot create license_rescan sysfs file for lm\n");
			kobject_put(lm_kobj);
		}
	} else {
		pr_err("Unable to create license manager sysfs entry\n");
	}

	dev_info(dev, "License Manager registered. License feature is %s and %s\n",
			svc->license_feature ? "enabled" : "disabled",
			svc->soc_bounded ? "SoC Bounded" : "Endpoint Bounded");

	return 0;

release_lm_svc_handle:
	qmi_handle_release(svc->lm_svc_hdl);
free_lm_svc_handle:
	kfree(svc->lm_svc_hdl);
deinit_ioctl:
	lm_ioctl_free();
free_lm_lic_names:
	if(svc->license_files)
		kfree(svc->license_files);
free_lm_lic_buf:
	if(svc->license_buf)
		dma_free_coherent(dev, LICENSE_BUF_MAX, svc->license_buf, svc->license_dma_addr);
free_lm_svc:
	kfree(svc);

	return ret;
}

static int license_manager_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct lm_svc_ctx *svc = lm_svc;
	struct feature_info *iter, *temp;

	qmi_handle_release(svc->lm_svc_hdl);

	if (!list_empty(&svc->clients_feature_list)) {
		list_for_each_entry_safe(iter, temp, &svc->clients_feature_list,
								node) {
			list_del(&iter->node);
			kfree(iter);
		}
	}

	if (svc->license_buf)
		dma_free_coherent(dev, LICENSE_BUF_MAX, svc->license_buf, svc->license_dma_addr);

	if(svc->license_files)
		kfree(svc->license_files);

	kfree(svc->lm_svc_hdl);
	kfree(svc);

	lm_ioctl_free();

	lm_svc = NULL;

	return 0;
}

static const struct of_device_id of_license_manager_match[] = {
	{.compatible = "qti,license-manager-service"},
	{  /* sentinel value */ },
};

static struct platform_driver license_manager_driver = {
	.probe		= license_manager_probe,
	.remove		= license_manager_remove,
	.driver		= {
		.name 	= "lm",
		.of_match_table	= of_license_manager_match,
	},
};

static int __init license_manager_init(void)
{
	return platform_driver_register(&license_manager_driver);
}
module_init(license_manager_init);

static void __exit license_manager_exit(void)
{
	platform_driver_unregister(&license_manager_driver);
}
module_exit(license_manager_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("License manager driver");
