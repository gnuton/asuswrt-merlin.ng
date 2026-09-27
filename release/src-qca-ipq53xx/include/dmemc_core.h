/*
 * BCM47XX Denali DDR1/DDR2 and SDR/DDR1 memory controlers.
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: dmemc_core.h,v 13.1.4.3 2008/10/31 21:26:25 Exp $
 */

#ifndef	_DMEMC_H
#define	_DMEMC_H

#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif	/* PAD */

#ifdef _LANGUAGE_ASSEMBLY

#define	DMEMC_CONTROL00		0x000
#define	DMEMC_CONTROL01		0x004
#define	DMEMC_CONTROL02		0x008
#define	DMEMC_CONTROL03		0x00c
#define	DMEMC_CONTROL04		0x010
#define	DMEMC_CONTROL05		0x014
#define	DMEMC_CONTROL06		0x018
#define	DMEMC_CONTROL07		0x01c
#define	DMEMC_CONTROL08		0x020
#define	DMEMC_CONTROL09		0x024
#define	DMEMC_CONTROL10		0x028
#define	DMEMC_CONTROL11		0x02c
#define	DMEMC_CONTROL12		0x030
#define	DMEMC_CONTROL13		0x034
#define	DMEMC_CONTROL14		0x038
#define	DMEMC_CONTROL15		0x03c
#define	DMEMC_CONTROL16		0x040
#define	DMEMC_CONTROL17		0x044
#define	DMEMC_CONTROL18		0x048
#define	DMEMC_CONTROL19		0x04c
#define	DMEMC_CONTROL20		0x050
#define	DMEMC_CONTROL21		0x054
#define	DMEMC_CONTROL22		0x058
#define	DMEMC_CONTROL23		0x05c
#define	DMEMC_CONTROL24		0x060
#define	DMEMC_CONTROL25		0x064
#define	DMEMC_CONTROL26		0x068
#define	DMEMC_CONTROL27		0x06c
#define	DMEMC_CONTROL28		0x070
#define	DMEMC_CONTROL29		0x074
#define	DMEMC_CONTROL30		0x078
#define	DMEMC_CONTROL31		0x07c
#define	DMEMC_CONTROL32		0x080
#define	DMEMC_CONTROL33		0x084
#define	DMEMC_CONTROL34		0x088
#define	DMEMC_CONTROL35		0x08c
#define	DMEMC_CONTROL36		0x090
#define	DMEMC_CONTROL37		0x094
#define	DMEMC_CONTROL38		0x098
#define	DMEMC_CONTROL39		0x09c
#define	DMEMC_CONTROL40		0x0a0
#define	DMEMC_CONTROL41		0x0a4
#define	DMEMC_CONTROL42		0x0a8
#define	DMEMC_CONTROL43		0x0ac
#define	DMEMC_CONTROL44		0x0b0
#define	DMEMC_CONTROL45		0x0b4
#define	DMEMC_CONTROL46		0x0b8
#define	DMEMC_CONTROL47		0x0bc
#define	DMEMC_CONTROL48		0x0c0
#define	DMEMC_CONTROL49		0x0c4
#define	DMEMC_CONTROL50		0x0c8
#define	DMEMC_CONTROL51		0x0cc
#define	DMEMC_CONTROL52		0x0d0
#define	DMEMC_CONTROL53		0x0d4

#define	DMEMC_CLK_CTL_ST	0x1e0
#define	DMEMC_DDR_CTRL		0x1e4
#define	DMEMC_STAT		0x1f0

#define DMEMC_PVTGROUPA 	0x400
#define DMEMC_PVTGROUPB 	0x404
#define DMEMC_PVTGROUPC 	0x408
#define DMEMC_PVTGROUPE 	0x40c
#define DMEMC_PVTGROUPF 	0x410
#define DMEMC_PVTGROUPG 	0x414
#define DMEMC_PVTGROUPH 	0x418
#define DMEMC_PVTGROUPI 	0x41c
#define DMEMC_PVTGROUPJ 	0x420

#define DMEMC_GPIOSEL	        0x800
#define DMEMC_GPIOOUTEN	0x804

#else	/* !_LANGUAGE_ASSEMBLY */

#define	DMEMC_MAXREG		50
#define	DMEMC_PVTREGS		9

/* DMEMC core registers */
typedef volatile struct dmemcregs {
	uint32	control[DMEMC_MAXREG];
	uint32	PAD[205];
	uint32	pvtgroup[DMEMC_PVTREGS];	/* 0x400 */
	uint32	PAD[247];
	uint32	gpiosel;			/* 0x800 */
	uint32	gpioouten;			/* 0x804 */
} dmemcregs_t;


#define	DMEMS_MAXREG		53

/* DMEMC core registers */
typedef volatile struct dmemsregs {
	uint32	control[DMEMS_MAXREG];
	uint32	PAD[66];
	uint32	clk_ctl_st;			/* 0x1e0 */
	uint32	ddr_ctrl;			/* 0x1e4 */
	uint32	PAD[2];
	uint32	stat;				/* 0x1f0 */
	uint32	PAD[386];
	uint32	gpiosel;			/* 0x800 */
	uint32	gpioouten;			/* 0x804 */
} dmemsregs_t;

#endif	/* _LANGUAGE_ASSEMBLY */

#define	DMEMC_TABLE_END		0xffffffff

/* Bits in control3 */
#define	DMC03_BIST_DATA		0x01000000
#define	DMC03_BIST_ADDR		0x00010000

/* Bits in control4 */
#define	DMC04_DLLLOCK		0x01000000
#define	DMC04_DDR2		0x00010000
#define	DMC04_BIST_GO		0x00000001

/* Bits in control09 */
#define	DMC09_START		0x01000000

/* Bits in control11 */
#define	DMC11_BIST_DATA_OK	0x01000000
#define	DMC11_BIST_ADDR_OK	0x02000000

/* Bits in control19 */
#define	DMC19_ADDRSP_MASK	0x1f000000
#define	DMC19_ADDRSP_SHIFT	24

/* Bits in control23 */
#define	DMC23_INTMASK_MASK	0xff000000
#define	DMC23_INTMASK_SHIFT	24
#define	DMC23_INTACK_MASK	0x0000007f

/* Bits in control24 */
#define	DMC24_INTSTAT_MASK	0x000000ff

/* Interrupt bits (in control24.status, control23.int_ack and
 * control23.int_mask)
 */
#define	DM_INT_SINGLE_BAD	0x01
#define	DM_INT_MULTI_BAD	0x02
#define	DM_INT_CMD_ERR		0x04
#define	DM_INT_DATA_ERR		0x08
#define	DM_INT_INIT_DONE	0x10
#define	DM_INT_BIST_DONE	0x20
#define	DM_INT_DLL_UNLOCK	0x40
#define	DM_INT_ANY		0x80

/* Stat bits */
#define	DM_STAT_DDR2_CAP	0x0400
#define	DM_STAT_DDR1_CAP	0x0200
#define	DM_STAT_SDR_CAP		0x0100
#define	DM_STAT_DDR2		0x0004
#define	DM_STAT_DDR1		0x0002
#define	DM_STAT_SDR		0x0001
#define	DM_STAT_MASK		0x0007

#endif	/* _SBMEMC_H */
