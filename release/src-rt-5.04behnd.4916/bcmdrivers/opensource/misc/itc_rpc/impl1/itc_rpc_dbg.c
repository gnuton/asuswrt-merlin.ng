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
 /****************************************************************************
 * Author: Peter Sulc <psulc@broadcom.com>
 *****************************************************************************/
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>

#include <itc_rpc.h>
#include <itc_msg_q.h>
#include <itc_channel_structs.h>

#include "itc_rpc_dbg.h"

#define PROC_DIR	"driver/itc-rpc"
#define PROC_FILE	"info"

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_file;

static int itc_info_proc_open(struct inode *inode, struct file *file);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
static const struct proc_ops info_fops = {
	.proc_open		= itc_info_proc_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};
#else
static const struct file_operations info_fops = {
	.owner		= THIS_MODULE,
	.open		= itc_info_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

int __init itc_proc_init(void)
{
	proc_dir = proc_mkdir(PROC_DIR, NULL);
	if (!proc_dir) {
		pr_err("Failed to create PROC directory %s.\n", PROC_DIR);
		return -EIO;
	}
	proc_file = proc_create(PROC_FILE, 0, proc_dir, &info_fops);
	if (!proc_file) {
		pr_err("Failed to create %s\n", PROC_FILE);
		remove_proc_entry(PROC_DIR, NULL);
		return -EIO;
	}
	return 0;
}

void itc_proc_exit(void)
{
	if (proc_file) {
		remove_proc_entry(PROC_FILE, proc_dir);
		proc_file = NULL;
	}
	if (proc_dir) {
		remove_proc_entry(PROC_DIR, NULL);
		proc_dir = NULL;
	}
}

static int itc_info_proc_show(struct seq_file *m, void *v)
{
	int service;
	rpc_service *s;

	seq_printf(m, "%15s %6s %5s %4s %4s %4s %8s %5s %8s %5s\n",
		   "Service", "Active", "Funcs", "RCVq", "REQq", "Orph",
		   "Tx", "TXerr", "Rx", "RXerr");
	for (service = 0; service < RPC_MAX_SERVICES; service++)
	{
		s = &itc_rpc_services[service];
		seq_printf(m, "%15s %6s %5d %4d %4d %4d %8d %5d %8d %5d\n",
			   s->thread_name, s->active ? "yes" : "no",
			   s->func_tab_sz,
			   s->rcv_queue ? s->rcv_queue->sema.count : 0,
			   s->req_queue ? s->req_queue->sema.count : 0,
			   s->orphan_queue ? s->orphan_queue->sema.count : 0,
			   s->tx_cnt, s->tx_err_cnt, s->rx_cnt, s->rx_err_cnt);
	}
	seq_printf(m, "Free MSG Pool: %d\n", itc_rpc_msg_pool_count());
	rpc_tunnel_info(m);
	return 0;
}

static int itc_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, itc_info_proc_show, NULL);
}
