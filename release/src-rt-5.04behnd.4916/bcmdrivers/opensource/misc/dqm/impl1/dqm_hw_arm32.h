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
#ifndef __DQM_HW_ARM32_H
#define __DQM_HW_ARM32_H

#if defined(CONFIG_FRAME_POINTER)

/* used for tracing debug */

static inline void __dqm_rx_msgs(u32 *reg, int msgsz, int count,
			       struct msgstruct *msg)
{
	dqm_rx_msgs_noburst(reg, msgsz, count, msg);
}

#else

/* arm32 multiword burst instructions */

#define LDM_2(addr) \
	__asm__("ldm   %0, {%1, %2}" \
		: "=r" (addr), "=r" (a0), "=r" (a1) \
		: "0" (addr))

#define LDM_3(addr) \
	__asm__("ldm   %0, {%1, %2, %3}" \
		: "=r" (addr), "=r" (a0), "=r" (a1), "=r" (a2) \
		: "0" (addr))

#define LDM_4(addr) \
	__asm__("ldm   %0, {%1, %2, %3, %4}" \
		: "=r" (addr), "=r" (a0), "=r" (a1), "=r" (a2), "=r" (a3) \
		: "0" (addr))

#define LDM_5(addr) \
	__asm__("ldm   %0, {%1, %2, %3, %4, %5}" \
		: "=r" (addr), \
		  "=r" (a0), "=r" (a1), "=r" (a2), "=r" (a3), "=r" (a4) \
		: "0" (addr))

#define LDM_6(addr) \
	__asm__("ldm   %0, {%1, %2, %3, %4, %5, %6}" \
		: "=r" (addr), \
		  "=r" (a0), "=r" (a1), "=r" (a2), "=r" (a3),	\
		  "=r" (a4), "=r" (a5) \
		: "0" (addr))

#define LDM_7(addr) \
	__asm__("ldm   %0, {%1, %2, %3, %4, %5, %6, %7}" \
		: "=r" (addr), \
		  "=r" (a0), "=r" (a1), "=r" (a2), "=r" (a3),	\
		  "=r" (a4), "=r" (a5), "=r" (a6) \
		: "0" (addr))

#define LDM_8(addr) \
	__asm__("ldm   %0, {%1, %2, %3, %4, %5, %6, %7, %8}" \
		: "=r" (addr), \
		  "=r" (a0), "=r" (a1), "=r" (a2), "=r" (a3),	\
		  "=r" (a4), "=r" (a5), "=r" (a6), "=r" (a7)	\
		: "0" (addr))

static inline void dqm_msg_read1(u32 *reg,  int msgsz, struct msgstruct *msg)
{
	u32 val = __raw_readl(reg);

	msg->msgdata[0] = val;
}

static inline void dqm_msg_read2(u32 *reg,  int msgsz, struct msgstruct *msg)
{
	register unsigned int a0 __asm__("r4");
	register unsigned int a1 __asm__("r5");
	LDM_2(reg);
	/* msgsz can be 1 or 2 */
	msg[0].msgdata[0] = a0;
	if (msgsz == 1)
		msg[1].msgdata[0] = a1;
	else	/* must be 2 */
		msg[0].msgdata[1] = a1;
}

static inline void dqm_msg_read3(u32 *reg,  int msgsz, struct msgstruct *msg)
{
	register unsigned int a0 __asm__("r4");
	register unsigned int a1 __asm__("r5");
	register unsigned int a2 __asm__("r6");
	LDM_3(reg);
	/* msgsz can be 1 or 3 */
	msg[0].msgdata[0] = a0;
	if (msgsz == 1) {
		msg[1].msgdata[0] = a1;
		msg[2].msgdata[0] = a2;
	} else {	/* must be 3 */
		msg[0].msgdata[1] = a1;
		msg[0].msgdata[2] = a2;
	}
}

static inline void dqm_msg_read4(u32 *reg,  int msgsz, struct msgstruct *msg)
{
	register unsigned int a0 __asm__("r4");
	register unsigned int a1 __asm__("r5");
	register unsigned int a2 __asm__("r6");
	register unsigned int a3 __asm__("r7");
	LDM_4(reg);
	msg->msgdata[0] = a0;
	if (msgsz == 1) {
		msg[1].msgdata[0] = a1;
		msg[2].msgdata[0] = a2;
		msg[3].msgdata[0] = a3;
	} else if (msgsz == 2) {
		msg[0].msgdata[1] = a1;
		msg[1].msgdata[0] = a2;
		msg[1].msgdata[1] = a3;
	} else { /* 4 */
		msg->msgdata[1] = a1;
		msg->msgdata[2] = a2;
		msg->msgdata[3] = a3;
	}
}

static inline void dqm_msg_read5(u32 *reg, int msgsz, struct msgstruct *msg)
{
	register unsigned int a0 __asm__("r4");
	register unsigned int a1 __asm__("r5");
	register unsigned int a2 __asm__("r6");
	register unsigned int a3 __asm__("r7");
	register unsigned int a4 __asm__("r8");
	LDM_5(reg);
	/* msgsz must be one. Nothing else is possible */
	msg[0].msgdata[0] = a0;
	msg[1].msgdata[0] = a1;
	msg[2].msgdata[0] = a2;
	msg[3].msgdata[0] = a3;
	msg[4].msgdata[0] = a4;
}

