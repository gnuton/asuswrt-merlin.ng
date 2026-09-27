/* Read-only classification and full-length selectors used by real consumers. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qca_vpn_deferred.h>

static struct qca_vpn_deferred_policy seen[64];
static int count, checks;

char *nvram_get(const char *name) { return getenv(name); }
int nvram_set(const char *name, const char *value) { (void)name; (void)value; abort(); }
int nvram_unset(const char *name) { (void)name; abort(); }
int nvram_commit(void) { abort(); }

static int capture(const struct qca_vpn_deferred_policy *policy, void *unused)
{
	(void)unused;
	assert(count < (int)(sizeof(seen) / sizeof(seen[0])));
	seen[count++] = *policy;
	return 0;
}

static void policy(const char *value, int result, int expected)
{
	int actual;
	setenv("vpnc_dev_policy_list", value, 1);
	count = 0;
	actual = qca_vpn_deferred_foreach(capture, NULL);
	if (actual != result || count != expected)
		fprintf(stderr, "Case %d: status %d/%d callbacks %d/%d: %s\n", checks + 1, actual, result, count, expected, value);
	assert(actual == result);
	assert(count == expected);
	++checks;
}

static int scope(int family, unsigned int flags, const char *iif)
{
	int i;
	for (i = 0; i < count; ++i)
		if (seen[i].family == family && seen[i].flags == flags && !strcmp(seen[i].iif, iif)) {
			if (flags) assert(!*seen[i].src && !*seen[i].dst);
			return 1;
		}
	return 0;
}

int main(void)
{
	char oversized[8193];
	setenv("lan_ifname", "br0", 1);
	setenv("lan_ipaddr", "192.0.2.1", 1);
	setenv("lan_netmask", "255.255.255.0", 1);
	setenv("vlan_rl", "<1>10>0", 1);
	setenv("subnet_rl", "<1>br1>198.51.100.1>255.255.255.0>1>198.51.100.10>198.51.100.200>86400>>>>0>0", 1);
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>0", 1);
	setenv("vpnc_default_wan", "0", 1);
	unsetenv("qca_merlin_vpn_migrated");
	setenv("vpnc_clientlist", "", 1);
	assert(!qca_vpn_binding_deferred(6));
	policy("<1>192.0.2.20>>5", 0, 0); /* New Merlin state, no retained stock namespace. */
	setenv("vpnc_clientlist", "<Provider>CyberGhost>1>>>0>8>>>0>0>Web", 1);
	assert(qca_vpn_binding_deferred(8) && qca_vpn_binding_deferred(5));
	assert(!qca_vpn_binding_deferred(0));
	policy("<1>198.51.100.128/25>203.0.113.99>5", 0, 2);
	assert(!strcmp(seen[0].src, "198.51.100.128/25") && seen[0].family == AF_INET);
	assert(scope(AF_INET6, QCA_VPN_DEFERRED_NETWORK, "br1"));
	policy("<1>2001:db8:1::10>2001:db8:ffff::/64>5", 0, 1);
	assert(!strcmp(seen[0].dst, "2001:db8:ffff::/64") && seen[0].family == AF_INET6);
	policy("<1>>203.0.113.99>5", 0, 2);
	assert(!*seen[0].src && seen[0].family == AF_INET);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_DNS_NETWORK, ""));
	policy("<1>>>5", 0, 2);
	assert(seen[0].family == AF_UNSPEC);
	policy("<0>not-an-address>>5<1>192.0.2.22>>0", 0, 0);
	assert(qca_vpn_deferred_wan_override("192.0.2.22", NULL) == 1);
	assert(!qca_vpn_deferred_wan_override("192.0.2.23", NULL));
	policy("<1>192.0.2.1/24>>0", 0, 0);
	assert(qca_vpn_deferred_wan_override("192.0.2.0/24", NULL) == 1);
	assert(!qca_vpn_deferred_wan_override("192.0.2.0/25", NULL));
	policy("<1>0.0.0.0/0>>0", 0, 0);
	assert(qca_vpn_deferred_wan_override("0.0.0.0/0", NULL));
	assert(!qca_vpn_deferred_wan_override("0.0.0.0", NULL));
	assert(!qca_vpn_deferred_wan_override("::/0", NULL));
	policy("<1>::/0>>0", 0, 0);
	assert(qca_vpn_deferred_wan_override("::/0", NULL));
	assert(!qca_vpn_deferred_wan_override("0.0.0.0/0", NULL));
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>5", 1);
	assert(qca_vpn_sdn_deferred(1, 5) == 1);
	assert(!qca_vpn_sdn_deferred(0, 0));
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>bad", 1);
	assert(qca_vpn_sdn_deferred(1, 0) == 2);
	policy("", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br1"));
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>999", 1);
	assert(qca_vpn_sdn_deferred(1, 999) == 2);
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>0", 1);
	assert(!qca_vpn_sdn_deferred(1, 0));
	setenv("vpnc_default_wan", "bad", 1);
	assert(qca_vpn_sdn_deferred(0, 0) == 2);
	assert(!qca_vpn_sdn_deferred(1, 0));
	policy("", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br0"));
	setenv("vpnc_default_wan", "0", 1);
	assert(!qca_vpn_sdn_deferred(0, 0));
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>5", 1);
	policy("<1>192.0.2.20>203.0.113.99>5>br1", 0, 5);
	assert(!*seen[0].iif && !strcmp(seen[0].src, "192.0.2.20"));
	assert(!*seen[1].src && !strcmp(seen[1].iif, "br1") && seen[1].family == AF_INET);
	assert(scope(AF_INET6, QCA_VPN_DEFERRED_NETWORK, "br0") && scope(AF_INET6, QCA_VPN_DEFERRED_NETWORK, "br1"));
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_DNS_NETWORK, "br1"));
	policy("<1>192.0.2.20>>5>br1", 0, 5);
	assert(seen[1].family == AF_UNSPEC);
	policy("<1>>>5>br0", 0, 2);
	assert(!strcmp(seen[0].iif, "br0"));
	policy("<1>192.0.2.20>>5>wan0", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, ""));
	policy("<1>192.0.2.20>>5>missing0", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, ""));
	policy("<1>192.0.2.20>>5<1>not-an-address>>5", 1, 3);
	assert(!strcmp(seen[0].src, "192.0.2.20"));
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, ""));
	policy("<1>192.0.2.20>2001:db8::1>5", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br0"));
	policy("<1>192.0.2.20/33>>5", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, ""));
	policy("<1>>>bad>br1", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br1"));
	policy("<1>bad>>5>br1", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, ""));
	policy("<bad>192.0.2.20>>0", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br0"));
	assert(!qca_vpn_deferred_wan_override("192.0.2.20", NULL));
	policy("<1>192.0.2.20>bad>5>br1", 1, 2);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br0") && scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br1"));
	policy("<1>10.99.0.20>>5", 0, 2);
	assert(scope(AF_INET6, QCA_VPN_DEFERRED_NETWORK, ""));
	policy("<1>192.0.0.0/4>>5", 0, 3);
	assert(scope(AF_INET6, QCA_VPN_DEFERRED_NETWORK, "br0") && scope(AF_INET6, QCA_VPN_DEFERRED_NETWORK, "br1"));
	setenv("lan_netmask", "255.0.255.0", 1);
	policy("<1>198.51.100.20>>5", 0, 2);
	assert(scope(AF_INET6, QCA_VPN_DEFERRED_NETWORK, ""));
	setenv("lan_netmask", "255.255.255.0", 1);
	policy("<1>192.0.2.20>>5>br1>extra", 1, 2);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br0") && scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br1"));
	memset(oversized, 'x', sizeof(oversized) - 1); oversized[sizeof(oversized) - 1] = '\0';
	policy(oversized, 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, ""));
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>bad>1>1>0>5", 1);
	assert(qca_vpn_sdn_deferred(1, 5) == 2);
	policy("", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br1"));
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<bad>SDN>1>1>1>0>5", 1);
	policy("", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, ""));
	assert(qca_vpn_sdn_deferred(0, 0) == 2);
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>0>1>1>0>bad", 1);
	assert(!qca_vpn_sdn_deferred(1, 0));
	policy("", 0, 0);
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>1>1junk>1>0>5", 1);
	policy("", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, ""));
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>5<1>Other>1>0>0>0>0", 1);
	assert(qca_vpn_sdn_deferred(1, 5) == 2);
	policy("", 1, 2);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br0") && scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br1"));
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>5<1>Other>0>0>0>0>0", 1);
	policy("", 1, 2);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br0") && scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br1"));
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>Other>0>0>0>0>0<1>SDN>1>1>1>0>5", 1);
	policy("", 1, 2);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br0") && scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, "br1"));
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>5", 1);
	setenv("vlan_rl", "<1junk>10>0", 1);
	policy("", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, ""));
	setenv("vlan_rl", "<1>10>0<1>20>0", 1);
	policy("", 1, 1);
	assert(scope(AF_UNSPEC, QCA_VPN_DEFERRED_NETWORK, ""));
	setenv("vlan_rl", "<1>10>0", 1);
	setenv("sdn_rl", "<0>LAN>1>0>0>0>0<1>SDN>1>1>1>0>0", 1);
	setenv("qca_merlin_vpn_migrated", "1", 1);
	assert(!qca_vpn_binding_deferred(5));
	policy("<1>192.0.2.20>>5", 0, 0);
	setenv("qca_merlin_vpn_migrated", "2", 1);
	setenv("vpnc_clientlist", "", 1);
	assert(qca_vpn_binding_deferred(5)); /* Unknown future schema is not reinterpreted. */
	policy("<1>192.0.2.20>>5", 0, 2);
	printf("Deferred binding parser passed %d selector cases; no NVRAM writes\n", checks);
	return 0;
}
