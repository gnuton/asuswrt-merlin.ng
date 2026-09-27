/* Full compiled firewall/service entry points in a disposable firmware chroot. */
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rtconfig.h>
#include <shared.h>
#include <amvpn_deferred.h>
#include <amvpn_routing.h>

extern void start_default_filter(int unit);
extern int start_firewall(int wanunit, int lanunit);
extern void filter_setting(int unit, char *lan_if, char *lan_ip, char *accept, char *drop);
extern void filter_setting2(char *lan_if, char *lan_ip, char *accept, char *drop);
extern void handle_notifications(void);
static int unexpected_filter_failure;

/* SDN removes old rules after a full replacement has already removed them.
 * Only these exact deletion-only scripts may return a missing-rule error. */
static int cleanup_restore(char *const argv[])
{
	const char *name, *suffix;
	char line[2048];
	FILE *fp;
	int stage = 0, deletions = 0, valid = 1;
	if (!argv[1] || strcmp(argv[1], "--noflush") || !argv[2] || argv[3]) return 0;
	name = argv[2];
	if (strncmp(name, "/tmp/.sdn/sdn-", 14)) return 0;
	suffix = name + 14;
	if (*suffix < '0' || *suffix > '9') return 0;
	while (*suffix >= '0' && *suffix <= '9') ++suffix;
	if (strcmp(suffix, "-filter") && strcmp(suffix, "-filter-v6") && strcmp(suffix, "-nat")) return 0;
	fp = fopen(name, "r");
	if (!fp) return 0;
	while (fgets(line, sizeof(line), fp)) {
		line[strcspn(line, "\r\n")] = '\0';
		if (!*line) continue;
		if (!stage && !strcmp(line, !strcmp(suffix, "-nat") ? "*nat" : "*filter")) stage = 1;
		else if (stage == 1 && !strncmp(line, "-D ", 3)) ++deletions;
		else if (stage == 1 && !strcmp(line, "COMMIT")) stage = 2;
		else { valid = 0; break; }
	}
	if (ferror(fp)) valid = 0;
	fclose(fp);
	return valid && stage == 2 && deletions;
}

/* Every read sees another process's published configuration; no per-process
 * environment snapshot may substitute for a concurrent router update. */
char *nvram_get(const char *name)
{
	char path[256], *value;
	long size;
	FILE *fp;
	assert(!strchr(name, '/'));
	snprintf(path, sizeof(path), "/tmp/lifecycle-nvram/%s", name);
	fp = fopen(path, "rb");
	if (!fp) return getenv(name);
	assert(!fseek(fp, 0, SEEK_END));
	size = ftell(fp);
	assert(size >= 0 && size < 65536 && !fseek(fp, 0, SEEK_SET));
	value = malloc(size + 1);
	assert(value && fread(value, 1, size, fp) == (size_t)size);
	value[size] = '\0';
	assert(!fclose(fp));
	return value;
}
int nvram_set(const char *name, const char *value)
{
	char path[256], temporary[280];
	FILE *fp;
	assert(!strchr(name, '/'));
	snprintf(path, sizeof(path), "/tmp/lifecycle-nvram/%s", name);
	snprintf(temporary, sizeof(temporary), "%s.%ld", path, (long)getpid());
	fp = fopen(temporary, "wb");
	assert(fp && fputs(value, fp) >= 0 && !fclose(fp));
	assert(!rename(temporary, path));
	return 0;
}
int nvram_unset(const char *name) { return nvram_set(name, ""); }
int nvram_commit(void) { abort(); }

/* The fixture models ordinary operation, never a factory/ATE device. */
int IS_ATE_FACTORY_MODE(void) { return 0; }

static void event(const char *phase, const char *detail, int status)
{
	const char *actor = getenv("LIFECYCLE_ACTOR");
	char path[256], ack[32];
	FILE *fp;
	if (!actor) return;
	fp = fopen("/tmp/lifecycle-events", "w");
	assert(fp && fprintf(fp, "%s\t%s\t%d\t%s\n", actor, phase, status, detail) > 0 && !fclose(fp));
	snprintf(path, sizeof(path), "/tmp/lifecycle-ack-%s", actor);
	fp = fopen(path, "r");
	assert(fp && fgets(ack, sizeof(ack), fp) && !strcmp(ack, "continue\n") && !fclose(fp));
}

/* Hardware acceleration is absent from the isolated native-kernel model. */
void reinit_ecm(int unit)
{
	assert(unit == -1);
	event("HARDWARE_CALLBACK", "reinit_ecm", unit);
}

