 /****************************************************************************
 *
 * Broadcom Proprietary and Confidential. (c) 2017 Broadcom.  All rights reserved.
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
 ****************************************************************************/
 /*******************************************************************************
 *
 * itc_rpc.c
 * Oct. 23 2012
 * Peter Sulc
 *
 *******************************************************************************/
#include <linux/version.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/semaphore.h>
#include <linux/hardirq.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/seq_file.h>

#include <itc_rpc.h>
#include <itc_msg_q.h>
#include <itc_rpc_dt.h>
#include <itc_channel_structs.h>
#include "itc_channel_tables.c"
#include "itc_rpc_fifo.h"
#include "itc_rpc_dbg.h"
#include "fpm.h"

#define MODULE_NAME	"brcm-itc-rpc"
#define MODULE_VER	"1.0"

#define DEBUG_DUMP_MESSAGE		0
#define RPC_RX_CRASH_POLL_DELAY_MS	1

extern void device_unblock_probing(void);

u32 rpc_version;

/* global list of available FIFO tunnels */
static fifo_tunnel *tunnels[RPC_TUNNELS_MAX];
static spinlock_t tunnels_lock;

static fifo_tunnel *boot_ft;

/* global message pool for all services */
static rpc_queue_msg_pool *msg_pool = 0;

static void tunnel_init_error(fifo_tunnel *ft)
{
	rpc_msg err_msg;
	int version =
		itc_rpc_services[RPC_SERVICE_INIT].func_tab[INIT_SVC_ERR].version;

	pr_err("Error: tunnel init error on FIFO tunnel %s\n",
	       ft->name);
	if (boot_ft) {
		rpc_msg_init(&err_msg, RPC_SERVICE_INIT, INIT_SVC_ERR, version,
			     (-INIT_SVC_ERR_RC_HANDSHAKE & 0xff), 0, 0);
		rpc_send_message(boot_ft->id, &err_msg, false);
	}
}

static int check_msg_version(rpc_msg *msg)
{
	int version, servidx, funcidx;
	rpc_service *serv;
	rpc_function *func;

	version = rpc_msg_version(msg);
	servidx = rpc_msg_service(msg);
	funcidx = rpc_msg_function(msg);
	if (servidx < 0 || servidx >= RPC_MAX_SERVICES)
		return 0;
	serv = &itc_rpc_services[servidx];
	if (funcidx < 0 ||
	    funcidx >= serv->func_tab_sz ||
	    !serv->func_tab)
		return 0;
	func = &serv->func_tab[funcidx];
	return (!func->func || func->version == version) ? 0 : -EINVAL;
}

static void msg_version_error(rpc_msg *msg)
{
	rpc_msg err_msg;
	int msg_ver = rpc_msg_version(msg);
	int servidx = rpc_msg_service(msg);
	int funcidx = rpc_msg_service(msg);
	rpc_service *serv;
	int expected_ver, init_err_ver;

	if (servidx < 0 || servidx >= RPC_MAX_SERVICES)
		return;
	serv = &itc_rpc_services[servidx];
	if (funcidx < 0 ||
	    funcidx >= serv->func_tab_sz ||
	    !serv->func_tab)
		return;
	expected_ver =
		itc_rpc_services[servidx].func_tab[funcidx].version;
	init_err_ver =
		itc_rpc_services[RPC_SERVICE_INIT].func_tab[INIT_SVC_ERR].version;
	pr_err("Error: version mismatch on received msg,\n"
	       "expected %d, received %d\n", expected_ver, msg_ver);
	rpc_dump_msg(msg);
	if (boot_ft) {
		rpc_msg_init(&err_msg, RPC_SERVICE_INIT, INIT_SVC_ERR, init_err_ver,
			     (-INIT_SVC_ERR_RC_MSG_VER_MISMATCH & 0xff), 0, 0);
		rpc_send_message(boot_ft->id, &err_msg, false);
	}
}

static void tunnel_version_error(int tunnel_ver)
{
	rpc_msg err_msg;
	int version =
		itc_rpc_services[RPC_SERVICE_INIT].func_tab[INIT_SVC_ERR].version;

	pr_err("Error: version mismatch on tunnel,\n"
	       "expected %d, received %d\n", rpc_version, tunnel_ver);
	if (boot_ft) {
		rpc_msg_init(&err_msg, RPC_SERVICE_INIT, INIT_SVC_ERR, version,
			     (-INIT_SVC_ERR_RC_RPC_VER_MISMATCH & 0xff), 0, 0);
		rpc_send_message(boot_ft->id, &err_msg, false);
	}
}

