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
 ****************************************************************************
 * vFlash Block IO Oops log driver
 *
 * Author: Jayesh Patel <jayeshp@broadcom.com>
 ****************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kmsg_dump.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>
#include <linux/scatterlist.h>
#include <linux/string_helpers.h>
#include <linux/delay.h>
#include <linux/capability.h>
#include <linux/compat.h>
#include <linux/uaccess.h>
#include <linux/file.h>
#include <vfbio.h>

#define VFBIO_OOPS_DUMP_SIGNATURE "BRCM-OOPS-DUMP-VFBIO"
#define VFBIO_OOPS_DUMP_HDR_LENGTH \
	((sizeof(OOPS_MMC_DUMP_SIGNATURE))-1+(sizeof(unsigned long)))
static char *dump_mark =
	"================================";
static char *dump_start_str =
	"PREVIOUS_KERNEL_OOPS_DUMP_START";
static char *dump_end_str =
	"PREVIOUS_KERNEL_OOPS_DUMP_END";

/* At a time how many blocks we read or write */
#define VFBIO_OOPS_NUM_RW_BLOCKS   16
#define VFBIO_OOPS_INVALID 0xFFFFFFFF

static struct platform_device *dummy;
static struct vfbio_oops_platform_data *dummy_data;

static char blkdev[80];
module_param_string(blkdev, blkdev, 80, 0400);
MODULE_PARM_DESC(blkdev,
		"Name or path of the block io device to use");

static char *dump_file_path;
module_param(dump_file_path, charp, 0400);
MODULE_PARM_DESC(dump_file_path,
		"Dump the panics to file instead of console");

static char *dump_to_console;
module_param(dump_to_console, charp, 0400);
MODULE_PARM_DESC(dump_to_console,
		"Specify whether to dump to console or not");

static unsigned long rw_blocks = VFBIO_OOPS_NUM_RW_BLOCKS;
module_param(rw_blocks, ulong, 0400);
MODULE_PARM_DESC(rw_blocks,
		"record size for in blocks (default 16)");

static struct vfbio_oops_context {
	struct kmsg_dumper	dump;
	struct file		*file;
	struct block_device	*bdev;
	/* Total bytes that we write or read each time */
	unsigned long		num_rw_bytes;
	unsigned long long	offset;
	char			*buff;
	u64			i_size;  /* Size of LUN */
	u32			blk_sz;  /* Block size of LUN */
	u32			n_blks;  /* Number of blocks */
} vfbio_oops_context;

struct vfbio_oops_platform_data {
	struct platform_device	*pdev;
	char			*devname;
	char			*dump_file_path;
	char			*dump_to_console;
};

static inline
struct file *file_open(const char *path, int flags, int rights)
{
	struct file *filp = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, rights);
	set_fs(oldfs);
	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		return NULL;
	}
	return filp;
}

static inline
void file_close(struct file *file)
{
	filp_close(file, NULL);
}

static inline
int file_read(struct file *file, unsigned long long offset,
	      unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_read(file, data, size, &offset);

	set_fs(oldfs);
	return ret;
}

static inline
int file_write(struct file *file, unsigned long long offset,
	       unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_write(file, data, size, &offset);

	set_fs(oldfs);
	return ret;
}

static inline
int blkdev_file_bind(struct block_device *bdev, struct file *filp)
{
	int err;

	bdgrab(bdev);
	err = blkdev_get(bdev, filp->f_mode | FMODE_EXCL, blkdev_file_bind);
	if (err)
		goto out;
	//filp->f_flags |= O_DIRECT;
	filp->f_mapping = bdev->bd_inode->i_mapping;
	file_inode(filp)->i_mapping = bdev->bd_inode->i_mapping;
	filp->private_data = bdev;
	return 0;
out:
	return err;
}

static inline
int blkdev_file_unbind(struct block_device *bdev, struct file *filp)
{
	file_inode(filp)->i_mapping = &(file_inode(filp)->i_data);
	blkdev_put(bdev, filp->f_mode | FMODE_EXCL);
	bdput(bdev);
	return 0;
}

extern int vfbio_panic_write(struct block_device *bdev, u32 start_blk, void *buf,
		      u32 n_blks);

static void vfbio_oops_do_dump(struct kmsg_dumper *dumper,
			       enum kmsg_dump_reason reason)
{
	struct vfbio_oops_context *cxt =
		container_of(dumper, struct vfbio_oops_context, dump);
	char *buff = cxt->buff;
	bool rc = true;
	size_t header_size, text_len = 0;
	int text_length = 0, read_cnt = 0;

