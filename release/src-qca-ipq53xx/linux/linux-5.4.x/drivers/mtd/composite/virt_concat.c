// SPDX-License-Identifier: GPL-2.0+
/*
 * Virtual concat MTD device driver
 *
 * Copyright (C) 2018 Bernhard Frauendienst
 * Author: Bernhard Frauendienst, kernel@nospam.obeliks.de
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/mtd/concat.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/slab.h>

/*
 * struct of_virt_concat - platform device driver data.
 * @cmtd the final mtd_concat device
 * @num_devices the number of devices in @devices
 * @devices points to an array of devices already loaded
 */
struct of_virt_concat {
	struct mtd_info	*cmtd;
	int num_devices;
	struct mtd_info	**devices;
};

static int virt_concat_remove(struct platform_device *pdev)
{
	struct of_virt_concat *info;
	int i;

	info = platform_get_drvdata(pdev);
	if (!info)
		return 0;

	// unset data for when this is called after a probe error
	platform_set_drvdata(pdev, NULL);

	if (info->cmtd) {
		mtd_device_unregister(info->cmtd);
		mtd_concat_destroy(info->cmtd);
	}

	if (info->devices) {
		for (i = 0; i < info->num_devices; i++)
			put_mtd_device(info->devices[i]);
	}

	return 0;
}

static int virt_concat_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct of_phandle_iterator it;
	struct of_virt_concat *info;
	struct mtd_info *mtd;
	int err = 0, count;

	count = of_count_phandle_with_args(node, "devices", NULL);
	if (count <= 0)
		return -EINVAL;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	info->devices = devm_kcalloc(&pdev->dev, count,
				     sizeof(*(info->devices)), GFP_KERNEL);
	if (!info->devices) {
		err = -ENOMEM;
		goto err_remove;
	}

	platform_set_drvdata(pdev, info);

	of_for_each_phandle(&it, err, node, "devices", NULL, 0) {
		mtd = get_mtd_device_by_node(it.node);
		if (IS_ERR(mtd)) {
			of_node_put(it.node);
			err = -EPROBE_DEFER;
			goto err_remove;
		}

		info->devices[info->num_devices++] = mtd;
	}

	info->cmtd = mtd_concat_create(info->devices, info->num_devices,
				       dev_name(&pdev->dev));
	if (!info->cmtd) {
		err = -ENXIO;
		goto err_remove;
	}

	info->cmtd->dev.parent = &pdev->dev;
	mtd_set_of_node(info->cmtd, node);
	mtd_device_register(info->cmtd, NULL, 0);

	return 0;

err_remove:
	virt_concat_remove(pdev);

	return err;
}

static const struct of_device_id virt_concat_of_match[] = {
	{ .compatible = "mtd-concat", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, virt_concat_of_match);

static struct platform_driver virt_concat_driver = {
	.probe = virt_concat_probe,
	.remove = virt_concat_remove,
	.driver	 = {
		.name   = "virt-mtdconcat",
		.of_match_table = virt_concat_of_match,
	},
};

module_platform_driver(virt_concat_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Bernhard Frauendienst <kernel@nospam.obeliks.de>");
MODULE_DESCRIPTION("Virtual concat MTD device driver");
