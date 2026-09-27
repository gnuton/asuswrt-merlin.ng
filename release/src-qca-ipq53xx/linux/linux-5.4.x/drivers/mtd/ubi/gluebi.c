// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2006
 *
 * Author: Artem Bityutskiy (Битюцкий Артём), Joern Engel
 */

/*
 * This is a small driver which implements fake MTD devices on top of UBI
 * volumes. This sounds strange, but it is in fact quite useful to make
 * MTD-oriented software (including all the legacy software) work on top of
 * UBI.
 *
 * Gluebi emulates MTD devices of "MTD_UBIVOLUME" type. Their minimal I/O unit
 * size (@mtd->writesize) is equivalent to the UBI minimal I/O unit. The
 * eraseblock size is equivalent to the logical eraseblock size of the volume.
 */

#include <linux/err.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/mtd/ubi.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/vmalloc.h>
#include <linux/of_fdt.h>
#include <asus-firmware.h>
#include "ubi.h"
#include "ubi-media.h"

#if defined(CONFIG_FACTORY_CHECKSUM)
#include <linux/mutex.h>
#include "crc32.c"

#define debug(fmt, args...)	do {} while (0)
static unsigned long check_csum_t2 = 0;
#define CHECK_INTERVAL	(600 * HZ)
#endif

static void *g_factory_buf = NULL;

