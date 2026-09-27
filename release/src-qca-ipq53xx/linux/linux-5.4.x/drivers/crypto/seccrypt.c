//SPDX-License-Identifier: GPL-2.0 only
/*
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <crypto/aes.h>
#include <crypto/internal/skcipher.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <crypto/scatterwalk.h>
#include <linux/err.h>
#include <linux/qcom_scm.h>

#define SEC_MAX_KEY_SIZE	64
/* Cipher algorithm */
#define SEC_ALG_AES		BIT(0)
/* cipher modes */
#define SEC_MODE_CBC		BIT(1)
#define SEC_MODE_ECB		BIT(2)
#define SEC_MODE_CTR		BIT(3)

#define IS_CBC(mode)                    (mode & SEC_MODE_CBC)
#define IS_ECB(mode)                    (mode & SEC_MODE_ECB)
#define IS_CTR(mode)                    (mode & SEC_MODE_CTR)

#define MODE_CBC	1
#define MODE_ECB	0
#define MODE_CTR	2

struct sec_config_key_sec {
	uint32_t keylen;
}__attribute__((packed));

struct secure_nand_aes_cmd {
	u64 direction;
	u64 mode;
	u64 *iv_buf;
	u64 iv_size;
	u64 *req_buf;
	u64 reqlen;
	u64 *rsp_buf;
	u64 rsplen;
};

struct sec_cipher_ctx {
	u8 enc_key[SEC_MAX_KEY_SIZE];
	unsigned int enc_keylen;
	struct crypto_skcipher *fallback;
};

struct sec_crypt_device {
	struct secure_nand_aes_cmd *cptr;
	struct device *dev;
	struct kobject kobj;
	struct kobject *kobj_parent;
	spinlock_t lock;
	bool fallback_tz;
};

/*
* struct sec_cipher_reqctx - holds private cipher objects per request
* @flags: operation flags
* @iv: pointer to the IV
* @ivsize: IV size
* @src_nents: source entries
* @dst_nents: destination entries
* @result_sg: scatterlist used for result buffer
* @dst_tbl: destination sg table
* @dst_sg: destination sg pointer table beginning
* @src_tbl: source sg table
* @src_sg: source sg pointer table beginning;
* @cryptlen: crypto length
*/
struct sec_cipher_reqctx {
	unsigned long flags;
	u8 *iv;
	unsigned int ivsize;
	int src_nents;
	int dst_nents;
	struct scatterlist result_sg;
	struct sg_table dst_tbl;
	struct scatterlist *dst_sg;
	struct sg_table src_tbl;
	struct scatterlist *src_sg;
	unsigned int cryptlen;
	struct skcipher_request fallback_req;   // keep at the end
 };

struct sec_skcipher_def {
	unsigned long flags;
	const char *name;
	const char *drv_name;
	unsigned int blocksize;
	unsigned int chunksize;
	unsigned int ivsize;
	unsigned int min_keysize;
	unsigned int max_keysize;
	unsigned int walksize;
};

struct sec_alg_template {
	struct list_head entry;
	u32 crypto_alg_type;
	unsigned long alg_flags;
	union {
		struct skcipher_alg skcipher;
	} alg;

	struct sec_crypt_device *sec;
};

static LIST_HEAD(skcipher_algs);

static DEFINE_MUTEX(seccrypt_spinlock);

static int seccrypt_setkey_sec(unsigned int keylen)
{
	struct sec_config_key_sec key;

	key.keylen = keylen;
	return qti_set_qcekey_sec(&key, sizeof(struct sec_config_key_sec));
}

static inline struct sec_alg_template *sec_to_cipher_tmpl(struct crypto_skcipher *tfm)
{
	struct skcipher_alg *alg = crypto_skcipher_alg(tfm);
	return container_of(alg, struct sec_alg_template, alg.skcipher);

	return NULL;
}

