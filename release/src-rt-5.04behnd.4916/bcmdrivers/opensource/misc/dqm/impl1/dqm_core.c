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
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/reboot.h>
#include <linux/bitops.h>

#include "dqm.h"
#include "dqm_dev.h"
#include "dqm_dt.h"
#include "dqm_hw.h"
#include "dqm_dbg.h"
#include "fpm.h"

#if defined(CONFIG_ARM)
#define DQM_ENABLE_HWM_LWM_TRM 1
#endif

static const struct of_device_id dqm_of_match[] = {
	{.compatible = "brcm,dqm"},
	{}
};

MODULE_DEVICE_TABLE(of, dqm_of_match);

static int dqm_probe(struct platform_device *pdev);
static int dqm_remove(struct platform_device *pdev);
static struct platform_driver dqm_driver = {
	.probe	= dqm_probe,
	.remove	= dqm_remove,
	.driver = {
		.name		= MODULE_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= dqm_of_match
	}
};

struct list_head dqmdevs;

static void dqm_legacy_tx(struct dqm *q, u32 *data)
{
	int i;

	for (i = 0; i < q->msg_size; i++)
		dqm_reg_write(&q->data[i], data[i]);
}

#if defined(CONFIG_ARM64) && defined(CONFIG_KERNEL_MODE_NEON)

/* rx */

static int dqm_neon_rx_1(u32 *reg, int count, struct msgstruct *msg)
{
	return _dqm_neon_rx_1(reg, count, msg);
}

static int dqm_neon_rx_2(u32 *reg, int count, struct msgstruct *msg)
{
	return _dqm_neon_rx_2(reg, count, msg);
}

static int dqm_neon_rx_3(u32 *reg, int count, struct msgstruct *msg)
{
	return _dqm_neon_rx_3(reg, count, msg);
}

static int dqm_neon_rx_4(u32 *reg, int count, struct msgstruct *msg)
{
	return _dqm_neon_rx_4x1(reg, count, msg);
}

int dqm_neon_rx(u32 *reg, int msgsz, int count, struct msgstruct *msg)
{
	int num_read = 0;

	switch (msgsz) {
		case 1:
			num_read = dqm_neon_rx_1(reg, count, msg);
			break;
		case 2:
			num_read = dqm_neon_rx_2(reg, count, msg);
			break;
		case 3:
			num_read = dqm_neon_rx_3(reg, count, msg);
			break;
		case 4:
			num_read = dqm_neon_rx_4(reg, count, msg);
			break;
	}
	dqm_kernel_neon_end();
	return num_read;
}

int dqm_arm_rx_2(u32 *reg, int count, struct msgstruct *msg)
{
	return _dqm_arm_rx_2(reg, count, msg);
}

int dqm_arm_rx_4(u32 *reg, int count, struct msgstruct *msg)
{
	return _dqm_arm_rx_4(reg, count, msg);
}

int dqm_arm_rx(u32 *reg, int msgsz, int count, struct msgstruct *msg)
{
	if (msgsz == 4)
		return dqm_arm_rx_4(reg, count, msg);
	if (msgsz == 2)
		return dqm_arm_rx_2(reg, count, msg);
	return dqm_noburst_rx(reg, msgsz, count, msg);
}

static int (*dqm_rx_ptr)(u32 *, int, int, struct msgstruct *) = dqm_arm_rx;

int dqm_burst_rx(void *q_h, int count, struct msgstruct *msg)
{
	int status = 0;
	struct dqm *q = q_h;
	unsigned long flags;

	if (unlikely(!msg)) {
		pr_err("Invalid message parameter - NULL pointer!\n");
		status = -EINVAL;
		goto done;
	}
	spin_lock_irqsave(&q->lock, flags);

	pr_debug("RX Addr: %08llx\n", (u64)q->dqmdev->data_phys_base + ((u64)q->data - (u64)q->dqmdev->data_base));
	count = dqm_rx_ptr(q->data, q->msg_size, count, msg);

	q->stats.rx_cnt += count;
	status = count;
	if (q->hwm > 1)
		q->ops->clear_status_hwm(q);
	else
		q->ops->clear_status_ne(q);
	if (q->timeout)
		q->ops->clear_status_tmr(q);
	q->stats.rx_attempts++;
	spin_unlock_irqrestore(&q->lock, flags);

done:
	return status;
}

int dqm_set_rx_burst_mode(int mode)
{
	switch (mode) {
		case BURST_MODE_NONE:
			dqm_rx_ptr = dqm_noburst_rx;
			break;
		case BURST_MODE_ARM:
			dqm_rx_ptr = dqm_arm_rx;
			break;
		case BURST_MODE_NEON:
			dqm_rx_ptr = dqm_neon_rx;
			break;
		default:
			return -EOPNOTSUPP;
	}
	return mode;
}

int dqm_get_rx_burst_mode(void)
{
	if (dqm_rx_ptr == dqm_noburst_rx)
		return BURST_MODE_NONE;
	if (dqm_rx_ptr == dqm_arm_rx)
		return BURST_MODE_ARM;
	if (dqm_rx_ptr == dqm_neon_rx)
		return BURST_MODE_NEON;
	return -EOPNOTSUPP;
}

/* tx */

void dqm_neon_tx_4(u32 *reg, u32 *data)
{
	_dqm_neon_tx_4(reg, data);
}

void dqm_arm_tx_4(u32 *reg, u32 *data)
{
	_dqm_arm_tx_4(reg, data);
}

void dqm_noburst_tx_4(u32 *reg, u32 *data)
{
	dqm_reg_write(reg++, *data++);
	dqm_reg_write(reg++, *data++);
	dqm_reg_write(reg++, *data++);
	dqm_reg_write(reg++, *data++);
}

void dqm_burst_tx_2(struct dqm *q, u32 *data)
{
	u64 *r = (u64 *)q->data;
	u64 *d = (u64 *)data;

	*r = *d;
}

void (*dqm_tx_ptr)(u32 *reg, u32 *data) = dqm_arm_tx_4;

void dqm_burst_tx_4(struct dqm *q, u32 *data)
{
	dqm_tx_ptr(q->data, data);
}

int dqm_set_tx_burst_mode(int mode)
{
	switch (mode) {
		case BURST_MODE_NONE:
			dqm_tx_ptr = dqm_noburst_tx_4;
			break;
		case BURST_MODE_ARM:
			dqm_tx_ptr = dqm_arm_tx_4;
			break;
		case BURST_MODE_NEON:
			dqm_tx_ptr = dqm_neon_tx_4;
			break;
		default:
			return -EOPNOTSUPP;
	}
	return mode;
}

int dqm_get_tx_burst_mode(void)
{
	if (dqm_tx_ptr == dqm_noburst_tx_4)
		return BURST_MODE_NONE;
	if (dqm_tx_ptr == dqm_arm_tx_4)
		return BURST_MODE_ARM;
	if (dqm_tx_ptr == dqm_neon_tx_4)
		return BURST_MODE_NEON;
	return -EOPNOTSUPP;
}

#else

#define dqm_burst_rx	 	dqm_legacy_rx
#define dqm_burst_tx_2		dqm_legacy_tx
#define dqm_burst_tx_4		dqm_legacy_tx

int dqm_set_rx_burst_mode(int mode)
{
	return -EOPNOTSUPP;
}

int dqm_get_rx_burst_mode(void)
{
	return -EOPNOTSUPP;
}

int dqm_set_tx_burst_mode(int mode)
{
	return -EOPNOTSUPP;
}

int dqm_get_tx_burst_mode(void)
{
	return -EOPNOTSUPP;
}

#endif

/*
 * Register to use a DQM.
 *
 * A DQM can only be used by one transmitter (DQM_F_TX) and
 * one receiver (DQM_F_RX) at a time. A DQM is typically only used
 * for tranmitting to another CPU or receiving from another CPU so it
 * is unusual for any one DQM to be registered for transmitting to
 * and receiving from by the same CPU. However, this driver does allow this
 * to be done for the primary purpose of testing.
 *
 * The transmitter and receiver can specify independent callbacks
 * by calling this function once with DQM_F_TX and again with
 * DQM_F_RX. If called with both DQM_F_TX and DQM_F_RX flags set
 * then only a single callback can be specified, in which case
 * the callback function's flags parameter can be used to determine
 * the reason for the callback.
 *
 * The DQM will not be emptied on registration. It is up to the
 * client to explicitly empty the DQM upon registering or
 * releasing.
 *
 * Callbacks for the DQM are not enabled here. They must be
 * explicitly enabled by the client via dqm_enable_rx_cb() or
 * dqm_enable_tx_cb().
 *
 * dqmdev_name	NULL-terminated string containing the name
 *		of the DQM device to be used. This name
 *		must match the name of a DQM device that
 *		was defined in the DeviceTree
 * qnum		0-based index of the DQM to be used
 * cb		pointer to function callback pointer and caller's
 *		context. May be NULL if no CB is desired.
 * msg_size	pointer to return the DQM's message size that
 *		was configured during dqm_probe or by another
 *		CPU
 * flags	must specify DQM_F_TX, DQM_F_RX, or both
 *
 * returns dqm handle (void *) on success, NULL on failure
 */
