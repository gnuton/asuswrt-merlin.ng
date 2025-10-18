/*
 * Broadcom Event  protocol definitions
 *
 * Dependencies: bcmeth.h
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
 * $Id: bcmevent.h 837370 2024-03-06 12:11:38Z $
 *
 */

/*
 * Broadcom Ethernet Events protocol defines
 *
 */

#ifndef _BCMEVENT_H_
#define _BCMEVENT_H_

#include <typedefs.h>
#include <bcmwifi_channels.h>
/* #include <ethernet.h> -- TODO: req., excluded to overwhelming coupling (break up ethernet.h) */
#include <ethernet.h>
#include <bcmeth.h>
#if defined(HEALTH_CHECK) || defined(DNGL_EVENT_SUPPORT)
#include <dnglevent.h>
#endif /* HEALTH_CHECK || DNGL_EVENT_SUPPORT */

/* This marks the start of a packed structure section. */
#include <packed_section_start.h>

#define BCM_EVENT_MSG_VERSION		2	/* wl_event_msg_t struct version */
#define BCM_MSG_IFNAME_MAX		16	/* max length of interface name */

/* flags */
#define WLC_EVENT_MSG_LINK		0x01	/* link is up */
#define WLC_EVENT_MSG_FLUSHTXQ		0x02	/* flush tx queue on MIC error */
#define WLC_EVENT_MSG_GROUP		0x04	/* group MIC error */
#define WLC_EVENT_MSG_UNKBSS		0x08	/* unknown source bsscfg */
#define WLC_EVENT_MSG_UNKIF		0x10	/* unknown source OS i/f */

/* these fields are stored in network order */

/* version 1 */
typedef BWL_PRE_PACKED_STRUCT struct
{
	uint16	version;
	uint16	flags;			/* see flags below */
	uint32	event_type;		/* Message (see below) */
	uint32	status;			/* Status code (see below) */
	uint32	reason;			/* Reason code (if applicable) */
	uint32	auth_type;		/* WLC_E_AUTH */
	uint32	datalen;		/* data buf */
	struct ether_addr	addr;	/* Station address (if applicable) */
	char	ifname[BCM_MSG_IFNAME_MAX]; /* name of the packet incoming interface */
} BWL_POST_PACKED_STRUCT wl_event_msg_v1_t;

/* the current version */
typedef BWL_PRE_PACKED_STRUCT struct
{
	uint16	version;
	uint16	flags;			/* see flags below */
	uint32	event_type;		/* Message (see below) */
	uint32	status;			/* Status code (see below) */
	uint32	reason;			/* Reason code (if applicable) */
	uint32	auth_type;		/* WLC_E_AUTH */
	uint32	datalen;		/* data buf */
	struct ether_addr	addr;	/* Station address (if applicable) */
	char	ifname[BCM_MSG_IFNAME_MAX]; /* name of the packet incoming interface */
	uint8	ifidx;			/* destination OS i/f index */
	uint8	bsscfgidx;		/* source bsscfg index */
} BWL_POST_PACKED_STRUCT wl_event_msg_t;

/* used by driver msgs */
typedef BWL_PRE_PACKED_STRUCT struct bcm_event {
	struct ether_header eth;
	bcmeth_hdr_t		bcm_hdr;
	wl_event_msg_t		event;
	/* data portion follows */
} BWL_POST_PACKED_STRUCT bcm_event_t;

#define WLC_E_CEVENT_VER       1

#define CE_PKT_DUMP_LEN		32 /* bytest to dump in eapd layer */
#define CA_PKT_DUMP_LEN		32 /* bytest to dump in cevent_app */

/* to be filled in wl_cevent_t type field */
typedef enum {
	CEVENT_TYPE_CTRL	= 0,	/* control from cevent app */
	CEVENT_TYPE_D2C		= 1,	/* driver to cevent */
	CEVENT_TYPE_A2C		= 2,	/* app to cevent */
	CEVENT_TYPE_E2C		= 3,	/* eapd to cevent  */
	CEVENT_TYPE_D2A		= 4,	/* encap(driver to app) */
	CEVENT_TYPE_A2D		= 5,	/* encap(app to driver) */
	CEVENT_TYPE_LAST	= 6
} CEVENT_TYPE;

/* cevent subtypes.
 * cevent "type" field indicates origin of event/frame - either driver or application.
 * "subtype" filed indicates exact application(like ACSD, HOSTAPD, WPASUPP) if it is from
 * application else it will indicate DRIVER if it is received from Driver.
 */
typedef enum {
	CEVENT_ST_NAS		= 0,
	CEVENT_ST_WPS		= 1,
	CEVENT_ST_WAI		= 2,
	CEVENT_ST_XXX		= 3,
	CEVENT_ST_MEVENT	= 4,
	CEVENT_ST_BSD		= 5,
	CEVENT_ST_SSD		= 6,
	CEVENT_ST_DRSDBD	= 7,
	CEVENT_ST_ASPM		= 8,
	CEVENT_ST_VISDCOLL	= 9,
	CEVENT_ST_CA		= 10,		/* cevent_app */
	CEVENT_ST_EVENTD	= 11,
	CEVENT_ST_EAPD		= 12,
	CEVENT_ST_PBCD		= 13,
	CEVENT_ST_HOSTAPD	= 14,
	CEVENT_ST_WPASUPP	= 15,
	CEVENT_ST_WBD		= 16,
	CEVENT_ST_ACSD		= 17,
	CEVENT_ST_DRIVER	= 18,
	CEVENT_ST_LAST		= 19
} CEVENT_SUB_TYPE;

#define CE_ETH_HDR_MASK		1

#define CE_SET_ETH_HDR_PRESENT(ce_flags)	((ce_flags) |= CE_ETH_HDR_MASK)

#define CE_HAS_ETH_HDR(ce_flags)		((ce_flags) & CE_ETH_HDR_MASK)

/* Msgtypes used in CEVENT_TYPE_D2C */
typedef enum {
	CEVENT_D2C_MT_AUTH_TX = 0,
	CEVENT_D2C_MT_ASSOC_TX = 1,
	CEVENT_D2C_MT_EAP_TX = 2,
	CEVENT_D2C_MT_DISASSOC_TX = 3,
	CEVENT_D2C_MT_DEAUTH_TX = 4,
	CEVENT_D2C_MT_TX_LAST = 5, /* used to identify last in tx for flag macro */

	CEVENT_D2C_MT_AUTH_RX = 6,
	CEVENT_D2C_MT_ASSOC_RX = 7,
	CEVENT_D2C_MT_EAP_RX = 8,
	CEVENT_D2C_MT_DISASSOC_RX = 9,
	CEVENT_D2C_MT_DEAUTH_RX = 10,
	CEVENT_D2C_MT_RX_LAST = 11,

	CEVENT_D2C_MT_IF = 12,
	CEVENT_D2C_MT_BTM_REQ = 13,
	CEVENT_D2C_MT_BTM_RESP = 14,
	CEVENT_D2C_MT_BTM_QUERY = 15,

	CEVENT_D2C_MT_ARP_TX = 16,
	CEVENT_D2C_MT_ARP_RX = 17,
	CEVENT_D2C_MT_DHCP_TX = 18,
	CEVENT_D2C_MT_DHCP_RX = 19,
	CEVENT_D2C_MT_HC = 20, /* Health check */
	CEVENT_D2C_MT_LAST = 21
} CEVENT_D2C_MT;

/* Msgtypes used in CEVENT_TYPE_A2C */
typedef enum {
	CEVENT_A2C_MT_PTK_INSTALL = 0,
	CEVENT_A2C_MT_GTK_INSTALL = 1,
	CEVENT_A2C_MT_M1_RX = 2,
	CEVENT_A2C_MT_M1_TX = 3,
	CEVENT_A2C_MT_M2_RX = 4,
	CEVENT_A2C_MT_M2_TX = 5,
	CEVENT_A2C_MT_M3_RX = 6,
	CEVENT_A2C_MT_M3_TX = 7,
	CEVENT_A2C_MT_M4_RX = 8,
	CEVENT_A2C_MT_M4_TX = 9,
	CEVENT_A2C_MT_NAS_TIMEOUT = 10,
	CEVENT_A2C_MT_ACS_CH_SW = 11,
	CEVENT_A2C_MT_BTM_REQ = 12,
	CEVENT_A2C_MT_BTM_BRUTE_FORCE = 13,
	CEVENT_A2C_MT_AUTH_COMMIT_TX = 14,
	CEVENT_A2C_MT_AUTH_COMMIT_RX = 15,
	CEVENT_A2C_MT_AUTH_CONFIRM_TX = 16,
	CEVENT_A2C_MT_AUTH_CONFIRM_RX = 17,
	CEVENT_A2C_MT_LAST = 18
} CEVENT_A2C_MT;

#define CEVENT_D2C_FLAG_QUEUED	0 /* Frame queued */
#define CEVENT_D2C_FLAG_SUCCESS	1 /* Frame tx success */
#define CEVENT_D2C_FLAG_FAIL	2 /* Frame tx failure */
#define CEVENT_D2C_FLAG_READY	3 /* Frame ready */

/* bit0 of cevent->flags,if set implies presence of ethernet hdr in cevent->data.
 * bit31-bit24 of flags field in wl_cevent_t(see below) are reserved.
 * bit31, bit30 of cevent->flags represent the direction of frame i.e Rx or Tx as described below:
 * | bit31 | bit30 |
 * +-------+-------+
 * |   0   |   0   | Direction not relevant. Ex: AUTH_IND, ASSOC_IND etc.
 * |   0   |   1   | Rx i.e driver/app received the frame. Ex: AUTH_RX/M2_RX from driver/nas.
 * |   1   |   0   | Tx i.e driver/app transmitted/sent the frame. Ex: EAP_TX/M1_TX from driver/nas.
 * |   1   |   1   | Not in use for now.
 * +-------+-------+
 */

