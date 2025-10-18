/*
 * Linux OS Independent Layer
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
 * $Id: linux_osl.h 837579 2024-03-11 07:03:12Z $
 */

#ifndef _linux_osl_h_
#define _linux_osl_h_

#include <typedefs.h>
#define DECLSPEC_ALIGN(x)	__attribute__ ((aligned(x)))

typedef struct task_struct osl_ext_task_t;

/**
 * Internal locking.
 *
 * Usage:
 *   int_lock_state_t saved;
 *   int_lock(osh, saved);
 *   ...critical section...
 *   int_unlock(osh, saved);
 */

typedef unsigned long			int_lock_state_t;
#define int_lock(osh, state)		spin_lock_irqsave(&globals(osh)->int_lock, (state))
#define int_unlock(osh, state)		spin_unlock_irqrestore(&globals(osh)->int_lock, (state))
#define int_is_locked(osh)		spin_is_locked(&globals(osh)->int_lock)

/* Linux Kernel: File Operations: start */
/* forward declaration */
struct kstat;
/* prototypes */
extern void * osl_os_open_image(char * filename);
extern int osl_os_get_image_block(char * buf, int len, void * image);
extern void osl_os_close_image(void * image);
extern int osl_os_image_size(void *image);
extern struct file* open_file(const char *file_name, uint32 flags);
extern void close_file(struct file *fp);
extern int append_to_file(struct file *fp, uint8 *buf, int size, loff_t *pos);
extern int osl_os_get_image_stat(const char *filename, struct kstat *stat);
/* Linux Kernel: File Operations: end */

#define HEALTH_CHECK_SIG	SIGURG

#ifdef BCMDRIVER

/* OSL initialization */
extern osl_t *osl_attach(void *pdev, enum bustype_e bustype, bool pkttag);
extern void osl_detach(osl_t *osh);
extern int osl_static_mem_init(osl_t *osh, void *adapter);
extern int osl_static_mem_deinit(osl_t *osh, void *adapter);
extern void osl_set_bus_handle(osl_t *osh, void *bus_handle);
extern void* osl_get_bus_handle(osl_t *osh);

#ifdef CONFIG_WIFI_RETAIN_ALLOC
extern void osl_retain_alloc_attach(void);
extern void osl_retain_alloc_detach(void);
#endif /* CONFIG_WIFI_RETAIN_ALLOC */

/* Global ASSERT type */
extern uint32 g_assert_type;

#ifdef CONFIG_PHYS_ADDR_T_64BIT
#define PRI_FMT_x       "llx"
#define PRI_FMT_X       "llX"
#define PRI_FMT_o       "llo"
#define PRI_FMT_d       "lld"
#else
#define PRI_FMT_x       "x"
#define PRI_FMT_X       "X"
#define PRI_FMT_o       "o"
#define PRI_FMT_d       "d"
#endif /* CONFIG_PHYS_ADDR_T_64BIT */

/* ASSERT */
#ifndef ASSERT
#if defined(BCMDBG_ASSERT)
	/* WL_ASSERT() prints the unit number, which is handy when debugging NIC MLO */
	#define WL_ASSERT(exp, osh) \
		do { \
			if (!(exp)) osl_assert(#exp, __FILE__, __FUNCTION__, __LINE__, (osh)); \
		} while (0)
	#define ASSERT(exp) WL_ASSERT(exp, NULL)
extern void osl_assert(const char *exp, const char *file, const char *func, int line, osl_t *osh);
#else
#ifdef __GNUC__
		#define GCC_VERSION \
			(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if GCC_VERSION > 30100
			#define ASSERT(exp)	do {} while (0)
#else
			/* ASSERT could cause segmentation fault on GCC3.1, use empty instead */
			#define ASSERT(exp)
#endif /* GCC_VERSION > 30100 */
#endif /* __GNUC__ */
	#define WL_ASSERT(exp, osh) ASSERT(exp)
#endif // endif
#endif /* ASSERT */

/* bcm_prefetch_32B */
static inline void bcm_prefetch_32B(const uint8 *addr, const int cachelines_32B)
{
#if (defined(STB) && defined(__arm__)) && (__LINUX_ARM_ARCH__ >= 5)
	switch (cachelines_32B) {
		case 4: __asm__ __volatile__("pld\t%a0" :: "p"(addr + 96) : "cc");
		case 3: __asm__ __volatile__("pld\t%a0" :: "p"(addr + 64) : "cc");
		case 2: __asm__ __volatile__("pld\t%a0" :: "p"(addr + 32) : "cc");
		case 1: __asm__ __volatile__("pld\t%a0" :: "p"(addr +  0) : "cc");
	}
#elif defined(__mips__)
	/* Hint Pref_Load = 0 */
	switch (cachelines_32B) {
		case 4: __asm__ __volatile__("pref %0, (%1)" :: "i"(0), "r"(addr + 96));
		case 3: __asm__ __volatile__("pref %0, (%1)" :: "i"(0), "r"(addr + 64));
		case 2: __asm__ __volatile__("pref %0, (%1)" :: "i"(0), "r"(addr + 32));
		case 1: __asm__ __volatile__("pref %0, (%1)" :: "i"(0), "r"(addr +  0));
	}
#endif /* STB, __mips__ */
}

/* Get the current time in raw 64 bit format */
extern void wl_get_raw_epoch(u64 *timestamp);
struct task_struct* _get_task_info(const char *pname);
int osl_create_directory(char *pathname, int mode);

/* microsecond delay */
#define	OSL_DELAY(usec)		osl_delay(usec)
extern void osl_delay(uint usec);

#define OSL_SLEEP(ms)			osl_sleep(ms)
extern void osl_sleep(uint ms);

/* PCI configuration space access macros */
#define	OSL_PCI_READ_CONFIG(osh, offset, size) \
	osl_pci_read_config((osh), (offset), (size))
#define	OSL_PCI_WRITE_CONFIG(osh, offset, size, val) \
	osl_pci_write_config((osh), (offset), (size), (val))
extern uint32 osl_pci_read_config(osl_t *osh, uint offset, uint size);
extern void osl_pci_write_config(osl_t *osh, uint offset, uint size, uint val);

/* PCI device bus # and slot # */
#define OSL_PCI_BUS(osh)	osl_pci_bus(osh)
#define OSL_PCI_SLOT(osh)	osl_pci_slot(osh)
#define OSL_PCIE_DOMAIN(osh)	osl_pcie_domain(osh)
#define OSL_PCIE_BUS(osh)	osl_pcie_bus(osh)
#if (defined(CMWIFI) && !defined(BCMDONGLEHOST))
#define OSL_UNIT(osh)		osl_unit(osh)
#endif // endif
extern uint osl_pci_bus(osl_t *osh);
extern uint osl_pci_slot(osl_t *osh);
extern uint osl_pcie_domain(osl_t *osh);
extern uint osl_pcie_bus(osl_t *osh);
extern uint osl_unit(osl_t *osh);
extern void osl_unit_set(osl_t *osh, uint unit);
extern void osl_set_timers_context(osl_t *osh, void *context);
extern void* osl_get_timers_context(osl_t *osh);
extern struct pci_dev *osl_pci_device(osl_t *osh);

#define OSL_PCIE_ASPM_ENABLE(osh, linkcap_offset) osl_pcie_aspm_enable(osh, linkcap_offset, TRUE)
#define OSL_PCIE_ASPM_DISABLE(osh, linkcap_offset) osl_pcie_aspm_enable(osh, linkcap_offset, FALSE)

extern void osl_pcie_aspm_enable(osl_t *osh, uint linkcap_offset, bool aspm);

#define OSL_PCIE_MPS_LIMIT(osh, devctl_offset, mps)	osl_pcie_mps_limit(osh, devctl_offset, mps)
#define OSL_PCIE_MRRS_CONFIG(osh, devctl_offset, mrrs)	\
	osl_pcie_mrrs_config(osh, devctl_offset, mrrs)
extern void osl_pcie_mps_limit(osl_t *osh, uint devctl_offset, uint mps);
extern void osl_pcie_mrrs_config(osl_t *osh, uint devctl_offset, uint mrrs);

#define OSL_ACP_COHERENCE		(1<<1L)
#define OSL_FWDERBUF			(1<<2L)
typedef struct skb_list {
	struct sk_buff *head;
	struct sk_buff *tail;
	uint32_t len;
} skb_list_t;

#define BPM_SKB_FREE_BUDGET     128

#if defined(BCM_NBUFF_PKT) && (defined(CONFIG_BCM_BPM) || \
	defined(CONFIG_BCM_BPM_MODULE))
/* Maintain a local pool (cache) of SKBUFFs allocated from BPM */
#define BCM_NBUFF_PKTPOOL_CACHE
#define BCM_NBUFF_PKTPOOL_CACHE_MAXPKTS       64
#endif /* BCM_NBUFF_PKT && BPM */

/* Pkttag flag should be part of public information */
typedef struct {
	bool pkttag;
	bool mmbus;		/**< Bus supports memory-mapped register accesses */
	pktfree_cb_fn_t tx_fn;  /**< Callback function for PKTFREE */
	void *tx_ctx;		/**< Context to the callback function */
#ifdef OSLREGOPS
	osl_rreg_fn_t rreg_fn;	/**< Read Register function */
	osl_wreg_fn_t wreg_fn;	/**< Write Register function */
	void *reg_ctx;		/**< Context to the reg callback functions */
#else
	void	*unused[3];
#endif // endif
	void (*rx_fn)(void *rx_ctx, void *p);
	void *rx_ctx;
	void *bme;		/**< Byte Move Engine handler */
	void *m2m;		/**< M2M DMA Engine handler */
	void *globals;		/**< Per-driver storage */
	bool skb_free_offload;
	skb_list_t skbfreelist; /* List to accumulate the skbs to be freed */
	struct sk_buff * skb_cache;	/* Local cache of SKBs allocated from BPM */
	void *pcn_pktpool;	/* Packet Classification for NIC(PCN) packet pool */
	void *pp_xpm;		/* pp_xpm structure abstracted from sdk */
	bool wl_going_down;     /* Stop accumulation to freelist when TRUE */
} osl_pubinfo_t;

#ifdef BCA_CPE_BSP_SHARED
#include <dhd_nic_common.h>
#endif // endif

extern void osl_flag_set(osl_t *osh, uint32 mask);
extern void osl_flag_clr(osl_t *osh, uint32 mask);
extern bool osl_is_flag_set(osl_t *osh, uint32 mask);

#define PKTFREESETCB(osh, _tx_fn, _tx_ctx)		\
	do {						\
	   ((osl_pubinfo_t*)osh)->tx_fn = _tx_fn;	\
	   ((osl_pubinfo_t*)osh)->tx_ctx = _tx_ctx;	\
	} while (0)

#define PKTFREESETRXCB(osh, _rx_fn, _rx_ctx)		\
	do {						\
	   ((osl_pubinfo_t*)osh)->rx_fn = _rx_fn;	\
	   ((osl_pubinfo_t*)osh)->rx_ctx = _rx_ctx;	\
	} while (0)

#ifdef OSLREGOPS
#define REGOPSSET(osh, rreg, wreg, ctx)			\
	do {						\
	   ((osl_pubinfo_t*)osh)->rreg_fn = rreg;	\
	   ((osl_pubinfo_t*)osh)->wreg_fn = wreg;	\
	   ((osl_pubinfo_t*)osh)->reg_ctx = ctx;	\
	} while (0)
#endif /* OSLREGOPS */

/* host/bus architecture-specific byte swap */
#if defined(DSLCPE) && !defined(DSLCPE_SDIO)
#define BUS_SWAP32(v)		bcmswap32(v)
#else
#define BUS_SWAP32(v)		(v)
#endif // endif
#ifdef BCMDBG_MEM
	#define MALLOC(osh, size)	osl_debug_malloc((osh), (size), __LINE__, __FILE__, \
		__FUNCTION__)
	#define MALLOCZ(osh, size)	osl_debug_mallocz((osh), (size), __LINE__, __FILE__, \
		__FUNCTION__)
	#define MFREE(osh, addr, size)	osl_debug_mfree((osh), (addr), (size), __LINE__, __FILE__, \
		__FUNCTION__)
	#define VMALLOC(osh, size)	osl_debug_vmalloc((osh), (size), __LINE__, __FILE__, \
		__FUNCTION__)
	#define VMALLOCZ(osh, size)	osl_debug_vmallocz((osh), (size), __LINE__, __FILE__, \
		__FUNCTION__)
	#define VMFREE(osh, addr, size)	osl_debug_vmfree((osh), (addr), (size), __LINE__, \
		__FILE__, __FUNCTION__)
	#define MALLOCED(osh)		osl_malloced((osh))
	#define MEMORY_LEFTOVER(osh)	osl_check_memleak(osh)
	#define MALLOC_DUMP(osh, b)	osl_debug_memdump((osh), (b))
	extern void *osl_debug_malloc(osl_t *osh, uint size, int line, const char* file,
		const char *func);
	extern void *osl_debug_mallocz(osl_t *osh, uint size, int line, const char* file,
		const char *func);
	extern void osl_debug_mfree(osl_t *osh, void *addr, uint size, int line, const char* file,
		const char *func);
	extern void *osl_debug_vmalloc(osl_t *osh, uint size, int line, const char* file,
		const char *func);
	extern void *osl_debug_vmallocz(osl_t *osh, uint size, int line, const char* file,
		const char *func);
	extern void osl_debug_vmfree(osl_t *osh, void *addr, uint size, int line, const char* file,
		const char *func);
	extern uint osl_malloced(osl_t *osh);
	struct bcmstrbuf;
	extern int osl_debug_memdump(osl_t *osh, struct bcmstrbuf *b);
	extern uint osl_check_memleak(osl_t *osh);
