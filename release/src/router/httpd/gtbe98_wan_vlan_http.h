/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Included by web.c: preflight must run before any request NVRAM writes. */
#include <gtbe98_wan_vlan.h>

static const char *gtbe98_http_value(json_object *root, const char *name)
{
	const char *value = get_cgi_json((char *)name, root);
	return value ? value : nvram_safe_get(name);
}

/* Unindexed names are only a WAN-unit-zero form projection. Conflicting
 * aliases are rejected by preflight, never resolved by iteration order. */
static const char *gtbe98_http_wan_value(json_object *root, const char *suffix)
{
	char key[32];
	const char *value;
	snprintf(key, sizeof(key), "wan_%s", suffix);
	value = get_cgi_json(key, root);
	if (value)
		return value;
	snprintf(key, sizeof(key), "wan0_%s", suffix);
	return gtbe98_http_value(root, key);
}

/* Validate supplied values even when disabling VLAN: this path deliberately
 * bypasses the generic unit projection and must still enforce its bounds. */
static int gtbe98_http_field_valid(const char *suffix, const char *value)
{
	unsigned int i, number = 0;
	if (!strcmp(suffix, "dot1q"))
		return !strcmp(value, "0") || !strcmp(value, "1");
	if (!strcmp(suffix, "dot1p"))
		return value[0] >= '0' && value[0] <= '7' && !value[1];
	if (!*value)
		return 1; /* Empty VID is valid only while disabled; plan checks enable. */
	for (i = 0; value[i]; ++i) {
		if (i >= 4 || value[i] < '0' || value[i] > '9')
			return 0;
		number = number * 10 + value[i] - '0';
	}
	return number >= 3 && number <= 4093;
}

/* Ownership survives a pending disable until reboot; dedicated mutation
 * endpoints cannot treat the requested enable flag as the hardware state. */
static int gtbe98_http_locked(void)
{
	const char *enabled = nvram_safe_get("wan0_dot1q");
	return (*enabled && strcmp(enabled, "0")) || nvram_match("gtbe98_wan_vlan_owned", "1");
}

#define GTBE98_HTTP_SDN_CONFLICT "Disable explicit WAN VLAN and reboot before changing SDN or LAN VLAN settings"
#define GTBE98_HTTP_USB_CONFLICT "Disable explicit WAN VLAN and reboot before enabling USB WAN backup"

