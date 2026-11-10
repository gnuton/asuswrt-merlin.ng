/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Execute the actual rc backend with local files and simulated devices. */
#define _GNU_SOURCE
#define __RC_H__
#define RTCONFIG_USB_WAN_BACKUP
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/if_vlan.h>
#include <linux/sockios.h>
#include "gtbe98_vlan_hw.h"

#define IFUP (IFF_UP | IFF_RUNNING)
static char work[128];
static struct { char key[64], value[256]; } vars[64];
static unsigned int vars_count, ifindex, vlan_vid, maps[8], selected;
static unsigned int up_commands, all_commands, vlan_creates, programming;
static int parent_up, vlan_up, parent_in_lan, vlan_in_lan, wrong_parent;
static int fail_capture, ignore_vlan_delete, fail_vlan_map;
static char vlan_name[16], vlan_parent[8];
static struct gtbe98_rtl8372_snapshot hardware;

static char *nvram_safe_get(const char *key)
{
	unsigned int i;
	for (i = 0; i < vars_count; ++i)
		if (!strcmp(vars[i].key, key)) return vars[i].value;
	return "";
}
static int nvram_match(const char *key, const char *value) { return !strcmp(nvram_safe_get(key), value); }
static void nvram_set(const char *key, const char *value)
{
	unsigned int i;
	for (i = 0; i < vars_count; ++i)
		if (!strcmp(vars[i].key, key)) break;
	if (i == vars_count) { assert(vars_count < 64); vars_count++; }
	snprintf(vars[i].key, sizeof(vars[i].key), "%s", key);
	snprintf(vars[i].value, sizeof(vars[i].value), "%s", value);
}
static void nvram_set_int(const char *key, int value)
{
	char number[16]; snprintf(number, sizeof(number), "%d", value); nvram_set(key, number);
}
static int is_rtl8372_boardid(void) { return 1; }
static int is_router_mode(void) { return 1; }
static int re_mode(void) { return 0; }
static void set_wan_phy(char *name) { nvram_set("wan_ifnames", name); }
static void logmessage(const char *tag, const char *fmt, ...)
{
	va_list ap;
	if (!getenv("GTBE98_TEST_DEBUG")) return;
	fprintf(stderr, "%s: ", tag); va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap); fputc('\n', stderr);
}
static size_t test_strlcpy(char *dst, const char *src, size_t size)
{
	size_t len = strlen(src); if (size) snprintf(dst, size, "%s", src); return len;
}
static const char *local_path(const char *path, char *buf)
{
	assert(!strncmp(path, "/var/run/gtbe98-wan-vlan", 24));
	snprintf(buf, 256, "%s/%s", work, strrchr(path, '/') + 1); return buf;
}
static int test_open(const char *path, int flags, ...)
{
	char buf[256]; return open(local_path(path, buf), flags, 0600);
}
static int test_access(const char *path, int mode)
{
	char buf[256];
	if (!strcmp(path, GTBE98_RTL8372_DEVICE)) return 0;
	if (!strcmp(path, "/sys/class/misc/gtbe98_rtl8372/dev")) return 0;
	return access(local_path(path, buf), mode);
}
static int test_unlink(const char *path)
{
	char buf[256]; return unlink(local_path(path, buf));
}
static int test_rename(const char *from, const char *to)
{
	char a[256], b[256]; return rename(local_path(from, a), local_path(to, b));
}
static FILE *test_fopen(const char *path, const char *mode)
{
	char buf[256]; FILE *fp; unsigned int i;
	if (!strncmp(path, "/proc/net/vlan/", 15)) {
		fp = tmpfile(); assert(fp);
		fputs(" EGRESS priority mappings: ", fp);
		for (i = 0; i < 8; ++i) fprintf(fp, "%u:%u ", i, maps[i]);
		fputs("\n", fp); rewind(fp); return fp;
	}
	return fopen(local_path(path, buf), mode);
}
static ssize_t test_readlink(const char *path, char *buf, size_t size)
{
	const char *target = "../../net/br0";
	int parent = strstr(path, "/eth1/") || strstr(path, "/eth0/");
	if (!(parent ? parent_in_lan : vlan_in_lan)) { errno = ENOENT; return -1; }
	assert(strlen(target) < size); memcpy(buf, target, strlen(target)); return strlen(target);
}
static unsigned int test_if_nametoindex(const char *name)
{
	return ifindex && !strcmp(name, vlan_name) ? ifindex : 0;
}
static int test_socket(int domain, int type, int protocol)
{
	assert(domain == AF_INET && type == SOCK_DGRAM && protocol == 0);
	/* The ioctl transport is simulated; no network capability is needed. */
	return open("/dev/null", O_RDONLY);
}
static int test_ioctl(int fd, unsigned long cmd, void *arg)
{
	struct vlan_ioctl_args *a = arg; struct ifreq *ifr = arg; (void)fd;
	if (cmd == SIOCGIFFLAGS) {
		ifr->ifr_flags = (!strcmp(ifr->ifr_name, vlan_name) ? vlan_up : parent_up) ? IFF_UP : 0;
		return 0;
	}
	assert(cmd == SIOCGIFVLAN && ifindex && !strcmp(a->device1, vlan_name));
	if (a->cmd == GET_VLAN_REALDEV_NAME_CMD) strcpy(a->u.device2, wrong_parent ? "eth9" : vlan_parent);
	else { assert(a->cmd == GET_VLAN_VID_CMD); a->u.VID = vlan_vid; }
	return 0;
}
static int ifconfig(const char *name, int flags, const char *ip, const char *mask)
{
	(void)ip; (void)mask;
	if (!strcmp(name, "eth0") || !strcmp(name, "eth1")) { parent_up = !!(flags & IFF_UP); return 0; }
	assert(ifindex && !strcmp(name, vlan_name));
	if (!parent_up) return -1;
	vlan_up = !!(flags & IFF_UP); return 0;
}
static int test_eval(const char **args)
{
	unsigned int cmd, arg, i, members;
	if (!strcmp(args[0], "insmod")) return 0;
	if (!strcmp(args[0], "vconfig")) {
		if (!strcmp(args[1], "set_name_type")) return 0;
		if (!strcmp(args[1], "add")) {
			assert(!ifindex); ifindex = 100; vlan_vid = atoi(args[3]);
			snprintf(vlan_name, sizeof(vlan_name), "vlan%u", vlan_vid);
			strcpy(vlan_parent, args[2]); vlan_creates++; return 0;
		}
		assert(!strcmp(args[1], "set_egress_map"));
		if (fail_vlan_map) return -1;
		maps[atoi(args[3])] = atoi(args[4]); return 0;
	}
	assert(!strcmp(args[0], "rtkswitch"));
	cmd = atoi(args[1]); arg = args[2] ? strtoul(args[2], NULL, 10) : 0;
	all_commands++;
	if (cmd == 11) { hardware.wan_phy_a610 = hardware.wan_phy_a610_after = 0x2858; hardware.wan_link = hardware.wan_link_after = 0; return 0; }
	if (cmd == 10) { hardware.wan_phy_a610 = hardware.wan_phy_a610_after = 0x2058; up_commands++; return 0; }
	assert(hardware.wan_phy_a610 == 0x2858);
	programming++;
	switch (cmd) {
	case 38:
		memset(hardware.vlans, 0, sizeof(hardware.vlans));
		hardware.vlans[1].members = hardware.vlans[1].untag = 0xf8;
		hardware.egress_filter = 1;
		for (i = 0; i < 8; ++i) hardware.ports[i].pvid = 1;
		break;
	case 36: selected = arg; break;
	case 37: break;
	case 3901: if (!ignore_vlan_delete) memset(&hardware.vlans[selected], 0, sizeof(hardware.vlans[0])); break;
	case 39:
		members = (arg & 0x3f7) | 8;
		hardware.vlans[selected].members = members;
		hardware.vlans[selected].untag = (arg >> 16) | (selected ? 0 : 8);
		hardware.vlans[selected].fid = selected ? 1 : 0;
		for (i = 0; i < 8; ++i) if (members & (1U << i)) hardware.ports[i].pvid = selected;
		break;
	case 380: for (i = 0; i < 8; ++i) if (arg & (1U << i)) hardware.ports[i].ingress_filter = 1; break;
	case 3951: case 3952: case 3953:
		for (i = 0; i < 8; ++i) if (arg & (1U << i)) hardware.ports[i].admission = cmd - 3951;
		break;
	default: assert(0);
	}
	return 0;
}
int __wrap_gtbe98_vlan_hw_capture(struct gtbe98_rtl8372_snapshot *s)
{
	*s = hardware; if (fail_capture) { s->complete = 0; return -1; } return 0;
}
int __wrap_gtbe98_vlan_hw_identity(void) { return 0; }
#define open test_open
#define access test_access
#define fopen test_fopen
#define unlink test_unlink
#define rename test_rename
#define readlink test_readlink
#define ioctl test_ioctl
#define socket test_socket
#define if_nametoindex test_if_nametoindex
#define strlcpy test_strlcpy
#define eval(...) test_eval((const char *[]){__VA_ARGS__, NULL})
#include "../release/src/router/rc/gtbe98_vlan.c"
#undef open
#undef access
#undef fopen
#undef unlink
#undef rename
#undef readlink
#undef ioctl
#undef if_nametoindex