#define err_msg(fmt, ...)                                   \
	pr_err("gluebi (pid %d): %s: " fmt "\n",            \
	       current->pid, __func__, ##__VA_ARGS__)

/**
 * struct gluebi_device - a gluebi device description data structure.
 * @mtd: emulated MTD device description object
 * @refcnt: gluebi device reference count
 * @desc: UBI volume descriptor
 * @ubi_num: UBI device number this gluebi device works on
 * @vol_id: ID of UBI volume this gluebi device works on
 * @list: link in a list of gluebi devices
 */
struct gluebi_device {
	struct mtd_info mtd;
	int refcnt;
	struct ubi_volume_desc *desc;
	int ubi_num;
	int vol_id;
	struct list_head list;
};

struct boot_param_header {
	__be32	magic;			/* magic word OF_DT_HEADER */
	__be32	totalsize;		/* total size of DT block */
	__be32	off_dt_struct;		/* offset to structure */
	__be32	off_dt_strings;		/* offset to strings */
	__be32	off_mem_rsvmap;		/* offset to memory reserve map */
	__be32	version;		/* format version */
	__be32	last_comp_version;	/* last compatible version */
	/* version 2 fields below */
	__be32	boot_cpuid_phys;	/* Physical CPU id we're booting on */
	/* version 3 fields below */
	__be32	dt_strings_size;	/* size of the DT strings block */
	/* version 17 fields below */
	__be32	dt_struct_size;		/* size of the DT structure block */
};

/* List of all gluebi devices */
static LIST_HEAD(gluebi_devices);
static DEFINE_MUTEX(devices_mutex);

/* additional mtd partition */
static struct mtd_partition rootfs = {
	.name = "rootfs",
	.size = 0,
	.offset = 0,
	.mask_flags = MTD_WRITEABLE,
}, rootfs2 = {
	.name = "rootfs2",
	.size = 0,
	.offset = 0,
	.mask_flags = MTD_WRITEABLE,
};

/**
 * find_gluebi_nolock - find a gluebi device.
 * @ubi_num: UBI device number
 * @vol_id: volume ID
 *
 * This function seraches for gluebi device corresponding to UBI device
 * @ubi_num and UBI volume @vol_id. Returns the gluebi device description
 * object in case of success and %NULL in case of failure. The caller has to
 * have the &devices_mutex locked.
 */
static struct gluebi_device *find_gluebi_nolock(int ubi_num, int vol_id)
{
	struct gluebi_device *gluebi;

	list_for_each_entry(gluebi, &gluebi_devices, list)
		if (gluebi->ubi_num == ubi_num && gluebi->vol_id == vol_id)
			return gluebi;
	return NULL;
}

#if defined(CONFIG_FACTORY_CHECKSUM)
#define LEB_SIZE			((128 - 2 * 2) * 1024U)
#define FACTORY_SIZE			(CONFIG_FACTORY_NR_LEB * LEB_SIZE)
#define EEPROM_SET_HEADER_OFFSET	(FACTORY_SIZE - 2 * 1024U)
#define MAX_EEPROM_SET_LENGTH		EEPROM_SET_HEADER_OFFSET
#define NFPAGE_SIZE			2048	/* keep buffer length equal to page size of nand flash */

#ifdef crc32
#undef crc32
#endif

#define FACTORY_IMAGE_MAGIC	0x46545259	/* 'F', 'T', 'R', 'Y' */
typedef struct eeprom_set_hdr_s {
	uint32_t ih_magic;	/* Image Header Magic Number = 'F', 'T', 'R', 'Y' */
	uint32_t ih_hcrc;	/* Image Header CRC Checksum    */
	uint32_t ih_hdr_ver;	/* Image Header Version Number  */
	uint32_t ih_write_ver;	/* Number of writes             */
	uint32_t ih_dcrc;	/* Image Data CRC Checksum      */
} eeprom_set_hdr_t;

struct active_eeprom_set_s {
	struct mutex mutex;
	struct mtd_info *mtd;
	unsigned int id;
	unsigned int attached_mtd_dev;
	unsigned int nr_bad;	/* number of damaged EEPROM set */
	unsigned int ver_diff;	/* rough number of different ver in all EEPROM set */
	unsigned char *buffer;
	int all_sets_damaged;
};

static struct active_eeprom_set_s active_eeprom_set = {
	.mtd = NULL,
	.all_sets_damaged = 0,
}, *act_set = &active_eeprom_set;

static struct factory_vol_s {
	const char *name;
	struct mtd_info *mtd;
} factory_vol_list[] = {
	{ "Factory", NULL },
	{ "Factory2", NULL },
};
#endif

/**
 * gluebi_get_device - get MTD device reference.
 * @mtd: the MTD device description object
 *
 * This function is called every time the MTD device is being opened and
 * implements the MTD get_device() operation. Returns zero in case of success
 * and a negative error code in case of failure.
 */
static int gluebi_get_device(struct mtd_info *mtd)
{
	struct gluebi_device *gluebi;
	int ubi_mode = UBI_READONLY;

	if (mtd->flags & MTD_WRITEABLE)
		ubi_mode = UBI_READWRITE;

	gluebi = container_of(mtd, struct gluebi_device, mtd);
	mutex_lock(&devices_mutex);
	if (gluebi->refcnt > 0) {
		/*
		 * The MTD device is already referenced and this is just one
		 * more reference. MTD allows many users to open the same
		 * volume simultaneously and do not distinguish between
		 * readers/writers/exclusive/meta openers as UBI does. So we do
		 * not open the UBI volume again - just increase the reference
		 * counter and return.
		 */
		gluebi->refcnt += 1;
		mutex_unlock(&devices_mutex);
		return 0;
	}

	/*
	 * This is the first reference to this UBI volume via the MTD device
	 * interface. Open the corresponding volume in read-write mode.
	 */
	gluebi->desc = ubi_open_volume(gluebi->ubi_num, gluebi->vol_id,
				       ubi_mode);
	if (IS_ERR(gluebi->desc)) {
		mutex_unlock(&devices_mutex);
		return PTR_ERR(gluebi->desc);
	}
	gluebi->refcnt += 1;
	mutex_unlock(&devices_mutex);
	return 0;
}

/**
 * gluebi_put_device - put MTD device reference.
 * @mtd: the MTD device description object
 *
 * This function is called every time the MTD device is being put. Returns
 * zero in case of success and a negative error code in case of failure.
 */
static void gluebi_put_device(struct mtd_info *mtd)
{
	struct gluebi_device *gluebi;

	gluebi = container_of(mtd, struct gluebi_device, mtd);
	mutex_lock(&devices_mutex);
	gluebi->refcnt -= 1;
	if (gluebi->refcnt == 0)
		ubi_close_volume(gluebi->desc);
	mutex_unlock(&devices_mutex);
}

/**
 * gluebi_read - read operation of emulated MTD devices.
 * @mtd: MTD device description object
 * @from: absolute offset from where to read
 * @len: how many bytes to read
 * @retlen: count of read bytes is returned here
 * @buf: buffer to store the read data
 *
 * This function returns zero in case of success and a negative error code in
 * case of failure.
 */
static int gluebi_read(struct mtd_info *mtd, loff_t from, size_t len,
		       size_t *retlen, unsigned char *buf)
{
	int err = 0, lnum, offs, bytes_left;
	struct gluebi_device *gluebi;

	gluebi = container_of(mtd, struct gluebi_device, mtd);
	lnum = div_u64_rem(from, mtd->erasesize, &offs);
	bytes_left = len;
	while (bytes_left) {
		size_t to_read = mtd->erasesize - offs;

		if (to_read > bytes_left)
			to_read = bytes_left;

		err = ubi_read(gluebi->desc, lnum, buf, offs, to_read);
		if (err)
			break;

		lnum += 1;
		offs = 0;
		bytes_left -= to_read;
		buf += to_read;
	}

	*retlen = len - bytes_left;
	return err;
}

/**
 * gluebi_write - write operation of emulated MTD devices.
 * @mtd: MTD device description object
 * @to: absolute offset where to write
 * @len: how many bytes to write
 * @retlen: count of written bytes is returned here
 * @buf: buffer with data to write
 *
 * This function returns zero in case of success and a negative error code in
 * case of failure.
 */
static int gluebi_write(struct mtd_info *mtd, loff_t to, size_t len,
			size_t *retlen, const u_char *buf)
{
	int err = 0, lnum, offs, bytes_left;
	struct gluebi_device *gluebi;
	struct ubi_volume *vol;

	gluebi = container_of(mtd, struct gluebi_device, mtd);
	lnum = div_u64_rem(to, mtd->erasesize, &offs);

	if (len % mtd->writesize || offs % mtd->writesize) {
		printk(KERN_ERR "%s: len 0x%zx and/or offs 0x%x not aligned to writesize 0x%x boundary. reject!\n",
			__func__, len, offs, mtd->writesize);
		return -EINVAL;
	}

	vol = gluebi->desc->vol;
	if (vol->upd_marker) {
		printk(KERN_WARNING "%s: reset upd_marker of volume [%s]!\n",
			__func__, vol->name);
		vol->upd_marker = 0;
	}
	bytes_left = len;
	while (bytes_left) {
		size_t to_write = mtd->erasesize - offs;

		if (to_write > bytes_left)
			to_write = bytes_left;

		err = ubi_leb_write(gluebi->desc, lnum, buf, offs, to_write);
		if (err)
			break;

		lnum += 1;
		offs = 0;
		bytes_left -= to_write;
		buf += to_write;
	}

	*retlen = len - bytes_left;
	return err;
}

/**
 * gluebi_erase - erase operation of emulated MTD devices.
 * @mtd: the MTD device description object
 * @instr: the erase operation description
 *
 * This function calls the erase callback when finishes. Returns zero in case
 * of success and a negative error code in case of failure.
 */
static int gluebi_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int err, i, lnum, count;
	struct gluebi_device *gluebi;
	struct ubi_volume *vol;

	if (mtd_mod_by_ws(instr->addr, mtd) || mtd_mod_by_ws(instr->len, mtd))
		return -EINVAL;

	lnum = mtd_div_by_eb(instr->addr, mtd);
	count = mtd_div_by_eb(instr->len, mtd);
	gluebi = container_of(mtd, struct gluebi_device, mtd);

	vol = gluebi->desc->vol;
	if (vol->upd_marker) {
		printk(KERN_WARNING "%s: reset upd_marker of volume [%s]!\n",
			__func__, vol->name);
		vol->upd_marker = 0;
	}
	for (i = 0; i < count - 1; i++) {
		err = ubi_leb_unmap(gluebi->desc, lnum + i);
		if (err)
			goto out_err;
	}
	/*
	 * MTD erase operations are synchronous, so we have to make sure the
	 * physical eraseblock is wiped out.
	 *
	 * Thus, perform leb_erase instead of leb_unmap operation - leb_erase
	 * will wait for the end of operations
	 */
	err = ubi_leb_erase(gluebi->desc, lnum + i);
	if (err)
		goto out_err;

	return 0;

out_err:
	instr->fail_addr = (long long)lnum * mtd->erasesize;
	return err;
}

