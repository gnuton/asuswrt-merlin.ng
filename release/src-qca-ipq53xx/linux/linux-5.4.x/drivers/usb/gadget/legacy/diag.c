/*
 * diag.c -- Diag Gadget driver
 *
 * Copyright (c) 2019-2020 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/platform_device.h>

#include "u_diag.h"

/*-------------------------------------------------------------------------*/

static const char longname[] = "diag";

USB_GADGET_COMPOSITE_OPTIONS();

#define DRIVER_VENDOR_NUM	0x05C6
#define DRIVER_PRODUCT_NUM	0x9091

/* string IDs are assigned dynamically */

#define STRING_DESCRIPTION_IDX		USB_GADGET_FIRST_AVAIL_IDX

static struct usb_device_descriptor device_desc = {
	.bLength =		USB_DT_DEVICE_SIZE,
	.bDescriptorType =	USB_DT_DEVICE,
	.bcdUSB =		cpu_to_le16(0x0200),
	.bDeviceClass =		USB_CLASS_PER_INTERFACE,
	.idVendor =		cpu_to_le16(DRIVER_VENDOR_NUM),
	.idProduct =		cpu_to_le16(DRIVER_PRODUCT_NUM),
	.bNumConfigurations =	1,
};

static struct usb_string strings_dev[] = {
	[USB_GADGET_MANUFACTURER_IDX].s	= "Qualcomm Technologies",
	[USB_GADGET_PRODUCT_IDX].s	= "Diag Gadget",
	[USB_GADGET_SERIAL_IDX].s	= "",
	{  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_function_instance *fi_diag;
static struct usb_function *f_diag;

static int diag_gadget_unbind(struct usb_composite_dev *dev)
{
	usb_put_function(f_diag);
	usb_put_function_instance(fi_diag);
	return 0;
}

static struct usb_configuration diag_config = {
	.label		= "diag",
	.bConfigurationValue = 1,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.MaxPower	= CONFIG_USB_GADGET_VBUS_DRAW,
};

static int diag_bind_config(struct usb_configuration *c)
{
	int status;

	f_diag = usb_get_function(fi_diag);
	if (IS_ERR(f_diag))
		return PTR_ERR(f_diag);

	status = usb_add_function(c, f_diag);
	if (status < 0) {
		usb_put_function(f_diag);
		return status;
	}

	return 0;
}

static int diag_gadget_bind(struct usb_composite_dev *cdev)
{
	struct diag_opts *diag_opts;
	int status;

	fi_diag = usb_get_function_instance("diag");
	if (IS_ERR(fi_diag))
		return PTR_ERR(fi_diag);

	diag_opts = container_of(fi_diag, struct diag_opts, func_inst);
	fi_diag->set_inst_name(fi_diag, "diag");

	status = usb_string_ids_tab(cdev, strings_dev);
	if (status < 0)
		goto put;

	device_desc.iManufacturer = strings_dev[USB_GADGET_MANUFACTURER_IDX].id;
	device_desc.iProduct = strings_dev[USB_GADGET_PRODUCT_IDX].id;
	diag_config.iConfiguration = strings_dev[STRING_DESCRIPTION_IDX].id;

	status = usb_add_config(cdev, &diag_config, diag_bind_config);
	if (status < 0)
		goto put;

	usb_composite_overwrite_options(cdev, &coverwrite);

	return 0;
put:
	usb_put_function_instance(fi_diag);
	return status;
}

static struct usb_composite_driver diag_gadget_driver = {
	.name		= (char *) longname,
	.dev		= &device_desc,
	.strings	= dev_strings,
	.max_speed	= USB_SPEED_HIGH,
	.bind		= diag_gadget_bind,
	.unbind		= diag_gadget_unbind,
};

static int diag_gadget_probe(struct platform_device *pdev)
{
	usb_composite_probe(&diag_gadget_driver);
	return 0;
}

static int diag_gadget_remove(struct platform_device *pdev)
{
	usb_composite_unregister(&diag_gadget_driver);
	return 0;
}

static const struct of_device_id diag_gadget_table[] = {
	{.compatible = "qti,gadget-diag"},
	{},
};
MODULE_DEVICE_TABLE(of, diag_gadget_table);

static struct platform_driver diag_gadget_platform_driver = {
	.probe = diag_gadget_probe,
	.remove = diag_gadget_remove,
	.driver = {
		.name = "DIAG Gadget Platform",
		.owner = THIS_MODULE,
		.of_match_table = diag_gadget_table,
	},
};

module_platform_driver(diag_gadget_platform_driver);
MODULE_DESCRIPTION("Qualcomm Technologies, Inc. Diag Gadget Driver");
MODULE_LICENSE("Dual BSD/GPL");
