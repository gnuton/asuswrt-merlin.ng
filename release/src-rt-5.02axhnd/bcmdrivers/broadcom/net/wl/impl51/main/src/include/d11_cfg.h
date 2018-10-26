/**
 * Common header file shared between bus layer and wl layer
 * As of today, splitrx related and cached flow processing related headers are shared here
 *
 * Copyright (C) 2018, Broadcom. All Rights Reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 * $Id$
 */

/*
 * Explains different splitrx modes, macros for classify, conversion.
 * splitrx twiki: http://hwnbu-twiki.broadcom.com/bin/view/Mwgroup/SplitRXModes
 * header conversion twiki: http://hwnbu-twiki.broadcom.com/bin/view/Mwgroup/Dot11MacHdrConversion
 */

#ifndef _d11_cfg_h_
#define _d11_cfg_h_
#include <wlconf.h>

#define	RXMODE1	1	/* descriptor split */
#define	RXMODE2	2	/* descriptor split + classification */
#define	RXMODE3	3	/* fifo split + classification */
#define	RXMODE4	4	/* fifo split + classification + hdr conversion */

#ifdef BCMSPLITRX
	extern bool _bcmsplitrx;
	#if defined(ROM_ENAB_RUNTIME_CHECK) || !defined(DONGLEBUILD)
		#define BCMSPLITRX_ENAB() (_bcmsplitrx)
	#elif defined(BCMSPLITRX_DISABLED)
		#define BCMSPLITRX_ENAB()	(0)
	#else
		#define BCMSPLITRX_ENAB()	(1)
	#endif
#else
	#define BCMSPLITRX_ENAB()		(0)
#endif /* BCMSPLITRX */

#ifdef BCMSPLITRX
	extern uint8 _bcmsplitrx_mode;
	#if defined(ROM_ENAB_RUNTIME_CHECK) || !defined(DONGLEBUILD)
		#define BCMSPLITRX_MODE() (_bcmsplitrx_mode)
	#elif defined(BCMSPLITRX_DISABLED)
		#define BCMSPLITRX_MODE()	(0)
	#else
		#define BCMSPLITRX_MODE() (_bcmsplitrx_mode)
	#endif
#else
	#define BCMSPLITRX_MODE()		(0)
#endif /* BCMSPLITRX */

#define SPLIT_RXMODE1()	((BCMSPLITRX_MODE() == RXMODE1))
#define SPLIT_RXMODE2()	((BCMSPLITRX_MODE() == RXMODE2))
#define SPLIT_RXMODE3()	((BCMSPLITRX_MODE() == RXMODE3))
#define SPLIT_RXMODE4()	((BCMSPLITRX_MODE() == RXMODE4))

#define PKT_CLASSIFY()	(SPLIT_RXMODE2() || SPLIT_RXMODE3() || SPLIT_RXMODE4())
#define RXFIFO_SPLIT()	(SPLIT_RXMODE3() || SPLIT_RXMODE4())
#define HDR_CONV()	(SPLIT_RXMODE4())

#define HW_HDR_CONV_PAD 2

#ifdef WLCFP
/**
 *  CFP opens up a direct path between wl and bus layers.
 *  It skips regular dongle/rte layers.
 *  Inorder to achieve that below macros and functions
 *  are externed to be accessible by both layers
 */
#define CFP_ALL_FLOWS                   (-1)
/**
 * Max CFP Flows = 1 per STA + SCB INTERNAL[1 BCMC per bsscfg + 1 HWRS + 1 OLPC]
 * Since flowid=0 is reserved, we add 1 more to CFP_FLOWS_MAX
 *
 * See wl_pktfwd.h where CFP flowids in the [0 .. MAXSCB) range are used. A
 * CFP flowid of N is essentially a pktlist::dest value of N-1, with a dest=0
 * being a valid value.
 *
 * Following are used in wlc_cfp.c, to carve out two flowid allocators service
 * CFP flowids in the ranges [1 .. MAXSCB] and [MAXSCB+1 .. CFP_FLOWS_MAX-1]
 */
