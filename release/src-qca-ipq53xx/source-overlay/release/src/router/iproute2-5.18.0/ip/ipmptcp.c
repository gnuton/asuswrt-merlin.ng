// SPDX-License-Identifier: GPL-2.0

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/genetlink.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/mptcp.h>

#include "utils.h"
#include "ip_common.h"
#include "json_print.h"
#include "libgenl.h"
#include "libnetlink.h"
#include "ll_map.h"

static void usage(void)
{
	fprintf(stderr,
		"Usage:	ip mptcp endpoint add ADDRESS [ dev NAME ] [ id ID ]\n"
		"				      [ port NR ] [ FLAG-LIST ]\n"
		"	ip mptcp endpoint delete id ID [ ADDRESS ]\n"
		"	ip mptcp endpoint change [ id ID ] [ ADDRESS ] [ port NR ] CHANGE-OPT\n"
		"	ip mptcp endpoint show [ id ID ]\n"
		"	ip mptcp endpoint flush\n"
		"	ip mptcp limits set [ subflows NR ] [ add_addr_accepted NR ]\n"
		"	ip mptcp limits show\n"
		"	ip mptcp monitor\n"
		"FLAG-LIST := [ FLAG-LIST ] FLAG\n"
		"FLAG  := [ signal | subflow | backup | fullmesh ]\n"
		"CHANGE-OPT := [ backup | nobackup | fullmesh | nofullmesh ]\n");

	exit(-1);
}

/* netlink socket */
static struct rtnl_handle genl_rth = { .fd = -1 };
static int genl_family = -1;

#define MPTCP_BUFLEN	4096
#define MPTCP_REQUEST(_req,  _cmd, _flags)	\
	GENL_REQUEST(_req, MPTCP_BUFLEN, genl_family, 0,	\
		     MPTCP_PM_VER, _cmd, _flags)

#define MPTCP_PM_ADDR_FLAG_NONE 0x0

/* Mapping from argument to address flag mask */
static const struct {
	const char *name;
	unsigned long value;
} mptcp_addr_flag_names[] = {
	{ "signal",		MPTCP_PM_ADDR_FLAG_SIGNAL },
	{ "subflow",		MPTCP_PM_ADDR_FLAG_SUBFLOW },
	{ "backup",		MPTCP_PM_ADDR_FLAG_BACKUP },
	{ "fullmesh",		MPTCP_PM_ADDR_FLAG_FULLMESH },
	{ "nobackup",		MPTCP_PM_ADDR_FLAG_NONE },
	{ "nofullmesh",		MPTCP_PM_ADDR_FLAG_NONE }
};

static void print_mptcp_addr_flags(unsigned int flags)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(mptcp_addr_flag_names); i++) {
		unsigned long mask = mptcp_addr_flag_names[i].value;

		if (flags & mask) {
			print_string(PRINT_FP, NULL, "%s ",
				     mptcp_addr_flag_names[i].name);
			print_bool(PRINT_JSON,
				   mptcp_addr_flag_names[i].name, NULL, true);
		}

		flags &= ~mask;
	}

	if (flags) {
		/* unknown flags */
		SPRINT_BUF(b1);

		snprintf(b1, sizeof(b1), "%02x", flags);
		print_string(PRINT_ANY, "rawflags", "rawflags %s ", b1);
	}
}

static int get_flags(const char *arg, __u32 *flags)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(mptcp_addr_flag_names); i++) {
		if (strcmp(arg, mptcp_addr_flag_names[i].name))
			continue;

		*flags |= mptcp_addr_flag_names[i].value;
		return 0;
	}
	return -1;
}

