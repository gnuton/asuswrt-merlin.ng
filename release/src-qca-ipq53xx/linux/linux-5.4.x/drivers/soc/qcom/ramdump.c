/* Copyright (c) 2011-2015, The Linux Foundation. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/elf.h>
#include <linux/wait.h>
#include <soc/qcom/ramdump.h>
#include <linux/dma-mapping.h>

#define RAMDUMP_WAIT_MSECS	600000
#define MAX_DEVICE 5

struct ramdump_device {
	char name[256];

	unsigned int data_ready;
	unsigned int consumer_present;
	int ramdump_status;
	int index;

	struct completion ramdump_complete;
	wait_queue_head_t dump_wait_q;
	int nsegments;
	struct ramdump_segment *segments;
	size_t elfcore_size;
	char *elfcore_buf;
};

DEFINE_MUTEX(g_dump_class_lock);
static struct class *dump_class;
static int dump_major;
static int g_class_refcnt;

static struct ramdump_device *g_rd_dev[MAX_DEVICE];
DEFINE_SPINLOCK(g_rd_dev_lock);

static int set_g_rd_dev(struct ramdump_device *rd_dev)
{
	int ret = 0;
	int index;

	spin_lock(&g_rd_dev_lock);
	for (index = 0; index < MAX_DEVICE; index++) {
		if (!g_rd_dev[index]) {
			g_rd_dev[index] = rd_dev;
			ret = index;
			break;
		}
	}
	spin_unlock(&g_rd_dev_lock);

	if (index == MAX_DEVICE) {
		pr_err("ERR: Can't save the rd_dev handle\n");
		dump_stack();
		ret = -EINVAL;
	}

	return ret;
}

static void unset_g_rd_dev(struct ramdump_device *rd_dev, int index)
{

	if (index < 0 ||  index >= MAX_DEVICE) {
		pr_err("ERR: Can't save the rd_dev handle\n");
		dump_stack();
		return;
	}
	spin_lock(&g_rd_dev_lock);
	if (g_rd_dev[index] == rd_dev)
		g_rd_dev[index] = NULL;
	spin_unlock(&g_rd_dev_lock);
}

static void destroy_ramdump_device_file(struct ramdump_device *rd_dev,
					int major, int minor)
{
	rd_dev->consumer_present = 0;
	rd_dev->data_ready = 0;
	unset_g_rd_dev(rd_dev, rd_dev->index);
	device_destroy(dump_class, MKDEV(major, minor));
	mutex_lock(&g_dump_class_lock);

	g_class_refcnt--;
	if (!g_class_refcnt) {
		class_destroy(dump_class);
		dump_class = NULL;
		unregister_chrdev(dump_major, "dump_q6v5");
		dump_major = 0;
	}

	mutex_unlock(&g_dump_class_lock);
}

static int ramdump_open(struct inode *inode, struct file *filep)
{
	int minor =  iminor(inode);
	struct ramdump_device *rd_dev;

	if (minor >= MAX_DEVICE) {
		pr_err("ERR: minor value greater than %d\n", MAX_DEVICE);
		return -EINVAL;
	}

	rd_dev = g_rd_dev[minor];
	if (rd_dev == NULL) {
		pr_err("ramdump_open error: rd_dev is NULL\n");
		return -EINVAL;
	}

	rd_dev->consumer_present = 1;
	rd_dev->ramdump_status = 0;
	filep->private_data = rd_dev;
	return 0;
}

static int ramdump_release(struct inode *inode, struct file *filep)
{
	struct ramdump_device *rd_dev = container_of(filep->private_data,
				struct ramdump_device, name);
	int dump_minor =  iminor(inode);
	int dump_major = imajor(inode);

	if (dump_minor >= MAX_DEVICE) {
		pr_err("ERR: minor value greater than %d\n", MAX_DEVICE);
		return -EINVAL;
	}

	destroy_ramdump_device_file(rd_dev, dump_major, dump_minor);
	complete(&rd_dev->ramdump_complete);
	return 0;
}

static unsigned long offset_translate(loff_t user_offset,
		struct ramdump_device *rd_dev, unsigned long *data_left,
		void **vaddr)
{
	int i = 0;
	*vaddr = NULL;

	for (i = 0; i < rd_dev->nsegments; i++)
		if (user_offset >= rd_dev->segments[i].size)
			user_offset -= rd_dev->segments[i].size;
		else
			break;

	if (i == rd_dev->nsegments) {
		pr_debug("Ramdump(%s): offset_translate returning zero\n",
				rd_dev->name);
		*data_left = 0;
		return 0;
	}

	*data_left = rd_dev->segments[i].size - user_offset;

	pr_debug("Ramdump(%s): Returning address: %llx, data_left = %ld\n",
		rd_dev->name, rd_dev->segments[i].address + user_offset,
		*data_left);

	if (rd_dev->segments[i].v_address)
		*vaddr = rd_dev->segments[i].v_address + user_offset;

	return rd_dev->segments[i].address + user_offset;
}

#define MAX_IOREMAP_SIZE SZ_1M

static ssize_t ramdump_read(struct file *filep, char __user *buf, size_t count,
			loff_t *pos)
{
	struct ramdump_device *rd_dev = container_of(filep->private_data,
				struct ramdump_device, name);
	void *device_mem = NULL, *origdevice_mem = NULL, *vaddr = NULL;
	unsigned long data_left = 0, bytes_before, bytes_after;
	unsigned long addr = 0;
	size_t copy_size = 0, alignsize;
	unsigned char *alignbuf = NULL, *finalbuf = NULL;
	int ret = 0;
	loff_t orig_pos = *pos;

	if ((filep->f_flags & O_NONBLOCK) && !rd_dev->data_ready)
		return -EAGAIN;

	ret = wait_event_interruptible(rd_dev->dump_wait_q, rd_dev->data_ready);
	if (ret)
		return ret;

	if (*pos < rd_dev->elfcore_size) {
		copy_size = rd_dev->elfcore_size - *pos;
		copy_size = min(copy_size, count);

		if (copy_to_user(buf, rd_dev->elfcore_buf + *pos, copy_size)) {
			ret = -EFAULT;
			goto ramdump_done;
		}
		*pos += copy_size;
		count -= copy_size;
		buf += copy_size;
		if (count == 0)
			return copy_size;
	}

	addr = offset_translate(*pos - rd_dev->elfcore_size, rd_dev,
				&data_left, &vaddr);

	/* EOF check */
	if (data_left == 0) {
		pr_info("Ramdump(%s): Ramdump complete. %lld bytes read.",
			rd_dev->name, *pos);
		rd_dev->ramdump_status = 0;
		return 0;
	}

	copy_size = min(count, (size_t)MAX_IOREMAP_SIZE);
	copy_size = min((unsigned long)copy_size, data_left);

	device_mem = vaddr;
	origdevice_mem = device_mem;

	if (device_mem == NULL) {
		pr_err("Ramdump(%s): Unable to ioremap: addr %lx, size %zd\n",
			rd_dev->name, addr, copy_size);
		rd_dev->ramdump_status = -1;
		ret = -ENOMEM;
		goto ramdump_done;
	}

	alignbuf = kzalloc(copy_size, GFP_KERNEL);
	if (!alignbuf) {
		pr_err("Ramdump(%s): Unable to alloc mem for aligned buf\n",
				rd_dev->name);
		rd_dev->ramdump_status = -1;
		ret = -ENOMEM;
		goto ramdump_done;
	}

	finalbuf = alignbuf;
	alignsize = copy_size;

	if ((unsigned long)device_mem & 0x7) {
		bytes_before = 8 - ((unsigned long)device_mem & 0x7);
		memcpy_fromio(alignbuf, device_mem, bytes_before);
		device_mem += bytes_before;
		alignbuf += bytes_before;
		alignsize -= bytes_before;
	}

	if (alignsize & 0x7) {
		bytes_after = alignsize & 0x7;
		memcpy(alignbuf, device_mem, alignsize - bytes_after);
		device_mem += alignsize - bytes_after;
		alignbuf += (alignsize - bytes_after);
		alignsize = bytes_after;
		memcpy_fromio(alignbuf, device_mem, alignsize);
	} else
		memcpy(alignbuf, device_mem, alignsize);

	if (copy_to_user(buf, finalbuf, copy_size)) {
		pr_err("Ramdump(%s): Couldn't copy all data to user.",
			rd_dev->name);
		rd_dev->ramdump_status = -1;
		ret = -EFAULT;
		goto ramdump_done;
	}

	kfree(finalbuf);

	*pos += copy_size;

	pr_debug("Ramdump(%s): Read %zd bytes from address %lx.",
			rd_dev->name, copy_size, addr);

	return *pos - orig_pos;

