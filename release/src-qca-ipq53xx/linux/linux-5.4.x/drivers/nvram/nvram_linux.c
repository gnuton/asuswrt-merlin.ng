/*
 * NVRAM variable manipulation (Linux kernel half)
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: nvram_linux.c,v 1.1 2007/06/08 07:38:05 arthur Exp $
 */
#define ASUS_NVRAM
#define WL_NVRAM	/* wear-levelling nvram */
#define COMPRESS_NVRAM

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/seq_file.h>

#include "../../../../include/bcmnvram.h"

static int nv_locked = 0;
/* If UBOOT_CFG_ENV_SIZE is defined as non-zero value,
 * we have to skip first UBOOT_CFG_ENV_SIZE bytes of MTD partition.
 */
#if !defined(UBOOT_CFG_ENV_SIZE)
#error no UBOOT_CFG_ENV_SIZE defined! check header file!
#endif

#if defined(WL_NVRAM)
#define RESERVED_BLOCK_SIZE	(UBOOT_CFG_ENV_SIZE)
#else	/* !WL_NVRAM */
#define RESERVED_BLOCK_SIZE	0
#endif	/* WL_NVRAM */

#ifdef ASUS_NVRAM
#	include <linux/mm.h>
#	include <linux/version.h>
#	if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#		define mem_map_reserve(p)		set_bit(PG_reserved, &((p)->flags))
#		define mem_map_unreserve(p)		clear_bit(PG_reserved, &((p)->flags))
#	else
#	include <linux/wrapper.h>
#	endif
#	if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
#	define MOD_INC_USE_COUNT
#	define MOD_DEC_USE_COUNT
#	endif
#include <asm/cacheflush.h>								// function decl. of flush_cache_all()
#include <linux/proc_fs.h>								// create_proc_read_entry()
#else	// !ASUS_NVRAM
#	include <linux/wrapper.h>
#endif	// ASUS_NVRAM

#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mtd/mtd.h>
#include <linux/nls.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
#include <mtd/mtd-abi.h>
#define MTD_OOB_RAW MTD_OPS_RAW
#endif

#ifdef ASUS_NVRAM
	// Only mips and sh architecture have this file
#else	// !ASUS_NVRAM
#include <asm/addrspace.h>
#endif	// ASUS_NVRAM

#include <asm/io.h>
#include <asm/uaccess.h>

#ifdef ASUS_NVRAM
#include <nvram/typedefs.h>
#include <nvram/bcmendian.h>
#include <nvram/bcmnvram.h>
#include <nvram/bcmutils.h>
#ifdef COMPRESS_NVRAM
#include <linux/zlib.h>
#include <linux/zutil.h>
#endif

#include "nvram.c"

#else	// !ASUS_NVRAM
#include <typedefs.h>
#include <bcmendian.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <sbconfig.h>
#include <sbchipc.h>
#include <sbutils.h>
#include <sflash.h>
#endif	// ASUS_NVRAM

/* In BSS to minimize text size and page aligned so it can be mmap()-ed */
#ifdef ASUS_NVRAM
static uint8_t *nvram_buf;
#else	// !ASUS_NVRAM
static uint8_t nvram_buf[NVRAM_SPACE] __attribute__((aligned(PAGE_SIZE)));
#endif	// ASUS_NVRAM

#define READ_BUF_SIZE	(NVRAM_SPACE + 1)
static DEFINE_MUTEX(read_buf_lock);	/* protect read_buf */
static char *read_buf = NULL;
static uint8_t *commit_buf = NULL;
static int nvram_idx = 0;

#ifdef WL_NVRAM
static uint8_t *wblk_buf = NULL;
static size_t rsv_blk_size = RESERVED_BLOCK_SIZE;	/* Aligned to nvram_mtd->writesize and step_unit both. */
static size_t step_unit = STEP_UNIT;
#endif

#ifdef ASUS_NVRAM
static void *sbh;
static struct class *s_nvram_class = NULL;
static struct device *s_nvram_device = NULL;
#else	// !ASUS_NVRAM
#define CFE_UPDATE 1 // added by Chen-I for mac/regulation update
#endif	// ASUS_NVRAM

#define MTD_NVRAM_NAME	"nvram"
#define NVRAM_DRV_NAME	MTD_NVRAM_NAME

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
#       define MTD_ERASE(mtd, args...) mtd_erase(mtd, args)
#       define MTD_POINT(mtd, a,b,c,d) mtd_point(mtd, a,b,c, (u_char **)(d))
#       define MTD_UNPOINT(mtd, arg) mtd_unpoint(mtd, (u_char *)arg)
#       define MTD_READ(mtd, args...) mtd_read(mtd, args)
#       define MTD_WRITE(mtd, args...) mtd_write(mtd, args)
#       define MTD_READV(mtd, args...) mtd_readv(mtd, args)
#       define MTD_WRITEV(mtd, args...) mtd_writev(mtd, args)
#       define MTD_READECC(mtd, args...) mtd_read_ecc(mtd, args)
#       define MTD_WRITEECC(mtd, args...) mtd_write_ecc(mtd, args)
#       define MTD_READOOB(mtd, args...) mtd_read_oob(mtd, args)
#       define MTD_WRITEOOB(mtd, args...) mtd_write_oob(mtd, args)
#       define MTD_SYNC(mtd) do { if (mtd->sync) (*(mtd->_sync))(mtd);  } while (0)
#       define MTD_UNLOCK(mtd, args...) do { if (mtd->_unlock) (*(mtd->_unlock))(mtd, args);  } while (0)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
/* Following macros are removed from linux/mtd/mtd.h since linux-2.6.0
 * mtd_info->funcptr is renamed as mtd_info->_funcptr since linux-3.4.
 */
#       define MTD_ERASE(mtd, args...) (*(mtd->_erase))(mtd, args)
#       define MTD_POINT(mtd, a,b,c,d) (*(mtd->_point))(mtd, a,b,c, (u_char **)(d))
#       define MTD_UNPOINT(mtd, arg) (*(mtd->_unpoint))(mtd, (u_char *)arg)
#       define MTD_READ(mtd, args...) (*(mtd->_read))(mtd, args)
#       define MTD_WRITE(mtd, args...) (*(mtd->_write))(mtd, args)
#       define MTD_READV(mtd, args...) (*(mtd->_readv))(mtd, args)
#       define MTD_WRITEV(mtd, args...) (*(mtd->_writev))(mtd, args)
#       define MTD_READECC(mtd, args...) (*(mtd->_read_ecc))(mtd, args)
#       define MTD_WRITEECC(mtd, args...) (*(mtd->_write_ecc))(mtd, args)
#       define MTD_READOOB(mtd, args...) (*(mtd->_read_oob))(mtd, args)
#       define MTD_WRITEOOB(mtd, args...) (*(mtd->_write_oob))(mtd, args)
#       define MTD_SYNC(mtd) do { if (mtd->sync) (*(mtd->_sync))(mtd);  } while (0)
#       define MTD_UNLOCK(mtd, args...) do { if (mtd->_unlock) (*(mtd->_unlock))(mtd, args);  } while (0)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
// Following macros are removed from linux/mtd/mtd.h since linux-2.6.0
#       define MTD_ERASE(mtd, args...) (*(mtd->erase))(mtd, args)
#       define MTD_POINT(mtd, a,b,c,d) (*(mtd->point))(mtd, a,b,c, (u_char **)(d))
#       define MTD_UNPOINT(mtd, arg) (*(mtd->unpoint))(mtd, (u_char *)arg)
#       define MTD_READ(mtd, args...) (*(mtd->read))(mtd, args)
#       define MTD_WRITE(mtd, args...) (*(mtd->write))(mtd, args)
#       define MTD_READV(mtd, args...) (*(mtd->readv))(mtd, args)
#       define MTD_WRITEV(mtd, args...) (*(mtd->writev))(mtd, args)
#       define MTD_READECC(mtd, args...) (*(mtd->read_ecc))(mtd, args)
#       define MTD_WRITEECC(mtd, args...) (*(mtd->write_ecc))(mtd, args)
#       define MTD_READOOB(mtd, args...) (*(mtd->read_oob))(mtd, args)
#       define MTD_WRITEOOB(mtd, args...) (*(mtd->write_oob))(mtd, args)
#       define MTD_SYNC(mtd) do { if (mtd->sync) (*(mtd->sync))(mtd);  } while (0)
#       define MTD_UNLOCK(mtd, args...) do { if (mtd->unlock) (*(mtd->unlock))(mtd, args);  } while (0)
#endif

#ifdef CFE_UPDATE
#include <sbextif.h>

extern void bcm947xx_watchdog_disable(void);

#define CFE_SPACE       4*1024
#define CFE_NVRAM_START 0x30000
#define CFE_NVRAM_END   0x30fff
#define CFE_NVRAM_SPACE 192*1024
static struct mtd_info *cfe_mtd = NULL;
static char *CFE_NVRAM_PREFIX="asuscfe";
static char *CFE_NVRAM_COMMIT="asuscfecommit";
static char *CFE_NVRAM_WATCHDOG="asuscfewatchdog";
char *cfe_buf;// = NULL;
struct nvram_header *cfe_nvram_header; // = NULL;
#endif

