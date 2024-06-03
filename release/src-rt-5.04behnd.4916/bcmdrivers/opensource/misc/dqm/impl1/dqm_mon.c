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
 * Author: Jayesh Patel <jayesh.patel@broadcom.com>
 ****************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include "dqm_dev.h"
#include "dqm_dbg.h"

static struct delayed_work mon_dwork;
static u32 enable;

static int dqm_mon_process(struct dqmdev *qdev)
{
	int i;
	struct dqm *q;
	struct dqm_mon_info info;

	for (i = 0; i < qdev->q_count; i++) {
		q = &qdev->dqm[i];
		if (!q->mon.hi_thresh)
			continue;
		info.space = DQM_GET_Q_SPACE(q);

		if (info.space < q->mon.space_hi) {
			if (!q->mon.state) {
				info.devname = q->dqmdev->name;
				info.qname = q->name;
				info.depth = q->depth;
				info.q_num = q->num;
				if (info.space == 0) {
					info.event = DQM_EVENT_FULL;
					info.thresh = 100;
				} else {
					info.event = DQM_EVENT_HIGH;
					info.thresh = q->mon.hi_thresh;
				}
				if (q->mon.cb.fn)
					q->mon.cb.fn(&info, q->mon.cb.context);
				if (q->mon.log)
					pr_alert("DQM Mon: Alert %s:%s space %d available of %d thresh %d%%\n",
						 qdev->name, q->name,
						 info.space, q->depth,
						 info.thresh);
				q->mon.state = 1;
				q->mon.ncnt++;
			}
		} else if (info.space > q->mon.space_lo) {
			if (q->mon.state) {
				info.devname = q->dqmdev->name;
				info.qname = q->name;
				info.depth = q->depth;
				info.q_num = q->num;
				if (info.space == q->depth) {
					info.event = DQM_EVENT_EMPTY;
					info.thresh = 0;
				} else {
					info.event = DQM_EVENT_LOW;
					info.thresh = q->mon.lo_thresh;
				}
				if (q->mon.cb.fn)
					q->mon.cb.fn(&info, q->mon.cb.context);
				if (q->mon.log)
					pr_alert("DQM Mon: Clear %s:%s space %d available of %d thresh %d%%\n",
						 qdev->name, q->name,
						 info.space, q->depth,
						 info.thresh);
				q->mon.state = 0;
			}
		}
	}
	return 0;
}

static int mon_work_start(struct delayed_work *dwork)
{
	unsigned long sec = msecs_to_jiffies(1 * 1000);

	if (sec)
		queue_delayed_work(system_highpri_wq, dwork, sec);
	return 0;
}

static int mon_work_stop(struct delayed_work *dwork)
{
	cancel_delayed_work(dwork);
	flush_delayed_work(dwork);
	return 0;
}

static void mon_work_handler(struct work_struct *work)
{
	struct dqmdev *qdev;

	list_for_each_entry(qdev, &dqmdevs, list)
		dqm_mon_process(qdev);
	mon_work_start(&mon_dwork);
}

static int mon_work_init(struct delayed_work *dwork)
{
	INIT_DELAYED_WORK(dwork, mon_work_handler);
	return 0;
}

static void mon_work_exit(struct delayed_work *dwork)
{
	flush_work(&dwork->work);
}

int dqm_mon_init(void)
{
	mon_work_init(&mon_dwork);
	return 0;
}

int dqm_mon_exit(void)
{
	mon_work_stop(&mon_dwork);
	mon_work_exit(&mon_dwork);
	return 0;
}

int is_dqm_mon_running(void)
{
	return enable;
}

int dqm_mon_start(int start)
{
	if (start) {
		mon_work_start(&mon_dwork);
		pr_alert("DQM: Monitoring Thread Started\n");
		enable = 1;
	} else {
		mon_work_stop(&mon_dwork);
		pr_alert("DQM: Monitoring Thread Stopped\n");
		enable = 0;
	}
	return 0;
}
EXPORT_SYMBOL(dqm_mon_start);
