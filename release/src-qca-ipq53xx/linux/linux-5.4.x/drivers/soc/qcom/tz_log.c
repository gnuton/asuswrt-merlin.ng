/*
 * Copyright (c) 2015-2017, 2020 The Linux Foundation. All rights reserved.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/debugfs.h> /* this is for DebugFS libraries */
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/qcom_scm.h>
#include <linux/slab.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/threads.h>
#include <linux/of_device.h>
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/sizes.h>

#define SMMU_DISABLE_NONE  0x0 /* SMMU Stage2 Enabled */
#define SMMU_DISABLE_S2    0x1 /* SMMU Stage2 bypass */
#define SMMU_DISABLE_ALL   0x2 /* SMMU Disabled */

#define HVC_DIAG_RING_OFFSET		2
#define HVC_DIAG_LOG_POS_INFO_OFFSET	3
#define TZ_LEGACY_RING_OFFSET		7
#define TZ_LEGACY_LOG_POS_INFO_OFFSET	522
#define TZ_LEGACY_BUF_LEN		0x1000
#define TZBSP_AES_256_ENCRYPTED_KEY_SIZE 256
#define TZBSP_NONCE_LEN 12
#define TZBSP_TAG_LEN 16
#define TZBSP_ENCRYPTION_HEADERS_SIZE 0x400

static unsigned int paniconaccessviolation = 0;
module_param(paniconaccessviolation, uint, 0644);
MODULE_PARM_DESC(paniconaccessviolation, "Panic on Access Violation detected: 0,1");

static char *smmu_state;

/**
 * struct tzbsp_log_pos_t - log position structure
 * @wrap: ring buffer wrap-around ctr
 * @offset: ring buffer current position
 */
struct tzbsp_log_pos_t {
	uint16_t wrap;
	uint16_t offset;
};

/**
 * struct tz_hvc_log_struct - TZ / HVC log info structure
 * @debugfs_dir: qti_debug_logs debugfs directory
 * @ker_buf: kernel buffer shared with TZ to get the diag log
 * @copy_buf: kernel buffer used to copy the diag log
 * @copy_len: length of the diag log that has been copied into the buffer
 * @log_buf_start: start address of the tz log buffer only available for ipq6018 and ipq9574
 * @tz_ring_off: offset in tz log buffer that contains the ring start offset
 * @tz_log_pos_info_off: offset in tz log buffer that contains log position info
 * @hvc_ring_off: offset in hvc log buffer that contains the ring start offset
 * @hvc_log_pos_info_off: offset in hvc log buffer that contains log position info
 * @buf_len: kernel buffer length
 * @lock: mutex lock for synchronization
 * @tz_kpss: boolean to handle ipq806x which has different diag log structure
 * @is_diag_id : indicates if legacy diag id scm call is available
 * @is_encrypted: indicates if tz log is encrypted on this target
 */
struct tz_hvc_log_struct {
	struct dentry *debugfs_dir;
	char *ker_buf;
	char *copy_buf;
	int copy_len;
	uint32_t log_buf_start;
	uint32_t tz_ring_off;
	uint32_t tz_log_pos_info_off;
	uint32_t hvc_ring_off;
	uint32_t hvc_log_pos_info_off;
	int buf_len;
	struct mutex lock;
	bool tz_kpss;
	bool is_diag_id;
	bool is_encrypted;
};

struct tzbsp_encr_log_t {
	/* Magic Number */
	uint32_t magic_num;
	/* version NUMBER */
	uint32_t version;
	/* encrypted log size */
	uint32_t encr_log_buff_size;
	/* Wrap value*/
	uint16_t wrap_count;
	/* AES encryption key wrapped up with oem public key*/
	uint8_t key[TZBSP_AES_256_ENCRYPTED_KEY_SIZE];
	/* Nonce used for encryption*/
	uint8_t nonce[TZBSP_NONCE_LEN];
	/* Tag to be used for Validation */
	uint8_t tag[TZBSP_TAG_LEN];
	/* Encrypted log buffer */
	uint8_t log_buf[1];
};

static int get_non_encrypted_tz_log(char *buf, uint32_t diag_start,
						uint32_t diag_size)
{
	void __iomem *virt_iobase;

	if (diag_start) {
		virt_iobase = ioremap(diag_start, diag_size);
		memcpy_fromio((void *)buf, virt_iobase, diag_size - 1);
	} else {
		pr_err("Unable to fetch TZ diag memory\n");
		return -1;
	}

	return 0;

}

