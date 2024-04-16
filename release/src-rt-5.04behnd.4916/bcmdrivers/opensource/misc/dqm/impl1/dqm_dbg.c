/****************************************************************************
 *
 * Broadcom Proprietary and Confidential.
 * (c) 2019 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
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
 ****************************************************************************
 * Author: Tim Ross <tross@broadcom.com>
 * Author: Jayesh Patel <jayesh.patel@broadcom.com>
 *****************************************************************************/
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include "dqm_dev.h"
#include "dqm_dbg.h"

#define PROC_DIR		    "driver/dqm"
#define STATUS_PROC_FILE	"status"
#define RX_MODE_PROC_FILE	"rx_mode"
#define TX_MODE_PROC_FILE	"tx_mode"

static int dqm_status_proc_open(struct inode *inode, struct file *file);
static void *dqm_status_proc_start(struct seq_file *seq, loff_t *pos);
static void dqm_status_proc_stop(struct seq_file *seq, void *v);
static void *dqm_status_proc_next(struct seq_file *seq, void *v,
				  loff_t *pos);
static int dqm_status_proc_show(struct seq_file *seq, void *v);

static ssize_t dqm_rx_mode_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dqm_rx_mode_write(struct file *, const char __user *, size_t, loff_t *);
static ssize_t dqm_tx_mode_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dqm_tx_mode_write(struct file *, const char __user *, size_t, loff_t *);

