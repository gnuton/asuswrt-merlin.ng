/* Copyright (c) 2015-2016, 2020, The Linux Foundation. All rights reserved.
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
 *
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/qcom_scm.h>
#include "qcom_scm.h"

#define NON_SECURE_WATCHDOG		0x1
#define AHB_TIMEOUT			0x3
#define NOC_ERROR			0x6
#define SYSTEM_RESET_OR_REBOOT		0x10
#define POWER_ON_RESET			0x20
#define SECURE_WATCHDOG			0x23
#define HLOS_PANIC			0x47
#define VFSM_RESET			0x68
#define TME_L_FATAL_ERROR		0x49
#define TME_L_WDT_BITE_FATAL_ERROR	0x69

#define RESET_REASON_MSG_MAX_LEN	100

static int dload_dis;

static void scm_restart_dload_mode_enable(void)
{
	if (!dload_dis) {
		unsigned int magic_cookie = SET_MAGIC;
		qti_scm_dload(QCOM_SCM_SVC_BOOT, SCM_CMD_TZ_FORCE_DLOAD_ID,
				&magic_cookie);
	}
}

static void scm_restart_dload_mode_disable(void)
{
	unsigned int magic_cookie = CLEAR_MAGIC;

	qti_scm_dload(QCOM_SCM_SVC_BOOT, SCM_CMD_TZ_FORCE_DLOAD_ID,
			&magic_cookie);
};

static void scm_restart_sdi_disable(void)
{
	qti_scm_sdi(QCOM_SCM_SVC_BOOT, SCM_CMD_TZ_CONFIG_HW_FOR_RAM_DUMP_ID);
}

static void scm_restart_abnormal_magic_disable(void)
{
	unsigned int magic_cookie = CLEAR_ABNORMAL_MAGIC;

	qti_scm_dload(QCOM_SCM_SVC_BOOT, SCM_CMD_TZ_FORCE_DLOAD_ID,
			&magic_cookie);
};

static int scm_restart_panic(struct notifier_block *this,
	unsigned long event, void *data)
{
	scm_restart_dload_mode_enable();
	scm_restart_sdi_disable();

	return NOTIFY_DONE;
}

static struct notifier_block panic_nb = {
	.notifier_call = scm_restart_panic,
};

static int scm_restart_reason_reboot(struct notifier_block *nb,
				unsigned long action, void *data)
{
	scm_restart_sdi_disable();
	scm_restart_dload_mode_disable();
	scm_restart_abnormal_magic_disable();

	return NOTIFY_DONE;
}

static struct notifier_block reboot_nb = {
	.notifier_call = scm_restart_reason_reboot,
	.priority = INT_MIN,
};

static const struct of_device_id scm_restart_reason_match_table[] = {
	{ .compatible = "qti,scm_restart_reason",
	  .data = (void *)CLEAR_MAGIC,
	},
	{ .compatible = "qti_ipq5018,scm_restart_reason",
	  .data = (void *)ABNORMAL_MAGIC,
	},
	{ .compatible = "qti_ipq6018,scm_restart_reason",
	  .data = (void *)ABNORMAL_MAGIC,
	},
	{ .compatible = "qti_ipq9574,scm_restart_reason",
	  .data = (void *)ABNORMAL_MAGIC,
	},
	{ .compatible = "qti_ipq5332,scm_restart_reason",
	  .data = (void *)ABNORMAL_MAGIC,
	},
	{}
};
MODULE_DEVICE_TABLE(of, scm_restart_reason_match_table);

static int restart_reason_logging(struct platform_device *pdev)
{
	unsigned int reset_reason;
	struct device_node *imem_np;
	void __iomem *imem_base;
	char reset_reason_msg[RESET_REASON_MSG_MAX_LEN];

	memset(reset_reason_msg, 0, sizeof(reset_reason_msg));
	imem_np = of_find_compatible_node(NULL, NULL,
				  "qcom,msm-imem-restart-reason-buf-addr");
	if (!imem_np) {
		dev_err(&pdev->dev,
		"restart_reason_buf_addr imem DT node does not exist\n");
		return -ENODEV;
	}

	imem_base = of_iomap(imem_np, 0);
	if (!imem_base) {
		dev_err(&pdev->dev,
		"restart_reason_buf_addr imem offset mapping failed\n");
		return -ENOMEM;
	}

	memcpy_fromio(&reset_reason, imem_base, 4);
	iounmap(imem_base);
	switch(reset_reason) {
		case NON_SECURE_WATCHDOG:
			scnprintf(reset_reason_msg, RESET_REASON_MSG_MAX_LEN,
						"%s", "Non-Secure Watchdog ");
			break;
		case AHB_TIMEOUT:
			scnprintf(reset_reason_msg, RESET_REASON_MSG_MAX_LEN,
						"%s", "AHB Timeout ");
			break;
		case NOC_ERROR:
			scnprintf(reset_reason_msg, RESET_REASON_MSG_MAX_LEN,
						"%s", "NOC Error ");
			break;
		case SYSTEM_RESET_OR_REBOOT:
			scnprintf(reset_reason_msg, RESET_REASON_MSG_MAX_LEN,
						"%s", "System reset or reboot ");
			break;
		case POWER_ON_RESET:
			scnprintf(reset_reason_msg, RESET_REASON_MSG_MAX_LEN,
						"%s", "Power on Reset ");
			break;
		case SECURE_WATCHDOG:
			scnprintf(reset_reason_msg, RESET_REASON_MSG_MAX_LEN,
						"%s", "Secure Watchdog ");
			break;
		case HLOS_PANIC:
			scnprintf(reset_reason_msg, RESET_REASON_MSG_MAX_LEN,
						"%s", "HLOS Panic ");
			break;
		case VFSM_RESET:
			scnprintf(reset_reason_msg, RESET_REASON_MSG_MAX_LEN,
						"%s", "VFSM Reset ");
			break;
		case TME_L_FATAL_ERROR:
			scnprintf(reset_reason_msg, RESET_REASON_MSG_MAX_LEN,
						"%s", "TME-L Fatal Error ");
			break;
		case TME_L_WDT_BITE_FATAL_ERROR:
			scnprintf(reset_reason_msg, RESET_REASON_MSG_MAX_LEN,
						"%s", "TME-L WDT Bite occurred ");
			break;
	}

	dev_info(&pdev->dev, "reset_reason : %s[0x%X]\n", reset_reason_msg,
		reset_reason);
	return 0;
}

static int scm_restart_reason_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;
	int ret, dload_dis_sec;
	struct device_node *np;
	unsigned int magic_cookie = SET_MAGIC_WARMRESET;
	unsigned int dload_warm_reset = 0;
	unsigned int no_reset_reason = 0;

	np = of_node_get(pdev->dev.of_node);
	if (!np)
		return 0;

	no_reset_reason = of_property_read_bool(np, "no-reset-reason");
	if (!no_reset_reason) {
		ret = restart_reason_logging(pdev);
		if (ret < 0) {
			dev_err(&pdev->dev, "reset reason logging failed!\n");
		}
	}

	id = of_match_device(scm_restart_reason_match_table, &pdev->dev);
	if (!id)
		return -ENODEV;

	ret = of_property_read_u32(np, "dload_status", &dload_dis);
	if (ret)
		dload_dis = 0;

	ret = of_property_read_u32(np, "dload_warm_reset", &dload_warm_reset);
	if (ret)
		dload_warm_reset = 0;

	ret = of_property_read_u32(np, "dload_sec_status", &dload_dis_sec);
	if (ret)
		dload_dis_sec = 0;

	if (dload_dis_sec) {
		qti_scm_dload(QCOM_SCM_SVC_BOOT,
			SCM_CMD_TZ_SET_DLOAD_FOR_SECURE_BOOT, NULL);
	}

	/* Ensure Disable before enabling the dload and sdi bits
	 * to make sure they are disabled during boot */
	if (dload_dis) {
		scm_restart_sdi_disable();
		if (!dload_warm_reset)
			magic_cookie = (uintptr_t)id->data;

		qti_scm_dload(QCOM_SCM_SVC_BOOT, SCM_CMD_TZ_FORCE_DLOAD_ID,
			      &magic_cookie);
	} else {
		scm_restart_dload_mode_enable();
	}

	ret = atomic_notifier_chain_register(&panic_notifier_list, &panic_nb);
	if (ret) {
		dev_err(&pdev->dev, "failed to setup download mode\n");
		goto fail;
	}

	ret = register_reboot_notifier(&reboot_nb);
	if (ret) {
		dev_err(&pdev->dev, "failed to setup reboot handler\n");
		atomic_notifier_chain_unregister(&panic_notifier_list,
								&panic_nb);
		goto fail;
	}

fail:
	return ret;
}

static int scm_restart_reason_remove(struct platform_device *pdev)
{
	atomic_notifier_chain_unregister(&panic_notifier_list, &panic_nb);
	unregister_reboot_notifier(&reboot_nb);
	return 0;
}

static struct platform_driver scm_restart_reason_driver = {
	.probe      = scm_restart_reason_probe,
	.remove     = scm_restart_reason_remove,
	.driver     = {
		.name = "qti_scm_restart_reason",
		.of_match_table = scm_restart_reason_match_table,
	},
};

module_platform_driver(scm_restart_reason_driver);

MODULE_DESCRIPTION("QTI SCM Restart Reason Driver");
