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

#include <linux/qcom_scm.h>
#include "qwes.h"

dev_t dev;
static struct class *dev_class;
static struct cdev qwes_cdev;

/*
 ** File operation sturcture
 */
static struct file_operations fops =
{
	.owner          = THIS_MODULE,
	.open           = qwes_open,
	.unlocked_ioctl = qwes_ioctl,
	.release        = qwes_release,
};

/*
 ** This function will be called when we open the Device file
 */
static int qwes_open(struct inode *inode, struct file *file)
{
	return 0;
}

/*
 ** This function will be called when we close the Device file
 */
static int qwes_release(struct inode *inode, struct file *file)
{
	return 0;
}

/*
 ** This function be called when we write IOCTL on the Device file
 */
static long qwes_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	void *key_buf = NULL;
	void *req_buf = NULL;
	void *claim_buf = NULL;
	void *resp_buf = NULL;
	u32 *key_len = NULL;
	u32 *resp_size = NULL;
	void __user *argp = (void __user *)arg;
	struct get_key key;
	struct attest_resp ar;
	struct prov_resp pr;
	int ret = 0;

	switch(cmd) {
		case GET_KEY:
			ret = copy_from_user(&key, argp, sizeof(struct get_key));
			if (ret) {
				pr_err("Data Write : Err!\n");
				return ret;
			}
			key_buf = kzalloc(QWES_M3_KEY_BUFF_MAX_SIZE, GFP_KERNEL);
			if (!key_buf) {
				pr_err("Memory allocation failed for key buffer\n");
				return -ENOMEM;
			}
			key_len = kzalloc(sizeof(u32), GFP_KERNEL);
			if (!key_len) {
				pr_err("Memory allocation failed for key length\n");
				ret = -ENOMEM;
				goto key_buf_alloc_err;
			}

			ret = qti_scm_get_device_attestation_ephimeral_key(QWES_SVC_ID,
					QWES_INIT_ATTEST, key_buf, QWES_M3_KEY_BUFF_MAX_SIZE, key_len);
			if (ret) {
				pr_err("qwes init attestatio scm failed\n");
				goto key_len_alloc_err;
			}
			key.len  = *key_len;

			ret = copy_to_user(key.buf, key_buf, key.len);
			if (ret) {
                                pr_err("Error : Key buf data read failed\n");
				goto key_len_alloc_err;
                        }
			ret = copy_to_user(argp, &key, sizeof(struct get_key));
			if (ret) {
				pr_err("Error : Structure get_key data read failed\n");
			}
		key_len_alloc_err:
			kfree(key_len);
		key_buf_alloc_err:
			kfree(key_buf);
			break;

		case ATTEST_RESP:
			ret =  copy_from_user(&ar, argp, sizeof(struct attest_resp));
			if (ret) {
				pr_err("Error : attest_resp structure data write failed\n");
				return ret;
			}
			if (ar.req_buf_len > QWES_RESP_BUFF_MAX_SIZE ||
					ar.claim_buf_len > QWES_RESP_BUFF_MAX_SIZE) {
				pr_err("Error : Requested attestation buffer is invalid\n");
				return -EINVAL;
			}
			req_buf = kzalloc(ar.req_buf_len, GFP_KERNEL);
			if (!req_buf) {
				pr_err("Memory allocation failed for req buffer\n");
				return -ENOMEM;
			}

			resp_size = kzalloc(sizeof(u32), GFP_KERNEL);
			if (!resp_size) {
				pr_err("Memory allocation failed for attest resp length\n");
				ret = -ENOMEM;
				goto req_buf_alloc_err;
			}

			resp_buf = kzalloc(QWES_RESP_BUFF_MAX_SIZE, GFP_KERNEL);
			if (!resp_buf) {
				pr_err("Memory allocation failed for resp buffer\n");
				ret = -ENOMEM;
				goto resp_size_alloc_err;
			}

			if (ar.req_buf == NULL || ar.req_buf_len <= 0) {
				pr_err("Error : attest req buf data is Null!\n");
				ret = -EINVAL;
				goto resp_buf_alloc_err;
			}
			ret = copy_from_user(req_buf, ar.req_buf, ar.req_buf_len);
			if (ret) {
				pr_err("Error : attest req buf data write failed !\n");
				goto resp_buf_alloc_err;
			}
			if(ar.claim_buf != NULL && ar.claim_buf_len > 0) {
				claim_buf = kzalloc(ar.claim_buf_len, GFP_KERNEL);
				if (!claim_buf) {
					pr_err("Memory allocation failed for claim buffer\n");
					ret = -ENOMEM;
					goto resp_buf_alloc_err;
				}
				ret = copy_from_user(claim_buf, ar.claim_buf, ar.claim_buf_len);
				if (ret) {
					pr_err("Error : External Claim buf Data Write Failed !\n");
					goto claim_buf_alloc_err;
				}
			}
			ret = qti_scm_get_device_attestation_response(QWES_SVC_ID, QWES_ATTEST_REPORT,
					req_buf, ar.req_buf_len, claim_buf, ar.claim_buf_len, resp_buf,
					QWES_RESP_BUFF_MAX_SIZE, resp_size);
			if (ret) {
				pr_err("qwes attestation response scm failed\n");
				goto claim_buf_alloc_err;
			}
			ar.resp_buf_len = *resp_size;
			ret = copy_to_user(ar.resp_buf, resp_buf, ar.resp_buf_len);
			if (ret) {
				pr_err("Error : resp buf data read failed\n");
				goto claim_buf_alloc_err;
			}
			ret = copy_to_user(argp, &ar, sizeof(ar));
			if (ret) {
				pr_err("Error : Structure attest_resp data read failed\n");
			}

		claim_buf_alloc_err:
			if (claim_buf != NULL)
				kfree(claim_buf);
		resp_buf_alloc_err:
			kfree(resp_buf);
		resp_size_alloc_err:
			kfree(resp_size);
		req_buf_alloc_err:
			kfree(req_buf);
			break;

		case PROV_RESP:
			ret =  copy_from_user(&pr ,argp, sizeof(struct prov_resp));
			if (ret) {
				pr_err("Error : prov_resp structure data write failed\n");
				return -EINVAL;
			}

			req_buf = kzalloc(pr.req_buf_len, GFP_KERNEL);
			if (!req_buf) {
				pr_err("Memory allocation failed for prov req buffer\n");
				return -EINVAL;
			}
			resp_size = kzalloc(sizeof(u32), GFP_KERNEL);
			if (!resp_size) {
				pr_err("Memory allocation failed for prov resp length\n");
				ret = -ENOMEM;
				goto prov_req_buf_alloc_err;
			}

			resp_buf = kzalloc(QWES_RESP_BUFF_MAX_SIZE, GFP_KERNEL);
			if (!resp_buf) {
				pr_err("Memory allocation failed for prov resp buffer\n");
				ret = -ENOMEM;
				goto prov_resp_size_alloc_err;
			}
			if (pr.provreq_buf == NULL || pr.req_buf_len <= 0) {
				pr_err("Error : prov req buf is Null !\n");
				ret = -EINVAL;
				goto prov_resp_buf_alloc_err;
			}
			ret = copy_from_user(req_buf, pr.provreq_buf, pr.req_buf_len);
			if (ret) {
				pr_err("Error : prov req buf data write failed !\n");
				goto prov_resp_buf_alloc_err;
			}
			ret = qti_scm_get_device_provision_response(QWES_SVC_ID,
					QWES_DEVICE_PROVISION, req_buf,
					pr.req_buf_len, resp_buf,
					QWES_RESP_BUFF_MAX_SIZE, resp_size);
			if (ret) {
				pr_err("qwes provision response scm failed\n");
				goto prov_resp_buf_alloc_err;
			}
			pr.resp_buf_len = *resp_size;
			ret = copy_to_user(pr.provresp_buf, resp_buf, *resp_size);
			if (ret) {
				pr_err("Error : prov resp buf data read failed\n");
				goto prov_resp_buf_alloc_err;
			}
			ret = copy_to_user(argp, &pr, sizeof(pr));
			if (ret) {
				pr_err("Error : Structure prov_resp data read failed\n");
			}

		prov_resp_buf_alloc_err:
			kfree(resp_buf);
		prov_resp_size_alloc_err:
                        kfree(resp_size);
                prov_req_buf_alloc_err:
                        kfree(req_buf);
			break;
		default:
			pr_info("Warning : Invalid Command..!\n");
			return 0;
	}
	return ret;
}

