/****************************************************************************
 *
 * Broadcom Proprietary and Confidential. (c) 2017 Broadcom.  All rights reserved.
 * The term "Broadcom" refers to Broadcom Limited and/or its subsidiaries.
 *
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to
 * you under the terms of the GNU General Public License version 2 (the
 * "GPL"), available at [http://www.broadcom.com/licenses/GPLv2.php], with
 * the following added to such license:
 *
 * As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy
 * and distribute the resulting executable under terms of your choice,
 * provided that you also meet, for each linked independent module, the
 * terms and conditions of the license of that module. An independent
 * module is a module which is not derived from this software. The special
 * exception does not apply to any modifications of the software.
 *
 * Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a
 * license other than the GPL, without Broadcom's express prior written
 * consent.
 *
 ****************************************************************************/
 /*
 * power_service.c
 * Dec. 2016
 *
 ******************************************************************************/
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <power_services.h>
#include "misc_services.h"
#include <brcm_mbox.h>
#include <brcm_ba.h>
#include <proc_cmd.h>

#define MODULE_NAME "power_service"
#define PROC_POWERMAN_STAT_CMD_FILE "pms"
static struct proc_dir_entry *proc_power_dir;
static struct proc_dir_entry *proc_cmpower_file;
static struct proc_dir_entry *proc_cmtemperature_file;
static struct proc_dir_entry *proc_pms_cmd_file;
static int cmpower_proc_open(struct inode *inode, struct file *file);
static int cmtemperature_proc_open(struct inode *inode, struct file *file);
static void power_proc_exit(void);
int __init power_proc_init(void);

static int current_cm_power_mode;
static int current_cm_temperature_state;

enum {
	PMS_CMD_NULL        = 0,
	PMS_CMD_GUT         = 1,
	PMS_CMD_GDT         = 2,
	PMS_CMD_GPS         = 3,
	PMS_CMD_ALL         = 4
};

struct powerman_stat {
	int command;
	unsigned long uptime;
	unsigned long downtime;
	int pulse_state;
};

static struct powerman_stat stat;

static void *pms_db_seq_start(struct seq_file *seq, loff_t *pos)
{
	if (!*pos)
		return SEQ_START_TOKEN;
	return 0;
}

static void *pms_db_seq_next(struct seq_file *seq, void *v,
				      loff_t *pos)
{
	(*pos)++;
	return 0;
}

static void pms_db_seq_stop(struct seq_file *seq, void *v)
{
}

static int pms_db_seq_show(struct seq_file *seq, void *v)
{
	switch (stat.command) {
	case PMS_CMD_GUT:
		seq_printf(seq,"%lu\n", stat.uptime);
		break;
	case PMS_CMD_GDT:
		seq_printf(seq,"%lu\n", stat.downtime);
		break;
	case PMS_CMD_GPS:
		seq_printf(seq,"%d\n", stat.pulse_state);
		break;
	default:
		break;
	}

	return 0;
}

static const struct seq_operations pms_db_seq_ops = {
	.start	= pms_db_seq_start,
	.next	= pms_db_seq_next,
	.stop	= pms_db_seq_stop,
	.show	= pms_db_seq_show,
};