#define CEVENT_FRAME_DIR_RX_MASK	30
#define CEVENT_FRAME_DIR_TX_MASK	31
#define CEVENT_FRAME_DIR_RX             (1 << CEVENT_FRAME_DIR_RX_MASK)
#define CEVENT_FRAME_DIR_TX             (1 << CEVENT_FRAME_DIR_TX_MASK)

/* CEVENT BTM response reason codes */
#define CEVENT_BTM_RESP_REASON_ACCEPT			0 /* BTM Resp Accept */
#define CEVENT_BTM_RESP_REASON_REJECT			1 /* BTM Resp Reject */
#define CEVENT_BTM_RESP_REASON_REJ_INSUFF_BCN		2 /* BTM Resp Rej Insuff BCN */
#define CEVENT_BTM_RESP_REASON_REJ_INSUFF_CAP		3 /* BTM Resp Rej Insuff CAP */
#define CEVENT_BTM_RESP_REASON_REJ_BSS_TERM_UNDES	4 /* BTM Resp Rej BSS Termination */
#define CEVENT_BTM_RESP_REASON_REJ_BSS_TERM_DELAY_REQ	5 /* BTM Resp Rej BSS Term Del Rq */
#define CEVENT_BTM_RESP_REASON_REJ_STA_BSS_LIST_PROV	6 /* BTM Resp Rej STA Bss List */
#define CEVENT_BTM_RESP_REASON_REJ_NO_SUITABLE_BSS	7 /* BTM Resp Rej No Suitable BSS */
#define CEVENT_BTM_RESP_REASON_REJ_LEAVING_ESS		8 /* BTM Resp Rej Leaving ESS */
#define CEVENT_BTM_RESP_REASON_LAST			9 /* BTM Resp Last */

/* struture to communicate connectivity info/issues to cevent */
typedef struct {
	uint16  version;
	uint16  length;		/* sizeof(wl_cevent_t) + size of data to be encapsulated */
	uint16  type;		/* event/frame origin and destn */
	uint16  data_offset;	/* offset to 'data' from beginning of this struct.
				   Fields may be added between data_offset and data
				*/
	uint32	subtype;	/* src/destn application or Driver */
	uint32	msgtype;	/* message type */
	uint32	flags;		/* see above for usage description */
	uint64  timestamp;	/* in milliseconds */
	/* ADD MORE FIELDS HERE */
	uint8   data[];
} wl_cevent_t;

/* wl_cevent_t allows for new fields to be added before data[]. Must use data_offset to get data */
#define CEVENT_HDR_LEN(ce)	((ce)->data_offset ? (ce)->data_offset : sizeof(*(ce)))
#define CEVENT_DATA_LEN(ce)	(((ce)->length > CEVENT_HDR_LEN(ce)) ? \
		((ce)->length - CEVENT_HDR_LEN(ce)) : 0)
#define CEVENT_DATA(ce)		(((uint8*)(ce)) + CEVENT_HDR_LEN(ce))

/*
 * used by host event
 * note: if additional event types are added, it should go with is_wlc_event_frame() as well.
 */
typedef union bcm_event_msg_u {
	wl_event_msg_t		event;
#if defined(HEALTH_CHECK) || defined(DNGL_EVENT_SUPPORT)
	bcm_dngl_event_msg_t	dngl_event;
#endif /* HEALTH_CHECK || DNGL_EVENT_SUPPORT */

	/* add new event here */
} bcm_event_msg_u_t;

#define BCM_MSG_LEN	(sizeof(bcm_event_t) - sizeof(bcmeth_hdr_t) - sizeof(struct ether_header))

/* Event messages */
#define WLC_E_SET_SSID		0	/* indicates status of set SSID */
#define WLC_E_JOIN		1	/* differentiates join IBSS from found (WLC_E_START) IBSS */
#define WLC_E_START		2	/* STA founded an IBSS or AP started a BSS */
#define WLC_E_AUTH		3	/* 802.11 AUTH request */
#define WLC_E_AUTH_IND		4	/* 802.11 AUTH indication */
#define WLC_E_DEAUTH		5	/* 802.11 DEAUTH request */
#define WLC_E_DEAUTH_IND	6	/* 802.11 DEAUTH indication */
#define WLC_E_ASSOC		7	/* 802.11 ASSOC request */
#define WLC_E_ASSOC_IND		8	/* 802.11 ASSOC indication */
#define WLC_E_REASSOC		9	/* 802.11 REASSOC request */
#define WLC_E_REASSOC_IND	10	/* 802.11 REASSOC indication */
#define WLC_E_DISASSOC		11	/* 802.11 DISASSOC request */
#define WLC_E_DISASSOC_IND	12	/* 802.11 DISASSOC indication */
#define WLC_E_QUIET_START	13	/* 802.11h Quiet period started */
#define WLC_E_QUIET_END		14	/* 802.11h Quiet period ended */
#define WLC_E_BEACON_RX		15	/* BEACONS received/lost indication */
#define WLC_E_LINK		16	/* generic link indication */
#define WLC_E_MIC_ERROR		17	/* TKIP MIC error occurred */
#define WLC_E_NDIS_LINK		18	/* NDIS style link indication */
#define WLC_E_ROAM		19	/* roam complete: indicate status & reason */
#define WLC_E_TXFAIL		20	/* change in dot11FailedCount (txfail) */
#define WLC_E_PMKID_CACHE	21	/* WPA2 pmkid cache indication */
#define WLC_E_RETROGRADE_TSF	22	/* current AP's TSF value went backward */
#define WLC_E_PRUNE		23	/* AP was pruned from join list for reason */
#define WLC_E_AUTOAUTH		24	/* report AutoAuth table entry match for join attempt */
#define WLC_E_EAPOL_MSG		25	/* Event encapsulating an EAPOL message */
#define WLC_E_SCAN_COMPLETE	26	/* Scan results are ready or scan was aborted */
#define WLC_E_ADDTS_IND		27	/* indicate to host addts fail/success */
#define WLC_E_DELTS_IND		28	/* indicate to host delts fail/success */
#define WLC_E_BCNSENT_IND	29	/* indicate to host of beacon transmit */
#define WLC_E_BCNRX_MSG		30	/* Send the received beacon up to the host */
#define WLC_E_BCNLOST_MSG	31	/* indicate to host loss of beacon */
#define WLC_E_ROAM_PREP		32	/* before attempting to roam association */
#define WLC_E_PFN_NET_FOUND	33	/* PFN network found event */
#define WLC_E_PFN_NET_LOST	34	/* PFN network lost event */
#define WLC_E_RESET_COMPLETE	35
#define WLC_E_JOIN_START	36
#define WLC_E_ROAM_START	37	/* roam attempt started: indicate reason */
#define WLC_E_ASSOC_START	38
#define WLC_E_IBSS_ASSOC	39
#define WLC_E_RADIO		40
#define WLC_E_PSM_WATCHDOG	41	/* PSM microcode watchdog fired */

#define WLC_E_PROBREQ_MSG       44      /* probe request received */
#define WLC_E_SCAN_CONFIRM_IND  45
#define WLC_E_PSK_SUP		46	/* WPA Handshake fail */
#define WLC_E_COUNTRY_CODE_CHANGED	47
#define	WLC_E_EXCEEDED_MEDIUM_TIME	48	/* WMMAC excedded medium time */
#define WLC_E_ICV_ERROR		49	/* WEP ICV error occurred */
#define WLC_E_UNICAST_DECODE_ERROR	50	/* Unsupported unicast encrypted frame */
#define WLC_E_MULTICAST_DECODE_ERROR	51	/* Unsupported multicast encrypted frame */
#define WLC_E_TRACE		52
#define WLC_E_IF		54	/* I/F change (for dongle host notification) */
#define WLC_E_P2P_DISC_LISTEN_COMPLETE	55	/* listen state expires */
#define WLC_E_RSSI		56	/* indicate RSSI change based on configured levels */
#define WLC_E_PFN_BEST_BATCHING	57	/* PFN best network batching event */
#define WLC_E_EXTLOG_MSG	58
#define WLC_E_ACTION_FRAME	59	/* Action frame Rx */
#define WLC_E_ACTION_FRAME_COMPLETE	60	/* Action frame Tx complete */
#define WLC_E_PRE_ASSOC_IND	61	/* assoc request received */
#define WLC_E_PRE_REASSOC_IND	62	/* re-assoc request received */
#define WLC_E_CHANNEL_ADOPTED	63
#define WLC_E_AP_STARTED	64	/* AP started */
#define WLC_E_DFS_AP_STOP	65	/* AP stopped due to DFS */
#define WLC_E_DFS_AP_RESUME	66	/* AP resumed due to DFS */
#define WLC_E_WAI_STA_EVENT	67	/* WAI stations event */
#define WLC_E_WAI_MSG		68	/* event encapsulating an WAI message */
#define WLC_E_ESCAN_RESULT	69	/* escan result event */
#define WLC_E_ACTION_FRAME_OFF_CHAN_COMPLETE	70	/* action frame off channel complete */
#define WLC_E_PROBRESP_MSG	71	/* probe response received */
#define WLC_E_P2P_PROBREQ_MSG	72	/* P2P Probe request received */
#define WLC_E_SPARE73		73	/* unused, was WLC_E_DCS_REQUEST */
#define WLC_E_FIFO_CREDIT_MAP	74	/* credits for D11 FIFOs. [AC0,AC1,AC2,AC3,BC_MC,ATIM] */
#define WLC_E_ACTION_FRAME_RX	75	/* Received action frame event WITH
					 * wl_event_rx_frame_data_t header
					 */
