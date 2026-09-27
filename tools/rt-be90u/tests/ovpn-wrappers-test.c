/* Link the firmware build's actual OpenVPN rc object. All service effects are
 * recorded by stubs; this does not execute the router's full rc lifecycle. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <rtconfig.h>
#include <shutils.h>
#include <shared.h>
#include <vpn_utils.h>
#include <openvpn_config.h>
#include <openvpn_control.h>

#if !defined(RTCONFIG_SOC_IPQ53XX) || !defined(RTCONFIG_VPN_FUSION_MERLIN) || !defined(RTCONFIG_MULTILAN_CFG)
#error "Test requires the RT-BE90U Merlin VPN and multi-LAN configuration"
#endif

extern void start_ovpn_eas(void);
extern void stop_ovpn_eas(void);
extern void start_ovpn_client(int unit);
extern void stop_ovpn_client(int unit);
extern void start_ovpn_server(int unit);
extern void stop_ovpn_server(int unit);
extern int ovpn_up_main(int argc, char **argv);
extern int ovpn_down_main(int argc, char **argv);
extern int ovpn_route_up_main(int argc, char **argv);
extern int ovpn_route_pre_down_main(int argc, char **argv);

static char calls[1024];

static void record(const char *format, ...)
{
	va_list ap;
	size_t used = strlen(calls);
	int n;
	va_start(ap, format);
	n = vsnprintf(calls + used, sizeof(calls) - used, format, ap);
	va_end(ap);
	assert(n >= 0 && (size_t)n < sizeof(calls) - used);
}

static void expect(const char *expected)
{
	if (strcmp(calls, expected))
		fprintf(stderr, "Expected: %s\nActual: %s\n", expected, calls);
	assert(!strcmp(calls, expected));
	calls[0] = '\0';
}

void ovpn_process_eas(int start) { record("eas:%d;", start); }
void ovpn_start_client(int unit) { record("start-client:%d;", unit); }
void ovpn_stop_client(int unit) { record("stop-client:%d;", unit); }
void ovpn_start_server(int unit) { record("start-server:%d;", unit); }
void ovpn_stop_server(int unit) { record("stop-server:%d;", unit); }
void ovpn_client_up_handler(int unit) { record("client-up:%d;", unit); }
void ovpn_client_down_handler(int unit) { record("client-down:%d;", unit); }
void ovpn_server_up_handler(int unit) { record("server-up:%d;", unit); }
void ovpn_server_down_handler(int unit) { record("server-down:%d;", unit); }
void ovpn_client_route_up_handler(void) { record("route-up;"); }
void ovpn_client_route_pre_down_handler(void) { record("route-pre-down;"); }
int update_resolvconf(void) { record("resolv;"); return 0; }
int get_vpnc_idx_by_proto_unit(VPN_PROTO_T proto, int unit)
{
	assert(proto == VPN_PROTO_OVPN);
	return 100 + unit;
}
void _vpnc_ipset_create(int idx) { record("ipset-create:%d;", idx); }
void _vpnc_ipset_destroy(int idx) { record("ipset-destroy:%d;", idx); }
void vpnc_ovpn_set_dns(int unit) { record("dns:%d;", unit); }
int _gen_vpnc_resolv_conf(int idx) { record("vpnc-resolv:%d;", idx); return 0; }
int update_sdn_by_vpnc(const int idx) { record("sdn:%d;", idx); return 0; }
int clean_vpnc_setting_value(const int idx) { record("clean:%d;", idx); return 0; }

int main(void)
{
	char *client[] = { "ovpn-event", "3", "client", NULL };
	char *server[] = { "ovpn-event", "2", "server", NULL };
	char *invalid[] = { "ovpn-event", "3", "unknown", NULL };

	/* These calls must resolve to six externally emitted firmware symbols. */
	start_ovpn_eas();
	stop_ovpn_eas();
	expect("eas:1;eas:0;");
	start_ovpn_client(3);
	expect("ipset-create:103;start-client:3;");
	stop_ovpn_client(3);
	expect("stop-client:3;ipset-destroy:103;");
	start_ovpn_server(2);
	stop_ovpn_server(2);
	expect("start-server:2;stop-server:2;");

	assert(ovpn_up_main(3, client) == 0);
	expect("client-up:3;dns:3;vpnc-resolv:103;resolv;sdn:103;");
	assert(ovpn_down_main(3, client) == 0);
	expect("sdn:103;clean:103;client-down:3;resolv;");
	assert(ovpn_up_main(3, server) == 0);
	assert(ovpn_down_main(3, server) == 0);
	expect("server-up:2;server-down:2;");
	assert(ovpn_up_main(2, client) == -1);
	assert(ovpn_down_main(2, client) == -1);
	assert(ovpn_up_main(3, invalid) == -1);
	assert(ovpn_down_main(3, invalid) == -1);
	expect("");
	assert(ovpn_route_up_main(0, NULL) == 0);
	assert(ovpn_route_pre_down_main(0, NULL) == 0);
	expect("route-up;route-pre-down;");
	puts("PASS actual ARM rc OpenVPN wrappers: linkage, dispatch, DNS/SDN ordering and invalid arguments");
	return 0;
}
