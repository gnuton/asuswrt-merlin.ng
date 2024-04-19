/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * Copyright  (C) 2019-2020  MediaTek Inc. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include <stdlib.h>
#include <stdio.h>
#include "wdev.h"
#include "driver_wext.h"
#include "hotspot.h"
#include "wapp_cmm.h"
#include "wps.h"

#define WPS_ON_5GL 1
#define WPS_ON_5GH 0
void wps_ctrl_run_ap_wps(struct wifi_app *wapp)
{
	struct wapp_dev *wdev_temp = NULL;
	struct dl_list *dev_list;
	BOOLEAN uuid_set = FALSE;
	BOOLEAN wsc_2p4_done = FALSE;
	BOOLEAN wsc_5G_done = FALSE;
	char uuid_buffer[16] = {0};
	char cmd[512] = {0};
	char radio_5GL=0,radio_5GH=0;
	struct ap_dev * ap = NULL;
	DBGPRINT(RT_DEBUG_OFF, "%s\n", __func__);
	dev_list = &wapp->dev_list;
	dl_list_for_each(wdev_temp, dev_list, struct wapp_dev, list){
		ap = (struct ap_dev *)wdev_temp->p_dev;
		if (wdev_temp->dev_type != WAPP_DEV_TYPE_AP)
			continue;
		if (ap == NULL)
			continue;
		if (ap->isActive != WAPP_BSS_START)
			continue;
		if (wdev_temp->radio->op_ch > 14 && wdev_temp->radio->op_ch < 100)
		{
			radio_5GL=1;
		}
		else if(wdev_temp->radio->op_ch >= 100)
		{
			radio_5GH=1;
		}
	}
	dl_list_for_each(wdev_temp, dev_list, struct wapp_dev, list){
		if (wdev_temp->dev_type != WAPP_DEV_TYPE_AP)
			continue;
		if (!wdev_temp->i_am_fh_bss)
			continue;	
#ifdef WIFI_MD_COEX_SUPPORT
		if (!wdev_temp->radio->operatable) {
			DBGPRINT(RT_DEBUG_OFF, "not operatbale for current wdev(%s)\n", wdev_temp->ifname);
			continue;
		}
#endif
		ap = (struct ap_dev *)wdev_temp->p_dev;
		if ((ap->bss_info.hidden_ssid)||(ap->bss_info.enc_type==ENCRYP_WEP) || (ap->bss_info.enc_type == ENCRYP_TKIP))
		{
			//printf("continue as either hidden %d or wep support %d\n",ap->bss_info.hidden_ssid, ap->bss_info.enc_type);
			continue;
		}
		if (wdev_temp->radio->op_ch <= 14 && wsc_2p4_done)
			continue;

		if(radio_5GL && radio_5GH && wapp->map->enable_wps_toggle_5GL_5GH && wapp->map->WPS_Fh_Fail)
		{
			if((wdev_temp->radio->op_ch >= 100)&& (wapp->map->g_LastWPS_ran_on==WPS_ON_5GH))
				continue;
			else if(wdev_temp->radio->op_ch > 14 && wdev_temp->radio->op_ch < 100 &&(wapp->map->g_LastWPS_ran_on==WPS_ON_5GL))
				continue;
		}
		if (wdev_temp->radio->op_ch > 14 && wsc_5G_done)
				continue;
				
		if (wdev_temp->radio->op_ch <= 14)
			wsc_2p4_done = TRUE;
		
		if (wdev_temp->radio->op_ch > 14)
		{
			wsc_5G_done = TRUE;
			wapp->map->g_LastWPS_ran_on=!wapp->map->g_LastWPS_ran_on;
			wapp->map->WPS_Fh_Fail=0;
		}
		driver_wext_get_set_uuid(wapp->drv_data, wdev_temp->ifname,
			uuid_buffer, uuid_set);
		uuid_set = TRUE;
		os_memset(cmd, 0, sizeof(cmd));
#ifdef HOSTAPD_MAP_SUPPORT
		os_snprintf(cmd,sizeof(cmd), "hostapd_cli -i%s wps_pbc", wdev_temp->ifname);
#else
		os_snprintf(cmd,sizeof(cmd),
			"iwpriv %s set WscConfMode=4;iwpriv %s set WscMode=2;iwpriv %s set WscConfStatus=2;iwpriv %s set WscGetConf=1",
			wdev_temp->ifname, wdev_temp->ifname, wdev_temp->ifname, wdev_temp->ifname);
#endif /* HOSTAPD_MAP_SUPPORT */
		system(cmd);
		DBGPRINT(RT_DEBUG_OFF, "%s\n", cmd);
	}

}
void *wps_ctrl_run_cli_wps(struct wifi_app *wapp,
	struct wapp_dev *wdev)
{
	struct wapp_dev *wdev_temp = NULL;
	struct dl_list *dev_list;
	char cmd[256] = {0};
	DBGPRINT(RT_DEBUG_OFF, "%s\n", __func__);
	dev_list = &wapp->dev_list;

