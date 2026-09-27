#!/bin/sh
# Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

MAP_VLANMON_DEBUG_OUTOUT=0
NETWORK_TYPE_LAN=""
NETWORK_TYPE_GUEST1=""
NETWORK_TYPE_GUEST2=""
NETWORK_TYPE_GUEST3=""
NETWORK_TYPE_BACKHAUL=""
MAP_GET_STA_VLAN_IOCTL="get_map_sta_vlan"
num_guest_vlan=0
vid_lan=0 vid_guest1=0 vid_guest2=0 vid_guest3=0
sta_vid=0 map_primary_bsta_vid=0
vid_8021q=0
MAP_IS_GW_REACHABLE=0
upstream_version=0
map_my_version=0
map_ts_active=0
map_ts_apply=0
map_ts_remove=0
maplite_enabled=0
eth_iface="" sta_iface=""
map_bsta_backhaul=0
map_fh_bh_vap_up=1
map_onboarding_done=0
map_gw_reachable_confirm_attempts=1
map_enable_vlan_logs=0
map_single_r1r2_bh=0
config_changed=0
sta_config_changed=0
maplite_restart_config=1
deviceMode=""
restart_required=0
fastOnboarding=0
OnboardingDone=0
mlobsta_list=''
enable_mlo=0
enable_slo=0
enable_single_netdev=0
external_controller=0
secondary_sta_iface=''

log_level="INFO"

CACstate=0

config_load 'repacd'
config_get_bool ezmesh repacd 'Ezmesh' '0'
config_get log_level MAPConfig 'MapLogLevel'

    if [ "$ezmesh" -eq 1 ]; then
        MAP='ezmesh'
    else
        MAP='hyd'
    fi

. /lib/functions/hyfi-iface.sh

