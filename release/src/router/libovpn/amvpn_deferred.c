/* Quarantine unresolved stock Fusion selectors without changing their NVRAM. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <shared.h>
#include <qca_vpn_deferred.h>
#include "amvpn_routing.h"
#include "amvpn_deferred.h"
#include "openvpn_config.h"
#include "openvpn_control.h"

#define DEFERRED_DNS_CHAIN "QCADEFDNS"
#define DEFERRED_NETWORK_CHAIN "QCADEFNET"
#define DEFERRED_NETWORK_DNS_CHAIN "QCADEFWIDE"
#define DEFERRED_GUARD 12225
#define DEFERRED_TRANSITION_GUARD 90
#define DEFERRED_PROTOCOL_FIRST 240
#define DEFERRED_PROTOCOL_LAST 249


struct deferred_rule {
	int family, priority, table, retained, created, protocol, opaque, removed;
	char src[INET6_ADDRSTRLEN + 4], dst[INET6_ADDRSTRLEN + 4], iif[IFNAMSIZ];
	/* Rule deletion compares stored address bytes, including host bits. */
	char captured_src[INET6_ADDRSTRLEN + 4], captured_dst[INET6_ADDRSTRLEN + 4];
	struct deferred_rule *next;
};

struct deferred_scope {
	struct deferred_rule *guards, *dns, *unsafe, *exceptions, *networks, *network_dns, *previous_networks;
	MTLAN_T *mtlan;
	size_t count;
	int unresolved, invalid_ingress;
};

static int collect_scope(struct deferred_scope *scope);
static int valid_ingress(const char *name);

/* The default LAN remains a known ingress when the SDN list has no LAN0 row. */
static const MTLAN_T *scope_sdn(const struct deferred_scope *scope, size_t index, MTLAN_T *fallback)
{
	size_t i;
	if (index < scope->count) return &scope->mtlan[index];
	if (index != scope->count || !valid_ingress(nvram_safe_get("lan_ifname"))) return NULL;
	for (i = 0; i < scope->count; ++i)
		if (!scope->mtlan[i].sdn_t.sdn_idx) return NULL;
	memset(fallback, 0, sizeof(*fallback));
	fallback->enable = 1;
	strlcpy(fallback->nw_t.ifname, nvram_safe_get("lan_ifname"), sizeof(fallback->nw_t.ifname));
	return fallback;
}

static void free_rules(struct deferred_rule *rule)
{
	struct deferred_rule *next;
	while (rule) { next = rule->next; free(rule); rule = next; }
}

static void free_scope(struct deferred_scope *scope)
{
	free_rules(scope->guards); free_rules(scope->dns); free_rules(scope->unsafe);
	free_rules(scope->exceptions);
	free_rules(scope->networks); free_rules(scope->network_dns);
	free_rules(scope->previous_networks);
	if (scope->mtlan) FREE_MTLAN((void *)scope->mtlan);
}

/* Semantic comparisons ignore host bits and equivalent host-prefix spelling. */
static int canonical_address(int family, const char *source, char *out, size_t size)
{
	unsigned char bytes[16];
	char input[INET6_ADDRSTRLEN + 4], address[INET6_ADDRSTRLEN], *slash, *end;
	long bits, maximum = family == AF_INET ? 32 : 128;
	int i;
	if (!source || !*source || !strcmp(source, "all")) { strlcpy(out, "all", size); return 0; }
	if (strlen(source) >= sizeof(input)) return -1;
	strcpy(input, source);
	slash = strchr(input, '/');
	bits = maximum;
	if (slash) {
		*slash++ = '\0';
		errno = 0; bits = strtol(slash, &end, 10);
		if (errno || !*slash || *end || bits < 0 || bits > maximum) return -1;
	}
	if (inet_pton(family, input, bytes) != 1) return -1;
	if (!bits) { strlcpy(out, "all", size); return 0; }
	for (i = 0; i < maximum / 8; ++i) {
		int remain = bits - i * 8;
		if (remain <= 0) bytes[i] = 0;
		else if (remain < 8) bytes[i] &= 0xff << (8 - remain);
	}
	if (!inet_ntop(family, bytes, address, sizeof(address))) return -1;
	if (bits == maximum) strlcpy(out, address, size);
	else snprintf(out, size, "%s/%ld", address, bits);
	return 0;
}

static int equal_rule(const struct deferred_rule *a, const struct deferred_rule *b)
{
	return a->family == b->family && a->priority == b->priority && a->table == b->table &&
		!strcmp(a->src, b->src) && !strcmp(a->dst, b->dst) && !strcmp(a->iif, b->iif);
}

static int address_covers(int family, const char *outer, const char *inner)
{
	char left[INET6_ADDRSTRLEN + 4], right[INET6_ADDRSTRLEN + 4], *slash;
	unsigned char a[16], b[16];
	int bits = family == AF_INET ? 32 : 128, other = bits, bytes;
	if (!strcmp(outer, "all")) return 1;
	if (!strcmp(inner, "all")) return 0;
	strlcpy(left, outer, sizeof(left)); strlcpy(right, inner, sizeof(right));
	if ((slash = strchr(left, '/'))) { *slash++ = 0; bits = atoi(slash); }
	if ((slash = strchr(right, '/'))) { *slash++ = 0; other = atoi(slash); }
	if (bits > other || inet_pton(family, left, a) != 1 || inet_pton(family, right, b) != 1) return 0;
	bytes = bits / 8;
	return !memcmp(a, b, bytes) && (!(bits % 8) || !((a[bytes] ^ b[bytes]) & (0xff << (8 - bits % 8))));
}

