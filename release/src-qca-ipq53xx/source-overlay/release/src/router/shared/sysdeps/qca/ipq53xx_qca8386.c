/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <bcmnvram.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/if_ether.h>	//have in front of <linux/mii.h> to avoid redefinition of 'struct ethhdr'
#include <linux/mii.h>
#include <dirent.h>

#include <shutils.h>
#include <shared.h>
#include <qca.h>

#if defined(RTCONFIG_PORT2_DEVICE)
#define NR_WANLAN_PORT	2
#else
#define NR_WANLAN_PORT	5
#endif
#define DBGOUT		NULL			/* "/dev/console" */
#define QCA8386_IFACE	"eth1"
#define QCA8386_CPUPORT	"0"

#if defined(RTCONFIG_BONDING_WAN) || defined(RTCONFIG_LACP)
#define BVID_OFFSET	901
#endif

enum {
	VLAN_TYPE_STB_VOIP = 0,
	VLAN_TYPE_WAN,
	VLAN_TYPE_WAN_NO_VLAN,	/* Used to bridge WAN/STB for Hinet MOD. */
	VLAN_TYPE_LAN_NO_VLAN,	/* LAN as WAN */

	VLAN_TYPE_MAX
};

enum {
	LAN1_PORT=0,
#if !defined(RTCONFIG_PORT2_DEVICE)
	LAN2_PORT,
	LAN3_PORT,
	LAN4_PORT,
#endif
	WAN_PORT,
	MAX_WANLAN_PORT
};

static const char *upstream_iptv_ifaces[16] = {
	[WANS_DUALWAN_IF_WAN] = "eth0",
};

/* 0:WAN, 1:LAN first index is switch_stb_x nvram variable.
 * lan_wan_partition[switch_stb_x][0] is virtual port0,
 * lan_wan_partition[switch_stb_x][1] is virtual port1, etc.
 */
static const int lan_wan_partition[9][NR_WANLAN_PORT] = {
#if defined(TUFBE6500) || defined(TUFBE9400) || defined(RTBE50) || defined(RT4GBE58)
	/* L1, L2, L3, X, W1G */
	{1,1,1,1,0}, // Normal
	{0,1,1,1,0}, // IPTV STB port = LAN1
	{1,0,1,1,0}, // IPTV STB port = LAN2
	{1,0,1,1,0}, // IPTV STB port = LAN2 (unused)
	{1,1,0,1,0}, // IPTV STB port = LAN3
	{0,0,1,1,0}, // IPTV STB port = LAN1 & LAN2
	{1,0,0,1,0}, // IPTV STB port = LAN2 & LAN3
	{1,1,1,1,1}  // ALL
#elif defined(RTCONFIG_PORT2_DEVICE)
	/* L1, WAN */
	{1,0}, // Normal
	{1,0}, // no use
	{1,0}, // no use
	{1,0}, // no use
	{1,0}, // no use
	{1,0}, // no use
	{1,0}, // no use
	{1,1}  // ALL
#else /* QCA MI01.2 reference board */
#warning FIXME
	/* L1, L2, L3, L4, W1G */
	{1,1,1,1,0}, // Normal
	{0,1,1,1,0}, // IPTV STB port = LAN1
	{1,0,1,1,0}, // IPTV STB port = LAN2
	{1,1,0,1,0}, // IPTV STB port = LAN3
	{1,1,1,0,0}, // IPTV STB port = LAN4
	{0,0,1,1,0}, // IPTV STB port = LAN1 & LAN2
	{1,1,0,0,0}, // IPTV STB port = LAN3 & LAN4
	{1,1,1,1,1}  // ALL
#endif
};

#if defined(RTCONFIG_BONDING_WAN) || defined(RTCONFIG_LACP)
/* array index:		port number used in wanports_bond, enum bs_port_id.
 * 			0: WAN, 1~4: LAN1~4
 * array element:	virtual port
 * 			e.g. LAN1_PORT ~ LAN6_PORT, WAN_PORT, etc.
 */
static const int bsport_to_vport[MAX_WANLAN_PORT] = {
	WAN_PORT, LAN1_PORT, LAN2_PORT, LAN3_PORT, LAN4_PORT
};
#endif

/* array index:		virtual port mapping enumeration.
 * 			e.g. LAN1_PORT, LAN2_PORT, etc.
 * array element:	PHY address, negative value means absent PHY.
 *
 * QCA MI01.2, IPQ5322+QCA8386
 * arch/arm64/boot/dts/qcom/ipq5332-mi01.2.dts
 */
static const int vport_to_phy_addr[MAX_WANLAN_PORT] = {
#if defined(TUFBE6500) || defined(TUFBE9400)
	1, 1, 1, -1, 2
#elif defined(RTCONFIG_PORT2_DEVICE)
	1, 2
#elif defined(BD4_OD)
	2, 1
#elif defined(RTBE50) || defined(RT4GBE58)
	1, 1, 1, -1, 2
#else /* QCA MI01.2 reference board */
#warning FIXME
	1, 1, 1, 1, 2
#endif
};

/**
 * The vport_to_iface array is used to get interface name of each virtual
 * port.  If bled need to know TX/RX statistics of LAN1~2, WAN1, WAN2 (AQR107),
 * and 10G SFP+, bled has to find this information from netdev.  So, define
 * this array and implement vport_to_iface_name() function which is used by
 * bled in update_swports_bled().
 *
 * array index:		virtual port mapping enumeration.
 * 			e.g. LAN1_PORT, LAN2_PORT, etc.
 * array element:	Interface name of specific virtual port.
 */
static const char *vport_to_iface[MAX_WANLAN_PORT] = {
#if defined(RTCONFIG_PORT2_DEVICE)
	"eth1", /* LAN1 */
#else
	"eth1", "eth1", "eth1", "eth1",	/* LAN1~4 */
#endif
	"eth0" 				/* WAN */
};

/**
 * The vport_to_ssdkport array is used to convert between virtual port mask
 * and real (QCA8386) port mask.  Because QCA MI01.2 use virtual port mask to
 * generate lanports_mask/wanports_mask and bled need to know TX/RX statistice
 * of some ports which are connected to QCA8386 due to LAN1~4 are connected to
 * QCA8386 directly. So, define this array and implement vportmask_to_rportmask()
 * and rportmask_to_vportmask() functions which are used by bled in
 * update_swports_bled().
 *
 * array index:		virtual port mapping enumeration.
 * 			e.g. LAN1_PORT, LAN2_PORT, etc.
 * array element:	Port mask of QCA8386
 */
static const int vport_to_ssdkport[MAX_WANLAN_PORT] = {
#if defined(TUFBE6500) || defined(TUFBE9400)
	3, 2, 1, -1,		/* LAN1~3, QCA8386 (sw1) */
#elif defined(RTCONFIG_PORT2_DEVICE)
	0x10 + 1,		/* LAN1 */
#elif defined(BD4_OD)
	0x10 + 2,		/* LAN1 */
#elif defined(RTBE50)
	1, 2, 3, -1,		/* LAN1~3, QCA8384 (sw1) */
#elif defined(RT4GBE58)
	3, 2, 1, -1,		/* LAN1~3, QCA8384 (sw1) */
#else /* QCA MI01.2 reference board */
#warning FIXME
	1, 2, 3, 4,		/* LAN1~4, QCA8386 (sw1) */
#endif
#if defined(BD4_OD)
	0x10 + 1		/* WAN, IPQ53xx (sw0) */
#else
	0x10 + 2		/* WAN, IPQ53xx (sw0) */
#endif
};

#if defined(RTCONFIG_PORT2_DEVICE)
static const unsigned int qca8386_vportmask = (1U << LAN1_PORT);
#else
static const unsigned int qca8386_vportmask = (1U << LAN1_PORT) | (1U << LAN2_PORT) | (1U << LAN3_PORT) | (1U << LAN4_PORT);
#endif

/* array index:		switch_stb_x nvram variable.
 * array element:	platform specific VoIP/STB virtual port bitmask.
 */
static const unsigned int stb_to_mask[7] = { 0,
#if defined(TUFBE6500) || defined(TUFBE9400) || defined(RTBE50) || defined(RT4GBE58)
	(1U << LAN1_PORT),
	(1U << LAN2_PORT),
	(1U << LAN2_PORT), /* unused */
	(1U << LAN3_PORT),
	(1U << LAN1_PORT) | (1U << LAN2_PORT),
	(1U << LAN2_PORT) | (1U << LAN3_PORT)
#elif defined(RTCONFIG_PORT2_DEVICE)
	(1U << LAN1_PORT),
#else /* QCA MI01.2 reference board */
#warning FIXME
	(1U << LAN1_PORT),
	(1U << LAN2_PORT),
	(1U << LAN3_PORT),
	(1U << LAN4_PORT),
	(1U << LAN1_PORT) | (1U << LAN2_PORT),
	(1U << LAN3_PORT) | (1U << LAN4_PORT)
#endif
};

void reset_qca_switch(void);

/* ALL WAN/LAN virtual port bit-mask */
#if defined(RTCONFIG_PORT2_DEVICE)
static unsigned int wanlanports_mask = (1U << WAN_PORT) | (1U << LAN1_PORT);
#else
static unsigned int wanlanports_mask = (1U << WAN_PORT) | (1U << LAN1_PORT) | (1U << LAN2_PORT) | (1U << LAN3_PORT) | (1U << LAN4_PORT);
#endif

/* Final model-specific LAN/WAN/WANS_LAN partition definitions.
 * Because LAN/WAN ports of QCA MI01.2 are distributed on several switches and phys.
 * These ports bitmask below are virtual port bitmask for a pseudo switch covers
 * IPQ53XX and QCA8386.
 * bit0: VP0, bit1: VP1, bit2: VP2, bit3: VP3, bit4: VP4, bit5: VP5
 */
static unsigned int lan_mask = 0;	/* LAN only. Exclude WAN, WANS_LAN, and generic IPTV port. */
static unsigned int wan_mask = 0;	/* wan_type = WANS_DUALWAN_IF_WAN. Include generic IPTV port. */
static unsigned int wan2_mask = 0;	/* wan_type = WANS_DUALWAN_IF_WAN2. Include generic IPTV port. */
static unsigned int wans_lan_mask = 0;	/* wan_type = WANS_DUALWAN_IF_LAN. */

/* RT-N56U's P0, P1, P2, P3, P4 = LAN4, LAN3, LAN2, LAN1, WAN
 * ==> Model-specific virtual port number.
 * array inex:	RT-N56U's port number.
 * array value:	Model-specific virtual port number
 */
static int n56u_to_model_port_mapping[] = {
#if defined(TUFBE6500) || defined(TUFBE9400) || defined(RTBE50) || defined(RT4GBE58)
	LAN3_PORT,	//0000 0000 0001 LAN4 (convert to LAN3)
	LAN2_PORT,	//0000 0000 0010 LAN3 (convert to LAN2)
	LAN2_PORT,	//0000 0000 0100 LAN2
	LAN1_PORT,	//0000 0000 1000 LAN1
	WAN_PORT,	//0000 0001 0000 WAN
#elif defined(RTCONFIG_PORT2_DEVICE)
	LAN1_PORT,	//0000 0000 0001 LAN1
	LAN1_PORT,	//0000 0000 0010 LAN1
	LAN1_PORT,	//0000 0000 0100 LAN1
	LAN1_PORT,	//0000 0000 1000 LAN1
	WAN_PORT,	//0000 0001 0000 WAN
#else /* QCA MI01.2 reference board */
#warning FIXME
	LAN4_PORT,	//0000 0000 0001 LAN4
	LAN3_PORT,	//0000 0000 0010 LAN3
	LAN2_PORT,	//0000 0000 0100 LAN2
	LAN1_PORT,	//0000 0000 1000 LAN1
	WAN_PORT,	//0000 0001 0000 WAN
#endif
};

#define RTN56U_WAN_GMAC	(1U << 9)

/* Model-specific LANx ==> Model-specific virtual PortX.
 * array index:	Model-specific LANx (started from 0).
 * array value:	Model-specific virtual port number.
 */
const int lan_id_to_vport[NR_WANLAN_PORT] = {
#if defined(RTCONFIG_PORT2_DEVICE)
	LAN1_PORT,
#else
	LAN1_PORT,
	LAN2_PORT,
	LAN3_PORT,
	LAN4_PORT,
#endif
	WAN_PORT,
};

/* Model-specific LANx (started from 0) ==> Model-specific virtual PortX */
static inline int lan_id_to_vport_nr(int id)
{
	return lan_id_to_vport[id];
}

/**
 * Get WAN port mask
 * @wan_unit:	wan_unit, if negative, select WANS_DUALWAN_IF_WAN
 * @return:	port bitmask
 */
static unsigned int get_wan_port_mask(int wan_unit)
{
	int sw_mode = sw_mode();
	char nv[] = "wanXXXports_maskXXXXXX";

	if (sw_mode == SW_MODE_AP || sw_mode == SW_MODE_REPEATER)
#if defined(RTCONFIG_AMAS)
		if (!nvram_match("re_mode", "1"))
#endif	/* RTCONFIG_AMAS */
		return 0;

	if (wan_unit <= 0 || wan_unit >= WAN_UNIT_MAX)
		strlcpy(nv, "wanports_mask", sizeof(nv));
	else
		snprintf(nv, sizeof(nv), "wan%dports_mask", wan_unit);
	return nvram_get_int(nv);
}

/**
 * Get LAN port mask
 * @return:	port bitmask
 */
static unsigned int get_lan_port_mask(void)
{
	int sw_mode = sw_mode();
	unsigned int m = nvram_get_int("lanports_mask");

	if (sw_mode == SW_MODE_AP || __mediabridge_mode(sw_mode))
		m = wanlanports_mask;

	return m;
}