#if defined(CONFIG_FACTORY_CHECKSUM)
/* Whether MTD device is Factory or not.
 * @return:
 * 	1:	is factory
 * otherwise:	invalid parameter, not factory
 */
static int is_factory_mtd_device(struct mtd_info *mtd)
{
	int i, ret = 0;
	struct factory_vol_s *p;

	if (!mtd)
		return 0;

	for (i = 0, p = &factory_vol_list[0]; !ret && i < ARRAY_SIZE(factory_vol_list); ++i, ++p) {
		if (!strcmp(mtd->name, p->name))
			ret = 1;
	}

	return ret;
}

/* Compare two EEPROM set header
 * @return:
 * 	0:	success
 *     -1:	Invalid parameter.
 *  otherwise:	Two EEPROM set header is not equal to each other.
 */
static inline int compare_eeprom_set_header(eeprom_set_hdr_t *hdr, eeprom_set_hdr_t *hdr2)
{
	if (!hdr || !hdr2)
		return -1;

	if (hdr->ih_magic != hdr2->ih_magic || hdr->ih_hdr_ver != hdr2->ih_hdr_ver ||
	    hdr->ih_write_ver != hdr2->ih_write_ver ||
	    hdr->ih_dcrc != hdr2->ih_dcrc || hdr->ih_hcrc != hdr2->ih_hcrc)
		return 1;

	return 0;
}

static int convert_check_result_to_msg(int w, char *buf, size_t buf_len)
{
	if (!buf || !buf_len)
		return -1;

	if (w == -2)
		snprintf(buf, buf_len, "%s", "Invalid magic number");
	else if (w == -3)
		snprintf(buf, buf_len, "%s", "Invalid header checksum");
	else if (w == -4)
		snprintf(buf, buf_len, "%s", "Invalid data checksum");
	else
		snprintf(buf, buf_len, "w = %d", w);

	return 0;
}

/* Check one EEPROM set checksum in Flash or RAM.
 * @mtd:	pointer to struct_mtdinfo
 * @id:		EEPROM set id in the MTD device
 * @return:
 *     >=0:	OK, write times of the EEPROM set
 * 	-1:	Invalid parameter.
 * 	-2:	Invalid magic number.
 * 	-3:	Invalid header checksum.
 * 	-4:	Invalid data checksum
 * 	-5:	Allocate memory fail.
 *  otherwise:	Not defined.
 */
static int __check_eeprom_set_checksum(struct mtd_info *mtd, unsigned int id, unsigned char *buf)
{
	int ret, in_ram = 0;
	loff_t o, offset;
	size_t rdlen;
	unsigned char *p;
	unsigned long hcrc, checksum;
	eeprom_set_hdr_t header, *hdr = &header;

	offset = id * FACTORY_SIZE;
	if ((!buf && (!mtd || offset >= mtd->size)) || (buf && mtd)) {
		printk(KERN_DEBUG "%s: mtd %p id %d buf %p. return -1\n",
			__func__, mtd, id, buf);
		return -1;
	}

	in_ram = !!buf;
	ret = gluebi_read(mtd, offset + EEPROM_SET_HEADER_OFFSET, sizeof(*hdr), &rdlen, (unsigned char *)hdr);
	if (ret || rdlen != sizeof(*hdr)) {
		printk("%s: read data from 0x%llx of mtd %s fail. (ret = %d, rdlen %zx)\n",
			__func__, offset + EEPROM_SET_HEADER_OFFSET, mtd->name, ret, rdlen);
		return ret;
	}
	if (hdr->ih_magic != htonl(FACTORY_IMAGE_MAGIC))
		return -2;

	hcrc = ntohl(hdr->ih_hcrc);
	hdr->ih_hcrc = 0;
	/* check header checksum */
	checksum = crc32(0, (const char *)hdr, sizeof(*hdr));
	if (hcrc != checksum) {
		debug("Header checksum mismatch. (%lx/%lx)\n", hcrc, checksum);
		return -3;
	}

	p = act_set->buffer;
	/* check image checksum */
	for (o = 0, checksum = 0; o < EEPROM_SET_HEADER_OFFSET; o += NFPAGE_SIZE) {
		if (!in_ram) {
			ret = gluebi_read(mtd, offset + o, NFPAGE_SIZE, &rdlen, p);
			if (ret || rdlen != NFPAGE_SIZE) {
				printk("%s: read data from 0x%llx of mtd %s fail. (ret = %d)\n",
					__func__, offset + o, mtd->name, ret);
				goto out_chk_checksum;
			}
		}

		checksum = crc32(checksum, p, NFPAGE_SIZE);

		if (in_ram)
			p += NFPAGE_SIZE;
	}
	if (ntohl(hdr->ih_dcrc) != checksum) {
		debug("Data checksum mismatch. (%x/%lx)\n", ntohl(hdr->ih_dcrc), checksum);
		ret = -4;
		goto out_chk_checksum;
	}
	ret = ntohl(hdr->ih_write_ver);

out_chk_checksum:

	return ret;
}

