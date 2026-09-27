/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <net/if.h>

#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>

#include <linux/nl80211.h>
#include <mtk_vendor_nl80211.h>

#include <ralink.h>
#include <shared.h>

/* Ciphers Suite */
#define CIPHER_WEP_40		(1U << 1)
#define CIPHER_TKIP		(1U << 2)
#define CIPHER_CCMP		(1U << 4)
#define CIPHER_WEP_104		(1U << 5)
#define CIPHER_GCMP_256		(1U << 9)

/* AKM (Authentication Key Management) Suite */
#define AUTH_IEEE_802P1X	(1U << 1)
#define AUTH_PSK		(1U << 2)
#define AUTH_SAE		(1U << 8)

/* Wireless Mode */
#define WMODE_HT		(1U << 1)
#define WMODE_VHT		(1U << 2)
#define WMODE_HE		(1U << 3)
#define WMODE_EHT		(1U << 4)

struct trigger_results {
	int done;
	int aborted;
};

/* used to merge mt_wifi7 reply data */
int asuscmd_reply_len;

/*** copy form src-mtk-7988_7990/mt799x/iw-tiny/source ***/
struct nl80211_state {
	struct nl_sock *nl_sock;
	int nl80211_id;
};

static int nl80211_init(struct nl80211_state *state)
{
	int err;

	state->nl_sock = nl_socket_alloc();
	if (!state->nl_sock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}

	if (genl_connect(state->nl_sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

	nl_socket_set_buffer_size(state->nl_sock, 8192, 8192);

	/* try to set NETLINK_EXT_ACK to 1, ignoring errors */
	err = 1;
	setsockopt(nl_socket_get_fd(state->nl_sock), SOL_NETLINK,
		   NETLINK_EXT_ACK, &err, sizeof(err));

	state->nl80211_id = genl_ctrl_resolve(state->nl_sock, "nl80211");
	if (state->nl80211_id < 0) {
		fprintf(stderr, "nl80211 not found.\n");
		err = -ENOENT;
		goto out_handle_destroy;
	}

	return 0;

 out_handle_destroy:
	nl_socket_free(state->nl_sock);
	return err;
}

static void nl80211_cleanup(struct nl80211_state *state)
{
	nl_socket_free(state->nl_sock);
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	struct nlmsghdr *nlh = (struct nlmsghdr *)err - 1;
	int len = nlh->nlmsg_len;
	struct nlattr *attrs;
	struct nlattr *tb[NLMSGERR_ATTR_MAX + 1];
	int *ret = arg;
	int ack_len = sizeof(*nlh) + sizeof(int) + sizeof(*nlh);

	if (err->error > 0) {
		/*
		 * This is illegal, per netlink(7), but not impossible (think
		 * "vendor commands"). Callers really expect negative error
		 * codes, so make that happen.
		 */
		fprintf(stderr,
			"ERROR: received positive netlink error code %d\n",
			err->error);
		*ret = -EPROTO;
	} else {
		*ret = err->error;
	}

	if (!(nlh->nlmsg_flags & NLM_F_ACK_TLVS))
		return NL_STOP;

	if (!(nlh->nlmsg_flags & NLM_F_CAPPED))
		ack_len += err->msg.nlmsg_len - sizeof(*nlh);

	if (len <= ack_len)
		return NL_STOP;

	attrs = (void *)((unsigned char *)nlh + ack_len);
	len -= ack_len;

	nla_parse(tb, NLMSGERR_ATTR_MAX, attrs, len, NULL);
	if (tb[NLMSGERR_ATTR_MSG]) {
		len = strnlen((char *)nla_data(tb[NLMSGERR_ATTR_MSG]),
			      nla_len(tb[NLMSGERR_ATTR_MSG]));
		fprintf(stderr, "kernel reports: %*s\n", len,
			(char *)nla_data(tb[NLMSGERR_ATTR_MSG]));
	}

	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

static int no_seq_check(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

struct handler_args {
	const char *group;
	int id;
};

static int family_handler(struct nl_msg *msg, void *arg)
{
	struct handler_args *grp = arg;
	struct nlattr *tb[CTRL_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *mcgrp;
	int rem_mcgrp;

	nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[CTRL_ATTR_MCAST_GROUPS])
		return NL_SKIP;

	nla_for_each_nested(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], rem_mcgrp) {
		struct nlattr *tb_mcgrp[CTRL_ATTR_MCAST_GRP_MAX + 1];

		nla_parse(tb_mcgrp, CTRL_ATTR_MCAST_GRP_MAX,
			  nla_data(mcgrp), nla_len(mcgrp), NULL);

		if (!tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME] ||
		    !tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID])
			continue;
		if (strncmp(nla_data(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME]),
			    grp->group, nla_len(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME])))
			continue;
		grp->id = nla_get_u32(tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID]);
		break;
	}

	return NL_SKIP;
}