/**
 * Convert (v)port to interface name.
 * @vport:	(virtual) port number
 * @return:
 * 	NULL:	@vport doesn't map to specific interface name.
 *  otherwise:	@vport do map to a specific interface name.
 */
const char *vport_to_iface_name(unsigned int vport)
{
	if (vport >= ARRAY_SIZE(vport_to_iface)) {
		dbg("%s: don't know vport %d\n", __func__, vport);
		return NULL;
	}

	return vport_to_iface[vport];
}

/**
 * Convert interface name to (v)port.
 * NOTE:	LAN1~4 of QCA MI01.2 are not supported due to they share same interface!
 * @iface:	interface name
 * @return:	(virtual) port number
 *  >=0:	(virtual) port number
 *  < 0:	can't find (virtual) port number for @iface.
 */
int iface_name_to_vport(const char *iface)
{
	int ret = -2, i;

	if (!iface)
		return -1;

	for (i = 0; ret < 0 && i < ARRAY_SIZE(vport_to_iface); ++i) {
		if (!vport_to_iface[i] || strcmp(vport_to_iface[i], iface))
			continue;
		ret = i;
	}

	return ret;
}

/**
 * Convert vportmask to real portmask (QCA8386 only).
 * In most platform, vportmask = rportmask.
 * @vportmask:	virtual port mask
 * @return:	real portmask (QCA8386 only)
 */
unsigned int vportmask_to_rportmask(unsigned int vportmask)
{
	int vport, port_id;
	unsigned int rportmask = 0, m;

	for (vport = 0, m = vportmask;
	     vport < MAX_WANLAN_PORT && m != 0;
	     m >>= 1, vport++)
	{
		port_id = *(vport_to_ssdkport + vport);
		if (!(m & 1) || port_id < 0 || port_id >= 0x10)
			continue;

		rportmask |= (1U << port_id);
	}

	return rportmask;
}

/**
 * Set VLAN to a QCA8386 switch port.
 * @port:	port number of QCA8386 switch.
 * @vid:	VLAN ID
 * @prio:	VLAN PRIO
 * @untag:	if true, untag frame before it egress from @port.
 * @return:
 * 	0:	success
 *     -1:	invalid parameter
 */
int set_qca8386_port_vlan(int port, int vid, int prio, int untag)
{
	char *vmbr_type = untag? "untagged" : "tagged";
	char *vegress_type = untag? "untagged" : "untouched";
	char vid_str[6], port_str[4], prio_str[4];
	char *vmbr_add[] = { "ssdk_sh", SWID_QCA8386, "vlan", "member", "add", vid_str, port_str, vmbr_type, NULL };
	char *v1mbr_del[] = { "ssdk_sh", SWID_QCA8386, "vlan", "member", "del", "1", port_str, NULL };
	char *defcvid[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "defaultCVid", "set", port_str, vid_str, NULL };
	char *clrdefcvid[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "defaultCVid", "set", port_str, "0", NULL };
	char *defcpri[] = { "ssdk_sh", SWID_QCA8386, "qos", "ptDefaultCpri", "set", port_str, prio_str, NULL };
	char *vegress[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "egress", "set", port_str, vegress_type, NULL };
	char *vingress[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "ingress", "set", port_str, "secure", NULL };

	if (port < 0 || port > 6 || vid < 0 || vid >= 4096 || prio < 0 || prio > 7)
		return -1;

	snprintf(vid_str, sizeof(vid_str), "%d", vid);
	snprintf(port_str, sizeof(port_str), "%d", port);
	snprintf(prio_str, sizeof(prio_str), "%d", prio);

	/* Remove port from default VLAN1 and add to VLAN @vid. */
	_eval(v1mbr_del, DBGOUT, 0, NULL);
	_eval(vmbr_add, DBGOUT, 0, NULL);
	if (untag) {
		_eval(defcvid, DBGOUT, 0, NULL);
		_eval(defcpri, DBGOUT, 0, NULL);
	} else {
		_eval(clrdefcvid, DBGOUT, 0, NULL);
	}
	_eval(vegress, DBGOUT, 0, NULL);
	_eval(vingress, DBGOUT, 0, NULL);

	return 0;
}

/**
 * Set a VLAN
 * @vtype:	VLAN type
 * 	0:	VLAN for STB/VoIP
 * 	1:	VLAN for WAN
 * @upstream_if:upstream interface name
 * @vid:	VLAN ID, 0 ~ 4095.
 * @prio:	VLAN Priority
 * @mbr:	VLAN members
 * @untag:	VLAN members that need untag
 *
 * @return
 * 	0:	success
 *     -1:	invalid parameter
 */
int ipq53xx_qca8386_vlan_set(int vtype, int vport, int vid, int prio, unsigned int mbr, unsigned int untag)
{
	unsigned int m = mbr, u = untag, upstream_mask = 0;
	int i, port, wan_vlan_br = 0, wan_br = 0;
	char upstream_if[IFNAMSIZ], wvlan_if[IFNAMSIZ], qvlan_if[IFNAMSIZ] __attribute__((unused)), vid_str[6], prio_str[4], brv_if[IFNAMSIZ];
	char *ventry_create[] __attribute__((unused)) = { "ssdk_sh", SWID_QCA8386, "vlan", "entry", "create", vid_str, NULL };
	char *add_upst_viface[] = { "ip", "link", "add", "link", upstream_if, wvlan_if, "type", "vlan", "id", vid_str, NULL };
	char *add_8386_viface[] __attribute__((unused)) = { "ip", "link", "add", "link", QCA8386_IFACE, qvlan_if, "type", "vlan", "id", vid_str, NULL };
	char *p0vmbr_add[] __attribute__((unused)) = { "ssdk_sh", SWID_QCA8386, "vlan", "member", "add", vid_str, QCA8386_CPUPORT, "unmodified", NULL };
	char *set_upst_viface_egress_map[] = { "vconfig", "set_egress_map", wvlan_if, "0", prio_str, NULL };

	dbg("%s: vtype %d vport %d vid %d prio %d mbr 0x%x untag 0x%x\n",
		__func__, vtype, vport, vid, prio, mbr, untag);
	if (vtype < 0 || vtype >= VLAN_TYPE_MAX || vport < 0 || vport >= MAX_WANLAN_PORT) {
		dbg("%s: invalid parameter\n", __func__);
		return -1;
	}

	if (vtype != VLAN_TYPE_LAN_NO_VLAN)
		upstream_mask = 1U << vport;
	strlcpy(upstream_if, vport_to_iface_name(vport), sizeof(upstream_if));

	if (vtype == VLAN_TYPE_WAN_NO_VLAN) {
		wan_br = 1;
		vtype = VLAN_TYPE_WAN;
	}
	if (vtype == VLAN_TYPE_WAN && (mbr & ~(1U << WAN_PORT)) != 0)
		wan_vlan_br = 1;

	/* Replace WAN port as selected upstream port. */
	if (mbr & (1U << WAN_PORT)) {
		mbr &= ~(1U << WAN_PORT);
		mbr |= upstream_mask;
	}
	if (untag & (1U << WAN_PORT)) {
		untag &= ~(1U << WAN_PORT);
		untag |= upstream_mask;
	}

	snprintf(vid_str, sizeof(vid_str), "%d", vid);
	snprintf(prio_str, sizeof(prio_str), "%d", prio);
	snprintf(brv_if, sizeof(brv_if), "brv%d", vid);

	if ((vtype == VLAN_TYPE_WAN && wan_vlan_br) || vtype == VLAN_TYPE_STB_VOIP) {
		/* Use bridge to connect WAN and STB/VoIP. */
		eval("brctl", "addbr", brv_if);
		eval("ifconfig", brv_if, "0.0.0.0", "up");

#if 0
                set_netdev_sysfs_param(brv_if, "bridge/multicast_querier", "1");
                set_netdev_sysfs_param(brv_if, "bridge/multicast_snooping",
                        nvram_match("switch_br_no_snooping", "1")? "0" : "1");
#else
                /* should not handle the multicast on IPTV bridge */
                set_netdev_sysfs_param(brv_if, "bridge/multicast_querier", "0");
                set_netdev_sysfs_param(brv_if, "bridge/multicast_snooping", "0");
#endif
	}

	if (vtype == VLAN_TYPE_WAN || vtype == VLAN_TYPE_LAN_NO_VLAN) {
		if (wan_br) {
			/* In this case, no VLAN on WAN port. */
			strlcpy(wvlan_if, upstream_if, sizeof(wvlan_if));
		} else {
			/* Follow naming rule in set_basic_ifname_vars() on upstream interface. */
			snprintf(wvlan_if, sizeof(wvlan_if), "vlan%d", vid);
			_eval(add_upst_viface, DBGOUT, 0, NULL);
			_eval(set_upst_viface_egress_map, DBGOUT, 0, NULL);
			eval("ifconfig", upstream_if, "0.0.0.0", "up");
		}
		if (wan_vlan_br) {
			eval("brctl", "addif", brv_if, wvlan_if);
		}
		eval("ifconfig", wvlan_if, "0.0.0.0", "up");
	} else if (vtype == VLAN_TYPE_STB_VOIP) {
		snprintf(wvlan_if, sizeof(wvlan_if), "%s.%d", upstream_if, vid);
		_eval(add_upst_viface, DBGOUT, 0, NULL);
		_eval(set_upst_viface_egress_map, DBGOUT, 0, NULL);
		eval("brctl", "addif", brv_if, wvlan_if);
		eval("ifconfig", wvlan_if, "0.0.0.0", "up");
	}

	if ((vtype == VLAN_TYPE_WAN && wan_vlan_br) || vtype == VLAN_TYPE_STB_VOIP || vtype == VLAN_TYPE_LAN_NO_VLAN) {
#if !defined(BD4D5) && !defined(BD4_OD)
		if (mbr & qca8386_vportmask) {
			if ((vtype == VLAN_TYPE_WAN && wan_vlan_br) || vtype == VLAN_TYPE_STB_VOIP) {
				snprintf(qvlan_if, sizeof(qvlan_if), "%s.%d", QCA8386_IFACE, vid);
				_eval(add_8386_viface, DBGOUT, 0, NULL);
				eval("brctl", "addif", brv_if, qvlan_if);
				eval("ifconfig", qvlan_if, "0.0.0.0", "up");
			}

			_eval(ventry_create, DBGOUT, 0, NULL);
			if (vid != 1)
				_eval(p0vmbr_add, DBGOUT, 0, NULL);
		}
#endif

		for (i = 0, m = mbr & ~upstream_mask, u = untag;
		     i < MAX_WANLAN_PORT && m > 0;
		     i++, m >>= 1, u >>= 1)
		{
			if (!(m & 1))
				continue;

			port = *(vport_to_ssdkport + i);
			if (port < 0) {
				continue;
			} else if (port >= 0x10) {
#if !defined(BD4D5) && !defined(BD4_OD)
				/* IPQ53XX ports */
				port -= 0x10;
				dbg("%s: vport %d iface %s vid %d prio %d u %d\n",
					__func__, i, vport_to_iface[i], vid, prio, u & 1);
				eval("brctl", "addif", brv_if, (char*) vport_to_iface[i]);
#else
				char *tmp_str;

				dbg("%s: vport %d iface %s vid %d prio %d u %d\n",
					__func__, i, vport_to_iface[i], vid, prio, u & 1);

				if (u & 1) {
					char br_untag_path[] = "/sys/class/net/ethXXXXXXXXXXXX/brport/untagged_vlan_en";
					eval("brctl", "addif", brv_if, (char*) vport_to_iface[i]);
					snprintf(br_untag_path, sizeof(br_untag_path), "/sys/class/net/%s/brport/untagged_vlan", (char*) vport_to_iface[i]);
					f_write_string(br_untag_path, vid_str, 0, 0); // vid value, include 0
					snprintf(br_untag_path, sizeof(br_untag_path), "/sys/class/net/%s/brport/untagged_vlan_en", (char*) vport_to_iface[i]);
					f_write_string(br_untag_path, "1", 0, 0); // enable
				} else {
					char lan_if[IFNAMSIZ], lan_vlan_if[IFNAMSIZ];
					char *add_lan_vlan_viface[] = { "ip", "link", "add", "link", lan_if, lan_vlan_if, "type", "vlan", "id", vid_str, NULL };
					char *set_lan_vlan_egress_map[] = { "vconfig", "set_egress_map", lan_vlan_if, "0", prio_str, NULL };
					snprintf(lan_if, sizeof(lan_if), "%s",  (char *)vport_to_iface[i]);
					snprintf(lan_vlan_if, sizeof(lan_vlan_if), "%s.%d", (char *)vport_to_iface[i], vid);
					_eval(add_lan_vlan_viface, DBGOUT, 0, NULL);
					_eval(set_lan_vlan_egress_map, DBGOUT, 0, NULL);
					eval("ifconfig", lan_vlan_if, "0.0.0.0", "up");
					eval("brctl", "addif", brv_if, lan_vlan_if);
				}
				// remove lan_nic from lan_ifnames
				tmp_str = strdup(nvram_safe_get("lan_ifnames"));
				if (tmp_str) {
					if (remove_word(tmp_str, ((char *)vport_to_iface[i]))) {
						trim_space(tmp_str);
						nvram_set("lan_ifnames", tmp_str);
					}
					free(tmp_str);
					eval("ifconfig", (char *)vport_to_iface[i], "0.0.0.0", "up");
				}
#endif
			} else {
				/* QCA8386 ports */
				dbg("%s: vport %d port %d vid %d prio %d u %d\n",
					__func__, i, port, vid, prio, u & 1);
				set_qca8386_port_vlan(port, vid, prio, u & 1);
			}
		}
	}

	return 0;
}