#define WLC_E_WAKE_EVENT	76	/* Wake Event timer fired, used for wake WLAN test mode */
#define WLC_E_RM_COMPLETE	77	/* Radio measurement complete */
#define WLC_E_HTSFSYNC		78	/* Synchronize TSF with the host */
#define WLC_E_OVERLAY_REQ	79	/* request an overlay IOCTL/iovar from the host */
#define WLC_E_CSA_COMPLETE_IND		80	/* 802.11 CHANNEL SWITCH ACTION completed */
#define WLC_E_EXCESS_PM_WAKE_EVENT	81	/* excess PM Wake Event to inform host  */
#define WLC_E_PFN_SCAN_NONE		82	/* no PFN networks around */
/* PFN BSSID network found event, conflict/share with  WLC_E_PFN_SCAN_NONE */
#define WLC_E_PFN_BSSID_NET_FOUND	82
#define WLC_E_PFN_SCAN_ALLGONE		83	/* last found PFN network gets lost */
/* PFN BSSID network lost event, conflict/share with WLC_E_PFN_SCAN_ALLGONE */
#define WLC_E_PFN_BSSID_NET_LOST	83
#define WLC_E_GTK_PLUMBED		84
#define WLC_E_ASSOC_IND_NDIS		85	/* 802.11 ASSOC indication for NDIS only */
#define WLC_E_REASSOC_IND_NDIS		86	/* 802.11 REASSOC indication for NDIS only */
#define WLC_E_ASSOC_REQ_IE		87
#define WLC_E_ASSOC_RESP_IE		88
#define WLC_E_ASSOC_RECREATED		89	/* association recreated on resume */
#define WLC_E_ACTION_FRAME_RX_NDIS	90	/* rx action frame event for NDIS only */
#define WLC_E_AUTH_REQ			91	/* authentication request received */
#define WLC_E_TDLS_PEER_EVENT		92	/* discovered peer, connected/disconnected peer */
#define WLC_E_SPEEDY_RECREATE_FAIL	93	/* fast assoc recreation failed */
#define WLC_E_NATIVE			94	/* port-specific event and payload (e.g. NDIS) */
#define WLC_E_PKTDELAY_IND		95	/* event for tx pkt delay suddently jump */
#define WLC_E_PSTA_PRIMARY_INTF_IND	99	/* psta primary interface indication */
#define WLC_E_SPARE_100			100	/* available */
#define WLC_E_BEACON_FRAME_RX		101
#define WLC_E_SERVICE_FOUND		102	/* desired service found */
#define WLC_E_GAS_FRAGMENT_RX		103	/* GAS fragment received */
#define WLC_E_GAS_COMPLETE		104	/* GAS sessions all complete */
#define WLC_E_P2PO_ADD_DEVICE		105	/* New device found by p2p offload */
#define WLC_E_P2PO_DEL_DEVICE		106	/* device has been removed by p2p offload */
#define WLC_E_WNM_STA_SLEEP		107	/* WNM event to notify STA enter sleep mode */
#define WLC_E_TXFAIL_THRESH		108	/* Indication of MAC tx failures (exhaustion of
						 * 802.11 retries) exceeding threshold(s)
						 */
#define WLC_E_PROXD			109	/* Proximity Detection event */
#define WLC_E_IBSS_COALESCE		110	/* IBSS Coalescing */
#define WLC_E_BSS_LOAD			114	/* Inform host of beacon bss load */
#define WLC_E_MIMO_PWR_SAVE		115	/* Inform host MIMO PWR SAVE learning events */
#define WLC_E_LEAKY_AP_STATS		116	/* Inform host leaky Ap stats events */
#define WLC_E_ALLOW_CREDIT_BORROW	117	/* Allow or disallow wlfc credit borrowing in DHD */
#define WLC_E_MSCH			120	/* Multiple channel scheduler event */
#define WLC_E_CSA_START_IND		121
#define WLC_E_CSA_DONE_IND		122
#define WLC_E_CSA_FAILURE_IND		123
#define WLC_E_CCA_CHAN_QUAL		124	/* CCA based channel quality report */
#define WLC_E_BSSID			125	/* to report change in BSSID while roaming */
#define WLC_E_TX_STAT_ERROR		126	/* tx error indication */
#define WLC_E_BCMC_CREDIT_SUPPORT	127	/* credit check for BCMC supported */
#define WLC_E_PEER_TIMEOUT		128	/* silently drop a STA because of inactivity */
#define WLC_E_BT_WIFI_HANDOVER_REQ	130	/* Handover Request Initiated */
#define WLC_E_SPW_TXINHIBIT		131	/* Southpaw TxInhibit notification */
#define WLC_E_FBT_AUTH_REQ_IND		132	/* FBT Authentication Request Indication */
#define WLC_E_RSSI_LQM			133	/* Enhancement addition for WLC_E_RSSI */
#define WLC_E_PFN_GSCAN_FULL_RESULT	134	/* Full probe/beacon (IEs etc) results */
#define WLC_E_PFN_SWC			135 /* Significant change in rssi of bssids being tracked */
#define WLC_E_AUTHORIZED		136	/* a STA been authroized for traffic */
#define WLC_E_PROBREQ_MSG_RX		137	/* probe req with wl_event_rx_frame_data_t header */
#define WLC_E_PFN_SCAN_COMPLETE		138	/* PFN completed scan of network list */
#define WLC_E_RMC_EVENT			139	/* RMC Event */
#define WLC_E_DPSTA_INTF_IND		140	/* DPSTA interface indication */
#define WLC_E_RRM			141	/* RRM Event */
#define WLC_E_PFN_SSID_EXT		142	/* SSID EXT event */
#define WLC_E_ROAM_EXP_EVENT		143	/* Expanded roam event */
#define WLC_E_MACDBG			147	/* Ucode debugging event */
#define WLC_E_RESERVED			148	/* reserved */
#define WLC_E_PRE_ASSOC_RSEP_IND	149	/* assoc resp received */
#define WLC_E_PSK_AUTH			150	/* PSK AUTH WPA2-PSK 4 WAY Handshake failure */
#define WLC_E_TKO			151	/* TCP keepalive offload */
#define WLC_E_SDB_TRANSITION		152	/* SDB mode-switch event */
#define WLC_E_NATOE_NFCT		153	/* natoe event */
#define WLC_E_TEMP_THROTTLE		154	/* Temperature throttling control event */
#define WLC_E_LINK_QUALITY		155	/* Link quality measurement complete */
#define WLC_E_BSSTRANS_RESP		156	/* BSS Transition Response received */
#define WLC_E_TWT_SETUP			157	/* TWT Setup Complete event */
#define WLC_E_HE_TWT_SETUP		157	/* TODO:Remove after merging TWT changes to trunk */
#define WLC_E_SPARE_158			158	/* available */
#define WLC_E_SPARE_159			159	/* available */
#define WLC_E_RADAR_DETECTED		160	/* Radar Detected event */
#define WLC_E_RANGING_EVENT		161	/* Ranging event */
#define WLC_E_INVALID_IE		162	/* Received invalid IE */
#define WLC_E_MODE_SWITCH		163	/* Mode switch event */
#define WLC_E_PKT_FILTER		164	/* Packet filter event */
#define WLC_E_DMA_TXFLUSH_COMPLETE	165	/* TxFlush done before changing tx/rxchain */
#define WLC_E_FBT			166	/* FBT event */
#define WLC_E_PFN_SCAN_BACKOFF		167	/* PFN SCAN Backoff event */
#define WLC_E_PFN_BSSID_SCAN_BACKOFF	168	/* PFN BSSID SCAN BAckoff event */
#define WLC_E_AGGR_EVENT		169	/* Aggregated event */
#define WLC_E_AP_CHAN_CHANGE		170	/* AP channe change event propagate to use */
#define WLC_E_PSTA_CREATE_IND		171	/* Indication for PSTA creation */
/* Customer extended EVENTs */
#define WLC_E_DFS_HIT			182	/* Found radar on channel */
#define WLC_E_FRAME_FIRST_RX		173	/* RX pkt from STA */
#define WLC_E_BCN_STUCK			174	/* Beacon Stuck */
#define WLC_E_PROBSUP_IND		175	/* WL_EAP_BANDSTEER:
						 * Probe Suppression Indication for a STA
						 */
#define WLC_E_SAS_RSSI			176	/* WL_EAP_SAS. An AI's RSSI has notable change */
#define WLC_E_RATE_CHANGE		177	/* WL_EAP_AP. A client's rate has changed. */
#define WLC_E_AMT_CHANGE		178	/* WL_EAP_AP. An AMT entry has changed. */
#define WLC_E_LTE_U_EVENT		179	/* LTE_U driver event */

#define WLC_E_CEVENT			180	/* connectivity event logging */
#define WLC_E_HWA_EVENT			181	/* HWA event */
#define WLC_E_AIRIQ_EVENT		172	/* AIRIQ driver event */
#define WLC_E_TXFAIL_TRFTHOLD		183	/* Indication of MAC tx failures */

#define WLC_E_CAC_STATE_CHANGE		184	/* Indication of CAC Status change */
#define WLC_E_REQ_BW_CHANGE		185	/* Event to request for BW change */
#define WLC_E_ASSOC_REASSOC_IND_EXT	186	/* 802.11 Extended (Re)ASSOC indication with whole
						 * frame even including MAC header
						 */
#define WLC_E_WNM_ERR			187	/* BSSTRANS error response */

