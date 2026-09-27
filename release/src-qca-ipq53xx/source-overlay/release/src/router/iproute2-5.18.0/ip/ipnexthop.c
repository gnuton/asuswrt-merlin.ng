// SPDX-License-Identifier: GPL-2.0
/*
 * ip nexthop
 *
 * Copyright (c) 2017-19 David Ahern <dsahern@gmail.com>
 */

#include <linux/nexthop.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <rt_names.h>
#include <errno.h>

#include "utils.h"
#include "ip_common.h"
#include "nh_common.h"

static struct {
	unsigned int flushed;
	unsigned int groups;
	unsigned int ifindex;
	unsigned int master;
	unsigned int proto;
	unsigned int fdb;
	unsigned int id;
	unsigned int nhid;
} filter;

enum {
	IPNH_LIST,
	IPNH_FLUSH,
};

#define RTM_NHA(h)  ((struct rtattr *)(((char *)(h)) + \
			NLMSG_ALIGN(sizeof(struct nhmsg))))

static struct hlist_head nh_cache[NH_CACHE_SIZE];
static struct rtnl_handle nh_cache_rth = { .fd = -1 };

static void usage(void) __attribute__((noreturn));

static void usage(void)
{
	fprintf(stderr,
		"Usage: ip nexthop { list | flush } [ protocol ID ] SELECTOR\n"
		"       ip nexthop { add | replace } id ID NH [ protocol ID ]\n"
		"       ip nexthop { get | del } id ID\n"
		"       ip nexthop bucket list BUCKET_SELECTOR\n"
		"       ip nexthop bucket get id ID index INDEX\n"
		"SELECTOR := [ id ID ] [ dev DEV ] [ vrf NAME ] [ master DEV ]\n"
		"            [ groups ] [ fdb ]\n"
		"BUCKET_SELECTOR := SELECTOR | [ nhid ID ]\n"
		"NH := { blackhole | [ via ADDRESS ] [ dev DEV ] [ onlink ]\n"
		"        [ encap ENCAPTYPE ENCAPHDR ] |\n"
		"        group GROUP [ fdb ] [ type TYPE [ TYPE_ARGS ] ] }\n"
		"GROUP := [ <id[,weight]>/<id[,weight]>/... ]\n"
		"TYPE := { mpath | resilient }\n"
		"TYPE_ARGS := [ RESILIENT_ARGS ]\n"
		"RESILIENT_ARGS := [ buckets BUCKETS ] [ idle_timer IDLE ]\n"
		"                  [ unbalanced_timer UNBALANCED ]\n"
		"ENCAPTYPE := [ mpls ]\n"
		"ENCAPHDR := [ MPLSLABEL ]\n");
	exit(-1);
}

static int nh_dump_filter(struct nlmsghdr *nlh, int reqlen)
{
	int err;

	if (filter.ifindex) {
		err = addattr32(nlh, reqlen, NHA_OIF, filter.ifindex);
		if (err)
			return err;
	}

	if (filter.groups) {
		err = addattr_l(nlh, reqlen, NHA_GROUPS, NULL, 0);
		if (err)
			return err;
	}

	if (filter.master) {
		err = addattr32(nlh, reqlen, NHA_MASTER, filter.master);
		if (err)
			return err;
	}

	if (filter.fdb) {
		err = addattr_l(nlh, reqlen, NHA_FDB, NULL, 0);
		if (err)
			return err;
	}

	return 0;
}

static int nh_dump_bucket_filter(struct nlmsghdr *nlh, int reqlen)
{
	struct rtattr *nest;
	int err = 0;

	err = nh_dump_filter(nlh, reqlen);
	if (err)
		return err;

	if (filter.id) {
		err = addattr32(nlh, reqlen, NHA_ID, filter.id);
		if (err)
			return err;
	}

	if (filter.nhid) {
		nest = addattr_nest(nlh, reqlen, NHA_RES_BUCKET);
		nest->rta_type |= NLA_F_NESTED;

		err = addattr32(nlh, reqlen, NHA_RES_BUCKET_NH_ID,
				filter.nhid);
		if (err)
			return err;

		addattr_nest_end(nlh, nest);
	}

	return err;
}

static struct rtnl_handle rth_del = { .fd = -1 };

static int delete_nexthop(__u32 id)
{
	struct {
		struct nlmsghdr	n;
		struct nhmsg	nhm;
		char		buf[64];
	} req = {
		.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct nhmsg)),
		.n.nlmsg_flags = NLM_F_REQUEST,
		.n.nlmsg_type = RTM_DELNEXTHOP,
		.nhm.nh_family = AF_UNSPEC,
	};

	req.n.nlmsg_seq = ++rth_del.seq;

	addattr32(&req.n, sizeof(req), NHA_ID, id);

	if (rtnl_talk(&rth_del, &req.n, NULL) < 0)
		return -1;
	return 0;
}

static int flush_nexthop(struct nlmsghdr *nlh, void *arg)
{
	struct nhmsg *nhm = NLMSG_DATA(nlh);
	struct rtattr *tb[NHA_MAX+1];
	__u32 id = 0;
	int len;

	len = nlh->nlmsg_len - NLMSG_SPACE(sizeof(*nhm));
	if (len < 0) {
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -1;
	}

	if (filter.proto && nhm->nh_protocol != filter.proto)
		return 0;

	parse_rtattr(tb, NHA_MAX, RTM_NHA(nhm), len);
	if (tb[NHA_ID])
		id = rta_getattr_u32(tb[NHA_ID]);

	if (id && !delete_nexthop(id))
		filter.flushed++;

	return 0;
}