static DEFINE_MUTEX(nvram_ioctl_lock);

#ifdef MODULE

#ifdef ASUS_NVRAM
#else	// !ASUS_NVRAM
#define early_nvram_get(name) nvram_get(name)
#endif	// ASUS_NVRAM

#define NLS_XFR 1
#ifdef NLS_XFR
				/* added by Jiahao for WL500gP */
static char *NLS_NVRAM_U2C="asusnlsu2c";
static char *NLS_NVRAM_C2U="asusnlsc2u";
__u16 unibuf[1024];
char codebuf[1024];
char tmpbuf[1024];

void				/* added by Jiahao for WL500gP */
asusnls_u2c(char *name)
{
	char *codepage;
	char *xfrstr;
	struct nls_table *nls;
	int ret, len=0;

	strcpy(codebuf, name);
	codepage=codebuf+strlen(NLS_NVRAM_U2C);
	if((xfrstr=strchr(codepage, '_')))
	{
		*xfrstr= '\0';
		xfrstr++;
		/* debug message, start */
/*
		printk("%s, xfr from utf8 to %s\n", xfrstr, codepage);
		int j;
		printk("utf8:    %d, ", strlen(xfrstr));
		for(j=0;j<strlen(xfrstr);j++)
			printk("%X ", (unsigned char)xfrstr[j]);
		printk("\n");
*/
		/* debug message, end */
		nls=load_nls(codepage);
		if(!nls)
		{
			printk("NLS table is null!!\n");
		} else {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
			ret=utf8s_to_utf16s(xfrstr, (int)strlen(xfrstr), UTF16_HOST_ENDIAN, unibuf, sizeof(unibuf)/2);
#else
			ret=utf8s_to_utf16s(xfrstr, (int)strlen(xfrstr), unibuf);
#endif
			if (ret)
			{
				int i;
				len = 0;
				for (i = 0; (i < ret) && unibuf[i]; i++) {
					int charlen;
					charlen = nls->uni2char(unibuf[i], &name[len], NLS_MAX_CHARSET_SIZE);
					if (charlen > 0) {
						len += charlen;
					} else {
//						name[len++] = '?';
						strcpy(name, "");
						unload_nls(nls);
						return;
					}
				}
				name[len] = 0;
			}
			unload_nls(nls);
			/* debug message, start */
/*
			int i;
			printk("unicode: %d, ", ret);
			for (i=0;i<ret;i++)
				printk("%X ", unibuf[i]);
			printk("\n");
			printk("local:   %d, ", strlen(name));
			for (i=0;i<strlen(name);i++)
				printk("%X ", (unsigned char)name[i]);
			printk("\n");
			printk("local:   %s\n", name);
*/
			/* debug message, end */
			if(!len)
			{
				printk("can not xfr from utf8 to %s\n", codepage);
				strcpy(name, "");
			}
		}
	}
	else
	{
		strcpy(name, "");
	}
}

void					/* added by Jiahao for WL500gP */
asusnls_c2u(char *name)
{
	char *codepage;
	char *xfrstr;
	struct nls_table *nls;
	int ret;

	strcpy(codebuf, name);
	codepage=codebuf+strlen(NLS_NVRAM_C2U);
	if((xfrstr=strchr(codepage, '_')))
	{
		*xfrstr='\0';
		xfrstr++;

		/* debug message, start */
/*
		printk("%s, xfr from %s to utf8\n", xfrstr, codepage);
		printk("local:   %d, ", strlen(xfrstr));
		int j;
		for (j=0;j<strlen(xfrstr);j++)
			printk("%X ", (unsigned char)xfrstr[j]);
		printk("\n");
		printk("local:   %s\n", xfrstr);
*/
		/* debug message, end */

		strcpy(name, "");
		nls=load_nls(codepage);
		if(!nls)
		{
			printk("NLS table is null!!\n");
		} else
		{
			int charlen;
			int i;
			int len = strlen(xfrstr);
			for (i = 0; len && *xfrstr; i++, xfrstr += charlen, len -= charlen) {	/* string to unicode */
				charlen = nls->char2uni(xfrstr, len, &unibuf[i]);
				if (charlen < 1) {
//					unibuf[i] = 0x003f;	/* a question mark */
//					charlen = 1;
					strcpy(name ,"");
					unload_nls(nls);
					return;
				}
			}
			unibuf[i] = 0;
			ret=utf16s_to_utf8s(unibuf, i, UTF16_HOST_ENDIAN, name, 1024);	/* unicode to utf-8, 1024 is size of array unibuf */
			name[ret] = '\0';
			unload_nls(nls);
			/* debug message, start */
/*
			int k;
			printk("unicode: %d, ", i);
			for(k=0;k<i;k++)
				printk("%X ", unibuf[k]);
			printk("\n");
			printk("utf-8:    %s, %d, ", name, strlen(name));
			for (i=0;i<strlen(name);i++)
				printk("%X ", (unsigned char)name[i]);
			printk("\n");
*/
			/* debug message, end */
			if(!ret)
			{
				printk("can not xfr from %s to utf8\n", codepage);
				strcpy(name, "");
			}
		}
	}
	else
	{
		strcpy(name, "");
	}
}

#endif	// NLS_XFR

#else /* !MODULE */

#ifdef ASUS_NVRAM
#error  This nvram driver only support kernel module.
#else   // !ASUS_NVRAM
/* Global SB handle */
extern void *bcm947xx_sbh;
extern spinlock_t bcm947xx_sbh_lock;

/* Convenience */
#define sbh bcm947xx_sbh
#define sbh_lock bcm947xx_sbh_lock
#define KB * 1024
#define MB * 1024 * 1024

/* Probe for NVRAM header */
static void __init
early_nvram_init(void)
{
	struct nvram_header *header;
	chipcregs_t *cc;
	struct sflash *info = NULL;
	int i;
	uint32 base, off, lim;
	u32 *src, *dst;

	if ((cc = sb_setcore(sbh, SB_CC, 0)) != NULL) {
		base = KSEG1ADDR(SB_FLASH2);
		switch (readl(&cc->capabilities) & CAP_FLASH_MASK) {
		case PFLASH:
			lim = SB_FLASH2_SZ;
			break;

		case SFLASH_ST:
		case SFLASH_AT:
			if ((info = sflash_init(cc)) == NULL)
				return;
			lim = info->size;
			break;

		case FLASH_NONE:
		default:
			return;
		}
	} else {
		/* extif assumed, Stop at 4 MB */
		base = KSEG1ADDR(SB_FLASH1);
		lim = SB_FLASH1_SZ;
	}

	off = FLASH_MIN;
	while (off <= lim) {
		/* Windowed flash access */
		header = (struct nvram_header *) KSEG1ADDR(base + off - NVRAM_SPACE);
		if (header->magic == NVRAM_MAGIC)
			goto found;
		off <<= 1;
	}

	/* Try embedded NVRAM at 4 KB and 1 KB as last resorts */
	header = (struct nvram_header *) KSEG1ADDR(base + 4 KB);
	if (header->magic == NVRAM_MAGIC)
		goto found;

	header = (struct nvram_header *) KSEG1ADDR(base + 1 KB);
	if (header->magic == NVRAM_MAGIC)
		goto found;

	printk("early_nvram_init: NVRAM not found\n");
	return;

found:
	src = (u32 *) header;
	dst = (u32 *) nvram_buf;
	for (i = 0; i < sizeof(struct nvram_header); i += 4)
		*dst++ = *src++;
	for (; i < header->len && i < NVRAM_SPACE; i += 4)
		*dst++ = ltoh32(*src++);
}

/* Early (before mm or mtd) read-only access to NVRAM */
static char * __init
early_nvram_get(const char *name)
{
	char *var, *value, *end, *eq;

	if (!name)
		return NULL;

	/* Too early? */
	if (sbh == NULL)
		return NULL;

	if (!nvram_buf[0])
		early_nvram_init();

	/* Look for name=value and return value */
	var = &nvram_buf[sizeof(struct nvram_header)];
	end = nvram_buf + sizeof(nvram_buf) - 2;
	end[0] = end[1] = '\0';
	for (; *var; var = value + strlen(value) + 1) {
		if (!(eq = strchr(var, '=')))
			break;
		value = eq + 1;
		if ((eq - var) == strlen(name) && strncmp(var, name, (eq - var)) == 0)
			return value;
	}

	return NULL;
}

#endif	// ASUS_NVRAM
#endif /* !MODULE */

extern char * _nvram_get(const char *name);
extern int _nvram_set(const char *name, const char *value);
extern int _nvram_unset(const char *name);
extern int _nvram_getall(char *buf, int count);
extern int _nvram_commit(struct nvram_header *header);
extern int _nvram_init(void *sbh);
extern void _nvram_exit(void);

/* Globals */
static DEFINE_SPINLOCK(nvram_lock);
static DEFINE_MUTEX(nvram_sem);
static unsigned long nvram_offset = 0;
static int nvram_major = -1;

#ifdef ASUS_NVRAM
static struct proc_dir_entry *g_pdentry = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
//static devfs_handle_t nvram_handle = NULL;
#endif
#else	// !ASUS_NVRAM
//static devfs_handle_t nvram_handle = NULL;
#endif	// ASUS_NVRAM