	if (wdev == NULL) {
		DBGPRINT(RT_DEBUG_TRACE, "start from very first CLI in the list\n");
		dl_list_for_each(wdev, dev_list, struct wapp_dev, list){
			if (wdev->dev_type != WAPP_DEV_TYPE_STA)
			{
				continue;
			}
#ifdef WIFI_MD_COEX_SUPPORT
			if (!wdev->radio->operatable) {
				DBGPRINT(RT_DEBUG_OFF, "wps_ctrl_run_cli_wps:not operatable for current wdev(%s)\n",
					wdev->ifname);
				continue;
			}
#endif
			if (wdev->dev_type == WAPP_DEV_TYPE_STA) {
				if (!wdev->wps_triggered) {
					DBGPRINT(RT_DEBUG_OFF, "TRIGGER WPS on interface --->%s\n", wdev->ifname);
					wdev->wps_triggered = TRUE;
					os_snprintf(cmd,sizeof(cmd),
						"iwpriv %s set ApCliSsid=MAP_RESET;iwpriv %s set ApCliBssid=00:00:00:00:00:01",
						wdev->ifname, wdev->ifname);
					system(cmd);
					DBGPRINT(RT_DEBUG_TRACE, "%s\n", cmd);
					os_memset(cmd, 0, sizeof(cmd));
					os_snprintf(cmd,sizeof(cmd),
						"iwpriv %s set ApCliEnable=1;iwpriv %s set WscConfMode=1;iwpriv %s set WscMode=2;iwpriv %s set WscGetConf=1",
						wdev->ifname, wdev->ifname, wdev->ifname, wdev->ifname);
					system(cmd);
					DBGPRINT(RT_DEBUG_TRACE, "%s\n", cmd);
				} else {
					DBGPRINT(RT_DEBUG_OFF, "WSC PBC EXEC on interface --->%s, pointer = %p\n",
						wdev->ifname, wdev);
					wapp_trigger_wsc_pbc_exec(wapp, wdev);
				}
				return (void *)wdev;
			}
		}
	} else {
		wdev_temp = (struct wapp_dev *)wdev->list.next;
		if (&wdev_temp->list == dev_list)
				wdev_temp = NULL;
		while(wdev_temp)
		{
			if (wdev_temp->dev_type == WAPP_DEV_TYPE_STA) {
				if (!wdev_temp->wps_triggered) {
					wdev_temp->wps_triggered = TRUE;
					DBGPRINT(RT_DEBUG_OFF,"TRIGGER WPS on interface --->%s\n", wdev_temp->ifname);
					os_snprintf(cmd,sizeof(cmd),
						"iwpriv %s set ApCliEnable=1;iwpriv %s set WscConfMode=1;iwpriv %s set WscMode=2;iwpriv %s set WscGetConf=1",
						wdev_temp->ifname, wdev_temp->ifname, wdev_temp->ifname, wdev_temp->ifname);
					system(cmd);
				} else {
					DBGPRINT(RT_DEBUG_OFF,"WSC PBC EXEC on interface --->%s, pointer =%p, dev_type = %d\n", 
						wdev_temp->ifname, wdev_temp, wdev_temp->dev_type);
					wapp_trigger_wsc_pbc_exec(wapp, wdev_temp);
				}
				eloop_register_timeout(WPS_TIMEOUT, 0, map_wps_timeout,
					wapp, &wapp->map->device_status);
				return (void *)wdev_temp;
			}
			wdev_temp = (struct wapp_dev *)wdev_temp->list.next;
			if (&wdev_temp->list == dev_list)
				wdev_temp = NULL;
		}
	}
	return NULL;
}