static int ipnh_flush(unsigned int all)
{
	int rc = -2;

	if (all) {
		filter.groups = 1;
		filter.ifindex = 0;
		filter.master = 0;
	}

	if (rtnl_open(&rth_del, 0) < 0) {
		fprintf(stderr, "Cannot open rtnetlink\n");
		return EXIT_FAILURE;
	}
again:
	if (rtnl_nexthopdump_req(&rth, preferred_family, nh_dump_filter) < 0) {
		perror("Cannot send dump request");
		goto out;
	}

	if (rtnl_dump_filter(&rth, flush_nexthop, stdout) < 0) {
		fprintf(stderr, "Dump terminated. Failed to flush nexthops\n");
		goto out;
	}

	/* if deleting all, then remove groups first */
	if (all && filter.groups) {
		filter.groups = 0;
		goto again;
	}

	rc = 0;
out:
	rtnl_close(&rth_del);
	if (!filter.flushed)
		printf("Nothing to flush\n");
	else
		printf("Flushed %d nexthops\n", filter.flushed);

	return rc;
}

static bool __valid_nh_group_attr(const struct rtattr *g_attr)
{
	int num = RTA_PAYLOAD(g_attr) / sizeof(struct nexthop_grp);

	return num && num * sizeof(struct nexthop_grp) == RTA_PAYLOAD(g_attr);
}

static void print_nh_group(const struct nh_entry *nhe)
{
	int i;

	open_json_array(PRINT_JSON, "group");
	print_string(PRINT_FP, NULL, "%s", "group ");
	for (i = 0; i < nhe->nh_groups_cnt; ++i) {
		open_json_object(NULL);

		if (i)
			print_string(PRINT_FP, NULL, "%s", "/");

		print_uint(PRINT_ANY, "id", "%u", nhe->nh_groups[i].id);
		if (nhe->nh_groups[i].weight)
			print_uint(PRINT_ANY, "weight", ",%u",
				   nhe->nh_groups[i].weight + 1);

		close_json_object();
	}
	print_string(PRINT_FP, NULL, "%s", " ");
	close_json_array(PRINT_JSON, NULL);
}

static const char *nh_group_type_name(__u16 type)
{
	switch (type) {
	case NEXTHOP_GRP_TYPE_MPATH:
		return "mpath";
	case NEXTHOP_GRP_TYPE_RES:
		return "resilient";
	default:
		return "<unknown type>";
	}
}

static void print_nh_group_type(__u16 nh_grp_type)
{
	if (nh_grp_type == NEXTHOP_GRP_TYPE_MPATH)
		/* Do not print type in order not to break existing output. */
		return;

	print_string(PRINT_ANY, "type", "type %s ", nh_group_type_name(nh_grp_type));
}

static void parse_nh_res_group_rta(const struct rtattr *res_grp_attr,
				   struct nha_res_grp *res_grp)
{
	struct rtattr *tb[NHA_RES_GROUP_MAX + 1];
	struct rtattr *rta;

	memset(res_grp, 0, sizeof(*res_grp));
	parse_rtattr_nested(tb, NHA_RES_GROUP_MAX, res_grp_attr);

	if (tb[NHA_RES_GROUP_BUCKETS])
		res_grp->buckets = rta_getattr_u16(tb[NHA_RES_GROUP_BUCKETS]);

	if (tb[NHA_RES_GROUP_IDLE_TIMER]) {
		rta = tb[NHA_RES_GROUP_IDLE_TIMER];
		res_grp->idle_timer = rta_getattr_u32(rta);
	}

	if (tb[NHA_RES_GROUP_UNBALANCED_TIMER]) {
		rta = tb[NHA_RES_GROUP_UNBALANCED_TIMER];
		res_grp->unbalanced_timer = rta_getattr_u32(rta);
	}

	if (tb[NHA_RES_GROUP_UNBALANCED_TIME]) {
		rta = tb[NHA_RES_GROUP_UNBALANCED_TIME];
		res_grp->unbalanced_time = rta_getattr_u64(rta);
	}
}

static void print_nh_res_group(const struct nha_res_grp *res_grp)
{
	struct timeval tv;

	open_json_object("resilient_args");

	print_uint(PRINT_ANY, "buckets", "buckets %u ", res_grp->buckets);

	 __jiffies_to_tv(&tv, res_grp->idle_timer);
	print_tv(PRINT_ANY, "idle_timer", "idle_timer %g ", &tv);

	__jiffies_to_tv(&tv, res_grp->unbalanced_timer);
	print_tv(PRINT_ANY, "unbalanced_timer", "unbalanced_timer %g ", &tv);

	__jiffies_to_tv(&tv, res_grp->unbalanced_time);
	print_tv(PRINT_ANY, "unbalanced_time", "unbalanced_time %g ", &tv);

	close_json_object();
}

