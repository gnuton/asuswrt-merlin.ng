/*
 * BCM43XX M2M DMA core hardware definitions.
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
 *
 * $Id:m2mdma _core.h 421139 2013-08-30 17:56:15Z kiranm $
 */

#ifndef	_M2MDMA_CORE_H
#define	_M2MDMA_CORE_H
#include <sbhnddma.h>
/* cpp contortions to concatenate w/arg prescan */
#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif // endif

/* See bcmpcie.h PCIe IPC PCIE_IPC_FLAGS and pcie_ipc_t layout */
#define M2M_IPC_VERSION       0x0001
#define M2M_IPC_VERSION_MASK  0x00FF /* PCIE_IPC_FLAGS_REVISION */
#define M2M_IPC_ASSERT_BUILT  0x0100 /* PCIE_IPC_FLAGS_ASSERT_BUILT */
#define M2M_IPC_ASSERT        0x0200 /* PCIE_IPC_FLAGS_ASSERT */
#define M2M_IPC_TRAP          0x0400 /* PCIE_IPC_FLAGS_TRAP */
#define M2M_IPC_IN_BRPT       0x0800 /* PCIE_IPC_FLAGS_IN_BRPT */
#define M2M_IPC_SET_BRPT      0x1000 /* PCIE_IPC_FLAGS_SET_BRPT */
#define M2M_IPC_PENDING_BRPT  0x2000 /* PCIE_IPC_FLAGS_PENDING_BRPT */

/* dma regs to control the flow between host2dev and dev2host  */
typedef struct m2m_devdmaregs {
	dma64regs_t	tx;
	uint32 		PAD[2];
	dma64regs_t	rx;
	uint32 		PAD[2];
} m2m_devdmaregs_t;

typedef struct dmaintregs {
	uint32 intstatus;
	uint32 intmask;
} dmaintregs_t;

/* rx header */
typedef volatile struct {
	uint16 len;
	uint16 flags;
} m2md_rxh_t;

/* NOTE: Layout MUST match the first 8 fields of pcie_ipc_t in bcmpcie.h */
typedef struct m2m_pcie_ipc {
	uint32  flags;                  /* pcie_ipc_t::flags */
	uint32  trap_daddr32;           /* pcie_ipc_t::trap_daddr32 */
	uint32  assert_exp_daddr32;     /* pcie_ipc_t::assert_exp_daddr32 */
	uint32  assert_file_daddr32;    /* pcie_ipc_t::assert_file_daddr32 */
	uint32  assert_line;            /* pcie_ipc_t::assert_line */
	uint32  console_daddr32;        /* pcie_ipc_t::console_daddr32 */
	uint32  msgtrace_daddr32;       /* pcie_ipc_t::msgtrace_daddr32 obsoleted */
	uint32  fwid;                   /* pcie_ipc_t::fwid */
} m2m_pcie_ipc_t;

/*
 * Software counters (first part matches hardware counters)
 */

typedef volatile struct {
	uint32 rxdescuflo;	/* receive descriptor underflows */
	uint32 rxfifooflo;	/* receive fifo overflows */
	uint32 txfifouflo;	/* transmit fifo underflows */
	uint32 runt;		/* runt (too short) frames recv'd from bus */
	uint32 badlen;		/* frame's rxh len does not match its hw tag len */
	uint32 badcksum;	/* frame's hw tag chksum doesn't agree with len value */
	uint32 seqbreak;	/* break in sequence # space from one rx frame to the next */
	uint32 rxfcrc;		/* frame rx header indicates crc error */
	uint32 rxfwoos;		/* frame rx header indicates write out of sync */
	uint32 rxfwft;		/* frame rx header indicates write frame termination */
	uint32 rxfabort;	/* frame rx header indicates frame aborted */
	uint32 woosint;		/* write out of sync interrupt */
	uint32 roosint;		/* read out of sync interrupt */
	uint32 rftermint;	/* read frame terminate interrupt */
	uint32 wftermint;	/* write frame terminate interrupt */
} m2md_cnt_t;

/* SB side: M2M DMA core registers */
typedef struct m2mregs {
	uint32 control;		/* Core control 0x0 */
	uint32 capabilities;	/* Core capabilities 0x4 */
	uint32 intcontrol;	/* Interrupt control 0x8 */
	uint32 PAD[5];
	uint32 intstatus;	/* Interrupt Status  0x20 */
	uint32 PAD[3];
	dmaintregs_t intregs[8]; /* 0x30 - 0x6c */
	uint32 PAD[36];
	uint32 intrxlazy[8];	/* 0x100 - 0x11c */
	uint32 PAD[48];
	uint32 clockctlstatus;  /* 0x1e0 */
	uint32 workaround;	/* 0x1e4 */
	uint32 powercontrol;	/* 0x1e8 */
	uint32 PAD[5];
	m2m_devdmaregs_t dmaregs[8]; /* 0x200 - 0x3f4 */
} m2md_regs_t;

