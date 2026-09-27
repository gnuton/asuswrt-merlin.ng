/*
 * NVRAM variable manipulation (common)
 *
 * Copyright 2006, Broadcom Corporation
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: nvram.c,v 1.1 2007/06/08 07:38:05 arthur Exp $
 */

#ifdef ASUS_NVRAM
#include <linux/string.h>
#include <nvram/typedefs.h>
#include <nvram/bcmdefs.h>
#include <nvram/bcmutils.h>
#include <nvram/bcmendian.h>
#include <nvram/bcmnvram.h>

#define bzero(p,l)		memset(p,0,l)
#define sb_osh(s)		s
#define MALLOC(o,s)		kmalloc(s, GFP_ATOMIC)
#define MFREE(o,a,s)	kfree(a);
#define printf			printk

#else	// !ASUS_NVRAM
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <sbutils.h>
#include <bcmendian.h>
#include <bcmnvram.h>
#include <sbsdram.h>
#endif	// ASUS_NVRAM

#ifdef WL_NVRAM
static struct wlnvram_s {
	unsigned int last_commit_times;
	unsigned int next_offset, cur_offset;
	unsigned int avg_len, may_erase_nexttime;
} g_wlnv = {0, 0, 0, 0, 0};

#define STEP_UNIT	(2 * 1024)
#endif	/* WL_NVRAM */


extern struct nvram_tuple * _nvram_realloc(struct nvram_tuple *t, const char *name,
                                           const char *value);
extern void _nvram_free(struct nvram_tuple *t);
#ifdef ASUS_NVRAM
extern int _nvram_read(char *buf);
#else	// !ASUS_NVRAM
extern int _nvram_read(void *buf);
#endif	// ASUS_NVRAM

char * _nvram_get(const char *name);
int _nvram_set(const char *name, const char *value);
int _nvram_unset(const char *name);
int _nvram_getall(char *buf, int count);
int _nvram_commit(struct nvram_header *header);
int _nvram_init(void *sb);
void _nvram_exit(void);

static struct nvram_tuple * BCMINITDATA(nvram_hash)[257];
static struct nvram_tuple * nvram_dead;


#ifdef WL_NVRAM
/*
 * Parsing private data if it is available.
 * @param	header		point to struct nvram_header
 * @param	priv		if nvram_header is follower by wlnv_priv_t, parsing and record here
 * @return	NULL		invalid parameter(s)
 * 		otherwise	point to region of nvram variables, wherher wlnv_priv_t is present or not
 */
char *read_private_date(struct nvram_header *header, wlnv_priv_t *priv)
{
	wlnv_priv_t *p;

	/* sanity check */
	if (header == NULL || priv == NULL) {
		printk(KERN_ERR "Invalid header or priv\n");
		return NULL;
	}

	p = (wlnv_priv_t*) header->private_data;
	if (header->magic != NVRAM_MAGIC) {
		printk("%s(): !NVRAM_MAGIC\n", __FUNCTION__);
		return (char*)header;
	}
	memset(priv, 0, sizeof(wlnv_priv_t));
	if (!strncmp(p->magic, "NVP", 3)) {
		if (p->length != sizeof(wlnv_priv_t)) {
			/* if size of private data is mismatch, do not use it. */
			printk(KERN_ERR "size of private data mismatch!\n");
		} else {
			*priv = *p;
		}
		p = (wlnv_priv_t*) ((char*)p + p->length);
	}

	if (p != (wlnv_priv_t*)(header->private_data))
		printk(KERN_DEBUG "found private data block and skip it\n");
	return (char*) p;
}
#endif	/* WL_NVRAM */


