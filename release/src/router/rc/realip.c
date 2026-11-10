/*
 * Real-IP lookup synchronization and STUN service
 *
 * Copyright (C) 2009, Broadcom Corporation. All Rights Reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: services.c,v 1.100 2010/03/04 09:39:18 Exp $
 */

/*
 * Volatile real-IP lookup ownership shared by rc, getrealip.sh and realip.
 * All generation checks and NVRAM publication take the same short POSIX lock.
 * Never hold it during STUN, DNS, process waits or route changes.
 */
#include <rc.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>

#ifdef RTCONFIG_GETREALIP
static volatile sig_atomic_t realip_interrupted;

static int realip_lock(void)
{
	return file_lock("realip");
}

static void realip_reset(const char *prefix)
{
	unsigned long long generation;
	char buf[32];

	generation = strtoull(nvram_pf_safe_get(prefix, "realip_generation"), NULL, 10);
	/* Do not reuse an ownership token even at counter exhaustion. */
	if (generation != ULLONG_MAX)
		generation++;
	snprintf(buf, sizeof(buf), "%llu", generation);
	nvram_pf_set(prefix, "realip_state", "0");
	nvram_pf_set(prefix, "realip_context", "");
	nvram_pf_set(prefix, "realip_generation", buf);
	nvram_pf_set(prefix, "realip_ip", "");
}

/* Multi-WAN aliases and their underlying WAN share publication ownership.
 * Keep the original state_t owner, but revoke lookups on both prefixes.
 * Caller holds realip_lock.
 */
static void realip_connection_reset(const char *prefix, int blocked)
{
#ifdef RTCONFIG_MULTIWAN_IF
	char mapped[32], *end;
	long unit = !strncmp(prefix, "wan", 3) ? strtol(prefix + 3, &end, 10) : -1;
#endif
	realip_reset(prefix);
	nvram_pf_set(prefix, "realip_blocked", blocked ? "1" : "0");
#ifdef RTCONFIG_MULTIWAN_IF
	if (unit >= 0 && unit <= INT_MAX && !strcmp(end, "_") && is_mtwan_unit(unit) &&
	    mtwan_get_real_wan(unit, mapped, sizeof(mapped)) >= 0 && strcmp(prefix, mapped)) {
		realip_reset(mapped);
		nvram_pf_set(mapped, "realip_blocked", blocked ? "1" : "0");
	}
#endif
}

/* Called before tearing down an interface, including a secondary WAN in LB. */
void realip_invalidate(const char *prefix)
{
	int lock = realip_lock();
	if (lock < 0)
		return;
	realip_connection_reset(prefix, 1);
	file_unlock(lock);
}

/* A service STOP cancels requests without changing connection eligibility. */
void cancel_realip(const char *prefix)
{
	int lock = realip_lock();
	if (lock < 0)
		return;
	realip_reset(prefix);
	file_unlock(lock);
}

/* State transition and lookup invalidation are one transaction. A DHCP renew
 * also revokes pending lookups, even when the address/interface are unchanged.
 */
void realip_wan_state(const char *prefix, int state)
{
	int lock = realip_lock();
	if (lock < 0)
		return;
	nvram_pf_set_int(prefix, "state_t", state);
	realip_connection_reset(prefix, state != WAN_STATE_CONNECTED);
	file_unlock(lock);
}

static void realip_context(const char *prefix, char *buf, size_t len)
{
	const char *fields[] = { "proto", "ifname", "pppoe_ifname", "ipaddr", "gateway", "primary", NULL };
	int i;

	strlcpy(buf, nvram_safe_get("wans_mode"), len);
	for (i = 0; fields[i]; i++) {
		strlcat(buf, "/", len);
		strlcat(buf, nvram_pf_safe_get(prefix, fields[i]), len);
	}
}

static int realip_connected(const char *prefix)
{
	return nvram_pf_get_int(prefix, "state_t") == WAN_STATE_CONNECTED &&
	       !nvram_pf_get_int(prefix, "realip_blocked");
}

/* Caller holds the publication lock, including across both WANs in LB. */
static int realip_begin_locked(const char *prefix, char *token, size_t len)
{
	char context[256];

	if (!realip_connected(prefix) ||
	    strtoull(nvram_pf_safe_get(prefix, "realip_generation"), NULL, 10) == ULLONG_MAX)
		return 0;
	realip_reset(prefix);
	strlcpy(token, nvram_pf_safe_get(prefix, "realip_generation"), len);
	realip_context(prefix, context, sizeof(context));
	nvram_pf_set(prefix, "realip_context", context);
	return 1;
}

