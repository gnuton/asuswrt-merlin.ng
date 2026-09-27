#!/bin/sh
# Copyright (c) 2016 Qualcomm Atheros, Inc.
#
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.

ETHMON_DEBUG_OUTOUT=0

# State information
switch_state=''

# Emit a message at debug level.
# input: $1 - the message to log
__ethmon_debug() {
    local stderr=''
    if [ "$ETHMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.ethmon -p user.debug "$1"
}

# Given switch_state from `swconfig dev switch0 show | grep "link: port:"`
# and a port number, return the state of that port.
__ethmon_get_port_state() {
    local switch_state="$1"
    local port=$2
    if [ -z "$switch_state" ]; then
        echo "unknown"
        return
    fi
    echo "$switch_state" | while read line; do
        local curr_port=$(echo $line | awk '{ print $2 }' | cut -d: -f2)
        if [ $port -eq $curr_port ]; then
            local link_state=$(echo $line | awk '{ print $3 }' | cut -d: -f2)
            echo $link_state
            return
        fi
    done
}

# Check the status of the Ethernet links (link state, speed, duplex).
repacd_ethmon_check() {
    local new_state
    new_state=$(swconfig dev switch0 show | grep "link: port:" | grep -v "port:0")
    if [ "$new_state" != "$switch_state" ]; then
        # One or more of the switch ports have changed state
        echo "$new_state" | while read line; do
            local port=$(echo $line | awk '{ print $2 }' | cut -d: -f2)
            local new_link_state=$(echo $line | awk '{ print $3 }' | cut -d: -f2)
            local link_state=$(__ethmon_get_port_state "$switch_state" $port)
            if [ "$link_state" != "$new_link_state" ]; then
                __ethmon_debug "port $port state changed from $link_state to $new_link_state"
                (export PORT=$port; export EVENT="link"; export STATE=$new_link_state; hotplug-call switch)
            fi
        done
        switch_state="$new_state"
    fi
}
