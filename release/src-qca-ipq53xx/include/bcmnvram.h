/*
 * NVRAM variable manipulation
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: bcmnvram.h,v 13.60.2.1 2008/05/02 22:47:26 Exp $
 */

#ifndef _bcmnvram_h_
#define _bcmnvram_h_

#ifndef _LANGUAGE_ASSEMBLY

#ifndef __KERNEL__
#include <string.h>
#include <rtconfig.h>
#include <asm/byteorder.h>
#endif

#ifdef ASUS_NVRAM
#include <nvram/typedefs.h>
#include <nvram/bcmdefs.h>
#else	// !ASUS_NVRAM
#include <typedefs.h>
#include <bcmdefs.h>
#endif	// ASUS_NVRAM

typedef struct wlnvram_private_s {
	uint8 length;		/* length. ALWAYS USE IT TO EVALUATE STARTING OFFSET OF NVRAM VARIABLES */
	char magic[3];		/* 'N', 'V', 'P' */
	uint32 commit_times;	/* nvram commit times in host endian */
	/* ALWAYS APPEND NEW DATA MEMBER and FIX THE read_private_date() FUNCTION. */
} wlnv_priv_t;

struct nvram_header {
	uint32 magic;
	uint32 len;		/* date length behind nvram_header, include last two '\0' but commit_times. */
	uint32 crc_ver_init;	/* 0:7 crc, 8:15 ver, 16:31 sdram_init */
	uint32 config_refresh;	/* 0:15 sdram_config, 16:31 sdram_refresh */
	uint32 config_ncdl;	/* ncdl values for memc */
	/* point to wlnvram_private if it was commited by wear-levelling nvram.  */
	char private_data[0];
};

struct nvram_tuple {
	char *name;
	char *value;
	unsigned short len;
	unsigned short type;
	unsigned short acc_level;
	unsigned short enc;
	struct nvram_tuple *next;
};

/*
 * Get default value for an NVRAM variable
 */
extern char *nvram_default_get(const char *name);

/*
 * Initialize NVRAM access. May be unnecessary or undefined on certain
 * platforms.
 */
extern int nvram_init(void *sih);

/*
 * Append a chunk of nvram variables to the global list
 */
extern int nvram_append(void *si, char *vars, uint varsz);

/*
 * Check for reset button press for restoring factory defaults.
 */
extern int nvram_reset(void *sih);

/*
 * Disable NVRAM access. May be unnecessary or undefined on certain
 * platforms.
 */
extern void nvram_exit(void *sih);

/*
 * Get the value of an NVRAM variable. The pointer returned may be
 * invalid after a set.
 * @param	name	name of variable to get
 * @return	value of variable or NULL if undefined
 */
extern char * nvram_get(const char *name);

/* 
 * Read the reset GPIO value from the nvram and set the GPIO
 * as input
 */
extern int BCMINITFN(nvram_resetgpio_init)(void *sih);

/* 
 * Get the value of an NVRAM variable.
 * @param	name	name of variable to get
 * @return	value of variable or NUL if undefined
 */
#define nvram_safe_get(name) (nvram_get(name) ? : "")

/*
 * Match an NVRAM variable.
 * @param	name	name of variable to match
 * @param	match	value to compare against value of variable
 * @return	TRUE if variable is defined and its value is string equal
 *		to match or FALSE otherwise
 */
static INLINE int
nvram_match(char *name, const char *match) {
	const char *value = nvram_get(name);
	return (value && !strcmp(value, match));
}

/*
 * Inversely match an NVRAM variable.
 * @param	name	name of variable to match
 * @param	match	value to compare against value of variable
 * @return	TRUE if variable is defined and its value is not string
 *		equal to invmatch or FALSE otherwise
 */
static INLINE int
nvram_invmatch(char *name, const char *invmatch) {
	const char *value = nvram_get(name);
	return (value && strcmp(value, invmatch));
}

/*
 * Set the value of an NVRAM variable. The name and value strings are
 * copied into private storage. Pointers to previously set values
 * may become invalid. The new value may be immediately
 * retrieved but will not be permanently stored until a commit.
 * @param	name	name of variable to set
 * @param	value	value of variable
 * @return	0 on success and errno on failure
 */
extern int nvram_set(const char *name, const char *value);

/*
 * Unset an NVRAM variable. Pointers to previously set values
 * remain valid until a set.
 * @param	name	name of variable to unset
 * @return	0 on success and errno on failure
 * NOTE: use nvram_commit to commit this change to flash.
 */
extern int nvram_unset(const char *name);

/*
 * Commit NVRAM variables to permanent storage. All pointers to values
 * may be invalid after a commit.
 * NVRAM values are undefined after a commit.
 * @return	0 on success and errno on failure
 */
extern int nvram_commit(void);

/*
 * Get all NVRAM variables (format name=value\0 ... \0\0).
 * @param	buf	buffer to store variables
 * @param	count	size of buffer in bytes
 * @return	0 on success and errno on failure
 */