#else
	#define MALLOC(osh, size)	osl_malloc((osh), (size))
	#define MALLOCZ(osh, size)	osl_mallocz((osh), (size))
	#define MFREE(osh, addr, size)	osl_mfree((osh), (addr), (size))
	#define VMALLOC(osh, size)	osl_vmalloc((osh), (size))
	#define VMALLOCZ(osh, size)	osl_vmallocz((osh), (size))
	#define VMFREE(osh, addr, size)	osl_vmfree((osh), (addr), (size))
	#define MALLOCED(osh)		osl_malloced((osh))
	#define MEMORY_LEFTOVER(osh) osl_check_memleak(osh)
	extern void *osl_malloc(osl_t *osh, uint size);
	extern void *osl_mallocz(osl_t *osh, uint size);
	extern void osl_mfree(osl_t *osh, void *addr, uint size);
	extern void *osl_vmalloc(osl_t *osh, uint size);
	extern void *osl_vmallocz(osl_t *osh, uint size);
	extern void osl_vmfree(osl_t *osh, void *addr, uint size);
	extern uint osl_malloced(osl_t *osh);
	extern uint osl_check_memleak(osl_t *osh);
#endif /* BCMDBG_MEM */

struct tkcore_alloc_entry;
extern int osl_get_alloc_entry(void *ctx, struct tkcore_alloc_entry *entry);

/* use vmalloc in case of allocation failure for get ioctl */
#define IOCTL_MALLOC(osh, set, size)        (((set) == 0) ? VMALLOC((osh), (size)) : \
							MALLOC((osh), (size)))
#define IOCTL_MFREE(osh, set, addr, size)   (((set) == 0) ? VMFREE((osh), (addr), (size)) : \
							MFREE((osh), (addr), (size)))

#define	MALLOC_FAILED(osh)	osl_malloc_failed((osh))
extern uint osl_malloc_failed(osl_t *osh);

/* allocate/free shared (dma-able) consistent memory (NOTE: allocated buffer is not guaranteed
 * to follow alignment request; it IS guaranteed however to be large enough for caller to honour
 * the requirement)
 */
#define	DMA_ALLOC_CONSISTENT_BOUNDARY(osh, size, palignbits, boundarybits, tot, pap, dmah) \
	osl_dma_alloc_consistent((osh), (size), (*palignbits), (tot), (pap))
#define	DMA_ALLOC_CONSISTENT(osh, size, align, tot, pap, dmah) \
	osl_dma_alloc_consistent((osh), (size), (align), (tot), (pap))
#define	DMA_FREE_CONSISTENT(osh, va, size, pa, dmah) \
	osl_dma_free_consistent((osh), (void*)(va), (size), (pa))

#define	DMA_ALLOC_CONSISTENT_FORCE32(osh, size, align, tot, pap, dmah) \
	osl_dma_alloc_consistent((osh), (size), (align), (tot), (pap))
#define	DMA_FREE_CONSISTENT_FORCE32(osh, va, size, pa, dmah) \
	osl_dma_free_consistent((osh), (void*)(va), (size), (pa))

/* Note that this function (despite what its parameters may suggest) does NOT guarantee alignment
 * of the returned buffer. It does allocate sufficient bytes for caller to honour the requirement.
 */
extern void *osl_dma_alloc_consistent(osl_t *osh, uint size, uint16 align,
	uint *tot, dmaaddr_t *pap);
