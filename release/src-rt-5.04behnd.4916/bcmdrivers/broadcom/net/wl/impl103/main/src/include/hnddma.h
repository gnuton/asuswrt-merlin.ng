/*
 * Generic Broadcom Home Networking Division (HND) DMA engine SW interface
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
 * $Id: hnddma.h 836975 2024-02-26 13:06:29Z $
 */

#ifndef	_hnddma_h_
#define	_hnddma_h_

#include <typedefs.h>
#include <osl_decl.h>
#include <siutils.h>
#include <sbhnddma.h>

#ifndef _hnddma_pub_
#define _hnddma_pub_
/* for pktpool_t */
#include <bcmutils.h>
typedef const struct hnddma_pub hnddma_t;
#endif /* _hnddma_pub_ */

#ifdef BCMHME
#include <bcmpcie.h>
#endif

#if defined(BCM_ROUTER)
/* Enable/Disable Bulk Descriptor Flushing optimization */
#define BULK_DESCR_FLUSH
#endif

/* range param for dma_getnexttxp() and dma_txreclaim */
typedef enum txd_range {
	HNDDMA_RANGE_ALL		= 1,
	HNDDMA_RANGE_TRANSMITTED,
	HNDDMA_RANGE_TRANSFERED
} txd_range_t;

/* dma parameters id */
enum dma_param_id {
	HNDDMA_PID_TX_MULTI_OUTSTD_RD	= 0,
	HNDDMA_PID_TX_PREFETCH_CTL,
	HNDDMA_PID_TX_PREFETCH_THRESH,
	HNDDMA_PID_TX_BURSTLEN,
	HNDDMA_PID_TX_CHAN_SWITCH,

	HNDDMA_PID_RX_PREFETCH_CTL	= 0x100,
	HNDDMA_PID_RX_PREFETCH_THRESH,
	HNDDMA_PID_RX_BURSTLEN,
	HNDDMA_PID_BURSTLEN_CAP,
	HNDDMA_PID_BURSTLEN_WAR,
	HNDDMA_SEP_RX_HDR,	/**< SPLITRX related */
	HNDDMA_SPLIT_FIFO,
	HNDDMA_PID_D11RX_WAR,
	HNDDMA_PID_RX_WAIT_CMPL,
	HNDDMA_NRXPOST,
	HNDDMA_NRXBUFSZ,
	HNDDMA_BCMRX_PCN_FIFO,  /* Packet Classification for NIC(PCN) Mode Related */
	HNDDMA_PP_XPM		/* BPM/MPM based pktpool used */
};

#define SPLIT_FIFO_0	1
#define SPLIT_FIFO_1	2

typedef void (*setup_context_t)(void *ctx, void *p, uint8 **desc0, uint16 *len0,
	uint8 **desc1, uint16 *len1);

extern void dma_detach(hnddma_t *dmah);
extern void dma_txinit(hnddma_t *dmah);
extern bool dma_txreset(hnddma_t *dmah);
extern bool dma_txenabled(hnddma_t *dmah);
extern void dma_txsuspend(hnddma_t *dmah);
extern void dma_txresume(hnddma_t *dmah);
extern bool dma_txsuspended(hnddma_t *dmah);
extern bool dma_txsuspendedidle(hnddma_t *dmah);
extern void dma_txflush(hnddma_t *dmah);
extern void dma_txflush_clear(hnddma_t *dmah);
extern int  dma_txfast(hnddma_t *dmah, void *p0, bool commit);
extern void dma_txcommit(hnddma_t *dmah);
extern void dma_txcommit_ext(hnddma_t *dmah, bool aqm_fifo);
extern int  dma_txunframed(hnddma_t *dmah, void *p, uint len, bool commit);
extern void *dma_getpos(hnddma_t *di, bool direction);
extern bool dma_txstopped(hnddma_t *dmah);
extern int  dma_txreclaim(hnddma_t *dmah, txd_range_t range);
extern void *dma_getnexttxp(hnddma_t *dmah, txd_range_t range);
extern void *dma_peeknexttxp(hnddma_t *dmah, txd_range_t range);
extern int dma_peeknexttxdd(hnddma_t *dmah, txd_range_t range);
extern int  dma_peekntxp(hnddma_t *dmah, int *len, void *txps[], txd_range_t range);
extern void dma_txblock(hnddma_t *dmah);
extern void dma_txunblock(hnddma_t *dmah);
extern uint dma_txactive(hnddma_t *dmah);
extern void dma_txrotate(hnddma_t *dmah);
extern void dma_rxinit(hnddma_t *dmah);
extern bool dma_rxreset(hnddma_t *dmah);
extern bool dma_rxidle(hnddma_t *dmah);
extern bool dma_rxstopped(hnddma_t *dmah);
extern void dma_rxenable(hnddma_t *dmah);
extern bool dma_rxenabled(hnddma_t *dmah);
extern void dma_rx_waitcomplete_enable(hnddma_t *dmah, bool enable);