int nl_get_multicast_id(struct nl_sock *sock, const char *family, const char *group)
{
	struct nl_msg *msg;
	struct nl_cb *cb;
	int ret, ctrlid;
	struct handler_args grp = {
		.group = group,
		.id = -ENOENT,
	};

	msg = nlmsg_alloc();
	if (!msg)
		return -ENOMEM;

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		ret = -ENOMEM;
		goto out_fail_cb;
	}

	ctrlid = genl_ctrl_resolve(sock, "nlctrl");

	genlmsg_put(msg, 0, 0, ctrlid, 0,
		    0, CTRL_CMD_GETFAMILY, 0);

	ret = -ENOBUFS;
	NLA_PUT_STRING(msg, CTRL_ATTR_FAMILY_NAME, family);

	ret = nl_send_auto_complete(sock, msg);
	if (ret < 0)
		goto out;

	ret = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, family_handler, &grp);

	while (ret > 0)
		nl_recvmsgs(sock, cb);

	if (ret == 0)
		ret = grp.id;
 nla_put_failure:
 out:
	nl_cb_put(cb);
 out_fail_cb:
	nlmsg_free(msg);
	return ret;
}

void mac_addr_n2a(char *mac_addr, const unsigned char *arg)
{
	int i, l;

	l = 0;
	for (i = 0; i < ETH_ALEN ; i++) {
		if (i == 0) {
			sprintf(mac_addr+l, "%02x", arg[i]);
			l += 2;
		} else {
			sprintf(mac_addr+l, ":%02x", arg[i]);
			l += 3;
		}
	}
}
/*** copy form src-mtk-7988_7990/mt799x/iw-tiny/source ***/

/*
 * Called by the kernel when the scan is done or aborted.
 */
static int callback_trigger(struct nl_msg *msg, void *arg)
{
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct trigger_results *results = arg;

	if (gnlh->cmd == NL80211_CMD_SCAN_ABORTED) {
		//fprintf(stderr, "got NL80211_CMD_SCAN_ABORTED.\n");
		results->done = 1;
		results->aborted = 1;
	}
	else if (gnlh->cmd == NL80211_CMD_NEW_SCAN_RESULTS) {
		//fprintf(stderr, "got NL80211_CMD_NEW_SCAN_RESULTS.\n");
		results->done = 1;
		results->aborted = 0;
	}

	return NL_SKIP;
}

/*
 * iw dev <ifname> scan trigger
 * Trigger the wireless driver to perform a scan
 */
int do_scan_trigger(struct nl80211_state *state, char *ifname)
{
	struct trigger_results results = { .done = 0, .aborted = 0 };
	struct nl_msg *msg;
	struct nl_cb *cb;
	struct nl_msg *ssids;
	int err;
	int mcid = nl_get_multicast_id(state->nl_sock, "nl80211", "scan");
	signed long long devidx = if_nametoindex(ifname);

	/* without this, callback_trigger() won't be called. */
	nl_socket_add_membership(state->nl_sock, mcid);

	/* allocate the messages and callback handler. */
	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message for msg\n");
		return -ENOMEM;
	}
	ssids = nlmsg_alloc();
	if (!ssids) {
		fprintf(stderr, "failed to allocate netlink message for ssids\n");
		nlmsg_free(msg);
		return -ENOMEM;
	}
	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		nlmsg_free(msg);
		nlmsg_free(ssids);
		return -ENOMEM;
	}

	/* setup the messages */
	genlmsg_put(msg, 0, 0, state->nl80211_id, 0, 0, NL80211_CMD_TRIGGER_SCAN, 0);
	nla_put_u32(msg, NL80211_ATTR_IFINDEX, devidx);
	nla_put(ssids, 1, 0, "");
	nla_put_nested(msg, NL80211_ATTR_SCAN_SSIDS, ssids);
	nlmsg_free(ssids);

	err = nl_send_auto_complete(state->nl_sock, msg);
	if (err < 0)
		goto out;

	/* callback handler. */
	err = 1;
	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, callback_trigger, &results);
	/* no sequence checking for multicast messages */
	nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);

	/* first wait for ack_handler(). This helps with basic errors. */
	while (err > 0)
		nl_recvmsgs(state->nl_sock, cb);
	if (err < 0)
		fprintf(stderr, "WARNING: err %d.\n", err);
	/* wait for the scan to done or abort. */
	while (!results.done)
		nl_recvmsgs(state->nl_sock, cb);
	if (results.aborted)
		fprintf(stderr, "kernel aborted scan.\n");

