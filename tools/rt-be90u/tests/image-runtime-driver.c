/* Exercise vendor validators offline; all NVRAM writes stay in this process. */
#include <stdio.h>
#include <stdlib.h>
char *nvram_get(const char *key) { return getenv(key); }
int nvram_set(const char *key, const char *value) { return setenv(key, value, 1); }
int nvram_unset(const char *key) { return unsetenv(key); }
int nvram_commit(void) { return 0; }
extern int check_imagefile(char *path);
extern int check_imageheader(char *buf, long *filelen);
int main(int argc, char **argv)
{
	int result, header;
	long size = 0;
	char buf[4096];
	FILE *fp;
	if (argc != 2)
		return 2;
	setenv("productid", "TUF-BE9400", 1);
	setenv("odmpid", "RT-BE90U", 1);
	setenv("firmver", "3.0.0.6", 1);
	setenv("buildno", "102", 1);
	setenv("extendno", "58500", 1);
	fp = fopen(argv[1], "rb");
	if (!fp || fread(buf, 1, sizeof(buf), fp) != sizeof(buf))
		return 3;
	fclose(fp);
	header = check_imageheader(buf, &size);
	result = header ? check_imagefile(argv[1]) : -1;
	printf("RESULT %d %ld %d\n", header, size, result);
	return 0;
}