static void is_singtel_mio(int is)
{
	int i;
	char port_str[4], *admit_type = is? "admit_all" : "admit_untagged";
	char *vadmit[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "invlan", "set", port_str, admit_type, NULL };

	/* admit all/untagged frames if @is is true/false, except P0. */
	for (i = 1; i <= 4; ++i) {
		snprintf(port_str, sizeof(port_str), "%d", i);
		_eval(vadmit, DBGOUT, 0, NULL);
	}
}

/**
 * Get link status and/or phy speed of a port.
 * @link:	pointer to unsigned integer.
 * 		If link != NULL,
 * 			*link = 0 means link-down
 * 			*link = 1 means link-up.
 * @speed:	pointer to unsigned integer.
 * 		If speed != NULL,
 * 			*speed = 1 means 100Mbps
 * 			*speed = 2 means 1000Mbps
 * @return:
 * 	0:	success
 *     -1:	invalid parameter
 *  otherwise:	fail
 */
static int get_phy_info(unsigned int phy, unsigned int port_id, unsigned int *link, unsigned int *speed, phy_info *info)
{
        FILE *fp;
	size_t rlen;
	unsigned int l = 0, s = 0;
	char buf[512], *pt;

	if (phy == 1)
		snprintf(buf, sizeof(buf), "ssdk_sh %s port linkstatus get %d", SWID_QCA8386, port_id);
	else if (phy == 2)
		snprintf(buf, sizeof(buf), "ssdk_sh %s port linkstatus get %d", SWID_IPQ53XX, phy);
	else
		return -1;
	if ((fp = popen(buf, "r")) == NULL) {
		_dprintf("%s: Run [%s] fail!\n", __func__, buf);
		return -2;
	}
	rlen = fread(buf, 1, sizeof(buf), fp);
	pclose(fp);
	if (rlen <= 1)
		return -3;

	buf[rlen-1] = '\0';
	if ((pt = strstr(buf, "[Status]:")) == NULL)
		return -4;

	pt += 9; // strlen of "[Status]:"
	if (!strncmp(pt, "ENABLE", 6)) {
		l = 1;

		if (phy == 1)
			snprintf(buf, sizeof(buf), "ssdk_sh %s port speed get %d", SWID_QCA8386, port_id);
		else if (phy == 2)
			snprintf(buf, sizeof(buf), "ssdk_sh %s port speed get %d", SWID_IPQ53XX, phy);
		if ((fp = popen(buf, "r")) == NULL) {
			_dprintf("%s: Run [%s] fail!\n", __func__, buf);
			return -5;
		}
		rlen = fread(buf, 1, sizeof(buf), fp);
		pclose(fp);
		if (rlen <= 1)
			return -6;

		buf[rlen-1] = '\0';
		if ((pt = strstr(buf, "[speed]:")) == NULL)
			return -7;

		pt += 8; // strlen of "[speed]:"
		if (!strncmp(pt, "10000", 5))
			s = 3;
		else if (!strncmp(pt, "5000", 4))
			s = 5;
		else if (!strncmp(pt, "2500", 4))
			s = 4;
		else if (!strncmp(pt, "1000", 4))
			s = 2;
		else if (!strncmp(pt, "100", 3))
			s = 1;
		else
			s = 0;
	}

	if (link)
		*link = l;
	if (speed)
		*speed = s;
#ifdef RTCONFIG_NEW_PHYMAP
	if (info) {
		if (l) {
			snprintf(info->state, sizeof(info->state), "up");
			switch (s) {
			case 0:
				info->link_rate = 10;
				break;
			case 1:
				info->link_rate = 100;
				break;
			case 2:
				info->link_rate = 1000;
				break;
			case 3:
				info->link_rate = 10000;
				break;
			case 4:
				info->link_rate = 2500;
				break;
			case 5:
				info->link_rate = 5000;
				break;
			default:
				_dprintf("%s: invalid speed!\n", __func__);
			}
		}
		else
			snprintf(info->state, sizeof(info->state), "down");
	}
#endif

	return 0;
}

/**
 * Get link status and/or phy speed of a virtual port.
 * @vport:	virtual port number
 * @link:	pointer to unsigned integer.
 * 		If link != NULL,
 * 			*link = 0 means link-down
 * 			*link = 1 means link-up.
 * @speed:	pointer to unsigned integer.
 * 		If speed != NULL,
 * 			*speed = 1 means 100Mbps
 * 			*speed = 2 means 1000Mbps
 * @return:
 * 	0:	success
 *     -1:	invalid parameter
 *  otherwise:	fail
 */
static int get_ipq53xx_qca8386_vport_info(unsigned int vport, unsigned int *link, unsigned int *speed, phy_info *info)
{
	int phy, port_id;

	if (vport >= MAX_WANLAN_PORT || (!link && !speed))
		return -1;

	if (link)
		*link = 0;
	if (speed)
		*speed = 0;

	phy = *(vport_to_phy_addr + vport);
	if (phy < 0) {
		//dbg("%s: can't get PHY address of vport %d\n", __func__, vport);
		return -1;
	}

	port_id = *(vport_to_ssdkport + vport);
	if (port_id >= 0x10)
		port_id -= 0x10;
	get_phy_info(phy, port_id, link, speed, info);

	return 0;
}

/**
 * Get linkstatus in accordance with port bit-mask.
 * @mask:	port bit-mask.
 * 		bit0 = VP0, bit1 = VP1, etc.
 * @linkStatus:	link status of all ports that is defined by mask.
 * 		If one port of mask is linked-up, linkStatus is true.
 */
static void get_ipq53xx_qca8386_phy_linkStatus(unsigned int mask, unsigned int *linkStatus)
{
	int i;
	unsigned int value = 0, m;

	m = mask & wanlanports_mask;
	for (i = 0; m > 0 && !value; ++i, m >>= 1) {
		if (!(m & 1))
			continue;

		get_ipq53xx_qca8386_vport_info(i, &value, NULL, NULL);
		value &= 0x1;
	}
	*linkStatus = value;
}

/**
 * Set wanports_mask, wanXports_mask, and lanports_mask based on
 * nvram configuration, @stb, and @stb_bitmask parameters.
 * @stb_bitmask should be platform-specific (v)port bitmask.
 */
static void build_wan_lan_mask(int stb, int stb_bitmask)
{
	int i, unit, type, m, upstream_unit = __get_upstream_wan_unit();
	int wanscap_lan = get_wans_dualwan() & WANSCAP_LAN;
	int wans_lanport = nvram_get_int("wans_lanport");
	int sw_mode = sw_mode();
	char prefix[8], nvram_ports[20];
	unsigned int unused_wan_mask = 0, iptv_mask = 0;

	if (stb < 0 || stb >= ARRAY_SIZE(stb_to_mask)) {
		return;
	}
	if (sw_mode == SW_MODE_AP || sw_mode == SW_MODE_REPEATER)
		wanscap_lan = 0;

	if (wanscap_lan && (wans_lanport < 0 || wans_lanport > 4)) {
		_dprintf("%s: invalid wans_lanport %d!\n", __func__, wans_lanport);
		wanscap_lan = 0;
	}

	/* To compatible to original architecture, @stb and @stb_bitmask are exclusive.
	 * @stb is assumed as zero if @stb_bitmask is non-zero value.  Because
	 * "rtkswitch  8 X" specifies STB port configuration via 0 ~ 6 and
	 * "rtkswitch 38 X" specifies VoIP/STB port via RT-N56U's port bitmask.
	 */
	if (stb_bitmask != 0)
		stb = 0;
	if (sw_mode == SW_MODE_ROUTER) {
		if (stb_bitmask != 0)
			iptv_mask = stb_bitmask;
		else
			iptv_mask = stb_to_mask[stb];
	}

	lan_mask = wan_mask = wans_lan_mask = 0;
	for (i = 0; i < NR_WANLAN_PORT; ++i) {
		switch (lan_wan_partition[stb][i]) {
		case 0:
			wan_mask |= 1U << lan_id_to_vport_nr(i);
			break;
		case 1:
			lan_mask |= 1U << lan_id_to_vport_nr(i);
			break;
#if 0
		case 2:
			wan2_mask |= 1U << lan_id_to_vport_nr(i);
			break;
#endif
		default:
			_dprintf("%s: Unknown LAN/WAN port definition. (stb %d i %d val %d)\n",
				__func__, stb, i, lan_wan_partition[stb][i]);
		}
	}

#if defined(RTCONFIG_BONDING_WAN)
	if (bond_wan_enabled()) {
		int b;
		uint32_t m =  nums_str_to_u32_mask(nvram_get("wanports_bond"));

		while ((b = ffs(m)) > 0) {
			b--;
			m &= ~(1U << b);
			wan_mask |= 1U << bsport_to_vport[b];
		}
	}
#endif

	/* One of LAN port is acting as WAN. */
	if (wanscap_lan) {
		wans_lan_mask = 1U << lan_id_to_vport_nr(wans_lanport - 1);
		lan_mask &= ~wans_lan_mask;
	}

	unused_wan_mask = wan_mask | wan2_mask;
	for (unit = WAN_UNIT_FIRST; unit < WAN_UNIT_MAX; ++unit) {
		snprintf(prefix, sizeof(prefix), "%d", unit);
		snprintf(nvram_ports, sizeof(nvram_ports), "wan%sports_mask", (unit == WAN_UNIT_FIRST)?"":prefix);
		m = 0;	/* In AP/RP/MB mode, all WAN ports are bridged to LAN. */

		if (sw_mode == SW_MODE_ROUTER
#ifdef RTCONFIG_AMAS
		 ||(sw_mode == SW_MODE_AP && nvram_match("re_mode", "1"))
#endif	/* RTCONFIG_AMAS */
		 ) {
			type = get_dualwan_by_unit(unit);

			switch (type) {
			case WANS_DUALWAN_IF_WAN:
				m = wan_mask;
				unused_wan_mask &= ~wan_mask;
				break;
			case WANS_DUALWAN_IF_WAN2:
				m = wan2_mask;
				unused_wan_mask &= ~wan2_mask;
				break;
			case WANS_DUALWAN_IF_LAN:
				m = wans_lan_mask;
				break;
			default:
				nvram_unset(nvram_ports);
				break;
			}

			if (m == 0)
				continue;

			if (unit == upstream_unit)
				m |= iptv_mask;
		}
		nvram_set_int(nvram_ports, m);
	}

	/* Let all unused WAN ports become LAN ports.
	 * 1. 10G RJ-45 and 10G SFP+ can be WAN or LAN, depends on dualwan configuration.
	 * 2. 1G WAN can aggreate with LAN1/LAN2, creates 3Gbps bandwidth. (TODO)
	 */
	lan_mask = (lan_mask | unused_wan_mask) & ~iptv_mask;

#if defined(RTCONFIG_LACP)
	if (nvram_match("lacp_enabled", "1")) {
		unsigned int lacp_mask = (1U << LAN1_PORT) | (1U << LAN2_PORT);
		lan_mask &= ~lacp_mask;
	}
#endif

	nvram_set_int("lanports_mask", lan_mask);
}

/**
 * Configure LAN/WAN partition base on generic IPTV type.
 * @type:
 * 	0:	Default.
 * 	1:	LAN1
 * 	2:	LAN2
 * 	3:	LAN3
 * 	4:	LAN4
 * 	5:	LAN1+LAN2
 * 	6:	LAN3+LAN4
 */
static void config_ipq53xx_qca8386_LANWANPartition(int type)
{
	build_wan_lan_mask(type, 0);
	reset_qca_switch();
	dbg("%s: LAN/P.WAN/S.WAN portmask %08x/%08x/%08x Upstream %s (unit %d)\n",
		__func__, lan_mask, nvram_get_int("wanports_mask"), nvram_get_int("wan1ports_mask"),
		get_wan_base_if(), __get_upstream_wan_unit());

	// DUALWAN
#if !defined(BD4D5) && !defined(BD4_OD)
	if (wans_lan_mask) {
		int wans_lanport = nvram_get_int("wans_lanport");
		ipq53xx_qca8386_vlan_set(VLAN_TYPE_LAN_NO_VLAN, lan_id_to_vport_nr(wans_lanport - 1), 3, 0, wans_lan_mask, wans_lan_mask);
	}
#endif
}

static void get_ipq53xx_qca8386_Port_Speed(unsigned int port_mask, unsigned int *speed)
{
	int i, v = -1, t;
	unsigned int m;

	if(speed == NULL)
		return;

	m = port_mask & wanlanports_mask;
	for (i = 0; m; ++i, m >>= 1) {
		if (!(m & 1))
			continue;
		get_ipq53xx_qca8386_vport_info(i, NULL, (unsigned int*) &t, NULL);

		t &= 0x7;
		if (t > v)
			v = t;
	}

	switch (v) {
	case 0x0:
		*speed = 10;
		break;
	case 0x1:
		*speed = 100;
		break;
	case 0x2:
		*speed = 1000;
		break;
	case 0x3:
		*speed = 10000;
		break;
	case 0x4:
		*speed = 2500;
		break;
	case 0x5:
		*speed = 5000;
		break;
	default:
		_dprintf("%s: invalid speed!\n", __func__);
	}
}

static void get_ipq53xx_qca8386_WAN_Speed(unsigned int *speed)
{
	get_ipq53xx_qca8386_Port_Speed(get_wan_port_mask(0) | get_wan_port_mask(1), speed);
}

/**
 * @vpmask:	Virtual port mask
 * @status:	0: power down PHY; otherwise: power up PHY
 */
