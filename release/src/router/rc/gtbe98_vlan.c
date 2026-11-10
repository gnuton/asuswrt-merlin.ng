/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "rc.h"
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <linux/if_vlan.h>
#include <linux/sockios.h>
#include <gtbe98_vlan_hw.h>

#define VLAN_OWNER "/var/run/gtbe98-wan-vlan"
#define VLAN_LOCK "/var/run/gtbe98-wan-vlan.lock"

struct vlan_context {
	struct gtbe98_rtl8372_snapshot hw;
	struct gtbe98_vlan_entry entries[GTBE98_VLAN_COUNT];
};

static int load_plan(struct gtbe98_vlan_plan *plan)
{
	struct gtbe98_vlan_request r;
	const char *extwan = nvram_safe_get("wans_extwan");
	memset(&r, 0, sizeof(r));
	r.dot1q = nvram_safe_get("wan0_dot1q");
	r.vid = nvram_safe_get("wan0_vid");
	r.dot1p = nvram_safe_get("wan0_dot1p");
	r.iptv_profile = nvram_safe_get("switch_wantag");
	r.stb_port = nvram_safe_get("switch_stb_x");
	r.gtbe98_rtl8372 = is_rtl8372_boardid();
	r.router_mode = is_router_mode() && !re_mode();
	r.manual_wan = nvram_match("autowan_enable", "0");
	r.single_wan = nvram_match("wans_dualwan", "wan none") && nvram_match("wans_standby", "0");
#ifdef RTCONFIG_USB_WAN_BACKUP
	r.single_wan = r.single_wan && nvram_match("wans_usb_bk", "0") && nvram_match("wans_usb_bk_act", "0");
#endif
	r.extwan = !strcmp(extwan, "0") ? 0 : !strcmp(extwan, "1") ? 1 : -1;
	r.wan_aggregation = !nvram_match("bond_wan", "0");
	/* Reject configured multi-LAN VLANs conservatively, including inactive
	 * definitions. Their ownership is not part of this first implementation.
	 */
	r.lan_vlan_state = *nvram_safe_get("vlan_rl") || !nvram_match("vlan_enable", "0");
	return gtbe98_wan_vlan_plan(&r, plan);
}

int gtbe98_vlan_requested(void)
{
	const char *enabled = nvram_safe_get("wan0_dot1q");
	return (*enabled && strcmp(enabled, "0")) || access(VLAN_OWNER, F_OK) == 0;
}

