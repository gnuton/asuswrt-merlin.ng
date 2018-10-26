# Helper makefile for building Broadcom wps libaries
# This file maps wps feature flags (import) to WPSFLAGS and WPSFILES (export).
#
# Copyright 2018 Broadcom
#
# This program is the proprietary software of Broadcom and/or
# its licensors, and may only be used, duplicated, modified or distributed
# pursuant to the terms and conditions of a separate, written license
# agreement executed between you and Broadcom (an "Authorized License").
# Except as set forth in an Authorized License, Broadcom grants no license
# (express or implied), right to use, or waiver of any kind with respect to
# the Software, and Broadcom expressly reserves all rights in and to the
# Software and all intellectual property rights therein.  IF YOU HAVE NO
# AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
# WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
# THE SOFTWARE.
#
# Except as expressly set forth in the Authorized License,
#
# 1. This program, including its structure, sequence and organization,
# constitutes the valuable trade secrets of Broadcom, and you shall use
# all reasonable efforts to protect the confidentiality thereof, and to
# use this information only in connection with your use of Broadcom
# integrated circuit products.
#
# 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
# "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
# REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
# OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
# DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
# NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
# ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
# CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
# OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
#
# 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
# BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
# SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
# IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
# IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
# ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
# OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
# NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
#
#
# <<Broadcom-WL-IPTag/Proprietary:>>
# $Id: $

WPSFILES :=
WPSBASE := components/apps/wps

ifeq ($(BLDTYPE), debug)
WPSFLAGS += -D_TUDEBUGTRACE
endif

ifeq ($(WCN_NET), 1)
WPSFLAGS += -DWCN_NET_SUPPORT
endif

# Include external openssl path
ifeq ($(EXTERNAL_OPENSSL),1)
WPS_CRYPT = 0
WPSFLAGS += -DEXTERNAL_OPENSSL
WPSFILES += $(WPSBASE)/common/shared/wps_openssl.c
endif

## wps common

## shared code
WPSFILES += $(WPSBASE)/common/shared/tutrace.c
WPSFILES += $(WPSBASE)/common/shared/dev_config.c
WPSFILES += $(WPSBASE)/common/shared/wps_sslist.c
WPSFILES += $(WPSBASE)/common/enrollee/enr_reg_sm.c
WPSFILES += $(WPSBASE)/common/registrar/reg_sm.c
WPSFILES += $(WPSBASE)/common/shared/reg_proto_utils.c
WPSFILES += $(WPSBASE)/common/shared/reg_proto_msg.c
WPSFILES += $(WPSBASE)/common/shared/tlv.c
WPSFILES += $(WPSBASE)/common/shared/state_machine.c
WPSFILES += $(WPSBASE)/common/shared/wps_utils.c
WPSFILES += $(WPSBASE)/common/shared/ie_utils.c
WPSFILES += $(WPSBASE)/common/shared/buffobj.c

# AP or APSTA
ifeq ($(WPS_AP), 1)
WPSFLAGS += -DBCMWPSAP
WPSFILES += $(WPSBASE)/common/ap/ap_api.c
WPSFILES += $(WPSBASE)/common/ap/ap_ssr.c
WPSFILES += $(WPSBASE)/common/ap/ap_eap_sm.c
endif

# STA supports
ifeq ($(WPS_STA), 1)
WPSFLAGS += -DBCMWPSAPSTA
WPSFILES += $(WPSBASE)/common/sta/sta_eap_sm.c
endif

# WPS monitor support
ifeq ($(WPS_ROUTER), 1)
WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_monitor.c
WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_aplockdown.c
WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_pb.c
WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_eap.c
WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_ie.c
WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_ui.c
WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_led.c
WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_apputils.c

WPS_ROUTERHALFILES += brcm_apps/arch/bcm947xx/wps_gpio.c
WPS_ROUTERHALFILES += brcm_apps/arch/bcm947xx/wps_hal.c
WPS_ROUTERHALFILES += brcm_apps/arch/bcm947xx/wps_wl.c

	# WFI supports
	ifeq ($(WPS_WFI),1)
	WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_wfi.c
	WPSFLAGS += -DBCMWFI
	endif

	ifeq ($(WPS_AP), 1)
	WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_ap.c
		ifeq ($(WPS_UPNP_DEVICE),1)
			WPSFILES += $(WPSBASE)/brcm_apps/upnp/WFADevice/soap_x_wfawlanconfig.c
			WPSFILES += $(WPSBASE)/brcm_apps/upnp/WFADevice/WFADevice.c
			WPSFILES += $(WPSBASE)/brcm_apps/upnp/WFADevice/WFADevice_table.c
			WPSFILES += $(WPSBASE)/brcm_apps/upnp/WFADevice/xml_x_wfawlanconfig.c
			# Release xml_WFADevice.c for customization
			WPS_ROUTERHALFILES += brcm_apps/upnp/WFADevice/xml_WFADevice.c
			WPSFILES += $(WPSBASE)/common/ap/ap_upnp_sm.c
			WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_libupnp.c
			WPSFLAGS += -DWPS_UPNP_DEVICE
		endif
	endif

	ifeq ($(WPS_STA), 1)
	WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_sta.c
	WPS_ROUTERHALFILES += brcm_apps/arch/bcm947xx/wps_sta_wl.c
	endif

	# NFC support
	ifeq ($(WPS_NFC_DEVICE), 1)
	WPSFILES += $(WPSBASE)/brcm_apps/apps/wps_nfc.c
	WPSFILES += $(WPSBASE)/brcm_apps/nfc/app_generic.c
	WPSFILES += $(WPSBASE)/brcm_apps/nfc/app_mgt.c
	WPSFILES += $(WPSBASE)/brcm_apps/nfc/app_nsa_utils.c
	endif
WPSFLAGS += -DWPS_ROUTER
endif # end WPS ROUTER

# Enrollee supports
ifeq ($(WPS_ENR),1)
WPSFILES += $(WPSBASE)/common/enrollee/enr_api.c
endif

ifeq ($(WPS_CRYPT), 1)
CRYPTDIR = $(WLAN_TreeBaseR)/components/bcmcrypto
CRYPTBASE := components/bcmcrypto
BCMCRYPTFILES := $(CRYPTBASE)/src/aes.c
BCMCRYPTFILES += $(CRYPTBASE)/src/rijndael-alg-fst.c
BCMCRYPTFILES += $(CRYPTBASE)/src/dh.c
BCMCRYPTFILES += $(CRYPTBASE)/src/bn.c
BCMCRYPTFILES += $(CRYPTBASE)/src/sha2.c
BCMCRYPTFILES += $(CRYPTBASE)/src/sha2x.c
BCMCRYPTFILES += $(CRYPTBASE)/src/md5.c
BCMCRYPTFILES += $(CRYPTBASE)/src/random.c
endif

# NFC support
ifeq ($(WPS_NFC_DEVICE), 1)
WPSFILES += $(WPSBASE)/common/shared/nfc_utils.c
WPSFLAGS += -DWPS_NFC_DEVICE
endif

export WPS_FLAGS = $(WPSFLAGS)
export WPS_FILES = $(WPSFILES)
export BCM_CRYPT_FILES = $(BCMCRYPTFILES)
export WPS_HALFILES = $(WPS_ROUTERHALFILES)