static int covers_rule(const struct deferred_rule *a, const struct deferred_rule *b)
{
	return a->family == b->family && a->priority == b->priority && a->table == b->table &&
		(!*a->iif || !strcmp(a->iif, b->iif)) && address_covers(a->family, a->src, b->src) &&
		address_covers(a->family, a->dst, b->dst);
}

static void prune_covered(struct deferred_rule **head)
{
	struct deferred_rule **next = head, *rule, *other;
	while ((rule = *next)) {
		for (other = *head; other; other = other->next)
			if (other != rule && covers_rule(other, rule)) break;
		if (other) { *next = rule->next; free(rule); }
		else next = &rule->next;
	}
}

static int append_rule(struct deferred_rule **head, int family, int priority, int table,
	const char *src, const char *dst, const char *iif)
{
	struct deferred_rule value, *item, **tail = head;
	memset(&value, 0, sizeof(value));
	value.family = family; value.priority = priority; value.table = table;
	if (canonical_address(family, src, value.src, sizeof(value.src)) ||
	    canonical_address(family, dst, value.dst, sizeof(value.dst)) ||
	    (iif && strlen(iif) >= sizeof(value.iif))) return -1;
	if (iif) strcpy(value.iif, iif);
	for (item = *head; item; item = item->next) {
		if (equal_rule(item, &value)) return 0;
		tail = &item->next;
	}
	item = malloc(sizeof(*item));
	if (!item) return -1;
	*item = value; *tail = item;
	return 0;
}

static int valid_ingress(const char *name)
{
	const unsigned char *p = (const unsigned char *)name;
	if (!name || !*name || strlen(name) >= IFNAMSIZ) return 0;
	for (; *p; ++p)
		if (!isalnum(*p) && *p != '_' && *p != '-' && *p != '.') return 0;
	return 1;
}

static int add_ingress(struct deferred_rule **head, struct deferred_scope *scope,
	int family, int priority, int table, const char *src, const char *dst, const char *iif, int include_disabled)
{
	size_t i;
	int found = 0;
	const char *lan = nvram_safe_get("lan_ifname");
	if (iif && *iif) return valid_ingress(iif) ? append_rule(head, family, priority, table, src, dst, iif) : -1;
	if (valid_ingress(lan)) {
		if (append_rule(head, family, priority, table, src, dst, lan)) return -1;
		found = 1;
	} else if (*lan) scope->unresolved = scope->invalid_ingress = 1;
	for (i = 0; i < scope->count; ++i) {
		if ((!include_disabled && !scope->mtlan[i].enable) || !*scope->mtlan[i].nw_t.ifname) continue;
		if (!valid_ingress(scope->mtlan[i].nw_t.ifname)) { scope->unresolved = scope->invalid_ingress = 1; continue; }
		if (append_rule(head, family, priority, table, src, dst, scope->mtlan[i].nw_t.ifname)) return -1;
		found = 1;
	}
	return found ? 0 : -1;
}

static int collect_policy(const struct qca_vpn_deferred_policy *policy, void *arg)
{
	struct deferred_scope *scope = arg;
	int family, wan;
	char legacy_source[16];
	if (policy->flags & (QCA_VPN_DEFERRED_NETWORK | QCA_VPN_DEFERRED_DNS_NETWORK)) {
		for (family = AF_INET; family <= AF_INET6; family += AF_INET6 - AF_INET) {
			if (policy->family != AF_UNSPEC && policy->family != family) continue;
			if ((policy->flags & QCA_VPN_DEFERRED_NETWORK) &&
			    add_ingress(&scope->networks, scope, family, DEFERRED_TRANSITION_GUARD,
			        -1, NULL, NULL, policy->iif, 1)) return -1;
			if (add_ingress(&scope->network_dns, scope, family, 0, -1, NULL, NULL, policy->iif, 1)) return -1;
		}
		return 0;
	}
	wan = qca_vpn_deferred_wan_override(policy->src, policy->iif);
	if (wan < 0) return -1;
	for (family = AF_INET; family <= AF_INET6; family += AF_INET6 - AF_INET) {
		if (policy->family != AF_UNSPEC && policy->family != family) continue;
		if (add_ingress(&scope->guards, scope, family, DEFERRED_GUARD, -1,
		    policy->src, policy->dst, policy->iif, 0)) return -1;
		/* Shared dnsmasq cannot retain the original external destination.
		 * Only identifiable source/interface DNS is quarantined here. */
		if ((*policy->src || *policy->iif) && add_ingress(&scope->dns, scope, family, 0, -1,
		    policy->src, NULL, policy->iif, 0)) return -1;
	}
	/* Legacy IPv4 policy installation truncated source to 15 bytes and
	 * ignored destination. Retire that exact old selector, not Director rules. */
	strlcpy(legacy_source, policy->src, sizeof(legacy_source));
	if (*legacy_source && !(wan && policy->index < 5)) {
		char normalized[INET6_ADDRSTRLEN + 4];
		if (!canonical_address(AF_INET, legacy_source, normalized, sizeof(normalized)) &&
		    append_rule(&scope->unsafe, AF_INET, 100, policy->index >= 5 ? policy->index : 254,
		        normalized, NULL, NULL)) return -1;
	}
	if (*policy->iif && !(wan && policy->index < 5) && append_rule(&scope->unsafe, AF_INET, 1000 + policy->index * 3,
	    policy->index >= 5 ? policy->index : 254, NULL, NULL, policy->iif)) return -1;
	return 0;
}