static inline void dqm_msg_read6(u32 *reg,  int msgsz, struct msgstruct *msg)
{
	register unsigned int a0 __asm__("r4");
	register unsigned int a1 __asm__("r5");
	register unsigned int a2 __asm__("r6");
	register unsigned int a3 __asm__("r7");
	register unsigned int a4 __asm__("r8");
	register unsigned int a5 __asm__("r9");
	LDM_6(reg);
	msg->msgdata[0] = a0;
	if (msgsz == 1) {
		msg[1].msgdata[0] = a1;
		msg[2].msgdata[0] = a2;
		msg[3].msgdata[0] = a3;
		msg[4].msgdata[0] = a4;
		msg[5].msgdata[0] = a5;
	} else if (msgsz == 2) {
		msg[0].msgdata[1] = a1;
		msg[1].msgdata[0] = a2;
		msg[1].msgdata[1] = a3;
		msg[2].msgdata[0] = a4;
		msg[2].msgdata[1] = a5;
	} else {	/* must be 3 */
		msg[0].msgdata[1] = a1;
		msg[0].msgdata[2] = a2;
		msg[1].msgdata[0] = a3;
		msg[1].msgdata[1] = a4;
		msg[1].msgdata[2] = a5;
	}
}

static inline void dqm_msg_read7(u32 *reg, int msgsz, struct msgstruct *msg)
{
	register unsigned int a0 __asm__("r4");
	register unsigned int a1 __asm__("r5");
	register unsigned int a2 __asm__("r6");
	register unsigned int a3 __asm__("r7");
	register unsigned int a4 __asm__("r8");
	register unsigned int a5 __asm__("r9");
	register unsigned int a6 __asm__("r10");
	LDM_7(reg);
	/* msgsz must be one. Nothing else is possible */
	msg[0].msgdata[0] = a0;
	msg[1].msgdata[0] = a1;
	msg[2].msgdata[0] = a2;
	msg[3].msgdata[0] = a3;
	msg[4].msgdata[0] = a4;
	msg[5].msgdata[0] = a5;
	msg[6].msgdata[0] = a6;
}

static inline void dqm_msg_read8(u32 *reg,  int msgsz, struct msgstruct *msg)
{
	register unsigned int a0 __asm__("r4");
	register unsigned int a1 __asm__("r5");
	register unsigned int a2 __asm__("r6");
	register unsigned int a3 __asm__("r7");
	register unsigned int a4 __asm__("r8");
	register unsigned int a5 __asm__("r9");
	register unsigned int a6 __asm__("r10");
	register unsigned int a7 __asm__("r11");
	LDM_8(reg);
	msg->msgdata[0] = a0;
	if (msgsz == 1) {
		msg[1].msgdata[0] = a1;
		msg[2].msgdata[0] = a2;
		msg[3].msgdata[0] = a3;
		msg[4].msgdata[0] = a4;
		msg[5].msgdata[0] = a5;
		msg[6].msgdata[0] = a6;
		msg[7].msgdata[0] = a7;
	} else if (msgsz == 2) {
		msg[0].msgdata[1] = a1;
		msg[1].msgdata[0] = a2;
		msg[1].msgdata[1] = a3;
		msg[2].msgdata[0] = a4;
		msg[2].msgdata[1] = a5;
		msg[3].msgdata[0] = a6;
		msg[3].msgdata[1] = a7;
	} else  { /* 4 */
		msg[0].msgdata[1] = a1;
		msg[0].msgdata[2] = a2;
		msg[0].msgdata[3] = a3;
		msg[1].msgdata[0] = a4;
		msg[1].msgdata[1] = a5;
		msg[1].msgdata[2] = a6;
		msg[1].msgdata[3] = a7;
	}
}

static inline void dqm_rx_msgs(u32 *reg, int msgsz, int count,
			       struct msgstruct *msg)
{
	int left = count;

	while (left) {
		int words = left * msgsz;
		int messages = left;

		if (words > 8) {
			if (unlikely(msgsz == 3)) {
				words = 6;
				messages = 2;
			} else {
				words = 8;
				/* (8 / msgsz) optimized for
				 * msgsz 1,2, or 4
				 */
				BUG_ON(msgsz > 4);
				messages = 8 >> (msgsz >> 1);
			}
		}
		switch (words) {
		case 1:
			dqm_msg_read1(reg, msgsz, msg);
			break;
		case 2:
			dqm_msg_read2(reg, msgsz, msg);
			break;
		case 3:
			dqm_msg_read3(reg, msgsz, msg);
			break;
		case 4:
			dqm_msg_read4(reg, msgsz, msg);
			break;
		case 5:
			dqm_msg_read5(reg, msgsz, msg);
			break;
		case 6:
			dqm_msg_read6(reg, msgsz, msg);
			break;
		case 7:
			dqm_msg_read7(reg, msgsz, msg);
			break;
		case 8:
			dqm_msg_read8(reg, msgsz, msg);
			break;
		}
		left -= messages;
		msg += messages;
	}
}

#endif

#endif
