// SPDX-License-Identifier: GPL-2.0
/*
 * Synopsys DesignWare PCIe controller debugfs driver
 *
 * Copyright (C) 2021 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Shradha Todi <shradha.t@samsung.com>
 */

#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include "pcie-designware.h"
#include "pcie-designware-debugfs.h"

#define DEBUGFS_BUF_MAX_SIZE	100

static char debugfs_buf[DEBUGFS_BUF_MAX_SIZE];

static struct dentry *dir;
static struct dentry *sub_dir;

static struct event_counters ras_events[] = {
	{"ebuf_overflow",		ebuf_overflow},
	{"ebuf_underrun",		ebuf_underrun},
	{"decode_error",		decode_error},
	{"sync_header_error",		sync_header_error},
	{"receiver_error",		receiver_error},
	{"framing_error",		framing_error},
	{"lcrc_error",			lcrc_error},
	{"ecrc_error",			ecrc_error},
	{"unsupp_req_error",		unsupp_req_error},
	{"cmpltr_abort_error",		cmpltr_abort_error},
	{"cmpltn_timeout_error",	cmpltn_timeout_error},
	{"tx_l0s_entry",		tx_l0s_entry},
	{"rx_l0s_entry",		rx_l0s_entry},
	{"l1_entry",			l1_entry},
	{"l1_1_entry",			l1_1_entry},
	{"l1_2_entry",			l1_2_entry},
	{"l2_entry",			l2_entry},
	{"speed_change",		speed_change},
	{"width_chage",			width_chage},
	{0, 0},
};

static struct error_injections error_list[] = {
	{"tx_lcrc",		tx_lcrc},
	{"tx_ecrc",		tx_ecrc},
	{"rx_lcrc",		rx_lcrc},
	{"rx_ecrc",		rx_ecrc},
	{0, 0},
};

static struct time_report time_based_list[] = {
	{"core_1_cycle",	core_1_cycle,		core_multiplier},
	{"core_tx_l0s",		core_tx_l0s,		core_multiplier},
	{"core_rx_l0s",		core_rx_l0s,		core_multiplier},
	{"core_l0",		core_l0,		core_multiplier},
	{"core_l1",		core_l1,		core_multiplier},
	{"core_cfg_rvcry",	core_cfg_rvcry,		core_multiplier},
	{"core_tx_rx_l0s",	core_tx_rx_l0s,		core_multiplier},
	{"aux_l1_1",		aux_l1_1,		aux_multiplier},
	{"aux_l1_2",		aux_l1_2,		aux_multiplier},
	{"aux_l1",		aux_l1,			aux_multiplier},
	{"ccix_1_cycle",	ccix_1_cycle,		ccix_multiplier},
	{"ccix_tx_l0s",		ccix_tx_l0s,		ccix_multiplier},
	{"ccix_rx_l0s",		ccix_rx_l0s,		ccix_multiplier},
	{"ccix_l0",		ccix_l0,		ccix_multiplier},
	{"ccix_l1",		ccix_l1,		ccix_multiplier},
	{"ccix_cfg_rvcry",	ccix_cfg_rvcry,		ccix_multiplier},
	{"ccix_tx_rx_l0s",	ccix_tx_rx_l0s,		data_multiplier},
	{"tx_pcie_tlp_data",	tx_pcie_tlp_data,	data_multiplier},
	{"rx_pcie_tlp_data",	rx_pcie_tlp_data,	data_multiplier},
	{"tx_ccix_tlp_data",	tx_ccix_tlp_data,	data_multiplier},
	{"rx_ccix_tlp_data",	rx_ccix_tlp_data,	data_multiplier},
	{0 ,0, 0},
};

static int open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

/*
 * set_time_event_number: Function to set event number based on filename
 *
 * This function is called from the common read and write function
 * written for all event counters. Using the debugfs filname, the
 * group number and event number for the counter is extracted and
 * then programmed into the control register.
 *
 * @file: file pointer to the debugfs entry
 *
 * Return: void
 */
