#!/bin/sh
# Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
# 2015-2016 Qualcomm Atheros, Inc.
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.

GWMON_DEBUG_OUTOUT=0
GWMON_SWITCH_CONFIG_COMMAND=swconfig

GWMON_MODE_NO_CHANGE=0
GWMON_MODE_CAP=1
GWMON_MODE_NON_CAP=2
GWMON_KERNEL="4.4.60"
GWMON_PLATFORM=QCA
backhaul_mode_single=SINGLE
backhaul_rate_zero=0

log_level="INFO"

. /lib/functions.sh
. /lib/functions/hyfi-iface.sh
. /lib/functions/hyfi-network.sh

config_load 'repacd'
config_get_bool gwmon_wxt repacd 'ForceWextMode' '0'
config_get_bool ezmesh repacd 'Ezmesh' '0'
config_get log_level MAPConfig 'MapLogLevel'

    if [ "$ezmesh" -eq 1 ]; then
        MAP='ezmesh'
    else
        MAP='hyd'
    fi


prev_gw_link='' router_detected=0 gw_iface="" gw_switch_port=""
router_detected_wifi=0
managed_network='' switch_iface="" vlan_group="" switch_ports=''
cpu_portmap=0
eswitch_support="0"
switch_present="1"
gw_mac=""
single_gmac=0
ping_success_max_cnt=0
ping_success_cnt=0
last_hop_count=0
gw_unreachable_max_attempts=3
gw_reachable_confirm_attempts=5
gw_reachable_confirm_min_replies=4

# Globals exported to other modules for reachability over any interface
IS_GW_REACHABLE=0
GW_NOT_REACHABLE_TIMESTAMP=0

# Globals exported to other modules for reachability over a wifi interface
IS_GW_WIFI_REACHABLE=0
GW_NOT_WIFI_REACHABLE_TIMESTAMP=0
gw_mac_wifi=""

# Globals exported to other modules for reachability over an Ethernet
# interface
IS_GW_ETH_REACHABLE=0
GW_NOT_ETH_REACHABLE_TIMESTAMP=0
restart_count=0
is_ip_resolved=0
#Check for Platform & Kernel version
dut_kernel=$(uname -a  | awk '{print $3}')
dut_platform=$(grep -w DISTRIB_RELEASE /etc/openwrt_release | awk -F "='" '{print $2}' | awk '{gsub(/.{3}/,"& ")}1' | awk '{print $1}')

# Fast Onboarding
map_fast_onboarding=0
gw_not_reachable_count=0
map_my_version=0
map_ts_enabled=0
dot1x_support=0
dot1x_prev_state=2
GWMON_MAP_VERSION_2=2
GWMON_MAP_VERSION_3=3
GWMON_MAP_VERSION_4=4


# Emit a log message
# input: $1 - level: the symbolic log level
# input: $2 - msg: the message to log
__gwmon_log() {
    local stderr=''
    if [ "$GWMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.gwmon -p "user.$1" "$*"
}

__gwmon_log_dump() {
    local stderr=''

    [ "$log_level" != "DUMP" ] && return

    if [ "$GWMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.gwmon -p "user.$1" "$*"
}

# Emit a log message at debug level
# input: $1 - msg: the message to log
__gwmon_debug() {
    __gwmon_log 'debug' "$1"
}

# Emit a log message at info level
# input: $1 - msg: the message to log
__gwmon_info() {
    __gwmon_log 'info' "$1"
}

# Emit a log message at warning level
# input: $1 - msg: the message to log
__gwmon_warn() {
    __gwmon_log 'warn' "$1"
}

# Obtain a timestamp from the system.
#
# These timestamps will be monontonically increasing and be unaffected by
# any time skew (eg. via NTP or manual date commands).
#
# output: $1 - the timestamp as an integer (with any fractional time truncated)
__gwmon_get_timestamp() {
    timestamp=$(cut -d' ' -f1 < /proc/uptime | cut -d. -f 1)
    eval "$1=$timestamp"
}

__gwmon_find_switch() {
    local vlan_grp
    local switch_num

    #Ignore value returned by eswitch_support in repacd. It is to be used by hyd only.
    __hyfi_get_switch_iface switch_iface eswitch_support switch_num switch_present

    if [ -z "$switch_iface" ]; then
        __gwmon_debug "Switch interface not found [$switch_present]"
    fi

    if [ -n "$switch_iface" ]; then
        __gwmon_debug "Detected Switch Interface in [$switch_iface] switch_num[$switch_num] switch_present[$switch_present]"
        $GWMON_SWITCH_CONFIG_COMMAND dev switch$switch_num set flush_arl 2>/dev/null
        vlan_grp="$(echo $switch_iface | awk -F. '{print $2}' 2>/dev/null)"
    fi

    if [ -z "$vlan_grp" ]; then
        vlan_group="1"
    else
        vlan_group="$vlan_grp"
    fi
}

__gwmon_get_switch_ports() {
    local config="$1"
    local vlan_group="$2"
    local ports vlan cpu_port __cpu_portmap

    config_get vlan "$config" vlan
    config_get ports "$config" ports

    [ ! "$vlan" = "$vlan_group" ] && return

    cpu_port=$(echo "$ports" | awk '{print $1}')
    ports=$(echo "$ports" | sed "s/$cpu_port //g")
    eval "$3='$ports'"

    cpu_port=$(echo "$cpu_port" | awk -Ft '{print $1}')

    case $cpu_port in
        0) __cpu_portmap=0x01;;
        1) __cpu_portmap=0x02;;
        2) __cpu_portmap=0x04;;
        3) __cpu_portmap=0x08;;
        4) __cpu_portmap=0x10;;
        5) __cpu_portmap=0x20;;
        6) __cpu_portmap=0x40;;
        7) __cpu_portmap=0x80;;
    esac
    eval "$4='$__cpu_portmap'"
}