static int sec_skcipher_setkey(struct crypto_skcipher *tfm, const u8 *key,
			 unsigned int keylen)
{
	struct sec_cipher_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct sec_alg_template *tmpl = sec_to_cipher_tmpl(tfm);
	struct sec_crypt_device *sec = tmpl->sec;
	int ret;

	if (!key || !keylen)
		return -EINVAL;

	if (sec->fallback_tz) {
		ret = seccrypt_setkey_sec(keylen);
		if (ret) {
			pr_info("Error in setting Key by tz\n");
			return ret;
		}
	}

	memcpy(ctx->enc_key, key, keylen);

	ret = crypto_skcipher_setkey(ctx->fallback, key, keylen);
	if (!ret)
		ctx->enc_keylen = keylen;

	return ret;
}

static int seccrypt_tz(struct skcipher_request *req,
			struct sec_alg_template *tmpl, int encrypt)
{
	struct sec_crypt_device *sec = tmpl->sec;
	struct secure_nand_aes_cmd *cptr = sec->cptr;
	struct device *dev;
	struct skcipher_walk walk;
	int err = 0, ivsize = 0, reqlen = 0, flag = 0;
	u8 *src, *dst, *iv_buf;
	dma_addr_t phy_src = 0, phy_dst = 0, phy_iv = 0;
	dev = sec->dev;
	flag = tmpl->alg_flags;

	mutex_lock(&seccrypt_spinlock);
	err = skcipher_walk_virt(&walk, req, false);

	src = walk.src.virt.addr;
	dst = walk.dst.virt.addr;
	ivsize = walk.ivsize;
	iv_buf = walk.iv;
	reqlen = walk.total;

	if (src) {
		phy_src = dma_map_single(dev, src, reqlen, DMA_TO_DEVICE);
		if (dma_mapping_error(dev, phy_src)) {
			dev_err(dev, "failed to DMA MAP phy_src buffer\n");
			return -EIO;
		}
	}

	if (dst) {
		phy_dst = dma_map_single(dev, dst, reqlen, DMA_FROM_DEVICE);
		if (dma_mapping_error(dev, phy_dst)) {
			dev_err(dev, "failed to DMA MAP phy_dst buffer\n");
			return -EIO;
		}
	}

	if (iv_buf) {
		phy_iv = dma_map_single(dev, iv_buf, AES_BLOCK_SIZE, DMA_TO_DEVICE);
		if (dma_mapping_error(dev, phy_iv)) {
			dev_err(dev, "failed to DMA MAP phy_iv buffer\n");
			return -EIO;
		}
	}

	/* Fill the structure to pass to TZ */
	cptr->direction = encrypt;
	if (IS_CBC(flag))
		cptr->mode = MODE_CBC;
	else if (IS_ECB(flag))
		cptr->mode = MODE_ECB;
	else if (IS_CTR(flag))
		cptr->mode = MODE_CTR;
	cptr->iv_buf = (u64 *)phy_iv;
	cptr->iv_size = AES_BLOCK_SIZE;
	cptr->req_buf = (u64 *)phy_src;
	cptr->reqlen = reqlen;
	cptr->rsp_buf = (u64 *)phy_dst;
	cptr->rsplen = reqlen;

	err = qti_sec_crypt(cptr, sizeof(struct secure_nand_aes_cmd));
	if (err) {
		dev_err(dev, "enc|dec smc call failed :%d\n", err);
	}

	if (phy_src)
		dma_unmap_single(dev, phy_src, reqlen, DMA_TO_DEVICE);
	if (phy_dst)
		dma_unmap_single(dev, phy_dst, reqlen, DMA_FROM_DEVICE);
	if (phy_iv)
		dma_unmap_single(dev, phy_iv, AES_BLOCK_SIZE, DMA_TO_DEVICE);

	err = skcipher_walk_done(&walk, 0);

	mutex_unlock(&seccrypt_spinlock);
	return err;
}