static void link_down_up_ipq53xx_qca8386_PHY(unsigned int vpmask, int status)
{
	int vport, phy, port_id;
	unsigned int m;
	char idx[3], *swid;

	vpmask &= wanlanports_mask;
	for (vport = 0, m = vpmask; m; ++vport, m >>= 1) {
		if (!(m & 1))
			continue;
		if (vport >= MAX_WANLAN_PORT) {
			dbg("%s: PHY address is not defined for vport %d\n", __func__, vport);
			continue;
		}

		phy = *(vport_to_phy_addr + vport);
		if (phy < 0) {
			dbg("%s: can't get PHY address of vport %d\n", __func__, vport);
			return;
		}

		port_id = *(vport_to_ssdkport + vport);
		swid = SWID_QCA8386;
		if (port_id >= 0x10 && port_id < 0x20) {
			port_id -= 0x10;
			swid = SWID_IPQ53XX;
		}
		sprintf(idx, "%d", port_id);
		eval("ssdk_sh", swid, "port", status? "poweron" : "poweroff", "set", idx);
	}
}

void reset_qca_switch(void)
{
	nvram_unset("vlan_idx");
}

static void set_Vlan_VID(int vid)
{
	char tmp[8];

	snprintf(tmp, sizeof(tmp), "%d", vid);
	nvram_set("vlan_vid", tmp);
}

static void set_Vlan_PRIO(int prio)
{
	char tmp[2];

	snprintf(tmp, sizeof(tmp), "%d", prio);
	nvram_set("vlan_prio", tmp);
}

static int convert_n56u_portmask_to_model_portmask(unsigned int orig)
{
	int i, bit, bitmask;
	bitmask = 0;
	for(i = 0; i < ARRAY_SIZE(n56u_to_model_port_mapping); i++) {
		bit = (1 << i);
		if (orig & bit)
			bitmask |= (1 << n56u_to_model_port_mapping[i]);
	}
	return bitmask;
}

/**
 * @stb_bitmask:	bitmask of STB port(s)
 * 			e.g. bit0 = P0, bit1 = P1, etc.
 */
static void initialize_Vlan(int stb_bitmask)
{
	char *p, wan_base_if[IFNAMSIZ] = "N/A";

	stb_bitmask = convert_n56u_portmask_to_model_portmask(stb_bitmask);
	build_wan_lan_mask(0, stb_bitmask);
	if ((p = get_wan_base_if()) != NULL)
		strlcpy(wan_base_if, p, sizeof(wan_base_if));

	dbg("%s: LAN/P.WAN/S.WAN portmask %08x/%08x/%08x Upstream %s (unit %d)\n",
		__func__, lan_mask, nvram_get_int("wanports_mask"), nvram_get_int("wan1ports_mask"),
		get_wan_base_if(), __get_upstream_wan_unit());
	reset_qca_switch();
}

/**
 * Create VLAN for LAN and/or WAN in accordance with bitmask parameter.
 * @bitmask:
 *  bit15~bit0:		member port bitmask.
 * 	bit0:		RT-N56U port0, LAN4
 * 	bit1:		RT-N56U port1, LAN3
 * 	bit2:		RT-N56U port2, LAN2
 * 	bit3:		RT-N56U port3, LAN1
 * 	bit4:		RT-N56U port4, WAN
 * 	bit8:		RT-N56U port8, LAN_CPU port
 * 	bit9:		RT-N56U port9, WAN_CPU port
 *  bit31~bit16:	untag port bitmask.
 * 	bit16:		RT-N56U port0, LAN4
 * 	bit17:		RT-N56U port1, LAN3
 * 	bit18:		RT-N56U port2, LAN2
 * 	bit19:		RT-N56U port3, LAN1
 * 	bit20:		RT-N56U port4, WAN
 * 	bit24:		RT-N56U port8, LAN_CPU port
 * 	bit25:		RT-N56U port9, WAN_CPU port
 * First Ralink-based model is RT-N56U.
 * Convert RT-N56U-specific bitmask to physical port of your model,
 * base on relationship between physical port and visual WAN/LAN1~4 of that model first.
 */
static void create_Vlan(int bitmask)
{
	const int vid = nvram_get_int("vlan_vid");
	const int prio = nvram_get_int("vlan_prio") & 0x7;
	unsigned int mbr = bitmask & 0xffff;
	unsigned int untag = (bitmask >> 16) & 0xffff;
	unsigned int mbr_qca, untag_qca;
	int vtype = VLAN_TYPE_STB_VOIP;

	//convert port mapping
	dbg("%s: bitmask:%08x, mbr:%08x, untag:%08x\n", __func__, bitmask, mbr, untag);
	mbr_qca   = convert_n56u_portmask_to_model_portmask(mbr);
	untag_qca = convert_n56u_portmask_to_model_portmask(untag);
	dbg("%s: mbr_qca:%08x, untag_qca:%08x\n", __func__, mbr_qca, untag_qca);
	if (untag & 0x10) {
		vtype = VLAN_TYPE_WAN_NO_VLAN;
	} else if (mbr & RTN56U_WAN_GMAC) {
		/* setup VLAN for WAN (WAN1 or WAN2), not VoIP/STB */
		vtype = VLAN_TYPE_WAN;
	}

	/* selecet upstream port for IPTV port. */
	ipq53xx_qca8386_vlan_set(vtype, WAN_PORT, vid, prio, mbr_qca, untag_qca);
}

int ipq53xx_qca8386_ioctl(int val, int val2)
{
	unsigned int value2 = 0;
	int i, max_wan_unit = 0;

#if defined(RTCONFIG_DUALWAN)
	max_wan_unit = 1;
#endif

	switch (val) {
	case 0:
		value2 = rtkswitch_wanPort_phyStatus(-1);
		printf("WAN link status : %u\n", value2);
		break;
	case 3:
		value2 = rtkswitch_lanPorts_phyStatus();
		printf("LAN link status : %u\n", value2);
		break;
	case 8:
		config_ipq53xx_qca8386_LANWANPartition(val2);
		break;
	case 13:
		get_ipq53xx_qca8386_WAN_Speed(&value2);
		printf("WAN speed : %u Mbps\n", value2);
		break;
	case 14: // Link up LAN ports
		link_down_up_ipq53xx_qca8386_PHY(get_lan_port_mask(), 1);
		break;
	case 15: // Link down LAN ports
		link_down_up_ipq53xx_qca8386_PHY(get_lan_port_mask(), 0);
		break;
	case 16: // Link up ALL ports
		link_down_up_ipq53xx_qca8386_PHY(wanlanports_mask, 1);
		break;
	case 17: // Link down ALL ports
		link_down_up_ipq53xx_qca8386_PHY(wanlanports_mask, 0);
		break;
	case 27:
		reset_qca_switch();
		break;
	case 36:
		set_Vlan_VID(val2);
		break;
	case 37:
		set_Vlan_PRIO(val2);
		break;
	case 38:
		initialize_Vlan(val2);
		break;
	case 39:
		create_Vlan(val2);
		break;
	case 40:
		is_singtel_mio(val2);
		break;
	case 114: // link up WAN ports
		for (i = WAN_UNIT_FIRST; i <= max_wan_unit; ++i)
			link_down_up_ipq53xx_qca8386_PHY(get_wan_port_mask(i), 1);
		break;
	case 115: // link down WAN ports
		for (i = WAN_UNIT_FIRST; i <= max_wan_unit; ++i)
			link_down_up_ipq53xx_qca8386_PHY(get_wan_port_mask(i), 0);
		break;
	case 200:	/* set LAN port number that is used as WAN port */
		/* Nothing to do, nvram_get_int("wans_lanport ") is enough. */
		break;

	/* unused ioctl command. */
	case 21:	/* reset storm control rate, only Realtek switch platform need. */
	case 22:	/* set unknown unicast storm control rate. RTK switch only. */
	case 23:	/* set unknown multicast storm control rate. RTK switch only. */
	case 24:	/* set multicast storm control rate. RTK switch only. */
	case 25:	/* set broadcast storm rate. RTK switch only. */
	case 29:	/* Set VoIP port.  Not using any more. */
	case 50:	/* Fix-up hwnat for WiFi interface on MTK platform. */
		break;
	default:
		printf("wrong ioctl cmd: %d\n", val);
	}

	return 0;
}

int config_rtkswitch(int argc, char *argv[])
{
	int val;
	int val2 = 0;
	char *cmd = NULL;
	char *cmd2 = NULL;

	if (argc >= 2)
		cmd = argv[1];
	else
		return -1;
	if (argc >= 3)
		cmd2 = argv[2];

	val = (int) strtol(cmd, NULL, 0);
	if (cmd2)
		val2 = (int) strtol(cmd2, NULL, 0);

	return ipq53xx_qca8386_ioctl(val, val2);
}

unsigned int
rtkswitch_Port_phyStatus(unsigned int port_mask)
{
        unsigned int status = 0;

	get_ipq53xx_qca8386_phy_linkStatus(port_mask, &status);

        return status;
}

unsigned int
rtkswitch_Port_phyLinkRate(unsigned int port_mask)
{
	unsigned int speed = 0;

	get_ipq53xx_qca8386_Port_Speed(port_mask, &speed);

	return speed;
}

int get_net_if_status(char *ifname)
{
	char tmp[100], buf[32];

	snprintf(tmp, sizeof(tmp), "/sys/class/net/%s/operstate", ifname);

	f_read_string(tmp, buf, sizeof(buf));
	if(!strncmp(buf, "up", 2))
		return 1;
	else
		return 0;
}

unsigned int
rtkswitch_wanPort_phyStatus(int wan_unit)
{
	unsigned int status = 0;

#if defined(BD4D5) || defined(BD4_OD)
	status = get_net_if_status(get_wanx_ifname(wan_unit));
#else
	get_ipq53xx_qca8386_phy_linkStatus(get_wan_port_mask(wan_unit), &status);
#endif

	return status;
}

unsigned int
rtkswitch_lanPorts_phyStatus(void)
{
	unsigned int status = 0;

	get_ipq53xx_qca8386_phy_linkStatus(get_lan_port_mask(), &status);

	return status;
}

unsigned int
rtkswitch_WanPort_phySpeed(void)
{
	unsigned int speed;

	get_ipq53xx_qca8386_WAN_Speed(&speed);

	return speed;
}

int rtkswitch_WanPort_linkUp(void)
{
	eval("rtkswitch", "114");

	return 0;
}

int rtkswitch_WanPort_linkDown(void)
{
	eval("rtkswitch", "115");

	return 0;
}

int
rtkswitch_LanPort_linkUp(void)
{
	system("rtkswitch 14");

	return 0;
}

int
rtkswitch_LanPort_linkDown(void)
{
	system("rtkswitch 15");

	return 0;
}

int
rtkswitch_AllPort_linkUp(void)
{
	system("rtkswitch 16");

	return 0;
}

int
rtkswitch_AllPort_linkDown(void)
{
	system("rtkswitch 17");

	return 0;
}

int
rtkswitch_Reset_Storm_Control(void)
{
	system("rtkswitch 21");

	return 0;
}

/**
 * @link:
 * 	0:	no-link
 * 	1:	link-up
 * @speed:
 * 	0,10:		10Mbps		==> 'M'
 * 	1,100:		100Mbps		==> 'M'
 * 	2,1000:		1000Mbps	==> 'G'
 * 	3,10000:	10Gbps		==> 'T'
 * 	4,2500:		2.5Gbps		==> 'Q'
 * 	5,5000:		5Gbps		==> 'F'
 */
static char conv_speed(unsigned int link, unsigned int speed)
{
	char ret = 'X';

	if (link != 1)
		return ret;

	if (speed == 2 || speed == 1000)
		ret = 'G';
	else if (speed == 3 || speed == 10000)
		ret = 'T';
	else if (speed == 4 || speed == 2500)
		ret = 'Q';
	else if (speed == 5 || speed == 5000)
		ret = 'F';
	else
		ret = 'M';

	return ret;
}

void ATE_port_status(int verbose, phy_info_list *list)
{
	int i, len;
	char buf[50];
#ifdef RTCONFIG_NEW_PHYMAP
	char cap_buf[64] = {0};
#endif
	phyState pS;

#ifdef RTCONFIG_NEW_PHYMAP
	phy_port_mapping port_mapping;
	get_phy_port_mapping(&port_mapping);

	len = 0;
	for (i = 0; i < port_mapping.count; i++) {
		// Only handle WAN/LAN ports
		if (((port_mapping.port[i].cap & PHY_PORT_CAP_WAN) == 0) && ((port_mapping.port[i].cap & PHY_PORT_CAP_LAN) == 0))
			continue;
		pS.link[i] = 0;
		pS.speed[i] = 0;
		get_ipq53xx_qca8386_vport_info(port_mapping.port[i].phy_port_id, &pS.link[i], &pS.speed[i], list ? &list->phy_info[i] : NULL);
		if (list) {
			list->phy_info[i].phy_port_id = port_mapping.port[i].phy_port_id;
			snprintf(list->phy_info[i].label_name, sizeof(list->phy_info[i].label_name), "%s", 
				port_mapping.port[i].label_name);
			list->phy_info[i].cap = port_mapping.port[i].cap;
			snprintf(list->phy_info[i].cap_name, sizeof(list->phy_info[i].cap_name), "%s", 
				get_phy_port_cap_name(port_mapping.port[i].cap, cap_buf, sizeof(cap_buf)));
			/*if (pS.link[i] == 1 && !list->status_and_speed_only)
				//TODO not complete*/

			list->count++;
		}
		len += sprintf(buf+len, "%s=%C;", port_mapping.port[i].label_name,
			conv_speed(pS.link[i], pS.speed[i]));
	}

#else
	for (i = 0; i < NR_WANLAN_PORT; i++) {
		get_ipq53xx_qca8386_vport_info(lan_id_to_vport_nr(i), &pS.link[i], &pS.speed[i], NULL);
	}

	len = 0;
	if (vport_to_phy_addr[WAN_PORT] >= 0)
		len += sprintf(buf+len, "W0=%C;", conv_speed(pS.link[WAN_PORT], pS.speed[WAN_PORT]));
	for (i = 0; i < WAN_PORT; i++) {
		if (vport_to_phy_addr[i] >= 0)
			len += sprintf(buf+len, "L%d=%C;", i+1, conv_speed(pS.link[i], pS.speed[i]));
		else
			; // break;
	}
#endif

	if (verbose)
		puts(buf);
}

