/*
 * MIPS 74k definitions
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: mips74k_core.h,v 13.1.4.1 2009/12/09 19:03:37 Exp $
 */

#ifndef	_mips74k_core_h_
#define	_mips74k_core_h_

#include <mipsinc.h>

#ifndef _LANGUAGE_ASSEMBLY

/* cpp contortions to concatenate w/arg prescan */
#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif	/* PAD */

typedef volatile struct {
	uint32	corecontrol;
	uint32	exceptionbase;
	uint32	PAD[1];
	uint32	biststatus;
	uint32	intstatus;
	uint32	intmask[6];
	uint32	nmimask;
	uint32	PAD[4];
	uint32	gpioselect;
	uint32	gpiooutput;
	uint32	gpioenable;
	uint32	PAD[101];
	uint32	clkcontrolstatus;
} mips74kregs_t;

/* Core specific status flags */
#define SISF_CHG_CLK_OTF_PRESENT        0x0001

#endif	/* _LANGUAGE_ASSEMBLY */

#endif	/* _mips74k_core_h_ */
