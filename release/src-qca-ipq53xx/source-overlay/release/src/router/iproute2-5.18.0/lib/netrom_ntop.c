/* SPDX-License-Identifier: GPL-2.0 */

#include <sys/socket.h>
#include <errno.h>
#include <linux/ax25.h>

#include "utils.h"

const char *ax25_ntop1(const ax25_address *src, char *dst, socklen_t size);

const char *netrom_ntop(int af, const void *addr, char *buf, socklen_t buflen)
{
	switch (af) {
	case AF_NETROM:
		errno = 0;
		return ax25_ntop1((ax25_address *)addr, buf, buflen);

	default:
		errno = EAFNOSUPPORT;
	}

	return NULL;
}