static struct mtd_info *nvram_mtd = NULL;


#ifdef ASUS_NVRAM
// from src/shared/bcmutils.c
/*******************************************************************************
 * crc8
 *
 * Computes a crc8 over the input data using the polynomial:
 *
 *       x^8 + x^7 +x^6 + x^4 + x^2 + 1
 *
 * The caller provides the initial value (either CRC8_INIT_VALUE
 * or the previous returned value) to allow for processing of
 * discontiguous blocks of data.  When generating the CRC the
 * caller is responsible for complementing the final return value
 * and inserting it into the byte stream.  When checking, a final
 * return value of CRC8_GOOD_VALUE indicates a valid CRC.
 *
 * Reference: Dallas Semiconductor Application Note 27
 *   Williams, Ross N., "A Painless Guide to CRC Error Detection Algorithms",
 *     ver 3, Aug 1993, ross@guest.adelaide.edu.au, Rocksoft Pty Ltd.,
 *     ftp://ftp.rocksoft.com/clients/rocksoft/papers/crc_v3.txt
 *
 * ****************************************************************************
 */

static uint8 crc8_table[256] = {
    0x00, 0xF7, 0xB9, 0x4E, 0x25, 0xD2, 0x9C, 0x6B,
    0x4A, 0xBD, 0xF3, 0x04, 0x6F, 0x98, 0xD6, 0x21,
    0x94, 0x63, 0x2D, 0xDA, 0xB1, 0x46, 0x08, 0xFF,
    0xDE, 0x29, 0x67, 0x90, 0xFB, 0x0C, 0x42, 0xB5,
    0x7F, 0x88, 0xC6, 0x31, 0x5A, 0xAD, 0xE3, 0x14,
    0x35, 0xC2, 0x8C, 0x7B, 0x10, 0xE7, 0xA9, 0x5E,
    0xEB, 0x1C, 0x52, 0xA5, 0xCE, 0x39, 0x77, 0x80,
    0xA1, 0x56, 0x18, 0xEF, 0x84, 0x73, 0x3D, 0xCA,
    0xFE, 0x09, 0x47, 0xB0, 0xDB, 0x2C, 0x62, 0x95,
    0xB4, 0x43, 0x0D, 0xFA, 0x91, 0x66, 0x28, 0xDF,
    0x6A, 0x9D, 0xD3, 0x24, 0x4F, 0xB8, 0xF6, 0x01,
    0x20, 0xD7, 0x99, 0x6E, 0x05, 0xF2, 0xBC, 0x4B,
    0x81, 0x76, 0x38, 0xCF, 0xA4, 0x53, 0x1D, 0xEA,
    0xCB, 0x3C, 0x72, 0x85, 0xEE, 0x19, 0x57, 0xA0,
    0x15, 0xE2, 0xAC, 0x5B, 0x30, 0xC7, 0x89, 0x7E,
    0x5F, 0xA8, 0xE6, 0x11, 0x7A, 0x8D, 0xC3, 0x34,
    0xAB, 0x5C, 0x12, 0xE5, 0x8E, 0x79, 0x37, 0xC0,
    0xE1, 0x16, 0x58, 0xAF, 0xC4, 0x33, 0x7D, 0x8A,
    0x3F, 0xC8, 0x86, 0x71, 0x1A, 0xED, 0xA3, 0x54,
    0x75, 0x82, 0xCC, 0x3B, 0x50, 0xA7, 0xE9, 0x1E,
    0xD4, 0x23, 0x6D, 0x9A, 0xF1, 0x06, 0x48, 0xBF,
    0x9E, 0x69, 0x27, 0xD0, 0xBB, 0x4C, 0x02, 0xF5,
    0x40, 0xB7, 0xF9, 0x0E, 0x65, 0x92, 0xDC, 0x2B,
    0x0A, 0xFD, 0xB3, 0x44, 0x2F, 0xD8, 0x96, 0x61,
    0x55, 0xA2, 0xEC, 0x1B, 0x70, 0x87, 0xC9, 0x3E,
    0x1F, 0xE8, 0xA6, 0x51, 0x3A, 0xCD, 0x83, 0x74,
    0xC1, 0x36, 0x78, 0x8F, 0xE4, 0x13, 0x5D, 0xAA,
    0x8B, 0x7C, 0x32, 0xC5, 0xAE, 0x59, 0x17, 0xE0,
    0x2A, 0xDD, 0x93, 0x64, 0x0F, 0xF8, 0xB6, 0x41,
    0x60, 0x97, 0xD9, 0x2E, 0x45, 0xB2, 0xFC, 0x0B,
    0xBE, 0x49, 0x07, 0xF0, 0x9B, 0x6C, 0x22, 0xD5,
    0xF4, 0x03, 0x4D, 0xBA, 0xD1, 0x26, 0x68, 0x9F
};

uint8
hndcrc8(
	uint8 *pdata,	/* pointer to array of data to process */
	uint  nbytes,	/* number of input data bytes to process */
	uint8 crc	/* either CRC8_INIT_VALUE or previous return value */
)
{
	/* hard code the crc loop instead of using CRC_INNER_LOOP macro
	 * to avoid the undefined and unnecessary (uint8 >> 8) operation.
	 */
	while (nbytes-- > 0)
		crc = crc8_table[(crc ^ *pdata++) & 0xff];

	return crc;
}

#define NVRAM_DRIVER_VERSION	"0.07"

static int 
nvram_proc_show(struct seq_file *m, void *v)
{
	char type[32];

	if (nvram_mtd->type == MTD_NORFLASH)
		strcpy(type, "NORFLASH");
	else if (nvram_mtd->type == MTD_NANDFLASH)
		strcpy(type, "NANDFLASH");
	else if (nvram_mtd->type == MTD_DATAFLASH)
		strcpy(type, "DATAFLASH");
	else if (nvram_mtd->type == MTD_UBIVOLUME)
		strcpy(type, "UBIVOLUME");
	else
		sprintf(type, "Unknown type(%d)", nvram_mtd->type);

	seq_printf(m, "nvram driver : v" NVRAM_DRIVER_VERSION "\n");
	seq_printf(m, "nvram space  : 0x%x\n", NVRAM_SPACE);
	seq_printf(m, "nvram multiple: %d\n", NVRAM_MULTIPLE);
	seq_printf(m, "major number : %d\n", nvram_major);
	seq_printf(m, "MTD            \n");
	seq_printf(m, "  name       : %s\n", nvram_mtd->name);
	seq_printf(m, "  index      : %d\n", nvram_mtd->index);
	seq_printf(m, "  type       : %s\n", type);
	seq_printf(m, "  flags      : 0x%x\n", nvram_mtd->flags);
	seq_printf(m, "  size       : 0x%x\n", (unsigned int)nvram_mtd->size);
	seq_printf(m, "  erasesize  : 0x%x\n", nvram_mtd->erasesize);
	seq_printf(m, "  writesize  : 0x%x\n", nvram_mtd->writesize);
#ifdef WL_NVRAM
	seq_printf(m, "private      : 0x%x, 0x%x -> 0x%x +0x%zx, 0x%x %d\n",
			g_wlnv.last_commit_times, g_wlnv.cur_offset, g_wlnv.next_offset, step_unit,
			g_wlnv.avg_len, g_wlnv.may_erase_nexttime);
	seq_printf(m, "rsv_blk_size : 0x%zx\n", rsv_blk_size);
#endif	/* WL_NVRAM */

	return 0;
}

static int nvram_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvram_proc_show, NULL);
}

