#!/bin/sh
# Copyright (c) 2017 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

BACKHAULMGRMON_DEBUG_OUTOUT=0

. /lib/functions/repacd-plcmon.sh
. /lib/functions/repacd-wifimon.sh

backhaulmgr_rate_samples=0
prev_plc_rate_compare_counter=0 prev_wlan_rate_compare_counter=0
wlan_rate_consistent_num=0 plc_rate_consistent_num=0
gw_not_reachable_consistent_num=0
PingTimeoutsToCAP=0

# Config Parameters
config_eval_time_plc=0 select_only_one_backhaul_interface=0

# PLC parameters
is_plc_enabled=0 is_plc_connected=0
is_plc_iface_brought_down=0 is_wlan_ifaces_brought_down=0
prev_plc_rate=0 force_down_plc_timestamp=''
prev_wlan_rate=0

# Emit a message at debug level.
# input: $1 - the message to log
__repacd_backhaulmgrmon_debug() {
    local stderr=''
    if [ "$BACKHAULMGRMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.backhaulmgrmon -p user.debug "$1"
}

# Emit a message at info level.
__repacd_backhaulmgrmon_info() {
    local stderr=''
    if [ "$BACKHAULMGRMON_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.backhaulmgrmon -p user.info "$1"
}

repacd_backhaulmgrmon_init() {
    # First resolve the config parameters.
    config_load repacd
    config_get config_eval_time_plc 'PLCLink' 'PLCBackhaulEvalTime' '1800'
    # Assuming that PLC link is guaranteed than 2G at configured threshold value
    config_get plc_link_threshold_to_2G 'BackhaulMgr' 'PLCLinkThresholdto2G' '60'
    config_get select_only_one_backhaul_interface 'BackhaulMgr' 'SelectOneBackHaulInterfaceInDaisy' '0'
    config_get backhaulmgr_rate_samples 'BackhaulMgr' 'BackHaulMgrRateNumMeasurements' '5'
    config_get PingTimeoutsToCAP 'BackhaulMgr' 'SwitchInterfaceAfterCAPPingTimeouts' '5'
}

repacd_backhaulmgrmon_check() {
    if [ "$select_only_one_backhaul_interface" -gt 0 ]; then
        if __repacd_plcmon_is_plc_enabled; then
            local plc_iface_rate=0

            if [ "$is_plc_enabled" -eq 0 ]; then
                __repacd_backhaulmgrmon_debug "PLC is ENABLED"
                is_plc_enabled=1
            fi

            if [ "$IS_GW_REACHABLE" -gt 0 ]; then
                gw_not_reachable_consistent_num=0
            else
                if [ "$curr_root_distance" -eq "$RE_ROOT_AP_DISTANCE_INVALID" ]; then
                    gw_not_reachable_consistent_num=$((gw_not_reachable_consistent_num + 1))
                fi
            fi

            if __repacd_plcmon_is_plc_connected plc_iface_rate; then
                if [ "$is_plc_connected" -eq 0 ]; then
                    __repacd_backhaulmgrmon_debug "PLC CONNECTED"
                    is_plc_connected=1
                fi
                __repacd_backhaulmgrmon_debug "Rate sample: PLC = $plc_iface_rate Mbps"

                # curr_root_distance greater than 1 means the topology is Daisy Chain
                if [ "$curr_root_distance" -gt 1 -a \
                     "$curr_root_distance" -ne "$RE_ROOT_AP_DISTANCE_INVALID" ]; then

                    # Check whether WLAN association is complete
                    if [ "$bssid_assoc_count" -ge "$min_bssid_assoc" ]; then

                        # In case of Daisy Chain, evaluate the link rate of available interfaces
                        # At this moment WLAN and PLC interfaces are considered,
                        # can be extend to other interfaces.

                        # Check whether WLAN link rate more than PLC interface
                        # For now, Rate to CAP is considered only 5G interface.
                        if [ "$rate_to_CAP" -ge "$plc_iface_rate" ]; then
                            # Increment the counter for WLAN rate samples
                            wlan_rate_consistent_num=$((wlan_rate_consistent_num + 1))
                            plc_rate_consistent_num=0

                        # Assume that if Rate to CAP is zero and if curr_root_distance greater than 1
                        # means 2Gis connected.
                        # Assume 2G value would be around 80 - 120 Mbps in real world
                        # then bring down PLC interface. In case the threshold value of PLC to 2G
                        # interface is configured and if PLC link rate is consistently better than
                        # threshold value then select PLC ineterface and bring down WLAN interfaces.
                        elif [ "$rate_to_CAP" -eq 0 -a "$plc_iface_rate" -lt "$plc_link_threshold_to_2G" ]; then
                            # Increment the counter for WLAN rate samples
                            wlan_rate_consistent_num=$((wlan_rate_consistent_num + 1))
                            plc_rate_consistent_num=0
                        else
                            # Increment the counter for PLC rate samples
                            plc_rate_consistent_num=$((plc_rate_consistent_num + 1))
                            wlan_rate_consistent_num=0
                        fi

                        # Choosing best uplink interface
                        # Bringing down either of the interface avoids loop in SON network.
                        if [ "$wlan_rate_consistent_num" -eq "$backhaulmgr_rate_samples" ]; then
                            __repacd_backhaulmgrmon_debug "Bringing down PLC interface..."
                            __repacd_plcmon_bring_plc_iface_down
                            is_plc_iface_brought_down=1
                            prev_plc_rate=$plc_iface_rate
                            __repacd_wifimon_get_timestamp force_down_plc_timestamp
                            wlan_rate_consistent_num=0
                            plc_rate_consistent_num=0
                        elif [ "$plc_rate_consistent_num" -eq "$backhaulmgr_rate_samples" ]; then
                            __repacd_backhaulmgrmon_debug "Bringing down WLAN interface..."
                            __repacd_gwmon_bring_iface_down $sta_iface_5g
                            __repacd_gwmon_bring_iface_down $sta_iface_24g
                            # Bringing down interfaces will not update Wifi assoc count to zero.
                            # Backhaul Manager bringing down both 2G and 5G, so update Wifi assoc count to zero.
                            bssid_assoc_count=0
                            is_wlan_ifaces_brought_down=1
                            prev_wlan_rate=$rate_to_CAP
                            wlan_rate_consistent_num=0
                            plc_rate_consistent_num=0
                        fi
                        rate_to_CAP=0
                    fi
                fi

                if [ "$is_wlan_ifaces_brought_down" -eq 1 ]; then

                    # Check if PLC link rate is bad then switch over to WLAN interface
                    # and bring up WLAN interface to re-evaluate the Link metrics.
                    if [ "$prev_wlan_rate" -gt "$plc_iface_rate" ]; then
                        prev_wlan_rate_compare_counter=$((prev_wlan_rate_compare_counter + 1))
                        if [ "$prev_wlan_rate_compare_counter" -eq "$backhaulmgr_rate_samples" ]; then
                            __repacd_backhaulmgrmon_debug "PLC Link rate ($plc_iface_rate Mbps) is bad than previous rate of WLAN Link ($prev_wlan_rate Mbps)"

                            # Check whether WLAN interfaces are in progress of association
                            if [ "$bssid_assoc_count" -gt 0 ]; then
                                __repacd_backhaulmgrmon_debug "WLAN interfaces are already trying to associate"
                            else
                                # Only bringup WLAN interfaces if WLAN interfaces are not up
                                __repacd_backhaulmgrmon_debug "Bringing up WLAN interface..."
                                __repacd_gwmon_bring_iface_up $sta_iface_5g
                                __repacd_gwmon_bring_iface_up $sta_iface_24g
                            fi
                            is_wlan_ifaces_brought_down=0
                            prev_wlan_rate_compare_counter=0
                        fi
                    else
                        prev_wlan_rate_compare_counter=0
                    fi

                    # Check whether Gateway is reachable consitently, if not bringup WLAN interfaces
                    if [ "$gw_not_reachable_consistent_num" -eq "$PingTimeoutsToCAP" ]; then
                        # Check whether WLAN interfaces are in progress of association
                        if [ "$bssid_assoc_count" -gt 0 ]; then
                            __repacd_backhaulmgrmon_debug "WLAN interfaces are already trying to associate"
                        else
                            # Only bringup WLAN interfaces if WLAN interfaces are not up
                            __repacd_backhaulmgrmon_debug "Bringing up WLAN interface..."
                            __repacd_gwmon_bring_iface_up $sta_iface_5g
                            __repacd_gwmon_bring_iface_up $sta_iface_24g
                        fi
                        is_wlan_ifaces_brought_down=0
                        prev_wlan_rate_compare_counter=0
                        gw_not_reachable_consistent_num=0
                    fi
                elif [ "$gw_not_reachable_consistent_num" -gt 0 ]; then

                    # Check whether WLAN associated and bring down PLC interface if Daisy Chain topology is formed
                    if [ "$curr_root_distance" -gt 1 -a \
                         "$curr_root_distance" -ne "$RE_ROOT_AP_DISTANCE_INVALID" ]; then

                        __repacd_backhaulmgrmon_debug "GW ($gw_ip) not reachable thru PLC, Bringing down PLC interface..."
                        __repacd_plcmon_bring_plc_iface_down
                        is_plc_iface_brought_down=1
                        prev_plc_rate=0
                        __repacd_wifimon_get_timestamp force_down_plc_timestamp
                        wlan_rate_consistent_num=0
                        plc_rate_consistent_num=0
                    fi
                fi
            else
                if [ "$is_plc_connected" -gt 0 ]; then
                    __repacd_backhaulmgrmon_debug "PLC NOT CONNECTED"
                    is_plc_connected=0
                fi

                # Try to bringup WLAN ifaces which were brought down earlier by repacd
                # When PLC backhaul connection is lost
                if [ "$is_wlan_ifaces_brought_down" -eq 1 -a \
                     "$curr_root_distance" -eq "$RE_ROOT_AP_DISTANCE_INVALID" ]; then
                    __repacd_backhaulmgrmon_debug "Bringing up WLAN interface..."
                    __repacd_gwmon_bring_iface_up $sta_iface_5g
                    __repacd_gwmon_bring_iface_up $sta_iface_24g
                    is_wlan_ifaces_brought_down=0

                # PLC not connected means either device is disconnected or
                # repacd brought down PLC interface
                # Check whether PLC is brought down by repacd
                elif [ "$is_plc_iface_brought_down" -eq 1 ]; then
                    # if curr_root_distance RE_ROOT_AP_DISTANCE_INVALID means WLAN interfaces were not
                    # brought down by repacd but lost connection to CAP,
                    # if curr_root_distance 1 means the topology is not Daisy Chain
                    # in either of above cases bringup PLC interface
                    if [ "$curr_root_distance" -eq "$RE_ROOT_AP_DISTANCE_INVALID" ] || \
                       [ "$curr_root_distance" -eq 1 ]; then
                        __repacd_backhaulmgrmon_debug "Bringing up PLC interface..."
                        __repacd_plcmon_bring_plc_iface_up
                        is_plc_iface_brought_down=0
                    elif  __repacd_wifimon_is_timeout $force_down_plc_timestamp $config_eval_time_plc; then
                        __repacd_backhaulmgrmon_debug "Bringing up PLC:eval interval $config_eval_time_plc sec expired"
                        __repacd_plcmon_bring_plc_iface_up
                        is_plc_iface_brought_down=0
                        __repacd_wifimon_get_timestamp force_down_plc_timestamp
                    fi

                    if [ "$curr_root_distance" -gt 1 ]; then
                        # Check if WLAN link rate is bad then switch over to PLC interface
                        # and bring up PLC interface to re-evaluate the Link metrics.

                        # For now, Rate to CAP is considered only 5G interface, assuming that if
                        # Rate to CAP is zero and if curr_root_distance greater than 1 means 2G
                        # is connected. Assume 2G value would be around 80 - 120 Mbps in real world
                        # then bring down PLC interface. In case the threshold value of PLC to 2G
                        # interface is configured and if PLC link rate is consistently better than
                        # threshold value then select PLC ineterface and bring down WLAN interfaces.
                        if [ "$rate_to_CAP" -eq 0 -a "$prev_plc_rate" -lt "$plc_link_threshold_to_2G" ]; then
                            prev_plc_rate_compare_counter=0

                        elif [ "$prev_plc_rate" -gt "$rate_to_CAP" ]; then
                            prev_plc_rate_compare_counter=$((prev_plc_rate_compare_counter + 1))
                            if [ "$prev_plc_rate_compare_counter" -eq "$backhaulmgr_rate_samples" ]; then
                                __repacd_backhaulmgrmon_debug "WLAN Link rate ($rate_to_CAP Mbps) is bad than previous rate of PLC Link ($prev_plc_rate Mbps)"
                                __repacd_backhaulmgrmon_debug "Bringing up PLC interface..."
                                __repacd_plcmon_bring_plc_iface_up
                                is_plc_iface_brought_down=0
                                prev_plc_rate_compare_counter=0
                            fi
                        else
                            prev_plc_rate_compare_counter=0
                        fi
                    fi
                    rate_to_CAP=0
                fi
            fi
        else
            if [ "$is_plc_enabled" -gt 0 ]; then
                __repacd_backhaulmgrmon_debug "PLC is DISABLED"
                is_plc_enabled=0
            fi
        fi
    fi
}
