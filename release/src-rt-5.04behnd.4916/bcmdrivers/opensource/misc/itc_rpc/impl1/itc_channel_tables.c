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
 ****************************************************************************/
 /*******************************************************************************
 *
 * itc_channel_tables.c
 * Oct. 23 2012
 * Peter Sulc
 *
 *******************************************************************************/

#include <itc_rpc.h>
#include <itc_channel_structs.h>

/* This file should only be included in ItcRpc.c
 * The tables below are intended to be modified
 * for specific implementation requirements
 */

/*
 * services table: must match service indexes
 */
enum init_svc_idx {
	INIT_SVC_INIT_TUNNEL = 0,
	INIT_SVC_ERR,
	INIT_SVC_MAX
};

enum init_err_rc {
	INIT_SVC_ERR_RC_HANDSHAKE = 1,
	INIT_SVC_ERR_RC_RPC_VER_MISMATCH,
	INIT_SVC_ERR_RC_MSG_VER_MISMATCH
};

char *init_err_rc_str[] = {
	"tunnel init failure",
	"RPC version mismatch",
	"message version mismatch"
};

#define INIT_SVC_HANDSHAKE_VER	0
#define INIT_SVC_ERR_VER	0

int init_service_handshake(int tunnel, rpc_msg *msg);
int init_service_err(int tunnel, rpc_msg *msg);
rpc_function init_svc_table[] =
{
	{ init_service_handshake,	INIT_SVC_HANDSHAKE_VER },
	{ init_service_err,		INIT_SVC_ERR_VER },
};

rpc_service itc_rpc_services[RPC_MAX_SERVICES] =
{
    { .thread_name = "rpc_init", .func_tab = init_svc_table, .func_tab_sz = 1,
      .active = true },
    { .thread_name = "rpc_misc",	.active = SERVICE_MISC_ACTIVE  },
    { .thread_name = "rpc_flash",	.active = SERVICE_FLASH_MTD_ACTIVE  },
    { .thread_name = "rpc_nonvol",	.active = SERVICE_NONVOL_ACTIVE },
    { .thread_name = "rpc_erouter",	.active = SERVICE_EROUTER_ACTIVE  },
    { .thread_name = "rpc_erouter_pm",	.active = SERVICE_EROUTER_PM_ACTIVE  },
    { .thread_name = "rpc_wpsctl",	.active = SERVICE_WPS_CTL_ACTIVE  },
    { .thread_name = "rpc_bash",	.active = SERVICE_BASH_USER_ACTIVE },
    { .thread_name = "rpc_testu",	.active = SERVICE_TEST_USER_ACTIVE },
    { .thread_name = "rpc_gpio",	.active = SERVICE_GPIO_ACTIVE },
    { .thread_name = "rpc_pinmux",	.active = SERVICE_PINMUX_ACTIVE },
    { .thread_name = "rpc_led",		.active = SERVICE_LED_ACTIVE },
    { .thread_name = "rpc_vfbio",	.active = SERVICE_VFBIO_ACTIVE },
    { .thread_name = "rpc_power",	.active = SERVICE_PWR_ACTIVE },
    { .thread_name = "rpc_clock",	.active = SERVICE_CLK_ACTIVE },
    { .thread_name = "rpc_system",	.active = SERVICE_SYS_ACTIVE },
    { .thread_name = "rpc_bbs",		.active = SERVICE_BBS_ACTIVE },
    { .thread_name = "rpc_ba",		.active = SERVICE_BA_ACTIVE },
    { .thread_name = "rpc_avs",		.active = SERVICE_AVS_ACTIVE },
    { .thread_name = "rpc_ubcap",	.active = SERVICE_UBCAP_ACTIVE },
    { .thread_name = "rpc_fpm",		.active = SERVICE_FPM_ACTIVE },
    { .thread_name = "rpc_rtc",		.active = SERVICE_RTC_ACTIVE },
    { .thread_name = "rpc_se",		.active = SERVICE_SE_ACTIVE },
    { .thread_name = "rpc_cr",		.active = SERVICE_CR_ACTIVE },
    { .thread_name = "rpc_alert",	.active = SERVICE_ALERT_ACTIVE },
    { .thread_name = "rpc_otp",		.active = SERVICE_OTP_ACTIVE },
    { .thread_name = "rpc_bp3",		.active = SERVICE_BP3_ACTIVE },
};