static const struct file_operations nvram_file_ops = {
	.open		= nvram_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif	// ASUS_NVRAM

#ifdef COMPRESS_NVRAM
/* reference from fs/jffs2/compr_zlib.c */
#define STREAM_END_SPACE 12

static DEFINE_MUTEX(deflate_mutex);
static DEFINE_MUTEX(inflate_mutex);
static z_stream inf_strm, def_strm;

#ifdef __KERNEL__ /* Linux-only */
#include <linux/vmalloc.h>
#include <linux/mutex.h>

static int __init alloc_workspaces(void)
{
	def_strm.workspace = vmalloc(zlib_deflate_workspacesize(MAX_WBITS,
							MAX_MEM_LEVEL));
	if (!def_strm.workspace)
		return -ENOMEM;

	//printk("Allocated %d bytes for deflate workspace\n",
	//	  zlib_deflate_workspacesize(MAX_WBITS, MAX_MEM_LEVEL));
	inf_strm.workspace = vmalloc(zlib_inflate_workspacesize());
	if (!inf_strm.workspace) {
		vfree(def_strm.workspace);
		return -ENOMEM;
	}
	//printk("Allocated %d bytes for inflate workspace\n",
	//	  zlib_inflate_workspacesize());
	return 0;
}

static void free_workspaces(void)
{
	vfree(def_strm.workspace);
	vfree(inf_strm.workspace);
}
#else
#define alloc_workspaces() (0)
#define free_workspaces() do { } while(0)
#endif /* __KERNEL__ */

static int kzlib_compress(unsigned char *data_in,
			  unsigned char *cpage_out,
			  uint32_t *sourcelen, uint32_t *dstlen)
{
	int ret;

	if (*dstlen <= STREAM_END_SPACE)
		return -1;

	mutex_lock(&deflate_mutex);

	if (Z_OK != zlib_deflateInit(&def_strm, 3)) {
		printk(KERN_WARNING "deflateInit failed\n");
		mutex_unlock(&deflate_mutex);
		return -1;
	}

	def_strm.next_in = data_in;
	def_strm.total_in = 0;

	def_strm.next_out = cpage_out;
	def_strm.total_out = 0;

	while (def_strm.total_out < *dstlen - STREAM_END_SPACE && def_strm.total_in < *sourcelen) {
		def_strm.avail_out = *dstlen - (def_strm.total_out + STREAM_END_SPACE);
		def_strm.avail_in = min_t(unsigned long,
			(*sourcelen-def_strm.total_in), def_strm.avail_out);
		//printk("calling deflate with avail_in %d, avail_out %d\n",
		//	  def_strm.avail_in, def_strm.avail_out);
		ret = zlib_deflate(&def_strm, Z_PARTIAL_FLUSH);
		//printk("deflate returned with avail_in %d, avail_out %d, total_in %ld, total_out %ld\n",
		//	  def_strm.avail_in, def_strm.avail_out,
		//	  def_strm.total_in, def_strm.total_out);
		if (ret != Z_OK) {
			printk("deflate in loop returned %d\n", ret);
			zlib_deflateEnd(&def_strm);
			mutex_unlock(&deflate_mutex);
			return -1;
		}
	}
	def_strm.avail_out += STREAM_END_SPACE;
	def_strm.avail_in = 0;
	ret = zlib_deflate(&def_strm, Z_FINISH);
	zlib_deflateEnd(&def_strm);

	if (ret != Z_STREAM_END) {
		printk("final deflate returned %d\n", ret);
		ret = -1;
		goto out;
	}

	if (def_strm.total_out >= def_strm.total_in) {
		printk("zlib compressed %ld bytes into %ld; failing\n",
			  def_strm.total_in, def_strm.total_out);
		ret = -1;
		goto out;
	}

	//printk("zlib compressed %ld bytes into %ld\n",
	//	  def_strm.total_in, def_strm.total_out);

	*dstlen = def_strm.total_out;
	*sourcelen = def_strm.total_in;
	ret = 0;
 out:
	mutex_unlock(&deflate_mutex);
	return ret;
}

static int kzlib_decompress(unsigned char *data_in,
			    unsigned char *cpage_out,
			    uint32_t srclen, uint32_t *destlen)
{
	int ret;
	int wbits = MAX_WBITS;

	mutex_lock(&inflate_mutex);

	inf_strm.next_in = data_in;
	inf_strm.avail_in = srclen;
	inf_strm.total_in = 0;

	inf_strm.next_out = cpage_out;
	inf_strm.avail_out = *destlen;
	inf_strm.total_out = 0;

	/* If it's deflate, and it's got no preset dictionary, then
	   we can tell zlib to skip the adler32 check. */
	if (srclen > 2 && !(data_in[1] & PRESET_DICT) &&
	    ((data_in[0] & 0x0f) == Z_DEFLATED) &&
	    !(((data_in[0]<<8) + data_in[1]) % 31)) {

		//printk("inflate skipping adler32\n");
		wbits = -((data_in[0] >> 4) + 8);
		inf_strm.next_in += 2;
		inf_strm.avail_in -= 2;
	} else {
		/* Let this remain D1 for now -- it should never happen */
		printk("inflate not skipping adler32\n");
	}


	if (Z_OK != zlib_inflateInit2(&inf_strm, wbits)) {
		pr_warn("inflateInit failed\n");
		mutex_unlock(&inflate_mutex);
		return 1;
	}

	while((ret = zlib_inflate(&inf_strm, Z_FINISH)) == Z_OK)
		;
	if (ret != Z_STREAM_END) {
		pr_notice("inflate returned %d\n", ret);
	}
	zlib_inflateEnd(&inf_strm);
	mutex_unlock(&inflate_mutex);
	*destlen -= inf_strm.avail_out;
	return 0;
}

static int kzlib_init(void)
{
	return alloc_workspaces();
}

static void kzlib_exit(void)
{
	free_workspaces();
}

/* Reserve allocated COMPRESS_NVRAM memory */
struct mms {
	unsigned char *mem;
	unsigned long size;
} cmprnv = {
	NULL,
	0
};

inline int cmprnv_alloc(struct mms *pmms, unsigned long size, gfp_t flags)
{
retry:
	if (pmms->mem == NULL) {
		pmms->mem = kmalloc(size, flags);
		if (pmms->mem == NULL) {
			pr_crit("%s: @@@@@@@@@@@@@ fail @@@@@@@@@@@@@\n", __func__);
			return -1;
		}
		pmms->size = size;
	}
	else if (pmms->size < size) {
		kfree(pmms->mem);
		pmms->mem = NULL;
		goto retry;
	}

	return 0;
}
#endif /* COMPRESS_NVRAM */

/*
 * Read raw nvram data from nvram MTD partition of FLASH. Include nvram_header and variables.
 * @param	buf	buffer. at least NVRAM_SPACE bytes
 */
int
_nvram_read(char *buf)
{
	int recover_flag = 0;
	char header_buf[64];
	struct nvram_header *h = (struct nvram_header *) header_buf;
	struct nvram_header *header = (struct nvram_header *) buf;
	size_t len;

#ifdef WL_NVRAM
	int ret;
	char valid_buf[sizeof(header_buf)];
	size_t rlen;
	unsigned int i, ni, valid_offset, cmt_times = 0;
	unsigned long t1, t2;
	struct nvram_header *valid_h = (struct nvram_header *) valid_buf;
	wlnv_priv_t *priv = NULL;
#ifdef COMPRESS_NVRAM
	char *cbuf = NULL;
	char *cptr, *ptr;
	int dlen, clen, skip = sizeof(struct nvram_header) + sizeof(wlnv_priv_t);
#endif

	if (!nvram_mtd) {
		recover_flag = 1;
		goto exit__nvram_read;
	}

	/* find latest commited unit */
	t1 = jiffies;
	valid_h->magic = 0;
	for (valid_offset = i = rsv_blk_size; i < nvram_mtd->size; i = ni) {
	    ni = i + step_unit;
	    if ((ret = MTD_READ(nvram_mtd, i, sizeof(header_buf), &len, header_buf)) ||
		len != sizeof(header_buf)) {
		    printk("read offset 0x%x failure. ret %d len 0x%zx/%zx\n", i, ret, len, sizeof(header_buf));
		    continue;
	    }

	    if (h->magic != NVRAM_MAGIC
#ifdef COMPRESS_NVRAM
	     && h->magic != ZLIB_NVRAM_MAGIC
#endif
	    ) {
		    continue;
	    }

	    /* found NVRAM_MAGIC at i */
	    priv = (wlnv_priv_t*) h->private_data;
	    if (strncmp(priv->magic, "NVP", 3) || priv->length != sizeof(wlnv_priv_t)) {
		    printk("Invalid private magic %c%c%c or size mismatch %d/%zd\n",
				priv->magic[0],  priv->magic[1], priv->magic[2],
				priv->length, sizeof(wlnv_priv_t));
		    continue;
	    }

	    /* smaller commit times shouldn't exist. */
	    if (priv->commit_times <= cmt_times) {
		continue;
	    }

	    memcpy(valid_buf, header_buf, sizeof(valid_buf));
	    cmt_times = priv->commit_times;
	    if (g_wlnv.avg_len)
		    g_wlnv.avg_len = (g_wlnv.avg_len + valid_h->len) / 2;
	    else
		    g_wlnv.avg_len = valid_h->len;
	    valid_offset = i;
	    ni = ROUNDUP(i + h->len, step_unit);
	}
	t2 = jiffies;
	g_wlnv.next_offset = rsv_blk_size;
	g_wlnv.may_erase_nexttime = 0;
	printk("offset 0x%x elapse %lums\n", valid_offset, ((t2 - t1) * 1000)/HZ);

	if (cmt_times > 0 && (valid_h->magic == NVRAM_MAGIC
#ifdef COMPRESS_NVRAM
			   || valid_h->magic == ZLIB_NVRAM_MAGIC
#endif
	)) {
		/* read nvram from latest offset that provide max. commit_times.
		 * skip wlnv_priv_t structure according to it's length.
		 */
		rlen = valid_h->len;

		if (MTD_READ(nvram_mtd, valid_offset, rlen, &len, buf) || len != rlen) {
			printk("MTD_READ 0x%x len 0x%zx/0x%zx fail\n", valid_offset, len, rlen);
			recover_flag = 2;
		} else {
			g_wlnv.last_commit_times = cmt_times;
			g_wlnv.cur_offset = valid_offset;

#ifdef COMPRESS_NVRAM
			if (valid_h->magic == NVRAM_MAGIC)
				goto exit__nvram_read;

			ptr = buf + skip;
			clen = rlen - skip;

			if (cmprnv_alloc(&cmprnv, rlen, GFP_ATOMIC)) {
				printk("%s: cbuf kmalloc failed\n", __func__);
				goto exit__nvram_read;
			}
			cbuf = cmprnv.mem;
			memcpy(cbuf, buf, rlen);

			cptr = cbuf + skip;
			dlen = NVRAM_SPACE - skip;
			if (kzlib_decompress(cptr, ptr, clen, &dlen)) {
				pr_crit("%s: decompress nvram failed\n", __func__);
				goto exit__nvram_read;
			}
			//printk("%s: decompress nvram %d into %d bytes\n", __func__, clen, dlen);

			header->len = skip + dlen;
			header->magic = NVRAM_MAGIC;
#endif
		}
	} else {
		/* try to read last unit (size: NVRAM_SPACE). for compatible */
		g_wlnv.cur_offset = rsv_blk_size;
		if ((ret = MTD_READ(nvram_mtd, nvram_mtd->size - NVRAM_SPACE, NVRAM_SPACE, &len, buf)) ||
		    len != NVRAM_SPACE || header->magic != NVRAM_MAGIC) {
			recover_flag = 3;
		} else {
			memcpy(valid_buf, buf, sizeof(valid_buf));
			g_wlnv.last_commit_times = 0;
		}
	}

exit__nvram_read:
	if (recover_flag) {
		/* Maybe we can recover some data from early initialization */
		memcpy(buf, nvram_buf, NVRAM_SPACE);
	}

#else	/* !WL_NVRAM */
	if (!nvram_mtd ||
	    MTD_READ(nvram_mtd, nvram_mtd->size - NVRAM_SPACE, NVRAM_SPACE, &len, buf) ||
	    len != NVRAM_SPACE ||
	    header->magic != NVRAM_MAGIC) {
		/* Maybe we can recover some data from early initialization */
		memcpy(buf, nvram_buf, NVRAM_SPACE);
	}
#endif	/* WL_NVRAM */

	return 0;
}

struct nvram_tuple *
_nvram_realloc(struct nvram_tuple *t, const char *name, const char *value)
{
	if ((nvram_offset + strlen(value) + 1) > NVRAM_SPACE)	{
		return NULL;
	}

	if (!t) {
		if (!(t = kmalloc(sizeof(struct nvram_tuple) + strlen(name) + 1, GFP_ATOMIC)))	{
			return NULL;
		}

		/* Copy name */
		t->name = (char *) &t[1];
		strcpy(t->name, name);

		t->value = NULL;
		t->next = NULL;
	}

	/* Copy value */
	if (!t->value || strcmp(t->value, value)) {
		t->value = &nvram_buf[nvram_offset] + (nvram_idx * NVRAM_SPACE);
		strcpy(t->value, value);
		nvram_offset += strlen(value) + 1;
	}

	return t;
}

void
_nvram_free(struct nvram_tuple *t)
{
	if (!t)
	{
		nvram_offset = 0;
#if defined(NVRAM_MULTIPLE) && (NVRAM_MULTIPLE > 1)
		nvram_idx = (nvram_idx + 1) % NVRAM_MULTIPLE;
#endif	/* NVRAM_MULTIPLE */
		/*printk("%s: %d(%s) nvram_idx(%d / %d)\n",
			__func__, current->pid, current->comm, nvram_idx, NVRAM_MULTIPLE);*/
	}
	else
		kfree(t);
}

int
nvram_set(const char *name, const char *value)
{
	unsigned long flags;
	int ret;
	static struct nvram_header *header = NULL;

	if (!header)
		header = kzalloc(NVRAM_SPACE, GFP_KERNEL);

	spin_lock_irqsave(&nvram_lock, flags);

#ifdef CFE_UPDATE //write back to default sector as well, Chen-I
        if(strncmp(name, CFE_NVRAM_PREFIX, strlen(CFE_NVRAM_PREFIX))==0)
        {
                if(strcmp(name, CFE_NVRAM_COMMIT)==0)
                        cfe_commit();
                else if(strcmp(name, CFE_NVRAM_WATCHDOG)==0)
                {
                        bcm947xx_watchdog_disable();
                }
                else
                {
                        cfe_update(name+strlen(CFE_NVRAM_PREFIX), value);
                        _nvram_set(name+strlen(CFE_NVRAM_PREFIX), value);
                }
        }
        else
#endif


	if ((ret = _nvram_set(name, value))) {
		/* Consolidate space and try again */
		if (header && _nvram_commit(header) == 0)
			ret = _nvram_set(name, value);
	}
	spin_unlock_irqrestore(&nvram_lock, flags);

	return ret;
}

char *
real_nvram_get(const char *name)
{
	unsigned long flags;
	char *value;

	spin_lock_irqsave(&nvram_lock, flags);
	value = _nvram_get(name);
	spin_unlock_irqrestore(&nvram_lock, flags);

	return value;
}

char *
nvram_get(const char *name)
{
	if (nvram_major >= 0)
		return real_nvram_get(name);
	else
#ifdef ASUS_NVRAM
		return NULL;
#else	// !ASUS_NVRAM
		return early_nvram_get(name);
#endif	// ASUS_NVRAM
}

int
nvram_unset(const char *name)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&nvram_lock, flags);
	ret = _nvram_unset(name);
	spin_unlock_irqrestore(&nvram_lock, flags);

	return ret;
}