int __real_handle_sdn_feature(int index, unsigned long features, int action);
int __wrap_handle_sdn_feature(int index, unsigned long features, int action)
{
	char detail[96];
	snprintf(detail, sizeof(detail), "%d %lu %d", index, features, action);
	event("SDN_CALLBACK", detail, 0);
	return __real_handle_sdn_feature(index, features, action);
}

int file_lock(const char *tag)
{
	int (*actual)(const char *) = dlsym(RTLD_NEXT, "file_lock");
	int lock;
	assert(actual);
	event("LOCK_BEFORE", tag, 0);
	lock = actual(tag);
	event("LOCK_ACQUIRED", tag, lock);
	return lock;
}
void file_unlock(int lock)
{
	void (*actual)(int) = dlsym(RTLD_NEXT, "file_unlock");
	assert(actual);
	event("LOCK_RELEASE_BEFORE", "", lock);
	actual(lock);
}

/* Observe real command execution, including the snapshot-before-restore
 * boundary. No routing/netfilter result or side effect is substituted. */
int _eval(char *const argv[], const char *path, int timeout, int *pid)
{
	int (*actual)(char *const [], const char *, int, int *) = dlsym(RTLD_NEXT, "_eval");
	char command[2048] = "";
	const char *name = strrchr(argv[0], '/');
	int i, result, observed, cleanup;
	name = name ? name + 1 : argv[0];
	observed = !strcmp(name, "ip") || !strcmp(name, "sed") ||
		strstr(name, "iptables") || strstr(name, "ip6tables");
	cleanup = strstr(name, "tables-restore") && cleanup_restore(argv);
	assert(actual);
	for (i = 0; argv[i]; ++i) {
		assert(strlen(command) + strlen(argv[i]) + 2 < sizeof(command));
		if (i) strcat(command, " ");
		strcat(command, argv[i]);
	}
	if (observed) event("COMMAND_BEFORE", command, 0);
	if (getenv("LIFECYCLE_FAIL_RESTORE") &&
	    !strcmp(command, getenv("LIFECYCLE_FAIL_RESTORE"))) {
		FILE *fp = fopen("/tmp/lifecycle-fault-hit", "w");
		assert(fp && fputs(command, fp) >= 0 && !fclose(fp));
		result = 42; /* One explicitly requested restore fault, no kernel mutation. */
	} else result = actual(argv, path, timeout, pid);
	if (result && strstr(name, "tables-restore")) {
		fprintf(stderr, "%s returned %d: %s\n", cleanup && result == 1 ? "Cleanup restore" : "Restore", result, command);
		if (strstr(command, "/tmp/filter") &&
		    !(getenv("LIFECYCLE_FAIL_RESTORE") &&
		      !strcmp(command, getenv("LIFECYCLE_FAIL_RESTORE"))))
			unexpected_filter_failure = 1;
	}
	if (result && !strcmp(name, "sed")) {
		fprintf(stderr, "sed returned %d: %s\n", result, command);
		unexpected_filter_failure = 1;
	}
	if (observed) event(cleanup ? "CLEANUP_AFTER" : "COMMAND_AFTER", command, result);
	return result;
}

void fixture_unexpected(const char *name)
{
	fprintf(stderr, "Unexpected lifecycle dependency: %s\n", name);
	exit(90);
}

int main(int argc, char **argv)
{
	int result = 0;
	alarm(900);
	assert(argc == 2);
	if (!strcmp(argv[1], "default")) start_default_filter(0);
	else if (!strcmp(argv[1], "normal")) result = start_firewall(0, 0);
	else if (!strcmp(argv[1], "filter")) filter_setting(0, "br0", "192.0.2.1", "ACCEPT", "DROP");
	else if (!strcmp(argv[1], "filter2")) filter_setting2("br0", "192.0.2.1", "ACCEPT", "DROP");
	else if (!strcmp(argv[1], "refresh")) result = amvpn_refresh_deferred();
	else if (!strcmp(argv[1], "ipv6guard")) amvpn_refresh_ipv6_killswitch();
	else {
		assert(!strcmp(argv[1], "restart_firewall") || !strcmp(argv[1], "stop_firewall") ||
		       !strcmp(argv[1], "stop_ddns") ||
		       !strcmp(argv[1], "start_vpnrouting1"));
		nvram_set("rc_service", argv[1]);
		nvram_set("rc_service_pid", "123");
		handle_notifications();
		assert(!*nvram_safe_get("rc_service"));
	}
	printf("Actual lifecycle %s returned %d\n", argv[1], result);
	return result || unexpected_filter_failure ? 1 : 0;
}
