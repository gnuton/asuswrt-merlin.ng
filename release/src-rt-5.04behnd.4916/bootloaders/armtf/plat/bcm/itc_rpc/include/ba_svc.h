/* SPDX-License-Identifier: GPL-2.0+ */
/***************************************************************************
 * Broadcom Proprietary and Confidential. (c)2019 Broadcom. All rights reserved.
 *
 *  THIS SOFTWARE MAY ONLY BE USED SUBJECT TO AN EXECUTED SOFTWARE LICENSE
 *  AGREEMENT  BETWEEN THE USER AND BROADCOM.  YOU HAVE NO RIGHT TO USE OR
 *  EXPLOIT THIS MATERIAL EXCEPT SUBJECT TO THE TERMS OF SUCH AN AGREEMENT.
 *
 ***************************************************************************/

#ifndef _BA_SVC_H_
#define _BA_SVC_H_

#include <stdint.h>
#include "itc_rpc.h"

enum ba_svc_func_idx {
    BA_ARMTF_ONLY_BEGIN = 10,
    BA_SVC_DDR_RANGE_SEC = BA_ARMTF_ONLY_BEGIN,

    /* ATTENTION:
     *
     * All ARMTF ONLY RPC commands should be added above this line
     *
     * */
    BA_ARMTF_UBOOT_BEGIN = 20,
    BA_SVC_BOOT_FROM_ADDR = BA_ARMTF_UBOOT_BEGIN,

    /* ATTENTION:
     *
     * All RPC commands issued from ARMTF and Uboot should be added above this line
     * and synced with .../arch/arm/mach-bcmbca/include/ba_svc.h
     *
     * */
    BA_SVC_FUNC_MAX
};

enum ba_req_rs_rsp {
	BA_SVC_RESPONSE_READY,
	BA_SVC_RESPONSE_BUSY,
	BA_SVC_RESPONSE_MAX
};

#define BA_SVC_RS_OFF		"OFF"
#define BA_SVC_RS_RESET		"RESET"
#define BA_SVC_RS_BOOT		"BOOT"
#define BA_SVC_RS_SHUTDOWN	"SHUTDOWN"
#define BA_SVC_RS_RUNNING	"RUNNING"
#define BA_SVC_RS_READY		"READY"

extern uint32_t ba_rs_off;
extern uint32_t ba_rs_reset;
extern uint32_t ba_rs_boot;
extern uint32_t ba_rs_shutdown;
extern uint32_t ba_rs_running;
extern uint32_t ba_rs_ready;

#define INVALID_ID	(0xffffffff)

struct ba_msg {
	uint32_t	hdr;
	union {
		uint32_t	rsvd0;
		struct {
			uint8_t	cpu_id;
			uint8_t	rs_id;
			union {
				uint8_t	be_rude:1;
				uint8_t	rsvd1:7;

				uint8_t	response:4;
				uint8_t rsvd2:4;
			};
			uint8_t	rc:8;
		};
	};
	union {
		uint32_t	rsvd3[2];
		char		name[8];
	};
};


/* ba_svc rpc message manipulation helpers */
static inline uint8_t ba_svc_msg_get_retcode(rpc_msg *msg)
{
	struct ba_msg *ba_msg = (struct ba_msg *)msg;
	return ba_msg->rc;
}

/* ba svc functions */
int ba_svc_boot_secondary(uint32_t cpu_mask, uint32_t vector);
int ba_svc_enable_ddr_range_sec(uint64_t addr, uint32_t size);
#endif