static int parse_table(const char *name)
{
	char *end;
	long value;
	if (!strcmp(name, "main")) return 254;
	if (!strncmp(name, "wgc", 3)) name += 3;
	else if (!strncmp(name, "ovpnc", 5)) {
		value = strtol(name + 5, &end, 10);
		return *end || value < 1 || value > 5 ? -2 : value + 5;
	}
	value = strtol(name, &end, 10);
	return !*name || *end || value < 0 || value > 255 ? -2 : value;
}

static int parse_rule(char *line, int family, struct deferred_rule *rule)
{
	char *token, *save, *end;
	int from = 0, to = 0, iif = 0, action = 0, protocol = 0;
	long priority;
	memset(rule, 0, sizeof(*rule));
	rule->family = family; rule->table = -2;
	strcpy(rule->dst, "all");
	errno = 0; priority = strtol(line, &end, 10);
	if (errno || end == line || *end != ':' || priority < 0 || priority > 32767) return 0;
	rule->priority = priority;
	for (token = strtok_r(end + 1, " \t\r\n", &save); token;
	     token = strtok_r(NULL, " \t\r\n", &save)) {
		char *value;
		if ((!strcmp(token, "prohibit") || !strcmp(token, "8")) && !action++) { rule->table = -1; continue; }
		value = strtok_r(NULL, " \t\r\n", &save);
		if (!value) return 0;
		if (!strcmp(token, "from") && !from++) {
			if (canonical_address(family, value, rule->src, sizeof(rule->src))) return 0;
			strlcpy(rule->captured_src, value, sizeof(rule->captured_src));
		} else if (!strcmp(token, "to") && !to++) {
			if (canonical_address(family, value, rule->dst, sizeof(rule->dst))) return 0;
			strlcpy(rule->captured_dst, value, sizeof(rule->captured_dst));
		} else if (!strcmp(token, "iif") && !iif++) {
			if (strlen(value) >= sizeof(rule->iif)) return 0;
			strcpy(rule->iif, value);
		} else if (!strcmp(token, "lookup") && !action++) rule->table = parse_table(value);
	    else if ((!strcmp(token, "proto") || !strcmp(token, "protocol")) && !protocol++) {
	        char *last;
	        long tag = strtol(value, &last, 10);
	        if (!*value || *last || (tag != 0 &&
	            (tag < DEFERRED_PROTOCOL_FIRST || tag > DEFERRED_PROTOCOL_LAST))) return 0;
	        rule->protocol = tag;
	    }
		else return 0; /* Unknown selector extensions are not owned. */
	}
	return from == 1 && action == 1 && rule->table >= -1;
}

static int read_rules(struct deferred_rule **rules)
{
	int family, failed;
	char line[512], original[512];
	FILE *fp;
	struct deferred_rule parsed, *item, **tail = rules;
	while (*tail) tail = &(*tail)->next;
	for (family = AF_INET; family <= AF_INET6; family += AF_INET6 - AF_INET) {
		fp = popen(family == AF_INET ? "ip -4 -N -details rule show" : "ip -6 -N -details rule show", "r");
		if (!fp) return -1;
		failed = 0;
		while (fgets(line, sizeof(line), fp)) {
			if (!strchr(line, '\n')) { failed = 1; break; }
			strlcpy(original, line, sizeof(original));
			if (!parse_rule(line, family, &parsed)) {
				char *end, *iif, *tag;
				unsigned int protocol;
				long priority = strtol(original, &end, 10);
				if (end == original || *end != ':' || priority < 0 || priority > 32767) continue;
				memset(&parsed, 0, sizeof(parsed));
				parsed.family = family; parsed.priority = priority; parsed.opaque = 1;
				if ((iif = strstr(original, " iif "))) sscanf(iif + 5, "%15s", parsed.iif);
				parsed.protocol = -1;
				if ((tag = strstr(original, " proto ")) && sscanf(tag + 7, "%u", &protocol) == 1 && protocol <= 255)
					parsed.protocol = protocol;
			}
			item = malloc(sizeof(*item));
			if (!item) { failed = 1; break; }
			*item = parsed; *tail = item; tail = &item->next;
		}
		if (ferror(fp)) failed = 1;
		if (pclose(fp)) failed = 1;
		if (failed) return -1;
	}
	return 0;
}

static int execute_rule(const char *action, const struct deferred_rule *rule)
{
	char priority[16], table[16], protocol[16], *argv[24];
	int n = 0;
	snprintf(priority, sizeof(priority), "%d", rule->priority);
	snprintf(table, sizeof(table), "%d", rule->table);
	snprintf(protocol, sizeof(protocol), "%d", rule->protocol);
	argv[n++] = "ip"; argv[n++] = rule->family == AF_INET ? "-4" : "-6";
	argv[n++] = "rule"; argv[n++] = (char *)action;
	argv[n++] = "from";
	argv[n++] = (char *)(!strcmp(action, "del") && *rule->captured_src ? rule->captured_src : rule->src);
	argv[n++] = "to";
	argv[n++] = (char *)(!strcmp(action, "del") && *rule->captured_dst ? rule->captured_dst : rule->dst);
	if (*rule->iif) { argv[n++] = "iif"; argv[n++] = (char *)rule->iif; }
	argv[n++] = "priority"; argv[n++] = priority;
	argv[n++] = "protocol"; argv[n++] = protocol;
	if (rule->table == -1) argv[n++] = "prohibit";
	else { argv[n++] = "table"; argv[n++] = table; }
	argv[n] = NULL;
	return _eval(argv, NULL, 0, NULL);
}