static int lock_vlan(void)
{
	int fd = open(VLAN_LOCK, O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (fd >= 0 && flock(fd, LOCK_EX)) {
		close(fd);
		fd = -1;
	}
	return fd;
}

/* Ownership survives service restarts, but never a reboot. Never remove an
 * existing interface merely because its name matches the requested VID.
 */
static int read_owner(struct gtbe98_vlan_plan *plan, unsigned int *index)
{
	FILE *fp;
	unsigned int extwan, vid, priority, version;
	char extra;
	int fields;
	fp = fopen(VLAN_OWNER, "r");
	if (!fp)
		return -1;
	fields = fscanf(fp, "%u %u %u %u %u %c", &version, &extwan, &vid, &priority, index, &extra);
	fclose(fp);
	if (fields != 5 || version != 1 || extwan > 1 || vid < 3 || vid > 4093 || priority > 7)
		return -1;
	memset(plan, 0, sizeof(*plan));
	plan->program_switch = extwan;
	plan->vid = vid;
	plan->priority = priority;
	snprintf(plan->parent, sizeof(plan->parent), "eth%u", extwan);
	snprintf(plan->ifname, sizeof(plan->ifname), "vlan%u", vid);
	return 0;
}

static int save_owner(const struct gtbe98_vlan_plan *plan, unsigned int index)
{
	int fd, len, ret;
	char buf[80];
	len = snprintf(buf, sizeof(buf), "1 %u %u %u %u\n", plan->program_switch,
		plan->vid, plan->priority, index);
	fd = open(VLAN_OWNER ".new", O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (fd < 0)
		return -1;
	ret = write(fd, buf, len) == len ? 0 : -1;
	if (close(fd))
		ret = -1;
	if (!ret)
		ret = rename(VLAN_OWNER ".new", VLAN_OWNER);
	if (ret)
		unlink(VLAN_OWNER ".new");
	return ret;
}

static int same_plan(const struct gtbe98_vlan_plan *a, const struct gtbe98_vlan_plan *b)
{
	return a->program_switch == b->program_switch && a->vid == b->vid && a->priority == b->priority;
}

static int switch_command(void *ctx, unsigned int cmd, unsigned int arg)
{
	char command[16], value[16];
	(void)ctx;
	snprintf(command, sizeof(command), "%u", cmd);
	snprintf(value, sizeof(value), "%u", arg);
	return eval("rtkswitch", command, value);
}

static int quiesce(void *ctx)
{
	struct vlan_context *c = ctx;
	unsigned int i;
	if (eval("rtkswitch", "11"))
		return -1;
	for (i = 0; i < 3; ++i) {
		if (gtbe98_vlan_hw_capture(&c->hw))
			return -1;
		if (gtbe98_vlan_hw_down(&c->hw))
			return 0;
		usleep(500000);
	}
	return -1;
}

static int switch_readback(void *ctx, struct gtbe98_vlan_observation *o)
{
	struct vlan_context *c = ctx;
	if (gtbe98_vlan_hw_capture(&c->hw) || !gtbe98_vlan_hw_down(&c->hw))
		return -1;
	return gtbe98_vlan_hw_observe(&c->hw, o, c->entries);
}

static int switch_matches(const struct gtbe98_vlan_plan *plan, struct vlan_context *c)
{
	struct gtbe98_vlan_observation o;
	return !gtbe98_vlan_hw_capture(&c->hw) &&
		!gtbe98_vlan_hw_observe(&c->hw, &o, c->entries) &&
		gtbe98_wan_vlan_verify(plan, &o) == GTBE98_VLAN_OK;
}

static int priority_matches(const struct gtbe98_vlan_plan *plan)
{
	char path[80], line[512], *p;
	unsigned int from, to, seen = 0;
	int used, found = 0;
	FILE *fp;
	snprintf(path, sizeof(path), "/proc/net/vlan/%s", plan->ifname);
	fp = fopen(path, "r");
	if (!fp)
		return 0;
	while (fgets(line, sizeof(line), fp)) {
		p = strstr(line, "EGRESS priority mappings: ");
		if (!p)
			continue;
		p += strlen("EGRESS priority mappings: ");
		found = 1;
		while (sscanf(p, "%u:%u%n", &from, &to, &used) == 2) {
			if (from > 7 || to != plan->priority || (seen & (1U << from))) {
				found = 0;
				break;
			}
			seen |= 1U << from;
			p += used;
		}
		while (*p == ' ' || *p == '\n' || *p == '\t')
			++p;
		if (*p)
			found = 0;
		break;
	}
	fclose(fp);
	/* An omitted mapping has Linux's default priority zero. */
	return found && (plan->priority == 0 || seen == 0xff);
}

static int linux_matches(const struct gtbe98_vlan_plan *plan, unsigned int index, int lan_started)
{
	struct vlan_ioctl_args args;
	struct ifreq ifr;
	char path[128], target[128];
	int fd, ret = 0;
	ssize_t len;
	if (!index || if_nametoindex(plan->ifname) != index)
		return 0;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return 0;
	memset(&args, 0, sizeof(args));
	strlcpy(args.device1, plan->ifname, sizeof(args.device1));
	args.cmd = GET_VLAN_REALDEV_NAME_CMD;
	if (ioctl(fd, SIOCGIFVLAN, &args) || strcmp(args.u.device2, plan->parent))
		goto out;
	args.cmd = GET_VLAN_VID_CMD;
	if (ioctl(fd, SIOCGIFVLAN, &args) || args.u.VID != (int)plan->vid)
		goto out;
	snprintf(path, sizeof(path), "/sys/class/net/%s/master", plan->ifname);
	if (readlink(path, target, sizeof(target)) >= 0 || errno != ENOENT)
		goto out;
	if (!plan->program_switch) {
		if (readlink("/sys/class/net/eth0/master", target, sizeof(target)) >= 0 || errno != ENOENT)
			goto out;
	}
	if (lan_started) {
		memset(&ifr, 0, sizeof(ifr));
		strlcpy(ifr.ifr_name, plan->parent, sizeof(ifr.ifr_name));
		if (ioctl(fd, SIOCGIFFLAGS, &ifr) || !(ifr.ifr_flags & IFF_UP))
			goto out;
		strlcpy(ifr.ifr_name, plan->ifname, sizeof(ifr.ifr_name));
		if (ioctl(fd, SIOCGIFFLAGS, &ifr) || !(ifr.ifr_flags & IFF_UP))
			goto out;
	}
	if (lan_started && plan->program_switch) {
		len = readlink("/sys/class/net/eth1/master", target, sizeof(target) - 1);
		if (len <= 0)
			goto out;
		target[len] = '\0';
		if (!strrchr(target, '/') || strcmp(strrchr(target, '/') + 1, nvram_safe_get("lan_ifname")))
			goto out;
	}
	ret = priority_matches(plan);
out:
	close(fd);
	return ret;
}

static int create_linux(const struct gtbe98_vlan_plan *plan, unsigned int *index)
{
	char vid[8], priority[8], from[8];
	unsigned int i;
	if (if_nametoindex(plan->ifname))
		return -1;
	if (ifconfig(plan->parent, IFUP | IFF_ALLMULTI, NULL, NULL))
		return -1;
	snprintf(vid, sizeof(vid), "%u", plan->vid);
	snprintf(priority, sizeof(priority), "%u", plan->priority);
	if (eval("vconfig", "set_name_type", "VLAN_PLUS_VID_NO_PAD") ||
	    eval("vconfig", "add", plan->parent, vid))
		return -1;
	*index = if_nametoindex(plan->ifname);
	/* Record ownership immediately, including incomplete setup for recovery. */
	if (!*index || save_owner(plan, *index))
		return -1;
	for (i = 0; i < 8; ++i) {
		snprintf(from, sizeof(from), "%u", i);
		if (eval("vconfig", "set_egress_map", plan->ifname, from, priority))
			return -1;
	}
	if (ifconfig(plan->ifname, IFUP | IFF_ALLMULTI, NULL, NULL))
		return -1;
	return linux_matches(plan, *index, 0) ? 0 : -1;
}

static void failed(const char *why)
{
	struct gtbe98_vlan_plan owned;
	unsigned int index;
	int have_owner = !read_owner(&owned, &index);
	nvram_set("gtbe98_wan_vlan_ready", "0");
	if (have_owner && index && if_nametoindex(owned.ifname) == index)
		ifconfig(owned.ifname, 0, NULL, NULL);
	/* Keep the last owned external WAN blocked even after pending port changes. */
	if ((have_owner && owned.program_switch) ||
	    nvram_match("wans_extwan", "1"))
		eval("rtkswitch", "11");
	logmessage("WAN VLAN", "%s; WAN stopped, reboot required after correction", why);
}

void gtbe98_vlan_driver_init(void)
{
	FILE *fp;
	unsigned int major_num, minor_num;
	struct vlan_context *c;
	nvram_set("gtbe98_wan_vlan_cap", "0");
	nvram_set("gtbe98_wan_vlan_ready", "0");
	if (access(VLAN_OWNER, F_OK)) {
		nvram_set("gtbe98_wan_vlan_owned", "0");
		nvram_set("gtbe98_wan_vlan_extwan", "0");
	}
	if (!is_rtl8372_boardid())
		return;
	/* Repeated switch initialization must not fail merely because the
	 * companion is already loaded. Its ABI is still checked on capture.
	 */
	if (gtbe98_vlan_hw_identity() &&
	    access("/sys/class/misc/gtbe98_rtl8372/dev", F_OK) &&
	    eval("insmod", "gtbe98_rtl8372_readback"))
		return;
	/* Firmware /dev is not necessarily managed by devtmpfs. */
	if (access(GTBE98_RTL8372_DEVICE, F_OK)) {
		fp = fopen("/sys/class/misc/gtbe98_rtl8372/dev", "r");
		if (!fp)
			return;
		if (fscanf(fp, "%u:%u", &major_num, &minor_num) == 2 && major_num == 10 && minor_num < 256)
			mknod(GTBE98_RTL8372_DEVICE, S_IFCHR | 0600, makedev(major_num, minor_num));
		fclose(fp);
	}
	c = calloc(1, sizeof(*c));
	if (!c)
		return;
	if (!gtbe98_vlan_hw_capture(&c->hw))
		nvram_set("gtbe98_wan_vlan_cap", "1");
	free(c);
}

int gtbe98_vlan_reconfig_ifnames(void)
{
	struct gtbe98_vlan_plan plan, owned;
	unsigned int index;
	char lan[128];
	if (!gtbe98_vlan_requested())
		return 0;
	/* A pending reboot must not move the currently owned physical WAN into
	 * br0. Leave existing LAN/LACP membership untouched during this boot.
	 */
	if (!access(VLAN_OWNER, F_OK)) {
		if (!read_owner(&owned, &index)) {
			set_wan_phy(owned.ifname);
			nvram_set("wan0_ifname", owned.ifname);
		}
		return 1;
	}
	/* Keep eth1 in LAN: it is the shared CPU uplink, never the WAN netdev.
	 * Existing LACP processing subsequently owns eth2/eth3 and bond0.
	 */
	strlcpy(lan, "eth1 eth2 eth3 wl0 wl1 wl2 wl3", sizeof(lan));
	if (load_plan(&plan) == GTBE98_VLAN_OK && plan.program_switch)
		strlcpy(lan, "eth0 eth1 eth2 eth3 wl0 wl1 wl2 wl3", sizeof(lan));
	nvram_set("lan_ifnames", lan);
	if (load_plan(&plan) == GTBE98_VLAN_OK) {
		/* init_switch_pre() still needs the physical WAN identity for HND
		 * role selection. Use the existing external-WAN sentinel for eth1,
		 * whose whole CPU uplink must remain LAN. Configure publishes the
		 * real VLAN name after this initialization, before WAN startup.
		 */
		nvram_set("wan_ifname", plan.program_switch ? "vlan4094" : "eth0");
		set_wan_phy(plan.ifname);
		nvram_set("wan0_ifname", plan.ifname);
		nvram_set("wan0_gw_ifname", plan.ifname);
	} else {
		/* An invalid request never falls back to an untagged physical WAN. */
		nvram_set("wan_ifname", "");
		set_wan_phy("");
		nvram_set("wan0_ifname", "");
		nvram_set("wan0_gw_ifname", "");
	}
	return 1;
}

int gtbe98_vlan_configure(void)
{
	const struct gtbe98_vlan_ops ops = { quiesce, switch_command, switch_readback };
	struct gtbe98_vlan_plan plan, owned;
	struct vlan_context *c = NULL;
	unsigned int index = 0;
	int fd, ret = -1, result;
	fd = lock_vlan();
	if (fd < 0)
		return -1;
	nvram_set("gtbe98_wan_vlan_ready", "0");
	result = load_plan(&plan);
	if (result != GTBE98_VLAN_OK) {
		failed(gtbe98_wan_vlan_error(result));
		goto out;
	}
	if (!nvram_match("gtbe98_wan_vlan_cap", "1")) {
		failed("Checked switch readback unavailable");
		goto out;
	}
	c = calloc(1, sizeof(*c));
	if (!c)
		goto fail;
	if (!access(VLAN_OWNER, F_OK)) {
		if (read_owner(&owned, &index) || !same_plan(&plan, &owned) ||
		    !linux_matches(&plan, index, 0))
			goto fail;
		if (plan.program_switch && (!switch_matches(&plan, c) || quiesce(c)))
			goto fail;
	} else {
		if (if_nametoindex(plan.ifname) || save_owner(&plan, 0))
			goto fail;
		nvram_set("gtbe98_wan_vlan_owned", "1");
		nvram_set_int("gtbe98_wan_vlan_extwan", plan.program_switch);
		if (plan.program_switch) {
			result = gtbe98_wan_vlan_stage(&plan, &ops, c);
			if (result != GTBE98_VLAN_OK) {
				failed(gtbe98_wan_vlan_error(result));
				goto out;
			}
		}
		if (create_linux(&plan, &index)) {
			failed("Linux VLAN setup or verification failed");
			goto out;
		}
	}
	/* init_switch() has already run config_lacp() during boot. Publish only
	 * WAN fields here; rebuilding lan_ifnames would drop its bond0 mapping.
	 */
	nvram_set("wan_ifname", plan.ifname);
	set_wan_phy(plan.ifname);
	nvram_set("wan0_ifname", plan.ifname);
	nvram_set("wan0_gw_ifname", plan.ifname);
	/* Physical WAN remains down until start_lan() has attached eth1 to br0. */
	ret = 0;
	goto out;
fail:
	failed("Configuration, ownership or readback check failed");
out:
	free(c);
	close(fd);
	return ret;
}

int gtbe98_vlan_wan_start(void)
{
	struct gtbe98_vlan_plan plan, owned;
	struct vlan_context *c = NULL;
	unsigned int index;
	int fd, ret = -1;
	fd = lock_vlan();
	if (fd < 0)
		return -1;
	if (load_plan(&plan) != GTBE98_VLAN_OK || read_owner(&owned, &index) ||
	    !same_plan(&plan, &owned) || !linux_matches(&plan, index, 0))
		goto fail;
	if (plan.program_switch) {
		c = calloc(1, sizeof(*c));
		if (!c || !switch_matches(&plan, c))
			goto fail;
	}
	if (ifconfig(plan.parent, IFUP | IFF_ALLMULTI, NULL, NULL) ||
	    ifconfig(plan.ifname, IFUP | IFF_ALLMULTI, NULL, NULL) ||
	    !linux_matches(&plan, index, 1))
		goto fail;
	if (plan.program_switch) {
		/* Do not cycle a working PHY on PPP/DHCP reconnect. */
		if (c->hw.wan_phy_a610 != 0x2058 || c->hw.wan_phy_a610_after != 0x2058) {
			if (!gtbe98_vlan_hw_down(&c->hw) || eval("rtkswitch", "10") ||
			    !switch_matches(&plan, c) || c->hw.wan_phy_a610 != 0x2058 ||
			    c->hw.wan_phy_a610_after != 0x2058)
				goto fail;
		}
	}
	nvram_set("gtbe98_wan_vlan_ready", "1");
	ret = 0;
	goto out;
fail:
	failed("WAN start verification failed");
out:
	free(c);
	close(fd);
	return ret;
}

void gtbe98_vlan_reset(void)
{
	struct gtbe98_vlan_plan owned;
	unsigned int index;
	int fd = lock_vlan();
	if (fd < 0)
		return;
	nvram_set("gtbe98_wan_vlan_ready", "0");
	if ((!read_owner(&owned, &index) && owned.program_switch) || nvram_match("wans_extwan", "1"))
		eval("rtkswitch", "11");
	/* Retain isolated switch state and the owned netdev for unchanged LAN
	 * service restarts. Parameter/port changes require reboot, enforced by
	 * both HTTP preflight and the owner comparison at configure/start.
	 */
	close(fd);
}
