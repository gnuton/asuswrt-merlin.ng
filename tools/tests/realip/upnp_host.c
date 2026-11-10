/* Compile the complete production UPnP service functions with file/process
 * shims. Only /etc/upnp paths are remapped into the test's temporary directory. */
#define UPNP_TEST
#define main realip_test_main
#include "host.c"
#undef main
#define ETHER_ADDR_LEN 6
#define WAN_UNIT_FIRST 0
#define WANS_DUALWAN_IF_NONE 0
static int nvram_set(const char *k, const char *v) { return nvram_pf_set("", k, v); }
static int nvram_set_int(const char *k, int v) { return nvram_pf_set_int("", k, v); }
static int nvram_unset(const char *k)
{
	char name[1024]; path(name, sizeof(name), k); return unlink(name);
}
static char *nvram_get(const char *k) { return nvram_safe_get(k); }
static char *strcat_r(const char *a, const char *b, char *d)
{
	strcpy(d, a); strcat(d, b); return d;
}
static int is_routing_enabled(void) { return !nvram_get_int("test_no_routing"); }
static int wan_primary_ifunit(void) { return nvram_match("wan1_primary", "1") ? 1 : 0; }
#ifdef RTCONFIG_DUALWAN
static int get_dualwan_by_unit(int u)
{
	char p[32]; snprintf(p, sizeof(p), "wan%d_", u); return !nvram_pf_get_int(p, "test_absent");
}
static int is_wan_connect(int u)
{
	char p[32]; snprintf(p, sizeof(p), "wan%d_", u); return nvram_pf_get_int(p, "state_t") == 2;
}
#endif
static int is_nat_enabled(void) { return 1; }
static char *get_lan_hwaddr(void) { return "00:11:22:33:44:55"; }
static char *get_lan_hostname(void) { return "router"; }
static char *get_productid(void) { return "host-router"; }
static int ether_atoe(const char *s, unsigned char *ea)
{
	(void)s; memset(ea, 1, 6); return 1;
}
static int f_read(const char *p, void *buf, int n) { (void)p; memset(buf, 1, n); return n; }
static int vstrsep(char *s, const char *sep, ...)
{
	(void)s; (void)sep; return 0;
}
static const char *upnp_path(const char *p)
{
	static char names[8][1024]; static unsigned slot;
	char *name = names[slot++ % 8];
	if (!strncmp(p, "/etc/upnp", 9)) {
		snprintf(name, 1024, "%s/upnp%s", getenv("REALIP_TEST_DIR"), p + 9);
		return name;
	}
	return p;
}
static FILE *upnp_fopen(const char *p, const char *m) { return fopen(upnp_path(p), m); }
static int upnp_unlink(const char *p) { return unlink(upnp_path(p)); }
static int upnp_mkdir(const char *p, mode_t m) { return mkdir(upnp_path(p), m); }
static int f_exists(const char *p) { return access(upnp_path(p), F_OK) == 0; }
static long fappend(FILE *out, const char *p)
{
	FILE *in = upnp_fopen(p, "r"); int c; long n = 0;
	if (!in) return -1;
	while ((c = fgetc(in)) != EOF) { fputc(c, out); n++; }
	fclose(in); return n;
}
static void append_custom_config(const char *name, FILE *f) { (void)name; (void)f; }
static void use_custom_config(const char *name, const char *p) { (void)name; (void)p; }
static void run_postconf(const char *name, const char *p)
{
	FILE *f;
	(void)name;
	/* Hook output is part of the effective configuration comparison. */
	if (nvram_safe_get("test_postconf")[0] && (f = upnp_fopen(p, "a")) != NULL) {
		fprintf(f, "# hook %s\n", nvram_safe_get("test_postconf")); fclose(f);
	}
}
static int pids(const char *name) { return nvram_get_int(name); }
static void killall_tk(const char *name)
{
	if (pids(name)) mock_eval("stop", name, NULL);
	nvram_set(name, "0");
}
static int test_xstart(const char *name, const char *flag, const char *p)
{
	mock_eval("start", name, flag, p, NULL);
	if (nvram_get_int("test_start_fail")) return -1;
	nvram_set(name, "1"); return 0;
}
#ifdef RTCONFIG_AUPNPC
static void start_aupnpc(void) { mock_eval("start", "aupnpc", NULL); }
static void stop_aupnpc(void) { mock_eval("stop", "aupnpc", NULL); }
#endif
void start_upnp(void);
void stop_upnp(void);
#define getuid() 0
#define fopen upnp_fopen
#define unlink upnp_unlink
#define mkdir upnp_mkdir
#define xstart test_xstart
#ifdef UPNP_WITHOUT_REALIP
#undef RTCONFIG_GETREALIP
#endif
#include "upnp_services.c"
#undef fopen
#undef unlink
#undef mkdir
int main(int argc, char **argv)
{
	if (argc != 2) return 1;
	if (!strcmp(argv[1], "start")) start_upnp();
	else if (!strcmp(argv[1], "stop")) stop_upnp();
	else if (!strcmp(argv[1], "update")) update_upnp();
	else if (!strcmp(argv[1], "restart")) { stop_upnp(); start_upnp(); }
	else return 1;
	return 0;
}