__gwmon_set_hop_count() {
    local config=$1
    local iface mode

    config_get mode "$config" mode
    config_get iface "$config" ifname

    if [ "$mode" = "ap" ] && [ "$gw_connected_mode" != "CAP" ]; then
        __gwmon_info "Setting intf [$iface] hop count $2"
        cfg80211tool_mesh "$iface" set_whc_dist "$2"
        if [ $2 == 255 ]; then
            uci set repacd.GatewayLink.setInvalidHopCount='1'
            uci commit repacd
        else
            uci set repacd.GatewayLink.setInvalidHopCount='0'
            uci commit repacd
        fi
    fi
}

# Determine the number of hops a given AP interface of RE is from the
# root AP.
# input: $1 - iface: the name of the AP interface (eg. ath0)
__gwmon_get_hop_count() {
    local iface=$1
    local command_result

    if [ -z "$iface" ]; then
        return 0
    fi

    command_result=`cfg80211tool_mesh $iface get_whc_dist | awk -F':' '{print $2}'`

    if [ "$command_result" -eq 255 ]; then
        return 0
    fi
    return 1
}

# Check MAC learning in ssdk_sh command with Port Status
__gwmon_check_mac_portstatus() {
    local sh_ssdk
    local sh_mac

    if [ "$single_gmac" -eq 1 ]; then
        sh_mac="$(echo "$1" | sed 's/:/-/g')"
        sh_ssdk=$(ssdk_sh fdb entry show 0 |grep  $sh_mac | awk -F':' '{print $5}')
    else
        sh_mac="$1"
        sh_ssdk=$(brctl showmacs br-lan  | awk "/${gw_mac}/"' { print $1}')
    fi

    for port_tmp in $sh_ssdk
    do
        if [ "$port_tmp" -gt 0 ]; then
            gw_switch_port=$port_tmp
            return 0
        fi
    done
    return 1
}

