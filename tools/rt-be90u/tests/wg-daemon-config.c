/* Compile the SDK's exact WG config functions; synthetic NVRAM, real libraries. */
#define _GNU_SOURCE
#include <assert.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <rtconfig.h>
#include <shutils.h>
#include <shared.h>
#include <vpn_utils.h>

char *nvram_get(const char *name) { return getenv(name); }
int nvram_set(const char *name, const char *value) { return setenv(name, value, 1); }
int nvram_unset(const char *name) { return unsetenv(name); }
int nvram_commit(void) { return 0; }

#include "wg-config-source.inc"

int main(int argc, char **argv)
{
	char resolved[128];
	assert(argc == 2);
	setenv("jffs2_scripts", "0", 1);
	if (!strcmp(argv[1], "client")) {
		assert(read_wgc_config_file("/tmp/provider.conf", 1) == 0);
		if (getenv("FIXTURE_UNSET_KEEPALIVE")) nvram_unset("wgc1_alive");
		assert(_wg_resolv_ep(nvram_pf_safe_get("wgc1_", "ep_addr"), resolved, sizeof(resolved)) == 0);
		assert(resolved[0]);
		nvram_pf_set("wgc1_", "ep_addr_r", resolved);
		_wg_client_gen_conf("wgc1_", "/tmp/client.conf");
	} else if (!strcmp(argv[1], "export")) {
		_wg_server_gen_client_conf("wgs1_", "wgs1_c1_", "/tmp/export.conf");
	} else {
		assert(!strcmp(argv[1], "server"));
		_wg_server_gen_conf("wgs1_", "/tmp/server.conf");
	}
	return 0;
}
