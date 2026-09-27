#!/bin/sh
# Copyright (c) 2016 Qualcomm Atheros, Inc.
#
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.

. /usr/share/libubox/jshn.sh
. /lib/functions/repacd-netdet.sh
. /lib/functions/hyfi-iface.sh

LP_DIRECTION_UPSTREAM="upstream"
LP_DIRECTION_DOWNSTREAM="downstream"
LP_DIRECTION_UNKNOWN="unknown"
LP_ROLE_CAP="CAP"
LP_ROLE_RE="RE"
LP_VLAN_OFFSET=10

LP_DIRECTION_DB="/etc/ports.dir"

EDMA_ETH0_PORTBMP="/proc/sys/net/edma/default_group1_bmp"
EDMA_ETH1_PORTBMP="/proc/sys/net/edma/default_group2_bmp"

#TODO - these values need to be determined by board
#Port number of the port in the eth0 vlan by default
LP_ETH0_PORT=5
#Port number of the cpu port including the "t" for tagging if appropriate
LP_CPU_PORT="0t"

#TODO - these should probably be configurable from outside ths script
#Timeout for dhcp requests from CA
LP_DHCP_TO_CAP=15
#Timeout for dhcp requests from RE (needs to be longer than from CA)
LP_DHCP_TO_RE=30

__lp_loginfo() {
    logger -t repacd.lp -p user.info "$1"
}

__lp_logdebug() {
    logger -t repacd.lp -p user.debug "$1"
}

#Returns the default interface for the given port
lp_defaultinterface() {
    if [ $port = $LP_ETH0_PORT ]; then
        echo "eth0"
    else
        echo "eth1"
    fi
}
#Returns the isolated inteface name for a given port
lp_interfacebyport() {
    local port=$1
    local baseinterface=$(lp_defaultinterface $port)
    local interface="${baseinterface}.$(( $port + $LP_VLAN_OFFSET ))"
    echo $interface
}

#Returns the role of the device, either "CAP" or "RE"
lp_devicerole() {
    #A CAP should have an entry for network.wan.ifname, an RE should not
    if [ "$openwrt_version_check" -eq 1 ]; then
        uci get network.wan.device >/dev/null
    else
        uci get network.wan.ifname >/dev/null
    fi
    if [ $? = 0 ]; then
        echo $LP_ROLE_CAP
    else
        echo $LP_ROLE_RE
    fi
}

#Returns the determined direction (upstream, downstram, or unknown) for the
#given port
lp_getportdirection() {
    local port=$1
    local direction=$LP_DIRECTION_UNKNOWN
    if [ -f $LP_DIRECTION_DB ]; then
        json_load "$(cat $LP_DIRECTION_DB)"
        json_select "port" >/dev/null
        json_get_var direction $port
    fi

    echo $direction
}

#set the current direction (upstream, downstream, or unknown) in a
#persistent db
lp_setportdb() {
    local port=$1
    local direction=$2
    local current
    local new
    local nports=$(lp_numports)
    local dir

    if [ -f $LP_DIRECTION_DB ]; then
        current=$(cat $LP_DIRECTION_DB)
    else
        current="{}"
    fi
    json_init
    json_add_array "port"
    new=$(json_dump)
    for p in $(seq 1 $nports); do
        if [ $p = $port ]; then
            dir=$direction
        else
            json_load "$current"
            dir=$LP_DIRECTION_UNKNOWN
            json_select "port" >/dev/null
            json_get_var dir $p
        fi
        json_load "$new"
        json_select "port"
        json_add_string $p $dir
        new=$(json_dump)
    done
    json_close_array
    new=$(json_dump)

    local tmpfile="$(dirname $LP_DIRECTION_DB)/.$(basename $LP_DIRECTION_DB)"
    echo "$new" > $tmpfile
    mv $tmpfile $LP_DIRECTION_DB
}

#Returns the current number of ports which are marked upstream
lp_numupstream() {
    local nup=0
    local dir
    local nports=$(lp_numports)

    if [ -f $LP_DIRECTION_DB ]; then
        json_load "$(cat $LP_DIRECTION_DB)"
        json_select "port" >/dev/null
        for p in $(seq 1 $nports); do
            dir=$LP_DIRECTION_UNKNOWN
            json_get_var dir $p
            if [ $dir = $LP_DIRECTION_UPSTREAM ]; then
                nup=$(( nup + 1 ))
            fi
        done
    fi
    echo $nup
}


#Returns the number of ethernet ports on switch0
lp_numports() {
    local lines=$(swconfig dev switch0 show | grep ports | wc -l)
    local words=$(swconfig dev switch0 show | grep ports | wc -w)

    echo $(( (( $words - $lines )) - $lines ))
}