void *dqm_register(char *dqmdev_name, u8 qnum, struct dqm_cb *cb,
		   u8 *msg_size, u32 flags)
{
	struct dqm *q = NULL;
	struct dqmdev *qdev;
	bool found = false;
	bool tx = flags & DQM_F_TX;
	bool rx = flags & DQM_F_RX;

	if (!dqmdev_name || !msg_size ||
	    (!(flags & DQM_F_TX) && !(flags & DQM_F_RX))) {
		pr_err("Invalid parameter!\n");
		goto done;
	}

	pr_debug("%s DQM %d on device %s\n", __func__, qnum, dqmdev_name);

	found = false;
	rcu_read_lock();
	list_for_each_entry_rcu(qdev, &dqmdevs, list) {
		if (!strncmp(qdev->name, dqmdev_name, sizeof(qdev->name))) {
			found = true;
			break;
		}
	}

	rcu_read_unlock();
	if (!found) {
		pr_err("DQM device %s not found!\n", dqmdev_name);
		goto done;
	}

	if (qnum >= qdev->q_count) {
		pr_err("DQM %d does not exist on device %s!\n", qnum,
		       dqmdev_name);
		goto done;
	}

	if (!qdev->restricted_access &&
		DQM_GET_Q_SIZE(&qdev->dqm[qnum]) == 0) {
		pr_debug("DQM #%d on device %s not initialized!\n", qnum,
			 dqmdev_name);
		goto done;
	}

	q = &qdev->dqm[qnum];
	if (tx && (q->flags & DQM_F_TX)) {
		pr_err("Transmitter already registered for DQM %d on device %s!\n",
		       qnum, dqmdev_name);
		q = NULL;
		goto done;
	}
	if (rx && (q->flags & DQM_F_RX)) {
		pr_err("Receiver already registered for DQM %d on device %s!\n",
		       qnum, dqmdev_name);
		q = NULL;
		goto done;
	}

	memset(&q->stats, 0, sizeof(struct dqm_stats));
	if (tx) {
		q->tx_cb.context = cb ? cb->context : NULL;
		q->tx_cb.fn = cb ? cb->fn : NULL;
	}
	if (rx) {
		q->rx_cb.context = cb ? cb->context : NULL;
		q->rx_cb.fn = cb ? cb->fn : NULL;
	}
	q->flags |= flags;
	if (!q->dqmdev->restricted_access) {
		q->depth = DQM_GET_NUM_TOK(q);
		q->msg_size = DQM_GET_TOK_SIZE(q) + 1;
	}
	if (q->msg_size == 4)
		q->tx = dqm_burst_tx_4;
	else if (q->msg_size == 2)
		q->tx = dqm_burst_tx_2;
	else
		q->tx = dqm_legacy_tx;
	*msg_size = q->msg_size;

	pr_debug("%s Qnum %d msg_size %d on device %s\n", __func__,
		 qnum, *msg_size, dqmdev_name);

	if (q->irq) {
		if (q->irq_requested) {
			enable_irq(q->irq);
			goto done;
		}
		/* Using individual irq */
		if (request_irq(q->irq,
				q->ops->do_irq, IRQF_SHARED,
				q->name, q)) {
			q = NULL;
			goto done;
		}
		q->irq_requested = 1;
	}

	if (cb && cb->name) {
		if (q->flags & DQM_F_TX)
			snprintf(q->name, DQM_NAME_SIZE-1, "Tx-%s", cb->name);
		else if (q->flags & DQM_F_RX)
			snprintf(q->name, DQM_NAME_SIZE-1, "Rx-%s", cb->name);
	}

done:
	if (tx)
		pr_debug("<-- %s transmitter (%px) on DQM #%d on device %s\n",
			 q ? "Registered" : "Failed to register", q,
			 qnum, dqmdev_name);
	if (rx)
		pr_debug("<-- %s receiver (%px) on DQM #%d on device %s\n",
			 q ? "Registered" : "Failed to register", q,
			 qnum, dqmdev_name);
	return q;
}
EXPORT_SYMBOL(dqm_register);

/*
 * Release a DQM which was previously registered.
 *
 * Callbacks will be disabled but the DQM will not be
 * emptied. It is up to the DQM client to
 * explicitly empty the DQM.
 *
 * q_h		the handle returned from dqm_register
 * flags	must specify DQM_F_TX, DQM_F_RX, or both
 *
 * returns success (0) or failure (< 0)
 */
int dqm_release(void *q_h, u32 flags)
{
	int status = 0;
	struct dqmdev *qdev;
	struct dqm *q = q_h;
	bool tx = flags & DQM_F_TX;
	bool rx = flags & DQM_F_RX;

	if (!tx && !rx) {
		pr_err("Invalid parameter!\n");
		status = -EINVAL;
		goto done;
	}
	qdev = q->dqmdev;
	if (tx) {
		pr_debug("--> Releasing transmitter on DQM %d on device %s\n",
			 q->num, qdev->name);
		dqm_disable_tx_cb(q);
		q->flags &= ~DQM_F_TX;
	}
	if (rx) {
		pr_debug("--> Releasing receiver on DQM %d on device %s\n",
			 q->num, qdev->name);
		dqm_disable_rx_cb(q);
		q->flags &= ~DQM_F_RX;
	}

	if (q->irq) {
		/* Using individual irq */
		disable_irq(q->irq);
	}
done:
	pr_debug("<--\n");
	return status;
}
EXPORT_SYMBOL(dqm_release);

/*
 * Enable receive callbacks.
 *
 * q_h	the handle returned from dqm_register
 *
 * returns success (0) or failure (< 0)
 */
int dqm_enable_rx_cb(void *q_h)
{
	struct dqm *q = q_h;

	if (!q->irq)
		return 0;

	if (q->hwm > 1 && q->intcreg->hwm_irq_mask) {
		if (q->intcreg->hwm_irq_status) {
			q->ops->clear_status_hwm(q);
			q->ops->enable_hwm(q);
		}
	} else {
		if (q->intcreg->ne_irq_status && q->intcreg->ne_irq_mask) {
			q->ops->clear_status_ne(q);
			q->ops->enable_ne(q);
		}
	}
	if (q->timeout) {
		if (q->intcreg->tmr_irq_status && q->intcreg->tmr_irq_mask) {
			q->ops->clear_status_tmr(q);
			q->ops->enable_tmr(q);
		}
	}

	return 0;
}
EXPORT_SYMBOL(dqm_enable_rx_cb);

/*
 * Disable receive callbacks.
 *
 * q_h	the handle returned from dqm_register
 *
 * returns success (0) or failure (< 0)
 */
int dqm_disable_rx_cb(void *q_h)
{
	struct dqm *q = q_h;

	if (!q->irq)
		return 0;

	if (q->hwm > 1 && q->intcreg->hwm_irq_mask)
		q->ops->disable_hwm(q);
	else
		if (q->intcreg->ne_irq_mask)
			q->ops->disable_ne(q);
	if (q->timeout)
		if (q->intcreg->tmr_irq_mask)
			q->ops->disable_tmr(q);

	return 0;
}
EXPORT_SYMBOL(dqm_disable_rx_cb);

/*
 * Enable transmit callbacks.
 *
 * q_h	the handle returned from dqm_register
 *
 * returns success (0) or failure (< 0)
 */
int dqm_enable_tx_cb(void *q_h)
{
	struct dqm *q = q_h;
	u32 ne;
	unsigned long flags;

	if (!q->irq)
		return 0;

	/*
	 * The LWM is only evaluated on pushes & pops so by the time we
	 * get here the Q may have already been drained which would mean
	 * we wouldn't ever get a LWM IRQ. So in addition to enabling the
	 * LWM IRQ we check the NE status. If the Q is empty we invoke the
	 * CB directly and disable the LWM IRQ to prevent invoking the CB
	 * more than once. IRQs are disabled during this to prevent a LWM
	 * IRQ from firing between enabling it and checking the NE status.
	 */
	local_irq_save(flags);
	q->ops->clear_status_lwm(q);
	q->ops->enable_lwm(q);
	ne = dqm_reg_read(q->intcreg->ne_status);
	if (!(ne & q->ne_bit_mask) && q->tx_cb.fn && (q->flags & DQM_F_TX)) {
		(*q->tx_cb.fn)(q, q->tx_cb.context, DQM_F_TX);
		q->ops->disable_lwm(q);
	}
	local_irq_restore(flags);

	return 0;
}
EXPORT_SYMBOL(dqm_enable_tx_cb);

/*
 * Disable receive callbacks.
 *
 * q_h	the handle returned from dqm_register
 *
 * returns success (0) or failure (< 0)
 */
int dqm_disable_tx_cb(void *q_h)
{
	struct dqm *q = q_h;

	if (!q->irq)
		return 0;

	q->ops->disable_lwm(q);

	return 0;
}
EXPORT_SYMBOL(dqm_disable_tx_cb);

/*
 * Read messages from a DQM.
 *
 * Callable from either thread or ISR context.
 *
 * q_h	the handle returned from dqm_register
 * cnt	# of messages to read
 * msg	Storage for the messages to be read from the DQM.
 *	The # of words/message that was specified when
 *      the DQM was configured will be read from the DQM.
 *
 * returns	the number of messages read (0 == no messages present),
 *		or failure (< 0)
 */
int dqm_legacy_rx(void *q_h, int count, struct msgstruct *msg)
{
	int status = 0;
	struct dqm *q = q_h;
	u32 occupied;
	unsigned long flags;

	if (unlikely(!msg)) {
		pr_err("Invalid message parameter - NULL pointer!\n");
		status = -EINVAL;
		goto done;
	}

	occupied = q->depth - DQM_GET_Q_SPACE(q);
	pr_debug("%s %d (%s) offload %d depth %d qspace %d occupied %d\n", __func__,
		 q->num, q->name, q->offload, q->depth, DQM_GET_Q_SPACE(q), occupied);
	if (q->type != DQM_INTC_LEGACY)
		occupied /= q->msg_size;
	if (!occupied)
		return 0;

	spin_lock_irqsave(&q->lock, flags);
	if (unlikely(occupied > q->stats.rx_q_depth_peak)) {
		pr_debug("New RX peak for DQM #%d on device %s\n",
			   q->num, q->dqmdev->name);
		q->stats.rx_q_depth_peak = occupied;
	}
	q->stats.rx_q_depth_sum += occupied;
	if (count > occupied)
		count = occupied;

	dqm_rx_msgs(q->data, q->msg_size, count, msg);

	q->stats.rx_cnt += count;
	status = count;
	if (q->hwm > 1 && q->intcreg->hwm_irq_mask)
		q->ops->clear_status_hwm(q);
	else
		q->ops->clear_status_ne(q);
	if (q->timeout)
		q->ops->clear_status_tmr(q);
	q->stats.rx_attempts++;
	spin_unlock_irqrestore(&q->lock, flags);

done:
	return status;
}

/*
 * Read messages from a DQM.
 *
 * Callable from either thread or ISR context.
 *
 * q_h	the handle returned from dqm_register
 * cnt	# of messages to read
 * msg	Storage for the messages to be read from the DQM.
 *	The # of words/message that was specified when
 *      the DQM was configured will be read from the DQM.
 *
 * returns	the number of messages read (0 == no messages present),
 *		or failure (< 0)
 */
