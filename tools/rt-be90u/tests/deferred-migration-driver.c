/* Frozen rc objects and packaged libraries; no substituted routing decisions. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtconfig.h>
#include <shared.h>
#include <amvpn_routing.h>

extern int migrate_qca_vpn_config(void);
extern int update_sdn_by_vpnc(int index);
extern int handle_sdn_feature(int index, unsigned long features, int action);
extern int _gen_vpnc_resolv_conf(int index);
extern void vpnc_ovpn_set_dns(int unit);
extern void vpnc_handle_dns_policy_rule(int command, int index);
extern int vpnc_get_dev_policy_list(VPNC_DEV_POLICY *list, int size, int temporary);
extern int vpnc_handle_policy_rule(int action, const VPNC_DEV_POLICY *policy);
extern int write_vpn_fusion_nat(FILE *file, const char *lan_ip);
extern int amvpn_refresh_deferred(void) __attribute__((weak));
/* Same constant as rc/services.c; unrelated service dispatch is not linked. */
const char dmservers[] = "/tmp/resolv.dnsmasq";
static int sets, firewall_updates, dnsmasq_launches, allow_commit, commits, wan_resolver_updates;

char *nvram_get(const char *name) { return getenv(name); }
int nvram_set(const char *name, const char *value) { ++sets; return setenv(name, value, 1); }
int nvram_unset(const char *name) { ++sets; return unsetenv(name); }
int nvram_commit(void) { assert(allow_commit); ++commits; return 0; }

int get_drop_accept(char *drop, size_t drop_size, char *accept, size_t accept_size)
{
	snprintf(drop, drop_size, "DROP");
	snprintf(accept, accept_size, "ACCEPT");
	return 0;
}
int update_SDN_iptables(const MTLAN_T *sdn, const char *drop, const char *accept)
{
	assert(!strcmp(sdn->nw_t.ifname, "br0") || !strcmp(sdn->nw_t.ifname, "br1"));
	assert(!strcmp(drop, "DROP") && !strcmp(accept, "ACCEPT"));
	++firewall_updates;
	return 0;
}

/* Independent global WAN resolver regeneration; SDN DNS generation is real. */
int update_resolvconf(void) { ++wan_resolver_updates; return 0; }

int __real__eval(char *const argv[], const char *path, int timeout, int *pid);
int __wrap__eval(char *const argv[], const char *path, int timeout, int *pid)
{
	/* Inspect generated configuration without starting DHCP/DNS services.
	 * Every ip/iptables command still executes through the packaged helper. */
	if (!strcmp(argv[0], "dnsmasq")) {
		assert(!strcmp(argv[1], "-C") && !strcmp(argv[2], "/etc/dnsmasq-1.conf"));
		++dnsmasq_launches;
		return 0;
	}
	return __real__eval(argv, path, timeout, pid);
}

