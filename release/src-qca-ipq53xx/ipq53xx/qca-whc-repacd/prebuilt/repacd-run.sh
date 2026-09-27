#!/bin/sh
# Copyright (c) 2015-2018 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
# 2015-2016 Qualcomm Atheros, Inc.
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.

REPACD_DEBUG_OUTOUT=0

. /lib/functions/repacd-lp.sh
. /lib/functions/repacd-gwmon.sh
. /lib/functions/repacd-led.sh

GWMON_DEBUG_OUTOUT=$REPACD_DEBUG_OUTOUT

cur_role='' managed_network=''
link_check_delay=''
restart_wifi=0
traffic_separation_enabl=0
traffic_separation_activ=0
backhaul_network=''
eth_mon_enabled=''

sta_iface_backup=''
radioid=''
radioid_backup=''
radio_device_backup=''
mld_down_list=''
base_mld_mac_addr=''
non_ap_mld=''
enable_config_service=0
create_partner_bsta=0

sta_iface=''
sta_vid=0
direct_bsta_switch=0
# MapRev2 Traffic Separation Config
map_ts_enabled=0
map_backhaul_nw=''
num_vlan_configured=0

log_level="INFO"

# MAP DPP status
map_onboarding_type=''

__repacd_info() {
    local stderr=''
    if [ "$REPACD_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd -p user.info "$1"
}

__repacd_dump() {
    local stderr=''

    [ "$log_level" != "DUMP" ] && return

    if [ "$REPACD_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd -p user.info "$1"
}

__kill_unread_pipe_msg()
{
    local PID;

    while [ 1 ];
    do
        PID=$(ps | grep cloning_done | grep -v grep | awk '{print$1}')
        __repacd_info "kill unread msg pipe, PID:$PID"
        if [ -z $PID ]; then
           break
        fi
        kill -9 $PID
    done
}

__repacd_restart() {
    local __mode="$1"

    __kill_unread_pipe_msg
    __repacd_info "repacd: restart in $__mode mode"

    /etc/init.d/repacd "restart_in_${__mode}_mode"
    exit 0
}

__repacd_update_mode() {
    local new_mode=$1
    if [ "$new_mode" -eq "$GWMON_MODE_CAP" ]; then
        __repacd_info "Restarting in CAP mode"
        __repacd_restart 'cap'
    elif [ "$new_mode" -eq "$GWMON_MODE_NON_CAP" ]; then
        if [ "$alg_set" = "map" ]; then
            repacd_wifimon_config_bsta "${managed_network}"
        fi

        __repacd_info "Restarting in NonCAP mode"
        __repacd_restart 'noncap'
    fi
}

__repacd_wifimon_init() {

    local map_ts_active=0
    config_load repacd
    config_get map_ts_active MAPConfig 'MapTrafficSeparationActive' '0'

    if [ "$traffic_separation_enabl" -gt 0 ] && \
       [ "$traffic_separation_activ" -gt 0 ]; then
        repacd_wifimon_init "$backhaul_network" "$current_re_mode" "$current_re_submode" "$autoconf_restart" \
                            new_state new_re_mode new_re_submode
    elif [ "$map_ts_enabled" -gt 0 ] && \
             [ "$map_ts_active" -gt 0 ]; then
        repacd_wifimon_init "$backhaul_network" "$current_re_mode" "$current_re_submode" "$autoconf_restart" \
                            new_state new_re_mode new_re_submode
    else
        repacd_wifimon_init $managed_network "$current_re_mode" "$current_re_submode" "$autoconf_restart" \
                            new_state new_re_mode new_re_submode
    fi

}

config_load repacd
config_get log_level MAPConfig 'MapLogLevel'
config_get managed_network repacd 'ManagedNetwork' 'lan'
config_get cur_role repacd 'Role' 'NonCAP'
config_get link_check_delay repacd 'LinkCheckDelay' '2'
config_get traffic_separation_enabl repacd TrafficSeparationEnabled '0'
config_get traffic_separation_activ repacd TrafficSeparationActive '0'
config_get backhaul_network repacd NetworkBackhaul 'backhaul'
config_get eth_mon_enabled repacd 'EnableEthernetMonitoring' '0'
config_get map_fast_onboarding MAPConfig 'MapFastOnboarding' '0'
config_get onboarding_done MAPConfig 'OnboardingDone' '0'
config_get base_mld_mac_addr MAPConfig 'mld_mac_addr' ''
config_get enable_slo MAPConfig 'EnableSLO' 0
config_get enable_fastcloning MAPConfig 'EnableCloningOptimization' '0'
# Config required for EasyMesh Rev2 Traffic Separation
config_get map_ts_enabled MAPConfig 'MapTrafficSeparationEnable' '0'
config_get map_backhaul_nw MAPConfig 'VlanNetworkBackHaul' ''
config_get gwcon_mode repacd GatewayConnectedMode 'AP'
config_get num_vlan_configured MAPConfig 'NumberOfVLANSupported' '0'



#R3 config
config_get map_onboarding_type MAPConfig 'OnboardingType'

if [ "$#" -lt 5 ]; then
    echo -n "Usage: $0 <alg_set> <start_role> <config RE mode> "
    echo "<current RE mode> <current RE submode> [autoconf]"
    exit 1
fi

__repacd_get_non_ap_mld() {
    local config="$1"
    local role_to_match="$2"
    local role

    config_get role "$config" role
    if [ "$role" == "$role_to_match" ]; then
        eval "$3=$config"
    fi

}
__repacd_get_default_mld() {
    local config="$1"
    local mld mode
    config_get mld "$config" mld ''
    config_get mode "$config" mode ''

    if [ -n "$mld" -a "$mode" != "sta" ]; then
        mld_down_list="$mld_down_list $mld"
    fi
}

__repacd_is_firewall_configured_ts() {
   local guest_network=$1
   local firewalls nw_intf

   firewalls=$(uci show firewall | grep zone | grep network)

   for fw in $firewalls; do
        nw_intf=$(echo $fw | cut -d "=" -f 2 | cut -d "'" -f 2)

        if [ "$nw_intf" = "$guest_network" ]; then
            eval $2=1
            eval $3=$(echo $fw | cut -d "[" -f 2 | cut -d "]" -f 1)
            break
        fi
    done
}


__repacd_firewall_addition() {
    local zone
    local guest_network fw_cfg_changed=0
    local rule_available=0
    local rule_index=0
    #Firewall rule config is needed only for Guest NW
    local num_vlan_count=2
    local traffic_separation_active=$1

    for i in One Two Three; do
        rule_available=0
        rule_index=0
        config_get guest_network MAPConfig "VlanNetwork"$i '0'
        __repacd_is_firewall_configured_ts $guest_network rule_available rule_index

        if [ "$rule_available" -eq 0 ] && [ "$traffic_separation_active" -eq 1 ] && [ "$num_vlan_configured" -ge "$num_vlan_count" ]; then
            __repacd_info "Adding rule for guest network : $guest_network"
            zone=$(uci add firewall zone)
            uci_set firewall "$zone" name "$guest_network"
            uci add_list "firewall.$zone.network=$guest_network"
            uci_set firewall "$zone" input 'ACCEPT'
            uci_set firewall "$zone" output 'ACCEPT'
            uci_set firewall "$zone" forward 'ACCEPT'
            uci_commit firewall
            fw_cfg_changed=1
        elif [ "$rule_available" -eq 1 ] && [ "$traffic_separation_active" -eq 1 ] && [ "$num_vlan_configured" -lt "$num_vlan_count" ]; then
            __repacd_info "Deleting rule for guest network : $guest_network"
            uci delete firewall.@zone[$rule_index]
            uci commit firewall
            fw_cfg_changed=1
        elif [ "$rule_available" -eq 1 ] && [ "$traffic_separation_active" -eq 0 ]; then
            __repacd_info "Deleting rule for guest network: $guest_network"
            uci delete firewall.@zone[$rule_index]
            uci commit firewall
            fw_cfg_changed=1
        fi
        num_vlan_count=$((num_vlan_count+1))
    done
    [ "$fw_cfg_changed" -eq 1 ] && /etc/init.d/firewall restart
}

if [ "$enable_slo" -eq 1 -a -z "$base_mld_mac_addr" ]; then
    config_load wireless
    config_foreach __repacd_get_default_mld wifi-iface
    config_foreach __repacd_get_non_ap_mld wifi-mld "Non-AP" non_ap_mld
fi

alg_set=$1
start_role=$2
config_rep_mode=$3
current_re_mode=$4
current_re_submode=$5
re_mode_change=0
fallback_to_nonDPP=''

if [ "$alg_set" = "son" ]; then
    . /lib/functions/repacd-wifimon.sh
    . /lib/functions/repacd-ethmon.sh
    . /lib/functions/repacd-netdet.sh
    . /lib/functions/repacd-backhaulmgr.sh
    . /lib/functions/repacd-plcmon.sh
    . /lib/functions/repacd-fronthaulmgr.sh
elif [ "$alg_set" = "map" ]; then
    . /lib/functions/repacd-wifimon-map.sh
    . /lib/functions/repacd-fronthaulmgr.sh
    . /lib/functions/repacd-vlanmon-map.sh
elif [ "$alg_set" = "maplite" ]; then
    . /lib/functions/repacd-maplitemode.sh
fi

# Clean up the background ping and related logic when being terminated
# by the init system.
trap 'repacd_wifimon_fini; repacd_led_set_states Reset; exit 0' SIGTERM

__repacd_info "Starting: Algorithm set=$alg_set"
__repacd_info "Starting: ConfiguredRole=$cur_role StartRole=$start_role"
__repacd_info "Starting: ConfigREMode=$config_rep_mode CurrentREMode=$current_re_mode CurrentRESubMode=$current_re_submode"

new_mode=
__gwmon_init $cur_role "$start_role" $managed_network
new_mode=$?
if [ "$eth_mon_enabled" -eq 0 ] || [ ${new_mode} -ne "$GWMON_MODE_NO_CHANGE" ]; then
    __repacd_update_mode $new_mode
fi

cur_state='' new_state=''
new_re_mode=$current_re_mode new_re_submode=$current_re_submode
autoconf_restart=''

# If the start was actually a restart triggered by automatic configuration
# logic (eg. mode or role switching), note that here so it can influence the
# LED states.
if [ -n "$6" ]; then
    __repacd_info "Startup triggered by auto-config change"
    autoconf_restart=1
else
    autoconf_restart=0
fi

if [ ! "$eth_mon_enabled" -eq 0 ]; then
    repacd_lp_init
    repacd_netdet_init
fi

# Initialise Map VLAN monitoring logic
if [ "$alg_set" = "map" ]; then
    repacd_map_vlanmon_init
fi

# Initialise Wi-Fi monitoring logic
__repacd_wifimon_init

# Since the Wi-Fi monitoring process does nothing when in CAP mode, force
# the state to one that indicates we are operating in CAP mode.
if [ "$cur_role" = 'CAP' ]; then
    new_state='InCAPMode'
else
    if [ "$alg_set" = "son" ]; then
        # Initialise Backhaul Manager logic for REs
        repacd_backhaulmgrmon_init
        repacd_plcmon_init
    fi

    if [ "$alg_set" != "maplite" ]; then
        # This is valid in both SON and MAP modes
        repacd_fronthaulmgrmon_init
    fi

    if [ "$alg_set" = "maplite" ]; then
        repacd_maplitemode_init
    fi
fi

if [ -n "$new_state" ]; then
    __repacd_info "Setting initial LED states to $new_state"
    repacd_led_set_states $new_state
    cur_state=$new_state
else
    __repacd_info "Failed to resolve STA interface; will attempt periodically"
fi

#adding dummy Vlan upfront to improve cloning time
if [ "$onboarding_done" -eq 0 -a "$map_onboarding_type" == "dpp" -a "$map_ts_enabled" -eq 1 ]; then
    config_load wireless
    config_foreach __add_primary_vlan_to_BH_AP wifi-iface
fi

# Loop forever (unless we are killed with SIGTERM which is handled above).
while true; do
    if [ "$alg_set" = "maplite" ]; then
        new_mode=''
        repacd_maplitemode_check $managed_network new_mode
        if [ "$new_mode" = "$WIFIMON_STATE_RE_SWITCH_MLO_BSTA" ] || \
           [ "$new_mode" = "$WIFIMON_STATE_RE_SWITCH_BSTA" ]; then
            __repacd_info "Restarting for MLO switch"
            wifi multi_down mld0
            sleep 2
            wifi multi_up mld0
        fi
        # Re-check the link conditions in a few seconds.
        sleep $link_check_delay
        continue
    fi

    if [ "$alg_set" = "map" ]; then
        __gwmon_check_wifi
    fi

    __gwmon_check

    new_mode=$?
    __repacd_update_mode $new_mode

    __repacd_dump "REPACD RUN cur_state $cur_state sta_iface_24g $sta_iface_24g
                sta_iface_5g $sta_iface_5g sta_iface_6g $sta_iface_6g
                sta_iface_5gl $sta_iface_5gl sta_iface_6gl $sta_iface_6gl
                staifacebackup $sta_iface_backup"

    if [ "$MBsta_onboarding" -ne 1 ]; then
        if [ -n "$sta_iface_5g" ]; then
            sta_iface_backup="$sta_iface_5g"
        elif [ -n "$sta_iface_24g" ]; then
            sta_iface_backup="$sta_iface_24g"
        fi

        if [ -n "$sta_iface_backup" ]; then
            radioid_backup=$(echo $sta_iface_backup | cut -c 4)
            radio_device_backup="wifi$radioid_backup"
            config_load wireless
            config_foreach __repacd_wifimon_determine_device_band wifi-iface \
            "$radio_device_backup"
        fi
    else
        sta_iface_backup="MBsta_onboarding"
    fi

    if [ -n "$cur_state" ]; then
        if [ "$direct_bsta_switch" -eq 0 ]; then
            new_state=''
            ping_started=0
            repacd_wifimon_check $managed_network "$current_re_mode" "$current_re_submode" \
                                new_state new_re_mode new_re_submode
        fi

        if [ "$MBsta_onboarding" -eq 0 ]; then
            if [ "$ping_started" -eq 1 ] && [ -z "$sta_iface_24g" ] && [ "$onboarding_done" -eq 0 ]; then
                ping_started=0
                if [ "$map_onboarding_type" != "dpp" ]; then
                    __repacd_info "GW reachable; restarting wsplcd"
                    uci_set wsplcd config 'HyFiSecurity' 1
                    uci commit wsplcd
                   /etc/init.d/wsplcd restart
                fi

                if [ "$map_onboarding_type" == "dpp" ]; then
                    [ -z $fallback_to_nonDPP ] && config_get fallback_to_nonDPP MAPConfig 'fallBackToNonDPP' '0'
                    if [ "$fallback_to_nonDPP" == "0" ]; then
                        /etc/init.d/hyfi-bridging start
                        __repacd_info "GW reachable; starting mapConfigService"
                        uci_set $MAP MAPConfigSettings 'EnableConfigService' 1
                        /etc/init.d/$MAP restart
                    fi
                fi
            fi
        else #MBsta Onboarding Enabled
            if [ "$ping_started" -eq 1 ] && [ "$onboarding_done" -eq 0 ]; then
                ping_started=0
                __repacd_info "GW reachable; restarting wsplcd"
                uci_set wsplcd config 'HyFiSecurity' 1
                uci commit wsplcd
                /etc/init.d/wsplcd restart
            fi
         fi

        # First test for range extender mode change, which could also include
        # a role change if the LED state is updated to indicate that.
        re_mode_change=0
        if [ "$config_rep_mode" = 'auto' ] && \
             [ ! "$current_re_mode" = "$new_re_mode" ]; then
            __repacd_info "New auto-derived RE mode=$new_re_mode"

            uci_set repacd repacd AssocDerivedREMode "$new_re_mode"
            uci_set repacd WiFiLink BSSIDResolveState 'resolving'
            uci_commit repacd

            re_mode_change=1
        fi

        # RE sub-mode change check.
        if [ ! "$current_re_submode" = "$new_re_submode" ]; then
            __repacd_info "New auto-derived RE sub-mode=$new_re_submode"

            uci_set repacd repacd AssocDerivedRESubMode "$new_re_submode"
            uci_commit repacd

            # As of now, no special handling required for "star" and "daisy" submodes.
            # So just keep the Current and New RE-submode in sync.
            current_re_submode=$new_re_submode
        fi

        if [ -n "$new_state" ] && [ ! "$new_state" = "$cur_state" ]; then
            __repacd_info "Updating LED states to $new_state"
            repacd_led_set_states $new_state
            cur_state=$new_state

            # Depending on the startup role, look for the special states
            # that indicate the new role should be different.
            if [ ! "$start_role" = 'RE' ]; then  # init and NonCAP roles
                if [ "$new_state" = "$WIFIMON_STATE_CL_ACTING_AS_RE" ]; then
                    __repacd_info "Restarting in RE role"
                    __repacd_restart 're'
                    re_mode_change=0  # role change includes mode change
                fi
            elif [ "$start_role" = 'RE' ]; then
                if [ "$new_state" = "$WIFIMON_STATE_CL_LINK_INADEQUATE" ] || \
                     [ "$new_state" = "$WIFIMON_STATE_CL_LINK_SUFFICIENT" ]; then
                    __repacd_info "Restarting in Client role"
                    __repacd_restart 'noncap'
                    re_mode_change=0  # role change includes mode change
                fi
            fi

            if [ "$new_state" = "$WIFIMON_STATE_RE_SWITCH_BSTA" ]; then
                config_load 'repacd'
                config_get map_version MAPConfig 'MapVersionEnabled'
                #refer to the vid from the config file during any bsta switch to make sure we don't have vid zero
                #incase of sta disassoc
                config_get map_vid_lan 'MAPWiFiLink' 'vid_lan' '0'
                config_get map_vid_guest1 'MAPWiFiLink' 'vid_guest1' '0'
                config_get map_vid_guest2 'MAPWiFiLink' 'vid_guest2' '0'
                config_get map_vid_guest3 'MAPWiFiLink' 'vid_guest3' '0'
                if [ "$MLOtolegacyvariable" == "true" ]; then
                    config_load wireless
                    config_foreach __repacd_wifimon_set_root_distance wifi-iface
                    __repacd_info " staifacebackup = $ath_interface_to_delete to be deleted
                    MLOtolegacyvar=$MLOtolegacyvariable mldbbss=$mldbbss
                    current sta device=$sta_device &
                    preferred device =$preferred_radio"
                    if [ "$MBsta_onboarding" -ne 1 ]; then
                        __repacd_info "SWITCHING FROM MLO TO SLO"
                    else
                        __repacd_info "SWITCHING FROM MLO TO BACKUP LINK AND SETTING WILDCARD BSSID"
                        # Set wildcard BSSID, so that next time when trying to connect
                        # back in MLO, supplicant will scan and connect to best bss
                        config_foreach __repacd_wifimon_MBsta_set_wildcard_bssid wifi-iface
                    fi
                    mldbsta_enabled=0
                    primary_mldbsta_iface=''
                    MLOtolegacyvariable=''

                    if [ "$MBsta_onboarding" -eq 0 ]; then
                        config_foreach __repacd_wifimon_set_root_distance wifi-iface
                        __repacd_info " staifacebackup = $ath_interface_to_delete to be deleted
                        MLOtolegacyvar=$MLOtolegacyvariable mldbbss=$mldbbss
                        current sta device=$sta_device &
                        preferred device =$preferred_radio"
                        config_load wireless
                        config_foreach __repacd_wifimon_traverse_to_sta wifi-iface
                        config_foreach __repacd_wifimon_add_del_sta_mld wifi-iface
                        if [ "$enable_slo" != 1 ] && [ "$map_version" -gt 1 ] && [ "$MBsta_onboarding" -eq 0 ]; then
                            __repacd_delete_interface $map_primary_nw $ath_interface_to_delete.$map_vid_lan
                            __repacd_delete_interface $NETWORK_TYPE_GUEST1 $ath_interface_to_delete.$map_vid_guest1
                            __repacd_delete_interface $NETWORK_TYPE_GUEST2 $ath_interface_to_delete.$map_vid_guest2
                            __repacd_delete_interface $NETWORK_TYPE_GUEST3 $ath_interface_to_delete.$map_vid_guest3
                        fi
                    else
                        config_load wireless
                        MBsta_desired_link="$MBsta_non_mlo_sta_iface_list"
                        MBsta_current_link="$MBsta_mlo_sta_iface_list"
                        __repacd_wifimon_MBsta_disconnect_link $MBsta_current_link
                        #Check if the ssid & key params were updated during WPS
                        #If not then we need to set it and call multi_up
                        if [ -n "$MBsta_non_mlo_sta_iface_list" ]; then
                            config_foreach __repacd_wifimon_MBsta_check_bhssid wifi-iface $MBsta_non_mlo_sta_iface_list
                        fi
                        if [ "$MBsta_multi_up_required" -eq 1 ]; then
                            __repacd_info "wifi multi_up $MBsta_nonmlo_bstaMld"
                            wifi multi_up $MBsta_nonmlo_bstaMld
                            MBsta_multi_up_required=0
                        else
                            __repacd_wifimon_MBsta_reconnect_link $MBsta_desired_link
                        fi
                        MBsta_mlo_mode=0
                        MBsta_non_mlo_mode=0
                        uci set repacd.MAPConfig.MBsta_mlo_mode='0'
                        uci set repacd.MAPConfig.MBsta_non_mlo_mode='0'
                        cnt_mlo_attempts=$((cnt_mlo_attempts+1))
                        uci_set repacd MAPWiFiLink 'MLOAttemptsCount' "$cnt_mlo_attempts"
                        uci_commit repacd
                        if [ "$map_version" -gt 1 ]; then
                            __repacd_delete_interface $map_primary_nw $MBsta_mlo_bstaMld.$map_vid_lan
                            __repacd_delete_interface $NETWORK_TYPE_GUEST1 $MBsta_mlo_bstaMld.$map_vid_guest1
                            __repacd_delete_interface $NETWORK_TYPE_GUEST2 $MBsta_mlo_bstaMld.$map_vid_guest2
                            __repacd_delete_interface $NETWORK_TYPE_GUEST3 $MBsta_mlo_bstaMld.$map_vid_guest3
                        fi
                        if [ "$map_ts_enabled" -gt 0 ] && [ "$map_version" -gt 1 ]; then
                            if [ -e "/sys/class/net/br-"$map_primary_nw"/brif/"$MBsta_mlo_bstaMld"" ]; then
                                __repacd_info "Interface $MBsta_mlo_bstaMld is present in br-$map_primary_nw and delete it"
                                brctl delif br-$map_primary_nw $MBsta_mlo_bstaMld
                            fi
                        fi
                    fi
                else
                    __repacd_info "SLO TO SLO SWITCHING"
                    __repacd_wifimon_legacy_switches
                    config_load wireless
                    config_foreach __repacd_wifimon_set_root_distance wifi-iface
                    config_load wireless
                    config_foreach __repacd_wifimon_configure_sta_iface wifi-iface
                    #Add or delete mld for 2G 11ax radio i.e. delete mld if radio is 11ax
                    #else add mld option for 11be radio
                    config_foreach __repacd_wifimon_add_del_sta_mld wifi-iface
                    if [ "$enable_slo" != 1 ] && [ "$map_version" -gt 1 ]; then
                        __repacd_info "SLO DISABLED & MAP Version > 1 ADD/DEL INTF"
                        [ "$map_vid_lan" -gt 0 ] && __repacd_add_interface $map_primary_nw $preferred_sta_ath_wifimon.$map_vid_lan
                        [ "$map_vid_guest1" -gt 0 ] && __repacd_add_interface $NETWORK_TYPE_GUEST1 $preferred_sta_ath_wifimon.$map_vid_guest1
                        [ "$map_vid_guest2" -gt 0 ] && __repacd_add_interface $NETWORK_TYPE_GUEST2 $preferred_sta_ath_wifimon.$map_vid_guest2
                        [ "$map_vid_guest3" -gt 0 ] && __repacd_add_interface $NETWORK_TYPE_GUEST3 $preferred_sta_ath_wifimon.$map_vid_guest3
                        [ "$map_vid_lan" -gt 0 ] && __repacd_delete_interface $map_primary_nw $sta_iface_backup.$map_vid_lan
                        [ "$map_vid_guest1" -gt 0 ] && __repacd_delete_interface $NETWORK_TYPE_GUEST1 $sta_iface_backup.$map_vid_guest1
                        [ "$map_vid_guest2" -gt 0 ] && __repacd_delete_interface $NETWORK_TYPE_GUEST2 $sta_iface_backup.$map_vid_guest2
                        [ "$map_vid_guest3" -gt 0 ] && __repacd_delete_interface $NETWORK_TYPE_GUEST3 $sta_iface_backup.$map_vid_guest3
                    fi
                    if [ "$enable_slo" -eq 1 -a -z "$base_mld_mac_addr" ]; then
                        sta_iface=$(cat /sys/class/net/$non_ap_mld/bonding/slaves)
                        sta_vid=$(eval cfg80211tool_mesh $sta_iface get_map_sta_vlan \
                                | grep "get_map_sta_vlan" | cut -d ':' -f2)

                        if [ "$sta_vid" -gt 0 ]; then
                            __repacd_info "Remove $non_ap_mld.$sta_vid from network config before multi_down"
                            __repacd_delete_interface $map_primary_nw $non_ap_mld.$sta_vid
                        fi

                        __repacd_info "wifi multi_up $non_ap_mld $preferred_sta_wifimon"
                        wifi multi_up $non_ap_mld $preferred_sta_wifimon

                        if [ "$sta_vid" -gt 0 ]; then
                            __repacd_info "Add $non_ap_mld.$sta_vid to network config after multi_up"
                            __repacd_add_interface $map_primary_nw $non_ap_mld.$sta_vid
                        fi
                    else
                        __repacd_info "wifi multi_up $preferred_sta_wifimon"
                        wifi multi_up $preferred_sta_wifimon
                    fi
                fi
                __repacd_wifimon_get_sta_info "$map_backhaul_nw"
            elif [ "$new_state" == "$WIFIMON_STATE_RE_SWITCH_MLO_BSTA" ]; then
                __repacd_info "SWITCHING TO MLO"
                if [ "$MBsta_onboarding" -eq 0 ]; then
                    if [ "$enable_mlo" -eq 1 ] && [ "$create_partner_bsta" -eq 1 ]; then
                        legacytoMLO=1
                        create_partner_bsta=0
                        primary_link_bssid="$config_bssid"
                        config_load wireless
                        last_assoc_state=0
                        config_foreach __repacd_wifimon_store_bssid_to_wireless wifi-iface
                        __repacd_info "Cmd: wifi multi_up $netdev_mld"
                        wifi multi_up $netdev_mld
                        __repacd_wifimon_get_sta_info "$map_backhaul_nw"
                        #to set the variable mldbsta_enabled
                        __repacd_wifimon_is_mlo_bsta
                        __repacd_info "switching to mlo, is_mlo_bsta: $mldbsta_enabled"
                    fi
                else
                    direct_bsta_switch=0
                    MBsta_desired_link="$MBsta_mlo_sta_iface_list"
                    MBsta_current_link="$MBsta_non_mlo_sta_iface_list"
                    last_assoc_state=0
                    primary_mldbsta_iface=''
                    config_load wireless
                    __repacd_info "SWITCH TO MLO current_link: $MBsta_current_link desired_link: $MBsta_desired_link"
                    __repacd_wifimon_MBsta_disconnect_link $MBsta_current_link
                    #Check if the ssid & key params were updated during WPS
                    #If not then we need to set it and call multi_up
                    config_foreach __repacd_wifimon_MBsta_check_bhssid wifi-iface  $MBsta_mlo_sta_iface_list
                    if [ "$MBsta_multi_up_required" -eq 1 ]; then
                        __repacd_info "wifi multi_up $MBsta_mlo_bstaMld"
                        wifi multi_up $MBsta_mlo_bstaMld
                        MBsta_multi_up_required=0
                    else
                        config_load wireless
                        __repacd_info "SETTING ROOT DIST TO 255"
                        config_foreach __repacd_wifimon_set_root_distance wifi-iface
                        __repacd_wifimon_MBsta_reconnect_link $MBsta_desired_link
                    fi
                    MBsta_non_mlo_mode=0
                    MBsta_mlo_mode=0
                    uci set repacd.MAPConfig.MBsta_mlo_mode='0'
                    uci set repacd.MAPConfig.MBsta_non_mlo_mode='0'
                    uci commit repacd
                    non_11be_radio_count=0
                    config_get non_11be_radio_count MAPConfig 'number_of_non_11be_radio' 0
                    #Mark short/long timer start time as null to avoid double switch to MLO
                    MBsta_mlo_attempt_start_time=''
                    if [ "$non_11be_radio_count" -eq 0 ]; then
                        if [ "$map_version" -gt 1 ]; then
                            __repacd_delete_interface $map_primary_nw $MBsta_nonmlo_bstaMld.$map_vid_lan
                            __repacd_delete_interface $NETWORK_TYPE_GUEST1 $MBsta_nonmlo_bstaMld.$map_vid_guest1
                            __repacd_delete_interface $NETWORK_TYPE_GUEST2 $MBsta_nonmlo_bstaMld.$map_vid_guest2
                            __repacd_delete_interface $NETWORK_TYPE_GUEST3 $MBsta_nonmlo_bstaMld.$map_vid_guest3
                            if [ "$map_ts_enabled" -gt 0 ]; then
                                #Sometimes mld_del is not getting retrieved correctly in vlanmon, so war added here
                                #to make sure to delete the parent mld before doing bsta switch
                                if [ -e "/sys/class/net/br-"$map_primary_nw"/brif/"$MBsta_nonmlo_bstaMld"" ]; then
                                    __repacd_info "Interface $MBsta_nonmlo_bstaMld is present in br-$map_primary_nw and delete it"
                                    brctl delif br-$map_primary_nw $MBsta_nonmlo_bstaMld
                                fi
                            fi
                        else
                            #MapR1
                            if [ -e "/sys/class/net/br-"$map_primary_nw"/brif/"$MBsta_nonmlo_bstaMld"" ]; then
                                __repacd_info "Interface $MBsta_nonmlo_bstaMld is present in br-$map_primary_nw and delete it"
                                brctl delif br-$map_primary_nw $MBsta_nonmlo_bstaMld
                            fi
                        fi
                    else
                        #Non11be
                        if [ "$map_version" -gt 1 ]; then
                            __repacd_delete_interface $map_primary_nw $MBsta_non_mlo_sta_iface_list.$map_vid_lan
                            __repacd_delete_interface $NETWORK_TYPE_GUEST1 $MBsta_non_mlo_sta_iface_list.$map_vid_guest1
                            __repacd_delete_interface $NETWORK_TYPE_GUEST2 $MBsta_non_mlo_sta_iface_list.$map_vid_guest2
                            __repacd_delete_interface $NETWORK_TYPE_GUEST3 $MBsta_non_mlo_sta_iface_list.$map_vid_guest3
                        fi
                        if [ "$map_ts_enabled" -gt 0 ]; then
                            if [ -e "/sys/class/net/br-"$map_primary_nw"/brif/"$MBsta_non_mlo_sta_iface_list"" ]; then
                                __repacd_info "Interface $MBsta_non_mlo_sta_iface_list is present in br-$map_primary_nw and delete it"
                                brctl delif br-$map_primary_nw $MBsta_non_mlo_sta_iface_list
                            fi
                        fi
                        #For Legacy STA vaps, when vlan is deleted. Ubus network reload invokes
                        #down event on the vap. So script needs to trigger an up event on it
                        __repacd_info "Ifconfig $MBsta_non_mlo_sta_iface_list up required for Legacy sta"
                        ifconfig $MBsta_non_mlo_sta_iface_list up
                    fi
                fi
            else
                if [ "$new_state" = "$WIFIMON_STATE_RE_BACKHAUL_GOOD" ] ||
                [ "$new_state" = "$WIFIMON_STATE_RE_BACKHAUL_FAIR" ] ||
                [ "$new_state" = "$WIFIMON_STATE_RE_BACKHAUL_POOR" ]; then
                    __repacd_info "bSTA is stable;"
                    uci_set wsplcd config 'HyFiSecurity' 1
                    uci commit wsplcd
                    config_load 'repacd'
                    config_get map_version MAPConfig 'MapVersionEnabled'
                    config_get map_ts_enabled MAPConfig 'MapTrafficSeparationEnable'
                    if [ "$map_version" -gt 1 ]; then
                        if [ "$map_ts_enabled" -eq 1 ]; then
                           [ "$map_onboarding_type" != "dpp" ] && /etc/init.d/hyfi-bridging stop
                        fi

                        if [ "$onboarding_done" -eq 1 ]; then
                            if [ "$map_ts_enabled" -eq 1 ]; then
                                /etc/init.d/hyfi-bridging start
                            fi
                            if [ "$MBsta_onboarding" -eq 1 ]; then
                                __repacd_info "restarted ezmesh"
                                /etc/init.d/$MAP restart
                            fi
                            config_load wireless
                            config_foreach __repacd_set_valid_root_distance wifi-iface
                        fi
                    fi
                    if [ "$MBsta_onboarding" -eq 0 ]; then
                        if [ "$map_onboarding_type" != "dpp" ] && [ -z "$(ps | grep wsplcd-lan.conf | grep -v grep | awk '{print$1}')" ]; then
                            /etc/init.d/wsplcd start
                            __repacd_info "WSPLCD is not up, starting again"
                        fi
                    else
                        if [ -z "$(ps | grep wsplcd-lan.conf | grep -v grep | awk '{print$1}')" ]; then
                            /etc/init.d/wsplcd start
                            __repacd_info "WSPLCD is not up, starting again"
                        fi
                    fi
                    if [ "$map_onboarding_type" == "dpp" ]; then
                        [ -z $fallback_to_nonDPP ] && config_get fallback_to_nonDPP MAPConfig 'fallBackToNonDPP' '0'
                        if [ "$fallback_to_nonDPP" == "0" ]; then
                            config_load $MAP
                            config_get enable_config_service MAPConfigSettings 'EnableConfigService' '0'
                            #Check for ezmesh PID and restart
                            if [ "$enable_config_service" -eq 0 ] || [ -z "$(ps | grep ezmesh-lan.conf | grep -v grep | awk '{print$1}')" ]; then
                                /etc/init.d/hyfi-bridging start
                                __repacd_info "Ezmesh is not up with ConfigService, enabling & restarting mapConfigService"
                                uci_set $MAP MAPConfigSettings 'EnableConfigService' 1
                                /etc/init.d/$MAP restart
                            fi
                        fi
                    fi

                    uci_set repacd FrontHaulMgr ForceDownOnStart 0
                    uci_commit repacd

                    # Perform special handling for the bBSS interfaces.
                    if [ "$new_state" = "$WIFIMON_STATE_RE_BACKHAUL_POOR" ]; then
                        repacd_fronthaulmgrmon_bring_down_bBSSes
                    fi
                fi
            fi
        fi
        if [ "$new_state" = "$WIFIMON_STATE_RE_BACKHAUL_GOOD" ] ||
            [ "$new_state" = "$WIFIMON_STATE_RE_BACKHAUL_FAIR" ]; then
            # This will normally be a nop, but is done here instead of only
            # during a state change just in case the bBSSes do not all come
            # up successfully the first time the ifconfig is run.
            repacd_fronthaulmgrmon_bring_up_bBSSes
        fi

        # Handle any RE mode change not implicitly handled above.
        if [ "$re_mode_change" -gt 0 ]; then
            if [ ! "$start_role" = 'RE' ]; then  # init and NonCAP roles
                __repacd_restart 'noncap'
            elif [ "$start_role" = 'RE' ]; then
                __repacd_restart 're'
            fi
        fi

        # if restart_wifi and re_mode_change is not start
        # go to determing if 2.4G backhaul interface need to down or not
        if [ "$restart_wifi" -eq 0 ]; then
            if [ "$re_mode_change" -eq 0 ]; then
                repacd_wifimon_independent_channel_check
            fi
        fi
    else
        # Initialise Wi-Fi monitoring logic
        __repacd_wifimon_init

        if [ -n "$new_state" ]; then
            __repacd_info "Setting initial LED states to $new_state"
            repacd_led_set_states $new_state
            cur_state=$new_state
        fi
    fi

    if [ "$eth_mon_enabled" -eq 1 ]; then
        repacd_ethmon_check
    fi

    if [ "$cur_role" != 'CAP' ] && [ "$alg_set" = "son" ]; then
        repacd_backhaulmgrmon_check
    fi

    if [ "$cur_role" != 'CAP' ] && [ "$alg_set" != "maplite" ]; then
        repacd_fronthaulmgrmon_check
    fi

    #Vlanmon is required for Bridge mode even before onboarding
    if [ "$map_ts_enabled" -eq 1 -a "$onboarding_done" -eq 1 ] ||
       [ "$map_ts_enabled" -eq 1 -a "$gwcon_mode" == "CAP" ]; then
        repacd_map_vlanmon_check
    fi

    if [ "$map_onboarding_type" == "dpp" ]; then
       [ -z $fallback_to_nonDPP ] && config_get fallback_to_nonDPP MAPConfig 'fallBackToNonDPP' '0'

       if [ "$fallback_to_nonDPP" == "0" ]; then
          repacd_wifimon_dpp_check_onboarding_status
       fi
    fi

    if [ "$map_fast_onboarding" -eq 1 ]; then
        [ "$enable_fastcloning" -eq 0 ] && repacd_wifimon_check_onboarding
        [ "$enable_fastcloning" -eq 1 ] && repacd_wifimon_check_onboarding_OPT
        config_get onboarding_done MAPConfig 'OnboardingDone' '0'
    fi
    if [ "$onboarding_done" -eq 1 ] && [ "$EU_cac_state" -eq 1 ] && [ "$enable_mlo" -eq 1 ]; then
        #if cac_state was one during link measurement means we did not attempt mlo
        #due to cac & stayed in Legacy. Let's keep checking cac status & once it
        #is out of cac let's resample and try going to MLO
        __repacd_wifimon_check_cac_status
        if [ "$cac_state" -eq 0 ]; then
            #Mark this variable 0 only in this place
            #Else it will cause race condition and
            #We cannot move to next bsta
            EU_cac_state=0
        fi

        if [ "$EU_cac_state" -eq 0 ]; then
            if [ "$MBsta_onboarding" -eq 1 ]; then
                #With multi bsta onboarding enabled, if we resample
                #we will only be able to attempt mlo again post
                #additional delay added by short timer.
                if [ "$MBsta_non_mlo_mode" -eq 1 ] && [ -n "$MBsta_non_mlo_sta_connected_list" ]; then
                    if [ "$MBsta_short_timer_used" -eq 0 ]; then
                        new_state="RE_SwitchingMLObSTA"
                        direct_bsta_switch=1
                        __repacd_info "Moving to MLO post CAC"
                        MBsta_mlo_attempt_start_time=''
                    else
                        last_assoc_state=0
                        __repacd_info "LETS RESAMPLE, shorttimer was used"
                    fi
                else
                    __repacd_info "CAC complete!"
                fi
            else
                last_assoc_state=0
                __repacd_info "LETS RESAMPLE"
            fi
        fi
    fi
    if [ "$onboarding_done" -eq 1 -a "$MBsta_onboarding" -eq 1 ]; then
        config_get MBsta_bh_ssid_wps MAPConfig BackhaulSSID
        if [ -z "$MBsta_bh_ssid_wps" ]; then
            config_foreach __repacd_wifimon_MBsta_get_bhssid wifi-iface
            uci set repacd.MAPConfig.BackhaulSSID="$MBsta_bh_ssid"
            uci commit repacd
            __repacd_info "Post WPS onboarding we set the BHSSID in MAPConfig"
        fi
    fi

    if [ "$onboarding_done" -eq 1 ]; then
        __repacd_firewall_addition $map_ts_active
    fi

    if [ "$map_fast_onboarding" -eq 0 -o "$onboarding_done" -eq 1 -o "$gwcon_mode" == "CAP" ]; then
        # Re-check the link conditions in a few seconds.
        sleep $link_check_delay
    fi

done
