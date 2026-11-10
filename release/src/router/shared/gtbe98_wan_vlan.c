/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stdio.h>
#include <string.h>
#include "gtbe98_wan_vlan.h"

/* Bounded decimal parsing: no signs, whitespace, suffixes or wraparound. */
static int vlan_number(const char *text, size_t maxlen, unsigned int max,
	unsigned int *value)
{
	unsigned int n = 0;
	size_t i;

	if (!text || !*text)
		return 0;
	for (i = 0; text[i]; ++i) {
		if (i >= maxlen || text[i] < '0' || text[i] > '9')
			return 0;
		n = n * 10 + (text[i] - '0');
		if (n > max)
			return 0;
	}
	*value = n;
	return 1;
}

enum gtbe98_vlan_result gtbe98_wan_vlan_plan(
	const struct gtbe98_vlan_request *r, struct gtbe98_vlan_plan *plan)
{
	struct gtbe98_vlan_plan next;
	unsigned int vid, priority;
	size_t i;

	if (!plan)
		return GTBE98_VLAN_INVALID;
	/* An error must never leave a previously valid plan available to execute. */
	memset(plan, 0, sizeof(*plan));
	if (!r || !r->dot1q)
		return GTBE98_VLAN_INVALID;
	if (!strcmp(r->dot1q, "0"))
		return GTBE98_VLAN_DISABLED;
	if (strcmp(r->dot1q, "1"))
		return GTBE98_VLAN_INVALID;
	if (!vlan_number(r->vid, 4, 4093, &vid) || vid < 3 ||
	    !vlan_number(r->dot1p, 1, 7, &priority))
		return GTBE98_VLAN_INVALID;
	/* VID 0/1/2 and 4094/4095 are reserved, independently of LAN settings. */
	if (r->gtbe98_rtl8372 != 1 || r->router_mode != 1 ||
	    r->wan_unit != 0 || r->manual_wan != 1 || r->single_wan != 1 ||
	    (r->extwan != 0 && r->extwan != 1) || r->wan_aggregation != 0)
		return GTBE98_VLAN_TOPOLOGY;
	/* Apply the same policy on 10G so that changing WAN port is unambiguous. */
	if (!r->iptv_profile || strcmp(r->iptv_profile, "none") ||
	    !r->stb_port || strcmp(r->stb_port, "0"))
		return GTBE98_VLAN_IPTV_CONFLICT;
	if (r->lan_vlan_state != 0 || (r->reserved_count && !r->reserved_vids))
		return GTBE98_VLAN_LAN_CONFLICT;
	for (i = 0; i < r->reserved_count; ++i) {
		if (r->reserved_vids[i] >= GTBE98_VLAN_COUNT || r->reserved_vids[i] == vid)
			return GTBE98_VLAN_LAN_CONFLICT;
	}

	memset(&next, 0, sizeof(next));
	next.vid = vid;
	next.priority = priority;
	next.program_switch = r->extwan;
	snprintf(next.parent, sizeof(next.parent), "eth%d", r->extwan);
	snprintf(next.ifname, sizeof(next.ifname), "vlan%u", vid);
	if (r->extwan) {
		next.isp_members = GTBE98_VLAN_CPU | GTBE98_VLAN_WAN;
		next.lan_members = GTBE98_VLAN_CPU | GTBE98_VLAN_LAN;
		/* Command 39 adds CPU automatically; VID 0 also makes CPU untagged.
		 * ISP must be created first, then LAN to restore CPU PVID to zero.
		 * Do not substitute command 390: it forces shared learning (FID 0).
		 */
		next.isp_create_mask = GTBE98_VLAN_WAN;
		next.lan_create_mask = (GTBE98_VLAN_LAN << 16) | GTBE98_VLAN_LAN;
	}
	*plan = next;
	return GTBE98_VLAN_OK;
}

enum gtbe98_vlan_result gtbe98_wan_vlan_verify(
	const struct gtbe98_vlan_plan *plan,
	const struct gtbe98_vlan_observation *o)
{
	const struct gtbe98_vlan_entry *lan, *isp;
	unsigned int i;

	/* This verifier only covers the external switch; never infer that a 10G
	 * plan or a correct switch table proves the Linux interface/bridge state.
	 */
	if (!plan || plan->program_switch != 1 || plan->vid < 3 || plan->vid > 4093 ||
	    plan->priority > 7 || memcmp(plan->parent, "eth1\0\0\0", 8) ||
	    plan->isp_members != (GTBE98_VLAN_CPU | GTBE98_VLAN_WAN) ||
	    plan->lan_members != (GTBE98_VLAN_CPU | GTBE98_VLAN_LAN))
		return GTBE98_VLAN_INVALID;
	if (!o || o->complete != 1 || !o->entries || o->count != GTBE98_VLAN_COUNT)
		return GTBE98_VLAN_UNVERIFIED;
	lan = &o->entries[0];
	isp = &o->entries[plan->vid];
	if (lan->members != plan->lan_members || lan->untag != plan->lan_members ||
	    lan->fid != 0 || isp->members != plan->isp_members || isp->untag != 0 ||
	    isp->fid == 0 || isp->fid > 15 || lan->table_flags || isp->table_flags)
		return GTBE98_VLAN_MISMATCH;
	for (i = 1; i < GTBE98_VLAN_COUNT; ++i) {
		/* Exclusive ownership requires a clean table, not just WAN exclusion.
		 * This also excludes stale VLANs sharing the ISP learning domain.
		 */
		if (i != plan->vid && (o->entries[i].members || o->entries[i].untag))
			return GTBE98_VLAN_MISMATCH;
	}
	if (o->ingress_filter != GTBE98_VLAN_PORTS ||
	    o->tagged_only != GTBE98_VLAN_WAN ||
	    o->untagged_only != GTBE98_VLAN_LAN || o->admit_all != GTBE98_VLAN_CPU ||
	    o->egress_filter != 1 || o->nondefault_tag_mode)
		return GTBE98_VLAN_MISMATCH;
	for (i = 0; i < 8; ++i) {
		if ((GTBE98_VLAN_LAN | GTBE98_VLAN_CPU) & (1U << i)) {
			if (o->pvid[i] != 0)
				return GTBE98_VLAN_MISMATCH;
		} else if (GTBE98_VLAN_WAN & (1U << i)) {
			if (o->pvid[i] != plan->vid)
				return GTBE98_VLAN_MISMATCH;
		}
	}
	return GTBE98_VLAN_OK;
}

