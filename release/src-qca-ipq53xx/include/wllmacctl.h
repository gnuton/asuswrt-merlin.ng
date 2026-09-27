/*
 * LMAC API definitions for
 * Broadcom 802.11abg Networking Device Driver
 *
 * Definitions subject to change without notice.
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: wllmacctl.h,v 13.17 2008/02/13 04:00:11 Exp $:
 */

#ifndef _wllmacctl_h_
#define	_wllmacctl_h_

#include <typedefs.h>
#include <proto/ethernet.h>
#include <proto/bcmeth.h>
#include <proto/bcmevent.h>
#include <proto/802.11.h>

/* Init/Configure response */

typedef struct {
	uint8	vec[32];	/* bitvec of channels */
} lmacchanvec_t;

#define LMAC_CAP_MAGIC		0xdeadbeef
#define LMAC_VERSION_BUF_SIZE	64

typedef struct wl_lmac_cap {
	uint32		magic;
	uint32		hwrates;
	uint8		scan_ssid;
	uint8		bands;		/* bands */
	uint8		nmulticast;
	uint8		nratefallbackclasses;
	uint16		ssid_buffer_len;
	uint16		rxpacketflags;
	struct {
		int32	max_dBm;
		int32	min_dBm;
		uint32	step_dB;
	} txpower[3];
	uint32		capabilities;
	uint16		max_txbuffers;
	uint16		max_rxbuffers;

	wlc_rev_info_t	lmac_rev_info;
	uint32		radioid[2];
	lmacchanvec_t 	sup_chan[2];
	char		lmac_version[LMAC_VERSION_BUF_SIZE];
} wl_lmac_cap_t;

/* rateset defines */
#define LMAC_RATE_1M		0x00000001
#define LMAC_RATE_2M		0x00000002
#define LMAC_RATE_5M5		0x00000004
#define LMAC_RATE_6M		0x00000008
#define LMAC_RATE_9M		0x00000010
#define LMAC_RATE_11M		0x00000020
#define LMAC_RATE_12M		0x00000040
#define LMAC_RATE_18M		0x00000080
#define LMAC_RATE_22M		0x00000100
#define LMAC_RATE_24M		0x00000200
#define LMAC_RATE_33M		0x00000400
#define LMAC_RATE_36M		0x00000800
#define LMAC_RATE_48M		0x00001000
#define LMAC_RATE_54M		0x00002000

/* LMAC capabilities */
#define LMAC_CAP_MAXRXLIFETIME_AC	0x00000001
#define LMAC_CAP_IBSS_PS		0x00000002
#define LMAC_CAP_TRUNCATE_TXPOLICY	0x00000004
#define LMAC_CAP_PREAMBLE_OVERRIDE	0x00000008
#define LMAC_CAP_TXPWR_SENDPKT		0x00000010
#define LMAC_CAP_EXPIRY_TXPACKET	0x00000020
#define LMAC_CAP_PROBEFOR_JOIN		0x00000040
#define LMAC_CAP_MAXLIFETIME_Q		0x00000080
#define LMAC_CAP_TXNOACK_Q		0x00000100
#define LMAC_CAP_BLOCKACK_Q		0x00000200
#define LMAC_CAP_DOT11SLOTTIME		0x00000400
#define LMAC_CAP_WMM_SA			0x00000800
#define LMAC_CAP_SAPSD			0x00001000
#define LMAC_CAP_RM			0x00002000
#define LMAC_CAP_LEGACY_PSPOLL_Q	0x00004000
#define LMAC_CAP_WEP128			0x00008000
#define LMAC_CAP_MOREDATA_ACK		0x00010000
#define LMAC_CAP_SCAN_MINMAX_TIME	0x00020000
#define LMAC_CAP_TXAUTORATE		0x00040000
#define LMAC_CAP_HT			0x00080000
#define LMAC_CAP_STA			0x80000000	/* (BRCM) STA functions included */

/* LMAC Join params */
#define WLC_LMAC_JOIN_MODE_IBSS		0
#define WLC_LMAC_JOIN_MODE_BSS		1

/* Note: order of fields is chosen carefully for proper alignment */
typedef struct wl_lmac_join_params {
	uint32			mode;
	uint32			basic_rates;
	uint32			beacon_interval;	/* TUs */
	uint16			atim_window;
	struct ether_addr	bssid;
	uint8			band;
	uint8			channel;
	uint8			preamble;
	uint8			SSID_len;
	uint8			SSID[32];
	uint8			probe_for_join;
} wl_lmac_join_params_t;