static const struct seq_operations status_seq_ops = {
	.start	= dqm_status_proc_start,
	.stop	= dqm_status_proc_stop,
	.next	= dqm_status_proc_next,
	.show	= dqm_status_proc_show,
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
static const struct proc_ops status_fops = {
	.proc_open		= dqm_status_proc_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= seq_release,
};
static const struct proc_ops rx_mode_fops = {
	.proc_read		= dqm_rx_mode_read,
	.proc_write		= dqm_rx_mode_write,
};

static const struct proc_ops tx_mode_fops = {
	.proc_read		= dqm_tx_mode_read,
	.proc_write		= dqm_tx_mode_write,
};
#else
static const struct file_operations status_fops = {
	.owner		= THIS_MODULE,
	.open		= dqm_status_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
static const struct file_operations rx_mode_fops = {
	.owner		= THIS_MODULE,
	.read		= dqm_rx_mode_read,
	.write		= dqm_rx_mode_write,
};

static const struct file_operations tx_mode_fops = {
	.owner		= THIS_MODULE,
	.read		= dqm_tx_mode_read,
	.write		= dqm_tx_mode_write,
};
#endif

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *status_proc_file;
static struct proc_dir_entry *rx_mode_proc_file;
static struct proc_dir_entry *tx_mode_proc_file;

int __init dqm_proc_init(void)
{
	int status = 0;

	pr_debug("-->\n");
	proc_dir = proc_mkdir(PROC_DIR, NULL);
	if (!proc_dir) {
		pr_err("Failed to create PROC directory %s.\n",
		       PROC_DIR);
		status = -EIO;
		goto done;
	}
	status_proc_file = proc_create(STATUS_PROC_FILE, S_IRUGO,
					 proc_dir, &status_fops);
	if (!status_proc_file) {
		pr_err("Failed to create %s\n", STATUS_PROC_FILE);
		status = -EIO;
		dqm_proc_exit();
		goto done;
	}
	rx_mode_proc_file = proc_create(RX_MODE_PROC_FILE, S_IRUGO|S_IWUGO,
					proc_dir, &rx_mode_fops);
	if (!rx_mode_proc_file) {
		pr_err("Failed to create %s\n", RX_MODE_PROC_FILE);
		status = -EIO;
		dqm_proc_exit();
		goto done;
	}
	tx_mode_proc_file = proc_create(TX_MODE_PROC_FILE, S_IRUGO|S_IWUGO,
					proc_dir, &tx_mode_fops);
	if (!tx_mode_proc_file) {
		pr_err("Failed to create %s\n", TX_MODE_PROC_FILE);
		status = -EIO;
		dqm_proc_exit();
		goto done;
	}

done:
	pr_debug("<--\n");
	return status;
}

void dqm_proc_exit(void)
{
	pr_debug("-->\n");
	if (status_proc_file) {
		remove_proc_entry(STATUS_PROC_FILE, proc_dir);
		status_proc_file = NULL;
	}
	if (proc_dir) {
		remove_proc_entry(PROC_DIR, NULL);
		proc_dir = NULL;
	}
	pr_debug("<--\n");
}

int dqmdev_proc_init(struct dqmdev *qdev)
{
	int status = 0;

	pr_debug("-->\n");
	qdev->proc_dir = proc_mkdir(qdev->name, proc_dir);
	if (!qdev->proc_dir) {
		pr_err("Failed to create PROC directory %s.\n",
		       PROC_DIR);
		status = -EIO;
		goto done;
	}
	qdev->status_proc_file = proc_create_data(STATUS_PROC_FILE, S_IRUGO,
						  qdev->proc_dir,
						  &status_fops, qdev);
	if (!qdev->status_proc_file) {
		pr_err("Failed to create %s/%s\n", qdev->name,
		       STATUS_PROC_FILE);
		status = -EIO;
		dqmdev_proc_exit(qdev);
		goto done;
	}
done:
	pr_debug("<--\n");
	return status;
}

void dqmdev_proc_exit(struct dqmdev *qdev)
{
	pr_debug("-->\n");
	if (qdev->status_proc_file) {
		remove_proc_entry(STATUS_PROC_FILE, qdev->proc_dir);
		qdev->status_proc_file = NULL;
	}
	if (qdev->proc_dir) {
		remove_proc_entry(qdev->name, proc_dir);
		qdev->proc_dir = NULL;
	}
	pr_debug("<--\n");
}

static int dqm_status_proc_open(struct inode *inode, struct file *file)
{
	int status;

	pr_debug("-->\n");
	status = seq_open(file, &status_seq_ops);
	if (!status) {
		struct seq_file *sf = file->private_data;

		sf->private = PDE_DATA(inode);
	}
	pr_debug("<--\n");
	return status;
}

static void *dqm_status_proc_start(struct seq_file *seq, loff_t *pos)
{
	struct dqmdev *qdev = NULL;
	struct dqmdev *tmp;
	loff_t off = 0;

	pr_debug("-->\n");

	if (seq->private) {
		if (off++ == *pos)
			return seq->private;
		return NULL;
	}

	rcu_read_lock();
	list_for_each_entry_rcu(tmp, &dqmdevs, list) {
		if (off++ == *pos) {
			qdev = tmp;
			break;
		}
	}
	rcu_read_unlock();

	pr_debug("<--\n");
	return qdev;
}

static void dqm_status_proc_stop(struct seq_file *seq, void *v)
{
	pr_debug("-->\n");
	pr_debug("<--\n");
}

static void *dqm_status_proc_next(struct seq_file *seq, void *v,
				  loff_t *pos)
{
	struct dqmdev *qdev = NULL;
	struct dqmdev *cur = v;
	struct list_head *next;

	pr_debug("-->\n");

	if (seq->private)
		return NULL;

	(*pos)++;
	rcu_read_lock();
	next = cur->list.next;
	if (next != &dqmdevs)
		qdev = container_of(next, struct dqmdev, list);
	rcu_read_unlock();
	pr_debug("<--\n");
	return qdev;
}

#define show_reg_dqm(qdev, name, format, member) ({ \
	int i, first = 0; \
	struct dqm *q; \
	for (i = 0; i < qdev->q_count; i++) { \
		q = &qdev->dqm[i]; \
		if ((q->flags & DQM_F_TX) || (q->flags & DQM_F_RX)) { \
			if (!first) { \
				seq_printf(seq, "\n\t%s", name); \
				first = 1; \
			} \
			seq_printf(seq, format, member); \
		} \
	} })

#define show_reg_dqm_avg_depth(qdev, name, format, member) ({ \
	int i, first = 0; \
	struct dqm *q; \
	for (i = 0; i < qdev->q_count; i++) { \
		q = &qdev->dqm[i]; \
		if ((q->flags & DQM_F_TX) || (q->flags & DQM_F_RX)) { \
			if (!first) { \
				seq_printf(seq, "\n\t%s", name); \
				first = 1; \
			} \
			seq_printf(seq, format, \
				q->stats.member ## _attempts ? \
				q->stats.member ## _q_depth_sum / \
				q->stats.member ## _attempts : 0); \
		} \
	} })