/* Check one EEPROM set checksum in RAM. */
static inline int __check_eeprom_set_in_ram(unsigned char *buf)
{
	return __check_eeprom_set_checksum(NULL, 0, buf);
}

/* Check one EEPROM set checksum in Flash. */
static inline int __check_eeprom_set_in_flash(struct mtd_info *mtd, unsigned int id)
{
	return __check_eeprom_set_checksum(mtd, id, NULL);
}

/* List all EEPROM set for debug.  */
#if defined(DEBUG_FACTORY_CHECKSUM)
static void __list_all_eeprom_set_status(void)
{
	char buf[128];
	int i, j, c, w, rem;
	struct factory_vol_s *p;

	printk("active EEPROM set: [%s-%d]\n", act_set->mtd->name, act_set->id);
	for (j = 0, p = &factory_vol_list[0]; j < ARRAY_SIZE(factory_vol_list); ++j, ++p) {
		if (!p->mtd) {
			debug("%s: mtd_info of %s not defined!\n", __func__, p->name);
			continue;
		}

		c = div_u64_rem(p->mtd->size, FACTORY_SIZE, &rem);
		for (i = 0; i < c; ++i) {
			w = __check_eeprom_set_in_flash(p->mtd, i);
			if (w >= 0) {
				printk("[%s-%d]: OK (ver: %d)\n", p->mtd->name, i, w);
			} else {
				convert_check_result_to_msg(w, buf, sizeof(buf));
				printk("[%s-%d]: DAMAGED (%s)\n", p->mtd->name, i, buf);
			}
		}
	}
}
#else
static inline void __list_all_eeprom_set_status(void) { }
#endif

/* Check all EEPROM set in Factory, Factory2 volume and select correct and latest one.
 * Caller must hold act_set->mutex
 * @return:
 * 	0:	Success. Latest and correct EEPROM set is copied to RAM.
 *  -ENOENT:	All EEPROM set is damaged. (always choose first EEPROM set)
 *  -EINVAL:	Get MTD device fail.
 */
static int __choose_active_eeprom_set(void)
{
	int i, j, w, ret = 0, c, rem, id = -1;
	char buf[128];
	unsigned int max_w = 0;
	const char *mtd_name = NULL;
	struct mtd_info *mtd;
	struct factory_vol_s *p;

	act_set->nr_bad = act_set->ver_diff = 0;
	for (j = 0, p = &factory_vol_list[0]; j < ARRAY_SIZE(factory_vol_list); ++j, ++p) {
		if (!p->mtd) {
			mtd = get_mtd_device_nm(p->name);
			if (IS_ERR(mtd)) {
				debug("%s: Can't get mtd_info of %s\n", __func__, p->name);
				continue;
			}
			p->mtd = mtd;
		}

		c = div_u64_rem(p->mtd->size, FACTORY_SIZE, &rem);
		for (i = 0; i < c; ++i) {
			w = __check_eeprom_set_in_flash(p->mtd, i);
			if (w >= 0) {
				printk("[%s-%d]: OK (ver: %d)\n", p->mtd->name, i, w);
				if (max_w && w != max_w)
					act_set->ver_diff++;
				if (w > max_w || (id < 0 && !max_w)) {
					mtd_name = p->name;
					id = i;
					max_w = w;
				}
			} else {
				act_set->nr_bad++;
				convert_check_result_to_msg(w, buf, sizeof(buf));
				printk("[%s-%d]: DAMAGED (%s)\n", p->mtd->name, i, buf);
			}
		}
	}

	/* All EEPROM set is damaged. choose first one. */
	act_set->all_sets_damaged = 0;
	if (id < 0) {
		ret = -ENOENT;
		mtd_name = factory_vol_list[0].name;
		id = 0;
		act_set->all_sets_damaged = 1;
		printk(KERN_ERR "%s: All EEPROM sets are damaged. Choose first one.\n", __func__);
	}

	if (act_set->mtd) {
		put_mtd_device(act_set->mtd);
		act_set->mtd = NULL;
	}
	mtd = get_mtd_device_nm(mtd_name);
	if (IS_ERR(act_set->mtd)) {
		debug("%s: Can't get mtd_info of %s\n", __func__, mtd_name);
		return -EINVAL;
	}
	act_set->mtd = mtd;
	act_set->id = id;
	printk("Select [%s-%d]\n", act_set->mtd->name, act_set->id);
	check_csum_t2 = jiffies + CHECK_INTERVAL;

	return ret;
}

/* Reload EEPROM set if necessary. If actived EEPROM set in Flash is damaged too, choose again.
 * Caller must hold act_set->mutex.
 * @return:
 *  NULL:	Error happen.
 *  otherwise:	pointer to mtd_info for active EEPROM set.
 */
static struct mtd_info *__reload_eeprom_set(void)
{
	int w;
	char buf[128];

	if (!act_set->mtd) {
		__choose_active_eeprom_set();
	} else if (!check_csum_t2 || time_after(jiffies, check_csum_t2)) {
		check_csum_t2 = jiffies + CHECK_INTERVAL;
		w = __check_eeprom_set_in_flash(act_set->mtd, act_set->id);
		if (w < 0) {
			convert_check_result_to_msg(w, buf, sizeof(buf));
			printk("[%s-%d] is damaged! (%s, all_sets_damaged %d)\n",
				(act_set->mtd)? act_set->mtd->name:"<NULL>", act_set->id, buf,
				act_set->all_sets_damaged);
			if (!act_set->all_sets_damaged) {
				printk(KERN_DEBUG "Select new one.\n");
				__choose_active_eeprom_set();
			}
		}
	}
	return act_set->mtd;
}

