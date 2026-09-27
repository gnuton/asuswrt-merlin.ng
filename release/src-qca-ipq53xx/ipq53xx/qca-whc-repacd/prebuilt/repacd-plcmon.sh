#!/bin/sh
# Copyright (c) 2017 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

PLCMON_DEBUG_OUTOUT=0

PLCMON_LINK_METRICS='/var/run/plcmetrics'
PLCMON_UDP_EFFICIENCY="65"
PLCMON_TX="TX"
PLCMON_BDA="BDA"
PLCMON_BDA_NONE="FF:FF:FF:FF:FF:FF"
PLCMON_REMOTE=":Remote"

# PLC config parameters
plc_enabled=0 plc_ifname="" plc_host_speed=0

# Emit a message at debug level.
# input: $1 - the message to log
__repacd_plcmon_debug() {
    local stderr=''
    if [ "$PLCMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.plcmon -p user.debug "$1"
}

# Emit a message at info level.
__repacd_plcmon_info() {
    local stderr=''
    if [ "$PLCMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.plcmon -p user.info "$1"
}

# Bringup PLC interface
# input: None
# output: None
__repacd_plcmon_bring_plc_iface_up() {
    if [ ! -z "$plc_ifname" ]; then
        ifconfig $plc_ifname up
        __repacd_plcmon_debug "PLC interface ($plc_ifname) Brought up"
    fi
}

# Bringdown PLC interface
# input: None
# output: None
__repacd_plcmon_bring_plc_iface_down() {
    if [ ! -z "$plc_ifname" ]; then
        ifconfig $plc_ifname down
        __repacd_plcmon_debug "PLC interface ($plc_ifname) Brought down"
    fi

    __repacd_plcmon_get_timestamp force_down_plc_timestamp
}

# Check whether PLC is enabled in configuration file
# output: 0 - success; if configured
#         1 - failure; if not configured
__repacd_plcmon_is_plc_enabled() {
    if [ "$plc_enabled" -eq 0 ]; then
        return 1
    else
        return 0
    fi
}

repacd_plcmon_init() {
    config_load plc
    config_get plc_enabled "config" "Enabled" '0'
    config_get plc_ifname "config" "PlcIfname" ""

    config_load hyd
    config_get plc_host_speed "PathChPlc" "HostPLCInterfaceSpeed" '1000'
}

# Measure the PLC link rate
# output: $1 - The supported PLC Link rate
# Returns: 0, success -- if PLC device is connected to CAP
#          1, failure -- if CAP MAC address is not found in any of the Remote
#                        devices BDA address
__repacd_plcmon_measure_plc_rate() {
    local plc_iface_tx_rate=0
    local is_w_bda=0
    local is_w_bdanone=0
    local found_cap_mac=0
    local found_remote_devices=0

    # Get the PLC Link Metrics
    /usr/sbin/plchost -i br-lan -m > $PLCMON_LINK_METRICS

    plc_iface_tx_rate=0
    # Parse every line in the the file PLCMON_LINK_METRICS
    while IFS= read -r line; do
        for w in $line; do
            # Check if Bridged DA is found,
            # i.e., connected to host
            if [ "$w" = $PLCMON_BDA ]; then
                is_w_bda=1
            fi
            if [ $is_w_bda -gt 0 ]; then
                if echo $w | grep -i "$gw_mac" > /dev/null; then
                    found_cap_mac=1
                    is_w_bda=0
                fi
            fi
            if [ "$w" = $PLCMON_REMOTE ]; then
                found_remote_devices=$((found_remote_devices+1))
            fi
            if [ $found_cap_mac -gt 0 ]; then
                if [ "$w" = $PLCMON_TX ]; then
                    plc_iface_tx_rate=$(echo $line | tr -dc '0-9')
                    plc_iface_tx_rate=$(echo $plc_iface_tx_rate | sed 's/^0*//')
                    found_cap_mac=0
                    break
                fi
            fi
        done
    done < $PLCMON_LINK_METRICS

    if [ -z "${plc_iface_tx_rate}" ]; then
        plc_iface_tx_rate=0
    fi

    if [ "$plc_iface_tx_rate" -gt 0 ]; then
        plc_iface_tx_rate=$((($plc_iface_tx_rate*$PLCMON_UDP_EFFICIENCY)/100))
        if [ "$plc_iface_tx_rate" -ge "$plc_host_speed" ]; then
            plc_iface_tx_rate=$plc_host_speed
        fi
    fi
    eval "$1=$plc_iface_tx_rate"

    if [ "$found_remote_devices" -gt 0 ]; then
        return 0
    else
        return 1
    fi
}

# Check whether PLC device is connected
# output: 0 - success; if MME return is successful
#         1 - failure; if MME return is not successful
__repacd_plcmon_is_plc_connected() {
    local plc_tx_rate=0

    if __repacd_plcmon_measure_plc_rate plc_tx_rate; then
        eval "$1=$plc_tx_rate"
        return 0
    else
        return 1
    fi
}