# Emit a message at debug level.
# input: $1 - the message to log
__repacd_map_vlanmon_debug() {
    if [ "$map_enable_vlan_logs" -eq 0 ]; then
        return
    fi

    local stderr=''
    if [ "$MAP_VLANMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
        echo "repacd (vlanmon): $*" > /dev/console
    fi

    logger $stderr -t repacd.mapvlanmon -p user.debug "$1"
}

# Emit a message at dump level.
# input: $1 - the message to log
__repacd_map_vlanmon_dump() {
    if [ "$map_enable_vlan_logs" -eq 0 -o "$log_level" != "DUMP" ]; then
        return
    fi

    local stderr=''
    if [ "$MAP_VLANMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
        echo "repacd (vlanmon): $*" > /dev/console
    fi

    logger $stderr -t repacd.mapvlanmon -p user.debug "$1"
}

# Determine if the network is configured or not.
#
# input: $1 network name
# return: 0 if network exist; otherwise non-zero
__repacd_network_exist() {
    local lan_name=$1
    local no_network

    no_network=$(uci show "network.$lan_name" 2>&1 | grep 'Entry not found')
    [ -n "$no_network" ] && return 1

    return 0
}

# Add the given interface to the given network.
# input: $1 network name
# input: $2 interface name
__repacd_add_interface() {
    local name=$1 new_if="$2"
    local if_name iface_name iface_vid

    # ubus network reload might miss adding vlan config
    # Add this check to remove and add back interface in traffic
    # separation is active and apply is set to 1

    if [ "$map_ts_active" -eq 1 -a "$map_ts_apply" -eq 1 ]; then
        __repacd_delete_interface $name $new_if
    fi
    if __repacd_network_exist "$name"; then
        if [ -n "$new_if" ]; then
            if [ "$openwrt_version_check" -eq 1 ]; then
                config_load network
                config_foreach hyfi_network_add_device_port device "$name" "$new_if"
            else
                if_name=$(uci get "network.$name.ifname")
                if [ -n "$if_name" ]; then
                    if_name="$if_name $new_if"
                else
                    if_name="$new_if"
                fi
                if_name=$(echo "$if_name" | xargs -n1 | sort -u | xargs)
                uci_set network "$name" ifname "$if_name"
            fi
            if [ "$maplite_enabled" -eq 1 ]; then
                iface_name=$(echo $new_if | cut -d '.' -f1 | awk '{$1=$1};1')
                iface_vid=$(echo $new_if | cut -d '.' -f2 | awk '{$1=$1};1')
                if [ -n "$iface_vid" ] && [[ "$iface_vid" != "$if_name" ]]; then
                    __repacd_map_vlanmon_debug "network $name $iface_name $iface_vid $new_if"
                    vconfig add "$iface_name" "$iface_vid"
                    __repacd_map_vlanmon_debug "Bringing up new_if: $new_if"
                    ifconfig "$new_if" up
                    brctl addif "br-$name" "$new_if"
                fi
            fi
            uci_commit network
        fi
    fi

    if [ "$maplite_enabled" -ne 1 ]; then
        ubus call network reload
    fi
}

# Delete the given interface from the given network.
# input: $1 network name
# input: $2 interface name
__repacd_delete_interface() {
    local name=$1 interface="$2"
    local if_name
    local new_if=' '

    if __repacd_network_exist "$name"; then
        if [ -n "$interface" ]; then
            if [ "$openwrt_version_check" -eq 1 ]; then
                config_load network
                config_foreach hyfi_network_delete_device_port device $name $interface
            else
                if_name=$(uci get "network.$name.ifname")
                uci_set network "$name" ifname ' '
                for iface in $if_name; do
                    if [ "$interface" != "$iface" ]; then
                        if [ -n "$new_if" ]; then
                            new_if="$new_if $iface"
                        else
                            new_if="$iface"
                        fi
                    fi
                done
                uci_set network "$name" ifname "$new_if"
            fi
        fi
    fi

    uci_commit network
    ubus call network reload
}

# Set egress and ingress priority map per VLAN interface
__repacd_map_set_egress_ingress_per_intf() {
    local ifname="$1"
    local vlan_id="$2"

    vconfig set_egress_map "$ifname.$vlan_id" 0 0
    vconfig set_egress_map "$ifname.$vlan_id" 1 1
    vconfig set_egress_map "$ifname.$vlan_id" 2 2
    vconfig set_egress_map "$ifname.$vlan_id" 3 3
    vconfig set_egress_map "$ifname.$vlan_id" 4 4
    vconfig set_egress_map "$ifname.$vlan_id" 5 5
    vconfig set_egress_map "$ifname.$vlan_id" 6 6
    vconfig set_egress_map "$ifname.$vlan_id" 7 7
    vconfig set_ingress_map "$ifname.$vlan_id" 0 0
    vconfig set_ingress_map "$ifname.$vlan_id" 1 1
    vconfig set_ingress_map "$ifname.$vlan_id" 2 2
    vconfig set_ingress_map "$ifname.$vlan_id" 3 3
    vconfig set_ingress_map "$ifname.$vlan_id" 4 4
    vconfig set_ingress_map "$ifname.$vlan_id" 5 5
    vconfig set_ingress_map "$ifname.$vlan_id" 6 6
    vconfig set_ingress_map "$ifname.$vlan_id" 7 7
}

# Create necessary VLAN interfaces for the backhaul vaps and add the
# created VLAN interfaces to the given network.
# VLAN interfaces are created by concatenating interface name and vlan id.
# input: $1 network name
# input: $2 iface name
# input: $3 VLAN id
__repacd_add_vlan_interfaces() {
    local network=$1
    local ifname=$2
    local id=$3
    local add_vlan=0
    local vlan_ifname vlan_ifname_brctl vlan_ifname_nw
    local iface_nw_list iface_name iface_vid
    local iface_nw

    local interface="$(iw dev $iface info | grep ssid | awk -F " " '{print $2}')"

    if [ -z "$network" ] || [ -z "$ifname" ]; then
        __repacd_map_vlanmon_debug "Invalid network $network iface $ifname"
    fi

    # if current vlan is different from previous delete older vlan config
    iface_nw_list="$(hyfi_network_get_ifnames $network)"

    for iface_nw in $iface_nw_list; do
        case $iface_nw in
            *.*)
                iface_name=${iface_nw%.*}
                iface_vid=${iface_nw#*.}

                if [ "$iface_name" = "$ifname" ]; then
                    if [ "$iface_vid" -eq "$id" ] || [ -n "$interface" ]; then
                    #Check if interface is connected cuz in MLO only one sta will have vid
                    #greater than 0.
                        continue
                    else
                        __repacd_map_vlanmon_debug "removing vlan configured iface $iface_nw"
                        __repacd_delete_interface $network $iface_nw

                        # Check if VLAN interface is part of bridge
                        vlan_ifname_brctl=$(brctl show br-"$network" | grep -w "$iface_nw" | awk '{print $1}')
                        if [ -z "$vlan_ifname_brctl" ]; then
                            __repacd_map_vlanmon_debug "Delete interface $iface_nw from network config"
                            __repacd_delete_interface $network $iface_nw
                        fi
                    fi
                fi
                ;;
        esac
    done

    # if VLAN ID is 0 return
    if [ "$id" -eq 0 ]; then
        __repacd_map_vlanmon_debug "VLAN ID is 0 for network $network iface $ifname"
        return
    fi
    if [ "$id" -gt 0 -a "$MBsta_onboarding" -eq 1 ]; then
        #If vid is non zero & interface
        #not connected we should not add vlan
        #for that interface for multi bsta
        #as that will cause a loop if partner
        #link is connected & TS got disabled
        if [ -z "$interface" ] && [ "$mode" = "sta" ]; then
            __repacd_map_vlanmon_dump "vid:$id but Not Adding vlan for interface $iface since its disconnected"
            return
        fi
    fi

    case $ifname in
        eth*.*)
           __repacd_map_vlanmon_debug "Invalid interface name $ifname. skip vlan add" > /dev/console
           return
    esac

    # Check if network is part of bridge
    if [ ! -e "/sys/class/net/br-"$network"" ]; then
        __repacd_map_vlanmon_debug "br-$network not part of bridge"
        return
    fi

    # Check if VLAN interface is already created
    if [ ! -e "/sys/class/net/"$ifname.$id"" ]; then
        __repacd_map_vlanmon_debug "VLAN for Interface $ifname and $id not created"
        add_vlan=1
    fi

    # Check if VLAN interface is part of bridge
    if [ ! -e "/sys/class/net/br-"$network"/brif/"$ifname.$id"" ]; then
        __repacd_map_vlanmon_debug "VLAN for Interface $ifname and $id not part of bridge"
        add_vlan=1
    fi

    # Check if VLAN interface is part of network
    vlan_ifname_nw=$(echo "$iface_nw_list" | grep -w "$ifname.$id")

    if [ -z "$vlan_ifname_nw" ]; then
        __repacd_map_vlanmon_debug "VLAN for Interface $ifname and $id not part of $network"
        add_vlan=1
    fi

    # Apply VLAN
    if [ "$add_vlan" -eq 1 ]; then
        __repacd_map_vlanmon_debug "VLAN not set. Apply Vlan $ifname $id br-$network"
        # Delete temp file to read config again
        [ -f /tmp/mapTempIntfList ] && rm /tmp/mapTempIntfList
        [ -f /tmp/mapTempIntfListWsplcd ] && rm /tmp/mapTempIntfListWsplcd

        map_ts_apply=1
        map_ts_remove=0
        config_changed=1
        __repacd_add_interface "$network" "$ifname.$id"

        # Set the traffic Separattion Active flag while applying vlan
        config_load repacd
        uci_set repacd MAPConfig MapTrafficSeparationActive '1'
        uci_commit repacd

        # Check if SP is enabled
        config_load $MAP
        config_get sp_enabled MAPSPSettings 'EnableSP' '0'

        if [ "$sp_enabled" -eq 1 ]; then
            __repacd_map_set_egress_ingress_per_intf $ifname $id
            # Set priority maps for sta vap with primary VLAN ID.
            # Because sta vap with primary VLAN ID is created
            # when assoc response is recieved, after wifi restart
            # during onboarding, the priority map is changed to default.
            # For this reason, this VAP is specially handled.
            if [ "$map_bsta_backhaul" -eq 1 ]; then
                pm_already_set=$(grep EGRESS /proc/net/vlan/"$sta_iface"."$map_primary_bsta_vid" | awk '{print $4}')
                if [ -z "$pm_already_set" ]; then
                    __repacd_map_set_egress_ingress_per_intf $sta_iface $map_primary_bsta_vid
                fi
            fi
        fi

    elif [ "$add_vlan" -eq 0 ]; then
        __repacd_map_vlanmon_dump " $id already applied on $ifname for $network"
        map_ts_active=1
        map_ts_apply=0
        map_ts_remove=0
    fi

    # Set Switch Config for ETH Interface for secondary VLANs
    if [ "$add_vlan" -eq 1 -a "$vid_lan" -ne "$id" ]; then
        __repacd_map_vlanmon_debug "Add VLAN $id for ethernet guest network support"
        swconfig dev switch0 vlan $id set ports "0t 1t 2t 3t 4t"
        swconfig dev switch0 vlan $id set ports "0t 1t 2t 3t 4t"
        swconfig dev switch0 set apply
    fi
}

# Create necessary VLAN interfaces for guest networks if they are valid
# input: $1 ifaceBH name
__repacd_add_guest_network_vlan_interfaces() {
    local ifaceBH="$1"

    if [ -n "$NETWORK_TYPE_GUEST1" ]; then
        __repacd_add_vlan_interfaces $NETWORK_TYPE_GUEST1 $ifaceBH $vid_guest1
    fi
    if [ -n "$NETWORK_TYPE_GUEST2" ]; then
        __repacd_add_vlan_interfaces $NETWORK_TYPE_GUEST2 $ifaceBH $vid_guest2
    fi
    if [ -n "$NETWORK_TYPE_GUEST3" ]; then
        __repacd_add_vlan_interfaces $NETWORK_TYPE_GUEST3 $ifaceBH $vid_guest3
    fi
}

# Remove VLAN interfaces for guest networks if they are valid
# input: $1 iface name
__repacd_delete_guest_nw_sta_vlan_interfaces() {
    local iface="$1"

    if [ -n "$NETWORK_TYPE_GUEST1" ]; then
        __repacd_delete_interface $NETWORK_TYPE_GUEST1 $iface.$vid_guest1
    fi
    if [ -n "$NETWORK_TYPE_GUEST2" ]; then
        __repacd_delete_interface $NETWORK_TYPE_GUEST2 $iface.$vid_guest2
    fi
    if [ -n "$NETWORK_TYPE_GUEST3" ]; then
        __repacd_delete_interface $NETWORK_TYPE_GUEST3 $iface.$vid_guest3
    fi
}

__repacd_map_remove_sta_vlan() {
    local sta_iface="$1"

    if [ "$sta_vid" -gt 0 ]; then
        __repacd_delete_interface $NETWORK_TYPE_LAN $sta_iface.$sta_vid
    fi

    __repacd_delete_guest_nw_sta_vlan_interfaces $sta_iface
}

# Check if backhaul BSS are VLAN configured. If not apply VLAN read at init
# for each backhaul for primary and secondary networks
# input: $1 config
__repacd_map_vlanmon_check_bh_bss_vlan_config() {
    local config="$1"
    local iface network disabled device ifaceBH
    local add_vlan=0
    local device hwmode

    config_get iface "$config" ifname
    config_get network "$config" network
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get mapVlanID "$config" mapVlanID '0'
    config_get MapBSSType "$config" MapBSSType '0'
    config_get mld "$config" mld ''
    config_get spEnabled MAPSPSettings 'EnableSP' '0'
    config_get device "$config" device
    config_get hwmode "$device" hwmode

    if [ $((MapBSSType & 0x40)) -ne 64 ]; then
        continue
    fi

    if [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" != "sta" ]; then

        # if r2 STA Assoc DisAllowed do not create vlan
        if [ $(($((MapBSSType&4)) >> 2)) -eq 1 ]; then
            return
        fi

        ifaceBH=$iface
        if [ "$enable_single_netdev" -eq 1 ] &&
            [ "$hwmode" = "11bea" -o "$hwmode" = "11beg" ]; then
            if [ -n "$mld" ]; then
                ifaceBH=$mld
            fi
        fi

        __repacd_map_vlanmon_dump "backhaul BSS $ifaceBH. Set VLAN"
        if [ -n "$NETWORK_TYPE_LAN" ]; then
            if [ "$map_single_r1r2_bh" -eq 1 ]; then
                if [ "$ifaceBH" != "$sta_iface" ]; then
                    __repacd_map_vlanmon_debug "Add vlan. Bringing up $ifaceBH"
                    ifconfig $ifaceBH up
                    sleep 2
                    __repacd_add_interface "$NETWORK_TYPE_LAN" "$ifaceBH"
                fi
            fi
            # Check if VLAN interface is already created
            if [ ! -e "/sys/class/net/"$ifaceBH.$vid_lan"" ]; then
                add_vlan=1
            fi

            # Check if VLAN interface is part of bridge
            if [ ! -e "/sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/"$ifaceBH.$vid_lan"" ]; then
                add_vlan=1
            fi

            # Check if VLAN interface is part of network
            local ifnames_list="$(hyfi_network_get_ifnames $NETWORK_TYPE_LAN)"
            vlan_ifname_nw=$(echo "$ifnames_list" | grep -w "$ifaceBH.$vid_lan")

            if [ -z "$vlan_ifname_nw" ]; then
                add_vlan=1
            fi

            if [ "$add_vlan" -eq 1 ]; then
                sleep 2
            fi
            __repacd_add_vlan_interfaces $NETWORK_TYPE_LAN $ifaceBH $vid_lan
        fi

        __repacd_add_guest_network_vlan_interfaces $ifaceBH

        if [ "$maplite_enabled" -eq 1 ]; then
            local hostapd_vlan=$(grep multi_ap_vlan /var/run/hostapd-$iface.conf | cut -d '=' -f2)
            __repacd_map_vlanmon_debug "Hostapd Vlan $hostapd_vlan"
            if [ -z "$hostapd_vlan" -a "$vid_lan" -gt 0 ]; then
                __repacd_map_vlanmon_debug "Restart BHBSS $iface"
                local radioIdx=$(echo $iface | cut -c 4)
                wifi multi_up wifi$radioIdx $iface
                if [ "$spEnabled" -eq 1 ]; then
                    __repacd_map_set_egress_ingress_per_intf $iface $vid_lan
                fi
            fi
        fi
    fi
}

# Return the backhaul mld interface
# input: $1 config
# output: $2 mld interface
__repacd_map_vlanmon_backhaul_mld() {
    local config="$1"
    local iface network disabled device mode interface_mld

    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get network "$config" network
    interface_mld=$(uci get wireless.$config.mld 2>/dev/null)

    if [ -n "$iface" -a "$disabled" -eq 0 -a "$network" = $NETWORK_TYPE_BACKHAUL \
            -a "$mode" = "sta" ]; then
        if [ -n "$interface_mld" ]; then
            eval "$2=$interface_mld"
            return
        fi
    fi

}

__repacd_delete_nw_vland_id() {
    local config=$1
    local ifaces remove_iface
    local iface nw_name ifnames nw_intf

    for nw_name in $NETWORK_TYPE_LAN $NETWORK_TYPE_GUEST1 $NETWORK_TYPE_GUEST2 $NETWORK_TYPE_GUEST3; do
        if [ -n "$nw_name" ]; then
            ifnames="$(hyfi_network_get_ifnames $nw_name)"
            for iface in $ifnames; do
                if __hyfi_is_vlan_iface $iface; then
                    if [ -n "$iface" ]; then
                        remove_iface=$(ifconfig $iface | grep "UP[A-Z' ']*RUNNING")
                        if [ -z "$remove_iface" ]; then
                            __repacd_delete_interface $nw_name $iface
                        fi
                    fi
                fi
            done
        fi
    done
}

# Check if backhaul link is VLAN configured.
# if backhaul is eth: apply vlan only on guest networks (per spec)
# if backhaul is sta: get vlan id from IOCTL set from assoc response
# and apply primary and secondary VLANs
__repacd_map_vlanmon_check_backhaul_vlan_config() {
    local ifaces_eth ifaces iface_wan
    local mld_iface=''

    __repacd_map_vlanmon_dump " [ Configure backhaul Link with VLAN ] "

    if [ "$map_bsta_backhaul" -eq 1 ]; then
        if [ "$MBsta_non_mlo_mode" -eq 1 ] && [ "$MBsta_onboarding" -eq 1 ]; then
            sta_iface="$MBsta_non_mlo_sta_iface_list"
            __repacd_map_vlanmon_debug "Update STA_IFACE to NONMLOSTAconnected"
        fi
        __repacd_map_vlanmon_dump "backhaul Type STA; iface: $sta_iface"
        eth_iface=""
        local staBitRate=$(repacdcli $sta_iface get_bitrate)
        if [ "$staBitRate" -eq 0 -o -z "$staBitRate" ]; then
            if [ "$MBsta_non_mlo_mode" -eq 1 ] && [ "$MBsta_onboarding" -eq 1 ] && [ "$non_11be_radio_count" -gt 0 ]; then
                local tmp_MBsta_connected_bhssid=$(wpa_cli -i $sta_iface -p /var/run/wpa_supplicant-$sta_iface status | grep 'ssid' | awk 'FNR == 2 {print}' | awk -F = '{print$2}')
                local wps_state=$(wpa_cli -i $sta_iface -p /var/run/wpa_supplicant-$sta_iface status | grep 'wpa_state' | awk -F = '{print$2}')
                __repacd_map_vlanmon_debug "WPASTATE $wps_state"
                if [ "$wps_state" = "INTERFACE_DISABLED" ]; then
                    __repacd_map_vlanmon_debug "ifconfig $sta_iface up"
                    ifconfig $sta_iface up
                fi
            fi
            if [ "$MBsta_onboarding" -eq 0 ] && [ "$non_11be_radio_count" -gt 0 ]; then
                local tmp_MBsta_connected_bhssid=$(wpa_cli -i $sta_iface -p /var/run/wpa_supplicant-$sta_iface status | grep 'ssid' | awk 'FNR == 2 {print}' | awk -F = '{print$2}')
                local wps_state=$(wpa_cli -i $sta_iface -p /var/run/wpa_supplicant-$sta_iface status | grep 'wpa_state' | awk -F = '{print$2}')
                __repacd_map_vlanmon_debug "WPASTATE $wps_state"
                if [ "$wps_state" = "INTERFACE_DISABLED" ]; then
                    __repacd_map_vlanmon_debug "ifconfig $sta_iface up"
                    ifconfig $sta_iface up
                fi
            fi
            if [ "$mldbsta_enabled" -eq 1 ]; then
                secondary_sta_iface=''
                config_load wireless
                config_foreach __repacd_map_find_second_sta wifi-iface
                if [ -n "$secondary_sta_iface" ]; then
                    local staBitRatesecondary=$(repacdcli $secondary_sta_iface get_bitrate)
                    local roundOffStaBitRatesecondary=${staBitRatesecondary%.*}
                    if [ "$roundOffStaBitRatesecondary" -eq 0 ] || [ -z "$staBitRatesecondary" ]; then
                        __repacd_map_vlanmon_debug "Secondary Sta Bit Rate is also Invalid"
                        return
                    fi
                    if [ "$roundOffStaBitRatesecondary" -gt 0 ]; then
                        __repacd_map_vlanmon_dump "bit rate valid for secondary sta: $secondary_sta_iface"
                    fi
                fi
            else
                __repacd_map_vlanmon_debug "bSta Bit Rate Invalid. Bringing up $sta_iface"
                return
            fi
        fi
        if [ "$MBsta_onboarding" -eq 0 ]; then
            if [ -n "$NETWORK_TYPE_LAN" ]; then
                # Apply VLAN on STA Interface
                if [ "$sta_vid" -gt 0 ]; then
                    uci_set repacd MAPConfig MapTrafficSeparationActive '1'
                    uci_commit repacd

                    if [ -e "/sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/"$sta_iface"" ]; then
                        __repacd_map_vlanmon_debug "Interface $sta_iface is present in br-$NETWORK_TYPE_LAN and delete it"
                        brctl delif br-$NETWORK_TYPE_LAN $sta_iface
                    fi
                    __repacd_map_vlanmon_dump "Apply STA Vlan Configuration"
                    if [ "$sta_vid" -ne "$map_primary_bsta_vid" ]; then
                        __repacd_map_vlanmon_debug "sta_vid($sta_vid) and primary_bsta_vid($map_primary_bsta_vid) is different"
                        __repacd_delete_interface $NETWORK_TYPE_LAN $sta_iface
                        __repacd_delete_guest_nw_sta_vlan_interfaces $sta_iface
                        map_primary_bsta_vid=$sta_vid
                        sta_config_changed=1
                    fi

                    ifaceBH=$sta_iface
                    if [ "$enable_single_netdev" -eq 1 ]; then
                        config_load wireless
                        config_foreach __repacd_map_vlanmon_backhaul_mld wifi-iface mld_iface
                        if [ -n "$mld_iface" ]; then
                            ifaceBH=$mld_iface
                        fi
                    fi
                    __repacd_add_vlan_interfaces $NETWORK_TYPE_LAN $ifaceBH $sta_vid
                    __repacd_add_guest_network_vlan_interfaces $ifaceBH
                fi
            fi
        else
            local interface="$(iw dev $iface info | grep ssid | awk -F " " '{print $2}')"
            __repacd_map_vlanmon_debug "sta_iface $sta_iface interface $interface"
            if [ -n "$NETWORK_TYPE_LAN" ] || [ -n "$interface" ]; then
                # Apply VLAN on STA Interface
                uci_set repacd MAPConfig MapTrafficSeparationActive '1'
                uci_commit repacd

                if [ -e "/sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/"$sta_iface"" ]; then
                    __repacd_map_vlanmon_debug "Interface $sta_iface is present in br-$NETWORK_TYPE_LAN and delete it"
                    brctl delif br-$NETWORK_TYPE_LAN $sta_iface
                    __repacd_map_vlanmon_debug "delif from lan $sta_iface staiface"
                fi
                __repacd_map_vlanmon_dump "Apply STA Vlan Configuration"
                local sta_vlanid=$(eval cfg80211tool_mesh $sta_iface $MAP_GET_STA_VLAN_IOCTL)
                sta_vlanid=${sta_vlanid#*:}
                if [ "$sta_vlanid" -ne "$map_primary_bsta_vid" ]; then
                    __repacd_delete_interface $NETWORK_TYPE_LAN $sta_iface
                    __repacd_delete_guest_nw_sta_vlan_interfaces $sta_iface
                    __repacd_map_vlanmon_debug "stavid $sta_vlanid != map_primary_bsta_vid $map_primary_bsta_vid"
                    map_primary_bsta_vid=$sta_vlanid
                    sta_config_changed=1
                fi

                ifaceBH=$sta_iface
                __repacd_map_vlanmon_dump "Number of Non-11be Radio: $non_11be_radio_count"
                if [ "$MBsta_mlo_mode" -eq 1 ]; then
                    ifaceBH="$MBsta_mlo_bstaMld"
                elif [ "$MBsta_non_mlo_mode" -eq 1 ] && [ "$non_11be_radio_count" -eq 0 ] && [ -n "$MBsta_nonmlo_bstaMld" ]; then
                    ifaceBH="$MBsta_nonmlo_bstaMld"
                fi

                #Race condition that MLO or Non MLO mode may not be updated at this check
                #Both will be 0, then it will add athx.vlanid to bridge.
                if [ "$MBsta_mlo_mode" -gt 0 ] || [ "$MBsta_non_mlo_mode" -gt 0 ]; then
                    if [ "$sta_vlanid" -gt 0 ]; then
                        if [ -e "/sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/"$ifaceBH"" ]; then
                            __repacd_map_vlanmon_debug "Interface $ifaceBH is present in br-$NETWORK_TYPE_LAN and delete it"
                            brctl delif br-$NETWORK_TYPE_LAN $ifaceBH
                        fi

                        __repacd_map_vlanmon_debug "Adding BH iface:$ifaceBH with Vid:$sta_vlanid to primary bridge"
                        __repacd_add_vlan_interfaces $NETWORK_TYPE_LAN $ifaceBH $sta_vlanid
                        __repacd_add_guest_network_vlan_interfaces $ifaceBH
                    fi
                fi
            fi
        fi
    fi

    # if backhaul is ETH wait for all AP vaps to be UP
    if [ "$map_fh_bh_vap_up" -eq 0 ]; then
        return
    fi

    __repacd_map_vlanmon_dump "ETH Iface to GW; iface: $eth_iface"

    for iface in /sys/class/net/eth*; do
    iface=`basename $iface`
        case $iface in
            *.*)
                # we will get valid eth interface without vlan as primary is untagged
                    continue
                ;;
        esac


        # Add ethernet lan interface to primary ifname if link is detected
        link_detected=$(ethtool $iface | grep "Link detected: yes")
        link_up=$(cat /sys/class/net/$iface/operstate)
        if [ "$link_up" == "up"  -a  -n "$link_detected" ]; then
            gw_ip=$(ip r | awk '/^def/{print $3}')
            if [ -z "$gw_ip" ]; then
                __repacd_map_vlanmon_debug "Could not respolve gw_ip; Check route"
            fi

            if [ "$MAP_IS_GW_REACHABLE" -eq 0 -o -z "$eth_iface" ]; then
                if __repacd_map_arping_confirm_gateway "br-$NETWORK_TYPE_LAN" $gw_ip $iface; then
                    eth_iface=$iface
                fi
            fi
            #TODO: The condition "$eth_iface == $iface" should be included below and verified
            if [ "$openwrt_version_check" -eq 1 ]; then
                iface_wan=$(uci get network.wan.device)
            else
                iface_wan=$(uci get network.wan.ifname)
            fi

            if [ "$iface" != "$iface_wan" ]; then
                # Daisy RE might be connected on other ETH
                # On Ethernet Backhaul only secondary VLAN needs to be created
                __repacd_map_vlanmon_dump " Add Vlan for iface: $iface"
                __repacd_add_guest_network_vlan_interfaces $iface
                continue
            fi
        fi
    done

    __repacd_map_vlanmon_dump "___________________________________________________________________"
}

__repacd_map_vlanmon_get_wlan_vlan_config() {
    local config="$1"
    local iface network disabled device
    local MAPeapolOverBridge mld mapVlanID bhmloenabled
    local isCacInProgress MapBSSType hidden mode upstream_version

    config_get iface "$config" ifname
    config_get network "$config" network
    config_get disabled "$config" disabled '0'
    config_get mapVlanID "$config" mapVlanID '0'
    config_get mode "$config" mode
    config_get MapBSSType "$config" MapBSSType '0'
    config_get device "$config" device
    config_get upstream_version "$device" upstream_version '1'
    config_get mld "$config" mld
    config_get MAPeapolOverBridge MAPConfig "MapEapolOverBridge" '0'
    config_get mld "$config" mld ''
    config_get hidden "$config" hidden '0'
    config_get_bool bhmloenabled "$device" map_mbsta_bhmlo_enabled '0'

    config_get map_vid_lan 'MAPWiFiLink' 'vid_lan' '0'
    config_get map_vid_guest1 'MAPWiFiLink' 'vid_guest1' '0'
    config_get map_vid_guest2 'MAPWiFiLink' 'vid_guest2' '0'
    config_get map_vid_guest3 'MAPWiFiLink' 'vid_guest3' '0'

    local interface="$(iw dev $iface info | grep ssid | awk -F " " '{print $2}')"

    if [ "$vid_lan" -gt 0 ] && [ "$map_vid_lan" != "$vid_lan" ]; then
        uci_set repacd MAPWiFiLink 'vid_lan' "$vid_lan"
        uci set repacd.MAPConfig.VlanIDNwPrimary=$vid_lan
        uci_commit repacd
    fi
    if [ "$vid_guest1" -gt 0 ] && [ "$map_vid_guest1" != "$vid_guest1" ]; then
        uci_set repacd MAPWiFiLink 'vid_guest1' "$vid_guest1"
        uci set repacd.MAPConfig.VlanIDNwOne=$vid_guest1
        uci_commit repacd
    fi
    if [ "$vid_guest2" -gt 0 ] && [ "$map_vid_guest2" != "$vid_guest2" ]; then
        uci_set repacd MAPWiFiLink 'vid_guest2' "$vid_guest2"
        uci set repacd.MAPConfig.VlanIDNwTwo=$vid_guest2
        uci_commit repacd
    fi
    if [ "$vid_guest3" -gt 0 ] && [ "$map_vid_guest3" != "$vid_guest3" ]; then
        uci_set repacd MAPWiFiLink 'vid_guest3' "$vid_guest3"
        uci set repacd.MAPConfig.VlanIDNwThree=$vid_guest3
        uci_commit repacd
    fi
    if [ -z "$interface" ]; then
        if [ "$map_vid_lan" -gt 0 ] && [ "$map_vid_lan" != "$vid_lan" ]; then
            vid_lan="$map_vid_lan"
            uci_set repacd MAPWiFiLink 'vid_lan' "$vid_lan"
            uci set repacd.MAPConfig.VlanIDNwPrimary=$vid_lan
            uci_commit repacd
        fi
        if [ "$map_vid_guest1" -gt 0 ] && [ "$map_vid_guest1" != "$vid_guest1" ]; then
            vid_guest1="$map_vid_guest1"
            uci_set repacd MAPWiFiLink 'vid_guest1' "$vid_guest1"
            uci set repacd.MAPConfig.VlanIDNwOne=$vid_guest1
            uci_commit repacd
	    fi
        if [ "$map_vid_guest2" -gt 0 ] && [ "$map_vid_guest2" != "$vid_guest2" ]; then
            vid_guest2="$map_vid_guest2"
            uci_set repacd MAPWiFiLink 'vid_guest2' "$vid_guest2"
            uci set repacd.MAPConfig.VlanIDNwTwo=$vid_guest2
            uci_commit repacd
	    fi
        if [ "$map_vid_guest3" -gt 0 ] && [ "$map_vid_guest3" != "$vid_guest3" ]; then
            vid_guest3="$map_vid_guest3"
            uci_set repacd MAPWiFiLink 'vid_guest3' "$vid_guest3"
            uci set repacd.MAPConfig.VlanIDNwThree=$vid_guest3
            uci_commit repacd
        fi
    fi

    if [ "$maplite_enabled" -ne 1 ]; then
        if [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" = "ap" ] && [ "$hidden" -ne 1 -o "$external_controller" -eq 1 ]; then
            if [ "$map_my_version" -ge 6 ]; then
                link_removed_list=$(uci get repacd.MAPConfig.LinksRemoved)
                local skipLinkMonitor=0
                for link in $link_removed_list; do
                    if [ "$link" == "$iface" ]; then
                        __repacd_map_vlanmon_debug " Skip monitor for: $iface"
                        skipLinkMonitor=1
                    fi
                done
                if [ "$skipLinkMonitor" -eq 1 ]; then
                    continue
                fi
            fi
            local bitRate=$(repacdcli $iface get_bitrate)
            if [ "$bitRate" -eq 0 -o -z "$bitRate" ]; then
                local isAcsInProgress=$(cfg80211tool $iface get_acs_state)
                isAcsInProgress=${isAcsInProgress#*:}
                isCacInProgress=$(cfg80211tool $iface get_cac_state)
                isCacInProgress=${isCacInProgress#*:}
                # wait time for 2seconds after CAC timer is over to get bitrate set
                if [ "$CACstate" -eq 1 -a "$isCacInProgress" -ne 1 ]; then
                    sleep 2
                    bitRate=$(repacdcli $iface get_bitrate)
                    CACstate=$(cfg80211tool $iface get_cac_state)
                    CACstate=${CACstate#*:}
                fi
                if [ "$CACstate" -eq 0 ] && [ "$bitRate" -eq 0 -o -z "$bitRate" ]; then
                    isCacInProgress=$(cfg80211tool $iface get_cac_state)
                    isCacInProgress=${isCacInProgress#*:}
                    CACstate= $isCacInProgress
                    if [ "$isCacInProgress" -eq 0 -a -n "$mld" ] && [ "$isAcsInProgress" -eq 0 ]; then
                        if [ $((MapBSSType & 0x10)) -eq 16 ]; then
                            __repacd_map_vlanmon_debug "Iface $iface is teardown vap. Don't bringup"
                        else
                            if [ "$map_my_version" -ge 6 ]; then
                                link_removed_list=$(uci get repacd.MAPConfig.LinksRemoved)
                                local skipLinkMonitor=0
                                for link in $link_removed_list; do
                                    if [ "$link" == "$iface" ]; then
                                        __repacd_map_vlanmon_debug " Skip monitor for: $iface"
                                        skipLinkMonitor=1
                                    fi
                                done
                                if [ "$skipLinkMonitor" -eq 1 ]; then
                                    continue
                                fi
                            fi
                            __repacd_map_vlanmon_debug "Iface $iface has invalid Bit Rate $bitRate post cac.
                                    Do multi_up for $mld"
                            wifi multi_up $mld
                            sleep 5
                        fi
                    elif [ "$isCacInProgress" -eq 0 ] && [ "$isAcsInProgress" -eq 0 ]; then
                        if [ $((MapBSSType & 0x10)) -eq 16 ]; then
		                    __repacd_map_vlanmon_debug "Iface $iface is teardown vap. Don't bringup"
                        else
                            if [ "$map_my_version" -ge 6 ]; then
                                link_removed_list=$(uci get repacd.MAPConfig.LinksRemoved)
                                local skipLinkMonitor=0
                                for link in $link_removed_list; do
                                    if [ "$link" == "$iface" ]; then
                                        __repacd_map_vlanmon_debug " Skip monitor for: $iface"
                                        skipLinkMonitor=1
                                    fi
                                done
                                if [ "$skipLinkMonitor" -eq 1 ]; then
                                    continue
                                fi
                            fi
                            __repacd_map_vlanmon_debug "wlanconfig: Cmd: ifconfig $iface down"
                            ifconfig $iface down
                            hapd $iface disable
                            sleep 2
                            __repacd_map_vlanmon_debug "wlanconfig: Cmd: ifconfig $iface up"
                            ifconfig $iface up
                            hapd $iface enable
                        fi
                    fi
                fi
            fi
        fi
    fi
    if [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" = "ap_smart_monitor" ]; then
        continue
    fi

    if [ "$mode" = "ap" ] && [ $((MapBSSType & 0x10)) -ne 16 ]; then
        __repacd_is_ap_iface_up $iface
        local iface_up=$?
        # For PF we might have some bands teared down. Add Bit rate check to
        # get correct vlanID
        if [ "$iface_up" -eq 0 ]; then
            continue
        fi
    fi

    if [ -n "$iface" -a "$disabled" -eq 0 -a "$network" = $NETWORK_TYPE_LAN \
            -a "$mode" = "ap" -a "$mapVlanID" -gt 0 ]; then
        vid_lan=$mapVlanID
    fi

    if [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" = "sta" ] && [ "$MBsta_IgnoreBstaOnEthBH" -eq 0 ]; then
        map_bsta_backhaul=1
        sta_iface=$iface
        if [ -n "$mld" ]; then
            mlobsta_list="$mlobsta_list $iface"
        fi
        if [ -n "$primary_mldbsta_iface" ] && [ "$primary_mldbsta_iface" != "$iface" ]; then
            sta_iface=$primary_mldbsta_iface
        fi
        sta_vid=$(eval cfg80211tool_mesh $sta_iface $MAP_GET_STA_VLAN_IOCTL)
        sta_vid=${sta_vid#*:}
        if [ "$sta_vid" -gt 0 ]; then
            if [ "$MBsta_onboarding" -ne 1 ] && [ -z "$primary_mldbsta_iface" -a "$mldbsta_enabled" -eq 1 ]; then
                 primary_mldbsta_iface=$iface
            fi

            ifaceBH=$iface
            if [ "$enable_single_netdev" -eq 1 ]; then
                if [ -n "$mld" ]; then
                    ifaceBH=$mld
                fi
            fi
            if [ "$MBsta_onboarding" -eq 1 ]; then
                local interface="$(iw dev $iface info | grep ssid | awk -F " " '{print $2}')"
                #Always check if interface is connected or not even if vid was
                #greater when MBsta onboarding enabled
                if [ -n "$interface" ]; then
	            if [ -z "$primary_mldbsta_iface" -a "$mldbsta_enabled" -eq 1 ] && [ "$bhmloenabled" -eq 1 ]; then
                        primary_mldbsta_iface=$iface
                    fi
                    __repacd_map_vlanmon_dump "interface $iface getting deleted mld: $ifaceBH"
                    brctl delif br-$NETWORK_TYPE_LAN $ifaceBH
                    __repacd_add_vlan_interfaces $NETWORK_TYPE_LAN $ifaceBH $sta_vid
                    uci_set wireless "$config" network "$NETWORK_TYPE_BACKHAUL"
                else
                    __repacd_vlanmon_dump "Not Adding vlan since interface $iface is disconnected"
                fi
            else
                if [ -e "/sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/"$ifaceBH"" ]; then
                    __repacd_map_vlanmon_dump "Interface $ifaceBH is present in br-$NETWORK_TYPE_LAN and delete it"
                    brctl delif br-$NETWORK_TYPE_LAN $ifaceBH
                fi
                __repacd_add_vlan_interfaces $NETWORK_TYPE_LAN $ifaceBH $sta_vid
                uci_set wireless "$config" network "$NETWORK_TYPE_BACKHAUL"
            fi
            if [ "$MAPeapolOverBridge" -gt 0 ]; then
                uci_set wireless "$config" vlan_bridge "br-$NETWORK_TYPE_LAN"
            fi
        elif [ "$sta_vid" -eq 0 ]; then
            if [ "$enable_slo" != 1 -a "$map_ts_active" -eq 1 -a "$maplite_enabled" -ne 1 ]; then
                #Add partner STA to network file
                __repacd_add_vlan_interfaces $NETWORK_TYPE_LAN $iface $vid_lan
                __repacd_add_vlan_interfaces $NETWORK_TYPE_GUEST1 $iface $vid_guest1
                __repacd_add_vlan_interfaces $NETWORK_TYPE_GUEST2 $iface $vid_guest2
                __repacd_add_vlan_interfaces $NETWORK_TYPE_GUEST3 $iface $vid_guest3
                uci_set wireless "$config" network "$NETWORK_TYPE_BACKHAUL"
            else
                if [ "$MBsta_onboarding" -eq 1 ]; then 
                    local interface="$(iw dev $iface info | grep ssid | awk -F " " '{print $2}')"
                    if [ -z "$interface" ]; then
                        uci_set wireless "$config" network "$NETWORK_TYPE_LAN"
                    fi
                else
                    uci_set wireless "$config" network "$NETWORK_TYPE_LAN"
                fi
            fi
            if [ "$maplite_enabled" -eq 1 -a "$upstream_version" -ge 2 -a "$vid_lan" -gt 0 ]; then
                sta_vid=$vid_lan
                uci_set wireless "$config" network "$NETWORK_TYPE_BACKHAUL"
                if [ "$MAPeapolOverBridge" -gt 0 ]; then
                    uci_set wireless "$config" vlan_bridge "br-$NETWORK_TYPE_LAN"
                fi
            fi
        fi

        uci_commit wireless
    fi

    # if iface is BH . it wont have vlanID in wireless configured
    if [ $(($((MapBSSType&32)) >> 5)) -eq 0 ]; then
        continue
    fi

    if [ -n "$iface" -a "$disabled" -eq 0 -a "$mapVlanID" -gt 0 ]; then
        [ "$mode" = "sta" -a "$network" = $NETWORK_TYPE_BACKHAUL -a "$MBsta_IgnoreBstaOnEthBH" -eq 0 ] && {
            map_primary_bsta_vid=$mapVlanID
    }
    [ "$mode" = "ap" ] && {
        case "$network" in
            "$NETWORK_TYPE_GUEST1")
                vid_guest1=$mapVlanID
                ;;
            "$NETWORK_TYPE_GUEST2")
                vid_guest2=$mapVlanID
                ;;
            "$NETWORK_TYPE_GUEST3")
                vid_guest3=$mapVlanID
                ;;
        esac
    }

    fi

    # vid change is detected before wifi happens. Add sleep to let it go through wifi
    if [ "$vid_8021q" -ne "$vid_lan" ]; then
        __repacd_map_vlanmon_debug "vid_lan:$vid_lan , vid_8021q:$vid_8021q"
        __repacd_map_vlanmon_debug "VID Information Changing. Sleep for 3 Seconds"
        # Onboarding might be happening. Sleep to avoid race condn
        sleep 3
        vid_8021q=$vid_lan
    fi
}

__repacd_map_vlan_removal_checks() {
    local config="$1"
    local iface network disabled device

    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get MapBSSType "$config" MapBSSType '0'
    config_get network "$config" network

    if [ -n "$iface" -a "$disabled" -eq 0 -a "$network" = $NETWORK_TYPE_BACKHAUL \
            -a "$mode" != "sta" ]; then
        brctl addif br-lan $iface
    fi

    if [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" = "ap" ]; then
        uci_set wireless "$config" network "$NETWORK_TYPE_LAN"
        uci_commit wireless
    fi
}

# While TS is disabled dynamically BH AP VAPs are still in network type backhaul even after deleting the vlan.
# This cause ezmesh failure because backhaul network in unknown with R1/TS disable case
# In this function network type is changed to LAN in vlan removal flow.
__repacd_map_vlanmon_update_BH_AP_network() {
    local config="$1" nw_change="$2"
    local iface network disabled device

    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get MapBSSType "$config" MapBSSType '0'
    config_get network "$config" network

    [ "$mode" = "ap_smart_monitor" ] && return

    if [ -n "$iface" -a "$disabled" -eq 0 -a "$network" = "$NETWORK_TYPE_BACKHAUL" ]; then
        uci_set wireless "$config" network "$nw_change"
    fi
}

__repacd_map_vlanmon_remove_vlan() {
    local ifaces_eth ifaces_ath ifaces iface_wan
    local sta_if sta_if_cfg if_unknown sta_vid
    if [ "$openwrt_version_check" -eq 1 ]; then
        local ifnames_config=''
    fi

    if [ -n "$NETWORK_TYPE_GUEST1" ]; then
        if [ "$openwrt_version_check" -eq 1 ]; then
            hyfi_network_get_device_config "$NETWORK_TYPE_GUEST1" ifnames_config
            uci delete network.$ifnames_config.ports
            ifnames_config=''
        else
            uci_set network "$NETWORK_TYPE_GUEST1" ifname ' '
        fi
    fi

    if [ -n "$NETWORK_TYPE_GUEST2" ]; then
        if [ "$openwrt_version_check" -eq 1 ]; then
            hyfi_network_get_device_config "$NETWORK_TYPE_GUEST2" ifnames_config
            uci delete network.$ifnames_config.ports
            ifnames_config=''
        else
            uci_set network "$NETWORK_TYPE_GUEST2" ifname ' '
        fi
    fi

    if [ -n "$NETWORK_TYPE_GUEST3" ]; then
        if [ "$openwrt_version_check" -eq 1 ]; then
            hyfi_network_get_device_config "$NETWORK_TYPE_GUEST3" ifnames_config
            uci delete network.$ifnames_config.ports
            ifnames_config=''
        else
            uci_set network "$NETWORK_TYPE_GUEST3" ifname ' '
        fi
    fi

    ifaces_ath=$(ifconfig -a 2>&1 | grep ath)
    ifaces=$(echo "$ifaces_ath" | cut -d ' ' -f1)
    for iface in $ifaces; do
        # Delete interface that is vlan configured
        echo "$iface" | grep '\.' >/dev/null 2>&1
        if [ "$?" -eq "0" ]; then
            __repacd_delete_interface $NETWORK_TYPE_LAN $iface
            continue
        else
            if [ "$maplite_enabled" -eq 1 ]; then
                __repacd_add_interface $NETWORK_TYPE_LAN $iface
            fi
        fi
    done

    ifaces_mld=$(ifconfig -a 2>&1 | grep mld)
    iface_mld=$(echo "$ifaces_mld" | cut -d ' ' -f1)
    for iface in $iface_mld; do
	# Delete interface that is vlan configured
        echo "$iface" | grep '\.' >/dev/null 2>&1
        if [ "$?" -eq "0" ]; then
            __repacd_delete_interface $NETWORK_TYPE_LAN $iface
            continue
        else
            if [ "$maplite_enabled" -eq 1 ]; then
                __repacd_add_interface $NETWORK_TYPE_LAN $iface
            fi
        fi
    done

    ifaces_eth=$(ifconfig 2>&1 | grep eth)
    if [ "$openwrt_version_check" -eq 1 ]; then
        iface_wan=$(uci get network.wan.device)
    else
        iface_wan=$(uci get network.wan.ifname)
    fi
    ifaces=$(echo "$ifaces_eth" | cut -d ' ' -f1)
    for iface in $ifaces; do
        # Delete interface that is vlan configured
        echo "$iface" | grep '\.' >/dev/null 2>&1
        if [ "$?" -eq "0" ]; then
            __repacd_delete_interface $NETWORK_TYPE_LAN $iface
            continue
        fi

        # Add ethernet lan interface to primary ifname if link is detected
        link_detected=$(ethtool $iface | grep Link | grep detected | awk -F':' '{print $2}' \
                            | awk '{$1=$1};1')
        link_up=$(ifconfig $iface | grep "UP[A-Z' ']*RUNNING")

        if [ -n "$link_up" -a "$link_detected" = "yes" ]; then
            # Add ethernet lan interface to primary ifname if link is detected
            if [ "$iface" != "$iface_wan" ]; then
                __repacd_add_interface $NETWORK_TYPE_LAN $iface
            fi
        fi
    done
    if [ "$MBsta_onboarding" -eq 0 ]; then
        if [ -n "$sta_iface_24g" ]; then
            sta_if="$sta_iface_24g"
            sta_if_cfg="$sta_iface_24g_config_name"
        elif [ -n "$sta_iface_5g" ]; then
            sta_if="$sta_iface_5g"
            sta_if_cfg="$sta_iface_5g_config_name"
        else
            config_load wireless
            config_foreach __repacd_wifimon_is_sta_iface_map wifi-iface \
                "$NETWORK_TYPE_BACKHAUL" sta_if sta_if_cfg \
                sta_if sta_if_cfg if_unknown
        fi

        local mld=''
        # Network should be set to LAN if there is no Vlan ID configured for STA VAP
        if [ -n "$sta_if" -a -n "$sta_if_cfg" ]; then
            if [ -n "$primary_mldbsta_iface" ] && [ "$primary_mldbsta_iface" != "$sta_if" ]; then
                sta_if=$primary_mldbsta_iface
            fi
            sta_vid=$(eval cfg80211tool_mesh $sta_if $MAP_GET_STA_VLAN_IOCTL \
            | grep $MAP_GET_STA_VLAN_IOCTL | cut -d ':' -f2)
            if [ "$sta_vid" -eq 0 ]  && [ "$MBsta_IgnoreBstaOnEthBH" -eq 0 ]; then
                uci_set wireless "$sta_if_cfg" network "$NETWORK_TYPE_LAN"

                if [ "$map_onboarding_done" -eq 1 ]; then
                    #initially if we bring up the VAP as backhaul network type that should not be changed to LAN
                    config_foreach __repacd_map_vlanmon_update_BH_AP_network wifi-iface "$NETWORK_TYPE_LAN"
                fi

                config_get mld "$sta_if_cfg" mld
                if [ -n "$mld" ]; then
                   __repacd_map_vlanmon_debug "Adding STA MLD $mld to br-$NETWORK_TYPE_LAN without Vlan"
                   brctl addif br-$NETWORK_TYPE_LAN $mld
                   ifconfig $mld up
                fi
            fi
        fi
    else
        __repacd_map_vlanmon_debug "Set Lan as nw type"
        config_foreach __repacd_vlanmon_MBsta_setbh_network wifi-iface
        uci_commit wireless
    fi
    uci_commit network
    uci_commit wireless

    if [ "$maplite_enabled" -eq 1 ]; then
        config_load wireless
        config_foreach __repacd_map_vlan_removal_checks wifi-iface
    fi
}

# Return appropriate mld mapping for the interface
# input: $1 config
# input: $2 interface
# output: $3 respective mld mapping
__repacd_vlanmon_get_mld_mapping() {
    local config="$1"
    local iface network disabled device mld

    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get network "$config" network
    config_get mld "$config" mld ''

    if [ -n "$iface" -a "$iface" = "$2" -a "$mode" = "sta" ]; then
        if [ -n "$mld" ]; then
            eval "$3=$mld"
            return
        fi
    fi
}
__repacd_vlanmon_remove_sta_mld() {
    local config="$1"
    local mld_iface=''

    config_get iface "$config" ifname
    config_get mld "$config" mld
    config_get mode "$config" mode

    if [ "$mode" != "sta" ]; then
        return
    fi
    local ifacebh
    if [ -n "$mld" ]; then
        ifacebh="$mld"
        __repacd_vlanmon_debug "Override $iface with $mld"
    else
        ifacebh="$iface"
    fi

    if [ -e "/sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/"$ifacebh"" ]; then
        __repacd_map_vlanmon_debug "Interface ifacebh $ifacebh is present in br-$NETWORK_TYPE_LAN and delete it"
        brctl delif br-$NETWORK_TYPE_LAN $ifacebh
    fi

    if [ -e "/sys/class/net/br-"$NETWORK_TYPE_GUEST1"/brif/"$ifacebh"" ]; then
        __repacd_map_vlanmon_debug "Interface ifacebh $ifacebh is present in br-$NETWORK_TYPE_GUEST1 and delete it"
        brctl delif br-$NETWORK_TYPE_GUEST1 $ifacebh
    fi

    if [ -e "/sys/class/net/br-"$NETWORK_TYPE_GUEST2"/brif/"$ifacebh"" ]; then
        __repacd_map_vlanmon_debug "Interface ifacebh $ifacebh is present in br-$NETWORK_TYPE_GUEST2 and delete it"
        brctl delif br-$NETWORK_TYPE_GUEST2 $ifacebh
    fi

    if [ -e "/sys/class/net/br-"$NETWORK_TYPE_GUEST3"/brif/"$ifacebh"" ]; then
        __repacd_map_vlanmon_debug "Interface ifacebh $ifacebh is present in br-$NETWORK_TYPE_GUEST3 and delete it"
        brctl delif br-$NETWORK_TYPE_GUEST3 $ifacebh
    fi

}

__repacd_vlanmon_check_sta_vlan() {
    local iface="$1"
    local mld_iface=''
    local MAPeapolOverBridge

    config_get MAPeapolOverBridge MAPConfig "MapEapolOverBridge" '0'

    __repacd_map_vlanmon_dump "Check Vlan for STA $iface"
    if [ "$enable_single_netdev" -eq 1 ]; then
        config_load wireless
        config_foreach __repacd_vlanmon_get_mld_mapping wifi-iface "$iface" mld_iface
    fi
    local interface="$(iw dev $iface info | grep ssid | awk -F " " '{print $2}')"
    if [ "$MBsta_onboarding" -eq 0 ]; then
        if [ -n "$iface" ]; then
            map_bsta_backhaul=1
            sta_iface=$iface
            sta_vid=$(eval cfg80211tool_mesh $sta_iface $MAP_GET_STA_VLAN_IOCTL)
            sta_vid=${sta_vid#*:}
            if [ "$sta_vid" -gt 0 ]; then
                uci_set repacd MAPWiFiLink 'vid_lan' "$sta_vid"
                uci set repacd.MAPConfig.VlanIDNwPrimary=$sta_vid
                uci_commit repacd

                ifaceBH=$sta_iface
                if [ "$enable_single_netdev" -eq 1 ]; then
                    config_load wireless
                    config_foreach __repacd_vlanmon_get_mld_mapping wifi-iface "$iface" mld_iface
                    if [ -n "$mld_iface" ]; then
                        ifaceBH=$mld_iface
                        __repacd_map_vlanmon_dump "Override $sta_iface with MLD interface $ifaceBH"
                    fi
                fi
                brctl delif "br-$NETWORK_TYPE_LAN" $ifaceBH
                __repacd_add_vlan_interfaces $NETWORK_TYPE_LAN $ifaceBH $sta_vid
                local staIdx
                staIdx=$(uci show wireless | grep "].mode\='sta'")
                staIdx=${staIdx#*[}
                staIdx=${staIdx#*[}
                staIdx=${staIdx%]*}
                uci set wireless.@wifi-iface[$staIdx].network="$NETWORK_TYPE_BACKHAUL"
                if [ "$MAPeapolOverBridge" -gt 0 ]; then
                    uci set wireless.@wifi-iface[$staIdx].vlan_bridge="br-$NETWORK_TYPE_LAN"
                fi
                uci commit wireless
            elif [ "$sta_vid" -eq 0 ]; then
                uci set wireless.@wifi-iface[$staIdx].network="$NETWORK_TYPE_LAN"
                uci commit wireless
            fi
            uci_commit wireless
        fi
    else
        if [ -n "$iface" ]; then
            local non_mlo_sta
            #Store non mlo link in variable again using
            #for loop for successful comparisons between
            #strings, as whitespace is not ignored
            for intf in $MBsta_non_mlo_sta_iface_list; do
                non_mlo_sta="$intf"
            done
            map_bsta_backhaul=1
            sta_iface=$iface
            sta_vid=$(eval cfg80211tool_mesh $sta_iface $MAP_GET_STA_VLAN_IOCTL)
            sta_vid=${sta_vid#*:}
            __repacd_map_vlanmon_debug "iface $iface Assoc $interface sta_vid $sta_vid"
            if [ "$sta_vid" -gt 0 ]; then
                if [ -n "$interface" ]; then
                    ifaceBH=$sta_iface
                    if [ "$enable_single_netdev" -eq 1 ]; then
                        config_load wireless
                        config_foreach __repacd_vlanmon_get_mld_mapping wifi-iface "$iface" mld_iface
                        if [ -n "$mld_iface" ]; then
                            ifaceBH=$mld_iface
                            __repacd_map_vlanmon_dump "Override $sta_iface with MLD interface $ifaceBH"
                        fi
                    fi
                    brctl delif "br-$NETWORK_TYPE_LAN" $ifaceBH
                    __repacd_add_vlan_interfaces $NETWORK_TYPE_LAN $ifaceBH $sta_vid
                    local staIdx
                    staIdx=$(uci show wireless | grep "].mode\='sta'")
                    staIdx=${staIdx#*[}
                    staIdx=${staIdx#*[}
                    staIdx=${staIdx%]*}
                    uci set wireless.@wifi-iface[$staIdx].network="$NETWORK_TYPE_BACKHAUL"
                    if [ "$MAPeapolOverBridge" -gt 0 ]; then
                        uci set wireless.@wifi-iface[$staIdx].vlan_bridge="br-$NETWORK_TYPE_LAN"
                    fi
                    uci commit wireless
                else
                    if [ -n "$mld_iface" ]; then
                        ifaceBH=$mld_iface
                        __repacd_map_vlanmon_dump "Override $sta_iface with MLD interface $ifaceBH"
                    else
                        ifaceBH="$sta_iface"
                    fi
                    #Return if any one of the MLO links is connected due to skip_link_timer, don't touch the vlans
                    if [ "$MBsta_mlo_mode" -eq 1 -a "$MBsta_non_mlo_mode" -eq 0 ] && [ -n "$MBsta_mlo_sta_connected_list" ] && [ "$non_mlo_sta" != "$iface" ]; then
                        return
                    else
                        if [ -e "/sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/"$ifaceBH"" ]; then
                            __repacd_map_vlanmon_debug "Interface ifaceBH $ifaceBH is present in br-$NETWORK_TYPE_LAN and delete it"
                            brctl delif "br-$NETWORK_TYPE_LAN" $ifaceBH
                            __repacd_delete_interface $NETWORK_TYPE_LAN $ifaceBH.$sta_vid
                            __repacd_map_vlanmon_debug "DELETED ifacebh $ifaceBH with vid $sta_vid since nt connected"
                        fi
                        __repacd_delete_interface $NETWORK_TYPE_LAN $ifaceBH.$sta_vid
                    fi
                fi
            elif [ "$sta_vid" -eq 0 ]; then
                local mld_del
                config_foreach __repacd_vlanmon_get_mld_mapping wifi-iface "$sta_iface" mld_del
                #Sometimes mld_del is not getting retrieved correctly at this point, so war added in repacd-run
                #to delete parent mld without vlan during bsta switch
                if [ -z "$mld_del" ]; then
                    mld_del="$sta_iface"
                fi
                if [ -z "$interface" ]; then
                    if [ "$MBsta_mlo_mode" -eq 1 -a "$MBsta_non_mlo_mode" -eq 0 ] && [ -n "$MBsta_mlo_sta_connected_list" ] && [ "$non_mlo_sta" != "$iface" ]; then
                        __repacd_map_vlanmon_dump "RETURNING for iface: $iface, nonmlostalist $non_mlo_sta"
                        return
                    else
                        if [ -e "/sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/"$mld_del"" ]; then
                            __repacd_map_vlanmon_debug "Interface ifaceBH $mld_del is present in br-$NETWORK_TYPE_LAN and delete it"
                            brctl delif "br-$NETWORK_TYPE_LAN" $mld_del
                        fi
                        if [ -e "/sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/"$mld_del.$vid_lan"" ]; then
                            __repacd_delete_interface $NETWORK_TYPE_LAN $mld_del.$vid_lan
                        fi
                    fi
                elif [ -n "$interface" ]; then
                    config_get_bool map_enable_vlan_monitor MAPConfig 'MapTrafficSeparationEnable' '0'
                    if [ "$MBsta_mlo_mode" -eq 1 ]; then
                        local bstaiface
                        for bstaiface in $MBsta_mlo_sta_connected_list; do
                            __repacd_map_vlanmon_dump "iface $iface, bstaiface $bstaiface"
                            if [ "$iface" != "$bstaiface" ]; then
                                local bsta_vid=0
                                bsta_vid=$(eval cfg80211tool_mesh $bstaiface $MAP_GET_STA_VLAN_IOCTL)
                                bsta_vid=${bsta_vid#*:}

                                __repacd_map_vlanmon_debug "bsta_vid $bsta_vid"
                                if [ "$bsta_vid" -gt 0 ]; then
                                    return
                                fi
                            fi
                        done
                    fi
                    if [ "$map_enable_vlan_monitor" -gt 0 ]; then
                        __repacd_map_vlanmon_dump "Agent onboarded to TS disabled controller, removing vlan tags"
                        if [ -e "/sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/"$mld_del.$vid_lan"" ]; then
                             __repacd_delete_interface $NETWORK_TYPE_LAN $mld_del.$vid_lan
                        fi
                        map_ts_apply=0
                        map_ts_remove=1
                        map_primary_bsta_vid=0
                        vid_lan=0 vid_guest1=0 vid_guest2=0 vid_guest3=0
                        uci_set repacd MAPWiFiLink 'vid_lan' "$vid_lan"
                        uci set repacd.MAPConfig.VlanIDNwPrimary=$vid_lan
                        uci_set repacd MAPWiFiLink 'vid_guest1' "$vid_guest1"
                        uci set repacd.MAPConfig.VlanIDNwOne=$vid_guest1
                        uci_set repacd MAPWiFiLink 'vid_guest2' "$vid_guest2"
                        uci set repacd.MAPConfig.VlanIDNwTwo=$vid_guest2
                        uci_set repacd MAPWiFiLink 'vid_guest3' "$vid_guest3"
                        uci set repacd.MAPConfig.VlanIDNwThree=$vid_guest3
                        uci_commit repacd
                        if [ ! -e "/sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/"$mld_del"" ]; then
                            __repacd_map_vlanmon_debug "Interface ifaceBH $mld_del is not present in br-$NETWORK_TYPE_LAN, Add it"
                            brctl addif "br-$NETWORK_TYPE_LAN" $mld_del
                        fi
                    fi
                fi
            fi

            config_foreach __repacd_vlanmon_MBsta_setbh_network wifi-iface
            uci_commit wireless
        fi
    fi
}

__repacd_vlanmon_MBsta_setbh_network() {
    local config="$1"
    local iface mode device network mld
    local map_enable_vlan_monitor

    config_get iface "$config" ifname
    config_get mode "$config" mode
    config_get device "$config" device
    config_get network "$config" network
    config_get mld "$config" mld

    config_get_bool map_enable_vlan_monitor MAPConfig 'MapTrafficSeparationEnable' '0'

    if [ "$map_enable_vlan_monitor" -gt 0 ] && [ "$map_ts_remove" -ne 1 ]; then
        if [ "$mode" = "sta" ] && [ "$network" != "backhaul" ]; then
            uci_set wireless "$config" network "$NETWORK_TYPE_BACKHAUL"
            uci_commit wireless
            __repacd_map_vlanmon_debug "SET BHNW  for iface $iface"
        else
            return
        fi
    elif [ "$map_ts_remove" -gt 0 ]; then
        if [ "$mode" = "sta" ]; then
            uci_set wireless "$config" network "$NETWORK_TYPE_LAN"
            uci_commit wireless
        fi
    fi
}

repacd_map_check_backhaul_network()
{
    local config="$1"
    local mode MapBSSType disabled iface network mld briface

    config_get iface "$config" ifname
    config_get network "$config" network
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get MapBSSType "$config" MapBSSType '0'
    config_get mld "$config" mld ''

    if [ "$network" != "$NETWORK_TYPE_BACKHAUL" -o -z "$mld" ]; then
        return
    fi
    if [ -n "$iface" ] && [ "$disabled" -eq 0 -a "$mode" = "ap" -a "$MapBSSType" -eq 72 ]; then
        briface=`find /sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/ -name "$mld.*" 2>/dev/null`
        if [ -n "$briface" ]; then
            return
        fi
        uci_set wireless "$config" network "$NETWORK_TYPE_LAN"
        uci_commit wireless
        if [ ! -e "/sys/class/net/br-"$NETWORK_TYPE_LAN"/brif/"$mld"" ]; then
            brctl addif br-$NETWORK_TYPE_LAN $mld
        fi
    fi

}

__repacd_map_vlan_monitor() {
    __repacd_map_vlanmon_dump " [[ Enter VLAN Monitoring ]] "

    vid_lan=0 vid_guest1=0 vid_guest2=0 vid_guest3=0
    mlobsta_list=''

    #Using this variable to handle r1 cap r2 agent daisy onboarding
    local interop_handle
    config_load repacd
    config_get interop_handle MAPConfig "InterOpR1R2Handle" 1

    config_load wireless
    config_foreach __repacd_map_vlanmon_get_wlan_vlan_config wifi-iface

    if [ "$sta_vid" -eq 0 ] && __repacd_map_sta_connected; then
        if [ "$interop_handle" -eq 1 ]; then
            config_foreach repacd_map_check_backhaul_network wifi-iface
        fi
    fi

    if [ "$MBsta_onboarding" -eq 1 ] && [ "$MBsta_IgnoreBstaOnEthBH" -eq 1 ]; then
        config_foreach __repacd_vlanmon_remove_sta_mld wifi-iface
    fi
    if [ "$map_bsta_backhaul" -eq 1 ]; then
        if [ "$sta_vid" -eq 0 ]; then
            if [ "$maplite_enabled" -eq 1 -a "$map_my_version" -ge 3 ]; then
                __repacd_map_vlanmon_debug "Apply Vlan on AP Vaps"
            else
                if [ "$MBsta_onboarding" -eq 0 ]; then
                    map_ts_apply=0
                    map_ts_remove=1
                    map_primary_bsta_vid=0
                    vid_lan=0 vid_guest1=0 vid_guest2=0 vid_guest3=0
                fi
            fi
        fi
    elif [ "$vid_lan" -eq 0 -a "$vid_guest1" -eq 0 -a "$vid_guest2" -eq 0 -a \
                      "$vid_guest3" -eq 0 ]; then
        map_ts_apply=0
        map_ts_remove=1
    fi

    __repacd_map_vlanmon_dump "My Map Version: $map_my_version"
    __repacd_map_vlanmon_dump "Map upstream version: $upstream_version"
    __repacd_map_vlanmon_dump "VID Information"
    __repacd_map_vlanmon_dump "vid_lan:$vid_lan, vid_guest1: $vid_guest1"
    __repacd_map_vlanmon_dump "vid_guest2:$vid_guest2, vid_guest3: $vid_guest3"
    __repacd_map_vlanmon_dump "sta_assoc_vid:$sta_vid sta_primary_vid=$map_primary_bsta_vid"
    __repacd_map_vlanmon_dump "mldbsta_enabled:$mldbsta_enabled primary_mldbsta_iface=$primary_mldbsta_iface"
    [ "$map_ts_apply" -eq 1 ] && __repacd_map_vlanmon_debug "Traffic Separation Apply: $map_ts_apply"
    __repacd_map_vlanmon_dump "Traffic Separation Active: $map_ts_active"
    [ "$map_ts_remove" -eq 1 ] && __repacd_map_vlanmon_debug "Traffic Separation Remove: $map_ts_remove"
    __repacd_map_vlanmon_dump "Map Onboarding Status: $map_onboarding_done"

    if [ "$fastOnboarding" -eq 1 ] && [ "$map_bsta_backhaul" -eq 1 ]; then
        if __repacd_map_sta_connected; then
            __repacd_map_vlanmon_dump "STA connected. Proceed VLAN check"
        else
            # check if bsta / eth is VLAN configurd
            __repacd_map_vlanmon_check_backhaul_vlan_config
            __repacd_map_vlanmon_debug "Unknown STA bit rate . return"
            return
        fi
    fi

    # maplite settings
    if [ "$maplite_enabled" -eq 1 -a "$deviceMode" = "agt1" ]; then
        # Hawkeye platform has separate gmac for each switch port so gw_iface & switch_iface are same
        # For Maple+Spruce / Waikiki platform, Since switch is not present, skip programming wds
        __hyfi_get_switch_iface switch_iface eswitch_support switch_num switch_present
        if [ "$switch_present" -gt 0 ]; then
            __repacd_maplitemode_debug "Program WDS"
            __repacd_maplite_program_wds "bhBSS"
        fi
        sleep 2
        if [ "$map_ts_active" -eq 1 -a "$map_ts_remove" -eq 1 ]; then
            __repacd_maplitemode_debug "Dont remove"
            /etc/init.d/$MAP stop
            /etc/init.d/hyfi-bridging stop
            /etc/init.d/repacd stop
            __repacd_maplitemode_debug "Stopping HYFI"
            return
        fi
    fi

    # check if bsta / eth is VLAN configurd
    __repacd_map_vlanmon_check_backhaul_vlan_config

    # Check if VAPs are UP
    __repacd_map_vlanmon_fronthaul_monitor

    #remove unused vlan id from network
    if [ "$external_controller" -eq 1 ]; then
        config_load wireless
        config_foreach __repacd_delete_nw_vland_id wifi-iface
    fi

    if [ "$map_fh_bh_vap_up" -eq 0 ]; then
        __repacd_map_vlanmon_debug "AP Vaps not up"
        return
    fi

    if [ "$map_ts_active" -eq 1 ]; then
        uci_set repacd MAPConfig MapTrafficSeparationActive '1'
        [ "$map_ts_remove" -eq 1 ] && __repacd_map_vlanmon_debug "Traffic Separation Remove: $map_ts_remove"
        if [ "$map_ts_remove" -eq 1 ]; then
            map_ts_active=0
            uci_set repacd MAPConfig MapTrafficSeparationActive '0'
            __repacd_map_vlanmon_debug "Removing Traffic Separation Settings"
            __repacd_map_vlanmon_remove_vlan
        fi
        uci_commit repacd
    fi

    # check if backhaul BSS is VLAN configured
    __repacd_map_vlanmon_dump " [ Configure backhaul BSS with VLAN ] "
    config_load wireless
    if [ "$MBsta_onboarding" -ne 1 ]; then
        config_foreach __repacd_map_vlanmon_check_bh_bss_vlan_config wifi-iface
    elif [ "$MBsta_onboarding" -eq 1 ]; then
        if [ "$map_ts_remove" -eq 0 ]; then
            config_foreach __repacd_map_vlanmon_check_bh_bss_vlan_config wifi-iface
        fi
    fi

    __repacd_map_vlanmon_dump "___________________________________________________________________"

    if [ "$maplite_enabled" -eq 1 ]; then
        __repacd_map_vlanmon_network_monitor
    fi
}

# Send consecutive ARPs to the gateway and expect replies to confirm it is
# indeed reachable and there was not a false positive due to the system also
# performing an ARP at the same time.
# input: $1 - bridge: the name of the bridge to use to listen for a response
# input: $2 - gw_ip: the IP address of the gateway to attempt to reach
# input: $3 - gw_iface: the iface through which the gateway can be reached
# return: 0 if the gateway is reachable, otherwise non-zero
__repacd_map_arping_confirm_gateway() {
    local bridge=$1
    local gw_ip=$2
    local gw_iface=$3

    local replies
    replies=$(arping -f -c "$map_gw_reachable_confirm_attempts" \
        -w "$map_gw_reachable_confirm_attempts" -I "$gw_iface" -B "$bridge" "$gw_ip")
    replies=${replies#*Received }
    replies=${replies% response*}

    if [ "$replies" -ge "$map_gw_reachable_confirm_attempts" ]; then
        __repacd_map_vlanmon_debug \
            "GW IP ($gw_ip) reachable on bridge $bridge, interface $gw_iface via $replies replies"
        MAP_IS_GW_REACHABLE=1
        return 0
    else
        MAP_IS_GW_REACHABLE=0
        return 1
    fi
}

# Send pings to the gateway and expect replies to confirm it is reachable
# input: $1 - gw_ip: the IP address of the gateway to attempt to reach
# return: 0 if the gateway is reachable, otherwise non-zero
__repacd_map_ping_confirm_gateway() {
    local gw_ip=$1

    # Other modules still need to know about overall GW reachability
    if ping -W 2 "$gw_ip" -c1 > /dev/null; then
        __repacd_map_vlanmon_dump "GW ($gw_ip) reachable"
        MAP_IS_GW_REACHABLE=1
        return 0
    else
        MAP_IS_GW_REACHABLE=0
        return 1
    fi
}

# Check if AP is up
# return: 1 if the AP is up and running, otherwise 0
__repacd_is_ap_iface_up() {
    local ifname=$1
    local apBitRate=$(repacdcli $ifname get_bitrate)
    local roundOffBitRate=${apBitRate%.*}

    if [ -z "$ifname" ]; then
        __repacd_map_vlanmon_debug "Invalid iface $ifname"
        return 0
    fi

    if [ "$roundOffBitRate" -eq 0 ] || [ -z "$apBitRate" ]; then
        __repacd_map_vlanmon_dump "ap Bit Rate Invalid $ifname"
        return 0
    fi

    if [ "$roundOffBitRate" -gt 0 ]; then
        return 1
    fi

    return 1
}
# Check if STA  is connected
# return: 0 if the STA is connected, otherwise non-zero
__repacd_map_sta_connected_legacy_pf() {

    if [ -z "$sta_iface_backup" ]; then
        return 1
    fi

    local staBitRate=$(repacdcli $sta_iface_backup get_bitrate)
    local roundOffStaBitRate=${staBitRate%.*}

    if [ "$roundOffStaBitRate" -eq 0 ] || [ -z "$staBitRate" ]; then
        __repacd_map_vlanmon_debug "Sta Bit Rate Invalid"
        return 1
    fi

    if [ "$roundOffStaBitRate" -gt 0 ]; then
        return 0
    fi

    return 0
}

# Find secondary sta in MLO connection if any is connected
# to be checked

__repacd_map_find_second_sta() {
    local config="$1"
    config_get mode "$config" mode
    config_get iface "$config" ifname

    if [ "$mode" = "sta" ] && [ "$iface" != "$sta_iface" ]; then
        secondary_sta_iface="$iface"
        __repacd_map_vlanmon_debug "Second iface $iface"
        return
    fi
}

# Check if STA  is connected
# return: 0 if the STA is connected, otherwise non-zero
__repacd_map_sta_connected() {
    if [ "$map_bsta_backhaul" -eq 0 ]; then
        return 1
    fi

    if [ -z "$sta_iface" ]; then
        return 1
    fi
    __repacd_map_vlanmon_dump "MBsta_mlo_mode $MBsta_mlo_mode MBsta_non_mlo_mode $MBsta_non_mlo_mode vlanmon"

    if [ "$MBsta_non_mlo_mode" -eq 1 ] && [ "$MBsta_onboarding" -eq 1 ]; then
       if [ -z "$sta_iface" -o "$sta_iface" != "$MBsta_non_mlo_sta_connected_list" ]; then
           __repacd_map_vlanmon_debug "Updated STAIFACE to $MBsta_non_mlo_sta_connected_list"
       fi
        sta_iface="$MBsta_non_mlo_sta_connected_list"
    fi
    local staBitRate=$(repacdcli $sta_iface get_bitrate)
    local roundOffStaBitRate=${staBitRate%.*}

    if [ "$roundOffStaBitRate" -eq 0 ] || [ -z "$staBitRate" ]; then
        if [ "$mldbsta_enabled" -eq 1 ]; then
            config_load wireless
            config_foreach __repacd_map_find_second_sta wifi-iface
        if [ -n "$secondary_sta_iface" ]; then
                local staBitRatesecondary=$(repacdcli $secondary_sta_iface get_bitrate)
                local roundOffStaBitRatesecondary=${staBitRatesecondary%.*}

                if [ "$roundOffStaBitRatesecondary" -eq 0 ] || [ -z "$staBitRatesecondary" ]; then
                    __repacd_map_vlanmon_debug "Secondary Sta Bit Rate is also Invalid"
                    return 1
                fi
                if [ "$roundOffStaBitRatesecondary" -gt 0 ]; then
                    return 0
                fi
            fi
        else
            __repacd_map_vlanmon_debug "Sta Bit Rate Invalid"
            return 1
        fi
    fi

    if [ "$roundOffStaBitRate" -gt 0 ]; then
        return 0
    fi

    return 0
}

__repacd_map_vlanmon_backhaul_monitor() {
    local brlan_ip
    local gw_ip

    __repacd_map_vlanmon_dump " [[ Enter BackHaul Link Monitor ]] "

    brlan_ip=$(ifconfig br-$NETWORK_TYPE_LAN | awk -F ' *|:' '/inet addr/{print $4}')

    gw_ip=$(ip r | awk '/^def/{print $3}')
    if [ -z "$gw_ip" ]; then
        __repacd_map_vlanmon_debug "Could not resolve gw_ip; Check route"
        return
    fi
    if [ -z "$brlan_ip" ]; then
        __repacd_map_vlanmon_debug "Could not resolve br-lan self IP"
        return
    fi

    __repacd_map_ping_confirm_gateway $gw_ip

    if [ -n "$NETWORK_TYPE_LAN" -a "$map_bsta_backhaul" -eq 0 ]; then
        arp_entry=$(awk "/br-$NETWORK_TYPE_LAN/"' { print $4 }' /proc/net/arp)
        if [ -z "$arp_entry" ]; then
            __repacd_map_arping_confirm_gateway "br-$NETWORK_TYPE_LAN" $gw_ip $eth_iface
        fi
    fi

    __repacd_map_vlanmon_dump "___________________________________________________________________"
}

__repacd_map_vlanmon_check_vaps() {
    local config="$1"
    local iface disabled device hidden
    local isCacInProgress

    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get MapBSSType "$config" MapBSSType '0'
    config_get mld "$config" mld ''
    hidden=$(uci get wireless.$config.hidden 2>/dev/null)
    config_get device "$config" device

    # Manage AP VAPs
    if [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" = "ap" ] && [ "$hidden" != "1" ]; then
        if [ "$maplite_enabled" -eq 1 ]; then
            # Set ap_bridge to 0 to disable intraBSS transmission.
            # This will not loop back broadcast packet on bhBSS
            if [ $((MapBSSType & 0x40)) -eq 64 ]; then
                cfg80211tool $iface ap_bridge 0
            fi

            wsplcdPID=$(ps | grep wsplcd-lan.conf | grep -v grep | awk '{print$1}')
            if [ -n "$wsplcdPID" ]; then
                local is_vap_teardown_set=$(cfg80211tool_mesh $iface mapget_vapup | cut -d ':' -f2)
                if [ "$is_vap_teardown_set" -eq 0 ]; then
                    __repacd_map_vlanmon_debug " Iface $iface has vap up 0 from wsplcd"
                    return
                fi
            fi
        fi
        if [ "$maplite_enabled" -ne 1 ]; then
            local bitRate=$(repacdcli $iface get_bitrate)
            if [ "$bitRate" -eq 0 -o -z "$bitRate" ]; then
                local isAcsInProgress=$(cfg80211tool $iface get_acs_state)
                isAcsInProgress=${isAcsInProgress#*:}
                # bitRate will be 0 even when CAC is in progress. check cac state before we do
                # interface enable/disable
                local isCacInProgress=$(cfg80211tool $iface get_cac_state)
                isCacInProgress=${isCacInProgress#*:}
                # wait time for 2seconds after CAC timer is over to get bitrate set
                if [ "$CACstate" -eq 1 -a "$isCacInProgress" -ne 1 ]; then
                    sleep 2
                    bitRate=$(repacdcli $iface get_bitrate)
                    CACstate=$(cfg80211tool $iface get_cac_state)
                    CACstate=${CACstate#*:}
                fi
                if [ "$CACstate" -eq 0 ] && [ "$bitRate" -eq 0 -o -z "$bitRate" ]; then
                    isCacInProgress=$(cfg80211tool $iface get_cac_state)
                    isCacInProgress=${isCacInProgress#*:}
                    CACstate= $isCacInProgress
                    if [ "$isCacInProgress" -eq 0 ] && [ "$isAcsInProgress" -eq 0 ] && [ -z "$mld" ]; then
                        if [ $((MapBSSType & 0x10)) -eq 16 ]; then
                            __repacd_map_vlanmon_debug "Iface $iface is teardown vap. Don't bringup"
                        else
                            if [ "$map_my_version" -ge 6 ]; then
                                link_removed_list=$(uci get repacd.MAPConfig.LinksRemoved)
                                local skipLinkMonitor=0
                                for link in $link_removed_list; do
                                    if [ "$link" == "$iface" ]; then
                                        __repacd_map_vlanmon_debug " Skip monitor for: $iface"
                                        skipLinkMonitor=1
                                    fi
                                done
                                if [ "$skipLinkMonitor" -eq 1 ]; then
                                    return
                                fi
                            fi
                            __repacd_map_vlanmon_debug " Iface $iface has invalid Bit Rate $bitRate"
                            __repacd_map_vlanmon_debug "Cmd: Ifconfig $iface down Bringing down $iface"
                            ifconfig $iface down
                            hapd $iface disable
                            sleep 2
                            __repacd_map_vlanmon_debug "Cmd: Ifconfig $iface down Bringing up $iface"
                            ifconfig $iface up
                            hapd $iface enable
                            local interface="$(iw dev $iface info | grep ssid | awk -F " " '{print $2}')"
                            __repacd_map_vlanmon_debug "iface $iface is $interface"
                            if [ -z "$interface" ]; then
                                __repacd_echo "iface $iface has interface $interface"
		                        local device_iface="$device $iface"
                                __repacd_map_vlanmon_debug " Since ifconfig didn't work Doing wifi multi_up of $device_iface "
		                        wifi multi_up $device_iface
                                sleep 5
                            fi
                            map_fh_bh_vap_up=0
                        fi
                    elif [ "$isCacInProgress" -eq 0 ] && [ "$isAcsInProgress" -eq 0 ] && [ -n "$mld" ]; then
                        if [ $((MapBSSType & 0x10)) -eq 16 ]; then
                            __repacd_map_vlanmon_debug "Iface $iface is teardown vap. Don't bringup"
                        else
                            if [ "$map_my_version" -ge 6 ]; then
                                link_removed_list=$(uci get repacd.MAPConfig.LinksRemoved)
                                local skipLinkMonitor=0
                                for link in $link_removed_list; do
                                    if [ "$link" == "$iface" ]; then
                                        __repacd_map_vlanmon_debug " Skip monitor for: $iface"
                                        skipLinkMonitor=1
                                    fi
                                done
                                if [ "$skipLinkMonitor" -eq 1 ]; then
                                    return
                                fi
                            fi
                            ifconfig $iface down
                            hapd $iface disable
                            sleep 2
                            __repacd_map_vlanmon_debug "Bringing up $iface with mld $mld"
                            ifconfig $iface up
                            hapd $iface enable
                            local interface="$(iw dev $iface info | grep ssid | awk -F " " '{print $2}')"
                            __repacd_map_vlanmon_debug "iface $iface is $interface"
                            if [ -z "$interface" ]; then
                                __repacd_echo "iface $iface has interface $interface"
                                __repacd_map_vlanmon_debug " Doing wifi multi_up since Iface $iface has invalid Bit Rate $bitRate"
                                wifi multi_up $mld
                                sleep 5
                            fi
                        fi
                    fi
                fi
            else
                # we do not use shared VAPs for TS
                if [ "$MapBSSType" -ne 96 ]; then
                    # If we find MapBSSType configured . we can mark onboarding done
                    map_onboarding_done=1
                fi
            fi
        else
            if [ $((MapBSSType & 0x20)) -eq 32 ] || [ $((MapBSSType & 0x40)) -eq 64 ]; then
                local bitRate=$(repacdcli $iface get_bitrate)
                if [ "$bitRate" -eq 0 -o -z "$bitRate" ]; then
                    local isAcsInProgress=$(cfg80211tool $iface get_acs_state)
                    isAcsInProgress=${isAcsInProgress#*:}

                    # bitRate will be 0 even when CAC is in progress. check cac state before we do
                    # interface enable/disable
                    local isCacInProgress=$(cfg80211tool $iface get_cac_state)
                    isCacInProgress=${isCacInProgress#*:}
                    # wait time for 2seconds after CAC timer is over to get bitrate set
                    if [ "$CACstate" -eq 1 -a "$isCacInProgress" -ne 1 ]; then
                        sleep 2
                        bitRate=$(repacdcli $iface get_bitrate)
                        CACstate=$(cfg80211tool $iface get_cac_state)
                        CACstate=${CACstate#*:}
                    fi
                    if [ "$CACstate" -eq 0 ] && [ "$bitRate" -eq 0 -o -z "$bitRate" ]; then
                        isCacInProgress=$(cfg80211tool $iface get_cac_state)
                        isCacInProgress=${isCacInProgress#*:}
                        CACstate= $isCacInProgress
                        if [ "$isCacInProgress" -eq 0 ] && [ "$isAcsInProgress" -eq 0 ] && [ -z "$mld" ]; then
                            if [ "$map_my_version" -ge 6 ]; then
                                link_removed_list=$(uci get repacd.MAPConfig.LinksRemoved)
                                local skipLinkMonitor=0
                                for link in $link_removed_list; do
                                    if [ "$link" == "$iface" ]; then
                                        __repacd_map_vlanmon_debug " Skip monitor for: $iface"
                                        skipLinkMonitor=1
                                    fi
                                done
                                if [ "$skipLinkMonitor" -eq 1 ]; then
                                    return
                                fi
                            fi
                            __repacd_map_vlanmon_debug " Iface $iface has invalid Bit Rate $bitRate"
                            __repacd_map_vlanmon_debug "Cmd: Ifconfig $iface down Bringing down $iface"
                            if [ "$MAP_IS_GW_REACHABLE" -eq 1 ]; then
                                ifconfig $iface down
                                hapd $iface disable
                                sleep 2
                                __repacd_map_vlanmon_debug "Cmd: Ifconfig $iface up Bringing up $iface"
                                ifconfig $iface up
                                hapd $iface enable
                            fi
                            map_fh_bh_vap_up=0
                        elif [ "$isCacInProgress" -eq 0 ] && [ "$isAcsInProgress" -eq 0 ] && [ -n "$mld" ]; then
                            if [ "$map_my_version" -ge 6 ]; then
                                link_removed_list=$(uci get repacd.MAPConfig.LinksRemoved)
                                local skipLinkMonitor=0
                                for link in $link_removed_list; do
                                    if [ "$link" == "$iface" ]; then
                                        __repacd_map_vlanmon_debug " Skip monitor for: $iface"
                                        skipLinkMonitor=1
                                    fi
                                done
                                if [ "$skipLinkMonitor" -eq 1 ]; then
                                    return
                                fi
                            fi
                            __repacd_map_vlanmon_debug "Cmd:wifi multi_up $mld"
                            wifi multi_up $mld
                            sleep 5
                        fi
                    fi
                else
                    # we do not use shared VAPs for TS
                    if [ "$MapBSSType" -ne 96 ]; then
                        # If we find MapBSSType configured . we can mark onboarding done
                        map_onboarding_done=1
                    fi
                fi
            fi
        fi
    elif [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" = "ap" ] && [ "$hidden" == "1" ]; then
        # Only for IoT, Guest & 6G-Only vaps in satellite This will bring down Hidden vaps from vlanmon
        local network st_d
        config_get network "$config" network
        config_get st_d "$config" SteeringDisabled
        config_get virtualap "$config" virtualAp
        if [ "$network" != "backhaul" ] && [ "$virtualap" != "1" ] && [ "$external_controller" != "1" ]; then
            local bitRate=$(repacdcli $iface get_bitrate)
            if [ -n "$bitRate" ] && [ "$bitRate" != "0" ]; then
                __repacd_map_vlanmon_debug "Hidden set, Bringing down $iface"
                ifconfig $iface down
                hapd $iface disable
                uci_set repacd MAPWiFiLink 'hiddenvaps_down' 1
                uci_commit repacd
            fi
        fi
    elif [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" = "ap_smart_monitor" ] && [ "$hidden" != "1" ]; then
        if [ "$map_my_version" -ge 6 ]; then
            link_removed_list=$(uci get repacd.MAPConfig.LinksRemoved)
            local skipLinkMonitor=0
            for link in $link_removed_list; do
                if [ "$link" == "$iface" ]; then
                    __repacd_map_vlanmon_debug " Skip monitor for: $iface"
                    skipLinkMonitor=1
                fi
            done
            if [ "$skipLinkMonitor" -eq 1 ]; then
                return
            fi
        fi
        local bitRate=$(repacdcli $iface get_bitrate)
        if [ "$bitRate" -eq 0 -o -z "$bitRate" ]; then
            local isAcsInProgress=$(cfg80211tool $iface get_acs_state)
            isAcsInProgress=${isAcsInProgress#*:}
            # bitRate will be 0 even when CAC is in progress. check cac state before we do
            # interface enable/disable
            isCacInProgress=$(cfg80211tool $iface get_cac_state)
            isCacInProgress=${isCacInProgress#*:}
            # wait time for 2seconds after CAC timer is over to get bitrate set
            if [ "$CACstate" -eq 1 -a "$isCacInProgress" -ne 1 ]; then
                sleep 2
                bitRate=$(repacdcli $iface get_bitrate)
                CACstate=$(cfg80211tool $iface get_cac_state)
                CACstate=${CACstate#*:}
            fi
            if [ "$CACstate" -eq 0 ] && [ "$bitRate" -eq 0 -o -z "$bitRate" ]; then
                isCacInProgress=$(cfg80211tool $iface get_cac_state)
                isCacInProgress=${isCacInProgress#*:}
                CACstate= $isCacInProgress
                if [ "$isCacInProgress" -eq 0 ] && [ "$isAcsInProgress" -eq 0 ]; then
                    __repacd_map_vlanmon_debug " smart mon Iface $iface has invalid Bit Rate $bitRate"
                    __repacd_map_vlanmon_debug "Bringing down $iface"
                    ifconfig $iface down
                    hapd $iface disable
                    sleep 2
                    __repacd_map_vlanmon_debug "Bringing up $iface"
                    ifconfig $iface up
                    hapd $iface enable
                    map_fh_bh_vap_up=0
                fi
            fi
        fi
    fi
}

__repacd_map_vlanmon_fronthaul_monitor() {
    __repacd_map_vlanmon_dump " [[ Enter fronthaul Monitoring ]] "

    if [ "$map_ts_active" -eq 1 -a "$map_ts_remove" -eq 1 ]; then
        # there might be a race condition where VAPs are still up
        sleep 5
    fi

    # if backhaul STA wait for STA to connect
    if [ "$map_bsta_backhaul" -eq 1 ]; then
        if __repacd_map_sta_connected; then
            [ "$map_fh_bh_vap_up" -eq 0 ] && __repacd_map_vlanmon_debug "STA connected. Proceed FH check"
        else
            __repacd_map_vlanmon_debug "Unknown STA bit rate . return"
            return
        fi
    fi

    map_fh_bh_vap_up=1
    config_load ezmesh
    config_get_bool external_controller MultiAP 'ExternalController' '0'

    config_load wireless
    config_foreach __repacd_map_vlanmon_check_vaps wifi-iface

    __repacd_map_vlanmon_dump " front haul VAPs are ready: $map_fh_bh_vap_up"

    __repacd_map_vlanmon_dump "___________________________________________________________________"
}

__repacd_maplite_is_mld_attached() {
    local config=$1
    local slave='' in_bridge=''

    slave=$(cat /sys/class/net/$config/bonding/slaves)
    in_bridge=$(hyctl show | grep $config)
    if [ -z "$slave" -a -n "$in_bridge" ]; then
        __repacd_map_vlanmon_debug "MLD $config is not valid, remove from bridge"
        brctl delif br-$NETWORK_TYPE_LAN $config
    fi
}

__repacd_map_vlanmon_start_dependencies() {
    local hyctl_portType
    local hyctl_disabled
    config_load repacd
    config_get_bool map_ftauth MAPConfig 'EnableFTAuth' '0'

    #Check if HYD and WSPLCD are running
    wsplcdPID=$(pidof wsplcd)
    hydPID=$(pidof $MAP)

    config_get gwcon_mode repacd GatewayConnectedMode 'AP'

    if [ "$fastOnboarding" -eq 1 ] && [ "$MAP_IS_GW_REACHABLE" -eq 0 ] && [ "$map_ftauth" -eq 0 ]; then
        __repacd_map_vlanmon_debug "GW not reachable"
        if [ "$gwcon_mode" != "CAP" ]; then
            config_load wireless
            config_foreach __repacd_wifimon_set_root_distance wifi-iface
        fi
        if [ "$OnboardingDone" -eq 1 ]; then
            __repacd_map_vlanmon_debug "Onboarding done. No need to restart $MAP"
            return
        fi
        hyctl_disabled=$(hyctl show | grep br-$NETWORK_TYPE_LAN)
        if [ -n "$hyctl_disabled" ]; then
            __repacd_map_vlanmon_debug "Disable hyfi bridging"
            /etc/init.d/hyfi-bridging stop
        fi

        if [ -n "$wsplcdPID" ]; then
            __repacd_map_vlanmon_debug "Disable wsplcd"
            /etc/init.d/wsplcd stop
        fi

        if [ -n "$hydPID" ]; then
            __repacd_map_vlanmon_debug "Disable ezmesh"
            /etc/init.d/$MAP stop
        fi

        return
    fi

    if [ "$maplite_enabled" -eq 1 -a "$map_my_version" -ge 6 ]; then
        config_load wireless
        config_foreach __repacd_maplite_is_mld_attached wifi-mld
    fi

    # Read HYCTL OP
    if [ "$map_onboarding_done" -eq 1 ] || __repacd_map_sta_connected; then
        hyctl_portType=$(hyctl show | grep Unknown)
        if [ -n "$hyctl_portType" ]; then
            if [ "$fastOnboarding" -eq 1 ]; then
                # Delete temp file to read config again
                [ -f /tmp/mapTempIntfList ] && rm /tmp/mapTempIntfList
                [ -f /tmp/mapTempIntfListWsplcd ] && rm /tmp/mapTempIntfListWsplcd
            fi

            if [ "$map_bsta_backhaul" -eq 1 ] && __repacd_map_sta_connected; then
                __repacd_map_vlanmon_debug "Unknown Port Type WIFI BH . Restart ezmesh to resolve"
                /etc/init.d/hyfi-bridging start
                /etc/init.d/$MAP restart
            elif [  "$map_bsta_backhaul" -eq 0 ]; then
                __repacd_map_vlanmon_debug "Unknown Port Type ETH BH . Restart ezmesh to resolve"
                /etc/init.d/hyfi-bridging start
                /etc/init.d/$MAP restart
            fi
        fi
    fi

    config_load $MAP
    config_get_bool mapConfigServiceEnabled MAPConfigSettings 'EnableConfigService' '0'

    # Map Lite check to set flag to disable VLAN application from wsplcd
    # if it is already applied
    if [ "$maplite_enabled" -eq 1 -a "$maplite_restart_config" -eq 1 -a "$map_ts_active" -eq 1 ]; then
        if [ "$map_onboarding_done" -eq 1 ]; then
            __repacd_map_vlanmon_debug " Map Lite mode and TS Enabled. Dont apply TS from WSPLCD"
            uci_set wsplcd config Map2TSSetFromHYD '1'
            uci_commit wsplcd
            /etc/init.d/wsplcd restart
            maplite_restart_config=0
        fi
    fi

    if [ "$sta_config_changed" -eq 1 ]; then
        sta_config_changed=0
    fi

    if [ "$MAP_IS_GW_REACHABLE" -eq 1 ]; then
        if [ -z "$hydPID" -o "$config_changed" -eq 1 ]; then
            if [ "$mapConfigServiceEnabled" -eq 1 ]; then
                uci_set $MAP MAPConfigSettings 'EnableConfigService' 1
                /etc/init.d/$MAP restart
            fi
        fi

        if [ -z "$wsplcdPID" -o "$config_changed" -eq 1 ]; then
            if [ "$mapConfigServiceEnabled" -eq 0 ]; then
                /etc/init.d/wsplcd restart
            fi
        fi

        if [ "$mapConfigServiceEnabled" -eq 1 ] && [ "$gwcon_mode" != "CAP" ]; then
            uci_set wsplcd config 'HyFiSecurity' 0
            uci commit wsplcd
            /etc/init.d/wsplcd stop
        fi
    fi

    if [ -z "$hydPID" -o "$config_changed" -eq 1 ]; then
        /etc/init.d/hyfi-bridging start
        /etc/init.d/$MAP restart
    fi

    if [ "$config_changed" -eq 1 ]; then
        config_changed=0
    fi
}

# After adding VLAN check if bridges are in UP State . If not bring it UP
__repacd_map_vlanmon_bridge_monitor() {
    local brState brError
    local networkName

    __repacd_map_vlanmon_dump " [[ Enter Bridge Monitoring ]] "

    config_load repacd
    for i in Primary One Two Three; do
        config_get networkName MAPConfig "VlanNetwork"$i '0'
        if [ ! -e "/sys/class/net/br-"$networkName"" ]; then
            continue
        fi

        brState=$(cat /sys/class/net/br-$networkName/operstate)
        __repacd_map_vlanmon_dump "Bridge State for br-$networkName : $brState"
        if [ "$brState" != "up" -o -z "$brState" ]; then
            __repacd_map_vlanmon_debug "Bridge br-$networkName is down. Bringing back UP"
            ifconfig br-$networkName up
        fi
    done

    __repacd_map_vlanmon_dump "___________________________________________________________________"
}

__repacd_maplite_disable_unicast_flood() {
    local iface="$1"
    local unicast_flood

    if [ "$sta_vid" -gt 0 ]; then
        unicast_flood=$(cat /sys/class/net/br-$NETWORK_TYPE_LAN/brif/$iface.$vid_lan/unicast_flood)
        if [ "$unicast_flood" -eq 1 ]; then
            echo 0 > /sys/class/net/br-$NETWORK_TYPE_LAN/brif/$iface.$vid_lan/unicast_flood
        fi
    fi

    if [ -n "$NETWORK_TYPE_GUEST1" ]; then
        unicast_flood=$(cat /sys/class/net/br-$NETWORK_TYPE_GUEST1/brif/$iface.$vid_guest1/unicast_flood)
        if [ "$unicast_flood" -eq 1 ]; then
            echo 0 > /sys/class/net/br-$NETWORK_TYPE_GUEST1/brif/$iface.$vid_guest1/unicast_flood
        fi
    fi

    if [ -n "$NETWORK_TYPE_GUEST2" ]; then
        unicast_flood=$(cat /sys/class/net/br-$NETWORK_TYPE_GUEST2/brif/$iface.$vid_guest2/unicast_flood)
        if [ "$unicast_flood" -eq 1 ]; then
            echo 0 > /sys/class/net/br-$NETWORK_TYPE_GUEST2/brif/$iface.$vid_guest2/unicast_flood
        fi
    fi

    if [ -n "$NETWORK_TYPE_GUEST3" ]; then
        unicast_flood=$(cat /sys/class/net/br-$NETWORK_TYPE_GUEST3/brif/$iface.$vid_guest3/unicast_flood)
        if [ "$unicast_flood" -eq 1 ]; then
            echo 0 > /sys/class/net/br-$NETWORK_TYPE_GUEST3/brif/$iface.$vid_guest3/unicast_flood
        fi
    fi
}


repacd_map_vlanmon_init() {
    # First resolve the config parameters.
    primary_mldbsta_iface=''
    config_load repacd
    config_get_bool map_enable_vlan_monitor MAPConfig 'MapTrafficSeparationEnable' '0'
    config_get_bool map_single_r1r2_bh MAPConfig 'CombinedR1R2Backhaul' '0'
    config_get_bool maplite_enabled MAPConfig 'EnableLiteMode' '0'
    config_get enable_single_netdev MAPConfig 'EnableSingleNetdev' 0
    config_get enable_mlo MAPConfig 'EnableMLO' 0
    config_get enable_slo MAPConfig 'EnableSLO' 0
    config_get MBsta_onboarding MAPConfig 'MultibSTAOnboarding' '0'

    if [ "$enable_slo" -eq 1 -o "$enable_mlo" -eq 1 ]; then
        if [ "$maplite_enabled" -ne 1 ]; then
            enable_single_netdev=1
        fi
    fi
    __repacd_map_vlanmon_debug "Single Netdev Support: $enable_single_netdev"

    if [ "$maplite_enabled" -eq 1 ]; then
        config_load wsplcd
        uci_set wsplcd config Map2TSSetFromHYD '0'
        uci_commit wsplcd
        /etc/init.d/wsplcd restart
    fi

    if [ "$map_enable_vlan_monitor" -eq 0 ]; then
        return
    fi
    __repacd_map_vlanmon_debug "Map VLAN Monitor Init"

    config_load repacd
    config_load wsplcd
    config_get num_vlan_supported MAPConfig 'NumberOfVLANSupported' '0'
    uci_set wsplcd config 'NumberOfVLANSupported' "$num_vlan_supported"
    num_guest_vlan=$((num_vlan_supported-1))

    # Get BackHaul Name
    if [ "$num_vlan_supported" -gt 0 ]; then
        config_get networkName MAPConfig VlanNetworkBackHaul '0'
        NETWORK_TYPE_BACKHAUL=$networkName
        uci_set wsplcd config backhaul "$networkName"
        __repacd_map_vlanmon_debug "NETWORK_TYPE_BACKHAUL=$NETWORK_TYPE_BACKHAUL"
    fi

    # Get network names
    for i in Primary One Two Three; do
        if [ "$num_vlan_supported" -eq 0 ]; then
            break
        fi

        config_get networkName MAPConfig "VlanNetwork"$i '0'

        if [ "$i" = "Primary" ]; then
            NETWORK_TYPE_LAN=$networkName
            __repacd_map_vlanmon_debug "NETWORK_TYPE_LAN=$NETWORK_TYPE_LAN"
            uci_set wsplcd config bridge "$networkName"
            if [ "$maplite_enabled" -eq 1 ]; then
                brctl setageing br-$NETWORK_TYPE_LAN 1000
            fi
        elif [ "$i" = "One" ]; then
            NETWORK_TYPE_GUEST1=$networkName
            __repacd_map_vlanmon_debug "NETWORK_TYPE_GUEST1=$NETWORK_TYPE_GUEST1"
            uci_set wsplcd config bridge1 "$networkName"
            if [ "$maplite_enabled" -eq 1 ]; then
                brctl setageing br-$NETWORK_TYPE_GUEST1 1000
            fi
        elif [ "$i" = "Two" ]; then
            NETWORK_TYPE_GUEST2=$networkName
            __repacd_map_vlanmon_debug "NETWORK_TYPE_GUEST2=$NETWORK_TYPE_GUEST2"
            uci_set wsplcd config bridge2 "$networkName"
        elif [ "$i" = "Three" ]; then
            NETWORK_TYPE_GUEST3=$networkName
            __repacd_map_vlanmon_debug "NETWORK_TYPE_GUEST3=$NETWORK_TYPE_GUEST3"
            uci_set wsplcd config bridge3 "$networkName"
        fi

        num_vlan_supported=$((num_vlan_supported-1))
    done

    # Remove existing vlan config
    __repacd_map_vlanmon_remove_vlan

    uci_set repacd MAPConfig MapTrafficSeparationActive '0'
    map_ts_apply=0
    map_ts_active=0
    map_ts_remove=0
    uci_commit repacd
    uci_commit wsplcd
}
__repacd_map_vlanmon_wlan_network_monitor() {
    local config="$1"
    local iface network disabled device vlan_ifname vlan_ifname_id
    local ifname_brctl
    config_get iface "$config" ifname
    config_get network "$config" network
    config_get disabled "$config" disabled '0'

    if [ -z "$iface" -o "$disabled" -eq 1 ]; then
        return
    fi

    for i in Primary One Two Three; do
        config_get networkName MAPConfig "VlanNetwork"$i '0'

        vlan_ifname_id=$(brctl show br-"$networkName" | grep -cw "$iface" | awk '{print $1}')

        if [ "$vlan_ifname_id" -eq 0 ]; then
             continue
        fi

        if [ "$vlan_ifname_id" -gt 1 ] ;then
            __repacd_map_vlanmon_debug " dup present $iface Deleting "
            brctl delif br-$networkName $iface
        fi

        if [ "$network" = "backhaul" ]; then
             continue
        fi

        vlan_ifname_id=$(brctl show br-"$networkName" | grep -cw "$iface" | awk '{print $1}')
        if [ "$networkName" != "$network" -a "$vlan_ifname_id" -gt 0 ]; then
            brctl delif br-$networkName $iface
            __repacd_map_vlanmon_debug " changing bridge  $vlan_ifname $vlan_ifname_id Deleting "
            brctl addif br-$network $iface
        fi
    done
}

__repacd_map_vlanmon_network_monitor() {
    config_load wireless
    config_foreach __repacd_map_vlanmon_wlan_network_monitor wifi-iface
}

__vlanmon_map_daemon_check() {
    local action=$1
    local wsplcdPID hydPID
    local hyctl_disabled

    #Check if HYD and WSPLCD are running
    wsplcdPID=$(ps | grep wsplcd-lan.conf | grep -v grep | awk '{print$1}')
    hydPID=$(ps | grep $MAP-lan.conf | grep -v grep | awk '{print$1}')
    hyctl_disabled=$(hyctl show | grep br-lan)

    if [ "$action" = "stop" ]; then
        if [ -n "$hyctl_disabled" ]; then
            __repacd_map_vlanmon_debug "Disable hyfi bridging"
            /etc/init.d/hyfi-bridging stop
        fi

        if [ -n "$wsplcdPID" ]; then
            __repacd_map_vlanmon_debug "Disable wsplcd"
            /etc/init.d/wsplcd stop
        fi

        if [ -n "$hydPID" ]; then
            __repacd_map_vlanmon_debug "Disable ezmesh"
            /etc/init.d/$MAP stop
        fi
    fi
}

repacd_map_vlanmon_check() {
    # First resolve the config parameters.
    config_load repacd
    local onboarding_type;
    config_get_bool map_enable_vlan_monitor MAPConfig 'MapTrafficSeparationEnable' '0'
    config_get_bool map_enable_vlan_logs MAPConfig 'EnableMapTSLogs' '0'
    config_get map_my_version MAPConfig 'MapVersionEnabled'
    config_get deviceMode MAPConfig 'MapDeviceMode'
    config_get fastOnboarding MAPConfig 'MapFastOnboarding'
    config_get OnboardingDone MAPConfig 'OnboardingDone'
    config_get onboarding_type MAPConfig 'OnboardingType'
    config_get gateway_connected_mode repacd 'GatewayConnectedMode' 'AP'
    config_get cur_role repacd 'Role' 'NonCAP'

    if [ "$map_enable_vlan_monitor" -eq 0 ]; then
        return
    elif [ "$non_11be_radio_count" -eq 0 ] && ! __repacd_map_sta_connected &&\
              [ "$cur_role" == "NonCAP" ]; then
        return
    fi

    if [ "$fastOnboarding" -eq 1 ] && [ "$gateway_connected_mode" != "CAP" ] &&\
            [ "$OnboardingDone" -eq 0 ]; then
        if [ "$onboarding_type" == "dpp" ] && ! __repacd_map_sta_connected; then
            return
        elif [ "$onboarding_type" != "dpp" ]; then
            return
        fi
    fi

    __repacd_map_vlanmon_dump "Start Map Vlan Monitor"

    # VLAN monitoring
    __repacd_map_vlan_monitor
    if [ "$fastOnboarding" -eq 1 ]; then
        if [ "$map_ts_remove" -eq 0 ]; then
            if [ "$map_ts_active" -eq 0 ] || [ "$map_ts_apply" -eq 1 ]; then
                __repacd_map_vlanmon_debug "Wait for Traffic Sep to be active"

                # Delete temp file to read config again
                [ -f /tmp/mapTempIntfList ] && rm /tmp/mapTempIntfList
                [ -f /tmp/mapTempIntfListWsplcd ] && rm /tmp/mapTempIntfListWsplcd

                # Stop deamons until VLAN is configured . If not interface
                # list will be wrong and CAC will delay VLAN config
                __vlanmon_map_daemon_check "stop"
                return
            fi
        fi
    fi

    # Check if we are able to reach the gateway
    __repacd_map_vlanmon_backhaul_monitor

    # Check the state of the bridge
    __repacd_map_vlanmon_bridge_monitor

    # Restart Dependencies
    __repacd_map_vlanmon_start_dependencies

    # maplite settings once onboarding done
    if [ "$maplite_enabled" -eq 1 -a "$map_onboarding_done" -eq 1 ]; then
        __repacd_maplite_disable_unicast_flood $sta_iface
        __repacd_maplite_post_onboarding_setting
    fi

    __repacd_map_vlanmon_dump "####################################################################"
}
