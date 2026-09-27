/*
 * Copyright 2018, ASUSTeK Inc.
 * All Rights Reserved.
 *
 */

#include "rc.h"
#include <sys/stat.h>

/* Keep Merlin's openvpn-event argument order and OpenVPN's environment. */
void run_ovpn_event_script(void)
{
	char *script = "/jffs/scripts/openvpn-event";
	char *dev = safe_getenv("dev");
	char *mtu;
	struct stat st;

	if (!nvram_match("jffs2_scripts", "1") || stat(script, &st) != 0)
		return;
	if (!(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
		logmessage("custom_script", "openvpn-event is not executable");
		return;
	}
	if (!strncmp(dev, "tun", 3))
		mtu = safe_getenv("tun_mtu");
	else if (!strncmp(dev, "tap", 3))
		mtu = safe_getenv("tap_mtu");
	else
		return;

	logmessage("custom_script", "Running openvpn-event");
	eval(script, dev, mtu, safe_getenv("link_mtu"),
		safe_getenv("ifconfig_local"), safe_getenv("ifconfig_remote"),
		safe_getenv("script_context"));
}

int ovpn_up_main(int argc, char **argv)
{
	ovpn_up_handler();
	run_ovpn_event_script();

	update_resolvconf();

	return 0;
}

int ovpn_down_main(int argc, char **argv)
{
	ovpn_down_handler();
	run_ovpn_event_script();

	update_resolvconf();

	return 0;
}

int ovpn_route_up_main(int argc, char **argv)
{
	ovpn_route_up_handler();
	run_ovpn_event_script();

	return 0;
}

int ovpn_route_pre_down_main(int argc, char **argv)
{
	ovpn_route_pre_down_handler();
	run_ovpn_event_script();

	return 0;
}