static int publish_rules(struct deferred_rule *desired, struct deferred_rule **old)
{
	struct deferred_rule *item, *existing, *legacy;
	for (item = desired; item; item = item->next) {
		for (legacy = *old; legacy; legacy = legacy->next)
			if (!legacy->opaque && legacy->table == -1 && !legacy->protocol &&
			    legacy->family == item->family && legacy->priority == item->priority) break;
		for (existing = *old; existing; existing = existing->next)
			if (!legacy && !existing->opaque && existing->protocol &&
			    !existing->retained && equal_rule(item, existing)) break;
		if (existing) { existing->retained = 1; continue; }
		existing = malloc(sizeof(*existing));
		if (!existing) return -1;
		if (execute_rule("add", item)) { free(existing); return -1; }
		*existing = *item; existing->retained = 1; existing->created = 1; existing->next = *old; *old = existing;
	}
	return 0;
}

static int delete_captured(struct deferred_rule *rule, struct deferred_rule *all)
{
	struct deferred_rule *earlier;
	if (!rule->protocol) {
		/* On the QCA kernel protocol0 is a delete wildcard. Never let an
		 * original untagged delete select an earlier retained/foreign rule. */
		for (earlier = all; earlier && earlier != rule; earlier = earlier->next)
			if (!earlier->created && !earlier->removed && earlier->family == rule->family &&
			    earlier->priority == rule->priority && (earlier->opaque || covers_rule(rule, earlier)))
				return -1;
	}
	if (execute_rule("del", rule)) return -1;
	rule->removed = 1;
	return 0;
}

/* Keep staged broad protection until routing reconciliation has succeeded.
 * Read the full table so a missing private chain differs from a read failure. */
static int retain_network_rules(struct deferred_rule **networks, struct deferred_rule **dns, int family)
{
	char line[512], *token, *save, *iif, *protocol, *value, *body;
	FILE *fp;
	int failed = 0, port, drop, network;
	fp = popen(family == AF_INET ? "iptables -S" : "ip6tables -S", "r");
	if (!fp) return -1;
	while (fgets(line, sizeof(line), fp)) {
		if (!strchr(line, '\n')) { failed = 1; break; }
		if (!strncmp(line, "-A " DEFERRED_NETWORK_CHAIN " ", sizeof("-A " DEFERRED_NETWORK_CHAIN " ") - 1)) {
			network = 1; body = line + sizeof("-A " DEFERRED_NETWORK_CHAIN " ") - 1;
		} else if (!strncmp(line, "-A " DEFERRED_NETWORK_DNS_CHAIN " ", sizeof("-A " DEFERRED_NETWORK_DNS_CHAIN " ") - 1)) {
			network = 0; body = line + sizeof("-A " DEFERRED_NETWORK_DNS_CHAIN " ") - 1;
		} else continue;
		iif = ""; protocol = NULL; port = drop = 0;
		for (token = strtok_r(body, " \t\r\n", &save); token; token = strtok_r(NULL, " \t\r\n", &save)) {
			value = strtok_r(NULL, " \t\r\n", &save);
			if (!value) { failed = 1; break; }
			if (!strcmp(token, "-i")) iif = value;
			else if (!network && !strcmp(token, "-p")) protocol = value;
			else if (!network && !strcmp(token, "-m") && (!strcmp(value, "udp") || !strcmp(value, "tcp"))) continue;
			else if (!network && !strcmp(token, "--dport") && !strcmp(value, "53")) port = 1;
			else if (!strcmp(token, "-j") && !strcmp(value, "DROP")) drop = 1;
			else { failed = 1; break; }
		}
		if (failed || !*iif || !drop || (!network && (!protocol ||
		    (strcmp(protocol, "udp") && strcmp(protocol, "17") && strcmp(protocol, "tcp") && strcmp(protocol, "6")) || !port)) ||
		    append_rule(network ? networks : dns, family, network ? DEFERRED_TRANSITION_GUARD : 0, -1, NULL, NULL, iif)) {
			failed = 1; break;
		}
	}
	if (ferror(fp)) failed = 1;
	if (pclose(fp)) failed = 1;
	return failed ? -1 : 0;
}

