/*
 * bcmevent read-only data shared by kernel or app layers
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
 * $Id: bcmevent.c 834507 2023-12-20 10:58:53Z $
 */

#include <typedefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <ethernet.h>
#include <bcmeth.h>
#include <bcmevent.h>
#include <802.11.h>

/* Table of event name strings for UIs and debugging dumps */
typedef struct {
	uint event;
	const char *name;
} bcmevent_name_str_t;

/* Use the actual name for event tracing */
#define BCMEVENT_NAME(_event) {(_event), #_event}

/* XXX - this becomes static data when all code is changed to use
 * the bcmevent_get_name() API
 */
static const bcmevent_name_str_t bcmevent_names[] = {
	BCMEVENT_NAME(WLC_E_SET_SSID),
	BCMEVENT_NAME(WLC_E_JOIN),
	BCMEVENT_NAME(WLC_E_START),
	BCMEVENT_NAME(WLC_E_AUTH),
	BCMEVENT_NAME(WLC_E_AUTH_IND),
	BCMEVENT_NAME(WLC_E_DEAUTH),
	BCMEVENT_NAME(WLC_E_DEAUTH_IND),
	BCMEVENT_NAME(WLC_E_ASSOC),
	BCMEVENT_NAME(WLC_E_ASSOC_IND),
	BCMEVENT_NAME(WLC_E_REASSOC),
	BCMEVENT_NAME(WLC_E_REASSOC_IND),
	BCMEVENT_NAME(WLC_E_DISASSOC),
	BCMEVENT_NAME(WLC_E_DISASSOC_IND),
	BCMEVENT_NAME(WLC_E_QUIET_START),
	BCMEVENT_NAME(WLC_E_QUIET_END),
	BCMEVENT_NAME(WLC_E_BEACON_RX),
	BCMEVENT_NAME(WLC_E_LINK),
	BCMEVENT_NAME(WLC_E_MIC_ERROR),
	BCMEVENT_NAME(WLC_E_NDIS_LINK),
	BCMEVENT_NAME(WLC_E_ROAM),
	BCMEVENT_NAME(WLC_E_TXFAIL),
	BCMEVENT_NAME(WLC_E_PMKID_CACHE),
	BCMEVENT_NAME(WLC_E_RETROGRADE_TSF),
	BCMEVENT_NAME(WLC_E_PRUNE),
	BCMEVENT_NAME(WLC_E_AUTOAUTH),
	BCMEVENT_NAME(WLC_E_EAPOL_MSG),
	BCMEVENT_NAME(WLC_E_SCAN_COMPLETE),
	BCMEVENT_NAME(WLC_E_ADDTS_IND),
	BCMEVENT_NAME(WLC_E_DELTS_IND),
	BCMEVENT_NAME(WLC_E_BCNSENT_IND),
	BCMEVENT_NAME(WLC_E_BCNRX_MSG),
	BCMEVENT_NAME(WLC_E_BCNLOST_MSG),
	BCMEVENT_NAME(WLC_E_ROAM_PREP),
	BCMEVENT_NAME(WLC_E_PFN_NET_FOUND),
	BCMEVENT_NAME(WLC_E_PFN_SCAN_ALLGONE),
	BCMEVENT_NAME(WLC_E_PFN_NET_LOST),
	BCMEVENT_NAME(WLC_E_JOIN_START),
	BCMEVENT_NAME(WLC_E_ROAM_START),
	BCMEVENT_NAME(WLC_E_ASSOC_START),
	BCMEVENT_NAME(WLC_E_RADIO),
	BCMEVENT_NAME(WLC_E_PSM_WATCHDOG),
	BCMEVENT_NAME(WLC_E_PROBREQ_MSG),
	BCMEVENT_NAME(WLC_E_SCAN_CONFIRM_IND),
	BCMEVENT_NAME(WLC_E_PSK_SUP),
	BCMEVENT_NAME(WLC_E_COUNTRY_CODE_CHANGED),
	BCMEVENT_NAME(WLC_E_EXCEEDED_MEDIUM_TIME),
	BCMEVENT_NAME(WLC_E_ICV_ERROR),
	BCMEVENT_NAME(WLC_E_UNICAST_DECODE_ERROR),
	BCMEVENT_NAME(WLC_E_MULTICAST_DECODE_ERROR),
	BCMEVENT_NAME(WLC_E_TRACE),
	BCMEVENT_NAME(WLC_E_IF),
#ifdef WLP2P
	BCMEVENT_NAME(WLC_E_P2P_DISC_LISTEN_COMPLETE),
#endif // endif
	BCMEVENT_NAME(WLC_E_RSSI),
	BCMEVENT_NAME(WLC_E_PFN_SCAN_COMPLETE),
	BCMEVENT_NAME(WLC_E_ACTION_FRAME),
	BCMEVENT_NAME(WLC_E_ACTION_FRAME_RX),
	BCMEVENT_NAME(WLC_E_ACTION_FRAME_COMPLETE),
	BCMEVENT_NAME(WLC_E_PRE_ASSOC_IND),
	BCMEVENT_NAME(WLC_E_PRE_REASSOC_IND),
	BCMEVENT_NAME(WLC_E_ESCAN_RESULT),
	BCMEVENT_NAME(WLC_E_ACTION_FRAME_OFF_CHAN_COMPLETE),
#ifdef WLP2P
	BCMEVENT_NAME(WLC_E_PROBRESP_MSG),
	BCMEVENT_NAME(WLC_E_P2P_PROBREQ_MSG),
#endif // endif
#ifdef PROP_TXSTATUS
	BCMEVENT_NAME(WLC_E_FIFO_CREDIT_MAP),
#endif // endif
	BCMEVENT_NAME(WLC_E_WAKE_EVENT),
	BCMEVENT_NAME(WLC_E_RM_COMPLETE),
#ifdef WLMEDIA_HTSF
	BCMEVENT_NAME(WLC_E_HTSFSYNC),
#endif // endif
	BCMEVENT_NAME(WLC_E_OVERLAY_REQ),
	BCMEVENT_NAME(WLC_E_CSA_COMPLETE_IND),
	BCMEVENT_NAME(WLC_E_EXCESS_PM_WAKE_EVENT),
	BCMEVENT_NAME(WLC_E_PFN_SCAN_NONE),
	BCMEVENT_NAME(WLC_E_PFN_SCAN_ALLGONE),
#ifdef SOFTAP
	BCMEVENT_NAME(WLC_E_GTK_PLUMBED),
#endif // endif
	BCMEVENT_NAME(WLC_E_ASSOC_REQ_IE),
	BCMEVENT_NAME(WLC_E_ASSOC_RESP_IE),
	BCMEVENT_NAME(WLC_E_BEACON_FRAME_RX),
	BCMEVENT_NAME(WLC_E_NATIVE),
#ifdef WLPKTDLYSTAT
	BCMEVENT_NAME(WLC_E_PKTDELAY_IND),
#endif /* WLPKTDLYSTAT */
	BCMEVENT_NAME(WLC_E_SERVICE_FOUND),
	BCMEVENT_NAME(WLC_E_GAS_FRAGMENT_RX),
	BCMEVENT_NAME(WLC_E_GAS_COMPLETE),
	BCMEVENT_NAME(WLC_E_P2PO_ADD_DEVICE),
	BCMEVENT_NAME(WLC_E_P2PO_DEL_DEVICE),
#ifdef WLWNM
	BCMEVENT_NAME(WLC_E_WNM_STA_SLEEP),
#endif /* WLWNM */
#ifdef WL_PROXDETECT
	BCMEVENT_NAME(WLC_E_PROXD),
#endif // endif
	BCMEVENT_NAME(WLC_E_CCA_CHAN_QUAL),
	BCMEVENT_NAME(WLC_E_BSSID),
#ifdef PROP_TXSTATUS
	BCMEVENT_NAME(WLC_E_BCMC_CREDIT_SUPPORT),
#endif // endif
	BCMEVENT_NAME(WLC_E_PSTA_PRIMARY_INTF_IND),
	BCMEVENT_NAME(WLC_E_TXFAIL_THRESH),
#ifdef GSCAN_SUPPORT
	BCMEVENT_NAME(WLC_E_PFN_GSCAN_FULL_RESULT),
	BCMEVENT_NAME(WLC_E_PFN_SSID_EXT),
#endif /* GSCAN_SUPPORT */
#ifdef WLBSSLOAD_REPORT
	BCMEVENT_NAME(WLC_E_BSS_LOAD),
#endif // endif
#if defined(BT_WIFI_HANDOVER) || defined(WL_TBOW)
	BCMEVENT_NAME(WLC_E_BT_WIFI_HANDOVER_REQ),
#endif // endif
#ifdef WLFBT
	BCMEVENT_NAME(WLC_E_FBT),
#endif /* WLFBT */
	BCMEVENT_NAME(WLC_E_AUTHORIZED),
	BCMEVENT_NAME(WLC_E_PROBREQ_MSG_RX),
	BCMEVENT_NAME(WLC_E_CSA_START_IND),
	BCMEVENT_NAME(WLC_E_CSA_DONE_IND),
	BCMEVENT_NAME(WLC_E_CSA_FAILURE_IND),
	BCMEVENT_NAME(WLC_E_RMC_EVENT),
	BCMEVENT_NAME(WLC_E_DPSTA_INTF_IND),
	BCMEVENT_NAME(WLC_E_RRM),
	BCMEVENT_NAME(WLC_E_ALLOW_CREDIT_BORROW),
	BCMEVENT_NAME(WLC_E_MSCH),
	BCMEVENT_NAME(WLC_E_PKT_FILTER),
	BCMEVENT_NAME(WLC_E_DMA_TXFLUSH_COMPLETE),
	BCMEVENT_NAME(WLC_E_PSK_AUTH),
	BCMEVENT_NAME(WLC_E_SDB_TRANSITION),
	BCMEVENT_NAME(WLC_E_PFN_SCAN_BACKOFF),
	BCMEVENT_NAME(WLC_E_PFN_BSSID_SCAN_BACKOFF),
	BCMEVENT_NAME(WLC_E_PRE_ASSOC_RSEP_IND),
	BCMEVENT_NAME(WLC_E_PSK_AUTH),
	BCMEVENT_NAME(WLC_E_TKO),
	BCMEVENT_NAME(WLC_E_SDB_TRANSITION),
	BCMEVENT_NAME(WLC_E_TEMP_THROTTLE),
	BCMEVENT_NAME(WLC_E_LINK_QUALITY),
	BCMEVENT_NAME(WLC_E_BSSTRANS_RESP),
	BCMEVENT_NAME(WLC_E_HE_TWT_SETUP),
	BCMEVENT_NAME(WLC_E_RADAR_DETECTED),
	BCMEVENT_NAME(WLC_E_RANGING_EVENT),
	BCMEVENT_NAME(WLC_E_INVALID_IE),
	BCMEVENT_NAME(WLC_E_MODE_SWITCH),
	BCMEVENT_NAME(WLC_E_PKT_FILTER),
	BCMEVENT_NAME(WLC_E_DMA_TXFLUSH_COMPLETE),
	BCMEVENT_NAME(WLC_E_FBT),
	BCMEVENT_NAME(WLC_E_PFN_SCAN_BACKOFF),
	BCMEVENT_NAME(WLC_E_PFN_BSSID_SCAN_BACKOFF),
	BCMEVENT_NAME(WLC_E_AGGR_EVENT),
	BCMEVENT_NAME(WLC_E_AP_CHAN_CHANGE),
	BCMEVENT_NAME(WLC_E_PSTA_CREATE_IND),
	BCMEVENT_NAME(WLC_E_AIRIQ_EVENT),
	BCMEVENT_NAME(WLC_E_FRAME_FIRST_RX),
	BCMEVENT_NAME(WLC_E_BCN_STUCK),
	BCMEVENT_NAME(WLC_E_PROBSUP_IND),
	BCMEVENT_NAME(WLC_E_SAS_RSSI),
	BCMEVENT_NAME(WLC_E_RATE_CHANGE),
	BCMEVENT_NAME(WLC_E_AMT_CHANGE),
	BCMEVENT_NAME(WLC_E_LTE_U_EVENT),
	BCMEVENT_NAME(WLC_E_CEVENT),
	BCMEVENT_NAME(WLC_E_HWA_EVENT),
	BCMEVENT_NAME(WLC_E_DFS_HIT),
	BCMEVENT_NAME(WLC_E_TXFAIL_TRFTHOLD),
	BCMEVENT_NAME(WLC_E_CAC_STATE_CHANGE),
	BCMEVENT_NAME(WLC_E_REQ_BW_CHANGE),
	BCMEVENT_NAME(WLC_E_ASSOC_REASSOC_IND_EXT),
	BCMEVENT_NAME(WLC_E_WNM_ERR),
	BCMEVENT_NAME(WLC_E_ASSOC_FAIL),
	BCMEVENT_NAME(WLC_E_REASSOC_FAIL),
	BCMEVENT_NAME(WLC_E_AUTH_FAIL),
	BCMEVENT_NAME(WLC_E_BSSTRANS_REQ),
	BCMEVENT_NAME(WLC_E_BSSTRANS_QUERY),
	BCMEVENT_NAME(WLC_E_START_AUTH),
	BCMEVENT_NAME(WLC_E_OMN_MASTER),
	BCMEVENT_NAME(WLC_E_MBO_CAPABILITY_STATUS),
	BCMEVENT_NAME(WLC_E_WNM_NOTIFICATION_REQ),
	BCMEVENT_NAME(WLC_E_WNM_BSSTRANS_QUERY),
	BCMEVENT_NAME(WLC_E_GAS_RQST_ANQP_QUERY),
	BCMEVENT_NAME(WLC_E_PWR_SAVE_SYNC),
	BCMEVENT_NAME(WLC_E_PROBREQ_MSG_RX_EXT),
	BCMEVENT_NAME(WLC_E_BAND_CHANGE),
	BCMEVENT_NAME(WLC_E_COLOR),
	BCMEVENT_NAME(WLC_E_CAPTURE_RXFRAME),
	BCMEVENT_NAME(WLC_E_CAPTURE_TXFRAME),
	BCMEVENT_NAME(WLC_E_BUZZZ),
	BCMEVENT_NAME(WLC_E_INCUMBENT_SIGNAL_EVENT),
	BCMEVENT_NAME(WLC_E_QOS_MGMT),
	BCMEVENT_NAME(WLC_E_HEALTH_CHECK),
	BCMEVENT_NAME(WLC_E_CSA_RECV_IND),
	BCMEVENT_NAME(WLC_E_PASN_AUTH),
};