void stop_con_ap_wps(struct wifi_app *wapp, struct wapp_dev *wdev)
{
	struct dl_list *dev_list;
	struct wapp_dev *temp_wdev = NULL;
	char cmd[256] = {0};

	dev_list = &wapp->dev_list;
	DBGPRINT(RT_DEBUG_TRACE, "%s\n", __FUNCTION__);
	dl_list_for_each(temp_wdev, dev_list, struct wapp_dev, list)
	{
		if (temp_wdev->dev_type != WAPP_DEV_TYPE_AP)
		{
			continue;
		}
		if (temp_wdev == wdev)
			continue;
		if (temp_wdev && temp_wdev->dev_type == WAPP_DEV_TYPE_AP) {
			os_memset(cmd, 0, sizeof(cmd));
			os_snprintf(cmd,sizeof(cmd),
				"iwpriv %s set WscStop=1;",
				temp_wdev->ifname);
			system(cmd);
			DBGPRINT(RT_DEBUG_ERROR, "%s\n", cmd);
		}
	}
}

void stop_con_cli_wps(struct wifi_app *wapp, struct wapp_dev *wdev)
{
	struct dl_list *dev_list;
	struct wapp_dev *temp_wdev = NULL;
	char cmd[256] = {0};

	dev_list = &wapp->dev_list;
	dl_list_for_each(temp_wdev, dev_list, struct wapp_dev, list)
	{
		if (temp_wdev->dev_type != WAPP_DEV_TYPE_STA)
		{
			continue;
		}
		if (temp_wdev == wdev)
			continue;
		if (temp_wdev && temp_wdev->dev_type == WAPP_DEV_TYPE_STA) {
			DBGPRINT(RT_DEBUG_OFF,"stopping WPS for %s\n", temp_wdev->ifname);
			temp_wdev->wps_triggered = FALSE;
			os_memset(cmd, 0, sizeof(cmd));
			os_snprintf(cmd,sizeof(cmd),
				"iwpriv %s set WscStop=1;iwpriv %s set ApCliEnable=0;",
				temp_wdev->ifname, temp_wdev->ifname);
			system(cmd);
		}
	}
	if (wdev)
	{
		DBGPRINT(RT_DEBUG_OFF,"continue WPS on %s\n", wdev->ifname);
		wapp->wsc_configs_pending = TRUE;
		wapp_trigger_wsc_pbc_exec(wapp, wdev);
	}
}
void wps_ctrl_process_scan_results(struct wifi_app *wapp)
{
	struct wapp_dev *wdev = NULL;
	struct wapp_dev *wdev_with_pbc_peer = NULL;
	struct dl_list *dev_list;
	int overlapp_detected = FALSE;
	int i = 0;
	unsigned char TempUuid[16] = {0};
	DBGPRINT(RT_DEBUG_OFF,"PBC cycle completed for all available CLI interfaces\n");
	dev_list = &wapp->dev_list;
	dl_list_for_each(wdev, dev_list, struct wapp_dev, list)
	{
		if (wdev->dev_type != WAPP_DEV_TYPE_STA)
		{
			continue;
		}
		if (wdev->wsc_scan_info.bss_count > 1) {
			DBGPRINT(RT_DEBUG_OFF,"overlapp detected for --->%s\n", wdev->ifname);
			overlapp_detected = TRUE;
			break;
		}
		if (wdev->wsc_scan_info.bss_count == 1)
		{
			if (wdev_with_pbc_peer == NULL)
			{
				DBGPRINT(RT_DEBUG_TRACE,"first CLI with pbc peer found --->%s\n", wdev->ifname);
				if(wdev->bh_connect_priority != 0) {
					wdev_with_pbc_peer = wdev;
				}
				os_memcpy(TempUuid, wdev->wsc_scan_info.Uuid, sizeof(TempUuid));
			} else {
				DBGPRINT(RT_DEBUG_TRACE,"Additional CLI with pbc peer found --->%s\n", wdev->ifname);
				if (os_memcmp(TempUuid, wdev->wsc_scan_info.Uuid, sizeof(TempUuid)))
				{
					DBGPRINT(RT_DEBUG_OFF,"overlapp detected between %s & %s\n", wdev->ifname, 
						wdev_with_pbc_peer->ifname);
					while(i < 16)
					{
						DBGPRINT(RT_DEBUG_OFF,"%02x\t", TempUuid[i]);
						i++;
					}
					printf("\n");
					i = 0;
					while(i < 16)
					{
						DBGPRINT(RT_DEBUG_OFF,"%02x\t", wdev->wsc_scan_info.Uuid[i]);
						i++;
					}
					DBGPRINT(RT_DEBUG_OFF,"\n");
					overlapp_detected = TRUE;
					break;
				}
				if (wdev_with_pbc_peer->bh_connect_priority >= wdev->bh_connect_priority &&
					(wdev->bh_connect_priority != 0))
				{
					if (wdev_with_pbc_peer->bh_connect_priority == wdev->bh_connect_priority) {
						if (wdev->radio && IS_MAP_CH_5G(wdev->radio->op_ch)) {
							wdev_with_pbc_peer = wdev;
						}
					} else
						wdev_with_pbc_peer = wdev;
				}
			}
		}
	}
	if (overlapp_detected) {
		wapp_device_status *device_status = &wapp->map->device_status;
		eloop_cancel_timeout(map_wps_timeout, wapp, device_status);
		device_status->status_bhsta = STATUS_BHSTA_WPS_FAILED;
		wapp_send_1905_msg(
			wapp,
			WAPP_DEVICE_STATUS,
			sizeof(wapp_device_status),
			(char *)device_status);
		stop_con_cli_wps(wapp, NULL);
		}
	else if (wdev_with_pbc_peer)
		stop_con_cli_wps(wapp, wdev_with_pbc_peer);
	else {
		stop_con_cli_wps(wapp, wdev_with_pbc_peer);
		wapp->wsc_trigger_wdev
			= wps_ctrl_run_cli_wps(wapp, NULL);
	}
}