#Determines whether the given interface is upstream or downstream
lp_determinedirection() {

    local port=$1
    #If netdet has already determined that this is a CAP device
    #then we will already have an upstream connection, and all
    #subsequent connections (including this one) should be
    #considered downstream
    local netdetmode=$(netdet_get_mode_db)
    local upstrmports=$(lp_numupstream)
    local portdirection=$(lp_getportdirection $port)

    logger -t repacd.lp -p user.debug "netdetmode: $netdetmode"
    logger -t repacd.lp -p user.debug "upstrmports: $upstrmports"
    if [ $netdetmode = "cap" ] && [ $upstrmports -gt 0 ] && [ $portdirection != $LP_DIRECTION_UPSTREAM ]; then
        logger -t repacd.lp -p user.info "Port $port assumed downstream on cap"
        echo $LP_DIRECTION_DOWNSTREAM
    else
        local role=$2
        local iface=$(lp_interfacebyport $port)
        local timeout
        if [ $role = $LP_ROLE_CAP ]; then
            timeout=$LP_DHCP_TO_CAP
        else
            timeout=$LP_DHCP_TO_RE
        fi

        if udhcpc -i $iface -t $timeout -T1 -n -q -R > /dev/null 2>&1; then
            __lp_loginfo "Port $port determined upstream"
            echo $LP_DIRECTION_UPSTREAM
        else
            __lp_loginfo "Port $port determined downstream"
            echo $LP_DIRECTION_DOWNSTREAM
        fi
    fi
}

#Isolate port in its own vlan and create virtual interface
lp_isolateport() {
    local port=$1
    local isovlan=$(( $port + $LP_VLAN_OFFSET ))
    local currvlan=$(swconfig dev switch0 port $port get pvid)
    local currports=$(swconfig dev switch0 vlan $currvlan show | grep ports | cut -d: -f2)
    local newports=""

    __lp_loginfo "Isolating switch port $port into its own vlan"
    #remove port from its current vlan
    for p in $currports; do
        if [ $p != $port ]; then
            newports="$newports $p"
        fi
    done
    __lp_logdebug "swconfig dev switch0 vlan $currvlan set ports \"$LP_CPU_PORT $newports\""
    swconfig dev switch0 vlan $currvlan set ports "$LP_CPU_PORT $newports"

    #add port to isovlan
    currports=$(swconfig dev switch0 vlan $isovlan show | grep ports | cut -d: -f2)
    __lp_logdebug "swconfig dev switch0 vlan $isovlan set ports \"$LP_CPU_PORT $currports $port\""
    swconfig dev switch0 vlan $isovlan set ports "$LP_CPU_PORT $currports $port"
    __lp_logdebug "swconfig dev switch0 set apply"
    swconfig dev switch0 set apply

    if [ $port = $LP_ETH0_PORT ]; then
        __lp_logdebug "vconfig add eth0 $isovlan"
        vconfig add eth0 $isovlan
    else
        __lp_logdebug "vconfig add eth1 $isovlan"
        vconfig add eth1 $isovlan
    fi
    lp_setedmamasks $port $(lp_defaultinterface $port)
}

lp_deisolateport() {
    local port=$1
    local isovlan=$(( $port + $LP_VLAN_OFFSET ))
    local currvlan=$(swconfig dev switch0 port $port get pvid)

    if [ $isovlan = $currvlan ]; then
        #This check might not be necessary, but it prevents us from
        #manipulating a port which is not isolated

        local isoiface=$(lp_interfacebyport $port)
        local unisovlan

        #Remove port form the isolated vlan
        swconfig dev switch0 vlan $isovlan set ports ""
        #Remove virtual interface
        vconfig rem $isoiface
        #put port back in the appropriate eth0 or eth1 vlan
        if [ $port = $LP_ETH0_PORT ]; then
            unisovlan=2
        else
            unisovlan=1
        fi

        local currports=$(swconfig dev switch0 vlan $unisovlan show | grep ports | cut -d: -f2)
        swconfig dev switch0 vlan $unisovlan set ports "$currports $port"
        swconfig dev switch0 set apply
    fi
}

