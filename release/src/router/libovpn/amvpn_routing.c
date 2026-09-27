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
 *
 */

/*
 * VPN utility library for Asuswrt-Merlin
 * Provides some of the functions found in Asuswrt's
 * proprietary libvpn, either re-implemented, or
 * implemented as wrappers around AM's functions.
 * Also includes additional functions developed
 * for Asuswrt-Merlin.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#include <shutils.h>
#include <shared.h>
#ifdef RTCONFIG_SOC_IPQ53XX
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#endif
#include "openvpn_config.h"
#include "openvpn_control.h"
#include "openvpn_setup.h"
#include "amvpn_routing.h"
#ifdef RTCONFIG_SOC_IPQ53XX
#include "amvpn_deferred.h"
#include <qca_vpn_deferred.h>
#endif

#ifdef RTCONFIG_SOC_IPQ53XX
/* QCA IPv4 rule transactions. Only the simple rules emitted here are owned:
 * selector extensions (marks, ports, suppression, etc.) remain untouched.
 * Publish the complete desired set before removing the captured old set. */
struct qca_vpn_rule {
	unsigned int priority;
	char src[64], dst[64], iif[IFNAMSIZ], table[32];
	int prohibit, added, retained, protocol, opaque;
	struct qca_vpn_rule *next;
};

#define QCA_RULE_PROTOCOL_FIRST 200
#define QCA_RULE_PROTOCOL_LAST 215

static void qca_free_rules(struct qca_vpn_rule *rules)
{
	struct qca_vpn_rule *next;
	while (rules) { next = rules->next; free(rules); rules = next; }
}

static int qca_selector(const char *text, struct in_addr *address, int *bits)
{
	char copy[64], *slash, *end;
	long prefix = 32;
	if (!strcmp(text, "all")) { address->s_addr = 0; *bits = 0; return 1; }
	if (strlen(text) >= sizeof(copy)) return 0;
	strcpy(copy, text);
	slash = strchr(copy, '/');
	if (slash) {
		*slash++ = '\0';
		errno = 0; prefix = strtol(slash, &end, 10);
		if (errno || end == slash || *end || prefix < 0 || prefix > 32) return 0;
	}
	if (inet_pton(AF_INET, copy, address) != 1) return 0;
	address->s_addr &= prefix ? htonl(0xffffffffU << (32 - prefix)) : 0;
	*bits = prefix;
	return 1;
}

static int qca_same_selector(const char *left, const char *right)
{
	struct in_addr a, b;
	int a_bits, b_bits;
	return !strcmp(left, right) ||
		(qca_selector(left, &a, &a_bits) && qca_selector(right, &b, &b_bits) &&
		 a_bits == b_bits && a.s_addr == b.s_addr);
}

static int qca_table_number(const char *table)
{
	char extra, *end;
	int unit;
	long number;
	if (!strcmp(table, "main")) return 254;
	if (sscanf(table, "ovpnc%d%c", &unit, &extra) == 1 && unit >= 1 && unit <= OVPN_CLIENT_MAX)
		return WG_CLIENT_MAX + unit;
	if (sscanf(table, "wgc%d%c", &unit, &extra) == 1 && unit >= 1 && unit <= WG_CLIENT_MAX)
		return unit;
	errno = 0; number = strtol(table, &end, 10);
	return !errno && end != table && !*end && number > 0 && number <= 254 ? number : -1;
}

static int qca_same_rule(const struct qca_vpn_rule *left, const struct qca_vpn_rule *right)
{
	int table = qca_table_number(left->table);
	return left->priority == right->priority && left->prohibit == right->prohibit &&
		qca_same_selector(left->src, right->src) && qca_same_selector(left->dst, right->dst) &&
		!strcmp(left->iif, right->iif) &&
		(!strcmp(left->table, right->table) || (table > 0 && table == qca_table_number(right->table)));
}

/* The kernel interprets omitted/zero selectors as wildcards when deleting.
 * A captured foreign rule must never be a possible deletion match. */
static int qca_delete_may_match(const struct qca_vpn_rule *command, const struct qca_vpn_rule *foreign)
{
	if (command->priority != foreign->priority) return 0;
	if (foreign->opaque) return 1;
	return command->prohibit == foreign->prohibit &&
		(command->prohibit || qca_table_number(command->table) == qca_table_number(foreign->table)) &&
		(!*command->iif || !strcmp(command->iif, foreign->iif)) &&
		(!command->protocol || command->protocol == foreign->protocol) &&
		(qca_same_selector(command->src, "all") || qca_same_selector(command->src, foreign->src)) &&
		(qca_same_selector(command->dst, "all") || qca_same_selector(command->dst, foreign->dst));
}

static int qca_append_rule(struct qca_vpn_rule **rules, unsigned int priority,
	const char *src, const char *dst, const char *iif, const char *table)
{
	struct qca_vpn_rule value, *item, **tail = rules;
	memset(&value, 0, sizeof(value));
	if (!src || !*src || !strcmp(src, "0.0.0.0") || !strcmp(src, "0.0.0.0/0")) src = "all";
	if (!dst || !*dst || !strcmp(dst, "0.0.0.0") || !strcmp(dst, "0.0.0.0/0")) dst = "all";
	if (strlen(src) >= sizeof(value.src) || strlen(dst) >= sizeof(value.dst) ||
	    (iif && strlen(iif) >= sizeof(value.iif)) || (table && strlen(table) >= sizeof(value.table)))
		return -1;
	value.priority = priority;
	strcpy(value.src, src); strcpy(value.dst, dst);
	if (iif) strcpy(value.iif, iif);
	if (table) strcpy(value.table, table);
	else value.prohibit = 1;
	for (item = *rules; item; item = item->next) {
		if (qca_same_rule(item, &value))
			return 0;
		tail = &item->next;
	}
	item = malloc(sizeof(*item));
	if (!item) return -1;
	*item = value;
	*tail = item;
	return 0;
}

static int qca_parse_rule(char *line, struct qca_vpn_rule *rule)
{
	char *token, *save, *end;
	unsigned long priority;
	int seen_from = 0, seen_to = 0, seen_iif = 0, seen_action = 0, seen_protocol = 0;
	memset(rule, 0, sizeof(*rule));
	strcpy(rule->dst, "all");
	errno = 0;
	priority = strtoul(line, &end, 10);
	if (errno || end == line || *end != ':' || priority > 32767) return 0;
	rule->priority = priority;
	for (token = strtok_r(end + 1, " \t\r\n", &save); token;
	     token = strtok_r(NULL, " \t\r\n", &save)) {
		char *value, *target;
		size_t size;
		if ((!strcmp(token, "proto") || !strcmp(token, "protocol")) && !seen_protocol++) {
			long protocol;
			value = strtok_r(NULL, " \t\r\n", &save);
			if (!value) return 0;
			errno = 0; protocol = strtol(value, &end, 10);
			if (errno || end == value || *end ||
			    (protocol != 0 && (protocol < QCA_RULE_PROTOCOL_FIRST || protocol > QCA_RULE_PROTOCOL_LAST)))
				return 0;
			rule->protocol = protocol;
			continue;
		}
		if ((!strcmp(token, "prohibit") || !strcmp(token, "8")) && !seen_action++) {
			rule->prohibit = 1;
			continue;
		}
		if (!strcmp(token, "from") && !seen_from++) { target = rule->src; size = sizeof(rule->src); }
		else if (!strcmp(token, "to") && !seen_to++) { target = rule->dst; size = sizeof(rule->dst); }
		else if (!strcmp(token, "iif") && !seen_iif++) { target = rule->iif; size = sizeof(rule->iif); }
		else if (!strcmp(token, "lookup") && !seen_action++) { target = rule->table; size = sizeof(rule->table); }
		else return 0;
		value = strtok_r(NULL, " \t\r\n", &save);
		if (!value || strlen(value) >= size) return 0;
		strcpy(target, value);
	}
	return seen_from == 1 && seen_action == 1;
}

static int qca_exec_rule(const char *action, const struct qca_vpn_rule *rule)
{
	char priority[16], protocol[16], *argv[20];
	int n = 0;
	snprintf(priority, sizeof(priority), "%u", rule->priority);
	snprintf(protocol, sizeof(protocol), "%d", rule->protocol);
	argv[n++] = "ip"; argv[n++] = "rule"; argv[n++] = (char *)action;
	argv[n++] = "from"; argv[n++] = (char *)rule->src;
	argv[n++] = "to"; argv[n++] = (char *)rule->dst;
	if (*rule->iif) { argv[n++] = "iif"; argv[n++] = (char *)rule->iif; }
	argv[n++] = "priority"; argv[n++] = priority;
	if (rule->prohibit) argv[n++] = "prohibit";
	else { argv[n++] = "table"; argv[n++] = (char *)rule->table; }
	argv[n++] = "protocol"; argv[n++] = protocol;
	argv[n] = NULL;
	return _eval(argv, NULL, 0, NULL);
}

static int qca_rule_table(const char *actual, const char *expected, int index)
{
	char numeric[16];
	snprintf(numeric, sizeof(numeric), "%d", index);
	return !strcmp(actual, expected) || !strcmp(actual, numeric);
}

static int qca_sdn_uses_client(const char *iif, int index, const char *supplied,
	const MTLAN_T *mtlan, size_t count)
{
	size_t i;
	int matches = 0;
	if (supplied && *supplied && strcmp(iif, supplied)) return 0;
	for (i = 0; i < count; ++i) {
		int target = mtlan[i].sdn_t.vpnc_idx;
		if (strcmp(iif, mtlan[i].nw_t.ifname)) continue;
		if (!target && !strcmp(mtlan[i].nw_t.ifname, nvram_safe_get("lan_ifname")))
			target = nvram_get_int("vpnc_default_wan");
		/* An explicit callback may arrive after this interface was reassigned.
		 * It cannot claim another client's guard or an unresolved stock binding. */
		if (qca_vpn_sdn_deferred(mtlan[i].sdn_t.sdn_idx, target) || (target && target != index)) return 0;
		matches |= target == index || (supplied && *supplied && !target);
	}
	return matches;
}

