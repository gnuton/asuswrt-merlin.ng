/*
 * Misc utility routines for accessing chip-specific features
 * of the SiliconBackplane-based Broadcom chips.
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
 * $Id: aiutils.c 828071 2023-08-01 08:45:40Z $
 */
#include <bcm_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <hndsoc.h>
#include <sbchipc.h>
#include <pcicfg.h>
#ifdef HNDGIC
#include <hndgic.h>
#endif // endif

#include "siutils_priv.h"
#include <bcmdevs.h>

#if !defined(BCMDONGLEHOST)

#define PMU_DMP()  (cores_info->coreid[sii->curidx] == PMU_CORE_ID)
#define GCI_DMP()  (cores_info->coreid[sii->curidx] == GCI_CORE_ID)
#define HWA_DMP()  (cores_info->coreid[sii->curidx] == HWA_CORE_ID)
#else
#define PMU_DMP() (0)
#define GCI_DMP() (0)
#define HWA_DMP() (0)
#endif /* !defined(BCMDONGLEHOST) */

#if defined(BCM_BACKPLANE_TIMEOUT)
static bool ai_get_apb_bridge(si_t *sih, uint32 coreidx, uint32 *apb_id, uint32 *apb_coreuinit);
#endif /* BCM_BACKPLANE_TIMEOUT */

/* EROM parsing */

static uint32
get_erom_ent(si_t *sih, uint32 **eromptr, uint32 mask, uint32 match)
{
	uint32 ent;
	uint inv = 0, nom = 0;
	uint32 size = 0;

	while (TRUE) {
		ent = R_REG(si_osh(sih), *eromptr);
		(*eromptr)++;

		if (mask == 0)
			break;

		if ((ent & ER_VALID) == 0) {
			inv++;
			continue;
		}

		if (ent == (ER_END | ER_VALID))
			break;

		if ((ent & mask) == match)
			break;

		/* escape condition related EROM size if it has invalid values */
		size += sizeof(*eromptr);
		if (size >= ER_SZ_MAX) {
			SI_ERROR(("Failed to find end of EROM marker\n"));
			break;
		}

		nom++;
	}

	SI_VMSG(("%s: Returning ent 0x%08x\n", __FUNCTION__, ent));
	if (inv + nom) {
		SI_VMSG(("  after %d invalid and %d non-matching entries\n", inv, nom));
	}
	return ent;
}

static uint32
get_asd(si_t *sih, uint32 **eromptr, uint sp, uint ad, uint st, uint32 *addrl_pa, uint32 *addrh_pa,
	uint32 *sizel, uint32 *sizeh)
{
	uint32 asd, sz, szd;

	BCM_REFERENCE(ad);

	asd = get_erom_ent(sih, eromptr, ER_VALID, ER_VALID);
	if (((asd & ER_TAG1) != ER_ADD) ||
	    (((asd & AD_SP_MASK) >> AD_SP_SHIFT) != sp) ||
	    ((asd & AD_ST_MASK) != st)) {
		/* This is not what we want, "push" it back */
		(*eromptr)--;
		return 0;
	}
	*addrl_pa = asd & AD_ADDR_MASK;
	if (asd & AD_AG32)
		*addrh_pa = get_erom_ent(sih, eromptr, 0, 0);
	else
		*addrh_pa = 0;
	*sizeh = 0;
	sz = asd & AD_SZ_MASK;
	if (sz == AD_SZ_SZD) {
		szd = get_erom_ent(sih, eromptr, 0, 0);
		*sizel = szd & SD_SZ_MASK;
		if (szd & SD_SG32)
			*sizeh = get_erom_ent(sih, eromptr, 0, 0);
	} else
		*sizel = AD_SZ_BASE << (sz >> AD_SZ_SHIFT);

	SI_VMSG(("  SP %d, ad %d: st = %d, 0x%08x_0x%08x @ 0x%08x_0x%08x\n",
	        sp, ad, st, *sizeh, *sizel, *addrh_pa, *addrl_pa));

	return asd;
}

/**
 * Called for the 63178 and 47622, these chips contain a hard WLAN macro with fixed address
 * decoding logic, the CPU needs to 'see' each WLAN core on a different address, to accomplish this
 * the UBUS performs address translation in the host_CPU->WLAN_core direction. The EROM in the WLAN
 * cores are identical, because the WLAN cores are instantiations of the same hard macro. So the
 * EROM addresses, after being read, need to be corrected for the particular core instance.
 *
 * @return    The physical address that the CPU can use to address a specific WLAN core
 */
static uint32
axiaddr_to_cpuaddr(si_info_t *sii, uint32 axiaddr_pa)
{
	uint32 cpuaddr_pa = axiaddr_pa;

	ASSERT(EMBEDDED_WLAN_CORE(sii->pub.chip));
	if (cpuaddr_pa != 0) { /* 0 means 'nothing here' */
		cpuaddr_pa -= EMBEDDED_11BE_CORE(sii->pub.chip) ?
			SI_ENUM_BASE_BP_POV_2x2BE_PA: /* == 0x9000_0000 */
			SI_ENUM_BASE_BP_POV_2x2AX_PA; /* == 0x8400_0000 */
		cpuaddr_pa += sii->pub.enum_base_pa; /* adds phys addr in pseudo PCIe BAR0 reg */
	}

	return cpuaddr_pa;
}

/** function description: see comment for function axiaddr_to_cpuaddr() */
static void
ai_scan_uniq_cpu_addresses(si_info_t *sii, si_cores_info_t *cores_info, axi_wrapper_t *axi_wrapper)
{
	uint32 i, j;

	ASSERT(EMBEDDED_WLAN_CORE(sii->pub.chip));
	sii->oob_router_pa = axiaddr_to_cpuaddr(sii, sii->oob_router_pa);
	for (i = 0; i < sii->numcores; i++) {
		cores_info->coresba_pa[i] = axiaddr_to_cpuaddr(sii, cores_info->coresba_pa[i]);
		cores_info->coresba2_pa[i] = axiaddr_to_cpuaddr(sii, cores_info->coresba2_pa[i]);
		cores_info->csp2ba_pa[i] = axiaddr_to_cpuaddr(sii, cores_info->csp2ba_pa[i]);
		for (j = 0; j < AI_MAX_WRAPPERS_PERCORE; j++) {
			cores_info->wrapba_pa[i][j] = axiaddr_to_cpuaddr(sii,
				cores_info->wrapba_pa[i][j]);
		}
	}
	for (i = 0; i < sii->axi_num_wrappers; i++) {
		axi_wrapper[i].wrapper_pa = axiaddr_to_cpuaddr(sii, axi_wrapper[i].wrapper_pa);
	}
	for (i = 0; i < sii->num_br; i++) {
		sii->br_wrapba_pa[i] = axiaddr_to_cpuaddr(sii, sii->br_wrapba_pa[i]);
	}
}

#ifdef HNDGIC
#ifdef __ARM_ARCH_7A__
/* Read ARM configuration to update gicd_spisr
 * Required for chip 6726 which does not have GIC
 * core but uses HNDGIC to work with more than 8 isr
 */
static void
ai_scan_set_gicd_spisr_arm(si_info_t *sii)
{
	uint32 periphbase;
	asm volatile("mrc p15,4,%0,c15,c0,0 @ Read Configuration Base Address Register"
		: "=&r" (periphbase) : : "cc");
	sii->gicd_spisr = periphbase;
	sii->gicd_spisr += MPCORE_GIC_DIST_OFF + GIC_DIST_SPISR0;

	return;
}
#endif /* __ARM_ARCH_7A__ */

static void
ai_scan_set_gicd_spisr(si_info_t *sii, si_cores_info_t *cores_info)
{
	uint32 i;

#ifdef __ARM_ARCH_7A__
	if (BCM6726_CHIP(sih->chip)) {
		/* 6726 does not use external GIC.
		 * we should read arm coprocessor to update gicd_spisr
		 */
		ai_scan_set_gicd_spisr_arm(sii);
		return;
	}
#endif /* __ARM_ARCH_7A__ */
	for (i = 0; i < sii->numcores; i++) {
		if (cores_info->coreid[i] == GIC400_CORE_ID) {
			sii->gicd_spisr = cores_info->coresba_pa[i];
			sii->gicd_spisr += MPCORE_GIC_DIST_OFF + GIC_DIST_SPISR0;
			break;
		}
	}
}
#endif /* HNDGIC */

/* Parse the enumeration rom to identify all cores
 * Erom content format can be found in:
 * http://hwnbu-twiki.broadcom.com/twiki/pub/Mwgroup/ArmDocumentation/SystemDiscovery.pdf
 */