static void print_nh_res_bucket(FILE *fp, const struct rtattr *res_bucket_attr)
{
	struct rtattr *tb[NHA_RES_BUCKET_MAX + 1];

	parse_rtattr_nested(tb, NHA_RES_BUCKET_MAX, res_bucket_attr);

	open_json_object("bucket");

	if (tb[NHA_RES_BUCKET_INDEX])
		print_uint(PRINT_ANY, "index", "index %u ",
			   rta_getattr_u16(tb[NHA_RES_BUCKET_INDEX]));

	if (tb[NHA_RES_BUCKET_IDLE_TIME]) {
		struct rtattr *rta = tb[NHA_RES_BUCKET_IDLE_TIME];
		struct timeval tv;

		__jiffies_to_tv(&tv, rta_getattr_u64(rta));
		print_tv(PRINT_ANY, "idle_time", "idle_time %g ", &tv);
	}

	if (tb[NHA_RES_BUCKET_NH_ID])
		print_uint(PRINT_ANY, "nhid", "nhid %u ",
			   rta_getattr_u32(tb[NHA_RES_BUCKET_NH_ID]));

	close_json_object();
}

static void ipnh_destroy_entry(struct nh_entry *nhe)
{
	if (nhe->nh_encap)
		free(nhe->nh_encap);
	if (nhe->nh_groups)
		free(nhe->nh_groups);
}

/* parse nhmsg into nexthop entry struct which must be destroyed by
 * ipnh_destroy_enty when it's not needed anymore
 */
static int ipnh_parse_nhmsg(FILE *fp, const struct nhmsg *nhm, int len,
			    struct nh_entry *nhe)
{
	struct rtattr *tb[NHA_MAX+1];
	int err = 0;

	memset(nhe, 0, sizeof(*nhe));
	parse_rtattr_flags(tb, NHA_MAX, RTM_NHA(nhm), len, NLA_F_NESTED);

	if (tb[NHA_ID])
		nhe->nh_id = rta_getattr_u32(tb[NHA_ID]);

	if (tb[NHA_OIF])
		nhe->nh_oif = rta_getattr_u32(tb[NHA_OIF]);

	if (tb[NHA_GROUP_TYPE])
		nhe->nh_grp_type = rta_getattr_u16(tb[NHA_GROUP_TYPE]);

	if (tb[NHA_GATEWAY]) {
		if (RTA_PAYLOAD(tb[NHA_GATEWAY]) > sizeof(nhe->nh_gateway)) {
			fprintf(fp, "<nexthop id %u invalid gateway length %lu>\n",
				nhe->nh_id, RTA_PAYLOAD(tb[NHA_GATEWAY]));
			err = -EINVAL;
			goto out_err;
		}
		nhe->nh_gateway_len = RTA_PAYLOAD(tb[NHA_GATEWAY]);
		memcpy(&nhe->nh_gateway, RTA_DATA(tb[NHA_GATEWAY]),
		       RTA_PAYLOAD(tb[NHA_GATEWAY]));
	}

	if (tb[NHA_ENCAP]) {
		nhe->nh_encap = malloc(RTA_LENGTH(RTA_PAYLOAD(tb[NHA_ENCAP])));
		if (!nhe->nh_encap) {
			err = -ENOMEM;
			goto out_err;
		}
		memcpy(nhe->nh_encap, tb[NHA_ENCAP],
		       RTA_LENGTH(RTA_PAYLOAD(tb[NHA_ENCAP])));
		memcpy(&nhe->nh_encap_type, tb[NHA_ENCAP_TYPE],
		       sizeof(nhe->nh_encap_type));
	}

	if (tb[NHA_GROUP]) {
		if (!__valid_nh_group_attr(tb[NHA_GROUP])) {
			fprintf(fp, "<nexthop id %u invalid nexthop group>",
				nhe->nh_id);
			err = -EINVAL;
			goto out_err;
		}

		nhe->nh_groups = malloc(RTA_PAYLOAD(tb[NHA_GROUP]));
		if (!nhe->nh_groups) {
			err = -ENOMEM;
			goto out_err;
		}
		nhe->nh_groups_cnt = RTA_PAYLOAD(tb[NHA_GROUP]) /
				     sizeof(struct nexthop_grp);
		memcpy(nhe->nh_groups, RTA_DATA(tb[NHA_GROUP]),
		       RTA_PAYLOAD(tb[NHA_GROUP]));
	}

	if (tb[NHA_RES_GROUP]) {
		parse_nh_res_group_rta(tb[NHA_RES_GROUP], &nhe->nh_res_grp);
		nhe->nh_has_res_grp = true;
	}

	nhe->nh_blackhole = !!tb[NHA_BLACKHOLE];
	nhe->nh_fdb = !!tb[NHA_FDB];

	nhe->nh_family = nhm->nh_family;
	nhe->nh_protocol = nhm->nh_protocol;
	nhe->nh_scope = nhm->nh_scope;
	nhe->nh_flags = nhm->nh_flags;

	return 0;

out_err:
	ipnh_destroy_entry(nhe);
	return err;
}