static int rpc_tunnel_replay_orphans(fifo_tunnel *ft)
{
	int status = 0;
	rpc_msg *msg = (rpc_msg *)fpm_token_to_buffer(ft->orph_tok);
	int orph_cnt = fpm_get_token_size(ft->orph_tok);
	int servidx;
	rpc_service *serv;
	rpc_queue_msg *qmsg;

	qmsg = rpc_queue_msg_pool_alloc(msg_pool);
	if (qmsg == NULL) {
		pr_err("Error: rpc msg pool is empty!\n");
		pr_err("Forced to toss incoming orphan msgs "
		       " on tunnel %s\n", ft->name);
		status = -EFAULT;
		goto done;
	}

	while (orph_cnt--) {
		servidx = rpc_msg_service(msg);
		serv = &itc_rpc_services[servidx];
		if (servidx >= 0 && servidx < RPC_MAX_SERVICES) {
			rpc_service *serv = &itc_rpc_services[servidx];
			serv->rx_cnt++;
			if (serv->rcv_queue) {
				qmsg->tunnel = ft->id;
				qmsg->msg = *msg++;
				rpc_add_to_queue_tail(serv->rcv_queue, qmsg);
				/* alloc another message and check again */
				qmsg = rpc_queue_msg_pool_alloc(msg_pool);
				if (qmsg == NULL) {
					pr_err("Error: rpc msg pool is empty!\n");
					pr_err("Forced to toss incoming orphan msgs "
					       " on tunnel %s\n", ft->name);
					status = -EFAULT;
					break;
				}
			} else {
				pr_err("Error: No service queue for message\n");
				serv->rx_err_cnt++;
				rpc_dump_qmsg(qmsg);
			}
		} else {
			pr_err("Error: No service for message\n");
			rpc_dump_qmsg(qmsg);
		}
	}
	if (qmsg)
		rpc_queue_msg_pool_free(msg_pool, qmsg);
done:
	return status;
}

int rpc_get_fifo_tunnel_id(char *name)
{
	int tunnel = -1;
	int i;

	spin_lock(&tunnels_lock);
	for (i = 0; i < RPC_TUNNELS_MAX; i++) {
		if (tunnels[i]) {
			fifo_tunnel *ft = tunnels[i];
			if (!strncmp(ft->name, name, sizeof(ft->name))) {
				tunnel = i;
				pr_debug("%s: returning id %d for %s\n", __func__,
					 i, name);
				break;;
			}
		}
	}
	spin_unlock(&tunnels_lock);
	if (i == RPC_TUNNELS_MAX)
		pr_err("%s unable to find RPC tunnel %s\n", __func__, name);

	return tunnel;
}
EXPORT_SYMBOL(rpc_get_fifo_tunnel_id);

/*
 * we are out of messages in the free pool, so we need to reclaim orphan queues
 */
static void reclaim_orphan_queues(void)
{
	int servidx;
	rpc_service *serv;
	rpc_queue_msg *qmsg;

	for (servidx = 0; servidx < RPC_MAX_SERVICES; servidx++)
	{
		serv = &itc_rpc_services[servidx];
		if (serv->orphan_queue)
			while ((qmsg = rpc_try_remove_head_from_queue(serv->orphan_queue)))
				rpc_queue_msg_pool_free(msg_pool, qmsg);
	}
}

/*
 * rx_isr handles all FIFO RX events for a specific RX FIFO
 */
static irqreturn_t rx_isr(void *fifo_h, void *context, u32 flags)
{
	int tunnel = (int)(uintptr_t)context;
	fifo_tunnel *ft;
	rpc_queue_msg *qmsg;

	if (tunnel < 0 || tunnel >= RPC_TUNNELS_MAX || !tunnels[tunnel]) {
		pr_err("%s Error invalid FIFO tunnel %d\n", __func__, tunnel);
		return 0;
	}
	ft = tunnels[tunnel];

	pr_debug("%s tunnel (%s) %d\n", __func__, ft->name, tunnel);
	if (fifo_h != ft->rx_fifo_h) {
		pr_err("%s Error FIFO mismatch: fifo_h %p, rx_fifo_h %p\n",
		       __func__, fifo_h, ft->rx_fifo_h);
		return 0;
	}

	qmsg = rpc_queue_msg_pool_alloc(msg_pool);
	if (qmsg == NULL) {
		reclaim_orphan_queues();
		qmsg = rpc_queue_msg_pool_alloc(msg_pool);
	}
	if (qmsg == NULL) {
		rpc_msg msg;
		pr_err("Error: rpc msg pool is empty!\n");
		while (rpc_fifo_rx(ft->rx_fifo_h, &msg) > 0);
		pr_err("Forced to drain FIFO %d on tunnel %d\n",
		       ft->rx_fifo, tunnel);
	}
	while (qmsg && (rpc_fifo_rx(ft->rx_fifo_h, &qmsg->msg) > 0)) {
		int servidx = rpc_qmsg_service(qmsg);
		if (servidx >= 0 && servidx < RPC_MAX_SERVICES) {
			rpc_service *serv = &itc_rpc_services[servidx];
			serv->rx_cnt++;
			if (serv->rcv_queue) {
				qmsg->tunnel = ft->id;
				rpc_add_to_queue_tail(serv->rcv_queue, qmsg);
				/* alloc another message and check again */
				qmsg = rpc_queue_msg_pool_alloc(msg_pool);
				if (qmsg == NULL) {
					reclaim_orphan_queues();
					qmsg = rpc_queue_msg_pool_alloc(msg_pool);
				}
				if (qmsg == NULL) {
					rpc_msg msg;
					pr_err("Error: rpc msg pool is empty!\n");
					while (rpc_fifo_rx(ft->rx_fifo_h, &msg) > 0);
					pr_err("Forced to drain FIFO %d on tunnel %d\n",
					       ft->rx_fifo, tunnel);
				}
			} else {
				pr_err("Error: No service queue for message\n");
				serv->rx_err_cnt++;
				rpc_dump_qmsg(qmsg);
			}
		} else {
			pr_err("Error: No service for message\n");
			rpc_dump_qmsg(qmsg);
		}
	}
	if (qmsg)
		rpc_queue_msg_pool_free(msg_pool, qmsg);

	if (rpc_fifo_enable_rx_cb(ft->rx_fifo_h))
		pr_err("%s Error: unable to enable RX FIFO %d on %s callbacks\n",
		       __func__, ft->rx_fifo, ft->fifo_dev);
	return 0;
}