int dqm_rx(void *q_h, int count, struct msgstruct *msg)
{
	struct dqm *q = q_h;
	return q->rx(q_h, count, msg);
}
EXPORT_SYMBOL(dqm_rx);

/*
 * Get the configured depth of the DQM.
 *
 * Callable from either thread or ISR context.
 *
 * q_h	the handle returned from dqm_register
 *
 * returns the configured depth of the DQM
 */
int dqm_depth(void *q_h)
{
	struct dqm *q = q_h;

	pr_debug("--> DQM %d on device %s\n", q->num, q->dqmdev->name);

	return q->depth;
}
EXPORT_SYMBOL(dqm_depth);

/*
 * Get the space available in the DQM.
 *
 * Callable from either thread or ISR context.
 *
 * q_h	the handle returned from dqm_register
 *
 * returns the space available in the DQM
 */
int dqm_space(void *q_h)
{
	struct dqm *q = q_h;
	unsigned long flags;
	u32 space;

	pr_debug("--> DQM %d on device %s\n", q->num, q->dqmdev->name);

	spin_lock_irqsave(&q->lock, flags);
	space = DQM_GET_Q_SPACE(q);
	spin_unlock_irqrestore(&q->lock, flags);
	return space;
}
EXPORT_SYMBOL(dqm_space);


/*
 *
 * Write a single message to a DQM.
 *
 * q_h	the handle returned from dqm_register
 * msg	Message to be written to the DQM. The msgLen
 *	field of this structure is ignored. The number of
 *      words that will be written to the DQM is always
 *      the # of words/message that was specified when
 *	the DQM was configured.
 *
 * returns success (0) or failure (< 0)
 */
int dqm_tx(void *q_h, struct msgstruct *msg)
{
	int status = 0;
	struct dqm *q = q_h;
	unsigned long flags;
	u32 space, occupied;

	if (unlikely(!msg)) {
		pr_err("Invalid message parameter - NULL pointer!\n");
		status = -EINVAL;
		goto done;
	}
	pr_debug("dqm %d: ctl %px data %px status %px\n", q->num, q->ctl, q->data, q->status);

	spin_lock_irqsave(&q->lock, flags);
	space = DQM_GET_Q_SPACE(q);
	occupied = q->depth - space;

	if (likely(space)) {
		q->tx(q, &msg->msgdata[0]);
		q->stats.tx_cnt++;
		occupied++;
		if (occupied > q->stats.tx_q_depth_peak) {
			pr_debug("New TX peak for DQM #%d on device %s\n",
				   q->num, q->dqmdev->name);
			q->stats.tx_q_depth_peak = occupied;
		}
	} else {
		pr_debug("No space available for TX on DQM %d ", q->num);
		pr_debug("on device %s\n", q->dqmdev->name);
		status = -EAGAIN;
	}
	q->stats.tx_attempts++;
	q->stats.tx_q_depth_sum += occupied;
	spin_unlock_irqrestore(&q->lock, flags);

done:
	return status;
}
EXPORT_SYMBOL(dqm_tx);

 /*
  * Empties a DQM of all messages it may contain.
  *
  * This should be used when initializing a DQM for communication between a
  * transmitter and receiver. If a transmitter is writing to the DQM when
  * this is called, only the messages currently in the DQM will be emptied.
  *
  * q_h	the handle returned from dqm_register
  *
  * returns success (0) or failure (< 0)
  */
int dqm_empty(void *q_h)
{
	struct dqm *q = q_h;
	unsigned long flags;
	u32 max_occupied;
	int i;

	pr_debug("--> DQM %d on device %s\n", q->num, q->dqmdev->name);

	spin_lock_irqsave(&q->lock, flags);
	max_occupied = q->depth;

	while ((dqm_reg_read(q->intcreg->ne_status) &
		q->ne_bit_mask) && max_occupied--)
		for (i = 0; i < q->msg_size; i++)
			dqm_reg_read(&q->data[i]);
	if (q->hwm > 1 && q->intcreg->hwm_irq_mask)
		q->ops->clear_status_hwm(q);
	else
		q->ops->clear_status_ne(q);
	if (q->timeout)
		q->ops->clear_status_tmr(q);
	spin_unlock_irqrestore(&q->lock, flags);

	pr_debug("<--\n");
	return 0;
}
EXPORT_SYMBOL(dqm_empty);

/*
 * Interrupt handler for DQM devices.
 *
 * RX: identify any masked DQM's which are not empty, and
 * call associated RX callback.
 *
 * TX: if any masked DQM's low mark status are set, call associated
 * TX callback.
 *
 * IOP hardware (DQM's, mailboxes, etc) is split amongst multiple drivers and
 * thus the interrupts must be shared. So, we must make sure it's a DQM IRQ
 * so we can share the IRQ with other non-DQM IOP drivers.
 *
 * irq		interrupt line for DQM device
 * dev		ptr to struct dqmdev
 */
static irqreturn_t dqm_irq(int irq, void *dev)
{
	struct dqm *q = dev;

	pr_debug("%s %s(%d) status %d depth %d\n", __func__,
		 q->name, q->num, DQM_GET_Q_SPACE(q), q->depth);

	if (q->rx_cb.fn && (q->flags & DQM_F_RX))
		q->rx_cb.fn(q,
			    q->rx_cb.context,
			    DQM_F_RX);

	if (q->tx_cb.fn && (q->flags & DQM_F_TX))
		q->tx_cb.fn(q,
			    q->tx_cb.context,
			    DQM_F_TX);
	return IRQ_HANDLED;
}

static irqreturn_t dqm_live_irq(int irq, void *dev)
{
	irqreturn_t status = IRQ_NONE;
	struct dqm *q = dev;
	struct dqmdev *qdev;
	u32 rx_active, tx_active;
	u32 irq_status;

	if (unlikely(!q)) {
		pr_err("Spurious interrupt on NULL device!!!\n");
		status = IRQ_HANDLED;
		goto done;
	}

	qdev = q->dqmdev;

	irq_status = dqm_reg_read(q->intcreg->ne_irq_status);

	pr_debug("%s %s(%d) status %08x\n", __func__,
		 q->name, q->num, irq_status);

	if (irq_status) {
		status = IRQ_HANDLED;
		rx_active = irq_status & DQM_LIVE_IRQ_RX_MASK;
		if (rx_active && q->rx_cb.fn) {
			q->rx_cb.fn(q,
				    q->rx_cb.context,
				    DQM_F_RX);
			if (irq_status & DQM_LIVE_IRQ_NE_MASK)
				q->cntne++;
			if (irq_status & DQM_LIVE_IRQ_HWM_MASK)
				q->cnthwm++;
			if (irq_status & DQM_LIVE_IRQ_TMR_MASK)
				q->cnttmr++;
		}

		tx_active = irq_status & DQM_LIVE_IRQ_LWM_MASK;
		if (tx_active && q->tx_cb.fn) {
			q->tx_cb.fn(q,
				    q->tx_cb.context,
				    DQM_F_TX);
			q->cntlwm++;
		}
	}

done:
	return status;
}

int dqm_mon(char *dqmdev_name, u8 qnum, int hi_thresh, int lo_thresh,
	    int log, struct dqm_mon_cb *cb)
{
	int status = 0;
	struct dqmdev *qdev;
	struct dqm *q = NULL;
	bool found = false;

	if (!dqmdev_name) {
		pr_err("Invalid parameter - NULL pointer!\n");
		status = -EINVAL;
		goto done;
	}

	pr_debug("--> DQM #%d on device %s\n", qnum, dqmdev_name);

	found = false;
	rcu_read_lock();
	list_for_each_entry_rcu(qdev, &dqmdevs, list) {
		if (!strncmp(qdev->name, dqmdev_name, sizeof(qdev->name))) {
			found = true;
			break;
		}
	}

	rcu_read_unlock();
	if (!found) {
		pr_err("DQM device %s not found!\n", dqmdev_name);
		status = -EFAULT;
		goto done;
	}

	if (qnum >= qdev->q_count) {
		pr_err("DQM %d does not exist on device %s!\n", qnum,
		       dqmdev_name);
		status = -EFAULT;
		goto done;
	}

	q = &(qdev->dqm[qnum]);
	if (DQM_GET_Q_SIZE(q) == 0) {
		pr_debug("DQM #%d on device %s not initialized!\n", qnum,
			dqmdev_name);
		status = -EFAULT;
		goto done;
	}

	if (hi_thresh == 0)
		lo_thresh = 0;
	else if (hi_thresh <= lo_thresh) {
		pr_err("Invalid parameter hi_thresh %d > lo_thresh %d!\n",
		       hi_thresh, lo_thresh);
		status = -EINVAL;
		goto done;
	}

	pr_debug("--> DQM %d on device %s hi_thresh %d%% lo_thresh %d%%\n",
		 q->num, q->dqmdev->name, hi_thresh, lo_thresh);
	q->mon.hi_thresh = hi_thresh;
	q->mon.lo_thresh = lo_thresh;
	q->mon.space_hi = (q->depth * (100-hi_thresh)) / 100;
	q->mon.space_lo = (q->depth * (100-lo_thresh)) / 100;
	q->mon.log = log;
	q->mon.cb.context = cb ? cb->context : NULL;
	q->mon.cb.fn = cb ? cb->fn : NULL;

done:
	return status;
}
EXPORT_SYMBOL(dqm_mon);

/*
 * Module init.
 *
 * returns success (0) or failure (< 0)
 */
__init int dqm_init(void)
{
	pr_debug("-->\n");

	pr_debug("%s driver v%s\n", MODULE_NAME, MODULE_VER);

	INIT_LIST_HEAD_RCU(&dqmdevs);
	dqm_proc_init();
	dqm_mon_init();
	platform_driver_register(&dqm_driver);

	pr_debug("<--\n");
	return 0;
}

/*
 * Module exit.
 */
void dqm_exit(void)
{
	pr_debug("-->\n");

	platform_driver_unregister(&dqm_driver);
	dqm_mon_exit();
	dqm_proc_exit();

	pr_debug("<--\n");
}

