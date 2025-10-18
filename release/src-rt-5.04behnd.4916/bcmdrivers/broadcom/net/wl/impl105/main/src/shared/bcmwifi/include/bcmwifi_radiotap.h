/*
 * RadioTap utility routines for WL and Apps
 * This header file housing the define and function prototype use by
 * both the wl driver, tools & Apps.
 *
 * Copyright (C) 2024, Broadcom. All Rights Reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: bcmwifi_radiotap.h 835642 2024-01-24 01:15:40Z $
 */

#ifndef _BCMWIFI_RADIOTAP_H_
#define _BCMWIFI_RADIOTAP_H_

#include <ieee80211_radiotap.h>
#include <monitor.h>
#include <802.11.h>
#include <802.11be.h>

#include <bcmwifi_rspec.h>

/* This marks the start of a packed structure section. */
#include <packed_section_start.h>
/*
 * RadioTap header specific implementation
 */

#define MAX_RADIOTAP_SIZE	(MAX(sizeof(wl_radiotap_eht_t),	\
					MAX(sizeof(wl_radiotap_he_t),	\
						MAX(sizeof(wl_radiotap_vht_t),	\
							MAX(sizeof(wl_radiotap_ht_t),	\
								sizeof(wl_radiotap_legacy_t))))))

#define MAX_MON_PKT_SIZE	(4096 + MAX_RADIOTAP_SIZE)

BWL_PRE_PACKED_STRUCT struct wl_radiotap_hdr {
	struct ieee80211_radiotap_header ieee_radiotap;
	uint64 tsft;
	uint8 flags;
	union {
		uint8 rate;
		uint8 pad;
	} u;
	uint16 channel_freq;
	uint16 channel_flags;
} BWL_POST_PACKED_STRUCT;

BWL_PRE_PACKED_STRUCT struct wl_radiotap_sna {
	uint8 signal;
	uint8 noise;
	uint8 antenna;
} BWL_POST_PACKED_STRUCT;

BWL_PRE_PACKED_STRUCT struct wl_radiotap_xchan {
	uint32 xchannel_flags;
	uint16 xchannel_freq;
	uint8 xchannel_channel;
	uint8 xchannel_maxpower;
} BWL_POST_PACKED_STRUCT;

BWL_PRE_PACKED_STRUCT struct wl_radiotap_ampdu {
	uint32 ref_num;
	uint16 flags;
	uint8 delimiter_crc;
	uint8 reserved;
} BWL_POST_PACKED_STRUCT;

BWL_PRE_PACKED_STRUCT struct wl_htmcs {
	uint8 mcs_known;
	uint8 mcs_flags;
	uint8 mcs_index;
	uint8 pad;		/* pad to 32 bit aligned */
} BWL_POST_PACKED_STRUCT;

BWL_PRE_PACKED_STRUCT struct wl_vhtmcs {
	uint16 vht_known;	/* IEEE80211_RADIOTAP_VHT */
	uint8 vht_flags;
	uint8 vht_bw;
	uint8 vht_mcs_nss[4];
	uint8 vht_coding;
	uint8 vht_group_id;
	uint16 vht_partial_aid;
} BWL_POST_PACKED_STRUCT;

BWL_PRE_PACKED_STRUCT struct wl_radiotap_ht_tail {
	struct wl_radiotap_xchan xc;
	struct wl_radiotap_ampdu ampdu;
	union {
		struct wl_htmcs ht;
		struct wl_vhtmcs vht;
	} u;
} BWL_POST_PACKED_STRUCT;

#define WL_RADIOTAP_PRESENT_RX				\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	 (1 << IEEE80211_RADIOTAP_FLAGS) |		\
	 (1 << IEEE80211_RADIOTAP_RATE) |		\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |	\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |	\
	 (1 << IEEE80211_RADIOTAP_ANTENNA))

#define WL_RADIOTAP_PRESENT_RX_HT			\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	 (1 << IEEE80211_RADIOTAP_FLAGS) |		\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |	\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |	\
	 (1 << IEEE80211_RADIOTAP_ANTENNA) |		\
	 (1 << IEEE80211_RADIOTAP_XCHANNEL) |		\
	 (1 << IEEE80211_RADIOTAP_AMPDU) |		\
	 (1 << IEEE80211_RADIOTAP_MCS))