static void handle_reply(rpc_service *serv, rpc_queue_msg *qmsg)
{
	rpc_queue_msg *reqmsg;

	reqmsg = rpc_remove_xid_from_queue(serv->req_queue, rpc_qmsg_xid(qmsg));
	if (reqmsg) {
		memcpy(&reqmsg->msg, &qmsg->msg, sizeof(rpc_msg));
#if DEBUG_DUMP_MESSAGE
		if (rpc_qmsg_service(reqmsg) == DEBUG_DUMP_MESSAGE) {
			pr_info("%s got reply\n", __func__);
			rpc_dump_msg((rpc_msg *)reqmsg);
		}
#endif
		up(&reqmsg->sema);
	} else {
		pr_err("Error: rx unexpected reply for msg xid %d\n",
		       rpc_qmsg_xid(qmsg));
		rpc_dump_qmsg(qmsg);
		rpc_dump_queue(serv->req_queue);
		serv->rx_err_cnt++;
	}
	rpc_queue_msg_pool_free(msg_pool, qmsg);
}

static void handle_request(rpc_service *serv, rpc_queue_msg *qmsg)
{
	int function = rpc_qmsg_function(qmsg);

	if (mutex_lock_interruptible(&serv->lock))
		return;
	if (function >= 0 &&
	    function < serv->func_tab_sz &&
	    serv->func_tab[function].func) {
#if DEBUG_DUMP_MESSAGE
		if (rpc_qmsg_service(qmsg) == DEBUG_DUMP_MESSAGE) {
			pr_info("%s got message\n", __func__);
			rpc_dump_msg((rpc_msg *)qmsg);
		}
#endif
		serv->func_tab[function].func(qmsg->tunnel, &(qmsg->msg));
		rpc_queue_msg_pool_free(msg_pool, qmsg);
	} else {
		rpc_queue_msg *redundant =
			rpc_remove_matching_from_queue(serv->orphan_queue,qmsg);
		if (redundant)
			rpc_queue_msg_pool_free(msg_pool, redundant);
		pr_info("%s service (%d) rx unregistered function %d, \n"
			"queued to be processed later.\n",
			serv->thread_name,
			(int)rpc_qmsg_service(qmsg),
			rpc_qmsg_function(qmsg));
		qmsg = rpc_add_to_queue_tail(serv->orphan_queue, qmsg);
		if (qmsg) {
			/* queue has reached limit */
			rpc_queue_msg_pool_free(msg_pool, qmsg);
		}
	}
	mutex_unlock(&serv->lock);
}

static int service_task(void *srv)
{
	rpc_service *serv = (rpc_service *)srv;

	while (1) {
		rpc_queue_msg *qmsg = rpc_remove_head_from_queue(serv->rcv_queue);
		if (qmsg) {
			if (check_msg_version(&qmsg->msg)) {
				msg_version_error(&qmsg->msg);
				rpc_queue_msg_pool_free(msg_pool, qmsg);
				serv->rx_err_cnt++;
			} else if (rpc_qmsg_reply(qmsg)) {
				handle_reply(serv, qmsg);
			} else if (serv->active) {
				handle_request(serv, qmsg);
			} else {
				pr_info("service task %s is inactive, tossing request\n",
					serv->thread_name);
				rpc_queue_msg_pool_free(msg_pool, qmsg);
				serv->rx_err_cnt++;
			}
		}
	}
	return 0;
}

static int rpc_service_init(int service)
{
	rpc_service *serv = &itc_rpc_services[service];

	if (serv->thread_name == NULL) {
		pr_err("%s: skipping undefined service %d\n",
		       __func__, service);
		return -EINVAL;
	}
	if (serv->active == false) {
		pr_err("%s: skipping inactive %s service\n",
		       __func__, serv->thread_name);
		return -EINVAL;
	}
	pr_info("%s: initializing %s service\n", __func__, serv->thread_name);

	mutex_init(&serv->lock);
	serv->rcv_queue = kmalloc(sizeof(rpc_queue), GFP_KERNEL);
	serv->req_queue = kmalloc(sizeof(rpc_queue), GFP_KERNEL);
	serv->orphan_queue = kmalloc(sizeof(rpc_queue), GFP_KERNEL);
	if (serv->rcv_queue == 0 ||
	    serv->req_queue == 0 ||
	    serv->orphan_queue == 0) {
		pr_err("Error allocating memory \n");
		return -ENOMEM;
	}
	rpc_init_queue(serv->rcv_queue);
	rpc_init_queue(serv->req_queue);
	rpc_init_queue(serv->orphan_queue);
	rpc_set_queue_limit(serv->orphan_queue, RPC_ORPHAN_QUEUE_LIMIT, true);

	serv->thread = kthread_run(service_task, serv, serv->thread_name);
	if (IS_ERR(serv->thread)) {
		pr_err("Fatal Error: Unable to run %s service task\n",
		       serv->thread_name);
		return -EFAULT;
	}
	if (serv->func_tab)
		rpc_register_functions(service,
				       serv->func_tab, serv->func_tab_sz);
	return 0;
}