out:
	nlmsg_free(msg);
	nl_cb_put(cb);
	nl_socket_drop_membership(state->nl_sock, mcid);

	return 0;
}

/*
 * Convert frequency to number of channels
 * reference form shared/sysdeps/api-qca.c
 */
u_int ieee80211_mhz2ieee(u_int freq)
{
	#define IS_CHAN_IN_PUBLIC_SAFETY_BAND(_c) ((_c) > 4940 && (_c) < 4990)
	if (freq < 2412)
		return 0;
	if (freq == 2484)
		return 14;
	if (freq < 2484)
		return (freq - 2407) / 5;
	if (freq < 5000) {
		if (IS_CHAN_IN_PUBLIC_SAFETY_BAND(freq)) {
			return ((freq * 10) + (((freq % 5) == 2) ? 5 : 0) - 49400) / 5;
		} else if (freq > 4900) {
			return (freq - 4000) / 5;
		} else {
			return 15 + ((freq - 2512) / 20);
		}
	}
	if (freq >= 5955 && freq <= 7115) {
		return (freq - 5950) / 5;
	}
	return (freq - 5000) / 5;
}

/*
 * Parse SSID from IE
 */
static void fprint_ssid(FILE *fp, unsigned char *ie, int ielen)
{
	uint8_t ssid[128] = { 0 };
	uint8_t *data;
	uint8_t len;

	if (ie == NULL || ielen < 0)
		return;

	/* reference from print_ies() and print_ie() */
	while (ielen >= 2 && ielen - 2 >= ie[1]) {
		/* SSID: id = 0 & length = 0~32 */
		if (ie[0] == 0 && ie[1] >= 0 && ie[1] <= 32) {
			uint8_t *p = ssid;
			uint8_t i, l, r = sizeof(ssid);
#ifdef RTCONFIG_UTF8_SSID
			uint8_t tmp[4];
#endif

			len = ie[1];
			data = ie + 2;

			/* reference from print_ssid() */
			for (i = 0; i < len; i++) {
				if (isprint(data[i]) && data[i] != ' ' && data[i] != '\\') {
#ifdef RTCONFIG_UTF8_SSID
					snprintf(tmp, sizeof(tmp), "%c", data[i]);
					l = char_to_ascii_safe_with_utf8((char *)p, (char *)tmp, strlen(tmp) * 3 + 1);
#else
					l = snprintf(p, r, "%c", data[i]);
#endif
				}
				else if (data[i] == ' ' && (i != 0 && i != len -1)) {
#ifdef RTCONFIG_UTF8_SSID
					snprintf(tmp, sizeof(tmp), " ");
					l = char_to_ascii_safe_with_utf8((char *)p, (char *)tmp, strlen(tmp) * 3 + 1);
#else
					l = snprintf(p, r, " ");
#endif
				}
				else {
#ifdef RTCONFIG_UTF8_SSID
					l = snprintf(p, r, "%%%.2x", data[i]);
#else
					l = snprintf(p, r, "\\x%.2x", data[i]);
#endif
				}

				p += l;
				r -= l;
			}

			break;
		}

		ielen -= ie[1] + 2;
		ie += ie[1] + 2;
	}

	/* convert to GUI format */
	fprintf(fp, "\"%s\",", ssid);
}

/*
 * Get the suite type of Pairwise Ciphers and AKM (Authentication Key
 * Management) from the data
 * @data:		IE data
 * @len:		length of IE data
 * @pc_mask:		suite type
 * @akm_mask:		suite type
 */