static int realip_begin(const char *prefix, char *token, size_t len)
{
	int ok, lock = realip_lock();
	if (lock < 0)
		return 0;
	ok = realip_begin_locked(prefix, token, len);
	file_unlock(lock);
	return ok;
}

static int realip_script_begin(void)
{
	int unit, lb, primary = -1, auxstate, lock = realip_lock();
	char prefix[16], tokens[2][32] = { "-", "-" };

	if (lock < 0)
		return 1;
	lb = nvram_match("wans_mode", "lb");
	if (nvram_match("wan0_primary", "1"))
		primary = 0;
	else if (nvram_match("wan1_primary", "1"))
		primary = 1;
	for (unit = 0; unit < 2; unit++) {
		if (!lb && unit != primary)
			continue;
		snprintf(prefix, sizeof(prefix), "wan%d_", unit);
		auxstate = nvram_pf_get_int(prefix, "auxstate_t");
		if (lb && (nvram_pf_get_int(prefix, "sbstate_t") != 0 ||
		           (auxstate != 0 && auxstate != 2)))
			continue;
		realip_begin_locked(prefix, tokens[unit], sizeof(tokens[unit]));
	}
	file_unlock(lock);
	if (!lb && !strcmp(tokens[0], "-") && !strcmp(tokens[1], "-"))
		return 1;
	printf("%d %s %s\n", lb, tokens[0], tokens[1]);
	return 0;
}

/* Must be called with realip_lock held. */
static int realip_matches(const char *prefix, const char *token)
{
	char context[256];

	realip_context(prefix, context, sizeof(context));
	return token[0] && realip_connected(prefix) &&
	       !strcmp(token, nvram_pf_safe_get(prefix, "realip_generation")) &&
	       !strcmp(context, nvram_pf_safe_get(prefix, "realip_context"));
}

static int realip_owned(const char *prefix, const char *token)
{
	return realip_matches(prefix, token) && nvram_pf_get_int(prefix, "realip_state") == 0;
}

static int realip_current(const char *prefix, const char *token)
{
	int ok, lock = realip_lock();
	if (lock < 0)
		return 0;
	ok = realip_owned(prefix, token);
	file_unlock(lock);
	return ok;
}

static int realip_valid(const char *ip)
{
	struct in_addr addr;
	unsigned long host;

	if (inet_pton(AF_INET, ip, &addr) != 1)
		return 0;
	host = ntohl(addr.s_addr);
	/* Unspecified, loopback, multicast and reserved/broadcast are not public IPs. */
	return (host >> 24) != 0 && (host >> 24) != 127 && (host >> 28) < 14;
}

static int realip_finish(const char *prefix, const char *token, const char *ip)
{
	int valid = realip_valid(ip);
	int ok, lock = realip_lock();
	if (lock < 0)
		return 0;
	ok = !realip_interrupted && realip_owned(prefix, token);
	/* A context change fails this request; it must not leave it pending.
	 * Only a pending token can publish, so completion cannot be replayed. */
	if (ok || (token[0] && !strcmp(token, nvram_pf_safe_get(prefix, "realip_generation")) &&
	           nvram_pf_get_int(prefix, "realip_state") == 0 &&
	           nvram_pf_safe_get(prefix, "realip_context")[0])) {
		valid = valid && ok;
		/* Readers must never observe ready with an old or invalid address. */
		nvram_pf_set(prefix, "realip_state", "0");
		nvram_pf_set(prefix, "realip_ip", valid ? ip : "");
		nvram_pf_set(prefix, "realip_state", valid ? "2" : "1");
	}
	file_unlock(lock);
	return ok;
}

/* inadyn ignores pclose() status, so never emit a partial LB result. Validate
 * every selected WAN under one lock and copy the report before releasing it.
 */