static int write_dns(FILE *fp, int family, const struct deferred_scope *scope,
	int input_jumps, int forward_jumps, int network_jumps, int preserve)
{
	const struct deferred_rule *rule;
	struct deferred_rule *drops = NULL, *networks = NULL, *wide = NULL;
	int i, result = -1;
	for (rule = scope->dns; rule; rule = rule->next) {
		if (rule->family != family) continue;
		if (append_rule(&drops, family, 17, -1, rule->src, NULL, rule->iif) ||
		    append_rule(&drops, family, 6, -1, rule->src, NULL, rule->iif)) goto done;
	}
	for (rule = scope->networks; rule; rule = rule->next)
		if (rule->family == family && append_rule(&networks, family, DEFERRED_TRANSITION_GUARD, -1, NULL, NULL, rule->iif)) goto done;
	for (rule = scope->network_dns; rule; rule = rule->next)
		if (rule->family == family && append_rule(&wide, family, 0, -1, NULL, NULL, rule->iif)) goto done;
	if (preserve && retain_network_rules(&networks, &wide, family)) goto done;
	fprintf(fp, ":%s - [0:0]\n:%s - [0:0]\n:%s - [0:0]\n-F %s\n-F %s\n-F %s\n",
	    DEFERRED_DNS_CHAIN, DEFERRED_NETWORK_CHAIN, DEFERRED_NETWORK_DNS_CHAIN,
	    DEFERRED_DNS_CHAIN, DEFERRED_NETWORK_CHAIN, DEFERRED_NETWORK_DNS_CHAIN);
	for (i = 0; i < input_jumps; ++i) fprintf(fp, "-D INPUT -j %s\n", DEFERRED_DNS_CHAIN);
	for (i = 0; i < forward_jumps; ++i) fprintf(fp, "-D FORWARD -j %s\n", DEFERRED_DNS_CHAIN);
	for (i = 0; i < network_jumps; ++i) fprintf(fp, "-D FORWARD -j %s\n", DEFERRED_NETWORK_CHAIN);
	fprintf(fp, "-I INPUT 1 -j %s\n-I FORWARD 1 -j %s\n-I FORWARD 1 -j %s\n-A %s -j %s\n",
	    DEFERRED_DNS_CHAIN, DEFERRED_DNS_CHAIN, DEFERRED_NETWORK_CHAIN, DEFERRED_DNS_CHAIN, DEFERRED_NETWORK_DNS_CHAIN);
	for (rule = networks; rule; rule = rule->next)
		fprintf(fp, "-A %s -i %s -j DROP\n", DEFERRED_NETWORK_CHAIN, rule->iif);
	for (rule = wide; rule; rule = rule->next) {
		fprintf(fp, "-A %s -i %s -p udp --dport 53 -j DROP\n", DEFERRED_NETWORK_DNS_CHAIN, rule->iif);
		fprintf(fp, "-A %s -i %s -p tcp --dport 53 -j DROP\n", DEFERRED_NETWORK_DNS_CHAIN, rule->iif);
	}
	for (rule = scope->exceptions; rule; rule = rule->next) {
		if (rule->family != family) continue;
		fprintf(fp, "-A %s", DEFERRED_DNS_CHAIN);
		if (strcmp(rule->src, "all")) fprintf(fp, " -s %s", rule->src);
		if (strcmp(rule->dst, "all")) fprintf(fp, " -d %s", rule->dst);
		fprintf(fp, " -j RETURN\n");
	}
	for (rule = drops; rule; rule = rule->next) {
		fprintf(fp, "-A %s", DEFERRED_DNS_CHAIN);
		if (*rule->iif) fprintf(fp, " -i %s", rule->iif);
		if (strcmp(rule->src, "all")) fprintf(fp, " -s %s", rule->src);
		fprintf(fp, " -p %s --dport 53 -j DROP\n", rule->priority == 17 ? "udp" : "tcp");
	}
	result = ferror(fp) ? -1 : 0;
done:
	free_rules(drops); free_rules(networks); free_rules(wide);
	return result;
}

static int refresh_dns(int family, const struct deferred_scope *scope, int preserve)
{
	char line[512], path[] = "/tmp/qca-deferred-dns.XXXXXX";
	int input_jumps = 0, forward_jumps = 0, network_jumps = 0, failed, fd, result;
	FILE *fp;
	fp = popen(family == AF_INET ? "iptables -S" : "ip6tables -S", "r");
	if (!fp) return -1;
	while (fgets(line, sizeof(line), fp)) {
		if (!strcmp(line, "-A INPUT -j " DEFERRED_DNS_CHAIN "\n")) ++input_jumps;
		if (!strcmp(line, "-A FORWARD -j " DEFERRED_DNS_CHAIN "\n")) ++forward_jumps;
		if (!strcmp(line, "-A FORWARD -j " DEFERRED_NETWORK_CHAIN "\n")) ++network_jumps;
	}
	failed = ferror(fp);
	if (pclose(fp)) failed = 1;
	if (failed) return -1;
	fd = mkstemp(path);
	if (fd < 0) return -1;
	fp = fdopen(fd, "w");
	if (!fp) { close(fd); unlink(path); return -1; }
	fprintf(fp, "*filter\n");
	failed = write_dns(fp, family, scope, input_jumps, forward_jumps, network_jumps, preserve);
	fprintf(fp, "COMMIT\n");
	if (fclose(fp)) failed = 1;
	result = failed ? -1 : eval(family == AF_INET ? "iptables-restore" : "ip6tables-restore", "--noflush", path);
	unlink(path);
	return result;
}

/* A source-only legacy rule can affect any ingress carrying that source.
 * Use a valid configured IPv4 subnet to rule out unrelated networks; unknown
 * subnet or IPv6 identity remains conservative. */
static int may_use_network(const struct deferred_scope *scope, const struct deferred_rule *rule,
	const struct deferred_rule *network)
{
	const char *address = NULL, *netmask = NULL;
	struct in_addr mask;
	unsigned int value, bits = 0;
	char prefix[INET_ADDRSTRLEN + 4], normalized[INET6_ADDRSTRLEN + 4];
	size_t i;
	if (*rule->iif) return !strcmp(rule->iif, network->iif);
	if (rule->opaque || rule->family != AF_INET || !strcmp(rule->src, "all")) return 1;
	if (!strcmp(network->iif, nvram_safe_get("lan_ifname"))) {
		address = nvram_safe_get("lan_ipaddr"); netmask = nvram_safe_get("lan_netmask");
	} else for (i = 0; i < scope->count; ++i) {
		if (strcmp(network->iif, scope->mtlan[i].nw_t.ifname)) continue;
		address = scope->mtlan[i].nw_t.addr; netmask = scope->mtlan[i].nw_t.netmask;
		break;
	}
	if (!address || !netmask || inet_pton(AF_INET, netmask, &mask) != 1) return 1;
	value = ntohl(mask.s_addr);
	if ((~value & (~value + 1U)) != 0) return 1;
	while (value & 0x80000000U) { ++bits; value <<= 1; }
	snprintf(prefix, sizeof(prefix), "%s/%u", address, bits);
	if (canonical_address(AF_INET, prefix, normalized, sizeof(normalized))) return 1;
	return address_covers(AF_INET, rule->src, normalized) || address_covers(AF_INET, normalized, rule->src);
}

