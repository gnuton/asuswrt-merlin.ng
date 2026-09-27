/*
 * Broadcom SiliconBackplane chipcommon serial flash interface
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: sflash.h,v 13.10 2008/03/04 22:17:11 Exp $
 */

#ifndef _sflash_h_
#define _sflash_h_

#include <typedefs.h>
#include <sbchipc.h>

struct sflash {
	uint blocksize;		/* Block size */
	uint numblocks;		/* Number of blocks */
	uint32 type;		/* Type */
	uint size;		/* Total size in bytes */
};

/* Utility functions */
extern int sflash_poll(si_t *sih, chipcregs_t *cc, uint offset);
extern int sflash_read(si_t *sih, chipcregs_t *cc,
                       uint offset, uint len, uchar *buf);
extern int sflash_write(si_t *sih, chipcregs_t *cc,
                        uint offset, uint len, const uchar *buf);
extern int sflash_erase(si_t *sih, chipcregs_t *cc, uint offset);
extern int sflash_commit(si_t *sih, chipcregs_t *cc,
                         uint offset, uint len, const uchar *buf);
extern struct sflash *sflash_init(si_t *sih, chipcregs_t *cc);

#endif /* _sflash_h_ */