/* Update header checksum, data checksum, write times, etc.
 * Caller must hold act_set->mutex
 * @buf:	pointer to a EEPROM set
 * @return:
 * 	0:	Success
 * 	-1:	Invalid parameter.
 */
static int __update_eeprom_checksum(unsigned char *buf)
{
	eeprom_set_hdr_t *hdr;
	unsigned long checksum, dcrc;

	if (!buf)
		return -1;

	/* calculate image checksum */
	dcrc = crc32(0, buf, MAX_EEPROM_SET_LENGTH);

	hdr = (eeprom_set_hdr_t *)(buf + EEPROM_SET_HEADER_OFFSET);
	/* reset write version to 0 if header magic number is incorrect or wrap */
	if (hdr->ih_magic != htonl(FACTORY_IMAGE_MAGIC) ||
	    ntohl(hdr->ih_write_ver) >= 0x7FFFFFFFU)
		hdr->ih_write_ver = htonl(0);

	/* Build new header */
	hdr->ih_magic = htonl(FACTORY_IMAGE_MAGIC);
	hdr->ih_hcrc = 0;
	hdr->ih_hdr_ver = htonl(1);
	hdr->ih_write_ver = htonl(ntohl(hdr->ih_write_ver) + 1);
	hdr->ih_dcrc = htonl(dcrc);

	checksum = crc32(0, (const char *)hdr, sizeof(*hdr));
	hdr->ih_hcrc = htonl(checksum);

	printk(KERN_DEBUG "header/data checksum: 0x%08x/0x%08x (ver: %d)\n",
		ntohl(hdr->ih_hcrc), ntohl(hdr->ih_dcrc), ntohl(hdr->ih_write_ver));

	return 0;
}

/* Write EEPROM set in RAM to all factory volume except active EEPROM set.
 * Caller must hod act_set->mutex
 * @buf:	pointer to active EEPROM set in RAM
 * @return:
 *     >0:	number erase/write to EEPROM set is failed.
 * 	0:	Success.
 *     -1:	Invalid parameter.
 *     -2:	Read header from active EEPROM set fail.
 */
static int __sync_eeprom_sets(unsigned char *buf)
{
	int i, j, c, err = 0, err1 = 0, ret = 0, rem;
	loff_t offset;
	size_t wrlen = 0;
	struct erase_info instr;
	struct mtd_info *mtd;
	struct factory_vol_s *p;

	if (!buf)
		return -1;

	for (j = 0, p = &factory_vol_list[0]; j < ARRAY_SIZE(factory_vol_list); ++j, ++p) {
		if (!p->mtd) {
			mtd = get_mtd_device_nm(p->name);
			if (IS_ERR(mtd)) {
				debug("%s: Can't get mtd_info of %s\n", __func__, p->name);
				continue;
			}
			p->mtd = mtd;
		}

		c = div_u64_rem(p->mtd->size, FACTORY_SIZE, &rem);
		for (i = 0; i < c; ++i) {
			if (i == act_set->id && !strcmp(act_set->mtd->name, p->name))
				continue;

			offset = i * FACTORY_SIZE;
			memset(&instr, 0, sizeof(instr));
			instr.addr = offset;
			instr.len = FACTORY_SIZE;
			err = gluebi_erase(p->mtd, &instr);
			if (err) {
				printk("%s: erase [%s-%d] fail! (err %d)\n",
					__func__, p->mtd->name, i, err);
				ret++;
			}
			err1 = gluebi_write(p->mtd, offset, FACTORY_SIZE, &wrlen, buf);
			if (err1 || wrlen != FACTORY_SIZE) {
				ret++;
				printk("%s: write to [%s-%d] fail!. (err %d)\n",
					__func__, p->mtd->name, i, err1);
			}
			if (!err && !err1)
				printk("Sync active EEPROM set to [%s-%d] OK.\n", p->mtd->name, i);
		}
	}

	return ret;
}

/**
 * factory_read - read factory configuration from active EEPROM set.
 * @mtd: MTD device description object
 * @from: absolute offset from where to read
 * @len: how many bytes to read
 * @retlen: count of read bytes is returned here
 * @buf: buffer to store the read data
 *
 * This function returns zero in case of success and a negative error code in
 * case of failure.
 *
 * If from < FACTORY_SIZE, read factory configuration from active EEPROM set.
 * If from >= FACTORY_SIZE, read raw data from specified MTD device. from would be minus FACTORY_SIZE.
 */
static int factory_read(struct mtd_info *mtd, loff_t from, size_t len,
		       size_t *retlen, unsigned char *buf)
{
	int err = 0;
	size_t rdlen = 0;
	struct mtd_info *factory_mtd;

	if (len < 0 || from < 0 ||
	    (from < FACTORY_SIZE && from + len > mtd->size) ||
	    (from >= FACTORY_SIZE && from + len > (mtd->size + FACTORY_SIZE)))
		return -EINVAL;

	if (from >= FACTORY_SIZE)
		return gluebi_read(mtd, from - FACTORY_SIZE, len, retlen, buf);

	/* choose latest and correct EEPROM set if necessary, return data to user. */
	mutex_lock(&act_set->mutex);
	factory_mtd = __reload_eeprom_set();
	if (!factory_mtd)
		return -EINVAL;

	err = gluebi_read(factory_mtd, from, len, &rdlen, buf);
	if (!err)
		*retlen = rdlen;
	__list_all_eeprom_set_status();
	mutex_unlock(&act_set->mutex);

	return err;
}