static int crypto_skcipher_decrypt_tz(struct skcipher_request *req,
					struct sec_alg_template *tmpl, int encrypt)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct crypto_alg *alg = tfm->base.__crt_alg;
	unsigned int cryptlen = req->cryptlen;
	int ret;
	crypto_stats_get(alg);

	if (crypto_skcipher_get_flags(tfm) & CRYPTO_TFM_NEED_KEY)
		ret = -ENOKEY;
	else
		ret = seccrypt_tz(req, tmpl, encrypt);

	crypto_stats_skcipher_decrypt(cryptlen, ret, alg);
	return ret;
}

static int crypto_skcipher_encrypt_tz(struct skcipher_request *req,
					struct sec_alg_template *tmpl, int encrypt)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct crypto_alg *alg = tfm->base.__crt_alg;
	unsigned int cryptlen = req->cryptlen;
	int ret;
	crypto_stats_get(alg);

	if (crypto_skcipher_get_flags(tfm) & CRYPTO_TFM_NEED_KEY)
		ret = -ENOKEY;
	else
		ret = seccrypt_tz(req, tmpl, encrypt);

	crypto_stats_skcipher_encrypt(cryptlen, ret, alg);
	return ret;
}

static int sec_skcipher_crypt(struct skcipher_request *req, int encrypt)
{
	int ret;
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct sec_cipher_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct sec_cipher_reqctx *rctx = skcipher_request_ctx(req);
	struct sec_alg_template *tmpl = sec_to_cipher_tmpl(tfm);
	struct sec_crypt_device *sec = tmpl->sec;

	skcipher_request_set_tfm(&rctx->fallback_req, ctx->fallback);
	skcipher_request_set_callback(&rctx->fallback_req,
					req->base.flags,
					req->base.complete,
					req->base.data);
	skcipher_request_set_crypt(&rctx->fallback_req, req->src,
				req->dst, req->cryptlen, req->iv);
	if (sec->fallback_tz)
		ret = encrypt ? crypto_skcipher_encrypt_tz(&rctx->fallback_req, tmpl, encrypt) :
			crypto_skcipher_decrypt_tz(&rctx->fallback_req, tmpl, encrypt);
	else
		ret = encrypt ? crypto_skcipher_encrypt(&rctx->fallback_req) :
			crypto_skcipher_decrypt(&rctx->fallback_req);

	return ret;
}

static int sec_skcipher_encrypt(struct skcipher_request *req)
{
	sec_skcipher_crypt(req, 1);

	return 0;
}

static int sec_skcipher_decrypt(struct skcipher_request *req)
{
	sec_skcipher_crypt(req, 0);

	return 0;
}

static int sec_skcipher_init_fallback(struct crypto_skcipher *tfm)
{
	struct sec_cipher_ctx *ctx = crypto_skcipher_ctx(tfm);

	ctx->fallback = crypto_alloc_skcipher(crypto_tfm_alg_name(&tfm->base),
			0, CRYPTO_ALG_NEED_FALLBACK);

	if (IS_ERR(ctx->fallback))
		return PTR_ERR(ctx->fallback);

	crypto_skcipher_set_reqsize(tfm, sizeof(struct sec_cipher_reqctx) +
						crypto_skcipher_reqsize(ctx->fallback));

	return 0;
}

static void sec_skcipher_exit(struct crypto_skcipher *tfm)
{
	 struct sec_cipher_ctx *ctx = crypto_skcipher_ctx(tfm);

	 crypto_free_skcipher(ctx->fallback);
}

