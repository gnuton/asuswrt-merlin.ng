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
 /****************************************************************************
 *
 * power_services.h
 * May 23 2017
 * Venky Selvaraj
 *
 *******************************************************************************/
#ifndef _POWER_SERVICES_H_
#define _POWER_SERVICES_H_

#define PROC_POWER_DIR		"driver/power"
#define PROC_CMPOWER_FILE	"cm-power-state"
#define PROC_CMTEMPERATURE_FILE	"cm-temperature-state"

/*
 * msg.type  = cm_power_op
 * msg.data  =
 *             31..16  | 15..0
 *             channel | device
 * msg.extra = pwm_op arg
 */
enum cm_power_op
{
	CM_POWER_FULL,
	CM_POWER_REDUCED,
	CM_POWER_NET_STDBY,
	CM_MAX_POWER_OP
};

/*
 * msg.type  = cm_temperature_op
 * msg.data  =
 *             31..16  | 15..0
 *             channel | device
 * msg.extra = pwm_op arg
 */
enum cm_temperature_op
{
	CM_TEMPERATURE_NORMAL,
	CM_TEMPERATURE_ELEVATED_LEVEL1,
	CM_TEMPERATURE_ELEVATED_LEVEL2,
	CM_TEMPERATURE_ELEVATED_LEVEL3,
	CM_TEMPERATURE_ELEVATED_LOW,
	CM_TEMPERATURE_ELEVATED_MEDIUM,
	CM_TEMPERATURE_ELEVATED_HIGH,
	CM_MAX_TEMPERATURE_OP
};

#endif
