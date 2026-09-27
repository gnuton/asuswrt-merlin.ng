/* Copyright (c) 2020 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MINIDUMP_H
#define __MINIDUMP_H

#include <soc/qcom/ctx-save.h>

/* TLV_Types */
typedef enum {
    QCA_WDT_LOG_DUMP_TYPE_INVALID,
    QCA_WDT_LOG_DUMP_TYPE_UNAME,
    QCA_WDT_LOG_DUMP_TYPE_DMESG,
    QCA_WDT_LOG_DUMP_TYPE_LEVEL1_PT,
    QCA_WDT_LOG_DUMP_TYPE_WLAN_MOD,
    QCA_WDT_LOG_DUMP_TYPE_WLAN_MOD_DEBUGFS,
    QCA_WDT_LOG_DUMP_TYPE_WLAN_MOD_INFO,
    QCA_WDT_LOG_DUMP_TYPE_WLAN_MMU_INFO,
    QCA_WDT_LOG_DUMP_TYPE_EMPTY,
} minidump_wdt_tlv_type_t;

#endif