#define WLC_E_ASSOC_FAIL		188	/* Assoc fail */
#define WLC_E_REASSOC_FAIL		189	/* Reassoc Fail */
#define WLC_E_AUTH_FAIL			190	/* Auth Fail */
#define WLC_E_BSSTRANS_REQ		191	/* BSS Transition Request received */
#define WLC_E_BSSTRANS_QUERY		192	/* BSS Transition Query received */
#define WLC_E_START_AUTH		193	/* Event to initiate External Auth */
#define WLC_E_OMN_MASTER		194	/* OMN Master change event */
#define WLC_E_MBO_CAPABILITY_STATUS	195	/* per bss mbo capability, association request
						 * handling capability etc
						 */
#define WLC_E_WNM_NOTIFICATION_REQ	196	/* WNM notification request payload from STA */
#define WLC_E_WNM_BSSTRANS_QUERY	197	/* BTM query payload from STA */
#define WLC_E_GAS_RQST_ANQP_QUERY	198	/* GAS ANQP query from unassociated STA */

#define WLC_E_PWR_SAVE_SYNC		199	/* Sync pm value with cfg80211 power save */
#define WLC_E_PROBREQ_MSG_RX_EXT	200	/* 802.11 Extended Probe Request indication with
						 * whole frame even including MAC header
						 */
#define WLC_E_BAND_CHANGE		201	/* Band change event */
#define WLC_E_COLOR			202	/* Color (collision) information event. */
#define WLC_E_CAPTURE_RXFRAME		203	/* Capture Rx frame */
#define WLC_E_CAPTURE_TXFRAME		204	/* Capture Tx frame */
#define WLC_E_BUZZZ			205	/* BUZZZ event */
#define WLC_E_EDCRS_HI_EVENT		206	/* EDCRS_HI detected event */
#define WLC_E_QOS_MGMT			207	/* Qos Management event */
#define WLC_E_HEALTH_CHECK		208
#define WLC_E_CSA_RECV_IND		209	/* CSA Received Event */
#define WLC_E_MLD_UP			210	/* MLD UP */
#define WLC_E_PASN_AUTH			211	/* PASN AUTH frames to cfg80211 */
#define WLC_E_LAST			212

/* define an API for getting the string name of an event */
extern const char *bcmevent_get_name(uint event_type);

/* validate if the event is proper and if valid copy event header to event */
extern int is_wlc_event_frame(void *pktdata, uint pktlen, uint16 exp_usr_subtype,
	bcm_event_msg_u_t *out_event);

/* conversion between host and network order for events */
extern void wl_event_to_host_order(wl_event_msg_t * evt);
extern void wl_event_to_network_order(wl_event_msg_t * evt);

/* xxx:
 * Please do not insert/delete events in the middle causing renumbering.
 * It is a problem for host-device compatibility, especially with ROMmed chips.
 */

/* Event status codes */
#define WLC_E_STATUS_SUCCESS		0	/* operation was successful */
#define WLC_E_STATUS_FAIL		1	/* operation failed */
#define WLC_E_STATUS_TIMEOUT		2	/* operation timed out */
#define WLC_E_STATUS_NO_NETWORKS	3	/* failed due to no matching network found */
#define WLC_E_STATUS_ABORT		4	/* operation was aborted */
#define WLC_E_STATUS_NO_ACK		5	/* protocol failure: packet not ack'd */
#define WLC_E_STATUS_UNSOLICITED	6	/* AUTH or ASSOC packet was unsolicited */
#define WLC_E_STATUS_ATTEMPT		7	/* attempt to assoc to an auto auth configuration */
#define WLC_E_STATUS_PARTIAL		8	/* scan results are incomplete */
#define WLC_E_STATUS_NEWSCAN		9	/* scan aborted by another scan */
#define WLC_E_STATUS_NEWASSOC		10	/* scan aborted due to assoc in progress */
#define WLC_E_STATUS_11HQUIET		11	/* 802.11h quiet period started */
#define WLC_E_STATUS_SUPPRESS		12	/* user disabled scanning (WLC_SET_SCANSUPPRESS) */
#define WLC_E_STATUS_NOCHANS		13	/* no allowable channels to scan */
#define WLC_E_STATUS_CS_ABORT		15	/* abort channel select */
#define WLC_E_STATUS_ERROR		16	/* request failed due to error */
#define WLC_E_STATUS_TRY_LATER		30	/* Try after some time */
#define WLC_E_STATUS_INVALID 0xff  /* Invalid status code to init variables. */

/* 4-way handshake event type */
#define WLC_E_PSK_AUTH_SUB_EAPOL_START		1	/* EAPOL start */
#define WLC_E_PSK_AUTH_SUB_EAPOL_DONE		2	/* EAPOL end */
/* GTK event type */
#define	WLC_E_PSK_AUTH_SUB_GTK_DONE		3	/* GTK end */

/* 4-way handshake event status code */
#define WLC_E_STATUS_PSK_AUTH_WPA_TIMOUT	1	/* operation timed out */
#define WLC_E_STATUS_PSK_AUTH_MIC_WPA_ERR		2	/* MIC error */
#define WLC_E_STATUS_PSK_AUTH_IE_MISMATCH_ERR		3	/* IE Missmatch error */
#define WLC_E_STATUS_PSK_AUTH_REPLAY_COUNT_ERR		4
#define WLC_E_STATUS_PSK_AUTH_PEER_BLACKISTED	5	/* Blaclisted peer */
#define WLC_E_STATUS_PSK_AUTH_GTK_REKEY_FAIL	6	/* GTK event status code */

/* SDB transition status code */
#define WLC_E_STATUS_SDB_START			1
#define WLC_E_STATUS_SDB_COMPLETE		2
/* Slice-swap status code */
#define WLC_E_STATUS_SLICE_SWAP_START		3
#define WLC_E_STATUS_SLICE_SWAP_COMPLETE	4

/* SDB transition reason code */
#define WLC_E_REASON_HOST_DIRECT	0
#define WLC_E_REASON_INFRA_ASSOC	1
#define WLC_E_REASON_INFRA_ROAM		2
#define WLC_E_REASON_INFRA_DISASSOC	3
#define WLC_E_REASON_NO_MODE_CHANGE_NEEDED	4
#define WLC_E_REASON_UNSUPPORTED1	5
#define WLC_E_REASON_UNSUPPORTED2	6

/* WLC_E_SDB_TRANSITION event data */
#define WL_MAX_BSSCFG     4
#define WL_EVENT_SDB_TRANSITION_VER     1
typedef struct wl_event_sdb_data {
	uint8 wlunit;           /* Core index */
	uint8 is_iftype;        /* Interface Type(Station, SoftAP, P2P_GO, P2P_GC */
	uint16 chanspec;        /* Interface Channel/Chanspec */
	char ssidbuf[(4 * 32) + 1];	/* SSID_FMT_BUF_LEN: ((4 * DOT11_MAX_SSID_LEN) + 1) */
} wl_event_sdb_data_t;

typedef struct wl_event_sdb_trans {
	uint8 version;          /* Event Data Version */
	uint8 rsdb_mode;
	uint8 enable_bsscfg;
	uint8 reserved;
	struct wl_event_sdb_data values[WL_MAX_BSSCFG];
} wl_event_sdb_trans_t;

/* roam reason codes */
#define WLC_E_REASON_INITIAL_ASSOC	0	/* initial assoc */
#define WLC_E_REASON_LOW_RSSI		1	/* roamed due to low RSSI */
#define WLC_E_REASON_DEAUTH		2	/* roamed due to DEAUTH indication */
#define WLC_E_REASON_DISASSOC		3	/* roamed due to DISASSOC indication */
#define WLC_E_REASON_BCNS_LOST		4	/* roamed due to lost beacons */

#define WLC_E_REASON_FAST_ROAM_FAILED	5	/* roamed due to fast roam failure */
#define WLC_E_REASON_DIRECTED_ROAM	6	/* roamed due to request by AP */
#define WLC_E_REASON_TSPEC_REJECTED	7	/* roamed due to TSPEC rejection */
#define WLC_E_REASON_BETTER_AP		8	/* roamed due to finding better AP */
#define WLC_E_REASON_MINTXRATE		9	/* roamed because at mintxrate for too long */
#define WLC_E_REASON_TXFAIL		10	/* We can hear AP, but AP can't hear us */
/* retained for precommit auto-merging errors; remove once all branches are synced */
#define WLC_E_REASON_REQUESTED_ROAM	11
#define WLC_E_REASON_BSSTRANS_REQ	11	/* roamed due to BSS Transition request by AP */
#define WLC_E_REASON_LOW_RSSI_CU		12 /* roamed due to low RSSI and Channel Usage */
#define WLC_E_REASON_RADAR_DETECTED	13	/* roamed due to radar detection by STA */
#define WLC_E_REASON_LOW_RSSI_END	14	/* roam stopped due to high RSSI */

/* prune reason codes */
#define WLC_E_PRUNE_ENCR_MISMATCH	1	/* encryption mismatch */
#define WLC_E_PRUNE_BCAST_BSSID		2	/* AP uses a broadcast BSSID */
#define WLC_E_PRUNE_MAC_DENY		3	/* STA's MAC addr is in AP's MAC deny list */
#define WLC_E_PRUNE_MAC_NA		4	/* STA's MAC addr is not in AP's MAC allow list */
#define WLC_E_PRUNE_REG_PASSV		5	/* AP not allowed due to regulatory restriction */
#define WLC_E_PRUNE_SPCT_MGMT		6	/* AP does not support STA locale spectrum mgmt */
#define WLC_E_PRUNE_RADAR		7	/* AP is on a radar channel of STA locale */
#define WLC_E_RSN_MISMATCH		8	/* STA does not support AP's RSN */
#define WLC_E_PRUNE_NO_COMMON_RATES	9	/* No rates in common with AP */
#define WLC_E_PRUNE_BASIC_RATES		10	/* STA does not support all basic rates of BSS */
#define WLC_E_PRUNE_CIPHER_NA		12	/* BSS's cipher not supported */
#define WLC_E_PRUNE_KNOWN_STA		13	/* AP is already known to us as a STA */
#define WLC_E_PRUNE_WDS_PEER		15	/* AP is already known to us as a WDS peer */
#define WLC_E_PRUNE_QBSS_LOAD		16	/* QBSS LOAD - AAC is too low */
#define WLC_E_PRUNE_HOME_AP		17	/* prune home AP */
#define WLC_E_PRUNE_AUTH_RESP_MAC	20	/* suppress auth resp by MAC filter */

