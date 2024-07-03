#!/bin/sh

wget_options="-q -t 2 -T 30"

fwsite="https://raw.githubusercontent.com/gnuton/asuswrt-merlin.ng/master/updates"

nvram set webs_state_update=0 # INITIALIZING
nvram set webs_state_flag=0   # 0: Don't do upgrade  1: Do upgrade
nvram set webs_state_error=0
nvram set webs_state_url=""

#openssl support rsa check
IS_SUPPORT_NOTIFICATION_CENTER=$(nvram get rc_support|grep -i nt_center)
if [ "$IS_SUPPORT_NOTIFICATION_CENTER" != "" ]; then
. /tmp/nc/event.conf
fi
# current firmware information
current_base=$(nvram get firmver | sed "s/\.//g")
current_firm=$(nvram get buildno | cut -d. -f1)
current_buildno=$(nvram get buildno | cut -d. -f2)
#Extract extendno, subtract value by 1 if it contains "alpha/beta", remove all other values such as "_rog" or "-g*"
current_extendno="$(nvram get extendno | awk -F'[_-]' '{n=$1} /[aA]lpha|[bB]eta/{n--} END{print n}')"

# get firmware information
model=$(nvram get productid)
model="$model#"

echo "---- update real normal----" > /tmp/webs_upgrade.log
/usr/sbin/wget $wget_options $fwsite/manifest2.txt -O /tmp/wlan_update.txt

if [ "$?" != "0" ]; then
	nvram set webs_state_error=1
else

	fullver="$(grep "$model" /tmp/wlan_update.txt | tail -n1 | sed 's/.*#FW//')"
	fullver=$(echo $fullver | sed s/#.*//;)
	firmbase=$(echo $fullver | cut -d. -f1)
	firmver=$(echo $fullver | cut -d. -f2)
	buildno=$(echo $fullver | cut -d. -f3)

	extendno="$(grep "$model" /tmp/wlan_update.txt | tail -n1 | sed 's/.*#EXT//')"
	extendno=$(echo $extendno | sed s/#.*//;)
	lextendno=$(echo $extendno | sed s/-g.*//;)

	nvram set webs_state_info=${firmbase}_${firmver}_${buildno}_${extendno}

	rm -f /tmp/wlan_update.*
fi

echo "---- Have ${current_base}.${current_firm}.${current_buildno}_${current_extendno}----" >> /tmp/webs_upgrade.log
echo "---- Stable available ${firmbase}.${firmver}.${buildno}_${extendno}----" >> /tmp/webs_upgrade.log

update_webs_state_info=$(nvram get webs_state_info)
last_webs_state_info=$(nvram get webs_last_info)
if [ "$firmbase" == "" ] || [ "$firmver" == "" ] || [ "$buildno" == "" ] || [ "$lextendno" == "" ]; then
	nvram set webs_state_error=1	# exist no Info
else

	if [ "$current_base" -lt "$firmbase" ]; then
	        newfirm=1
	elif [ "$current_base" -eq "$firmbase" ] && [ "$current_firm" -lt "$firmver" ]; then
	        newfirm=1
	elif [ "$current_base" -eq "$firmbase" ] && [ "$current_firm" -eq "$firmver" ] && [ "$current_buildno" -lt "$buildno" ]; then
	        newfirm=1
	elif [ "$current_base" -eq "$firmbase" ] && [ "$current_firm" -eq "$firmver" ] && [ "$current_buildno" -eq "$buildno" ] && [ "$current_extendno" -lt "$lextendno" ]; then
		newfirm=1
	else
		newfirm=0
	fi

	if [ "$newfirm" -eq "1" ]; then
		echo "---- Update available" >> /tmp/webs_upgrade.log
		nvram set webs_state_flag=1	# Do upgrade
		if [ "$IS_SUPPORT_NOTIFICATION_CENTER" != "" ]; then
			if [ "$last_webs_state_info" != "$update_webs_state_info" ]; then
				Notify_Event2NC "$SYS_FW_NWE_VERSION_AVAILABLE_EVENT" "{\"fw_ver\":\"$update_webs_state_info\"}"
				nvram set webs_last_info="$update_webs_state_info"
			fi
		fi
	fi
fi
# download releasee note
webs_state_flag=$(nvram get webs_state_flag)

if [ "$webs_state_flag" -eq "1" ]; then
	releasenote_file0=$(nvram get webs_state_info)_note.txt
	releasenote_path0="/tmp/release_note0.txt"
	echo "---- download real release note ----" >> /tmp/webs_upgrade.log
	/usr/sbin/wget $wget_options $fwsite/$releasenote_file0_US -O $releasenote_path0
	echo "---- $fwsite/$releasenote_file0_US ----" >> /tmp/webs_upgrade.log

	if [ "$?" != "0" ]; then
		echo "---- download $fwsite/$releasenote_file0 failed ----" >> /tmp/webs_upgrade.log
		nvram set webs_state_error=1
  	else
		echo "---- $fwsite/$releasenote_file0 ----" >> /tmp/webs_upgrade.log
	fi
fi

nvram set webs_state_update=1
