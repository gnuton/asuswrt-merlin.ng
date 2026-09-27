/* Test-only NVRAM/process replacements for the unmodified image HTTP server. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

extern char *nvram_default_get(const char *name);

char *nvram_get(const char *name)
{
	char key[256];
	char *value;
	static int loading_default;
	if (!name || snprintf(key, sizeof(key), "RTBE90U_NVRAM_%s", name) >= (int)sizeof(key))
		return NULL;
	value = getenv(key);
	if (value || loading_default) return value;
	loading_default = 1;
	value = nvram_default_get(name);
	loading_default = 0;
	return value;
}

int nvram_set(const char *name, const char *value)
{
	char key[256];
	if (!name || snprintf(key, sizeof(key), "RTBE90U_NVRAM_%s", name) >= (int)sizeof(key))
		return -1;
	return setenv(key, value ? value : "", 1);
}

int nvram_unset(const char *name) { return nvram_set(name, ""); }
int nvram_init(void) { return 0; }
int nvram_commit(void) { return 0; }
int nvram_getall(char *buffer, int count)
{
	memset(buffer, 0, count);
	return 0;
}

int notify_rc(const char *event)
{
	fprintf(stderr, "TEST service event: %s\n", event);
	return 0;
}

int notify_rc_and_wait(const char *event) { return notify_rc(event); }
int notify_rc_and_wait_2min(const char *event) { return notify_rc(event); }
int notify_rc_after_period_wait(const char *event, int wait)
{
	(void)wait;
	return notify_rc(event);
}

int _eval(char *const argv[], const char *path, int timeout, pid_t *pid)
{
	(void)path;
	(void)timeout;
	if (pid) *pid = 0;
	fprintf(stderr, "TEST skipped program: %s\n", argv[0]);
	return 0;
}

int system(const char *command)
{
	fprintf(stderr, "TEST skipped shell: %s\n", command);
	return 0;
}

FILE *popen(const char *command, const char *mode)
{
	(void)command;
	return fopen("/dev/null", mode);
}

int pclose(FILE *stream) { return fclose(stream); }