#ifdef BULKRX_PKTLIST
typedef struct rx_list {
	void *rx_head;
	void *rx_tail;
	/* Packets retrieved from RxFIFO 0/1/2.
	 * CAUTION: Will not match with list length.
	 */
	uint32 rxfifocnt;
} rx_list_t;

#if defined(STS_XFER_PHYRXS_PKT)
extern void dma_set_phyrxs_dma_flags_war(hnddma_t *dmah, bool phyrxs_dma_flags_war);
#endif /* STS_XFER_PHYRXS_PKT */

#if defined(STS_XFER_PHYRXS_SEQ)
extern void dma_set_sts_xfer_phyrxs(hnddma_t *dmah, bool sts_xfer_phyrxs);
#endif /* STS_XFER_PHYRXS_SEQ */

#if defined(STS_XFER_PHYRXS_MODE_FIFO)
typedef enum sts_mem_type {
	PHYRXS		= 1,
	PHYRXS_MBX
} sts_mem_type_t;

extern void dma_sts_set_memory(hnddma_t *dmah, sts_mem_type_t sts_mem_type,
	uint32 memaddr_hi, uint32 memaaddr_lo);
extern void dma_sts_rxinit(hnddma_t *dmah);
extern bool dma_sts_rxfill(hnddma_t *dmah);
extern uint16 dma_sts_rx(hnddma_t *dmah);
extern void dma_sts_rx_done(hnddma_t *dmah, uint16 rxin);
extern void dma_sts_rxreclaim(hnddma_t *dmah);
extern uint16 dma_sts_rxin(hnddma_t *dmah);
#endif /* STS_XFER_PHYRXS_MODE_FIFO */

extern void dma_rx(hnddma_t *dmah, rx_list_t *rx_list, uint nbound);
#ifdef BCM_HWALITE
extern void dma_mlo_rx(hnddma_t *dmah, rx_list_t *rx_list, uint nbound, uint32 mlo_unit);
#endif /* BCM_HWALITE */
#else /* ! BULKRX_PKTLIST */
extern void *dma_rx(hnddma_t *dmah);
#endif /* ! BULKRX_PKTLIST */

extern bool dma_rxfill(hnddma_t *dmah);
extern void dma_rxreclaim(hnddma_t *dmah);
extern void *dma_getnextrxp(hnddma_t *dmah, bool forceall);
extern void *dma_peeknextrxp(hnddma_t *dmah);
extern void dma_rxparam_get(hnddma_t *dmah, uint16 *rxoffset, uint16 *rxbufsize);
extern void dma_fifoloopbackenable(hnddma_t *dmah);
extern uintptr dma_getvar(hnddma_t *dmah, const char *name);
extern void dma_counterreset(hnddma_t *dmah);
extern uint dma_ctrlflags(hnddma_t *dmah, uint mask, uint flags);

#if defined(BCMDBG) || defined(BCMDBG_TXSTALL) || defined(WL_MACDBG)
extern void dma_dump(hnddma_t *dmah, struct bcmstrbuf *b, bool dumpring);
extern void dma_dumptx(hnddma_t *dmah, struct bcmstrbuf *b, bool dumpring);
extern void dma_dumprx(hnddma_t *dmah, struct bcmstrbuf *b, bool dumpring);
#endif