#define CFP_FLOWID_RSVD_TOTAL           (1)
#define CFP_FLOWID_SCB_TOTAL            (MAXSCB)
#define CFP_FLOWID_INT_TOTAL            (WLC_MAXBSSCFG + 1 + 1)

#define CFP_FLOWID_SCB_STARTID          (1)
#define CFP_FLOWID_INT_STARTID          (MAXSCB + 1)

#define CFP_FLOWS_MAX \
	(CFP_FLOWID_RSVD_TOTAL + CFP_FLOWID_SCB_TOTAL + CFP_FLOWID_INT_TOTAL)

#define CFP_FLOWID_INVALID              (CFP_FLOWS_MAX)
#define CFP_FLOWID_RESERVED             (0)
#define CFP_FLOWID_VALID(id)            (((uint16)(id) != CFP_FLOWID_INVALID) && \
					((uint16)(id) != CFP_FLOWID_RESERVED) && \
					((uint16)(id) < CFP_FLOWS_MAX))
/** Debug Asserts */
#define ASSERT_CFP_FLOWID(id)           ASSERT(CFP_FLOWID_VALID(id))
#define ASSERT_CFP_PRIO(prio)           ASSERT(prio < NUMPRIO)

/** Fetch wireless tx packet exptime. */
/** Check if CFP enabled for given ID. Fetch wireless tx packet exptime. */
#if defined(DONGLEBUILD)
extern bool wlc_cfp_tx_enabled(int cfp_unit, uint16 cfp_flowid,
	uint32* cfp_exptime);
#else  /* ! DONGLEBUILD */
struct wlc_info;
extern uint32 wlc_cfp_exptime(struct wlc_info *wlc);
extern bool wlc_cfp_tx_enabled(int cfp_unit, uint16 cfp_flowid, uint32 prio);
#endif /* ! DONGLEBUILD */

/** Prepare a CFP capable packet by setting up its PKTTAG */
extern void wlc_cfp_pkt_prepare(int cfp_unit, uint16 cfp_flowid, uint8 pkt_prio,
	void *pkt, uint32 cfp_exptime);

/** Entry point of Wireless CFP capable transmit fast path */
extern void wlc_cfp_tx_sendup(int cfp_unit, uint16 cfp_flowid, uint8 pkt_prio,
	void *pktlist_head, void *pktlist_tail, uint16 pkt_count);

#endif /* WLCFP */
#ifdef WLSQS
/** Wireless SQS entry point. */
extern int wlc_sqs_sendup(uint16 sqs_flowid, uint8 prio, uint16 pkt_count);

/** Entry point for v2r packets */
extern void wlc_sqs_v2r_sendup(uint16 sqs_flowid, uint8 prio,
	void *pktlist_head, void *pktlist_tail, uint16 pkt_count);

extern bool wlc_sqs_capable(uint16 cfp_flowid, uint8 prio);
extern uint16 wlc_sqs_vpkts(uint16 sqs_flowid, uint8 prio);
extern uint16 wlc_sqs_v2r_pkts(uint16 cfp_flowid, uint8 prio);
extern void wlc_sqs_v2r_enqueue(uint16 cfp_flowid, uint8 prio, int *v2r_request);
extern void wlc_sqs_v2r_dequeue(uint16 cfp_flowid, uint8 prio, uint16 pkt_count);
extern void wlc_sqs_v2r_revert(uint16 cfp_flowid, uint8 prio, uint16 v2r_reverts);
extern void wlc_sqs_vpkts_enqueue(uint16 cfp_flowid, uint8 prio, uint16 v_pkts);
extern void wlc_sqs_vpkts_rewind(uint16 cfp_flowid, uint8 prio, uint16 count);
extern void wlc_sqs_vpkts_forward(uint16 cfp_flowid, uint8 prio, uint16 count);
extern bool wlc_sqs_stride_paused(uint8 prio);
#endif /* WLSQS */
#endif /* _d11_cfg_h_ */
