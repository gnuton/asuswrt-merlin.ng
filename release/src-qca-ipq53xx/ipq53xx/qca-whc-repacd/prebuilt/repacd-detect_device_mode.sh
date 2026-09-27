#!/bin/sh
# Copyright (c) 2016 Qualcomm Atheros, Inc.
#
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.

. /lib/functions/repacd-netdet.sh

netdet_wan_lan_detect
case $? in
    $NETDET_RESULT_ROOTAP)
        __netdet_debug "we have a WAN link so configure as CAP"
        (export EVENT="location"; export STATE=cap; hotplug-call edge)
        ;;
    $NETDET_RESULT_RE)
        __netdet_debug "we see a WiFi SON device upstream so configure as range extender"
        (export EVENT="location"; export STATE=re; hotplug-call edge)
        ;;
    $NETDET_RESULT_INDETERMINATE)
        __netdet_debug "We can't tell so don't do anything"
        (export EVENT="location"; export STATE=unknown; hotplug-call edge)
        ;;
    *)
        echo "error: unknown return code: $?" >&2
        exit 4
        ;;
esac
