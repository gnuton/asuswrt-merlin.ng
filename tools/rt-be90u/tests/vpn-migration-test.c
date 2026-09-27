/* Actual rc init/format/migration objects and packaged VPN libraries, QEMU only. */
#include <assert.h>
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <rtconfig.h>
#include <bcmnvram.h>
#include <shared.h>
#include <vpn_utils.h>
#include <openvpn_config.h>
#include <amvpn_routing.h>

extern int migrate_qca_vpn_config(void);
extern void config_format_compatibility_handler(void);
extern void init_router_defaults(int restore_defaults);
static struct { char *name, *value; } store[16384];
static size_t used;
static int active, sets, commits, injected, hooks, syncs;
static int policy_operation, policy_fd = -1, policy_writes, policy_created, policy_published;
static const char *fault = "";

void fixture_unexpected(const char *name)
{
	fprintf(stderr, "Unexpected migration dependency: %s\n", name);
	exit(1);
}

char *nvram_get(const char *name)
{
	size_t i;
	for (i = 0; i < used; ++i) if (!strcmp(store[i].name, name)) return store[i].value;
	return NULL;
}

int nvram_set(const char *name, const char *value)
{
	size_t i;
	char *copy;
	if (active && ++sets == atoi(fault)) { ++injected; return -1; }
	copy = value ? strdup(value) : NULL;
	assert(!value || copy);
	for (i = 0; i < used; ++i) if (!strcmp(store[i].name, name)) break;
	if (i == used) {
		assert(used < sizeof(store) / sizeof(store[0]));
		store[i].name = strdup(name);
		assert(store[i].name);
		++used;
	}
	free(store[i].value);
	store[i].value = copy;
	return 0;
}

int nvram_unset(const char *name) { return nvram_set(name, NULL); }

static void save(const char *path)
{
	size_t i;
	FILE *f = fopen(path, "wb");
	assert(f);
	for (i = 0; i < used; ++i) if (store[i].value) {
		assert(fwrite(store[i].name, strlen(store[i].name) + 1, 1, f) == 1);
		assert(fwrite(store[i].value, strlen(store[i].value) + 1, 1, f) == 1);
	}
	assert(!fclose(f));
}

static void load(void)
{
	char *key = NULL, *value = NULL;
	size_t k = 0, v = 0;
	FILE *f = fopen("/tmp/nvram.db", "rb");
	assert(f);
	while (getdelim(&key, &k, 0, f) > 0) {
		assert(getdelim(&value, &v, 0, f) > 0);
		assert(!nvram_set(key, value));
	}
	assert(!ferror(f));
	fclose(f);
	free(key); free(value);
}

int nvram_commit(void)
{
	++commits;
	if (!strcmp(fault, "crash")) _exit(77);
	if (!strcmp(fault, "commit")) { ++injected; return -1; }
	save("/tmp/nvram.db");
	return 0;
}

int __real_fsync(int fd);
int __wrap_fsync(int fd)
{
	++syncs;
	if ((!strcmp(fault, "fsync") && syncs == 1) ||
	    (!strncmp(fault, "sync", 4) && syncs == atoi(fault + 4))) {
		++injected; errno = ENOSPC; return -1;
	}
	return __real_fsync(fd);
}

/* Interpose calls from the packaged libovpn, not just linked rc objects. */
int mkstemp(char *path)
{
	int (*real_mkstemp)(char *) = dlsym(RTLD_NEXT, "mkstemp");
	int fd;
	assert(real_mkstemp);
	fd = real_mkstemp(path);
	if (policy_operation) { policy_fd = fd; policy_created = fd >= 0; }
	return fd;
}

ssize_t write(int fd, const void *buffer, size_t length)
{
	if (policy_operation && fd == policy_fd) {
		++policy_writes;
		if (!strcmp(fault, "policy-write") ||
		    (!strcmp(fault, "policy-partial") && policy_writes > 1)) {
			++injected; errno = ENOSPC; return -1;
		}
		if (!strcmp(fault, "policy-zero")) { ++injected; return 0; }
		if (!strcmp(fault, "policy-partial") && length > 1) length /= 2;
	}
	return syscall(SYS_write, fd, buffer, length);
}

