
/*
 * Fundamental types and constants relating to 802.11
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
 * $Id: 802.11.h 838603 2024-04-04 12:07:03Z $
 */

#ifndef _802_11_H_
#define _802_11_H_

#ifndef _TYPEDEFS_H_
#include <typedefs.h>
#endif // endif

#ifndef _NET_ETHERNET_H_
#include <ethernet.h>
#endif // endif

#include <wpa.h>

/* This marks the start of a packed structure section. */
#include <packed_section_start.h>

#define DOT11_TU_TO_US			1024	/* 802.11 Time Unit is 1024 microseconds */

/* Generic 802.11 frame constants */
#define DOT11_A3_HDR_LEN		24	/* d11 header length with A3 */
#define DOT11_A4_HDR_LEN		30	/* d11 header length with A4 */
#define DOT11_MAC_HDR_LEN		DOT11_A3_HDR_LEN	/* MAC header length */
#define DOT11_FCS_LEN			4	/* d11 FCS length */
#define DOT11_ICV_LEN			4	/* d11 ICV length */
#define DOT11_ICV_AES_LEN		8	/* d11 ICV/AES length */
#define DOT11_QOS_LEN			2	/* d11 QoS length */
#define DOT11_HTC_LEN			4	/* d11 HT Control field length */

#define DOT11_KEY_INDEX_SHIFT		6	/* d11 key index shift */
#define DOT11_IV_LEN			4	/* d11 IV length */
#define DOT11_IV_TKIP_LEN		8	/* d11 IV TKIP length */
#define DOT11_IV_AES_OCB_LEN		4	/* d11 IV/AES/OCB length */
#define DOT11_IV_AES_CCM_LEN		8	/* d11 IV/AES/CCM length */
#define DOT11_IV_MAX_LEN		8	/* maximum iv len for any encryption */

/* Includes MIC */
#define DOT11_MAX_MPDU_BODY_LEN		2304	/* max MPDU body length */
/* A4 header + QoS + CCMP + PDU + ICV + FCS = 2352 */
#define DOT11_MAX_MPDU_LEN		(DOT11_A4_HDR_LEN + \
					 DOT11_QOS_LEN + \
					 DOT11_IV_AES_CCM_LEN + \
					 DOT11_MAX_MPDU_BODY_LEN + \
					 DOT11_ICV_LEN + \
					 DOT11_FCS_LEN)	/* d11 max MPDU length */

#define DOT11_MAX_SSID_LEN		32	/* d11 max ssid length */
#define DOT11_SHORT_SSID_LEN		4	/* d11 short ssid length */

/* dot11RTSThreshold */
#define DOT11_DEFAULT_RTS_LEN		2347	/* d11 default RTS length */
#define DOT11_MAX_RTS_LEN		2347	/* d11 max RTS length */

/* dot11FragmentationThreshold */
#define DOT11_MIN_FRAG_LEN		256	/* d11 min fragmentation length */
#define DOT11_MAX_FRAG_LEN		2346	/* Max frag is also limited by aMPDUMaxLength
						* of the attached PHY
						*/
#define DOT11_DEFAULT_FRAG_LEN		2346	/* d11 default fragmentation length */

/* dot11BeaconPeriod */
#define DOT11_MIN_BEACON_PERIOD		1	/* d11 min beacon period */
#define DOT11_MAX_BEACON_PERIOD		0xFFFF	/* d11 max beacon period */

/* dot11DTIMPeriod */
#define DOT11_MIN_DTIM_PERIOD		1	/* d11 min DTIM period */
#define DOT11_MAX_DTIM_PERIOD		0xFF	/* d11 max DTIM period */

/** 802.2 LLC/SNAP header used by 802.11 per 802.1H */
#define DOT11_LLC_SNAP_HDR_LEN		8	/* d11 LLC/SNAP header length */
/* minimum LLC header length; DSAP, SSAP, 8 bit Control (unnumbered) */
#define DOT11_LLC_HDR_LEN_MIN		3
#define DOT11_OUI_LEN			3	/* d11 OUI length */
BWL_PRE_PACKED_STRUCT struct dot11_llc_snap_header {
	uint8	dsap;				/* always 0xAA */
	uint8	ssap;				/* always 0xAA */
	uint8	ctl;				/* always 0x03 */
	uint8	oui[DOT11_OUI_LEN];		/* RFC1042: 0x00 0x00 0x00
						 * Bridge-Tunnel: 0x00 0x00 0xF8
						 */
	uint16	type;				/* ethertype */
} BWL_POST_PACKED_STRUCT;

/* RFC1042 header used by 802.11 per 802.1H */
#define RFC1042_HDR_LEN	(ETHER_HDR_LEN + DOT11_LLC_SNAP_HDR_LEN)	/* RCF1042 header length */

/* Generic 802.11 MAC header */
/**
 * N.B.: This struct reflects the full 4 address 802.11 MAC header.
 *		 The fields are defined such that the shorter 1, 2, and 3
 *		 address headers just use the first k fields.
 */
BWL_PRE_PACKED_STRUCT struct dot11_header {
	uint16			fc;		/* frame control */
	uint16			durid;		/* duration/ID */
	struct ether_addr	a1;		/* address 1 */
	struct ether_addr	a2;		/* address 2 */
	struct ether_addr	a3;		/* address 3 */
	uint16			seq;		/* sequence control */
	struct ether_addr	a4;		/* address 4 */
} BWL_POST_PACKED_STRUCT;

/* Control frames */

BWL_PRE_PACKED_STRUCT struct dot11_rts_frame {
	uint16			fc;		/* frame control */
	uint16			durid;		/* duration/ID */
	struct ether_addr	ra;		/* receiver address */
	struct ether_addr	ta;		/* transmitter address */
} BWL_POST_PACKED_STRUCT;
#define	DOT11_RTS_LEN		16		/* d11 RTS frame length */

BWL_PRE_PACKED_STRUCT struct dot11_cts_frame {
	uint16			fc;		/* frame control */
	uint16			durid;		/* duration/ID */
	struct ether_addr	ra;		/* receiver address */
} BWL_POST_PACKED_STRUCT;
#define	DOT11_CTS_LEN		10		/* d11 CTS frame length */

BWL_PRE_PACKED_STRUCT struct dot11_ack_frame {
	uint16			fc;		/* frame control */
	uint16			durid;		/* duration/ID */
	struct ether_addr	ra;		/* receiver address */
} BWL_POST_PACKED_STRUCT;
#define	DOT11_ACK_LEN		10		/* d11 ACK frame length */

BWL_PRE_PACKED_STRUCT struct dot11_ps_poll_frame {
	uint16			fc;		/* frame control */
	uint16			durid;		/* AID */
	struct ether_addr	bssid;		/* receiver address, STA in AP */
	struct ether_addr	ta;		/* transmitter address */
} BWL_POST_PACKED_STRUCT;
#define	DOT11_PS_POLL_LEN	16		/* d11 PS poll frame length */

BWL_PRE_PACKED_STRUCT struct dot11_cf_end_frame {
	uint16			fc;		/* frame control */
	uint16			durid;		/* duration/ID */
	struct ether_addr	ra;		/* receiver address */
	struct ether_addr	bssid;		/* transmitter address, STA in AP */
} BWL_POST_PACKED_STRUCT;
#define	DOT11_CS_END_LEN	16		/* d11 CF-END frame length */

/**
 * RWL wifi protocol: The Vendor Specific Action frame is defined for vendor-specific signaling
 *  category+OUI+vendor specific content ( this can be variable)
 */
BWL_PRE_PACKED_STRUCT struct dot11_action_wifi_vendor_specific {
	uint8	category;
	uint8	OUI[3];
	uint8	type;
	uint8	subtype;
	uint8	data[1040];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_action_wifi_vendor_specific dot11_action_wifi_vendor_specific_t;

/** generic vendor specific action frame with variable length */
BWL_PRE_PACKED_STRUCT struct dot11_action_vs_frmhdr {
	uint8	category;
	uint8	OUI[3];
	uint8	type;
	uint8	subtype;
	uint8	data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_action_vs_frmhdr dot11_action_vs_frmhdr_t;

BWL_PRE_PACKED_STRUCT struct dot11_action_vs_frmhdr_v2 {
	uint8	category;
	uint8	OUI[DOT11_OUI_LEN];
	uint8	data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_action_vs_frmhdr_v2 dot11_action_vs_frmhdr_v2_t;

#define APPLE_OUI			"\x00\x17\xF2"

/* APPLE ASR INFO IE */
BWL_PRE_PACKED_STRUCT struct asr_ie {
	uint8 id;	/* DOT11_MNG_VS_ID */
	uint8 len;
	uint8 oui[3];	/* APPLE_OUI */
	uint8 type;	/* ASR_INFO_OUI_TYPE */
	uint8 info;
} BWL_POST_PACKED_STRUCT;
typedef struct asr_ie asr_ie_t;

BWL_PRE_PACKED_STRUCT struct asr_vsie {
	uint8 id;	/* DOT11_MNG_VS_ID */
	uint8 len;
	uint8 oui[3];	/* APPLE_OUI */
	uint8 oui_type;	/* ASR_INFO_OUI_TYPE */
	uint8 data[0];
} BWL_POST_PACKED_STRUCT;
typedef struct asr_vsie asr_vsie_t;

BWL_PRE_PACKED_STRUCT struct asr_vsie_data {
	uint8 version;
	uint8 sub_type;
	uint8 length;
	uint8 data[0];
} BWL_POST_PACKED_STRUCT;
typedef struct asr_vsie_data asr_vsie_data_t;

typedef enum {
	ASR_SUBTYPE_CAP = 0,
	ASR_SUBTYPE_REQUEST = 1,
	ASR_SUBTYPE_MEASUREMENT_REPORT = 2,
	ASR_SUBTYPE_MAX = 3
} asr_subtype_t;

BWL_PRE_PACKED_STRUCT struct asr_vsie_cap {
	uint16 cap;
} BWL_POST_PACKED_STRUCT;
typedef struct asr_vsie_cap asr_vsie_cap_t;

#define ASR_INFO_OUI_TYPE		0x0d	/* ASR Info */
#define ASR_INFO_ENAB			0x01	/* ASR capable */
						/* B1..B7 are reserved */

#define DOT11_ACTION_VS_HDR_LEN	6

#define BCM_ACTION_OUI_BYTE0	0x00
#define BCM_ACTION_OUI_BYTE1	0x90
#define BCM_ACTION_OUI_BYTE2	0x4c

/* BA/BAR Control parameters */
#define DOT11_BA_CTL_POLICY_NORMAL	0x0000	/* normal ack */
#define DOT11_BA_CTL_POLICY_NOACK	0x0001	/* no ack */
#define DOT11_BA_CTL_POLICY_MASK	0x0001	/* ack policy mask */

#define DOT11_BA_CTL_MTID		0x0002	/* multi tid BA */
#define DOT11_BA_CTL_COMPRESSED		0x0004	/* compressed bitmap */

#define DOT11_BA_CTL_NUMMSDU_MASK	0x0FC0	/* num msdu in bitmap mask */
#define DOT11_BA_CTL_NUMMSDU_SHIFT	6	/* num msdu in bitmap shift */

#define DOT11_BA_CTL_TID_MASK		0xF000	/* tid mask */
#define DOT11_BA_CTL_TID_SHIFT		12	/* tid shift */

/** control frame header (BA/BAR) */
BWL_PRE_PACKED_STRUCT struct dot11_ctl_header {
	uint16			fc;		/* frame control */
	uint16			durid;		/* duration/ID */
	struct ether_addr	ra;		/* receiver address */
	struct ether_addr	ta;		/* transmitter address */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ctl_header dot11_ctl_header_t;
#define DOT11_CTL_HDR_LEN	16		/* control frame hdr len */

/** BAR frame payload */
BWL_PRE_PACKED_STRUCT struct dot11_bar {
	uint16			bar_control;	/* BAR Control */
	uint16			seqnum;		/* Starting Sequence control */
} BWL_POST_PACKED_STRUCT;
#define DOT11_BAR_LEN		4		/* BAR frame payload length */

#define DOT11_BA_BITMAP_LEN	128		/* bitmap length */
#define DOT11_BA_CMP_BITMAP_LEN	8		/* compressed bitmap length */
/** BA frame payload */
BWL_PRE_PACKED_STRUCT struct dot11_ba {
	uint16			ba_control;	/* BA Control */
	uint16			seqnum;		/* Starting Sequence control */
	uint8			bitmap[DOT11_BA_BITMAP_LEN];	/* Block Ack Bitmap */
} BWL_POST_PACKED_STRUCT;
#define DOT11_BA_LEN		4		/* BA frame payload len (wo bitmap) */

/** Management frame header */
BWL_PRE_PACKED_STRUCT struct dot11_management_header {
	uint16			fc;		/* frame control */
	uint16			durid;		/* duration/ID */
	struct ether_addr	da;		/* receiver address */
	struct ether_addr	sa;		/* transmitter address */
	struct ether_addr	bssid;		/* BSS ID */
	uint16			seq;		/* sequence control */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_management_header dot11_management_header_t;
#define	DOT11_MGMT_HDR_LEN	24		/* d11 management header length */

/* Management frame payloads */

BWL_PRE_PACKED_STRUCT struct dot11_bcn_prb {
	uint32			timestamp[2];
	uint16			beacon_interval;
	uint16			capability;
} BWL_POST_PACKED_STRUCT;
#define	DOT11_BCN_PRB_LEN	12		/* 802.11 beacon/probe frame fixed length */
#define	DOT11_BCN_PRB_FIXED_LEN	12		/* 802.11 beacon/probe frame fixed length */

BWL_PRE_PACKED_STRUCT struct dot11_auth {
	uint16			alg;		/* algorithm */
	uint16			seq;		/* sequence control */
	uint16			status;		/* status code */
} BWL_POST_PACKED_STRUCT;
#define DOT11_AUTH_FIXED_LEN		6	/* length of auth frame without challenge IE */
#define DOT11_AUTH_SEQ_STATUS_LEN	4	/* length of auth frame without challenge IE and
						 * without algorithm
						 */

BWL_PRE_PACKED_STRUCT struct dot11_assoc_req {
	uint16			capability;	/* capability information */
	uint16			listen;		/* listen interval */
} BWL_POST_PACKED_STRUCT;
#define DOT11_ASSOC_REQ_FIXED_LEN	4	/* length of assoc frame without info elts */

BWL_PRE_PACKED_STRUCT struct dot11_reassoc_req {
	uint16			capability;	/* capability information */
	uint16			listen;		/* listen interval */
	struct ether_addr	ap;		/* Current AP address */
} BWL_POST_PACKED_STRUCT;
#define DOT11_REASSOC_REQ_FIXED_LEN	10	/* length of assoc frame without info elts */

BWL_PRE_PACKED_STRUCT struct dot11_assoc_resp {
	uint16			capability;	/* capability information */
	uint16			status;		/* status code */
	uint16			aid;		/* association ID */
} BWL_POST_PACKED_STRUCT;
#define DOT11_ASSOC_RESP_FIXED_LEN	6	/* length of assoc resp frame without info elts */

BWL_PRE_PACKED_STRUCT struct dot11_action_measure {
	uint8	category;
	uint8	action;
	uint8	token;
	uint8	data[1];
} BWL_POST_PACKED_STRUCT;
#define DOT11_ACTION_MEASURE_LEN	3	/* d11 action measurement header length */

BWL_PRE_PACKED_STRUCT struct dot11_action_ht_ch_width {
	uint8	category;
	uint8	action;
	uint8	ch_width;
} BWL_POST_PACKED_STRUCT;

BWL_PRE_PACKED_STRUCT struct dot11_action_ht_mimops {
	uint8	category;
	uint8	action;
	uint8	control;
} BWL_POST_PACKED_STRUCT;

/*  Operating channel information element */
BWL_PRE_PACKED_STRUCT struct dot11_oci_ie {
	uint8 op_class; /* Operating class */
	uint8 channel; /* Primary Channel */
	uint8 seg1_chan; /* Frequency segment 1 channel number */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_oci_ie dot11_oci_ie_t;

#define DOT11_OCI_IE_LEN (sizeof(dot11_oci_ie_t))

BWL_PRE_PACKED_STRUCT struct dot11_action_sa_query {
	uint8	category;
	uint8	action;
	uint16	id;
	uint8 data[0];
} BWL_POST_PACKED_STRUCT;

BWL_PRE_PACKED_STRUCT struct dot11_action_vht_oper_mode {
	uint8	category;
	uint8	action;
	uint8	mode;
} BWL_POST_PACKED_STRUCT;

/* These lengths assume 64 MU groups, as specified in 802.11ac-2013 */
#define DOT11_ACTION_GID_MEMBERSHIP_LEN  8    /* bytes */
#define DOT11_ACTION_GID_USER_POS_LEN   16    /* bytes */
BWL_PRE_PACKED_STRUCT struct dot11_action_group_id {
	uint8   category;
	uint8   action;
	uint8   membership_status[DOT11_ACTION_GID_MEMBERSHIP_LEN];
	uint8   user_position[DOT11_ACTION_GID_USER_POS_LEN];
} BWL_POST_PACKED_STRUCT;

#define SM_PWRSAVE_ENABLE	1
#define SM_PWRSAVE_MODE		2

/* ************* 802.11h related definitions. ************* */
BWL_PRE_PACKED_STRUCT struct dot11_power_cnst {
	uint8 id;
	uint8 len;
	uint8 power;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_power_cnst dot11_power_cnst_t;

BWL_PRE_PACKED_STRUCT struct dot11_power_cap {
	int8 min;
	int8 max;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_power_cap dot11_power_cap_t;

BWL_PRE_PACKED_STRUCT struct dot11_tpc_rep {
	uint8 id;
	uint8 len;
	uint8 tx_pwr;
	uint8 margin;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tpc_rep dot11_tpc_rep_t;
#define DOT11_MNG_IE_TPC_REPORT_LEN	2 	/* length of IE data, not including 2 byte header */

BWL_PRE_PACKED_STRUCT struct dot11_supp_channels {
	uint8 id;
	uint8 len;
	uint8 first_channel;
	uint8 num_channels;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_supp_channels dot11_supp_channels_t;

/**
 * Secondary Channel Offset IE
 * Specifies the position of secondary 20MHz channel in relation to the primary 20MHz channel.
 * Values of the Secondary Channel Offset field
 * 0 = SCN - no secondary channel in case bandwidth is 20MHz
 * 1 = SCA - secondary channel is above control channel
 * 3 = SCB - secondary channel is below control channel
 */
BWL_PRE_PACKED_STRUCT struct dot11_sec_ch_offs {
	uint8	id;		/* IE ID, 62, DOT11_MNG_SEC_CHANNEL_OFFSET */
	uint8	len;		/* IE length */
	uint8	sec_ch_offs;	/* IE field */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_sec_ch_offs dot11_sec_ch_offs_ie_t;

BWL_PRE_PACKED_STRUCT struct dot11_brcm_extch {
	uint8	id;		/* IE ID, 221, DOT11_MNG_PROPR_ID */
	uint8	len;		/* IE length */
	uint8	oui[3];
	uint8	type;           /* type indicates what follows */
	uint8	extch;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_brcm_extch dot11_brcm_extch_ie_t;

#define BRCM_EXTCH_IE_LEN	5
#define BRCM_EXTCH_IE_TYPE	53	/* 802.11n ID not yet assigned */

#define DOT11_SEC_CH_IE_LEN	1u
#define DOT11_SEC_CH_MASK	0x03	/* secondary 20MHz channel offset_mask */
#define DOT11_SEC_CH_NONE	0u	/* no secondary 20MHz channel */
#define DOT11_SEC_CH_ABOVE	1u	/* secondary 20MHz channel on upper sb */
#define DOT11_SEC_CH_BELOW	3u	/* secondary 20MHz channel on lower sb */

BWL_PRE_PACKED_STRUCT struct dot11_brcm_radarch {
	uint8	id;		/* IE ID, 221, DOT11_MNG_PROPR_ID */
	uint8	len;		/* IE length */
	uint8	oui[3];
	uint8	type;           /* type indicates what follows */
	uint8	radarch[];	/* list of channels on which radar is detected */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_brcm_radarch dot11_brcm_radarch_ie_t;

#define BRCM_RADARCH_IE_LEN	4
#define BRCM_RADARCH_IE_TYPE	56

BWL_PRE_PACKED_STRUCT struct dot11_action_frmhdr {
	uint8	category;
	uint8	action;
	uint8	data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_action_frmhdr dot11_action_frmhdr_t;
#define DOT11_ACTION_FRMHDR_LEN	2

/** CSA IE data structure */
BWL_PRE_PACKED_STRUCT struct dot11_channel_switch {
	uint8 id;	/* id DOT11_MNG_CHANNEL_SWITCH_ID */
	uint8 len;	/* length of IE */
	uint8 mode;	/* mode 0 or 1 */
	uint8 channel;	/* channel switch to */
	uint8 count;	/* number of beacons before switching */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_channel_switch dot11_chan_switch_ie_t;

/** mbssid index IE data structure */
BWL_PRE_PACKED_STRUCT struct dot11_mbssid_index_ie {
	uint8 id;       /* id DOT11_MNG_MULTIPLE_BSSID_IDX_ID */
	uint8 len;      /* length of IE */
	uint8 bssid_index;     /* index 1 --- n */
	uint8 dtim_period;
	uint8 dtim_count;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mbssid_index_ie dot11_mbssid_index_ie_t;

BWL_PRE_PACKED_STRUCT struct dot11_mbssid_profile_subie {
	uint8 subie_id; /* 0 */
	uint8 subie_len;
	uint8 moreie[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mbssid_profile_subie dot11_mbssid_profile_subie_t;

/** mbssid  IE data structure */
BWL_PRE_PACKED_STRUCT struct dot11_mbssid_ie {
	uint8 id;       /* id DOT11_MNG_MULTIPLE_BSSID_ID */
	uint8 len;      /* length of IE */
	uint8 maxbssid_indicator;     /* if assigned n, 2^n is the max bss allowed */
	dot11_mbssid_profile_subie_t profile[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mbssid_ie dot11_mbssid_ie_t;

/** mbssid subelement IE data structure */
BWL_PRE_PACKED_STRUCT struct dot11_mbssid_nontrans_subelement {
	uint8 id;       /* id : sublement 0 */
	uint8 len;      /* length of IE */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mbssid_nontrans_subelement  dot11_mbssid_nontrans_subelement_t;

/* Multiple BSSID element */
BWL_PRE_PACKED_STRUCT struct dot11_mbssid_cap {
	uint8 id; /* 83 */
	uint8 len;
	uint16 capability;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mbssid_cap dot11_mbssid_cap_t;

/* *********************************
 * *****  NON INHERITANCE IE  ******
 * *********************************
 */
/*  non-inheritance element */
BWL_PRE_PACKED_STRUCT struct dot11_mbssid_non_inheritance {
	uint8 id; /* 255 */
	uint8 len;
	uint8 ext_id; /* 56 */
	uint8 var_list[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mbssid_non_inheritance dot11_mbssid_non_inheritance_t;

/* P802.11REVmd D5.0 Figure 9-779 List Of Element ID Extensions field */
typedef BWL_PRE_PACKED_STRUCT struct non_inh_id_ext_list {
	uint8   num_id_ext;
	uint8   id_ext[];
} BWL_POST_PACKED_STRUCT non_inh_id_ext_list_t;

/* TBD */
BWL_PRE_PACKED_STRUCT struct non_inh_ie {
	uint8 id;
	uint8 len;
	uint8 id_ext;
	/* non_inh_id_list_t id_list */
	/* non_inh_id_ext_list_t id_ext_list */
} BWL_POST_PACKED_STRUCT;
typedef struct non_inh_ie non_inh_ie_t;

/* P802.11REVmd D5.0 Figure 9-778 List Of Element IDs field */
typedef BWL_PRE_PACKED_STRUCT struct non_inh_id_list {
	uint8   num_id;
	uint8   id[];
} BWL_POST_PACKED_STRUCT non_inh_id_list_t;

/*  mbssid configuration element */
BWL_PRE_PACKED_STRUCT struct dot11_mbssid_configuration_ie {
	uint8 id; /* 255 */
	uint8 len;
	uint8 ext_id; /* 55 */
	uint8 bssid_count; /* active bss count */
	uint8 periodicity; /* no of beacons need to get complete non tx profile */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mbssid_configuration_ie dot11_mbssid_configuration_ie_t;

#define DOT11_SWITCH_IE_LEN	3	/* length of IE data, not including 2 byte header */
/* CSA mode - 802.11h-2003 $7.3.2.20 */
#define DOT11_CSA_MODE_ADVISORY		0	/* no DOT11_CSA_MODE_NO_TX restriction imposed */
#define DOT11_CSA_MODE_NO_TX		1	/* no transmission upon receiving CSA frame. */

/* CSA mode for AP's own specific use case. Purpose is switch to DFS
 * channel, do CAC and return to previous channel on CAC completion.
 *
 * AP does not expose this mode to clients.
 *
 * Clients receives by default CSA_MODE_ADVISORY mode in CSA frame
 * if not explictly stated otherwise.
 */
#define DOT11_CSA_PROPRIETARY_MODE_DO_CAC_AND_RETURN_TO_OLD_CHAN	100

BWL_PRE_PACKED_STRUCT struct dot11_action_switch_channel {
	uint8	category;
	uint8	action;
	dot11_chan_switch_ie_t chan_switch_ie;	/* for switch IE */
	dot11_brcm_extch_ie_t extch_ie;		/* extension channel offset */
} BWL_POST_PACKED_STRUCT;

BWL_PRE_PACKED_STRUCT struct dot11_csa_body {
	uint8 mode;	/* mode 0 or 1 */
	uint8 reg;	/* regulatory class */
	uint8 channel;	/* channel switch to */
	uint8 count;	/* number of beacons before switching */
} BWL_POST_PACKED_STRUCT;

/** 11n Extended Channel Switch IE data structure */
BWL_PRE_PACKED_STRUCT struct dot11_ext_csa {
	uint8 id;	/* id DOT11_MNG_EXT_CSA_ID */
	uint8 len;	/* length of IE */
	struct dot11_csa_body b;	/* body of the ie */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ext_csa dot11_ext_csa_ie_t;
#define DOT11_EXT_CSA_IE_LEN	4	/* length of extended channel switch IE body */

BWL_PRE_PACKED_STRUCT struct dot11_action_ext_csa {
	uint8	category;
	uint8	action;
	dot11_ext_csa_ie_t chan_switch_ie;	/* for switch IE */
} BWL_POST_PACKED_STRUCT;

BWL_PRE_PACKED_STRUCT struct dot11y_action_ext_csa {
	uint8	category;
	uint8	action;
	struct dot11_csa_body b;	/* body of the ie */
} BWL_POST_PACKED_STRUCT;

/**  Wide Bandwidth Channel Switch IE data structure */
BWL_PRE_PACKED_STRUCT struct dot11_wide_bw_channel_switch {
	uint8 id;				/* id DOT11_MNG_WIDE_BW_CHANNEL_SWITCH_ID */
	uint8 len;				/* length of IE */
	uint8 channel_width;			/* new channel width */
	uint8 center_frequency_segment_0;	/* center frequency segment 0 */
	uint8 center_frequency_segment_1;	/* center frequency segment 1 */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_wide_bw_channel_switch dot11_wide_bw_chan_switch_ie_t;

#define DOT11_WIDE_BW_SWITCH_IE_LEN     3       /* length of IE data, not including 2 byte header */

/**
 * Bandwidth Indication Information field
 * IEEE 802.11be D3.0 Figure 9-1002c EHT Operation Information field format
 */
BWL_PRE_PACKED_STRUCT struct dot11_bw_ind_info {
	uint8 channel_width;		/* Control: Channel Width */
	uint8 ccfs0;			/* CCFS0: Channel Center Frequency Segment 0 */
	uint8 ccfs1;			/* CCFS1: Channel Center Frequency Segment 1 */
	uint16 dis_subchan_bmp;		/* Disabled Subchannel Bitmap */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_bw_ind_info dot11_bw_ind_info_t;

/**
 * Bandwidth Indication IE data structure
 * IEEE 802.11be D3.0 paragraph 9.4.2.319
 */
BWL_PRE_PACKED_STRUCT struct dot11_bw_ind {
	uint8 id;				/* DOT11_MNG_ID_EXT_ID */
	uint8 len;				/* length of IE */
	uint8 element_id_ext;			/* EXT_MNG_BW_IND_ID */
	uint8 bandwidth_ind_params;		/* Bandwidth Indication Parameters */
	dot11_bw_ind_info_t dot11_bw_ind_info;	/* Bandwidth Indication Information field */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_bw_ind dot11_bw_ind_ie_t;

#define BW_IND_PARAMS		0x2	/* B1 set B0, B2..B7 reserved bits set to 0 */
#define DIS_SUBCHAN_BMP		2	/* Disabled Subchannel Bitmap */
#define DOT11_BW_IND_IE_LEN     7       /* length of IE data, not including 2 byte header */

/** Channel Switch Wrapper IE data structure */
BWL_PRE_PACKED_STRUCT struct dot11_channel_switch_wrapper {
	uint8 id;				/* DOT11_MNG_CHANNEL_SWITCH_WRAPPER_ID */
	uint8 len;				/* length of IE */
	dot11_wide_bw_chan_switch_ie_t wb_chan_switch_ie;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_channel_switch_wrapper dot11_chan_switch_wrapper_ie_t;

/* IEEE P802.11-REVme/D1.3 Table 9-212 HT/VHT Operation Information subfields */
typedef enum ht_vht_op_chan_width {
	HT_VHT_OP_CHAN_WIDTH_20		= 0,
	HT_VHT_OP_CHAN_WIDTH_40		= 1,
	HT_VHT_OP_CHAN_WIDTH_80		= 2,
	HT_VHT_OP_CHAN_WIDTH_160	= 3,
	HT_VHT_OP_CHAN_WIDTH_80_80	= 4
} ht_vht_op_chan_width_t;

/**  Wide Bandwidth Channel IE data structure */
BWL_PRE_PACKED_STRUCT struct dot11_wide_bw_channel {
	uint8 id;				/* id DOT11_MNG_WIDE_BW_CHANNEL_ID */
	uint8 len;				/* length of IE */
	uint8 channel_width;			/* channel width */
	uint8 center_frequency_segment_0;	/* center frequency segment 0 */
	uint8 center_frequency_segment_1;	/* center frequency segment 1 */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_wide_bw_channel dot11_wide_bw_chan_ie_t;

#define DOT11_WIDE_BW_IE_LEN     3       /* length of IE data, not including 2 byte header */
/** >=VHT Transmit Power Envelope IE data structure */
BWL_PRE_PACKED_STRUCT struct dot11_transmit_power_envelope {
	uint8 id;				/* DOT11_MNG_TRANSMIT_POWER_ENVELOPE_ID */
	uint8 len;				/* length of IE */
	uint8 transmit_power_info;
	uint8 local_max_transmit_power_20;
} BWL_POST_PACKED_STRUCT;

typedef struct dot11_transmit_power_envelope dot11_transmit_power_envelope_ie_t;
typedef struct dot11_transmit_power_envelope dot11_vht_transmit_power_envelope_ie_t; /* obsolete */

/* vht transmit power envelope IE length depends on channel width */
enum dot11_pwr_envelope_ie_len_e {
	DOT11_TRANSMIT_PWR_ENVELOPE_IE_LEN_40MHZ = 1,
	DOT11_TRANSMIT_PWR_ENVELOPE_IE_LEN_80MHZ = 2,
	DOT11_TRANSMIT_PWR_ENVELOPE_IE_LEN_160MHZ = 3
};

/* below DOT11_VHT_TRANSMIT_PWR_ENVELOPE_IE* definitions are going to be obsoleted */
#define DOT11_VHT_TRANSMIT_PWR_ENVELOPE_IE_LEN_40MHZ DOT11_TRANSMIT_PWR_ENVELOPE_IE_LEN_40MHZ
#define DOT11_VHT_TRANSMIT_PWR_ENVELOPE_IE_LEN_80MHZ DOT11_TRANSMIT_PWR_ENVELOPE_IE_LEN_80MHZ
#define DOT11_VHT_TRANSMIT_PWR_ENVELOPE_IE_LEN_160MHZ DOT11_TRANSMIT_PWR_ENVELOPE_IE_LEN_160MHZ

BWL_PRE_PACKED_STRUCT struct dot11_obss_coex {
	uint8	id;
	uint8	len;
	uint8	info;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_obss_coex dot11_obss_coex_t;
#define DOT11_OBSS_COEXINFO_LEN	1	/* length of OBSS Coexistence INFO IE */

#define	DOT11_OBSS_COEX_INFO_REQ		0x01
#define	DOT11_OBSS_COEX_40MHZ_INTOLERANT	0x02
#define	DOT11_OBSS_COEX_20MHZ_WIDTH_REQ	0x04

BWL_PRE_PACKED_STRUCT struct dot11_obss_chanlist {
	uint8	id;
	uint8	len;
	uint8	regclass;
	uint8	chanlist[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_obss_chanlist dot11_obss_chanlist_t;
#define DOT11_OBSS_CHANLIST_FIXED_LEN	1	/* fixed length of regclass */

BWL_PRE_PACKED_STRUCT struct dot11_extcap_ie {
	uint8 id;
	uint8 len;
	uint8 cap[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_extcap_ie dot11_extcap_ie_t;

#define DOT11_EXTCAP_LEN_COEX	1
#define DOT11_EXTCAP_LEN_BT	3
#define DOT11_EXTCAP_LEN_IW	4
#define DOT11_EXTCAP_LEN_SI	6

#define DOT11_EXTCAP_LEN_TDLS	5
#define DOT11_11AC_EXTCAP_LEN_TDLS	8

#define DOT11_EXTCAP_LEN_FMS			2
#define DOT11_EXTCAP_LEN_PROXY_ARP		2
#define DOT11_EXTCAP_LEN_TFS			3
#define DOT11_EXTCAP_LEN_WNM_SLEEP		3
#define DOT11_EXTCAP_LEN_TIMBC			3
#define DOT11_EXTCAP_LEN_BSSTRANS		3
#define DOT11_EXTCAP_LEN_MBSSID                 3
#define DOT11_EXTCAP_LEN_DMS			4
#define DOT11_EXTCAP_LEN_WNM_NOTIFICATION	6
#define DOT11_EXTCAP_LEN_TDLS_WBW		8
#define DOT11_EXTCAP_LEN_OPMODE_NOTIFICATION	8
#define DOT11_EXTCAP_LEN_OBSS_NBWRU_TOLERANCE	10u
#define DOT11_EXTCAP_LEN_BCN_PROT		11u

/* TDLS Capabilities */
#define DOT11_TDLS_CAP_TDLS			37		/* TDLS support */
#define DOT11_TDLS_CAP_PU_BUFFER_STA	28		/* TDLS Peer U-APSD buffer STA support */
#define DOT11_TDLS_CAP_PEER_PSM		20		/* TDLS Peer PSM support */
#define DOT11_TDLS_CAP_CH_SW			30		/* TDLS Channel switch */
#define DOT11_TDLS_CAP_PROH			38		/* TDLS prohibited */
#define DOT11_TDLS_CAP_CH_SW_PROH		39		/* TDLS Channel switch prohibited */
#define DOT11_TDLS_CAP_TDLS_WIDER_BW	61	/* TDLS Wider Band-Width */

#define TDLS_CAP_MAX_BIT		39		/* TDLS max bit defined in ext cap */

#define DOT11_CAP_SAE_HASH_TO_ELEMENT   5       /* SAE Hash-to-element support */
#define DOT11_EXT_RSN_CAP_SAE_PK	6u	/* SAE-PK support */
/* Draft P802.11az/D2.5 - Table 9-321 Extended RSN Capabilities field */
#define DOT11_EXT_RSN_CAP_SECURE_LTF		8u	/* Secure LTF support */
#define DOT11_EXT_RSN_CAP_SECURE_RTT		9u	/* Secure RTT(EDMG measurement) support */
/* Protection of Ranging management frame is required */
#define DOT11_EXT_RSN_CAP_URNM_MFPR_X20		10u	/* except BW20 ranging */
#define DOT11_EXT_RSN_CAP_RANGE_PMF_REQUIRED	DOT11_EXT_RSN_CAP_URNM_MFPR_X20 /* to be removed */
#define DOT11_EXT_RSN_CAP_URNM_MFPR		15u	/* P802.11az/D4.1 */

#define DOT11_EXT_RSN_CAP_MAX_BIT       DOT11_EXT_RSN_CAP_URNM_MFPR /* Last bit */
#define DOT11_EXT_RSN_CAP_NUM_BITS_MAX	(DOT11_EXT_RSN_CAP_MAX_BIT + 1) /* last bit idx + 1 */
#define DOT11_EXT_RSN_CAP_BYTE_LEN_MAX	((DOT11_EXT_RSN_CAP_NUM_BITS_MAX + 7) >> 3) /* byte len */

#define DOT11_BSS_MEMBERSHIP_SELECTOR_SAE_H2E_ONLY	0x7B	/* 123 */

BWL_PRE_PACKED_STRUCT struct dot11_rsnxe {
	uint8 id;       /* id DOT11_MNG_RSNXE_ID */
	uint8 len;
	uint8 cap[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rsnxe dot11_rsnxe_t;

#define RSNXE_CAP_LENGTH_MASK           (0x0f)
#define RSNXE_CAP_LENGTH(cap)           ((uint8)(cap) & RSNXE_CAP_LENGTH_MASK)
#define RSNXE_SET_CAP_LENGTH(cap, len)\
	(cap = (cap & ~RSNXE_CAP_LENGTH_MASK) | ((uint8)(len) & RSNXE_CAP_LENGTH_MASK))
#define DOT11_FTIE_RSNXE_USED	0x1u
/* The first 4 bit of CAPs field is the length of the Extended RSN Capabilities minus 1 */
#define IS_RSNXE_VALID(ie)	((ie) && (((dot11_rsnxe_t *)(ie))->len >= 1u) && \
					(((dot11_rsnxe_t *)(ie))->len > \
					RSNXE_CAP_LENGTH(((dot11_rsnxe_t *)(ie))->cap[0])))

BWL_PRE_PACKED_STRUCT struct dot11_rejected_groups_ie {
	uint8 id;       /* DOT11_MNG_EXT_ID */
	uint8 len;
	uint8 id_ext; /* DOT11_MNG_REJECTED_GROUPS_ID */
	uint16 groups[];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rejected_groups_ie dot11_rejected_groups_ie_t;

/* Diffie-Hellman Parameter element */
BWL_PRE_PACKED_STRUCT struct dh_params_ie {
	uint8 id;
	uint8 len;
	uint8 id_ext;
	uint16 owe_group;
	uint8 pub_key[253];
} BWL_POST_PACKED_STRUCT;

typedef struct dh_params_ie dh_params_ie_t;

/* 802.11h/802.11k Measurement Request/Report IEs */
/* Measurement Type field */
#define DOT11_MEASURE_TYPE_BASIC 	0   /* d11 measurement basic type */
#define DOT11_MEASURE_TYPE_CCA 		1   /* d11 measurement CCA type */
#define DOT11_MEASURE_TYPE_RPI		2   /* d11 measurement RPI type */
#define DOT11_MEASURE_TYPE_CHLOAD	3   /* d11 measurement Channel Load type */
#define DOT11_MEASURE_TYPE_NOISE	4   /* d11 measurement Noise Histogram type */
#define DOT11_MEASURE_TYPE_BEACON	5   /* d11 measurement Beacon type */
#define DOT11_MEASURE_TYPE_FRAME	6   /* d11 measurement Frame type */
#define DOT11_MEASURE_TYPE_STAT		7   /* d11 measurement STA Statistics type */
#define DOT11_MEASURE_TYPE_LCI		8   /* d11 measurement LCI type */
#define DOT11_MEASURE_TYPE_TXSTREAM	9   /* d11 measurement TX Stream type */
#define DOT11_MEASURE_TYPE_MCDIAGS	10  /* d11 measurement multicast diagnostics */
#define DOT11_MEASURE_TYPE_CIVICLOC	11  /* d11 measurement location civic */
#define DOT11_MEASURE_TYPE_LOC_ID	12  /* d11 measurement location identifier */
#define DOT11_MEASURE_TYPE_DIRCHANQ	13  /* d11 measurement dir channel quality */
#define DOT11_MEASURE_TYPE_DIRMEAS	14  /* d11 measurement directional */
#define DOT11_MEASURE_TYPE_DIRSTATS	15  /* d11 measurement directional stats */
#define DOT11_MEASURE_TYPE_FTMRANGE	16  /* d11 measurement Fine Timing */
#define DOT11_MEASURE_TYPE_PAUSE	255	/* d11 measurement pause type */

/* Measurement Request Modes */
#define DOT11_MEASURE_MODE_PARALLEL 	(1<<0)	/* d11 measurement parallel */
#define DOT11_MEASURE_MODE_ENABLE 	(1<<1)	/* d11 measurement enable */
#define DOT11_MEASURE_MODE_REQUEST	(1<<2)	/* d11 measurement request */
#define DOT11_MEASURE_MODE_REPORT 	(1<<3)	/* d11 measurement report */
#define DOT11_MEASURE_MODE_DUR 	(1<<4)	/* d11 measurement dur mandatory */
/* Measurement Report Modes */
#define DOT11_MEASURE_MODE_LATE 	(1<<0)	/* d11 measurement late */
#define DOT11_MEASURE_MODE_INCAPABLE	(1<<1)	/* d11 measurement incapable */
#define DOT11_MEASURE_MODE_REFUSED	(1<<2)	/* d11 measurement refuse */
/* Basic Measurement Map bits */
#define DOT11_MEASURE_BASIC_MAP_BSS	((uint8)(1<<0))	/* d11 measurement basic map BSS */
#define DOT11_MEASURE_BASIC_MAP_OFDM	((uint8)(1<<1))	/* d11 measurement map OFDM */
#define DOT11_MEASURE_BASIC_MAP_UKNOWN	((uint8)(1<<2))	/* d11 measurement map unknown */
#define DOT11_MEASURE_BASIC_MAP_RADAR	((uint8)(1<<3))	/* d11 measurement map radar */
#define DOT11_MEASURE_BASIC_MAP_UNMEAS	((uint8)(1<<4))	/* d11 measurement map unmeasuremnt */

BWL_PRE_PACKED_STRUCT struct dot11_meas_req {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint8 channel;
	uint8 start_time[8];
	uint16 duration;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_meas_req dot11_meas_req_t;
#define DOT11_MNG_IE_MREQ_LEN 14	/* d11 measurement request IE length */
/* length of Measure Request IE data not including variable len */
#define DOT11_MNG_IE_MREQ_FIXED_LEN 3	/* d11 measurement request IE fixed length */

BWL_PRE_PACKED_STRUCT struct dot11_meas_req_loc {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	BWL_PRE_PACKED_STRUCT union
	{
		BWL_PRE_PACKED_STRUCT struct {
			uint8 subject;
			uint8 data[1];
		} BWL_POST_PACKED_STRUCT lci;
		BWL_PRE_PACKED_STRUCT struct {
			uint8 subject;
			uint8 type;  /* type of civic location */
			uint8 siu;   /* service interval units */
			uint16 si; /* service interval */
			uint8 data[1];
		} BWL_POST_PACKED_STRUCT civic;
		BWL_PRE_PACKED_STRUCT struct {
			uint8 subject;
			uint8 siu;   /* service interval units */
			uint16 si; /* service interval */
			uint8 data[1];
		} BWL_POST_PACKED_STRUCT locid;
		BWL_PRE_PACKED_STRUCT struct {
			uint16 max_init_delay;		/* maximum random initial delay */
			uint8 min_ap_count;
			uint8 data[1];
		} BWL_POST_PACKED_STRUCT ftm_range;
	} BWL_POST_PACKED_STRUCT req;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_meas_req_loc dot11_meas_req_loc_t;
#define DOT11_MNG_IE_MREQ_MIN_LEN           4	/* d11 measurement report IE length */
#define DOT11_MNG_IE_MREQ_LCI_FIXED_LEN     4	/* d11 measurement report IE length */
#define DOT11_MNG_IE_MREQ_CIVIC_FIXED_LEN   8	/* d11 measurement report IE length */
#define DOT11_MNG_IE_MREQ_FRNG_FIXED_LEN    6	/* d11 measurement report IE length */

BWL_PRE_PACKED_STRUCT struct dot11_lci_subelement {
	uint8 subelement;
	uint8 length;
	uint8 lci_data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_lci_subelement dot11_lci_subelement_t;

BWL_PRE_PACKED_STRUCT struct dot11_colocated_bssid_list_se {
	uint8 sub_id;
	uint8 length;
	uint8 max_bssid_ind; /* MaxBSSID Indicator */
	struct ether_addr bssid[1]; /* variable */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_colocated_bssid_list_se dot11_colocated_bssid_list_se_t;
#define DOT11_LCI_COLOCATED_BSSID_LIST_FIXED_LEN     3
#define DOT11_LCI_COLOCATED_BSSID_SUBELEM_ID         7

BWL_PRE_PACKED_STRUCT struct dot11_civic_subelement {
	uint8 type;  /* type of civic location */
	uint8 subelement;
	uint8 length;
	uint8 civic_data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_civic_subelement dot11_civic_subelement_t;

BWL_PRE_PACKED_STRUCT struct dot11_meas_rep {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	BWL_PRE_PACKED_STRUCT union
	{
		BWL_PRE_PACKED_STRUCT struct {
			uint8 channel;
			uint8 start_time[8];
			uint16 duration;
			uint8 map;
		} BWL_POST_PACKED_STRUCT basic;
		BWL_PRE_PACKED_STRUCT struct {
			uint8 subelement;
			uint8 length;
			uint8 data[1];
		} BWL_POST_PACKED_STRUCT lci;
		BWL_PRE_PACKED_STRUCT struct {
			uint8 type;  /* type of civic location */
			uint8 subelement;
			uint8 length;
			uint8 data[1];
		} BWL_POST_PACKED_STRUCT civic;
		BWL_PRE_PACKED_STRUCT struct {
			uint8 exp_tsf[8];
			uint8 subelement;
			uint8 length;
			uint8 data[1];
		} BWL_POST_PACKED_STRUCT locid;
		BWL_PRE_PACKED_STRUCT struct {
			uint8 entry_count;
			uint8 data[1];
		} BWL_POST_PACKED_STRUCT ftm_range;
		uint8 data[1];
	} BWL_POST_PACKED_STRUCT rep;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_meas_rep dot11_meas_rep_t;
#define DOT11_MNG_IE_MREP_MIN_LEN           5	/* d11 measurement report IE length */
#define DOT11_MNG_IE_MREP_LCI_FIXED_LEN     5	/* d11 measurement report IE length */
#define DOT11_MNG_IE_MREP_CIVIC_FIXED_LEN   6	/* d11 measurement report IE length */
#define DOT11_MNG_IE_MREP_LOCID_FIXED_LEN   13	/* d11 measurement report IE length */
#define DOT11_MNG_IE_MREP_BASIC_FIXED_LEN   15	/* d11 measurement report IE length */
#define DOT11_MNG_IE_MREP_FRNG_FIXED_LEN    4

/* length of Measure Report IE data not including variable len */
#define DOT11_MNG_IE_MREP_FIXED_LEN	3	/* d11 measurement response IE fixed length */

BWL_PRE_PACKED_STRUCT struct dot11_meas_rep_basic {
	uint8 channel;
	uint8 start_time[8];
	uint16 duration;
	uint8 map;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_meas_rep_basic dot11_meas_rep_basic_t;
#define DOT11_MEASURE_BASIC_REP_LEN	12	/* d11 measurement basic report length */

BWL_PRE_PACKED_STRUCT struct dot11_quiet {
	uint8 id;
	uint8 len;
	uint8 count;	/* TBTTs until beacon interval in quiet starts */
	uint8 period;	/* Beacon intervals between periodic quiet periods ? */
	uint16 duration;	/* Length of quiet period, in TU's */
	uint16 offset;	/* TU's offset from TBTT in Count field */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_quiet dot11_quiet_t;

BWL_PRE_PACKED_STRUCT struct chan_map_tuple {
	uint8 channel;
	uint8 map;
} BWL_POST_PACKED_STRUCT;
typedef struct chan_map_tuple chan_map_tuple_t;

BWL_PRE_PACKED_STRUCT struct dot11_ibss_dfs {
	uint8 id;
	uint8 len;
	uint8 eaddr[ETHER_ADDR_LEN];
	uint8 interval;
	chan_map_tuple_t map[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ibss_dfs dot11_ibss_dfs_t;

/* WME Elements */
#define WME_OUI			"\x00\x50\xf2"	/* WME OUI */
#define WME_OUI_LEN		3
#define WME_OUI_TYPE		2	/* WME type */
#define WME_TYPE		2	/* WME type, deprecated */
#define WME_SUBTYPE_IE		0	/* Information Element */
#define WME_SUBTYPE_PARAM_IE	1	/* Parameter Element */
#define WME_SUBTYPE_TSPEC	2	/* Traffic Specification */
#define WME_VER			1	/* WME version */

/* WME Access Category Indices (ACIs) */
enum wme_ac {
	AC_BE = 0,	/* Best Effort */
	AC_BK = 1,	/* Background */
	AC_VI = 2,	/* Video */
	AC_VO = 3,	/* Voice */
	AC_COUNT	/* number of ACs */
};

typedef uint8 ac_bitmap_t;	/* AC bitmap of (1 << AC_xx) */

#define AC_BITMAP_NONE		0x0	/* No ACs */
#define AC_BITMAP_ALL		0xf	/* All ACs */
#define AC_BITMAP_TST(ab, ac)	(((ab) & (1 << (ac))) != 0)
#define AC_BITMAP_SET(ab, ac)	(((ab) |= (1 << (ac))))
#define AC_BITMAP_RESET(ab, ac) (((ab) &= ~(1 << (ac))))

/* Management PKT Lifetime indices */
/* Removing flag checks 'BCMINTERNAL || WLTEST'
 * while merging MERGE BIS120RC4 to DINGO2
 */
#define MGMT_ALL		0xffff
#define MGMT_AUTH_LT	FC_SUBTYPE_AUTH
#define MGMT_ASSOC_LT	FC_SUBTYPE_ASSOC_REQ

/** WME Information Element (IE) */
BWL_PRE_PACKED_STRUCT struct wme_ie {
	uint8 oui[3];
	uint8 type;
	uint8 subtype;
	uint8 version;
	uint8 qosinfo;
} BWL_POST_PACKED_STRUCT;
typedef struct wme_ie wme_ie_t;
#define WME_IE_LEN 7	/* WME IE length */

BWL_PRE_PACKED_STRUCT struct edcf_acparam {
	uint8	ACI;
	uint8	ECW;
	uint16  TXOP;		/* stored in network order (ls octet first) */
} BWL_POST_PACKED_STRUCT;
typedef struct edcf_acparam edcf_acparam_t;

/** WME Parameter Element (PE) */
BWL_PRE_PACKED_STRUCT struct wme_param_ie {
	uint8 oui[3];
	uint8 type;
	uint8 subtype;
	uint8 version;
	uint8 qosinfo;
	uint8 rsvd;
	edcf_acparam_t acparam[AC_COUNT];
} BWL_POST_PACKED_STRUCT;
typedef struct wme_param_ie wme_param_ie_t;
#define WME_PARAM_IE_LEN            24          /* WME Parameter IE length */

/* QoS Info field for IE as sent from AP */
#define WME_QI_AP_APSD_MASK         0x80        /* U-APSD Supported mask */
#define WME_QI_AP_APSD_SHIFT        7           /* U-APSD Supported shift */
#define WME_QI_AP_COUNT_MASK        0x0f        /* Parameter set count mask */
#define WME_QI_AP_COUNT_SHIFT       0           /* Parameter set count shift */

/* QoS Info field for IE as sent from STA */
#define WME_QI_STA_MAXSPLEN_MASK    0x60        /* Max Service Period Length mask */
#define WME_QI_STA_MAXSPLEN_SHIFT   5           /* Max Service Period Length shift */
#define WME_QI_STA_APSD_ALL_MASK    0xf         /* APSD all AC bits mask */
#define WME_QI_STA_APSD_ALL_SHIFT   0           /* APSD all AC bits shift */
#define WME_QI_STA_APSD_BE_MASK     0x8         /* APSD AC_BE mask */
#define WME_QI_STA_APSD_BE_SHIFT    3           /* APSD AC_BE shift */
#define WME_QI_STA_APSD_BK_MASK     0x4         /* APSD AC_BK mask */
#define WME_QI_STA_APSD_BK_SHIFT    2           /* APSD AC_BK shift */
#define WME_QI_STA_APSD_VI_MASK     0x2         /* APSD AC_VI mask */
#define WME_QI_STA_APSD_VI_SHIFT    1           /* APSD AC_VI shift */
#define WME_QI_STA_APSD_VO_MASK     0x1         /* APSD AC_VO mask */
#define WME_QI_STA_APSD_VO_SHIFT    0           /* APSD AC_VO shift */

/* ACI */
#define EDCF_AIFSN_MIN               1           /* AIFSN minimum value */
#define EDCF_AIFSN_MAX               15          /* AIFSN maximum value */
#define EDCF_AIFSN_MASK              0x0f        /* AIFSN mask */
#define EDCF_ACM_MASK                0x10        /* ACM mask */
#define EDCF_ACI_MASK                0x60        /* ACI mask */
#define EDCF_ACI_SHIFT               5           /* ACI shift */
#define EDCF_AIFSN_SHIFT             12          /* 4 MSB(0xFFF) in ifs_ctl for AC idx */

/* ECW */
#define EDCF_ECW_MIN                 0           /* cwmin/cwmax exponent minimum value */
#define EDCF_ECW_MAX                 15          /* cwmin/cwmax exponent maximum value */
#define EDCF_ECW2CW(exp)             ((1 << (exp)) - 1)
#define EDCF_ECWMIN_MASK             0x0f        /* cwmin exponent form mask */
#define EDCF_ECWMAX_MASK             0xf0        /* cwmax exponent form mask */
#define EDCF_ECWMAX_SHIFT            4           /* cwmax exponent form shift */

/* TXOP */
#define EDCF_TXOP_MIN                0           /* TXOP minimum value */
#define EDCF_TXOP_MAX                65535       /* TXOP maximum value */
#define EDCF_TXOP2USEC(txop)         ((txop) << 5)

/* Default BE ACI value for non-WME connection STA */
#define NON_EDCF_AC_BE_ACI_STA          0x02

/* Default EDCF parameters that AP advertises for STA to use; WMM draft Table 12 */
#define EDCF_AC_BE_ACI_STA           0x03	/* STA ACI value for best effort AC */
#define EDCF_AC_BE_ECW_STA           0xA4	/* STA ECW value for best effort AC */
#define EDCF_AC_BE_TXOP_STA          0x0000	/* STA TXOP value for best effort AC */
#define EDCF_AC_BK_ACI_STA           0x27	/* STA ACI value for background AC */
#define EDCF_AC_BK_ECW_STA           0xA4	/* STA ECW value for background AC */
#define EDCF_AC_BK_TXOP_STA          0x0000	/* STA TXOP value for background AC */
#define EDCF_AC_VI_ACI_STA           0x42	/* STA ACI value for video AC */
#define EDCF_AC_VI_ECW_STA           0x43	/* STA ECW value for video AC */
#define EDCF_AC_VI_TXOP_STA          0x005e	/* STA TXOP value for video AC */
#define EDCF_AC_VO_ACI_STA           0x62	/* STA ACI value for audio AC */
#define EDCF_AC_VO_ECW_STA           0x32	/* STA ECW value for audio AC */
#define EDCF_AC_VO_TXOP_STA          0x002f	/* STA TXOP value for audio AC */

/* Default EDCF parameters that AP uses; WMM draft Table 14 */
#define EDCF_AC_BE_ACI_AP            0x03	/* AP ACI value for best effort AC */
#define EDCF_AC_BE_ECW_AP            0x64	/* AP ECW value for best effort AC */
#define EDCF_AC_BE_TXOP_AP           0x0000	/* AP TXOP value for best effort AC */
#define EDCF_AC_BK_ACI_AP            0x27	/* AP ACI value for background AC */
#define EDCF_AC_BK_ECW_AP            0xA4	/* AP ECW value for background AC */
#define EDCF_AC_BK_TXOP_AP           0x0000	/* AP TXOP value for background AC */
#define EDCF_AC_VI_ACI_AP            0x41	/* AP ACI value for video AC */
#define EDCF_AC_VI_ECW_AP            0x43	/* AP ECW value for video AC */
#define EDCF_AC_VI_TXOP_AP           0x005e	/* AP TXOP value for video AC */
#define EDCF_AC_VO_ACI_AP            0x61	/* AP ACI value for audio AC */
#define EDCF_AC_VO_ECW_AP            0x32	/* AP ECW value for audio AC */
#define EDCF_AC_VO_TXOP_AP           0x002f	/* AP TXOP value for audio AC */

/** EDCA Parameter IE */
BWL_PRE_PACKED_STRUCT struct edca_param_ie {
	uint8 qosinfo;
	uint8 rsvd;
	edcf_acparam_t acparam[AC_COUNT];
} BWL_POST_PACKED_STRUCT;
typedef struct edca_param_ie edca_param_ie_t;
#define EDCA_PARAM_IE_LEN            18          /* EDCA Parameter IE length */

/** QoS Capability IE */
BWL_PRE_PACKED_STRUCT struct qos_cap_ie {
	uint8 qosinfo;
} BWL_POST_PACKED_STRUCT;
typedef struct qos_cap_ie qos_cap_ie_t;

BWL_PRE_PACKED_STRUCT struct dot11_qbss_load_ie {
	uint8 id;			/* 11, DOT11_MNG_QBSS_LOAD_ID */
	uint8 length;
	uint16 station_count;		/* total number of STAs associated */
	uint8 channel_utilization;	/* % of time, normalized to 255, QAP sensed medium busy */
	uint16 aac;			/* available admission capacity */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_qbss_load_ie dot11_qbss_load_ie_t;
#define BSS_LOAD_IE_SIZE	7	/* BSS load IE size */

#define WLC_QBSS_LOAD_CHAN_FREE_MAX	0xff	/* max for channel free score */
#define WLC_QBSS_CHAN_FREE_DEFAULT	0x7f	/* default channel free score for unsupported AP */

/* Estimated Service Parameters (ESP) IE - 802.11-2016 9.4.2.174 */
typedef BWL_PRE_PACKED_STRUCT struct dot11_esp_ie {
	uint8		id;
	uint8		length;
	uint8		id_ext;
	/* variable len info */
	uint8		esp_info_lists[];
} BWL_POST_PACKED_STRUCT dot11_esp_ie_t;

#define DOT11_ESP_IE_HDR_SIZE	(OFFSETOF(dot11_esp_ie_t, esp_info_lists))

/* ESP Information list - 802.11-2016 9.4.2.174 */
typedef BWL_PRE_PACKED_STRUCT struct dot11_esp_ie_info_list {
	/* acess category, data format, ba win size */
	uint8		ac_df_baws;
	/* estimated air time fraction */
	uint8		eat_frac;
	/* data PPDU duration target (50us units) */
	uint8		ppdu_dur;
} BWL_POST_PACKED_STRUCT dot11_esp_ie_info_list_t;

#define DOT11_ESP_IE_INFO_LIST_SIZE	(sizeof(dot11_esp_ie_info_list_t))
/* Macros for ESP */
#define DATA_FORMAT_SHIFT	3
#define BA_WSIZE_SHIFT		5

#define ESP_BE			1
#define ESP_AMSDU_ENABLED	(1 << DATA_FORMAT_SHIFT)
#define ESP_AMPDU_ENABLED	(2 << DATA_FORMAT_SHIFT)
#define ESP_BA_WSIZE_NONE	(0 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_2		(1 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_4		(2 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_6		(3 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_8		(4 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_16		(5 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_32		(6 << BA_WSIZE_SHIFT)
#define ESP_BA_WSIZE_64		(7 << BA_WSIZE_SHIFT)

#define DOT11_ESP_NBR_INFO_LISTS	4u	/* max nbr of esp information lists */
#define DOT11_ESP_INFO_LIST_AC_BK	0u	/* access category of esp information list AC_BK */
#define DOT11_ESP_INFO_LIST_AC_BE	1u	/* access category of esp information list AC_BE */
#define DOT11_ESP_INFO_LIST_AC_VI	2u	/* access category of esp information list AC_VI */
#define DOT11_ESP_INFO_LIST_AC_VO	3u	/* access category of esp information list AC_VO */

/* nom_msdu_size */
#define FIXED_MSDU_SIZE 0x8000		/* MSDU size is fixed */
#define MSDU_SIZE_MASK	0x7fff		/* (Nominal or fixed) MSDU size */

/* surplus_bandwidth */
/* Represented as 3 bits of integer, binary point, 13 bits fraction */
#define	INTEGER_SHIFT	13	/* integer shift */
#define FRACTION_MASK	0x1FFF	/* fraction mask */

/** Management Notification Frame */
BWL_PRE_PACKED_STRUCT struct dot11_management_notification {
	uint8 category;			/* DOT11_ACTION_NOTIFICATION */
	uint8 action;
	uint8 token;
	uint8 status;
	uint8 data[1];			/* Elements */
} BWL_POST_PACKED_STRUCT;
#define DOT11_MGMT_NOTIFICATION_LEN 4	/* Fixed length */

/** Timeout Interval IE */
BWL_PRE_PACKED_STRUCT struct ti_ie {
	uint8 ti_type;
	uint32 ti_val;
} BWL_POST_PACKED_STRUCT;
typedef struct ti_ie ti_ie_t;
#define TI_TYPE_REASSOC_DEADLINE	1
#define TI_TYPE_KEY_LIFETIME		2

#ifndef CISCO_AIRONET_OUI
#define CISCO_AIRONET_OUI	"\x00\x40\x96"	/* Cisco AIRONET OUI */
#endif // endif
/* QoS FastLane IE. */
BWL_PRE_PACKED_STRUCT struct ccx_qfl_ie {
	uint8	id;		/* 221, DOT11_MNG_VS_ID */
	uint8	length;		/* 5 */
	uint8	oui[3];		/* 00:40:96 */
	uint8	type;		/* 11 */
	uint8	data;
} BWL_POST_PACKED_STRUCT;
typedef struct ccx_qfl_ie ccx_qfl_ie_t;
#define CCX_QFL_IE_TYPE	11
#define CCX_QFL_ENABLE_SHIFT	5
#define CCX_QFL_ENALBE (1 << CCX_QFL_ENABLE_SHIFT)

/* WME Action Codes */
#define WME_ADDTS_REQUEST	0	/* WME ADDTS request */
#define WME_ADDTS_RESPONSE	1	/* WME ADDTS response */
#define WME_DELTS_REQUEST	2	/* WME DELTS request */

/* WME Setup Response Status Codes */
#define WME_ADMISSION_ACCEPTED		0	/* WME admission accepted */
#define WME_INVALID_PARAMETERS		1	/* WME invalide parameters */
#define WME_ADMISSION_REFUSED		3	/* WME admission refused */

/* Macro to take a pointer to a beacon or probe response
 * body and return the char* pointer to the SSID info element
 */
#define BCN_PRB_SSID(body) ((char*)(body) + DOT11_BCN_PRB_LEN)

/* Authentication frame payload constants */
#define DOT11_OPEN_SYSTEM	0	/* d11 open authentication */
#define DOT11_SHARED_KEY	1	/* d11 shared authentication */
#define DOT11_FAST_BSS		2	/* d11 fast bss authentication */
#define DOT11_SAE		3	/* d11 simultaneous authentication of equals */
#define DOT11_FILS_SKEY		4	/* d11 fils shared key authentication w/o pfs */
#define DOT11_FILS_SKEY_PFS	5	/* d11 fils shared key authentication w/ pfs */
#define DOT11_FILS_PKEY		6	/* d11 fils public key authentication */
#define DOT11_AUTH_PASN		7	/* d11 Pre Security association negotiation */
#define DOT11_CHALLENGE_LEN	128	/* d11 challenge text length */

/* Frame control macros */
#define FC_PVER_MASK		0x3	/* PVER mask */
#define FC_PVER_SHIFT		0	/* PVER shift */
#define FC_TYPE_MASK		0xC	/* type mask */
#define FC_TYPE_SHIFT		2	/* type shift */
#define FC_SUBTYPE_MASK		0xF0	/* subtype mask */
#define FC_SUBTYPE_SHIFT	4	/* subtype shift */
#define FC_TODS			0x100	/* to DS */
#define FC_TODS_SHIFT		8	/* to DS shift */
#define FC_FROMDS		0x200	/* from DS */
#define FC_FROMDS_SHIFT		9	/* from DS shift */
#define FC_MOREFRAG		0x400	/* more frag. */
#define FC_MOREFRAG_SHIFT	10	/* more frag. shift */
#define FC_RETRY		0x800	/* retry */
#define FC_RETRY_SHIFT		11	/* retry shift */
#define FC_PM			0x1000	/* PM */
#define FC_PM_SHIFT		12	/* PM shift */
#define FC_MOREDATA		0x2000	/* more data */
#define FC_MOREDATA_SHIFT	13	/* more data shift */
#define FC_WEP			0x4000	/* WEP */
#define FC_WEP_SHIFT		14	/* WEP shift */
#define FC_ORDER		0x8000	/* order */
#define FC_ORDER_SHIFT		15	/* order shift */

/* sequence control macros */
#define SEQNUM_SHIFT		4	/* seq. number shift */
#define SEQNUM_MAX		0x1000	/* max seqnum + 1 */
#define FRAGNUM_MASK		0xF	/* frag. number mask */

/* Frame Control type/subtype defs */

/* FC Types */
#define FC_TYPE_MNG		0	/* management type */
#define FC_TYPE_CTL		1	/* control type */
#define FC_TYPE_DATA		2	/* data type */

/* Management Subtypes */
#define FC_SUBTYPE_ASSOC_REQ		0	/* assoc. request */
#define FC_SUBTYPE_ASSOC_RESP		1	/* assoc. response */
#define FC_SUBTYPE_REASSOC_REQ		2	/* reassoc. request */
#define FC_SUBTYPE_REASSOC_RESP		3	/* reassoc. response */
#define FC_SUBTYPE_PROBE_REQ		4	/* probe request */
#define FC_SUBTYPE_PROBE_RESP		5	/* probe response */
#define FC_SUBTYPE_BEACON		8	/* beacon */
#define FC_SUBTYPE_ATIM			9	/* ATIM */
#define FC_SUBTYPE_DISASSOC		10	/* disassoc. */
#define FC_SUBTYPE_AUTH			11	/* authentication */
#define FC_SUBTYPE_DEAUTH		12	/* de-authentication */
#define FC_SUBTYPE_ACTION		13	/* action */
#define FC_SUBTYPE_ACTION_NOACK		14	/* action no-ack */

/* Control Subtypes */
#define FC_SUBTYPE_TRIGGER		2	/* Trigger frame */
#define FC_SUBTYPE_NDPA                 5	/* NDPA  */
#define FC_SUBTYPE_CTL_WRAPPER		7	/* Control Wrapper */
#define FC_SUBTYPE_BLOCKACK_REQ		8	/* Block Ack Req */
#define FC_SUBTYPE_BLOCKACK		9	/* Block Ack */
#define FC_SUBTYPE_PS_POLL		10	/* PS poll */
#define FC_SUBTYPE_RTS			11	/* RTS */
#define FC_SUBTYPE_CTS			12	/* CTS */
#define FC_SUBTYPE_ACK			13	/* ACK */
#define FC_SUBTYPE_CF_END		14	/* CF-END */
#define FC_SUBTYPE_CF_END_ACK		15	/* CF-END ACK */

/* Data Subtypes */
#define FC_SUBTYPE_DATA			0	/* Data */
#define FC_SUBTYPE_DATA_CF_ACK		1	/* Data + CF-ACK */
#define FC_SUBTYPE_DATA_CF_POLL		2	/* Data + CF-Poll */
#define FC_SUBTYPE_DATA_CF_ACK_POLL	3	/* Data + CF-Ack + CF-Poll */
#define FC_SUBTYPE_NULL			4	/* Null */
#define FC_SUBTYPE_CF_ACK		5	/* CF-Ack */
#define FC_SUBTYPE_CF_POLL		6	/* CF-Poll */
#define FC_SUBTYPE_CF_ACK_POLL		7	/* CF-Ack + CF-Poll */
#define FC_SUBTYPE_QOS_DATA		8	/* QoS Data */
#define FC_SUBTYPE_QOS_DATA_CF_ACK	9	/* QoS Data + CF-Ack */
#define FC_SUBTYPE_QOS_DATA_CF_POLL	10	/* QoS Data + CF-Poll */
#define FC_SUBTYPE_QOS_DATA_CF_ACK_POLL	11	/* QoS Data + CF-Ack + CF-Poll */
#define FC_SUBTYPE_QOS_NULL		12	/* QoS Null */
#define FC_SUBTYPE_QOS_CF_POLL		14	/* QoS CF-Poll */
#define FC_SUBTYPE_QOS_CF_ACK_POLL	15	/* QoS CF-Ack + CF-Poll */

/* Data Subtype Groups */
#define FC_SUBTYPE_ANY_QOS(s)		(((s) & 8) != 0)
#define FC_SUBTYPE_ANY_NULL(s)		(((s) & 4) != 0)
#define FC_SUBTYPE_ANY_CF_POLL(s)	(((s) & 2) != 0)
#define FC_SUBTYPE_ANY_CF_ACK(s)	(((s) & 1) != 0)
#define FC_SUBTYPE_ANY_PSPOLL(s)	(((s) & 10) != 0)
#define FC_SUBTYPE_ANY_DATA(s)		(!FC_SUBTYPE_ANY_NULL(s))

/* Type/Subtype Combos */
#define FC_KIND_MASK		(FC_TYPE_MASK | FC_SUBTYPE_MASK)	/* FC kind mask */

#define FC_KIND(t, s)	(((t) << FC_TYPE_SHIFT) | ((s) << FC_SUBTYPE_SHIFT))	/* FC kind */

#define FC_SUBTYPE(fc)	(((fc) & FC_SUBTYPE_MASK) >> FC_SUBTYPE_SHIFT)	/* Subtype from FC */
#define FC_TYPE(fc)	(((fc) & FC_TYPE_MASK) >> FC_TYPE_SHIFT)	/* Type from FC */

#define FC_ASSOC_REQ	FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_ASSOC_REQ)	/* assoc. request */
#define FC_ASSOC_RESP	FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_ASSOC_RESP)	/* assoc. response */
#define FC_REASSOC_REQ	FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_REASSOC_REQ)	/* reassoc. request */
#define FC_REASSOC_RESP	FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_REASSOC_RESP)	/* reassoc. response */
#define FC_PROBE_REQ	FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_PROBE_REQ)	/* probe request */
#define FC_PROBE_RESP	FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_PROBE_RESP)	/* probe response */
#define FC_BEACON	FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_BEACON)		/* beacon */
#define FC_ATIM		FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_ATIM)		/* ATIM */
#define FC_DISASSOC	FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_DISASSOC)	/* disassoc */
#define FC_AUTH		FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_AUTH)		/* authentication */
#define FC_DEAUTH	FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_DEAUTH)		/* deauthentication */
#define FC_ACTION	FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_ACTION)		/* action */
#define FC_ACTION_NOACK	FC_KIND(FC_TYPE_MNG, FC_SUBTYPE_ACTION_NOACK)	/* action no-ack */

#define FC_CTL_TRIGGER	FC_KIND(FC_TYPE_CTL, FC_SUBTYPE_TRIGGER)	/* Trigger frame */
#define FC_CTL_NDPA	FC_KIND(FC_TYPE_CTL, FC_SUBTYPE_NDPA)	/* NDPA frame */
#define FC_CTL_WRAPPER	FC_KIND(FC_TYPE_CTL, FC_SUBTYPE_CTL_WRAPPER)	/* Control Wrapper */
#define FC_BLOCKACK_REQ	FC_KIND(FC_TYPE_CTL, FC_SUBTYPE_BLOCKACK_REQ)	/* Block Ack Req */
#define FC_BLOCKACK	FC_KIND(FC_TYPE_CTL, FC_SUBTYPE_BLOCKACK)	/* Block Ack */
#define FC_PS_POLL	FC_KIND(FC_TYPE_CTL, FC_SUBTYPE_PS_POLL)	/* PS poll */
#define FC_RTS		FC_KIND(FC_TYPE_CTL, FC_SUBTYPE_RTS)		/* RTS */
#define FC_CTS		FC_KIND(FC_TYPE_CTL, FC_SUBTYPE_CTS)		/* CTS */
#define FC_ACK		FC_KIND(FC_TYPE_CTL, FC_SUBTYPE_ACK)		/* ACK */
#define FC_CF_END	FC_KIND(FC_TYPE_CTL, FC_SUBTYPE_CF_END)		/* CF-END */
#define FC_CF_END_ACK	FC_KIND(FC_TYPE_CTL, FC_SUBTYPE_CF_END_ACK)	/* CF-END ACK */

#define FC_DATA		FC_KIND(FC_TYPE_DATA, FC_SUBTYPE_DATA)		/* data */
#define FC_NULL_DATA	FC_KIND(FC_TYPE_DATA, FC_SUBTYPE_NULL)		/* null data */
#define FC_DATA_CF_ACK	FC_KIND(FC_TYPE_DATA, FC_SUBTYPE_DATA_CF_ACK)	/* data CF ACK */
#define FC_QOS_DATA	FC_KIND(FC_TYPE_DATA, FC_SUBTYPE_QOS_DATA)	/* QoS data */
#define FC_QOS_NULL	FC_KIND(FC_TYPE_DATA, FC_SUBTYPE_QOS_NULL)	/* QoS null */

/* QoS Control Field */

/* 802.1D Priority */
#define QOS_PRIO_SHIFT		0	/* QoS priority shift */
#define QOS_PRIO_MASK		0x0007	/* QoS priority mask */
#define QOS_PRIO(qos)		(((qos) & QOS_PRIO_MASK) >> QOS_PRIO_SHIFT)	/* QoS priority */

/* Traffic Identifier */
#define QOS_TID_SHIFT		0	/* QoS TID shift */
#define QOS_TID_MASK		0x000f	/* QoS TID mask */
#define QOS_TID(qos)		(((qos) & QOS_TID_MASK) >> QOS_TID_SHIFT)	/* QoS TID */

/* End of Service Period (U-APSD) */
#define QOS_EOSP_SHIFT		4	/* QoS End of Service Period shift */
#define QOS_EOSP_MASK		0x0010	/* QoS End of Service Period mask */
#define QOS_EOSP(qos)		(((qos) & QOS_EOSP_MASK) >> QOS_EOSP_SHIFT)	/* Qos EOSP */

/* Ack Policy */
#define QOS_ACK_NORMAL_ACK	0	/* Normal Ack */
#define QOS_ACK_NO_ACK		1	/* No Ack (eg mcast) */
#define QOS_ACK_NO_EXP_ACK	2	/* No Explicit Ack */
#define QOS_ACK_BLOCK_ACK	3	/* Block Ack */
#define QOS_ACK_SHIFT		5	/* QoS ACK shift */
#define QOS_ACK_MASK		0x0060	/* QoS ACK mask */
#define QOS_ACK(qos)		(((qos) & QOS_ACK_MASK) >> QOS_ACK_SHIFT)	/* QoS ACK */

/* A-MSDU flag */
#define QOS_AMSDU_SHIFT		7	/* AMSDU shift */
#define QOS_AMSDU_MASK		0x0080	/* AMSDU mask */

/* Queue Size */
#define QOS_QS_PRESENT_SHIFT	4	/* queue size present bit */
#define QOS_QS_PRESENT_MASK	0x0010
#define QOS_QS_INFO_SHIFT	8	/* queue size subfield */
#define QOS_QS_INFO_MASK	0xff00
#define QOS_QS_UV_SHIFT		8	/* queue size unscaled value */
#define QOS_QS_UV_MASK		0x3f00
#define QOS_QS_SF_SHIFT		14	/* queue size scaled factor */
#define QOS_QS_SF_MASK		0xc000
#define QOS_QS_SF(x)		(((x) & QOS_QS_SF_MASK) >> QOS_QS_SF_SHIFT)
#define QOS_QS_UV(x)		(((x) & QOS_QS_UV_MASK) >> QOS_QS_UV_SHIFT)

/* Management Frames */

/* Management Frame Constants */

/* Fixed fields */
#define DOT11_MNG_AUTH_ALGO_LEN		2	/* d11 management auth. algo. length */
#define DOT11_MNG_AUTH_SEQ_LEN		2	/* d11 management auth. seq. length */
#define DOT11_MNG_BEACON_INT_LEN	2	/* d11 management beacon interval length */
#define DOT11_MNG_CAP_LEN		2	/* d11 management cap. length */
#define DOT11_MNG_AP_ADDR_LEN		6	/* d11 management AP address length */
#define DOT11_MNG_LISTEN_INT_LEN	2	/* d11 management listen interval length */
#define DOT11_MNG_REASON_LEN		2	/* d11 management reason length */
#define DOT11_MNG_AID_LEN		2	/* d11 management AID length */
#define DOT11_MNG_STATUS_LEN		2	/* d11 management status length */
#define DOT11_MNG_TIMESTAMP_LEN		8	/* d11 management timestamp length */

/* DUR/ID field in assoc resp is 0xc000 | AID */
#define DOT11_AID_MASK			0x3fff	/* d11 AID mask */

/* Reason Codes */
#define DOT11_RC_RESERVED		0	/* d11 RC reserved */
#define DOT11_RC_UNSPECIFIED		1	/* Unspecified reason */
#define DOT11_RC_AUTH_INVAL		2	/* Previous authentication no longer valid */
#define DOT11_RC_DEAUTH_LEAVING		3	/* Deauthenticated because sending station
						 * is leaving (or has left) IBSS or ESS
						 */
#define DOT11_RC_INACTIVITY		4	/* Disassociated due to inactivity */
#define DOT11_RC_BUSY			5	/* Disassociated because AP is unable to handle
						 * all currently associated stations
						 */
#define DOT11_RC_INVAL_CLASS_2		6	/* Class 2 frame received from
						 * nonauthenticated station
						 */
#define DOT11_RC_INVAL_CLASS_3		7	/* Class 3 frame received from
						 *  nonassociated station
						 */
#define DOT11_RC_DISASSOC_LEAVING	8	/* Disassociated because sending station is
						 * leaving (or has left) BSS
						 */
#define DOT11_RC_NOT_AUTH		9	/* Station requesting (re)association is not
						 * authenticated with responding station
						 */
#define DOT11_RC_BAD_PC			10	/* Unacceptable power capability element */
#define DOT11_RC_BAD_CHANNELS		11	/* Unacceptable supported channels element */

/* 12 is unused by STA but could be used by AP/GO */
#define DOT11_RC_DISASSOC_BTM		12	/* Disassociated due to BSS Transition Magmt */

/* 32-39 are QSTA specific reasons added in 11e */
#define DOT11_RC_UNSPECIFIED_QOS	32	/* unspecified QoS-related reason */
#define DOT11_RC_INSUFFCIENT_BW		33	/* QAP lacks sufficient bandwidth */
#define DOT11_RC_EXCESSIVE_FRAMES	34	/* excessive number of frames need ack */
#define DOT11_RC_TX_OUTSIDE_TXOP	35	/* transmitting outside the limits of txop */
#define DOT11_RC_LEAVING_QBSS		36	/* QSTA is leaving the QBSS (or restting) */
#define DOT11_RC_BAD_MECHANISM		37	/* does not want to use the mechanism */
#define DOT11_RC_SETUP_NEEDED		38	/* mechanism needs a setup */
#define DOT11_RC_TIMEOUT		39	/* timeout */

#define DOT11_RC_MESH_PEERING_CANCELLED		52
#define DOT11_RC_MESH_MAX_PEERS			53
#define DOT11_RC_MESH_CONFIG_POLICY_VIOLN	54
#define DOT11_RC_MESH_CLOSE_RECVD		55
#define DOT11_RC_MESH_MAX_RETRIES		56
#define DOT11_RC_MESH_CONFIRM_TIMEOUT		57
#define DOT11_RC_MESH_INVALID_GTK		58
#define DOT11_RC_MESH_INCONSISTENT_PARAMS	59

#define DOT11_RC_MESH_INVALID_SEC_CAP		60
#define DOT11_RC_MESH_PATHERR_NOPROXYINFO	61
#define DOT11_RC_MESH_PATHERR_NOFWINFO		62
#define DOT11_RC_MESH_PATHERR_DSTUNREACH	63
#define DOT11_RC_MESH_MBSSMAC_EXISTS		64
#define DOT11_RC_MESH_CHANSWITCH_REGREQ		65
#define DOT11_RC_MESH_CHANSWITCH_UNSPEC		66

#define DOT11_RC_STALE_DETECTION		254 /* Internal usage, not to be send in air
						 * when multiple assoc are detected by middleware,
						 * driver is informed to clean scb up
						 */

#define DOT11_RC_MAX			66	/* Reason codes > 66 are reserved */

#define DOT11_RC_TDLS_PEER_UNREACH	25
#define DOT11_RC_TDLS_DOWN_UNSPECIFIED	26

/* Status Codes */
#define DOT11_SC_SUCCESS		0	/* Successful */
#define DOT11_SC_FAILURE		1	/* Unspecified failure */
#define DOT11_SC_TDLS_WAKEUP_SCH_ALT 2	/* TDLS wakeup schedule rejected but alternative  */
					/* schedule provided */
#define DOT11_SC_TDLS_WAKEUP_SCH_REJ 3	/* TDLS wakeup schedule rejected */
#define DOT11_SC_TDLS_SEC_DISABLED	5	/* TDLS Security disabled */
#define DOT11_SC_LIFETIME_REJ		6	/* Unacceptable lifetime */
#define DOT11_SC_NOT_SAME_BSS		7	/* Not in same BSS */
#define DOT11_SC_CAP_MISMATCH		10	/* Cannot support all requested
						 * capabilities in the Capability
						 * Information field
						 */
#define DOT11_SC_REASSOC_FAIL		11	/* Reassociation denied due to inability
						 * to confirm that association exists
						 */
#define DOT11_SC_ASSOC_FAIL		12	/* Association denied due to reason
						 * outside the scope of this standard
						 */
#define DOT11_SC_AUTH_MISMATCH		13	/* Responding station does not support
						 * the specified authentication
						 * algorithm
						 */
#define DOT11_SC_AUTH_SEQ		14	/* Received an Authentication frame
						 * with authentication transaction
						 * sequence number out of expected
						 * sequence
						 */
#define DOT11_SC_AUTH_CHALLENGE_FAIL	15	/* Authentication rejected because of
						 * challenge failure
						 */
#define DOT11_SC_AUTH_TIMEOUT		16	/* Authentication rejected due to timeout
						 * waiting for next frame in sequence
						 */
#define DOT11_SC_ASSOC_BUSY_FAIL	17	/* Association denied because AP is
						 * unable to handle additional
						 * associated stations
						 */
#define DOT11_SC_ASSOC_RATE_MISMATCH	18	/* Association denied due to requesting
						 * station not supporting all of the
						 * data rates in the BSSBasicRateSet
						 * parameter
						 */
#define DOT11_SC_ASSOC_SHORT_REQUIRED	19	/* Association denied due to requesting
						 * station not supporting the Short
						 * Preamble option
						 */
#define DOT11_SC_ASSOC_PBCC_REQUIRED	20	/* Association denied due to requesting
						 * station not supporting the PBCC
						 * Modulation option
						 */
#define DOT11_SC_ASSOC_AGILITY_REQUIRED	21	/* Association denied due to requesting
						 * station not supporting the Channel
						 * Agility option
						 */
#define DOT11_SC_ASSOC_SPECTRUM_REQUIRED	22	/* Association denied because Spectrum
							 * Management capability is required.
							 */
#define DOT11_SC_ASSOC_BAD_POWER_CAP	23	/* Association denied because the info
						 * in the Power Cap element is
						 * unacceptable.
						 */
#define DOT11_SC_ASSOC_BAD_SUP_CHANNELS	24	/* Association denied because the info
						 * in the Supported Channel element is
						 * unacceptable
						 */
#define DOT11_SC_ASSOC_SHORTSLOT_REQUIRED	25	/* Association denied due to requesting
							 * station not supporting the Short Slot
							 * Time option
							 */
#define DOT11_SC_ASSOC_DSSSOFDM_REQUIRED 26	/* Association denied because requesting station
						 * does not support the DSSS-OFDM option
						 */
#define DOT11_SC_ASSOC_HT_REQUIRED	27	/* Association denied because the requesting
						 * station does not support HT features
						 */
#define DOT11_SC_ASSOC_R0KH_UNREACHABLE	28	/* Association denied due to AP
						 * being unable to reach the R0 Key Holder
						 */
#define DOT11_SC_ASSOC_TRY_LATER	30	/* Association denied temporarily, try again later
						 */
#define DOT11_SC_ASSOC_MFP_VIOLATION	31	/* Association denied due to Robust Management
						 * frame policy violation
						 */

#define DOT11_SC_INSUFFICIENT_BANDWIDTH	33	/* Insufficient Bandwidth */
#define DOT11_SC_POOR_CHAN_CONDITION	34	/* Bad channel condition */
#define	DOT11_SC_DECLINED		37	/* request declined */
#define	DOT11_SC_INVALID_PARAMS		38	/* One or more params have invalid values */
#define DOT11_SC_REJECT_WITH_SUGGEST_CHANGES	39 /* Rejected with suggested changes */
#define DOT11_SC_INVALID_ELEMENT	40	/* Invalid element */
#define DOT11_SC_INVALID_GROUP_CIPHER	41	/* Invalid group cipher */
#define DOT11_SC_INVALID_PAIRWISE_CIPHER	42 /* Invalid pairwise cipher */
#define	DOT11_SC_INVALID_AKMP		43	/* Association denied due to invalid AKMP */
#define DOT11_SC_UNSUPPORTED_RSNE_VER	44	/* Unsupported RSNE version */
#define DOT11_SC_INVALID_RSNIE_CAP	45	/* Invalid RSN IE capabilities */
#define DOT11_SC_CIPHER_OUT_OF_POLICY	46	/* Cipher suite rejected because of
						 * security policy
						 */
#define DOT11_SC_DLS_NOT_ALLOWED	48	/* DLS is not allowed in the BSS by policy */
#define	DOT11_SC_INVALID_PMKID		53	/* Association denied due to invalid PMKID */
#define	DOT11_SC_INVALID_MDID		54	/* Association denied due to invalid MDID */
#define	DOT11_SC_INVALID_FTIE		55	/* Association denied due to invalid FTIE */

/* Requested TCLAS processing is not supported by the AP or PCP. */
#define	DOT11_SC_REQUESTED_TCLAS_NOT_SUPPORTED_BY_AP		56u

/* The AP or PCP has insufficient TCLAS processing resources to satisfy the request. */
#define DOT11_SC_INSUFFICIENT_TCLAS_PROCESSING_RESOURCES	57u

#define DOT11_SC_ADV_PROTO_NOT_SUPPORTED	59	/* ad proto not supported */
#define DOT11_SC_NO_OUTSTAND_REQ			60	/* no outstanding req */
#define DOT11_SC_RSP_NOT_RX_FROM_SERVER		61	/* no response from server */
#define DOT11_SC_TIMEOUT					62	/* timeout */
#define DOT11_SC_QUERY_RSP_TOO_LARGE		63	/* query rsp too large */
#define DOT11_SC_SERVER_UNREACHABLE			65	/* server unreachable */

#define DOT11_SC_UNEXP_MSG			70	/* Unexpected message */
#define DOT11_SC_INVALID_SNONCE		71	/* Invalid SNonce */
#define DOT11_SC_INVALID_RSNIE		72	/* Invalid contents of RSNIE */

#define DOT11_SC_ANTICLOG_TOCKEN_REQUIRED	76	/* Anti-clogging tocken required */
#define DOT11_SC_INVALID_FINITE_CYCLIC_GRP	77	/* Invalid contents of RSNIE */

#define DOT11_SC_ASSOC_VHT_REQUIRED	104	/* Association denied because the requesting
						 * station does not support VHT features.
						 */

#define DOT11_SC_TRANSMIT_FAILURE	79	/* transmission failure */

#define DOT11_SC_REQUESTED_TCLAS_NOT_SUPPORTED	80u	/* Requested TCLAS not supported */
#define DOT11_SC_TCLAS_RESOURCES_EXHAUSTED	81u	/* TCLAS resources exhausted */
#define DOT11_SC_TCLAS_PROCESSING_TERMINATED	97u	/* End traffic classification */

#define DOT11_SC_SAE_HASH_TO_ELEMENT	126	/* Using SAE HtoE PWE generator */
#define DOT11_SC_SAE_PK			127u	/* SAE PK required */

/* 802.11 be 2.0 9.4.1.9 Status Code field */
#define DOT11_SC_DENIDED_SAME_MLD_ASSOC_EXIST	130	/* Association denied because the
							 * requesting STA is affiliated with
							 * a non-AP MLD that is associated with
							 * the AP MLD.
							 */

#define DOT11_EPCS_DENIED_UNAUTHORIZED		131	/* EPCS priority access denied because the
							 * non-AP MLD is not authorized to use the
							 * service
							 */

#define DOT11_EPCS_DENIED			132	/* EPCS priority access denied with reason
							 * Unknown
							 */

#define DOT11_SC_DENIED_TID_MAP			133    /* Request denied because the requested */
#define DOT11_SC_PREFER_TID_MAP			134    /* Preferred TID-to-Link map suggested */

#define DOT11_SC_EHT_NOT_SUPPORTED		135
#define DOT11_SC_DENIED_ACTIVE_LINK_REJECT	139	/* 80211 be 3.0 Link not accepted because
							 * the link on which the (Re)Association
							 * request frame transmitted not accepted.
							 */

#define DOT11_EPCS_DENIED_VERIFICATION_FAILURE	140	/* EPCS priority access temporarily denied
							 * because the receiving AP MLD is unable
							 * to verify that the non-AP MLD authorized
							 * for an unspecified reason
							 */

#define DOT11_DENIED_OPERATION_PARAMETER_UPDATE	141	/* Operation parameter update denied because
							 * the requested operation parameters or
							 * capabilities are not acceptable.
							 */

/* Requested TCLAS processing has been terminated by the AP due to insufficient QoS capacity. */
#define DOT11_SC_TCLAS_PROCESSING_TERMINATED_INSUFFICIENT_QOS	128u

/* Requested TCLAS processing has been terminated by the AP due to conflict with
 * higher layer QoS policies.
 */
#define DOT11_SC_TCLAS_PROCESSING_TERMINATED_POLICY_CONFLICT	129u

/* Info Elts, length of INFORMATION portion of Info Elts */
#define DOT11_MNG_DS_PARAM_LEN			1	/* d11 management DS parameter length */
#define DOT11_MNG_IBSS_PARAM_LEN		2	/* d11 management IBSS parameter length */

/* TIM Info element has 3 bytes fixed info in INFORMATION field,
 * followed by 1 to 251 bytes of Partial Virtual Bitmap
 */
#define DOT11_MNG_TIM_FIXED_LEN			3	/* d11 management TIM fixed length */
#define DOT11_MNG_TIM_DTIM_COUNT		0	/* d11 management DTIM count */
#define DOT11_MNG_TIM_DTIM_PERIOD		1	/* d11 management DTIM period */
#define DOT11_MNG_TIM_BITMAP_CTL		2	/* d11 management TIM BITMAP control  */
#define DOT11_MNG_TIM_PVB			3	/* d11 management TIM PVB */

/* TLV defines */
#define TLV_TAG_OFF		0	/* tag offset */
#define TLV_LEN_OFF		1	/* length offset */
#define TLV_HDR_LEN		2	/* header length */
#define TLV_BODY_OFF		2	/* body offset */
#define TLV_BODY_LEN_MAX	255	/* max body length */
#define TLV_EXT_HDR_LEN		3u	/* extended IE header length */
#define TLV_EXT_BODY_OFF	3u	/* extended IE body offset */

/* Management Frame Information Element IDs */
#define DOT11_MNG_SSID_ID			0	/* d11 management SSID id */
#define DOT11_MNG_RATES_ID			1	/* d11 management rates id */
#define DOT11_MNG_FH_PARMS_ID			2	/* d11 management FH parameter id */
#define DOT11_MNG_DS_PARMS_ID			3	/* d11 management DS parameter id */
#define DOT11_MNG_CF_PARMS_ID			4	/* d11 management CF parameter id */
#define DOT11_MNG_TIM_ID			5	/* d11 management TIM id */
#define DOT11_MNG_IBSS_PARMS_ID			6	/* d11 management IBSS parameter id */
#define DOT11_MNG_COUNTRY_ID			7	/* d11 management country id */
#define DOT11_MNG_HOPPING_PARMS_ID		8	/* d11 management hopping parameter id */
#define DOT11_MNG_HOPPING_TABLE_ID		9	/* d11 management hopping table id */
#define DOT11_MNG_FTM_SYNC_INFO_ID		9	/* 11mc D4.3 */
#define DOT11_MNG_REQUEST_ID			10	/* d11 management request id */
#define DOT11_MNG_QBSS_LOAD_ID 			11	/* d11 management QBSS Load id */
#define DOT11_MNG_EDCA_PARAM_ID			12	/* 11E EDCA Parameter id */
#define DOT11_MNG_TSPEC_ID			13	/* d11 management TSPEC id */
#define DOT11_MNG_TCLAS_ID			14	/* d11 management TCLAS id */
#define DOT11_MNG_CHALLENGE_ID			16	/* d11 management chanllenge id */
#define DOT11_MNG_PWR_CONSTRAINT_ID		32	/* 11H PowerConstraint */
#define DOT11_MNG_PWR_CAP_ID			33	/* 11H PowerCapability */
#define DOT11_MNG_TPC_REQUEST_ID 		34	/* 11H TPC Request */
#define DOT11_MNG_TPC_REPORT_ID			35	/* 11H TPC Report */
#define DOT11_MNG_SUPP_CHANNELS_ID		36	/* 11H Supported Channels */
#define DOT11_MNG_CHANNEL_SWITCH_ID		37	/* 11H ChannelSwitch Announcement */
#define DOT11_MNG_MEASURE_REQUEST_ID		38	/* 11H MeasurementRequest */
#define DOT11_MNG_MEASURE_REPORT_ID		39	/* 11H MeasurementReport */
#define DOT11_MNG_QUIET_ID			40	/* 11H Quiet */
#define DOT11_MNG_IBSS_DFS_ID			41	/* 11H IBSS_DFS */
#define DOT11_MNG_ERP_ID			42	/* d11 management ERP id */
#define DOT11_MNG_TS_DELAY_ID			43	/* d11 management TS Delay id */
#define DOT11_MNG_TCLAS_PROC_ID			44	/* d11 management TCLAS processing id */
#define	DOT11_MNG_HT_CAP			45	/* d11 mgmt HT cap id */
#define DOT11_MNG_QOS_CAP_ID			46	/* 11E QoS Capability id */
#define DOT11_MNG_NONERP_ID			47	/* d11 management NON-ERP id */
#define DOT11_MNG_RSN_ID			48	/* d11 management RSN id */
#define DOT11_MNG_EXT_RATES_ID			50	/* d11 management ext. rates id */
#define DOT11_MNG_AP_CHREP_ID			51	/* 11k AP Channel report id */
#define DOT11_MNG_NEIGHBOR_REP_ID		52	/* 11k & 11v Neighbor report id */
#define DOT11_MNG_RCPI_ID			53	/* 11k RCPI */
#define DOT11_MNG_MDIE_ID			54	/* 11r Mobility domain id */
#define DOT11_MNG_FTIE_ID			55	/* 11r Fast Bss Transition id */
#define DOT11_MNG_FT_TI_ID			56	/* 11r Timeout Interval id */
#define DOT11_MNG_RDE_ID			57	/* 11r RIC Data Element id */
#define	DOT11_MNG_REGCLASS_ID			59	/* d11 management regulatory class id */
#define DOT11_MNG_EXT_CSA_ID			60	/* d11 Extended CSA */
#define	DOT11_MNG_HT_ADD			61	/* d11 mgmt additional HT info */
#define	DOT11_MNG_SEC_CHANNEL_OFFSET		62	/* d11 mgmt secondary channel offset */
#define DOT11_MNG_BSS_AVR_ACCESS_DELAY_ID	63	/* 11k bss average access delay */
#define DOT11_MNG_ANTENNA_ID			64	/* 11k antenna id */
#define DOT11_MNG_RSNI_ID			65	/* 11k RSNI id */
#define DOT11_MNG_MEASUREMENT_PILOT_TX_ID	66	/* 11k measurement pilot tx info id */
#define DOT11_MNG_BSS_AVAL_ADMISSION_CAP_ID	67	/* 11k bss aval admission cap id */
#define DOT11_MNG_BSS_AC_ACCESS_DELAY_ID	68	/* 11k bss AC access delay id */
#define DOT11_MNG_WAPI_ID			68	/* d11 management WAPI id */
#define DOT11_MNG_TIME_ADVERTISE_ID		69	/* 11p time advertisement */
#define DOT11_MNG_RRM_CAP_ID			70	/* 11k radio measurement capability */
#define DOT11_MNG_MULTIPLE_BSSID_ID		71	/* 11k multiple BSSID id */
#define DOT11_MNG_HT_BSS_COEXINFO_ID		72	/* d11 mgmt OBSS Coexistence INFO */
#define DOT11_MNG_HT_BSS_CHANNEL_REPORT_ID	73	/* d11 mgmt OBSS Intolerant Channel list */
#define DOT11_MNG_HT_OBSS_ID			74	/* d11 mgmt OBSS HT info */
#define DOT11_MNG_MMIE_ID			76	/* d11 mgmt MIC IE */
#define DOT11_MNG_EVENT_REQ_ID			78	/* d11 Event Request id */
#define DOT11_MNG_EVENT_REP_ID			79	/* d11 Event Report id */
#define DOT11_MNG_NONTRANS_BSSID_CAP_ID		83	/* d11 Nontransmitted BSSID Capability */
#define DOT11_MNG_MULTIPLE_BSSID_IDX_ID		85	/* d11 Multiple BSSID-Index */
#define DOT11_MNG_FMS_DESCR_ID			86	/* 11v FMS descriptor */
#define DOT11_MNG_FMS_REQ_ID			87	/* 11v FMS request id */
#define DOT11_MNG_FMS_RESP_ID			88	/* 11v FMS response id */
#define DOT11_MNG_BSS_MAX_IDLE_PERIOD_ID	90	/* 11v bss max idle id */
#define DOT11_MNG_TFS_REQUEST_ID		91	/* 11v tfs request id */
#define DOT11_MNG_TFS_RESPONSE_ID		92	/* 11v tfs response id */
#define DOT11_MNG_WNM_SLEEP_MODE_ID		93	/* 11v wnm-sleep mode id */
#define DOT11_MNG_TIMBC_REQ_ID			94	/* 11v TIM broadcast request id */
#define DOT11_MNG_TIMBC_RESP_ID			95	/* 11v TIM broadcast response id */
#define DOT11_MNG_CHANNEL_USAGE			97	/* 11v channel usage */
#define DOT11_MNG_TIME_ZONE_ID			98	/* 11v time zone */
#define DOT11_MNG_DMS_REQUEST_ID		99	/* 11v dms request id */
#define DOT11_MNG_DMS_RESPONSE_ID		100	/* 11v dms response id */
#define DOT11_MNG_LINK_IDENTIFIER_ID		101	/* 11z TDLS Link Identifier IE */
#define DOT11_MNG_WAKEUP_SCHEDULE_ID		102	/* 11z TDLS Wakeup Schedule IE */
#define DOT11_MNG_CHANNEL_SWITCH_TIMING_ID	104	/* 11z TDLS Channel Switch Timing IE */
#define DOT11_MNG_PTI_CONTROL_ID		105	/* 11z TDLS PTI Control IE */
#define DOT11_MNG_PU_BUFFER_STATUS_ID		106	/* 11z TDLS PU Buffer Status IE */
#define DOT11_MNG_INTERWORKING_ID		107	/* 11u interworking */
#define DOT11_MNG_ADVERTISEMENT_ID		108	/* 11u advertisement protocol */
#define DOT11_MNG_EXP_BW_REQ_ID			109	/* 11u expedited bandwith request */
#define DOT11_MNG_QOS_MAP_ID			110	/* 11u QoS map set */
#define DOT11_MNG_ROAM_CONSORT_ID		111	/* 11u roaming consortium */
#define DOT11_MNG_EMERGCY_ALERT_ID		112	/* 11u emergency alert identifier */
#define DOT11_MNG_MESH_CONFIG			113	/* Mesh Configuration */
#define DOT11_MNG_MESH_ID			114	/* Mesh ID */
#define DOT11_MNG_MESH_PEER_MGMT_ID		117	/* Mesh PEER MGMT IE */
#define DOT11_MNG_EXT_CAP_ID			127	/* d11 mgmt ext capability */
#define DOT11_MNG_EXT_PREQ_ID			130	/* Mesh PREQ IE */
#define DOT11_MNG_EXT_PREP_ID			131	/* Mesh PREP IE */
#define DOT11_MNG_EXT_PERR_ID			132	/* Mesh PERR IE */
#define DOT11_MNG_MULTI_BAND_ID			158	/* Multi-band IE */
#define DOT11_MNG_EXT_ADDBA_ID			159	/* ADDBA extension IE */
#define DOT11_MNG_INTRA_AC_PRIO_ID		184	/* Intra-Access Category Priority IE */
#define	DOT11_MNG_SCS_DESCR_ID			185	/* SCS Descriptor IE */
#define	DOT11_MNG_GCR_GRPADDR_ID		189	/* GCR group address IE */
#define	DOT11_MNG_VHT_CAP_ID			191	/* d11 mgmt VHT cap id */
#define	DOT11_MNG_VHT_OPERATION_ID		192	/* d11 mgmt VHT op id */
#define	DOT11_MNG_EXT_BSSLOAD_ID		193	/* d11 mgmt VHT extended bss load id */
#define DOT11_MNG_WIDE_BW_CHANNEL_SWITCH_ID	194	/* Wide BW Channel Switch IE */
#define DOT11_MNG_TRANSMIT_POWER_ENVELOPE_ID	195	/* >=VHT transmit Power Envelope IE */
#define DOT11_MNG_VHT_TRANSMIT_POWER_ENVELOPE_ID DOT11_MNG_TRANSMIT_POWER_ENVELOPE_ID /* obsolete */
#define DOT11_MNG_CHANNEL_SWITCH_WRAPPER_ID	196	/* Channel Switch Wrapper IE */
#define DOT11_MNG_AID_ID			197	/* Association ID  IE */
#define	DOT11_MNG_OPER_MODE_NOTIF_ID		199	/* d11 mgmt VHT oper mode notif */
#define DOT11_MNG_RNR_ID			201
#define DOT11_MNG_FTM_PARAMS_ID			206
#define DOT11_MNG_TWT_ID			216	/* 11ah D5.0 */
#define DOT11_MNG_WPA_ID			221	/* d11 management WPA id */
#define DOT11_MNG_PROPR_ID			221
/* should start using this one instead of above two */
#define DOT11_MNG_VS_ID				221	/* d11 management Vendor Specific IE */
#define DOT11_MNG_MESH_CSP_ID			222	/* d11 Mesh Channel Switch Parameter */
#define DOT11_MNG_FILS_IND_ID			240	/* 11ai FILS Indication element */
#define DOT11_MNG_FRAGMENT_ID			242     /* IE's fragment ID */
#define DOT11_MNG_RSNXE_ID			244     /* RSN Extension Element (RSNXE) ID */

/* The following ID extensions should be defined >= 255
 * i.e. the values should include 255 (DOT11_MNG_ID_EXT_ID + ID Extension).
 */
#define DOT11_MNG_ID_EXT_ID			255	/* Element ID Extension 11mc D4.3 */

/* FILS and OCE ext ids */
#define FILS_EXTID_MNG_REQ_PARAMS		2u	/* FILS Request Parameters element */
#define DOT11_MNG_FILS_REQ_PARAMS		(DOT11_MNG_ID_EXT_ID + FILS_EXTID_MNG_REQ_PARAMS)
#define FILS_EXTID_MNG_KEY_CONFIRMATION_ID	3u	/* FILS Key Confirmation element */
#define DOT11_MNG_FILS_KEY_CONFIRMATION		(DOT11_MNG_ID_EXT_ID +\
							FILS_EXTID_MNG_KEY_CONFIRMATION_ID)
#define FILS_EXTID_MNG_SESSION_ID		4u	/* FILS Session element */
#define DOT11_MNG_FILS_SESSION			(DOT11_MNG_ID_EXT_ID + FILS_EXTID_MNG_SESSION_ID)
#define FILS_EXTID_MNG_HLP_CONTAINER_ID		5u	/* FILS HLP Container element */
#define DOT11_MNG_FILS_HLP_CONTAINER		(DOT11_MNG_ID_EXT_ID +\
							FILS_EXTID_MNG_HLP_CONTAINER_ID)
#define FILS_EXTID_MNG_WRAPPED_DATA_ID		8u	/* FILS Wrapped Data element */
#define DOT11_MNG_FILS_WRAPPED_DATA		(DOT11_MNG_ID_EXT_ID +\
							FILS_EXTID_MNG_WRAPPED_DATA_ID)
#define OCE_EXTID_MNG_ESP_ID			11u	/* Estimated Service Parameters element */
#define DOT11_MNG_ESP				(DOT11_MNG_ID_EXT_ID + OCE_EXTID_MNG_ESP_ID)
#define FILS_EXTID_MNG_PUBLIC_KEY_ID		12u	/* FILS Public Key element */
#define DOT11_MNG_FILS_PUBLIC_KEY		(DOT11_MNG_ID_EXT_ID + FILS_EXTID_MNG_PUBLIC_KEY_ID)
#define FILS_EXTID_MNG_NONCE_ID			13u	/* FILS Nonce element */
#define DOT11_MNG_FILS_NONCE			(DOT11_MNG_ID_EXT_ID + FILS_EXTID_MNG_NONCE_ID)

#define EXT_MNG_DH_PARAM_ID			32	/* Diffie-Hellman Parameter */
#define DOT11_MNG_DH_PARAM_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_DH_PARAM_ID)
#define EXT_MNG_HE_CAP_ID			35	/* HE Capabilities, 11ax */
#define DOT11_MNG_HE_CAP_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_HE_CAP_ID)
#define EXT_MNG_HE_OP_ID			36	/* HE Operation, 11ax */
#define DOT11_MNG_HE_OP_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_HE_OP_ID)
#define EXT_MNG_RAPS_ID				37	/* OFDMA Random Access Parameter Set */
#define DOT11_MNG_RAPS_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_RAPS_ID)
#define EXT_MNG_MU_EDCA_ID			38	/* MU EDCA Parameter Set */
#define DOT11_MNG_MU_EDCA_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_MU_EDCA_ID)
#define EXT_MNG_SRPS_ID				39	/* Spatial Reuse Parameter Set */
#define DOT11_MNG_SRPS_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_SRPS_ID)
#define EXT_MNG_NDP_FR_ID			41	/* NDP Feedback Report Parameter Set */
#define DOT11_MNG_NDP_FR_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_NDP_FR_ID)
#define EXT_MNG_COLOR_CHANGE_ID			42	/* BSS Color Change Announcement */
#define DOT11_MNG_COLOR_CHANGE_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_COLOR_CHANGE_ID)
#define EXT_MNG_QUIET_TIME_ID			43	/* Quiet Time Period Setup */
#define DOT11_MNG_QUIET_TIME_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_QUIET_TIME_ID)
#define EXT_MNG_ESS_REPORT_ID			45	/* ESS Report */
#define DOT11_MNG_ESS_REPORT_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_ESS_REPORT_ID)
#define EXT_MNG_OPS_ID				46	/* OPS - Opportunistic Power Save */
#define DOT11_MNG_OPS_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_OPS_ID)
#define EXT_MNG_HE_BSS_LOAD_ID			47	/* HE BSS Load */
#define DOT11_MNG_HE_BSS_LOAD_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_HE_BSS_LOAD_ID)
#define EXT_MNG_MAX_CHANNEL_SWITCH_TIME_ID	52	/* Max Channel Switch Time */
#define DOT11_MNG_MAX_CHANNEL_SWITCH_TIME_ID	(DOT11_MNG_ID_EXT_ID +\
							EXT_MNG_MAX_CHANNEL_SWITCH_TIME_ID)
#define EXT_MNG_OCI_ID	                        54	/* Operating channel validation */
#define DOT11_MNG_OCI_ID			(DOT11_MNG_ID_EXT_ID +\
							EXT_MNG_OCI_ID)
#define EXT_MNG_MBSSID_CONFIG_ID	        55	/* MBSSID CONFIG */
#define DOT11_MNG_MBSSID_CONFIG_ID	(DOT11_MNG_ID_EXT_ID +\
							EXT_MNG_MBSSID_CONFIG_ID)
#define EXT_MNG_MBSSID_NON_INHERITANCE_ID	56	/* MBSSID NON-INHERITANCE */
#define DOT11_MNG_MBSSID_NON_INHERITANCE_ID	(DOT11_MNG_ID_EXT_ID +\
							EXT_MNG_MBSSID_NON_INHERITANCE_ID)
#define EXT_MNG_SHORT_SSID_ID			58	/* 255+58==313 */
#define DOT11_MNG_SHORT_SSID_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_SHORT_SSID_ID)
#define EXT_MNG_HE_6G_BAND_CAPS_ID		59	/* HE 6G Band Capabilities */
#define DOT11_MNG_HE_6G_BAND_CAPS_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_HE_6G_BAND_CAPS_ID)

#define MSCS_EXTID_MNG_DESCR_ID			88u	/* Ext ID for the MSCS descriptor */
#define DOT11_MNG_MSCS_DESCR_ID			(DOT11_MNG_ID_EXT_ID + MSCS_EXTID_MNG_DESCR_ID)

#define TCLAS_EXTID_MNG_MASK_ID			89u	/* Ext ID for the TCLAS Mask element */
#define DOT11_MNG_TCLASS_MASK_ID		(DOT11_MNG_ID_EXT_ID + TCLAS_EXTID_MNG_MASK_ID)

/* P802.11az/D2.5 Table 9-92 */
#define EXT_MNG_SLTF_PARAMS_ID			94u	/* Secure LTF Parameters */
#define DOT11_MNG_SLTF_PARAMS_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_SLTF_PARAMS_ID)
#define EXT_MNG_ISTA_PASV_TB_RMR_ID		95u	/* ISTA Passive TB Ranging Meas. Report */
#define DOT11_MNG_ISTA_PASV_TB_RMR_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_ISTA_PASV_TB_RMR_ID)
#define EXT_MNG_RSTA_PASV_TB_RMR_ID		96u	/* RSTA Passive TB Ranging Meas. Report */
#define DOT11_MNG_RSTA_PASV_TB_RMR_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_RSTA_PASV_TB_RMR_ID)
#define EXT_MNG_PASV_TB_LCI_TABLE_ID		97u	/* Passive TB Ranging LCI Table element */
#define DOT11_MNG_PASV_TB_LCI_TABLE_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_PASV_TB_LCI_TABLE_ID)
#define EXT_MNG_ISTA_AVAIL_WINDOW_ID		98u	/* ISTA Availablity Window */
#define DOT11_MNG_ISTA_AVAIL_WINDOW_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_ISTA_AVAIL_WINDOW_ID)
#define EXT_MNG_RSTA_AVAIL_WINDOW_ID		99u	/* RSTA Availablity Window */
#define DOT11_MNG_RSTA_AVAIL_WINDOW_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_RSTA_AVAIL_WINDOW_ID)
#define EXT_MNG_PASN_PARAMS_ID			100u	/* PASN Parameters */
#define DOT11_MNG_PASN_PARAMS_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_PASN_PARAMS_ID)
#define EXT_MNG_RANGING_PARAMS_ID		101u	/* Ranging Parameters */
#define DOT11_MNG_RANGING_PARAMS_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_RANGING_PARAMS_ID)
#define EXT_MNG_DIR_MEAS_RESULTS_ID		102u	/* Direction Measurement Results */
#define DOT11_MNG_DIR_MEAS_RESULTS_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_DIR_MEAS_RESULTS_ID)
#define EXT_MNG_MULTI_AOD_FEEDBACK_ID		103u	/* Multiple AOD Feedback */
#define DOT11_MNG_MULTI_AOD_FEEDDBACK_ID	(DOT11_MNG_ID_EXT_ID + \
						 EXT_MNG_MULTI_AOD_FEEDBACK_ID)
#define EXT_MNG_MULTI_BEST_AWV_ID_ID		104u	/* Multiple Best AWV ID */
#define DOT11_MNG_MULTI_BEST_AWV_ID_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_MULTI_BEST_AWV_ID_ID)
#define EXT_MNG_LOS_LIKELIHOOD_ID		105u	/* LOS Likelihood */
#define DOT11_MNG_LOS_LIKELIHOOD_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_LOS_LIKELIHOOD_ID)

/* Draft P802.11be 3.0 Table 9-128 */
#define EXT_MNG_EHT_OP_ID			106u	/* EHT Operation */
#define DOT11_MNG_EHT_OP_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_EHT_OP_ID)
#define EXT_MNG_EHT_ML_ID			107u	/* Multi-Link */
#define DOT11_MNG_EHT_ML_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_EHT_ML_ID)
#define EXT_MNG_EHT_CAP_ID			108u	/* EHT Capabilities */
#define DOT11_MNG_EHT_CAP_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_EHT_CAP_ID)
#define EXT_MNG_TID_LINK_MAPPING_ID		109u	/* TID-To-Link Mapping */
#define DOT11_MNG_TID_LINK_MAPPING_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_TID_LINK_MAPPING_ID)
#define EXT_MNG_ML_TRAFFIC_ID			110u	/* Multi-Link Traffic */
#define DOT11_MNG_ML_TRAFFIC_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_ML_TRAFFIC_ID)
#define EXT_MNG_QOS_CHARACTERISTICS_ID		113u	/* QoS characteristics */
#define DOT11_MNG_QOS_CHARACTERISTICS_ID	(DOT11_MNG_ID_EXT_ID + \
						 EXT_MNG_QOS_CHARACTERISTICS_ID)
#define EXT_MNG_MLO_LINK_INFO_ID		133u	/* MLO Link Information */
#define DOT11_MNG_MLO_LINK_INFO_ID		(DOT11_MNG_ID_EXT_ID + EXT_MNG_MLO_LINK_INFO_ID)
#define EXT_MNG_AID_BMP_ID			134u	/* AID Bitmap */
#define DOT11_MNG_AID_BMP_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_AID_BMP_ID)
#define EXT_MNG_BW_IND_ID			135u	/* Bandwidth Indication */
#define DOT11_MNG_BW_IND_ID			(DOT11_MNG_ID_EXT_ID + EXT_MNG_BW_IND_ID)

/* deprecated definitions, do not use, to be deleted later */
#define FILS_HLP_CONTAINER_EXT_ID	FILS_EXTID_MNG_HLP_CONTAINER_ID
#define DOT11_ESP_EXT_ID		OCE_EXTID_MNG_ESP_ID
#define FILS_REQ_PARAMS_EXT_ID		FILS_EXTID_MNG_REQ_PARAMS
/* End of deprecated definitions */

#define DOT11_MNG_IE_ID_EXT_MATCH(_ie, _id) (\
	((_ie)->id == DOT11_MNG_ID_EXT_ID) && \
	((_ie)->len > 0) && \
	((_id) == ((uint8 *)(_ie) + TLV_HDR_LEN)[0]))

#define DOT11_MNG_IE_ID_EXT_INIT(_ie, _id, _len) do {\
		(_ie)->id = DOT11_MNG_ID_EXT_ID; \
		(_ie)->len = _len; \
		(_ie)->id_ext = _id; \
	} while (0)

/* Rate Defines */

/* Valid rates for the Supported Rates and Extended Supported Rates IEs.
 * Encoding is the rate in 500kbps units, rouding up for fractional values.
 * 802.11-2012, section 6.5.5.2, DATA_RATE parameter enumerates all the values.
 * The rate values cover DSSS, HR/DSSS, ERP, and OFDM phy rates.
 * The defines below do not cover the rates specific to 10MHz, {3, 4.5, 27},
 * and 5MHz, {1.5, 2.25, 3, 4.5, 13.5}, which are not supported by Broadcom devices.
 */

#define DOT11_RATE_1M   2       /* 1  Mbps in 500kbps units */
#define DOT11_RATE_2M   4       /* 2  Mbps in 500kbps units */
#define DOT11_RATE_5M5  11      /* 5.5 Mbps in 500kbps units */
#define DOT11_RATE_11M  22      /* 11 Mbps in 500kbps units */
#define DOT11_RATE_6M   12      /* 6  Mbps in 500kbps units */
#define DOT11_RATE_9M   18      /* 9  Mbps in 500kbps units */
#define DOT11_RATE_12M  24      /* 12 Mbps in 500kbps units */
#define DOT11_RATE_18M  36      /* 18 Mbps in 500kbps units */
#define DOT11_RATE_24M  48      /* 24 Mbps in 500kbps units */
#define DOT11_RATE_36M  72      /* 36 Mbps in 500kbps units */
#define DOT11_RATE_48M  96      /* 48 Mbps in 500kbps units */
#define DOT11_RATE_54M  108     /* 54 Mbps in 500kbps units */
#define DOT11_RATE_MAX  108     /* highest rate (54 Mbps) in 500kbps units */

/* Supported Rates and Extended Supported Rates IEs
 * The supported rates octets are defined a the MSB indicatin a Basic Rate
 * and bits 0-6 as the rate value
 */
#define DOT11_RATE_BASIC                0x80 /* flag for a Basic Rate */
#define DOT11_RATE_MASK                 0x7F /* mask for numeric part of rate */

/* BSS Membership Selector parameters
 * 802.11-2016 (and 802.11ax-D1.1), Sec 9.4.2.3
 * These selector values are advertised in Supported Rates and Extended Supported Rates IEs
 * in the supported rates list with the Basic rate bit set.
 * Constants below include the basic bit.
 */
#define DOT11_BSS_MEMBERSHIP_HT		0xFF  /* Basic 0x80 + 127, HT Required to join */
#define DOT11_BSS_MEMBERSHIP_VHT        0xFE  /* Basic 0x80 + 126, VHT Required to join */
#define DOT11_BSS_MEMBERSHIP_GLK	0XFD  /* Basic rate 0x80 + 125 */
#define DOT11_BSS_MEMBERSHIP_EPD	0XFC  /* Basic rate 0x80 + 124 */
#define DOT11_BSS_MEMBERSHIP_SAE_H2E_ONLY	0XFB /* Basic 0x80 + 123, to join */
#define DOT11_BSS_MEMBERSHIP_HE         0xFA	/* Basic 0x80 + 122, HE Required to join  */
/* P802.11be D2.0 Table 9-129 BSS membership selector value encoding - TBD */
#define DOT11_BSS_MEMBERSHIP_EHT	0xF9	/* Basic 0x80 + 121, EHT Required to join */

/* ERP info element bit values */
#define DOT11_MNG_ERP_LEN			1	/* ERP is currently 1 byte long */
#define DOT11_MNG_NONERP_PRESENT		0x01	/* NonERP (802.11b) STAs are present
							 *in the BSS
							 */
#define DOT11_MNG_USE_PROTECTION		0x02	/* Use protection mechanisms for
							 *ERP-OFDM frames
							 */
#define DOT11_MNG_BARKER_PREAMBLE		0x04	/* Short Preambles: 0 == allowed,
							 * 1 == not allowed
							 */
/* TS Delay element offset & size */
#define DOT11_MGN_TS_DELAY_LEN		4	/* length of TS DELAY IE */
#define TS_DELAY_FIELD_SIZE			4	/* TS DELAY field size */

/* (non-VHT, non-HE) Capability Information Field, as found in e.g. an assoc-req frame */
#define DOT11_CAP_ESS				0x0001	/* d11 cap. ESS */
#define DOT11_CAP_IBSS				0x0002	/* d11 cap. IBSS */
#define DOT11_CAP_POLLABLE			0x0004	/* d11 cap. pollable */
#define DOT11_CAP_POLL_RQ			0x0008	/* d11 cap. poll request */
#define DOT11_CAP_PRIVACY			0x0010	/* d11 cap. privacy */
#define DOT11_CAP_SHORT				0x0020	/* d11 cap. short */
#define DOT11_CAP_PBCC				0x0040	/* d11 cap. PBCC */
#define DOT11_CAP_AGILITY			0x0080	/* d11 cap. agility */
#define DOT11_CAP_SPECTRUM			0x0100	/* d11 cap. spectrum */
#define DOT11_CAP_QOS				0x0200	/* d11 cap. qos */
#define DOT11_CAP_SHORTSLOT			0x0400	/* d11 cap. shortslot */
#define DOT11_CAP_APSD				0x0800	/* d11 cap. apsd */
#define DOT11_CAP_RRM				0x1000	/* d11 cap. 11k radio measurement */
#define DOT11_CAP_CCK_OFDM			0x2000	/* d11 cap. CCK/OFDM */
#define DOT11_CAP_DELAY_BA			0x4000	/* d11 cap. delayed block ack */
#define DOT11_CAP_IMMEDIATE_BA			0x8000	/* d11 cap. immediate block ack */

/* Extended capabilities IE bitfields */
/* 20/40 BSS Coexistence Management support bit position */
#define DOT11_EXT_CAP_OBSS_COEX_MGMT		0
/* Extended Channel Switching support bit position */
#define DOT11_EXT_CAP_EXT_CHAN_SWITCHING	2
/* scheduled PSMP support bit position */
#define DOT11_EXT_CAP_SPSMP			6
/* Event Request/Report support bit position */
#define DOT11_EXT_CAP_EVENT			7
/*  Flexible Multicast Service */
#define DOT11_EXT_CAP_FMS			11
/* proxy ARP service support bit position */
#define DOT11_EXT_CAP_PROXY_ARP			12
/* Civic Location */
#define DOT11_EXT_CAP_CIVIC_LOC			14
/* Geospatial Location */
#define DOT11_EXT_CAP_LCI			15
/* Traffic Filter Service */
#define DOT11_EXT_CAP_TFS			16
/* WNM-Sleep Mode */
#define DOT11_EXT_CAP_WNM_SLEEP			17
/* TIM Broadcast service */
#define DOT11_EXT_CAP_TIMBC			18
/* BSS Transition Management support bit position */
#define DOT11_EXT_CAP_BSSTRANS_MGMT		19
/* Multiple BSSID */
#define DOT11_EXT_CAP_MBSSID			22
/* Direct Multicast Service */
#define DOT11_EXT_CAP_DMS			26
/* Interworking support bit position */
#define DOT11_EXT_CAP_IW			31
/* QoS map support bit position */
#define DOT11_EXT_CAP_QOS_MAP			32
/* service Interval granularity bit position and mask */
#define DOT11_EXT_CAP_SI			41
#define DOT11_EXT_CAP_SI_MASK			0x0E
/* Location Identifier service */
#define DOT11_EXT_CAP_IDENT_LOC			44
/* WNM notification */
#define DOT11_EXT_CAP_WNM_NOTIF			46
#define DOT11_EXT_CAP_SCS			54
/* Operating mode notification - VHT (11ac D3.0 - 8.4.2.29) */
#define DOT11_EXT_CAP_OPER_MODE_NOTIF		62
/* Bit 63 - 64
 * Indicates the maximum number of MSDUs in an A-MSDU that the STA is able to
 * receive from a VHT STA:
 * Set to 0 to indicate that no limit applies. (Bit64 = 0, Bit63 = 0)
 * Set to 1 for 32. (Bit64 = 0, Bit63 = 1)
 * Set to 2 for 16. (Bit64 = 1, Bit63 = 0)
 * Set to 3 for 8. (Bit64 = 1, Bit63 = 1)
 * Reserved if A-MSDU is not supported or if the STA is not an HT STA.
*/
#define DOT11_EXT_CAP_NUM_MSDU			63

/* Fine timing measurement - D3.0 */
#define DOT11_EXT_CAP_FTM_RESPONDER		70
#define DOT11_EXT_CAP_FTM_INITIATOR		71 /* tentative 11mcd3.0 */
#define DOT11_EXT_CAP_FILS			72 /* FILS Capability */
/* TWT support */
#define DOT11_EXT_CAP_TWT_REQUESTER		77
#define DOT11_EXT_CAP_TWT_RESPONDER		78
/*
 * An AP STA sets the OBSS Narrow Bandwidth RU In OFDMA Tolerance
 * Support field to 1 if dot11OBSSNarrowBWRUinOFDMATolerated
 * is true, and sets it to 0 otherwise.
 * A non-AP STA always sets it to 0.
 */
#define DOT11_EXT_CAP_OBSS_NBWRU_TOLERANCE      79
/* Multiple BSSID support, Complete List of NonTxBSSID Profiles (11ax D4.0) */
#define DOT11_EXT_CAP_COMPLETE_NONTXBSSID_PROF	80

/* SAE password ID */
#define DOT11_EXT_CAP_SAE_PWD_ID_INUSE		81u
#define DOT11_EXT_CAP_SAE_PWD_ID_USED_EXCLUSIVE	82u
/* Beacon Protection Enabled 802.11 D3.0 - 9.4.2.26
 * This field is reserved for a STA.
 */
#define DOT11_EXT_CAP_BCN_PROT			84u
#define DOT11_EXT_CAP_MSCS			85u
#define DOT11_EXT_CAP_SAEPK_PW_USED_EXCLU	88u
/* Draft P802.11az/D2.5 - Table 9-153 Extended Capabilities field */
#define DOT11_EXT_CAP_NTB_RANGING_RESPONDER	90u /* 11az NTB RSTA */
#define DOT11_EXT_CAP_TB_RANGING_RESPONDER	91u /* 11az TB RSTA */
#define DOT11_EXT_CAP_PSV_RANGING_RESPONDER	92u /* 11az Passive RSTA */
#define DOT11_EXT_CAP_PSV_RANGING_INITIATOR	93u /* 11az Passive ISTA */
#define DOT11_EXT_CAP_AOA_MEASUREMENT		94u /* AOA measurement available */
#define DOT11_EXT_CAP_PHASE_SHIFT_FEEDBACK	95u /* Phase shift feedback support */
#define DOT11_EXT_CAP_DMG_LOCATION_SUPPORT_AP	96u /* DMG/Location supporting APs in the area */
#define DOT11_EXT_CAP_I2R_LMR_FEEDBACK_POLICY	97u /* I2R LMR Feedback policy (of RSTA) */

/* TODO: Update DOT11_EXT_CAP_MAX_IDX to reflect the highest offset.
 * Note: DOT11_EXT_CAP_MAX_IDX must only be used in attach path.
 *       It will cause ROM invalidation otherwise.
 * Note: This is highest bit index + 1, as bits counting start at 0.
 */
#define DOT11_EXT_CAP_MAX_IDX		(DOT11_EXT_CAP_I2R_LMR_FEEDBACK_POLICY + 1)

#define DOT11_EXTCAP_LEN_MAX			((DOT11_EXT_CAP_MAX_IDX + 7) >> 3)

BWL_PRE_PACKED_STRUCT struct dot11_extcap {
	uint8 extcap[DOT11_EXTCAP_LEN_MAX];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_extcap dot11_extcap_t;

/* VHT Operating mode bit fields -  (11ac D8.0/802.11-2016 - 9.4.1.53) */
#define DOT11_OPER_MODE_CHANNEL_WIDTH_SHIFT 0
#define DOT11_OPER_MODE_CHANNEL_WIDTH_MASK 0x3
#define DOT11_OPER_MODE_160_8080_BW_SHIFT 2
#define DOT11_OPER_MODE_160_8080_BW_MASK 0x04
#define DOT11_OPER_MODE_NOLDPC_SHIFT 3
#define DOT11_OPER_MODE_NOLDPC_MASK 0x08
#define DOT11_OPER_MODE_RXNSS_SHIFT 4
#define DOT11_OPER_MODE_RXNSS_MASK 0x70
#define DOT11_OPER_MODE_RXNSS_TYPE_SHIFT 7
#define DOT11_OPER_MODE_RXNSS_TYPE_MASK 0x80

#define DOT11_OPER_MODE(type, nss, chanw) (\
	((type) << DOT11_OPER_MODE_RXNSS_TYPE_SHIFT &\
		 DOT11_OPER_MODE_RXNSS_TYPE_MASK) |\
	(((nss) - 1) << DOT11_OPER_MODE_RXNSS_SHIFT & DOT11_OPER_MODE_RXNSS_MASK) |\
	((chanw) << DOT11_OPER_MODE_CHANNEL_WIDTH_SHIFT &\
		 DOT11_OPER_MODE_CHANNEL_WIDTH_MASK))

#define DOT11_D8_OPER_MODE(type, nss, ldpc, bw160_8080, chanw) (\
	((type) << DOT11_OPER_MODE_RXNSS_TYPE_SHIFT &\
		 DOT11_OPER_MODE_RXNSS_TYPE_MASK) |\
	(((nss) - 1) << DOT11_OPER_MODE_RXNSS_SHIFT & DOT11_OPER_MODE_RXNSS_MASK) |\
	((ldpc) << DOT11_OPER_MODE_NOLDPC_SHIFT & DOT11_OPER_MODE_NOLDPC_MASK) |\
	((bw160_8080) << DOT11_OPER_MODE_160_8080_BW_SHIFT &\
		 DOT11_OPER_MODE_160_8080_BW_MASK) |\
	((chanw) << DOT11_OPER_MODE_CHANNEL_WIDTH_SHIFT &\
		 DOT11_OPER_MODE_CHANNEL_WIDTH_MASK))

#define DOT11_OPER_MODE_CHANNEL_WIDTH(mode) \
	(((mode) & DOT11_OPER_MODE_CHANNEL_WIDTH_MASK)\
		>> DOT11_OPER_MODE_CHANNEL_WIDTH_SHIFT)
#define DOT11_OPER_MODE_160_8080(mode) \
	(((mode) & DOT11_OPER_MODE_160_8080_BW_MASK)\
		>> DOT11_OPER_MODE_160_8080_BW_SHIFT)
#define DOT11_OPER_MODE_RXNSS(mode) \
	((((mode) & DOT11_OPER_MODE_RXNSS_MASK)		\
		>> DOT11_OPER_MODE_RXNSS_SHIFT) + 1)
#define DOT11_OPER_MODE_RXNSS_TYPE(mode) \
	(((mode) & DOT11_OPER_MODE_RXNSS_TYPE_MASK)\
		>> DOT11_OPER_MODE_RXNSS_TYPE_SHIFT)

#define DOT11_OPER_MODE_20MHZ 0
#define DOT11_OPER_MODE_40MHZ 1
#define DOT11_OPER_MODE_80MHZ 2
#define DOT11_OPER_MODE_160MHZ 3
#define DOT11_OPER_MODE_8080MHZ 3
#define DOT11_OPER_MODE_1608080MHZ 1

#define DOT11_OPER_MODE_CHANNEL_WIDTH_20MHZ(mode) (\
	((mode) & DOT11_OPER_MODE_CHANNEL_WIDTH_MASK) == DOT11_OPER_MODE_20MHZ)
#define DOT11_OPER_MODE_CHANNEL_WIDTH_40MHZ(mode) (\
	((mode) & DOT11_OPER_MODE_CHANNEL_WIDTH_MASK) == DOT11_OPER_MODE_40MHZ)
#define DOT11_OPER_MODE_CHANNEL_WIDTH_80MHZ(mode) (\
	((mode) & DOT11_OPER_MODE_CHANNEL_WIDTH_MASK) == DOT11_OPER_MODE_80MHZ)
#define DOT11_OPER_MODE_CHANNEL_WIDTH_160MHZ(mode) (\
	((mode) & DOT11_OPER_MODE_160_8080_BW_MASK))
#define DOT11_OPER_MODE_CHANNEL_WIDTH_8080MHZ(mode) (\
	((mode) & DOT11_OPER_MODE_160_8080_BW_MASK))

/* Operating mode information element 802.11ac D3.0 - 8.4.2.168 */
BWL_PRE_PACKED_STRUCT struct dot11_oper_mode_notif_ie {
	uint8 mode;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_oper_mode_notif_ie dot11_oper_mode_notif_ie_t;

#define DOT11_OPER_MODE_NOTIF_IE_LEN 1

/* Extended Capability Information Field */
#define DOT11_OBSS_COEX_MNG_SUPPORT	0x01	/* 20/40 BSS Coexistence Management support */

/*
 * Action Frame Constants
 */
#define DOT11_ACTION_HDR_LEN		2	/* action frame category + action field */
#define DOT11_ACTION_CAT_OFF		0	/* category offset */
#define DOT11_ACTION_ACT_OFF		1	/* action offset */

/* Action Category field (sec 8.4.1.11) */
#define DOT11_ACTION_CAT_ERR_MASK	0x80	/* category error mask */
#define DOT11_ACTION_CAT_MASK		0x7F	/* category mask */
#define DOT11_ACTION_CAT_SPECT_MNG	0	/* category spectrum management */
#define DOT11_ACTION_CAT_QOS		1	/* category QoS */
#define DOT11_ACTION_CAT_DLS		2	/* category DLS */
#define DOT11_ACTION_CAT_BLOCKACK	3	/* category block ack */
#define DOT11_ACTION_CAT_PUBLIC		4	/* category public */
#define DOT11_ACTION_CAT_RRM		5	/* category radio measurements */
#define DOT11_ACTION_CAT_FBT		6	/* category fast bss transition */
#define DOT11_ACTION_CAT_HT		7	/* category for HT */
#define	DOT11_ACTION_CAT_SA_QUERY	8	/* security association query */
#define	DOT11_ACTION_CAT_PDPA		9	/* protected dual of public action */
#define DOT11_ACTION_CAT_WNM		10	/* category for WNM */
#define DOT11_ACTION_CAT_UWNM		11	/* category for Unprotected WNM */
#define DOT11_ACTION_CAT_MESH		13	/* category for Mesh */
#define DOT11_ACTION_CAT_SELFPROT	15	/* category for Mesh, self protected */
#define DOT11_ACTION_NOTIFICATION	17
#define DOT11_ACTION_CAT_QOSMGMT	19	/* category for Robust AV Streaming */
#define DOT11_ACTION_CAT_VHT		21	/* VHT action */
#define DOT11_ACTION_CAT_S1G		22	/* S1G action */
#define DOT11_ACTION_CAT_FILS		26	/* FILS action frame */
#define DOT11_ACTION_CAT_HE		30	/* HE action frame */
#define DOT11_ACTION_CAT_PFT		34	/* Protected Fine Timing action frame */
/* EHT Action frames - Draft P802.11be D2.2 Sections 9.6.35.8 */
#define DOT11_ACTION_CAT_EHT		36u	/* EHT action frame */
#define DOT11_ACTION_CAT_EHTP		37u	/* Protected EHT action frame */
#define DOT11_ACTION_CAT_VSP		126	/* protected vendor specific */
#define DOT11_ACTION_CAT_VS		127	/* category Vendor Specific */
#define DOT11_ACTION_CAT_WBD_INVALID 125 /* WBD specific action frame for stamon */

/* Spectrum Management Action IDs (sec 7.4.1) */
#define DOT11_SM_ACTION_M_REQ		0	/* d11 action measurement request */
#define DOT11_SM_ACTION_M_REP		1	/* d11 action measurement response */
#define DOT11_SM_ACTION_TPC_REQ		2	/* d11 action TPC request */
#define DOT11_SM_ACTION_TPC_REP		3	/* d11 action TPC response */
#define DOT11_SM_ACTION_CHANNEL_SWITCH	4	/* d11 action channel switch */
#define DOT11_SM_ACTION_EXT_CSA		5	/* d11 extened CSA for 11n */

/* QoS action ids */
#define DOT11_QOS_ACTION_ADDTS_REQ	0	/* d11 action ADDTS request */
#define DOT11_QOS_ACTION_ADDTS_RESP	1	/* d11 action ADDTS response */
#define DOT11_QOS_ACTION_DELTS		2	/* d11 action DELTS */
#define DOT11_QOS_ACTION_SCHEDULE	3	/* d11 action schedule */
#define DOT11_QOS_ACTION_QOS_MAP	4	/* d11 action QOS map */

/* HT action ids */
#define DOT11_ACTION_ID_HT_CH_WIDTH	0	/* notify channel width action id */
#define DOT11_ACTION_ID_HT_MIMO_PS	1	/* mimo ps action id */

/* Public action ids */
#define DOT11_PUB_ACTION_BSS_COEX_MNG	0	/* 20/40 Coexistence Management action id */
#define DOT11_PUB_ACTION_CHANNEL_SWITCH	4	/* d11 action channel switch */
#define DOT11_PUB_ACTION_VENDOR_SPEC	9	/* Vendor specific */
#define DOT11_PUB_ACTION_GAS_CB_REQ	12	/* GAS Comeback Request */
#define DOT11_PUB_ACTION_FTM_REQ	32		/* FTM request */
#define DOT11_PUB_ACTION_FTM		33		/* FTM measurement */
#define DOT11_PUB_ACTION_LMR		47		/* 11az Location Measurement Report */
#define DOT11_PUB_ACTION_PSV_TB_RMR_ISTA	48 /* ISTA Passive TB Ranging Measurement Report */
#define DOT11_PUB_ACTION_PSV_TB_RMR_RSTA_PRI	49 /* Primary RSTA Broadcase Passive TB RMR */
#define DOT11_PUB_ACTION_PSV_TB_RMR_RSTA_2ND	50 /* Secondary RSTA Broadcase Passive TB RMR */

#define DOT11_PUB_ACTION_FTM_REQ_TRIGGER_START	1u	/* FTM request start trigger */
#define DOT11_PUB_ACTION_FTM_REQ_TRIGGER_STOP	0u	/* FTM request stop trigger */

/* Block Ack action types */
#define DOT11_BA_ACTION_ADDBA_REQ	0	/* ADDBA Req action frame type */
#define DOT11_BA_ACTION_ADDBA_RESP	1	/* ADDBA Resp action frame type */
#define DOT11_BA_ACTION_DELBA		2	/* DELBA action frame type */

/* ADDBA action parameters */
#define DOT11_ADDBA_PARAM_AMSDU_SUP	0x0001	/* AMSDU supported under BA */
#define DOT11_ADDBA_PARAM_POLICY_MASK	0x0002	/* policy mask(ack vs delayed) */
#define DOT11_ADDBA_PARAM_POLICY_SHIFT	1	/* policy shift */
#define DOT11_ADDBA_PARAM_TID_MASK	0x003c	/* tid mask */
#define DOT11_ADDBA_PARAM_TID_SHIFT	2	/* tid shift */
#define DOT11_ADDBA_PARAM_BSIZE_MASK	0xffc0	/* buffer size mask */
#define DOT11_ADDBA_PARAM_BSIZE_SHIFT	6	/* buffer size shift */
#define DOT11_ADDBA_PARAM_BSIZE_FSZ	10	/* buffer size bits */

#define DOT11_ADDBA_POLICY_DELAYED	0	/* delayed BA policy */
#define DOT11_ADDBA_POLICY_IMMEDIATE	1	/* immediate BA policy */

/* Fast Transition action types */
#define DOT11_FT_ACTION_FT_RESERVED		0
#define DOT11_FT_ACTION_FT_REQ			1	/* FBT request - for over-the-DS FBT */
#define DOT11_FT_ACTION_FT_RES			2	/* FBT response - for over-the-DS FBT */
#define DOT11_FT_ACTION_FT_CON			3	/* FBT confirm - for OTDS with RRP */
#define DOT11_FT_ACTION_FT_ACK			4	/* FBT ack */

/* DLS action types */
#define DOT11_DLS_ACTION_REQ			0	/* DLS Request */
#define DOT11_DLS_ACTION_RESP			1	/* DLS Response */
#define DOT11_DLS_ACTION_TD			2	/* DLS Teardown */

/* Robust Audio Video streaming action types */
#define DOT11_QOSMGMT_SCS_REQ			0  /* SCS Request */
#define DOT11_QOSMGMT_SCS_RES			1  /* SCS Response */
#define DOT11_QOSMGMT_GM_REQ			2  /* Group Membership Request */
#define DOT11_QOSMGMT_GM_RES			3  /* Group Membership Response */
#define DOT11_QOSMGMT_MSCS_REQ			4  /* MSCS Request */
#define DOT11_QOSMGMT_MSCS_RES			5  /* MSCS Response */

/* Wireless Network Management (WNM) action types */
#define DOT11_WNM_ACTION_EVENT_REQ		0
#define DOT11_WNM_ACTION_EVENT_REP		1
#define DOT11_WNM_ACTION_DIAG_REQ		2
#define DOT11_WNM_ACTION_DIAG_REP		3
#define DOT11_WNM_ACTION_LOC_CFG_REQ		4
#define DOT11_WNM_ACTION_LOC_RFG_RESP		5
#define DOT11_WNM_ACTION_BSSTRANS_QUERY		6
#define DOT11_WNM_ACTION_BSSTRANS_REQ		7
#define DOT11_WNM_ACTION_BSSTRANS_RESP		8
#define DOT11_WNM_ACTION_FMS_REQ		9
#define DOT11_WNM_ACTION_FMS_RESP		10
#define DOT11_WNM_ACTION_COL_INTRFRNCE_REQ	11
#define DOT11_WNM_ACTION_COL_INTRFRNCE_REP	12
#define DOT11_WNM_ACTION_TFS_REQ		13
#define DOT11_WNM_ACTION_TFS_RESP		14
#define DOT11_WNM_ACTION_TFS_NOTIFY_REQ		15
#define DOT11_WNM_ACTION_WNM_SLEEP_REQ		16
#define DOT11_WNM_ACTION_WNM_SLEEP_RESP		17
#define DOT11_WNM_ACTION_TIMBC_REQ		18
#define DOT11_WNM_ACTION_TIMBC_RESP		19
#define DOT11_WNM_ACTION_QOS_TRFC_CAP_UPD	20
#define DOT11_WNM_ACTION_CHAN_USAGE_REQ		21
#define DOT11_WNM_ACTION_CHAN_USAGE_RESP	22
#define DOT11_WNM_ACTION_DMS_REQ		23
#define DOT11_WNM_ACTION_DMS_RESP		24
#define DOT11_WNM_ACTION_TMNG_MEASUR_REQ	25
#define DOT11_WNM_ACTION_NOTFCTN_REQ		26
#define DOT11_WNM_ACTION_NOTFCTN_RESP		27
#define DOT11_WNM_ACTION_TFS_NOTIFY_RESP	28

/* Unprotected Wireless Network Management (WNM) action types */
#define DOT11_UWNM_ACTION_TIM			0
#define DOT11_UWNM_ACTION_TIMING_MEASUREMENT	1

#define DOT11_CNTRY_STRING_LEN		3
#define DOT11_CNTRY_SUBBAND_LEN		3
#define DOT11_CNTRY_OPERATING_LEN	3
#define DOT11_CNTRY_OPERATING_EXT_ID	201

/* VHT category action types - 802.11ac D3.0 - 8.5.23.1 */
#define DOT11_VHT_ACTION_CBF				0	/* Compressed Beamforming */
#define DOT11_VHT_ACTION_GID_MGMT			1	/* Group ID Management */
#define DOT11_VHT_ACTION_OPER_MODE_NOTIF	2	/* Operating mode notif'n */

/* FILS category action types - 802.11ai D11.0 - 9.6.8.1 */
#define DOT11_FILS_ACTION_DISCOVERY		34	/* FILS Discovery */

/* section: 9.4.2.256 ESS Report element Draft P802.11ax_D8.0 */
typedef BWL_PRE_PACKED_STRUCT struct ess_report {
	uint8	id;
	uint8	len;
	uint8	id_ext;
	uint8	val;
} BWL_POST_PACKED_STRUCT ess_report_t;

/** DLS Request frame header */
BWL_PRE_PACKED_STRUCT struct dot11_dls_req {
	uint8 category;			/* category of action frame (2) */
	uint8 action;				/* DLS action: req (0) */
	struct ether_addr	da;		/* destination address */
	struct ether_addr	sa;		/* source address */
	uint16 cap;				/* capability */
	uint16 timeout;			/* timeout value */
	uint8 data[1];				/* IE:support rate, extend support rate, HT cap */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_dls_req dot11_dls_req_t;
#define DOT11_DLS_REQ_LEN 18	/* Fixed length */

/** DLS response frame header */
BWL_PRE_PACKED_STRUCT struct dot11_dls_resp {
	uint8 category;			/* category of action frame (2) */
	uint8 action;				/* DLS action: req (0) */
	uint16 status;				/* status code field */
	struct ether_addr	da;		/* destination address */
	struct ether_addr	sa;		/* source address */
	uint8 data[1];				/* optional: capability, rate ... */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_dls_resp dot11_dls_resp_t;
#define DOT11_DLS_RESP_LEN 16	/* Fixed length */

/* ************* 802.11v related definitions. ************* */

/** BSS Management Transition Query frame header */
BWL_PRE_PACKED_STRUCT struct dot11_bsstrans_query {
	uint8 category;			/* category of action frame (10) */
	uint8 action;			/* WNM action: trans_query (6) */
	uint8 token;			/* dialog token */
	uint8 reason;			/* transition query reason */
	uint8 data[1];			/* Elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_bsstrans_query dot11_bsstrans_query_t;
#define DOT11_BSSTRANS_QUERY_LEN 4	/* Fixed length */

/* BTM transition reason */
#define DOT11_BSSTRANS_REASON_UNSPECIFIED		0
#define DOT11_BSSTRANS_REASON_EXC_FRAME_LOSS		1
#define DOT11_BSSTRANS_REASON_EXC_TRAFFIC_DELAY		2
#define DOT11_BSSTRANS_REASON_INSUFF_QOS_CAPACITY	3
#define DOT11_BSSTRANS_REASON_FIRST_ASSOC		4
#define DOT11_BSSTRANS_REASON_LOAD_BALANCING		5
#define DOT11_BSSTRANS_REASON_BETTER_AP_FOUND		6
#define DOT11_BSSTRANS_REASON_DEAUTH_RX			7
#define DOT11_BSSTRANS_REASON_8021X_EAP_AUTH_FAIL	8
#define DOT11_BSSTRANS_REASON_4WAY_HANDSHK_FAIL		9
#define DOT11_BSSTRANS_REASON_MANY_REPLAYCNT_FAIL	10
#define DOT11_BSSTRANS_REASON_MANY_DATAMIC_FAIL		11
#define DOT11_BSSTRANS_REASON_EXCEED_MAX_RETRANS	12
#define DOT11_BSSTRANS_REASON_MANY_BCAST_DISASSOC_RX	13
#define DOT11_BSSTRANS_REASON_MANY_BCAST_DEAUTH_RX	14
#define DOT11_BSSTRANS_REASON_PREV_TRANSITION_FAIL	15
#define DOT11_BSSTRANS_REASON_LOW_RSSI			16
#define DOT11_BSSTRANS_REASON_ROAM_FROM_NON_80211	17
#define DOT11_BSSTRANS_REASON_RX_BTM_REQ		18
#define DOT11_BSSTRANS_REASON_PREF_LIST_INCLUDED	19
#define DOT11_BSSTRANS_REASON_LEAVING_ESS		20

/** BSS Management Transition Request frame header */
BWL_PRE_PACKED_STRUCT struct dot11_bsstrans_req {
	uint8 category;			/* category of action frame (10) */
	uint8 action;			/* WNM action: trans_req (7) */
	uint8 token;			/* dialog token */
	uint8 reqmode;			/* transition request mode */
	uint16 disassoc_tmr;		/* disassociation timer */
	uint8 validity_intrvl;		/* validity interval */
	uint8 data[1];			/* optional: BSS term duration, ... */
						/* ...session info URL, candidate list */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_bsstrans_req dot11_bsstrans_req_t;
#define DOT11_BSSTRANS_REQ_LEN 7	/* Fixed length */

/* BSS Mgmt Transition Request Mode Field - 802.11v */
#define DOT11_BSSTRANS_REQMODE_PREF_LIST_INCL		0x01
#define DOT11_BSSTRANS_REQMODE_ABRIDGED			0x02
#define DOT11_BSSTRANS_REQMODE_DISASSOC_IMMINENT	0x04
#define DOT11_BSSTRANS_REQMODE_BSS_TERM_INCL		0x08
#define DOT11_BSSTRANS_REQMODE_ESS_DISASSOC_IMNT	0x10
#define DOT11_BSSTRANS_REQMODE_LINK_REMOVAL_IMNT	0x20

/** BSS Management transition response frame header */
BWL_PRE_PACKED_STRUCT struct dot11_bsstrans_resp {
	uint8 category;			/* category of action frame (10) */
	uint8 action;			/* WNM action: trans_resp (8) */
	uint8 token;			/* dialog token */
	uint8 status;			/* transition status */
	uint8 term_delay;		/* validity interval */
	uint8 data[1];			/* optional: BSSID target, candidate list */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_bsstrans_resp dot11_bsstrans_resp_t;
#define DOT11_BSSTRANS_RESP_LEN 5	/* Fixed length */

/* BSS Mgmt Transition Response Status Field */
#define DOT11_BSSTRANS_RESP_STATUS_ACCEPT			0
#define DOT11_BSSTRANS_RESP_STATUS_REJECT			1
#define DOT11_BSSTRANS_RESP_STATUS_REJ_INSUFF_BCN		2
#define DOT11_BSSTRANS_RESP_STATUS_REJ_INSUFF_CAP		3
#define DOT11_BSSTRANS_RESP_STATUS_REJ_TERM_UNDESIRED		4
#define DOT11_BSSTRANS_RESP_STATUS_REJ_TERM_DELAY_REQ		5
#define DOT11_BSSTRANS_RESP_STATUS_REJ_BSS_LIST_PROVIDED	6
#define DOT11_BSSTRANS_RESP_STATUS_REJ_NO_SUITABLE_BSS		7
#define DOT11_BSSTRANS_RESP_STATUS_REJ_LEAVING_ESS		8

/** BSS Max Idle Period element */
BWL_PRE_PACKED_STRUCT struct dot11_bss_max_idle_period_ie {
	uint8 id;				/* 90, DOT11_MNG_BSS_MAX_IDLE_PERIOD_ID */
	uint8 len;
	uint16 max_idle_period;			/* in unit of 1000 TUs */
	uint8 idle_opt;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_bss_max_idle_period_ie dot11_bss_max_idle_period_ie_t;
#define DOT11_BSS_MAX_IDLE_PERIOD_IE_LEN	3	/* bss max idle period IE size */
#define DOT11_BSS_MAX_IDLE_PERIOD_OPT_PROTECTED	1	/* BSS max idle option */

/** TIM Broadcast request element */
BWL_PRE_PACKED_STRUCT struct dot11_timbc_req_ie {
	uint8 id;				/* 94, DOT11_MNG_TIMBC_REQ_ID */
	uint8 len;
	uint8 interval;				/* in unit of beacon interval */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_timbc_req_ie dot11_timbc_req_ie_t;
#define DOT11_TIMBC_REQ_IE_LEN		1	/* Fixed length */

/** TIM Broadcast request frame header */
BWL_PRE_PACKED_STRUCT struct dot11_timbc_req {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: DOT11_WNM_ACTION_TIMBC_REQ(18) */
	uint8 token;				/* dialog token */
	uint8 data[1];				/* TIM broadcast request element */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_timbc_req dot11_timbc_req_t;
#define DOT11_TIMBC_REQ_LEN		3	/* Fixed length */

/** TIM Broadcast response element */
BWL_PRE_PACKED_STRUCT struct dot11_timbc_resp_ie {
	uint8 id;				/* 95, DOT11_MNG_TIM_BROADCAST_RESP_ID */
	uint8 len;
	uint8 status;				/* status of add request */
	uint8 interval;				/* in unit of beacon interval */
	int32 offset;				/* in unit of ms */
	uint16 high_rate;			/* in unit of 0.5 Mb/s */
	uint16 low_rate;			/* in unit of 0.5 Mb/s */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_timbc_resp_ie dot11_timbc_resp_ie_t;
#define DOT11_TIMBC_DENY_RESP_IE_LEN	1	/* Deny. Fixed length */
#define DOT11_TIMBC_ACCEPT_RESP_IE_LEN	10	/* Accept. Fixed length */

#define DOT11_TIMBC_STATUS_ACCEPT		0
#define DOT11_TIMBC_STATUS_ACCEPT_TSTAMP	1
#define DOT11_TIMBC_STATUS_DENY			2
#define DOT11_TIMBC_STATUS_OVERRIDDEN		3
#define DOT11_TIMBC_STATUS_RESERVED		4

/** TIM Broadcast request frame header */
BWL_PRE_PACKED_STRUCT struct dot11_timbc_resp {
	uint8 category;			/* category of action frame (10) */
	uint8 action;			/* action: DOT11_WNM_ACTION_TIMBC_RESP(19) */
	uint8 token;			/* dialog token */
	uint8 data[1];			/* TIM broadcast response element */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_timbc_resp dot11_timbc_resp_t;
#define DOT11_TIMBC_RESP_LEN	3	/* Fixed length */

/** TIM element */
BWL_PRE_PACKED_STRUCT struct dot11_tim_ie {
	uint8 id;			/* 5, DOT11_MNG_TIM_ID	 */
	uint8 len;			/* 4 - 255 */
	uint8 dtim_count;		/* DTIM decrementing counter */
	uint8 dtim_period;		/* DTIM period */
	uint8 bitmap_control;		/* AID 0 + bitmap offset */
	uint8 pvb[1];			/* Partial Virtual Bitmap, variable length */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tim_ie dot11_tim_ie_t;
#define DOT11_TIM_IE_FIXED_LEN	3	/* Fixed length, without id and len */
#define DOT11_TIM_IE_FIXED_TOTAL_LEN	5	/* Fixed length, with id and len */

/** TIM Broadcast frame header */
BWL_PRE_PACKED_STRUCT struct dot11_timbc {
	uint8 category;			/* category of action frame (11) */
	uint8 action;			/* action: TIM (0) */
	uint8 check_beacon;		/* need to check-beacon */
	uint8 tsf[8];			/* Time Synchronization Function */
	dot11_tim_ie_t tim_ie;		/* TIM element */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_timbc dot11_timbc_t;
#define DOT11_TIMBC_HDR_LEN	(sizeof(dot11_timbc_t) - sizeof(dot11_tim_ie_t))
#define DOT11_TIMBC_FIXED_LEN	(sizeof(dot11_timbc_t) - 1)	/* Fixed length */
#define DOT11_TIMBC_LEN			11	/* Fixed length */

/** TCLAS frame classifier type */
BWL_PRE_PACKED_STRUCT struct dot11_tclas_fc_hdr {
	uint8 type;
	uint8 mask;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tclas_fc_hdr dot11_tclas_fc_hdr_t;
#define DOT11_TCLAS_FC_HDR_LEN		2	/* Fixed length */

#define DOT11_TCLAS_MASK_0		0x1
#define DOT11_TCLAS_MASK_1		0x2
#define DOT11_TCLAS_MASK_2		0x4
#define DOT11_TCLAS_MASK_3		0x8
#define DOT11_TCLAS_MASK_4		0x10
#define DOT11_TCLAS_MASK_5		0x20
#define DOT11_TCLAS_MASK_6		0x40
#define DOT11_TCLAS_MASK_7		0x80

#define DOT11_TCLAS_FC4_MASK_VER		0x1
#define DOT11_TCLAS_FC4_MASK_SRC_IP		0x2
#define DOT11_TCLAS_FC4_MASK_DST_IP		0x4
#define DOT11_TCLAS_FC4_MASK_SRC_PORT		0x8
#define DOT11_TCLAS_FC4_MASK_DST_PORT		0x10
#define DOT11_TCLAS_FC4_MASK_DSCP		0x20
#define DOT11_TCLAS_FC4_MASK_PROT		0x40	/* Valid for ipv4 */
#define DOT11_TCLAS_FC4_MASK_NEXT_HDR		0x40	/* Valid for ipv6 */
#define DOT11_TCLAS_FC4_MASK_FLOW_LABEL		0x80

#define DOT11_TCLAS_FC_0_ETH		0
#define DOT11_TCLAS_FC_1_IP		1
#define DOT11_TCLAS_FC_2_8021Q		2
#define DOT11_TCLAS_FC_3_OFFSET		3
#define DOT11_TCLAS_FC_4_IP_HIGHER	4
#define DOT11_TCLAS_FC_5_8021D		5
#define DOT11_TCLAS_FC_10_IP_EXTEN_HIGHER	10

/** TCLAS frame classifier type 0 parameters for Ethernet */
BWL_PRE_PACKED_STRUCT struct dot11_tclas_fc_0_eth {
	uint8 type;
	uint8 mask;
	uint8 sa[ETHER_ADDR_LEN];
	uint8 da[ETHER_ADDR_LEN];
	uint16 eth_type;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tclas_fc_0_eth dot11_tclas_fc_0_eth_t;
#define DOT11_TCLAS_FC_0_ETH_LEN	16

/** TCLAS frame classifier type 1 parameters for IPV4 */
BWL_PRE_PACKED_STRUCT struct dot11_tclas_fc_1_ipv4 {
	uint8 type;
	uint8 mask;
	uint8 version;
	uint32 src_ip;
	uint32 dst_ip;
	uint16 src_port;
	uint16 dst_port;
	uint8 dscp;
	uint8 protocol;
	uint8 reserved;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tclas_fc_1_ipv4 dot11_tclas_fc_1_ipv4_t;
#define DOT11_TCLAS_FC_1_IPV4_LEN	18

/** TCLAS frame classifier type 2 parameters for 802.1Q */
BWL_PRE_PACKED_STRUCT struct dot11_tclas_fc_2_8021q {
	uint8 type;
	uint8 mask;
	uint16 tci;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tclas_fc_2_8021q dot11_tclas_fc_2_8021q_t;
#define DOT11_TCLAS_FC_2_8021Q_LEN	4

/** TCLAS frame classifier type 3 parameters for filter offset */
BWL_PRE_PACKED_STRUCT struct dot11_tclas_fc_3_filter {
	uint8 type;
	uint8 mask;
	uint16 offset;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tclas_fc_3_filter dot11_tclas_fc_3_filter_t;
#define DOT11_TCLAS_FC_3_FILTER_LEN	4

/** TCLAS frame classifier type 4 parameters for IPV4 is the same as TCLAS type 1 */
typedef struct dot11_tclas_fc_1_ipv4 dot11_tclas_fc_4_ipv4_t;
#define DOT11_TCLAS_FC_4_IPV4_LEN	DOT11_TCLAS_FC_1_IPV4_LEN

/** TCLAS frame classifier type 4 parameters for IPV6 */
BWL_PRE_PACKED_STRUCT struct dot11_tclas_fc_4_ipv6 {
	uint8 type;
	uint8 mask;
	uint8 version;
	uint8 saddr[16];
	uint8 daddr[16];
	uint16 src_port;
	uint16 dst_port;
	uint8 dscp;
	uint8 nexthdr;
	uint8 flow_lbl[3];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tclas_fc_4_ipv6 dot11_tclas_fc_4_ipv6_t;
#define DOT11_TCLAS_FC_4_IPV6_LEN	44

/** TCLAS frame classifier type 5 parameters for 802.1D */
BWL_PRE_PACKED_STRUCT struct dot11_tclas_fc_5_8021d {
	uint8 type;
	uint8 mask;
	uint8 pcp;
	uint8 cfi;
	uint16 vid;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tclas_fc_5_8021d dot11_tclas_fc_5_8021d_t;
#define DOT11_TCLAS_FC_5_8021D_LEN	6

/** TCLAS frame classifier type 10 parameters for ip extension and higher layers */
BWL_PRE_PACKED_STRUCT struct dot11_tclas_fc_10_ip_ext {
	uint8 type;
	uint8 prot_instance;
	uint8 prot_nexthdr;
	uint8 filter[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tclas_fc_10_ip_ext dot11_tclas_fc_10_ip_ext_t;
#define DOT11_TCLAS_FC10_IE_DATA_LEN	6	/* assuming 1B for filter val and 1B for mask */

/** TCLAS frame classifier type parameters */
BWL_PRE_PACKED_STRUCT union dot11_tclas_fc {
	uint8 data[1];
	dot11_tclas_fc_hdr_t hdr;
	dot11_tclas_fc_0_eth_t t0_eth;
	dot11_tclas_fc_1_ipv4_t	t1_ipv4;
	dot11_tclas_fc_2_8021q_t t2_8021q;
	dot11_tclas_fc_3_filter_t t3_filter;
	dot11_tclas_fc_4_ipv4_t	t4_ipv4;
	dot11_tclas_fc_4_ipv6_t	t4_ipv6;
	dot11_tclas_fc_5_8021d_t t5_8021d;
	dot11_tclas_fc_10_ip_ext_t t10_ip_ext;
} BWL_POST_PACKED_STRUCT;
typedef union dot11_tclas_fc dot11_tclas_fc_t;

#define DOT11_TCLAS_FC_MIN_LEN		4	/* Classifier Type 2 has the min size */
#define DOT11_TCLAS_FC_MAX_LEN		254
#define DOT11_TCLAS_FC4_IPV4_LEN	18u
#define DOT11_TCLAS_FC4_IPV6_LEN	44u

/** TCLAS element */
BWL_PRE_PACKED_STRUCT struct dot11_tclas_ie {
	uint8 id;				/* 14, DOT11_MNG_TCLAS_ID */
	uint8 len;
	uint8 user_priority;
	dot11_tclas_fc_t fc;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tclas_ie dot11_tclas_ie_t;
#define DOT11_TCLAS_IE_LEN		3	/* Fixed length, include id and len */
#define DOT11_TCLAS_IE_DATA_LEN_FC4_IPV4 (1 + DOT11_TCLAS_FC4_IPV4_LEN) /* 1B user_priority */
#define DOT11_TCLAS_IE_DATA_LEN_FC4_IPV6 (1 + DOT11_TCLAS_FC4_IPV6_LEN) /* 1B user_priority */

/** TCLAS processing element */
BWL_PRE_PACKED_STRUCT struct dot11_tclas_proc_ie {
	uint8 id;				/* 44, DOT11_MNG_TCLAS_PROC_ID */
	uint8 len;
	uint8 process;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tclas_proc_ie dot11_tclas_proc_ie_t;
#define DOT11_TCLAS_PROC_IE_LEN		3	/* Fixed length, include id and len */
#define DOT11_TCLAS_PROC_LEN		1u	/* Proc ie length is always 1 byte */

#define DOT11_TCLAS_PROC_MATCHALL	0	/* All high level element need to match */
#define DOT11_TCLAS_PROC_MATCHONE	1	/* One high level element need to match */
#define DOT11_TCLAS_PROC_NONMATCH	2	/* Non match to any high level element */

/* Intra-Access Category priority  element */
BWL_PRE_PACKED_STRUCT struct dot11_intra_access_prio_ie {
	uint8 id;				/* 184, DOT11_MNG_INTRA_AC_PRIO_ID */
	uint8 len;
	uint8 access_prio;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_intra_access_prio_ie dot11_intra_access_prio_ie_t;
#define DOT11_INTRA_ACCESS_PRIO_IE_LEN	3	/* Fixed one byte length, include id and len */
#define DOT11_INTRA_ACCESS_PRIO_DATA_LEN	1u	/* Only 1B data access_prio */
#define DOT11_INTRA_ACCESS_PRIO_BIT_FIELD_PRIO 7 /* 3 LSBs of access_prio is priority */

/* TSPEC element defined in 802.11 std section 8.4.2.32 - Not supported */
#define DOT11_TSPEC_IE_LEN		57	/* Fixed length */
/** WNM Event request */

BWL_PRE_PACKED_STRUCT struct dot11_event_req {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: Event request (0) */
	uint8 dialog_token;			/* Dialog Token */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_event_req dot11_event_req_t;
#define DOT11_EVENT_REQ_LEN		3	/* Fixed length */

/** WNM Event report */
BWL_PRE_PACKED_STRUCT struct dot11_event_rep {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: Event report (1) */
	uint8 dialog_token;			/* Dialog Token */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_event_rep dot11_event_rep_t;
#define DOT11_EVENT_REP_LEN		3	/* Fixed length */

/** WNM Event request element */
BWL_PRE_PACKED_STRUCT struct dot11_event_req_element {
	uint8 id;				/* 78 (request) */
	uint8 len;
	uint8 event_token;			/* Event Token */
	uint8 event_type;			/* Event Type */
	uint8 event_response_limit;		/* Event Response Limit */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_event_req_element dot11_event_req_element_t;
#define DOT11_EVENT_REQ_ELEMENT_LEN	5	/* Fixed length */

/** WNM Event report element */
BWL_PRE_PACKED_STRUCT struct dot11_event_rep_element {
	uint8 id;				/* 79 (report) */
	uint8 len;
	uint8 event_token;			/* Event Token */
	uint8 event_type;			/* Event Type */
	uint8 event_report_status;		/* Event Response Limit */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_event_rep_element dot11_event_rep_element_t;
#define DOT11_EVENT_REP_ELEMENT_LEN	5	/* Fixed length */

/** Event Type */
#define DOT11_EVENT_TYPE_TRANSITION		0
#define DOT11_EVENT_TYPE_RSNA			1
#define DOT11_EVENT_TYPE_PEER_TO_PEER_LINK	2
#define DOT11_EVENT_TYPE_WNM_LOG		3
#define DOT11_EVENT_TYPE_BSS_COLOR_COLLISION	4
#define DOT11_EVENT_TYPE_BSS_COLOR_IN_USE	5
#define DOT11_EVENT_TYPE_VENDOR_SPECIFIC	221

/** Event report Status */
#define DOT11_EVENT_REPORT_STATUS_SUCCESS	0
#define DOT11_EVENT_REPORT_STATUS_FAILED	1
#define DOT11_EVENT_REPORT_STATUS_REFUSED	2
#define DOT11_EVENT_REPORT_STATUS_INCAPABLE	3
#define DOT11_EVENT_REPORT_STATUS_TRANSITION	4

/** Event report elements length */
#define DOT11_EVENT_REPORT_TSF			8
#define DOT11_EVENT_REPORT_UTC_OFFSET		10
#define DOT11_EVENT_TIME_ERROR			5

/** BSS color collision event */
BWL_PRE_PACKED_STRUCT struct dot11_event_bss_color_collision_report {
	uint8 color[8];				/* each bit representing a BSS color value */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_event_bss_color_collision_report dot11_event_bss_color_collision_report_t;
#define DOT11_EVENT_COLOR_COLLISION_REPORT_LEN	8	/* Fixed length */

/** TFS request element */
BWL_PRE_PACKED_STRUCT struct dot11_tfs_req_ie {
	uint8 id;				/* 91, DOT11_MNG_TFS_REQUEST_ID */
	uint8 len;
	uint8 tfs_id;
	uint8 actcode;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tfs_req_ie dot11_tfs_req_ie_t;
#define DOT11_TFS_REQ_IE_LEN		2	/* Fixed length, without id and len */

/** TFS request action codes (bitfield) */
#define DOT11_TFS_ACTCODE_DELETE	1
#define DOT11_TFS_ACTCODE_NOTIFY	2

/** TFS request subelement IDs */
#define DOT11_TFS_REQ_TFS_SE_ID		1
#define DOT11_TFS_REQ_VENDOR_SE_ID	221

/** TFS subelement */
BWL_PRE_PACKED_STRUCT struct dot11_tfs_se {
	uint8 sub_id;
	uint8 len;
	uint8 data[1];				/* TCLAS element(s) + optional TCLAS proc */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tfs_se dot11_tfs_se_t;

/** TFS response element */
BWL_PRE_PACKED_STRUCT struct dot11_tfs_resp_ie {
	uint8 id;				/* 92, DOT11_MNG_TFS_RESPONSE_ID */
	uint8 len;
	uint8 tfs_id;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tfs_resp_ie dot11_tfs_resp_ie_t;
#define DOT11_TFS_RESP_IE_LEN		1	/* Fixed length, without id and len */

/** TFS response subelement IDs (same subelments, but different IDs than in TFS request */
#define DOT11_TFS_RESP_TFS_STATUS_SE_ID		1
#define DOT11_TFS_RESP_TFS_SE_ID		2
#define DOT11_TFS_RESP_VENDOR_SE_ID		221

/** TFS status subelement */
BWL_PRE_PACKED_STRUCT struct dot11_tfs_status_se {
	uint8 sub_id;				/* 92, DOT11_MNG_TFS_RESPONSE_ID */
	uint8 len;
	uint8 resp_st;
	uint8 data[1];				/* Potential dot11_tfs_se_t included */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tfs_status_se dot11_tfs_status_se_t;
#define DOT11_TFS_STATUS_SE_LEN			1	/* Fixed length, without id and len */

/* Following Definition should be merged to FMS_TFS macro below */
/* TFS Response status code. Identical to FMS Element status, without N/A  */
#define DOT11_TFS_STATUS_ACCEPT			0
#define DOT11_TFS_STATUS_DENY_FORMAT		1
#define DOT11_TFS_STATUS_DENY_RESOURCE		2
#define DOT11_TFS_STATUS_DENY_POLICY		4
#define DOT11_TFS_STATUS_DENY_UNSPECIFIED	5
#define DOT11_TFS_STATUS_ALTPREF_POLICY		7
#define DOT11_TFS_STATUS_ALTPREF_TCLAS_UNSUPP	14

/* FMS Element Status and TFS Response Status Definition */
#define DOT11_FMS_TFS_STATUS_ACCEPT		0
#define DOT11_FMS_TFS_STATUS_DENY_FORMAT	1
#define DOT11_FMS_TFS_STATUS_DENY_RESOURCE	2
#define DOT11_FMS_TFS_STATUS_DENY_MULTIPLE_DI	3
#define DOT11_FMS_TFS_STATUS_DENY_POLICY	4
#define DOT11_FMS_TFS_STATUS_DENY_UNSPECIFIED	5
#define DOT11_FMS_TFS_STATUS_ALT_DIFF_DI	6
#define DOT11_FMS_TFS_STATUS_ALT_POLICY		7
#define DOT11_FMS_TFS_STATUS_ALT_CHANGE_DI	8
#define DOT11_FMS_TFS_STATUS_ALT_MCRATE		9
#define DOT11_FMS_TFS_STATUS_TERM_POLICY	10
#define DOT11_FMS_TFS_STATUS_TERM_RESOURCE	11
#define DOT11_FMS_TFS_STATUS_TERM_HIGHER_PRIO	12
#define DOT11_FMS_TFS_STATUS_ALT_CHANGE_MDI	13
#define DOT11_FMS_TFS_STATUS_ALT_TCLAS_UNSUPP	14

/** TFS Management Request frame header */
BWL_PRE_PACKED_STRUCT struct dot11_tfs_req {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: TFS request (13) */
	uint8 token;				/* dialog token */
	uint8 data[1];				/* Elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tfs_req dot11_tfs_req_t;
#define DOT11_TFS_REQ_LEN		3	/* Fixed length */

/** TFS Management Response frame header */
BWL_PRE_PACKED_STRUCT struct dot11_tfs_resp {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: TFS request (14) */
	uint8 token;				/* dialog token */
	uint8 data[1];				/* Elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tfs_resp dot11_tfs_resp_t;
#define DOT11_TFS_RESP_LEN		3	/* Fixed length */

/** TFS Management Notify frame request header */
BWL_PRE_PACKED_STRUCT struct dot11_tfs_notify_req {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: TFS notify request (15) */
	uint8 tfs_id_cnt;			/* TFS IDs count */
	uint8 tfs_id[1];			/* Array of TFS IDs */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tfs_notify_req dot11_tfs_notify_req_t;
#define DOT11_TFS_NOTIFY_REQ_LEN	3	/* Fixed length */

/** TFS Management Notify frame response header */
BWL_PRE_PACKED_STRUCT struct dot11_tfs_notify_resp {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: TFS notify response (28) */
	uint8 tfs_id_cnt;			/* TFS IDs count */
	uint8 tfs_id[1];			/* Array of TFS IDs */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tfs_notify_resp dot11_tfs_notify_resp_t;
#define DOT11_TFS_NOTIFY_RESP_LEN	3	/* Fixed length */

/** WNM-Sleep Management Request frame header */
BWL_PRE_PACKED_STRUCT struct dot11_wnm_sleep_req {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: wnm-sleep request (16) */
	uint8 token;				/* dialog token */
	uint8 data[1];				/* Elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_wnm_sleep_req dot11_wnm_sleep_req_t;
#define DOT11_WNM_SLEEP_REQ_LEN		3	/* Fixed length */

/** WNM-Sleep Management Response frame header */
BWL_PRE_PACKED_STRUCT struct dot11_wnm_sleep_resp {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: wnm-sleep request (17) */
	uint8 token;				/* dialog token */
	uint16 key_len;				/* key data length */
	uint8 data[1];				/* Elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_wnm_sleep_resp dot11_wnm_sleep_resp_t;
#define DOT11_WNM_SLEEP_RESP_LEN	5	/* Fixed length */

#define DOT11_WNM_SLEEP_SUBELEM_ID_GTK	0
#define DOT11_WNM_SLEEP_SUBELEM_ID_IGTK	1

BWL_PRE_PACKED_STRUCT struct dot11_wnm_sleep_subelem_gtk {
	uint8 sub_id;
	uint8 len;
	uint16 key_info;
	uint8 key_length;
	uint8 rsc[8];
	uint8 key[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_wnm_sleep_subelem_gtk dot11_wnm_sleep_subelem_gtk_t;
#define DOT11_WNM_SLEEP_SUBELEM_GTK_FIXED_LEN	11	/* without sub_id, len, and key */
#define DOT11_WNM_SLEEP_SUBELEM_GTK_MAX_LEN	43	/* without sub_id and len */

BWL_PRE_PACKED_STRUCT struct dot11_wnm_sleep_subelem_igtk {
	uint8 sub_id;
	uint8 len;
	uint16 key_id;
	uint8 pn[6];
	uint8 key[16];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_wnm_sleep_subelem_igtk dot11_wnm_sleep_subelem_igtk_t;
#define DOT11_WNM_SLEEP_SUBELEM_IGTK_LEN 24	/* Fixed length */

BWL_PRE_PACKED_STRUCT struct dot11_wnm_sleep_ie {
	uint8 id;				/* 93, DOT11_MNG_WNM_SLEEP_MODE_ID */
	uint8 len;
	uint8 act_type;
	uint8 resp_status;
	uint16 interval;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_wnm_sleep_ie dot11_wnm_sleep_ie_t;
#define DOT11_WNM_SLEEP_IE_LEN		4	/* Fixed length */

#define DOT11_WNM_SLEEP_ACT_TYPE_ENTER	0
#define DOT11_WNM_SLEEP_ACT_TYPE_EXIT	1

#define DOT11_WNM_SLEEP_RESP_ACCEPT	0
#define DOT11_WNM_SLEEP_RESP_UPDATE	1
#define DOT11_WNM_SLEEP_RESP_DENY	2
#define DOT11_WNM_SLEEP_RESP_DENY_TEMP	3
#define DOT11_WNM_SLEEP_RESP_DENY_KEY	4
#define DOT11_WNM_SLEEP_RESP_DENY_INUSE	5
#define DOT11_WNM_SLEEP_RESP_LAST	6

/** DMS Management Request frame header */
BWL_PRE_PACKED_STRUCT struct dot11_dms_req {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: dms request (23) */
	uint8 token;				/* dialog token */
	uint8 data[1];				/* Elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_dms_req dot11_dms_req_t;
#define DOT11_DMS_REQ_LEN		3	/* Fixed length */

/** DMS Management Response frame header */
BWL_PRE_PACKED_STRUCT struct dot11_dms_resp {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: dms request (24) */
	uint8 token;				/* dialog token */
	uint8 data[1];				/* Elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_dms_resp dot11_dms_resp_t;
#define DOT11_DMS_RESP_LEN		3	/* Fixed length */

/** DMS request element */
BWL_PRE_PACKED_STRUCT struct dot11_dms_req_ie {
	uint8 id;				/* 99, DOT11_MNG_DMS_REQUEST_ID */
	uint8 len;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_dms_req_ie dot11_dms_req_ie_t;
#define DOT11_DMS_REQ_IE_LEN		2	/* Fixed length */

/** DMS response element */
BWL_PRE_PACKED_STRUCT struct dot11_dms_resp_ie {
	uint8 id;				/* 100, DOT11_MNG_DMS_RESPONSE_ID */
	uint8 len;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_dms_resp_ie dot11_dms_resp_ie_t;
#define DOT11_DMS_RESP_IE_LEN		2	/* Fixed length */

/** DMS request descriptor */
BWL_PRE_PACKED_STRUCT struct dot11_dms_req_desc {
	uint8 dms_id;
	uint8 len;
	uint8 type;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_dms_req_desc dot11_dms_req_desc_t;
#define DOT11_DMS_REQ_DESC_LEN		3	/* Fixed length */

#define DOT11_DMS_REQ_TYPE_ADD		0
#define DOT11_DMS_REQ_TYPE_REMOVE	1
#define DOT11_DMS_REQ_TYPE_CHANGE	2

/** DMS response status */
BWL_PRE_PACKED_STRUCT struct dot11_dms_resp_st {
	uint8 dms_id;
	uint8 len;
	uint8 type;
	uint16 lsc;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_dms_resp_st dot11_dms_resp_st_t;
#define DOT11_DMS_RESP_STATUS_LEN	5	/* Fixed length */

#define DOT11_DMS_RESP_TYPE_ACCEPT	0
#define DOT11_DMS_RESP_TYPE_DENY	1
#define DOT11_DMS_RESP_TYPE_TERM	2

#define DOT11_DMS_RESP_LSC_UNSUPPORTED	0xFFFF

/** WNM-Notification Request frame header */
BWL_PRE_PACKED_STRUCT struct dot11_wnm_notif_req {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: Notification request (26) */
	uint8 token;				/* dialog token */
	uint8 type;				   /* type */
	uint8 data[1];				/* Sub-elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_wnm_notif_req dot11_wnm_notif_req_t;
#define DOT11_WNM_NOTIF_REQ_LEN		4	/* Fixed length */

/** WNM-Notification Response frame header */
BWL_PRE_PACKED_STRUCT struct dot11_wnm_notif_resp {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: Notification request (26) */
	uint8 token;				/* dialog token same from wnm notification rqst */
	uint8 status;				/* response status */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_wnm_notif_resp dot11_wnm_notif_resp_t;
#define DOT11_WNM_NOTIF_RESP_LEN		4	/* Fixed length */

/** FMS Management Request frame header */
BWL_PRE_PACKED_STRUCT struct dot11_fms_req {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: fms request (9) */
	uint8 token;				/* dialog token */
	uint8 data[1];				/* Elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_fms_req dot11_fms_req_t;
#define DOT11_FMS_REQ_LEN		3	/* Fixed length */

/** FMS Management Response frame header */
BWL_PRE_PACKED_STRUCT struct dot11_fms_resp {
	uint8 category;				/* category of action frame (10) */
	uint8 action;				/* WNM action: fms request (10) */
	uint8 token;				/* dialog token */
	uint8 data[1];				/* Elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_fms_resp dot11_fms_resp_t;
#define DOT11_FMS_RESP_LEN		3	/* Fixed length */

/** FMS Descriptor element */
BWL_PRE_PACKED_STRUCT struct dot11_fms_desc {
	uint8 id;
	uint8 len;
	uint8 num_fms_cnt;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_fms_desc dot11_fms_desc_t;
#define DOT11_FMS_DESC_LEN		1	/* Fixed length */

#define DOT11_FMS_CNTR_MAX		0x8
#define DOT11_FMS_CNTR_ID_MASK		0x7
#define DOT11_FMS_CNTR_ID_SHIFT		0x0
#define DOT11_FMS_CNTR_COUNT_MASK	0xf1
#define DOT11_FMS_CNTR_SHIFT		0x3

/** FMS request element */
BWL_PRE_PACKED_STRUCT struct dot11_fms_req_ie {
	uint8 id;
	uint8 len;
	uint8 fms_token;			/* token used to identify fms stream set */
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_fms_req_ie dot11_fms_req_ie_t;
#define DOT11_FMS_REQ_IE_FIX_LEN		1	/* Fixed length */

BWL_PRE_PACKED_STRUCT struct dot11_rate_id_field {
	uint8 mask;
	uint8 mcs_idx;
	uint16 rate;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rate_id_field dot11_rate_id_field_t;
#define DOT11_RATE_ID_FIELD_MCS_SEL_MASK	0x7
#define DOT11_RATE_ID_FIELD_MCS_SEL_OFFSET	0
#define DOT11_RATE_ID_FIELD_RATETYPE_MASK	0x18
#define DOT11_RATE_ID_FIELD_RATETYPE_OFFSET	3
#define DOT11_RATE_ID_FIELD_LEN		sizeof(dot11_rate_id_field_t)

/** FMS request subelements */
BWL_PRE_PACKED_STRUCT struct dot11_fms_se {
	uint8 sub_id;
	uint8 len;
	uint8 interval;
	uint8 max_interval;
	dot11_rate_id_field_t rate;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_fms_se dot11_fms_se_t;
#define DOT11_FMS_REQ_SE_LEN		6	/* Fixed length */

#define DOT11_FMS_REQ_SE_ID_FMS		1	/* FMS subelement */
#define DOT11_FMS_REQ_SE_ID_VS		221	/* Vendor Specific subelement */

/** FMS response element */
BWL_PRE_PACKED_STRUCT struct dot11_fms_resp_ie {
	uint8 id;
	uint8 len;
	uint8 fms_token;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_fms_resp_ie dot11_fms_resp_ie_t;
#define DOT11_FMS_RESP_IE_FIX_LEN		1	/* Fixed length */

/* FMS status subelements */
#define DOT11_FMS_STATUS_SE_ID_FMS	1	/* FMS Status */
#define DOT11_FMS_STATUS_SE_ID_TCLAS	2	/* TCLAS Status */
#define DOT11_FMS_STATUS_SE_ID_VS	221	/* Vendor Specific subelement */

/** FMS status subelement */
BWL_PRE_PACKED_STRUCT struct dot11_fms_status_se {
	uint8 sub_id;
	uint8 len;
	uint8 status;
	uint8 interval;
	uint8 max_interval;
	uint8 fmsid;
	uint8 counter;
	dot11_rate_id_field_t rate;
	uint8 mcast_addr[ETHER_ADDR_LEN];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_fms_status_se dot11_fms_status_se_t;
#define DOT11_FMS_STATUS_SE_LEN		15	/* Fixed length */

/** TCLAS status subelement */
BWL_PRE_PACKED_STRUCT struct dot11_tclas_status_se {
	uint8 sub_id;
	uint8 len;
	uint8 fmsid;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tclas_status_se dot11_tclas_status_se_t;
#define DOT11_TCLAS_STATUS_SE_LEN		1	/* Fixed length */

BWL_PRE_PACKED_STRUCT struct dot11_addba_req {
	uint8 category;				/* category of action frame (3) */
	uint8 action;				/* action: addba req */
	uint8 token;				/* identifier */
	uint16 addba_param_set;		/* parameter set */
	uint16 timeout;				/* timeout in seconds */
	uint16 start_seqnum;		/* starting sequence number */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_addba_req dot11_addba_req_t;
#define DOT11_ADDBA_REQ_LEN		9	/* length of addba req frame */

BWL_PRE_PACKED_STRUCT struct dot11_addba_resp {
	uint8 category;				/* category of action frame (3) */
	uint8 action;				/* action: addba resp */
	uint8 token;				/* identifier */
	uint16 status;				/* status of add request */
	uint16 addba_param_set;			/* negotiated parameter set */
	uint16 timeout;				/* negotiated timeout in seconds */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_addba_resp dot11_addba_resp_t;
#define DOT11_ADDBA_RESP_LEN		9	/* length of addba resp frame */

/* DELBA action parameters */
#define DOT11_DELBA_PARAM_INIT_MASK	0x0800	/* initiator mask */
#define DOT11_DELBA_PARAM_INIT_SHIFT	11	/* initiator shift */
#define DOT11_DELBA_PARAM_TID_MASK	0xf000	/* tid mask */
#define DOT11_DELBA_PARAM_TID_SHIFT	12	/* tid shift */

BWL_PRE_PACKED_STRUCT struct dot11_delba {
	uint8 category;				/* category of action frame (3) */
	uint8 action;				/* action: addba req */
	uint16 delba_param_set;			/* paarmeter set */
	uint16 reason;				/* reason for dellba */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_delba dot11_delba_t;
#define DOT11_DELBA_LEN			6	/* length of delba frame */

/* SA Query action field value */
#define SA_QUERY_REQUEST		0
#define SA_QUERY_RESPONSE		1

/* ************* 802.11r related definitions. ************* */

/** Over-the-DS Fast Transition Request frame header */
BWL_PRE_PACKED_STRUCT struct dot11_ft_req {
	uint8 category;			/* category of action frame (6) */
	uint8 action;			/* action: ft req */
	uint8 sta_addr[ETHER_ADDR_LEN];
	uint8 tgt_ap_addr[ETHER_ADDR_LEN];
	uint8 data[1];			/* Elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ft_req dot11_ft_req_t;
#define DOT11_FT_REQ_FIXED_LEN 14

/** Over-the-DS Fast Transition Response frame header */
BWL_PRE_PACKED_STRUCT struct dot11_ft_res {
	uint8 category;			/* category of action frame (6) */
	uint8 action;			/* action: ft resp */
	uint8 sta_addr[ETHER_ADDR_LEN];
	uint8 tgt_ap_addr[ETHER_ADDR_LEN];
	uint16 status;			/* status code */
	uint8 data[1];			/* Elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ft_res dot11_ft_res_t;
#define DOT11_FT_RES_FIXED_LEN 16

/** RDE RIC Data Element. */
BWL_PRE_PACKED_STRUCT struct dot11_rde_ie {
	uint8 id;			/* 11r, DOT11_MNG_RDE_ID */
	uint8 length;
	uint8 rde_id;			/* RDE identifier. */
	uint8 rd_count;			/* Resource Descriptor Count. */
	uint16 status;			/* Status Code. */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rde_ie dot11_rde_ie_t;

/* 11r - Size of the RDE (RIC Data Element) IE, including TLV header. */
#define DOT11_MNG_RDE_IE_LEN sizeof(dot11_rde_ie_t)

/* ************* 802.11k related definitions. ************* */

/* Radio measurements enabled capability ie */
#define DOT11_RRM_CAP_LEN		5	/* length of rrm cap bitmap */
#define RCPI_IE_LEN 1
#define RSNI_IE_LEN 1
BWL_PRE_PACKED_STRUCT struct dot11_rrm_cap_ie {
	uint8 cap[DOT11_RRM_CAP_LEN];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rrm_cap_ie dot11_rrm_cap_ie_t;

/* Bitmap definitions for cap ie */
#define DOT11_RRM_CAP_LINK		0
#define DOT11_RRM_CAP_NEIGHBOR_REPORT	1
#define DOT11_RRM_CAP_PARALLEL		2
#define DOT11_RRM_CAP_REPEATED		3
#define DOT11_RRM_CAP_BCN_PASSIVE	4
#define DOT11_RRM_CAP_BCN_ACTIVE	5
#define DOT11_RRM_CAP_BCN_TABLE		6
#define DOT11_RRM_CAP_BCN_REP_COND	7
#define DOT11_RRM_CAP_FM		8
#define DOT11_RRM_CAP_CLM		9
#define DOT11_RRM_CAP_NHM		10
#define DOT11_RRM_CAP_SM		11
#define DOT11_RRM_CAP_LCIM		12
#define DOT11_RRM_CAP_LCIA		13
#define DOT11_RRM_CAP_TSCM		14
#define DOT11_RRM_CAP_TTSCM		15
#define DOT11_RRM_CAP_AP_CHANREP	16
#define DOT11_RRM_CAP_RMMIB		17
/* bit18-bit23, not used for RRM_IOVAR */
#define DOT11_RRM_CAP_MPC0		24
#define DOT11_RRM_CAP_MPC1		25
#define DOT11_RRM_CAP_MPC2		26
#define DOT11_RRM_CAP_MPTI		27
#define DOT11_RRM_CAP_NBRTSFO		28
#define DOT11_RRM_CAP_RCPI		29
#define DOT11_RRM_CAP_RSNI		30
#define DOT11_RRM_CAP_BSSAAD		31
#define DOT11_RRM_CAP_BSSAAC		32
#define DOT11_RRM_CAP_AI		33
#define DOT11_RRM_CAP_FTM_RANGE		34
#define DOT11_RRM_CAP_CIVIC_LOC		35
#define DOT11_RRM_CAP_IDENT_LOC		36
#define DOT11_RRM_CAP_LAST		36

#ifdef WL11K_ALL_MEAS
#define DOT11_RRM_CAP_LINK_ENAB			(1 << DOT11_RRM_CAP_LINK)
#define DOT11_RRM_CAP_FM_ENAB			(1 << (DOT11_RRM_CAP_FM - 8))
#define DOT11_RRM_CAP_CLM_ENAB			(1 << (DOT11_RRM_CAP_CLM - 8))
#define DOT11_RRM_CAP_NHM_ENAB			(1 << (DOT11_RRM_CAP_NHM - 8))
#define DOT11_RRM_CAP_SM_ENAB			(1 << (DOT11_RRM_CAP_SM - 8))
#define DOT11_RRM_CAP_LCIM_ENAB			(1 << (DOT11_RRM_CAP_LCIM - 8))
#define DOT11_RRM_CAP_TSCM_ENAB			(1 << (DOT11_RRM_CAP_TSCM - 8))
#ifdef WL11K_AP
#define DOT11_RRM_CAP_MPC0_ENAB			(1 << (DOT11_RRM_CAP_MPC0 - 24))
#define DOT11_RRM_CAP_MPC1_ENAB			(1 << (DOT11_RRM_CAP_MPC1 - 24))
#define DOT11_RRM_CAP_MPC2_ENAB			(1 << (DOT11_RRM_CAP_MPC2 - 24))
#define DOT11_RRM_CAP_MPTI_ENAB			(1 << (DOT11_RRM_CAP_MPTI - 24))
#else
#define DOT11_RRM_CAP_MPC0_ENAB			0
#define DOT11_RRM_CAP_MPC1_ENAB			0
#define DOT11_RRM_CAP_MPC2_ENAB			0
#define DOT11_RRM_CAP_MPTI_ENAB			0
#endif /* WL11K_AP */
#define DOT11_RRM_CAP_CIVIC_LOC_ENAB		(1 << (DOT11_RRM_CAP_CIVIC_LOC - 32))
#define DOT11_RRM_CAP_IDENT_LOC_ENAB		(1 << (DOT11_RRM_CAP_IDENT_LOC - 32))
#define DOT11_RRM_CAP_FTM_RANGE_ENAB		(1 << (DOT11_RRM_CAP_FTM_RANGE - 32))
#else
#define DOT11_RRM_CAP_LINK_ENAB			0
#define DOT11_RRM_CAP_FM_ENAB			0
#define DOT11_RRM_CAP_CLM_ENAB			0
#define DOT11_RRM_CAP_NHM_ENAB			0
#define DOT11_RRM_CAP_SM_ENAB			0
#define DOT11_RRM_CAP_LCIM_ENAB			0
#define DOT11_RRM_CAP_TSCM_ENAB			0
#define DOT11_RRM_CAP_MPC0_ENAB			0
#define DOT11_RRM_CAP_MPC1_ENAB			0
#define DOT11_RRM_CAP_MPC2_ENAB			0
#define DOT11_RRM_CAP_MPTI_ENAB			0
#define DOT11_RRM_CAP_CIVIC_LOC_ENAB		0
#define DOT11_RRM_CAP_IDENT_LOC_ENAB		0
#define DOT11_RRM_CAP_FTM_RANGE_ENAB		0
#endif /* WL11K_ALL_MEAS */
#ifdef WL11K_NBR_MEAS
#define DOT11_RRM_CAP_NEIGHBOR_REPORT_ENAB	(1 << DOT11_RRM_CAP_NEIGHBOR_REPORT)
#else
#define DOT11_RRM_CAP_NEIGHBOR_REPORT_ENAB	0
#endif /* WL11K_NBR_MEAS */
#ifdef WL11K_BCN_MEAS
#define DOT11_RRM_CAP_BCN_PASSIVE_ENAB		(1 << DOT11_RRM_CAP_BCN_PASSIVE)
#define DOT11_RRM_CAP_BCN_ACTIVE_ENAB		(1 << DOT11_RRM_CAP_BCN_ACTIVE)
#define DOT11_RRM_CAP_BCN_TABLE_ENAB		(1 << DOT11_RRM_CAP_BCN_TABLE)
#else
#define DOT11_RRM_CAP_BCN_PASSIVE_ENAB		0
#define DOT11_RRM_CAP_BCN_ACTIVE_ENAB		0
#define DOT11_RRM_CAP_BCN_TABLE_ENAB		0
#endif /* WL11K_BCN_MEAS */
#define DOT11_RRM_CAP_MPA_MASK		0x7
/* Operating Class (formerly "Regulatory Class") definitions */
#define DOT11_OP_CLASS_NONE			255

BWL_PRE_PACKED_STRUCT struct do11_ap_chrep {
	uint8 id;
	uint8 len;
	uint8 reg;
	uint8 chanlist[1];
} BWL_POST_PACKED_STRUCT;
typedef struct do11_ap_chrep dot11_ap_chrep_t;

/* Radio Measurements action ids */
#define DOT11_RM_ACTION_RM_REQ		0	/* Radio measurement request */
#define DOT11_RM_ACTION_RM_REP		1	/* Radio measurement report */
#define DOT11_RM_ACTION_LM_REQ		2	/* Link measurement request */
#define DOT11_RM_ACTION_LM_REP		3	/* Link measurement report */
#define DOT11_RM_ACTION_NR_REQ		4	/* Neighbor report request */
#define DOT11_RM_ACTION_NR_REP		5	/* Neighbor report response */
#define DOT11_PUB_ACTION_MP		7	/* Measurement Pilot public action id */

/** Generic radio measurement action frame header */
BWL_PRE_PACKED_STRUCT struct dot11_rm_action {
	uint8 category;				/* category of action frame (5) */
	uint8 action;				/* radio measurement action */
	uint8 token;				/* dialog token */
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rm_action dot11_rm_action_t;
#define DOT11_RM_ACTION_LEN 3

BWL_PRE_PACKED_STRUCT struct dot11_rmreq {
	uint8 category;				/* category of action frame (5) */
	uint8 action;				/* radio measurement action */
	uint8 token;				/* dialog token */
	uint16 reps;				/* no. of repetitions */
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmreq dot11_rmreq_t;
#define DOT11_RMREQ_LEN	5

BWL_PRE_PACKED_STRUCT struct dot11_rm_ie {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rm_ie dot11_rm_ie_t;
#define DOT11_RM_IE_LEN	5

/* Definitions for "mode" bits in rm req */
#define DOT11_RMREQ_MODE_PARALLEL	1
#define DOT11_RMREQ_MODE_ENABLE		2
#define DOT11_RMREQ_MODE_REQUEST	4
#define DOT11_RMREQ_MODE_REPORT		8
#define DOT11_RMREQ_MODE_DURMAND	0x10	/* Duration Mandatory */

/* Definitions for "mode" bits in rm rep */
#define DOT11_RMREP_MODE_LATE		1
#define DOT11_RMREP_MODE_INCAPABLE	2
#define DOT11_RMREP_MODE_REFUSED	4

BWL_PRE_PACKED_STRUCT struct dot11_rmreq_bcn {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint8 reg;
	uint8 channel;
	uint16 interval;
	uint16 duration;
	uint8 bcn_mode;
	struct ether_addr	bssid;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmreq_bcn dot11_rmreq_bcn_t;
#define DOT11_RMREQ_BCN_LEN	18

BWL_PRE_PACKED_STRUCT struct dot11_rmrep_bcn {
	uint8 reg;
	uint8 channel;
	uint32 starttime[2];
	uint16 duration;
	uint8 frame_info;
	uint8 rcpi;
	uint8 rsni;
	struct ether_addr	bssid;
	uint8 antenna_id;
	uint32 parent_tsf;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmrep_bcn dot11_rmrep_bcn_t;
#define DOT11_RMREP_BCN_LEN	26

/* Beacon request measurement mode */
#define DOT11_RMREQ_BCN_PASSIVE	0
#define DOT11_RMREQ_BCN_ACTIVE	1
#define DOT11_RMREQ_BCN_TABLE	2

/* Sub-element IDs for Beacon Request */
#define DOT11_RMREQ_BCN_SSID_ID		0
#define DOT11_RMREQ_BCN_REPINFO_ID	1
#define DOT11_RMREQ_BCN_REPDET_ID	2
#define DOT11_RMREQ_BCN_REQUEST_ID	10
#define DOT11_RMREQ_BCN_REQUEST_EXT_ID  11
#define DOT11_RMREQ_BCN_LAST_RPT_IND_REQ_ID	164
#define DOT11_RMREQ_BCN_LAST_RPT_IND_REQ_LEN	1
#define DOT11_RMREQ_BCN_APCHREP_ID  DOT11_MNG_AP_CHREP_ID

/* Reporting Detail element definition */
#define DOT11_RMREQ_BCN_REPDET_FIXED	0	/* Fixed length fields only */
#define DOT11_RMREQ_BCN_REPDET_REQUEST	1	/* + requested information elems */
#define DOT11_RMREQ_BCN_REPDET_ALL	2	/* All fields */

/* Last Beacon Report Indication Request definition */
#define DOT11_RMREQ_BCN_LAST_RPT_IND_REQ_ENAB  1

BWL_PRE_PACKED_STRUCT struct dot11_rmrep_last_bcn_rpt_ind_req {
	uint8 id;                       /* DOT11_RMREQ_BCN_LAST_RPT_IND_REQ_ID */
	uint8 len;                      /* length of remaining fields */
	uint8 data;                     /* data = 1 means last bcn rpt ind requested */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmrep_last_bcn_rpt_ind_req dot11_rmrep_last_bcn_rpt_ind_req_t;

/* Reporting Information (reporting condition) element definition */
#define DOT11_RMREQ_BCN_REPINFO_LEN	2	/* Beacon Reporting Information length */
#define DOT11_RMREQ_BCN_REPCOND_DEFAULT	0	/* Report to be issued after each measurement */

/* Sub-element IDs for Beacon Report */
#define DOT11_RMREP_BCN_FRM_BODY	1
#define DOT11_RMREP_BCN_FRAG_IND	2
#define DOT11_RMREP_BCN_LAST_RPT_IND 164
#define DOT11_RMREP_BCN_FRM_BODY_LEN_MAX	224 /* 802.11k-2008 7.3.2.22.6 */

BWL_PRE_PACKED_STRUCT struct dot11_rmrep_last_bcn_rpt_ind {
	uint8 id;                       /* DOT11_RMREP_BCN_LAST_RPT_IND */
	uint8 len;                      /* length of remaining fields */
	uint8 data;                     /* data = 1 is last bcn rpt */
} BWL_POST_PACKED_STRUCT;

typedef struct dot11_rmrep_last_bcn_rpt_ind dot11_rmrep_last_bcn_rpt_ind_t;
#define DOT11_RMREP_LAST_BCN_RPT_IND_DATA_LEN 1
#define DOT11_RMREP_LAST_BCN_RPT_IND_SE_LEN sizeof(dot11_rmrep_last_bcn_rpt_ind_t)

/* Sub-element IDs for Frame Report */
#define DOT11_RMREP_FRAME_COUNT_REPORT 1

/* Channel load request */
BWL_PRE_PACKED_STRUCT struct dot11_rmreq_chanload {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint8 reg;
	uint8 channel;
	uint16 interval;
	uint16 duration;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmreq_chanload dot11_rmreq_chanload_t;
#define DOT11_RMREQ_CHANLOAD_LEN	11

/** Channel load report */
BWL_PRE_PACKED_STRUCT struct dot11_rmrep_chanload {
	uint8 reg;
	uint8 channel;
	uint32 starttime[2];
	uint16 duration;
	uint8 channel_load;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmrep_chanload dot11_rmrep_chanload_t;
#define DOT11_RMREP_CHANLOAD_LEN	13

/** Noise histogram request */
BWL_PRE_PACKED_STRUCT struct dot11_rmreq_noise {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint8 reg;
	uint8 channel;
	uint16 interval;
	uint16 duration;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmreq_noise dot11_rmreq_noise_t;
#define DOT11_RMREQ_NOISE_LEN 11

/** Noise histogram report */
BWL_PRE_PACKED_STRUCT struct dot11_rmrep_noise {
	uint8 reg;
	uint8 channel;
	uint32 starttime[2];
	uint16 duration;
	uint8 antid;
	uint8 anpi;
	uint8 ipi0_dens;
	uint8 ipi1_dens;
	uint8 ipi2_dens;
	uint8 ipi3_dens;
	uint8 ipi4_dens;
	uint8 ipi5_dens;
	uint8 ipi6_dens;
	uint8 ipi7_dens;
	uint8 ipi8_dens;
	uint8 ipi9_dens;
	uint8 ipi10_dens;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmrep_noise dot11_rmrep_noise_t;
#define DOT11_RMREP_NOISE_LEN 25

/** Frame request */
BWL_PRE_PACKED_STRUCT struct dot11_rmreq_frame {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint8 reg;
	uint8 channel;
	uint16 interval;
	uint16 duration;
	uint8 req_type;
	struct ether_addr	ta;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmreq_frame dot11_rmreq_frame_t;
#define DOT11_RMREQ_FRAME_LEN 18

/** Frame report */
BWL_PRE_PACKED_STRUCT struct dot11_rmrep_frame {
	uint8 reg;
	uint8 channel;
	uint32 starttime[2];
	uint16 duration;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmrep_frame dot11_rmrep_frame_t;
#define DOT11_RMREP_FRAME_LEN 12

/** Frame report entry */
BWL_PRE_PACKED_STRUCT struct dot11_rmrep_frmentry {
	struct ether_addr	ta;
	struct ether_addr	bssid;
	uint8 phy_type;
	uint8 avg_rcpi;
	uint8 last_rsni;
	uint8 last_rcpi;
	uint8 ant_id;
	uint16 frame_cnt;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmrep_frmentry dot11_rmrep_frmentry_t;
#define DOT11_RMREP_FRMENTRY_LEN 19

/** STA statistics request */
BWL_PRE_PACKED_STRUCT struct dot11_rmreq_stat {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	struct ether_addr	peer;
	uint16 interval;
	uint16 duration;
	uint8 group_id;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmreq_stat dot11_rmreq_stat_t;
#define DOT11_RMREQ_STAT_LEN 16

/** STA statistics report */
BWL_PRE_PACKED_STRUCT struct dot11_rmrep_stat {
	uint16 duration;
	uint8 group_id;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmrep_stat dot11_rmrep_stat_t;

/* Statistics Group Report: Group IDs */
enum {
	DOT11_RRM_STATS_GRP_ID_0 = 0,
	DOT11_RRM_STATS_GRP_ID_1,
	DOT11_RRM_STATS_GRP_ID_2,
	DOT11_RRM_STATS_GRP_ID_3,
	DOT11_RRM_STATS_GRP_ID_4,
	DOT11_RRM_STATS_GRP_ID_5,
	DOT11_RRM_STATS_GRP_ID_6,
	DOT11_RRM_STATS_GRP_ID_7,
	DOT11_RRM_STATS_GRP_ID_8,
	DOT11_RRM_STATS_GRP_ID_9,
	DOT11_RRM_STATS_GRP_ID_10,
	DOT11_RRM_STATS_GRP_ID_11,
	DOT11_RRM_STATS_GRP_ID_12,
	DOT11_RRM_STATS_GRP_ID_13,
	DOT11_RRM_STATS_GRP_ID_14,
	DOT11_RRM_STATS_GRP_ID_15,
	DOT11_RRM_STATS_GRP_ID_16
};

/* Statistics Group Report: Group Data length  */
#define DOT11_RRM_STATS_RPT_LEN_GRP_ID_0	28
typedef struct rrm_stat_group_0 {
	uint32	txfrag;
	uint32	txmulti;
	uint32	txfail;
	uint32	rxframe;
	uint32	rxmulti;
	uint32	rxbadfcs;
	uint32	txframe;
} rrm_stat_group_0_t;

#define DOT11_RRM_STATS_RPT_LEN_GRP_ID_1	24
typedef struct rrm_stat_group_1 {
	uint32	txretry;
	uint32	txretries;
	uint32	rxdup;
	uint32	txrts;
	uint32	rtsfail;
	uint32	ackfail;
} rrm_stat_group_1_t;

/* group 2-9 use same qos data structure (tid 0-7), total 52 bytes */
#define DOT11_RRM_STATS_RPT_LEN_GRP_ID_2_9	52
typedef struct rrm_stat_group_qos {
	uint32	txfrag;
	uint32	txfail;
	uint32	txretry;
	uint32	txretries;
	uint32	rxdup;
	uint32	txrts;
	uint32	rtsfail;
	uint32	ackfail;
	uint32	rxfrag;
	uint32	txframe;
	uint32	txdrop;
	uint32	rxmpdu;
	uint32	rxretries;
} rrm_stat_group_qos_t;

/* dot11BSSAverageAccessDelay Group (only available at an AP): 8 byte */
#define DOT11_RRM_STATS_RPT_LEN_GRP_ID_10	8
typedef BWL_PRE_PACKED_STRUCT struct rrm_stat_group_10 {
	uint8	apavgdelay;
	uint8	avgdelaybe;
	uint8	avgdelaybg;
	uint8	avgdelayvi;
	uint8	avgdelayvo;
	uint16	stacount;
	uint8	chanutil;
} BWL_POST_PACKED_STRUCT rrm_stat_group_10_t;

/* AMSDU, 40 bytes */
#define DOT11_RRM_STATS_RPT_LEN_GRP_ID_11	40
typedef struct rrm_stat_group_11 {
	uint32	txamsdu;
	uint32	amsdufail;
	uint32	amsduretry;
	uint32	amsduretries;
	uint32	txamsdubyte_h;
	uint32	txamsdubyte_l;
	uint32	amsduackfail;
	uint32	rxamsdu;
	uint32	rxamsdubyte_h;
	uint32	rxamsdubyte_l;
} rrm_stat_group_11_t;

/* AMPDU, 36 bytes */
#define DOT11_RRM_STATS_RPT_LEN_GRP_ID_12	36
typedef struct rrm_stat_group_12 {
	uint32	txampdu;
	uint32	txmpdu;
	uint32	txampdubyte_h;
	uint32	txampdubyte_l;
	uint32	rxampdu;
	uint32	rxmpdu;
	uint32	rxampdubyte_h;
	uint32	rxampdubyte_l;
	uint32	ampducrcfail;
} rrm_stat_group_12_t;

/* BACK etc, 36 bytes */
#define DOT11_RRM_STATS_RPT_LEN_GRP_ID_13	36
typedef struct rrm_stat_group_13 {
	uint32	rximpbarfail;
	uint32	rxexpbarfail;
	uint32	chanwidthsw;
	uint32	txframe20mhz;
	uint32	txframe40mhz;
	uint32	rxframe20mhz;
	uint32	rxframe40mhz;
	uint32	psmpgrantdur;
	uint32	psmpuseddur;
} rrm_stat_group_13_t;

/* RD Dual CTS etc, 36 bytes */
#define DOT11_RRM_STATS_RPT_LEN_GRP_ID_14	36
typedef struct rrm_stat_group_14 {
	uint32	grantrdgused;
	uint32	grantrdgunused;
	uint32	txframeingrantrdg;
	uint32	txbyteingrantrdg_h;
	uint32	txbyteingrantrdg_l;
	uint32	dualcts;
	uint32	dualctsfail;
	uint32	rtslsi;
	uint32	rtslsifail;
} rrm_stat_group_14_t;

/* bf and STBC etc, 20 bytes */
#define DOT11_RRM_STATS_RPT_LEN_GRP_ID_15	20
typedef struct rrm_stat_group_15 {
	uint32	bfframe;
	uint32	stbccts;
	uint32	stbcctsfail;
	uint32	nonstbccts;
	uint32	nonstbcctsfail;
} rrm_stat_group_15_t;

/* RSNA, 28 bytes */
#define DOT11_RRM_STATS_RPT_LEN_GRP_ID_16	28
typedef struct rrm_stat_group_16 {
	uint32	rsnacmacicverr;
	uint32	rsnacmacreplay;
	uint32	rsnarobustmgmtccmpreplay;
	uint32	rsnatkipicverr;
	uint32	rsnatkipicvreplay;
	uint32	rsnaccmpdecrypterr;
	uint32	rsnaccmpreplay;
} rrm_stat_group_16_t;

/* Transmit stream/category measurement request */
BWL_PRE_PACKED_STRUCT struct dot11_rmreq_tx_stream {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint16 interval;
	uint16 duration;
	struct ether_addr	peer;
	uint8 traffic_id;
	uint8 bin0_range;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmreq_tx_stream dot11_rmreq_tx_stream_t;
#define DOT11_RMREQ_TXSTREAM_LEN	17

/** Transmit stream/category measurement report */
BWL_PRE_PACKED_STRUCT struct dot11_rmrep_tx_stream {
	uint32 starttime[2];
	uint16 duration;
	struct ether_addr	peer;
	uint8 traffic_id;
	uint8 reason;
	uint32 txmsdu_cnt;
	uint32 msdu_discarded_cnt;
	uint32 msdufailed_cnt;
	uint32 msduretry_cnt;
	uint32 cfpolls_lost_cnt;
	uint32 avrqueue_delay;
	uint32 avrtx_delay;
	uint8 bin0_range;
	uint32 bin0;
	uint32 bin1;
	uint32 bin2;
	uint32 bin3;
	uint32 bin4;
	uint32 bin5;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmrep_tx_stream dot11_rmrep_tx_stream_t;
#define DOT11_RMREP_TXSTREAM_LEN	71

typedef struct rrm_tscm {
	uint32 msdu_tx;
	uint32 msdu_exp;
	uint32 msdu_fail;
	uint32 msdu_retries;
	uint32 cfpolls_lost;
	uint32 queue_delay;
	uint32 tx_delay_sum;
	uint32 tx_delay_cnt;
	uint32 bin0_range_us;
	uint32 bin0;
	uint32 bin1;
	uint32 bin2;
	uint32 bin3;
	uint32 bin4;
	uint32 bin5;
} rrm_tscm_t;
enum {
	DOT11_FTM_LOCATION_SUBJ_LOCAL = 0, 		/* Where am I? */
	DOT11_FTM_LOCATION_SUBJ_REMOTE = 1,		/* Where are you? */
	DOT11_FTM_LOCATION_SUBJ_THIRDPARTY = 2   /* Where is he/she? */
};

BWL_PRE_PACKED_STRUCT struct dot11_rmreq_ftm_lci {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint8 subj;

	/* Following 3 fields are unused. Keep for ROM compatibility. */
	uint8 lat_res;
	uint8 lon_res;
	uint8 alt_res;

	/* optional sub-elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmreq_ftm_lci dot11_rmreq_ftm_lci_t;
#define DOT11_RMREQ_LCI_LEN	9

BWL_PRE_PACKED_STRUCT struct dot11_rmrep_ftm_lci {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint8 lci_sub_id;
	uint8 lci_sub_len;
	/* optional LCI field */
	/* optional sub-elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmrep_ftm_lci dot11_rmrep_ftm_lci_t;

#define DOT11_FTM_LCI_SUBELEM_ID 		0
#define DOT11_FTM_LCI_SUBELEM_LEN 		2
#define DOT11_FTM_LCI_FIELD_LEN 		16
#define DOT11_FTM_LCI_UNKNOWN_LEN 		2

BWL_PRE_PACKED_STRUCT struct dot11_rmreq_ftm_civic {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint8 subj;
	uint8 civloc_type;
	uint8 siu;	/* service interval units */
	uint16 si;  /* service interval */
	/* optional sub-elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmreq_ftm_civic dot11_rmreq_ftm_civic_t;
#define DOT11_RMREQ_CIVIC_LEN	10

BWL_PRE_PACKED_STRUCT struct dot11_rmrep_ftm_civic {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint8 civloc_type;
	uint8 civloc_sub_id;
	uint8 civloc_sub_len;
	/* optional location civic field */
	/* optional sub-elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmrep_ftm_civic dot11_rmrep_ftm_civic_t;

#define DOT11_FTM_CIVIC_LOC_TYPE_RFC4776	0
#define DOT11_FTM_CIVIC_SUBELEM_ID 			0
#define DOT11_FTM_CIVIC_SUBELEM_LEN 		2
#define DOT11_FTM_CIVIC_LOC_SI_NONE			0
#define DOT11_FTM_CIVIC_TYPE_LEN			1
#define DOT11_FTM_CIVIC_UNKNOWN_LEN 		3

/* Location Identifier measurement request */
BWL_PRE_PACKED_STRUCT struct dot11_rmreq_locid {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint8 subj;
	uint8 siu;
	uint16 si;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmreq_locid dot11_rmreq_locid_t;
#define DOT11_RMREQ_LOCID_LEN	9

/* Location Identifier measurement report */
BWL_PRE_PACKED_STRUCT struct dot11_rmrep_locid {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint8 exp_tsf[8];
	uint8 locid_sub_id;
	uint8 locid_sub_len;
	/* optional location identifier field */
	/* optional sub-elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmrep_locid dot11_rmrep_locid_t;
#define DOT11_LOCID_UNKNOWN_LEN		10
#define DOT11_LOCID_SUBELEM_ID		0

BWL_PRE_PACKED_STRUCT struct dot11_ftm_range_subel {
	uint8 id;
	uint8 len;
	uint16 max_age;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ftm_range_subel dot11_ftm_range_subel_t;
#define DOT11_FTM_RANGE_SUBELEM_ID      4
#define DOT11_FTM_RANGE_SUBELEM_LEN     2

BWL_PRE_PACKED_STRUCT struct dot11_rmreq_ftm_range {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint16 max_init_delay;		/* maximum random initial delay */
	uint8 min_ap_count;
	uint8 data[1];
	/* neighbor report sub-elements */
	/* optional sub-elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmreq_ftm_range dot11_rmreq_ftm_range_t;
#define DOT11_RMREQ_FTM_RANGE_LEN 8

#define DOT11_FTM_RANGE_LEN		3
#define DOT11_FTM_RANGE_ERR_LEN		1
BWL_PRE_PACKED_STRUCT struct dot11_ftm_range_entry {
	uint32 start_tsf;		/* 4 lsb of tsf */
	struct ether_addr bssid;
	uint8 range[DOT11_FTM_RANGE_LEN];
	uint8 max_err[DOT11_FTM_RANGE_ERR_LEN];
	uint8  rsvd;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ftm_range_entry dot11_ftm_range_entry_t;
#define DOT11_FTM_RANGE_ENTRY_MAX_COUNT   15

enum {
	DOT11_FTM_RANGE_ERROR_AP_INCAPABLE = 2,
	DOT11_FTM_RANGE_ERROR_AP_FAILED = 3,
	DOT11_FTM_RANGE_ERROR_TX_FAILED = 8,
	DOT11_FTM_RANGE_ERROR_MAX
};

BWL_PRE_PACKED_STRUCT struct dot11_ftm_range_error_entry {
	uint32 start_tsf;		/* 4 lsb of tsf */
	struct ether_addr bssid;
	uint8  code;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ftm_range_error_entry dot11_ftm_range_error_entry_t;
#define DOT11_FTM_RANGE_ERROR_ENTRY_MAX_COUNT   11

BWL_PRE_PACKED_STRUCT struct dot11_rmrep_ftm_range {
    uint8 id;
    uint8 len;
    uint8 token;
    uint8 mode;
    uint8 type;
    uint8 entry_count;
    uint8 data[2]; /* includes pad */
	/*
	dot11_ftm_range_entry_t entries[entry_count];
	uint8 error_count;
	dot11_ftm_error_entry_t errors[error_count];
	 */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmrep_ftm_range dot11_rmrep_ftm_range_t;

#define DOT11_FTM_RANGE_REP_MIN_LEN     6       /* No extra byte for error_count */
#define DOT11_FTM_RANGE_ENTRY_CNT_MAX   15
#define DOT11_FTM_RANGE_ERROR_CNT_MAX   11
#define DOT11_FTM_RANGE_REP_FIXED_LEN   1       /* No extra byte for error_count */
/** Measurement pause request */
BWL_PRE_PACKED_STRUCT struct dot11_rmreq_pause_time {
	uint8 id;
	uint8 len;
	uint8 token;
	uint8 mode;
	uint8 type;
	uint16 pause_time;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_rmreq_pause_time dot11_rmreq_pause_time_t;
#define DOT11_RMREQ_PAUSE_LEN	7

/* Neighbor Report subelements ID (11k & 11v) */
#define DOT11_NGBR_TSF_INFO_SE_ID	1
#define DOT11_NGBR_CCS_SE_ID		2
#define DOT11_NGBR_BSSTRANS_PREF_SE_ID	3
#define DOT11_NGBR_BSS_TERM_DUR_SE_ID	4
#define DOT11_NGBR_BEARING_SE_ID	5
#define DOT11_NGBR_WIDE_BW_CHAN_SE_ID	6	/* Wide Bandwidth Channel subelement ID */
#define DOT11_NGBR_HE_6GHZ_CAP_SE_ID	198	/* HE 6GHz capability subelement ID */
#define DOT11_NGBR_EHT_CAP_SE_ID	199	/* EHT capability subelement ID */
#define DOT11_NGBR_EHT_OP_SE_ID		200	/* EHT Operation subelement ID */
#define DOT11_NGBR_MLIE_SE_ID		201	/* Basic multi-Link subelement ID */

/** Neighbor Report, BSS Transition Candidate Preference subelement */
BWL_PRE_PACKED_STRUCT struct dot11_ngbr_bsstrans_pref_se {
	uint8 sub_id;
	uint8 len;
	uint8 preference;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ngbr_bsstrans_pref_se dot11_ngbr_bsstrans_pref_se_t;
#define DOT11_NGBR_BSSTRANS_PREF_SE_LEN		1
#define DOT11_NGBR_BSSTRANS_PREF_SE_IE_LEN	3
#define DOT11_NGBR_BSSTRANS_PREF_SE_HIGHEST	0xff
#define DOT11_NGBR_BSSTRANS_PREF_SE_INTERMED	0x7f

/** Neighbor Report, BSS Termination Duration subelement */
BWL_PRE_PACKED_STRUCT struct dot11_ngbr_bss_term_dur_se {
	uint8 sub_id;
	uint8 len;
	uint8 tsf[8];
	uint16 duration;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ngbr_bss_term_dur_se dot11_ngbr_bss_term_dur_se_t;
#define DOT11_NGBR_BSS_TERM_DUR_SE_LEN	10

/* Neighbor Report BSSID Information Field (32 bits) */
#define DOT11_NGBR_BI_REACHABILTY_NOTR	0x0001
#define DOT11_NGBR_BI_REACHABILTY_UNKN	0x0002
#define DOT11_NGBR_BI_REACHABILTY	0x0003
#define DOT11_NGBR_BI_SEC		0x0004
#define DOT11_NGBR_BI_KEY_SCOPE		0x0008
#define DOT11_NGBR_BI_CAP		0x03f0
#define DOT11_NGBR_BI_CAP_SPEC_MGMT	0x0010
#define DOT11_NGBR_BI_CAP_QOS		0x0020
#define DOT11_NGBR_BI_CAP_APSD		0x0040
#define DOT11_NGBR_BI_CAP_RDIO_MSMT	0x0080
#define DOT11_NGBR_BI_CAP_DEL_BA	0x0100
#define DOT11_NGBR_BI_CAP_IMM_BA	0x0200
#define DOT11_NGBR_BI_MOBILITY		0x0400
#define DOT11_NGBR_BI_HT		0x0800
#define DOT11_NGBR_BI_VHT		0x1000
#define DOT11_NGBR_BI_FTM		0x2000
#define DOT11_NGBR_BI_HE		0x00004000
#define DOT11_NGBR_BI_ER_BSS		0x00008000 /**< robust beacon on HE, RU242 rate */
#define DOT11_NGBR_BI_COLOC_AP		0x00010000 /**< multiple APs in one device */
#define DOT11_NGBR_BI_UNSOL_PRBRSP	0x00020000 /**< unsolicited probe responses active */
#define DOT11_NGBR_BI_ESS_WITH_COLOC_AP 0x00040000 /**< colocated with a 2G/5G AP */
#define DOT11_NGBR_BI_OCT_WITH_REP_AP	0x00080000 /**< on-channel tunneling supported */
#define DOT11_NGBR_BI_COLOC_6G_AP	0x00100000 /**< colocated with a 6G AP */

/* Neighbor Report Phytype Field (8 bits) */
#define DOT11_PHYTYPE_FHSS		1
#define DOT11_PHYTYPE_DSSS		2
#define DOT11_PHYTYPE_IRBASEBAND	3
#define DOT11_PHYTYPE_OFDM		4
#define DOT11_PHYTYPE_HRDSSS		5
#define DOT11_PHYTYPE_ERP		6
#define DOT11_PHYTYPE_HT		7
#define DOT11_PHYTYPE_DMG		8
#define DOT11_PHYTYPE_VHT		9
#define DOT11_PHYTYPE_TVHT		10
#define DOT11_PHYTYPE_S1G		11
#define DOT11_PHYTYPE_CDMG		12
#define DOT11_PHYTYPE_CMMG		13
#define DOT11_PHYTYPE_HE		14

/** Neighbor Report element (11k & 11v) */
BWL_PRE_PACKED_STRUCT struct dot11_neighbor_rep_ie {
	uint8 id;
	uint8 len;
	struct ether_addr bssid;
	uint32 bssid_info;	/* uses DOT11_NGBR_BI_* macros */
	uint8 reg;		/* Operating class */
	uint8 channel;
	uint8 phytype;
	uint8 data[1];		/**< Variable size subelements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_neighbor_rep_ie dot11_neighbor_rep_ie_t;
#define DOT11_NEIGHBOR_REP_IE_FIXED_LEN	13

/* MLME Enumerations */
#define DOT11_BSSTYPE_INFRASTRUCTURE		0	/* d11 infrastructure */
#define DOT11_BSSTYPE_INDEPENDENT		1	/* d11 independent */
#define DOT11_BSSTYPE_ANY			2	/* d11 any BSS type */
#define DOT11_BSSTYPE_MESH			3	/* d11 Mesh */
#define DOT11_SCANTYPE_ACTIVE			0	/* d11 scan active */
#define DOT11_SCANTYPE_PASSIVE			1	/* d11 scan passive */

/** Link Measurement */
BWL_PRE_PACKED_STRUCT struct dot11_lmreq {
	uint8 category;				/* category of action frame (5) */
	uint8 action;				/* radio measurement action */
	uint8 token;				/* dialog token */
	uint8 txpwr;				/* Transmit Power Used */
	uint8 maxtxpwr;				/* Max Transmit Power */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_lmreq dot11_lmreq_t;
#define DOT11_LMREQ_LEN	5

BWL_PRE_PACKED_STRUCT struct dot11_lmrep {
	uint8 category;				/* category of action frame (5) */
	uint8 action;				/* radio measurement action */
	uint8 token;				/* dialog token */
	dot11_tpc_rep_t tpc;			/* TPC element */
	uint8 rxant;				/* Receive Antenna ID */
	uint8 txant;				/* Transmit Antenna ID */
	uint8 rcpi;				/* RCPI */
	uint8 rsni;				/* RSNI */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_lmrep dot11_lmrep_t;
#define DOT11_LMREP_LEN	11

#define DOT11_MP_CAP_SPECTRUM			0x01	/* d11 cap. spectrum */
#define DOT11_MP_CAP_SHORTSLOT			0x02	/* d11 cap. shortslot */
/* Measurement Pilot */
BWL_PRE_PACKED_STRUCT struct dot11_mprep {
	uint8 cap_info;				/* Condensed capability Info. */
	uint8 country[2];				/* Condensed country string */
	uint8 opclass;				/* Op. Class */
	uint8 channel;				/* Channel */
	uint8 mp_interval;			/* Measurement Pilot Interval */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mprep dot11_mprep_t;
#define DOT11_MPREP_LEN	6

/* 802.11 BRCM "Compromise" Pre N constants */
#define PREN_PREAMBLE		24	/* green field preamble time */
#define PREN_MM_EXT		12	/* extra mixed mode preamble time */
#define PREN_PREAMBLE_EXT	4	/* extra preamble (multiply by unique_streams-1) */

/* 802.11N PHY constants */
#define RIFS_11N_TIME		2	/* NPHY RIFS time */

/* 802.11 HT PLCP format 802.11n-2009, sec 20.3.9.4.3
 * HT-SIG is composed of two 24 bit parts, HT-SIG1 and HT-SIG2
 */
/* HT-SIG1 */
#define HT_SIG1_MCS_MASK        0x00007F
#define HT_SIG1_CBW             0x000080
#define HT_SIG1_CBW_SHIFT       7u
#define HT_SIG1_HT_LENGTH       0xFFFF00
#define HT_SIG1_HT_LENGTH_SHIFT 8u

/* HT-SIG2 */
#define HT_SIG2_SMOOTHING       0x000001
#define HT_SIG2_NOT_SOUNDING    0x000002
#define HT_SIG2_RESERVED        0x000004
#define HT_SIG2_AGGREGATION     0x000008
#define HT_SIG2_STBC_MASK       0x000030
#define HT_SIG2_STBC_SHIFT      4
#define HT_SIG2_FEC_CODING      0x000040
#define HT_SIG2_SHORT_GI        0x000080
#define HT_SIG2_ESS_MASK        0x000300
#define HT_SIG2_ESS_SHIFT       8
#define HT_SIG2_CRC             0x03FC00
#define HT_SIG2_TAIL            0x1C0000

/* HT Timing-related parameters (802.11-2012, sec 20.3.6) */
#define HT_T_LEG_PREAMBLE      16
#define HT_T_L_SIG              4
#define HT_T_SIG                8
#define HT_T_LTF1               4
#define HT_T_GF_LTF1            8
#define HT_T_LTFs               4
#define HT_T_STF                4
#define HT_T_GF_STF             8
#define HT_T_SYML               4

#define HT_N_SERVICE           16       /* bits in SERVICE field */
#define HT_N_TAIL               6       /* tail bits per BCC encoder */

/* 802.11 A PHY constants */
#define APHY_SLOT_TIME          9       /* APHY slot time */
#define APHY_SIFS_TIME          16      /* APHY SIFS time */
#define APHY_DIFS_TIME          (APHY_SIFS_TIME + (2 * APHY_SLOT_TIME))  /* APHY DIFS time */
#define APHY_PREAMBLE_TIME      16      /* APHY preamble time */
#define APHY_SIGNAL_TIME        4       /* APHY signal time */
#define APHY_SYMBOL_TIME        4       /* APHY symbol time */
#define APHY_SERVICE_NBITS      16      /* APHY service nbits */
#define APHY_TAIL_NBITS         6       /* APHY tail nbits */
#define APHY_CWMIN              15      /* APHY cwmin */
#define APHY_PHYHDR_DUR		20	/* APHY PHY Header Duration */

/* 802.11 B PHY constants */
#define BPHY_SLOT_TIME          20      /* BPHY slot time */
#define BPHY_SIFS_TIME          10      /* BPHY SIFS time */
#define BPHY_DIFS_TIME          50      /* BPHY DIFS time */
#define BPHY_PLCP_TIME          192     /* Long PLCP preamble + PLCP header, in [us] */
#define BPHY_PLCP_SHORT_TIME    96      /* BPHY PLCP short time */
#define BPHY_CWMIN              31      /* BPHY cwmin */
#define BPHY_SHORT_PHYHDR_DUR	96	/* BPHY Short PHY Header Duration */
#define BPHY_LONG_PHYHDR_DUR	192	/* BPHY Long PHY Header Duration */

/* 802.11 G constants */
#define DOT11_OFDM_SIGNAL_EXTENSION	6	/* d11 OFDM signal extension */

#define PHY_CWMAX		1023	/* PHY cwmax */

#define	DOT11_MAXNUMFRAGS	16	/* max # fragments per MSDU */

/* 802.11 VHT constants */

typedef int vht_group_id_t;

/* for VHT-A1 */
/* SIG-A1 reserved bits */
#define VHT_SIGA1_CONST_MASK            0x800004

#define VHT_SIGA1_BW_MASK               0x000003
#define VHT_SIGA1_20MHZ_VAL             0x000000
#define VHT_SIGA1_40MHZ_VAL             0x000001
#define VHT_SIGA1_80MHZ_VAL             0x000002
#define VHT_SIGA1_160MHZ_VAL            0x000003

#define VHT_SIGA1_STBC                  0x000008

#define VHT_SIGA1_GID_MASK              0x0003f0
#define VHT_SIGA1_GID_SHIFT             4
#define VHT_SIGA1_GID_TO_AP             0x00
#define VHT_SIGA1_GID_NOT_TO_AP         0x3f
#define VHT_SIGA1_GID_MAX_GID           0x3f

#define VHT_SIGA1_NSTS_SHIFT_MASK_USER0 0x001C00
#define VHT_SIGA1_NSTS_SHIFT            10
#define VHT_SIGA1_MAX_USERPOS           3

#define VHT_SIGA1_PARTIAL_AID_MASK      0x3fe000
#define VHT_SIGA1_PARTIAL_AID_SHIFT     13

#define VHT_SIGA1_TXOP_PS_NOT_ALLOWED   0x400000

/* for VHT-A2 */
#define VHT_SIGA2_GI_NONE               0x000000
#define VHT_SIGA2_GI_SHORT              0x000001
#define VHT_SIGA2_GI_W_MOD10            0x000002
#define VHT_SIGA2_CODING_LDPC           0x000004
#define VHT_SIGA2_LDPC_EXTRA_OFDM_SYM   0x000008
#define VHT_SIGA2_BEAMFORM_ENABLE       0x000100
#define VHT_SIGA2_MCS_SHIFT             4

#define VHT_SIGA2_B9_RESERVED           0x000200
#define VHT_SIGA2_TAIL_MASK             0xfc0000
#define VHT_SIGA2_TAIL_VALUE            0x000000

/* VHT Timing-related parameters (802.11ac D4.0, sec 22.3.6) */
#define VHT_T_LEG_PREAMBLE      16
#define VHT_T_L_SIG              4
#define VHT_T_SIG_A              8
#define VHT_T_LTF                4
#define VHT_T_STF                4
#define VHT_T_SIG_B              4
#define VHT_T_SYML               4

#define VHT_N_SERVICE           16	/* bits in SERVICE field */
#define VHT_N_TAIL               6	/* tail bits per BCC encoder */

/* 802.11 HE constants */

#define HE_PLCP0_FORMAT_MASK		(0x00000001)
#define HE_PLCP0_FORMAT_SHIFT		(0)
#define HE_PLCP0_DL_UL			(0x00000002)
#define HE_PLCP0_DL_UL_SHIFT		(2)
#define HE_PLCP0_MCS_MASK		(0x00000078)
#define HE_PLCP0_MCS_SHIFT		(3)
#define HE_PLCP0_DCM_MASK		(0x00000080)
#define HE_PLCP0_DCM_SHIFT		(7)
#define HE_PLCP0_BW_MASK		(0x00180000)
#define HE_PLCP0_BW_SHIFT		(19)
#define HE_PLCP0_CPLTF_MASK		(0x00600000)
#define HE_PLCP0_CPLTF_SHIFT		(21)
#define HE_PLCP0_NSTS_MASK		(0x03800000)
#define HE_PLCP0_NSTS_SHIFT		(23)

#define HE_PLCP1_CODING_MASK		(0x02)
#define HE_PLCP1_CODING_SHIFT		(1)
#define HE_PLCP1_STBC_MASK		(0x08)
#define HE_PLCP1_STBC_SHIFT		(3)
#define HE_PLCP1_TXBF_MASK		(0x10)
#define HE_PLCP1_TXBF_SHIFT		(4)

/* 802.11 HEMU PLCP siga bit defintions */
#define HEMU_PLCP0_BSSCOLOR_MASK		(0x000007E0)
#define HEMU_PLCP0_BSSCOLOR_SHIFT		(5)
#define HEMU_PLCP0_BW_MASK			(0x00038000)
#define HEMU_PLCP0_BW_SHIFT			(15)
#define HEMU_PLCP0_CPLTF_MASK			(0x01800000)
#define HEMU_PLCP0_CPLTF_SHIFT			(23)

#define HEMU_PLCP1_STBC_MASK			(0x1000)
#define HEMU_PLCP1_STBC_SHIFT			(12)

#define HEMU_PLCP0_BSSCOLOR(plcp0) \
	((plcp0 & HEMU_PLCP0_BSSCOLOR_MASK) >> HEMU_PLCP0_BSSCOLOR_SHIFT)
#define HEMU_PLCP0_BW(plcp0) \
	((plcp0 & HEMU_PLCP0_BW_MASK) >> HEMU_PLCP0_BW_SHIFT)
#define HEMU_PLCP0_CPLTF(plcp0)	\
	((plcp0 & HEMU_PLCP0_CPLTF_MASK) >> HEMU_PLCP0_CPLTF_SHIFT)

/* 802.11 HEMU PLCP sigb bit defintions */
#define HEMU_SIGB_MCS_MASK			(0x00078000)
#define HEMU_SIGB_MCS_SHIFT			(15)
#define HEMU_SIGB_NSTS_MASK			(0x00003800)
#define HEMU_SIGB_NSTS_SHIFT			(11)
#define HEMU_SIGB_CODING_MASK			(0x00100000)
#define HEMU_SIGB_CODING_SHIFT			(20)
#define HEMU_SIGB_DCM_MASK			(0x00080000)
#define HEMU_SIGB_DCM_SHIFT			(19)
#define HEMU_SIGB_TXBF_MASK			(0x00004000)
#define HEMU_SIGB_TXBF_SHIFT			(14)

#define HE_LTF_1_GI_1_6us		(0)
#define HE_LTF_2_GI_0_8us		(1)
#define HE_LTF_2_GI_1_6us		(2)
#define HE_LTF_4_GI_3_2us		(3)

#define WL_HE_CAP_MCS_MAP_NSS_MAX	8

/** dot11Counters Table - 802.11 spec., Annex D */
typedef struct d11cnt {
	uint32		txfrag;		/* dot11TransmittedFragmentCount */
	uint32		txmulti;	/* dot11MulticastTransmittedFrameCount */
	uint32		txfail;		/* dot11FailedCount */
	uint32		txretry;	/* dot11RetryCount */
	uint32		txretrie;	/* dot11MultipleRetryCount */
	uint32		rxdup;		/* dot11FrameduplicateCount */
	uint32		txrts;		/* dot11RTSSuccessCount */
	uint32		txnocts;	/* dot11RTSFailureCount */
	uint32		txnoack;	/* dot11ACKFailureCount */
	uint32		rxfrag;		/* dot11ReceivedFragmentCount */
	uint32		rxmulti;	/* dot11MulticastReceivedFrameCount */
	uint32		rxcrc;		/* dot11FCSErrorCount */
	uint32		txfrmsnt;	/* dot11TransmittedFrameCount */
	uint32		rxundec;	/* dot11WEPUndecryptableCount */
} d11cnt_t;

#define BRCM_PROP_OUI		"\x00\x90\x4C"
#define VENDOR_MLO_LINKID_OUI_TYPE 0x9E

/* xxx Broadcom Proprietary OUI type list. Please update below twiki page when adding a new type.
 * xxx Twiki http://hwnbu-twiki.sj.broadcom.com/bin/view/Mwgroup/WlBrcmPropIE
 */
/* xxx The following BRCM_PROP_OUI types are currently in use (defined in
 * relevant subsections). Each of them will be in a separate proprietary(221) IE
 * #define RWL_WIFI_DEFAULT		0
 * #define SES_VNDR_IE_TYPE		1   (defined in src/ses/shared/ses.h)
 * #define VHT_FEATURES_IE_TYPE  		4
 * #define RWL_WIFI_FIND_MY_PEER		9
 * #define RWL_WIFI_FOUND_PEER		10
 * #define PROXD_IE_TYPE			11
 */

#define BRCM_FTM_IE_TYPE			14

/* #define HT_CAP_IE_TYPE			51
 * #define HT_ADD_IE_TYPE			52
 * #define BRCM_EXTCH_IE_TYPE		53
 * #define MEMBER_OF_BRCM_PROP_IE_TYPE	54
 * #define BRCM_RELMACST_IE_TYPE		55
 * #define BRCM_EVT_WL_BSS_INFO		64
 * #define RWL_ACTION_WIFI_FRAG_TYPE	85
 * #define BTC_INFO_BRCM_PROP_IE_TYPE	90
 * #define ULB_BRCM_PROP_IE_TYPE	91
 * #define SDB_BRCM_PROP_IE_TYPE	92
 */

/* Action frame type for RWL */
#define RWL_WIFI_DEFAULT		0
#define RWL_WIFI_FIND_MY_PEER		9 /* Used while finding server */
#define RWL_WIFI_FOUND_PEER		10 /* Server response to the client  */
#define RWL_ACTION_WIFI_FRAG_TYPE	85 /* Fragment indicator for receiver */

#define PROXD_AF_TYPE			11 /* Wifi proximity action frame type */
#define BRCM_RELMACST_AF_TYPE	        12 /* RMC action frame type */

/* Action frame type for FTM Initiator Report */
#define BRCM_FTM_VS_AF_TYPE	14
enum {
	BRCM_FTM_VS_INITIATOR_RPT_SUBTYPE = 1,	/* FTM Initiator Report */
	BRCM_FTM_VS_COLLECT_SUBTYPE = 2,	/* FTM Collect debug protocol */
};

/* xxx Action frame type for ULB
 * #define BRCM_ULB_AF_TYPE		15
 */

#ifndef LINUX_POSTMOGRIFY_REMOVAL
/*
 * This BRCM_PROP_OUI types is intended for use in events to embed additional
 * data, and would not be expected to appear on the air -- but having an IE
 * format allows IE frame data with extra data in events in that allows for
 * more flexible parsing.
 */
#define BRCM_EVT_WL_BSS_INFO	64

/**
 * Following is the generic structure for brcm_prop_ie (uses BRCM_PROP_OUI).
 * DPT uses this format with type set to DPT_IE_TYPE
 */
BWL_PRE_PACKED_STRUCT struct brcm_prop_ie_s {
	uint8 id;		/* IE ID, 221, DOT11_MNG_PROPR_ID */
	uint8 len;		/* IE length */
	uint8 oui[3];
	uint8 type;		/* type of this IE */
	uint16 cap;		/* DPT capabilities */
} BWL_POST_PACKED_STRUCT;
typedef struct brcm_prop_ie_s brcm_prop_ie_t;

#define BRCM_PROP_IE_LEN	6	/* len of fixed part of brcm_prop ie */

#define DPT_IE_TYPE             2

#define BRCM_SYSCAP_IE_TYPE	3
#define WET_TUNNEL_IE_TYPE	3
#endif /* LINUX_POSTMOGRIFY_REMOVAL */

/* MultiAp extensible attribute */
BWL_PRE_PACKED_STRUCT struct multiap_ext_attr {
	uint8 attr;			/* Attribute */
	uint8 attr_len;			/* Attribute len */
	uint8 attr_val;			/* Attribute value */
} BWL_POST_PACKED_STRUCT;
typedef struct multiap_ext_attr multiap_ext_attr_t;

#define MAP_EXT_ATTR						0x06
#define MAP_EXT_ATTR_LEN					0x01
#define MAP_EXT_ATTR_PROFILE2_STA_ASSOC_DISALLOWED		0x04
#define MAP_EXT_ATTR_PROFILE1_STA_ASSOC_DISALLOWED		0x08
#define MAP_EXT_ATTR_TEAR_DOWN					0x10
#define MAP_EXT_ATTR_FRNTHAUL_BSS				0x20
#define MAP_EXT_ATTR_BACKHAUL_BSS				0x40
#define MAP_EXT_ATTR_BACKHAUL_STA				0x80

/* Multi-AP Profile subelement */
BWL_PRE_PACKED_STRUCT struct multiap_profile_se {
	uint8 id;		/* sub element id */
	uint8 len;		/* sub element len */
	uint8 map_profile;	/* Multi-AP Profile field */
} BWL_POST_PACKED_STRUCT;
typedef struct multiap_profile_se multiap_profile_se_t;

#define MAP_PROFILE_SE_ID		0x07	/* tmp subject to change as per MAP spec */
#define MAP_PROFILE_SE_LEN		0x01
#define MAP_PROFILE_0			0x00	/* Multi-AP R1. No profile */
#define MAP_PROFILE_1			0x01	/* Multi-AP Profile-1(Release 4 onwards) */
#define MAP_PROFILE_2			0x02	/* Multi-AP Profile-2 */

/* Multi-AP Default 802.1Q Setting subelement */
BWL_PRE_PACKED_STRUCT struct multiap_def_8021Q_settings_se {
	uint8 id;		/* sub element id */
	uint8 len;		/* sub element len */
	uint16 prim_vlan_id;	/* Primary VLAN ID */
} BWL_POST_PACKED_STRUCT;
typedef struct multiap_def_8021Q_settings_se multiap_def_8021Q_settings_se_t;

#define MAP_8021Q_SETTINGS_SE_ID	0x08 /* tmp subject to change as per MAP spec */
#define MAP_8021Q_SETTINGS_SE_LEN	0x02

/* MultiAP info element */
BWL_PRE_PACKED_STRUCT struct multiap_ie {
	uint8 id;
	uint8 len;
	uint8 oui[3];
	uint8 type;
	uint8 attr[];		/* Variable length MultiAP extensible attributes */
} BWL_POST_PACKED_STRUCT;
typedef struct multiap_ie multiap_ie_t;
#define MAP_IE_FIXED_LEN		4
#define MAP_IE_MAX_LEN			16	/* includes attributes, with tlv hdr */
#define EXT_RSN_CAP_MAX_LEN		5
/* MultiAP oui type */
#define WFA_OUI_TYPE_MULTIAP	0x1B

/* brcm syscap_ie cap */
#define BRCM_SYSCAP_WET_TUNNEL	0x0100	/* Device with WET_TUNNEL support */

#define BRCM_OUI		"\x00\x10\x18"	/* Broadcom OUI */

/** BRCM info element */
BWL_PRE_PACKED_STRUCT struct brcm_ie {
	uint8	id;		/* IE ID, 221, DOT11_MNG_PROPR_ID */
	uint8	len;		/* IE length */
	uint8	oui[3];
	uint8	ver;		/* type/ver of this IE */
	uint8	assoc;		/* # of assoc STAs */
	uint8	flags;		/* misc flags */
	uint8	flags1;		/* misc flags */
	uint16	amsdu_mtu_pref;	/* preferred A-MSDU MTU */
	uint8	flags2;		/* misc flags */
} BWL_POST_PACKED_STRUCT;
typedef	struct brcm_ie brcm_ie_t;
#define BRCM_IE_LEN		12	/* BRCM IE length */
#define BRCM_IE_VER		2	/* BRCM IE version */
#define BRCM_IE_LEGACY_AES_VER	1	/* BRCM IE legacy AES version */

/* brcm_ie flags */
#define	BRF_ABCAP		0x1	/* afterburner is obsolete,  defined for backward compat */
#define	BRF_ABRQRD		0x2	/* afterburner is obsolete,  defined for backward compat */
#define	BRF_LZWDS		0x4	/* lazy wds enabled */
#define	BRF_BLOCKACK		0x8	/* BlockACK capable */
#define BRF_ABCOUNTER_MASK	0xf0	/* afterburner is obsolete,  defined for backward compat */
#define BRF_PROP_11N_MCS	0x10	/* prop11n is obsolete */
#define BRF_MEDIA_CLIENT	0x20	/* re-use afterburner bit to indicate media client device */

/* brcm_ie flags1 */
#define	BRF1_AMSDU		0x1	/* A-MSDU capable */
#define	BRF1_WNM		0x2	/* WNM capable */
#define BRF1_WMEPS		0x4	/* AP is capable of handling WME + PS w/o APSD */
#define BRF1_PSOFIX		0x8	/* AP has fixed PS mode out-of-order packets */
#define	BRF1_RX_LARGE_AGG	0x10	/* device can rx large aggregates */
#define BRF1_SPARE20		0x20	/* unused, was BRF1_RFAWARE_DCS */
#define BRF1_SOFTAP		0x40	/* Configure as Broadcom SOFTAP */
#define BRF1_DWDS		0x80	/* DWDS capable */

/* brcm_ie flags2 */
#define BRF2_DTPC_TXCAP		0x1	/* DTPC capable */
#define BRF2_DTPC_RXCAP		0x2	/* DTPC capable */

/** Vendor IE structure */
BWL_PRE_PACKED_STRUCT struct vndr_ie {
	uchar id;
	uchar len;
	uchar oui [3];
	uchar data [1]; 	/* Variable size data */
} BWL_POST_PACKED_STRUCT;
typedef struct vndr_ie vndr_ie_t;

#define VNDR_IE_HDR_LEN		2	/* id + len field */
#define VNDR_IE_MIN_LEN		3	/* size of the oui field */
#define VNDR_IE_FIXED_LEN	(VNDR_IE_HDR_LEN + VNDR_IE_MIN_LEN)

#define VNDR_IE_MAX_LEN		255	/* vendor IE max length, without ID and len */

/** BRCM PROP DEVICE PRIMARY MAC ADDRESS IE */
BWL_PRE_PACKED_STRUCT struct member_of_brcm_prop_ie {
	uchar id;
	uchar len;
	uchar oui[3];
	uint8	type;           /* type indicates what follows */
	struct ether_addr ea;   /* Device Primary MAC Adrress */
} BWL_POST_PACKED_STRUCT;
typedef struct member_of_brcm_prop_ie member_of_brcm_prop_ie_t;

#define MEMBER_OF_BRCM_PROP_IE_LEN		10	/* IE max length */
#define MEMBER_OF_BRCM_PROP_IE_HDRLEN	        (sizeof(member_of_brcm_prop_ie_t))
#define MEMBER_OF_BRCM_PROP_IE_TYPE		54

/** BRCM Reliable Multicast IE */
BWL_PRE_PACKED_STRUCT struct relmcast_brcm_prop_ie {
	uint8 id;
	uint8 len;
	uint8 oui[3];
	uint8 type;           /* type indicates what follows */
	struct ether_addr ea;   /* The ack sender's MAC Adrress */
	struct ether_addr mcast_ea;  /* The multicast MAC address */
	uint8 updtmo; /* time interval(second) for client to send null packet to report its rssi */
} BWL_POST_PACKED_STRUCT;
typedef struct relmcast_brcm_prop_ie relmcast_brcm_prop_ie_t;

/* IE length */
/* BRCM_PROP_IE_LEN = sizeof(relmcast_brcm_prop_ie_t)-((sizeof (id) + sizeof (len)))? */
#define RELMCAST_BRCM_PROP_IE_LEN	(sizeof(relmcast_brcm_prop_ie_t)-(2*sizeof(uint8)))

#define RELMCAST_BRCM_PROP_IE_TYPE	55

/* BRCM BTC IE */
BWL_PRE_PACKED_STRUCT struct btc_brcm_prop_ie {
	uint8 id;
	uint8 len;
	uint8 oui[3];
	uint8 type;           /* type inidicates what follows */
	uint32 info;
} BWL_POST_PACKED_STRUCT;
typedef struct btc_brcm_prop_ie btc_brcm_prop_ie_t;

#define BTC_INFO_BRCM_PROP_IE_TYPE	90
#define BRCM_BTC_INFO_TYPE_LEN	(sizeof(btc_brcm_prop_ie_t) - (2 * sizeof(uint8)))

/* ************* HT definitions. ************* */
#define MCSSET_LEN	16	/* 16-bits per 8-bit set to give 128-bits bitmap of MCS Index */
#define MAX_MCS_NUM	(128)	/* max mcs number = 128 */

BWL_PRE_PACKED_STRUCT struct ht_cap_ie {
	uint16	cap;
	uint8	params;				/**< a-mpdu parameters */
	uint8	supp_mcs[MCSSET_LEN];
	uint16	ext_htcap;
	uint32	txbf_cap;
	uint8	as_cap;
} BWL_POST_PACKED_STRUCT;
typedef struct ht_cap_ie ht_cap_ie_t;

BWL_PRE_PACKED_STRUCT struct dot11_ht_cap_ie {
	uint8	id;
	uint8	len;
	ht_cap_ie_t ht_cap;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ht_cap_ie dot11_ht_cap_ie_t;

/* CAP IE: HT 1.0 spec. simply stole a 802.11 IE, we use our prop. IE until this is resolved */
/* the capability IE is primarily used to convey this nodes abilities */
BWL_PRE_PACKED_STRUCT struct ht_prop_cap_ie {
	uint8	id;		/* IE ID, 221, DOT11_MNG_PROPR_ID */
	uint8	len;		/* IE length */
	uint8	oui[3];
	uint8	type;           /* type indicates what follows */
	ht_cap_ie_t cap_ie;
} BWL_POST_PACKED_STRUCT;
typedef struct ht_prop_cap_ie ht_prop_cap_ie_t;

#define HT_PROP_IE_OVERHEAD	4	/* overhead bytes for prop oui ie */
#define HT_CAP_IE_LEN		26	/* HT capability len (based on .11n d2.0) */
#define HT_CAP_IE_TYPE		51

#define HT_CAP_LDPC_CODING	0x0001	/* Support for rx of LDPC coded pkts */
#define HT_CAP_40MHZ		0x0002  /* FALSE:20Mhz, TRUE:20/40MHZ supported */
#define HT_CAP_MIMO_PS_MASK	0x000C  /* Mimo PS mask */
#define HT_CAP_MIMO_PS_SHIFT	0x0002	/* Mimo PS shift */
#define HT_CAP_MIMO_PS_OFF	0x0003	/* Mimo PS, no restriction */
#define HT_CAP_MIMO_PS_RTS	0x0001	/* Mimo PS, send RTS/CTS around MIMO frames */
#define HT_CAP_MIMO_PS_ON	0x0000	/* Mimo PS, MIMO disallowed */
#define HT_CAP_GF		0x0010	/* Greenfield preamble support */
#define HT_CAP_SHORT_GI_20	0x0020	/* 20MHZ short guard interval support */
#define HT_CAP_SHORT_GI_40	0x0040	/* 40Mhz short guard interval support */
#define HT_CAP_TX_STBC		0x0080	/* Tx STBC support */
#define HT_CAP_RX_STBC_MASK	0x0300	/* Rx STBC mask */
#define HT_CAP_RX_STBC_SHIFT	8	/* Rx STBC shift */
#define HT_CAP_DELAYED_BA	0x0400	/* delayed BA support */
#define HT_CAP_MAX_AMSDU	0x0800	/* Max AMSDU size in bytes , 0=3839, 1=7935 */

#define HT_CAP_DSSS_CCK	0x1000	/* DSSS/CCK supported by the BSS */
#define HT_CAP_PSMP		0x2000	/* Power Save Multi Poll support */
#define HT_CAP_40MHZ_INTOLERANT 0x4000	/* 40MHz Intolerant */
#define HT_CAP_LSIG_TXOP	0x8000	/* L-SIG TXOP protection support */

#define HT_CAP_RX_STBC_NO		0x0	/* no rx STBC support */
#define HT_CAP_RX_STBC_ONE_STREAM	0x1	/* rx STBC support of 1 spatial stream */
#define HT_CAP_RX_STBC_TWO_STREAM	0x2	/* rx STBC support of 1-2 spatial streams */
#define HT_CAP_RX_STBC_THREE_STREAM	0x3	/* rx STBC support of 1-3 spatial streams */

#define HT_CAP_TXBF_CAP_IMPLICIT_TXBF_RX	0x1
#define HT_CAP_TXBF_CAP_NDP_RX			0x8
#define HT_CAP_TXBF_CAP_NDP_TX			0x10
#define HT_CAP_TXBF_CAP_EXPLICIT_CSI		0x100
#define HT_CAP_TXBF_CAP_EXPLICIT_NC_STEERING	0x200
#define HT_CAP_TXBF_CAP_EXPLICIT_C_STEERING	0x400
#define HT_CAP_TXBF_CAP_EXPLICIT_CSI_FB_MASK	0x1800
#define HT_CAP_TXBF_CAP_EXPLICIT_CSI_FB_SHIFT	11
#define HT_CAP_TXBF_CAP_EXPLICIT_NC_FB_MASK	0x6000
#define HT_CAP_TXBF_CAP_EXPLICIT_NC_FB_SHIFT	13
#define HT_CAP_TXBF_CAP_EXPLICIT_C_FB_MASK	0x18000
#define HT_CAP_TXBF_CAP_EXPLICIT_C_FB_SHIFT	15
#define HT_CAP_TXBF_CAP_CSI_BFR_ANT_SHIFT	19
#define HT_CAP_TXBF_CAP_NC_BFR_ANT_SHIFT	21
#define HT_CAP_TXBF_CAP_C_BFR_ANT_SHIFT		23
#define HT_CAP_TXBF_CAP_C_BFR_ANT_MASK		0x1800000

#define HT_CAP_TXBF_CAP_CHAN_ESTIM_SHIFT	27
#define HT_CAP_TXBF_CAP_CHAN_ESTIM_MASK		0x18000000

#define HT_CAP_TXBF_FB_TYPE_NONE 	0
#define HT_CAP_TXBF_FB_TYPE_DELAYED 	1
#define HT_CAP_TXBF_FB_TYPE_IMMEDIATE 	2
#define HT_CAP_TXBF_FB_TYPE_BOTH 	3

#define HT_CAP_TX_BF_CAP_EXPLICIT_CSI_FB_MASK	0x400
#define HT_CAP_TX_BF_CAP_EXPLICIT_CSI_FB_SHIFT	10
#define HT_CAP_TX_BF_CAP_EXPLICIT_COMPRESSED_FB_MASK 0x18000
#define HT_CAP_TX_BF_CAP_EXPLICIT_COMPRESSED_FB_SHIFT 15

#define HT_CAP_MCS_FLAGS_SUPP_BYTE 12 /* byte offset in HT Cap Supported MCS for various flags */
#define HT_CAP_MCS_RX_8TO15_BYTE_OFFSET                1
#define HT_CAP_MCS_FLAGS_TX_RX_UNEQUAL              0x02
#define HT_CAP_MCS_FLAGS_MAX_SPATIAL_STREAM_MASK    0x0C

#define VHT_MAX_MPDU		11454	/* max mpdu size for now (bytes) */
#define VHT_MPDU_MSDU_DELTA	56		/* Difference in spec - vht mpdu, amsdu len */
/* Max AMSDU len - per spec */
#define VHT_MAX_AMSDU		(VHT_MAX_MPDU - VHT_MPDU_MSDU_DELTA)

#define HT_MAX_AMSDU		7935	/* max amsdu size (bytes) per the HT spec */
#define HT_MIN_AMSDU		3835	/* min amsdu size (bytes) per the HT spec */

#define HT_PARAMS_RX_FACTOR_MASK	0x03	/* ampdu rcv factor mask */
#define HT_PARAMS_DENSITY_MASK		0x1C	/* ampdu density mask */
#define HT_PARAMS_DENSITY_SHIFT	2	/* ampdu density shift */

/* HT/AMPDU specific define */
#define AMPDU_MAX_MPDU_DENSITY  7       /* max mpdu density; in 1/4 usec units */
#define AMPDU_DENSITY_NONE      0       /* No density requirement */
#define AMPDU_DENSITY_1over4_US 1       /* 1/4 us density */
#define AMPDU_DENSITY_1over2_US 2       /* 1/2 us density */
#define AMPDU_DENSITY_1_US      3       /*   1 us density */
#define AMPDU_DENSITY_2_US      4       /*   2 us density */
#define AMPDU_DENSITY_4_US      5       /*   4 us density */
#define AMPDU_DENSITY_8_US      6       /*   8 us density */
#define AMPDU_DENSITY_16_US     7       /*  16 us density */

/* RX factor related defines, first set from HT std */
#define AMPDU_RX_FACTOR_8K		0	/* max rcv ampdu len (8kb) */
#define AMPDU_RX_FACTOR_16K		1	/* max rcv ampdu len (16kb) */
#define AMPDU_RX_FACTOR_32K		2	/* max rcv ampdu len (32kb) */
#define AMPDU_RX_FACTOR_64K		3	/* max rcv ampdu len (64kb) */
/* AMPDU RX factors for VHT rates */
#define AMPDU_RX_FACTOR_128K		4	/* max rcv ampdu len (128kb) */
#define AMPDU_RX_FACTOR_256K		5	/* max rcv ampdu len (256kb) */
#define AMPDU_RX_FACTOR_512K		6	/* max rcv ampdu len (512kb) */
#define AMPDU_RX_FACTOR_1024K		7	/* max rcv ampdu len (1024kb) */
/* AMPDU RX factors for HE rates */
#define AMPDU_RX_FACTOR_2048K		8	/* max rcv ampdu len (2048kb) */
#define AMPDU_RX_FACTOR_4096K		9	/* max rcv ampdu len (4096kb) */
#define AMPDU_RX_FACTOR_8192K		10	/* max rcv ampdu len (8192kb) */
/* AMPDU RX factors for EHT rates */
#define AMPDU_RX_FACTOR_16384K		11	/* max rcv ampdu len (16384kb) */
/* AMPDU RX factor computional support */
#define AMPDU_RX_FACTOR_BASE_PWR	13	/* base for RXFACTOR, in power of 2 => 8kb */

#define AMPDU_DELIMITER_LEN	4	/* length of ampdu delimiter */
#define AMPDU_DELIMITER_LEN_MAX	63	/* max length of ampdu delimiter(enforced in HW) */

#define HT_CAP_EXT_PCO			0x0001
#define HT_CAP_EXT_PCO_TTIME_MASK	0x0006
#define HT_CAP_EXT_PCO_TTIME_SHIFT	1
#define HT_CAP_EXT_MCS_FEEDBACK_MASK	0x0300
#define HT_CAP_EXT_MCS_FEEDBACK_SHIFT	8
#define HT_CAP_EXT_HTC			0x0400
#define HT_CAP_EXT_RD_RESP		0x0800

/** 'ht_add' is called 'HT Operation' information element in the 802.11 standard */
BWL_PRE_PACKED_STRUCT struct ht_add_ie {
	uint8	ctl_ch;			/* control channel number */
	uint8	byte1;			/* ext ch,rec. ch. width, RIFS support */
	uint16	opmode;			/* operation mode */
	uint16	misc_bits;		/* misc bits */
	uint8	basic_mcs[MCSSET_LEN];  /* required MCS set */
} BWL_POST_PACKED_STRUCT;
typedef struct ht_add_ie ht_add_ie_t;

/* ADD IE: HT 1.0 spec. simply stole a 802.11 IE, we use our prop. IE until this is resolved */
/* the additional IE is primarily used to convey the current BSS configuration */
BWL_PRE_PACKED_STRUCT struct ht_prop_add_ie {
	uint8	id;		/* IE ID, 221, DOT11_MNG_PROPR_ID */
	uint8	len;		/* IE length */
	uint8	oui[3];
	uint8	type;		/* indicates what follows */
	ht_add_ie_t add_ie;
} BWL_POST_PACKED_STRUCT;
typedef struct ht_prop_add_ie ht_prop_add_ie_t;

#define HT_ADD_IE_LEN	22
#define HT_ADD_IE_TYPE	52

/* byte1 defn's */
#define HT_BW_ANY		0x04	/* set, STA can use 20 or 40MHz */
#define HT_RIFS_PERMITTED     	0x08	/* RIFS allowed */

/* opmode defn's */
#define HT_OPMODE_MASK	        0x0003	/* protection mode mask */
#define HT_OPMODE_SHIFT		0	/* protection mode shift */
#define HT_OPMODE_PURE		0x0000	/* protection mode PURE */
#define HT_OPMODE_OPTIONAL	0x0001	/* protection mode optional */
#define HT_OPMODE_HT20IN40	0x0002	/* protection mode 20MHz HT in 40MHz BSS */
#define HT_OPMODE_MIXED	0x0003	/* protection mode Mixed Mode */
#define HT_OPMODE_NONGF	0x0004	/* protection mode non-GF */
#define DOT11N_TXBURST		0x0008	/* Tx burst limit */
#define DOT11N_OBSS_NONHT	0x0010	/* OBSS Non-HT STA present */
#define HT_OPMODE_CCFS2_MASK	0x1fe0	/* Channel Center Frequency Segment 2 mask */
#define HT_OPMODE_CCFS2_SHIFT	5	/* Channel Center Frequency Segment 2 shift */

/* misc_bites defn's */
#define HT_BASIC_STBC_MCS	0x007f	/* basic STBC MCS */
#define HT_DUAL_STBC_PROT	0x0080	/* Dual STBC Protection */
#define HT_SECOND_BCN		0x0100	/* Secondary beacon support */
#define HT_LSIG_TXOP		0x0200	/* L-SIG TXOP Protection full support */
#define HT_PCO_ACTIVE		0x0400	/* PCO active */
#define HT_PCO_PHASE		0x0800	/* PCO phase */
#define HT_DUALCTS_PROTECTION	0x0080	/* DUAL CTS protection needed */

/* Tx Burst Limits */
#define DOT11N_2G_TXBURST_LIMIT	6160	/* 2G band Tx burst limit per 802.11n Draft 1.10 (usec) */
#define DOT11N_5G_TXBURST_LIMIT	3080	/* 5G band Tx burst limit per 802.11n Draft 1.10 (usec) */

/* Macros for opmode */
#define GET_HT_OPMODE(add_ie)		((ltoh16_ua(&add_ie->opmode) & HT_OPMODE_MASK) \
					>> HT_OPMODE_SHIFT)
#define HT_MIXEDMODE_PRESENT(add_ie)	((ltoh16_ua(&add_ie->opmode) & HT_OPMODE_MASK) \
					== HT_OPMODE_MIXED)	/* mixed mode present */
#define HT_HT20_PRESENT(add_ie)	((ltoh16_ua(&add_ie->opmode) & HT_OPMODE_MASK) \
					== HT_OPMODE_HT20IN40)	/* 20MHz HT present */
#define HT_OPTIONAL_PRESENT(add_ie)	((ltoh16_ua(&add_ie->opmode) & HT_OPMODE_MASK) \
					== HT_OPMODE_OPTIONAL)	/* Optional protection present */
#define HT_USE_PROTECTION(add_ie)	(HT_HT20_PRESENT((add_ie)) || \
					HT_MIXEDMODE_PRESENT((add_ie))) /* use protection */
#define HT_NONGF_PRESENT(add_ie)	((ltoh16_ua(&add_ie->opmode) & HT_OPMODE_NONGF) \
					== HT_OPMODE_NONGF)	/* non-GF present */
#define DOT11N_TXBURST_PRESENT(add_ie)	((ltoh16_ua(&add_ie->opmode) & DOT11N_TXBURST) \
					== DOT11N_TXBURST)	/* Tx Burst present */
#define DOT11N_OBSS_NONHT_PRESENT(add_ie)	((ltoh16_ua(&add_ie->opmode) & DOT11N_OBSS_NONHT) \
					== DOT11N_OBSS_NONHT)	/* OBSS Non-HT present */
#define HT_OPMODE_CCFS2_GET(add_ie)	((ltoh16_ua(&(add_ie)->opmode) & HT_OPMODE_CCFS2_MASK) \
					>> HT_OPMODE_CCFS2_SHIFT)	/* get CCFS2 */
#define HT_OPMODE_CCFS2_SET(add_ie, ccfs2)	do { /* set CCFS2 */ \
	(add_ie)->opmode &= htol16(~HT_OPMODE_CCFS2_MASK); \
	(add_ie)->opmode |= htol16(((ccfs2) << HT_OPMODE_CCFS2_SHIFT) & HT_OPMODE_CCFS2_MASK); \
} while (0)

/* Macros for HT MCS field access */
#define HT_CAP_MCS_BITMASK(supp_mcs)                 \
	((supp_mcs)[HT_CAP_MCS_RX_8TO15_BYTE_OFFSET])
#define HT_CAP_MCS_TX_RX_UNEQUAL(supp_mcs)          \
	((supp_mcs)[HT_CAP_MCS_FLAGS_SUPP_BYTE] & HT_CAP_MCS_FLAGS_TX_RX_UNEQUAL)
#define HT_CAP_MCS_TX_STREAM_SUPPORT(supp_mcs)          \
		((supp_mcs)[HT_CAP_MCS_FLAGS_SUPP_BYTE] & HT_CAP_MCS_FLAGS_MAX_SPATIAL_STREAM_MASK)

BWL_PRE_PACKED_STRUCT struct obss_params {
	uint16	passive_dwell;
	uint16	active_dwell;
	uint16	bss_widthscan_interval;
	uint16	passive_total;
	uint16	active_total;
	uint16	chanwidth_transition_dly;
	uint16	activity_threshold;
} BWL_POST_PACKED_STRUCT;
typedef struct obss_params obss_params_t;

BWL_PRE_PACKED_STRUCT struct dot11_obss_ie {
	uint8	id;
	uint8	len;
	obss_params_t obss_params;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_obss_ie dot11_obss_ie_t;
#define DOT11_OBSS_SCAN_IE_LEN	sizeof(obss_params_t)	/* HT OBSS len (based on 802.11n d3.0) */

/* HT control field */
#define HT_CTRL_LA_TRQ		0x00000002	/* sounding request */
#define HT_CTRL_LA_MAI		0x0000003C	/* MCS request or antenna selection indication */
#define HT_CTRL_LA_MAI_SHIFT	2
#define HT_CTRL_LA_MAI_MRQ	0x00000004	/* MCS request */
#define HT_CTRL_LA_MAI_MSI	0x00000038	/* MCS request sequence identifier */
#define HT_CTRL_LA_MFSI		0x000001C0	/* MFB sequence identifier */
#define HT_CTRL_LA_MFSI_SHIFT	6
#define HT_CTRL_LA_MFB_ASELC	0x0000FE00	/* MCS feedback, antenna selection command/data */
#define HT_CTRL_LA_MFB_ASELC_SH	9
#define HT_CTRL_LA_ASELC_CMD	0x00000C00	/* ASEL command */
#define HT_CTRL_LA_ASELC_DATA	0x0000F000	/* ASEL data */
#define HT_CTRL_CAL_POS		0x00030000	/* Calibration position */
#define HT_CTRL_CAL_SEQ		0x000C0000	/* Calibration sequence */
#define HT_CTRL_CSI_STEERING	0x00C00000	/* CSI/Steering */
#define HT_CTRL_CSI_STEER_SHIFT	22
#define HT_CTRL_CSI_STEER_NFB	0		/* no fedback required */
#define HT_CTRL_CSI_STEER_CSI	1		/* CSI, H matrix */
#define HT_CTRL_CSI_STEER_NCOM	2		/* non-compressed beamforming */
#define HT_CTRL_CSI_STEER_COM	3		/* compressed beamforming */
#define HT_CTRL_NDP_ANNOUNCE	0x01000000	/* NDP announcement */
#define HT_CTRL_AC_CONSTRAINT	0x40000000	/* AC Constraint */
#define HT_CTRL_RDG_MOREPPDU	0x80000000	/* RDG/More PPDU */

/* ************* VHT definitions. ************* */

/**
 * VHT Capabilites IE (sec 8.4.2.160)
 */

BWL_PRE_PACKED_STRUCT struct vht_cap_ie {
	uint32  vht_cap_info;
	/* supported MCS set - 64 bit field */
	uint16	rx_mcs_map;
	uint16  rx_max_rate;
	uint16  tx_mcs_map;
	uint16	tx_max_rate;
} BWL_POST_PACKED_STRUCT;
typedef struct vht_cap_ie vht_cap_ie_t;

/* 4B cap_info + 8B supp_mcs */
#define VHT_CAP_IE_LEN 12

/* VHT Capabilities Info field - 32bit - in VHT Cap IE */
#define VHT_CAP_INFO_MAX_MPDU_LEN_MASK          0x00000003
#define VHT_CAP_INFO_SUPP_CHAN_WIDTH_MASK       0x0000000c
#define VHT_CAP_INFO_LDPC                       0x00000010
#define VHT_CAP_INFO_SGI_80MHZ                  0x00000020
#define VHT_CAP_INFO_SGI_160MHZ                 0x00000040
#define VHT_CAP_INFO_TX_STBC                    0x00000080
#define VHT_CAP_INFO_RX_STBC_MASK               0x00000700
#define VHT_CAP_INFO_RX_STBC_SHIFT              8
#define VHT_CAP_INFO_SU_BEAMFMR                 0x00000800
#define VHT_CAP_INFO_SU_BEAMFMEE                0x00001000
#define VHT_CAP_INFO_NUM_BMFMR_ANT_MASK         0x0000e000
#define VHT_CAP_INFO_NUM_BMFMR_ANT_SHIFT        13
#define VHT_CAP_INFO_NUM_SOUNDING_DIM_MASK      0x00070000
#define VHT_CAP_INFO_NUM_SOUNDING_DIM_SHIFT     16
#define VHT_CAP_INFO_MU_BEAMFMR                 0x00080000
#define VHT_CAP_INFO_MU_BEAMFMEE                0x00100000
#define VHT_CAP_INFO_TXOPPS                     0x00200000
#define VHT_CAP_INFO_HTCVHT                     0x00400000
#define VHT_CAP_INFO_AMPDU_MAXLEN_EXP_MASK      0x03800000
#define VHT_CAP_INFO_AMPDU_MAXLEN_EXP_SHIFT     23
#define VHT_CAP_INFO_LINK_ADAPT_CAP_MASK        0x0c000000
#define VHT_CAP_INFO_LINK_ADAPT_CAP_SHIFT       26
#define VHT_CAP_INFO_EXT_NSS_BW_SUP_MASK        0xc0000000
#define VHT_CAP_INFO_EXT_NSS_BW_SUP_SHIFT       30

/* get Extended NSS BW Support passing vht cap info */
#define VHT_CAP_EXT_NSS_BW_SUP(cap_info) \
	(((cap_info) & VHT_CAP_INFO_EXT_NSS_BW_SUP_MASK) >> VHT_CAP_INFO_EXT_NSS_BW_SUP_SHIFT)

/* VHT CAP INFO extended NSS BW support - refer to IEEE 802.11 REVmc D8.0 Figure 9-559 */
#define VHT_CAP_INFO_EXT_NSS_BW_HALF_160	1 /* 160MHz at half NSS CAP */
#define VHT_CAP_INFO_EXT_NSS_BW_HALF_160_80P80	2 /* 160 & 80p80 MHz at half NSS CAP */

/* VHT Supported MCS Set - 64-bit - in VHT Cap IE */
#define VHT_CAP_SUPP_MCS_RX_HIGHEST_RATE_MASK   0x1fff
#define VHT_CAP_SUPP_MCS_RX_HIGHEST_RATE_SHIFT  0
#define VHT_CAP_SUPP_CHAN_WIDTH_SHIFT		5

#define VHT_CAP_SUPP_MCS_TX_HIGHEST_RATE_MASK   0x1fff
#define VHT_CAP_SUPP_MCS_TX_HIGHEST_RATE_SHIFT  0

/* defines for field(s) in vht_cap_ie->rx_max_rate */
#define VHT_CAP_MAX_NSTS_MASK			0xe000
#define VHT_CAP_MAX_NSTS_SHIFT			13

/* defines for field(s) in vht_cap_ie->tx_max_rate */
#define VHT_CAP_EXT_NSS_BW_CAP			0x2000

#define VHT_CAP_MCS_MAP_0_7                     0
#define VHT_CAP_MCS_MAP_0_8                     1
#define VHT_CAP_MCS_MAP_0_9                     2
#define VHT_CAP_MCS_MAP_NONE                    3
#define VHT_CAP_MCS_MAP_S                       2 /* num bits for 1-stream */
#define VHT_CAP_MCS_MAP_M                       0x3 /* mask for 1-stream */
/* assumes VHT_CAP_MCS_MAP_NONE is 3 and 2 bits are used for encoding */
#define VHT_CAP_MCS_MAP_NONE_ALL                0xffff

/* VHT rates bitmap */
#define VHT_CAP_MCS_0_7_RATEMAP		0x00ff
#define VHT_CAP_MCS_0_8_RATEMAP		0x01ff
#define VHT_CAP_MCS_0_9_RATEMAP		0x03ff
#define VHT_CAP_MCS_FULL_RATEMAP 	VHT_CAP_MCS_0_9_RATEMAP

#define VHT_PROP_MCS_MAP_10_11                   0
#define VHT_PROP_MCS_MAP_UNUSED1                 1
#define VHT_PROP_MCS_MAP_UNUSED2                 2
#define VHT_PROP_MCS_MAP_NONE                    3
#define VHT_PROP_MCS_MAP_NONE_ALL                0xffff

/* VHT prop rates bitmap */
#define VHT_PROP_MCS_10_11_RATEMAP	0x0c00
#define VHT_PROP_MCS_FULL_RATEMAP	VHT_PROP_MCS_10_11_RATEMAP

#if !defined(VHT_CAP_MCS_MAP_0_9_NSS3)
/* mcsmap with MCS0-9 for Nss = 3 */
#define VHT_CAP_MCS_MAP_0_9_NSS3 \
	        ((VHT_CAP_MCS_MAP_0_9 << VHT_MCS_MAP_GET_SS_IDX(1)) | \
	         (VHT_CAP_MCS_MAP_0_9 << VHT_MCS_MAP_GET_SS_IDX(2)) | \
	         (VHT_CAP_MCS_MAP_0_9 << VHT_MCS_MAP_GET_SS_IDX(3)))
#endif /* !VHT_CAP_MCS_MAP_0_9_NSS3 */

#define VHT_CAP_MCS_MAP_NSS_MAX                 8

/* get mcsmap with given mcs for given nss streams */
#define VHT_CAP_MCS_MAP_CREATE(mcsmap, nss, mcs) \
	do { \
		int i; \
		for (i = 1; i <= nss; i++) { \
			VHT_MCS_MAP_SET_MCS_PER_SS(i, mcs, mcsmap); \
		} \
	} while (0)

/* Map the mcs code to mcs bit map */
#define VHT_MCS_CODE_TO_MCS_MAP(mcs_code) \
	((mcs_code == VHT_CAP_MCS_MAP_0_7) ? VHT_CAP_MCS_0_7_RATEMAP : \
	 (mcs_code == VHT_CAP_MCS_MAP_0_8) ? VHT_CAP_MCS_0_8_RATEMAP : \
	 (mcs_code == VHT_CAP_MCS_MAP_0_9) ? VHT_CAP_MCS_0_9_RATEMAP : 0)

#define VHT_PROP_MCS_CODE_TO_PROP_MCS_MAP(mcs_code) \
	((mcs_code == VHT_PROP_MCS_MAP_10_11) ? VHT_PROP_MCS_10_11_RATEMAP : 0)

/* Map the mcs bit map to mcs code */
#define VHT_MCS_MAP_TO_MCS_CODE(mcs_map) \
	((mcs_map == VHT_CAP_MCS_0_7_RATEMAP) ? VHT_CAP_MCS_MAP_0_7 : \
	 (mcs_map == VHT_CAP_MCS_0_8_RATEMAP) ? VHT_CAP_MCS_MAP_0_8 : \
	 (mcs_map == VHT_CAP_MCS_0_9_RATEMAP) ? VHT_CAP_MCS_MAP_0_9 : VHT_CAP_MCS_MAP_NONE)

#define VHT_PROP_MCS_MAP_TO_PROP_MCS_CODE(mcs_map) \
	(((mcs_map & 0xc00) == 0xc00)  ? VHT_PROP_MCS_MAP_10_11 : VHT_PROP_MCS_MAP_NONE)

/** VHT Capabilities Supported Channel Width */
typedef enum vht_cap_chan_width {
	VHT_CAP_CHAN_WIDTH_SUPPORT_MANDATORY = 0x00,
	VHT_CAP_CHAN_WIDTH_SUPPORT_160       = 0x04,
	VHT_CAP_CHAN_WIDTH_SUPPORT_160_8080  = 0x08
} vht_cap_chan_width_t;

/** VHT Capabilities Supported max MPDU LEN (sec 8.4.2.160.2) */
typedef enum vht_cap_max_mpdu_len {
	VHT_CAP_MPDU_MAX_4K     = 0x00,
	VHT_CAP_MPDU_MAX_8K     = 0x01,
	VHT_CAP_MPDU_MAX_11K    = 0x02
} vht_cap_max_mpdu_len_t;

/* Maximum MPDU Length byte counts for the VHT Capabilities advertised limits */
#define VHT_MPDU_LIMIT_4K        3895
#define VHT_MPDU_LIMIT_8K        7991
#define VHT_MPDU_LIMIT_11K      11454

/**
 * VHT Operation IE (sec 8.4.2.161)
 */

BWL_PRE_PACKED_STRUCT struct vht_op_ie {
	uint8	chan_width;
	uint8	chan1;
	uint8	chan2;
	uint16	supp_mcs;  /*  same def as above in vht cap */
} BWL_POST_PACKED_STRUCT;
typedef struct vht_op_ie vht_op_ie_t;

/* 3B VHT Op info + 2B Basic MCS */
#define VHT_OP_IE_LEN 5

/* IEEE P802.11-REVme/D1.3 Table 9-313 VHT Operation Information subfields */
typedef enum vht_op_chan_width {
	VHT_OP_CHAN_WIDTH_20_40	= 0,
	VHT_OP_CHAN_WIDTH_80	= 1,
	VHT_OP_CHAN_WIDTH_160	= 2, /* deprecated - IEEE 802.11 REVmc D8.0 Table 11-25 */
	VHT_OP_CHAN_WIDTH_80_80	= 3  /* deprecated - IEEE 802.11 REVmc D8.0 Table 11-25 */
} vht_op_chan_width_t;

/* AID length */
#define AID_IE_LEN		2
/**
 * BRCM vht features IE header
 * The header if the fixed part of the IE
 * On the 5GHz band this is the entire IE,
 * on 2.4GHz the VHT IEs as defined in the 802.11ac
 * specification follows
 *
 *
 * VHT features rates  bitmap.
 * Bit0:		5G MCS 0-9 BW 160MHz
 * Bit1:		5G MCS 0-9 support BW 80MHz
 * Bit2:		5G MCS 0-9 support BW 20MHz
 * Bit3:		2.4G MCS 0-9 support BW 20MHz
 * Bits:4-7	Reserved for future use
 *
 */
#define VHT_FEATURES_IE_TYPE	0x4
BWL_PRE_PACKED_STRUCT struct vht_features_ie_hdr {
	uint8 oui[3];
	uint8 type;		/* type of this IE = 4 */
	uint8 rate_mask;	/* VHT rate mask */
} BWL_POST_PACKED_STRUCT;
typedef struct vht_features_ie_hdr vht_features_ie_hdr_t;

/* Def for rx & tx basic mcs maps - ea ss num has 2 bits of info */
#define VHT_MCS_MAP_GET_SS_IDX(nss) (((nss)-1) * VHT_CAP_MCS_MAP_S)
#define VHT_MCS_MAP_GET_MCS_PER_SS(nss, mcsMap) \
	(((mcsMap) >> VHT_MCS_MAP_GET_SS_IDX(nss)) & VHT_CAP_MCS_MAP_M)
#define VHT_MCS_MAP_SET_MCS_PER_SS(nss, numMcs, mcsMap) \
	do { \
	 (mcsMap) &= (~(VHT_CAP_MCS_MAP_M << VHT_MCS_MAP_GET_SS_IDX(nss))); \
	 (mcsMap) |= (((numMcs) & VHT_CAP_MCS_MAP_M) << VHT_MCS_MAP_GET_SS_IDX(nss)); \
	} while (0)
#define VHT_MCS_SS_SUPPORTED(nss, mcsMap) \
		 (VHT_MCS_MAP_GET_MCS_PER_SS((nss), (mcsMap)) != VHT_CAP_MCS_MAP_NONE)

/* Get the max ss supported from the mcs map */
#define VHT_MAX_SS_SUPPORTED(mcsMap) \
	VHT_MCS_SS_SUPPORTED(8, mcsMap) ? 8 : \
	VHT_MCS_SS_SUPPORTED(7, mcsMap) ? 7 : \
	VHT_MCS_SS_SUPPORTED(6, mcsMap) ? 6 : \
	VHT_MCS_SS_SUPPORTED(5, mcsMap) ? 5 : \
	VHT_MCS_SS_SUPPORTED(4, mcsMap) ? 4 : \
	VHT_MCS_SS_SUPPORTED(3, mcsMap) ? 3 : \
	VHT_MCS_SS_SUPPORTED(2, mcsMap) ? 2 : \
	VHT_MCS_SS_SUPPORTED(1, mcsMap) ? 1 : 0

/* ************* WPA definitions. ************* */
#define WPA_OUI			"\x00\x50\xF2"	/* WPA OUI */
#define WPA_OUI_LEN		3		/* WPA OUI length */
#ifndef WPA_OUI_TYPE
#define WPA_OUI_TYPE		1
#endif // endif
#define WPA_VERSION		1		/* WPA version */
#define WPA2_OUI		"\x00\x0F\xAC"	/* WPA2 OUI */
#define WPA2_OUI_LEN		3		/* WPA2 OUI length */
#define WPA2_VERSION		1		/* WPA2 version */
#define WPA2_VERSION_LEN	2		/* WAP2 version length */
#define WPA3_OUI               "\x50\x6F\x9A"   /* WPA3 OUI */
#define WPA3_OUI_LEN		3		/* WPA3 OUI length */

/* ************* WPS definitions. ************* */
#define WPS_OUI			"\x00\x50\xF2"	/* WPS OUI */
#define WPS_OUI_LEN		3		/* WPS OUI length */
#define WPS_OUI_TYPE		4

/* ************* WFA definitions. ************* */
#if defined(MACOSX)
#define MAC_OUI			"\x00\x17\xF2"	/* MACOSX OUI */
#define MAC_OUI_TYPE_P2P	5
#endif /* MACOSX */

#if defined(MACOSX) && !defined(WLP2P_NEW_WFA_OUI)
#define WFA_OUI			WPS_OUI		/* WFA OUI */
#else
#ifdef P2P_IE_OVRD
#define WFA_OUI			MAC_OUI
#else
#define WFA_OUI			"\x50\x6F\x9A"	/* WFA OUI */
#endif /* P2P_IE_OVRD */
#endif /* MACOSX && !WLP2P_NEW_WFA_OUI */
#define WFA_OUI_LEN		3		/* WFA OUI length */
#ifdef P2P_IE_OVRD
#define WFA_OUI_TYPE_P2P	MAC_OUI_TYPE_P2P
#else
#define WFA_OUI_TYPE_TPC	8
#define WFA_OUI_TYPE_P2P	9
#endif // endif

#define WFA_OUI_TYPE_TPC	8
#ifdef WLTDLS
#define WFA_OUI_TYPE_TPQ	4	/* WFD Tunneled Probe ReQuest */
#define WFA_OUI_TYPE_TPS	5	/* WFD Tunneled Probe ReSponse */
#define WFA_OUI_TYPE_WFD	10
#endif /* WTDLS */
#define WFA_OUI_TYPE_HS20	0x10
#define WFA_OUI_TYPE_OSEN	0x12
#define WFA_OUI_TYPE_MBO	0x16
#define WFA_OUI_TYPE_MBO_OCE	0x16
#define WFA_OUI_TYPE_OWE        0x1C
#define WFA_OUI_TYPE_SAE_PK		0x1F
#define WFA_OUI_TYPE_TD_INDICATION	0x20 /* Transition disable ind */

/* DPP Action frame fixed params */
#define DOT11_DPP_AF_FIXED_PARAMS	"\x04\x09\x50\x6F\x9A\x1A"
#define DOT11_DPP_AF_FIXED_PARAMS_LEN	6

#define SAE_PK_MOD_LEN		32u
BWL_PRE_PACKED_STRUCT struct dot11_sae_pk_element {
	uint8 id;			/* IE ID, 221, DOT11_MNG_PROPR_ID */
	uint8 len;			/* IE length */
	uint8 oui[WFA_OUI_LEN];		/* WFA_OUI */
	uint8 type;			/* SAE-PK */
	uint8 data[SAE_PK_MOD_LEN];	/* Modifier. 32Byte fixed */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_sae_pk_element dot11_sae_pk_element_t;

/* RSN authenticated key managment suite */
#define RSN_AKM_NONE		0	/* None (IBSS) */
#define RSN_AKM_UNSPECIFIED	1	/* Over 802.1x */
#define RSN_AKM_PSK		2	/* Pre-shared Key */
#define RSN_AKM_FBT_1X		3	/* Fast Bss transition using 802.1X */
#define RSN_AKM_FBT_PSK		4	/* Fast Bss transition using Pre-shared Key */
/* RSN_AKM_MFP_1X and RSN_AKM_MFP_PSK are not used any more
 * Just kept here to avoid build issue in BISON/CARIBOU branch
 */
#define RSN_AKM_MFP_1X		5	/* SHA256 key derivation, using 802.1X */
#define RSN_AKM_MFP_PSK		6	/* SHA256 key derivation, using Pre-shared Key */
#define RSN_AKM_SHA256_1X	5	/* SHA256 key derivation, using 802.1X */
#define RSN_AKM_SHA256_PSK	6	/* SHA256 key derivation, using Pre-shared Key */
#define RSN_AKM_TPK		7	/* TPK(TDLS Peer Key) handshake */
#define RSN_AKM_SAE_PSK		8	/* AKM for SAE with 4-way handshake */
#define RSN_AKM_SAE_FBT		9	/* AKM for SAE with FBT */
#define RSN_AKM_SUITEB		12	/* AKM for SUITE B */
#define RSN_AKM_FILS_SHA256	14	/* SHA256 key derivation, using FILS */
#define RSN_AKM_FILS_SHA384	15	/* SHA384 key derivation, using FILS */
#define RSN_AKM_OWE		18	/* AKM for OWE */
#define RSN_AKM_DPP		2       /* DPP */
#define RSN_AKM_PASN            21      /* PASN Authentication */
#define RSN_AKM_SAE_PSK_EXT     24       /* AKM for SAE-EXT with 4-way handshake */
#define RSN_AKM_SAE_FBT_EXT     25       /* AKM for SAE-EXT with FBT */

/* OSEN authenticated key managment suite */
#define OSEN_AKM_UNSPECIFIED	RSN_AKM_UNSPECIFIED	/* Over 802.1x */

/* Key related defines */
#define DOT11_MAX_DEFAULT_KEYS	4	/* number of default keys */
#define DOT11_MAX_IGTK_KEYS		2
#define DOT11_MAX_BIGTK_KEYS		2
#define DOT11_MAX_KEY_SIZE	32	/* max size of any key */
#define DOT11_MAX_IV_SIZE	16	/* max size of any IV */
#define DOT11_EXT_IV_FLAG	(1<<5)	/* flag to indicate IV is > 4 bytes */
#define DOT11_FTM_PFT_IV_FLAG   0x10    /* Protected Fine Timing frame flag in IV */
#define DOT11_WPA_KEY_RSC_LEN   8       /* WPA RSC key len */

#define WEP1_KEY_SIZE		5	/* max size of any WEP key */
#define WEP1_KEY_HEX_SIZE	10	/* size of WEP key in hex. */
#define WEP128_KEY_SIZE		13	/* max size of any WEP key */
#define WEP128_KEY_HEX_SIZE	26	/* size of WEP key in hex. */
#define TKIP_MIC_SIZE		8	/* size of TKIP MIC */
#define TKIP_EOM_SIZE		7	/* max size of TKIP EOM */
#define TKIP_EOM_FLAG		0x5a	/* TKIP EOM flag byte */
#define TKIP_KEY_SIZE		32	/* size of any TKIP key, includs MIC keys */
#define TKIP_TK_SIZE		16
#define TKIP_MIC_KEY_SIZE	8
#define TKIP_MIC_AUTH_TX	16	/* offset to Authenticator MIC TX key */
#define TKIP_MIC_AUTH_RX	24	/* offset to Authenticator MIC RX key */
#define TKIP_MIC_SUP_RX		TKIP_MIC_AUTH_TX	/* offset to Supplicant MIC RX key */
#define TKIP_MIC_SUP_TX		TKIP_MIC_AUTH_RX	/* offset to Supplicant MIC TX key */
#define AES_KEY_SIZE		16	/* size of AES key */
#define AES_MIC_SIZE		8	/* size of AES MIC */
#define BIP_KEY_SIZE		16	/* size of BIP key */
#define BIP_MIC_SIZE		8   /* sizeof BIP MIC */

#define AES_GCM_MIC_SIZE	16	/* size of MIC for 128-bit GCM - .11adD9 */

#define AES256_KEY_SIZE		32	/* size of AES 256 key - .11acD5 */
#define AES256_MIC_SIZE		16	/* size of MIC for 256 bit keys, incl BIP */

/* WCN */
#define WCN_OUI			"\x00\x50\xf2"	/* WCN OUI */
#define WCN_TYPE		4	/* WCN type */

/* 802.11r protocol definitions */

/** Mobility Domain IE */
BWL_PRE_PACKED_STRUCT struct dot11_mdid_ie {
	uint8 id;
	uint8 len;		/* DOT11_MDID_IE_DATA_LEN (3) */
	uint16 mdid;		/* Mobility Domain Id */
	uint8 cap;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mdid_ie dot11_mdid_ie_t;

/* length of data portion of Mobility Domain IE */
#define DOT11_MDID_IE_DATA_LEN	3

#define FBT_MDID_CAP_OVERDS	0x01	/* Fast Bss transition over the DS support */
#define FBT_MDID_CAP_RRP	0x02	/* Resource request protocol support */

/** Fast Bss Transition IE */
BWL_PRE_PACKED_STRUCT struct dot11_ft_ie {
	uint8 id;
	uint8 len;			/* At least equal to DOT11_FT_IE_FIXED_LEN (82) */
	uint16 mic_control;		/* Mic Control */
	uint8 mic[16];
	uint8 anonce[32];
	uint8 snonce[32];
	/* Optional sub-elements follow */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ft_ie dot11_ft_ie_t;

/* Fixed length of data portion of Fast BSS Transition IE. There could be
 * optional parameters, which if present, could raise the FT IE length to 255.
 */
#define DOT11_FT_IE_FIXED_LEN	82

/* OWE definitions */
/* ID + len + OUI + OI type + BSSID + SSID_len */
#define OWE_TRANS_MODE_IE_FIXED_LEN  13u

#define TIE_TYPE_RESERVED		0
#define TIE_TYPE_REASSOC_DEADLINE	1
#define TIE_TYPE_KEY_LIEFTIME		2
#define TIE_TYPE_ASSOC_COMEBACK		3
BWL_PRE_PACKED_STRUCT struct dot11_timeout_ie {
	uint8 id;
	uint8 len;
	uint8 type;		/* timeout interval type */
	uint32 value;		/* timeout interval value */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_timeout_ie dot11_timeout_ie_t;

/** GTK ie */
BWL_PRE_PACKED_STRUCT struct dot11_gtk_ie {
	uint8 id;
	uint8 len;
	uint16 key_info;
	uint8 key_len;
	uint8 rsc[8];
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_gtk_ie dot11_gtk_ie_t;

/** IGTK ie */
BWL_PRE_PACKED_STRUCT struct dot11_igtk_ie {
	uint8 sub_id;
	uint8 len;
	uint16 key_id;
	uint8 pn[6];
	uint8 key_len;
	uint8 key[16];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_igtk_ie dot11_igtk_ie_t;

#define DOT11_IGTK_LEN 25	/* Fixed length */

/** Management MIC ie */
BWL_PRE_PACKED_STRUCT struct mmic_ie {
	uint8   id;					/* IE ID: DOT11_MNG_MMIE_ID */
	uint8   len;				/* IE length */
	uint16  key_id;				/* key id */
	uint8   ipn[6];				/* ipn */
	uint8   mic[16];			/* mic */
} BWL_POST_PACKED_STRUCT;
typedef struct mmic_ie mmic_ie_t;

#define DOT11_MMIC_IE_HDR_SIZE (OFFSETOF(mmic_ie_t, mic))

/* 802.11r-2008, 11A.10.3 - RRB frame format */
BWL_PRE_PACKED_STRUCT struct dot11_ft_rrb_frame {
	uint8  frame_type; /* 1 for RRB */
	uint8  packet_type; /* 0 for Request 1 for Response */
	uint16 len;
	uint8  cur_ap_addr[ETHER_ADDR_LEN];
	uint8  data[1];	/* IEs Received/Sent in FT Action Req/Resp Frame */
} BWL_POST_PACKED_STRUCT;

typedef struct dot11_ft_rrb_frame dot11_ft_rrb_frame_t;

#define DOT11_FT_RRB_FIXED_LEN 10
#define DOT11_FT_REMOTE_FRAME_TYPE 1
#define DOT11_FT_PACKET_REQ 0
#define DOT11_FT_PACKET_RESP 1

#define BSSID_INVALID           "\x00\x00\x00\x00\x00\x00"
#define BSSID_BROADCAST         "\xFF\xFF\xFF\xFF\xFF\xFF"

/* ************* WMM Parameter definitions. ************* */
#define WMM_OUI			"\x00\x50\xF2"	/* WNN OUI */
#define WMM_OUI_LEN		3		/* WMM OUI length */
#define WMM_OUI_TYPE	2		/* WMM OUT type */
#define WMM_VERSION		1
#define WMM_VERSION_LEN	1

/* WMM OUI subtype */
#define WMM_OUI_SUBTYPE_PARAMETER	1
#define WMM_PARAMETER_IE_LEN		24

/** Link Identifier Element */
BWL_PRE_PACKED_STRUCT struct link_id_ie {
	uint8 id;
	uint8 len;
	struct ether_addr	bssid;
	struct ether_addr	tdls_init_mac;
	struct ether_addr	tdls_resp_mac;
} BWL_POST_PACKED_STRUCT;
typedef struct link_id_ie link_id_ie_t;
#define TDLS_LINK_ID_IE_LEN		18

/** Link Wakeup Schedule Element */
BWL_PRE_PACKED_STRUCT struct wakeup_sch_ie {
	uint8 id;
	uint8 len;
	uint32 offset;			/* in ms between TSF0 and start of 1st Awake Window */
	uint32 interval;		/* in ms bwtween the start of 2 Awake Windows */
	uint32 awake_win_slots;	/* in backof slots, duration of Awake Window */
	uint32 max_wake_win;	/* in ms, max duration of Awake Window */
	uint16 idle_cnt;		/* number of consecutive Awake Windows */
} BWL_POST_PACKED_STRUCT;
typedef struct wakeup_sch_ie wakeup_sch_ie_t;
#define TDLS_WAKEUP_SCH_IE_LEN		18

/** Channel Switch Timing Element */
BWL_PRE_PACKED_STRUCT struct channel_switch_timing_ie {
	uint8 id;
	uint8 len;
	uint16 switch_time;		/* in ms, time to switch channels */
	uint16 switch_timeout;	/* in ms */
} BWL_POST_PACKED_STRUCT;
typedef struct channel_switch_timing_ie channel_switch_timing_ie_t;
#define TDLS_CHANNEL_SWITCH_TIMING_IE_LEN		4

/** PTI Control Element */
BWL_PRE_PACKED_STRUCT struct pti_control_ie {
	uint8 id;
	uint8 len;
	uint8 tid;
	uint16 seq_control;
} BWL_POST_PACKED_STRUCT;
typedef struct pti_control_ie pti_control_ie_t;
#define TDLS_PTI_CONTROL_IE_LEN		3

/** PU Buffer Status Element */
BWL_PRE_PACKED_STRUCT struct pu_buffer_status_ie {
	uint8 id;
	uint8 len;
	uint8 status;
} BWL_POST_PACKED_STRUCT;
typedef struct pu_buffer_status_ie pu_buffer_status_ie_t;
#define TDLS_PU_BUFFER_STATUS_IE_LEN	1
#define TDLS_PU_BUFFER_STATUS_AC_BK		1
#define TDLS_PU_BUFFER_STATUS_AC_BE		2
#define TDLS_PU_BUFFER_STATUS_AC_VI		4
#define TDLS_PU_BUFFER_STATUS_AC_VO		8

/* TDLS Action Field Values */
#define TDLS_SETUP_REQ				0
#define TDLS_SETUP_RESP				1
#define TDLS_SETUP_CONFIRM			2
#define TDLS_TEARDOWN				3
#define TDLS_PEER_TRAFFIC_IND			4
#define TDLS_CHANNEL_SWITCH_REQ			5
#define TDLS_CHANNEL_SWITCH_RESP		6
#define TDLS_PEER_PSM_REQ			7
#define TDLS_PEER_PSM_RESP			8
#define TDLS_PEER_TRAFFIC_RESP			9
#define TDLS_DISCOVERY_REQ			10

/* 802.11z TDLS Public Action Frame action field */
#define TDLS_DISCOVERY_RESP			14

/* 802.11u GAS action frames */
#define GAS_REQUEST_ACTION_FRAME				10
#define GAS_RESPONSE_ACTION_FRAME				11
#define GAS_COMEBACK_REQUEST_ACTION_FRAME		12
#define GAS_COMEBACK_RESPONSE_ACTION_FRAME		13

/* tag_ID/length/value_buffer tuple */
typedef BWL_PRE_PACKED_STRUCT struct {
	uint8	id;
	uint8	len;
	uint8	data[1];
} BWL_POST_PACKED_STRUCT ftm_vs_tlv_t;

BWL_PRE_PACKED_STRUCT struct dot11_ftm_vs_ie {
	uint8 id;						/* DOT11_MNG_VS_ID */
	uint8 len;						/* length following */
	uint8 oui[3];					/* BRCM_PROP_OUI (or Customer) */
	uint8 sub_type;					/* BRCM_FTM_IE_TYPE (or Customer) */
	uint8 version;
	ftm_vs_tlv_t	tlvs[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ftm_vs_ie dot11_ftm_vs_ie_t;

/* same as payload of dot11_ftm_vs_ie.
* This definition helps in having struct access
* of pay load while building FTM VS IE from other modules
*/
BWL_PRE_PACKED_STRUCT struct dot11_ftm_vs_ie_pyld {
	uint8 sub_type;					/* BRCM_FTM_IE_TYPE (or Customer) */
	uint8 version;
	ftm_vs_tlv_t	tlvs[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ftm_vs_ie_pyld dot11_ftm_vs_ie_pyld_t;

/* ftm vs api version */
#define BCM_FTM_VS_PARAMS_VERSION 0x01

/* ftm vendor specific information tlv types */
enum {
	FTM_VS_TLV_NONE = 0,
	FTM_VS_TLV_REQ_PARAMS = 1,		/* additional request params (in FTM_REQ) */
	FTM_VS_TLV_MEAS_INFO = 2,		/* measurement information (in FTM_MEAS) */
	FTM_VS_TLV_SEC_PARAMS = 3,		/* security parameters (in either) */
	FTM_VS_TLV_SEQ_PARAMS = 4,		/* toast parameters (FTM_REQ, BRCM proprietary) */
	FTM_VS_TLV_MF_BUF = 5,			/* multi frame buffer - may span ftm vs ie's */
	FTM_VS_TLV_TIMING_PARAMS = 6,            /* timing adjustments */
	FTM_VS_TLV_MF_STATS_BUF = 7		/* multi frame statistics buffer */
	/* add additional types above */
};

/* the following definitions are *DEPRECATED* and moved to implemenetion files. They
 * are retained here because previous (May 2016) some branches use them
 */

BWL_PRE_PACKED_STRUCT struct dot11_ftm_vs_params {
	uint8 id;                       /* DOT11_MNG_VS_ID */
	uint8 len;
	uint8 oui[3];                   /* Proprietary OUI, BRCM_PROP_OUI */
	uint8 bcm_vs_id;
	ftm_vs_tlv_t ftm_tpk_ri_rr[1];          /* ftm_TPK_ri_rr place holder */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_ftm_vs_params dot11_ftm_vs_tpk_ri_rr_params_t;
#define DOT11_FTM_VS_LEN  (sizeof(dot11_ftm_vs_tpk_ri_rr_params_t) - TLV_HDR_LEN)
/* end *DEPRECATED* ftm definitions */

BWL_PRE_PACKED_STRUCT struct dot11_max_channel_switch_time_ie {
	uint8 id;		/* Extended - 255 11md D3.0  */
	uint8 len;
	uint8 id_ext;
	uint8 switch_time[3];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_max_channel_switch_time_ie dot11_max_channel_switch_time_ie_t;
#define DOT11_MAX_CHANNEL_SWITCH_TIME_IE_LEN		4

/* 802.11u interworking access network options */
#define IW_ANT_MASK					0x0f
#define IW_INTERNET_MASK				0x10
#define IW_ASRA_MASK					0x20
#define IW_ESR_MASK					0x40
#define IW_UESA_MASK					0x80

/* 802.11u interworking access network type */
#define IW_ANT_PRIVATE_NETWORK				0
#define IW_ANT_PRIVATE_NETWORK_WITH_GUEST		1
#define IW_ANT_CHARGEABLE_PUBLIC_NETWORK		2
#define IW_ANT_FREE_PUBLIC_NETWORK			3
#define IW_ANT_PERSONAL_DEVICE_NETWORK			4
#define IW_ANT_EMERGENCY_SERVICES_NETWORK		5
#define IW_ANT_TEST_NETWORK				14
#define IW_ANT_WILDCARD_NETWORK				15

#define IW_ANT_LEN			1
#define IW_VENUE_LEN			2
#define IW_HESSID_LEN			6
#define IW_HESSID_OFF			(IW_ANT_LEN + IW_VENUE_LEN)
#define IW_MAX_LEN			(IW_ANT_LEN + IW_VENUE_LEN + IW_HESSID_LEN)

/* 802.11u advertisement protocol */
#define ADVP_ANQP_PROTOCOL_ID				0
#define ADVP_MIH_PROTOCOL_ID				1

/* 802.11u advertisement protocol masks */
#define ADVP_QRL_MASK					0x7f
#define ADVP_PAME_BI_MASK				0x80

/* 802.11u advertisement protocol values */
#define ADVP_QRL_REQUEST				0x00
#define ADVP_QRL_RESPONSE				0x7f
#define ADVP_PAME_BI_DEPENDENT				0x00
#define ADVP_PAME_BI_INDEPENDENT			ADVP_PAME_BI_MASK

/* 802.11u ANQP information ID */
#define ANQP_ID_QUERY_LIST				256
#define ANQP_ID_CAPABILITY_LIST				257
#define ANQP_ID_VENUE_NAME_INFO				258
#define ANQP_ID_EMERGENCY_CALL_NUMBER_INFO		259
#define ANQP_ID_NETWORK_AUTHENTICATION_TYPE_INFO	260
#define ANQP_ID_ROAMING_CONSORTIUM_LIST			261
#define ANQP_ID_IP_ADDRESS_TYPE_AVAILABILITY_INFO	262
#define ANQP_ID_NAI_REALM_LIST				263
#define ANQP_ID_G3PP_CELLULAR_NETWORK_INFO		264
#define ANQP_ID_AP_GEOSPATIAL_LOCATION			265
#define ANQP_ID_AP_CIVIC_LOCATION			266
#define ANQP_ID_AP_LOCATION_PUBLIC_ID_URI		267
#define ANQP_ID_DOMAIN_NAME_LIST			268
#define ANQP_ID_EMERGENCY_ALERT_ID_URI			269
#define ANQP_ID_EMERGENCY_NAI				271
#define ANQP_ID_NEIGHBOR_REPORT				272
#define ANQP_ID_VENDOR_SPECIFIC_LIST			56797

/* 802.11u ANQP OUI */
#define ANQP_OUI_SUBTYPE				9

/* 802.11u venue name */
#define VENUE_LANGUAGE_CODE_SIZE			3
#define VENUE_NAME_SIZE					255

/* 802.11u venue groups */
#define VENUE_UNSPECIFIED				0
#define VENUE_ASSEMBLY					1
#define VENUE_BUSINESS					2
#define VENUE_EDUCATIONAL				3
#define VENUE_FACTORY					4
#define VENUE_INSTITUTIONAL				5
#define VENUE_MERCANTILE				6
#define VENUE_RESIDENTIAL				7
#define VENUE_STORAGE					8
#define VENUE_UTILITY					9
#define VENUE_VEHICULAR					10
#define VENUE_OUTDOOR					11

/* 802.11u network authentication type indicator */
#define NATI_UNSPECIFIED				-1
#define NATI_ACCEPTANCE_OF_TERMS_CONDITIONS		0
#define NATI_ONLINE_ENROLLMENT_SUPPORTED		1
#define NATI_HTTP_HTTPS_REDIRECTION			2
#define NATI_DNS_REDIRECTION				3

/* 802.11u IP address type availability - IPv6 */
#define IPA_IPV6_SHIFT					0
#define IPA_IPV6_MASK					(0x03 << IPA_IPV6_SHIFT)
#define	IPA_IPV6_NOT_AVAILABLE				0x00
#define IPA_IPV6_AVAILABLE				0x01
#define IPA_IPV6_UNKNOWN_AVAILABILITY			0x02

/* 802.11u IP address type availability - IPv4 */
#define IPA_IPV4_SHIFT					2
#define IPA_IPV4_MASK					(0x3f << IPA_IPV4_SHIFT)
#define	IPA_IPV4_NOT_AVAILABLE				0x00
#define IPA_IPV4_PUBLIC					0x01
#define IPA_IPV4_PORT_RESTRICT				0x02
#define IPA_IPV4_SINGLE_NAT				0x03
#define IPA_IPV4_DOUBLE_NAT				0x04
#define IPA_IPV4_PORT_RESTRICT_SINGLE_NAT		0x05
#define IPA_IPV4_PORT_RESTRICT_DOUBLE_NAT		0x06
#define IPA_IPV4_UNKNOWN_AVAILABILITY			0x07

/* 802.11u NAI realm encoding */
#define REALM_ENCODING_RFC4282				0
#define REALM_ENCODING_UTF8				1

/* 802.11u IANA EAP method type numbers */
#define REALM_EAP_TLS					13
#define REALM_EAP_LEAP					17
#define REALM_EAP_SIM					18
#define REALM_EAP_TTLS					21
#define REALM_EAP_AKA					23
#define REALM_EAP_PEAP					25
#define REALM_EAP_FAST					43
#define REALM_EAP_PSK					47
#define REALM_EAP_AKAP					50
#define REALM_EAP_EXPANDED				254

/* 802.11u authentication ID */
#define REALM_EXPANDED_EAP				1
#define REALM_NON_EAP_INNER_AUTHENTICATION		2
#define REALM_INNER_AUTHENTICATION_EAP			3
#define REALM_EXPANDED_INNER_EAP			4
#define REALM_CREDENTIAL				5
#define REALM_TUNNELED_EAP_CREDENTIAL			6
#define REALM_VENDOR_SPECIFIC_EAP			221

/* 802.11u non-EAP inner authentication type */
#define REALM_RESERVED_AUTH				0
#define REALM_PAP					1
#define REALM_CHAP					2
#define REALM_MSCHAP					3
#define REALM_MSCHAPV2					4

/* 802.11u credential type */
#define REALM_SIM					1
#define REALM_USIM					2
#define REALM_NFC					3
#define REALM_HARDWARE_TOKEN				4
#define REALM_SOFTOKEN					5
#define REALM_CERTIFICATE				6
#define REALM_USERNAME_PASSWORD				7
#define REALM_SERVER_SIDE				8
#define REALM_RESERVED_CRED				9
#define REALM_VENDOR_SPECIFIC_CRED			10

/* 802.11u 3GPP PLMN */
#define G3PP_GUD_VERSION				0
#define G3PP_PLMN_LIST_IE				0

/* AP Location Public ID Info encoding */
#define PUBLIC_ID_URI_FQDN_SE_ID		0
/* URI/FQDN Descriptor field values */
#define LOCATION_ENCODING_HELD			1
#define LOCATION_ENCODING_SUPL			2
#define URI_FQDN_SIZE					255

/** hotspot2.0 indication element (vendor specific) */
BWL_PRE_PACKED_STRUCT struct hs20_ie {
	uint8 oui[3];
	uint8 type;
	uint8 config;
} BWL_POST_PACKED_STRUCT;
typedef struct hs20_ie hs20_ie_t;
#define HS20_IE_LEN 5	/* HS20 IE length */

/** IEEE 802.11 Annex E */
typedef enum {
	DOT11_2GHZ_20MHZ_CLASS_12	= 81,	/* Ch 1-11 */
	DOT11_5GHZ_20MHZ_CLASS_1	= 115,	/* Ch 36-48 */
	DOT11_5GHZ_20MHZ_CLASS_2_DFS	= 118,	/* Ch 52-64 */
	DOT11_5GHZ_20MHZ_CLASS_3	= 124,	/* Ch 149-161 */
	DOT11_5GHZ_20MHZ_CLASS_4_DFS	= 121,	/* Ch 100-140 */
	DOT11_5GHZ_20MHZ_CLASS_5	= 125,	/* Ch 149-165 */
	DOT11_5GHZ_40MHZ_CLASS_22	= 116,	/* Ch 36-44,   lower */
	DOT11_5GHZ_40MHZ_CLASS_23_DFS 	= 119,	/* Ch 52-60,   lower */
	DOT11_5GHZ_40MHZ_CLASS_24_DFS	= 122,	/* Ch 100-132, lower */
	DOT11_5GHZ_40MHZ_CLASS_25	= 126,	/* Ch 149-157, lower */
	DOT11_5GHZ_40MHZ_CLASS_27	= 117,	/* Ch 40-48,   upper */
	DOT11_5GHZ_40MHZ_CLASS_28_DFS	= 120,	/* Ch 56-64,   upper */
	DOT11_5GHZ_40MHZ_CLASS_29_DFS	= 123,	/* Ch 104-136, upper */
	DOT11_5GHZ_40MHZ_CLASS_30	= 127,	/* Ch 153-161, upper */
	DOT11_2GHZ_40MHZ_CLASS_32	= 83,	/* Ch 1-7,     lower */
	DOT11_2GHZ_40MHZ_CLASS_33	= 84,	/* Ch 5-11,    upper */
} dot11_op_class_t;

/* QoS map */
#define QOS_MAP_FIXED_LENGTH	(8 * 2)	/* DSCP ranges fixed with 8 entries */

/* RAV QoS -- MSCS/SCS */
/** TCLAS Mask element */
BWL_PRE_PACKED_STRUCT struct dot11_tclas_mask_ie {
	uint8 id;				/* DOT11_MNG_ID_EXT_ID (255) */
	uint8 len;
	uint8 id_ext;				/* TCLAS_EXTID_MNG_MASK_ID (89) */
	dot11_tclas_fc_t fc;			/* Variable length frame classifier (fc) */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_tclas_mask_ie dot11_tclas_mask_ie_t;
#define DOT11_TCLAS_MASK_IE_LEN		1u	/* Fixed length, excludes id and len */
#define DOT11_TCLAS_MASK_IE_HDR_LEN	3u	/* Fixed length */

/* Bitmap definitions for the User Priority Bitmap
 * Each bit in the bitmap corresponds to a user priority.
 */
#define DOT11_UP_CTRL_UP_0		0u
#define DOT11_UP_CTRL_UP_1		1u
#define DOT11_UP_CTRL_UP_2		2u
#define DOT11_UP_CTRL_UP_3		3u
#define DOT11_UP_CTRL_UP_4		4u
#define DOT11_UP_CTRL_UP_5		5u
#define DOT11_UP_CTRL_UP_6		6u
#define DOT11_UP_CTRL_UP_7		7u

/* User priority control (up_ctl)  macros */
#define DOT11_UPC_UP_BITMAP_MASK	0xFFu	/* UP bitmap mask */
#define DOT11_UPC_UP_BITMAP_SHIFT	0u	/* UP bitmap shift */
#define DOT11_UPC_UP_LIMIT_MASK		0x700u	/* UP limit mask */
#define DOT11_UPC_UP_LIMIT_SHIFT	8u	/* UP limit shift */

/* MSCS Request Types */
#define DOT11_MSCS_REQ_TYPE_ADD		0u
#define DOT11_MSCS_REQ_TYPE_REMOVE	1u
#define DOT11_MSCS_REQ_TYPE_CHANGE	2u

/** MSCS Descriptor element */
BWL_PRE_PACKED_STRUCT struct dot11_mscs_descr_ie {
	uint8  id;				/* DOT11_MNG_ID_EXT_ID (255) */
	uint8  len;
	uint8  id_ext;				/* MSCS_EXTID_MNG_DESCR_ID (88) */
	uint8  req_type;			/* MSCS request type */
	uint16 up_ctl;				/* User priority control:
						 * Bits 0..7, up_bitmap(8 bits);
						 * Bits 8..10, up_limit (3 bits)
						 * Bits 11..15 reserved (5 bits)
						 */
	uint32 stream_timeout;
	uint8  data[];
	/* optional tclas mask elements */	/* dot11_tclas_mask_ie_t */
	/* optional sub-elements */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mscs_descr_ie dot11_mscs_descr_ie_t;
#define DOT11_MSCS_DESCR_IE_OFFSET	2u	/* first 2 bytes are id and len */
#define DOT11_MSCS_DESCR_IE_LEN		8u	/* Fixed length, exludes id and len */
#define DOT11_MSCS_DESCR_IE_HDR_LEN	10u	/* Entire descriptor header length */

/** MSCS Request frame, refer section 9.6.18.6 in the spec 802.11-2020 */
BWL_PRE_PACKED_STRUCT struct dot11_mscs_req {
	uint8 category;				/* ACTION_RAV_STREAMING (19) */
	uint8 robust_action;			/* action: MSCS Req (4), MSCS Res (5), etc. */
	uint8 dialog_token;			/* To identify the MSCS request and response */
	dot11_mscs_descr_ie_t mscs_descr;	/* MSCS descriptor */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mscs_req dot11_mscs_req_t;
#define DOT11_MSCS_REQ_HDR_LEN		3u	/* Fixed length */

/** MSCS Response frame, refer section 9.6.18.7 in the spec 802.11-2020 */
BWL_PRE_PACKED_STRUCT struct dot11_mscs_res {
	uint8  category;			/* ACTION_RAV_STREAMING (19) */
	uint8  robust_action;			/* action: MSCS Req (4), MSCS Res (5), etc. */
	uint8  dialog_token;			/* To identify the MSCS request and response */
	uint16 status;				/* status code */
	uint8  data[];				/* optional MSCS descriptor */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mscs_res dot11_mscs_res_t;
#define DOT11_MSCS_RES_HDR_LEN		5u	/* Fixed length */

/* MSCS subelement */
#define DOT11_MSCS_SUBELEM_ID_STATUS	1u	/* MSCS subelement ID for the status */

BWL_PRE_PACKED_STRUCT struct dot11_mscs_subelement {
	uint8 id;				/* MSCS specific subelement ID */
	uint8 len;				/* Length in bytes */
	uint8 data[];				/* Subelement specific data */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_mscs_subelement dot11_mscs_subelement_t;
#define DOT11_MSCS_DESCR_SUBELEM_IE_STATUS_LEN	2u	/* Subelement ID status length */

/** Generic dot11 sub element structure */
BWL_PRE_PACKED_STRUCT struct dot11_sub_elem {
	uint8 id;
	uint8 len;
	uint8 data[1];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_sub_elem dot11_sub_elem_t;
#define DOT11_SUB_ELEMENT_HDR_LEN	2u	/* Fixed length */
#define DOT11_SUB_ELEMENT_DATA_LEN_MIN	1u	/* Minimun amount of data */

/* char char direction */
#define DOT11_QOS_CHAR_DIR_UL			0x0u
#define DOT11_QOS_CHAR_DIR_DL			0x1u
#define DOT11_QOS_CHAR_DIR_DIRECT		0x2u

#define DOT11_QOS_CHAR_DIR_MASK			0x00000003u
#define DOT11_QOS_CHAR_DIR_SHIFT		0
#define DOT11_QOS_CHAR_TID_MASK			0x0000003Cu
#define DOT11_QOS_CHAR_TID_SHIFT		2
#define DOT11_QOS_CHAR_UP_MASK			0x000001C0u
#define DOT11_QOS_CHAR_UP_SHIFT			6
#define DOT11_QOS_CHAR_PRESENCE_MASK		0x01FFFE00u
#define DOT11_QOS_CHAR_PRESENCE_SHIFT		9
#define DOT11_QOS_CHAR_LINKID_MASK		0x1E000000u
#define DOT11_QOS_CHAR_LINKID_SHIFT		25

/* 9.4.2.316 QoS Characteristics element in Draft P802.11be_D3.0
 * Figure 9-1002as Control Info field in Draft P802.11be_D3.0
 */
#define DOT11_QOS_CHAR_DATA_RATE_LEN   3u
#define DOT11_QOS_CHAR_DELAY_BOUND_LEN 3u
BWL_PRE_PACKED_STRUCT struct dot11_qos_char_ie {
	uint8  id;						/* DOT11_MNG_ID_EXT_ID (255) */
	uint8  len;
	uint8  id_ext;						/* EXT_MNG_QOS_CHAR_ID (133) */
	uint32 ctrl_info;					/* bits 0..1   -> direction
								 * bits 2..5   -> tid
								 * bits 6..8   -> user priority
								 * bits 9..24  -> params bitmap
								 * bits 25..28 -> linkID
								 * bits 29..31 -> resreved
								 */
	uint32 min_srv_interval;				/* Minimum Service Interval in
								 * microseconds
								 */
	uint32 max_srv_interval;				/* Maximum Service Interval in
								 * microseconds
								 */
	uint8 min_data_rate[DOT11_QOS_CHAR_DATA_RATE_LEN];	/* Minimum Data Rate in kilobits
								 * per second
								 */
	uint8 delay_bound[DOT11_QOS_CHAR_DELAY_BOUND_LEN];	/* Delay Bound in microseconds */
	uint8  data[];
	/* optional Maxmimum MSDU Size */
	/* optional Service Start Time */
	/* optional Service Start Time LinkID */
	/* optional Mean Data Rate */
	/* optional Burst Size */
	/* optional MSDU Lifetime */
	/* optional MSDU Delivery Ratio */
	/* optional MSDU Count Exponent */
	/* optional Medium Time */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_qos_char_ie dot11_qos_char_ie_t;
#define DOT11_SCS_QOS_CHAR_IE_LEN	19u	/* Fixed length, exludes id and len */
#define DOT11_QOS_CHAR_IE_HDR_LEN	21u	/* Entire descriptor header length */

/* SCS elements */
BWL_PRE_PACKED_STRUCT union dot11_scs_sub_elem {
	uint8 data[1];
	dot11_intra_access_prio_ie_t	intra_ac_prio;
	dot11_tclas_ie_t		tclas_ie;
	dot11_tclas_proc_ie_t		tclas_proc_ie;
	dot11_qos_char_ie_t		qos_char_ie;
	dot11_sub_elem_t		sub_elem;
} BWL_POST_PACKED_STRUCT;
typedef union dot11_scs_sub_elem dot11_scs_sub_elem_t;
#define DOT11_SCS_SUB_ELEM_MIN_LEN		3u	/* id, len and one byte data */
/* max length is sub elem hdr len plus size of tclas_ie with fc4 ipv6 */
#define DOT11_SCS_SUB_ELEM_MAX_LEN	DOT11_TCLAS_FC_4_IPV6_LEN + DOT11_SUB_ELEMENT_HDR_LEN

/* SCS Request Types */
typedef enum {
	DOT11_SCS_REQ_TYPE_ADD = 0,
	DOT11_SCS_REQ_TYPE_REMOVE,
	DOT11_SCS_REQ_TYPE_CHANGE,
	DOT11_SCS_REQ_TYPE_MAX
} dot11_scs_req_type_t;

/** SCS Descriptor element */
BWL_PRE_PACKED_STRUCT struct dot11_scs_descr_ie {
	uint8  id;				/* DOT11_MNG_SCS_DESCR_ID (185) */
	uint8  len;
	uint8  scsID;				/* SCS descriptor id */
	uint8  req_type;			/* SCS request type(0/1/2) */
	uint8  data[];
	/* optional Intra-Access Category Priority element, dot11_intrac_ac_prio_ie_t */
	/* zero or more tclas elements, dot11_tclas_ie_t */
	/* optional tclas processing element, dot11_tclas_proc_ie_t */
	/* zero or one dot11_qos_char_ie_t */
	/* zero or more SCS sub-elements, dot11_scs_subelement_t */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_scs_descr_ie dot11_scs_descr_ie_t;
#define DOT11_SCS_IE_OFFSET		2u	/* first 2 bytes are id and len */
#define DOT11_ELEM_OFFSET		2u	/* first 2 bytes are id and len */
#define DOT11_SCS_SUB_ELEM_OFFSET	2u	/* 1 bytes SCS id and 1 byte req_type */
#define DOT11_SCS_DESCR_IE_HDR_LEN	4u	/* Entire descriptor header length */
#define DOT11_SCS_DATA_LEN_MIN		2u	/* When no sub element. 1B scsID and 1B req_type */
/** SCS Request frame, refer section 9.4.18.6 in the spec IEEE802.11REVmd (aka IEEE802.11-2020) */
BWL_PRE_PACKED_STRUCT struct dot11_scs_req {
	uint8 category;				/* ACTION_RAV_STREAMING (19) */
	uint8 robust_action;			/* action: SCS Req (0) */
	uint8 dialog_token;			/* To identify the SCS request and response */
	uint8 data[];				/* SCS descriptor list */
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_scs_req dot11_scs_req_t;
#define DOT11_SCS_REQ_HDR_LEN		3u	/* Fixed length */

BWL_PRE_PACKED_STRUCT struct dot11_scs_status_duple {
	uint8 scsID;
	uint16 status;
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_scs_status_duple dot11_scs_status_duple_t;

/** SCS Response frame, refer section 9.6.18.3 in 802.11-2020.
 * Currently, an approval(doc: 11-21/668r8) is pending in the IEEE802.11REVme.
 */
BWL_PRE_PACKED_STRUCT struct dot11_scs_res {
	uint8  category;	/* ACTION_RAV_STREAMING (19) */
	uint8  robust_action;	/* action: SCS Res (1) */
	uint8  dialog_token;	/* To identify the SCS request and response */
	uint8  count;		/* Specifies the number of items in the scs_status_list */
	dot11_scs_status_duple_t scs_status_list[];
} BWL_POST_PACKED_STRUCT;
typedef struct dot11_scs_res dot11_scs_res_t;
#define DOT11_SCS_RES_HDR_LEN		4u	/* Fixed length */

/* WPA3 Transition Mode bits */
#define TRANSISION_MODE_WPA3_PSK		BCM_BIT(0)
#define TRANSITION_MODE_SAE_PK			BCM_BIT(1)
#define TRANSITION_MODE_WPA3_ENTERPRISE		BCM_BIT(2)
#define TRANSITION_MODE_ENHANCED_OPEN		BCM_BIT(3)
#define TRANSITION_MODE_SUPPORTED_MASK (\
	TRANSITION_MODE_WPA3_PSK | \
	TRANSITION_MODE_SAE_PK | \
	TRANSITION_MODE_WPA3_ENTERPRISE | \
	TRANSITION_MODE_ENHANCED_OPEN)
/* This marks the end of a packed structure section. */
#include <packed_section_end.h>
#define TID_BMP_AC_BE	((1 << 0) | (1 << 3))
#define TID_BMP_AC_BK	((1 << 1) | (1 << 2))
#define TID_BMP_AC_VI	((1 << 4) | (1 << 5))
#define TID_BMP_AC_VO	((1 << 6) | (1 << 7))
#endif /* _802_11_H_ */
