/* Actual image parser with synthetic NVRAM; never reads router settings. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int read_wgc_config_file(const char *path, int unit);
extern char *nvram_default_get(const char *name);
static int writes, loading_default;

char *nvram_get(const char *name)
{
	char *value = getenv(name);
	if (value || loading_default) return value;
	loading_default = 1;
	value = nvram_default_get(name);
	loading_default = 0;
	return value;
}
int nvram_set(const char *name, const char *value) { writes++; return setenv(name, value, 1); }
int nvram_unset(const char *name) { writes++; return unsetenv(name); }
int nvram_commit(void) { return 0; }

#define PRIVATE "BwcHBwcHBwcHBwcHBwcHBwcHBwcHBwcHBwcHBwcHBwc="
#define PUBLIC "CQkJCQkJCQkJCQkJCQkJCQkJCQkJCQkJCQkJCQkJCQk="
#define PSK "CwsLCwsLCwsLCwsLCwsLCwsLCwsLCwsLCwsLCwsLCws="
#define INTERFACE "[Interface]\nPrivateKey = " PRIVATE "\nAddress = 10.77.0.2/32\n"
#define PEER "[Peer]\nPublicKey = " PUBLIC "\nPresharedKey = " PSK "\nAllowedIPs = 0.0.0.0/0,::/0\nEndpoint = [2001:db8::1]:51820\nPersistentKeepalive = 25\n"

static int same(const char *name, const char *value)
{
	return getenv(name) && !strcmp(getenv(name), value);
}

int main(int argc, char **argv)
{
	const char *path = "/tmp/wg-import.conf", *test;
	FILE *fp;
	int unit = 1, good, ret, i;
	if (argc != 2) return 2;
	test = argv[1];
	good = !strcmp(test, "valid") || !strcmp(test, "whitespace") || !strcmp(test, "long-allowedips")
	       || !strcmp(test, "unbracketed-ipv6");
	setenv("wgc1_priv", "keep-existing-private", 1);
	setenv("wgc1_desc", "keep-existing-description", 1);
	setenv("wgc2_priv", "keep-other-profile", 1);
	unlink(path);
	if (strcmp(test, "missing-file")) {
		fp = fopen(path, "w");
		if (!fp) return 2;
		if (!strcmp(test, "valid") || !strcmp(test, "unit-zero") || !strcmp(test, "unit-six"))
			fputs(INTERFACE PEER, fp);
		else if (!strcmp(test, "whitespace"))
			fputs("  # comment\r\n\t[Interface]\r\n PrivateKey\t=\t" PRIVATE
			      "\r\n Address = 10.77.0.2/32 # address\r\n [Peer]\r\n PublicKey = " PUBLIC
			      "\r\n PresharedKey = " PSK "\r\n AllowedIPs = 0.0.0.0/0, ::/0\r\n"
			      " Endpoint = [2001:db8::1]:51820\r\n PersistentKeepalive = 25\r\n", fp);
		else if (!strcmp(test, "empty")) { /* Deliberately empty. */ }
		else if (!strcmp(test, "unbracketed-ipv6"))
			fputs(INTERFACE "[Peer]\nPublicKey = " PUBLIC "\nPresharedKey = " PSK
			      "\nAllowedIPs = 0.0.0.0/0,::/0\nEndpoint = 2001:db8::1:51820\nPersistentKeepalive = 25\n", fp);
		else if (!strcmp(test, "missing-equals"))
			fputs("[Interface]\nPrivateKey\n", fp);
		else if (!strcmp(test, "wrong-key"))
			fputs("[Interface]\nPrivateKey = invalid\nAddress = 10.77.0.2/32\n" PEER, fp);
		else if (!strcmp(test, "invalid-mtu"))
			fputs(INTERFACE "MTU = 65536\n" PEER, fp);
		else if (!strcmp(test, "invalid-endpoint"))
			fputs(INTERFACE "[Peer]\nPublicKey = " PUBLIC "\nAllowedIPs = 0.0.0.0/0\nEndpoint = [2001:db8::1]:65536\n", fp);
		else if (!strcmp(test, "invalid-prefix"))
			fputs(INTERFACE "[Peer]\nPublicKey = " PUBLIC "\nAllowedIPs = ::/129\nEndpoint = vpn.example:51820\n", fp);
		else if (!strcmp(test, "duplicate-key"))
			fputs(INTERFACE "PrivateKey = " PRIVATE "\n" PEER, fp);
		else if (!strcmp(test, "embedded-nul")) {
			fputs(INTERFACE, fp);
			fputc('\0', fp);
			fputs(PEER, fp);
		}
		else if (!strcmp(test, "multiple-peers"))
			fputs(INTERFACE PEER PEER, fp);
		else if (!strcmp(test, "unsupported-option"))
			fputs(INTERFACE "Table = off\n" PEER, fp);
		else if (!strcmp(test, "long-allowedips") || !strcmp(test, "oversized-allowedips")) {
			fputs(INTERFACE "[Peer]\nPublicKey = " PUBLIC "\nPresharedKey = " PSK
			      "\nEndpoint = [2001:db8::1]:51820\nPersistentKeepalive = 25\nAllowedIPs = ", fp);
			for (i = 0; i < (!strcmp(test, "long-allowedips") ? 80 : 400); i++)
				fprintf(fp, "%s10.1.%d.0/24", i ? "," : "", i % 256);
			fputc('\n', fp);
		} else { fclose(fp); return 2; }
		fclose(fp);
	}
	if (!strcmp(test, "unit-zero")) unit = 0;
	if (!strcmp(test, "unit-six")) unit = 6;
	ret = read_wgc_config_file(path, unit);
	if (!good) {
		if (ret >= 0 || writes || !same("wgc1_priv", "keep-existing-private") ||
		    !same("wgc1_desc", "keep-existing-description") || !same("wgc2_priv", "keep-other-profile")) {
			fprintf(stderr, "FAIL %s: rejected imports must preserve NVRAM (result=%d writes=%d)\n", test, ret, writes);
			return 1;
		}
	} else if (ret || !same("wgc1_priv", PRIVATE) || !same("wgc1_ppub", PUBLIC) ||
	           !same("wgc1_psk", PSK) || !same("wgc1_addr", "10.77.0.2/32") ||
	           !same("wgc1_ep_addr", "2001:db8::1") || !same("wgc1_ep_port", "51820") ||
	           !same("wgc1_alive", "25") || !same("wgc2_priv", "keep-other-profile")) {
		fprintf(stderr, "FAIL %s: imported fields/isolation differ (result=%d writes=%d)\n", test, ret, writes);
		return 1;
	} else if (!strcmp(test, "long-allowedips")) {
		char *allowed = getenv("wgc1_aips");
		if (!allowed || strlen(allowed) < 900 || !strstr(allowed, "10.1.79.0/24")) {
			fprintf(stderr, "FAIL long-allowedips: truncated import\n");
			return 1;
		}
	} else if (!same("wgc1_aips", "0.0.0.0/0,::/0")) {
		fprintf(stderr, "FAIL %s: AllowedIPs differ\n", test);
		return 1;
	}
	printf("PASS %s\n", test);
	return 0;
}
