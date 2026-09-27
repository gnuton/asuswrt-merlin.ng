/*
 * iplink_batadv.c	Batman-adv support
 *
 * Authors:     Nicolas Escande <nico.escande@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/batman_adv.h>

#include "rt_names.h"
#include "utils.h"
#include "ip_common.h"

static void print_explain(FILE *f)
{
	fprintf(f,
		"Usage: ... batadv [ ra ROUTING_ALG ]\n"
		"\n"
		"Where: ROUTING_ALG := { BATMAN_IV | BATMAN_V }\n"
	);
}

static void explain(void)
{
	print_explain(stderr);
}

static int batadv_parse_opt(struct link_util *lu, int argc, char **argv,
			    struct nlmsghdr *n)
{
	while (argc > 0) {
		if (matches(*argv, "ra") == 0) {
			NEXT_ARG();
			addattrstrz(n, 1024, IFLA_BATADV_ALGO_NAME, *argv);
		} else if (matches(*argv, "help") == 0) {
			explain();
			return -1;
		} else {
			fprintf(stderr,
				"batadv: unknown command \"%s\"?\n",
				*argv);
			explain();
			return -1;
		}
		argc--, argv++;
	}

	return 0;
}

static void batadv_print_help(struct link_util *lu, int argc, char **argv,
			      FILE *f)
{
	print_explain(f);
}

struct link_util batadv_link_util = {
	.id		= "batadv",
	.maxattr	= IFLA_BATADV_MAX,
	.parse_opt	= batadv_parse_opt,
	.print_help	= batadv_print_help,
};