#ifdef ASUS_NVRAM
// Copy from sbsdram.h
/* SDRAM configuration (config) register bits */
#define SDRAM_BURSTFULL 0x0000  /* Use full page bursts */
#define SDRAM_BURST8    0x0001  /* Use burst of 8 */
#define SDRAM_BURST4    0x0002  /* Use burst of 4 */
#define SDRAM_BURST2    0x0003  /* Use burst of 2 */
#define SDRAM_CAS3  0x0000  /* Use CAS latency of 3 */
#define SDRAM_CAS2  0x0004  /* Use CAS latency of 2 */
/* SDRAM refresh control (refresh) register bits */
#define SDRAM_REF(p)    (((p)&0xff) | SDRAM_REF_EN) /* Refresh period */
#define SDRAM_REF_EN    0x8000      /* Writing 1 enables periodic refresh */
/* SDRAM Core default Init values (OCP ID 0x803) */
#define SDRAM_INIT  MEM4MX16X2
#define SDRAM_CONFIG    SDRAM_BURSTFULL
#define SDRAM_REFRESH   SDRAM_REF(0x40)
#define MEM1MX16    0x009   /* 2 MB */
#define MEM1MX16X2  0x409   /* 4 MB */
#define MEM2MX8X2   0x809   /* 4 MB */
#define MEM2MX8X4   0xc09   /* 8 MB */
#define MEM2MX32    0x439   /* 8 MB */
#define MEM4MX16    0x019   /* 8 MB */
#define MEM4MX16X2  0x419   /* 16 MB */
#define MEM8MX8X2   0x819   /* 16 MB */
#define MEM8MX16    0x829   /* 16 MB */
#define MEM4MX32    0x429   /* 16 MB */
#define MEM8MX8X4   0xc19   /* 32 MB */
#define MEM8MX16X2  0xc29   /* 32 MB */


static int
bcm_isspace (char c)
{
	switch (c)	{
		case ' ':		// fall-through
		case '\f':		// fall-through
		case '\n':		// fall-through
		case '\r':		// fall-through
		case '\t':		// fall-through
		case '\v':		// fall-through
			return 1;
	}
	
	return 0;
}

static int
bcm_isxdigit(char c)
{
	char t[] = "0123456789abcdefABCDEF";
	if (strchr (t, c) != NULL)
		return 1;
	return 0;
}

static int
bcm_islower(char c)
{
	if (c >= 'a' && c <= 'z')
		return 1;
	return 0;
}

static int 
bcm_toupper(uchar c)
{
	if (bcm_islower(c))
		c -= 'a' - 'A';
	return (c);
}


static int
bcm_isdigit(char c)
{
	if (c >= '0' && c <= '9')
		return 1;
	return 0;
}

