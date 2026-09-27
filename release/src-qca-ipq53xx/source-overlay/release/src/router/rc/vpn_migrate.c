/* One-time conversion of ordinary ASUS Fusion profiles to Merlin's VPN state.
 * Plan first, keep a private backup, then commit all NVRAM destinations and the
 * completion marker together. Existing JFFS keys/custom settings are untouched.
 * Ambiguous/unsupported input is retained for manual resolution, not discarded.
 */
#include <rtconfig.h>
#include <bcmnvram.h>
#include <shared.h>
#include <openvpn_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define MIGRATED "qca_merlin_vpn_migrated"
#define BACKUP "/jffs/openvpn/fusion-migration.backup"
#define POLICY "/jffs/openvpn/vpndirector_rulelist"
#define MAX_INDEX 20
#define MAX_CHANGES 64

struct vpn_change {
	char name[64];
	char *before, *after;
};

struct vpn_migration {
	struct vpn_change changes[MAX_CHANGES];
	int count, found, map[MAX_INDEX + 1], occupied[11];
	int enabled[OVPN_CLIENT_MAX + 1];
	int records, keep[MAX_VPNC_PROFILE];
	char profiles[8192], rules[8000], sdn[4096];
};

static int decimal(const char *s, int maximum)
{
	unsigned int n = 0;
	if (!s || !*s) return -1;
	for (; *s; ++s) {
		if (*s < '0' || *s > '9') return -1;
		n = n * 10 + (*s - '0');
		if (n > (unsigned int)maximum) return -1;
	}
	return n;
}

static int append(char *out, size_t size, const char *s)
{
	if (strlen(out) + strlen(s) >= size) return -1;
	strcat(out, s);
	return 0;
}

static int fields(char *record, char **out, int size)
{
	int n = 0;
	while (record) {
		if (n == size) return -1;
		out[n++] = strsep(&record, ">");
	}
	return n;
}

static int change(struct vpn_migration *m, const char *name, const char *value)
{
	struct vpn_change *c;
	const char *old = nvram_get(name);
	int i;
	if (old && !strcmp(old, value)) return 0;
	for (i = 0; i < m->count; ++i)
		if (!strcmp(m->changes[i].name, name)) return -1;
	if (m->count == MAX_CHANGES || strlen(name) >= sizeof(c->name)) return -1;
	c = &m->changes[m->count++];
	strcpy(c->name, name);
	c->before = old ? strdup(old) : NULL;
	c->after = strdup(value);
	return ((!old || c->before) && c->after) ? 0 : -1;
}

/* Only replace an absent/default destination or one already matching the
 * source. Never silently overwrite an independently edited Merlin setting. */
static int setting(struct vpn_migration *m, const char *prefix, const char *suffix, const char *value)
{
	char name[64];
	const char *old, *def;
	if (snprintf(name, sizeof(name), "%s%s", prefix, suffix) >= (int)sizeof(name)) return -1;
	old = nvram_get(name);
	def = nvram_default_get(name);
	if (old && strcmp(old, value) && (!def || strcmp(old, def))) return -1;
	return change(m, name, value);
}