int main(int argc, char **argv)
{
	int result;
	assert(argc == 3);
	if (!strcmp(argv[1], "defer")) {
		result = migrate_qca_vpn_config();
		assert(result == -1 && sets == 0);
		assert(!*nvram_safe_get("qca_merlin_vpn_migrated"));
		printf("Migration deferred without writes; WGC5 enable=%s description=%s\n",
		       nvram_safe_get("wgc5_enable"), nvram_safe_get("wgc5_desc"));
	} else if (!strcmp(argv[1], "refresh")) {
		if (atoi(argv[2]))
			assert(!update_sdn_by_vpnc(atoi(argv[2])) && firewall_updates > 0);
		else {
			/* Selected WAN refresh expresses a current zero assignment. */
			assert(!handle_sdn_feature(0, 0x200, 0)); /* SDN_FEATURE_WAN */
			assert(!handle_sdn_feature(1, 0x200, 0));
			assert(firewall_updates == 2 && wan_resolver_updates == 1);
		}
		printf("Actual SDN refresh index %s; independent firewall updates %d\n", argv[2], firewall_updates);
	} else if (!strcmp(argv[1], "quarantine")) {
		assert(amvpn_refresh_deferred && !amvpn_refresh_deferred());
	} else if (!strcmp(argv[1], "recover")) {
		VPNC_DEV_POLICY stale;
		memset(&stale, 0, sizeof(stale));
		stale.active = 1; stale.vpnc_idx = 5;
		strcpy(stale.src_ip, "192.0.2.20");
		allow_commit = 1;
		assert(!migrate_qca_vpn_config() && commits == 1);
		assert(!strcmp(nvram_safe_get("qca_merlin_vpn_migrated"), "1"));
		assert(!*nvram_safe_get("vpnc_clientlist") && !*nvram_safe_get("vpnc_dev_policy_list"));
		assert(!strcmp(nvram_safe_get("wgc5_enable"), "1"));
		assert(!strcmp(nvram_safe_get("wgc5_desc"), "Unrelated active WG5"));
		assert(!strcmp(nvram_safe_get("sdn_rl"), "<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>6"));
		assert(amvpn_refresh_deferred && !amvpn_refresh_deferred());
		assert(!vpnc_handle_policy_rule(1, &stale));
		assert(!update_sdn_by_vpnc(6));
		amvpn_set_routing_rules(5, VPNDIR_PROTO_WIREGUARD);
		amvpn_set_routing_rules(1, VPNDIR_PROTO_OPENVPN);
		vpnc_ovpn_set_dns(1);
		assert(!_gen_vpnc_resolv_conf(6));
		assert(!handle_sdn_feature(1, 0x10, 0x02) && dnsmasq_launches == 1);
		printf("Actual migration committed once; SDN index6 and DNS restored; independent WG5 settings retained\n");
	} else if (!strcmp(argv[1], "resolver")) {
		vpnc_ovpn_set_dns(1);
		assert(!_gen_vpnc_resolv_conf(6));
		assert(!_gen_vpnc_resolv_conf(5));
		assert(!handle_sdn_feature(1, 0x10, 0x02)); /* SDN_FEATURE_DNSMASQ, RC_SERVICE_START */
		assert(dnsmasq_launches == 1);
		printf("Actual resolver files: OVPN1 index6=%s; retained index5=%s\n",
		       nvram_safe_get("vpnc6_dns"), nvram_safe_get("vpnc5_dns"));
	} else if (!strcmp(argv[1], "dnsroute")) {
		vpnc_handle_dns_policy_rule(0, atoi(argv[2])); /* VPNC_ROUTE_ADD */
	} else if (!strcmp(argv[1], "policies") || !strcmp(argv[1], "parse")) {
		VPNC_DEV_POLICY policy[MAX_DEV_POLICY];
		int i, count = vpnc_get_dev_policy_list(policy, MAX_DEV_POLICY, 0);
		assert(count > 0);
		for (i = 0; i < count; ++i) {
			printf("Actual parsed policy: active=%d from=%s to=%s index=%d\n",
			       policy[i].active, policy[i].src_ip, policy[i].dst_ip, policy[i].vpnc_idx);
			if (policy[i].active && !strcmp(argv[1], "policies"))
				assert(!vpnc_handle_policy_rule(1, &policy[i]));
		}
	} else if (!strcmp(argv[1], "nat")) {
		FILE *file = fopen("/tmp/deferred-nat.rules", "w");
		assert(file);
		fputs("*nat\n:PREROUTING ACCEPT [0:0]\n:INPUT ACCEPT [0:0]\n"
		      ":OUTPUT ACCEPT [0:0]\n:POSTROUTING ACCEPT [0:0]\n:VPN_FUSION - [0:0]\n", file);
		assert(!write_vpn_fusion_nat(file, "192.0.2.1"));
		fputs("COMMIT\n", file);
		assert(!fclose(file));
	} else if (!strcmp(argv[1], "independent")) {
		amvpn_set_routing_rules(5, VPNDIR_PROTO_WIREGUARD);
	} else abort();
	return 0;
}