const char *bcmevent_get_name(uint event_type)
{
	/* note:  first coded this as a static const but some
	 * ROMs already have something called event_name so
	 * changed it so we don't have a variable for the
	 * 'unknown string
	 */
	const char *event_name = NULL;

	uint idx;
	for (idx = 0; idx < (uint)ARRAYSIZE(bcmevent_names); idx++) {

		if (bcmevent_names[idx].event == event_type) {
			event_name = bcmevent_names[idx].name;
			break;
		}
	}

	/* if we find an event name in the array, return it.
	 * otherwise return unknown string.
	 */
	return ((event_name) ? event_name : "Unknown Event");
}

void
wl_event_to_host_order(wl_event_msg_t * evt)
{
	/* Event struct members passed from dongle to host are stored in network
	* byte order. Convert all members to host-order.
	*/
	evt->event_type = ntoh32(evt->event_type);
	evt->flags = ntoh16(evt->flags);
	evt->status = ntoh32(evt->status);
	evt->reason = ntoh32(evt->reason);
	evt->auth_type = ntoh32(evt->auth_type);
	evt->datalen = ntoh32(evt->datalen);
	evt->version = ntoh16(evt->version);
}

void
wl_event_to_network_order(wl_event_msg_t * evt)
{
	/* Event struct members passed from dongle to host are stored in network
	* byte order. Convert all members to host-order.
	*/
	evt->event_type = hton32(evt->event_type);
	evt->flags = hton16(evt->flags);
	evt->status = hton32(evt->status);
	evt->reason = hton32(evt->reason);
	evt->auth_type = hton32(evt->auth_type);
	evt->datalen = hton32(evt->datalen);
	evt->version = hton16(evt->version);
}

