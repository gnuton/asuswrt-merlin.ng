/*
 * CDC network driver ioctl/indication encoding
 * Broadcom 802.11abg Networking Device Driver
 *
 * Definitions subject to change without notice.
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: bcmcdc.h,v 13.14 2007/11/21 02:07:16 Exp $
 */
#include <proto/ethernet.h>

typedef struct cdc_ioctl {
	uint32 cmd;      /* ioctl command value */
	uint32 len;      /* lower 16: output buflen; upper 16: input buflen (excludes header) */
	uint32 flags;    /* flag defns given below */
	uint32 status;   /* status code returned from the device */
} cdc_ioctl_t;

/* Max valid buffer size that can be sent to the dongle */
#define CDC_MAX_MSG_SIZE   ETHER_MAX_LEN

/* len field is divided into input and output buffer lengths */
#define CDCL_IOC_OUTLEN_MASK   0x0000FFFF  /* maximum or expected response length, */
					   /* excluding IOCTL header */
#define CDCL_IOC_OUTLEN_SHIFT  0
#define CDCL_IOC_INLEN_MASK    0xFFFF0000   /* input buffer length, excluding IOCTL header */
#define CDCL_IOC_INLEN_SHIFT   16

/* CDC flag definitions */
#define CDCF_IOC_ERROR		0x01	/* 0=success, 1=ioctl cmd failed */
#define CDCF_IOC_SET		0x02	/* 0=get, 1=set cmd */
#define CDCF_IOC_ID_MASK	0xFFFF0000	/* used to uniquely id an ioctl req/resp pairing */
#define CDCF_IOC_ID_SHIFT	16		/* # of bits of shift for ID Mask */

/* Convenient extraction of message ID */
#define CDC_IOC_ID(flags)	(((flags) & CDCF_IOC_ID_MASK) >> CDCF_IOC_ID_SHIFT)

/*
 * BDC header
 *
 *   The BDC header is used on data packets to convey priority across USB.
 */

#define	BDC_HEADER_LEN		4

#define BDC_PROTO_VER		1	/* Protocol version */

#define BDC_FLAG_VER_MASK	0xf0	/* Protocol version mask */
#define BDC_FLAG_VER_SHIFT	4	/* Protocol version shift */

#define BDC_FLAG_SUM_NEEDED	0x08	/* Dongle needs to do TX checksums */
#define BDC_FLAG_SUM_GOOD	0x04	/* Dongle has verified good RX checksums */

#define BDC_FLAG__UNUSED	0x03	/* Unassigned */

#define BDC_PRIORITY_MASK	0x7

struct bdc_header {
	uint8	flags;			/* Flags */
	uint8	priority;		/* 802.1d Priority (low 3 bits) */
	uint8	pad[2];
};