/* WPA failure reason codes carried in the WLC_E_PSK_SUP event */
#define WLC_E_SUP_OTHER			0	/* Other reason */
#define WLC_E_SUP_DECRYPT_KEY_DATA	1	/* Decryption of key data failed */
#define WLC_E_SUP_BAD_UCAST_WEP128	2	/* Illegal use of ucast WEP128 */
#define WLC_E_SUP_BAD_UCAST_WEP40	3	/* Illegal use of ucast WEP40 */
#define WLC_E_SUP_UNSUP_KEY_LEN		4	/* Unsupported key length */
#define WLC_E_SUP_PW_KEY_CIPHER		5	/* Unicast cipher mismatch in pairwise key */
#define WLC_E_SUP_MSG3_TOO_MANY_IE	6	/* WPA IE contains > 1 RSN IE in key msg 3 */
#define WLC_E_SUP_MSG3_IE_MISMATCH	7	/* WPA IE mismatch in key message 3 */
#define WLC_E_SUP_NO_INSTALL_FLAG	8	/* INSTALL flag unset in 4-way msg */
#define WLC_E_SUP_MSG3_NO_GTK		9	/* encapsulated GTK missing from msg 3 */
#define WLC_E_SUP_GRP_KEY_CIPHER	10	/* Multicast cipher mismatch in group key */
#define WLC_E_SUP_GRP_MSG1_NO_GTK	11	/* encapsulated GTK missing from group msg 1 */
#define WLC_E_SUP_GTK_DECRYPT_FAIL	12	/* GTK decrypt failure */
#define WLC_E_SUP_SEND_FAIL		13	/* message send failure */
#define WLC_E_SUP_DEAUTH		14	/* received FC_DEAUTH */
#define WLC_E_SUP_WPA_PSK_TMO		15	/* WPA PSK 4-way handshake timeout */
#define WLC_E_SUP_WPA_PSK_M1_TMO	16	/* WPA PSK 4-way handshake M1 timeout */
#define WLC_E_SUP_WPA_PSK_M3_TMO	17	/* WPA PSK 4-way handshake M3 timeout */

/* macdbg reason codes carried in the WLC_E_MACDBG event */
#define WLC_E_MACDBG_LIST_PSM		0	/* Dump list update for PSM registers */
#define WLC_E_MACDBG_LIST_PSMX		1	/* Dump list update for PSMx registers */
#define WLC_E_MACDBG_REGALL		2	/* Dump all registers, old define will be removed */
#define WLC_E_MACDBG_DUMPALL		2	/* Dump all registers */
#define WLC_E_MACDBG_LISTALL		3	/* Update offsets of all registers to dump */
#define WLC_E_MACDBG_DTRACE		4	/* Send dtrace log bytes */
#define WLC_E_MACDBG_RATELINKMEM	5	/* Update occurred in ratelinkmem usage */

/* Event data for events that include frames received over the air */
/* WLC_E_PROBRESP_MSG
 * WLC_E_P2P_PROBREQ_MSG
 * WLC_E_ACTION_FRAME_RX
 */

/* HWA event reason codes */
#ifdef BCMHWA
#define WLC_E_HWA_RX_STOP		0	/* Stop RxPost to dongle and reclaim all WI */
#define WLC_E_HWA_RX_POST		1	/* Redo RxPost to dongle */
#define WLC_E_HWA_RX_REINIT		2	/* Redo RxPost and enable Rx blocks */
#define WLC_E_HWA_RX_STOP_REFILL	3	/* Stop RxPost to dongle */
#define WLC_E_HWA_DHD_DUMP		4	/* Request DHD dump */
#endif // endif

typedef BWL_PRE_PACKED_STRUCT struct wl_event_rx_frame_data {
	uint16	version;
	chanspec_t chspec20; /** 20Mhz wide chanspec */
	int32	rssi;
	uint32	mactime;
	uint32	rate;
} BWL_POST_PACKED_STRUCT wl_event_rx_frame_data_t;

#define BCM_RX_FRAME_DATA_VERSION 1

/* WLC_E_IF event data */
typedef struct wl_event_data_if {
	uint8 ifidx;		/* RTE virtual device index (for dongle) */
	uint8 opcode;		/* see I/F opcode */
	uint8 reserved;		/* bit mask (WLC_E_IF_FLAGS_XXX ) */
	uint8 bssidx;		/* bsscfg index */
	uint8 role;		/* see I/F role */
	uint8 mld_unit;
	uint8 nlinks;
	struct ether_addr peer_addr;	/* peer (WDS/DWDS Client) MAC address (if applicable) */
} wl_event_data_if_t;

/* WLC_E_NATOE event data */
typedef struct wl_event_data_natoe {
	uint32 natoe_active;
	uint32 sta_ip;
	uint16 start_port;
	uint16 end_port;
} wl_event_data_natoe_t;

/* opcode in WLC_E_IF event */
#define WLC_E_IF_ADD		1	/* bsscfg add */
#define WLC_E_IF_DEL		2	/* bsscfg delete */
#define WLC_E_IF_CHANGE		3	/* bsscfg role change */
#define WLC_E_IF_BSSCFG_UP	4	/* bsscfg up */
#define WLC_E_IF_BSSCFG_DOWN	5	/* bsscfg down */

/* I/F role code in WLC_E_IF event */
#define WLC_E_IF_ROLE_STA		0	/* Infra STA */
#define WLC_E_IF_ROLE_AP		1	/* Access Point */
#define WLC_E_IF_ROLE_WDS		2	/* WDS link */
#define WLC_E_IF_ROLE_P2P_GO		3	/* P2P Group Owner */
#define WLC_E_IF_ROLE_P2P_CLIENT	4	/* P2P Client */
#define WLC_E_IF_ROLE_IBSS              8       /* IBSS */

/* WLC_E_RSSI event data */
typedef struct wl_event_data_rssi {
	int32 rssi;
	int32 snr;
	int32 noise;
} wl_event_data_rssi_t;

/* WLC_E_IF flag */
#define WLC_E_IF_FLAGS_BSSCFG_NOIF	0x1	/* no host I/F creation needed */
#define WLC_E_IF_FLAGS_WDS_STA		0x2	/* WDS supplicant interface */
#define WLC_E_IF_FLAGS_WDS_AP		0x4	/* WDS Authenticator or DWDS client interface */

#define WLC_E_IF_FLAGS_LEGACY_WDS_STA	WLC_E_IF_FLAGS_WDS_STA
#define WLC_E_IF_FLAGS_LEGACY_WDS_AP	WLC_E_IF_FLAGS_WDS_AP

/* Reason codes for LINK */
#define WLC_E_LINK_BCN_LOSS     1   /* Link down because of beacon loss */
#define WLC_E_LINK_DISASSOC     2   /* Link down because of disassoc */
#define WLC_E_LINK_ASSOC_REC    3   /* Link down because assoc recreate failed */
#define WLC_E_LINK_BSSCFG_DIS   4   /* Link down due to bsscfg down */

/* WLC_E_NDIS_LINK event data */
typedef BWL_PRE_PACKED_STRUCT struct ndis_link_parms {
	struct ether_addr peer_mac; /* 6 bytes */
	uint16 chanspec;            /* 2 bytes */
	uint32 link_speed;          /* current datarate in units of 500 Kbit/s */
	uint32 max_link_speed;      /* max possible datarate for link in units of 500 Kbit/s  */
	int32  rssi;                /* average rssi */
} BWL_POST_PACKED_STRUCT ndis_link_parms_t;

/* reason codes for WLC_E_OVERLAY_REQ event */
#define WLC_E_OVL_DOWNLOAD		0	/* overlay download request */
#define WLC_E_OVL_UPDATE_IND	1	/* device indication of host overlay update */

/* reason codes for WLC_E_TDLS_PEER_EVENT event */
#define WLC_E_TDLS_PEER_DISCOVERED		0	/* peer is ready to establish TDLS */
#define WLC_E_TDLS_PEER_CONNECTED		1
#define WLC_E_TDLS_PEER_DISCONNECTED	2

/* reason codes for WLC_E_RMC_EVENT event */
#define WLC_E_REASON_RMC_NONE		0
#define WLC_E_REASON_RMC_AR_LOST		1
#define WLC_E_REASON_RMC_AR_NO_ACK		2

#ifdef WLTDLS
/* TDLS Action Category code */
#define TDLS_AF_CATEGORY		12
/* Wi-Fi Display (WFD) Vendor Specific Category */
/* used for WFD Tunneled Probe Request and Response */
#define TDLS_VENDOR_SPECIFIC		127
/* TDLS Action Field Values */
#define TDLS_ACTION_SETUP_REQ		0
#define TDLS_ACTION_SETUP_RESP		1
#define TDLS_ACTION_SETUP_CONFIRM	2
#define TDLS_ACTION_TEARDOWN		3
#define WLAN_TDLS_SET_PROBE_WFD_IE	11
#define WLAN_TDLS_SET_SETUP_WFD_IE	12
#define WLAN_TDLS_SET_WFD_ENABLED	13
#define WLAN_TDLS_SET_WFD_DISABLED	14
#endif // endif