static void set_time_event_number(struct file *file)
{
	int i;
	u32 val;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;
	u32 max_size = sizeof(time_based_list) / sizeof(struct time_report);

	for (i = 0; i < max_size; i++) {
		if (strcmp(time_based_list[i].name,
					file->f_path.dentry->d_parent->d_iname) == 0) {
			val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
					RAS_DES_TIME_BASE_CTRL_REG);
			val = time_based_list[i].tevent_num << TIME_BASED_REPORT_SHIFT;
			dw_pcie_writel_dbi(pci, pci->ras_cap_offset +
					RAS_DES_TIME_BASE_CTRL_REG, val);
			break;
		}
	}
}

/*
 * set_event_number: Function to set event number based on filename
 *
 * This function is called from the common read and write function
 * written for all event counters. Using the debugfs filname, the
 * group number and event number for the counter is extracted and
 * then programmed into the control register.
 *
 * @file: file pointer to the debugfs entry
 *
 * Return: void
 */
static void set_event_number(struct file *file)
{
	int i;
	u32 val;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;
	u32 max_size = sizeof(ras_events) / sizeof(struct event_counters);

	for (i = 0; i < max_size; i++) {
		if (strcmp(ras_events[i].name,
			   file->f_path.dentry->d_parent->d_iname) == 0) {
			val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
					RAS_DES_EVENT_COUNTER_CTRL_REG);
			val &= ~(EVENT_COUNTER_ENABLE);
			val &= ~(0xFFF << 16);
			val |= (ras_events[i].event_num << 16);
			dw_pcie_writel_dbi(pci, pci->ras_cap_offset +
					RAS_DES_EVENT_COUNTER_CTRL_REG, val);
			break;
		}
	}
}

/*
 * get_error_inj_number: Function to get error number based on filename
 *
 * This function is called from the common read and write function
 * written for all error injection debugfs entries. Using the debugfs
 * filname, the error group and type of error to be injected is extracted.
 *
 * @file: file pointer to the debugfs entry
 *
 * Return: u32
 * [31:8]: Type of error to be injected
 * [7:0]: Group of error it belongs to
 */
static u32 get_error_inj_number(struct file *file)
{
	int i;
	u32 max_size = sizeof(error_list) / sizeof(struct error_injections);

	for (i = 0; i < max_size; i++) {
		if (strcmp(error_list[i].name,
			   file->f_path.dentry->d_iname) == 0) {
			return error_list[i].type_of_err;
		}
	}

	return 0;
}

/*
 * ras_event_counter_en_read: Function to get if counter is enable
 *
 * This function is invoked when the following command is made:
 * cat /sys/kernel/debug/dwc_pcie_plat/ras_des_counter/<name>/counter_enable
 * It returns whether the counter is enabled or not
 */
static ssize_t ras_event_counter_en_read(struct file *file, char __user *buf,
					 size_t count, loff_t *ppos)
{
	u32 ret;
	u32 val;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;

	set_event_number(file);

	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
				RAS_DES_EVENT_COUNTER_CTRL_REG);
	val = (val >> EVENT_COUNTER_STATUS_SHIFT) & EVENT_COUNTER_STATUS_MASK;
	if (val)
		scnprintf(debugfs_buf, sizeof(debugfs_buf), "Enabled\n");
	else
		scnprintf(debugfs_buf, sizeof(debugfs_buf), "Disabled\n");

	ret = simple_read_from_buffer(buf, count, ppos, debugfs_buf,
				      strlen(debugfs_buf));

	return ret;
}

/*
 * ras_event_counter_lane_sel_read: Function to get lane number selected
 *
 * This function is invoked when the following command is made:
 * cat /sys/kernel/debug/dwc_pcie_plat/ras_des_counter/<name>/lane_select
 * It returns for which lane the counter configurations are done
 */
static ssize_t ras_event_counter_lane_sel_read(struct file *file,
					       char __user *buf, size_t count,
					       loff_t *ppos)
{
	u32 ret;
	u32 val;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;

	set_event_number(file);

	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
			RAS_DES_EVENT_COUNTER_CTRL_REG);
	val = (val >> LANE_SELECT_SHIFT) & LANE_SELECT_MASK;
	scnprintf(debugfs_buf, sizeof(debugfs_buf), "0x%x\n", val);
	ret = simple_read_from_buffer(buf, count, ppos, debugfs_buf,
				      strlen(debugfs_buf));

	return ret;
}

