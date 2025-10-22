/*
 * Misc useful routines to access NIC local SROM/OTP .
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
 * $Id: bcmsrom.h 821234 2023-02-06 14:16:52Z $
 */

#ifndef	_bcmsrom_h_
#define	_bcmsrom_h_

#include <typedefs.h>
#include <osl_decl.h>
#include <siutils.h>

#include <bcmsrom_fmt.h>

typedef struct varbuf {
	char *base;		/* pointer to buffer base */
	char *buf;		/* pointer to current position */
	unsigned int size;	/* current (residual) size in bytes */
} varbuf_t;

typedef struct srom_info {
	char *_srom_vars;
	bool is_caldata_prsnt;
} srom_info_t;

/* */
enum nvram_line_devpath_type_e {
	NVRAM_DEVPATH_NONE,
	NVRAM_DEVPATH_SB,
	NVRAM_DEVPATH_PCIE,
	NVRAM_DEVPATH_COMPACT
};

/* Prototypes */
extern int srom_var_init(si_t *sih, enum bustype_e bustype, void *curmap, osl_t *osh,
                         char **vars, uint *count);
extern void srom_var_deinit(si_t *sih);

extern int srom_read(si_t *sih, enum bustype_e bustype, void *curmap, osl_t *osh,
                     uint byteoff, uint nbytes, uint16 *buf,
                     bool check_crc);

extern int srom_write(si_t *sih, enum bustype_e bustype, void *curmap, osl_t *osh,
                      uint byteoff, uint nbytes, uint16 *buf);

extern int srom_write_short(si_t *sih, enum bustype_e bustype, void *curmap, osl_t *osh,
                            uint byteoff, uint16 value);
extern int srom_otp_cisrwvar(si_t *sih, osl_t *osh, char *vars, int *count);
extern int srom_otp_write_region_crc(si_t *sih, uint nbytes, uint16* buf16, bool write);

/* parse standard PCMCIA cis, normally used by SB/PCMCIA/SDIO/SPI/OTP
 *   and extract from it into name=value pairs
 */
extern int srom_probe_boardtype(si_t *sih, uint8 *pcis[], uint ciscnt);
extern int srom_parsecis(si_t *sih, osl_t *osh, uint8 **pcis, uint ciscnt,
                         char **vars, uint *count);
extern int _initvars_srom_pci_caldata(si_t *sih, uint16 *srom, uint32 sromrev);
extern void srom_vars_update(char **vars);
extern void srom_set_sromvars(char *vars);
extern char * srom_get_sromvars(void);
extern srom_info_t * srom_info_init(osl_t *osh);
extern int get_srom_pci_caldata_size(uint32 sromrev);
extern uint32 get_srom_size(uint32 sromrev);

/* Return sprom size in 16-bit words */
extern uint srom_size(si_t *sih, osl_t *osh);

extern bool srom_caldata_prsnt(si_t *sih);
extern int srom_get_caldata(si_t *sih, uint16 *srom);
extern enum nvram_line_devpath_type_e srom_get_nvram_line_devpath_type(const char *nvram_line);
extern int initvars_srom_pci(si_t *sih, void *curmap, char **vars, uint *count);
extern int sprom_read_pci(osl_t *osh, si_t *sih, uint16 *sprom, uint wordoff, uint16 *buf,
		uint nwords, bool check_crc);
extern uint8* srom_offset(si_t *sih, void *curmap);

#endif	/* _bcmsrom_h_ */
