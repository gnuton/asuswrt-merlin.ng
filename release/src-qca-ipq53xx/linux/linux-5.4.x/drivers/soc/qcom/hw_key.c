// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/err.h>
#include <linux/key.h>
#include <linux/moduleparam.h>
#include <linux/qcom_scm.h>

#define CONTEXT_SIZE_CMDLINE	257
#define CONTEXT_SIZE		128
#define MAX_KEY_SIZE		32

#define KEYRING_TYPE		"user"
#define KEYRING_DESC_HW_KEY_O	"hw_key_o"
#define KEYRING_DESC_HW_KEY_CR	"hw_key_cr"

static char ctx_s[CONTEXT_SIZE_CMDLINE];
static char ctx_d[CONTEXT_SIZE_CMDLINE];

static int hw_key_cr_len, hw_key_o_len;

extern int look_up_user_keyrings(struct key **, struct key **);

__init int hw_key_gen_store(char *ctx_data, size_t ctx_data_len,
			    char *ctx_salt, size_t ctx_salt_len,
			    const char *type, const char *description,
			    u32 hw_key_len)
{
	key_ref_t key_ref, keyring_ref;
	struct key *user_key_ref;
	uint8_t hw_key[MAX_KEY_SIZE * 2] = {0};
	int key_len = 0;
	int ret;

	if (hw_key_len != 16 && hw_key_len != 32)
		return -EINVAL;

	ret = qti_scm_derive_and_share_key(QTI_SCM_DERIVE_KEY,
			QTI_SCM_DERIVE_KEY_PARAM_ID, hw_key_len,
			ctx_data, (u32)ctx_data_len, hw_key, hw_key_len);
	if (ret < 0)
		goto error;

	key_len = hw_key_len;

	/* If ctx_data_len = 0, derive single key */
	if (ctx_data_len > 0 && ctx_salt_len > 0) {
		ret = qti_scm_derive_and_share_key(QTI_SCM_DERIVE_KEY,
				QTI_SCM_DERIVE_KEY_PARAM_ID, hw_key_len,
				ctx_salt, (u32)ctx_salt_len, &hw_key[key_len],
				hw_key_len);
		if (ret < 0)
			goto error;

		key_len += hw_key_len;
	}

	ret = look_up_user_keyrings(&user_key_ref, NULL);
	if (ret < 0)
		goto error;

	keyring_ref = make_key_ref(user_key_ref, 1);
	if (IS_ERR(keyring_ref)) {
		ret = PTR_ERR(keyring_ref);
		return ret;
	}

	key_ref = key_create_or_update(keyring_ref, type, description,
				       hw_key, key_len, KEY_PERM_UNDEF,
				       KEY_ALLOC_IN_QUOTA);
	if (!IS_ERR(key_ref)) {
		key_ref_put(key_ref);
		ret = 0;
	} else {
		ret = PTR_ERR(key_ref);
	}

	key_ref_put(keyring_ref);
error:
	return ret;
}

static __init int str_to_hex(const char *in, int in_len, char *out)
{
	int i = 0;

	for (i = 0; i < in_len/2; i++) {
		sscanf(in, "%2hhx", &out[i]);
		in += 2;
	}

	return i;
}

static __init int tmel_hw_key_init(void)
{
	uint8_t context_salt[CONTEXT_SIZE] = {0};
	uint8_t context_data[CONTEXT_SIZE] = {0};
	size_t ctx_d_len = strlen(ctx_d);
	size_t ctx_s_len = strlen(ctx_s);
	size_t context_data_len = 0;
	size_t context_salt_len = 0;
	int ret = 0;

	/*
	 * Context to generate salt key is required only for XTS mode but the
	 * context to generate key for data is mandatory.
	 */
	if (ctx_d_len == 0) {
		pr_err("Context is not provided, skipping key init");
		return ret;
	}

	/*
	 * Convert the hex string to hex-value
	 */
	context_data_len = str_to_hex(ctx_d, ctx_d_len, context_data);
	if (ctx_s_len != 0) {
		context_salt_len = str_to_hex(ctx_s, ctx_s_len, context_salt);
	}

	ret = hw_key_gen_store(context_data, context_data_len, context_salt,
			       context_salt_len, KEYRING_TYPE,
			       KEYRING_DESC_HW_KEY_O,
			       (u32) hw_key_o_len);
	if (ret) {
		pr_err("Failed to generate key for given context, error = 0x%x",
		       ret);
		return ret;
	}

	ret = hw_key_gen_store(NULL, 0, NULL, 0, KEYRING_TYPE,
			       KEYRING_DESC_HW_KEY_CR,
			       (u32) hw_key_cr_len);
	if (ret) {
		pr_err("Failed to generate key for NULL context, error = 0x%x",
		       ret);
		return ret;
	}

	return ret;
}

module_param_string(ctx_s, ctx_s, sizeof(ctx_s), 0);
module_param_string(ctx_d, ctx_d, sizeof(ctx_d), 0);

module_param(hw_key_o_len, int, 0);
module_param(hw_key_cr_len, int, 0);

device_initcall(tmel_hw_key_init);
