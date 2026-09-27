// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
*/
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/qcom_scm.h>
#include <linux/utsname.h>
#include <linux/sizes.h>
#include <soc/qcom/ctx-save.h>
#include <linux/spinlock.h>
#include <linux/pfn.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/mhi.h>
#include <linux/sysrq.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <uapi/linux/major.h>
#include <linux/highmem.h>
#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/of_reserved_mem.h>

typedef struct ctx_save_tlv_msg {
	unsigned char *msg_buffer;
	unsigned char *cur_msg_buffer_pos;
	unsigned int len;
	spinlock_t spinlock;
	bool is_panic;
} ctx_save_tlv_msg_t;

#ifdef CONFIG_QCA_MINIDUMP
struct minidump_tlv_info {
	uint64_t start;
	uint64_t size;
};

/* Metadata List for bookkeeping and managing entries and invalidation of
* TLVs into the global crashdump buffer and the Metadata text file
*/
struct minidump_metadata_list {
	struct list_head list;	/*kernel's list structure*/
	unsigned long va;		/* Virtual address of TLV. Set to 0 if invalid*/
	unsigned long pa;       /*Physical address of TLV segment*/
	unsigned long modinfo_offset; /* Offset associated with the entry for
				* module information in Metadata text file
				*/
	unsigned long size; /*size associated with TLV entry */
	unsigned char *tlv_offset;	/* Offset associated with the TLV entry in
					* the crashdump buffer
					*/
	unsigned long mmuinfo_offset; /* Offset associated with the entry for
				* mmu information in MMU Metadata text file
				*/
    unsigned char type;
#ifdef CONFIG_QCA_MINIDUMP_DEBUG
	char *name;  /* Name associated with the TLV */
#endif
};
#endif /* CONFIG_QCA_MINIDUMP */

struct ctx_save_props {
	unsigned int tlv_msg_offset;
	unsigned int crashdump_page_size;
};

ctx_save_tlv_msg_t tlv_msg;

#ifdef CONFIG_QCA_MINIDUMP
struct minidump_metadata {
	char mod_log[METADATA_FILE_SZ];
	unsigned long mod_log_len;
	unsigned long cur_modinfo_offset;
	char mmu_log[MMU_FILE_SZ];
	unsigned long mmu_log_len;
	unsigned long cur_mmuinfo_offset;
};

struct minidump2mem_metadata {
	struct reserved_mem *rsvd_mem;
	struct delayed_work work;
	struct class *dev_class;
	struct mutex dev_lock;
	unsigned int max_dump_sz;
	int dev_major_no;
	unsigned char *rsvd_mem_ptr;
};

struct minidump_metadata_list metadata_list;
struct minidump_metadata minidump_meta_info;
struct minidump2mem_metadata dump2mem_info;

static const struct file_operations minidump2mem_ops;
static const struct file_operations mini_dump_ops;
static struct class *dump_class;
int dump_major = 0;

/* struct to store physical address and
 *  size of crashdump segments for live minidump
 *  capture
 *
 * @param:
 *   node - struct obj for kernel list
 *   addr - virtual address of dump segment
 *   size - size of dump segemnt
 */
struct dump_segment {
    struct list_head node;
    unsigned long addr;
    size_t size;
};

/* struct to store metadata info for
 *  preparing crashdump segemnts for
 *  live minidump capture.
 *
 * @param:
 *   total_size - total size of dumps
 *   num_seg - total number of dump segments
 *   flag - size of dump segment
 *   type - array to store 'type' of dump segments
 *   seg_size - array to store segment size of
 *   dump segments
 *   phy_addr - array to store physical address of
 *   dump segments
 */
struct mini_hdr {
    int total_size;
    int num_seg;
    int flag;
    unsigned char *type;
    int *seg_size;
    unsigned long *phy_addr;
};

/* struct to store device file info for /dev/minidump
 *
 *  @param:
 *   name - name of device node
 *   fops - struct obj for file operations
 *   fmode - file modes
 *   hdr - struct obj for metadata info
 *   dump_segments - struct obj for dump segment list
 */
struct dumpdev {
    const char *name;
    const struct file_operations *fops;
    fmode_t fmode;
    struct mini_hdr hdr;
    struct list_head dump_segments;
} minidump = {"minidump", &mini_dump_ops, FMODE_UNSIGNED_OFFSET | FMODE_EXCL};

#define MINIDUMP_IOCTL_MAGIC    'm'
#define MINIDUMP_IOCTL_PREPARE_HDR _IOR(MINIDUMP_IOCTL_MAGIC, 0, int)
#define MINIDUMP_IOCTL_PREPARE_SEG _IOR(MINIDUMP_IOCTL_MAGIC, 1, int)
#define MINIDUMP_IOCTL_PREPARE_TYP _IOR(MINIDUMP_IOCTL_MAGIC, 2, int)
#define MINIDUMP_IOCTL_PREPARE_PHY _IOR(MINIDUMP_IOCTL_MAGIC, 3, int)

#define REPLACE 1
#define APPEND 0
extern void minidump_get_pgd_info(uint64_t *pt_start, uint64_t *pt_len);
extern void minidump_get_linux_buf_info(uint64_t *plinux_buf, uint64_t *plinux_buf_len);
extern void minidump_get_log_buf_info(uint64_t *plog_buf, uint64_t *plog_buf_len);
extern struct list_head *minidump_modules;
char *minidump_module_list[MINIDUMP_MODULE_COUNT] = {"qca_ol", "wifi_3_0", "umac", "qdf"};
int minidump_dump_wlan_modules(void);
extern int log_buf_len;

#define MINIDUMP2MEM_CMN_HEADER_SIZE		(32)
#define MINIDUMP_MAGIC1_COOKIE			(0x4D494E49)	/* MINI */
#define MINIDUMP_MAGIC2_COOKIE			(0x44554D50)	/* DUMP */

static int minidump2mem_open(struct inode *inode, struct file *file)
{
	if (!mutex_trylock(&dump2mem_info.dev_lock))
		return -EBUSY;

	return 0;
}

static ssize_t minidump2mem_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	size_t copy_size = count;
	int ret;

	if (*ppos >= dump2mem_info.max_dump_sz)
		return 0;

	else if ((*ppos + count) > dump2mem_info.max_dump_sz)
		copy_size = (dump2mem_info.max_dump_sz - *ppos);

	ret = copy_to_user(buf, (dump2mem_info.rsvd_mem_ptr + *ppos),
			  copy_size);
	if (ret)
		pr_err("copy_to_user err, copied only: %d \n", ret);

	*ppos += (count - ret);
	return (count - ret);
}

static int minidump2mem_release(struct inode *inode, struct file *file)
{
	int dump_minor_dev = iminor(inode);
	int dump_major_dev = imajor(inode);

	device_destroy(dump2mem_info.dev_class,
			MKDEV(dump_major_dev, dump_minor_dev));
	class_destroy(dump2mem_info.dev_class);
	unregister_chrdev(dump_major_dev, "minidump2mem");

	/* clear dump2mem identifier in memory */
	memset(dump2mem_info.rsvd_mem_ptr, 0,
			MINIDUMP2MEM_CMN_HEADER_SIZE);
	memunmap(dump2mem_info.rsvd_mem_ptr);
	dump2mem_info.rsvd_mem_ptr = NULL;
	dump2mem_info.max_dump_sz = 0;
	dump2mem_info.dev_major_no = 0;
	dump2mem_info.dev_class = NULL;

	mutex_unlock(&dump2mem_info.dev_lock);
	return 0;
}

/* file ops for /dev/minidump2mem */
static const struct file_operations minidump2mem_ops = {
	.open       =   minidump2mem_open,
	.read       =   minidump2mem_read,
	.release    =   minidump2mem_release,
};

static void dump2mem_workfn(struct work_struct *work)
{
	struct device *dump2mem_dev;

	dump2mem_info.rsvd_mem_ptr = memremap(dump2mem_info.rsvd_mem->base,
			dump2mem_info.rsvd_mem->size, MEMREMAP_WB);
	if (!dump2mem_info.rsvd_mem_ptr) {
		pr_err("Unable to memremap rsvd region err\n");
		return;
	}

	if ((*((uint32_t*)&dump2mem_info.rsvd_mem_ptr[0])
			!= MINIDUMP_MAGIC1_COOKIE) ||
			(*((uint32_t*)&dump2mem_info.rsvd_mem_ptr[4])
			!= MINIDUMP_MAGIC2_COOKIE)) {
		pr_debug("Minidump: dump2mem identifier not present!\n");
		memunmap(dump2mem_info.rsvd_mem_ptr);
		dump2mem_info.rsvd_mem_ptr = NULL;
		return;
	}

	dump2mem_info.max_dump_sz =
		*((uint32_t*)&dump2mem_info.rsvd_mem_ptr[12]);
	dump2mem_info.max_dump_sz = ALIGN(dump2mem_info.max_dump_sz, 64);

	mutex_init(&dump2mem_info.dev_lock);

	dump2mem_info.dev_major_no = register_chrdev(UNNAMED_MAJOR,
			"minidump2mem", &minidump2mem_ops);
	if (dump2mem_info.dev_major_no < 0) {
		pr_err("Unable to allocate a major number err = %d \n",
				dump2mem_info.dev_major_no);
		return;
	}

	dump2mem_info.dev_class = class_create(THIS_MODULE,
			"minidump2mem");
	if (IS_ERR(dump2mem_info.dev_class)) {
		pr_err("Unable to create dump class = %ld\n",
				PTR_ERR(dump2mem_info.dev_class));
		return;
	}

	dump2mem_dev = device_create(dump2mem_info.dev_class, NULL,
			MKDEV(dump2mem_info.dev_major_no, 0), NULL,
			"minidump2mem");
	if (IS_ERR(dump2mem_dev)) {
		pr_err("Unable to create a device err = %ld\n",
				PTR_ERR(dump2mem_dev));
		return;
	}

	return;
}