extern int nvram_getall(char *nvram_buf, int count);

/*
 * returns the crc value of the nvram
 * @param	nvh	nvram header pointer
 */
uint8 nvram_calc_crc(struct nvram_header * nvh);

#endif /* _LANGUAGE_ASSEMBLY */

/* The NVRAM version number stored as an NVRAM variable */
#define NVRAM_SOFTWARE_VERSION	"1"

#if !defined(UBOOT_CFG_ENV_SIZE)
////// NAND flash solution
#define UBOOT_CFG_ENV_SIZE 0 // nand solution ubootenv is independant with nvram
#define UBI_OVERHEAD	0x1000
////// expand SPI NOR definition here if someday use that~
#endif

#define NVRAM_MAGIC_BE		0x464C5348	/* 'FLSH' */
#define NVRAM_MAGIC_LE		0x48534C46	/* 'FLSH' */
#define ZLIB_NVRAM_MAGIC_BE	0x5A4C4942	/* 'ZLIB' */
#define ZLIB_NVRAM_MAGIC_LE	0x42494C5A	/* 'ZLIB' */

#if defined(NVRAM_MAGIC)
#undef NVRAM_MAGIC
#endif
#if defined(ZLIB_NVRAM_MAGIC)
#undef ZLIB_NVRAM_MAGIC
#endif

#if defined(__KERNEL__)
#if defined(CONFIG_CPU_BIG_ENDIAN)
#define NVRAM_MAGIC		NVRAM_MAGIC_BE
#define ZLIB_NVRAM_MAGIC	ZLIB_NVRAM_MAGIC_BE
#else
#define NVRAM_MAGIC		NVRAM_MAGIC_LE
#define ZLIB_NVRAM_MAGIC	ZLIB_NVRAM_MAGIC_LE
#endif
#else	/* !__KERNEL__ */
#if (__BYTE_ORDER == __BIG_ENDIAN)
#define NVRAM_MAGIC		NVRAM_MAGIC_BE
#define ZLIB_NVRAM_MAGIC	ZLIB_NVRAM_MAGIC_BE
#elif (__BYTE_ORDER == __LITTLE_ENDIAN)
#define NVRAM_MAGIC		NVRAM_MAGIC_LE
#define ZLIB_NVRAM_MAGIC	ZLIB_NVRAM_MAGIC_LE
#endif
#endif	/* __KERNEL__ */

#if !defined(NVRAM_MAGIC)
#error Endian is not defined!
#endif

#define NVRAM_CLEAR_MAGIC	0x0
#define NVRAM_INVALID_MAGIC	0xFFFFFFFF
#define NVRAM_VERSION		1
#define NVRAM_HEADER_SIZE	20
#if defined(RTCONFIG_NVRAM_SIZE)
#define CFG_NVRAM_SIZE	RTCONFIG_NVRAM_SIZE
#elif defined(CONFIG_NVRAM_SIZE)
#define CFG_NVRAM_SIZE	CONFIG_NVRAM_SIZE
#elif defined(RTCONFIG_NVRAM_64K)
#define CFG_NVRAM_SIZE	0x10000
#endif

#if defined(COMPRESS_NVRAM)
#define COMPRESS_NVRAM_RATIO	(2)
#else
#define COMPRESS_NVRAM_RATIO	(1)
#endif

#if defined(CFG_NVRAM_SIZE)
#if (CFG_NVRAM_SIZE >= (0x10000 - UBOOT_CFG_ENV_SIZE - UBI_OVERHEAD))
#define NVRAM_SPACE		((CFG_NVRAM_SIZE - UBOOT_CFG_ENV_SIZE - UBI_OVERHEAD) * COMPRESS_NVRAM_RATIO)
#else
#define NVRAM_SPACE		((CFG_NVRAM_SIZE - UBI_OVERHEAD) * COMPRESS_NVRAM_RATIO)
#endif
#else
#define NVRAM_SPACE		((0x10000 - UBOOT_CFG_ENV_SIZE - UBI_OVERHEAD) * COMPRESS_NVRAM_RATIO)
#endif
/* For CFE builds this gets passed in thru the makefile */
#ifndef MAX_NVRAM_SPACE
#define MAX_NVRAM_SPACE		NVRAM_SPACE
#endif

#define NVRAM_MULTIPLE		(2)
#define NVRAM_SPACE_FULL	((NVRAM_SPACE) * (NVRAM_MULTIPLE))

#define NVRAM_IOCTL_GET_SPACE	0x0001

#define DEF_NVRAM_SPACE		0x8000
#define NVRAM_LZMA_MAGIC	0x4c5a4d41	/* 'LZMA' */

#define NVRAM_MAX_VALUE_LEN 255
#define NVRAM_MAX_PARAM_LEN 64

#define NVRAM_CRC_START_POSITION	9 /* magic, len, crc8 to be skipped */
#define NVRAM_CRC_VER_MASK	0xffffff00 /* for crc_ver_init */

#define NVRAM_MAJOR	228

#endif /* _bcmnvram_h_ */