/* WLC_E_RANGING_EVENT subtypes */
#define WLC_E_RANGING_RESULTS	0

/* GAS event data */
typedef BWL_PRE_PACKED_STRUCT struct wl_event_gas {
	uint16	channel;		/* channel of GAS protocol */
	uint8	dialog_token;	/* GAS dialog token */
	uint8	fragment_id;	/* fragment id */
	uint16	status_code;	/* status code on GAS completion */
	uint16 	data_len;		/* length of data to follow */
	uint8	data[1];		/* variable length specified by data_len */
} BWL_POST_PACKED_STRUCT wl_event_gas_t;

/* service discovery TLV */
typedef BWL_PRE_PACKED_STRUCT struct wl_sd_tlv {
	uint16	length;			/* length of response_data */
	uint8	protocol;		/* service protocol type */
	uint8	transaction_id;		/* service transaction id */
	uint8	status_code;		/* status code */
	uint8	data[1];		/* response data */
} BWL_POST_PACKED_STRUCT wl_sd_tlv_t;

/* service discovery event data */
typedef BWL_PRE_PACKED_STRUCT struct wl_event_sd {
	uint16	channel;		/* channel */
	uint8	count;			/* number of tlvs */
	wl_sd_tlv_t	tlv[1];		/* service discovery TLV */
} BWL_POST_PACKED_STRUCT wl_event_sd_t;

/* WLC_E_PKT_FILTER event sub-classification codes */
#define WLC_E_PKT_FILTER_TIMEOUT	1 /* Matching packet not received in last timeout seconds */

/* Note: proxd has a new API (ver 3.0) deprecates the following */

/* Reason codes for WLC_E_PROXD */
#define WLC_E_PROXD_FOUND		1	/* Found a proximity device */
#define WLC_E_PROXD_GONE		2	/* Lost a proximity device */
#define WLC_E_PROXD_START		3	/* used by: target  */
#define WLC_E_PROXD_STOP		4	/* used by: target   */
#define WLC_E_PROXD_COMPLETED		5	/* used by: initiator completed */
#define WLC_E_PROXD_ERROR		6	/* used by both initiator and target */
#define WLC_E_PROXD_COLLECT_START	7	/* used by: target & initiator */
#define WLC_E_PROXD_COLLECT_STOP	8	/* used by: target */
#define WLC_E_PROXD_COLLECT_COMPLETED	9	/* used by: initiator completed */
#define WLC_E_PROXD_COLLECT_ERROR	10	/* used by both initiator and target */
#define WLC_E_PROXD_UNUSED_11		11	/* available */
#define WLC_E_PROXD_TS_RESULTS		12	/* used by: initiator completed */

/*  proxd_event data */
typedef struct ftm_sample {
	uint32 value;	/* RTT in ns */
	int8 rssi;	/* RSSI */
} ftm_sample_t;

typedef struct ts_sample {
	uint32 t1;
	uint32 t2;
	uint32 t3;
	uint32 t4;
} ts_sample_t;

typedef BWL_PRE_PACKED_STRUCT struct proxd_event_data {
	uint16 ver;			/* version */
	uint16 mode;			/* mode: target/initiator */
	uint16 method;			/* method: rssi/TOF/AOA */
	uint8  err_code;		/* error classification */
	uint8  TOF_type;		/* one way or two way TOF */
	uint8  OFDM_frame_type;		/* legacy or VHT */
	uint8  bandwidth;		/* Bandwidth is 20, 40,80, MHZ */
	struct ether_addr peer_mac;	/* (e.g for tgt:initiator's */
	uint32 distance;		/* dst to tgt, units meter */
	uint32 meanrtt;			/* mean delta */
	uint32 modertt;			/* Mode delta */
	uint32 medianrtt;		/* median RTT */
	uint32 sdrtt;			/* Standard deviation of RTT */
	int32  gdcalcresult;		/* Software or Hardware Kind of redundant, but if */
					/* frame type is VHT, then we should do it by hardware */
	int16  avg_rssi;		/* avg rssi accroos the ftm frames */
	int16  validfrmcnt;		/* Firmware's valid frame counts */
	int32  peer_router_info;	/* Peer router information if available in TLV, */
					/* We will add this field later  */
	int32 var1;			/* average of group delay */
	int32 var2;			/* average of threshold crossing */
	int32 var3;			/* difference between group delay and threshold crossing */
					/* raw Fine Time Measurements (ftm) data */
	uint16 ftm_unit;		/* ftm cnt resolution in picoseconds , 6250ps - default */
	uint16 ftm_cnt;			/*  num of rtd measurments/length in the ftm buffer  */
	ftm_sample_t ftm_buff[1];	/* 1 ... ftm_cnt  */
} BWL_POST_PACKED_STRUCT wl_proxd_event_data_t;

typedef BWL_PRE_PACKED_STRUCT struct proxd_event_ts_results {
	uint16 ver;                     /* version */
	uint16 mode;                    /* mode: target/initiator */
	uint16 method;                  /* method: rssi/TOF/AOA */
	uint8  err_code;                /* error classification */
	uint8  TOF_type;                /* one way or two way TOF */
	uint16  ts_cnt;                 /* number of timestamp measurements */
	ts_sample_t ts_buff[1];         /* Timestamps */
} BWL_POST_PACKED_STRUCT wl_proxd_event_ts_results_t;

/* Video Traffic Interference Monitor Event */
#define INTFER_EVENT_VERSION		1
#define INTFER_STREAM_TYPE_NONTCP	1
#define INTFER_STREAM_TYPE_TCP		2
#define WLINTFER_STATS_NSMPLS		4
typedef struct wl_intfer_event {
	uint16 version;			/* version */
	uint16 status;			/* status */
	uint8 txfail_histo[WLINTFER_STATS_NSMPLS]; /* txfail histo */
} wl_intfer_event_t;

#define RRM_EVENT_VERSION		0
typedef struct wl_rrm_event {
	int16 version;
	int16 len;
	int16 cat;		/* Category */
	int16 subevent;
	char payload[1]; /* Measurement payload */
} wl_rrm_event_t;

/* WLC_E_PSTA_PRIMARY_INTF_IND event data */
typedef struct wl_psta_primary_intf_event {
	struct ether_addr prim_ea;	/* primary intf ether addr */
} wl_psta_primary_intf_event_t;

/* WLC_E_DPSTA_INTF_IND event data */
typedef enum {
	WL_INTF_PSTA = 1,
	WL_INTF_DWDS = 2
} wl_dpsta_intf_type;

typedef struct wl_dpsta_intf_event {
	wl_dpsta_intf_type intf_type;    /* dwds/psta intf register */
} wl_dpsta_intf_event_t;

/* TCP keepalive event data */
typedef BWL_PRE_PACKED_STRUCT struct wl_event_tko {
	uint8 index;		/* TCP connection index, 0 to max-1 */
	uint8 pad[3];		/* 4-byte struct alignment */
} BWL_POST_PACKED_STRUCT wl_event_tko_t;

typedef struct {
	uint8 radar_type;       /* one of RADAR_TYPE_XXX */
	uint16 min_pw;          /* minimum pulse-width (usec * 20) */
	uint16 max_pw;          /* maximum pulse-width (usec * 20) */
	uint16 min_pri;         /* minimum pulse repetition interval (usec) */
	uint16 max_pri;         /* maximum pulse repetition interval (usec) */
	uint16 subband;         /* subband/frequency */
} radar_detected_event_info_t;
typedef struct wl_event_radar_detect_data {

	uint32 version;
	uint16 current_chanspec; /* chanspec on which the radar is recieved */
	uint16 target_chanspec; /*  Target chanspec after detection of radar on current_chanspec */
	radar_detected_event_info_t radar_info[2];
} wl_event_radar_detect_data_t;

typedef enum {
	WL_CHAN_REASON_CSA = 0,
	WL_CHAN_REASON_DFS_AP_MOVE_START = 1,
	WL_CHAN_REASON_DFS_AP_MOVE_RADAR_FOUND = 2,
	WL_CHAN_REASON_DFS_AP_MOVE_ABORTED = 3,
	WL_CHAN_REASON_DFS_AP_MOVE_SUCCESS = 4,
	WL_CHAN_REASON_DFS_AP_MOVE_STUNT = 5,
	WL_CHAN_REASON_DFS_AP_MOVE_STUNT_SUCCESS = 6,
	WL_CHAN_REASON_CSA_TO_DFS_CHAN_FOR_CAC_ONLY = 7, /* Support Co-ordinated CAC for MAP R2 */
	WL_CHAN_REASON_OBSS_AP_BW_SWITCH = 8, /* AP bandwidth switch due to OBSS coex */
	WL_CHAN_REASON_ANY = 9
} wl_chan_change_reason_t;

typedef struct wl_event_change_chan {
	uint16 version;
	uint16 length;			/* excluding pad field */
	wl_chan_change_reason_t reason;
	chanspec_t target_chanspec;
	chanspec_t cur_chanspec;
} wl_event_change_chan_t;

#define WL_CHAN_CHANGE_EVENT_VER_1	1 /* channel change event version */
#define WL_CHAN_CHANGE_EVENT_LEN_VER_1	10
/* VER_2: added OBSS reason and replaced pad with cur_chanspec */
#define WL_CHAN_CHANGE_EVENT_VER_2	2
#define WL_CHAN_CHANGE_EVENT_LEN_VER_2	12