#define WL_RADIOTAP_PRESENT_RX_VHT			\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	 (1 << IEEE80211_RADIOTAP_FLAGS) |		\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |	\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |	\
	 (1 << IEEE80211_RADIOTAP_ANTENNA) |		\
	 (1 << IEEE80211_RADIOTAP_XCHANNEL) |		\
	 (1 << IEEE80211_RADIOTAP_AMPDU) |		\
	 (1 << IEEE80211_RADIOTAP_VHT))

#if defined(WLTXMONITOR) || defined(BCM_PCAP)
BWL_PRE_PACKED_STRUCT struct wl_radiotap_hdr_tx {
	struct ieee80211_radiotap_header        ieee_radiotap;
	uint64 tsft;
	uint8 flags;
	union {
		uint8 rate;
		uint8 pad;
	} u;
	uint16 channel_freq;
	uint16 channel_flags;
	uint16 txflags;
	uint8 retries;
	uint8 pad[3];
} BWL_POST_PACKED_STRUCT;

#define WL_RADIOTAP_TXHDR				\
	((1 << IEEE80211_RADIOTAP_TXFLAGS) |		\
	 (1 << IEEE80211_RADIOTAP_RETRIES))

#define WL_RADIOTAP_PRESENT_TX				\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	 (1 << IEEE80211_RADIOTAP_FLAGS) |		\
	 (1 << IEEE80211_RADIOTAP_RATE) |		\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	 WL_RADIOTAP_TXHDR)

