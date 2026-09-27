// SPDX-License-Identifier: GPL-2.0
/*
 * Sample in-kernel QMI client driver
 *
 * Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 * Copyright (C) 2017 Linaro Ltd.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/qrtr.h>
#include <linux/net.h>
#include <linux/completion.h>
#include <linux/idr.h>
#include <linux/string.h>
#include <net/sock.h>
#include <linux/soc/qcom/qmi.h>
#include <linux/kthread.h>

#define PING_REQ1_TLV_TYPE		0x1
#define PING_RESP1_TLV_TYPE		0x2
#define PING_OPT1_TLV_TYPE		0x10
#define PING_OPT2_TLV_TYPE		0x11

#define DATA_REQ1_TLV_TYPE		0x1
#define DATA_RESP1_TLV_TYPE		0x2
#define DATA_OPT1_TLV_TYPE		0x10
#define DATA_OPT2_TLV_TYPE		0x11

#define TEST_MED_DATA_SIZE_V01		8192
#define TEST_SML_DATA_SIZE_V01		255
#define TEST_MAX_NAME_SIZE_V01		255

#define TEST_PING_REQ_MSG_ID_V01	0x20
#define TEST_DATA_REQ_MSG_ID_V01	0x21

#define TEST_PMIC_GET_RAIL_INFO		0x29

#define TEST_PMIC_SET_RAIL_VOLT		0x2A

#define TEST_PING_REQ_MAX_MSG_LEN_V01	266
#define TEST_DATA_REQ_MAX_MSG_LEN_V01	8456

#define LIC_GET_OEM_ID_REQ_V01		0x30
#define LIC_GET_SERIAl_NUM_REQ_V01	0x31
#define LIC_GET_EXTERNAL_PART_REQ_V01	0x32
#define LIC_GET_JTAG_ID_REQ_V01		0x33

/* Number of iterations to run during test */
static unsigned long niterations = 5;
module_param_named(niterations, niterations, ulong, S_IRUGO | S_IWUSR | S_IWGRP);

/* Size of data during "data" command */
static unsigned long data_size = 50;
module_param_named(data_size, data_size, ulong, S_IRUGO | S_IWUSR | S_IWGRP);

/* Number of cuncurrent Threads running during test */
static unsigned long nthreads = 5;
module_param_named(nthreads, nthreads, ulong, S_IRUGO | S_IWUSR | S_IWGRP);

/* Variable to hold the test result */
static unsigned long test_res;

/* Data element to be queued during multiple commands */
struct test_qmi_data {
	struct list_head list;
	char data[64];
	atomic_t refs_count;
};

/* DebugFS directory structure for QMI */
static struct qmi_dir {
	char string[16];
	unsigned long *value;
	umode_t mode;
}qdentry[] = {
	{"test", &test_res, S_IRUGO | S_IWUGO},
	{"niterations", &niterations, S_IRUGO | S_IWUGO},
	{"data_size", &data_size, S_IRUGO | S_IWUGO},
	{"nthreads", &nthreads, S_IRUGO | S_IWUGO},
};

static struct mutex status_print_lock;
u8 rail_id;
u32 rail_voltage_uv;
u32 oem_id;
u32 serial_num;
u32 external_part;
u32 jtag_id;

struct test_name_type_v01 {
	u32 name_len;
	char name[TEST_MAX_NAME_SIZE_V01];
};

static struct qmi_elem_info test_name_type_v01_ei[] = {
	{
		.data_type	= QMI_DATA_LEN,
		.elem_len	= 1,
		.elem_size	= sizeof(u8),
		.array_type	= NO_ARRAY,
		.tlv_type	= QMI_COMMON_TLV_TYPE,
		.offset		= offsetof(struct test_name_type_v01,
					   name_len),
	},
	{
		.data_type	= QMI_UNSIGNED_1_BYTE,
		.elem_len	= TEST_MAX_NAME_SIZE_V01,
		.elem_size	= sizeof(char),
		.array_type	= VAR_LEN_ARRAY,
		.tlv_type	= QMI_COMMON_TLV_TYPE,
		.offset		= offsetof(struct test_name_type_v01,
					   name),
	},
	{}
};

struct qmi_pmic_railinfo {
	u8 rail_id;
	u8 cpr_mode;
	char name[14];
	u32 voltage_uv; /* value in micro volts */
}__packed;

struct pmic_rail_info_req_v01 {
	union {
		char arr[4];
		u32 rail_id;
	};

	u8 client_name_valid;
	struct test_name_type_v01 client_name;
}__packed;

static struct qmi_elem_info pmic_rail_info_req_v01_ei[] = {
	{
		.data_type	= QMI_UNSIGNED_1_BYTE,
		.elem_len	= 4,
		.elem_size	= sizeof(char),
		.array_type	= STATIC_ARRAY,
		.tlv_type	= PING_REQ1_TLV_TYPE,
		.offset		= offsetof(struct pmic_rail_info_req_v01,
					   arr),
	},
	{
		.data_type	= QMI_OPT_FLAG,
		.elem_len	= 1,
		.elem_size	= sizeof(u8),
		.array_type	= NO_ARRAY,
		.tlv_type	= PING_OPT1_TLV_TYPE,
		.offset		= offsetof(struct pmic_rail_info_req_v01,
					   client_name_valid),
	},
	{
		.data_type	= QMI_STRUCT,
		.elem_len	= 1,
		.elem_size	= sizeof(struct test_name_type_v01),
		.array_type	= NO_ARRAY,
		.tlv_type	= PING_OPT1_TLV_TYPE,
		.offset		= offsetof(struct pmic_rail_info_req_v01,
					   client_name),
		.ei_array	= test_name_type_v01_ei,
	},
	{}
};

struct pmic_rail_info_resp_v01 {
	struct qmi_response_type_v01 resp;

	u8 data_valid;
	struct qmi_pmic_railinfo railinfo;
}__packed;

static struct qmi_elem_info pmic_rail_info_resp_v01_ei[] = {
	{
		.data_type	= QMI_STRUCT,
		.elem_len	= 1,
		.elem_size	= sizeof(struct qmi_response_type_v01),
		.array_type	= NO_ARRAY,
		.tlv_type	= DATA_RESP1_TLV_TYPE,
		.offset		= offsetof(struct pmic_rail_info_resp_v01,
					   resp),
		.ei_array	= qmi_response_type_v01_ei,
	},
	{
		.data_type	= QMI_OPT_FLAG,
		.elem_len	= 1,
		.elem_size	= sizeof(u8),
		.array_type	= NO_ARRAY,
		.tlv_type	= DATA_OPT1_TLV_TYPE,
		.offset		= offsetof(struct pmic_rail_info_resp_v01,
					   data_valid),
	},
	{
		.data_type	= QMI_UNSIGNED_1_BYTE,
		.elem_len	= 20,
		.elem_size	= sizeof(u8),
		.array_type	= STATIC_ARRAY,
		.tlv_type	= DATA_OPT1_TLV_TYPE,
		.offset		= offsetof(struct pmic_rail_info_resp_v01,
					   railinfo),
	},
	{}
};

