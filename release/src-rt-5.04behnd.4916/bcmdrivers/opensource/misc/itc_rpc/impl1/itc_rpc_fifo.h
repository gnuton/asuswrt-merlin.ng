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

#ifndef _ITC_RPC_FIFO_H_
#define _ITC_RPC_FIFO_H_

#include <itc_msg_defs.h>

#define RPC_FIFO_F_TX	0x00000001	/* FIFO TX client */
#define RPC_FIFO_F_RX	0x00000002	/* FIFO RX client */
typedef irqreturn_t (*fifo_isr_callback)(void *fifo_h, void *context,
					 u32 flags);
struct fifo_cb {
	fifo_isr_callback fn;	/* callback for ISR's */
	void *context;		/* user provided context */
};

void *rpc_fifo_register(char *fifo_dev, u32 fifo, struct fifo_cb *cb,
			u32 flags);
int rpc_fifo_release(void *fifo_h, u32 flags);
int rpc_fifo_enable_rx_cb(void *fifo_h);
int rpc_fifo_disable_rx_cb(void *fifo_h);
int rpc_fifo_rx(void *fifo_h, rpc_msg *msg);
int rpc_fifo_tx(void *fifo_h, rpc_msg *msg);

#endif