static int dqm_status_proc_show(struct seq_file *seq, void *v)
{
	struct dqmdev *qdev = v;
	struct dqm_intc_reg *intcreg;
	int i, bank;
	struct dqm *q;

	pr_debug("-->\n");

	seq_printf(seq, "DEVICE: %s\n", qdev->name);
	if (!qdev->intc)
		goto print_dqm;
	if (qdev->intc[0].type != DQM_INTC_LEGACY)
		goto print_dqm;
	for (i = 0; i < qdev->intc_count; i++) {
		intcreg = qdev->intc[i].reg;
		seq_printf(seq, "\tIRQ Controller %d: Type %d\n", i,
			   qdev->intc[i].type);
		if (qdev->intc[0].reg[0].l1_irq_mask)
			seq_printf(seq, "\t\tL1 IRQ Mask  : 0x%08x\n",
			   dqm_reg_read(intcreg[0].l1_irq_mask));
		if (qdev->intc[0].reg[0].l1_irq_status)
			seq_printf(seq, "\t\tL1 IRQ Status: 0x%08x\n",
			   dqm_reg_read(intcreg[0].l1_irq_status));
		seq_printf(seq, "\tDQM  | Low Watermark | Low Watermark | "
			   "Not Empty  | Not Empty  | Not Empty ");
		seq_printf(seq, " | Hi Watermark | Hi Watermark");
		seq_printf(seq, " |   Timer    |   Timer   ");
		seq_printf(seq, "\n");
		seq_printf(seq, "\tBank |   IRQ Mask    |   IRQ Status  | "
			   "IRQ Mask   | IRQ Status |   Status  ");
		seq_printf(seq, " |   IRQ Mask   |  IRQ Status ");
		seq_printf(seq, " | IRQ Mask   | IRQ Status");
		seq_printf(seq, "\n");
		for (bank = 0; bank < qdev->bank_count; bank++) {
			seq_printf(seq, "\t%4d  | ", bank);
			seq_printf(seq, "0x%08x    | ",
				   dqm_reg_read(intcreg[bank].lwm_irq_mask));
			seq_printf(seq, "0x%08x    | ",
				   dqm_reg_read(intcreg[bank].lwm_irq_status));
			seq_printf(seq, "0x%08x | ",
				   dqm_reg_read(intcreg[bank].ne_irq_mask));
			seq_printf(seq, "0x%08x | ",
				   dqm_reg_read(intcreg[bank].ne_irq_status));
			seq_printf(seq, "0x%08x",
				   dqm_reg_read(intcreg[bank].ne_status));
			if (intcreg[bank].hwm_irq_mask) {
				seq_printf(seq, " | 0x%08x  ",
				   dqm_reg_read(intcreg[bank].hwm_irq_mask));
				seq_printf(seq, " | 0x%08x  ",
				   dqm_reg_read(intcreg[bank].hwm_irq_status));
			}
			if (intcreg[bank].tmr_irq_mask) {
				seq_printf(seq, " | 0x%08x",
				   dqm_reg_read(intcreg[bank].tmr_irq_mask));
				seq_printf(seq, " | 0x%08x",
				   dqm_reg_read(intcreg[bank].tmr_irq_status));
			}
			seq_printf(seq, "\n");
		}
	}
	seq_printf(seq, "\n");
print_dqm:
	seq_printf(seq, "\tRegistered DQM's:\n");

	seq_printf(seq, "     |   Msgs   |   Msgs   | Attempts | Attempts |   Peak   |   Peak   |    Avg   |    Avg   |\n");
	seq_printf(seq, " DQM |    TX    |    RX    |    TX    |    RX    | Depth TX | Depth RX | Depth TX | Depth RX |\n");
	for (i = 0; i < qdev->q_count; i++) {
		q = &qdev->dqm[i];
		if ((q->flags & DQM_F_TX) || (q->flags & DQM_F_RX)) {
			seq_printf(seq, " %3d |%10d|%10d|%10d|%10d|%10d|%10d|%10d|%10d|\n",
				   q->num, q->stats.tx_cnt, q->stats.rx_cnt,
				   q->stats.tx_attempts, q->stats.rx_attempts,
				   q->stats.tx_q_depth_peak, q->stats.rx_q_depth_peak,
				   q->stats.tx_attempts ? q->stats.tx_q_depth_sum / q->stats.tx_attempts : 0,
				   q->stats.rx_attempts ? q->stats.rx_q_depth_sum / q->stats.rx_attempts : 0);
		}
	}
	seq_printf(seq, "---------------------------------------------------------------------------------------------------\n");
	seq_printf(seq, "     |  Token   |    QSM   |    Num   |   Avail  |   Low    |    Hi    |  Timeout |   Start  |\n");
	seq_printf(seq, " DQM |   Size   | Allocated|   Token  |   Token  | Water Mrk| Water Mrk|   (ns)   |   Addr   |\n");
	for (i = 0; i < qdev->q_count; i++) {
		q = &qdev->dqm[i];
		if ((q->flags & DQM_F_TX) || (q->flags & DQM_F_RX)) {
			seq_printf(seq, " %3d | %8d | %8d | %8d | %8d | %8d | %8d | %8d | 0x%06x |\n",
				   q->num, DQM_GET_TOK_SIZE(q) + 1,
				   DQM_GET_Q_SIZE(q),
				   q->depth, DQM_GET_Q_SPACE(q),
				   q->lwm, q->hwm, q->timeout,
				   DQM_GET_Q_ADDR(q));
		}
	}
	seq_printf(seq, "---------------------------------------------------------------------------------------------------\n");
	seq_printf(seq, "-------------------Monitoring-Running-%d-----------------------------------------------------------\n",
		   is_dqm_mon_running());
	seq_printf(seq, " DQM | %%hi %%lo | L | S |    Count   | Name\n");
	for (i = 0; i < qdev->q_count; i++) {
		q = &qdev->dqm[i];
		if ((q->flags & DQM_F_TX) || (q->flags & DQM_F_RX)) {
			seq_printf(seq, " %3d | %3d %3d | %d | %d | %10d | %-*s\n",
				   q->num, q->mon.hi_thresh,
				   q->mon.lo_thresh, q->mon.log,
				   q->mon.state, q->mon.ncnt,
				   (int)sizeof(q->name), q->name);
		}
	}
	seq_printf(seq, "---------------------------------------------------------------------------------------------------\n\n");
	pr_debug("<--\n");
	return 0;
}