void
BCMATTACHFN(ai_scan)(si_t *sih, void *regs, uint devid)
{
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
	chipcregs_t *cc = (chipcregs_t *)regs;
	uint32 erombase_pa; /** physical address */
	uint32 *eromptr, *eromlim;
	axi_wrapper_t * axi_wrapper = sii->axi_wrapper;

	BCM_REFERENCE(devid);

	erombase_pa = R_REG(sii->osh, &cc->eromptr);

	switch (BUSTYPE(sih->bustype)) {
	case SI_BUS:
		if (EMBEDDED_WLAN_CORE(sih->chip)) {
			/* linear large addres space, directly accessible; no need for new map */
			erombase_pa = axiaddr_to_cpuaddr(sii, erombase_pa);
			eromptr = (uint32*) (sih->enum_base_va +
			                     (erombase_pa - SI_ENUM_BASE_PA(sih)));
		} else {
			eromptr = (uint32 *)REG_MAP(erombase_pa, SI_CORE_SIZE);
		}
		break;

	case PCI_BUS:
		/* Set wrappers address */
		sii->curwrap = (void *)((uintptr)regs + SI_CORE_SIZE);

		/* Now point the window at the erom */
		OSL_PCI_WRITE_CONFIG(sii->osh, PCI_BAR0_WIN, 4, erombase_pa);
		eromptr = regs;
		break;

#ifdef BCMJTAG
	case JTAG_BUS:
		eromptr = (uint32 *)(uintptr)erombase_pa;
		break;
#endif	/* BCMJTAG */

	default:
		SI_ERROR(("Don't know how to do AXI enumertion on bus %d\n", sih->bustype));
		ASSERT(0);
		return;
	}

	eromlim = eromptr + (ER_REMAPCONTROL / sizeof(uint32));
	sii->axi_num_wrappers = 0;

	SI_VMSG(("ai_scan: regs = 0x%p, erombase_pa = 0x%08x, eromptr = 0x%p, eromlim = 0x%p\n",
	         OSL_OBFUSCATE_BUF(regs), erombase_pa,
	         OSL_OBFUSCATE_BUF(eromptr), OSL_OBFUSCATE_BUF(eromlim)));

	while (eromptr < eromlim) {
		uint32 cia, cib, cid, mfg, crev, nmw, nsw, nmp, nsp;
		uint32 mpd, asd, addrl_pa, addrh_pa, sizel, sizeh;
		uint i, j, idx;
		bool br;

		br = FALSE;

		/* Grok a component */
		cia = get_erom_ent(sih, &eromptr, ER_TAG, ER_CI);
		if (cia == (ER_END | ER_VALID)) {
			SI_VMSG(("Found END of erom after %d cores\n", sii->numcores));
			goto done;
		}

		cib = get_erom_ent(sih, &eromptr, 0, 0);

		if ((cib & ER_TAG) != ER_CI) {
			SI_ERROR(("CIA not followed by CIB\n"));
			goto error;
		}

		cid = (cia & CIA_CID_MASK) >> CIA_CID_SHIFT;
		mfg = (cia & CIA_MFG_MASK) >> CIA_MFG_SHIFT;
		crev = (cib & CIB_REV_MASK) >> CIB_REV_SHIFT;
		nmw = (cib & CIB_NMW_MASK) >> CIB_NMW_SHIFT;
		nsw = (cib & CIB_NSW_MASK) >> CIB_NSW_SHIFT;
		nmp = (cib & CIB_NMP_MASK) >> CIB_NMP_SHIFT;
		nsp = (cib & CIB_NSP_MASK) >> CIB_NSP_SHIFT;

		SI_VMSG(("Found component 0x%04x/0x%04x rev %d at erom addr 0x%p, with nmw = %d, "
		         "nsw = %d, nmp = %d & nsp = %d\n",
		         mfg, cid, crev, OSL_OBFUSCATE_BUF(eromptr - 1), nmw, nsw, nmp, nsp));

		/* Include Default slave wrapper for timeout monitoring */
		if ((nsp == 0) ||
#if !defined(AXI_TIMEOUTS) && !defined(BCM_BACKPLANE_TIMEOUT)
			((mfg == MFGID_ARM) && (cid == DEF_AI_COMP)) ||
#else
			((CHIPTYPE(sii->pub.socitype) == SOCI_NAI) &&
			(mfg == MFGID_ARM) && (cid == DEF_AI_COMP)) ||
#endif /* !defined(AXI_TIMEOUTS) && !defined(BCM_BACKPLANE_TIMEOUT) */
			FALSE) {
			continue;
		}

		if ((nmw + nsw == 0)) {
			/* A component which is not a core */
			if (cid == OOB_ROUTER_CORE_ID) {
				asd = get_asd(sih, &eromptr, 0, 0, AD_ST_SLAVE,
					&addrl_pa, &addrh_pa, &sizel, &sizeh);
				if (asd != 0) {
					sii->oob_router_pa = addrl_pa;
				}
			}
			if (cid != NS_CCB_CORE_ID &&
				cid != PMU_CORE_ID && cid != GCI_CORE_ID && cid != SR_CORE_ID &&
				cid != HUB_CORE_ID && cid != AVS_CORE_ID)
				continue;
		}

		idx = sii->numcores;

		cores_info->cia[idx] = cia;
		cores_info->cib[idx] = cib;
		cores_info->coreid[idx] = cid;

		for (i = 0; i < nmp; i++) {
			mpd = get_erom_ent(sih, &eromptr, ER_VALID, ER_VALID);
			if ((mpd & ER_TAG) != ER_MP) {
				SI_ERROR(("Not enough MP entries for component 0x%x\n", cid));
				goto error;
			}
			SI_VMSG(("  Master port %d, mp: %d id: %d\n", i,
			         (mpd & MPD_MP_MASK) >> MPD_MP_SHIFT,
			         (mpd & MPD_MUI_MASK) >> MPD_MUI_SHIFT));
		}

		/* First Slave Address Descriptor should be port 0:
		 * the main register space for the core
		 */
		asd = get_asd(sih, &eromptr, 0, 0, AD_ST_SLAVE, &addrl_pa, &addrh_pa,
		              &sizel, &sizeh);
		if (asd == 0) {
			do {
			/* Try again to see if it is a bridge */
			asd = get_asd(sih, &eromptr, 0, 0, AD_ST_BRIDGE, &addrl_pa, &addrh_pa,
			              &sizel, &sizeh);
			if (asd != 0)
				br = TRUE;
			else {
					if (br == TRUE) {
						break;
					}
					else if ((addrh_pa != 0) || (sizeh != 0) ||
						(sizel != SI_CORE_SIZE)) {
						SI_ERROR(("addrh_pa = 0x%x\t sizeh = 0x%x\t size1 ="
							"0x%x\n", addrh_pa, sizeh, sizel));
						SI_ERROR(("First Slave ASD for"
							"core 0x%04x malformed "
							"(0x%08x)\n", cid, asd));
						goto error;
					}
				}
			} while (1);
		}
		cores_info->coresba_pa[idx] = addrl_pa;
		cores_info->coresba_size[idx] = sizel;
		/* Get any more ASDs in first port */
		j = 1;
		do {
			asd = get_asd(sih, &eromptr, 0, j, AD_ST_SLAVE, &addrl_pa, &addrh_pa,
			              &sizel, &sizeh);
			if ((asd != 0) && (j == 1) && (sizel == SI_CORE_SIZE)) {
				cores_info->coresba2_pa[idx] = addrl_pa;
				cores_info->coresba2_size[idx] = sizel;
			}
			j++;
		} while (asd != 0);

		/* Go through the ASDs for other slave ports */
		for (i = 1; i < nsp; i++) {
			j = 0;
			do {
				asd = get_asd(sih, &eromptr, i, j, AD_ST_SLAVE,
				              &addrl_pa, &addrh_pa, &sizel, &sizeh);
				/* To get the first base address of second slave port */
				if ((asd != 0) && (i == 1) && (j == 0)) {
					cores_info->csp2ba_pa[idx] = addrl_pa;
					cores_info->csp2ba_size[idx] = sizel;
				}
				if (asd == 0)
					break;
				j++;
			} while (1);
			if (j == 0) {
				SI_ERROR((" SP %d has no address descriptors\n", i));
				goto error;
			}
		}

		/* Now get master wrappers */
		for (i = 0; i < nmw; i++) {
			asd = get_asd(sih, &eromptr, i, 0, AD_ST_MWRAP, &addrl_pa, &addrh_pa,
			              &sizel, &sizeh);
			if (asd == 0) {
				SI_ERROR(("Missing descriptor for MW %d\n", i));
				goto error;
			}
			if ((sizeh != 0) || (sizel != SI_CORE_SIZE)) {
				SI_ERROR(("Master wrapper %d is not 4KB\n", i));
				goto error;
			}

			ASSERT(i < AI_MAX_WRAPPERS_PERCORE);
			cores_info->wrapba_pa[idx][i] = addrl_pa;

			if (axi_wrapper &&
				(sii->axi_num_wrappers < SI_MAX_AXI_WRAPPERS)) {
				axi_wrapper[sii->axi_num_wrappers].mfg = mfg;
				axi_wrapper[sii->axi_num_wrappers].cid = cid;
				axi_wrapper[sii->axi_num_wrappers].rev = crev;
				axi_wrapper[sii->axi_num_wrappers].wrapper_type = AI_MASTER_WRAPPER;
				axi_wrapper[sii->axi_num_wrappers].wrapper_pa = addrl_pa;
				sii->axi_num_wrappers++;
				SI_VMSG(("MASTER WRAPPER: %d, mfg:%x, cid:%x,"
					"rev:%x, addr:%x, size:%x\n",
					sii->axi_num_wrappers, mfg, cid, crev, addrl_pa, sizel));
			}
		}

		/* And finally slave wrappers */
		for (i = 0; i < nsw; i++) {
			uint fwp = (nsp == 1) ? 0 : 1;
			asd = get_asd(sih, &eromptr, fwp + i, 0, AD_ST_SWRAP, &addrl_pa, &addrh_pa,
			              &sizel, &sizeh);

			/* cache APB bridge wrapper address for set/clear timeout */
			if ((mfg == MFGID_ARM) && (cid == APB_BRIDGE_ID)) {
				ASSERT(sii->num_br < SI_MAXBR);
				sii->br_wrapba_pa[sii->num_br++] = addrl_pa;
			}

			if (asd == 0) {
				SI_ERROR(("Missing descriptor for SW %d\n", i));
				goto error;
			}
			if ((sizeh != 0) || (sizel != SI_CORE_SIZE)) {
				SI_ERROR(("Slave wrapper %d is not 4KB\n", i));
				goto error;
			}
			if (nmw == 0) {
				ASSERT(i < AI_MAX_WRAPPERS_PERCORE);
				cores_info->wrapba_pa[idx][i] = addrl_pa;
			}

			/* Include all slave wrappers to the list to
			 * enable and monitor watchdog timeouts
			 */

			if (axi_wrapper &&
				(sii->axi_num_wrappers < SI_MAX_AXI_WRAPPERS)) {
				axi_wrapper[sii->axi_num_wrappers].mfg = mfg;
				axi_wrapper[sii->axi_num_wrappers].cid = cid;
				axi_wrapper[sii->axi_num_wrappers].rev = crev;
				axi_wrapper[sii->axi_num_wrappers].wrapper_type = AI_SLAVE_WRAPPER;
				axi_wrapper[sii->axi_num_wrappers].wrapper_pa = addrl_pa;
				sii->axi_num_wrappers++;

				SI_VMSG(("SLAVE WRAPPER: %d,  mfg:%x, cid:%x,"
					"rev:%x, addr:%x, size:%x\n",
					sii->axi_num_wrappers,  mfg, cid, crev, addrl_pa, sizel));
			}
		}

#ifndef BCM_BACKPLANE_TIMEOUT
		/* Don't record bridges */
		if (br)
			continue;
#endif // endif

		/* Done with core */
		sii->numcores++;
	}

	SI_ERROR(("Reached end of erom without finding END"));

error:
	sii->numcores = 0;
	return;

done:
	if (EMBEDDED_WLAN_CORE(sih->chip)) {
		/* These chips run on UBUS */
		ai_scan_uniq_cpu_addresses(sii, cores_info, axi_wrapper);
	}

#ifdef HNDGIC
	if (BCM6717_CHIP(sih->chip) || BCM6726_CHIP(sih->chip)) {
		ai_scan_set_gicd_spisr(sii, cores_info);
	}
#endif /* HNDGIC */
} /* ai_scan */