char *WscGetAuthTypeStr(
	unsigned short authFlag)
{
	switch (authFlag) {
	case WSC_AUTHTYPE_OPEN:
		return "OPEN";

	case WSC_AUTHTYPE_WPAPSK:
		return "WPAPSK";

	case WSC_AUTHTYPE_SHARED:
		return "SHARED";

	case WSC_AUTHTYPE_WPANONE:
		return "WPANONE";

	case WSC_AUTHTYPE_WPA:
		return "WPA";

	case WSC_AUTHTYPE_WPA2:
#ifdef MAP_R2
		/* this is only for APCLI */
		return "WPA2PSKWPA3PSK";
#else
		return "WPA2";
#endif

	default:
	case (WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK):
		return "WPAPSKWPA2PSK";

/*WPA3 Changes to test with MAP_R1*/
#ifdef MAP_SUPPORT

	case WSC_AUTHTYPE_SAE:
		return "WPA3PSK";

	case (WSC_AUTHTYPE_SAE | WSC_AUTHTYPE_WPA2PSK):
		return "WPA2PSKWPA3PSK";
#endif
	case (WSC_AUTHTYPE_WPA2PSK):
		return "WPA2PSK";

	case (WSC_AUTHTYPE_OPEN | WSC_AUTHTYPE_SHARED):
		return "WEPAUTO";
	}
}

char *WscGetEncryTypeStr(unsigned short encryFlag)
{
	switch (encryFlag) {
	case WSC_ENCRTYPE_NONE:
		return "NONE";

	case WSC_ENCRTYPE_WEP:
		return "WEP";

	case WSC_ENCRTYPE_TKIP:
		return "TKIP";

	default:
	case (WSC_ENCRTYPE_TKIP | WSC_ENCRTYPE_AES):
		return "TKIPAES";

	case WSC_ENCRTYPE_AES:
		return "AES";
	}
}

