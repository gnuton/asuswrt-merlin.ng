#ifndef _NF_FLOW_TABLE_H
#define _NF_FLOW_TABLE_H

#include <linux/in.h>
#include <linux/in6.h>
#include <linux/netdevice.h>
#include <linux/rhashtable-types.h>
#include <linux/rcupdate.h>
#include <linux/netfilter.h>
#include <linux/netfilter/nf_conntrack_tuple_common.h>
#include <net/dst.h>

struct nf_flowtable;

struct nf_flowtable_type {
	struct list_head		list;
	int				family;
	int				(*init)(struct nf_flowtable *ft);
	void				(*free)(struct nf_flowtable *ft);
	nf_hookfn			*hook;
	struct module			*owner;
};

enum nf_flowtable_flags {
	NF_FLOWTABLE_F_HW		= 0x1,
};

struct nf_flowtable {
	struct list_head		list;
	struct rhashtable		rhashtable;
	const struct nf_flowtable_type	*type;
	u32				flags;
	struct delayed_work		gc_work;
	possible_net_t			ft_net;
};

enum flow_offload_tuple_dir {
	FLOW_OFFLOAD_DIR_ORIGINAL = IP_CT_DIR_ORIGINAL,
	FLOW_OFFLOAD_DIR_REPLY = IP_CT_DIR_REPLY,
	FLOW_OFFLOAD_DIR_MAX = IP_CT_DIR_MAX
};

struct flow_offload_tuple {
	union {
		struct in_addr		src_v4;
		struct in6_addr		src_v6;
	};
	union {
		struct in_addr		dst_v4;
		struct in6_addr		dst_v6;
	};
	struct {
		__be16			src_port;
		__be16			dst_port;
	};

	int				iifidx;

	u8				l3proto;
	u8				l4proto;
	u8				dir;

	u16				mtu;

	struct dst_entry		*dst_cache;
};

struct flow_offload_tuple_rhash {
	struct rhash_head		node;
	struct flow_offload_tuple	tuple;
};

#define FLOW_OFFLOAD_SNAT	0x1
#define FLOW_OFFLOAD_DNAT	0x2
#define FLOW_OFFLOAD_DYING	0x4
#define FLOW_OFFLOAD_TEARDOWN	0x8
#define FLOW_OFFLOAD_HW		0x10
#define FLOW_OFFLOAD_KEEP	0x20

struct flow_offload {
	struct flow_offload_tuple_rhash		tuplehash[FLOW_OFFLOAD_DIR_MAX];
	u32					flags;
	u32					timeout;
	union {
		/* Your private driver data here. */
		void *priv;
	};
};

#define FLOW_OFFLOAD_PATH_ETHERNET	BIT(0)
#define FLOW_OFFLOAD_PATH_VLAN		BIT(1)
#define FLOW_OFFLOAD_PATH_PPPOE		BIT(2)
#define FLOW_OFFLOAD_PATH_DSA		BIT(3)

struct flow_offload_hw_path {
	struct net_device *dev;
	u32 flags;

	u8 eth_src[ETH_ALEN];
	u8 eth_dest[ETH_ALEN];
	u16 vlan_proto;
	u16 vlan_id;
	u16 pppoe_sid;
	u16 dsa_port;
};

#define NF_FLOW_TIMEOUT (30 * HZ)

struct nf_flow_route {
	struct {
		struct dst_entry	*dst;
	} tuple[FLOW_OFFLOAD_DIR_MAX];
};

struct flow_offload *flow_offload_alloc(struct nf_conn *ct,
					struct nf_flow_route *route);
void flow_offload_free(struct flow_offload *flow);

int flow_offload_add(struct nf_flowtable *flow_table, struct flow_offload *flow);
struct flow_offload_tuple_rhash *flow_offload_lookup(struct nf_flowtable *flow_table,
						     struct flow_offload_tuple *tuple);
void nf_flow_table_cleanup(struct net_device *dev);

int nf_flow_table_init(struct nf_flowtable *flow_table);
void nf_flow_table_free(struct nf_flowtable *flow_table);

void flow_offload_teardown(struct flow_offload *flow);
static inline void flow_offload_dead(struct flow_offload *flow)
{
	flow->flags |= FLOW_OFFLOAD_DYING;
}

int nf_flow_table_iterate(struct nf_flowtable *flow_table,
                      void (*iter)(struct flow_offload *flow, void *data),
                      void *data);

int nf_flow_snat_port(const struct flow_offload *flow,
		      struct sk_buff *skb, unsigned int thoff,
		      u8 protocol, enum flow_offload_tuple_dir dir);
int nf_flow_dnat_port(const struct flow_offload *flow,
		      struct sk_buff *skb, unsigned int thoff,
		      u8 protocol, enum flow_offload_tuple_dir dir);

struct flow_ports {
	__be16 source, dest;
};

unsigned int nf_flow_offload_ip_hook(void *priv, struct sk_buff *skb,
				     const struct nf_hook_state *state);
unsigned int nf_flow_offload_ipv6_hook(void *priv, struct sk_buff *skb,
				       const struct nf_hook_state *state);

void nf_flow_offload_hw_add(struct net *net, struct flow_offload *flow,
			    struct nf_conn *ct);
void nf_flow_offload_hw_del(struct net *net, struct flow_offload *flow);

struct nf_flow_table_hw {
	struct module	*owner;
	void		(*add)(struct net *net, struct flow_offload *flow,
			       struct nf_conn *ct);
	void		(*del)(struct net *net, struct flow_offload *flow);
};

int nf_flow_table_hw_register(const struct nf_flow_table_hw *offload);
void nf_flow_table_hw_unregister(const struct nf_flow_table_hw *offload);

void nf_flow_table_acct(struct flow_offload *flow, struct sk_buff *skb, int dir);

extern struct work_struct nf_flow_offload_hw_work;

#define MODULE_ALIAS_NF_FLOWTABLE(family)	\
	MODULE_ALIAS("nf-flowtable-" __stringify(family))

#endif /* _NF_FLOW_TABLE_H */