#define AI_SETCOREIDX_MAPSIZE(coreid) \
	(((coreid) == NS_CCB_CORE_ID) ? 15 * SI_CORE_SIZE : SI_CORE_SIZE)

/* This function changes the logical "focus" to the indicated core.
 * Return the current core's virtual address.
 */
static volatile void *
_ai_setcoreidx(si_t *sih, uint coreidx, uint wrapidx)
{
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
	uint32 addr_pa, wrapba_pa;
	volatile void *regs;

	if (coreidx >= MIN(sii->numcores, SI_MAXCORES))
		return (NULL);

	addr_pa = cores_info->coresba_pa[coreidx]; /* 0 means n/a: no core registers implemented */
	ASSERT(wrapidx < AI_MAX_WRAPPERS_PERCORE);
	wrapba_pa = cores_info->wrapba_pa[coreidx][wrapidx];

#ifndef DONGLEBUILD
#ifdef BCM_BACKPLANE_TIMEOUT
	/* No need to disable interrupts while entering/exiting APB bridge core */
	if ((cores_info->coreid[coreidx] != APB_BRIDGE_CORE_ID) &&
		(cores_info->coreid[sii->curidx] != APB_BRIDGE_CORE_ID))
#endif /* BCM_BACKPLANE_TIMEOUT */
	{
		/*
		 * If the user has provided an interrupt mask enabled function,
		 * then assert interrupts are disabled before switching the core.
		 */
		ASSERT((sii->intrsenabled_fn == NULL) ||
			!(*(sii)->intrsenabled_fn)((sii)->intr_arg));
	}
#endif /* DONGLEBUILD */

	switch (BUSTYPE(sih->bustype)) {
	case SI_BUS:
		if (!cores_info->regs[coreidx] && (addr_pa != 0)) {
			if (EMBEDDED_WLAN_CORE(sih->chip)) {
				/* linear large addres space, directly accessible */
				regs = (void*) (sih->enum_base_va +
				                (addr_pa - SI_ENUM_BASE_PA(sih)));
			} else {
				/* map new one */
				regs = REG_MAP(addr_pa,
					AI_SETCOREIDX_MAPSIZE(cores_info->coreid[coreidx]));
			}

			ASSERT(GOODREGS(regs));
			cores_info->regs[coreidx] = regs;
		}

		sii->curmap = regs = cores_info->regs[coreidx];
		if (!cores_info->wrappers[coreidx][wrapidx] && (wrapba_pa != 0)) {
			void *wrapptr;

			if (EMBEDDED_WLAN_CORE(sih->chip)) {
				/* linear large addres space, directly accessible */
				wrapptr = (void*) (sih->enum_base_va +
				                   (wrapba_pa - SI_ENUM_BASE_PA(sih)));
			} else {
				/* map new one */
				wrapptr = REG_MAP(wrapba_pa, SI_CORE_SIZE);
			}

			ASSERT(GOODREGS(wrapptr));
			cores_info->wrappers[coreidx][wrapidx] = wrapptr;
		}
		sii->curwrap = cores_info->wrappers[coreidx][wrapidx];
		break;

	case PCI_BUS:
#ifdef BCM_BACKPLANE_TIMEOUT
		/* No need to set the BAR0 if core is APB Bridge.
		 * This is to reduce 2 PCI writes while checkng for errlog
		 */
		if (cores_info->coreid[coreidx] != APB_BRIDGE_CORE_ID)
#endif /* BCM_BACKPLANE_TIMEOUT */
		{
			/* point bar0 window */
			OSL_PCI_WRITE_CONFIG(sii->osh, PCI_BAR0_WIN, 4, addr_pa);
		}

		regs = sii->curmap;
		/* point bar0 2nd 4KB window to the primary wrapper */
		if (PCIE_GEN2(sii))
			OSL_PCI_WRITE_CONFIG(sii->osh, PCIE2_BAR0_WIN2, 4, wrapba_pa);
		else
			OSL_PCI_WRITE_CONFIG(sii->osh, PCI_BAR0_WIN2, 4, wrapba_pa);
		break;

#ifdef BCMJTAG
	case JTAG_BUS:
		sii->curmap = regs = (void *)((uintptr)addr_pa);
		sii->curwrap = (void *)((uintptr)wrapba_pa);
		break;
#endif	/* BCMJTAG */

	default:
		ASSERT(0);
		regs = NULL;
		break;
	}

	sii->curmap = regs;
	sii->curidx = coreidx;

	return regs;
}

volatile void *
ai_setcoreidx(si_t *sih, uint coreidx)
{
	return _ai_setcoreidx(sih, coreidx, 0);
}

void
ai_coreaddrspaceX(si_t *sih, uint asidx, uint32 *addr, uint32 *size)
{
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
	chipcregs_t *cc = NULL;
	uint32 erombase_pa, *eromptr, *eromlim;
	uint i, j, cidx;
	uint32 cia, cib, nmp, nsp;
	uint32 asd, addrl_pa, addrh_pa, sizel, sizeh;

	for (i = 0; i < sii->numcores; i++) {
		if (cores_info->coreid[i] == CC_CORE_ID) {
			cc = (chipcregs_t *)cores_info->regs[i];
			break;
		}
	}
	if (cc == NULL)
		goto error;

	erombase_pa = R_REG(sii->osh, &cc->eromptr);
	if (EMBEDDED_WLAN_CORE(sih->chip)) {
		/* no need to map - linear large addres space, directly accessible */
		erombase_pa = axiaddr_to_cpuaddr(sii, erombase_pa);
		eromptr = (uint32*) (sih->enum_base_va + (erombase_pa - SI_ENUM_BASE_PA(sih)));
	} else {
		eromptr = (uint32 *)REG_MAP(erombase_pa, SI_CORE_SIZE);
	}
	eromlim = eromptr + (ER_REMAPCONTROL / sizeof(uint32));

	cidx = sii->curidx;
	cia = cores_info->cia[cidx];
	cib = cores_info->cib[cidx];

	nmp = (cib & CIB_NMP_MASK) >> CIB_NMP_SHIFT;
	nsp = (cib & CIB_NSP_MASK) >> CIB_NSP_SHIFT;

	/* scan for cores */
	while (eromptr < eromlim) {
		if ((get_erom_ent(sih, &eromptr, ER_TAG, ER_CI) == cia) &&
			(get_erom_ent(sih, &eromptr, 0, 0) == cib)) {
			break;
		}
	}

	/* skip master ports */
	for (i = 0; i < nmp; i++)
		get_erom_ent(sih, &eromptr, ER_VALID, ER_VALID);

	/* Skip ASDs in port 0 */
	asd = get_asd(sih, &eromptr, 0, 0, AD_ST_SLAVE, &addrl_pa, &addrh_pa, &sizel, &sizeh);
	if (asd == 0) {
		/* Try again to see if it is a bridge */
		asd = get_asd(sih, &eromptr, 0, 0, AD_ST_BRIDGE, &addrl_pa, &addrh_pa,
		              &sizel, &sizeh);
	}

	j = 1;
	do {
		asd = get_asd(sih, &eromptr, 0, j, AD_ST_SLAVE, &addrl_pa, &addrh_pa,
		              &sizel, &sizeh);
		j++;
	} while (asd != 0);

	/* Go through the ASDs for other slave ports */
	for (i = 1; i < nsp; i++) {
		j = 0;
		do {
			asd = get_asd(sih, &eromptr, i, j, AD_ST_SLAVE, &addrl_pa, &addrh_pa,
				&sizel, &sizeh);
			if (asd == 0)
				break;

			if (!asidx--) {
				*addr = addrl_pa;
				*size = sizel;
				return;
			}
			j++;
		} while (1);

		if (j == 0) {
			SI_ERROR((" SP %d has no address descriptors\n", i));
			break;
		}
	}

error:
	*size = 0;
	return;
}

/* Return the number of address spaces in current core */
int
ai_numaddrspaces(si_t *sih)
{

	BCM_REFERENCE(sih);

	return 2;
}

/**
 * Return the physical address of the nth address space in the current core
 * Arguments:
 * @param sih    Pointer to struct si_t
 * @param spidx  Slave port index
 * @param baidx  Base address index
 */
uint32
ai_addrspace(si_t *sih, uint spidx, uint baidx)
{
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
	uint cidx;

	cidx = sii->curidx;

	if (spidx == CORE_SLAVE_PORT_0) {
		if (baidx == CORE_BASE_ADDR_0)
			return cores_info->coresba_pa[cidx];
		else if (baidx == CORE_BASE_ADDR_1)
			return cores_info->coresba2_pa[cidx];
	}
	else if (spidx == CORE_SLAVE_PORT_1) {
		if (baidx == CORE_BASE_ADDR_0)
			return cores_info->csp2ba_pa[cidx];
	}

	SI_ERROR(("%s: Need to parse the erom again to find %d base addr in %d slave port\n",
	      __FUNCTION__, baidx, spidx));

	return 0;

}

/* Return the size of the nth address space in the current core
* Arguments:
* sih : Pointer to struct si_t
* spidx : slave port index
* baidx : base address index
*/
uint32
ai_addrspacesize(si_t *sih, uint spidx, uint baidx)
{
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
	uint cidx;

	cidx = sii->curidx;
	if (spidx == CORE_SLAVE_PORT_0) {
		if (baidx == CORE_BASE_ADDR_0)
			return cores_info->coresba_size[cidx];
		else if (baidx == CORE_BASE_ADDR_1)
			return cores_info->coresba2_size[cidx];
	}
	else if (spidx == CORE_SLAVE_PORT_1) {
		if (baidx == CORE_BASE_ADDR_0)
			return cores_info->csp2ba_size[cidx];
	}

	SI_ERROR(("%s: Need to parse the erom again to find %d base addr in %d slave port\n",
	      __FUNCTION__, baidx, spidx));

	return 0;
}