static int realip_report(const char *token0, const char *token1, int lb)
{
	const char *tokens[] = { token0, token1 };
	char prefix[16], line[96], report[192] = "";
	int unit, state, lock = realip_lock();

	if (lock < 0)
		return 1;
	for (unit = 0; unit < 2; unit++) {
		if (!tokens[unit][0])
			continue;
		snprintf(prefix, sizeof(prefix), "wan%d_", unit);
		state = nvram_pf_get_int(prefix, "realip_state");
		if (!realip_matches(prefix, tokens[unit]) || (state != 1 && state != 2)) {
			file_unlock(lock);
			return 1;
		}
		if (lb) {
			if (state == 2)
				snprintf(line, sizeof(line), "wan%d external IP is %s\n", unit, nvram_pf_safe_get(prefix, "realip_ip"));
			else
				snprintf(line, sizeof(line), "wan%d failed.\n", unit);
		} else {
			if (state == 2)
				snprintf(line, sizeof(line), "External IP is %s\n", nvram_pf_safe_get(prefix, "realip_ip"));
			else
				strlcpy(line, "Failed.\n", sizeof(line));
		}
		strlcat(report, line, sizeof(report));
	}
	file_unlock(lock);
	fputs(report, stdout);
	return 0;
}

/* Reserve before the asynchronous launch; a delayed child cannot register as
 * a newer request after another WAN event or a direct/inadyn invocation.
 */
void start_realip(int unit)
{
	char prefix[32], token[32], unitstr[16];
	char *args[] = { "getrealip.sh", unitstr, token, NULL, NULL };
	pid_t pid;

	if (unit < 0)
		return;
	snprintf(prefix, sizeof(prefix), "wan%d_", unit);
	if (unit > 1) {
		args[0] = "realip";
		args[3] = prefix;
	}
#ifdef RTCONFIG_MULTIWAN_IF
	args[0] = "realip";
	args[3] = prefix;
	if (is_mtwan_unit(unit) && mtwan_get_real_wan(unit, prefix, sizeof(prefix)) < 0)
		return;
#endif
	snprintf(unitstr, sizeof(unitstr), "%d", unit);
	if (realip_begin(prefix, token, sizeof(token)) &&
	    _eval(args, ">>/dev/null", 0, &pid) != 0)
		realip_finish(prefix, token, "");
}

static void realip_signal(int sig)
{
	realip_interrupted = sig;
}

/* Own and reap ministun, including when superseded or terminated. */
static int realip_exec(char **args, const char *output, const char *prefix, const char *token)
{
	pid_t pid, ret;
	int status;

	if (realip_interrupted || !realip_current(prefix, token) ||
	    _eval(args, output, 0, &pid) != 0)
		return -1;
	for (;;) {
		ret = waitpid(pid, &status, WNOHANG);
		if (ret == pid)
			return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
		if (ret < 0 && errno != EINTR)
			return -1;
		if (realip_interrupted || !realip_current(prefix, token)) {
			kill(pid, SIGKILL);
			while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
				;
			return -1;
		}
		usleep(100000);
	}
}

/* Script probes use the same child ownership as the routed C lookup. This
 * also cancels a superseded probe during DNS/STUN, rather than waiting for it.
 */
static int realip_probe(const char *prefix, const char *token, char *ifname, char *server)
{
	char result_path[] = "/tmp/stun.XXXXXX", output[64], ip[64];
	char *args[] = { "ministun", "-t", "1000", "-c", "1", "-i", ifname, server, NULL };
	FILE *fp;
	int fd, success = 0;

	if (!server[0])
		args[7] = NULL;
	fd = mkstemp(result_path);
	if (fd < 0)
		return 1;
	close(fd);
	snprintf(output, sizeof(output), ">%s", result_path);
	if (!realip_exec(args, output, prefix, token) && !realip_interrupted && realip_current(prefix, token)) {
		fp = fopen(result_path, "r");
		while (fp && fgets(ip, sizeof(ip), fp)) {
			trimNL(ip);
			if (realip_valid(ip)) {
				puts(ip);
				success = 1;
				break;
			}
		}
		if (fp)
			fclose(fp);
	}
	unlink(result_path);
	return success ? 0 : 1;
}

