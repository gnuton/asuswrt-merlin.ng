/* Copyright (c) 2020 The Linux Foundation. All rights reserved.
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

#include <linux/coresight.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/socket.h>
#include <linux/udp.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <net/net_namespace.h>
#include <net/udp.h>
#include <net/udp_tunnel.h>
#include <linux/net.h>
#include <linux/inet.h>

#include "coresight-priv.h"
#include "coresight-tmc.h"
extern struct tmc_drvdata *tmc_drvdata_stream;

struct udp_port_cfg udp_conf;
extern void tmc_enable_hw(struct tmc_drvdata *drvdata);
#define INT_MODULE_PARM(n, v) static int n = v; \
				module_param(n, int, 0444)
#define STRING_MODULE_PARM(s, v) static char *s = v; \
				module_param(s, charp, 0000)

/* insmod coresight-stream.ko stream_dst_port=5004
 * stream_dst_addr="10.201.2.100" stream_src_port=5006
 * stream_src_addr="10.201.11.50"
 */
INT_MODULE_PARM(stream_src_port, 5005);
MODULE_PARM_DESC(stream_src_port, "UDP src port");
INT_MODULE_PARM(stream_dst_port, 5004);
MODULE_PARM_DESC(stream_dst_port, "UDP dst port");
STRING_MODULE_PARM(stream_src_addr, "127.0.0.1");
MODULE_PARM_DESC(stream_src_addr, "IPv4 src address");
STRING_MODULE_PARM(stream_dst_addr, "127.0.0.1");
MODULE_PARM_DESC(stream_dst_addr, "IPv4 src address");
STRING_MODULE_PARM(stream_board_id, "AABBCCDD");
MODULE_PARM_DESC(stream_board_id, "Board Id");

unsigned int udp_packet_no;
#define QLD_STREAM_PORT 5004
#define SEQ_NO_SIZE 4

void qld_stream_work_hdlr(struct work_struct *work)
{
	struct tmc_drvdata *drvdata = container_of(work, struct tmc_drvdata,
							qld_stream_work);
	struct etr_sg_table *etr_table = drvdata->sysfs_buf->private;
	struct tmc_sg_table *sg_table = etr_table->sg_table;
	sgte_t *ptr = sg_table->table_vaddr;
	int ret = 0;
	int i = 0;
	struct page *transfer_page;
	void *vaddr, *next_page;
	unsigned long flags;
	unsigned int need_to_enable;
	int wrapped_around_seq_no;

	need_to_enable = 0;

	spin_lock_irqsave(&drvdata->spinlock, flags);
	if (atomic_read(&drvdata->seq_no) >= COMP_PAGES_PER_DATA) {
		wrapped_around_seq_no = (atomic_read(&drvdata->seq_no) %
					TOTAL_PAGES_PER_DATA);
		need_to_enable = 1;
	}
	spin_unlock_irqrestore(&drvdata->spinlock, flags);

	for (i = atomic_read(&drvdata->completed_seq_no);
		(i < atomic_read(&drvdata->seq_no))
		&& (i < TOTAL_PAGES_PER_DATA) ; i++) {

		vaddr = phys_to_virt(ETR_SG_ADDR(ptr[i]));
		next_page = vaddr + PAGE_SIZE;
		udp_packet_no++;
		*(unsigned int *)next_page = udp_packet_no;
		strlcpy(next_page+sizeof(udp_packet_no), stream_board_id, 32);
		dmac_flush_range((void *)next_page, (void *)next_page
					+ SEQ_NO_SIZE + 32);
		transfer_page = virt_to_page(vaddr);
		ret = kernel_sendpage(drvdata->qld_stream_sock, transfer_page,
				0, PAGE_SIZE + SEQ_NO_SIZE + 32, MSG_DONTWAIT);
		if (ret < PAGE_SIZE + SEQ_NO_SIZE)
			pr_emerg("ERROR: Can't send 4096 bytes %d\n", ret);
		atomic_inc(&drvdata->completed_seq_no);
	}

	if (need_to_enable) {
		spin_lock_irqsave(&drvdata->spinlock, flags);
		if (atomic_read(&drvdata->completed_seq_no)
				== TOTAL_PAGES_PER_DATA) {
			atomic_set(&drvdata->completed_seq_no, 0);
			atomic_set(&drvdata->seq_no, wrapped_around_seq_no);
		}
		CS_UNLOCK(drvdata->base);
		tmc_enable_hw(drvdata);
		CS_LOCK(drvdata->base);
		spin_unlock_irqrestore(&drvdata->spinlock, flags);
	}
}
EXPORT_SYMBOL(qld_stream_work_hdlr);

int setup_kernel_qld_socket(struct tmc_drvdata *drvdata)
{
	struct socket *sock;
	int err = 0;

	memset(&udp_conf, 0, sizeof(udp_conf));
	udp_conf.family = AF_INET;
	udp_conf.local_ip.s_addr = in_aton(stream_src_addr);
	udp_conf.local_udp_port = htons(stream_src_port);
	udp_conf.peer_udp_port = htons(stream_dst_port);
	udp_conf.use_udp_checksums = 1;
	udp_conf.peer_ip.s_addr = in_aton(stream_dst_addr);

	/* Open QLD stream socket */
	err = udp_sock_create(&init_net, &udp_conf, &sock);
	if (err < 0) {
		pr_emerg("ERROR: Could not create QLD STREAM socket\n");
		return -err;
	}

	drvdata->qld_stream_sock = sock;
	INIT_WORK(&drvdata->qld_stream_work, qld_stream_work_hdlr);

	return 0;
}

static void __exit coresight_stream_fini(void)
{
	flush_work(&tmc_drvdata_stream->qld_stream_work);

	kernel_sock_shutdown(tmc_drvdata_stream->qld_stream_sock, SHUT_RDWR);
	sock_release(tmc_drvdata_stream->qld_stream_sock);
}

static int __init coresight_stream_init(void)
{
	setup_kernel_qld_socket(tmc_drvdata_stream);
	return 0;
}
module_init(coresight_stream_init);
module_exit(coresight_stream_fini);

MODULE_LICENSE("GPL v2");
