// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/*
 * res-srq.c	RDMA tool
 * Authors:     Neta Ostrovsky <netao@nvidia.com>
 */

#include "res.h"
#include <inttypes.h>

#define MAX_QP_STR_LEN 256

static const char *srq_types_to_str(uint8_t idx)
{
	static const char *const srq_types_str[] = { "BASIC",
						     "XRC",
						     "TM" };

	if (idx < ARRAY_SIZE(srq_types_str))
		return srq_types_str[idx];
	return "UNKNOWN";
}

static void print_type(struct rd *rd, uint32_t val)
{
	print_color_string(PRINT_ANY, COLOR_NONE, "type", "type %s ",
			   srq_types_to_str(val));
}

static void print_qps(char *qp_str)
{
	char *qpn;

	if (!strlen(qp_str))
		return;

	open_json_array(PRINT_ANY, "lqpn");
	print_color_string(PRINT_FP, COLOR_NONE, NULL, " ", NULL);
	qpn = strtok(qp_str, ",");
	while (qpn) {
		print_color_string(PRINT_ANY, COLOR_NONE, NULL, "%s", qpn);
		qpn = strtok(NULL, ",");
		if (qpn)
			print_color_string(PRINT_FP, COLOR_NONE, NULL, ",", NULL);
	}
	print_color_string(PRINT_FP, COLOR_NONE, NULL, " ", NULL);
	close_json_array(PRINT_JSON, NULL);
}

static int filter_srq_range_qps(struct rd *rd, struct nlattr **qp_line,
				uint32_t min_range, uint32_t max_range,
				char **delimiter, char *qp_str)
{
	uint32_t qpn = 0, tmp_min_range = 0, tmp_max_range = 0;
	char tmp[16] = {};

	for (qpn = min_range; qpn <= max_range; qpn++) {
		if (rd_is_filtered_attr(rd, "lqpn", qpn,
				qp_line[RDMA_NLDEV_ATTR_MIN_RANGE])) {
			/* The QPs range contains a LQPN that is filtered */
			if (!tmp_min_range)
				/* There are no QPs previous to
				 * the filtered one
				 */
				continue;
			if (!tmp_max_range)
				snprintf(tmp, sizeof(tmp), "%s%d", *delimiter,
					 tmp_min_range);
			else
				snprintf(tmp, sizeof(tmp), "%s%d-%d",
					 *delimiter, tmp_min_range,
					 tmp_max_range);

			strncat(qp_str, tmp,
				MAX_QP_STR_LEN - strlen(qp_str) - 1);

			memset(tmp, 0, strlen(tmp));
			*delimiter = ",";
			tmp_min_range = 0;
			tmp_max_range = 0;
			continue;
		}
		if (!tmp_min_range)
			tmp_min_range = qpn;
		else
			tmp_max_range = qpn;
	}

	if (!tmp_min_range)
		return 0;
	if (!tmp_max_range)
		snprintf(tmp, sizeof(tmp), "%s%d", *delimiter, tmp_min_range);
	else
		snprintf(tmp, sizeof(tmp), "%s%d-%d", *delimiter,
			 tmp_min_range, tmp_max_range);

	strncat(qp_str, tmp, MAX_QP_STR_LEN - strlen(qp_str) - 1);
	*delimiter = ",";
	return 0;
}