static int profiles(struct vpn_migration *m)
{
	char *copy, *cursor, *row, *parts[12], *raw, prefix[32], start[32] = "", unitstr[8];
	const char *source = nvram_safe_get("vpnc_clientlist"), *eas = nvram_get("vpn_clientx_eas");
	int count = 0, n, unit, active, index, id, i;
	if (strlen(source) >= sizeof(m->profiles) || (eas && strlen(eas) >= sizeof(start))) return -1;
	copy = strdup(eas ? eas : "");
	if (!copy) return -1;
	cursor = copy;
	while ((row = strsep(&cursor, ","))) {
		if (!*row && !cursor) break;
		unit = decimal(row, OVPN_CLIENT_MAX);
		if (unit < 1 || m->enabled[unit]) { free(copy); return -1; }
		m->enabled[unit] = 1;
	}
	free(copy);
	copy = strdup(source);
	if (!copy) return -1;
	cursor = copy;
	while ((row = strsep(&cursor, "<"))) {
		if (!*row) continue;
		if (++count > MAX_VPNC_PROFILE) goto fail;
		raw = strdup(row);
		if (!raw) goto fail;
		n = fields(row, parts, 12);
		/* Older records without explicit active/index fields are ambiguous. */
		if (n < 7) { free(raw); goto fail; }
		index = decimal(parts[6], MAX_INDEX);
		active = decimal(parts[5], 1);
		if (index < 1 || active < 0 || m->map[index]) { free(raw); goto fail; }
		m->map[index] = -1; /* preserve unknown profiles, reject references to them */
		if (strcmp(parts[1], "OpenVPN") && strcmp(parts[1], "WireGuard")) {
			/* Retained provider records may own the same physical profile slot. */
			id = 0;
			if (!strcmp(parts[1], "NordVPN") || !strcmp(parts[1], "Surfshark")) id = 1;
			if (!strcmp(parts[1], "HMA") || !strcmp(parts[1], "CyberGhost")) id = 6;
			if (id) {
				unit = decimal(parts[2], 5);
				if (unit < 1 || m->occupied[id + unit - 1]) { free(raw); goto fail; }
				m->occupied[id + unit - 1] = 1;
			}
			m->keep[count - 1] = 1;
			if (append(m->profiles, sizeof(m->profiles), "<") ||
			    append(m->profiles, sizeof(m->profiles), raw)) { free(raw); goto fail; }
			free(raw);
			continue;
		}
		free(raw);
		unit = decimal(parts[2], 5);
		if (unit < 1 || strlen(parts[0]) > 31) goto fail;
		/* Provider, tunnel or alternate-WAN variants need a separate converter. */
		/* Field 11 is origin metadata (e.g. Web), not a routing option. */
		for (i = 7; i < n && i < 11; ++i)
			if (*parts[i] && strcmp(parts[i], "0")) goto fail;
		id = unit + (!strcmp(parts[1], "OpenVPN") ? 5 : 0);
		if (m->occupied[id]) goto fail;
		m->occupied[id] = 1;
		m->map[index] = id;
		++m->found;
		if (id > 5) {
			char key[32];
			/* Boot defaults leave this absent while conversion is pending.
			 * An existing value, including empty, may be a user edit. */
			if (eas && m->enabled[unit] != active) goto fail;
			m->enabled[unit] = active;
			snprintf(prefix, sizeof(prefix), "vpn_client%d_", unit);
			snprintf(key, sizeof(key), "%saddr", prefix);
			if (active && !*nvram_safe_get(key)) goto fail;
			if (strlen(parts[3]) > 63 || strlen(parts[4]) > 255 ||
			    setting(m, prefix, "username", parts[3]) ||
			    setting(m, prefix, "password", parts[4]) ||
			    setting(m, prefix, "rgw", "2")) goto fail;
		} else {
			const char *old;
			char key[32];
			snprintf(prefix, sizeof(prefix), "wgc%d_", unit);
			snprintf(key, sizeof(key), "%senable", prefix);
			old = nvram_get(key);
			if (old && decimal(old, 1) != active) goto fail;
			if (change(m, key, active ? "1" : "0")) goto fail;
			/* A reset removes these fields; a stale active Fusion row must not
			 * recreate an enabled client whose configuration was cleared. */
			if (active) {
				snprintf(key, sizeof(key), "%spriv", prefix);
				if (!*nvram_safe_get(key)) goto fail;
				snprintf(key, sizeof(key), "%sppub", prefix);
				if (!*nvram_safe_get(key)) goto fail;
				snprintf(key, sizeof(key), "%saddr", prefix);
				if (!*nvram_safe_get(key)) goto fail;
			}
		}
		if (setting(m, prefix, "desc", parts[0])) goto fail;
	}
	free(copy);
	m->records = count;
	for (i = 1; i <= OVPN_CLIENT_MAX; ++i)
		if (m->enabled[i]) {
			snprintf(unitstr, sizeof(unitstr), "%d,", i);
			if (append(start, sizeof(start), unitstr)) return -1;
		}
	return m->found ? change(m, "vpn_clientx_eas", start) : 0;
fail:
	free(copy);
	return -1;
}

/* ASUS keeps PPTP options in a separate, positionally aligned list. */
static int pptp_options(struct vpn_migration *m)
{
	const char *source = nvram_safe_get("vpnc_pptp_options_x_list");
	char *copy, *cursor, *row, output[2048] = "";
	int count = 0, result = -1;
	if (!*source) return 0;
	if (*source != '<' || strlen(source) >= sizeof(output)) return -1;
	copy = strdup(source + 1);
	if (!copy) return -1;
	cursor = copy;
	while ((row = strsep(&cursor, "<"))) {
		if (count >= m->records) goto done;
		if (m->keep[count] && (append(output, sizeof(output), "<") || append(output, sizeof(output), row))) goto done;
		++count;
	}
	if (count == m->records) result = change(m, "vpnc_pptp_options_x_list", output);
done:
	free(copy);
	return result;
}