/*
* Function: mini_dump_open
*
* Description: Traverse metadata list and store valid address
* size pairs in dump segment list for minidump device node. Also
* save useful metadata information of segment size, physical address
* and dump type per dump segment.
*
* Return: 0
*/
static int mini_dump_open(struct inode *inode, struct file *file) {
	struct minidump_metadata_list *cur_node;
	struct list_head *pos;
	unsigned long flags;
	struct dump_segment *segment = NULL;
	int index = 0;

	if (!tlv_msg.msg_buffer)
		return -ENOMEM;

	minidump.hdr.seg_size = (unsigned int *)
		kmalloc((sizeof(int) * minidump.hdr.num_seg), GFP_KERNEL);
	minidump.hdr.phy_addr =(unsigned long *)
		kmalloc((sizeof(unsigned long) *  minidump.hdr.num_seg), GFP_KERNEL);
	minidump.hdr.type = (unsigned char *)
		kmalloc((sizeof(unsigned char) * minidump.hdr.num_seg), GFP_KERNEL);

	if (!minidump.hdr.seg_size || !minidump.hdr.phy_addr ||
			!minidump.hdr.type)
		return -ENOMEM;

	INIT_LIST_HEAD(&minidump.dump_segments);
	spin_lock_irqsave(&tlv_msg.spinlock, flags);

	/* Traverse Metadata list and store valid address-size pairs in
	* dump segment list for /dev/minidump
	*/
	list_for_each(pos, &metadata_list.list) {
		cur_node = list_entry(pos, struct minidump_metadata_list, list);

		if (cur_node->va != INVALID) {
			segment = (struct dump_segment *)
				kmalloc(sizeof(struct dump_segment), GFP_ATOMIC);
			if (!segment) {
				pr_err("\nMinidump: Unable to allocate memory for dump segment");
				return -ENOMEM;
			}

			switch (cur_node->type) {
				case CTX_SAVE_LOG_DUMP_TYPE_DMESG:
					segment->size = log_buf_len;
					break;

				case CTX_SAVE_LOG_DUMP_TYPE_WLAN_MMU_INFO:
				case CTX_SAVE_LOG_DUMP_TYPE_WLAN_MOD_INFO:
					segment->size = *(unsigned long *)(uintptr_t)
						((unsigned long)__va(cur_node->size));
					break;

				default:
					segment->size = cur_node->size;
			}

			segment->addr = cur_node->va;
			list_add_tail(&(segment->node), &(minidump.dump_segments));
			minidump.hdr.total_size += segment->size;
			minidump.hdr.seg_size[index] = segment->size;
			minidump.hdr.phy_addr[index] = cur_node->pa;
			minidump.hdr.type[index] = cur_node->type;
			index++;
		}
	}
	spin_unlock_irqrestore(&tlv_msg.spinlock, flags);

	file->f_mode |= minidump.fmode;
	file->private_data = (void *)&minidump;

	return 0;
}

/*
* Function: mini_dump_release
*
* Description: Free resources for minidump device node
*
* Return: 0
*/
static int mini_dump_release(struct inode *inode, struct file *file)
{
	int dump_minor_dev = iminor(inode);
	int dump_major_dev = imajor(inode);

	struct dump_segment *segment, *tmp;

	struct dumpdev *dfp = (struct dumpdev *) file->private_data;

	list_for_each_entry_safe(segment, tmp, &dfp->dump_segments, node) {
		list_del(&segment->node);
		kfree(segment);
	}

	kfree(minidump.hdr.seg_size);
	kfree(minidump.hdr.phy_addr);
	kfree(minidump.hdr.type);

	device_destroy(dump_class, MKDEV(dump_major_dev, dump_minor_dev));
	class_destroy(dump_class);
	unregister_chrdev(dump_major_dev, "minidump");

	dump_major = 0;
	dump_class = NULL;

	return 0;
}

/*
* Function: mini_dump_read
*
* Description: Traverse dump segment list and copy dump segment
* content into user space buffer
*
* Return: 0
*/
static ssize_t mini_dump_read(struct file *file, char __user *buf,
    size_t count, loff_t *ppos)
{
	int ret = 0;
	int seg_num = 0;
	struct dumpdev *dfp = (struct dumpdev *) file->private_data;
	struct dump_segment *segment, *tmp;
	int copied = 0;

	list_for_each_entry_safe(segment, tmp, &dfp->dump_segments, node) {
		size_t pending = 0;
		seg_num ++;
		pending = segment->size;

		ret = copy_to_user(buf, (const void *)(uintptr_t)segment->addr, pending);
		if (ret) {
			pr_info("\n Minidump: copy_to_user error");
			return 0;
		}

		buf = buf + (pending - ret);
		copied = copied + (pending-ret);

		list_del(&segment->node);
		kfree(segment);
	}

	return copied;
}

