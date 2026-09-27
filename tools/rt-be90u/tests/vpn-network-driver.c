/* Test-only NVRAM for the image's ARM library. Commands execute normally. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtconfig.h>
#include <openvpn_config.h>
#include <amvpn_routing.h>

extern void ovpn_client_up_handler(int unit);
/* Weak for running regressions against earlier packaged libraries. */
extern void amvpn_write_ipv6_killswitch(FILE *fp) __attribute__((weak));

#if !defined(RTCONFIG_SOC_IPQ53XX) || !defined(RTCONFIG_VPN_FUSION_MERLIN)
#error "Test requires the RT-BE90U Merlin VPN build configuration"
#endif

char *nvram_get(const char *name) { return getenv(name); }
int nvram_set(const char *name, const char *value) { return setenv(name, value, 1); }
int nvram_unset(const char *name) { return unsetenv(name); }
int nvram_commit(void) { return 0; }

int main(int argc, char **argv)
{
	vpndir_proto_t proto;
	char *sdn;
	if (argc < 3)
		return 2;
	proto = !strcmp(argv[2], "ovpn") ? VPNDIR_PROTO_OPENVPN : VPNDIR_PROTO_WIREGUARD;
	sdn = argc > 3 ? argv[3] : NULL;
	if (!strcmp(argv[1], "ipv6-write")) {
		if (!amvpn_write_ipv6_killswitch)
			return 3;
		amvpn_write_ipv6_killswitch(stdout);
	} else if (!strcmp(argv[1], "rules")) {
		amvpn_set_wan_routing_rules();
		amvpn_set_routing_rules(1, proto);
	} else if (!strcmp(argv[1], "clear")) {
		amvpn_clear_routing_rules(1, proto);
	} else if (!strcmp(argv[1], "kill")) {
		amvpn_set_killswitch_rules(proto, 1, sdn);
	} else if (!strcmp(argv[1], "unkill")) {
		amvpn_clear_killswitch_rules(proto, 1, sdn);
	} else if (!strcmp(argv[1], "dns")) {
		if (proto == VPNDIR_PROTO_OPENVPN)
			ovpn_set_exclusive_dns(1);
		else
			wgc_set_exclusive_dns(1);
		amvpn_update_exclusive_dns_rules();
	} else if (!strcmp(argv[1], "undns")) {
		amvpn_clear_exclusive_dns(1, proto);
	} else if (!strcmp(argv[1], "up")) {
		ovpn_client_up_handler(1);
	} else {
		return 2;
	}
	return 0;
}
