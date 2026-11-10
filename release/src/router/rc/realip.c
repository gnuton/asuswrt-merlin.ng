/*
 * Real-IP lookup synchronization and STUN service
 *
 * The routed lookup below was extracted from rc/services.c. Its inherited
 * license follows; the synchronization code was added in 2026.
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

/* Preserve only a result for this exact connection while its refresh runs.
 * Failure/cancellation or any connection change clears it; no failure cache. */
static void realip_reset(const char *prefix, int refresh)
{
	unsigned long long generation;
	char buf[32], context[256];

	realip_context(prefix, context, sizeof(context));
	refresh = refresh && !strcmp(context, nvram_pf_safe_get(prefix, "realip_context"));
	generation = strtoull(nvram_pf_safe_get(prefix, "realip_generation"), NULL, 10);
	refresh = refresh && generation != ULLONG_MAX;
	/* Do not reuse an ownership token even at counter exhaustion. */
	if (generation != ULLONG_MAX)
		generation++;
	snprintf(buf, sizeof(buf), "%llu", generation);
	nvram_pf_set(prefix, "realip_state", "0");
	if (!refresh) {
		nvram_pf_set(prefix, "realip_context", "");
		nvram_pf_set(prefix, "realip_ip", "");
	}
	nvram_pf_set(prefix, "realip_generation", buf);
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
	realip_reset(prefix, !blocked &&
		!nvram_pf_get_int(prefix, "realip_blocked"));
	if (blocked && !nvram_pf_get_int(prefix, "realip_blocked"))
		nvram_pf_set(prefix, "realip_epoch", nvram_pf_safe_get(prefix, "realip_generation"));
	nvram_pf_set(prefix, "realip_blocked", blocked ? "1" : "0");
#ifdef RTCONFIG_MULTIWAN_IF
	if (unit >= 0 && unit <= INT_MAX && !strcmp(end, "_") && is_mtwan_unit(unit) &&
	    mtwan_get_real_wan(unit, mapped, sizeof(mapped)) >= 0 && strcmp(prefix, mapped)) {
		realip_reset(mapped, !blocked &&
			!nvram_pf_get_int(mapped, "realip_blocked"));
		if (blocked && !nvram_pf_get_int(mapped, "realip_blocked"))
			nvram_pf_set(mapped, "realip_epoch", nvram_pf_safe_get(mapped, "realip_generation"));
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
	realip_reset(prefix, 0);
	file_unlock(lock);
}

/* State transition and lookup invalidation are one transaction. A DHCP renew
 * revokes pending lookups but preserves the address for an unchanged refresh.
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
	realip_reset(prefix, 1);
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

static int realip_script_begin(char tokens[2][32], int *lb)
{
	int unit, primary = -1, auxstate, lock = realip_lock();
	char prefix[16];

	if (lock < 0)
		return 1;
	*lb = nvram_match("wans_mode", "lb");
	if (nvram_match("wan0_primary", "1"))
		primary = 0;
	else if (nvram_match("wan1_primary", "1"))
		primary = 1;
	for (unit = 0; unit < 2; unit++) {
		if (!*lb && unit != primary)
			continue;
		snprintf(prefix, sizeof(prefix), "wan%d_", unit);
		auxstate = nvram_pf_get_int(prefix, "auxstate_t");
		if (*lb && (nvram_pf_get_int(prefix, "sbstate_t") != 0 ||
		           (auxstate != 0 && auxstate != 2)))
			continue;
		realip_begin_locked(prefix, tokens[unit], sizeof(tokens[unit]));
	}
	file_unlock(lock);
	return !*lb && !tokens[0][0] && !tokens[1][0];
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

/* A single locked snapshot: never combine state/IP from different requests.
 * A pending refresh may retain its previous address on the same connection.
 * Completed failure clears the address, and connection changes revoke it.
 * When STUN agrees with the interface, let MiniUPnPd use the interface address
 * directly; a later unavailable STUN result need not restart a public WAN. */
void realip_upnp_address(const char *prefix, char *ip, size_t len)
{
	int lock = realip_lock();

	ip[0] = 0;
	if (lock < 0)
		return;
	if ((nvram_pf_get_int(prefix, "realip_state") == 2 ||
	     nvram_pf_get_int(prefix, "realip_state") == 0) &&
	    realip_matches(prefix, nvram_pf_safe_get(prefix, "realip_generation")) &&
	    realip_valid(nvram_pf_safe_get(prefix, "realip_ip")) &&
	    strcmp(nvram_pf_safe_get(prefix, "realip_ip"), nvram_pf_safe_get(prefix, "ipaddr")))
		strlcpy(ip, nvram_pf_safe_get(prefix, "realip_ip"), len);
	file_unlock(lock);
}

static int realip_finish(const char *prefix, const char *token, const char *ip)
{
	int valid = realip_valid(ip);
	int published = 0;
	int ok, lock = realip_lock();
	if (lock < 0)
		return 0;
	ok = !realip_interrupted && realip_owned(prefix, token);
	/* A context change fails this request; it must not leave it pending.
	 * Only a pending token can publish, so completion cannot be replayed. */
	if (ok || (token[0] && !strcmp(token, nvram_pf_safe_get(prefix, "realip_generation")) &&
	           nvram_pf_get_int(prefix, "realip_state") == 0 &&
	           nvram_pf_safe_get(prefix, "realip_context")[0])) {
		published = 1;
		valid = valid && ok;
		/* Readers must never observe ready with an old or invalid address. */
		nvram_pf_set(prefix, "realip_state", "0");
		nvram_pf_set(prefix, "realip_ip", valid ? ip : "");
		nvram_pf_set(prefix, "realip_state", valid ? "2" : "1");
	}
	file_unlock(lock);
	if (published && getpid() != 1)
		notify_rc("update_upnp");
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
	char *args[] = { "realip", "--script", unitstr, token, NULL };
	pid_t pid;

	if (unit < 0)
		return;
	snprintf(prefix, sizeof(prefix), "wan%d_", unit);
	if (unit > 1) {
		args[1] = unitstr;
		args[2] = token;
		args[3] = prefix;
	}
#ifdef RTCONFIG_MULTIWAN_IF
	args[1] = unitstr;
	args[2] = token;
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
	    _eval(args, output, 10, &pid) != 0)
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

static int realip_read_result(const char *path, char *ip, size_t len)
{
	char line[64];
	FILE *fp = fopen(path, "r");
	int found = 0;

	while (fp && fgets(line, sizeof(line), fp)) {
		trimNL(line);
		if (realip_valid(line)) {
			strlcpy(ip, line, len);
			found = 1;
			break;
		}
	}
	if (fp)
		fclose(fp);
	return found;
}

/* Own the probe through DNS and STUN. _eval supplies a ten-second alarm. */
static int realip_probe(const char *prefix, const char *token, char *ifname, char *server,
	char *result, size_t len)
{
	char result_path[] = "/tmp/stun.XXXXXX", output[64];
	char *args[] = { "ministun", "-t", "1000", "-c", "1", "-i", ifname, server, NULL };
	int fd, success = 0;

	if (!server[0])
		args[7] = NULL;
	fd = mkstemp(result_path);
	if (fd < 0)
		return 1;
	close(fd);
	snprintf(output, sizeof(output), ">%s", result_path);
	if (!realip_exec(args, output, prefix, token) && !realip_interrupted && realip_current(prefix, token))
		success = realip_read_result(result_path, result, len);
	unlink(result_path);
	return success ? 0 : 1;
}

/* The DDNS script and rc use the same worker, without shell helper processes.
 * Reserve both LB units before either probe and report only a current pair. */
static int realip_script(int argc, char **argv)
{
	char *servers[] = { "", "stun.l.google.com:19302", "stun.cloudflare.com:3478",
		"stun1.l.google.com:19302", "stun2.l.google.com:19302",
		"stun3.l.google.com:19302", "stun4.l.google.com:19302", NULL };
	char tokens[2][32] = { "", "" }, prefix[16], ifname[IFNAMSIZ], ip[64];
	int unit, i, lb = nvram_match("wans_mode", "lb"), rc = 1;

	if (argc == 4) {
		if (strcmp(argv[2], "0") && strcmp(argv[2], "1"))
			return 1;
		strlcpy(tokens[atoi(argv[2])], argv[3], sizeof(tokens[0]));
	} else if (argc != 2 || realip_script_begin(tokens, &lb))
		return 1;

	for (unit = 0; unit < 2; unit++) {
		if (!tokens[unit][0])
			continue;
		snprintf(prefix, sizeof(prefix), "wan%d_", unit);
		if (realip_interrupted || !realip_current(prefix, tokens[unit]))
			goto done;
		strlcpy(ifname, get_wan_ifname(unit), sizeof(ifname));
		ip[0] = 0;
		for (i = 0; servers[i]; i++) {
			if (realip_interrupted || !realip_current(prefix, tokens[unit]))
				goto done;
			if (!realip_probe(prefix, tokens[unit], ifname, servers[i], ip, sizeof(ip)))
				break;
		}
		if (!realip_finish(prefix, tokens[unit], ip))
			goto done;
	}
	if (!realip_interrupted)
		rc = realip_report(tokens[0], tokens[1], lb);
 done:
	/* Completed tokens cannot publish twice; only unfinished ownership fails. */
	for (unit = 0; unit < 2; unit++) {
		if (tokens[unit][0]) {
			snprintf(prefix, sizeof(prefix), "wan%d_", unit);
			realip_finish(prefix, tokens[unit], "");
		}
	}
	return rc;
}

static int realip_lookup(int argc, char *argv[])
{
	char *stun_server_list[] = {
		"stun.l.google.com",
		"stun.stunprotocol.org",
		"stun4.l.google.com:19302",
		"stun.pjsip.org",
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

		if (!ret && realip_read_result(result_path, buf, sizeof(buf))) {
			success = 1;
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
	unlink(result_path);
	if (route_lock >= 0)
		file_unlock(route_lock);
	if (!realip_finish(wan_prefix, token, success ? buf : ""))
		success = 0;
	if (success)
		printf("%s\n", buf);
	return success ? 0 : -3;
}

int realip_main(int argc, char *argv[])
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = realip_signal;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	realip_interrupted = 0;
	/* exec preserves an inherited SIG_IGN; waitpid must own our children. */
	signal(SIGCHLD, SIG_DFL);
	if (argc >= 2 && !strcmp(argv[1], "--script"))
		return realip_script(argc, argv);

	return realip_lookup(argc, argv);
}
#endif