/*
 * ras_event_counter_value_read: Function to get counter value
 *
 * This function is invoked when the following command is made:
 * cat /sys/kernel/debug/dwc_pcie_plat/ras_des_counter/<name>/counter_value
 * It returns the number of time the selected event has happened if enabled
 */
static ssize_t ras_event_counter_value_read(struct file *file, char __user *buf,
					    size_t count, loff_t *ppos)
{
	u32 ret;
	u32 val;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;

	set_event_number(file);

	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
				RAS_DES_EVENT_COUNTER_DATA_REG);
	scnprintf(debugfs_buf, sizeof(debugfs_buf), "0x%x\n", val);
	ret = simple_read_from_buffer(buf, count, ppos, debugfs_buf,
				      strlen(debugfs_buf));

	return ret;
}

/*
 * ras_event_counter_en_write: Function to set if counter is enable
 *
 * This function is invoked when the following command is made:
 * echo n > /sys/kernel/debug/dwc_pcie_plat/
 *		ras_des_counter/<name>/counter_enable
 * Here n can be 1 to enable and 0 to disable
 */
static ssize_t ras_event_counter_en_write(struct file *file,
					  const char __user *buf,
					  size_t count, loff_t *ppos)
{
	u32 ret;
	u32 val;
	u32 enable;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;

	ret = kstrtou32_from_user(buf, count, 0, &enable);
	if (ret)
		return ret;

	set_event_number(file);

	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
				RAS_DES_EVENT_COUNTER_CTRL_REG);
	if (enable)
		val |= PER_EVENT_ON;
	else
		val |= PER_EVENT_OFF;

	dw_pcie_writel_dbi(pci, pci->ras_cap_offset +
			   RAS_DES_EVENT_COUNTER_CTRL_REG, val);

	return count;
}

/*
 * ras_event_counter_lane_sel_write: Function to set lane number
 *
 * This function is invoked when the following command is made:
 * echo n > /sys/kernel/debug/dwc_pcie_plat/ras_des_counter/<name>/lane_select
 * Here n is the lane that we want to select for counter configuration
 */
static ssize_t ras_event_counter_lane_sel_write(struct file *file,
						const char __user *buf,
						size_t count, loff_t *ppos)
{
	u32 ret;
	u32 val;
	u32 lane;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;

	ret = kstrtou32_from_user(buf, count, 0, &lane);
	if (ret)
		return ret;

	set_event_number(file);
	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
				RAS_DES_EVENT_COUNTER_CTRL_REG);
	val &= ~(LANE_SELECT_MASK << LANE_SELECT_SHIFT);
	val |= (lane << LANE_SELECT_SHIFT);
	dw_pcie_writel_dbi(pci, pci->ras_cap_offset +
			   RAS_DES_EVENT_COUNTER_CTRL_REG, val);

	return count;
}

/*
 * ras_error_inj_read: Function to read number of errors left to be injected
 *
 * This function is invoked when the following command is made:
 * cat /sys/kernel/debug/dwc_pcie_plat/ras_des_error_inj/<name of error>
 * This returns the number of errors left to be injected which will
 * keep reducing as we make pcie transactions to inject error.
 */
static ssize_t ras_error_inj_read(struct file *file, char __user *buf,
				  size_t count, loff_t *ppos)
{
	u32 ret;
	u32 val, err_num, inj_num;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;

	err_num = get_error_inj_number(file);
	inj_num = (err_num & 0xFF);

	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset + ERR_INJ0_OFF +
				(0x4 * inj_num));
	scnprintf(debugfs_buf, sizeof(debugfs_buf), "0x%x\n", (val & EINJ_COUNT_MASK));
	ret = simple_read_from_buffer(buf, count, ppos, debugfs_buf,
				      strlen(debugfs_buf));

	return ret;
}

