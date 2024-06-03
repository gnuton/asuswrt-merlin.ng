 /****************************************************************************
 *
 * Broadcom Proprietary and Confidential. (c) 2017 Broadcom.  All rights reserved.
 * The term "Broadcom" refers to Broadcom Limited and/or its subsidiaries.
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
 ****************************************************************************/
//**************************************************************************/
//    Filename:	     itc_rpc_dt.c
//    Author:	     Tim Ross
//    Creation Date: 4/14/13
//
//**************************************************************************/
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include <itc_rpc_dt.h>
#include <itc_channel_structs.h>

int rpc_parse_dt_node(struct platform_device *pdev)
{
	int status = 0;
	fifo_tunnel *ft = pdev->dev.platform_data;
	struct device_node *of_node = pdev->dev.of_node;
	char *str;

	status = of_property_read_string(of_node, "dev-name",
					 (const char **)&str);
	if (status)
		goto done;
	strncpy(ft->name, str, sizeof(ft->name));
	ft->name[sizeof(ft->name)-1] = '\0';

	status = of_property_read_string(of_node, "fifo-dev",
					 (const char **)&str);
	if (status)
		goto done;
	strncpy(ft->fifo_dev, str, sizeof(ft->fifo_dev));
	ft->fifo_dev[sizeof(ft->fifo_dev)-1] = '\0';

	status = of_property_read_u32(of_node, "tx-fifo",
				      &ft->tx_fifo);
	if (status)
		goto done;

	status = of_property_read_u32(of_node, "rx-fifo",
				      &ft->rx_fifo);
	if (status)
		goto done;

	status = of_property_read_u32(of_node, "orphan-token",
				      &ft->orph_tok);
	ft->handshake = status ? true : false;
	status = 0;

	ft->boot_tunnel = of_property_read_bool(of_node, "boot_tunnel");

done:
	return status;
}
