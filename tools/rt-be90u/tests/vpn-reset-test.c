/* Compiled rc dispatcher + packaged defaults/reset libraries in a disposable
 * QEMU chroot. Persistence is a file-backed NVRAM substitute, not router flash.
 * Service stops and hooks are recorded; unrelated rc entry points must abort. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rtconfig.h>
#include <bcmnvram.h>
#include <shared.h>
#include <openvpn_config.h>

#if !defined(RTCONFIG_SOC_IPQ53XX) || !defined(RTCONFIG_MULTILAN_CFG)
#error "Test requires the RT-BE90U MULTILAN build configuration"
#endif

extern struct nvram_tuple router_defaults[];
extern void handle_notifications(void);
static struct { char *name, *value; } settings[2048];
static size_t setting_count;
static int commits, stops, events, ends, dpi_refreshes, expected_type, expected_unit, expect_stop;
static const char *types[] = { "server", "client", "wgc" };
static const char *commands[] = { "clearovpnserver", "clearovpnclient", "clearwgclient" };
static const char *key_data = "-----BEGIN TEST KEY-----\nfixture\n-----END TEST KEY-----\n";
static const char *custom_data = "remote fixture.example 443\n";

void fixture_unexpected(const char *name)
{
	fprintf(stderr, "Unexpected service call: %s\n", name);
	exit(1);
}

char *nvram_get(const char *name)
{
	size_t i;
	for (i = 0; i < setting_count; ++i)
		if (!strcmp(settings[i].name, name)) return settings[i].value;
	return NULL;
}

int nvram_set(const char *name, const char *value)
{
	size_t i;
	char *copy = value ? strdup(value) : NULL;
	assert(!value || copy);
	for (i = 0; i < setting_count; ++i)
		if (!strcmp(settings[i].name, name)) break;
	if (i == setting_count) {
		assert(setting_count < sizeof(settings) / sizeof(settings[0]));
		settings[i].name = strdup(name);
		assert(settings[i].name);
		++setting_count;
	}
	free(settings[i].value);
	settings[i].value = copy;
	return 0;
}

int nvram_unset(const char *name) { return nvram_set(name, NULL); }

static void save(void)
{
	size_t i;
	FILE *f = fopen("/tmp/nvram.db", "wb");
	assert(f);
	for (i = 0; i < setting_count; ++i) {
		if (!settings[i].value) continue;
		assert(fwrite(settings[i].name, strlen(settings[i].name) + 1, 1, f) == 1);
		assert(fwrite(settings[i].value, strlen(settings[i].value) + 1, 1, f) == 1);
	}
	assert(!fclose(f));
}

static void load(void)
{
	char *name = NULL, *value = NULL;
	size_t n = 0, v = 0;
	FILE *f = fopen("/tmp/nvram.db", "rb");
	assert(f);
	while (getdelim(&name, &n, '\0', f) > 0) {
		assert(getdelim(&value, &v, '\0', f) > 0);
		nvram_set(name, value);
	}
	assert(!ferror(f));
	assert(!fclose(f));
	free(name);
	free(value);
}

int nvram_commit(void)
{
	assert(expected_unit > 0 && commits == 0);
	assert(stops == expect_stop && events == expect_stop + 1 && ends == expect_stop);
	++commits;
	save();
	return 0;
}

#undef logmessage
void logmessage(char *head, char *format, ...) { (void)head; (void)format; }
void logmessage_normal(const char *head, char *format, ...) { (void)head; (void)format; }
int system(const char *command) { fixture_unexpected(command); return -1; }

static void stop(int type, int unit)
{
	assert(expect_stop && !stops && !commits && events == 1 && ends == 0);
	assert(type == expected_type && unit == expected_unit);
	++stops;
}

void stop_ovpn_server(int unit) { stop(0, unit); }
void stop_ovpn_client(int unit) { stop(1, unit); }
void stop_wgc(int unit) { stop(2, unit); }

/* The QCA WireGuard stop branch refreshes the DPI service after stopping. */
void start_dpi_engine_service(void)
{
	assert(expected_type == 2 && expect_stop && stops == 1 && !commits);
	assert(events == 1 && ends == 0 && !dpi_refreshes);
	++dpi_refreshes;
}

void run_custom_script(char *name, int timeout, char *action, char *script)
{
	char expected[64];
	int stopping = expect_stop && ends == 0;
	if (stopping) {
		if (expected_type == 2) strcpy(expected, "wgc");
		else snprintf(expected, sizeof(expected), "vpn%s%d", types[expected_type], expected_unit);
	} else snprintf(expected, sizeof(expected), "%s", commands[expected_type]);
	assert(!strcmp(script, expected));
	assert(!strcmp(action, stopping ? "stop" : ""));
	if (!strcmp(name, "service-event")) {
		assert(timeout == 120 && events == ends);
		++events;
	} else {
		assert(!strcmp(name, "service-event-end") && timeout == 0 && events == ends + 1);
		assert(stopping ? stops == 1 : commits == (expected_unit > 0));
		++ends;
	}
}

static int max_unit(int type)
{
	return type == 0 ? OVPN_SERVER_MAX : type == 1 ? OVPN_CLIENT_MAX : WG_CLIENT_MAX;
}

