#ifndef TEST_SHUTILS_H
#define TEST_SHUTILS_H
int _eval(char *const argv[], const char *path, int timeout, int *pid);
int test_eval(const char *cmd, ...);
#define eval(...) test_eval(__VA_ARGS__)
#endif