extern uint dma_rxactive(hnddma_t *dmah);
extern uint dma_txpending(hnddma_t *dmah);
extern uint dma_txcommitted(hnddma_t *dmah);
extern int  dma_pktpool_set(hnddma_t *dmah, pktpool_t *pool);
extern bool dma_rxtxerror(hnddma_t *dmah, bool istx);
extern uint dma_avoidance_cnt(hnddma_t *dmah);
extern uint16 dma_param_get(hnddma_t *dmah, uint16 paramid);
extern void dma_param_set(hnddma_t *dmah, uint16 paramid, uint16 paramval);
extern bool dma_glom_enable(hnddma_t *dmah, uint32 val);
extern uint dma_activerxbuf(hnddma_t *dmah);
extern bool dma_rxidlestatus(hnddma_t *dmah);
extern void dma_context(hnddma_t *dmah, setup_context_t fn, void *ctx);
#if defined(PKTQ_STATUS) && defined(BULK_PKTLIST)
extern void *dma_get_nextpkt(hnddma_t *dmah, void *pkt);
#endif /* PKTQ_STATUS && BULK_PKTLIST */

/**
 * Exported data structure (read-only)
 */
/* export structure */
struct hnddma_pub {
	uint		txavail;	/**< # free tx descriptors */
	uint		dmactrlflags;	/**< dma control flags */

	/* rx error counters */
	uint		rxgiants;	/**< rx giant frames */
	uint		rxnobuf;	/**< rx out of dma descriptors */
	/* tx error counters */
	uint		txnobuf;	/**< tx out of dma descriptors */
	uint		txnodesc;	/**< tx out of dma descriptors running count */
};

typedef struct dma_common dma_common_t;

/* Flags for dma_attach_ext function */
#define BCM_DMA_IND_INTF_FLAG		0x00000001	/* set for using INDIRECT DMA INTERFACE */
#define BCM_DMA_DESC_ONLY_FLAG		0x00000002	/* For DMA that posts descriptors only and
							 * no packets
							 */
#define BCM_DMA_CHAN_SWITCH_EN		0x00000008	/* for d11 corerev 64+ to help arbitrate
							 * btw dma channels.
							 */
#define BCM_DMA_ROEXT_SUPPORT		0x00000010	/* for d11 corerev 128+ to support receive
							 * frame offset >=128B and <= 255B
							 */
#define BCM_DMA_RX_ALIGN_8BYTE		0x00000020	/* RXDMA address 8-byte aligned for
							 * 43684A0
							 */
#define BCM_DMA_BULK_PROCESSING		0x00000080	/* Datapath bulk DMA processing */
#define BCM_DMA_HWA_MACRXFIFO		0x00000100	/* HWA managed rx fifo */
#define BCM_DMA_HWA_MACTXFIFO		0x00000200	/* HWA managed tx/aqm fifo */
#define BCM_DMA_BULKRX_PROCESSING	0x00000400	/* Datapath bulk DMA processing */
#define BCM_DMA_HWA_HME_RX		0x00000800	/* Request to use HME memory */
#define BCM_DMA_HWA_HME_TX		0x00001000	/* Request to use HME memory */
#define BCM_DMA_DESC_ONLY_8B_FLAG	0x00002000	/* 8 bytes descriptor only */
#define BCM_DMA_DESC_BASE_IDX_UPD	0x00004000	/* descriptor basis on Index programmed
							 * for the Descriptor ring
							 */
#define BCM_DMA_NUM_DESC_NON_POWER_2	0x00008000	/* support Non power of 2 num DMA desc */
/* MLO FIFO related flags */
#define BCM_DMA_MLO_SKIP_ALLOC		0x00010000	/* DMA alloc not required on this link */
#define BCM_DMA_MLO_RXFIFO		0x00020000	/* MLO RxFifo */

#define BCM_DMA_BOUNDRY64KB_WAR		0x00040000	/**< WAR for 6717 TXDMA descriptor ring
							 * buffer can not crossed 64K boundary
							 * when doing TXD manual read.
							 */
#define BCM_DMA_PUQ_QREMAP		0x00080000	/* Enable Queue Remap to support PUQ */
#define BCM_DMA_PUQ_WAR			0x00100000	/**< WAR for incorrect read value of
							 * AQM/TXDMA register.
							 */
#define BCM_DMA_COHERENT_PER_TRANS	0x00200000	/**< DEBUG for DMA descriptor protocol error
							 * in Router NIC driver.
							 */
#define BCM_DMA_MLO_BI_BUS		0x00400000	/* MLO Bi-Bus high bit required */

#define BCM_DMA_REQ_REG_COUNT		3		/* Number of SuspReq[x], FlushReq[x] and
							 * associated Status registers
							 */

#define BCM_DMA_RX_ALIGN_8BYTE_COUNT	8		/* Extra pad for 8 byte alignment */