# __gwmon_check_gateway_iface_lan_iface
# input: $1 ethernet interfaces part of lan
# input: $2 Gateway interface
# returns: 0 if gateway interface matches with ether interface
# and assign gateway interface to swicth_iface
__gwmon_check_gateway_iface_lan_iface() {
    local ether_iface
    local gwiface

    # Get the parent interface if gwiface or ether_iface got created for vlan.
    gwiface=${1//.[0-9]*/}
    ether_iface=${2//.[0-9]*/}

    if [ "$ether_iface" = "$gwiface" ]; then
        return 0
    fi
    return 1
}

# Attempt to find the gateway's IP address on the given bridge
# input: $1 - bridge: the name of the bridge for which to find the gateway
# output: $2 - gw_ip: the parameter into which to place the gateway's IP
__gwmon_resolve_gw_ip() {
    local bridge=$1
    local resolved_ip=''

    resolved_ip=$(ip r | awk '/^def/{print $3}')
    if [ -n "$resolved_ip" ]; then
        eval "$2=$resolved_ip"
    else
        __gwmon_debug "Failed to resolve GW IP for $bridge"
    fi
}

# Check GW reachability over Ethernet backhaul,
# Set hop count correctly to prevent isolated island condition
# returns: 0 if gateway is still reachable; otherwise 1
__gwmon_prevent_island_loop() {
    local network=$1

    local retries="$gw_unreachable_max_attempts"
    local gw_ip next_hop_count=255

    while [ "$retries" -gt 0 ]; do
        __gwmon_resolve_gw_ip "br-$network" gw_ip
        [ -z "$gw_ip" ] && break

        # Ping returns zero if at least one response was heard from the specified host
        if ping -W 2 "$gw_ip" -c1 > /dev/null; then
            # Corner case handling where IS_GW_REACHABLE is not set as ping had failed
            # in gateway reachability check but has passed in the loop check
            if [ "$IS_GW_REACHABLE" -eq 0 ]; then
                IS_GW_REACHABLE=1
            fi
            if [ "$ping_success_cnt" -ge "$ping_success_max_cnt" ]; then
                next_hop_count=1
                if [ $last_hop_count -ne $next_hop_count ]; then
                    __gwmon_info "Changing hop_count to $next_hop_count "
                    config_load wireless
                    config_foreach __gwmon_set_hop_count wifi-iface $next_hop_count
                    last_hop_count=$next_hop_count
                fi
            fi
            break
        else
            # no ping response was received, retry
            retries=$((retries - 1))
            __gwmon_debug "Ping to GW IP[$gw_ip] on $gw_iface failed ($retries retries left)"
            if [ $last_hop_count -ne $next_hop_count ]; then
                __gwmon_info "Changing hop_count from $last_hop_count to $next_hop_count"
                config_load wireless
                config_foreach __gwmon_set_hop_count wifi-iface $next_hop_count
                last_hop_count=$next_hop_count
            fi
        fi
    done

    if [ "$retries" -eq 0 ]; then
        ping_success_cnt=0
        __gwmon_info "GW IP[$gw_ip] no longer reachable via $gw_iface"
        return 1
    else
        ping_success_cnt=$((ping_success_cnt + 1))
        return 0
    fi
}

# Check the link status of the interface connected to the gataway over wifi interface
# returns: 0 if gateway is detected; non-zero if it is not
__gwmon_check_gw_iface_link_wifi() {

    # Check if GW is reachable, set appropriate hop count to avoid
    # isolated island condition
    __gwmon_prevent_island_loop $managed_network
    return $?
}

# Check the link status of the interface connected to the gataway
# returns: 0 if gateway is detected; non-zero if it is not
__gwmon_check_gw_iface_link() {
    local ret

    if __gwmon_check_gateway_iface_lan_iface $gw_iface $switch_iface; then
        local link_status
        local switch_num
        local link_detected

        # Before we check local link status, make sure gw_iface (eth) is up
        ret=$(ifconfig $gw_iface | grep "UP[A-Z' ']*RUNNING")
        [ -z "$ret" ] && prev_gw_link="down" && return 1

        __hyfi_get_switch_iface switch_iface eswitch_support switch_num switch_present

        if [ "$single_gmac" -eq 1 ]; then
            link_status=$($GWMON_SWITCH_CONFIG_COMMAND dev switch$switch_num port $gw_switch_port get link |awk -F':' '{print $3}'|awk -F ' ' '{print $1}')
        else
            link_detected=$(ethtool $gw_iface | grep "Link detected: yes")
            if [ -z "$link_detected" ]; then
                link_status="down"
            else
                link_status="up"
            fi
        fi

        if [ ! "$link_status" = "up" ]; then
            link_status="down"
        fi

        if [ ! "$link_status" = "down" ]; then
            # link is up
            if [ ! "$prev_gw_link" = "up" ]; then
                __gwmon_info "Link to GW UP"
                prev_gw_link="up"
            fi

            # Check if GW is reachable, set appropriate hop count to avoid
            # isolated island condition
            __gwmon_prevent_island_loop $managed_network
            return $?
        fi
    else
        ret=$(ifconfig $gw_iface | grep "UP[A-Z' ']*RUNNING")
        [ -z "$ret" ] && prev_gw_link="down" && return 1

        # Check if GW is reachable, set appropriate hop count to avoid
        # isolated island condition
        __gwmon_prevent_island_loop $managed_network
        return $?
    fi

    if [ ! "$prev_gw_link" = "down" ]; then
        __gwmon_info "Link to GW DOWN"
        prev_gw_link="down"
    fi
    return 1
}

# Determine if the gateway is reachable over the given interface using
# arping
# input: $1 - iface: the name of the egress interface to use for the arping
# input: $2 - bridge: the name of the bridge to use to listen for a response
# input: $3 - gw_ip: the IP address of the gateway to attempt to reach
# return: 0 if the gateway is reachable, otherwise non-zero
__gwmon_arping_gateway() {
    local iface=$1
    local bridge=$2
    local gw_ip=$3

    arping -f -c 1 -w 0 -I "$iface" -B "$bridge" "$gw_ip" > /dev/null
    return $?
}

# Send consecutive ARPs to the gateway and expect replies to confirm it is
# indeed reachable and there was not a false positive due to the system also
# performing an ARP at the same time.
# input: $1 - iface: the name of the egress interface to use for the arping
# input: $2 - bridge: the name of the bridge to use to listen for a response
# input: $3 - gw_ip: the IP address of the gateway to attempt to reach
# return: 0 if the gateway is reachable, otherwise non-zero
__gwmon_arping_confirm_gateway() {
    local iface=$1
    local bridge=$2
    local gw_ip=$3

    __gwmon_info "Confirming GW IP ($gw_ip) is reachable on $iface"
    local replies
    replies=$(arping -c "$gw_reachable_confirm_attempts" \
                     -w "$gw_reachable_confirm_attempts" \
                     -I "$iface" -B "$bridge" "$gw_ip" |
              grep 'Received' | awk '{print $2;}')
    if [ "$replies" -ge "$gw_reachable_confirm_min_replies" ]; then
        __gwmon_info "GW IP ($gw_ip) confirmed reachable on $iface via $replies replies"
        return 0
    else
        __gwmon_info "GW IP ($gw_ip) confirmed not reachable on $iface via $replies replies"
        return 1
    fi
}

# __gwmon_is_restart_required
# restarting wifi if there is a change in gateway
# input: $1 5g bit rate
# input: $2 2g bit rate
# returns: 0 if there is a change in gateway; non-zero otherwise
__gwmon_is_restart_required() {
    local rate_5g=$1
    local rate_2g=$2

    config_load repacd
    config_get restart_max_attempts GatewayLink 'RestartMaxAttempts' '5'

    __gwmon_info "wait $restart_count rate_5g=$rate_5g rate_2g=$rate_2g is_ip=$is_ip_resolved"
    [ "$is_ip_resolved" -eq 0 ] && return 1
    if [ -n "$rate_5g" -o -n "$rate_2g" ]; then
        if [ "$rate_5g" != "0" -o "$rate_2g" != "0" ]; then
            restart_count=$((restart_count+1))
            if [ "$restart_count" -gt "$restart_max_attempts" ]; then
                restart_count=0
                is_ip_resolved=0
                return 0
            fi
        fi
    fi
    return 1
}

# __gwmon_check_gateway_wifi
# input: $1 1905.1 managed bridge
# output: $2 Gateway interface
# returns: 0 if gateway over wifi is detected; non-zero if not detected
__gwmon_check_gateway_wifi() {
    local network=$1

    local gw_ip gw_br_port __gw_iface
    local wlan_ifaces_full wlan_ifaces
    local wlan_iface ret
    local interface_gw
    local switch_num
    local iface ifaces_ath ifaces bss_type
    current_backhaul_5g_rate=`repacdcli $sta_iface_5g get_bitrate`
    current_backhaul_24g_rate=`repacdcli $sta_iface_24g get_bitrate`

    __gwmon_resolve_gw_ip "br-$network" gw_ip
    if [ "$map_fast_onboarding" -eq 1 ]; then
        [ -z "$gw_ip" ] && return 1
    else
        if [ -z "$gw_ip" ]; then
            if __gwmon_is_restart_required "$current_backhaul_5g_rate" "$current_backhaul_24g_rate"; then
                return 0
            fi
            return 1
        fi
    fi

    is_ip_resolved=1
    restart_count=0
    # Other modules still need to know about overall GW reachability
    if ping -W 2 "$gw_ip" -c1 > /dev/null; then
        if [ "$IS_GW_REACHABLE" -eq 0 ]; then
            __gwmon_info "GW ($gw_ip) reachable"
            rssi=`(repacdcli $sta_iface get_signal)`
            rssi=${rssi#*-}
            if [ "$map_fast_onboarding" -eq 1 ] && [ "$rssi" -gt 90 ]; then
                /etc/init.d/hyfi-bridging start
                /etc/init.d/$MAP start
                config_load repacd
                config_get retry 'WiFiLink' 'MinBestUplinkRetry' '5'
                local assoc_retry=$retry
                while [ "$assoc_retry" -gt 0 ]; do
                rssi=`(repacdcli $sta_iface get_signal)`
                rssi=${rssi#*-}
                if [ "$rssi" -gt 90 ]; then
                        assoc_retry=$((assoc_retry -1))
                        sleep 1
                    else
                        /etc/init.d/hyfi-bridging stop
                        /etc/init.d/$MAP stop
                        break
                    fi
                done
            fi
        fi
        IS_GW_REACHABLE=1
        GW_NOT_REACHABLE_TIMESTAMP=0
    else
        if [ "$IS_GW_REACHABLE" -eq 1 ]; then
            __gwmon_info "GW ($gw_ip) NOT reachable"
        fi
        IS_GW_REACHABLE=0
        if [ "$GW_NOT_REACHABLE_TIMESTAMP" -eq 0 ]; then
            __gwmon_get_timestamp GW_NOT_REACHABLE_TIMESTAMP
        fi
    fi

    if [ "$IS_GW_WIFI_REACHABLE" -eq 0 ]; then
        for iface in /sys/class/net/ath*; do
            iface=`basename $iface`

            case $iface in
                ath*.sta*)
                continue
                ;;
            esac

            bss_type=$(eval cfg80211tool_mesh $iface get_MapBSSType | awk -F: '{print $2}')

            # check for BSTA iface and allow arping
            if [ "$bss_type" != "128" ];then
                continue
            fi
            # arping to iface that has link UP
            link_up=$(ifconfig $iface | grep "UP[A-Z' ']*RUNNING")
            if [ -z "$link_up" ]; then
                continue
            fi

            if __gwmon_arping_gateway "$iface" "br-$network" "$gw_ip" &&
                __gwmon_arping_confirm_gateway "$iface" "br-$network" "$gw_ip"; then
                IS_GW_WIFI_REACHABLE=1
                GW_NOT_WIFI_REACHABLE_TIMESTAMP=0

                __gw_iface="$iface"
                break
            fi
        done
    else  # Currently reachable; just check the last interface
        if ! __gwmon_arping_gateway "$gw_iface" "br-$network" "$gw_ip" &&
            ! __gwmon_arping_confirm_gateway "$iface" "br-$network" "$gw_ip"; then
            __gwmon_info "GW ($gw_ip) no longer reachable on wifi intf $gw_iface"
            IS_GW_WIFI_REACHABLE=0
            if [ "$GW_NOT_WIFI_REACHABLE_TIMESTAMP" -eq 0 ]; then
                __gwmon_get_timestamp GW_NOT_WIFI_REACHABLE_TIMESTAMP
            fi
        fi
    fi

    gw_mac_wifi=$(grep -w "$gw_ip" /proc/net/arp | grep "br-$1" | awk '{print $4}')
    [ -z "$gw_mac_wifi" ] && return 1
    if [ -z "$__gw_iface" ]; then
        gw_br_port=$(brctl showmacs "br-$1" | grep -i "$gw_mac_wifi" | awk '{print $1}')
        [ -z "$gw_br_port" ] && return 1
        __gw_iface_2=$(brctl showstp "br-$1" | grep \("$gw_br_port"\) | awk '{print $1}')
        [ -z "$__gw_iface_2" ] && return 1
         __gw_iface=$__gw_iface_2
    fi

    # Check if this interface belongs to our network
    for iface in /sys/class/net/ath*; do
        iface=`basename $iface`
        if [ "$iface" = "$__gw_iface" ]; then
            gw_iface=$__gw_iface
            __gwmon_info "Detected Gateway on interface $gw_iface"
            return 0
        fi
    done

    for iface in /sys/class/net/mld*; do
        iface=`basename $iface`
        if [ "$iface" = "$__gw_iface" ]; then
            gw_iface=$__gw_iface
            __gwmon_info "Detected Gateway on MLD interface $gw_iface"
            return 0
        fi
    done

    return 1

}

# __gwmon_check_gateway
# input: $1 1905.1 managed bridge
# output: $2 Gateway interface
# returns: 0 if gateway is detected; non-zero if not detected
__gwmon_check_gateway() {
    local network=$1

    local gw_ip gw_br_port __gw_iface
    local ether_ifaces_full ether_ifaces
    local ether_iface ret
    local interface_gw
    local switch_num
    local upstream_mac upstream_iface upstream_iface_2 upstream_br_port
    current_backhaul_5g_rate=`repacdcli $sta_iface_5g get_bitrate`
    current_backhaul_24g_rate=`repacdcli $sta_iface_24g get_bitrate`

    __gwmon_resolve_gw_ip "br-$network" gw_ip
    if [ "$map_fast_onboarding" -eq 1 ]; then
        [ -z "$gw_ip" ] && return 1
    else
        if [ -z "$gw_ip" ]; then
            if __gwmon_is_restart_required "$current_backhaul_5g_rate" "$current_backhaul_24g_rate"; then
                return 0
            fi
            return 1
        fi
    fi
    is_ip_resolved=1
    restart_count=0
    # Other modules still need to know about overall GW reachability
    if ping -W 2 "$gw_ip" -c1 > /dev/null; then
        if [ "$IS_GW_REACHABLE" -eq 0 ]; then
            __gwmon_info "GW ($gw_ip) reachable"
            rssi=`(repacdcli $sta_iface get_signal)`
            rssi=${rssi#*-}
            if [ "$map_fast_onboarding" -eq 1 ] && [ "$rssi" -gt 90 ]; then
                /etc/init.d/hyfi-bridging start
                /etc/init.d/$MAP start
                config_load repacd
                config_get retry 'WiFiLink' 'MinBestUplinkRetry' '5'
                local assoc_retry=$retry
                while [ "$assoc_retry" -gt 0 ]; do
                rssi=`(repacdcli $sta_iface get_signal)`
                rssi=${rssi#*-}
                if [ "$rssi" -gt 90 ]; then
                        assoc_retry=$((assoc_retry -1))
                        sleep 1
                    else
                        /etc/init.d/hyfi-bridging stop
                        /etc/init.d/$MAP stop
                        break
                    fi
                done
            fi
        fi
        IS_GW_REACHABLE=1
        GW_NOT_REACHABLE_TIMESTAMP=0
    else
        if [ "$IS_GW_REACHABLE" -eq 1 ]; then
            __gwmon_info "GW ($gw_ip) NOT reachable"
	    config_get MBsta_onboarding MAPConfig 'MultibSTAOnboarding' '0'
            if [ "$MBsta_onboarding" -eq 1 ]; then
                __gwmon_debug "GW unreachable"
            else
                if [ "$backhaul_mode_configured" = "$backhaul_mode_single" ] && [ "$current_backhaul_5g_rate" = "$backhaul_rate_zero" ] && [ "$current_backhaul_24g_rate" = "$backhaul_rate_zero" ]; then
                    __repacd_gwmon_bring_iface_up $sta_iface_24g
                    __gwmon_debug "2g VAP brought up since 5g brought down & AP is operating in SINGLE backhaul mode"
                fi
            fi
        fi
        IS_GW_REACHABLE=0
        if [ "$GW_NOT_REACHABLE_TIMESTAMP" -eq 0 ]; then
            __gwmon_get_timestamp GW_NOT_REACHABLE_TIMESTAMP
        fi
    fi

    # Get all Ethernet interfaces
    hyfi_get_ether_ifaces "$1" ether_ifaces_full
    hyfi_strip_list "$ether_ifaces_full" ether_ifaces

    if [ "$IS_GW_ETH_REACHABLE" -eq 0 ]; then
        for ether_iface in $ether_ifaces; do
            # arping to iface that has link UP
            link_up=$(ifconfig $ether_iface | grep "UP[A-Z' ']*RUNNING")
            if [ -z "$link_up" ]; then
                continue
            fi

            if __gwmon_arping_gateway "$ether_iface" "br-$network" "$gw_ip" &&
                __gwmon_arping_confirm_gateway "$ether_iface" "br-$network" "$gw_ip"; then
                IS_GW_ETH_REACHABLE=1
                GW_NOT_ETH_REACHABLE_TIMESTAMP=0

                __gw_iface="$ether_iface"
                break
            fi
        done
    else  # Currently reachable; just check the last interface
        if ! __gwmon_arping_gateway "$gw_iface" "br-$network" "$gw_ip" &&
            ! __gwmon_arping_confirm_gateway "$ether_iface" "br-$network" "$gw_ip"; then
            __gwmon_info "GW ($gw_ip) no longer reachable on $gw_iface"
            IS_GW_ETH_REACHABLE=0
            if [ "$GW_NOT_ETH_REACHABLE_TIMESTAMP" -eq 0 ]; then
                __gwmon_get_timestamp GW_NOT_ETH_REACHABLE_TIMESTAMP
            fi
        fi
    fi

    gw_mac=$(awk "/^${gw_ip//./\.}\>/"' { print $4 }' /proc/net/arp)
    [ -z "$gw_mac" ] && return 1
    if [ -z "$__gw_iface" ]; then
        gw_br_port=$(brctl showmacs "br-$1"  | awk "/${gw_mac}/"' { print $1}')
        [ -z "$gw_br_port" ] && return 1
        __gw_iface_2=$(brctl showstp "br-$1"  | grep \("$gw_br_port"\))
        __gw_iface_2=${__gw_iface_2% (*}
        [ -z "$__gw_iface_2" ] && return 1
        __gw_iface=$__gw_iface_2
    fi

    # Check if this interface belongs to our network
    for ether_iface in $ether_ifaces; do
        if [ "$ether_iface" = "$__gw_iface" ]; then
            #Topo: Cap<----wifi--αAgent-1---wifi----> Agent-2.
            # if eth plucked in b/w star and daisy Agent. packets starts looping because both the link alive for sometime.
            # LOOP Cap<----wifi-->Agent-1|<---wifi---->| Agent-2.
            #                            |<----eth---->|
            # once repacd deamon detects gw is reachable via eth, BH will switch from wifi to eth iface as below
            # it is observed due to the packets looping some there is a fake stp entry created in stp table on Agent-1.
            # this fake gw mac entry shows gw is reachable via eth iface.
            # So, before switching to eth iface lets Confirm GW is reachable on eth or not by sending arping.
            # if arping is failed do not switch to eth and break the loop.
            if ! __gwmon_arping_gateway "$__gw_iface" "br-$network" "$gw_ip"; then
                __gwmon_info "GW ($gw_ip) arping failed $__gw_iface"
                break
            fi
            gw_iface=$__gw_iface
            __gwmon_info "Detected Gateway on interface $gw_iface"

            # Hawkeye platform has separate gmac for each switch port so gw_iface & switch_iface are same
            # For Maple+Spruce platform, Since switch is not present, skip assigning gateway interface as switch_iface
            __hyfi_get_switch_iface switch_iface eswitch_support switch_num switch_present

            if [ "$switch_present" -gt 0 ]; then
                if [ -z "$switch_iface" ]; then
                    if __gwmon_check_gateway_iface_lan_iface  "$gw_iface" "$ether_iface"; then
                        switch_iface="$gw_iface"
                    fi
                fi
            fi

            if __gwmon_check_gateway_iface_lan_iface "$gw_iface" "$switch_iface"; then
                if ! __gwmon_check_mac_portstatus $gw_mac; then
                    __gwmon_warn "invalid port map portmap"
                    gw_switch_port=9
                    # CAP <--eth--> RE1 <--eth--> RE2
                    # If eth disconnected between CAP and RE1, then topology will be
                    # CAP <--vap--> RE1 <--eth--> RE2 <--vap--> CAP
                    # this will form loop, RE1 and RE2 not able to reach gateway IP.
                    # RE1 become Non-Cap mode and Gateway mac still in eth port due to the loop.
                    # Hence ping fail observed.
                    # To avoid loop, bringing down the eth interface for 2 seconds and bringing back to up
                    ifconfig "$gw_iface" down
                    sleep 2
                    ifconfig "$gw_iface" up
                    return 1
                fi
            fi

	    config_get MBsta_onboarding MAPConfig 'MultibSTAOnboarding' '0'
            if [ "$MBsta_onboarding" -eq 1 ]; then
                #Let's disconnect sta interfaces to avoid looping since GW
                #is reachable on ethernet at this point
                __gwmon_debug "Trigger Disconnect on all links"
                if [ -z "$MBsta_sta_intf_list" ]; then
                    config_load wireless
                    config_foreach __repacd_initialize_sta_ifaces wifi-iface
                fi
                __repacd_wifimon_MBsta_disconnect_link $MBsta_sta_intf_list
            else
                __repacd_gwmon_bring_iface_down $sta_iface_5g
                __repacd_gwmon_bring_iface_down $sta_iface_24g
            fi
            return 0
        fi
    done

    #Topo: Cap<----wifi-->Agent-1<---wifi----> Agent-2.
    # if eth plucked in b/w star and daisy Agent. packets starts looping because both the link alive for sometime.
    # once repacd deamon detects gw is reachable via eth, BH will switch from wifi to eth iface as below
    # it is observed some there is a fake stp entry in stp table on Agent-1, shows gw is reachable via eth iface.
    # in that case Agent-2 will not able to reach to gw via eth iface.
    # To overcome this issue we are checking if Upstaream device is reachable via eth iface.
    # if so switch to eth and disable the sta vaps.
    config_load repacd
    config_get upstream_mac MAPConfig 'UpstreamMAC' '0'
    # if gw_mac and upstream mac are same it means device is start Agent, avoid duplicate check.
    if [ "$upstream_mac" != "$gw_mac" ]; then
        __gwmon_info "Upstream Device MAC Adderss $upstream_mac"
        # return if upstream device mac is zero.
        [ -z "$upstream_mac" ] && return 1
        if [ -z "$upstream_iface" ]; then
            upstream_br_port=$(brctl showmacs "br-$1"  | awk "/${upstream_mac}/"' { print $1}')
            [ -z "$upstream_br_port" ] && return 1
            upstream_iface_2=$(brctl showstp "br-$1"	| grep \("$upstream_br_port"\))
            upstream_iface_2=${upstream_iface_2% (*}
            [ -z "$upstream_iface_2" ] && return 1
            upstream_iface=$upstream_iface_2
        fi
        # Check if this interface belongs to our network
        for ether_iface in $ether_ifaces; do
            if [ "$ether_iface" = "$upstream_iface" ]; then
                gw_iface=$upstream_iface
                __gwmon_info "Detected Upstream device is reachable on interface $gw_iface"

                __repacd_gwmon_bring_iface_down $sta_iface_5g
                __repacd_gwmon_bring_iface_down $sta_iface_24g
                return 0
            fi
        done
    fi

    # also check the loop prevention code to see if it believes we have
    # an upstream facing Ethernet interface
    local num_upstream
    num_upstream=$(lp_numupstream)
    if [ "${num_upstream}" -gt 0 ]; then
        return 0
    fi
    return 1
}

# Determine if the GW is reachable over wifi interface and update
# the router_detected_wifi global variable accordingly.
__gwmon_update_router_detected_wifi() {
    if __gwmon_check_gateway_wifi "$managed_network"; then
        router_detected_wifi=1
    else
        router_detected_wifi=0
    fi
}

# Determine if the GW is reachable over eth interface and update the
# router_detected global variable accordingly.
__gwmon_update_router_detected() {
    if __gwmon_check_gateway "$managed_network"; then
        router_detected=1
    else
        router_detected=0
    fi
}

# Check whether the configured mode matches the mode that is determined by
# checking for connectivity to the gateway.
#
# input: $1 cur_role: the current mode that is configured
# input: $2 start_mode: the mode in which the auto-configuration script is being
#                       run; This is used by the init script to help indicate
#                       that it was an explicit change into this mode.
#                       If the mode was CAP, then it should take some time
#                       before it is willing to switch back to non-CAP due
#                       to lack of a gateway.
# input: $3 managed_network: the logical name for the network interfaces to
#                            monitor
#
# return: value indicating the desired mode of operation
#  - $GWMON_MODE_CAP to act as the main AP
#  - $GWMON_MODE_NON_CAP to switch to being a secondary AP
#  - $GWMON_MODE_NO_CHANGE for now change in the mode
__gwmon_init() {
    local cur_mode=$1
    local start_mode=$2
    local eth_mon_enabled
    local maplite_enabled
    local map_onboarding_type
    local unreachable_max_attempts_wifi
    local unreachable_max_attempts_eth

    config_load repacd
    config_get_bool maplite_enabled MAPConfig 'EnableLiteMode' '0'
    config_get_bool map_fast_onboarding MAPConfig 'MapFastOnboarding' '0'
    config_get map_my_version MAPConfig 'MapVersionEnabled'
    config_get map_ts_enabled MAPConfig 'MapTrafficSeparationEnable' '0'
    config_get dot1x_support MAPConfig  'Enable8021x' '0'
    config_get map_onboarding_type MAPConfig 'OnboardingType'
    config_get single_gmac repacd 'SingleGmac' '0'
    config_get ping_success_max_cnt MAPConfig 'MaxPingSampleCount' '1'

    if [ "$maplite_enabled" -eq 1 ]; then
        return
    fi

    managed_network=$3
    __gwmon_find_switch "$managed_network"
    [ -n "$switch_iface" ] && __gwmon_info "found switch on $switch_iface VLAN=$vlan_group"

    config_load repacd
    config_get gw_connected_mode repacd 'GatewayConnectedMode' 'AP'
    config_get eth_mon_enabled repacd 'EnableEthernetMonitoring' '0'
    config_get unreachable_max_attempts_wifi GatewayLink 'UnreachableMaxAttemptsWifi' \
        "$gw_unreachable_max_attempts"
    config_get unreachable_max_attempts_eth GatewayLink 'UnreachableMaxAttempts' \
        "$gw_unreachable_max_attempts"
    config_get gw_reachable_confirm_attempts GatewayLink 'ReachableConfirmationAttempts' \
        "$gw_reachable_confirm_attempts"
    config_get gw_reachable_confirm_min_replies GatewayLink 'ReachableConfirmationReplies' \
        "$gw_reachable_confirm_min_replies"
    config_get gw_set_invalid_hop_count GatewayLink 'setInvalidHopCount' \
        "$gw_set_invalid_hop_count"

    config_load $MAP
    config_get backhaul_mode_configured 'hy' 'ForwardingMode' 'APS'
    __gwmon_debug "Backhaul mode is $backhaul_mode_configured"

    config_load network
    config_foreach __gwmon_get_switch_ports switch_vlan "$vlan_group" switch_ports cpu_portmap
    __gwmon_info "switch ports in the $managed_network network: $switch_ports"

    # Set invalid hop count based on flag after repacd restart
    if [ "$gw_set_invalid_hop_count" -eq 1 ]; then
        __gwmon_info "Setting hop count to 255 on VAPs after restart"
        config_load wireless
        config_foreach __gwmon_set_hop_count wifi-iface 255
        last_hop_count=255
    fi

    __gwmon_update_router_detected

    gw_unreachable_max_attempts=$unreachable_max_attempts_wifi
    if [ "$cur_mode" = "CAP" ]; then
        gw_unreachable_max_attempts=$unreachable_max_attempts_eth
        if [ "$router_detected" -eq 0 ]; then
            if [ "$eth_mon_enabled" -eq 0 ] && [ ! "$start_mode" = "CAP" ]; then
                return $GWMON_MODE_NON_CAP
            else
                local retries="$gw_unreachable_max_attempts"

                while [ "$retries" -gt 0 ]; do
                    __gwmon_update_router_detected
                    [ "$router_detected" -gt 0 ] && break
                    retries=$((retries - 1))
                    __gwmon_debug "redetecting gateway ($retries retries left)"
                done

                # If gateway was still not detected after our attempts,
                # indicate we should change to non-CAP mode.
                if [ "$router_detected" -eq 0 ]; then
                    if [ "$eth_mon_enabled" -eq 0 ]; then
                        return $GWMON_MODE_NON_CAP
                    else
                        return $GWMON_MODE_NO_CHANGE
                    fi
                fi
            fi
        fi
    else   # non-CAP mode
        if [ "$router_detected" -eq 1 ]; then
            local mixedbh
            mixedbh=$(uci get repacd.repacd.EnableMixedBackhaul 2>/dev/null)
            if [ "$mixedbh" != "1" ]; then
                return $GWMON_MODE_CAP
            fi
        fi
    fi

    if [ "$map_fast_onboarding" -eq 1 ] && [ "$IS_GW_REACHABLE" -eq 0 ]; then
        if [ "$map_onboarding_type" != "dpp" ]; then
            /etc/init.d/hyfi-bridging stop
            /etc/init.d/wsplcd stop
            /etc/init.d/$MAP stop
        fi
    fi

    return $GWMON_MODE_NO_CHANGE
}

# function to check whether gw is reachable via wifi
__gwmon_check_wifi() {
    if [ -n "$sta_iface_24g" ] || [ -n "$sta_iface_5g" ]; then
        if [ "$router_detected_wifi" -eq 0 ]; then
            __gwmon_update_router_detected_wifi
        else
            if ! __gwmon_check_gw_iface_link_wifi "$managed_network"; then
                # Gateway is gone
                router_detected_wifi=0
            fi
        fi
    fi
}

__gwmon_map_daemon_check() {
    local wsplcdPID hydPID
    local hyctl_disabled

    config_load repacd
    config_get map_onboarding_type MAPConfig 'OnboardingType'

    #Check if HYD and WSPLCD are running
    wsplcdPID=$(ps | grep wsplcd-lan.conf | grep -v grep | awk '{print$1}')
    hydPID=$(ps | grep $MAP-lan.conf | grep -v grep | awk '{print$1}')
    hyctl_disabled=$(hyctl show | grep br-lan)

    # Stop MAP applications if GW not reachable
    if [ "$IS_GW_REACHABLE" -eq 0 ]; then
        if [ "$map_onboarding_type" == "dpp" ]; then
            return
        fi

        gw_not_reachable_count=$((gw_not_reachable_count + 1))
        if [ "$gw_not_reachable_count" -gt 15 ]; then
            __gwmon_info "GW ($gw_ip) NOT reachable. Stopping MAP Daemons"
            if [ -n "$hyctl_disabled" ]; then
                __gwmon_info "Disable hyfi bridging"
                /etc/init.d/hyfi-bridging stop
            fi

            if [ -n "$wsplcdPID" ]; then
                __gwmon_info "Disable wsplcd"
                /etc/init.d/wsplcd stop
            fi

            if [ -n "$hydPID" ]; then
                __gwmon_info "Disable ezmesh"
                /etc/init.d/$MAP stop
            fi
        fi
    else
        gw_not_reachable_count=0
        if [ "$map_my_version" -eq 1 ] || [ "$map_ts_enabled" -eq 0 ]; then
            if [ -z "$hyctl_disabled" ]; then
                __gwmon_info "Enable hyfi bridging"
                /etc/init.d/hyfi-bridging start
            fi

            if [ -z "$hydPID" ]; then
                __gwmon_info "Enable ezmesh"
                /etc/init.d/$MAP start
                # Give time for ezmesh to start
                sleep 3
            fi

            if [ -z "$wsplcdPID" ]; then
                __gwmon_info "Enable wsplcd"
                /etc/init.d/wsplcd start
            fi
        fi
    fi
}

# return: 2 to indicate CAP mode; 1 for non-CAP mode; 0 for no change
__gwmon_check() {
    if [ "$map_fast_onboarding" -eq 1 ]; then
        __gwmon_map_daemon_check
    fi

    if [ "$dot1x_support" -eq 1 ]; then
        __gwmon_check_8021x_vap
    fi

    if [ "$router_detected" -eq 0 ]; then
        __gwmon_update_router_detected

        if [ "$router_detected" -gt 0 ]; then
            local mixedbh
            mixedbh=$(uci get repacd.repacd.EnableMixedBackhaul 2>/dev/null)
            # if we want to support mixed backhaul, e.g., if we want to
            # enable both WiFi and Ethernet backhaul, then we stay in
            # non-cap mode so the STA interfaces remain up.  otherwise,
            # set to cap mode which brings down the STA interfaces.
            if [ "$mixedbh" != "1" ]; then
                return $GWMON_MODE_CAP
            fi
        fi
    else
        if ! __gwmon_check_gw_iface_link "$managed_network"; then
            # Gateway is gone
            router_detected=0
            gw_iface=""
            gw_switch_port=""
            return $GWMON_MODE_NON_CAP
        fi
    fi

    return $GWMON_MODE_NO_CHANGE
}

# input: $1 - sta interface: the name of the interface for bringing up.
__repacd_gwmon_bring_iface_up() {
    local sta_iface=$1

    if [ -n "$sta_iface" ];then
        network_id=`wpa_cli -p /var/run/wpa_supplicant-$sta_iface list_network | grep DISABLED | awk '{print $1}'`
        if [ -z  $network_id ]; then
            network_id=0
        fi
        wpa_cli -p /var/run/wpa_supplicant-$sta_iface enable_network $network_id
        __gwmon_info "Interface $sta_iface Brought up with network id $network_id"
        if [ "$backhaul_mode_configured" = "$backhaul_mode_single" ]; then
            [ ! -f /etc/init.d/wsplcd ] || /etc/init.d/wsplcd restart
        fi
        if [ "$sta_iface" = "$sta_iface_5g" ]; then
            force_down_5g=0
        else
            force_down_24g=0
            #if 2.4G interface up, force reset independent_channel parameters to 0
            is_24G_down_by_independent_channel=0
            uci_set wireless $wifi_2G_interface_name independent_channel_set '0'
            uci_commit wireless
        fi
        rssi_counter=0
        last_assoc_state=0
    fi
}

# Bring down sta vap interface.
# input: $1 - sta interface: the name of the interface for bringing down.
__repacd_gwmon_bring_iface_down() {
    local sta_iface=$1
    if [ -n "$sta_iface" ];then
        network_id=`wpa_cli -p /var/run/wpa_supplicant-$sta_iface list_network | grep CURRENT | awk '{print $1}'`
        if [ -z  $network_id ]; then
            network_id=0
        fi
        wpa_cli -p /var/run/wpa_supplicant-$sta_iface disable_network $network_id
        __gwmon_info "Interface $sta_iface Brought down with network id $network_id"
        if [ "$sta_iface" = "$sta_iface_5g" ]; then
            force_down_5g=1
            if [ -n "$force_down_5g_timestamp" ] ;then
                backhaul_eval_time=$config_long_eval_time5g
            else
                backhaul_eval_time=$config_short_eval_time5g
            fi
            force_down_5g_timestamp=`cat /proc/uptime | cut -d' ' -f1 | cut -d. -f 1`
        elif [ "$sta_iface" = "$sta_iface_5gl" ]; then
            force_down_5g=0
        elif [ "$sta_iface" = "$sta_iface_5g_backup" ]; then
            force_down_5g=0
        else
            force_down_24g=1
        fi
        rssi_counter=0
    fi
}

# restarting application
# input: $1 - application to restart
__gwmon_restart_app() {
    local app=$1
    local IsDot1xActive

    config_load 'repacd'
    config_get IsDot1xActive MAPConfig 'IsActive8021x' 0
    if [ "$dot1x_prev_state" -ne "$IsDot1xActive" ]; then
        __gwmon_info "dot1x: Restarting $app .."
        /etc/init.d/$app restart
        dot1x_prev_state=$IsDot1xActive
    fi
}

# Restarting ezmesh / wsplcd based on 8021x state
__gwmon_check_8021x_vap() {
    if [ -z "$dot1x_support" -o "$dot1x_support" -eq 0 ]; then
        return
    fi

    if [ "$map_my_version" -eq "$GWMON_MAP_VERSION_2" ]; then
        __gwmon_restart_app wsplcd
    elif [ "$map_my_version" -ge "$GWMON_MAP_VERSION_3" ]; then
        __gwmon_restart_app ezmesh
    fi
}