/* register function table */
int rpc_register_functions(int service, rpc_function *func_tab, int func_cnt)
{
	int status;
        rpc_queue_msg *qmsg, *dejavu;
	rpc_service *serv = &itc_rpc_services[service];

	if (serv->thread_name == NULL) {
		pr_err("ERROR %s service %d: no thread name\n", __func__, service);
		return -EINVAL;
	}
	if (serv->registered) {
		pr_err("%s ERROR %d functions already registered for %s service\n",
		       __func__, serv->func_tab_sz, serv->thread_name);
		return -EBUSY;
	}
	if (serv->thread == NULL) {
		serv->active = true;
		rpc_service_init(service);
	}
	status = mutex_lock_interruptible(&serv->lock);
	if (status)
		return status;
	serv->func_tab = func_tab;
	serv->func_tab_sz = func_cnt;
	mutex_unlock(&serv->lock);
	serv->registered = true;
	pr_info("%s registered %s service with %d functions\n",
		__func__, serv->thread_name, func_cnt);

	dejavu = 0;
        while ((qmsg = rpc_try_remove_head_from_queue(serv->orphan_queue))) {
		int function = rpc_qmsg_function(qmsg);
		if (serv->func_tab[function].func) {
			rpc_add_to_queue_tail(serv->rcv_queue, qmsg);
		} else {
			/* for someone else so put it back */
			rpc_add_to_queue_tail(serv->orphan_queue, qmsg);

			if (dejavu == 0)
				dejavu = qmsg;
			else if (qmsg == dejavu) {
				mutex_unlock(&serv->lock);
				break;
			}
		}
	}
	return 0;
}
EXPORT_SYMBOL(rpc_register_functions);

/* register a single function */
int rpc_register_function(int service, int func_idx, rpc_function *func)
{
	rpc_service *serv = &itc_rpc_services[service];

	if (serv->thread_name == NULL || func == NULL)
		return -EINVAL;

	if (serv->func_tab && func_idx >= 0 && func_idx < serv->func_tab_sz) {
		rpc_queue_msg *qmsg;

		serv->func_tab[func_idx] = *func;

		/* lets see if there are orphan messages for this func */
		while ((qmsg = rpc_remove_func_from_queue(serv->orphan_queue, func_idx)))
			rpc_add_to_queue_tail(serv->rcv_queue, qmsg);
		return 0;
	}
	pr_err("%s Error %s service: func cnt %d function index %d\n",
	       __func__, serv->thread_name, serv->func_tab_sz, func_idx);
	return -EFAULT;
}
EXPORT_SYMBOL(rpc_register_function);

/* register function table */
int rpc_unregister_functions(int service)
{
	int status;
	rpc_queue_msg *qmsg;
	rpc_service *serv = &itc_rpc_services[service];

	if (serv->thread_name == NULL) {
		pr_err("%s ERROR service %d: no thread name\n", __func__, service);
		return -EINVAL;
	}
	status = mutex_lock_interruptible(&serv->lock);
	if (status)
		return status;
	serv->func_tab = NULL;
	serv->func_tab_sz = 0;
	mutex_unlock(&serv->lock);
	serv->registered = false;
	pr_info("%s unregistered %s service\n",
		__func__, serv->thread_name);

        while ((qmsg = rpc_try_remove_head_from_queue(serv->rcv_queue)))
		rpc_queue_msg_pool_free(msg_pool, qmsg);
	return 0;
}
EXPORT_SYMBOL(rpc_unregister_functions);

/* unregister a single function */
int rpc_unregister_function(int service, int func_idx)
{
	int status;
	rpc_service *serv = &itc_rpc_services[service];

	if (serv->thread_name == NULL)
		return -EINVAL;

	status = mutex_lock_interruptible(&serv->lock);
	if (status)
		return status;
	if (serv->func_tab && func_idx >= 0 && func_idx < serv->func_tab_sz) {
		rpc_queue_msg *qmsg;

		serv->func_tab[func_idx].func = NULL;
		mutex_unlock(&serv->lock);
		while ((qmsg = rpc_remove_func_from_queue(serv->rcv_queue, func_idx)))
			rpc_queue_msg_pool_free(msg_pool, qmsg);
		pr_info("%s unregistered %s service function %d\n",
			__func__, serv->thread_name, func_idx);
		return 0;
	}
	mutex_unlock(&serv->lock);
	pr_err("%s Error %s service: func cnt %d function index %d\n",
	       __func__, serv->thread_name, serv->func_tab_sz, func_idx);
	return -EFAULT;
}
EXPORT_SYMBOL(rpc_unregister_function);