	if (!in_irq()) {
		pr_info("vfbio_oops_do_dump: Not in IRQ Routine...\n");
		return;
	}
	/* Add Dump signature and
	* block size before kernel log
	*/
	memset(buff, '\0', cxt->num_rw_bytes);
	memcpy(buff, VFBIO_OOPS_DUMP_SIGNATURE,
			strlen(VFBIO_OOPS_DUMP_SIGNATURE));
	header_size = strlen(VFBIO_OOPS_DUMP_SIGNATURE)
						+ sizeof(int);
	read_cnt = cxt->num_rw_bytes-header_size-text_length;
	buff += header_size;
	while (1) {
		rc = kmsg_dump_get_buffer(dumper, false,
				buff, read_cnt, &text_len);
		text_length += text_len;
		if (!rc) {
			pr_err("vfbio_oops_do_dump: end of read from the kmsg dump\n");
			break;
		}
		buff = (char *)cxt->buff+header_size+text_length;
		read_cnt = cxt->num_rw_bytes-header_size-text_length;
		if (read_cnt<=0) {
			pr_err("vfbio_oops_do_dump: read limit reached from the kmsg dump\n");
			break;
		}
	}
	pr_info("vfbio_oops_do_dump: writing to VFBIO = %d\n",
				text_length);
	buff = (char *)cxt->buff+strlen(VFBIO_OOPS_DUMP_SIGNATURE);
	memcpy(buff, &text_length, sizeof(int));
	//file_write(cxt->file, cxt->offset, cxt->buff, cxt->num_rw_bytes);

	vfbio_crash_write(cxt->bdev, cxt->offset, cxt->buff, rw_blocks);
}

static int __init vfbio_oops_probe(struct platform_device *pdev)
{
	struct vfbio_oops_platform_data *pdata = pdev->dev.platform_data;
	struct vfbio_oops_context *cxt = &vfbio_oops_context;
	int err = -EINVAL;
	int i;
	char *buf;
	int text_len = 0;
	loff_t pos = 0;
	struct file *file = NULL;
	mm_segment_t old_fs;
	char marker_string[200]="";

	if (!pdata) {
		pr_err("vfbio_oops_probe: No platform data. Error!\n");
		return -EINVAL;
	}

	if (pdata->devname == NULL) {
		pr_err("vfbio_oops_probe: devname is NULL\n");
		return err;
	}

	cxt->file = file_open(blkdev, O_RDWR, 0);
	if (!cxt->file) {
		pr_err("vfbio_oops_probe: %s open failed\n", blkdev);
		goto kmalloc_failed;
	}
	cxt->bdev = I_BDEV(cxt->file->f_mapping->host);
	cxt->i_size = i_size_read(cxt->bdev->bd_inode);
	cxt->blk_sz = bdev_logical_block_size(cxt->bdev);
	cxt->n_blks = cxt->i_size / cxt->blk_sz;
	cxt->num_rw_bytes = cxt->blk_sz * rw_blocks;
	pr_alert("vfbio_oops_probe: %s blk_sz=%d blocks=%d\n", blkdev,
		 cxt->blk_sz, cxt->n_blks);

	/* Allocate min io size buffer to be used in do_dump */
	cxt->buff = kmalloc(cxt->num_rw_bytes, GFP_KERNEL);
	if (cxt->buff == NULL) {
		err = -EINVAL;
		goto kmalloc_failed;
	}

	cxt->dump.dump = vfbio_oops_do_dump;
	err = kmsg_dump_register(&cxt->dump);
	if (err) {
		pr_err("vfbio_oops_probe: registering kmsg dumper failed\n");
		goto kmsg_dump_register_failed;
	}

	if (pdata->dump_file_path) {
		pr_info("vfbio_oops_probe: dump_file_path = %s\n",
				pdata->dump_file_path);
		file = filp_open(pdata->dump_file_path, O_WRONLY|O_CREAT, 0644);
		if (IS_ERR(file)) {
			pr_err("vfbio_oops_probe: filp_open failed, dump to console only\n");
			file = NULL;
		} else {
			old_fs = get_fs();
			set_fs(get_ds());
		}
	} else if ((pdata->dump_to_console) &&
			((!strcmp(pdata->dump_to_console, "n")) ||
			(!strcmp(pdata->dump_to_console, "no"))))
		pr_info("vfbio_oops_probe: dump_to_console=no,OEM has own script");
	else
		pr_info("vfbio_oops_probe: If any panic , it will be dumped to console\n");

	buf = (char *)cxt->buff;

	file_read(cxt->file, 0, buf, strlen(VFBIO_OOPS_DUMP_SIGNATURE));
	if (!strncmp(VFBIO_OOPS_DUMP_SIGNATURE,
			buf, strlen(VFBIO_OOPS_DUMP_SIGNATURE))) {
		sprintf(marker_string, "\n%s%s%s\n", dump_mark, dump_start_str, dump_mark);
		pr_err("%s", marker_string);
		if (file) {
			vfs_write(file, marker_string,
				strlen(marker_string), &pos);
			pos = pos+strlen(marker_string);

		}
		if (pdata->dump_file_path)
			pr_info("vfbio_oops_probe: panics dumped to the file [%s]\n",
				pdata->dump_file_path);
		else if ((pdata->dump_to_console) &&
			((!strcmp(pdata->dump_to_console, "n")) ||
			(!strcmp(pdata->dump_to_console, "no")))) {
				pr_info("vfbio_oops_probe:OEM has own script to read!\n");
				pr_err("\n%s%s%s\n", dump_mark,
					dump_end_str, dump_mark);
				return 0;
		}

		for (i = 0; i < cxt->n_blks; i+=rw_blocks) {
			file_read(cxt->file, i*cxt->num_rw_bytes, buf, cxt->num_rw_bytes);
			if (strncmp(VFBIO_OOPS_DUMP_SIGNATURE, buf,
				    strlen(VFBIO_OOPS_DUMP_SIGNATURE))) {
				break;
			}
			memcpy(&text_len, &buf[strlen(VFBIO_OOPS_DUMP_SIGNATURE)],
						sizeof(int));
			buf = buf+strlen(VFBIO_OOPS_DUMP_SIGNATURE)+sizeof(int);

			if ((text_len == 0) || (text_len > cxt->num_rw_bytes)) {
				pr_info("vfbio_oops_probe:Invalid text length[%d]\n",
						text_len);
				break;
			}
			pr_info("vfbio_oops_probe: printing text length = %d\n",
								text_len);

			if (file) {
				vfs_write(file, buf, text_len, &pos);
				pos = pos+text_len;
			} else {
				char *ptr = buf;
				char *line;
				while ((line = strsep(&ptr, "\n")) != NULL) {
					pr_info("%s\n", line);
				}
			}
			buf = (char *)cxt->buff;
		}
		sprintf(marker_string,"\n%s%s%s\n", dump_mark, dump_end_str, dump_mark);
		pr_err("%s", marker_string);
		if (file) {
			vfs_write(file, marker_string,
				strlen(marker_string), &pos);
			pos = pos+strlen(marker_string);
		}
		/* Clear buffer */
		buf = (char *)cxt->buff;
		memset(buf, '\0', cxt->num_rw_bytes);
		for (i = 0; i < cxt->n_blks; i+=rw_blocks) {
			file_write(cxt->file, i*cxt->num_rw_bytes, buf, cxt->num_rw_bytes);
		}
	} else
		pr_info("vfbio_oops_probe: There was no panic in earlier run\n");

	if (file) {
		filp_close(file, NULL);
		set_fs(old_fs); /*Reset to save FS*/
	}

	return 0;

kmsg_dump_register_failed:
	kfree(cxt->buff);
	cxt->buff = NULL;
kmalloc_failed:
	return err;

}

