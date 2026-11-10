/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gtbe98_wan_vlan.h"

static struct gtbe98_vlan_request request(void)
{
	struct gtbe98_vlan_request r;
	memset(&r, 0, sizeof(r));
	r.dot1q = "1";
	r.vid = "835";
	r.dot1p = "0";
	r.iptv_profile = "none";
	r.stb_port = "0";
	r.gtbe98_rtl8372 = 1;
	r.router_mode = 1;
	r.manual_wan = 1;
	r.single_wan = 1;
	r.extwan = 1;
	return r;
}

static void rejected(struct gtbe98_vlan_request *r, enum gtbe98_vlan_result result)
{
	struct gtbe98_vlan_plan plan, empty;
	memset(&plan, 0xff, sizeof(plan));
	memset(&empty, 0, sizeof(empty));
	assert(gtbe98_wan_vlan_plan(r, &plan) == result);
	assert(!memcmp(&plan, &empty, sizeof(plan)));
}

static void test_input(void)
{
	const char *bad_vid[] = { NULL, "", "0", "1", "2", "4094", "4095", "4096",
		"-835", "+835", " 835", "835 ", "835x", "0x343", "8.35", "08350",
		"4294968131", "999999999999999999999", "835\n" };
	const char *bad_prio[] = { NULL, "", "-1", "8", "00", "+0", " 0", "0x0", "0x", "0\n" };
	const char *bad_enable[] = { NULL, "", "2", "01", "true", "-1" };
	struct gtbe98_vlan_request r = request();
	struct gtbe98_vlan_plan plan;
	char number[16];
	unsigned int reserved[] = { 10, 501, 502, 503, 504 };
	size_t i;
	unsigned int n;

	for (i = 0; i < sizeof(bad_vid) / sizeof(bad_vid[0]); ++i) {
		r.vid = bad_vid[i];
		rejected(&r, GTBE98_VLAN_INVALID);
	}
	r = request();
	for (i = 0; i < sizeof(bad_prio) / sizeof(bad_prio[0]); ++i) {
		r.dot1p = bad_prio[i];
		rejected(&r, GTBE98_VLAN_INVALID);
	}
	r = request();
	for (i = 0; i < sizeof(bad_enable) / sizeof(bad_enable[0]); ++i) {
		r.dot1q = bad_enable[i];
		rejected(&r, GTBE98_VLAN_INVALID);
	}
	r = request();
	for (n = 3; n <= 4093; ++n) {
		snprintf(number, sizeof(number), "%u", n);
		r.vid = number;
		assert(gtbe98_wan_vlan_plan(&r, &plan) == GTBE98_VLAN_OK);
		assert(plan.vid == n);
	}
	r = request();
	for (n = 0; n < 8; ++n) {
		snprintf(number, sizeof(number), "%u", n);
		r.dot1p = number;
		assert(gtbe98_wan_vlan_plan(&r, &plan) == GTBE98_VLAN_OK);
		assert(plan.priority == n);
	}
	r = request();
	r.reserved_vids = reserved;
	r.reserved_count = sizeof(reserved) / sizeof(reserved[0]);
	assert(gtbe98_wan_vlan_plan(&r, &plan) == GTBE98_VLAN_OK);
	for (i = 0; i < r.reserved_count; ++i) {
		snprintf(number, sizeof(number), "%u", reserved[i]);
		r.vid = number;
		rejected(&r, GTBE98_VLAN_LAN_CONFLICT);
	}
	r.vid = "835";
	r.reserved_vids = NULL;
	rejected(&r, GTBE98_VLAN_LAN_CONFLICT);
	assert(gtbe98_wan_vlan_plan(NULL, &plan) == GTBE98_VLAN_INVALID);
	assert(gtbe98_wan_vlan_plan(&r, NULL) == GTBE98_VLAN_INVALID);

	/* Disabling must remain possible with stale or incompatible settings. */
	r.dot1q = "0";
	r.vid = NULL;
	r.dot1p = NULL;
	r.iptv_profile = "manual";
	r.manual_wan = 0;
	rejected(&r, GTBE98_VLAN_DISABLED);
}