int
nvram_commit(void)
{
	#define HEADER		(zlib_done ? (uint8_t *)cbuf : (uint8_t *)header)
	#define HEADER_LEN	(zlib_done ? h->len : header->len)
	size_t erasesize, len;
	int ret, is_nand = 0;
	struct nvram_header *header;
	unsigned long flags;
	u_int32_t offset;
	struct erase_info erase;
#ifdef WL_NVRAM
	int need_erase = 0, e_retry, w_retry;
	size_t rlen, wlen, d_len, total_wlen;
	uint8_t *data, *d;
	u_int32_t o, prev_erase_offset, next_erase_offset, unit_offset, end_offset;
	struct nvram_header *h;
	char *cbuf = NULL;
#ifdef COMPRESS_NVRAM
	char *cptr, *ptr;
	int dlen, clen, skip = sizeof(struct nvram_header) + sizeof(wlnv_priv_t);
#endif
	int zlib_done = 0;
#else	/* !WL_NVRAM */
	unsigned int i;
#endif	/* WL_NVRAM */

	if (nv_locked) {
		// pr_crit("[%s:%d] under reboot, skip!\n", __func__, __LINE__);
		return 0;
	}
	printk(KERN_DEBUG "%s(): pid %d comm [%s]\n", __func__, current->pid, current->comm);
	if (!nvram_mtd) {
		printk("nvram_commit: NVRAM not found\n");
		return -ENODEV;
	}

	if (in_interrupt()) {
		printk("nvram_commit: not committing in interrupt\n");
		return -EINVAL;
	}

	/* Backup sector blocks to be erased */
	erasesize = ROUNDUP(NVRAM_SPACE + rsv_blk_size, nvram_mtd->erasesize);
	is_nand = (nvram_mtd->type == MTD_NANDFLASH || nvram_mtd->type == MTD_UBIVOLUME);

	mutex_lock(&nvram_sem);

#ifdef WL_NVRAM
	g_wlnv.last_commit_times++;

	/* Regenerate NVRAM */
	header = (struct nvram_header*) (commit_buf + rsv_blk_size);
	spin_lock_irqsave(&nvram_lock, flags);
	ret = _nvram_commit(header);
	spin_unlock_irqrestore(&nvram_lock, flags);

#ifdef COMPRESS_NVRAM
	ptr = (char *)header + skip;
	dlen = header->len - skip;

	if (cmprnv_alloc(&cmprnv, (nvram_mtd->size - rsv_blk_size), GFP_ATOMIC)) {
		printk("%s: cbuf kmalloc failed\n", __func__);
		goto compress_fail;
	}
	cbuf = cmprnv.mem;
	memcpy(cbuf, (char *)header, skip);

	cptr = cbuf + skip;
	clen = nvram_mtd->size - rsv_blk_size - skip;
	if (kzlib_compress(ptr, cptr, &dlen, &clen)) {
		printk("%s: compress nvram failed\n", __func__);
		goto compress_fail;
	}
	//printk("%s: compress nvram %d into %d bytes\n", __func__, dlen, clen);

	h = (struct nvram_header *)cbuf;
	h->len = skip + clen;
	h->magic = ZLIB_NVRAM_MAGIC;
	zlib_done = 1;

compress_fail:
#endif

nvram_commit_wrapped:
	unit_offset = g_wlnv.next_offset;
	total_wlen = HEADER_LEN;
	if (is_nand)
		total_wlen = ROUNDUP(HEADER_LEN, nvram_mtd->writesize);
	end_offset = unit_offset + total_wlen;

	if (end_offset > nvram_mtd->size) {
		/* last unit and the data length exceed nvram_mtd */
		g_wlnv.next_offset = rsv_blk_size;
		goto nvram_commit_wrapped;
	}

	for (offset = unit_offset, data = HEADER; offset < end_offset && total_wlen > 0; offset = next_erase_offset) {
		o = offset;
		e_retry = w_retry = need_erase = 0;
		prev_erase_offset = ROUNDDOWN(offset, nvram_mtd->erasesize);
		next_erase_offset = prev_erase_offset + nvram_mtd->erasesize;
		d_len = wlen = min(next_erase_offset - offset, (u_int32_t)total_wlen);
		if (unlikely(!d_len)) {
			ret = -EINVAL;
			goto done;
		}

		/* Erase a block if unit_offset is aligned to start offset of a block,
		 * or is equal to end of reseverd area.
		 */
		if (offset == ROUNDDOWN(offset, nvram_mtd->erasesize) || offset == rsv_blk_size)
			need_erase = 1;

		/* If commit data is not writted to start of a block, to keep previous data safe,
		 * always backup data in dest. block to wblk_buf first.
		 * If reserved area belongs to the block or erase/write to this block failed,
		 * use wbuf_blk instead and write from start of wblk_buf ~ end of commit data.
		 */
		if (offset > prev_erase_offset) {
			rlen = nvram_mtd->erasesize;
			ret = MTD_READ(nvram_mtd, prev_erase_offset, rlen, &len, wblk_buf);
			if (ret || len != rlen) {
				printk(KERN_WARNING "%s: backup data at 0x%x ret %d, len 0x%zx/%zx\n",
					__func__, prev_erase_offset, ret, len, rlen);
				memset(wblk_buf, 0, nvram_mtd->erasesize);
			}
		}
		memcpy(wblk_buf + (offset - prev_erase_offset), data, wlen);

start_erase_write:
		/* If size of reserved block is not zero and is about to write same block,
		 * copy reserved block to commit_buf.
		 */
		d = data;
		if (need_erase && ((rsv_blk_size > 0 && offset == rsv_blk_size) || e_retry > 0 || w_retry > 0)) {
			d = wblk_buf;
			o = prev_erase_offset;
			wlen = (offset - o) + d_len;
		}

		/* Erase sector blocks */
		if (need_erase) {
			erase.addr = o;
			erase.len = nvram_mtd->erasesize;

			/* Unlock sector blocks */
			MTD_UNLOCK(nvram_mtd, o, nvram_mtd->erasesize);

			if ((ret = MTD_ERASE(nvram_mtd, &erase))) {
				printk(KERN_WARNING "%s: erase error. (retry %d)\n", __func__, e_retry);
				if (++e_retry <= 3)
					goto start_erase_write;
				ret = -EIO;
				goto done;
			}
		}

		/* Write sector blocks */
		ret = MTD_WRITE(nvram_mtd, o, wlen, &len, d);
		if (!ret && len == wlen) {
			total_wlen -= d_len;
			data += d_len;
		} else {
			printk("%s: write to offset 0x%x error. ret %d len 0x%zx/%zx\n", __func__, o, ret, len, wlen);
			if (++w_retry <= 3) {
				need_erase = 1;
				goto start_erase_write;
			}
			ret = -EIO;
			goto done;
		}
	}

	g_wlnv.cur_offset = unit_offset;
	g_wlnv.next_offset = ROUNDUP(end_offset, step_unit);
	if (g_wlnv.avg_len)
		g_wlnv.avg_len = (g_wlnv.avg_len + HEADER_LEN) / 2;
	else
		g_wlnv.avg_len = HEADER_LEN;
	g_wlnv.may_erase_nexttime = ((g_wlnv.next_offset + g_wlnv.avg_len) >= nvram_mtd->size)? 1:0;

	offset = unit_offset;
	ret = MTD_READ(nvram_mtd, offset, 4, &len, commit_buf);
#else	/* !WL_NVRAM */
	if ((i = erasesize - NVRAM_SPACE) > 0) {
		offset = nvram_mtd->size - erasesize;
		len = 0;
		ret = MTD_READ(nvram_mtd, offset, i, &len, commit_buf);
		if (ret || len != i) {
			printk("nvram_commit: read error ret = %d, len = %d/%d\n", ret, len, i);
			ret = -EIO;
			goto done;
		}
		header = (struct nvram_header *)(commit_buf + i);
	} else {
		offset = nvram_mtd->size - NVRAM_SPACE;
		header = (struct nvram_header *)commit_buf;
	}

	/* Regenerate NVRAM */
	spin_lock_irqsave(&nvram_lock, flags);
	ret = _nvram_commit(header);
	spin_unlock_irqrestore(&nvram_lock, flags);

	if (ret)
		goto done;

	/* Erase sector blocks */
	for (; offset < nvram_mtd->size - NVRAM_SPACE + header->len; offset += nvram_mtd->erasesize) {
		erase.addr = offset;
		erase.len = nvram_mtd->erasesize;

		/* Unlock sector blocks */
		if (nvram_mtd->unlock)
			nvram_mtd->unlock(nvram_mtd, offset, nvram_mtd->erasesize);

		if ((ret = MTD_ERASE(nvram_mtd, &erase))) {
			printk("nvram_commit: erase error\n");
			goto done;
		}
	}

	/* Write partition up to end of data area */
	offset = nvram_mtd->size - erasesize;
	i = erasesize - NVRAM_SPACE + header->len;
	ret = MTD_WRITE(nvram_mtd, offset, i, &len, commit_buf);
	if (ret || len != i) {
		printk("nvram_commit: write error\n");
		ret = -EIO;
		goto done;
	}

	offset = nvram_mtd->size - erasesize;
	ret = MTD_READ(nvram_mtd, offset, 4, &len, commit_buf);
#endif	/* WL_NVRAM */

 done:
	mutex_unlock(&nvram_sem);
	return ret;
}