int rpc_send_reply(int tunnel, rpc_msg *msg)
{
	int service, status;
	fifo_tunnel *ft;
	rpc_service *serv;

	service = rpc_msg_service(msg);
	if (service <= 0 || service >= RPC_MAX_SERVICES) {
		pr_err("Error: %s invalid service %d\n",__func__, service);
		return -EINVAL;
	}
	serv = &itc_rpc_services[service];

	if (tunnel < 0 || tunnel >= RPC_TUNNELS_MAX || !tunnels[tunnel]) {
		pr_err("%s Error invalid FIFO tunnel %d\n", __func__, tunnel);
		serv->tx_err_cnt++;
		return -EINVAL;
	}
	ft = tunnels[tunnel];

	rpc_msg_set_reply(msg, 1);
	if (!ft->link_up) {
		down(&ft->lock);
		up(&ft->lock);
	}
	serv->tx_cnt++;
	status = rpc_fifo_tx(ft->tx_fifo_h, msg);
#if DEBUG_DUMP_MESSAGE
	if (rpc_msg_service(msg) == DEBUG_DUMP_MESSAGE) {
		pr_info("%s\n", __func__);
		rpc_dump_msg(msg);
	}
#endif
	if (status) {
		pr_err("%s Error unable to send FIFO %d a message\n",
		       __func__, ft->tx_fifo);
		serv->tx_err_cnt++;
	}
	return status;
}
EXPORT_SYMBOL(rpc_send_reply);

int rpc_send_request_timeout(int tunnel, rpc_msg *msg, int sec)
{
	int service, status;
	fifo_tunnel *ft;
	rpc_queue_msg qmsg;
	rpc_service *serv;
	unsigned long jiffies_to;

	if(sec)
		jiffies_to =  __msecs_to_jiffies(sec*1000);

	pr_debug("%s tunnel_id %d with timeout set to %d\n", __func__, tunnel, sec);
	service = rpc_msg_service(msg);
	if (service <= 0 || service >= RPC_MAX_SERVICES) {
		pr_err("Error: %s invalid service %d\n",__func__, service);
		return -EINVAL;
	}
	serv = &itc_rpc_services[service];

	if (!serv->active) {
		pr_err("%s service %s id %dis not active\n", __func__,
		       serv->thread_name, service);
		return -EINVAL;
	}
	if (tunnel < 0 || tunnel >= RPC_TUNNELS_MAX || !tunnels[tunnel]) {
		pr_err("%s Error invalid FIFO tunnel %d\n", __func__, tunnel);
		serv->tx_err_cnt++;
		return -EINVAL;
	}
	ft = tunnels[tunnel];

	rpc_msg_set_counter(msg, serv->req_count++);
	rpc_msg_set_request(msg, 1);
	qmsg.msg.header = msg->header;

	sema_init(&qmsg.sema, 0);
	rpc_add_to_queue_tail(serv->req_queue, &qmsg);
	if (!ft->link_up) {
		if (sec) {
			if (down_timeout(&ft->lock, jiffies_to)) {
				rpc_remove_xid_from_queue(serv->req_queue, rpc_qmsg_xid(&qmsg));
				serv->tx_err_cnt++;
				pr_err("%s timeout waiting for init on tunnel %d (%s)\n",
				       __func__, tunnel, ft->name);
				return -EAGAIN;
			}
		} else if (down_interruptible(&ft->lock)) {
			rpc_remove_xid_from_queue(serv->req_queue, rpc_qmsg_xid(&qmsg));
			serv->tx_err_cnt++;
			pr_err("%s no init on tunnel %d (%s)\n",
			       __func__, tunnel, ft->name);
			return -EINTR;
		}
		up(&ft->lock);
	}
	serv->tx_cnt++;
	status = rpc_fifo_tx(ft->tx_fifo_h, msg);
	if (status) {
		pr_err("%s Error: unable to send TX FIFO %d on %s a message\n",
		       __func__, ft->tx_fifo, ft->fifo_dev);
		serv->tx_err_cnt++;
		return -EIO;
	}
	status = rpc_fifo_enable_rx_cb(ft->rx_fifo_h);
	if (status)
		pr_err("%s Error: unable to enable RX FIFO %d on %s callbacks\n",
		       __func__, ft->rx_fifo, ft->fifo_dev);
#if DEBUG_DUMP_MESSAGE
	if (rpc_msg_service(msg) == DEBUG_DUMP_MESSAGE) {
		pr_info("%s\n", __func__);
		rpc_dump_msg(msg);
	}
#endif
	if (sec) {
		if (down_timeout(&qmsg.sema, jiffies_to)) {
			rpc_remove_xid_from_queue(serv->req_queue, rpc_qmsg_xid(&qmsg));
			serv->tx_err_cnt++;
			pr_err("%s tunnel is busy try again %d (%s)\n", __func__, tunnel, ft->name);
			return -EBUSY;
		}
	} else if (down_interruptible(&qmsg.sema)) {
		rpc_remove_xid_from_queue(serv->req_queue, rpc_qmsg_xid(&qmsg));
		serv->tx_err_cnt++;
		return -EINTR;
	}
	msg->header  = qmsg.msg.header;
	msg->data[0] = qmsg.msg.data[0];
	msg->data[1] = qmsg.msg.data[1];
	msg->data[2] = qmsg.msg.data[2];

#if DEBUG_DUMP_MESSAGE
	if (rpc_msg_service(msg) == DEBUG_DUMP_MESSAGE) {
		pr_info("%s GOT REPLY\n", __func__);
		rpc_dump_msg(msg);
	}
#endif
	return status;
}
EXPORT_SYMBOL(rpc_send_request_timeout);