union pmic_rail_volt_set_req_v01 {
	char arr[5];
	struct {
		u8 rail_id;
		u32 rail_voltage_uv; /* value in micro volts */
	}__packed;
}__packed;

static struct qmi_elem_info pmic_rail_volt_set_req_v01_ei[] = {
	{
		.data_type	= QMI_UNSIGNED_1_BYTE,
		.elem_len	= 5,
		.elem_size	= sizeof(char),
		.array_type	= STATIC_ARRAY,
		.tlv_type	= DATA_REQ1_TLV_TYPE,
		.offset		= offsetof(union pmic_rail_volt_set_req_v01,
					   arr),

	},
	{}
};

struct pmic_rail_volt_set_resp_v01 {
	struct qmi_response_type_v01 resp;

	u8 data_valid;
	union {
		char arr[4];
		u32 rail_voltage_uv; /* value in micro volts */
	};

	u8 service_name_valid;
	struct test_name_type_v01 service_name;
}__packed;

static struct qmi_elem_info pmic_rail_volt_set_resp_v01_ei[] = {
	{
		.data_type	= QMI_STRUCT,
		.elem_len	= 1,
		.elem_size	= sizeof(struct qmi_response_type_v01),
		.array_type	= NO_ARRAY,
		.tlv_type	= PING_RESP1_TLV_TYPE,
		.offset		= offsetof(struct pmic_rail_volt_set_resp_v01,
					   resp),
		.ei_array	= qmi_response_type_v01_ei,
	},
	{
		.data_type	= QMI_OPT_FLAG,
		.elem_len	= 1,
		.elem_size	= sizeof(u8),
		.array_type	= NO_ARRAY,
		.tlv_type	= PING_OPT1_TLV_TYPE,
		.offset		= offsetof(struct pmic_rail_volt_set_resp_v01,
					   data_valid),
	},
	{
		.data_type	= QMI_UNSIGNED_1_BYTE,
		.elem_len	= 4,
		.elem_size	= sizeof(char),
		.array_type	= STATIC_ARRAY,
		.tlv_type	= PING_OPT1_TLV_TYPE,
		.offset		= offsetof(struct pmic_rail_volt_set_resp_v01,
					   arr),
	},
	{
		.data_type	= QMI_OPT_FLAG,
		.elem_len	= 1,
		.elem_size	= sizeof(u8),
		.array_type	= NO_ARRAY,
		.tlv_type	= PING_OPT2_TLV_TYPE,
		.offset		= offsetof(struct pmic_rail_volt_set_resp_v01,
					   service_name_valid),
	},
	{
		.data_type	= QMI_STRUCT,
		.elem_len	= 1,
		.elem_size	= sizeof(struct test_name_type_v01),
		.array_type	= NO_ARRAY,
		.tlv_type	= PING_OPT2_TLV_TYPE,
		.offset		= offsetof(struct pmic_rail_volt_set_resp_v01,
					   service_name),
		.ei_array	= test_name_type_v01_ei,
	},
	{}
};

struct test_ping_req_msg_v01 {
	char ping[4];

	u8 client_name_valid;
	struct test_name_type_v01 client_name;
};

static struct qmi_elem_info test_ping_req_msg_v01_ei[] = {
	{
		.data_type	= QMI_UNSIGNED_1_BYTE,
		.elem_len	= 4,
		.elem_size	= sizeof(char),
		.array_type	= STATIC_ARRAY,
		.tlv_type	= PING_REQ1_TLV_TYPE,
		.offset		= offsetof(struct test_ping_req_msg_v01,
					   ping),
	},
	{
		.data_type	= QMI_OPT_FLAG,
		.elem_len	= 1,
		.elem_size	= sizeof(u8),
		.array_type	= NO_ARRAY,
		.tlv_type	= PING_OPT1_TLV_TYPE,
		.offset		= offsetof(struct test_ping_req_msg_v01,
					   client_name_valid),
	},
	{
		.data_type	= QMI_STRUCT,
		.elem_len	= 1,
		.elem_size	= sizeof(struct test_name_type_v01),
		.array_type	= NO_ARRAY,
		.tlv_type	= PING_OPT1_TLV_TYPE,
		.offset		= offsetof(struct test_ping_req_msg_v01,
					   client_name),
		.ei_array	= test_name_type_v01_ei,
	},
	{}
};

struct test_ping_resp_msg_v01 {
	struct qmi_response_type_v01 resp;

	u8 pong_valid;
	char pong[4];

	u8 service_name_valid;
	struct test_name_type_v01 service_name;
};

static struct qmi_elem_info test_ping_resp_msg_v01_ei[] = {
	{
		.data_type	= QMI_STRUCT,
		.elem_len	= 1,
		.elem_size	= sizeof(struct qmi_response_type_v01),
		.array_type	= NO_ARRAY,
		.tlv_type	= PING_RESP1_TLV_TYPE,
		.offset		= offsetof(struct test_ping_resp_msg_v01,
					   resp),
		.ei_array	= qmi_response_type_v01_ei,
	},
	{
		.data_type	= QMI_OPT_FLAG,
		.elem_len	= 1,
		.elem_size	= sizeof(u8),
		.array_type	= NO_ARRAY,
		.tlv_type	= PING_OPT1_TLV_TYPE,
		.offset		= offsetof(struct test_ping_resp_msg_v01,
					   pong_valid),
	},
	{
		.data_type	= QMI_UNSIGNED_1_BYTE,
		.elem_len	= 4,
		.elem_size	= sizeof(char),
		.array_type	= STATIC_ARRAY,
		.tlv_type	= PING_OPT1_TLV_TYPE,
		.offset		= offsetof(struct test_ping_resp_msg_v01,
					   pong),
	},
	{
		.data_type	= QMI_OPT_FLAG,
		.elem_len	= 1,
		.elem_size	= sizeof(u8),
		.array_type	= NO_ARRAY,
		.tlv_type	= PING_OPT2_TLV_TYPE,
		.offset		= offsetof(struct test_ping_resp_msg_v01,
					   service_name_valid),
	},
	{
		.data_type	= QMI_STRUCT,
		.elem_len	= 1,
		.elem_size	= sizeof(struct test_name_type_v01),
		.array_type	= NO_ARRAY,
		.tlv_type	= PING_OPT2_TLV_TYPE,
		.offset		= offsetof(struct test_ping_resp_msg_v01,
					   service_name),
		.ei_array	= test_name_type_v01_ei,
	},
	{}
};

struct test_data_req_msg_v01 {
	u32 data_len;
	u8 data[TEST_MED_DATA_SIZE_V01];

	u8 client_name_valid;
	struct test_name_type_v01 client_name;
};