static int mptcp_parse_opt(int argc, char **argv, struct nlmsghdr *n, int cmd)
{
	bool setting = cmd == MPTCP_PM_CMD_SET_FLAGS;
	bool adding = cmd == MPTCP_PM_CMD_ADD_ADDR;
	bool deling = cmd == MPTCP_PM_CMD_DEL_ADDR;
	struct rtattr *attr_addr;
	bool addr_set = false;
	inet_prefix address;
	bool id_set = false;
	__u32 index = 0;
	__u32 flags = 0;
	__u16 port = 0;
	__u8 id = 0;

	ll_init_map(&rth);
	while (argc > 0) {
		if (get_flags(*argv, &flags) == 0) {
			if (adding &&
			    (flags & MPTCP_PM_ADDR_FLAG_SIGNAL) &&
			    (flags & MPTCP_PM_ADDR_FLAG_FULLMESH))
				invarg("flags mustn't have both signal and fullmesh", *argv);

			/* allow changing the 'backup' and 'fullmesh' flags only */
			if (setting &&
			    (flags & ~(MPTCP_PM_ADDR_FLAG_BACKUP |
				       MPTCP_PM_ADDR_FLAG_FULLMESH)))
				invarg("invalid flags, backup and fullmesh only", *argv);

		} else if (matches(*argv, "id") == 0) {
			NEXT_ARG();

			if (get_u8(&id, *argv, 0))
				invarg("invalid ID\n", *argv);
			id_set = true;
		} else if (matches(*argv, "dev") == 0) {
			const char *ifname;

			NEXT_ARG();

			ifname = *argv;

			if (check_ifname(ifname))
				invarg("invalid interface name\n", ifname);

			index = ll_name_to_index(ifname);

			if (!index)
				invarg("device does not exist\n", ifname);

		} else if (matches(*argv, "port") == 0) {
			NEXT_ARG();
			if (get_u16(&port, *argv, 0))
				invarg("expected port", *argv);
		} else if (get_addr(&address, *argv, AF_UNSPEC) == 0) {
			addr_set = true;
		} else {
			invarg("unknown argument", *argv);
		}
		NEXT_ARG_FWD();
	}

	if (!addr_set && adding)
		missarg("ADDRESS");

	if (!id_set && deling) {
		missarg("ID");
	} else if (id_set && deling) {
		if (id && addr_set)
			invarg("invalid for non-zero id address\n", "ADDRESS");
		else if (!id && !addr_set)
			invarg("address is needed for deleting id 0 address\n", "ID");
	}

	if (adding && port && !(flags & MPTCP_PM_ADDR_FLAG_SIGNAL))
		invarg("flags must have signal when using port", "port");

	if (setting && id_set && port)
		invarg("port can't be used with id", "port");

	attr_addr = addattr_nest(n, MPTCP_BUFLEN,
				 MPTCP_PM_ATTR_ADDR | NLA_F_NESTED);
	if (id_set)
		addattr8(n, MPTCP_BUFLEN, MPTCP_PM_ADDR_ATTR_ID, id);
	if (flags)
		addattr32(n, MPTCP_BUFLEN, MPTCP_PM_ADDR_ATTR_FLAGS, flags);
	if (index)
		addattr32(n, MPTCP_BUFLEN, MPTCP_PM_ADDR_ATTR_IF_IDX, index);
	if (port)
		addattr16(n, MPTCP_BUFLEN, MPTCP_PM_ADDR_ATTR_PORT, port);
	if (addr_set) {
		int type;

		addattr16(n, MPTCP_BUFLEN, MPTCP_PM_ADDR_ATTR_FAMILY,
			  address.family);
		type = address.family == AF_INET ? MPTCP_PM_ADDR_ATTR_ADDR4 :
						   MPTCP_PM_ADDR_ATTR_ADDR6;
		addattr_l(n, MPTCP_BUFLEN, type, &address.data,
			  address.bytelen);
	}

	addattr_nest_end(n, attr_addr);
	return 0;
}

