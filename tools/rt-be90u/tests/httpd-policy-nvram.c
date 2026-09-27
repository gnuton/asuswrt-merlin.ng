/* Test-only durability observations for the packaged HTTP apply handler. */
#define nvram_commit unused_httpd_fixture_commit
#define _eval unused_httpd_fixture_eval
#include "httpd-nvram.c"
#undef nvram_commit
#undef _eval

int nvram_commit(void)
{
	FILE *stream;
	int failed;
	const char *policy = nvram_get("vpndirector_rulelist");
	stream = fopen("/tmp/policy-durable", "w");
	if (!stream) return -1;
	failed = fputs(policy ? policy : "", stream) < 0;
	if (fclose(stream) || failed) return -1;
	stream = fopen("/tmp/policy-commits", "a");
	if (!stream) return -1;
	failed = fputs("commit\n", stream) < 0;
	if (fclose(stream) || failed) return -1;
	return 0;
}

int _eval(char *const argv[], const char *path, int timeout, pid_t *pid)
{
	/* The real QCA httpd_nvram_commit -> sync_profile_update_time chain
	 * dispatches "nvram commit" through nvram_commit_bg. Emulate storage at
	 * that process boundary, without replacing the HTTP/library handlers. */
	if (argv && argv[0] && argv[1] && !argv[2] &&
	    !strcmp(argv[0], "nvram") && !strcmp(argv[1], "commit")) {
		if (pid) *pid = 0;
		fprintf(stderr, "TEST observed background nvram commit\n");
		return nvram_commit();
	}
	return unused_httpd_fixture_eval(argv, path, timeout, pid);
}
