/* File-backed NVRAM and real POSIX processes/locks; no router/network access. */
#include "rc.h"
#include <stdarg.h>
#include <sys/wait.h>

size_t strlcpy(char *d, const char *s, size_t n)
{
	size_t len = strlen(s);
	if (n) { size_t copy = len < n - 1 ? len : n - 1; memcpy(d, s, copy); d[copy] = 0; }
	return len;
}
size_t strlcat(char *d, const char *s, size_t n)
{
	size_t len = strlen(d); return len + strlcpy(d + len, s, n - len);
}
static void path(char *buf, size_t n, const char *key)
{
	snprintf(buf, n, "%s/%s", getenv("REALIP_TEST_DIR"), key);
}
char *nvram_safe_get(const char *key)
{
	static char values[16][512];
	static unsigned idx;
	char name[1024], *buf = values[idx++ % 16];
	FILE *fp;
	path(name, sizeof(name), key);
	buf[0] = 0;
	fp = fopen(name, "r");
	if (fp) { size_t len = fread(buf, 1, 511, fp); buf[len] = 0; fclose(fp); }
	return buf;
}
char *nvram_pf_safe_get(const char *prefix, const char *key)
{
	char name[128]; snprintf(name, sizeof(name), "%s%s", prefix, key); return nvram_safe_get(name);
}
int nvram_get_int(const char *key) { return atoi(nvram_safe_get(key)); }
int nvram_pf_get_int(const char *p, const char *k) { return atoi(nvram_pf_safe_get(p, k)); }
int nvram_match(const char *k, const char *v) { return !strcmp(nvram_safe_get(k), v); }
int nvram_pf_set(const char *prefix, const char *key, const char *value)
{
	char name[1024], fullkey[128];
	FILE *fp;
	snprintf(fullkey, sizeof(fullkey), "%s%s", prefix, key);
	path(name, sizeof(name), fullkey);
	fp = fopen(name, "w");
	if (!fp) abort();
	fputs(value, fp); fclose(fp);
	if (getenv("REALIP_TEST_REGISTER_GATE") && !strcmp(prefix, "wan0_") &&
	    !strcmp(key, "realip_context") && value[0]) {
		char marker[1024]; FILE *event;
		path(marker, sizeof(marker), "register_entered");
		event = fopen(marker, "w"); fclose(event);
		while (access(getenv("REALIP_TEST_REGISTER_GATE"), F_OK)) usleep(10000);
	}
	/* Deterministically widen publication's check/write race for tests. */
	if (getenv("REALIP_TEST_WRITE_DELAY") && !strcmp(key, "realip_ip")) usleep(200000);
	return 0;
}
int nvram_pf_set_int(const char *p, const char *k, int v)
{
	char buf[32]; snprintf(buf, sizeof(buf), "%d", v); return nvram_pf_set(p, k, buf);
}
int file_lock(const char *tag)
{
	char name[1024]; struct flock lock; int fd;
	path(name, sizeof(name), tag);
	fd = open(name, O_CREAT | O_RDWR, 0600);
	if (fd < 0) abort();
	memset(&lock, 0, sizeof(lock)); lock.l_type = F_WRLCK;
	while (fcntl(fd, F_SETLKW, &lock) < 0) if (errno != EINTR) abort();
	return fd;
}
void file_unlock(int fd) { close(fd); }
int _eval(char **args, const char *output, int timeout, pid_t *child)
{
	pid_t pid; int status;
	(void)timeout;
	pid = fork();
	if (pid < 0) return -1;
	if (!pid) {
		if (output) {
			const char *file = output; int fd;
			while (*file == '>') file++;
			fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0600);
			if (fd < 0) _exit(126);
			dup2(fd, STDOUT_FILENO); dup2(fd, STDERR_FILENO); close(fd);
		}
		execvp(args[0], args); _exit(127);
	}
	if (child) { *child = pid; return 0; }
	while (waitpid(pid, &status, 0) < 0) if (errno != EINTR) return -1;
	return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
int mock_eval(const char *arg, ...)
{
	char name[1024]; FILE *fp; va_list ap;
	path(name, sizeof(name), "routes"); fp = fopen(name, "a");
	va_start(ap, arg);
	while (arg) { fprintf(fp, "%s ", arg); arg = va_arg(ap, const char *); }
	va_end(ap); fputc('\n', fp); fclose(fp); return 0;
}
int resolv_addr4(const char *host, char *ip, size_t n)
{
	(void)host; strlcpy(ip, "192.0.2.1", n); return 0;
}
int safe_atoi(const char *s) { return s ? atoi(s) : 0; }
char *get_wan_ifname(int unit)
{
	char prefix[32]; const char *proto;
	snprintf(prefix, sizeof(prefix), "wan%d_", unit);
	proto = nvram_pf_safe_get(prefix, "proto");
	return nvram_pf_safe_get(prefix, !strcmp(proto, "pppoe") ? "pppoe_ifname" : "ifname");
}
void trimNL(char *s) { s[strcspn(s, "\r\n")] = 0; }
int get_nr_wan_unit(void) { return 2; }
int is_mtwan_unit(int unit) { return unit >= 10 && unit < 100; }
int mtwan_get_real_wan(int unit, char *prefix, size_t n)
{
	int real = nvram_get_int("test_mapped_unit"); (void)unit;
	snprintf(prefix, n, "wan%d_", real); return real;
}
int mtwan_get_route_table_id(int unit, char *table, size_t n)
{
	snprintf(table, n, "%d", 100 + unit * 100); return 0;
}
int main(int argc, char **argv)
{
	const char *prog = strrchr(argv[0], '/'); prog = prog ? prog + 1 : argv[0];
	if (!strcmp(prog, "nvram")) {
		if (argc == 3 && !strcmp(argv[1], "get")) { puts(nvram_safe_get(argv[2])); return 0; }
		return 1;
	}
	if (argc >= 3 && !strcmp(argv[1], "--test-state")) { realip_wan_state(argv[2], atoi(argv[3])); return 0; }
	if (argc == 3 && !strcmp(argv[1], "--test-down")) { realip_invalidate(argv[2]); return 0; }
	if (argc == 3 && !strcmp(argv[1], "--test-cancel")) { cancel_realip(argv[2]); return 0; }
	if (argc == 3 && !strcmp(argv[1], "--test-start")) { start_realip(atoi(argv[2])); return 0; }
	return realip_main(argc, argv);
}
