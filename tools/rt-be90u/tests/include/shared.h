#ifndef TEST_SHARED_H
#define TEST_SHARED_H
#include <stdio.h>
int nvram_match(const char *name, const char *value);
int f_exists(const char *path);
int d_exists(const char *path);
void logmessage(const char *name, const char *format, ...);
void run_custom_script(char *name, int timeout, char *arg1, char *arg2);
void run_postconf(char *name, char *config);
void append_custom_config(char *config, FILE *fp);
void use_custom_config(char *config, char *target);
void setup_jffs_dirs(void);
#endif