typedef struct wl_lmac_bss_params {
	uint16			aid;
	uint8			dtim_interval;
} wl_lmac_bss_params_t;

typedef enum queue_id {
	Q_BE = 0,
	Q_BK,
	Q_VI,
	Q_VO,
	Q_HCCA
} queue_id_t;

typedef enum ps_scheme {
	PS_REGULAR = 0,
	PS_UAPSD,
	PS_LEGACY_PSPOLL,
	PS_SAPSD
} ps_scheme_t;

typedef enum ack_policy {
	ACKP_NORMAL = 0,
	ACKP_NOACK,
	ACKP_BLOCKACK
} ack_policy_t;

typedef struct wl_lmac_conf_q {
	queue_id_t 	qid;
	ack_policy_t	ack;
	ps_scheme_t	ps;
	uint8		pad;

	uint32		maxtxlifetime;	/* usec */
	struct {
		uint32	start_time;
		uint32	interval;
	} sapsd_conf;
	uint16		mediumtime;
} wl_lmac_conf_q_t;

typedef struct wl_lmac_conf_ac {
	uint16	cwmin[AC_COUNT];
	uint16	cwmax[AC_COUNT];
	uint8	aifs[AC_COUNT];
	uint16	txop[AC_COUNT];
	uint16	max_rxlifetime[AC_COUNT];
} wl_lmac_conf_ac_t;

#define TEMPLATE_BUFFER_LEN		(256 + 32)
#define TEMPLATE_BEACON_FRAME		0
#define TEMPLATE_PRBREQ_FRAME		1
#define TEMPLATE_NULLDATA_FRAME		2
#define TEMPLATE_PRBRESP_FRAME		3
#define TEMPLATE_QOSNULLDATA_FRAME	4
#define TEMPLATE_PSPOLL_FRAME		5
#define	TEMPLATE_COUNT			6	/* Must be last */

struct wl_lmac_frmtemplate {
	uint32	frm_type;
	uint32	rate;
	uint32	length;			/* Actual size of frm_data[] content only */
};

#define LMAC_FRMTEMPLATE_FIXED_SIZE	(sizeof(struct wl_lmac_frmtemplate))

#define BCNIE_FILTER_BIT0_MASK		0x01
#define BCNIE_FILTER_BIT1_MASK		0x02

typedef struct wl_lmac_bcnfilter {
	uint32	bcn_filter_enable;
	uint32	hostwake_bcn_count;
} wl_lmac_bcnfilter_t;

typedef struct wl_lmac_bcn_reg_ie {
	uint8	id;
	uint8	mask;
} wl_lmac_bcn_regie_t;

typedef struct wl_lmac_bcn_prop_ie {
	uint8	id;
	uint8	mask;
	uint8	OUI[3];
	uint8	type;
	uint16	ver;
} wl_lmac_bcn_propie_t;

typedef struct wl_lmac_bcniefilter {
	uint32		nies;
} wl_lmac_bcniefilter_t;

struct wl_lmac_txrate_class {
	uint8		retry_54Mbps;
	uint8		retry_48Mbps;
	uint8		retry_36Mbps;
	uint8		retry_33Mbps;
	uint8		retry_24Mbps;
	uint8		retry_22Mbps;
	uint8		retry_18Mbps;
	uint8		retry_12Mbps;
	uint8		retry_11Mbps;
	uint8		retry_9Mbps;
	uint8		retry_6Mbps;
	uint8		retry_5p5Mbps;
	uint8		retry_2Mbps;
	uint8		retry_1Mbps;
	uint8		SRL;
	uint8		LRL;
	uint32		iflags;
};

typedef struct wl_lmac_txrate_policy {
	uint32		nclasses;
	struct wl_lmac_txrate_class txrate_classes[1];
} wl_lmac_txrate_policy_t;

typedef struct wl_lmac_txautorate_policy {
	uint32		nclasses;
	uint8		data[1];
} wl_lmac_txautorate_policy_t;

typedef struct wl_lmac_setchannel {
	uint8 		channel;
	uint8		txp_user_target[45];
	uint8		txp_limit[45];
	uint8		override;
	uint8		txpwr_percent;
} wl_lmac_set_channel_t;