/*
 ** Module Init function
 */
static int __init qwes_driver_init(void)
{
	long ret;
	struct device *device;

	/*Allocating Major number*/
	ret = alloc_chrdev_region(&dev, 0, 1, "qwes_dev");
	if (ret < 0) {
		pr_err("Cannot allocate major number\n");
		return ret;
	}

	/*Creating cdev structure*/
	cdev_init(&qwes_cdev,&fops);

	/*Adding character device to the system*/
	ret = cdev_add(&qwes_cdev,dev,1);
	if (ret < 0) {
		pr_err("Cannot add the device to the system\n");
		goto r_class;
	}

	/*Creating struct class*/
	dev_class = class_create(THIS_MODULE,"qwes_class");
	if (IS_ERR(dev_class)){
		pr_err("Cannot create the struct class\n");
		ret = PTR_ERR(dev_class);
		goto r_class;
	}

	/*Creating device*/
	device = device_create(dev_class,NULL,dev,NULL,"qwes_device");
	if (IS_ERR(device)) {
		pr_err("Cannot create the Device\n");
		ret =  PTR_ERR(device);
		goto r_device;
	}
	return 0;

r_device:
	class_destroy(dev_class);

r_class:
	unregister_chrdev_region(dev,1);

	return ret;
}

/*
 ** Module exit function
 */
static void __exit qwes_driver_exit(void)
{
	device_destroy(dev_class,dev);
	class_destroy(dev_class);
	cdev_del(&qwes_cdev);
	unregister_chrdev_region(dev, 1);
}

module_init(qwes_driver_init);
module_exit(qwes_driver_exit);

MODULE_DESCRIPTION("QWES device driver");