/*
 * ras_error_inj_write: Function to set number of errors to be injected
 *
 * This function is invoked when the following command is made:
 * echo n > /sys/kernel/debug/dwc_pcie_plat/ras_des_error_inj/<name of error>
 * Here n is the number of errors we want to inject
 */
static ssize_t ras_error_inj_write(struct file *file, const char __user *buf,
				   size_t count, loff_t *ppos)
{
	u32 ret;
	u32 val, err_num, inj_num;
	u32 counter;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;

	if (count > DEBUGFS_BUF_MAX_SIZE)
		return -EINVAL;

	ret = kstrtou32_from_user(buf, count, 0, &counter);
	if (ret)
		return ret;

	err_num = get_error_inj_number(file);
	inj_num = (err_num & 0xFF);
	err_num = (err_num >> 8);

	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset + ERR_INJ0_OFF +
				(0x4 * inj_num));
	val &= ~(EINJ_TYPE_MASK << EINJ_TYPE_SHIFT);
	val |= (err_num << EINJ_TYPE_SHIFT);
	val &= ~(EINJ_COUNT_MASK);
	val |= counter;
	dw_pcie_writel_dbi(pci, pci->ras_cap_offset + ERR_INJ0_OFF +
			   (0x4 * inj_num), val);
	dw_pcie_writel_dbi(pci, pci->ras_cap_offset +
			   ERR_INJ_ENABLE_REG, (0x1 << inj_num));

	return count;
}

static ssize_t lane_detection_read(struct file *file, char __user *buf,
				   size_t count, loff_t *ppos)
{
	u32 ret;
	u32 val;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;

	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
				SD_STATUS_L1LANE_REG);
	val = (val >> LANE_DETECT_SHIFT) & LANE_DETECT_MASK;
	scnprintf(debugfs_buf, sizeof(debugfs_buf), "0x%x\n", val);

	ret = simple_read_from_buffer(buf, count, ppos, debugfs_buf,
				      strlen(debugfs_buf));

	return ret;
}

static ssize_t rx_valid_read(struct file *file, char __user *buf,
			     size_t count, loff_t *ppos)
{
	u32 ret;
	u32 val;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;

	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
				SD_STATUS_L1LANE_REG);
	val = (val >> PIPE_RXVALID_SHIFT) & PIPE_RXVALID_MASK;
	scnprintf(debugfs_buf, sizeof(debugfs_buf), "0x%x\n", val);

	ret = simple_read_from_buffer(buf, count, ppos, debugfs_buf,
				      strlen(debugfs_buf));

	return ret;
}

static ssize_t lane_selection_write(struct file *file, const char __user *buf,
				    size_t count, loff_t *ppos)
{
	u32 ret;
	u32 val;
	u32 lane;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;

	if (count > DEBUGFS_BUF_MAX_SIZE)
		return -EINVAL;

	ret = kstrtou32_from_user(buf, count, 0, &lane);
	if (ret)
		return ret;

	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
				SD_STATUS_L1LANE_REG);
	val &= ~(LANE_SELECT_MASK);
	val |= lane;
	dw_pcie_writel_dbi(pci, pci->ras_cap_offset +
			   SD_STATUS_L1LANE_REG, val);

	return count;
}

/*
 * ras_des_config_timer: Function to set timer
 * This function is invoked when the following command is made:
 * echo n > /sys/kernel/debug/dwc_pcie_plat/
 *              ras_des_time_analysis/<name>/select_duration
 * Here n can be 0 to 6 based on time duration
 */
static ssize_t ras_des_config_timer(struct file *file,
					const char __user *buf,
					size_t count, loff_t *ppos)
{
	u32 ret;
	u32 val;
	u32 timer;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;

	ret = kstrtou32_from_user(buf, count, 0, &timer);
	if (ret)
		return ret;

	if (timer < 0 || timer > 6)
		return -EINVAL;

	set_time_event_number(file);

	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
			RAS_DES_TIME_BASE_CTRL_REG);

	val |= (timer << TIMER_SHIFT);
	val |= TIMER_ENABLE;

	dw_pcie_writel_dbi(pci, pci->ras_cap_offset +
			RAS_DES_TIME_BASE_CTRL_REG , val);
	return count;
}