#define LMAC_WEP_DEFAULT_KEY		0
#define LMAC_WEP_PAIRWISE_KEY		1
#define LMAC_TKIP_GROUP_KEY		2
#define LMAC_TKIP_PAIRWISE_KEY		3
#define LMAC_AES_GROUP_KEY		4
#define LMAC_AES_PAIRWISE_KEY		5

typedef struct wl_lmac_addkey {
	uint8		keytype;
	uint8		keydata[64];
	uint8		keyindex;
} wl_lmac_addkey_t;

typedef struct wl_lmac_delkey {
	uint8		keyindex;
} wl_lmac_delkey_t;

typedef struct wl_bcmlmac_txdone {
	/* Length is D11_TXH_LEN(104),TXSTATUS_LEN(16),D11_PHY_HDR_LEN(6) */
	uint8		data[128];
} wl_bcmlmac_txdone_t;

#define LMAC_MODE_NORMAL	0
#define LMAC_MODE_EXTENDED	1	/* Expands format of txheader/rxheader/rxstatus/etc */

/* LMAC RX filter mode bits */
#define LMAC_PROMISC		0x00000001
#define LMAC_BSSIDFILTER	0x00000002
#define LMAC_MONITOR		0x80000000

/* Error codes that can be returned by LMAC */
#define LMAC_SUCCESS			0
#define LMAC_FAILURE			1
#define LMAC_RX_DECRYPT_FAILED		2
#define LMAC_RX_MICFAILURE		3
#define LMAC_SUCCESSXFER		4
#define	LMAC_PENDING			5
#define LMAC_TXQ_FULL			6
#define LMAC_EXCEED_RETRY		7
#define LMAC_LIFETIME_EXPIRED		8
#define LMAC_NOLINK			9
#define	LMAC_MAC_ERROR			10

/* LMAC events */
#define LMAC_EVENT_INITDONE		0
#define LMAC_EVENT_FATAL		1
#define LMAC_EVENT_BSSLOST		2
#define LMAC_EVENT_BSSREGAINED		3
#define LMAC_EVENT_RADARDETECT		4
#define LMAC_EVENT_LOWRSSI		5
#define LMAC_EVENT_SCANCOMPLETE		6
#define LMAC_EVENT_RMCOMPLETE		7
#define LMAC_EVENT_JOINCOMPLETE		8
#define LMAC_EVENT_PSCOMPLETE		9
#define LMAC_EVENT_LAST			10	/* Must be last */

typedef struct wllmac_join_data {
	int32	max_powerlevel;
	int32	min_powerlevel;
} wllmac_join_data_t;

typedef struct wllmac_pscomplete_data {
	uint32 pmstate;
} wllmac_pscomplete_data_t;

typedef struct wllmac_scancomplete_data {
	uint32 pmstate;
} wllmac_scancomplete_data_t;

#define LMAC_BAND2G		0x01
#define LMAC_BAND4G		0x02
#define LMAC_BAND5G		0x04
#define LMAC_BANDS_ALL		(LMAC_BAND2G | LMAC_BAND4G | LMAC_BAND5G)

/* LMAC Band and Channel defines, freq in MHz  */
#define LMAC_BAND2G_BASE		2407
#define LMAC_BAND4G_BASE		4900
#define LMAC_BAND5G_BASE		5000
#define LMAC_CHAN_FREQ_OFFSET		5

#define LMAC_CHAN_MAX			200

typedef struct wl_lmac_rmreq_params {
	int32		tx_power;
	uint32		channel;
	uint8		band;
	uint8		activation_delay;
	uint8		measurement_offset;
	uint8		nmeasures;
} wl_lmac_rmreq_params_t;

typedef struct wlc_lmac_rm_req {
	uint32		type;
	uint32		dur;
	uint32		resrved;
} wl_lmac_rm_req_t;

struct wlc_lmac_rm_bcn_measure {
	uint32		scan_mode;
};

#define LMAC_WAKEUP_BEACON			0
#define LMAC_WAKEUP_DTIMBEACON			1
#define LMAC_WAKEUP_NBEACONS			2
#define LMAC_WAKEUP_NDTIMBEACONS		3

#define LMAC_SLEEPMODE_WAKEUP			0
#define LMAC_SLEEPMODE_PDOWN			1
#define LMAC_SLEEPMODE_LPDOWN			2

#endif /* _wllmacctl_h_ */