static void test_policy(void)
{
	struct gtbe98_vlan_request r;
	int extwan;
#define REJECT_FIELD(field, value, error) do { \
	r = request(); r.extwan = extwan; r.field = value; rejected(&r, error); \
} while (0)
	for (extwan = 0; extwan < 2; ++extwan) {
		REJECT_FIELD(gtbe98_rtl8372, 0, GTBE98_VLAN_TOPOLOGY);
		REJECT_FIELD(router_mode, 0, GTBE98_VLAN_TOPOLOGY);
		REJECT_FIELD(wan_unit, 1, GTBE98_VLAN_TOPOLOGY);
		REJECT_FIELD(manual_wan, 0, GTBE98_VLAN_TOPOLOGY);
		REJECT_FIELD(single_wan, 0, GTBE98_VLAN_TOPOLOGY);
		REJECT_FIELD(extwan, 2, GTBE98_VLAN_TOPOLOGY);
		REJECT_FIELD(wan_aggregation, 1, GTBE98_VLAN_TOPOLOGY);
		REJECT_FIELD(lan_vlan_state, 1, GTBE98_VLAN_LAN_CONFLICT);
		REJECT_FIELD(lan_vlan_state, -1, GTBE98_VLAN_LAN_CONFLICT);
		REJECT_FIELD(iptv_profile, "manual", GTBE98_VLAN_IPTV_CONFLICT);
		REJECT_FIELD(iptv_profile, "unifi_home", GTBE98_VLAN_IPTV_CONFLICT);
		REJECT_FIELD(iptv_profile, "", GTBE98_VLAN_IPTV_CONFLICT);
		REJECT_FIELD(iptv_profile, NULL, GTBE98_VLAN_IPTV_CONFLICT);
		REJECT_FIELD(stb_port, "1", GTBE98_VLAN_IPTV_CONFLICT);
		REJECT_FIELD(stb_port, NULL, GTBE98_VLAN_IPTV_CONFLICT);
	}
#undef REJECT_FIELD
}

static void test_plan(void)
{
	struct gtbe98_vlan_request r = request();
	struct gtbe98_vlan_plan plan;
	unsigned int members, untag;

	assert(gtbe98_wan_vlan_plan(&r, &plan) == GTBE98_VLAN_OK);
	assert(!strcmp(plan.parent, "eth1"));
	assert(!strcmp(plan.ifname, "vlan835"));
	assert(plan.program_switch == 1);
	/* Independently apply the driver's mask transformation. */
	members = (plan.isp_create_mask & 0x3f7) | 8;
	untag = plan.isp_create_mask >> 16;
	assert(members == 0x28 && untag == 0);
	members = (plan.lan_create_mask & 0x3f7) | 8;
	untag = (plan.lan_create_mask >> 16) | 8;
	assert(members == 0xd8 && untag == 0xd8);
	assert((plan.isp_members & plan.lan_members) == GTBE98_VLAN_CPU);
	r.extwan = 0;
	assert(gtbe98_wan_vlan_plan(&r, &plan) == GTBE98_VLAN_OK);
	assert(!strcmp(plan.parent, "eth0"));
	assert(!strcmp(plan.ifname, "vlan835"));
	assert(!plan.program_switch && !plan.isp_create_mask && !plan.lan_create_mask);
	assert(gtbe98_wan_vlan_verify(&plan, NULL) == GTBE98_VLAN_INVALID);
}

