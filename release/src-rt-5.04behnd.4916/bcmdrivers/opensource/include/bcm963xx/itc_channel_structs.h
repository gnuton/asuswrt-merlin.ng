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
 * itc_channel_structs.h
 * Oct. 23 2012
 * Peter Sulc
 *
 *******************************************************************************/
#ifndef _ITC_CHANNEL_STRUCTS_H_
#define _ITC_CHANNEL_STRUCTS_H_

#include <itc_rpc.h>
#include <itc_msg_q.h>

/* service and FIFO tunnel structures */

/* services */
typedef struct
{
	struct mutex		lock;
	const char		*thread_name;
	struct task_struct	*thread;
	rpc_function		*func_tab;
	int			func_tab_sz;
	rpc_queue		*rcv_queue;
	rpc_queue		*req_queue;
	rpc_queue		*orphan_queue;
	bool			active;
	bool			registered;
	u32			req_count;
	u32			tx_cnt;
	u32			rx_cnt;
	u32			tx_err_cnt;
	u32			rx_err_cnt;
} rpc_service;

/* FIFO tunnels */
typedef struct
{
	struct platform_device	*pdev;
	char			name[16];
	int			id;
	bool			link_up;
	char			fifo_dev[16];
	u32			tx_fifo;
	u32			rx_fifo;
	void			*tx_fifo_h;
	void			*rx_fifo_h;
	struct semaphore	lock;
	u32			orph_tok;
	bool			handshake;
	bool			boot_tunnel;
} fifo_tunnel;

#endif