#define WL_RADIOTAP_PRESENT_HT_TX			\
	 ((1 << IEEE80211_RADIOTAP_TSFT) |		\
	  (1 << IEEE80211_RADIOTAP_FLAGS) |		\
	  (1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	  WL_RADIOTAP_TXHDR |				\
	  (1 << IEEE80211_RADIOTAP_XCHANNEL) |		\
	  (1 << IEEE80211_RADIOTAP_MCS))

typedef struct bsd_header_tx {
	struct wl_radiotap_hdr_tx hdr;
	uint8 ht[sizeof(struct wl_radiotap_ht_tail)];
} bsd_header_tx_t;
#endif /* WLTXMONITOR || BCM_PCAP */

typedef struct bsd_header_rx {
	struct wl_radiotap_hdr hdr;
	/*
	 * include extra space beyond wl_radiotap_ht size
	 * (larger of two structs in union):
	 *   signal/noise/ant plus max of 3 pad for xchannel
	 *   tail struct (xchannel and MCS info)
	 */
	uint8 pad[3];
	uint8 ht[sizeof(struct wl_radiotap_ht_tail)];
} bsd_header_rx_t;

typedef struct radiotap_parse {
	struct ieee80211_radiotap_header *hdr;
	void *fields;
	uint fields_len;
	uint idx;
	uint offset;
} radiotap_parse_t;

struct rtap_field {
	uint len;
	uint align;
};

/* EHT USIG radiotap - https://www.radiotap.org/fields/U-SIG.html */
#define EHT_RADIOTAP_VER_SHIFT                  12u
#define EHT_RADIOTAP_BW_SHIFT                   15u
#define EHT_RADIOTAP_UL_DL_SHIFT                18u
#define EHT_RADIOTAP_BSS_COLOR_SHIFT            19u
#define EHT_RADIOTAP_TXOP_SHIFT                 25u
#define EHT_RADIOTAP_USIG2_SHIFT                6u
#define EHT_RADIOTAP_VALUE_MASK                 0xffffffffu

#define WL_RADIOTAP_BRCM_SNS		0x01
#define WL_RADIOTAP_BRCM_MCS		0x00000001
#define WL_RADIOTAP_LEGACY_SNS		0x02
#define WL_RADIOTAP_LEGACY_VHT		0x00000001

#define IEEE80211_RADIOTAP_HTMOD_40		0x01
#define IEEE80211_RADIOTAP_HTMOD_SGI		0x02
#define IEEE80211_RADIOTAP_HTMOD_GF		0x04
#define IEEE80211_RADIOTAP_HTMOD_LDPC		0x08
#define IEEE80211_RADIOTAP_HTMOD_STBC_MASK	0x30
#define IEEE80211_RADIOTAP_HTMOD_STBC_SHIFT	4

/* Common fields for legacy/HT/VHT/HE/EHT radiotap structure */
BWL_PRE_PACKED_STRUCT struct wl_radiotap_common {
	uint32 tsft_l;
	uint32 tsft_h;
	uint8  flags;
	union {
		uint8 rate;	/* legacy radiotap header */
		uint8 pad1;	/* non-legacy radiotap headers */
	} u;
	uint16 channel_freq;
	uint16 channel_flags;
	int8  signal;
	int8  noise;
	uint8 antenna;
} BWL_POST_PACKED_STRUCT;
typedef struct wl_radiotap_common wl_radiotap_common_t;

/* Dyanmic bandwidth for VHT signaled in NONHT */
#define WL_RADIOTAP_F_NONHT_VHT_DYN_BW			0x01
/* VHT BW is valid in NONHT */
#define WL_RADIOTAP_F_NONHT_VHT_BW			0x02

/* VHT information in non-HT frames; primarily VHT b/w signaling
 * in frames received at legacy rates.
 */
BWL_PRE_PACKED_STRUCT struct wl_radiotap_nonht_vht {
	uint8 len;				/* length of the field excluding 'len' field */
	uint8 flags;
	uint8 bw;
} BWL_POST_PACKED_STRUCT;

typedef struct wl_radiotap_nonht_vht wl_radiotap_nonht_vht_t;

/* radiotap standard - non-HT, non-VHT information with Broadcom vendor namespace extension
 * that includes VHT information.
 * Used with monitor type 3 when received by HT/Legacy PHY and received rate is legacy.
 */
BWL_PRE_PACKED_STRUCT struct wl_radiotap_legacy {
	struct ieee80211_radiotap_header ieee_radiotap;
	uint32 it_present_ext;
	uint32 pad1;
	uint32 tsft_l;
	uint32 tsft_h;
	uint8 flags;
	uint8 rate;
	uint16 channel_freq;
	uint16 channel_flags;
	uint8 signal;
	uint8 noise;
	int8 antenna;
	uint8 pad2;
	uint8 vend_oui[3];
	uint8 vend_sns;
	uint16 vend_skip_len;
	/* XXX although noht_vht is always included, one could optimize by
	 * allowing for variable length data when other BCM specific fields
	 * are added with appropriate indiciation in it_present_ext
	*/
	wl_radiotap_nonht_vht_t nonht_vht;
} BWL_POST_PACKED_STRUCT;

typedef struct wl_radiotap_legacy wl_radiotap_legacy_t;

BWL_PRE_PACKED_STRUCT struct wl_radiotap_tx_legacy {
	struct ieee80211_radiotap_header ieee_radiotap;
	wl_radiotap_common_t rt_common;
	uint8 pad2;
	uint8 vend_oui[3];
	uint8 vend_sns;
	uint16 vend_skip_len;
} BWL_POST_PACKED_STRUCT;

typedef struct wl_radiotap_tx_legacy wl_radiotap_tx_legacy_t;

#define WL_RADIOTAP_LEGACY_SKIP_LEN htol16(sizeof(struct wl_radiotap_legacy) - \
	OFFSETOF(struct wl_radiotap_legacy, nonht_vht))

#define WL_RADIOTAP_NONHT_VHT_LEN (sizeof(wl_radiotap_nonht_vht_t) - 1)

/* Radiotap standard that includes HT information. This is for use with monitor type 3
 * whenever frame is received by HT-PHY, and received rate is non-VHT.
 */
BWL_PRE_PACKED_STRUCT struct wl_radiotap_ht {
	struct ieee80211_radiotap_header ieee_radiotap;
	wl_radiotap_common_t rt_common;
	uint8 mcs_known;
	uint8 mcs_flags;
	uint8 mcs_index;
} BWL_POST_PACKED_STRUCT;

typedef struct wl_radiotap_ht wl_radiotap_ht_t;

/* Radiotap standard that includes VHT information.
 * This is for use with monitor type 3 whenever frame is
 * received by HT-PHY (VHT-PHY), and received rate is VHT.
 */