extern void osl_dma_free_consistent(osl_t *osh, void *va, uint size, dmaaddr_t pa);

extern void *osl_dma_alloc_contiguous(osl_t *osh, uint size, uint16 align_bits,
	uint *alloced, dmaaddr_t *pap);
extern void osl_dma_free_contiguous(osl_t *osh, void * va, uint size, dmaaddr_t pa);

#define DMA_ALLOC_CONTIGUOUS(osh, size, align_bits, tot, pap, dmah)	\
	osl_dma_alloc_contiguous((osh), (size), (align_bits), (tot), (pap))
#define DMA_FREE_CONTIGUOUS(osh, va, size, pa, dmah)		\
	osl_dma_free_contiguous((osh), (va), (size), (pa))

/* map/unmap direction */
#define DMA_NO	0	/* Used to skip cache op */
#define	DMA_TX	1	/* TX direction for DMA */
#define	DMA_RX	2	/* RX direction for DMA */

/* map/unmap shared (dma-able) memory */
#define	DMA_UNMAP(osh, pa, size, direction, p, dmah) \
	osl_dma_unmap((osh), (pa), (size), (direction))
extern void osl_dma_flush(osl_t *osh, void *va, uint size, int direction, void *p,
	hnddma_seg_map_t *txp_dmah);
extern void osl_dma_sync(osl_t *osh, dmaaddr_t pa, uint size, int direction);
extern dmaaddr_t osl_dma_map(osl_t *osh, void *va, uint size, int direction, void *p,
	hnddma_seg_map_t *txp_dmah);
extern void osl_dma_unmap(osl_t *osh, dmaaddr_t pa, uint size, int direction);

#ifndef PHYS_TO_VIRT
#define	PHYS_TO_VIRT(pa)	osl_phys_to_virt(pa)
#endif // endif
#ifndef VIRT_TO_PHYS
#define	VIRT_TO_PHYS(va)	osl_virt_to_phys(va)
#endif // endif
extern void * osl_phys_to_virt(void * pa);
extern void * osl_virt_to_phys(void * va);

#if defined(BCMDMA64OSL) && !defined(CONFIG_ARM)
#define BCMVADDR64OSL
#else  /* !BCMDMA64OSL || CONFIG_ARM */
#define BCMVADDR32OSL
#endif /* !BCMDMA64OSL || CONFIG_ARM */

#ifndef OSL_VIRT_TO_PHYSADDR
#if defined(BCMDMA64OSL) && defined(BCMVADDR32OSL)
#define	OSL_VIRT_TO_PHYSADDR(osh, va, pa)	                           \
	ULONGTOPHYSADDR(osl_get_virt_to_phys((osh), (va)), (pa))
extern phys_addr_t osl_get_virt_to_phys(osl_t *osh, void *va);
#else
#define	OSL_VIRT_TO_PHYSADDR(osh, va, pa)	                           \
	ULONGTOPHYSADDR((unsigned long)VIRT_TO_PHYS((va)), (pa))
#endif /* !BCMDMA64OSL || !BCMVADDR32OSL */
#endif /* !OSL_VIRT_TO_PHYSADDR */

/* API for DMA addressing capability */
#define OSL_DMADDRWIDTH(osh, addrwidth) ({BCM_REFERENCE(osh); BCM_REFERENCE(addrwidth);})

#define OSL_SMP_WMB()	smp_wmb()

#if defined(CONFIG_BCM_GLB_COHERENCY)
/* Compile time macro OSL_CACHE_COHERENT, instead of OSL_ARCH_IS_COHERENT() */
#define OSL_CACHE_COHERENT    1
#define ACP_WAR_ENAB()        0
#define ACP_WIN_LIMIT          (~0ULL)    /* entire memory */
#define arch_is_coherent()     1
#else /* !CONFIG_BCM_GLB_COHERENCY */
#if defined(BCM_ROUTER) && defined(__ARM_ARCH_7A__)
/* ACP definitions for 47189 + Linux-4.1 */
#define ACP_WAR_ENAB()	0		/* Do not need this WAR for 47189 b1 */
#define ACP_WIN_LIMIT	0xff	/* Added to passthrough the compilation */
#define arch_is_coherent()	0
#endif /* BCM_ROUTER && __ARM_ARCH_7A__ */
#endif /* !CONFIG_BCM_GLB_COHERENCY */

/* API for CPU relax */
extern void osl_cpu_relax(void);
#define OSL_CPU_RELAX() osl_cpu_relax()

extern void osl_preempt_disable(osl_t *osh);
extern void osl_preempt_enable(osl_t *osh);
#define OSL_DISABLE_PREEMPTION(osh)	osl_preempt_disable(osh)
#define OSL_ENABLE_PREEMPTION(osh)	osl_preempt_enable(osh)

#if ((defined STB) || (defined STBAP)) || (defined(CMWIFI) && (LINUX_VERSION_CODE >= \
	KERNEL_VERSION(5, 4, 0)))

/* Point to osh holder var for dhd and nic drivers */
#if defined(BCMDONGLEHOST)
#define OSHPTR dhd->osh
#else
#define OSHPTR wlc->osh
#endif /* BCMDONGLEHOST */
	extern void osl_cache_flush(osl_t* osh, void *va, uint size);
	extern void osl_cache_inv(osl_t* osh, void *va, uint size);
	extern void osl_prefetch(const void *ptr);
	#define OSL_CACHE_FLUSH(va, len)   osl_cache_flush(OSHPTR, (void *)(va), len)
	#define OSL_CACHE_INV(va, len)     osl_cache_inv(OSHPTR, (void *)(va), len)

	#define OSL_PREFETCH(ptr)               osl_prefetch(ptr)
	#define OSL_ARCH_IS_COHERENT()          NULL
	#define OSL_ACP_WAR_ENAB()              NULL

#elif defined(__mips__) || \
	(!defined(DHD_USE_COHERENT_MEM_FOR_RING)&&defined(__ARM_ARCH_7A__)) || \
(defined(BCM_ROUTER) && defined(__aarch64__)) || \
(defined(CMWIFI) && defined(__ARM_ARCH_7A__))
	extern void osl_cache_flush(void *va, uint size);
	extern void osl_cache_inv(void *va, uint size);
	extern void osl_prefetch(const void *ptr);
	#define OSL_CACHE_FLUSH(va, len)	osl_cache_flush((void *)(va), len)
	#define OSL_CACHE_INV(va, len)		osl_cache_inv((void *)(va), len)
	#define OSL_PREFETCH(ptr)			osl_prefetch(ptr)
#if defined(__ARM_ARCH_7A__) || defined(OSL_CACHE_COHERENT)
	extern int osl_arch_is_coherent(void);
	#define OSL_ARCH_IS_COHERENT()		osl_arch_is_coherent()
	extern int osl_acp_war_enab(void);
	#define OSL_ACP_WAR_ENAB()			osl_acp_war_enab()
#else  /* !__ARM_ARCH_7A__ */
	#define OSL_ARCH_IS_COHERENT()		NULL
	#define OSL_ACP_WAR_ENAB()			NULL
#endif /* !__ARM_ARCH_7A__ */
#else  /* !__mips__ && !__ARM_ARCH_7A__ */
	#define OSL_CACHE_FLUSH(va, len)	({BCM_REFERENCE(va); BCM_REFERENCE(len);})
	#define OSL_CACHE_INV(va, len)		({BCM_REFERENCE(va); BCM_REFERENCE(len);})
	#define OSL_PREFETCH(ptr)		BCM_REFERENCE(ptr)

	#define OSL_ARCH_IS_COHERENT()		NULL
	#define OSL_ACP_WAR_ENAB()			NULL
#endif /* (defined STB) || (defined STBAP) */

#ifdef BCM_BACKPLANE_TIMEOUT
extern void osl_set_bpt_cb(osl_t *osh, void *bpt_cb, void *bpt_ctx);
extern void osl_bpt_rreg(osl_t *osh, ulong addr, volatile void *v, uint size);
#endif /* BCM_BACKPLANE_TIMEOUT */

#if defined(STB) && defined(__arm__)
extern void osl_pcie_rreg(osl_t *osh, ulong addr, volatile void *v, uint size);
#endif	/* STB && __arm__ */

/* register access macros */
#if defined(BCMJTAG)
	#include <bcmjtag.h>
	#define OSL_WRITE_REG(osh, r, v) \
		({ \
		 BCM_REFERENCE(osh); \
		 bcmjtag_write(NULL, (uintptr)(r), (v), sizeof(*(r))); \
		 })
	#define OSL_READ_REG(osh, r) \
		({ \
		 BCM_REFERENCE(osh); \
		 bcmjtag_read(NULL, (uintptr)(r), sizeof(*(r)));
		 })
