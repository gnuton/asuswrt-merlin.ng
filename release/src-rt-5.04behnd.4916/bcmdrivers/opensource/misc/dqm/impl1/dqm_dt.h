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
#ifndef _DQM_DT_H_
#define _DQM_DT_H_

#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include "dqm_dev.h"

int dqm_parse_dt_prop_reg_off_array(struct device_node *of_node,
				    const char *propname, int n,
				    u32 *base, u32 *tmp, void ***dst);
int dqm_dt_read_u32_array(struct device_node *np,
			  const char *propname,
			  u32 *out_values, size_t sz,
			  bool required);
int dqm_parse_dt_node(struct platform_device *pdev);
int dqm_parse_dt_node_child(struct platform_device *pdev,
			    struct _qsm_alloc *qsm_alloc);

#endif