/*
* Function: mini_dump_ioctl
*
* Description: Based on ioctl code, copy relevant metadata
* information to userspace buffer.
*
* Return: 0
*/
static long mini_dump_ioctl(struct file *file, unsigned int ioctl_num,
	unsigned long arg) {

	int ret = 0;
	struct dumpdev *dfp = (struct dumpdev *) file->private_data;

	switch (ioctl_num) {
		case MINIDUMP_IOCTL_PREPARE_HDR:
			ret = copy_to_user((void __user *)arg,
			(const void *)(uintptr_t)(&(dfp->hdr)), sizeof(dfp->hdr));
			break;
		case MINIDUMP_IOCTL_PREPARE_SEG:
			ret = copy_to_user((void __user *)arg,
			(const void *)(uintptr_t)((dfp->hdr.seg_size)),
			(sizeof(int) * minidump.hdr.num_seg));
			break;
		case MINIDUMP_IOCTL_PREPARE_TYP:
			ret = copy_to_user((void __user *)arg,
			(const void *)(uintptr_t)((dfp->hdr.type)),
			(sizeof(unsigned char) * minidump.hdr.num_seg));
			break;
		case MINIDUMP_IOCTL_PREPARE_PHY:
			ret = copy_to_user((void __user *)arg,
			(const void *)(uintptr_t)((dfp->hdr.phy_addr)),
			(sizeof(unsigned long) * minidump.hdr.num_seg));
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

/* file ops for /dev/minidump */
static const struct file_operations mini_dump_ops = {
	.open       =   mini_dump_open,
	.read       =   mini_dump_read,
	.unlocked_ioctl = mini_dump_ioctl,
	.release    =   mini_dump_release,
};
/*
* Function: do_minidump
*
* Description: Create and register minidump device node /dev/minidump
*
* @param: none
*
* Return: 0
*/
int do_minidump(void) {

    int ret = 0;
    struct device *dump_dev = NULL;

#ifdef CONFIG_QCA_MINIDUMP_DEBUG
    int count = 0;
    struct minidump_metadata_list *cur_node;
    struct list_head *pos;
    unsigned long flags;
#endif

    minidump.hdr.total_size = 0;
    if (!tlv_msg.msg_buffer) {
        pr_err("\n Minidump: Crashdump buffer is empty");
        return NOTIFY_OK;
    }

    /* Add subset of kernel module list to minidump metadata list */
    ret = minidump_dump_wlan_modules();
    if (ret)
        pr_err("Minidump: Error dumping modules: %d", ret);

#ifdef CONFIG_QCA_MINIDUMP_DEBUG
    pr_err("\n Minidump: Size of Metadata file = %ld", minidump_meta_info.mod_log_len);
    pr_err("\n Minidump: Printing out contents of Metadata list");

    spin_lock_irqsave(&tlv_msg.spinlock, flags);
    list_for_each(pos, &metadata_list.list) {
        count ++;
        cur_node = list_entry(pos, struct minidump_metadata_list, list);
 if (cur_node->va != 0) {
            if (cur_node->name != NULL)
                pr_info(" %s [%lx] ---> ", cur_node->name, cur_node->va);
            else
                pr_info(" un-named [%lx] ---> ", cur_node->va);
        }
    }
    spin_unlock_irqrestore(&tlv_msg.spinlock, flags);
    pr_err("\n Minidump: # nodes in the Metadata list = %d", count);
    pr_err("\n Minidump: Size of node in Metadata list = %ld\n",
    (unsigned long)sizeof(struct minidump_metadata_list));
#endif

    if (dump_class || dump_major) {
        device_destroy(dump_class, MKDEV(dump_major, 0));
        class_destroy(dump_class);
    }

    dump_major = register_chrdev(UNNAMED_MAJOR, "minidump", &mini_dump_ops);
    if (dump_major < 0) {
        ret = dump_major;
        pr_err("Unable to allocate a major number err = %d \n", ret);
        goto reg_failed;
    }

    dump_class = class_create(THIS_MODULE, "minidump");
    if (IS_ERR(dump_class)) {
        ret = PTR_ERR(dump_class);
        pr_err("Unable to create dump class = %d\n", ret);
        goto class_failed;
    }

    dump_dev = device_create(dump_class, NULL, MKDEV(dump_major, 0), NULL,
		    minidump.name);
    if (IS_ERR(dump_dev)) {
        ret = PTR_ERR(dump_dev);
        pr_err("Unable to create a device err = %d\n", ret);
        goto device_failed;
    }

    return ret;
device_failed:
    class_destroy(dump_class);
class_failed:
    unregister_chrdev(dump_major, "minidump");
reg_failed:
    return ret;

}
EXPORT_SYMBOL(do_minidump);

/*
* Function: sysrq_minidump_handler
*
* Description: Handler function for sysrq key event which
* is invoked on command line trigger 'echo y > /proc/sysrq-trigger'
*
* @param: key registered for sysrq event
*
* Return: 0
*/
static void sysrq_minidump_handler(int key)
{
    int ret =0;
    ret = do_minidump();
    if (ret)
        pr_info("\n Minidump: unable to init minidump dev node");

}

/* sysrq_key_op struct for registering operation
 * for a particular key.
 *
 * @param:
 * .hander - the key handler function
 * .help_msg - help_msg string
 * .action_msg - action_msg string
 */
static struct sysrq_key_op sysrq_minidump_op = {
    .handler    = sysrq_minidump_handler,
    .help_msg   = "minidump(y)",
    .action_msg = "MINIDUMP",
};
#endif /* CONFIG_QCA_MINIDUMP */

/*
* Function: ctx_save_replace_tlv
* Description: Adds dump segment as a TLV into the global crashdump
* buffer at specified offset.
*
* @param:	[in] type - Type associated with Dump segment
*		[in] size - Size associted with Dump segment
*		[in] data - Physical address of the Dump segment
*		[in] offset - offset at which TLV entry is added to the crashdump
*		buffer
*
* Return: 0 on success, -ENOBUFS on failure
*/
int ctx_save_replace_tlv(unsigned char type, unsigned int size, const char *data, unsigned char *offset)
{
	unsigned char *x;
	unsigned char *y;
	unsigned long flags;

	if (!tlv_msg.msg_buffer) {
		return -ENOMEM;
	}

	spin_lock_irqsave(&tlv_msg.spinlock, flags);
	x = offset;
	y = tlv_msg.msg_buffer + tlv_msg.len;

	if ((x + CTX_SAVE_SCM_TLV_TYPE_LEN_SIZE + size) >= y) {
		spin_unlock_irqrestore(&tlv_msg.spinlock, flags);
		return -ENOBUFS;
	}

	x[0] = type;
	x[1] = size;
	x[2] = size >> 8;

	memcpy(x + 3, data, size);
	spin_unlock_irqrestore(&tlv_msg.spinlock, flags);

	return 0;
}
/*
* Function: ctx_save_add_tlv
* Description: Appends dump segment as a TLV entry to the end of the
* global crashdump buffer.
*
* @param: 	[in] type - Type associated with Dump segment
*		[in] size - Size associated with Dump segment
*		[in] data - Physical address of the Dump segment
*
* Return: 0 on success, -ENOBUFS on failure
*/
int ctx_save_add_tlv(unsigned char type, unsigned int size, const char *data)
{
	unsigned char *x;
	unsigned char *y;
	unsigned long flags;

	if (!tlv_msg.msg_buffer) {
		return -ENOMEM;
	}

	spin_lock_irqsave(&tlv_msg.spinlock, flags);
	x = tlv_msg.cur_msg_buffer_pos;
	y = tlv_msg.msg_buffer + tlv_msg.len;

	if ((x + CTX_SAVE_SCM_TLV_TYPE_LEN_SIZE + size) >= y) {
		spin_unlock_irqrestore(&tlv_msg.spinlock, flags);
		return -ENOBUFS;
	}

	x[0] = type;
	x[1] = size;
	x[2] = size >> 8;

	memcpy(x + 3, data, size);

	tlv_msg.cur_msg_buffer_pos +=
		(size + CTX_SAVE_SCM_TLV_TYPE_LEN_SIZE);

	spin_unlock_irqrestore(&tlv_msg.spinlock, flags);
	return 0;
}

/*
* Function: minidump_remove_segments
* Description: Traverse metadata list and search for the TLV
* entry corresponding to the input virtual address. If found,
* set va of the Metadata list node to 0 and invalidate the TLV
* entry in the crashdump buffer by setting type to
* CTX_SAVE_LOG_DUMP_TYPE_EMPTY
*
* @param: [in] virt_addr - virtual address of the TLV to be invalidated
*
* Return: 0
*/
#ifdef CONFIG_QCA_MINIDUMP
int minidump_remove_segments(const uint64_t virt_addr)
{
	struct minidump_metadata_list *cur_node;
	struct list_head *pos;
	unsigned long flags;

	if (!tlv_msg.msg_buffer) {
		return -ENOMEM;
	}
	if (!virt_addr) {
		pr_info("\nMINIDUMP: Attempt to remove an invalid VA.");
		return 0;
	}
	spin_lock_irqsave(&tlv_msg.spinlock, flags);
	/* Traverse Metadata list*/
	list_for_each(pos, &metadata_list.list) {
	cur_node = list_entry(pos, struct minidump_metadata_list, list);
		if (cur_node->va == virt_addr) {
			/* If entry with a matching va is found, invalidate
			* this entry by setting va to 0
			*/
			cur_node->va = INVALID;
			/* Invalidate TLV entry in the crashdump buffer by setting type
			* ( value pointed to by cur_node->tlv_offset ) to
			* CTX_SAVE_LOG_DUMP_TYPE_EMPTY
			*/
			*(cur_node->tlv_offset) = CTX_SAVE_LOG_DUMP_TYPE_EMPTY;

#ifdef CONFIG_QCA_MINIDUMP_DEBUG
            if (cur_node->name != NULL) {
		    kfree(cur_node->name);
		    cur_node->name = NULL;
            }
#endif

            /* If the metadata list node has an entry in the Metadata file,
            * invalidate that entry.
            */
if (cur_node->modinfo_offset != 0)
                memset((void *)(uintptr_t)cur_node->modinfo_offset, '\0',
                        METADATA_FILE_ENTRY_LEN);

            /* If the metadata list node has an entry in the MMU Metadata file,
            * invalidate that entry.
            */
            if (cur_node->mmuinfo_offset != 0)
                memset((void *)(uintptr_t)cur_node->mmuinfo_offset, '\0',
                        MMU_FILE_ENTRY_LEN);

            minidump.hdr.num_seg--;
            break;
		}
	}
	spin_unlock_irqrestore(&tlv_msg.spinlock, flags);
	return 0;
}
EXPORT_SYMBOL(minidump_remove_segments);

/*
* Function: minidump_traverse_metadata_list
*
* Description: Maintain a Metadata list to keep track
* of TLVs in the crashdump buffer and entries in the Meta
* data file and MMU Metadata file.
*
* Each node in the Metadata list stores the name and virtual
* address associated with the dump segments and three offsets,
* tlv_offset, mod_offset and mmu_offset that stores the offset
* corresponding to the TLV in the crashdump buffer and the
* entries in the Metadata file and MMU Metadata file.
*
*                    Metadata file (12 K)   MMU Metadata file(12 K)
*                   |-------------|             |------------|
*                   |  Entry 1    |<--|         |  Entry 1   |
*                   |-------------|   |         |------------|
*                   |  Entry 2    |   | |---->  |  Entry 2   |
*                   |-------------|   | |       |------------|
*              |--->|  Entry 3    |   | |       |  Entry 3   |
*              |    |-------------|   | |       |------------|
*              |    |  Entry 4    |   | |   |-->|  Entry 4   |
*              |    |-------------|   | |   |   |------------|
*              |    |  Entry n    |   | |   |   |  Entry n   |
*              |    |-------------|   | |   |   |------------|
*              |                      | |   |
*              | |--------------------|-|---|
*              | |     Metadata List  | |
*      --------------------------------------------------------
*     | Node | Node | Node | Node | Node | Node | Node | Node |
*     |  1   |  2   |  3   |  4   |  5   |  6   |  7   |  n   |
*     --------------------------------------------------------
*              |                        |
*              |                        |
*              |-------------------|    |
*                         --------------|
*                         |        |
*                        \/       \/
*   --------------------------------------------------------------
*   |        |         |       |       |        |       |        |
*   | TLV    | TLV     | TLV   | TLV   | TLV    | TLV   | TLV    |
*   |        |         |       |       |        |       |        |
*   --------------------------------------------------------------
*                      Crashdump Buffer (12 K)
*
* When a dump segment needs to be added, the Metadata list is travered
* to check if any invalid entries (entries with va = 0) exist. If an invalid
* enrty exists, name and va of the node is updated with info from new dump segment
* and the dump segment is added as a TLV in the crashdump buffer at tlv_offset. If
* the dumpsegment has a valid name, entry is added to the Metadata file at mod_offset.
*
*
* @param: name - name associated with TLV
*	[in]  virt_addr - virtual address of the Dump segment to be added
*	[in]  phy_addr - physical address of the Dump segment to be added
*	[out] tlv_offset - offset at which corresponding TLV entry will be
*	      added to the crashdump buffer
*
* Return: 'REPLACE' if TLV needs to be inserted into the crashdump buffer at
*	offset position. 'APPEND' if TLV needs to be appended to the crashdump buffer.
*	Also tlv_offset is updated to offset at which corresponding TLV entry will be
*	added to the crashdump buffer. Return -ENOMEM if new list node was not created
*   due to an alloc failure or NULL address. Return -EINVAL if there is an attempt to
*   add a duplicate entry
*/
int minidump_traverse_metadata_list(const char *name, const unsigned long
		virt_addr, const unsigned long phy_addr, unsigned
		char **tlv_offset, unsigned long size, unsigned char type)
{

	unsigned long flags;
	struct minidump_metadata_list *cur_node;
	struct minidump_metadata_list *list_node;
	struct list_head *pos;
	int invalid_flag = 0;
	cur_node = NULL;

	/* If tlv_msg has not been initialized with non NULL value , return error*/
	if (!tlv_msg.msg_buffer) {
		return -ENOMEM;
	}

	spin_lock_irqsave(&tlv_msg.spinlock, flags);
	list_for_each(pos, &metadata_list.list) {
		/* Traverse Metadata list to check if dump sgment to be added
		already has a duplicate entry in the crashdump buffer. Also store address
		of first invalid entry , if it exists. Return EINVAL*/
		list_node = list_entry(pos, struct minidump_metadata_list, list);
		if (list_node->va == virt_addr && list_node->size == size) {
			spin_unlock_irqrestore(&tlv_msg.spinlock,
					flags);
#ifdef CONFIG_QCA_MINIDUMP_DEBUG
	pr_debug("Minidump: TLV entry with this VA is already present %s %lx\n",
			name, virt_addr);
#endif
			return -EINVAL;
		}

		if (!invalid_flag) {
			if (list_node->va == INVALID) {
				cur_node = list_node;
				invalid_flag = 1;
			}
		}
	}

	if (invalid_flag && cur_node) {
		/* If an invalid entry exits, update node entries and use
		* offset values to write TLVs to the crashdump buffer and
		* an entry in the Metadata file if applicable.
		*/
		*tlv_offset = cur_node->tlv_offset;
		cur_node->va = virt_addr;
		cur_node->pa = phy_addr;
		cur_node->size = size;
		cur_node->type = type;
		minidump.hdr.num_seg++;

		if (cur_node->modinfo_offset != 0) {
		/* If the metadata list node has an entry in the Metadata file,
		* invalidate that entry and update metadata file pointer with the
		* value at mod_offset.
		*/
			minidump_meta_info.cur_modinfo_offset = cur_node->modinfo_offset;
#ifdef CONFIG_QCA_MINIDUMP_DEBUG
		if (name != NULL) {
			cur_node->name = kstrndup(name, strlen(name), GFP_ATOMIC);
		}
#endif
		} else {
			if (name != NULL) {
				/* If the metadta list node does not have an entry in the
				* Metdata file, update metadata file pointer to point
				* to the end of the metadata file.
				*/
				cur_node->modinfo_offset = minidump_meta_info.cur_modinfo_offset;
#ifdef CONFIG_QCA_MINIDUMP_DEBUG
				cur_node->name = kstrndup(name, strlen(name), GFP_ATOMIC);
#endif
			}
		}

		if (cur_node->mmuinfo_offset != 0) {
		/* If the metadata list node has an entry in the MMU Metadata file,
		* invalidate that entry and update the MMU metadata file pointer with the
		* value at mmu_offset.
		*/
			minidump_meta_info.cur_mmuinfo_offset = cur_node->mmuinfo_offset;
		} else {
			if (IS_ENABLED(CONFIG_ARM64) || ((unsigned long)virt_addr < PAGE_OFFSET
				|| (unsigned long)virt_addr >= (unsigned long)high_memory))
				cur_node->mmuinfo_offset = minidump_meta_info.cur_mmuinfo_offset;
		}

		spin_unlock_irqrestore(&tlv_msg.spinlock,
				flags);
		/* return REPLACE to indicate TLV needs to be inserted to the crashdump buffer*/
		return REPLACE;
	}

	spin_unlock_irqrestore(&tlv_msg.spinlock, flags);
	/*
	* If no invalid entry was found, create new node provided the
	* crashdump buffer, metadata file and mmu metadata file are not full.
	*/
	if ((tlv_msg.cur_msg_buffer_pos + CTX_SAVE_SCM_TLV_TYPE_LEN_SIZE +
			sizeof(struct minidump_tlv_info) >=
			tlv_msg.msg_buffer + tlv_msg.len) ||
			(minidump_meta_info.mod_log_len + METADATA_FILE_ENTRY_LEN >= METADATA_FILE_SZ)) {
		return -ENOMEM;
	}

	if ((tlv_msg.cur_msg_buffer_pos + CTX_SAVE_SCM_TLV_TYPE_LEN_SIZE +
			sizeof(struct minidump_tlv_info) >=
			tlv_msg.msg_buffer + tlv_msg.len) ||
			(minidump_meta_info.mmu_log_len + MMU_FILE_ENTRY_LEN >= MMU_FILE_SZ)) {
		return -ENOMEM;
	}

	if (in_interrupt() || !preemptible() || rcu_preempt_depth()) {
		cur_node = (struct minidump_metadata_list *)
					 kmalloc(sizeof(struct minidump_metadata_list), GFP_ATOMIC);
	} else {
		cur_node = (struct minidump_metadata_list *)
					 kmalloc(sizeof(struct minidump_metadata_list), GFP_KERNEL);
	}


	if (!cur_node) {
		return -ENOMEM;
	}

	if (name != NULL) {
		/* If dump segment has a valid name, update name and offset with
		* pointer to the Metadata file
		*/
		cur_node->modinfo_offset = minidump_meta_info.cur_modinfo_offset;
#ifdef CONFIG_QCA_MINIDUMP_DEBUG
		cur_node->name = kstrndup(name, strlen(name), GFP_KERNEL);
#endif
	} else {
		/* If dump segment does not have a valid name, set name to null and
		* mod_offset to 0
		*/
		cur_node->modinfo_offset = 0;
#ifdef CONFIG_QCA_MINIDUMP_DEBUG
		cur_node->name = NULL;
#endif
	}
	/* Update va and offsets to crashdump buffer and MMU Metadata file*/
	cur_node->va = virt_addr;
	cur_node->size = size;
	cur_node->pa = phy_addr;
	cur_node->type = type;
	cur_node->tlv_offset = tlv_msg.cur_msg_buffer_pos;
	if ( IS_ENABLED(CONFIG_ARM64) || ( (unsigned long)virt_addr < PAGE_OFFSET
		|| (unsigned long)virt_addr >= (unsigned long)high_memory) ) {
		cur_node->mmuinfo_offset = minidump_meta_info.cur_mmuinfo_offset;
	} else {
		cur_node->mmuinfo_offset = 0;
	}
	minidump.hdr.num_seg++;
	spin_lock_irqsave(&tlv_msg.spinlock, flags);
	list_add_tail(&(cur_node->list), &(metadata_list.list));
	spin_unlock_irqrestore(&tlv_msg.spinlock, flags);
	/* return APPEND to indicate TLV needs to be appended to the crashdump buffer*/
	return APPEND;
}

/*
* Function: minidump_fill_tlv_crashdump_buffer
*
* Description: Add TLV entries into the global crashdump
* buffer at specified offset.
*
* @param: [in] start_address - Physical address of Dump segment
*		[in] type - Type associated with the	Dump segment
*		[in] size - Size associated with the Dump segment
*		[in] replace - Flag used to determine if TLV entry needs to be
*		inserted at a specified offset or appended to the end of
*		the crashdump buffer
*		[in] tlv_offset - offset at which TLV entry is added to the
*		crashdump buffer
*
* Return: 0 on success, -ENOBUFS on failure
*/
int minidump_fill_tlv_crashdump_buffer(const uint64_t start_addr, uint64_t size,
		minidump_tlv_type_t type, unsigned int replace, unsigned char *tlv_offset)
{
	struct minidump_tlv_info minidump_tlv_info;

	int ret;

	minidump_tlv_info.start = start_addr;
	minidump_tlv_info.size = size;

	if (replace && (*(tlv_offset) == CTX_SAVE_LOG_DUMP_TYPE_EMPTY)) {
		ret = ctx_save_replace_tlv(type,
				sizeof(minidump_tlv_info),
				(unsigned char *)&minidump_tlv_info, tlv_offset);
	} else {
		ret = ctx_save_add_tlv(type,
			sizeof(minidump_tlv_info),
			(unsigned char *)&minidump_tlv_info);
	}

	if (ret) {
		pr_err("Minidump: Crashdump buffer is full %d\n", ret);
		return ret;
	}

	if (tlv_msg.cur_msg_buffer_pos >=
		tlv_msg.msg_buffer + tlv_msg.len){
		pr_err("MINIDUMP buffer overflow %d\n", (int)type);
		return -ENOBUFS;
	}
	*tlv_msg.cur_msg_buffer_pos =
		CTX_SAVE_LOG_DUMP_TYPE_INVALID;

	return 0;
}

/*
* Function: minidump_fill_segments_internal
*
* Description: Add a dump segment as a TLV entry in the Metadata list
* and global crashdump buffer. Store relevant VA and PA information in
* MMU Metadata file. Also writes module information to Metadata text
* file, which is useful for post processing of collected dumps.
*
* @param: [in] start_address - Virtual address of Dump segment
*		[in] type - Type associated with the Dump segment
*		[in] size - Size associated with the Dump segment
*		[in] name - name associated with the Dump segment. Can be set to NULL.
*
* Return: 0 on success, -ENOMEM on failure
*/
int minidump_fill_segments_internal(const uint64_t start_addr, uint64_t size, minidump_tlv_type_t type, const char *name, int islowmem)
{

	int ret = 0;
	unsigned int replace = 0;
	int highmem = 0;
	struct page *minidump_tlv_page;
	uint64_t phys_addr;
	unsigned char *tlv_offset = NULL;

	/*
	* Calculate PA of Dump segment using relevant APIs for lowmem and highmem
	* virtual address.
	*/
	if (islowmem) {
		phys_addr = (uint64_t)__pa(start_addr);
	} else {
		if (!is_vmalloc_or_module_addr((const void *)(uintptr_t)(start_addr & (~(PAGE_SIZE - 1))))) {
			phys_addr = (uint64_t)__pa(start_addr);
		} else {
			minidump_tlv_page = vmalloc_to_page((const void *)(uintptr_t)
					(start_addr & (~(PAGE_SIZE - 1))));
			phys_addr = page_to_phys(minidump_tlv_page) + offset_in_page(start_addr);
			highmem = 1;
		}
	}

	replace = minidump_traverse_metadata_list(name, start_addr,(const unsigned long)phys_addr, &tlv_offset, size, (unsigned char)type);
	/* return value of -ENOMEM indicates  new list node was not created
    * due to an alloc failure. return value of -EINVAL indicates an attempt to
    * add a duplicate entry
    */
	if (replace == -EINVAL)
		return 0;

	if (replace == -ENOMEM)
		return replace;

	ret = minidump_fill_tlv_crashdump_buffer((const uint64_t)phys_addr, size, type, replace, tlv_offset);
	if (ret)
		return ret;

	minidump_store_mmu_info(start_addr,(const unsigned long)phys_addr);

	if (name)
		minidump_store_module_info(name, start_addr,(const unsigned long)phys_addr, type);

	return 0;
}

/*
* Function: minidump_fill_segments
*
* Description: Add a dump segment as a TLV entry in the Metadata list
* and global crashdump buffer. Store relevant VA and PA information in
* MMU Metadata file. Also writes module information to Metadata text
* file, which is useful for post processing of collected dumps.
*
* @param: [in] start_address - Virtual address of Dump segment
*		[in] type - Type associated with the Dump segment
*		[in] size - Size associated with the Dump segment
*		[in] name - name associated with the Dump segment. Can be set to NULL.
*
* Return: 0 on success, -ENOMEM on failure
*/

int minidump_fill_segments(const uint64_t start_addr, uint64_t size, minidump_tlv_type_t type, const char *name)
{
	return minidump_fill_segments_internal(start_addr, size, type, name, 0);
}
EXPORT_SYMBOL(minidump_fill_segments);

/*
* Function: minidump_store_mmu_info
* Description: Add virtual address and physical address
* information to a metadata file 'MMU_INFO.txt' at the
* specified offset. Useful for post processing with the
* collected dumps and offline pagetable constuction
*
* @param: [in] va - Virtual address of Dump segment
*       [in] pa - Physical address of the Dump segment
*
* Return: 0 on success, -ENOBUFS on failure
*/
int minidump_store_mmu_info(const unsigned long va, const unsigned long pa)
{

	char substring[MMU_FILE_ENTRY_LEN];
	int ret_val =0;
	unsigned long flags;

	if (!tlv_msg.msg_buffer) {
		return -ENOBUFS;
	}

	/* Check for Metadata file overflow */
	if ((minidump_meta_info.cur_mmuinfo_offset == (uintptr_t)minidump_meta_info.mmu_log + minidump_meta_info.mmu_log_len) &&
		(minidump_meta_info.mmu_log_len + MMU_FILE_ENTRY_LEN >= MMU_FILE_SZ)) {
		pr_err("\nMINIDUMP Metadata file overflow error");
		return 0;
	}

	/*
	* Check for valid minidump_meta_info.cur_modinfo_offset value. Ensure
	* that the offset is not NULL and is within bounds
	* of the Metadata file.
	*/

	if ((!(void *)(uintptr_t)minidump_meta_info.cur_mmuinfo_offset) ||
		(minidump_meta_info.cur_mmuinfo_offset < (uintptr_t)minidump_meta_info.mmu_log) ||
		(minidump_meta_info.cur_mmuinfo_offset + MMU_FILE_ENTRY_LEN >=
		((uintptr_t)minidump_meta_info.mmu_log + MMU_FILE_SZ))) {
		pr_err("\nMINIDUMP Metadata file offset error");
		return  -ENOBUFS;
	}

	ret_val = snprintf(substring, MMU_FILE_ENTRY_LEN,
			"\nva=%lx pa=%lx", va,pa);

    /* Check for Metadatafile overflow */
	if (minidump_meta_info.mmu_log_len + MMU_FILE_ENTRY_LEN >=  MMU_FILE_SZ) {
		return -ENOBUFS;
	}

	spin_lock_irqsave(&tlv_msg.spinlock, flags);
	memset((void *)(uintptr_t)minidump_meta_info.cur_mmuinfo_offset, '\0', MMU_FILE_ENTRY_LEN);
	snprintf((char *)(uintptr_t)minidump_meta_info.cur_mmuinfo_offset, MMU_FILE_ENTRY_LEN, "%s", substring);

	if (minidump_meta_info.cur_mmuinfo_offset == (uintptr_t)minidump_meta_info.mmu_log + minidump_meta_info.mmu_log_len) {
		minidump_meta_info.mmu_log_len = minidump_meta_info.mmu_log_len + MMU_FILE_ENTRY_LEN;
		minidump_meta_info.cur_mmuinfo_offset = (uintptr_t)minidump_meta_info.mmu_log + minidump_meta_info.mmu_log_len;
	} else {
		minidump_meta_info.cur_mmuinfo_offset = (uintptr_t)minidump_meta_info.mmu_log + minidump_meta_info.mmu_log_len;
	}
	spin_unlock_irqrestore(&tlv_msg.spinlock, flags);
	return 0;
}
/*
* Function: store_module_info
* Description: Add module name and virtual address information
* to a metadata file 'MODULE_INFO.txt' at the specified offset.
* Useful for post processing with the collected dumps.
*
* @param: [in] address - Virtual address of Dump segment
*		[in] type - Type associated with the Dump segment
*		[in] name - name associated with the Dump segment.
*		If set to NULL,enrty is not written to the file
*
* Return: 0 on success, -ENOBUFS on failure
*/
int minidump_store_module_info(const char *name ,const unsigned long va,
					const unsigned long pa, minidump_tlv_type_t type)
{

	char substring[METADATA_FILE_ENTRY_LEN];
	char *mod_name;
	int ret_val =0;
	unsigned long flags;

	if (!tlv_msg.msg_buffer) {
		return -ENOBUFS;
	}

	/* Check for Metadata file overflow */
	if ((minidump_meta_info.cur_modinfo_offset == (uintptr_t)minidump_meta_info.mod_log + minidump_meta_info.mod_log_len) &&
		(minidump_meta_info.mod_log_len + METADATA_FILE_ENTRY_LEN >= METADATA_FILE_SZ)) {
		pr_err("\nMINIDUMP Metadata file overflow error");
		return 0;
	}

	/*
	* Check for valid minidump_meta_info.cur_modinfo_offset value. Ensure
	* that the offset is not NULL and is within bounds
	* of the Metadata file.
	*/
	if ((!(void *)(uintptr_t)minidump_meta_info.cur_modinfo_offset) ||
		(minidump_meta_info.cur_modinfo_offset < (uintptr_t)minidump_meta_info.mod_log) ||
		(minidump_meta_info.cur_modinfo_offset + METADATA_FILE_ENTRY_LEN >=
		((uintptr_t)minidump_meta_info.mod_log + METADATA_FILE_SZ))) {
		pr_err("\nMINIDUMP Metadata file offset error");
		return  -ENOBUFS;
	}

	/* Check for valid name */
	if (!name)
		return 0;

	if (in_interrupt() || !preemptible() || rcu_preempt_depth()) {
		mod_name = kstrndup(name, strlen(name), GFP_ATOMIC);
	} else {
		mod_name = kstrndup(name, strlen(name), GFP_KERNEL);
	}

	if (!mod_name)
		return 0;

	/* Truncate name if name is greater than 28 char */
	if (strlen(mod_name) > NAME_LEN) {
		mod_name[NAME_LEN] = '\0';
	}

	if (type == CTX_SAVE_LOG_DUMP_TYPE_LEVEL1_PT || type == CTX_SAVE_LOG_DUMP_TYPE_DMESG) {
		ret_val = snprintf(substring, METADATA_FILE_ENTRY_LEN,
		"\n%s pa=%lx", mod_name, (unsigned long)pa);
	} else if (type == CTX_SAVE_LOG_DUMP_TYPE_WLAN_MOD_DEBUGFS) {
		ret_val = snprintf(substring, METADATA_FILE_ENTRY_LEN,
		"\nDFS %s pa=%lx", mod_name, (unsigned long)pa);
	} else {
		ret_val = snprintf(substring, METADATA_FILE_ENTRY_LEN,
		"\n%s va=%lx", mod_name, va);
	}

	/* Check for Metadatafile overflow */
	if (minidump_meta_info.mod_log_len + METADATA_FILE_ENTRY_LEN >=  METADATA_FILE_SZ) {
		kfree(mod_name);
		return -ENOBUFS;
	}

	spin_lock_irqsave(&tlv_msg.spinlock, flags);
	memset((void *)(uintptr_t)minidump_meta_info.cur_modinfo_offset, '\0', METADATA_FILE_ENTRY_LEN);
	snprintf((char *)(uintptr_t)minidump_meta_info.cur_modinfo_offset, METADATA_FILE_ENTRY_LEN, "%s", substring);

	if (minidump_meta_info.cur_modinfo_offset == (uintptr_t)minidump_meta_info.mod_log + minidump_meta_info.mod_log_len) {
		minidump_meta_info.mod_log_len = minidump_meta_info.mod_log_len + METADATA_FILE_ENTRY_LEN;
		minidump_meta_info.cur_modinfo_offset = (uintptr_t)minidump_meta_info.mod_log + minidump_meta_info.mod_log_len;
	} else {
		minidump_meta_info.cur_modinfo_offset = (uintptr_t)minidump_meta_info.mod_log + minidump_meta_info.mod_log_len;
	}
	spin_unlock_irqrestore(&tlv_msg.spinlock, flags);
	kfree(mod_name);
	return 0;
}
#endif /* CONFIG_QCA_MINIDUMP */

/*
* Function: ctx_save_fill_log_dump_tlv
* Description: Add 'static' dump segments - uname, demsg,
* page global directory, linux buffer and metadata text
* file to the global crashdump buffer
*
*
* Return: 0 on success, -ENOBUFS on failure
*/
static int ctx_save_fill_log_dump_tlv(void)
{
	struct new_utsname *uname;
	int ret_val;

#ifdef CONFIG_QCA_MINIDUMP
	struct minidump_tlv_info pagetable_tlv_info;
	struct minidump_tlv_info log_buf_info;
	struct minidump_tlv_info linux_banner_info;
	minidump_meta_info.mod_log_len = 0;
	minidump.hdr.num_seg = 0;
	minidump_meta_info.cur_modinfo_offset = (uintptr_t)minidump_meta_info.mod_log;
	minidump_meta_info.mmu_log_len = 0;
	minidump_meta_info.cur_mmuinfo_offset = (uintptr_t)minidump_meta_info.mmu_log;
	INIT_LIST_HEAD(&metadata_list.list);
#endif /* CONFIG_QCA_MINIDUMP */
	uname = utsname();

	ret_val = ctx_save_add_tlv(CTX_SAVE_LOG_DUMP_TYPE_UNAME,
			    sizeof(*uname),
			    (unsigned char *)uname);
	if (ret_val)
		return ret_val;

#ifdef CONFIG_QCA_MINIDUMP
	minidump_get_log_buf_info(&log_buf_info.start, &log_buf_info.size);
	ret_val = minidump_fill_segments_internal(log_buf_info.start, log_buf_info.size,
						CTX_SAVE_LOG_DUMP_TYPE_DMESG, "DMESG", 1);
	if (ret_val) {
		pr_err("Minidump: Crashdump buffer is full %d \n", ret_val);
		return ret_val;
	}

	minidump_get_pgd_info(&pagetable_tlv_info.start, &pagetable_tlv_info.size);
	ret_val = minidump_fill_segments_internal(pagetable_tlv_info.start,
				pagetable_tlv_info.size, CTX_SAVE_LOG_DUMP_TYPE_LEVEL1_PT, "PGD", 1);
	if (ret_val) {
		pr_err("Minidump: Crashdump buffer is full %d \n", ret_val);
		return ret_val;
	}

	minidump_get_linux_buf_info(&linux_banner_info.start, &linux_banner_info.size);
	ret_val = minidump_fill_segments_internal(linux_banner_info.start, linux_banner_info.size,
				CTX_SAVE_LOG_DUMP_TYPE_WLAN_MOD, NULL, 1);
	if (ret_val) {
		pr_err("Minidump: Crashdump buffer is full %d \n", ret_val);
		return ret_val;
	}

	ret_val = minidump_fill_segments_internal((uint64_t)(uintptr_t)minidump_meta_info.mod_log,(uint64_t)__pa(&minidump_meta_info.mod_log_len),
					CTX_SAVE_LOG_DUMP_TYPE_WLAN_MOD_INFO, NULL, 1);
	if (ret_val) {
		pr_err("Minidump: Crashdump buffer is full %d \n", ret_val);
		return ret_val;
	}

	ret_val = minidump_fill_segments_internal((uint64_t)(uintptr_t)minidump_meta_info.mmu_log,(uint64_t)__pa(&minidump_meta_info.mmu_log_len),
					CTX_SAVE_LOG_DUMP_TYPE_WLAN_MMU_INFO, NULL, 1);
	if (ret_val) {
		pr_err("Minidump: Crashdump buffer is full %d \n", ret_val);
		return ret_val;
	}
#endif /* CONFIG_QCA_MINIDUMP */
	if (tlv_msg.cur_msg_buffer_pos >=
		tlv_msg.msg_buffer + tlv_msg.len)
	return -ENOBUFS;

	return 0;
}

/*
* Function: minidump_dump_wlan_modules
* Description: Add module structure, section attributes and bss sections
* of specified modules to the Metadata list. Also include a subset of
* kernel module list in the Metadata list due to T32 limitaion
*
* T32 Limitation: T32 scripts expect to parse the module list from
* the list head , and allows loading of specific modules only if it
* is included in this list. Instead of dumping each node of the complete
* kernel module list ( which is very large and will take up a lot of TLVs ),
* dump only a subset of the list that is required to load the specified modules.
*
* @param: none
*
* Return: NOTIFY_DONE on success, -ENOMEM on failure
*/
#ifdef CONFIG_QCA_MINIDUMP
int minidump_dump_wlan_modules(void){

	struct module *mod;
	struct minidump_tlv_info module_tlv_info;
	int ret_val;
	unsigned int i;
	int wlan_count = 0;
	int minidump_module_list_index;

	/*Dump list head*/
	module_tlv_info.start = (uintptr_t)minidump_modules;
	module_tlv_info.size = sizeof(struct module);
	ret_val = minidump_fill_segments_internal(module_tlv_info.start,
		module_tlv_info.size, CTX_SAVE_LOG_DUMP_TYPE_WLAN_MOD, "mod_list_head", 0);
	if (ret_val) {
		pr_err("Minidump: Crashdump buffer is full %d\n", ret_val);
		return ret_val;
	}

	list_for_each_entry_rcu(mod, minidump_modules, list) {

		#ifdef CONFIG_QCA_MINIDUMP_DEBUG
		pr_info("\n Dumping %s \n",mod->name);
		#endif

		if (mod->state != MODULE_STATE_LIVE)
			continue;

		minidump_module_list_index = 0;
		while (minidump_module_list_index < MINIDUMP_MODULE_COUNT) {
			if (!strcmp(minidump_module_list[minidump_module_list_index], mod->name))
				break;
			minidump_module_list_index ++;
		}

	/* For specified modules in minidump modules list,
		dump module struct, sections and bss */

		if (minidump_module_list_index < MINIDUMP_MODULE_COUNT ) {
			wlan_count++ ;
			module_tlv_info.start = (uintptr_t)mod;
			module_tlv_info.size = sizeof(struct module);
			ret_val = minidump_fill_segments_internal(module_tlv_info.start,
				module_tlv_info.size, CTX_SAVE_LOG_DUMP_TYPE_WLAN_MOD, NULL, 0);
			if (ret_val) {
				pr_err("Minidump: Crashdump buffer is full %d\n", ret_val);
				return ret_val;
			}

			module_tlv_info.start = (unsigned long)mod->sect_attrs;
			module_tlv_info.size = (unsigned long)(sizeof(struct module_sect_attrs) + ((sizeof(struct module_sect_attr))*(mod->sect_attrs->nsections)));
			ret_val = minidump_fill_segments_internal(module_tlv_info.start,
				module_tlv_info.size, CTX_SAVE_LOG_DUMP_TYPE_WLAN_MOD, NULL, 0);
			if (ret_val) {
				pr_err("Minidump: Crashdump buffer is full %d\n", ret_val);
				return ret_val;
			}

			for (i = 0; i < mod->sect_attrs->nsections; i++) {
				if ((!strcmp(".bss", mod->sect_attrs->attrs[i].battr.attr.name))) {
					module_tlv_info.start = (unsigned long)
					mod->sect_attrs->attrs[i].address;
					module_tlv_info.size = (unsigned long)mod->core_layout.base
						+ (unsigned long) mod->core_layout.size -
						(unsigned long)mod->sect_attrs->attrs[i].address;
#ifdef CONFIG_QCA_MINIDUMP_DEBUG
					pr_err("\n MINIDUMP VA .bss start=%lx module=%s",
						(unsigned long)mod->sect_attrs->attrs[i].address,
						mod->name);
#endif
					/* Log .bss VA of module in buffer */
					ret_val = minidump_fill_segments_internal(module_tlv_info.start,
					module_tlv_info.size, CTX_SAVE_LOG_DUMP_TYPE_WLAN_MOD,
						mod->name, 0);
					if (ret_val) {
						pr_err("Minidump: Crashdump buffer is full %d", ret_val);
						return ret_val;
					}
				}
			}
		} else {

			/* For all other modules dump module meta data*/
			module_tlv_info.start = (unsigned long)mod;
			module_tlv_info.size = sizeof(mod->list) + sizeof(mod->state) + sizeof(mod->name);
			ret_val = minidump_fill_segments_internal(module_tlv_info.start,
				module_tlv_info.size, CTX_SAVE_LOG_DUMP_TYPE_WLAN_MOD, NULL, 0);
			if (ret_val) {
				pr_err("Minidump: Crashdump buffer is full %d\n", ret_val);
				return ret_val;
			}
		}

		if ( wlan_count == MINIDUMP_MODULE_COUNT)
			return NOTIFY_DONE;
	}
	return NOTIFY_DONE;
}

static int wlan_modinfo_panic_handler(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	int ret;

	#ifdef CONFIG_QCA_MINIDUMP_DEBUG
	int count =0;
	struct minidump_metadata_list *cur_node;
	struct list_head *pos;
	#endif

	if (!tlv_msg.msg_buffer) {
		pr_err("\n Minidump: Crashdump buffer is empty");
		return NOTIFY_OK;
	}

	ret = minidump_dump_wlan_modules();
	if (ret)
		pr_err("Minidump: Error dumping modules: %d", ret);

	#ifdef CONFIG_QCA_MINIDUMP_DEBUG
	pr_err("\n Minidump: Size of Metadata file = %ld",minidump_meta_info.mod_log_len);
	pr_err("\n Minidump: Printing out contents of Metadata list");

	list_for_each(pos, &metadata_list.list) {
		count ++;
		cur_node = list_entry(pos, struct minidump_metadata_list, list);
		if (cur_node->va != 0) {
			if (cur_node->name != NULL)
				pr_info(" %s [%lx] ---> ", cur_node->name, cur_node->va);
			else
				pr_info(" un-named [%lx] ---> ", cur_node->va);
		}
	}
	pr_err("\n Minidump: # nodes in the Metadata list = %d",count);
	pr_err("\n Minidump: Size of node in Metadata list = %ld\n",
		(unsigned long)sizeof(struct minidump_metadata_list));

	#endif
	return NOTIFY_DONE;
}

/*
* Function: wlan_module_notify_exit
*
* Description: Remove module information from metadata list
* when module is unloaded. This ensures the Module/MMU metadata
* files are updated when TLVs are invlaidated.
*
* Return: 0
*/
static int wlan_module_notify_exit(struct notifier_block *self, unsigned long val, void *data) {
	struct module *mod = data;
	int i=0;
	int minidump_module_list_index = 0;

	if (val == MODULE_STATE_GOING) {
		minidump_module_list_index = 0;
	/* Remove module info TLV from metadata list and invalidate entires in Metadata files*/
		minidump_remove_segments((const uint64_t)(uintptr_t)mod);

		while (minidump_module_list_index < MINIDUMP_MODULE_COUNT) {
			if (!strcmp(minidump_module_list[minidump_module_list_index], mod->name)) {
			/* For specific modules, additionally remove bss and sect attribute TLVs*/
				minidump_remove_segments((const uint64_t)(uintptr_t)mod->sect_attrs);
				for (i = 0; i < mod->sect_attrs->nsections; i++) {
					if ((!strcmp(".bss", mod->sect_attrs->attrs[i].battr.attr.name))) {
						minidump_remove_segments((const uint64_t)
						(uintptr_t)mod->sect_attrs->attrs[i].address);
#ifdef CONFIG_QCA_MINIDUMP_DEBUG
				pr_err("\n Minidump: mod=%s sect=%lx bss=%lx has been removed",
					mod->name, (unsigned long)(uintptr_t)mod->sect_attrs,
					(unsigned long)(uintptr_t)mod->sect_attrs->attrs[i].address);
#endif
						break;
					}
				}
				break;
			}
			minidump_module_list_index ++;
		}
	}
	return 0;
}

struct notifier_block wlan_module_exit_nb = {
    .notifier_call = wlan_module_notify_exit,
};

static struct notifier_block wlan_panic_nb = {
	.notifier_call  = wlan_modinfo_panic_handler,
};
#endif /* CONFIG_QCA_MINIDUMP */

static int ctx_save_panic_handler(struct notifier_block *nb,
				  unsigned long event, void *ptr)
{
	tlv_msg.is_panic = true;
	return NOTIFY_DONE;
}

static struct notifier_block panic_nb = {
	.notifier_call = ctx_save_panic_handler,
};

static int ctx_save_probe(struct platform_device *pdev)
{
	void *scm_regsave;
#ifdef CONFIG_QCA_MINIDUMP
	struct device_node *of_node = pdev->dev.of_node;
	struct device_node *node;
#endif /* CONFIG_QCA_MINIDUMP */
	const struct ctx_save_props *prop = device_get_match_data(&pdev->dev);
	int ret;

	if (!prop)
		return -ENODEV;

	scm_regsave = (void *) __get_free_pages(GFP_KERNEL,
				get_order(prop->crashdump_page_size));

	if (!scm_regsave)
		return -ENOMEM;

	ret = qti_scm_regsave(SCM_SVC_UTIL, SCM_CMD_SET_REGSAVE,
			scm_regsave, prop->crashdump_page_size);

	if (ret) {
		pr_err("Setting register save address failed.\n"
			"Registers won't be dumped on a dog bite\n");
		return ret;
	}

#ifdef CONFIG_QCA_MINIDUMP
	node = of_parse_phandle(of_node, "memory-region", 0);
	if (node) {
		dump2mem_info.rsvd_mem = of_reserved_mem_lookup(node);
		if (!dump2mem_info.rsvd_mem)
			pr_warn("Minidump: rsvd region is not specified \n");
		else {
			INIT_DELAYED_WORK(&dump2mem_info.work,
					dump2mem_workfn);

			/* kickstart worker after 50 secs */
			schedule_delayed_work(&dump2mem_info.work,
					msecs_to_jiffies(50000));
		}
	}
#endif /* CONFIG_QCA_MINIDUMP */

	spin_lock_init(&tlv_msg.spinlock);
	tlv_msg.msg_buffer = scm_regsave + prop->tlv_msg_offset;
	tlv_msg.cur_msg_buffer_pos = tlv_msg.msg_buffer;
	tlv_msg.len = prop->crashdump_page_size -
				 prop->tlv_msg_offset;
	ret = ctx_save_fill_log_dump_tlv();

	/* if failed, we still return 0 because it should not
	 * affect the boot flow. The return value 0 does not
	 * necessarily indicate success in this function.
	 */
	if (ret) {
		pr_err("log dump initialization failed\n");
		return 0;
	}

	ret = atomic_notifier_chain_register(&panic_notifier_list, &panic_nb);

	if (ret)
		dev_err(&pdev->dev,
			"Failed to register panic notifier\n");

#ifdef CONFIG_QCA_MINIDUMP
	ret = register_module_notifier(&wlan_module_exit_nb);
	if (ret)
		dev_err(&pdev->dev, "Failed to register WLAN  module exit notifier\n");

	ret = atomic_notifier_chain_register(&panic_notifier_list,
				&wlan_panic_nb);
	if (ret)
		dev_err(&pdev->dev,
			"Failed to register panic notifier for WLAN module info\n");
	register_sysrq_key('y', &sysrq_minidump_op);
#endif /* CONFIG_QCA_MINIDUMP */
	return ret;
}

const struct ctx_save_props ctx_save_props_ipq5018 = {
	.tlv_msg_offset = (1012 * SZ_1K),
	/* As SBL overwrites the NSS IMEM, TZ has to copy it to some memory
	 * on crash before it restarts the system. Hence, reserving of 384K
	 * is required to copy the NSS IMEM before restart is done.
	 * So that TZ can dump NSS dump data after the first 8K.
	 *
	 * get_order function returns the next higher order as output,
	 * so when we pass 392K(8K for regsave + 384K for NSS IMEM) as argument
	 * 512K will be allocated.
	 *
	 * 3K is required for DCC regsave memory.
	 * 82K is unused currently and can be used based on future needs.
	 * 12K is used for crashdump TLV buffer for Minidump feature.
	 */

	/*
	 * The memory is allocated using alloc_pages, hence it will be in
	 * power of 2. The unused memory is the result of using alloc_pages.
	 * As we need contigous memory for > 256K we have to use alloc_pages.
	 *
	 *		 ---------------
	 *		|      8K	|
	 *		|    regsave	|
	 *		 ---------------
	 *		|		|
	 *		|     192K	|
	 *		|    NSS IMEM	|
	 *		|		|
	 *		|		|
	 *		 ---------------
	 *		|     352 K     |
	 *		|    BTSS RAM   |
	 *		 ---------------
	 *		|    3K - DCC	|
	 *		 ---------------
	 *		|		|
	 *		|     457K	|
	 *		|    Unused	|
	 *		|		|
	 *		 ---------------
	 *		|     12 K     |
	 *		|   TLV Buffer |
	 *		 ---------------
	 *
	 */
	.crashdump_page_size = (SZ_8K + (192 * SZ_1K) + (352 * SZ_1K) +
				(3 * SZ_1K) + (457 * SZ_1K) + (12 * SZ_1K)),
};

const struct ctx_save_props ctx_save_props_ipq5332 = {
	.tlv_msg_offset = (500 * SZ_1K),

	/* 300K for TME-L Crashdump
	 * 8K for regsave
	 * 192K is unused currently and can be used based on future needs.
	 * 12K is used for crashdump TLV buffer for Minidump feature.
	 *
	 * get_order function returns the next higher order as output,
	 * so when we pass 320K as argument 512K will be allocated.
	 *
	 * The memory is allocated using alloc_pages, hence it will be in
	 * power of 2. The unused memory is the result of using alloc_pages.
	 * As we need contigous memory for > 256K we have to use alloc_pages.
	 *
	 *              -----------------
	 *              |           	|
	 *              |      300K	|
	 *              |    TMEL ctxt  |
	 *              |               |
	 *              -----------------
	 *              |     8K        |
	 *              |    regsave    |
	 *              |               |
	 *              -----------------
	 *              |               |
	 *              |     192K      |
	 *              |    Unused     |
	 *              |               |
	 *              -----------------
	 *              |     12 K      |
	 *              |   TLV Buffer  |
	 *		 ---------------
	 *
	 */
	.crashdump_page_size = ((300 * SZ_1K) + (8 * SZ_1K) + (192 * SZ_1K) +
				(12 * SZ_1K)),
};

const struct ctx_save_props ctx_save_props_ipq6018 = {
	.tlv_msg_offset = (244 * SZ_1K),
	/* As XBL overwrites the NSS UTCM, TZ has to copy it to some memory
	 * on crash before it restarts the system. Hence, reserving of 192K
	 * is required to copy the NSS UTCM before restart is done.
	 * So that TZ can dump NSS dump data after the first 8K.
	 *
	 * 3K for DCC Memory
	 *
	 * get_order function returns the next higher order as output,
	 * so when we pass 203K as argument 256K will be allocated.
	 * 41K is unused currently and can be used based on future needs.
	 *
	 * 12K is used for crashdump TLV buffer for Minidump feature.
	 * For minidump feature, last 16K of crashdump page size is used for
	 * TLV buffer in the case of ipq807x. Same offset (last 16 K from end
	 * of crashdump page) is used for ipq60xx as well, to keep design
	 * consistent.
	 *
	 *
	 * The memory is allocated using alloc_pages, hence it will be in
	 * power of 2. The unused memory is the result of using alloc_pages.
	 * As we need contigous memory for > 256K we have to use alloc_pages.
	 *
	 *		 ---------------
	 *		|      8K	|
	 *		|    regsave	|
	 *		 ---------------
	 *		|		|
	 *		|     192K	|
	 *		|    NSS UTCM	|
	 *		|		|
	 *		|		|
	 *		 ---------------
	 *		|    3K - DCC	|
	 *		 ---------------
	 *		|		|
	 *		|     41K	|
	 *		|    Unused	|
	 *		|		|
	 *		 ---------------
	 *		|     12 K     |
	 *		|   TLV Buffer |
	 *		---------------
	 *
	 */
	.crashdump_page_size = (SZ_8K + (192 * SZ_1K) + (3 * SZ_1K) +
				(41 * SZ_1K) + (12 * SZ_1K)),
};

const struct ctx_save_props ctx_save_props_ipq807x = {
	.tlv_msg_offset = (500 * SZ_1K),
	/* As SBL overwrites the NSS IMEM, TZ has to copy it to some memory
	 * on crash before it restarts the system. Hence, reserving of 384K
	 * is required to copy the NSS IMEM before restart is done.
	 * So that TZ can dump NSS dump data after the first 8K.
	 * Additionally 8K memory is allocated which can be used by TZ
	 * to dump PMIC memory.
	 * get_order function returns the next higher order as output,
	 * so when we pass 400K as argument 512K will be allocated.
	 * 3K is required for DCC regsave memory.
	 * 15K is required for CPR.
	 * 82K is unused currently and can be used based on future needs.
	 * 12K is used for crashdump TLV buffer for Minidump feature.
	 *
	 * The memory is allocated using alloc_pages, hence it will be in
	 * power of 2. The unused memory is the result of using alloc_pages.
	 * As we need contigous memory for > 256K we have to use alloc_pages.
	 *
	 *		*---------------*
	 *		|      8K	|
	 *		|    regsave	|
	 *		*---------------*
	 *		|		|
	 *		|     384K	|
	 *		|    NSS IMEM	|
	 *		|		|
	 *		|		|
	 *		*---------------*
	 *		|      8K	|
	 *		|    PMIC mem	|
	 *		*---------------*
	 *		|    3K - DCC	|
	 *		|		|
	 *		*---------------*
	 *		|      15K      |
	 *		|    CPR Reg    |
	 *		* --------------*
	 *		|		|
	 *		|     82K	|
	 *		|    Unused	|
	 *		|		|
	 *		* --------------*
	 *		|     12 K      |
	 *		|   TLV Buffer  |
	 *		*---------------*
	 *
	 */
	.crashdump_page_size = (SZ_8K + (384 * SZ_1K) + (SZ_8K) + (3 * SZ_1K) +
				(15 * SZ_1K) + (82 * SZ_1K) + (12 * SZ_1K)),
};

const struct ctx_save_props ctx_save_props_ipq9574 = {
	.tlv_msg_offset = (500 * SZ_1K),

	/* Allocating 300K for TME-L Crashdump
	 * 80K for regsave
	 * 3K for DCC Memory
	 * 117K is unused currently and can be used based on future needs.
	 * 12K is used for crashdump TLV buffer for Minidump feature.
	 *
	 * get_order function returns the next higher order as output,
	 * so when we pass 395K as argument 512K will be allocated.
	 *
	 * The memory is allocated using alloc_pages, hence it will be in
	 * power of 2. The unused memory is the result of using alloc_pages.
	 * As we need contigous memory for > 256K we have to use alloc_pages.
	 *
	 *              -----------------
	 *              |           	|
	 *              |      300K	|
	 *              |    TMEL ctxt  |
	 *              |               |
	 *              |               |
	 *              -----------------
	 *              |     80K       |
	 *              |    regsave    |
	 *              |               |
	 *              -----------------
	 *              |    3K - DCC   |
	 *              -----------------
	 *              |               |
	 *              |     117K      |
	 *              |    Unused     |
	 *              |               |
	 *              -----------------
	 *              |     12 K      |
	 *              |   TLV Buffer  |
	 *              -----------------
	 *
	 */
	.crashdump_page_size = ((300 * SZ_1K) + (80 * SZ_1K) + (3 * SZ_1K) +
				(117 * SZ_1K) + (12 * SZ_1K)),
};

static const struct of_device_id ctx_save_of_table[] = {
	{
		.compatible = "qti,ctxt-save-ipq5018",
		.data = (void *)&ctx_save_props_ipq5018,
	},
	{
		.compatible = "qti,ctxt-save-ipq5332",
		.data = (void *)&ctx_save_props_ipq5332,
	},
	{
		.compatible = "qti,ctxt-save-ipq6018",
		.data = (void *)&ctx_save_props_ipq6018,
	},
	{
		.compatible = "qti,ctxt-save-ipq8074",
		.data = (void *)&ctx_save_props_ipq807x,
	},
	{
		.compatible = "qti,ctxt-save-ipq9574",
		.data = (void *)&ctx_save_props_ipq9574,
	},
	{}
};

static struct platform_driver ctx_save_driver = {
	.probe = ctx_save_probe,
	.driver = {
		.name = "qti_ctx_save_driver",
		.of_match_table = ctx_save_of_table,
	},
};
module_platform_driver(ctx_save_driver);

MODULE_DESCRIPTION("QTI context save driver for storing cpu regs, etc");
MODULE_LICENSE("GPL v2");