void read_system_command_output(char *system_command, char *output_buffer)
{
	char temp_file[] = "/tmp/system_command_output";
	char command[256] = {0};
	FILE *file;

	os_snprintf(command, sizeof(command), "%s > %s", system_command, temp_file);
	system(command);

	file = fopen(temp_file, "r");
	if (!file) {
		return;
	}
	fgets(output_buffer, 128, file);
	output_buffer[os_strlen(output_buffer) - 1] = '\0';
	printf("%s -----> %s\n", system_command, output_buffer);
	fclose(file);
}

unsigned char WscGetAuthType(char *AuthTypeString)
{
	if (!os_strcmp(AuthTypeString,"OPEN"))
		return WSC_AUTHTYPE_OPEN;
	else if (!os_strcmp(AuthTypeString,"WPAPSK"))
		return WSC_AUTHTYPE_WPAPSK;
	else if (!os_strcmp(AuthTypeString,"SHARED"))
		return WSC_AUTHTYPE_SHARED;
	else if (!os_strcmp(AuthTypeString,"WPANONE"))
		return WSC_AUTHTYPE_WPANONE;
	else if (!os_strcmp(AuthTypeString,"WPA"))
		return (WSC_AUTHTYPE_WPA);
	else if (!os_strcmp(AuthTypeString,"WPA2"))
		return (WSC_AUTHTYPE_WPA2);
	else if (!os_strcmp(AuthTypeString,"WPAPSKWPA2PSK"))
		return (WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK);
	else if (!os_strcmp(AuthTypeString,"WPA2PSK"))
		return (WSC_AUTHTYPE_WPA2PSK);
	else if (!os_strcmp(AuthTypeString,"WEPAUTO"))
		return (WSC_AUTHTYPE_OPEN | WSC_AUTHTYPE_SHARED);
	return 0;
	
}
unsigned char WscGetEncrypType(char *EncrypTypeString)
{
	if (!os_strcmp(EncrypTypeString,"NONE"))
		return WSC_ENCRTYPE_NONE;
	else if (!os_strcmp(EncrypTypeString,"WEP"))
		return WSC_ENCRTYPE_WEP;
	else if (!os_strcmp(EncrypTypeString,"TKIP"))
		return WSC_ENCRTYPE_TKIP;
	else if (!os_strcmp(EncrypTypeString,"AES"))
		return WSC_ENCRTYPE_AES;
	else if (!os_strcmp(EncrypTypeString,"TKIPAES"))
		return (WSC_ENCRTYPE_AES | WSC_ENCRTYPE_TKIP);
	return 0;
}
void read_backhaul_configs(struct wifi_app *wapp)
{
	int i = 0;
	wsc_apcli_config_msg *apcli_config_msg = NULL;
	wsc_apcli_config *apcli_config = NULL;
	char value[200] = {0};
	char param[65];
	char role[5] = {0};
	unsigned char config_count = 0;
	int msg_size = sizeof(wsc_apcli_config_msg) +
		sizeof(wsc_apcli_config)*MAX_NUM_OF_RADIO;
	os_alloc_mem(NULL, (unsigned char **)&apcli_config_msg, msg_size);
	os_memset(apcli_config_msg, 0, msg_size);
	get_map_parameters(wapp->map, "role_detection_external", value, NON_DRIVER_PARAM, sizeof(value));
	get_map_parameters(wapp->map, "DeviceRole", role, NON_DRIVER_PARAM, sizeof(role));
	if((!strcmp(value,"0")) || (!strcmp(value,"1") && strcmp(role, "1"))) {
		for (i = 0; i < MAX_NUM_OF_RADIO; i++) {
			if (wapp->map->apcli_configs[i].config_valid) {
				os_memcpy(&apcli_config_msg->apcli_config[config_count],
					&wapp->map->apcli_configs[i].apcli_config,
					sizeof(wsc_apcli_config));
				printf("BH Config SSID : %s\n", apcli_config_msg->apcli_config[config_count].ssid);
				config_count++;
			}
		}
		i=0;
		if (config_count == 0) {
		for (i = 0; i < MAX_NUM_OF_RADIO; i++)
		{
			u8 ra_id[8] = {0};
			apcli_config = &apcli_config_msg->apcli_config[i];
			os_snprintf(param, sizeof(param), "BhProfile%dValid", i);
			get_map_parameters(wapp->map, param, value, NON_DRIVER_PARAM, sizeof(value));
			if(strcmp(value,"1")) {
				break;
			}
			os_snprintf(param, sizeof(param), "BhProfile%dSsid", i);
			get_map_parameters(wapp->map, param, value, NON_DRIVER_PARAM, sizeof(apcli_config->ssid));
			os_snprintf(apcli_config->ssid, sizeof(apcli_config->ssid), "%s", value);
			apcli_config->SsidLen = os_strlen(apcli_config->ssid);

			os_snprintf(param, sizeof(param), "BhProfile%dWpaPsk", i);
			get_map_parameters(wapp->map, param, value, NON_DRIVER_PARAM, sizeof(apcli_config->Key));
			os_snprintf((char *)apcli_config->Key, sizeof(apcli_config->Key), "%s", value);
			apcli_config->KeyLength = os_strlen(value);

			os_snprintf(param, sizeof(param), "BhProfile%dAuthMode", i);
			get_map_parameters(wapp->map, param, value, NON_DRIVER_PARAM, sizeof(value));
			apcli_config->AuthType = WscGetAuthType(value);

			os_snprintf(param, sizeof(param), "BhProfile%dEncrypType", i);
			get_map_parameters(wapp->map, param, value, NON_DRIVER_PARAM, sizeof(value));
			apcli_config->EncrType = WscGetEncrypType(value);

			os_snprintf(param, sizeof(param), "BhProfile%dRaID", i);
			get_map_parameters(wapp->map, param, value, NON_DRIVER_PARAM, sizeof(value));
			os_memcpy(ra_id, value, sizeof(ra_id));

		        printf("Mark Config %d as valid\n", config_count);
		        wapp->map->apcli_configs[config_count].config_valid = 1;
		        os_memcpy(&wapp->map->apcli_configs[config_count].apcli_config,
		                        &apcli_config_msg->apcli_config[i],
		                        sizeof(wsc_apcli_config));
				os_memcpy(&wapp->map->apcli_configs[config_count].raid, ra_id, sizeof(ra_id));
				os_memset(ra_id, 0, sizeof(ra_id));
				config_count++;
			}
		}
		apcli_config_msg->profile_count = config_count;
		wapp_send_1905_msg(wapp, WAPP_MAP_BH_CONFIG, msg_size, (void *)apcli_config_msg);
	}
	os_free(apcli_config_msg);
}