static int mptcp_addr_modify(int argc, char **argv, int cmd)
{
	MPTCP_REQUEST(req, cmd, NLM_F_REQUEST);
	int ret;

	ret = mptcp_parse_opt(argc, argv, &req.n, cmd);
	if (ret)
		return ret;

	if (rtnl_talk(&genl_rth, &req.n, NULL) < 0)
		return -2;

	return 0;
}

static int print_mptcp_addrinfo(struct rtattr *addrinfo)
{
	struct rtattr *tb[MPTCP_PM_ADDR_ATTR_MAX + 1];
	__u8 family = AF_UNSPEC, addr_attr_type;
	const char *ifname;
	unsigned int flags;
	__u16 id, port;
	int index;

	parse_rtattr_nested(tb, MPTCP_PM_ADDR_ATTR_MAX, addrinfo);

	open_json_object(NULL);
	if (tb[MPTCP_PM_ADDR_ATTR_FAMILY])
		family = rta_getattr_u8(tb[MPTCP_PM_ADDR_ATTR_FAMILY]);

	addr_attr_type = family == AF_INET ? MPTCP_PM_ADDR_ATTR_ADDR4 :
					     MPTCP_PM_ADDR_ATTR_ADDR6;
	if (tb[addr_attr_type]) {
		print_string(PRINT_ANY, "address", "%s ",
			     format_host_rta(family, tb[addr_attr_type]));
	}
	if (tb[MPTCP_PM_ADDR_ATTR_PORT]) {
		port = rta_getattr_u16(tb[MPTCP_PM_ADDR_ATTR_PORT]);
		if (port)
			print_uint(PRINT_ANY, "port", "port %u ", port);
	}
	if (tb[MPTCP_PM_ADDR_ATTR_ID]) {
		id = rta_getattr_u8(tb[MPTCP_PM_ADDR_ATTR_ID]);
		print_uint(PRINT_ANY, "id", "id %u ", id);
	}
	if (tb[MPTCP_PM_ADDR_ATTR_FLAGS]) {
		flags = rta_getattr_u32(tb[MPTCP_PM_ADDR_ATTR_FLAGS]);
		print_mptcp_addr_flags(flags);
	}
	if (tb[MPTCP_PM_ADDR_ATTR_IF_IDX]) {
		index = rta_getattr_s32(tb[MPTCP_PM_ADDR_ATTR_IF_IDX]);
		ifname = index ? ll_index_to_name(index) : NULL;

		if (ifname)
			print_string(PRINT_ANY, "dev", "dev %s ", ifname);
	}

	close_json_object();
	print_string(PRINT_FP, NULL, "\n", NULL);
	fflush(stdout);

	return 0;
}

static int print_mptcp_addr(struct nlmsghdr *n, void *arg)
{
	struct rtattr *tb[MPTCP_PM_ATTR_MAX + 1];
	struct genlmsghdr *ghdr;
	struct rtattr *addrinfo;
	int len = n->nlmsg_len;

	if (n->nlmsg_type != genl_family)
		return 0;

	len -= NLMSG_LENGTH(GENL_HDRLEN);
	if (len < 0)
		return -1;

	ghdr = NLMSG_DATA(n);
	parse_rtattr_flags(tb, MPTCP_PM_ATTR_MAX, (void *) ghdr + GENL_HDRLEN,
			   len, NLA_F_NESTED);
	addrinfo = tb[MPTCP_PM_ATTR_ADDR];
	if (!addrinfo)
		return -1;

	ll_init_map(&rth);
	return print_mptcp_addrinfo(addrinfo);
}

static int mptcp_addr_dump(void)
{
	MPTCP_REQUEST(req, MPTCP_PM_CMD_GET_ADDR, NLM_F_REQUEST | NLM_F_DUMP);

	if (rtnl_send(&genl_rth, &req.n, req.n.nlmsg_len) < 0) {
		perror("Cannot send show request");
		exit(1);
	}

	new_json_obj(json);

	if (rtnl_dump_filter(&genl_rth, print_mptcp_addr, stdout) < 0) {
		fprintf(stderr, "Dump terminated\n");
		delete_json_obj();
		fflush(stdout);
		return -2;
	}

	delete_json_obj();
	fflush(stdout);
	return 0;
}