int rpc_send_request(int tunnel, rpc_msg *msg)
{
	return rpc_send_request_timeout(tunnel, msg, 0);
}
EXPORT_SYMBOL(rpc_send_request);

int rpc_send_message(int tunnel, rpc_msg *msg, bool wait_for_link)
{
	int service, status;
	fifo_tunnel *ft;
	rpc_service *serv;

	service = rpc_msg_service(msg);
	if (service <= 0 || service >= RPC_MAX_SERVICES) {
		pr_err("Error: %s invalid service %d\n",__func__, service);
		return -EINVAL;
	}
	serv = &itc_rpc_services[service];

	if (tunnel < 0 || tunnel >= RPC_TUNNELS_MAX || !tunnels[tunnel]) {
		pr_err("%s Error invalid FIFO tunnel %d\n", __func__, tunnel);
		serv->tx_err_cnt++;
		return -EINVAL;
	}
	ft = tunnels[tunnel];

	if (!ft->link_up) {
		if (!wait_for_link) {
			serv->tx_err_cnt++;
			return -EBUSY;
		}
		down(&ft->lock);
		up(&ft->lock);
	}
#if DEBUG_DUMP_MESSAGE
	if (rpc_msg_service(msg) == DEBUG_DUMP_MESSAGE) {
		pr_info("%s\n", __func__);
		rpc_dump_msg(msg);
	}
#endif
	status = rpc_fifo_tx(ft->tx_fifo_h, msg);
	serv->tx_cnt++;
	if (status) {
		pr_err("%s Error: unable to send TX FIFO %d on %s a message\n",
		       __func__, ft->tx_fifo, ft->fifo_dev);
		serv->tx_err_cnt++;
	}
	return status;
}
EXPORT_SYMBOL(rpc_send_message);

/*
 * Only for use when a crash occurs and we are certain we are
 * in single CPU, non-preemptible context. We must not do anything
 * that will sleep or cause the scheduler to attempt to run.
 *
 */
int rpc_receive_message_crash(int tunnel, rpc_msg *msg, int msec)
{
	int service = -1, status = 0;
	fifo_tunnel *ft;
	int melapsed;

	if (num_online_cpus() > 1 || preemptible()) {
		pr_err("%s Error: called from non-atomic context!\n",
		       __func__);
		status = -EINVAL;
		goto done;
	}

	if (tunnel < 0 || tunnel >= RPC_TUNNELS_MAX || !tunnels[tunnel]) {
		pr_err("%s Error: invalid FIFO tunnel %d\n", __func__, tunnel);
		status = -EINVAL;
		goto done;
	}
	ft = tunnels[tunnel];

	/* Poll until message is received or timeout. */
	for (melapsed = 0;
	     !status && melapsed < msec;
	     melapsed += RPC_RX_CRASH_POLL_DELAY_MS)  {
		status = rpc_fifo_rx(ft->rx_fifo_h, msg);
		if (status < 0) {
			pr_err("%s Error: unable to receive a message from FIFO %d on %s\n",
			       __func__, ft->rx_fifo, ft->fifo_dev);
			status = -EIO;
			goto done;
		}
		mdelay(RPC_RX_CRASH_POLL_DELAY_MS);
	}
	if (melapsed >= msec) {
		pr_err("%s Error: timeout waiting to receive a message from FIFO %d on %s\n",
		       __func__, ft->rx_fifo, ft->fifo_dev);
		status = -ETIME;
		goto done;
	}

	service = rpc_msg_service(msg);
	if (service <= 0 || service >= RPC_MAX_SERVICES) {
		pr_err("Error: %s invalid service %d\n",__func__, service);
		status = -ERANGE;
		goto done;
	}

done:
#if DEBUG_DUMP_MESSAGE
	if (service == DEBUG_DUMP_MESSAGE) {
		pr_info("%s\n", __func__);
		rpc_dump_msg(msg);
	}
#endif
	return status;
}
EXPORT_SYMBOL(rpc_receive_message_crash);

void rpc_dump_msg(rpc_msg *msg)
{
	pr_info("msg %p Ver %d Req %d Rep %d Serv %d Func %d reqcnt %d\n",
		msg, rpc_msg_version(msg), rpc_msg_request(msg),rpc_msg_reply(msg),
		rpc_msg_service(msg),rpc_msg_function(msg),rpc_msg_counter(msg));
	pr_info("%08x %08x %08x %08x\n",
		msg->header, msg->data[0], msg->data[1], msg->data[2]);
}
EXPORT_SYMBOL(rpc_dump_msg);