/* Callback function which is used to fin brvX interface, X must be number.
 * @return:
 * 	0:	d->d_name is not brvX interface.
 *  non-zero:	d->d_name is brvx interface.
 */
static int brvx_filter(const struct dirent *d)
{
	const char *p;

	if (!d || strncmp(d->d_name, "brv", 3))
		return 0;

	p = d->d_name + 3;
	while (*p != '\0') {
		if (!isdigit(*p))
			return 0;
		p++;
	}

	return 1;
}

void __pre_config_switch(void)
{
	int i, j, nr_brvx, nr_brif;
#if defined(RTCONFIG_BONDING_WAN)
	int bw_vport = -1;
#endif
	struct dirent **brvx = NULL, **brif = NULL;
	char vlan1_if[IFNAMSIZ] __attribute__((unused)), port_str[4];
	char brif_path[sizeof("/sys/class/net/X/brifXXXXX") + IFNAMSIZ];
	char *vlan_flush[] __attribute__((unused)) = { "ssdk_sh", SWID_QCA8386, "vlan", "entry", "flush", NULL };
	char *create_vlan1[] __attribute__((unused)) = { "ssdk_sh", SWID_QCA8386, "vlan", "entry", "create", "1", NULL };
	char *p0v1vmbr_add[] = { "ssdk_sh", SWID_QCA8386, "vlan", "member", "add", "1", QCA8386_CPUPORT, "unmodified", NULL };
	char *v1mbr_add[] = { "ssdk_sh", SWID_QCA8386, "vlan", "member", "add", "1", port_str, "untagged", NULL };
	char *def1cvid[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "defaultCVid", "set", port_str, "1", NULL };
	char *vegress[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "egress", "set", port_str, "untouched", NULL };
	char *p0vegress[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "egress", "set", QCA8386_CPUPORT, "untouched", NULL };
	char *vingress[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "ingress", "set", port_str, "check", NULL };

	/* Remove all brvXXX bridge interfaces that are used to bridge WAN and STB/VoIP. */
	nr_brvx = scandir(SYS_CLASS_NET, &brvx, brvx_filter, alphasort);
	for (i = 0; i < nr_brvx; ++i) {
		snprintf(brif_path, sizeof(brif_path), "%s/%s/brif", SYS_CLASS_NET, brvx[i]->d_name);
		nr_brif = scandir(brif_path, &brif, NULL, alphasort);
		if (nr_brif <= 0) {
			free(brvx[i]);
			continue;
		}

		for (j = 0; j < nr_brif; ++j) {
			eval("brctl", "delif", brvx[i]->d_name, brif[j]->d_name);
		}
		free(brif);
		free(brvx[i]);
	}
	free(brvx);

	/* Set P1 ~ P4 of QCA8386 as untouched, otherwise, RE's guest client can't get IP address
	 * due to VLAN 501/502 DHCP request/reply frames are untagged by QCA8386.
	 */
	for (i = 0; i < NR_WANLAN_PORT; ++i) {
		int port_id = *(vport_to_ssdkport + i);

		if (port_id < 0 || port_id >= 0x10)
			continue;

		snprintf(port_str, sizeof(port_str), "%d", port_id);
		_eval(vegress, DBGOUT, 0, NULL);
	}

	if (aimesh_re_node())
		_eval(p0vegress, DBGOUT, 0, NULL);

	if (sw_mode() != SW_MODE_ROUTER)
		return;

#if !defined(BD4D5) && !defined(BD4_OD)
	snprintf(vlan1_if, sizeof(vlan1_if), "%s.%s", QCA8386_IFACE, "1");
	eval("ip", "link", "add", "link", QCA8386_IFACE, "name", vlan1_if, "type", "vlan", "id", "1");
	eval("ifconfig", QCA8386_IFACE, "0.0.0.0", "up");
	eval("ifconfig", vlan1_if, "0.0.0.0", "up");

	/* Flush VLAN entry and create VLAN1 for LAN. */
	_eval(vlan_flush, DBGOUT, 0, NULL);
	_eval(create_vlan1, DBGOUT, 0, NULL);
#endif

#if defined(RTCONFIG_BONDING_WAN)
	/* create VLAN90X for bonding WAN */
	if (bond_wan_enabled()) {
		int b;
		uint32_t m =  nums_str_to_u32_mask(nvram_get("wanports_bond"));

		while ((b = ffs(m)) > 0) {
			b--;
			m &= ~(1U << b);
			if (b == BS_WAN_PORT_ID)
				continue;
			bw_vport = bsport_to_vport[b];
			ipq53xx_qca8386_vlan_set(VLAN_TYPE_LAN_NO_VLAN, bw_vport, (BVID_OFFSET + bw_vport), 0, (1 << bw_vport), (1 << bw_vport));
		}
	}
#endif

	/* Add all ports to VLAN1, accept untagged frames only, and egress untouched. */
	_eval(p0v1vmbr_add, DBGOUT, 0, NULL);
	for (i = 0; i < NR_WANLAN_PORT; ++i) {
		int port_id = *(vport_to_ssdkport + i);

		if (port_id < 0 || port_id >= 0x10)
			continue;
#if defined(RTCONFIG_LACP)
		/* ignore LACP port into VLAN1 */
		if (nvram_match("lacp_enabled", "1")
		 && (i == LAN1_PORT || i == LAN2_PORT))
			continue;
#endif
#if defined(RTCONFIG_BONDING_WAN)
		/* ignore bonding WAN port into VLAN1 */
		if (bond_wan_enabled() && i == bw_vport)
			continue;
#endif

		snprintf(port_str, sizeof(port_str), "%d", port_id);
		_eval(v1mbr_add, DBGOUT, 0, NULL);
		_eval(def1cvid, DBGOUT, 0, NULL);
		_eval(vingress, DBGOUT, 0, NULL);
	}

#if defined(RTCONFIG_LACP)
	/* create VLAN90X for LACP */
	if (nvram_match("lacp_enabled", "1")) {
		ipq53xx_qca8386_vlan_set(VLAN_TYPE_LAN_NO_VLAN, LAN1_PORT, (BVID_OFFSET + LAN1_PORT), 0, (1 << LAN1_PORT), (1 << LAN1_PORT));
		ipq53xx_qca8386_vlan_set(VLAN_TYPE_LAN_NO_VLAN, LAN2_PORT, (BVID_OFFSET + LAN2_PORT), 0, (1 << LAN2_PORT), (1 << LAN2_PORT));
	}
#endif
}

void __post_config_switch(void)
{
	int i;
	char port[sizeof("1XX")];
	char *ipq53xx_p1_8023az[] = { "ssdk_sh", SWID_IPQ53XX, "port", "ieee8023az", "set", "1", "disable", NULL };
	char *qca8386_px_8023az[] = { "ssdk_sh", SWID_QCA8386, "port", "ieee8023az", "set", port, "disable", NULL };
	char *p0_vegress[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "egress", "set", QCA8386_CPUPORT, "untouched", NULL };
	char *p0_vingress[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "ingress", "set", QCA8386_CPUPORT, "check", NULL };

#if defined(TUFBE6500) || defined(TUFBE9400)
	/* Turn off GRO of eth0, eth1. */
	ctrl_gro("eth0", 0);
	ctrl_gro("eth1", 0);
#endif

	/* Always turn off IEEE 802.3az support on IPQ53XX port 1 and QCA8386 port 1~5. */
	_eval(ipq53xx_p1_8023az, DBGOUT, 0, NULL);
	for (i = 1; i <= 5; ++i) {
		snprintf(port, sizeof(port), "%d", i);
		_eval(qca8386_px_8023az, DBGOUT, 0, NULL);
	}

	if (sw_mode() != SW_MODE_ROUTER)
		return;

	_eval(p0_vegress, DBGOUT, 0, NULL);
	_eval(p0_vingress, DBGOUT, 0, NULL);
}

void __post_start_lan(void)
{
	char br_if[IFNAMSIZ];

	strlcpy(br_if, nvram_get("lan_ifname")? : nvram_default_get("lan_ifname"), sizeof(br_if));
	set_netdev_sysfs_param(br_if, "bridge/multicast_querier", "1");
	set_netdev_sysfs_param(br_if, "bridge/multicast_snooping",
		nvram_match("switch_br0_no_snooping", "1")? "0" : "1");
}

void __post_start_lan_wl(void)
{
	__post_start_lan();
}

int __sw_based_iptv(void)
{
	/* IPQ53XX always use software bridge to implement IPTV feature.
	 */
	return 1;
}

/**
 * for compatible with ipq53xx_qca8386.c mechanism
 */
int __sw_bridge_iptv_different_switches(void)
{
	return 1;
}

/* Return wan_base_if for start_vlan() and selectable upstream port for IPTV.
 * @wan_base_if:	pointer to buffer, minimal length is IFNAMSIZ.
 * @return:		pointer to base interface name for start_vlan().
 */
char *__get_wan_base_if(char *wan_base_if)
{
	int unit, wanif_type;

	if (!wan_base_if)
		return NULL;

	/* Select upstream port of IPTV profile based on configuration at run-time. */
	*wan_base_if = '\0';
	for (unit = WAN_UNIT_FIRST; *wan_base_if == '\0' && unit < WAN_UNIT_MAX; ++unit) {
		wanif_type = get_dualwan_by_unit(unit);
		if (!upstream_iptv_ifaces[wanif_type] || *upstream_iptv_ifaces[wanif_type] == '\0')
			continue;

		strlcpy(wan_base_if, upstream_iptv_ifaces[wanif_type], IFNAMSIZ);
	}

	return wan_base_if;
}

/* enum ecm_front_end_type, e.g., ECM_FRONT_END_TYPE_PPE_SFE. */
static const char *g_ecm_fe_type[] = { "Auto", "NSS", "SFE", "PPE", "NSS+SFE", "PPE+SFE" };

/* Return name of @accel_type
 * @return:	Pointer to a string or empty string.
 */
const char *__nat_accel_type_name(int accel_type)
{

	if (accel_type < 0 || accel_type >= ARRAY_SIZE(g_ecm_fe_type))
		return "";

	return g_ecm_fe_type[accel_type];
}

/* Return NAT acceleration type
 * @return:
 *  >= 0	Platform-specific acceleration type
 *    -1	No NAT acceleration.
 */
int __nat_accel_type(const char *kmod_name)
{
	int accel_type = -1, qca_hwnat_type;

	if (!kmod_name || strcmp(kmod_name, "ecm"))
		return -1;

	qca_hwnat_type = nvram_get_int("qca_hwnat_type");
	if (qca_hwnat_type < 0
	 || qca_hwnat_type >= ARRAY_SIZE(g_ecm_fe_type))
		qca_hwnat_type = 0;	/* Auto */
	if (strstr(g_ecm_fe_type[qca_hwnat_type], "NSS"))
		qca_hwnat_type = 0;	/* Auto */

	if (!qca_hwnat_type && is_tcode_country("JP"))
		accel_type = 2;		/* SFE-only */
	else
		accel_type = qca_hwnat_type;

	return accel_type;
}


/* Return current NAT acceleration type
 * @return:
 *  >= 0	Platform-specific acceleration type
 *    -1	No NAT acceleration.
 */
int __cur_nat_accel_type(const char *kmod_name)
{
	int cur_accel_type = -1;
	char val_str[10] = "";

	if (!kmod_name || strcmp(kmod_name, "ecm"))
		return -1;

	if (!f_exists(FRONT_END_SELECTION_ATTR)) {
		dbg("%s: %s doesn't exist\n", __func__,
			FRONT_END_SELECTION_ATTR);
		return -1;
	}
	f_read_string(FRONT_END_SELECTION_ATTR, val_str, sizeof(val_str));
	if (!strlen(val_str))
		return -1;

	cur_accel_type = safe_atoi(val_str);
	if (cur_accel_type < 0
	 || cur_accel_type >= ARRAY_SIZE(g_ecm_fe_type))
		return -1;

	return cur_accel_type;
}

#if defined(RTCONFIG_BONDING_WAN) || defined(RTCONFIG_LACP)
/** Convert bs_port_id to interface name.
 * @bs_port:	enum bs_port_id
 * @return:	pointer to interface name or NULL.
 *  NULL:	@bs_port doesn't have interface name or error.
 *  otherwise:	interface name.
 */
const char *bs_port_id_to_iface(enum bs_port_id bs_port)
{
	int vport;

	if (bs_port < 0 || bs_port >= ARRAY_SIZE(bsport_to_vport))
		return NULL;

	vport = bsport_to_vport[bs_port];
	return vport_to_iface_name(vport);
}
#endif

/* Return wan unit of upstream port for IPTV.
 * This function must same wan unit that returned by get_wan_base_if()!
 * @return:	wan unit of upstream port of IPTV.
 */
