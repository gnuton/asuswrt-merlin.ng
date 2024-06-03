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
 * itc_user.h
 * Peter Sulc
 *
 *******************************************************************************/

#ifndef _ITC_USER_H_
#define _ITC_USER_H_

#include <linux/ioctl.h>

#define ITC_IOC_CODE		'>'
#define ITC_TUNNEL_NAME_MAX	32

#define ITC_IOCTL_SERVICE_REGISTER	_IOR(ITC_IOC_CODE,	0, int)
#define ITC_IOCTL_SERVICE_LISTEN	_IOR(ITC_IOC_CODE,	1, rpc_msg)
#define ITC_IOCTL_GET_TUNNEL_ID		_IOR(ITC_IOC_CODE,	2, char[ITC_TUNNEL_NAME_MAX])
#define ITC_IOCTL_SEND_REPLY		_IOW(ITC_IOC_CODE,	3, rpc_msg)
#define ITC_IOCTL_SEND_MESSAGE		_IOW(ITC_IOC_CODE,	4, rpc_msg)
#define ITC_IOCTL_SEND_REQUEST		_IOWR(ITC_IOC_CODE,	5, rpc_msg)
#define ITC_IOCTL_SEND_REQUEST_TIMEOUT	_IOWR(ITC_IOC_CODE,	6, rpc_msg)

#endif
