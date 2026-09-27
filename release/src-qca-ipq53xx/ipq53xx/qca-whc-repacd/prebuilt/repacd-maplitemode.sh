#!/bin/sh
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

MAPLITEMODE_DEBUG_OUTOUT=0
chirp_count=0
search_hyd_restart=0
sta_iface=""
dpp_lite_sta_connector=0
bhBSS_lite=""
map_num_radio=0
config_get_bool ezmesh repacd 'Ezmesh' '0'
    if [ "$ezmesh" -eq 1 ]; then
        MAP='ezmesh'
    else
        MAP='hyd'
    fi

# Include to use vlan monitoring functionality
. /lib/functions/repacd-vlanmon-map.sh
. /lib/functions/repacd-wifimon-map.sh

# Emit a message at debug level.
# input: $1 - the message to log
__repacd_maplitemode_debug() {
    local stderr=''
    if [ "$MAPLITEMODE_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.maplitemode -p user.debug "$1"
}

__repacd_maplite_chirp() {
    local mod_value
    local staBitRate=$(repacdcli $sta_iface get_bitrate)
    local connectorFound

    if [ "$staBitRate" -eq 0 -o -z "$staBitRate" ]; then
        __repacd_maplitemode_debug "Sta Bit Rate Invalid"
        search_hyd_restart=0
        if [ "$chirp_count" -eq 0 ]; then
            wpa_cli -p /var/run/wpa_supplicant-$sta_iface dpp_configurator_remove 1
            wpa_cli -p /var/run/wpa_supplicant-$sta_iface dpp_bootstrap_gen type=qrcode
            wpa_cli -p /var/run/wpa_supplicant-$sta_iface dpp_bootstrap_info 1
            wpa_cli -p /var/run/wpa_supplicant-$sta_iface dpp_bootstrap_get_uri 1
        fi

        chirp_count=$((chirp_count + 1))
        mod_value=$((chirp_count % 8))

        # Send 2 chirps quickly
        if [ "$chirp_count" -eq 1 -o "$chirp_count" -eq 2 ]; then
            wpa_cli -p /var/run/wpa_supplicant-$sta_iface dpp_stop_listen
            wpa_cli -p /var/run/wpa_supplicant-$sta_iface dpp_chirp own=1
        fi

        if [ "$mod_value" -ne 0 ]; then
           return
        fi

        connectorFound=$(cat "/tmp/map_sta_info.tmp" | grep DPP_STA_CONNECTOR)
        __repacd_maplitemode_debug "$mod_value"
        __repacd_maplitemode_debug "Connector found file : $connectorFound"
        __repacd_maplitemode_debug "Connector found UCI : $dpp_lite_sta_connector"

        if [ -n "$connectorFound" ]; then
            __repacd_maplitemode_debug "Conf Object received . Dont chirp"
           return
        fi

        if [ "$dpp_lite_sta_connector" -ne 0 ]; then
            __repacd_maplitemode_debug "Conf Object received . Dont chirp"
           return
        fi

        __repacd_maplitemode_debug "Start Chirping on $sta_iface , count $chirp_count"
        wpa_cli -p /var/run/wpa_supplicant-$sta_iface dpp_stop_listen
        wpa_cli -p /var/run/wpa_supplicant-$sta_iface dpp_chirp own=1
    else
        __repacd_maplitemode_debug "Valid STA BitRate $staBitRate"
        dpp_lite_sta_connector=0
        if [ "$search_hyd_restart" -eq 0 ]; then
            search_hyd_restart=1
            __repacd_maplitemode_debug "Restart HYD to send search"
            /etc/init.d/$MAP restart
        fi
    fi
}

__repacd_maplite_get_sta_iface() {
    local config="$1"
    local iface network disabled device

    config_get iface "$config" ifname
    config_get network "$config" network
    config_get disabled "$config" disabled '0'
    config_get mapVlanID "$config" mapVlanID '0'
    config_get mode "$config" mode
    config_get MapBSSType "$config" MapBSSType '0'

    if [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" = "sta" ]; then
        sta_iface=$iface
        config_get dpp_lite_sta_connector "$config" dpp_connector
    fi
}

__repacd_maplite_program_wds() {
    local intfType=$1
    local port_lan port_lan1 port
    local nexthop mac_addr
    local i add_entry local_entry
    local intf

    __repacd_maplitemode_debug "Maplite program wds for intf $intfType"
    if [ "$intfType" == "bhBSS" ]; then
        __repacd_maplitemode_debug "Maplite program wds for BH $bhBSS_lite"
        intf=$bhBSS_lite
        nexthop=$(wlanconfig $bhBSS_lite list | grep -o -E \
                                                     '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}')

    elif [ "$intfType" == "bSTA" ]; then
        __repacd_maplitemode_debug "Maplite program wds for bSTA $sta_iface"
        intf=$sta_iface
        nexthop=$(cat /sys/class/net/$sta_iface/address)
    fi

    if [ -n "$intf" ]; then
        port_lan=$(brctl showstp br-lan | grep $intf | awk -F "[()]" '{print $2}' )
        port_lan1=$(brctl showstp br-lan1 | grep $intf | awk -F "[()]" '{print $2}' )

        __repacd_maplitemode_debug "port_lan=$port_lan , port_lan1=$port_lan1"

        brctl showmacs br-lan | \
        while read i
        do
            port=$(echo $i | cut -d ' ' -f1)
            local_entry=$(echo $i | cut -d ' ' -f3)
            if [ "$port" == "$port_lan" -a "$local_entry" = "no" ]; then
                mac_addr=`echo $i | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}'`
                add_entry=$(wlanconfig $intf hmwds read-table | grep $mac_addr)
                if [ "$nexthop" = "$mac_addr" ]; then
                    continue
                fi

                if [ -z "$add_entry" ]; then
                    __repacd_maplitemode_debug "Add hmwds entry $intf $nexthop $mac_addr"
                    wlanconfig $intf hmwds add-addr $nexthop $mac_addr
                    sleep 1
                fi
            fi
        done

        brctl showmacs br-lan1 | \
        while read i
        do
            port=$(echo $i | cut -d ' ' -f1)
            local_entry=$(echo $i | cut -d ' ' -f3)
            if [ "$port" == "$port_lan1" -a "$local_entry" = "no" ]; then
                mac_addr=`echo $i | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}'`
                add_entry=$(wlanconfig $intf hmwds read-table | grep $mac_addr)
                if [ -z "$add_entry" ]; then
                    __repacd_maplitemode_debug "Add hmwds entry $intf $nexthop $mac_addr"
                    wlanconfig $intf hmwds add-addr $nexthop $mac_addr
                    sleep 1
                fi
            fi
        done
    fi
}

__repacd_maplite_program_peer_isolation() {
    local port_lan port_lan1 port
    local dut_stamac mac_addr
    local i add_entry local_entry

    __repacd_maplitemode_debug "Maplite program peer isolation for sta $sta_iface"

    if [ -n "$sta_iface" ]; then
        port_lan=$(brctl showstp br-lan | grep $sta_iface | awk -F "[()]" '{print $2}' )
        __repacd_maplitemode_debug "port_lan=$port_lan"

        brctl showmacs br-lan | \
        while read i
        do
            port=$(echo $i | cut -d ' ' -f1)
            local_entry=$(echo $i | cut -d ' ' -f3)
            if [ "$port" == "$port_lan" -a "$local_entry" = "no" ]; then
                mac_addr=`echo $i | grep -o -E '([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}'`
                add_entry=$(wlanconfig $sta_iface peer_isolation list | grep $mac_addr)

                if [ -z "$add_entry" ]; then
                    __repacd_maplitemode_debug "Add peer isolation entry $mac_addr"
                    wlanconfig $sta_iface peer_isolation add $mac_addr
                    sleep 1
                fi
            fi
        done
    fi
}

__repacd_maplite_get_bhBSS() {
    local config="$1"
    local iface network disabled device MapBSSType

    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get MapBSSType "$config" MapBSSType '0'

    if [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" = "ap" ]; then
        if [ $((MapBSSType & 0x40)) -eq 64 ]; then
            bhBSS_lite=$iface
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

repacd_config_set_channel() {
    local iface="$1"
    local channelSet
    local channelCurrent
    local currentBand
    local bitRate
    local radioIdx=$(echo $iface | cut -c 4)

    if [ -n "$sta_iface" ]; then
        bitRate=$(repacdcli $sta_iface get_bitrate)
        if [ "$bitRate" -eq 0 ] || [ -z "$bitRate" ]; then
            __repacd_maplitemode_debug "Invalid STA Bit rate. Dont set channel"
            return
        fi
    fi

    bitRate=$(repacdcli $iface get_bitrate)
    if [ "$bitRate" -eq 0 ] || [ -z "$bitRate" ]; then
        return
    fi

    channelSet=$(uci show wireless | grep wifi$radioIdx | grep channel | cut -d '=' -f2)
    channelCurrent=$(cfg80211tool wifi$radioIdx g_oper_reg_info | awk -F "=" '{print $2}' | awk -F "," '{print $1}')
    currentBand=$(cfg80211tool wifi$radioIdx g_oper_reg_info | awk -F "=" '{print $4}' | cut -c-1)
    eval channelSet=$channelSet

    __repacd_maplitemode_debug "Set channel $channelSet on wifi$radioIdx"
    __repacd_maplitemode_debug "Current channel $channelCurrent on wifi$radioIdx with band:$currentBand"
    if [ "$channelSet" -ne "$channelCurrent" ]; then
        if [ "$currentBand" -eq "2" ]; then
            cfg80211tool $iface channel $channelSet --band 1
        elif [ "$currentBand" -eq "5" ]; then
            cfg80211tool $iface channel $channelSet --band 2
        else #currentBand" -eq "6"
            cfg80211tool $iface channel $channelSet --band 3
        fi
    fi
}

repacd_maplite_post_onboard_setting() {
    local config="$1"
    local iface network disabled device

    config_get iface "$config" ifname
    config_get disabled "$config" disabled '0'
    config_get mode "$config" mode
    config_get MapBSSType "$config" MapBSSType '0'
    config_get map_version MAPConfig 'MapVersionEnabled' '0'

    # Manage AP VAPs
    if [ -n "$iface" -a "$disabled" -eq 0 -a "$mode" = "ap" ]; then
        if [ $((MapBSSType & 0x20)) -eq 32 ]; then
            repacd_config_set_channel $iface
        elif [ "$MapBSSType" -eq 0 ]; then
            __repacd_maplitemode_debug "tear down $iface"
            ifconfig $iface down
        fi
    fi
}

__repacd_maplite_post_onboarding_setting() {
    __repacd_map_vlanmon_debug " [[ Map Lite Post onboarding Setting ]] "

    local ageingTime=$(brctl showstp br-lan | grep ageing | awk '{print $3}')
    local roundOffAgeingTime=${ageingTime%.*}
    if [ "$roundOffAgeingTime" -ne 1000 ]; then
        brctl setageing br-lan 1000
        brctl setageing br-lan1 1000
    fi

    # Hawkeye platform has separate gmac for each switch port so gw_iface & switch_iface are same
    # For Maple+Spruce / Waikiki platform, Since switch is not present, skip programming wds
    __hyfi_get_switch_iface switch_iface eswitch_support switch_num switch_present

    if [ "$switch_present" -gt 0 ]; then
        __repacd_maplite_program_wds "bSTA"
    fi

    config_load wireless
    config_foreach repacd_maplite_post_onboard_setting wifi-iface
}

repacd_maplitemode_init() {
    # First resolve the config parameters.
    config_load repacd
    config_get_bool enable_mlo MAPConfig 'EnableMLO' '0'
    config_get map_version MAPConfig 'MapVersionEnabled' '0'

    __repacd_maplitemode_debug "Map Lite Mode Init"
    repacd_map_vlanmon_init
    if [ "$map_version" -ge 6 -a "$enable_mlo" -eq 1 ]; then
        repacd_wifimon_init
    fi
    chirp_count=0

    # Get number of radio
    config_load wireless
    config_foreach __repacd_config_get_num_radio wifi-device
}

repacd_maplitemode_check() {
    local network=$1
    local restartWifi bh_type onboarding_type
    local bsta_ssid='' ap_ssid=''
    local mld_ssid
    local curStaIdx

    # First resolve the config parameters.
    config_load repacd
    config_get_bool restartWifi MAPConfig 'restartWifiDPP' '0'
    config_get bh_type MAPConfig 'MapBackaulType'
    config_get onboarding_type MAPConfig 'OnboardingType'
    config_get_bool enable_mlo MAPConfig 'EnableMLO' '0'
    config_get map_version MAPConfig 'MapVersionEnabled' '0'

    config_load wireless
    config_foreach __repacd_maplite_get_bhBSS wifi-iface
    config_foreach __repacd_wifimon_get_bsta_ssid wifi-iface \
            bsta_ssid
    config_foreach __repacd_wifimon_is_ap_configured wifi-iface \
            ap_ssid

    __repacd_maplitemode_debug "bsta_ssid configured: $bsta_ssid"
    __repacd_maplitemode_debug "ap_ssid configured: $ap_ssid"

    __repacd_maplitemode_debug "Map Lite Mode Checks"
    repacd_map_vlanmon_check

    if [ "$map_version" -ge 6 -a "$enable_mlo" -eq 1 ]; then
        if [ -n "$bsta_ssid" ] && [ -n "$ap_ssid" ]; then
            if [ "$enable_mlo" -eq 1 ]; then
                __repacd_start_ping
                new_mode=''
                __repacd_wifimon_measure_link "$network" new_mode
                if [ "$new_mode" = "$WIFIMON_STATE_RE_SWITCH_MLO_BSTA" ]; then
                    __repacd_maplitemode_debug "Restarting for MLO switch"
                    eval "$2=$WIFIMON_STATE_RE_SWITCH_MLO_BSTA"
                    return
                elif [ "$new_mode" = "$WIFIMON_STATE_RE_SWITCH_BSTA" ]; then
                    __repacd_maplitemode_debug "Restarting for SLO switch"
                    eval "$2=$WIFIMON_STATE_RE_SWITCH_BSTA"
                    return
                fi
            fi
        fi
    fi

    __repacd_maplite_program_peer_isolation

    if [ "$bh_type" = "wifi" -a "$onboarding_type" = "dpp" ]; then
        config_load wireless
        config_foreach __repacd_maplite_get_sta_iface wifi-iface

        __repacd_maplite_chirp
    fi

    if [ "$restartWifi" -eq 1 ]; then
        uci set repacd.MAPConfig.restartWifiDPP='0'
        uci commit repacd

        wifi load
    fi
}
