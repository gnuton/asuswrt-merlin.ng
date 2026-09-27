/*
 * OTP support.
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: bcmotp.h,v 13.15.2.13 2009/04/30 22:42:14 Exp $
 */

#ifndef	_bcmotp_h_
#define	_bcmotp_h_

/* OTP regions */
#define OTP_HW_RGN	1
#define OTP_SW_RGN	2
#define OTP_CI_RGN	4
#define OTP_FUSE_RGN	8

/* OTP Size */
#define OTP_SZ_MAX		(6144/8)	/* maximum bytes in one CIS */

/* Fixed size subregions sizes in words */
#define OTPGU_CI_SZ		2

/* OTP usage */
#define OTP4325_FM_DISABLED_OFFSET	188


/* Exported functions */
extern int	otp_status(void *oh);
extern int	otp_size(void *oh);
extern uint16	otp_read_bit(void *oh, uint offset);
extern void*	otp_init(si_t *sih);
extern int	otp_read_region(si_t *sih, int region, uint16 *data, uint *wlen);
extern int	otp_nvread(void *oh, char *data, uint *len);
#ifdef BCMNVRAMW
extern int	otp_write_region(si_t *sih, int region, uint16 *data, uint wlen);
extern int	otp_cis_append_region(si_t *sih, int region, char *vars, int count);
extern int	otp_lock(si_t *sih);
extern int	otp_nvwrite(void *oh, uint16 *data, uint wlen);
#endif /* BCMNVRAMW */

#if defined(WLTEST)
extern int	otp_dump(void *oh, int arg, char *buf, uint size);
extern int	otp_dumpstats(void *oh, int arg, char *buf, uint size);
#endif 

#if defined(BCMNVRAMW)
#define otp_write_rde(oh, rde, bit, val)	ipxotp_write_rde(oh, rde, bit, val)
extern int	ipxotp_write_rde(void *oh, int rde, uint bit, uint val);
#endif 

#endif /* _bcmotp_h_ */