uint
ai_flag(si_t *sih)
{
	si_info_t *sii = SI_INFO(sih);
#if !defined(BCMDONGLEHOST)
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
#endif // endif
	aidmp_t *ai;

	if (PMU_DMP()) {
		uint idx, flag;
		idx = sii->curidx;
		ai_setcoreidx(sih, SI_CC_IDX);
		flag = ai_flag_alt(sih);
		ai_setcoreidx(sih, idx);
		return flag;
	}

	ai = sii->curwrap;
	ASSERT(ai != NULL);

	return (R_REG(sii->osh, &ai->oobselouta30) & 0x1f);
}

uint
ai_flag_alt(si_t *sih)
{
	si_info_t *sii = SI_INFO(sih);
	aidmp_t *ai;

	ai = sii->curwrap;

	return ((R_REG(sii->osh, &ai->oobselouta30) >> AI_OOBSEL_1_SHIFT) & AI_OOBSEL_MASK);
}

void
ai_setint(si_t *sih, int siflag)
{
	BCM_REFERENCE(sih);
	BCM_REFERENCE(siflag);

}

uint
ai_wrap_reg(si_t *sih, uint32 offset, uint32 mask, uint32 val)
{
	si_info_t *sii = SI_INFO(sih);
	uint32 *addr = (uint32 *) ((uchar *)(sii->curwrap) + offset);

	if (mask || val) {
		uint32 w = R_REG(sii->osh, addr);
		w &= ~mask;
		w |= val;
		W_REG(sii->osh, addr, w);
	}
	return (R_REG(sii->osh, addr));
}

uint
ai_corevendor(si_t *sih)
{
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
	uint32 cia;

	cia = cores_info->cia[sii->curidx];
	return ((cia & CIA_MFG_MASK) >> CIA_MFG_SHIFT);
}

uint
ai_corerev(si_t *sih)
{
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
	uint32 cib;

	cib = cores_info->cib[sii->curidx];
	return ((cib & CIB_REV_MASK) >> CIB_REV_SHIFT);
}

uint
ai_corerev_minor(si_t *sih)
{
	return (ai_core_sflags(sih, 0, 0) >> SISF_MINORREV_D11_SHIFT) &
			SISF_MINORREV_D11_MASK;
}

bool
ai_iscoreup(si_t *sih)
{
	si_info_t *sii = SI_INFO(sih);
	aidmp_t *ai;

	ai = sii->curwrap;

	return (((R_REG(sii->osh, &ai->ioctrl) & (SICF_FGC | SICF_CLOCK_EN)) == SICF_CLOCK_EN) &&
	        ((R_REG(sii->osh, &ai->resetctrl) & AIRC_RESET) == 0));
}

/*
 * Switch to 'coreidx', issue a single arbitrary 32bit register mask&set operation,
 * switch back to the original core, and return the new value.
 *
 * When using the silicon backplane, no fiddling with interrupts or core switches is needed.
 *
 * Also, when using pci/pcie, we can optimize away the core switching for pci registers
 * and (on newer pci cores) chipcommon registers.
 */
uint
ai_corereg(si_t *sih, uint coreidx, uint regoff, uint mask, uint val)
{
	uint origidx = 0;
	volatile uint32 *r = NULL;
	uint w;
	uint intr_val = 0;
	bool fast = FALSE;
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;

	ASSERT(GOODIDX(coreidx));
	ASSERT(regoff < SI_CORE_SIZE);
	ASSERT((val & ~mask) == 0);

	if (coreidx >= SI_MAXCORES)
		return 0;

	if (BUSTYPE(sih->bustype) == SI_BUS) {
		/* If internal bus, we can always get at everything */
		fast = TRUE;
		/* map if does not exist */
		if (!cores_info->regs[coreidx]) {
			void *regs;
			if (EMBEDDED_WLAN_CORE(sih->chip)) {
				/* linear large addres space, directly accessible */
				regs = (void*)
				       (sih->enum_base_va +
				        (cores_info->coresba_pa[coreidx] - SI_ENUM_BASE_PA(sih)));
			} else {
				regs = REG_MAP(cores_info->coresba_pa[coreidx], SI_CORE_SIZE);
			}
			ASSERT(GOODREGS(regs));
			cores_info->regs[coreidx] = regs;
		}
		r = (volatile uint32 *)((volatile uchar *)cores_info->regs[coreidx] + regoff);
	} else if (BUSTYPE(sih->bustype) == PCI_BUS) {
		/* If pci/pcie, we can get at pci/pcie regs and on newer cores to chipc */

		if ((cores_info->coreid[coreidx] == CC_CORE_ID) && SI_FAST(sii)) {
			/* Chipc registers are mapped at 12KB */

			fast = TRUE;
			r = (volatile uint32 *)((volatile char *)sii->curmap +
			               PCI_16KB0_CCREGS_OFFSET + regoff);
		} else if (sii->pub.buscoreidx == coreidx) {
			/* pci registers are at either in the last 2KB of an 8KB window
			 * or, in pcie and pci rev 13 at 8KB
			 */
			fast = TRUE;
			if (SI_FAST(sii))
				r = (volatile uint32 *)((volatile char *)sii->curmap +
				               PCI_16KB0_PCIREGS_OFFSET + regoff);
			else
				r = (volatile uint32 *)((volatile char *)sii->curmap +
				               ((regoff >= SBCONFIGOFF) ?
				                PCI_BAR0_PCISBR_OFFSET : PCI_BAR0_PCIREGS_OFFSET) +
				               regoff);
		}
	}

	if (!fast) {
		INTR_OFF(sii, intr_val);

		/* save current core index */
		origidx = si_coreidx(&sii->pub);

		/* switch core */
		r = (volatile uint32*) ((volatile uchar*) ai_setcoreidx(&sii->pub, coreidx) +
		               regoff);
	}
	ASSERT(r != NULL);

	/* mask and set */
	if (mask || val) {
		w = (R_REG(sii->osh, r) & ~mask) | val;
		W_REG(sii->osh, r, w);
	}

	/* readback */
	w = R_REG(sii->osh, r);

	if (!fast) {
		/* restore core index */
		if (origidx != coreidx)
			ai_setcoreidx(&sii->pub, origidx);

		INTR_RESTORE(sii, intr_val);
	}

	return (w);
}

/*
 * If there is no need for fiddling with interrupts or core switches (typically silicon
 * back plane registers, pci registers and chipcommon registers), this function
 * returns the register offset on this core to a mapped (=virtual) address. This address can
 * be used for W_REG/R_REG directly.
 *
 * For accessing registers that would need a core switch, this function will return
 * NULL.
 */
volatile uint32 *
ai_corereg_addr(si_t *sih, uint coreidx, uint regoff)
{
	volatile uint32 *r = NULL;
	bool fast = FALSE;
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;

	ASSERT(GOODIDX(coreidx));
	ASSERT(regoff < SI_CORE_SIZE);

	if (coreidx >= SI_MAXCORES)
		return 0;

	if (BUSTYPE(sih->bustype) == SI_BUS) {
		/* If internal bus, we can always get at everything */
		fast = TRUE;
		/* map if does not exist */
		if (!cores_info->regs[coreidx]) {
			void *regs;
			if (EMBEDDED_WLAN_CORE(sih->chip)) {
				/* direct memory access, so only calculate, don't map */
				regs = (void*)
				       (sih->enum_base_va +
				        (cores_info->coresba_pa[coreidx] - SI_ENUM_BASE_PA(sih)));
			} else {
				regs = REG_MAP(cores_info->coresba_pa[coreidx], SI_CORE_SIZE);
			}
			ASSERT(GOODREGS(regs));
			cores_info->regs[coreidx] = regs;
		}

		r = (volatile uint32 *)((volatile uchar *)cores_info->regs[coreidx] + regoff);
	} else if (BUSTYPE(sih->bustype) == PCI_BUS) {
		/* If pci/pcie, we can get at pci/pcie regs and on newer cores to chipc */

		if ((cores_info->coreid[coreidx] == CC_CORE_ID) && SI_FAST(sii)) {
			/* Chipc registers are mapped at 12KB */

			fast = TRUE;
			r = (volatile uint32 *)((volatile char *)sii->curmap +
			               PCI_16KB0_CCREGS_OFFSET + regoff);
		} else if (sii->pub.buscoreidx == coreidx) {
			/* pci registers are at either in the last 2KB of an 8KB window
			 * or, in pcie and pci rev 13 at 8KB
			 */
			fast = TRUE;
			if (SI_FAST(sii))
				r = (volatile uint32 *)((volatile char *)sii->curmap +
				               PCI_16KB0_PCIREGS_OFFSET + regoff);
			else
				r = (volatile uint32 *)((volatile char *)sii->curmap +
				               ((regoff >= SBCONFIGOFF) ?
				                PCI_BAR0_PCISBR_OFFSET : PCI_BAR0_PCIREGS_OFFSET) +
				               regoff);
		}
	}

	if (!fast) {
		ASSERT(sii->curidx == coreidx);
		r = (volatile uint32*) ((volatile uchar*)sii->curmap + regoff);
	}

	return (r);
}

void
ai_core_disable(si_t *sih, uint32 bits)
{
	si_info_t *sii = SI_INFO(sih);
	volatile uint32 dummy;
	uint32 status;
	aidmp_t *ai;

	ASSERT(GOODREGS(sii->curwrap));
	ai = sii->curwrap;

	/* if core is already in reset, just return */
	if (R_REG(sii->osh, &ai->resetctrl) & AIRC_RESET) {
		return;
	}

	/* ensure there are no pending backplane operations */
	SPINWAIT(((status = R_REG(sii->osh, &ai->resetstatus)) != 0), 300);

	/* if pending backplane ops still, try waiting longer */
	if (status != 0) {
		/* 300usecs was sufficient to allow backplane ops to clear for big hammer */
		/* during driver load we may need more time */
		SPINWAIT(((status = R_REG(sii->osh, &ai->resetstatus)) != 0), 10000);
		/* if still pending ops, continue on and try disable anyway */
		/* this is in big hammer path, so don't call wl_reinit in this case... */
#ifdef BCMDBG
		if (status != 0) {
			SI_ERROR(("%s: WARN: resetstatus=%0x on core disable\n",
				__FUNCTION__, status));
		}
#endif // endif
	}

	W_REG(sii->osh, &ai->resetctrl, AIRC_RESET);
	dummy = R_REG(sii->osh, &ai->resetctrl);
	BCM_REFERENCE(dummy);
	OSL_DELAY(1);

	W_REG(sii->osh, &ai->ioctrl, bits);
	dummy = R_REG(sii->osh, &ai->ioctrl);
	BCM_REFERENCE(dummy);
	OSL_DELAY(10);
}