#elif defined(BCM_BACKPLANE_TIMEOUT)
#define OSL_READ_REG(osh, r) \
	({\
		__typeof(*(r)) __osl_v; \
		osl_bpt_rreg(osh, (uintptr)(r), &__osl_v, sizeof(*(r))); \
		__osl_v; \
	})
#elif (defined(STB) && defined(__arm__))
#define OSL_READ_REG(osh, r) \
	({\
		__typeof(*(r)) __osl_v; \
		osl_pcie_rreg(osh, (uintptr)(r), &__osl_v, sizeof(*(r))); \
		__osl_v; \
	})
#endif // endif

#if defined(BCM47XX_CA9) || defined(BCM_BACKPLANE_TIMEOUT) || (defined(STB) && \
	defined(__arm__))
	#define SELECT_BUS_WRITE(osh, mmap_op, bus_op) ({BCM_REFERENCE(osh); mmap_op;})
	#define SELECT_BUS_READ(osh, mmap_op, bus_op) ({BCM_REFERENCE(osh); bus_op;})
#else /* !BCM47XX_CA9 && !BCM_BACKPLANE_TIMEOUT && !(STB && __arm__) */
#if defined(BCMJTAG)
	#define SELECT_BUS_WRITE(osh, mmap_op, bus_op) if (((osl_pubinfo_t*)(osh))->mmbus) \
		mmap_op else bus_op
	#define SELECT_BUS_READ(osh, mmap_op, bus_op) (((osl_pubinfo_t*)(osh))->mmbus) ? \
		mmap_op : bus_op
#else
	#define SELECT_BUS_WRITE(osh, mmap_op, bus_op) ({BCM_REFERENCE(osh); mmap_op;})
	#define SELECT_BUS_READ(osh, mmap_op, bus_op) ({BCM_REFERENCE(osh); mmap_op;})
#endif // endif
#endif /* BCM47XX_CA9 || BCM_BACKPLANE_TIMEOUT || (STB && __arm__) */

#define OSL_ERROR(bcmerror)	osl_error(bcmerror)
extern int osl_error(int bcmerror);

/* the largest reasonable packet buffer driver uses for ethernet MTU in bytes */
#define	PKTBUFSZ	2048   /* largest reasonable packet buffer, driver uses for ethernet MTU */

#define OSH_NULL   NULL

/*
 * BINOSL selects the slightly slower function-call-based binary compatible osl.
 * Macros expand to calls to functions defined in linux_osl.c .
 */
#ifndef BINOSL
#include <linuxver.h>           /* use current 2.4.x calling conventions */
#include <linux/kernel.h>       /* for vsn/printf's */
#include <linux/string.h>       /* for mem*, str* */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 29)
extern uint64 osl_sysuptime_us(void);
#define OSL_SYSUPTIME()		((uint32)jiffies_to_msecs(jiffies))
#define OSL_SYSUPTIME_US()	osl_sysuptime_us()
#else
#define OSL_SYSUPTIME()		((uint32)jiffies * (1000 / HZ))
#error "OSL_SYSUPTIME_US() may need to be defined"
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 29) */
#define	printf(fmt, args...)	printk(KERN_CONT fmt , ## args)
#include <linux/kernel.h>	/* for vsn/printf's */
#include <linux/string.h>	/* for mem*, str* */
/* bcopy's: Linux kernel doesn't provide these (anymore) */
#define	bcopy_hw(src, dst, len)		memcpy((dst), (src), (len))
#define	bcopy_hw_async(src, dst, len)	memcpy((dst), (src), (len))
#define	bcopy_hw_poll_for_completion()
#define	bcopy(src, dst, len)	memcpy((dst), (src), (len))
#define	bcmp(b1, b2, len)	memcmp((b1), (b2), (len))
#define	bzero(b, len)		memset((b), '\0', (len))

/* register access macros */
#if defined(OSLREGOPS)
#define R_REG(osh, r) (\
	sizeof(*(r)) == sizeof(uint8) ? osl_readb((osh), (volatile uint8*)(r)) : \
	sizeof(*(r)) == sizeof(uint16) ? osl_readw((osh), (volatile uint16*)(r)) : \
	osl_readl((osh), (volatile uint32*)(r)) \
)
#define _W_REG(osh, r, v) do { \
	switch (sizeof(*(r))) { \
	case sizeof(uint8):	osl_writeb((osh), (volatile uint8*)(r), (uint8)(v)); break; \
	case sizeof(uint16):	osl_writew((osh), (volatile uint16*)(r), (uint16)(v)); break; \
	case sizeof(uint32):	osl_writel((osh), (volatile uint32*)(r), (uint32)(v)); break; \
	} \
} while (0)

extern uint8 osl_readb(osl_t *osh, volatile uint8 *r);
extern uint16 osl_readw(osl_t *osh, volatile uint16 *r);
extern uint32 osl_readl(osl_t *osh, volatile uint32 *r);
extern void osl_writeb(osl_t *osh, volatile uint8 *r, uint8 v);
extern void osl_writew(osl_t *osh, volatile uint16 *r, uint16 v);
extern void osl_writel(osl_t *osh, volatile uint32 *r, uint32 v);

#else /* OSLREGOPS */

#ifndef IL_BIGENDIAN
#ifndef __mips__
#ifdef CONFIG_64BIT
/* readq is defined only for 64 bit platform */
#define R_REG(osh, r) (\
	SELECT_BUS_READ(osh, \
		({ \
			__typeof(*(r)) __osl_v = 0; \
			BCM_REFERENCE(osh);	\
			switch (sizeof(*(r))) { \
				case sizeof(uint8):	__osl_v = \
					readb((volatile uint8*)(r)); break; \
				case sizeof(uint16):	__osl_v = \
					readw((volatile uint16*)(r)); break; \
				case sizeof(uint32):	__osl_v = \
					readl((volatile uint32*)(r)); break; \
				case sizeof(uint64):	__osl_v = \
					readq((volatile uint64*)(r)); break; \
			} \
			__osl_v; \
		}), \
		OSL_READ_REG(osh, r)) \
)
#else /* !CONFIG_64BIT */
#define R_REG(osh, r) (\
	SELECT_BUS_READ(osh, \
		({ \
			__typeof(*(r)) __osl_v = 0; \
			switch (sizeof(*(r))) { \
				case sizeof(uint8):	__osl_v = \
					readb((volatile uint8*)(r)); break; \
				case sizeof(uint16):	__osl_v = \
					readw((volatile uint16*)(r)); break; \
				case sizeof(uint32):	__osl_v = \
					readl((volatile uint32*)(r)); break; \
			} \
			__osl_v; \
		}), \
		OSL_READ_REG(osh, r)) \
)
#endif /* CONFIG_64BIT */

#else /* __mips__ */

#ifdef CONFIG_64BIT
/* readq is defined only for 64 bit platform */
#define R_REG(osh, r) (\
	SELECT_BUS_READ(osh, \
		({ \
			__typeof(*(r)) __osl_v = 0; \
			__asm__ __volatile__("sync"); \
			switch (sizeof(*(r))) { \
				case sizeof(uint8):	__osl_v = \
					readb((volatile uint8*)(r)); break; \
				case sizeof(uint16):	__osl_v = \
					readw((volatile uint16*)(r)); break; \
				case sizeof(uint32):	__osl_v = \
					readl((volatile uint32*)(r)); break; \
				case sizeof(uint64):	__osl_v = \
					readq((volatile uint64*)(r)); break; \
			} \
			__asm__ __volatile__("sync"); \
			__osl_v; \
		}), \
		({ \
			__typeof(*(r)) __osl_v; \
			__asm__ __volatile__("sync"); \
			__osl_v = OSL_READ_REG(osh, r); \
			__asm__ __volatile__("sync"); \
			__osl_v; \
		})) \
)
#else /* !CONFIG_64BIT */
#define R_REG(osh, r) (\
	SELECT_BUS_READ(osh, \
		({ \
			__typeof(*(r)) __osl_v = 0; \
			__asm__ __volatile__("sync"); \
			switch (sizeof(*(r))) { \
				case sizeof(uint8):	__osl_v = \
					readb((volatile uint8*)(r)); break; \
				case sizeof(uint16):	__osl_v = \
					readw((volatile uint16*)(r)); break; \
				case sizeof(uint32):	__osl_v = \
					readl((volatile uint32*)(r)); break; \
			} \
			__asm__ __volatile__("sync"); \
			__osl_v; \
		}), \
		({ \
			__typeof(*(r)) __osl_v__; \
			__asm__ __volatile__("sync"); \
			__osl_v__ = OSL_READ_REG(osh, r); \
			__asm__ __volatile__("sync"); \
			__osl_v__; \
		})) \
)
#endif /* CONFIG_64BIT */
#endif /* __mips__ */

