/* Actual sdn.o refresh and packaged VPN routing; synthetic NVRAM only. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtconfig.h>
#include <shared.h>

#if !defined(RTCONFIG_SOC_IPQ53XX) || !defined(RTCONFIG_VPN_FUSION_MERLIN)
#error "RT-BE90U Merlin VPN configuration required"
#endif

extern int update_sdn_by_vpnc(const int index);
static int firewall_updates, label_reads;

char *nvram_get(const char *name) { return getenv(name); }
int nvram_set(const char *name, const char *value) { return setenv(name, value, 1); }
int nvram_unset(const char *name) { return unsetenv(name); }
int nvram_commit(void) { abort(); }

/* Independent hardware firewall generation is outside this routing fixture. */
int get_drop_accept(char *drop, size_t drop_size, char *accept, size_t accept_size)
{
	snprintf(drop, drop_size, "DROP");
	snprintf(accept, accept_size, "ACCEPT");
	++label_reads;
	return 0;
}

int update_SDN_iptables(const MTLAN_T *sdn, const char *drop, const char *accept)
{
	assert(!strcmp(sdn->nw_t.ifname, "br1"));
	assert(!strcmp(drop, "DROP") && !strcmp(accept, "ACCEPT"));
	++firewall_updates;
	return 0;
}

/* IPv6 Fusion profile lookup is not part of these IPv4 refresh cases. */
void vpnc_init(void) { fprintf(stderr, "Unexpected vpnc_init\n"); abort(); }
VPNC_PROTO vpnc_get_proto_in_profile_by_vpnc_id(const int index)
{
	(void)index;
	fprintf(stderr, "Unexpected Fusion profile lookup\n");
	abort();
}

int main(int argc, char **argv)
{
	int failed, result;
	assert((argc == 2 || argc == 3) && (!strcmp(argv[1], "1") || !strcmp(argv[1], "6")));
	failed = argc == 3;
	if (failed) assert(!strcmp(argv[2], "fail"));
	result = update_sdn_by_vpnc(atoi(argv[1]));
	assert(result == (failed ? -1 : 0));
	assert(label_reads == 1 && firewall_updates == (failed ? 0 : 1));
	printf("Actual SDN refresh index %s; result %d; independent firewall updates %d\n",
	       argv[1], result, firewall_updates);
	return 0;
}