ramdump_done:
	kfree(finalbuf);
	rd_dev->data_ready = 0;
	*pos = 0;
	return ret;
}

static unsigned int ramdump_poll(struct file *filep,
					struct poll_table_struct *wait)
{
	struct ramdump_device *rd_dev = container_of(filep->private_data,
				struct ramdump_device, name);
	unsigned int mask = 0;

	if (rd_dev->data_ready)
		mask |= (POLLIN | POLLRDNORM);

	poll_wait(filep, &rd_dev->dump_wait_q, wait);
	return mask;
}

static const struct file_operations ramdump_file_ops = {
	.open = ramdump_open,
	.release = ramdump_release,
	.read = ramdump_read,
	.poll = ramdump_poll
};

void *create_ramdump_device(const char *dev_name, struct device *parent)
{
	struct ramdump_device *rd_dev;

	if (!dev_name) {
		pr_err("%s: Invalid device name.\n", __func__);
		return NULL;
	}

	rd_dev = kzalloc(sizeof(struct ramdump_device), GFP_KERNEL);
	if (!rd_dev) {
		pr_err("%s: Couldn't alloc space for ramdump device!",
			__func__);
		return NULL;
	}
	rd_dev->index  = set_g_rd_dev(rd_dev);
	snprintf(rd_dev->name, ARRAY_SIZE(rd_dev->name), "%s",
		 dev_name);

	init_completion(&rd_dev->ramdump_complete);

	init_waitqueue_head(&rd_dev->dump_wait_q);

	return (void *)rd_dev;
}
EXPORT_SYMBOL(create_ramdump_device);