// copy from src/shared/bcmutils.c
static ulong
bcm_strtoul(char *cp, char **endp, uint base)
{
	ulong result, value;
	bool minus;

	minus = FALSE;

	while (bcm_isspace(*cp))
		cp++;

	if (cp[0] == '+')
		cp++;
	else if (cp[0] == '-') {
		minus = TRUE;
		cp++;
	}

	if (base == 0) {
		if (cp[0] == '0') {
			if ((cp[1] == 'x') || (cp[1] == 'X')) {
				base = 16;
				cp = &cp[2];
			} else {
				base = 8;
				cp = &cp[1];
			}
		} else
			base = 10;
	} else if (base == 16 && (cp[0] == '0') && ((cp[1] == 'x') || (cp[1] == 'X'))) {
		cp = &cp[2];
	}

	result = 0;

	while (bcm_isxdigit(*cp) &&
	       (value = bcm_isdigit(*cp) ? *cp-'0' : bcm_toupper(*cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}

	if (minus)
		result = (ulong)(result * -1);

	if (endp)
		*endp = (char *)cp;

	return (result);
}
#endif	// ASUS_NVRAM


/* Free all tuples. Should be locked. */
static void
BCMINITFN(nvram_free)(void)
{
	uint i;
	struct nvram_tuple *t, *next;

	/* Free hash table */
	for (i = 0; i < ARRAYSIZE(nvram_hash); i++) {
		for (t = nvram_hash[i]; t; t = next) {
			next = t->next;
			_nvram_free(t);
		}
		nvram_hash[i] = NULL;
	}

	/* Free dead table */
	for (t = nvram_dead; t; t = next) {
		next = t->next;
		_nvram_free(t);
	}
	nvram_dead = NULL;

	/* Indicate to per-port code that all tuples have been freed */
	_nvram_free(NULL);
}

/* String hash */
static INLINE uint
hash(const char *s)
{
	uint hash = 0;

	while (*s)
		hash = 31 * hash + *s++;

	return hash;
}

/* (Re)initialize the hash table. Should be locked. */
static int
BCMINITFN(nvram_rehash)(struct nvram_header *header)
{
#ifdef WL_NVRAM
	wlnv_priv_t dummy;
#endif	/* WL_NVRAM */
	char /*eric--buf[] = "0xXXXXXXXX",*/ *name, *value, *end, *eq;

	/* (Re)initialize hash table */
	nvram_free();

	/* Parse and set "name=value\0 ... \0\0" */
#ifdef WL_NVRAM
	name = read_private_date(header, &dummy);
	end = (char*) header + NVRAM_SPACE - 2 - (name - (char*) header);
#else	/* !WL_NVRAM */
	name = (char *) &header[1];
	end = (char *) header + NVRAM_SPACE - 2;
#endif	/* WL_NVRAM */
	end[0] = end[1] = '\0';
	for (; *name; name = value + strlen(value) + 1) {
		if (!(eq = strchr(name, '=')))
			break;
		*eq = '\0';
		value = eq + 1;
		_nvram_set(name, value);
		*eq = '=';
	}

#if 0 //eric++
	/* Set special SDRAM parameters */
	if (!_nvram_get("sdram_init")) {
		sprintf(buf, "0x%04X", (uint16)(header->crc_ver_init >> 16));
		_nvram_set("sdram_init", buf);
	}
	if (!_nvram_get("sdram_config")) {
		sprintf(buf, "0x%04X", (uint16)(header->config_refresh & 0xffff));
		_nvram_set("sdram_config", buf);
	}
	if (!_nvram_get("sdram_refresh")) {
		sprintf(buf, "0x%04X", (uint16)((header->config_refresh >> 16) & 0xffff));
		_nvram_set("sdram_refresh", buf);
	}
	if (!_nvram_get("sdram_ncdl")) {
		sprintf(buf, "0x%08X", header->config_ncdl);
		_nvram_set("sdram_ncdl", buf);
	}
#endif	// #if 0 //eric++

	return 0;
}

/* Get the value of an NVRAM variable. Should be locked. */
char *
BCMINITFN(_nvram_get)(const char *name)
{
	uint i;
	struct nvram_tuple *t;
	char *value;

	if (!name)
		return NULL;

	/* Hash the name */
	i = hash(name) % ARRAYSIZE(nvram_hash);

	/* Find the associated tuple in the hash table */
	for (t = nvram_hash[i]; t && strcmp(t->name, name); t = t->next);

	value = t ? t->value : NULL;

	return value;
}

/* Set the value of an NVRAM variable. Should be locked. */
int
BCMINITFN(_nvram_set)(const char *name, const char *value)
{
	uint i;
	struct nvram_tuple *t, *u, **prev;

	/* Hash the name */
	i = hash(name) % ARRAYSIZE(nvram_hash);

	/* Find the associated tuple in the hash table */
	for (prev = &nvram_hash[i], t = *prev; t && strcmp(t->name, name);
	     prev = &t->next, t = *prev);

	/* (Re)allocate tuple */
	if (!(u = _nvram_realloc(t, name, value)))
		return -12; /* -ENOMEM */

	/* Value reallocated */
	if (t && t == u)
		return 0;

	/* Move old tuple to the dead table */
	if (t) {
		*prev = t->next;
		t->next = nvram_dead;
		nvram_dead = t;
	}

	/* Add new tuple to the hash table */
	u->next = nvram_hash[i];
	nvram_hash[i] = u;

	return 0;
}

/* Unset the value of an NVRAM variable. Should be locked. */
int
BCMINITFN(_nvram_unset)(const char *name)
{
	uint i;
	struct nvram_tuple *t, **prev;

	if (!name)
		return 0;

	/* Hash the name */
	i = hash(name) % ARRAYSIZE(nvram_hash);

	/* Find the associated tuple in the hash table */
	for (prev = &nvram_hash[i], t = *prev; t && strcmp(t->name, name);
	     prev = &t->next, t = *prev);

	/* Move it to the dead table */
	if (t) {
		*prev = t->next;
		t->next = nvram_dead;
		nvram_dead = t;
	}

	return 0;
}

/* Get all NVRAM variables. Should be locked. */
int
BCMINITFN(_nvram_getall)(char *buf, int count)
{
	uint i;
	struct nvram_tuple *t;
	int len = 0;

	bzero(buf, count);

	/* Write name=value\0 ... \0\0 */
	for (i = 0; i < ARRAYSIZE(nvram_hash); i++) {
		for (t = nvram_hash[i]; t; t = t->next) {
			if ((count - len) > (strlen(t->name) + 1 + strlen(t->value) + 1))
				len += sprintf(buf + len, "%s=%s", t->name, t->value) + 1;
			else
				break;
		}
	}

	return 0;
}

/* Regenerate NVRAM. Should be locked. */
int
BCMINITFN(_nvram_commit)(struct nvram_header *header)
{
	char *init, *config, *refresh, *ncdl;
	char *ptr, *end;
	int i;
	struct nvram_tuple *t;
	struct nvram_header tmp;
	uint8 crc;
#ifdef WL_NVRAM
	wlnv_priv_t *priv = (wlnv_priv_t*) header->private_data;
#endif	/* WL_NVRAM */

	/* Regenerate header */
	header->magic = NVRAM_MAGIC;
	header->crc_ver_init = (NVRAM_VERSION << 8);
	if (!(init = _nvram_get("sdram_init")) ||
	    !(config = _nvram_get("sdram_config")) ||
	    !(refresh = _nvram_get("sdram_refresh")) ||
	    !(ncdl = _nvram_get("sdram_ncdl"))) {
		header->crc_ver_init |= SDRAM_INIT << 16;
		header->config_refresh = SDRAM_CONFIG;
		header->config_refresh |= SDRAM_REFRESH << 16;
		header->config_ncdl = 0;
	} else {
		header->crc_ver_init |= (bcm_strtoul(init, NULL, 0) & 0xffff) << 16;
		header->config_refresh = bcm_strtoul(config, NULL, 0) & 0xffff;
		header->config_refresh |= (bcm_strtoul(refresh, NULL, 0) & 0xffff) << 16;
		header->config_ncdl = bcm_strtoul(ncdl, NULL, 0);
	}

	/* Clear data area */
	ptr = (char *) header + sizeof(struct nvram_header);
	bzero(ptr, NVRAM_SPACE - sizeof(struct nvram_header));

	/* Leave space for a double NUL at the end */
	end = (char *) header + NVRAM_SPACE - 2;

#ifdef WL_NVRAM
	end -= sizeof(wlnv_priv_t);

	priv->length = sizeof(wlnv_priv_t);
	strncpy(priv->magic, "NVP", 3);
	priv->commit_times = g_wlnv.last_commit_times;
	ptr = (char*)priv + sizeof(wlnv_priv_t);
#endif /* WL_NVRAM */

	/* Write out all tuples */
	for (i = 0; i < ARRAYSIZE(nvram_hash); i++) {
		for (t = nvram_hash[i]; t; t = t->next) {
			if ((ptr + strlen(t->name) + 1 + strlen(t->value) + 1) > end)
				break;
			ptr += sprintf(ptr, "%s=%s", t->name, t->value) + 1;
		}
	}

	/* End with a double NUL */
	ptr += 2;

	/* Set new length */
	header->len = ROUNDUP(ptr - (char *) header, 4);

	/* Little-endian CRC8 over the last 11 bytes of the header */
	tmp.crc_ver_init = htol32(header->crc_ver_init);
	tmp.config_refresh = htol32(header->config_refresh);
	tmp.config_ncdl = htol32(header->config_ncdl);
	crc = hndcrc8((uint8 *) &tmp + 9, sizeof(struct nvram_header) - 9, CRC8_INIT_VALUE);

	/* Continue CRC8 over data bytes */
	crc = hndcrc8((uint8 *) &header[1], header->len - sizeof(struct nvram_header), crc);

	/* Set new CRC8 */
	header->crc_ver_init |= crc;

	/* Reinitialize hash table */
	return nvram_rehash(header);
}

/* Initialize hash table. Should be locked. */
int
BCMINITFN(_nvram_init)(void *sb)
{
	struct nvram_header *header;
	int ret;

	if (!(header = (struct nvram_header *) MALLOC(sb_osh(sb), NVRAM_SPACE))) {
		printf("nvram_init: out of memory\n");
		return -12; /* -ENOMEM */
	}

#ifdef ASUS_NVRAM
	if ((ret = _nvram_read((char*)header)) == 0 && header->magic == NVRAM_MAGIC)
#else	// !ASUS_NVRAM
	if ((ret = _nvram_read(header)) == 0 && header->magic == NVRAM_MAGIC)
#endif	// ASUS_NVRAM
		nvram_rehash(header);

	MFREE(sb_osh(sb), header, NVRAM_SPACE);
//eric--	bcm_nvram_cache(sb);
	return ret;
}

/* Free hash table. Should be locked. */
void
BCMINITFN(_nvram_exit)(void)
{
	nvram_free();
}