static void __print_nexthop_entry(FILE *fp, const char *jsobj,
				  struct nh_entry *nhe,
				  bool deleted)
{
	SPRINT_BUF(b1);

	open_json_object(jsobj);

	if (deleted)
		print_bool(PRINT_ANY, "deleted", "Deleted ", true);

	print_uint(PRINT_ANY, "id", "id %u ", nhe->nh_id);

	if (nhe->nh_groups)
		print_nh_group(nhe);

	print_nh_group_type(nhe->nh_grp_type);

	if (nhe->nh_has_res_grp)
		print_nh_res_group(&nhe->nh_res_grp);

	if (nhe->nh_encap)
		lwt_print_encap(fp, &nhe->nh_encap_type.rta, nhe->nh_encap);

	if (nhe->nh_gateway_len)
		__print_rta_gateway(fp, nhe->nh_family,
				    format_host(nhe->nh_family,
				    nhe->nh_gateway_len,
				    &nhe->nh_gateway));

	if (nhe->nh_oif)
		print_rta_ifidx(fp, nhe->nh_oif, "dev");

	if (nhe->nh_scope != RT_SCOPE_UNIVERSE || show_details > 0) {
		print_string(PRINT_ANY, "scope", "scope %s ",
			     rtnl_rtscope_n2a(nhe->nh_scope, b1, sizeof(b1)));
	}

	if (nhe->nh_blackhole)
		print_null(PRINT_ANY, "blackhole", "blackhole ", NULL);

	if (nhe->nh_protocol != RTPROT_UNSPEC || show_details > 0) {
		print_string(PRINT_ANY, "protocol", "proto %s ",
			     rtnl_rtprot_n2a(nhe->nh_protocol, b1, sizeof(b1)));
	}

	print_rt_flags(fp, nhe->nh_flags);

	if (nhe->nh_fdb)
		print_null(PRINT_ANY, "fdb", "fdb", NULL);

	close_json_object();
}

static int  __ipnh_get_id(struct rtnl_handle *rthp, __u32 nh_id,
			  struct nlmsghdr **answer)
{
	struct {
		struct nlmsghdr n;
		struct nhmsg	nhm;
		char		buf[1024];
	} req = {
		.n.nlmsg_len	= NLMSG_LENGTH(sizeof(struct nhmsg)),
		.n.nlmsg_flags	= NLM_F_REQUEST,
		.n.nlmsg_type	= RTM_GETNEXTHOP,
		.nhm.nh_family	= preferred_family,
	};

	addattr32(&req.n, sizeof(req), NHA_ID, nh_id);

	return rtnl_talk(rthp, &req.n, answer);
}

static struct hlist_head *ipnh_cache_head(__u32 nh_id)
{
	nh_id ^= nh_id >> 20;
	nh_id ^= nh_id >> 10;

	return &nh_cache[nh_id % NH_CACHE_SIZE];
}

static void ipnh_cache_link_entry(struct nh_entry *nhe)
{
	struct hlist_head *head = ipnh_cache_head(nhe->nh_id);

	hlist_add_head(&nhe->nh_hash, head);
}

static void ipnh_cache_unlink_entry(struct nh_entry *nhe)
{
	hlist_del(&nhe->nh_hash);
}

static struct nh_entry *ipnh_cache_get(__u32 nh_id)
{
	struct hlist_head *head = ipnh_cache_head(nh_id);
	struct nh_entry *nhe;
	struct hlist_node *n;

	hlist_for_each(n, head) {
		nhe = container_of(n, struct nh_entry, nh_hash);
		if (nhe->nh_id == nh_id)
			return nhe;
	}

	return NULL;
}

static int __ipnh_cache_parse_nlmsg(const struct nlmsghdr *n,
				    struct nh_entry *nhe)
{
	int err, len;

	len = n->nlmsg_len - NLMSG_SPACE(sizeof(struct nhmsg));
	if (len < 0) {
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -EINVAL;
	}

	err = ipnh_parse_nhmsg(stderr, NLMSG_DATA(n), len, nhe);
	if (err) {
		fprintf(stderr, "Error parsing nexthop: %s\n", strerror(-err));
		return err;
	}

	return 0;
}

static struct nh_entry *ipnh_cache_add(__u32 nh_id)
{
	struct nlmsghdr *answer = NULL;
	struct nh_entry *nhe = NULL;

	if (nh_cache_rth.fd < 0 && rtnl_open(&nh_cache_rth, 0) < 0) {
		nh_cache_rth.fd = -1;
		goto out;
	}

	if (__ipnh_get_id(&nh_cache_rth, nh_id, &answer) < 0)
		goto out;

	nhe = malloc(sizeof(*nhe));
	if (!nhe)
		goto out;

	if (__ipnh_cache_parse_nlmsg(answer, nhe))
		goto out_free_nhe;

	ipnh_cache_link_entry(nhe);

out:
	if (answer)
		free(answer);

	return nhe;

out_free_nhe:
	free(nhe);
	nhe = NULL;
	goto out;
}

static void ipnh_cache_del(struct nh_entry *nhe)
{
	ipnh_cache_unlink_entry(nhe);
	ipnh_destroy_entry(nhe);
	free(nhe);
}

/* update, add or delete a nexthop entry based on nlmsghdr */
static int ipnh_cache_process_nlmsg(const struct nlmsghdr *n,
				    struct nh_entry *new_nhe)
{
	struct nh_entry *nhe;

