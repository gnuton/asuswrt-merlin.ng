/* Copyright (c) 2011-2014, 2017 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _RAMDUMP_HEADER
#define _RAMDUMP_HEADER

struct device;

struct ramdump_segment {
	unsigned long address;
	void *v_address;
	unsigned long size;
	/*to hold the virt addr read from the mdt file*/
	void *vaddr;
};

#ifdef CONFIG_IPQ_SUBSYSTEM_RAMDUMP
extern void *create_ramdump_device(const char *dev_name, struct device *parent);
extern int create_ramdump_device_file(void *);
extern void destroy_ramdump_device(void *dev);
extern int do_ramdump(void *handle, struct ramdump_segment *segments,
		int nsegments);
extern int do_elf_ramdump(void *handle, struct ramdump_segment *segments,
		int nsegments);

#else
static inline void *create_ramdump_device(const char *dev_name,
		struct device *parent)
{
	return NULL;
}
static inline int create_ramdump_device_file(void *handle)
{
	return -EINVAL;
}

static inline void destroy_ramdump_device(void *dev)
{
}

static inline int do_ramdump(void *handle, struct ramdump_segment *segments,
		int nsegments)
{
	return -ENODEV;
}

static inline int do_elf_ramdump(void *handle, struct ramdump_segment *segments,
		int nsegments)
{
	return -ENODEV;
}
#endif /* CONFIG_IPQ_SUBSYSTEM_RESTART */

#endif
