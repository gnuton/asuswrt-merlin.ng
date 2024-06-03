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
 * itc_msg_defs.h
 * Oct. 23 2012
 * Peter Sulc
 *
 *******************************************************************************/
#ifndef _ITC_MSG_DEFS_H_
#define _ITC_MSG_DEFS_H_

typedef struct {
	uint32_t header;
	uint32_t data[3];
} rpc_msg;
typedef struct {
	int	tunnel;
	rpc_msg	msg;
	int	timeout; /* seconds */
} rpc_msg_user;

#define RPC_MAX_FUNCTIONS	256

/*
 * Message header encoding in first 32bit word
 * |   6   |   1   |   1   |   8   |   8    |   8   |
 *  Version Request  Reply  Service Function Counter
 */

static inline uint8_t rpc_msg_version(rpc_msg *msg)
{
	return ((msg->header >> 26) & 0x3f);
}
static inline uint8_t rpc_msg_request(rpc_msg *msg)
{
	return ((msg->header >> 25) & 0x01);
}
static inline uint8_t rpc_msg_reply(rpc_msg *msg)
{
	return ((msg->header >> 24) & 0x01);
}
static inline uint8_t rpc_msg_service(rpc_msg *msg)
{
	return ((msg->header >> 16) & 0xff);
}
static inline uint8_t rpc_msg_function(rpc_msg *msg)
{
	return ((msg->header >>  8) & 0xff);
}
static inline uint8_t rpc_msg_counter(rpc_msg *msg)
{
	return ((msg->header >>  0) & 0xff);
}

/* xid = all except request and reply bits */
static inline uint32_t rpc_msg_xid(rpc_msg *msg)
{
	return (msg->header & ~(3 << 24));
}

static inline void rpc_msg_set_version(rpc_msg *msg, uint8_t v)
{
	(msg->header = (msg->header & ~(0x3f << 26)) | (v << 26));
}
static inline void rpc_msg_set_request(rpc_msg *msg, uint8_t v)
{
	(msg->header = (msg->header & ~(0x01 << 25)) | (v << 25));
}
static inline void rpc_msg_set_reply(rpc_msg *msg, uint8_t v)
{
	(msg->header = (msg->header & ~(0x01 << 24)) | (v << 24));
}
static inline void rpc_msg_set_service(rpc_msg *msg, uint8_t v)
{
	(msg->header = (msg->header & ~(0xff << 16)) | (v << 16));
}
static inline void rpc_msg_set_function(rpc_msg *msg, uint8_t v)
{
	(msg->header = (msg->header & ~(0xff <<  8)) | (v <<  8));
}
static inline void rpc_msg_set_counter(rpc_msg *msg, uint8_t v)
{
	(msg->header = (msg->header & ~(0xff <<  0)) | (v <<  0));
}

#define RPC_MSG_MAKE_HEADER(version, req, rep, serv, func, reqcnt)( \
		(version << 26) |	\
		(req << 25) |	\
		(rep << 24) |	\
		(serv << 16) |	\
		(func << 8) |	\
		(reqcnt) )

#define RPC_MSG_INIT_CODE0		0xbeef0000
#define RPC_MSG_INIT_CODE1		0xbeef0001
#define RPC_MSG_INIT_CODE2		0xbeef0002

#endif