static int send_handshake_message(fifo_tunnel *ft, uint32_t code)
{
	int status;
	rpc_msg msg;
	rpc_service *serv = &itc_rpc_services[RPC_SERVICE_INIT];;

	rpc_msg_init(&msg, RPC_SERVICE_INIT, 0, serv->func_tab[0].version, code, rpc_version, 0);

	status = rpc_fifo_tx(ft->tx_fifo_h, &msg);
	if (status)
		pr_err("%s failed sending handshake %08x message tunnel %s on %s\n",
		       __func__, code, ft->name, ft->fifo_dev);
	else
		pr_info("%s sent handshake %08x message tunnel %s on %s\n",
			__func__, code, ft->name, ft->fifo_dev);
	return status;
}

int init_service_handshake(int tunnel, rpc_msg *msg)
{
	int status = 0;
	fifo_tunnel *ft;

	if (tunnel < 0 || tunnel >= RPC_TUNNELS_MAX || !tunnels[tunnel]) {
		pr_err("%s Error invalid FIFO tunnel %d\n", __func__, tunnel);
		status = -1;
		goto done;
	}
	ft = tunnels[tunnel];

	if (msg->data[0] == RPC_MSG_INIT_CODE0) {
		pr_info("%s received RPC_MSG_INIT_CODE0 on tunnel %d\n",
			__func__, tunnel);
		status = send_handshake_message(ft, RPC_MSG_INIT_CODE1);
		if (status) {
			pr_err("%s Unable to send TX FIFO %d on %s a message\n",
			       __func__, ft->tx_fifo, ft->fifo_dev);
			tunnel_init_error(ft);
		}
	} else if (msg->data[0] == RPC_MSG_INIT_CODE1) {
		pr_info("%s received RPC_MSG_INIT_CODE1 on tunnel %d\n",
			__func__, tunnel);
		status = send_handshake_message(ft, RPC_MSG_INIT_CODE2);
		if (status) {
			pr_err("%s Unable to send TX FIFO %d on %s a message\n",
			       __func__, ft->tx_fifo, ft->fifo_dev);
			tunnel_init_error(ft);
		}
	} else if (msg->data[0] == RPC_MSG_INIT_CODE2) {
		pr_info("%s received RPC_MSG_INIT_CODE2 on tunnel %d\n",
			__func__, tunnel);
		if (msg->data[1] != rpc_version) {
			tunnel_version_error(msg->data[1]);
			status = -EFAULT;
			goto done;
		}
		ft->link_up = true;
		up(&ft->lock);	/* allow transmit */
		pr_info("Init complete for FIFO tunnel %s\n", ft->name);
		device_unblock_probing();
	} else {
		pr_err("%s Error invalid init message on FIFO tunnel %s\n",
		       __func__, ft->name);
		tunnel_init_error(ft);
		status = -EFAULT;
	}

  done:
	return status;
}

int init_service_err(int tunnel, rpc_msg *msg)
{
	int status = 0;
	fifo_tunnel *ft;

	if (tunnel < 0 || tunnel >= RPC_TUNNELS_MAX || !tunnels[tunnel]) {
		pr_err("%s Error invalid FIFO tunnel %d\n", __func__, tunnel);
		status = -1;
		goto done;
	}
	ft = tunnels[tunnel];

	pr_info("%s received \"%s\" error message on tunnel %s\n",
		 __func__,
		init_err_rc_str[(-INIT_SVC_ERR_RC_HANDSHAKE & 0xff) -
				(msg->data[0] & 0xff)],
		ft->name);

	/*
	 * TODO: Add handling of INIT_SVC_ERR msg here. We should only receive
	 * one of these messages when we are the CPU that booted some other RPC-
	 * connected CPU (i.e. such as on the 3390). On the SMC-based SoCs this
	 * shouldn't ever be reached.
	 */

done:
	return status;
}

int itc_rpc_msg_pool_count(void)
{
	return msg_pool->free_cnt.count;
}

int rpc_tunnel_info(struct seq_file *m)
{
	int i;
	fifo_tunnel *ft;
	for (i = 0; i < RPC_TUNNELS_MAX; i++) {
		if (tunnels[i] == NULL)
			continue;
		ft = tunnels[i];
		seq_printf(m, "Tunnel %-2d %-15s: link(%d) handshake(%d) DQM:txq %02d | rxq %02d | dev %-15s\n",
			   ft->id, ft->name, ft->link_up, ft->handshake,
			   ft->rx_fifo, ft->tx_fifo, ft->fifo_dev);
	}
	return 0;
}