static void test_readback(void)
{
	static struct gtbe98_vlan_entry entries[GTBE98_VLAN_COUNT];
	struct gtbe98_vlan_request r = request();
	struct gtbe98_vlan_plan plan;
	struct gtbe98_vlan_observation o, changed;
	unsigned int vid, port, fid;

	assert(gtbe98_wan_vlan_plan(&r, &plan) == GTBE98_VLAN_OK);
	memset(&o, 0, sizeof(o));
	o.complete = 1;
	o.entries = entries;
	o.count = GTBE98_VLAN_COUNT;
	entries[0].members = entries[0].untag = 0xd8;
	entries[835].members = 0x28;
	entries[835].fid = 1;
	o.pvid[5] = 835;
	o.ingress_filter = 0xf8;
	o.tagged_only = 0x20;
	o.untagged_only = 0xd0;
	o.admit_all = 0x08;
	o.egress_filter = 1;
	assert(gtbe98_wan_vlan_verify(&plan, &o) == GTBE98_VLAN_OK);
	assert(gtbe98_wan_vlan_verify(&plan, NULL) == GTBE98_VLAN_UNVERIFIED);
	changed = o; changed.complete = 0;
	assert(gtbe98_wan_vlan_verify(&plan, &changed) == GTBE98_VLAN_UNVERIFIED);
	changed = o; changed.count--;
	assert(gtbe98_wan_vlan_verify(&plan, &changed) == GTBE98_VLAN_UNVERIFIED);
	changed = o; changed.entries = NULL;
	assert(gtbe98_wan_vlan_verify(&plan, &changed) == GTBE98_VLAN_UNVERIFIED);

	/* Every stale VID, including default 1 and internal 4094, must fail. */
	for (vid = 1; vid < GTBE98_VLAN_COUNT; ++vid) {
		if (vid == 835)
			continue;
		entries[vid].members = 0xf8;
		assert(gtbe98_wan_vlan_verify(&plan, &o) == GTBE98_VLAN_MISMATCH);
		entries[vid].members = 0;
	}
	for (fid = 0; fid <= 16; ++fid) {
		entries[835].fid = fid;
		assert(gtbe98_wan_vlan_verify(&plan, &o) ==
			((fid > 0 && fid < 16) ? GTBE98_VLAN_OK : GTBE98_VLAN_MISMATCH));
	}
	entries[835].fid = 1;
	for (port = 0; port < 8; ++port) {
		/* Any unexpected member/untag change is rejected. */
		entries[835].members ^= 1U << port;
		assert(gtbe98_wan_vlan_verify(&plan, &o) == GTBE98_VLAN_MISMATCH);
		entries[835].members ^= 1U << port;
		entries[835].untag = 1U << port;
		assert(gtbe98_wan_vlan_verify(&plan, &o) == GTBE98_VLAN_MISMATCH);
		entries[835].untag = 0;
		entries[0].members ^= 1U << port;
		assert(gtbe98_wan_vlan_verify(&plan, &o) == GTBE98_VLAN_MISMATCH);
		entries[0].members ^= 1U << port;
		if (!(GTBE98_VLAN_PORTS & (1U << port)))
			continue;
		changed = o; changed.pvid[port] = 4094;
		assert(gtbe98_wan_vlan_verify(&plan, &changed) == GTBE98_VLAN_MISMATCH);
		changed = o; changed.ingress_filter &= ~(1U << port);
		assert(gtbe98_wan_vlan_verify(&plan, &changed) == GTBE98_VLAN_MISMATCH);
	}
	changed = o; changed.tagged_only = 0;
	assert(gtbe98_wan_vlan_verify(&plan, &changed) == GTBE98_VLAN_MISMATCH);
	changed = o; changed.untagged_only = 0;
	assert(gtbe98_wan_vlan_verify(&plan, &changed) == GTBE98_VLAN_MISMATCH);
	changed = o; changed.admit_all |= GTBE98_VLAN_WAN;
	assert(gtbe98_wan_vlan_verify(&plan, &changed) == GTBE98_VLAN_MISMATCH);
	assert(gtbe98_wan_vlan_verify(&plan, &o) == GTBE98_VLAN_OK);
}

struct fake_switch {
	struct gtbe98_vlan_entry entries[GTBE98_VLAN_COUNT];
	struct gtbe98_vlan_observation observed;
	unsigned int selected, commands, quiesces;
	unsigned int fail_command, ignore_command;
	int fail_quiesce, fail_readback;
};

static int fake_quiesce(void *ctx)
{
	struct fake_switch *s = ctx;
	s->quiesces++;
	return s->fail_quiesce;
}

