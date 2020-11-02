/*
<:copyright-BRCM:2019:proprietary:standard

   Copyright (c) 2019 Broadcom 
   All Rights Reserved

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
*/

/*
 *******************************************************************************
 * File Name  : bcm_eapfwd.c
 * Purpose : Model a EAP Tier-1 customer stack that manages proprietary tunnel
 * termination and origination. In this module, the forwarder sits
 * between a LAN ingress network device and the WLAN network device. No explicit
 * Tunnel handling is performed.
 *
 * Binding Broadcom's LAN network device to WLAN network device, facilitating:
 * - "binning packets" based on destination WLAN station
 * - bins of packets translated to packet linked lists
 * - handoff of entire packet linked list to WLAN datapath
 *
 * Requirements:
 * - EAP Forwarder should not impact the thread model or the thread to CPU core
 *   assignment.
 * - EAP Forwarder should allow for performance breakdown per subsystem.
 *
 * Package into eapfwd.ko
 * eapfwd.ko must be insmod after bcm_enet.ko and before wl.ko
 *
 *
 *******************************************************************************
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h> /* copy_from_user */
#include <linux/gbpm.h>
#include <bcm_eapfwd.h>

#if defined(CONFIG_BCM_FPP)
#include "bcm_fpp.h"
#else
#define fpp_entry(fpp_id)                   do { /* no-op */ } while (0)
#define fpp_exit(fpp_id, param)             do { /* no-op */ } while (0)
#endif /* CONFIG_BCM_FPP */

#include "bcm_eapfwd.h"

#define EAPFWD_VERSION              "0.1"
#define EAPFWD_VER_STR              "v" EAPFWD_VERSION
#define EAPFWD_MODDES               "Broadcom EAP Forwarder"

/**
 * =============================================================================
 * Section: Typedefs and Globals
 * =============================================================================
 */

