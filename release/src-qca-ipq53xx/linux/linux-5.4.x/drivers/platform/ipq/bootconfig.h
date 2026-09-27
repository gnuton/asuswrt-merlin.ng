/*
 * Copyright (c) 2015-2016, 2020, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _BOOTCONFIG_H_
#define _BOOTCONFIG_H_

#define BOOTCONFIG_PART_IDX_MAX 21

#define ALT_PART_NAME_LENGTH 16
struct per_part_info {
	char name[ALT_PART_NAME_LENGTH];
	uint32_t primaryboot;
};

/* version 2 */
#define SMEM_DUAL_BOOTINFO_MAGIC_START 0xA3A2A1A0
#define SMEM_DUAL_BOOTINFO_MAGIC_START_TRYMODE 0xA3A2A1A1
#define SMEM_DUAL_BOOTINFO_MAGIC_END 0xB3B2B1B0

struct sbl_if_dualboot_info_type_v2 {
	uint32_t magic_start;
	uint32_t age;
	uint32_t numaltpart;
	struct per_part_info per_part_entry[CONFIG_NUM_ALT_PARTITION];
	uint32_t magic_end;
} __packed;

#endif /* _BOOTCONFIG_H_ */