static int fake_command(void *ctx, unsigned int cmd, unsigned int arg)
{
	struct fake_switch *s = ctx;
	unsigned int i, members;
	assert(s->quiesces);
	s->commands++;
	if (s->commands == s->fail_command)
		return -1;
	if (s->commands == s->ignore_command)
		return 0; /* Legacy driver may hide SDK errors. */
	switch (cmd) {
	case 38:
		memset(s->entries, 0, sizeof(s->entries));
		s->observed.egress_filter = 1;
		s->entries[1].members = s->entries[1].untag = 0xf8;
		for (i = 0; i < 8; ++i)
			s->observed.pvid[i] = 1;
		break;
	case 36: s->selected = arg; break;
	case 37: break;
	case 3901:
		memset(&s->entries[s->selected], 0, sizeof(s->entries[0]));
		break;
	case 39:
		members = (arg & 0x3f7) | 8;
		s->entries[s->selected].members = members;
		s->entries[s->selected].untag = (arg >> 16) | (s->selected ? 0 : 8);
		s->entries[s->selected].fid = s->selected ? 1 : 0;
		for (i = 0; i < 8; ++i)
			if (members & (1U << i))
				s->observed.pvid[i] = s->selected;
		break;
	case 380: s->observed.ingress_filter = arg; break;
	case 3951: s->observed.admit_all = arg; break;
	case 3952: s->observed.tagged_only = arg; break;
	case 3953: s->observed.untagged_only = arg; break;
	default: assert(0); /* In particular, never power up the WAN here. */
	}
	return 0;
}

static int fake_readback(void *ctx, struct gtbe98_vlan_observation *o)
{
	struct fake_switch *s = ctx;
	*o = s->observed;
	o->entries = s->entries;
	o->count = GTBE98_VLAN_COUNT;
	o->complete = 1;
	return s->fail_readback;
}

static void test_stage(void)
{
	static struct fake_switch s;
	struct gtbe98_vlan_request r = request();
	struct gtbe98_vlan_plan plan;
	struct gtbe98_vlan_ops ops = { fake_quiesce, fake_command, fake_readback };
	unsigned int i, commands;
	assert(gtbe98_wan_vlan_plan(&r, &plan) == GTBE98_VLAN_OK);
	assert(gtbe98_wan_vlan_stage(&plan, &ops, &s) == GTBE98_VLAN_OK);
	assert(s.quiesces == 1);
	commands = s.commands;
	for (i = 1; i <= commands; ++i) {
		memset(&s, 0, sizeof(s));
		s.fail_command = i;
		assert(gtbe98_wan_vlan_stage(&plan, &ops, &s) == GTBE98_VLAN_IO_ERROR);
		assert(s.commands == i && s.quiesces == 2);
	}
	memset(&s, 0, sizeof(s));
	s.fail_quiesce = 1;
	assert(gtbe98_wan_vlan_stage(&plan, &ops, &s) == GTBE98_VLAN_QUIESCE_ERROR);
	assert(!s.commands);
	memset(&s, 0, sizeof(s));
	s.fail_readback = 1;
	assert(gtbe98_wan_vlan_stage(&plan, &ops, &s) == GTBE98_VLAN_IO_ERROR);
	assert(s.quiesces == 2);
	/* An ignored VLAN-delete succeeds at ioctl level but must fail verification. */
	memset(&s, 0, sizeof(s));
	s.ignore_command = 3;
	assert(gtbe98_wan_vlan_stage(&plan, &ops, &s) == GTBE98_VLAN_MISMATCH);
	assert(s.quiesces == 2);
	memset(&s, 0, sizeof(s));
	plan.isp_create_mask |= GTBE98_VLAN_LAN;
	assert(gtbe98_wan_vlan_stage(&plan, &ops, &s) == GTBE98_VLAN_INVALID);
	assert(!s.quiesces && !s.commands);
}

int main(void)
{
	test_input();
	test_policy();
	test_plan();
	test_readback();
	test_stage();
	puts("GT-BE98 WAN VLAN policy, plan and synthetic readback tests passed");
	return 0;
}