#set the edma masks so the bitmap for the given iface (eth0 or eth1)
#contains the given port, and the bitmask fot the other iface does not
lp_setedmamasks() {
    local port=$1
    local dest_iface=$2
    local currmask_eth0=$(cat $EDMA_ETH0_PORTBMP)
    local currmask_eth1=$(cat $EDMA_ETH1_PORTBMP)
    local newmask_eth0
    local newmask_eth1

    if [ $dest_iface = "eth0" ]; then
        newmask_eth0=$(( $currmask_eth0 | (1 << $port) ))
        newmask_eth1=$(( $currmask_eth1 & ~(1 << $port) ))
    elif [ $dest_iface = "eth1" ]; then
        newmask_eth0=$(( $currmask_eth0 & ~(1 << $port) ))
        newmask_eth1=$(( $currmask_eth1 | (1 << $port) ))
    else
        #should not get here, but if we do do not adjust either bitmap
        newmask_eth0=$currmask_eth0
        newmask_eth1=$currmask_eth1
    fi

    printf "0x%x" $newmask_eth0 > $EDMA_ETH0_PORTBMP
    printf "0x%x" $newmask_eth1 > $EDMA_ETH1_PORTBMP
}

#moves the given port to the eth0 vlan (2) or the eth1 vlan (1)
#and removes it from the other via swconfig
lp_moveportswconfig() {
    local port=$1
    local dest_iface=$2
    local removefromvlan
    local addtovlan
    local isovlan=$(( $port + $LP_VLAN_OFFSET ))
    local isoiface=$(lp_interfacebyport $port)

    if [ $dest_iface = "eth0" ]; then
        removefromvlan=1
        addtovlan=2
    elif [ $dest_iface = "eth1" ]; then
        removefromvlan=2
        addtovlan=1
    else
        #should not get here, but if we do do not move port
        removefromvlan=3
        addtovlan=3
    fi

    local currports=$(swconfig dev switch0 vlan $removefromvlan show | grep ports | cut -d: -f2)
    local newports
    for p in $currports; do
        if [ $p != $port ]; then
            newports="$newports $p"
        fi
    done
    __lp_logdebug "swconfig dev switch0 vlan $removefromvlan set ports $newports"
    swconfig dev switch0 vlan $removefromvlan set ports $newports

    currports=$(swconfig dev switch0 vlan $addtovlan show | grep ports | cut -d: -f2)
    __lp_logdebug "swconfig dev switch0 vlan $addtovlan set ports \"$currports $port\""
    swconfig dev switch0 vlan $addtovlan set ports "$currports $port"

    #eliminate the isolation vlan and virtual interface
    __lp_logdebug "swconfig dev switch0 vlan $isovlan set ports \"\""
    swconfig dev switch0 vlan $isovlan set ports ""
    __lp_logdebug "vconfig rem $isoiface"
    vconfig rem $isoiface

    __lp_logdebug "swconfig dev switch0 set apply"
    swconfig dev switch0 set apply
}


#Move the given port to the eth0 vlan
lp_moveporttoeth0() {
    local port=$1
    __lp_loginfo "Moving switch port $port into the eth0 vlan"
    lp_moveportswconfig $port "eth0"
    lp_setedmamasks $port "eth0"
}

#Move the given port to the eth1 vlan
lp_moveporttoeth1() {
    local port=$1
    __lp_loginfo "Moving switch port $port into the eth1 vlan"
    lp_moveportswconfig $port "eth1"
    lp_setedmamasks $port "eth1"
}


#Make the hotplug call to signal the direction has been determined for
#the given port
lp_signaldirectionhotplug() {
    local port=$1
    local direction=$2
    (export PORT=$port; export EVENT="direction"; export STATE=$direction; hotplug-call switch)
}

#Called by hotplug whenever a port transtions to link-up status
lp_onlinkup() {
    local port=$1
    local role=$(lp_devicerole)
    local direction
    local current_direction=$(lp_getportdirection $port)

    logger -t repacd.lp -p user.debug "did we miss a down notification for port $port? current_direction: $current_direction"
    if [ $current_direction != $LP_DIRECTION_UNKNOWN ]; then
        logger -t repacd.lp -p user.debug "we missed a down notification for port $port so act like we didn't"
        lp_onlinkdown $port
    fi
    direction=$(lp_determinedirection $port $role)
    lp_signaldirectionhotplug $port $direction
}

#Called by hotplug whenever a port transtions to link-down status
lp_onlinkdown() {
    local port=$1
    lp_setportdb $port $LP_DIRECTION_UNKNOWN
    #isolate port back into its own iface
    lp_isolateport $port
}

#Called by hotplug whenever a port direction has been determined upstream
lp_onupstream() {
    local port=$1
    lp_setportdb $port $LP_DIRECTION_UPSTREAM
    lp_moveporttoeth0 $port
}

#Called by hotplug whenever a port direction has been determined downstream
lp_ondownstream() {
    local port=$1
    lp_setportdb $port $LP_DIRECTION_DOWNSTREAM
    lp_moveporttoeth1 $port
}

repacd_lp_init() {
    local nports=$(lp_numports)
    for p in $(seq 1 $nports); do
        lp_isolateport $p
        lp_setportdb $p $LP_DIRECTION_UNKNOWN
    done
}
