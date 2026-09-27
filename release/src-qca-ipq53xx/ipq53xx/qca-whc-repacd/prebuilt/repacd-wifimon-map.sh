#!/bin/sh
# Copyright (c) 2015-2019 Qualcomm Technologies, Inc.
#
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
# 2015-2016 Qualcomm Atheros, Inc.
#
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.

WIFIMON_DEBUG_OUTOUT=0

# Set this to a filename to log all commands executed.
# The output of relevant commands will be appended to the file.
WIFIMON_DEBUG_COMMAND_FILE=

WIFIMON_STATE_NOT_ASSOCIATED='NotAssociated'
WIFIMON_STATE_ASSOCIATED='Associated'
WIFIMON_STATE_AUTOCONFIG_IN_PROGRESS='AutoConfigInProgress'
WIFIMON_STATE_MEASURING='Measuring'
WIFIMON_STATE_WPS_TIMEOUT='WPSTimeout'
WIFIMON_STATE_ASSOC_TIMEOUT='AssocTimeout'
WIFIMON_STATE_RE_BACKHAUL_GOOD='RE_BackhaulGood'
WIFIMON_STATE_RE_BACKHAUL_FAIR='RE_BackhaulFair'
WIFIMON_STATE_RE_BACKHAUL_POOR='RE_BackhaulPoor'
WIFIMON_STATE_RE_SWITCH_BSTA='RE_SwitchingBSTA'
WIFIMON_STATE_RE_SWITCH_MLO_BSTA="RE_SwitchingMLObSTA"

WIFIMON_PIPE_NAME='/var/run/repacd.pipe'

WIFIMON_WILDCARD_BSSID='00:00:00:00:00:00'

. /lib/functions.sh
. /lib/functions/whc-network.sh
. /lib/wifi_interface_helper.sh
. /lib/netifd/utils.sh
. /lib/wifi/hostapd.sh

# State information
sta_iface_24g='' sta_iface_24g_config_name=''
sta_iface_5g='' sta_iface_5g_config_name='' unknown_ifaces=0
config_bssid='' current_bssid=''
assoc_timeout_logged=0 wps_timeout_logged=0
wps_in_progress=0 wps_start_time=''
wps_stabilization=0 wps_assoc_count=0
assoc_start_time='' last_assoc_state=0
reassoc_forced=0
hyd_restart=0
ping_running=0 last_ping_gw_ip=
ping_running_pid=''
rssi_num=0 rssi_filename=
bsta_max_preference=
connectivity_type_check=1
current_staiface=''
current_lastface=''

# Config parameters
rssi_samples=''
backhaul_rssi_2='' backhaul_rssi_5='' backhaul_rssi_offset='' max_valid_backhaul_rssi=''
min_wps_assoc=
assoc_timeout='' mlo_assoc_timeout='' wps_timeout='' bssid_timeout=''
measuring_cnt=0 measuring_attempts=0
force_bsses_down_on_all_bsta_switches=0
cnt_5g_attempts=0 max_5g_attempts=0
cnt_2gtimes=0
cnt_mlo_attempts=0 max_mlo_attempts=0
map_fast_onboarding=0
mlo_blocked_start_time=0 mlo_blocked_timeout=43200
mlo_start_time=0 mlo_stable_timeout=300
mlo_bsta_connection_broken=0
cac_state=0
# DPP
dpp_sta_iface=''
chirp_start_time=''
chirp_timeout=''
dpp_sta_connector=0
onboarding_type=0
chirp_count=0
connector_reset_count=0
connector_reset_threshold=30
dpp_connector_retry=0
dpp_key=0

# Onboarding
map_num_radio=0
sta_device=''
sta_iface_current=''
sta_mld_current=''
sta_nw=''
wifi0_restart_list=''
wifi1_restart_list=''
wifi2_restart_list=''
wifi3_restart_list=''
wifi4_restart_list=''
wifi0_config_list=''
wifi1_config_list=''
wifi2_config_list=''
wifi3_config_list=''
wifi4_config_list=''
wifi0_list=''
wifi1_list=''
wifi2_list=''
wifi3_list=''
wifi4_list=''
teardown_list=''
smartmonitor_list=''
mld_list=''
mld_vap_device=''
wifi_reload_req=0
total_vap_count=-1
new_vaps_added=0
unused_mld=0
teardown_done=0
skip_sta_restart=0

#CloningOptimization
mld_iface_maping=''
non_mld_iface_list=''
teardown_radio=''
teardown_mld=''
mld_changed_list=''
newly_added_mld_list=''
wifi0_new_vap_list=''
wifi1_new_vap_list=''
wifi2_new_vap_list=''
wifi3_new_vap_list=''
wifi4_new_vap_list=''
bh_ssid_param=''
bh_key_param=''

staiafce5g_lastvapradio=''
sta_lastvapradio=''
staiafce5g_lastvapvap=''
current_sta_wifimon1=''
global_preferred_radio=''
current_sta_wifimon='' preferred_sta_wifimon=''
preferred_sta_ath_wifimon=''
last_sta_vap=''
current_sta_ath=''
primary_link_bssid=''
counter_i=0
rddevice=''
assoc_timeout_occurred=0
isOnboardingDone=0
first_cloning=0
onboarding_complete=0
opt_wifi_list=''

# MLO
enable_mlo=0
mld_sta_count=0
mldbsta_list=''
mldbsta_iface_list=''
mldbsta_config_name_list=''
mldbbss_list=''
primary_mldbsta_iface=''
mldbsta_enabled=0
mlo_device_count=0
netdev_mld=''
MLOtolegacyvariable=''
mld_mlo_group=''
legacytoMLO=0
weather_radar_start_timer=0 weather_radar_timeout=0
pre_cac_enabled=0
ath_interface_to_delete=''
ath_device_to_delete=''
MBsta_onboarding=0
MBsta_mlo_mode=0
MBsta_non_mlo_mode=0
MBsta_mlo_sta_connected_list=''
MBsta_non_mlo_sta_connected_list=''
MBsta_multi_up_list=''
#SLO
enable_slo=0

log_level="INFO"

external_controller=0
MBsta_current_link=''
MBsta_desired_link=''

#used to indicate rssi is valid or not for
#any of the Mbsta interface
#Ex: 1- valid 0 - invalid
MBsta_rssi_valid=0

#Timer variables for Short & Long timers
MBsta_mlo_attempt_start_time=''
MBsta_mlo_attempt_time_duration=''
MBsta_short_timer_duration=''
MBsta_long_timer_duration=''

#if this is true then long timer duration is used for next MLO attempt
MBsta_short_timer_used=0
MBsta_staiface_config_name=''
MBsta_disconnectnonmlo_once=0
MBsta_upfront_mlo=0

#STAIFACES
sta_iface_24g=''
sta_iface_24g_config=''
sta_iface_5g=''
sta_iface_5g_config=''
sta_iface_5gl=''
sta_iface_5gl_config=''
sta_iface_6g=''
sta_iface_6g_config=''
sta_iface_6gl=''
sta_iface_6gl_config=''
MBsta_mlo_sta_iface_list=''
MBsta_non_mlo_sta_iface_list=''
MBsta_sta_intf_list=''
MBsta_IgnoreBstaOnEthBH=0
MBsta_mlo_bstaMld=''
MBsta_nonmlo_bstaMld=''
MBsta_connected_bhssid=''
MBsta_connected_bhkey=''
MBsta_connected_bhencryption=''
MBsta_multi_up_required=0

MBsta_bh_ssid_wps=''
MBsta_bh_ssid=''
MBsta_current_bssid=''
MBsta_old_bssid=''
EU_cac_state=0
agent_connected=''
MBsta_cnt_weather_radar_timer=0
MBsta_invalid_rssi_count=0

config_load 'ezmesh'
config_get_bool external_controller MultiAP 'ExternalController' '0'

config_load 'repacd'
config_get_bool ezmesh repacd 'Ezmesh' '0'

config_get log_level MAPConfig 'MapLogLevel'

if [ "$ezmesh" -eq 1 ]; then
    MAP='ezmesh'
else
    MAP='hyd'
fi

# Emit a message at debug level.
# input: $1 - the message to log
__repacd_wifimon_debug() {
    local stderr=''
    if [ "$WIFIMON_DEBUG_OUTOUT" -gt 0 ]; then
        echo "repacd (wifimon): $*" > /dev/console
        stderr='-s'
    fi

    logger $stderr -t repacd.wifimon -p user.debug "$1"
}

__repacd_wifimon_dump() {
    local stderr=''

    [ "$log_level" != "DUMP" ] && return

    if [ "$WIFIMON_DEBUG_OUTOUT" -gt 0 ]; then
        echo "repacd (wifimon): $*" > /dev/console
        stderr='-s'
    fi

    logger $stderr -t repacd.wifimon -p user.debug "$1"
}

# Log the output of a command to a file (when enabled).
# This is a nop unless WIFIMON_DEBUG_COMMAND_FILE is set.
# input: $1 - command output to log
__repacd_wifimon_dump_cmd() {
    if [ -n "$WIFIMON_DEBUG_COMMAND_FILE" ]; then
        touch $WIFIMON_DEBUG_COMMAND_FILE
        { date; echo "$1"; echo; } >> $WIFIMON_DEBUG_COMMAND_FILE
    fi
}

# Emit a message at info level.
__repacd_wifimon_info() {
    local stderr=''
    if [ "$WIFIMON_DEBUG_OUTOUT" -gt 0 ]; then
        echo "repacd (wifimon): $*" > /dev/console
        stderr='-s'
    fi

    logger $stderr -t repacd.wifimon -p user.info "$1"
}

# Obtain a timestamp from the system.
#
# These timestamps will be monontonically increasing and be unaffected by
# any time skew (eg. via NTP or manual date commands).
#
# output: $1 - the timestamp as an integer (with any fractional time truncated)
__repacd_wifimon_get_timestamp() {
    timestamp=$(cut -d' ' -f1 < /proc/uptime | cut -d. -f 1)
    eval "$1=$timestamp"
}

# Terminate any background ping that may be running.
# If no background pings are running, this will be a nop.
__repacd_stop_ping() {
    if [ "$ping_running" -gt 0 ]; then
        kill $ping_running_pid
        ping_running=0
        __repacd_wifimon_debug "Stopped ping to GW IP $last_ping_gw_ip"
    fi

    if [ -n "$rssi_filename" ]; then
        # Clean up the temporary file
        rm -f $rssi_filename
        rssi_filename=
    fi
}

# restart the hyd once bSTA is stable;
# As per new change in wpa supplicant, bSTA will be added to bridge once
# it is associated. So, hyd restart is required after bSTA is associated
__repacd_restart_hyd() {
    /etc/init.d/$MAP restart
}

# Start a background ping to the gateway address (if it can be resolved).
# This helps ensure the RSSI values are updated (as firmware will not report
# updates if only beacons are being received on the STA interface).
# input: $1 - network: the name of the network being managed
# return: 0 if the ping was started or is already running; otherwise 1
__repacd_start_ping() {
    config_load repacd
    config_get_bool maplite_enabled MAPConfig 'EnableLiteMode' '0'

    gw_ip=$(ip r | awk '/^def/{print $3}')
    if [ -n "$gw_ip" ]; then
        if [ ! "$gw_ip" = "$last_ping_gw_ip" ]; then
            # First need to kill the existing one due to the IP change.
            __repacd_stop_ping
            # This will leave ping_running set to 0.
        fi

        if [ "$ping_running" -eq 0 ]; then
            __repacd_wifimon_debug "Pinging GW IP $gw_ip"

            # Unfortunately the busybox ping command does not support an
            # interval. Thus, we can only ping once per second so there will
            # only be a handful of measurements over the course of our RSSI
            # sampling.
            ping "$gw_ip" > /dev/null & ping_running_pid=$!
            __repacd_wifimon_debug "PID of running ping job=$ping_running_pid"
            ping_running=1
            last_ping_gw_ip=$gw_ip
        fi

        # Ping is running now or was started.
        return 0
    fi

    __repacd_wifimon_info "Failed to resolve GW when starting ping; will re-attempt"
    local hyctl_portType=$(hyctl show | grep Unknown)
    if [ -n "$hyctl_portType" -a "$maplite_enabled" -ne 1 ]; then
        __repacd_wifimon_info "Port type Unknown, stop & start hyfi-bridging"
        /etc/init.d/hyfi-bridging stop
        /etc/init.d/hyfi-bridging start
    fi
    return 1
}

# Determine if the gateway is reachable.
#
# Ideally this would be limited to only the 5 GHz STA interface, but there
# is no good way to do this (since packets would need to be received on the
# bridge interface).
#
# return: 0 if the gateway is reachable; otherwise 1
__repacd_is_gw_reachable() {
    if [ -n "$last_ping_gw_ip" ]; then
        if ping -c 1 -W 1 "${last_ping_gw_ip}" > /dev/null; then
            return 0
        fi
    fi

    # Gateway is unknown or is not reachable
    return 1
}

__repacd_wifimon_get_sta_info() {
    # Resolve the STA interfaces.
    # Here we assume that if we have the 5 GHz interface, that is sufficient,
    # as not all modes will have a 2.4 GHz interface.
    if [ "$MBsta_onboarding" -eq 1 ]; then
        return
    fi
    sta_iface_5g='' sta_iface_24g='' sta_iface_24g_config_name='' sta_iface_5g_config_name=''
    __repacd_wifimon_get_sta_iface_map "$1"
    if [ -n "$sta_iface_5g" ] || [ -n "$sta_iface_24g" ]; then
        if [ -n "$sta_iface_24g" ]; then
            __repacd_wifimon_debug "Resolved 2.4 GHz STA interface to $sta_iface_24g"
            __repacd_wifimon_debug "2.4 GHz STA interface section $sta_iface_24g_config_name"
            config_get config_bssid "$sta_iface_24g_config_name" 'bssid' ''
        fi

        if [ -n "$sta_iface_5g" ]; then
            __repacd_wifimon_debug "Resolved 5 GHz STA interface to $sta_iface_5g"
            __repacd_wifimon_debug "5 GHz STA interface section $sta_iface_5g_config_name"
            config_get config_bssid "$sta_iface_5g_config_name" 'bssid' ''
        fi
        [ "$config_bssid" = "$WIFIMON_WILDCARD_BSSID" ] && config_bssid=''
        __repacd_wifimon_debug "Resolved target BSSID to $config_bssid"
    fi
}

