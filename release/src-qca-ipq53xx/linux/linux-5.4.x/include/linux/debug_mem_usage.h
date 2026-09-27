/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __DEBUG_MEM_USAGE_H__
#define __DEBUG_MEM_USAGE_H__

#ifdef CONFIG_DEBUG_MEM_USAGE

#define dma_alloc_coherent		__wrap_dma_alloc_coherent
#define dma_free_coherent		__wrap_dma_free_coherent
#define dma_pool_alloc			__wrap_dma_pool_alloc
#define dma_pool_free			__wrap_dma_pool_free

#define __kmalloc			__wrap___kmalloc
#define kmalloc				__wrap_kmalloc
#define kcalloc				__wrap_kcalloc
#define kzalloc				__wrap_kzalloc
#define __kmalloc_node			__wrap___kmalloc_node
#define kmalloc_node			__wrap_kmalloc_node
#define kmalloc_array			__wrap_kmalloc_array
#define kmalloc_order			__wrap_kmalloc_order
#define kmalloc_order_trace		__wrap_kmalloc_order_trace
#define kzalloc_node			__wrap_kzalloc_node
#define kfree				__wrap_kfree

#define __kmalloc_track_caller		__wrap___kmalloc_track_caller
#define __kmalloc_node_track_caller	__warp___kmalloc_node_track_caller

#define kmem_cache_alloc		__wrap_kmem_cache_alloc
#define kmem_cache_alloc_node		__wrap_kmem_cache_alloc_node
#define kmem_cache_alloc_node_trace	__wrap_kmem_cache_alloc_node_trace
#define kmem_cache_alloc_trace		__wrap_kmem_cache_alloc_trace
#define kmem_cache_free			__wrap_kmem_cache_free

#define kmemdup				__wrap_kmemdup

#define __get_free_pages		__wrap___get_free_pages
#define get_zeroed_page			__wrap_get_zeroed_page

#ifdef alloc_pages
#undef alloc_pages
#define alloc_pages			__wrap_alloc_pages
#endif

#define __alloc_pages			__wrap___alloc_pages
#define __alloc_pages_node		__wrap___alloc_pages_node
#define alloc_pages_node		__wrap_alloc_pages_node
#define __alloc_pages_nodemask		__wrap___alloc_pages_nodemask
#define free_pages			__wrap_free_pages
#define __free_pages			__wrap___free_pages

#define alloc_pages_exact		__wrap_alloc_pages_exact
#define alloc_pages_exact_nid		__wrap_alloc_pages_exact_nid
#define free_pages_exact		__wrap_free_pages_exact

#define mempool_alloc			__wrap_mempool_alloc
#define memdup_user			__wrap_memdup_user
#define mempool_free			__wrap_mempool_free

#define __vmalloc			__wrap___vmalloc
#define vmalloc				__wrap_vmalloc
#define vzalloc				__wrap_vzalloc
#define vmalloc_user			__wrap_vmalloc_user
#define vmalloc_node			__wrap_vmalloc_node
#define vzalloc_node			__wrap_vzalloc_node
#define vmalloc_32			__wrap_vmalloc_32
#define krealloc			__wrap_krealloc
#define kzfree				__wrap_kzfree
#define kvfree				__wrap_kvfree
#define vfree				__wrap_vfree

#define devm_kmalloc			__wrap_devm_kmalloc
#define devm_kcalloc			__wrap_devm_kcalloc
#define devm_kzalloc			__wrap_devm_kzalloc
#define devm_kmalloc_array		__wrap_devm_kmalloc_array
#define devm_kfree			__wrap_devm_kfree
#define devm_kmemdup			__wrap_devm_kmemdup
#define devm_get_free_pages		__wrap_devm_get_free_pages
#define devm_free_pages			__wrap_devm_free_pages

#define page_frag_alloc			__wrap_page_frag_alloc
#define page_frag_free			__wrap_page_frag_free

#define alloc_large_system_hash		__wrap_alloc_large_system_hash

#define __put_page			__wrap___put_page

#define mem_tracer_update_caller	__wrap_update_call_stack

#define mem_debug_update_skb(skb) \
do { \
        mem_tracer_update_caller(skb->head); \
        mem_tracer_update_caller(skb); \
} while (0)

#define mem_debug_update_skb_list(skb_list) \
do { \
        struct sk_buff *skb = NULL, *next = NULL; \
        skb_queue_walk_safe(skb_list, skb, next) { \
                if (skb) \
                        mem_debug_update_skb(skb); \
        } \
} while (0)
#else
#define mem_tracer_update_caller
#define  mem_debug_update_skb(skb)
#define  mem_debug_update_skb_list(skb_list)
#endif

#endif /* __DEBUG_MEM_USAGE_H__ */
