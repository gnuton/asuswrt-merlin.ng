/****************************************************************************
 *
 * Broadcom Proprietary and Confidential.
 * (c) 2018 Broadcom. All rights reserved.
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
#ifndef _DQM_DEV_H_
#define _DQM_DEV_H_

#include <linux/types.h>

#include "dqm.h"
#include "dqm_hw.h"
#include <linux/platform_device.h>
#include <msgfifo.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/irqchip.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/of_irq.h>
#include <proc_cmd.h>

#define MODULE_NAME	"brcm-dqm"
#define MODULE_VER	"3.0"

struct dqm_ops {
	int (*enable_ne)(void *q_h);
	int (*enable_lwm)(void *q_h);
	int (*enable_hwm)(void *q_h);
	int (*enable_tmr)(void *q_h);
	int (*disable_ne)(void *q_h);
	int (*disable_lwm)(void *q_h);
	int (*disable_hwm)(void *q_h);
	int (*disable_tmr)(void *q_h);
	int (*clear_status_ne)(void *q_h);
	int (*clear_status_lwm)(void *q_h);
	int (*clear_status_hwm)(void *q_h);
	int (*clear_status_tmr)(void *q_h);
	irqreturn_t (*do_irq)(int irq, void *dev);
};

/* Stats collected for all the queues */
struct dqm_stats {
	unsigned int tx_cnt;		/* # of messages transmitted */
	unsigned int rx_cnt;		/* # of messages received */
	unsigned int tx_attempts;	/* # of attempts to TX a msg */
	unsigned int rx_attempts;	/* # of attempts to RX a msg */
	unsigned int rx_q_depth_peak;	/* max # of messages in queue on RX */
	unsigned int tx_q_depth_peak;	/* max # of messages in queue on TX */
	unsigned int rx_q_depth_sum;	/* sum of RX q depth samplings */
	unsigned int tx_q_depth_sum;	/* sum of TX q depth samplings */
};

#define DQM_NAME_SIZE 33

struct _qsm_alloc {
	char name[DQM_NAME_SIZE];
	bool dt_valid;
	u32 msg_size;
	u32 depth;
	bool offload;
	u32 lwm;
	u32 hwm;
	u32 timeout;
};

#define DQM_INTC_LEGACY 1
#define DQM_INTC_LIVE   2
#define DQM_INTC_EXT    3
#define DQM_INT_BANKS   3

struct dqm_intc_reg {
	struct dqm_intc *intc;
	u32 *l1_irq_mask;
	u32 *l1_irq_status;
	u32 *l1_irq_type;
	u32 l1_irq_dqm_mask;
	u32 *lwm_irq_mask;
	u32 *lwm_irq_mask_set;
	u32 *lwm_irq_mask_clr;
	u32 *lwm_irq_status;
	u32 *hwm_irq_mask;
	u32 *hwm_irq_mask_set;
	u32 *hwm_irq_mask_clr;
	u32 *hwm_irq_status;
	u32 *tmr_irq_mask;
	u32 *tmr_irq_mask_set;
	u32 *tmr_irq_mask_clr;
	u32 *tmr_irq_status;
	u32 *ne_irq_mask;
	u32 *ne_irq_mask_set;
	u32 *ne_irq_mask_clr;
	u32 *ne_irq_status;
	u32 *ne_irq_status_set;
	u32 *ne_irq_status_clr;
	u32 *ne_status;
};

struct dqm_intc {
	struct dqmdev *qdev;
	struct irq_domain *domain;
	struct irq_chip   irq_chip;
	u32 type;
	u32 irq;
	u32 banks;
	struct dqm_intc_reg reg[DQM_INT_BANKS];
};

struct dqm_mon {
	u8  hi_thresh;
	u8  lo_thresh;
	u32 space_hi;
	u32 space_lo;
	u32 state;
	u32 log;
	u32 ncnt;
	struct dqm_mon_cb cb;
};

struct dqm {
	struct dqmdev *dqmdev;
	u32 flags;
	u32 num;
	u32 type;
	u32 q_bit_mask;
	u32 irq;
	u32 irq_requested;
	u32 hwm_clr_bit_val;
	u32 lwm_clr_bit_val;
	u32 tmr_clr_bit_val;
	u32 ne_clr_bit_val;
	u32 lwm_bit_mask;
	u32 hwm_bit_mask;
	u32 tmr_bit_mask;
	u32 ne_bit_mask;
	struct dqm_cb tx_cb;
	struct dqm_cb rx_cb;
	struct dqm_stats stats;
	struct dqmctl *ctl;
	struct dqmtmr *tmr;
	u32 *data;
	u32 *status;
	u32 *mib;
	u32 cntlwm;
	u32 cnthwm;
	u32 cnttmr;
	u32 cntne;
	u32 msg_size;
	u32 depth;
	u32 lwm;
	u32 hwm;
	u32 timeout;
	spinlock_t lock;
	char name[DQM_NAME_SIZE];
	bool offload;
	struct dqm_ops *ops;
	void (*tx)(struct dqm *q, u32 *data);
	int (*rx)(void *q_h, int count, struct msgstruct *msg);
	struct dqm_intc_reg *intcreg;
	struct dqm_mon mon;
};

struct dqmdev {
	struct list_head list;

	struct platform_device *pdev;

	char name[16];
	u32 irq;
	u32 *reg_base;
	u32 *data_base;
	u64 data_phys_base;

	u32 bank_count;
	struct dqmtok *tok_base;

	u32 **cfg;
	u32 **fpm_alloc;

	struct dqmctl **q_ctl_base;
	struct dqmtmr **q_tmr_base;
	u32 **q_data_base;
	u32 **q_status_base;
	u32 **q_mib_base;

	u32 q_count;
	u32 qsm_size;
	bool cfg_qsm;
	bool offload;
	bool restricted_access;

	bool    probe_defer;
	u32     defer_offset;
	u32     defer_mask;
	u32     defer_value;
	u32     *probe_defer_register;
	struct dqm *dqm;
	u32 intc_count;
	struct dqm_intc *intc;

	struct proc_dir_entry *proc_dir;
	struct proc_dir_entry *status_proc_file;
	struct proc_dir_entry *cmd_proc_file;
	struct proc_cmd_table proc_cmd;
};

int is_dqm_mon_running(void);
int dqm_mon_init(void);
int dqm_mon_exit(void);

#endif
