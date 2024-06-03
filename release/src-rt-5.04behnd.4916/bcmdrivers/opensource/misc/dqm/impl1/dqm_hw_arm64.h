/****************************************************************************
 *
 * Broadcom Proprietary and Confidential.
 * (c) 2020 Broadcom. All rights reserved.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to
 * you under the terms of the GNU General Public License version 2 (the
 * "GPL"), available at [http://www.broadcom.com/licenses/GPLv2.php], with
 * the following added to such license:
 *
 * As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy
 * and distribute the resulting executable under terms of your choice,
 * provided that you also meet, for each linked independent module, the
 * terms and conditions of the license of that module. An independent
 * module is a module which is not derived from this software. The special
 * exception does not apply to any modifications of the software.
 *
 * Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a
 * license other than the GPL, without Broadcom's express prior written
 * consent.
 *
 ****************************************************************************
 * Author: Peter Sulc <peter.sulc@broadcom.com>
 *****************************************************************************/
#ifndef __DQM_HW_ARM64_H
#define __DQM_HW_ARM64_H

#if defined(CONFIG_KERNEL_MODE_NEON)

#include <asm/neon.h>

static inline int dqm_noburst_rx(u32 *reg, int msgsz, int count,
				 struct msgstruct *msg)
{
	int i,j;

	for (i = 0; i < count; i++) {
		for (j = 0; j < msgsz; j++) {
			msg[i].msgdata[j] = dqm_reg_read(&reg[j]);
		}
		if (!msg[i].msgdata[0]) {
			msg[i].msglen = 0;
			return i;
		}
		msg[i].msglen = msgsz;
	}
	return count;
}

static inline int _dqm_neon_rx_1(u32 *reg, int count, struct msgstruct *msg)
{
	int sz = sizeof(struct msgstruct);
	int nread = 0;
	u32 word1;
	u32 *dqm = reg;
	u32 *data = &(msg[0].msgdata[0]);
	u32 *dlen = &(msg[0].msglen);

	__asm__("	movi	v1.4s,  #1		;"
		"1:	ld1	{v0.4s},	%[dqm]	;"
		"	mov	%w[word1],	v0.s[0]	;"
		"	cbz	%w[word1],	2f	;"
		"	st1	{v0.s}[0],	%[data]	;"
		"	st1	{v1.s}[0],	%[dlen]	;"
		"	add	%[nread],	%[nread], 1	;"
		"	add	%[rdata],	%[rdata], %[sz];"
		"	add	%[rdlen],	%[rdlen], %[sz];"
		"	sub	%[count],	%[count], 1	;"
		"	cbz	%[count],	2f	;"
		"	mov	%w[word1],	v0.s[1]	;"
		"	cbz	%w[word1],	2f	;"
		"	st1	{v0.s}[1],	%[data]	;"
		"	st1	{v1.s}[0],	%[dlen]	;"
		"	add	%[nread],	%[nread], 1	;"
		"	add	%[rdata],	%[rdata], %[sz];"
		"	add	%[rdlen],	%[rdlen], %[sz];"
		"	sub	%[count],	%[count], 1	;"
		"	cbz	%[count], 	2f	;"
		"	mov	%w[word1],	v0.s[2]	;"
		"	cbz	%w[word1],	2f	;"
		"	st1	{v0.s}[2],	%[data]	;"
		"	st1	{v1.s}[0],	%[dlen]	;"
		"	add	%[nread],	%[nread], 1	;"
		"	add	%[rdata],	%[rdata], %[sz];"
		"	add	%[rdlen],	%[rdlen], %[sz];"
		"	sub	%[count],	%[count], 1	;"
		"	cbz	%[count], 	2f	;"
		"	mov	%w[word1],	v0.s[3]	;"
		"	cbz	%w[word1],	2f	;"
		"	st1	{v0.s}[3],	%[data]	;"
		"	st1	{v1.s}[0],	%[dlen]	;"
		"	add	%[nread],	%[nread], 1	;"
		"	add	%[rdata],	%[rdata], %[sz];"
		"	add	%[rdlen],	%[rdlen], %[sz];"
		"	sub	%[count],	%[count], 1	;"
		"	cbz	%[count], 	2f	;"
		"	b	1b			;"
		"2:	;"
		:	[data]	"=Q"(*data),
			[dlen]	"=Q"(*dlen),
			[count] "+r"(count),
			[word1] "=&r"(word1),
			[nread]	"+r"(nread)
		:	[dqm]	"rm"(*dqm),
			[rdata] "r"(data),
			[rdlen] "r"(dlen),
			[sz]	"r"(sz)
		:);
	return nread;
}

