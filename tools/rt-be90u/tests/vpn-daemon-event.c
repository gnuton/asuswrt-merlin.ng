/* Native test scripts dispatch the real ARM library hooks through QEMU. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openvpn_config.h>
#include <openvpn_control.h>

char *nvram_get(const char *name) { return getenv(name); }
int nvram_set(const char *name, const char *value) { return setenv(name, value, 1); }
int nvram_unset(const char *name) { return unsetenv(name); }
int nvram_commit(void) { return 0; }

int main(int argc, char **argv)
{
	char path[80];
	FILE *fp;
	assert(argc >= 2);
	if (!strcmp(argv[1], "route-up")) {
		ovpn_client_route_up_handler();
	} else if (!strcmp(argv[1], "route-pre-down")) {
		ovpn_client_route_pre_down_handler();
	} else {
		assert(argc >= 4 && !strcmp(argv[2], "1"));
		if (!strcmp(argv[3], "client")) {
			if (!strcmp(argv[1], "up"))
				ovpn_client_up_handler(1);
			else
				ovpn_client_down_handler(1);
		} else {
			assert(!strcmp(argv[3], "server"));
			if (!strcmp(argv[1], "up"))
				ovpn_server_up_handler(1);
			else
				ovpn_server_down_handler(1);
		}
		snprintf(path, sizeof(path), "/tmp/hook-%s-%s", argv[1], argv[3]);
		fp = fopen(path, "w");
		assert(fp);
		fputs("complete\n", fp);
		fclose(fp);
	}
	return 0;
}