static struct qmi_elem_info test_data_req_msg_v01_ei[] = {
	{
		.data_type	= QMI_DATA_LEN,
		.elem_len	= 1,
		.elem_size	= sizeof(u32),
		.array_type	= NO_ARRAY,
		.tlv_type	= DATA_REQ1_TLV_TYPE,
		.offset		= offsetof(struct test_data_req_msg_v01,
					   data_len),
	},
	{
		.data_type	= QMI_UNSIGNED_1_BYTE,
		.elem_len	= TEST_MED_DATA_SIZE_V01,
		.elem_size	= sizeof(u8),
		.array_type	= VAR_LEN_ARRAY,
		.tlv_type	= DATA_REQ1_TLV_TYPE,
		.offset		= offsetof(struct test_data_req_msg_v01,
					   data),
	},
	{
		.data_type	= QMI_OPT_FLAG,
		.elem_len	= 1,
		.elem_size	= sizeof(u8),
		.array_type	= NO_ARRAY,
		.tlv_type	= DATA_OPT1_TLV_TYPE,
		.offset		= offsetof(struct test_data_req_msg_v01,
					   client_name_valid),
	},
	{
		.data_type	= QMI_STRUCT,
		.elem_len	= 1,
		.elem_size	= sizeof(struct test_name_type_v01),
		.array_type	= NO_ARRAY,
		.tlv_type	= DATA_OPT1_TLV_TYPE,
		.offset		= offsetof(struct test_data_req_msg_v01,
					   client_name),
		.ei_array	= test_name_type_v01_ei,
	},
	{}
};

struct test_data_resp_msg_v01 {
	struct qmi_response_type_v01 resp;

	u8 data_valid;
	u32 data_len;
	u8 data[TEST_MED_DATA_SIZE_V01];

	u8 service_name_valid;
	struct test_name_type_v01 service_name;
};

static struct qmi_elem_info test_data_resp_msg_v01_ei[] = {
	{
		.data_type	= QMI_STRUCT,
		.elem_len	= 1,
		.elem_size	= sizeof(struct qmi_response_type_v01),
		.array_type	= NO_ARRAY,
		.tlv_type	= DATA_RESP1_TLV_TYPE,
		.offset		= offsetof(struct test_data_resp_msg_v01,
					   resp),
		.ei_array	= qmi_response_type_v01_ei,
	},
	{
		.data_type	= QMI_OPT_FLAG,
		.elem_len	= 1,
		.elem_size	= sizeof(u8),
		.array_type	= NO_ARRAY,
		.tlv_type	= DATA_OPT1_TLV_TYPE,
		.offset		= offsetof(struct test_data_resp_msg_v01,
					   data_valid),
	},
	{
		.data_type	= QMI_DATA_LEN,
		.elem_len	= 1,
		.elem_size	= sizeof(u32),
		.array_type	= NO_ARRAY,
		.tlv_type	= DATA_OPT1_TLV_TYPE,
		.offset		= offsetof(struct test_data_resp_msg_v01,
					   data_len),
	},
	{
		.data_type	= QMI_UNSIGNED_1_BYTE,
		.elem_len	= TEST_MED_DATA_SIZE_V01,
		.elem_size	= sizeof(u8),
		.array_type	= VAR_LEN_ARRAY,
		.tlv_type	= DATA_OPT1_TLV_TYPE,
		.offset		= offsetof(struct test_data_resp_msg_v01,
					   data),
	},
	{
		.data_type	= QMI_OPT_FLAG,
		.elem_len	= 1,
		.elem_size	= sizeof(u8),
		.array_type	= NO_ARRAY,
		.tlv_type	= DATA_OPT2_TLV_TYPE,
		.offset		= offsetof(struct test_data_resp_msg_v01,
					   service_name_valid),
	},
	{
		.data_type	= QMI_STRUCT,
		.elem_len	= 1,
		.elem_size	= sizeof(struct test_name_type_v01),
		.array_type	= NO_ARRAY,
		.tlv_type	= DATA_OPT2_TLV_TYPE,
		.offset		= offsetof(struct test_data_resp_msg_v01,
					   service_name),
		.ei_array	= test_name_type_v01_ei,
	},
	{}
};

/*
 * ping_write() - ping_pong debugfs file write handler
 * @file:	debugfs file context
 * @user_buf:	reference to the user data (ignored)
 * @count:	number of bytes in @user_buf
 * @ppos:	offset in @file to write
 *
 * This function allows user space to send out a ping_pong QMI encoded message
 * to the associated remote test service and will return with the result of the
 * transaction. It serves as an example of how to provide a custom response
 * handler.
 *
 * Return: @count, or negative errno on failure.
 */
static ssize_t ping_write(struct file *file, const char __user *user_buf,
			  size_t count, loff_t *ppos)
{
	struct qmi_handle *qmi = file->private_data;
	struct test_ping_req_msg_v01 req = {};
	struct qmi_txn txn;
	int ret;


	memcpy(req.ping, "ping", sizeof(req.ping));

	ret = qmi_txn_init(qmi, &txn, NULL, NULL);
	if (ret < 0)
		return ret;

	ret = qmi_send_request(qmi, NULL, &txn,
			       TEST_PING_REQ_MSG_ID_V01,
			       TEST_PING_REQ_MAX_MSG_LEN_V01,
			       test_ping_req_msg_v01_ei, &req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		return ret;
	}

	ret = qmi_txn_wait(&txn, 5 * HZ);
	if (ret < 0)
		count = ret;

	return count;
}

static const struct file_operations ping_fops = {
	.open = simple_open,
	.write = ping_write,
};

static void ping_pong_cb(struct qmi_handle *qmi, struct sockaddr_qrtr *sq,
			 struct qmi_txn *txn, const void *data)
{
	const struct test_ping_resp_msg_v01 *resp = data;

	if (!txn) {
		pr_err("spurious ping response\n");
		return;
	}

	if (resp->resp.result == QMI_RESULT_FAILURE_V01)
		txn->result = -ENXIO;
	else if (!resp->pong_valid || memcmp(resp->pong, "pong", 4))
		txn->result = -EINVAL;

	pr_info("Response for ping from %d:%d handle[%p] is %s\n",
			qmi->sq.sq_port, qmi->sq.sq_node, qmi, resp->pong);
	complete(&txn->completion);
}

/*
 * lic_write() - lic debugfs file write handler
 * @file:	debugfs file context
 * @user_buf:	reference to the user data (ignored)
 * @count:	number of bytes in @user_buf
 * @pos:	offset in @file to write
 *
 * This function allows user space to send out OEM ID, Serial number and External
 * part to fuse via QMI encoded message to the associated remote test service and
 * will return with the result of the transaction.
 *
 * Return: @count, or negative errno on failure.
 */