int
nvram_getall(char *buf, int count)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&nvram_lock, flags);
	ret = _nvram_getall(buf, count);
	spin_unlock_irqrestore(&nvram_lock, flags);

	return ret;
}

EXPORT_SYMBOL(nvram_get);
EXPORT_SYMBOL(nvram_getall);
EXPORT_SYMBOL(nvram_set);
EXPORT_SYMBOL(nvram_unset);
EXPORT_SYMBOL(nvram_commit);

/* User mode interface below */

static ssize_t
dev_nvram_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	char *name = read_buf, *value;
	ssize_t ret;
	unsigned long off;
	
	if ((count+1) > READ_BUF_SIZE) {
		if (!(name = kmalloc(count+1, GFP_KERNEL)))
			return -ENOMEM;
	}

	if (name == read_buf)
		mutex_lock(&read_buf_lock);

	if (copy_from_user(name, buf, count)) {
		ret = -EFAULT;
		goto done;
	}
	name[count] = '\0';

	if (*name == '\0') {
		/* Get all variables */
		ret = nvram_getall(name, count);
		if (ret == 0) {
			if (copy_to_user(buf, name, count)) {
				ret = -EFAULT;
				goto done;
			}
			ret = count;
		}
	} else {
		if (!(value = nvram_get(name))) {
			ret = 0;
			goto done;
		}

		/* Provide the offset into mmap() space */
		off = (unsigned long) value - (unsigned long) nvram_buf;

		if (put_user(off, (unsigned long *) buf)) {
			ret = -EFAULT;
			goto done;
		}

		ret = sizeof(unsigned long);
	}

#if !(defined(CONFIG_ARM64))
	flush_cache_all();
#else
	{
		void *ptr;
		for (ptr = nvram_buf; ptr < (void *)(nvram_buf + NVRAM_SPACE_FULL); ptr += PAGE_SIZE)
			flush_dcache_page(virt_to_page(ptr));
	}
#endif
 
done:
	if (name == read_buf)
		mutex_unlock(&read_buf_lock);
	else
		kfree(name);

	return ret;
}

static ssize_t
dev_nvram_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	char tmp[100], *name = tmp, *value, *p;
	ssize_t ret;

	if ((count+1) > sizeof(tmp)) {
		if (!(name = kmalloc(count+1, GFP_KERNEL)))
			return -ENOMEM;
	}

	if (copy_from_user(name, buf, count)) {
		ret = -EFAULT;
		goto done;
	}

	name[count] = '\0';
	value = name;
	name = strsep(&value, "=");
	if (unlikely(!strcmp(name, "productid")
		  && (p = nvram_get("productid")) != NULL)
		  && value && strcmp(p, value)
	) {
		printk("productid [%s] --> [%s]!\n", p, value? : "NULL");
	}

	if (value)
		ret = nvram_set(name, value) ? : count;
	else
		ret = nvram_unset(name) ? : count;

 done:
	if (name != tmp)
		kfree(name);

	return ret;
}

static char *
nvram_xfr(const char *buf)
{
	char *name = tmpbuf;
	ssize_t ret=0;

	//printk("nvram xfr 1: %s\n", buf);	// tmp test
	if (copy_from_user(name, buf, sizeof(tmpbuf))) {
		ret = -EFAULT;
		goto done;
	}

	if (strncmp(tmpbuf, NLS_NVRAM_U2C, strlen(NLS_NVRAM_U2C))==0)
	{
		asusnls_u2c(tmpbuf);
	}
	else if (strncmp(tmpbuf, NLS_NVRAM_C2U, strlen(NLS_NVRAM_C2U))==0)
	{
		asusnls_c2u(tmpbuf);
	}
	else
	{
		strcpy(tmpbuf, "");
		//printk("nvram xfr 2: %s\n", tmpbuf);	// tmp test
	}

	if (copy_to_user((char*)buf, tmpbuf, sizeof(tmpbuf)))
	{
		ret = -EFAULT;
		goto done;
	}
	//printk("nvram xfr 3: %s\n", tmpbuf);	// tmp test

done:
	if(ret==0) return tmpbuf;
	else return NULL;
}