static int realip_lookup(int argc, char *argv[])
{
	char *stun_server_list[] = {
		"stun.l.google.com",
		"stun.stunprotocol.org",
		"stun4.l.google.com:19302",
		// "stun3.l.google.com:19302",
		// "stun2.l.google.com:19302",
		// "stun1.l.google.com:19302",
		"stun.pjsip.org",
		// "stun.linphone.org",
		// "stun.freeswitch.org",
		// "stun.develz.org",
		// "stun.wifirst.net",
		// "stun.sipnet.net",
		// "stun.sipgate.net",
		// "stun.rolmail.net",
		// "stun.nonoh.net",
		// "stun.nfon.net",
		// "stun.infra.net",
		// "stun.dus.net",
		NULL};
	int i = 0;
	int wan_unit, requested_unit;
	char wan_prefix[16] = {0};
	char wan_ifname[IFNAMSIZ] = {0};
	char wan_gateway[INET_ADDRSTRLEN] = {0};
	char stun_server[64] = {0};
	char *stun_argv[] = {"ministun", "-t", "1000", "-c", "1"
		, "-i", wan_ifname, stun_server
		, NULL};
	char buf[64] = {0};
	char *server, *tmp_server, *port;
	char stun_server_ip[INET_ADDRSTRLEN] = {0};
	char table_str[8] = "main";
#if defined(RTCONFIG_MULTIWAN_IF) || defined(RTCONFIG_DUALWAN)
	char pref_str[8] = "30";
#endif
#ifdef RTCONFIG_MULTIWAN_IF
	int is_mtwan;
#endif
#ifdef RTCONFIG_DUALWAN
	int load_balance = (nvram_match("wans_mode", "lb") && get_nr_wan_unit() > 1) ? 1 : 0;
#endif
	int ret;
	int success = 0;
	int route_lock = -1, fd;
	char token[32];
	char result_path[] = "/tmp/stun.XXXXXX";
	char output[64];

	if (argc < 2) {
		printf("Usage: realip <wan_unit>\n");
		return EXIT_FAILURE;
	}

	{
		char *end;
		long value = strtol(argv[1], &end, 10);
		if (!argv[1][0] || *end || value < 0 || value > INT_MAX)
			return -1;
		wan_unit = requested_unit = value;
	}
	snprintf(wan_prefix, sizeof(wan_prefix), "wan%d_", wan_unit);
#ifdef RTCONFIG_MULTIWAN_IF
	is_mtwan = is_mtwan_unit(wan_unit);
	if (is_mtwan) {
		wan_unit = mtwan_get_real_wan(wan_unit, wan_prefix, sizeof(wan_prefix));
		if (wan_unit < 0) {
			printf("Get WAN(%d) dualwan_idx failed\n", safe_atoi(argv[1]));
			return -10;
		}
		mtwan_get_route_table_id(wan_unit, table_str, sizeof(table_str));
	}
#endif
#ifdef RTCONFIG_DUALWAN
	if (load_balance && wan_unit < WAN_UNIT_MAX)
		snprintf(table_str, sizeof(table_str), "%d", IP_ROUTE_TABLE_ID_DUALWAN_BASE + wan_unit * 100);
#endif
	if (nvram_pf_get_int(wan_prefix, "state_t") != WAN_STATE_CONNECTED) {
		printf("WAN(%d) is not connected\n", wan_unit);
		return -1;
	}

	if (argc == 4) {
		/* Numeric tokens are per-WAN: bind delayed launches to the exact
		 * prefix reserved by rc, even if a Multi-WAN alias was remapped. */
		if (strcmp(wan_prefix, argv[3]))
			return -1;
		strlcpy(token, argv[2], sizeof(token));
		if (!realip_current(wan_prefix, token))
			return -1;
	} else if (!realip_begin(wan_prefix, token, sizeof(token)))
		return -1;
	while (nvram_get_int("link_internet") != 2) {
		if (realip_interrupted || !realip_current(wan_prefix, token))
			goto done;
		sleep(1);
	}
	/* Legacy realip installs temporary routes. Serialize those separately;
	 * the short publication lock remains available to WAN transitions.
	 */
	route_lock = file_lock("realip_route");
	if (route_lock < 0 || realip_interrupted || !realip_current(wan_prefix, token))
		goto done;
	fd = mkstemp(result_path);
	if (fd < 0)
		goto done;
	close(fd);
	snprintf(output, sizeof(output), ">%s", result_path);

	strlcpy(wan_ifname, get_wan_ifname(wan_unit), sizeof(wan_ifname));
	strlcpy(wan_gateway, nvram_pf_safe_get(wan_prefix, "gateway"), sizeof(wan_gateway));

	for (i = 0; stun_server_list[i]; i++) {
		if (realip_interrupted || !realip_current(wan_prefix, token))
			break;
		strlcpy(buf, stun_server_list[i], sizeof(buf));
		server = tmp_server = buf;
		port = NULL;
		if (strchr(buf, ':')) {
			server = strsep(&tmp_server, ":");
			port = tmp_server;
		}
		if (resolv_addr4(server, stun_server_ip, sizeof(stun_server_ip)) < 0) {
			printf("resolve stun server failed\n");
			continue;
		}

		// add stun server route
#ifdef RTCONFIG_MULTIWAN_IF
		if (is_mtwan)
			eval("ip", "rule", "add", "iif", "lo", "to", stun_server_ip, "table", table_str, "pref", pref_str);
#endif
#ifdef RTCONFIG_DUALWAN
		if (load_balance && wan_unit < WAN_UNIT_MAX)
			eval("ip", "rule", "add", "iif", "lo", "to", stun_server_ip, "table", table_str, "pref", pref_str);
#endif
		eval("ip", "route", "add", stun_server_ip, "via", wan_gateway, "dev", wan_ifname, "table", table_str);

		if (port)
			snprintf(stun_server, sizeof(stun_server), "%s:%s", stun_server_ip, port);
		else
			strlcpy(stun_server, stun_server_ip, sizeof(stun_server));
		// _dprintf("final stun server: %s\n", stun_server);

		ret = realip_exec(stun_argv, output, wan_prefix, token);

		// del stun server route
#ifdef RTCONFIG_MULTIWAN_IF
		if (is_mtwan)
			eval("ip", "rule", "del", "iif", "lo", "to", stun_server_ip, "table", table_str, "pref", pref_str);
#endif
#ifdef RTCONFIG_DUALWAN
		if (load_balance && wan_unit < WAN_UNIT_MAX)
			eval("ip", "rule", "del", "iif", "lo", "to", stun_server_ip, "table", table_str, "pref", pref_str);
#endif
		eval("ip", "route", "del", stun_server_ip, "via", wan_gateway, "dev", wan_ifname, "table", table_str);

		if (!ret) {
			FILE *fp = fopen(result_path, "r");
			while (fp && fgets(buf, sizeof(buf), fp)) {
				trimNL(buf);
				if (realip_valid(buf)) {
					success = 1;
					break;
				}
			}

			if (fp)
				fclose(fp);
			if (success)
				break;
		}
	}

done:
#ifdef RTCONFIG_MULTIWAN_IF
	if (is_mtwan) {
		char current_prefix[16];
		if (mtwan_get_real_wan(requested_unit, current_prefix, sizeof(current_prefix)) != wan_unit ||
		    strcmp(current_prefix, wan_prefix))
			success = 0;
	}
#else
	(void)requested_unit;
#endif
	if (realip_interrupted)
		success = 0;
	if (!realip_finish(wan_prefix, token, success ? buf : ""))
		success = 0;
	if (success)
		printf("%s\n", buf);
	unlink(result_path);
	if (route_lock >= 0)
		file_unlock(route_lock);
	return success ? 0 : -3;
}