/* reset and re-enable a core
 * inputs:
 * bits - core specific bits that are set during and after reset sequence
 * resetbits - core specific bits that are set only during reset sequence
 */
static void
_ai_core_reset(si_t *sih, uint32 bits, uint32 resetbits)
{
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
	aidmp_t *ai;
	volatile uint32 dummy;
	uint loop_counter = 10;
	bool war_enab = (cores_info->coreid[sii->curidx] == D11_CORE_ID) &&
		(((ai_corerev(sih) >= 128) && (ai_corerev(sih) <= 132)) ||
		(ai_corerev(sih) == 61));

	ASSERT(GOODREGS(sii->curwrap));
	ai = sii->curwrap;

	/* ensure there are no pending backplane operations */
	SPINWAIT(((dummy = R_REG(sii->osh, &ai->resetstatus)) != 0), 300);

#ifdef BCMDBG_ERR
	if (dummy != 0) {
		SI_ERROR(("%s: WARN1: resetstatus=0x%0x\n", __FUNCTION__, dummy));
	}
#endif /* BCMDBG_ERR */

	/* put core into reset state */
	W_REG(sii->osh, &ai->resetctrl, AIRC_RESET);
	OSL_DELAY(10);

	/* ensure there are no pending backplane operations */
	SPINWAIT((R_REG(sii->osh, &ai->resetstatus) != 0), 300);

	W_REG(sii->osh, &ai->ioctrl, (bits | resetbits | SICF_FGC | SICF_CLOCK_EN));
	dummy = R_REG(sii->osh, &ai->ioctrl);
	BCM_REFERENCE(dummy);
	if (war_enab) {
		/* Reset FGC */
		OSL_DELAY(1);
		W_REG(sii->osh, &ai->ioctrl, (dummy & (~SICF_FGC)));
	}
	/* ensure there are no pending backplane operations */
	SPINWAIT(((dummy = R_REG(sii->osh, &ai->resetstatus)) != 0), 300);

#ifdef BCMDBG_ERR
	if (dummy != 0)
		SI_ERROR(("%s: WARN2: resetstatus=0x%0x\n", __FUNCTION__, dummy));
#endif // endif

	while (R_REG(sii->osh, &ai->resetctrl) != 0 && --loop_counter != 0) {
		/* ensure there are no pending backplane operations */
		SPINWAIT(((dummy = R_REG(sii->osh, &ai->resetstatus)) != 0), 300);

#ifdef BCMDBG_ERR
		if (dummy != 0)
			SI_ERROR(("%s: WARN3 resetstatus=0x%0x\n", __FUNCTION__, dummy));
#endif // endif

		if (BCM6717_CHIP(sih->chip) && HWA_DMP()) {
			/* stop clock */
			W_REG(sii->osh, &ai->ioctrl, 0);
			OSL_DELAY(1);
		}

		/* take core out of reset */
		W_REG(sii->osh, &ai->resetctrl, 0);

		/* ensure there are no pending backplane operations */
		SPINWAIT((R_REG(sii->osh, &ai->resetstatus) != 0), 300);
	}

#ifdef BCMDBG_ERR
	if (loop_counter == 0)
		SI_ERROR(("%s: Failed to take core 0x%x out of reset\n",
		          __FUNCTION__, cores_info->coreid[sii->curidx]));
#endif // endif

	if (war_enab) {
		/* Pulse FGC after lifting Reset */
		W_REG(sii->osh, &ai->ioctrl, (bits | SICF_FGC | SICF_CLOCK_EN));
	} else {
		W_REG(sii->osh, &ai->ioctrl, (bits | SICF_CLOCK_EN));
	}
	dummy = R_REG(sii->osh, &ai->ioctrl);
	BCM_REFERENCE(dummy);

	if (war_enab) {
		/* Reset FGC */
		OSL_DELAY(1);
		W_REG(sii->osh, &ai->ioctrl, (dummy & (~SICF_FGC)));
	}

	OSL_DELAY(1);
}

void
ai_core_reset(si_t *sih, uint32 bits, uint32 resetbits)
{
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
	uint idx = sii->curidx;
	uint wrapidx;

	for (wrapidx = 1; wrapidx < AI_MAX_WRAPPERS_PERCORE; wrapidx++) {
		if (cores_info->wrapba_pa[idx][wrapidx] != 0) {
			_ai_setcoreidx(sih, idx, wrapidx);
			_ai_core_reset(sih, bits, resetbits);
		}
	}

	_ai_setcoreidx(sih, idx, 0);
	_ai_core_reset(sih, bits, resetbits);
}

void
ai_core_cflags_wo(si_t *sih, uint32 mask, uint32 val)
{
	si_info_t *sii = SI_INFO(sih);
#if !defined(BCMDONGLEHOST)
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
#endif // endif
	aidmp_t *ai;
	uint32 w;

	if (PMU_DMP()) {
		SI_ERROR(("%s: Accessing PMU DMP register (ioctrl)\n",
			__FUNCTION__));
		return;
	}

	ASSERT(GOODREGS(sii->curwrap));
	ai = sii->curwrap;

	ASSERT((val & ~mask) == 0);

	if (mask || val) {
		w = ((R_REG(sii->osh, &ai->ioctrl) & ~mask) | val);
		W_REG(sii->osh, &ai->ioctrl, w);
	}
}

uint32
ai_core_cflags(si_t *sih, uint32 mask, uint32 val)
{
	si_info_t *sii = SI_INFO(sih);
#if !defined(BCMDONGLEHOST)
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
#endif // endif
	aidmp_t *ai;
	uint32 w;

	if (PMU_DMP()) {
		SI_ERROR(("%s: Accessing PMU DMP register (ioctrl)\n",
			__FUNCTION__));
		return 0;
	}
	ASSERT(GOODREGS(sii->curwrap));
	ai = sii->curwrap;

	ASSERT((val & ~mask) == 0);

	if (mask || val) {
		w = ((R_REG(sii->osh, &ai->ioctrl) & ~mask) | val);
		W_REG(sii->osh, &ai->ioctrl, w);
	}

	return R_REG(sii->osh, &ai->ioctrl);
}

uint32
ai_core_sflags(si_t *sih, uint32 mask, uint32 val)
{
	si_info_t *sii = SI_INFO(sih);
#if !defined(BCMDONGLEHOST)
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
#endif // endif
	aidmp_t *ai;
	uint32 w;

	if (PMU_DMP()) {
		SI_ERROR(("%s: Accessing PMU DMP register (ioctrl)\n",
			__FUNCTION__));
		return 0;
	}

	ASSERT(GOODREGS(sii->curwrap));
	ai = sii->curwrap;

	ASSERT((val & ~mask) == 0);
	ASSERT((mask & ~SISF_CORE_BITS) == 0);

	if (mask || val) {
		w = ((R_REG(sii->osh, &ai->iostatus) & ~mask) | val);
		W_REG(sii->osh, &ai->iostatus, w);
	}

	return R_REG(sii->osh, &ai->iostatus);
}

#if defined(BCMDBG) || defined(BCMDBG_PHYDUMP)
/* print interesting aidmp registers */
void
ai_dumpregs(si_t *sih, struct bcmstrbuf *b)
{
	si_info_t *sii = SI_INFO(sih);
	osl_t *osh;
	aidmp_t *ai;
	uint i;
	uint32 prev_value = 0;
	axi_wrapper_t * axi_wrapper = sii->axi_wrapper;
	uint32 cfg_reg = 0;
	uint bar0_win_offset = 0;

	osh = sii->osh;

	/* Save and restore wrapper access window */
	if (BUSTYPE(sii->pub.bustype) == PCI_BUS) {
		if (PCIE_GEN2(sii)) {
			cfg_reg = PCIE2_BAR0_CORE2_WIN2;
			bar0_win_offset = PCIE2_BAR0_CORE2_WIN2_OFFSET;
		} else {
			cfg_reg = PCI_BAR0_WIN2;
			bar0_win_offset = PCI_BAR0_WIN2_OFFSET;
		}

		prev_value = OSL_PCI_READ_CONFIG(osh, cfg_reg, 4);

		if (prev_value == ID32_INVALID) {
			SI_PRINT(("%s, PCI_BAR0_WIN2 - %x\n", __FUNCTION__, prev_value));
			return;
		}
	}

	bcm_bprintf(b, "ChipNum:%x, ChipRev;%x, BusType:%x, BoardType:%x, BoardVendor:%x\n\n",
		sih->chip, sih->chiprev, sih->bustype, sih->boardtype, sih->boardvendor);

	for (i = 0; i < sii->axi_num_wrappers; i++) {

		if (BUSTYPE(sii->pub.bustype) == PCI_BUS) {
			/* Set BAR0 window to bridge wapper base address */
			OSL_PCI_WRITE_CONFIG(osh,
				cfg_reg, 4, axi_wrapper[i].wrapper_pa);

			ai = (aidmp_t *) ((volatile uint8*)sii->curmap + bar0_win_offset);
		} else {
			ai = (aidmp_t *)(uintptr) axi_wrapper[i].wrapper_pa;
		}

		bcm_bprintf(b, "core 0x%x: core_rev:%d, %s_WR ADDR:%x \n", axi_wrapper[i].cid,
			axi_wrapper[i].rev,
			axi_wrapper[i].wrapper_type == AI_SLAVE_WRAPPER ? "SLAVE" : "MASTER",
			axi_wrapper[i].wrapper_pa);

		bcm_bprintf(b, "ioctrlset 0x%x ioctrlclear 0x%x ioctrl 0x%x iostatus 0x%x "
			    "ioctrlwidth 0x%x iostatuswidth 0x%x\n"
			    "resetctrl 0x%x resetstatus 0x%x resetreadid 0x%x resetwriteid 0x%x\n"
			    "errlogctrl 0x%x errlogdone 0x%x errlogstatus 0x%x "
			    "errlogaddrlo 0x%x errlogaddrhi 0x%x\n"
			    "errlogid 0x%x errloguser 0x%x errlogflags 0x%x\n"
			    "intstatus 0x%x config 0x%x itcr 0x%x\n\n",
			    R_REG(osh, &ai->ioctrlset),
			    R_REG(osh, &ai->ioctrlclear),
			    R_REG(osh, &ai->ioctrl),
			    R_REG(osh, &ai->iostatus),
			    R_REG(osh, &ai->ioctrlwidth),
			    R_REG(osh, &ai->iostatuswidth),
			    R_REG(osh, &ai->resetctrl),
			    R_REG(osh, &ai->resetstatus),
			    R_REG(osh, &ai->resetreadid),
			    R_REG(osh, &ai->resetwriteid),
			    R_REG(osh, &ai->errlogctrl),
			    R_REG(osh, &ai->errlogdone),
			    R_REG(osh, &ai->errlogstatus),
			    R_REG(osh, &ai->errlogaddrlo),
			    R_REG(osh, &ai->errlogaddrhi),
			    R_REG(osh, &ai->errlogid),
			    R_REG(osh, &ai->errloguser),
			    R_REG(osh, &ai->errlogflags),
			    R_REG(osh, &ai->intstatus),
			    R_REG(osh, &ai->config),
			    R_REG(osh, &ai->itcr));
	}

	/* Restore the initial wrapper space */
	if (BUSTYPE(sii->pub.bustype) == PCI_BUS) {
		if (prev_value && cfg_reg) {
			OSL_PCI_WRITE_CONFIG(osh, cfg_reg, 4, prev_value);
		}
	}
}
#endif // endif

