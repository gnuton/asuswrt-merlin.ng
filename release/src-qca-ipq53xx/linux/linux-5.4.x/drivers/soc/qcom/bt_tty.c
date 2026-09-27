/*
 * Copyright (c) 2020 The Linux Foundation. All rights reserved.
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

#include <linux/of.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
#include <linux/debugfs.h>
#include <linux/mfd/syscon.h>
#include <uapi/linux/major.h>
#include <linux/ipc_logging.h>
#include <linux/remoteproc.h>
#include <linux/clk.h>
#include <linux/bt.h>

static bool btss_debug;
module_param(btss_debug, bool, 0644);

unsigned int pid_distinct(struct bt_descriptor *btDesc, enum pid_ops action)
{
	int ret;
	struct list_head *pid_q = &btDesc->pid_q;
	pid_t pid = current->tgid;
	struct pid_n *pid_cursor;
	struct platform_device *rproc_pdev = btDesc->rproc_pdev;
	struct rproc *rproc = platform_get_drvdata(rproc_pdev);
	struct device *dev = &btDesc->pdev->dev;

	list_for_each_entry(pid_cursor, pid_q, list) {
		if (pid_cursor->pid == pid) {
			switch (action) {
			case ADD:
				atomic_inc(&pid_cursor->refcnt);
				break;
			case REMOVE:
				atomic_dec(&pid_cursor->refcnt);
				break;
			case TERMINATE:
				ret = atomic_read(&pid_cursor->refcnt);
				atomic_sub(ret - 1, &rproc->power);
				list_del(&pid_cursor->list);
				kfree(pid_cursor);
				pid_cursor = NULL;
				break;
			default:
				dev_info(dev, "Invalid operation\n");
				pr_err("Invalid operation\n");
				return -ENOENT;
			}
			goto out;
		}
	}

	pid_cursor = kzalloc(sizeof(struct pid_n), GFP_KERNEL);
	if (!pid_cursor)
		return -ENOMEM;

	pid_cursor->pid = pid;
	atomic_inc(&pid_cursor->refcnt);
	list_add_tail(&pid_cursor->list, pid_q);
out:
	return pid_cursor ? atomic_read(&pid_cursor->refcnt) : 0;
}
EXPORT_SYMBOL(pid_distinct);

void pid_show(struct bt_descriptor *btDesc)
{
	struct pid_n *pid_cursor;
	struct list_head *pid_q = &btDesc->pid_q;
	struct device *dev = &btDesc->pdev->dev;

	dev_info(&btDesc->pdev->dev, "Rgistered PIDS:\n");
	list_for_each_entry(pid_cursor, pid_q, list) {
		dev_info(dev, "%d\n", pid_cursor->pid);
	}
}

int bt_ipc_avail_size(struct bt_descriptor *btDesc)
{
	return tty_buffer_space_avail(&btDesc->tty_port);
}

static
void bt_read(struct bt_descriptor *btDesc, unsigned char *buf, int len)
{
	tty_insert_flip_string(&btDesc->tty_port, buf, len);
	tty_flip_buffer_push(&btDesc->tty_port);

	wake_up(&btDesc->ipc.wait_q);
}

static
int bt_write(struct tty_struct *tty, const unsigned char *buf, int len)
{
	int ret = 0;
	struct bt_descriptor *btDesc = container_of(tty->port,
						struct bt_descriptor, tty_port);
	struct device *dev = &btDesc->pdev->dev;

	if (btDesc->sendmsg_cb) {
		ret = btDesc->sendmsg_cb(btDesc, (unsigned char *)buf, len);
		if (ret < 0 && ret != EAGAIN)
			dev_err(dev, "failed to send msg, ret = %d\n", ret);
	}

	return len;
}

static int bt_write_room(struct tty_struct *tty)
{
	return 2048;
}

static
void bt_set_termios(struct tty_struct *tty, struct ktermios *old_termios)
{
}

static int bt_tiocmget(struct tty_struct *tty)
{
	return 0;
}

static
int bt_tiocmset(struct tty_struct *tty, unsigned int set, unsigned int clear)
{
	return 0;
}

static
int bt_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct bt_descriptor *btDesc = container_of(tty->port,
						struct bt_descriptor, tty_port);
	struct device *dev = &btDesc->pdev->dev;
	struct platform_device *rproc_pdev = btDesc->rproc_pdev;
	struct rproc *rproc = platform_get_drvdata(rproc_pdev);

	switch (cmd) {
	case IOCTL_IPC_BOOT:
		if (arg) {
			ret = rproc_boot(rproc);
			if (ret)
				dev_err(dev, "m0 boot fail, ret = %d\n", ret);
			else
				pid_distinct(btDesc, ADD);
		} else {
			pid_distinct(btDesc, tty->closing ? TERMINATE : REMOVE);
			rproc_shutdown(rproc);
		}
		break;
	default:
		ret = -ENOIOCTLCMD;
	}

	return ret;
}

void bt_throttle(struct tty_struct *tty)
{
	struct bt_descriptor *btDesc = container_of(tty->port,
						struct bt_descriptor, tty_port);

	disable_irq_nosync(btDesc->ipc.irq);
}

void bt_unthrottle(struct tty_struct *tty)
{
	struct bt_descriptor *btDesc = container_of(tty->port,
						struct bt_descriptor, tty_port);

	wake_up(&btDesc->ipc.wait_q);
	enable_irq(btDesc->ipc.irq);
}

static int bt_open(struct tty_struct *tty, struct file *file)
{
	int ret = 0;

	tty->closing = 0;
	ret = bt_ioctl(tty, IOCTL_IPC_BOOT, 1);
	return ret;
}

static void bt_close(struct tty_struct *tty, struct file *file)
{
	struct bt_descriptor *btDesc = container_of(tty->port,
						struct bt_descriptor, tty_port);

	tty->closing = 1;
	bt_ioctl(tty, IOCTL_IPC_BOOT, 0);
	pid_show(btDesc);
}

static int bt_tty_activate(struct tty_port *port, struct tty_struct *tty)
{
	return 0;
}

static void bt_tty_shutdown(struct tty_port *port)
{
}

static const struct tty_port_operations bt_port_ops = {
	.activate = bt_tty_activate,
	.shutdown = bt_tty_shutdown
};

static const struct tty_operations bt_ops = {
	.open		= bt_open,
	.close		= bt_close,
	.write		= bt_write,
	.write_room	= bt_write_room,
	.set_termios	= bt_set_termios,
	.tiocmget	= bt_tiocmget,
	.tiocmset	= bt_tiocmset,
	.ioctl		= bt_ioctl,
	.throttle	= bt_throttle,
	.unthrottle	= bt_unthrottle,
};

int bt_tty_init(struct bt_descriptor *btDesc)
{
	int ret;
	struct device *btdev = &btDesc->pdev->dev;
	struct device *dev;

	btDesc->tty_drv = tty_alloc_driver(1,
				TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV);
	if (!btDesc->tty_drv) {
		ret = PTR_ERR(btDesc->tty_drv);
		dev_err(btdev, "failed to alloc driver, ret %d\n", ret);
		goto err;
	}

	btDesc->tty_drv->driver_name = "bt_tty";
	btDesc->tty_drv->name = "ttyBT";
	btDesc->tty_drv->major = 0;
	btDesc->tty_drv->minor_start = 0;
	btDesc->tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
	btDesc->tty_drv->subtype = SERIAL_TYPE_NORMAL;

	btDesc->tty_drv->init_termios = tty_std_termios;
	btDesc->tty_drv->init_termios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK
						| ISTRIP | INLCR | IGNCR | ICRNL
						| IXON);

	btDesc->tty_drv->init_termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG
						| IEXTEN);

	btDesc->tty_drv->init_termios.c_cflag &= ~(CSIZE | PARENB | CBAUD);
	btDesc->tty_drv->init_termios.c_oflag &= ~(OPOST);
	btDesc->tty_drv->init_termios.c_cflag |= CS8;

	btDesc->tty_drv->init_termios.c_cc[VMIN] = 0;
	btDesc->tty_drv->init_termios.c_cc[VTIME] = 255;
	btDesc->tty_drv->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;

	tty_set_operations(btDesc->tty_drv, &bt_ops);

	ret = tty_register_driver(btDesc->tty_drv);
	if (ret < 0) {
		dev_err(btdev, "failed to register driver, ret %d\n", ret);
		goto err_tty_put;
	}

	tty_port_init(&btDesc->tty_port);
	btDesc->tty_port.ops = &bt_port_ops;

	if (btDesc->is_serdev)
		dev = tty_port_register_device_serdev(&btDesc->tty_port,
						      btDesc->tty_drv, 0, btdev);
	else
		dev = tty_port_register_device(&btDesc->tty_port,
					       btDesc->tty_drv, 0, NULL);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		dev_err(btdev, "failed to register port, ret %d\n", ret);
		goto err_tty_unreg;
	}

	return 0;

err_tty_unreg:
	tty_unregister_driver(btDesc->tty_drv);
err_tty_put:
	put_tty_driver(btDesc->tty_drv);
err:
	return ret;
}

void bt_tty_deinit(struct bt_descriptor *btDesc)
{
	struct tty_driver *tty_drv = btDesc->tty_drv;

	tty_unregister_device(tty_drv, 0);
	tty_unregister_driver(tty_drv);
	put_tty_driver(tty_drv);
}

int bt_parse_ipc(struct bt_descriptor *btDesc)
{
	int ret;
	struct device_node *np;
	const char *key = "qcom,ipc";
	struct bt_ipc *ipc = &btDesc->ipc;
	struct device *dev = &btDesc->pdev->dev;

	np = of_parse_phandle(dev->of_node, key, 0);
	if (!np) {
		dev_err(dev, "no qcom,ipc node\n");
		return -ENODEV;
	}

	ipc->regmap = syscon_node_to_regmap(np);
	if (IS_ERR(ipc->regmap))
		return PTR_ERR(btDesc->ipc.regmap);

	ret = of_property_read_u32_index(dev->of_node, key, 1, &ipc->offset);
	if (ret < 0) {
		dev_err(dev, "no offset in %s, ret = %d\n", key, ret);
		return ret;
	}

	ret = of_property_read_u32_index(dev->of_node, key, 2, &ipc->bit);
	if (ret < 0) {
		dev_err(dev, "no bit in %s, ret = %d\n", key, ret);
		return ret;
	}

	ipc->irq = platform_get_irq(btDesc->pdev, 0);
	if (ipc->irq < 0) {
		dev_err(dev, "err getting IRQ ret = %d\n", ipc->irq);
		return ipc->irq;
	}

	dev_err(dev, "%s\n", __func__);

	return 0;
}

int bt_parse_mem(struct bt_descriptor *btDesc)
{
	struct resource *res;
	struct device_node *np;
	struct device *dev = &btDesc->pdev->dev;
	struct reserved_mem *rmem = btDesc->rmem;
	struct bt_mem *btmem = &btDesc->btmem;

	res = platform_get_resource_byname(btDesc->pdev, IORESOURCE_MEM,
								"bt_warm_rst");
	btDesc->warm_reset = devm_ioremap_resource(dev, res);
	if (IS_ERR(btDesc->warm_reset)) {
		dev_err(dev, "no reset defined\n");
		return PTR_ERR(btDesc->warm_reset);
	}

	np = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (!np) {
		dev_err(dev, "no memory-region node\n");
		return -ENODEV;
	}

	rmem = of_reserved_mem_lookup(np);
	if (!rmem) {
		dev_err(dev, "unable to acquire memory-region\n");
		return -EINVAL;
	}

	btmem->phys = rmem->base;
	btmem->reloc = rmem->base;
	btmem->size = rmem->size;
	btmem->virt = devm_ioremap_wc(dev, btmem->phys, btmem->size);
	if (!btmem->virt) {
		dev_err(dev, "unable to map memory region: %pa+%pa\n",
				&rmem->base, &rmem->size);
		return -EBUSY;
	}

	dev_err(dev, "%s\n", __func__);

	return 0;
}

int bt_parse_clks(struct bt_descriptor *btDesc)
{
	struct device *dev = &btDesc->pdev->dev;

	btDesc->btss_reset = devm_reset_control_get(dev, "btss_reset");
	if (IS_ERR_OR_NULL(btDesc->btss_reset)) {
		dev_err(dev, "unable to acquire btss_reset\n");
		return PTR_ERR(btDesc->btss_reset);
	}

	btDesc->lpo_clk = devm_clk_get(dev, "lpo_clk");
	if (IS_ERR(btDesc->lpo_clk)) {
		dev_err(dev, "failed to get lpo_clk\n");
		return PTR_ERR(btDesc->lpo_clk);
	}

	return 0;
}

int bt_parse_pinctrl(struct bt_descriptor *btDesc)
{
	struct device *dev = &btDesc->pdev->dev;

	btDesc->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(btDesc->pinctrl)) {
		dev_err(dev, "unable to get pinctrl\n");
		return PTR_ERR(btDesc->pinctrl);
	}

	return 0;
}

int bt_parse_dt(struct bt_descriptor *btDesc)
{
	int ret;
	struct device_node *child;
	struct device *dev = &btDesc->pdev->dev;

	ret = of_property_read_string(dev->of_node, "firmware",
							&btDesc->fw_name);
	if (ret < 0) {
		dev_err(dev, "could not find firmware name, ret = %d\n", ret);
		return ret;
	}

	ret = bt_parse_ipc(btDesc);
	if (ret < 0) {
		dev_err(dev, "could not get IPC info, ret = %d\n", ret);
		return ret;
	}

	ret = bt_parse_mem(btDesc);
	if (ret < 0) {
		dev_err(dev, "could not get mem info, ret = %d\n", ret);
		return ret;
	}

	ret = bt_parse_clks(btDesc);
	if (ret < 0) {
		dev_err(dev, "could not get clk info, ret = %d\n", ret);
		return ret;
	}

	if (btss_debug) {
		ret = bt_parse_pinctrl(btDesc);
		if (ret < 0) {
			dev_err(dev, "could not get pinctrl info, ret = %d\n",
									ret);
			return ret;
		}
	}

	btDesc->is_serdev = false;
	for_each_available_child_of_node(dev->of_node, child){
		if (of_device_is_compatible(child, "qcom,maple-bt")){
			dev_info(dev, "%s : serdev node enabled for bluez\n", __func__);
			btDesc->is_serdev = true;
			break;
		}
	}

	btDesc->nosecure = of_property_read_bool(dev->of_node, "qcom,nosecure");

	dev_info(dev, "%s operating in %s mode\n", __func__,
				btDesc->nosecure ? "non secure" : "secure");

	return 0;
}

#if defined(CONFIG_DEBUG_FS)
static int bt_dbgfs_open(struct inode *ip, struct file *fp)
{
	return 0;
}

static ssize_t bt_dbgfs_read(struct file *fp, char __user *buf,
		size_t count, loff_t *pos)
{
	return count;
}

static int bt_dbgfs_release(struct inode *ip, struct file *fp)
{
	return 0;
}


static ssize_t bt_dbgfs_write(struct file *fp, const char __user *buf,
		size_t count, loff_t *pos)
{
	return count;
}

static const struct file_operations bt_dbgfs_ops = {
	.owner = THIS_MODULE,
	.open = bt_dbgfs_open,
	.read = bt_dbgfs_read,
	.write = bt_dbgfs_write,
	.release = bt_dbgfs_release,
};

static int bt_debugfs_init(struct bt_descriptor *btDesc)
{
	void *pret;
	int index;
	struct device *dev = &btDesc->pdev->dev;
	static const char * const dirs[] = {"status", "to_console"};

	btDesc->dbgfs = debugfs_create_dir(btDesc->pdev->name, NULL);
	if (IS_ERR(btDesc->dbgfs)) {
		dev_err(dev, "debugfs creation failed %s, ret = %ld\n",
				btDesc->pdev->name, PTR_ERR(btDesc->dbgfs));
		return PTR_ERR(btDesc->dbgfs);
	}

	for (index = 0; index < ARRAY_SIZE(dirs); index++) {
		pret = debugfs_create_file(dirs[index], S_IRUSR | S_IWUSR,
					btDesc->dbgfs, NULL, &bt_dbgfs_ops);
		if (IS_ERR(pret)) {
			dev_err(dev, "debugfs creation failed %s, ret = %ld\n",
					dirs[index], PTR_ERR(pret));
			debugfs_remove_recursive(btDesc->dbgfs);
			return PTR_ERR(pret);
		}
	}

	return 0;
}

static void bt_debugfs_deinit(struct bt_descriptor *btDesc)
{
	debugfs_remove_recursive(btDesc->dbgfs);
}
#else
static inline int bt_debugfs_init(struct bt_descriptor *btDesc)
{
	return 0;
}

static inline void bt_debugfs_deinit(struct bt_descriptor *btDesc)
{
}
#endif

static int bt_probe(struct platform_device *pdev)
{
	int ret;
	struct bt_descriptor *btDesc;
	struct pinctrl_state *pin_state;

	btDesc = devm_kzalloc(&pdev->dev, sizeof(*btDesc), GFP_KERNEL);
	if (!btDesc)
		return -ENOMEM;

	btDesc->pdev = pdev;

	ret = bt_parse_dt(btDesc);
	if (ret < 0) {
		dev_err(&pdev->dev, "err parsing DT, ret = %d\n", ret);
		goto err;
	}

	ret = bt_debugfs_init(btDesc);
	if (ret < 0)
		dev_err(&pdev->dev, "err register debugFs, ret = %d\n", ret);

	if (!btss_debug) {
		ret = bt_tty_init(btDesc);
		if (ret < 0) {
			dev_err(&pdev->dev,
				"err initializing TTY, ret = %d\n", ret);
			goto err;
		}

		btDesc->recvmsg_cb = bt_read;
	} else {
		btDesc->debug_en = true;

		pin_state = pinctrl_lookup_state(btDesc->pinctrl, "btss_pins");
		if (IS_ERR(pin_state)) {
			ret = PTR_ERR(pin_state);
			dev_err(&pdev->dev,
				"btss pinctrl state err, ret = %d\n", ret);
			goto err;
		}
		pinctrl_select_state(btDesc->pinctrl, pin_state);

	}

	btDesc->rproc_pdev = platform_device_register_data(&pdev->dev,
							"bt_rproc_driver",
							pdev->id, &btDesc,
							sizeof(btDesc));
	if (IS_ERR(btDesc->rproc_pdev)) {
		ret = PTR_ERR(btDesc->rproc_pdev);
		dev_err(&pdev->dev, "err registering rproc, ret = %d\n", ret);
		goto err_deinit_tty;
	}

	INIT_LIST_HEAD(&btDesc->pid_q);

	platform_set_drvdata(pdev, btDesc);

	return 0;

err_deinit_tty:
	bt_tty_deinit(btDesc);
err:
	return ret;
}

static int bt_remove(struct platform_device *pdev)
{
	struct bt_descriptor *btDesc = platform_get_drvdata(pdev);

	bt_tty_deinit(btDesc);
	bt_debugfs_deinit(btDesc);

	return 0;
}

static const struct of_device_id bt_of_match[] = {
	{.compatible    = "qcom,bt"},
	{}
};
MODULE_DEVICE_TABLE(of, bt_of_match);

static struct platform_driver bt_driver = {
	.probe  = bt_probe,
	.remove = bt_remove,
	.driver = {
		.name   = "bt_driver",
		.owner  = THIS_MODULE,
		.of_match_table = bt_of_match,
	},
};

static int __init bt_init(void)
{
	int ret;

	ret = platform_driver_register(&bt_driver);
	if (ret)
		pr_err("%s: plat_driver registeration  failed\n", __func__);

	return ret;
}

static void __exit bt_exit(void)
{
	platform_driver_unregister(&bt_driver);
}

module_init(bt_init);
module_exit(bt_exit);

MODULE_DESCRIPTION("QTI Technologies, Inc.");
MODULE_LICENSE("GPL v2");
