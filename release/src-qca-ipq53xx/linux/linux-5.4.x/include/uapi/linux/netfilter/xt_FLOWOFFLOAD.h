/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _XT_FLOWOFFLOAD_H
#define _XT_FLOWOFFLOAD_H

#include <linux/types.h>

enum {
	XT_FLOWOFFLOAD_HW	= 1 << 0,

	XT_FLOWOFFLOAD_MASK	= XT_FLOWOFFLOAD_HW
};

struct xt_flowoffload_target_info {
	__u32 flags;
};

#endif /* _XT_FLOWOFFLOAD_H */
