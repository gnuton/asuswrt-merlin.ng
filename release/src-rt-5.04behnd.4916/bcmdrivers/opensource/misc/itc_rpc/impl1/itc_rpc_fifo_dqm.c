 /****************************************************************************
 *
 * Broadcom Proprietary and Confidential. (c) 2018 Broadcom.  All rights reserved.
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
#include <linux/types.h>
#include <itc_msg_defs.h>
#include <msgfifo.h>
#include "dqm.h"
#include "itc_rpc_fifo.h"

void *rpc_fifo_register(char *fifo_dev, u32 fifo, struct fifo_cb *cb, u32 flags)
{
	u8 msg_size;
	struct dqm_cb dqm_cb = {};
	u32 dqm_flags = 0;;

	dqm_cb.fn = cb->fn;
	dqm_cb.context = cb->context;
	dqm_flags |= (flags & RPC_FIFO_F_TX) ? DQM_F_TX : 0;
	dqm_flags |= (flags & RPC_FIFO_F_RX) ? DQM_F_RX : 0;
	return dqm_register(fifo_dev, fifo, &dqm_cb, &msg_size,
			    dqm_flags);
}

int rpc_fifo_release(void *fifo_h, u32 flags)
{
	u32 dqm_flags = 0;

	dqm_flags |= (flags & RPC_FIFO_F_TX) ? DQM_F_TX : 0;
	dqm_flags |= (flags & RPC_FIFO_F_RX) ? DQM_F_RX : 0;
	return dqm_release(fifo_h, dqm_flags);
}

int rpc_fifo_enable_rx_cb(void *fifo_h)
{
	return dqm_enable_rx_cb(fifo_h);
}

int rpc_fifo_disable_rx_cb(void *fifo_h)
{
	return dqm_disable_rx_cb(fifo_h);
}

int rpc_fifo_rx(void *fifo_h, rpc_msg *msg)
{
	int status;
	struct msgstruct dqm_msg;

	status = dqm_rx(fifo_h, 1 ,&dqm_msg);
	msg->header = dqm_msg.msgdata[0];
	msg->data[0] = dqm_msg.msgdata[1];
	msg->data[1] = dqm_msg.msgdata[2];
	msg->data[2] = dqm_msg.msgdata[3];
	return status;
}

int rpc_fifo_tx(void *fifo_h, rpc_msg *msg)
{
	struct msgstruct dqm_msg;

	dqm_msg.msglen = 4;
	dqm_msg.msgdata[0] = msg->header;
	dqm_msg.msgdata[1] = msg->data[0];
	dqm_msg.msgdata[2] = msg->data[1];
	dqm_msg.msgdata[3] = msg->data[2];
	return dqm_tx(fifo_h, &dqm_msg);
}
