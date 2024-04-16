/****************************************************************************
 *
 * Broadcom Proprietary and Confidential.
 * (c) 2016 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Limited and/or its subsidiaries.
 *
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to
 * you under the terms of the GNU General Public License version 2 (the
 * "GPL"), available at [http://www.broadcom.com/licenses/GPLv2.php], with
 * the following added to such license:
 *
 * As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy
 * and distribute the resulting executable under terms of your choice,
 * provided that you also meet, for each linked independent module, the
 * terms and conditions of the license of that module. An independent
 * module is a module which is not derived from this software. The special
 * exception does not apply to any modifications of the software.
 *
 * Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a
 * license other than the GPL, without Broadcom's express prior written
 * consent.
 *
 ****************************************************************************
 * Author: Jayesh Patel <jayesh.patel@broadcom.com>
 *****************************************************************************/
#ifndef _BCM_CACHE_H_
#define _BCM_CACHE_H_

#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>

#if defined(CONFIG_ARM) /* This is a temporary hack  - REMOVE */
#ifndef DMA_H
#define DMA_H

#include <asm/glue-cache.h>

#ifndef MULTI_CACHE
#define dmac_map_area			__glue(_CACHE,_dma_map_area)
#define dmac_unmap_area 		__glue(_CACHE,_dma_unmap_area)

/*
 * These are private to the dma-mapping API.  Do not use directly.
 * Their sole purpose is to ensure that data held in the cache
 * is visible to DMA, or data written by DMA to system memory is
 * visible to the CPU.
 */
extern void dmac_map_area(const void *, size_t, int);
extern void dmac_unmap_area(const void *, size_t, int);

#else

/*
 * These are private to the dma-mapping API.  Do not use directly.
 * Their sole purpose is to ensure that data held in the cache
 * is visible to DMA, or data written by DMA to system memory is
 * visible to the CPU.
 */
#define dmac_map_area			cpu_cache.dma_map_area
#define dmac_unmap_area 		cpu_cache.dma_unmap_area

#endif

#endif
#endif /* This is a temporary hack - REMOVE */

static inline void cache_invalidate_buffer(void *start, u32 size)
{
#if defined(CONFIG_ARM)
	dmac_unmap_area(start, size, DMA_FROM_DEVICE);
#elif defined(CONFIG_ARM64)
	__dma_unmap_area(start, size, DMA_FROM_DEVICE);
#elif defined(CONFIG_MIPS)
	dma_cache_inv((u32)start, size);
#else
#error No arch-specific DMA invalidate specified.
#endif
}

static inline void cache_flush_invalidate_buffer(void *start, u32 size)
{
#if defined(CONFIG_ARM)
	dmac_flush_range(start, start + size);
#elif defined(CONFIG_ARM64)
	__dma_flush_area( start, size);
#elif defined(CONFIG_MIPS)
	dma_cache_wback_inv((u32)start, size);
#else
#error No arch-specific DMA invalidate specified.
#endif
}
#endif