static void get_cipher_and_auth(uint8_t *data, uint8_t len,
				uint64_t *pc_mask, uint64_t *akm_mask)
{
	__u16 count;
	int i;

	/* reference from print_rsn() and print_wifi_wpa() */
	data += 2;
	len -= 2;

	if (len < 4)
		return;

	data += 4;
	len -= 4;

	if (len < 2)
		return;

	count = data[0] | (data[1] << 8);
	if (2 + (count * 4) > len)
		return;

	/* parse "* Pairwise ciphers:" */
	for (i = 0; i < count && i < 3; i++) {
		uint8_t *p = data + 2 + (i * 4);
		*pc_mask |= 1U << p[3];
	}

	data += 2 + (count * 4);
	len -= 2 + (count * 4);

	if (len < 2)
		return;

	count = data[0] | (data[1] << 8);
	if (2 + (count * 4) > len)
		return;

	/* parse "* Authentication suites:" */
	for (i = 0; i < count; i++) {
		uint8_t *p = data + 2 + (i * 4);
		*akm_mask |= 1U << p[3];
	}
}

/*
 * Parse RSN IE
 */
static int fprint_rsn(FILE *fp, unsigned char *ie, int ielen)
{
	uint8_t enc[13] = "", auth[30] = "";
	uint64_t pc_mask = 0, akm_mask = 0;
	uint8_t found = 0;

	if (ie == NULL || ielen < 0)
		return 0;

	/* reference from print_ies() and print_ie() */
	while (ielen >= 2 && ielen - 2 >= ie[1]) {
		/* RSN: id = 48 & length = 2~255 */
		if (ie[0] == 48 && ie[1] >= 2 && ie[1] <= 255) {
			found = 1;
			get_cipher_and_auth(ie + 2, ie[1], &pc_mask, &akm_mask);

			break;
		}

		ielen -= ie[1] + 2;
		ie += ie[1] + 2;
	}

	/* convert to GUI format */
	if (akm_mask & AUTH_IEEE_802P1X) {
		if ((pc_mask & CIPHER_TKIP) && (pc_mask & CIPHER_CCMP)) {
			strlcpy(auth, "WPA2-Enterprise", sizeof(auth));
			strlcpy(enc, "TKIP+AES", sizeof(enc));
		}
		else if (pc_mask & CIPHER_CCMP) {
			strlcpy(auth, "WPA2-Enterprise", sizeof(auth));
			strlcpy(enc, "AES", sizeof(enc));
		}
		else if (pc_mask & CIPHER_TKIP) {
			strlcpy(auth, "WPA-Enterprise", sizeof(auth));
			strlcpy(enc, "TKIP", sizeof(enc));
		}
	}
	else if (akm_mask & AUTH_SAE) {
		strlcpy(auth, "WPA3-Personal", sizeof(auth));
		if (pc_mask & CIPHER_GCMP_256)
			strlcpy(enc, "AES+GCMP256", sizeof(enc));
		else
			strlcpy(enc, "AES", sizeof(enc));
	}
	else if (akm_mask & AUTH_PSK) {
		strlcpy(auth, "WPA2-Personal", sizeof(auth));
		if ((pc_mask & CIPHER_TKIP) && (pc_mask & CIPHER_CCMP))
			strlcpy(enc, "TKIP+AES", sizeof(enc));
		else if (pc_mask & CIPHER_CCMP)
			strlcpy(enc, "AES", sizeof(enc));
		else if (pc_mask & CIPHER_TKIP)
			strlcpy(enc, "TKIP", sizeof(enc));
	}

	if (found) {
		fprintf(fp, "\"%s\",", auth);
		fprintf(fp, "\"%s\",", enc);
	}

	return found;
}

/*
 * Parse WPA IE
 */