int create_ramdump_device_file(void *handle)
{
	struct ramdump_device *rd_dev = (struct ramdump_device *)handle;
	struct device *dump_dev;
	int ret = 0;

	if (!rd_dev) {
		pr_err("%s: Couldn't alloc space for ramdump device!",
			__func__);
		return -EINVAL;
	}

	mutex_lock(&g_dump_class_lock);
	if (!dump_class) {
		/* Create once and reuse */
		dump_major = register_chrdev(UNNAMED_MAJOR, "dump_q6v5",
					     &ramdump_file_ops);
		if (dump_major < 0) {
			pr_err("Unable to allocate a major number err = %d",
					dump_major);
			mutex_unlock(&g_dump_class_lock);
			return dump_major;
		}

		dump_class = class_create(THIS_MODULE, "dump_q6v5");
		if (IS_ERR(dump_class)) {
			pr_err("Unable to create a dump_class");
			ret = PTR_ERR(dump_class);
			unregister_chrdev(dump_major, "dump_q6v5");
			dump_major = 0;
			mutex_unlock(&g_dump_class_lock);
			goto class_failed;
		}
	}
	mutex_unlock(&g_dump_class_lock);

	dump_dev = device_create(dump_class, NULL,
				 MKDEV(dump_major, rd_dev->index), rd_dev,
				 rd_dev->name);
	if (IS_ERR(dump_dev)) {
		pr_err("Unable to create a device");
		ret = PTR_ERR(dump_dev);
		goto device_failed;
	}

	mutex_lock(&g_dump_class_lock);
	g_class_refcnt++;
	mutex_unlock(&g_dump_class_lock);

	return ret;

device_failed:
	mutex_lock(&g_dump_class_lock);
	if (!g_class_refcnt) {
		class_destroy(dump_class);
		dump_class = NULL;
		unregister_chrdev(dump_major, "dump_q6v5");
		dump_major = 0;
	}
	mutex_unlock(&g_dump_class_lock);
class_failed:
	return ret;
}
EXPORT_SYMBOL(create_ramdump_device_file);

