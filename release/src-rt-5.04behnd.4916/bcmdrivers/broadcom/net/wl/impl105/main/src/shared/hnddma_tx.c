/*
 * Generic Broadcom Home Networking Division (HND) DMA transmit routines.
 * This supports the following chips: BCM42xx, 44xx, 47xx .
 *
 * Copyright (C) 2024, Broadcom. All Rights Reserved.
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
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: hnddma_tx.c 837195 2024-03-01 20:29:00Z $
 */

/**
 * @file
 * @brief
 * Source file for HNDDMA module. This file contains the functionality for the TX data path.
 */

#include <bcm_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <bcmdevs.h>
#include <osl.h>
#include <bcmendian.h>
#include <hndsoc.h>
#include <bcmutils.h>
#include <siutils.h>

#include <sbhnddma.h>
#include <hnddma.h>
#include "hnddma_priv.h"

#ifdef BCM_HWALITE
#include <mlc_export.h>
#endif // endif

/* Packets allocated from BPM and have dirty set, can avail of smart cache-op */
#if defined(BCM_NBUFF_PKT) && (defined(CONFIG_BCM_BPM) || \
	defined(CONFIG_BCM_BPM_MODULE))
#define BCM_NBUFF_PKT_BPM /* Packet data buffers, are from BPM pool */
#ifndef BCMDBG
#define BCM_NBUFF_PKT_BPM_COHERENCY /* Smart cache-flush of dirty lines */
#endif // endif
#endif /* BCM_NBUFF_PKT && CONFIG_BCM_BPM */

#ifdef BCMLFRAG
static int dma64_txfast_lfrag(dma_info_t *di, void *p0, bool commit);
#endif /* BCMLFRAG */

#if defined(BULK_DESCR_FLUSH)
static void BCMFASTPATH
dma_bulk_descr_tx_map(hnddma_t *dmah, uint16 start, bool is_txout)
{
#if !defined(OSL_CACHE_COHERENT)
	dma_info_t *di = DI_INFO(dmah);
	uint16 flush_cnt;

	// di->txout is the next available descriptor.
	flush_cnt = dma_get_txd_count(dmah, start, is_txout);

	if (flush_cnt) {
		// di can be TX or AQM
		if (start > di->txout) {
			DMA_MAP(di->osh, DMA_AQM_DESC_SHORT(di) ? dma64_txd64_short(di, 0) :
				dma64_txd64(di, 0), DMA64_FLUSH_LEN(di, di->txout),
				DMA_TX, NULL, NULL);
			flush_cnt -= di->txout;
		}
		DMA_MAP(di->osh, DMA_AQM_DESC_SHORT(di) ? dma64_txd64_short(di, start) :
			dma64_txd64(di, start), DMA64_FLUSH_LEN(di, flush_cnt),
			DMA_TX, NULL, NULL);
	}
#endif /* ! OSL_CACHE_COHERENT */
}

bool BCMFASTPATH
dma_bulk_descr_tx_is_active(hnddma_t *dmah)
{
	return (DMA_BULK_DESCR_TX_IS_VALID(DI_INFO(dmah)));
}

/* This API must be called by any user to mark start of user bulk dma tx
 */
void BCMFASTPATH
dma_bulk_descr_tx_start(hnddma_t *dmah)
{
	dma_info_t *di = DI_INFO(dmah);
	/* save the current txout */
	di->bulk_descr_tx_start_txout = di->txout;
}

/* This API must be called by any user to cancel user bulk dma tx in case of errors */
void BCMFASTPATH
dma_bulk_descr_tx_cancel(hnddma_t *dmah)
{
	/* save the current txout */
	DMA_BULK_DESCR_TX_SET_INVALID(DI_INFO(dmah));
}

/* If an user is performing bulk dma tx, this api must be called (instead of dma_comit)
 * at the end of bulk dma tx.
 * A dma_bulk_map is done here.
 */
void BCMFASTPATH
dma_bulk_descr_tx_commit(hnddma_t *dmah, bool aqm_fifo)
{
	dma_info_t *di = DI_INFO(dmah);

	/* Make sure first user dma bulk tx is indeed active */
	ASSERT(DMA_BULK_DESCR_TX_IS_VALID(di));

	if (DMA_BULK_DESCR_TX_IS_VALID(di)) {
		dma_bulk_descr_tx_map(dmah, di->bulk_descr_tx_start_txout, TRUE);
		/* reset the bulk dma dd idx */
		DMA_BULK_DESCR_TX_SET_INVALID(di);
	}

	/* issue the commit */
	dma_txcommit_ext(dmah, aqm_fifo);
}
#endif  /* BULK_DESCR_FLUSH */

int BCMFASTPATH
dma_txreclaim(hnddma_t *dmah, txd_range_t range)
{
	dma_info_t *di = DI_INFO(dmah);

	void *p;
	int pktcnt = 0;

	DMA_TRACE(("%s: dma_txreclaim %s\n", di->name,
	           (range == HNDDMA_RANGE_ALL) ? "all" :
	           ((range == HNDDMA_RANGE_TRANSMITTED) ? "transmitted" : "transfered")));

	if (di->txin == di->txout)
		return 0;

	/* if this is decriptor only DMA then just reset the txin. No data packets to free */
	if (DMA_AQM_DESC(di)) {
		DMA_TRACE(("%s: DESC only DMA. Seting txin=txout=%d \n", di->name, di->txout));
		di->txin = di->txout;
		di->hnddma.txavail = di->ntxd - 1;
		return 0;
	}

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel(dmah, FALSE);
	}

	while ((p = dma_getnexttxp(dmah, range))) {
		/* For unframed data, we don't have any packets to free */
		if (!(di->hnddma.dmactrlflags & DMA_CTRL_UNFRAMED)) {
			PKTFREE(di->osh, p, TRUE);
			pktcnt++;
		}
	}

	return pktcnt;
}

#if defined(WL_SPC)
static bool BCMFASTPATH
is_last_frag(uint8 index, uint8 nfrags)
{
	return (index + 1) == nfrags;
}

/*
 * This procedure takes a single data buffer and its length for DMA. It is
 * actually agnostic to WL_SPC and _dma_txfast's pkt->next loop can be replaced
 * with this later, if we decide to drop support for DMASGLIST_ENAB for good.
 * *flags is returned as well because of "if last txd eof not set, fix it" code
 * which probably can be done in this routine too.
 */
static int BCMFASTPATH
_dma_txfast_buff(dma_info_t *di, void *p0, bool commit, uchar *data, uint len,
		uint32 *flags, bool first, bool last, uint16 txout)
{
	dmaaddr_t pa;

#ifdef BCM_DMAPAD
	if (DMAPADREQUIRED(di)) {
		len += PKTDMAPAD(di->osh, p);
	}
#endif /* BCM_DMAPAD */

	/* return nonzero if out of tx descriptors */
	if (NEXTTXD(txout) == di->txin)
		return BCME_NORESOURCE;

	if (len == 0)
		return BCME_OK;

	/* get physical address of buffer start */
#ifdef BCM_SECURE_DMA
	pa = SECURE_DMA_MAP(di->osh, data, len, DMA_TX, NULL, NULL,
			&di->sec_cma_info_tx, 0, SECDMA_TXBUF_POST);
#else  /* ! BCM_SECURE_DMA */
	OSL_VIRT_TO_PHYSADDR(di->osh, data, pa);
#endif /* BCM_SECURE_DMA */

	*flags = 0;

	if (first) {
		*flags |= D64_CTRL1_SOF;
	}

	if (last) {
		/* Set "interrupt on completion" bit only on last commit packet
		 * to reduce the Tx completion event
		 */
		*flags |= D64_CTRL1_EOF;
		if (commit)
			*flags |= D64_CTRL1_IOC;
	}

	if (txout == (di->ntxd - 1))
		*flags |= D64_CTRL1_EOT;

	if (di->burstsize_ctrl)
		*flags |= D64_CTRL1_NOTPCIE;

	dma64_dd_upd(di, di->txd64, pa, txout, flags, len);
#ifdef BULK_PKTLIST_DEBUG
	ASSERT(di->txp[txout] == NULL);
#else
	if (!DMA_BULK_PATH(di)) {
		/* bump the tx descriptor index */
		ASSERT(di->txp[txout] == NULL);
	}
#endif // endif
	return BCME_OK;
}