	nhe = ipnh_cache_get(new_nhe->nh_id);
	switch (n->nlmsg_type) {
	case RTM_DELNEXTHOP:
		if (nhe)
			ipnh_cache_del(nhe);
		ipnh_destroy_entry(new_nhe);
		break;
	case RTM_NEWNEXTHOP:
		if (!nhe) {
			nhe = malloc(sizeof(*nhe));
			if (!nhe) {
				ipnh_destroy_entry(new_nhe);
				return -1;
			}
		} else {
			/* this allows us to save 1 allocation on updates by
			 * reusing the old nh entry, but we need to cleanup its
			 * internal storage
			 */
			ipnh_cache_unlink_entry(nhe);
			ipnh_destroy_entry(nhe);
		}
		memcpy(nhe, new_nhe, sizeof(*nhe));
		ipnh_cache_link_entry(nhe);
		break;
	}

	return 0;
}

void print_cache_nexthop_id(FILE *fp, const char *fp_prefix, const char *jsobj,
			    __u32 nh_id)
{
	struct nh_entry *nhe = ipnh_cache_get(nh_id);

	if (!nhe) {
		nhe = ipnh_cache_add(nh_id);
		if (!nhe)
			return;
	}

	if (fp_prefix)
		print_string(PRINT_FP, NULL, "%s", fp_prefix);
	__print_nexthop_entry(fp, jsobj, nhe, false);
}

int print_cache_nexthop(struct nlmsghdr *n, void *arg, bool process_cache)
{
	struct nhmsg *nhm = NLMSG_DATA(n);
	FILE *fp = (FILE *)arg;
	struct nh_entry nhe;
	int len, err;

	if (n->nlmsg_type != RTM_DELNEXTHOP &&
	    n->nlmsg_type != RTM_NEWNEXTHOP) {
		fprintf(stderr, "Not a nexthop: %08x %08x %08x\n",
			n->nlmsg_len, n->nlmsg_type, n->nlmsg_flags);
		return -1;
	}

	len = n->nlmsg_len - NLMSG_SPACE(sizeof(*nhm));
	if (len < 0) {
		close_json_object();
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -1;
	}

	if (filter.proto && filter.proto != nhm->nh_protocol)
		return 0;

	err = ipnh_parse_nhmsg(fp, nhm, len, &nhe);
	if (err) {
		close_json_object();
		fprintf(stderr, "Error parsing nexthop: %s\n", strerror(-err));
		return -1;
	}
	__print_nexthop_entry(fp, NULL, &nhe, n->nlmsg_type == RTM_DELNEXTHOP);
	print_string(PRINT_FP, NULL, "%s", "\n");
	fflush(fp);

	if (process_cache)
		ipnh_cache_process_nlmsg(n, &nhe);
	else
		ipnh_destroy_entry(&nhe);

	return 0;
}

static int print_nexthop_nocache(struct nlmsghdr *n, void *arg)
{
	return print_cache_nexthop(n, arg, false);
}

int print_nexthop_bucket(struct nlmsghdr *n, void *arg)
{
	struct nhmsg *nhm = NLMSG_DATA(n);
	struct rtattr *tb[NHA_MAX+1];
	FILE *fp = (FILE *)arg;
	int len;

	if (n->nlmsg_type != RTM_DELNEXTHOPBUCKET &&
	    n->nlmsg_type != RTM_NEWNEXTHOPBUCKET) {
		fprintf(stderr, "Not a nexthop bucket: %08x %08x %08x\n",
			n->nlmsg_len, n->nlmsg_type, n->nlmsg_flags);
		return -1;
	}

	len = n->nlmsg_len - NLMSG_SPACE(sizeof(*nhm));
	if (len < 0) {
		close_json_object();
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -1;
	}

	parse_rtattr_flags(tb, NHA_MAX, RTM_NHA(nhm), len, NLA_F_NESTED);

	open_json_object(NULL);

	if (n->nlmsg_type == RTM_DELNEXTHOP)
		print_bool(PRINT_ANY, "deleted", "Deleted ", true);

	if (tb[NHA_ID])
		print_uint(PRINT_ANY, "id", "id %u ",
			   rta_getattr_u32(tb[NHA_ID]));

	if (tb[NHA_RES_BUCKET])
		print_nh_res_bucket(fp, tb[NHA_RES_BUCKET]);

	print_rt_flags(fp, nhm->nh_flags);

	print_string(PRINT_FP, NULL, "%s", "\n");
	close_json_object();
	fflush(fp);

	return 0;
}

static int add_nh_group_attr(struct nlmsghdr *n, int maxlen, char *argv)
{
	struct nexthop_grp *grps = NULL;
	int count = 0, i;
	int err = -1;
	char *sep, *wsep;

	if (*argv != '\0')
		count = 1;

	/* separator is '/' */
	sep = strchr(argv, '/');
	while (sep) {
		count++;
		sep = strchr(sep + 1, '/');
	}

	if (count == 0)
		goto out;

	grps = calloc(count, sizeof(*grps));
	if (!grps)
		goto out;

	for (i = 0; i < count; ++i) {
		sep = strchr(argv, '/');
		if (sep)
			*sep = '\0';

		wsep = strchr(argv, ',');
		if (wsep)
			*wsep = '\0';

		if (get_unsigned(&grps[i].id, argv, 0))
			goto out;
		if (wsep) {
			unsigned int w;

			wsep++;
			if (get_unsigned(&w, wsep, 0) || w == 0 || w > 256)
				invarg("\"weight\" is invalid\n", wsep);
			grps[i].weight = w - 1;
		}

		if (!sep)
			break;

		argv = sep + 1;
	}

	err = addattr_l(n, maxlen, NHA_GROUP, grps, count * sizeof(*grps));
out:
	free(grps);
	return err;
}