static long
dev_nvram_do_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (cmd == NVRAM_IOCTL_GET_SPACE && arg != 0) {
		unsigned int nvram_space = NVRAM_SPACE;
		copy_to_user((unsigned int *)arg, &nvram_space, sizeof(nvram_space));
		return 0;
	} else if (cmd == 0x0003) {
		nvram_free();
		/* reload nvram */
		_nvram_init(sbh);
		return 0;
	} else if (cmd == 0x0004) {
		nv_locked = 1;
		return 0;
	}
	if (cmd != NVRAM_MAGIC)
		return -EINVAL;
	if(arg==0)
		return nvram_commit();
#ifdef NLS_XFR
	else {
		if(nvram_xfr((char *)arg)!=NULL) return 0;
		else return -EFAULT;
	}
#endif
}

static long
dev_nvram_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret;

	mutex_lock(&nvram_ioctl_lock);
	ret = dev_nvram_do_ioctl(file, cmd, arg);
	mutex_unlock(&nvram_ioctl_lock);
	return ret;
}

#if defined (ASUS_NVRAM) && !defined (CONFIG_MMU)
static unsigned long
dev_nvram_get_unmapped_area (struct file *file, unsigned long addr, unsigned long len, unsigned long pgoff, unsigned long flags)
{
	/* If driver need shared mmap, it have to provide an get_unmapped_area method on MMU-less system.
	 * get_unmapped_area method is responsible to validation addr, len, pgoff, and flags
	 * if these parameters are invalid, return -ENOSYS.
	 * if these parameters are valid, return an address that would be apply to vma->vm_start.
	 * You may have a look at Documentation/nommu-mmap.txt
	 */
	if (len > NVRAM_SPACE || (addr + (pgoff << PAGE_SHIFT) + len) > NVRAM_SPACE)	{
		return -ENOSYS;
	}

	/* The vma->vm_start is overwrite by remap_pfn_range() on MMU-less system.
	 * So, we don't have to take care it.
	 */
	return 0;
}
#endif	// ASUS_NVRAM && !CONFIG_MMU


static int
dev_nvram_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long offset = virt_to_phys(nvram_buf);

#ifdef ASUS_NVRAM
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,8)

#ifdef CONFIG_MMU
//error Eric didn't test this situations.
	int ret;
	if ((ret = remap_pfn_range(vma, vma->vm_start, offset >> PAGE_SHIFT, vma->vm_end-vma->vm_start, vma->vm_page_prot)))
#else	// !CONFIG_MMU
	vma->vm_pgoff = ((vma->vm_pgoff << PAGE_SHIFT) + offset) >> PAGE_SHIFT;
	vma->vm_end = (vma->vm_pgoff << PAGE_SHIFT) + (vma->vm_end - vma->vm_start);
	vma->vm_start = vma->vm_pgoff << PAGE_SHIFT;
	if (0)
#endif	// CONFIG_MMU

#else	// !(LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,8))
	int ret;

	if ((ret = remap_page_range(vma->vm_start, offset, vma->vm_end-vma->vm_start, vma->vm_page_prot)))
#endif	// LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,8)

#else	// !ASUS_NVRAM
	int ret;

	if (((ret = remap_page_range(vma->vm_start, offset, vma->vm_end-vma->vm_start, vma->vm_page_prot))))
#endif	// ASUS_NVRAM
	{
		return -EAGAIN;
	}

	return 0;
}

static int
dev_nvram_open(struct inode *inode, struct file * file)
{
	MOD_INC_USE_COUNT;	// 1018 disable
	return 0;
}

static int
dev_nvram_release(struct inode *inode, struct file * file)
{
	MOD_DEC_USE_COUNT;	// 1018 disable
	return 0;
}
/*
static int
dev_nvram_owner(struct inode *inode, struct file * file)
{
        return 0;
}
*/
static struct file_operations dev_nvram_fops = {
	owner:		THIS_MODULE,
	//owner:		dev_nvram_owner,	// 1018 add
	open:		dev_nvram_open,
	release:	dev_nvram_release,
	read:		dev_nvram_read,
	write:		dev_nvram_write,
	unlocked_ioctl:	dev_nvram_ioctl,
	mmap:		dev_nvram_mmap,
#if defined (ASUS_NVRAM) && !defined (CONFIG_MMU)
	get_unmapped_area:	dev_nvram_get_unmapped_area,
#endif	// ASUS_NVRAM && !CONFIG_MMU
};

static void
dev_nvram_exit(void)
{
	int order = 0;
	struct page *page, *end;

#ifdef COMPRESS_NVRAM
	kzlib_exit();
#endif

#ifdef ASUS_NVRAM
	if (s_nvram_device) {
		device_destroy(s_nvram_class, MKDEV(nvram_major, 0));
		s_nvram_device = NULL;
	}
	if (s_nvram_class) {
		class_destroy(s_nvram_class);
		s_nvram_class = NULL;
	}
	if (nvram_major >= 0)
		unregister_chrdev(NVRAM_MAJOR, NVRAM_DRV_NAME);
#else	// !ASUS_NVRAM
	if (nvram_handle)
		devfs_unregister(nvram_handle);

	if (nvram_major >= 0)
		devfs_unregister_chrdev(nvram_major, NVRAM_DRV_NAME);
#endif	// ASUS_NVRAM

#ifdef WL_NVRAM
	kfree(wblk_buf);
#endif

	kfree(commit_buf);
	kfree(read_buf);

	if (nvram_mtd)
		put_mtd_device(nvram_mtd);

	while ((PAGE_SIZE << order) < NVRAM_SPACE)
		order++;
	end = virt_to_page(nvram_buf + (PAGE_SIZE << order) - 1);
	for (page = virt_to_page(nvram_buf); page <= end; page++)
		mem_map_unreserve(page);

#ifdef ASUS_NVRAM
	if (g_pdentry != NULL)	{
		remove_proc_entry(NVRAM_DRV_NAME, NULL);
	}

	kfree (nvram_buf);
#endif	// ASUS_NVRAM

	_nvram_exit();
}