/* Exact SDN lookups have a separate owner. A WAN correction can retire them
 * here; a fixed assignment still awaiting its owner retains broad staging.
 * The SDN caller runs another refresh after successfully replacing its lookup. */
static int current_sdn_guard_ready(int target, const struct deferred_rule *network,
	const struct deferred_rule *old)
{
	char key[32];
	int enforced, unit, rgw;
	const struct deferred_rule *guard;
	if (target >= 1 && target <= WG_CLIENT_MAX) {
		snprintf(key, sizeof(key), "wgc%d_enable", target); enforced = nvram_get_int(key);
		snprintf(key, sizeof(key), "wgc%d_enforce", target); enforced = enforced && nvram_get_int(key);
	} else {
		unit = target - WG_CLIENT_MAX;
		if (unit < 1 || unit > OVPN_CLIENT_MAX) return 0;
		snprintf(key, sizeof(key), "vpn_client%d_rgw", unit); rgw = nvram_get_int(key);
		snprintf(key, sizeof(key), "vpn_client%d_enforce", unit);
		enforced = nvram_get_int(key) && ovpn_is_client_enabled(unit) &&
		    (rgw == OVPN_RGW_ALL || rgw == OVPN_RGW_POLICY);
	}
	if (!enforced) return 1;
	if (network->family == AF_INET6)
		return !eval("ip6tables", "-C", "FORWARD", "-j", "VPN6KS") &&
		    !eval("ip6tables", "-C", "VPN6KS", "-i", (char *)network->iif, "-j", "DROP");
	for (guard = old; guard; guard = guard->next)
		if (!guard->opaque && guard->family == AF_INET && guard->priority == VPNDIR_PRIO_KS_SDN &&
		    guard->table == -1 && !strcmp(guard->src, "all") && !strcmp(guard->dst, "all") &&
		    !strcmp(guard->iif, network->iif)) return 1;
	return 0;
}

static int known_sdn_release(struct deferred_scope *scope, const struct deferred_rule *rule,
	const struct deferred_rule *network, const struct deferred_rule *old)
{
	size_t i;
	int target;
	MTLAN_T fallback;
	const MTLAN_T *mtl;
	if (rule->opaque || rule->protocol || rule->table < 1 || rule->table > 20 ||
	    strcmp(rule->src, "all") || strcmp(rule->dst, "all") || strcmp(rule->iif, network->iif)) return 0;
	for (i = 0; i <= scope->count; ++i) {
		mtl = scope_sdn(scope, i, &fallback);
		if (!mtl) continue;
		if (strcmp(mtl->nw_t.ifname, network->iif) ||
		    (rule->priority != 1000 + rule->table * 3 &&
		     !(rule->priority == 999 && mtl->sdn_t.sdn_idx == 0))) continue;
		target = mtl->enable ? mtl->sdn_t.vpnc_idx : 0;
		if (mtl->enable && !target && !mtl->sdn_t.sdn_idx) target = nvram_get_int("vpnc_default_wan");
		if (qca_vpn_sdn_deferred(mtl->sdn_t.sdn_idx, target)) return 0;
		if (!target) {
			if (append_rule(&scope->unsafe, rule->family, rule->priority, rule->table,
			    rule->src, rule->dst, rule->iif)) return -1;
		} else if (target != rule->table || !current_sdn_guard_ready(target, network, old)) {
			if (append_rule(&scope->networks, network->family, DEFERRED_TRANSITION_GUARD, -1,
			    NULL, NULL, network->iif) || append_rule(&scope->network_dns, network->family,
			    0, -1, NULL, NULL, network->iif)) return -1;
		}
		return 1;
	}
	return 0;
}

static int stage_current_sdn(struct deferred_scope *scope, const struct deferred_rule *network,
	const struct deferred_rule *old)
{
	size_t i;
	int target, ready;
	MTLAN_T fallback;
	const MTLAN_T *mtl;
	const struct deferred_rule *lookup;
	for (i = 0; i <= scope->count; ++i) {
		mtl = scope_sdn(scope, i, &fallback);
		if (!mtl) continue;
		if (!mtl->enable || strcmp(mtl->nw_t.ifname, network->iif)) continue;
		target = mtl->sdn_t.vpnc_idx;
		if (!target && !mtl->sdn_t.sdn_idx) target = nvram_get_int("vpnc_default_wan");
		if (!target || qca_vpn_sdn_deferred(mtl->sdn_t.sdn_idx, target)) continue;
		ready = current_sdn_guard_ready(target, network, old);
		if (network->family == AF_INET) {
			for (lookup = old; lookup; lookup = lookup->next)
				if (!lookup->opaque && !lookup->protocol && lookup->family == AF_INET &&
				    lookup->table == target && !strcmp(lookup->iif, network->iif) &&
				    !strcmp(lookup->src, "all") && !strcmp(lookup->dst, "all") &&
				    lookup->priority == (mtl->sdn_t.sdn_idx ? 1000 + target * 3 : 999)) break;
			ready &= lookup != NULL;
		}
		if (!ready && (append_rule(&scope->networks, network->family, DEFERRED_TRANSITION_GUARD,
		    -1, NULL, NULL, network->iif) || append_rule(&scope->network_dns, network->family,
		    0, -1, NULL, NULL, network->iif))) return -1;
	}
	return 0;
}