static int qca_policy_guards(struct qca_vpn_rule **desired, unsigned int priority,
	const char *src, const char *dst, const MTLAN_T *mtlan, size_t count)
{
	size_t i;
	if (*src && strcmp(src, "0.0.0.0") && strcmp(src, "0.0.0.0/0"))
		return qca_append_rule(desired, priority, src, dst, NULL, NULL);
	if (qca_append_rule(desired, priority, "all", dst, nvram_safe_get("lan_ifname"), NULL)) return -1;
	for (i = 0; i < count; ++i)
		if (mtlan[i].enable && *mtlan[i].nw_t.ifname &&
		    qca_append_rule(desired, priority, "all", dst, mtlan[i].nw_t.ifname, NULL)) return -1;
	return 0;
}

/* Caller holds VPNROUTING_LOCK. clear is an intentional removal; a failed
 * publication keeps the previous set and rolls back successful new additions. */
static int qca_reconcile_vpn_rules(int unit, vpndir_proto_t proto, int guard,
	int clear, const char *supplied)
{
	struct qca_vpn_rule *desired = NULL, *old = NULL, **old_tail = &old, *foreign = NULL, *item, *other, parsed;
	MTLAN_T *mtlan = NULL;
	size_t count = 0, i;
	char table[32], prefix[32], buffer[8000], line[512], *remaining, *record;
	char *enabled, *description, *src, *dst, *target;
	unsigned int priority, first, last;
	int index, active = 1, rgw = OVPN_RGW_POLICY, state, owned, result = -1;
	int changed = 0, generation = 0, rank, specificity;
	FILE *fp = NULL;
	if (unit == 0 && !guard) {
		strcpy(table, "main"); index = 254;
		priority = first = VPNDIR_PRIO_WAN; last = first + VPNDIR_PRIO_MAX_RULES - 1;
	} else if (proto == VPNDIR_PROTO_OPENVPN && unit >= 1 && unit <= OVPN_CLIENT_MAX) {
		snprintf(table, sizeof(table), "ovpnc%d", unit); index = WG_CLIENT_MAX + unit;
		snprintf(prefix, sizeof(prefix), "vpn_client%d_", unit);
		rgw = nvram_pf_get_int(prefix, "rgw");
		active = nvram_pf_get_int(prefix, "enforce") && ovpn_is_client_enabled(unit);
		priority = first = guard ? VPNDIR_PRIO_KS_OPENVPN + unit - 1 :
			VPNDIR_PRIO_OPENVPN + VPNDIR_PRIO_MAX_RULES * (unit - 1);
		last = guard ? first : first + VPNDIR_PRIO_MAX_RULES - 1;
#ifdef RTCONFIG_WIREGUARD
	} else if (proto == VPNDIR_PROTO_WIREGUARD && unit >= 1 && unit <= WG_CLIENT_MAX) {
		snprintf(table, sizeof(table), "wgc%d", unit); index = unit;
		snprintf(prefix, sizeof(prefix), "wgc%d_", unit);
		active = nvram_pf_get_int(prefix, "enforce") && nvram_pf_get_int(prefix, "enable");
		priority = first = guard ? VPNDIR_PRIO_KS_WIREGUARD + unit - 1 :
			VPNDIR_PRIO_WIREGUARD + VPNDIR_PRIO_MAX_RULES * (unit - 1);
		last = guard ? first : first + VPNDIR_PRIO_MAX_RULES - 1;
#endif
	} else return -1;
#ifdef RTCONFIG_MULTILAN_CFG
	if (guard) {
		mtlan = (MTLAN_T *)INIT_MTLAN(sizeof(MTLAN_T));
		if (!mtlan) goto done;
		/* An empty SDN list does not remove ordinary LAN Director policy. */
		get_mtlan(mtlan, &count);
	}
#endif
	if (!clear && (!guard || (active && (rgw == OVPN_RGW_POLICY || rgw == OVPN_RGW_ALL)))) {
		if (rgw == OVPN_RGW_POLICY || !unit) {
			amvpn_get_policy_rules(unit, buffer, sizeof(buffer), proto);
			remaining = buffer;
			while ((record = strsep(&remaining, "<")) != NULL) {
				if (vstrsep(record, ">", &enabled, &description, &src, &dst, &target) != 5 || !atoi(enabled)) continue;
				if (guard) {
					if (qca_policy_guards(&desired, priority, src, dst, mtlan, count)) goto done;
				} else {
					if (priority > last || qca_append_rule(&desired, priority++, src, dst, NULL, table)) goto done;
				}
			}
		} else if (proto == VPNDIR_PROTO_OPENVPN) {
			if (guard) {
				if (qca_append_rule(&desired, priority, "all", "all", nvram_safe_get("lan_ifname"), NULL)) goto done;
			} else if (rgw == OVPN_RGW_NONE || rgw == OVPN_RGW_ALL) {
				state = get_ovpn_status(OVPN_TYPE_CLIENT, unit);
				if ((state == OVPN_STS_RUNNING || state == OVPN_STS_INIT) &&
				    qca_append_rule(&desired, VPNDIR_PRIO_ALL + unit, "all", "all", NULL, table)) goto done;
			}
		}
		if (guard) {
			if (supplied && *supplied) {
				if (qca_sdn_uses_client(supplied, index, supplied, mtlan, count) &&
				    qca_append_rule(&desired, VPNDIR_PRIO_KS_SDN, "all", "all", supplied, NULL)) goto done;
			} else for (i = 0; i < count; ++i) {
				if (!mtlan[i].enable || !*mtlan[i].nw_t.ifname) continue;
				if (qca_sdn_uses_client(mtlan[i].nw_t.ifname, index, NULL, mtlan, count)) {
					if (qca_append_rule(&desired, VPNDIR_PRIO_KS_SDN, "all", "all", mtlan[i].nw_t.ifname, NULL)) goto done;
				} else if (rgw == OVPN_RGW_ALL && !mtlan[i].sdn_t.vpnc_idx) {
					if (qca_append_rule(&desired, priority, "all", "all", mtlan[i].nw_t.ifname, NULL)) goto done;
				}
			}
		}
	}
	fp = popen("ip -N -details rule show", "r");
	if (!fp) goto done;
	while (fgets(line, sizeof(line), fp)) {
		if (!strchr(line, '\n')) goto done;
		if (!qca_parse_rule(line, &parsed)) {
			/* Unknown selectors/protocols are foreign. A protocol-0 delete
			 * can match their omitted attributes, so remember the priority. */
			item = calloc(1, sizeof(*item));
			if (!item) goto done;
			item->priority = parsed.priority;
			item->opaque = 1;
			item->next = foreign; foreign = item;
			continue;
		}
		if (guard) {
			owned = parsed.prohibit && parsed.priority == first;
			owned |= parsed.prohibit && parsed.priority == VPNDIR_PRIO_KS_SDN &&
				!strcmp(parsed.src, "all") && !strcmp(parsed.dst, "all") &&
				qca_sdn_uses_client(parsed.iif, index, supplied, mtlan, count);
		} else {
			owned = !parsed.prohibit && !*parsed.iif && qca_rule_table(parsed.table, table, index) &&
				((parsed.priority >= first && parsed.priority <= last) ||
				 (unit && proto == VPNDIR_PROTO_OPENVPN && parsed.priority == VPNDIR_PRIO_ALL + unit));
		}
		item = malloc(sizeof(*item));
		if (!item) goto done;
		if (!owned) {
			*item = parsed; item->next = foreign; foreign = item;
			continue;
		}
		*item = parsed; item->next = NULL; *old_tail = item; old_tail = &item->next;
	}
	state = ferror(fp);
	if (pclose(fp)) state = 1;
	fp = NULL;
	if (state) goto done;
	for (item = desired; item; item = item->next) {
		for (other = old; other; other = other->next)
			if (!other->retained && qca_same_rule(other, item)) break;
		if (other) {
			other->retained = item->retained = 1;
			item->protocol = other->protocol;
		} else if (item->priority != VPNDIR_PRIO_KS_SDN) changed = 1;
	}
	for (item = old; item; item = item->next)
		if (item->priority != VPNDIR_PRIO_KS_SDN && !item->retained) changed = 1;
	if (changed) {
		/* Linux treats zero-length selectors as wildcards when checking an
		 * exclusive add. A fresh metadata generation lets widened rules
		 * coexist at the same traffic priority until the old set is removed. */
		for (generation = QCA_RULE_PROTOCOL_FIRST; generation <= QCA_RULE_PROTOCOL_LAST; ++generation) {
			for (item = old; item && item->protocol != generation; item = item->next) {}
			if (!item) break;
		}
		if (generation > QCA_RULE_PROTOCOL_LAST) goto done;
		for (item = old; item; item = item->next)
			if (item->priority != VPNDIR_PRIO_KS_SDN) item->retained = 0;
		for (item = desired; item; item = item->next)
			if (item->priority != VPNDIR_PRIO_KS_SDN) {
				item->protocol = generation;
				item->retained = 0;
			}
	}
	for (item = old; item; item = item->next)
		if (!item->retained)
			for (other = foreign; other; other = other->next)
				if (qca_delete_may_match(item, other)) {
					logmessage("vpndirector", "Foreign IPv4 rule prevents safe cleanup at priority %u", item->priority);
					goto done;
				}
	for (item = desired; item; item = item->next)
		if (!item->retained)
			for (other = foreign; other; other = other->next)
				if (qca_delete_may_match(item, other)) {
					logmessage("vpndirector", "Foreign IPv4 rule prevents safe publication at priority %u", item->priority);
					goto done;
				}
	/* Broad wildcards precede narrower desired rules within a generation.
	 * The narrower add then cannot mistake the broad rule for a duplicate. */
	for (rank = 0; rank <= 2; ++rank) {
		for (item = desired; item; item = item->next) {
			specificity = !qca_same_selector(item->src, "all") + !qca_same_selector(item->dst, "all");
			if (item->retained || specificity != rank) continue;
			if (qca_exec_rule("add", item)) {
				for (other = desired; other; other = other->next)
					if (other->added && qca_exec_rule("del", other))
						logmessage("vpndirector", "Failed to roll back a new IPv4 rule; previous protection retained");
				goto done;
			}
			item->added = 1;
		}
	}
	/* Preserve kernel order: legacy protocol 0 is a deletion wildcard, and
	 * every captured original precedes newly appended rules at its priority. */
	for (item = old; item; item = item->next)
		if (!item->retained && qca_exec_rule("del", item)) goto done;
	result = 0;
done:
	if (fp) pclose(fp);
	if (mtlan) FREE_MTLAN((void *)mtlan);
	qca_free_rules(desired); qca_free_rules(old); qca_free_rules(foreign);
	if (result) logmessage("vpndirector", "Failed to reconcile IPv4 rules for protocol %d unit %d", proto, unit);
	return result;
}