static ssize_t lic_write(struct file *file, const char __user *user_buf,
			  size_t count, loff_t *pos)
{
	struct qmi_handle *qmi = file->private_data;
	struct test_ping_req_msg_v01 req = {};
	struct qmi_txn txn;
	int ret;

	if(!strncmp(file->f_path.dentry->d_iname, "oem_id",
			sizeof(file->f_path.dentry->d_iname))) {
		ret = kstrtou32_from_user(user_buf, count, 0, &oem_id);
		if (ret)
			return ret;

		memcpy(req.ping, &oem_id, sizeof(req.ping));

		ret = qmi_txn_init(qmi, &txn, NULL, NULL);
		if (ret < 0)
			return ret;

		ret = qmi_send_request(qmi, NULL, &txn,
					LIC_GET_OEM_ID_REQ_V01,
					TEST_PING_REQ_MAX_MSG_LEN_V01,
					test_ping_req_msg_v01_ei, &req);
	} else if(!strncmp(file->f_path.dentry->d_iname, "serial_num",
			sizeof(file->f_path.dentry->d_iname))) {
		ret = kstrtou32_from_user(user_buf, count, 0, &serial_num);
		if (ret)
			return ret;

		memcpy(req.ping, &serial_num, sizeof(req.ping));

		ret = qmi_txn_init(qmi, &txn, NULL, NULL);
		if (ret < 0)
			return ret;

		ret = qmi_send_request(qmi, NULL, &txn,
					LIC_GET_SERIAl_NUM_REQ_V01,
					TEST_PING_REQ_MAX_MSG_LEN_V01,
					test_ping_req_msg_v01_ei, &req);
	} else if(!strncmp(file->f_path.dentry->d_iname, "external_part",
			sizeof(file->f_path.dentry->d_iname))) {
		ret = kstrtou32_from_user(user_buf, count, 0, &external_part);
		if (ret)
			return ret;

		memcpy(req.ping, &external_part, sizeof(req.ping));

		ret = qmi_txn_init(qmi, &txn, NULL, NULL);
		if (ret < 0)
			return ret;

		ret = qmi_send_request(qmi, NULL, &txn,
					LIC_GET_EXTERNAL_PART_REQ_V01,
					TEST_PING_REQ_MAX_MSG_LEN_V01,
					test_ping_req_msg_v01_ei, &req);
	} else {
		return -EIO;
	}

	if (ret < 0) {
		qmi_txn_cancel(&txn);
		return ret;
	}

	ret = qmi_txn_wait(&txn, 5 * HZ);
	if (ret < 0)
		count = ret;

	return count;
}

/*
 * lic_read() - lic debugfs file read handler
 * @file:	debugfs file context
 * @user_buf:	reference to the user data (ignored)
 * @count:	number of bytes in @user_buf
 * @pos:	offset in @file to write
 *
 * This function allows user space to read out OEM ID, Serial number, External
 * part and JTAG ID from fuse via QMI encoded message to the associated remote
 * test service and will return with the result of the transaction.
 *
 * Return: zero, or negative errno on failure.
 */

static ssize_t lic_read(struct file *file, char __user *buf,
		size_t count, loff_t *pos)
{
	struct qmi_handle *qmi = file->private_data;
	struct test_ping_req_msg_v01 req = {};
	struct qmi_txn txn;
	int ret;

	if(!strncmp(file->f_path.dentry->d_iname, "oem_id",
			sizeof(file->f_path.dentry->d_iname))) {

		oem_id = 0;
		memcpy(req.ping, &oem_id, sizeof(req.ping));

		ret = qmi_txn_init(qmi, &txn, NULL, NULL);
		if (ret < 0)
			return ret;

		ret = qmi_send_request(qmi, NULL, &txn,
					LIC_GET_OEM_ID_REQ_V01,
					TEST_PING_REQ_MAX_MSG_LEN_V01,
					test_ping_req_msg_v01_ei, &req);
	} else if(!strncmp(file->f_path.dentry->d_iname, "serial_num",
			sizeof(file->f_path.dentry->d_iname))) {

		serial_num = 0;
		memcpy(req.ping, &serial_num, sizeof(req.ping));

		ret = qmi_txn_init(qmi, &txn, NULL, NULL);
		if (ret < 0)
			return ret;

		ret = qmi_send_request(qmi, NULL, &txn,
					LIC_GET_SERIAl_NUM_REQ_V01,
					TEST_PING_REQ_MAX_MSG_LEN_V01,
					test_ping_req_msg_v01_ei, &req);
	} else if(!strncmp(file->f_path.dentry->d_iname, "external_part",
			sizeof(file->f_path.dentry->d_iname))) {

		external_part = 0;
		memcpy(req.ping, &external_part, sizeof(req.ping));

		ret = qmi_txn_init(qmi, &txn, NULL, NULL);
		if (ret < 0)
			return ret;

		ret = qmi_send_request(qmi, NULL, &txn,
					LIC_GET_EXTERNAL_PART_REQ_V01,
					TEST_PING_REQ_MAX_MSG_LEN_V01,
					test_ping_req_msg_v01_ei, &req);
	} else if(!strncmp(file->f_path.dentry->d_iname, "jtag_id",
			sizeof(file->f_path.dentry->d_iname))) {

		jtag_id = 0;
		memcpy(req.ping, &jtag_id, sizeof(req.ping));

		ret = qmi_txn_init(qmi, &txn, NULL, NULL);
		if (ret < 0)
			return ret;

		ret = qmi_send_request(qmi, NULL, &txn,
					LIC_GET_JTAG_ID_REQ_V01,
					TEST_PING_REQ_MAX_MSG_LEN_V01,
					test_ping_req_msg_v01_ei, &req);
	} else {
		return -EIO;
	}

	if (ret < 0) {
		qmi_txn_cancel(&txn);
		return ret;
	}

	ret = qmi_txn_wait(&txn, 5 * HZ);
	if (ret < 0)
		count = ret;

	return 0;
}

static const struct file_operations lic_fops = {
	.open = simple_open,
	.write = lic_write,
	.read = lic_read,
};

static void lic_cb(struct qmi_handle *qmi, struct sockaddr_qrtr *sq,
			 struct qmi_txn *txn, const void *data)
{
	const struct test_ping_resp_msg_v01 *resp = data;
	struct qmi_header *hdr = (struct qmi_header *)qmi->recv_buf;
	u32 resp_val;

	if (!txn) {
		pr_err("spurious OEM ID response\n");
		return;
	}

	if (resp->resp.result == QMI_RESULT_FAILURE_V01)
		txn->result = -ENXIO;

	if (hdr->msg_id == LIC_GET_OEM_ID_REQ_V01) {

		if ((oem_id != 0) && memcmp(resp->pong, &oem_id, sizeof(resp->pong)))
			txn->result = -EINVAL;

		memcpy(&resp_val, resp->pong, sizeof(resp->pong));
		pr_info("Response for OEM ID from %d:%d handle[%p] is 0x%x\n",
				qmi->sq.sq_port, qmi->sq.sq_node, qmi, resp_val);
	} else if (hdr->msg_id == LIC_GET_SERIAl_NUM_REQ_V01) {

		if ((serial_num != 0) && memcmp(resp->pong, &serial_num, sizeof(resp->pong)))
			txn->result = -EINVAL;

		memcpy(&resp_val, resp->pong, sizeof(resp->pong));
		pr_info("Response for serial number from %d:%d handle[%p] is 0x%x\n",
				qmi->sq.sq_port, qmi->sq.sq_node, qmi, resp_val);
	} else if (hdr->msg_id == LIC_GET_EXTERNAL_PART_REQ_V01) {

		if ((external_part != 0) && memcmp(resp->pong, &external_part, sizeof(resp->pong)))
			txn->result = -EINVAL;

		memcpy(&resp_val, resp->pong, sizeof(resp->pong));
		pr_info("Response for external part from %d:%d handle[%p] is 0x%x\n",
				qmi->sq.sq_port, qmi->sq.sq_node, qmi, resp_val);
	} else if (hdr->msg_id == LIC_GET_JTAG_ID_REQ_V01) {

		if ((jtag_id != 0) && memcmp(resp->pong, &jtag_id, sizeof(resp->pong)))
			txn->result = -EINVAL;

		memcpy(&resp_val, resp->pong, sizeof(resp->pong));
		pr_info("Response for JTAG ID from %d:%d handle[%p] is 0x%x\n",
				qmi->sq.sq_port, qmi->sq.sq_node, qmi, resp_val);
	}