#ifdef BCMDBG
static void
_ai_view(osl_t *osh, aidmp_t *ai, uint32 cid, uint32 addr_pa, bool verbose)
{
	uint32 config;

	if (ai == NULL) {
		config = 0;
	} else {
		config = R_REG(osh, &ai->config);
	}
	SI_PRINT(("\nCore ID: 0x%x, addr_pa 0x%x, config 0x%x, ai: 0x%x\n", cid, addr_pa, config,
		(uint32)(uintptr)ai));

	if (config & AICFG_RST)
		SI_PRINT(("resetctrl 0x%x, resetstatus 0x%x, resetreadid 0x%x, resetwriteid 0x%x\n",
		          R_REG(osh, &ai->resetctrl), R_REG(osh, &ai->resetstatus),
		          R_REG(osh, &ai->resetreadid), R_REG(osh, &ai->resetwriteid)));

	if (config & AICFG_IOC)
		SI_PRINT(("ioctrl 0x%x, width %d\n", R_REG(osh, &ai->ioctrl),
		          R_REG(osh, &ai->ioctrlwidth)));

	if (config & AICFG_IOS)
		SI_PRINT(("iostatus 0x%x, width %d\n", R_REG(osh, &ai->iostatus),
		          R_REG(osh, &ai->iostatuswidth)));

	if (config & AICFG_ERRL) {
		SI_PRINT(("errlogctrl 0x%x, errlogdone 0x%x, errlogstatus 0x%x, intstatus 0x%x\n",
		          R_REG(osh, &ai->errlogctrl), R_REG(osh, &ai->errlogdone),
		          R_REG(osh, &ai->errlogstatus), R_REG(osh, &ai->intstatus)));
		SI_PRINT(("errlogid 0x%x, errloguser 0x%x, errlogflags 0x%x, errlogaddr "
		          "0x%x/0x%x\n",
		          R_REG(osh, &ai->errlogid), R_REG(osh, &ai->errloguser),
		          R_REG(osh, &ai->errlogflags), R_REG(osh, &ai->errlogaddrhi),
		          R_REG(osh, &ai->errlogaddrlo)));
	}

	if (verbose && (config & AICFG_OOB)) {
		SI_PRINT(("oobselina30 0x%x, oobselina74 0x%x\n",
		          R_REG(osh, &ai->oobselina30), R_REG(osh, &ai->oobselina74)));
		SI_PRINT(("oobselinb30 0x%x, oobselinb74 0x%x\n",
		          R_REG(osh, &ai->oobselinb30), R_REG(osh, &ai->oobselinb74)));
		SI_PRINT(("oobselinc30 0x%x, oobselinc74 0x%x\n",
		          R_REG(osh, &ai->oobselinc30), R_REG(osh, &ai->oobselinc74)));
		SI_PRINT(("oobselind30 0x%x, oobselind74 0x%x\n",
		          R_REG(osh, &ai->oobselind30), R_REG(osh, &ai->oobselind74)));
		SI_PRINT(("oobselouta30 0x%x, oobselouta74 0x%x\n",
		          R_REG(osh, &ai->oobselouta30), R_REG(osh, &ai->oobselouta74)));
		SI_PRINT(("oobseloutb30 0x%x, oobseloutb74 0x%x\n",
		          R_REG(osh, &ai->oobseloutb30), R_REG(osh, &ai->oobseloutb74)));
		SI_PRINT(("oobseloutc30 0x%x, oobseloutc74 0x%x\n",
		          R_REG(osh, &ai->oobseloutc30), R_REG(osh, &ai->oobseloutc74)));
		SI_PRINT(("oobseloutd30 0x%x, oobseloutd74 0x%x\n",
		          R_REG(osh, &ai->oobseloutd30), R_REG(osh, &ai->oobseloutd74)));
		SI_PRINT(("oobsynca 0x%x, oobseloutaen 0x%x\n",
		          R_REG(osh, &ai->oobsynca), R_REG(osh, &ai->oobseloutaen)));
		SI_PRINT(("oobsyncb 0x%x, oobseloutben 0x%x\n",
		          R_REG(osh, &ai->oobsyncb), R_REG(osh, &ai->oobseloutben)));
		SI_PRINT(("oobsyncc 0x%x, oobseloutcen 0x%x\n",
		          R_REG(osh, &ai->oobsyncc), R_REG(osh, &ai->oobseloutcen)));
		SI_PRINT(("oobsyncd 0x%x, oobseloutden 0x%x\n",
		          R_REG(osh, &ai->oobsyncd), R_REG(osh, &ai->oobseloutden)));
		SI_PRINT(("oobaextwidth 0x%x, oobainwidth 0x%x, oobaoutwidth 0x%x\n",
		          R_REG(osh, &ai->oobaextwidth), R_REG(osh, &ai->oobainwidth),
		          R_REG(osh, &ai->oobaoutwidth)));
		SI_PRINT(("oobbextwidth 0x%x, oobbinwidth 0x%x, oobboutwidth 0x%x\n",
		          R_REG(osh, &ai->oobbextwidth), R_REG(osh, &ai->oobbinwidth),
		          R_REG(osh, &ai->oobboutwidth)));
		SI_PRINT(("oobcextwidth 0x%x, oobcinwidth 0x%x, oobcoutwidth 0x%x\n",
		          R_REG(osh, &ai->oobcextwidth), R_REG(osh, &ai->oobcinwidth),
		          R_REG(osh, &ai->oobcoutwidth)));
		SI_PRINT(("oobdextwidth 0x%x, oobdinwidth 0x%x, oobdoutwidth 0x%x\n",
		          R_REG(osh, &ai->oobdextwidth), R_REG(osh, &ai->oobdinwidth),
		          R_REG(osh, &ai->oobdoutwidth)));
	}
}

void
ai_view(si_t *sih, bool verbose)
{
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
	osl_t *osh;
	aidmp_t *ai;
	uint32 cid, addr_pa;

	ai = sii->curwrap;
	osh = sii->osh;
	if (PMU_DMP()) {
		SI_ERROR(("Cannot access pmu DMP\n"));
		return;
	}
	cid = cores_info->coreid[sii->curidx];
	addr_pa = cores_info->wrapba_pa[sii->curidx][0];
	_ai_view(osh, ai, cid, addr_pa, verbose);
}

void
ai_viewall(si_t *sih, bool verbose)
{
	si_info_t *sii = SI_INFO(sih);
	uint i;

	for (i = 0; i < sii->numcores; i++) {
		si_setcoreidx(sih, i);
		ai_view(sih, verbose);
	}
}
#endif	/* BCMDBG */

void
ai_update_backplane_timeouts(si_t *sih, bool enable, uint32 timeout_exp, uint32 cid)
{
#if defined(AXI_TIMEOUTS) || defined(BCM_BACKPLANE_TIMEOUT)
	si_info_t *sii = SI_INFO(sih);
	aidmp_t *ai;
	uint32 i;
	axi_wrapper_t * axi_wrapper = sii->axi_wrapper;
	uint32 errlogctrl = (enable << AIELC_TO_ENAB_SHIFT) |
		((timeout_exp << AIELC_TO_EXP_SHIFT) & AIELC_TO_EXP_MASK);

#ifdef BCM_BACKPLANE_TIMEOUT
	uint32 prev_value = 0;
	osl_t *osh = sii->osh;
	uint32 cfg_reg = 0;
	uint32 offset = 0;
#endif /* BCM_BACKPLANE_TIMEOUT */

	if ((sii->axi_num_wrappers == 0) ||
#ifdef BCM_BACKPLANE_TIMEOUT
		(!PCIE(sii)) ||
#endif /* BCM_BACKPLANE_TIMEOUT */
		FALSE) {
		SI_VMSG((" %s, axi_num_wrappers:%d, Is_PCIE:%d, BUS_TYPE:%d, ID:%x\n",
			__FUNCTION__, sii->axi_num_wrappers, PCIE(sii),
			BUSTYPE(sii->pub.bustype), sii->pub.buscoretype));
		return;
	}

#ifdef BCM_BACKPLANE_TIMEOUT
	/* Save and restore the wrapper access window */
	if (BUSTYPE(sii->pub.bustype) == PCI_BUS) {
		if (PCIE_GEN1(sii)) {
			cfg_reg = PCI_BAR0_WIN2;
			offset = PCI_BAR0_WIN2_OFFSET;
		} else if (PCIE_GEN2(sii)) {
			cfg_reg = PCIE2_BAR0_CORE2_WIN2;
			offset = PCIE2_BAR0_CORE2_WIN2_OFFSET;
		}
		else {
			ASSERT(!"!PCIE_GEN1 && !PCIE_GEN2");
		}

		prev_value = OSL_PCI_READ_CONFIG(osh, cfg_reg, 4);
		if (prev_value == ID32_INVALID) {
			SI_PRINT(("%s, PCI_BAR0_WIN2 - %x\n", __FUNCTION__, prev_value));
			return;
		}
	}
#endif /* BCM_BACKPLANE_TIMEOUT */

	for (i = 0; i < sii->axi_num_wrappers; ++i) {

		if (axi_wrapper[i].wrapper_type != AI_SLAVE_WRAPPER) {
			SI_VMSG(("SKIP ENABLE BPT: MFG:%x, CID:%x, ADDR:%x\n",
				axi_wrapper[i].mfg,
				axi_wrapper[i].cid,
				axi_wrapper[i].wrapper_pa));
			continue;
		}

		/* Update only given core if requested */
		if ((cid != 0) && (axi_wrapper[i].cid != cid)) {
			continue;
		}

#ifdef BCM_BACKPLANE_TIMEOUT
		if (BUSTYPE(sii->pub.bustype) == PCI_BUS) {
			/* Set BAR0_CORE2_WIN2 to bridge wapper base address */
			OSL_PCI_WRITE_CONFIG(osh,
				cfg_reg, 4, axi_wrapper[i].wrapper_pa);

			/* set AI to BAR0 + Offset corresponding to Gen1 or gen2 */
			ai = (aidmp_t *) (DISCARD_QUAL(sii->curmap, uint8) + offset);
		}
		else
#endif /* BCM_BACKPLANE_TIMEOUT */
		{
			ai = (aidmp_t *)(uintptr) axi_wrapper[i].wrapper_pa;
		}

		W_REG(sii->osh, &ai->errlogctrl, errlogctrl);

		SI_VMSG(("ENABLED BPT: MFG:%x, CID:%x, ADDR:%x, ERR_CTRL:%x\n",
			axi_wrapper[i].mfg,
			axi_wrapper[i].cid,
			axi_wrapper[i].wrapper_pa,
			R_REG(sii->osh, &ai->errlogctrl)));
	}

#ifdef BCM_BACKPLANE_TIMEOUT
	/* Restore the initial wrapper space */
	if (prev_value) {
		OSL_PCI_WRITE_CONFIG(osh, cfg_reg, 4, prev_value);
	}
#endif /* BCM_BACKPLANE_TIMEOUT */

#endif /* AXI_TIMEOUTS || BCM_BACKPLANE_TIMEOUT */
}

