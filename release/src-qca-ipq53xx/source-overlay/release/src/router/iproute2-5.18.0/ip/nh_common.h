/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __NH_COMMON_H__
#define __NH_COMMON_H__ 1

#include <list.h>

#define NH_CACHE_SIZE		1024

struct nha_res_grp {
	__u16			buckets;
	__u32			idle_timer;
	__u32			unbalanced_timer;
	__u64			unbalanced_time;
};

struct nh_entry {
	struct hlist_node	nh_hash;

	__u32			nh_id;
	__u32			nh_oif;
	__u32			nh_flags;
	__u16			nh_grp_type;
	__u8			nh_family;
	__u8			nh_scope;
	__u8			nh_protocol;

	bool			nh_blackhole;
	bool			nh_fdb;

	int			nh_gateway_len;
	union {
		__be32		ipv4;
		struct in6_addr	ipv6;
	}			nh_gateway;

	struct rtattr		*nh_encap;
	union {
		struct rtattr   rta;
		__u8		_buf[RTA_LENGTH(sizeof(__u16))];
	}			nh_encap_type;

	bool			nh_has_res_grp;
	struct nha_res_grp	nh_res_grp;

	int			nh_groups_cnt;
	struct nexthop_grp	*nh_groups;
};

void print_cache_nexthop_id(FILE *fp, const char *fp_prefix, const char *jsobj,
			    __u32 nh_id);
int print_cache_nexthop(struct nlmsghdr *n, void *arg, bool process_cache);

#endif /* __NH_COMMON_H__ */