static inline int _dqm_neon_rx_2(u32 *reg, int count, struct msgstruct *msg)
{
	int sz = sizeof(struct msgstruct);
	int nread = 0;
	u32 word1;
	u32 *dqm = reg;
	u32 *data = &(msg[0].msgdata[0]);
	u32 *dlen = &(msg[0].msglen);

	__asm__("	movi	v1.4s,  #2		;"
		"1:	ld1	{v0.4s},	%[dqm]	;"
		"	mov	%w[word1],	v0.s[0]	;"
		"	cbz	%w[word1],	2f	;"
		"	st1	{v0.d}[0],	%[data]	;"
		"	st1	{v1.s}[0],	%[dlen]	;"
		"	add	%[nread],	%[nread], 1	;"
		"	add	%[rdata],	%[rdata], %[sz];"
		"	add	%[rdlen],	%[rdlen], %[sz];"
		"	sub	%[count],	%[count], 1	;"
		"	cbz	%[count],	2f	;"
		"	mov	%w[word1],	v0.s[2]	;"
		"	cbz	%w[word1],	2f	;"
		"	st1	{v0.d}[1],	%[data]	;"
		"	st1	{v1.s}[0],	%[dlen]	;"
		"	add	%[nread],	%[nread], 1	;"
		"	add	%[rdata],	%[rdata], %[sz];"
		"	add	%[rdlen],	%[rdlen], %[sz];"
		"	sub	%[count],	%[count], 1	;"
		"	cbz	%[count], 	2f	;"
		"	b	1b			;"
		"2:	;"
		:	[data]	"=Q"(*data),
			[dlen]	"=Q"(*dlen),
			[count] "+r"(count),
			[word1] "=&r"(word1),
			[nread]	"+r"(nread)
		:	[dqm]	"rm"(*dqm),
			[rdata] "r"(data),
			[rdlen] "r"(dlen),
			[sz]	"r"(sz)
		:);
	return nread;
}

static inline int _dqm_neon_rx_3(u32 *reg, int count, struct msgstruct *msg)
{
	int num_read = 0;
	int sz = sizeof(struct msgstruct);
	int valid;
	u32 word1;
	u32 *dqm = reg;
	u32 *data = &(msg[0].msgdata[0]);
	u32 *dlen = &(msg[0].msglen);

	__asm__("	movi	v3.4s,  #3		;" :::);

	while (count >= 4) {
		__asm__("	mov	%x[valid], 	xzr	;"
			"	ld1	{v0.4s},  %[dqm]	;"
			"	st1	{v0.4s},  %[data]	;"
			"	st1	{v3.s}[0],%[dlen]	;"
			"	mov	%w[word1], v0.s[0]	;"
			"	cbz	%w[word1], 2f		;"
			"	add	%[valid], %[valid], 1	;"
			"	add	%[rdata], %[rdata], %[sz];"
			"	add	%[rdlen], %[rdlen], %[sz];"
			"	mov	%w[word1], v0.s[3]	;"
			"	cbz	%w[word1], 2f		;"
			"	ld1	{v1.4s},  %[dqm]	;"
			"       mov     v0.s[0], %w[word1]	;"
			"	mov     v0.s[1], v1.s[0]	;"
			"	mov     v0.s[2], v1.s[1]	;"
			"	st1	{v0.4s},  %[data]	;"
			"	st1	{v3.s}[0],%[dlen]	;"
			"	add	%[valid], %[valid], 1	;"
			"	add	%[rdata], %[rdata], %[sz];"
			"	add	%[rdlen], %[rdlen], %[sz];"
			"	mov	%w[word1], v1.s[2]	;"
			"	cbz	%w[word1], 2f		;"
			"	ld1	{v2.4s},  %[dqm]	;"
			"	mov     v0.s[0], v1.s[2]	;"
			"	mov     v0.s[1], v1.s[3]	;"
			"	mov     v0.s[2], v2.s[0]	;"
			"	st1	{v0.4s},  %[data]	;"
			"	st1	{v3.s}[0],%[dlen]	;"
			"	add	%[valid], %[valid], 1	;"
			"	add	%[rdata], %[rdata], %[sz];"
			"	add	%[rdlen], %[rdlen], %[sz];"
			"	mov	%w[word1], v2.s[1]	;"
			"	cbz	%w[word1], 2f		;"
			"	mov     v0.s[0], v2.s[1]	;"
			"	mov     v0.s[1], v2.s[2]	;"
			"	mov     v0.s[2], v2.s[3]	;"
			"	st1	{v0.4s},  %[data]	;"
			"	st1	{v3.s}[0],%[dlen]	;"
			"	add	%[valid], %[valid], 1	;"
			"	add	%[rdata], %[rdata], %[sz];"
			"	add	%[rdlen], %[rdlen], %[sz];"
			"2:	;"
			:	[data]	"=Q"(*data),
				[dlen]	"=Q"(*dlen),
				[word1] "=&r"(word1),
				[valid] "=r"(valid)
			:	[dqm]	"rm"(*dqm),
				[rdata] "r"(data),
				[rdlen] "r"(dlen),
				[sz]	"r"(sz)
			:);
		num_read += valid;
		if (valid < 4)
			return num_read;
		count -= valid;
	}
	while (count) {
		num_read += dqm_noburst_rx(reg, 3, count, msg + num_read);
	}
	return num_read;
}