static void equal(const char *name, const char *actual, const char *expected)
{
	if ((!actual != !expected) || (actual && strcmp(actual, expected))) {
		fprintf(stderr, "%s: got [%s], expected [%s]\n", name,
			actual ? actual : "unset", expected ? expected : "unset");
		exit(1);
	}
}

/* Exercise every packaged profile default, including credentials, descriptions,
 * routing options and WireGuard keys. Also require representative fields so a
 * missing/empty defaults table cannot make this pass vacuously. */
static void profile_settings(int seed, int type, int unit)
{
	struct nvram_tuple *t;
	char prefix[32], name[128], sentinel[160];
	int count = 0, reset = type == expected_type && unit == expected_unit;
	snprintf(prefix, sizeof(prefix), "vpn_%s%d_", types[type], unit);
	for (t = router_defaults; t->name; ++t) {
		if (type == 2) {
			if (strncmp(t->name, "wgc_", 4)) continue;
			snprintf(name, sizeof(name), "wgc%d_%s", unit, t->name + 4);
		} else {
			if (strncmp(t->name, prefix, strlen(prefix))) continue;
			snprintf(name, sizeof(name), "%s", t->name);
		}
		++count;
		snprintf(sentinel, sizeof(sentinel), "fixture:%s", name);
		if (seed) nvram_set(name, sentinel);
		else equal(name, nvram_get(name), reset ? (type == 2 ? NULL : t->value) : sentinel);
	}
	assert(count >= (type == 2 ? 16 : 20));
	if (seed) return;
	if (type == 2) snprintf(name, sizeof(name), "wgc%d_priv", unit);
	else snprintf(name, sizeof(name), "%s%s", prefix, type == 0 ? "port" : "password");
	if (!reset) assert(nvram_get(name));
}

static void profile_files(int seed, int type, int unit)
{
	static const ovpn_key_t client_keys[] = { OVPN_CLIENT_STATIC, OVPN_CLIENT_CA,
		OVPN_CLIENT_CERT, OVPN_CLIENT_KEY, OVPN_CLIENT_CRL, OVPN_CLIENT_EXTRA };
	static const ovpn_key_t server_keys[] = { OVPN_SERVER_STATIC, OVPN_SERVER_CA,
		OVPN_SERVER_CA_KEY, OVPN_SERVER_CERT, OVPN_SERVER_KEY, OVPN_SERVER_DH,
		OVPN_SERVER_CRL, OVPN_SERVER_CLIENT_CERT, OVPN_SERVER_CLIENT_KEY, OVPN_SERVER_EXTRA };
	const ovpn_key_t *keys = type == 0 ? server_keys : client_keys;
	size_t i, count = type == 0 ? sizeof(server_keys) / sizeof(server_keys[0]) :
		sizeof(client_keys) / sizeof(client_keys[0]);
	char buffer[4096];
	int reset = type == expected_type && unit == expected_unit;
	if (seed) assert(!set_ovpn_custom(type, unit, (char *)custom_data));
	else equal("custom", get_ovpn_custom(type, unit, buffer, sizeof(buffer)), reset ? "" : custom_data);
	for (i = 0; i < count; ++i) {
		if (seed) assert(!set_ovpn_key(type, unit, keys[i], (char *)key_data, NULL));
		else {
			assert(ovpn_key_exists(type, unit, keys[i]) == !reset);
			equal("key", get_ovpn_key(type, unit, keys[i], buffer, sizeof(buffer)), reset ? "" : key_data);
		}
	}
}

static void check_or_seed(int seed)
{
	int type, unit;
	for (type = 0; type < 3; ++type) {
		char start[16] = "", entry[8];
		for (unit = 1; unit <= max_unit(type); ++unit) {
			profile_settings(seed, type, unit);
			if (type < 2) profile_files(seed, type, unit);
			if (seed || type != expected_type || unit != expected_unit) {
				snprintf(entry, sizeof(entry), "%d,", unit);
				strcat(start, entry);
			}
		}
		if (type < 2) {
			const char *name = type == 0 ? "vpn_serverx_start" : "vpn_clientx_eas";
			if (seed) nvram_set(name, start);
			else equal(name, nvram_get(name), start);
		}
	}
	if (seed) nvram_set("unrelated_setting", "preserved");
	else equal("unrelated_setting", nvram_get("unrelated_setting"), "preserved");
}

int main(int argc, char **argv)
{
	assert(argc >= 2);
	if (!strcmp(argv[1], "seed")) {
		check_or_seed(1);
		save();
		return 0;
	}
	assert(argc >= 4);
	expected_type = atoi(argv[2]);
	expected_unit = atoi(argv[3]);
	assert(expected_type >= 0 && expected_type < 3);
	load();
	if (!strcmp(argv[1], "run")) {
		assert(argc == 6);
		expect_stop = atoi(argv[4]);
		nvram_set("rc_service", argv[5]);
		nvram_set("rc_service_pid", "123");
		handle_notifications();
		assert(commits == (expected_unit > 0));
		assert(stops == expect_stop && events == expect_stop + 1 && ends == events);
		assert(dpi_refreshes == (expect_stop && expected_type == 2));
		equal("rc_service", nvram_safe_get("rc_service"), "");
		equal("rc_service_pid", nvram_safe_get("rc_service_pid"), "");
	} else assert(!strcmp(argv[1], "verify"));
	check_or_seed(0);
	return 0;
}
