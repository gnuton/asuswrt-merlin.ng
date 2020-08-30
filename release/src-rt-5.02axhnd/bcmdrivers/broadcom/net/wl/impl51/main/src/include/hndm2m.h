/**
 * -----------------------------------------------------------------------------
 * Generic Broadcom Home Networking Division (HND) M2M module.
 *
 * M2MCORE (m2mdma_core.h) may instantiate descriptor based mem2mem DMA engines
 * (DMA Channels #0 and #1), in addition to the Byte Move Engines (hndbme.[hc]).
 *
 * hndbme.[hc] provide a synchronous (poll for DONE) HW assisted memcpy API.
 * hndm2m.[hc] provides an asynchronous HW assisted mem2mem copy API, with an
 * explicit DMA Done Interrupt driven callback to resume an application's
 * processing.
 *
 * Disclaimer:
 * Descriptor based M2M channels #0 and #1 may only be used in Dongle Mode
 *
 * Copyright 2020 Broadcom
 *
 * This program is the proprietary software of Broadcom and/or
 * its licensors, and may only be used, duplicated, modified or distributed
 * pursuant to the terms and conditions of a separate, written license
 * agreement executed between you and Broadcom (an "Authorized License").
 * Except as set forth in an Authorized License, Broadcom grants no license
 * (express or implied), right to use, or waiver of any kind with respect to
 * the Software, and Broadcom expressly reserves all rights in and to the
 * Software and all intellectual property rights therein.  IF YOU HAVE NO
 * AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 * WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 * THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1. This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof, and to
 * use this information only in connection with your use of Broadcom
 * integrated circuit products.
 *
 * 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 * "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 * OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 * SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 * IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 * IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 * ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 * OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 * NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id$
 *
 * vim: set ts=4 noet sw=4 tw=80:
 * -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * -----------------------------------------------------------------------------
 */

#ifndef _HNDM2M_H_
#define _HNDM2M_H_

#if defined(DONGLEBUILD)

/**
 * +--------------------------------------------------------------------------+
 * Section: M2M Core Interface
 * Includes support for default Descriptor based DMA. See M2M_DD_ENAB.
 * See also hndbme.h,c for non-descriptor based M2M.
 * +--------------------------------------------------------------------------+
 */

/** Debug dump the M2M information */
extern void m2m_dump(bool verbose); /* M2M state, includeing users */
extern void m2m_regs(bool verbose); /* M2M Core registers */
extern void m2m_stats(void);        /* M2M statistics */
extern void hnd_cons_m2m_dump(void *arg, int argc, char *argv[]);

/** Initialize/Finalize M2M service */
extern int m2m_fini(si_t *sih, osl_t *osh);
extern int m2m_init(si_t *sih, osl_t *osh);

struct dngl_bus; /* pciedev bus layer */
struct pcie_ipc; /* Link Host reported information */
extern int m2m_link_pcie_ipc(struct dngl_bus *pciedev, struct pcie_ipc *pcie_ipc);

/**
 * +--------------------------------------------------------------------------+
 *
 * Section M2M_DD: Descriptor based DMA Processor
 *
 * Two descriptor based mem2mem channels are available.
 *
 * Applications (users) register their callbacks in a registry table.
 *
 *  + Channel#0 is reserved for applications seeking to DMA data from
 *    Dongle SysMem or D11 MAC/PHY memory to Router Host DDR.
 *    Channel State Information (CSI) and AirIQ are two known applications.
 *
 *  + Channel#1 is reserved for applications seeking to DMA data from
 *    Router Host DDR to Dongle SysMem
 *
 * Implementation Caveat:
 *    No support is planned for NIC mode.
 *
 * +--------------------------------------------------------------------------+
 */

#if !defined(M2M_DD_ENG)
#define M2M_DD_ENG          (2U)    /* Two Descriptor DMA engines */
#endif // endif

#define M2M_DD_CH0          (0U)    /* DMA from Dngl SysMem|SVMP to Host DDR */
#define M2M_DD_CH1          (1U)    /* Not productized */

