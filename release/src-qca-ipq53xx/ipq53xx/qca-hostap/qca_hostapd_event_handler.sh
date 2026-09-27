#!/bin/sh

debug_on="/tmp/hapd_dbg"
script_name="qca_hostapd_event_handler.sh"

# [ -e $LED_VARS_FILE ] && . $LED_VARS_FILE

# Due to a change in hostapd event input we change parameters settings:
# VAP name is added as third parameter
# input examples:
#   disconnect VAP:
#   /opt/lantiq/wave/scripts/fapi_wlan_wave_events_hostapd.sh wlan0 AP-STA-DISCONNECTED wlan0.0 30:5a:3a:18:bd:7b
#interface_name=$1
name=$2
#radio_name=$1
interface_name=$3
param3=$4
param4=$5


if [ -f $debug_on ]; then
	echo "1($1) 2($2) 3($3) 4($4) 5($5)" > /dev/console
fi

case $name in
	"WPS-FAIL")
	#	WPS_STATE="WPS_IDLE"
	
	# event example (PIN Error): 
	#   wlan2 WPS-FAIL wlan2 msg=8 config_error=18
	#From the code:
	#	enum wps_config_error {
	#		WPS_CFG_NO_ERROR = 0,
	#		WPS_CFG_OOB_IFACE_READ_ERROR = 1,
	#		WPS_CFG_DECRYPTION_CRC_FAILURE = 2,
	#		WPS_CFG_24_CHAN_NOT_SUPPORTED = 3,
	#		WPS_CFG_50_CHAN_NOT_SUPPORTED = 4,
	#		WPS_CFG_SIGNAL_TOO_WEAK = 5,
	#		WPS_CFG_NETWORK_AUTH_FAILURE = 6,
	#		WPS_CFG_NETWORK_ASSOC_FAILURE = 7,
	#		WPS_CFG_NO_DHCP_RESPONSE = 8,
	#		WPS_CFG_FAILED_DHCP_CONFIG = 9,
	#		WPS_CFG_IP_ADDR_CONFLICT = 10,
	#		WPS_CFG_NO_CONN_TO_REGISTRAR = 11,
	#		WPS_CFG_MULTIPLE_PBC_DETECTED = 12,
	#		WPS_CFG_ROGUE_SUSPECTED = 13,
	#		WPS_CFG_DEVICE_BUSY = 14,
	#		WPS_CFG_SETUP_LOCKED = 15,
	#		WPS_CFG_MSG_TIMEOUT = 16,
	#		WPS_CFG_REG_SESS_TIMEOUT = 17,
	#		WPS_CFG_DEV_PASSWORD_AUTH_FAILURE = 18,
	#		WPS_CFG_60G_CHAN_NOT_SUPPORTED = 19,
	#		WPS_CFG_PUBLIC_KEY_HASH_MISMATCH = 20
	#	}
		#code=`${event##*config_error=}`
		code=`echo $param4 | awk -F "=" '{print $2}'`
		if [ -f $debug_on ]; then
			echo "$script_name : unknown error event, code=$code" > /dev/console
		fi
	;;
	"AP-STA-PROBEREQ")
		if [ -f $debug_on ]; then
			echo "show 0~4 of p3 is ${3:0:4}" > /dev/console
		fi
		if [ "${3:0:4}" = "0101" ]
		then
			hapdevent $name $3
		fi
	;;
	"DFS-RADAR-DETECTED")
		hapdevent $name $3 $4
	;;
	*)
	;;
esac