BWL_PRE_PACKED_STRUCT struct wl_radiotap_vht {
	struct ieee80211_radiotap_header ieee_radiotap;
	wl_radiotap_common_t rt_common;
	uint8 pad2;
	uint16 pad3;
	uint32 ampdu_ref_num;	/* A-MPDU ID */
	uint16 ampdu_flags;		/* A-MPDU flags */
	uint8 ampdu_delim_crc;	/* Delimiter CRC if present in flags */
	uint8 ampdu_reserved;
	uint16 vht_known;		/* IEEE80211_RADIOTAP_VHT */
	uint8 vht_flags;		/* IEEE80211_RADIOTAP_VHT */
	uint8 vht_bw;		/* IEEE80211_RADIOTAP_VHT */
	uint8 vht_mcs_nss[4];	/* IEEE80211_RADIOTAP_VHT */
	uint8 vht_coding;		/* IEEE80211_RADIOTAP_VHT */
	uint8 vht_group_id;		/* IEEE80211_RADIOTAP_VHT */
	uint16 vht_partial_aid;	/* IEEE80211_RADIOTAP_VHT */
} BWL_POST_PACKED_STRUCT;

typedef struct wl_radiotap_vht wl_radiotap_vht_t;

BWL_PRE_PACKED_STRUCT struct wl_radiotap_he {
	struct ieee80211_radiotap_header ieee_radiotap;
	wl_radiotap_common_t rt_common;
	uint8 pad2;
	uint16 pad3;
	uint32 ampdu_ref_num;		/* A-MPDU ID */
	uint16 ampdu_flags;		/* A-MPDU flags */
	uint8 ampdu_delim_crc;	/* Delimiter CRC if present in flags */
	uint8 ampdu_reserved;
	uint16 data1;                   /* radiotap HE fields */
	uint16 data2;
	uint16 data3;
	uint16 data4;
	uint16 data5;
	uint16 data6;
} BWL_POST_PACKED_STRUCT;
typedef struct wl_radiotap_he wl_radiotap_he_t;

BWL_PRE_PACKED_STRUCT struct wl_radiotap_eht {
	struct ieee80211_radiotap_header ieee_radiotap;
	wl_radiotap_common_t rt_common;
	uint8 pad2;
	uint16 pad3;
	uint32 ampdu_ref_num;		/* A-MPDU ID */
	uint16 ampdu_flags;		/* A-MPDU flags */
	uint8 ampdu_delim_crc;		/* Delimiter CRC if present in flags */
	uint8 ampdu_reserved;
	uint16 eht_t;			/* EHT TLV */
	uint16 eht_l;
	uint32 known;
	uint32 data[9];			/* radiotap EHT data fields */
	uint32 user_info;
	uint16 usig_t;			/* U-SIG TLV */
	uint16 usig_l;
	uint32 common;
	uint32 value;
	uint32 mask;
} BWL_POST_PACKED_STRUCT;
typedef struct wl_radiotap_eht wl_radiotap_eht_t;

union wl_radiotap_elem {
	wl_radiotap_tx_legacy_t rtap_leg;
	wl_radiotap_ht_t  rtap_ht;
	wl_radiotap_vht_t rtap_vht;
	wl_radiotap_he_t rtap_he;
	wl_radiotap_eht_t rtap_eht;
};

typedef union wl_radiotap_elem wl_radiotap_elem_t;
#define WL_RADIOTAP_PRESENT_LEGACY			\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	 (1 << IEEE80211_RADIOTAP_RATE) |		\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |	\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |	\
	 (1 << IEEE80211_RADIOTAP_FLAGS) |		\
	 (1 << IEEE80211_RADIOTAP_ANTENNA) |		\
	 (1 << IEEE80211_RADIOTAP_VENDOR_NAMESPACE) |	\
	 (1 << IEEE80211_RADIOTAP_EXT))