void write_backhaul_configs(struct wifi_app *wapp, wsc_apcli_config_msg *bh_configs_msg)
{
	int i = 0;
	char param[65];
	char value[200];
	wsc_apcli_config *apcli_config = NULL;

	for (i = 0; i < bh_configs_msg->profile_count; i++)
	{
		apcli_config = &bh_configs_msg->apcli_config[i];
		os_memset(param, 0, sizeof(param));
		os_memset(value, 0, sizeof(value));

		os_snprintf(param, sizeof(param), "BhProfile%dSsid", i);
		save_map_parameters(wapp, param, (char *)apcli_config->ssid, NON_DRIVER_PARAM);

		os_snprintf(param, sizeof(param), "BhProfile%dAuthMode", i);
		os_snprintf(value, sizeof(value),"%s",WscGetAuthTypeStr(apcli_config->AuthType));
		save_map_parameters(wapp, param, value, NON_DRIVER_PARAM);

		os_snprintf(param, sizeof(param), "BhProfile%dEncrypType", i);
		os_snprintf(value, sizeof(value),"%s",WscGetEncryTypeStr(apcli_config->EncrType));
		save_map_parameters(wapp, param, value, NON_DRIVER_PARAM);

		os_snprintf(param, sizeof(param), "BhProfile%dWpaPsk", i);
		save_map_parameters(wapp, param, (char *)apcli_config->Key, NON_DRIVER_PARAM);

		os_snprintf(param, sizeof(param), "BhProfile%dValid", i);
		save_map_parameters(wapp, param, "1", NON_DRIVER_PARAM);
	}
}