/**
 * !! tx entry routine
 * WARNING: call must check the return value for error.
 *   the error(toss frames) could be fatal and cause many subsequent hard to debug problems
 */
static int BCMFASTPATH
dma_txfast_spc(dma_info_t *di, void *p0, bool commit)
{
	uint16 txout = di->txout;
	uint32 flags = 0;
	uint8 nfrags, i = 0;
	int ret;
	bool war, is_last;

	DMA_TRACE(("%s: %s\n", di->name, __FUNCTION__));

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	txout = di->txout;
	war = (di->hnddma.dmactrlflags & DMA_CTRL_DMA_AVOIDANCE_WAR) ? TRUE : FALSE;

	nfrags = PKT_FRAG_CNT(di->osh, p0);

	/* the very first subframe is the last if there are no fragments */
	is_last = !nfrags;
	/*
	 * The first segment is the 'data' field of the SKB, so we just DMA it.
	 * Note that the PKTLEN is the length of *just this segment*
	 */
	ret = _dma_txfast_buff(di, p0, commit,
			PKTDATA(di->osh, p0), PKTLEN(di->osh, p0),
			&flags, TRUE, is_last, txout);

	if (ret == BCME_NORESOURCE) {
		goto spc_outoftxd;
	}
	txout = NEXTTXD(txout);

	for (i = 0; i < nfrags; i++) {
		is_last = is_last_frag(i, nfrags);
		/*
		 * this is the part that differs from the base _dma_txfast()
		 * implementation. Instead of walking through subframes with
		 * PKTNEXT, we walk through the segments attached to this one
		 * SKB.
		 */
		ret = _dma_txfast_buff(di, p0, commit,
				(uchar *) PKT_SEG_DATA(di->osh, p0, i),
				PKT_SEG_LEN(di->osh, p0, i),
				&flags, FALSE, is_last, txout);

		if (ret == BCME_NORESOURCE) {
			goto spc_outoftxd;
		}
		txout = NEXTTXD(txout);
	}

	/* if last txd eof not set, fix it */
	if (!(flags & D64_CTRL1_EOF))
		W_SM(&di->txd64[PREVTXD(txout)].ctrl1,
		     BUS_SWAP32(flags | D64_CTRL1_IOC | D64_CTRL1_EOF));

#ifdef BULK_PKTLIST_DEBUG
	/* save the packet */
	di->txp[PREVTXD(txout)] = p0;
#else
	if (!DMA_BULK_PATH(di)) {
		/* save the packet */
		di->txp[PREVTXD(txout)] = p0;
	}
#endif // endif

	/* bump the tx descriptor index */
	di->txout = txout;

	/* kick the chip */
	if (commit) {
		dma64_txcommit_local(di, FALSE);
	}

	/* tx flow control */
	di->hnddma.txavail = di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1;

	return 0;

spc_outoftxd:
	DMA_ERROR(("%s: %s: out of txds !!!\n", di->name, __FUNCTION__));
	di->hnddma.txavail = 0;
	di->hnddma.txnobuf++;
	return -1;
}
#endif /* WL_SPC */
/**
 * !! tx entry routine
 * WARNING: call must check the return value for error.
 *   the error(toss frames) could be fatal and cause many subsequent hard to debug problems
 */