int realip_main(int argc, char *argv[])
{
	char prefix[32], token[32];
	char *end;
	long unit;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = realip_signal;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	realip_interrupted = 0;
	if (argc == 5 && !strcmp(argv[1], "--report"))
		return realip_report(argv[2], argv[3], !strcmp(argv[4], "1"));
	if (argc == 2 && !strcmp(argv[1], "--begin-script"))
		return realip_script_begin();
	if (argc == 3 && !strcmp(argv[1], "--valid"))
		return realip_valid(argv[2]) ? 0 : 1;
	if (argc >= 3 && !strncmp(argv[1], "--", 2)) {
		unit = strtol(argv[2], &end, 10);
		if (!argv[2][0] || *end || unit < 0 || unit > INT_MAX)
			return 1;
		snprintf(prefix, sizeof(prefix), "wan%ld_", unit);
		if (argc == 3 && !strcmp(argv[1], "--begin")) {
			if (!realip_begin(prefix, token, sizeof(token)))
				return 1;
			puts(token);
			return 0;
		}
		if (argc == 4 && !strcmp(argv[1], "--current"))
			return realip_current(prefix, argv[3]) ? 0 : 1;
		if (argc == 5 && !strcmp(argv[1], "--finish"))
			return realip_finish(prefix, argv[3], argv[4]) ? 0 : 1;
		if (argc == 6 && !strcmp(argv[1], "--probe"))
			return realip_probe(prefix, argv[3], argv[4], argv[5]);
		return 1;
	}

	return realip_lookup(argc, argv);
}
#endif