/** Trace enabled at level 2 and above */
#define CC_EAP_DEBUG 0
#if (CC_EAP_DEBUG >= 2)
#define EAP_TRACE(fmt, arg...) \
    printk(CLRb "%s: " fmt CLRnl, __FUNCTION__, ##arg)
#else
#define EAP_TRACE(fmt, arg...)      do { /* no-op */ } while (0)
#endif  /* CC_PKTLIST_DEBUG < 2 */


/* In BRCM reference, we do not have a Tunnel, and ONLY 802.3 header */
#define EAPFWD_HLEN    14 /* uapi/linux/if_ether.h ETH_HLEN */

#if defined(BCM_PKTFWD)
#define EAPFWD_FLCTL /* BPM PktPool Availability based EAPFWD Ingress throttle */
#define EAPFWD_FLCTL_PKT_PRIO_FAVOR         4  /* Favor Pkt Prio >= 4 : VI,VO */
#define EAPFWD_FLCTL_SKB_EXHAUSTION_LO_PCNT 25 /* Favored Pkt threshold */
#define EAPFWD_FLCTL_SKB_EXHAUSTION_HI_PCNT 10
#define EAPFWD_FLCTL_DROP_CREDITS           32
#define EAPFWD_FLCTL_ENABLE                 1
#endif

/** typedefs */

struct eap_dev {                /** eap_dev */
    struct d3lut           * d3lut;
    struct pktlist_context * pktlist_context[EAP_WLAN_RADIOS_MAX];
#if defined(EAPFWD_FLCTL)  /* BPM Skb availability based Rx flowcontrol */
    unsigned int        skb_exhaustion_lo[EAP_WLAN_RADIOS_MAX]; /* avail < lo: admit ONLY favored */
    unsigned int        skb_exhaustion_hi[EAP_WLAN_RADIOS_MAX]; /* avail < hi: admit none */
    unsigned short      pkt_prio_favor[EAP_WLAN_RADIOS_MAX];    /* favor pkt prio >= 4 */
    unsigned int        flctl_enable[EAP_WLAN_RADIOS_MAX];      /* Is HFC feature enabled/disabled */
    unsigned int        count_flctl_pkts[EAP_WLAN_RADIOS_MAX];
#endif
} ____cacheline_aligned;
typedef struct eap_dev eap_dev_t;

/** Globals */

eap_dev_t eap_dev_g = {
    .d3lut           = D3LUT_NULL,
    .pktlist_context = { PKTLIST_CONTEXT_NULL, PKTLIST_CONTEXT_NULL },
};

typedef                                                     // CONFIG_BCM_FPP
enum eap_fpp_func {
    EAP_ENET_RX_LOOP = 0,
    EAP_NETIF_RECEIVE_SKB,
    EAP_XMIT_ENQUEUE,
    EAP_XMIT_SCHEDULE,
    EAP_FPP_FUNC_MAX
} eap_fpp_func_t;

uint32_t eap_fpp_id[EAP_FPP_FUNC_MAX];                      // CONFIG_BCM_FPP

#define ETHER_ISMULTI(ea) (((const uint8_t *)(ea))[0] & 1)

/**
 * =============================================================================
 * Section: Ethernet
 *
 * Theory of Operation: (reference bcmdrivers/opensource/net/enet/impl#/enet.c)
 *
 * Current Operation:
 * - Ethernet ingress do-while loop in function rx_pkt_from_q(), dequeues
 *   packets from the receive descriptor ring, that are ready to be processed.
 *   The loop may be bounded by a configurable budget number of packets.
 * - Each ready to process packet is passed to rx_skb().
 * - rx_skb() is responsible for initializing the struct sk_buff, with
 *   packet attributes such as, data, len, dev, priority and protocol.
 *   sk_buff may be BPM managed, for optimized alloc/free and initialization.
 * - Each packet is delivered up the stack though netif_receive_skb().
 *
 * EAPFWD: A forwarder may intercept each packet via one
 * of three mechanisms:
 * 1. Override netif_receive_skb()
 * 2. Derive a proprietary skbuff::protocol using
 * eth_type_trans(), with PTYPE
 * 3. Promiscuous PTYPE_ALL snooping with skbuff consumed
 *
 * -----------------------------------------------------------------------------
 *
 * Step #1. Receive packet into EAPFWD
 * EAPFWD will use mechanism #1, wherein the netif_receive_skb()
 * in the Ethernet driver (enet.c) is replaced by
 * eap_receive_skb()
 * #if defined(BCM_EAPFWD)
 *     if (eap_receive_skb_hook)
 *         eap_receive_skb_hook(skb);
 *     else
 * #endif
 *     netif_receive_skb(skb);
 *
 * Step #2. Commit budget number of receive packets
 * In addition to overriding netif_receive_skb() in the ethernet driver, at the
 * end of each do-while loop (budget or less number of packets), a call to
 * eap_xmit_schedule() will be invoked in Ethernet driver (enet.c)
 *
 * Step #3. Profiling
 * Add fpp_entry(0) and fpp_exit(0, count) into rx_pkt_from_q().
 *
 * =============================================================================
 */

#if defined(CONFIG_BCM_FPP)
/**
 * In rx_pkt_from_q(),
 *  - add "fpp_entry(0);" before do {} loop
 *  - add "fpp_exit(0, count);" before return count
 */
void enet_rx_loop(void) {} /* used by FPP tool for symbol printing */
#endif /*  CONFIG_BCM_FPP */

#ifdef EAPFWD_FLCTL
/* FlowControl over-subscription */
static inline bool ucast_should_drop_skb(pktlist_context_t *pktlist_context,
        uint32_t avail_skb, uint16_t pkt_prio, uint16_t pkt_dest, uint32_t radio_idx)
{
    if (eap_dev_g.flctl_enable[radio_idx] == 0) return false;

	if (avail_skb <= eap_dev_g.skb_exhaustion_hi[radio_idx]) return true; // drop ALL
    if ((avail_skb <= eap_dev_g.skb_exhaustion_lo[radio_idx]) &&
        (pkt_prio < eap_dev_g.pkt_prio_favor[radio_idx]))    return true; // drop low prio

#if defined(BCM_PKTFWD_FLCTL)
    if ((pktlist_context->fctable != PKTLIST_FCTABLE_NULL) &&
        (pkt_prio < eap_dev_g.pkt_prio_favor[radio_idx]))
    {
        int32_t credits;
        credits = __pktlist_fctable_get_credits(pktlist_context, 
                                                pkt_prio, pkt_dest);

        if (credits <= 0) return true;      // drop BE/BK if no credits avail
    }
#endif /* BCM_PKTFWD_FLCTL */

    return false;
}
#endif


/**
 * -----------------------------------------------------------------------------
 * Description :
 * Upon a D3LUT lookup failure, packets will be sent to the Linux stack.
 * Packets delivered to the network stack will pass through the Linux bridge and
 * populate the D3LUT (br_input.c and br_device.c)
 * -----------------------------------------------------------------------------
 */
void /* eap_netif_receive_skb */
eap_netif_receive_skb(struct sk_buff * skb)
{
    fpp_entry(eap_fpp_id[EAP_NETIF_RECEIVE_SKB]);           // CONFIG_BCM_FPP

    EAP_TRACE("skb<%p>", skb);
    netif_receive_skb(skb);

    /* In BRCM ref design, Linux bridge forwarding database (fdb) will allocate
     * a fdb entry in br0 and populate the D3LUT.
     *
     * D3LUT could have been populated on a STA assoc event or by the Tier-1
     * EAPFWD in itself.
     */

    /* Eventually this packet will be sent to WLAN through wl_start */

    fpp_exit(eap_fpp_id[EAP_NETIF_RECEIVE_SKB], 1);         // CONFIG_BCM_FPP

}   /* eap_netif_receive_skb */


/**
 * -----------------------------------------------------------------------------
 * Description :
 * This function would typically implement a Tier-1 EAP stack handoff packets
 * to WLAN network device. Typically handoff would be done via a packet queue.
 * A WLAN thread running on another CPU core would be scheduled to xmit all
 * packets in the packet queue.
 * -----------------------------------------------------------------------------
 */

void /* Invoked by EAP Tunnel stack in place of wl_start or enqueue */
eap_xmit_enqueue(struct sk_buff * skb)
{
    uint8_t * d3addr;

    uint32_t  radio;    /* domain */
    uint32_t  dest;     /* endpoint */
    uint32_t  prio;     /* pkt priority */

    pktlist_context_t * pktlist_context;
    d3lut_elem_t      * d3lut_elem;
    d3lut_t           * d3lut;
    d3fwd_wlif_t      * d3fwd_wlif;
    struct ethhdr     * eh;

#ifdef EAPFWD_FLCTL
    unsigned long drop_credits = 0;
    uint32_t avail_skb = gbpm_avail_skb(); /* BPM free skb availability */
#endif

    fpp_entry(eap_fpp_id[EAP_XMIT_ENQUEUE]);                // CONFIG_BCM_FPP

    d3addr = skb->data - EAPFWD_HLEN;
    d3lut  = eap_dev_g.d3lut;
    eh = (struct ethhdr *)d3addr;

    /**
     * In this model, we leverage the Linux bridge to populate the
     * D3LUT (802.3 Address to WLAN Station Identification Table)
     * Could be PKTCTBL or D3LUT. See br_input.c and br_device.c
     *
     * D3LUT table could be populated by EAPFWD (as in PKTCTBL).
     */

    if (d3lut == D3LUT_NULL) /* wl.ko not yet loaded */
        goto sendup_to_linux;

    /* d3lut::lock is NOT taken !!! */
    d3lut_elem = d3lut_lkup(d3lut, d3addr, D3LUT_LKUP_GLOBAL_POOL);

    /* Not found or destination is not a WLAN endpoint */
    if ((d3lut_elem == D3LUT_ELEM_NULL) || (!d3lut_elem->ext.wlan))
        goto sendup_to_linux;

#if 0
    /* First test netif_receive_skb is working fine with the new prototype
     * infra also making sure the d3lut lookup also is working
     */
    goto sendup_to_linux;
#endif

    /* Multicast and ARP handled by linux stack */
    if ((ETHER_ISMULTI(d3addr)) || (eh->h_proto == ntohs(ETH_P_ARP))) {
        goto sendup_to_linux;
    }

    /* undo eth_type_trans() */
    __skb_push(skb, EAPFWD_HLEN);

    /* EAPFWD tunnel termination would have set this */
    d3fwd_wlif = d3lut_elem->ext.d3fwd_wlif; /* Use D3LUT's extension D3FWD */
    if (d3fwd_wlif == D3FWD_WLIF_NULL)
        goto sendup_to_linux;
    skb->dev = d3fwd_wlif->net_device;

    skb->wl.ucast.nic.wl_chainidx = d3lut_elem->key.v16;
    radio   = d3lut_elem->key.domain;
    dest    = PKTLIST_DEST(d3lut_elem->key.v16);
    /* FIXME: Temporarily assign prio as 0. It needs to be derived from the
       802.1P field or IP DSCP/ToS */
    prio    = 0; //skb->priority;

    EAP_TRACE("skb<%p> radio:sta:prio %d:%d:%d", skb, radio, dest, prio);

    pktlist_context = eap_dev_g.pktlist_context[radio];
    if (pktlist_context == (PKTLIST_CONTEXT_NULL))
        goto sendup_to_linux;

#if defined(EAPFWD_FLCTL)
    if (ucast_should_drop_skb(pktlist_context, avail_skb, prio, dest, radio))
    {
        drop_credits++;
        eap_dev_g.count_flctl_pkts[radio]++; 
        dev_kfree_skb(skb);
        return;
     }
#endif /* EAPFWD_FLCTL */

    /**
     * BINNING packets into multiple queues<dest,prio>
     *
     * Place the packet into appropriate <dest,prio> queue and move queue to
     * active work list of queues.
     */
    __pktlist_add_pkt(pktlist_context, prio, dest, d3lut_elem->key.v16, skb, SKBUFF_PTR);

#if defined(BCM_PKTFWD_FLCTL)
    if (pktlist_context->fctable != PKTLIST_FCTABLE_NULL)
    {
        /* Decrement avail credits for pktlist */
        __pktlist_fctable_dec_credits(pktlist_context, prio, dest);
    }
#endif /* BCM_PKTFWD_FLCTL */

    fpp_exit(eap_fpp_id[EAP_XMIT_ENQUEUE], 1);              // CONFIG_BCM_FPP

    return;

sendup_to_linux:
    eap_netif_receive_skb(skb);

    fpp_exit(eap_fpp_id[EAP_XMIT_ENQUEUE], 1);              // CONFIG_BCM_FPP

}   /* eap_xmit_enqueue */


/**
 * -----------------------------------------------------------------------------
 * Description :
 * This function is used to tag a COMMIT action after N number of packets have
 * been received by Enet and EAPFWD.
 *
 * The COMMIT signal may be directly embedded into a sk_buff structure in the
 * enet loop, in itself. If COMMIT signal is directly embedded into the sk_buff
 * structure then there is NO NEED for explicit eap_xmit_schedule() invocation
 * Recommendation is to use an explicit eap_xmit_schedule() invocation.
 * -----------------------------------------------------------------------------
 */
void /* Commit all skbs that have been delivered in the last enet budget loop */
eap_xmit_schedule(void)
{
    int prio, radio;
    pktlist_context_t * eap_pktlist_context;
    pktlist_context_t * wl_pktlist_context;

    fpp_entry(eap_fpp_id[EAP_XMIT_SCHEDULE]);               // CONFIG_BCM_FPP

    for (radio = 0; radio < EAP_WLAN_RADIOS_MAX; radio++)
    {
        eap_pktlist_context = eap_dev_g.pktlist_context[radio];
        if (eap_pktlist_context == PKTLIST_CONTEXT_NULL)
            continue;

        wl_pktlist_context = eap_pktlist_context->peer;

        PKTLIST_LOCK(wl_pktlist_context);

#if 0
        /* D3LUT does not have IP-Multicast protocol's 802.3 MacDA 01:00:5e.. */
        __pktlist_xfer_work(eap_pktlist_context, wl_pktlist_context,
                        &eap_pktlist_context->mcast,
                        &wl_pktlist_context->mcast, "MCAST");
#endif

        /* Dispatch active ucast pktlists from eap to wl - by priority */
        for (prio = 0; prio < PKTLIST_PRIO_MAX; ++prio)
        {
            /* Process non empty ucast[] worklists in eap pktlist context */
            __pktlist_xfer_work(eap_pktlist_context, wl_pktlist_context,
                            &eap_pktlist_context->ucast[prio],
                            &wl_pktlist_context->ucast[prio], "UCAST",
                            SKBUFF_PTR);
        }

        /* Release peer's pktlist context */
        PKTLIST_UNLK(wl_pktlist_context);


        /* Wake peer wl thread: invoke handoff handler to wake peer driver.
         * handoff handler is the HOOKP wl_pktfwd_xfer_hook in wl_eap_bind
         *
         * xfer_fn = wl_schedule_hook registered via wl_eap_bind.
         */
        (eap_pktlist_context->xfer_fn)(eap_pktlist_context->peer);

    } /* per-radio loop */

    fpp_exit(eap_fpp_id[EAP_XMIT_SCHEDULE], 1);             // CONFIG_BCM_FPP

}   /* eap_xmit_schedule */


/**
 * -----------------------------------------------------------------------------
 * Description : Override the netif_receive_skb in enet driver
 * -----------------------------------------------------------------------------
 */

static inline void EAP_TUNNEL_STACK(void) { /* no-op in model */ }

void /* Override the netif_receive_skb in enet driver */
eap_receive_skb(struct sk_buff * skb)
{
    /* ---------------------------------------- */
    /* Proprietary Tier-1 tunnel stack handling */
    /* ---------------------------------------- */

    EAP_TUNNEL_STACK();


    /**
     * At this point, outer tunnel headers removed, and we have a 802.3 pkt
     * EAP Tunnel stack, would have also performed some lookup to determine the
     * WLAN network_device and place into skb->dev.
     *
     * This is where the "wl_start" would have been invoked, using the
     * netdev ops' hardstart_xmit function for skb->dev
     *
     * Some Tier-1 may simply place the packet into a queue and later dispatch
     * the queue of packets in a different thread, by scheduling a WLAN thread.
     */

    eap_xmit_enqueue(skb); /* packets are binned in multiple prio work queues */

}   /* eap_receive_skb */


/**
 * =============================================================================
 * Section: WLAN
 *
 * -----------------------------------------------------------------------------
 * wl_pktfwd.c
 * -----------
 * wl_pktfwd_xfer_callback() is implemented in wl_pktfwd.c (Used by Retail)
 * Description :
 * EAPFWD in eap_xmit_schedule(), will invoke this WLAN
 * callback, after passing all bins from the LAN ingress
 *      pktlist_context to the peer wl_pktlist_context
 * (eap_pktlist_context->xfer_fn)(eap_pktlist_context->peer);
 * The parameter is wl_pktlist_context pointer.
 *
 * wl_pktlist_context bins are dispatched to each wlif and wlif(s) are woken up.
 *      _wl_schedule_work(wl_info) ::
 *          txq_txchain_dispatched = TRUE
 *          wake_up_interruptible(&wl->kthread_wqh)
 *
 * Notes on changes to wl_pktfwd_xfer_callback()
 * - no need for wl_pktfwd_xfer_work_mcast in EAP.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * wl_thread.c
 * -----------
 * wl_worker_thread_func wait_event_interruptible loop:
 *   event on txq_txchain_dispatched?
 *   while (wlif != NULL) { wl_pktfwd_dnstream(wl, wlif->d3fwd_wlif) }
 *    ***OLD PKCTBL: wl_start_txchain_txqwork() instead of wl_pktfwd_dnstream()
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * wl_pktfwd.c
 * -----------
 * wl_pktfwd_dnstream: walks per wlif's ucast worklist and for each non-empty
 *  ucast work list (ffs bmap of 8), takes each pktlist and converts to PKTC
 *  formatted linked list using wl_pktfwd_pktlist_pktc.
 * wl_start_int() is then invoked to transmit the PKTC based chained list WLAN
 * -----------------------------------------------------------------------------
 *
 * =============================================================================
 */

/**
 * =============================================================================
 * Section: Debug
 * =============================================================================
 */

void /* Debug dump the eap_dev object */
eap_dev_dump(void)
{
    int idx;
    pktlist_context_t * pktlist_context;

    const bool dump_peer = false;
    const bool dump_verbose = true;

    for (idx = 0; idx < EAP_WLAN_RADIOS_MAX; idx++)
    {
        pktlist_context = eap_dev_g.pktlist_context[idx];
        if (pktlist_context != PKTLIST_CONTEXT_NULL) {
            pktlist_context_dump(pktlist_context, dump_peer, dump_verbose);
        }
    }

}   /* eap_dev_dump */

/* FIXME: use procFS to invoke eap_dev_dump() */

/**
 * =============================================================================
 * Section: Profiling
 * =============================================================================
 */
#if defined(CONFIG_BCM_FPP)
static void
eap_fpp_register(void)
{
    eap_fpp_id[EAP_ENET_RX_LOOP]  = fpp_register((void*)enet_rx_loop);
    eap_fpp_id[EAP_NETIF_RECEIVE_SKB] = fpp_register((void*)eap_netif_receive_skb);
    eap_fpp_id[EAP_XMIT_ENQUEUE]  = fpp_register((void*)eap_xmit_enqueue);
    eap_fpp_id[EAP_XMIT_SCHEDULE] = fpp_register((void*)eap_xmit_schedule);
}
#endif /* CONFIG_BCM_FPP */

/**
 * =============================================================================
 * Section: Module
 * =============================================================================
 */

/**
 * -----------------------------------------------------------------------------
 * Description : Bind WLAN to EAP. Invoked in wl_linux.c::wl_attach().
 *
 * In wl_linux.c, in wl_attach function, (search for wl_wfd_bind())
 *
 * In wl_pktfwd.h,c: extern d3lut_t * wl_pktfwd_lut(void); // accessor function
 *
 * #if defined(BCM_EAPFWD) && defined(BCM_PKTFWD) {
 *      int eap_idx = wl_eap_bind(
 *           wl, wl->dev, wl->unit,
 *           wl_pktfwd_lut(),
 *           wl_pktfwd_request(wl_pktfwd_req_pktlist_e, wl->unit, 0, 0),
 *           wl_pktfwd_xfer_callback);
 *
 *      wl_pktfwd_wfd_ins(wl, eap_idx);
 * }
 * #endif
 * -----------------------------------------------------------------------------
 */
int /* wl_eap_bind */
wl_eap_bind(void * wl, struct net_device * wl_dev, int radio_idx,
            d3lut_t * lut, pktlist_context_t * wl_pktlist_context,
            HOOKP wl_pktfwd_xfer_hook)
{
    PKTFWD_ASSERT(wl_pktlist_context != PKTLIST_CONTEXT_NULL);

    EAP_TRACE("%p::%s : radio<%d> pktlist_context self<%p> peer<%p>",
        wl_dev, wl_dev->name, radio_idx,
        eap_dev_g.pktlist_context[radio_idx], wl_pktlist_context);

    PKTFWD_ASSERT(lut != D3LUT_NULL);
    PKTFWD_ASSERT(wl_pktfwd_xfer_hook != NULL);
    PKTFWD_ASSERT(eap_dev_g.pktlist_context[radio_idx] == PKTLIST_CONTEXT_NULL);

    eap_dev_g.d3lut = lut;
    eap_dev_g.pktlist_context[radio_idx] =
        pktlist_context_init(wl_pktlist_context,
            (pktlist_context_xfer_fn_t)wl_pktfwd_xfer_hook,
             PKTLIST_CONTEXT_KEYMAP_NULL,
             &eap_dev_g, "eap_dev_g", radio_idx);

    PKTFWD_ASSERT(eap_dev_g.pktlist_context[radio_idx] != PKTLIST_CONTEXT_NULL);

    printk("\033[1m\033[32m eap_dev_g: initialized pktlists:"
        " radio %u nodes %u xfer %pS \033[0m\n",
        radio_idx, PKTLIST_NODES_MAX, wl_pktfwd_xfer_hook);

    eap_receive_skb_hook   =  eap_receive_skb;
    eap_xmit_schedule_hook =  eap_xmit_schedule;

#if defined(EAPFWD_FLCTL)
    /* Apply default BPM exhaustion level thresholds */
    if (wl_pktlist_context != PKTLIST_CONTEXT_NULL)
    {
        uint32_t bpm_total_skb = gbpm_total_skb();
        eap_dev_g.skb_exhaustion_lo[radio_idx] =
            ((bpm_total_skb * EAPFWD_FLCTL_SKB_EXHAUSTION_LO_PCNT) / 100);
        eap_dev_g.skb_exhaustion_hi[radio_idx] =
            ((bpm_total_skb * EAPFWD_FLCTL_SKB_EXHAUSTION_HI_PCNT) / 100);
        eap_dev_g.pkt_prio_favor[radio_idx] = EAPFWD_FLCTL_PKT_PRIO_FAVOR;
        eap_dev_g.flctl_enable[radio_idx] = EAPFWD_FLCTL_ENABLE;
        eap_dev_g.count_flctl_pkts[radio_idx] = 0;
        printk("%u FlowControl total<%u> lo<%u> hi<%u> favor prio<%u> enabled<%u>\n",
               radio_idx, bpm_total_skb,
               eap_dev_g.skb_exhaustion_lo[radio_idx],
               eap_dev_g.skb_exhaustion_hi[radio_idx],
               eap_dev_g.pkt_prio_favor[radio_idx],
               eap_dev_g.flctl_enable[radio_idx]);
    }
#endif /* EAPFWD_FLCTL */

#if defined(BCM_PKTFWD_FLCTL)
        if (wl_pktlist_context->fctable != PKTLIST_FCTABLE_NULL)
        {
            /* Point EAPFWD pktlist_fctable to WLAN pktlist_fctable */
            eap_dev_g.pktlist_context[radio_idx]->fctable = 
                wl_pktlist_context->fctable;

            /* Set pktlist_context pkt_prio_favor */
            wl_pktlist_context->fctable->pkt_prio_favor =
                eap_dev_g.pkt_prio_favor[radio_idx];
        }
        else
        {
            eap_dev_g.pktlist_context[radio_idx]->fctable =
               PKTLIST_FCTABLE_NULL;
        }
#endif /* BCM_PKTFWD_FLCTL */

    return radio_idx;

}   /* wl_eap_bind */
EXPORT_SYMBOL(wl_eap_bind);


int wl_eap_unbind(int radio_idx)
{
    pktlist_context_t * pktlist_context;

	pktlist_context = eap_dev_g.pktlist_context[radio_idx];
	if (pktlist_context != PKTLIST_CONTEXT_NULL) {
		eap_dev_g.pktlist_context[radio_idx] = pktlist_context_fini(pktlist_context);
	}
	return 0;
}   /* wl_eap_unbind */
EXPORT_SYMBOL(wl_eap_unbind);

/**************** EAPFWD PROC ENTRIES **********************/ 
#define EAPFWD_PROC_DEBUG_MAX_SIZE 10

static struct proc_dir_entry *eapfwd_proc_dir;
static struct proc_dir_entry *eapfwd_proc_debug, *eapfwd_proc_stats;
#ifdef EAPFWD_FLCTL
static struct proc_dir_entry *eapfwd_proc_flctl;
#endif

/* Global debug variable for eapfwd */
static unsigned int eapfwd_debug = 0;

static ssize_t eapfwd_stats_rd_func(struct file *filep, char __user *buf, size_t count, loff_t *offset)
{
    int len = 0;
#if defined(BCM_PKTFWD_FLCTL)
    int radio = 0;
#endif

    if (*offset != 0)
        return 0;

    len += sprintf(buf + len, "EAPFWD Stats \n");

#if defined(BCM_PKTFWD_FLCTL)
    for (radio = 0; radio < EAP_WLAN_RADIOS_MAX; radio++) {
        len += sprintf(buf + len, "Radio %d flctl_pkts = %u\n", radio, eap_dev_g.count_flctl_pkts[radio]);
    }
#endif

    *offset = len;

    return len;

} /* eapfwd_debug_rd_func */

static ssize_t eapfwd_debug_rd_func(struct file *filep, char __user *buf, size_t count, loff_t *offset)
{
    int len = 0;

    if (*offset != 0)
        return 0;

    len += sprintf(buf, "debug %d\n", eapfwd_debug);

    *offset = len;

    return len;

} /* eapfwd_debug_rd_func */

static ssize_t eapfwd_debug_wr_func(struct file *filep, const char __user *buf, size_t count, loff_t *offset)
{
	char localbuf[EAPFWD_PROC_DEBUG_MAX_SIZE];
    int count_to_write = count;

    if (count_to_write > EAPFWD_PROC_DEBUG_MAX_SIZE) {
         count_to_write = EAPFWD_PROC_DEBUG_MAX_SIZE;
	}

	if (copy_from_user(localbuf, buf, count_to_write)) {
		return (-EFAULT);
	}
	 
	sscanf(localbuf, "%d", &eapfwd_debug);

    return count_to_write;

} /* eapfwd_debug_rd_func */

#if defined(EAPFWD_FLCTL)
static ssize_t eapfwd_flctl_rd_proc(struct file *file, char *buff, size_t len, loff_t *offset)
{
    uint32_t radio_idx;

    if (*offset)
        return 0;
    *offset += sprintf(buff + *offset,
        "BPM system total<%u> avail<%u>\n", gbpm_total_skb(), gbpm_avail_skb());
    *offset += sprintf(buff + *offset,
                       "Radio ExhaustionLO ExhaustionHI PktPrioFavor Enabled\n");

    for (radio_idx = 0; radio_idx < EAP_WLAN_RADIOS_MAX; radio_idx++) {
        *offset += sprintf(buff + *offset, "%2u. %14u %12u %12u %7u\n", radio_idx,
                eap_dev_g.skb_exhaustion_lo[radio_idx],
                eap_dev_g.skb_exhaustion_hi[radio_idx],
                eap_dev_g.pkt_prio_favor[radio_idx],
                eap_dev_g.flctl_enable[radio_idx]);
    }

    return *offset;
}

#define EAPFWD_FLCTL_PROC_CMD_MAX_LEN    64
static ssize_t eapfwd_flctl_wr_proc(struct file *file, const char *buff, size_t len, loff_t *offset)
{
    int ret;
    char input[EAPFWD_FLCTL_PROC_CMD_MAX_LEN];
    uint32_t radio_idx, skb_exhaustion_lo, skb_exhaustion_hi, pkt_prio_favor, flctl_enable;
    if (copy_from_user(input, buff, len) != 0)
        return -EFAULT;
    ret = sscanf(input, "%u %u %u %u %u",
        &radio_idx, &skb_exhaustion_lo, &skb_exhaustion_hi, &pkt_prio_favor, &flctl_enable);
    if (ret < 5)
        goto Usage;
    if (radio_idx >= EAP_WLAN_RADIOS_MAX) {
        printk("Invalid radio_id %u, must be less than %u\n",
            radio_idx, EAP_WLAN_RADIOS_MAX);
        goto Usage;
    }
    if (skb_exhaustion_lo < skb_exhaustion_hi) {
        printk("Invalid exhaustion level lo<%u> hi<%u>\n",
            skb_exhaustion_lo, skb_exhaustion_hi);
        goto Usage;
    }
    if (pkt_prio_favor > 7) { /* prio 0 .. 7 */
        printk("Invalid pkt priority <%u>\n", pkt_prio_favor);
        goto Usage;
    }
    if (flctl_enable != 0 && flctl_enable != 1) { /* enable 0, 1 */
        printk("Enable must be 0 or 1 <%u>\n", flctl_enable);
        goto Usage;
    }
    eap_dev_g.skb_exhaustion_lo[radio_idx] = skb_exhaustion_lo;
    eap_dev_g.skb_exhaustion_hi[radio_idx] = skb_exhaustion_hi;
    eap_dev_g.pkt_prio_favor[radio_idx]    = pkt_prio_favor;
    eap_dev_g.flctl_enable[radio_idx]      = flctl_enable;
#if defined(BCM_PKTFWD_FLCTL)
    if (eap_dev_g.pktlist_context[radio_idx]->fctable != PKTLIST_FCTABLE_NULL)
    {
        eap_dev_g.pktlist_context[radio_idx]->fctable->pkt_prio_favor = pkt_prio_favor;
    }
#endif /* BCM_PKTFWD_FLCTL */
    printk("Radio<%u> exhaustion level lo<%u> hi<%u>, favor pkt prio >= %u enabled<%u>\n",
        radio_idx,  skb_exhaustion_lo, skb_exhaustion_hi, pkt_prio_favor, flctl_enable);
    goto Exit;
Usage:
    printk("\nUsage: <radio_id> <skb_exhaustion_lo> <skb_exhaustion_hi> <pkt_prio_favor> <enable(0/1)>\n");
Exit:
    return len;
}
#endif /* EAPFWD_FLCTL */

static const struct file_operations eapfwd_stats_fops = {
	.owner = THIS_MODULE,
	.read  = eapfwd_stats_rd_func,
};

static const struct file_operations eapfwd_debug_fops = {
	.owner = THIS_MODULE,
	.read  = eapfwd_debug_rd_func,
	.write  = eapfwd_debug_wr_func,
};


#ifdef EAPFWD_FLCTL
static const struct file_operations eapfwd_flctl_fops = {
	.owner = THIS_MODULE,
	.read  = eapfwd_flctl_rd_proc,
	.write  = eapfwd_flctl_wr_proc,
};
#endif

static int eapfwd_proc_init(void)
{
	eapfwd_proc_dir = proc_mkdir("eapfwd", NULL);
	if (!eapfwd_proc_dir) {
		printk("%s %s: Failed to create proc dir eapfwd\n", __FILE__, __FUNCTION__);
		return (-EIO);
	}

	if ((eapfwd_proc_stats = proc_create("eapfwd/stats", 0666, NULL, &eapfwd_stats_fops))
		== NULL) {
		printk("%s %s: Failed to create proc entry stats for eapfwd\n", __FILE__, __FUNCTION__);
		return (-EIO);
	}

	if ((eapfwd_proc_debug = proc_create("eapfwd/debug", 0666, NULL, &eapfwd_debug_fops))
		== NULL) {
		printk("%s %s: Failed to create proc entry debug for eapfwd\n", __FILE__, __FUNCTION__);
		return (-EIO);
	}

#ifdef EAPFWD_FLCTL
	if ((eapfwd_proc_flctl = proc_create("eapfwd/flctl", 0666, NULL, &eapfwd_flctl_fops))
		== NULL) {
		printk("%s %s: Failed to create proc entry flctl for eapfwd\n", __FILE__, __FUNCTION__);
		return (-EIO);
	}
#endif

	return (0);

}

static void eapfwd_proc_deinit(void)
{
	remove_proc_entry("eapfwd/stats", NULL);
	remove_proc_entry("eapfwd/debug", NULL);
	remove_proc_entry("eapfwd", NULL);

	return;
}

/***************** EAPFWD Module Init  ******************/
static int __init eapfwd_module_init( void )
{
    int idx;

    if (eapfwd_proc_init() != 0) {
        printk("%s: Failed eapfwd_proc_init \n", __FUNCTION__);
    }

    eap_dev_g.d3lut = D3LUT_NULL;
    for (idx = 0; idx < EAP_WLAN_RADIOS_MAX; idx++)
    {
        eap_dev_g.pktlist_context[idx] = PKTLIST_CONTEXT_NULL;
    }

#if defined(CONFIG_BCM_FPP)
    eap_fpp_register();
#endif /* CONFIG_BCM_FPP */

    printk("\033[1m\033[34m%s %s is loaded\033[0m\n", EAPFWD_MODDES, EAPFWD_VER_STR);

    return 0;
}

static void __exit eapfwd_module_exit( void )
{
    int idx;
    pktlist_context_t * pktlist_context;

    printk("\033[1m\033[34m%s %s is unloaded\033[0m\n",EAPFWD_MODDES, EAPFWD_VER_STR);

    eapfwd_proc_deinit();

    for (idx = 0; idx < EAP_WLAN_RADIOS_MAX; idx++)
    {
        pktlist_context = eap_dev_g.pktlist_context[idx];
        if (pktlist_context != PKTLIST_CONTEXT_NULL) {
            eap_dev_g.pktlist_context[idx] = PKTLIST_CONTEXT_NULL;
            pktlist_context_fini(pktlist_context);
        }
    }
    return;
}

module_init(eapfwd_module_init);
module_exit(eapfwd_module_exit);

MODULE_DESCRIPTION(EAPFWD_MODDES);
MODULE_VERSION(EAPFWD_VER_STR);
MODULE_LICENSE("Proprietary");
