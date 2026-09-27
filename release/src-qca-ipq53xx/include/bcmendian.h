/*
 * local version of endian.h - byte order defines
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: bcmendian.h,v 1.31.296.2 2009/10/12 23:03:37 Exp $
*/

#ifndef _BCMENDIAN_H_
#define _BCMENDIAN_H_

#ifdef ASUS_NVRAM
#include <nvram/typedefs.h>
#else	// !ASUS_NVRAM
#include <typedefs.h>
#endif	// ASUS_NVRAM

/* Byte swap a 16 bit value */
#define BCMSWAP16(val) \
	((uint16)(\
		(((uint16)(val) & (uint16)0x00ffU) << 8) | \
		(((uint16)(val) & (uint16)0xff00U) >> 8)))

/* Byte swap a 32 bit value */
#define BCMSWAP32(val) \
	((uint32)(\
		(((uint32)(val) & (uint32)0x000000ffUL) << 24) | \
		(((uint32)(val) & (uint32)0x0000ff00UL) <<  8) | \
		(((uint32)(val) & (uint32)0x00ff0000UL) >>  8) | \
		(((uint32)(val) & (uint32)0xff000000UL) >> 24)))

/* 2 Byte swap a 32 bit value */
#define BCMSWAP32BY16(val) \
	((uint32)(\
		(((uint32)(val) & (uint32)0x0000ffffUL) << 16) | \
		(((uint32)(val) & (uint32)0xffff0000UL) >> 16)))


static INLINE uint16
bcmswap16(uint16 val)
{
	return BCMSWAP16(val);
}

static INLINE uint32
bcmswap32(uint32 val)
{
	return BCMSWAP32(val);
}

static INLINE uint32
bcmswap32by16(uint32 val)
{
	return BCMSWAP32BY16(val);
}

/* buf	- start of buffer of shorts to swap */
/* len  - byte length of buffer */
static INLINE void
bcmswap16_buf(uint16 *buf, uint len)
{
	len = len/2;

	while (len--) {
		*buf = bcmswap16(*buf);
		buf++;
	}
}

#ifndef hton16
#ifndef IL_BIGENDIAN
#define HTON16(i) BCMSWAP16(i)
#define NTOH16(i) BCMSWAP16(i)
#define HTON32(i) BCMSWAP32(i)
#define NTOH32(i) BCMSWAP32(i)
#define	hton16(i) bcmswap16(i)
#define	hton32(i) bcmswap32(i)
#define	ntoh16(i) bcmswap16(i)
#define	ntoh32(i) bcmswap32(i)
#define ltoh16(i) (i)
#define ltoh32(i) (i)
#define htol16(i) (i)
#define htol32(i) (i)
#else
#define HTON16(i) (i)
#define NTOH16(i) (i)
#define HTON32(i) (i)
#define NTOH32(i) (i)
#define	hton16(i) (i)
#define	hton32(i) (i)
#define	ntoh16(i) (i)
#define	ntoh32(i) (i)
#define	ltoh16(i) bcmswap16(i)
#define	ltoh32(i) bcmswap32(i)
#define htol16(i) bcmswap16(i)
#define htol32(i) bcmswap32(i)
#endif /* IL_BIGENDIAN */
#endif /* hton16 */

#ifndef IL_BIGENDIAN
#define ltoh16_buf(buf, i)
#define htol16_buf(buf, i)
#else
#define ltoh16_buf(buf, i) bcmswap16_buf((uint16*)buf, i)
#define htol16_buf(buf, i) bcmswap16_buf((uint16*)buf, i)
#endif /* IL_BIGENDIAN */

/*
* Store 16-bit value to unaligned little endian byte array.
*/
static INLINE void
htol16_ua_store(uint16 val, uint8 *bytes)
{
	bytes[0] = val&0xff;
	bytes[1] = val>>8;
}

/*
* Store 32-bit value to unaligned little endian byte array.
*/
static INLINE void
htol32_ua_store(uint32 val, uint8 *bytes)
{
	bytes[0] = val&0xff;
	bytes[1] = (val>>8)&0xff;
	bytes[2] = (val>>16)&0xff;
	bytes[3] = val>>24;
}

/*
* Store 16-bit value to unaligned network(big) endian byte array.
*/
static INLINE void
hton16_ua_store(uint16 val, uint8 *bytes)
{
	bytes[1] = val&0xff;
	bytes[0] = val>>8;
}

/*
* Store 32-bit value to unaligned network(big) endian byte array.
*/
static INLINE void
hton32_ua_store(uint32 val, uint8 *bytes)
{
	bytes[3] = val&0xff;
	bytes[2] = (val>>8)&0xff;
	bytes[1] = (val>>16)&0xff;
	bytes[0] = val>>24;
}

#define _LTOH16_UA(cp)	((cp)[0] | ((cp)[1] << 8))
#define _LTOH32_UA(cp)	((cp)[0] | ((cp)[1] << 8) | ((cp)[2] << 16) | ((cp)[3] << 24))
#define _NTOH16_UA(cp)	(((cp)[0] << 8) | (cp)[1])
#define _NTOH32_UA(cp)	(((cp)[0] << 24) | ((cp)[1] << 16) | ((cp)[2] << 8) | (cp)[3])

/*
 * Load 16-bit value from unaligned little-endian byte array.
 */
static INLINE uint16
ltoh16_ua(const void *bytes)
{
	return _LTOH16_UA((const uint8 *)bytes);
}

/*
 * Load 32-bit value from unaligned little-endian byte array.
 */
static INLINE uint32
ltoh32_ua(const void *bytes)
{
	return _LTOH32_UA((const uint8 *)bytes);
}

/*
 * Load 16-bit value from unaligned big-(network-)endian byte array.
 */
static INLINE uint16
ntoh16_ua(const void *bytes)
{
	return _NTOH16_UA((const uint8 *)bytes);
}

/*
 * Load 32-bit value from unaligned big-(network-)endian byte array.
 */
static INLINE uint32
ntoh32_ua(const void *bytes)
{
	return _NTOH32_UA((const uint8 *)bytes);
}

#define ltoh_ua(ptr) \
	(sizeof(*(ptr)) == sizeof(uint8) ? *(const uint8 *)ptr : \
	 sizeof(*(ptr)) == sizeof(uint16) ? _LTOH16_UA((const uint8 *)ptr) : \
	 sizeof(*(ptr)) == sizeof(uint32) ? _LTOH32_UA((const uint8 *)ptr) : \
	 0xfeedf00d)

#define ntoh_ua(ptr) \
	(sizeof(*(ptr)) == sizeof(uint8) ? *(const uint8 *)ptr : \
	 sizeof(*(ptr)) == sizeof(uint16) ? _NTOH16_UA((const uint8 *)ptr) : \
	 sizeof(*(ptr)) == sizeof(uint32) ? _NTOH32_UA((const uint8 *)ptr) : \
	 0xfeedf00d)


#endif /* _BCMENDIAN_H_ */