static const char *gtbe98_http_preflight(json_object *root)
{
	static const char *fields[] = { "dot1q", "vid", "dot1p" };
	static const char *topology[] = { "sw_mode", "autowan_enable", "wans_dualwan",
		"wans_extwan", "bond_wan", "vlan_enable", "vlan_rl", "wans_standby",
#ifdef RTCONFIG_USB_WAN_BACKUP
		"wans_usb_bk", "wans_usb_bk_act",
#endif
		"sdn_rl" };
	static const char *iptv[] = { "switch_wantag", "switch_stb_x", "switch_wan0tagid",
		"switch_wan0prio", "switch_wan1tagid", "switch_wan1prio",
		"switch_wan2tagid", "switch_wan2prio" };
	struct gtbe98_vlan_request request;
	struct gtbe98_vlan_plan plan;
	enum gtbe98_vlan_result result;
	const char *value, *alias, *unit, *action;
	char key[32], indexed[32];
	unsigned int i, j;
	int relevant = 0, changed = 0, was_enabled, enabled;

	was_enabled = gtbe98_http_locked();
	unit = get_cgi_json("wan_unit", root);
	for (i = 0; i < sizeof(fields)/sizeof(fields[0]); ++i) {
		snprintf(key, sizeof(key), "wan_%s", fields[i]);
		snprintf(indexed, sizeof(indexed), "wan0_%s", fields[i]);
		value = get_cgi_json(key, root);
		alias = get_cgi_json(indexed, root);
		if (value && (!unit || strcmp(unit, "0")))
			return "Explicit WAN VLAN form fields require WAN unit 0";
		if (value && alias && strcmp(value, alias))
			return "Conflicting indexed and unindexed WAN VLAN parameters";
		if (value || alias) {
			if (!gtbe98_http_field_valid(fields[i], value ? value : alias))
				return "Invalid WAN VLAN parameters";
			relevant = 1;
			if (strcmp(gtbe98_http_wan_value(root, fields[i]), nvram_safe_get(indexed)))
				changed = 1;
		}
		/* Includes the IPTV WAN units accepted by validate_instance(). */
		for (j = 1; j <= 15; ++j) {
			snprintf(key, sizeof(key), "wan%u_%s", j, fields[i]);
			if (get_cgi_json(key, root))
				return "Explicit WAN VLAN is supported only on WAN unit 0";
		}
	}
	for (i = 0; i < sizeof(topology)/sizeof(topology[0]); ++i) {
		value = get_cgi_json((char *)topology[i], root);
		if (value) {
			relevant = 1;
			if (strcmp(value, nvram_safe_get(topology[i])))
				changed = 1;
		}
	}
	enabled = !strcmp(gtbe98_http_wan_value(root, "dot1q"), "1");
	value = get_cgi_json("sdn_rl", root);
	if ((was_enabled || enabled) && value && strcmp(value, nvram_safe_get("sdn_rl")))
		return GTBE98_HTTP_SDN_CONFLICT;
	for (i = 0; i < sizeof(iptv)/sizeof(iptv[0]); ++i) {
		value = get_cgi_json((char *)iptv[i], root);
		if (value) {
			relevant = 1;
			if (was_enabled && strcmp(value, nvram_safe_get(iptv[i])))
				return "Disable explicit WAN VLAN and reboot before changing IPTV settings";
		}
	}
	if (!relevant)
		return NULL;
	memset(&request, 0, sizeof(request));
	request.dot1q = gtbe98_http_wan_value(root, "dot1q");
	/* Older installations have no canonical key until first explicit apply. */
	if (!*request.dot1q && !get_cgi_json("wan_dot1q", root) &&
	    !get_cgi_json("wan0_dot1q", root))
		request.dot1q = "0";
	request.vid = gtbe98_http_wan_value(root, "vid");
	request.dot1p = gtbe98_http_wan_value(root, "dot1p");
	request.iptv_profile = gtbe98_http_value(root, "switch_wantag");
	request.stb_port = gtbe98_http_value(root, "switch_stb_x");
	request.gtbe98_rtl8372 = 1;
	request.router_mode = !strcmp(gtbe98_http_value(root, "sw_mode"), "1");
	request.wan_unit = 0;
	request.manual_wan = !strcmp(gtbe98_http_value(root, "autowan_enable"), "0");
	request.single_wan = !strcmp(gtbe98_http_value(root, "wans_dualwan"), "wan none") &&
		!strcmp(gtbe98_http_value(root, "wans_standby"), "0");
#ifdef RTCONFIG_USB_WAN_BACKUP
	request.single_wan = request.single_wan &&
		!strcmp(gtbe98_http_value(root, "wans_usb_bk"), "0") &&
		!strcmp(gtbe98_http_value(root, "wans_usb_bk_act"), "0");
#endif
	value = gtbe98_http_value(root, "wans_extwan");
	request.extwan = !strcmp(value, "0") ? 0 : !strcmp(value, "1") ? 1 : -1;
	request.wan_aggregation = strcmp(gtbe98_http_value(root, "bond_wan"), "0") != 0;
	request.lan_vlan_state = (*gtbe98_http_value(root, "vlan_rl") ||
		strcmp(gtbe98_http_value(root, "vlan_enable"), "0")) ? 1 : 0;
	result = gtbe98_wan_vlan_plan(&request, &plan);
	if (result != GTBE98_VLAN_OK && result != GTBE98_VLAN_DISABLED)
		return gtbe98_wan_vlan_error(result);
	if (enabled && !nvram_match("gtbe98_wan_vlan_cap", "1"))
		return "Explicit WAN VLAN is unavailable: verified switch readback support is required";
	if (changed && (was_enabled || enabled)) {
		/* Each endpoint executes a different field. Do not accept an ignored
		 * alias as evidence that this request will actually reboot. */
		action = get_cgi_json(root ? "rc_service" : "action_script", root);
		if (!action || strcmp(action, "reboot"))
			return "WAN VLAN or port changes require applying with a reboot";
	}
	return NULL;
}