int fsync(int fd)
{
	if (policy_operation && ((!strcmp(fault, "policy-fsync") && fd == policy_fd) ||
	    (!strcmp(fault, "policy-parent-sync") && !policy_created) ||
	    (!strcmp(fault, "policy-dir-sync") && policy_published))) {
		++injected; errno = EIO; return -1;
	}
	return syscall(SYS_fsync, fd);
}

int close(int fd)
{
	int was_policy = policy_operation && fd == policy_fd;
	int result = syscall(SYS_close, fd);
	if (was_policy) policy_fd = -1;
	if (was_policy && !strcmp(fault, "policy-close")) {
		++injected; errno = EIO; return -1;
	}
	return result;
}

int rename(const char *from, const char *to)
{
	int result;
	if (policy_operation && !strcmp(fault, "policy-rename")) {
		++injected; errno = ENOSPC; return -1;
	}
	result = syscall(SYS_renameat, AT_FDCWD, from, AT_FDCWD, to);
	if (policy_operation && !result) policy_published = 1;
	return result;
}

int __real_link(const char *from, const char *to);
int __wrap_link(const char *from, const char *to)
{
	if (!strcmp(fault, "link")) { ++injected; errno = ENOSPC; return -1; }
	return __real_link(from, to);
}

#undef logmessage
void logmessage(char *head, char *format, ...) { (void)head; (void)format; }
void logmessage_normal(const char *head, char *format, ...) { (void)head; (void)format; }
void __wrap_adjust_url_urlelist(void) { hooks |= 1; }
void __wrap_adjust_ddns_config(void) { hooks |= 2; }
void __wrap_adjust_access_restrict_config(void) { hooks |= 4; }
void __wrap_sync_nc_conf(void) { hooks |= 8; }
void adjust_sdn0_vpnc_idx(void) { fixture_unexpected("legacy SDN remapping"); }

int main(int argc, char **argv)
{
	int result = 0;
	char rules[8000];
	FILE *f;
	VPNC_PROFILE profiles[MAX_VPNC_PROFILE];
	assert(argc >= 2);
	load();
	fault = argc > 2 ? argv[2] : "";
	active = 1;
	if (!strcmp(argv[1], "migrate")) result = migrate_qca_vpn_config();
	else if (!strcmp(argv[1], "defaults") || !strcmp(argv[1], "factory-defaults")) {
		init_router_defaults(!strcmp(argv[1], "factory-defaults"));
		nvram_commit();
	}
	else if (!strcmp(argv[1], "boot")) {
		config_format_compatibility_handler();
#ifdef RTCONFIG_NOTIFICATION_CENTER
		assert(hooks == 15);
#else
		assert(hooks == 7);
#endif
	} else if (!strcmp(argv[1], "reader")) {
		int count = vpnc_load_profile(profiles, MAX_VPNC_PROFILE, VPNC_PROFILE_VER1);
		assert(count >= 0);
		if (!strcmp(nvram_safe_get("vpnc_pptp_options_x_list"), "<+mppe-128")) {
			assert(count == 1 && profiles[0].protocol == VPNC_PROTO_PPTP);
			assert(profiles[0].config.pptp.option == VPNC_PPTP_OPT_MPPE128);
		}
	} else if (!strcmp(argv[1], "reset")) {
		reset_ovpn_setting(OVPN_TYPE_CLIENT, 1, 1);
		reset_wgc_setting(1);
	} else if (!strcmp(argv[1], "policy")) {
		assert(argc == 4);
		policy_operation = 1;
		result = amvpn_set_policy_rules(argv[3]);
		policy_operation = 0;
		if (!result) nvram_commit();
	} else assert(!strcmp(argv[1], "read"));
	save("/tmp/result.db");
	f = fopen("/tmp/report.json", "w"); assert(f);
	fprintf(f, "{\"result\":%d,\"sets\":%d,\"commits\":%d,\"injected\":%d}\n", result, sets, commits, injected);
	assert(!fclose(f));
	f = fopen("/tmp/rules", "w"); assert(f);
	fputs(amvpn_get_policy_rules(-1, rules, sizeof(rules), VPNDIR_PROTO_NONE), f);
	assert(!fclose(f));
	return 0;
}