static int BCMFASTPATH
_dma_txfast(dma_info_t *di, void *p0, bool commit)
{
	void *p, *next;
	uchar *data;
	uint len;
	uint16 txout;
	uint32 flags = 0;
	dmaaddr_t pa;
	bool war;
#if defined(BCMDBG)
	int numd = 0;
#endif /* BCMDBG */

	DMA_TRACE(("%s: dma_txfast\n", di->name));

#ifdef BCMHWA
	if (DMA_HWA_TX(di)) {
		DMA_ERROR(("%s: Why go through SW txfast when HWA TX is enabled?\n",
			__FUNCTION__));
		ASSERT(0);
	}
#endif /* BCMHWA */

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	/* new DMA routine for LFRAGS */
#ifdef BCMLFRAG
	if (BCMLFRAG_ENAB()) {
		if (PKTISTXFRAG(di->osh, p0)) {
			return dma64_txfast_lfrag(di, p0, commit);
		}
	}
#endif // endif

#ifdef WL_SPC
	if (PKTISSPC(di->osh, p0)) {
		return dma_txfast_spc(di, p0, commit);
	}
#endif /* WL_SPC */
	txout = di->txout;
	war = (di->hnddma.dmactrlflags & DMA_CTRL_DMA_AVOIDANCE_WAR) ? TRUE : FALSE;

	/*
	 * Walk the chain of packet buffers
	 * allocating and initializing transmit descriptor entries.
	 */
	for (p = p0; p; p = next) {
		uint nsegs, j, segsadd;
		hnddma_seg_map_t *map = NULL;

		data = PKTDATA(di->osh, p);
		len = PKTLEN(di->osh, p);
		next = PKTNEXT(di->osh, p);
#ifdef BCM_DMAPAD
		if (DMAPADREQUIRED(di)) {
			len += PKTDMAPAD(di->osh, p);
		}
#endif /* BCM_DMAPAD */

		/* return nonzero if out of tx descriptors */
		if (NEXTTXD(txout) == di->txin)
			goto outoftxd;

		if (len == 0)
			continue;

		/* get physical address of buffer start */

#if !defined(BCM_SECURE_DMA)
		if (DMASGLIST_ENAB)
			bzero(&di->txp_dmah[txout], sizeof(hnddma_seg_map_t));
#endif // endif

#ifdef BCM_SECURE_DMA
		if (DMASGLIST_ENAB) {
			bzero(&di->txp_dmah[txout], sizeof(hnddma_seg_map_t));
			pa = SECURE_DMA_MAP(di->osh, data, len, DMA_TX, p,
				&di->txp_dmah[txout], &di->sec_cma_info_tx, 0, SECDMA_TXBUF_POST);
		} else {
			pa = SECURE_DMA_MAP(di->osh, data, len, DMA_TX, NULL, NULL,
				&di->sec_cma_info_tx, 0, SECDMA_TXBUF_POST);
		}
#else  /* ! BCM_SECURE_DMA */
#if defined(OSL_CACHE_COHERENT)
		OSL_VIRT_TO_PHYSADDR(di->osh, data, pa);
#else
#if !defined(BCM_NBUFF_PKT_BPM)
		pa = DMA_MAP(di->osh, data, len, DMA_TX, p, &di->txp_dmah[txout]);
#else
		{
#if defined(BCM_NBUFF_PKT_BPM_COHERENCY) && defined(CC_BPM_SKB_POOL_BUILD)
			uchar *dirty = PKTDIRTYP(di->osh, p);
			if (dirty && ((dirty - data) > 0) && ((dirty - data) <= len))
				/* XXX: WAR for BCAWLAN-210771,
				 * Sometimes, we got the invalid dirty pointer.
				 * If we use it without checking, kernel paging request failed.
				 * Checking the dirty value before use it as WAR.
				 */
				pa = DMA_MAP(di->osh, data, (dirty - data), DMA_TX, p,
					&di->txp_dmah[txout]);
			else
#endif /* BCM_NBUFF_PKT_BPM_COHERENCY */
			pa = DMA_MAP(di->osh, data, len, DMA_TX, p, &di->txp_dmah[txout]);
		}
#endif /* BCM_NBUFF_PKT_BPM */
#endif /* ! OSL_CACHE_COHERENT */
#endif /* BCM_SECURE_DMA */

		if (DMASGLIST_ENAB) {
			map = &di->txp_dmah[txout];

			/* See if all the segments can be accounted for */
			if (map->nsegs > (uint)(di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1))
				goto outoftxd;

			nsegs = map->nsegs;
		} else
			nsegs = 1;

		segsadd = 0;
		for (j = 1; j <= nsegs; j++) {
			flags = 0;

			if (p == p0 && j == 1) {
				flags |= D64_CTRL1_SOF;
#if defined(BCMDBG)
				numd = 1;
#endif /* BCMDBG */
			}

			/* With a DMA segment list, Descriptor table is filled
			 * using the segment list instead of looping over
			 * buffers in multi-chain DMA. Therefore, EOF for SGLIST is when
			 * end of segment list is reached.
			 */
			if ((!DMASGLIST_ENAB && next == NULL) ||
			    (DMASGLIST_ENAB && j == nsegs)) {
				/* Set "interrupt on completion" bit only on last commit packet
				 * to reduce the Tx completion event
				 */
				flags |= D64_CTRL1_EOF;
				if (commit)
					flags |= D64_CTRL1_IOC;
			}

			if (txout == (di->ntxd - 1))
				flags |= D64_CTRL1_EOT;

			if (di->burstsize_ctrl)
				flags |= D64_CTRL1_NOTPCIE;

			if (DMASGLIST_ENAB) {
				len = map->segs[j - 1].length;
				pa = map->segs[j - 1].addr;
				if (len > 128 && war) {
					uint remain, new_len, align64;
					/* check for 64B aligned of pa */
					align64 = (uint)(PHYSADDRLO(pa) & 0x3f);
					align64 = (64 - align64) & 0x3f;
					new_len = len - align64;
					remain = new_len % 128;
					if (remain > 0 && remain <= 4) {
						uint32 buf_addr_lo;
						uint32 tmp_flags =
							flags & (~(D64_CTRL1_EOF | D64_CTRL1_IOC));
						flags &= ~(D64_CTRL1_SOF | D64_CTRL1_EOT);
						remain += 64;
						dma64_dd_upd(di, di->txd64, pa, txout,
							&tmp_flags, len-remain);
#ifdef BULK_PKTLIST_DEBUG
						ASSERT(di->txp[txout] == NULL);
#else
						if (!DMA_BULK_PATH(di)) {
							ASSERT(di->txp[txout] == NULL);
						}
#endif // endif
						txout = NEXTTXD(txout);
						/* return nonzero if out of tx descriptors */
						if (txout == di->txin) {
							DMA_ERROR(("%s: dma_txfast: Out-of-DMA"
								" descriptors (txin %d txout %d"
								" nsegs %d)\n", __FUNCTION__,
								di->txin, di->txout, nsegs));
							goto outoftxd;
						}
						if (txout == (di->ntxd - 1))
							flags |= D64_CTRL1_EOT;
						buf_addr_lo = PHYSADDRLO(pa);
						PHYSADDRLOSET(pa, (PHYSADDRLO(pa) + (len-remain)));
						if (PHYSADDRLO(pa) < buf_addr_lo) {
							PHYSADDRHISET(pa, (PHYSADDRHI(pa) + 1));
						}
						len = remain;
						segsadd++;
						di->dma_avoidance_cnt++;
					}
				}
			}
#if defined(BCMDBG)
			if (numd > BCM_DMA_AQM_MAX_NUMD) {
				DMA_ERROR(("%s %d: numd %d > 15 (out of numd), flags 0x%08x \n",
				__func__, __LINE__, numd, flags));
				ASSERT(0);
			}
#endif /* BCMDBG */
			dma64_dd_upd(di, di->txd64, pa, txout, &flags, len);
#ifdef BULK_PKTLIST_DEBUG
			ASSERT(di->txp[txout] == NULL);
#else
			if (!DMA_BULK_PATH(di)) {
				/* bump the tx descriptor index */
				ASSERT(di->txp[txout] == NULL);
			}
#endif // endif
			txout = NEXTTXD(txout);
#if defined(BCMDBG)
			numd++;
#endif /* BCMDBG */
			/* return nonzero if out of tx descriptors */
			if (txout == di->txin) {
				DMA_ERROR(("%s: dma_txfast: Out-of-DMA descriptors"
					   " (txin %d txout %d nsegs %d)\n", __FUNCTION__,
					   di->txin, di->txout, nsegs));
				goto outoftxd;
			}
		}
		if (segsadd && DMASGLIST_ENAB)
			map->nsegs += segsadd;

		/* See above. No need to loop over individual buffers */
		if (DMASGLIST_ENAB)
			break;
	}

	/* if last txd eof not set, fix it */
	if (!(flags & D64_CTRL1_EOF))
		W_SM(&di->txd64[PREVTXD(txout)].ctrl1,
		     BUS_SWAP32(flags | D64_CTRL1_IOC | D64_CTRL1_EOF));

#ifdef BULK_PKTLIST_DEBUG
	/* save the packet */
	di->txp[PREVTXD(txout)] = p0;
#else
	if (!DMA_BULK_PATH(di)) {
		/* save the packet */
		di->txp[PREVTXD(txout)] = p0;
	}
#endif // endif

#if !defined(OSL_CACHE_COHERENT)
#if defined(BULK_DESCR_FLUSH)
	if (!DMA_BULK_DESCR_TX_IS_VALID(di)) {
		uint32 flush_cnt = NTXDACTIVE(di->txout, txout);
		/* Should not be AQM */
		ASSERT(!DMA_AQM_DESC_SHORT(di));
		if (txout < di->txout) {
			DMA_MAP(di->osh, dma64_txd64(di, 0), DMA64_FLUSH_LEN(di, txout),
				DMA_TX, NULL, NULL);
			flush_cnt -= txout;
		}
		DMA_MAP(di->osh, dma64_txd64(di, di->txout), DMA64_FLUSH_LEN(di, flush_cnt),
		        DMA_TX, NULL, NULL);
	}
#endif  /* BULK_DESCR_FLUSH */
#endif  /* !OSL_CACHE_COHERENT */

	/* bump the tx descriptor index */
	di->txout = txout;

	/* kick the chip */
	if (commit) {
		dma64_txcommit_local(di, FALSE);
	}

	/* tx flow control */
	di->hnddma.txavail = di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1;

	return (0);

outoftxd:
	DMA_ERROR(("%s: dma_txfast: out of txds !!!\n", di->name));
	di->hnddma.txavail = 0;
	di->hnddma.txnobuf++;
	return (-1);
}

