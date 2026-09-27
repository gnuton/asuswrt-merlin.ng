/*
 * HND SiliconBackplane chipcommon support.
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: hndchipc.h,v 13.6 2008/03/28 19:06:09 Exp $
 */

#ifndef _hndchipc_h_
#define _hndchipc_h_

typedef void (*si_serial_init_fn)(void *regs, uint irq, uint baud_base, uint reg_shift);

extern void si_serial_init(si_t *sih, si_serial_init_fn add);

extern void *hnd_jtagm_init(si_t *sih, uint clkd, bool exttap);
extern void hnd_jtagm_disable(osl_t *osh, void *h);
extern uint32 jtag_rwreg(osl_t *osh, void *h, uint32 ir, uint32 dr);

typedef	void (*cc_isr_fn)(void* cbdata, uint32 ccintst);

extern bool si_cc_register_isr(si_t *sih, cc_isr_fn isr, uint32 ccintmask, void *cbdata);
extern void si_cc_isr(si_t *sih, chipcregs_t *regs);

#endif /* _hndchipc_h_ */
