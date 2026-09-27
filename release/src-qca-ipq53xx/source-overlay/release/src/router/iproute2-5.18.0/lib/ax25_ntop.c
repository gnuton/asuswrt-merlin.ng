/* SPDX-License-Identifier: GPL-2.0+ */

#include <errno.h>
#include <sys/socket.h>
#include <linux/ax25.h>

#include "utils.h"

const char *ax25_ntop1(const ax25_address *src, char *dst, socklen_t size);

/*
 * AX.25 addresses are based on Amateur radio callsigns followed by an SSID
 * like XXXXXX-SS where the callsign consists of up to 6 ASCII characters
 * which are either letters or digits and the SSID is a decimal number in the
 * range 0..15.
 * Amateur radio callsigns are assigned by a country's relevant authorities
 * and are 3..6 characters though a few countries have assigned callsigns
 * longer than that.  AX.25 is not able to handle such longer callsigns.
 * There are further restrictions on the format of valid callsigns by
 * applicable national and international law.  Linux doesn't need to care and
 * will happily accept anything that consists of 6 ASCII characters in the
 * range of A-Z and 0-9 for a callsign such as the default AX.25 MAC address
 * LINUX-1 and the default broadcast address QST-0.
 * The SSID is just a number and not encoded in ASCII digits.
 *
 * Being based on HDLC AX.25 encodes addresses by shifting them one bit left
 * thus zeroing bit 0, the HDLC extension bit for all but the last bit of
 * a packet's address field but for our purposes here we're not considering
 * the HDLC extension bit that is it will always be zero.
 *
 * Linux' internal representation of AX.25 addresses in Linux is very similar
 * to this on the on-air or on-the-wire format.  The callsign is padded to
 * 6 octets by adding spaces, followed by the SSID octet then all 7 octets
 * are left-shifted by one bit.
 *
 * For example, for the address "LINUX-1" the callsign is LINUX and SSID is 1
 * the internal format is 98:92:9c:aa:b0:40:02.
 */

const char *ax25_ntop1(const ax25_address *src, char *dst, socklen_t size)
{
	char c, *s;
	int n;

	for (n = 0, s = dst; n < 6; n++) {
		c = (src->ax25_call[n] >> 1) & 0x7f;
		if (c != ' ')
			*s++ = c;
	}

	*s++ = '-';

	n = ((src->ax25_call[6] >> 1) & 0x0f);
	if (n > 9) {
		*s++ = '1';
		n -= 10;
	}

	*s++ = n + '0';
	*s++ = '\0';

	if (*dst == '\0' || *dst == '-') {
		dst[0] = '*';
		dst[1] = '\0';
	}

	return dst;
}

const char *ax25_ntop(int af, const void *addr, char *buf, socklen_t buflen)
{
	switch (af) {
	case AF_AX25:
		errno = 0;
		return ax25_ntop1((ax25_address *)addr, buf, buflen);

	default:
		errno = EAFNOSUPPORT;
	}

	return NULL;
}