#ifdef CONFIG_64BIT
/* writeq is defined only for 64 bit platform */
#define _W_REG(osh, r, v) do { \
	SELECT_BUS_WRITE(osh, \
		switch (sizeof(*(r))) { \
			case sizeof(uint8):	writeb((uint8)(v), (volatile uint8*)(r)); break; \
			case sizeof(uint16):	writew((uint16)(v), (volatile uint16*)(r)); break; \
			case sizeof(uint32):	writel((uint32)(v), (volatile uint32*)(r)); break; \
			case sizeof(uint64):	writeq((uint64)(v), (volatile uint64*)(r)); break; \
		}, \
		(OSL_WRITE_REG(osh, r, v))); \
	} while (0)

#else /* !CONFIG_64BIT */
#define _W_REG(osh, r, v) do { \
	SELECT_BUS_WRITE(osh, \
		switch (sizeof(*(r))) { \
			case sizeof(uint8):	writeb((uint8)(v), (volatile uint8*)(r)); break; \
			case sizeof(uint16):	writew((uint16)(v), (volatile uint16*)(r)); break; \
			case sizeof(uint32):	writel((uint32)(v), (volatile uint32*)(r)); break; \
		}, \
		(OSL_WRITE_REG(osh, r, v))); \
	} while (0)
#endif /* CONFIG_64BIT */

#else	/* IL_BIGENDIAN */

#ifdef CONFIG_64BIT
/* readq and writeq is defined only for 64 bit platform */
#define R_REG(osh, r) (\
	SELECT_BUS_READ(osh, \
		({ \
			__typeof(*(r)) __osl_v = 0; \
			switch (sizeof(*(r))) { \
				case sizeof(uint8):	__osl_v = \
					readb((volatile uint8*)((uintptr)(r)^3)); break; \
				case sizeof(uint16):	__osl_v = \
					readw((volatile uint16*)((uintptr)(r)^2)); break; \
				case sizeof(uint32):	__osl_v = \
					readl((volatile uint32*)(r)); break; \
				case sizeof(uint64):    __osl_v = \
					readq((volatile uint64*)(r)); break; \
			} \
			__osl_v; \
		}), \
		OSL_READ_REG(osh, r)) \
)
#define _W_REG(osh, r, v) do { \
	SELECT_BUS_WRITE(osh, \
		switch (sizeof(*(r))) { \
			case sizeof(uint8):	writeb((uint8)(v), \
					(volatile uint8*)((uintptr)(r)^3)); break; \
			case sizeof(uint16):	writew((uint16)(v), \
					(volatile uint16*)((uintptr)(r)^2)); break; \
			case sizeof(uint32):	writel((uint32)(v), \
					(volatile uint32*)(r)); break; \
			case sizeof(uint64):	writeq((uint64)(v), \
					(volatile uint64*)(r)); break; \
		}, \
		(OSL_WRITE_REG(osh, r, v))); \
	} while (0)

#else /* !CONFIG_64BIT */
#define R_REG(osh, r) (\
	SELECT_BUS_READ(osh, \
		({ \
			__typeof(*(r)) __osl_v = 0; \
			switch (sizeof(*(r))) { \
				case sizeof(uint8):	__osl_v = \
					readb((volatile uint8*)((uintptr)(r)^3)); break; \
				case sizeof(uint16):	__osl_v = \
					readw((volatile uint16*)((uintptr)(r)^2)); break; \
				case sizeof(uint32):	__osl_v = \
					readl((volatile uint32*)(r)); break; \
			} \
			__osl_v; \
		}), \
		OSL_READ_REG(osh, r)) \
)
#define _W_REG(osh, r, v) do { \
	SELECT_BUS_WRITE(osh, \
		switch (sizeof(*(r))) { \
			case sizeof(uint8):	writeb((uint8)(v), \
					(volatile uint8*)((uintptr)(r)^3)); break; \
			case sizeof(uint16):	writew((uint16)(v), \
					(volatile uint16*)((uintptr)(r)^2)); break; \
			case sizeof(uint32):	writel((uint32)(v), \
					(volatile uint32*)(r)); break; \
		}, \
		(OSL_WRITE_REG(osh, r, v))); \
	} while (0)
#endif /* CONFIG_64BIT */
#endif /* IL_BIGENDIAN */

#endif /* OSLREGOPS */

/* bcopy, bcmp, and bzero functions */
#define	bcopy(src, dst, len)	memcpy((dst), (src), (len))
#define	bcmp(b1, b2, len)	memcmp((b1), (b2), (len))
#define	bzero(b, len)		memset((b), '\0', (len))

/* uncached/cached virtual address */
#ifdef __mips__
#include <asm/addrspace.h>
#define OSL_UNCACHED(va)	((void *)KSEG1ADDR((va)))
#define OSL_CACHED(va)		((void *)KSEG0ADDR((va)))
#else
#define OSL_UNCACHED(va)	((void *)va)
#define OSL_CACHED(va)		((void *)va)
#endif /* mips */

#ifdef __mips__
#define OSL_PREF_RANGE_LD(va, sz) prefetch_range_PREF_LOAD_RETAINED(va, sz)
#define OSL_PREF_RANGE_ST(va, sz) prefetch_range_PREF_STORE_RETAINED(va, sz)
#else /* __mips__ */
#define OSL_PREF_RANGE_LD(va, sz) BCM_REFERENCE(va)
#define OSL_PREF_RANGE_ST(va, sz) BCM_REFERENCE(va)
#endif /* __mips__ */

/* get processor cycle count */
#if defined(mips)
#define	OSL_GETCYCLES(x)	((x) = read_c0_count() * 2)
#elif defined(__i386__)
#define	OSL_GETCYCLES(x)	rdtscl((x))
#else
#define OSL_GETCYCLES(x)	((x) = 0)
#endif /* defined(mips) */

/* dereference an address that may cause a bus exception */
#ifdef mips
#if defined(MODULE) && (LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 17))
#define BUSPROBE(val, addr)	panic("get_dbe() will not fixup a bus exception when compiled into"\
					" a module")
#else
#define	BUSPROBE(val, addr)	get_dbe((val), (addr))
#include <asm/paccess.h>
#endif /* defined(MODULE) && (LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 17)) */
#else
#define	BUSPROBE(val, addr)	({ (val) = R_REG(NULL, (addr)); 0; })
#endif /* mips */

/* map/unmap physical to virtual I/O */
#if !defined(CONFIG_MMC_MSM7X00A)
#define	REG_MAP(pa, size)	ioremap((unsigned long)(pa), (unsigned long)(size))
#else
#define REG_MAP(pa, size)       (void *)(0)
#endif /* !defined(CONFIG_MMC_MSM7X00A */
#define	REG_UNMAP(va)		iounmap((va))

/* shared (dma-able) memory access macros */
#define	R_SM(r)			*(r)
#define	W_SM(r, v)		(*(r) = (v))
#define	BZERO_SM(r, len)	memset((r), '\0', (len))

/* Because the non BINOSL implemenation of the PKT OSL routines are macros (for
 * performance reasons),  we need the Linux headers.
 */
/* XXX REVISIT  Is there a more specific header file we should be including for the
 * struct/definitions we need? johnvb
 */
#include <linuxver.h>		/* use current 2.4.x calling conventions */

#else	/* BINOSL */

