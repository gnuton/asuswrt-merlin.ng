#!/bin/sh
# Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

FRONTHAULMGRMON_DEBUG_OUTOUT=0

# This file assumes the gwmon and wifimon (the appropriate sub-version)
# have already been sourced.
. /lib/functions/whc-network.sh

is_front_haul_VAPs_brought_down=0
bBSSes_forced_down=0

# Config Parameters
manage_fronthaul_and_backhaul_independently=0
fronthaul_VAPs_bringdown_time=0

# Local parameters
bbss_ifaces_24g='' bbss_ifaces_5g=''
fbss_ifaces_24g='' fbss_ifaces_5g=''

IS_GW_REACHABLE=0
GW_NOT_REACHABLE_TIMESTAMP=0

# Emit a message at debug level.
# input: $1 - the message to log
__repacd_fronthaulmgrmon_debug() {
    local stderr=''
    if [ "$FRONTHAULMGRMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.fronthaulmgrmon -p user.debug "$1"
}

# Emit a message at info level.
__repacd_fronthaulmgrmon_info() {
    local stderr=''
    if [ "$BACKHAULMGRMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.fronthaulmgrmon -p user.info "$1"
}

__repacd_fronthaulmgrmon_get_fronthaul_vaps() {
    local config="$1"
    local network_to_match="$2"
    local network iface disabled mode device hwmode

    config_get network "$config" network
    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get bssid "$config" bssid
    config_get device "$config" device
    config_get hwmode "$device" hwmode
    config_get bss_type "$config" MapBSSType ''

    if [ "$hwmode" != "11ad" ]; then
        if [ "$network" = "$network_to_match" ] && [ -n "$iface" ] && \
			[ "$mode" = "ap" ] && [ "$disabled" -eq 0 ]; then

            if whc_is_5g_vap "$config"; then
                __repacd_fronthaulmgrmon_debug "5 GHz AP VAP ($iface) found"
                if [ -n "${bss_type}" ] && [ "${bss_type}" -eq 64 ]; then
                    bbss_ifaces_5g="${bbss_ifaces_5g}${iface} "
                else
                    fbss_ifaces_5g="${fbss_ifaces_5g}${iface} "
                fi
            else
                __repacd_fronthaulmgrmon_debug "2.4 GHz AP VAP ($iface) found"
                if [ -n "${bss_type}" ] && [ "${bss_type}" -eq 64 ]; then
                    bbss_ifaces_24g="${bbss_ifaces_24g}${iface} "
                else
                    fbss_ifaces_24g="${fbss_ifaces_24g}${iface} "
                fi
            fi
        fi
    fi
}

# Check whether all of the provided interfaces are up
#
# input: $1 - list of AP VAPs to check
#
# return 0 if all VAPs are up, otherwise non-zero
__repacd_fronthaulmgrmon_all_AP_VAPs_up() {
    local ap_ifaces="$1"
    if [ -n "${ap_ifaces}" ]; then
        # Check whether all VAPs are up
        for item in $ap_ifaces
        do
            if ip link show ${item} | grep -q 'state DOWN'; then
                __repacd_fronthaulmgrmon_debug "Fronthaul $item is down; will re-attempt"
                return 1
            fi
        done
    fi

    return 0
}

# Perform an ifconfig operation on all of the VAPs provided
#
# input: $1 - list of all 2.4 GHz AP interfaces to bring up/down
# input: $2 - list of all 5 GHz AP interfaces to bring up/down
# input: $3 - the operation to perform (one of "up" or "down")
__repacd_fronthaulmgrmon_ifconfig_vaps() {
    local ap_ifaces_24g="$1"
    local ap_ifaces_5g="$2"
    local ifconfig_op="$3"

    if [ -z "${ap_ifaces_5g}" ]; then
        __repacd_fronthaulmgrmon_debug "No 5 GHz AP VAPs found"
    else
        for item in $ap_ifaces_5g
        do
            ifconfig "$item" "$ifconfig_op"
        done
    fi

    if [ -z "${ap_ifaces_24g}" ]; then
        __repacd_fronthaulmgrmon_debug "No 2.4 GHz AP VAPs found"
    else
        for item in $ap_ifaces_24g
        do
            ifconfig "$item" "$ifconfig_op"
        done
    fi
}

# Attempt to find the gateway's IP address on the given bridge
# input: $1 - bridge: the name of the bridge for which to find the gateway
# output: $2 - gw_ip: the parameter into which to place the gateway's IP
__repacd_fhmon_resolve_gw_ip() {
    local bridge=$1
    local resolved_ip=''

    resolved_ip=$(ip r | awk '/^def/{print $3}')
    if [ -n "$resolved_ip" ]; then
        eval "$2=$resolved_ip"
    else
        __gwmon_debug "Failed to resolve GW IP for $bridge"
    fi
}

__repacd_fhmon_get_timestamp() {
    timestamp=$(cut -d' ' -f1 < /proc/uptime | cut -d. -f 1)
    eval "$1=$timestamp"
}

__repacd_fhmon_check_gateway() {
    local network=$1
    local gw_ip

    __repacd_fhmon_resolve_gw_ip "br-$network" gw_ip
    if ping -W 2 "$gw_ip" -c1 > /dev/null; then
        if [ "$IS_GW_REACHABLE" -eq 0 ]; then
            __repacd_fronthaulmgrmon_debug "GW ($gw_ip) reachable"
            IS_GW_REACHABLE=1
            GW_NOT_REACHABLE_TIMESTAMP=0
        fi
    else
        if [ "$IS_GW_REACHABLE" -eq 1 ]; then
            __repacd_fronthaulmgrmon_debug "GW ($gw_ip) not reachable"
	    config_load wireless
	    config_foreach __repacd_wifimon_set_root_distance wifi-iface
        fi
        IS_GW_REACHABLE=0
        if [ "$GW_NOT_REACHABLE_TIMESTAMP" -eq 0 ]; then
            __repacd_fhmon_get_timestamp GW_NOT_REACHABLE_TIMESTAMP
        fi
    fi
}

# Initialize the fronthaul manager, taking action to bring down the fronthaul
# VAPs if the feature is enabled and the gateway is not reachable.
repacd_fronthaulmgrmon_init() {
    local force_down_on_start
    local mngd_nw

    # First resolve the config parameters.
    config_load repacd
    config_get manage_fronthaul_and_backhaul_independently 'FrontHaulMgr' \
        'ManageFrontAndBackHaulsIndependently' '0'
    config_get fronthaul_VAPs_bringdown_time 'FrontHaulMgr' \
        'FrontHaulMgrTimeout' '3600'
    config_get force_down_on_start 'FrontHaulMgr' 'ForceDownOnStart' 0
    config_get mngd_nw repacd 'ManagedNetwork' 'lan'

    if [ "$manage_fronthaul_and_backhaul_independently" -gt 0 ]; then
        config_load wireless
        config_foreach __repacd_fronthaulmgrmon_get_fronthaul_vaps wifi-iface "lan"

        __repacd_fronthaulmgrmon_debug "Resolved 2.4 GHz fBSSes: $fbss_ifaces_24g"
        __repacd_fronthaulmgrmon_debug "Resolved 5 GHz fBSSes: $fbss_ifaces_5g"

        __repacd_fronthaulmgrmon_debug "Resolved 2.4 GHz bBSSes: $bbss_ifaces_24g"
        __repacd_fronthaulmgrmon_debug "Resolved 5 GHz bBSSes: $bbss_ifaces_5g"

        __repacd_fhmon_check_gateway "$mngd_nw"
        if [ "$IS_GW_REACHABLE" -gt 0 ]; then
            __repacd_fronthaulmgrmon_debug "Bringing up Front-haul VAPs"
            __repacd_fronthaulmgrmon_ifconfig_vaps "${fbss_ifaces_24g}${bbss_ifaces_24g}" \
                "${fbss_ifaces_5g}${bbss_ifaces_5g}" "up"
            is_front_haul_VAPs_brought_down=0
        elif [ "$force_down_on_start" -gt 0 ]; then
            __repacd_fronthaulmgrmon_debug "Bringing down fronthaul VAPs on start"
            __repacd_fronthaulmgrmon_ifconfig_vaps "${fbss_ifaces_24g}{bbss_ifaces_24g}" \
                "${fbss_ifaces_5g}${bbss_ifaces_5g}" "down"
            is_front_haul_VAPs_brought_down=1
        fi
    fi
}

# Perform the periodic check to determine whether the fronthual interface
# states need to be updated or not.
repacd_fronthaulmgrmon_check() {
    local mngd_nw

    if [ "$manage_fronthaul_and_backhaul_independently" -gt 0 ]; then
        config_load repacd
        config_get mngd_nw repacd 'ManagedNetwork' 'lan'
        __repacd_fhmon_check_gateway "$mngd_nw"
        if [ "$IS_GW_REACHABLE" -eq 0 ]; then
            if [ "$is_front_haul_VAPs_brought_down" -eq 0 ]; then
                if  __repacd_wifimon_is_timeout "$GW_NOT_REACHABLE_TIMESTAMP" \
                    $fronthaul_VAPs_bringdown_time; then
                    __repacd_fronthaulmgrmon_debug "Expired $fronthaul_VAPs_bringdown_time sec, Bringing down Front-haul VAPs"
                    __repacd_fronthaulmgrmon_ifconfig_vaps "${fbss_ifaces_24g}${bbss_ifaces_24g}" \
                        "${fbss_ifaces_5g}${bbss_ifaces_5g}" "down"
                    is_front_haul_VAPs_brought_down=1
                fi
            fi
        else
            if [ "$is_front_haul_VAPs_brought_down" -gt 0 ]; then
                local ap_ifaces_24g="${fbss_ifaces_24g}"
                local ap_ifaces_5g="${fbss_ifaces_5g}"

                # If the bBSSes were not explicitly brought down due to a poor
                # backhaul link and rather were only down due to a completely
                # missing backhaul link, bring them back up. This is being
                # optimistic that the backhaul link will be good. If it is not, they
                # will be brought down again.
                if [ "$bBSSes_forced_down" -eq 0 ]; then
                    ap_ifaces_24g="${ap_ifaces_24g}${bbss_ifaces_24g}"
                    ap_ifaces_5g="${ap_ifaces_5g}${bbss_ifaces_5g}"
                fi

                __repacd_fronthaulmgrmon_debug "Bringing up Front-haul VAPs"
                __repacd_fronthaulmgrmon_ifconfig_vaps "$ap_ifaces_24g" "$ap_ifaces_5g" "up"

                if __repacd_fronthaulmgrmon_all_AP_VAPs_up "${ap_ifaces_5g}" &&
                    __repacd_fronthaulmgrmon_all_AP_VAPs_up "${ap_ifaces_24g}"; then
                    __repacd_fronthaulmgrmon_debug "All Front-haul VAPs are up"
                    is_front_haul_VAPs_brought_down=0
                fi
            fi
        fi
    fi
}

# Force down all bBSSes due to a poor backhaul link.
repacd_fronthaulmgrmon_bring_down_bBSSes() {
    __repacd_fronthaulmgrmon_debug "Bringing down bBSSes"
    __repacd_fronthaulmgrmon_ifconfig_vaps "$bbss_ifaces_24g" "$bbss_ifaces_5g" "down"
    bBSSes_forced_down=1
}

# Force up all bBSSes due to a better backhaul link
repacd_fronthaulmgrmon_bring_up_bBSSes() {
    # If they are already up, nop this. This is primarily done to avoid
    # disturbing already up interfaces as we may need to attempt to bring
    # them up multiple times (in case they don't come up the first time).
    if [ "${bBSSes_forced_down}" -gt 0 ]; then
        __repacd_fronthaulmgrmon_debug "Bringing up bBSSes"
        __repacd_fronthaulmgrmon_ifconfig_vaps "$bbss_ifaces_24g" "$bbss_ifaces_5g" "up"

        if ! __repacd_fronthaulmgrmon_all_AP_VAPs_up "${bbss_ifaces_5g}" ||
            ! __repacd_fronthaulmgrmon_all_AP_VAPs_up "${bbss_ifaces_24g}"; then
            __repacd_fronthaulmgrmon_debug "All Front-haul VAPs are up"
            bBSSes_forced_down=0
        fi
    fi
}
