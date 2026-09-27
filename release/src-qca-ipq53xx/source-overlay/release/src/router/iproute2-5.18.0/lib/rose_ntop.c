/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/sockios.h>
#include <linux/rose.h>

#include "rt_names.h"
#include "utils.h"

static const char *rose_ntop1(const rose_address *src, char *dst,
			      socklen_t size)
{
	char *p = dst;
	int i;

	if (size < 10)
		return NULL;

	for (i = 0; i < 5; i++) {
		*p++ = '0' + ((src->rose_addr[i] >> 4) & 0xf);
		*p++ = '0' + ((src->rose_addr[i]     ) & 0xf);
	}

	if (size == 10)
		return dst;

	*p = '\0';

	return dst;
}

const char *rose_ntop(int af, const void *addr, char *buf, socklen_t buflen)
{
	switch (af) {
	case AF_ROSE:
		errno = 0;
		return rose_ntop1((rose_address *)addr, buf, buflen);

	default:
		errno = EAFNOSUPPORT;
	}

	return NULL;
}