unsigned int print_text(char *intro_message,
			unsigned char *text_addr,
			unsigned int size,
			char *buf, uint32_t buf_len)
{
	unsigned int i;
	unsigned int len = 0;

	pr_debug("begin address 0x%lx, size 0x%x\n", (uintptr_t)text_addr, size);
	len += scnprintf(buf + len, buf_len - len, "%s\n", intro_message);
	for (i = 0;  i < size;  i++) {
		if (buf_len <= len + 6) {
			pr_err("buffer not enough, buf_len %d, len %u\n",
				buf_len, len);
			return buf_len;
		}
		len += scnprintf(buf + len, buf_len - len, "%02hhx",
					text_addr[i]);
		if ((i & 0x1f) == 0x1f)
			len += scnprintf(buf + len, buf_len - len, "%c", '\n');
	}
	len += scnprintf(buf + len, buf_len - len, "%c", '\n');
	return len;
}

int parse_encrypted_log(char *ker_buf, uint32_t buf_len, char *copy_buf,
							uint32_t log_id)
{
	int len = 0;
	struct tzbsp_encr_log_t *encr_log_head;
	uint32_t size = 0;
	uint32_t display_buf_size = buf_len;

	encr_log_head = (struct tzbsp_encr_log_t *)ker_buf;
	pr_debug("display_buf_size = 0x%x, encr_log_buff_size = 0x%x\n",
		buf_len, encr_log_head->encr_log_buff_size);
	size = encr_log_head->encr_log_buff_size;

	len += scnprintf(copy_buf + len,
		(display_buf_size - 1) - len,
		"\n-------- New Encrypted %s --------\n",
		((log_id == QTI_TZ_QSEE_LOG_ENCR_ID) ?
		"QSEE Log" : "TZ Dialog"));

	len += scnprintf(copy_buf + len,
		(display_buf_size - 1) - len,
		"\nMagic_Num :\n0x%x\n"
		"\nVerion :\n%d\n"
		"\nEncr_Log_Buff_Size :\n%d\n"
		"\nWrap_Count :\n%d\n",
		encr_log_head->magic_num,
		encr_log_head->version,
		encr_log_head->encr_log_buff_size,
		encr_log_head->wrap_count);

	len += print_text("\nKey : ", encr_log_head->key,
		TZBSP_AES_256_ENCRYPTED_KEY_SIZE,
		copy_buf + len, display_buf_size);
	len += print_text("\nNonce : ", encr_log_head->nonce,
		TZBSP_NONCE_LEN,
		copy_buf + len, display_buf_size - len);
	len += print_text("\nTag : ", encr_log_head->tag,
		TZBSP_TAG_LEN,
		copy_buf + len, display_buf_size - len);

	if (len > display_buf_size - size)
		pr_warn("Cannot fit all info into the buffer\n");
	pr_debug("encrypted log size 0x%x, display buffer size 0x%x, used len 0x%x\n",
		size, display_buf_size, (unsigned int)len);
	len += print_text("\nLog : ", encr_log_head->log_buf, size,
				copy_buf + len, display_buf_size - len);
	return len;

}

static int get_encrypted_tz_log(char *ker_buf, uint32_t buf_len, char *copy_buf)
{
	int ret;

	/* SCM call to TZ to get encrypted tz log */
	ret = qti_scm_get_encrypted_tz_log(ker_buf, buf_len,
					QTI_TZ_DIAG_LOG_ENCR_ID);
	if (ret == QTI_TZ_LOG_NO_UPDATE) {
		pr_err("No TZ log updation from last read\n");
		return QTI_TZ_LOG_NO_UPDATE;
	} else if (ret != 0) {
		pr_err("Error in getting encrypted tz log %d\n", ret);
		return -1;
	}
	return parse_encrypted_log(ker_buf, buf_len, copy_buf, QTI_TZ_DIAG_LOG_ENCR_ID);
}