	complete(&txn->completion);
}

/*
 * data_write() - data debugfs file write handler
 * @file:	debugfs file context
 * @user_buf:	reference to the user data
 * @count:	number of bytes in @user_buf
 * @ppos:	offset in @file to write
 *
 * This function allows user space to send out a data QMI encoded message to
 * the associated remote test service and will return with the result of the
 * transaction. It serves as an example of how to have the QMI helpers decode a
 * transaction response into a provided object automatically.
 *
 * Return: @count, or negative errno on failure.
 */
static ssize_t data_write(struct file *file, const char __user *user_buf,
			  size_t count, loff_t *ppos)

{
	struct qmi_handle *qmi = file->private_data;
	struct test_data_resp_msg_v01 *resp;
	struct test_data_req_msg_v01 *req;
	struct qmi_txn txn;
	int ret;

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->data_len = min_t(size_t, sizeof(req->data), count);
	if (copy_from_user(req->data, user_buf, req->data_len)) {
		ret = -EFAULT;
		goto out;
	}

	ret = qmi_txn_init(qmi, &txn, test_data_resp_msg_v01_ei, resp);
	if (ret < 0)
		goto out;

	ret = qmi_send_request(qmi, NULL, &txn,
			       TEST_DATA_REQ_MSG_ID_V01,
			       TEST_DATA_REQ_MAX_MSG_LEN_V01,
			       test_data_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		goto out;
	}

	ret = qmi_txn_wait(&txn, 5 * HZ);
	if (ret < 0) {
		goto out;
	} else if (!resp->data_valid ||
		   resp->data_len != req->data_len ||
		   memcmp(resp->data, req->data, req->data_len)) {
		pr_err("response data doesn't match expectation\n");
		ret = -EINVAL;
		goto out;
	}

	pr_info("Response for data from %d:%d handle[%p] is %s\n",
			qmi->sq.sq_port, qmi->sq.sq_node, qmi, resp->data);
	ret = count;

out:
	kfree(resp);
	kfree(req);

	return ret;
}

static const struct file_operations data_fops = {
	.open = simple_open,
	.write = data_write,
};

static ssize_t pmic_write(struct file *fp, const char __user *user_buf,
			  size_t count, loff_t *ppos)

{
	struct qmi_handle *qmi = fp->private_data;
	struct qmi_txn txn;
	int ret;

	if (count < 1)
		return 0;

	if (!strncmp(fp->f_path.dentry->d_iname, "rail_id",
		     sizeof(fp->f_path.dentry->d_iname))) {
		struct pmic_rail_info_req_v01 req = {};

		ret = kstrtou8_from_user(user_buf, count, 0, &rail_id);
		if (ret)
			return ret;

		req.rail_id = rail_id;

		ret = qmi_txn_init(qmi, &txn, NULL, NULL);
		if (ret < 0)
			return ret;

		ret = qmi_send_request(qmi, NULL,
				       &txn, TEST_PMIC_GET_RAIL_INFO,
				       TEST_SML_DATA_SIZE_V01,
				       pmic_rail_info_req_v01_ei, &req);

	} else if (!strncmp(fp->f_path.dentry->d_iname, "rail_voltage_uv",
		   sizeof(fp->f_path.dentry->d_iname))) {
		union pmic_rail_volt_set_req_v01 req = {};

		ret = kstrtouint_from_user(user_buf, count, 0,
					   &rail_voltage_uv);
		if (ret)
			return ret;

		req.rail_id = rail_id;
		req.rail_voltage_uv = rail_voltage_uv;

		ret = qmi_txn_init(qmi, &txn, NULL, NULL);
		if (ret < 0)
			return ret;

		ret = qmi_send_request(qmi, NULL, &txn,
				       TEST_PMIC_SET_RAIL_VOLT,
				       TEST_SML_DATA_SIZE_V01,
				       pmic_rail_volt_set_req_v01_ei, &req);
	} else {
		return -EIO;
	}

	if (ret < 0) {
		qmi_txn_cancel(&txn);
		return ret;
	}

	ret = qmi_txn_wait(&txn, 5 * HZ);
	if (ret < 0)
		count = ret;

	return count;
}

static ssize_t pmic_read(struct file *fp, char __user *buf,
		size_t count, loff_t *pos)
{
	char _buf[16] = {0};

	if (!strncmp(fp->f_path.dentry->d_iname, "rail_id",
		     sizeof(fp->f_path.dentry->d_iname))) {
		snprintf(_buf, sizeof(_buf), "%u\n", rail_id);
	} else if (!strncmp(fp->f_path.dentry->d_iname, "rail_voltage_uv",
			    sizeof(fp->f_path.dentry->d_iname))) {
		snprintf(_buf, sizeof(_buf), "%u\n", rail_voltage_uv);
	}

	return simple_read_from_buffer(buf, count, pos, _buf, strnlen(_buf, 16));
}

static const struct file_operations pmic_fops = {
	.open = simple_open,
	.write = pmic_write,
	.read = pmic_read,
};

static void rail_info_cb(struct qmi_handle *qmi, struct sockaddr_qrtr *sq,
			 struct qmi_txn *txn, const void *data)
{
	const struct pmic_rail_info_resp_v01 *resp = data;

	if (!txn) {
		pr_err("spurious rail info response\n");
		return;
	}

	if (resp->resp.result == QMI_RESULT_FAILURE_V01)
		txn->result = -ENXIO;
	else if (!resp->data_valid)
		txn->result = -EINVAL;

	pr_info("Rail info requested from %d:%d handle[%p]\n",
		qmi->sq.sq_port, qmi->sq.sq_node, qmi);
	pr_info("==========================================================\n");
	pr_info("Rail ID   CPR Mode  Rail Voltage(uV)   Name \n");
	pr_info("%02d        %02d        %08d           %s\n",
		resp->railinfo.rail_id,  resp->railinfo.cpr_mode,
		resp->railinfo.voltage_uv, resp->railinfo.name);
	pr_info("==========================================================\n");

	complete(&txn->completion);
}

static void volt_set_cb(struct qmi_handle *qmi, struct sockaddr_qrtr *sq,
			 struct qmi_txn *txn, const void *data)
{
	const struct pmic_rail_volt_set_resp_v01 *resp = data;

	if (!txn) {
		pr_err("spurious voltage set response\n");
		return;
	}

	if (resp->resp.result == QMI_RESULT_FAILURE_V01)
		txn->result = -ENXIO;

	pr_info("Voltage set requested from %d:%d handle[%p] is successful!\n",
		qmi->sq.sq_port, qmi->sq.sq_node, qmi);

