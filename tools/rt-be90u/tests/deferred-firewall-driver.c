/* The packaged quarantine writer/transaction; native netfilter executes normally. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <amvpn_deferred.h>

char *nvram_get(const char *name) { return getenv(name); }
int nvram_set(const char *name, const char *value) { return setenv(name, value, 1); }
int nvram_unset(const char *name) { return unsetenv(name); }
int nvram_commit(void) { abort(); }

int main(int argc, char **argv)
{
	int family;
	assert(argc == 2);
	if (!strcmp(argv[1], "refresh")) return amvpn_refresh_deferred() ? 1 : 0;
	assert(!strcmp(argv[1], "filter4") || !strcmp(argv[1], "filter6"));
	family = !strcmp(argv[1], "filter4") ? AF_INET : AF_INET6;
	puts("*filter\n:INPUT ACCEPT [0:0]\n:FORWARD ACCEPT [0:0]\n:OUTPUT ACCEPT [0:0]");
	/* The production writer must put quarantine before existing accepts. */
	puts("-A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT");
	puts("-A FORWARD -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT");
	assert(!amvpn_write_deferred_dns(stdout, family));
	puts("COMMIT");
	return 0;
}
