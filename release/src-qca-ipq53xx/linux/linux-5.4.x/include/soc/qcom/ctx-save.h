/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (c) 2020, The Linux Foundation. All rights reserved.
 */
#ifndef __CTX_SAVE_H
#define __CTX_SAVE_H

#include <linux/module.h>

#define CTX_SAVE_SCM_TLV_TYPE_SIZE	1
#define CTX_SAVE_SCM_TLV_LEN_SIZE	2
#define CTX_SAVE_SCM_TLV_TYPE_LEN_SIZE (CTX_SAVE_SCM_TLV_TYPE_SIZE + CTX_SAVE_SCM_TLV_LEN_SIZE)
#define INVALID 0

/* Module Meta Data File is currently set to 12K size
* by default, where (12K / 50) = 245 entries can be supported.
* To support max capacity of 646 entries,please modify
* METADATA_FILE_SZ from 12K to 32K.
*
* MMU Meta Data File is currently set to 12K size
* by default, where (12K / 33) = 372 entries can be supported.
* To support max capacity of 646 entries , please modify
* MMU_FILE_SZ from 12K to 21K.
*/

#define METADATA_FILE_SZ 12288
#define METADATA_FILE_ENTRY_LEN 50
#define NAME_LEN 28
#define MINIDUMP_MODULE_COUNT 4

#define MMU_FILE_SZ 12288
#define MMU_FILE_ENTRY_LEN 33

/* TLV_Types */
typedef enum {
    CTX_SAVE_LOG_DUMP_TYPE_INVALID,
    CTX_SAVE_LOG_DUMP_TYPE_UNAME,
    CTX_SAVE_LOG_DUMP_TYPE_DMESG,
    CTX_SAVE_LOG_DUMP_TYPE_LEVEL1_PT,
    CTX_SAVE_LOG_DUMP_TYPE_WLAN_MOD,
    CTX_SAVE_LOG_DUMP_TYPE_WLAN_MOD_DEBUGFS,
    CTX_SAVE_LOG_DUMP_TYPE_WLAN_MOD_INFO,
    CTX_SAVE_LOG_DUMP_TYPE_WLAN_MMU_INFO,
    CTX_SAVE_LOG_DUMP_TYPE_EMPTY,
} minidump_tlv_type_t;

int minidump_fill_segments_internal(const uint64_t start_addr, uint64_t size, minidump_tlv_type_t type, const char *name, int islowmem);
int minidump_fill_segments(const uint64_t start_addr, uint64_t size, minidump_tlv_type_t type, const char *name);
int minidump_store_module_info(const char *name , const unsigned long va, const unsigned long pa, minidump_tlv_type_t type);
int minidump_store_mmu_info(const unsigned long va, const unsigned long pa);
int minidump_remove_segments(const uint64_t virtual_address);
int do_minidump(void);

struct module_sect_attr {
	struct bin_attribute battr;
	unsigned long address;
};

struct module_sect_attrs {
	struct attribute_group grp;
	unsigned int nsections;
	struct module_sect_attr attrs[0];
};

#endif