static int mptcp_addr_show(int argc, char **argv)
{
	MPTCP_REQUEST(req, MPTCP_PM_CMD_GET_ADDR, NLM_F_REQUEST);
	struct nlmsghdr *answer;
	int ret;

	if (argc <= 0)
		return mptcp_addr_dump();

	ret = mptcp_parse_opt(argc, argv, &req.n, MPTCP_PM_CMD_GET_ADDR);
	if (ret)
		return ret;

	if (rtnl_talk(&genl_rth, &req.n, &answer) < 0)
		return -2;

	new_json_obj(json);
	ret = print_mptcp_addr(answer, stdout);
	delete_json_obj();
	fflush(stdout);
	return ret;
}

static int mptcp_addr_flush(int argc, char **argv)
{
	MPTCP_REQUEST(req, MPTCP_PM_CMD_FLUSH_ADDRS, NLM_F_REQUEST);

	if (rtnl_talk(&genl_rth, &req.n, NULL) < 0)
		return -2;

	return 0;
}

static int mptcp_parse_limit(int argc, char **argv, struct nlmsghdr *n)
{
	bool set_rcv_add_addrs = false;
	bool set_subflows = false;
	__u32 rcv_add_addrs = 0;
	__u32 subflows = 0;

	while (argc > 0) {
		if (matches(*argv, "subflows") == 0) {
			NEXT_ARG();

			if (get_u32(&subflows, *argv, 0))
				invarg("invalid subflows\n", *argv);
			set_subflows = true;
		} else if (matches(*argv, "add_addr_accepted") == 0) {
			NEXT_ARG();

			if (get_u32(&rcv_add_addrs, *argv, 0))
				invarg("invalid add_addr_accepted\n", *argv);
			set_rcv_add_addrs = true;
		} else {
			invarg("unknown limit", *argv);
		}
		NEXT_ARG_FWD();
	}

	if (set_rcv_add_addrs)
		addattr32(n, MPTCP_BUFLEN, MPTCP_PM_ATTR_RCV_ADD_ADDRS,
			  rcv_add_addrs);
	if (set_subflows)
		addattr32(n, MPTCP_BUFLEN, MPTCP_PM_ATTR_SUBFLOWS, subflows);
	return set_rcv_add_addrs || set_subflows;
}

static int print_mptcp_limit(struct nlmsghdr *n, void *arg)
{
	struct rtattr *tb[MPTCP_PM_ATTR_MAX + 1];
	struct genlmsghdr *ghdr;
	int len = n->nlmsg_len;
	__u32 val;

	if (n->nlmsg_type != genl_family)
		return 0;

	len -= NLMSG_LENGTH(GENL_HDRLEN);
	if (len < 0)
		return -1;

	ghdr = NLMSG_DATA(n);
	parse_rtattr(tb, MPTCP_PM_ATTR_MAX, (void *) ghdr + GENL_HDRLEN, len);

	open_json_object(NULL);
	if (tb[MPTCP_PM_ATTR_RCV_ADD_ADDRS]) {
		val = rta_getattr_u32(tb[MPTCP_PM_ATTR_RCV_ADD_ADDRS]);

		print_uint(PRINT_ANY, "add_addr_accepted",
			   "add_addr_accepted %d ", val);
	}

	if (tb[MPTCP_PM_ATTR_SUBFLOWS]) {
		val = rta_getattr_u32(tb[MPTCP_PM_ATTR_SUBFLOWS]);

		print_uint(PRINT_ANY, "subflows", "subflows %d ", val);
	}
	print_string(PRINT_FP, NULL, "%s", "\n");
	fflush(stdout);
	close_json_object();
	return 0;
}

