/* Host-only firmware API shim for the real production realip.c. */
#ifndef REALIP_TEST_RC_H
#define REALIP_TEST_RC_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <net/if.h>
#define WAN_STATE_CONNECTED 2
#define WAN_UNIT_MAX 2
#define IP_ROUTE_TABLE_ID_DUALWAN_BASE 100
size_t strlcpy(char *, const char *, size_t);
size_t strlcat(char *, const char *, size_t);
char *nvram_safe_get(const char *);
char *nvram_pf_safe_get(const char *, const char *);
int nvram_get_int(const char *);
int nvram_pf_get_int(const char *, const char *);
int nvram_match(const char *, const char *);
int nvram_pf_set(const char *, const char *, const char *);
int nvram_pf_set_int(const char *, const char *, int);
int file_lock(const char *);
void file_unlock(int);
int _eval(char **, const char *, int, pid_t *);
#define eval(...) mock_eval(__VA_ARGS__, NULL)
int mock_eval(const char *, ...);
int resolv_addr4(const char *, char *, size_t);
int safe_atoi(const char *);
char *get_wan_ifname(int);
void trimNL(char *);
int get_nr_wan_unit(void);
int is_mtwan_unit(int);
int mtwan_get_real_wan(int, char *, size_t);
int mtwan_get_route_table_id(int, char *, size_t);
int realip_main(int, char **);
void realip_wan_state(const char *, int);
void realip_invalidate(const char *);
void cancel_realip(const char *);
void start_realip(int);
int notify_rc(const char *);
void update_upnp(void);
void realip_upnp_address(const char *, char *, size_t);
#endif