static int get_srq_qps(struct rd *rd, struct nlattr *qp_table,  char *qp_str)
{
	uint32_t qpn = 0, min_range = 0, max_range = 0;
	struct nlattr *nla_entry;
	struct filter_entry *fe;
	char *delimiter = "";
	char tmp[16] = {};

	if (!qp_table)
		return MNL_CB_ERROR;

	/* If there are no QPs associated with the SRQ, return */
	if (!(mnl_attr_get_payload_len(qp_table))) {
		list_for_each_entry(fe, &rd->filter_list, list) {
			if (!strcmpx(fe->key, "lqpn"))
				/* We found the key -
				 * user requested to filter by LQPN
				 */
				return -EINVAL;
		}
		return MNL_CB_OK;
	}

	mnl_attr_for_each_nested(nla_entry, qp_table) {
		struct nlattr *qp_line[RDMA_NLDEV_ATTR_MAX] = {};

		if (mnl_attr_parse_nested(nla_entry, rd_attr_cb, qp_line) !=
		    MNL_CB_OK)
			goto out;

		if (qp_line[RDMA_NLDEV_ATTR_RES_LQPN]) {
			qpn = mnl_attr_get_u32(qp_line[RDMA_NLDEV_ATTR_RES_LQPN]);
			if (rd_is_filtered_attr(rd, "lqpn", qpn,
					qp_line[RDMA_NLDEV_ATTR_RES_LQPN]))
				continue;
			snprintf(tmp, sizeof(tmp), "%s%d", delimiter, qpn);
			strncat(qp_str, tmp,
				MAX_QP_STR_LEN - strlen(qp_str) - 1);
			delimiter = ",";
		} else if (qp_line[RDMA_NLDEV_ATTR_MIN_RANGE] &&
			   qp_line[RDMA_NLDEV_ATTR_MAX_RANGE]) {
			min_range = mnl_attr_get_u32(qp_line[RDMA_NLDEV_ATTR_MIN_RANGE]);
			max_range = mnl_attr_get_u32(qp_line[RDMA_NLDEV_ATTR_MAX_RANGE]);

			if (filter_srq_range_qps(rd, qp_line, min_range,
						 max_range, &delimiter,
						 qp_str))
				goto out;
		} else {
			goto out;
		}
	}

	if (!strlen(qp_str))
		/* Check if there are no QPs to display after filter */
		goto out;

	return MNL_CB_OK;

out:
	memset(qp_str, 0, strlen(qp_str));
	return -EINVAL;
}

static int res_srq_line(struct rd *rd, const char *name, int idx,
			struct nlattr **nla_line)
{
	uint32_t srqn = 0, pid = 0, pdn = 0, cqn = 0;
	char qp_str[MAX_QP_STR_LEN] = {};
	char *comm = NULL;
	uint8_t type = 0;

	if (!nla_line[RDMA_NLDEV_ATTR_RES_SRQN])
		return MNL_CB_ERROR;

	if (nla_line[RDMA_NLDEV_ATTR_RES_PID]) {
		SPRINT_BUF(b);

		pid = mnl_attr_get_u32(nla_line[RDMA_NLDEV_ATTR_RES_PID]);
		if (!get_task_name(pid, b, sizeof(b)))
			comm = b;
	} else if (nla_line[RDMA_NLDEV_ATTR_RES_KERN_NAME]) {
		/* discard const from mnl_attr_get_str */
		comm = (char *)mnl_attr_get_str(
			nla_line[RDMA_NLDEV_ATTR_RES_KERN_NAME]);
	}

	if (rd_is_filtered_attr(rd, "pid", pid,
				nla_line[RDMA_NLDEV_ATTR_RES_PID]))
		goto out;

	if (nla_line[RDMA_NLDEV_ATTR_RES_SRQN])
		srqn = mnl_attr_get_u32(nla_line[RDMA_NLDEV_ATTR_RES_SRQN]);
	if (rd_is_filtered_attr(rd, "srqn", srqn,
				nla_line[RDMA_NLDEV_ATTR_RES_SRQN]))
		goto out;

	if (nla_line[RDMA_NLDEV_ATTR_RES_TYPE])
		type = mnl_attr_get_u8(nla_line[RDMA_NLDEV_ATTR_RES_TYPE]);
	if (rd_is_string_filtered_attr(rd, "type", srq_types_to_str(type),
				       nla_line[RDMA_NLDEV_ATTR_RES_TYPE]))
		goto out;

