/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
#include <linux/debugobjects.h>
#include <linux/slab.h>
#include <linux/hash.h>

#define ODEBUG_HASH_BITS	14
#define ODEBUG_HASH_SIZE	(1 << ODEBUG_HASH_BITS)


#define ODEBUG_CHUNK_SHIFT	PAGE_SHIFT

int debug_mem_usage_enabled = 1;

struct debug_bucket {
	struct hlist_head	list;
	raw_spinlock_t		lock;
};

static struct debug_bucket	mem_trace_hash[ODEBUG_HASH_SIZE];

static struct kmem_cache	*obj_trace_cache;

static int __init disable_mem_usage_debug(char *str)
{
	debug_mem_usage_enabled = 0;
	return 0;
}

early_param("skip_mem_usage_tracking", disable_mem_usage_debug);

static struct debug_obj_trace *lookup_object_with_trace(void *addr,
					struct debug_bucket *b)
{
	struct debug_obj_trace *trace_obj;

	hlist_for_each_entry(trace_obj, &b->list, node) {
		if (trace_obj->addr == addr)
			return trace_obj;
	}

	return NULL;
}

static struct debug_obj_trace *
alloc_object_trace(void *addr, struct debug_bucket *b)
{
	struct debug_obj_trace *new = NULL;
	gfp_t gfp = GFP_ATOMIC | __GFP_NORETRY | __GFP_NOWARN;

	new = kmem_cache_zalloc(obj_trace_cache, gfp);
	if (!new)
		return new;

	hlist_add_head(&new->node, &b->list);

	return new;
}

static struct debug_bucket *get_bucket(unsigned long addr)
{
	unsigned long hash;

	hash = hash_long((addr >> ODEBUG_CHUNK_SHIFT), ODEBUG_HASH_BITS);
	return &mem_trace_hash[hash];
}

void debug_object_trace_init(void *addr, void **stack, size_t size)
{

	struct debug_bucket *db;
	unsigned long flags;
	struct debug_obj_trace *trace_obj;

	db = get_bucket((unsigned long) addr);

	raw_spin_lock_irqsave(&db->lock, flags);

	trace_obj = lookup_object_with_trace(addr, db);
	if (!trace_obj) {
		trace_obj = alloc_object_trace(addr, db);
		if (!trace_obj) {
			pr_err("debug object allocation for size %zu failed\n",
						sizeof(struct debug_obj_trace));
			raw_spin_unlock_irqrestore(&db->lock, flags);
			return;
		}
		trace_obj->addr = addr;
		memcpy(trace_obj->stack, stack, 9 * sizeof(void *));
		trace_obj->size = size;

	} else {
		memcpy(trace_obj->stack, stack, 9 * sizeof(void *));
		trace_obj->size = size;
	}

	raw_spin_unlock_irqrestore(&db->lock, flags);
}

void debug_object_trace_update(void *addr, void **stack)
{

	struct debug_bucket *db;
	unsigned long flags;
	struct debug_obj_trace *trace_obj;

	db = get_bucket((unsigned long) addr);

	raw_spin_lock_irqsave(&db->lock, flags);

	trace_obj = lookup_object_with_trace(addr, db);
	if (!trace_obj)
		goto out_unlock;

	memcpy(trace_obj->stack, stack, 9 * sizeof(void *));

out_unlock:
	raw_spin_unlock_irqrestore(&db->lock, flags);
}

void debug_object_trace_free(void *addr)
{
	struct debug_bucket *db;
	struct debug_obj_trace *trace_obj;
	unsigned long flags;

	db = get_bucket((unsigned long) addr);

	raw_spin_lock_irqsave(&db->lock, flags);

	trace_obj = lookup_object_with_trace(addr, db);
	if (!trace_obj)
		goto out_unlock;
	trace_obj->size = 0;

out_unlock:
	raw_spin_unlock_irqrestore(&db->lock, flags);
}

void __init debug_mem_usage_init(void)
{
	int i;

	if (!debug_mem_usage_enabled)
		return;

	obj_trace_cache = kmem_cache_create("obj_trace_cache",
		sizeof(struct debug_obj_trace), 0, 0, NULL);

	for (i = 0; i < ODEBUG_HASH_SIZE; i++)
		raw_spin_lock_init(&mem_trace_hash[i].lock);
}