/* Where to get the declarations for mem, str, printf, bcopy's? Two basic approaches.
 *
 * First, use the Linux header files and the C standard library replacmenent versions
 * built-in to the kernel.  Use this approach when compiling non hybrid code or compling
 * the OS port files.  The second approach is to use our own defines/prototypes and
 * functions we have provided in the Linux OSL, i.e. linux_osl.c.  Use this approach when
 * compiling the files that make up the hybrid binary.  We are ensuring we
 * don't directly link to the kernel replacement routines from the hybrid binary.
 *
 * NOTE: The issue we are trying to avoid is any questioning of whether the
 * hybrid binary is derived from Linux.  The wireless common code (wlc) is designed
 * to be OS independent through the use of the OSL API and thus the hybrid binary doesn't
 * derive from the Linux kernel at all.  But since we defined our OSL API to include
 * a small collection of standard C library routines and these routines are provided in
 * the kernel we want to avoid even the appearance of deriving at all even though clearly
 * usage of a C standard library API doesn't represent a derivation from Linux.  Lastly
 * note at the time of this checkin 4 references to memcpy/memset could not be eliminated
 * from the binary because they are created internally by GCC as part of things like
 * structure assignment.  I don't think the compiler should be doing this but there is
 * no options to disable it on Intel architectures (there is for MIPS so somebody must
 * agree with me).  I may be able to even remove these references eventually with
 * a GNU binutil such as objcopy via a symbol rename (i.e. memcpy to osl_memcpy).
 */
	#define	printf(fmt, args...)	printk(KERN_CONT fmt , ## args)
	#include <linux/kernel.h>	/* for vsn/printf's */
	#include <linux/string.h>	/* for mem*, str* */
	/* bcopy's: Linux kernel doesn't provide these (anymore) */
	#define	bcopy(src, dst, len)	memcpy((dst), (src), (len))
	#define	bcmp(b1, b2, len)	memcmp((b1), (b2), (len))
	#define	bzero(b, len)		memset((b), '\0', (len))

	/* These are provided only because when compiling linux_osl.c there
	 * must be an explicit prototype (separate from the definition) because
	 * we are compiling with GCC option -Wstrict-prototypes.  Conversely
	 * these could be placed directly in linux_osl.c.
	 */
	extern int osl_printf(const char *format, ...);
	extern int osl_sprintf(char *buf, const char *format, ...);
	extern int osl_snprintf(char *buf, size_t n, const char *format, ...);
	extern int osl_vsprintf(char *buf, const char *format, va_list ap);
	extern int osl_vsnprintf(char *buf, size_t n, const char *format, va_list ap);
	extern int osl_strcmp(const char *s1, const char *s2);
	extern int osl_strncmp(const char *s1, const char *s2, uint n);
	extern int osl_strlen(const char *s);
	extern char* osl_strcpy(char *d, const char *s);
	extern char* osl_strncpy(char *d, const char *s, uint n);
	extern char* osl_strchr(const char *s, int c);
	extern char* osl_strrchr(const char *s, int c);
	extern void *osl_memset(void *d, int c, size_t n);
	extern void *osl_memcpy(void *d, const void *s, size_t n);
	extern void *osl_memmove(void *d, const void *s, size_t n);
	extern int osl_memcmp(const void *s1, const void *s2, size_t n);

/* register access macros */
#if !defined(BCMJTAG)
#define R_REG(osh, r) \
	({ \
	 BCM_REFERENCE(osh); \
	 sizeof(*(r)) == sizeof(uint8) ? osl_readb((volatile uint8*)(r)) : \
	 sizeof(*(r)) == sizeof(uint16) ? osl_readw((volatile uint16*)(r)) : \
	 osl_readl((volatile uint32*)(r)); \
	 })
#define _W_REG(osh, r, v) do { \
	BCM_REFERENCE(osh); \
	switch (sizeof(*(r))) { \
	case sizeof(uint8):	osl_writeb((uint8)(v), (volatile uint8*)(r)); break; \
	case sizeof(uint16):	osl_writew((uint16)(v), (volatile uint16*)(r)); break; \
	case sizeof(uint32):	osl_writel((uint32)(v), (volatile uint32*)(r)); break; \
	} \
} while (0)

/* else added by johnvb to make sdio and jtag work with BINOSL, at least compile ... UNTESTED */
#else
#define R_REG(osh, r) OSL_READ_REG(osh, r)
#define _W_REG(osh, r, v) do { OSL_WRITE_REG(osh, r, v); } while (0)
#endif // endif

extern uint8 osl_readb(volatile uint8 *r);
extern uint16 osl_readw(volatile uint16 *r);
extern uint32 osl_readl(volatile uint32 *r);
extern void osl_writeb(uint8 v, volatile uint8 *r);
extern void osl_writew(uint16 v, volatile uint16 *r);
extern void osl_writel(uint32 v, volatile uint32 *r);

/* system up time in ms */
#define OSL_SYSUPTIME()		osl_sysuptime()
extern uint32 osl_sysuptime(void);

/* uncached/cached virtual address */
#define OSL_UNCACHED(va)	osl_uncached((va))
extern void *osl_uncached(void *va);
#define OSL_CACHED(va)		osl_cached((va))
extern void *osl_cached(void *va);

#define OSL_PREF_RANGE_LD(va, sz)
#define OSL_PREF_RANGE_ST(va, sz)

/* get processor cycle count */
#define OSL_GETCYCLES(x)	((x) = osl_getcycles())
extern uint osl_getcycles(void);

/* dereference an address that may target abort */
#define	BUSPROBE(val, addr)	osl_busprobe(&(val), (addr))
extern int osl_busprobe(uint32 *val, uint32 addr);

/* map/unmap physical to virtual */
#define	REG_MAP(pa, size)	osl_reg_map((pa), (size))
#define	REG_UNMAP(va)		osl_reg_unmap((va))
extern void *osl_reg_map(uint32 pa, uint size);
extern void osl_reg_unmap(void *va);

/* shared (dma-able) memory access macros */
#define	R_SM(r)			*(r)
#define	W_SM(r, v)		(*(r) = (v))
#define	BZERO_SM(r, len)	bzero((r), (len))

#endif	/* BINOSL */

#define W_REG(osh, r, v) do { \
	SIZECHECK(*(r), (v)); \
	_W_REG(osh, r, v); \
} while (0)

#define AND_REG(osh, r, v) do { \
	SIZECHECK(*(r), (v)); \
	_W_REG(osh, (r), R_REG(osh, r) & (v)); \
} while (0)

#define OR_REG(osh, r, v) do { \
	SIZECHECK(*(r), (v)); \
	_W_REG(osh, (r), R_REG(osh, r) | (v)); \
} while (0)

#define OSL_RAND()		osl_rand()
extern uint32 osl_rand(void);

#define	DMA_FLUSH(osh, va, size, direction, p, dmah) \
	osl_dma_flush((osh), (va), (size), (direction), (p), (dmah))
#define	DMA_SYNC(osh, pa, size, direction) \
	osl_dma_sync((osh), (pa), (size), (direction))
#if !defined(BCM_SECURE_DMA)
#define DMA_MAP(osh, va, size, direction, p, dmah) \
	osl_dma_map((osh), (va), (size), (direction), (p), (dmah))

#define BULK_DMA_MAP(osh, dmah, map_start, map_end) \
	({\
		BCM_REFERENCE(dmah); \
		BCM_REFERENCE(osh); \
		BCM_REFERENCE(map_start); \
		BCM_REFERENCE(map_end); \
	})

#define BULK_DMA_UNMAP(osh, dmah, map_start, map_end) \
	({\
		BCM_REFERENCE(dmah); \
		BCM_REFERENCE(osh); \
		BCM_REFERENCE(map_start); \
		BCM_REFERENCE(map_end); \
	})

#endif /* !(defined(BCM_SECURE_DMA)) */

/*
 * The main router kernel has a separate namespace for some FS functions that
 * are exported in the mainline kernel. These functions include filp_open,
 * kernel_read, kernel_write, kern_path, and close_fd.  For compatibility we
 * import them using the NS below. If the NS doesn't exist, linux doesn't seem
 * to complain.
 */
#ifdef MODULE_IMPORT_NS
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif // endif
#else /* ! BCMDRIVER */

/* XXX  Non BCMDRIVER code "OSL".
 *   There are only a very limited number of OSL API's made available here:
 *     mem*'s, str*'s, b*'s, *printf's, MALLOC/MFREE and ASSERT.  All others are
 *   missing.  This doesn't really seem like an OSL implementation.  I am wondering
 *   if non BCMDRIVER code should be using a different header file defined for that
 *   purpose.  johnvb.
 */

/* ASSERT */
#ifdef BCMDBG_ASSERT
	#include <assert.h>
	#define ASSERT assert
#else /* BCMDBG_ASSERT */
	#define ASSERT(exp)	do {} while (0)
#endif /* BCMDBG_ASSERT */

/* MALLOC and MFREE */
#define MALLOC(o, l) malloc(l)
#define MFREE(o, p, l) free(p)
#include <stdlib.h>

/* str* and mem* functions */
#include <string.h>

/* *printf functions */
#include <stdio.h>

/* bcopy, bcmp, and bzero */
extern void bcopy(const void *src, void *dst, size_t len);
extern int bcmp(const void *b1, const void *b2, size_t len);
extern void bzero(void *b, size_t len);
#endif /* ! BCMDRIVER */

/* Current STB 7445D1 doesn't use ACP and it is non-coherrent.
 * Adding these dummy values for build apss only
 * When we revisit need to change these.
 */
