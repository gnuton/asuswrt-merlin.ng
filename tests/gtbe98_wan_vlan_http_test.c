/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct { const char *name; const char *value; } item;
typedef struct { item values[64]; unsigned int count; } json_object;
static json_object config, post;
static void put(json_object *table, const char *name, const char *value)
{
	unsigned int i;
	for (i = 0; i < table->count; ++i)
		if (!strcmp(table->values[i].name, name)) {
			table->values[i].value = value;
			return;
		}
	assert(table->count < 64);
	table->values[table->count].name = name;
	table->values[table->count++].value = value;
}
static char *get_cgi_json(char *name, json_object *root)
{
	unsigned int i;
	if (!root) root = &post;
	for (i = 0; i < root->count; ++i)
		if (!strcmp(root->values[i].name, name))
			return (char *)root->values[i].value;
	return NULL;
}
static char *nvram_safe_get(const char *name)
{
	char *value = get_cgi_json((char *)name, &config);
	return value ? value : "";
}
static int nvram_match(const char *name, const char *value)
{
	return !strcmp(nvram_safe_get(name), value);
}
#define RTCONFIG_USB_WAN_BACKUP
#include "../release/src/router/httpd/gtbe98_wan_vlan_http.h"
static void reset(void)
{
	memset(&config, 0, sizeof(config));
	memset(&post, 0, sizeof(post));
	put(&config, "wan0_dot1q", "0");
	put(&config, "wan0_vid", "835");
	put(&config, "wan0_dot1p", "0");
	put(&config, "sw_mode", "1");
	put(&config, "switch_wantag", "none");
	put(&config, "switch_stb_x", "0");
	put(&config, "autowan_enable", "0");
	put(&config, "wans_dualwan", "wan none");
	put(&config, "wans_standby", "0");
	put(&config, "wans_usb_bk", "0");
	put(&config, "wans_usb_bk_act", "0");
	put(&config, "wans_extwan", "1");
	put(&config, "bond_wan", "0");
	put(&config, "vlan_enable", "0");
	put(&config, "gtbe98_wan_vlan_cap", "1");
	put(&post, "wan0_dot1q", "1");
	put(&post, "action_script", "reboot");
}
int main(void)
{
	const char *invalid[] = { "0", "1", "2", "4094", "4095", "-1", "835 ", "835x", "99999999999999", "" };
	const char *mode_keys[] = { "sw_mode", "autowan_enable", "wans_dualwan", "bond_wan", "vlan_enable", "vlan_rl", "wans_standby" };
	const char *mode_values[] = { "2", "1", "wan usb", "1", "1", "<1>10", "1" };
	unsigned int i;
	reset(); assert(!gtbe98_http_preflight(NULL));
	reset(); put(&post, "action_script", "restart_wan"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&post, "rc_service", "reboot"); assert(!gtbe98_http_preflight(&post));
	reset(); assert(gtbe98_http_preflight(&post)); /* JSON ignores action_script. */
	reset(); put(&post, "rc_service", "reboot"); put(&post, "action_script", "restart_wan"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&config, "gtbe98_wan_vlan_cap", "0"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&config, "wan0_dot1q", "1"); put(&config, "gtbe98_wan_vlan_cap", "0"); put(&post, "wan0_dot1q", "0"); assert(!gtbe98_http_preflight(NULL));
	reset(); put(&post, "wan_dot1q", "1"); assert(gtbe98_http_preflight(NULL));
	put(&post, "wan_unit", "0"); assert(!gtbe98_http_preflight(NULL));
	put(&post, "wan_dot1q", "0"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&post, "wan1_dot1q", "1"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&post, "wan10_vid", "835"); assert(gtbe98_http_preflight(NULL));
	for (i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
		reset(); put(&post, "wan0_vid", invalid[i]); assert(gtbe98_http_preflight(NULL));
	}
	reset(); put(&post, "wan0_dot1q", "0"); put(&post, "wan0_vid", "99999999999999"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&post, "wan0_dot1q", "0"); put(&post, "wan0_dot1p", "8"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&post, "wan0_dot1q", ""); assert(gtbe98_http_preflight(NULL));
	reset(); put(&post, "wans_standby", "01"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&post, "wan0_dot1p", "8"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&post, "wan0_dot1p", "00"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&config, "switch_wantag", "manual"); assert(gtbe98_http_preflight(NULL));
	assert(nvram_match("switch_wantag", "manual")); /* Preflight does not migrate. */
	reset(); put(&post, "switch_stb_x", "1"); assert(gtbe98_http_preflight(NULL));
	for (i = 0; i < sizeof(mode_keys)/sizeof(mode_keys[0]); ++i) {
		reset(); put(&config, "wan0_dot1q", "1"); post.count = 0;
		put(&post, mode_keys[i], mode_values[i]); assert(gtbe98_http_preflight(NULL));
	}
	reset(); put(&config, "wan0_dot1q", "1"); post.count = 0;
	put(&post, "switch_wan0tagid", "42"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&config, "wan0_dot1q", "1"); post.count = 0;
	put(&post, "wans_extwan", "0"); assert(gtbe98_http_preflight(NULL));
	put(&post, "action_script", "reboot"); assert(!gtbe98_http_preflight(NULL));
	reset(); put(&config, "wan0_dot1q", "0"); post.count = 0;
	put(&post, "switch_wantag", "manual"); assert(!gtbe98_http_preflight(NULL));
	reset(); put(&config, "gtbe98_wan_vlan_cap", "0"); post.count = 0;
	put(&post, "http_username", "admin"); assert(!gtbe98_http_preflight(NULL));
	reset(); put(&config, "wans_usb_bk", "1"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&config, "wans_usb_bk_act", "1"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&config, "wan0_dot1q", "1"); post.count = 0;
	put(&post, "wans_usb_bk", "1"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&config, "gtbe98_wan_vlan_owned", "1"); put(&config, "wan0_dot1q", "0"); post.count = 0;
	assert(gtbe98_http_locked()); put(&post, "sdn_rl", "<1>Guest>1>1"); assert(gtbe98_http_preflight(NULL));
	/* A saved disable is not a hardware release. IPTV must wait for reboot,
	 * including a request trying to disable and change IPTV in one apply. */
	post.count = 0; put(&post, "switch_wantag", "manual");
	assert(gtbe98_http_preflight(NULL));
	put(&post, "action_script", "reboot"); assert(gtbe98_http_preflight(NULL));
	reset(); put(&config, "wan0_dot1q", "1"); put(&post, "wan0_dot1q", "0");
	put(&post, "switch_wantag", "manual"); assert(gtbe98_http_preflight(NULL));
	/* Flashing with no new keys leaves legacy IPTV, USB backup and LAN VLANs
	 * usable; these exclusions apply only to the opt-in path. */
	reset(); put(&config, "wan0_dot1q", ""); put(&config, "gtbe98_wan_vlan_cap", "0");
	put(&config, "switch_wantag", "manual"); put(&config, "switch_wan0tagid", "835");
	put(&config, "wans_extwan", "0"); put(&config, "wans_usb_bk", "1");
	put(&config, "vlan_rl", "<1>10"); post.count = 0;
	put(&post, "switch_wan0prio", "0"); assert(!gtbe98_http_preflight(NULL));
	put(&post, "wan_unit", "1"); put(&post, "wan_proto", "dhcp");
	assert(!gtbe98_http_preflight(NULL)); assert(!gtbe98_http_locked());
	/* After disable + reboot, the legacy profile can be restored even if the
	 * experimental readback is unavailable. Its ISP values are preserved. */
	put(&config, "wan0_dot1q", "0"); put(&config, "gtbe98_wan_vlan_owned", "0");
	put(&post, "switch_wantag", "manual"); assert(!gtbe98_http_preflight(NULL));
	assert(nvram_match("switch_wan0tagid", "835"));
	puts("GT-BE98 HTTP prospective preflight tests passed");
	return 0;
}