static int mptcp_limit_get_set(int argc, char **argv, int cmd)
{
	bool do_get = cmd == MPTCP_PM_CMD_GET_LIMITS;
	MPTCP_REQUEST(req, cmd, NLM_F_REQUEST);
	struct nlmsghdr *answer;
	int ret;

	ret = mptcp_parse_limit(argc, argv, &req.n);
	if (ret < 0)
		return -1;

	if (rtnl_talk(&genl_rth, &req.n, do_get ? &answer : NULL) < 0)
		return -2;

	if (do_get)
		return print_mptcp_limit(answer, stdout);
	return 0;
}

static const char * const event_to_str[] = {
	[MPTCP_EVENT_CREATED] = "CREATED",
	[MPTCP_EVENT_ESTABLISHED] = "ESTABLISHED",
	[MPTCP_EVENT_CLOSED] = "CLOSED",
	[MPTCP_EVENT_ANNOUNCED] = "ANNOUNCED",
	[MPTCP_EVENT_REMOVED] = "REMOVED",
	[MPTCP_EVENT_SUB_ESTABLISHED] = "SF_ESTABLISHED",
	[MPTCP_EVENT_SUB_CLOSED] = "SF_CLOSED",
	[MPTCP_EVENT_SUB_PRIORITY] = "SF_PRIO",
};

static void print_addr(const char *key, int af, struct rtattr *value)
{
	void *data = RTA_DATA(value);
	char str[INET6_ADDRSTRLEN];

	if (inet_ntop(af, data, str, sizeof(str)))
		printf(" %s=%s", key, str);
}

static int mptcp_monitor_msg(struct rtnl_ctrl_data *ctrl,
			     struct nlmsghdr *n, void *arg)
{
	const struct genlmsghdr *ghdr = NLMSG_DATA(n);
	struct rtattr *tb[MPTCP_ATTR_MAX + 1];
	int len = n->nlmsg_len;

	len -= NLMSG_LENGTH(GENL_HDRLEN);
	if (len < 0)
		return -1;

	if (n->nlmsg_type != genl_family)
		return 0;

	if (timestamp)
		print_timestamp(stdout);

	if (ghdr->cmd >= ARRAY_SIZE(event_to_str)) {
		printf("[UNKNOWN %u]\n", ghdr->cmd);
		goto out;
	}

	if (event_to_str[ghdr->cmd] == NULL) {
		printf("[UNKNOWN %u]\n", ghdr->cmd);
		goto out;
	}

	printf("[%14s]", event_to_str[ghdr->cmd]);

	parse_rtattr(tb, MPTCP_ATTR_MAX, (void *) ghdr + GENL_HDRLEN, len);

	printf(" token=%08x", rta_getattr_u32(tb[MPTCP_ATTR_TOKEN]));

	if (tb[MPTCP_ATTR_REM_ID])
		printf(" remid=%u", rta_getattr_u8(tb[MPTCP_ATTR_REM_ID]));
	if (tb[MPTCP_ATTR_LOC_ID])
		printf(" locid=%u", rta_getattr_u8(tb[MPTCP_ATTR_LOC_ID]));

