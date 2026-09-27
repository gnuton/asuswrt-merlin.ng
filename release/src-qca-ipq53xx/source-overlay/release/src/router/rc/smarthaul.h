#ifndef _SMARTHAUL_H_
#define _SMARTHAUL_H_

#define MAX_MLO_LINKS   4
#define SH_OK           0       /* Success */
#define SH_ERROR        -1      /* Error generic */

#ifndef _TYPEDEFS_H_
#include "typedefs.h"
//#include<sys/types.h>
#endif

#define UNUSED_VAR __attribute__((unused))

/* compare two ethernet addresses - assumes the pointers can be referenced as shorts */
#define eacmp(a, b)	((((const uint16 *)(a))[0] ^ ((const uint16 *)(b))[0]) | \
	                 (((const uint16 *)(a))[1] ^ ((const uint16 *)(b))[1]) | \
	                 (((const uint16 *)(a))[2] ^ ((const uint16 *)(b))[2]))

/* copy an ethernet address - assumes the pointers can be referenced as shorts */
#define eacopy(s, d) \
do { \
	((uint16 *)(d))[0] = ((const uint16 *)(s))[0]; \
	((uint16 *)(d))[1] = ((const uint16 *)(s))[1]; \
	((uint16 *)(d))[2] = ((const uint16 *)(s))[2]; \
} while (0)

#include <libasuslog.h>
/* define debug log */
#define SH_DBG_LOG	"smarthaul.log"
#define SMARTHAUL_DEBUG_ERROR	0x0001
#define SMARTHAUL_DEBUG_TRACE	0x0002
#define SMARTHAUL_DEFAULT_LEVEL SMARTHAUL_DEBUG_ERROR
int smarthaul_debug_level = SMARTHAUL_DEFAULT_LEVEL;

#define SMARTHAUL_ERROR(fmt, arg...) \
	do { if (smarthaul_debug_level & SMARTHAUL_DEBUG_ERROR) \
		_dprintf("SMARTHAUL >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); } while (0)

#define SMARTHAUL_TRACE(fmt, arg...) \
	do { if (smarthaul_debug_level & SMARTHAUL_DEBUG_TRACE) \
		_dprintf("SMARTHAUL >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); } while (0)

#define SMARTHAUL_SYSLOG(fmt, arg...) \
    do { \
        logmessage("smarthaul: ", fmt, ##arg); \
		asusdebuglog(LOG_INFO, SH_DBG_LOG, LOG_CUSTOM, LOG_SHOWTIME, 0, fmt, ##arg); \
        _dprintf("SMARTHUAL >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); } while (0)

#define SMARTHAUL_DBGLOG(fmt, arg...) \
    do { \
		asusdebuglog(LOG_INFO, SH_DBG_LOG, LOG_CUSTOM, LOG_SHOWTIME, 0, fmt, ##arg); } while (0)

#if defined(RTCONFIG_RALINK) || defined(RTCONFIG_QCA)
#define SH_ETHERP_ADDR(x) ((x)->ether_addr_octet)
#define SH_ETHER_ADDR(x) ((x).ether_addr_octet)
#else
#define SH_ETHERP_ADDR(x) ((x)->octet)
#define SH_ETHER_ADDR(x) ((x).octet)
#endif

typedef struct sh_tidmap {
	uint32 sh_idx;    // smarthaul interface index
	uint8 tids[MAX_MLO_LINKS];	// ordered by band, 6g, 5g, 2g, DC
} sh_tidmap_t;

typedef struct sh_mlo_link_info {
	struct ether_addr pap_bssid;
	uint8 wlc_unit;
	uint8 link_id;
	uint8 bandidx;  /* 2 5 6 */
} sh_mlo_link_info_t;

typedef struct sh_mlo_info {
	struct ether_addr self_mld_addr;
	uint8 mlo_num_links;
	sh_mlo_link_info_t mlo_link_info[MAX_MLO_LINKS];
} sh_mlo_info_t;

typedef struct sh_dev_info {
	struct ether_addr self_mac;
	sh_mlo_info_t mlo_info;
} sh_dev_info_t;

typedef struct sh_repeater_node {
	struct sh_repeater_node *next;
	struct ether_addr self_mac;
	sh_mlo_info_t mlo_info;
	uint32 sh_idx;
} sh_repeater_node_t;

typedef struct sh_root_info {
	sh_repeater_node_t *rpt_list;
	int num_repeaters;
	sh_dev_info_t local_dev;
} sh_root_info_t;

typedef struct smarthaul {
	void *dev_info;
} smarthaul_t;

/* internal representation of the stats */
typedef struct sh_link_stats {
	/* -- from chanim_stats */
	uint8 txop;
	uint8 obss;
	uint8 tx;
	uint8 inbss;
	uint8 nocat;
	uint8 nopkt;

	/* -- from txrx_summary */
	uint32 phy_rate; /* units in mbps */
	uint32 data_rate;
	uint32 airtime; /* airtime percentage */

	/* -- from mlo scb_stats */
	uint64_t ntx;
	uint64_t nrx;
	uint64_t last_ntx;
	uint64_t last_nrx;
	int rssi;

	/* -- from mlo tidmap */
	uint8 tidmap[MAX_MLO_LINKS];

	/* -- stats update state & time */
	uint flags;     /* valid bits: 0 (chanim), 1 (scb_stats), ... */
} sh_link_stats_t;

typedef struct sh_link {
	uint8 link_id;
	uint8 wlc_unit;
	uint8 band_ix;		/* 0: 6G, 1: 5G, 2: 2G */
	struct sh_mbh *mbh; /* back pointer */

	/* dynamic from last stats */
	struct sh_link_stats stats;
} sh_link_t;

/* read from nvram by algo, expose it to utils */
typedef struct sh_algo_info {
	uint8 txop_hi;		/* thresh get out of congestion */
	uint8 txop_lo;		/* thresh fall into congestion */
	uint8 anchor_band;	/* globally xross all bh interfaces, FF means no */

	uint16 cycle_ms;	/* duration of stats collection cycle, units in ms */
} sh_algo_info_t;

typedef struct sh_mbh {
	struct sh_mbh *next;	/* next AP backhaul interface */
	struct sh_mbh *bsta;	/* companion backhaul STA */
	struct sh_algo *algo;	/* back pointer */
	const sh_mlo_info_t *mlo_info;

	uint32 ix;	/* index of a BH */
	bool is_bap;
	uint8 link_cnt;

	uint8 cfg_linkset;	/* linkset user configed */
	uint8 anchor_link;	/* current anchor link */
	uint8 cur_linkset;	/* cur bitmap of links */
	uint8 can_linkset;	/* candidate bitmap of links */
	uint32 change_cycle;	/* cycle # of last linkset change */
	uint8 live_cycle;

	uint32 stats_cycle;	/* cycle # of last stats update */
	struct sh_link link[MAX_MLO_LINKS];
} sh_mbh_t;

typedef struct sh_algo {
	struct sh_algo_info info;
	void *arg;
	uint32 cycle;		/* increase by 1 every cycle */
	uint8 mbh_cnt;		/* num of mlo bh interfaces managed */
	struct sh_mbh *mbh;
	const struct sh_ls_algo *ls_algo;	/* per bh interface algo */
} sh_algo_t;

#endif	/* _SMARTHAUL_H_ */