int __get_upstream_wan_unit(void)
{
	int i, wanif_type, unit = -1;

	for (i = WAN_UNIT_FIRST; unit < 0 && i < WAN_UNIT_MAX; ++i) {
		wanif_type = get_dualwan_by_unit(i);
		if (wanif_type != WANS_DUALWAN_IF_WAN &&
		    wanif_type != WANS_DUALWAN_IF_WAN2)
			continue;

		unit = i;
	}

	return unit;
}

void set_jumbo_frame(void)
{
	unsigned int mtu = 1500;
	char ifname[IFNAMSIZ], mtu_iface[sizeof("9000XXX")], mtu_frame[sizeof("9000XX")];
	char *ifconfig_argv[] = { "ifconfig", ifname, "mtu", mtu_iface, NULL };
	char *qca8386_framesize[] = { "ssdk_sh", SWID_QCA8386, "misc", "frameMaxSize", "set", mtu_frame, NULL };

	if (!nvram_contains_word("rc_support", "switchctrl"))
		return;

	if (nvram_get_int("jumbo_frame_enable"))
		mtu = 9000;

	snprintf(mtu_iface, sizeof(mtu_iface), "%d", mtu);
	snprintf(mtu_frame, sizeof(mtu_frame), "%d", mtu + 18);

	/* Enable jumbo frame of QCA8386 switch. */
	_eval(qca8386_framesize, DBGOUT, 0, NULL);
	strlcpy(ifname, QCA8386_IFACE, sizeof(ifname));
	_eval(ifconfig_argv, DBGOUT, 0, NULL);
}

/* Platform-specific function of wgn_sysdep_swtich_unset()
 * Unconfigure VLAN settings that is used to connect AiMesh guest network.
 * @vid:	VLAN ID
 */
void __wgn_sysdep_swtich_unset(int vid)
{
	char vid_str[6];
	char *delete_vlan[] = { "ssdk_sh", SWID_QCA8386, "vlan", "entry", "del", vid_str, NULL };

	snprintf(vid_str, sizeof(vid_str), "%d", vid);
	_eval(delete_vlan, DBGOUT, 0, NULL);
}

/* Platform-specific function of wgn_sysdep_swtich_set()
 * Unconfigure VLAN settings that is used to connect AiMesh guest network.
 * @vid:	VLAN ID
 */
void __wgn_sysdep_swtich_set(int vid)
{
	int i;
	int wans_lanport = strstr(nvram_safe_get("wans_dualwan"), "lan") ? nvram_get_int("wans_lanport") : -1;
	char port_str[4], vid_str[6];
	char *create_vlan[] = { "ssdk_sh", SWID_QCA8386, "vlan", "entry", "create", vid_str, NULL };
	char *vmbr_add[] = { "ssdk_sh", SWID_QCA8386, "vlan", "member", "add", vid_str, port_str, "unmodified", NULL };

	snprintf(vid_str, sizeof(vid_str), "%d", vid);
	_eval(create_vlan, DBGOUT, 0, NULL);
	for (i = 0; i <= 6; ++i) {
		if (i == wans_lanport)
			continue;

		snprintf(port_str, sizeof(port_str), "%d", i);
		_eval(vmbr_add, DBGOUT, 0, NULL);
	}
}

#ifdef RTCONFIG_NEW_PHYMAP
extern int get_trunk_port_mapping(int trunk_port_value)
{
	return trunk_port_value;
}

/* phy port related start */
void get_phy_port_mapping(phy_port_mapping *port_mapping)
{
	int i __attribute__((unused));
	static phy_port_mapping port_mapping_static = {
#if defined(TUFBE6500) || defined(TUFBE9400)
		.count = 4,
		.port[0] = { .phy_port_id = WAN_PORT,  .ext_port_id = WAN_PORT,  .label_name = "W0", .cap = PHY_PORT_CAP_WAN, .max_rate = 2500, .ifname = "eth0", .flag = 0, .ui_display = "2.5G WAN" },
		.port[1] = { .phy_port_id = LAN1_PORT, .ext_port_id = LAN1_PORT, .label_name = "L1", .cap = PHY_PORT_CAP_LAN | PHY_PORT_CAP_GAME, .max_rate = 2500, .ifname = "eth1", .flag = 0, .ui_display = "2.5G LAN1" },
		.port[2] = { .phy_port_id = LAN2_PORT, .ext_port_id = LAN2_PORT, .label_name = "L2", .cap = PHY_PORT_CAP_LAN, .max_rate = 2500, .ifname = "eth1", .flag = 0, .ui_display = "2.5G LAN2" },
		.port[3] = { .phy_port_id = LAN3_PORT, .ext_port_id = LAN3_PORT, .label_name = "L3", .cap = PHY_PORT_CAP_LAN, .max_rate = 2500, .ifname = "eth1", .flag = 0, .ui_display = "2.5G LAN3" },
#elif defined(RTCONFIG_PORT2_DEVICE)
		.count = 2,
		.port[0] = { .phy_port_id = WAN_PORT,  .ext_port_id = WAN_PORT,  .label_name = "W0", .cap = PHY_PORT_CAP_WAN | PHY_PORT_CAP_WANLAN | PHY_PORT_CAP_WANAUTO, .max_rate = 2500, .ifname = "eth0", .flag = 0, .seq_no = -1, .ui_display = "2.5G 2" },
		.port[1] = { .phy_port_id = LAN1_PORT, .ext_port_id = LAN1_PORT, .label_name = "L1", .cap = PHY_PORT_CAP_LAN | PHY_PORT_CAP_WANLAN | PHY_PORT_CAP_WANAUTO, .max_rate = 2500, .ifname = "eth1", .flag = 0, .seq_no = -1, .ui_display = "2.5G 1" },
#elif defined(BD4_OD)
		.count = 2,
		.port[0] = { .phy_port_id = WAN_PORT,  .ext_port_id = WAN_PORT,  .label_name = "W0", .cap = PHY_PORT_CAP_WAN | PHY_PORT_CAP_WANLAN | PHY_PORT_CAP_WANAUTO, .max_rate = 2500, .ifname = "eth0", .flag = 0, .seq_no = -1, .ui_display = "2.5G 1" },
		.port[1] = { .phy_port_id = LAN1_PORT, .ext_port_id = LAN1_PORT, .label_name = "L1", .cap = PHY_PORT_CAP_LAN | PHY_PORT_CAP_WANLAN | PHY_PORT_CAP_WANAUTO, .max_rate = 2500, .ifname = "eth1", .flag = 0, .seq_no = -1, .ui_display = "2.5G 2" },
#elif defined(RTBE50) || defined(RT4GBE58)
		.count = 4,
		.port[0] = { .phy_port_id = WAN_PORT,  .ext_port_id = WAN_PORT,  .label_name = "W0", .cap = PHY_PORT_CAP_WAN, .max_rate = 2500, .ifname = "eth0", .flag = 0, .ui_display = "2.5G WAN" },
		.port[1] = { .phy_port_id = LAN1_PORT, .ext_port_id = LAN1_PORT, .label_name = "L1", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = "eth1", .flag = 0, .ui_display = "LAN1" },
		.port[2] = { .phy_port_id = LAN2_PORT, .ext_port_id = LAN2_PORT, .label_name = "L2", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = "eth1", .flag = 0, .ui_display = "LAN2" },
		.port[3] = { .phy_port_id = LAN3_PORT, .ext_port_id = LAN3_PORT, .label_name = "L3", .cap = PHY_PORT_CAP_LAN, .max_rate = 1000, .ifname = "eth1", .flag = 0, .ui_display = "LAN3" },
#else /* QCA MI01.2 reference board */
#warning FIXME
		.count = 5,
		.port[0] = { .phy_port_id = WAN_PORT,  .ext_port_id = WAN_PORT,  .label_name = "W0", .cap = PHY_PORT_CAP_WAN, .max_rate = 2500, .ifname = "eth0", .flag = 0 },
		.port[1] = { .phy_port_id = LAN1_PORT, .ext_port_id = LAN1_PORT, .label_name = "L1", .cap = PHY_PORT_CAP_LAN, .max_rate = 2500, .ifname = "eth1", .flag = 0 },
		.port[2] = { .phy_port_id = LAN2_PORT, .ext_port_id = LAN2_PORT, .label_name = "L2", .cap = PHY_PORT_CAP_LAN, .max_rate = 2500, .ifname = "eth1", .flag = 0 },
		.port[3] = { .phy_port_id = LAN3_PORT, .ext_port_id = LAN3_PORT, .label_name = "L3", .cap = PHY_PORT_CAP_LAN, .max_rate = 2500, .ifname = "eth1", .flag = 0 },
		.port[4] = { .phy_port_id = LAN4_PORT, .ext_port_id = LAN4_PORT, .label_name = "L4", .cap = PHY_PORT_CAP_LAN, .max_rate = 2500, .ifname = "eth1", .flag = 0 },
#endif
	};

	if (!port_mapping)
		return;

	memcpy(port_mapping, &port_mapping_static, sizeof(phy_port_mapping));

///////////////// Add USB port define here ////////////////////////
#if defined(TUFBE6500) || defined(TUFBE9400)
////  1 USB3 port device
	i = port_mapping->count++;
	port_mapping->port[i].phy_port_id = -1;
	port_mapping->port[i].label_name = "U1";
	port_mapping->port[i].cap = PHY_PORT_CAP_USB;
	port_mapping->port[i].max_rate = 5000;
	port_mapping->port[i].ifname = NULL;
#endif

	add_sw_cap(port_mapping);
	swap_wanlan(port_mapping);
	return;
}
#endif /* RTCONFIG_NEW_PHYMAP */