/** SB side: BME registers for phyrxsts / txstatus acceleration */
typedef struct m2md_bme_status_regs {
	uint32 cfg;		/* 0x0 */
	uint32 ctl;	/* 0x4 */
	uint32 sts;	/* 0x8 */
	uint32 debug;	/*  0xC */
	uint32 da_base_l;	/* 0x10 */
	uint32 da_base_h;	/* 0x14 */
	uint32 size;	/* 0x18 */
	uint32 wr_idx;	/* 0x1C */
	uint32 rd_idx;	/* 0x20 */
	uint32 dma_template;	/* 0x24 */
	uint32 sa_base_l;	/* 0x28 */
	uint32 sa_base_h;	/* 0x2C */
} m2md_bme_status_regs_t;

#define BITMASK(nbits, shift) (((1 << (nbits)) - 1) << (shift))

/** bits in m2md_bme_status_regs_t::cfg register */
#define BME_STS_CFG_MOD_ENBL_NBITS 1
#define BME_STS_CFG_MOD_ENBL_SHIFT 0
#define BME_STS_CFG_MOD_ENBL_MASK BITMASK(BME_STS_CFG_MOD_ENBL_NBITS, BME_STS_CFG_MOD_ENBL_SHIFT)
#define BME_STS_CFG_MOD_CH_NBITS 3
#define BME_STS_CFG_MOD_CH_SHIFT 3
#define BME_STS_CFG_MOD_CH_MASK BITMASK(BME_STS_CFG_MOD_CH_NBITS, BME_STS_CFG_MOD_CH_SHIFT)
#define BME_STS_CFG_EXTRARD_NBITS 1
#define BME_STS_CFG_EXTRARD_SHIFT 4
#define BME_STS_CFG_EXTRARD_MASK BITMASK(BME_STS_CFG_EXTRARD_NBITS, BME_STS_CFG_EXTRARD_SHIFT)
#define BME_STS_CFG_STOPCOND_NBITS 2
#define BME_STS_CFG_STOPCOND_SHIFT 2
#define BME_STS_CFG_STOPCOND_MASK  BITMASK(BME_STS_CFG_STOPCOND_NBITS, BME_STS_CFG_STOPCOND_SHIFT)

/** bits in m2md_bme_status_regs_t::dma_template register */
#define BME_DMATMPL_NOT_PCIE_SHIFT    0 /**< NotPcie filled in DMA descr used for destination */
#define BME_DMATMPL_NOT_PCIE_NBITS    1
#define BME_DMATMPL_COHERENT_SHIFT    1 /**< Coherent filled in DMA descr used for destination */
#define BME_DMATMPL_COHERENT_NBITS    1
#define BME_DMATMPL_ADDREXTDP_SHIFT   2 /**< AddrExt filled in DMA descr used for destination */
#define BME_DMATMPL_ADDREXTDP_NBITS   2
#define BME_DMATMPL_NOTPCIESP_SHIFT   4 /**< NotPCIe filled in DMA descr used for source */
#define BME_DMATMPL_NOTPCIESP_NBITS   1
#define BME_DMATMPL_COHERENTSP_SHIFT  5 /**< Coherent filled in DMA descr used for source */
#define BME_DMATMPL_COHERENTSP_NBITS  1
#define BME_DMATMPL_PERDESCWC_SHIFT   7 /**< PerDescWC filled in DMA descr used for source */
#define BME_DMATMPL_PERDESCWC_NBITS   1

#define BME_DMATMPL_NOT_PCIE_BITMASK(BME_DMATMPL_NOT_PCIE_NBITS, BME_DMATMPL_NOT_PCIE_SHIFT)
#define BME_DMATMPL_COHERENT_BITMASK(BME_DMATMPL_COHERENT_NBITS, BME_DMATMPL_COHERENT_SHIFT)
#define BME_DMATMPL_ADDREXTDP_BITMASK(BME_DMATMPL_ADDREXTDP_NBITS, BME_DMATMPL_ADDREXTDP_SHIFT)
#define BME_DMATMPL_NOTPCIESP_BITMASK(BME_DMATMPL_NOTPCIESP_NBITS, BME_DMATMPL_NOTPCIESP_SHIFT)
#define BME_DMATMPL_COHERENTSP_BITMASK(BME_DMATMPL_COHERENTSP_NBITS, BME_DMATMPL_COHERENTSP_SHIFT)
#define BME_DMATMPL_PERDESCWC_BITMASK(BME_DMATMPL_PERDESCWC_NBITS, BME_DMATMPL_PERDESCWC_SHIFT)

#endif	/* _M2MDMA_CORE_H */