static int selector(const char *value)
{
	char buf[32], *slash;
	struct in_addr address;
	if (!*value) return 1;
	if (strlen(value) >= sizeof(buf)) return 0;
	strcpy(buf, value);
	slash = strchr(buf, '/');
	if (slash) { *slash++ = 0; if (decimal(slash, 32) < 0) return 0; }
	return inet_pton(AF_INET, buf, &address) == 1;
}

static int policies(struct vpn_migration *m)
{
	char *copy, *cursor, *row, *parts[5], target[16], rule[128];
	int n, index, id, active, count = 0;
	const char *source = nvram_safe_get("vpnc_dev_policy_list");
	if (strlen(source) >= 8192) return -1;
	copy = strdup(source);
	if (!copy) return -1;
	cursor = copy;
	while ((row = strsep(&cursor, "<"))) {
		if (!*row) continue;
		n = fields(row, parts, 5);
		if (n < 4 || ++count > MAX_DEV_POLICY || (n == 5 && *parts[4])) goto fail;
		active = decimal(parts[0], 1);
		index = decimal(parts[3], MAX_INDEX);
		if (active < 0 || index < 0 || !selector(parts[1]) || !selector(parts[2])) goto fail;
		id = index ? m->map[index] : 0;
		if (index && id < 1) goto fail;
		if (!id) strcpy(target, "WAN");
		else snprintf(target, sizeof(target), "%s%d", id > 5 ? "OVPN" : "WGC", id > 5 ? id - 5 : id);
		n = snprintf(rule, sizeof(rule), "<%d>Fusion rule %d>%s>%s>%s", active, count, parts[1], parts[2], target);
		if (n >= (int)sizeof(rule) || append(m->rules, sizeof(m->rules), rule)) goto fail;
	}
	free(copy);
	return 0;
fail:
	free(copy);
	return -1;
}

static int networks(struct vpn_migration *m)
{
	const char *source = nvram_safe_get("sdn_rl"), *default_value = nvram_safe_get("vpnc_default_wan");
	char *copy, *cursor, *row, *parts[23], replacement[8];
	int n, index, id, i, default_index, saw_default = 0;
	default_index = *default_value ? decimal(default_value, MAX_INDEX) : 0;
	if (default_index < 0 || (default_index && m->map[default_index] < 1) ||
	    strlen(source) >= sizeof(m->sdn) || nvram_get_int("vpnc_default_wan_tmp")) return -1;
	copy = strdup(source);
	if (!copy) return -1;
	cursor = copy;
	while ((row = strsep(&cursor, "<"))) {
		if (!*row) continue;
		n = fields(row, parts, 23);
		if (n < 7 || decimal(parts[0], 255) < 0) goto fail;
		index = decimal(parts[6], MAX_INDEX);
		if (index < 0) goto fail;
		if (!strcmp(parts[0], "0")) {
			if (saw_default++) goto fail;
			if (default_index) {
				if (index && index != default_index) goto fail;
				index = default_index;
			}
		}
		id = index ? m->map[index] : 0;
		if (index && id < 1) goto fail;
		snprintf(replacement, sizeof(replacement), "%d", id);
		parts[6] = replacement;
		if (append(m->sdn, sizeof(m->sdn), "<")) goto fail;
		for (i = 0; i < n; ++i)
			if ((i && append(m->sdn, sizeof(m->sdn), ">")) || append(m->sdn, sizeof(m->sdn), parts[i])) goto fail;
	}
	free(copy);
	if (default_index && !saw_default) return -1;
	return change(m, "sdn_rl", m->sdn) || change(m, "vpnc_default_wan", "0") ? -1 : 0;
fail:
	free(copy);
	return -1;
}

