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
 /*******************************************************************************
 *
 * itc_msg_q.c
 * Oct. 23 2012
 * Peter Sulc
 *
 *******************************************************************************/
#include <linux/slab.h>
#include <itc_msg_q.h>

void rpc_init_queue(rpc_queue *q)
{
	spin_lock_init(&q->lock);
	sema_init(&q->sema, 0);
	q->head = 0;
	q->tail = 0;
	q->limit = 0;
	q->leaky_bucket = false;
}

static void rpc_add_to_queue_tail_atomic(rpc_queue *q, rpc_queue_msg *msg)
{
	msg->next = 0;

	if (q->tail)
		q->tail->next = msg;
	else
		q->head = msg;
	q->tail = msg;
}

static rpc_queue_msg *rpc_remove_head_from_queue_atomic(rpc_queue *q)
{
	rpc_queue_msg *msg = q->head;
	if (msg) {
		q->head = msg->next;
		if (msg == q->tail)
			q->tail = msg->next;
	}
	return msg;
}

rpc_queue_msg *rpc_add_to_queue_tail(rpc_queue *q, rpc_queue_msg *msg)
{
	unsigned long flags;
	rpc_queue_msg *ret = NULL;

	spin_lock_irqsave(&q->lock, flags);

	if (unlikely(q->limit && (q->sema.count >= q->limit))) {
		if (q->leaky_bucket) {
			rpc_add_to_queue_tail_atomic(q, msg);
			ret = rpc_remove_head_from_queue_atomic(q);
		}
		else {
			ret = msg;
		}
		spin_unlock_irqrestore(&q->lock, flags);
	}
	else {
		rpc_add_to_queue_tail_atomic(q, msg);
		spin_unlock_irqrestore(&q->lock, flags);
		up(&q->sema);
	}
	return ret;
}



rpc_queue_msg *rpc_remove_head_from_queue(rpc_queue *q)
{
	unsigned long flags;
	rpc_queue_msg *msg;

	if (down_interruptible(&q->sema))
		return NULL;	/* ctrl-c ? */

	spin_lock_irqsave(&q->lock, flags);

	msg = rpc_remove_head_from_queue_atomic(q);

	spin_unlock_irqrestore(&q->lock, flags);

	return msg;
}

rpc_queue_msg *rpc_try_remove_head_from_queue(rpc_queue *q)
{
	unsigned long flags;
	rpc_queue_msg *msg;

	if (down_trylock(&q->sema))
		return NULL;
	spin_lock_irqsave(&q->lock, flags);

	msg = q->head;
	if (msg) {
		q->head = msg->next;
		if (msg == q->tail)
			q->tail = msg->next;
	}
	spin_unlock_irqrestore(&q->lock, flags);

	return msg;
}

rpc_queue_msg *rpc_remove_xid_from_queue(rpc_queue *q, u32 xid)
{
	unsigned long flags;
	rpc_queue_msg *prev, *msg;

	spin_lock_irqsave(&q->lock, flags);

	/* if there is nothing in the queue then leave */
	if (down_trylock(&q->sema))
	{
		spin_unlock_irqrestore(&q->lock, flags);
		return NULL;
	}
	msg = q->head;
	prev = NULL;
	while (msg) {
		if (rpc_qmsg_xid(msg) == xid) {
			if (prev) {
				prev->next = msg->next;
				if (q->tail == msg)
					q->tail = prev;
			}
			else {
				q->head = msg->next;
				if (q->tail == msg)
					q->tail = msg->next; /* NULL */
			}
			break;
		}
		prev = msg;
		msg = msg->next;
	}
	if (msg == NULL)
		up(&q->sema);

	spin_unlock_irqrestore(&q->lock, flags);
	return msg;
}

rpc_queue_msg *rpc_remove_func_from_queue(rpc_queue *q, u32 func_idx)
{
	unsigned long flags;
	rpc_queue_msg *prev, *msg;

	spin_lock_irqsave(&q->lock, flags);

	/* if there is nothing in the queue then leave */
	if (down_trylock(&q->sema))
	{
		spin_unlock_irqrestore(&q->lock, flags);
		return NULL;
	}
	msg = q->head;
	prev = NULL;
	while (msg) {
		if (rpc_qmsg_function(msg) == func_idx) {
			if (prev) {
				prev->next = msg->next;
				if (q->tail == msg)
					q->tail = prev;
			}
			else {
				q->head = msg->next;
				if (q->tail == msg)
					q->tail = msg->next; /* NULL */
			}
			break;
		}
		prev = msg;
		msg = msg->next;
	}
	if (msg == NULL)
		up(&q->sema);

	spin_unlock_irqrestore(&q->lock, flags);
	return msg;
}

