/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_MROUTE6_H
#define __LINUX_MROUTE6_H


#include <linux/pim.h>
#include <linux/skbuff.h>	/* for struct sk_buff_head */
#include <net/net_namespace.h>
#include <uapi/linux/mroute6.h>
#include <linux/mroute_base.h>
#include <net/fib_rules.h>

#ifdef CONFIG_IPV6_MROUTE
static inline int ip6_mroute_opt(int opt)
{
	return (opt >= MRT6_BASE) && (opt <= MRT6_MAX);
}
#else
static inline int ip6_mroute_opt(int opt)
{
	return 0;
}
#endif

struct sock;

#ifdef CONFIG_IPV6_MROUTE
extern int ip6_mroute_setsockopt(struct sock *, int, char __user *, unsigned int);
extern int ip6_mroute_getsockopt(struct sock *, int, char __user *, int __user *);
extern int ip6_mr_input(struct sk_buff *skb);
extern int ip6mr_ioctl(struct sock *sk, int cmd, void __user *arg);
extern int ip6mr_compat_ioctl(struct sock *sk, unsigned int cmd, void __user *arg);
extern int ip6_mr_init(void);
extern void ip6_mr_cleanup(void);
#else
static inline
int ip6_mroute_setsockopt(struct sock *sock,
			  int optname, char __user *optval, unsigned int optlen)
{
	return -ENOPROTOOPT;
}

static inline
int ip6_mroute_getsockopt(struct sock *sock,
			  int optname, char __user *optval, int __user *optlen)
{
	return -ENOPROTOOPT;
}

static inline
int ip6mr_ioctl(struct sock *sk, int cmd, void __user *arg)
{
	return -ENOIOCTLCMD;
}

static inline int ip6_mr_init(void)
{
	return 0;
}

static inline void ip6_mr_cleanup(void)
{
	return;
}
#endif

#ifdef CONFIG_IPV6_MROUTE_MULTIPLE_TABLES
bool ip6mr_rule_default(const struct fib_rule *rule);
#else
static inline bool ip6mr_rule_default(const struct fib_rule *rule)
{
	return true;
}
#endif

#define VIFF_STATIC 0x8000

struct mfc6_cache_cmp_arg {
	struct in6_addr mf6c_mcastgrp;
	struct in6_addr mf6c_origin;
};

struct mfc6_cache {
	struct mr_mfc _c;
	union {
		struct {
			struct in6_addr mf6c_mcastgrp;
			struct in6_addr mf6c_origin;
		};
		struct mfc6_cache_cmp_arg cmparg;
	};
};

#define MFC_ASSERT_THRESH (3*HZ)		/* Maximal freq. of asserts */

#define IP6MR_MFC_EVENT_UPDATE   1
#define IP6MR_MFC_EVENT_DELETE   2

struct rtmsg;
extern int ip6mr_get_route(struct net *net, struct sk_buff *skb,
			   struct rtmsg *rtm, u32 portid);

/*
 * Callback to registered modules in the event of updates to a multicast group
 */
typedef void (*ip6mr_mfc_event_offload_callback_t)(struct in6_addr *origin,
						   struct in6_addr *group,
						   u32 max_dest_dev,
						   u32 dest_dev_idx[],
						   uint8_t op);

/*
 * Register the callback used to inform offload modules when updates occur
 * to MFC. The callback is registered by offload modules
 */
extern bool ip6mr_register_mfc_event_offload_callback(
			ip6mr_mfc_event_offload_callback_t mfc_offload_cb);

/*
 * De-Register the callback used to inform offload modules when updates occur
 * to MFC
 */
extern void ip6mr_unregister_mfc_event_offload_callback(void);

/*
 * Find the destination interface list given a multicast group and source
 */
extern int ip6mr_find_mfc_entry(struct net *net, struct in6_addr *origin,
				struct in6_addr *group, u32 max_dst_cnt,
				u32 dest_dev[]);

/*
 * Out-of-band multicast statistics update for flows that are offloaded from
 * Linux
 */
extern int ip6mr_mfc_stats_update(struct net *net, struct in6_addr *origin,
				  struct in6_addr *group, uint64_t pkts_in,
				  uint64_t bytes_in, uint64_t pkts_out,
				  uint64_t bytes_out);

#ifdef CONFIG_IPV6_MROUTE
bool mroute6_is_socket(struct net *net, struct sk_buff *skb);
extern int ip6mr_sk_done(struct sock *sk);
#else
static inline bool mroute6_is_socket(struct net *net, struct sk_buff *skb)
{
	return false;
}
static inline int ip6mr_sk_done(struct sock *sk)
{
	return 0;
}
#endif
#endif