static int qca_refresh_vpn_rules(int unit, vpndir_proto_t proto, int guard, int clear, const char *supplied)
{
	int result, lock = file_lock(VPNROUTING_LOCK);
	if (lock < 0) return -1;
	if (amvpn_refresh_deferred_locked()) { file_unlock(lock); return -1; }
#ifdef RTCONFIG_IPV6
	if (guard && !clear) amvpn_refresh_ipv6_killswitch();
#endif
	result = qca_reconcile_vpn_rules(unit, proto, guard, clear, supplied);
	file_unlock(lock);
	return result;
}

/* services.c owns the outer lock for the complete routing/DNS transaction. */
int amvpn_refresh_policy_rules_locked(int unit)
{
	int i;
	if (amvpn_refresh_deferred_locked()) return -1;
	if (qca_reconcile_vpn_rules(0, VPNDIR_PROTO_NONE, 0, 0, NULL)) return -1;
#ifdef RTCONFIG_IPV6
	amvpn_refresh_ipv6_killswitch();
#endif
#ifdef RTCONFIG_WIREGUARD
	if (!unit) for (i = WG_CLIENT_MAX; i > 0; --i) {
		if (qca_reconcile_vpn_rules(i, VPNDIR_PROTO_WIREGUARD, 1, 0, NULL) ||
		    qca_reconcile_vpn_rules(i, VPNDIR_PROTO_WIREGUARD, 0, 0, NULL)) return -1;
		amvpn_clear_exclusive_dns(i, VPNDIR_PROTO_WIREGUARD);
		wgc_set_exclusive_dns(i);
	}
#endif
	for (i = unit ? unit : OVPN_CLIENT_MAX; i > 0; --i) {
		if (qca_reconcile_vpn_rules(i, VPNDIR_PROTO_OPENVPN, 1, 0, NULL) ||
		    qca_reconcile_vpn_rules(i, VPNDIR_PROTO_OPENVPN, 0, 0, NULL)) return -1;
		amvpn_clear_exclusive_dns(i, VPNDIR_PROTO_OPENVPN);
		ovpn_set_exclusive_dns(i);
		if (unit) break;
	}
	if (unit) amvpn_update_exclusive_dns_rules();
	return 0;
}
#endif


// Remove all rules pointing to a specific client table
// If unit is 0, then remove rules targetting main (i.e. WAN)
void amvpn_clear_routing_rules(int unit, vpndir_proto_t proto) {
#ifdef RTCONFIG_SOC_IPQ53XX
	qca_refresh_vpn_rules(unit, proto, 0, 1, NULL);
	return;
#endif
	FILE *fp;
	char buffer[128], buffer2[128], buffer3[128];
	int prio, verb = 3;
	char table[12], *lookup;
	char target_table[12];

	if (proto == VPNDIR_PROTO_OPENVPN) {
		snprintf(buffer, sizeof (buffer), "vpn_client%d_verb", unit);
		verb = nvram_get_int(buffer);
#ifdef RTCONFIG_WIREGUARD
	} else if (proto == VPNDIR_PROTO_WIREGUARD) {
		verb = 3;
#endif
	} else if (unit != 0){
		return;
	}

	snprintf(buffer, sizeof (buffer), "/usr/sbin/ip rule show > /tmp/vpnrules%d_tmp", unit);
	system(buffer);

	snprintf(buffer, sizeof (buffer), "/tmp/vpnrules%d_tmp", unit);
	fp = fopen(buffer, "r");
	if (fp) {
		if (unit == 0)
			strlcpy(target_table, "main", sizeof (target_table));
		else if (proto == VPNDIR_PROTO_OPENVPN)
			snprintf(target_table, sizeof (target_table), "ovpnc%d", unit);
#ifdef RTCONFIG_WIREGUARD
		else if (proto == VPNDIR_PROTO_WIREGUARD)
			snprintf(target_table, sizeof (target_table), "wgc%d", unit);
#endif
		while (fgets(buffer2, sizeof(buffer2), fp) != NULL) {
			if (buffer2[strlen(buffer2)-1] == '\n')
				buffer2[strlen(buffer2)-1] = '\0';

			if (sscanf(buffer2, "%u", &prio) != 1)
				continue;

			// Only remove rules within our official range
			// max = base + 10 all + (200 wan) + (5 * 200 ovpn) + (5 * 200 wg)
			if ((prio < VPNDIR_PRIO_BASE) ||
			     (prio > (VPNDIR_PRIO_BASE + (OVPN_CLIENT_MAX + WG_CLIENT_MAX) + VPNDIR_PRIO_MAX_RULES + (OVPN_CLIENT_MAX * VPNDIR_PRIO_MAX_RULES) + (WG_CLIENT_MAX * VPNDIR_PRIO_MAX_RULES))))
				continue;

			if ((lookup = strstr(buffer2, "lookup")) == NULL)
				continue;

			if (sscanf(lookup, "lookup %11s", table) != 1)
				continue;
			if (strcmp(table, target_table))
				continue;

			snprintf(buffer3, sizeof (buffer3), "/usr/sbin/ip rule del prio %d", prio);
			if (verb >= 6)
				logmessage("vpndirector", "Removed rule %d", prio);
			system(buffer3);
		}
		fclose(fp);
	}
	unlink(buffer);

#if defined(RTCONFIG_WIREGUARD) && (defined(RTCONFIG_HND_ROUTER_AX_6756) || defined(RTCONFIG_BCM_502L07P2) || defined(RTCONFIG_HND_ROUTER_AX_675X)) || defined(RTCONFIG_HND_ROUTER_BE_4916)
	// Remove all bypass for this unit
	if (proto == VPNDIR_PROTO_WIREGUARD) {
		_amvpn_apply_wg_bypass(unit, 0);
	}
#endif
}

#if defined(RTCONFIG_WIREGUARD) && (defined(RTCONFIG_HND_ROUTER_AX_6756) || defined(RTCONFIG_BCM_502L07P2) || defined(RTCONFIG_HND_ROUTER_AX_675X)) || defined(RTCONFIG_HND_ROUTER_BE_4916)
/* Add or remove WG bypass rules for a specific unit */
void _amvpn_apply_wg_bypass(int unit, int add) {
	char buffer[32], buffer2[128];
	FILE *fp;

	snprintf(buffer, sizeof (buffer), "/etc/wg/vpndirector%d", unit);
	fp = fopen(buffer, "r");
	if (fp) {
		while (fgets(buffer2, sizeof(buffer2), fp) != NULL) {
			if (buffer2[strlen(buffer2)-1] == '\n')
				buffer2[strlen(buffer2)-1] = '\0';
			if (!strcmp(buffer2, "LAN"))
				hnd_skip_wg_all_lan(add);
			else
				hnd_skip_wg_network(add, buffer2);
		}
		fclose(fp);
	}

	if (!add)
		unlink(buffer);
}

/* Re-apply all WGC bypass rules (following a removal) in case we have overlaps */
void amvpn_refresh_wg_bypass_rules() {
	int unit;
	char buffer[32];

	for (unit = 1; unit <= WG_CLIENT_MAX; unit++ ) {
		sprintf(buffer, "wgc%d_enable", unit);
		if (nvram_get_int(buffer)) {
			_amvpn_apply_wg_bypass(unit, 1);
		}
	}
}
#endif


/*
	Rule priority allocations:

	VPNDIR_PRIO_ALL
	- 10000-10009: clients set to OVPN_RGW_ALL

	VPNDIR_PRIO_WAN
	- 10010-10209: WAN rules


	VPNDIR_PRIO_OPENVPN
	- 10210-10409: OVPN 1
	- 10410-10609: OVPN 2
	- 10610-10809: OVPN 3
	- 10810-11009: OVPN 4
	- 11010-11209: OVPN 5

	VPNDIR_PRIO_WIREGUARD
	- 11210-11409: WGC 1
	- 11410-11609: WGC 2
	- 11610-11809: WGC 3
	- 11810-12009: WGC 4
	- 12010-12209: WGC 5

	VPNDIR_PRIO_KS_OPENVPN
	- 12210-12214 (OVPN1 through 5, multiple rules per unit)

	VPNDIR_PRIO_KS_WIREGUARD
	- 12215-12219 (WG1 through 5, multiple rules per unit)

	VPNDIR_PRIO_KS_SDN
	- 122220
*/
void amvpn_set_wan_routing_rules() {
#ifdef RTCONFIG_SOC_IPQ53XX
	qca_refresh_vpn_rules(0, VPNDIR_PROTO_NONE, 0, 0, NULL);
	return;
#endif
	char buffer[8000];

	amvpn_clear_routing_rules(0, VPNDIR_PROTO_NONE);
	amvpn_get_policy_rules(0, buffer, sizeof (buffer), VPNDIR_PROTO_NONE);
	_write_routing_rules(0, buffer, 3, VPNDIR_PROTO_NONE);
}