/**
 * factory_write - write to active EEPROM set and sync to another EEPROM sets.
 * @mtd: MTD device description object
 * @to: absolute offset where to write
 * @len: how many bytes to write
 * @retlen: count of written bytes is returned here
 * @buf: buffer with data to write
 *
 * This function returns zero in case of success and a negative error code in
 * case of failure.
 *
 * Write raw data to Factory* MTD device is inhibited.
 */
static int factory_write(struct mtd_info *mtd, loff_t to, size_t len,
			size_t *retlen, const u_char *buf)
{
	int ret, err = 0;
	size_t rdlen = 0, wrlen = 0;
	loff_t offset;
	struct mtd_info *factory_mtd;
	struct erase_info instr;
	unsigned char *p = NULL;

	if (len < 0 || to < 0 || len + to > mtd->size)
		return -EINVAL;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;

	/* choose latest and correct EEPROM set if necessary, return data to user. */
	mutex_lock(&act_set->mutex);
	factory_mtd = __reload_eeprom_set();
	if (!factory_mtd)
		return -EINVAL;

	p = g_factory_buf;
	/* read actived EEPROM set to RAM */
	offset = act_set->id * FACTORY_SIZE;
	ret = gluebi_read(factory_mtd, offset, FACTORY_SIZE, &rdlen, p);
	if (ret || rdlen != FACTORY_SIZE) {
		printk("%s: read data from 0x%llx of mtd %s fail. (ret = %d)\n",
			__func__, offset, factory_mtd->name, ret);
		goto out_factory_write;
	}
	memcpy(p + to, buf, len);

	/* update checksum */
	__update_eeprom_checksum(p);

	/* erase active EEPROM set */
	memset(&instr, 0, sizeof(instr));
	instr.addr = offset;
	instr.len = FACTORY_SIZE;
	err = gluebi_erase(factory_mtd, &instr);
	if (err) {
		printk("%s: erase *[%s-%d] fail! (err %d)\n",
			__func__, act_set->mtd->name, act_set->id, err);
		goto out_factory_write;
	}

	/* write to active EEPROM set */
	err = gluebi_write(factory_mtd, offset, FACTORY_SIZE, &wrlen, p);
	if (err || wrlen != FACTORY_SIZE) {
		printk("%s: write to *[%s-%d] fail!. (err %d)\n",
			__func__, act_set->mtd->name, act_set->id, err);
	}
	act_set->all_sets_damaged = 0;
	*retlen = len;	/* cheat caller */

	/* sync to another EEPROM set */
	__sync_eeprom_sets(p);

	__list_all_eeprom_set_status();

out_factory_write:
	mutex_unlock(&act_set->mutex);

	return err;
}

/**
 * factory_erase - erase operation of emulated MTD devices.
 * @mtd: the MTD device description object
 * @instr: the erase operation description
 *
 * This function calls the erase callback when finishes. Returns zero in case
 * of success and a negative error code in case of failure.
 */
static int factory_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	if (instr->addr < 0 || instr->addr > mtd->size - mtd->erasesize)
		return -EINVAL;
	if (instr->len < 0 || instr->addr + instr->len > mtd->size)
		return -EINVAL;
	if (mtd_mod_by_ws(instr->addr, mtd) || mtd_mod_by_ws(instr->len, mtd))
		return -EINVAL;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;

	return 0;
}
#endif	/* CONFIG_FACTORY_CHECKSUM */

/**
 * Create read-only MTD partition for rootfilesystem.
 * @return
 * 	0:	success or nothing happen
 *     -1:	invalid parameter
 *     -2:	read image header failed.
 */
static int create_rootfs_partition(struct mtd_info *mtd,
				   struct ubi_device_info *di,
				   struct ubi_volume_info *vi)
{
	struct mtd_partition *part = NULL;
	int i, r;
	size_t retlen;
	struct boot_param_header blob;
	image_header_t hdr;
	version_t *hw = &hdr.u.tail.hw[0];
	uint32_t rfs_offset = 0, dtb_len = 0;
	union {
		uint32_t rfs_offset_net_endian;
		uint8_t p[4];
	} u;

	if (!mtd || !di || !vi)
		return -1;

	if (!strcmp(vi->name, "linux"))
		part = &rootfs;
	if (!strcmp(vi->name, "linux2"))
		part = &rootfs2;

	if (!part)
		return 0;

	/* Create rootfs or rootfs2 MTD partition */
	part->offset = 0;
	part->size = mtd->size;
	part->mask_flags |= MTD_WRITEABLE;
	r = mtd->_read(mtd, 0, 0x40, &retlen, (unsigned char*) &hdr);
	if (r || retlen != sizeof(hdr)) {
		printk(KERN_WARNING "%s: read image header fail. (r 0x%x retlen %zu)\n",
			__func__, r, retlen);
	}
	else {
		/* Looking for rootfilesystem offset */
		u.rfs_offset_net_endian = 0;
		for (i = 0; i < (MAX_VER*2); ++i, ++hw) {
			if (hw->major != ROOTFS_OFFSET_MAGIC || (hw + 1)->minor & 0x3)
				continue;
			u.p[1] = hw->minor;
			hw++;
			u.p[2] = hw->major;
			u.p[3] = hw->minor;
			rfs_offset = ntohl(u.rfs_offset_net_endian);
		}

		if (rfs_offset != 0) {
			r = mtd->_read(mtd, rfs_offset, sizeof(blob), &retlen, (unsigned char*) &blob);
			if (r || retlen != sizeof(blob)) {
				printk(KERN_WARNING "%s: read blob header fail. "
					"(r 0x%x retlen %zu)\n", __func__, r, retlen);
			} else {
				if (be32_to_cpu(blob.magic) == OF_DT_HEADER) {
					dtb_len = be32_to_cpu(blob.totalsize);
					printk(KERN_INFO "Found DTB, length %x\n", dtb_len);
					dtb_len += 63;
					dtb_len &= ~((1UL << 6) - 1);
					rfs_offset += dtb_len;
				}
			}
			part->offset = rfs_offset;
			part->size = mtd->size - rfs_offset;
		}
	}

