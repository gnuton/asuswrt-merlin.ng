#include <assert.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "rc.h"

static const char *setting = "";
static int calls;
static char args[6][128];

int nvram_match(const char *name, const char *value)
{
	assert(!strcmp(name, "jffs2_scripts"));
	return !strcmp(setting, value);
}

char *safe_getenv(const char *name)
{
	char *value = getenv(name);
	return value ? value : "";
}

void logmessage(const char *name, const char *format, ...) { }
void ovpn_up_handler(void) { }
void ovpn_down_handler(void) { }
void ovpn_route_up_handler(void) { }
void ovpn_route_pre_down_handler(void) { }
void update_resolvconf(void) { }

int test_eval(const char *cmd, ...)
{
	va_list ap;
	int i;
	assert(!strcmp(cmd, "/jffs/scripts/openvpn-event"));
	va_start(ap, cmd);
	for (i = 0; i < 6; i++)
		snprintf(args[i], sizeof(args[i]), "%s", va_arg(ap, char *));
	va_end(ap);
	calls++;
	return 0;
}

int main(void)
{
	FILE *fp;
	mkdir("/jffs/scripts", 0755);
	fp = fopen("/jffs/scripts/openvpn-event", "w");
	assert(fp);
	fputs("#!/bin/sh\n", fp);
	fclose(fp);
	chmod("/jffs/scripts/openvpn-event", 0700);
	setenv("dev", "tun11", 1);
	run_ovpn_event_script();
	assert(!calls);
	setting = "0";
	run_ovpn_event_script();
	assert(!calls);
	setting = "1";
	chmod("/jffs/scripts/openvpn-event", 0600);
	run_ovpn_event_script();
	assert(!calls);
	chmod("/jffs/scripts/openvpn-event", 0700);
	setenv("dev", "eth0", 1);
	run_ovpn_event_script();
	assert(!calls);
	setenv("dev", "tun11", 1);
	setenv("tun_mtu", "1500", 1);
	setenv("tap_mtu", "1400", 1);
	setenv("link_mtu", "1550", 1);
	setenv("ifconfig_local", "10.0.0.2", 1);
	setenv("ifconfig_remote", "10.0.0.1", 1);
	setenv("script_context", "two words; $(literal)", 1);
	run_ovpn_event_script();
	assert(calls == 1);
	assert(!strcmp(args[0], "tun11") && !strcmp(args[1], "1500"));
	assert(!strcmp(args[2], "1550") && !strcmp(args[3], "10.0.0.2"));
	assert(!strcmp(args[4], "10.0.0.1"));
	assert(!strcmp(args[5], "two words; $(literal)"));
	setenv("dev", "tap21", 1);
	run_ovpn_event_script();
	assert(calls == 2 && !strcmp(args[1], "1400"));
	puts("OpenVPN event argument/enable behavior passed");
	return 0;
}