void write_configs(struct wifi_app *wapp, wsc_apcli_config *apcli_config, int i, char *ra_match)
{
	char param[65];
	char value[200];
	os_memset(param, 0, sizeof(param));
	os_memset(value, 0, sizeof(value));

	os_snprintf(param, sizeof(param), "BhProfile%dSsid", i);
	save_map_parameters(wapp, param, (char *)apcli_config->ssid, NON_DRIVER_PARAM);

	os_snprintf(param, sizeof(param), "BhProfile%dAuthMode", i);
	os_snprintf(value, sizeof(value),"%s",WscGetAuthTypeStr(apcli_config->AuthType));
	save_map_parameters(wapp, param, value, NON_DRIVER_PARAM);

	os_snprintf(param, sizeof(param), "BhProfile%dEncrypType", i);
	os_snprintf(value, sizeof(value),"%s",WscGetEncryTypeStr(apcli_config->EncrType));
	save_map_parameters(wapp, param, value, NON_DRIVER_PARAM);

	os_snprintf(param, sizeof(param), "BhProfile%dWpaPsk", i);
	save_map_parameters(wapp, param, (char *)apcli_config->Key, NON_DRIVER_PARAM);

	os_snprintf(param, sizeof(param), "BhProfile%dValid", i);
	save_map_parameters(wapp, param, "1", NON_DRIVER_PARAM);

	if(ra_match) {
		os_snprintf(param, sizeof(param), "BhProfile%dRaID", i);
		os_snprintf(value, sizeof(value), "%s", ra_match);
		save_map_parameters(wapp, param, value, NON_DRIVER_PARAM);
	}
}


void update_cli_config(wsc_apcli_config_wrapper *apcli_config_wrapper, wsc_apcli_config *apcli_config, char *ra_match)
{
	apcli_config_wrapper->config_valid = TRUE;
	os_memcpy(apcli_config_wrapper->raid, ra_match, sizeof(apcli_config_wrapper->raid));
	os_memcpy(&(apcli_config_wrapper->apcli_config),apcli_config, sizeof(wsc_apcli_config));
}

void write_backhaul_configs_all(struct wifi_app *wapp, wsc_apcli_config_msg *bh_configs_msg, struct map_radio_identifier *ra_identifier)
{
	int i = 0, j = 0, k = 0;
	wsc_apcli_config *apcli_config = NULL;
	char ra_match[8] = {0};

	os_snprintf(ra_match, sizeof(ra_match), "%02x:%02x", ra_identifier->card_id, ra_identifier->ra_id);

	if (bh_configs_msg->profile_count == 0) {
		for (k = 0;k < MAX_NUM_OF_RADIO; k++)
		{
			char param[64] = {0};
			if (os_strcmp((char *)wapp->map->apcli_configs[k].raid,
				ra_match)==0)
			{
				wapp->map->apcli_configs[k].config_valid =
					FALSE;
				os_snprintf(param, sizeof(param),
					"BhProfile%dValid", k);
				save_map_parameters(wapp,param,
					"0", NON_DRIVER_PARAM);
			}
		}
	}

	for (i = 0; j < bh_configs_msg->profile_count && i < MAX_NUM_OF_RADIO; i++)
	{
		 if (wapp->map->apcli_configs[i].config_valid){
			if(os_strcmp((char *)wapp->map->apcli_configs[i].raid,ra_match)==0){
				apcli_config = &bh_configs_msg->apcli_config[j++];
				write_configs(wapp,apcli_config, i, ra_match);
				update_cli_config(&(wapp->map->apcli_configs[i]), apcli_config, ra_match);
				break;
			}
		} else {
			apcli_config = &bh_configs_msg->apcli_config[j++];
			write_configs(wapp,apcli_config, i, ra_match);
			update_cli_config(&(wapp->map->apcli_configs[i]), apcli_config, ra_match);
		}
	}
}

