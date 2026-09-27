/* Image shared library, actual script execution, synthetic NVRAM only. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

char *nvram_get(const char *name) { return getenv(name); }
int nvram_set(const char *name, const char *value) { return setenv(name, value, 1); }
int nvram_unset(const char *name) { return unsetenv(name); }
int nvram_commit(void) { return 0; }
extern void run_custom_script(char *name, int timeout, char *arg1, char *arg2);

int main(int argc, char **argv)
{
	if (argc == 2 && !strcmp(argv[1], "daemon")) {
		FILE *fp = fopen("/opt/var/run/fixture.ready", "w");
		if (!fp)
			return 1;
		fprintf(fp, "%ld\n", (long)getpid());
		fclose(fp);
		for (;;) pause();
	}
	if (argc != 3)
		return 2;
	setenv("jffs2_scripts", argv[1], 1);
	run_custom_script(argv[2], 30, NULL, NULL);
	return 0;
}