static int network_release_ambiguous(struct deferred_scope *scope, const struct deferred_rule *old)
{
	const struct deferred_rule *network, *rule, *desired, *owned;
	int known;
	for (network = scope->previous_networks; network; network = network->next) {
		for (desired = scope->networks; desired; desired = desired->next)
			if (equal_rule(network, desired)) break;
		if (desired) continue;
		if (stage_current_sdn(scope, network, old)) return -1;
		for (rule = old; rule; rule = rule->next) {
			if (rule->family != network->family || rule->protocol > 0 || !may_use_network(scope, rule, network)) continue;
			if (rule->priority != 100 && rule->priority != 999 &&
			    !(rule->priority >= 1003 && rule->priority <= 1060 && (rule->priority - 1000) % 3 == 0)) continue;
			if (!rule->opaque && (rule->table == -1 || (rule->table != 254 && (rule->table < 1 || rule->table > 20)))) continue;
			for (owned = scope->unsafe; owned; owned = owned->next)
				if (!rule->opaque && !rule->protocol && equal_rule(rule, owned)) break;
			if (owned) continue; /* Exact known ownership will be reconciled. */
			known = known_sdn_release(scope, rule, network, old);
			if (known < 0) return -1;
			if (known) continue;
			if (!rule->opaque && rule->table == 254 && rule->priority == 100 &&
			    qca_vpn_deferred_wan_override(strcmp(rule->src, "all") ? rule->src :
			        (rule->family == AF_INET ? "0.0.0.0/0" : "::/0"), rule->iif) == 1) continue;
			logmessage("vpndirector", "Ambiguous legacy lookup prevents releasing network quarantine on %s", network->iif);
			return -1;
		}
	}
	return 0;
}

int amvpn_write_deferred_dns(FILE *fp, int family)
{
	struct deferred_scope scope;
	int result;
	if (!fp || (family != AF_INET && family != AF_INET6)) return -1;
	result = collect_scope(&scope);
	if (!result) result = write_dns(fp, family, &scope, 0, 0, 0, 1);
	free_scope(&scope);
	return result;
}

int amvpn_refresh_deferred_locked(void)
{
	struct deferred_scope scope;
	struct deferred_rule *old = NULL, *temporary = NULL, *item, *unsafe, *previous_dns = NULL;
	int result = -1, protocol, failed, persist;
	if (collect_scope(&scope)) goto done;
	if (retain_network_rules(&scope.previous_networks, &previous_dns, AF_INET) ||
	    retain_network_rules(&scope.previous_networks, &previous_dns, AF_INET6)) goto done;
	/* Publish network protection before inspecting routes: a foreign earlier
	 * lookup must not bypass newly activated conservative quarantine. Retain
	 * old broad protection until this whole reconciliation succeeds. */
	failed = refresh_dns(AF_INET, &scope, 1);
	failed |= refresh_dns(AF_INET6, &scope, 1);
	if (failed || scope.invalid_ingress || read_rules(&old) || network_release_ambiguous(&scope, old)) goto done;
	/* Unknown selectors may carry an owned protocol tag. A delete omitting
	 * those selectors can match the foreign rule, so do not publish into an
	 * ambiguous guard priority at all. */
	for (item = old; item; item = item->next)
		if (item->opaque && (item->priority == DEFERRED_TRANSITION_GUARD ||
		    item->priority == DEFERRED_GUARD)) goto done;
	/* Separate rule identities permit broader selectors to be published before
	 * deleting old narrower rules at the same routing priority. */
	for (protocol = DEFERRED_PROTOCOL_FIRST; protocol <= DEFERRED_PROTOCOL_LAST; ++protocol) {
	    for (item = old; item; item = item->next)
	        if ((item->priority == DEFERRED_TRANSITION_GUARD || item->priority == DEFERRED_GUARD) && item->protocol == protocol) break;
	    if (!item) break;
	}
	if (protocol > DEFERRED_PROTOCOL_LAST && (scope.guards || scope.networks)) goto done;
	for (item = scope.networks; item; item = item->next)
		if (append_rule(&temporary, item->family, DEFERRED_TRANSITION_GUARD, -1,
		    NULL, NULL, item->iif)) goto done;
	for (item = scope.guards; item; item = item->next)
		if (append_rule(&temporary, item->family, DEFERRED_TRANSITION_GUARD, -1,
		    item->src, item->dst, item->iif)) goto done;
	/* Historical source-only/truncated lookups can be broader than the new
	 * selector. Protect their exact old scope until they are removed. */
	for (item = old; item; item = item->next)
		for (unsafe = scope.unsafe; unsafe; unsafe = unsafe->next)
			if (!item->opaque && !item->protocol && equal_rule(item, unsafe) && add_ingress(&temporary, &scope, item->family,
			    DEFERRED_TRANSITION_GUARD, -1, item->src, item->dst, item->iif, 0)) goto done;
	prune_covered(&temporary); prune_covered(&scope.guards);
	for (item = temporary; item; item = item->next) item->protocol = protocol;
	for (item = scope.guards; item; item = item->next) item->protocol = protocol;
	if (publish_rules(temporary, &old) || publish_rules(scope.guards, &old)) goto done;
	for (item = old; item; item = item->next)
		for (unsafe = scope.unsafe; unsafe; unsafe = unsafe->next)
			if (!item->opaque && !item->protocol && equal_rule(item, unsafe) && delete_captured(item, old)) goto done;
	/* Every unsafe lookup is gone and complete durable-in-kernel protection
	 * is present. Only now release transition guards and stale owned guards. */
	for (item = old; item; item = item->next) {
		if (item->opaque || item->table != -1) continue;
		persist = 0;
		if (item->retained)
			for (unsafe = scope.networks; unsafe; unsafe = unsafe->next)
				if (equal_rule(item, unsafe)) { persist = 1; break; }
		if ((item->priority == DEFERRED_TRANSITION_GUARD && !persist) ||
		    (item->priority == DEFERRED_GUARD && !item->retained))
			if (delete_captured(item, old)) goto done;
	}
	failed = refresh_dns(AF_INET, &scope, 0);
	failed |= refresh_dns(AF_INET6, &scope, 0);
	if (failed) goto done;
	result = 0;
	if (scope.unresolved) logmessage("vpndirector", "Unresolved VPN selectors remain; conservative network quarantine active");
done:
	free_scope(&scope); free_rules(old); free_rules(temporary); free_rules(previous_dns);
	if (result) logmessage("vpndirector", "Deferred VPN quarantine refresh failed; existing guards retained");
	return result;
}