#if defined(STBLINUX)

#if defined(__ARM_ARCH_7A__)
#define ACP_WAR_ENAB() 0
#define ACP_WIN_LIMIT 1
#define arch_is_coherent() 0
#endif /* __ARM_ARCH_7A__ */

#endif /* STBLINUX */

#ifdef CMWIFI
#define PRIO_LOC_NFMARK 16
extern struct pci_dev* osh_get_pdev(osl_t *osh);
uint osl_pktprio(void *skb);
void osl_pktsetprio(void *skb, uint x);
#endif // endif

#ifdef BCM_SECURE_DMA
#if defined BCMDBG
#define GLOBAL_CMA_ENABLE ((LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 89))	&&	\
	(LINUX_VERSION_CODE <= KERNEL_VERSION(4, 15, 18)))
#else
#define GLOBAL_CMA_ENABLE	0
#endif /* BCMDBG */

#define	SECURE_DMA_MAP(osh, va, size, direction, p, dmah, pcma, offset, buftype) \
	osl_secdma_map((osh), (va), (size), (direction), (p), (dmah), (pcma), (offset), (buftype))
#define	SECURE_DMA_DD_MAP(osh, va, size, direction, p, dmah) \
	osl_secdma_dd_map((osh), (va), (size), (direction), (p), (dmah))
#define	SECURE_DMA_MAP_TXMETA(osh, va, size, direction, p, dmah, pcma) \
	osl_secdma_map_txmeta((osh), (va), (size), (direction), (p), (dmah), (pcma))

#if defined(STS_XFER_PHYRXS)
#define	SECURE_DMA_MAP_STS_PHYRX(osh, size, align, tot, pap, dmah) \
	osl_secdma_map_sts_phyrx((osh), (size), (align), (pap))
#endif /* STS_XFER_PHYRXS */

#if defined(STS_XFER_TXS)
#define	SECURE_DMA_MAP_STS_XFER_TXS(osh, size, align, tot, pap, dmah) \
	osl_secdma_map_sts_xfer_txs((osh), (size), (align), (pap))
#endif /* STS_XFER_TXS */

#if defined(STS_XFER_PHYRXS) || defined(STS_XFER_TXS)
#define	SECURE_DMA_UNMAP_STS_XFER(osh, va, size, pa, dmah) \
	osl_secdma_unmap_sts_xfer((osh), (void*)(va), (size), (pa))
#endif /* STS_XFER_PHYRXS || STS_XFER_TXS */

#define	SECURE_DMA_UNMAP(osh, pa, size, direction, p, dmah, pcma, offset) \
	osl_secdma_unmap((osh), (pa), (size), (direction), (p), (dmah), (pcma), (offset))
#define	SECURE_DMA_UNMAP_ALL(osh, pcma) \
	osl_secdma_unmap_all((osh), (pcma))

#define DMA_MAP(osh, va, size, direction, p, dmah)

#define	SECURE_DMA_BUFFS_IS_AVAIL(osh) \
	osl_secdma_buffs_is_avail((osh))

#define	SECURE_DMA_RX_BUFFS_IS_AVAIL(osh) \
	osl_secdma_rx_buffs_is_avail((osh))
#ifdef BCMDONGLEHOST
#define	SECURE_DMA_RXCTL_BUFFS_IS_AVAIL(osh) \
	osl_secdma_rxctl_buffs_is_avail((osh))
#endif /* BCMDONGLEHOST */
typedef struct sec_cma_info {
	struct sec_mem_elem *sec_alloc_list;
	struct sec_mem_elem *sec_alloc_list_tail;
} sec_cma_info_t;

#define MB_1     1048576
#define KB_16    16384
#define KB_8     8192
#define KB_4     4096
#define KB_2     2048
#define KB_1     1024
#define B_512    512

/*
 * Total SECDMA memory Reserved is 20M.
 * This secdma memory will be used by both DMA data memory and DMA descriptor memory
 * Data buffers are of 2K each, divided in to 3 pools as below.
 * RXBUF POST Pool
 * RXCTR_BUF_POST POST Pool = RXCTR_BUF_POST  8K Buffers = 64
 * TXBUF Pool (CMA_TXBUF_BUFNUM)
 */

#define SECDMA_DATA_BUF_SIZE KB_2

#define SECDMA_RXBUF_POST	0
#define SECDMA_TXBUF_POST	1
#ifdef BCMDONGLEHOST
#define SECDMA_RXCTR_BUF_POST	2
#endif /* BCMDONGLEHOST */

#if defined(DHD_DEBUG)
#define	SECDMA_RXCTRL_BUF_SIZE	KB_16 /* align with DHD_FLOWRING_IOCTL_BUFPOST_PKTSZ */
#else
#define	SECDMA_RXCTRL_BUF_SIZE	KB_8
#endif /* DHD_DEBUG */

#ifdef BCMDONGLEHOST
/* RXBUF for FD */
/* != H2DRING_RXPOST_MAX_ITEM(bcmmsgbuf.h), found empirically */
#define SECDMA_RXBUF_CNT	2048
#define	SECDMA_RXCTRL_BUF_CNT	64	/* 8K buffer count(pool) for RXCTRL Buffers */
#else
/* RXBUF for NIC */
#define SECDMA_RXBUF_CNT	500	/* NRXBUFPOST(stbap: WLTUNEFILE=wltunable_lx_router.h) */
#define	SECDMA_RXCTRL_BUF_CNT	0	/* RXCTRL Buffers are used only in FD */
#endif /* BCMDONGLEHOST */
/*
 * Given below is an hint of the origin of Descriptor requests:
 * Max txflows(MAX_DHD_TX_FLOWS)	= 320(64 STA)
 * h2dctrl				= 1
 * d2hctrl				= 1
 * h2drxp				= 4 (with 43684, it can grow up to 4)
 * d2htxctrl				= 1
 * d2hrxcpl				= 4 (with 43684, it can be upto 4)
 * dma read/write indices allocations	= 5
 * IOCTL response buffer		= 1
 * IOCTL request buffer			= 1
 * host_bus_throughput_buf		= 1
 * Total				= 340
 */
#define SECDMA_MEMBLOCK_SIZE		(20 * MB_1)
#define SECDMA_DESC_MEMBLOCK_SIZE	(6 * MB_1) /* empirically derived */

#if defined(STS_XFER_PHYRXS)
#ifndef PHYRX_STATUS_RING_DEPTH
#ifdef STBAP
#define PHYRX_STATUS_RING_DEPTH		2048
#else
#define PHYRX_STATUS_RING_DEPTH		512
#endif /* ! STBAP */
#endif /* ! PHYRX_STATUS_RING_DEPTH */

// d11phyrxsts_t x PhyRx Status Ring depth
#define SECDMA_STS_PHYRX_MEMBLOCK_SIZE		\
	(PHYRX_STATUS_BYTES * PHYRX_STATUS_RING_DEPTH)
#else /* ! STS_XFER_PHYRXS */
#define SECDMA_STS_PHYRX_MEMBLOCK_SIZE		0
#endif /* ! STS_XFER_PHYRXS */

#if defined(STS_XFER_TXS)
#ifndef TX_STATUS_MACTXS_RING_DEPTH
#ifdef STBAP
#define TX_STATUS_MACTXS_RING_DEPTH	1024
#else
#define TX_STATUS_MACTXS_RING_DEPTH	512
#endif /* ! STBAP */
#endif /* ! TX_STATUS_MACTXS_RING_DEPTH */

// No of packages/TxStatus unit x TxStatus Ring depth
#define SECDMA_STS_XFER_TXS_MEMBLOCK_SIZE	\
	(TX_STATUS_MACTXS_BYTES * TX_STATUS_MACTXS_RING_DEPTH)
#else /* ! STS_XFER_TXS */
#define SECDMA_STS_XFER_TXS_MEMBLOCK_SIZE	0
#endif /* ! STS_XFER_TXS */

#define SECDMA_NONDESC_MEMBLOCK_SIZE	((SECDMA_MEMBLOCK_SIZE) - (SECDMA_DESC_MEMBLOCK_SIZE))
#define SECDMA_RXCTRL_MEMBLOCK_SIZE	((SECDMA_RXCTRL_BUF_CNT) * (SECDMA_RXCTRL_BUF_SIZE))
#define SECDMA_DATA_MEMBLOCK_SIZE	((SECDMA_NONDESC_MEMBLOCK_SIZE) - \
	((SECDMA_RXCTRL_MEMBLOCK_SIZE) + (SECDMA_STS_PHYRX_MEMBLOCK_SIZE) + \
	(SECDMA_STS_XFER_TXS_MEMBLOCK_SIZE)))

