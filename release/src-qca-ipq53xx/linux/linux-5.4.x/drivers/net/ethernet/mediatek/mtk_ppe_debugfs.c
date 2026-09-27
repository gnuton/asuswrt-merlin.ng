/*   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   Copyright (C) 2014-2016 Sean Wang <sean.wang@mediatek.com>
 *   Copyright (C) 2016-2017 John Crispin <blogic@openwrt.org>
 *   Copyright (C) 2020 Felix Fietkau <nbd@nbd.name>
 */

#include <linux/kernel.h>
#include <linux/debugfs.h>
#include "mtk_eth_soc.h"

static const char *mtk_foe_entry_state_str[] = {
	"INVALID",
	"UNBIND",
	"BIND",
	"FIN"
};

static const char *mtk_foe_packet_type_str[] = {
	"IPV4_HNAPT",
	"IPV4_HNAT",
	"IPV6_1T_ROUTE",
	"IPV4_DSLITE",
	"IPV6_3T_ROUTE",
	"IPV6_5T_ROUTE",
	"IPV6_6RD",
};

#define es(entry)		(mtk_foe_entry_state_str[FIELD_GET(MTK_FOE_IB1_STATE, entry->ib1)])
//#define ei(entry, end)		(MTK_PPE_TBL_SZ - (int)(end - entry))
#define pt(entry)		(mtk_foe_packet_type_str[FIELD_GET(MTK_FOE_IB1_PACKET_TYPE, entry->ib1)])

static int mtk_ppe_debugfs_foe_show(struct seq_file *m, void *private)
{
	struct mtk_ppe *ppe = m->private;
	int i, count;

	for (i = 0, count = 0; i < MTK_PPE_ENTRIES; i++) {
		struct mtk_foe_entry *entry = &ppe->foe_table[i];

		if (!FIELD_GET(MTK_FOE_IB1_STATE, entry->ib1))
			continue;

		if (FIELD_GET(MTK_FOE_IB1_PACKET_TYPE, entry->ib1) ==
		    MTK_PPE_PKT_TYPE_IPV4_HNAPT) {
			struct mtk_foe_ipv4 *ip4 = &entry->ipv4;
			struct mtk_foe_mac_info *l2 = &ip4->l2;

			__be32 saddr = htonl(ip4->orig.src_ip);
			__be32 daddr = htonl(ip4->orig.dest_ip);
			__be32 nsaddr = htonl(ip4->new.src_ip);
			__be32 ndaddr = htonl(ip4->new.dest_ip);
			unsigned char h_dest[ETH_ALEN];
			unsigned char h_source[ETH_ALEN];

			*((__be32 *) h_source) = htonl(l2->src_mac_hi);
			*((__be16*) &h_source[4]) = htons(l2->src_mac_lo);
			*((__be32*) h_dest) = htonl(l2->dest_mac_hi);
			*((__be16*) &h_dest[4]) = htons(l2->dest_mac_lo);
			seq_printf(m,
				   "(%x)0x%05x|state=%s|type=%s|"
				   "%pI4:%d->%pI4:%d=>%pI4:%d->%pI4:%d|%pM=>%pM|"
				   "etype=0x%04x|info1=0x%x|info2=0x%x|"
				   "vlan1=%d|vlan2=%d\n",
				   count, i, es(entry), pt(entry),
				   &saddr, ip4->orig.src_port,
				   &daddr, ip4->orig.dest_port,
				   &nsaddr, ip4->new.src_port,
				   &ndaddr, ip4->new.dest_port,
				   h_source, h_dest,
				   ntohs(l2->etype),
				   entry->ib1,
				   ip4->ib2,
				   l2->vlan1,
				   l2->vlan2);
			count++;
		} else
			seq_printf(m, "0x%05x state=%s\n", count, es(entry));
	}

	return 0;
}

static int mtk_ppe_debugfs_foe_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_ppe_debugfs_foe_show, inode->i_private);
}

static const struct file_operations mtk_ppe_debugfs_foe_fops = {
	.open = mtk_ppe_debugfs_foe_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int mtk_ppe_debugfs_init(struct mtk_ppe *ppe)
{
	struct dentry *root;

	root = debugfs_create_dir("mtk_ppe", NULL);
	if (!root)
		return -ENOMEM;

	debugfs_create_file("entries", S_IRUGO, root, ppe, &mtk_ppe_debugfs_foe_fops);

	return 0;
}