static const struct sec_skcipher_def skcipher_def[] = {
	{
		.flags          = SEC_ALG_AES | SEC_MODE_ECB,
		.name           = "ecb(aes)",
		.drv_name       = "ecb-aes-qce",
		.blocksize      = AES_BLOCK_SIZE,
		.ivsize         = 0,
		.min_keysize    = AES_MIN_KEY_SIZE,
		.max_keysize    = AES_MAX_KEY_SIZE,
	},
	{
		.flags          = SEC_ALG_AES | SEC_MODE_CBC,
		.name           = "cbc(aes)",
		.drv_name       = "cbc-aes-qce",
		.blocksize      = AES_BLOCK_SIZE,
		.ivsize         = AES_BLOCK_SIZE,
		.min_keysize    = AES_MIN_KEY_SIZE,
		.max_keysize    = AES_MAX_KEY_SIZE,
	},
	{
		.flags          = SEC_ALG_AES | SEC_MODE_CTR,
		.name           = "ctr(aes)",
		.drv_name       = "ctr-aes-qce",
		.blocksize      = AES_BLOCK_SIZE,
		.ivsize         = AES_BLOCK_SIZE,
		.min_keysize    = AES_MIN_KEY_SIZE,
		.max_keysize    = AES_MAX_KEY_SIZE,
	},
#ifndef CONFIG_ARM64
	{
		.flags          = 0,
		.name           = "cts(cbc(aes))",
		.drv_name       = "cts-cbc-aes-qce",
		.blocksize      = AES_BLOCK_SIZE,
		.ivsize         = AES_BLOCK_SIZE,
		.walksize       = 2 * AES_BLOCK_SIZE,
		.min_keysize    = AES_MIN_KEY_SIZE,
		.max_keysize    = AES_MAX_KEY_SIZE,
	},
#endif
	{
		.flags          = 0,
		.name           = "xts(aes)",
		.drv_name       = "xts-aes-qce",
		.blocksize      = AES_BLOCK_SIZE,
		.ivsize         = AES_BLOCK_SIZE,
		.walksize       = 2 * AES_BLOCK_SIZE,
		.min_keysize    = 2 * AES_MIN_KEY_SIZE,
		.max_keysize    = 2 * AES_MAX_KEY_SIZE,
	},
};

static int sec_skcipher_register_one(const struct sec_skcipher_def *def, struct sec_crypt_device *sec)
{
	struct sec_alg_template *tmpl;
	struct skcipher_alg *alg;
	int ret;

	tmpl = kzalloc(sizeof(*tmpl), GFP_KERNEL);
	if (!tmpl)
		return -ENOMEM;

	alg = &tmpl->alg.skcipher;

	snprintf(alg->base.cra_name, CRYPTO_MAX_ALG_NAME, "%s", def->name);
	snprintf(alg->base.cra_driver_name, CRYPTO_MAX_ALG_NAME, "%s",
		def->drv_name);

	alg->base.cra_blocksize         = def->blocksize;
	alg->chunksize                  = def->chunksize;
	alg->ivsize                     = def->ivsize;
	alg->walksize                   = def->walksize;
	alg->min_keysize                = def->min_keysize;
	alg->max_keysize                = def->max_keysize;
	alg->setkey			= sec_skcipher_setkey;
	alg->encrypt                    = sec_skcipher_encrypt;
	alg->decrypt                    = sec_skcipher_decrypt;
	alg->base.cra_priority          = 300;
	alg->base.cra_flags		= CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_NEED_FALLBACK;
	alg->base.cra_ctxsize           = sizeof(struct sec_cipher_ctx);
	alg->base.cra_alignmask         = 0;
	alg->base.cra_module            = THIS_MODULE;
	alg->init			= sec_skcipher_init_fallback;
	alg->exit               	= sec_skcipher_exit;

	INIT_LIST_HEAD(&tmpl->entry);

	tmpl->crypto_alg_type = CRYPTO_ALG_TYPE_SKCIPHER;
	tmpl->alg_flags = def->flags;
	tmpl->sec = sec;

	ret = crypto_register_skcipher(alg);
	if (ret) {
		dev_err(sec->dev, "%s registration failed\n", alg->base.cra_name);
		kfree(tmpl);
		return ret;
	}

	list_add_tail(&tmpl->entry, &skcipher_algs);

	dev_dbg(sec->dev, "%s is registered\n", alg->base.cra_name);
	return 0;
}

static void sec_skcipher_unregister(struct device *dev)
{
	struct sec_alg_template *tmpl, *n;
	list_for_each_entry_safe(tmpl, n, &skcipher_algs, entry) {
		crypto_unregister_skcipher(&tmpl->alg.skcipher);
		list_del(&tmpl->entry);
		kfree(tmpl);
	}
}