static int read_nh_group_type(const char *name)
{
	if (strcmp(name, "mpath") == 0)
		return NEXTHOP_GRP_TYPE_MPATH;
	else if (strcmp(name, "resilient") == 0)
		return NEXTHOP_GRP_TYPE_RES;

	return __NEXTHOP_GRP_TYPE_MAX;
}

static void parse_nh_group_type_res(struct nlmsghdr *n, int maxlen, int *argcp,
				    char ***argvp)
{
	char **argv = *argvp;
	struct rtattr *nest;
	int argc = *argcp;

	if (!NEXT_ARG_OK())
		return;

	nest = addattr_nest(n, maxlen, NHA_RES_GROUP);
	nest->rta_type |= NLA_F_NESTED;

	NEXT_ARG_FWD();
	while (argc > 0) {
		if (strcmp(*argv, "buckets") == 0) {
			__u16 buckets;

			NEXT_ARG();
			if (get_u16(&buckets, *argv, 0))
				invarg("invalid buckets value", *argv);

			addattr16(n, maxlen, NHA_RES_GROUP_BUCKETS, buckets);
		} else if (strcmp(*argv, "idle_timer") == 0) {
			__u32 idle_timer;

			NEXT_ARG();
			if (get_unsigned(&idle_timer, *argv, 0) ||
			    idle_timer >= UINT32_MAX / 100)
				invarg("invalid idle timer value", *argv);

			addattr32(n, maxlen, NHA_RES_GROUP_IDLE_TIMER,
				  idle_timer * 100);
		} else if (strcmp(*argv, "unbalanced_timer") == 0) {
			__u32 unbalanced_timer;

			NEXT_ARG();
			if (get_unsigned(&unbalanced_timer, *argv, 0) ||
			    unbalanced_timer >= UINT32_MAX / 100)
				invarg("invalid unbalanced timer value", *argv);

			addattr32(n, maxlen, NHA_RES_GROUP_UNBALANCED_TIMER,
				  unbalanced_timer * 100);
		} else {
			break;
		}
		argc--; argv++;
	}

	/* argv is currently the first unparsed argument, but ipnh_modify()
	 * will move to the next, so step back.
	 */
	*argcp = argc + 1;
	*argvp = argv - 1;

	addattr_nest_end(n, nest);
}

static void parse_nh_group_type(struct nlmsghdr *n, int maxlen, int *argcp,
				char ***argvp)
{
	char **argv = *argvp;
	int argc = *argcp;
	__u16 type;

	NEXT_ARG();
	type = read_nh_group_type(*argv);
	if (type > NEXTHOP_GRP_TYPE_MAX)
		invarg("\"type\" value is invalid\n", *argv);

	switch (type) {
	case NEXTHOP_GRP_TYPE_MPATH:
		/* No additional arguments */
		break;
	case NEXTHOP_GRP_TYPE_RES:
		parse_nh_group_type_res(n, maxlen, &argc, &argv);
		break;
	}

	*argcp = argc;
	*argvp = argv;

	addattr16(n, maxlen, NHA_GROUP_TYPE, type);
}

static int ipnh_parse_id(const char *argv)
{
	__u32 id;

	if (get_unsigned(&id, argv, 0))
		invarg("invalid id value", argv);
	return id;
}

static int ipnh_modify(int cmd, unsigned int flags, int argc, char **argv)
{
	struct {
		struct nlmsghdr	n;
		struct nhmsg	nhm;
		char		buf[1024];
	} req = {
		.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct nhmsg)),
		.n.nlmsg_flags = NLM_F_REQUEST | flags,
		.n.nlmsg_type = cmd,
		.nhm.nh_family = preferred_family,
	};
	__u32 nh_flags = 0;

	while (argc > 0) {
		if (!strcmp(*argv, "id")) {
			NEXT_ARG();
			addattr32(&req.n, sizeof(req), NHA_ID,
				  ipnh_parse_id(*argv));
		} else if (!strcmp(*argv, "dev")) {
			int ifindex;

			NEXT_ARG();
			ifindex = ll_name_to_index(*argv);
			if (!ifindex)
				invarg("Device does not exist\n", *argv);
			addattr32(&req.n, sizeof(req), NHA_OIF, ifindex);
			if (req.nhm.nh_family == AF_UNSPEC)
				req.nhm.nh_family = AF_INET;
		} else if (strcmp(*argv, "via") == 0) {
			inet_prefix addr;
			int family;

			NEXT_ARG();
			family = read_family(*argv);
			if (family == AF_UNSPEC)
				family = req.nhm.nh_family;
			else
				NEXT_ARG();
			get_addr(&addr, *argv, family);
			if (req.nhm.nh_family == AF_UNSPEC)
				req.nhm.nh_family = addr.family;
			else if (req.nhm.nh_family != addr.family)
				invarg("address family mismatch\n", *argv);
			addattr_l(&req.n, sizeof(req), NHA_GATEWAY,
				  &addr.data, addr.bytelen);
		} else if (strcmp(*argv, "encap") == 0) {
			char buf[1024];
			struct rtattr *rta = (void *)buf;

			rta->rta_type = NHA_ENCAP;
			rta->rta_len = RTA_LENGTH(0);

			lwt_parse_encap(rta, sizeof(buf), &argc, &argv,
					NHA_ENCAP, NHA_ENCAP_TYPE);

			if (rta->rta_len > RTA_LENGTH(0)) {
				addraw_l(&req.n, 1024, RTA_DATA(rta),
					 RTA_PAYLOAD(rta));
			}
		} else if (!strcmp(*argv, "blackhole")) {
			addattr_l(&req.n, sizeof(req), NHA_BLACKHOLE, NULL, 0);
			if (req.nhm.nh_family == AF_UNSPEC)
				req.nhm.nh_family = AF_INET;
		} else if (!strcmp(*argv, "fdb")) {
			addattr_l(&req.n, sizeof(req), NHA_FDB, NULL, 0);
		} else if (!strcmp(*argv, "onlink")) {
			nh_flags |= RTNH_F_ONLINK;
		} else if (!strcmp(*argv, "group")) {
			NEXT_ARG();

			if (add_nh_group_attr(&req.n, sizeof(req), *argv))
				invarg("\"group\" value is invalid\n", *argv);
		} else if (!strcmp(*argv, "type")) {
			parse_nh_group_type(&req.n, sizeof(req), &argc, &argv);
		} else if (matches(*argv, "protocol") == 0) {
			__u32 prot;

			NEXT_ARG();
			if (rtnl_rtprot_a2n(&prot, *argv))
				invarg("\"protocol\" value is invalid\n", *argv);
			req.nhm.nh_protocol = prot;
		} else if (strcmp(*argv, "help") == 0) {
			usage();
		} else {
			invarg("", *argv);
		}
		argc--; argv++;
	}

	req.nhm.nh_flags = nh_flags;

	if (rtnl_talk(&rth, &req.n, NULL) < 0)
		return -2;

	return 0;
}