static int fprint_wifi_wpa(FILE *fp, unsigned char *ie, int ielen)
{
	static unsigned char ms_oui[3] = { 0x00, 0x50, 0xf2 };
	static unsigned char wfa_oui[3] = { 0x50, 0x6f, 0x9a };
	uint8_t enc[13] = "", auth[30] = "";
	uint64_t pc_mask = 0, akm_mask = 0;
	uint8_t *data;
	uint8_t len, found = 0;

	if (ie == NULL || ielen < 0)
		return 0;

	/* reference from print_ies() and print_vendor() */
	while (ielen >= 2 && ielen - 2 >= ie[1]) {
		/* vendor IE: id = 221 */
		if (ie[0] == 221) {
			len = ie[1];
			data = ie + 2;

			/* reference from print_ie() and print_wifi_wpa() */
			if (len >= 4 && (memcmp(data, ms_oui, 3) == 0
				      || memcmp(data, wfa_oui, 3) == 0)
			) {
				/* WPA: version = 1 & length = 2~255 */
				if (data[3] == 1 && (len - 4) >= 2 && (len - 4) <= 255) {
					found = 1;
					get_cipher_and_auth(data + 4, len - 4, &pc_mask, &akm_mask);

					break;
				}
			}
		}

		ielen -= ie[1] + 2;
		ie += ie[1] + 2;
	}

	/* convert to GUI format */
	if (akm_mask & AUTH_IEEE_802P1X) {
		strlcpy(auth, "WPA-Enterprise", sizeof(auth));
		strlcpy(enc, "TKIP", sizeof(enc));
	}
	else if (akm_mask & AUTH_PSK) {
		strlcpy(auth, "WPA-Personal", sizeof(auth));
		strlcpy(enc, "TKIP", sizeof(enc));
	}

	if (found) {
		fprintf(fp, "\"%s\",", auth);
		fprintf(fp, "\"%s\",", enc);
	}

	return found;
}

/*
 * Parse IE (EHT, HE, VHT, HT, etc.)
 */
static int fprint_wmode(FILE *fp, unsigned char *ie, int ielen, char *dev)
{
	uint8_t wmode[8] = "";
	uint64_t wmode_mask = 0;
	uint8_t *data;
	uint8_t len;

	if (ie == NULL || ielen < 0)
		return 0;

	/* reference from print_ies(), print_ie(), and print_extension() */
	while (ielen >= 2 && ielen - 2 >= ie[1]) {
		/* HT Capabilities: id = 45 & length = 26 */
		if (ie[0] == 45 && ie[1] == 26) {
			wmode_mask |= WMODE_HT;
		}
		/* VHT Capabilities: id = 191 & length = 12~255 */
		else if (ie[0] == 191 && ie[1] >= 12 && ie[1] <= 255) {
			wmode_mask |= WMODE_VHT;
		}
		/* extension IE: id = 255 */
		else if (ie[0] == 255) {
			len = ie[1];
			data = ie + 2;

			/* HE Capabilities: number = 35 & length = 21~54 */
			if (data[0] == 35 && len >= 21 && len <= 54) {
				wmode_mask |= WMODE_HE;
			}
			/* EHT Capabilities: number = 108 & length = ??? */
			else if (data[0] == 108) {
				wmode_mask |= WMODE_EHT;
			}
		}

		ielen -= ie[1] + 2;
		ie += ie[1] + 2;
	}

	/* convert to GUI format */
	if (wmode_mask & WMODE_EHT)
		strlcpy(wmode, "be", sizeof(wmode));
	else if (wmode_mask & WMODE_HE)
		strlcpy(wmode, "ax", sizeof(wmode));
	else if ((wmode_mask & WMODE_VHT) && (strcmp(dev, WIF_5G) == 0))
		strlcpy(wmode, "ac", sizeof(wmode));
	else if (wmode_mask & WMODE_HT) {
		if (strcmp(dev, WIF_2G) == 0)
			strlcpy(wmode, "bgn", sizeof(wmode));
		else if (strcmp(dev, WIF_5G) == 0)
			strlcpy(wmode, "an", sizeof(wmode));
	}
	else {
		if (strcmp(dev, WIF_2G) == 0)
			strlcpy(wmode, "bg", sizeof(wmode));
		else if (strcmp(dev, WIF_5G) == 0)
			strlcpy(wmode, "a", sizeof(wmode));
	}
	fprintf(fp, "\"%s\",", wmode);

	return 1;
}

/*
 * Parse extension IE (Multi-Link)
 */