#if defined(RTCONFIG_FRS_FEEDBACK)
static const struct reg_bit_defs_s {
	int sbit;	/* lowest start bit */
	int blen;	/* bit-length of the field */
	char *description;
} g_8386_reg_4[] = {
	{ 30,  1, "RMII master" },
	{ 29,  1, "RMII slave" },
	{ 28,  1, "Inverse RMII clock" },
	{ 27,  1, "RMII clock edge" },
	{ 26,  1, "RGMII EN" },
	{ 25,  1, "MAC0 RGMII TXCLK delay EN" },
	{ 22,  2, "MAC0 RGMII TXCLK delay" },
	{ 20,  2, "MAC0 RGMII RXCLK delay" },
	{ 19,  1, "SGMII CLK125M RX edge" },
	{ 18,  1, "SGMII CLK125M TX edge" },
	{ 17,  1, "FX100_EN" },
	{ 14,  1, "MAC0_PHY_GMII_EN" },
	{ 13,  1, "MAC0_PHY_GMII_TXCLK inv." },
	{ 12,  1, "MAC0_PHY_GMII_RXCLK inv." },
	{ 11,  1, "MAC0_PHY_MII_PIPE_RXCLK inv." },
	{ 10,  1, "MAC0_PHY_MII_EN" },
	{  9,  1, "MAC0_PHY_MII_TXCLK inv." },
	{  8,  1, "MAC0_PHY_MII_RXCLK inv." },
	{  7,  1, "MAC0_SGMII_EN" },
	{  6,  1, "MAC0_GMII_EN" },
	{  5,  1, "MAC0_MAC_TXCLK inv." },
	{  4,  1, "MAC0_MAC_RXCLK inv." },
	{  2,  1, "MAC0_MAC_MII_EN" },
	{  1,  1, "MAC0_MAC_MII_TXCLK inv." },
	{  0,  1, "MAc0_MAC_MII_RXCLK inv." },
	{ -1, -1, NULL }
}, g_8386_reg_8[] = {
	{ 26,  1, "MAC5_RGMII_EN" },
	{ 25,  1, "MAC5_RGMII_TXCLK_DELAY_EN" },
	{ 24,  1, "MAC5_RGMII_RXCLK_DELAY_EN" },
	{ 22,  2, "MAC5_RGMII_TXCLK_DELAY" },
	{ 20,  2, "MAC5_RGMII_RXCLK_DELAY" },
	{ 11,  1, "MAC5_PHY_MII_PIPE_RXCLK edge" },
	{ 10,  1, "MAC5_PHY_MII_EN" },
	{  9,  1, "MAC5_PHY_MII_TXCLK inv." },
	{  8,  1, "MAC5_PHY_MII_RXCLK inv." },
	{  2,  1, "MAC5_MAC_MII_EN" },
	{  1,  1, "MAC5_MAC_MII_TXCLK inv." },
	{  0,  1, "MAC5_MAC_MII_TXCLK inv." },
	{ -1, -1, NULL }
}, g_8386_reg_c[] = {
	{ 26,  1, "MAC6_RGMII_EN" },
	{ 25,  1, "MAC6_RGMII_TXCLK_DELAY_EN" },
	{ 22,  2, "MAC6_RGMII_TXCLK_DELAY" },
	{ 20,  2, "MAC6_RGMII_RXCLK_DELAY" },
	{ 17,  1, "PHY4_RGMII_EN edge" },
	{ 11,  1, "MAC6_PHY_MII_PIPE_RXCLK edge" },
	{ 10,  1, "MAC6_PHY_MII_EN" },
	{  9,  1, "MAC6_PHY_MII_TXCLK inv." },
	{  8,  1, "MAC6_PHY_MII_RXCLK inv." },
	{  7,  1, "MAC6_SGMII_EN" },
	{  2,  1, "MAC6_MAC_MII_EN" },
	{  1,  1, "MAC6_MAC_MII_TXCLK inv." },
	{  0,  1, "MAC6_MAC_MII_TXCLK inv." },
	{ -1, -1, NULL }
}, g_8386_reg_10[] = {
	{  31, 1, "Power-ON strap" },
	{  28, 1, "PACKAGEMIN_EN" },
	{  27, 1, "INPUT_MODE" },
	{  25, 1, "SPI_EEPROM_EN" },
	{  24, 1, "LED pad mode" },
	{  17, 1, "Hibernate" },
	{   7, 1, "SerDes autoneg EN" },
	{ -1, -1, NULL }
}, g_8386_reg_28[] = {
	{ 29,  1, "ACL" },
	{ 28,  1, "LOOKUP" },
	{ 27,  1, "QM" },
	{ 26,  1, "MIB" },
	{ 25,  1, "OFFLOAD" },
	{ 24,  1, "HARDWARE INT DONE EN" },
	{ 23,  1, "ACL MATCH" },
	{ 22,  1, "ARL DONE" },
	{ 21,  1, "ARL CPU FULL" },
	{ 20,  1, "VT DONE" },
	{ 19,  1, "MIB DONE" },
	{ 18,  1, "ACL DONE" },
	{ 17,  1, "OFFLOAD DONE" },
	{ 16,  1, "OFFLOAD CPU FULL DONE" },
	{ 11,  1, "ARL LEARN CREATE" },
	{ 10,  1, "ARL LEARN CHANGE" },
	{  9,  1, "ARL DELETE" },
	{  8,  1, "ARL LEARN FULL" },
	{  5,  1, "ARP LEARN CREATE" },
	{  4,  1, "ARP LEARN CHANGE" },
	{  3,  1, "ARP AGE DELETE" },
	{  2,  1, "ARP LEARN FULL" },
	{  1,  1, "VT MISS VIO" },
	{  0,  1, "VT MEM VIO" },
	{ -1, -1, NULL }
}, g_8386_reg_2c[] = {
	{ 19,  1, "THERM" },
	{ 18,  1, "EEPROM ERR" },
	{ 17,  1, "EEPROM" },
	{ 16,  1, "MDIO DONE" },
	{ 15,  1, "PHY" },
	{ 14,  1, "QM ERR" },
	{ 13,  1, "LOOKUP ERR" },
	{ 12,  1, "LOOP CHECK" },
	{  7,  7, "LINK_CHG" },
	{  0,  1, "BIST_DONE" },
	{ -1, -1, NULL }
}, g_8386_reg_30[] = {
	{ 10,  1, "SPECIAL DIP EN" },
	{  1,  1, "ACL_EN" },
	{  0,  1, "MIB_EN" },
	{ -1, -1, NULL }
}, g_8386_reg_38[] = {
	{ 24,  4, "RELOAD_TIMER" },
	{ 19,  1, "SGMII_CLK125M_RX inv." },
	{ 18,  1, "SGMII_CLK125M_TX inv." },
	{ -1, -1, NULL }
}, g_8386_reg_3c[] = {
	{ 26,  1, "MDIO_SUP_PRE" },
	{ 16,  5, "REG_ADDR" },
	{  0, 16, "MDIO_DATA" },
	{ -1, -1, NULL }
}, g_8386_reg_40[] = {
	{ 31,  1, "BIST_BUSY" },
	{ 29,  1, "BIST_PASS" },
	{ 23,  1, "BIST_CRITICAL" },
	{ 22,  1, "BIST_PTN_EN_2" },
	{ 21,  1, "BIST_PTN_EN_1" },
	{ 20,  1, "BIST_PTN_EN_0" },
	{ -1, -1, NULL }
}, g_8386_reg_7c[] = {
	{ 12,  1, "FLOW_LINK_EN" },
	{ 11,  1, "AUTO RX FLOW EN" },
	{ 10,  1, "AUTO TX FLOW EN" },
	{  9,  1, "LINK_EN" },
	{  8,  1, "LINK" },
	{  7,  1, "TX_HALF_FLOW_EN" },
	{  6,  1, "DUPLEX" },
	{  5,  1, "RX_FLOW_EN" },
	{  4,  1, "TX_FLOW_EN" },
	{  3,  1, "RXMAC_EN" },
	{  2,  1, "TXMAC_EN" },
	{  0,  2, "SPEED" },
	{ -1, -1, NULL }
}, g_8386_reg_100[] = {
	{ 12,  1, "LPI_EN_5" },
	{ 10,  1, "LPI_EN_4" },
	{  8,  1, "LPI_EN_3" },
	{  6,  1, "LPI_EN_2" },
	{  4,  1, "LPI_EN_1" },
	{  3,  1, "EEE_CPU_CHANGE_EN" },
	{  2,  1, "EEE_LLDP_TO_CPU_EN" },
	{  1,  1, "EEE_EN" },
	{ -1, -1, NULL }
}, g_8386_reg_400[] = {
	{ 31,  1, "ACL_BUSY" },
	{  8,  2, "ACL_RULE_SEL" },
	{  0,  7, "ACL rule index" },
	{ -1, -1, NULL }
}, g_8386_reg_420[] = {
	{ 29,  1, "CVLAN priority" },
	{ 16, 12, "Default CVID" },
	{ 13,  3, "SVLAN priority" },
	{  0, 12, "Default SVID" },
	{ -1, -1, NULL }
}, g_8386_reg_424[] = {
	{ 14,  1, "EG_VLAN_TYPE" },
	{ 12,  2, "EG_VLAN_MODE" },
	{ 10,  1, "L3 src port check" },
	{  9,  1, "CORE_PORT_EN" },
	{  8,  1, "FORCE_DEF_VID_EN" },
	{  7,  1, "PORT_TLS_MODE" },
	{  6,  1, "PORT_VLAN_PROP_EN" },
	{  5,  1, "PORT_CLONE_EN" },
	{  4,  1, "VLAN_PRI_PRO_EN" },
	{  2,  2, "ING_VLAN_MODE" },
	{ -1, -1, NULL }
}, g_8386_reg_660[] = {
	{ 31,  1, "MCAST_DROP_EN" },
	{ 28,  1, "UNI_LEAKY_EN" },
	{ 27,  1, "MULTI_LEAKY_EN" },
	{ 26,  1, "ARP_LEAKY_EN" },
	{ 25,  1, "NG_MIRROR_EN" },
	{ 21,  1, "PORT_LOOPBACK_EN" },
	{ 20,  1, "LEARN_EN" },
	{ 16,  3, "PORT_STATE" },
	{ 10,  1, "FORCE_PORT_VLAN_EN" },
	{  8,  2, "VLAN_MODE" },
	{  0,  7, "PORT_VID_MEM" },
	{ -1, -1, NULL }
};

static const struct qca8386_regs_def_s {
	int offset;	/* aligned to 4-bytes */
	char *name;
	const struct reg_bit_defs_s *bdef;
} g_qca8386_regs_def[] = {
	/* 0.Global control registers */
	{    4, "PORT0_PAD_CTRL", g_8386_reg_4 },
	{    8, "PORT5_PAD_CTRL", g_8386_reg_8 },
	{  0xC, "PORT6_PAD_CTLR", g_8386_reg_c },
	{ 0x10, "PWS_REG", g_8386_reg_10 },
	{ 0x28, "GLOBAL_INT0_MASK", g_8386_reg_28 },
	{ 0x2C, "GLOBAL_INT1_MASK", g_8386_reg_2c },
	{ 0x30, "MODULE_EN", g_8386_reg_30 },
	{ 0x38, "INTERFACE_HIGH_ADDR", g_8386_reg_38 },
	{ 0x3C, "MDIO master control", g_8386_reg_3c },
	{ 0x40, "BIST_CTRL", g_8386_reg_40 },
	{ 0x7C, "PORT0_STATUS", g_8386_reg_7c },
	{ 0x80, "PORT1_STATUS", g_8386_reg_7c },	/* same as PORT0_STATUS */
	{ 0x84, "PORT2_STATUS", g_8386_reg_7c },	/* same as PORT0_STATUS */
	{ 0x88, "PORT3_STATUS", g_8386_reg_7c },	/* same as PORT0_STATUS */
	{ 0x8C, "PORT4_STATUS", g_8386_reg_7c },	/* same as PORT0_STATUS */
	{ 0x90, "PORT5_STATUS", g_8386_reg_7c },	/* same as PORT0_STATUS */
	{ 0x94, "PORT6_STATUS", g_8386_reg_7c },	/* same as PORT0_STATUS */

	/* 1.EEE control registers */
	{ 0x100, "EEE_CTRL", g_8386_reg_100 },

	/* 2.Parser control registers */

	/* 3.ACL control registers */
	{ 0x400, "ACL_FUNC0", g_8386_reg_400 },
	{ 0x420, "PORT0_VLAN_CTRL0", g_8386_reg_420 },
	{ 0x424, "PORT0_VLAN_CTRL1", g_8386_reg_424 },
	{ 0x428, "PORT1_VLAN_CTRL0", g_8386_reg_420 },	/* same as PORT0_VLAN_CTRL0 */
	{ 0x42C, "PORT1_VLAN_CTRL1", g_8386_reg_424 },	/* same as PORT0_VLAN_CTRL1 */
	{ 0x430, "PORT2_VLAN_CTRL0", g_8386_reg_420 },	/* same as PORT0_VLAN_CTRL0 */
	{ 0x434, "PORT2_VLAN_CTRL1", g_8386_reg_424 },	/* same as PORT0_VLAN_CTRL1 */
	{ 0x438, "PORT3_VLAN_CTRL0", g_8386_reg_420 },	/* same as PORT0_VLAN_CTRL0 */
	{ 0x43C, "PORT3_VLAN_CTRL1", g_8386_reg_424 },	/* same as PORT0_VLAN_CTRL1 */
	{ 0x440, "PORT4_VLAN_CTRL0", g_8386_reg_420 },	/* same as PORT0_VLAN_CTRL0 */
	{ 0x444, "PORT4_VLAN_CTRL1", g_8386_reg_424 },	/* same as PORT0_VLAN_CTRL1 */
	{ 0x448, "PORT5_VLAN_CTRL0", g_8386_reg_420 },	/* same as PORT0_VLAN_CTRL0 */
	{ 0x44C, "PORT5_VLAN_CTRL1", g_8386_reg_424 },	/* same as PORT0_VLAN_CTRL1 */
	{ 0x450, "PORT6_VLAN_CTRL0", g_8386_reg_420 },	/* same as PORT0_VLAN_CTRL0 */
	{ 0x454, "PORT7_VLAN_CTRL1", g_8386_reg_424 },	/* same as PORT0_VLAN_CTRL1 */

	/* 4.Lookup control registers */
	{ 0x660, "PORT0_LOOKUP_CTRL", g_8386_reg_660 },
	{ 0x66C, "PORT1_LOOKUP_CTRL", g_8386_reg_660 },	/* same as PORT0_LOOKUP_CTRL */
	{ 0x678, "PORT2_LOOKUP_CTRL", g_8386_reg_660 },	/* same as PORT0_LOOKUP_CTRL */
	{ 0x684, "PORT3_LOOKUP_CTRL", g_8386_reg_660 },	/* same as PORT0_LOOKUP_CTRL */
	{ 0x690, "PORT4_LOOKUP_CTRL", g_8386_reg_660 },	/* same as PORT0_LOOKUP_CTRL */
	{ 0x69C, "PORT5_LOOKUP_CTRL", g_8386_reg_660 },	/* same as PORT0_LOOKUP_CTRL */
	{ 0x6A8, "PORT6_LOOKUP_CTRL", g_8386_reg_660 },	/* same as PORT0_LOOKUP_CTRL */

	/* 5.QM control registers */

	/* 6.PKT edit control registers */

	{ -1, NULL, NULL }
};

/* Dump important QCA8386 switch registers. */
static int dump_qca8386_regs(FILE *fp)
{
	int i, j, rlen, r, found, s_offset;
	FILE *fp_res;
	unsigned int tmp[8], reg[240], *p, m, v;
	char line[128], cmd[sizeof("ssdk_sh " SWID_QCA8386 " debug reg dump 0XXX")];
	const struct reg_bit_defs_s *b;
	const struct qca8386_regs_def_s *q;

	if (!fp)
		return -1;

	fprintf(fp, "\n\n######## Dump QCA8386 switch registers ########\n");
	/* Example: ssdk_sh sw1 debug reg dump 0
	 *
	 *  SSDK Init OK!
	 * [Register dump]
	 * 0.Global control registers.
	 *              0        4        8        c       10       14       18       1c
	 *  [0000] 00001302 05600000 01000000 00000080 00261320 f0107650 00004d86 00003f1f
	 *  [0020] 3f000800 00010000 00000000 00008000 80000401 00000000 0f000000 00000000
	 *  [0040] 00700000 00000000 000088a8 00000000 cc35cc35 0000007e c935c935 03ffff00
	 *  [0060] 00000001 00000000 00000000 00000000 b00ee060 03707f07 000005ee 0000004e
	 *  [0080] 000010c2 000010c2 000010c2 00001ffe 000010c2 00000200 00000000 00000000
	 *  [00a0] 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	 *  [00c0] 00000000 00000000 80901040 00000000 fffbff7e 00000001 00000100 000303ff
	 *  [00e0] c78164de 000aa545
	 *
	 *
	 *
	 * operation done.
	 */
	for (i = 0; i <= 6; ++i) {
		snprintf(cmd, sizeof(cmd), "ssdk_sh %s debug reg dump %d", SWID_QCA8386, i);
		if (!(fp_res = popen(cmd, "r"))) {
			fprintf(fp, "cmd [%s] failed. errno %d (%s)\n", cmd, errno, strerror(errno));
			continue;
		}
		p = &reg[0];
		rlen = 0;
		found = s_offset = 0;
		while (fgets(line, sizeof(line), fp_res) != NULL) {
			fprintf(fp, "%s", line);
			if (strncmp(line, " [0", 3))
				continue;

			if (!found) {
				if (sscanf(line + 2, "%x", &s_offset) != 1) {
					fprintf(fp, "Looking up start offset failed\n");
				}
				found = 1;
			}

			r = sscanf(line + 8, "%x %x %x %x %x %x %x %x", tmp, tmp + 1,
				tmp + 2, tmp + 3, tmp + 4, tmp + 5, tmp + 6, tmp + 7);
			if (r > 0) {
				if ((rlen + r) * 4 > sizeof(reg)) {
					fprintf(fp, "Size of reg (%d) is not enough (%d)!\n", (int) sizeof(reg), (rlen + r) * 4);
					rlen = sizeof(reg) - rlen;
				}
				memcpy(p, tmp, r * 4);
				p += r;
				rlen += r;
			}

			if (r < 8)
				break;
		}
		fprintf(fp, "\n");

		/* Parse interesting registers. */
		for (q = &g_qca8386_regs_def[0]; q->offset >= 0 && q->name && q->bdef; ++q) {
			if (q->offset < s_offset || q->offset >= (s_offset + rlen * 4))
				continue;

			j = (q->offset - s_offset) >> 2;
			if (!*(reg + j))
				continue;
			fprintf(fp, "%20s(%04x): %08x", q->name, q->offset, *(reg + j));
			for (b = q->bdef; b->sbit >= 0 && b->blen > 0 && b->description; ++b) {
				if (b->sbit >= 32 || b->blen >= 32)
					break;
				m = (1U << b->blen) - 1;
				v = (*(reg + j) >> b->sbit) & m;
				if (!v)
					continue;
				fprintf(fp, ", %s %x", b->description, v);
			}
			fprintf(fp, "\n");
		}

		pclose(fp_res);
		fprintf(fp, "\n");
	}

	return 0;
}

