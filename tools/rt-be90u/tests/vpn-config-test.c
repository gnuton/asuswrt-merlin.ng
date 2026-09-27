/* Run the target libovpn under QEMU in a disposable chroot. No router NVRAM. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <rtconfig.h>
#include <openvpn_config.h>
#include <amvpn_routing.h>

#if !defined(RTCONFIG_SOC_IPQ53XX) || !defined(RTCONFIG_VPN_FUSION_MERLIN)
#error "Test requires the RT-BE90U Merlin VPN build configuration"
#endif

/* An empty defaults table; only the terminating name is read by reset. */
const char *router_defaults[] = { NULL };
static int command_count;
static char last_command[512];

/* Capture generated routing commands; never change the container's routes. */
int system(const char *command)
{
	assert(command != NULL);
	assert(strlen(command) < sizeof(last_command));
	strcpy(last_command, command);
	command_count++;
	return 0;
}

char *nvram_get(const char *name)
{
	return getenv(name);
}

int nvram_set(const char *name, const char *value)
{
	return setenv(name, value, 1);
}

int nvram_unset(const char *name)
{
	return unsetenv(name);
}

int nvram_commit(void)
{
	return 0;
}

int main(void)
{
	char password[201], buffer[4096];
	ovpn_cconf_t *client;
	struct stat st;
	const char *key = "-----BEGIN TEST KEY-----\nfixture\n-----END TEST KEY-----\n";
	char rules[] = "<1>client one>192.0.2.10>>OVPN1"
		"<0>disabled>192.0.2.11>>OVPN1"
		"<1>client two>192.0.2.12>>OVPN2"
		"<1>wireguard>192.0.2.13>>WGC1"
		"<1>wan>192.0.2.14>>WAN<broken";

	memset(password, 'p', sizeof(password) - 1);
	password[sizeof(password) - 1] = '\0';
	assert(nvram_set("vpn_client1_if", "tun") == 0);
	assert(nvram_set("vpn_client1_username", "test-user") == 0);
	assert(nvram_set("vpn_client1_password", password) == 0);
	assert(nvram_set("vpn_client1_custom", "remote legacy.example 1194\n") == 0);
	client = ovpn_get_cconf(1);
	assert(client != NULL);
	assert(strcmp(client->if_name, "tun11") == 0);
	assert(strcmp(client->username, "test-user") == 0);
	assert(strcmp(client->password, password) == 0);
	assert(strcmp(client->custom, "remote legacy.example 1194\n") == 0);
	free(client);

	assert(set_ovpn_custom(OVPN_TYPE_CLIENT, 1, "remote saved.example 443\n") == 0);
	assert(strcmp(get_ovpn_custom(OVPN_TYPE_CLIENT, 1, buffer, sizeof(buffer)),
		"remote saved.example 443\n") == 0);
	assert(set_ovpn_custom(OVPN_TYPE_CLIENT, 1, "") == 0);
	assert(get_ovpn_custom(OVPN_TYPE_CLIENT, 1, buffer, sizeof(buffer))[0] == '\0');
	assert(amvpn_set_policy_rules(rules) == 0);
	assert(strcmp(amvpn_get_policy_rules(-1, buffer, sizeof(buffer), VPNDIR_PROTO_NONE), rules) == 0);
	assert(strcmp(amvpn_get_policy_rules(1, buffer, sizeof(buffer), VPNDIR_PROTO_OPENVPN),
		"<1>client one>192.0.2.10>>OVPN1<0>disabled>192.0.2.11>>OVPN1") == 0);
	command_count = 0;
	_write_routing_rules(1, buffer, 0, VPNDIR_PROTO_OPENVPN);
	assert(command_count == 1);
	assert(strstr(last_command, "from 192.0.2.10") != NULL);
	assert(strstr(last_command, "table ovpnc1 priority 10210") != NULL);
	assert(strcmp(amvpn_get_policy_rules(1, buffer, sizeof(buffer), VPNDIR_PROTO_WIREGUARD),
		"<1>wireguard>192.0.2.13>>WGC1") == 0);
	assert(strcmp(amvpn_get_policy_rules(0, buffer, sizeof(buffer), VPNDIR_PROTO_NONE),
		"<1>wan>192.0.2.14>>WAN") == 0);
	assert(amvpn_get_policy_rules(5, buffer, sizeof(buffer), VPNDIR_PROTO_OPENVPN)[0] == '\0');

	/* Key enum changes must still select the correct persistent filename. */
	get_ovpn_filename(OVPN_TYPE_CLIENT, 1, OVPN_CLIENT_KEY, buffer, sizeof(buffer));
	assert(strcmp(buffer, "vpn_crt_client1_key") == 0);
	get_ovpn_filename(OVPN_TYPE_CLIENT, 1, OVPN_CLIENT_EXTRA, buffer, sizeof(buffer));
	assert(strcmp(buffer, "vpn_crt_client1_extra") == 0);
	get_ovpn_filename(OVPN_TYPE_SERVER, 2, OVPN_SERVER_KEY, buffer, sizeof(buffer));
	assert(strcmp(buffer, "vpn_crt_server2_key") == 0);
	assert(set_ovpn_key(OVPN_TYPE_CLIENT, 1, OVPN_CLIENT_KEY, (char *)key, NULL) == 0);
	assert(strcmp(get_ovpn_key(OVPN_TYPE_CLIENT, 1, OVPN_CLIENT_KEY, buffer, sizeof(buffer)), key) == 0);
	assert(stat("/jffs/openvpn/vpn_crt_client1_key", &st) == 0);
	assert((st.st_mode & 0777) == 0600);
	assert(ovpn_key_exists(OVPN_TYPE_CLIENT, 1, OVPN_CLIENT_KEY));
	reset_ovpn_setting(OVPN_TYPE_CLIENT, 1, 1);
	assert(!ovpn_key_exists(OVPN_TYPE_CLIENT, 1, OVPN_CLIENT_KEY));
	assert(get_ovpn_custom(OVPN_TYPE_CLIENT, 1, buffer, sizeof(buffer))[0] == '\0');
	puts("Target VPN configuration tests passed");
	return 0;
}
