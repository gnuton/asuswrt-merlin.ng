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
 * itc_rpc.h
 * Peter Sulc
 *
 *******************************************************************************/

#ifndef _ITC_RPC_H_
#define _ITC_RPC_H_

#include <itc_msg_defs.h>
#include <itc_channel_defs.h>

int rpc_receive_message_crash(int tunnel, rpc_msg *msg, int msec);

static inline void rpc_msg_init_header(rpc_msg *msg,
				       int service,
				       int function,
				       int version)
{
	msg->header = RPC_MSG_MAKE_HEADER(version, 0, 0, service, function, 1);
}

static inline void rpc_msg_init(rpc_msg *msg,
				int service,
				int function,
				int version,
				uint32_t data0,
				uint32_t data1,
				uint32_t data2)
{
	msg->header = RPC_MSG_MAKE_HEADER(version, 0, 0, service, function, 1);
	msg->data[0] = data0;
	msg->data[1] = data1;
	msg->data[2] = data2;
}

/*
 * get an ID for a named DQM tunnel
 * multiple clients may simultaneously use the same tunnel
 * name must be NULL terminated
 * returns -1 if tunnel does not exist
 */
int rpc_get_fifo_tunnel_id(char *name);

/*
 * functions used by the service handler to handle incoming messages/requests
 * the rpc_msg * memory is owned and maintained by ItcRpc
 */

typedef int (*rpc_rx_handler)(int tunnel, rpc_msg *msg);

typedef struct {
	rpc_rx_handler func;
	int version;
} rpc_function;

/*
 * service function table that is an
 * array of function pointers that correspond to the function indexes
 * you have defined somewhere in your code.
 */

int rpc_register_functions(int service,
			   rpc_function *func_table,
			   int func_cnt);

/*
 * register a single function
 * must be done after you have rpc_register_functions()
 * this is useful when you have a function in a module
 * that will be loaded later than the rpc_register_functions()
 */
int rpc_register_function(int service,
			  int function,
			  rpc_function *func);

/*
 * unregister a previously registered service function table
 */
int rpc_unregister_functions(int service);

/*
 * unregister a previously registered service function
 */
int rpc_unregister_function(int service, int function);

/*
 * Reply to message.
 * Used by request handler.
 * If you are replying to a message from your service callback function
 * you MUST use this because this api will set the reply
 * bit so that the other side can do the right thing.
 */
int rpc_send_reply(int tunnel, rpc_msg *msg);

/*
 * functions used to send messages
 */

/* wait for response
 * rpc_send_request cannot be used in one of your callback functions
 * on the service side. There you can only use rpc_send_reply, or
 * rpc_send_message.
 * Ideally your callback funtion would only send rpc_send_reply
 * or nothing at all.
 */
int rpc_send_request(int tunnel, rpc_msg *msg);
int rpc_send_request_timeout(int tunnel, rpc_msg *msg, int sec);

/*
 * do not wait for response
 * will block if wait_for_link is true and the tunnel has not completed init
 * do not use this to reply to a message, use rpc_send_reply
 */
int rpc_send_message(int tunnel, rpc_msg *msg, bool wait_for_link);


/* convenience functions */

static inline int rpc_send_requesta(int tunnel,
				    int service,
				    int function,
				    int version,
				    uint32_t *data0,
				    uint32_t *data1,
				    uint32_t *data2)
{
	int rc;
	rpc_msg msg;
	rpc_msg_init(&msg, service, function, version, *data0, *data1, *data2);
	rc = rpc_send_request(tunnel, &msg);
	if (rc == 0)
	{
		*data0 = msg.data[0];
		*data1 = msg.data[1];
		*data2 = msg.data[2];
	}
	return rc;
}

static inline int rpc_send_messagea(int tunnel,
				    int service,
				    int function,
				    int version,
				    uint32_t data0,
				    uint32_t data1,
				    uint32_t data2)
{
	rpc_msg msg;
	rpc_msg_init(&msg, service, function, version, data0, data1, data2);
	return rpc_send_message(tunnel, &msg, true);
}


/* debug */
void rpc_dump_msg(rpc_msg *msg);

/* init everything */
int rpc_init(void);
void rpc_cleanup(void);

#endif