#define M2M_DD_D2H          (M2M_DD_CH0)
#define M2M_DD_H2D          (M2M_DD_CH1)

#if !defined(M2M_DD_ENAB)
#define M2M_DD_ENAB         ((uint32)((1 << M2M_DD_CH0) | (1 << M2M_DD_CH1)))
#endif // endif

#define M2M_DD_SYS          (0U)    /* Reserved for System OS */
#define M2M_DD_CSI          (1U)    /* CH#0 : SVMP to DDR over PCIe */
#define M2M_DD_AIQ          (2U)    /* CH#0 : SVMP to DDR over PCIe */
#define M2M_DD_PGO          (3U)    /* CH#0 : SMEM to DDR over PCIe */
#define M2M_DD_PGI          (4U)    /* CH#1 : DDR over PCIe to SMEM */
#define M2M_DD_USR          (8U)    /* Maximum number of DD users */

/* Return values */
#define M2M_SUCCESS         (0)
#define M2M_FAILURE         (~0)
#define M2M_INVALID         (~0)

typedef uint32 m2m_dd_key_t;        /* M2M User Registration key */
typedef uint32 m2m_dd_cpy_t;        /* M2M Copy Transaction key */

/** M2M DD DMA DONE Callback for DD's with M2M_DD_XFER_RESUME */
/* Usr callback must not modify the xfer_src or xfer_dst - READ ACCESS ONLY */
typedef void (* m2m_dd_done_cb_t)(void *usr_cbdata,
                dma64addr_t *xfer_src, dma64addr_t *xfer_dst, int xfer_len,
                uint32 xfer_arg);

/** M2M DD Resource Available WAKE Callback for stalled users */
typedef void (* m2m_dd_wake_cb_t)(void *usr_cbdata);

/**
 * ----------------------------------------------------------------------------
 * Section: Descriptor based DMA API:
 *
 * Implementation Caveat :
 * 1. APIs are not reentrant!
 * 2. usr_done_cb will be invoked with dma64addr_t pointers to xfer_src and
 *    xfer_dst that were used in the m2m DMA transfer. If the source or the
 *    destination were over PCIe (i.e. DDR), then M2M_PCI64ADDR_HIGH may be
 *    set. User provided callback routine may only READ ACCESS the address.
 * 3. If no m2m_dd_done_cb was resgietered in m2m_dd_usr_register(), then the
 *    user must not use a xfer_op M2M_DD_XFER_RESUME in m2m_dd_xfer()
 *
 * ----------------------------------------------------------------------------
 */

/* M2M transaction attributes: max 8 operation flags, see m2m_dd_xi::xop */
#define M2M_DD_XFER_NOOP    (0)
#define M2M_DD_XFER_COMMIT  (1) /* advance DMA Processor */
#define M2M_DD_XFER_RESUME  (2) /* invoke user's callback on DMA done */

/** Register a user, returns registration key for use in copy, or M2M_INVALID */
extern m2m_dd_key_t m2m_dd_usr_register(uint32 usr_idx, uint32 eng_idx,
                void *usr_cbdata, m2m_dd_done_cb_t usr_done_cb,
                m2m_dd_wake_cb_t usr_wake_cb, uint16 usr_wake_thr);

/** Determine available space */
extern int m2m_dd_avail(m2m_dd_key_t m2m_dd_key);

/** Request a WAKE callback on DD availability in a M2M engine */
extern int m2m_dd_wake_request(m2m_dd_key_t m2m_dd_key);

/** Initiate a m2m data transfer, returns a copy key, or M2M_INVALID. */
extern m2m_dd_cpy_t m2m_dd_xfer(m2m_dd_key_t m2m_dd_key,
          dma64addr_t *xfer_src, dma64addr_t *xfer_dst, int xfer_len,
          uint32 xfer_arg, uint32 xfer_op); /* NOOP, COMMIT, RESUME */

#endif /* DONGLEBUILD */

#endif  /* _HNDM2M_H_ */
