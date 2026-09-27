/*
 * BCM44XX Ethernet Windows device driver custom OID definitions.
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 * $Id: etioctl.h,v 13.14.2.2 2009/05/14 02:06:08 Exp $
 */

#ifndef _etioctl_h_
#define	_etioctl_h_

/*
 * Minor kludge alert:
 * Duplicate a few definitions that irelay requires from epiioctl.h here
 * so caller doesn't have to include this file and epiioctl.h .
 * If this grows any more, it would be time to move these irelay-specific
 * definitions out of the epiioctl.h and into a separate driver common file.
 */
#ifndef EPICTRL_COOKIE
#define EPICTRL_COOKIE		0xABADCEDE
#endif

/* common ioctl definitions */
#define	ETCUP		0
#define	ETCDOWN		1
#define ETCLOOP		2
#define ETCDUMP		3
#define ETCSETMSGLEVEL	4
#define	ETCPROMISC	5
#define ETCVAR		6
#define	ETCSPEED	7
#define ETCPHYRD	9
#define ETCPHYWR	10
#define	ETCQOS		11
#define ETCPHYRD2	12
#define ETCPHYWR2	13
#define ETCROBORD	14
#define ETCROBOWR	15

/*
 * A set of iovars defined for ET set/get 
 */
#define IOV_ET_POWER_SAVE_MODE 1

#if defined(linux)
#define SIOCSETCUP		(SIOCDEVPRIVATE + ETCUP)
#define SIOCSETCDOWN		(SIOCDEVPRIVATE + ETCDOWN)
#define SIOCSETCLOOP		(SIOCDEVPRIVATE + ETCLOOP)
#define SIOCGETCDUMP		(SIOCDEVPRIVATE + ETCDUMP)
#define SIOCSETCSETMSGLEVEL	(SIOCDEVPRIVATE + ETCSETMSGLEVEL)
#define SIOCSETCPROMISC		(SIOCDEVPRIVATE + ETCPROMISC)
#define SIOCSETGETVAR		(SIOCDEVPRIVATE + ETCVAR)
#define SIOCSETCSPEED		(SIOCDEVPRIVATE + ETCSPEED)
#define SIOCTXGEN		(SIOCDEVPRIVATE + 8)
#define SIOCGETCPHYRD		(SIOCDEVPRIVATE + ETCPHYRD)
#define SIOCSETCPHYWR		(SIOCDEVPRIVATE + ETCPHYWR)
#define SIOCSETCQOS		(SIOCDEVPRIVATE + ETCQOS)
#define SIOCGETCPHYRD2		(SIOCDEVPRIVATE + ETCPHYRD2)
#define SIOCSETCPHYWR2		(SIOCDEVPRIVATE + ETCPHYWR2)
#define SIOCGETCROBORD		(SIOCDEVPRIVATE + ETCROBORD)
#define SIOCSETCROBOWR		(SIOCDEVPRIVATE + ETCROBOWR)

/* structure to send a generic var set/get */
typedef struct et_var_s {
	uint cmd;
	uint set;
	void *buf;
	uint len;
} et_var_t;

/* arg to SIOCTXGEN */
struct txg {
	uint32 num;		/* number of frames to send */
	uint32 delay;		/* delay in microseconds between sending each */
	uint32 size;		/* size of ether frame to send */
	uchar buf[1514];	/* starting ether frame data */
};
#endif /* linux */


/*
 * custom OID support
 *
 * 0xFF - implementation specific OID
 * 0xE4 - first byte of Broadcom PCI vendor ID
 * 0x14 - second byte of Broadcom PCI vendor ID
 * 0xXX - the custom OID number
 */
#define ET_OID_BASE		0xFFE41400 /* OID Base for ET */

#define	OID_ET_UP				(ET_OID_BASE + ETCUP)
#define	OID_ET_DOWN				(ET_OID_BASE + ETCDOWN)
#define	OID_ET_LOOP				(ET_OID_BASE + ETCLOOP)
#define	OID_ET_DUMP				(ET_OID_BASE + ETCDUMP)
#define	OID_ET_SETMSGLEVEL			(ET_OID_BASE + ETCSETMSGLEVEL)
#define	OID_ET_PROMISC				(ET_OID_BASE + ETCPROMISC)
#define	OID_ET_TXDOWN				(ET_OID_BASE + 6)
#define	OID_ET_SPEED				(ET_OID_BASE + ETCSPEED)
#define	OID_ET_GETINSTANCE			(ET_OID_BASE + 8)
#define	OID_ET_SETCALLBACK			(ET_OID_BASE + 9)
#define	OID_ET_UNSETCALLBACK			(ET_OID_BASE + 10)

#define IS_ET_OID(oid) (((oid) & 0xFFFFFF00) == 0xFFE41400)

#define	ET_ISQUERYOID(oid)	((oid == OID_ET_DUMP) || (oid == OID_ET_GETINSTANCE))

/* OID_ET_SETCALLBACK data type */
typedef struct et_cb {
	void (*fn)(void *, int);	/* Callback function */
	void *context;				/* Passed to callback function */
} et_cb_t;

#endif /* _etioctl_h_ */