static int tz_hvc_log_open(struct inode *inode, struct file *file)
{
	struct tz_hvc_log_struct *tz_hvc_log;
	char *ker_buf;
	char *copy_buf;
	uint32_t buf_len;
	uint32_t ring_off;
	uint32_t log_pos_info_off;

	uint32_t *diag_buf;
	uint16_t ring;
	struct tzbsp_log_pos_t *log;
	uint16_t offset;
	uint16_t wrap;
	int ret;

	file->private_data = inode->i_private;

	tz_hvc_log = file->private_data;
	mutex_lock(&tz_hvc_log->lock);

	ker_buf = tz_hvc_log->ker_buf;
	copy_buf = tz_hvc_log->copy_buf;
	buf_len = tz_hvc_log->buf_len;
	tz_hvc_log->copy_len = 0;

	if (!strncmp(file->f_path.dentry->d_iname, "tz_log", sizeof("tz_log"))) {
		/* SCM call to TZ to get the tz log */
		if (tz_hvc_log->is_diag_id) {
			ret = qti_scm_tz_log(ker_buf, buf_len);
			if (ret != 0) {
				pr_err("Error in getting tz log, ret = %d\n",
					ret);
				goto out_err;
			}
		} else {
			if (tz_hvc_log->is_encrypted) {
				ret = get_encrypted_tz_log(ker_buf, buf_len,
							    copy_buf);
				if (ret == -1)
					goto out_err;
				else if (ret == QTI_TZ_LOG_NO_UPDATE)
					goto out_success;
				tz_hvc_log->copy_len = ret;
				goto out_success;
			} else {
				ret = get_non_encrypted_tz_log(ker_buf,
						tz_hvc_log->log_buf_start,
						buf_len);
				if (ret == -1)
					goto out_err;
			}
		}

		ring_off = tz_hvc_log->tz_ring_off;
		log_pos_info_off = tz_hvc_log->tz_log_pos_info_off;
	} else {
		/* SCM call to TZ to get the hvc log */
		ret = qti_scm_hvc_log(ker_buf, buf_len);
		if (ret != 0) {
			pr_err("Error in getting hvc log\n");
			goto out_err;
		}

		ring_off = tz_hvc_log->hvc_ring_off;
		log_pos_info_off = tz_hvc_log->hvc_log_pos_info_off;
	}

	diag_buf = (uint32_t *) ker_buf;
	ring = diag_buf[ring_off];
	log = (struct tzbsp_log_pos_t *) &diag_buf[log_pos_info_off];
	offset = log->offset;
	wrap = log->wrap;

	/* To support IPQ806x platform */
	if (tz_hvc_log->tz_kpss) {
		offset = buf_len - ring;
		wrap = 0;
	}

	if (wrap != 0) {
		/* since ring wrap occurred, log starts at the offset position
		 * and offset will be in the middle of the ring.
		 * ring buffer - [ <second half of log> $ <first half of log> ]
		 * $ - represents current position of the log start i.e. offset
		 */
		memcpy(copy_buf, (ker_buf + ring + offset),
				(buf_len - ring - offset));
		memcpy(copy_buf + (buf_len - ring - offset),
				(ker_buf + ring), offset);
		tz_hvc_log->copy_len = (buf_len - offset - ring)
			+ offset;
	} else {
		/* since there is no ring wrap condition, log starts at the
		 * start of the ring and offset will be the end of the log.
		 */
		memcpy(copy_buf, (ker_buf + ring), offset);
		tz_hvc_log->copy_len = offset;
	}

out_success:
	mutex_unlock(&tz_hvc_log->lock);
	return 0;

out_err:
	mutex_unlock(&tz_hvc_log->lock);
	return -EIO;
}

static ssize_t tz_hvc_log_read(struct file *fp, char __user *user_buffer,
				size_t count, loff_t *position)
{
	struct tz_hvc_log_struct *tz_hvc_log;

	tz_hvc_log = fp->private_data;

	return simple_read_from_buffer(user_buffer, count,
					position, tz_hvc_log->copy_buf,
					tz_hvc_log->copy_len);
}

static int tz_hvc_log_release(struct inode *inode, struct file *file)
{
	struct tz_hvc_log_struct *tz_hvc_log;

	tz_hvc_log = file->private_data;
	mutex_unlock(&tz_hvc_log->lock);

	return 0;
}

static const struct file_operations fops_tz_hvc_log = {
	.open = tz_hvc_log_open,
	.read = tz_hvc_log_read,
	.release = tz_hvc_log_release,
};

static ssize_t tz_smmu_state_read(struct file *fp, char __user *user_buffer,
				size_t count, loff_t *position)
{
	return simple_read_from_buffer(user_buffer, count, position,
				smmu_state, strlen(smmu_state));
}

static const struct file_operations fops_tz_smmu_state = {
	.read = tz_smmu_state_read,
};

static irqreturn_t tzerr_irq(int irq, void *data)
{
	if (paniconaccessviolation) {
		panic("WARN: Access Violation!!!");
	} else {
		pr_emerg_ratelimited("WARN: Access Violation!!!, "
			"Run \"cat /sys/kernel/debug/qti_debug_logs/tz_log\" "
			"for more details \n");
	}
	return IRQ_HANDLED;
}

