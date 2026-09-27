/* SPDX-License-Identifier: GPL-2.0 */
/*
 * libgenl.c	GENL library
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/genetlink.h>
#include "libgenl.h"

static int genl_parse_getfamily(struct nlmsghdr *nlh)
{
	struct rtattr *tb[CTRL_ATTR_MAX + 1];
	struct genlmsghdr *ghdr = NLMSG_DATA(nlh);
	int len = nlh->nlmsg_len;
	struct rtattr *attrs;

	if (nlh->nlmsg_type != GENL_ID_CTRL) {
		fprintf(stderr, "Not a controller message, nlmsg_len=%d "
			"nlmsg_type=0x%x\n", nlh->nlmsg_len, nlh->nlmsg_type);
		return -1;
	}

	len -= NLMSG_LENGTH(GENL_HDRLEN);

	if (len < 0) {
		fprintf(stderr, "wrong controller message len %d\n", len);
		return -1;
	}

	if (ghdr->cmd != CTRL_CMD_NEWFAMILY) {
		fprintf(stderr, "Unknown controller command %d\n", ghdr->cmd);
		return -1;
	}

	attrs = (struct rtattr *) ((char *) ghdr + GENL_HDRLEN);
	parse_rtattr(tb, CTRL_ATTR_MAX, attrs, len);

	if (tb[CTRL_ATTR_FAMILY_ID] == NULL) {
		fprintf(stderr, "Missing family id TLV\n");
		return -1;
	}

	return rta_getattr_u16(tb[CTRL_ATTR_FAMILY_ID]);
}

int genl_resolve_family(struct rtnl_handle *grth, const char *family)
{
	GENL_REQUEST(req, 1024, GENL_ID_CTRL, 0, 0, CTRL_CMD_GETFAMILY,
		     NLM_F_REQUEST);
	struct nlmsghdr *answer;
	int fnum;

	addattr_l(&req.n, sizeof(req), CTRL_ATTR_FAMILY_NAME,
		  family, strlen(family) + 1);

	if (rtnl_talk(grth, &req.n, &answer) < 0) {
		fprintf(stderr, "Error talking to the kernel\n");
		return -2;
	}

	fnum = genl_parse_getfamily(answer);
	free(answer);

	return fnum;
}

static int genl_parse_grps(struct rtattr *attr, const char *name, unsigned int *id)
{
	const struct rtattr *pos;

	rtattr_for_each_nested(pos, attr) {
		struct rtattr *tb[CTRL_ATTR_MCAST_GRP_MAX + 1];

		parse_rtattr_nested(tb, CTRL_ATTR_MCAST_GRP_MAX, pos);

		if (tb[CTRL_ATTR_MCAST_GRP_NAME] && tb[CTRL_ATTR_MCAST_GRP_ID]) {
			if (strcmp(name, rta_getattr_str(tb[CTRL_ATTR_MCAST_GRP_NAME])) == 0) {
				*id = rta_getattr_u32(tb[CTRL_ATTR_MCAST_GRP_ID]);
				return 0;
			}
		}
	}

	errno = ENOENT;
	return -1;
}

int genl_add_mcast_grp(struct rtnl_handle *grth, __u16 fnum, const char *group)
{
	GENL_REQUEST(req, 1024, GENL_ID_CTRL, 0, 0, CTRL_CMD_GETFAMILY,
		     NLM_F_REQUEST);
	struct rtattr *tb[CTRL_ATTR_MAX + 1];
	struct nlmsghdr *answer = NULL;
	struct genlmsghdr *ghdr;
	struct rtattr *attrs;
	int len, ret = -1;
	unsigned int id;

	addattr16(&req.n, sizeof(req), CTRL_ATTR_FAMILY_ID, fnum);

	if (rtnl_talk(grth, &req.n, &answer) < 0) {
		fprintf(stderr, "Error talking to the kernel\n");
		return -2;
	}

	ghdr = NLMSG_DATA(answer);
	len = answer->nlmsg_len;

	if (answer->nlmsg_type != GENL_ID_CTRL) {
		errno = EINVAL;
		goto err_free;
	}

	len -= NLMSG_LENGTH(GENL_HDRLEN);
	if (len < 0) {
		errno = EINVAL;
		goto err_free;
	}

	attrs = (struct rtattr *) ((char *) ghdr + GENL_HDRLEN);
	parse_rtattr(tb, CTRL_ATTR_MAX, attrs, len);

	if (tb[CTRL_ATTR_MCAST_GROUPS] == NULL) {
		errno = ENOENT;
		fprintf(stderr, "Missing mcast groups TLV\n");
		goto err_free;
	}

	if (genl_parse_grps(tb[CTRL_ATTR_MCAST_GROUPS], group, &id) < 0)
		goto err_free;

	ret = rtnl_add_nl_group(grth, id);

err_free:
	free(answer);
	return ret;
}

int genl_init_handle(struct rtnl_handle *grth, const char *family,
		     int *genl_family)
{
	if (*genl_family >= 0)
		return 0;

	if (rtnl_open_byproto(grth, 0, NETLINK_GENERIC) < 0) {
		fprintf(stderr, "Cannot open generic netlink socket\n");
		return -1;
	}

	*genl_family = genl_resolve_family(grth, family);
	if (*genl_family < 0)
		return -1;

	return 0;
}