const char *gtbe98_wan_vlan_error(enum gtbe98_vlan_result result)
{
	switch (result) {
	case GTBE98_VLAN_OK: return "Valid WAN VLAN plan or matching switch observation";
	case GTBE98_VLAN_DISABLED: return "Explicit WAN VLAN disabled";
	case GTBE98_VLAN_INVALID: return "Invalid WAN VLAN parameters";
	case GTBE98_VLAN_TOPOLOGY: return "Requires GT-BE98 RTL8372, router mode and one manual WAN without aggregation";
	case GTBE98_VLAN_IPTV_CONFLICT: return "Disable the IPTV profile and STB port before enabling explicit WAN VLAN";
	case GTBE98_VLAN_LAN_CONFLICT: return "LAN VLAN configuration is active, unknown or reserves the requested VID";
	case GTBE98_VLAN_UNVERIFIED: return "Complete checked hardware readback is required";
	case GTBE98_VLAN_MISMATCH: return "Switch state does not match the isolated WAN VLAN plan";
	case GTBE98_VLAN_IO_ERROR: return "WAN VLAN programming or hardware readback failed";
	case GTBE98_VLAN_QUIESCE_ERROR: return "Physical WAN could not be confirmed down";
	default: return "Unknown WAN VLAN result";
	}
}

enum gtbe98_vlan_result gtbe98_wan_vlan_stage(
	const struct gtbe98_vlan_plan *plan, const struct gtbe98_vlan_ops *ops, void *ctx)
{
	struct gtbe98_vlan_observation observed;
	enum gtbe98_vlan_result ret;
	unsigned int i;
	struct { unsigned int cmd, arg; } sequence[] = {
		{ 38, 0 },       /* Initialize; physical WAN must already be down. */
		{ 36, 1 }, { 3901, 0 }, /* Remove default VLAN 1 (includes WAN). */
		{ 36, 0 }, { 37, 0 }, { 39, GTBE98_VLAN_WAN },
		{ 36, 0 }, { 37, 0 },
		{ 39, (GTBE98_VLAN_LAN << 16) | GTBE98_VLAN_LAN },
		{ 380, GTBE98_VLAN_PORTS },
		{ 3951, GTBE98_VLAN_CPU },
		{ 3952, GTBE98_VLAN_WAN },
		{ 3953, GTBE98_VLAN_LAN }
	};

	if (!ops || !ops->quiesce || !ops->command || !ops->readback ||
	    gtbe98_wan_vlan_verify(plan, NULL) != GTBE98_VLAN_UNVERIFIED ||
	    plan->isp_create_mask != GTBE98_VLAN_WAN ||
	    plan->lan_create_mask != ((GTBE98_VLAN_LAN << 16) | GTBE98_VLAN_LAN))
		return GTBE98_VLAN_INVALID;
	if (ops->quiesce(ctx))
		return GTBE98_VLAN_QUIESCE_ERROR;
	sequence[3].arg = plan->vid;
	for (i = 0; i < sizeof(sequence) / sizeof(sequence[0]); ++i) {
		if (ops->command(ctx, sequence[i].cmd, sequence[i].arg)) {
			ret = GTBE98_VLAN_IO_ERROR;
			goto fail;
		}
	}
	memset(&observed, 0, sizeof(observed));
	if (ops->readback(ctx, &observed)) {
		ret = GTBE98_VLAN_IO_ERROR;
		goto fail;
	}
	ret = gtbe98_wan_vlan_verify(plan, &observed);
	if (ret == GTBE98_VLAN_OK)
		return ret;
fail:
	/* Never reset to all-LAN on error: a connected ISP could enter the LAN.
	 * Keep the port down; a failed containment attempt has its own status.
	 */
	if (ops->quiesce(ctx))
		return GTBE98_VLAN_QUIESCE_ERROR;
	return ret;
}
