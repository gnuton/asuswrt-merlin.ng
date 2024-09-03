/* SPDX-License-Identifier: GPL-2.0+ */
/***************************************************************************
 * Broadcom Proprietary and Confidential. (c)2019 Broadcom. All rights reserved.
 *
 *  THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 *  AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 *  EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "itc_rpc.h"
#include "ba_svc.h"

#define BA_SVC_RPC_REQUEST_TIMEOUT	1 /* sec */

extern char *strncpy(char *dest, const char *src, size_t n);

/* BA service helpers */
static inline int ba_svc_request(rpc_msg *msg)
{
	int rc = 0;
	rc = rpc_send_request_timeout(RPC_TUNNEL_ARMTF_SMC_SEC,
		msg, BA_SVC_RPC_REQUEST_TIMEOUT);
	if (rc) {
		printf("ba_svc: rpc_send_request failure (%d)\n", rc);
		rpc_dump_msg(msg);
		goto done;
	}
	rc = ba_svc_msg_get_retcode(msg);
done:
	return rc;
}

static inline int ba_svc_message(rpc_msg *msg)
{
	int rc = 0;
	rc = rpc_send_message(RPC_TUNNEL_ARMTF_SMC_SEC, msg);
	if (rc) {
		printf("ba_svc: rpc_send_message failure (%d)\n", rc);
		rpc_dump_msg(msg);
	}
	return rc;
}

int ba_svc_boot_secondary(uint32_t cpu_mask, uint32_t vector)
{
	int rc = 0;
	rpc_msg msg;

	rpc_msg_init(&msg, RPC_SERVICE_BA, BA_SVC_BOOT_FROM_ADDR, 0, 0, 0, 0);
	msg.data[0] = 0;
	msg.data[1] = vector;
	msg.data[2] = cpu_mask;
	rc = ba_svc_message(&msg);
	return rc;
}

int ba_svc_enable_ddr_range_sec(uint64_t addr, uint32_t size)
{
	int rc = 0;
	rpc_msg msg;

	rpc_msg_init(&msg, RPC_SERVICE_BA, BA_SVC_DDR_RANGE_SEC, 0, 0, 0, 0);
	msg.data[0] = (uint32_t)(addr & 0xffffffff);
	msg.data[1] = (uint32_t)((addr >> 32) & 0xffffffff);
	msg.data[2] = size;
	rc = ba_svc_message(&msg);
	return rc;
}
	
/* BA service handlers */