/* Compare regular files only; a read error is not an absent file. */
static int same_file(const char *path, const char *data, size_t size)
{
	struct stat st;
	char buf[512];
	size_t offset = 0, chunk;
	ssize_t got;
	int fd, ok = 0;
	fd = open(path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
	if (fd < 0) return errno == ENOENT ? 0 : -1;
	if (fstat(fd, &st) || !S_ISREG(st.st_mode) || st.st_size != (off_t)size) goto done;
	while (offset < size) {
		chunk = size - offset < sizeof(buf) ? size - offset : sizeof(buf);
		got = read(fd, buf, chunk);
		if (got <= 0 || memcmp(buf, data + offset, got)) goto done;
		offset += got;
	}
	ok = 1;
done:
	close(fd);
	return ok ? 1 : -1;
}

/* Durable, mode-0600, no-clobber backup of every NVRAM value we change.
 * A retry must match the existing backup exactly; never replace an old backup.
 * Binary format: header then repeated presence byte, name NUL, old value NUL.
 */
static int backup(struct vpn_migration *m)
{
	static const char header[] = "RT-BE90U Fusion migration 1\n";
	char temporary[] = "/jffs/openvpn/.fusion-backup-XXXXXX";
	char *data, *p;
	size_t size = sizeof(header) - 1, left;
	ssize_t n;
	int i, fd = -1, dir = -1, result = -1, made_temporary = 0;
	struct stat st;
	for (i = 0; i < m->count; ++i)
		size += 1 + strlen(m->changes[i].name) + 1 + (m->changes[i].before ? strlen(m->changes[i].before) : 0) + 1;
	data = malloc(size);
	if (!data) return -1;
	memcpy(data, header, sizeof(header) - 1);
	p = data + sizeof(header) - 1;
	for (i = 0; i < m->count; ++i) {
		struct vpn_change *c = &m->changes[i];
		*p++ = c->before != NULL;
		strcpy(p, c->name); p += strlen(p) + 1;
		strcpy(p, c->before ? c->before : ""); p += strlen(p) + 1;
	}
	i = same_file(BACKUP, data, size);
	if (i < 0) goto done;
	if (i == 1) {
		if (lstat(BACKUP, &st) || (st.st_mode & 0777) != 0600 || st.st_uid != geteuid()) goto done;
		dir = open(OVPN_DIR_SAVE, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
		if (dir >= 0 && !fsync(dir)) result = 0;
		goto done;
	}
	if (mkdir(OVPN_DIR_SAVE, 0700) && errno != EEXIST) goto done;
	if (lstat(OVPN_DIR_SAVE, &st) || !S_ISDIR(st.st_mode)) goto done;
	/* Persist a newly created openvpn directory in its parent as well. */
	dir = open("/jffs", O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
	if (dir < 0 || fsync(dir)) goto done;
	close(dir);
	dir = open(OVPN_DIR_SAVE, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
	fd = mkstemp(temporary);
	made_temporary = fd >= 0;
	if (dir < 0 || fd < 0 || fchmod(fd, 0600)) goto done;
	p = data; left = size;
	while (left) {
		n = write(fd, p, left);
		if (n < 0 && errno == EINTR) continue;
		if (n <= 0) goto done;
		p += n; left -= n;
	}
	if (fsync(fd)) goto done;
	if (close(fd)) { fd = -1; goto done; }
	fd = -1;
	if (link(temporary, BACKUP) || fsync(dir)) goto done;
	result = 0;
done:
	if (fd >= 0) close(fd);
	if (dir >= 0) close(dir);
	if (made_temporary) unlink(temporary);
	free(data);
	return result;
}

int migrate_qca_vpn_config(void)
{
	struct vpn_migration *m;
	int i, result = -1;
	const char *old_rules, *version = nvram_get(MIGRATED);
	if (nvram_match(MIGRATED, "1") || !*nvram_safe_get("vpnc_clientlist")) return 0;
	if (version && *version) return -1; /* Do not reinterpret a newer schema. */
	m = calloc(1, sizeof(*m));
	if (!m) return -1;
	if (profiles(m)) goto done;
	if (!m->found) { result = 0; goto done; }
	if (pptp_options(m) || policies(m) || networks(m)) goto done;
	old_rules = nvram_safe_get("vpndirector_rulelist");
	if ((*old_rules && strcmp(old_rules, m->rules)) || same_file(POLICY, m->rules, strlen(m->rules)) < 0) goto done;
	if (change(m, "vpndirector_rulelist", m->rules) ||
	    change(m, "vpnc_clientlist", m->profiles) ||
	    change(m, "vpnc_dev_policy_list", "") || change(m, MIGRATED, "1")) goto done;
	/* This SDK can report success without writing when commits are suppressed. */
	if (nvram_match("restart_wifi", "1") || nvram_get(ASUS_STOP_COMMIT) || backup(m)) goto done;
	for (i = 0; i < m->count; ++i)
		if (nvram_set(m->changes[i].name, m->changes[i].after)) goto rollback;
	if (nvram_commit()) goto rollback;
	result = 0;
	goto done;
rollback:
	/* Restore the process-visible state too, so a failed commit cannot become
	 * an accidental partial migration in a later, unrelated commit. */
	for (i = 0; i < m->count; ++i) {
		struct vpn_change *c = &m->changes[i];
		if (c->before ? nvram_set(c->name, c->before) : nvram_unset(c->name))
			logmessage("vpn-migration", "Could not restore a setting after migration failure");
	}
done:
	for (i = 0; i < m->count; ++i) { free(m->changes[i].before); free(m->changes[i].after); }
	free(m);
	if (result) logmessage("vpn-migration", "Fusion conversion deferred: unsupported, conflicting or unwritable configuration");
	return result;
}