static int dqm_enable_ne(void *q_h)
{
	struct dqm *q = q_h;

	if (q->intcreg->ne_irq_mask_set) {
		dqm_reg_write_mask(q->intcreg->ne_irq_mask_set,
				   q->ne_bit_mask, q->ne_bit_mask);
		pr_debug("%s %d reg: %px mask:0x%x val:0x%x status:0x%x mask:0x%x\n", __func__, q->num,
			 q->intcreg->ne_irq_mask_set, q->ne_bit_mask, q->ne_clr_bit_val,
			 *((u32 *)q->intcreg->ne_irq_status), *((u32 *)q->intcreg->ne_irq_mask));
	}
	return 0;
}

static int dqm_enable_lwm(void *q_h)
{
	struct dqm *q = q_h;

	if (q->intcreg->lwm_irq_mask_set)
		dqm_reg_write_mask(q->intcreg->lwm_irq_mask_set,
				   q->lwm_bit_mask, q->lwm_bit_mask);
	return 0;
}

static int dqm_enable_hwm(void *q_h)
{
	struct dqm *q = q_h;

	if (q->intcreg->hwm_irq_mask_set)
		dqm_reg_write_mask(q->intcreg->hwm_irq_mask_set,
				   q->hwm_bit_mask, q->hwm_bit_mask);
	return 0;
}

static int dqm_enable_tmr(void *q_h)
{
	struct dqm *q = q_h;

	if (q->intcreg->tmr_irq_mask_set)
		dqm_reg_write_mask(q->intcreg->tmr_irq_mask_set,
				   q->tmr_bit_mask, q->tmr_bit_mask);
	return 0;
}

static int dqm_disable_ne(void *q_h)
{
	struct dqm *q = q_h;

	if (q->intcreg->ne_irq_mask_clr) {
		if (q->ne_clr_bit_val == 0)
			dqm_reg_write_mask(q->intcreg->ne_irq_mask_clr,
					   q->ne_bit_mask, q->ne_clr_bit_val);
		else
			dqm_reg_write(q->intcreg->ne_irq_mask_clr,q->ne_bit_mask);
		pr_debug("%s %d reg: %px mask:0x%x val:0x%x status:0x%x mask:0x%x\n", __func__, q->num,
			 q->intcreg->ne_irq_mask_clr, q->ne_bit_mask, q->ne_clr_bit_val,
			 *((u32 *)q->intcreg->ne_irq_status), *((u32 *)q->intcreg->ne_irq_mask));
	}
	return 0;
}

static int dqm_disable_lwm(void *q_h)
{
	struct dqm *q = q_h;

	if (q->intcreg->lwm_irq_mask_set)
		dqm_reg_write_mask(q->intcreg->lwm_irq_mask_set,
				   q->lwm_bit_mask, q->lwm_clr_bit_val);
	return 0;
}

static int dqm_disable_hwm(void *q_h)
{
	struct dqm *q = q_h;

	if (q->intcreg->hwm_irq_mask_clr)
		dqm_reg_write_mask(q->intcreg->hwm_irq_mask_clr,
				   q->hwm_bit_mask, q->hwm_clr_bit_val);
	return 0;
}

static int dqm_disable_tmr(void *q_h)
{
	struct dqm *q = q_h;

	if (q->intcreg->tmr_irq_mask_clr)
		dqm_reg_write_mask(q->intcreg->tmr_irq_mask_clr,
				   q->tmr_bit_mask, q->tmr_clr_bit_val);
	return 0;
}

static int dqm_clear_status_ne(void *q_h)
{
	struct dqm *q = q_h;

	if ((q->type == DQM_INTC_EXT) && (q->intcreg->ne_irq_status_clr)) {
#if defined (CONFIG_ARM64)
		pr_debug("ne_irq_status_clr:0x%llx\n",(u64)q->intcreg->ne_irq_status_clr);
#else
		pr_debug("ne_irq_status_clr:0x%px\n",q->intcreg->ne_irq_status_clr);
#endif
		pr_debug("ne_bit_mask:0x%x\n",q->ne_bit_mask);
		pr_debug("ne_clr_bit_val:0x%x\n",q->ne_clr_bit_val);
		dqm_reg_write_mask(q->intcreg->ne_irq_status_clr,
			           q->ne_bit_mask, q->ne_clr_bit_val);
	} else if (q->intcreg->ne_irq_status)
                dqm_reg_write(q->intcreg->ne_irq_status,
                              q->ne_bit_mask);

	return 0;
}

static int dqm_clear_status_lwm(void *q_h)
{
	struct dqm *q = q_h;

	if (q->intcreg->lwm_irq_status)
		dqm_reg_write(q->intcreg->lwm_irq_status,
			      q->lwm_bit_mask);
	return 0;
}

static int dqm_clear_status_hwm(void *q_h)
{
	struct dqm *q = q_h;

	if (q->intcreg->hwm_irq_status)
		dqm_reg_write(q->intcreg->hwm_irq_status,
			      q->hwm_bit_mask);
	return 0;
}

static int dqm_clear_status_tmr(void *q_h)
{
	struct dqm *q = q_h;

	if (q->intcreg->tmr_irq_status)
		dqm_reg_write(q->intcreg->tmr_irq_status,
			      q->tmr_bit_mask);
	return 0;
}

static int dqm_ops_null(void *q_h)
{
	return 0;
}

static irqreturn_t dqm_irq_null(int irq, void *dev)
{
	return IRQ_HANDLED;
}

struct dqm_ops dqmops_null = {
	.enable_ne   = dqm_ops_null,
	.enable_lwm  = dqm_ops_null,
	.enable_hwm  = dqm_ops_null,
	.enable_tmr  = dqm_ops_null,
	.disable_ne  = dqm_ops_null,
	.disable_lwm = dqm_ops_null,
	.disable_hwm = dqm_ops_null,
	.disable_tmr = dqm_ops_null,
	.clear_status_ne  = dqm_ops_null,
	.clear_status_lwm = dqm_ops_null,
	.clear_status_hwm = dqm_ops_null,
	.clear_status_tmr = dqm_ops_null,
	.do_irq     = dqm_irq_null,
};

struct dqm_ops dqmops = {
	.enable_ne   = dqm_enable_ne,
	.enable_lwm  = dqm_enable_lwm,
	.enable_hwm  = dqm_enable_hwm,
	.enable_tmr  = dqm_enable_tmr,
	.disable_ne  = dqm_disable_ne,
	.disable_lwm = dqm_disable_lwm,
	.disable_hwm = dqm_disable_hwm,
	.disable_tmr = dqm_disable_tmr,
	.clear_status_ne  = dqm_clear_status_ne,
	.clear_status_lwm = dqm_clear_status_lwm,
	.clear_status_hwm = dqm_clear_status_hwm,
	.clear_status_tmr = dqm_clear_status_tmr,
	.do_irq     = dqm_irq,
};

static void dqm_irq_mask(struct irq_data *d)
{
	struct dqm_intc *intc = irq_data_get_irq_chip_data(d);

	pr_debug("dqm_irq_mask: %px %d %ld\n", intc, d->irq, d->hwirq);
}

static void dqm_irq_unmask(struct irq_data *d)
{
	struct dqm_intc *intc = irq_data_get_irq_chip_data(d);

	pr_debug("dqm_irq_unmask: %px %d %ld\n", intc, d->irq, d->hwirq);
}

static int dqm_irqdomain_map(struct irq_domain *d, unsigned int irq,
			     irq_hw_number_t hwirq)
{
	struct dqm_intc *intc = d->host_data;

	pr_debug("dqm_irqdomain_map: %s %d %ld\n", intc->irq_chip.name, irq,
		 hwirq);
	irq_set_chip_and_handler(irq, &intc->irq_chip, handle_simple_irq);
	irq_set_chip_data(irq, intc);
	irq_set_noprobe(irq);

	return 0;
}

const struct irq_domain_ops dqm_intc_ops = {
	.map = dqm_irqdomain_map,
	.xlate = irq_domain_xlate_onetwocell,
};

static irqreturn_t dqm_irqchip_handler(int irq, void *data)
{
	struct dqm_intc *intc = data;
	struct dqmdev *qdev = intc->qdev;
	int i, bank;
	struct dqm_intc_reg *intcreg;
	u32 active, ne, lwm, hwm, tmr;
	u32 l1, qnum;

	if (unlikely(!qdev)) {
		pr_err("Spurious interrupt on NULL device!!!\n");
		goto done;
	}

	l1 = dqm_reg_read(intc->reg[0].l1_irq_status);

	for (bank = 0; bank < qdev->bank_count; bank++) {
		intcreg = &intc->reg[bank];
		if (!(l1 & intcreg->l1_irq_dqm_mask))
			continue;
		ne = dqm_reg_read(intcreg->ne_irq_mask) &
			dqm_reg_read(intcreg->ne_irq_status);
		lwm = dqm_reg_read(intcreg->lwm_irq_mask) &
			dqm_reg_read(intcreg->lwm_irq_status);
		hwm = 0;
		if (intcreg->hwm_irq_mask)
			hwm = dqm_reg_read(intcreg->hwm_irq_mask) &
				dqm_reg_read(intcreg->hwm_irq_status);
		tmr = 0;
		if (intcreg->tmr_irq_mask)
			tmr = dqm_reg_read(intcreg->tmr_irq_mask) &
				dqm_reg_read(intcreg->tmr_irq_status);
		active = ne | lwm | hwm | tmr;
		for (i = 0; active; i++, active >>= 1) {
			if (active & 0x1) {
				qnum = (bank << 5) + i;
				irq = irq_linear_revmap(intc->domain, qnum);
				if (ne & 0x1)
					qdev->dqm[qnum].cntne++;
				if (lwm & 0x1)
					qdev->dqm[qnum].cntlwm++;
				if (hwm & 0x1)
					qdev->dqm[qnum].cnthwm++;
				if (tmr & 0x1)
					qdev->dqm[qnum].cnttmr++;

				generic_handle_irq(irq);
			}
			ne >>= 1;
			lwm >>= 1;
			hwm >>= 1;
			tmr >>= 1;
		}
	}


done:
	return IRQ_HANDLED;
}

