/*
 * Copyright (c) 2020 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/of.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
#include <linux/debugfs.h>
#include <linux/notifier.h>
#include <linux/mfd/syscon.h>
#include <uapi/linux/major.h>
#include <linux/completion.h>
#include <linux/ipc_logging.h>
#include <linux/workqueue.h>
#include <linux/qcom_scm.h>
#include <linux/bt.h>

void bt_ipc_purge_tx_queue(struct bt_descriptor *btDesc)
{
	struct ipc_intent *intent;

	while (!list_empty(&btDesc->ipc.tx_q)) {
		intent = list_first_entry(&btDesc->ipc.tx_q, struct ipc_intent,
						list);

		list_del(&intent->list);
		kfree(intent->buf);
		kfree(intent);
	}

	atomic_set(&btDesc->ipc.tx_q_cnt, 0);
}
EXPORT_SYMBOL(bt_ipc_purge_tx_queue);

static void bt_ipc_process_pending_tx_queue(struct bt_descriptor *btDesc)
{
	int ret;
	struct ipc_intent *intent;

	while (!list_empty(&btDesc->ipc.tx_q)) {
		intent = list_first_entry(&btDesc->ipc.tx_q, struct ipc_intent,
						list);

		ret = bt_ipc_send_msg(btDesc, (uint8_t)0x100, intent->buf,
					intent->len, false);
		if (ret) {
			mdelay(50);
			break;
		}

		list_del(&intent->list);
		atomic_dec(&btDesc->ipc.tx_q_cnt);
		kfree(intent->buf);
		kfree(intent);
	}
}

int
bt_ipc_queue_tx(struct bt_descriptor *btDesc, const uint8_t *buf, uint16_t len)
{
	struct ipc_intent *intent;
	uint8_t *buffer;
	struct device *dev = &btDesc->pdev->dev;

	if (unlikely(atomic_read(&btDesc->ipc.tx_q_cnt) >= IPC_TX_QSIZE)) {
		dev_err(dev, "TX Queue Limit reached\n");
		return -ENOSPC;
	}

	intent = kzalloc(sizeof(struct ipc_intent), GFP_KERNEL);
	if (!intent)
		return -ENOMEM;

	buffer = kzalloc(len, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	memcpy_toio(buffer, buf, len);
	intent->buf = buffer;

	intent->len = len;
	list_add_tail(&intent->list, &btDesc->ipc.tx_q);
	atomic_inc(&btDesc->ipc.tx_q_cnt);

	return 0;
}

static void *bt_ipc_alloc_lmsg(struct bt_descriptor *btDesc, uint32_t len,
		struct ipc_aux_ptr *aux_ptr, uint8_t *is_lbuf_full)
{
	uint8_t idx;
	uint8_t blks;
	uint8_t blks_consumed;
	struct bt_mem *btmem = &btDesc->btmem;
	struct device *dev = &btDesc->pdev->dev;
	uint32_t lsz = IPC_LBUF_SZ(btmem->tx_ctxt, TotalMemorySize, lring_buf,
				   lmsg_buf_cnt);

	if (btmem->tx_ctxt->lring_buf == 0) {
		dev_err(dev, "no long message buffer not initialized\n");
		return ERR_PTR(-ENODEV);
	}

	blks = GET_NO_OF_BLOCKS(len, lsz);

	if (!btmem->lmsg_ctxt.lmsg_free_cnt ||
			(blks > btmem->lmsg_ctxt.lmsg_free_cnt))
		return ERR_PTR(-EAGAIN);

	idx = btmem->lmsg_ctxt.widx;

	if ((btmem->lmsg_ctxt.widx + blks) > btmem->tx_ctxt->lmsg_buf_cnt) {
		blks_consumed = btmem->tx_ctxt->lmsg_buf_cnt - idx;
		aux_ptr->len = len - (blks_consumed * lsz);
		aux_ptr->buf = btmem->tx_ctxt->lring_buf;
	}

	btmem->lmsg_ctxt.widx = (btmem->lmsg_ctxt.widx + blks) %
		btmem->tx_ctxt->lmsg_buf_cnt;

	btmem->lmsg_ctxt.lmsg_free_cnt -= blks;

	if (btmem->lmsg_ctxt.lmsg_free_cnt <=
			((btmem->tx_ctxt->lmsg_buf_cnt * 20) / 100))
		*is_lbuf_full = 1;

	return (TO_APPS_ADDR(btmem->tx_ctxt->lring_buf) + (idx * lsz));
}

static struct ring_buffer_info *bt_ipc_get_tx_rbuf(struct bt_descriptor *btDesc,
		uint8_t *is_sbuf_full)
{
	uint8_t idx;
	struct ring_buffer_info *rinfo;
	struct bt_mem *btmem = &btDesc->btmem;

	for (rinfo = &(btmem->tx_ctxt->sring_buf_info);	rinfo != NULL;
		rinfo = (struct ring_buffer_info *)(uintptr_t)(rinfo->next)) {
		idx = (rinfo->widx + 1) % (btmem->tx_ctxt->smsg_buf_cnt);

		if (idx != rinfo->tidx) {
			btmem->lmsg_ctxt.smsg_free_cnt--;

			if (btmem->lmsg_ctxt.smsg_free_cnt <=
				((btmem->tx_ctxt->smsg_buf_cnt * 20) / 100))
				*is_sbuf_full = 1;

			return rinfo;
		}
	}

	return ERR_PTR(-EAGAIN);
}

int bt_ipc_send_msg(struct bt_descriptor *btDesc, uint16_t msg_hdr,
		const uint8_t *pData, uint16_t len, bool dequeue)
{
	int ret = 0;
	struct bt_mem *btmem = &btDesc->btmem;
	struct device *dev = &btDesc->pdev->dev;
	struct ring_buffer_info *rinfo;
	struct ring_buffer *rbuf;
	uint8_t is_lbuf_full = 0;
	uint8_t is_sbuf_full = 0;
	struct ipc_aux_ptr aux_ptr;
	void *lmsg_data;

	if (dequeue)
		bt_ipc_process_pending_tx_queue(btDesc);

	rinfo = bt_ipc_get_tx_rbuf(btDesc, &is_sbuf_full);
	if (IS_ERR(rinfo)) {
		dev_err(dev, "short msg buf full, queuing msg[%d]\n",
				atomic_read(&btDesc->ipc.tx_q_cnt));
		ret = PTR_ERR(rinfo);
		if (dequeue)
			ret = bt_ipc_queue_tx(btDesc, pData, len);
		return ret;
	}

	rbuf = &((struct ring_buffer *)(TO_APPS_ADDR(
						rinfo->rbuf)))[rinfo->widx];
	rbuf->msg_hdr = msg_hdr;
	rbuf->len = len;

	if (len > IPC_MSG_PLD_SZ) {
		rbuf->msg_hdr = rbuf->msg_hdr | IPC_LMSG_MASK;

		aux_ptr.len = 0;
		aux_ptr.buf = 0;

		lmsg_data = bt_ipc_alloc_lmsg(btDesc, len,
				&aux_ptr, &is_lbuf_full);

		if (IS_ERR(lmsg_data)) {
			dev_err(dev, "long msg buf full, queuing msg[%d]\n",
					atomic_read(&btDesc->ipc.tx_q_cnt));
			ret = PTR_ERR(lmsg_data);
			if (dequeue)
				ret = bt_ipc_queue_tx(btDesc, pData, len);
			return ret;
		}

		memcpy_toio(lmsg_data, pData,
				(len - aux_ptr.len));

		if (aux_ptr.buf) {
			memcpy_toio(TO_APPS_ADDR(aux_ptr.buf),
				(pData + (len - aux_ptr.len)), aux_ptr.len);
		}

		rbuf->payload.lmsg_data = TO_BT_ADDR(lmsg_data);
	} else {
		memcpy_toio(rbuf->payload.smsg_data, pData, len);
	}

	if (is_sbuf_full || is_lbuf_full)
		rbuf->msg_hdr = rbuf->msg_hdr | IPC_RACK_MASK;

	rinfo->widx = (rinfo->widx + 1) % btmem->tx_ctxt->smsg_buf_cnt;

	regmap_write(btDesc->ipc.regmap, btDesc->ipc.offset,
			BIT(btDesc->ipc.bit));

	return ret;
}

static
void bt_ipc_free_lmsg(struct bt_descriptor *btDesc, uint32_t lmsg, uint16_t len)
{
	uint8_t idx;
	uint8_t blks;
	struct bt_mem *btmem = &btDesc->btmem;
	uint32_t lsz = IPC_LBUF_SZ(btmem->tx_ctxt, TotalMemorySize, lring_buf,
				   lmsg_buf_cnt);

	idx = GET_TX_INDEX_FROM_BUF(lmsg, lsz);

	if (idx != btmem->lmsg_ctxt.ridx)
		return;

	blks = GET_NO_OF_BLOCKS(len, lsz);

	btmem->lmsg_ctxt.ridx  = (btmem->lmsg_ctxt.ridx  + blks) %
		btmem->tx_ctxt->lmsg_buf_cnt;

	btmem->lmsg_ctxt.lmsg_free_cnt += blks;
}

static void bt_ipc_cust_msg(struct bt_descriptor *btDesc, uint8_t msgid)
{
	struct device *dev = &btDesc->pdev->dev;
	struct bt_mem *btmem = &btDesc->btmem;
	uint16_t msg_hdr = 0;
	int ret;

	msg_hdr |= msgid;

	switch (msgid) {
	case IPC_CMD_IPC_STOP:
		spin_unlock(&btDesc->lock);
		ret = qti_scm_toggle_bt_eco(PAS_ID, 0x4);
		spin_lock(&btDesc->lock);
		if (ret) {
			dev_err(dev, "Failed to set BT ECO\n");
			return;
		}

		dev_info(dev, "BT IPC Stopped, gracefully stopping APSS IPC\n");
		break;
	case IPC_CMD_SWITCH_TO_UART:
		dev_info(dev, "Configured UART, Swithing BT to debug mode\n");
		break;
	case IPC_CMD_PREPARE_DUMP:
		dev_info(dev, "IPQ crashed, inform BT to prepare dump\n");
		break;
	case IPC_CMD_COLLECT_DUMP:
		dev_info(dev, "BT Crashed, gracefully stopping IPC\n");
		return;
	case IPC_CMD_IPC_START:
		spin_unlock(&btDesc->lock);
		ret = qti_scm_toggle_bt_eco(PAS_ID, 0x0);
		spin_lock(&btDesc->lock);
		if (ret) {
			dev_err(dev, "Failed to reset BT ECO\n");
			return;
		}

		btmem->tx_ctxt = (struct context_info *)((void *)
			btmem->rx_ctxt + btmem->rx_ctxt->TotalMemorySize);
		btmem->lmsg_ctxt.widx = 0;
		btmem->lmsg_ctxt.ridx = 0;
		btmem->lmsg_ctxt.smsg_free_cnt = btmem->tx_ctxt->smsg_buf_cnt;
		btmem->lmsg_ctxt.lmsg_free_cnt = btmem->tx_ctxt->lmsg_buf_cnt;
		atomic_set(&btDesc->state, 1);

		dev_info(dev, "BT IPC Started, starting APSS IPC\n");
		return;
	default:
		dev_err(dev, "invalid custom message\n");
		return;
	}

	if (unlikely(!atomic_read(&btDesc->state))) {
		dev_err(dev, "BT IPC not initialized, no message sent\n");
		return;
	}

	atomic_set(&btDesc->state, 0);

	ret = bt_ipc_send_msg(btDesc, msg_hdr, NULL, 0, true);
	if (ret)
		dev_err(dev, "err: sending message\n");
}

static bool bt_ipc_process_peer_msgs(struct bt_descriptor *btDesc,
		struct ring_buffer_info *rinfo, uint8_t *pRxMsgCount)
{
	struct bt_mem *btmem = &btDesc->btmem;
	struct ring_buffer *rbuf;
	uint8_t ridx, lbuf_idx;
	uint8_t blks_consumed;
	struct ipc_aux_ptr aux_ptr;
	enum ipc_pkt_type pktType = IPC_CUST_PKT;
	bool ackReqd = false;
	uint8_t *rxbuf = NULL;
	unsigned char *buf;
	uint32_t lsz = IPC_LBUF_SZ(btmem->rx_ctxt, TotalMemorySize, lring_buf,
				   lmsg_buf_cnt);

	ridx = rinfo->ridx;

	rbuf = &((struct ring_buffer *)(TO_APPS_ADDR(
			btmem->rx_ctxt->sring_buf_info.rbuf)))[ridx];

	while (ridx != rinfo->widx) {
		memset(&aux_ptr, 0, sizeof(struct ipc_aux_ptr));

		rbuf = &((struct ring_buffer *)(TO_APPS_ADDR(
				btmem->rx_ctxt->sring_buf_info.rbuf)))[ridx];

		if (IS_LONG_MSG(rbuf->msg_hdr)) {
			rxbuf = TO_APPS_ADDR(rbuf->payload.lmsg_data);

			if (IS_RX_MEM_NON_CONTIGIOUS(rbuf->payload.lmsg_data,
							rbuf->len, lsz)) {

				lbuf_idx = GET_RX_INDEX_FROM_BUF(
						rbuf->payload.lmsg_data, lsz);

				blks_consumed = btmem->rx_ctxt->lmsg_buf_cnt -
					lbuf_idx;
				aux_ptr.len = rbuf->len - (blks_consumed * lsz);
				aux_ptr.buf = btmem->rx_ctxt->lring_buf;
			}
		} else {
			rxbuf = rbuf->payload.smsg_data;
		}

		if (IS_REQ_ACK(rbuf->msg_hdr))
			ackReqd = true;

		pktType = IPC_GET_PKT_TYPE(rbuf->msg_hdr);

		switch (pktType) {
		case IPC_HCI_PKT:
			buf = kzalloc(rbuf->len, GFP_ATOMIC);
			if (!buf)
				return -ENOMEM;

			memcpy_fromio(buf, rxbuf, (rbuf->len - aux_ptr.len));

			if (aux_ptr.buf)
				memcpy_fromio(buf + (rbuf->len - aux_ptr.len),
					TO_APPS_ADDR(aux_ptr.buf), aux_ptr.len);

			btDesc->recvmsg_cb(btDesc, buf, rbuf->len);
			kfree(buf);
			break;
		case IPC_CUST_PKT:
			bt_ipc_cust_msg(btDesc, IPC_GET_MSG_ID(rbuf->msg_hdr));
			break;
		case IPC_AUDIO_PKT:
			break;
		default:
			break;
		}

		ridx = (ridx + 1) % rinfo->ring_buf_cnt;
	}

	spin_unlock(&btDesc->lock);
	wait_event(btDesc->ipc.wait_q,
			((2 * rbuf->len) < bt_ipc_avail_size(btDesc)));
	spin_lock(&btDesc->lock);
	rinfo->ridx = ridx;

	return ackReqd;
}

static void bt_ipc_process_ack(struct bt_descriptor *btDesc)
{
	struct ring_buffer_info *rinfo;
	struct bt_mem *btmem = &btDesc->btmem;

	for (rinfo = &btmem->tx_ctxt->sring_buf_info; rinfo != NULL;
		rinfo = (struct ring_buffer_info *)(uintptr_t)(rinfo->next)) {
		uint8_t tidx = rinfo->tidx;
		struct ring_buffer *rbuf = (struct ring_buffer *)
			TO_APPS_ADDR(rinfo->rbuf);

		while (tidx != rinfo->ridx) {
			if (IS_LONG_MSG(rbuf[tidx].msg_hdr)) {
				bt_ipc_free_lmsg(btDesc,
						 rbuf[tidx].payload.lmsg_data,
						 rbuf[tidx].len);
			}

			tidx = (tidx + 1) % btmem->tx_ctxt->smsg_buf_cnt;
			btmem->lmsg_ctxt.smsg_free_cnt++;
		}

		rinfo->tidx = tidx;
	}
}

static
int bt_ipc_sendmsg(struct bt_descriptor *btDesc, unsigned char *buf, int len)
{
	int ret;
	uint16_t msg_hdr = 0x100;
	struct device *dev = &btDesc->pdev->dev;

	if (unlikely(!atomic_read(&btDesc->state))) {
		dev_err(dev, "BT IPC not initialized, no message sent\n");
		return -ENODEV;
	}

	spin_lock(&btDesc->lock);

	ret = bt_ipc_send_msg(btDesc, msg_hdr, (uint8_t *)buf, (uint16_t)len,
				true);
	if (ret)
		dev_err(dev, "err: sending message\n");

	spin_unlock(&btDesc->lock);

	return ret;
}

static void bt_ipc_worker(struct work_struct *work)
{
	struct ring_buffer_info *rinfo;

	struct bt_ipc *ipc = container_of(work, struct bt_ipc, work);
	struct bt_descriptor *btDesc = container_of(ipc, struct bt_descriptor,
						    ipc);
	struct bt_mem *btmem = &btDesc->btmem;
	bool ackReqd = false;
	struct rproc *rproc = platform_get_drvdata(btDesc->rproc_pdev);

	if (!atomic_read(&rproc->power))
		return;

	spin_lock(&btDesc->lock);

	if (unlikely(!atomic_read(&btDesc->state)))
		btmem->rx_ctxt = (struct context_info *)(btmem->virt + 0xe000);
	else
		bt_ipc_process_ack(btDesc);

	for (rinfo = &(btmem->rx_ctxt->sring_buf_info); rinfo != NULL;
		rinfo = (struct ring_buffer_info *)(uintptr_t)(rinfo->next)) {
		if (bt_ipc_process_peer_msgs(btDesc, rinfo,
					&btmem->rx_ctxt->smsg_buf_cnt)) {
			ackReqd = true;
		}
	}

	if (ackReqd) {
		regmap_write(ipc->regmap, ipc->offset, BIT(ipc->bit));
	}

	if (btDesc->debug_en)
		bt_ipc_cust_msg(btDesc, IPC_CMD_SWITCH_TO_UART);

	spin_unlock(&btDesc->lock);
	enable_irq(ipc->irq);
}

static irqreturn_t bt_ipc_irq_handler(int irq, void *data)
{
	struct bt_descriptor *btDesc = data;

	disable_irq_nosync(btDesc->ipc.irq);
	queue_work(btDesc->ipc.wq, &btDesc->ipc.work);

	return IRQ_HANDLED;
}


static
int ipc_panic_handler(struct notifier_block *nb, unsigned long event, void *ptr)
{
	struct bt_descriptor *btDesc = container_of(nb, struct bt_descriptor,
								panic_nb);
	bt_ipc_cust_msg(btDesc, IPC_CMD_PREPARE_DUMP);

	return NOTIFY_DONE;
}

int bt_ipc_init(struct bt_descriptor *btDesc)
{
	int ret;
	struct bt_ipc *ipc = &btDesc->ipc;
	struct device *dev = &btDesc->pdev->dev;

	init_waitqueue_head(&ipc->wait_q);
	spin_lock_init(&btDesc->lock);
	INIT_LIST_HEAD(&ipc->tx_q);

	ipc->wq = create_singlethread_workqueue("bt_ipc");
	if (!ipc->wq) {
		dev_err(dev, "failed to initialize WQ\n");
		return -EAGAIN;
	}

	INIT_WORK(&ipc->work, bt_ipc_worker);

	ret = devm_request_threaded_irq(dev, ipc->irq, NULL, bt_ipc_irq_handler,
		IRQF_TRIGGER_RISING | IRQF_ONESHOT, "bt_ipc_irq", btDesc);

	if (ret) {
		dev_err(dev, "error registering irq[%d] ret = %d\n",
				ipc->irq, ret);
		goto irq_err;
	}

	btDesc->panic_nb.notifier_call = ipc_panic_handler;

	ret = atomic_notifier_chain_register(&panic_notifier_list,
							&btDesc->panic_nb);
	if (ret)
		goto panic_nb_err;

	btDesc->sendmsg_cb = bt_ipc_sendmsg;

	return 0;

panic_nb_err:
	devm_free_irq(dev, ipc->irq, btDesc);
irq_err:
	return ret;
}
EXPORT_SYMBOL(bt_ipc_init);

void bt_ipc_deinit(struct bt_descriptor *btDesc)
{
	struct bt_ipc *ipc = &btDesc->ipc;
	struct device *dev = &btDesc->pdev->dev;

	atomic_set(&btDesc->state, 0);
	if (ipc->wq != NULL) {
		devm_free_irq(dev, ipc->irq, btDesc);
		bt_ipc_purge_tx_queue(btDesc);
		atomic_notifier_chain_unregister(&panic_notifier_list,
				&btDesc->panic_nb);
		flush_work(&ipc->work);
		destroy_workqueue(ipc->wq);
	}
}
EXPORT_SYMBOL(bt_ipc_deinit);