static ssize_t dqm_rx_mode_read(struct file *f, char __user *buf,
				size_t size, loff_t *ppos)
{
	int num;

	if (*ppos)
		return 0;

	switch (dqm_get_rx_burst_mode()) {
		case BURST_MODE_NONE:
			strncpy(buf, "none\n", size);
			break;
		case BURST_MODE_ARM:
			strncpy(buf, "arm\n", size);
			break;
		case BURST_MODE_NEON:
			strncpy(buf, "neon\n", size);
			break;
	}
	num = strlen(buf);
	*ppos += strlen(buf);
	return num;
}

static ssize_t dqm_rx_mode_write(struct file *f, const char __user *buf,
				 size_t size, loff_t *ppos)
{
	if (strncmp(buf, "none", 4) == 0)
		dqm_set_rx_burst_mode(BURST_MODE_NONE);
	else if (strncmp(buf, "arm", 3) == 0)
		dqm_set_rx_burst_mode(BURST_MODE_ARM);
	else if (strncmp(buf, "neon", 4) == 0)
		dqm_set_rx_burst_mode(BURST_MODE_NEON);
	*ppos += size;
	return size;
}

static ssize_t dqm_tx_mode_read(struct file *f, char __user *buf,
				size_t size, loff_t *ppos)
{
	int num;

	if (*ppos)
		return 0;

	switch (dqm_get_tx_burst_mode()) {
		case BURST_MODE_NONE:
			strncpy(buf, "none\n", size);
			break;
		case BURST_MODE_ARM:
			strncpy(buf, "arm\n", size);
			break;
		case BURST_MODE_NEON:
			strncpy(buf, "neon\n", size);
			break;
	}
	num = strlen(buf);
	*ppos += strlen(buf);
	return num;
}

static ssize_t dqm_tx_mode_write(struct file *f, const char __user *buf,
				 size_t size, loff_t *ppos)
{
	if (strncmp(buf, "none", 4) == 0)
		dqm_set_tx_burst_mode(BURST_MODE_NONE);
	else if (strncmp(buf, "arm", 3) == 0)
		dqm_set_tx_burst_mode(BURST_MODE_ARM);
	else if (strncmp(buf, "neon", 4) == 0)
		dqm_set_tx_burst_mode(BURST_MODE_NEON);
	*ppos += size;
	return size;
}