static irqreturn_t dqm_ext_irqchip_handler(int irq, void *data)
{
	struct dqm_intc *intc = data;
	struct dqmdev *qdev = intc->qdev;
	struct dqm_intc_reg *intcreg;
	int i, bank;
	u32 active, ne, lwm, hwm, tmr;
	u32 qnum;
	u32 l1 = 0;

	if (unlikely(!qdev)) {
		pr_err("Spurious interrupt on NULL device!!!\n");
		goto done;
	}

	if (intc->reg[0].l1_irq_status) {
		l1 = dqm_reg_read(intc->reg[0].l1_irq_status);

		if (!l1)
			goto done;
	}

	for (bank = 0; bank < qdev->bank_count; bank++) {
                intcreg = &intc->reg[bank];
                ne = dqm_reg_read(intcreg->ne_irq_mask) &
                        dqm_reg_read(intcreg->ne_irq_status);
                lwm = dqm_reg_read(intcreg->lwm_irq_mask) &
                        dqm_reg_read(intcreg->lwm_irq_status);
                hwm = 0;
                if (intcreg->hwm_irq_mask)
                        hwm = dqm_reg_read(intcreg->hwm_irq_mask) &
                                dqm_reg_read(intcreg->hwm_irq_status);
                tmr = 0;
                if (intcreg->tmr_irq_mask)
                        tmr = dqm_reg_read(intcreg->tmr_irq_mask) &
                                dqm_reg_read(intcreg->tmr_irq_status);
		active = ne | lwm | hwm | tmr;
		for (i = 0; active; i++, active >>= 1) {
			if (active & 0x1) {
				qnum = (bank << 5) + i;
				irq = irq_linear_revmap(intc->domain, qnum);
				if (ne & 0x1)
					qdev->dqm[qnum].cntne++;
				if (lwm & 0x1)
					qdev->dqm[qnum].cntlwm++;
				if (hwm & 0x1)
					qdev->dqm[qnum].cnthwm++;
				if (tmr & 0x1)
					qdev->dqm[qnum].cnttmr++;
				generic_handle_irq(irq);
			}
			ne >>= 1;
			lwm >>= 1;
			hwm >>= 1;
			tmr >>= 1;
		}
	}


done:
	return IRQ_HANDLED;
}

static void dqm_live_intc_init(struct dqmdev *qdev)
{
	struct dqm_intc *intc;
	struct device_node *of_node = qdev->pdev->dev.of_node;
	struct device_node *of_intc;
	int status, id;
	u32 arr[3], tmp;
	struct dqm_intc_reg *intcreg;

	qdev->intc_count = qdev->q_count;
	intc = kzalloc(sizeof(struct dqm_intc) * qdev->intc_count, GFP_KERNEL);
	if (!intc)
		return;

	qdev->intc = intc;
	for_each_child_of_node(of_node, of_intc) {

		if (of_property_read_bool(of_intc, "interrupt-controller"))
			continue;

		status = of_property_read_u32(of_intc, "id", &id);
		if (status) {
			pr_err("dqm: Unable to retrieve child node id\n");
			return;
		}

		intc[id].type = DQM_INTC_LIVE;
		intcreg = intc[id].reg;

		if (id >= qdev->q_count) {
			pr_err("dqm: Error id %d > %d\n", id, qdev->q_count);
			return;
		}

		status = of_property_read_u32(of_intc,
					      "live-irq-status-offset",
					      &tmp);
		if (status)
			continue;
		status = dqm_dt_read_u32_array(of_intc,
					       "live-irq-mask-offset",
					       arr, 3, false);
		if (status)
			continue;

		intcreg->intc = &intc[id];
		intcreg->lwm_irq_status   =
			(void *)(qdev->reg_base + (tmp >> 2));
		intcreg->lwm_irq_mask     =
			(void *)(qdev->reg_base + (arr[0] >> 2));
		intcreg->lwm_irq_mask_set =
			(void *)(qdev->reg_base + (arr[1] >> 2));
		intcreg->lwm_irq_mask_clr =
			(void *)(qdev->reg_base + (arr[2] >> 2));

		intcreg->hwm_irq_status   = intcreg->lwm_irq_status;
		intcreg->hwm_irq_mask     = intcreg->lwm_irq_mask;
		intcreg->hwm_irq_mask_set = intcreg->lwm_irq_mask_set;
		intcreg->hwm_irq_mask_clr = intcreg->lwm_irq_mask_clr;

		intcreg->tmr_irq_status   = intcreg->lwm_irq_status;
		intcreg->tmr_irq_mask     = intcreg->lwm_irq_mask;
		intcreg->tmr_irq_mask_set = intcreg->lwm_irq_mask_set;
		intcreg->tmr_irq_mask_clr = intcreg->lwm_irq_mask_clr;

		intcreg->ne_irq_status   = intcreg->lwm_irq_status;
		intcreg->ne_irq_mask     = intcreg->lwm_irq_mask;
		intcreg->ne_irq_mask_set = intcreg->lwm_irq_mask_set;
		intcreg->ne_irq_mask_clr = intcreg->lwm_irq_mask_clr;

		intcreg->ne_status = intcreg->ne_irq_status;
		qdev->dqm[id].intcreg = intc[id].reg;
	}

	pr_info("%s: DQM device %s - Live IRQ Interrupt Handling\n",
		MODULE_NAME, qdev->name);
}

static void dqm_legacy_intc_init(struct dqmdev *qdev)
{
	struct dqm_intc *intc;
	struct device_node *of_node = qdev->pdev->dev.of_node;
	struct device_node *of_intc;
	int status, id, bank;
	u32 tmp, arr[DQM_INT_BANKS];
	struct dqm_intc_reg *intcreg;

	if (qdev->bank_count > DQM_INT_BANKS) {
		pr_err("Bank count %d > %d\n", qdev->bank_count,
		       DQM_INT_BANKS);
		return;
	}

	for_each_child_of_node(of_node, of_intc) {
		if (of_property_read_bool(of_intc, "interrupt-controller"))
			qdev->intc_count++;
	}

	intc = kzalloc(sizeof(struct dqm_intc) * qdev->intc_count, GFP_KERNEL);
	if (!intc)
		return;
	qdev->intc = intc;
	id = 0;
	for_each_child_of_node(of_node, of_intc) {

		if (!of_property_read_bool(of_intc, "interrupt-controller"))
			continue;

		intc[id].type = DQM_INTC_LEGACY;
		intcreg = intc[id].reg;

		status = of_property_read_u32(of_intc, "l1-irq-mask-offset",
					      &tmp);
		if (status) {
			pr_err("Missing %s property!\n", "l1-irq-mask-offset");
			return;
		}
		intcreg[0].l1_irq_mask = qdev->reg_base + (tmp >> 2);
		for (bank = 1; bank < qdev->bank_count; bank++)
			intcreg[bank].l1_irq_mask = intcreg[0].l1_irq_mask;

		status = of_property_read_u32(of_intc, "l1-irq-status-offset",
					      &tmp);
		if (status) {
			pr_err("Missing %s property!\n",
			       "l1-irq-status-offset");
			return;
		}
		intcreg[0].l1_irq_status = qdev->reg_base + (tmp >> 2);
		for (bank = 1; bank < qdev->bank_count; bank++)
			intcreg[bank].l1_irq_status = intcreg[0].l1_irq_status;

		status = of_property_read_u32(of_intc, "l1-irq-type-offset",
					      &tmp);
		if (!status) {
			intcreg[0].l1_irq_type = qdev->reg_base + (tmp >> 2);
			for (bank = 1; bank < qdev->bank_count; bank++)
				intcreg[bank].l1_irq_type = intcreg[0].l1_irq_type;
		}

		status = dqm_dt_read_u32_array(of_intc, "l1-irq-dqm-mask",
					       arr, qdev->bank_count, true);
		if (status)
			return;
		for (bank = 0; bank < qdev->bank_count; bank++)
			intcreg[bank].l1_irq_dqm_mask = arr[bank];

		status = dqm_dt_read_u32_array(of_intc, "lwm-irq-mask-offset",
					       arr, qdev->bank_count, true);
		if (status)
			return;
		for (bank = 0; bank < qdev->bank_count; bank++) {
			intcreg[bank].lwm_irq_mask =
				qdev->reg_base + (arr[bank] >> 2);
			intcreg[bank].lwm_irq_mask_set =
				intcreg[bank].lwm_irq_mask;
			intcreg[bank].lwm_irq_mask_clr =
				intcreg[bank].lwm_irq_mask;
		}

		status = dqm_dt_read_u32_array(of_intc,
					       "lwm-irq-status-offset",
					       arr, qdev->bank_count, true);
		if (status)
			return;
		for (bank = 0; bank < qdev->bank_count; bank++)
			intcreg[bank].lwm_irq_status =
			qdev->reg_base + (arr[bank] >> 2);

		status = dqm_dt_read_u32_array(of_intc,
					       "hwm-irq-mask-offset",
					       arr, qdev->bank_count, false);
		if (!status) {
			for (bank = 0; bank < qdev->bank_count; bank++) {
				intcreg[bank].hwm_irq_mask =
					qdev->reg_base + (arr[bank] >> 2);
				intcreg[bank].hwm_irq_mask_set =
					intcreg[bank].hwm_irq_mask;
				intcreg[bank].hwm_irq_mask_clr =
					intcreg[bank].hwm_irq_mask;
			}
			status = dqm_dt_read_u32_array(of_intc,
						       "hwm-irq-status-offset",
						       arr, qdev->bank_count,
						       true);
			if (!status) {
				for (bank = 0; bank < qdev->bank_count; bank++)
					intcreg[bank].hwm_irq_status =
					qdev->reg_base + (arr[bank] >> 2);
			}
		}

		status = dqm_dt_read_u32_array(of_intc,
					       "tmr-irq-mask-offset",
					       arr, qdev->bank_count, false);
		if (!status) {
			for (bank = 0; bank < qdev->bank_count; bank++) {
				intcreg[bank].tmr_irq_mask =
					qdev->reg_base + (arr[bank] >> 2);
				intcreg[bank].tmr_irq_mask_set =
					intcreg[bank].tmr_irq_mask;
				intcreg[bank].tmr_irq_mask_clr =
					intcreg[bank].tmr_irq_mask;
			}
			status = dqm_dt_read_u32_array(of_intc,
						       "tmr-irq-status-offset",
						       arr, qdev->bank_count,
						       true);
			if (!status) {
				for (bank = 0; bank < qdev->bank_count; bank++)
					intcreg[bank].tmr_irq_status =
					qdev->reg_base + (arr[bank] >> 2);
			}
		}

		status = dqm_dt_read_u32_array(of_intc,
					       "ne-irq-mask-offset",
					       arr, qdev->bank_count, true);
		if (status)
			return;
		for (bank = 0; bank < qdev->bank_count; bank++) {
			intcreg[bank].ne_irq_mask =
				qdev->reg_base + (arr[bank] >> 2);
			intcreg[bank].ne_irq_mask_set =
				intcreg[bank].ne_irq_mask;
			intcreg[bank].ne_irq_mask_clr =
				intcreg[bank].ne_irq_mask;
		}

		status = dqm_dt_read_u32_array(of_intc, "ne-irq-status-offset",
					       arr, qdev->bank_count, true);
		if (status)
			return;
		for (bank = 0; bank < qdev->bank_count; bank++)
			intcreg[bank].ne_irq_status =
			qdev->reg_base + (arr[bank] >> 2);

		status = dqm_dt_read_u32_array(of_intc, "ne-status-offset",
					       arr, qdev->bank_count, true);
		if (status)
			return;
		for (bank = 0; bank < qdev->bank_count; bank++)
			intcreg[bank].ne_status =
			qdev->reg_base + (arr[bank] >> 2);

		for (bank = 0; bank < qdev->bank_count; bank++) {
			if (intcreg[bank].l1_irq_mask)
				dqm_reg_write_mask(intcreg[bank].l1_irq_mask,
					intcreg[bank].l1_irq_dqm_mask, 0);
			dqm_reg_write(intcreg[bank].lwm_irq_mask, 0);
			dqm_reg_write(intcreg[bank].ne_irq_mask, 0);
			if (intcreg[bank].hwm_irq_mask)
				dqm_reg_write(intcreg[bank].hwm_irq_mask, 0);
			if (intc[id].reg[0].tmr_irq_mask)
				dqm_reg_write(intcreg[bank].tmr_irq_mask, 0);
		}

		intc[id].irq = irq_of_parse_and_map(of_intc, 0);
		intc[id].irq_chip.name = of_intc->full_name;
		intc[id].irq_chip.irq_mask = dqm_irq_mask;
		intc[id].irq_chip.irq_unmask = dqm_irq_unmask;
		intc[id].qdev = qdev;
		for (bank = 0; bank < qdev->bank_count; bank++)
			intc[id].reg[bank].intc = &intc[id];
		intc[id].domain = irq_domain_add_linear(of_intc, qdev->q_count,
							&dqm_intc_ops,
							&intc[id]);
		status = devm_request_irq(&qdev->pdev->dev, intc[id].irq,
					  dqm_irqchip_handler, IRQF_SHARED,
					  of_intc->name, &intc[id]);
		id++;
	}
	pr_info("%s: DQM device %s - Legacy Interrupt Controller\n",
		MODULE_NAME, qdev->name);
}

