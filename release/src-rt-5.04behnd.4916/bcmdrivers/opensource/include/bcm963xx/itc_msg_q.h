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
 * itc_msg_q.h
 * Oct. 23 2012
 * Peter Sulc
 *
 *******************************************************************************/
#ifndef _ITC_MSG_Q_H_
#define _ITC_MSG_Q_H_

#include <linux/semaphore.h>
#include <itc_rpc.h>

/* queue management */

typedef struct rpc_queue_msg
{
	rpc_msg			msg;
	int 			tunnel;	/* incoming tunnel */
	struct semaphore	sema;  	/* for waiting on roundtrip requests */
	struct rpc_queue_msg	*next;
}	rpc_queue_msg;

static inline u8 rpc_qmsg_version(rpc_queue_msg *qmsg)
{
	return rpc_msg_version(&qmsg->msg);
}
static inline u8 rpc_qmsg_request(rpc_queue_msg *qmsg)
{
	return rpc_msg_request(&qmsg->msg);
}
static inline u8 rpc_qmsg_reply(rpc_queue_msg *qmsg)
{
	return rpc_msg_reply(&qmsg->msg);
}
static inline u8 rpc_qmsg_service(rpc_queue_msg *qmsg)
{
	return rpc_msg_service(&qmsg->msg);
}
static inline u8 rpc_qmsg_function(rpc_queue_msg *qmsg)
{
	return rpc_msg_function(&qmsg->msg);
}
static inline u8 rpc_qmsg_counter(rpc_queue_msg *qmsg)
{
	return rpc_msg_counter(&qmsg->msg);
}
static inline u32 rpc_qmsg_xid(rpc_queue_msg *qmsg)
{
	return rpc_msg_xid(&qmsg->msg);
}

static inline void rpc_qmsg_set_version(rpc_queue_msg *qmsg, u8 v)
{
	rpc_msg_set_version(&qmsg->msg, v);
}
static inline void rpc_qmsg_set_request(rpc_queue_msg *qmsg, u8 v)
{
	rpc_msg_set_request(&qmsg->msg, v);
}
static inline void rpc_qmsg_set_reply(rpc_queue_msg *qmsg, u8 v)
{
	rpc_msg_set_reply(&qmsg->msg, v);
}
static inline void rpc_qmsg_set_service(rpc_queue_msg *qmsg, u8 v)
{
	rpc_msg_set_service(&qmsg->msg, v);
}
static inline void rpc_qmsg_set_function(rpc_queue_msg *qmsg, u8 v)
{
	rpc_msg_set_function(&qmsg->msg, v);
}
static inline void rpc_qmsg_set_counter(rpc_queue_msg *qmsg, u8 v)
{
	rpc_msg_set_counter(&qmsg->msg, v);
}

typedef struct
{
	spinlock_t		lock;
	struct semaphore  	sema;	/* for counting and waiting */
	rpc_queue_msg		*head;
	rpc_queue_msg		*tail;
	int			limit; /* queue limit */
	bool			leaky_bucket; /* leaks if full */
}	rpc_queue;

extern void rpc_init_queue(rpc_queue *q);
extern rpc_queue_msg *rpc_add_to_queue_tail(rpc_queue *q, rpc_queue_msg *msg);
extern rpc_queue_msg *rpc_remove_head_from_queue(rpc_queue *q);
extern rpc_queue_msg *rpc_try_remove_head_from_queue(rpc_queue *q);
extern rpc_queue_msg *rpc_remove_xid_from_queue(rpc_queue *q, u32 xid);
extern rpc_queue_msg *rpc_remove_func_from_queue(rpc_queue *q, u32 function);
extern rpc_queue_msg *rpc_remove_matching_from_queue(rpc_queue *q, rpc_queue_msg *msg);
extern void rpc_dump_queue(rpc_queue *q);
extern void rpc_dump_qmsg(rpc_queue_msg *msg);

static inline void rpc_set_queue_limit(rpc_queue *q, int limit, bool leaky)
{
	q->limit = limit;
	q->leaky_bucket = leaky;
}

/*
 * rpc_queue_msg pool
 */

typedef struct
{
	spinlock_t		lock;
	struct semaphore	free_cnt;
	rpc_queue_msg		*bottom;
	rpc_queue_msg		*top;
	rpc_queue_msg		*free_pool;
}	rpc_queue_msg_pool;

extern rpc_queue_msg_pool *rpc_queue_msg_pool_create(int cnt);
extern rpc_queue_msg *rpc_queue_msg_pool_alloc(rpc_queue_msg_pool *qp);
extern int rpc_queue_msg_pool_free(rpc_queue_msg_pool *qp, rpc_queue_msg *msg);

static inline int rpc_msg_free(rpc_queue_msg_pool *qp, rpc_msg *msg)
{
	return rpc_queue_msg_pool_free(qp, (rpc_queue_msg *)msg);
}

#endif