static int fprint_mlo(FILE *fp, unsigned char *ie, int ielen)
{
	//char mld_addr[20];
	uint8_t *data;
	uint8_t len, found = 0;

	if (ie == NULL || ielen < 0)
		return 0;

	/* reference from print_ies() and print_extension() */
	while (ielen >= 2 && ielen - 2 >= ie[1]) {
		/* extension IE: id = 255 */
		if (ie[0] == 255) {
			len = ie[1];
			data = ie + 2;

			/* Multi-Link: number = 107 & length = ??? */
			if (data[0] == 107) {
				found = 1;
				//mac_addr_n2a(mld_addr, data + 4);
				//fprintf(stderr, "%s: MLD Address: %s\n", __func__, mld_addr);

				break;
			}
		}

		ielen -= ie[1] + 2;
		ie += ie[1] + 2;
	}

	if (found)
		fprintf(fp, ",\"mlo\"");
	else
		fprintf(fp, ",\"not\"");

	return found;
}

/*
 * Convert signal strength from dBm to percentage
 */
static int singal_dBm2percent(int P_dBm)
{
	int p;

	/*
	 * P_dBm is parsed from IE. This value is magnified 100 times.
	 *
	 * reference from website
	 * https://www.intuitibits.com/2016/03/23/dbm-to-percent-conversion/
	 * P_dBm_max = -20 dBm
	 * P_dBm_min = -85 dBm
	 *                          (P_dBm_max - P_dBm)
	 * percent = 100 x (1  -  -----------------------)
	 *                        (P_dBm_max - P_dBm_min)
	 */
	if (P_dBm >= -2000)
		p = 100;
	else if (P_dBm <= -8500)
		p = 1;
	else
		p = 100 - (((-2000 - P_dBm) * 100) / 6500);

	return p;
}

/*
 * Called by the kernel if the message (scan results) is valid. Called for each SSID
 */
static int callback_dump(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *bss[NL80211_BSS_MAX + 1];
	char mac_addr[20], dev[20];
	static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
		[NL80211_BSS_TSF] = { .type = NLA_U64 },
		[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_BSS_BSSID] = { },
		[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
		[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
		[NL80211_BSS_INFORMATION_ELEMENTS] = { },
		[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
		[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
		[NL80211_BSS_STATUS] = { .type = NLA_U32 },
		[NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
		[NL80211_BSS_BEACON_IES] = { },
	};
	FILE *fp = (FILE *)arg;

	/* parse and error check */
	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[NL80211_ATTR_BSS]) {
		fprintf(stderr, "bss info missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], bss_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	if (!bss[NL80211_BSS_BSSID])
		return NL_SKIP;

	/* BAND */
	if (tb[NL80211_ATTR_IFINDEX]) {
		if_indextoname(nla_get_u32(tb[NL80211_ATTR_IFINDEX]), dev);
		if (strcmp(dev, WIF_2G) == 0)
			fprintf(fp, "\"2G\",");
		else if (strcmp(dev, WIF_5G) == 0)
			fprintf(fp, "\"5G\",");
#if defined(RTCONFIG_HAS_5G_2) && defined(RTCONFIG_WIFI7_NO_6G)
		else if (strcmp(dev, WIF_5G2) == 0)
			fprintf(fp, "\"5G\",");
#else
		else if (strcmp(dev, WIF_6G) == 0)
			fprintf(fp, "\"6G\",");
#endif
	}
	/* SSID */
	if (bss[NL80211_BSS_INFORMATION_ELEMENTS])
		fprint_ssid(fp, nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]), nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]));
	else
		fprintf(fp, "\"\",");
	/* Channel */
	fprintf(fp, "\"%d\",", ieee80211_mhz2ieee(nla_get_u32(bss[NL80211_BSS_FREQUENCY])));
	/* Authentication/Encryption */
	if (bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
		if (fprint_rsn(fp, nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]), nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]))
		 || fprint_wifi_wpa(fp, nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]), nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS])))
			;
		else
			fprintf(fp, "\"Open System\",\"NONE\",");
	}
	else
		fprintf(fp, "\"\",\"\",");
	/* Signal */
	if (bss[NL80211_BSS_SIGNAL_MBM]) {
		int P_dBm = nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM]);
		int percent = singal_dBm2percent(P_dBm);

		fprintf(fp, "\"%d\",", percent);
	}
	else
		fprintf(fp, "\"0\",");
	/* MAC */
	mac_addr_n2a(mac_addr, nla_data(bss[NL80211_BSS_BSSID]));
	fprintf(fp, "\"%s\",", mac_addr);
	/* Wireless Mode */
	if (bss[NL80211_BSS_INFORMATION_ELEMENTS])
		fprint_wmode(fp, nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]), nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]), dev);
	else
		fprintf(fp, "\"\",");
	/* connected/disconnect */
	fprintf(fp, "\"0\"");
	/* MLO */
	if (bss[NL80211_BSS_INFORMATION_ELEMENTS])
		fprint_mlo(fp, nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]), nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]));
	fprintf(fp, "\n");

	return NL_SKIP;
}

