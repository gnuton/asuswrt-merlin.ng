/* Copyright (c) 2014, 2020 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/of.h>

#include "mdss_panel.h"
#include "mdss_qpic.h"
#include "mdss_qpic_panel.h"

static u32 panel_is_on;
static u32 panel_refresh_rate;

int (*qpic_panel_on)(struct qpic_panel_io_desc *qpic_panel_io);
EXPORT_SYMBOL(qpic_panel_on);
void (*qpic_panel_off)(struct qpic_panel_io_desc *qpic_panel_io);
EXPORT_SYMBOL(qpic_panel_off);

u32 qpic_panel_get_framerate(void)
{
	return panel_refresh_rate;
}

/* write a frame of pixels to a MIPI screen */
u32 qpic_send_frame(u32 x_start,
				u32 y_start,
				u32 x_end,
				u32 y_end,
				u32 *data,
				u32 total_bytes)
{
	u8 param[4];
	u32 status;
	u32 start_0_7;
	u32 end_0_7;
	u32 start_8_15;
	u32 end_8_15;

	/* convert to 16 bit representation */
	x_start = x_start & 0xffff;
	y_start = y_start & 0xffff;
	x_end = x_end & 0xffff;
	y_end = y_end & 0xffff;

	/* set column/page */
	start_0_7 = x_start & 0xff;
	end_0_7 = x_end & 0xff;
	start_8_15 = (x_start >> 8) & 0xff;
	end_8_15 = (x_end >> 8) & 0xff;
	param[0] = start_8_15;
	param[1] = start_0_7;
	param[2] = end_8_15;
	param[3] = end_0_7;
	status = qpic_send_pkt(OP_SET_COLUMN_ADDRESS, param, 4);
	if (status) {
		pr_err("Failed to set column address");
		return status;
	}

	start_0_7 = y_start & 0xff;
	end_0_7 = y_end & 0xff;
	start_8_15 = (y_start >> 8) & 0xff;
	end_8_15 = (y_end >> 8) & 0xff;
	param[0] = start_8_15;
	param[1] = start_0_7;
	param[2] = end_8_15;
	param[3] = end_0_7;
	status = qpic_send_pkt(OP_SET_PAGE_ADDRESS, param, 4);
	if (status) {
		pr_err("Failed to set page address");
		return status;
	}

	status = qpic_send_pkt(OP_WRITE_MEMORY_START, (u8 *)data, total_bytes);
	if (status) {
		pr_err("Failed to start memory write");
		return status;
	}
	return 0;
}

int mdss_qpic_panel_on(struct mdss_panel_data *pdata,
	struct qpic_panel_io_desc *panel_io)
{
	int rc = 0;

	if (panel_is_on)
		return 0;
	mdss_qpic_init();

	if (!panel_io->init) {
		panel_io->init = true;
	}

	if (qpic_panel_on)
		rc = qpic_panel_on(panel_io);
	if (rc)
		return rc;
	panel_is_on = true;
	mdss_qpic_set_cfg0();
	return 0;
}

int mdss_qpic_panel_off(struct mdss_panel_data *pdata,
	struct qpic_panel_io_desc *panel_io)
{
	if (qpic_panel_off)
		qpic_panel_off(panel_io);

	panel_is_on = false;
	return 0;
}

static int mdss_panel_parse_dt(struct platform_device *pdev,
			       struct mdss_panel_data *panel_data)
{
	struct device_node *np = pdev->dev.of_node;
	u32 res[6], tmp;
	int rc;

	rc = of_property_read_u32_array(np, "qti,mdss-pan-res", res, 2);
	if (rc) {
		pr_err("%s:%d, panel resolution not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	panel_data->panel_info.xres = (!rc ? res[0] : 240);
	panel_data->panel_info.yres = (!rc ? res[1] : 320);
	rc = of_property_read_u32(np, "qti,mdss-pan-bpp", &tmp);
	if (rc) {
		pr_err("%s:%d, panel bpp not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	panel_data->panel_info.bpp = (!rc ? tmp : 24);
	of_property_read_u32(np, "qti,refresh_rate", &panel_refresh_rate);

	panel_data->panel_info.type = EBI2_PANEL;
	panel_data->panel_info.pdest = DISPLAY_1;

	return rc;
}

static int mdss_qpic_panel_probe(struct platform_device *pdev)
{
	int rc = 0;
	static struct mdss_panel_data vendor_pdata;
	static const char *panel_name;

	pr_debug("%s:%d, debug info id=%d", __func__, __LINE__, pdev->id);
	if (!pdev->dev.of_node)
		return -ENODEV;

	panel_name = of_get_property(pdev->dev.of_node, "label", NULL);
	if (!panel_name)
		pr_info("%s:%d, panel name not specified\n",
						__func__, __LINE__);
	else
		pr_info("%s: Panel Name = %s\n", __func__, panel_name);

	rc = mdss_panel_parse_dt(pdev, &vendor_pdata);
	if (rc)
		return rc;

	rc = qpic_register_panel(&vendor_pdata);
	if (rc)
		return rc;

	return 0;
}

static const struct of_device_id mdss_qpic_panel_match[] = {
	{.compatible = "qti,mdss-qpic-panel"},
	{}
};

static struct platform_driver this_driver = {
	.probe  = mdss_qpic_panel_probe,
	.driver = {
		.name = "qpic_panel",
		.of_match_table = mdss_qpic_panel_match,
	},
};

static int __init mdss_qpic_panel_init(void)
{
	return platform_driver_register(&this_driver);
}
MODULE_DEVICE_TABLE(of, mdss_qpic_panel_match);
module_init(mdss_qpic_panel_init);