#define BCM_DMA_AQM_MAX_NUMD		15		/* Max NUMD of AQM DMA desc field [23:20] */

extern dma_common_t * dma_common_attach(osl_t *osh, volatile uint32 *indqsel,
	volatile uint32 **suspreq, volatile uint32 **flushreq, uint reg_count,
	volatile uint32 *puq_aqm_ring_depth, volatile uint32 *suspflush_grpsel);
extern void dma_common_detach(dma_common_t *dmacommon);

#ifdef BCM_DMA_INDIRECT
#define DMA_INDQSEL_IA			0x80000000	/* Allow indirect regs for non-ctmode */
extern void dma_set_indqsel(hnddma_t *dmah, bool force);
#else
#define dma_set_indqsel(a, b)
#endif /* #ifdef BCM_DMA_INDIRECT */

extern hnddma_t * dma_attach_ext(dma_common_t *dmac, osl_t *osh, const char *name, si_t *sih,
	volatile void *dmaregstx, volatile void *dmaregsrx, uint32 flags, uint8 qnum,
	uint ntxd, uint nrxd, uint rxbufsize, int rxextheadroom, uint nrxpost, uint rxoffset,
	uint *msg_level);

extern hnddma_t * dma_attach(osl_t *osh, const char *name, si_t *sih,
	volatile void *dmaregstx, volatile void *dmaregsrx,
	uint ntxd, uint nrxd, uint rxbufsize, int rxextheadroom, uint nrxpost,
	uint rxoffset, uint *msg_level);

/* return addresswidth allowed
 * This needs to be done after SB attach but before dma attach.
 * SB attach provides ability to probe backplane and dma core capabilities
 * This info is needed by DMA_ALLOC_CONSISTENT in dma attach
 */
extern uint dma_addrwidth(si_t *sih, void *dmaregs);

/* count the number of tx packets that are queued to the dma ring */
extern uint dma_txp(hnddma_t *di);

extern void dma_txrewind(hnddma_t *di);

/* pio helpers */
extern int dma_msgbuf_txfast(hnddma_t *di, dma64addr_t p0, bool com, uint32 ln, bool fst, bool lst);
extern int dma_ptrbuf_txfast(hnddma_t *dmah, dma64addr_t p0, void *p, bool commit,
	uint32 len, bool first, bool last);

extern int dma_rxfast(hnddma_t *di, dma64addr_t p, uint32 len);
extern int dma_rxfill_suspend(hnddma_t *dmah, bool suspended);
extern void dma_link_handle(hnddma_t *dmah1, hnddma_t *dmah2);
extern int dma_rxfill_unframed(hnddma_t *di, void *buf, uint len, bool commit);
extern uint16 dma_get_next_txd_idx(hnddma_t *di, bool txout);
extern uint16 dma_get_txd_count(hnddma_t *dmah, uint16 start, bool txout);
extern uintptr dma_get_txd_addr(hnddma_t *di, uint16 idx);

/* returns the memory address (hi and low) of the buffer associated with the dma descriptor
 * having index idx.
 */
extern void dma_get_txd_memaddr(hnddma_t *dmah, uint32 *addrlo, uint32 *addrhi, uint idx);

extern int dma_txdesc(hnddma_t *dmah, dma64dd_t *dd, bool commit);
extern int dma_getnexttxdd(hnddma_t *dmah, txd_range_t range, uint32 *flags);
extern int dmatx_map_pkts(hnddma_t *di, map_pkts_cb_fn cb, void *ctx);

#ifdef BULK_DESCR_FLUSH
extern bool dma_bulk_descr_tx_is_active(hnddma_t *dmah);
extern void dma_bulk_descr_tx_start(hnddma_t *dmah);
extern void dma_bulk_descr_tx_cancel(hnddma_t *dmah);
extern void dma_bulk_descr_tx_commit(hnddma_t *dmah, bool aqm_fifo);
#define DMA_BULK_DESCR_TX_IS_ACTIVE(x)  dma_bulk_descr_tx_is_active(x)
#define DMA_BULK_DESCR_TX_START(x)      dma_bulk_descr_tx_start(x)
#define DMA_BULK_DESCR_TX_CANCEL(x)     dma_bulk_descr_tx_cancel(x)
#define DMA_BULK_DESCR_TX_COMMIT(x, y)  dma_bulk_descr_tx_commit(x, y)
#else /* !BULK_DESCR_FLUSH */
#define DMA_BULK_DESCR_TX_IS_ACTIVE(x)  ({ BCM_REFERENCE(x); FALSE;})
#define DMA_BULK_DESCR_TX_START(x)      ({ BCM_REFERENCE(x); })
#define DMA_BULK_DESCR_TX_CANCEL(x)     ({ BCM_REFERENCE(x); })
#define DMA_BULK_DESCR_TX_COMMIT(x, y)  dma_txcommit_ext(x, y)
#endif /* BULK_DESCR_FLUSH */

