/****************************************************************************
 *
 * Broadcom Proprietary and Confidential.
 * (c) 2016 Broadcom. All rights reserved.
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
 ****************************************************************************
 * Author: Tim Ross <tross@broadcom.com>
 * Author: Jayesh Patel <jayesh.patel@broadcom.com>
 *****************************************************************************/
#ifndef _DQM_H_
#define _DQM_H_

#include <linux/types.h>
#include <linux/irqreturn.h>
#include <linux/slab.h>

#include <msgfifo.h>

#define DQM_F_TX	0x00000001	/* DQM TX client or callback */
#define DQM_F_RX	0x00000002	/* DQM RX client or callback */
typedef irqreturn_t (*dqm_isr_callback)(void *q_h, void *context, u32 flags);
struct dqm_cb {
	dqm_isr_callback fn;	/* callback for ISR's */
	void *context;		/* user provided context */
	char *name;             /* user name */
};

void *dqm_register(char *dqmdev_name, u8 qnum, struct dqm_cb *cb,
		   u8 *msg_size, u32 flags);
int dqm_release(void *q_h, u32 flags);
int dqm_enable_rx_cb(void *q_h);
int dqm_disable_rx_cb(void *q_h);
int dqm_rx(void *q_h, int count, struct msgstruct *msg);
int dqm_empty(void *q_h);
int dqm_enable_tx_cb(void *q_h);
int dqm_disable_tx_cb(void *q_h);
int dqm_tx(void *q_h, struct msgstruct *msg);
int dqm_depth(void *q_h);
int dqm_space(void *q_h);

enum dqm_event {
	DQM_EVENT_EMPTY,
	DQM_EVENT_LOW,
	DQM_EVENT_HIGH,
	DQM_EVENT_FULL,
};

struct dqm_mon_info {
	const char *devname;
	const char *qname;
	u32     event;
	u32     q_num;
	u32     depth;
	u32     space;
	u32     thresh;
};

typedef int (*dqm_mon_callback)(struct dqm_mon_info *info, void *context);
struct dqm_mon_cb {
	dqm_mon_callback fn;	/* callback for notification */
	void *context;		/* user provided context */
};
int dqm_mon(char *dqmdev_name, u8 qnum, int hi_thresh, int lo_thresh,
	    int log, struct dqm_mon_cb *cb);
int dqm_mon_start(int start);

#endif