static int rpc_probe(struct platform_device *pdev)
{
	int status = 0;
	fifo_tunnel *ft;
	struct fifo_cb cb;
	int i;
	unsigned long flags;

	ft = kmalloc(sizeof(fifo_tunnel), GFP_KERNEL);
	if (!ft) {
		status = -ENOMEM;
		goto done;
	}
	memset(ft, 0, sizeof(fifo_tunnel));

	ft->link_up = false;

	pdev->dev.platform_data = ft;
	ft->pdev = pdev;

	status = rpc_parse_dt_node(pdev);
	if (status)
		goto err_free_dt;

	spin_lock_irqsave(&tunnels_lock, flags);
	for (i = 0; i < RPC_TUNNELS_MAX; i++) {
		if (tunnels[i] == NULL) {
			tunnels[i] = ft;
			ft->id = i;
			break;
		}
	}
	spin_unlock_irqrestore(&tunnels_lock, flags);
	if (i == RPC_TUNNELS_MAX) {
		pr_err("%s Exceeded max tunnels!\n", __func__);
		status = -EFAULT;
		goto err_free_dt;
	}
	pr_debug("Initializing RPC tunnel %s on %s tx-fifo %d rx-fifo %d\n",
		 ft->name, ft->fifo_dev, ft->tx_fifo, ft->rx_fifo);

	cb.fn = NULL;
	cb.context = NULL;
	ft->tx_fifo_h = rpc_fifo_register(ft->fifo_dev,
					  ft->tx_fifo,
					  &cb,
					  RPC_FIFO_F_TX);
	if (!ft->tx_fifo_h) {
		pr_err("%s unable to acquire TX FIFO %d on device %s\n",
		       __func__, ft->tx_fifo, ft->fifo_dev);
		status = -EFAULT;
		goto err_free_dt;
	}
	cb.fn = rx_isr;
	cb.context = (void *)(uintptr_t)ft->id;
	ft->rx_fifo_h = rpc_fifo_register(ft->fifo_dev,
					  ft->rx_fifo,
					  &cb,
					  RPC_FIFO_F_RX);
	if (!ft->rx_fifo_h) {
		pr_err("%s unable to acquire RX FIFO %d on device %s\n",
		       __func__, ft->rx_fifo, ft->fifo_dev);
		status = -EFAULT;
		goto err_free_dt;
	}

	if (ft->boot_tunnel)
		boot_ft = ft;

	sema_init(&ft->lock, 0); /* lock pending initialization */

	if (!ft->handshake) {
		rpc_tunnel_replay_orphans(ft);
		ft->link_up = true;
		up(&ft->lock);
		pr_debug("no handshake on tunnel %s on %sd\n",
			 ft->name, ft->fifo_dev);
	}

	status = rpc_fifo_enable_rx_cb(ft->rx_fifo_h);
	if (status) {
		pr_err("%s unable to enable FIFO callbacks\n", __func__);
		goto done;
	}

	if (ft->handshake) {
		pr_debug("starting handshake on tunnel %s on %s\n",
			 ft->name, ft->fifo_dev);
		status = send_handshake_message(ft, RPC_MSG_INIT_CODE0);
	}

	goto done;

err_free_dt:
	kfree(ft);

done:
	return status;
}

static int rpc_remove(struct platform_device *pdev)
{
	int status = 0;
	fifo_tunnel *ft = pdev->dev.platform_data;

	if (!ft) {
		pr_err("%s release called with uninitialized platform_data\n",
		       __func__);
		status = -EINVAL;
		goto done;
	}

	if (ft->id < 0 || ft->id >= RPC_TUNNELS_MAX) {
		pr_err("%s Error tunnel %d invalid\n", __func__, ft->id);
		status = -EINVAL;
		goto done;
	}

	status = rpc_fifo_disable_rx_cb(ft->rx_fifo_h);
	if (status) {
		pr_err("%s unable to disable FIFO callback\n", __func__);
		goto done;
	}

	tunnels[ft->id] = NULL;

	status = rpc_fifo_release(ft->rx_fifo_h, RPC_FIFO_F_RX);
	if (status) {
		pr_err("%s unable to release RX FIFO %d on device %s\n",
		       __func__, ft->rx_fifo, ft->fifo_dev);
		goto done;
	}
	status = rpc_fifo_release(ft->tx_fifo_h, RPC_FIFO_F_TX);
	if (status) {
		pr_err("%s unable to release TX FIFO %d on device %s\n",
		       __func__, ft->tx_fifo, ft->fifo_dev);
		goto done;
	}

	kfree(ft);

  done:
	return status;
}

static const struct of_device_id rpc_of_match[] = {
	{.compatible = "brcm,itc-rpc"},
	{}
};

MODULE_DEVICE_TABLE(of, rpc_of_match);

static struct platform_driver rpc_driver = {
	.probe  = rpc_probe,
	.remove = rpc_remove,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = rpc_of_match
	}
};

int __init rpc_init(void)
{
	int status = 0;
	int i;
	struct device_node *np;

	pr_debug("%s driver v%s\n", MODULE_NAME, MODULE_VER);

	if (msg_pool) {
		pr_err("Benign Warning: Redundant rpc_init()\n");
		status = 0;
		goto done;
	}

	np = of_find_node_by_path("/rpc");
	if (!np) {
		pr_err("Error: unable to retrieve DT node \"rpc\"\n");
		status = -EINVAL;
		goto done;
	}
	status = of_property_read_u32(np, "rpc-version", &rpc_version);
	if (status) {
		pr_err("Error: unable to retrieve RPC version from \"rpc\" node\n");
		goto done;
	}
	of_node_put(np);

	memset(tunnels, 0, sizeof(tunnels));
	spin_lock_init(&tunnels_lock);

	msg_pool = rpc_queue_msg_pool_create(RPC_MSG_POOL_MSG_CNT);

	for (i = 0; i < RPC_MAX_SERVICES; i++)
		rpc_service_init(i);

	platform_driver_register(&rpc_driver);

	itc_proc_init();

  done:
	return status;
}

postcore_initcall(rpc_init);

MODULE_LICENSE("GPL");