	printk(KERN_DEBUG "volume %s rfs_offset %x mtd->size %lx\n",
		vi->name, rfs_offset, (unsigned long)mtd->size);
	mtd_device_register(mtd, part, 1);

	return 0;
}

/**
 * gluebi_create - create a gluebi device for an UBI volume.
 * @di: UBI device description object
 * @vi: UBI volume description object
 *
 * This function is called when a new UBI volume is created in order to create
 * corresponding fake MTD device. Returns zero in case of success and a
 * negative error code in case of failure.
 */
static int gluebi_create(struct ubi_device_info *di,
			 struct ubi_volume_info *vi)
{
	struct gluebi_device *gluebi, *g;
	struct mtd_info *mtd;

#if defined(CONFIG_UBIFS_FS)
	if (!strcmp(vi->name, "jffs2")) {
		printk(KERN_INFO "%s: skip jffs2 volume\n", __func__);
		return 0;
	}
#endif

	gluebi = kzalloc(sizeof(struct gluebi_device), GFP_KERNEL);
	if (!gluebi)
		return -ENOMEM;

	mtd = &gluebi->mtd;
	mtd->name = kmemdup(vi->name, vi->name_len + 1, GFP_KERNEL);
	if (!mtd->name) {
		kfree(gluebi);
		return -ENOMEM;
	}

	gluebi->vol_id = vi->vol_id;
	gluebi->ubi_num = vi->ubi_num;
	mtd->type = MTD_UBIVOLUME;
	if (!di->ro_mode)
		mtd->flags = MTD_WRITEABLE;
	mtd->owner      = THIS_MODULE;
	mtd->writesize  = di->min_io_size;
	mtd->erasesize  = vi->usable_leb_size;
	mtd->_read       = gluebi_read;
	mtd->_write      = gluebi_write;
	mtd->_erase      = gluebi_erase;
	mtd->_get_device = gluebi_get_device;
	mtd->_put_device = gluebi_put_device;

#if defined(CONFIG_FACTORY_CHECKSUM)
	if (is_factory_mtd_device(mtd)) {
		printk("%s: Hook read/write method of %s.\n", __func__, mtd->name);
		mtd->_read = factory_read;
		mtd->_write = factory_write;
		mtd->_erase = factory_erase;
	}
#endif

	/*
	 * In case of dynamic a volume, MTD device size is just volume size. In
	 * case of a static volume the size is equivalent to the amount of data
	 * bytes.
	 */
	if (vi->vol_type == UBI_DYNAMIC_VOLUME)
		mtd->size = (unsigned long long)vi->usable_leb_size * vi->size;
	else
		mtd->size = vi->used_bytes;

	/* Just a sanity check - make sure this gluebi device does not exist */
	mutex_lock(&devices_mutex);
	g = find_gluebi_nolock(vi->ubi_num, vi->vol_id);
	if (g)
		err_msg("gluebi MTD device %d form UBI device %d volume %d already exists",
			g->mtd.index, vi->ubi_num, vi->vol_id);
	mutex_unlock(&devices_mutex);

	if (mtd_device_register(mtd, NULL, 0)) {
		err_msg("cannot add MTD device");
		kfree(mtd->name);
		kfree(gluebi);
		return -ENFILE;
	}

	mtd->_get_device(mtd);
	create_rootfs_partition(mtd, di, vi);

	mutex_lock(&devices_mutex);
	list_add_tail(&gluebi->list, &gluebi_devices);
	mutex_unlock(&devices_mutex);

#if defined(CONFIG_FACTORY_CHECKSUM)
	if (is_factory_mtd_device(mtd)) {
		/* If all factory mtd devices are attached and
		 * one of EEPROM is damaged or write version is different,
		 * sync all EEPROM set to latest and correct one.
		 */
		mutex_lock(&act_set->mutex);
		act_set->attached_mtd_dev++;
		if (act_set->attached_mtd_dev >= ARRAY_SIZE(factory_vol_list)) {
			int ret;
			size_t rdlen = 0;
			unsigned char *p;
			struct mtd_info *factory_mtd;

			factory_mtd = __reload_eeprom_set();
			if (factory_mtd &&
			    (act_set->nr_bad || act_set->ver_diff) &&
			    !act_set->all_sets_damaged)
			{
				act_set->nr_bad = act_set->ver_diff = 0;
				p = g_factory_buf;
				ret = gluebi_read(factory_mtd, act_set->id * FACTORY_SIZE, FACTORY_SIZE, &rdlen, p);
				if (!ret && rdlen == FACTORY_SIZE)
					__sync_eeprom_sets(p);
			}
		}
		mutex_unlock(&act_set->mutex);
	}
#endif
	return 0;
}

/**
 * gluebi_remove - remove a gluebi device.
 * @vi: UBI volume description object
 *
 * This function is called when an UBI volume is removed and it removes
 * corresponding fake MTD device. Returns zero in case of success and a
 * negative error code in case of failure.
 */