static int qti_tzlog_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *imem_np;
	struct tz_hvc_log_struct *tz_hvc_log;
	struct dentry *fileret;
	struct page *page_buf;
	bool tz_legacy_scm = false;
	int ret = 0;
	int irq;
	void __iomem *imem_base;

	tz_hvc_log = (struct tz_hvc_log_struct *)
			kzalloc(sizeof(struct tz_hvc_log_struct), GFP_KERNEL);
	if (tz_hvc_log == NULL) {
		dev_err(&pdev->dev, "unable to get tzlog memory\n");
		return -ENOMEM;
	}

	tz_hvc_log->is_diag_id = !(of_device_is_compatible(np, "qti,tzlog-ipq5332") ||
				   of_device_is_compatible(np, "qti,tzlog-ipq6018") ||
				   of_device_is_compatible(np, "qti,tzlog-ipq9574") ||
				   qti_scm_is_tz_log_encryption_supported());
	if (!tz_hvc_log->is_diag_id) {
		imem_np = of_find_compatible_node(NULL, NULL,
						  "qcom,msm-imem-tz-log-buf-addr");
		if (!imem_np) {
			dev_err(&pdev->dev,
				"tz_log_buf_addr imem DT node does not exist\n");
			return -ENODEV;
		}

		imem_base = of_iomap(imem_np, 0);
		if (!imem_base) {
			dev_err(&pdev->dev,
				"tz_log_buf_addr imem offset mapping failed\n");
			return -ENOMEM;
		}

		memcpy_fromio(&tz_hvc_log->log_buf_start, imem_base, 4);
		dev_dbg(&pdev->dev, "Log Buf Start address is 0x%x\n",
			tz_hvc_log->log_buf_start);
		iounmap(imem_base);

		if (qti_scm_is_tz_log_encryption_supported()) {
			tz_hvc_log->is_encrypted = (qti_qfprom_show_authenticate() &&
						    qti_scm_is_tz_log_encrypted());
		}
	}

	tz_hvc_log->tz_kpss = of_property_read_bool(np, "qti,tz_kpss");

	tz_legacy_scm = !is_scm_armv8();

	ret = of_property_read_u32(np, "qti,tz-diag-buf-size",
				   &(tz_hvc_log->buf_len));
	if (ret < 0) {
		dev_err(&pdev->dev, "unable to get diag-buf-size property\n");
		goto free_mem;
	}

	ret = of_property_read_u32(np, "qti,tz-ring-off",
				   &(tz_hvc_log->tz_ring_off));
	if (ret < 0) {
		dev_err(&pdev->dev, "unable to get ring-off property\n");
		goto free_mem;
	}

	ret = of_property_read_u32(np, "qti,tz-log-pos-info-off",
				   &(tz_hvc_log->tz_log_pos_info_off));
	if (ret < 0) {
		dev_err(&pdev->dev, "unable to get log-pos-info-off property\n");
		goto free_mem;
	}

	tz_hvc_log->hvc_ring_off = HVC_DIAG_RING_OFFSET;
	tz_hvc_log->hvc_log_pos_info_off = HVC_DIAG_LOG_POS_INFO_OFFSET;

	/* To support TZ 2.10 */
	if (tz_legacy_scm) {
		tz_hvc_log->tz_ring_off = TZ_LEGACY_RING_OFFSET;
		tz_hvc_log->tz_log_pos_info_off = TZ_LEGACY_LOG_POS_INFO_OFFSET;
		tz_hvc_log->buf_len = TZ_LEGACY_BUF_LEN;
	}

	page_buf = alloc_pages(GFP_KERNEL,
					get_order(tz_hvc_log->buf_len));
	if (page_buf == NULL) {
		dev_err(&pdev->dev, "unable to get data buffer memory\n");
		ret = -ENOMEM;
		goto free_mem;
	}

	tz_hvc_log->ker_buf = page_address(page_buf);

	page_buf = alloc_pages(GFP_KERNEL,
					get_order(tz_hvc_log->buf_len));
	if (page_buf == NULL) {
		dev_err(&pdev->dev, "unable to get copy buffer memory\n");
		ret = -ENOMEM;
		goto free_mem;
	}

	tz_hvc_log->copy_buf = page_address(page_buf);

	mutex_init(&tz_hvc_log->lock);

	tz_hvc_log->debugfs_dir = debugfs_create_dir("qti_debug_logs", NULL);
	if (IS_ERR_OR_NULL(tz_hvc_log->debugfs_dir)) {
		dev_err(&pdev->dev, "unable to create debugfs\n");
		ret = -EIO;
		goto free_mem;
	}

	fileret = debugfs_create_file("tz_log", 0444,  tz_hvc_log->debugfs_dir,
					tz_hvc_log, &fops_tz_hvc_log);
	if (IS_ERR_OR_NULL(fileret)) {
		dev_err(&pdev->dev, "unable to create tz_log debugfs\n");
		ret = -EIO;
		goto remove_debugfs;
	}

	if (!tz_legacy_scm && of_property_read_bool(np, "qti,hvc-enabled")) {
		fileret = debugfs_create_file("hvc_log", 0444,
			tz_hvc_log->debugfs_dir, tz_hvc_log, &fops_tz_hvc_log);
		if (IS_ERR_OR_NULL(fileret)) {
			dev_err(&pdev->dev, "can't create hvc_log debugfs\n");
			ret = -EIO;
			goto remove_debugfs;
		}
	}

	if (of_property_read_bool(np, "qti,get-smmu-state")) {
		ret = qti_scm_get_smmustate();
		switch(ret) {
			case SMMU_DISABLE_NONE:
				smmu_state = "SMMU Stage2 Enabled\n";
				break;
			case SMMU_DISABLE_S2:
				smmu_state = "SMMU Stage2 Bypass\n";
				break;
			case SMMU_DISABLE_ALL:
				smmu_state = "SMMU is Disabled\n";
				break;
			default:
				smmu_state = "Can't detect SMMU State\n";
		}
		pr_notice("TZ SMMU State: %s", smmu_state);

		fileret = debugfs_create_file("tz_smmu_state", 0444,
			tz_hvc_log->debugfs_dir, NULL, &fops_tz_smmu_state);
		if (IS_ERR_OR_NULL(fileret)) {
			dev_err(&pdev->dev, "can't create tz_smmu_state\n");
			ret = -EIO;
			goto remove_debugfs;
		}
	}

	irq = platform_get_irq(pdev, 0);
	if (irq > 0) {
		devm_request_irq(&pdev->dev, irq, tzerr_irq,
				IRQF_ONESHOT, "tzerror", NULL);
	}

	platform_set_drvdata(pdev, tz_hvc_log);

	if (paniconaccessviolation) {
		printk("TZ Log : Will panic on Access Violation, as paniconaccessviolation is set\n");
	} else {
		printk("TZ Log : Will warn on Access Violation, as paniconaccessviolation is not set\n");
	}

	return 0;