#ifdef BCM_BACKPLANE_TIMEOUT

/* Function to return the APB bridge details corresponding to the core */
static bool
ai_get_apb_bridge(si_t * sih, uint32 coreidx, uint32 *apb_id, uint32 * apb_coreuinit)
{
	uint i;
	uint32 core_base, core_end;
	si_info_t *sii = SI_INFO(sih);
	static uint32 coreidx_cached = 0, apb_id_cached = 0, apb_coreunit_cached = 0;
	uint32 tmp_coreunit = 0;
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;

	if (coreidx >= MIN(sii->numcores, SI_MAXCORES))
		return FALSE;

	/* Most of the time apb bridge query will be for d11 core.
	 * Maintain the last cache and return if found rather than iterating the table
	 */
	if (coreidx_cached == coreidx) {
		*apb_id = apb_id_cached;
		*apb_coreuinit = apb_coreunit_cached;
		return TRUE;
	}

	core_base = cores_info->coresba_pa[coreidx];
	core_end = core_base + cores_info->coresba_size[coreidx];

	for (i = 0; i < sii->numcores; i++) {
		if (cores_info->coreid[i] == APB_BRIDGE_ID) {
			uint32 apb_base;
			uint32 apb_end;

			apb_base = cores_info->coresba_pa[i];
			apb_end = apb_base + cores_info->coresba_size[i];

			if ((core_base >= apb_base) &&
				(core_end <= apb_end)) {
				/* Current core is attached to this APB bridge */
				*apb_id = apb_id_cached = APB_BRIDGE_ID;
				*apb_coreuinit = apb_coreunit_cached = tmp_coreunit;
				coreidx_cached = coreidx;
				return TRUE;
			}
			/* Increment the coreunit */
			tmp_coreunit++;
		}
	}

	return FALSE;
}

uint32
ai_clear_backplane_to_fast(si_t *sih, void *addr)
{
	si_info_t *sii = SI_INFO(sih);
	volatile void *curmap = sii->curmap;
	bool core_reg = FALSE;

	/* Use fast path only for core register access */
	if (((uintptr)addr >= (uintptr)curmap) &&
		((uintptr)addr < ((uintptr)curmap + SI_CORE_SIZE))) {
		/* address being accessed is within current core reg map */
		core_reg = TRUE;
	}

	if (core_reg) {
		uint32 apb_id, apb_coreuinit;

		if (ai_get_apb_bridge(sih, si_coreidx(&sii->pub),
			&apb_id, &apb_coreuinit) == TRUE) {
			/* Found the APB bridge corresponding to current core,
			 * Check for bus errors in APB wrapper
			 */
			return ai_clear_backplane_to_per_core(sih,
				apb_id, apb_coreuinit, NULL);
		}
	}

	/* Default is to poll for errors on all slave wrappers */
	return si_clear_backplane_to(sih);
}
#endif /* BCM_BACKPLANE_TIMEOUT */

#if defined(AXI_TIMEOUTS) || defined(BCM_BACKPLANE_TIMEOUT)
static bool g_disable_backplane_logs = FALSE;

/*
 * API to clear the back plane timeout per core.
 * Caller may passs optional wrapper address. If present this will be used as
 * the wrapper base address. If wrapper base address is provided then caller
 * must provide the coreid also.
 * If both coreid and wrapper is zero, then err status of current bridge
 * will be verified.
 */
uint32
ai_clear_backplane_to_per_core(si_t *sih, uint coreid, uint coreunit, void *wrap)
{
	int ret = AXI_WRAP_STS_NONE;
	aidmp_t *ai = NULL;
	uint32 errlog_status = 0;
	si_info_t *sii = SI_INFO(sih);
	uint32 errlog_lo = 0, errlog_hi = 0, errlog_id = 0, errlog_flags = 0;
	uint32 current_coreidx = si_coreidx(sih);
	uint32 target_coreidx = si_findcoreidx(sih, coreid, coreunit);

#if defined(BCM_BACKPLANE_TIMEOUT)
	si_axi_error_t * axi_error = sih->err_info ?
		&sih->err_info->axi_error[sih->err_info->count] : NULL;
#endif /* BCM_BACKPLANE_TIMEOUT */
	bool restore_core = FALSE;

	if ((sii->axi_num_wrappers == 0) ||
#ifdef BCM_BACKPLANE_TIMEOUT
		(!PCIE(sii)) ||
#endif /* BCM_BACKPLANE_TIMEOUT */
		FALSE) {
		SI_VMSG((" %s, axi_num_wrappers:%d, Is_PCIE:%d, BUS_TYPE:%d, ID:%x\n",
			__FUNCTION__, sii->axi_num_wrappers, PCIE(sii),
			BUSTYPE(sii->pub.bustype), sii->pub.buscoretype));
		return AXI_WRAP_STS_NONE;
	}

	if (wrap != NULL) {
		ai = (aidmp_t *)wrap;
	} else if (coreid && (target_coreidx != current_coreidx)) {

		if (ai_setcoreidx(sih, target_coreidx) == NULL) {
			/* Unable to set the core */
			SI_PRINT(("Set Code Failed: coreid:%x, unit:%d, target_coreidx:%d\n",
				coreid, coreunit, target_coreidx));
			errlog_lo = target_coreidx;
			ret = AXI_WRAP_STS_SET_CORE_FAIL;
			goto end;
		}

		restore_core = TRUE;
		ai = (aidmp_t *)si_wrapperregs(sih);
	} else {
		/* Read error status of current wrapper */
		ai = (aidmp_t *)si_wrapperregs(sih);

		/* Update CoreID to current Code ID */
		coreid = si_coreid(sih);
	}

	/* read error log status */
	errlog_status = R_REG(sii->osh, &ai->errlogstatus);

	if (errlog_status == ID32_INVALID) {
		/* Do not try to peek further */
		SI_PRINT(("%s, errlogstatus:%x - Slave Wrapper:%x\n",
			__FUNCTION__, errlog_status, coreid));
		ret = AXI_WRAP_STS_WRAP_RD_ERR;
		errlog_lo = (uint32)(uintptr)&ai->errlogstatus;
		goto end;
	}

	if ((errlog_status & AIELS_TIMEOUT_MASK) != 0) {
		uint32 tmp;
		uint32 count = 0;
		/* set ErrDone to clear the condition */
		W_REG(sii->osh, &ai->errlogdone, AIELD_ERRDONE_MASK);

		/* SPINWAIT on errlogstatus timeout status bits */
		while ((tmp = R_REG(sii->osh, &ai->errlogstatus)) & AIELS_TIMEOUT_MASK) {

			if (tmp == ID32_INVALID) {
				SI_PRINT(("%s: prev errlogstatus:%x, errlogstatus:%x\n",
					__FUNCTION__, errlog_status, tmp));
				ret = AXI_WRAP_STS_WRAP_RD_ERR;
				errlog_lo = (uint32)(uintptr)&ai->errlogstatus;
				goto end;
			}
			/*
			 * Clear again, to avoid getting stuck in the loop, if a new error
			 * is logged after we cleared the first timeout
			 */
			W_REG(sii->osh, &ai->errlogdone, AIELD_ERRDONE_MASK);

			count++;
			OSL_DELAY(10);
			if ((10 * count) > AI_REG_READ_TIMEOUT) {
				errlog_status = tmp;
				break;
			}
		}

		errlog_lo = R_REG(sii->osh, &ai->errlogaddrlo);
		errlog_hi = R_REG(sii->osh, &ai->errlogaddrhi);
		errlog_id = R_REG(sii->osh, &ai->errlogid);
		errlog_flags = R_REG(sii->osh, &ai->errlogflags);

		/* only reset APB Bridge on timeout (not slave error, or dec error) */
		switch (errlog_status & AIELS_TIMEOUT_MASK) {
			case AIELS_SLAVE_ERR:
				SI_PRINT(("AXI slave error"));
				ret = AXI_WRAP_STS_SLAVE_ERR;
				break;

			case AIELS_TIMEOUT:
				/* reset APB Bridge */
				OR_REG(sii->osh, &ai->resetctrl, AIRC_RESET);
				/* sync write */
				(void)R_REG(sii->osh, &ai->resetctrl);
				/* clear Reset bit */
				AND_REG(sii->osh, &ai->resetctrl, ~(AIRC_RESET));
				/* sync write */
				(void)R_REG(sii->osh, &ai->resetctrl);
				SI_PRINT(("AXI timeout"));
				if (R_REG(sii->osh, &ai->resetctrl) & AIRC_RESET) {
					SI_PRINT(("reset failed on wrapper 0x%04x", (uint32)ai));
					g_disable_backplane_logs = TRUE;
				}
				ret = AXI_WRAP_STS_TIMEOUT;
				break;

			case AIELS_DECODE:
				SI_PRINT(("AXI decode error"));
				ret = AXI_WRAP_STS_DECODE_ERR;
				break;
			default:
				ASSERT(0);	/* should be impossible */
		}

		SI_PRINT(("\tCoreID: %x\n", coreid));
		SI_PRINT(("\t errlog: lo 0x%08x, hi 0x%08x, id 0x%08x, flags 0x%08x"
			", status 0x%08x\n",
			errlog_lo, errlog_hi, errlog_id, errlog_flags,
			errlog_status));
	}

end:
#if defined(BCM_BACKPLANE_TIMEOUT)
	if (axi_error && (ret != AXI_WRAP_STS_NONE)) {
		axi_error->error = ret;
		axi_error->coreid = coreid;
		axi_error->errlog_lo = errlog_lo;
		axi_error->errlog_hi = errlog_hi;
		axi_error->errlog_id = errlog_id;
		axi_error->errlog_flags = errlog_flags;
		axi_error->errlog_status = errlog_status;
		sih->err_info->count++;

		if (sih->err_info->count == SI_MAX_ERRLOG_SIZE) {
			sih->err_info->count = SI_MAX_ERRLOG_SIZE - 1;
			SI_PRINT(("AXI Error log overflow\n"));
		}
	}
#endif /* BCM_BACKPLANE_TIMEOUT */

	if (restore_core) {
		if (ai_setcoreidx(sih, current_coreidx) == NULL) {
			/* Unable to set the core */
			return ID32_INVALID;
		}
	}

	return ret;
}
#endif /* AXI_TIMEOUTS || BCM_BACKPLANE_TIMEOUT */