/* Dump @fn content to @fp.
 * @fp:	FILE pointer
 * @fn: filename
 * @nl: print newline at end of file
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
static int __dump_file(FILE *fp, const char *fn, int nl)
{
	FILE *fp_res;
	char line[512];

	if (!fp || !fn)
		return -1;

	if (!(fp_res = fopen(fn, "r")))
		return -2;

	while (fgets(line, sizeof(line), fp_res) != NULL) {
		fprintf(fp, "%s", line);
	}
	if (nl)
		fprintf(fp, "\n");
	fclose(fp_res);

	return 0;
}

static inline int dump_file(FILE *fp, const char *fn) { return __dump_file(fp, fn, 0); }

/* Exec @cmd and dump output to @fp
 * @fp:	FILE pointer
 * @cmd: command
 * @return:
 * 	0:	success
 *  otherwise:	error
 */
static int exec_and_dump(FILE *fp, const char *cmd)
{
	FILE *fp_res;
	char line[512];

	if (!fp || !cmd || *cmd == '\0')
		return -1;

	if (!(fp_res = popen(cmd, "r")))
		return -2;

	while (fgets(line, sizeof(line), fp_res) != NULL) {
		fprintf(fp, "%s", line);
	}
	fprintf(fp, "\n");
	pclose(fp_res);

	return 0;
}

struct sw_port_name_s {
	const char *sw;
	int port;
	const char *name;
};

static struct sw_port_name_s sw_port_name[] = {
	{ SWID_IPQ53XX, 2, "WAN" },
	{ SWID_QCA8386, 3, "LAN1" },
	{ SWID_QCA8386, 2, "LAN2" },
	{ SWID_QCA8386, 1, "LAN3" },

	{ NULL, -1, NULL }
};

static const char *ppe_stats_fn[] = { "common_stats", "policer_stats", "acl_stats", "sawf_sc_stats", "sc_stats", NULL };
static const char *ppe_sfe_dir_tbl[] = { "ecm_ppe_ipv4", "ecm_ppe_ipv6", "ecm_sfe_ipv4", "ecm_sfe_ipv6", NULL };

struct dump_ppe_sfe_counter_s {
	const char *path;	/* /PATH/TO/ecm_ppe_ipv[46] or ecm_sfe_ipv[46] */
	const char *dir;	/* ecm_ppe_ipv[46] or ecm_sfe_ipv[46] */
	FILE *fp;
};

static int dump_ppe_sfe_counter(const char *path, const struct dirent *de, size_t de_size, void *arg)
{
	struct dump_ppe_sfe_counter_s *s = (struct dump_ppe_sfe_counter_s*) arg;
	char ppe_sfe_path[sizeof("/sys/kernel/debug/ecm/ecm_ppe_ipv6/non_ported_accelerated_countXXX")];

	if (!s || !s->path || !s->dir || !s->fp)
		return -1;

	if (sizeof(*de) != de_size) {
		/* If size of struct dirent mismatch, make sure readdir_wrapper() and this function see same struct dirent.h.
		 * e.g., it's different in uclibc if _FILE_OFFSET_BITS=64 is defined or not.
		 */
		dbg("%s: size of struct dirent mismatch (%zu v.s. %zu)!\n", __func__, sizeof(*de), de_size);
		return -1;
	}

	snprintf(ppe_sfe_path, sizeof(ppe_sfe_path), "%s/%s", s->path, de->d_name);
	fprintf(s->fp, "%s/%s=", s->dir, de->d_name);
	dump_file(s->fp,  ppe_sfe_path);
	return 0;
}

void __gen_switch_log(char *fn)
{
	const char **ppe_fn, **ppe_sfe_dir;
	const char *v4_stop_fn = "/sys/kernel/debug/ecm/front_end_ipv4_stop";
	const char *v6_stop_fn = "/sys/kernel/debug/ecm/front_end_ipv6_stop";
	char path[64], *ate_cmd[] = { "ATE", "Get_WanLanStatus", NULL };
	char fc_state[sizeof("DISABLEXXXXXX")], fc_cmd[sizeof("ssdk_sh sw0 port flowctrl get XYYY")];
	char ppe_stat_path[sizeof("/sys/kernel/debug/qca-nss-ppe/stats/policer_statsXXX")];
	struct sw_port_name_s *swp;
	FILE *fp;
	struct dump_ppe_sfe_counter_s arg;

	if (!fn || *fn == '\0')
		return;

	snprintf(path, sizeof(path), ">%s", fn);
	_eval(ate_cmd, path, 0, NULL);

	if (!(fp = fopen(fn, "a")))
		return;

	/* ssdk_sh sw0 port flowctrl get 6
	 *
	 *   SSDK Init OK![Flow control]:ENABLE
	 *  operation done.
	 */
	fprintf(fp, "\n\n######## Flow control ########\n");
	for (swp = &sw_port_name[0]; swp->sw != NULL; ++swp) {
		snprintf(fc_cmd, sizeof(fc_cmd), "ssdk_sh %s port flowctrl get %d", swp->sw, swp->port);
		strlcpy(fc_state, "READ FAIL", sizeof(fc_state));
		parse_ssdk_sh(fc_cmd, "%*[^:]:%13s", 1, fc_state);
		fprintf(fp, "%10s: %7s (%s port %d)\n", swp->name, fc_state, swp->sw, swp->port);
	}

	/* Check run-time status of ecm. */
	fprintf(fp, "\n\n######## ecm stopped? ########\n");
	fprintf(fp, "IPv4: ");
	dump_file(fp, v4_stop_fn);
	fprintf(fp, "IPv6: ");
	dump_file(fp, v6_stop_fn);

	/* NAT acceleration type. */
	fprintf(fp, "\n\n######## NAT acceleration type ########\n");
	dump_file(fp, FRONT_END_SELECTION_ATTR);

	/* Check ebtables* kernel modules is loaded or not. */
	fprintf(fp, "\n\n######## ebtables status ########\n");
	exec_and_dump(fp, "lsmod|grep ebtable");

	/* QCA8376 registers */
	dump_qca8386_regs(fp);

	/* Check run-time status of ecm. */
	for (ppe_fn = &ppe_stats_fn[0]; *ppe_fn != NULL; ++ppe_fn) {
		snprintf(ppe_stat_path, sizeof(ppe_stat_path),
			"/sys/kernel/debug/qca-nss-ppe/stats/%s", *ppe_fn);
		if (!f_exists(ppe_stat_path))
			continue;
		fprintf(fp, "\n\n######## PPE %s ########\n", *ppe_fn);
		dump_file(fp, ppe_stat_path);
	}

	/* Check counter of ecm_ppe_ipv[46], ecm_sfe_ipv[46] */
	// readdir_wrapper
	arg.fp = fp;
	arg.path = ppe_stat_path;
	fprintf(fp, "\n\n######## PPE/SFE counter ########\n");
	for (ppe_sfe_dir = &ppe_sfe_dir_tbl[0]; *ppe_sfe_dir != NULL; ++ppe_sfe_dir) {
		snprintf(ppe_stat_path, sizeof(ppe_stat_path),
			"/sys/kernel/debug/ecm/%s", *ppe_sfe_dir);
		if (!d_exists(ppe_stat_path))
			continue;
		arg.dir = *ppe_sfe_dir;
		fprintf(fp, "________ %s ________\n", *ppe_sfe_dir);
		readdir_wrapper(ppe_stat_path, NULL, dump_ppe_sfe_counter, &arg);
	}

	fclose(fp);
}
#endif

#ifdef RTCONFIG_MULTILAN_CFG
void __apg_switch_vlan_set(int vid, unsigned int default_portmask, unsigned int trunk_portmask, unsigned int access_portmask)
{
	int i, port_id;
	unsigned int m;
	char vid_str[6], port_str[4], vmbr_type[12];
	char *ventry_create[] = { "ssdk_sh", SWID_QCA8386, "vlan", "entry", "create", vid_str, NULL };
	char *p0vmbr_add[] = { "ssdk_sh", SWID_QCA8386, "vlan", "member", "add", vid_str, QCA8386_CPUPORT, "unmodified", NULL };
	char *vmbr_add[] = { "ssdk_sh", SWID_QCA8386, "vlan", "member", "add", vid_str, port_str, vmbr_type, NULL };
	char *v1mbr_del[] = { "ssdk_sh", SWID_QCA8386, "vlan", "member", "del", "1", port_str, NULL };
	char *defcvid[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "defaultCVid", "set", port_str, vid_str, NULL };

	snprintf(vid_str, sizeof(vid_str), "%d", vid);
	_eval(ventry_create, DBGOUT, 0, NULL);
	_eval(p0vmbr_add, DBGOUT, 0, NULL);

	if (default_portmask) {
		snprintf(vmbr_type, sizeof(vmbr_type), "tagged");
		m = default_portmask;
		for (i = 0; i <= MAX_WANLAN_PORT; i++, m >>= 1) {
			if (m & 1) {
				port_id = *(vport_to_ssdkport + i);
				if (port_id < 0 || port_id >= 0x10) {
					dbg("%s: skip port_id 0x%x\n", __func__, port_id);
					continue;
				}
				snprintf(port_str, sizeof(port_str), "%d", port_id);
				_eval(vmbr_add, DBGOUT, 0, NULL);
			}
		}
	}

	if (trunk_portmask) {
		snprintf(vmbr_type, sizeof(vmbr_type), "tagged");
		m = trunk_portmask;
		for (i = 0; i <= MAX_WANLAN_PORT; i++, m >>= 1) {
			if (m & 1) {
				port_id = *(vport_to_ssdkport + i);
				if (port_id < 0 || port_id >= 0x10) {
					dbg("%s: skip port_id 0x%x\n", __func__, port_id);
					continue;
				}
				snprintf(port_str, sizeof(port_str), "%d", port_id);
				_eval(v1mbr_del, DBGOUT, 0, NULL);
				_eval(vmbr_add, DBGOUT, 0, NULL);
			}
		}
	}

	if (access_portmask) {
		snprintf(vmbr_type, sizeof(vmbr_type), "untagged");
		m = access_portmask;
		for (i = 0; i <= MAX_WANLAN_PORT; i++, m >>= 1) {
			if (m & 1) {
				port_id = *(vport_to_ssdkport + i);
				if (port_id < 0 || port_id >= 0x10) {
					dbg("%s: skip port_id 0x%x\n", __func__, port_id);
					continue;
				}
				snprintf(port_str, sizeof(port_str), "%d", port_id);
				_eval(v1mbr_del, DBGOUT, 0, NULL);
				_eval(vmbr_add, DBGOUT, 0, NULL);
				_eval(defcvid, DBGOUT, 0, NULL);
			}
		}
	}
}

void __apg_switch_vlan_unset(int vid, unsigned int portmask)
{
	int i, port_id;
	unsigned int m;
	char vid_str[6], port_str[4];
	char *ventry_del[] = { "ssdk_sh", SWID_QCA8386, "vlan", "entry", "del", vid_str, NULL };
	char *v1mbr_add[] = { "ssdk_sh", SWID_QCA8386, "vlan", "member", "add", "1", port_str, "untagged", NULL };
	char *def1cvid[] = { "ssdk_sh", SWID_QCA8386, "portVlan", "defaultCVid", "set", port_str, "1", NULL };

	snprintf(vid_str, sizeof(vid_str), "%d", vid);
	_eval(ventry_del, DBGOUT, 0, NULL);

	if (aimesh_re_node())
		return;

	if (portmask) {
		m = portmask;
		for (i = 0; i <= MAX_WANLAN_PORT; i++, m >>= 1) {
			if (m & 1) {
				port_id = *(vport_to_ssdkport + i);
				if (port_id < 0 || port_id >= 0x10) {
					dbg("%s: skip port_id 0x%x\n", __func__, port_id);
					continue;
				}
				snprintf(port_str, sizeof(port_str), "%d", port_id);
				_eval(v1mbr_add, DBGOUT, 0, NULL);
				_eval(def1cvid, DBGOUT, 0, NULL);
			}
		}
	}
}
#endif /* RTCONFIG_MULTILAN_CFG */