#define WL_RADIOTAP_PRESENT_TX_LEGACY                   \
	((1 << IEEE80211_RADIOTAP_TSFT) |               \
	 (1 << IEEE80211_RADIOTAP_RATE) |               \
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |            \
	 (1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |      \
	 (1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |       \
	 (1 << IEEE80211_RADIOTAP_FLAGS) |              \
	 (1 << IEEE80211_RADIOTAP_ANTENNA))

#define WL_RADIOTAP_PRESENT_HT				\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	 (1 << IEEE80211_RADIOTAP_FLAGS) |		\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |	\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |	\
	 (1 << IEEE80211_RADIOTAP_ANTENNA) |		\
	 (1 << IEEE80211_RADIOTAP_MCS))

#define WL_RADIOTAP_PRESENT_VHT			\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	 (1 << IEEE80211_RADIOTAP_FLAGS) |		\
	 (1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |	\
	 (1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |	\
	 (1 << IEEE80211_RADIOTAP_ANTENNA) |		\
	 (1 << IEEE80211_RADIOTAP_AMPDU) |		\
	 (1 << IEEE80211_RADIOTAP_VHT))

#define WL_RADIOTAP_PRESENT_HE			\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	(1 << IEEE80211_RADIOTAP_FLAGS) |		\
	(1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	(1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |	\
	(1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |	\
	(1 << IEEE80211_RADIOTAP_ANTENNA) |		\
	(1 << IEEE80211_RADIOTAP_AMPDU) |		\
	(1 << IEEE80211_RADIOTAP_HE))

#define WL_RADIOTAP_PRESENT_EHT			\
	((1 << IEEE80211_RADIOTAP_TSFT) |		\
	(1 << IEEE80211_RADIOTAP_FLAGS) |		\
	(1 << IEEE80211_RADIOTAP_CHANNEL) |		\
	(1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL) |	\
	(1 << IEEE80211_RADIOTAP_DBM_ANTNOISE) |	\
	(1 << IEEE80211_RADIOTAP_ANTENNA) |		\
	(1 << IEEE80211_RADIOTAP_AMPDU) |		\
	(1 << IEEE80211_RADIOTAP_TLVS))
/* include/linux/if_arp.h
 *	#define ARPHRD_IEEE80211_PRISM 802 IEEE 802.11 + Prism2 header
 *	#define ARPHRD_IEEE80211_RADIOTAP 803 IEEE 802.11 + radiotap header
 * include/net/ieee80211_radiotap.h
 *	radiotap structure
 */

#ifndef ARPHRD_IEEE80211_RADIOTAP
#define ARPHRD_IEEE80211_RADIOTAP 803
#endif // endif

/* This marks the end of a packed structure section. */
#include <packed_section_end.h>

extern void wl_rtapParseInit(radiotap_parse_t *rtap, uint8 *rtap_header);
extern ratespec_t wl_calcRspecFromRTap(uint8 *rtap_header);
extern bool wl_rtapFlags(uint8 *rtap_header, uint8* flags);
extern uint wl_radiotap_rx(struct dot11_header *mac_header, wl_rxsts_t *rxsts,
	bsd_header_rx_t *bsd_header);
extern uint wl_radiotap_rx_legacy(struct dot11_header *mac_header, wl_rxsts_t *rxsts,
	wl_radiotap_legacy_t *rtl);
extern uint wl_radiotap_rx_ht(struct dot11_header *mac_header, wl_rxsts_t *rxsts,
	wl_radiotap_ht_t *rtht);
extern uint wl_radiotap_rx_vht(struct dot11_header *mac_header, wl_rxsts_t *rxsts,
	wl_radiotap_vht_t *rtvht);
extern uint wl_radiotap_rx_he(struct dot11_header *mac_header, wl_rxsts_t *rxsts,
       wl_radiotap_he_t *rthe);
extern uint wl_radiotap_rx_eht(struct dot11_header *mac_header, wl_rxsts_t *rxsts,
       wl_radiotap_eht_t *rteht);
extern uint16 wl_rxsts_to_rtap(struct wl_rxsts* rxsts, void *pdata,
		uint16 len, void* pout, int16 *offset);
#ifdef WLTXMONITOR
extern uint wl_radiotap_tx(struct dot11_header *mac_header, wl_txsts_t *txsts,
	bsd_header_tx_t *bsd_header);
#endif /* WLTXMONITOR */
#endif	/* _BCMWIFI_RADIOTAP_H_ */
