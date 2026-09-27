/* Actual SDN/services objects, real shared locks/routing, shared synthetic NVRAM. */
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rtconfig.h>
#include <shared.h>
#include <openvpn_config.h>
#include <openvpn_control.h>
#include <amvpn_routing.h>

extern int update_sdn_by_vpnc(const int index);
extern void handle_notifications(void);
/* vpn_utils.h declares this shared string without const. */
char vpnc_resolv_path[] = "/tmp/resolv.vpnc%d";
static int observed_lock = -1, service_events;

void fixture_unexpected(const char *name)
{
	fprintf(stderr, "Unexpected service call: %s\n", name);
	exit(1);
}

/* Re-read each value so another process's configuration update is observable.
 * Values are retained for the lifetime of this short-lived test process. */
char *nvram_get(const char *name)
{
	char path[256], *value;
	long length;
	FILE *fp;
	assert(!strchr(name, '/'));
	snprintf(path, sizeof(path), "/tmp/concurrent-nvram/%s", name);
	fp = fopen(path, "rb");
	if (!fp) return getenv(name);
	assert(!fseek(fp, 0, SEEK_END));
	length = ftell(fp);
	assert(length >= 0 && length < 65536 && !fseek(fp, 0, SEEK_SET));
	value = malloc(length + 1);
	assert(value && fread(value, 1, length, fp) == (size_t)length);
	value[length] = '\0';
	assert(!fclose(fp));
	return value;
}

int nvram_set(const char *name, const char *value)
{
	char path[256], temporary[280];
	FILE *fp;
	assert(!strchr(name, '/'));
	snprintf(path, sizeof(path), "/tmp/concurrent-nvram/%s", name);
	snprintf(temporary, sizeof(temporary), "%s.%ld", path, (long)getpid());
	fp = fopen(temporary, "wb");
	assert(fp && fputs(value, fp) >= 0 && !fclose(fp));
	assert(!rename(temporary, path));
	return 0;
}
int nvram_unset(const char *name) { return nvram_set(name, ""); }
int nvram_commit(void) { fixture_unexpected("nvram_commit"); return -1; }

static void event(const char *kind, const char *detail)
{
	const char *actor = getenv("CONCURRENT_ACTOR");
	char path[256], ack[32];
	FILE *fp;
	if (!actor) return;
	fp = fopen("/tmp/concurrent-events", "w");
	assert(fp && fprintf(fp, "%s\t%s\t%s\n", actor, kind, detail) > 0 && !fclose(fp));
	snprintf(path, sizeof(path), "/tmp/concurrent-ack-%s", actor);
	fp = fopen(path, "r");
	assert(fp && fgets(ack, sizeof(ack), fp) && !strcmp(ack, "continue\n") && !fclose(fp));
}

/* Observe real lock calls; never substitute lock results or exclusion. */
int file_lock(const char *tag)
{
	int (*actual)(const char *) = dlsym(RTLD_NEXT, "file_lock");
	int lock;
	assert(actual);
	if (!strcmp(tag, VPNROUTING_LOCK)) event("LOCK_BEFORE", tag);
	lock = actual(tag);
	if (!strcmp(tag, VPNROUTING_LOCK)) {
		assert(lock >= 0);
		observed_lock = lock;
		event("LOCK_ACQUIRED", tag);
	}
	return lock;
}

void file_unlock(int lock)
{
	void (*actual)(int) = dlsym(RTLD_NEXT, "file_unlock");
	assert(actual);
	if (lock == observed_lock && lock >= 0) {
		event("LOCK_RELEASING", VPNROUTING_LOCK);
		observed_lock = -1;
	}
	actual(lock);
}

int get_drop_accept(char *drop, size_t drop_size, char *accept, size_t accept_size)
{
	snprintf(drop, drop_size, "DROP");
	snprintf(accept, accept_size, "ACCEPT");
	return 0;
}

int update_SDN_iptables(const MTLAN_T *sdn, const char *drop, const char *accept)
{
	assert(!strcmp(sdn->nw_t.ifname, "br1"));
	assert(!strcmp(drop, "DROP") && !strcmp(accept, "ACCEPT"));
	return 0;
}

void run_custom_script(char *name, int timeout, char *action, char *script)
{
	assert(!strcmp(script, "vpnrouting1") && !strcmp(action, "start"));
	assert((!strcmp(name, "service-event") && timeout == 120 && service_events == 0) ||
	       (!strcmp(name, "service-event-end") && timeout == 0 && service_events == 1));
	++service_events;
}

int main(int argc, char **argv)
{
	int result;
	alarm(120);
	assert(argc >= 2);
	if (!strcmp(argv[1], "refresh") || !strcmp(argv[1], "refresh-fail")) {
		assert(argc == 3 && (!strcmp(argv[2], "1") || !strcmp(argv[2], "6")));
		result = update_sdn_by_vpnc(atoi(argv[2]));
		printf("Actual SDN refresh index %s returned %d\n", argv[2], result);
		assert(!strcmp(argv[1], "refresh") ? result == 0 : result != 0);
	} else {
		assert(argc == 2 && !strcmp(argv[1], "service"));
		nvram_set("rc_service", "start_vpnrouting1");
		nvram_set("rc_service_pid", "123");
		handle_notifications();
		assert(service_events == 2 && !*nvram_safe_get("rc_service"));
		printf("Actual services.o vpnrouting1 dispatch completed\n");
	}
	return 0;
}
