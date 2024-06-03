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
 * Author: Tim Ross <tross@broadcom.com>
 *****************************************************************************/
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "fpm_dev.h"
#include "fpm_priv.h"
#include "fpm_dt.h"

int fpm_parse_dt_prop_u32(struct device_node *of_node,
			  const char *propname, u32 *dst)
{
	int status = 0;

	if (!of_property_read_u32(of_node, propname, dst)) {
		pr_debug("%s = %xh\n", propname, *dst);
	} else {
		pr_debug("Missing %s property!\n", propname);
		status = -EINVAL;
	}

	return status;
}

int fpm_parse_dt_prop_u32_array(struct device_node *of_node,
			        const char *propname, int n,
			        u32 *dst)
{
	int status = 0;
	int i;

	if (!of_property_read_u32_array(of_node, propname, dst, n)) {
		pr_debug("%s =", propname);
		for (i = 0; i < n; i++)
			pr_debug(" %xh", dst[i]);
		pr_debug("\n");
	} else {
		pr_debug("Missing %s property!\n", propname);
		status = -EINVAL;
	}

	return status;
}

int fpm_parse_dt_node(struct platform_device *pdev)
{
	int status = 0, i;
	struct fpmdev *fdev = pdev->dev.platform_data;
	struct device_node *of_node = pdev->dev.of_node;
	struct resource *mem;

	fdev->init = of_property_read_bool(of_node, "init");

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		pr_err("Unable to retrieve reg base.\n");
		status = -EFAULT;
		goto done;
	}
	fdev->reg_pbase = (u32 *)(uintptr_t)mem->start;
	fdev->reg_vbase = ioremap(mem->start, mem->end - mem->start);
	if (!fdev->reg_vbase) {
		pr_err("Unable to ioremap reg base.\n");
		status = -EFAULT;
		goto done;
	}

	fdev->irq = platform_get_irq(pdev, 0);

	fdev->pad_in_ctrl_spare = of_property_read_bool(of_node,
		"pad-in-ctrl-spare");

	for (i = 0,
		mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		mem;
		i++,
		mem = platform_get_resource(pdev,
					IORESOURCE_MEM, i + 1)) {
		fdev->pool_pbase[i] = mem->start;
		fdev->pool_size[i] = (u32)resource_size(mem);
	}
	fdev->npools = i;
	if (!fdev->npools) {
		pr_err("Pool memory not specified.\n");
		goto err_unmap_reg;
	}
	
	fdev->lwm_wd_timeout = 0;
	fpm_parse_dt_prop_u32(of_node, "lwm-watchdog-timeout",
			      &fdev->lwm_wd_timeout);
	fdev->lwm_wd_timeout = msecs_to_jiffies(fdev->lwm_wd_timeout);

	fdev->track_tokens = of_property_read_bool(of_node, "track-tokens");
	fdev->track_on_err = of_property_read_bool(of_node, "track-on-err");

	goto done;

err_unmap_reg:
	iounmap(fdev->reg_vbase);

done:
	return status;
}