#ifdef BULK_PKTLIST
/* General description
 * ===================
 * The Bulk DMA enqueue maintains a linked list of packets enqueued for transmission
 * by the DMA hardware.
 * Previously this was stored in a parallel ring of buffers (txp) to the actual DMA descriptor ring
 *
 * In this method, the enqueued packets are held in a linked list and the head of the list is
 * maintained by the DMA di control structure.
 *
 * A new packet is appended to the end of this pending list,
 * the mapping proceeds as before as does the
 * special per-packet handling of SDF and LFRAG packets.
 *
 * Optionally this can be bulk mapped at the end if there is a chain of packets but this determines
 * on the specific implementation and features such as Secure DMA or the SGLIST processing.
 *
 * dma_txfast() is a default case of routine with a list containing 1 packet
 *
 * Detailed description
 * ====================
 *
 * Bulk DMA Enqueue accepts di and list of packets - txlist.
 *
 * List of packets are a 2 way linked list.
 *  Pkt heads joined to the next packet via  PKTLINK(). NULL terminates pkt list
 *  Pkt buffers joined via PKTNEXT(). NULL terminated buffer list
 *
 * di structure contains a list_head and list_tail pointer.
 *
 * Accessor macros handle the packet manipulations using PKTLINK() and PKTNEXT()
 *
 * Extract initial packet from tx_list.
 *
 * Save current dma index into map_start
 *
 * Iterate over list of packets  in txlist{
 *	for each buffer p in packet  {
 *		map and save buffer dma index (last_dma_index)
 *		program DMA information for the buffer p
 *		increment dma index (di->txout)
 *		get next buffer in packet
 *	}
 *	Note:Last dma index stored in first buffer of p0, to prevent iterating down buffer list.
 *	Set lb->dma_index of p0 to last_dma_index
 *	get next packet in chain
 * }
 *
 * append txlist to pending dma chain.
 * set di->list_tail pointer to last processed packet
 *
 * save dma index of last buffer map_end
 *
 * map_start and map_end is optionally used when bulk descriptor mapping is needed.
 *
 * Map descriptors if needed
 *
 * Commit DMA if needed.
 */
static int BCMFASTPATH
_dma_bulk_tx(dma_info_t *di, void *tx_list, bool do_commit)
{
	uint16 start;
	uint16 last_dma_index;
	void *current_pkt, *previous_pkt;
	int rc = BCME_OK;

#ifdef BULK_PKTLIST_DEBUG
	volatile uint npkt = 0;
#endif // endif
	DMA_TRACE(("%s: dma_bulk_tx\n", di->name));

	if (!DMA_BULK_PATH(di)) {
		return _dma_txfast(di, tx_list, do_commit);
	}

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	/* get first packet of the tx packet list */
	current_pkt = tx_list;
	previous_pkt = NULL;
	start = di->txout;

	while (current_pkt) {
#ifdef BULK_PKTLIST_DEBUG
		di->dma_pkt_list.npkt++;
		npkt++;
#endif // endif
		rc = _dma_txfast(di, current_pkt, FALSE);
		if (rc != BCME_OK) {
			goto dma_txerr;
		}
		last_dma_index = PREVTXD(di->txout);

#ifdef BULK_PKTLIST_DEBUG
		DMA_TRACE(("dma64_txbulk:di:0x%p npkt:%d txin:%d txout:(%d->%d) "
			"index:%d p0:0x%p p1:0x%p\n",
			di, npkt, di->txin, map_start, di->txout,
			last_dma_index, p0, PKTLINK(p0)));
#endif // endif

		DMA_SET_INDEX(di->osh, current_pkt, last_dma_index);
		previous_pkt = current_pkt;
		/* Packets are nodes in a linked list, get the next node */
		current_pkt = DMA_GET_NEXTPKT(current_pkt);
	}

	/* Do bulk map here using map-start and map_end as limits */
	BULK_DMA_MAP(di->osh, di, start, last_dma_index);

	/* Append list of new packets to the pending DMA list */
	if (DMA_GET_LISTHEAD(di)) {
		DMA_APPEND_LIST(DMA_GET_LISTTAIL(di), tx_list);
	} else {
		DMA_SET_LISTHEAD(di, tx_list);
	}

	DMA_SET_LISTTAIL(di, previous_pkt);

	if (do_commit) {
		dma64_txcommit_local(di, FALSE);
	}
	return (0);

dma_txerr:
	DMA_ERROR(("%s: dma64_txbulk: dma_error !!!\n", di->name));
	return (rc);
}
#endif /* BULK_PKTLIST */

int
dma_txfast(hnddma_t *dmah, void *p0, bool commit)
{
#ifdef BULK_PKTLIST
	return _dma_bulk_tx(DI_INFO(dmah), p0, commit);
#else
	return _dma_txfast(DI_INFO(dmah), p0, commit);
#endif // endif
}

/** get the address of the var in order to change later */
uintptr
dma_getvar(hnddma_t *dmah, const char *name)
{
	dma_info_t *di = DI_INFO(dmah);

	if (!strcmp(name, "&txavail"))
		return ((uintptr) &(di->hnddma.txavail));
	else if (!strcmp(name, "&rxavail"))
		return ((uintptr) &(di->rxavail));
	else {
		ASSERT(0);
	}
	return (0);
}

/**
 * Helper routine to do commit only operation on descriptors
 * Externally txcommit visible call
 */
void
dma_txcommit(hnddma_t *dmah)
{
	if (DMA_INDIRECT(DI_INFO(dmah))) {
		dma_set_indqsel(dmah, FALSE);
	}

	dma64_txcommit_local(DI_INFO(dmah), FALSE);
}

void
dma_txcommit_ext(hnddma_t *dmah, bool aqm_fifo)
{
	if (DMA_INDIRECT(DI_INFO(dmah))) {
		dma_set_indqsel(dmah, FALSE);
	}

	dma64_txcommit_local(DI_INFO(dmah), aqm_fifo);
}

static void BCMFASTPATH
dma64_dd_upd_64_from_struct(dma_info_t *di, dma64dd_t *ddring, dma64dd_t *dd, uint outidx)
{
	/* bit 63 is arleady set for host addresses by the caller */
	if (DMA_AQM_DESC_SHORT(di)) {
		dma64dd_short_t *ddring_short = (dma64dd_short_t *)ddring;
		W_SM(&ddring_short[outidx].ctrl1, BUS_SWAP32(dd->ctrl1));
		W_SM(&ddring_short[outidx].ctrl2, BUS_SWAP32(dd->ctrl2));
		if ((di->hnddma.dmactrlflags & DMA_CTRL_PEN) &&
			DMA64_DD_SHORT_PARITY(&ddring_short[outidx])) {
			W_SM(&ddring_short[outidx].ctrl2, BUS_SWAP32(dd->ctrl2 | D64_CTRL2_PARITY));
		}
#if (defined(__ARM_ARCH_7A__) && defined(CA7)) || (defined(__ARM_ARCH_8A__) && \
	defined(CA53))
		/* memory barrier before posting the descriptor */
		DMB();
#endif // endif
#if !defined(OSL_CACHE_COHERENT)
#if (defined(BCM_ROUTER) && !defined(BULK_DESCR_FLUSH))
		DMA_MAP(di->osh, (void *)(((unsigned long)(&ddring_short[outidx])) & ~0x1f),
			32, DMA_TX, NULL, NULL);
#endif // endif
#endif /* ! OSL_CACHE_COHERENT */
	} else {
		W_SM(&ddring[outidx].addrlow, BUS_SWAP32(dd->addrlow));
		W_SM(&ddring[outidx].addrhigh, BUS_SWAP32(dd->addrhigh));
		W_SM(&ddring[outidx].ctrl1, BUS_SWAP32(dd->ctrl1));
		W_SM(&ddring[outidx].ctrl2, BUS_SWAP32(dd->ctrl2));
		if ((di->hnddma.dmactrlflags & DMA_CTRL_PEN) &&
			DMA64_DD_PARITY(&ddring[outidx])) {
			W_SM(&ddring[outidx].ctrl2, BUS_SWAP32(dd->ctrl2 | D64_CTRL2_PARITY));
		}
#if (defined(__ARM_ARCH_7A__) && defined(CA7)) || (defined(__ARM_ARCH_8A__) && \
	defined(CA53))
		/* memory barrier before posting the descriptor */
		DMB();
