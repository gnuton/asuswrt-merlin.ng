/* Actual compiled SDN entry points; only independent services are substituted. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtconfig.h>
#include <shared.h>
#include <sdn.h>

static unsigned int firewall_mask;
static int resolver_calls, commits;
extern int change_default_wan(void);
const char dmservers[] = ""; /* Inactive DNS generation branch. */

char *nvram_get(const char *name) { return getenv(name); }
int nvram_set(const char *name, const char *value) { return setenv(name, value, 1); }
int nvram_unset(const char *name) { return unsetenv(name); }
int nvram_commit(void) { ++commits; return 0; }

void fixture_unexpected(const char *name)
{
	fprintf(stderr, "Unexpected service: %s\n", name);
	abort();
}

int get_drop_accept(char *drop, size_t drop_size, char *accept, size_t accept_size)
{
	snprintf(drop, drop_size, "DROP");
	snprintf(accept, accept_size, "ACCEPT");
	return 0;
}

int update_SDN_iptables(const MTLAN_T *sdn, const char *drop, const char *accept)
{
	int index = sdn->sdn_t.sdn_idx;
	char name[8];
	assert(index >= 0 && index <= 2);
	snprintf(name, sizeof(name), "br%d", index);
	assert(!strcmp(sdn->nw_t.ifname, name));
	assert(!strcmp(drop, "DROP") && !strcmp(accept, "ACCEPT"));
	firewall_mask |= 1U << index;
	return 0;
}

/* Main-LAN resolver regeneration is observed, outside these IPv4 route tests. */
int update_resolvconf(void) { ++resolver_calls; return 0; }

/* Firewall generation is an observed independent service boundary. */
int reset_sdn_firewall(unsigned long features, int index)
{
	assert(features == SDN_FEATURE_VPNC && index == 0);
	return 0;
}
int create_iptables_file(int type, int index, FILE **fp, char *path, size_t size, int v6)
{
	assert(type >= 0 && type <= 2 && index == 0 && !v6);
	snprintf(path, size, "/tmp/default-firewall-%d", type);
	*fp = tmpfile();
	assert(*fp);
	return 0;
}
int close_n_restore_iptables_file(int type, FILE **fp, const char *path, int v6)
{
	assert(type >= 0 && type <= 2 && !v6 && path && *fp);
	fclose(*fp);
	*fp = NULL;
	return 0;
}
/* Independent service notifications; route operations use real rc/common.o. */
void stop_aae_sip_conn(int mode) { assert(mode == 1); }
void reload_dnsmasq(int index) { assert(index == ALL_SDN); ++resolver_calls; }
void update_wgc_by_sdn(MTLAN_T *mtl, size_t count, int all)
{
	assert(mtl && count == 1 && !all && mtl[0].sdn_t.sdn_idx == 0);
}

int main(int argc, char **argv)
{
	int result, index;
	assert(argc == 3);
	index = atoi(argv[2]);
	if (!strcmp(argv[1], "default")) {
		assert(index == 0 || index == 1 || index == 6);
		nvram_set("vpnc_default_wan_tmp", argv[2]);
		result = change_default_wan();
		assert(commits == 1);
		assert(!strcmp(nvram_safe_get("vpnc_default_wan"), argv[2]));
		assert(!*nvram_safe_get("vpnc_default_wan_tmp"));
		printf("NVRAM vpnc_default_wan=%s\nNVRAM sdn_rl=%s\n",
		       nvram_safe_get("vpnc_default_wan"), nvram_safe_get("sdn_rl"));
	} else if (!strcmp(argv[1], "wan")) {
		assert(index == ALL_SDN || (index >= 0 && index <= 2));
		result = handle_sdn_feature(index, SDN_FEATURE_WAN, 0);
	} else {
		assert(!strcmp(argv[1], "vpnc") && (index == 1 || index == 6));
		result = update_sdn_by_vpnc(index);
	}
	assert(result == 0);
	printf("SDN entry %s %d: firewall mask %u, resolver calls %d\n",
	       argv[1], index, firewall_mask, resolver_calls);
	return 0;
}
