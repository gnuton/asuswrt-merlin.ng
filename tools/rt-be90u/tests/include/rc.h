#ifndef TEST_RC_H
#define TEST_RC_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared.h"
#include "shutils.h"
char *safe_getenv(const char *name);
void ovpn_up_handler(void);
void ovpn_down_handler(void);
void ovpn_route_up_handler(void);
void ovpn_route_pre_down_handler(void);
void update_resolvconf(void);
void run_ovpn_event_script(void);
#endif