#endif // endif
#if !defined(OSL_CACHE_COHERENT)
#if (defined(BCM_ROUTER) && !defined(BULK_DESCR_FLUSH))
		DMA_MAP(di->osh, (void *)(((unsigned long)(&ddring[outidx])) & ~0x1f), 32, DMA_TX,
			NULL, NULL);
#endif // endif
#endif /* ! OSL_CACHE_COHERENT */
	}
}

/* Routine to post a descriptor. It requires the caller to send in partially formatted
 * information for the descriptor.
 */
int
dma_txdesc(hnddma_t *dmah, dma64dd_t *dd, bool commit)
{
	uint16 txout;
	dma_info_t *di = DI_INFO(dmah);

	DMA_TRACE(("%s: dma_txdesc\n", di->name));
	txout = di->txout;

	/* return nonzero if out of tx descriptors */
	if (NEXTTXD(txout) == di->txin) {
		goto outoftxd;
	}

	/* fill in remaining bits in ctrl1 */
	if (di->burstsize_ctrl)
		dd->ctrl1 |= D64_CTRL1_NOTPCIE;

	/* dongle aqm desc need to have CO as the SOFD buffer comes from local
	 * memory.
	 */
	if (DMA_TRANSCOHERENT(di))
		dd->ctrl1 |= D64_CTRL1_COHERENT;
	if (txout == (di->ntxd - 1)) {
		dd->ctrl1 |= D64_CTRL1_EOT;
		if (DMA_PUQ_QREMAP(di)) {
			di->desc_wrap_ind++;
		}
	}

	DMA_TRACE(("%s: dma_txdesc Descriptor index = %d, dd->ctrl1 = 0x%x, dd->ctrl2 = 0x%x,"
		"dd->addrlow = 0x%x, dd->addrhigh = 0x%x \n", di->name, txout, dd->ctrl1,
		dd->ctrl2, dd->addrlow, dd->addrhigh));

	/* load the descriptor */
	dma64_dd_upd_64_from_struct(di, di->txd64, dd, txout);

	txout = NEXTTXD(txout);

#if !defined(OSL_CACHE_COHERENT)
#if defined(BULK_DESCR_FLUSH)
	if (!DMA_BULK_DESCR_TX_IS_VALID(di)) {
		DMA_MAP(di->osh,  DMA_AQM_DESC_SHORT(di) ? dma64_txd64_short(di, di->txout) :
			dma64_txd64(di, di->txout), DMA64_FLUSH_LEN(di, 1), DMA_TX, NULL, NULL);
	}
#endif /* BULK_DESCR_FLUSH */
#endif /* ! OSL_CACHE_COHERENT */

	/* bump the descriptor index */
	di->txout = txout;

	/* If commit is set, write the DMA register to inform the DMA of the new descriptor */
	if (commit) {
		dma64_txcommit_local(di, TRUE);
	}

	di->hnddma.txavail = di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1;

	return (0);

outoftxd:
	DMA_ERROR(("%s: dma_txdesc: out of txds !!!\n", di->name));
	di->hnddma.txavail = 0;
	di->hnddma.txnobuf++;
	return (-1);
}

/**
 * Reclaim next completed txd (txds if using chained buffers) in the range
 * specified and return associated packet.
 * If range is HNDDMA_RANGE_TRANSMITTED, reclaim descriptors that have be
 * transmitted as noted by the hardware "CurrDescr" pointer.
 * If range is HNDDMA_RANGE_TRANSFERED, reclaim descriptors that have be
 * transfered by the DMA as noted by the hardware "ActiveDescr" pointer.
 * If range is HNDDMA_RANGE_ALL, reclaim all txd(s) posted to the ring and
 * return associated packet regardless of the value of hardware pointers.
 */

#ifdef BULK_PKTLIST
/*
 * General description
 * ===================
 * Bulk TX completion of the DMA linked list is determined by the number of packets to consume
 * This is specified by the caller via ncons
 * A packet consists of several data buffers, the start index is the current di->txout
 * and the end index is embedded in the lbuf of the first packet (for FD mode) or
 * sk_buff (for NIC Mode).
 * This routine extracts a sublist of ncons packets, removes this from the internal DMA list
 * and returns this to the caller, with an  optional pointer to the last packet in this sublist.
 *
 * dma_getnexttxp() is a subset of this with ncons=1
 *
 * Detail description
 * ==================
 * Routine accepts:
 *	data fifo (di)
 *	packet pointers to list head (list_head) and list tail (list_tail)
 *	max number of packets to consume (ncons)
 *	Actual implementation below calculated the DMA index range, this is not mentioned to
 *	simplify the description.
 *
 *  List of packets are a 2 way linked list.
 *   Pkt heads joined via  PKTLINK(). NULL terminates pkt list
 *   Pkt buffers joined via PKTNEXT(). NULL terminated buffer list
 *
 * Extract dma ring index, to be used for later bulk unmapping of descriptors.
 *
 * Set *list_head to point to this packet.
 * di->list_head is the internal pending list of packets.
 *
 * while (npkts < ncons) {
 *	 save current di->list_head into previous_p pointer
 *	 set di->list head to the next packet in chain
 *	 if di->list_head is NULL, all packets have been consumed
 *	 if the packets consumed is less than ncons, return error to caller
 * }
 *
 *
 * set *list_tail to previous_p
 *
 * Extract dma index of last buffer from previous_p.
 * Last dma index stored in first buffer of prev_p, to prevent iterating down buffer list.
 *
 * Perform bulk DMA unmap of descriptors if required.
 *
 * set di->txin = NEXTTXD(unmap_end)
 *
 * Return list head and list end pointers to caller
 *
 * return rc code.
 *
 * Note: In the case of error, return list of packets up to the point of error,
 * caller can decide on course of action.
 */