#define WL_EVENT_MODESW_VER_1			1
#define WL_EVENT_MODESW_VER_CURRENT		WL_EVENT_MODESW_VER_1

#define WL_E_MODESW_FLAG_MASK_DEVICE		0x01u /* mask of device: belongs to local or peer */
#define WL_E_MODESW_FLAG_MASK_FROM		0x02u /* mask of origin: firmware or user */
#define WL_E_MODESW_FLAG_MASK_STATE		0x0Cu /* mask of state: modesw progress state */

#define WL_E_MODESW_FLAG_DEVICE_LOCAL		0x00u /* flag - device: info is about self/local */
#define WL_E_MODESW_FLAG_DEVICE_PEER		0x01u /* flag - device: info is about peer */

#define WL_E_MODESW_FLAG_FROM_FIRMWARE		0x00u /* flag - from: request is from firmware */
#define WL_E_MODESW_FLAG_FROM_USER		0x02u /* flag - from: request is from user/iov */

#define WL_E_MODESW_FLAG_STATE_REQUESTED	0x00u /* flag - state: mode switch request */
#define WL_E_MODESW_FLAG_STATE_INITIATED	0x04u /* flag - state: switch initiated */
#define WL_E_MODESW_FLAG_STATE_COMPLETE		0x08u /* flag - state: switch completed/success */
#define WL_E_MODESW_FLAG_STATE_FAILURE		0x0Cu /* flag - state: failed to switch */

/* Get sizeof *X including variable data's length where X is pointer to wl_event_mode_switch_t */
#define WL_E_MODESW_SIZE(X) (sizeof(*(X)) + (X)->length)

/* Get variable data's length where X is pointer to wl_event_mode_switch_t */
#define WL_E_MODESW_DATA_SIZE(X) (((X)->length > sizeof(*(X))) ? ((X)->length - sizeof(*(X))) : 0)

#define WL_E_MODESW_REASON_UNKNOWN		0u /* reason: UNKNOWN */
#define WL_E_MODESW_REASON_ACSD			1u /* reason: ACSD (based on events from FW */
#define WL_E_MODESW_REASON_OBSS_DBS		2u /* reason: OBSS DBS (eg. on interference) */
#define WL_E_MODESW_REASON_DFS			3u /* reason: DFS (eg. on subband radar) */
#define WL_E_MODESW_REASON_DYN160		4u /* reason: DYN160 (160/2x2 - 80/4x4) */

/* event structure for WLC_E_MODE_SWITCH */
typedef struct {
	uint16 version;
	uint16 length;	/* size including 'data' field */
	uint16 opmode_from;
	uint16 opmode_to;
	uint32 flags;	/* bit 0: peer(/local==0);
			 * bit 1: user(/firmware==0);
			 * bits 3,2: 00==requested, 01==initiated,
			 *           10==complete, 11==failure;
			 * rest: reserved
			 */
	uint16 reason;	/* value 0: unknown, 1: ACSD, 2: OBSS_DBS,
			 *       3: DFS, 4: DYN160, rest: reserved
			 */
	uint16 data_offset;	/* offset to 'data' from beginning of this struct.
				 * fields may be added between data_offset and data
				 */
	/* ADD NEW FIELDS HERE */
	uint8 data[];	/* reason specific data; could be empty */
} wl_event_mode_switch_t;

/* when reason in WLC_E_MODE_SWITCH is DYN160, data will carry the following structure */
typedef struct {
	uint16 trigger;		/* value 0: MU to SU, 1: SU to MU, 2: metric_dyn160, 3:re-/assoc,
				 *       4: disassoc, 5: rssi, 6: traffic, 7: interference,
				 *       8: chanim_stats
				 */
	struct ether_addr sta_addr;	/* causal STA's MAC address when known */
	uint16 metric_160_80;		/* latest dyn160 metric */
	uint8 nss;		/* NSS of the STA */
	uint8 bw;		/* BW of the STA */
	int8 rssi;		/* RSSI of the STA */
	uint8 traffic;		/* internal metric of traffic */
} wl_event_mode_switch_dyn160;

#define WL_EVENT_FBT_VER_1		1

#define WL_E_FBT_TYPE_FBT_OTD_AUTH	1
#define WL_E_FBT_TYPE_FBT_OTA_AUTH	2

/* event structure for WLC_E_FBT */
typedef struct {
	uint16 version;
	uint16 length;	/* size including 'data' field */
	uint16 type; /* value 0: unknown, 1: FBT OTD Auth Req */
	uint16 data_offset;	/* offset to 'data' from beginning of this struct.
				 * fields may be added between data_offset and data
				 */
	/* ADD NEW FIELDS HERE */
	uint8 data[];	/* type specific data; could be empty */
} wl_event_fbt_t;

/* TWT Setup Completion is designed to notify the user of TWT Setup process
 * status. When 'status' field is value of BCME_OK, the user must check the
 * 'setup_cmd' field value in 'wl_twt_sdesc_t' structure that at the end of
 * the event data to see the response from the TWT Responding STA; when
 * 'status' field is value of BCME_ERROR or non BCME_OK, user must not use
 * anything from 'wl_twt_sdesc_t' structure as it is the TWT Requesting STA's
 * own TWT parameter.
 */

#define WL_TWT_SETUP_CPLT_VER	0

/* TWT Setup Completion event data */
typedef struct wl_twt_setup_cplt {
	uint16 version;
	uint16 length;	/* the byte count of fields from 'dialog' onwards */
	uint8 dialog;	/* the dialog token user supplied to the TWT setup API */
	uint8 pad[3];
	int32 status;
	/* wl_twt_sdesc_t desc; - defined in wlioctl.h */
} wl_twt_setup_cplt_t;

#define WL_INVALID_IE_EVENT_VERSION	0

/* Invalid IE Event data */
typedef struct wl_invalid_ie_event {
	uint16 version;
	uint16 len;      /* Length of the invalid IE copy */
	uint16 type;     /* Type/subtype of the frame which contains the invalid IE */
	uint16 error;    /* error code of the wrong IE, defined in ie_error_code_t */
	uint8  ie[];     /* Variable length buffer for the invalid IE copy */
} wl_invalid_ie_event_t;

/* Fixed header portion of Invalid IE Event */
typedef struct wl_invalid_ie_event_hdr {
	uint16 version;
	uint16 len;      /* Length of the invalid IE copy */
	uint16 type;     /* Type/subtype of the frame which contains the invalid IE */
	uint16 error;    /* error code of the wrong IE, defined in ie_error_code_t */
	/* var length IE data follows */
} wl_invalid_ie_event_hdr_t;

typedef enum ie_error_code {
	IE_ERROR_OUT_OF_RANGE = 0x01
} ie_error_code_t;

/* This marks the end of a packed structure section. */
#include <packed_section_end.h>

#define EVENT_AGGR_DATA_HDR_LEN	8

typedef struct event_aggr_data {
	uint16  num_events; /* No of events aggregated */
	uint16	len;	/* length of the aggregated events, excludes padding */
	uint8	pad[4]; /* Padding to make aggr event packet header aligned
					 * on 64-bit boundary, for a 64-bit host system.
					 */
	uint8	data[];	/* Aggregate buffer containing Events */
} event_aggr_data_t;

#define WLC_E_TRF_THOLD_VER       2
#define WLC_E_TRF_THOLD_LEN       4
typedef struct  wlc_trf_thold_event {
	uint16 version;
	uint16 length;
	uint16 type;
	uint16 count;
} wlc_trf_thold_event_t;

#define WLC_E_TRAFFIC_THRESH_VER       2
#define WLC_E_TRAFFIC_THRESH_LEN       4
typedef struct  wlc_traffic_thresh_event {
	uint16 version;
	uint16 length;
	uint16 type;
	uint16 count;
} wlc_traffic_thresh_event_t;

/* used with WLC_E_OMN_MASTER */
#define WLC_E_OMNM_VER			1

typedef struct {
	uint16 ver;		/* see WLC_E_OMNM_VER above */
	uint16 len;		/* for forward compatibility to add optional elements at end */
	uint16 type;		/* unused */
	uint16 flags;		/* unused */
	uint16 mod;		/* see wl_omnm_mod_t in wlioctl.h */
	uint16 lock_life;	/* master lock expires after these many seconds */
} wl_event_omnm_t;

typedef struct  wl_edcrs_hi_event {
	uint16 version;
	uint16 length;
	uint16 type;
	uint16 status;
} wl_edcrs_hi_event_t;

#define WLC_E_EDCRS_HI_VER       1
#define WLC_E_EDCRS_HI_LEN       sizeof(wl_edcrs_hi_event_t)

/* TODO: Old defines/synonyms with "_INCUMBENT_* to be removed after related precommit checkins */
#define WLC_E_INCUMBENT_SIGNAL_EVENT	WLC_E_EDCRS_HI_EVENT
#define wl_incumbent_signal_event_t	wl_edcrs_hi_event_t
#define WLC_E_INCUMBENT_SIGNAL_VER	WLC_E_EDCRS_HI_VER
#define WLC_E_INCUMBENT_SIGNAL_LEN      WLC_E_EDCRS_HI_LEN

/* event data passed in WLC_E_MACDBG ratelinkmem update */
#define WLC_E_MACDBG_RLM_VERSION	1

/*
 * struct wlc_rlm_event - ratelinkmem update event.
 *
 * @version: version of this event structure.
 * @length: number of bytes after this field.
 * @action: type of update (not used for now).
 * @entry: ratelinkmem index.
 */
typedef struct wlc_rlm_event {
	uint16 version;
	uint16 length;
	uint16 action;
	uint16 entry;
} wlc_rlm_event_t;

/*
 * Event data for WLC_E_REQ_BW_UPGRADE event
 */