#define SECDMA_DATA_BUF_CNT		((SECDMA_DATA_MEMBLOCK_SIZE) / (SECDMA_DATA_BUF_SIZE))
#define SECDMA_TXBUF_CNT		(SECDMA_DATA_BUF_CNT - SECDMA_RXBUF_CNT)
#define SECDMA_TXBUF_MEMBLOCK_SIZE      ((SECDMA_TXBUF_CNT) * (SECDMA_DATA_BUF_SIZE))
#define SECDMA_RXBUF_MEMBLOCK_SIZE      ((SECDMA_DATA_BUF_SIZE)*(SECDMA_RXBUF_CNT))

/* Keep every Desc addr aligned */
#define SECDMA_DESC_ADDR_ALIGN	(32)

/* Keep physts 8 byte aligned */
#define SECDMA_PHYRXSTS_ALIGN	(8)

typedef struct sec_mem_elem {
	size_t			size;
	int			direction;
	int			buftype;
	phys_addr_t		pa;		/**< physical  address */
	void			*pkt;		/**< virtual address of driver pkt */
	dma_addr_t		dma_handle;	/**< bus address assign by linux */
	void			*va;		/**< virtual address of cma buffer */
	struct page		*pa_page;	/* phys to page address */
	struct	sec_mem_elem	*next;
} sec_mem_elem_t;

extern bool osl_secdma_buffs_is_avail(osl_t *osh);
extern bool osl_secdma_rx_buffs_is_avail(osl_t *osh);
#ifdef BCMDONGLEHOST
extern bool osl_secdma_rxctl_buffs_is_avail(osl_t *osh);
#endif /* BCMDONGLEHOST */

extern dmaaddr_t osl_secdma_map(osl_t *osh, void *va, uint size, int direction, void *p,
	hnddma_seg_map_t *dmah, void *ptr_cma_info, uint offset, uint buftype);
extern dma_addr_t osl_secdma_dd_map(osl_t *osh, void *va, uint size, int direction, void *p,
	hnddma_seg_map_t *dmah);
extern dma_addr_t osl_secdma_map_txmeta(osl_t *osh, void *va, uint size,
  int direction, void *p, hnddma_seg_map_t *dmah, void *ptr_cma_info);
extern void osl_secdma_unmap(osl_t *osh, dmaaddr_t pa, uint size, int direction,
	void *p, hnddma_seg_map_t *map, void *ptr_cma_info, uint offset);

#if defined(STS_XFER_PHYRXS)
extern void *osl_secdma_map_sts_phyrx(osl_t *osh, uint size, uint16 align, dmaaddr_t *pap);
#endif /* STS_XFER_PHYRXS */
#if defined(STS_XFER_TXS)
extern void *osl_secdma_map_sts_xfer_txs(osl_t *osh, uint size, uint16 align, dmaaddr_t *pap);
#endif /* STS_XFER_STS */
#if defined(STS_XFER_PHYRXS) || defined(STS_XFER_TXS)
extern void osl_secdma_unmap_sts_xfer(osl_t *osh, void *va, uint size, dmaaddr_t pa);
#endif /* STS_XFER_PHYRXS || STS_XFER_TXS */
extern void osl_secdma_unmap_all(osl_t *osh, void *ptr_cma_info);
#endif /* BCM_SECURE_DMA */

#if defined(BCM_NBUFF)
/*
 *  NBUFF (fkb) type packet does not have prev or next pointers,
 *  and can't be made into double linked list.
 *  We add prev and next pointers in the dhd_pkttag_fd
 *  to help make double linked list for packets
 */
typedef struct dll PKT_LIST;
#define PKTLIST_INIT(x)		dhd_pkt_queue_head_init((x))
#define PKTLIST_ENQ(x, y)	dhd_pkt_queue_head((PKT_LIST *)(x), (void *)(y))
#define PKTLIST_DEQ(x)		dhd_pkt_dequeue((PKT_LIST *)(x))
#define PKTLIST_UNLINK(x, y)	dhd_pkt_unlink((PKT_LIST *)(x), (void *)(y))
#define PKTLIST_FINI(osh, x)	dhd_pkt_queue_purge((osl_t *)(osh), (PKT_LIST *)(x))

#else /* !BCM_NBUFF */

typedef struct sk_buff_head PKT_LIST;
extern void osl_pkt_queue_purge(osl_t *osh, struct sk_buff_head * list);
#define PKTLIST_INIT(x)		skb_queue_head_init((x))
#define PKTLIST_ENQ(x, y)	skb_queue_head((struct sk_buff_head *)(x), (struct sk_buff *)(y))
#define PKTLIST_DEQ(x)		skb_dequeue((struct sk_buff_head *)(x))
#define PKTLIST_UNLINK(x, y)	skb_unlink((struct sk_buff *)(y), (struct sk_buff_head *)(x))
#if defined(CMWIFI)
#define PKTLIST_FINI(osh, x)	osl_pkt_queue_purge((osl_t *)(osh), (struct sk_buff_head *)(x))
#else
#define PKTLIST_FINI(osh, x)	({BCM_REFERENCE(osh); skb_queue_purge((struct sk_buff_head *)(x));})
#endif /* CMWIFI */
#endif /* BCM_NBUFF */

#ifdef BCM_ROUTER
extern void osl_adjust_mac(unsigned int instance_id,char *mac); /* for NIC */
extern int osl_nvram_vars_adjust_mac(unsigned int instance_id, char *memblock, uint* len);
#endif // endif

#ifdef BCM_SKB_FREE_OFFLOAD
#define	BCM_SKB_FREE_OFFLOAD_ENAB(osh)	OSH_PUB(osh).skb_free_offload
#else
#define	BCM_SKB_FREE_OFFLOAD_ENAB(osh)	(0)
#endif // endif

#define OSH_SETBME(osh, bmeh)   ((osl_pubinfo_t*)osh)->bme = (bmeh)
#define OSH_GETBME(osh)	        ((osl_pubinfo_t*)osh)->bme

#define OSH_SETM2M(osh, m2mh)   ((osl_pubinfo_t*)osh)->m2m = (m2mh)
#define OSH_GETM2M(osh)	        ((osl_pubinfo_t*)osh)->m2m

#define OSH_SET_PCNPKTPOOL(osh, pool)	(((osl_pubinfo_t*)(osh))->pcn_pktpool = (pool))
#define OSH_GET_PCNPKTPOOL(osh)		(((osl_pubinfo_t*)(osh))->pcn_pktpool)

#define OSL_DEV_TO_IFID_INVALID			       (0xF)
#ifdef BCM_ROUTER
#define OSL_GET_DEV_TO_IFID_MAP(dev, ifid_map)	       \
		((ifid_map >> (pci_domain_nr(dev->bus) * 4)) & 0xF)
#define OSL_SAVE_DEV_TO_IFID_MAP(dev, ifid_map, ifid)   \
		(ifid_map = (ifid_map & ~(0xF << (pci_domain_nr(dev->bus) * 4))) | \
		 (ifid << (pci_domain_nr(dev->bus) * 4)))
#else /* !BCM_ROUTER */
#define OSL_GET_DEV_TO_IFID_MAP(dev, ifid_map)	        OSL_DEV_TO_IFID_INVALID
#define OSL_SAVE_DEV_TO_IFID_MAP(dev, ifid_map, ifid)   do { } while (0)
#endif /* !BCM_ROUTER */

extern void *osl_spin_lock_init(osl_t *osh);
extern void osl_spin_lock_deinit(osl_t *osh, void *lock);
extern unsigned long osl_spin_lock(void *lock);
extern void osl_spin_unlock(void *lock, unsigned long flags);
extern unsigned long osl_spin_lock_irq(void *lock);
extern void osl_spin_unlock_irq(void *lock, unsigned long flags);
extern unsigned long osl_spin_lock_bh(void *lock);
extern void osl_spin_unlock_bh(void *lock, unsigned long flags);

extern void osl_dev_hold(void *ndev);
extern void osl_dev_put(void *ndev);

#if defined(CMWIFI) && defined(__KERNEL__)
#include <linux/dma-direct.h>
/* map addr from device dma range to allocator range */
extern dma_addr_t osl_dma_to_phys(osl_t *osh, dma_addr_t pa);
/* map addr from allocator device to device dma range */
extern dma_addr_t osl_phys_to_dma(osl_t *osh, dma_addr_t pa);
#endif /* CMWIFI && __KERNEL__ */

#endif	/* _linux_osl_h_ */