void amvpn_set_routing_rules(int unit, vpndir_proto_t proto) {
#ifdef RTCONFIG_SOC_IPQ53XX
	qca_refresh_vpn_rules(unit, proto, 0, 0, NULL);
	return;
#endif
	char prefix[32], buffer[8000];
	int rgw, state, verb;
#if 0	//#ifdef RTCONFIG_MULTILAN_CFG
	int i;
	MTLAN_T *pmtl = NULL;
	size_t mtl_sz = 0;
#endif

	if (unit < 1)
		return;

	if (proto == VPNDIR_PROTO_OPENVPN && unit <= OVPN_CLIENT_MAX) {
		snprintf(prefix, sizeof(prefix), "vpn_client%d_", unit);
		verb = nvram_pf_get_int(prefix, "verb");
		rgw = nvram_pf_get_int(prefix, "rgw");
#ifdef RTCONFIG_WIREGUARD
	} else if (proto == VPNDIR_PROTO_WIREGUARD && unit <= WG_CLIENT_MAX) {
		snprintf(prefix, sizeof(prefix), "wgc%d_", unit);
		verb = 3;
		rgw = OVPN_RGW_POLICY;
#endif
	} else {
		return;
	}

	amvpn_clear_routing_rules(unit, proto);

	switch (rgw) {
		case OVPN_RGW_NONE:
		case OVPN_RGW_ALL:
			// Set client rules if running or currently connecting
			state = get_ovpn_status(OVPN_TYPE_CLIENT, unit);
			if (state == OVPN_STS_RUNNING || state == OVPN_STS_INIT) {
//				snprintf(buffer, sizeof (buffer), "/usr/sbin/ip rule add table ovpnc%d priority %d iif %s", unit, VPNDIR_PRIO_ALL + unit, nvram_safe_get("lan_ifname"));
				snprintf(buffer, sizeof (buffer), "/usr/sbin/ip rule add table ovpnc%d priority %d", unit, VPNDIR_PRIO_ALL + unit);
				system(buffer);
				if (verb >= 3)
					logmessage("vpndirector","Routing all traffic through ovpnc%d", unit);
#ifdef RTCONFIG_MULTILAN_CFG
#if 0	// redirect everything, not just by interface - rely on main table copied to vpn table
				pmtl = (MTLAN_T *)INIT_MTLAN(sizeof(MTLAN_T));
				if (pmtl) {
					get_mtlan(pmtl, &mtl_sz);
					// Skip first SDN (Default)
					for (i = 1; i < mtl_sz; ++i) {
						if (pmtl[i].sdn_t.vpnc_idx == 0) {	// If not associated with a VPN, route it
							snprintf(buffer, sizeof (buffer), "/usr/sbin/ip rule add table ovpnc%d priority %d iif %s", unit, VPNDIR_PRIO_ALL + unit, pmtl[i].nw_t.ifname);
							system(buffer);
							if (verb > 3)
								logmessage("vpndirector", "Routing all traffic for SDN %s through ovpnc%d", pmtl[i].nw_t.ifname, unit);
						}
					}
					FREE_MTLAN((void *)pmtl);
				}
#endif
#endif

			}
			break;

		case OVPN_RGW_POLICY:
			amvpn_get_policy_rules(unit, buffer, sizeof (buffer), proto);
			_write_routing_rules(unit, buffer, verb, proto);
			break;
	}
}


void _write_routing_rules(int unit, char *rules, int verb, vpndir_proto_t proto) {
	char *buffer_tmp, *buffer_tmp2, *rule;
	char buffer[128], table[16];
	int ruleprio, vpnprio, wanprio;
	char *enable, *desc, *target, *src, *dst;
	char srcstr[64], dststr[64];
#if defined(RTCONFIG_WIREGUARD) && (defined(RTCONFIG_HND_ROUTER_AX_6756) || defined(RTCONFIG_BCM_502L07P2) || defined(RTCONFIG_HND_ROUTER_AX_675X)) || defined(RTCONFIG_HND_ROUTER_BE_4916)
	int ret;
	char bypass_filename[64];
#endif

	wanprio = VPNDIR_PRIO_WAN;

	if (proto == VPNDIR_PROTO_OPENVPN)
		vpnprio = VPNDIR_PRIO_OPENVPN + (VPNDIR_PRIO_MAX_RULES * (unit-1));
#ifdef RTCONFIG_WIREGUARD
	else if (proto == VPNDIR_PROTO_WIREGUARD)
		vpnprio = VPNDIR_PRIO_WIREGUARD + (VPNDIR_PRIO_MAX_RULES * (unit-1));
#endif
	else if (proto == VPNDIR_PROTO_NONE)
		vpnprio = 0;	// Unused
	else
		return;

	buffer_tmp = buffer_tmp2 = strdup(rules);

	while (buffer_tmp && (rule = strsep(&buffer_tmp2, "<")) != NULL) {
		if((vstrsep(rule, ">", &enable, &desc, &src, &dst, &target)) != 5)
			 continue;

		if (!atoi(&enable[0]))
			continue;

		if (!strcmp(target,"WAN")) {
			strcpy(table, "main");
			ruleprio = wanprio++;
		}
		else if (!strncmp(target, "OVPN", 4) && proto == VPNDIR_PROTO_OPENVPN) {
			snprintf(table, sizeof (table), "ovpnc%d", unit);
			ruleprio = vpnprio++;
		}
#ifdef RTCONFIG_WIREGUARD
		else if (!strncmp(target, "WGC", 3) && proto == VPNDIR_PROTO_WIREGUARD) {
			snprintf(table, sizeof (table), "wgc%d", unit);
			ruleprio = vpnprio++;
		}
#endif

		else
			continue;

		if (*src && strcmp(src, "0.0.0.0")) {
			snprintf(srcstr, sizeof (srcstr), "from %s", src);
#if defined(RTCONFIG_WIREGUARD) && (defined(RTCONFIG_HND_ROUTER_AX_6756) || defined(RTCONFIG_BCM_502L07P2) || defined(RTCONFIG_HND_ROUTER_AX_675X)) || defined(RTCONFIG_HND_ROUTER_BE_4916)
			if ((proto == VPNDIR_PROTO_WIREGUARD) && (!strncmp(target,"WGC", 3))) {
				snprintf(bypass_filename, sizeof (bypass_filename), "/etc/wg/vpndirector%d", unit);

				if (strchr(src, '/'))
					snprintf(buffer, sizeof(buffer), "%s\n", src);
				else {
					ret = is_valid_ip(src);
					if (ret > 1)
						snprintf(buffer, sizeof(buffer), "%s/128\n", src);
					else if (ret > 0)
						snprintf(buffer, sizeof(buffer), "%s/32\n", src);
				}
				f_write_string(bypass_filename, buffer, FW_APPEND, 0);
				sprintf(buffer, "wgc%d_enable", unit);
				if (nvram_get_int(buffer))
					hnd_skip_wg_network(1, src);
			}
#endif
		}
		else {
			*srcstr = '\0';
#if defined(RTCONFIG_WIREGUARD) && (defined(RTCONFIG_HND_ROUTER_AX_6756) || defined(RTCONFIG_BCM_502L07P2) || defined(RTCONFIG_HND_ROUTER_AX_675X)) || defined(RTCONFIG_HND_ROUTER_BE_4916)
			if ((proto == VPNDIR_PROTO_WIREGUARD) && (!strncmp(target,"WGC", 3))) {
				snprintf(bypass_filename, sizeof (bypass_filename), "/etc/wg/vpndirector%d", unit);
				f_write_string(bypass_filename, "LAN\n", FW_APPEND, 0);
				sprintf(buffer, "wgc%d_enable", unit);
				if (nvram_get_int(buffer))
					hnd_skip_wg_all_lan(1);
			}
#endif
		}

		if (*dst && strcmp(dst, "0.0.0.0"))
			snprintf(dststr, sizeof (dststr), "to %s", dst);
		else
			*dststr = '\0';

		snprintf(buffer, sizeof (buffer), "/usr/sbin/ip rule add %s %s table %s priority %d",
				                                   srcstr, dststr, table, ruleprio);

		if (verb >= 3)
			logmessage("vpndirector","Routing %s from %s to %s through %s", desc, (*src ? src : "any"), (*dst ? dst : "any"), table);

		system(buffer);
	}
	free(buffer_tmp);
}

inline void _flush_routing_cache() {
        system("/usr/sbin/ip route flush cache");
}


/* Remove/add server routes from all client routing tables when stopping/starting a client */

void update_client_routes(char *server_iface, int addroute) {
	int unit;
	char buffer[32];

	for (unit = 1; unit <= OVPN_CLIENT_MAX; unit++ ) {
		sprintf(buffer, "vpnclient%d", unit);
		if ( pidof(buffer) >= 0 ) {
			if (addroute)
				_add_server_routes(server_iface, unit, VPNDIR_PROTO_OPENVPN);
			else
				_del_server_routes(server_iface, unit, VPNDIR_PROTO_OPENVPN);
		}
	}

#ifdef RTCONFIG_WIREGUARD
	for (unit = 1; unit <= WG_CLIENT_MAX; unit++ ) {
		sprintf(buffer, "wgc%d_enable", unit);
		if (nvram_get_int(buffer)) {
			if (addroute)
				_add_server_routes(server_iface, unit, VPNDIR_PROTO_WIREGUARD);
			else
				_del_server_routes(server_iface, unit, VPNDIR_PROTO_WIREGUARD);
		}
	}
#endif
}


/* Add / remove OpenVPN server routes from client tables */
/* Server-agnostic, could eventually be reused for other servers like WG/IPSEC */

void _add_server_routes(char *server_iface, int client_unit, vpndir_proto_t proto) {
	char buffer[128], routecmd[128], line[128], table[8];
	FILE *fp_route;

	if (proto == VPNDIR_PROTO_OPENVPN)
		snprintf(table, sizeof (table), "ovpnc%d", client_unit);
#ifdef RTCONFIG_WIREGUARD
	else if (proto == VPNDIR_PROTO_WIREGUARD)
		snprintf(table, sizeof (table), "wgc%d", client_unit);
#endif
	else
		return;

	snprintf(buffer, sizeof (buffer), "/usr/sbin/ip route list table main | grep %s > /tmp/vpnroute_%s_tmp", server_iface, table);
	system(buffer);

	snprintf(buffer, sizeof (buffer), "/tmp/vpnroute_%s_tmp", table);
	fp_route = fopen(buffer, "r");

	if (fp_route) {
		while (fgets(line, sizeof(line), fp_route) != NULL) {
			snprintf(routecmd, sizeof (routecmd), "/usr/sbin/ip route add %s table %s", trimNL(line), table);
			system(routecmd);
		}
		fclose(fp_route);
	}
	unlink(buffer);
}