static int gphy_pulse_stat(int argc, char *argv[])
{
	int ret = 0;
	unsigned long ulv;
	long lv;

	stat.command = PMS_CMD_NULL;

	if (argc < 2)
		goto help;

	if (strstr(argv[1], "get")) {
		if(!strcmp(argv[1], "get_uptime"))
			stat.command = PMS_CMD_GUT;
		else if(!strcmp(argv[1], "get_downtime"))
			stat.command = PMS_CMD_GDT;
		else if(!strcmp(argv[1], "get_pulse_state"))
			stat.command = PMS_CMD_GPS;
		else
			goto help;	
	}
	else if ((strstr(argv[1], "set")) && (argc > 2)) {
		if(!strcmp(argv[1], "set_uptime")) {
			ret = kstrtoul(argv[2], 10, &ulv);
			if (ret || ulv > UINT_MAX)
				goto help;
			stat.uptime = ulv;
		}
		else if(!strcmp(argv[1], "set_downtime")) {
			ret = kstrtoul(argv[2], 10, &ulv);
			if (ret || ulv > UINT_MAX)
				goto help;
			stat.downtime = ulv;
		}
		else if(!strcmp(argv[1], "set_pulse_state")) {
			ret = kstrtol(argv[2], 10, &lv);
			if (ret)
				goto help;
			stat.pulse_state = (int)lv;
		}
		else
			goto help;	
	}
	else
		goto help;

	if (ret == 0)
		goto done;
help:
	pr_info("To Get Powerman Pulse info:\n");
	pr_info("pms gphy_pulse <get_uptime | get_downtime | get_pulse_state>\n");
	pr_info("To Set Powerman Pulse info:\n");
	pr_info("pms gphy_pulse <set_uptime | set_downtime | set_pulse_state> <value>\n");
	pr_info("<value> for pulse state <0-None | 1-Disable | 2-Enable>\n");

done:
	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
static const struct proc_ops cmpower_fops = {
	.proc_open		= cmpower_proc_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};

static const struct proc_ops cmtemperature_fops = {
	.proc_open		= cmtemperature_proc_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};
#else
static const struct file_operations cmpower_fops = {
	.owner		= THIS_MODULE,
	.open		= cmpower_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations cmtemperature_fops = {
	.owner		= THIS_MODULE,
	.open		= cmtemperature_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static struct proc_cmd_ops pms_command_entries[] = {
	{ .name = "gphy_pulse", .do_command = gphy_pulse_stat},
};

struct proc_cmd_table pms_command_table = {
	.module_name = MODULE_NAME,
	.size = ARRAY_SIZE(pms_command_entries),
	.data_seq_read = (void *) &pms_db_seq_ops,
	.ops = pms_command_entries
};

void power_proc_exit(void)
{
	if (proc_cmpower_file) {
		remove_proc_entry(PROC_CMPOWER_FILE, proc_power_dir);
		proc_cmpower_file = NULL;
	}

	if (proc_cmtemperature_file) {
		remove_proc_entry(PROC_CMTEMPERATURE_FILE, proc_power_dir);
		proc_cmtemperature_file = NULL;
	}

	if (proc_pms_cmd_file) {
		remove_proc_entry(PROC_POWERMAN_STAT_CMD_FILE, proc_power_dir);
		proc_pms_cmd_file = NULL;
	}

	if (proc_power_dir) {
		remove_proc_entry(PROC_POWER_DIR, NULL);
		proc_power_dir = NULL;
	}
}

int __init power_proc_init(void)
{
	int status = 0;

	proc_power_dir = proc_mkdir(PROC_POWER_DIR, NULL);
	if (!proc_power_dir) {
		pr_err("Failed to create PROC proc_power  directory %s.\n",
			   PROC_POWER_DIR);
		status = -EIO;
		goto done;
	}

	proc_cmpower_file = proc_create(PROC_CMPOWER_FILE, 0, proc_power_dir,
									&cmpower_fops);
	if (!proc_cmpower_file) {
		pr_err("Failed to create %s\n", PROC_CMPOWER_FILE);
		status = -EIO;
		power_proc_exit();
		goto done;
	}

	proc_cmtemperature_file = proc_create(PROC_CMTEMPERATURE_FILE, 0, proc_power_dir,
									&cmtemperature_fops);
	if (!proc_cmtemperature_file) {
		pr_err("Failed to create %s\n", PROC_CMTEMPERATURE_FILE);
		status = -EIO;
		power_proc_exit();
		goto done;
	}

	proc_pms_cmd_file = proc_create_cmd(PROC_POWERMAN_STAT_CMD_FILE, 
			proc_power_dir, &pms_command_table);
	if (!proc_pms_cmd_file) {
		pr_err("Failed to create %s\n", PROC_POWERMAN_STAT_CMD_FILE);
		status = -EIO;
		power_proc_exit();
		goto done;
	}
done:
	return status;
}

int __init power_services_init(void)
{
	current_cm_power_mode = CM_POWER_FULL;
	current_cm_temperature_state = CM_TEMPERATURE_NORMAL;

	power_proc_init();
	return 0;
}
EXPORT_SYMBOL(power_services_init);

void power_services_cleanup(void)
{
	power_proc_exit();
}
EXPORT_SYMBOL(power_services_cleanup);

static int cmpower_proc_read(struct seq_file *m, void *v)
{
	seq_printf(m,"%d\n",(unsigned int)current_cm_power_mode);
	return 0;
}

static int cmpower_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cmpower_proc_read, NULL);
}

int cm_power_ctrl(int dqm_tunnel, struct misc_msg *msg)
{
	current_cm_power_mode = (unsigned int)msg->data;
	return 0;
}

static int cmtemperature_proc_read(struct seq_file *m, void *v)
{
	seq_printf(m,"%d\n",(int)current_cm_temperature_state);
	return 0;
}

static int cmtemperature_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cmtemperature_proc_read, NULL);
}

int cm_temperature_state(int dqm_tunnel, struct misc_msg *msg)
{
	current_cm_temperature_state = (int)msg->data;
	return 0;
}

int rg_power_config(int dqm_tunnel, struct misc_msg *msg)
{
#if defined(CONFIG_BCM_BOOT_ASSIST)
	msg->data = (uint32_t)brcm_ba_rg_batt_mode();
#else
	msg->data = 0;
#endif

	if (rpc_msg_request((rpc_msg *)msg))
	 rpc_send_reply(dqm_tunnel, (rpc_msg *)msg);

	return 0;
}

module_init(power_services_init);
module_exit(power_services_cleanup);
MODULE_LICENSE("GPL");
