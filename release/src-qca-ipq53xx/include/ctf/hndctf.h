/*
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: hndctf.h,v 1.1.2.5 2010/01/06 19:09:50 Exp $
 */

#ifndef _HNDCTF_H_
#define _HNDCTF_H_

#include <bcmutils.h>
#include <proto/ethernet.h>

#define CTF_ENAB(ci)		(((ci) != NULL) && (ci)->_ctf)

#define CTF_ACTION_TAG		(1 << 0)
#define CTF_ACTION_UNTAG	(1 << 1)
#define CTF_ACTION_SNAT		(1 << 2)
#define CTF_ACTION_DNAT		(1 << 3)

#define	ctf_attach(osh, n, m, c, a) \
	(ctf_attach_fn ? ctf_attach_fn(osh, n, m, c, a) : NULL)
#define ctf_forward(ci, p)	(ci)->fn.forward(ci, p)
#define ctf_isenabled(ci, d)	(CTF_ENAB(ci) ? (ci)->fn.isenabled(ci, d) : FALSE)
#define ctf_isbridge(ci, d)	(CTF_ENAB(ci) ? (ci)->fn.isbridge(ci, d) : FALSE)
#define ctf_enable(ci, d, e)	(CTF_ENAB(ci) ? (ci)->fn.enable(ci, d, e) : BCME_OK)
#define ctf_brc_add(ci, b)	(CTF_ENAB(ci) ? (ci)->fn.brc_add(ci, b) : BCME_OK)
#define ctf_brc_delete(ci, e)	(CTF_ENAB(ci) ? (ci)->fn.brc_delete(ci, e) : BCME_OK)
#define ctf_brc_update(ci, b)	(CTF_ENAB(ci) ? (ci)->fn.brc_update(ci, b) : BCME_OK)
#define ctf_brc_lkup(ci, e)	(CTF_ENAB(ci) ? (ci)->fn.brc_lkup(ci, e) : NULL)
#define ctf_ipc_add(ci, i)	(CTF_ENAB(ci) ? (ci)->fn.ipc_add(ci, i) : BCME_OK)
#define ctf_ipc_delete(ci, sip, dip, p, sp, dp)	\
	(CTF_ENAB(ci) ? (ci)->fn.ipc_delete(ci, sip, dip, p, sp, dp) : BCME_OK)
#define ctf_ipc_lkup(ci, sip, dip, p, sp, dp)	\
	(CTF_ENAB(ci) ? (ci)->fn.ipc_lkup(ci, sip, dip, p, sp, dp) : NULL)
#define ctf_dev_register(ci, d, b)	\
	(CTF_ENAB(ci) ? (ci)->fn.dev_register(ci, d, b) : BCME_OK)
#define ctf_detach(ci)			if (CTF_ENAB(ci)) (ci)->fn.detach(ci)
#define ctf_dump(ci, b)			if (CTF_ENAB(ci)) (ci)->fn.dump(ci, b)
#define ctf_dev_unregister(ci, d)	if (CTF_ENAB(ci)) (ci)->fn.dev_unregister(ci, d)

typedef struct ctf_pub	ctf_t;
typedef struct ctf_brc	ctf_brc_t;
typedef struct ctf_ipc	ctf_ipc_t;

typedef void (*ctf_detach_cb_t)(ctf_t *ci, void *arg);
typedef ctf_t * (*ctf_attach_t)(osl_t *osh, uint8 *name, uint32 *msg_level,
                                ctf_detach_cb_t cb, void *arg);
