/*
 *   mnlg.c	Generic Netlink helpers for libmnl
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License
 *              as published by the Free Software Foundation; either version
 *              2 of the License, or (at your option) any later version.
 *
 * Authors:     Jiri Pirko <jiri@mellanox.com>
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <libmnl/libmnl.h>
#include <linux/genetlink.h>

#include "libnetlink.h"
#include "mnl_utils.h"
#include "utils.h"
#include "mnlg.h"

struct mnlg_socket {
	struct mnl_socket *nl;
	char *buf;
	uint32_t id;
	uint8_t version;
	unsigned int seq;
};

int mnlg_socket_send(struct mnlu_gen_socket *nlg, const struct nlmsghdr *nlh)
{
	return mnl_socket_sendto(nlg->nl, nlh, nlh->nlmsg_len);
}

struct group_info {
	bool found;
	uint32_t id;
	const char *name;
};

static int parse_mc_grps_cb(const struct nlattr *attr, void *data)
{
	const struct nlattr **tb = data;
	int type = mnl_attr_get_type(attr);

	if (mnl_attr_type_valid(attr, CTRL_ATTR_MCAST_GRP_MAX) < 0)
		return MNL_CB_OK;

	switch (type) {
	case CTRL_ATTR_MCAST_GRP_ID:
		if (mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
			return MNL_CB_ERROR;
		break;
	case CTRL_ATTR_MCAST_GRP_NAME:
		if (mnl_attr_validate(attr, MNL_TYPE_STRING) < 0)
			return MNL_CB_ERROR;
		break;
	}
	tb[type] = attr;
	return MNL_CB_OK;
}

static void parse_genl_mc_grps(struct nlattr *nested,
			       struct group_info *group_info)
{
	struct nlattr *pos;
	const char *name;

	mnl_attr_for_each_nested(pos, nested) {
		struct nlattr *tb[CTRL_ATTR_MCAST_GRP_MAX + 1] = {};

		mnl_attr_parse_nested(pos, parse_mc_grps_cb, tb);
		if (!tb[CTRL_ATTR_MCAST_GRP_NAME] ||
		    !tb[CTRL_ATTR_MCAST_GRP_ID])
			continue;

		name = mnl_attr_get_str(tb[CTRL_ATTR_MCAST_GRP_NAME]);
		if (strcmp(name, group_info->name) != 0)
			continue;

		group_info->id = mnl_attr_get_u32(tb[CTRL_ATTR_MCAST_GRP_ID]);
		group_info->found = true;
	}
}

static int get_group_id_attr_cb(const struct nlattr *attr, void *data)
{
	const struct nlattr **tb = data;
	int type = mnl_attr_get_type(attr);

	if (mnl_attr_type_valid(attr, CTRL_ATTR_MAX) < 0)
		return MNL_CB_ERROR;

	if (type == CTRL_ATTR_MCAST_GROUPS &&
	    mnl_attr_validate(attr, MNL_TYPE_NESTED) < 0)
		return MNL_CB_ERROR;
	tb[type] = attr;
	return MNL_CB_OK;
}

static int get_group_id_cb(const struct nlmsghdr *nlh, void *data)
{
	struct group_info *group_info = data;
	struct nlattr *tb[CTRL_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

	mnl_attr_parse(nlh, sizeof(*genl), get_group_id_attr_cb, tb);
	if (!tb[CTRL_ATTR_MCAST_GROUPS])
		return MNL_CB_ERROR;
	parse_genl_mc_grps(tb[CTRL_ATTR_MCAST_GROUPS], group_info);
	return MNL_CB_OK;
}

int mnlg_socket_group_add(struct mnlu_gen_socket *nlg, const char *group_name)
{
	struct nlmsghdr *nlh;
	struct group_info group_info;
	int err;

	nlh = _mnlu_gen_socket_cmd_prepare(nlg, CTRL_CMD_GETFAMILY,
					   NLM_F_REQUEST | NLM_F_ACK,
					   GENL_ID_CTRL, 1);

	mnl_attr_put_u16(nlh, CTRL_ATTR_FAMILY_ID, nlg->family);

	err = mnlg_socket_send(nlg, nlh);
	if (err < 0)
		return err;

	group_info.found = false;
	group_info.name = group_name;
	err = mnlu_gen_socket_recv_run(nlg, get_group_id_cb, &group_info);
	if (err < 0)
		return err;

	if (!group_info.found) {
		errno = ENOENT;
		return -1;
	}

	err = mnl_socket_setsockopt(nlg->nl, NETLINK_ADD_MEMBERSHIP,
				    &group_info.id, sizeof(group_info.id));
	if (err < 0)
		return err;

	return 0;
}

int mnlg_socket_get_fd(struct mnlu_gen_socket *nlg)
{
	return mnl_socket_get_fd(nlg->nl);
}