	if (tb[MPTCP_ATTR_SADDR4])
		print_addr("saddr4", AF_INET, tb[MPTCP_ATTR_SADDR4]);
	if (tb[MPTCP_ATTR_DADDR4])
		print_addr("daddr4", AF_INET, tb[MPTCP_ATTR_DADDR4]);
	if (tb[MPTCP_ATTR_SADDR6])
		print_addr("saddr6", AF_INET6, tb[MPTCP_ATTR_SADDR6]);
	if (tb[MPTCP_ATTR_DADDR6])
		print_addr("daddr6", AF_INET6, tb[MPTCP_ATTR_DADDR6]);
	if (tb[MPTCP_ATTR_SPORT])
		printf(" sport=%u", rta_getattr_be16(tb[MPTCP_ATTR_SPORT]));
	if (tb[MPTCP_ATTR_DPORT])
		printf(" dport=%u", rta_getattr_be16(tb[MPTCP_ATTR_DPORT]));
	if (tb[MPTCP_ATTR_BACKUP])
		printf(" backup=%d", rta_getattr_u8(tb[MPTCP_ATTR_BACKUP]));
	if (tb[MPTCP_ATTR_ERROR])
		printf(" error=%d", rta_getattr_u8(tb[MPTCP_ATTR_ERROR]));
	if (tb[MPTCP_ATTR_FLAGS])
		printf(" flags=%x", rta_getattr_u16(tb[MPTCP_ATTR_FLAGS]));
	if (tb[MPTCP_ATTR_TIMEOUT])
		printf(" timeout=%u", rta_getattr_u32(tb[MPTCP_ATTR_TIMEOUT]));
	if (tb[MPTCP_ATTR_IF_IDX])
		printf(" ifindex=%d", rta_getattr_s32(tb[MPTCP_ATTR_IF_IDX]));
	if (tb[MPTCP_ATTR_RESET_REASON])
		printf(" reset_reason=%u", rta_getattr_u32(tb[MPTCP_ATTR_RESET_REASON]));
	if (tb[MPTCP_ATTR_RESET_FLAGS])
		printf(" reset_flags=0x%x", rta_getattr_u32(tb[MPTCP_ATTR_RESET_FLAGS]));

	puts("");
out:
	fflush(stdout);
	return 0;
}

static int mptcp_monitor(void)
{
	if (genl_add_mcast_grp(&genl_rth, genl_family, MPTCP_PM_EV_GRP_NAME) < 0) {
		perror("can't subscribe to mptcp events");
		return 1;
	}

	if (rtnl_listen(&genl_rth, mptcp_monitor_msg, stdout) < 0)
		return 2;

	return 0;
}

int do_mptcp(int argc, char **argv)
{
	if (argc == 0)
		usage();

	if (matches(*argv, "help") == 0)
		usage();

	if (genl_init_handle(&genl_rth, MPTCP_PM_NAME, &genl_family))
		exit(1);

	if (matches(*argv, "endpoint") == 0) {
		NEXT_ARG_FWD();
		if (argc == 0)
			return mptcp_addr_show(0, NULL);

		if (matches(*argv, "add") == 0)
			return mptcp_addr_modify(argc-1, argv+1,
						 MPTCP_PM_CMD_ADD_ADDR);
		if (matches(*argv, "change") == 0)
			return mptcp_addr_modify(argc-1, argv+1,
						 MPTCP_PM_CMD_SET_FLAGS);
		if (matches(*argv, "delete") == 0)
			return mptcp_addr_modify(argc-1, argv+1,
						 MPTCP_PM_CMD_DEL_ADDR);
		if (matches(*argv, "show") == 0)
			return mptcp_addr_show(argc-1, argv+1);
		if (matches(*argv, "flush") == 0)
			return mptcp_addr_flush(argc-1, argv+1);

		goto unknown;
	}

	if (matches(*argv, "limits") == 0) {
		NEXT_ARG_FWD();
		if (argc == 0)
			return mptcp_limit_get_set(0, NULL,
						   MPTCP_PM_CMD_GET_LIMITS);

		if (matches(*argv, "set") == 0)
			return mptcp_limit_get_set(argc-1, argv+1,
						   MPTCP_PM_CMD_SET_LIMITS);
		if (matches(*argv, "show") == 0)
			return mptcp_limit_get_set(argc-1, argv+1,
						   MPTCP_PM_CMD_GET_LIMITS);
	}

	if (matches(*argv, "monitor") == 0) {
		NEXT_ARG_FWD();
		if (argc == 0)
			return mptcp_monitor();

		goto unknown;
	}

unknown:
	fprintf(stderr, "Command \"%s\" is unknown, try \"ip mptcp help\".\n",
		*argv);
	exit(-1);
}