static inline int _dqm_neon_rx_4x1(u32 *reg, int count, struct msgstruct *msg)
{
	int sz = sizeof(struct msgstruct);
	int nread = 0;
	u32 word1;
	u32 *dqm = reg;
	u32 *data = &(msg[0].msgdata[0]);
	u32 *dlen = &(msg[0].msglen);

	__asm__("	movi	v1.4s,  #4		;"
		"1:	ld1	{v0.4s},  %[dqm]	;"
		"	st1	{v0.4s},  %[data]	;"
		"	st1	{v1.s}[0],%[dlen]	;"
		"	mov	%w[word1], v0.s[0]	;"
		"	cbz	%w[word1], 2f		;"
		"	add	%[nread], %[nread], 1	;"
		"	add	%[rdata], %[rdata], %[sz];"
		"	add	%[rdlen], %[rdlen], %[sz];"
		"	sub	%[count], %[count], 1	;"
		"	cbz	%[count], 2f		;"
		"	b	1b			;"
		"2:	;"
		:	[data]	"=Q"(*data),
			[dlen]	"=Q"(*dlen),
			[count] "+r"(count),
			[word1] "=&r"(word1),
			[nread]	"+r"(nread)
		:	[dqm]	"rm"(*dqm),
			[rdata] "r"(data),
			[rdlen] "r"(dlen),
			[sz]	"r"(sz)
		:);
	return nread;
}

static inline int _dqm_neon_rx_4x4(u32 *reg, int count, struct msgstruct *msg)
{
	int num_read = 0;
	int sz = sizeof(struct msgstruct);
	int valid;
	u32 word1;
	u32 *dqm = reg;
	u32 *data = &(msg[0].msgdata[0]);
	u32 *dlen = &(msg[0].msglen);

	__asm__("	movi	v4.4s,  #4		;" :::);

	while (count >= 4) {
		__asm__("	mov	%x[valid], 	xzr	;"
			"	ld1	{v0.4s},	%[dqm]	;"
			"	ld1	{v1.4s},	%[dqm]	;"
			"	ld1	{v2.4s},	%[dqm]	;"
			"	ld1	{v3.4s},	%[dqm]	;"
			"	mov	%w[word1],	v0.4s[0];"
			"	cbz	%w[word1],	1f	;"
			"	st1	{v0.4s},	%[data]	;"
			"	st1	{v4.s}[0],	%[dlen]	;"
			"	add	%x[valid], %x[valid], 1	;"
			"	add	%x[datar], %[datar], %x[sz];"
			"	add	%x[dlenr], %[dlenr], %x[sz];"
			"1:	mov	%w[word1],	v1.4s[0];"
			"	cbz	%w[word1],	2f	;"
			"	st1	{v1.4s},	%[data]	;"
			"	st1	{v4.s}[0],	%[dlen]	;"
			"	add	%x[valid], %x[valid], 1	;"
			"	add	%x[datar], %x[datar], %x[sz];"
			"	add	%x[dlenr], %x[dlenr], %x[sz];"
			"2:	mov	%w[word1],	v2.4s[0];"
			"	cbz	%w[word1],	3f	;"
			"	st1	{v2.4s},	%[data]	;"
			"	st1	{v4.s}[0],	%[dlen]	;"
			"	add	%x[valid], %x[valid], 1	;"
			"	add	%x[datar], %x[datar], %x[sz];"
			"	add	%x[dlenr], %x[dlenr], %x[sz];"
			"3:	mov	%w[word1],      v3.4s[0];"
			"	cbz	%w[word1],	4f	;"
			"	st1	{v3.4s},	%[data]	;"
			"	st1	{v4.s}[0],	%[dlen]	;"
			"	add	%x[valid], %x[valid], 1	;"
			"	add	%x[datar], %x[datar], %x[sz];"
			"	add	%x[dlenr], %x[dlenr], %x[sz];"
			"4:	;"
			:	[data]	"=Q"(*data),
				[dlen]	"=Q"(*dlen),
				[word1] "=r"(word1),
				[valid] "=r"(valid)
			:	[dqm]	"Q"(*dqm),
				[sz]	"r"(sz),
				[dlenr] "r"(dlen),
				[datar] "r"(data)
			:	"cc");
		num_read += valid;
		if (valid < 4)
			return num_read;
		count -= 4;
	}
	if (count)
		num_read += _dqm_neon_rx_4x1(reg, count, msg + num_read);
	return num_read;
}

