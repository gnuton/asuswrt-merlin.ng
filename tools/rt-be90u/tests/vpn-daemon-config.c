/* Generate tunnel fixtures with the image's actual libovpn config writers. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtconfig.h>
#include <openvpn_config.h>
#include <openvpn_setup.h>

#if !defined(RTCONFIG_SOC_IPQ53XX) || !defined(RTCONFIG_VPN_FUSION_MERLIN)
#error "Tunnel fixtures require the matching RT-BE90U build configuration"
#endif
_Static_assert(sizeof(((ovpn_cconf_t *)0)->password) == 256,
               "Tunnel fixture must use the IPQ53xx client configuration ABI");

char *nvram_get(const char *name) { return getenv(name); }
int nvram_set(const char *name, const char *value) { return setenv(name, value, 1); }
int nvram_unset(const char *name) { return unsetenv(name); }
int nvram_commit(void) { return 0; }

int main(int argc, char **argv)
{
	ovpn_cconf_t client = {0};
	ovpn_sconf_t server = {0};
	int tls, crypt, crypt_v2;
	assert(argc == 2);
	crypt = !strcmp(argv[1], "tls-crypt");
	crypt_v2 = !strcmp(argv[1], "tls-crypt-v2");
	tls = !strcmp(argv[1], "tls") || crypt || crypt_v2;
	assert(tls || !strcmp(argv[1], "static"));
	setenv("ipv6_service", "disabled", 1);
	setenv("wan0_ipaddr", "10.250.0.2", 1);
	setenv("wan_primary", "0", 1);
	setenv("lan_ipaddr", "192.0.2.1", 1);
	setenv("lan_netmask", "255.255.255.0", 1);
	setenv("buildno", "fixture", 1);
	client.auth_mode = server.auth_mode = tls ? OVPN_AUTH_TLS : OVPN_AUTH_STATIC;
	client.if_type = server.if_type = OVPN_IF_TUN;
	client.port = server.port = 1194;
	client.verb = server.verb = 3;
	client.redirect_gateway = OVPN_RGW_POLICY;
	client.direction = server.direction = -1;
	client.reneg = server.reneg = -1;
	strcpy(client.addr, "10.250.0.2");
	strcpy(client.proto, "udp");
	strcpy(server.proto, "udp");
	strcpy(client.if_name, "tun11");
	strcpy(server.if_name, "tunpeer");
	strcpy(client.local, "10.9.0.1");
	strcpy(client.remote, "10.9.0.2");
	strcpy(server.local, "10.9.0.2");
	strcpy(server.remote, "10.9.0.1");
	strcpy(client.cipher, "AES-256-CBC");
	strcpy(server.cipher, "AES-256-CBC");
	strcpy(client.ncp_ciphers, "AES-256-GCM:AES-128-GCM");
	strcpy(server.ncp_ciphers, "AES-256-GCM:AES-128-GCM");
	strcpy(client.digest, "SHA256");
	strcpy(server.digest, "SHA256");
	strcpy(client.comp, "-1");
	strcpy(server.comp, "-1");
	strcpy(server.network, "10.9.0.0");
	strcpy(server.netmask, "255.255.255.0");
	strcpy(server.lan_ipaddr, "203.0.113.99");
	strcpy(server.lan_netmask, "255.255.255.255");
	if (tls) {
		server.ccd = 1;
		strcpy(server.ccd_val, "<1>rtbe90u-test-client>192.0.2.0>255.255.255.0>0");
		client.verify_x509_type = 1;
		strcpy(client.verify_x509_name, "rtbe90u-test-server");
		/* Exercise a provider supplying a gateway but no usable pushed routes. */
		strcpy(client.custom, "remote-cert-tls server\npull-filter ignore \"route \"\n");
		assert(set_ovpn_key(OVPN_TYPE_SERVER, 1, OVPN_SERVER_DH, "none", NULL) == 0);
		assert(set_ovpn_key(OVPN_TYPE_CLIENT, 1, OVPN_CLIENT_CA, NULL, "/tmp/certs/ca.crt") > 0);
		assert(set_ovpn_key(OVPN_TYPE_CLIENT, 1, OVPN_CLIENT_CERT, NULL, "/tmp/certs/client.crt") > 0);
		assert(set_ovpn_key(OVPN_TYPE_CLIENT, 1, OVPN_CLIENT_KEY, NULL, "/tmp/certs/client.key") > 0);
		assert(set_ovpn_key(OVPN_TYPE_SERVER, 1, OVPN_SERVER_CA, NULL, "/tmp/certs/ca.crt") > 0);
		assert(set_ovpn_key(OVPN_TYPE_SERVER, 1, OVPN_SERVER_CERT, NULL, "/tmp/certs/server.crt") > 0);
		assert(set_ovpn_key(OVPN_TYPE_SERVER, 1, OVPN_SERVER_KEY, NULL, "/tmp/certs/server.key") > 0);
		assert(set_ovpn_key(OVPN_TYPE_SERVER, 1, OVPN_SERVER_CLIENT_CERT, NULL, "/tmp/certs/client.crt") > 0);
		assert(set_ovpn_key(OVPN_TYPE_SERVER, 1, OVPN_SERVER_CLIENT_KEY, NULL, "/tmp/certs/client.key") > 0);
		if (crypt) {
			client.direction = server.direction = 3;
			client.tlscrypt = server.tlscrypt = 1;
			assert(set_ovpn_key(OVPN_TYPE_CLIENT, 1, OVPN_CLIENT_STATIC, NULL, "/tmp/static.key") > 0);
			assert(set_ovpn_key(OVPN_TYPE_SERVER, 1, OVPN_SERVER_STATIC, NULL, "/tmp/static.key") > 0);
		} else if (crypt_v2) {
			client.direction = 3;
			client.tlscrypt = 2;
			assert(set_ovpn_key(OVPN_TYPE_CLIENT, 1, OVPN_CLIENT_STATIC, NULL, "/tmp/tlscrypt-client.key") > 0);
			/* The server UI offers v1; emulate a v2 provider through custom config. */
			strcpy(server.custom, "tls-crypt-v2 /tmp/tlscrypt-server.key\n");
		}
	} else {
		assert(set_ovpn_key(OVPN_TYPE_CLIENT, 1, OVPN_CLIENT_STATIC, NULL, "/tmp/static.key") > 0);
		assert(set_ovpn_key(OVPN_TYPE_SERVER, 1, OVPN_SERVER_STATIC, NULL, "/tmp/static.key") > 0);
	}
	assert(ovpn_write_server_config(&server, 1) == 0);
	assert(ovpn_write_client_config(&client, 1) == 0);
	ovpn_write_client_keys(&client, 1);
	ovpn_write_server_keys(&server, 1);
	puts("Generated server and client configurations through ARM libovpn");
	return 0;
}