static void dqm_ext_intc_init(struct dqmdev *qdev)
{
	struct dqm_intc *intc;
	struct device_node *of_node = qdev->pdev->dev.of_node;
	struct device_node *of_intc;
	int status, id, bank;
	u32 tmp, arr[3*DQM_INT_BANKS];
	struct dqm_intc_reg *intcreg;

	if (qdev->bank_count > DQM_INT_BANKS) {
		pr_err("Bank count %d > %d\n", qdev->bank_count,
		       DQM_INT_BANKS);
		return;
	}

	for_each_child_of_node(of_node, of_intc) {

		if (of_property_read_bool(of_intc, "interrupt-controller"))
			qdev->intc_count++;
	}
	intc = kzalloc(sizeof(struct dqm_intc) * qdev->intc_count, GFP_KERNEL);
	if (!intc)
		return;
	qdev->intc = intc;
	id = 0;
	for_each_child_of_node(of_node, of_intc) {

		if (!of_property_read_bool(of_intc, "interrupt-controller"))
			continue;

		intc[id].type = DQM_INTC_EXT;
		intcreg = intc[id].reg;

		status = of_property_read_u32(of_intc, "l1-irq-status-offset",
					      &tmp);
		if (!status)
			intcreg[0].l1_irq_status =
			qdev->reg_base + (tmp >> 2);
		for (bank = 1; bank < qdev->bank_count; bank++)
			intcreg[bank].l1_irq_status =
			intcreg[0].l1_irq_status;

		status = dqm_dt_read_u32_array(of_intc,
					       "lwm-irq-status-offset",
					       arr, 1, true);
		if (status)
			return;
		intcreg[0].lwm_irq_status =
			(void *)(qdev->reg_base + (arr[0] >> 2));
		for (bank = 1; bank < qdev->bank_count; bank++)
			intcreg[bank].lwm_irq_status =
			intcreg[0].lwm_irq_status;

		status = dqm_dt_read_u32_array(of_intc,
					       "lwm-irq-mask-offset",
					       arr, 3*qdev->bank_count, true);
		if (status)
			return;

		for (bank = 0; bank < qdev->bank_count; bank++) {
			intcreg[bank].lwm_irq_mask     =
			(void *)(qdev->reg_base + (arr[bank*3+0] >> 2));
			intcreg[bank].lwm_irq_mask_set =
			(void *)(qdev->reg_base + (arr[bank*3+1] >> 2));
			intcreg[bank].lwm_irq_mask_clr =
			(void *)(qdev->reg_base + (arr[bank*3+2] >> 2));
		}

		status = dqm_dt_read_u32_array(of_intc,
					       "hwm-irq-status-offset",
					       arr, 1, true);
		if (status)
			return;
		intcreg[0].hwm_irq_status =
			(void *)(qdev->reg_base + (arr[0] >> 2));
		for (bank = 1; bank < qdev->bank_count; bank++)
			intcreg[bank].hwm_irq_status =
			intcreg[0].hwm_irq_status;

		status = dqm_dt_read_u32_array(of_intc,
					       "hwm-irq-mask-offset",
					       arr, 3*qdev->bank_count, true);
		if (status)
			return;

		for (bank = 0; bank < qdev->bank_count; bank++) {
			intcreg[bank].hwm_irq_mask     =
			(void *)(qdev->reg_base + (arr[bank*3+0] >> 2));
			intcreg[bank].hwm_irq_mask_set =
			(void *)(qdev->reg_base + (arr[bank*3+1] >> 2));
			intcreg[bank].hwm_irq_mask_clr =
			(void *)(qdev->reg_base + (arr[bank*3+2] >> 2));
		}

		status = dqm_dt_read_u32_array(of_intc,
					       "tmr-irq-status-offset",
					       arr, 1, true);
		if (status)
			return;
		intcreg[0].tmr_irq_status =
			(void *)(qdev->reg_base + (arr[0] >> 2));
		for (bank = 1; bank < qdev->bank_count; bank++)
			intcreg[bank].tmr_irq_status =
			intcreg[0].tmr_irq_status;

		status = dqm_dt_read_u32_array(of_intc,
					       "tmr-irq-mask-offset",
					       arr, 3*qdev->bank_count, true);
		if (status)
			return;

		for (bank = 0; bank < qdev->bank_count; bank++) {
			intcreg[bank].tmr_irq_mask     =
			(void *)(qdev->reg_base + (arr[bank*3+0] >> 2));
			intcreg[bank].tmr_irq_mask_set =
			(void *)(qdev->reg_base + (arr[bank*3+1] >> 2));
			intcreg[bank].tmr_irq_mask_clr =
			(void *)(qdev->reg_base + (arr[bank*3+2] >> 2));
		}

		status = dqm_dt_read_u32_array(of_intc,
					       "ne-irq-status-offset",
					       arr, 3*qdev->bank_count, true);
		if (status)
			return;

                for (bank = 0; bank < qdev->bank_count; bank++) {
                        intcreg[bank].ne_irq_status    =
                        (void *)(qdev->reg_base + (arr[bank*3+0] >> 2));
                        intcreg[bank].ne_irq_status_set =
                        (void *)(qdev->reg_base + (arr[bank*3+1] >> 2));
                        intcreg[bank].ne_irq_status_clr=
                        (void *)(qdev->reg_base + (arr[bank*3+2] >> 2));
                }

		status = dqm_dt_read_u32_array(of_intc,
					       "ne-irq-mask-offset",
					       arr, 3*qdev->bank_count, true);
		if (status)
			return;
		for (bank = 0; bank < qdev->bank_count; bank++) {
			intcreg[bank].ne_irq_mask     =
			(void *)(qdev->reg_base + (arr[bank*3+0] >> 2));
			intcreg[bank].ne_irq_mask_set =
			(void *)(qdev->reg_base + (arr[bank*3+1] >> 2));
			intcreg[bank].ne_irq_mask_clr =
			(void *)(qdev->reg_base + (arr[bank*3+2] >> 2));
		}

		for (bank = 0; bank < qdev->bank_count; bank++) {
			dqm_reg_write(intcreg[bank].lwm_irq_mask_clr,
				      0xFFFFFFFF);
			dqm_reg_write(intcreg[bank].ne_irq_mask_clr,
				      0xFFFFFFFF);
			if (intcreg[bank].hwm_irq_mask)
				dqm_reg_write(intcreg[bank].hwm_irq_mask_clr,
					      0xFFFFFFFF);
			if (intcreg[bank].tmr_irq_mask)
				dqm_reg_write(intcreg[bank].tmr_irq_mask_clr,
					      0xFFFFFFFF);
		}

		intc[id].irq = irq_of_parse_and_map(of_intc, 0);
		intc[id].irq_chip.name = of_intc->full_name;
		intc[id].irq_chip.irq_mask = dqm_irq_mask;
		intc[id].irq_chip.irq_unmask = dqm_irq_unmask;
		intc[id].qdev = qdev;
		for (bank = 0; bank < qdev->bank_count; bank++)
			intc[id].reg[bank].intc = &intc[id];
		intc[id].domain = irq_domain_add_linear(of_intc, qdev->q_count,
							&dqm_intc_ops,
							&intc[id]);
		status = devm_request_irq(&qdev->pdev->dev, intc[id].irq,
					  dqm_ext_irqchip_handler, IRQF_SHARED,
					  of_intc->name, &intc[id]);
		id++;
	}
	pr_info("%s: DQM device %s - Ext Interrupt Controller\n",
		MODULE_NAME, qdev->name);
}