remove_debugfs:
	debugfs_remove_recursive(tz_hvc_log->debugfs_dir);
free_mem:
	if (tz_hvc_log->copy_buf)
		__free_pages(virt_to_page(tz_hvc_log->copy_buf),
				get_order(tz_hvc_log->buf_len));

	if (tz_hvc_log->ker_buf)
		__free_pages(virt_to_page(tz_hvc_log->ker_buf),
				get_order(tz_hvc_log->buf_len));

	kfree(tz_hvc_log);

	return ret;
}

static int qti_tzlog_remove(struct platform_device *pdev)
{
	struct tz_hvc_log_struct *tz_hvc_log = platform_get_drvdata(pdev);

	/* removing the directory recursively */
	debugfs_remove_recursive(tz_hvc_log->debugfs_dir);

	if (tz_hvc_log->copy_buf)
		__free_pages(virt_to_page(tz_hvc_log->copy_buf),
				get_order(tz_hvc_log->buf_len));

	if (tz_hvc_log->ker_buf)
		__free_pages(virt_to_page(tz_hvc_log->ker_buf),
				get_order(tz_hvc_log->buf_len));

	kfree(tz_hvc_log);

	return 0;
}

static const struct of_device_id qti_tzlog_of_match[] = {
	{ .compatible = "qti,tzlog" },
	{ .compatible = "qti,tzlog-ipq5332" },
	{ .compatible = "qti,tzlog-ipq6018" },
	{ .compatible = "qti,tzlog-ipq9574" },
	{}
};
MODULE_DEVICE_TABLE(of, qti_tzlog_of_match);

static struct platform_driver qti_tzlog_driver = {
	.probe = qti_tzlog_probe,
	.remove = qti_tzlog_remove,
	.driver  = {
		.name  = "qti_tzlog",
		.of_match_table = qti_tzlog_of_match,
	},
};

module_platform_driver(qti_tzlog_driver);
