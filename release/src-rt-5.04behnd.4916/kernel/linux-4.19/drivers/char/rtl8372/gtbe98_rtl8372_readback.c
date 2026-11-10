// SPDX-License-Identifier: GPL-2.0
/* Read-only userspace interface; the prebuilt retains its own module init. */
#include <linux/build_bug.h>
#include <linux/capability.h>
#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include "gtbe98_readback.h"

static long gtbe98_readback_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	void __user *user = (void __user *)arg;
	struct gtbe98_rtl8372_identity identity = {
		.abi_version = GTBE98_RTL8372_ABI_VERSION,
		.size = sizeof(struct gtbe98_rtl8372_identity),
		.profile = GTBE98_RTL8372_PROFILE,
		.snapshot_size = sizeof(struct gtbe98_rtl8372_snapshot),
		.vlan_count = GTBE98_RTL8372_VLAN_COUNT,
		.port_mask = GTBE98_RTL8372_PORT_MASK,
	};
	struct gtbe98_rtl8372_capture request;
	struct gtbe98_rtl8372_snapshot *snapshot;
	void __user *output;
	int ret;

	/* Recheck on every call, including descriptors inherited from root. */
	if (!capable(CAP_NET_ADMIN))
		return -EPERM;
	if (cmd == GTBE98_RTL8372_IOC_IDENTITY) {
		memcpy(identity.prebuilt_sha256, GTBE98_RTL8372_PREBUILT_SHA256,
		       sizeof(identity.prebuilt_sha256));
		return copy_to_user(user, &identity, sizeof(identity)) ? -EFAULT : 0;
	}
	if (cmd != GTBE98_RTL8372_IOC_CAPTURE)
		return -ENOTTY;
	if (copy_from_user(&request, user, sizeof(request)))
		return -EFAULT;
	if (request.abi_version != GTBE98_RTL8372_ABI_VERSION ||
	    request.size != sizeof(request) || request.reserved ||
	    request.snapshot_size != sizeof(*snapshot))
		return -EINVAL;
#ifdef CONFIG_COMPAT
	if (is_compat_task() && request.snapshot >> 32)
		return -EFAULT;
#endif
	output = u64_to_user_ptr(request.snapshot);
	if (!request.snapshot || !access_ok(VERIFY_WRITE, output, sizeof(*snapshot)))
		return -EFAULT;
	snapshot = vzalloc(sizeof(*snapshot));
	if (!snapshot)
		return -ENOMEM;
	ret = gtbe98_rtl8372_capture_snapshot(snapshot);
	/* Preserve SDK error/stage/index on failure, complete always stays zero. */
	if (copy_to_user(output, snapshot, sizeof(*snapshot)))
		ret = -EFAULT;
	vfree(snapshot);
	return ret;
}

#ifdef CONFIG_COMPAT
static long gtbe98_readback_compat_ioctl(struct file *file, unsigned int cmd,
					unsigned long arg)
{
	return gtbe98_readback_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations gtbe98_readback_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = gtbe98_readback_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gtbe98_readback_compat_ioctl,
#endif
	.llseek = no_llseek,
};

static struct miscdevice gtbe98_readback_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gtbe98_rtl8372",
	.fops = &gtbe98_readback_fops,
	.mode = 0600,
};

static int __init gtbe98_readback_init(void)
{
	BUILD_BUG_ON(sizeof(struct gtbe98_rtl8372_capture) != 24);
	BUILD_BUG_ON(offsetof(struct gtbe98_rtl8372_capture, snapshot) != 8);
	BUILD_BUG_ON(sizeof(struct gtbe98_rtl8372_identity) != 88);
	BUILD_BUG_ON(sizeof(struct gtbe98_rtl8372_snapshot) != 65728);
	return misc_register(&gtbe98_readback_device);
}

static void __exit gtbe98_readback_exit(void)
{
	misc_deregister(&gtbe98_readback_device);
}

module_init(gtbe98_readback_init);
module_exit(gtbe98_readback_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GT-BE98 pinned RTL8372 VLAN configuration readback");