static inline void _dqm_neon_tx_4(u32 *reg, u32 *data)
{
	__asm__("	ld1	{v0.4s},	%[data]	;"
		"	st1	{v0.4s},	%[reg]	;"
       		:	[reg]	"=Q"(*reg)
		:	[data]	"Q"(*data)
		:	"cc");
}

static inline void dqm_kernel_neon_begin(void)
{
	if (!in_interrupt())
		preempt_disable();
}		
	
static inline void dqm_kernel_neon_end(void)
{
	if (!in_interrupt())
		preempt_enable();
}	

#endif /* neon */

static inline int _dqm_arm_rx_4(u32 *reg, int count, struct msgstruct *msg)
{
	unsigned long long r1, r2;
	int num_read = 0;
	u32 *out;
	u32 word1;
	u32 *in = reg;

	while (count) {
		out = msg[num_read].msgdata;
		__asm__("	ldp	%x[r1], %x[r2],	%[in]	;"
			"	stp	%x[r1], %x[r2],	%[out]	;"
			"	mov	%w[word1], %w[r1]	;"
			:	[out]	"=Q"(*out),
				[word1] "=r"(word1),
				[r1]	"=r"(r1),
				[r2]	"=r"(r2)
			:	[in]	"Q"(*in)
			:	"cc");
		if (word1 == 0) {
			msg[num_read].msglen = 0;
			break;
		}
		msg[num_read].msglen = 4;
		num_read++;
		count--;
	}
	return num_read;
}

static inline int _dqm_arm_rx_2(u32 *reg, int count, struct msgstruct *msg)
{
	unsigned long r1, r2;
	int num_read = 0;
	u32 *out;
	u32 word1;
	u32 *in = reg;

	while (count) {
		out = msg[num_read].msgdata;
		__asm__("	ldp	%w[r1], %w[r2],	%[in]	;"
			"	stp	%w[r1], %w[r2],	%[out]	;"
			"	mov	%w[word1], %w[r1]	;"
			:	[out]	"=Q"(*out),
				[word1] "=r"(word1),
				[r1]	"=r"(r1),
				[r2]	"=r"(r2)
			:	[in]	"Q"(*in)
			:	"cc");
		if (word1 == 0) {
			msg[num_read].msglen = 0;
			break;
		}
		msg[num_read].msglen = 2;
		num_read++;
		count--;
	}
	return num_read;
}

static inline void _dqm_arm_tx_4(u32 *reg, u32 *data)
{
	register unsigned long long r1, r2;

	__asm__("	ldp	%x[r1], %x[r2],	%[data]	;"
		"	stp	%x[r1], %x[r2],	%[reg]	;"
       		:	[reg]	"=Q"(*reg),
			[r1]	"=r"(r1),
			[r2]	"=r"(r2)
		:	[data]	"Q"(*data)
		:	"cc");
}

#endif