typedef void (*ctf_detach_t)(ctf_t *ci);
typedef int32 (*ctf_forward_t)(ctf_t *ci, void *p);
typedef bool (*ctf_isenabled_t)(ctf_t *ci, void *dev);
typedef bool (*ctf_isbridge_t)(ctf_t *ci, void *dev);
typedef int32 (*ctf_brc_add_t)(ctf_t *ci, ctf_brc_t *brc);
typedef int32 (*ctf_brc_delete_t)(ctf_t *ci, uint8 *ea);
typedef int32 (*ctf_brc_update_t)(ctf_t *ci, ctf_brc_t *brc);
typedef ctf_brc_t * (*ctf_brc_lkup_t)(ctf_t *ci, uint8 *da);
typedef int32 (*ctf_ipc_add_t)(ctf_t *ci, ctf_ipc_t *ipc);
typedef int32 (*ctf_ipc_delete_t)(ctf_t *ci, uint32 sip, uint32 dip, uint8 proto,
                                  uint16 sp, uint16 dp);
typedef ctf_ipc_t * (*ctf_ipc_lkup_t)(ctf_t *ci, uint32 sip, uint32 dip, uint8 proto,
                                    uint16 sp, uint16 dp);
typedef int32 (*ctf_enable_t)(ctf_t *ci, void *dev, bool enable);
typedef int32 (*ctf_dev_register_t)(ctf_t *ci, void *dev, bool br);
typedef void (*ctf_dev_unregister_t)(ctf_t *ci, void *dev);
#if defined(BCMDBG_DUMP)
typedef void (*ctf_dump_t)(ctf_t *ci, struct bcmstrbuf *b);
#endif 

typedef struct ctf_fn {
	ctf_detach_t		detach;
	ctf_forward_t		forward;
	ctf_isenabled_t		isenabled;
	ctf_isbridge_t		isbridge;
	ctf_brc_add_t		brc_add;
	ctf_brc_delete_t	brc_delete;
	ctf_brc_update_t	brc_update;
	ctf_brc_lkup_t		brc_lkup;
	ctf_ipc_add_t		ipc_add;
	ctf_ipc_delete_t	ipc_delete;
	ctf_ipc_lkup_t		ipc_lkup;
	ctf_enable_t		enable;
	ctf_dev_register_t	dev_register;
	ctf_dev_unregister_t	dev_unregister;
	ctf_detach_cb_t		detach_cb;
	void			*detach_cb_arg;
#if defined(BCMDBG_DUMP)
	ctf_dump_t		dump;
#endif 
} ctf_fn_t;

struct ctf_pub {
	bool			_ctf;		/* Global CTF enable/disable */
	ctf_fn_t		fn;		/* Exported functions */
};

struct ctf_brc {
	struct	ctf_brc		*next;		/* Pointer to brc entry */
	struct	ether_addr	dhost;		/* MAC addr of host */
	uint16			vid;		/* VLAN id to use on txif */
	void			*txifp;		/* Interface connected to host */
	uint32			action;		/* Tag or untag the frames */
	uint32			hits;		/* Num frames matching brc entry */
	uint32			live;		/* Counter used to expire the entry */
};

typedef struct ctf_conn_tuple {
	uint32	sip, dip;
	uint16	sp, dp;
	uint8	proto;
} ctf_conn_tuple_t;

typedef struct ctf_nat {
	uint32	ip;
	uint16	port;
} ctf_nat_t;

struct ctf_ipc {
	struct	ctf_ipc		*next;		/* Pointer to ipc entry */
	ctf_conn_tuple_t	tuple;		/* Tuple to uniquely id the flow */
	uint16			vid;		/* VLAN id to use on txif */
	struct	ether_addr	dhost;		/* Destination MAC address */
	struct	ether_addr	shost;		/* Source MAC address */
	void			*txif;		/* Target interface to send */
	uint32			action;		/* NAT and/or VLAN actions */
	uint32			hits;		/* Num frames matching ipc entry */
	uint32			live;		/* Counter used to expire the entry */
	struct	ctf_nat		nat[2];		/* Manip data for SNAT, DNAT */
};

extern ctf_t *ctf_kattach(osl_t *osh, uint8 *name);
extern void ctf_kdetach(ctf_t *kci);
extern ctf_attach_t ctf_attach_fn;
extern ctf_t *_ctf_attach(osl_t *osh, uint8 *name, uint32 *msg_level,
                          ctf_detach_cb_t cb, void *arg);

#endif /* _HNDCTF_H_ */