	complete(&txn->completion);
}

static struct qmi_msg_handler qmi_sample_handlers[] = {
	{
		.type = QMI_RESPONSE,
		.msg_id = TEST_PING_REQ_MSG_ID_V01,
		.ei = test_ping_resp_msg_v01_ei,
		.decoded_size = sizeof(struct test_ping_req_msg_v01),
		.fn = ping_pong_cb
	},
	{
		.type = QMI_RESPONSE,
		.msg_id = TEST_PMIC_GET_RAIL_INFO,
		.ei = pmic_rail_info_resp_v01_ei,
		.decoded_size = sizeof(struct pmic_rail_info_resp_v01),
		.fn = rail_info_cb
	},
	{
		.type = QMI_RESPONSE,
		.msg_id = TEST_PMIC_SET_RAIL_VOLT,
		.ei = pmic_rail_volt_set_resp_v01_ei,
		.decoded_size = sizeof(struct pmic_rail_volt_set_resp_v01),
		.fn = volt_set_cb
	},
	{
		.type = QMI_RESPONSE,
		.msg_id = LIC_GET_OEM_ID_REQ_V01,
		.ei = test_ping_resp_msg_v01_ei,
		.decoded_size = sizeof(struct test_ping_req_msg_v01),
		.fn = lic_cb
	},
	{
		.type = QMI_RESPONSE,
		.msg_id = LIC_GET_SERIAl_NUM_REQ_V01,
		.ei = test_ping_resp_msg_v01_ei,
		.decoded_size = sizeof(struct test_ping_req_msg_v01),
		.fn = lic_cb
	},
	{
		.type = QMI_RESPONSE,
		.msg_id = LIC_GET_EXTERNAL_PART_REQ_V01,
		.ei = test_ping_resp_msg_v01_ei,
		.decoded_size = sizeof(struct test_ping_req_msg_v01),
		.fn = lic_cb
	},
	{
		.type = QMI_RESPONSE,
		.msg_id = LIC_GET_JTAG_ID_REQ_V01,
		.ei = test_ping_resp_msg_v01_ei,
		.decoded_size = sizeof(struct test_ping_req_msg_v01),
		.fn = lic_cb
	},
	{}
};

struct qmi_sample {
	struct qmi_handle qmi;

	struct dentry *de_dir;
	struct dentry *de_data;
	struct dentry *de_ping;
	struct dentry *de_test;
	struct dentry *de_nthreads;
	struct dentry *de_niterations;
	struct dentry *de_data_size;
	struct dentry *de_oem_id;
	struct dentry *de_serial_num;
	struct dentry *de_external_part;
	struct dentry *de_jtag_id;
	struct dentry *de_pmic_dir;
	struct dentry *de_rail_id;
	struct dentry *de_rail_voltage_uv;
};

static struct dentry *qmi_debug_dir;

static void update_status(struct qmi_handle *qmi)
{
	unsigned int max = nthreads * niterations;
	unsigned int count = atomic_read(&qmi->cnt);
	unsigned int percent;
	static unsigned int pre_percent;

	percent = (count * 100)/max;

	if (percent > pre_percent)
		pr_info("Client %s Completed(%d%%)...\n",
				(current->comm + 4), percent);

	pre_percent = percent;
}

static int test_qmi_ping_pong_send_msg(struct qmi_handle *qmi)
{
	struct test_ping_req_msg_v01 req = {};
	struct qmi_txn txn;
	int ret;

	atomic_inc(&qmi->cnt);

	memcpy(req.ping, "ping", sizeof(req.ping));

	ret = qmi_txn_init(qmi, &txn, NULL, NULL);
	if (ret < 0)
		return ret;

	ret = qmi_send_request(qmi, NULL, &txn,
			TEST_PING_REQ_MSG_ID_V01,
			TEST_PING_REQ_MAX_MSG_LEN_V01,
			test_ping_req_msg_v01_ei, &req);
	if (ret < 0) {
		atomic_inc(&qmi->fail);
		qmi_txn_cancel(&txn);
		return ret;
	}

	ret = qmi_txn_wait(&txn, 5 * HZ);
	if (ret < 0) {
		pr_err("Failed to get response on the txn\n");
		atomic_inc(&qmi->fail);
		return ret;
	}

	atomic_inc(&qmi->pass);
	mutex_lock(&status_print_lock);
	update_status(qmi);
	mutex_unlock(&status_print_lock);
	return ret;

}

static int test_qmi_data_send_msg(struct qmi_handle *qmi, unsigned int data_len)
{
	struct test_data_resp_msg_v01 *resp;
	struct test_data_req_msg_v01 *req;
	struct qmi_txn txn;
	int ret, i;

	atomic_inc(&qmi->cnt);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req) {
		atomic_inc(&qmi->fail);
		return -ENOMEM;
	}

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		atomic_inc(&qmi->fail);
		return -ENOMEM;
	}

	req->data_len = data_len;
	for (i = 0; i < req->data_len; i = i + sizeof(int))
		memcpy(req->data + i, (uint8_t *)&i, sizeof(int));
	req->client_name_valid = 0;

	ret = qmi_txn_init(qmi, &txn, test_data_resp_msg_v01_ei, resp);
	if (ret < 0) {
		atomic_inc(&qmi->fail);
		goto out;
	}

	ret = qmi_send_request(qmi, NULL, &txn,
			TEST_DATA_REQ_MSG_ID_V01,
			TEST_DATA_REQ_MAX_MSG_LEN_V01,
			test_data_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		atomic_inc(&qmi->fail);
		goto out;
	}

	ret = qmi_txn_wait(&txn, 5 * HZ);

	if (ret < 0) {
		atomic_inc(&qmi->fail);
		goto out;
	}

	if (!resp->data_valid ||
			resp->data_len != req->data_len ||
			memcmp(resp->data, req->data, req->data_len)) {
		pr_err("response data doesn't match expectation\n");
		atomic_inc(&qmi->fail);
		ret = -EINVAL;
		goto out;
	} else {
		pr_debug("Data valid\n");
		atomic_inc(&qmi->pass);
	}

	mutex_lock(&status_print_lock);
	update_status(qmi);
	mutex_unlock(&status_print_lock);

out:
	kfree(resp);
	kfree(req);

	return ret;
}

int qmi_process_user_input(void *data)
{
	struct test_qmi_data *qmi_data, *temp_qmi_data;
	unsigned short index = 0;
	struct qmi_handle *qmi;
	struct list_head *data_list = data;

	qmi = container_of(data, struct qmi_handle, data_list);

	wait_for_completion_timeout(&qmi->complete, msecs_to_jiffies(1000));

	list_for_each_entry_safe(qmi_data, temp_qmi_data, data_list, list) {
		atomic_inc(&qmi_data->refs_count);

		if (!strncmp(qmi_data->data, "ping_pong", sizeof(qmi_data->data))) {
			for (index = 0; index < niterations; index++) {
				test_res = test_qmi_ping_pong_send_msg(qmi);
			}
		} else if (!strncmp(qmi_data->data, "data", sizeof(qmi_data->data))) {
			for (index = 0; index < niterations; index++) {
				test_res = test_qmi_data_send_msg(qmi, data_size);
			}
		} else {
			test_res = 0;
			pr_err("Invalid Test.\n");
			list_del(&qmi_data->list);
			break;
		}

		if (atomic_dec_and_test(&qmi_data->refs_count)) {
			pr_info("Test Completed. Pass: %d Fail: %d\n",
					atomic_read(&qmi->pass), atomic_read(&qmi->fail));
			list_del(&qmi_data->list);
			kfree(qmi_data);
			qmi_data = NULL;
		}
	}
	return 0;
}