static int dqm_intc_init(struct dqmdev *qdev)
{
	struct device_node *of_node = qdev->pdev->dev.of_node;

	if (of_property_read_bool(of_node, "legacy-irq")) {
		dqm_legacy_intc_init(qdev);
		return DQM_INTC_LEGACY;
	}
	else if (of_property_read_bool(of_node, "ext-irq")) {
		dqm_ext_intc_init(qdev);
		return DQM_INTC_EXT;
	}
	else if (of_property_read_bool(of_node, "live-irq")) {
		dqm_live_intc_init(qdev);
		return DQM_INTC_LIVE;
	}
	return 0;
}

static void dqm_intc_qmap(struct dqmdev *qdev)
{
	int i, bank;

	if (!qdev->intc)
		return;

	for (i = 0; i < qdev->q_count; i++) {
		struct irq_data *data;
		struct dqm *q = &qdev->dqm[i];
                struct dqm_intc *intc;

		q->rx = dqm_legacy_rx;
		if (!q->irq)
			continue;

		q->ops = &dqmops;

		if (q->type == DQM_INTC_LIVE) {
			q->ops->do_irq = dqm_live_irq;
			q->rx = dqm_burst_rx;
			q->lwm_bit_mask = DQM_LIVE_IRQ_LWM_MASK;
			q->hwm_bit_mask = DQM_LIVE_IRQ_HWM_MASK;
			q->tmr_bit_mask = DQM_LIVE_IRQ_TMR_MASK;
			q->ne_bit_mask =  DQM_LIVE_IRQ_NE_MASK;
			q->lwm_clr_bit_val = DQM_LIVE_IRQ_LWM_MASK;
			q->hwm_clr_bit_val = DQM_LIVE_IRQ_HWM_MASK;
			q->tmr_clr_bit_val = DQM_LIVE_IRQ_TMR_MASK;
			q->ne_clr_bit_val =  DQM_LIVE_IRQ_NE_MASK;
		} else if (q->type == DQM_INTC_EXT) {
			data = irq_get_irq_data(q->irq);
			intc = data->domain->host_data;
			bank = i>>5;
			q->intcreg = &intc->reg[bank];

			q->ops->do_irq = dqm_irq;
			q->lwm_bit_mask = q->q_bit_mask;
			q->hwm_bit_mask = q->q_bit_mask;
			q->tmr_bit_mask = q->q_bit_mask;
			q->ne_bit_mask = q->q_bit_mask;
			q->lwm_clr_bit_val = q->q_bit_mask;
			q->hwm_clr_bit_val = q->q_bit_mask;
			q->tmr_clr_bit_val = q->q_bit_mask;
			q->ne_clr_bit_val = q->q_bit_mask;
		} else {
			data = irq_get_irq_data(q->irq);
			intc = data->domain->host_data;
			bank = i>>5;
			q->intcreg = &intc->reg[bank];
			q->ops->do_irq = dqm_irq;
			q->lwm_bit_mask = q->q_bit_mask;
			q->hwm_bit_mask = q->q_bit_mask;
			q->tmr_bit_mask = q->q_bit_mask;
			q->ne_bit_mask = q->q_bit_mask;
			q->lwm_clr_bit_val = 0;
			q->hwm_clr_bit_val = 0;
			q->tmr_clr_bit_val = 0;
			q->ne_clr_bit_val = 0;
		}
	}

	for (i = 0; i < qdev->intc_count; i++) {
		struct dqm_intc *intc = &qdev->intc[i];

		if (intc->type != DQM_INTC_LEGACY)
			continue;
		for (bank = 0; bank < qdev->bank_count; bank++) {
			dqm_reg_write(intc->reg[bank].lwm_irq_status,
				      0xffffffff);
			dqm_reg_write(intc->reg[bank].ne_irq_status,
				      0xffffffff);
			if (intc->reg[bank].hwm_irq_status)
				dqm_reg_write(intc->reg[bank].hwm_irq_status,
					      0xffffffff);
			if (intc->reg[bank].tmr_irq_status)
				dqm_reg_write(intc->reg[bank].tmr_irq_status,
					      0xffffffff);
			if (intc->reg[bank].l1_irq_mask)
				dqm_reg_write_mask(intc->reg[bank].l1_irq_mask,
					   intc->reg[bank].l1_irq_dqm_mask,
					   intc->reg[bank].l1_irq_dqm_mask);
			if (intc->reg[bank].l1_irq_type)
				dqm_reg_write_mask(intc->reg[bank].l1_irq_type,
					   intc->reg[bank].l1_irq_dqm_mask, 0);
		}
	}
}

/*
 * Device probe
 *
 * This will be called by Linux for each device tree node matching our
 * driver name we passed when we called platform_driver_register().
 * Each node specifies a network device interface to create.
 *
 * The QSM may have already been configured by another CPU or by us during
 * a previous cold boot (we may be warm booting). Since other CPU's may
 * already be using it we must be careful when resetting/initializing
 * the DQM HW.
 *
 * Returns
 *	0 for success, < 0 is error code
 */