static int ipnh_get_id(__u32 id)
{
	struct nlmsghdr *answer;

	if (__ipnh_get_id(&rth, id, &answer) < 0)
		return -2;

	new_json_obj(json);

	if (print_nexthop_nocache(answer, (void *)stdout) < 0) {
		free(answer);
		return -1;
	}

	delete_json_obj();
	fflush(stdout);

	free(answer);

	return 0;
}

static int ipnh_list_flush_id(__u32 id, int action)
{
	int err;

	if (action == IPNH_LIST)
		return ipnh_get_id(id);

	if (rtnl_open(&rth_del, 0) < 0) {
		fprintf(stderr, "Cannot open rtnetlink\n");
		return EXIT_FAILURE;
	}

	err = delete_nexthop(id);
	rtnl_close(&rth_del);

	return err;
}

static int ipnh_list_flush(int argc, char **argv, int action)
{
	unsigned int all = (argc == 0);

	while (argc > 0) {
		if (!matches(*argv, "dev")) {
			NEXT_ARG();
			filter.ifindex = ll_name_to_index(*argv);
			if (!filter.ifindex)
				invarg("Device does not exist\n", *argv);
		} else if (!matches(*argv, "groups")) {
			filter.groups = 1;
		} else if (!matches(*argv, "master")) {
			NEXT_ARG();
			filter.master = ll_name_to_index(*argv);
			if (!filter.master)
				invarg("Device does not exist\n", *argv);
		} else if (matches(*argv, "vrf") == 0) {
			NEXT_ARG();
			if (!name_is_vrf(*argv))
				invarg("Invalid VRF\n", *argv);
			filter.master = ll_name_to_index(*argv);
			if (!filter.master)
				invarg("VRF does not exist\n", *argv);
		} else if (!strcmp(*argv, "id")) {
			NEXT_ARG();
			return ipnh_list_flush_id(ipnh_parse_id(*argv), action);
		} else if (!matches(*argv, "protocol")) {
			__u32 proto;

			NEXT_ARG();
			if (get_unsigned(&proto, *argv, 0))
				invarg("invalid protocol value", *argv);
			filter.proto = proto;
		} else if (!matches(*argv, "fdb")) {
			filter.fdb = 1;
		} else if (matches(*argv, "help") == 0) {
			usage();
		} else {
			invarg("", *argv);
		}
		argc--; argv++;
	}

	if (action == IPNH_FLUSH)
		return ipnh_flush(all);

	if (rtnl_nexthopdump_req(&rth, preferred_family, nh_dump_filter) < 0) {
		perror("Cannot send dump request");
		return -2;
	}

	new_json_obj(json);

	if (rtnl_dump_filter(&rth, print_nexthop_nocache, stdout) < 0) {
		fprintf(stderr, "Dump terminated\n");
		return -2;
	}

	delete_json_obj();
	fflush(stdout);

	return 0;
}

static int ipnh_get(int argc, char **argv)
{
	__u32 id = 0;

	while (argc > 0) {
		if (!strcmp(*argv, "id")) {
			NEXT_ARG();
			id = ipnh_parse_id(*argv);
		} else  {
			usage();
		}
		argc--; argv++;
	}

	if (!id) {
		usage();
		return -1;
	}

	return ipnh_get_id(id);
}