static int __exit vfbio_oops_remove(struct platform_device *pdev)
{
	struct vfbio_oops_context *cxt = &vfbio_oops_context;

	pr_info("vfbio_oops_remove\n");
	if (kmsg_dump_unregister(&cxt->dump) < 0)
		pr_err("vfbio_oops_remove: could not unregister kmsg dumper\n");
	file_close(cxt->file);
	kfree(cxt->buff);
	cxt->buff = NULL;
	return 0;
}

static struct platform_driver vfbio_oops_driver = {
	.remove			= __exit_p(vfbio_oops_remove),
	.probe                  = vfbio_oops_probe,
	.driver			= {
		.name		= "vfbio_oops",
		.owner		= THIS_MODULE,
	},
};

static int __init vfbio_oops_init(void)
{
	int ret;

	pr_info("vfbio_oops_init\n");
	ret = platform_driver_probe(&vfbio_oops_driver, vfbio_oops_probe);
	if (ret == -ENODEV) {
		/*
		* If we didn't find a platform device, we use module parameters
		* building platform data on the fly.
		*/
		pr_info("platform device not found, using module parameters\n");

		if (strlen(blkdev) == 0) {
			pr_err("vfbio_oops_init: vfbio device (blkdev=name) must be supplied\n");
			return -EINVAL;
		}

		dummy_data = kzalloc(sizeof(struct vfbio_oops_platform_data),
					GFP_KERNEL);
		if (!dummy_data)
			return -ENOMEM;

		dummy_data->devname = blkdev;
		dummy_data->dump_file_path = dump_file_path;
		dummy_data->dump_to_console = dump_to_console;
		dummy = platform_create_bundle(&vfbio_oops_driver, vfbio_oops_probe,
			NULL, 0, dummy_data,
			sizeof(struct vfbio_oops_platform_data));
		if (IS_ERR(dummy)) {
			ret = PTR_ERR(dummy);
			kfree(dummy_data);
		} else
			ret = 0;
	}
	return ret;

}
static void __exit vfbio_oops_exit(void)
{
	kfree(dummy_data);
	dummy_data = NULL;
	pr_info("vfbio_oops_exit\n");
	platform_device_unregister(dummy);
	platform_driver_unregister(&vfbio_oops_driver);
	dummy = NULL;
}

module_init(vfbio_oops_init);
module_exit(vfbio_oops_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jayesh Patel <jayesh.patel@broadcom.com>");
MODULE_DESCRIPTION("vFlash block IO Oops/Panic console logger/driver");