typedef struct wl_event_req_bw_upgd {
	uint16 version;
	uint16 length;
	uint16 flags;			/* Reserved */
	chanspec_t upgrd_chspec;	/* Target chanspec to upgrade to 160Mhz */
} wl_event_req_bw_upgd_t;
#define WL_EVENT_REQ_BW_UPGD_VER_1	1
#define WL_EVENT_REQ_BW_UPGD_LEN	8

/* opcode in WLC_E_QOS_MGMT event */
typedef enum {
	WLC_QOS_MAP_SET			= 1,	/* Qosmap set */
	WLC_QOS_MAP_ASSOC		= 2,	/* Qosmap assoc */
	WLC_MSCS_REQUEST		= 3,	/* MSCS request */
	WLC_MSCS_ASSOC			= 4,	/* MSCS assoc */
	WLC_SCS_REQUEST			= 5,	/* SCS request */
	WLC_DSCP_POLICY_QUERY		= 6,	/* DSCP policy query */
	WLC_QOS_VENDOR_SPECIFIC_REQUEST	= 7,	/* Vendor specific QoS management request */
	WLC_QOS_MSCS_STATE_CHANGE	= 8,	/* IOVAR initiated event: enable/disable MSCS */
	WLC_QOS_ASR_STATE_CHANGE	= 9,	/* IOVAR initiated event: enable/disable ASR */
	WLC_MSCS_TERMINATE		= 10,	/* MSCS terminate */
	WLC_QOS_SCS_STATE_CHANGE	= 11,	/* IOVAR initiated event: enable/disable SCS */
	WLC_DSCP_POLICY_ASSOC		= 12,	/* DSCP Policy assoc */
	WLC_QOS_VENDOR_SPECIFIC_AF	= 13,	/* Vendor specific QoS management actframe */
	WLC_QOS_ASSOC			= 14	/* Assoc */
} wl_qosmgmt_reason_t;

typedef struct {
	uint16			tag;	/* data tag */
	uint16			length; /* data length */
	uint8			value[]; /* data value with variable length specified by length */
} bcm_alertind_t;

/* Tags to indicate the host of an alert/bad state
 * 0xCXXX - C is for access, 0 is mobility code
 */
typedef enum {
	ALERT_IND_TAG_ASSERT = 0xC001,
	ALERT_IND_TAG_HEALTH_CHECK_TRAP,
	ALERT_IND_TAG_HEALTH_CHECK_ERR
} alert_ind_tag_t;

/* Health check top level module tags */
typedef struct {
	uint16			top_module_tag;	/* top level module tag */
	uint16			top_module_len; /* Type of PCIE issue indication */
	uint8			value[]; /* data value with variable length specified by length */
} bcm_event_healthcheck_t;

/* WLD - Wireless log buffer type */
typedef enum {
	WLD_BUF_TYPE_GENERAL = 0,
	WLD_BUF_TYPE_SPECIAL,
	WLD_BUF_TYPE_ECNTRS,
	WLD_BUF_TYPE_FILTER,
	WLD_BUF_TYPE_HEALTH_CHECK,
	WLD_BUF_TYPE_ALL
} wl_log_dump_type_t;

/* Radar Detected Event(WLC_E_RADAR_DETECTED) Reasons */
#define WLC_E_REASON_RADAR_CHANNEL_DETECTED	0
#define WLC_E_REASON_RADAR_CHANNEL_CLEARED	1

/* Health check defines */

/* Different health Check module ID's, used in enum to string conversion
 * Reference: wlioctl.h file
 */
#ifndef HC_MODULE_NAME_ARRAY_INITIALIZER
#define HC_MODULE_NAME_ARRAY_INITIALIZER	\
	[WL_HC_DD_UNDEFINED] = \
		"UNDEFINED", \
	[WL_HC_DD_PCIE] = \
		"PCIE", \
	[WL_HC_DD_RX_DMA_STALL] = \
		"RX_DMA_STALL", \
	[WL_HC_DD_RX_STALL] = \
		"RX_STALL", \
	[WL_HC_DD_TX_STALL] = \
		"TX_STALL", \
	[WL_HC_DD_SCAN_STALL] = \
		"SCAN_STALL", \
	[WL_HC_DD_PHY] = \
		"PHY", \
	[WL_HC_DD_REINIT] = \
		"REINIT", \
	[WL_HC_DD_TXQ_STALL] = \
		"TXQ_STALL", \
	[WL_HC_DD_SOUNDING] = \
		"SOUNDING"
#endif /* HC_MODULE_NAME_ARRAY_INITIALIZER */

/* The HC event buffer has a size limit (See hnd_ev_ptr->maxsz).
 * Consider 1450 bytes excluding overheads
 */

#define WL_HEALTH_CHECK_MODULE_BUF_MAXSIZE	1450

/* Health check status format:
 * reporting status size = uint32
 * 8 LSB bits are reserved for: WARN (0), ERROR (1), and other levels
 * MSB 24 bits are reserved for client to fill in its specific status
 */

#define WL_HEALTH_CHECK_STATUS_OK			0
/* Bit positions. */
#define WL_HEALTH_CHECK_STATUS_WARN		0x1
#define WL_HEALTH_CHECK_STATUS_ERROR		0x2
#define WL_HEALTH_CHECK_STATUS_TRAP		0x4
#define WL_HEALTH_CHECK_STATUS_NOEVENT		0x8

/* Indication that required information is populated in log buffers */
#define WL_HEALTH_CHECK_STATUS_INFO_LOG_BUF	0x80
#define WL_HEALTH_CHECK_STATUS_MASK		(0xFF)

#define WL_HEALTH_CHECK_STATUS_MSB_SHIFT	8

#define HEALTH_CHECK_STATUS_MASK_FAIL	(0x0F)

/* Do not send any notification to app etc (eg cevent) */
#define HEALTH_CHECK_STATUS_NO_NOTIF	0x10

/** Data stall detection accounting levels */
enum {
	WLC_DSTALL_MODULE_GLOBAL = 1, /* wl driver level  */
	WLC_DSTALL_MODULE_BSSCFG, /* bsscfg level */
	WLC_DSTALL_MODULE_SCB, /* SCB level */
	WLC_DSTALL_MODULE_NONE
};

/** Stall detection types(bitwise) */
#define	WLC_DSTALL_NOTFOUND	0 /* No Data STALL detected */
#define	WLC_DSTALL_ONCE	1 /* Data stall is detected in this time interval */
#define	WLC_DSTALL_PROLONG	2 /* Data stalls are detected for continuous N(timeout) sec */

/* Health Check report structure for data packet failure check */
typedef struct {
	uint8	stall_module; /* wlc, bss and sta for stall module */
	uint8	stall_type; /* Prolonged stall or just once */
	uint8	bssidx; /* bss index */
	struct ether_addr ethaddr; /* MAC ID of the stalled device/level */
} wl_dstall_hc_log_t;

/* The HC event buffer has a size limit (See hnd_ev_ptr->maxsz).
 * Consider 1400 bytes / sizeof(wl_txstall_hc_info_t ~ 9B) = 155
 */
#define WLC_DSTALL_MAX_REPORTS	155

/* Health Check report structure for Tx packet failure check */
typedef struct {
	uint16 type;	/* TX/RX Stall */
	uint16 length;	/* Length of the total filled stall_log */
	wl_dstall_hc_log_t dstall_log[WLC_DSTALL_MAX_REPORTS];
} wl_dstall_hc_info_t;

/* Below structure is used to update bss index to ifname conversion and which will be
 * used by dhd_monitor to identity right interface for action
 * stall_mode - module in which stall has happened (wlc/bss/sta level)
 * stall_type - prolonged stall or once stall
 * ifname -  bss index to ifname
 * ethaddr - mac address of stalled device
 */

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif /* IFNAMSIZ */

typedef struct {
	uint8   stall_module; /* wlc, bss and sta for stall module */
	uint8   stall_type; /* Prolonged stall or just once */
	uint8   ifname[IFNAMSIZ];
	struct ether_addr ethaddr; /* MAC ID of the stalled device/level */
} wl_dstall_hc_log_extended_t;

/* Below structure in tlv format to update to dhd_monitor "WLC_DSTALL_MAX_REPORTS" reports */
typedef struct {
	uint16 type;
	uint16 length;
	wl_dstall_hc_log_extended_t dstall_log_ext[WLC_DSTALL_MAX_REPORTS];
} wl_dstall_hc_info_extended_t;

/* Health Check report structure for a single module for cevent(+pkt drop rsn) */
typedef struct {
	uint16 type;
	uint16 length;	/* Length of the total filled txstall_log */
	wl_dstall_hc_log_t dstall_log;
	uint8	last_pktdrop_rsn; /* Rsn for the last pkt drop reported */
} wl_dstall_hc_info_cev_t;

/* Health Check report log structure for sounding failure check */
typedef struct {
	uint8	stall_module;  /* stall module */
	uint8	stall_type;    /* Prolonged stall or just once */
} wl_hc_sounding_log_t;

/* The HC event buffer has a size limit (See hnd_ev_ptr->maxsz).
 * Consider 80 bytes = sizeof(wl_sounding_hc_log_t ~ 8B)
 */
#define WL_SOUNDING_MAX_REPORTS	10

/* Health Check report structure for Tx packet failure check */
typedef struct {
	uint16 type;
	uint16 length;	/* Length of the total filled sounding_log */
	wl_hc_sounding_log_t sounding_log[WL_SOUNDING_MAX_REPORTS];
} wl_hc_sounding_info_t;

typedef struct mld_link_info {
	int8 mld_unit;
	int8 link_id;
} mld_link_info_t;
#endif /* _BCMEVENT_H_ */