/*
 * iw dev <ifname> scan dump
 * Scan results will be dumped to /tmp/apscan_info.txt for GUI
 */
int do_scan_dump(struct nl80211_state *state, char *ifname)
{
	struct nl_msg *msg;
	struct nl_cb *cb;
	int err;
	signed long long devidx = if_nametoindex(ifname);
	FILE *fp;

	/* allocate the messages and callback handler. */
	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message for msg\n");
		return -ENOMEM;
	}
	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		nlmsg_free(msg);
		return -ENOMEM;
	}

	if (!(fp = fopen("/tmp/apscan_info.txt", "a+")))
		return -EPERM;

	/* setup the messages */
	genlmsg_put(msg, 0, 0, state->nl80211_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
	nla_put_u32(msg, NL80211_ATTR_IFINDEX, devidx);

	err = nl_send_auto_complete(state->nl_sock, msg);
	if (err < 0)
		goto out;

	/* callback handler. */
	err = 1;
	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, callback_dump, fp);

	while (err > 0)
		nl_recvmsgs(state->nl_sock, cb);

out:
	nlmsg_free(msg);
	nl_cb_put(cb);
	fclose(fp);

	return 0;
}

/*
 * Called by the kernel when mt_wifi7 asus_cfg80211_subcmd_handler() is done.
 */
static int callback_asuscmd(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct nlattr *vndr_tb[MTK_NL80211_VENDOR_ATTR_VENDOR_SHOW_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	int err = 0;

	err = nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			  genlmsg_attrlen(gnlh, 0), NULL);
	if (err < 0)
		return err;

	if (tb[NL80211_ATTR_VENDOR_DATA]) {
		err = nla_parse_nested(vndr_tb, MTK_NL80211_VENDOR_ATTR_VENDOR_SHOW_ATTR_MAX,
			tb[NL80211_ATTR_VENDOR_DATA], NULL);
		if (err < 0)
			return err;

		if (vndr_tb[ASUS_NL80211_VENDOR_ATTR_VENDOR_SUBCMD_RSP]) {
			int l = nla_len(vndr_tb[ASUS_NL80211_VENDOR_ATTR_VENDOR_SUBCMD_RSP]);
			char *d = nla_data(vndr_tb[ASUS_NL80211_VENDOR_ATTR_VENDOR_SUBCMD_RSP]);

			memcpy(arg + asuscmd_reply_len, d, l);
			asuscmd_reply_len += l;
		}
	}
	else
		fprintf(stderr, "no any asuscmd content from driver\n");

	return 0;
}

/*
 * asuscmd handler
 */
int do_asuscmd(struct nl80211_state *state, struct nl80211_cmd *nc)
{
	struct nl_msg *msg;
	struct nl_cb *cb;
	int err;
	signed long long devidx = if_nametoindex(nc->ifname);
	void *asuscmd;

	asuscmd_reply_len = 0;

	/* allocate the messages and callback handler. */
	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message for msg\n");
		return -ENOMEM;
	}
	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		nlmsg_free(msg);
		return -ENOMEM;
	}

	/* setup the messages */
	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, state->nl80211_id, 0, 0, NL80211_CMD_VENDOR, 0);
	nla_put_u32(msg, NL80211_ATTR_IFINDEX, devidx);
	nla_put_u32(msg, NL80211_ATTR_VENDOR_ID, MTK_NL80211_VENDOR_ID);
	nla_put_u32(msg, NL80211_ATTR_VENDOR_SUBCMD, ASUS_NL80211_VENDOR_SUBCMD);
	/* asus sub-command & buffer size */
	asuscmd = nla_nest_start(msg, NL80211_ATTR_VENDOR_DATA);
	nla_put_u8(msg, ASUS_NL80211_VENDOR_ATTR_VENDOR_SUBCMD_NO, nc->data.flags);
	nla_put_u16(msg, ASUS_NL80211_VENDOR_ATTR_VENDOR_SUBCMD_BUF_SIZE, nc->data.length);
	nla_nest_end(msg, asuscmd);

	err = nl_send_auto_complete(state->nl_sock, msg);
	if (err < 0)
		goto out;

	/* callback handler. */
	err = 1;
	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, callback_asuscmd, nc->data.pointer);

	while (err > 0)
		nl_recvmsgs(state->nl_sock, cb);

