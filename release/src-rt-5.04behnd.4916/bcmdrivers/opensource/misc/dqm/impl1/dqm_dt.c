/****************************************************************************
 *
 * Broadcom Proprietary and Confidential.
 * (c) 2018 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to
 * you under the terms of the GNU General Public License version 2 (the
 * "GPL"), available at [http://www.broadcom.com/licenses/GPLv2.php], with
 * the following added to such license:
 *
 * As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy
 * and distribute the resulting executable under terms of your choice,
 * provided that you also meet, for each linked independent module, the
 * terms and conditions of the license of that module. An independent
 * module is a module which is not derived from this software. The special
 * exception does not apply to any modifications of the software.
 *
 * Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a
 * license other than the GPL, without Broadcom's express prior written
 * consent.
 *
 ****************************************************************************
 * Author: Tim Ross <tross@broadcom.com>
 *****************************************************************************/
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>

#include "dqm_dt.h"
#include "dqm_dev.h"
#include "dqm_dbg.h"

int dqm_parse_dt_prop_reg_off_array(struct device_node *of_node,
				    const char *propname, int n,
				    u32 *base, u32 *tmp, void ***dst)
{
	int status = 0;
	int i;

	if (!of_property_read_u32_array(of_node, propname, tmp, n)) {
		*dst = kzalloc(sizeof(void *) * n, GFP_KERNEL);
		if (!(*dst)) {
			status = -ENOMEM;
			goto done;
		}
		pr_debug("%s =", propname);
		for (i = 0; i < n; i++) {
			pr_debug(" %xh", tmp[i]);
			(*dst)[i] = (void *)(base + (tmp[i] >> 2));
		}
		pr_debug("\n");
	} else {
		pr_debug("Missing %s property!\n", propname);
		status = -EINVAL;
	}

done:
	return status;
}

int dqm_dt_read_u32_array(struct device_node *np,
			  const char *propname,
			  u32 *out_values, size_t sz,
			  bool required)
{
	int status;

	status = of_property_read_u32_array(np, propname, out_values, sz);
	if (status && required)
		pr_err("Missing required %s property!\n", propname);
	return status;
}

int dqm_parse_dt_node(struct platform_device *pdev)
{
	int status = 0;
	struct dqmdev *qdev = pdev->dev.platform_data;
	struct device_node *of_node = pdev->dev.of_node;
	struct resource *mem;
	char *str;
	u32 tmp;
	u32 *arr;
	int i;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		pr_err("Unable to retrieve reg base.\n");
		status = -EFAULT;
		goto done;
	}
	pr_debug("Reg Base Physical = 0x%08llx\n", (u64)mem->start);
	qdev->reg_base = ioremap(mem->start, mem->end - mem->start + 1);
	if (!qdev->reg_base) {
		pr_err("Unable to ioremap reg base.\n");
		status = -EFAULT;
		goto done;
	}
	pr_debug("Reg Base Virtual = %p\n", qdev->reg_base);
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (mem) {
		pr_debug("Data Base Physical = 0x%08llx\n", (u64)mem->start);
		qdev->data_phys_base = (u64)mem->start;
		qdev->data_base = ioremap(mem->start,
					  mem->end - mem->start + 1);
		if (!qdev->data_base) {
			pr_err("Unable to ioremap reg base.\n");
			status = -EFAULT;
			goto done;
		}
		pr_debug("Data Base Virtual = %p\n", qdev->data_base);
	}

	status = of_property_read_string(of_node, "dev-name",
					 (const char **)&str);
	if (status)
		goto err_unmap;
	strncpy(qdev->name, str, sizeof(qdev->name));

	qdev->name[sizeof(qdev->name)-1] = '\0';
	status = of_property_read_u32(of_node, "q-count",
				      &qdev->q_count);
	if (status)
		goto err_unmap;

	qdev->bank_count = ((qdev->q_count - 1) >> 5) + 1;

	arr = kzalloc(sizeof(u32) * qdev->bank_count, GFP_KERNEL);
	if (!arr) {
		status = -ENOMEM;
		goto err_unmap;
	}
	/*
	 * q-word-count is deprecated & replaced with fpm-alloc-offset which
	 * is a reg offset for OL Q banks and 0 for non-OL Q banks.
	 */
	status = dqm_parse_dt_prop_reg_off_array(of_node, "fpm-alloc-offset",
			qdev->bank_count, qdev->reg_base, arr,
			(void ***)&qdev->fpm_alloc);
	if (!status) {
		for (i = 0; i < qdev->bank_count; i++)
			if (qdev->fpm_alloc[i] == qdev->reg_base)
				qdev->fpm_alloc[i] = NULL;
	}

	qdev->cfg_qsm = of_property_read_bool(of_node, "cfg-qsm");

	qdev->offload = of_property_read_bool(of_node, "offload");

	status = of_property_read_u32(of_node, "qsm-size",
				      &qdev->qsm_size);

	status = of_property_read_u32(of_node, "token-offset",
				      &tmp);
	if (!status)
		qdev->tok_base = (struct dqmtok *)(qdev->reg_base + (tmp >> 2));
	else
		qdev->tok_base = NULL;

	qdev->restricted_access = of_property_read_bool(of_node, "restricted-access");
	if (qdev->restricted_access) {
		/* Restricted access override cfg_qsm */
		qdev->cfg_qsm = 0;
	} else {
		status = dqm_parse_dt_prop_reg_off_array(of_node, "q-ctl-base-offset",
				qdev->bank_count, qdev->reg_base, arr,
				(void ***)&qdev->q_ctl_base);
		if (status)
			goto err_free_fpm_alloc;
	}

	if (qdev->cfg_qsm) {
		status = dqm_parse_dt_prop_reg_off_array(of_node, "cfg-offset",
				qdev->bank_count, qdev->reg_base, arr,
				(void ***)&qdev->cfg);
		if (status)
			goto err_free_fpm_alloc;
	} else {
		/* Use cfg for reading counters */
		status = dqm_parse_dt_prop_reg_off_array(of_node, "cfg-offset",
				qdev->bank_count, qdev->reg_base, arr,
				(void ***)&qdev->cfg);
	}

	status = dqm_parse_dt_prop_reg_off_array(of_node, "q-tmr-base-offset",
			qdev->bank_count, qdev->reg_base, arr,
			(void ***)&qdev->q_tmr_base);
	if (status) {
		qdev->q_tmr_base = kzalloc(sizeof(u32 *) * qdev->bank_count,
					   GFP_KERNEL);
		if (!qdev->q_tmr_base) {
			status = -ENOMEM;
			goto err_free_q_ctl_base;
		}
	}

	if (qdev->data_base) {
		status = dqm_parse_dt_prop_reg_off_array(of_node,
				"q-data-base-offset",
				qdev->bank_count, qdev->data_base, arr,
				(void ***)&qdev->q_data_base);
	} else {
		status = dqm_parse_dt_prop_reg_off_array(of_node,
				"q-data-base-offset",
				qdev->bank_count, qdev->reg_base, arr,
				(void ***)&qdev->q_data_base);
	}
	if (status)
		goto err_free_q_tmr_base;

	status = dqm_parse_dt_prop_reg_off_array(of_node,
			"q-status-base-offset",
			qdev->bank_count, qdev->reg_base, arr,
			(void ***)&qdev->q_status_base);
	if (status)
		goto err_free_q_data_base;

	status = dqm_parse_dt_prop_reg_off_array(of_node, "q-mib-base-offset",
			qdev->bank_count, qdev->reg_base, arr,
			(void ***)&qdev->q_mib_base);
	if (status) {
		qdev->q_mib_base = kzalloc(sizeof(void *) * qdev->bank_count,
					   GFP_KERNEL);
		if (!qdev->q_mib_base) {
			status = -ENOMEM;
			goto err_free_q_status_base;
		}
		for (i = 0; i < qdev->bank_count; i++)
			qdev->q_mib_base[i] = NULL;
		status = 0;
	}

	status = of_property_read_u32(of_node, "probe-defer",
				      &tmp);
	if (status) {
		qdev->probe_defer = 0;
		status = 0;
	} else {
		qdev->probe_defer = tmp;
		status = of_property_read_u32(of_node, "defer-offset",
					      &tmp);

		if (status) {
			goto err_free_q_status_base;
		}

		qdev->probe_defer_register = qdev->reg_base + (tmp >> 2);

		status = of_property_read_u32(of_node, "defer-mask",
					      &qdev->defer_mask);

		if (status) {
			goto err_free_q_status_base;
		}
		status = of_property_read_u32(of_node, "defer-value",
					      &qdev->defer_value);
		if (status)
			goto err_free_q_status_base;
	}

	kfree(arr);
	goto done;