static int test_qmi_open(struct inode *ip, struct file *fp)
{
	char thread_name[32];
	struct task_struct *qmi_task;
	int index = 0;
	struct qmi_handle *qmi;

	for (index = 1; index < sizeof(qdentry) / sizeof(struct qmi_dir); index++) {
		if (!strncmp(fp->f_path.dentry->d_iname, qdentry[index].string,
					sizeof(fp->f_path.dentry->d_iname)))
			return 0;
	}

	if (!ip->i_private)
		return -ENODATA;

	fp->private_data = ip->i_private;
	qmi = fp->private_data;

	pr_info("Total commands: %lu (Threads: %lu Iteration: %lu)\n",
			nthreads * niterations, nthreads, niterations);

	atomic_set(&qmi->cnt, 0);
	atomic_set(&qmi->pass, 0);
	atomic_set(&qmi->fail, 0);
	atomic_set(&qmi->async_cnt, 0);
	atomic_set(&qmi->async_req, 0);
	atomic_set(&qmi->async_rsp, 0);

	init_completion(&qmi->complete);
	for (index = 0; index < nthreads; index++) {
		snprintf(thread_name, sizeof(thread_name), "qmi_""%s_%d",
				fp->f_path.dentry->d_parent->d_iname, index);
		qmi_task = kthread_run(qmi_process_user_input,
					&qmi->data_list, thread_name);
	}
	return 0;

}

static ssize_t test_qmi_read(struct file *fp, char __user *buf,
		size_t count, loff_t *pos)
{
	char _buf[16] = {0};
	int index = 0;

	for (index = 0; index < sizeof(qdentry)/sizeof(struct qmi_dir); index++) {
		if (!strncmp(fp->f_path.dentry->d_iname, qdentry[index].string, \
					sizeof(fp->f_path.dentry->d_iname)))
			snprintf(_buf, sizeof(_buf), "%lu\n", *qdentry[index].value);
	}

	return simple_read_from_buffer(buf, count, pos, _buf, strnlen(_buf, 16));

}

static int test_qmi_release(struct inode *ip, struct file *fp)
{
	return 0;
}

static ssize_t test_qmi_write(struct file *fp, const char __user *buf,
		size_t count, loff_t *pos)
{
	unsigned char cmd[64];
	int len;
	int index = 0;
	struct test_qmi_data *qmi_data;
	struct qmi_handle *qmi;

	if (count < 1)
		return 0;

	len = min(count, (sizeof(cmd) - 1));

	if (copy_from_user(cmd, buf, len))
		return -EFAULT;

	cmd[len] = 0;
	if (cmd[len-1] == '\n') {
		cmd[len-1] = 0;
		len--;
	}

	for (index = 1; index < sizeof(qdentry)/sizeof(struct qmi_dir); index++) {
		if (!strncmp(fp->f_path.dentry->d_iname, qdentry[index].string, \
					sizeof(fp->f_path.dentry->d_iname))) {
			kstrtoul(cmd, 0, qdentry[index].value);
			return count;
		}
	}

	qmi_data = kmalloc(sizeof(struct test_qmi_data), GFP_KERNEL);
	if (!qmi_data) {
		pr_err("Unable to allocate memory for qmi_data\n");
		return -ENOMEM;
	}

	memcpy(qmi_data->data, cmd, sizeof(cmd));

	atomic_set(&qmi_data->refs_count, 0);

	qmi = fp->private_data;
	list_add_tail(&qmi_data->list, &qmi->data_list);
	complete_all(&qmi->complete);

	return count;
}

static const struct file_operations debug_ops = {
	.open = test_qmi_open,
	.read = test_qmi_read,
	.write = test_qmi_write,
	.release = test_qmi_release,
};

