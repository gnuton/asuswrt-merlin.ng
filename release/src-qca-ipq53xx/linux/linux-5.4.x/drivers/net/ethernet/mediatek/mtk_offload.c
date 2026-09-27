/*   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   Copyright (C) 2018 John Crispin <john@phrozen.org>
 */

#include <net/netfilter/nf_flow_table.h>
#include "mtk_eth_soc.h"

static int
mtk_offload_prepare_v4(struct mtk_eth *eth, struct mtk_foe_entry *entry,
		       struct flow_offload_tuple *s_tuple,
		       struct flow_offload_tuple *d_tuple,
		       struct flow_offload_hw_path *src,
		       struct flow_offload_hw_path *dest)
{
	int dest_port = 1;

	if (dest->dev == eth->netdev[1])
	    dest_port = 2;

	mtk_foe_entry_prepare(entry, MTK_PPE_PKT_TYPE_IPV4_HNAPT, s_tuple->l4proto,
			      dest_port, dest->eth_src, dest->eth_dest);
	mtk_foe_entry_set_ipv4_tuple(entry, false,
				     s_tuple->src_v4.s_addr, s_tuple->src_port,
				     s_tuple->dst_v4.s_addr, s_tuple->dst_port);
	mtk_foe_entry_set_ipv4_tuple(entry, true,
				     d_tuple->dst_v4.s_addr, d_tuple->dst_port,
				     d_tuple->src_v4.s_addr, d_tuple->src_port);

	if (dest->flags & FLOW_OFFLOAD_PATH_PPPOE)
		mtk_foe_entry_set_pppoe(entry, dest->pppoe_sid);

	if (dest->flags & FLOW_OFFLOAD_PATH_VLAN)
		mtk_foe_entry_set_vlan(entry, dest->vlan_id);

	if (dest->flags & FLOW_OFFLOAD_PATH_DSA)
		mtk_foe_entry_set_dsa(entry, dest->dsa_port);

	return 0;
}

int mtk_flow_offload_add(struct mtk_eth *eth,
			 enum flow_offload_type type,
			 struct flow_offload *flow,
			 struct flow_offload_hw_path *src,
			 struct flow_offload_hw_path *dest)
{
	struct flow_offload_tuple *otuple = &flow->tuplehash[FLOW_OFFLOAD_DIR_ORIGINAL].tuple;
	struct flow_offload_tuple *rtuple = &flow->tuplehash[FLOW_OFFLOAD_DIR_REPLY].tuple;
	struct mtk_foe_entry orig, reply;
	u32 ohash, rhash, timestamp;

	if (otuple->l4proto != IPPROTO_TCP && otuple->l4proto != IPPROTO_UDP)
		return -EINVAL;

	if (type == FLOW_OFFLOAD_DEL) {
		ohash = (unsigned long)flow->priv;
		rhash = ohash >> 16;
		ohash &= 0xffff;
		mtk_foe_entry_clear(&eth->ppe, ohash);
		mtk_foe_entry_clear(&eth->ppe, rhash);
		rcu_assign_pointer(eth->foe_flow_table[ohash], NULL);
		rcu_assign_pointer(eth->foe_flow_table[rhash], NULL);
		synchronize_rcu();

		return 0;
	}

	switch (otuple->l3proto) {
	case AF_INET:
		if (mtk_offload_prepare_v4(eth, &orig, otuple, rtuple, src, dest) ||
		    mtk_offload_prepare_v4(eth, &reply, rtuple, otuple, dest, src))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	timestamp = mtk_r32(eth, 0x0010);

	ohash = mtk_foe_entry_commit(&eth->ppe, &orig, timestamp);
	if (ohash < 0)
		return -EINVAL;

	rhash = mtk_foe_entry_commit(&eth->ppe, &reply, timestamp);
	if (rhash < 0) {
		mtk_foe_entry_clear(&eth->ppe, ohash);
		return -EINVAL;
	}

	rcu_assign_pointer(eth->foe_flow_table[ohash], flow);
	rcu_assign_pointer(eth->foe_flow_table[rhash], flow);

	ohash |= rhash << 16;
	flow->priv = (void *)(unsigned long)ohash;

	return 0;
}

static void mtk_offload_keepalive(struct mtk_eth *eth, unsigned int hash)
{
	struct flow_offload *flow;

	rcu_read_lock();
	flow = rcu_dereference(eth->foe_flow_table[hash]);
	if (flow)
		flow->timeout = jiffies + 30 * HZ;
	rcu_read_unlock();
}

int mtk_offload_check_rx(struct mtk_eth *eth, struct sk_buff *skb, u32 rxd4)
{
	unsigned int hash;

	switch (FIELD_GET(MTK_RXD4_PPE_CPU_REASON, rxd4)) {
	case MTK_PPE_CPU_REASON_KEEPALIVE_UC_OLD_HDR:
	case MTK_PPE_CPU_REASON_KEEPALIVE_MC_NEW_HDR:
	case MTK_PPE_CPU_REASON_KEEPALIVE_DUP_OLD_HDR:
		hash = FIELD_GET(MTK_RXD4_FOE_ENTRY, rxd4);
		mtk_offload_keepalive(eth, hash);
		return -1;
	case MTK_PPE_CPU_REASON_PACKET_SAMPLING:
		return -1;
	default:
		return 0;
	}
}

int mtk_flow_offload_init(struct mtk_eth *eth)
{
	eth->foe_flow_table = devm_kcalloc(eth->dev, MTK_PPE_ENTRIES,
					   sizeof(*eth->foe_flow_table),
					   GFP_KERNEL);

	if (!eth->foe_flow_table)
		return -ENOMEM;

	return 0;
}
