#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/rhashtable.h>
#include <linux/netdevice.h>
#include <net/netfilter/nf_flow_table.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack_tuple.h>

static DEFINE_SPINLOCK(flow_offload_hw_pending_list_lock);
static LIST_HEAD(flow_offload_hw_pending_list);

static DEFINE_MUTEX(nf_flow_offload_hw_mutex);

struct flow_offload_hw {
	struct list_head	list;
	enum flow_offload_type	type;
	struct flow_offload	*flow;
	struct nf_conn		*ct;

	struct flow_offload_hw_path src;
	struct flow_offload_hw_path dest;
};

static void flow_offload_check_ethernet(struct flow_offload_tuple *tuple,
                    struct dst_entry *dst,
					struct flow_offload_hw_path *path)
{
	struct net_device *dev = path->dev;
	struct neighbour *n;

	if (dev->type != ARPHRD_ETHER)
		return;

	memcpy(path->eth_src, path->dev->dev_addr, ETH_ALEN);
    n = dst_neigh_lookup(dst, &tuple->src_v4);
	if (!n)
		return;

	memcpy(path->eth_dest, n->ha, ETH_ALEN);
	path->flags |= FLOW_OFFLOAD_PATH_ETHERNET;
	neigh_release(n);
}

static int flow_offload_check_path(struct net *net,
				   struct flow_offload_tuple *tuple,
                   struct dst_entry *dst,
				   struct flow_offload_hw_path *path)
{
	struct net_device *dev;

	dev = dev_get_by_index_rcu(net, tuple->iifidx);
	if (!dev)
		return -ENOENT;

	path->dev = dev;
	flow_offload_check_ethernet(tuple, dst, path);

	if (dev->netdev_ops->ndo_flow_offload_check)
		return dev->netdev_ops->ndo_flow_offload_check(path);

	return 0;
}

static int do_flow_offload_hw(struct flow_offload_hw *offload)
{
	struct net_device *src_dev = offload->src.dev;
	struct net_device *dest_dev = offload->dest.dev;
	int ret;

	ret = src_dev->netdev_ops->ndo_flow_offload(offload->type,
						    offload->flow,
						    &offload->src,
						    &offload->dest);

	/* restore devices in case the driver mangled them */
	offload->src.dev = src_dev;
	offload->dest.dev = dest_dev;

	return ret;
}

static void flow_offload_hw_free(struct flow_offload_hw *offload)
{
	dev_put(offload->src.dev);
	dev_put(offload->dest.dev);
	if (offload->ct)
		nf_conntrack_put(&offload->ct->ct_general);
	list_del(&offload->list);
	kfree(offload);
}

static void flow_offload_hw_work(struct work_struct *work)
{
	struct flow_offload_hw *offload, *next;
	LIST_HEAD(hw_offload_pending);

	spin_lock_bh(&flow_offload_hw_pending_list_lock);
	list_replace_init(&flow_offload_hw_pending_list, &hw_offload_pending);
	spin_unlock_bh(&flow_offload_hw_pending_list_lock);

	list_for_each_entry_safe(offload, next, &hw_offload_pending, list) {
		mutex_lock(&nf_flow_offload_hw_mutex);
		switch (offload->type) {
		case FLOW_OFFLOAD_ADD:
			if (nf_ct_is_dying(offload->ct))
				break;

			if (do_flow_offload_hw(offload) >= 0)
				offload->flow->flags |= FLOW_OFFLOAD_HW;
			break;
		case FLOW_OFFLOAD_DEL:
			do_flow_offload_hw(offload);
			break;
		}
		mutex_unlock(&nf_flow_offload_hw_mutex);

		flow_offload_hw_free(offload);
	}
}

static void flow_offload_queue_work(struct flow_offload_hw *offload)
{
	spin_lock_bh(&flow_offload_hw_pending_list_lock);
	list_add_tail(&offload->list, &flow_offload_hw_pending_list);
	spin_unlock_bh(&flow_offload_hw_pending_list_lock);

	schedule_work(&nf_flow_offload_hw_work);
}

static struct flow_offload_hw *
flow_offload_hw_prepare(struct net *net, struct flow_offload *flow)
{
	struct flow_offload_hw_path src = {};
	struct flow_offload_hw_path dest = {};
	struct flow_offload_tuple *tuple_s, *tuple_d;
    struct flow_offload_hw *offload = NULL;

	rcu_read_lock_bh();

    tuple_s = &flow->tuplehash[FLOW_OFFLOAD_DIR_ORIGINAL].tuple;
    tuple_d = &flow->tuplehash[FLOW_OFFLOAD_DIR_REPLY].tuple;

    if (flow_offload_check_path(net, tuple_s, tuple_d->dst_cache, &src))
		goto out;

    if (flow_offload_check_path(net, tuple_d, tuple_s->dst_cache, &dest))
		goto out;

	if (!src.dev->netdev_ops->ndo_flow_offload)
		goto out;

	offload = kzalloc(sizeof(struct flow_offload_hw), GFP_ATOMIC);
	if (!offload)
		goto out;

	dev_hold(src.dev);
	dev_hold(dest.dev);
	offload->src = src;
	offload->dest = dest;
	offload->flow = flow;

out:
	rcu_read_unlock_bh();

	return offload;
}

static void flow_offload_hw_add(struct net *net, struct flow_offload *flow,
				struct nf_conn *ct)
{
	struct flow_offload_hw *offload;

	offload = flow_offload_hw_prepare(net, flow);
	if (!offload)
		return;

	nf_conntrack_get(&ct->ct_general);
	offload->type = FLOW_OFFLOAD_ADD;
	offload->ct = ct;

	flow_offload_queue_work(offload);
}

static void flow_offload_hw_del(struct net *net, struct flow_offload *flow)
{
	struct flow_offload_hw *offload;

	offload = flow_offload_hw_prepare(net, flow);
	if (!offload)
		return;

	offload->type = FLOW_OFFLOAD_DEL;

	flow_offload_queue_work(offload);
}

static const struct nf_flow_table_hw flow_offload_hw = {
	.add	= flow_offload_hw_add,
	.del	= flow_offload_hw_del,
	.owner	= THIS_MODULE,
};

static int __init nf_flow_table_hw_module_init(void)
{
	INIT_WORK(&nf_flow_offload_hw_work, flow_offload_hw_work);
	nf_flow_table_hw_register(&flow_offload_hw);

	return 0;
}

static void __exit nf_flow_table_hw_module_exit(void)
{
	struct flow_offload_hw *offload, *next;
	LIST_HEAD(hw_offload_pending);

	nf_flow_table_hw_unregister(&flow_offload_hw);
	cancel_work_sync(&nf_flow_offload_hw_work);

	list_for_each_entry_safe(offload, next, &hw_offload_pending, list)
		flow_offload_hw_free(offload);
}

module_init(nf_flow_table_hw_module_init);
module_exit(nf_flow_table_hw_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pablo Neira Ayuso <pablo@netfilter.org>");
MODULE_ALIAS("nf-flow-table-hw");
