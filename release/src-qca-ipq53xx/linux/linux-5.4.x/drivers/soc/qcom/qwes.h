/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#define QWES_SVC_ID    0x1E
#define QWES_INIT_ATTEST 0x1
#define QWES_ATTEST_REPORT 0x2
#define QWES_DEVICE_PROVISION 0x3
#define QWES_M3_KEY_BUFF_MAX_SIZE 128
#define QWES_RESP_BUFF_MAX_SIZE 2048
#define MAX_SIZE_OF_KEY_BUF_LEN 255

struct prov_resp {
	void *provreq_buf;
	void *provresp_buf;
	unsigned int req_buf_len;
	unsigned int resp_buf_len;
};

struct attest_resp {
	void *req_buf;
	void *claim_buf;
	void *resp_buf;
	unsigned int req_buf_len;
	unsigned int claim_buf_len;
	unsigned int resp_buf_len;
};

struct get_key {
	void *buf;
	unsigned int len;
};

#define GET_KEY		_IOWR('a', 'a', struct get_key)
#define ATTEST_RESP	_IOWR('a', 'b', struct attest_resp)
#define PROV_RESP	_IOWR('a', 'c', struct prov_resp)

static int      __init qwes_driver_init(void);
static void     __exit qwes_driver_exit(void);
static int      qwes_open(struct inode *inode, struct file *file);
static int      qwes_release(struct inode *inode, struct file *file);
static long     qwes_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