/*
 * This API polls all slave wrappers for errors and returns bit map of
 * all reported errors.
 * return - bit map of
 *	AXI_WRAP_STS_NONE
 *	AXI_WRAP_STS_TIMEOUT
 *	AXI_WRAP_STS_SLAVE_ERR
 *	AXI_WRAP_STS_DECODE_ERR
 *	AXI_WRAP_STS_PCI_RD_ERR
 *	AXI_WRAP_STS_WRAP_RD_ERR
 *	AXI_WRAP_STS_SET_CORE_FAIL
 * On timeout detection, correspondign bridge will be reset to
 * unblock the bus.
 * Error reported in each wrapper can be retrieved using the API
 * si_get_axi_errlog_info()
 */
uint32
ai_clear_backplane_to(si_t *sih)
{
	uint32 ret = 0;
#if defined(AXI_TIMEOUTS) || defined(BCM_BACKPLANE_TIMEOUT)

	si_info_t *sii = SI_INFO(sih);
	aidmp_t *ai;
	uint32 i;
	axi_wrapper_t * axi_wrapper = sii->axi_wrapper;

#ifdef BCM_BACKPLANE_TIMEOUT
	uint32 prev_value = 0;
	osl_t *osh = sii->osh;
	uint32 cfg_reg = 0;
	uint32 offset = 0;

	if ((sii->axi_num_wrappers == 0) || (!PCIE(sii)))
#else
	if (sii->axi_num_wrappers == 0)
#endif // endif
	{
		SI_VMSG((" %s, axi_num_wrappers:%d, Is_PCIE:%d, BUS_TYPE:%d, ID:%x\n",
			__FUNCTION__, sii->axi_num_wrappers, PCIE(sii),
			BUSTYPE(sii->pub.bustype), sii->pub.buscoretype));
		return AXI_WRAP_STS_NONE;
	}

#ifdef BCM_BACKPLANE_TIMEOUT
	/* Save and restore wrapper access window */
	if (BUSTYPE(sii->pub.bustype) == PCI_BUS) {
		if (PCIE_GEN1(sii)) {
			cfg_reg = PCI_BAR0_WIN2;
			offset = PCI_BAR0_WIN2_OFFSET;
		} else if (PCIE_GEN2(sii)) {
			cfg_reg = PCIE2_BAR0_CORE2_WIN2;
			offset = PCIE2_BAR0_CORE2_WIN2_OFFSET;
		}
		else {
			ASSERT(!"!PCIE_GEN1 && !PCIE_GEN2");
		}

		prev_value = OSL_PCI_READ_CONFIG(osh, cfg_reg, 4);

		if (prev_value == ID32_INVALID) {
			si_axi_error_t * axi_error =
				sih->err_info ?
					&sih->err_info->axi_error[sih->err_info->count] :
					NULL;

			SI_PRINT(("%s, PCI_BAR0_WIN2 - %x\n", __FUNCTION__, prev_value));
			if (axi_error) {
				axi_error->error = ret = AXI_WRAP_STS_PCI_RD_ERR;
				axi_error->errlog_lo = cfg_reg;
				sih->err_info->count++;

				if (sih->err_info->count == SI_MAX_ERRLOG_SIZE) {
					sih->err_info->count = SI_MAX_ERRLOG_SIZE - 1;
					SI_PRINT(("AXI Error log overflow\n"));
				}
			}

			return ret;
		}
	}
#endif /* BCM_BACKPLANE_TIMEOUT */

	for (i = 0; i < sii->axi_num_wrappers; ++i) {
		uint32 tmp;

		if (axi_wrapper[i].wrapper_type != AI_SLAVE_WRAPPER) {
			continue;
		}

#ifdef BCM_BACKPLANE_TIMEOUT
		if (BUSTYPE(sii->pub.bustype) == PCI_BUS) {
			/* Set BAR0_CORE2_WIN2 to bridge wapper base address */
			OSL_PCI_WRITE_CONFIG(osh,
				cfg_reg, 4, axi_wrapper[i].wrapper_pa);

			/* set AI to BAR0 + Offset corresponding to Gen1 or gen2 */
			ai = (aidmp_t *) (DISCARD_QUAL(sii->curmap, uint8) + offset);
		}
		else
#endif /* BCM_BACKPLANE_TIMEOUT */
		{
			ai = (aidmp_t *)(uintptr) axi_wrapper[i].wrapper_pa;
		}

		tmp = ai_clear_backplane_to_per_core(sih, axi_wrapper[i].cid, 0,
			DISCARD_QUAL(ai, void));

		ret |= tmp;
	}

#ifdef BCM_BACKPLANE_TIMEOUT
	/* Restore the initial wrapper space */
	if (prev_value) {
		OSL_PCI_WRITE_CONFIG(osh, cfg_reg, 4, prev_value);
	}
#endif /* BCM_BACKPLANE_TIMEOUT */

#endif /* AXI_TIMEOUTS || BCM_BACKPLANE_TIMEOUT */

	return ret;
}

uint
ai_num_slaveports(si_t *sih, uint coreidx)
{
	si_info_t *sii = SI_INFO(sih);
	si_cores_info_t *cores_info = (si_cores_info_t *)sii->cores_info;
	uint32 cib;

	cib = cores_info->cib[coreidx];
	return ((cib & CIB_NSP_MASK) >> CIB_NSP_SHIFT);
}

#ifdef UART_TRAP_DBG
void
ai_dump_APB_Bridge_registers(si_t *sih)
{
aidmp_t *ai;
si_info_t *sii = SI_INFO(sih);

	ai = (aidmp_t *) sii->br_wrapba_pa[0];
	printf("APB Bridge 0\n");
	printf("lo 0x%08x, hi 0x%08x, id 0x%08x, flags 0x%08x",
		R_REG(sii->osh, &ai->errlogaddrlo),
		R_REG(sii->osh, &ai->errlogaddrhi),
		R_REG(sii->osh, &ai->errlogid),
		R_REG(sii->osh, &ai->errlogflags));
	printf("\n status 0x%08x\n", R_REG(sii->osh, &ai->errlogstatus));
}
#endif /* UART_TRAP_DBG */

#ifdef DONGLEBUILD
/* XXX:
 * this is not declared as static const, although that is the right thing to do
 * reason being if declared as static const, compile/link process would that in
 * read only section...
 * currently this code/array is used to identify the registers which are dumped
 * during trap processing
 * and usually for the trap buffer, .rodata buffer is reused,  so for now just static
*/
static uint32 wrapper_offsets_to_dump[] = {
	OFFSETOF(aidmp_t, ioctrl),
	OFFSETOF(aidmp_t, iostatus),
	OFFSETOF(aidmp_t, resetctrl),
	OFFSETOF(aidmp_t, resetstatus),
	OFFSETOF(aidmp_t, errlogctrl),
	OFFSETOF(aidmp_t, errlogdone),
	OFFSETOF(aidmp_t, errlogstatus),
	OFFSETOF(aidmp_t, errlogaddrlo),
	OFFSETOF(aidmp_t, errlogaddrhi),
	OFFSETOF(aidmp_t, errlogid),
	OFFSETOF(aidmp_t, errloguser),
	OFFSETOF(aidmp_t, errlogflags)};

static uint32*
BCMRAMFN(ai_wrapper_dump_binary_one)(si_info_t *sii, uint32 *p32, uint32 wrap_ba)
{
	uint i;
	uint32 *addr;

	for (i = 0; i < ARRAYSIZE(wrapper_offsets_to_dump); i++) {
		addr = (uint32 *)(wrap_ba + wrapper_offsets_to_dump[i]);
		*p32++ = (uint32)addr;
		*p32++ = R_REG(sii->osh, addr);
	}
	return p32;
}

uint32
ai_wrapper_dump_binary(si_t *sih, uchar *p)
{
	uint32 *p32 = (uint32 *)p;
	uint32 i;
	si_info_t *sii = SI_INFO(sih);

	for (i = 0; i < sii->axi_num_wrappers; i++) {
		p32 = ai_wrapper_dump_binary_one(sii, p32, sii->axi_wrapper[i].wrapper_pa);
	}
	return 0;
}

bool
ai_check_enable_backplane_log(si_t *sih)
{
#if defined(AXI_TIMEOUTS) || defined(BCM_BACKPLANE_TIMEOUT)
	if (g_disable_backplane_logs) {
		return FALSE;
	}
	else {
		return TRUE;
	}
#else /*  (AXI_TIMEOUTS) || defined (BCM_BACKPLANE_TIMEOUT) */
	return FALSE;
#endif /*  (AXI_TIMEOUTS) || defined (BCM_BACKPLANE_TIMEOUT) */
}
#endif /* DONGLEBUILD */