/*
 * ras_des_stop_timer: Function to stop manual timer
 *
 * This function is invoked when the following command is made:
 * echo n > /sys/kernel/debug/dwc_pcie_plat/
 *              ras_des_time_analysis/<name>/stop_timer
 * Here n should be 1 to stop timer
 */
static ssize_t ras_des_stop_timer(struct file *file, const char __user *buf,
					size_t count, loff_t *ppos)
{
	u32 ret;
	u32 val;
	u32 stop;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;

	ret = kstrtou32_from_user(buf, count, 0, &stop);
	if (ret)
		return ret;

	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
			RAS_DES_TIME_BASE_CTRL_REG);

	/* Disabling configured Manual Timer mode */
	if ((val & TIMER_MASK) == 0) {
		val &= ~TIMER_ENABLE;
		dw_pcie_writel_dbi(pci, pci->ras_cap_offset +
				RAS_DES_TIME_BASE_CTRL_REG, val);
	}

	return count;
}

/*
 * ras_des_read_time: Function to get counter value
 *
 * This function is invoked when the following command is made:
 * cat /sys/kernel/debug/dwc_pcie_plat/ras_des_time_analysis/<name>/read_time
 * It returns time analysis of the selected event has happened if enabled
*/
static ssize_t ras_des_read_time(struct file *file, char __user *buf,
					size_t count, loff_t *ppos)
{
	int i;
	u32 ret;
	u32 val;
	u32 multiplier;
	struct dw_pcie *pci = (struct dw_pcie *) file->private_data;
	u32 max_size = sizeof(time_based_list) / sizeof(struct time_report);

	val = dw_pcie_readl_dbi(pci, pci->ras_cap_offset +
			RAS_DES_TIME_BASE_DATA_REG);

	for (i = 0; i < max_size; i++) {
		if (strcmp(time_based_list[i].name,
					file->f_path.dentry->d_parent->d_iname) == 0) {
			multiplier = time_based_list[i].multiplier;
			break;
		}
	}

	val = val * multiplier;

	scnprintf(debugfs_buf, sizeof(debugfs_buf), "0x%x\n", val);
	ret = simple_read_from_buffer(buf, count, ppos, debugfs_buf,
			strlen(debugfs_buf));
	return ret;
}

static const struct file_operations lane_detection_fops = {
	.open = open,
	.read = lane_detection_read,
	.write = lane_selection_write
};

static const struct file_operations rx_valid_fops = {
	.open = open,
	.read = rx_valid_read,
	.write = lane_selection_write
};

static const struct file_operations cnt_en_ops = {
	.read = ras_event_counter_en_read,
	.write = ras_event_counter_en_write,
	.open = simple_open,
};

static const struct file_operations lane_sel_ops = {
	.read = ras_event_counter_lane_sel_read,
	.write = ras_event_counter_lane_sel_write,
	.open = simple_open,
};

static const struct file_operations cnt_val_ops = {
	.read = ras_event_counter_value_read,
	.open = simple_open,
};

static const struct file_operations inj_ops = {
	.read = ras_error_inj_read,
	.write = ras_error_inj_write,
	.open = simple_open,
};

static const struct file_operations config_timer_ops = {
	.write = ras_des_config_timer,
	.open = simple_open,
};

static const struct file_operations stop_timer_ops = {
	.write = ras_des_stop_timer,
	.open = simple_open,
};

static const struct file_operations read_data_ops = {
	.read = ras_des_read_time,
	.open = simple_open,
};