int BCMFASTPATH
dma_bulk_txcomplete(hnddma_t *dmah, uint16 ncons, uint16 *nproc,
		void **list_head, void **list_tail, txd_range_t range)
{
	dma_info_t *di = DI_INFO(dmah);
	void *current_pkt;
	uint16 start, end;
	uint16 active_desc;
	uint16 npkt;

	DMA_TRACE(("%s: dma_bulk_txcomplete\n", di->name));
	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel(dmah, FALSE);
	}

	if (di->ntxd == 0) {
		return (BCME_NOMEM);
	}

	if (ncons == 0) {
		return (BCME_BADARG);
	}

	current_pkt = NULL;

	start = di->txin;
	if (range == HNDDMA_RANGE_ALL)
	{
		end = di->txout;
	} else {
		if (di->txin == di->xs0cd) {
#ifdef BCM_HWALITE
			if (mlc_hwalite_is_enable(di->mlc_dev)) {
				uint16 cd = mlc_hwalite_txfifo_cd(di->mlc_dev,
					di->fifo, di->is_aqm, mlo_unit(di->mlc_dev));
				end = (uint16)((cd & D64_XS0_CD_MASK) -
					(B2I(di->xmtptrbase, di->dd64_size) & D64_XS0_CD_MASK));
			} else
#endif /* BCM_HWALITE */
			{
				end = DMA64_TX_CD(di);
			}
			di->xs0cd = end;
		} else {
			end = di->xs0cd;
		}

		if (range == HNDDMA_RANGE_TRANSFERED) {
			active_desc = DMA64_TX_AD(di);
			if (end != active_desc) {
				end = PREVTXD(active_desc);
			}
		}
	}

	if ((start == 0) && (end > di->txout))  {
		return BCME_RANGE;
	}

	if (start == end) {
		return BCME_RANGE;
	}

	*list_head = DMA_GET_LISTHEAD(di);
	ASSERT(*list_head);
	start = di->txin;
	end = di->txin;

	npkt = 0;
	/* Consume ncons packets from list */
	while (npkt != ncons) {
		current_pkt = DMA_GET_LISTHEAD(di);
		if (current_pkt) {
			/* Set list head to the next packet in the list */
			DMA_SET_LISTHEAD(di, DMA_GET_NEXTPKT(current_pkt));

			/* Required cleanup to lbuf dma_index field to
			 * avoid problems in pkt free and shared pool routines
			 * Save the next TXD in "end"
			 * and zero out the field in the lbuf
			 */
			end = DMA_GET_INDEX(di->osh, current_pkt);
#ifdef BULK_PKTLIST_DEBUG
			/* XXX shadow array consistency check
			 * Goes away once shadow array is removed
			 */
			ASSERT(current_pkt == di->txp[end]);
#endif // endif
			/* lbuf/sk_buff field used for dma index has to be zeroed out
			 * before sending packet back to WLAN layer to
			 * prevent unexpected crashes.
			 */
			DMA_SET_INDEX(di->osh, current_pkt, 0);
			npkt++;
		} else {
			/* npkt!=ncons and list head is NULL.
			 * Ran out of packets before consuming ncons pkts
			 * This is a hard error, return to caller.
			 */
			return BCME_BADARG;
		}
	}

	/* At the exit of the processing loop "end"
	 * points to the last buffer in the packet
	 * this is used for any subsequent unmap calls
	 */
	end = NEXTTXD(end);

	BULK_DMA_UNMAP(di->osh, di, start, end);

	di->txin = end;

	{
		/* XXX Cleanup txp shadow array.
		 * This goes away once the shadow array is removed.
		 * Prevents unwanted crashes in the code in the meantime
		 */
		uint16 i;
		for (i = start; i != end; i = NEXTTXD(i))
		{
#ifdef BULK_PKTLIST_DEBUG
			di->txp[i] = NULL;
#endif // endif
#ifdef DESCR_DEADBEEF
			W_SM(&di->txd64[i].addrlow, 0xdeadbeef);
			W_SM(&di->txd64[i].addrhigh, 0xdeadbeef);
#endif // endif
		}
#ifdef BULK_PKTLIST_DEBUG
		ASSERT(di->dma_pkt_list.npkt >= ncons);

		di->dma_pkt_list.npkt -= ncons;
		di->dma_pkt_list.last_txin = di->txin;
		di->dma_pkt_list.last_pkt = current_pkt;
#endif // endif
	}

	/* Zero the link of the last packet in the chain */
	PKTSETLINK(current_pkt, NULL);

	if (list_tail) {
		*list_tail = current_pkt;
	}

	if (nproc) {
		*nproc = npkt;
	}

	/* tx flow control */
	di->hnddma.txavail = di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1;

	return BCME_OK;
} /* dma_bulk_txcomplete */

#endif /* BULK_PKTLIST */

static void * BCMFASTPATH
_dma_getnexttxp(dma_info_t *di, txd_range_t range)
{
	uint16 start, end, i;
	uint16 active_desc;
	void *txp;

	DMA_TRACE(("%s: dma_getnexttxp %s\n", di->name,
			(range == HNDDMA_RANGE_ALL) ? "all" :
			((range == HNDDMA_RANGE_TRANSMITTED) ? "transmitted" : "transfered")));

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	if (di->ntxd == 0)
		return (NULL);

	txp = NULL;

	start = di->txin;
	if (range == HNDDMA_RANGE_ALL) {
		end = di->txout;
	} else {
		if (di->txin == di->xs0cd) {
#ifdef BCM_HWALITE
			if (mlc_hwalite_is_enable(di->mlc_dev)) {
				uint16 cd = mlc_hwalite_txfifo_cd(di->mlc_dev,
					di->fifo, di->is_aqm, mlo_unit(di->mlc_dev));
				end = (uint16)((cd & D64_XS0_CD_MASK) -
					(B2I(di->xmtptrbase, di->dd64_size) & D64_XS0_CD_MASK));
			} else
#endif /* BCM_HWALITE */
			{
				end = DMA64_TX_CD(di);
			}
			di->xs0cd = end;
		} else {
			end = di->xs0cd;
		}

		if (range == HNDDMA_RANGE_TRANSFERED) {
			active_desc = DMA64_TX_AD(di);
			if (end != active_desc) {
				end = PREVTXD(active_desc);
			}
		}
	}

	if ((start == 0) && (end > di->txout))
		goto bogus;

	for (i = start; i != end && !txp; i = NEXTTXD(i)) {
		/* dma 64-bit */
		hnddma_seg_map_t *map = NULL;
		uint j, nsegs;

#if (!defined(__mips__) && !(defined(STB) || defined(BCM_ROUTER))) || \
	defined(BCM_SECURE_DMA)
		dmaaddr_t pa;
		uint size;
		PHYSADDRLOSET(pa, (BUS_SWAP32(R_SM(&di->txd64[i].addrlow)) -
				di->dataoffsetlow));
		PHYSADDRHISET(pa, (BUS_SWAP32(R_SM(&di->txd64[i].addrhigh)) -
				di->dataoffsethigh));
#endif /* !defined(__mips__) && !(defined(STB) || defined(BCM_ROUTER)) */

		if (DMASGLIST_ENAB) {
			map = &di->txp_dmah[i];
#if (!defined(__mips__) && !(defined(STB) || defined(BCM_ROUTER))) || \
	defined(BCM_SECURE_DMA)
			size = map->origsize;
#endif // endif
			nsegs = map->nsegs;
			if (nsegs > (uint)NTXDACTIVE(i, end)) {
				di->xs0cd = i;
				break;
			}
		} else {
#if (!defined(__mips__) && !(defined(STB) || defined(BCM_ROUTER))) || \
	defined(BCM_SECURE_DMA)
			size = (BUS_SWAP32(R_SM(&di->txd64[i].ctrl2)) & D64_CTRL2_BC_MASK);
#endif // endif
			nsegs = 1;
		}

		for (j = nsegs; j > 0; j--) {
#if defined(DESCR_DEADBEEF)
			W_SM(&di->txd64[i].addrlow, 0xdeadbeef);
			W_SM(&di->txd64[i].addrhigh, 0xdeadbeef);
#endif // endif

#ifdef BULK_PKTLIST_DEBUG
			txp = di->txp[i];
			di->txp[i] = NULL;
#else
			if (!DMA_BULK_PATH(di)) {
				txp = di->txp[i];
				di->txp[i] = NULL;
			}
#endif // endif
			if (j > 1) {
				i = NEXTTXD(i);
			}
		}
#ifdef BCM_SECURE_DMA
			SECURE_DMA_UNMAP(di->osh, pa, size, DMA_TX,
				NULL, NULL, &di->sec_cma_info_tx, 0);
#else
#if !defined(OSL_CACHE_COHERENT)
#if (!defined(__mips__) && !(defined(STB) || defined(BCM_ROUTER)))
			DMA_UNMAP(di->osh, pa, size, DMA_TX, txp, map);
#endif // endif
#endif /* ! OSL_CACHE_COHERENT */
#endif /* BCM_SECURE_DMA */
	}

	di->txin = i;

	/* tx flow control */
	di->hnddma.txavail = di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1;

	return (txp);

bogus:
	DMA_NONE(("dma_getnexttxp: bogus curr: start %d end %d txout %d range %d\n",
			start, end, di->txout, range));
	return (NULL);
}