rpc_queue_msg *rpc_remove_matching_from_queue(rpc_queue *q, rpc_queue_msg *m)
{
	unsigned long flags;
	rpc_queue_msg *prev, *msg;

	spin_lock_irqsave(&q->lock, flags);

	/* if there is nothing in the queue then leave */
	if (down_trylock(&q->sema))
	{
		spin_unlock_irqrestore(&q->lock, flags);
		return NULL;
	}
	msg = q->head;
	prev = NULL;
	while (msg) {
		if ((rpc_qmsg_function(msg) == rpc_qmsg_function(m)) &&
		    (rpc_qmsg_request(msg) == rpc_qmsg_request(m)) &&
		    (rpc_qmsg_reply(msg) == rpc_qmsg_reply(m)) &&
		    (rpc_qmsg_service(msg) == rpc_qmsg_service(m)) &&
		    (msg->msg.data[0] == m->msg.data[0]) &&
		    (msg->msg.data[1] == m->msg.data[1]) &&
		    (msg->msg.data[2] == m->msg.data[2]))	{
			if (prev) {
				prev->next = msg->next;
				if (q->tail == msg)
					q->tail = prev;
			}
			else {
				q->head = msg->next;
				if (q->tail == msg)
					q->tail = msg->next; /* NULL */
			}
			break;
		}
		prev = msg;
		msg = msg->next;
	}
	if (msg == NULL)
		up(&q->sema);

	spin_unlock_irqrestore(&q->lock, flags);
	return msg;
}


void rpc_dump_qmsg(rpc_queue_msg *msg)
{
	if (msg) {
		pr_info("req = %p next = %p\n", msg, msg->next);
		rpc_dump_msg(&msg->msg);
	}
	else
		pr_info("req = NULL\n");
}

void rpc_dump_queue(rpc_queue *q)
{
	unsigned long flags;
	rpc_queue_msg *msg;

	spin_lock_irqsave(&q->lock, flags);
	pr_info("QUEUE count: %d\n", q->sema.count);
	pr_info("head: ");
	rpc_dump_qmsg(q->head);
	pr_info("tail: ");
	rpc_dump_qmsg(q->tail);

	msg = q->head;
	while (msg) {
		rpc_dump_qmsg(msg);
		msg = msg->next;
	}
	spin_unlock_irqrestore(&q->lock, flags);
}

/* queue_msg pool functions */

rpc_queue_msg_pool *rpc_queue_msg_pool_create(int cnt)
{
	int i, sz;
	rpc_queue_msg	*msg;
	rpc_queue_msg_pool *qp;

	sz = sizeof(rpc_queue_msg_pool) + (cnt * sizeof(rpc_queue_msg));
	qp = (rpc_queue_msg_pool *)kmalloc(sz, GFP_KERNEL);
	if (!qp)
		return NULL;

	qp->bottom = (rpc_queue_msg *)(qp + 1);
	memset(qp->bottom, 0, cnt * sizeof(rpc_queue_msg));
	qp->top = qp->bottom + cnt;
	qp->free_pool = qp->bottom;

	for (i = 0, msg = qp->free_pool; i < cnt-1; i++, msg++)
		msg->next = msg + 1;

	spin_lock_init(&qp->lock);
	sema_init(&qp->free_cnt, cnt);

	return qp;
}

rpc_queue_msg *rpc_queue_msg_pool_alloc(rpc_queue_msg_pool *qp)
{
	unsigned long flags;
	rpc_queue_msg *msg = NULL;

	if (down_trylock(&qp->free_cnt))
		return NULL;

	spin_lock_irqsave(&qp->lock, flags);

	msg = qp->free_pool;
	qp->free_pool = msg->next;

	spin_unlock_irqrestore(&qp->lock, flags);
	return msg;
}

int rpc_queue_msg_pool_free(rpc_queue_msg_pool *qp, rpc_queue_msg *msg)
{
	unsigned long flags;
	if (msg >= qp->bottom && msg < qp->top)
	{
		spin_lock_irqsave(&qp->lock, flags);

		msg->next = qp->free_pool;
		qp->free_pool = msg;

		up(&qp->free_cnt);
		spin_unlock_irqrestore(&qp->lock, flags);

		return 0;
	}
	pr_info("%s: msg %p does not belong in pool %p - %p\n",
		__func__, msg, qp->bottom, qp->top);
	return -1;
}
