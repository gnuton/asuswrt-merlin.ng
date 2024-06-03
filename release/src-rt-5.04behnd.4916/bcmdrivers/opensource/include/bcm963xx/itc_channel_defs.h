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
 * itc_channel_defs.h
 * Oct. 23 2012
 * Peter Sulc
 *
 *******************************************************************************/
#ifndef _ITC_CHANNEL_DEFS_H_
#define _ITC_CHANNEL_DEFS_H_

/* DQM channel and device/implementation specific defines */
#define RPC_TUNNELS_MAX		4
#define RPC_INVALID_TUNNEL	-1

/* service indexes */
enum {
	RPC_SERVICE_INIT = 0,	/* special channel init and handshake messages */
	RPC_SERVICE_MISC,	/* misc. messages/services */
	RPC_SERVICE_FLASH_MTD,	/* Flash MTD */
	RPC_SERVICE_FLASH = RPC_SERVICE_FLASH_MTD,
	RPC_SERVICE_NONVOL,	/* Split mode nonvol support */
	RPC_SERVICE_EROUTER,	/* erouter - Linux user space */
	RPC_SERVICE_EROUTER_PM,	/* erouter - Linux user space power management */
	RPC_SERVICE_WPS_CTL,	/* wireless reset button */
	RPC_SERVICE_BASH_USER,	/* Linux bash shell service */
	RPC_SERVICE_TEST_USER,	/* Linux user space test service */
	RPC_SERVICE_GPIO,	/* GPIO */
	RPC_SERVICE_PINCTRL,	/* Pin Control */
	RPC_SERVICE_LED,	/* LED */
	RPC_SERVICE_VFBIO,	/* Flash block I/O */
	RPC_SERVICE_PWR,	/* Power Domain Control */
	RPC_SERVICE_CLK,	/* Clock Control */
	RPC_SERVICE_SYS,	/* System-wide Services */
	RPC_SERVICE_BBS,	/* BBS Control */
	RPC_SERVICE_BA,		/* Boot Assist */
	RPC_SERVICE_AVS,	/* Adaptive Voltage Scaling service */
	RPC_SERVICE_UBCAP,	/* UBUS Capture */
	RPC_SERVICE_FPM,	/* FPM */
	RPC_SERVICE_RTC,	/* Real-Time Clock */
        RPC_SERVICE_SE,         /* */
        RPC_SERVICE_CR,         /* */
        RPC_SERVICE_ALERT,      /* */
        RPC_SERVICE_OTP,        /* */
        RPC_SERVICE_BP3,        /* */
	RPC_MAX_SERVICES	/* Total number of supported services */
};

#define SERVICE_MISC_ACTIVE		true
#define SERVICE_FLASH_MTD_ACTIVE	true
#define SERVICE_NONVOL_ACTIVE		false
#define SERVICE_EROUTER_ACTIVE		true
#define SERVICE_EROUTER_PM_ACTIVE	false
#define SERVICE_WPS_CTL_ACTIVE		true
#define SERVICE_BASH_USER_ACTIVE	false
#define SERVICE_TEST_USER_ACTIVE	false
#define SERVICE_GPIO_ACTIVE		false
#define SERVICE_PINMUX_ACTIVE		false
#define SERVICE_LED_ACTIVE		true
#define SERVICE_VFBIO_ACTIVE		false
#define SERVICE_PWR_ACTIVE		true
#define SERVICE_CLK_ACTIVE		true
#define SERVICE_SYS_ACTIVE		true
#define SERVICE_BBS_ACTIVE		false
#define SERVICE_BA_ACTIVE		true
#define SERVICE_AVS_ACTIVE		true
#define SERVICE_UBCAP_ACTIVE		false
#define SERVICE_FPM_ACTIVE		false
#define SERVICE_RTC_ACTIVE		false
#define SERVICE_SE_ACTIVE		false
#define SERVICE_CR_ACTIVE		false
#define SERVICE_ALERT_ACTIVE		false
#define SERVICE_OTP_ACTIVE		true
#define SERVICE_BP3_ACTIVE		true

#define RPC_MSG_POOL_MSG_CNT	64
#define RPC_ORPHAN_QUEUE_LIMIT	8

#endif