extern void dma_update_rxfill(hnddma_t *dmah);
extern void dma_rxchan_reset(hnddma_t *di);
extern void dma_txchan_reset(hnddma_t *di);
extern pktpool_t* dma_pktpool_get(hnddma_t *dmah);
extern void dma_clearrxp(hnddma_t *dmah);
extern void dma_cleartxp(hnddma_t *dmah);
extern void dma_get_fifo_info(hnddma_t *dmah, dmaaddr_t *pa, uint *depth, bool tx);
#ifdef PP_XPM
extern void dma_pp_xpm_aux_init(hnddma_t *dmah, uint32 mlo_unit, uint32 nrxd);
#endif /* PP_XPM */

#ifdef BULK_PKTLIST
extern int dma_bulk_txcomplete(hnddma_t *dmah, uint16 ncons, uint16 *nproc,
		void **list_head, void **list_tail, txd_range_t range);
#endif /* BULK_PKTLIST */
extern void* dma_rx_head_pkt_entry_ptr(hnddma_t *dmah);

/* Following M2M Request applies to PCIECORE Mem2Mem channels (not M2MCORE) */
#define PCIE_M2M_REQ_VEC_MAX	4

#define XFER_FROM_LBUF	0x1
#define XFER_TO_LBUF	0x2
#define XFER_INJ_ERR	0x4

/* PCIECORE Mem2Mem request is specified as a table of RX and TX <address,len>
 * vectors, in the PCIE bus layer and passed to the hnddma layer, where the
 * actual DMA descriptors in the Rx and Tx DMA descriptor rings are programmed.
 */
typedef struct pcie_m2m_vec_s {
	dma64addr_t	addr64;
	uint32		len;
} pcie_m2m_vec_t;

typedef struct pcie_m2m_req_s {
	uint8		num_rx_vec;
	uint8		num_tx_vec;
	uint8		flags;
	bool		commit;
	uint32		vec_max;
	pcie_m2m_vec_t	vec[];
} pcie_m2m_req_t;

#define PCIE_M2M_REQ_INIT(req) \
{ \
	(req)->num_rx_vec = 0; \
	(req)->num_tx_vec = 0; \
	(req)->flags = 0; \
	(req)->commit = TRUE; \
}

#define PCIE_M2M_REQ_RX_SETUP(req, rxaddr64, rxlen) \
{ \
	pcie_m2m_vec_t *vec = &((req)->vec[(req)->num_rx_vec]); \
	ASSERT((req)->num_tx_vec == 0); \
	vec->addr64 = rxaddr64; \
	vec->len = rxlen; \
	(req)->num_rx_vec++; \
}

#define PCIE_M2M_REQ_TX_SETUP(req, txaddr64, txlen) \
{ \
	pcie_m2m_vec_t *vec = &((req)->vec[(req)->num_rx_vec + (req)->num_tx_vec]); \
	vec->addr64 = txaddr64; \
	vec->len = txlen; \
	(req)->num_tx_vec++; \
}

#define PCIE_M2M_REQ_FLAGS_SETUP(req, flag) \
{ \
	(req)->flags |= flag; \
}

extern int pcie_m2m_req_submit(hnddma_t *dmah, pcie_m2m_req_t *m2m_req, bool implicit);
extern void dma_chan_enable(hnddma_t *dmah, bool enable);
#ifdef WL_EAP_REKEY_WAR
extern void dma_set_dbgprnt(hnddma_t *dmah, bool disable);
#endif /* WL_EAP_REKEY_WAR */

#ifdef BCMHME
extern int dma_common_hme_upd(dma_common_t *dmac, pcie_ipc_hme_sgmt_t *hme_sgmt,
	uint16 sgmt_avail);