int create_debugfs_files(struct dw_pcie *pci)
{
	int ret = 0;
	char dirname[32];
	struct device *dev;

	struct dentry *ras_des_debug_regs;
	struct dentry *ras_des_error_inj;
	struct dentry *ras_des_event_counter;
	struct dentry *ras_des_time_analysis;
	struct dentry *lane_detection;
	struct dentry *rx_valid;

	if (!pci) {
		pr_err("pcie struct is NULL\n");
		return -ENODEV;
	}

	dev = pci->dev;
	scnprintf(dirname, sizeof(dirname), "pcie_dwc_%s", dev_name(dev));

	pci->ras_cap_offset = dw_pcie_find_vsec_capability(pci,
							   DW_PCIE_RAS_CAP_ID);
	if (!pci->ras_cap_offset) {
		pr_err("No RAS capability available\n");
		return -ENODEV;
	}

	/* Create main directory for each platform driver */
	dir = debugfs_create_dir(dirname, NULL);
	if (dir == NULL) {
		pr_err("error creating directory: %s\n", dirname);
		return -ENODEV;
	}

	/* Create sub dirs for Debug, Error injection, Statistics */
	ras_des_debug_regs = debugfs_create_dir("ras_des_debug_regs", dir);
	if (ras_des_debug_regs == NULL) {
		pr_err("error creating directory: %s\n", dirname);
		ret = -ENODEV;
		goto remove_debug_file;
	}

	ras_des_error_inj = debugfs_create_dir("ras_des_error_inj", dir);
	if (ras_des_error_inj == NULL) {
		pr_err("error creating directory: %s\n", dirname);
		ret = -ENODEV;
		goto remove_debug_file;
	}

	ras_des_event_counter = debugfs_create_dir("ras_des_counter", dir);
	if (ras_des_event_counter == NULL) {
		pr_err("error creating directory: %s\n", dirname);
		ret = -ENODEV;
		goto remove_debug_file;
	}

	ras_des_time_analysis = debugfs_create_dir("ras_des_time_analysis", dir);
	if (ras_des_time_analysis == NULL) {
		pr_err("error creating directory: %s\n", dirname);
		ret = -ENODEV;
		goto remove_debug_file;
	}

	/* Create debugfs files for Debug subdirectory */
	lane_detection = debugfs_create_file("lane_detection", 0644,
					     ras_des_debug_regs, pci,
					     &lane_detection_fops);

	rx_valid = debugfs_create_file("rx_valid", 0644,
					     ras_des_debug_regs, pci,
					     &lane_detection_fops);

	/* Create debugfs files for Error injection sub dir */
	CREATE_RAS_ERROR_INJECTION_DEBUGFS(tx_ecrc);
	CREATE_RAS_ERROR_INJECTION_DEBUGFS(rx_ecrc);
	CREATE_RAS_ERROR_INJECTION_DEBUGFS(tx_lcrc);
	CREATE_RAS_ERROR_INJECTION_DEBUGFS(rx_lcrc);

	/* Create debugfs files for Statistical counter sub dir */
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(ebuf_overflow);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(ebuf_underrun);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(decode_error);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(receiver_error);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(framing_error);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(lcrc_error);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(ecrc_error);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(unsupp_req_error);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(cmpltr_abort_error);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(cmpltn_timeout_error);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(tx_l0s_entry);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(rx_l0s_entry);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(l1_entry);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(l1_1_entry);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(l1_2_entry);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(l2_entry);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(speed_change);
	CREATE_RAS_EVENT_COUNTER_DEBUGFS(width_chage);

	/* Create debugfs files for Statistical time analysis sub dir */
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(core_1_cycle);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(core_tx_l0s);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(core_rx_l0s);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(core_l0);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(core_l1);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(core_cfg_rvcry);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(core_tx_rx_l0s);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(aux_l1_1);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(aux_l1_2);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(aux_l1);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(ccix_1_cycle);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(ccix_tx_l0s);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(ccix_rx_l0s);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(ccix_l0);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(ccix_l1);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(ccix_cfg_rvcry);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(ccix_tx_rx_l0s);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(tx_pcie_tlp_data);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(rx_pcie_tlp_data);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(tx_ccix_tlp_data);
	CREATE_RAS_DES_TIME_ANALYSIS_DEBUGFS(rx_ccix_tlp_data);

	return ret;

remove_debug_file:
	remove_debugfs_files();
	return ret;
}

void remove_debugfs_files(void)
{
	debugfs_remove_recursive(dir);
}
