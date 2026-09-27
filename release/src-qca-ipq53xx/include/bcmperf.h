/*
 * Performance counters software interface.
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: bcmperf.h,v 13.5 2007/09/14 22:00:59 Exp $
 */
/* essai */
#ifndef _BCMPERF_H_
#define _BCMPERF_H_
/* get cache hits and misses */
#if defined(BCMMIPS) && defined(BCMPERFSTATS)
#include <hndmips.h>
#define BCMPERF_ENABLE_INSTRCOUNT() hndmips_perf_instrcount_enable()
#define BCMPERF_ENABLE_ICACHE_MISS() hndmips_perf_icache_miss_enable()
#define BCMPERF_ENABLE_ICACHE_HIT() hndmips_perf_icache_hit_enable()
#define	BCMPERF_GETICACHE_MISS(x)	((x) = hndmips_perf_read_cache_miss())
#define	BCMPERF_GETICACHE_HIT(x)	((x) = hndmips_perf_read_cache_hit())
#define	BCMPERF_GETINSTRCOUNT(x)	((x) = hndmips_perf_read_instrcount())
#else
#define BCMPERF_ENABLE_INSTRCOUNT()
#define BCMPERF_ENABLE_ICACHE_MISS()
#define BCMPERF_ENABLE_ICACHE_HIT()
#define	BCMPERF_GETICACHE_MISS(x)	((x) = 0)
#define	BCMPERF_GETICACHE_HIT(x)	((x) = 0)
#define	BCMPERF_GETINSTRCOUNT(x)	((x) = 0)
#endif /* defined(mips) */
#endif /* _BCMPERF_H_ */