static void seccrypt_sysfs_deinit(struct sec_crypt_device *sec)
{
	kobject_del(&sec->kobj);
	kobject_del(sec->kobj_parent);
}

#define to_seccryptdev(k) container_of(k, struct sec_crypt_device, kobj)

static ssize_t tz_fallback_show(struct kobject *kobj,
		struct attribute *attr, char *buf)
{
	struct sec_crypt_device *sec = to_seccryptdev(kobj);

	return scnprintf(buf, sizeof(int), "%d\n", sec->fallback_tz);
}

static ssize_t tz_fallback_store(struct kobject *kobj,
		struct attribute *attr, const char *buf, size_t count)
{
	int use_fixed_key;
	struct sec_crypt_device *sec = to_seccryptdev(kobj);

	sscanf(buf, "%du", &use_fixed_key);
	if (use_fixed_key == 1) {
		sec->fallback_tz = true;
	} else {
		if (qti_seccrypt_clearkey() < 0)
			pr_err("error in clearing key.\n");
		sec->fallback_tz = false;
	}
	return count;
}

static struct attribute seccrypt_tz_fallback_attribute = {
	.name = "fixed_sec_key",
	.mode = 0660,
};

static struct attribute *seccrypt_attrs[] = {
	&seccrypt_tz_fallback_attribute,
	NULL
};

static struct sysfs_ops seccrypt_sysfs_ops = {
	.show = tz_fallback_show,
	.store = tz_fallback_store,
};

static struct kobj_type seccrypt_ktype = {
	.sysfs_ops = &seccrypt_sysfs_ops,
	.default_attrs = seccrypt_attrs,
};

static int sec_sysfs_init(struct sec_crypt_device *sec)
{
	int ret;

	sec->kobj_parent = kobject_create_and_add("crypto", kernel_kobj);
	if (!sec->kobj_parent)
		return -ENOMEM;

	ret = kobject_init_and_add(&sec->kobj, &seccrypt_ktype, sec->kobj_parent,
				"%s", "seccrypt");
	if (ret)
		kobject_del(sec->kobj_parent);

	return ret;
}

static int seccrypt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sec_crypt_device *sec;
	int ret, i;

	sec = devm_kzalloc(dev, sizeof(*sec), GFP_KERNEL);
	if (!sec) {
		pr_err("%s : Error in allocating memory\n",__func__);
		return -ENOMEM;
	}

	sec->dev = dev;
	platform_set_drvdata(pdev, sec);

	sec->cptr = devm_kzalloc(dev, sizeof(struct secure_nand_aes_cmd), GFP_KERNEL);
	if (!sec->cptr) {
		pr_err("%s : Error in allocating memory\n",__func__);
		return -ENOMEM;
	}

	if (device_property_read_bool(dev, "seccrypt,fallback_tz"))
		sec->fallback_tz = true;

	for (i = 0; i < ARRAY_SIZE(skcipher_def); i++) {
		ret =  sec_skcipher_register_one(&skcipher_def[i], sec);
		if (ret)
			goto err;
	}

	ret = sec_sysfs_init(sec);
	if (ret)
		goto err;

	return 0;
err:
	sec_skcipher_unregister(dev);
	return ret;
}

static int seccrypt_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sec_crypt_device *sec = platform_get_drvdata(pdev);
	sec_skcipher_unregister(dev);
	seccrypt_sysfs_deinit(sec);
	return 0;
}

static const struct of_device_id seccrypt_of_match[] = {
	{ .compatible = "qcom,seccrypt", },
	{}
};
MODULE_DEVICE_TABLE(of, seccrypt_of_match);

static struct platform_driver seccrypt_driver = {
	.probe = seccrypt_probe,
	.remove = seccrypt_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = seccrypt_of_match,
	},
};
module_platform_driver(seccrypt_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Qualcomm Technologies Inc. seccrypt driver");
MODULE_ALIAS("platform:" KBUILD_MODNAME);
MODULE_AUTHOR("Md Sadre Alam <quic_mdalam@quicinc.com>");
