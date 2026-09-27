// SPDX-License-Identifier: GPL-2.0
/*
 * ioam6.c "ip ioam"
 *
 * Author: Justin Iurman <justin.iurman@uliege.be>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>

#include <linux/genetlink.h>
#include <linux/ioam6_genl.h>

#include "utils.h"
#include "ip_common.h"
#include "libgenl.h"
#include "json_print.h"

static void usage(void)
{
	fprintf(stderr,
		"Usage:	ip ioam { COMMAND | help }\n"
		"	ip ioam namespace show\n"
		"	ip ioam namespace add ID [ data DATA32 ] [ wide DATA64 ]\n"
		"	ip ioam namespace del ID\n"
		"	ip ioam schema show\n"
		"	ip ioam schema add ID DATA\n"
		"	ip ioam schema del ID\n"
		"	ip ioam namespace set ID schema { ID | none }\n");
	exit(-1);
}

static struct rtnl_handle grth = { .fd = -1 };
static int genl_family = -1;

#define IOAM6_REQUEST(_req, _bufsiz, _cmd, _flags) \
	 GENL_REQUEST(_req, _bufsiz, genl_family, 0, \
				IOAM6_GENL_VERSION, _cmd, _flags)

static struct {
	unsigned int cmd;
	__u32 sc_id;
	__u32 ns_data;
	__u64 ns_data_wide;
	__u16 ns_id;
	bool has_ns_data;
	bool has_ns_data_wide;
	bool sc_none;
	__u8 sc_data[IOAM6_MAX_SCHEMA_DATA_LEN];
} opts;

static void print_namespace(struct rtattr *attrs[])
{
	print_uint(PRINT_ANY, "namespace", "namespace %u",
		   rta_getattr_u16(attrs[IOAM6_ATTR_NS_ID]));

	if (attrs[IOAM6_ATTR_SC_ID])
		print_uint(PRINT_ANY, "schema", " [schema %u]",
			   rta_getattr_u32(attrs[IOAM6_ATTR_SC_ID]));

	if (attrs[IOAM6_ATTR_NS_DATA])
		print_hex(PRINT_ANY, "data", ", data %#010x",
			  rta_getattr_u32(attrs[IOAM6_ATTR_NS_DATA]));

	if (attrs[IOAM6_ATTR_NS_DATA_WIDE])
		print_0xhex(PRINT_ANY, "wide", ", wide %#018lx",
			    rta_getattr_u64(attrs[IOAM6_ATTR_NS_DATA_WIDE]));

	print_nl();
}

static void print_schema(struct rtattr *attrs[])
{
	__u8 data[IOAM6_MAX_SCHEMA_DATA_LEN];
	int len, i = 0;

	print_uint(PRINT_ANY, "schema", "schema %u",
		   rta_getattr_u32(attrs[IOAM6_ATTR_SC_ID]));

	if (attrs[IOAM6_ATTR_NS_ID])
		print_uint(PRINT_ANY, "namespace", " [namespace %u]",
			   rta_getattr_u16(attrs[IOAM6_ATTR_NS_ID]));

	len = RTA_PAYLOAD(attrs[IOAM6_ATTR_SC_DATA]);
	memcpy(data, RTA_DATA(attrs[IOAM6_ATTR_SC_DATA]), len);

	print_null(PRINT_ANY, "data", ", data:", NULL);
	while (i < len) {
		print_hhu(PRINT_ANY, "", " %02x", data[i]);
		i++;
	}
	print_nl();
}

static int process_msg(struct nlmsghdr *n, void *arg)
{
	struct rtattr *attrs[IOAM6_ATTR_MAX + 1];
	struct genlmsghdr *ghdr;
	int len = n->nlmsg_len;

	if (n->nlmsg_type != genl_family)
		return -1;

	len -= NLMSG_LENGTH(GENL_HDRLEN);
	if (len < 0)
		return -1;

	ghdr = NLMSG_DATA(n);
	parse_rtattr(attrs, IOAM6_ATTR_MAX, (void *)ghdr + GENL_HDRLEN, len);

	open_json_object(NULL);
	switch (ghdr->cmd) {
	case IOAM6_CMD_DUMP_NAMESPACES:
		print_namespace(attrs);
		break;
	case IOAM6_CMD_DUMP_SCHEMAS:
		print_schema(attrs);
		break;
	}
	close_json_object();

	return 0;
}

static int ioam6_do_cmd(void)
{
	IOAM6_REQUEST(req, 1056, opts.cmd, NLM_F_REQUEST);
	int dump = 0;

	if (genl_init_handle(&grth, IOAM6_GENL_NAME, &genl_family))
		exit(1);

	req.n.nlmsg_type = genl_family;

	switch (opts.cmd) {
	case IOAM6_CMD_ADD_NAMESPACE:
		addattr16(&req.n, sizeof(req), IOAM6_ATTR_NS_ID, opts.ns_id);
		if (opts.has_ns_data)
			addattr32(&req.n, sizeof(req), IOAM6_ATTR_NS_DATA,
				  opts.ns_data);
		if (opts.has_ns_data_wide)
			addattr64(&req.n, sizeof(req), IOAM6_ATTR_NS_DATA_WIDE,
				  opts.ns_data_wide);
		break;
	case IOAM6_CMD_DEL_NAMESPACE:
		addattr16(&req.n, sizeof(req), IOAM6_ATTR_NS_ID, opts.ns_id);
		break;
	case IOAM6_CMD_DUMP_NAMESPACES:
	case IOAM6_CMD_DUMP_SCHEMAS:
		dump = 1;
		break;
	case IOAM6_CMD_ADD_SCHEMA:
		addattr32(&req.n, sizeof(req), IOAM6_ATTR_SC_ID, opts.sc_id);
		addattr_l(&req.n, sizeof(req), IOAM6_ATTR_SC_DATA, opts.sc_data,
			  strlen((const char *)opts.sc_data));
		break;
	case IOAM6_CMD_DEL_SCHEMA:
		addattr32(&req.n, sizeof(req), IOAM6_ATTR_SC_ID, opts.sc_id);
		break;
	case IOAM6_CMD_NS_SET_SCHEMA:
		addattr16(&req.n, sizeof(req), IOAM6_ATTR_NS_ID, opts.ns_id);
		if (opts.sc_none)
			addattr(&req.n, sizeof(req), IOAM6_ATTR_SC_NONE);
		else
			addattr32(&req.n, sizeof(req), IOAM6_ATTR_SC_ID,
				  opts.sc_id);
		break;
	}

	if (!dump) {
		if (rtnl_talk(&grth, &req.n, NULL) < 0)
			return -1;
	} else {
		req.n.nlmsg_flags |= NLM_F_DUMP;
		req.n.nlmsg_seq = grth.dump = ++grth.seq;
		if (rtnl_send(&grth, &req, req.n.nlmsg_len) < 0) {
			perror("Failed to send dump request");
			exit(1);
		}

		new_json_obj(json);
		if (rtnl_dump_filter(&grth, process_msg, stdout) < 0) {
			fprintf(stderr, "Dump terminated\n");
			exit(1);
		}
		delete_json_obj();
		fflush(stdout);
	}

	return 0;
}

int do_ioam6(int argc, char **argv)
{
	bool maybe_wide = false;

	if (argc < 1 || strcmp(*argv, "help") == 0)
		usage();

	memset(&opts, 0, sizeof(opts));

	if (strcmp(*argv, "namespace") == 0) {
		NEXT_ARG();

		if (strcmp(*argv, "show") == 0) {
			opts.cmd = IOAM6_CMD_DUMP_NAMESPACES;

		} else if (strcmp(*argv, "add") == 0) {
			NEXT_ARG();

			if (get_u16(&opts.ns_id, *argv, 0))
				invarg("Invalid namespace ID", *argv);

			if (NEXT_ARG_OK()) {
				NEXT_ARG_FWD();

				if (strcmp(*argv, "data") == 0) {
					NEXT_ARG();

					if (get_u32(&opts.ns_data, *argv, 0))
						invarg("Invalid data", *argv);

					maybe_wide = true;
					opts.has_ns_data = true;

				} else if (strcmp(*argv, "wide") == 0) {
					NEXT_ARG();

					if (get_u64(&opts.ns_data_wide, *argv, 16))
						invarg("Invalid wide data", *argv);

					opts.has_ns_data_wide = true;

				} else {
					invarg("Invalid argument", *argv);
				}
			}

			if (NEXT_ARG_OK()) {
				NEXT_ARG_FWD();

				if (!maybe_wide || strcmp(*argv, "wide") != 0)
					invarg("Unexpected argument", *argv);

				NEXT_ARG();

				if (get_u64(&opts.ns_data_wide, *argv, 16))
					invarg("Invalid wide data", *argv);

				opts.has_ns_data_wide = true;
			}

			opts.cmd = IOAM6_CMD_ADD_NAMESPACE;

		} else if (strcmp(*argv, "del") == 0) {
			NEXT_ARG();

			if (get_u16(&opts.ns_id, *argv, 0))
				invarg("Invalid namespace ID", *argv);

			opts.cmd = IOAM6_CMD_DEL_NAMESPACE;

		} else if (strcmp(*argv, "set") == 0) {
			NEXT_ARG();

			if (get_u16(&opts.ns_id, *argv, 0))
				invarg("Invalid namespace ID", *argv);

			NEXT_ARG();

			if (strcmp(*argv, "schema") != 0)
				invarg("Unknown", *argv);

			NEXT_ARG();

			if (strcmp(*argv, "none") == 0) {
				opts.sc_none = true;

			} else {
				if (get_u32(&opts.sc_id, *argv, 0))
					invarg("Invalid schema ID", *argv);

				opts.sc_none = false;
			}

			opts.cmd = IOAM6_CMD_NS_SET_SCHEMA;

		} else {
			invarg("Unknown", *argv);
		}

	} else if (strcmp(*argv, "schema") == 0) {
		NEXT_ARG();

		if (strcmp(*argv, "show") == 0) {
			opts.cmd = IOAM6_CMD_DUMP_SCHEMAS;

		} else if (strcmp(*argv, "add") == 0) {
			NEXT_ARG();

			if (get_u32(&opts.sc_id, *argv, 0))
				invarg("Invalid schema ID", *argv);

			NEXT_ARG();

			if (strlen(*argv) > IOAM6_MAX_SCHEMA_DATA_LEN)
				invarg("Schema DATA too big", *argv);

			memcpy(opts.sc_data, *argv, strlen(*argv));
			opts.cmd = IOAM6_CMD_ADD_SCHEMA;

		} else if (strcmp(*argv, "del") == 0) {
			NEXT_ARG();

			if (get_u32(&opts.sc_id, *argv, 0))
				invarg("Invalid schema ID", *argv);

			opts.cmd = IOAM6_CMD_DEL_SCHEMA;

		} else {
			invarg("Unknown", *argv);
		}

	} else {
		invarg("Unknown", *argv);
	}

	return ioam6_do_cmd();
}