static int gluebi_remove(struct ubi_volume_info *vi)
{
	int err = 0;
	struct mtd_info *mtd;
	struct gluebi_device *gluebi;

	mutex_lock(&devices_mutex);
	gluebi = find_gluebi_nolock(vi->ubi_num, vi->vol_id);
	if (!gluebi) {
		err_msg("got remove notification for unknown UBI device %d volume %d",
			vi->ubi_num, vi->vol_id);
		err = -ENOENT;
	} else if (gluebi->refcnt)
		err = -EBUSY;
	else
		list_del(&gluebi->list);
	mutex_unlock(&devices_mutex);
	if (err)
		return err;

	mtd = &gluebi->mtd;
	mtd->_put_device(mtd);
	err = mtd_device_unregister(mtd);
	if (err) {
		err_msg("cannot remove fake MTD device %d, UBI device %d, volume %d, error %d",
			mtd->index, gluebi->ubi_num, gluebi->vol_id, err);
		mutex_lock(&devices_mutex);
		list_add_tail(&gluebi->list, &gluebi_devices);
		mutex_unlock(&devices_mutex);
		return err;
	}

#if defined(CONFIG_FACTORY_CHECKSUM)
	if (is_factory_mtd_device(mtd)) {
		mutex_lock(&act_set->mutex);
		act_set->attached_mtd_dev--;
		mutex_unlock(&act_set->mutex);
	}
#endif

	kfree(mtd->name);
	kfree(gluebi);
	return 0;
}

/**
 * gluebi_updated - UBI volume was updated notifier.
 * @vi: volume info structure
 *
 * This function is called every time an UBI volume is updated. It does nothing
 * if te volume @vol is dynamic, and changes MTD device size if the
 * volume is static. This is needed because static volumes cannot be read past
 * data they contain. This function returns zero in case of success and a
 * negative error code in case of error.
 */
static int gluebi_updated(struct ubi_volume_info *vi)
{
	struct gluebi_device *gluebi;

	mutex_lock(&devices_mutex);
	gluebi = find_gluebi_nolock(vi->ubi_num, vi->vol_id);
	if (!gluebi) {
		mutex_unlock(&devices_mutex);
		err_msg("got update notification for unknown UBI device %d volume %d",
			vi->ubi_num, vi->vol_id);
		return -ENOENT;
	}

	if (vi->vol_type == UBI_STATIC_VOLUME)
		gluebi->mtd.size = vi->used_bytes;
	mutex_unlock(&devices_mutex);
	return 0;
}

/**
 * gluebi_resized - UBI volume was re-sized notifier.
 * @vi: volume info structure
 *
 * This function is called every time an UBI volume is re-size. It changes the
 * corresponding fake MTD device size. This function returns zero in case of
 * success and a negative error code in case of error.
 */
static int gluebi_resized(struct ubi_volume_info *vi)
{
	struct gluebi_device *gluebi;

	mutex_lock(&devices_mutex);
	gluebi = find_gluebi_nolock(vi->ubi_num, vi->vol_id);
	if (!gluebi) {
		mutex_unlock(&devices_mutex);
		err_msg("got update notification for unknown UBI device %d volume %d",
			vi->ubi_num, vi->vol_id);
		return -ENOENT;
	}
	gluebi->mtd.size = vi->used_bytes;
	mutex_unlock(&devices_mutex);
	return 0;
}

/**
 * gluebi_notify - UBI notification handler.
 * @nb: registered notifier block
 * @l: notification type
 * @ptr: pointer to the &struct ubi_notification object
 */
static int gluebi_notify(struct notifier_block *nb, unsigned long l,
			 void *ns_ptr)
{
	struct ubi_notification *nt = ns_ptr;

	switch (l) {
	case UBI_VOLUME_ADDED:
		gluebi_create(&nt->di, &nt->vi);
		break;
	case UBI_VOLUME_REMOVED:
		gluebi_remove(&nt->vi);
		break;
	case UBI_VOLUME_RESIZED:
		gluebi_resized(&nt->vi);
		break;
	case UBI_VOLUME_UPDATED:
		gluebi_updated(&nt->vi);
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block gluebi_notifier = {
	.notifier_call	= gluebi_notify,
};

static int __init ubi_gluebi_init(void)
{
#if defined(CONFIG_FACTORY_CHECKSUM)
	act_set->buffer = kmalloc(NFPAGE_SIZE, GFP_KERNEL);
	if (!act_set->buffer) {
		printk("Allocate %d bytes fail.\n", NFPAGE_SIZE);
		return -ENOMEM;
	}
	mutex_init(&act_set->mutex);
#endif

	g_factory_buf = kmalloc(FACTORY_SIZE, GFP_KERNEL);
	if (!g_factory_buf) {
		printk(KERN_ERR "%s: allocate 0x%x bytes fail!\n",
			__func__, FACTORY_SIZE);
		return -ENOMEM;
	}

	return ubi_register_volume_notifier(&gluebi_notifier, 0);
}

static void __exit ubi_gluebi_exit(void)
{
	struct gluebi_device *gluebi, *g;

	kfree(g_factory_buf);
	g_factory_buf = NULL;

#if defined(CONFIG_FACTORY_CHECKSUM)
	mutex_lock(&act_set->mutex);
	if (act_set->mtd) {
		put_mtd_device(act_set->mtd);
		act_set->mtd = NULL;
		act_set->id = -1;
	}
	if (act_set->buffer) {
		kfree(act_set->buffer);
		act_set->buffer = NULL;
	}
	mutex_unlock(&act_set->mutex);
#endif
	list_for_each_entry_safe(gluebi, g, &gluebi_devices, list) {
		int err;
		struct mtd_info *mtd = &gluebi->mtd;

		mtd->_put_device(mtd);
		mtd->_put_device(mtd);
		err = mtd_device_unregister(mtd);
		if (err)
			err_msg("error %d while removing gluebi MTD device %d, UBI device %d, volume %d - ignoring",
				err, mtd->index, gluebi->ubi_num,
				gluebi->vol_id);
		kfree(mtd->name);
		kfree(gluebi);
	}
	ubi_unregister_volume_notifier(&gluebi_notifier);
}

module_init(ubi_gluebi_init);
module_exit(ubi_gluebi_exit);
MODULE_DESCRIPTION("MTD emulation layer over UBI volumes");
MODULE_AUTHOR("Artem Bityutskiy, Joern Engel");
MODULE_LICENSE("GPL");