void * BCMFASTPATH
dma_getnexttxp(hnddma_t *dmah, txd_range_t range)
{
#ifdef BULK_PKTLIST
	if (DMA_BULK_PATH(DI_INFO(dmah))) {
		void *txp = NULL;

		dma_bulk_txcomplete(dmah, 1, NULL, &txp, NULL, range);
		return txp;
	} else
#endif // endif

	return (_dma_getnexttxp(DI_INFO(dmah), range));

}

/* Function to reclaim the next completed descriptors for DESCRIPTOR only DMA */
int
dma_getnexttxdd(hnddma_t *dmah, txd_range_t range, uint32 *flags)
{
	dma_info_t *di = DI_INFO(dmah);
	uint16 end = 0;
	uint16 prev_txin = di->txin;

	DMA_TRACE(("  %s: dma_getnexttxdd %s\n", di->name,
	           (range == HNDDMA_RANGE_ALL) ? "all" :
	           ((range == HNDDMA_RANGE_TRANSMITTED) ? "transmitted" : "transfered")));

	/* The check below can be removed when/if new chips implement a DMA that does support
	 * use of the AD for Descriptor only DMAs, and that implementation to support the AD
	 * checking needs to be added below where the range is being set.
	 */
	if (range == HNDDMA_RANGE_TRANSFERED) {
		DMA_ERROR(("%s: dma_getnexttxdd: HNDDMA_RANGE_TRANSFERED is not valid range \n",
			di->name));
		ASSERT(range != HNDDMA_RANGE_TRANSFERED);
		return BCME_RANGE;
	}

	if (di->ntxd == 0) {
		DMA_ERROR(("%s: dma_getnexttxdd ntxd=0 \n", di->name));
		return BCME_ERROR;
	}

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel(dmah, FALSE);
	}

	if (range == HNDDMA_RANGE_ALL)
		end = di->txout;
	else {
		if (di->txin == di->xs0cd) {
#ifdef BCM_HWALITE
			if (mlc_hwalite_is_enable(di->mlc_dev)) {
				uint16 cd = mlc_hwalite_txfifo_cd(di->mlc_dev,
					di->fifo, di->is_aqm, mlo_unit(di->mlc_dev));
				end = (uint16)((cd & D64_XS0_CD_MASK) -
					(B2I(di->xmtptrbase, di->dd64_size) & D64_XS0_CD_MASK));
			} else
#endif /* BCM_HWALITE */
			{
				end = DMA64_TX_CD(di);
			}
			di->xs0cd = end;
		} else
			end = di->xs0cd;
	}

	if (prev_txin == end)
		return BCME_NOTFOUND;

	di->txin = NEXTTXD(prev_txin);
	di->hnddma.txavail = di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1;

	DMA_TRACE(("%s: dma_getnexttxdd pre-txin=%d post-txin =%d, txavail = %d \n",
		di->name, prev_txin, di->txin, di->hnddma.txavail));
	return BCME_OK;
}

/* Function to get the next descriptor index to post */
uint16
dma_get_next_txd_idx(hnddma_t *di, bool txout)
{
	if (txout)
		return (DI_INFO(di))->txout;
	else
		return (DI_INFO(di))->txin;
}

/* Function to get the number of descriptors between start and txout/txin */
uint16
dma_get_txd_count(hnddma_t *dmah, uint16 start, bool txout)
{
	dma_info_t *di = DI_INFO(dmah);

	if (txout)
		return (NTXDACTIVE(start, di->txout));
	else
		return (NTXDACTIVE(start, di->txin));
}

/* Function to return the address of the descriptor with the given index */
uintptr
dma_get_txd_addr(hnddma_t *di, uint16 idx)
{
	dma_info_t *ddi = DI_INFO(di);

	if (ddi->hnddma.dmactrlflags & DMA_CTRL_DESC_BASE_IDX_UPD)
		return (uintptr)(B2I(PHYSADDRLO(ddi->txdpa), ddi->dd64_size) + idx);
	else
		return ((uintptr)(((dma64dd_t *)((uintptr)PHYSADDRLO(ddi->txdpa))) + idx));
}

/* Function to get the memory address of the buffer pointed to by the
 * descriptor #idx
 */
void
dma_get_txd_memaddr(hnddma_t *dmah, uint32 *addrlo, uint32 *addrhi, uint idx)
{
	dma_info_t *di = DI_INFO(dmah);
	/* get the memory address of the data buffer pointed by descriptor */
	*addrlo = BUS_SWAP32(R_SM(&di->txd64[idx].addrlow));
	*addrhi = BUS_SWAP32(R_SM(&di->txd64[idx].addrhigh));
}

#ifdef BCMLFRAG
/*
 * Sequentially program the pktdata(lfrag) - from TCM, followed by the
 * individual fragments from the HOST.
 */