void _del_server_routes(char *server_iface, int client_unit, vpndir_proto_t proto) {
	char buffer[128], routecmd[128], line[128], table[8];
	FILE *fp_route;

	if (proto == VPNDIR_PROTO_OPENVPN)
		snprintf(table, sizeof (table), "ovpnc%d", client_unit);
#ifdef RTCONFIG_WIREGUARD
	else if (proto == VPNDIR_PROTO_WIREGUARD)
		snprintf(table, sizeof (table), "wgc%d", client_unit);
#endif
	else
		return;

	snprintf(buffer, sizeof (buffer), "/usr/sbin/ip route list table %s | grep %s > /tmp/vpnroute_%s_tmp", table, server_iface, table);
	system(buffer);

	snprintf(buffer, sizeof (buffer), "/tmp/vpnroute_%s_tmp", table);
	fp_route = fopen(buffer, "r");

	if (fp_route) {
		while (fgets(line, sizeof(line), fp_route) != NULL) {
			snprintf(routecmd, sizeof (routecmd), "/usr/sbin/ip route del %s table %s", trimNL(line), table);
			system(routecmd);
		}
		fclose(fp_route);
	}
	unlink(buffer);
}

// Unit -1 = all rules;  unit 0 = WAN rules,  unit 1-5: OVPN or WG instance rules
char *amvpn_get_policy_rules(int unit, char *buffer, int bufferlen, vpndir_proto_t proto)
{
	char filename[128];
	int datalen;
	char *buffer_tmp, *buffer_tmp2;
	char *rule, *enable, *desc, *src, *dst, *target;
	char entry[128];

	snprintf(filename, sizeof(filename), "%s/vpndirector_rulelist", OVPN_FS_PATH);

	datalen = f_read(filename, buffer, bufferlen-1);
	if (datalen < 0) {
#ifdef RTCONFIG_SOC_IPQ53XX
		/* Migration commits policy and SDN/profile mappings together in NVRAM.
		 * A later UI save creates the JFFS file, which always takes precedence. */
		if (errno == ENOENT && nvram_match("qca_merlin_vpn_migrated", "1"))
			strlcpy(buffer, nvram_safe_get("vpndirector_rulelist"), bufferlen);
		else
#endif
		buffer[0] = '\0';
	} else {
		buffer[datalen] = '\0';
	}

	if (unit == -1)
		return buffer;

	buffer_tmp = buffer_tmp2 = strdup(buffer);
	buffer[0] = '\0';

	while (buffer_tmp && (rule = strsep(&buffer_tmp2, "<")) != NULL) {
		if((vstrsep(rule, ">", &enable, &desc, &src, &dst, &target)) != 5)
			continue;

		snprintf(entry, sizeof(entry),"<%s>%s>%s>%s>%s",enable, desc, src, dst, target);

		if (unit == 0 && !strcmp(target, "WAN")) {
			strlcat(buffer, entry, bufferlen);
		}
		else if (unit != 0 && !strncmp(target, "OVPN", 4) &&
		    strlen(target) > 4 &&
		    atoi(&target[4]) == unit &&
		    proto == VPNDIR_PROTO_OPENVPN) {
			strlcat(buffer, entry, bufferlen);
		}
#ifdef RTCONFIG_WIREGUARD
		else if (unit != 0 && !strncmp(target, "WGC", 3) &&
		    strlen(target) > 3 &&
		    atoi(&target[3]) == unit &&
		    proto == VPNDIR_PROTO_WIREGUARD) {
			strlcat(buffer, entry, bufferlen);
		}
#endif
	}
	free(buffer_tmp);

	return buffer;
}