	if (nla_line[RDMA_NLDEV_ATTR_RES_PDN])
		pdn = mnl_attr_get_u32(nla_line[RDMA_NLDEV_ATTR_RES_PDN]);
	if (rd_is_filtered_attr(rd, "pdn", pdn,
				nla_line[RDMA_NLDEV_ATTR_RES_PDN]))
		goto out;

	if (nla_line[RDMA_NLDEV_ATTR_RES_CQN])
		cqn = mnl_attr_get_u32(nla_line[RDMA_NLDEV_ATTR_RES_CQN]);
	if (rd_is_filtered_attr(rd, "cqn", cqn,
				nla_line[RDMA_NLDEV_ATTR_RES_CQN]))
		goto out;

	if (get_srq_qps(rd, nla_line[RDMA_NLDEV_ATTR_RES_QP], qp_str) !=
			MNL_CB_OK)
		goto out;

	open_json_object(NULL);
	print_dev(rd, idx, name);
	res_print_u32(rd, "srqn", srqn, nla_line[RDMA_NLDEV_ATTR_RES_SRQN]);
	print_type(rd, type);
	print_qps(qp_str);
	res_print_u32(rd, "pdn", pdn, nla_line[RDMA_NLDEV_ATTR_RES_PDN]);
	res_print_u32(rd, "cqn", cqn, nla_line[RDMA_NLDEV_ATTR_RES_CQN]);
	res_print_u32(rd, "pid", pid, nla_line[RDMA_NLDEV_ATTR_RES_PID]);
	print_comm(rd, comm, nla_line);

	print_driver_table(rd, nla_line[RDMA_NLDEV_ATTR_DRIVER]);
	newline(rd);

out:
	return MNL_CB_OK;
}

int res_srq_idx_parse_cb(const struct nlmsghdr *nlh, void *data)
{
	struct nlattr *tb[RDMA_NLDEV_ATTR_MAX] = {};
	struct rd *rd = data;
	const char *name;
	uint32_t idx;

	mnl_attr_parse(nlh, 0, rd_attr_cb, tb);
	if (!tb[RDMA_NLDEV_ATTR_DEV_INDEX] || !tb[RDMA_NLDEV_ATTR_DEV_NAME])
		return MNL_CB_ERROR;

	name = mnl_attr_get_str(tb[RDMA_NLDEV_ATTR_DEV_NAME]);
	idx = mnl_attr_get_u32(tb[RDMA_NLDEV_ATTR_DEV_INDEX]);

	return res_srq_line(rd, name, idx, tb);
}

int res_srq_parse_cb(const struct nlmsghdr *nlh, void *data)
{
	struct nlattr *tb[RDMA_NLDEV_ATTR_MAX] = {};
	struct nlattr *nla_table, *nla_entry;
	struct rd *rd = data;
	int ret = MNL_CB_OK;
	const char *name;
	uint32_t idx;

	mnl_attr_parse(nlh, 0, rd_attr_cb, tb);
	if (!tb[RDMA_NLDEV_ATTR_DEV_INDEX] || !tb[RDMA_NLDEV_ATTR_DEV_NAME] ||
	    !tb[RDMA_NLDEV_ATTR_RES_SRQ])
		return MNL_CB_ERROR;

	name = mnl_attr_get_str(tb[RDMA_NLDEV_ATTR_DEV_NAME]);
	idx = mnl_attr_get_u32(tb[RDMA_NLDEV_ATTR_DEV_INDEX]);
	nla_table = tb[RDMA_NLDEV_ATTR_RES_SRQ];

	mnl_attr_for_each_nested(nla_entry, nla_table) {
		struct nlattr *nla_line[RDMA_NLDEV_ATTR_MAX] = {};

		ret = mnl_attr_parse_nested(nla_entry, rd_attr_cb, nla_line);
		if (ret != MNL_CB_OK)
			break;

		ret = res_srq_line(rd, name, idx, nla_line);
		if (ret != MNL_CB_OK)
			break;
	}
	return ret;
}