err_free_q_status_base:
	kfree(qdev->q_status_base);
err_free_q_data_base:
	kfree(qdev->q_data_base);
err_free_q_tmr_base:
	kfree(qdev->q_tmr_base);
err_free_q_ctl_base:
	kfree(qdev->q_ctl_base);
err_free_fpm_alloc:
	kfree(qdev->fpm_alloc);
err_unmap:
	iounmap(qdev->reg_base);

done:
	return status;
}

int dqm_parse_dt_node_child(struct platform_device *pdev,
			    struct _qsm_alloc *qsm_alloc)
{
	int status = 0, id;
	struct dqmdev *qdev = pdev->dev.platform_data;
	struct device_node *of_node = pdev->dev.of_node;
	struct device_node *child;
	char *str;

	if (!of_get_child_count(of_node))
		return -ENOENT;

	for_each_child_of_node(of_node, child) {

		if (of_property_read_bool(child, "interrupt-controller"))
			continue;

		status = of_property_read_u32(child, "id", &id);
		if (status) {
			pr_err("dqm: Unable to retrieve child node id\n");
			return status;
		}

		if (id >= qdev->q_count) {
			pr_info("dqm: Error id %d > %d\n", id, qdev->q_count);
			return status;
		}
		qdev->dqm[id].irq = irq_of_parse_and_map(child, 0);

		status = of_property_read_string(child, "qname",
						 (const char **)&str);
		if (!status)
			strncpy(qsm_alloc[id].name, str, sizeof(qsm_alloc[id].name)-1);
		else
			snprintf(qsm_alloc[id].name, DQM_NAME_SIZE-1, "%s%d", qdev->name, id);


		of_property_read_u32(child, "words", &qsm_alloc[id].msg_size);
		of_property_read_u32(child, "depth", &qsm_alloc[id].depth);
		of_property_read_u32(child, "lwm", &qsm_alloc[id].lwm);
		of_property_read_u32(child, "hwm", &qsm_alloc[id].hwm);
		of_property_read_u32(child, "timeout", &qsm_alloc[id].timeout);
		qsm_alloc[id].offload =
			!of_property_read_bool(child, "non-offload");

		pr_debug("dt parse Q %s id %d irq %d words %d depth %d lwm %d hwm %d\n",
			 str, id, qdev->dqm[id].irq, qsm_alloc[id].msg_size,
			 qsm_alloc[id].depth, qsm_alloc[id].lwm, qsm_alloc[id].hwm);
		qsm_alloc[id].dt_valid = true;
	}
	return 0;
}