extern int dma64_hme_alloc(hnddma_t *dmah, uint *flags);
extern void dma_common_hme_dump(dma_common_t *dmac, struct bcmstrbuf *b);
#endif
#if defined(WL_MLO)
extern void dma64_mlo_hme_tx_update(hnddma_t *dmah, dma64addr_t addr, uint32 fifo_depth);
extern void dma_set_mlo_rxoffset(hnddma_t *dmah, uint rxoffset);
#ifdef BCM_HWALITE
extern void dma_get_di_rxinfo(hnddma_t *dmah, uint32 *nrxpost, uint64 *fifo_addr_va,
	uint32 *rcvptrbase);
extern void dma_set_mlo_param(hnddma_t *dmah, void *mlc_dev, uint32 fifo, bool is_aqm);
#endif /* BCM_HWALITE */
#endif /* WL_MLO */

#ifdef BCM_PCAP
void dma_pcap_set(hnddma_t *dmah, bool en, uint16 nrxpost, uint16 nrxd, bool up);
void dma_pcap_attach(dma_common_t *dmacommon, void *pcap_hdl);
void dma_pcap_rx(hnddma_t *dmah);
#endif /* BCM_PCAP */
extern void pcn_pktpool_dump(hnddma_t *dmah, int fifo, struct bcmstrbuf *b);
/**
 * ------------------------------------------------------------------------------------------------
 * Section: rx_list_t Accessor Macros
 * ------------------------------------------------------------------------------------------------
 */

#if defined(BULKRX_PKTLIST)
#define RX_LIST_RESET(rx_list)		((rx_list)->rx_head = (rx_list)->rx_tail = NULL)

#define RX_LIST_INIT(rx_list)								\
	({										\
		RX_LIST_RESET(rx_list);							\
		(rx_list)->rxfifocnt = 0;						\
	})

#define RX_LIST_RX_HEAD(rx_list)	((rx_list)->rx_head)
#define RX_LIST_RX_TAIL(rx_list)	((rx_list)->rx_tail)

#define RX_LIST_APPEND(at_rx_list, new_rx_list)						\
	({										\
		ASSERT((new_rx_list)->rx_head != NULL);					\
		if ((at_rx_list)->rx_tail)						\
			 PKTSETLINK((at_rx_list)->rx_tail, (new_rx_list)->rx_head);	\
		 else									\
			 (at_rx_list)->rx_head = (new_rx_list)->rx_head;		\
		 (at_rx_list)->rx_tail = (new_rx_list)->rx_tail;			\
	})

#define RX_LIST_PREPEND(at_rx_list, new_rx_list)					\
	({										\
		ASSERT((new_rx_list)->rx_head != NULL);					\
		if ((at_rx_list)->rx_head)						\
			 PKTSETLINK((new_rx_list)->rx_tail, (at_rx_list)->rx_head);	\
		 else									\
			 (at_rx_list)->rx_tail = (new_rx_list)->rx_tail;		\
		 (at_rx_list)->rx_head = (new_rx_list)->rx_head;			\
	})

#define RX_LIST_INSERT_HEAD(rx_list, _pkt)						\
	({										\
		PKTSETLINK((_pkt), (rx_list)->rx_head);					\
		(rx_list)->rx_head = (_pkt);						\
		if ((rx_list)->rx_tail == NULL)						\
			 (rx_list)->rx_tail = (_pkt);					\
	 })

#define RX_LIST_DELETE_HEAD(rx_list)							\
	({										\
		void * _pkt = (rx_list)->rx_head;					\
		ASSERT((rx_list)->rx_head != NULL);					\
		(rx_list)->rx_head = PKTLINK((rx_list)->rx_head);			\
		if ((rx_list)->rx_head == NULL)						\
			(rx_list)->rx_tail = NULL;					\
		PKTSETLINK(_pkt, NULL);							\
		(void *) _pkt;								\
	 })

#define RX_LIST_INSERT_TAIL(rx_list, _pkt)						\
	({										\
		if ((rx_list)->rx_tail == NULL)						\
		{									\
			ASSERT((rx_list)->rx_head == NULL);				\
			(rx_list)->rx_head = (_pkt);					\
		} else {								\
			PKTSETLINK((rx_list)->rx_tail, (_pkt));				\
		}									\
		(rx_list)->rx_tail = (_pkt);						\
	 })

#endif /* BULKRX_PKTLIST */
#endif	/* _hnddma_h_ */