out:
	nlmsg_free(msg);
	nl_cb_put(cb);

	return 0;
}

/*
 * nl80211 user space tool
 */
int do_nl80211_cmd(struct nl80211_cmd *nc)
{
	struct nl80211_state nlstate;
	char *ifname = NULL, *cmd = NULL, *subcmd = NULL;
	int err;

	if (strcmp(nc->ifname, "") == 0
	 || strcmp(nc->cmd, "") == 0)
		return 1;

	ifname = nc->ifname;
	cmd = nc->cmd;
	subcmd = nc->subcmd;

	err = nl80211_init(&nlstate);

	if (strcmp(cmd, "scan") == 0) {
		if (subcmd && strcmp(subcmd, "trigger") == 0)
			err = do_scan_trigger(&nlstate, ifname);
		else if (subcmd && strcmp(subcmd, "dump") == 0)
			err = do_scan_dump(&nlstate, ifname);
		else {
			err = do_scan_trigger(&nlstate, ifname);
			err = do_scan_dump(&nlstate, ifname);
		}
	}
	else if (strcmp(cmd, "asuscmd") == 0) {
		err = do_asuscmd(&nlstate, nc);
	}
	else
		fprintf(stderr, "%s: unknown cmd!\n", __func__);

	nl80211_cleanup(&nlstate);

	return err;
}

/* get no_bcn value of raX */
int get_no_bcn(char *ifname)
{
	struct nl80211_cmd nc;
	char data[4];

	memset(&nc, 0x0, sizeof(struct nl80211_cmd));
	strlcpy(nc.ifname, ifname, IFNAMSIZ);
	strlcpy(nc.cmd, "asuscmd", 16);

	nc.data.pointer = (void *)data;
	nc.data.length = (u16)sizeof(data);
	nc.data.flags = ASUS_NL80211_VENDOR_SUBCMD_GET_NO_BCN;

	do_nl80211_cmd(&nc);

	return atoi(data);
}

/* turn on/off raX broadcast beacon */
void set_no_bcn(char *ifname, int value)
{
	char str[2];

	snprintf(str, sizeof(str), "%d", value ? 1 : 0);
	eval(IWPRIV, ifname, "set", "no_bcn", str);
	if (value)
		eval("hostapd_cli", "-i", ifname, "disassociate", "ff:ff:ff:ff:ff:ff");
}

/* get MLO information of apcliX */
void get_apcli_mlo_info(char *ifname, struct apcli_mlo_info *ami)
{
	struct nl80211_cmd nc;

	memset(&nc, 0x0, sizeof(struct nl80211_cmd));
	strlcpy(nc.ifname, ifname, IFNAMSIZ);
	strlcpy(nc.cmd, "asuscmd", 16);

	memset(ami, 0x0, sizeof(struct apcli_mlo_info));
	nc.data.pointer = (void *)ami;
	nc.data.length = (u16)sizeof(struct apcli_mlo_info);
	nc.data.flags = ASUS_NL80211_VENDOR_SUBCMD_GET_APCLI_MLO_INFO;

	do_nl80211_cmd(&nc);
}

/*
 * get DFS CAC status of raX
 * return 1: during CAC
 *        0: done
 */
int get_dfs_cac_status(char *ifname)
{
	struct nl80211_cmd nc;
	char data[4];

	memset(&nc, 0x0, sizeof(struct nl80211_cmd));
	strlcpy(nc.ifname, ifname, IFNAMSIZ);
	strlcpy(nc.cmd, "asuscmd", 16);

	nc.data.pointer = (void *)data;
	nc.data.length = (u16)sizeof(data);
	nc.data.flags = ASUS_NL80211_VENDOR_SUBCMD_GET_DFS_CAC_STATUS;

	do_nl80211_cmd(&nc);

	return atoi(data);
}
