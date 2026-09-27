#ifndef _AMVPN_DEFERRED_H_
#define _AMVPN_DEFERRED_H_

#include <stdio.h>

/* QCA retained Fusion bindings must not select unrelated fixed Merlin slots. */
int amvpn_refresh_deferred(void);
int amvpn_refresh_deferred_locked(void);
int amvpn_write_deferred_dns(FILE *fp, int family);

#endif