static int __init
dev_nvram_init(void)
{
	int order = 0, ret = 0;
	struct page *page, *end;
	unsigned int i;
	struct class *tmp_class;
	struct device *tmp_device;
	size_t erasesize;

#ifdef ASUS_NVRAM
	nvram_buf = kzalloc (NVRAM_SPACE_FULL, GFP_ATOMIC);
	if (nvram_buf == NULL) {
		printk(KERN_ERR "%s(): Allocate %d bytes fail!\n", __func__, NVRAM_SPACE_FULL);
		return -ENOMEM;
	}
#endif	// ASUS_NVRAM

	/* Allocate and reserve memory to mmap() */
	while ((PAGE_SIZE << order) < NVRAM_SPACE_FULL)
		order++;
	end = virt_to_page(nvram_buf + (PAGE_SIZE << order) - 1);
	for (page = virt_to_page(nvram_buf); page <= end; page++)	{
		mem_map_reserve(page);
	}

#ifdef CONFIG_MTD
	/* Find associated MTD device */
	for (i = 0; true; i++) {
		nvram_mtd = get_mtd_device(NULL, i);
		if (IS_ERR(nvram_mtd))
		{
			ret = PTR_ERR(nvram_mtd);
			nvram_mtd = NULL;
			break;
		}
		else
		{
			if (!strcmp(nvram_mtd->name, MTD_NVRAM_NAME) &&
#ifdef COMPRESS_NVRAM
			    nvram_mtd->size >= (NVRAM_SPACE / 2)
#else
			    nvram_mtd->size >= NVRAM_SPACE
#endif
			) {
				break;
			}
			put_mtd_device(nvram_mtd);
		}
	}
#endif

	if (!nvram_mtd) {
		printk(KERN_ERR "%s(): nvram MTD partition not found!\n", __func__);
		ret = -ENODEV;
		goto err;
	}

	if (nvram_mtd->type == MTD_NANDFLASH || nvram_mtd->type == MTD_UBIVOLUME) {
		step_unit = ROUNDUP(STEP_UNIT, nvram_mtd->writesize);
		rsv_blk_size = ROUNDUP(ROUNDUP(RESERVED_BLOCK_SIZE, nvram_mtd->writesize), step_unit);
	}

	if (rsv_blk_size >= nvram_mtd->erasesize) {
		printk("%s: Size of reserved area must not exceed block size.\n", __func__);
		ret = -EINVAL;
		goto err;
	}

	/* buffer for dev_nvram_read() */
	if (!(read_buf = kzalloc(NVRAM_SPACE, GFP_KERNEL))) {
		printk("%s: out of memory\n", __func__);
		ret = -ENOMEM;
		goto err;
	}

	/* Pre-allocate buffer */
	erasesize = ROUNDUP(NVRAM_SPACE + rsv_blk_size, nvram_mtd->erasesize);
	if (!(commit_buf = kmalloc(erasesize, GFP_KERNEL))) {
		printk("%s: out of memory\n", __func__);
		ret = -ENOMEM;
		goto err;
	}
#ifdef WL_NVRAM
	if (!(wblk_buf = kmalloc(erasesize, GFP_KERNEL))) {
		printk("%s:: out of memory\n", __func__);
		ret = -ENOMEM;
		goto err;
	}
#endif

#ifdef COMPRESS_NVRAM
	kzlib_init();
#endif

	/* Initialize hash table lock */
	spin_lock_init(&nvram_lock);

	/* Register char device */
#ifdef ASUS_NVRAM
	if ((nvram_major = register_chrdev(NVRAM_MAJOR, NVRAM_DRV_NAME, &dev_nvram_fops)) < 0) {
		printk(KERN_ERR "%s(): Register character driver fail. ret %d!\n", __func__, nvram_major);
		ret = nvram_major;
		goto err;
	}
	if (!nvram_major)
		nvram_major = NVRAM_MAJOR;

	tmp_class = class_create(THIS_MODULE, NVRAM_DRV_NAME);
	if (IS_ERR(tmp_class)) {
		printk(KERN_ERR "%s(): Create class fail. ret %p!\n", __func__, tmp_class);
		ret = PTR_ERR(tmp_class);
		goto err;
	}
	s_nvram_class = tmp_class;

	tmp_device = device_create(s_nvram_class, NULL, MKDEV(nvram_major, 0), "%s", NVRAM_DRV_NAME);
	if (IS_ERR(tmp_device)) {
		printk(KERN_ERR "%s(): Create device fail. ret %p!\n", __func__, tmp_device);
		ret = PTR_ERR(tmp_device);
		goto err;
	}
	s_nvram_device = tmp_device;
#else	// !ASUS_NVRAM
	if ((nvram_major = devfs_register_chrdev(NVRAM_MAJOR, NVRAM_DRV_NAME, &dev_nvram_fops)) < 0) {
		ret = nvram_major;
		goto err;
	}
#endif	// ASUS_NVRAM

	/* Initialize hash table */
	_nvram_init(sbh);

	/* Create /dev/nvram handle */
#ifdef ASUS_NVRAM
#else	// !ASUS_NVRAM
	nvram_handle = devfs_register(NULL, NVRAM_DRV_NAME, DEVFS_FL_NONE, nvram_major, 0,
				      S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP, &dev_nvram_fops, NULL);
#endif	// ASUS_NVRAM

#ifdef ASUS_NVRAM
	// We don't need sdram_XXX
#else	// !ASUS_NVRAM
	/* Set the SDRAM NCDL value into NVRAM if not already done */
	if (getintvar(NULL, "sdram_ncdl") == 0) {
		unsigned int ncdl;
		char buf[] = "0x00000000";

		if ((ncdl = sb_memc_get_ncdl(sbh))) {
			sprintf(buf, "0x%08x", ncdl);
			nvram_set("sdram_ncdl", buf);
			nvram_commit();
		}
	}
#endif	// ASUS_NVRAM

#ifdef ASUS_NVRAM
	g_pdentry = proc_create(NVRAM_DRV_NAME, S_IRUGO, NULL, &nvram_file_ops);
	if (g_pdentry == NULL) {
		printk(KERN_ERR "%s(): Create /proc/nvram fail!\n", __func__);
		ret  = -ENOMEM;
		goto err;
	}

#endif	// ASUS_NVRAM

//	extern int (*nvram_set_p)(char *, char *);
//	nvram_set_p = nvram_set;

	return 0;

err:
	dev_nvram_exit();
	return ret;
}
#ifdef CFE_UPDATE
void cfe_init(void)
{
        size_t erasesize, len;
        int i;

        /* Find associated MTD device */
        for (i = 0; i < true ; i++) {
                cfe_mtd = get_mtd_device(NULL, i);
                if (IS_ERR(cfe_mtd)) {
			printk("No CFE MTD\n");
			cfe_mtd = NULL;
			break;
		} else {
                        printk("CFE MTD: %x %s %x\n", i, cfe_mtd->name, cfe_mtd->size);
                        if (!strcmp(cfe_mtd->name, "boot"))
                                break;
                        put_mtd_device(cfe_mtd);
                }
        }

        if(!cfe_mtd) goto fail;

        /* sector blocks to be erased and backup */
        erasesize = ROUNDUP(CFE_NVRAM_SPACE, cfe_mtd->erasesize);

        //printk("block size %d\n", erasesize);

        cfe_buf = kmalloc(erasesize, GFP_KERNEL);

        if(!cfe_buf)
        {
                //printk("No CFE Memory\n");
                goto fail;
        }
        MTD_READ(cfe_mtd, CFE_NVRAM_START, erasesize, &len, cfe_buf);

        // find nvram header
        for(i=0;i<len;i+=4)
        {
                cfe_nvram_header=(struct nvram_header *)&cfe_buf[i];
                if (cfe_nvram_header->magic==NVRAM_MAGIC) break;
        }

        bcm947xx_watchdog_disable(); //disable watchdog as well

        //printf("read from nvram %d %s\n", i, cfe_buf);
        //for(i=0;i<CFE_SPACE;i++)
        //{
        //      if(i%16) printk("\n");
        //      printk("%02x ", (unsigned char)cfe_buf[i]);
        //}
        return;
fail:
        if (cfe_mtd)
        {
                put_mtd_device(cfe_mtd);
                cfe_mtd=NULL;
        }
        if(cfe_buf)
        {
                kfree(cfe_buf);
                cfe_buf=NULL;
        }
        return;
}

void cfe_update(char *keyword, char *value)
{
        unsigned long i, offset;
        struct nvram_header tmp, *header;
        uint8 crc;
        int ret;
        int found = 0;

        if(!cfe_buf||!cfe_mtd)
                cfe_init();

        if (!cfe_buf||!cfe_mtd) return;

        header = cfe_nvram_header;

        //printk("before: %x %x\n", header->len,  cfe_nvram_header->crc_ver_init&0xff);

        for(i=CFE_NVRAM_START;i<=CFE_NVRAM_END;i++)
        {
                if(strncmp(&cfe_buf[i], keyword, strlen(keyword))==0)
                {
                        //printk("before: %s\n", cfe_buf+i);
                        offset=strlen(keyword);
                        memcpy(cfe_buf+i+offset+1, value, strlen(value));
                        //printk("after: %s\n", cfe_buf+i);
                        found = 1;
                }
        }

        if(!found)
        {
                char *tmp_buf = (char *)cfe_nvram_header;

                //printk("header len: %x\n", header->len);
                sprintf(tmp_buf+header->len, "%s=%s", keyword, value);
                header->len = header->len + strlen(keyword) + strlen(value) + 2;
                //printk("header len: %x\n", header->len);
        }

        tmp.crc_ver_init = htol32(header->crc_ver_init);
        tmp.config_refresh = htol32(header->config_refresh);
        tmp.config_ncdl = htol32(header->config_ncdl);
        crc = hndcrc8((char *) &tmp + 9, sizeof(struct nvram_header) - 9, CRC8_INIT_VALUE);

        /* Continue CRC8 over data bytes */
        crc = hndcrc8((char *) &header[1], header->len - sizeof(struct nvram_header), crc);
        header->crc_ver_init = (header->crc_ver_init&0xFFFFFF00)|crc;
        //printk("after: %x %x\n", header->crc_ver_init&0xFF, crc);
}

int cfe_commit(void)
{
        struct erase_info erase;
        unsigned int i;
        int ret;
        size_t erasesize, len;
        u_int32_t offset;
        char *buf;

        if(!cfe_buf||!cfe_mtd) cfe_init();

        if(!cfe_mtd||!cfe_buf)
        {
                ret = - ENOMEM;
                goto done;
        }

        /* Backup sector blocks to be erased */
        erasesize = ROUNDUP(CFE_NVRAM_SPACE, cfe_mtd->erasesize);

        /* Erase sector blocks */
        for (offset=CFE_NVRAM_START;offset <= CFE_NVRAM_END;offset += cfe_mtd->erasesize) {
           erase.addr = offset;
           erase.len = cfe_mtd->erasesize;

           /* Unlock sector blocks */
           if (cfe_mtd->unlock)
                   cfe_mtd->unlock(cfe_mtd, offset, cfe_mtd->erasesize);

           if ((ret = MTD_ERASE(cfe_mtd, &erase))) {
                printk("cfe_commit: erase error\n");
                goto done;
           }
        }

        ret = MTD_WRITE(cfe_mtd, CFE_NVRAM_START, erasesize, &len, cfe_buf);
        //printk("Write offset: %x %x %x\n", ret, len, erasesize);

        if (ret || len != erasesize) {
           printk("cfe_commit: write error\n");
           ret = -EIO;
        }

done:
        if (cfe_mtd)
        {
                put_mtd_device(cfe_mtd);
                cfe_mtd=NULL;
        }
        if(cfe_buf)
        {
                kfree(cfe_buf);
                cfe_buf=NULL;
        }
        //printk("commit: %d\n", ret);
        return ret;

}
#endif



module_init(dev_nvram_init);
module_exit(dev_nvram_exit);
#ifdef ASUS_NVRAM
MODULE_LICENSE("GPL");
#endif	// ASUS_NVRAM

MODULE_VERSION("V0.01");
