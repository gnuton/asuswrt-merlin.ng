/* Run only in the disposable test container: /jffs is a tmpfs. */
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "shared.h"

static const char *setting = "";
static int calls, wait_seconds, background, copies;
static char args[3][256];

int nvram_match(const char *name, const char *value)
{
	assert(!strcmp(name, "jffs2_scripts"));
	return !strcmp(setting, value);
}

int f_exists(const char *path) { return access(path, F_OK) == 0; }
int d_exists(const char *path)
{
	struct stat st;
	return !stat(path, &st) && S_ISDIR(st.st_mode);
}

void logmessage(const char *name, const char *format, ...) { }

int _eval(char *const argv[], const char *path, int timeout, int *pid)
{
	int i;
	calls++;
	wait_seconds = timeout;
	background = pid != NULL;
	memset(args, 0, sizeof(args));
	for (i = 0; i < 3 && argv[i]; i++)
		snprintf(args[i], sizeof(args[i]), "%s", argv[i]);
	return 0;
}

int test_eval(const char *cmd, ...)
{
	va_list ap;
	const char *source, *target;
	assert(!strcmp(cmd, "cp"));
	va_start(ap, cmd);
	source = va_arg(ap, const char *);
	target = va_arg(ap, const char *);
	assert(!strcmp(source, "/jffs/configs/dnsmasq.conf"));
	assert(!strcmp(target, "/tmp/dnsmasq.conf"));
	va_end(ap);
	copies++;
	return 0;
}

static void write_file(const char *path, const char *value, int mode)
{
	FILE *fp = fopen(path, "w");
	assert(fp);
	fputs(value, fp);
	assert(!fclose(fp));
	assert(!chmod(path, mode));
}

static void expect_config(const char *enabled, const char *expected)
{
	char buf[128] = {0};
	FILE *fp = tmpfile();
	assert(fp);
	setting = enabled;
	fputs("base\n", fp);
	append_custom_config("dnsmasq.conf", fp);
	rewind(fp);
	fread(buf, 1, sizeof(buf) - 1, fp);
	assert(!strcmp(buf, expected));
	fclose(fp);
}

int main(void)
{
	setup_jffs_dirs();
	setup_jffs_dirs();
	assert(d_exists("/jffs/scripts"));
	assert(d_exists("/jffs/configs"));
	assert(d_exists("/jffs/addons"));
	write_file("/jffs/scripts/service-event", "#!/bin/sh\n", 0700);
	/* Upgrade from stock: missing setting must not enable arbitrary scripts. */
	run_custom_script("service-event", 120, "restart", "dnsmasq");
	assert(calls == 0);
	setting = "0";
	run_custom_script("service-event", 120, "restart", "dnsmasq");
	assert(calls == 0);
	setting = "1";
	chmod("/jffs/scripts/service-event", 0600);
	run_custom_script("service-event", 120, "restart", "dnsmasq");
	assert(calls == 0);
	chmod("/jffs/scripts/service-event", 0700);
	run_custom_script("service-event", 120, "restart", "two words; $(literal)");
	assert(calls == 1 && wait_seconds == 120 && !background);
	assert(!strcmp(args[0], "/jffs/scripts/service-event"));
	assert(!strcmp(args[1], "restart"));
	assert(!strcmp(args[2], "two words; $(literal)"));
	run_custom_script("missing", 0, NULL, NULL);
	assert(calls == 1);
	run_custom_script("service-event", 0, NULL, NULL);
	assert(calls == 2 && background && !wait_seconds);
	assert(!args[1][0] && !args[2][0]);
	write_file("/jffs/scripts/dnsmasq.postconf", "#!/bin/sh\n", 0700);
	run_postconf("dnsmasq", "/etc/dnsmasq.conf");
	assert(calls == 3 && wait_seconds == 120 && !background);
	assert(!strcmp(args[0], "/jffs/scripts/dnsmasq.postconf"));
	assert(!strcmp(args[1], "/etc/dnsmasq.conf"));
	write_file("/jffs/configs/dnsmasq.conf.add", "custom\n", 0600);
	expect_config("", "base\n");
	expect_config("0", "base\n");
	expect_config("1", "base\ncustom\n");
	write_file("/jffs/configs/dnsmasq.conf", "replacement\n", 0600);
	setting = "0";
	use_custom_config("dnsmasq.conf", "/tmp/dnsmasq.conf");
	assert(copies == 0);
	setting = "1";
	use_custom_config("dnsmasq.conf", "/tmp/dnsmasq.conf");
	assert(copies == 1);
	puts("Custom script/config behavior passed");
	return 0;
}