static int dqm_probe(struct platform_device *pdev)
{
	int status;
	int type;
	struct dqmdev *qdev;
	struct _qsm_alloc *qsm_alloc;
	u32 qsm_addr;
	int i, bank;
	u32 q_qsm;
	struct fpm_hw_info fpm_info;
	bool q_prev_cfgd = false;

	qdev = kzalloc(sizeof(struct dqmdev), GFP_KERNEL);
	if (!qdev) {
		status = -ENOMEM;
		goto done;
	}

	pdev->dev.platform_data = qdev;
	qdev->pdev = pdev;

	status = dqm_parse_dt_node(pdev);
	if (status)
		goto err_free_qdev;
	if (qdev->fpm_alloc || qdev->tok_base) {
		if (!driver_find("brcm-fpm", &platform_bus_type) ||
		    fpm_get_hw_info(&fpm_info)) {
			status = -EINVAL;
			goto err_free_qdev;
		}
	}

	if (qdev->probe_defer) {
		u32 defer_value = dqm_reg_read(qdev->probe_defer_register);

		if (qdev->defer_value != (defer_value & qdev->defer_mask)) {
			status = -EPROBE_DEFER;
			goto err_free_qdev;
		}
	}

	qdev->dqm = kzalloc((sizeof(struct dqm) * qdev->q_count),
		GFP_KERNEL);
	if (!qdev->dqm) {
		status = -ENOMEM;
		goto err_free_qdev;
	}

	qsm_alloc = kzalloc(sizeof(struct _qsm_alloc) * qdev->q_count,
		GFP_KERNEL);
	if (!qsm_alloc) {
		status = -ENOMEM;
		goto err_free_dqm;
	}

	/* Get the DQM interrupt type after initalizating it */
	type = dqm_intc_init(qdev);

	status = dqm_parse_dt_node_child(pdev, qsm_alloc);
	if (status) {
		status = -ENOMEM;
		goto err_free_qsm_alloc;
	}

	for (i = 0; i < qdev->q_count; i++) {
		bool restricted = qdev->restricted_access;
		if (!qsm_alloc[i].dt_valid)
			continue;
		bank = i >> 5;
		qdev->dqm[i].type = type;
		qdev->dqm[i].dqmdev = qdev;
		qdev->dqm[i].ops = &dqmops_null;
		qdev->dqm[i].flags = 0;
		qdev->dqm[i].num = i;
		qdev->dqm[i].q_bit_mask = 1 << (i & 0x1f);
		qdev->dqm[i].offload = qdev->offload;
		if (!restricted) {
			qdev->dqm[i].ctl = qdev->dqm[i].offload ?
				(struct dqmctl *)((struct dqmctl_ol *)
						  (qdev->q_ctl_base[bank]) +
						  (i & 0x1f)) :
				qdev->q_ctl_base[bank] + (i & 0x1f);
		}

		qdev->dqm[i].tmr = qdev->q_tmr_base[bank] ?
			qdev->q_tmr_base[bank] + (i & 0x1f) : NULL;
		qdev->dqm[i].data = qdev->q_data_base[bank] +
			((i & 0x1f) * (qdev->dqm[i].offload ?
				       DQM_OL_Q_WORD_COUNT : DQM_Q_WORD_COUNT));
		qdev->dqm[i].status = qdev->q_status_base[bank] + (i & 0x1f);
		if (qdev->dqm[i].type == DQM_INTC_LEGACY) {
			qdev->dqm[i].mib = qdev->q_mib_base[bank] ?
				qdev->q_mib_base[bank] + (i & 0x1f) : NULL;
		} else {
			qdev->dqm[i].mib = qdev->q_mib_base[bank] ?
				qdev->q_mib_base[bank] + ((i & 0x1f) * 4) : NULL;
		}
		spin_lock_init(&qdev->dqm[i].lock);

		if (!restricted && DQM_GET_Q_SIZE(&qdev->dqm[i])) {
			pr_debug("Q #%d already configured.\n", i);
			q_prev_cfgd = true;
		}
		strncpy(qdev->dqm[i].name, qsm_alloc[i].name,
			sizeof(qsm_alloc[i].name)-1);
		pr_debug("DQM %d (%s): ctl %px data %px status %px mib %px\n", i, qdev->dqm[i].name,
			 qdev->dqm[i].ctl, qdev->dqm[i].data, qdev->dqm[i].status, qdev->dqm[i].mib);
		if (qdev->q_ctl_base)
                pr_debug("bank: %d ctl_base[0]: %px, ctl_base[bank]: %px\n",
                         bank, qdev->q_ctl_base[0], qdev->q_ctl_base[bank]);

		if (restricted) 
		{
			qdev->dqm[i].msg_size = qsm_alloc[i].msg_size;
			qdev->dqm[i].depth = qsm_alloc[i].depth;
			qdev->dqm[i].lwm = qsm_alloc[i].lwm;
			qdev->dqm[i].hwm = qsm_alloc[i].hwm;
		}
		else
		{
			qdev->dqm[i].msg_size = DQM_GET_TOK_SIZE(&qdev->dqm[i]);
			qdev->dqm[i].depth = DQM_GET_NUM_TOK(&qdev->dqm[i]);
			qdev->dqm[i].lwm = DQM_GET_LWM(&qdev->dqm[i]);
			qdev->dqm[i].hwm = DQM_GET_HWM(&qdev->dqm[i]);
		}
		pr_debug("msg_size %d depth %d lwm %d hwm %d\n",
				((restricted) ? qdev->dqm[i].msg_size : qdev->dqm[i].msg_size + 1), /* This maxes out at a value of 3 to mean that a token is 4 words long. */
				qdev->dqm[i].depth,
				qdev->dqm[i].lwm,
				qdev->dqm[i].hwm);
	}

	if (qdev->cfg_qsm) {
		if (q_prev_cfgd)
			pr_debug("Overwriting Q config & QSM allocation.\n");

		for (i = 0, qsm_addr = 0; i < qdev->q_count; i++) {
			if (!qsm_alloc[i].msg_size)
				continue;
			pr_debug("Configuring Q # %d\n", i);
			DQM_SET_TOK_SIZE(&qdev->dqm[i],
				qsm_alloc[i].msg_size - 1);
			pr_debug("Words/Element: %d\n", qsm_alloc[i].msg_size);
			DQM_SET_Q_ADDR(&qdev->dqm[i], qsm_addr);
			pr_debug("QSM Address: 0x%08x\n", qsm_addr);
			if (qsm_alloc[i].offload) {
				if (!qdev->dqm[i].offload) {
					pr_err("%s DQM %d non-OL Q but ",
					       qdev->name, i);
					pr_err("configured for OL!\n");
					goto err_free_qsm_alloc;
				}
				if (qsm_alloc[i].depth >
				    DQM_OL_Q_MAX_DEPTH(qsm_alloc[i].msg_size)) {
					pr_err("%s DQM %d depth exceeds ",
					       qdev->name, i);
					pr_err("max depth for OL Q!\n");
					goto err_free_qsm_alloc;
				}
				q_qsm = DQM_OL_Q_QSM_BYTES >> 2;
			} else {
				if (qdev->dqm[i].offload)
					DQM_SET_OL_DISABLE(&qdev->dqm[i], 1);
				q_qsm = qsm_alloc[i].msg_size *
					qsm_alloc[i].depth;
			}
			DQM_SET_Q_SIZE(&qdev->dqm[i], q_qsm);
			pr_debug("QSM Size: %d\n", q_qsm);
			qsm_addr += ALIGN(q_qsm, DQM_Q_QSM_ALIGN);
			DQM_SET_NUM_TOK(&qdev->dqm[i], qsm_alloc[i].depth);
			pr_debug("Depth: %d\n", qsm_alloc[i].depth);
			if (qsm_addr >= qdev->qsm_size) {
				pr_err("%s QSM allocation exceeds QSM size!\n",
				       qdev->name);
				goto err_free_qsm_alloc;
			}
			DQM_SET_LWM(&qdev->dqm[i], qsm_alloc[i].lwm);
			pr_debug("LWM: %d\n", qsm_alloc[i].lwm);
			DQM_SET_HWM(&qdev->dqm[i], qsm_alloc[i].hwm);
			pr_debug("HWM: %d\n", qsm_alloc[i].hwm);
			if (qdev->dqm[i].tmr && qsm_alloc[i].timeout) {
				DQM_SET_TIMEOUT(&qdev->dqm[i],
						qsm_alloc[i].timeout);
				pr_debug("timeout: %d\n", qsm_alloc[i].timeout);
				DQM_SET_TMR_MODE(&qdev->dqm[i],
						 DQM_TMR_MODE_REPETITIVE);
				DQM_ENABLE_TMR(&qdev->dqm[i], true);
			}
		}

		for (bank = 0; bank < qdev->bank_count; bank++) {
			dqm_reg_write(qdev->cfg[bank],
				      (qdev->qsm_size << 16));
			if (qdev->fpm_alloc[bank]) {
				for (i = 0; i < 4; i++) {
					if (fpm_info.chunk_size >> (8 + i) & 1)
						break;
				}
				if (i == 4) {
					pr_err("Invalid fpm_info.chunk_size: %d\n",
					       fpm_info.chunk_size);
					goto err_free_qsm_alloc;
				}
				dqm_reg_write(qdev->fpm_alloc[bank],
					      fpm_info.alloc_dealloc[i]);
			}
		}
		if (qdev->tok_base) {
			dqm_reg_write(&qdev->tok_base->buf_base,
				      fpm_info.pool_base[0]);
			dqm_reg_write(&qdev->tok_base->buf_base1,
				      fpm_info.pool_base[1]);
			dqm_reg_write_mask(&qdev->tok_base->buf_size,
				DQM_ALLOC_CHUNKS_MASK(0),
				DQM_ALLOC_8_CHUNKS << DQM_ALLOC_CHUNKS_SHIFT(0));
			dqm_reg_write_mask(&qdev->tok_base->buf_size,
				DQM_ALLOC_CHUNKS_MASK(1),
				DQM_ALLOC_4_CHUNKS << DQM_ALLOC_CHUNKS_SHIFT(1));
			dqm_reg_write_mask(&qdev->tok_base->buf_size,
				DQM_ALLOC_CHUNKS_MASK(2),
				DQM_ALLOC_2_CHUNKS << DQM_ALLOC_CHUNKS_SHIFT(2));
			dqm_reg_write_mask(&qdev->tok_base->buf_size,
				DQM_ALLOC_CHUNKS_MASK(3),
				DQM_ALLOC_1_CHUNKS << DQM_ALLOC_CHUNKS_SHIFT(3));
			dqm_reg_write_mask(&qdev->tok_base->buf_size2,
				DQM_CHUNK_SIZE_MASK,
				((fpm_info.chunk_size >> 9) <<
				 DQM_CHUNK_SIZE_SHIFT));
		}
	}

	#if defined(DQM_ENABLE_HWM_LWM_TRM)
	for (i = 0; i < qdev->q_count; i++) {
		if (!qsm_alloc[i].dt_valid)
			continue;
		qdev->dqm[i].hwm = DQM_GET_HWM(&qdev->dqm[i]);
		qdev->dqm[i].lwm = DQM_GET_LWM(&qdev->dqm[i]);
		qdev->dqm[i].timeout = qdev->dqm[i].tmr ?
			DQM_GET_TIMEOUT(&qdev->dqm[i]) : 0;
	}
	#else
	pr_info("%s: DQM device %s - HWM,LWM and TMR Disabled\n", MODULE_NAME,
		qdev->name);
	#endif
	kfree(qsm_alloc);
	list_add_rcu(&qdev->list, &dqmdevs);
	synchronize_rcu();

	dqm_intc_qmap(qdev);

	pr_info("%s: DQM device %s with %d Q's\n", MODULE_NAME,
		qdev->name, qdev->q_count);

	dqmdev_proc_init(qdev);
	goto done;

err_free_qsm_alloc:
	kfree(qsm_alloc);
err_free_dqm:
	kfree(qdev->dqm);
err_free_qdev:
	kfree(qdev);

done:
	pr_debug("<--\n");
	return status;
}

/*
 * Device remove
 *
 * Returns
 *	0 for success, < 0 is error code
 */
static int dqm_remove(struct platform_device *pdev)
{
	int status = 0;
	struct dqmdev *qdev = pdev->dev.platform_data;
	struct dqm *q;
	int i, bank;
	struct dqmdev *qdev_tmp;

	if (!qdev) {
		pr_err("Release called with uninitialized platform_data.\n");
		status = -EINVAL;
		goto done;
	}

	pr_debug("--> Device %s\n", qdev->name);

	dqmdev_proc_exit(qdev);

	if (!qdev->intc)
		goto skip_intc;
	for (i = 0; i < qdev->intc_count; i++) {
		if (!qdev->intc[i].type)
			continue;

		if (qdev->intc[i].reg[0].l1_irq_mask)
			dqm_reg_write_mask(qdev->intc[i].reg[0].l1_irq_mask,
				   qdev->intc[i].reg[0].l1_irq_dqm_mask, 0);
		for (bank = 0; bank < qdev->bank_count; bank++) {
			dqm_reg_write(qdev->intc[i].reg[bank].lwm_irq_mask, 0);
			dqm_reg_write(qdev->intc[i].reg[bank].ne_irq_mask, 0);
			if (qdev->intc[i].reg[bank].hwm_irq_mask)
			dqm_reg_write(qdev->intc[i].reg[bank].hwm_irq_mask, 0);
			if (qdev->intc[i].reg[bank].tmr_irq_mask)
			dqm_reg_write(qdev->intc[i].reg[bank].tmr_irq_mask, 0);
		}
	}
	irq_set_chained_handler_and_data(qdev->intc->irq, NULL, NULL);
	for (i = 0; i < qdev->q_count; i++) {
		int irq = irq_linear_revmap(qdev->intc->domain, i);

		irq_dispose_mapping(irq);
	}
	irq_domain_remove(qdev->intc->domain);
	kfree(qdev->intc);
skip_intc:

	for (i = 0; i < qdev->q_count; i++) {
		q = &qdev->dqm[i];
		if ((q->flags & DQM_F_TX) || (q->flags & DQM_F_RX)) {
			pr_err("Releasing device %s containing in-use DQM %d!\n",
				   qdev->name, q->num);
		}
	}
	list_for_each_entry_rcu(qdev_tmp, &dqmdevs, list) {
		if (qdev_tmp == qdev) {
			list_del_rcu(&qdev_tmp->list);
			break;
		}
	}
	synchronize_rcu();

	iounmap(qdev->reg_base);

	kfree(qdev->cfg);
	kfree(qdev->q_ctl_base);
	kfree(qdev->q_tmr_base);
	kfree(qdev->q_data_base);
	kfree(qdev->q_status_base);
	kfree(qdev->q_mib_base);
	kfree(qdev->fpm_alloc);
	kfree(qdev->dqm);
	kfree(qdev);

done:
	pr_debug("<--\n");
	return status;
}

core_initcall(dqm_init);
MODULE_LICENSE("GPL v2");