static void reset(int extwan)
{
	char path[256];
	unlink(local_path(VLAN_OWNER, path)); unlink(local_path(VLAN_OWNER ".new", path));
	memset(vars, 0, sizeof(vars)); vars_count = 0;
	ifindex = up_commands = all_commands = vlan_creates = programming = selected = 0;
	parent_up = vlan_up = vlan_in_lan = wrong_parent = fail_capture = ignore_vlan_delete = fail_vlan_map = 0;
	parent_in_lan = extwan;
	memset(maps, 0, sizeof(maps)); memset(&hardware, 0, sizeof(hardware));
	hardware.abi_version = GTBE98_RTL8372_ABI_VERSION; hardware.size = sizeof(hardware);
	hardware.complete = 1; hardware.port_mask = 0xf8; hardware.vlan_count = 4096;
	hardware.wan_phy_a610 = hardware.wan_phy_a610_after = 0x2058;
	nvram_set("wan0_dot1q", "1"); nvram_set("wan0_vid", "835"); nvram_set("wan0_dot1p", "3");
	nvram_set("switch_wantag", "none"); nvram_set("switch_stb_x", "0");
	nvram_set("autowan_enable", "0"); nvram_set("wans_dualwan", "wan none");
	nvram_set("wans_standby", "0"); nvram_set_int("wans_extwan", extwan);
	nvram_set("wans_usb_bk", "0"); nvram_set("wans_usb_bk_act", "0");
	nvram_set("bond_wan", "0"); nvram_set("vlan_enable", "0"); nvram_set("lan_ifname", "br0");
	nvram_set("gtbe98_wan_vlan_cap", "1");
}
int main(void)
{
	char directory[] = "/tmp/gtbe98-runtime.XXXXXX", path[256]; unsigned int commands;
	unsigned int port, lacp, vid, priority;
	const char *vids[] = { "3", "100", "835", "4093" };
	const char *priorities[] = { "0", "7" };
	char lan_before[128];
	assert(mkdtemp(directory)); strcpy(work, directory);
	reset(1); nvram_set("gtbe98_wan_vlan_owned", "1");
	gtbe98_vlan_driver_init();
	assert(nvram_match("gtbe98_wan_vlan_cap", "1") && nvram_match("gtbe98_wan_vlan_owned", "0"));
	gtbe98_vlan_driver_init(); assert(nvram_match("gtbe98_wan_vlan_cap", "1"));
	fail_capture = 1; gtbe98_vlan_driver_init(); assert(nvram_match("gtbe98_wan_vlan_cap", "0"));
	reset(1); assert(gtbe98_vlan_reconfig_ifnames() == 1);
	assert(nvram_match("wan_ifname", "vlan4094")); assert(nvram_match("wan0_ifname", "vlan835"));
	assert(!strstr(nvram_safe_get("lan_ifnames"), "vlan4094"));
	nvram_set("lan_ifnames", "eth0 eth1 bond0 wl0 wl1 wl2 wl3");
	assert(gtbe98_vlan_configure() == 0 && !up_commands);
	assert(strstr(nvram_safe_get("lan_ifnames"), "bond0"));
	assert(gtbe98_vlan_wan_start() == 0 && up_commands == 1);
	commands = programming;
	assert(gtbe98_vlan_wan_start() == 0 && up_commands == 1 && programming == commands);
	gtbe98_vlan_reset(); parent_up = vlan_up = 0; parent_in_lan = 0;
	assert(gtbe98_vlan_configure() == 0 && programming == commands);
	parent_in_lan = 1;
	assert(gtbe98_vlan_wan_start() == 0 && up_commands == 2 && programming == commands);
	nvram_set("wan0_vid", "836"); nvram_set("wans_extwan", "0");
	assert(gtbe98_vlan_reconfig_ifnames() == 1);
	assert(strstr(nvram_safe_get("lan_ifnames"), "bond0"));
	assert(gtbe98_vlan_configure() == -1 && vlan_creates == 1 && programming == commands);
	assert(hardware.wan_phy_a610 == 0x2858 && nvram_match("gtbe98_wan_vlan_ready", "0"));
	reset(1); ignore_vlan_delete = 1;
	assert(gtbe98_vlan_configure() == -1 && !vlan_creates && !up_commands);
	reset(1); fail_capture = 1;
	assert(gtbe98_vlan_configure() == -1 && !programming && !up_commands);
	reset(1); fail_vlan_map = 1;
	assert(gtbe98_vlan_configure() == -1 && vlan_creates == 1 && !up_commands);
	assert(gtbe98_vlan_wan_start() == -1 && !up_commands);
	reset(1); ifindex = 42; strcpy(vlan_name, "vlan835");
	assert(gtbe98_vlan_configure() == -1 && ifindex == 42 && !vlan_creates && !programming);
	reset(1); assert(!gtbe98_vlan_configure()); vlan_in_lan = 1;
	assert(gtbe98_vlan_wan_start() == -1 && !up_commands);
	reset(1); assert(!gtbe98_vlan_configure()); wrong_parent = 1;
	assert(gtbe98_vlan_wan_start() == -1 && !up_commands);
	reset(0); assert(gtbe98_vlan_reconfig_ifnames() == 1);
	assert(nvram_match("wan_ifname", "eth0"));
	assert(!strstr(nvram_safe_get("lan_ifnames"), "eth0"));
	assert(!gtbe98_vlan_configure() && !gtbe98_vlan_wan_start());
	assert(!all_commands && parent_up && vlan_up);
	nvram_set("wan0_vid", "836"); assert(gtbe98_vlan_wan_start() == -1 && !vlan_up);
	reset(1); nvram_set("switch_wantag", "manual");
	assert(gtbe98_vlan_configure() == -1 && !programming && !vlan_creates);
	reset(1); nvram_set("wans_usb_bk", "1");
	assert(gtbe98_vlan_configure() == -1 && !programming && !vlan_creates);
	reset(1); nvram_set("wan0_dot1q", "0"); assert(!gtbe98_vlan_requested());
	/* No dependence on the reference ISP or on LACP. LAN membership supplied
	 * by the existing LACP lifecycle is retained in both configurations. */
	for (port = 0; port < 2; ++port)
	for (lacp = 0; lacp < 2; ++lacp)
	for (vid = 0; vid < sizeof(vids)/sizeof(vids[0]); ++vid)
	for (priority = 0; priority < sizeof(priorities)/sizeof(priorities[0]); ++priority) {
		reset(port); nvram_set("wan0_vid", vids[vid]); nvram_set("wan0_dot1p", priorities[priority]);
		nvram_set_int("lacp_enabled", lacp);
		assert(gtbe98_vlan_reconfig_ifnames() == 1);
		if (lacp)
			nvram_set("lan_ifnames", port ? "eth0 eth1 bond0 wl0 wl1 wl2 wl3" : "eth1 bond0 wl0 wl1 wl2 wl3");
		strcpy(lan_before, nvram_safe_get("lan_ifnames"));
		assert(!gtbe98_vlan_configure() && !gtbe98_vlan_wan_start());
		assert(nvram_match("lan_ifnames", lan_before));
		assert(nvram_match("gtbe98_wan_vlan_ready", "1"));
		assert(!!strstr(lan_before, "bond0") == lacp);
		assert(vlan_vid == (unsigned int)atoi(vids[vid]) && maps[0] == (unsigned int)atoi(priorities[priority]));
	}
	/* A flash with absent new keys, or a reboot after disable, must not take
	 * ownership of an existing eth0/IPTV WAN or rewrite its LAN/NVRAM. */
	reset(0); nvram_set("wan0_dot1q", ""); nvram_set("switch_wantag", "manual");
	nvram_set("switch_wan0tagid", "835"); nvram_set("wan0_ifname", "vlan835");
	nvram_set("wan_ifname", "eth0"); nvram_set("lan_ifnames", "eth1 eth2 eth3 wl0 wl1 wl2 wl3");
	nvram_set("wans_usb_bk", "1"); nvram_set("vlan_rl", "<1>10");
	fail_capture = 1; gtbe98_vlan_driver_init();
	assert(!gtbe98_vlan_requested() && !gtbe98_vlan_reconfig_ifnames());
	nvram_set("wan0_dot1q", "0");
	assert(!gtbe98_vlan_requested() && !gtbe98_vlan_reconfig_ifnames());
	assert(nvram_match("switch_wantag", "manual") && nvram_match("switch_wan0tagid", "835"));
	assert(nvram_match("wan_ifname", "eth0") && nvram_match("wan0_ifname", "vlan835"));
	assert(nvram_match("lan_ifnames", "eth1 eth2 eth3 wl0 wl1 wl2 wl3"));
	assert(!all_commands && !vlan_creates);
	unlink(local_path(VLAN_LOCK, path)); assert(!rmdir(work));
	puts("GT-BE98 runtime boot/reconnect/restart, ownership, LACP and failure tests passed");
	return 0;
}
