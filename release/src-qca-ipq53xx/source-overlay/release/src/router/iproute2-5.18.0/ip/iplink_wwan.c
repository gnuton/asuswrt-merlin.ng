/* SPDX-License-Identifier: GPL-2.0 */

#include <stdio.h>
#include <linux/netlink.h>
#include <linux/wwan.h>

#include "utils.h"
#include "ip_common.h"

static void print_explain(FILE *f)
{
	fprintf(f,
		"Usage: ... wwan linkid LINKID\n"
		"\n"
		"Where: LINKID := 0-4294967295\n"
	);
}

static void explain(void)
{
	print_explain(stderr);
}

static int wwan_parse_opt(struct link_util *lu, int argc, char **argv,
			  struct nlmsghdr *n)
{
	while (argc > 0) {
		if (matches(*argv, "linkid") == 0) {
			__u32 linkid;

			NEXT_ARG();
			if (get_u32(&linkid, *argv, 0))
				invarg("linkid", *argv);
			addattr32(n, 1024, IFLA_WWAN_LINK_ID, linkid);
		} else if (matches(*argv, "help") == 0) {
			explain();
			return -1;
		} else {
			fprintf(stderr, "wwan: unknown command \"%s\"?\n",
				*argv);
			explain();
			return -1;
		}
		argc--, argv++;
	}

	return 0;
}

static void wwan_print_opt(struct link_util *lu, FILE *f, struct rtattr *tb[])
{
	if (!tb)
		return;

	if (tb[IFLA_WWAN_LINK_ID])
		print_uint(PRINT_ANY, "linkid", "linkid %u ",
			   rta_getattr_u32(tb[IFLA_WWAN_LINK_ID]));
}

static void wwan_print_help(struct link_util *lu, int argc, char **argv,
			    FILE *f)
{
	print_explain(f);
}

struct link_util wwan_link_util = {
	.id		= "wwan",
	.maxattr	= IFLA_WWAN_MAX,
	.parse_opt	= wwan_parse_opt,
	.print_opt	= wwan_print_opt,
	.print_help	= wwan_print_help,
};