static int ipnh_bucket_list(int argc, char **argv)
{
	while (argc > 0) {
		if (!matches(*argv, "dev")) {
			NEXT_ARG();
			filter.ifindex = ll_name_to_index(*argv);
			if (!filter.ifindex)
				invarg("Device does not exist\n", *argv);
		} else if (!matches(*argv, "master")) {
			NEXT_ARG();
			filter.master = ll_name_to_index(*argv);
			if (!filter.master)
				invarg("Device does not exist\n", *argv);
		} else if (matches(*argv, "vrf") == 0) {
			NEXT_ARG();
			if (!name_is_vrf(*argv))
				invarg("Invalid VRF\n", *argv);
			filter.master = ll_name_to_index(*argv);
			if (!filter.master)
				invarg("VRF does not exist\n", *argv);
		} else if (!strcmp(*argv, "id")) {
			NEXT_ARG();
			filter.id = ipnh_parse_id(*argv);
		} else if (!strcmp(*argv, "nhid")) {
			NEXT_ARG();
			filter.nhid = ipnh_parse_id(*argv);
		} else if (matches(*argv, "help") == 0) {
			usage();
		} else {
			invarg("", *argv);
		}
		argc--; argv++;
	}

	if (rtnl_nexthop_bucket_dump_req(&rth, preferred_family,
					 nh_dump_bucket_filter) < 0) {
		perror("Cannot send dump request");
		return -2;
	}

	new_json_obj(json);

	if (rtnl_dump_filter(&rth, print_nexthop_bucket, stdout) < 0) {
		fprintf(stderr, "Dump terminated\n");
		return -2;
	}

	delete_json_obj();
	fflush(stdout);

	return 0;
}

static int ipnh_bucket_get_id(__u32 id, __u16 bucket_index)
{
	struct {
		struct nlmsghdr	n;
		struct nhmsg	nhm;
		char		buf[1024];
	} req = {
		.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct nhmsg)),
		.n.nlmsg_flags = NLM_F_REQUEST,
		.n.nlmsg_type  = RTM_GETNEXTHOPBUCKET,
		.nhm.nh_family = preferred_family,
	};
	struct nlmsghdr *answer;
	struct rtattr *nest;

	addattr32(&req.n, sizeof(req), NHA_ID, id);

	nest = addattr_nest(&req.n, sizeof(req), NHA_RES_BUCKET);
	nest->rta_type |= NLA_F_NESTED;

	addattr16(&req.n, sizeof(req), NHA_RES_BUCKET_INDEX, bucket_index);

	addattr_nest_end(&req.n, nest);

	if (rtnl_talk(&rth, &req.n, &answer) < 0)
		return -2;

	new_json_obj(json);

	if (print_nexthop_bucket(answer, (void *)stdout) < 0) {
		free(answer);
		return -1;
	}

	delete_json_obj();
	fflush(stdout);

	free(answer);

	return 0;
}

static int ipnh_bucket_get(int argc, char **argv)
{
	bool bucket_valid = false;
	__u16 bucket_index;
	__u32 id = 0;

	while (argc > 0) {
		if (!strcmp(*argv, "id")) {
			NEXT_ARG();
			id = ipnh_parse_id(*argv);
		} else if (!strcmp(*argv, "index")) {
			NEXT_ARG();
			if (get_u16(&bucket_index, *argv, 0))
				invarg("invalid bucket index value", *argv);
			bucket_valid = true;
		} else  {
			usage();
		}
		argc--; argv++;
	}

	if (!id || !bucket_valid) {
		usage();
		return -1;
	}

	return ipnh_bucket_get_id(id, bucket_index);
}

static int do_ipnh_bucket(int argc, char **argv)
{
	if (argc < 1)
		return ipnh_bucket_list(0, NULL);

	if (!matches(*argv, "list") ||
	    !matches(*argv, "show") ||
	    !matches(*argv, "lst"))
		return ipnh_bucket_list(argc-1, argv+1);

	if (!matches(*argv, "get"))
		return ipnh_bucket_get(argc-1, argv+1);

	if (!matches(*argv, "help"))
		usage();

	fprintf(stderr,
		"Command \"%s\" is unknown, try \"ip nexthop help\".\n", *argv);
	exit(-1);
}

int do_ipnh(int argc, char **argv)
{
	if (argc < 1)
		return ipnh_list_flush(0, NULL, IPNH_LIST);

	if (!matches(*argv, "add"))
		return ipnh_modify(RTM_NEWNEXTHOP, NLM_F_CREATE|NLM_F_EXCL,
				   argc-1, argv+1);
	if (!matches(*argv, "replace"))
		return ipnh_modify(RTM_NEWNEXTHOP, NLM_F_CREATE|NLM_F_REPLACE,
				   argc-1, argv+1);
	if (!matches(*argv, "delete"))
		return ipnh_modify(RTM_DELNEXTHOP, 0, argc-1, argv+1);

	if (!matches(*argv, "list") ||
	    !matches(*argv, "show") ||
	    !matches(*argv, "lst"))
		return ipnh_list_flush(argc-1, argv+1, IPNH_LIST);

	if (!matches(*argv, "get"))
		return ipnh_get(argc-1, argv+1);

	if (!matches(*argv, "flush"))
		return ipnh_list_flush(argc-1, argv+1, IPNH_FLUSH);

	if (!matches(*argv, "bucket"))
		return do_ipnh_bucket(argc-1, argv+1);

	if (!matches(*argv, "help"))
		usage();

	fprintf(stderr,
		"Command \"%s\" is unknown, try \"ip nexthop help\".\n", *argv);
	exit(-1);
}