int amvpn_refresh_deferred(void)
{
	int lock = file_lock(VPNROUTING_LOCK), result;
	if (lock < 0) return -1;
	result = amvpn_refresh_deferred_locked();
	file_unlock(lock);
	return result;
}

static int collect_scope(struct deferred_scope *scope)
{
	size_t i;
	int target, family, table, parsed, deferred, covered;
	int unit, allowed;
	MTLAN_T fallback;
	const MTLAN_T *mtl;
	struct deferred_rule *rule;
	char director[8000], *cursor, *row, *enabled, *description, *src, *dst, *name, extra;
	char prefix[32], normalized[INET6_ADDRSTRLEN + 4];
	memset(scope, 0, sizeof(*scope));
	scope->mtlan = (MTLAN_T *)INIT_MTLAN(sizeof(MTLAN_T));
	if (!scope->mtlan) return -1;
	get_mtlan(scope->mtlan, &scope->count);
	if (*nvram_safe_get("lan_ifname") && !valid_ingress(nvram_safe_get("lan_ifname")))
		scope->unresolved = scope->invalid_ingress = 1;
	/* Explicit fixed-slot Director policies remain independent. Preserve both
	 * selectors: an unrelated destination never exempts all of a host's DNS. */
	amvpn_get_policy_rules(-1, director, sizeof(director), VPNDIR_PROTO_NONE);
	cursor = director;
	while ((row = strsep(&cursor, "<"))) {
		if (vstrsep(row, ">", &enabled, &description, &src, &dst, &name) != 5 || !atoi(enabled)) continue;
		allowed = !strcmp(name, "WAN");
		if (sscanf(name, "WGC%d%c", &unit, &extra) == 1 && unit >= 1 && unit <= 5) {
			snprintf(prefix, sizeof(prefix), "wgc%d_enable", unit);
			allowed = nvram_get_int(prefix);
		} else if (sscanf(name, "OVPN%d%c", &unit, &extra) == 1 && unit >= 1 && unit <= 5)
			allowed = ovpn_is_client_enabled(unit);
		if (!allowed || canonical_address(AF_INET, src, normalized, sizeof(normalized)) ||
		    canonical_address(AF_INET, dst, normalized, sizeof(normalized))) continue;
		if (append_rule(&scope->exceptions, AF_INET, 0, -1, src, dst, NULL)) return -1;
	}
	parsed = qca_vpn_deferred_foreach(collect_policy, scope);
	if (parsed < 0) return -1;
	scope->unresolved |= parsed > 0;
	for (i = 0; i <= scope->count; ++i) {
		mtl = scope_sdn(scope, i, &fallback);
		if (!mtl) continue;
		if (!mtl->enable || !*mtl->nw_t.ifname) continue;
		if (!valid_ingress(mtl->nw_t.ifname)) { scope->unresolved = scope->invalid_ingress = 1; continue; }
		target = mtl->sdn_t.vpnc_idx;
		if (!target && !mtl->sdn_t.sdn_idx) target = nvram_get_int("vpnc_default_wan");
		deferred = qca_vpn_sdn_deferred(mtl->sdn_t.sdn_idx, target);
		covered = 0;
		for (rule = scope->guards; rule; rule = rule->next)
			if (!strcmp(rule->iif, mtl->nw_t.ifname)) covered = 1;
		if (!deferred && (target || !covered)) continue;
		for (family = AF_INET; family <= AF_INET6; family += AF_INET6 - AF_INET) {
			if (deferred && (append_rule(&scope->guards, family, DEFERRED_GUARD, -1, NULL, NULL, mtl->nw_t.ifname) ||
			    append_rule(&scope->dns, family, 0, -1, NULL, NULL, mtl->nw_t.ifname))) return -1;
			/* A previous assignment may have left another unit's SDN lookup.
			 * Remove every exact SDN-owned VPN lookup before releasing priority90. */
			for (table = 1; table <= 20; ++table)
				if (append_rule(&scope->unsafe, family, mtl->sdn_t.sdn_idx ? 1000 + table * 3 : 999,
				    table, NULL, NULL, mtl->nw_t.ifname)) return -1;
		}
	}
	if (scope->invalid_ingress) logmessage("vpndirector", "Invalid configured ingress name omitted from VPN quarantine");
	return 0;
}
