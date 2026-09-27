/*
 * iplink_amt.c	AMT device support
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Taehee Yoo <ap420073@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__GLIBC__) && !defined(__UCLIBC__) /* musl */
#else
#include <net/if.h>
#endif
#include <linux/ip.h>
#include <linux/if_link.h>
#include <arpa/inet.h>
#include <linux/amt.h>

#include "rt_names.h"
#include "utils.h"
#include "ip_common.h"

#define AMT_ATTRSET(attrs, type) (((attrs) & (1L << (type))) != 0)

static void print_usage(FILE *f)
{
	fprintf(f,
		"Usage: ... amt\n"
		"               [ discovery IP_ADDRESS ]\n"
		"               [ mode MODE ]\n"
		"               [ local ADDR ]\n"
		"               [ dev PHYS_DEV ]\n"
		"               [ relay_port PORT ]\n"
		"               [ gateway_port PORT ]\n"
		"               [ max_tunnels NUMBER ]\n"
		"\n"
		"Where: ADDR	:= { IP_ADDRESS }\n"
		"       MODE	:= { gateway | relay }\n"
		);
}

static char *modename[] = {"gateway", "relay"};

static void usage(void)
{
	print_usage(stderr);
}

static void check_duparg(__u64 *attrs, int type, const char *key,
		const char *argv)
{
	if (!AMT_ATTRSET(*attrs, type)) {
		*attrs |= (1L << type);
		return;
	}
	duparg2(key, argv);
}

static int amt_parse_opt(struct link_util *lu, int argc, char **argv,
			 struct nlmsghdr *n)
{
	unsigned int mode, max_tunnels;
	inet_prefix saddr, daddr;
	__u64 attrs = 0;
	__u16 port;

	saddr.family = daddr.family = AF_UNSPEC;

	inet_prefix_reset(&saddr);
	inet_prefix_reset(&daddr);

	while (argc > 0) {
		if (strcmp(*argv, "mode") == 0) {
			NEXT_ARG();
			if (strcmp(*argv, "gateway") == 0) {
				mode = 0;
			} else if (strcmp(*argv, "relay") == 0) {
				mode = 1;
			} else {
				usage();
				return -1;
			}
			addattr32(n, 1024, IFLA_AMT_MODE, mode);
		} else if (strcmp(*argv, "relay_port") == 0) {
			NEXT_ARG();
			if (get_u16(&port, *argv, 0))
				invarg("relay_port", *argv);
			addattr16(n, 1024, IFLA_AMT_RELAY_PORT, htons(port));
		} else if (strcmp(*argv, "gateway_port") == 0) {
			NEXT_ARG();
			if (get_u16(&port, *argv, 0))
				invarg("gateway_port", *argv);
			addattr16(n, 1024, IFLA_AMT_GATEWAY_PORT, htons(port));
		} else if (strcmp(*argv, "max_tunnels") == 0) {
			NEXT_ARG();
			if (get_u32(&max_tunnels, *argv, 0))
				invarg("max_tunnels", *argv);
			addattr32(n, 1024, IFLA_AMT_MAX_TUNNELS, max_tunnels);
		} else if (strcmp(*argv, "dev") == 0) {
			unsigned int link;

			NEXT_ARG();
			link = ll_name_to_index(*argv);
			if (!link)
				exit(nodev(*argv));
			addattr32(n, 1024, IFLA_AMT_LINK, link);
		} else if (strcmp(*argv, "local") == 0) {
			NEXT_ARG();
			check_duparg(&attrs, IFLA_AMT_LOCAL_IP, "local", *argv);
			get_addr(&saddr, *argv, daddr.family);

			if (is_addrtype_inet(&saddr))
				addattr_l(n, 1024, IFLA_AMT_LOCAL_IP,
					  saddr.data, saddr.bytelen);
		} else if (strcmp(*argv, "discovery") == 0) {
			NEXT_ARG();
			check_duparg(&attrs, IFLA_AMT_DISCOVERY_IP,
				     "discovery", *argv);
			get_addr(&daddr, *argv, daddr.family);
			if (is_addrtype_inet(&daddr))
				addattr_l(n, 1024, IFLA_AMT_DISCOVERY_IP,
					  daddr.data, daddr.bytelen);
		} else if (strcmp(*argv, "help") == 0) {
			usage();
			return -1;
		} else {
			fprintf(stderr, "amt: unknown command \"%s\"?\n", *argv);
			usage();
			return -1;
		}
		argc--, argv++;
	}

	return 0;
}

static void amt_print_opt(struct link_util *lu, FILE *f, struct rtattr *tb[])
{
	if (!tb)
		return;

	if (tb[IFLA_AMT_MODE])
		print_string(PRINT_ANY, "mode", "%s ",
			     modename[rta_getattr_u32(tb[IFLA_AMT_MODE])]);

	if (tb[IFLA_AMT_GATEWAY_PORT])
		print_uint(PRINT_ANY, "gateway_port", "gateway_port %u ",
			   rta_getattr_be16(tb[IFLA_AMT_GATEWAY_PORT]));

	if (tb[IFLA_AMT_RELAY_PORT])
		print_uint(PRINT_ANY, "relay_port", "relay_port %u ",
			   rta_getattr_be16(tb[IFLA_AMT_RELAY_PORT]));

	if (tb[IFLA_AMT_LOCAL_IP]) {
		__be32 addr = rta_getattr_u32(tb[IFLA_AMT_LOCAL_IP]);

		print_string(PRINT_ANY, "local", "local %s ",
			     format_host(AF_INET, 4, &addr));
	}

	if (tb[IFLA_AMT_REMOTE_IP]) {
		__be32 addr = rta_getattr_u32(tb[IFLA_AMT_REMOTE_IP]);

		print_string(PRINT_ANY, "remote", "remote %s ",
			     format_host(AF_INET, 4, &addr));
	}

	if (tb[IFLA_AMT_DISCOVERY_IP]) {
		__be32 addr = rta_getattr_u32(tb[IFLA_AMT_DISCOVERY_IP]);

		print_string(PRINT_ANY, "discovery", "discovery %s ",
			     format_host(AF_INET, 4, &addr));
	}

	if (tb[IFLA_AMT_LINK]) {
		unsigned int link = rta_getattr_u32(tb[IFLA_AMT_LINK]);

		print_string(PRINT_ANY, "link", "dev %s ",
			     ll_index_to_name(link));
	}

	if (tb[IFLA_AMT_MAX_TUNNELS])
		print_uint(PRINT_ANY, "max_tunnels", "max_tunnels %u ",
			   rta_getattr_u32(tb[IFLA_AMT_MAX_TUNNELS]));
}

static void amt_print_help(struct link_util *lu, int argc, char **argv, FILE *f)
{
	print_usage(f);
}

struct link_util amt_link_util = {
	.id		= "amt",
	.maxattr	= IFLA_AMT_MAX,
	.parse_opt	= amt_parse_opt,
	.print_opt	= amt_print_opt,
	.print_help	= amt_print_help,
};
