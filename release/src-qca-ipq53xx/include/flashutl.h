/*
 * BCM47XX FLASH driver interface
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 * $Id: flashutl.h,v 13.10 2005/09/07 00:57:49 Exp $
 */

#ifndef _flashutl_h_
#define _flashutl_h_


#ifndef _LANGUAGE_ASSEMBLY

int	sysFlashInit(char *flash_str);
int sysFlashRead(uint off, uchar *dst, uint bytes);
int sysFlashWrite(uint off, uchar *src, uint bytes);
void nvWrite(unsigned short *data, unsigned int len);
void nvWriteChars(unsigned char *data, unsigned int len);

#endif	/* _LANGUAGE_ASSEMBLY */

#endif /* _flashutl_h_ */