/*
 * Validate if the event is proper and if valid copy event header to event.
 * If proper event pointer is passed, to just validate, pass NULL to event.
 *
 * Return values are
 *	BCME_OK - It is a BRCM event or BRCM dongle event
 *	BCME_NOTFOUND - Not BRCM, not an event, may be okay
 *	BCME_BADLEN - Bad length, should not process, just drop
 */
int
is_wlc_event_frame(void *pktdata, uint pktlen, uint16 exp_usr_subtype,
	bcm_event_msg_u_t *out_event)
{
	uint16 evlen = 0;	/* length in bcmeth_hdr */
	uint16 subtype;
	uint16 usr_subtype;
	bcm_event_t *bcm_event;
	uint8 *pktend;
	uint8 *evend;
	int err = BCME_OK;
	uint32 data_len = 0; /* data length in bcm_event */

	pktend = (uint8 *)pktdata + pktlen;
	bcm_event = (bcm_event_t *)pktdata;

	/* only care about 16-bit subtype / length versions */
	if ((uint8 *)&bcm_event->bcm_hdr < pktend) {
		uint8 short_subtype = *(uint8 *)&bcm_event->bcm_hdr;
		if (!(short_subtype & 0x80)) {
			err = BCME_NOTFOUND;
			goto done;
		}
	}

	/* must have both ether_header and bcmeth_hdr */
	if (pktlen < OFFSETOF(bcm_event_t, event)) {
		err = BCME_BADLEN;
		goto done;
	}

	/* check length in bcmeth_hdr */

	evlen = ntoh16_ua((void *)&bcm_event->bcm_hdr.length);
	evend = (uint8 *)&bcm_event->bcm_hdr.version + evlen;
	if (evend > pktend) {
		err = BCME_BADLEN;
		goto done;
	}

	/* match on subtype, oui and usr subtype for BRCM events */
	subtype = ntoh16_ua((void *)&bcm_event->bcm_hdr.subtype);
	if (subtype != BCMILCP_SUBTYPE_VENDOR_LONG) {
		err = BCME_NOTFOUND;
		goto done;
	}

	if (bcmp(BRCM_OUI, &bcm_event->bcm_hdr.oui[0], DOT11_OUI_LEN)) {
		err = BCME_NOTFOUND;
		goto done;
	}

	/* if it is a bcm_event or bcm_dngl_event_t, validate it */
	usr_subtype = ntoh16_ua((void *)&bcm_event->bcm_hdr.usr_subtype);
	switch (usr_subtype) {
	case BCMILCP_BCM_SUBTYPE_EVENT:
		/* check that header length and pkt length are sufficient */
		if ((pktlen < sizeof(bcm_event_t)) ||
			(evend < ((uint8 *)bcm_event + sizeof(bcm_event_t)))) {
			err = BCME_BADLEN;
			goto done;
		}

		/* ensure data length in event is not beyond the packet. */
		data_len = ntoh32_ua((void *)&bcm_event->event.datalen);
		if (data_len > (pktlen - sizeof(bcm_event_t))) {
			err = BCME_BADLEN;
			goto done;
		}

		if (exp_usr_subtype && (exp_usr_subtype != usr_subtype)) {
			err = BCME_NOTFOUND;
			goto done;
		}

		if (out_event) {
			/* ensure BRCM event pkt aligned */
			memcpy(&out_event->event, &bcm_event->event, sizeof(wl_event_msg_t));
		}

		break;
	default:
		err = BCME_NOTFOUND;
		goto done;
	}

	BCM_REFERENCE(data_len);
done:
	return err;
}