static int qmi_sample_probe(struct platform_device *pdev)
{
	struct sockaddr_qrtr *sq;
	struct qmi_sample *sample;
	char path[20];
	int ret;

	sample = devm_kzalloc(&pdev->dev, sizeof(*sample), GFP_KERNEL);
	if (!sample)
		return -ENOMEM;

	ret = qmi_handle_init(&sample->qmi, TEST_DATA_REQ_MAX_MSG_LEN_V01,
			      NULL,
			      qmi_sample_handlers);
	if (ret < 0)
		return ret;

	sq = dev_get_platdata(&pdev->dev);
	ret = kernel_connect(sample->qmi.sock, (struct sockaddr *)sq,
			     sizeof(*sq), 0);
	if (ret < 0) {
		pr_err("failed to connect to remote service port\n");
		goto err_release_qmi_handle;
	}

	snprintf(path, sizeof(path), "%d:%d", sq->sq_node, sq->sq_port);

	INIT_LIST_HEAD(&sample->qmi.data_list);
	sample->de_dir = debugfs_create_dir(path, qmi_debug_dir);
	if (IS_ERR(sample->de_dir)) {
		ret = PTR_ERR(sample->de_dir);
		goto err_release_qmi_handle;
	}

	sample->de_data = debugfs_create_file("data", 0600, sample->de_dir,
					      sample, &data_fops);
	if (IS_ERR(sample->de_data)) {
		ret = PTR_ERR(sample->de_data);
		goto err_remove_de_dir;
	}

	sample->de_ping = debugfs_create_file("ping", 0600, sample->de_dir,
					      sample, &ping_fops);
	if (IS_ERR(sample->de_ping)) {
		ret = PTR_ERR(sample->de_ping);
		goto err_remove_de_data;
	}

	sample->de_test = debugfs_create_file("test", 0600, sample->de_dir,
					      sample, &debug_ops);

	if (IS_ERR(sample->de_test)) {
		pr_err("Failed to create debugfs entry for test\n");
		goto err_remove_de_data;
	}

	sample->de_nthreads = debugfs_create_file("nthreads", 0600,
						  sample->de_dir, NULL,
						  &debug_ops);
	if (IS_ERR(sample->de_nthreads)) {
		pr_err("Failed to create debugfs entry for nthreads\n");
		goto err_remove_de_test;
	}

	sample->de_niterations = debugfs_create_file("niterations", 0600,
						     sample->de_dir, NULL,
						     &debug_ops);

	if (IS_ERR(sample->de_niterations)) {
		pr_err("Failed to create debugfs entry for niterations\n");
		goto err_remove_de_nthreads;
	}

	sample->de_data_size = debugfs_create_file("data_size", 0600,
						   sample->de_dir, NULL,
						   &debug_ops);
	if (IS_ERR(sample->de_data_size)) {
		pr_err("Failed to create debugfs entry for data size\n");
		goto err_remove_de_niterations;
	}

	sample->de_oem_id = debugfs_create_file("oem_id", 0600,
						   sample->de_dir, sample,
						   &lic_fops);
	if (IS_ERR(sample->de_oem_id)) {
		pr_err("Failed to create debugfs entry for OEM ID\n");
		goto err_remove_de_oem_id;
	}

	sample->de_serial_num = debugfs_create_file("serial_num", 0600,
						   sample->de_dir, sample,
						   &lic_fops);
	if (IS_ERR(sample->de_serial_num)) {
		pr_err("Failed to create debugfs entry for Serial Number\n");
		goto err_remove_de_serial_num;
	}

	sample->de_external_part = debugfs_create_file("external_part", 0600,
						   sample->de_dir, sample,
						   &lic_fops);
	if (IS_ERR(sample->de_external_part)) {
		pr_err("Failed to create debugfs entry for external part\n");
		goto err_remove_de_external_part;
	}

	sample->de_jtag_id = debugfs_create_file("jtag_id", 0400,
						   sample->de_dir, sample,
						   &lic_fops);
	if (IS_ERR(sample->de_jtag_id)) {
		pr_err("Failed to create debugfs entry for JTAG ID\n");
		goto err_remove_de_jtag_id;
	}

	sample->de_pmic_dir = debugfs_create_dir("pmic", sample->de_dir);
	if (IS_ERR(sample->de_pmic_dir)) {
		ret = PTR_ERR(sample->de_pmic_dir);
		goto err_remove_de_data_size;
	}

	sample->de_rail_id = debugfs_create_file("rail_id", 0600,
						      sample->de_pmic_dir,
						      sample, &pmic_fops);
	if (IS_ERR(sample->de_rail_id)) {
		ret = PTR_ERR(sample->de_rail_id);
		goto err_remove_de_pmic_dir;
	}

	sample->de_rail_voltage_uv = debugfs_create_file("rail_voltage_uv",
							   0600,
							   sample->de_pmic_dir,
							   sample, &pmic_fops);
	if (IS_ERR(sample->de_rail_voltage_uv)) {
		ret = PTR_ERR(sample->de_rail_voltage_uv);
		goto err_remove_de_rail_id;
	}

	platform_set_drvdata(pdev, sample);

	return 0;

err_remove_de_rail_id:
	debugfs_remove(sample->de_rail_id);
err_remove_de_pmic_dir:
	debugfs_remove(sample->de_pmic_dir);
err_remove_de_data_size:
	debugfs_remove(sample->de_data_size);
err_remove_de_jtag_id:
	debugfs_remove(sample->de_jtag_id);
err_remove_de_external_part:
	debugfs_remove(sample->de_external_part);
err_remove_de_serial_num:
	debugfs_remove(sample->de_serial_num);
err_remove_de_oem_id:
	debugfs_remove(sample->de_oem_id);
err_remove_de_niterations:
	debugfs_remove(sample->de_niterations);
err_remove_de_nthreads:
	debugfs_remove(sample->de_nthreads);
err_remove_de_test:
	debugfs_remove(sample->de_test);
err_remove_de_data:
	debugfs_remove(sample->de_data);
err_remove_de_dir:
	debugfs_remove(sample->de_dir);
err_release_qmi_handle:
	qmi_handle_release(&sample->qmi);

	return ret;
}

static int qmi_sample_remove(struct platform_device *pdev)
{
	struct qmi_sample *sample = platform_get_drvdata(pdev);
	struct qmi_handle *qmi = &sample->qmi;
	struct test_qmi_data *qmi_data, *temp_qmi_data;

	debugfs_remove(sample->de_rail_voltage_uv);
	debugfs_remove(sample->de_rail_id);
	debugfs_remove(sample->de_pmic_dir);
	debugfs_remove(sample->de_jtag_id);
	debugfs_remove(sample->de_external_part);
	debugfs_remove(sample->de_serial_num);
	debugfs_remove(sample->de_oem_id);
	debugfs_remove(sample->de_data_size);
	debugfs_remove(sample->de_niterations);
	debugfs_remove(sample->de_nthreads);
	debugfs_remove(sample->de_test);
	debugfs_remove(sample->de_ping);
	debugfs_remove(sample->de_data);
	debugfs_remove(sample->de_dir);

	qmi_handle_release(&sample->qmi);

	list_for_each_entry_safe(qmi_data, temp_qmi_data, &qmi->data_list, list) {
		list_del(&qmi_data->list);
		kfree(qmi_data);
	}
	list_del(&qmi->data_list);

	return 0;
}

static struct platform_driver qmi_sample_driver = {
	.probe = qmi_sample_probe,
	.remove = qmi_sample_remove,
	.driver = {
		.name = "qmi_sample_client",
	},
};

static int qmi_sample_new_server(struct qmi_handle *qmi,
				 struct qmi_service *service)
{
	struct platform_device *pdev;
	struct sockaddr_qrtr sq = { AF_QIPCRTR, service->node, service->port };
	int ret;

	pdev = platform_device_alloc("qmi_sample_client", PLATFORM_DEVID_AUTO);
	if (!pdev)
		return -ENOMEM;

	ret = platform_device_add_data(pdev, &sq, sizeof(sq));
	if (ret)
		goto err_put_device;

	ret = platform_device_add(pdev);
	if (ret)
		goto err_put_device;

	service->priv = pdev;

	return 0;

err_put_device:
	platform_device_put(pdev);

	return ret;
}

static void qmi_sample_del_server(struct qmi_handle *qmi,
				  struct qmi_service *service)
{

	struct platform_device *pdev = service->priv;

	platform_device_unregister(pdev);
}

static struct qmi_handle lookup_client;

static struct qmi_ops lookup_ops = {
	.new_server = qmi_sample_new_server,
	.del_server = qmi_sample_del_server,
};

static int qmi_sample_init(void)
{
	int ret;

	pr_info("%s \n", __func__);
	qmi_debug_dir = debugfs_create_dir("qmi_sample", NULL);
	if (IS_ERR(qmi_debug_dir)) {
		pr_err("failed to create qmi_sample dir\n");
		return PTR_ERR(qmi_debug_dir);
	}

	ret = platform_driver_register(&qmi_sample_driver);
	if (ret)
		goto err_remove_debug_dir;

	ret = qmi_handle_init(&lookup_client, 0, &lookup_ops, NULL);
	if (ret < 0)
		goto err_unregister_driver;

	qmi_add_lookup(&lookup_client, 15, 0, 0);

	mutex_init(&status_print_lock);

	return 0;

err_unregister_driver:
	platform_driver_unregister(&qmi_sample_driver);
err_remove_debug_dir:
	debugfs_remove(qmi_debug_dir);

	return ret;
}

static void qmi_sample_exit(void)
{
	qmi_handle_release(&lookup_client);

	platform_driver_unregister(&qmi_sample_driver);

	debugfs_remove(qmi_debug_dir);
}

module_init(qmi_sample_init);
module_exit(qmi_sample_exit);

MODULE_DESCRIPTION("Sample QMI client driver");
MODULE_LICENSE("GPL v2");