static int BCMFASTPATH
dma64_txfast_lfrag(dma_info_t *di, void *p0, bool commit)
{
	void *p, *next;
	uchar *data;
	uint len;
	uint16 txout;
	uint32 flags = 0;
	dmaaddr_t pa;
	dma64addr_t pa64 = { .low_addr = 0, .high_addr = 0 };
	uint8 i = 0, j = 0;

	/* if using indirect DMA access, then configure IndQSel */
	if (DMA_INDIRECT(di)) {
		dma_set_indqsel((hnddma_t *)di, FALSE);
	}

	txout = di->txout;

	/*
	 * Lfrag - Program the descriptor for Lfrag data first before
	 * considering the individual fragments
	 */
	for (p = p0; p; p = next) {
		uint ftot = 0;
		uint nsegs = 1;

		next = PKTNEXT(di->osh, p);
		data = PKTDATA(di->osh, p);
		len  = PKTLEN(di->osh, p);

		if (PKTISFRAG(di->osh, p)) {
			ftot = PKTFRAGTOTNUM(di->osh, p);
		}

		if (len == 0) {
			/* Should not happen ideally unless this is a chained lfrag */
			goto program_frags;
		}
#ifndef BCM_SECURE_DMA
#if defined(OSL_CACHE_COHERENT)
		OSL_VIRT_TO_PHYSADDR(di->osh, data, pa);
#else
		pa = DMA_MAP(di->osh, data, len, DMA_TX, p, &di->txp_dmah[txout]);
#endif /* ! OSL_CACHE_COHERENT */
#endif /* BCM_SECURE_DMA */
		{
			if ((nsegs+ftot) > (uint)(di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1))
				goto outoftxd;
		}

		for (j = 1; j <= nsegs; j++) {
			flags = 0;
			if ((p == p0) && (j == 1))
				flags |= D64_CTRL1_SOF;
			if (txout == (di->ntxd - 1))
				flags |= D64_CTRL1_EOT;
			if (di->burstsize_ctrl)
				flags |= D64_CTRL1_NOTPCIE;

			if ((j == nsegs) && (ftot == 0) && (next == NULL))
				flags |= (D64_CTRL1_IOC | D64_CTRL1_EOF);

			dma64_dd_upd(di, di->txd64, pa, txout,
				&flags, len);
#ifdef BULK_PKTLIST_DEBUG
			ASSERT(di->txp[txout] == NULL);
#else
			if (!DMA_BULK_PATH(di)) {
				ASSERT(di->txp[txout] == NULL);
			}
#endif // endif
			txout = NEXTTXD(txout);
		}

program_frags:
		/*
		 * Now, walk the chain of fragments in this lfrag allocating
		 * and initializing transmit descriptor entries.
		 */
		for (i = LB_FRAG1, j = 1; j <= ftot; i++, j++) {
			flags = 0;
			if (PKTFRAGISCHAINED(di->osh, i)) {
				 i = LB_FRAG1;
				 p = PKTNEXT(di->osh, p);
				 ASSERT(p != NULL);
				 next = PKTNEXT(di->osh, p);
			}

			len = PKTFRAGLEN(di->osh, p, i);

#ifdef BCM_DMAPAD
			if (DMAPADREQUIRED(di)) {
				len += PKTDMAPAD(di->osh, p);
			}
#endif /* BCM_DMAPAD */

			pa64.loaddr = PKTFRAGDATA_LO(di->osh, p, i);
			pa64.hiaddr = PKTFRAGDATA_HI(di->osh, p, i);

			if ((j == ftot) && (next == NULL))
				flags |= (D64_CTRL1_IOC | D64_CTRL1_EOF);
			if (txout == (di->ntxd - 1))
				flags |= D64_CTRL1_EOT;

			/* War to handle 64 bit dma address for now */
			dma64_dd_upd_64_from_params(di, di->txd64, pa64, txout, &flags, len);

#ifdef BULK_PKTLIST_DEBUG
			ASSERT(di->txp[txout] == NULL);
#else
			if (!DMA_BULK_PATH(di)) {
				ASSERT(di->txp[txout] == NULL);
			}
#endif // endif
			txout = NEXTTXD(txout);
		}
	}

#ifdef BULK_PKTLIST_DEBUG
	di->txp[PREVTXD(txout)] = p0;
#else
	if (!DMA_BULK_PATH(di)) {
		/* save the packet */
		di->txp[PREVTXD(txout)] = p0;
	}
#endif // endif

	/* bump the tx descriptor index */
	di->txout = txout;

	/* kick the chip */
	if (commit) {
		dma64_txcommit_local(di, FALSE);
	}

	/* tx flow control */
	di->hnddma.txavail = di->ntxd - NTXDACTIVE(di->txin, di->txout) - 1;

	return (0);

outoftxd:
	DMA_ERROR(("%s: %s: out of txds !!!\n", di->name, __FUNCTION__));
	di->hnddma.txavail = 0;
	di->hnddma.txnobuf++;
	return (-1);
}
#endif /* BCMLFRAG */

int
dma_peeknexttxdd(hnddma_t *dmah, txd_range_t range)
{
	dma_info_t *di = DI_INFO(dmah);
	uint16 end = 0;

	if (range == HNDDMA_RANGE_TRANSFERED) {
		DMA_ERROR(("%s: dma_peeknexttxdd: not valid range \n",
			di->name));
		ASSERT(range != HNDDMA_RANGE_TRANSFERED);
		return BCME_RANGE;
	}

	if (di->ntxd == 0) {
		DMA_ERROR(("%s: dma_peeknexttxdd ntxd=0 \n", di->name));
		return BCME_ERROR;
	}

	if (range == HNDDMA_RANGE_ALL) {
		end = di->txout;
	} else {
		if (di->txin == di->xs0cd) {
			/* if using indirect DMA access, then configure IndQSel */
			if (DMA_INDIRECT(di)) {
				dma_set_indqsel(dmah, FALSE);
			}
#ifdef BCM_HWALITE
			if (mlc_hwalite_is_enable(di->mlc_dev)) {
				uint16 cd = mlc_hwalite_txfifo_cd(di->mlc_dev,
					di->fifo, di->is_aqm, mlo_unit(di->mlc_dev));
				end = (uint16)((cd & D64_XS0_CD_MASK) -
					(B2I(di->xmtptrbase, di->dd64_size) & D64_XS0_CD_MASK));
			} else
#endif /* BCM_HWALITE */
			{
				end = DMA64_TX_CD(di);
			}
			di->xs0cd = end;
		} else {
			end = di->xs0cd;
		}
	}

	if (di->txin == end)
		return BCME_NOTFOUND;

	return BCME_OK;
}

/** like getnexttxp but no reclaim */
void *
dma_peeknexttxp(hnddma_t *dmah, txd_range_t range)
{
	dma_info_t *di = DI_INFO(dmah);

	uint16 end;

	if (di->ntxd == 0)
		return (NULL);

	if (range == HNDDMA_RANGE_ALL) {
		end = di->txout;
	} else {
		if (di->txin == di->xs0cd) {
			/* if this DMA channel is using indirect access,
			 * then configure the IndQSel register
			 * for this DMA channel
			 */
			if (DMA_INDIRECT(di)) {
				dma_set_indqsel(dmah, FALSE);
			}
#ifdef BCM_HWALITE
			if (mlc_hwalite_is_enable(di->mlc_dev)) {
				uint16 cd = mlc_hwalite_txfifo_cd(di->mlc_dev,
					di->fifo, di->is_aqm, mlo_unit(di->mlc_dev));
				end = (uint16)((cd & D64_XS0_CD_MASK) -
					(B2I(di->xmtptrbase, di->dd64_size) & D64_XS0_CD_MASK));
			} else
#endif /* BCM_HWALITE */
			{
				end = DMA64_TX_CD(di);
			}
			di->xs0cd = end;
		} else {
			end = di->xs0cd;
		}
	}

#ifdef BULK_PKTLIST
	/* Routine does not appear to be used anywhere.
	 * XXX If is is used the caller has to reclaim this packet
	 * and terminate the PKTLINK before freeing the packet,
	 * the packet free code ASSERTS on a non NULL link.
	 * The packet cannot be terminated here as it had not been reclaimed
	 * and will break the pending DMA packet chain.
	 */
	if (DMA_BULK_PATH(di)) {
		return (di->dma_pkt_list.head_pkt);
	} else
#endif /* BULK_PKTLIST */
	{
		uint16 i;
		for (i = di->txin; i != end; i = NEXTTXD(i)) {
			if (di->txp[i]) {
				return (di->txp[i]);
			}
		}
	}

	return (NULL);
}