# Determine if the STA interface named is current associated and active.
#
# input: $1 - sta_iface: the name of the interface (eg. ath01)
# return: 0 if associated; 1 if not associated or empty interface name
__repacd_wifimon_is_active_assoc() {
    local sta_iface=$1
    if [ -n "$sta_iface" ]; then
        local assoc_str=
        assoc_str=$(repacdcli "$sta_iface" get_sta_link)
        __repacd_wifimon_dump_cmd "State of $sta_iface: $assoc_str"

        assoc_str=${assoc_str#*Not-Associated}
        if [ -z $assoc_str ]; then
            return 1
        else
            return 0
        fi
    else
       # An unknown STA interface is considered not associated.
        return 1
    fi
}

# Determine if the STA interface named is current associated.
#
# Note that for the purposes of this function, an empty interface name is
# considered associated. This is done because in some configurations, only
# one interface is enabled.
#
# input: $1 - sta_iface: the name of the interface (eg. ath01)
# return: 0 if associated or if the interface name is empty; otherwise 1
__repacd_wifimon_is_assoc() {
    local sta_iface=$1

    if [ -n "$sta_iface" ];
    then
        if __repacd_wifimon_is_active_assoc "$sta_iface"; then
            return 0
        else
            return 1
        fi
    else
        # An unknown STA interface is considered associated.
        return 0
    fi
}

# Determine if the STA association is stable enough to be able to start
# the next step of the process.
# return: 0 if the association is stable; non-zero if it is not yet deemed
#         stable
__repacd_wifimon_is_assoc_stable() {
    if [ "$wps_stabilization" -gt 0 ]; then
        if [ "$wps_assoc_count" -ge "$min_wps_assoc" ]; then
            return 0
        else
            return 1
        fi
    else
        # No stabilization in progress
        return 0
    fi
}

__add_primary_vlan_to_BH_AP()
{
    local config="$1"
    local iface mode network mld_name vid

    config_get iface "$config" ifname
    config_get mode "$config" mode
    config_get network "$config" network
    config_get mld_name "$config" mld

    [ -n "$mld_name" ] && iface="$mld_name"

    if [ "$mode" == "ap" ] && [ "$network" == "backhaul" ]; then
        if [ "$sta_vid" -eq 0 ]; then
            config_get vid MAPConfig 'VlanIDNwPrimary' 10
            __repacd_add_vlan_interfaces $NETWORK_TYPE_LAN $iface $vid
        else
            __repacd_add_vlan_interfaces $NETWORK_TYPE_LAN $iface $sta_vid
        fi
    fi
}

# Determine if we are in MLO mode
# If any of MLO sta interface is
# connected, then we mark MBsta_mlo_mode=1
# If any Non Mlo sta interface is assoc
# we mark MBsta_non_mlo_mode=1
# Populates sta interface connected
__repacd_wifimon_MBsta_check_if_mlo_mode() {
    local config="$1"
    local mode iface mld device bhmlo_enabled

    config_get mode "$config" mode
    config_get iface "$config" ifname
    config_get mld "$config" mld
    config_get device "$config" device
    config_get_bool bhmlo_enabled "$device" map_mbsta_bhmlo_enabled '0'

    local assoced_bssid=
    if [ "$mode" = "sta" ]; then
        local   tmp_MBsta_connected_bhssid=$(wpa_cli -i $iface -p /var/run/wpa_supplicant-$iface status | grep 'ssid' | awk 'FNR == 2 {print}' | awk -F = '{print$2}')
        local wps_state=$(wpa_cli -i $iface -p /var/run/wpa_supplicant-$iface status | grep 'wpa_state' | awk -F = '{print$2}')
        __repacd_wifimon_debug "iface $iface ssid $ssid key $key temp-MultibSTAConnectedBackhaulSSID: $tmp_MBsta_connected_bhssid"
        __repacd_wifimon_debug "WPASTATE $wps_state"
        if [ "$wps_state" = "COMPLETED" ]; then #Completed means interface is actually connected
            if [ "$bhmlo_enabled" -eq 1 ]; then
                assoced_bssid=$(repacdcli "$iface" get_sta_link)
                __repacd_wifimon_dump_cmd "State of $iface: $assoced_bssid"
                assoced_bssid=${assoced_bssid#*Not-Associated}
                if [ -z $assoced_bssid ]; then
                    #basically from 12.2 even if one mlo link breaks it will still
                    #maintain connection on other link so we add the check that if one
                    #link is active then don't make the variable 0. Sta intefaces could
                    #be iterated and be active as 101 or 110 so if last sta entry was
                    #disconnected it will make this flag 0 wrongly indicating that it
                    #is not MLO connection
                    if [ "$MBsta_mlo_mode" -lt 1 ]; then
                        MBsta_mlo_mode=0
                        uci set repacd.MAPConfig.MBsta_mlo_mode='0'
                        uci commit repacd
                    fi
                else
                    MBsta_mlo_mode=1
                    MBsta_mlo_sta_connected_list="$MBsta_mlo_sta_connected_list $iface"
                    uci set repacd.MAPConfig.MBsta_mlo_mode='1'
                    uci set repacd.MAPConfig.MBsta_non_mlo_mode='0'
                    uci commit repacd
                fi
            elif [ "$bhmlo_enabled" -eq 0 ]; then
                assoced_bssid=$(repacdcli "$iface" get_sta_link)
                __repacd_wifimon_dump_cmd "State of $iface: $assoced_bssid"

                assoced_bssid=${assoced_bssid#*Not-Associated}
                if [ -n $assoced_bssid ]; then
                    MBsta_non_mlo_sta_connected_list="$MBsta_non_mlo_sta_connected_list $iface"
                fi
            fi
        fi
    fi
}

# Disconnects & disabled interfaces to scan & connect
# input: iface/list of ifaces to be disconnected
__repacd_wifimon_MBsta_disconnect_link() {

    local link="$@"
    local link_item
    local network_id

    if [ -n "$link" ]; then
        for link_item in $link; do
            wpa_cli -p "/var/run/wpa_supplicant-$link_item" -i "$link_item" disconnect
            __repacd_wifimon_debug "DISCONNECT & DISABLE $link_item"
            network_id=`wpa_cli -p /var/run/wpa_supplicant-$link_item list_network | grep $MBsta_connected_bhssid | awk '{print $1}'`
            if [ -z  $network_id ]; then
                network_id=0
            fi
            __repacd_wifimon_debug "Brought down $link_item on networkid $network_id"
            wpa_cli -p /var/run/wpa_supplicant-$link_item disable_network $network_id
        done
    else
        __repacd_wifimon_debug "Empty Link, Cannot disconnect!"
    fi
}

# Reconnects & enables interfaces to scan & connect
# input: iface/list of ifaces to be reconnected
__repacd_wifimon_MBsta_reconnect_link() {

    local link="$@"
    local link_item
    local network_id

    if [ -n "$link" ]; then
        __repacd_wifimon_debug "MultibSTAConnectedBackhaulSSID: $MBsta_connected_bhssid"
        for link_item in $link; do
            usleep 500000
            network_id=`wpa_cli -p /var/run/wpa_supplicant-$link_item list_network | grep $MBsta_connected_bhssid | awk '{print $1}'`
            if [ -z  $network_id ]; then
                network_id=0
            fi

            wpa_cli -p /var/run/wpa_supplicant-$link_item enable_network $network_id
            __repacd_wifimon_debug "Brought up $link_item on networkid $network_id"
            wpa_cli -p "/var/run/wpa_supplicant-$link_item" -i "$link_item" reconnect
            __repacd_wifimon_debug "ENABLING $link_item TO SCAN & CONNECT"

        done
    else
        __repacd_wifimon_debug "Empty Link, Cannot reconnect!"
    fi
}

__repacd_wifimon_MBsta_get_bhssid() {
    local config="$1"

    config_get mode "$config" mode
    config_get network "$config" network
    config_get ssid "$config" ssid

    if [ "$mode" = "ap" ] && [ "$network" = "backhaul" ]; then
        MBsta_bh_ssid="$ssid"
        return
    fi
}
# Fetches iface ex. athx from config
# input: wifi-iface
__repacd_wifimon_MBsta_get_iface_from_configname() {
    local config="$1"
    local item_to_match="$2"
    local mode_to_match="$3"
    local mode iface

    config_get mode "$config" mode
    config_get iface "$config" ifname

    __repacd_wifimon_debug "mode_to_match $mode_to_match item_to_match $item_to_match"
    if [ "$mode" != "$mode_to_match" ]; then
        return
    fi

    if [ "$mode" = "$mode_to_match" ] && [ "$iface" = "$item_to_match" ]; then
        config_get device "$config" device
        MBsta_staiface_config_name="$config"
        break
    fi
}

# Measure rssi for links
# Determines if thresholds are in range
# input: iface/iface list
__repacd_wifimon_MBsta_measure_rssi() {
    local link="$@"
    local link_if rssi device iface
    local sta_iface_config_name
    local device iface

    MBsta_rssi_valid=0
    if [ -n "$link" ]; then
        __repacd_wifimon_debug "Measure RSSI for link: $link"
        for link_if in $link; do
            rssi=$(repacdcli $link_if get_signal)

            device="wifi$(echo $link_if | cut -c 4)"
            #__repacd_wifimon_debug "device $device iface $iface"

            if __repacd_wifimon_whc_is_5g_radio $device; then
                rssi_threshold="$backhaul_rssi_5"
                rssi_led_state="$WIFIMON_STATE_RE_BACKHAUL_GOOD"
            else
                rssi_threshold="$backhaul_rssi_2"
                rssi_led_state="$WIFIMON_STATE_RE_BACKHAUL_FAIR"
            fi
            if [ "$rssi_num" -lt 2 ]; then
                __repacd_wifimon_debug "RSSI sample #$rssi_num"
            fi
            __repacd_wifimon_debug " Iface: $link_if RSSI: $rssi Threshold: $rssi_threshold"
            if [ "$rssi" -gt "$max_valid_backhaul_rssi" ] && [ "$rssi" -lt 0 ]; then
                #RSSI is not completely invalid, it may be poor or good
                if [ "$rssi" -gt "$rssi_threshold" ]; then
                    #RSSI is good and is within range so mark MBsta_rssi_valid true
                    if [ "$MBsta_mlo_mode" -eq 1 ]; then
                        #We reset this since successful measurement of MLO link
                        MBsta_invalid_rssi_count=0
                    fi

                    MBsta_rssi_valid=1
                fi
            else
                __repacd_wifimon_debug "RSSI $rssi is out of valid range (0 to $max_valid_backhaul_rssi)"
                #increase failed count here
                measuring_cnt=$((measuring_cnt + 1))
            fi
        done
        rssi_num=$((rssi_num + 1))
        if [ "$MBsta_rssi_valid" -eq 1 ]; then
            measuring_cnt=0 #we reset failed attempt count since we have atleast one valid rssi
        fi

        __repacd_wifimon_debug " MBsta_rssi_valid: $MBsta_rssi_valid"
    else
        __repacd_wifimon_debug "Empty link! cannot measure rssi"
    fi
}

#Function which will update the MBsta_connected_bhssid
#with valid ssid of the controller or node the Agent
#got connected to by checking if wpa_state=COMPLETED
__repacd_wifimon_MBsta_update_new_ssid() {

    local config="$1"
    local list="$@"

    local mld iface mode ssid key
    local list_item encryption

    config_get iface "$config" ifname

    for list_item in $list; do
        if [ "$iface" = "$list_item" ]; then
            config_get key "$config" key
            config_get encryption "$config" encryption

            #MBsta_connected_bhssid=$(iw dev $iface info | grep "ssid" | awk -F " " '{print $2}')
            local   tmp_MBsta_connected_bhssid=$(wpa_cli -i $iface -p /var/run/wpa_supplicant-$iface status | grep 'ssid' | awk 'FNR == 2 {print}' | awk -F = '{print$2}')
            local wps_state=$(wpa_cli -i $iface -p /var/run/wpa_supplicant-$iface status | grep 'wpa_state' | awk -F = '{print$2}')

            __repacd_wifimon_debug "update new credentials if changed: iface $iface ssid $ssid key $key temp-MultibSTAConnectedBackhaulSSID $tmp_MBsta_connected_bhssid"
            __repacd_wifimon_debug "WPASTATE: $wps_state"

            if [ -n "$key" -a -n "$tmp_MBsta_connected_bhssid" ] && [ "$wps_state" = "COMPLETED" ] && [ -n "$encryption" ]; then
                MBsta_connected_bhssid="$tmp_MBsta_connected_bhssid"
                MBsta_connected_bhkey="$key"
                MBsta_connected_bhencryption="$encryption"
                #This SSID will be compared against all bsta vaps as this is valid SSID. 
                #On mismatch, corresponding vap will be updated with valid credentials and wifi multi_up will be invoked.
                __repacd_wifimon_debug "ConnectedBackhaulSSID: $MBsta_connected_bhssid ConnectedBackhaulKey: $MBsta_connected_bhkey"
                __repacd_wifimon_debug "ConnectedBackhaulEncryption: $MBsta_connected_bhencryption"
                uci set repacd.MAPConfig.MultibSTAConnectedBackhaulSSID="$tmp_MBsta_connected_bhssid"
                uci set repacd.MAPConfig.MultibSTAConnectedBackhaulKey="$MBsta_connected_bhkey"
                uci set repacd.MAPConfig.MultibSTAConnectedBackhaulEncryption="$MBsta_connected_bhencryption"
                uci commit repacd
                break
            fi
        fi
    done
}

#Function for reference, Check if BHssid matches
#Bsta ssid & key options in wireless
__repacd_wifimon_MBsta_check_bhssid() {
    local config="$1"
    local list="$@"

    local iface ssid key mode
    local device currentband
    local encryption
    config_get iface "$config" ifname

    local list_item

    for list_item in $list; do
        if [ "$iface" = "$list_item" ]; then
            config_get ssid "$config" ssid
            config_get key "$config" key
            config_get device "$config" device
            config_get currentband "$device" band '0'
            config_get encryption "$config" encryption

            config_get MBsta_connected_bhssid MAPConfig MultibSTAConnectedBackhaulSSID
            config_get MBsta_connected_bhkey MAPConfig MultibSTAConnectedBackhaulKey
            config_get MBsta_connected_bhencryption MAPConfig MultibSTAConnectedBackhaulEncryption
            __repacd_wifimon_debug "interface: $list_item ssid: $ssid key: $key encryption: $encryption Connected Node's SSID: $MBsta_connected_bhssid"
            if [ -n "$MBsta_connected_bhssid" ] && [ -n "$MBsta_connected_bhkey" ] && [ -n "$MBsta_connected_bhencryption" ]; then
                if [ "$ssid" != "$MBsta_connected_bhssid" ] || [ "$key" != "$MBsta_connected_bhkey" ] || [ "$MBsta_connected_bhencryption" != "$encryption" ]; then
                    uci_set wireless "$config" ssid "$MBsta_connected_bhssid"
                    uci_set wireless "$config" key "$MBsta_connected_bhkey"
                    uci_set wireless "$config" encryption "$MBsta_connected_bhencryption"
                    if [ "$MBsta_connected_bhencryption" = "ccmp" ] || [ "$MBsta_connected_bhencryption" = "psk2+ccmp" ]; then
                        uci_set wireless "$config" sae 1
                    fi
                    if [ "$currentband" -eq 3 ]; then
                        __repacd_wifimon_debug "currentband: $currentband"
                        uci_set wireless "$config" 'en_6g_sec_comp' 0
                    fi
                    uci_commit wireless
                    MBsta_multi_up_required=1
                    __repacd_wifimon_debug "Mismatch of SSID/Key/Encryption for $iface, requires Multi_up"
                fi
            fi
    fi
    done
}


__repacd_wifimon_check_bsta_assoc() {
    local config="$1"
    local iface mode

    config_get iface "$config" ifname
    config_get mode "$config" mode

    if [ "$mode" = "sta" ]; then
      __repacd_wifimon_debug "MLO mode: check connectivity on iface $iface"
      if __repacd_wifimon_is_assoc $iface; then
        agent_connected=1
        break
      fi

    fi

}

# Determine if the STA is associated and update the state accordingly.
# input: $1 - network: the name of the network being managed
# input: $2 - cur_re_mode: the currently configured range extender mode
# input: $3 - cur_re_submode: the currently configured range extender sub-mode
# input: $4 - whether this is a check during init for a restart triggered
#             by mode switching
# output: $5 - state: the variable to update with the new state name (if there
#                     was a change)
# output: $6 - re_mode: the desired range extender mode
# output: $7 - re_submode: the desired range extender sub-mode
# return: 0 if associated; otherwise 1
__repacd_wifimon_check_associated() {
    local network=$1
    local associated=0 enable_force_reassoc=0
    local intf
    local MBsta_is_Agent_connected=0
    local backhual_ssid

    config_get enable_force_reassoc MAPConfig 'EnableForceReAssoc' 0
    config_get backhaul_ssid MAPConfig 'BackhaulSSID' ''

    #We check for any intf associated by cfg call
    if [ "$MBsta_onboarding" -eq 1 ]; then
        for intf in $MBsta_sta_intf_list; do
            if __repacd_wifimon_is_assoc $intf; then
                MBsta_is_Agent_connected=1
                break
            fi
        done

        if [ "$MBsta_is_Agent_connected" -eq 1 ]; then
            MBsta_non_mlo_sta_connected_list=''
            MBsta_mlo_sta_connected_list=''
            __repacd_wifimon_debug "Check if MLO mode"
            #Room for optimzation instead of all intf we know sta is connected so just check the sta intf lists
            config_load wireless
            config_foreach __repacd_wifimon_MBsta_check_if_mlo_mode wifi-iface
            __repacd_wifimon_debug " MBsta_mlo_mode $MBsta_mlo_mode MBsta_non_mlo_mode $MBsta_non_mlo_mode Mlo_Sta_connected $MBsta_mlo_sta_connected_list MBsta_non_mlo_sta_connected_list $MBsta_non_mlo_sta_connected_list"
            if [ "$MBsta_mlo_mode" -eq 0 ] && [ -n "$MBsta_non_mlo_sta_connected_list" ]; then
                __repacd_wifimon_debug "Associated in Non-Mlo mode $MBsta_non_mlo_sta_connected_list"
                MBsta_non_mlo_mode=1
                mldbsta_enabled=0
                primary_mldbsta_iface=''
                uci set repacd.MAPConfig.MBsta_mlo_mode='0'
                uci set repacd.MAPConfig.MBsta_non_mlo_mode='1'
                uci commit repacd
                config_load wireless
                config_foreach __repacd_wifimon_MBsta_update_new_ssid wifi-iface $MBsta_non_mlo_sta_connected_list
                __repacd_wifimon_debug "MultibSTAConnectedBackhaulSSID: $MBsta_connected_bhssid"

            else
                mldbsta_enabled=1
                __repacd_wifimon_debug "Associated in Mlo mode $MBsta_mlo_sta_connected_list"
                MBsta_short_timer_used=0 #we reset shorttimer everytime we hace successful MLO connection
                uci set repacd.MAPConfig.MBsta_shorttimerused='0'
                uci set repacd.MAPConfig.MBsta_mlo_mode='1'
                uci set repacd.MAPConfig.MBsta_non_mlo_mode='0'
                uci commit repacd
                MBsta_non_mlo_sta_connected_list=''
                __repacd_wifimon_debug "MLO connection success, will use short timer incase of failure of MLO and trying to assoc back"
                MBsta_non_mlo_sta_connected_list=''
                if [ "$cnt_mlo_attempt" -gt 0 ]; then
                    __repacd_wifimon_debug "Marking Failed MLO attempts count as zero since we had a successful MLO connection"
                    cnt_mlo_attempts=0
                    uci_set repacd MAPWiFiLink 'MLOAttemptsCount' "$cnt_mlo_attempts"
                fi

                config_load wireless
                config_foreach __repacd_wifimon_MBsta_update_new_ssid wifi-iface $MBsta_mlo_sta_connected_list
            fi
            if [ "$rssi_num" -gt 3 ]; then
                config_get MBsta_mlo_mode MAPConfig 'MBsta_mlo_mode' '0'
                config_get MBsta_non_mlo_mode MAPConfig 'MBsta_non_mlo_mode' '0'
                if [ "$MBsta_mlo_mode" -eq 1 ] && [ "$MBsta_non_mlo_mode" -eq 0 ]; then
                    __repacd_wifimon_MBsta_disconnect_link $MBsta_non_mlo_sta_iface_list
                elif [ "$MBsta_non_mlo_mode" -eq 1 ]; then
                    __repacd_wifimon_MBsta_disconnect_link $MBsta_mlo_sta_iface_list
                fi
            fi

            #If Agent is connected, we must check for any timer running and if timed out switch to MLO from backup link
            if [ -n "$MBsta_mlo_attempt_time_duration" ] && [ "$onboarding_done" -eq 1 ]; then
                local cur_time
                __repacd_wifimon_get_timestamp cur_time
                local elapsed_time=$((cur_time - MBsta_mlo_attempt_start_time))
                if [ "$(($MBsta_mlo_attempt_time_duration-$elapsed_time))" -gt 0 ]; then
                    __repacd_wifimon_debug "Will attempt MLO connection in $(($MBsta_mlo_attempt_time_duration-$elapsed_time))"
                fi
                #Only Check Timer expiry when in NON MLO mode
                if [ -n "$MBsta_mlo_attempt_start_time" ] && [ "$MBsta_non_mlo_mode" -eq 1 ]; then
                    if __repacd_wifimon_is_timeout $MBsta_mlo_attempt_start_time $MBsta_mlo_attempt_time_duration; then
                        eval "$5=$WIFIMON_STATE_RE_SWITCH_MLO_BSTA"
                        #MBsta_mlo_attempt_time_duration=0
                        MBsta_mlo_attempt_start_time=''
                        MBsta_non_mlo_mode=0
                        if [ "$MBsta_mlo_attempt_time_duration" = "$MBsta_short_timer_duration" ]; then
                            __repacd_wifimon_debug "Short timer expired. Attempting MLO"
                            MBsta_short_timer_used=1
                            uci set repacd.MAPConfig.MBsta_shorttimerused='1'
                            uci commit repacd
                        elif [ "$MBsta_mlo_attempt_time_duration" = "$MBsta_long_timer_duration" ]; then
                            __repacd_wifimon_debug "Long timer expired. Attempting MLO"
                            #Reset  weather radar timer count post Long timer expiry
                            MBsta_cnt_weather_radar_timer=0
                        fi
                        return 0
                    fi
                fi
           fi 
            associated=1
        fi
    else
        if [ "$mldbsta_enabled" -eq 0 ]; then
            if __repacd_wifimon_is_assoc $sta_iface_24g \
                && __repacd_wifimon_is_assoc $sta_iface_5g; then
                associated=1
            fi
        else
            agent_connected=0 #always mark as 0 before calling below function for reupdating
            config_load wireless
            config_foreach __repacd_wifimon_check_bsta_assoc wifi-iface
            if [ "$agent_connected" -gt 0 ]; then
                associated=1
                __repacd_wifimon_debug "MLO ASSOC'D associated $associated last_assoc_state $last_assoc_state"
            fi
        fi
    fi

    if [ "$map_onboarding_type" == "dpp" ]; then
       if [ $connectivity_type_check -eq 1 ] && [ "$associated" -eq 1 ]; then
          connection_type=$(wpa_cli -i "$sta_iface" -p /var/run/wpa_supplicant-"$sta_iface" status | grep onboard_type | awk -F'[/=]' '{print $2}')
          if [ -n "$connection_type" ]; then
              connectivity_type_check=0
          fi
          # If DPP Agent associates with R2 Controller or to non-DPP controller via WPS
          # then Agent should move to non-DPP mode and proceed cloning via wsplcd
          if [ "$connection_type" == "WPS" ]; then
              __repacd_wifimon_debug "non-DPP Connection, Fallback to non-DPP mode"
              local sta_iface_conf=''

              fallback_to_nonDPP=1

              uci set ezmesh.MAPConfigSettings.EnableConfigService='0'
              uci set repacd.MAPConfig.fallBackToNonDPP='1'
              if [ -n "$sta_iface_24g" ]; then
                  sta_iface_conf="$sta_iface_24g_config_name"
              else
                  sta_iface_conf="$sta_iface_5g_config_name"
              fi
              uci set repacd.MAPConfig.enableDPPOverWPS='0'
              uci delete wireless."$sta_iface_conf.dpp"
              uci delete wireless."$sta_iface_conf.dpp_map"
              uci delete wireless."$sta_iface_conf.dpp_key"
              uci commit
          fi
       fi
    fi

    config_get onboarding_done MAPConfig 'OnboardingDone' '0'
    if [ "$associated" -gt 0 ]; then
        if [ "$MBsta_onboarding" -eq 0 ]; then
            local old_bssid="$current_bssid"
            if [ -n "$sta_iface_5g" ]; then
                __repacd_wifimon_update_serving_bssid "$sta_iface_5g"
            else
                __repacd_wifimon_update_serving_bssid "$sta_iface_24g"
            fi
        elif [ "$MBsta_onboarding" -eq 1 ]; then
            local intf=''
            local interface_list=''
            MBsta_non_mlo_sta_connected_list=''
            MBsta_mlo_sta_connected_list=''
            config_load wireless
            config_foreach __repacd_wifimon_MBsta_check_if_mlo_mode wifi-iface
            if [ "$MBsta_mlo_mode" -eq 1 ]; then
                interface_list="$MBsta_mlo_sta_connected_list"
            elif [ "$MBsta_non_mlo_mode" -eq 1 ]; then
                interface_list="$MBsta_non_mlo_sta_connected_list"
            fi
            for intf in $interface_list; do
                __repacd_wifimon_update_serving_bssid "$intf"
                break
            done
        fi
        if [ "$MBsta_onboarding" -eq 0 ]; then
            if __repacd_wifimon_is_bssid_change "$old_bssid"; then
                # Ensure this is treated as a new association that triggers a
                # new measurement
                last_assoc_state=0
            fi
        else
            __repacd_wifimon_debug "old_bssid $MBsta_old_bssid current_bssid $current_bssid"
            if __repacd_wifimon_is_bssid_change "$MBsta_old_bssid"; then
                # Ensure this is treated as a new association that triggers a
                # new measurement
                last_assoc_state=0
            fi
            MBsta_old_bssid="$current_bssid"
        fi
        local map_ts_enabled
        config_get map_ts_enabled MAPConfig 'MapTrafficSeparationEnable' '0'
        if [ "$MBsta_onboarding" -eq 1 ]; then
            if [ "$map_fast_onboarding" -eq 1 ] && [ "$map_ts_enabled" -eq 1 ]; then
                for iface_name in $sta_iface_24g $sta_iface_5g $sta_iface_5gl $sta_iface_6g $sta_iface_6gl; do
                    __repacd_vlanmon_check_sta_vlan "$iface_name"
                done
            fi
            # Restart the hyd once bSTA is associated
            if [ "$hyd_restart" -eq 1 ]; then
                __repacd_restart_hyd
                hyd_restart=0
            fi
        fi
        # Only update the LED state if we transitioned from not associated
        # to associated.
        if [ "$last_assoc_state" -eq 0 ]; then
            if [ "$wps_in_progress" -gt 0 ]; then
                # If WPS was triggered, it could take some time for the
                # interfaces to settle into their final state. Thus, update
                # the start time for the measurement to the point at which
                # the WPS button was pressed.
                assoc_start_time=$wps_start_time

                # Clear this as we only want to extend the association time
                # for this one instance. All subsequent ones should be based
                # on the time we detect a disassociation (unless WPS is
                # triggered again).
                wps_start_time=''

                # Clear this flag so that we now use the association timer
                # instead of the WPS one.
                wps_in_progress=0

                wps_stabilization=1

                # We do not want to force reassociation after WPS, so mark
                # it as not needed through this special value.
                reassoc_forced=2
            elif [ "$reassoc_forced" -eq 0 ]; then
                # This is the first time we've associated since repacd started.
                # Do not consider ourselves associated just yet. Instead,
                # return immediately so that in the next attempt we can trigger
                # the reassociation.
                reassoc_forced=1
                return 1
            elif [ "$reassoc_forced" -eq 1 ]; then
                # We've ignored one association detection. Now trigger a new
                # association. This is done to improve the probability that
                # the scan results are complete. We have seen cases where the
                # scan results at initial driver bringup are often missing
                # BSSes that should have been easily detected. Generally a
                # subsequent scan does not have this problem.
                local sta_iface=''
                if [ -n "$sta_iface_24g" ]; then
                    sta_iface="$sta_iface_24g"
                else
                    sta_iface="$sta_iface_5g"
                fi

                if [ "$enable_mlo" -eq 1 ] && __repacd_wifimon_is_mlo_bsta; then
                    __repacd_wifimon_debug "Skip Forced reassociation for MLO bSTA"
                    reassoc_forced=2
                    hyd_restart=1
                    return 1
                fi

                #disconnect is required, as we faced issue that sta interface
                #is not added to bridge if we use reassociate alone.
                if [ "$enable_force_reassoc" -eq 1 ]; then
                    wpa_cli -p "/var/run/wpa_supplicant-$sta_iface" \
                        -i "$sta_iface" disconnect

                    wpa_cli -p "/var/run/wpa_supplicant-$sta_iface" \
                        -i "$sta_iface" reconnect
                fi

                # Forget the BSSID we first had as that may have been a
                # poor association decision due to incomplete scan results
                config_bssid=''

                __repacd_wifimon_debug "Forced reassociation of $sta_iface"
                reassoc_forced=2
                hyd_restart=1
                return 1
            fi

            if [ "$wps_stabilization" -gt 0 ]; then
                wps_assoc_count=$((wps_assoc_count + 1))
                __repacd_wifimon_debug "Assoc post WPS (#$wps_assoc_count)"
            fi

            if __repacd_wifimon_is_assoc_stable; then
                # Assume we are going into the measuring state. This may be
                # overruled if we decide to switch to a different bSTA
                # interface.
                eval "$5=$WIFIMON_STATE_MEASURING"
                assoc_start_time=''
                last_assoc_state=1
                wps_stabilization=0

                # Restart the measurements. We do not remember any past
                # ones as they might not reflect the current state (eg.
                # if the root AP was moved).
                rssi_num=0

                if [ "$wps_stabilization" -gt 0 ]; then
                    # Once WPS completes, switch to the most preferred bSTA.
                    local sta_iface_config=''
                    if [ -n "$sta_iface_24g" ]; then
                        sta_iface_config="$sta_iface_24g_config_name"
                    else
                        sta_iface_config="$sta_iface_5g_config_name"
                    fi

                    # Be optimistic that the most preferred bSTA will have a
                    # good link, so don't force the BSSes down.
                    local force_bsses_down=0
                    __repacd_wifimon_select_next_bsta "$sta_iface_config" \
                        $force_bsses_down \
                        "$WIFIMON_STATE_MEASURING" "$5"

                    # Although we're stable, since in most cases we're going to
                    # change the bSTA radio, we don't want to proceed into the
                    # measuring state below. If it turns out we didn't switch
                    # to a new radio, we'll pick up the measurements in the
                    # next iteration.
                    return 0
                fi
            else
                # Pretend like we are not associated since we need it to be
                # stable.
                return 1
            fi
        fi
        if [ "$MBsta_onboarding" -eq 0 ]; then
            # Restart the hyd once bSTA is associated
            if [ "$hyd_restart" -eq 1 ]; then
                __repacd_restart_hyd
                hyd_restart=0
            fi

            local map_ts_enabled
            config_get map_ts_enabled MAPConfig 'MapTrafficSeparationEnable' '0'
            if [ "$map_fast_onboarding" -eq 1 ] && [ "$map_ts_enabled" -eq 1 ]; then
                if [ -n "$sta_iface_5g" ]; then
                    __repacd_vlanmon_check_sta_vlan "$sta_iface_5g"
                else
                    __repacd_vlanmon_check_sta_vlan "$sta_iface_24g"
                fi
            fi
        fi
        # Association is considered stable. Measure the link RSSI.
        if [ "$rssi_num" -le "$rssi_samples" ] && \
            __repacd_start_ping "$network"; then
            config_load wireless
            config_foreach __repacd_wifimon_set_root_distance wifi-iface
            config_foreach __repacd_wifimon_update_partner_bssid wifi-iface
            __repacd_wifimon_debug "Set the root distance to 255 & stored bssid"
            if [ "$MBsta_onboarding" -eq 1 ]; then
                __repacd_wifimon_debug "MBsta_mlo_mode $MBsta_mlo_mode MBsta_non_mlo_mode $MBsta_non_mlo_mode"
                if [ "$rssi_num" -lt 1 ]; then
                    #We determine the final mode of connection as soon as GW is reachable, only check
                    #for 0th sample cause it may keep varying in some flaky conditions
                    MBsta_non_mlo_sta_connected_list=''
                    MBsta_mlo_sta_connected_list=''
                    config_foreach __repacd_wifimon_MBsta_check_if_mlo_mode wifi-iface
                    if [ "$MBsta_mlo_mode" -eq 1 ] && [ -n "$MBsta_mlo_sta_connected_list" ]; then
                        __repacd_wifimon_debug " RSSI NUM  #$rssi_num let's disconnect Non Mlo sta vaps: $MBsta_non_mlo_sta_iface_list"
                        __repacd_wifimon_MBsta_disconnect_link $MBsta_non_mlo_sta_iface_list
                        MBsta_non_mlo_mode=0
                        uci set repacd.MAPConfig.MBsta_mlo_mode='1'
                        uci set repacd.MAPConfig.MBsta_non_mlo_mode='0'
                        uci commit repacd

                    fi
                    if [ "$MBsta_non_mlo_mode" -eq 1 ] && [ -n "$MBsta_non_mlo_sta_connected_list" ] && [ -z "$MBsta_mlo_sta_connected_list" ]; then
                        __repacd_wifimon_MBsta_disconnect_link $MBsta_mlo_sta_iface_list
                        __repacd_wifimon_debug "DISCONNECT: $sta_if"
                        MBsta_mlo_mode=0
                        uci set repacd.MAPConfig.MBsta_non_mlo_mode='1'
                        uci set repacd.MAPConfig.MBsta_mlo_mode='0'
                        uci commit repacd
                    fi
                else
                    #Always make sure other link we keep disabling to avoid looping in corner case
                    if [ "$MBsta_mlo_mode" -eq 1 ]; then
                        __repacd_wifimon_MBsta_disconnect_link $MBsta_non_mlo_sta_iface_list
                    elif [ "$MBsta_non_mlo_mode" -eq 1 ]; then
                        __repacd_wifimon_MBsta_disconnect_link $MBsta_mlo_sta_iface_list
                    fi
                fi
            if [ "$rssi_num" -ge 2 ]; then
                rssi_num=$((rssi_samples + 1))
                __repacd_wifimon_debug "rssi_num:$rssi_num Not measuring for MLO onboarding anymore"
                return 0
            fi
            if [ "$MBsta_mlo_mode" -eq 1 -o "$MBsta_non_mlo_mode" -eq 1 ]; then
                __repacd_wifimon_measure_link "$network" "$5"
            fi
        else #MBsta_onboarding disabled
            __repacd_wifimon_measure_link "$network" "$5"
        fi
    fi
        return 0
    elif [ "$wps_in_progress" -eq 0 ] && [ "$wps_stabilization" -eq 0 ]; then
        # Record the first time we detected ourselves as not being associated.
        # This will drive a timer in the check function that will change the
        # state if we stay disassociated for too long.
        if [ -z "$assoc_start_time" ]; then
            __repacd_wifimon_get_timestamp assoc_start_time
        fi

        last_assoc_state=0
        eval "$5=$WIFIMON_STATE_NOT_ASSOCIATED"
    elif [ "$wps_in_progress" -gt 0 ]; then
        if [ "$wps_timeout_logged" -eq 0 ] && [ "$assoc_timeout_logged" -eq 0 ]; then
            # Suppress logs after we've timed out
            __repacd_wifimon_debug "Auto config in progress - not assoc"
        fi

        wps_assoc_count=0
        config_get onboarding_done MAPConfig 'OnboardingDone' '0'
        if [ "$onboarding_done" -eq 0 ]; then #Agent gets stuck in wps doesn't switch bands if no controller up
            wps_in_progress=0
            if [ -z "$assoc_start_time" ]; then
                __repacd_wifimon_get_timestamp assoc_start_time
            fi
            last_assoc_state=0
            eval "$5=$WIFIMON_STATE_NOT_ASSOCIATED"
        fi
    fi

    if [ "$associated" -eq 0 ] && [ "$map_fast_onboarding" -eq 1 ]; then
        config_load repacd
        config_get map_ts_active MAPConfig 'MapTrafficSeparationActive' '0'
        config_get map_backhaul_nw MAPConfig 'VlanNetworkBackHaul'
        config_get map_primary_nw MAPConfig 'VlanNetworkPrimary'

        # Resolve the STA interfaces.
        if [ "$map_ts_active" -gt 0 ]; then
            __repacd_wifimon_get_sta_info "$map_backhaul_nw"
        else
            __repacd_wifimon_get_sta_info "$map_primary_nw"
        fi
    fi

    # Restart the hyd once bSTA is associated.
    hyd_restart=1
    # Not associated and WPS is in progress. No LED update.
    return 1
}

# Find the median from the samples file.
# This is a crude way to compute the median when the number of
# samples is odd. It is not strictly correct for an even number
# of samples since it does not compute the average of the two
# samples in the middle and rather just takes the lower one, but
# this should be sufficient for our purposes. The average is not
# performed due to the values being on the logarithmic scale and
# because shell scripts do not directly support floating point
# arithmetic.
# input: $1 - Samples filename
# input: $2 - Number of samples in the samples file
# output: $3 - computed Median
__repacd_wifimon_compute_median() {
    local median median_index
    local samples_filename="$1"

    median_index=$((($2 + 1) / 2))
    median=$(sort -n < "$samples_filename" | head -n $median_index | tail -n 1)

    eval "$3=$median"
}

# Update what the serving BSSID is for the given interface.
# input: $1 - sta_iface: the interface for which to update the BSSID
__repacd_wifimon_update_serving_bssid() {
    local sta_iface="$1"
    current_bssid=$(repacdcli "$sta_iface" get_sta_link)
}

# Determine if the serving BSSID has changed from the one provided.
#
# Note that __repacd_wifimon_update_serving_bssid must have been
# called prior to this function.
#
# input: $1 - old_bssid: the previous BSSID to compare the current one
#                        against
#
# return 0 on a BSSID change; otherwise non-zero
__repacd_wifimon_is_bssid_change() {
    local old_bssid="$1"

    if [ -n "$old_bssid" ] && [ "$old_bssid" != "$current_bssid" ]; then
        # Not the same as our previous valid one, so consider this a change
        return 0
    fi

    # No change or our old BSSID is invalid
    return 1
}


__repacd_wifimon_check_cac () {
    local config="$1"
    local iface mld

    config_get iface "$config" ifname
    config_get mld "$config" mld
    local interface="$(iw dev $iface info | grep ssid | awk -F " " '{print $2}')"

    local bitRate=$(repacdcli $iface get_bitrate)

        if [ -n "$iface" ]; then
            if [ -n "$interface" ] && [ -n "$bitRate" ] && [ "$bitRate" = "0" ]; then
                #interface is associated but bitrate 0
                #check cac
                # bitRate will be 0 when CAC is in progress.
                local isCacInProgress=$(cfg80211tool $iface get_cac_state)
                isCacInProgress=${isCacInProgress#*:}
                if [ "$isCacInProgress" -eq 1 ]; then
                    __repacd_wifimon_debug "CAC IN PROGRESS FOR $iface*"
                    eval "$2=0"
                    break
                fi
            fi
        fi
}
__repacd_wifimon_check_cac_status () {
    local return
    config_load wireless
    cac_state=0
    #Let's not check for cac if config not enabled
    if [ "$pre_cac_enabled" -eq 1 ]; then
        config_foreach __repacd_wifimon_check_cac wifi-iface return
        if [ "$return" -eq 0 ]; then
            cac_state=1
            #Set the variable to 1 when cac is running
            #reset only from repacd-run
            EU_cac_state=1
            return 0
        fi
    fi

    return 1
}

__repacd_wifimon_get_mld_bstas() {
    local config="$1"
    local check_disabled_option="$2"
    local iface network mode map
    local map_type mld device

    local disabled
    config_get iface "$config" ifname
    config_get network "$config" network
    config_get mode "$config" mode
    config_get map "$config" map '0'
    config_get map_type "$config" MapBSSType
    config_get mld "$config" mld
    config_get device "$config" device
    config_get disabled "$config" disabled 0

    if [ "$check_disabled_option" -eq 1 ]; then
        if [ "$mode" = "sta" ] && \
            [ "$map" -gt 0 ]; then
            if [ "$map_type" -eq 128 ] && [ -n "$mld" ] && [ "$disabled" -ne 1 ]; then  # bSTA
                __repacd_wifimon_debug "device : $device , iface : $iface"
                mldbsta_list="$mldbsta_list $device"
                mldbsta_iface_list="$mldbsta_iface_list $device $iface"
                mldbsta_config_name_list="$mldbsta_config_name_list $config"
                mld_mlo_group=$mld
            fi
        fi
    else
        if [ "$mode" = "sta" ] && \
            [ "$map" -gt 0 ]; then
            if [ "$map_type" -eq 128 ] && [ -n "$mld" ]; then  # bSTA
                __repacd_wifimon_debug "device : $device , iface : $iface"
                mldbsta_list="$mldbsta_list $device"
                mldbsta_iface_list="$mldbsta_iface_list $device $iface"
                mldbsta_config_name_list="$mldbsta_config_name_list $config"
                mld_mlo_group=$mld
            fi
        fi
    fi
}

# Measure the RSSI to the serving AP and update the state accordingly.
# input: $1 - network: the name of the network being monitored
# output: $2 - state: the variable to update with the new state name (if there
#                     was a change)
__repacd_wifimon_measure_link() {
    local rssi enable_fastcloning

    config_load repacd
    config_get enable_fastcloning MAPConfig 'EnableCloningOptimization' '0'
    config_get bsta_link_remove_active MAPConfig 'bstaLinkRemoveActive' '0'

    if [ "$maplite_enabled" -ne 1 ]; then
        if ! __repacd_is_gw_reachable; then
            if [ -n "$last_ping_gw_ip" ]; then
                __repacd_wifimon_debug "GW ${last_ping_gw_ip} not reachable"
            else
                __repacd_wifimon_debug "GW unknown"
            fi
            return
        fi

        if [ "$rssi_num" -eq 0 ] && [ "$MBsta_onboarding" -eq 0 ]; then
            if [ "$measuring_cnt" -gt 0 ]; then
                __repacd_wifimon_debug "Measurement failed attempt # $measuring_cnt"
            fi
            measuring_cnt=$((measuring_cnt + 1))
        fi
    fi

    if [ "$enable_mlo" -eq 1 ]; then
        __repacd_wifimon_debug "Monitoring bsta link addition/removal"
        mldbsta_list=''
        mldbsta_iface_list=''
        mldbsta_config_name_list=''
        local check_disabled=0
        config_load wireless
        config_foreach __repacd_wifimon_get_mld_bstas wifi-iface $check_disabled
        __repacd_wifimon_debug "mldbsta_config_name_list : $mldbsta_config_name_list"
        __repacd_wifimon_add_bsta_link
        config_load wireless
        check_disabled=1
        mldbsta_list=''
        mldbsta_iface_list=''
        mldbsta_config_name_list=''
        config_foreach __repacd_wifimon_get_mld_bstas wifi-iface $check_disabled
        __repacd_wifimon_debug "mldbsta_config_name_list : $mldbsta_config_name_list"
        __repacd_wifimon_remove_bsta_link
        config_load repacd
        config_get bsta_link_remove_active MAPConfig 'bstaLinkRemoveActive' '0'
    fi

    if [ "$MBsta_onboarding" -eq 1 ]; then
        if [ "$measuring_cnt" -gt 0 ]; then
            __repacd_wifimon_debug "Measurement failed attempt # $measuring_cnt, Switching link"
            measuring_cnt=0 #We make it zero
            if [ "$MBsta_non_mlo_mode" -eq 1 ] && [ "$MBsta_mlo_mode" -eq 0 ]; then
                __repacd_wifimon_debug "Failed Attempt count switching to MLO"
                eval "$2=$WIFIMON_STATE_RE_SWITCH_MLO_BSTA"
                return
            elif [ "$MBsta_non_mlo_mode" -eq 0 ] && [ "$MBsta_mlo_mode" -eq 1 ] && [ "$backup_link" -eq 1 ]; then
                eval "$2=$WIFIMON_STATE_RE_SWITCH_BSTA"
                MLOtolegacyvariable="true"
                __repacd_wifimon_debug "Failed Attempt Switching to Non MLO"
                MBsta_invalid_rssi_count=$((MBsta_invalid_rssi_count + 1))
                __repacd_wifimon_debug "Invalid RSSI count in MLO: $MBsta_invalid_rssi_count"
                if [ "$MBsta_invalid_rssi_count" -ge 2 ]; then
                    MBsta_short_timer_used=1
                    uci set repacd.MAPConfig.MBsta_shorttimerused='1'
                    uci commit repacd
                fi
                return
            fi
            if [ "$backup_link" -eq 0 ]; then
                __repacd_wifimon_debug "Skip switching to backup link on RSSI measurement failure as there is no backup link"
            fi
        fi
    fi

    # If we go beyond the maximum attempt count (+1 due to the increment
    # above), that must mean we have no other radios available. Thus, we
    # might as well just proceed in the hope we're able to collect valid
    # measurements.
    if [ "$measuring_cnt" -eq $(($measuring_attempts + 1)) ]; then
        # Could not complete measurements with this interface as a bSTA.
        #
        # Try switching to the next most preferred interface.
        # Since this interface was so flaky, the next one might be too, so
        # tell the fronthaul manager to force the BSSes down at the next
        # startup.
        local force_bsses_down=1

        # if the bSTA is MLO, do not try to switch the bSTA
        if ! __repacd_wifimon_is_mlo_bsta; then
            if  __repacd_wifimon_select_next_bsta "$sta_iface_config_name" \
                    $force_bsses_down "$WIFIMON_STATE_RE_BACKHAUL_POOR" "$2"; then
                # New radio was selected
                measuring_cnt=0
                return
            fi
        fi

        # Otherwise, no other radios. Still fall through to try to get a
        # measurement.
    fi

    # Currently only one STA interface is allocated at a time. Determine the
    # active one so that it can be used to check the RSSI.
    local sta_iface=''
    local rssi_threshold=''
    local rssi_led_state=''  # state of LEDs if above RSSI threshold

    if [ "$MBsta_onboarding" -eq 1 ]; then
        if [ "$rssi_num" -le 2 ]; then
            if [ "$rssi_num" -eq 0 ]; then
                ping_started=1
            fi
            if [ "$MBsta_non_mlo_mode" -eq 1 ] && [ "$MBsta_mlo_mode" -ne 1 ]; then
                __repacd_wifimon_debug "MEASURING FOR NON MLO $MBsta_non_mlo_sta_connected_list"
                __repacd_wifimon_MBsta_measure_rssi $MBsta_non_mlo_sta_connected_list
            elif [ "$MBsta_mlo_mode" -eq 1 ]; then
                __repacd_wifimon_debug "MEASURING FOR MLO $MBsta_mlo_sta_connected_list"
                __repacd_wifimon_MBsta_measure_rssi $MBsta_mlo_sta_connected_list
            fi
        fi
        __repacd_stop_ping
        #MLO ONBOARDING FR
        __repacd_wifimon_debug "rssi_num $rssi_num"
        if [ "$rssi_num" -eq 2 ]; then
            if [ "$MBsta_mlo_mode" -eq 1 ]; then
                uci set repacd.MAPConfig.MLObSTAconfigured='1'
                uci_commit repacd
            fi
            __repacd_wifimon_check_cac_status
            #Doesn't matter if backup link was weak or goo we still don't switch to MLO just directly
            if [ "$MBsta_non_mlo_mode" -eq 1 ] && [ "$MBsta_mlo_mode" -ne 1 ]; then
                if [ "$MBsta_rssi_valid" -eq 1 ]; then
                    if __repacd_wifimon_is_assoc $sta_iface_24g; then
                        local rssi_24g=$(repacdcli $sta_iface_24g get_signal)
                        local keep_24g_bsta_threshold=$((backhaul_rssi_2 + backhaul_rssi_offset))
                        __repacd_wifimon_debug "rssi_24g $rssi_24g, keep_24g_bsta_threshold $keep_24g_bsta_threshold"
                    fi
                    if [ "$rssi_24g" -lt "$keep_24g_bsta_threshold" ] && [ "$cac_state" -eq 0 ]; then
                        eval "$2=$rssi_led_state"
                        MBsta_mlo_mode=0
                        MBsta_non_mlo_mode=1
                        uci set repacd.MAPConfig.MBsta_mlo_mode='0'
                        uci set repacd.MAPConfig.MBsta_non_mlo_mode='1'
                        uci commit repacd
                        if [ "$MBsta_short_timer_used" -eq 1 ]; then
                            __repacd_wifimon_debug "BACKHAUL is FAIR, let's try bsta switching only post long timer expires"
                            MBsta_mlo_attempt_time_duration="$MBsta_long_timer_duration"
                        else #attempt mlo post short timer
                            __repacd_wifimon_debug "BACKHAUL is FAIR, let's try bsta switching only post short timer expires"
                            MBsta_mlo_attempt_time_duration="$MBsta_short_timer_duration"
                        fi
                        __repacd_wifimon_get_timestamp MBsta_mlo_attempt_start_time
                    elif [ "$cac_state" -eq 1 ] || [ "$onboarding_done" -eq 0 ]; then
                        __repacd_wifimon_debug "Let's stay in current link since Cac is going on"
                        eval "$2=$rssi_led_state"
                        MBsta_mlo_mode=0
                        MBsta_non_mlo_mode=1
                        uci set repacd.MAPConfig.MBsta_mlo_mode='0'
                        uci set repacd.MAPConfig.MBsta_non_mlo_mode='1'
                        uci commit repacd
                    elif [ "$cac_state" -eq 0 ] && [ "$onboarding_done" -gt 0 ]; then
                        #Let's switch to MLO based on short/long timer
                        config_get MBsta_short_timer_used MAPConfig 'MBsta_shorttimerused' 0
                        eval "$2=$rssi_led_state"
                        if [ "$MBsta_short_timer_used" -eq 0 ]; then
                            __repacd_wifimon_debug "STARTING SHORT TIMER FOR ATTEMPTING MLO CONNECTION IN $MBsta_short_timer_duration sec"
                            MBsta_mlo_attempt_time_duration="$MBsta_short_timer_duration"
                            #MBsta_short_timer_used=1
                            #uci set repacd.MAPConfig.MBsta_shorttimerused='1'
                            #uci commit repacd
                        else
                            __repacd_wifimon_debug "STARTING LONG TIMER FOR ATTEMPTING MLO CONNECTION IN $MBsta_long_timer_duration sec"
                            MBsta_mlo_attempt_time_duration="$MBsta_long_timer_duration"
                        fi
                        __repacd_wifimon_MBsta_disconnect_link $MBsta_mlo_sta_iface_list
                        __repacd_wifimon_debug "We disconnnected MLO links since 2g became stable & mlo didn't associate"
                        MBsta_mlo_mode=0
                        MBsta_non_mlo_mode=1
                        uci set repacd.MAPConfig.MBsta_mlo_mode='0'
                        uci set repacd.MAPConfig.MBsta_non_mlo_mode='1'
                        uci commit repacd
                        __repacd_wifimon_get_timestamp MBsta_mlo_attempt_start_time
                    fi
                else
                    #If any Backup link has poor rssi it will try mlo post some
                    #timer expiry. This is to avoid ping pong between links
                    eval "$2=$WIFIMON_STATE_RE_BACKHAUL_POOR"
                    MBsta_mlo_mode=0
                    MBsta_non_mlo_mode=1
                    uci set repacd.MAPConfig.MBsta_mlo_mode='0'
                    uci set repacd.MAPConfig.MBsta_non_mlo_mode='1'
                    uci commit repacd
                    if [ "$cac_state" -eq 0 ]; then
                        if [ "$MBsta_short_timer_used" -eq 1 ]; then
                            __repacd_wifimon_debug "BACKHAUL is poor, let's try bsta switching only post long timer expires"
                            MBsta_mlo_attempt_time_duration="$MBsta_long_timer_duration"
                        else #attempt mlo post short timer
                            __repacd_wifimon_debug "BACKHAUL is poor, let's try bsta switching only post short timer expires"
                            MBsta_mlo_attempt_time_duration="$MBsta_short_timer_duration"
                        fi
                            __repacd_wifimon_get_timestamp MBsta_mlo_attempt_start_time
                    else
                        __repacd_wifimon_debug "CAC is going on. Attempt MLO post CAC completes."
                    fi
                fi

            elif [ "$MBsta_mlo_mode" -eq 1 ]; then
                if [ "$MBsta_rssi_valid" -eq 0 ] && [ "$backup_link" -eq 1 ]; then
                    __repacd_wifimon_Debug "MLO BH POOR, but stay in MLO"
                    eval "$2=$WIFIMON_STATE_RE_BACKHAUL_POOR"
                else
                    #RE_BACKHAUL_GOOD
                    eval "$2=$rssi_led_state"
                fi
                MBsta_non_mlo_mode=0
                MBsta_mlo_mode=1
                uci set repacd.MAPConfig.MBsta_mlo_mode='1'
                uci set repacd.MAPConfig.MBsta_non_mlo_mode='0'
                uci commit repacd
            fi
            local intf=''
            local interface_list=''
            if [ "$MBsta_mlo_mode" -eq 1 ]; then
                interface_list="$MBsta_mlo_sta_connected_list"
            elif  [ "$MBsta_non_mlo_mode" -eq 1 ]; then
                interface_list="$MBsta_non_mlo_sta_connected_list"
            fi
            local count=0
            for intf in $interface_list; do
                current_bssid=$(repacdcli "$sta_iface" get_sta_link)
                if [ -n "$current_bssid" -a "$current_bssid" != "Not-Associated" ]; then
                    __repacd_wifimon_MBsta_set_target_bssid "$current_bssid" "$current_bssid" "$intf"
                fi
                if [ "$count" -lt 1 ]; then
                        MBsta_current_bssid="$current_bssid"
                fi
            done
        fi
        __repacd_wifimon_debug "MBsta_onboarding enabled"
        return
    fi
    if [ "$MBsta_onboarding" -eq 0 ]; then
        if [ -n "$sta_iface_5g" ]; then
            sta_iface="$sta_iface_5g"
            sta_iface_config_name="$sta_iface_5g_config_name"
            rssi_threshold="$backhaul_rssi_5"
            rssi_led_state="$WIFIMON_STATE_RE_BACKHAUL_GOOD"
        else
            sta_iface="$sta_iface_24g"
            sta_iface_config_name="$sta_iface_24g_config_name"
            rssi_threshold="$backhaul_rssi_2"
            rssi_led_state="$WIFIMON_STATE_RE_BACKHAUL_FAIR"
        fi
        rssi=$(repacdcli $sta_iface get_signal)
        if [ "$MBsta_onboarding" -eq 1 ]; then
            __repacd_wifimon_debug "rssi $rssi rssi_num $rssi_num $max_valid_backhaul_rssi max_valid_backhaul_rssi"
        fi
        # We explicitly ignore clearly bogus values. -95 dBm has been seen in
        # some instances where the STA is not associated by the time the RSSI
        # check is done. The check against 0 tries to guard against scenarios
        # where the firmware has yet to report an RSSI value (although this may
        # never happen if the RSSI gets primed through the association messaging).
        if [ "$rssi" -gt "$max_valid_backhaul_rssi" ] && [ "$rssi" -lt 0 ]; then
            if [ "$rssi_num" -lt "$rssi_samples" ]; then
                __repacd_wifimon_debug "RSSI sample #$rssi_num = $rssi dBm"

                # Ignore the very first sample since it is taken at the same time
                # the ping is started (and thus the RSSI might not have been
                # updated).
                if [ "$rssi_num" -eq 0 ]; then
                    rssi_filename=$(mktemp /tmp/repacd-rssi.XXXXXX)
                    ping_started=1
                else
                    # Not the first sample
                    echo "$rssi" >> "$rssi_filename"
                fi
                rssi_num=$((rssi_num + 1))
            elif [ "$rssi_num" -eq "$rssi_samples" ]; then
                __repacd_wifimon_debug "RSSI sample #$rssi_num = $rssi dBm"

                # We will take one more sample and then draw the conclusion.
                # No further measurements will be taken (although this may be
                # changed in the future).
                echo "$rssi" >> "$rssi_filename"

                # We got the required number of samples, now derive the median rssi.
                local rssi_median
                __repacd_wifimon_compute_median "$rssi_filename" $rssi_num rssi_median
                __repacd_wifimon_debug "Median RSSI = $rssi_median dBm"
                __repacd_wifimon_debug "RSSI threshold = $rssi_threshold dBm"

                measuring_cnt=0
                rssi_num=$((rssi_num + 1))  # to prevent future samples

                # We have our measurement, so the ping is no longer needed.
                __repacd_stop_ping

                # If we are on 2.4 GHz and the serving BSSID changed, reduce the
                # the attempt count on 5 GHz and trigger a new measurement.
                # Hopefully this means there is a new AP in the network that would
                # be able to provide us better upstream connectivity.
                if [ "$sta_iface" = "$sta_iface_24g" ]; then
                    if __repacd_wifimon_is_bssid_change "$config_bssid"; then
                        __repacd_wifimon_debug "BSSID change from $config_bssid to $current_bssid; allow 5 GHz attempt"
                        if [ "$cnt_5g_attempts" -gt 0 ]; then
                            cnt_5g_attempts=$((cnt_5g_attempts - 1))
                        fi
                    fi
                fi

                # In case we disassociate after this, we will want to start the
                # association timer again, so clear our state of the last time we
                # started it so that it can be started afresh upon disassociation.
                assoc_start_time=''
                # check if we are configured skip threshold checking for Backhaul steering
                # and if Backhaul steering progress flag is set, then skip threshold checking
                local skip_threshold_for_BHS=$(uci get repacd.MAPConfig.SkipThresholdForBHS)
                local bhs_in_progress=$(uci get ezmesh.MultiAP.BHSInProgress)
                local skip_threshold_checking=0
                if [[ $skip_threshold_for_BHS -eq 1 ]] && [[ $bhs_in_progress -eq 1 ]]; then
                    __repacd_wifimon_debug "Skipping RSSI threshold check for Backhaul steering"
                    skip_threshold_checking=1
                fi
                # clear BHSInProgress if it was set
                if [[ $bhs_in_progress -eq 1 ]]; then
                    (uci set ezmesh.MultiAP.BHSInProgress='0'; uci commit ezmesh)
                    __repacd_wifimon_debug "cleared BHSInProgress flag"
                fi
                # now check if backhaul steering is in progress
                if [ $skip_threshold_checking -eq 1 ] || [ "$rssi_median" -gt "$rssi_threshold" ]; then
                    local keep_24g_bsta_threshold=$((rssi_threshold + backhaul_rssi_offset))
                    if [ "$sta_iface" = "$sta_iface_24g" ]; then
                        __repacd_wifimon_check_cac_status
                        if [ "$cnt_5g_attempts" -ge "$max_5g_attempts" ] ||
                            [ "$rssi_median" -lt "$keep_24g_bsta_threshold" ]; then
                            eval "$2=$rssi_led_state"
                        elif [ "$cnt_5g_attempts" -lt "$max_5g_attempts" ] && [ "$cac_state" -eq 1 ]; then
                            echo "STABLE IN 2G since CAC going on" >/dev/console
                            eval "$2=$rssi_led_state"
                            return
                        else
                            if [ "$maplite_enabled" -ne 1 ]; then
                                # 2.4 GHz is good enough that 5 GHz might be viable. Try it again.
                                __repacd_wifimon_debug "2.4 GHz good; considering 5 GHz"

                                if [ "$enable_mlo" -eq 1 ] && __repacd_wifimon_is_mlo_bsta; then
                                    __repacd_wifimon_debug "MLO bSTA is configured; skip bSTA switch"
                                    return
                                fi
                                # Let's be optimistic that 5 GHz will be good enough
                                # and thus we should leave the BSSes up at startup.
                                local force_bsses_down=0
                                __repacd_wifimon_select_next_bsta "$sta_iface_config_name" \
                                    $force_bsses_down "$rssi_led_state" "$2"
                                return
                            else
                                if [ "$enable_mlo" -eq 1 ] && __repacd_wifimon_configure_mld_bsta; then
                                    __repacd_wifimon_debug "Legacy bSTA is stable and switch to MLO bSTA"
                                    eval "$2=$WIFIMON_STATE_RE_SWITCH_MLO_BSTA"
                                    config_foreach __repacd_wifimon_get_mld_bsta wifi-iface
                                    current_sta_list="$sta_device $sta_iface"
                                    create_partner_bsta=1
                                fi
                            fi
                        fi
                    else
                        __repacd_wifimon_check_cac_status
                        if [ "$cnt_mlo_attempts" -ge "$max_mlo_attempts" ]; then
                            eval "$2=$rssi_led_state"
                        elif [ "$enable_mlo" -eq 1 ] && [ "$cac_state" -eq 0 ] && [ "$bsta_link_remove_active" -eq 0 ]; then
                            echo "CONFIGURING MLO*cac_state $cac_state" >/dev/console
                            if __repacd_wifimon_configure_mld_bsta; then
                                __repacd_wifimon_debug "Legacy bSTA is stable and switch to MLO bSTA"
                                eval "$2=$WIFIMON_STATE_RE_SWITCH_MLO_BSTA"
                                config_foreach __repacd_wifimon_get_mld_bsta wifi-iface
                                current_sta_list="$sta_device $sta_iface"
                                create_partner_bsta=1
                            else
                                eval "$2=$rssi_led_state"
                            fi
                        else
                            # 5 GHz bSTA with sufficient; we are done
                            if [ "$maplite_enabled" -eq 1 ]; then
                                if [ "$enable_mlo" -eq 1 ] && __repacd_wifimon_configure_mld_bsta; then
                                    __repacd_wifimon_debug "Legacy bSTA is stable and switch to MLO bSTA"
                                    eval "$2=$WIFIMON_STATE_RE_SWITCH_MLO_BSTA"
                                    config_foreach __repacd_wifimon_get_mld_bsta wifi-iface
                                    current_sta_list="$sta_device $sta_iface"
                                    create_partner_bsta=1
                                fi
                            else
                                eval "$2=$rssi_led_state"
                            fi
                        fi
                    fi
                elif [ "$sta_iface" = "$sta_iface_24g" ]; then
                    if [ "$maplite_enabled" -eq 1 ]; then
                        if [ "$enable_mlo" -eq 1 ] && __repacd_wifimon_configure_mld_bsta; then
                            __repacd_wifimon_debug "Legacy bSTA is stable and switch to MLO bSTA"
                            eval "$2=$WIFIMON_STATE_RE_SWITCH_MLO_BSTA"
                            config_foreach __repacd_wifimon_get_mld_bsta wifi-iface
                            current_sta_list="$sta_device $sta_iface"
                            create_partner_bsta=1
                        fi
                    else
                        __repacd_wifimon_debug "2.4 GHz bSTA weak; not re-attempting 5 GHz"
                        eval "$2=$WIFIMON_STATE_RE_BACKHAUL_POOR"
                    fi
                else
                    # Try next best bSTA radio
                    __repacd_wifimon_debug "5 GHz bSTA too weak; trying next radio"

                    if [ "$cnt_mlo_attempts" -ge "$max_mlo_attempts" ]; then
                        eval "$2=$rssi_led_state"
                    elif [ "$onboarding_done" -eq 0 -a "$enable_fastcloning" -eq 1 ]; then
                        eval "$2=$rssi_led_state"
                        __repacd_wifimon_debug "Cloning is not done. Do not switch BSTA based on signal strength"
                        return
                    elif  [ "$enable_mlo" -eq 1 ] && __repacd_wifimon_is_mlo_bsta; then
                        __repacd_wifimon_debug "MLO bSTA is configured; skip bSTA switch"
                        config_load wireless
                        config_foreach __repacd_set_valid_root_distance wifi-iface
                        eval "$2=$rssi_led_state"
                        return
                    fi

                    #while switching the BSTA, same wirless section is reused for the next BSTA which is in other radio
                    #this leads to using old band's BSSID and stored in config_bssid so, clearing it before BH switch
                    __repacd_wifimon_debug "update the sta BSSID to wildcard as same config section will be used for next BSTA"
                    uci set wireless."$sta_iface_5g_config_name".bssid="$WIFIMON_WILDCARD_BSSID"
                    uci commit wireless

                    # Until we know we can get a good bSTA link, let's be
                    # conservative and bring the BSSes down at startup.
                    local force_bsses_down=1
                    __repacd_wifimon_select_next_bsta "$sta_iface_config_name" \
                        $force_bsses_down "$WIFIMON_STATE_RE_BACKHAUL_POOR" "$2"
                    return
                fi

                # We have completed the link evaluation, so lock in on that BSSID.
                # This ensures that even if the link briefly goes down, we always
                # reconnect to the same agent.
                __repacd_wifimon_set_target_bssid "$current_bssid" "$current_bssid"
            fi
        else
            __repacd_wifimon_debug "RSSI $rssi is out of valid range (0 to $max_valid_backhaul_rssi)"
        fi
    fi
}

# Determine the radio that is next most preferred radio.
#
# If the current radio's preference value is not set, pick the highest
# preference radio.
#
# input: $1 config: section name
# input: $2 cur_pref: the preference level for the current bSTA
# output: $3 preferred_radio: the radio with the highest preference, but below
#                             the cur_pref value if it is set
__repacd_wifimon_resolve_next_bsta_radio() {
    local config="$1"
    local cur_pref="$2"

    local bsta_preference=''
    config_get bsta_preference "$config" repacd_map_bsta_preference 0

    # Skip radios that have no preference set. Maybe the OEM never wants
    # to use that radio.
    if [ -n "$bsta_preference" ]; then
        if [ -n "$cur_pref" ]; then
            if [ "$bsta_preference" -lt "$cur_pref" ] &&
                [ "$bsta_preference" -gt "$bsta_max_preference" ]; then
                eval "$3=$config"
                bsta_max_preference="$bsta_preference"
            fi
        else
            if [ "$bsta_preference" -gt "$bsta_max_preference" ]; then
                eval "$3=$config"
                bsta_max_preference="$bsta_preference"
            fi
        fi
    fi
}

# Mark the desired radio as selected for the bSTA.
#
# If a radio is marked as selected (using the repacd_map_bsta_selected config
# option), it will be used. If instead none is marked, the radio with the
# highest repacd_map_bsta_pref value will be used.
#
# input: $1 config: section name
# input: $2 selected_radio: the radio that should be selected
__repacd_wifimon_select_bsta_radio() {
    local config="$1"
    local selected_radio="$2"

    if [ "$config" = "$selected_radio" ]; then
        uci_set wireless "$config" repacd_map_bsta_selected '1'
        global_preferred_radio="$selected_radio"
    else
        uci_set wireless "$config" repacd_map_bsta_selected '0'
    fi
}

# Fetch the configured MLO bSTA related information
#
# resolve the MLO bSTA's device, interface name and config section name
# and form the list to check if the MLO bSTA is configured or not
#
# input: $1 config: section name
__repacd_wifimon_get_mld_bsta() {
    local config="$1"

    local iface network mode map
    local map_type mld device

    local disabled
    config_get iface "$config" ifname
    config_get network "$config" network
    config_get mode "$config" mode
    config_get map "$config" map '0'
    config_get map_type "$config" MapBSSType
    config_get mld "$config" mld
    config_get device "$config" device
    config_get disabled "$config" disabled 0

    if [ "$mode" = "sta" ] && \
        [ "$map" -gt 0 ]; then
        if [ "$map_type" -eq 128 ] && [ -n "$mld" ] && [ "$disabled" -ne 1 ]; then  # bSTA
            mldbsta_list="$mldbsta_list $device"
            mldbsta_iface_list="$mldbsta_iface_list $device $iface"
            mldbsta_config_name_list="$mldbsta_config_name_list $config"
            mlo_device_count=$((mlo_device_count+1))
            mld_mlo_group=$mld
        fi
    fi
}

# Check if the MLO bSTA is configured or not
#
# return: 0 on MLO bSTA found; non-zero if not found
__repacd_wifimon_is_mlo_bsta() {
    mldbsta_list=''
    mldbsta_iface_list=''
    mldbsta_config_name_list=''
    mldbsta_enabled=0
    mlo_device_count=0
    
    if [ "$MBsta_onboarding" -eq 0 ]; then
        config_load wireless
        config_foreach __repacd_wifimon_get_mld_bsta wifi-iface

        if [ -n "$mldbsta_list" ]; then
            if [ "$mlo_device_count" -gt 1 ];then
                mldbsta_enabled=1
                return 0
            fi
        fi
    elif [ "$MBsta_onboarding" -eq 1 ] && [ "$MBsta_mlo_mode" -eq 1 ]; then
        mldbsta_enabled=1
        return 0
    fi

    return 1
}

# Resolve the MLO backhaul BSS that is currently configured
#
# input: $1 config: section name
# output: $2 ssid: SSID of the MLO backhaul BSS
__repacd_wifimon_get_mld_bbss_ssid() {
    local config="$1"

    local iface network mode map
    local map_type mld ssid device

    config_get ssid "$config" ssid
    config_get iface "$config" ifname
    config_get mode "$config" mode
    config_get map "$config" map '0'
    config_get map_type "$config" MapBSSType
    config_get mld "$config" mld
    config_get device "$config" device

    if [ "$map" -gt 0 ] && [ "$mode" = "ap" ]; then
        local bbss=$(( map_type & 64 ))
        if [ ${bbss} -eq 64 ] && [ -n "$mld" ]; then  # bbss
            if [ "$enable_slo" -eq 1 ]; then
                local addApMld=$(echo $bh_mld_list | grep -w $mld)
                if [ -z "$addApMld" ]; then
                    bh_mld_list="$bh_mld_list $mld"
                else
                    eval "$2='$ssid'"
                    eval "$3='$mld'"
                    return 0
                fi
            else
                mldbbss_list="$mldbbss_list $device"
                eval "$2='$ssid'"
            fi
        fi
    fi
}

# Resolve the backhaul BSS encrption
#
# input: $1 config: section name
# output: $2 encrption: SSID of the MLO backhaul BSS
__repacd_wifimon_get_bbss_encrpytion() {
    local config="$1"

    local iface network mode map
    local map_type mld ssid encryption

    config_get iface "$config" ifname
    config_get mode "$config" mode
    config_get map "$config" map '0'
    config_get map_type "$config" MapBSSType
    config_get mld "$config" mld
    config_get encryption "$config" encryption

    if [ "$map" -gt 0 ] && [ "$mode" = "ap" ]; then
        local bbss=$(( map_type & 64 ))
        if [ ${bbss} -eq 64 ] && [ -n "$mld" ]; then  # bbss
            if [ "$enable_slo" -eq 1 ]; then
                eval "$2='$encryption'"
            fi
        fi
    fi
}

# Resolve the MLO bsta that is currently configured
#
# input: $1 config: section name
# output: $2 ssid: SSID of the MLO bsta
__repacd_wifimon_get_bsta_ssid() {
    local config="$1"

    local iface network mode map
    local ssid

    config_get ssid "$config" ssid
    config_get mode "$config" mode
    config_get map "$config" map '0'

    if [ "$map" -gt 0 ] && [ "$mode" = "sta" ] && [ $ssid != "00_"* ]; then
        eval "$2='$ssid'"
        return 0
    fi
}

__repacd_wifimon_is_ap_configured() {
    local config="$1"

    local iface network mode map
    local ssid

    config_get ssid "$config" ssid
    config_get mode "$config" mode
    config_get map "$config" map '0'

    if [ "$map" -gt 0 ] && [ "$mode" = "ap" ]; then
        if [[ "$ssid" != *"OpenWrt"* ]] && [[ "$ssid" != *"Openwrt"* ]]; then
            eval "$2='$ssid'"
            return 0
        fi
    fi
}

# Resolve the MLD group section from ssid
#
# Fetch the MLD section name from MLD SSID and MLD Role
#
# input: $1 config: section name
# input: $2 ssid_to_match: MLO SSID to match the MLD Section
# input: $3 role_to_match: MLD Role to match the MLD Section
# output: $4 mld_section: MLD Section name
__repacd_wifimon_get_mld_device() {
    local config="$1"
    local ssid_to_match="$2"
    local role_to_match="$3"

    local mld_ssid role

    config_get mld_ssid "$config" mld_ssid
    config_get role "$config" role

    if [ "$mld_ssid" == "$ssid_to_match" ] && \
        [ "$role" == "$role_to_match" ]; then
        eval "$4=$config"
    fi
}

__repacd_wifimon_get_mld_bbss_device() {
    local config="$1"
    local mld_to_match="$2"

    local mld device

    config_get mld "$config" mld
    config_get device "$config" device

    if [ "$mld" == "$mld_to_match" ]; then
        mldbbss_list="$mldbbss_list $device"
    fi
}

# Count total number of Vaps configured
__repacd_map_check_get_total_vap() {
    total_vap_count=$((total_vap_count + 1))
}


# Configure the MLO bSTA based on backhaul stablization
#
# create a partner bSTA interface and configure MLD group accordingly
#
# output: 0 on MLD bSTA configured; non-zero otherwise
__repacd_wifimon_configure_mld_bsta() {
    local backhaul_mldssid='' mld_device='' backhaul_mld='' onboarding_isdone
    local bsta_mldssid='' bsta_ssid=''
    local sta_mld='' backhaul_encryption=''
    local primaryRadio=''
    local curStaIdx mldStaIdx=0
    config_load repacd
    config_get map_version MAPConfig 'MapVersionEnabled' '0'
    config_get_bool maplite_enabled MAPConfig 'EnableLiteMode' '0'

    mldbbss_list=''
    bh_mld_list=''
    config_load wireless
    config_foreach __repacd_wifimon_get_mld_bbss_ssid wifi-iface \
        backhaul_mldssid backhaul_mld

    if [ "$enable_slo" -eq 1 -a -n "$backhaul_mld" ]; then
        config_foreach __repacd_wifimon_get_mld_bbss_device wifi-iface \
            "$backhaul_mld"
    fi

    config_get onboarding_isdone MAPConfig 'OnboardingDone' '0'

    __repacd_wifimon_debug "mldbbss_list: $mldbbss_list"
    __repacd_wifimon_debug "backhaul_mldssid: $backhaul_mldssid"
    __repacd_wifimon_debug "backhaul_mld: $backhaul_mld"
    __repacd_wifimon_debug "onboarding_isdone: $onboarding_isdone"

    total_vap_count=-1

    if [ "$map_version" -ge 6 -a "$onboarding_isdone" -eq 1 ] || [ "$map_version" -ge 6 -a "$maplite_enabled" -eq 1 ]; then

        # Resolve the bSTA SSID
        config_foreach __repacd_wifimon_get_bsta_ssid wifi-iface \
            bsta_ssid

        # Resolve the MLD section
        sta_mld="mld0"

        __repacd_wifimon_debug "bsta ssid: $bsta_ssid"
        __repacd_wifimon_debug "bsta mld: $sta_mld"

        if [ -n "$bsta_ssid" ]; then
            bsta_mldssid=$(uci get wireless.$sta_mld.mld_ssid)
            if [ "$mld_ssid" != "$bsta_mldssid" ]; then
                uci set wireless.$sta_mld.mld_ssid=$bsta_ssid
                uci_commit wireless
            fi
        fi

        if ! __repacd_wifimon_is_mlo_bsta; then
            bsta_device_list=$(uci get repacd.MAPConfig.PartnerLink)
            if [ -z "$bsta_device_list" ]; then
                __repacd_wifimon_debug "Could not create mlo bsta Since bsta_device_list is empty"
                return 1
            fi

            #Delete any stored bssid so as not to copy wrong bssid to new sta vap
	        config_foreach __repacd_wifimon_traverse_to_sta wifi-iface
            config_foreach __repacd_map_check_get_total_vap wifi-iface

            # Move the bSTA interface to last
            curStaIdx=$( uci show wireless | grep mode | grep sta | cut -d "[" -f2 | cut -d "]" -f1 | awk '{print $1}')
            primaryRadio=$(uci show wireless.@wifi-iface[$curStaIdx\].device | cut -d '=' -f2 | tr -d \'\")
            uci show wireless | grep "wifi-iface\[$curStaIdx\]" > /tmp/mldstaConfig
            config_foreach __repacd_map_delete_sta_idx wifi-iface
            uci_commit wireless

            # create MLO bsta
            __repacd_wifimon_debug "bsta_device_list: $bsta_device_list"
            __repacd_wifimon_debug "primary radio: $primaryRadio"
            for device in $bsta_device_list; do
                __repacd_wifimon_debug "device: $device"
                idx=$(($total_vap_count + $mldStaIdx))
                sed -i "s/\[[^]]*\]/[$idx]/g" /tmp/mldstaConfig
                uci add wireless wifi-iface
                # Copy STA config to last Idx
                while IFS= read -r line; do
                    eval "uci set $line"
                done < /tmp/mldstaConfig

                local key
                local bsta_ssid
                key=$(uci show wireless.@wifi-iface[$idx].key | cut -d '=' -f2 | tr -d \'\")
                bsta_ssid=$(uci show wireless.@wifi-iface[$idx].ssid | cut -d '=' -f2 | tr -d \'\")
                if [ "$maplite_enabled" -eq 1 ]; then
                    uci set wireless.$sta_mld.mld_ssid=$bsta_ssid
                fi
                config_load wireless
                config_foreach __repacd_wifimon_get_bbss_encrpytion wifi-iface \
                    backhaul_encryption

                __repacd_wifimon_debug "backhaul_encryption: $backhaul_encryption"

                if [ "$maplite_enabled" -eq 1 ]; then
                    uci set wireless.@wifi-iface[$idx].mld=$sta_mld
                    uci set wireless.@wifi-iface[$idx].encryption='gcmp-256'
                    uci set wireless.@wifi-iface[$idx].key_mgmt='SAE SAE-EXT-KEY'
                    uci set wireless.@wifi-iface[$idx].sae_password=$key
                    uci set wireless.@wifi-iface[$idx].rrm='1'
                    uci set wireless.@wifi-iface[$idx].sae='1'
                elif [ "$backhaul_encryption" == "psk2+ccmp+gcmp-256" ] || \
                     [ "$backhaul_encryption" == "psk2+gcmp-256" ]; then
                    uci set wireless.@wifi-iface[$idx].mld=$sta_mld
                    uci set wireless.@wifi-iface[$idx].encryption='ccmp+gcmp-256'
                    uci set wireless.@wifi-iface[$idx].key_mgmt='SAE SAE-EXT-KEY'
                    uci set wireless.@wifi-iface[$idx].sae_password=$key
                    uci set wireless.@wifi-iface[$idx].rrm='1'
                    uci set wireless.@wifi-iface[$idx].sae='1'
                fi


                netdev_mld="$sta_mld"
                mldStaIdx=$((mldStaIdx+1))
            done

            local i=0 idx=0 cur_band=0
            for device in $bsta_device_list; do
                idx=$(($total_vap_count + $i))
                uci set wireless.@wifi-iface[$idx].device=$device
                cur_band=0
                config_get cur_band "$device" band
                if [ "$cur_band" -eq "3" ]; then
                    uci set wireless.@wifi-iface[$idx].en_6g_sec_comp='0'
                else
                    uci delete wireless.@wifi-iface[$idx].en_6g_sec_comp
                fi
                # update primary link on which pbc happen to be preferred assoc link
                if [ "$maplite_enabled" -eq 1 ] && [ "$device" == "$primaryRadio" ]; then
                    uci set wireless.@wifi-iface[$idx].preferred_assoc_link='1'
                fi
                i=$((i+1))
            done

            uci_commit wireless
            rm /tmp/mldstaConfig
            total_vap_count=-1

            if ! __repacd_wifimon_is_mlo_bsta; then
               __repacd_wifimon_debug "slo bsta is configured"
               return 1
            else
               uci set repacd.MAPConfig.MLObSTAconfigured='1'
               uci_commit repacd
               return 0
            fi
        fi
    # Configure MLO bSTA only if MLO Backhaul BSS is found
    elif [ -n "$backhaul_mldssid" -a "$onboarding_isdone" -eq 1 ]; then
        if ! __repacd_wifimon_is_mlo_bsta; then
            # Resolve the MLD section from SSID
            config_foreach __repacd_wifimon_get_mld_device wifi-mld \
               "$backhaul_mldssid" "Non-AP" mld_device
            #Delete any stored bssid so as not to copy wrong bssid to new sta vap
	        config_foreach __repacd_wifimon_traverse_to_sta wifi-iface
            config_foreach __repacd_map_check_get_total_vap wifi-iface

            # Move the bSTA interface to last
            curStaIdx=$( uci show wireless | grep mode | grep sta | cut -d "[" -f2 | cut -d "]" -f1 | awk '{print $1}')
            uci show wireless | grep "wifi-iface\[$curStaIdx\]" > /tmp/mldstaConfig
            config_foreach __repacd_map_delete_sta_idx wifi-iface
            uci_commit wireless

            __repacd_wifimon_debug " ======= Before MLO VAP Creation ========"
            cat /tmp/mldstaConfig > /dev/console
            __repacd_wifimon_debug " ===================================="

            # create partner link bSTA interface
            while [ $mldStaIdx -lt 2 ]; do
                idx=$(($total_vap_count + $mldStaIdx))
                sed -i "s/\[[^]]*\]/[$idx]/g" /tmp/mldstaConfig
                uci add wireless wifi-iface
                # Copy STA config to last Idx
                while IFS= read -r line; do
                    eval "uci set $line"
                done < /tmp/mldstaConfig

                uci set wireless.@wifi-iface[$idx].mld=$mld_device
                netdev_mld="$mld_device"
                mldStaIdx=$((mldStaIdx+1))
            done

            local i=0 idx=0 cur_band=0
            # update the wifi device based on backhaul BSS configuration
            for mldbbss in $mldbbss_list; do
                idx=$(($total_vap_count + $i))
                uci set wireless.@wifi-iface[$idx].device=$mldbbss
                cur_band=0
                config_get cur_band "$mldbbss" band
                if [ "$cur_band" -eq "3" ]; then
                    uci set wireless.@wifi-iface[$idx].en_6g_sec_comp='0'
                fi
                i=$((i+1))
            done

            uci_commit wireless
            rm /tmp/mldstaConfig
            total_vap_count=-1

            uci set repacd.MAPConfig.MLObSTAconfigured='1'
            uci_commit repacd

            __repacd_wifimon_debug " =========== [After MLO VAP creation] =========== "
            uci show wireless | grep "wifi-iface\[$curStaIdx\]" > /tmp/mldstaConfig
            cat /tmp/mldstaConfig > /dev/console
            __repacd_wifimon_debug " ================================================ "
            curStaIdx=$((curStaIdx+1))
            __repacd_wifimon_debug " ================================================ "
            uci show wireless | grep "wifi-iface\[$curStaIdx\]" > /tmp/mldstaConfig
            cat /tmp/mldstaConfig > /dev/console
            __repacd_wifimon_debug " ================================================ "
            rm /tmp/mldstaConfig
            return 0
        fi
    fi

    # failure
    return 1
}

# Configure the MLO bSTA based on reconfiguration request
#
# create a partner bSTA interface and configure MLD group accordingly
#
# output: none
__repacd_wifimon_add_bsta_link() {
    local curStaIdx mldStaIdx=0
    config_load repacd
    config_get bsta_link_to_add MAPConfig 'bstaLinkToAdd' "default"
    local numbSTA=2

    total_vap_count=-1

    __repacd_wifimon_debug "bsta_link_to_add = $bsta_link_to_add"

    if [ "$bsta_link_to_add" != "default" ]; then
        for config_name in $mldbsta_config_name_list; do
            config_get device "$config_name" device
            __repacd_wifimon_debug "device : $device"
            if [ "$device" != "$bsta_link_to_add" ]; then
                continue
            fi
            uci set repacd.MAPConfig.bstaLinkToAdd="default"
            uci set repacd.MAPConfig.bstaLinkRemoveActive='0'
            uci set wireless.$config_name.disabled='0'
            uci commit repacd
            uci commit wireless
            need_restart=1
        done

        if [ $need_restart -eq 1 ]; then
            __repacd_wifimon_debug "Restart after adding link multiup mld0"
            __repacd_wifimon_is_mlo_bsta
            local bsta_mld_group
            config_load wireless
            config_foreach __repacd_get_non_ap_mld wifi-mld "Non-AP" bsta_mld_group
            __repacd_wifimon_debug "bsta_mld_group = $bsta_mld_group"
            wifi multi_up $bsta_mld_group
            #Restart ping samples
            last_assoc_state=0
            return
        fi
    fi

    return
}

# Configure the MLO bSTA based on reconfiguration request
#
# Resolve the radio and configure it as SLO bSTA.
# remove the MLD config section from bSTA and partnet bSTA VAP section
#
# output: none
__repacd_wifimon_remove_bsta_link() {
    config_load repacd
    config_get MBsta_onboarding MAPConfig 'MultibSTAOnboarding' '0'
    config_get bsta_link_to_remove MAPConfig 'bstaLinkToRemove' "default"
    local device
    local need_restart=0

    __repacd_wifimon_debug "bsta_link_to_remove = $bsta_link_to_remove"

    if [ "$bsta_link_to_remove" != "default" ]; then
        # Update $sta_iface_5g to ensure RSSI measurements happen
        # on the correct link
        if [ "$MBsta_onboarding" -eq 0 ]; then
            for config_name in $mldbsta_config_name_list; do
                config_get device "$config_name" device
                config_get iface "$config_name" ifname

                if [ "$device" != "$bsta_link_to_remove" ]; then
                    sta_iface_5g="$iface"
                fi
            done
        fi
        local multiup_cmd
        config_load wireless
        config_foreach __repacd_get_non_ap_mld wifi-mld "Non-AP" multiup_cmd
        __repacd_wifimon_debug "multiup_cmd = $multiup_cmd"
        for config_name in $mldbsta_config_name_list; do
            config_get device "$config_name" device
            config_get iface "$config_name" ifname
            __repacd_wifimon_debug "device : $device"

            if [ "$device" != "$bsta_link_to_remove" ]; then
                continue
            fi
            multiup_cmd="$multiup_cmd $device $iface"
            uci set repacd.MAPConfig.bstaLinkToRemove="default"
            uci set repacd.MAPConfig.bstaLinkRemoveActive='1'
            uci set wireless.$config_name.disabled='1'
            uci commit repacd
            uci commit wireless
            need_restart=1
        done

        if [ $need_restart -eq 1 ]; then
            __repacd_wifimon_debug "Restart after removing link."
            __repacd_wifimon_debug "Switching from MLO to Legacy bSTA multiupcmd:$multiup_cmd"
            wifi multi_down $multiup_cmd
            # sleep 2
            local bsta_mld_group
            config_load wireless
            config_foreach __repacd_get_non_ap_mld wifi-mld "Non-AP" bsta_mld_group
            __repacd_wifimon_debug "bsta_mld_group = $bsta_mld_group"
            wifi multi_up $bsta_mld_group
            # sleep 2
            #Restart ping samples
            last_assoc_state=0
            mldbsta_enabled=0
            primary_mldbsta_iface=''
            MLOtolegacyvariable=''
            return
        fi
    fi

    return
}

#Fetch the last vap based on the
#current preferred radio
#Input 1: wifi-iface
#output 2: current sta-iface
#output 3: last iface
__repacd_wifimon_get_last_vap() {
    local config="$1"
    local sta_iface="$2"
    local last_iface="$3"
    local mode network disabled bssid
    local device hwmode type iface

    config_get device "$config" device
    config_get hwmode "$device" hwmode
    config_get type "$device" type
    config_get iface "$config" ifname
    config_get mode "$config" mode
    config_get disabled "$config" disabled 0

    if [ "$hwmode" = '11ad' ] && [ "$type" = 'mac80211' ] ;then
        return
    fi

    if [ "$device" != "$global_preferred_radio" ]; then
        return
    fi
    if __repacd_is_matching_mode "sta" "$mode"; then
        eval $sta_iface="$iface"

    else
        eval $last_iface="$iface"
    fi
}

#Function to form a new sta vap
#to be multi-up'd since
#preferred radio is changed
#Fetch last vap of preferred radio
#Add 1 to its vap index
__repacd_wifimon_get_new_sta() {
    local sta_vap last_vap
    local rIdx vapIdx

    config_load wireless
    config_foreach __repacd_wifimon_get_last_vap wifi-iface sta_vap last_vap

    __repacd_wifimon_debug "sta: $sta_vap , last vap: $last_vap"
    if [ "$sta_vap" != "$last_vap" ]; then
        rIdx=$(echo $last_vap | cut -c 4)
        vapIdx=$(echo $last_vap | cut -c 5)
        vapIdx=$((vapIdx + 1))
        preferred_sta_wifimon="$global_preferred_radio ath$rIdx$vapIdx"
        preferred_sta_ath_wifimon="ath$rIdx$vapIdx"
    fi
}

#Function to get new sta iface only
#if the wireless file has atleast one
#sta interface.
#Input 1: wifi-iface
__repacd_wifimon_get_new_sta_iface() {
    local config="$1"
    local iface disabled mode mld sta_iface
    local device

    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get device "$config" device
    config_get mld "$config" mld ''
    if [ "$mode" != "sta" ]; then
        return
    fi
    __repacd_wifimon_get_new_sta


}

#Function to set rootdistance to 1
__repacd_set_valid_root_distance() {
   local config="$1"
   config_get iface "$config" ifname
   config_get mode "$config" mode

   if [ "$mode" != "sta" ]; then
       __repacd_wifimon_debug "Set Root Distance to 1"
       cfg80211tool_mesh "$iface" set_whc_dist "1"
       last_hop_count=1
   fi
}


#Function to set root distance
#to 255 of the Backhaul AP Vaps
#It will be used at every bsta
#switch to ensure root distance
#is 255 and write to wireless
#It will also be called post
#cloning when link has completed
#5 samples
#Input 1: wifi-iface
__repacd_wifimon_set_root_distance() {
    local config="$1"
    config_get iface "$config" ifname
    config_get mode "$config" mode
    config_get network "$config" network
    config_get root_distance "$config" root_distance '255'

    if [ "$external_controller" -eq 1 ]; then
        __repacd_wifimon_debug "Skip to set root distance $root_distance"
        return
    fi

    if [ "$mode" == "ap" ] && [ "$network" == "backhaul" ]; then
        uci_set wireless "$config" root_distance "255"
        uci commit wireless
        __repacd_wifimon_debug "UPDATED WIRELESS FILE TO ROOTDISTANCE $root_distance"
    fi
    if [ "$mode" == "ap" ]; then
        cfg80211tool_mesh "$iface" set_whc_dist "255"
        last_hop_count=255
    fi

}

#Function that will write the current
#Associated bssid to wireless file
__repacd_wifimon_store_bssid_to_wireless() {
    local config="$1"
    local mode=''
    local device=''
    config_get mode "$config" mode
    config_get device "$config" device

    __repacd_wifimon_set_root_distance "$config"
    rddevice="wifi$(echo $sta_iface_backup | cut -c 4)"
    if [ "$mode" == "sta" ] && [ "$device" == "$rddevice" ]; then
        if [ "$primary_link_bssid" != "Not-Associated" ]; then
            __repacd_wifimon_debug " STORING $primary_link_bssid for $device in wireless"
            uci_set wireless "$config" bssid "$primary_link_bssid"
            uci commit wireless
        fi
    fi
}


# update the wildcard bssid to all STA ifaces in wireless file
# will set this when BSSID timeout has elapsed
__repacd_wifimon_store_wildcard_bssid_to_wireless()
{
    local config="$1"
    local mode=''
    local device=''
    config_get mode "$config" mode
    config_get device "$config" device

    __repacd_wifimon_set_root_distance "$config"
    if [ "$mode" == "sta" ]; then
        __repacd_wifimon_debug " STORING $WIFIMON_WILDCARD_BSSID for $device in wireless"
        uci_set wireless "$config" bssid "$WIFIMON_WILDCARD_BSSID"
        uci commit wireless
    fi
}

#Determines the radio band of
#athx
#Input 1: wifi-iface
#Input 2: Radio device (e.g. wifix)
__repacd_wifimon_determine_device_band() {
    local config="$1"
    local radio="$2"
    config_get device "$config" device
    if [ "$device" == "$radio" ]; then
        if whc_is_5g_radio $device; then
            sta_iface_5g="$sta_iface_backup"
            sta_iface_24g=''
        else
            sta_iface_24g="$sta_iface_backup"
            sta_iface_5g=''
        fi
    fi
}

#Function to update the global variable with the
#next chosen preferred STA interface
__repacd_wifimon_update_preferred_radio() {
    local config="$1"
    local radio="$2"
    config_get device "$config" device
    if [ "$device" == "$radio" ]; then
        if whc_is_5g_radio $device; then
            sta_iface_5g="$preferred_sta_ath_wifimon"
            sta_iface_24g=''
        else
            sta_iface_24g="$preferred_sta_ath_wifimon"
            sta_iface_5g=''
        fi
    fi
}

#Function to calculate new preferred stavap
#overwrite the new selected radio in wireless
#Make appropriate globals NULL
__repacd_wifimon_legacy_switches() {
    config_load wireless
    config_foreach __repacd_wifimon_get_new_sta_iface wifi-iface
    if [ -n "$sta_iface_backup" ]; then
    radioid_backup=$(echo $sta_iface_backup | cut -c 4)
    radio_device_backup="wifi$radioid_backup"
    config_foreach __repacd_wifimon_update_preferred_radio wifi-iface \
                    "$radio_device_backup"
    fi

    sta_lastvapradio=$(echo $sta_iface_backup | cut -c 4)
    current_sta_wifimon="wifi$sta_lastvapradio $sta_iface_backup"
    __repacd_wifimon_debug " nonapmld $non_ap_mld & CURRENT STA BEFORE SWITCH $current_sta_wifimon staifacebackup $sta_iface_backup preferred_sta $preferred_sta_wifimon"
    if [ "$enable_slo" -eq 1 ]; then
	__repacd_wifimon_debug "wifi multi_down nonapmld currentstawifimon $non_ap_mld $current_sta_wifimon"
	wifi multi_down $non_ap_mld $current_sta_wifimon
    else
	__repacd_wifimon_debug "wifi multi_down currentstawifimon $current_sta_wifimon"
	wifi multi_down $current_sta_wifimon
    fi
    total_vap_count=-1
    config_foreach __repacd_map_check_get_total_vap wifi-iface
    uci set wireless.@wifi-iface[$total_vap_count].device="$global_preferred_radio"
    uci set wireless.@wifi-iface[$total_vap_count].en_6g_sec_comp=''
    uci_commit wireless
    local cur_band=0
    config_get cur_band "$global_preferred_radio" band
    __repacd_wifimon_debug "cur_band $cur_band"
    if [ "$cur_band" -eq 3 ]; then
        uci set wireless.@wifi-iface[$total_vap_count].en_6g_sec_comp='0'
    fi
    total_vap_count=-1
}

#Function to store the iface name athx in
#ifname of config of STA vap. This is
#needed since we directly overwrite wireless
#file rather than deleting wifi-iface &
#re-adding.We update option device directly
# in legacy switch so the sta iface is
# returned NULL & needs to be updated
__repacd_wifimon_configure_last_iface() {
    local config="$1"
    config_get device "$config" device
    config_get iface "$config" ifname
    config_get mode "$config" mode
    config_get onboarding_isdone MAPConfig 'OnboardingDone' '0'
    __repacd_wifimon_debug "device $device mode $mode iface $iface onboarding_isdone $onboarding_done "
    if [ "$device" != "$global_preferred_radio" ]; then
        return
    fi
    if [ "$mode" == "sta" ]; then
        current_staiface="$iface"
    else
    current_lastiface="$iface"
    fi
}
__repacd_wifimon_get_the_mld_matching_role() {
    local config="$1"
    local role_to_match="$2"
    local role
    config_get role "$config" role

    if [ "$role" == "$role_to_match" ]; then
        eval "$3=$config"
    fi
}

__repacd_wifimon_add_del_sta_mld() {
    local config="$1"
    local sta_Mld
    config_get hwmode_new "$global_preferred_radio" hwmode
    config_get mld "$config" mld
    config_get iface "$config" ifname
    config_get mode "$config" mode

    if [ "$mode" != "sta" ]; then
        return
    fi
    config_load wireless
    config_foreach __repacd_wifimon_get_the_mld_matching_role wifi-mld "Non-AP" sta_Mld

    if [ "$hwmode_new" = "11bea" ] || [ "$hwmode_new" = "11beg" ]; then
        if [ -n "$sta_Mld" ]; then
	    uci_set wireless "$config" mld "$sta_Mld"
	    uci_commit wireless
        fi
    else
        if [ -n "$mld" ]; then
            uci delete wireless."$config".mld
            uci_commit wireless
        fi
    fi
}

#Function to traverse to STA interface
#at end of wireless
__repacd_wifimon_traverse_to_sta() {
    local config="$1"
    config_get mode "$config" mode
    config_get device "$config" device
    config_get iface "$config" ifname
    if [ "$mode" != "sta" ]; then
        return
    fi

    config_get staface "$config" ifname
    uci_set wireless "$config" bssid ""
    uci commit wireless

}

__repacd_wifimon_get_last_vap_for_mlotolegacy() {
    local config="$1"
    local stavap_iface="$2"
    local lastvap_iface="$3"
    local mode network disabled bssid
    local device hwmode type iface

    config_get device "$config" device
    config_get hwmode "$device" hwmode
    config_get type "$device" type
    config_get iface "$config" ifname
    config_get mode "$config" mode
    config_get disabled "$config" disabled 0

    if [ "$hwmode" = '11ad' ] && [ "$type" = 'mac80211' ] ;then
        return
    fi

    if [ "$device" != "$ath_device_to_delete" ]; then
        return
    fi
    if __repacd_is_matching_mode "sta" "$mode"; then
        eval $stavap_iface="$iface"

    else
        eval $lastvap_iface="$iface"
    fi

 }
# Dertmine the radio that is next most preferred legacy STA
#
# Resolve the max preference radio and configure it as legacy bSTA.
# remove the MLD config sectio from bSTA and partnet bSTA VAP section
#
# output: $submode: Backhaul switch to legacy bSTA
__repacd_map_switch_mld_to_legacy_sta() {
    local preferred_radio=''
    local device bsta_dev
    local default_mldbsta_iface_list=''
    local staiface_vap=''
    local lastiface_vap=''
    # Select the max preferred radio from MLD bSTA
    bsta_max_preference=0
    for bsta_dev in $mldbsta_list; do
        __repacd_wifimon_resolve_next_bsta_radio $bsta_dev "" preferred_radio
    done

    # Remove non preferred radio bSTA
    for config_name in $mldbsta_config_name_list; do
        config_get device "$config_name" device
        config_get iface "$config_name" ifname

        if [ $enable_slo -eq 0 ]; then
            uci set wireless.$config_name.mld=''
        fi
        uci_set wireless "$config_name" bssid ""
        uci commit wireless
        if [ "$device" == "$preferred_radio" ]; then
            continue
        fi
        ath_device_to_delete="$device"
        default_mldbsta_iface_list="$default_mldbsta_iface_list $device $iface"
        uci set repacd.MAPConfig.MLObSTAconfigured='0'
        uci delete wireless.$config_name
        uci_commit wireless
    done

    # Select the preferred radio for legacy connection
    config_foreach __repacd_wifimon_select_bsta_radio wifi-device \
        "$preferred_radio"
    uci_commit wireless
    uci_commit repacd

    config_load wireless
    config_foreach __repacd_wifimon_traverse_to_sta wifi-iface
    config_foreach __repacd_wifimon_add_del_sta_mld wifi-iface
    #Get last vap of non preferred radio to remove from nw file when switching to SLO
    config_foreach __repacd_wifimon_get_last_vap_for_mlotolegacy wifi-iface staiface_vap lastiface_vap
    __repacd_wifimon_debug "sta: $staiface_vap , last vap: $lastiface_vap"
    ath_interface_to_delete="$lastiface_vap"
    echo "$device iface delete ath_interface_to_delete= $ath_interface_to_delete" >/dev/console
    # Bring down MLO bSTA before switching to Legacy bSTA
    if [ "$map_fast_onboarding" -eq 1 ]; then
        __repacd_wifimon_debug "Switching from MLO to Legacy bSTA"
        if [ -z "$base_mld_mac_addr" ]; then
            __repacd_wifimon_debug "wifi multi_down $non_ap_mld $default_mldbsta_iface_list"
            wifi multi_down $non_ap_mld $default_mldbsta_iface_list
            __repacd_wifimon_debug "wifi multi_up $non_ap_mld"
            wifi multi_up $non_ap_mld
        else
            __repacd_wifimon_debug "wifi multi_up $mldbsta_iface_list"
            wifi multi_up $mldbsta_iface_list
        fi
    fi
    MLOtolegacyvariable="true"
    eval "$1=$WIFIMON_STATE_RE_SWITCH_BSTA"
}

#Since we overwrite wireless file with
#new preferred radio instead of deleting
#& adding new wifi-iface, it doesn't
#retrieve the new sta-iface. So store the
#new iface in config of the the STA wifi-iface
#Input 1: wifi-iface
__repacd_wifimon_configure_sta_iface(){
    local config="$1"

    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get device "$config" device
    config_get mld "$config" mld ''
    if [ "$mode" != "sta" ]; then
        return
    fi
    config_get band "$device" band
    config_load wireless
    config_foreach __repacd_wifimon_configure_last_iface wifi-iface
    local rIdx=$(echo $current_lastiface | cut -c 4)
    local vapIdx=$(echo $current_lastiface | cut -c 5)
    vapIdx=$((vapIdx + 1))
    current_staiface=ath$rIdx$vapIdx
    config_foreach __repacd_wifimon_traverse_to_sta wifi-iface
}


# Update the radio on which to instantiate the bSTA.
#
# If the currently selected bSTA is 2.4 GHz, select the highest priority radio.
# Otherwise, select the highest priority radio that is less than the current
# priority.
#
# input: $1 - config: the name of the config section for the bSTA
# input: $2 - force_bsses_down: whether to force the BSSes down on the next
#                               startup
# input: $3 - state_no_bsta_radios: the state to transition into if there are
#                                   no other bSTA radios to switch to
# output: $4 - state: the variable to update with the new state name (if there
#                     was a change)
#
# return: 0 if a new radio was selected; otherwise non-zero
__repacd_wifimon_select_next_bsta() {
    local config="$1"
    local force_bsses_down="$2"
    local state_no_bsta_radios="$3"

    # Resolve the current preference value
    local device
    config_load wireless
    config_get device "$config" device

    local cur_pref
    config_get cur_pref "$device" repacd_map_bsta_preference

    if [ -n "$sta_iface_24g" ]; then
        # We want to take the maximum one
        cur_pref=''
    fi

    local preferred_radio=''
    bsta_max_preference=0
    config_foreach __repacd_wifimon_resolve_next_bsta_radio wifi-device \
        "$cur_pref" preferred_radio

    if [ -n "$preferred_radio" ] && [ "$device" != "$preferred_radio" ]; then
        __repacd_wifimon_debug "Selected $preferred_radio as next bSTA"

        # If switching from 5 GHz to 2.4 GHz, increment the count to record we
        # have tried all 5 GHz bSTAs. The assumption here is that 2.4 GHz will
        # always come after 5 GHz in the preference order.
        if ! whc_is_5g_radio "$preferred_radio"; then
            # Do not allow it to go over the max since that will break the
            # 2.4 GHz BSSID change retry logic.
            config_get isOnboardingDone MAPConfig 'OnboardingDone'
            __repacd_wifimon_debug "OnboardingDone= $isOnboardingDone"
            if [ "$cnt_5g_attempts" -lt "$max_5g_attempts" ]; then
                cnt_5g_attempts=$((cnt_5g_attempts + 1))
                uci_set repacd MAPWiFiLink '5gAttemptsCount' "$cnt_5g_attempts"
                uci_commit repacd
            fi

            __repacd_wifimon_debug "Completed 5 GHz bSTA attempt #$cnt_5g_attempts"
        fi

        config_foreach __repacd_wifimon_select_bsta_radio wifi-device \
            "$preferred_radio"
        uci_commit wireless

        if [ "$force_bsses_down_on_all_bsta_switches" -gt 0 ]; then
            force_bsses_down=1
        fi

        # Overwriting the ForceDownOnStart to 0 for all cases for now
        # to prevent deadlock on any BSTA switch
        __repacd_wifimon_debug "Updating ForceDownOnStart to 0"
        uci_set repacd FrontHaulMgr ForceDownOnStart '0'
        uci_commit repacd

        eval "$4=$WIFIMON_STATE_RE_SWITCH_BSTA"
        return 0
    else
        __repacd_wifimon_debug "No other bSTA radios available"

        # Remember the current BSSID if we are associated
        if __repacd_wifimon_is_assoc "$sta_iface_24g" \
            && __repacd_wifimon_is_assoc "$sta_iface_5g"; then
            __repacd_wifimon_set_target_bssid "$current_bssid" "$current_bssid"
        fi

        eval "$4=$state_no_bsta_radios"
        return 1
    fi
}

# Determine if a provided amount of time has elapsed.
# input: $1 - start_time: the timestamp (in seconds)
# input: $2 - duration: the amount of time to check against (in seconds)
# return: 0 on timeout; non-zero if no timeout
__repacd_wifimon_is_timeout() {
    local start_time=$1
    local duration=$2

    # Check if the amount of elapsed time exceeds the timeout duration.
    local cur_time
    __repacd_wifimon_get_timestamp cur_time
    local elapsed_time=$((cur_time - start_time))
    if [ "$elapsed_time" -gt "$duration" ]; then
        return 0
    fi

    return 1
}

# Check whether the given interface is the STA interface on the desired
# network and the desired band.
#
# input: $1 - config: the name of the interface config section
# input: $2 - network: the name of the network to which the STA interface
#                      must belong to be matched
# output: $3 - iface: the resolved STA interface name on 2.4 GHz (if found)
# output: $4 - iface_config_name: the resolved name of the config section
#                                 for the STA interface on 2.4 GHz (if found)
# output: $5 - iface: the resolved STA interface name on 5 GHz (if found)
# output: $6 - iface_config_name: the resolved name of the config section
#                                 for the STA interface on 5 GHz (if found)
# output: $7 - unknown_ifaces: whether any Wi-Fi interfaces are as yet
#                              unknown (in terms of their interface name)
__repacd_wifimon_is_sta_iface_map() {
    local config="$1"
    local network_to_match="$2"
    local iface disabled mode device hwmode

    config_get network "$config" network
    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get device "$config" device
    config_get hwmode "$device" hwmode
    local band_freq=$(cfg80211tool $device g_oper_reg_inf | awk -F "," '{print $2}' | awk -F "=" '{print $2}' | cut -c1-4)

        if [ "$mode" = "sta" ] &&  [ "$disabled" -eq 0 ]; then
                if [ "$band_freq" -gt 5175 ] && [ "$band_freq" -lt 7130 ]; then
                        eval "$5=$iface"
                        eval "$6=$config"
                else
                        eval "$3=$iface"
                        eval "$4=$config"
                fi
        fi
    
}


# Initialize the sta_iface_5g variable with the STA interface that is enabled
# on the specified network (if any).
# input: $1 - network: the name of the network being managed
__repacd_wifimon_get_sta_iface_map() {
    unknown_ifaces=0

    config_load wireless
    config_foreach __repacd_wifimon_is_sta_iface_map wifi-iface "$1" \
        sta_iface_24g sta_iface_24g_config_name \
        sta_iface_5g sta_iface_5g_config_name unknown_ifaces
    if [ "$unknown_ifaces" -gt 0 ]; then
        # Clear out everything because we cannot be certain we have the
        # right names (eg. interfaces may not all be up yet).
        sta_iface_24g=
        sta_iface_24g_config_name=
        sta_iface_5g=
        sta_iface_5g_config_name=
    fi
}

#Function introduced for MBsta_onboarding.
#Not to be replaced in other places
__repacd_wifimon_whc_is_5g_radio() {
    local hwmode
    config_get hwmode $1 hwmode '11ng'

    case "$hwmode" in
        11axa|11bea|11ac|11na|11a)
            return 0
        ;;

        *)
            return 1
        ;;
    esac
}

#Store STA ifaces in appropriate varaibles
__repacd_initialize_sta_ifaces() {

    local config="$1"
    local device mode iface cur_band bhmloenabled device_6GL device_5GL
    local mld

    config_get device "$config" device
    config_get mode "$config" mode
    config_get iface "$config" ifname
    config_get cur_band "$device" band
    config_get_bool bhmloenabled "$device" map_mbsta_bhmlo_enabled '0'
    config_get mld "$config" mld
    config_get disabled "$config" disabled '0'
    config_get device_6GL MAPConfig Radio6GL '0'
    config_get device_5GL MAPConfig Radio5GL '0'

    __repacd_wifimon_dump "sta_iface_6g_config $sta_iface_6g_config sta_iface_6gl_config $sta_iface_6gl_config sta_iface_5gl_config $sta_iface_5gl_config "
    __repacd_wifimon_dump "6G intf: $sta_iface_6g, 6GL intf: $sta_iface_6gl, 5G intf: $sta_iface_5g, 5GL intf: $sta_iface_5gl, 2G intf: $sta_iface_24g"
    if [ "$mode" = "sta" ]; then
        if __repacd_wifimon_whc_is_5g_radio $device; then
            if [ "$cur_band" -eq 3 ]; then
                if [ "$device" != "$device_6GL" ]; then
                    sta_iface_6g="$iface"
                    sta_iface_6g_config="$config"
                else
                    sta_iface_6gl="$iface"
                    sta_iface_6gl_config="$config"
                fi
            elif [ "$device" != "$device_5GL" ]; then
                sta_iface_5g="$iface"
                sta_iface_5g_config="$config"
            else
                sta_iface_5gl="$iface"
                sta_iface_5gl_config="$config"
            fi
        elif ! __repacd_wifimon_whc_is_5g_radio $device; then
            sta_iface_24g="$iface"
            sta_iface_24g_config="$config"
        fi
        if [ "$bhmloenabled" -eq 1 ]; then
            if [ "$disabled" -eq 0 ]; then
                MBsta_mlo_sta_iface_list="$MBsta_mlo_sta_iface_list $iface"
                MBsta_sta_intf_list="$iface $MBsta_sta_intf_list"
                if [ -z "$MBsta_mlo_bstaMld" ]; then
                    MBsta_mlo_bstaMld="$mld"
                fi
            fi
        else
            MBsta_non_mlo_sta_iface_list="$iface $MBsta_non_mlo_sta_iface_list"
            MBsta_sta_intf_list="$iface $MBsta_sta_intf_list"
            if [ -z "$MBsta_nonmlo_bstaMld" ]; then
                if [ -n "$mld" ]; then
                    MBsta_nonmlo_bstaMld="$mld"
                else
                    #Non 11 be
                    MBsta_nonmlo_bstaMld="$device $iface"
                fi
            fi
        fi
    fi
}

# Determines if we are in Ethernetmode or not
# If config has unusedignore_bstavapvap 1 then make
# MBsta_IgnoreBstaOnEthBH=1
__repacd_wifimon_MBsta_check_ignore_bstavap(){
    local config="$1"
    local iface mode ignore_bstavap

    config_get iface "$config" ifname
    config_get mode "$config" mode
    config_get ignore_bstavap "$config" ignore_bstavap 0

    if [ "$mode" != "sta" ]; then
        return
    fi
    if [ "$ignore_bstavap" -eq 1 ]; then
        __repacd_wifimon_debug "We are in CAP mode"
        MBsta_IgnoreBstaOnEthBH=1
    else
        MBsta_IgnoreBstaOnEthBH=0
    fi
}




# Initialize the Wi-Fi monitoring logic with the name of the network being
# monitored.
# input: $1 - network: the name of the network being managed
# input: $2 - cur_re_mode: the current operating range extender mode
# input: $3 - cur_re_submode: the current operating range extender submode
# input: $4 - autoconfig: whether it was an auto-config restart
# output: $5 - state: the name of the initial state
# output: $6 - new_re_mode: the resolved range extender mode
# output: $7 - new_re_submode: the resolved range extender submode
repacd_wifimon_init() {
    # Resolve the STA interfaces.
    # Here we assume that if we have the 5 GHz interface, that is sufficient,
    # as not all modes will have a 2.4 GHz interface.
    __repacd_wifimon_get_sta_iface_map "$1"

    # Resolve the MLO STA interfaces
    __repacd_wifimon_is_mlo_bsta
    config_get pre_cac_enabled MAPConfig 'EnablePreCAC' 0
    config_get weather_radar_timeout MAPConfig 'MLOBstaCACWaitTime' 600
    config_get MBsta_onboarding MAPConfig 'MultibSTAOnboarding' '0'
    config_get MBsta_mlo_mode MAPConfig 'MBsta_mlo_mode' '0'
    config_get MBsta_non_mlo_mode MAPConfig 'MBsta_non_mlo_mode' '0'
    config_get MBsta_disconnectnonmlo_once MAPConfig 'MultibSTADisconnectNonMLO' '0'
    config_get backup_link MAPConfig 'MultibSTABackUpLink' 0
    config_get MBsta_short_timer_duration MAPConfig 'MultibSTABackhaulEvalShortTimerPeriod' 300 #5mins
    config_get MBsta_long_timer_duration MAPConfig 'MultibSTABackhaulEvalLongTimerPeriod' 18000 #5hrs
    config_get backhaul_ssid MAPConfig BackhaulSSID
    config_get MBsta_short_timer_used MAPConfig 'MBsta_shorttimerused' '0'
    config_get MBsta_upfront_mlo MAPConfig 'MultibSTAConfigureMLObSTAUpfront' '0'
    config_get non_11be_radio_count MAPConfig 'number_of_non_11be_radio' 0
    config_get map_ts_enabled MAPConfig 'MapTrafficSeparationEnable' '0'

    config_load wireless
    config_foreach __repacd_wifimon_MBsta_check_ignore_bstavap wifi-iface
    local onboarding_Done=0
    config_get onboarding_Done MAPConfig 'OnboardingDone' '0'
    __repacd_wifimon_debug "Onboarding Done $onboarding_Done"

    if [ "$MBsta_onboarding" -eq 1 ]; then

        MBsta_mlo_sta_iface_list=''
        MBsta_non_mlo_sta_iface_list=''
        MBsta_sta_intf_list=''
        config_load wireless
        #Mark variables as NULL before intialize
        sta_iface_24g='' sta_iface_24g_config='' sta_iface_5g='' sta_iface_5g_config=''
        sta_iface_5gl='' sta_iface_5gl_config='' sta_iface_6g='' sta_iface_6g_config=''
        sta_iface_6gl='' sta_iface_6gl_config=''
        config_foreach __repacd_initialize_sta_ifaces wifi-iface
        __repacd_wifimon_debug " MLO bSTA interfaces: $MBsta_mlo_sta_iface_list"
        __repacd_wifimon_debug " Non MLO bSTA interfaces: $MBsta_non_mlo_sta_iface_list"
        __repacd_wifimon_debug " MLO bsta grouping: $MBsta_mlo_bstaMld Non MLO bsta grouping: $MBsta_nonmlo_bstaMld"
        __repacd_wifimon_debug "Enable Upfront MLO connection: $MBsta_upfront_mlo backhaul_ssid:$backhaul_ssid Disconnect NON-MLO: $MBsta_disconnectnonmlo_once"

        if [ "$MBsta_disconnectnonmlo_once" -eq 0 ] && [ "$MBsta_upfront_mlo" -eq 1 ]; then
            if [ -n "$MBsta_non_mlo_sta_iface_list" ]; then
                MBsta_disconnectnonmlo_once=1
                uci set repacd.MAPConfig.MultibSTADisconnectNonMLO='1'
                    __repacd_wifimon_MBsta_disconnect_link $MBsta_non_mlo_sta_iface_list
                uci commit repacd
                __repacd_wifimon_debug "Disconnected NON MLO STA IFACES to allow MLO connection first"
            fi
        fi
        if [ "$map_ts_enabled" -eq 0 ]; then
            #TS is disabled, we will add the non vlan parent interface to bridge during init
            __repacd_wifimon_debug "Traffic Separation disabled"
            if [ ! -e "/sys/class/net/br-"$map_primary_nw"/brif/"$MBsta_mlo_bstaMld"" ]; then
                __repacd_wifimon_debug "Adding $MBsta_mlo_bstaMld to bridge since it is not added already"
                brctl addif br-lan $MBsta_mlo_bstaMld
            fi
            local non_11be_radio_count
            config_get non_11be_radio_count MAPConfig 'number_of_non_11be_radio' 0
            if [ -n "$MBsta_non_mlo_sta_iface_list" ]; then
                if [ "$non_11be_radio_count" -gt 0 ]; then
                    if [ ! -e "/sys/class/net/br-"$map_primary_nw"/brif/"$MBsta_non_mlo_sta_iface_list"" ]; then
                        __repacd_wifimon_debug "Adding $MBsta_non_mlo_sta_iface_list to bridge since it is not added already"
                        brctl addif br-lan $MBsta_non_mlo_sta_iface_list
                    fi
                else
                    if [ ! -e "/sys/class/net/br-"$map_primary_nw"/brif/"$MBsta_nonmlo_bstaMld"" ]; then
                        __repacd_wifimon_debug "Adding $MBsta_nonmlo_bstaMld to bridge since it is not added already"
                        brctl addif br-lan $MBsta_nonmlo_bstaMld
                    fi
                fi
            fi
        fi
    fi
    if [ "$MBsta_IgnoreBstaOnEthBH" -eq 0 ]; then
        #For Legacy case, we initialize variables here
        if [ "$MBsta_onboarding" -eq 1 ]; then
            if [ "$MBsta_non_mlo_mode" -eq 1 ]; then
                __repacd_wifimon_MBsta_disconnect_link $MBsta_mlo_sta_iface_list
                __repacd_wifmon_debug "Disconnecting $MBsta_mlo_sta_iface_list since Non MLO MODE =1 "
            elif [ "$MBsta_mlo_mode" -eq 1 ]; then
                __repacd_wifimon_MBsta_disconnect_link $MBsta_non_mlo_sta_iface_list
                __repacd_wifimon_debug "Disconnecting $MBsta_non_mlo_sta_iface_list since MLO mode=1 "
            elif [ "$MBsta_mlo_mode" -eq 0 ] && [ "$MBsta_non_mlo_mode" -eq 0 ] && [ "$MBsta_upfront_mlo" -eq 1 ]; then
                __repacd_wifimon_MBsta_disconnect_link $MBsta_non_mlo_sta_iface_list
                __repacd_wifimon_debug "Disconnecting NON MLO intf as we never connected in wifi before"
                __repacd_wifimon_MBsta_reconnect_link $MBsta_mlo_sta_iface_list
            elif [ "$MBsta_mlo_mode" -eq 0 ] && [ "$MBsta_non_mlo_mode" -eq 0 ] && [ "$MBsta_upfront_mlo" -eq 0 ]; then
                __repacd_wifimon_MBsta_reconnect_link $MBsta_mlo_sta_iface_list
                __repacd_wifimon_MBsta_reconnect_link $MBsta_non_mlo_sta_iface_list
                __repacd_wifimon_debug "Reconnecting on all sta"
            fi
        fi
        if [ -n "$sta_iface_5g" ] || [ -n "$sta_iface_24g" ] || [ -n "$sta_iface_6g" ] || [ -n "$sta_iface_5gl" ]; then
            if [ -n "$sta_iface_24g" ]; then
                __repacd_wifimon_debug "Resolved 2.4 GHz STA interface to $sta_iface_24g"
                __repacd_wifimon_debug "2.4 GHz STA interface section $sta_iface_24g_config_name"

                config_get config_bssid "$sta_iface_24g_config_name" 'bssid' ''
            fi

            if [ -n "$sta_iface_5g" ]; then
                __repacd_wifimon_debug "Resolved 5 GHz STA interface to $sta_iface_5g"
                __repacd_wifimon_debug "5 GHz STA interface section $sta_iface_5g_config_name"

                config_get config_bssid "$sta_iface_5g_config_name" 'bssid' ''
            fi

            __repacd_wifimon_debug "Resolved target BSSID to $config_bssid"

            # First resolve the config parameters.
            config_load repacd
            config_get min_wps_assoc 'MAPWiFiLink' 'MinAssocCheckPostWPS' '5'
            config_get wps_timeout 'MAPWiFiLink' 'WPSTimeout' '120'
            config_get assoc_timeout 'MAPWiFiLink' 'AssociationTimeout' '300'
            config_get mlo_assoc_timeout 'MAPWiFiLink' 'MLOAssociationTimeout' '300'
            config_get bssid_timeout 'MAPWiFiLink' 'BSSIDTimeout' '90'
            config_get rssi_samples 'MAPWiFiLink' 'RSSINumMeasurements' '5'
            config_get backhaul_rssi_2 'MAPWiFiLink' 'BackhaulRSSIThreshold_2' '-78'
            config_get backhaul_rssi_5 'MAPWiFiLink' 'BackhaulRSSIThreshold_5' '-78'
            config_get backhaul_rssi_offset 'MAPWiFiLink' 'BackhaulRSSIThreshold_offset' '10'
            config_get max_5g_attempts 'MAPWiFiLink' 'Max5gAttempts' '3'
            config_get cnt_5g_attempts 'MAPWiFiLink' '5gAttemptsCount' '0'
            config_get max_mlo_attempts 'MAPWiFiLink' 'MaxMLOAttempts' '3'
            config_get mlo_blocked_timeout 'MAPWiFiLink' 'MloBlockTimeout' '43200'
            config_get mlo_stable_timeout 'MAPWiFiLink' 'MloStableTimeout' '300'
            config_get cnt_mlo_attempts 'MAPWiFiLink' 'MLOAttemptsCount' '0'
            config_get measuring_attempts 'MAPWiFiLink' 'MaxMeasuringStateAttempts' '3'
            config_get max_valid_backhaul_rssi 'MAPWiFiLink' 'MaxValidBackhaulRSSI' '-95'
            config_get force_bsses_down_on_all_bsta_switches 'MAPWiFiLink' \
                    'ForceBSSesDownOnAllBSTASwitches' '0'
            config_get map_fast_onboarding 'MAPConfig' 'MapFastOnboarding' '0'
            config_get enable_mlo MAPConfig 'EnableMLO' 0
            config_get enable_slo MAPConfig 'EnableSLO' 0

            # Create ourselves a named pipe so we can be informed of WPS push
            # button events.
            if [ -e $WIFIMON_PIPE_NAME ]; then
                rm -f $WIFIMON_PIPE_NAME
            fi

            mkfifo $WIFIMON_PIPE_NAME
            __repacd_wifimon_debug "IN WIFIMON INIT $1 $2 $3 $4 $5 $6 $7"
            # If already associated, go to the InProgress state.
            __repacd_wifimon_check_associated "$1" "$2" "$3" "$4" "$5" "$6" "$7"
        fi
    fi
    # Get number of radio
    config_load wireless
    config_foreach __repacd_config_get_num_radio wifi-device

    # Otherwise, must be operating in CAP mode.
    # reset BHSInProgress flag for safety
    (uci set ezmesh.MultiAP.BHSInProgress='0'; uci commit ezmesh)

}

#Function which will store bssid of
#primary and partner bsta in MLO in
#wireless. So that if MLO connection
#breaks, it will resolve the bssid
# & try to connect to it avoiding a
#looping condition in case of 1+2
#topology
__repacd_wifimon_update_partner_bssid() {
    local config="$1"
    local nw="$2"
    local nw_id
    local primary_if_nm="$3"
    local bssid mode iface

    config_get bssid "$config" bssid
    config_get mode "$config" mode
    config_get iface "$config" ifname
    if [ "$mode" == "sta" -a -n "$iface" ]; then
        if [ "$MBsta_onboarding" -eq 1 ]; then
            local intf=''
            for intf in $MBsta_non_mlo_sta_iface_list; do
                if [ "$iface" != "$intf" ] && [ "$MBsta_non_mlo_mode" -eq 1 ]; then
                    return
                elif [ "$iface" = "$intf" ] && [ "$MBsta_mlo_mode" -eq 1 ]; then
                    return
                fi
            done
            local cur_bssid=$(wpa_cli -i $iface -p /var/run/wpa_supplicant-$iface status | grep 'bssid' | awk -F = '{print$2}')
            if [ -n "$cur_bssid" ]; then
                cur_bssid=${cur_bssid#*=}
            fi
            if [ "$cur_bssid" != "Not-Associated" ] && [ "$cur_bssid" != "$WIFIMON_WILDCARD_BSSID" ]; then
                uci_set wireless "$config" bssid "$cur_bssid"
                uci commit wireless
                nw_id=`wpa_cli -p /var/run/wpa_supplicant-$iface list_network \
                    | grep CURRENT | awk '{print $1}'`
                if [ -z $nw_id ]; then
                    nw_id=`wpa_cli -p /var/run/wpa_supplicant-$iface list_network \
                    | grep -i "$cur_bssid" | awk '{print $1}'`
                fi
                wpa_cli -p /var/run/wpa_supplicant-$iface bssid $nw_id $cur_bssid
                __repacd_wifimon_debug "Set target BSSID to: $cur_bssid for partner iface $iface, nw_id: $nw_id"
            fi
        else
            if [ "$iface" == "$primary_if_nm" ]; then
                return;
            fi
            local cur_bssid=$(repacdcli "$iface" get_sta_link)
            if [ "$cur_bssid" != "Not-Associated" ]; then
                uci_set wireless "$config" bssid "$cur_bssid"
                uci commit wireless
                wpa_cli -p /var/run/wpa_supplicant-$iface bssid $nw $cur_bssid
                __repacd_wifimon_debug "Set target BSSID to: $cur_bssid for partner iface $iface"
            fi
        fi
    fi
}

#Function which will set wildcard bssid for
#partner MLO BSTA inface
__repacd_wifimon_set_wildcard_for_partner() {
    local config="$1"
    local nw="$2"
    local primary_if_nm="$3"
    config_get bssid "$config" bssid
    config_get mode "$config" mode
    config_get iface "$config" ifname
    if [ "$mode" == "sta" ]; then
        if [ "$iface" == "$primary_if_nm" ]; then
            return;
        fi
        __repacd_wifimon_debug "Set target BSSID to wildcard for partner iface $iface"
        wpa_cli -p /var/run/wpa_supplicant-$iface bssid $nw $WIFIMON_WILDCARD_BSSID
    fi
}

#Configure supplicant and wireless with the bssid
__repacd_wifimon_MBsta_store_bssid_to_wireless() {
    local config="$1"
    local staiface="$2"
    local bssid="$3"
    local iface mode device

    config_get iface "$config" ifname
    config_get mode "$config" mode
    config_get device "$config" device

    __repacd_wifimon_set_root_distance "$config"
    if [ "$mode" == "sta" ] && [ "$iface" == "$staiface" ] && [ "$bssid" != "$WIFIMON_WILDCARD_BSSID" ]; then
        __repacd_wifimon_debug " STORING $bssid for $iface in wireless"
        uci_set wireless "$config" bssid "$bssid"
        uci commit wireless
        break
    fi
}

#Configure supplicant and update wireless with connected bssid
#after ping sampling completed
__repacd_wifimon_MBsta_set_target_bssid() {
    local desired_bssid="$1"
    local current_bssid="$2"
    local nw_id=''

    local sta_iface="$3"

    __repacd_wifimon_debug "desired_bssid $desired_bssid current_bssid $current_bssid sta_iface $sta_iface"
    nw_id=`wpa_cli -p /var/run/wpa_supplicant-$sta_iface list_network \
            | grep CURRENT | awk '{print $1}'`
    if [ -z "$nw_id" ]; then
        nw_id=`wpa_cli -p /var/run/wpa_supplicant-$sta_iface list_network \
            | grep -i $current_bssid | awk '{print $1}'`
    fi
    __repacd_wifimon_debug "nw_id $nw_id, configuring supplicant for $sta_iface $desired_bssid"

    wpa_cli -p /var/run/wpa_supplicant-$sta_iface bssid $nw_id $desired_bssid
    config_foreach __repacd_wifimon_MBsta_store_bssid_to_wireless wifi-iface $sta_iface $desired_bssid

}
# Update the supplicant with the desired BSSID
# input: $1 - desired_bssid: the desired BSSID to use for the association
# input: $2 - current_bssid: the current BSSID associated to
__repacd_wifimon_set_target_bssid() {
    local desired_bssid="$1"
    local current_bssid="$2"
    local nw_id

    local sta_iface=''
    if [ -n "$sta_iface_24g" ]; then
        sta_iface="$sta_iface_24g"
    else
        sta_iface="$sta_iface_5g"
    fi

    nw_id=`wpa_cli -p /var/run/wpa_supplicant-$sta_iface list_network \
                    | grep CURRENT | awk '{print $1}'`
    if [ -z  $nw_id ]; then
        nw_id=`wpa_cli -p /var/run/wpa_supplicant-$sta_iface list_network \
                     | grep -i "$current_bssid" | awk '{print $1}'`
    fi
    wpa_cli -p /var/run/wpa_supplicant-$sta_iface bssid $nw_id $desired_bssid

    if [ "$desired_bssid" = "$WIFIMON_WILDCARD_BSSID" ]; then
        __repacd_wifimon_debug "Set target BSSID to wildcard for $sta_iface"
        config_bssid=''
        config_foreach __repacd_wifimon_store_wildcard_bssid_to_wireless wifi-iface
        config_foreach __repacd_wifimon_set_wildcard_for_partner wifi-iface $nw_id $sta_iface
    else
        __repacd_wifimon_debug "Set target BSSID to $desired_bssid"
        config_bssid="$desired_bssid"
    fi

    if [ -z "$mldbsta_list" ] && [ "$onboarding_done" -eq 1 ]; then
        __repacd_wifimon_debug "MLO not enabled! STORE BSSID"
        primary_link_bssid="$config_bssid"
        config_load wireless
        config_foreach __repacd_wifimon_store_bssid_to_wireless wifi-iface
    fi
    if [ -n "$mldbsta_list" ] && [ "$onboarding_done" -eq 1 ] && ([ "$cnt_mlo_attempts" -eq "$max_mlo_attempts" ] || [ "$cnt_5g_attempts" -eq "$max_5g_attempts"]); then
        __repacd_wifimon_debug "BECOMING STABLE IN SLO SO STORE BSSID"
        primary_link_bssid="$config_bssid"
        config_load wireless
        config_foreach __repacd_wifimon_store_bssid_to_wireless wifi-iface
    fi
    #Update the bssid in wireless file if Legacy to MLO switch
    __repacd_wifimon_debug "legacytoMLO = $legacytoMLO"
    if [ "$legacytoMLO" -eq 1 ]; then
        legacytoMLO=0
    fi

    if [ "$desired_bssid" != "$WIFIMON_WILDCARD_BSSID" ]; then
        config_load wireless
        config_foreach __repacd_wifimon_update_partner_bssid wifi-iface \
                "$nw_id" $sta_iface
    fi

    #Set root distance of BH AP vaps to 255 in wireless post 5 samples of link
    #and post Onboarding done
    config_get onboarding_complete MapConfig 'OnboardingDone' '0'
    if [ "$onboarding_done" -eq 1 ] && [ "$first_cloning" -eq 0 ]; then
        config_load wireless
        config_foreach __repacd_wifimon_set_root_distance wifi-iface
        first_cloning=1
    fi
}

__repacd_wifimon_check_mlo_link_assoc() {
    local config="$1"

    local iface device mode map bitRate mloenabled
    local map_type mld count=$(($2))

    config_get device "$config" device
    config_get iface "$config" ifname
    config_get mode "$config" mode
    config_get map "$config" map '0'
    config_get map_type "$config" MapBSSType
    config_get mld "$config" mld
    config_get mloenabled "$device" mloenabled

    if [ "$mode" = "sta" ] && [ "$map" -gt 0 ]; then
        if [ "$map_type" -eq 128 ] && [ -n "$mld" ]; then
            bitRate=$(repacdcli $iface get_bitrate)
            __repacd_wifimon_debug "iface $iface mode $mode bitRate $bitRate"
            if [ "$MBsta_onboarding" -eq 0 ]; then
                if [ "$bitRate" -eq 0 ]; then
                    __repacd_wifimon_debug "Iface:$iface has invalid bitrate" > /dev/console
                else
                    count=$((count+1))
                    eval "$2=$count"
                fi
            elif [ "$MBsta_onboarding" -eq 1 ] && [ "$mld" = "$MBsta_mlo_bstaMld" ]; then
                if [ "$bitRate" -eq 0 ]; then
                    __repacd_wifimon_debug "Iface:$iface has invalid bitrate" > /dev/console
                else
                    count=$((count+1))
                    __repacd_wifimon_debug "count $count"
                    eval "$2=$count"
                fi
            fi
        fi
    fi
}

__repacd_wifimon_force_reassoc_mlo_sta() {
    local config="$1"

    local iface mode map
    local map_type mld

    config_get iface "$config" ifname
    config_get mode "$config" mode
    config_get map "$config" map '0'
    config_get map_type "$config" MapBSSType
    config_get mld "$config" mld

    if [ "$mode" = "sta" ] && [ "$map" -gt 0 ]; then
        if [ "$map_type" -eq 128 ] && [ -n "$mld" ]; then
            wpa_cli -p "/var/run/wpa_supplicant-$iface" -i "$iface" disconnect
            wpa_cli -p "/var/run/wpa_supplicant-$iface" -i "$iface" reconnect
        fi
    fi
}
# Check the status of the Wi-Fi link (WPS, association, and RSSI).
# input: $1 - network: the name of the network being managed
# input: $2 - cur_re_mode: the currently configured range extender mode
# input: $3 - cur_re_submode: the currently configured range extender sub-mode
# output: $4 - state: the name of the new state (only set upon a change)
# output: $5 - re_mode: the desired range extender mode (updated only once
#                       the link to the AP is considered stable)
# output: $6 - re_submode: the desired range extender submode (updated only once
#                       the link to the AP is considered stable)
repacd_wifimon_check() {
    local wps_pressed=0
    local MBsta_is_Agent_connected=0
    config_load repacd
    config_get map_version MAPConfig 'MapVersionEnabled'
    config_get bsta_link_remove_active MAPConfig 'bstaLinkRemoveActive' '0'

    if [ "$map_version" -ge 6 -a "$enable_mlo" -eq 1 ]; then
        __repacd_wifimon_debug "Monitoring bsta link addition/removal"
        mldbsta_list=''
        mldbsta_iface_list=''
        mldbsta_config_name_list=''
        local check_disabled=0
        config_load wireless
        config_foreach __repacd_wifimon_get_mld_bstas wifi-iface $check_disabled
        __repacd_wifimon_debug "mldbsta_config_name_list : $mldbsta_config_name_list"
        __repacd_wifimon_add_bsta_link
        mldbsta_list=''
        mldbsta_iface_list=''
        mldbsta_config_name_list=''
        config_load wireless
        check_disabled=1
        config_foreach __repacd_wifimon_get_mld_bstas wifi-iface $check_disabled
        __repacd_wifimon_debug "mldbsta_config_name_list : $mldbsta_config_name_list"
        __repacd_wifimon_remove_bsta_link
        config_load repacd
        config_get bsta_link_remove_active MAPConfig 'bstaLinkRemoveActive' '0'
    fi
    if [ "$MBsta_IgnoreBstaOnEthBH" -eq 1 ] && [ "$MBsta_onboarding" -eq 1 ]; then
        #Disconnect if any sta interfaces connected when in Ethernet
        __repacd_wifimon_debug "Monintoring bsta vaps in Ethernet"
        for intf in $MBsta_sta_intf_list; do
            if __repacd_wifimon_is_assoc $intf; then
                __repacd_wifimon_debug "STA $iface was connected, disconnect it"
                __repacd_wifimon_MBsta_disconnect_link $intf
            fi
        done

    elif [ "$MBsta_IgnoreBstaOnEthBH" -eq 0 ]; then
        if [ -n "$sta_iface_24g" ] || [ -n "$sta_iface_5g" ]; then
            local wps_pbc
            local cur_assoc_state=0
            #Check if any of the STAs connected, to use it during wps push event
            if [ "$MBsta_onboarding" -eq 1 ]; then
                #We check for any intf associated by cfg call
                for intf in $MBsta_sta_intf_list; do
                    if __repacd_wifimon_is_assoc $intf; then
                        MBsta_is_Agent_connected=1
                        break
                    fi
                done

                if [ "$MBsta_is_Agent_connected" -eq 1 ]; then
                    __repacd_wifimon_debug "cur_assoc_state=1"
                    cur_assoc_state=1
                fi
            else
                if [ "$mldbsta_enabled" -eq 0 ]; then
                    #Check if any of the STAs connected, to use it during wps push event
                    if { [ -n "$sta_iface_24g" ] && __repacd_wifimon_is_assoc $sta_iface_24g; } \
                    || { [ -n "$sta_iface_5g" ] && __repacd_wifimon_is_assoc $sta_iface_5g; }; then
                        cur_assoc_state=1
                    fi
                else
                    #In MLO
                    agent_connected=0 #mark 0 first then check if agent is connected everytime
                    config_load wireless
                    config_foreach __repacd_wifimon_check_bsta_assoc wifi-iface
                    if [ "$agent_connected" -eq 1 ]; then
                        cur_assoc_state=1
                    fi
                fi
            fi
            if read -r -t 1 wps_pbc <>$WIFIMON_PIPE_NAME; then
                __repacd_wifimon_debug "Received $wps_pbc on wifimon pipe"

            #Change wifimon state only if none of the STAs connected before WPS
                if [ "$cur_assoc_state" -eq 0 ]; then
                    eval "$4=$WIFIMON_STATE_AUTOCONFIG_IN_PROGRESS"
                    wps_in_progress=1
                    __repacd_wifimon_get_timestamp wps_start_time
                    cnt_5g_attempts=0
                    uci_set repacd MAPWiFiLink '5gAttemptsCount' "$cnt_5g_attempts"
                    uci_commit repacd
                else
                    __repacd_wifimon_debug "STA is connected, No change in wifimon state"
                fi
            fi

            if __repacd_wifimon_check_associated "$1" "$2" "$3" 0 "$4" "$5" "$6"; then
                assoc_timeout_logged=0
                wps_timeout_logged=0
                if [ "$MBsta_onboarding" -eq 1 ]; then
                    if [ "$MBsta_non_mlo_mode" -eq 1 ] && __repacd_wifimon_is_assoc $sta_iface_24g ; then
                        __repacd_wifimon_debug "2G is associated"
                        local rssi=$(repacdcli $sta_iface_24g get_signal)
                        local gateway_ipaddr=$(ip r | awk '/^def/{print $3}')
                        __repacd_wifimon_debug "gateway_ipaddr: $gateway_ipaddr"
                        if [ "$rssi" -gt "$backhaul_rssi_2" ] && [ "$rssi" -lt 0 ] && ([ "$IS_GW_REACHABLE" -eq 0 ] || [ -z "$gateway_ipaddr" ]); then
                            #eval "$4=$WIFIMON_STATE_ASSOCIATED"
                            cnt_2gtimes=$((cnt_2gtimes + 1))
                            __repacd_wifimon_debug "cnt_2gtimes= $cnt_2gtimes - No of times 2G was associated but GW not reachable"
                            __repacd_wifimon_check_cac_status
                            if [ "$cnt_2gtimes" -gt 5 ] && [ "$cac_state" -eq 0 ]; then
                                eval "$4=$WIFIMON_STATE_ASSOCIATED"
                                __repacd_wifimon_debug "cur_state $cur_state new_state $new_state"
                                __repacd_wifimon_debug "Switch to MLO Links since 2G was congested"
                                local force_bsses_down=1
                                #__repacd_wifimon_select_next_bsta "$sta_iface_5g_config_name" \
                                    #      $force_bsses_down \
                                    #     "$WIFIMON_STATE_AUTOCONFIG_IN_PROGRESS" "$4"
                                eval "$4=$WIFIMON_STATE_RE_SWITCH_MLO_BSTA"
                                #Reset 5G Attempts Counter & MLO Attempts Counter
                                cnt_5g_attempts=0
                                cnt_mlo_attempts=0
                                uci_set repacd MAPWiFiLink 'MLOAttemptsCount' "$cnt_mlo_attempts"
                                uci_set repacd MAPWiFiLink '5gAttemptsCount' "$cnt_5g_attempts"
                                uci_commit repacd
                                cnt_2gtimes=0
                            fi
                        elif [ "$rssi" -gt "$backhaul_rssi_2" ] && [ "$rssi" -lt 0 ] && [ "$IS_GW_REACHABLE" -eq 1 ]; then
                            if [ "$cnt_2gtimes" -gt 0 ]; then
                                __repacd_wifimon_debug "cnt_2gtimes= $cnt_2gtimes Decrease now by 1 since GW was reachable"
                                cnt_2gtimes=$((cnt_2gtimes - 1)) #Decrease counter whenever GW is reachable on 2G
                            fi
                        fi
                    fi
                else #MBsta_onboarding disabled
                    if [ -n "$sta_iface_24g" ]; then
                        __repacd_wifimon_debug "2G is associated"
                        local rssi=$(repacdcli $sta_iface_24g get_signal)
                        local gateway_ipaddr=$(ip r | awk '/^def/{print $3}')
                        __repacd_wifimon_debug "gateway_ipaddr: $gateway_ipaddr"
                        if [ "$rssi" -gt "$backhaul_rssi_2" ] && [ "$rssi" -lt 0 ] && ([ "$IS_GW_REACHABLE" -eq 0 ] || [ -z "$gateway_ipaddr" ]); then
                            #eval "$4=$WIFIMON_STATE_ASSOCIATED"
                            cnt_2gtimes=$((cnt_2gtimes + 1))
                            __repacd_wifimon_debug "cnt_2gtimes= $cnt_2gtimes - No of times 2G was associated but GW not reachable"
                            __repacd_wifimon_check_cac_status
                            if [ "$cnt_2gtimes" -gt 5 ] && [ "$cac_state" -eq 0 ]; then
                                eval "$4=$WIFIMON_STATE_ASSOCIATED"
                                __repacd_wifimon_debug "cur_state $cur_state new_state $new_state"
                                __repacd_wifimon_debug "Select next bsta radio since 2G is overloaded,reset 5G & MLO Attempts counter"
                                local force_bsses_down=1
                                __repacd_wifimon_select_next_bsta "$sta_iface_5g_config_name" \
                                        $force_bsses_down \
                                        "$WIFIMON_STATE_AUTOCONFIG_IN_PROGRESS" "$4"
                                #Reset 5G Attempts Counter & MLO Attempts Counter
                                cnt_5g_attempts=0
                                cnt_mlo_attempts=0
                                uci_set repacd MAPWiFiLink 'MLOAttemptsCount' "$cnt_mlo_attempts"
                                uci_set repacd MAPWiFiLink '5gAttemptsCount' "$cnt_5g_attempts"
                                uci_commit repacd
                                cnt_2gtimes=0
                            fi
                        elif [ "$rssi" -gt "$backhaul_rssi_2" ] && [ "$rssi" -lt 0 ] && [ "$IS_GW_REACHABLE" -eq 1 ]; then
                            if [ "$cnt_2gtimes" -gt 0 ]; then
                                __repacd_wifimon_debug "cnt_2gtimes= $cnt_2gtimes Decrease now by 1 since GW was reachable"
                                cnt_2gtimes=$((cnt_2gtimes - 1)) #Decrease counter whenever GW is reachable on 2G
                            fi
                        fi
                    fi
                fi
                #WEATHER RADAR TIMER
                if [ "$pre_cac_enabled" -eq 1 ] && [ "$bsta_link_remove_active" -eq 0 ]; then
                    if __repacd_wifimon_is_mlo_bsta; then
                        local mlo_link_count=0 cur_time=0
                        config_load wireless
                        config_foreach __repacd_wifimon_check_mlo_link_assoc wifi-iface mlo_link_count
                        __repacd_wifimon_debug "mlo_link_count $mlo_link_count "
                        if [ "$mlo_link_count" -eq 2 ] && [ "$MBsta_onboarding" -eq 1 ]; then
                            MBsta_cnt_weather_radar_timer=0
                        fi
                        if [ "$weather_radar_start_timer" -eq 0 ]; then
                            if [ "$cnt_mlo_attempts" -lt "$max_mlo_attempts" ] && [ "$MBsta_onboarding" -eq 0 ]; then
                                if [ "$mlo_link_count" -eq 1 ]; then
                                    __repacd_wifimon_get_timestamp weather_radar_start_timer
                                    __repacd_wifimon_debug "START WEATHER RADAR TIMER at $weather_radar_start_timer"
                                fi
                            elif [ "$MBsta_onboarding" -eq 1 ]; then
                                if [ "$MBsta_cnt_weather_radar_timer" -lt "$max_mlo_attempts" ]; then
                                    if [ "$mlo_link_count" -eq 1 ]; then
                                        __repacd_wifimon_get_timestamp weather_radar_start_timer
                                        __repacd_wifimon_debug "START WEATHER RADAR TIMER at $weather_radar_start_timer"
                                    fi
                                elif [ "$MBsta_cnt_weather_radar_timer" -ge "$max_mlo_attempts" ]; then
                                    __repacd_wifimon_debug "MLO attempts made with radar timer reached. Not attempting anymore"
                                fi
                            fi
                        else
                            config_load wireless
                            mlo_link_count=0
                            config_foreach __repacd_wifimon_check_mlo_link_assoc wifi-iface mlo_link_count
                            __repacd_wifimon_debug "mlo_link_count $mlo_link_count "
                            if [ "$mlo_link_count" -lt 2 ]; then
                                if __repacd_wifimon_is_timeout $weather_radar_start_timer $weather_radar_timeout; then
                                    weather_radar_start_timer=0
                                    mlo_link_count=0
                                    config_foreach __repacd_wifimon_check_mlo_link_assoc wifi-iface mlo_link_count
                                    if [ "$mlo_link_count" -eq 1 ]; then
                                        __repacd_wifimon_debug "Reconnect MLO STAs, mlo_link_count:$mlo_link_count"
                                        config_foreach __repacd_wifimon_force_reassoc_mlo_sta wifi-iface
                                        if [ "$MBsta_onboarding" -eq 1 ]; then
                                            MBsta_cnt_weather_radar_timer=$((MBsta_cnt_weather_radar_timer + 1))
                                            __repacd_wifimon_debug "MLO Attempt made: $MBsta_cnt_weather_radar_timer"
                                        else
                                            cnt_mlo_attempts=$((cnt_mlo_attempts + 1))
                                            uci_set repacd MAPWiFiLink 'MLOAttemptsCount' "$cnt_mlo_attempts"
                                            uci_commit repacd
                                            __repacd_wifimon_debug "MLO Attempt made: $cnt_mlo_attempts"
                                        fi
                                        #Restart ping samples
                                        last_assoc_state=0
                                    fi
                                else
                                    __repacd_wifimon_get_timestamp cur_time
                                    __repacd_wifimon_debug "Weather CAC wait time: $((cur_time - weather_radar_start_timer))"
                                fi
                            else
                                __repacd_wifimon_debug "Both links connected, stop cac wait timer"
                                weather_radar_start_timer=0
                            fi
                        fi
                    fi
                fi
            else
                # not associated
                # If the WPS button was pressed, do not proceed to checking the
                # association timeouts. That will happen in subsequent checks.

                #Print the wpa_state of the bsta interfaces when disconnected for better debugging
                if [ "$MBsta_onboarding" -eq 1 ] && [ "$bsta_link_remove_active" -eq 0 ]; then
                    local bsta_interface=''
                    for bsta_interface in $MBsta_sta_intf_list; do
                        local tmp_MBsta_connected_bhssid=$(wpa_cli -i $bsta_interface -p /var/run/wpa_supplicant-$bsta_interface status | grep 'ssid' | awk 'FNR == 2 {print}' | awk -F = '{print$2}')
                        local wps_state=$(wpa_cli -i $bsta_interface -p /var/run/wpa_supplicant-$bsta_interface status | grep 'wpa_state' | awk -F = '{print$2}')
                        __repacd_wifimon_debug "Interface $bsta_interface, wpa_state: $wps_state"
                        if [ "$wps_state" = "INTERFACE_DISABLED" ]; then
                            __repacd_wifimon_debug "INTERFACE_DISABLED, $MBsta_non_mlo_sta_iface_list intf list bsta_interface: $bsta_interface"
                            __repacd_wifimon_debug "Interface $bsta_interface is down, bring up"
                            ifconfig $bsta_interface up
                        fi
                    done
                fi

                if [ "$wps_pressed" -gt 0 ]; then
                    return 1
                fi
                if [ -n "$sta_iface_24g" ]; then
                    cnt_2gtimes=0 #reset 2g unreachable counter everytime intf is not asociated
                    __repacd_wifimon_debug "cnt_2gtimes= $cnt_2gtimes make it 0 since Not-Associated"
                fi

                if [ "$wps_in_progress" -gt 0 ]; then
                    if __repacd_wifimon_is_timeout $wps_start_time $wps_timeout; then
                        if [ "$wps_timeout_logged" -eq 0 ]; then
                            __repacd_wifimon_debug "WPS timeout"
                            wps_timeout_logged=1
                        fi

                        eval "$4=$WIFIMON_STATE_WPS_TIMEOUT"
                    fi
                else
                    local cur_time
                    __repacd_wifimon_get_timestamp cur_time
                    if [ "$assoc_timeout_occurred" -eq 1 ] ||
                    [ "$cur_state" == "$WIFIMON_STATE_RE_SWITCH_BSTA" ]; then
                        __repacd_wifimon_get_timestamp assoc_start_time
                        last_assoc_state=0
                        __repacd_wifimon_debug "assoc_start_time $assoc_start_time"
                        __repacd_wifimon_debug "WILL RESET THE TIMER!!"
                        eval "$4=$WIFIMON_STATE_NOT_ASSOCIATED"
                        assoc_timeout_occurred=0
                        # Ideally AP should not go to 'Not-associated' state during backhaul
                        # steering as it should connect to either to target bss or fallback
                        # to previously connected bss, but if fallback too fails, then BHSInProgress
                        # flag needs to be reset here, so that if RSSI threshold checking works
                        # properly the next time AP get associated to a bss
                        (uci set ezmesh.MultiAP.BHSInProgress='0'; uci commit ezmesh)
                    fi
                    local elapsed_time=$((cur_time - assoc_start_time))
                    if [ "$MBsta_onboarding" -eq 1 ]; then
                            MBsta_mlo_attempt_time_duration=''
                            MBsta_mlo_attempt_start_time=''
                            disconnect_start_time=''
                    fi
                    # For MLO bSTA, choose MLOAssociationTimeout as association Timeout,
                    # Otherwise use default AssociationTimeout
                    __repacd_wifimon_debug "enable_mlo $enable_mlo MBsta_non_mlo_mode $MBsta_non_mlo_mode MBsta_mlo_mode $MBsta_mlo_mode MBsta_onboarding $MBsta_onboarding "
                    if [ "$enable_mlo" -eq 1 ] && __repacd_wifimon_is_mlo_bsta; then
                        cur_assoc_timeout=$mlo_assoc_timeout
                        mlo_bsta_connection_broken=1
                        if [ "$(($mlo_assoc_timeout-$elapsed_time))" -gt 0 ]; then
                            __repacd_wifimon_debug "MLO Assoc timeout in $(($mlo_assoc_timeout-$elapsed_time))"
                        fi
                    else
                        cur_assoc_timeout=$assoc_timeout
                        #If we experience a temporary disconnect of non mlo and
                        #we started the short timer then short timer would be marked as used
                        #which will unnecessarily force next mlo attempt to happen after long
                        #timer expires. So check if genuinely mlo failure happened in that case
                        #its ok to attempt for mlo after long timer expires. If not then we must
                        #reattempt MLO in a shorter time period.

                        if [ "$(($assoc_timeout-$elapsed_time))" -gt 0 ]; then
                            __repacd_wifimon_debug "Assoc timeout in $(($assoc_timeout-$elapsed_time))"
                        fi
                    fi
                    if __repacd_wifimon_is_timeout $assoc_start_time $cur_assoc_timeout; then
                        if [ "$assoc_timeout_logged" -eq 0 ]; then
                            __repacd_wifimon_debug "Association timeout"
                            assoc_timeout_logged=1
                            assoc_start_time=''
                            assoc_timeout_occurred=1
                        fi

                        __repacd_wifimon_debug " cur_assoc_state $cur_assoc_state"
                        __repacd_wifimon_debug "$sta_iface_5g stiface5g $sta_iface_24g
                                                staiface24g $sta_iface_backup staifacebackup"
                        __repacd_wifimon_debug "$sta_iface_config_name staifaceconfigname
                                                $sta_iface_5g_config_name stiface5gconfigname
                                                $sta_iface_24g_config_name staiface24gconfigname "
                        __repacd_wifimon_debug "**ASSOC TIMEOUT MLO MODE $MBsta_mlo_mode MBsta_non_mlo_mode $MBsta_non_mlo_mode"

                        # check CAC status
                        __repacd_wifimon_check_cac_status
                        __repacd_wifimon_debug "CAC STATE: $cac_state"
                        if [ "$cac_state" -eq 1 ] && [ -n "$sta_iface_24g" ] && [ "$MBsta_onboarding" -eq 0 ]; then
                            __repacd_wifimon_debug "Since bSTA is on 2G and CAC is in progress, not switing bSTA"
                            assoc_timeout_occurred=1
                            assoc_timeout_logged=0
                            return
                        fi

                        if [ -n "$sta_iface_backup" ] || __repacd_wifimon_is_mlo_bsta; then
                            # Try falling back to another interface
                            if [ "$MBsta_onboarding" -eq 0 ]; then
                                __repacd_wifimon_debug "Trying next bSTA radio"
                            fi

                            # Since MLO bSTA cound not associate, try with legacy STA mode
                            if [ "$enable_mlo" -eq 1 ] && __repacd_wifimon_is_mlo_bsta; then
                                if [ "$cnt_mlo_attempts" -lt "$max_mlo_attempts" ]; then
                                    cnt_mlo_attempts=$((cnt_mlo_attempts + 1))
                                    uci_set repacd MAPWiFiLink 'MLOAttemptsCount' "$cnt_mlo_attempts"
                                    uci_commit repacd
                                fi
                                if [ "$MBsta_onboarding" -eq 0 ]; then
                                    __repacd_map_switch_mld_to_legacy_sta "$4" #Preserve old way
                                else
                                    if [ "$onboarding_done" -eq 1 ] && [ "$backup_link" -eq 1 ]; then #We only do bsta switching if Onboarding is done
                                        MLOtolegacyvariable="true"
                                        __repacd_wifimon_debug "Switch from MLO bSTA to legacy bSTA"
                                        eval "$4=$WIFIMON_STATE_RE_SWITCH_BSTA"
                                        MBsta_mlo_mode=0
                                        MBsta_non_mlo_mode=0
                                    else
                                        config_load wireless
                                        config_foreach __repacd_wifimon_MBsta_check_bhssid wifi-iface $MBsta_mlo_sta_iface_list
                                        if [ "$MBsta_multi_up_required" -eq 1 ]; then
                                            MBsta_multi_up_list="$MBsta_mlo_bstaMld $MBsta_multi_up_list"
                                            MBsta_multi_up_required=0
                                        else
                                            __repacd_wifimon_MBsta_reconnect_link $MBsta_mlo_sta_iface_list
                                        fi
                                        if [ -n "$MBsta_non_mlo_sta_iface_list" ]; then
                                            config_foreach __repacd_wifimon_MBsta_check_bhssid wifi-iface $MBsta_non_mlo_sta_iface_list
                                        fi
                                        if [ "$MBsta_multi_up_required" -eq 1 ]; then
                                            MBsta_multi_up_list="$MBsta_nonmlo_bstaMld $MBsta_multi_up_list"
                                            MBsta_multi_up_required=0
                                        else
                                            __repacd_wifimon_MBsta_reconnect_link $MBsta_non_mlo_sta_iface_list
                                        fi
                                        if [ -n "$MBsta_multi_up_list" ]; then
                                            __repacd_wifimon_debug "bSTA credential changed, update it with multi_up vaps:$MBsta_multi_up_list"
                                            wifi multi_up $MBsta_multi_up_list
                                            MBsta_multi_up_list=''
                                        else
                                            #Let's issue wps reconnect upon disassoc if no backup link
                                            __repacd_wifimon_MBsta_reconnect_link $MBsta_sta_intf_list
                                        fi
                                        __repacd_wifimon_debug "Reset Assoc timeout timer, issue connect on all sta interfaces."
                                        __repacd_wifimon_get_timestamp assoc_start_time
                                        last_assoc_state=0
                                        MBsta_mlo_mode=0
                                        MBsta_non_mlo_mode=0
                                        uci commit repacd.MAPConfig.MBsta_mlo_mode='0'
                                        uci commit repacd.MAPConfig.MBsta_non_mlo_mode='0'
                                        uci commit repacd
                                    fi
                                fi
                                return
                            fi
                            # Since this bSTA could not associate, assume the next
                            # one might not be able to either. Thus, tell the
                            # fronthaul manager to force the BSSes down at startup.
                            if [ "$MBsta_onboarding" -eq 0 ]; then
                                local force_bsses_down=1
                                __repacd_wifimon_select_next_bsta "$sta_iface_5g_config_name" \
                                $force_bsses_down \
                                "$WIFIMON_STATE_AUTOCONFIG_IN_PROGRESS" "$4"
                            else
                                #backup link also disconnected so let's keep all radios open to connect
                                __repacd_wifimon_debug "onboarding_done: $onboarding_done backup_link:$backup_link"
                                __repacd_wifimon_debug "Reset Timer"
                                __repacd_wifimon_get_timestamp assoc_start_time
                                config_load wireless
                                config_foreach __repacd_wifimon_MBsta_check_bhssid wifi-iface $MBsta_mlo_sta_iface_list
                                if [ "$MBsta_multi_up_required" -eq 1 ]; then
                                    MBsta_multi_up_list="$MBsta_mlo_bstaMld $MBsta_multi_up_list"
                                    MBsta_multi_up_required=0
                                else
                                    __repacd_wifimon_MBsta_reconnect_link $MBsta_mlo_sta_iface_list
                                fi
                                if [ -n "$MBsta_non_mlo_sta_iface_list" ]; then
                                    config_foreach __repacd_wifimon_MBsta_check_bhssid wifi-iface $MBsta_non_mlo_sta_iface_list
                                fi
                                if [ "$MBsta_multi_up_required" -eq 1 ]; then
                                    MBsta_multi_up_list="$MBsta_nonmlo_bstaMld $MBsta_multi_up_list"
                                    MBsta_multi_up_required=0
                                else
                                    __repacd_wifimon_MBsta_reconnect_link $MBsta_non_mlo_sta_iface_list
                                fi
                                if [ -n "$MBsta_multi_up_list" ]; then
                                    __repacd_wifimon_debug "bSTA credential changed, update it with multi_up vaps:$MBsta_multi_up_list"
                                    wifi multi_up $MBsta_multi_up_list
                                    MBsta_multi_up_list=''
                                else
                                    #Let's issue wps reconnect upon disassoc if no backup link
                                    __repacd_wifimon_MBsta_reconnect_link $MBsta_sta_intf_list
                                fi
                                last_assoc_state=0
                            fi
                        else
                            # Already on 2.4 GHz. If we cannot associate here, we
                            # probably cannot associate on 5 GHz.
                            eval "$4=$WIFIMON_STATE_ASSOC_TIMEOUT"
                        fi
                    elif [ -n "$config_bssid" -a "$MBsta_onboarding" -eq 0 ] && \
                        __repacd_wifimon_is_timeout $assoc_start_time $bssid_timeout; then
                        __repacd_wifimon_debug "Timeout trying to associate with $config_bssid"

                        # Remove our BSSID constraint in the hope that some other AP
                        # is out there that will be able to serve us.
                        __repacd_wifimon_set_target_bssid "$WIFIMON_WILDCARD_BSSID" "$config_bssid"
                    elif [ "$MBsta_onboarding" -eq 1 ] && \
                        __repacd_wifimon_is_timeout $assoc_start_time $bssid_timeout; then
                        __repacd_wifimon_debug "SETTING WILDCARD BSSID"
                        config_load wireless
                        config_foreach __repacd_wifimon_MBsta_set_wildcard_bssid wifi-iface
                    fi
                fi
            fi
        fi
    fi
}

#Set Wildcard bssid upon bssid timeout
#for the sta ifaces to scan and connect
__repacd_wifimon_MBsta_set_wildcard_bssid() {

    local config="$1"
    local mode iface cbssid

    config_get mode "$config" mode
    config_get iface "$config" ifname
    config_get cbssid "$config" bssid


    if [ "$mode" = "sta" ]; then
        local intf=''
        local intflist=''
        if [ "$MBsta_mlo_mode" -eq 1 ]; then
            intflist="$MBsta_mlo_sta_iface_list"
        elif [ "$MBsta_non_mlo_mode" -eq 1 ]; then
            intflist="$MBsta_non_mlo_sta_iface_list"
        elif [ "$MBsta_mlo_mode" -eq 0 ] && [ "$MBsta_non_mlo_mode" -eq 0 ]; then
            intflist="$MBsta_sta_intf_list"
        fi
        for intf in $intflist; do
            if [ "$intf" = "$iface" ] && [ -n "$cbssid" ]; then
                __repacd_wifimon_MBsta_set_target_bssid "$WIFIMON_WILDCARD_BSSID" "$cbssid" "$iface"
            fi
        done
    fi
}

# Hook function that, in SON mode, would determine whether to bring up/down the
# 2.4 GHz backhaul. In Multi-AP SIG mode, this is not currently relevant
# (given the 1 backhaul link assumption).
repacd_wifimon_independent_channel_check() {
    /bin/true
}

# Terminate the Wi-Fi link monitoring, cleaning up any state in preparation
# for shutdown.
repacd_wifimon_fini() {
    __repacd_stop_ping
    /bin/true
}

# Resolve the credentials for any VAP that has bBSS support.
#
# input: $1 - config: the name of the interface config section
# input: $2 - network: the name of the network to which the AP interface
#                      must belong to be matched
# output: $3 - ssid: the backhaul SSID
# output: $4 - key: the backhaul key (aka. passphrase)
# output: $5 - enc: the backhaul encryption mode
# output: $6 - sae: SAE authentication mode
__repacd_wifimon_resolve_bbss_creds() {
    local config="$1"
    local network_to_match="$2"

    local network mode device hwmode disabled
    local ssid key enc map map_type

    config_get network "$config" network
    config_get mode "$config" mode
    config_get device "$config" device
    config_get hwmode "$device" hwmode
    config_get disabled "$config" disabled '0'

    config_get ssid "$config" ssid
    config_get key "$config" key
    config_get enc "$config" encryption
    config_get map "$config" map '0'
    config_get map_type "$config" MapBSSType
    config_get sae "$config" sae '0'

    if [ "$hwmode" != "11ad" ]; then
        if [ "$network" = "$network_to_match" ] && [ "$mode" = "ap" ] && \
            [ "$disabled" -eq 0 ] && [ "$map" -gt 0 ]; then
            local bbss=$(( map_type & 64 ))
            if [ ${bbss} -eq 64 ]; then
                # Has bBSS capabilty, so record the credentials
                eval "$3='${ssid}'"
                eval "$4='${key}'"
                eval "$5='${enc}'"
                eval "$6='${sae}'"
            fi
        fi
    fi
}

# Set the credentials on the bSTA VAP(s).
#
# input: $1 - config: the name of the interface config section
# input: $2 - network: the name of the network to which the STA interface
#                      must belong to be matched
# input: $3 - ssid: the backhaul SSID
# input: $4 - key: the backhaul key (aka. passphrase)
# input: $5 - enc: the backhaul encryption mode
# input: $6 - sae: SAE authentication mode
__repacd_wifimon_set_bsta_creds() {
    local config="$1"
    local network_to_match="$2"
    local ssid="$3"
    local key="$4"
    local enc="$5"
    local sae="$6"

    local network mode hwmode device disabled
    local ssid key enc map map_type

    config_get network "$config" network
    config_get mode "$config" mode
    config_get device "$config" device
    config_get hwmode "$device" hwmode
    config_get disabled "$config" disabled '0'

    config_get map "$config" map '0'
    config_get map_type "$config" MapBSSType

    if [ "$hwmode" != "11ad" ]; then
        if [ "$network" = "$network_to_match" ] && [ "$mode" = "sta" ] && \
            [ "$map" -gt 0 ]; then
            if [ "$map_type" -eq 128 ]; then  # bSTA
                uci_set wireless "$config" ssid "$ssid"
                uci_set wireless "$config" key "$key"
                uci_set wireless "$config" encryption "$enc"
                uci_set wireless "$config" sae "$sae"
                if [ "$map_fast_onboarding" -eq 0 ]; then
                    uci_set wireless "$config" disabled "0"
                fi
                uci_commit wireless
            fi
        fi
    fi
}

# Set the credentials on the bSTA VAP for DPP(s).
#
# input: $1 - config: the name of the interface config section
# input: $2 - ssid: the backhaul SSID
# input: $3 - key: the backhaul key (aka. passphrase)
# input: $4 - enc: the backhaul encryption mode
# input: $5 - connector: DPP Connector
# input: $6 - csign: DPP CSIGN
# input: $7 - net_access: DPP Net Access Key
# input: $8 - connector_1905: DPP 1905 connector
__repacd_wifimon_set_bsta_creds_dpp() {
    local config="$1"
    local ssid="$2"
    local key="$3"
    local enc="$4"
    local connector="$5"
    local csign="$6"
    local net_access="$7"
    local connector_1905="$8"
    local encr_key

    local mode hwmode device disabled
    local ssid key enc map map_type connector

    config_get mode "$config" mode
    config_get device "$config" device
    config_get hwmode "$device" hwmode
    config_get disabled "$config" disabled '0'

    config_get map "$config" map '0'
    config_get map_type "$config" MapBSSType

    if [ "$enc" = "dpp" ]; then
        encr_key="dpp"
    else
        encr_key="psk2+ccmp"
    fi

    if [ "$hwmode" != "11ad" ]; then
        if [ "$mode" = "sta" ] && [ "$map" -ge 3 ]; then
            if [ "$map_type" -eq 128 ]; then  # bSTA
                uci_set wireless "$config" ssid "$ssid"
                uci_set wireless "$config" key "$key"
                uci_set wireless "$config" encryption "$encr_key"
		if [ "$map_fast_onboarding" -eq 0 ]; then
                    uci_set wireless "$config" disabled "0"
                fi
                uci_set wireless "$config" dpp_connector "$connector"
                uci_set wireless "$config" dpp_csign "$csign"
                uci_set wireless "$config" dpp_netaccesskey "$net_access"
                uci_set wireless "$config" dpp_1905_connector "$connector_1905"
                uci_commit wireless
            fi
        fi
    fi
}

# Copy the backhaul credentials from a bBSS interface (or a combined
# fBSS + bBSS) to the bSTA interface. This ensures the credentials obtained
# during Ethernet onboarding can be used when switching into RE mode.
repacd_wifimon_config_bsta() {
    bbss_ssid=''
    bbss_key=''
    bbss_enc=''
    bbss_sae=''
    bbss_connector='' bbss_csign='' bbss_netaccess='' bbss_connector_1905=''
    local OnboardingDone

    config_load repacd
    config_get map_version MAPConfig 'MapVersionEnabled' '0'
    config_get map_ts_enabled MAPConfig 'MapTrafficSeparationEnable' '0'
    config_get map_backhaul_nw MAPConfig 'VlanNetworkBackHaul'
    config_get onboarding_type MAPConfig 'OnboardingType'
    config_get map_fast_onboarding 'MAPConfig' 'MapFastOnboarding' '0'
    config_get OnboardingDone MAPConfig 'OnboardingDone' '0'

    if [ "$map_fast_onboarding" -eq 1 ] && [ "$OnboardingDone" -eq 0 ]; then
        __repacd_wifimon_debug "Skip copying Cred to STA"
        __repacd_wifimon_debug "Onboarding not yet done . BhBSS wont have correct cred"
        return
    fi

    # First resolve the SSID, passphrase, and encryption from the bBSS
    # capable AP interfaces.
    config_load wireless
    config_foreach __repacd_wifimon_resolve_bbss_creds wifi-iface "$1" \
                   bbss_ssid bbss_key bbss_enc bbss_sae
    if [ "$map_version" -ge 2 -a "$map_ts_enabled" -gt 0 ]; then
        config_foreach __repacd_wifimon_resolve_bbss_creds wifi-iface "$map_backhaul_nw" \
                       bbss_ssid bbss_key bbss_enc bbss_sae
    fi

    # Now apply these values to the bSTA interface.
    config_foreach __repacd_wifimon_set_bsta_creds wifi-iface "$1" \
                   "${bbss_ssid}" "${bbss_key}" "${bbss_enc}" "${bbss_sae}"
    if [ "$map_version" -ge 2 -a "$map_ts_enabled" -gt 0 ]; then
        config_foreach __repacd_wifimon_set_bsta_creds wifi-iface "$map_backhaul_nw" \
                       "${bbss_ssid}" "${bbss_key}" "${bbss_enc}" "${bbss_sae}"
    fi

    if [ "$map_version" -ge 3 -a "$onboarding_type" = "dpp" ]; then
        bbss_ssid=$(cat /tmp/map_sta_info.tmp | grep DPP_STA_SSID | cut -d '@' -f2 | tail -1)
        bbss_key=$(cat /tmp/map_sta_info.tmp | grep DPP_STA_PASS | cut -d '@' -f2 | tail -1)
        bbss_enc=$(cat /tmp/map_sta_info.tmp | grep DPP_STA_AKM | cut -d '@' -f2 | tail -1)
        bbss_connector=$(cat /tmp/map_sta_info.tmp | grep DPP_STA_CONNECTOR | \
                             cut -d '@' -f2 | tail -1)
        bbss_csign=$(cat /tmp/map_sta_info.tmp | grep DPP_STA_CSIGN | cut -d '@' -f2 | tail -1)
        bbss_netaccess=$(cat /tmp/map_sta_info.tmp | grep DPP_STA_NET_ACCESS | \
                             cut -d '@' -f2 | tail -1)
        bbss_connector_1905=$(cat /tmp/map_sta_info.tmp | grep DPP_STA_1905_CONNECTOR | \
                             cut -d '@' -f2 | tail -1)
        if [ -n "$bbss_ssid" ] && [ -n "$bbss_key" ]; then
            config_foreach __repacd_wifimon_set_bsta_creds_dpp wifi-iface  \
                       "${bbss_ssid}" "${bbss_key}" "${bbss_enc}" "${bbss_connector}" \
                       "${bbss_csign}" "${bbss_netaccess}" "$bbss_connector_1905"
        else
            __repacd_wifimon_debug "Skip DPP Bsta configuration, as the credentials are not cloned"
        fi
    fi
}

__repacd_trigger_chirp() {
    local mod_value
    local staBitRate=$(repacdcli $dpp_sta_iface get_bitrate)
    local connectorFound

    if [ -z "$dpp_sta_iface" ]; then
        return
    fi

    if [ "$staBitRate" -eq 0 -o -z "$staBitRate" ]; then
        __repacd_wifimon_debug "DPP Sta $dpp_sta_iface Bit Rate Invalid"
        __repacd_wifimon_debug "Number of Chirps sent $chirp_count"
        if [ "$chirp_count" -eq 0 ]; then
            wpa_cli -p /var/run/wpa_supplicant-$dpp_sta_iface dpp_configurator_remove 1
            wpa_cli -p /var/run/wpa_supplicant-$dpp_sta_iface dpp_bootstrap_gen type=qrcode \
                    key=$dpp_key
            wpa_cli -p /var/run/wpa_supplicant-$dpp_sta_iface dpp_bootstrap_info 1
            wpa_cli -p /var/run/wpa_supplicant-$dpp_sta_iface dpp_bootstrap_get_uri 1
        fi

        if [ -z "$chirp_start_time" ]; then
            __repacd_wifimon_get_timestamp chirp_start_time
        fi

        local connLen=${#dpp_sta_connector}
        if [ -z "$dpp_sta_connector" ] || [ "$dpp_sta_connector" -eq 0 ] || [ "$connLen" -eq 0 ]; then
            __repacd_wifimon_debug "Found InValid STA connector $dpp_sta_iface"
            local cur_time
            __repacd_wifimon_get_timestamp cur_time
            local elapsed_time=$((cur_time - chirp_start_time))

            if [ "$(($chirp_timeout-$elapsed_time))" -gt 0 ]; then
                __repacd_wifimon_debug "Next Chirp in $(($chirp_timeout-$elapsed_time))"
            fi

            if __repacd_wifimon_is_timeout $chirp_start_time $chirp_timeout; then
                __repacd_wifimon_debug "Start Chirping on $dpp_sta_iface"
                wpa_cli -p /var/run/wpa_supplicant-$dpp_sta_iface dpp_configurator_remove 1
                wpa_cli -p /var/run/wpa_supplicant-$dpp_sta_iface dpp_bootstrap_gen type=qrcode \
                        key=$dpp_key
                wpa_cli -p /var/run/wpa_supplicant-$dpp_sta_iface dpp_stop_listen
                wpa_cli -p /var/run/wpa_supplicant-$dpp_sta_iface dpp_chirp own=1
                chirp_count=$((chirp_count + 1))
                chirp_start_time=''
                connector_reset_count=0
            fi
        else
            __repacd_wifimon_debug "Connector reset count $connector_reset_count"
            __repacd_wifimon_debug "Connector retry $dpp_connector_retry"
            connector_reset_count=$((connector_reset_count + 1))
            if [ "$connector_reset_count" -ge 1 ] && [ "$connector_reset_count" -le 3 ] \
                   && [ "$dpp_connector_retry" -eq 0 ]; then
                __repacd_wifimon_get_timestamp assoc_start_time
            fi
            local cur_time
            __repacd_wifimon_get_timestamp cur_time
            local elapsed_time=$((cur_time - assoc_start_time))
            if [ "$(($assoc_timeout-$elapsed_time))" -gt 0 ]; then
                __repacd_wifimon_debug "Assoc timeout in $(($assoc_timeout-$elapsed_time))"
            fi
            # We do not want to force reassociation after receiving connector, so mark
            # it as not needed through this special value.
            reassoc_forced=2
        fi
    else
        chirp_start_time=''
        dpp_sta_connector=0
        chirp_count=0
        connector_reset_count=0
        dpp_connector_retry=0
    fi
}

__repacd_wifimon_get_dpp_sta_iface() {
    local config="$1"
    local iface disabled mode

    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode

    if [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" = "sta" ]; then
        dpp_sta_iface=$iface
        config_get dpp_sta_connector "$config" dpp_connector '0'
        if [ "$connector_reset_count" -ge "$connector_reset_threshold" ]; then
            __repacd_wifimon_debug "Invalid STA Connector . Reset"
            uci_set wireless "$config" dpp_connector '0'
            uci_commit wireless
            dpp_sta_connector=0
            dpp_connector_retry=$((dpp_connector_retry + 1))
        fi
    fi
}


_mlo_connection_monitor() {
    local cur_time=0
    local elapsed_time=0
    if [ $cnt_mlo_attempts -gt '0' ]; then
        if [ "$mldbsta_enabled" -eq '1' ] && [ "$cnt_mlo_attempts" -lt "$max_mlo_attempts" ]; then
            if [ $mlo_start_time -eq '0' ]; then
                 __repacd_wifimon_get_timestamp mlo_start_time
            fi
            if __repacd_wifimon_is_timeout $mlo_start_time $mlo_stable_timeout; then
                cnt_mlo_attempts=0
                cnt_5g_attempts=0
                mlo_start_time=0
                uci_set repacd MAPWiFiLink 'MLOAttemptsCount' "$cnt_mlo_attempts"
                uci_set repacd MAPWiFiLink '5gAttemptsCount' "$cnt_5g_attempts"
                last_assoc_state=0
                __repacd_wifimon_debug "last_assoc_state making 0 for resampling mlo connection stable  $cnt_mlo_attempts, $mlo_stable_timeout"
            else
                __repacd_wifimon_get_timestamp cur_time
                elapsed_time=$((cur_time - mlo_start_time))
                __repacd_wifimon_debug "mlo connection monitoring  $elapsed_time, $cnt_mlo_attempts,$mlo_stable_timeout"
            fi
        elif [ "$cnt_mlo_attempts" -ge "$max_mlo_attempts" ]; then
            if [ $mlo_blocked_start_time -eq '0' ]; then
                 __repacd_wifimon_get_timestamp mlo_blocked_start_time
            fi

            if __repacd_wifimon_is_timeout $mlo_blocked_start_time $mlo_blocked_timeout; then
                cnt_mlo_attempts=0
                cnt_5g_attempts=0
                mlo_blocked_start_time=0
                uci_set repacd MAPWiFiLink 'MLOAttemptsCount' "$cnt_mlo_attempts"
                uci_set repacd MAPWiFiLink '5gAttemptsCount' "$cnt_5g_attempts"
                last_assoc_state=0
                __repacd_wifimon_debug "Make lastassoc= 0"
                #Make bssid null else will get copied when configuring mlo
                config_load wireless
                config_foreach __repacd_wifimon_traverse_to_sta wifi-iface

            fi
            __repacd_wifimon_get_timestamp cur_time
            elapsed_time=$((cur_time - mlo_blocked_start_time))
            __repacd_wifimon_debug "blocked mlo connection monitoring  $elapsed_time, $cnt_mlo_attempts, $mlo_blocked_timeout"
        fi
        if [ "$mldbsta_enabled" -eq '0' ] || [ "$mlo_bsta_connection_broken" -eq '1' ]; then
            mlo_start_time=0
            mlo_bsta_connection_broken=0
            __repacd_wifimon_debug "Reset mlo_start_time $mlo_start_time $mlo_bsta_connection_broken"
        fi
    fi
}

repacd_wifimon_dpp_check_onboarding_status() {
    local restartWifi VapSpecificUpdate wifiArgs=

    # First resolve the config parameters.
    config_load wsplcd
    config_get VapSpecificUpdate config 'VapSpecificRestart' '0'
    config_load repacd
    config_get_bool restartWifi MAPConfig 'restartWifiDPP' '0'
    config_get wifiArgs MAPConfig 'restartWifiargs' "NULL"
    config_get onboarding_type MAPConfig 'OnboardingType'
    config_get chirp_timeout MAPConfig 'ChirpTimeout' '30'
    config_get connector_reset_threshold MAPConfig 'ConnectorResetThreshold' '30'
    if [ "$restartWifi" -eq 1 ] && [ "$map_fast_onboarding" -eq 0 ]; then
        uci set repacd.MAPConfig.restartWifiDPP='0'
        uci set repacd.MAPConfig.restartWifiargs="NULL"
        uci commit repacd
        if [ "$wifiArgs" == "NULL" ] || [ "$VapSpecificUpdate" -eq "0" ]; then
            wifi load
        else
            wifi multi_up $wifiArgs
        fi
    fi

    # Check STA iface
    config_load wireless
    config_foreach __repacd_wifimon_get_dpp_sta_iface wifi-iface

    if [ "$onboarding_type" = "dpp" ]; then
        config_load $MAP
        config_get dpp_key MAPConfigSettings 'DPPConfiguratorKey'

        if [ -z "$dpp_key" ]; then
            __repacd_wifimon_debug "Invalid DPP Key $dpp_key"
            return
        fi

        __repacd_trigger_chirp
    fi
}

__repacd_map_teardown_vap() {
    local config="$1"
    local iface network disabled device MapBSSType mode

    config_get iface "$config" ifname
    config_get network "$config" network
    config_get disabled "$config" disabled '0'
    config_get device "$config" device
    config_get MapBSSType "$config" MapBSSType '0'
    config_get mode "$config" mode

    if [ "$mode" = "ap_smart_monitor" ]; then
        return
    fi

    if [ $((MapBSSType & 0x10)) -eq 16 ]; then
        __repacd_is_ap_iface_up $iface
        local iface_up=$?
        if [ "$iface_up" -eq 0 ]; then
            continue
        else
            __repacd_wifimon_debug "teardown"
            __repacd_wifimon_debug "Bringing down $iface"
            hapd $iface disable
            teardown_done=1
        fi
    fi
}

__repacd_map_teardown_vap_OPT() {
    local config="$1"
    local iface device MapBSSType mode mld exist mld_exist

    config_get iface "$config" ifname
    config_get device "$config" device
    config_get MapBSSType "$config" MapBSSType '0'
    config_get mode "$config" mode
    config_get mld "$config" mld ''

    if [ "$mode" = "ap_smart_monitor" ]; then
        return
    fi

    if [ $((MapBSSType & 0x10)) -eq 16 ]; then
        __repacd_wifimon_debug "teardown vap, delete $iface."
        uci delete wireless.$config
        teardown_done=1
        exist=$(echo $teardown_radio | grep -w $device)
        if [ -z "$exist" ]; then
            teardown_radio="$teardown_radio $device"
        fi

        if [ -n "$mld" ]; then
            mld_exist=$(echo $teardown_mld | grep -w $mld)
            [ -z "$mld_exist" ] && teardown_mld="$teardown_mld $mld"
        fi
    fi
}

# Determine if a string contains duplicates
 __repacd_update_multiup_mld_args() {
    local list="$1"
    local mld_exist=''

    if [ -n "$list" ]; then
        for mld in $list; do
            mld_exist=$(echo $opt_wifi_list | grep -w $mld)
            [ -z "$mld_exist" ] && opt_wifi_list="$opt_wifi_list $mld"
        done
    fi
}

#During mlo groping some mld may be unused. to remove those from bridge it is added to mld_change_list
#In mlo reconfiguring or switching from MLO to SLO, one partner mld will not have config change set to ture. This logic will resolve that.
__repacd_map_mld_remove_list() {
    local config="$1"
    local remove_mld=0 exist

    config_get remove_mld "$config" wsplcdRemoveMld 0

    if [ "$remove_mld" -eq 1 ]; then
        exist=$(echo $mld_changed_list | grep -w $config)
        if [ -z "$exist" ]; then
            mld_changed_list="$mld_changed_list $config"
            __repacd_wifimon_debug "Added $config to mld_changed_list"
        fi
    fi

    #TODO: clean wsplcdRemoveMld before VAP credential update
    uci_set wireless "$config" wsplcdRemoveMld "0"
}

__repacd_map_teardown_list() {
    local config="$1"
    local iface network disabled device MapBSSType mode

    config_get iface "$config" ifname
    config_get network "$config" network
    config_get disabled "$config" disabled '0'
    config_get device "$config" device
    config_get MapBSSType "$config" MapBSSType '0'
    config_get mode "$config" mode

    if [ "$mode" = "ap_smart_monitor" ]; then
        return
    fi

    if [ $((MapBSSType & 0x10)) -eq 16 ]; then
        teardown_list="$teardown_list $device $iface"
    fi
}

__repacd_map_check_onboarding() {
    local config="$1"
    local wifiName="$2"
    local iface network disabled device MapBSSType mode map_config_changed
    local last_intf=''
    local mld
    local exist_intf=1

    config_get iface "$config" ifname
    config_get network "$config" network
    config_get disabled "$config" disabled '0'
    config_get device "$config" device
    config_get MapBSSType "$config" MapBSSType '0'
    config_get mode "$config" mode
    config_get map_config_changed "$config" map_config_changed '0'
    config_get mld "$config" mld ''
    config_get onboarding_isdone MAPConfig 'OnboardingDone' '0'
    config_get virtualap "$config" 'virtualAP' '0'

    total_vap_count=$((total_vap_count + 1))
    if [ "$mode" = "ap_smart_monitor" ]; then
        smartmonitor_list="$smartmonitor_list $iface"
        return
    fi

    if [ "$virtualap" == "1" ] && [ "$external_controller" == "1" ]; then
        virtualap_list="$virtualap_list $iface"
        return
    fi

    if [ $((MapBSSType & 0x10)) -eq 16 ]; then
        __repacd_wifimon_debug "Teardown $iface"
        sed -i '/ssid/d' /var/run/hostapd-$iface.conf
        echo "ssid=teardown" >> /var/run/hostapd-$iface.conf
        hostapd_cli -i $iface-p /var/run/hostapd-$device reload_config
        cfg80211tool_mesh $iface MapBSSType 16
        __repacd_wifimon_debug "Bringing down $iface"
        hapd $iface disable
        return
    fi

    if [ "$mode" = "sta" ]; then
        sta_device=$device
        if [ -n "$iface" ]; then
            __repacd_wifimon_debug "STA iface $iface"
            sta_iface_current=$iface

            # Reload wifi when new sta config has been received in M2 without
            # skipping restart inorder to re-connect with new credentials
            if [ "$disabled" -eq 0 ] && [ "$map_config_changed" -eq 1 ]; then
                uci set repacd.MAPConfig.SkipStaRestart='0'
                uci commit repacd
            fi

            config_load 'repacd'
            config_get skip_sta_restart MAPConfig 'SkipStaRestart' '0'
            # Prevent appending of sta vap in multi_up command,
            # thereby skipping restart of sta vap.
            if [ "$skip_sta_restart" -eq 1 ]; then
                __repacd_wifimon_debug "Skip STA iface $iface"
                return
            else
                map_config_changed=1
            fi
        fi
    fi

    if [ "$disabled" -eq 1 ] && [ "$mode" = "ap" ]; then
        __repacd_wifimon_debug "Bring down $iface"
        return
    fi

    if [ -z "$iface" ] && [ "$mode" = "ap" ]; then
        new_vaps_added=1
        exist_intf=0
        __repacd_wifimon_debug "Invalid ifname $iface for device $device"
        local devVal=$(eval echo \$"$device"_config_list)
        last_vap=$(echo $devVal | awk '{print $NF}')
        __repacd_wifimon_debug "last intf $last_vap"

        if [ -z "$last_vap" ]; then
            __repacd_wifimon_debug "last intf not found in restart list"
            local devVal=$(eval echo \$"$device"_list)
            last_vap=$(echo $devVal | awk '{print $NF}')
            __repacd_wifimon_debug "last intf from wifi list $last_vap"
        fi

        local rIdx=$(echo $last_vap | cut -c 4)
        local vapIdx=$(echo $last_vap | cut -c 5)
        vapIdx=$((vapIdx + 1))
        iface=ath$rIdx$vapIdx

        local isSmartMonVap=$(echo $smartmonitor_list | grep -w $iface)
        if [ -n "$isSmartMonVap" ]; then
            __repacd_wifimon_debug "New Intf is smart monitor Vap. Get next Idx"
            vapIdx=$((vapIdx + 1))
            iface=ath$rIdx$vapIdx
        fi

        local isVirtualAp=$(echo $virtualap_list | grep -w $iface)
        if [ -n "$isVirtualAp" ] && [ "$external_controller" == "1" ]; then
            __repacd_wifimon_debug "New Intf is VirtualAp. Get next Idx"
            vapIdx=$((vapIdx + 1))
            iface=ath$rIdx$vapIdx
        fi

        if [ "$iface" = "$sta_iface" ]; then
            __repacd_wifimon_debug "New Intf is STA Vap. Get next Idx"
            vapIdx=$((vapIdx + 1))
            iface=ath$rIdx$vapIdx
            config_load 'repacd'
            config_get skip_sta_restart MAPConfig 'SkipStaRestart' '0'
            if [ "$skip_sta_restart" -eq 1 ]; then
                iface="$sta_iface_current $iface"
            fi
        fi
    fi

    __repacd_wifimon_debug "iface: $iface, config_changed: $map_config_changed mld: $mld"
    if [ "$map_config_changed" -eq 1 ]; then
        wifi_reload_req=1
        if [ "$device" = "wifi0" ]; then
            wifi0_config_list="$wifi0_config_list $iface"
        elif [ "$device" = "wifi1" ]; then
            wifi1_config_list="$wifi1_config_list $iface"
        elif [ "$device" = "wifi2" ]; then
            wifi2_config_list="$wifi2_config_list $iface"
        elif [ "$device" = "wifi3" ]; then
            wifi3_config_list="$wifi3_config_list $iface"
        elif [ "$device" = "wifi4" ]; then
            wifi4_config_list="$wifi4_config_list $iface"
        else
            __repacd_wifimon_debug "Invlaid device: $device"
            return
        fi

        if [ -n "$mld" ]; then
            mld_vap_device="$mld_vap_device $iface"
            local addMld=$(echo $mld_list | grep -w $mld)
            if [ -z "$addMld" ]; then
                if [ "$MBsta_onboarding" -eq 0 ]; then
                        mld_list="$mld_list $mld"
                elif [ "$MBsta_onboarding" -eq 1 ] && [ "$mode" != "sta" ]; then
                mld_list="$mld_list $mld"
                fi
                if [ "$exist_intf" -eq "1" ]; then
                    __repacd_wifimon_debug "Interface $iface is already exist"
                    if [ "$MBsta_onboarding" -eq 0 ]; then
                        if [ "$mode" = "sta" ]; then
                            if [ -z "$base_mld_mac_addr" ]; then
                                mld_down_list="$device $iface $mld $mld_down_list"
                            else
                                mld_down_list="$device $iface $mld_down_list"
                            fi
                        else
                            local mld_exists=0
                            for item in $mld_down_list
                            do
                                if [ "$mld" == "$item" ]; then
                                    mld_exists=1
                                    break
                                fi
                            done

                            if [ "$mld_exists" == 0 ]; then
                                mld_down_list="$mld_down_list $mld"
                            fi
                        fi
                    else #MBsta_onboarding
                        #Exclude sta interfaces in the mld_down_list or multi_up list
                        #To not disconnect the BSTA during Onboarding
                        local mld_exists=0
                        for item in $mld_down_list; do
                            if [ "$mld" == "$item" ]; then
                                mld_exists=1
                                break
                            fi
                        done
                        if [ "$mld_exists" == 0 ] && [ "$mode" != "sta" ]; then
                            mld_down_list="$mld_down_list $mld"
                        fi
                    fi
                fi
            fi
        else
            if [ "$MBsta_onboarding" -eq 0 ]; then
                if [ "$device" = "wifi0" ]; then
                    wifi0_restart_list="$wifi0_restart_list $iface"
                elif [ "$device" = "wifi1" ]; then
                    wifi1_restart_list="$wifi1_restart_list $iface"
                elif [ "$device" = "wifi2" ]; then
                    wifi2_restart_list="$wifi2_restart_list $iface"
                elif [ "$device" = "wifi3" ]; then
                    wifi3_restart_list="$wifi3_restart_list $iface"
                elif [ "$device" = "wifi4" ]; then
                    wifi4_restart_list="$wifi4_restart_list $iface"
                else
                    __repacd_wifimon_debug "Invlaid device: $device"
                    return
                fi
            else
                if [ "$device" = "wifi0" ] && [ "$mode" != "sta" ]; then
                    wifi0_restart_list="$wifi0_restart_list $iface"
                elif [ "$device" = "wifi1" ] && [ "$mode" != "sta" ]; then
                    wifi1_restart_list="$wifi1_restart_list $iface"
                elif [ "$device" = "wifi2" ] && [ "$mode" != "sta" ]; then
                    wifi2_restart_list="$wifi2_restart_list $iface"
                elif [ "$device" = "wifi3" ] && [ "$mode" != "sta" ]; then
                    wifi3_restart_list="$wifi3_restart_list $iface"
                elif [ "$device" = "wifi4" ] && [ "$mode" != "sta" ]; then
                    wifi4_restart_list="$wifi4_restart_list $iface"
                else
                    __repacd_wifimon_debug "Invlaid device: $device"
                    return
                fi
            fi
        fi
        uci_set wireless "$config" map_config_changed "0"
        if [ "$onboarding_isdone" -eq 0 ]; then
            last_assoc_state=0
        fi
    else
        if [ "$device" = "wifi0" ]; then
            wifi0_list="$wifi0_list $iface"
        elif [ "$device" = "wifi1" ]; then
            wifi1_list="$wifi1_list $iface"
        elif [ "$device" = "wifi2" ]; then
            wifi2_list="$wifi2_list $iface"
        elif [ "$device" = "wifi3" ]; then
            wifi3_list="$wifi3_list $iface"
        elif [ "$device" = "wifi4" ]; then
            wifi4_list="$wifi4_list $iface"
        else
            __repacd_wifimon_debug "Invlaid device: $device"
            return
        fi
    fi
}

__repacd_map_clean_wireless_config() {
    local config="$1"
    local mld_mapping_changed vap_restart_required new_vap iface

    config_get iface "$config" ifname ''
    config_get mld_mapping_changed "$config" wsplcdMldChanged ''
    config_get vap_restart_required "$config" wsplcdAddtoRestartList ''
    config_get new_vap "$config" wsplcdNewVap ''

    [ -n "$mld_mapping_changed" ] && uci delete wireless."$config.wsplcdMldChanged"
    [ -n "$vap_restart_required" ] && uci delete wireless."$config.wsplcdAddtoRestartList"
    [ -n "$new_vap" ] && uci delete wireless."$config.wsplcdNewVap"
}

__repacd_map_check_onboarding_OPT() {
    local config="$1"
    local iface network disabled device MapBSSType mode map_config_changed enable_fastcloning new_vap
    local last_intf='' vap_restart_required hwmode
    local mld
    local exist_intf=1

    config_get iface "$config" ifname ''
    config_get network "$config" network
    config_get disabled "$config" disabled '0'
    config_get device "$config" device
    config_get hwmode "$device" hwmode
    config_get MapBSSType "$config" MapBSSType '0'
    config_get mode "$config" mode
    config_get map_config_changed "$config" map_config_changed '0'
    config_get mld_mapping_changed "$config" wsplcdMldChanged '0'
    config_get mld "$config" mld ''
    config_get onboarding_isdone MAPConfig 'OnboardingDone' '0'
    config_get enable_fastcloning MAPConfig 'EnableCloningOptimization' '0'
    config_get new_vap "$config" 'wsplcdNewVap' '0'
    # Non-non 11BE VAPs network change is identified based in the uci flag wsplcdAddtoRestartList. libstorage will update this
    config_get vap_restart_required "$config" wsplcdAddtoRestartList '0'
    config_get virtualap "$config" 'virtualAP' '0'

    if [ $((MapBSSType & 0x10)) -eq 16 ]; then
        __repacd_wifimon_debug "Skip Teardown iface $iface"
        return
    fi

    total_vap_count=$((total_vap_count + 1))
    if [ "$mode" = "ap_smart_monitor" ]; then
        smartmonitor_list="$smartmonitor_list $iface"
        return
    fi

    if [ "$virtualap" == "1" ] && [ "$external_controller" == "1" ]; then
        virtualap_list="$virtualap_list $iface"
        return
    fi

    if [ "$mode" = "sta" ]; then
        sta_device=$device
        if [ -n "$iface" ]; then
            __repacd_wifimon_debug "STA iface $iface"
            sta_iface_current=$iface
            sta_mld_current=$mld
            # Reload wifi when new sta config has been received in M2 without
            # skipping restart inorder to re-connect with new credentials
            if [ "$disabled" -eq 0 ] && [ "$map_config_changed" -eq 1 ]; then
                map_config_changed=1
            else
                __repacd_wifimon_debug "Skip STA iface $iface"
                return
            fi
        fi
    fi

    if [ "$disabled" -eq 1 ] && [ "$mode" = "ap" ]; then
        __repacd_wifimon_debug "Bring down $iface"
        return
    fi

    if [ "$new_vap" -eq 1 ]; then
        __repacd_wifimon_debug "Iface: $iface is new a new VAP of device:$device"
        uci_set wireless "$config" wsplcdNewVap "0"
        iface=''
    fi

    if [ -z "$iface" ] && [ "$mode" = "ap" ]; then
        new_vaps_added=1
        exist_intf=0
        __repacd_wifimon_debug "Invalid ifname $iface for device $device"
        local devVal=$(eval echo \$"$device"_config_list)
        last_vap=$(echo $devVal | awk '{print $NF}')
        __repacd_wifimon_debug "last intf $last_vap"

        if [ -z "$last_vap" ]; then
            __repacd_wifimon_debug "last intf not found in restart list"
            local devVal=$(eval echo \$"$device"_list)
            last_vap=$(echo $devVal | awk '{print $NF}')
            __repacd_wifimon_debug "last intf from wifi list $last_vap"
        fi

        local rIdx=$(echo $last_vap | cut -c 4)
        local vapIdx=$(echo $last_vap | cut -c 5)
        vapIdx=$((vapIdx + 1))
        iface=ath$rIdx$vapIdx

        local isSmartMonVap=$(echo $smartmonitor_list | grep -w $iface)
        if [ -n "$isSmartMonVap" ]; then
            __repacd_wifimon_debug "New Intf is smart monitor Vap. Get next Idx"
            vapIdx=$((vapIdx + 1))
            iface=ath$rIdx$vapIdx
        fi

        local isVirtualAp=$(echo $virtualap_list | grep -w $iface)
        if [ -n "$isVirtualAp" ] && [ "$external_controller" == "1" ]; then
            __repacd_wifimon_debug "New Intf is VirtualAp. Get next Idx"
            vapIdx=$((vapIdx + 1))
            iface=ath$rIdx$vapIdx
        fi

       is_sta=$(iw dev $iface info | grep type | awk -F " " '{print $2}' | grep "managed")
       if [ -n "$is_sta" ]; then
           __repacd_wifimon_debug "New Intf is STA Vap. Get next Idx"
           vapIdx=$((vapIdx + 1))
           iface=ath$rIdx$vapIdx
       fi

       if [ "$hwmode" != "11bea" -a "$hwmode" != "11beg" ]; then
          if [ -n "$mld" ]; then
              __repacd_wifimon_debug "mld mapped to non 11BE VAP $iface, clear it" > /dev/console
              mld=''
          fi
       fi
    fi

    __repacd_wifimon_debug "CheckOnboarding: iface:$iface, config_changed:$map_config_changed mld:$mld mld_mapping_changed:$mld_mapping_changed"

    local entryExist=''
    if [ "$mld_mapping_changed" -eq 1 -a -n "$mld" ]; then
        if [ "$exist_intf" -eq 1 ]; then
            entryExist=$(echo $mld_changed_list | grep -w $mld)
            [ -z "$entryExist" ] && mld_changed_list="$mld_changed_list $mld"
        else
            entryExist=$(echo $newly_added_mld_list | grep -w $mld)
            [ -z "$entryExist" ] && newly_added_mld_list="$newly_added_mld_list $mld"
        fi
    fi

    [ "$mld_mapping_changed" -eq "1" ] && uci_set wireless "$config" wsplcdMldChanged "0"

    if [ "$map_config_changed" -eq 1 ]; then
        wifi_reload_req=1
        if [ "$device" = "wifi0" ]; then
            wifi0_config_list="$wifi0_config_list $iface"
        elif [ "$device" = "wifi1" ]; then
            wifi1_config_list="$wifi1_config_list $iface"
        elif [ "$device" = "wifi2" ]; then
            wifi2_config_list="$wifi2_config_list $iface"
        elif [ "$device" = "wifi3" ]; then
            wifi3_config_list="$wifi3_config_list $iface"
        elif [ "$device" = "wifi4" ]; then
            wifi4_config_list="$wifi4_config_list $iface"
        else
            __repacd_wifimon_debug "Invlaid device: $device"
            return
        fi

        [ "$exist_intf" -eq "1" ] && __repacd_update_hostapd_config $config

        if [ -n "$mld" ]; then
            # "mld_iface_maping" will hold the list of mld which needs credential update via hostapd_cli reload_config.
            # Example Format: mld1,wifi0,ath0:mld2,wifi0,ath01:mld1,wifi1,ath1 - This list will be parsed in __repacd_reload_hostapd()
            if [ "$exist_intf" -eq "1" ]; then
                [ -n "$mld_iface_maping" ] && mld_iface_maping="$mld_iface_maping:$mld,$device,$iface"
                [ -z "$mld_iface_maping" ] && mld_iface_maping="$mld,$device,$iface"
            fi
        else
            # "non_mld_iface_list" will hold the list of VAPs which needs credential update via hostapd_cli reload_config.
            # Example Format: ath0,wifi0:ath1,wifi1 - This list will be parsed in __repacd_reload_hostapd()
            if [ "$exist_intf" -eq "1" ]; then
                [ -n "$non_mld_iface_list" ] && non_mld_iface_list="$non_mld_iface_list:$iface,$device"
                [ -z "$non_mld_iface_list" ] && non_mld_iface_list="$iface,$device"
            fi

            if [ "$device" = "wifi0" ]; then
                if [ "$exist_intf" -eq "0" -o "$vap_restart_required" -eq "1" ]; then
                    wifi0_new_vap_list="$wifi0_new_vap_list $iface"
                fi
            elif [ "$device" = "wifi1" ]; then
                if [ "$exist_intf" -eq "0" -o "$vap_restart_required" -eq "1" ]; then
                    wifi1_new_vap_list="$wifi1_new_vap_list $iface"
                fi
            elif [ "$device" = "wifi2" ]; then
                if [ "$exist_intf" -eq "0" -o "$vap_restart_required" -eq "1" ]; then
                    wifi2_new_vap_list="$wifi2_new_vap_list $iface"
                fi
            elif [ "$device" = "wifi3" ]; then
                if [ "$exist_intf" -eq "0" -o "$vap_restart_required" -eq "1" ]; then
                    wifi3_new_vap_list="$wifi3_new_vap_list $iface"
                fi
            elif [ "$device" = "wifi4" ]; then
                if [ "$exist_intf" -eq "0" -o "$vap_restart_required" -eq "1" ]; then
                     wifi4_new_vap_list="$wifi4_new_vap_list $iface"
                fi
            else
                __repacd_wifimon_debug "Invlaid device: $device"
                return
            fi
        fi
        uci_set wireless "$config" map_config_changed "0"
        [ "$vap_restart_required" -eq 1 ] && uci_set wireless "$config" wsplcdAddtoRestartList "0"

        if [ "$onboarding_isdone" -eq 0 ]; then
            last_assoc_state=0
        fi
    else
        if [ "$device" = "wifi0" ]; then
            wifi0_list="$wifi0_list $iface"
        elif [ "$device" = "wifi1" ]; then
            wifi1_list="$wifi1_list $iface"
        elif [ "$device" = "wifi2" ]; then
            wifi2_list="$wifi2_list $iface"
        elif [ "$device" = "wifi3" ]; then
            wifi3_list="$wifi3_list $iface"
        elif [ "$device" = "wifi4" ]; then
            wifi4_list="$wifi4_list $iface"
        else
            __repacd_wifimon_debug "Invlaid device: $device"
            return
        fi
    fi
}

# Enable Radio
#
# input: $1 config: section to update
__repacd_config_get_num_radio() {
    local config="$1"
    local country="$2"
    config_get hwmode "$config" hwmode
    config_get type "$config" type

    if [ "$hwmode" = '11ad' ] && [ "$type" = 'mac80211' ]; then
        return
    fi

    map_num_radio=$((map_num_radio + 1))
}

__repacd_map_delete_sta_idx() {
    local config="$1"
    local mode device network

    config_get mode "$config" mode
    config_get device "$config" device
    config_get network "$config" network
    if [ "$mode" = "sta" ]; then
        sta_device=$device
        sta_nw=$network
        uci delete wireless.$config
    fi
}

__repacd_map_sta_reorder_OPT() {
    local sta_0=$(uci show wireless | grep mode | grep sta | cut -d "[" -f2 | cut -d "]" -f1 | awk 'NR==1{print $1}')
    local sta_1=$(uci show wireless | grep mode | grep sta | cut -d "[" -f2 | cut -d "]" -f1 | awk 'NR==2{print $1}')
    local sta_count=0 count=0 sta_count=0 staIdx delete_index

    if [ -n "$sta_0" ]; then
        uci show wireless | grep "wifi-iface\[$sta_0\]" > /tmp/staConfig_0
        sta_count=$(($sta_count + 1))
    fi

    if [ -n "$sta_1" ]; then
        uci show wireless | grep "wifi-iface\[$sta_1\]" > /tmp/staConfig_1
        sta_count=$(($sta_count + 1))
    fi

    __repacd_wifimon_debug "STA-Reorder:  1stSTAIndex:$sta_0 2ndSTAIndex:$sta_1  sta_iface:$sta_iface"
    __repacd_wifimon_debug "STA-Reorder:  Total vap count $total_vap_count"

    #For 3link MLO below logic will work if "sta_2" varialbe is updated
    while [ "$sta_count" -ne "0" ]; do
        staIdx=$(eval echo \$sta_"$count")
        sta_count=$(($sta_count - 1))
        replace_pos=$(($total_vap_count - $sta_count))

        __repacd_wifimon_debug "STA-Reorder:  staIdx:$staIdx newIdx:$replace_pos count:$count"
        if [ "$staIdx" -ne "$replace_pos" ]; then
            # Delete STA at current Idx
            delete_index=$(($staIdx - $count))
            uci delete wireless.@wifi-iface[$delete_index]
            uci_commit wireless

            # add new entry for moving STA to last idx
            sed -i "s/\[[^]]*\]/[$total_vap_count]/g" /tmp/staConfig_"$count"
            uci add wireless wifi-iface

            # Copy STA config to last Idx
            while IFS= read -r line; do
                eval "uci set $line"
            done < /tmp/staConfig_"$count"
            uci_commit wireless
            rm /tmp/staConfig_"$count"
        fi
        count=$(($count + 1))
    done
}

__repacd_map_sta_reorder() {
    local staIdx=$( uci show wireless | grep mode | grep sta | cut -d "[" -f2 | cut -d "]" -f1)
    uci show wireless | grep "wifi-iface\[$staIdx\]" > /tmp/staConfig
    __repacd_wifimon_debug "sta Idx $staIdx"
    __repacd_wifimon_debug "Total vap count $total_vap_count"

    if [ "$staIdx" -ne "$total_vap_count" ]; then
        # Delete STA at current Idx
        config_load wireless
        config_foreach __repacd_map_delete_sta_idx wifi-iface
        uci_commit wireless

        # add new entry for moving STA to last idx
        sed -i "s/\[[^]]*\]/[$total_vap_count]/g" /tmp/staConfig
        uci add wireless wifi-iface

        # Copy STA config to last Idx
        while IFS= read -r line; do
            eval "uci set $line"
        done < /tmp/staConfig
        uci_commit wireless
        rm /tmp/staConfig
    fi
}

__repacd_map_get_last_mld_index() {
    local config="$1" mld_index=$2
    eval "$2=$(($mld_index + 1))"
}

__repacd_wifimon_is_bsta_iface() {
    local config="$1"
    local sta_ifname="$2"
    local iface mode

    config_get iface "$config" ifname
    config_get mode "$config" mode

    if [ "$mode" = "sta" ] && [ -n "$sta_ifname" ]; then
        if [ "$iface" = "$sta_ifname" ]; then
            eval "$3=$config"
        fi
    fi
}


__repacd_reload_hostapd() {
    local mld_mapping_list="$mld_iface_maping"
    local non_mld_if_list="$non_mld_iface_list"
    local dev mld_name radio interface count=1 last_mld_index=0 ret=''

    config_foreach __repacd_map_get_last_mld_index wifi-mld last_mld_index

    __repacd_wifimon_debug "mld_mapping_list=$mld_mapping_list"
    __repacd_wifimon_debug "last_mld_index:$last_mld_index"

    #11BE VAPs config update using hostapd_cli. NOTE: MLO grouped VAPs should be restarted together
       #mld_mapping_list Format: mld1,wifi0,ath0:mld2,wifi0,ath01:mld1,wifi1,ath1
       #main loop:- loop for each MLD
       #in Loop1 combine different radio VAPs which has same mld's (ie., MLO grouped VAPs) and store it in "mlo_links" as "ath0,wifi0:ath1,wifi1"
       #in Loop2 call hostapd_cli command for all link VAPs avaialble in "mlo_links" as mentioned below
           #hostapd_cli -i ath0 -p /var/run/hostapd-wifi0 disable
           #hostapd_cli -i ath1 -p /var/run/hostapd-wifi1 disable
           #hostapd_cli -i ath0 -p /var/run/hostapd-wifi0 reload_config
           #hostapd_cli -i ath1 -p /var/run/hostapd-wifi1 reload_config
           #hostapd_cli -i ath0 -p /var/run/hostapd-wifi0 enable
           #hostapd_cli -i ath1 -p /var/run/hostapd-wifi1 enable
       #move to next MLD and repeat Loop1 & Loop2
    if [ -n "$mld_mapping_list" ]; then
        local mld_count=0 mlo_links='' mld='' cmd=''
        while [ "$mld_count" -lt "$last_mld_index" ]; do
            mld="mld$mld_count" mlo_links='' count=1

            # skip Bsta MLD from config reload
            config_get mldrole "$mld" role
            if [ "$mldrole" = "Non-AP" ]; then
               mld_count=$((mld_count+1))
               continue
            fi

            dev=$(eval "echo $mld_mapping_list | awk 'BEGIN { FS = \":\" }; { print \$$count }'")
            #Loop1: Find all the link ifaces in the current mld
            while [ -n "$dev" ]; do
               mld_name=$(eval "echo $dev | awk 'BEGIN { FS = \",\" }; { print \$1 }'")
               radio=$(eval "echo $dev | awk 'BEGIN { FS = \",\" }; { print \$2 }'")
               interface=$(eval "echo $dev | awk 'BEGIN { FS = \",\" }; { print \$3 }'")

               if [ "$mld" == "$mld_name" ]; then
                  [ -n "$mlo_links" ] && mlo_links="$mlo_links:$interface,$radio"
                  [ -z "$mlo_links" ] && mlo_links="$interface,$radio"
               fi

               count=$((count+1))
               dev=$(eval "echo $mld_mapping_list | awk 'BEGIN { FS = \":\" }; { print \$$count }'")
            done

            __repacd_wifimon_debug "$mld's LINK:$mlo_links"
            #Loop2: execute the hostapd_cli in below sequence
            for cmd in disable reload_config enable; do
                dev='' radio='' interface='' count=1
                dev=$(eval "echo $mlo_links | awk 'BEGIN { FS = \":\" }; { print \$$count }'")
                while [ -n "$dev" ]; do
                   interface=$(eval "echo $dev | awk 'BEGIN { FS = \",\" }; { print \$1 }'")
                   radio=$(eval "echo $dev | awk 'BEGIN { FS = \",\" }; { print \$2 }'")

                   __repacd_wifimon_debug "$cmd for iface $interface"
                   ret=$(hostapd_cli -i $interface -p /var/run/hostapd-$radio $cmd)
                   if [ "$ret" = "FAIL" ]; then
                       __repacd_wifimon_debug "STAUS: $ret"
                   fi

                   count=$((count+1))
                   dev=$(eval "echo $mlo_links | awk 'BEGIN { FS = \":\" }; { print \$$count }'")
                done
            done

            mld_count=$((mld_count+1))
        done
    fi

    #Non-11BE VAPs config update using hostapd_cli
    dev='' radio='' interface='' count=1
    __repacd_wifimon_debug "NON-MLD_Interface_list: $non_mld_if_list"
    dev=$(eval "echo $non_mld_if_list | awk 'BEGIN { FS = \":\" }; { print \$$count }'")
    while [ -n "$dev" ]; do
        interface=$(eval "echo $dev | awk 'BEGIN { FS = \",\" }; { print \$1 }'")
        radio=$(eval "echo $dev | awk 'BEGIN { FS = \",\" }; { print \$2 }'")
        count=$((count+1))

         # skip Bsta from config reload
        is_staiface=''
        config_foreach __repacd_wifimon_is_bsta_iface wifi-iface "$interface" is_staiface
        if [ -n "$is_staiface" ]; then
            dev=$(eval "echo $non_mld_if_list | awk 'BEGIN { FS = \":\" }; { print \$$count }'")
            continue
        fi

	__repacd_wifimon_debug "$cmd of nonMld iface $interface"
        hostapd_cli -i $interface -p /var/run/hostapd-$radio disable
        hostapd_cli -i $interface -p /var/run/hostapd-$radio reload_config
        hostapd_cli -i $interface -p /var/run/hostapd-$radio enable
        dev=$(eval "echo $non_mld_if_list | awk 'BEGIN { FS = \":\" }; { print \$$count }'")
    done
}

repacd_update_or_add_config() {
    local name=$1 value=$2 file=$3

    if grep -q "$name" "$file"; then
        __repacd_wifimon_debug "Update $name=$value"
        sed -i -e "/^${name}=/ s/=.*/=${value}/" $file
    else
        __repacd_wifimon_debug "add $name=$value"
        echo "$name=$value" >> $file
    fi
}

__repacd_update_hostapd_config() {
    local config="$1" mld_links="" mld_mac="" mld_link_ids="" ctrl_if=""
    local hostapd_conf_file=""
    local iface new_ssid bss_type mode dev

    config_get mode "$config" mode
    config_get iface "$config" ifname
    config_get dev "$config" device

    if [ "$mode" != "ap" ]; then
        __repacd_wifimon_debug "iface $iface is not AP VAP. mode:$mode"
        return;
    fi

    config_get new_ssid "$config" ssid
    config_get bss_type "$config" MapBSSType

    hostapd_conf_file="/var/run/hostapd-$iface.conf"
    __repacd_wifimon_debug "Update hostapd file for iface:$iface file:$hostapd_conf_file ssid:$new_ssid dev:$dev"

    mld_link_macs=$(cat $hostapd_conf_file| grep mld_link_macs | awk -F'[/=]' '{print $2}')
    mld_mac=$(cat $hostapd_conf_file| grep mld_mac_addr  | awk -F'[/=]' '{print $2}')
    mld_link_ids=$(cat $hostapd_conf_file| grep mld_link_ids | awk -F'[/=]' '{print $2}')
    ctrl_if=$(cat $hostapd_conf_file| grep ctrl_interface | awk -F'=' '{print $2}')

    if [ "$bss_type" -eq 32 ]; then
        hostapd_setup_vif "$config" nl80211 no_nconfig bBSS "$bh_ssid_param" "$bh_key_param"
    else
        hostapd_setup_vif "$config" nl80211 no_nconfig
    fi

    [ -n "$mld_link_macs" ] && repacd_update_or_add_config "mld_link_macs" "$mld_link_macs" "$hostapd_conf_file"
    [ -n "$mld_mac" ] && repacd_update_or_add_config "mld_mac_addr" "$mld_mac" "$hostapd_conf_file"
    [ -n "$mld_link_ids" ] && repacd_update_or_add_config "mld_link_ids" "$mld_link_ids" "$hostapd_conf_file"
    [ -n "$ctrl_if" ] && sed -i -e "/^ctrl_interface=/ s/=.*/=\/var\/run\/hostapd-$dev/" $hostapd_conf_file
}

__repacd_map_get_BH_ssid_key() {
    local config="$1"
    local disabled MapBSSType mode ssid key

    config_get disabled "$config" disabled '0'
    config_get MapBSSType "$config" MapBSSType '0'
    config_get mode "$config" mode

    [ "$disabled" -eq 1 ] && return
    [ "$mode" != "ap" ] && return

    local bbss=$(( MapBSSType & 64 ))
    if [ ${bbss} -eq 64 ]; then
        config_get bh_ssid_param "$config" ssid
        config_get bh_key_param "$config" key
        return
    fi
}

__repacd_map_get_unused_mld() {
    local config="$1"
    local mld_used='' mld_exist=''

    mld_used=$(uci show | grep wifi-iface | grep $config)

    if [ -z "$mld_used" ]; then
        mld_exist=$(echo $mld_down_list | grep -w $config)
        [ -z "$mld_exist" ] && mld_down_list="$mld_down_list $config"
        unused_mld=1
        [ -z "$mld_exist" ] && __repacd_wifimon_debug "Unused MLD $config adding it to down list"
    fi
}

repacd_wifimon_check_onboarding() {
    local radio_count=0 restart_list='' wifi_list=''
    local use_single_multiup
    local map_ts_active

    config_load repacd
    config_get_bool restartWifi MAPConfig 'restartWifi' '0'
    config_get fastOnboarding MAPConfig 'MapFastOnboarding'
    config_get map_ts_active MAPConfig 'MapTrafficSeparationActive' '0'
    config_get map_backhaul_nw MAPConfig 'VlanNetworkBackHaul'
    config_get map_primary_nw MAPConfig 'VlanNetworkPrimary'
    config_get use_single_line_multi_up MAPConfig 'MapUseSingleMultiUp'
    config_get map_version MAPConfig 'MapVersionEnabled'
    config_get_bool map_fast_onboarding MAPConfig 'MapFastOnboarding' '0'
    local hyctl_portType
    if [ "$MBsta_onboarding" -eq 0 ]; then
        _mlo_connection_monitor
    fi
    __repacd_wifimon_dump "Onboarding $fastOnboarding onboarding_done $onboarding_done sta_iface $sta_iface sta_iface_backup $sta_iface_backup"
    if [ "$onboarding_done" -eq 1 ] || __repacd_map_sta_connected_legacy_pf; then
        hyctl_portType=$(hyctl show | grep Unknown)
        if [ -n "$hyctl_portType" ]; then
            if [ "$fastOnboarding" -eq 1 ]; then
                # Delete temp file to read config again
                [ -f /tmp/mapTempIntfList ] && rm /tmp/mapTempIntfList
                [ -f /tmp/mapTempIntfListWsplcd ] && rm /tmp/mapTempIntfListWsplcd
            fi

            if __repacd_map_sta_connected_legacy_pf; then
                __repacd_wifimon_debug "Unknown Port Type BH . Restart ezmesh to resolve"
                /etc/init.d/hyfi-bridging start
                /etc/init.d/$MAP restart
            fi
        fi
    fi
    teardown_done=0
    config_load wireless
    config_foreach __repacd_map_teardown_vap wifi-iface
    if [ "$teardown_done" -eq 1 ]; then
        __repacd_wifimon_debug "teardown : $teardown_done"
        # Delete temp file to read config again
        [ -f /tmp/mapTempIntfList ] && rm /tmp/mapTempIntfList
        [ -f /tmp/mapTempIntfListWsplcd ] && rm /tmp/mapTempIntfListWsplcd
        /etc/init.d/$MAP stop
        /etc/init.d/wsplcd stop
    fi

    if [ "$fastOnboarding" -eq 1 ] && [ "$restartWifi" -eq 1 ]; then
        new_vaps_added=0
        unused_mld=0
        wifi_reload_req=0
        total_vap_count=-1
        sta_iface_current=''
        sta_nw=''
        uci set repacd.MAPConfig.OnboardingDone='0'
        uci commit repacd

        __repacd_wifimon_debug "num radio : $map_num_radio"
        for r in wifi0 wifi1 wifi2 wifi3 wifi4; do
            if [ "$map_num_radio" -eq "$radio_count" ]; then
                break
            fi
            eval $r_restart_list=$r
            eval $r_list=$r
            radio_count=$((radio_count + 1))
        done

        smartmonitor_list=''
        config_load wireless
        config_foreach __repacd_map_check_onboarding wifi-iface
        uci_commit wireless

        teardown_list=''
        config_load wireless
        config_foreach __repacd_map_teardown_list wifi-iface

        if [ "$MBsta_onboarding" -eq 0 ]; then
            __repacd_map_sta_reorder
        fi

        #Some MLDs will be not attached to link after MLO/SLO regrouping. Those are identified and added to mld_down_list
        config_foreach __repacd_map_get_unused_mld wifi-mld
    fi

    if [ "$restartWifi" -eq 1 ]; then
        # Delete temp file to read config again
        [ -f /tmp/mapTempIntfList ] && rm /tmp/mapTempIntfList
        [ -f /tmp/mapTempIntfListWsplcd ] && rm /tmp/mapTempIntfListWsplcd

        __repacd_wifimon_debug "wifi reload req: $wifi_reload_req"
        __repacd_wifimon_debug "new vaps added: $new_vaps_added"
        if [ "$wifi_reload_req" -eq 1 ]; then
            __repacd_wifimon_debug "wifi0 list : $wifi0_list"
            __repacd_wifimon_debug "wifi1 list : $wifi1_list"
            __repacd_wifimon_debug "wifi2 list : $wifi2_list"
            __repacd_wifimon_debug "wifi3 list : $wifi3_list"
            __repacd_wifimon_debug "wifi4 list : $wifi4_list"
            __repacd_wifimon_debug "wifi0 restart list : $wifi0_restart_list"
            __repacd_wifimon_debug "wifi1 restart list : $wifi1_restart_list"
            __repacd_wifimon_debug "wifi2 restart list : $wifi2_restart_list"
            __repacd_wifimon_debug "wifi3 restart list : $wifi3_restart_list"
            __repacd_wifimon_debug "wifi4 restart list : $wifi4_restart_list"
            __repacd_wifimon_debug "mld list : $mld_list"
            __repacd_wifimon_debug "teardown : $teardown_list"
            __repacd_wifimon_debug "sta radio : $sta_device"
            __repacd_wifimon_debug "MLD VAP Interface list: $mld_vap_device"
            config_load wireless
            config_foreach __repacd_wifimon_set_root_distance wifi-iface
            local restart_list=$(eval echo \$"$sta_device"_restart_list)
            restart_list=$(echo $restart_list | \
                               awk '{ for (i=NF; i>1; i--) printf("%s ",$i); print $1; }')
            __repacd_wifimon_debug "Restart sta device $sta_device $restart_list skip_sta_restart:$skip_sta_restart"

            if [ -n "$restart_list" ]; then
                if [ "$skip_sta_restart" -eq 0 ]; then
                    wifi_list="wifi multi_up $sta_device $restart_list"
                    if [ "$new_vaps_added" -eq 1 ] && [ "$use_single_line_multi_up" -eq 0 ]; then
                        $wifi_list
                    fi
                else
                    wifi_list="wifi multi_up"
                fi
                eval "$sta_device"_list=''
            fi

            if [ -z "$wifi_list" ]; then
                wifi_list="wifi multi_up"
            fi

            for r in wifi0 wifi1 wifi2 wifi3 wifi4; do
                if [ "$sta_device" = "$r" ] && [ "$skip_sta_restart" -eq 0 ]; then
                    continue
                fi

                local restart_list=$(eval echo \$"$r"_restart_list)
                if [ -n "$restart_list" ]; then
                    __repacd_wifimon_debug "$r $restart_list"
                    if [ "$new_vaps_added" -eq 1 ]  && [ "$use_single_line_multi_up" -eq 0 ]; then
                        wifi multi_up $r $restart_list
                    fi
                    wifi_list="$wifi_list $r $restart_list"
                fi
            done

            if [ -n "$mld_list" ]; then
                wifi_list="$wifi_list $mld_list"
            fi

            if [ -n "$mld_down_list" ] && [ "$new_vaps_added" -eq 1 -o "$unused_mld" -eq 1 ]; then
                __repacd_wifimon_debug "wifi multi_down list: $mld_down_list"
                wifi multi_down $mld_down_list
            fi

            __repacd_wifimon_debug "Restart wifi_list: $wifi_list"
            if [ "$new_vaps_added" -eq 0 ] || [ "$use_single_line_multi_up" -eq 1 ]; then
                $wifi_list

            fi
        fi

        wifi0_restart_list='' wifi1_restart_list='' wifi2_restart_list=''
        wifi3_restart_list='' wifi4_restart_list=''
        wifi0_config_list='' wifi1_config_list='' wifi2_config_list=''
        wifi3_config_list='' wifi4_config_list=''
        mld_down_list=''
        wifi0_list='' wifi1_list='' wifi2_list=''
        wifi3_list='' wifi4_list=''
        mld_list=''
        uci set repacd.MAPConfig.restartWifi='0'
        uci set repacd.MAPConfig.OnboardingDone='1'
        uci set repacd.MAPConfig.SkipStaRestart='1'
        uci commit repacd

        /etc/init.d/hyfi-bridging stop
        /etc/init.d/$MAP stop
        /etc/init.d/wsplcd stop

        if [ "$map_fast_onboarding" -eq 1 ]; then
            __repacd_wifimon_debug "current STA $sta_iface_current"
            if [ "$skip_sta_restart" -eq 0 ]; then
                __repacd_map_remove_sta_vlan $sta_iface_current
            fi

            # Resolve the STA interfaces.
            __repacd_wifimon_debug "STA network $sta_nw"
            __repacd_wifimon_get_sta_info "$sta_nw"
        fi
    fi
}

__repacd_map_restart_disabled_vaps() {

    local config="$1"
    local iface device disabled mode MapBSSType hidden
    config_get iface "$config" ifname
    config_get device "$config" device
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get MapBSSType "$config" MapBSSType '0'
    hidden=$(uci get wireless.$config.hidden 2>/dev/null)

    if [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" = "ap" ] && [ "$hidden" != "1" ]; then
        if [ "$maplite_enabled" -ne 1 ]; then
            local bitRate=$(repacdcli $iface get_bitrate)
            if [ "$bitRate" -eq 0 -o -z "$bitRate" ]; then
                local isAcsInProgress=$(cfg80211tool $iface get_acs_state)
                isAcsInProgress=${isAcsInProgress#*:}
                # bitRate will be 0 even when CAC is in progress. check cac state before we do
                # interface enable/disable
                local isCacInProgress=$(cfg80211tool $iface get_cac_state)
                isCacInProgress=${isCacInProgress#*:}
                if [ "$isCacInProgress" -eq 0 ] && [ "$isAcsInProgress" -eq 0 ]; then
                    if [ $((MapBSSType & 0x10)) -eq 16 ]; then
                        __repacd_map_vlanmon_debug "Iface $iface is teardown vap. Don't bringup"
                    else
                        for cmd in disable  enable; do
                            __repacd_wifimon_debug "hostapd_cli -i $iface -p /var/run/hostapd-$device $cmd"
                            hostapd_cli -i $iface -p /var/run/hostapd-$device $cmd
                        done
                    fi
                fi
            fi
        fi
    fi

}

repacd_wifimon_check_onboarding_OPT() {
    local radio_count=0 restart_list=''
    local use_single_multiup
    local map_ts_active

    config_load repacd
    config_get_bool restartWifi MAPConfig 'restartWifi' '0'
    config_get fastOnboarding MAPConfig 'MapFastOnboarding'
    config_get map_ts_active MAPConfig 'MapTrafficSeparationActive' '0'
    config_get map_backhaul_nw MAPConfig 'VlanNetworkBackHaul'
    config_get map_primary_nw MAPConfig 'VlanNetworkPrimary'
    config_get use_single_line_multi_up MAPConfig 'MapUseSingleMultiUp'
    config_get map_version MAPConfig 'MapVersionEnabled'
    config_get_bool map_fast_onboarding MAPConfig 'MapFastOnboarding' '0'
    config_get enable_fastcloning MAPConfig 'EnableCloningOptimization' '0'
    local hyctl_portType

    __repacd_wifimon_dump " fastOnboarding $fastOnboarding restartWifi $restartWifi onboarding_done $onboarding_done sta_iface $sta_iface sta_iface_backup $sta_iface_backup EnableCloningOptimization:$enable_fastcloning"
    if [ "$map_version" -eq 1 ]; then
        if [ "$onboarding_done" -eq 1 ] || __repacd_map_sta_connected_legacy_pf; then
            hyctl_portType=$(hyctl show | grep Unknown)
            if [ -n "$hyctl_portType" ]; then
                if [ "$fastOnboarding" -eq 1 ]; then
                    # Delete temp file to read config again
                    [ -f /tmp/mapTempIntfList ] && rm /tmp/mapTempIntfList
                    [ -f /tmp/mapTempIntfListWsplcd ] && rm /tmp/mapTempIntfListWsplcd
                fi

                if __repacd_map_sta_connected_legacy_pf; then
                    __repacd_map_vlanmon_debug "Unknown Port Type BH . Restart ezmesh to resolve"
                    /etc/init.d/hyfi-bridging start
                    /etc/init.d/$MAP restart
                fi
            fi
        fi
    fi

    teardown_done=0
    #TODO: find teardown radio and delete them only if restartWifi is 0

    if [ "$fastOnboarding" -eq 1 ] && [ "$restartWifi" -eq 1 ]; then
        new_vaps_added=0
        wifi_reload_req=0
        total_vap_count=-1
        sta_iface_current=''
        sta_mld_current=''
        sta_nw=''
        uci set repacd.MAPConfig.OnboardingDone='0'
        uci commit repacd

        __repacd_wifimon_debug "num radio : $map_num_radio"
        for r in wifi0 wifi1 wifi2 wifi3 wifi4; do
            if [ "$map_num_radio" -eq "$radio_count" ]; then
                break
            fi
            eval $r_restart_list=$r
            eval $r_list=$r
            radio_count=$((radio_count + 1))
        done

        config_load wireless
        config_foreach __repacd_map_get_BH_ssid_key wifi-iface

        smartmonitor_list=''
        config_foreach __repacd_map_check_onboarding_OPT wifi-iface
        uci_commit wireless

        teardown_list=''
        config_load wireless

        config_foreach __repacd_map_teardown_vap_OPT wifi-iface
        uci_commit wireless

        config_foreach __repacd_map_mld_remove_list wifi-mld
        uci_commit wireless

        [ -n "$teardown_radio" ] && wifi_reload_req=1
        if [ "$MBsta_onboarding" -eq 0 ]; then
            __repacd_map_sta_reorder_OPT
        fi
    fi

    if [ "$restartWifi" -eq 1 ]; then
        # Delete temp file to read config again
        [ -f /tmp/mapTempIntfList ] && rm /tmp/mapTempIntfList
        [ -f /tmp/mapTempIntfListWsplcd ] && rm /tmp/mapTempIntfListWsplcd

        __repacd_wifimon_debug "wifi reload req: $wifi_reload_req"
        __repacd_wifimon_debug "new vaps added: $new_vaps_added"
        if [ "$wifi_reload_req" -eq 1 ]; then
            __repacd_wifimon_debug "wifi0 list : $wifi0_list"
            __repacd_wifimon_debug "wifi1 list : $wifi1_list"
            __repacd_wifimon_debug "wifi2 list : $wifi2_list"
            __repacd_wifimon_debug "wifi3 list : $wifi3_list"
            __repacd_wifimon_debug "wifi4 list : $wifi4_list"
            __repacd_wifimon_debug "wifi0 newly add VAP list : $wifi0_new_vap_list"
            __repacd_wifimon_debug "wifi1 newly add VAP list : $wifi1_new_vap_list"
            __repacd_wifimon_debug "wifi2 newly add VAP list : $wifi2_new_vap_list"
            __repacd_wifimon_debug "wifi3 newly add VAP list : $wifi3_new_vap_list"
            __repacd_wifimon_debug "wifi4 newly add VAP list : $wifi4_new_vap_list"

            __repacd_wifimon_debug " "
            __repacd_wifimon_debug "non_mld_iface_maping : $non_mld_iface_list"
            __repacd_wifimon_debug "mld_iface_maping : $mld_iface_maping"
            __repacd_wifimon_debug "mld_changed_list : $mld_changed_list"
            __repacd_wifimon_debug "newly_added_mld_list : $newly_added_mld_list"

            __repacd_wifimon_debug " "
            __repacd_wifimon_debug "teardown_radio : $teardown_radio ( teardown_mld:$teardown_mld )"
            __repacd_wifimon_debug "STA radio/iface/mld : $sta_device - $sta_iface_current - $sta_mld_current"

            #update hostapd config file with new credentials
            __repacd_reload_hostapd

            config_load wireless
            config_foreach __repacd_wifimon_set_root_distance wifi-iface
            local restart_list=$(eval echo \$"$sta_device"_new_vap_list)
            restart_list=$(echo $restart_list | \
                               awk '{ for (i=NF; i>1; i--) printf("%s ",$i); print $1; }')
            __repacd_wifimon_debug "Restart sta device $sta_device $restart_list"

            #Legacy
            if [ -n "$restart_list" ]; then
                if [ "$new_vaps_added" -eq 1 ]; then
                    opt_wifi_list="wifi multi_up $sta_device $restart_list $sta_iface_current"
                fi
                eval "$sta_device"_list=''
            fi

            if [ -z "$opt_wifi_list" ]; then
                opt_wifi_list="wifi multi_up"
            fi

            for r in wifi0 wifi1 wifi2 wifi3 wifi4; do
                if [ "$sta_device" = "$r" -a "$new_vaps_added" -eq 1 ]; then
                    continue
                fi

                if [ -n "$(echo $teardown_radio | grep -w $r)" ]; then
                     __repacd_wifimon_debug "skip $r as complete radio needs restart" > /dev/console
                    continue;
                fi

                local restart_list=$(eval echo \$"$r"_new_vap_list)
                if [ -n "$restart_list" ]; then
                    __repacd_wifimon_debug "$r $restart_list"
                    opt_wifi_list="$opt_wifi_list $r $restart_list"
                fi
            done

            #MLO
            local up_list='' down_list=''
            if [ -n "$mld_changed_list" -o "$new_vaps_added" -eq 1 ]; then
                if [ -n "$mld_changed_list" -o -n "$newly_added_mld_list" ]; then
                    up_list=$(echo "$mld_changed_list $newly_added_mld_list" | xargs -n1 | cut -b 4- | sort -n | sed -e 's/^/mld/' | xargs)
                    __repacd_update_multiup_mld_args "$up_list"
                fi
            fi

            if [ -n "$mld_changed_list" -o -n "$teardown_mld" ]; then
               down_list=$(echo "$mld_changed_list $teardown_mld" | xargs -n1 | cut -b 4- | sort -n | sed -e 's/^/mld/' | xargs)
               # When a VAP is deleted its corresponding mld should brought down
               __repacd_wifimon_debug "Exec: wifi multi_down $down_list"
               wifi multi_down $down_list
            fi

            if [ "$new_vaps_added" -eq 1 ]; then
                 __repacd_wifimon_debug "new_vaps_added:$new_vaps_added"
                if [ -n "$sta_mld_current" ]; then
                    __repacd_update_multiup_mld_args "$sta_mld_current"
                fi
            fi

            config_foreach __repacd_map_clean_wireless_config wifi-iface
            uci_commit wireless

            if [ -n "$teardown_radio" ]; then
                __repacd_wifimon_debug " Exec: /sbin/wifi"
                primary_mldbsta_iface=''
                /sbin/wifi
            fi

            if [ "$new_vaps_added" -eq 1 -o -n "$mld_changed_list" ] && [ -z "$teardown_radio" ]; then
                __repacd_wifimon_debug "Exec: $opt_wifi_list"
                primary_mldbsta_iface=''
                $opt_wifi_list
            fi
        fi

        # Fail proof check to bring up any vaps which might have gone down.
        config_foreach __repacd_map_restart_disabled_vaps wifi-iface

        # Reset 5G attempt counter when cloning happened in 2G
        # Avoids bsta settling in 2G when 5G attempts are maxed out
        if [ -n "$sta_iface_24g" ]; then
            cnt_5g_attempts=0
            uci_set repacd MAPWiFiLink '5gAttemptsCount' "$cnt_5g_attempts"
            uci_commit repacd
        fi

        wifi0_config_list='' wifi1_config_list='' wifi2_config_list=''
        wifi3_config_list='' wifi4_config_list=''
        wifi0_list='' wifi1_list='' wifi2_list=''
        wifi3_list='' wifi4_list=''
        mld_iface_maping='' non_mld_iface_list=''
        mld_mapping_list='' mld_changed_list=''
        wifi0_new_vap_list='' wifi1_new_vap_list='' wifi2_new_vap_list=''
        wifi3_new_vap_list='' wifi4_new_vap_list='' newly_added_mld_list=''
        opt_wifi_list=''
        uci set repacd.MAPConfig.restartWifi='0'
        uci set repacd.MAPConfig.OnboardingDone='1'
        uci set repacd.MAPConfig.SkipStaRestart='1'
        uci commit repacd

        /etc/init.d/hyfi-bridging stop
        /etc/init.d/$MAP stop
        /etc/init.d/wsplcd stop

        if [ "$map_fast_onboarding" -eq 1 ]; then
            __repacd_wifimon_debug "current STA $sta_iface_current"
            if [ "$new_vaps_added" -eq 1 -o -n "$teardown_radio" ]; then
                __repacd_map_remove_sta_vlan $sta_iface_current
            fi

            # Resolve the STA interfaces.
            __repacd_wifimon_debug "STA network $sta_nw"
            __repacd_wifimon_get_sta_info "$sta_nw"
        fi
        teardown_radio=''
        teardown_mld=''
    fi
}