int amvpn_set_policy_rules(char* buffer)
{
#ifdef RTCONFIG_SOC_IPQ53XX
	char filename[128], temporary[160];
	const char *next = buffer;
	size_t left = strlen(buffer);
	ssize_t written;
	int fd = -1, dir = -1, created = 0, result = -1, saved_errno;

	if (mkdir(OVPN_FS_PATH, S_IRWXU) && errno != EEXIST)
		return -1;
	/* Persist the directory entry too if this save created openvpn. */
	dir = open("/jffs", O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
	if (dir < 0)
		return -1;
	if (fsync(dir))
		goto done;
	close(dir);
	dir = open(OVPN_FS_PATH, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
	if (dir < 0)
		return -1;
	snprintf(filename, sizeof(filename), "%s/vpndirector_rulelist", OVPN_FS_PATH);
	snprintf(temporary, sizeof(temporary), "%s/.vpndirector_rulelist.XXXXXX", OVPN_FS_PATH);
	fd = mkstemp(temporary);
	if (fd < 0)
		goto done;
	created = 1;
	if (fchmod(fd, S_IRUSR | S_IWUSR))
		goto done;
	while (left) {
		written = write(fd, next, left);
		if (written < 0 && errno == EINTR)
			continue;
		if (written <= 0) {
			if (!written) errno = EIO;
			goto done;
		}
		next += written;
		left -= written;
	}
	if (fsync(fd))
		goto done;
	if (close(fd)) {
		fd = -1;
		goto done;
	}
	fd = -1;
	/* Publish only a complete file; a failed first save must not mask the
	 * migration fallback with a truncated or empty destination. */
	if (rename(temporary, filename) || fsync(dir))
		goto done;
	/* The caller commits this retirement after successful publication. */
	result = nvram_unset("vpndirector_rulelist");
done:
	saved_errno = errno;
	if (fd >= 0) close(fd);
	if (created) unlink(temporary);
	close(dir);
	errno = saved_errno;
	return result;
#else
	char filename[128];

	if (!d_exists(OVPN_FS_PATH))
		mkdir(OVPN_FS_PATH, S_IRWXU);

	snprintf(filename, sizeof(filename), "%s/vpndirector_rulelist", OVPN_FS_PATH);
	if (f_write(filename, buffer, strlen(buffer), 0, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) < 0)
		return -1;

	return 0;
#endif
}


void amvpn_clear_exclusive_dns(int unit, vpndir_proto_t proto)
{
	char buffer[32];
	char filename[32];

	if (proto == VPNDIR_PROTO_OPENVPN) {
		snprintf(filename, sizeof (filename), "/etc/openvpn/client%d/dns.sh", unit);
		unit = unit + WG_CLIENT_MAX;
	}
#ifdef RTCONFIG_WIREGUARD
	else if (proto == VPNDIR_PROTO_WIREGUARD) {
		snprintf(filename, sizeof (filename), "/etc/wg/dns%d.sh", unit);
	}
#endif

	sprintf(buffer, "DNSVPN%d", unit);
	eval("/usr/sbin/iptables", "-t", "nat", "-D", "PREROUTING", "-p", "udp", "-m", "udp", "--dport", "53", "-j", buffer);
	eval("/usr/sbin/iptables", "-t", "nat", "-D", "PREROUTING", "-p", "tcp", "-m", "tcp", "--dport", "53", "-j", buffer);
	eval("/usr/sbin/iptables", "-t", "nat", "-F", buffer);
	eval("/usr/sbin/iptables", "-t", "nat", "-X", buffer);

	if (f_exists(filename))
		unlink(filename);
}


// Recreate the port 53 PREROUTING rules to ensure they are in the correct order (OVPN1 first, OVPN5 last, followed by WGC)
void amvpn_update_exclusive_dns_rules()
{
	int unit;
	char buffer[100];

#ifdef RTCONFIG_WIREGUARD
	for (unit = WG_CLIENT_MAX; unit > 0; unit--) {
		snprintf(buffer, sizeof (buffer), "/etc/wg/dns%d.sh", unit);
		if (f_exists(buffer)) {
			// Remove and re-add to ensure proper order
			snprintf(buffer, sizeof (buffer), "DNSVPN%d", unit);

			eval("/usr/sbin/iptables", "-t", "nat", "-D", "PREROUTING", "-p", "udp", "-m", "udp", "--dport", "53", "-j", buffer);
			eval("/usr/sbin/iptables", "-t", "nat", "-D", "PREROUTING", "-p", "tcp", "-m", "tcp", "--dport", "53", "-j", buffer);

			eval("/usr/sbin/iptables", "-t", "nat", "-I", "PREROUTING", "-p", "udp", "-m", "udp", "--dport", "53", "-j", buffer);
			eval("/usr/sbin/iptables", "-t", "nat", "-I", "PREROUTING", "-p", "tcp", "-m", "tcp", "--dport", "53", "-j", buffer);
		}
	}
#endif

	for (unit = OVPN_CLIENT_MAX; unit > 0; unit--) {
		snprintf(buffer, sizeof (buffer), "/etc/openvpn/client%d/dns.sh", unit);
		if (f_exists(buffer)) {
			// Remove and re-add to ensure proper order
			snprintf(buffer, sizeof (buffer), "DNSVPN%d", unit + WG_CLIENT_MAX);

			eval("/usr/sbin/iptables", "-t", "nat", "-D", "PREROUTING", "-p", "udp", "-m", "udp", "--dport", "53", "-j", buffer);
			eval("/usr/sbin/iptables", "-t", "nat", "-D", "PREROUTING", "-p", "tcp", "-m", "tcp", "--dport", "53", "-j", buffer);

			eval("/usr/sbin/iptables", "-t", "nat", "-I", "PREROUTING", "-p", "udp", "-m", "udp", "--dport", "53", "-j", buffer);
			eval("/usr/sbin/iptables", "-t", "nat", "-I", "PREROUTING", "-p", "tcp", "-m", "tcp", "--dport", "53", "-j", buffer);
		}
	}
}


void ovpn_set_exclusive_dns(int unit) {
	char rules[8000], wanrules[8000], buffer[64], buffer2[64], server[20], iface_match[8];
	char *nvp, *entry;
	char *src, *dst, *iface, *desc, *enable, *netptr;
	struct in_addr addr;
	int mask;
	char prefix[32];

	FILE *fp_resolv, *fp_dns;

	snprintf(buffer, sizeof (buffer), "/etc/openvpn/client%d/resolv.dnsmasq", unit);
	fp_resolv = fopen(buffer, "r");
	snprintf(buffer, sizeof (buffer), "/etc/openvpn/client%d/dns.sh", unit);
	fp_dns = fopen(buffer, "w");

	if (!fp_resolv || !fp_dns) {
		if (fp_resolv)
			fclose(fp_resolv);
		if (fp_dns)
			fclose(fp_dns);
		return;
	}

	fprintf(fp_dns, "#!/bin/sh\n"
	                "/usr/sbin/iptables -t nat -N DNSVPN%d\n",
	                 unit + WG_CLIENT_MAX);

	sprintf(prefix, "vpn_client%d_", unit);

	if (nvram_pf_get_int(prefix, "rgw") == OVPN_RGW_ALL) {
		// Iterate through servers
		while (fgets(buffer2, sizeof(buffer2), fp_resolv) != NULL) {
			if (sscanf(buffer2, "server=%16s", server) != 1)
				continue;

			if (!inet_aton(server, &addr))
				continue;

			fprintf(fp_dns, "/usr/sbin/iptables -t nat -A DNSVPN%d -j DNAT --to-destination %s\n", unit + WG_CLIENT_MAX, server);
			logmessage("openvpn", "Forcing all to use DNS server %s (OpenVPN client %d is set to Exclusive DNS mode)", server, unit);
			// Only configure first server found, as others would never get used
			break;
		}
	} else if (nvram_pf_get_int(prefix, "rgw") == OVPN_RGW_POLICY) {
		amvpn_get_policy_rules(unit, rules, sizeof (rules), VPNDIR_PROTO_OPENVPN);
		amvpn_get_policy_rules(0, wanrules, sizeof (wanrules), VPNDIR_PROTO_OPENVPN);
		strlcat(rules, wanrules, sizeof (rules));

		nvp = rules;

		snprintf(iface_match, sizeof (iface_match), "OVPN%d", unit);

		while ((entry = strsep(&nvp, "<")) != NULL) {
			if (vstrsep(entry, ">", &enable, &desc, &src, &dst, &iface) != 5)
				continue;

			if (atoi(&enable[0]) == 0)
				continue;

			if (*src && !*dst) {
				strlcpy(buffer, src, sizeof(buffer));

				if ((netptr = strchr(buffer, '/'))) {
					*netptr = '\0';
					mask = atoi(&netptr[1]);
				} else {
					mask = 32;
				}

				if ((mask >= 0) &&
				    (mask <= 32) &&
				    (inet_aton(buffer, &addr))) {
					if (!strcmp(iface, iface_match)) {

						// Iterate through servers
						rewind(fp_resolv);
						while (fgets(buffer2, sizeof(buffer2), fp_resolv) != NULL) {
							if (sscanf(buffer2, "server=%16s", server) != 1)
								continue;

							if (!inet_aton(server, &addr))
								continue;

			                                fprintf(fp_dns, "/usr/sbin/iptables -t nat -A DNSVPN%d -s %s -j DNAT --to-destination %s\n", unit + WG_CLIENT_MAX, src, server);
			                                logmessage("openvpn", "Forcing %s to use DNS server %s for OVPNC%d", src, server, unit);
							// Only configure first server found, as others would never get used
							break;
						}
		                        } else if (!strcmp(iface, "WAN")) {
		                                fprintf(fp_dns, "/usr/sbin/iptables -t nat -I DNSVPN%d -s %s -j RETURN\n", unit + WG_CLIENT_MAX, src);
		                                logmessage("openvpn", "Excluding %s from forced DNS routing for OVPNC%d", src, unit);
		                        }
				}
			}
		}
	}

	fprintf(fp_dns, "/usr/sbin/iptables -t nat -I PREROUTING -p udp -m udp --dport 53 -j DNSVPN%d\n"
	                "/usr/sbin/iptables -t nat -I PREROUTING -p tcp -m tcp --dport 53 -j DNSVPN%d\n",
	                 unit + WG_CLIENT_MAX, unit + WG_CLIENT_MAX);

	fclose(fp_resolv);
	fclose(fp_dns);
	sprintf(buffer, "/etc/openvpn/client%d/dns.sh", unit);
	if (f_exists(buffer)) {
		chmod(buffer, 0755);
		eval(buffer);
	}
}


#ifdef RTCONFIG_WIREGUARD
void wgc_set_exclusive_dns(int unit) {
	char rules[8000], wanrules[8000], buffer[64], server[32] = {0}, iface_match[8];
	char *nvp, *entry;
	char *src, *dst, *iface, *desc, *enable, *netptr;
	struct in_addr addr;
	int mask;
	char prefix[32];
	char dns[128] = {0};
	FILE *fp_dns;
	char scriptname[32];

	snprintf(buffer, sizeof (buffer), "wgc%d_enable", unit);
	if (nvram_get_int(buffer) != 1)
		return;

	snprintf(scriptname, sizeof (scriptname), "/etc/wg/dns%d.sh", unit);
	fp_dns = fopen(scriptname, "w");
	if (!fp_dns)
		return;

	snprintf(prefix, sizeof(prefix), "wgc%d_", unit);
	snprintf(dns, sizeof(dns), "%s", nvram_pf_safe_get(prefix, "dns"));

	fprintf(fp_dns, "#!/bin/sh\n"
	                "/usr/sbin/iptables -t nat -N DNSVPN%d\n",
	                 unit);

	amvpn_get_policy_rules(unit, rules, sizeof (rules), VPNDIR_PROTO_WIREGUARD);
	amvpn_get_policy_rules(0, wanrules, sizeof (wanrules), VPNDIR_PROTO_WIREGUARD);
	strlcat(rules, wanrules, sizeof (rules));

	nvp = rules;

	snprintf(iface_match, sizeof (iface_match), "WGC%d", unit);

	while ((entry = strsep(&nvp, "<")) != NULL) {
		if (vstrsep(entry, ">", &enable, &desc, &src, &dst, &iface) != 5)
			continue;

		if (atoi(&enable[0]) == 0)
			continue;

		if (*src && !*dst) {
			strlcpy(buffer, src, sizeof(buffer));

			if ((netptr = strchr(buffer, '/'))) {
				*netptr = '\0';
				mask = atoi(&netptr[1]);
			} else {
				mask = 32;
			}

			if ((mask >= 0) &&
			    (mask <= 32) &&
			    (inet_aton(buffer, &addr))) {
				if (!strcmp(iface, iface_match)) {
					if (dns[0] != '\0') {
						char *next = NULL;
						char *p = NULL;

						foreach_44 (server, dns, next) {
							if ((p = strchr(server, '/')) != NULL)
								*p = '\0';

							fprintf(fp_dns, "/usr/sbin/iptables -t nat -A DNSVPN%d -s %s -j DNAT --to-destination %s\n", unit, src, server);
							logmessage("wireguard", "Forcing %s to use DNS server %s for WGC%d", src, server, unit);

							// currently added by rc/wireguard.c - should I add it to the correct table, like Fusion?
							//eval("ip", "route", "add", server, "dev", ifname);
						}
					}
	                        } else if (!strcmp(iface, "WAN")) {
	                                fprintf(fp_dns, "/usr/sbin/iptables -t nat -I DNSVPN%d -s %s -j RETURN\n", unit, src);
	                                logmessage("wireguard", "Excluding %s from forced DNS routing for WGC%d", src, unit);
	                        }
			}
		}
	}

	fprintf(fp_dns, "/usr/sbin/iptables -t nat -I PREROUTING -p udp -m udp --dport 53 -j DNSVPN%d\n"
	                "/usr/sbin/iptables -t nat -I PREROUTING -p tcp -m tcp --dport 53 -j DNSVPN%d\n",
	                 unit, unit);

	fclose(fp_dns);
	chmod(scriptname, 0755);
	eval(scriptname);
}
#endif


#if defined(RTCONFIG_SOC_IPQ53XX) && defined(RTCONFIG_IPV6)
/* The IPQ53xx kernel has no IPv6 policy tables. Until dual-stack policy
 * routing exists, protect whole-interface assignments by blocking forwarded
 * IPv6, including while the IPv4 tunnel is up. Never guess an IPv6 identity
 * from an IPv4 source/destination rule.
 */
static int amvpn_ipv6_enforced(vpndir_proto_t proto, int unit)
{
	char prefix[32];
	int rgw;

	if (proto == VPNDIR_PROTO_OPENVPN && unit >= 1 && unit <= OVPN_CLIENT_MAX) {
		snprintf(prefix, sizeof(prefix), "vpn_client%d_", unit);
		rgw = nvram_pf_get_int(prefix, "rgw");
		return nvram_pf_get_int(prefix, "enforce") && ovpn_is_client_enabled(unit) &&
		       (rgw == OVPN_RGW_ALL || rgw == OVPN_RGW_POLICY);
	}
#ifdef RTCONFIG_WIREGUARD
	if (proto == VPNDIR_PROTO_WIREGUARD && unit >= 1 && unit <= WG_CLIENT_MAX) {
		snprintf(prefix, sizeof(prefix), "wgc%d_", unit);
		return nvram_pf_get_int(prefix, "enforce") && nvram_pf_get_int(prefix, "enable");
	}
#endif
	return 0;
}

static int amvpn_ipv4_wildcard(const char *address)
{
	return !*address || !strcmp(address, "0.0.0.0") || !strcmp(address, "0.0.0.0/0");
}

static int amvpn_ipv6_catchall(vpndir_proto_t proto, int unit)
{
	char buffer[8000], *next, *rule;
	char *enable, *desc, *src, *dst, *target;

	amvpn_get_policy_rules(unit, buffer, sizeof(buffer), proto);
	next = buffer;
	while ((rule = strsep(&next, "<")) != NULL) {
		if (vstrsep(rule, ">", &enable, &desc, &src, &dst, &target) == 5 &&
		    atoi(enable) && strcmp(target, "WAN") &&
		    amvpn_ipv4_wildcard(src) && amvpn_ipv4_wildcard(dst))
			return 1;
	}
	return 0;
}

static void amvpn_ipv6_drop(FILE *fp, const char *ifname)
{
	const char *p;
	if (!ifname || !*ifname || strlen(ifname) >= IFNAMSIZ)
		return;
	/* Interface names are emitted into an iptables-restore file. */
	for (p = ifname; *p; ++p)
		if (!isalnum((unsigned char)*p) && *p != '_' && *p != '-' && *p != '.')
			return;
	fprintf(fp, "-A VPN6KS -i %s -j DROP\n", ifname);
}

/* Called inside rc's full filter transaction, after other FORWARD rules.
 * Insertion at the head also protects already-established connections.
 */
void amvpn_write_ipv6_killswitch(FILE *fp)
{
	int unit, global = 0, catchall = 0;
	char prefix[32];
#ifdef RTCONFIG_MULTILAN_CFG
	MTLAN_T *pmtl;
	VPN_VPNX_T vpnx;
	size_t mtl_sz = 0;
	int i, j, protect;
#endif

	for (unit = 1; unit <= OVPN_CLIENT_MAX; ++unit) {
		if (!amvpn_ipv6_enforced(VPNDIR_PROTO_OPENVPN, unit))
			continue;
		snprintf(prefix, sizeof(prefix), "vpn_client%d_", unit);
		if (nvram_pf_get_int(prefix, "rgw") == OVPN_RGW_ALL)
			global = 1;
		else if (amvpn_ipv6_catchall(VPNDIR_PROTO_OPENVPN, unit))
			catchall = 1;
	}
#ifdef RTCONFIG_WIREGUARD
	for (unit = 1; unit <= WG_CLIENT_MAX; ++unit)
		if (amvpn_ipv6_enforced(VPNDIR_PROTO_WIREGUARD, unit) &&
		    amvpn_ipv6_catchall(VPNDIR_PROTO_WIREGUARD, unit))
			catchall = 1;
#endif

	fprintf(fp, ":VPN6KS - [0:0]\n");
	if (global || catchall)
		amvpn_ipv6_drop(fp, nvram_safe_get("lan_ifname"));
#ifdef RTCONFIG_MULTILAN_CFG
	pmtl = (MTLAN_T *)INIT_MTLAN(sizeof(MTLAN_T));
	if (pmtl) {
		get_mtlan(pmtl, &mtl_sz);
		/* Include the default network if it has an explicit VPN assignment. */
		for (i = 0; i < mtl_sz; ++i) {
			if (!pmtl[i].enable || !*pmtl[i].nw_t.ifname)
				continue;
			if ((global || catchall) && !strcmp(pmtl[i].nw_t.ifname, nvram_safe_get("lan_ifname")))
				continue;
			protect = catchall || (global && pmtl[i].sdn_t.vpnc_idx == 0);
			protect |= qca_vpn_sdn_deferred(pmtl[i].sdn_t.sdn_idx, pmtl[i].sdn_t.vpnc_idx);
			if (pmtl[i].sdn_t.vpnc_idx && get_vpnx_by_vpnc_idx(&vpnx, pmtl[i].sdn_t.vpnc_idx)) {
				if (vpnx.proto == VPN_PROTO_OVPN)
					protect |= amvpn_ipv6_enforced(VPNDIR_PROTO_OPENVPN, vpnx.unit);
#ifdef RTCONFIG_WIREGUARD
				else if (vpnx.proto == VPN_PROTO_WG)
					protect |= amvpn_ipv6_enforced(VPNDIR_PROTO_WIREGUARD, vpnx.unit);
#endif
			}
			if (!protect)
				continue;
			/* Duplicate network records must not duplicate the rule. */
			for (j = 0; j < i; ++j)
				if (pmtl[j].enable && pmtl[j].sdn_t.vpnc_idx == pmtl[i].sdn_t.vpnc_idx &&
				    !strcmp(pmtl[j].nw_t.ifname, pmtl[i].nw_t.ifname))
					break;
			if (j == i)
				amvpn_ipv6_drop(fp, pmtl[i].nw_t.ifname);
		}
		FREE_MTLAN((void *)pmtl);
	}
#endif
	fprintf(fp, "-I FORWARD 1 -j VPN6KS\n");
}

/* Recompute all units from configuration: stopping one tunnel must not remove
 * another's protection. --noflush with a declared user chain replaces that
 * chain atomically while retaining the rest of the filter table.
 */
void amvpn_refresh_ipv6_killswitch(void)
{
	char path[] = "/tmp/vpn6ks.XXXXXX";
	int fd, lock, ret;
	FILE *fp;

	lock = file_lock("vpn6ks");
	fd = mkstemp(path);
	if (fd < 0) {
		file_unlock(lock);
		return;
	}
	fp = fdopen(fd, "w");
	if (!fp) {
		close(fd);
		unlink(path);
		file_unlock(lock);
		return;
	}
	fprintf(fp, "*filter\n");
	if (!eval("ip6tables", "-C", "FORWARD", "-j", "VPN6KS"))
		fprintf(fp, "-D FORWARD -j VPN6KS\n");
	amvpn_write_ipv6_killswitch(fp);
	fprintf(fp, "COMMIT\n");
	ret = ferror(fp);
	if (fclose(fp))
		ret = 1;
	if (ret || eval("ip6tables-restore", "--noflush", path))
		logmessage("vpndirector", "Failed to update IPv6 kill switch");
	unlink(path);
	file_unlock(lock);
}
#endif


/* Will remove all rules for a specific unit
   Will also remove all rules for a specific SDN interface,
   or search for any SDN that is tied to specified unit and remove
   the rules.
*/

void amvpn_clear_killswitch_rules(vpndir_proto_t proto, int unit, char *sdn_ifname) {
#ifdef RTCONFIG_SOC_IPQ53XX
	if (unit >= 1) {
		qca_refresh_vpn_rules(unit, proto, 1, 1, sdn_ifname);
		return;
	}
#endif
	int prio, verb = 3;
	char buffer[256], prio_str[6];
#ifdef RTCONFIG_MULTILAN_CFG
	int vpnc_idx, i;
	MTLAN_T *pmtl = NULL;
	size_t mtl_sz = 0;
#endif

	if (proto == VPNDIR_PROTO_OPENVPN) {
		prio = VPNDIR_PRIO_KS_OPENVPN + unit - 1;

		snprintf(buffer, sizeof (buffer), "vpn_client%d_verb", unit);
		verb = nvram_get_int(buffer);

		// Delete as many priority "prio" as there are rules of that priority
		snprintf(buffer, sizeof (buffer), "ip rule show priority %d | while read -r rule; do ip rule del priority %d; done", prio, prio);
		if (verb > 3)
			logmessage("openvpn-routing", "Clearing killswitch for OpenVPN unit %d", unit);
		system(buffer);
#ifdef RTCONFIG_WIREGUARD
	} else if (proto == VPNDIR_PROTO_WIREGUARD) {
		prio = VPNDIR_PRIO_KS_WIREGUARD + unit - 1;

		// Delete as many priority "prio" as there are rules of that priority
		snprintf(buffer, sizeof (buffer), "ip rule show priority %d | while read -r rule; do ip rule del priority %d; done", prio, prio);
		if (verb > 3)
			logmessage("openvpn-routing", "Clearing killswitch for WireGuard unit %d", unit);
		system(buffer);
#endif
	}


#ifdef RTCONFIG_MULTILAN_CFG
	prio = VPNDIR_PRIO_KS_SDN;
	snprintf(prio_str, sizeof (prio_str), "%d", prio);

	// SDN specified interface
	if (sdn_ifname && *sdn_ifname) {
		eval("ip", "rule", "del", "priority", prio_str, "iif", sdn_ifname);
		if (verb > 3)
			logmessage("openvpn-routing", "Clearing killswitch for SDN %s", sdn_ifname);

	} else if (unit == -1) {	// Remove all existing SDN rules
		snprintf(buffer, sizeof (buffer), "ip rule show priority %d | while read -r rule; do ip rule del priority %d; done", prio, prio);
		if (verb > 3)
			logmessage("openvpn-routing", "Clearing killswitch for all SDN");
		system(buffer);

	} else {	// Only by unit, so walk through whole SDN list for matching vpnc_idx
		vpnc_idx = get_vpnc_idx_by_proto_unit((proto == VPNDIR_PROTO_OPENVPN ? VPN_PROTO_OVPN : VPN_PROTO_WG), unit);
		pmtl = (MTLAN_T *)INIT_MTLAN(sizeof(MTLAN_T));
		if (pmtl) {
			get_mtlan(pmtl, &mtl_sz);

			for (i = 1; i < mtl_sz; ++i) {	// Skip first (Default) SDN
				if (pmtl[i].sdn_t.vpnc_idx == vpnc_idx) {
					eval("ip", "rule", "del", "priority", prio_str,  "iif", pmtl[i].nw_t.ifname);
					if (verb > 3)
						logmessage("openvpn-routing", "Clearing killswitch for SDN %s", pmtl[i].nw_t.ifname);
				}
			}
			FREE_MTLAN((void *)pmtl);
		}

	}
#endif
}


#ifdef RTCONFIG_SOC_IPQ53XX
/* Keep the same selectors as the Director rule, including destination-only and
 * catch-all policies. An omitted selector means all, just as in routing rules.
 */
static void amvpn_add_policy_killswitch(char *src, char *dst, char *prio)
{
	char *destination = (*dst && strcmp(dst, "0.0.0.0")) ? dst : "all";
#ifdef RTCONFIG_MULTILAN_CFG
	MTLAN_T *pmtl;
	size_t mtl_sz = 0;
	int i, j;
#endif

	if (*src && strcmp(src, "0.0.0.0") && strcmp(src, "0.0.0.0/0")) {
		eval("ip", "rule", "add", "from", src, "to", destination,
		     "priority", prio, "prohibit");
		return;
	}

	/* Wildcard source policies protect forwarded LAN traffic. Blocking locally
	 * originated traffic here would prevent the tunnel from reconnecting.
	 */
	eval("ip", "rule", "add", "iif", nvram_safe_get("lan_ifname"),
	     "to", destination, "priority", prio, "prohibit");
#ifdef RTCONFIG_MULTILAN_CFG
	pmtl = (MTLAN_T *)INIT_MTLAN(sizeof(MTLAN_T));
	if (pmtl) {
		get_mtlan(pmtl, &mtl_sz);
		for (i = 1; i < mtl_sz; ++i) {
			if (!pmtl[i].enable || !*pmtl[i].nw_t.ifname ||
			    !strcmp(pmtl[i].nw_t.ifname, nvram_safe_get("lan_ifname")))
				continue;
			for (j = 1; j < i; ++j)
				if (pmtl[j].enable && !strcmp(pmtl[j].nw_t.ifname, pmtl[i].nw_t.ifname))
					break;
			if (j < i)
				continue;
			eval("ip", "rule", "add", "iif", pmtl[i].nw_t.ifname,
			     "to", destination, "priority", prio, "prohibit");
		}
		FREE_MTLAN((void *)pmtl);
	}
#endif
}

#endif

void amvpn_set_killswitch_rules(vpndir_proto_t proto, int unit, char *sdn_ifname) {
#ifdef RTCONFIG_SOC_IPQ53XX
	qca_refresh_vpn_rules(unit, proto, 1, 0, sdn_ifname);
	return;
#endif
	char buffer[8000], prefix[32], prio_str[6];
	char *buffer_tmp, *buffer_tmp2, *rule;
	char *enable, *desc, *target, *src, *dst;
	int killswitch, rgw, verb = 3;
#ifdef RTCONFIG_MULTILAN_CFG
	int vpnc_idx, i;
	MTLAN_T *pmtl = NULL;
	size_t mtl_sz = 0;
#endif
#ifdef RTCONFIG_WIREGUARD
	int enabled;
#endif

#if defined(RTCONFIG_SOC_IPQ53XX) && defined(RTCONFIG_IPV6)
	amvpn_refresh_ipv6_killswitch();
#endif

	if (proto == VPNDIR_PROTO_OPENVPN) {
		// Clear existing rules
		amvpn_clear_killswitch_rules(VPNDIR_PROTO_OPENVPN, unit, sdn_ifname);

		snprintf(prio_str, sizeof(prio_str), "%d", VPNDIR_PRIO_KS_OPENVPN + unit - 1);

                snprintf(prefix, sizeof(prefix), "vpn_client%d_", unit);
                killswitch = nvram_pf_get_int(prefix, "enforce");
                rgw = nvram_pf_get_int(prefix, "rgw");
		verb = nvram_pf_get_int(prefix, "verb");

		if (killswitch == 0 || ovpn_is_client_enabled(unit) == 0)
			return;

		if (rgw == OVPN_RGW_ALL) {
			eval("ip", "rule", "add", "from", "all", "iif", nvram_safe_get("lan_ifname"), "priority", prio_str, "prohibit");
			if (verb > 3)
				logmessage("openvpn-routing","Setting global killswitch rule for OpenVPN client %d", unit);
		} else if (rgw == OVPN_RGW_POLICY) {
			/* Do VPNDirector */
			amvpn_get_policy_rules(unit, buffer, sizeof (buffer), proto);
			buffer_tmp = buffer_tmp2 = strdup(buffer);

			while (buffer_tmp && (rule = strsep(&buffer_tmp2, "<")) != NULL) {
				if((vstrsep(rule, ">", &enable, &desc, &src, &dst, &target)) != 5)
					continue;

				if (!atoi(&enable[0]))
					continue;

				if (!strcmp(target,"WAN"))
					continue;

#ifdef RTCONFIG_SOC_IPQ53XX
				if (!strncmp(target, "OVPN", 4)) {
					// Create deny rule
					amvpn_add_policy_killswitch(src, dst, prio_str);
#else
				if (!strncmp(target, "OVPN", 4) && *src && strcmp(src, "0.0.0.0")) {
					// Create deny rule
					eval("ip", "rule", "add", "from", src, "priority", prio_str, "prohibit");
#endif
					if (verb > 3)
						logmessage("openvpn-routing","Setting killswitch rule for %s", src);
				}
			}
			free(buffer_tmp);
		}
#ifdef RTCONFIG_MULTILAN_CFG
		if (rgw == OVPN_RGW_ALL || rgw == OVPN_RGW_POLICY) {
			/* Do SDN - use supplied interface if available */
			if (sdn_ifname && *sdn_ifname) {
				snprintf(prio_str, sizeof(prio_str), "%d", VPNDIR_PRIO_KS_SDN);
				eval("ip", "rule", "add", "from", "all", "iif", sdn_ifname, "priority", prio_str, "prohibit");
				if (verb > 3)
					logmessage("openvpn-routing", "Setting killswitch rule for SDN %s", sdn_ifname);
			} else {	/* Locate SDN instance that might be using this VPN client */
				vpnc_idx = get_vpnc_idx_by_proto_unit(VPN_PROTO_OVPN, unit);
				pmtl = (MTLAN_T *)INIT_MTLAN(sizeof(MTLAN_T));
				if (pmtl) {
					get_mtlan(pmtl, &mtl_sz);

					for (i = 1; i < mtl_sz; ++i) {	// Skip first (Default) SDN
						if ((rgw == OVPN_RGW_POLICY && pmtl[i].sdn_t.vpnc_idx == vpnc_idx) ||
						    (rgw == OVPN_RGW_ALL && pmtl[i].sdn_t.vpnc_idx == 0)) {
#ifdef RTCONFIG_SOC_IPQ53XX
							/* Assigned SDNs own a separate guard from Director policy. */
							if (rgw == OVPN_RGW_POLICY)
								snprintf(prio_str, sizeof(prio_str), "%d", VPNDIR_PRIO_KS_SDN);
#endif
							eval("ip", "rule", "add", "from", "all", "priority", prio_str,  "iif", pmtl[i].nw_t.ifname, "prohibit");
							if (verb > 3)
								logmessage("openvpn-routing", "Setting killswitch rule for SDN %s", pmtl[i].nw_t.ifname);
						}
					}
					FREE_MTLAN((void *)pmtl);
				}
			}
		}
#endif
#ifdef RTCONFIG_WIREGUARD
	} else if (proto == VPNDIR_PROTO_WIREGUARD) {
		// Clear existing rules
		amvpn_clear_killswitch_rules(VPNDIR_PROTO_WIREGUARD, unit, sdn_ifname);

		snprintf(prio_str, sizeof(prio_str), "%d", VPNDIR_PRIO_KS_WIREGUARD + unit - 1);

		snprintf(prefix, sizeof(prefix), "wgc%d_", unit);
		killswitch = nvram_pf_get_int(prefix, "enforce");

		enabled = nvram_pf_get_int(prefix, "enable");
		if (killswitch == 0 || enabled == 0)
			return;

		/* Do VPNDirector */
		amvpn_get_policy_rules(unit, buffer, sizeof (buffer), proto);
		buffer_tmp = buffer_tmp2 = strdup(buffer);

		while (buffer_tmp && (rule = strsep(&buffer_tmp2, "<")) != NULL) {
			if((vstrsep(rule, ">", &enable, &desc, &src, &dst, &target)) != 5)
				continue;

			if (!atoi(&enable[0]))
				continue;

			if (!strcmp(target,"WAN"))
				continue;

#ifdef RTCONFIG_SOC_IPQ53XX
			if (!strncmp(target, "WGC", 3)) {
				// Create deny rule
				amvpn_add_policy_killswitch(src, dst, prio_str);
#else
			if (!strncmp(target, "WGC", 3) && *src && strcmp(src, "0.0.0.0")) {
				// Create deny rule
				eval("ip", "rule", "add", "from", src, "priority", prio_str, "prohibit");
#endif
				if (verb > 3)
					logmessage("openvpn-routing","Setting killswitch rule for %s", src);
			}
		}
		free(buffer_tmp);
#ifdef RTCONFIG_MULTILAN_CFG
		/* Do SDN - use supplied interface if available */
		if (sdn_ifname && *sdn_ifname) {
			snprintf(prio_str, sizeof(prio_str), "%d", VPNDIR_PRIO_KS_SDN);
			eval("ip", "rule", "add", "from", "all", "iif", sdn_ifname, "priority", prio_str, "prohibit");
			if (verb > 3)
				logmessage("openvpn-routing", "Setting killswitch rule for SDN %s", sdn_ifname);
		} else {	/* Locate SDN instance that might be using this VPN client */
			vpnc_idx = get_vpnc_idx_by_proto_unit(VPN_PROTO_WG, unit);
			pmtl = (MTLAN_T *)INIT_MTLAN(sizeof(MTLAN_T));
			if (pmtl) {
				get_mtlan(pmtl, &mtl_sz);

				for (i = 1; i < mtl_sz; ++i) {	// Skip first (Default) SDN
					if (pmtl[i].sdn_t.vpnc_idx == vpnc_idx) {
#ifdef RTCONFIG_SOC_IPQ53XX
						snprintf(prio_str, sizeof(prio_str), "%d", VPNDIR_PRIO_KS_SDN);
#endif
						eval("ip", "rule", "add", "from", "all", "priority", prio_str,  "iif", pmtl[i].nw_t.ifname, "prohibit");
						if (verb > 3)
							logmessage("openvpn-routing", "Setting killswitch rule for SDN %s", pmtl[i].nw_t.ifname);
					}
				}
				FREE_MTLAN((void *)pmtl);
			}
		}
#endif
#endif
	}
}

void amvpn_set_kilswitch_rules_all() {
	int i;
		logmessage("openvpn-routing", "Applying all killswitches");
#ifdef RTCONFIG_WIREGUARD
	for (i = WG_CLIENT_MAX; i > 0; i--) {
		amvpn_set_killswitch_rules(VPNDIR_PROTO_WIREGUARD, i, NULL);
	}
#endif
	for (i = OVPN_CLIENT_MAX; i > 0; i --) {
		amvpn_set_killswitch_rules(VPNDIR_PROTO_OPENVPN, i, NULL);
	}
}
