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
 /*
 * pwm_service.c
 * Sep. 2015
 * Peter Sulc - Moved Fabian Chicaiza's pwm service code to its own file in order
 * to keep misc relatively small and simple.
 * No logic changes just aesthetics.
 *
 *******************************************************************************/
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/pwm.h>

#include "misc_services.h"

/* PWM related constants */
#define MAX_PWM_DEVICES  2
#define MAX_PWM_CHANNELS_PER_DEV 2
#define MAX_PWM_CHANNELS (MAX_PWM_CHANNELS_PER_DEV*MAX_PWM_DEVICES)

/*
 * msg.type  = pwp_op
 * msg.data  =
 *             31..16  | 15..0
 *             channel | device
 * msg.extra = pwm_op arg
 */
enum pwm_op
{
	PWM_GET,             /* get pwm device     */
	PWM_PUT,             /* free pwm device    */
	PWM_CONFIG_DUTY_NS,  /* set duty_cycle(ns) */
	PWM_CONFIG_PERIOD_NS,/* set period(ns)     */
	PWM_CONFIG,          /* configure PWM      */
	PWM_ENABLE,          /* start pulse        */
	PWM_DISABLE,         /* stop pulse         */
};

/*
 * struct pwm_id
 */
struct pwm_id
{
	uint16_t device;
	uint16_t channel;
};

static struct pwm_device *pwm_dev[MAX_PWM_CHANNELS] = {NULL};

int pwm_ctrl(int dqm_tunnel, struct misc_msg *msg)
{
	struct pwm_id *pid;
	unsigned int duty_cycle,period,ch_idx;

	pid = (struct pwm_id *)&msg->data;
	ch_idx = (pid->device)*MAX_PWM_CHANNELS_PER_DEV + pid->channel;

	if (ch_idx>=MAX_PWM_CHANNELS)
	{
		pr_err("%s:Unknown channel idx:%d\n",__func__,ch_idx);
		goto done;
	}

	switch (msg->type)
	{
	case PWM_GET:
		if (pwm_dev[ch_idx]==NULL)
		{
			switch (pid->device)
			{
			case 0:/*PWM_A*/
				pwm_dev[ch_idx]=pwm_request(ch_idx,"pwmchip0");
				break;
			case 1:/*PWM_B*/
				pwm_dev[ch_idx]=pwm_request(ch_idx,"pwmchip2");
				break;
			default:
				pr_err("PWM_GET-Unknown pwm device:%d\n",pid->device);
				msg->data = -1;
				break;
			}
		}
		else
		{
			pr_err("PWM_GET-pwm_dev[%d] is NOT NULL!\n",ch_idx);
			msg->data = -1;
		}
		break;

	case PWM_PUT:
		if (pwm_dev[ch_idx])
		{
			pwm_put(pwm_dev[ch_idx]);
			pwm_dev[ch_idx] = NULL;
		}
		else
		{
			pr_err("PWM_PUT-pwm_dev[%d] is NULL!\n",ch_idx);
			msg->data = -1;
		}
		break;

	case PWM_CONFIG_DUTY_NS:
		if (pwm_dev[ch_idx])
		{
			pwm_set_duty_cycle(pwm_dev[ch_idx],msg->extra);
		}
		else
		{
			pr_err("PWM_CONFIG_DUTY_NS-pwm_dev[%d] is NULL!\n",ch_idx);
			msg->data = -1;
		}
		break;

	case PWM_CONFIG_PERIOD_NS:
		if (pwm_dev[ch_idx])
		{
			pwm_set_period(pwm_dev[ch_idx],msg->extra);
		}
		else
		{
			pr_err("PWM_CONFIG_PERIOD_NS-pwm_dev[%d] is NULL!\n",ch_idx);
			msg->data = -1;
		}
		break;

	case PWM_CONFIG:
		if (pwm_dev[ch_idx])
		{
			duty_cycle=pwm_get_duty_cycle(pwm_dev[ch_idx]);
			period=pwm_get_period(pwm_dev[ch_idx]);

			if (duty_cycle>period)
			{
				pr_err("PWM_CONFIG-Duty Cycle(%d)>Period(%d) on pwm_dev[%d]!\n",
				duty_cycle,period,ch_idx);
				msg->data = -1;
				break;
			}

			if (pwm_config(pwm_dev[ch_idx],duty_cycle,period))
			{
				pr_err("pwm_config(pwm_dev[%d]) failed.Period=%d,Duty C.=%d\n",
				ch_idx,period,duty_cycle);
				msg->data = -1;
			}
		}
		else
		{
			pr_err("PWM_CONFIG_PERIOD_NS-pwm_dev[%d] is NULL!\n",ch_idx);
			msg->data = -1;
		}
		break;

	case PWM_ENABLE:
		if (pwm_dev[ch_idx])
		{
			pwm_enable(pwm_dev[ch_idx]);
		}
		else
		{
			pr_err("PWM_ENABLE-pwm_dev[%d] is NULL!\n",ch_idx);
			msg->data = -1;
		}
		break;

	case PWM_DISABLE:
		if (pwm_dev[ch_idx])
		{
			pwm_disable(pwm_dev[ch_idx]);
		}
		{
			pr_err("PWM_DISABLE-pwm_dev[%d] is NULL!\n",ch_idx);
			msg->data = -1;
		}
		break;

	default:
		pr_err("%s Unknown message type(pwm_op):%d\n",__func__,msg->type);
		msg->data = -1;
		break;
	}

done:
	if (rpc_msg_request((rpc_msg *)msg))
		rpc_send_reply(dqm_tunnel, (rpc_msg *)msg);

	return 0;
}