void destroy_ramdump_device(void *dev)
{
	struct ramdump_device *rd_dev = dev;

	if (IS_ERR_OR_NULL(rd_dev))
		return;
	unset_g_rd_dev(rd_dev, rd_dev->index);
	kfree(rd_dev);
}
EXPORT_SYMBOL(destroy_ramdump_device);

static int _do_ramdump(void *handle, struct ramdump_segment *segments,
		int nsegments, bool use_elf)
{
	int ret, i;
	struct ramdump_device *rd_dev = (struct ramdump_device *)handle;
	Elf32_Phdr *phdr;
	Elf32_Ehdr *ehdr;
	unsigned long offset;


	for (i = 0; i < nsegments; i++)
		segments[i].size = PAGE_ALIGN(segments[i].size);

	rd_dev->segments = segments;
	rd_dev->nsegments = nsegments;

	if (use_elf) {
		rd_dev->elfcore_size = sizeof(*ehdr) +
				       sizeof(*phdr) * nsegments;
		ehdr = kzalloc(rd_dev->elfcore_size, GFP_KERNEL);
		rd_dev->elfcore_buf = (char *)ehdr;
		if (!rd_dev->elfcore_buf)
			return -ENOMEM;

		memcpy(ehdr->e_ident, ELFMAG, SELFMAG);
		ehdr->e_ident[EI_CLASS] = ELFCLASS32;
#ifdef CONFIG_CPU_BIG_ENDIAN
		ehdr->e_ident[EI_DATA] = ELFDATA2MSB;
#else
		ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
#endif
		ehdr->e_ident[EI_VERSION] = EV_CURRENT;
		ehdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
		ehdr->e_type = ET_CORE;
		ehdr->e_version = EV_CURRENT;
		ehdr->e_phoff = sizeof(*ehdr);
		ehdr->e_ehsize = sizeof(*ehdr);
		ehdr->e_phentsize = sizeof(*phdr);
		ehdr->e_phnum = nsegments;

		offset = rd_dev->elfcore_size;
		phdr = (Elf32_Phdr *)(ehdr + 1);
		for (i = 0; i < nsegments; i++, phdr++) {
			phdr->p_type = PT_LOAD;
			phdr->p_offset = offset;
			phdr->p_paddr = segments[i].address;
			if (segments[i].vaddr)
				phdr->p_vaddr = (uintptr_t)segments[i].vaddr;
			else
				phdr->p_vaddr = segments[i].address;

			phdr->p_filesz = phdr->p_memsz = segments[i].size;
			phdr->p_flags = PF_R | PF_W | PF_X;
			offset += phdr->p_filesz;
		}
	}

	rd_dev->data_ready = 1;
	rd_dev->ramdump_status = -1;

	reinit_completion(&rd_dev->ramdump_complete);

	/* Tell userspace that the data is ready */
	wake_up(&rd_dev->dump_wait_q);

	/* Wait (with a timeout) to let the ramdump complete */
	ret = wait_for_completion_timeout(&rd_dev->ramdump_complete,
			msecs_to_jiffies(RAMDUMP_WAIT_MSECS));

	if (!ret) {
		pr_err("Ramdump(%s): Timed out waiting for userspace.\n",
			rd_dev->name);
		destroy_ramdump_device_file(rd_dev, dump_major, rd_dev->index);
		ret = -EPIPE;
	} else
		ret = (rd_dev->ramdump_status == 0) ? 0 : -EPIPE;

	rd_dev->data_ready = 0;
	rd_dev->elfcore_size = 0;
	kfree(rd_dev->elfcore_buf);
	rd_dev->elfcore_buf = NULL;
	return ret;

}

int do_ramdump(void *handle, struct ramdump_segment *segments, int nsegments)
{
	return _do_ramdump(handle, segments, nsegments, false);
}
EXPORT_SYMBOL(do_ramdump);

int
do_elf_ramdump(void *handle, struct ramdump_segment *segments, int nsegments)
{
	return _do_ramdump(handle, segments, nsegments, true);
}
EXPORT_SYMBOL(do_elf_ramdump);
