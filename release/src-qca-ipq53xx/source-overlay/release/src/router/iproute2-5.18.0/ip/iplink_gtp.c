/* SPDX-License-Identifier: GPL-2.0 */

#include <stdio.h>

#include "rt_names.h"
#include "utils.h"
#include "ip_common.h"

#define GTP_ATTRSET(attrs, type) (((attrs) & (1L << (type))) != 0)

static void print_explain(FILE *f)
{
	fprintf(f,
		"Usage: ... gtp role ROLE\n"
		"		[ hsize HSIZE ]\n"
		"		[ restart_count RESTART_COUNT ]\n"
		"\n"
		"Where:	ROLE		:= { sgsn | ggsn }\n"
		"	HSIZE		:= 1-131071\n"
		"	RESTART_COUNT	:= 0-255\n"
	);
}

static void check_duparg(__u32 *attrs, int type, const char *key,
			 const char *argv)
{
	if (!GTP_ATTRSET(*attrs, type)) {
		*attrs |= (1L << type);
		return;
	}
	duparg2(key, argv);
}

static int gtp_parse_opt(struct link_util *lu, int argc, char **argv,
			 struct nlmsghdr *n)
{
	__u32 attrs = 0;

	/* When creating GTP device through ip link,
	 * this flag has to be set.
	 */
	addattr8(n, 1024, IFLA_GTP_CREATE_SOCKETS, true);

	while (argc > 0) {
		if (!strcmp(*argv, "role")) {
			NEXT_ARG();
			check_duparg(&attrs, IFLA_GTP_ROLE, "role", *argv);
			if (!strcmp(*argv, "sgsn"))
				addattr32(n, 1024, IFLA_GTP_ROLE, GTP_ROLE_SGSN);
			else if (!strcmp(*argv, "ggsn"))
				addattr32(n, 1024, IFLA_GTP_ROLE, GTP_ROLE_GGSN);
			else
				invarg("invalid role, use sgsn or ggsn", *argv);
		} else if (!strcmp(*argv, "hsize")) {
			__u32 hsize;

			NEXT_ARG();
			check_duparg(&attrs, IFLA_GTP_PDP_HASHSIZE, "hsize", *argv);

			if (get_u32(&hsize, *argv, 0))
				invarg("invalid PDP hash size", *argv);
			if (hsize >= 1u << 17)
				invarg("PDP hash size too big", *argv);
			addattr32(n, 1024, IFLA_GTP_PDP_HASHSIZE, hsize);
		} else if (!strcmp(*argv, "restart_count")) {
			__u8 restart_count;

			NEXT_ARG();
			check_duparg(&attrs, IFLA_GTP_RESTART_COUNT, "restart_count", *argv);

			if (get_u8(&restart_count, *argv, 10))
				invarg("invalid restart_count", *argv);
			addattr8(n, 1024, IFLA_GTP_RESTART_COUNT, restart_count);
		} else if (!strcmp(*argv, "help")) {
			print_explain(stderr);
			return -1;
		}
		argc--, argv++;
	}

	if (!GTP_ATTRSET(attrs, IFLA_GTP_ROLE)) {
		fprintf(stderr, "gtp: role of the gtp device was not specified\n");
		return -1;
	}

	if (!GTP_ATTRSET(attrs, IFLA_GTP_PDP_HASHSIZE))
		addattr32(n, 1024, IFLA_GTP_PDP_HASHSIZE, 1024);

	return 0;
}

static const char *gtp_role_to_string(__u32 role)
{
	switch (role) {
	case GTP_ROLE_GGSN:
		return "ggsn";
	case GTP_ROLE_SGSN:
		return "sgsn";
	default:
		return "unknown";
	}
}

static void gtp_print_opt(struct link_util *lu, FILE *f, struct rtattr *tb[])
{

	if (tb[IFLA_GTP_ROLE]) {
		__u32 role = rta_getattr_u32(tb[IFLA_GTP_ROLE]);

		print_string(PRINT_ANY, "role", "role %s ",
			     gtp_role_to_string(role));
	}

	if (tb[IFLA_GTP_PDP_HASHSIZE]) {
		__u32 hsize = rta_getattr_u32(tb[IFLA_GTP_PDP_HASHSIZE]);

		print_uint(PRINT_ANY, "hsize", "hsize %u ", hsize);
	}

	if (tb[IFLA_GTP_RESTART_COUNT]) {
		__u8 restart_count = rta_getattr_u8(tb[IFLA_GTP_RESTART_COUNT]);

		print_uint(PRINT_ANY, "restart_count",
			   "restart_count %u ", restart_count);
	}
}

static void gtp_print_help(struct link_util *lu, int argc, char **argv,
			   FILE *f)
{
	print_explain(f);
}

struct link_util gtp_link_util = {
	.id		= "gtp",
	.maxattr	= IFLA_GTP_MAX,
	.parse_opt	= gtp_parse_opt,
	.print_opt	= gtp_print_opt,
	.print_help	= gtp_print_help,
};
