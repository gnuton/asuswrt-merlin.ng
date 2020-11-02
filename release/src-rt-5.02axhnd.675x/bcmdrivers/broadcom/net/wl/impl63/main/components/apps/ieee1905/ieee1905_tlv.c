/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2013:proprietary:standard
 *
 *  This program is the proprietary software of Broadcom and/or its
 *  licensors, and may only be used, duplicated, modified or distributed pursuant
 *  to the terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied), right
 *  to use, or waiver of any kind with respect to the Software, and Broadcom
 *  expressly reserves all rights in and to the Software and all intellectual
 *  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 *  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 *  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1. This program, including its structure, sequence and organization,
 *     constitutes the valuable trade secrets of Broadcom, and you shall use
 *     all reasonable efforts to protect the confidentiality thereof, and to
 *     use this information only in connection with your use of Broadcom
 *     integrated circuit products.
 *
 *  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 *     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 *     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
 *     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
 *     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
 *     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
 *     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
 *     PERFORMANCE OF THE SOFTWARE.
 *
 *  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
 *     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 *     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
 *     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
 *     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
 *     LIMITED REMEDY.
 * :>
 *
 * $Change: 116460 $
 ***********************************************************************/

/*
 * IEEE1905 TLVs
 */

#define __USE_XOPEN
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "ieee1905_message.h"
#include "ieee1905_datamodel_priv.h"
#include "ieee1905_tlv.h"
#include "ieee1905_json.h"
#include "ieee1905_trace.h"
#include "ieee1905_utils.h"
#include "ieee1905_wlcfg.h"

#include <security_ipc.h>

#define I5_TRACE_MODULE i5TraceTlv

#define I5_SWAP(a, b, T) do { T _t; _t = a; a = b; b = _t; } while (0)

/* Modify i5_tlv_list_type_name when modifying this */
typedef enum i5_tlv_types {
  i5TlvEndOfMessageType = 0,
  i5TlvAlMacAddressType,
  i5TlvMacAddressType,
  i5TlvDeviceInformationType,
  i5TlvDeviceBridgingCapabilityType,
  i5TlvMediaType,
  i5TlvLegacyNeighborDeviceType,
  i5Tlv1905NeighborDeviceType,
  i5TlvLinkMetricQueryType,
  i5TlvTransmitterLinkMetricType,
  i5TlvReceiverLinkMetricType,
  i5TlvVendorSpecificType,
  i5TlvLinkMetricResultCodeType,
  i5TlvSearchedRoleType,
  i5TlvAutoconfigFreqBandType,
  i5TlvSupportedRoleType,
  i5TlvSupportedFreqBandType,
  i5TlvWscType,
  i5TlvPushButtonEventNotificationType,
  i5TlvPushButtonJoinNotificationType,   // UNUSED
  i5TlvGenericPhyDevInfoType,
  i5TlvDeviceIdentificationType,         // UNUSED
  i5TlvControlUrlType,                   // UNUSED
  i5TlvIpv4Type,                         // UNUSED
  i5TlvIpv6Type,                         // UNUSED
  i5TlvPushButtonGenericPhyEventNotificationType,

#ifdef MULTIAP
  i5TlvSupportedServicesType			= 0x80,
  i5TlvSearchedServicesType			= 0x81,
  i5TlvAPRadioIndentifierType			= 0x82,
  i5TlvAPOperationalBSSType			= 0x83,
  i5TlvAssocaitedClientsType			= 0x84,
  i5TlvAPRadioBasicCapabilitiesType		= 0x85,
  i5TlvAPHTCapabilitiesType			= 0x86,
  i5TlvAPVHTCapabilitiesType			= 0x87,
  i5TlvAPHECapabilitiesType			= 0x88,
  i5TlvSteeringPolicyType			= 0x89,
  i5TlvMetricReportingPolicyType		= 0x8a,
  i5TlvChannelPreferenceType			= 0x8b,
  i5TlvRadioOperationRestrictionType		= 0x8c,
  i5TlvTransmitPowerLimitType			= 0x8d,
  i5TlvChannelSelectionResponseType		= 0x8e,
  i5TlvOperatingChannelReportType		= 0x8f,
  i5TlvClientInfoType				= 0x90,
  i5TlvClientCapabilityReportType		= 0x91,
  i5TlvClientAssociationEventType		= 0x92,
  i5TlvAPMetricQueryType			= 0x93,
  i5TlvAPMetricsType				= 0x94,
  i5TlvSTAMACAddressType			= 0x95,
  i5TlvAssociatedSTALinkMetricsType		= 0x96,
  i5TlvUnAssociatedSTALinkMetricsQueryType	= 0x97,
  i5TlvUnAssociatedSTALinkMetricsResponseType	= 0x98,
  i5TlvBeaconMetricsQueryType			= 0x99,
  i5TlvBeaconMetricsResponseType		= 0x9a,
  i5TlvSteeringRequestType			= 0x9b,
  i5TlvSteeringBTMReportType			= 0x9c,
  i5TlvClientAssociationControlRequestType	= 0x9d,
  i5TlvBackhaulSteeringRequestType		= 0x9e,
  i5TlvBackhaulSteeringResponseType		= 0x9f,
  i5TlvHigherLayerDataType			= 0xa0,
  i5TlvAPCapabilityType				= 0xa1,
  i5TlvAssociatedSTATrafficStatsType		= 0xa2,
  i5TlvErrorCodeType                            = 0xa3,
#if defined(MULTIAPR2)
  i5TlvChannelScanReportingPolicyType		= 0xa4,
  i5TlvChannelScanCapabilitiesType		= 0xa5,
  i5TlvChannelScanRequestType			= 0xa6,
  i5TlvChannelScanResultType			= 0xa7,
  i5TlvTimestampType				= 0xa8,
  i5TlvCACRequestType				= 0xad,
  i5TlvCACTerminationType			= 0xae,
  i5TlvCACCompletionReportType			= 0xaf,
  i5TlvCACStatusReportType			= 0xb1,
  i5TlvCACCapabilitiesType			= 0xb2,
  i5TlvMultiAPProfileType			= 0xb3,
  i5TlvProfile2APCapabilityType			= 0xb4,
  i5TlvDefault8021QSettingsType			= 0xb5,
  i5TlvTrafficSeparationPolicyType		= 0xb6,
  i5TlvServicePrioritizationRuleType		= 0xb9,
  i5TlvR2ErrorCodeType				= 0xbc,
  i5TlvAPRadioAdvancedCapabilitiesType		= 0xbe,
  i5TlvAssociationStatusNotificationType	= 0xbf,
  i5TlvSourceInfoType				= 0xc0,/* TODO: correct in spec v1.1 */
  i5TlvTunneledMessgeType			= 0xc1,
  i5TlvTunneledType				= 0xc2,
  i5TlvProfile2SteeringRequestType              = 0xc3,
  i5TlvUnsuccessfulAssociationPolicyType	= 0xc4,
  i5TlvMetricCollectionIntervalType             = 0xc5,
  i5TlvRadioMetricsType                         = 0xc6,
  i5TlvAPExtendedMetricsType                    = 0xc7,
  i5TlvAssociatedSTAExtendedLinkMetricsType	= 0xc8,
  i5TlvStatusCodeType				= 0xc9,
  i5TlvReasonCodeType				= 0xca,
#endif /* MULTIAPR2 */
#endif /* MULTIAP */
  i5TlvBrcmRoutingTableType			= 0xF0,
  i5TlvBrcmFriendlyNameType			= 0xF1,
  i5TlvBrcmFriendlyUrlType			= 0xF2,
  i5TlvBrcmFriendlyIpv4Type			= 0xF4,
  i5TlvBrcmFriendlyIpv6Type			= 0xF6,
} i5_tlv_types_t;

#ifdef MULTIAP
/* Supported Services */
enum {
  i5MultiAPController = 0,	/* Multi AP Controller service supported */
  i5MultiAPAgent,		/* Multi AP Agent service supported */
};
#endif /* MULTIAP */

/* LLDP TLV Types */
#define I5_LLDP_CHASIS_ID_TLV_TYPE	0x2
#define I5_LLDP_PORT_ID_TLV_TYPE	0x4

/* Vendor specifics */
#define i5TlvVendorSpecificOui_Byte1 0x00
#define i5TlvVendorSpecificOui_Byte2 0x10
#define i5TlvVendorSpecificOui_Byte3 0x18
#define i5TlvVendorSpecificOui_Length 3

/* AP Radio Advanced Capabilities traffic separation flag */
#define I5_TLV_TS_COMBINED_FH_P1BH_SUPPORT 0x80  /* bit 7 */
#define I5_TLV_TS_COMBINED_P1BH_P2BH_SUPPORT 0x40  /* bit 6 */

typedef struct i5_tlv_types_name {
	i5_tlv_types_t tlvType;
	char tlvName[40];
} i5_tlv_types_name_t;

static i5_tlv_types_name_t i5_tlv_list_type_name[] = {
  {i5TlvEndOfMessageType, "End of Message"},
  {i5TlvAlMacAddressType, "Al MAC Address"},
  {i5TlvMacAddressType, "MAC Address"},
  {i5TlvDeviceInformationType, "Device Information"},
  {i5TlvDeviceBridgingCapabilityType, "Device Bridge Cap."},
  {i5TlvMediaType, "Media Type"},
  {i5TlvLegacyNeighborDeviceType, "Legacy Neighbor Dev."},
  {i5Tlv1905NeighborDeviceType, "1905 Neighbor Dev."},
  {i5TlvLinkMetricQueryType, "Link Metric Query"},
  {i5TlvTransmitterLinkMetricType, "Tx Link Metric"},
  {i5TlvReceiverLinkMetricType, "Rx Link Metric"},
  {i5TlvVendorSpecificType, "Vendor Specific"},
  {i5TlvLinkMetricResultCodeType, "Link Metric Result"},
  {i5TlvSearchedRoleType, "Searched Role"},
  {i5TlvAutoconfigFreqBandType, "Autoconfig Freq Band"},
  {i5TlvSupportedRoleType, "Supported Role"},
  {i5TlvSupportedFreqBandType, "Supported Freq Band"},
  {i5TlvWscType, "Wsc"},
  {i5TlvPushButtonEventNotificationType, "Push Button Notify"},
  {i5TlvPushButtonJoinNotificationType, "Push Button Join Notify"},
  {i5TlvGenericPhyDevInfoType, "Generic Phy Dev"},
  {i5TlvDeviceIdentificationType, "Device Identification"},
  {i5TlvControlUrlType, "Control Url"},
  {i5TlvIpv4Type, "Ipv4"},
  {i5TlvIpv6Type, "Ipv6"},
  {i5TlvPushButtonGenericPhyEventNotificationType, "Push Button Generic Phy"},

#ifdef MULTIAP
  {i5TlvSupportedServicesType, "Supported Services"},
  {i5TlvSearchedServicesType, "Searched Services"},
  {i5TlvAPRadioIndentifierType, "AP Radio Indentifier"},
  {i5TlvAPOperationalBSSType, "AP Operational BSS"},
  {i5TlvAssocaitedClientsType, "Assocaited Clients"},
  {i5TlvAPRadioBasicCapabilitiesType, "AP Radio Basic Capabilities"},
  {i5TlvAPHTCapabilitiesType, "AP HT Capabilities"},
  {i5TlvAPVHTCapabilitiesType, "AP VHT Capabilities"},
  {i5TlvAPHECapabilitiesType, "AP HE Capabilities"},
  {i5TlvSteeringPolicyType, "Steering Policy"},
  {i5TlvMetricReportingPolicyType, "Metric Reporting Policy"},
  {i5TlvChannelPreferenceType, "Channel Preference"},
  {i5TlvRadioOperationRestrictionType, "Radio Operation Restriction"},
  {i5TlvTransmitPowerLimitType, "Transmit Power Limit"},
  {i5TlvChannelSelectionResponseType, "Channel Selection Response"},
  {i5TlvOperatingChannelReportType, "Operating Channel Report"},
  {i5TlvClientInfoType, "Client Info"},
  {i5TlvClientCapabilityReportType, "Client Capability Report"},
  {i5TlvClientAssociationEventType, "Client Association Event"},
  {i5TlvAPMetricQueryType, "AP Metric Query"},
  {i5TlvAPMetricsType, "AP Metrics"},
  {i5TlvSTAMACAddressType, "STA MAC Address"},
  {i5TlvAssociatedSTALinkMetricsType, "Associated STA Link Metrics"},
  {i5TlvUnAssociatedSTALinkMetricsQueryType, "UnAssociated STA Link Metrics Query"},
  {i5TlvUnAssociatedSTALinkMetricsResponseType, "UnAssociated STA Link Metrics Response"},
  {i5TlvBeaconMetricsQueryType, "Beacon Metrics Query"},
  {i5TlvBeaconMetricsResponseType, "Beacon Metrics Response"},
  {i5TlvSteeringRequestType, "Steering Request"},
  {i5TlvSteeringBTMReportType, "Steering BTM Report"},
  {i5TlvClientAssociationControlRequestType, "Client Association Control Request"},
  {i5TlvBackhaulSteeringRequestType, "Backhaul Steering Request"},
  {i5TlvBackhaulSteeringResponseType, "Backhaul Steering Response"},
  {i5TlvHigherLayerDataType, "Higher Layer Data"},
  {i5TlvAPCapabilityType, "Ap Capabilities"},
  {i5TlvAssociatedSTATrafficStatsType, "Associated STA Traffic Stats"},
  {i5TlvErrorCodeType, "Error Code"},
#if defined(MULTIAPR2)
  {i5TlvChannelScanReportingPolicyType, "Channel Scan Reporting Policy"},
  {i5TlvChannelScanCapabilitiesType, "Channel Scan Capabilities"},
  {i5TlvChannelScanRequestType, "Channel Scan Request"},
  {i5TlvChannelScanResultType, "Channel Scan Result"},
  {i5TlvTimestampType, "Timestamp"},
  {i5TlvMultiAPProfileType, "MultiAP Profile"},
  {i5TlvProfile2APCapabilityType, "Profile-2 AP Capability"},
  {i5TlvDefault8021QSettingsType, "Default 802.1Q Settings"},
  {i5TlvTrafficSeparationPolicyType, "Traffic Separation Policy"},
  {i5TlvServicePrioritizationRuleType, "Service Prioritization Rule"},
  {i5TlvR2ErrorCodeType, "R2 Error Code"},
  {i5TlvAPRadioAdvancedCapabilitiesType, "AP Radio Advanced Capabilities"},
  {i5TlvAssociationStatusNotificationType, "Association Status Notification"},
  {i5TlvSourceInfoType, "Source Info"},
  {i5TlvTunneledMessgeType, "Tunneled message type"},
  {i5TlvTunneledType, "Tunneled"},
  {i5TlvProfile2SteeringRequestType, "Profile-2 Steering Request"},
  {i5TlvUnsuccessfulAssociationPolicyType, "Unsuccessful Association Policy"},
  {i5TlvMetricCollectionIntervalType, "Metric Collection Interval"},
  {i5TlvRadioMetricsType, "Radio Metrics"},
  {i5TlvAPExtendedMetricsType, "AP Extended Metrics"},
  {i5TlvAssociatedSTAExtendedLinkMetricsType, "Associated STA Extended Link Metrics"},
  {i5TlvStatusCodeType, "Status Code"},
  {i5TlvReasonCodeType, "Reason Code"},
#endif /* MULTIAPR2 */
#endif /* MULTIAP */
  {i5TlvBrcmRoutingTableType, "Brcm Routing Table"},
  {i5TlvBrcmFriendlyNameType, "Brcm Friendly"},
  {i5TlvBrcmFriendlyUrlType, "Brcm Friendly URL"},
  {i5TlvBrcmFriendlyIpv4Type, "Brcm Friendly IPV4"},
  {i5TlvBrcmFriendlyIpv6Type, "Brcm Friendly IPV6"},
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif // endif

#define I5_TLV_STEER_REQUEST_MODE   0x80  /* bit 7. 0: Request is a steering opportunity.
                                           * 1: Request is a steering mandate to trigger
                                           * steering for specific client STA(s)
                                           */
#define I5_TLV_STEER_DISASSOC_IMNT  0x40  /* bit 6. BTM Disassociation Imminent bit */
#define I5_TLV_STEER_BTM_ABRIDGED   0x20  /* bit 5. BTM Abridged bit */

extern void i5GlueDeleteAllVlanInterfaces();

/* For Debug only, not required at check-in time */
/* pretty hex print a contiguous buffer */
void
prhex(const char *msg, const uchar *buf, uint nbytes)
{
	char line[128], *p;
	int len = sizeof(line);
	int nchar;
	uint i;

	if (msg && (msg[0] != '\0'))
		printf("%s:\n", msg);

	p = line;
	for (i = 0; i < nbytes; i++) {
		if (i % 16 == 0) {
			nchar = snprintf(p, len, "  %04x: ", i);	/* line prefix */
			p += nchar;
			len -= nchar;
		}
		if (len > 0) {
			nchar = snprintf(p, len, "%02x ", buf[i]);
			p += nchar;
			len -= nchar;
		}

		if (i % 16 == 15) {
			printf("%s\n", line);		/* flush line */
			p = line;
			len = sizeof(line);
		}
	}

	/* flush last partial line */
	if (p != line)
		printf("%s\n", line);
}

int i5_cpy_host16_to_netbuf(unsigned char *dst, unsigned short src)
{
  dst[0] = (src >> 8) & 0xFF;
  dst[1] = (src >> 0) & 0xFF;
  return 2;
}

int i5_cpy_host32_to_netbuf(unsigned char *dst, unsigned int src)
{
  dst[0] = (src >> 24) & 0xFF;
  dst[1] = (src >> 16) & 0xFF;
  dst[2] = (src >>  8) & 0xFF;
  dst[3] = (src >>  0) & 0xFF;
  return 4;
}

int i5_cpy_netbuf_to_host16(unsigned short *dst, unsigned char *src)
{
  *dst = (src[0] << 8) | src[1];
  return 2;
}

int i5_cpy_netbuf_to_host32(unsigned int *dst, unsigned char *src)
{
  *dst = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
  return 4;
}

int i5TlvIsEndOfMessageType(int tlvType)
{
  return (tlvType == i5TlvEndOfMessageType);
}

char const *i5TlvGetTlvTypeString(int tlvType)
{
  int i = 0;

  for (i = 0; i < ARRAY_SIZE(i5_tlv_list_type_name); i++) {
    if (tlvType == i5_tlv_list_type_name[i].tlvType) {
      return (i5_tlv_list_type_name[i].tlvName);
    }
  }

  return "Unknown Tlv";
}

int i5TlvEndOfMessageTypeInsert(i5_message_type *pmsg)
{
  unsigned char buf[3];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvEndOfMessageType;
  ptlv->length = htons(0);
  len += sizeof(i5_tlv_t);

  return (i5MessageInsertTlv(pmsg, buf, len));
}

int i5TlvEndOfMessageTypeExtract(i5_message_type *pmsg)
{
  unsigned int length;
  unsigned char *pvalue;

  if (i5MessageTlvExtract(pmsg, i5TlvEndOfMessageType, &length, &pvalue, i5MessageTlvExtractWithReset) == 0) {
    if (length == 0) {
      return 0;
    }
  }
  return -1;
}

int i5TlvAlMacAddressTypeInsert(i5_message_type *pmsg)
{
  unsigned char buf[9];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvAlMacAddressType;
  ptlv->length = htons(MAC_ADDR_LEN);
  len += sizeof(i5_tlv_t);
  memcpy(&buf[len], i5_config.i5_mac_address, MAC_ADDR_LEN);
  len+=MAC_ADDR_LEN;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

int i5TlvAlMacAddressTypeExtract(i5_message_type *pmsg, unsigned char *mac_address)
{
  unsigned char *pvalue;
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvAlMacAddressType, &length, &pvalue, i5MessageTlvExtractWithReset) == 0) {
    if (length == MAC_ADDR_LEN) {
      if ( i5DmDeviceIsSelf(pvalue) ) {
        i5TraceInfo("Received packet using local AL MAC\n");
        return -1;
      }
      memcpy(mac_address, pvalue, MAC_ADDR_LEN);
      i5DmRefreshDeviceTimer(mac_address, 0);
      return 0;
    }
  }
  return -1;
}

int i5TlvMacAddressTypeInsert(i5_message_type *pmsg, unsigned char *mac_address)
{
  unsigned char buf[9];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvMacAddressType;
  ptlv->length = htons(MAC_ADDR_LEN);
  len += sizeof(i5_tlv_t);
  memcpy(&buf[len], mac_address, MAC_ADDR_LEN);
  len+=MAC_ADDR_LEN;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

int i5TlvMacAddressTypeExtract(i5_message_type *pmsg, unsigned char *mac_address)
{
  unsigned char *pvalue;
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvMacAddressType, &length, &pvalue, i5MessageTlvExtractWithReset) == 0) {
    if (length == MAC_ADDR_LEN) {
      memcpy(mac_address, pvalue, MAC_ADDR_LEN);
      return 0;
    }
  }
  return -1;
}

int i5TlvDeviceInformationTypeInsert(i5_message_type *pmsg, unsigned char useLegacyHpav, char* containsGenericPhy)
{
  unsigned char *pbuf, *pmem, *tmpmem;
  unsigned char interfaceCount = 0;
  i5_tlv_t *ptlv;
  i5_dm_device_type *pdmdev;
  i5_dm_interface_type *pdmif;
  i5_dm_bss_type *pbss;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  pbuf = pmem + 3; // Header filled at the end
  memcpy(pbuf, i5_config.i5_mac_address, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  if ((pdmdev = i5DmGetSelfDevice()) != NULL) {
    *pbuf = pdmdev->InterfaceNumberOfEntries;
    tmpmem = pbuf; /* Location to store interface count */
    pbuf++;

    pdmif = (i5_dm_interface_type *)pdmdev->interface_list.ll.next;
    while (pdmif != NULL) {
#ifdef MULTIAP_PLUGFEST
      /* Do not add wireless interfaces if the device is controller */
      if (!I5_IS_MULTIAP_AGENT(i5_config.flags) && i5DmIsInterfaceWireless(pdmif->MediaType)) {
        pdmif = pdmif->ll.next;
        continue;
      }
#endif /* MULTIAP_PLUGFEST */
      memcpy(pbuf, pdmif->InterfaceId, MAC_ADDR_LEN);
      pbuf += MAC_ADDR_LEN;
      *((unsigned short *)pbuf) = htons(pdmif->MediaType);
      if (pdmif->MediaType == I5_MEDIA_TYPE_UNKNOWN) {
        *containsGenericPhy = 1;
        if ( (useLegacyHpav) && i5DmIsInterfacePlc(pdmif->MediaType, pdmif->netTechOui) ) {
          // The legacy node has to be told to use the old school HPAV 1
          *((unsigned short *)pbuf) =  htons(I5_MEDIA_TYPE_1901_FFT);
        }
      }
      pbuf+=2;
      *pbuf = pdmif->MediaSpecificInfoSize;
      pbuf++;
      if (pdmif->MediaSpecificInfoSize > 0) {
        memcpy(pbuf, pdmif->MediaSpecificInfo, pdmif->MediaSpecificInfoSize);
        pbuf += pdmif->MediaSpecificInfoSize;
      }
      /* Add the virtual BSS into the device information TLV */
      pbss = (i5_dm_bss_type*)pdmif->bss_list.ll.next;
      while (pbss != NULL) {
        /* If the interface MAC and BSSID is same dont add bcoz its already added */
        if (memcmp(pdmif->InterfaceId, pbss->BSSID, MAC_ADDR_LEN) != 0) {
          memcpy(pbuf, pbss->BSSID, MAC_ADDR_LEN);
          pbuf += MAC_ADDR_LEN;
          *((unsigned short *)pbuf) = htons(pdmif->MediaType);
          pbuf+=2;
          *pbuf = pdmif->MediaSpecificInfoSize;
          pbuf++;
          if (pdmif->MediaSpecificInfoSize > 0) {
            /* In the media specific info change the BSSID and make the role as AP */
            memcpy(pbuf, pdmif->MediaSpecificInfo, pdmif->MediaSpecificInfoSize);
            memcpy(pbuf, pbss->BSSID, MAC_ADDR_LEN);
            *(pbuf+MAC_ADDR_LEN) = I5_MEDIA_INFO_ROLE_AP;
            pbuf += pdmif->MediaSpecificInfoSize;
          }
          interfaceCount++;
        }
        pbss = pbss->ll.next;
      }
      interfaceCount++;
      pdmif = pdmif->ll.next;
    }
    /* Add number of interfaces */
    *tmpmem = interfaceCount;

    ptlv = (i5_tlv_t *)pmem;
    ptlv->type = i5TlvDeviceInformationType;
    ptlv->length = htons(pbuf-pmem-3);
    rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);
  }

  free(pmem);
  return (rc);
}

int i5TlvDeviceInformationTypeExtractAlMac(i5_message_type *pmsg, unsigned char *neighbor_al_mac_address)
{
  unsigned char *pvalue;
  unsigned int length;
  int retval = -1;

  i5MessageReset(pmsg);
  while (i5MessageTlvExtract(pmsg, i5TlvDeviceInformationType, &length, &pvalue, i5MessageTlvExtractWithoutReset) == 0) {
    if (length >= MAC_ADDR_LEN) {
      memcpy(neighbor_al_mac_address, pvalue, MAC_ADDR_LEN);
      retval = 0;
    }
  }

  return retval;
}

int i5TlvDeviceInformationTypeExtract(i5_message_type *pmsg, unsigned char *neighbor_al_mac_address, unsigned char *deviceHasGenericPhy)
{
  unsigned char *pvalue;
  unsigned int length, pos, n;
  unsigned char interface_id[MAC_ADDR_LEN];
  unsigned short if_media_type;
  unsigned char if_media_specific_info[I5_MEDIA_SPECIFIC_INFO_MAX_SIZE];
  unsigned char *pif_media_specific_info = NULL;
  unsigned int if_media_specific_info_size = 0;
  int interfacesCleared = 0;
  int retval = -1;

  *deviceHasGenericPhy = 0;
  i5MessageReset(pmsg);
  while (i5MessageTlvExtract(pmsg, i5TlvDeviceInformationType, &length, &pvalue, i5MessageTlvExtractWithoutReset) == 0) {
    if (length >= MAC_ADDR_LEN) {
      if ( memcmp(neighbor_al_mac_address, pvalue, MAC_ADDR_LEN) != 0 ) {
        retval = -1;
        break;
      }
      pos = MAC_ADDR_LEN;
      pos++;

      if (0 == interfacesCleared) {
        i5DmInterfacePending(neighbor_al_mac_address);
        interfacesCleared = 1;
      }
      while (length >= pos + MAC_ADDR_LEN + 3) {
        memcpy(interface_id, &pvalue[pos], MAC_ADDR_LEN);
        pos += MAC_ADDR_LEN;
        if_media_type = ntohs(*((unsigned short *)&pvalue[pos]));
        pos+=2;
        n = pvalue[pos];
        pos++;
        if (if_media_type == I5_MEDIA_TYPE_UNKNOWN) {
          *deviceHasGenericPhy = 1;
        }
        if ((i5DmIsInterfaceWireless(if_media_type) && (n == i5TlvMediaSpecificInfoWiFi_Length)) ||
            ((if_media_type >= 0x20) && (if_media_type <= 0x21) && (n == i5TlvMediaSpecificInfo1901_Length))) {
          if (length >= pos + n) {
            memcpy(if_media_specific_info, &pvalue[pos], n);
            pif_media_specific_info = if_media_specific_info;
            if_media_specific_info_size = n;
          }
        }
        else {
          pif_media_specific_info = NULL;
          if_media_specific_info_size = 0;
        }
        pos += n;
        i5DmInterfaceUpdate(neighbor_al_mac_address, interface_id, i5MessageVersionGet(pmsg), if_media_type,
                            pif_media_specific_info, if_media_specific_info_size, NULL, NULL, 0);
      }
      retval = 0;
    }
  }

  if (0 == retval) {
    i5DmInterfaceDone(neighbor_al_mac_address);
  }

  return retval;
}

int i5TlvGenericPhyTypeInsert (i5_message_type *pmsg)
{
  i5_dm_device_type *selfDevice = i5DmGetSelfDevice();
  i5_dm_interface_type *currIf = (i5_dm_interface_type *)(selfDevice->interface_list.ll.next);

  unsigned char buf[1024]; // This needs to be calculated somehow
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  unsigned int index = 0;
  unsigned int remaining, fullPacket;
  unsigned char *numPhyEntries = NULL;
  unsigned int phyIfIndex = 0;

  i5MessageGetPacketSpace(pmsg, &remaining, &fullPacket);
  ptlv->type = i5TlvGenericPhyDevInfoType;
  ptlv->length = 0; /* this is a variable length packet, add as we go */
  index += sizeof(i5_tlv_t);
  remaining -= sizeof(i5_tlv_t);

  memcpy(&buf[index], i5_config.i5_mac_address, MAC_ADDR_LEN);
  index += MAC_ADDR_LEN;

  buf[index] = 0;
  numPhyEntries = &buf[index];
  index += 1;

  for ( ; phyIfIndex < selfDevice->InterfaceNumberOfEntries; phyIfIndex ++ ) {
    if (currIf->MediaType == I5_MEDIA_TYPE_UNKNOWN) {
      (*numPhyEntries) ++;

      // Local Interface MAC
      memcpy(&buf[index], currIf->InterfaceId, MAC_ADDR_LEN);
      index += MAC_ADDR_LEN;

      // OUI
      memcpy(&buf[index], currIf->netTechOui, I5_PHY_INTERFACE_NETTECHOUI_SIZE);
      index += I5_PHY_INTERFACE_NETTECHOUI_SIZE;

      // variant index
      buf[index] = currIf->netTechVariant;
      index ++;

      // UTF-8 string [32]
      memcpy(&buf[index], &currIf->netTechName, I5_PHY_INTERFACE_NETTECHNAME_SIZE);
      index += I5_PHY_INTERFACE_NETTECHNAME_SIZE;

      // u (sizeof URL)
      buf[index] = strlen((char *)currIf->url);
      index ++;

      // s (sizeof media spec info field)
      buf[index] = currIf->MediaSpecificInfoSize;
      index ++;

      // URL (copy without NULL, since we have a length recorded above)
      memcpy(&buf[index], &currIf->url, strlen((char *)currIf->url));
      index += strlen((char *)currIf->url);

      // Media spec info field
      memcpy(&buf[index], &currIf->MediaSpecificInfo, currIf->MediaSpecificInfoSize);
      index += currIf->MediaSpecificInfoSize;
    }
    currIf = (i5_dm_interface_type *)(currIf->ll.next);
  }

  i5TraceInfo("total length = %d\n", index);
  ptlv->length = htons(index-3);
  return i5MessageInsertTlv(pmsg, buf, index);
}

int i5TlvGenericPhyTypeExtract (i5_message_type *pmsg)
{
  unsigned char *pvalue;
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvGenericPhyDevInfoType, &length, &pvalue, i5MessageTlvExtractWithReset) == 0) {
    unsigned char mac_address[MAC_ADDR_LEN];
    int numPhyInterfaces = 0;
    int phyIfIndex = 0;
    int remaining = length;
    i5_dm_device_type *reportingDevice = NULL;

    if (length < MAC_ADDR_LEN + 1) {
      i5TraceInfo("Minimum size 7 bytes not met (%d bytes rx'd)\n", length);
      return -1;
    }

    memcpy(mac_address, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;
    remaining -= MAC_ADDR_LEN;

    reportingDevice = i5DmDeviceFind(mac_address);
    if (!reportingDevice) {
      i5TraceInfo("Reporting device " I5_MAC_DELIM_FMT " not found\n", I5_MAC_PRM(mac_address));
      return -1;
    }

    numPhyInterfaces = (int)*pvalue;
    pvalue ++;
    remaining --;
    i5TraceInfo("Gen Phy TLV contains %d PHY interfaces\n", numPhyInterfaces);
    for ( ; phyIfIndex < numPhyInterfaces ; phyIfIndex++) {
      unsigned char ifAddress[MAC_ADDR_LEN];
      unsigned char netTechOui[I5_PHY_INTERFACE_NETTECHOUI_SIZE];
      unsigned char netTechVariant;
      unsigned char netTechName[I5_PHY_INTERFACE_NETTECHNAME_SIZE];
      unsigned char u, s;
      unsigned char url[I5_PHY_INTERFACE_URL_MAX_SIZE];
      unsigned char mediaSpecInfo[I5_MEDIA_SPECIFIC_INFO_MAX_SIZE];

      // Check for min length (6+3+1+32+1+1+0+0) assuming url and media spec info could be zero length
      if (remaining < 44) {
        i5TraceInfo("Minimum size of GenPhy record = 44 bytes not met (%d bytes remain)\n", remaining);
        return -1;
      }

      // Fetch interface ADDR
      memcpy(ifAddress, pvalue, MAC_ADDR_LEN);
      pvalue += MAC_ADDR_LEN;
      remaining -= MAC_ADDR_LEN;

      // Fetch OUI
      memcpy(netTechOui, pvalue, I5_PHY_INTERFACE_NETTECHOUI_SIZE);
      pvalue += I5_PHY_INTERFACE_NETTECHOUI_SIZE;
      remaining -= I5_PHY_INTERFACE_NETTECHOUI_SIZE;

      // Fetch Variant
      netTechVariant = *pvalue;
      pvalue ++;
      remaining --;

      // Fetch Variant name
      memcpy(netTechName, pvalue, I5_PHY_INTERFACE_NETTECHNAME_SIZE);
      pvalue += I5_PHY_INTERFACE_NETTECHNAME_SIZE;
      remaining -= I5_PHY_INTERFACE_NETTECHNAME_SIZE;

      // Fetch u, s
      u = *pvalue;
      pvalue ++;
      remaining --;
      s = *pvalue;
      pvalue ++;
      remaining --;

      if (remaining < u + s) {
        i5TraceInfo("Url and media spec info want %d bytes but only %d bytes remain\n", u+s, remaining);
        return -1;
      }

      // Fetch url
      memcpy(url, pvalue, u < I5_PHY_INTERFACE_URL_MAX_SIZE ? u : I5_PHY_INTERFACE_URL_MAX_SIZE);
      pvalue += u;
      remaining -= u;
      url[(u < I5_PHY_INTERFACE_URL_MAX_SIZE-1) ? u : I5_PHY_INTERFACE_URL_MAX_SIZE-1] = '\0';

      // Fetch media spec info
      memcpy(mediaSpecInfo, pvalue, s < I5_MEDIA_SPECIFIC_INFO_MAX_SIZE ? s : I5_MEDIA_SPECIFIC_INFO_MAX_SIZE);
      pvalue += s;
      remaining -= s;
      mediaSpecInfo[(s < I5_MEDIA_SPECIFIC_INFO_MAX_SIZE-1) ? s : I5_MEDIA_SPECIFIC_INFO_MAX_SIZE-1] = '\0';
      i5DmInterfacePhyUpdate(reportingDevice->DeviceId, ifAddress, netTechOui, &netTechVariant,
                             netTechName, url);
    }
  }
  return 0;
}

int i5TlvDeviceBridgingCapabilityTypeInsert(i5_message_type *pmsg)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  i5_dm_device_type *pdmdev;
  i5_dm_bridging_tuple_info_type *pdmbrtuple;
  int rc = 0;

  pdmdev = i5DmGetSelfDevice();
  if ( NULL == pdmdev ) {
    return -1;
  }

  if ( 0 == pdmdev->BridgingTuplesNumberOfEntries ) {
    return 0;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceError("Out of memory error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t);
  *pbuf = pdmdev->BridgingTuplesNumberOfEntries;
  pbuf++;

  pdmbrtuple = (i5_dm_bridging_tuple_info_type *)pdmdev->bridging_tuple_list.ll.next;
  while (pdmbrtuple != NULL) {
    int len = pdmbrtuple->forwardingInterfaceListNumEntries * MAC_ADDR_LEN;
    *pbuf = pdmbrtuple->forwardingInterfaceListNumEntries;
    pbuf++;
    memcpy(pbuf, &pdmbrtuple->ForwardingInterfaceList[0], len);
    pbuf += len;
    pdmbrtuple = pdmbrtuple->ll.next;
  }

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvDeviceBridgingCapabilityType;
  ptlv->length = htons(pbuf-pmem-3);
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

int i5TlvDeviceBridgingCapabilityTypeExtract(i5_message_type *pmsg, unsigned char *pdevid)
{
  unsigned char *pvalue;
  unsigned int length = 0;

  int rc = 0;

  i5MessageReset(pmsg);
  i5DmBridgingTuplePending(pdevid);
  while ((rc = i5MessageTlvExtract(pmsg, i5TlvDeviceBridgingCapabilityType, &length, &pvalue, i5MessageTlvExtractWithoutReset)) == 0) {
    if (length > 0) {
      unsigned char  i;
      unsigned int pos = 0;
      unsigned char device_num_tuples;

      device_num_tuples = pvalue[pos];
      pos++;

      for (i = 0; i < device_num_tuples; i++) {
        i5Trace("Index = %d\n", i);
        if (length >= pos + 1) {
          unsigned char tuple_num_macaddrs = pvalue[pos];
          pos++;

          if (length >= pos + tuple_num_macaddrs*MAC_ADDR_LEN) {
              i5DmBridgingTupleUpdate(pdevid, i5MessageVersionGet(pmsg), NULL, tuple_num_macaddrs, &pvalue[pos]);
              pos += tuple_num_macaddrs*MAC_ADDR_LEN;
          }
        }
      }
    }
    else {
      rc = -1;
      break;
    }
  }
  i5DmBridgingTupleDone(pdevid);

  /* This is an optional TLV */
  if (rc == -2) {
    rc = 0;
  }

  return rc;
}

int i5TlvLegacyNeighborDeviceTypeInsert(i5_message_type *pmsg)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  i5_dm_device_type *pdmdev;
  i5_dm_legacy_neighbor_type *pdmnbor;
  i5_dm_interface_type *pdmif;
  int rc = 0;
  unsigned int remaining, fullPacket;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  i5MessageGetPacketSpace(pmsg, &remaining, &fullPacket);

  i5TraceInfo ("remain = %d  fullPacket = %d\n", remaining, fullPacket);

  if (remaining < sizeof(i5_tlv_t) + MAC_ADDR_LEN + MAC_ADDR_LEN) {
    /* Can't fit even one interface MAC ADDR + 1 legacy neighbor MAC ADDR */
    remaining = fullPacket;
    i5TraceInfo("Set remain = %d full packet size\n", remaining);
  }

  if ((pdmdev = i5DmGetSelfDevice()) != NULL) {
    pdmif = (i5_dm_interface_type *)pdmdev->interface_list.ll.next;
    while ((pdmif != NULL) && (rc == 0)) {

      pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end
      remaining -= sizeof(i5_tlv_t);
      memcpy(pbuf, pdmif->InterfaceId, MAC_ADDR_LEN);

      pbuf += MAC_ADDR_LEN;
      pdmnbor = (i5_dm_legacy_neighbor_type *)pdmdev->legacy_list.ll.next;
      remaining -= MAC_ADDR_LEN;

      while (pdmnbor != NULL) {
        if (memcmp(pdmif->InterfaceId, pdmnbor->LocalInterfaceId, MAC_ADDR_LEN) == 0) {
          memcpy(pbuf, pdmnbor->NeighborInterfaceId, MAC_ADDR_LEN);
          pbuf += MAC_ADDR_LEN;
          remaining -= MAC_ADDR_LEN;

          if (remaining < MAC_ADDR_LEN) {
            i5TraceInfo ("Out of room\n");
            ptlv = (i5_tlv_t *)pmem;
            ptlv->type = i5TlvLegacyNeighborDeviceType;
            ptlv->length = htons(pbuf-pmem-3);
            rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

            i5TraceInfo ("Resetting for full packet size \n");
            remaining = fullPacket - sizeof(i5_tlv_t) - MAC_ADDR_LEN;
            pbuf = pmem + sizeof(i5_tlv_t) + MAC_ADDR_LEN;
          }
        }
        pdmnbor = pdmnbor->ll.next;
      }

      if (pbuf - pmem > MAC_ADDR_LEN + 3) {
        /* This interface has neighbors */
        ptlv = (i5_tlv_t *)pmem;
        ptlv->type = i5TlvLegacyNeighborDeviceType;
        ptlv->length = htons(pbuf-pmem-3);
        rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);
      }
      else {
        /* This interface had no legacy neighbors, so rewind the "remaining" count */
        remaining += 3 + MAC_ADDR_LEN;
      }
      pdmif = pdmif->ll.next;
    }
  }

  free(pmem);
  return (rc);
}

int i5TlvLegacyNeighborDeviceTypeExtract(i5_message_type *pmsg, unsigned char *pdevid)
{
  unsigned char *pvalue;
  unsigned int length, pos;
  unsigned char local_interface[MAC_ADDR_LEN];
  unsigned char neighbor_interface[MAC_ADDR_LEN];
  int rc = 0;

  i5Trace("\n");
  i5MessageReset(pmsg);
  i5DmLegacyNeighborPending(pdevid);
  while ((rc = i5MessageTlvExtract(pmsg, i5TlvLegacyNeighborDeviceType, &length, &pvalue, i5MessageTlvExtractWithoutReset)) == 0) {
    pos = 0;
    if (length >= MAC_ADDR_LEN) {
      memcpy(local_interface, &pvalue[pos], MAC_ADDR_LEN);
      pos += MAC_ADDR_LEN;
      while (length >= pos + MAC_ADDR_LEN) {
        memcpy(neighbor_interface, &pvalue[pos], MAC_ADDR_LEN);
        pos += MAC_ADDR_LEN;
        i5DmLegacyNeighborUpdate(pdevid, local_interface, neighbor_interface);
      }
    }
  }
  i5DmLegacyNeighborDone(pdevid);

  /* This is an optional TLV */
  return 0;
}

int i5Tlv1905NeighborDeviceTypeInsert(i5_message_type *pmsg)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  i5_dm_device_type *pdmdev;
  i5_dm_1905_neighbor_type *pdmnbor;
  i5_dm_interface_type *pdmif;
  i5_dm_bss_type *pbss = NULL;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  if ((pdmdev = i5DmGetSelfDevice()) == NULL) {
    goto end;
  }

  pdmnbor = (i5_dm_1905_neighbor_type *)pdmdev->neighbor1905_list.ll.next;
  while (pdmnbor != NULL) {
    /* Find the interface in the device which is matching neighbor local interface ID */
    if ((pdmif = i5DmInterfaceFind(pdmdev, pdmnbor->LocalInterfaceId)) == NULL) {
      /* If the interface not found find the BSS for virtual backhaul case */
      pbss = i5DmFindBSSFromDevice(pdmdev, pdmnbor->LocalInterfaceId);
    }
    /* Check if the interface found */
    if (pdmif != NULL || pbss != NULL) {
      pbuf = pmem + 3; // Header filled at the end
      if (pdmif) {
        memcpy(pbuf, pdmif->InterfaceId, MAC_ADDR_LEN);
      } else if (pbss) {
        memcpy(pbuf, pbss->BSSID, MAC_ADDR_LEN);
      } else {
        goto next;
      }
      pbuf += MAC_ADDR_LEN;
      memcpy(pbuf, pdmnbor->Ieee1905Id, MAC_ADDR_LEN);
      pbuf += MAC_ADDR_LEN;
      *pbuf = pdmnbor->IntermediateLegacyBridge;
      pbuf++;
      ptlv = (i5_tlv_t *)pmem;
      ptlv->type = i5Tlv1905NeighborDeviceType;
      ptlv->length = htons(pbuf-pmem-3);
      rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);
    }
next:
    pdmnbor = pdmnbor->ll.next;
  }

end:
  free(pmem);
  return (rc);
}

int i5Tlv1905NeighborDeviceTypeExtract(i5_message_type *pmsg, unsigned char *pdevid)
{
  unsigned char *pvalue;
  unsigned int length, pos;
  unsigned char local_interface[MAC_ADDR_LEN];
  unsigned char neighbor_al_mac_address[MAC_ADDR_LEN];
  unsigned char intermediate_legacy_bridge;
  int rc = 0;

  i5Trace("\n");
  i5Dm1905NeighborPending(pdevid);
  i5MessageReset(pmsg);
  while (i5MessageTlvExtract(pmsg, i5Tlv1905NeighborDeviceType, &length, &pvalue, i5MessageTlvExtractWithoutReset) == 0) {
    if (length == 0)
        continue;
    if ((length >= MAC_ADDR_LEN) && ((length - MAC_ADDR_LEN) % (MAC_ADDR_LEN + 1) == 0)) {
      pos = 0;
      memcpy(local_interface, &pvalue[pos], MAC_ADDR_LEN);
      pos += MAC_ADDR_LEN;

      while (length >= pos + MAC_ADDR_LEN + 1) {
        memcpy(neighbor_al_mac_address, &pvalue[pos], MAC_ADDR_LEN);
        pos += MAC_ADDR_LEN;
        intermediate_legacy_bridge = pvalue[pos] & 0x01;
        pos++;

        /* if pdevid has a neigbour pointing back to us then we can use the
           rx interface to fill in the neighbour id for pdevid's neighbour entry */
        i5Dm1905NeighborUpdate(pdevid, local_interface, neighbor_al_mac_address, NULL, &intermediate_legacy_bridge,
                               i5SocketGetIfName(pmsg->psock), i5SocketGetIfIndex(pmsg->psock), 1);
        i5DmDeviceNewIfNew(neighbor_al_mac_address);
      }
    } else {
      i5TraceError("Bad TLV length: i5Tlv1905NeighborDeviceType: Length: %d\n", length);
      rc = -1;
      break;
    }
  }
  i5Dm1905NeighborDone(pdevid);
  /* Do not remove the unreachable neighbors immediately */
  i5DmTopologyFreeUnreachableDevices(TRUE);

  return rc;
}

int i5TlvSearchedRoleTypeInsert(i5_message_type *pmsg)
{
    unsigned char buf[sizeof(i5_tlv_t) + i5TlvSearchRole_Length];
    i5_tlv_t *ptlv = (i5_tlv_t *)buf;
    int len = 0;

    ptlv->type = i5TlvSearchedRoleType;
    ptlv->length = htons(i5TlvSearchRole_Length);
    len += sizeof(i5_tlv_t);
    buf[len] = (unsigned char)i5TlvRole_Registrar;
    len += i5TlvSearchRole_Length;

    return (i5MessageInsertTlv(pmsg, buf, len));
}

int i5TlvSearchedRoleTypeExtract(i5_message_type *pmsg, unsigned char *searchRole)
{
    unsigned char *pvalue;
    unsigned int length;

    if (i5MessageTlvExtract(pmsg, i5TlvSearchedRoleType, &length, &pvalue, i5MessageTlvExtractWithReset) == 0) {
      if (length == i5TlvSearchRole_Length) {
        memcpy(searchRole, pvalue, i5TlvSearchRole_Length);
        return 0;
      }
    }
    return -1;
}

unsigned int i5TlvGetFreqBandFromMediaType(unsigned short mediaType)
{
    switch (mediaType) {
      case I5_MEDIA_TYPE_WIFI_B:
      case I5_MEDIA_TYPE_WIFI_G:
      case I5_MEDIA_TYPE_WIFI_N24:
        return i5MessageFreqBand_802_11_2_4Ghz;
      case I5_MEDIA_TYPE_WIFI_A:
      case I5_MEDIA_TYPE_WIFI_N5:
      case I5_MEDIA_TYPE_WIFI_AC:
        return i5MessageFreqBand_802_11_5Ghz;
      case I5_MEDIA_TYPE_WIFI_AD:
        return i5MessageFreqBand_802_11_60Ghz;
    }
    return i5MessageFreqBand_Reserved;
}

int i5TlvAutoconfigFreqBandTypeInsert(i5_message_type *pmsg, unsigned int freqBand)
{
    unsigned char buf[sizeof(i5_tlv_t) + i5TlvAutoConfigFreqBand_Length];
    i5_tlv_t *ptlv = (i5_tlv_t *)buf;
    int len = 0;

    ptlv->type = i5TlvAutoconfigFreqBandType;
    ptlv->length = htons(i5TlvAutoConfigFreqBand_Length);
    len += sizeof(i5_tlv_t);

    buf[len] = (unsigned char)freqBand;
    len += i5TlvAutoConfigFreqBand_Length;

    return (i5MessageInsertTlv(pmsg, buf, len));
}

int i5TlvAutoconfigFreqBandTypeExtract(i5_message_type *pmsg, unsigned char *autoconfigFreqBand)
{
    unsigned char *pvalue;
    unsigned int length;

    if (i5MessageTlvExtract(pmsg, i5TlvAutoconfigFreqBandType, &length, &pvalue, i5MessageTlvExtractWithReset) == 0) {
      if (length == i5TlvAutoConfigFreqBand_Length) {
        memcpy(autoconfigFreqBand, pvalue, i5TlvAutoConfigFreqBand_Length);
        return 0;
      }
    }
    return -1;
}

int i5TlvSupportedRoleTypeInsert(i5_message_type *pmsg)
{
    unsigned char buf[sizeof(i5_tlv_t) + i5TlvSupportedRole_Length];
    i5_tlv_t *ptlv = (i5_tlv_t *)buf;
    int len = 0;

    ptlv->type = i5TlvSupportedRoleType;
    ptlv->length = htons(i5TlvSupportedRole_Length);
    len += sizeof(i5_tlv_t);
    buf[len] = (unsigned char)i5TlvRole_Registrar;
    len += i5TlvSupportedRole_Length;

    return (i5MessageInsertTlv(pmsg, buf, len));
}

int i5TlvSupportedRoleTypeExtract(i5_message_type *pmsg, unsigned char *supportedRole)
{
    unsigned char *pvalue;
    unsigned int length;

    if (i5MessageTlvExtract(pmsg, i5TlvSupportedRoleType, &length, &pvalue, i5MessageTlvExtractWithReset) == 0) {
      if (length == i5TlvSupportedRole_Length) {
        memcpy(supportedRole, pvalue, i5TlvSupportedRole_Length);
        return 0;
      }
    }
    return -1;
}

int i5TlvSupportedFreqBandTypeInsert(i5_message_type *pmsg, unsigned int freqBand)
{
    unsigned char buf[sizeof(i5_tlv_t) + i5TlvSupportedFreqBand_Length];
    i5_tlv_t *ptlv = (i5_tlv_t *)buf;
    int len = 0;

    ptlv->type = i5TlvSupportedFreqBandType;
    ptlv->length = htons(i5TlvSupportedFreqBand_Length);
    len += sizeof(i5_tlv_t);

    buf[len] = (unsigned char)freqBand;
    len += i5TlvSupportedFreqBand_Length;

    return (i5MessageInsertTlv(pmsg, buf, len));
}

int i5TlvSupportedFreqBandTypeExtract(i5_message_type *pmsg, unsigned char *supportedFreqBand)
{
    unsigned char *pvalue;
    unsigned int length;

    if (i5MessageTlvExtract(pmsg, i5TlvSupportedFreqBandType, &length, &pvalue, i5MessageTlvExtractWithReset) == 0) {
      if (length == i5TlvSupportedFreqBand_Length) {
        memcpy(supportedFreqBand, pvalue, i5TlvSupportedFreqBand_Length);
        return 0;
      }
    }
    return -1;
}

int i5TlvWscTypeInsert(i5_message_type *pmsg, unsigned char const * wscPacket, unsigned wscLength)
{
    int totalLength = sizeof(i5_tlv_t) + wscLength;
    unsigned char buf[totalLength];
    i5_tlv_t *ptlv = (i5_tlv_t *)buf;

    ptlv->type = i5TlvWscType;
    ptlv->length = htons(wscLength);
    memcpy(&buf[sizeof(i5_tlv_t)], wscPacket, wscLength);

    return (i5MessageInsertTlv(pmsg, buf, totalLength));
}

int i5TlvWscTypeExtract(i5_message_type *pmsg, unsigned char * wscPacket, unsigned maxWscLength, unsigned *pactualWscLength)
{
    unsigned char *pvalue;

    if (i5MessageTlvExtract(pmsg, i5TlvWscType, pactualWscLength, &pvalue, i5MessageTlvExtractWithReset) == 0) {
      if (*pactualWscLength < maxWscLength) {
        memcpy(wscPacket, pvalue, *pactualWscLength);
        return 0;
      }
    }
    return -1;
}

/* Extract WSC M2 message and Put all M2 in a list */
int i5TlvWscTypeM2Extract(i5_message_type *pmsg)
{
  unsigned char *pvalue;
  unsigned char found = 0;
  unsigned int length;

  i5MessageReset(pmsg);

  while (i5MessageTlvExtract(pmsg, i5TlvWscType, &length, &pvalue,
    i5MessageTlvExtractWithoutReset) == 0) {
    unsigned char *m2 = NULL;

    if (length <= 0) {
      return -1;
    }
    found = 1;
    if ((m2 = malloc(length)) == NULL) {
      i5TraceDirPrint("Malloc Failed\n");
      return -1;
    }

    memcpy(m2, pvalue, length);
    pvalue += length;
    i5DmM2New(m2, length);
  }

  if (!found) {
    return -1;
  }

  return 0;
}

int i5TlvPushButtonEventNotificationTypeInsert(i5_message_type *pmsg, unsigned char* genericPhyIncluded)
{
  unsigned char *pBuf;
  i5_tlv_t *ptlv;
  unsigned int index = 0;
  i5_dm_device_type *pdmdev;
  i5_dm_interface_type *pdmif;
  unsigned int interfaceCount = 0;
  unsigned int bufLength = sizeof(i5_tlv_t) + i5TlvPushButtonNotificationMediaCount_Length;
  int rc;

  pdmdev = i5DmGetSelfDevice();
  if ( NULL == pdmdev ) {
    return -1;
  }

  pdmif = (i5_dm_interface_type *)pdmdev->interface_list.ll.next;
  while (pdmif != NULL) {
    i5Trace("%p, len %d, sec %d\n", pdmif, pdmif->MediaSpecificInfoSize, pdmif->SecurityStatus);
    if ((1 == pdmif->SecurityStatus) &&
        ((I5_MEDIA_TYPE_UNKNOWN != pdmif->MediaType) ||
         ((i5DmIsInterfacePlc(pdmif->MediaType, pdmif->netTechOui)) && (i5DmAreThereNodesWithVersion(I5_DM_NODE_VERSION_1905) ) )
        )
       ) {
      bufLength += pdmif->MediaSpecificInfoSize + 3;
      interfaceCount++;
    }
    pdmif = pdmif->ll.next;
  }

  pBuf = (unsigned char *)malloc(bufLength);
  ptlv = (i5_tlv_t *)pBuf;
  ptlv->type   = i5TlvPushButtonEventNotificationType;
  ptlv->length = htons(bufLength - sizeof(i5_tlv_t));
  index+=sizeof(i5_tlv_t);
  i5Trace("Insert PB - count is %d, message Len = %d, index %d\n", interfaceCount, bufLength, index);
  pBuf[index] = interfaceCount;
  index++;

  pdmif = (i5_dm_interface_type *)pdmdev->interface_list.ll.next;
  while (pdmif != NULL) {
    if (1 == pdmif->SecurityStatus) {
      i5Trace("Mediatype 0x%04x Media Info Size %d\n", pdmif->MediaType, pdmif->MediaSpecificInfoSize);
      if (I5_MEDIA_TYPE_UNKNOWN == pdmif->MediaType) {
        *genericPhyIncluded = 1;
        if ((i5DmIsInterfacePlc(pdmif->MediaType, pdmif->netTechOui)) && (i5DmAreThereNodesWithVersion(I5_DM_NODE_VERSION_1905) )) {
          *((unsigned short *)&pBuf[index]) = I5_MEDIA_TYPE_1901_FFT;
          index+=2;
          pBuf[index] = (unsigned char)pdmif->MediaSpecificInfoSize;
          index++;
          memcpy(&pBuf[index], pdmif->MediaSpecificInfo, pdmif->MediaSpecificInfoSize);
          index+=pdmif->MediaSpecificInfoSize;
        }
      }
      else {
        *((unsigned short *)&pBuf[index]) = htons(pdmif->MediaType);
        index+=2;
        pBuf[index] = (unsigned char)pdmif->MediaSpecificInfoSize;
        index++;
        memcpy(&pBuf[index], pdmif->MediaSpecificInfo, pdmif->MediaSpecificInfoSize);
        index+=pdmif->MediaSpecificInfoSize;
      }
    }
    pdmif = pdmif->ll.next;
  }

  rc = i5MessageInsertTlv(pmsg, pBuf, bufLength);
  free(pBuf);

  return rc;
}

int i5TlvPushButtonEventNotificationTypeExtract(i5_message_type * pmsg, unsigned int *pMediaCount, unsigned short **pMediaList)
{
  int             rc;
  unsigned char  *pValue;
  unsigned int    length;
  unsigned int    i;
  unsigned int    mediaCount = 0;
  unsigned short *pBuf = NULL;

  rc = i5MessageTlvExtract(pmsg, i5TlvPushButtonEventNotificationType, &length, &pValue, i5MessageTlvExtractWithReset);
  if (rc == 0) {
    mediaCount = *pValue;
    pValue += 1;

    i5Trace("MediaCount is %u\n", mediaCount);
    pBuf = (unsigned short *)malloc(mediaCount * 2);
    for(i=0; i<mediaCount; i++) {
      pBuf[i] = ntohs(*((unsigned short *)pValue));
      i5Trace("MediaType is %x\n", pBuf[i]);
      pValue += 2;
      pValue += (*pValue) + 1;
    }
  }
  *pMediaCount = mediaCount;
  *pMediaList = pBuf;

  return 0;
}

int i5TlvPushButtonGenericPhyEventNotificationTypeExtract (i5_message_type * pmsg, unsigned int *pMediaCount, unsigned char **pMediaList)
{
  int             rc;
  unsigned char  *pValue;
  unsigned int    length;
  unsigned int    i;
  unsigned int    mediaCount = 0;
  unsigned char  *pBuf = NULL;
  unsigned char   mediaInfoSize = 0;

  rc = i5MessageTlvExtract(pmsg, i5TlvPushButtonGenericPhyEventNotificationType, &length, &pValue, i5MessageTlvExtractWithReset);
  if (rc == 0) {
    mediaCount = *pValue;
    pValue += 1;

    i5Trace("MediaCount is %u\n", mediaCount);
    pBuf = (unsigned char *)malloc(mediaCount * 4);
    for(i=0; i<mediaCount; i++) {
      memcpy (&pBuf[4*i], pValue, 4);
      i5Trace("OUI is %x:%x:%x Variant %x\n", pBuf[4*i],pBuf[4*i+1],pBuf[4*i+2],pBuf[4*i+3] );
      pValue += 4;
      mediaInfoSize = *pValue;
      pValue += 1;
      // Ignore "Media Specific Info"
      pValue += mediaInfoSize;
    }
  }
  *pMediaCount = mediaCount;
  *pMediaList = pBuf;

  return 0;
}

int i5TlvPushButtonEventNotificationTypeExtractFree(unsigned short *pMediaList)
{
  if ( pMediaList ) {
    free(pMediaList);
  }
  return 0;
}

int i5TlvPushButtonGenericPhyEventNotificationTypeExtractFree(unsigned char *pPhyMediaList)
{
  if ( pPhyMediaList ) {
    free(pPhyMediaList);
  }
  return 0;
}

int i5TlvPushButtonGenericPhyEventNotificationTypeInsert(i5_message_type *pmsg)
{
  unsigned char *pBuf;
  i5_tlv_t *ptlv;
  unsigned int index = 0;
  i5_dm_device_type *pdmdev;
  i5_dm_interface_type *pdmif;
  unsigned int interfaceCount = 0;
  unsigned int bufLength = sizeof(i5_tlv_t) + i5TlvPushButtonNotificationMediaCount_Length;
  int rc;

  pdmdev = i5DmGetSelfDevice();
  if ( NULL == pdmdev ) {
    return -1;
  }

  pdmif = (i5_dm_interface_type *)pdmdev->interface_list.ll.next;
  while (pdmif != NULL) {
    i5Trace("%p, len %d, sec %d\n", pdmif, pdmif->MediaSpecificInfoSize, pdmif->SecurityStatus);
    if ((1 == pdmif->SecurityStatus) && (i5DmIsInterfacePlc(pdmif->MediaType, pdmif->netTechOui) )) {
      bufLength += pdmif->MediaSpecificInfoSize + 5;
      interfaceCount++;
    }
    pdmif = pdmif->ll.next;
  }

  pBuf = (unsigned char *)malloc(bufLength);
  ptlv = (i5_tlv_t *)pBuf;
  ptlv->type   = i5TlvPushButtonGenericPhyEventNotificationType;
  ptlv->length = htons(bufLength - sizeof(i5_tlv_t));
  index += sizeof(i5_tlv_t);
  i5Trace("Insert Gen PHY PB - count is %d, message Len = %d, index %d\n", interfaceCount, bufLength, index);
  pBuf[index] = interfaceCount;
  index++;

  pdmif = (i5_dm_interface_type *)pdmdev->interface_list.ll.next;
  while (pdmif != NULL) {
    if (1 == pdmif->SecurityStatus) {
      if (I5_MEDIA_TYPE_UNKNOWN == pdmif->MediaType) {
        memcpy( &pBuf[index], pdmif->netTechOui, 3);
        index+=3;
        pBuf[index] = (unsigned char)pdmif->netTechVariant;
        index++;
        pBuf[index] = (unsigned char)pdmif->MediaSpecificInfoSize;
        index++;
        memcpy(&pBuf[index], pdmif->MediaSpecificInfo, pdmif->MediaSpecificInfoSize);
        index+=pdmif->MediaSpecificInfoSize;
      }
    }
    pdmif = pdmif->ll.next;
  }

  rc = i5MessageInsertTlv(pmsg, pBuf, bufLength);
  free(pBuf);

  return rc;
}

static int i5Tlv_brcm_getRoutingTlvSize (i5_routing_table_type *table)
{
  int totalLength = sizeof(i5_tlv_t) + sizeof(table->numEntries);
  int entry = 0;

  i5_routing_table_entry *currEntry = (i5_routing_table_entry *)table->entryList.ll.next;

  for ( ; (entry < table->numEntries) && currEntry ; entry ++) {
    totalLength += MAC_ADDR_LEN;                                    /* 6 bytes for the interface MAC */
    totalLength += sizeof(table->entryList.numDestinations);        /* 1 byte for the number of Destinations */

    totalLength += table->entryList.numDestinations * MAC_ADDR_LEN; /* 6 bytes for each Destination */

    currEntry = (i5_routing_table_entry *)currEntry->ll.next;
  }
  return totalLength;
}

/* This TLV should only be inserted nested in the Vendor Specific TLV */
int i5Tlv_brcm_RoutingTableInsert (i5_message_type * pmsg, i5_routing_table_type *table)
{
  int totalLength = i5Tlv_brcm_getRoutingTlvSize(table);
  unsigned char buf[totalLength];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  unsigned int entryIndex = 0;
  int index = 0;

  i5_routing_table_entry *currEntry = (i5_routing_table_entry *)table->entryList.ll.next;

  ptlv->type = i5TlvBrcmRoutingTableType;
  ptlv->length = htons(totalLength - sizeof(i5_tlv_t));
  index += sizeof(i5_tlv_t);

  buf[index] = table->numEntries;
  index += sizeof(table->numEntries);

  for ( ; (entryIndex < table->numEntries) && currEntry; entryIndex++) {
    unsigned int destIndex = 0;
    i5_routing_destination *currDest = (i5_routing_destination *)currEntry->destinationList.ll.next;

    memcpy (&buf[index], currEntry->interfaceMac, MAC_ADDR_LEN);
    index += MAC_ADDR_LEN;

    buf[index] = currEntry->numDestinations;
    index += sizeof(currEntry->numDestinations);

    for ( ; (destIndex < currEntry->numDestinations) && currDest; destIndex ++) {
      memcpy (&buf[index], currDest->macAddress, MAC_ADDR_LEN);
      index += MAC_ADDR_LEN;

      currDest = (i5_routing_destination *)currDest->ll.next;
    }

    currEntry = (i5_routing_table_entry *)currEntry->ll.next;
  }

  return (i5MessageInsertTlv(pmsg, buf, totalLength));
}

int i5TlvLinkMetricQueryInsert (i5_message_type * pmsg, enum i5TlvLinkMetricNeighbour_Values specifyAddress,
                                unsigned char const * mac_address, enum i5TlvLinkMetricType_Values metricTypes)
{
  int totalLength = sizeof(i5_tlv_t) + i5TlvLinkMetricNeighbour_Length +
                    ((specifyAddress == i5TlvLinkMetricNeighbour_Specify) ? MAC_ADDR_LEN : 0) +
                    i5TlvLinkMetricType_Length;
  unsigned char buf[totalLength];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  unsigned int index = 0;

  ptlv->type = i5TlvLinkMetricQueryType;
  ptlv->length = htons(totalLength - sizeof(i5_tlv_t));
  index += sizeof(i5_tlv_t);

  buf[index] = (unsigned char)specifyAddress;
  index += i5TlvLinkMetricNeighbour_Length;

  if (specifyAddress == i5TlvLinkMetricNeighbour_Specify) {
    if (mac_address) {
      memcpy (&buf[index], mac_address, MAC_ADDR_LEN);
      index += MAC_ADDR_LEN;
    } else {
      /* The caller wants to specify a MAC, but didn't provide one */
      return -1;
    }
  }

  buf[index] = (unsigned char)metricTypes;

  return (i5MessageInsertTlv(pmsg, buf, totalLength));
}

int i5TlvLinkMetricQueryExtract(i5_message_type * pmsg,
                                unsigned char * neighbours,
                                unsigned char * alMacAddress,
                                enum i5TlvLinkMetricType_Values * metricsRequested)
{
  int             rc;
  unsigned char  *pValue;
  unsigned int    length;

  rc = i5MessageTlvExtract(pmsg, i5TlvLinkMetricQueryType, &length, &pValue, i5MessageTlvExtractWithReset);
  if (rc == 0) {
    if (length == 8) {
      *neighbours = pValue[0];
      pValue += i5TlvLinkMetricNeighbour_Length;
      memcpy(alMacAddress, pValue, MAC_ADDR_LEN);
      pValue += MAC_ADDR_LEN;
      *metricsRequested = (enum i5TlvLinkMetricType_Values) pValue[0];
    }
    else if (length == 2) {
      *neighbours = pValue[0];
      pValue += i5TlvLinkMetricNeighbour_Length;
      if (*neighbours == i5TlvLinkMetricNeighbour_Specify) {
        i5TraceError("Illegal Packet: length is 2 but 'specify neighbour' is chosen.\n");
        return -1;
      }
      /* There is no MAC address field in the 2-byte version of the query */
      *metricsRequested = (enum i5TlvLinkMetricType_Values) pValue[0];
    }
    else {
      i5TraceError("Read Error: packet length must be 8 or 2.\n");
    }
  }
  else {
    i5Trace("Read failure rc=%d length=%d\n",rc,length);
    return -1;
  }

  return 0;
}

/* The only "result code" is "Invalid" meaning that the neighbor info was requested for a non-neighbor */
int i5TlvLinkMetricResultCodeInsert (i5_message_type * pmsg)
{
  int totalLength = sizeof(i5_tlv_t) + i5TlvLinkMetricResultCode_Length;
  unsigned char buf[totalLength];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;

  ptlv->type = i5TlvLinkMetricResultCodeType;
  ptlv->length = htons(i5TlvLinkMetricResultCode_Length);
  buf[sizeof(i5_tlv_t)] = i5TlvLinkMetricResultCode_InvalidNeighbor;

  return (i5MessageInsertTlv(pmsg, buf, totalLength));
}

/* This function is more about future-proofing
 * The only current "result code" is "Invalid" meaning that the request for neighbor info has been rejected because we don't have that neighbor
 * So really, someone receiving a "Link Metric Result Code" Packet has no need to process the TLV
 * since the only "resultCode" they'll ever get is "0"
 */
int i5TlvLinkMetricResultCodeExtract (i5_message_type * pmsg, enum i5TlvLinkMetricResultCode_Values * resultCode)
{
  int             rc;
  unsigned char  *pValue;
  unsigned int    length;

  rc = i5MessageTlvExtract(pmsg, i5TlvLinkMetricResultCodeType, &length, &pValue, i5MessageTlvExtractWithReset);
  if ((rc == 0) && (length == i5TlvLinkMetricResultCode_Length)) {
    *resultCode = pValue[0];
  } else {
    i5Trace("Read failure rc=%d length=%d\n",rc,length);
    return -1;
  }

  return 0;
}

int i5TlvLinkMetricTxInsert (i5_message_type * pmsg,
                             unsigned char const * local_al_mac, unsigned char const * neighbor_al_mac,
                             i5_tlv_linkMetricTx_t const * txStats, int numLinks)
{
  int totalLength = sizeof(i5_tlv_t) + i5TlvLinkMetricTxOverhead_Length + i5TlvLinkMetricTxPerLink_Length * numLinks;
  unsigned char buf[totalLength];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  unsigned int index = 0;
  unsigned int linkCounter = 0;
  unsigned int remaining, fullPacket;

  i5Trace("\n");

  i5MessageGetPacketSpace(pmsg, &remaining, &fullPacket);

  i5TraceInfo("remain = %d  fullPacket = %d\n", remaining, fullPacket);

  if (remaining < sizeof(i5_tlv_t) + i5TlvLinkMetricTxOverhead_Length + i5TlvLinkMetricTxPerLink_Length) {
    /* Can't fit even one metric with what's left */
    remaining = fullPacket;
    i5TraceInfo("Setting remaining = %d full packet size\n", remaining);
  }

  ptlv->type = i5TlvTransmitterLinkMetricType;
  ptlv->length = 0; /* this is a variable length packet, add as we go */
  index += sizeof(i5_tlv_t);
  remaining -= sizeof(i5_tlv_t);

  memcpy(&buf[index], local_al_mac, MAC_ADDR_LEN);
  index += MAC_ADDR_LEN;

  memcpy(&buf[index], neighbor_al_mac, MAC_ADDR_LEN);
  index += MAC_ADDR_LEN;
  ptlv->length = htons( ntohs(ptlv->length) + i5TlvLinkMetricTxOverhead_Length);
  remaining -= i5TlvLinkMetricTxOverhead_Length;

  for (;linkCounter < numLinks;linkCounter++) {
    memcpy (&buf[index], &txStats[linkCounter], (MAC_ADDR_LEN * 2));
    index += (MAC_ADDR_LEN * 2);
    index += i5_cpy_host16_to_netbuf(&buf[index], txStats[linkCounter].intfType);
    buf[index] = txStats[linkCounter].ieee8021BridgeFlag;
    index += 1;
    index += i5_cpy_host32_to_netbuf(&buf[index], txStats[linkCounter].packetErrors);
    index += i5_cpy_host32_to_netbuf(&buf[index], txStats[linkCounter].transmittedPackets);
    index += i5_cpy_host16_to_netbuf(&buf[index], txStats[linkCounter].macThroughPutCapacity);
    index += i5_cpy_host16_to_netbuf(&buf[index], txStats[linkCounter].linkAvailability);
    index += i5_cpy_host16_to_netbuf(&buf[index], txStats[linkCounter].phyRate);

    ptlv->length = htons( ntohs(ptlv->length) + i5TlvLinkMetricTxPerLink_Length);
    remaining -= i5TlvLinkMetricTxPerLink_Length;

    i5TraceInfo("Compare remain = %d  i5TlvLinkMetricTxPerLink_Length = %d\n", remaining, i5TlvLinkMetricTxPerLink_Length);
    if (remaining < i5TlvLinkMetricTxPerLink_Length)
    {
      /* Send the TLV */
      if (i5MessageInsertTlv(pmsg, buf, ntohs(ptlv->length) + sizeof(i5_tlv_t) ) ) {
        return -1;
      }
      i5TraceInfo("Resetting\n");
      /* Reset the TLV */
      ptlv->length = htons(i5TlvLinkMetricTxOverhead_Length);
      index = sizeof(i5_tlv_t) + i5TlvLinkMetricTxOverhead_Length;
      remaining = fullPacket - index;
    }
  }

  if (ntohs(ptlv->length) > sizeof(i5_tlv_t) + i5TlvLinkMetricTxOverhead_Length) {
    return (i5MessageInsertTlv(pmsg, buf, ntohs(ptlv->length) + sizeof(i5_tlv_t) ) );
  }
  return 0;
}

/* return of 0  : okay
 * return of -1 : an error
 * return of -2 : not found
 * if numLinksReturned > maxLinks, this means that there were more links available, but they could not be returned
 *   (in this situation, the function returns 0 anyway, but the caller can tell the information is incomplete)
 */
int i5TlvLinkMetricTxExtract (i5_message_type * pmsg,
                              unsigned char * reporter_al_mac, unsigned char * neighbor_al_mac,
                              i5_tlv_linkMetricTx_t * txStats, int maxLinks, int *numLinksReturned)
{
  int             rc;
  unsigned char  *pValue;
  unsigned int    length;
  int linkCounter = 0;

  rc = i5MessageTlvExtract(pmsg, i5TlvTransmitterLinkMetricType, &length, &pValue, i5MessageTlvExtractWithoutReset);

  /* Validate the length */
  if (rc != 0) {
    return rc;
  }
  else if ((length - i5TlvLinkMetricTxOverhead_Length) % i5TlvLinkMetricTxPerLink_Length != 0) {
    i5Trace("Read failure rc=%d length=%d (must be %d + %d n) \n",rc,length,i5TlvLinkMetricTxOverhead_Length,i5TlvLinkMetricTxPerLink_Length);
    return -1;
  }

  *numLinksReturned = (length-i5TlvLinkMetricTxOverhead_Length) / i5TlvLinkMetricTxPerLink_Length;

  memcpy (reporter_al_mac, pValue, MAC_ADDR_LEN);
  pValue += MAC_ADDR_LEN;

  memcpy (neighbor_al_mac, pValue, MAC_ADDR_LEN);
  pValue += MAC_ADDR_LEN;

  for ( ; (linkCounter < *numLinksReturned) && (linkCounter < maxLinks) ; linkCounter++) {
    memcpy (&txStats[linkCounter], pValue, (MAC_ADDR_LEN * 2));
    pValue += (MAC_ADDR_LEN * 2);
    pValue += i5_cpy_netbuf_to_host16(&txStats[linkCounter].intfType, pValue);
    txStats[linkCounter].ieee8021BridgeFlag = pValue[0];
    pValue += 1;
    pValue += i5_cpy_netbuf_to_host32(&txStats[linkCounter].packetErrors, pValue);
    pValue += i5_cpy_netbuf_to_host32(&txStats[linkCounter].transmittedPackets, pValue);
    pValue += i5_cpy_netbuf_to_host16(&txStats[linkCounter].macThroughPutCapacity, pValue);
    pValue += i5_cpy_netbuf_to_host16(&txStats[linkCounter].linkAvailability, pValue);
    pValue += i5_cpy_netbuf_to_host16(&txStats[linkCounter].phyRate, pValue);
  }

  /* Everything is fine, numLinksReturned vs maxLinks will tell the caller anything else */
  return 0;
}

int i5TlvLinkMetricRxInsert (i5_message_type * pmsg,
                             unsigned char const * local_al_mac, unsigned char const * neighbor_al_mac,
                             i5_tlv_linkMetricRx_t const * rxStats, int numLinks)
{
  int totalLength = sizeof(i5_tlv_t) + i5TlvLinkMetricRxOverhead_Length + i5TlvLinkMetricRxPerLink_Length * numLinks;
  unsigned char buf[totalLength];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  unsigned int index = 0;
  unsigned int linkCounter = 0;
  unsigned int remaining, fullPacket;

  i5Trace("\n");

  i5MessageGetPacketSpace(pmsg, &remaining, &fullPacket);

  i5TraceInfo("remain = %d  fullPacket = %d\n", remaining, fullPacket);

  if (remaining < sizeof(i5_tlv_t) + i5TlvLinkMetricRxOverhead_Length + i5TlvLinkMetricRxPerLink_Length) {
    /* Can't fit even one metric with what's left */
    remaining = fullPacket;
    i5TraceInfo("Setting remain = %d full packet size\n", remaining);
  }

  ptlv->type = i5TlvReceiverLinkMetricType;
  ptlv->length = 0; /* this is a variable length packet */
  index += sizeof(i5_tlv_t);
  remaining -= sizeof(i5_tlv_t);

  memcpy(&buf[index], local_al_mac, MAC_ADDR_LEN);
  index += MAC_ADDR_LEN;

  memcpy(&buf[index], neighbor_al_mac, MAC_ADDR_LEN);
  index += MAC_ADDR_LEN;
  ptlv->length = htons( ntohs(ptlv->length) + i5TlvLinkMetricRxOverhead_Length);
  remaining -= i5TlvLinkMetricRxOverhead_Length;

  for (;linkCounter < numLinks; linkCounter++) {
    memcpy (&buf[index], &rxStats[linkCounter], (MAC_ADDR_LEN * 2));
    index += (MAC_ADDR_LEN * 2);
    index += i5_cpy_host16_to_netbuf(&buf[index], rxStats[linkCounter].intfType);
    index += i5_cpy_host32_to_netbuf(&buf[index], rxStats[linkCounter].packetErrors);
    index += i5_cpy_host32_to_netbuf(&buf[index], rxStats[linkCounter].receivedPackets);
    buf[index] = rxStats[linkCounter].rcpi;
    index += 1;

    ptlv->length = htons( ntohs(ptlv->length) + i5TlvLinkMetricRxPerLink_Length);
    remaining -= i5TlvLinkMetricRxPerLink_Length;

    i5TraceInfo("Compare remain = %d  i5TlvLinkMetricRxPerLink_Length = %d\n", remaining, i5TlvLinkMetricRxPerLink_Length);
    if (remaining < i5TlvLinkMetricRxPerLink_Length)
    {
      /* Send the TLV */
      if (i5MessageInsertTlv(pmsg, buf, ntohs(ptlv->length) + sizeof(i5_tlv_t) ) ) {
        return -1;
      }
      i5TraceInfo("Resetting\n");
      /* Reset the TLV */
      ptlv->length = htons(i5TlvLinkMetricRxOverhead_Length);
      index = sizeof(i5_tlv_t) + i5TlvLinkMetricRxOverhead_Length;
      remaining = fullPacket - index;
    }

  }

  if (ntohs(ptlv->length) > sizeof(i5_tlv_t) + i5TlvLinkMetricRxOverhead_Length) {
    return (i5MessageInsertTlv(pmsg, buf, ntohs(ptlv->length) + sizeof(i5_tlv_t) ) );
  }
  return 0;
}

/* return of 0  : okay
 * return of -1 : an error
 * return of -2 : not found
 * if numLinksReturned > maxLinks, this means that there were more links available, but they could not be returned
 *   (in this situation, the function returns 0 anyway, but the caller can tell the information is incomplete)
 */
int i5TlvLinkMetricRxExtract (i5_message_type * pmsg,
                              unsigned char * reporter_al_mac, unsigned char * neighbor_al_mac,
                              i5_tlv_linkMetricRx_t * rxStats, int maxLinks, int *numLinksReturned)
{
  int             rc;
  unsigned char  *pValue;
  unsigned int    length;
  int linkCounter = 0;

  rc = i5MessageTlvExtract(pmsg, i5TlvReceiverLinkMetricType, &length, &pValue, i5MessageTlvExtractWithoutReset);

  /* Validate the length */
  if (rc != 0) {
    return rc;
  }
  else if ((length - i5TlvLinkMetricRxOverhead_Length) % i5TlvLinkMetricRxPerLink_Length != 0) {
    i5Trace("Read failure rc=%d length=%d (must be %d + %d n) \n",rc,length,i5TlvLinkMetricRxOverhead_Length,i5TlvLinkMetricRxPerLink_Length);
    return -1;
  }

  *numLinksReturned = (length-i5TlvLinkMetricRxOverhead_Length) / i5TlvLinkMetricRxPerLink_Length;

  memcpy (reporter_al_mac, pValue, MAC_ADDR_LEN);
  pValue += MAC_ADDR_LEN;

  memcpy (neighbor_al_mac, pValue, MAC_ADDR_LEN);
  pValue += MAC_ADDR_LEN;

  for ( ; (linkCounter < *numLinksReturned) && (linkCounter < maxLinks) ; linkCounter++) {
    memcpy (&rxStats[linkCounter], pValue, (MAC_ADDR_LEN * 2));
    pValue += (MAC_ADDR_LEN * 2);
    pValue += i5_cpy_netbuf_to_host16(&rxStats[linkCounter].intfType, pValue);
    pValue += i5_cpy_netbuf_to_host32(&rxStats[linkCounter].packetErrors, pValue);
    pValue += i5_cpy_netbuf_to_host32(&rxStats[linkCounter].receivedPackets, pValue);
    rxStats[linkCounter].rcpi = *pValue;
    pValue += 1;
  }

  /* Everything is fine, numLinksReturned vs maxLinks will tell the caller anything else */
  return 0;
}

#ifdef MULTIAP
static void i5ConvertTxStatsToLinkeMetric(i5_tlv_linkMetricTx_t *txStats,
  ieee1905_backhaul_link_metric *metric)
{
  metric->txPacketErrors = txStats->packetErrors;
  metric->transmittedPackets = txStats->transmittedPackets;
  metric->macThroughPutCapacity = txStats->macThroughPutCapacity;
  metric->linkAvailability = txStats->linkAvailability;
  metric->phyRate = txStats->phyRate;
}

static void i5ConvertRxStatsToLinkeMetric(i5_tlv_linkMetricRx_t *rxStats,
  ieee1905_backhaul_link_metric *metric)
{
  metric->receivedPackets = rxStats->receivedPackets;
  metric->rxPacketErrors = rxStats->packetErrors;
  metric->rcpi = rxStats->rcpi;
}
#endif /* MULTIAP */

void i5TlvLinkMetricResponseExtract(i5_message_type * pmsg)
{
  i5_dm_device_type *reportingDevice = NULL;

  i5MessageReset(pmsg);

  /* loop through all TLVs */
  while (1) {
    unsigned char reporter_al_mac[MAC_ADDR_LEN];
    unsigned char neighbor_al_mac[MAC_ADDR_LEN];
    i5_tlv_linkMetricTx_t txStats[3];
    int numLinks = 0;
    int linkIndex = 0;

    int rc = i5TlvLinkMetricTxExtract(pmsg, reporter_al_mac, neighbor_al_mac, txStats, 3, &numLinks);
    i5DmRefreshDeviceTimer(reporter_al_mac, 0);

    if (-2 == rc) {
      i5Trace("No more TxStats TLVs.\n");
      break;
    }
    else if (-1 == rc) {
      i5TraceError("Error in Tx TLV\n");
      break;
    }
    /* if txstats TLV, extract addr, addr, macthroughput, linkavail */
    i5Trace("TxStats received, processing.\n");

    reportingDevice = i5DmDeviceFind(reporter_al_mac);
    if (!reportingDevice) {
      i5TraceInfo("Reporting device " I5_MAC_DELIM_FMT " not found\n", I5_MAC_PRM(reporter_al_mac));
      continue;
    }

    i5TraceInfo("Reporter: " I5_MAC_DELIM_FMT " Regarding: " I5_MAC_DELIM_FMT " \n",
      I5_MAC_PRM(reporter_al_mac),
      I5_MAC_PRM(neighbor_al_mac));
    if (numLinks > 3) {
      numLinks = 3;
    }
    for ( ; linkIndex < numLinks ; linkIndex ++) {
      i5TraceInfo("Rep's If: " I5_MAC_DELIM_FMT " Other I/f: " I5_MAC_DELIM_FMT " %d/%d \n",
        I5_MAC_PRM(txStats[linkIndex].localInterface),
        I5_MAC_PRM(txStats[linkIndex].neighborInterface),
        txStats[linkIndex].linkAvailability,
        txStats[linkIndex].macThroughPutCapacity);
      i5_dm_1905_neighbor_type *neighbor = i5Dm1905NeighborFind(reportingDevice, txStats[linkIndex].localInterface, neighbor_al_mac);
      if (!neighbor) {
        i5TraceInfo("Neighbor device " I5_MAC_DELIM_FMT " not found\n", I5_MAC_PRM(neighbor_al_mac));
        continue;
      }
#ifdef MULTIAP
      ieee1905_backhaul_link_metric metric;
      memset(&metric, 0, sizeof(metric));
      i5ConvertTxStatsToLinkeMetric(&txStats[linkIndex], &metric);
      i5Dm1905NeighborLinkMetricUpdate(neighbor, &metric, 0,
        I5_DM_LINK_METRIC_UPDATE_TX | I5_DM_LINK_METRIC_UPDATE_RAW);
#else
      i5Dm1905NeighborBandwidthUpdate(neighbor, txStats[linkIndex].macThroughPutCapacity, txStats[linkIndex].linkAvailability, 0,
                                      reporter_al_mac);
#endif /* MULTIAP */
    }
  }

  i5MessageReset(pmsg);
  while (1) {
    unsigned char reporter_al_mac[MAC_ADDR_LEN];
    unsigned char neighbor_al_mac[MAC_ADDR_LEN];
    i5_tlv_linkMetricRx_t rxStats[3];
    int numLinks = 0;
    int linkIndex = 0;
    int rc = i5TlvLinkMetricRxExtract(pmsg, reporter_al_mac, neighbor_al_mac, rxStats, 3, &numLinks);
    i5DmRefreshDeviceTimer(reporter_al_mac, 0);
    if (rc != 0) {
      break;
    }
    i5Trace("RxStats received, processing.\n");

    reportingDevice = i5DmDeviceFind(reporter_al_mac);
    if (!reportingDevice) {
      i5TraceInfo("Reporting device " I5_MAC_DELIM_FMT " not found\n", I5_MAC_PRM(reporter_al_mac));
      continue;
    }

    i5TraceInfo("Reporter: " I5_MAC_DELIM_FMT " Regarding: " I5_MAC_DELIM_FMT " \n",
      I5_MAC_PRM(reporter_al_mac),
      I5_MAC_PRM(neighbor_al_mac));
    if (numLinks > 3) {
      numLinks = 3;
    }

    for ( ; linkIndex < numLinks ; linkIndex ++) {
      i5TraceInfo("Rep's If: " I5_MAC_DELIM_FMT " Other I/f: " I5_MAC_DELIM_FMT " %d/%d/%d \n",
        I5_MAC_PRM(rxStats[linkIndex].localInterface),
        I5_MAC_PRM(rxStats[linkIndex].neighborInterface),
        rxStats[linkIndex].packetErrors,
        rxStats[linkIndex].receivedPackets, rxStats[linkIndex].rcpi);
      i5_dm_1905_neighbor_type *neighbor = i5Dm1905NeighborFind(reportingDevice,
        rxStats[linkIndex].localInterface, neighbor_al_mac);
      if (!neighbor) {
        i5TraceInfo("Neighbor device " I5_MAC_DELIM_FMT " not found\n",
          I5_MAC_PRM(neighbor_al_mac));
        continue;
      }
#ifdef MULTIAP
      ieee1905_backhaul_link_metric metric;
      memset(&metric, 0, sizeof(metric));
      i5ConvertRxStatsToLinkeMetric(&rxStats[linkIndex], &metric);
      i5Dm1905NeighborLinkMetricUpdate(neighbor, &metric, 0,
        I5_DM_LINK_METRIC_UPDATE_RX | I5_DM_LINK_METRIC_UPDATE_RAW);
#endif /* MULTIAP */
    }
  }
}

int i5TlvLldpTypeInsert(i5_message_type *pmsg, const unsigned char *chassis_mac, const unsigned char *portid_mac)
{
  unsigned char buf[32];
  int len = 0;
  unsigned short val;

  /* for LLDP - TLV type is 7 bits and TLV length is 9 bits */

  /* chassis ID */
  buf[len] = I5_LLDP_CHASIS_ID_TLV_TYPE; /* chassis ID type is 1 */
  len++;
  buf[len] = MAC_ADDR_LEN + 1; /* MAC addres + chassis subtype */
  len++;
  buf[len] = 0x4;              /* chassis ID subtype - 4 - MAC address */
  len++;
  memcpy(&buf[len], chassis_mac, MAC_ADDR_LEN);
  len+=MAC_ADDR_LEN;

  /* port ID */
  buf[len] = I5_LLDP_PORT_ID_TLV_TYPE; /* port ID type is 2 */
  len++;
  buf[len] = MAC_ADDR_LEN + 1; /* MAC addres + port id subtype */
  len++;
  buf[len] = 0x3;              /* port id subtype - 3 - MAC address */
  len++;
  memcpy(&buf[len], portid_mac, MAC_ADDR_LEN);
  len+=MAC_ADDR_LEN;

  /* TTL */
  buf[len] = 0x6;              /* port ID type is 3 */
  len++;
  buf[len] = 2;                /* ttl vlaue is two bytes  - value is 180 as per 1905 spec */
  len++;
  val = 180;
  i5_cpy_host16_to_netbuf(&buf[len], val);
  len+=2;

  /* End of LLDP */
  buf[len] = 0;                /* end of lldp type is 0 */
  len++;
  buf[len] = 0;                /* end of lldp length is 0 */
  len++;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

int i5TlvLldpTypeExtract(i5_message_type *pmsg, unsigned char *neighbor_al_mac, unsigned char *neighbor_interface_mac)
{
  unsigned char *ptr = &pmsg->ppkt->pbuf[0];
  int            index = sizeof(struct ethhdr);
  int            tlvType;
  int            tlvLen;

  /* for LLDP - TLV type is 7 bits and TLV length is 9 bits */

  while ( index < pmsg->ppkt->length ) {
    tlvType = ptr[index] >> 1;
    tlvLen  = (ptr[index] & 0x01) << 8;
    index++;
    tlvLen |= ptr[index];
    index++;
    if ( tlvLen + index > pmsg->ppkt->length ) {
      return -1;
    }

    if ( 0x00 == tlvType ) {
      break;
    }

    /* chassis ID */
    if ( 0x1 == tlvType ) {
      /* if chassis id sub type is not MAC address break */
      if (ptr[index] != 0x4) {
        break;
      }
      else {
        memcpy(neighbor_al_mac, &ptr[index+1], MAC_ADDR_LEN);
      }
    }

    /* port ID */
    if ( 0x2 == tlvType ) {
      /* if port id sub type is not MAC address break */
      if (ptr[index] != 0x3) {
        break;
      }
      else {
        memcpy(neighbor_interface_mac, &ptr[index+1], MAC_ADDR_LEN);
      }
    }

    index += tlvLen;
  }

  return 0;
}

/* Friendly Name TLV
 * Variable bytes: friendlyName
 */
int i5TlvFriendlyNameInsert(i5_message_type *pmsg, char const *friendlyName)
{
  int totalLength = sizeof(i5_tlv_t) + strlen((char *)friendlyName) + 1; /* room for the null char */
  char buf[totalLength];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int index = 0;

  i5Trace("\n");

  ptlv->type = i5TlvBrcmFriendlyNameType;
  ptlv->length = htons(totalLength - sizeof(i5_tlv_t));
  index += sizeof(i5_tlv_t);

  strcpy(&buf[index], (char *) friendlyName);
  buf[totalLength-1] = '\0';

  return (i5MessageInsertTlv(pmsg, (unsigned char *)buf, totalLength));
}

int i5TlvFriendlyNameExtract(i5_message_type *pmsg, char *friendlyName, int maxFriendlyNameSize)
{
  int             rc;
  unsigned char  *pValue;
  unsigned int    length;
  int index = 0;

  rc = i5MessageTlvExtract(pmsg, i5TlvBrcmFriendlyNameType, &length, &pValue, i5MessageTlvExtractWithReset);
  if (rc == 0) {
    strncpy( friendlyName, (char *)&pValue[index], maxFriendlyNameSize);
    friendlyName[maxFriendlyNameSize-1] = '\0';
  }
  else {
    i5Trace("Read failure rc=%d length=%d\n",rc,length);
    return -1;
  }

  return 0;
}

/* Friendly URL TLV
 * Variable bytes: Control URL
 */
int i5TlvFriendlyUrlInsert(i5_message_type *pmsg, unsigned char const *controlUrl)
{
  int totalLength = sizeof(i5_tlv_t) + strlen((char *)controlUrl) + 1; /* room for the null char */
  char buf[totalLength];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int index = 0;

  i5Trace("\n");

  ptlv->type = i5TlvBrcmFriendlyUrlType;
  ptlv->length = htons(totalLength - sizeof(i5_tlv_t));
  index += sizeof(i5_tlv_t);

  strcpy(&buf[index], (char *) controlUrl);
  buf[totalLength-1] = '\0';

  return (i5MessageInsertTlv(pmsg, (unsigned char *)buf, totalLength));
}

int i5TlvFriendlyUrlExtract(i5_message_type *pmsg, unsigned char *controlUrl, int maxControlUrlSize)
{
  int             rc;
  unsigned char  *pValue;
  unsigned int    length;
  int index = 0;

  rc = i5MessageTlvExtract(pmsg, i5TlvBrcmFriendlyUrlType, &length, &pValue, i5MessageTlvExtractWithReset);
  if (rc == 0) {
    strncpy( (char *)controlUrl, (char *)&pValue[index], maxControlUrlSize);
    controlUrl[maxControlUrlSize-1] = '\0';
  } else {
    i5Trace("Read failure rc=%d length=%d\n",rc,length);
    return -1;
  }

  return 0;
}

int i5TlvFriendlyIpv4Insert (i5_message_type * pmsg, i5_tlv_ipv4Type_t const *ipv4Info, unsigned char numEntries)
{
  int totalLength = sizeof(i5_tlv_t) + sizeof(char) + (sizeof(i5_tlv_ipv4Type_t) * numEntries);
  char buf[totalLength];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int index = 0;
  int entryCounter = 0;

  i5Trace("Inserting %d entries of size %d\n", numEntries, (int)sizeof(i5_tlv_ipv4Type_t) );

  ptlv->type = i5TlvBrcmFriendlyIpv4Type;
  ptlv->length = htons(totalLength - sizeof(i5_tlv_t));
  index += sizeof(i5_tlv_t);

  buf[index] = (char) numEntries;
  index ++;

  for ( ; entryCounter < numEntries; entryCounter ++) {
    // TBD - may have to use htonl() on these IP addresses.
    memcpy(&buf[index], (char *) (&ipv4Info[entryCounter]), sizeof(i5_tlv_ipv4Type_t) );
    index += sizeof(i5_tlv_ipv4Type_t);
  }

  return (i5MessageInsertTlv(pmsg, (unsigned char *)buf, totalLength));
}

int i5TlvFriendlyIpv4Extract (i5_message_type * pmsg, i5_tlv_ipv4Type_t *ipv4Info, unsigned char const maxEntries, unsigned char *numEntriesReturned)
{
  int             rc;
  unsigned char  *pValue;
  unsigned int    length;
  int index = 0;
  int entryCounter = 0;
  numEntriesReturned = 0;

  rc = i5MessageTlvExtract(pmsg, i5TlvBrcmFriendlyIpv4Type, &length, &pValue, i5MessageTlvExtractWithReset);
  if (rc == 0) {

    int entries = pValue[index];
    index ++;

    for ( ; (entryCounter < entries) && (entryCounter < maxEntries); entryCounter++) {
      // separate check for length violation
      if ( (index + sizeof(i5_tlv_ipv4Type_t) ) > length ) {
        i5TraceError("Packet not long enough for entry #%d\n", entryCounter + 1);
        if (numEntriesReturned) {
          return 0;
        }
        else {
          return -1;
        }
      }

      // TBD - probably some htonl() here, too
      memcpy( (char *)(&ipv4Info[entryCounter]), (char *)(&pValue[index]), sizeof(i5_tlv_ipv4Type_t) );
      numEntriesReturned ++;
      index += sizeof(i5_tlv_ipv4Type_t);
    }

  } else {
    i5Trace("Read failure rc=%d length=%d\n",rc,length);
    return -1;
  }

  return 0;
}

int i5TlvVendorSpecificTypeInsert(i5_message_type *pmsg, unsigned char *vendorSpec_msg, unsigned int vendorSpec_len)
{
  unsigned char *pbuf;
  i5_tlv_t *ptlv = NULL;
  unsigned short len = i5TlvVendorSpecificOui_Length + vendorSpec_len;
  int index = 0;
  int rc = 0;

  if ((pbuf = (unsigned char *)malloc(len + sizeof(i5_tlv_t))) == NULL) {
    printf("malloc error\n");
    return -1;
  }
  ptlv = (i5_tlv_t *)pbuf;
  ptlv->type = i5TlvVendorSpecificType;
  ptlv->length = htons(len);
  index += sizeof(i5_tlv_t);

  /* copy the OUI */
  pbuf [index]   = i5TlvVendorSpecificOui_Byte1;
  pbuf [index+1] = i5TlvVendorSpecificOui_Byte2;
  pbuf [index+2] = i5TlvVendorSpecificOui_Byte3;
  index += i5TlvVendorSpecificOui_Length;

  /* copy the vendor specific info */
  memcpy(&pbuf[index], vendorSpec_msg, vendorSpec_len);
  index += vendorSpec_len;

  rc = i5MessageInsertTlv(pmsg, pbuf, index);
  free (pbuf);
  return rc;
}

/* This function returns the vendor specific part of the message in its entirety
 * (not including the OUI, but otherwise unparsed)
 */
int i5TlvVendorSpecificTypeExtract(i5_message_type *pmsg, char withReset,
  unsigned char **vendorSpec_data, unsigned int * vendorSpec_len)
{
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvVendorSpecificType, &length, vendorSpec_data, withReset) == 0) {
    i5TraceInfo("Extracted %d bytes at %p\n", length, *vendorSpec_data);
    if (((*vendorSpec_data)[0] == i5TlvVendorSpecificOui_Byte1) &&
        ((*vendorSpec_data)[1] == i5TlvVendorSpecificOui_Byte2) &&
        ((*vendorSpec_data)[2] == i5TlvVendorSpecificOui_Byte3)) {
      /* The user doesn't want to know about the OUI, so move the pointer past it */
      *vendorSpec_data += i5TlvVendorSpecificOui_Length;
      *vendorSpec_len = length - i5TlvVendorSpecificOui_Length;
      return 0;
    }
    else {
      i5TraceInfo("Non Broadcom Vendor specific ID\n");
      return -1;
    }

  }
  return -1;
}

#ifdef MULTIAP
/* Multi AP Related TLV's Follows */

int i5TlvSupportedServiceTypeInsert(i5_message_type *pmsg)
{
  unsigned char *pbuf, *pmem, *ptmpbuf;
  unsigned char service_count = 0;
  i5_dm_device_type *pdmdev;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pdmdev = i5DmGetSelfDevice()) == NULL) {
    return -1;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end
  pbuf++; /* Fill the number of supported service later */
  if (I5_IS_MULTIAP_CONTROLLER(i5_config.flags)) {
    *pbuf = i5MultiAPController;
    pbuf++;
    service_count++;
  }
  if (I5_IS_MULTIAP_AGENT(i5_config.flags)) {
    *pbuf = i5MultiAPAgent;
    pbuf++;
    service_count++;
  }

  /* Fill the number of supported service */
  ptmpbuf = pmem + sizeof(i5_tlv_t);
  *ptmpbuf = service_count;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvSupportedServicesType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

int i5TlvSupportedServiceTypeExtract(i5_message_type *pmsg, unsigned int *supportedService)
{
  unsigned char *pvalue;
  unsigned int length = 0;
  int rc = 0;
  unsigned char  i;
  unsigned int pos = 0;
  unsigned char num_supported_services;

  i5MessageReset(pmsg);
  rc = i5MessageTlvExtract(pmsg, i5TlvSupportedServicesType, &length, &pvalue, i5MessageTlvExtractWithoutReset);
  if (rc != 0) {
    goto end;
  }

  if (length <= 0) {
    rc = -1;
    goto end;
  }

  num_supported_services = pvalue[pos];
  pos++;

  for (i = 0; i < num_supported_services; i++) {
    if (length >= pos + 1) {
      if (pvalue[pos] == i5MultiAPController) {
        (*supportedService) |= I5_CONFIG_FLAG_CONTROLLER;
        pos++;
      } else if (pvalue[pos] == i5MultiAPAgent) {
        (*supportedService) |= I5_CONFIG_FLAG_AGENT;
        pos++;
      }
    }
  }

end:
  /* This is an optional TLV */
  if (rc == -2) {
    rc = 0;
  }

  return rc;
}

int i5TlvSearchedServiceTypeInsert(i5_message_type *pmsg, unsigned int searchService)
{
  unsigned char *pbuf, *pmem, *ptmpbuf;
  unsigned char service_count = 0;
  i5_dm_device_type *pdmdev;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pdmdev = i5DmGetSelfDevice()) == NULL) {
    return -1;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end
  pbuf++; /* Fill the number of supported service later */
  if (searchService & I5_CONFIG_FLAG_CONTROLLER) {
    *pbuf = i5MultiAPController;
    pbuf++;
    service_count++;
  }

  /* Fill the number of supported service */
  ptmpbuf = pmem + sizeof(i5_tlv_t);
  *ptmpbuf = service_count;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvSearchedServicesType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

int i5TlvSearchedServiceTypeExtract(i5_message_type *pmsg, unsigned int *searchedService)
{
  unsigned char *pvalue;
  unsigned int length = 0;
  int rc = 0;
  unsigned char  i;
  unsigned int pos = 0;
  unsigned char num_searched_services;

  i5MessageReset(pmsg);
  rc = i5MessageTlvExtract(pmsg, i5TlvSearchedServicesType, &length, &pvalue, i5MessageTlvExtractWithoutReset);
  if (rc != 0) {
    goto end;
  }

  if (length <= 0) {
    rc = -1;
    goto end;
  }

  num_searched_services = pvalue[pos];
  pos++;

  for (i = 0; i < num_searched_services; i++) {
    if (length >= pos + 1) {
      if (pvalue[pos] == i5MultiAPController) {
        (*searchedService) |= I5_CONFIG_FLAG_CONTROLLER;
        pos++;
      }
      if (pvalue[pos] == i5MultiAPAgent) {
        (*searchedService) |= I5_CONFIG_FLAG_AGENT;
        pos++;
      }
    }
  }

end:
  /* This is an optional TLV */
  if (rc == -2) {
    rc = 0;
  }

  return rc;
}

/* TLV for AP Radio Identifier */
int i5TlvAPRadioIndentifierTypeInsert(i5_message_type *pmsg, unsigned char *mac)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvAPRadioIndentifierType_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvAPRadioIndentifierType;
  ptlv->length = htons(i5TlvAPRadioIndentifierType_Length);
  len += sizeof(i5_tlv_t);

  memcpy(&buf[len], mac, MAC_ADDR_LEN);
  len += MAC_ADDR_LEN;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract AP Radio Identifier TLV */
int i5TlvAPRadioIndentifierTypeExtract(i5_message_type *pmsg, unsigned char **radio_macs_out,
  unsigned char *count)
{
  unsigned char *pvalue, *radio_macs = NULL, *tmp;
  unsigned char tmpCount = 0;
  unsigned int length;
  int rc = 0;

  i5MessageReset(pmsg);
  while ((rc = i5MessageTlvExtract(pmsg, i5TlvAPRadioIndentifierType, &length, &pvalue,
    i5MessageTlvExtractWithoutReset)) == 0) {
    if (length >= i5TlvAPRadioIndentifierType_Length) {
      if (radio_macs == NULL) {
        /* First time allocate */
        radio_macs = (unsigned char*)malloc(MAC_ADDR_LEN);
        if (radio_macs == NULL) {
          i5TraceDirPrint("Malloc for radio_macs failed\n");
          goto end;
        }
      } else {
        /* Reallocate memory for one more radio */
        tmp = (unsigned char*)realloc(radio_macs, ((tmpCount * MAC_ADDR_LEN) + MAC_ADDR_LEN));
        if (tmp == NULL) {
          i5TraceDirPrint("Realloc for radio_macs failed\n");
          goto end;
        }
        radio_macs = tmp;
      }
      memcpy(&radio_macs[tmpCount*MAC_ADDR_LEN], pvalue, MAC_ADDR_LEN);
      tmpCount++;
    }
  }

  if (radio_macs_out && count) {
    *radio_macs_out = radio_macs;
    *count = tmpCount;
  }

  return 0;

end:
  if (radio_macs) {
    free(radio_macs);
  }

  if (radio_macs_out && count) {
    *radio_macs_out = NULL;
    *count = 0;
  }
  return -1;
}

/* TLV to indicate all BSS(s) it is currently operating on each of its radios */
int i5TlvAPOperationalBSSTypeInsert(i5_message_type *pmsg)
{
  unsigned char num_of_ifr = 0;
  unsigned char *pbuf, *pmem, *pbufifr;
  i5_tlv_t *ptlv;
  i5_dm_device_type *pdmdev;
  i5_dm_interface_type *pdmif;
  i5_dm_bss_type *pdmbss;
  int rc = 0;

  if ((pdmdev = i5DmGetSelfDevice()) == NULL) {
    return -1;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  pbufifr = pbuf; /* Fill number of interfaces in the end */
  pbuf++;

  /* For all the interfaces in this device if, the device is Agent */
  if (I5_IS_MULTIAP_AGENT(i5_config.flags)) {
    for (pdmif = pdmdev->interface_list.ll.next; pdmif; pdmif = pdmif->ll.next) {

      /* Add only the wireless interfaces */
      if (!i5DmIsInterfaceWireless(pdmif->MediaType)) {
        continue;
      }

      memcpy(pbuf, pdmif->InterfaceId, MAC_ADDR_LEN);
      pbuf += MAC_ADDR_LEN;

      *pbuf = (unsigned char)pdmif->BSSNumberOfEntries;
      pbuf++;

      /* Fill all the operational BSS */
      for (pdmbss = pdmif->bss_list.ll.next; pdmbss; pdmbss = pdmbss->ll.next) {
        memcpy(pbuf, pdmbss->BSSID, MAC_ADDR_LEN);
        pbuf += MAC_ADDR_LEN;
        *pbuf = (unsigned char)pdmbss->ssid.SSID_len;
        pbuf++;
        memcpy(pbuf, pdmbss->ssid.SSID, pdmbss->ssid.SSID_len);
        pbuf += pdmbss->ssid.SSID_len;
      }
      num_of_ifr++;
    }
  }

  /* Now fill the number of interfaces */
  *pbufifr = (unsigned char)num_of_ifr;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvAPOperationalBSSType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract information from AP operational BSS TLV */
int i5TlvAPOperationalBSSTypeExtract(i5_message_type *pmsg, unsigned char *pdevid)
{
  unsigned char *pvalue;
  unsigned int length = 0;
  int rc = 0;
  unsigned char  i;
  unsigned int pos = 0;
  unsigned char num_interfaces;

  i5MessageReset(pmsg);
  rc = i5MessageTlvExtract(pmsg, i5TlvAPOperationalBSSType, &length, &pvalue, i5MessageTlvExtractWithoutReset);
  if (rc != 0) {
    goto end;
  }

  if (length <= 0) {
    rc = -1;
    goto end;
  }

  num_interfaces = pvalue[pos];
  pos++;

  for (i = 0; i < num_interfaces; i++) {
    if (length >= pos + MAC_ADDR_LEN + 1) {
      unsigned char interface_mac[MAC_ADDR_LEN];
      unsigned char num_bss;
      unsigned char k;

      memcpy(interface_mac, &pvalue[pos], MAC_ADDR_LEN);
      pos += MAC_ADDR_LEN;
      num_bss = pvalue[pos];
      pos++;
      for (k = 0; k < num_bss; k++) {
        if (length >= pos + MAC_ADDR_LEN + 1) {
          unsigned char bssid[MAC_ADDR_LEN];
          ieee1905_ssid_type ssid;

          memset(&ssid, 0, sizeof(ssid));
          memcpy(bssid, &pvalue[pos], MAC_ADDR_LEN);
          pos += MAC_ADDR_LEN;
          ssid.SSID_len = pvalue[pos];
          pos++;
          if (length >= pos + ssid.SSID_len) {
            memcpy(ssid.SSID, &pvalue[pos], ssid.SSID_len);
            pos += ssid.SSID_len;
          }
          i5DmBSSUpdate(pdevid, interface_mac, bssid, &ssid);
        }
      }
    }
  }

end:

  return rc;
}

/* TLV to indicate all the 802.11 clients that are directly associated with each of the BSS(s)
 * that is operated by the Multi-AP Agent
 */
int i5TlvAssocaitedClientsTypeInsert(i5_message_type *pmsg)
{
  unsigned char *pbuf, *pmem, *tmpBSSCountPtr;
  i5_tlv_t *ptlv;
  i5_dm_device_type *pdmdev;
  i5_dm_interface_type *pdmif;
  i5_dm_bss_type *pdmbss;
  i5_dm_clients_type *pdmclients;
  int rc = 0;
  unsigned char bss_count = 0;
  struct timeval now;

  if ((pdmdev = i5DmGetSelfDevice()) == NULL) {
    return -1;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  gettimeofday(&now, NULL);

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  /* Fill the BSS count in the end */
  tmpBSSCountPtr = pbuf;
  pbuf++;

  /* For all the interfaces in this devices */
  pdmif = (i5_dm_interface_type *)pdmdev->interface_list.ll.next;
  while (pdmif != NULL) {
    /* For all the operational BSS */
    pdmbss = (i5_dm_bss_type *)pdmif->bss_list.ll.next;
    while (pdmbss != NULL) {

      /* If the clients are associated */
      if (pdmbss->ClientsNumberOfEntries > 0) {
        bss_count++;
        memcpy(pbuf, pdmbss->BSSID, MAC_ADDR_LEN);
        pbuf += MAC_ADDR_LEN;
        *((unsigned short *)pbuf) = htons(pdmbss->ClientsNumberOfEntries);
        pbuf += 2;

        /* For all the associated clients */
        pdmclients = (i5_dm_clients_type *)pdmbss->client_list.ll.next;
        while (pdmclients != NULL) {
          long diff;
          unsigned short time_elapsed;

          memcpy(pbuf, pdmclients->mac, MAC_ADDR_LEN);
          pbuf += MAC_ADDR_LEN;

          diff = now.tv_sec - pdmclients->assoc_tm.tv_sec;
          /* Time elapsed value is 0x0000 – 0xFFFE: 0 - 65,534 or 0xFFFF: 65,535 or higher */
          if (diff > 65534) {
            time_elapsed = 65535;
          } else {
            time_elapsed = (unsigned short)diff;
          }
          *((unsigned short *)pbuf) = htons(time_elapsed);
          pbuf += 2;
          pdmclients = pdmclients->ll.next;
        }
      }
      pdmbss = pdmbss->ll.next;
    }
    pdmif = pdmif->ll.next;
  }

  if (bss_count <= 0) {
    rc = 0;
    goto end;
  }

  /* Fill the BSS count now */
  *tmpBSSCountPtr = (unsigned char)bss_count;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvAssocaitedClientsType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

end:
  free(pmem);
  return (rc);
}

/* Extract clients associated to the BSS */
int i5TlvAssocaitedClientsTypeExtract(i5_message_type *pmsg, unsigned char *pdevid)
{
  unsigned char *pvalue;
  unsigned int length = 0;
  int rc = 0;
  i5_dm_device_type *pdevice;
  unsigned char  i;
  unsigned int pos = 0;
  unsigned char num_bss;

  pdevice = i5DmDeviceFind(pdevid);
  if ( pdevice == NULL ) {
    return -1;
  }

  i5MessageReset(pmsg);
  rc = i5MessageTlvExtract(pmsg, i5TlvAssocaitedClientsType, &length, &pvalue, i5MessageTlvExtractWithoutReset);
  if (rc != 0) {
    goto end;
  }

  if (length <= 0) {
    rc = -1;
    goto end;
  }

  num_bss = pvalue[pos];
  pos++;

  for (i = 0; i < num_bss; i++) {
    if (length >= pos + MAC_ADDR_LEN + 2) {
      unsigned char bssid[MAC_ADDR_LEN];
      unsigned short num_clients;
      unsigned char k;

      memcpy(bssid, &pvalue[pos], MAC_ADDR_LEN);
      pos += MAC_ADDR_LEN;
      num_clients = ntohs(*((unsigned short *)&pvalue[pos]));
      pos += 2;

      for (k = 0; k < num_clients; k++) {
        if (length >= pos + MAC_ADDR_LEN + 2) {
          unsigned char mac[MAC_ADDR_LEN];
          unsigned short time_elapsed;

          memcpy(mac, &pvalue[pos], MAC_ADDR_LEN);
          pos += MAC_ADDR_LEN;
          time_elapsed = ntohs(*((unsigned short *)&pvalue[pos]));
          pos += 2;
          i5DmClientUpdate(pmsg, pdevice, bssid, mac, time_elapsed);
        }
      }
    }
  }

end:
  /* This is an optional TLV */
  if (rc == -2) {
    rc = 0;
  }

  return rc;
}

/* TLV to indicate a 802.11 client joins or leaves a BSS */
int i5TlvClientAssociationEventTypeInsert(i5_message_type *pmsg, unsigned char *bssid, unsigned char *mac, unsigned char isAssoc)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvClientAssociationEvent_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;
  unsigned char assoc_event = 0x00;

  ptlv->type = i5TlvClientAssociationEventType;
  ptlv->length = htons(i5TlvClientAssociationEvent_Length);
  len += sizeof(i5_tlv_t);

  memcpy(&buf[len], mac, MAC_ADDR_LEN);
  len +=  MAC_ADDR_LEN;
  memcpy(&buf[len], bssid, MAC_ADDR_LEN);
  len +=  MAC_ADDR_LEN;

  if (isAssoc) {
    assoc_event |= I5_TLV_ASSOC_EVENT_JOIN;
  } else {
    assoc_event &= ~(I5_TLV_ASSOC_EVENT_JOIN);
  }
  buf[len] = (unsigned char)assoc_event;
  len++;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Client Association Event TLV */
int i5TlvClientAssociationEventTypeExtract(i5_message_type *pmsg, unsigned char *neighbor_al_mac_address)
{
  int rc = 0;
  unsigned char *pvalue;
  unsigned int length;
  unsigned char assoc_event = 0x00;
  unsigned char mac[MAC_ADDR_LEN], bssid[MAC_ADDR_LEN];
  i5_dm_device_type *pdmdev = i5DmDeviceFind(neighbor_al_mac_address);

  if (!pdmdev) {
    i5TraceError("neighbor device "I5_MAC_DELIM_FMT" not found\n",
      I5_MAC_PRM(neighbor_al_mac_address));
    goto end;
  }

  rc = i5MessageTlvExtract(pmsg, i5TlvClientAssociationEventType, &length, &pvalue, i5MessageTlvExtractWithReset);
  if (rc != 0) {
    goto end;
  }

  if (length < i5TlvClientAssociationEvent_Length) {
    rc = -1;
    goto end;
  }

  memcpy(mac, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;
  memcpy(bssid, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;
  assoc_event = pvalue[0];

  i5TraceInfo("neighbor_al_mac_address "I5_MAC_DELIM_FMT"  BSSID "I5_MAC_DELIM_FMT" MAC "I5_MAC_DELIM_FMT" assoc_event %x\n\n",
    I5_MAC_PRM(neighbor_al_mac_address), I5_MAC_PRM(bssid), I5_MAC_PRM(mac), assoc_event);
  if (assoc_event & I5_TLV_ASSOC_EVENT_JOIN) {
    i5DmAssociateClient(pmsg, pdmdev, bssid, mac, 0, 0, NULL, 0);
  } else {
    i5DmDisAssociateClient(pdmdev, bssid, mac, 0, 0);
  }

end:
  /* This is an optional TLV */
  if (rc == -2) {
    rc = 0;
  }

  return rc;
}

/* TLV to get the client info */
int i5TlvClientInfoTypeInsert(i5_message_type *pmsg, unsigned char *mac, unsigned char *bssid)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvClientInfo_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvClientInfoType;
  ptlv->length = htons(i5TlvClientInfo_Length);
  len += sizeof(i5_tlv_t);

  memcpy(&buf[len], bssid, MAC_ADDR_LEN);
  len +=  MAC_ADDR_LEN;
  memcpy(&buf[len], mac, MAC_ADDR_LEN);
  len +=  MAC_ADDR_LEN;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Client Info TLV */
int i5TlvClientInfoTypeExtract(i5_message_type *pmsg, unsigned char *mac, unsigned char *bssid)
{
  unsigned char *pvalue;
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvClientInfoType, &length, &pvalue, i5MessageTlvExtractWithReset) == 0) {
    if (length >= i5TlvClientInfo_Length) {
      memcpy(bssid, pvalue, MAC_ADDR_LEN);
      pvalue += MAC_ADDR_LEN;
      memcpy(mac, pvalue, MAC_ADDR_LEN);
      pvalue += MAC_ADDR_LEN;
      return 0;
    }
  }
  return -1;
}

/* TLV to report the client capability */
int i5TlvClientCapabilityReportTypeInsert(i5_message_type *pmsg, unsigned char result, unsigned char *frame, unsigned int frame_len)
{
  unsigned char *pbuf;
  i5_tlv_t *ptlv;
  int rc = 0, index = 0, len = 1 + frame_len;

  if ((pbuf = (unsigned char *)malloc(len + sizeof(i5_tlv_t))) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  ptlv = (i5_tlv_t *)pbuf;
  ptlv->type = i5TlvClientCapabilityReportType;
  ptlv->length = htons(len);
  index += sizeof(i5_tlv_t);

  pbuf[index] = result;
  index++;
  if (result == 0x00) {
    memcpy(&pbuf[index], frame, frame_len);
    index += frame_len;
  }

  rc = i5MessageInsertTlv(pmsg, pbuf, index);

  free(pbuf);
  return (rc);
}

/* Extract Client capability TLV. Free the frame variable once used */
int i5TlvClientCapabilityReportTypeExtract(i5_message_type *pmsg,
  unsigned char *neighbor_al_mac_address, unsigned char *mac, unsigned char *bssid,
  i5TlvClientCapabilityReporResultCode_Values_t *res)
{
  unsigned char *pvalue;
  unsigned int length;
  i5_dm_device_type *pdevice;
  i5_dm_bss_type *pbss;
  i5_dm_clients_type *pclient;

  pdevice = i5DmDeviceFind(neighbor_al_mac_address);
  if (pdevice == NULL) {
    i5TraceError("Received Client Capability Report Response From Unknown Device " I5_MAC_DELIM_FMT " \n",
      I5_MAC_PRM(neighbor_al_mac_address));
    goto end;
  }

  pbss = i5DmFindBSSFromDevice(pdevice, bssid);
  if (pbss == NULL) {
    goto end;
  }

  pclient  = i5DmFindClientInBSS(pbss, mac);
  if (pclient == NULL) {
    goto end;
  }

  if (i5MessageTlvExtract(pmsg, i5TlvClientCapabilityReportType, &length, &pvalue, i5MessageTlvExtractWithReset) == 0) {
    if (length >= 1) {
      *res = pvalue[0];
      pvalue++;
      if (*res == i5TlvClientCapabilityReporResultCode_Success) {
        i5DmUpdateClientCapability(pclient, pvalue, (length - 1));
      }
      return 0;
    }
  }

end:
  return -1;
}

/* TLV to report the AP basic Capabilities */
int i5TlvAPCapabilitiesTypeInsert(i5_message_type *pmsg, unsigned char caps)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvAPCapabilities_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvAPCapabilityType;
  ptlv->length = htons(i5TlvAPCapabilities_Length);
  len += sizeof(i5_tlv_t);

  buf[len] = caps;
  len++;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract AP basic Capabilities TLV */
int i5TlvAPCapabilitiesTypeExtract(i5_message_type *pmsg, i5_dm_device_type *pdevice)
{
  unsigned char *pvalue;
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvAPCapabilityType, &length, &pvalue, i5MessageTlvExtractWithReset) == 0) {
    if (length >= i5TlvAPCapabilities_Length) {
      pdevice->BasicCaps = *pvalue;
      pvalue++;
    } else {
      goto end;
    }
  } else {
    goto end;
  }

  return 0;

end:
  return -1;
}

#if defined(MULTIAPR2)
/* TLV to report the Channel Scan Capabilities */
int i5TlvChannelScanCapabilitiesTypeInsert(i5_message_type *pmsg)
{
  i5_dm_device_type *pdevice = i5_dm_network_topology.selfDevice;
  unsigned char *pbuf, *pmem, *tmpPtrLoc;
  i5_tlv_t *ptlv;
  int rc = 0;
  i5_dm_interface_type *pdmif;
  unsigned char num_of_radios = 0;

  if (pdevice == NULL) {
    i5TraceError("device does not exist\n");
    return -1;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }
  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  tmpPtrLoc = pbuf; /* Fill number of radios field later */
  pbuf++;

  foreach_i5glist_item(pdmif, i5_dm_interface_type, pdevice->interface_list) {

    /* Do below operation only for Wireless interfaces */
    if (!i5DmIsInterfaceWireless(pdmif->MediaType)) {
      continue;
    }

    /* Radio Unique Identifier of a radio of the Multi-AP Agent */
    memcpy(pbuf, pdmif->InterfaceId, MAC_ADDR_LEN);
    pbuf += MAC_ADDR_LEN;

    /* Channel Scan Capabilities Flags */
    *pbuf = pdmif->ApCaps.ChScanCaps.chscan_cap_flag;
    pbuf++;

    /* Minimum Scan Interval */
    *((unsigned int *)pbuf) = htonl(pdmif->ApCaps.ChScanCaps.min_scan_interval);
    pbuf += 4;

    memcpy(pbuf, pdmif->ApCaps.ChScanCaps.List, pdmif->ApCaps.ChScanCaps.Len);
    pbuf += pdmif->ApCaps.ChScanCaps.Len;

    num_of_radios++;
  }

  /* Now fill the number of radios field */
  *tmpPtrLoc = (unsigned char)num_of_radios;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvChannelScanCapabilitiesType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract radio Channel Scan Capabilities */
static void RadioChannelScanCapabilitiesTypeExtract(unsigned char pvalue[], int *num_octets_extracted,
	ieee1905_channel_scan_caps_type *ChScanCaps)
{
  unsigned int pos = 0, idx = 0, idx_rclass = 0;
  unsigned char rclass_count = 0, chan_count = 0;

  /* Extract Channel Scan Capabilities Flags */
  ChScanCaps->chscan_cap_flag = pvalue[pos];
  pos += 1;

  /* Extract Minimum Scan Interval */
  ChScanCaps->min_scan_interval = ntohl(*((unsigned int *)&(pvalue[pos])));
  pos += 4;

  /* Extract Number of Operating Classes */
  rclass_count = pvalue[pos];
  pos += 1;
  ChScanCaps->List[idx++] = rclass_count; /* 0th element : Number of Operating Classes */

  /* Extract details for each Operating Class */
  for (idx_rclass = 0; idx_rclass < rclass_count; idx_rclass++) {

    chan_count = 0;

    /* Check for Operating Class Octect */
    if (ChScanCaps->ListSize < idx + 1) {
      i5TraceError("Operating Class field is not present.\n");
      goto error;
    }

    /* Extract Operating Class Value */
    ChScanCaps->List[idx++] = pvalue[pos];
    pos += 1;

    /* Extract Number of Channels specified in the Channel List */
    chan_count = pvalue[pos];
    pos += 1;

    /* Check for Number of Channel Octect + Channels Octects */
    if (ChScanCaps->ListSize < (idx + 1 + chan_count)) {
      i5TraceError("Either Number_of_Channel or Channels field is not present.\n");
      goto error;
    }
    ChScanCaps->List[idx++] = chan_count;

    /* Extract Channel List : Each octet describes a single channel number in the Operating Class
        * on which the Agent is capable of performing a scan. An empty Channel List field (k=0)
        * indicates that the Agent is capable of scanning on all channels in the Operating Class.
        */
    memcpy(&(ChScanCaps->List[idx]), &(pvalue[pos]), chan_count);
    pos += chan_count;
    idx += chan_count;

  }

  ChScanCaps->Valid = 1;
  ChScanCaps->Len = idx;

  *num_octets_extracted = pos;
  i5TraceInfo("Success : Extract Channel Scan capabilities TLV\n");
  return;

error:
  i5TraceError("Failed : Extract Channel Scan capabilities TLV\n");
  ChScanCaps->Valid = 0;

  *num_octets_extracted = pos;
  return;
}

/* Extract Channel Scan Capabilities TLV */
int i5TlvChannelScanCapabilitiesTypeExtract(i5_message_type *pmsg)
{
  i5_dm_device_type *pdevice;
  i5_dm_interface_type *pdmif;
  unsigned char *pvalue, mac[MAC_ADDR_LEN];
  unsigned int length, num_of_radios = 0, idx_radio = 0;
  int rc = 0, num_octets_extracted = 0;

  pdevice = i5DmDeviceFind(i5MessageSrcMacAddressGet(pmsg));
  if (pdevice == NULL) {
    i5TraceError("Neighbour device does not exist\n");
    rc = -1;
    goto end;
  }

  rc = i5MessageTlvExtract(pmsg, i5TlvChannelScanCapabilitiesType, &length, &pvalue, i5MessageTlvExtractWithReset);

  /* Validate the length */
  if (rc != 0) {
    return rc;
  }

  /* If TLV size is not greate or equal to 1, this TLV is corrupted */
  if (length < 1) {
    i5TraceError("Channel Scan Capabilities TLV Length is not proper.\n");
    rc = -1;
    goto end;
  }

  /* Extract Total Number of radios */
  num_of_radios = *pvalue;
  pvalue++;
  if (num_of_radios <= 0) {
    rc = 0;
    goto end;
  }

  /* Extract details for each Radio */
  for (idx_radio = 0; idx_radio < num_of_radios; idx_radio++) {

    num_octets_extracted = 0;

    /* Extract Radio Unique Identifier of a radio of the Multi-AP Agent */
    memcpy(mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;

    /* Check if Radio is available in Multi-AP Agent topology */
    pdmif = i5DmInterfaceFind(pdevice, mac);
    if (pdmif == NULL) {
      i5TraceError("Neighbour Interface " I5_MAC_DELIM_FMT " does not exist\n", I5_MAC_PRM(mac));
      continue;
    }

    /* If ChannelScan caps list is not present, allocate new */
    if (pdmif->ApCaps.ChScanCaps.List != NULL) {
      memset(pdmif->ApCaps.ChScanCaps.List, 0, I5_CHSCAN_CAP_SIZE);
    } else {
      pdmif->ApCaps.ChScanCaps.List = (unsigned char*)malloc(I5_CHSCAN_CAP_SIZE);

      if (pdmif->ApCaps.ChScanCaps.List != NULL) {
          memset(pdmif->ApCaps.ChScanCaps.List, 0, I5_CHSCAN_CAP_SIZE);
          pdmif->ApCaps.ChScanCaps.ListSize = I5_CHSCAN_CAP_SIZE;
      } else {
        i5TraceDirPrint("Malloc failed for  " I5_MAC_DELIM_FMT " ChannelScan Caps List\n", I5_MAC_PRM(mac));
        rc = -1;
        goto end;
      }
    }

    /* Extract radio specific Channel Scan Capabilities */
    RadioChannelScanCapabilitiesTypeExtract(pvalue, &num_octets_extracted,
      &pdmif->ApCaps.ChScanCaps);

    /* If radio specific Extraction not succeeded, free ChannelScan caps list of raio and Leave */
    if (pdmif->ApCaps.ChScanCaps.List && !pdmif->ApCaps.ChScanCaps.Valid) {

      i5TraceDirPrint("Failed to extract Channel Scan capabilities TLV "
        "Removing ChScanCaps for " I5_MAC_DELIM_FMT " \n", I5_MAC_PRM(mac));

      free(pdmif->ApCaps.ChScanCaps.List);
      memset(&pdmif->ApCaps.ChScanCaps, 0, sizeof(pdmif->ApCaps.ChScanCaps));

      rc = -1;
      goto end;

    /* If radio specific Extraction succeeded, move ptr ahead and go for next radio */
    } else {

      pvalue += num_octets_extracted;
    }

  }

  return 0;

end:

  return rc;
}
#endif /* MULTIAPR2 */

/* TLV to report the AP Radios basic Capabilities */
int i5TlvAPRadioBasicCapabilitiesTypeInsert(i5_message_type *pmsg, unsigned char *mac,
  unsigned char bssCount, unsigned char *data, unsigned char len)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end
  memcpy(pbuf, mac, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  *pbuf = bssCount;
  pbuf++;

  memcpy(pbuf, data, len);
  pbuf += len;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvAPRadioBasicCapabilitiesType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract radio basic capabilities */
static void RadioBasicCapabilitiesTypeExtract(unsigned char *pvalue, ieee1905_radio_caps_type *RadioCaps)
{
      unsigned int idx = 0;
      unsigned char count = 0;

      RadioCaps->maxBSSSupported = *pvalue;
      pvalue++;
      count = *pvalue;
      pvalue++;
      RadioCaps->List[idx++] = count; /* 0th elem count of rclass */
      while (count > 0) {
        unsigned char chan_count = 0;
        unsigned int jdx = 0;

        /* rclass and max tx pwr */
        if (RadioCaps->ListSize < idx + 2) { /* For rclass and max tx power */
          goto end;
        }
        RadioCaps->List[idx++] = *pvalue;
        pvalue++;
        RadioCaps->List[idx++] = *pvalue;
        pvalue++;
        /* Unsupported chan count */
        chan_count = *pvalue;
        pvalue++;
        /* Chan list */
        if (RadioCaps->ListSize < (idx + 1 + chan_count)) {
          goto end;
        }
        RadioCaps->List[idx++] = chan_count;
        for (jdx = 0; jdx < chan_count; jdx++) {
          RadioCaps->List[idx++] = *pvalue;
          pvalue++;
        }
        count--;
      }

      RadioCaps->Valid = 1;
      RadioCaps->Len = idx;

      return;
end:
      i5TraceInfo("Failed to extract Radio basic capabilities\n");
      RadioCaps->Valid = 0;
      return;
}

/* Extract AP Radios basic Capabilities TLV */
int i5TlvAPRadioBasicCapabilitiesTypeExtract(i5_message_type *pmsg, i5_dm_device_type *pdevice)
{
  i5_dm_interface_type *pdmif;
  unsigned char *pvalue, mac[MAC_ADDR_LEN];
  unsigned int length, found = 0;
  int rc = 0;

  i5MessageReset(pmsg);
  while ((rc = i5MessageTlvExtract(pmsg, i5TlvAPRadioBasicCapabilitiesType,
    &length, &pvalue, i5MessageTlvExtractWithoutReset)) == 0) {
    found = 1;
    if (length >= MAC_ADDR_LEN + 2) {
      memset(mac, 0, MAC_ADDR_LEN);
      memcpy(mac, pvalue, MAC_ADDR_LEN);
      pvalue += MAC_ADDR_LEN;
      pdmif = i5DmInterfaceFind(pdevice, mac);
      if (pdmif == NULL) {
        i5TraceError("Neighbour Interface " I5_MAC_DELIM_FMT " does not exist\n", I5_MAC_PRM(mac));
        continue;
      }

      /* If Radio caps list is present reset it otherwise allocate and fill. */
      if (pdmif->ApCaps.RadioCaps.List != NULL) {
        memset(pdmif->ApCaps.RadioCaps.List, 0, I5_RADIO_CAP_SIZE);
      } else {
        pdmif->ApCaps.RadioCaps.List = (unsigned char*)malloc(I5_RADIO_CAP_SIZE);
        if (pdmif->ApCaps.RadioCaps.List != NULL) {
          memset(pdmif->ApCaps.RadioCaps.List, 0, I5_RADIO_CAP_SIZE);
          pdmif->ApCaps.RadioCaps.ListSize = I5_RADIO_CAP_SIZE;
        } else {
          i5TraceDirPrint("Malloc failed for  " I5_MAC_DELIM_FMT " Radio Caps List\n", I5_MAC_PRM(mac));
          rc = -1;
          goto end;
        }
      }

      RadioBasicCapabilitiesTypeExtract(pvalue, &pdmif->ApCaps.RadioCaps);
      pdmif->band = ieee1905_get_band_from_radiocaps(&pdmif->ApCaps.RadioCaps);
    } else {
      rc = -1;
      goto end;
    }
  }

  if (!found) {
    rc = -1;
    goto end;
  }

  return 0;

end:

  return rc;
}

/* Extract AP Radios basic Capabilities TLV got in WSC M1 message */
int i5TlvAPRadioBasicCapabilitiesTypeExtractFromWSCM1(i5_message_type *pmsg, unsigned char *outMac,
  ieee1905_radio_caps_type *RadioCaps)
{
  unsigned char *pvalue, mac[MAC_ADDR_LEN];
  unsigned int length;
  int rc = 0;

  if (RadioCaps == NULL || outMac == NULL) {
    i5TraceError("Invalid Arguments\n");
    rc = -1;
    goto end;
  }

  if ((rc = i5MessageTlvExtract(pmsg, i5TlvAPRadioBasicCapabilitiesType,
    &length, &pvalue, i5MessageTlvExtractWithReset)) != 0) {
    goto end;
  }

  if (length < (MAC_ADDR_LEN + 2)) {
    i5TraceError("length[%d] < %d\n", length, (MAC_ADDR_LEN + 2));
    rc = -1;
    goto end;
  }

  memcpy(mac, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;

  /* If Radio caps list is present reset it otherwise allocate and fill. */
  if (RadioCaps->List != NULL) {
    memset(RadioCaps->List, 0, I5_RADIO_CAP_SIZE);
  } else {
    RadioCaps->List = (unsigned char*)malloc(I5_RADIO_CAP_SIZE);
    if (RadioCaps->List != NULL) {
      memset(RadioCaps->List, 0, I5_RADIO_CAP_SIZE);
      RadioCaps->ListSize = I5_RADIO_CAP_SIZE;
    } else {
      i5TraceDirPrint("Malloc failed for  " I5_MAC_DELIM_FMT " Radio Caps List\n", I5_MAC_PRM(mac));
      rc = -1;
      goto end;
    }
  }

  RadioBasicCapabilitiesTypeExtract(pvalue, RadioCaps);

  if (NULL != outMac) {
     memcpy(outMac, mac, MAC_ADDR_LEN);
  }

  return 0;

end:

  return rc;
}

int i5TlvAPHTCapabilitiesTypeInsert(i5_message_type *pmsg, unsigned char *mac, unsigned char caps)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvAPHTCapabilities_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvAPHTCapabilitiesType;
  ptlv->length = htons(i5TlvAPHTCapabilities_Length);
  len += sizeof(i5_tlv_t);

  memcpy(&buf[len], mac, MAC_ADDR_LEN);
  len += MAC_ADDR_LEN;
  buf[len] = caps;
  len++;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract AP HT Capabilities TLV */
int i5TlvAPHTCapabilitiesTypeExtract(i5_message_type *pmsg, i5_dm_device_type *pdevice)
{
  i5_dm_interface_type *pdmif;
  unsigned char *pvalue, mac[MAC_ADDR_LEN];
  unsigned int length;
  int rc = 0;

  i5MessageReset(pmsg);
  while ((rc = i5MessageTlvExtract(pmsg, i5TlvAPHTCapabilitiesType,
    &length, &pvalue, i5MessageTlvExtractWithoutReset)) == 0) {
    if (length >= i5TlvAPHTCapabilities_Length) {
      memset(mac, 0, MAC_ADDR_LEN);
      memcpy(mac, pvalue, MAC_ADDR_LEN);
      pvalue += MAC_ADDR_LEN;
      pdmif = i5DmInterfaceFind(pdevice, mac);
      if (pdmif == NULL) {
        i5TraceError("Neighbour Interface " I5_MAC_DELIM_FMT " does not exist\n", I5_MAC_PRM(mac));
        continue;
      }
      pdmif->ApCaps.HTCaps = *pvalue;
      pvalue++;
    } else {
      rc = -1;
      goto end;
     }
  }

end:
  if (rc == -2) {
    rc = 0;
  }

  return rc;
}

/* TLV to report the AP VHT Capabilities */
int i5TlvAPVHTCapabilitiesTypeInsert(i5_message_type *pmsg, unsigned char *mac,
   unsigned short txMCSMap, unsigned short rxMCSMap, unsigned char capsEx, unsigned char caps)
{

  unsigned char buf[sizeof(i5_tlv_t) + i5TlvAPVHTCapabilities_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvAPVHTCapabilitiesType;
  ptlv->length = htons(i5TlvAPVHTCapabilities_Length);
  len += sizeof(i5_tlv_t);

  memcpy(&buf[len], mac, MAC_ADDR_LEN);
  len += MAC_ADDR_LEN;
  *((unsigned short *)&buf[len]) = htons(txMCSMap);
  len += 2;
  *((unsigned short *)&buf[len]) = htons(rxMCSMap);
  len += 2;
  buf[len] = capsEx;
  len++;
  buf[len] = caps;
  len++;
  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract AP VHT Capabilities TLV */
int i5TlvAPVHTCapabilitiesTypeExtract(i5_message_type *pmsg, i5_dm_device_type *pdevice)
{
  i5_dm_interface_type *pdmif;
  unsigned char *pvalue, mac[MAC_ADDR_LEN];
  unsigned int length;
  int rc = 0;

  i5MessageReset(pmsg);
  while((rc = i5MessageTlvExtract(pmsg, i5TlvAPVHTCapabilitiesType,
    &length, &pvalue, i5MessageTlvExtractWithoutReset)) == 0) {
    if (length >= i5TlvAPVHTCapabilities_Length) {
      memset(mac, 0, MAC_ADDR_LEN);
      memcpy(mac, pvalue, MAC_ADDR_LEN);
      pvalue += MAC_ADDR_LEN;
      pdmif = i5DmInterfaceFind(pdevice, mac);
      if (pdmif == NULL) {
        i5TraceError("Neighbour bss " I5_MAC_DELIM_FMT " does not exist\n", I5_MAC_PRM(mac));
        continue;
      }
      pdmif->ApCaps.VHTCaps.TxMCSMap = ntohs(*((unsigned short *)pvalue));
      pvalue += 2;
      pdmif->ApCaps.VHTCaps.RxMCSMap = ntohs(*((unsigned short *)pvalue));
      pvalue += 2;
      pdmif->ApCaps.VHTCaps.CapsEx = *pvalue;
      pvalue++;
      pdmif->ApCaps.VHTCaps.Caps = *pvalue;
      pvalue++;
      pdmif->ApCaps.VHTCaps.Valid = 1;
    } else {
      rc = -1;
      goto end;
    }
  }

end:
  if (rc == -2) {
    rc = 0;
  }

  return rc;
}

/* TLV to report the AP HE Capabilities */
int i5TlvAPHECapabilitiesTypeInsert(i5_message_type *pmsg, unsigned char *mac, ieee1905_he_caps_type* HECaps)
{
  unsigned char *pbuf, *pmem, *mcslen;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end
  memcpy(pbuf, mac, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  mcslen = pbuf;
  *mcslen = 0;
  pbuf++;

  /* Add mcs map of 80MHZ */
  *((unsigned short *)pbuf) = htons(HECaps->TxBW80MCSMap);
  pbuf += 2;
  *((unsigned short *)pbuf) = htons(HECaps->RxBW80MCSMap);
  pbuf += 2;

  /* Add mcs map of 160 MHz */
  if ((HECaps->TxBW160MCSMap != 0xffff) || (HECaps->RxBW160MCSMap != 0xffff)) {
    *((unsigned short *)pbuf) = htons(HECaps->TxBW160MCSMap);
    pbuf += 2;
    *((unsigned short *)pbuf) = htons(HECaps->RxBW160MCSMap);
    pbuf += 2;
  }

  /* Add mcs map of 80p80 MHz */
  if ((HECaps->TxBW80p80MCSMap != 0xffff) || (HECaps->RxBW80p80MCSMap != 0xffff)) {
    *((unsigned short *)pbuf) = htons(HECaps->TxBW80p80MCSMap);
    pbuf += 2;
    *((unsigned short *)pbuf) = htons(HECaps->RxBW80p80MCSMap);
    pbuf += 2;
  }
  *mcslen = pbuf - mcslen - 1;

  *pbuf = HECaps->CapsEx;
  pbuf++;
  *pbuf = HECaps->Caps;
  pbuf++;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvAPHECapabilitiesType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract AP HE Capabilities TLV */
int i5TlvAPHECapabilitiesTypeExtract(i5_message_type *pmsg, i5_dm_device_type *pdevice)
{
  i5_dm_interface_type *pdmif;
  unsigned char *pvalue, mac[MAC_ADDR_LEN], hemcslen;
  unsigned int length;
  unsigned char *hemcsmap;
  int rc = 0;

  i5MessageReset(pmsg);
  while((rc = i5MessageTlvExtract(pmsg, i5TlvAPHECapabilitiesType,
    &length, &pvalue, i5MessageTlvExtractWithoutReset)) == 0) {
    if (length >= i5TlvAPHECapabilities_Min_Length) {
      memset(mac, 0, MAC_ADDR_LEN);
      memcpy(mac, pvalue, MAC_ADDR_LEN);
      pvalue += MAC_ADDR_LEN;
      pdmif = i5DmInterfaceFind(pdevice, mac);
      if (pdmif == NULL) {
        i5TraceError("Neighbour bss " I5_MAC_DELIM_FMT " does not exist\n", I5_MAC_PRM(mac));
        continue;
      }
      hemcslen = *pvalue;
      pvalue++;
      hemcsmap = pvalue;

      /* Extact mcs after getting caps */
      pvalue += hemcslen;
      pdmif->ApCaps.HECaps.CapsEx = *pvalue;
      pvalue++;
      pdmif->ApCaps.HECaps.Caps = *pvalue;
      pvalue++;

      /* Extract 80Mhz mcs map */
      if (hemcslen >= 4) {
        pdmif->ApCaps.HECaps.TxBW80MCSMap = ntohs(*((unsigned short *)hemcsmap));
        hemcsmap += 2;
        pdmif->ApCaps.HECaps.RxBW80MCSMap = ntohs(*((unsigned short *)hemcsmap));
        hemcsmap += 2;
	hemcslen -= 4;
      }

      /* Extract 160 Mhz mcs map */
      if ((hemcslen >= 4) && (pdmif->ApCaps.HECaps.CapsEx & IEEE1905_AP_HECAP_160MHZ)) {
        pdmif->ApCaps.HECaps.TxBW160MCSMap = ntohs(*((unsigned short *)hemcsmap));
        hemcsmap += 2;
        pdmif->ApCaps.HECaps.RxBW160MCSMap = ntohs(*((unsigned short *)hemcsmap));
        hemcsmap += 2;
	hemcslen -= 4;
      }

      /* Extract 80p80 mcs map */
      if ((hemcslen >= 4) && (pdmif->ApCaps.HECaps.CapsEx & IEEE1905_AP_HECAP_80P80MHZ)) {
        pdmif->ApCaps.HECaps.TxBW80p80MCSMap = ntohs(*((unsigned short *)hemcsmap));
        hemcsmap += 2;
        pdmif->ApCaps.HECaps.RxBW80p80MCSMap = ntohs(*((unsigned short *)hemcsmap));
        hemcsmap += 2;
	hemcslen -= 4;
      }

      pdmif->ApCaps.HECaps.Valid = 1;
    } else {
      rc = -1;
      goto end;
    }
  }

end:
  if (rc == -2) {
    rc = 0;
  }

  return rc;

}

/* TLV to insert steering policy */
int i5TlvSteeringPolicyTypeInsert(i5_message_type *pmsg, unsigned char *neighbor_al_mac_address,
  ieee1905_policy_config *configlist)
{
  i5_tlv_t *ptlv;
  unsigned char *pbuf, *pmem, *tmpPtrLoc;
  unsigned char num_of_radios = 0;
  int rc = 0;
  dll_t *item_p, *next_p;
  ieee1905_sta_list *staInfo;
  i5_dm_device_type *pDeviceNeighbor;

  if ((pDeviceNeighbor = i5DmDeviceFind(neighbor_al_mac_address)) == NULL) {
    i5TraceError("Neighbor Device "I5_MAC_FMT" Not Found\n", I5_MAC_PRM(neighbor_al_mac_address));
    return -1;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  /* Fill local steering disallowed sta's */
  *pbuf = (unsigned char)configlist->no_steer_sta_list.count;
  pbuf++;

  /* Steer disallowed sta list. */
  for (item_p = dll_head_p(&configlist->no_steer_sta_list.head);
    !dll_end(&configlist->no_steer_sta_list.head, item_p);
    item_p = next_p) {
    next_p = dll_next_p(item_p);
    staInfo = (ieee1905_sta_list*)item_p;
    memcpy(pbuf, staInfo->mac, MAC_ADDR_LEN);
    pbuf += MAC_ADDR_LEN;
  }

  /* Fill BTM steering disallowed sta's */
  *pbuf = (unsigned char)configlist->no_btm_steer_sta_list.count;
  pbuf++;

  /* BTM Steer disallowed sta list. */
  for (item_p = dll_head_p(&configlist->no_btm_steer_sta_list.head);
    !dll_end(&configlist->no_btm_steer_sta_list.head, item_p);
    item_p = next_p) {
    next_p = dll_next_p(item_p);
    staInfo = (ieee1905_sta_list*)item_p;
    memcpy(pbuf, staInfo->mac, MAC_ADDR_LEN);
    pbuf += MAC_ADDR_LEN;
  }

  tmpPtrLoc = pbuf; /* Fill number of radios field later */
  pbuf++;

  /* BSS radio id list. */
  for (item_p = dll_head_p(&configlist->steercfg_bss_list.head);
    !dll_end(&configlist->steercfg_bss_list.head, item_p);
    item_p = next_p) {
    ieee1905_bss_steer_config *bss;
    next_p = dll_next_p(item_p);
    bss = (ieee1905_bss_steer_config*)item_p;

    /* Send only for its interfaces */
    if (i5DmInterfaceFind(pDeviceNeighbor, bss->mac)) {
      memcpy(pbuf, bss->mac, MAC_ADDR_LEN);
      pbuf += MAC_ADDR_LEN;
      *pbuf = bss->policy;
      pbuf++;
      *pbuf = bss->bssload_thld;
      pbuf++;
      *pbuf = bss->rssi_thld;
      pbuf++;
      num_of_radios++;
    }
  }
  /* Now fill the number of radios field */
  *tmpPtrLoc = (unsigned char)num_of_radios;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvSteeringPolicyType;
  ptlv->length = htons(pbuf - pmem - sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* TLV to extract steering policy */
int i5TlvSteeringPolicyTypeExtract(i5_message_type *pmsg)
{
  unsigned char *pvalue;
  unsigned int length, count;
  int rc = 0, idx = 0;
  ieee1905_sta_list *sta_info;

  rc = i5MessageTlvExtract(pmsg, i5TlvSteeringPolicyType, &length, &pvalue, i5MessageTlvExtractWithReset);
  if (rc < 0) {
    goto end;
  }

  if (length < i5TlvSteeringPolicyType_Min_Length) {
    goto end;
  }
  /* Clean up before filling the latest data. */
  i5DmGlistCleanup(&i5_config.policyConfig.no_steer_sta_list);
  ieee1905_glist_init(&i5_config.policyConfig.no_steer_sta_list);

  count = *pvalue;
  pvalue++;
  for (idx = 0; idx < count; idx++) {
    sta_info = (ieee1905_sta_list*)malloc(sizeof(*sta_info));
    if (!sta_info) {
      i5TraceDirPrint("malloc error\n");
      goto end;
    }

    memcpy(sta_info->mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;
    ieee1905_glist_append(&i5_config.policyConfig.no_steer_sta_list, (dll_t*)sta_info);
  }

  /* Clean up before filling the latest data. */
  i5DmGlistCleanup(&i5_config.policyConfig.no_btm_steer_sta_list);
  ieee1905_glist_init(&i5_config.policyConfig.no_btm_steer_sta_list);

  count = *pvalue;
  pvalue++;
  for (idx = 0; idx < count; idx++) {
    sta_info = (ieee1905_sta_list*)malloc(sizeof(*sta_info));
    if (!sta_info) {
      i5TraceDirPrint("malloc error\n");
      goto end;
    }

    memcpy(sta_info->mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;
    ieee1905_glist_append(&i5_config.policyConfig.no_btm_steer_sta_list, (dll_t*)sta_info);
  }

  /* Clean up before filling the latest data. */
  i5DmGlistCleanup(&i5_config.policyConfig.steercfg_bss_list);
  ieee1905_glist_init(&i5_config.policyConfig.steercfg_bss_list);

  count = *pvalue;
  pvalue++;
  for (idx = 0; idx < count; idx++) {
    ieee1905_bss_steer_config *bss = (ieee1905_bss_steer_config*)malloc(sizeof(*bss));
    if (!bss) {
      i5TraceDirPrint("malloc error\n");
      goto end;
    }

    memcpy(bss->mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;
    bss->policy = *pvalue;
    pvalue++;
    bss->bssload_thld = *pvalue;
    pvalue++;
    bss->rssi_thld = *pvalue;
    pvalue++;
    ieee1905_glist_append(&i5_config.policyConfig.steercfg_bss_list, (dll_t*)bss);
  }

  return 0;

end:
  return -1;
}

/* TLV to insert metric reporting policy */
int i5TlvMetricReportingPolicyTypeInsert(i5_message_type *pmsg, unsigned char *neighbor_al_mac_address,
  ieee1905_policy_config *configlist)
{
  unsigned char *pbuf, *pmem, *tmpPtrLoc;
  unsigned char num_of_radios = 0;
  i5_tlv_t *ptlv;
  int rc = 0;
  dll_t *item_p, *next_p;
  i5_dm_device_type *pDeviceNeighbor;
  i5_dm_interface_type *pInterface;

  if ((pDeviceNeighbor = i5DmDeviceFind(neighbor_al_mac_address)) == NULL) {
    i5TraceError("Neighbor Device "I5_MAC_FMT" Not Found\n", I5_MAC_PRM(neighbor_al_mac_address));
    return -1;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  *pbuf = configlist->metricrpt_config.ap_rpt_intvl;
  pbuf++;

  tmpPtrLoc = pbuf; /* Fill number of radios field later */
  pbuf++;

  /* BSS radio id list. */
  for (item_p = dll_head_p(&configlist->metricrpt_config.ifr_list.head);
    !dll_end(&configlist->metricrpt_config.ifr_list.head, item_p);
    item_p = next_p) {
    ieee1905_ifr_metricrpt *bss;
    next_p = dll_next_p(item_p);
    bss = (ieee1905_ifr_metricrpt*)item_p;

    /* Send only for its interfaces */
    if ((pInterface = i5DmInterfaceFind(pDeviceNeighbor, bss->mac)) != NULL) {
      if (!pInterface->isConfigured) {
        continue;
      }
      memcpy(pbuf, bss->mac, MAC_ADDR_LEN);
      pbuf += MAC_ADDR_LEN;
      *pbuf = bss->sta_mtrc_rssi_thld;
      pbuf++;
      *pbuf = bss->sta_mtrc_rssi_hyst;
      pbuf++;
      *pbuf = bss->ap_mtrc_chan_util;
      pbuf++;
      *pbuf = bss->sta_mtrc_policy_flag;
      pbuf++;
      num_of_radios++;
    }
  }
  /* Now fill the number of radios field */
  *tmpPtrLoc = (unsigned char)num_of_radios;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvMetricReportingPolicyType;
  ptlv->length = htons(pbuf - pmem - sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* TLV to extract metric reporting policy */
int i5TlvMetricReportingPolicyTypeExtract(i5_message_type *pmsg)
{
  unsigned char *pvalue;
  unsigned int length, count;
  int rc = 0, idx = 0, index = 0;

  rc = i5MessageTlvExtract(pmsg, i5TlvMetricReportingPolicyType, &length, &pvalue, i5MessageTlvExtractWithReset);
  if (rc < 0) {
    goto end;
  }

  if (length < i5TlvMetricReportingPolicyType_Min_Length) {
    goto end;
  }

  /* Clean up bss list before update. */
  i5DmGlistCleanup(&i5_config.policyConfig.metricrpt_config.ifr_list);
  ieee1905_glist_init(&i5_config.policyConfig.metricrpt_config.ifr_list);

  i5_config.policyConfig.metricrpt_config.ap_rpt_intvl = *pvalue;
  pvalue++;

  count = *pvalue;
  pvalue++;
  index += i5TlvMetricReportingPolicyType_Min_Length;
  for (idx = 0; idx < count && length >= (index + i5TlvMetricReportingPolicyInterface_Min_Length);
    idx++) {
    ieee1905_ifr_metricrpt *bss = (ieee1905_ifr_metricrpt*)malloc(sizeof(*bss));
    if (!bss) {
      i5TraceDirPrint("malloc error\n");
      goto end;
    }

    memcpy(bss->mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;
    bss->sta_mtrc_rssi_thld = *pvalue;
    pvalue++;
    bss->sta_mtrc_rssi_hyst = *pvalue;
    pvalue++;
    bss->ap_mtrc_chan_util = *pvalue;
    pvalue++;
    bss->sta_mtrc_policy_flag = *pvalue;
    pvalue++;
    index += i5TlvMetricReportingPolicyInterface_Min_Length;

    ieee1905_glist_append(&i5_config.policyConfig.metricrpt_config.ifr_list, (dll_t*)bss);
  }

  return 0;

end:
  return -1;
}

#if defined(MULTIAPR2)
/* TLV to insert Channel Scan Reporting Policy */
int i5TlvChannelScanReportingPolicyTypeInsert(i5_message_type *pmsg, unsigned char *neighbor_al_mac_address,
  ieee1905_policy_config *configlist)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvChannelScanReportingPolicyType_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvChannelScanReportingPolicyType;
  ptlv->length = htons(i5TlvChannelScanReportingPolicyType_Length);
  len += sizeof(i5_tlv_t);

  buf[len] = configlist->chscanrpt_config.chscan_rpt_policy_flag;
  len++;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* TLV to extract Channel Scan Reporting Policy */
int i5TlvChannelScanReportingPolicyTypeExtract(i5_message_type *pmsg)
{
  unsigned char *pvalue;
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvChannelScanReportingPolicyType, &length, &pvalue, i5MessageTlvExtractWithReset) == 0) {
    if (length >= i5TlvChannelScanReportingPolicyType_Length) {
      i5_config.policyConfig.chscanrpt_config.chscan_rpt_policy_flag = *pvalue;
      pvalue++;
    } else {
      goto error;
    }
  } else {
    goto error;
  }

  i5TraceInfo("Success : Extract Channel Scan Reporting Policy TLV\n");
  return 0;

error:
  i5TraceError("Failed : Extract Channel Scan Reporting Policy TLV\n");
  return -1;
}
#endif /* MULTIAPR2 */

/* TLV to insert the Client Steering Request */
int i5TlvSteeringRequestTypeInsert(i5_message_type *pmsg, ieee1905_steer_req *steer_req)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;
  dll_t *item_p, *next_p;
  ieee1905_sta_list *staInfo;
  ieee1905_bss_list *bssInfo;
  unsigned char req_flag = 0x00;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  /* BSSID */
  memcpy(pbuf, steer_req->source_bssid, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  /* Request Mode */
  if (IEEE1905_IS_STEER_MANDATE(steer_req->request_flags)) {
    req_flag |= I5_TLV_STEER_REQUEST_MODE;
  }
  if (IEEE1905_IS_DISASSOC_IMNT(steer_req->request_flags)) {
    req_flag |= I5_TLV_STEER_DISASSOC_IMNT;
  }
  if (IEEE1905_IS_BTM_ABRIDGED(steer_req->request_flags)) {
    req_flag |= I5_TLV_STEER_BTM_ABRIDGED;
  }
  *pbuf = (unsigned char)req_flag;
  pbuf++;

  *((unsigned short *)pbuf) = htons(steer_req->opportunity_window);
  pbuf += 2;

  /* BTM Disassociation Timer */
  *((unsigned short *)pbuf) = htons(steer_req->dissassociation_timer);
  pbuf += 2;

  /* STA List Count */
  *pbuf = (unsigned char)steer_req->sta_list.count;
  pbuf++;

  /* STA MAC address */
  for (item_p = dll_head_p(&steer_req->sta_list.head);
    !dll_end(&steer_req->sta_list.head, item_p);
    item_p = next_p) {
    next_p = dll_next_p(item_p);
    staInfo = (ieee1905_sta_list*)item_p;
    memcpy(pbuf, staInfo->mac, MAC_ADDR_LEN);
    pbuf += MAC_ADDR_LEN;
  }

  /* If request mode is 1(Means Steering Mandate) include Target BSSID List Count */
  if (IEEE1905_IS_STEER_MANDATE(steer_req->request_flags)) {
    *pbuf = (unsigned char)steer_req->bss_list.count;
    pbuf++;

    for (item_p = dll_head_p(&steer_req->bss_list.head);
      !dll_end(&steer_req->bss_list.head, item_p);
      item_p = next_p) {
      next_p = dll_next_p(item_p);
      bssInfo = (ieee1905_bss_list*)item_p;

      /* Target BSSID */
      memcpy(pbuf, bssInfo->bssid, MAC_ADDR_LEN);
      pbuf += MAC_ADDR_LEN;

      /* Target BSS Operating Class */
      *pbuf = bssInfo->target_op_class;
      pbuf++;

      /* Target BSS Channel Number */
      *pbuf = bssInfo->target_channel;
      pbuf++;
    }
  } else {
    *pbuf = (unsigned char)0;
    pbuf++;
  }

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvSteeringRequestType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Client Steering Request TLV */
int i5TlvSteeringRequestTypeExtract(i5_message_type *pmsg, ieee1905_steer_req *steer_req)
{
  unsigned char *pvalue;
  unsigned int length, extracted_len = i5TlvSteeringRequest_Min_Length;
  int rc = 0, i;
  unsigned char sta_count = 0, bss_count = 0;
  unsigned char req_flag = 0x00;

  /* Initialize sta and bss list */
  ieee1905_glist_init(&steer_req->sta_list);
  ieee1905_glist_init(&steer_req->bss_list);

  rc = i5MessageTlvExtract(pmsg, i5TlvSteeringRequestType, &length, &pvalue, i5MessageTlvExtractWithReset);
  if (rc != 0) {
    goto end;
  }

  if (length < i5TlvSteeringRequest_Min_Length) {
    goto end;
  }

  memcpy(steer_req->source_bssid, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;

  req_flag = *pvalue;
  pvalue++;
  if (req_flag & I5_TLV_STEER_REQUEST_MODE) {
    steer_req->request_flags |= IEEE1905_STEER_FLAGS_MANDATE;
  } else {
    steer_req->request_flags |= IEEE1905_STEER_FLAGS_OPPORTUNITY;
  }

  if (req_flag & I5_TLV_STEER_DISASSOC_IMNT) {
    steer_req->request_flags |= IEEE1905_STEER_FLAGS_DISASSOC_IMNT;
  }
  if (req_flag & I5_TLV_STEER_BTM_ABRIDGED) {
    steer_req->request_flags |= IEEE1905_STEER_FLAGS_BTM_ABRIDGED;
  }

  /* If request mode is not 1(Means its steering opportunity) read steering opportunity window */
  if (IEEE1905_IS_STEER_OPPORTUNITY(steer_req->request_flags)) {
    steer_req->opportunity_window = ntohs(*((unsigned short *)pvalue));
  }
  pvalue += 2;

  steer_req->dissassociation_timer = ntohs(*((unsigned short *)pvalue));
  pvalue += 2;

  sta_count = *pvalue;
  pvalue++;

  if (sta_count == 0) { /* No STAs */
    goto end;
  }

  if (length < (extracted_len + (sta_count * MAC_ADDR_LEN))) {
    rc = -1;
    goto end;
  }

  /* Get all the STAs */
  for (i = 0; i < sta_count; i++) {
    ieee1905_sta_list *sta_info;

    sta_info = (ieee1905_sta_list*)malloc(sizeof(*sta_info));
    if (!sta_info) {
      i5TraceDirPrint("malloc error\n");
      rc = -1;
      goto end;
    }

    memcpy(sta_info->mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;
    ieee1905_glist_append(&steer_req->sta_list, (dll_t*)sta_info);
  }

  extracted_len += (sta_count * MAC_ADDR_LEN);

  if (length < extracted_len + 1) {
    rc = -1;
    goto end;
  }

  bss_count = *pvalue;
  if (bss_count <= 0) { /* No BSS */
    goto end;
  }
  pvalue++;
  extracted_len++;

  if (length < (extracted_len + (bss_count * (MAC_ADDR_LEN + 1 + 1)))) {
    rc = -1;
    goto end;
  }

  /* Get all the BSS */
  for (i = 0; i < bss_count; i++) {
    ieee1905_bss_list *bss_info;

    bss_info = (ieee1905_bss_list*)malloc(sizeof(*bss_info));
    if (!bss_info) {
      i5TraceDirPrint("malloc error\n");
      rc = -1;
      goto end;
    }
    memset(bss_info, 0, sizeof(*bss_info));
    memcpy(bss_info->bssid, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;
    bss_info->target_op_class = *pvalue;
    pvalue++;
    bss_info->target_channel = *pvalue;
    pvalue++;
    ieee1905_glist_append(&steer_req->bss_list, (dll_t*)bss_info);
  }

end:
  return rc;
}

#if defined(MULTIAPR2)
/* Insert Unsuccessful Association Policy TLV */
int i5TlvUnsuccessfulAssociationPolicyTypeInsert(i5_message_type *pmsg,
  ieee1905_policy_config *configlist)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvUnsuccessfulAssociationPolicy_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvUnsuccessfulAssociationPolicyType;
  ptlv->length = htons(i5TlvUnsuccessfulAssociationPolicy_Length);
  len += sizeof(i5_tlv_t);

  buf[len] = (unsigned char)configlist->unsuccessful_assoc_config.report_flag;
  len++;
  *((unsigned int *)&buf[len]) = htonl(configlist->unsuccessful_assoc_config.max_reporting_rate);
  len += 4;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Unsuccessful Association Policy TLV */
int i5TlvUnsuccessfulAssociationPolicyTypeExtract(i5_message_type *pmsg,
  ieee1905_policy_config *configlist)
{
  unsigned char *pvalue;
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvUnsuccessfulAssociationPolicyType, &length, &pvalue,
    i5MessageTlvExtractWithReset) == 0) {
    if (length >= i5TlvUnsuccessfulAssociationPolicy_Length) {
      configlist->unsuccessful_assoc_config.report_flag = *pvalue;
      pvalue++;

      configlist->unsuccessful_assoc_config.max_reporting_rate = ntohl(*(unsigned int *)pvalue);
      i5TraceInfo("Unsuccessful Asssociation report_flag = 0x%02x, max_reporting_rate= %d\n",
        (unsigned char)configlist->unsuccessful_assoc_config.report_flag,
	configlist->unsuccessful_assoc_config.max_reporting_rate);
      return 0;
    }
  }
  return -1;
}
#endif /* MULTIAPR2 */

/* TLV to Send the Client Association Control Request */
int i5TlvClientAssociationControlRequestTypeInsert(i5_message_type *pmsg,
  ieee1905_block_unblock_sta *block_unblock_sta)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;
  dll_t *item_p, *next_p;
  ieee1905_sta_list *staInfo;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  /* BSSID */
  memcpy(pbuf, block_unblock_sta->source_bssid, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  /* Association Control */
  *pbuf = (unsigned char)block_unblock_sta->unblock;
  pbuf++;

  /* validity period */
  *((unsigned short *)pbuf) = htons(block_unblock_sta->time_period);
  pbuf += 2;

  /* STA List Count */
  *pbuf = (unsigned char)block_unblock_sta->sta_list.count;
  pbuf++;

  /* STA MAC address */
  for (item_p = dll_head_p(&block_unblock_sta->sta_list.head);
    !dll_end(&block_unblock_sta->sta_list.head, item_p);
    item_p = next_p) {
    next_p = dll_next_p(item_p);
    staInfo = (ieee1905_sta_list*)item_p;
    memcpy(pbuf, staInfo->mac, MAC_ADDR_LEN);
    pbuf += MAC_ADDR_LEN;
  }

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvClientAssociationControlRequestType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Client Association Control Request TLV */
int i5TlvClientAssociationControlRequestTypeExtract(i5_message_type *pmsg,
  ieee1905_block_unblock_sta *block_unblock_sta)
{
  unsigned char *pvalue;
  unsigned int length, extracted_len;
  int rc = 0, i;
  unsigned char sta_count = 0;

  /* Initialize sta list */
  ieee1905_glist_init(&block_unblock_sta->sta_list);

  rc = i5MessageTlvExtract(pmsg, i5TlvClientAssociationControlRequestType, &length, &pvalue,
    i5MessageTlvExtractWithReset);
  if (rc != 0) {
    goto end;
  }

  if (length < i5TlvClientAssociationControlRequest_Min_Length) {
    goto end;
  }

  memcpy(block_unblock_sta->source_bssid, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;

  block_unblock_sta->unblock = *pvalue;
  pvalue++;

  block_unblock_sta->time_period = ntohs(*((unsigned short *)pvalue));
  pvalue += 2;

  sta_count = *pvalue;
  pvalue++;
  extracted_len = i5TlvClientAssociationControlRequest_Min_Length;

  if (length < (extracted_len + (sta_count * MAC_ADDR_LEN))) {
    goto end;
  }
  /* Get all the STAs */
  for (i = 0; i < sta_count; i++) {
    ieee1905_sta_list *sta_info;

    sta_info = (ieee1905_sta_list*)malloc(sizeof(*sta_info));
    if (!sta_info) {
      i5TraceDirPrint("malloc error\n");
      goto end;
    }

    memcpy(sta_info->mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;
    ieee1905_glist_append(&block_unblock_sta->sta_list, (dll_t*)sta_info);
  }

  return 0;

end:
  return -1;
}

/* TLV to add BTM Report */
int i5TlvSteeringBTMReportTypeInsert(i5_message_type *pmsg, ieee1905_btm_report *btm_report)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  /* BSSID */
  memcpy(pbuf, btm_report->source_bssid, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  /* STA MAC */
  memcpy(pbuf, btm_report->sta_mac, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  /* BTM Status code */
  *pbuf = (unsigned char)btm_report->status;
  pbuf++;

  /* Target BSSID if valid */
  if (!i5DmIsMacNull(btm_report->trgt_bssid)) {
    memcpy(pbuf, btm_report->trgt_bssid, MAC_ADDR_LEN);
    pbuf += MAC_ADDR_LEN;
  }

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvSteeringBTMReportType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract BTM Report TLV */
int i5TlvSteeringBTMReportTypeExtract(i5_message_type *pmsg, ieee1905_btm_report *btm_report)
{
  unsigned char *pvalue;
  unsigned int length;
  int rc = 0;

  rc = i5MessageTlvExtract(pmsg, i5TlvSteeringBTMReportType, &length, &pvalue,
    i5MessageTlvExtractWithReset);
  if (rc != 0) {
    goto end;
  }

  if (length < i5TlvSteeringBTMReport_Min_Length) {
    goto end;
  }

  memcpy(btm_report->source_bssid, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;

  memcpy(btm_report->sta_mac, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;

  btm_report->status = *pvalue;
  pvalue++;

  /* If the Target BSSID is included, extract it as it is optional */
  if (length >= (i5TlvSteeringBTMReport_Min_Length + MAC_ADDR_LEN)) {
    memcpy(btm_report->trgt_bssid, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;
  }

  return 0;

end:
  return -1;
}

/* TLV to report the Higher layer data TLV */
int i5TlvHigherLayerDataTypeInsert(i5_message_type *pmsg,
  i5HigherLayerProtocolField_Values protocol, unsigned char *data, unsigned int data_len)
{
  unsigned char *pbuf;
  unsigned int rc = 0;
  int iter_frag = 0;
  div_t q = div(1, 1);

  /* Header octets (22) = Ethernet frame header (14 octets) + 1905.1 CMDU header (8 octets) */
  unsigned int hdr_sz = (sizeof(struct ethhdr) + sizeof(i5_message_header_type));

  /* Per HLD TLV octets (4) = TLV header (3 octects) + protocol (1 octect) */
  unsigned int per_tlv_octets = (sizeof(i5_tlv_t) + sizeof(unsigned char));

  /* Skip Size (26) = ETH Heaher (22) + Per HLD TLV octets (4) */
  unsigned int skip_sz = hdr_sz + per_tlv_octets;

  /* Max Fragment Size (1488) = ETH_FRAME_LEN (1514) - Skip Size (26) */
  unsigned int max_fragment_sz = I5_PACKET_BUF_LEN - skip_sz;

  /* Find Number of fragments required to send Full Payload */
  int num_fragments = 1;
  if (data_len > max_fragment_sz) {

    q = div(data_len, max_fragment_sz);
    num_fragments  = (q.rem == 0) ?  q.quot : (q.quot + 1);
  }

  /* Allocate memory for single fragment */
  if ((pbuf = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  /* Loop through each fragment and create a HLD TLV for that Fragment of Payload */
  for (iter_frag = 1; iter_frag <= num_fragments; iter_frag++) {

    i5_tlv_t *ptlv;
    unsigned short index = 0, tlv_data_len = 0, this_fragment_sz = 0;

    memset(pbuf, 0, I5_PACKET_BUF_LEN);

    /* Find Actual Fragment size :
      * If num_fragments == 1, Means data can be sent in a single fragment
      * If num_fragments > 1, and :
      *     If Curr_frag is not last fragment, this_frag_sz = max_fragment_sz
      *     If Curr_frag is       last fragment & reminder == 0, this_frag_sz = max_fragment_sz
      *     If Curr_frag is       last fragment & reminder != 0, this_frag_sz = remaining bytes of whole data
      */
    this_fragment_sz = (num_fragments > 1) ?
      ( (num_fragments == iter_frag) ? ( (q.rem == 0) ? max_fragment_sz : q.rem ) :
           max_fragment_sz ) : (data_len);

    /* Find TLV Length = Actual Fragment size + Protocol */
    tlv_data_len = this_fragment_sz + sizeof(unsigned char);

    /* Fill TLV Header */
    ptlv = (i5_tlv_t *)pbuf;
    ptlv->type = i5TlvHigherLayerDataType;
    ptlv->length = htons(tlv_data_len);
    index += sizeof(i5_tlv_t);

    /* Fill Protocol Field */
    pbuf[index] = (unsigned char)protocol;
    index++;

    /* Fill Fragmented Data with this_fragment_sz */
    if (this_fragment_sz > 0) {
      memcpy(&pbuf[index], data, this_fragment_sz);
      index += this_fragment_sz;
    }

    /* Insert Full TLV for this_fragment */
    rc &= i5MessageInsertTlv(pmsg, pbuf, index);

    /* Move data pointer ahead with this_fragment_sz */
    data += this_fragment_sz;

  }

  free(pbuf);

  return (rc);
}

/* Get the Maximum length of all the TLVs in a message */
static unsigned int i5TlvGetAllTLVHigherLayerPayloadMaxLength(i5_message_type *pmsg)
{
  /* Maximum Length of all TLVs = Num of Fragments * Maximum ethernet packet size */
  return ( (i5MessageLastPacketFragmentIdentifierGet(pmsg) + 1) * I5_PACKET_BUF_LEN );
}

/* Copy all the TLVs and the last header */
static int i5TlvGetAllTLVHigherLayerPayload(i5_message_type *pmsg,
  unsigned char *header, unsigned char *payload, unsigned int *out_payload_length, unsigned char *out_protocol)
{
  i5_packet_type *lastPacket = (i5_packet_type *)(pmsg->packet_list.ll.next);
  unsigned char *pvalue = NULL;
  unsigned int tlv_data_len = 0, this_fragment_sz = 0, payload_length = 0;

  /* Header octets (22) = Ethernet frame header (14 octets) + 1905.1 CMDU header (8 octets) */
  unsigned int hdr_sz = (sizeof(struct ethhdr) + sizeof(i5_message_header_type));

  if (!header || !payload || !out_payload_length || !out_protocol) {
    i5TraceError("Invalid Arguments\n");
    return -1;
  }
  *out_payload_length = 0;

  i5MessageReset(pmsg);
  while ((i5MessageTlvExtract(pmsg, i5TlvHigherLayerDataType,
    &tlv_data_len, &pvalue, i5MessageTlvExtractWithoutReset)) == 0) {

    if (tlv_data_len < 1) {
      i5TraceError("Invalid Protocol in Higher Layer Data : tlv_len[%d]\n", tlv_data_len);
      return -1;
    }

    /* Get Higher Layer Data Protocol */
    *out_protocol = pvalue[0];
    i5Trace("Higher Layer Data Protocol %d\n", *out_protocol);
    /* Decreament Protocol field size fm this_fragment_sz */
    this_fragment_sz = tlv_data_len - sizeof(*out_protocol);

    /* Get Higher Layer Data Payload & Append to Out Payload Buffer */
    memcpy(&payload[payload_length], &pvalue[1], this_fragment_sz);

    /* Update Payload Length */
    payload_length += this_fragment_sz;
  }

  /* Go thorugh each packet to get Last Packet */
  if ( NULL == lastPacket ) {
    i5TraceError("Invalid Packet List\n");
    return -1;
  }
  else {
    while ( lastPacket->ll.next != NULL ) {
      lastPacket = lastPacket->ll.next;
    }
  }

  /* Now copy the Last Header to Out Header Buffer */
  if (lastPacket) {
    memcpy(header, &lastPacket->pbuf[0], hdr_sz);
  }

  /* Return Full Payload Length */
  *out_payload_length = payload_length;
  return 0;
}

/* Routine to Send Higher Layer data to Apps which have not registered this cb : e.g. HLE */
static void ieee1905_send_higher_layer_data_to_HLE(unsigned char *al_mac, unsigned char protocol,
  unsigned char *payload, unsigned int payload_length, unsigned char *header, unsigned char header_length)
{
  unsigned int sock_data_len = 0, pos = 0;
  unsigned char *sock_data = NULL;
  i5_higher_layer_data_recv_t hl_data_obj, *hl_data = NULL;
  int sockfd = -1;

  /* Get Socket data Size and Allocate the Buffer for Socket data to be sent */
  sock_data_len = sizeof(hl_data_obj.tag) + sizeof(hl_data_obj.length) + sizeof(hl_data_obj.src_al_mac) +
	  sizeof(hl_data_obj.protocol) + sizeof(hl_data_obj.header_length) +
	  header_length + payload_length;

  sock_data = (unsigned char*)malloc(sock_data_len);
  if (!sock_data) {
    i5TraceDirPrint("malloc error\n");
    return;
  }
  memset(sock_data, 0, sock_data_len);

  /* Prepare sock data. */
  hl_data = (i5_higher_layer_data_recv_t*)sock_data;

  /* [1] Higher Layer Data TAG. Its value is fixed to I5_API_CMD_HLE_SEND_HL_DATA (44) */
  hl_data->tag = I5_API_CMD_HLE_SEND_HL_DATA;
  pos += sizeof(hl_data_obj.tag);

  /* [2] Length : Total Length of subsequent (following) fields, which is =
    * 6 + 1 + 1 + Header length (22) + Paylaod length (Variable)
    */
  hl_data->length = sock_data_len;
  pos += sizeof(hl_data_obj.length);

  /* [3] Source AL MAC Address : AL Mac address of sender of HLD */
  memcpy(hl_data->src_al_mac, al_mac, IEEE1905_MAC_ADDR_LEN);
  pos += sizeof(hl_data_obj.src_al_mac);

  /* [4] Higher Layer Protocol ID of HLD, sent towards HLE through BRCM MAP Entity */
  hl_data->protocol = protocol;
  pos += sizeof(hl_data_obj.protocol);

  /* [5] Header Length = Ethernet frame header (14 octets) + 1905.1 CMDU header (8 octets) */
  hl_data->header_length = header_length;
  pos += sizeof(hl_data_obj.header_length);

  /* [6] Copy Actual Header */
  memcpy(&sock_data[pos], header, header_length);
  pos += header_length;

  /* [7] Copy Actual Payload */
  memcpy(&sock_data[pos], payload, payload_length);
  pos += payload_length;

  /* Connect to the server */
  sockfd = i5SocketConnectToServer(I5_SOCKET_LOOPBACK_IP, EAPD_WKSP_WBD_EVENT_PORT);
  if (sockfd == -1) {
    i5TraceError("Failed to connect\n");
    goto end;
  }

  /* Send the payload */
  if (i5SocketSendData(sockfd, (char*)sock_data, sock_data_len) <= 0) {
    i5TraceError("Failed to send\n");
    goto end;
  }

end:
  i5SocketCloseSockFD(&sockfd);
  free(sock_data);
}

/* Extract Higher layer data TLV */
int i5TlvHigherLayerDataTypeExtract(i5_message_type *pmsg)
{
  unsigned char *payload = NULL, *header = NULL;
  unsigned int payload_length = 0, max_payload_length = 0;
  unsigned char protocol = 0, header_length = 0;
  int rc = 0;

  /* Get Payload Size and Allocate the Buffer for Payload */
  max_payload_length = i5TlvGetAllTLVHigherLayerPayloadMaxLength(pmsg);
  payload = (unsigned char*)malloc(max_payload_length);
  if (payload == NULL) {
    i5TraceDirPrint("malloc error\n");
    rc = -1;
    goto end;
  }

  /* Get Header Size and Allocate the Buffer for Header */
  header_length = sizeof(struct ethhdr) + sizeof(i5_message_header_type);
  header = (unsigned char*)malloc(header_length);
  if (header == NULL) {
    i5TraceDirPrint("malloc error\n");
    rc = -1;
    goto end;
  }

  /* Copy Full Payload and Header data from each packet, each HLD TLV */
  rc = i5TlvGetAllTLVHigherLayerPayload(pmsg, header, payload, &payload_length, &protocol);
  if (rc == -1) {
    i5TraceError("Higher Layer Data TLV Parsing Failed\n");
    goto end;
  }

  /* call callback to notify Higher Layer data to Apps which have registered this cb */
  if (i5_config.cbs.higher_layer_data) {
    i5_config.cbs.higher_layer_data(i5MessageSrcMacAddressGet(pmsg), protocol,
     payload, payload_length, header, header_length);
  }

  /* Send Higher Layer data to Apps which have not registered this cb : e.g. HLE */
  ieee1905_send_higher_layer_data_to_HLE(i5MessageSrcMacAddressGet(pmsg), protocol,
    payload, payload_length, header, header_length);

end:
  if (payload) {
    free(payload);
  }
  if (header) {
    free(header);
  }
  return rc;
}

/* TLV to report the Channel Preference Capabilities */
int i5TlvChannelPreferenceTypeInsert(i5_message_type *pmsg, unsigned char *mac,
  unsigned char *data, unsigned char len)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end
  memcpy(pbuf, mac, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  memcpy(pbuf, data, len);
  pbuf += len;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvChannelPreferenceType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Insert the stored channel pref report */
int i5TlvChannelPreferenceTypeInsert_Stored(i5_message_type *pmsg, unsigned char *mac,
  ieee1905_chan_pref_rc_map_array *cp)
{
  unsigned char *pbuf, *pmem, pval = 0x0;
  i5_tlv_t *ptlv;
  int rc = 0, i = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end
  memcpy(pbuf, mac, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  *pbuf = cp->rc_count;
  pbuf++;

  i5TraceInfo("Total rc count = %d\n", cp->rc_count);

  /* For each regulatory class */
  while (i < cp->rc_count) {
    *pbuf = cp->rc_map[i].regclass;
    pbuf++;

    *pbuf = cp->rc_map[i].count;
    pbuf++;

    memcpy(pbuf, cp->rc_map[i].channel, cp->rc_map[i].count);
    pbuf += cp->rc_map[i].count;

    pval |= cp->rc_map[i].reason & 0xF;		/* 0 - 3 */
    pval |= (cp->rc_map[i].pref << 4) & 0xF0;	/* 4 - 7 */

    *pbuf = pval;
    pbuf++;

    i++;
  }
  i5TraceInfo("Rpt data set done\n");
  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvChannelPreferenceType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Channel Preference Capabilities TLV */
int i5TlvChannelPreferenceTypeExtract(i5_message_type *pmsg, int isAgent)
{
  i5_dm_device_type *pdevice;
  i5_dm_interface_type *pdmif;
  unsigned char *pvalue, mac[MAC_ADDR_LEN], *buf;
  unsigned int length;
  ieee1905_chan_pref_rc_map_array *cp = NULL;
  int i,j, k, idx;
  unsigned int reg_class_count;
  i5_dm_rc_chan_map_type *rc_chan_ptr;
  i5_dm_rc_chan_map_type *rc_chan_map;
  uint8 chan_count = 0;
  uint8 *chan_list = NULL;

  if (isAgent) {
    pdevice = i5DmGetSelfDevice();
  } else {
    pdevice = i5DmDeviceFind(i5MessageSrcMacAddressGet(pmsg));
  }
  if (pdevice == NULL) {
    i5TraceError("Neighbour device does not exist\n");
    return -1;
  }

  rc_chan_map = i5DmGetRCChannelMap(&reg_class_count);

  i5MessageReset(pmsg);
  while ((i5MessageTlvExtract(pmsg, i5TlvChannelPreferenceType,
    &length, &pvalue, i5MessageTlvExtractWithoutReset)) == 0) {
    if (length < MAC_ADDR_LEN + 1) {
      i5TraceError("Invalid channel preference length: %d\n", length);
      return -1;
    }
    memset(mac, 0, MAC_ADDR_LEN);
    memcpy(mac, pvalue, MAC_ADDR_LEN);
    idx = MAC_ADDR_LEN;

    pdmif = (i5_dm_interface_type *)pdevice->interface_list.ll.next;
    while (pdmif != NULL) {
      if (memcmp(mac, pdmif->InterfaceId, MAC_ADDR_LEN) == 0)
        break;
      pdmif = pdmif->ll.next;
    }

    if (!pdmif) {
      i5TraceError("Interface " I5_MAC_DELIM_FMT " does not exist\n", I5_MAC_PRM(mac));
      continue;
    }

    if (isAgent && pdmif->msg_stat_flag == I5_MSG_RECEIVED) {
      i5Trace("Channel already set by vendor TLV for " I5_MAC_DELIM_FMT
        ". Ignore prefernece TLV\n", I5_MAC_PRM(mac));
      continue;
    }
    pdmif->msg_stat_flag = I5_MSG_RECEIVED;

    if (!pdmif->bss_list.ll.next) {
      i5TraceError("No bss configured on Interface " I5_MAC_DELIM_FMT "\n", I5_MAC_PRM(mac));
      continue;
    }

    if (pvalue[idx] > I5_MAX_REGCLASS) {
      i5TraceError("Regulatory classes %d is more than max allowed\n", pvalue[idx]);
      return -1;
    }

    cp = &pdmif->ChanPrefs;
    if (cp->rc_map) {
      i5Trace("Clearing old channel preferences on receival of new preferences\n");
      free(cp->rc_map);
      cp->rc_map = NULL;
    }

    cp->rc_count = pvalue[idx++];
    if (cp->rc_count == 0) {
      continue;
    }
    i5TraceInfo("Regclass count: %d\n", cp->rc_count);
    buf = malloc(sizeof(ieee1905_chan_pref_rc_map) * cp->rc_count);
    if (buf == NULL) {
      i5TraceError("malloc failed for rc_map \n");
      return -1;
    }
    cp->rc_map = (ieee1905_chan_pref_rc_map *)buf;

    for (i = 0; (i < cp->rc_count) && (idx + 1 < length); i++) {
      for (k = 0; k < reg_class_count; k++) {
        if (pvalue[idx] == rc_chan_map[k].regclass)
          break;
      }
      if (k == reg_class_count) {
        i5TraceError("Invalid regclass: %d\n", pvalue[idx]);
        goto end;
      }
      cp->rc_map[i].regclass = pvalue[idx++];
      rc_chan_ptr = &rc_chan_map[k];

      if (pvalue[idx] >  IEEE1905_MAX_RCCHANNELS) {
        i5TraceError("Channel per regclass count %d is more than max allowed\n", pvalue[idx]);
        goto end;
      }

      cp->rc_map[i].count = pvalue[idx++];
     chan_count = rc_chan_ptr->count;
     chan_list = rc_chan_ptr->channel;

     for (j = 0; (j < cp->rc_map[i].count) && (idx + 1 < length) ; j++) {
        for (k = 0; k < chan_count; k++) {
          if (pvalue[idx] == chan_list[k]) {
            break;
          }
        }
        if (k == chan_count) {
          i5TraceError("Invalid channel: %d\n", pvalue[idx]);
          goto end;
        }
        cp->rc_map[i].channel[j] = pvalue[idx++];
      }
      cp->rc_map[i].reason = pvalue[idx] & 0xf;
      cp->rc_map[i].pref = (pvalue[idx++] & 0xf0) >> 4;
      i5TraceInfo("Regclass: %d Channel count: %d reason: %d pref: %d\n",
        cp->rc_map[i].regclass, cp->rc_map[i].count,
        cp->rc_map[i].reason, cp->rc_map[i].pref);
    }
    /* If the channel selection request callback is registered call it */
    if (isAgent && i5_config.cbs.recv_chan_selection_req) {
      ieee1905_chan_pref_rc_map_array LocalChanPrefs;

      memset(&LocalChanPrefs, 0, sizeof(LocalChanPrefs));
      if (i5_config.cbs.prepare_channel_pref) {
        i5_config.cbs.prepare_channel_pref(pdmif, &LocalChanPrefs);
      }
      i5_config.cbs.recv_chan_selection_req(pdevice->DeviceId, pdmif->InterfaceId,
        cp, LocalChanPrefs.rc_count, LocalChanPrefs.rc_map);
      if (LocalChanPrefs.rc_map) {
        free(LocalChanPrefs.rc_map);
      }
    }

    /* Update the DFS status of the current operating channel in controller */
    if (!isAgent) {
      i5DmUpdateDFSStatusFromChannelPreference(pdmif);
    }
  }
  return 0;

end:
  if (cp && cp->rc_map) {
    free(cp->rc_map);
    cp->rc_map = NULL;
    cp->rc_count = 0;
  }
  return -1;
}

/* Extract Transmit power limit TLV */
int i5TlvTransmitPowerLimitTypeExtract(i5_message_type *pmsg)
{
  i5_dm_device_type *pdevice;
  i5_dm_interface_type *pdmif;
  unsigned char *pvalue, mac[MAC_ADDR_LEN];
  unsigned int length;
  int ret = 0;

  pdevice = i5DmGetSelfDevice();
  if (pdevice == NULL) {
    i5TraceError("Neighbour device does not exist\n");
    ret = -1;
    goto end;
  }

  i5MessageReset(pmsg);
  while ((i5MessageTlvExtract(pmsg, i5TlvTransmitPowerLimitType,
    &length, &pvalue, i5MessageTlvExtractWithoutReset)) == 0) {
    if (length < MAC_ADDR_LEN + 1) {
      i5TraceError("Invalid  length: %d\n", length);
      ret = -1;
      goto end;
    }
    memset(mac, 0, MAC_ADDR_LEN);
    memcpy(mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;

    pdmif = (i5_dm_interface_type *)pdevice->interface_list.ll.next;
    while (pdmif != NULL) {
      if (memcmp(mac, pdmif->InterfaceId, MAC_ADDR_LEN) == 0)
        break;
      pdmif = pdmif->ll.next;
    }

    if (!pdmif) {
      i5TraceError("Interface " I5_MAC_DELIM_FMT " does not exist\n", I5_MAC_PRM(mac));
      continue;
    }

    if (!pdmif->bss_list.ll.next) {
      i5TraceError("No bss configured on Interface " I5_MAC_DELIM_FMT "\n", I5_MAC_PRM(mac));
      continue;
    }

    pdmif->TxPowerLimit = *pvalue;
    if (i5_config.cbs.set_tx_power_limit) {
      i5TraceInfo("Setting tx power as %d for Interface %s\n",
        pdmif->TxPowerLimit, pdmif->ifname);
      i5_config.cbs.set_tx_power_limit(pdmif->ifname, pdmif->TxPowerLimit);
    }
  }
end:
  return ret;
}

/* Insert Channel Selection response TLV */
int i5TlvChannelSelectionResponseTypeInsert(i5_message_type *pmsg, unsigned char *mac, uint8 resp_code)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end
  memcpy(pbuf, mac, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  *pbuf = resp_code;
  pbuf++;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvChannelSelectionResponseType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Channel Selection Response TLV */
int i5TlvChannelSelectionResponseTypeExtract(i5_message_type *pmsg)
{
  i5_dm_device_type *pdevice;
  i5_dm_interface_type *pdmif;
  unsigned char *pvalue, mac[MAC_ADDR_LEN];
  unsigned int length, found = 0;
  int rc = 0;

  pdevice = i5DmDeviceFind(i5MessageSrcMacAddressGet(pmsg));
  if (pdevice == NULL) {
    i5TraceError("Neighbour device does not exist\n");
    rc = -1;
    goto end;
  }

  i5MessageReset(pmsg);
  while ((rc = i5MessageTlvExtract(pmsg, i5TlvChannelSelectionResponseType,
    &length, &pvalue, i5MessageTlvExtractWithoutReset)) == 0) {
    found = 1;
    if (length < MAC_ADDR_LEN + 1) {
      rc = -1;
      goto end;
    }
    memset(mac, 0, MAC_ADDR_LEN);
    memcpy(mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;

    pdmif = (i5_dm_interface_type *)pdevice->interface_list.ll.next;
    while (pdmif != NULL) {
      if (pdmif->bss_list.ll.next != NULL &&
        memcmp(mac, pdmif->InterfaceId, MAC_ADDR_LEN) == 0)
        break;
      pdmif = pdmif->ll.next;
    }
    if (!pdmif) {
      i5TraceError("Neighbour Interface " I5_MAC_DELIM_FMT " does not exist\n", I5_MAC_PRM(mac));
      continue;
    }
    i5Trace("Neighbour Interface " I5_MAC_DELIM_FMT " Channel Selection Response: %s(0x%02x)\n", I5_MAC_PRM(mac),
      *pvalue ? "Decline": "Accept", *pvalue);
  }
  if (!found) {
    rc = -1;
  }
end:
  return rc;
}
/* Insert Operating Channel report TLV */
int i5TlvOperatingChannelReportTypeInsert(i5_message_type *pmsg,
	ieee1905_operating_chan_report *chan_rpt)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;
  int i;

  if (!chan_rpt || !chan_rpt->list) {
    i5TraceError("NULL Operating Channel report Data\n");
    return -1;
  }
  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end
  memcpy(pbuf, chan_rpt->radio_mac, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  *pbuf = chan_rpt->n_op_class;
  pbuf++;
  for (i = 0; i < chan_rpt->n_op_class; i++) {
    *pbuf = chan_rpt->list[i].op_class;
    pbuf++;
    *pbuf = chan_rpt->list[i].chan;
    pbuf++;
  }
  *pbuf = chan_rpt->tx_pwr;
  pbuf++;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvOperatingChannelReportType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Operating Channel report TLV */
int i5TlvOperatingChannelReportTypeExtract(i5_message_type *pmsg)
{
  int rc = 0;
  unsigned char *pvalue;
  unsigned int length;
  i5_dm_device_type *pdevice;
  i5_dm_interface_type *pdmif;
  ieee1905_operating_chan_report chan_report;
  int i;
  unsigned int tlvLengthReqd;

  pdevice = i5DmDeviceFind(i5MessageSrcMacAddressGet(pmsg));
  if (pdevice == NULL) {
    i5TraceError("Neighbour device does not exist\n");
    rc = -1;
    goto end;
  }

  i5MessageReset(pmsg);
  while ((rc = i5MessageTlvExtract(pmsg, i5TlvOperatingChannelReportType, &length, &pvalue,
    i5MessageTlvExtractWithoutReset)) == 0) {

    memset(&chan_report, 0, sizeof(chan_report));
    if (length < i5TlvOperatingChannelReport_Min_Length) {
      i5TraceError("Length %d is less than the minimum Operating Channel Report Length\n", length);
      rc = -1;
      goto end;
    }

    memcpy(chan_report.radio_mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;

    pdmif = i5DmInterfaceFind(pdevice, chan_report.radio_mac);
    if (pdmif == NULL) {
      i5TraceError("Neighbour Interface " I5_MAC_DELIM_FMT " does not exist\n",
        I5_MAC_PRM(chan_report.radio_mac));
      rc = -1;
      goto end;
    }

    chan_report.n_op_class = *pvalue++;

    tlvLengthReqd = i5TlvOperatingChannelReport_Min_Length + ((chan_report.n_op_class - 1) * 2);
    if (length != tlvLengthReqd) {
      i5TraceError("Operating channel report TLV Length %d not correct for %d operating classes. "
        "Required length is %d\n", length, chan_report.n_op_class, tlvLengthReqd);
      rc = -1;
      goto end;
    }

    chan_report.list = (operating_rpt_opclass_chan_list *)malloc(
      sizeof(operating_rpt_opclass_chan_list) * chan_report.n_op_class);
    if (!chan_report.list) {
      i5TraceDirPrint("Memory Malloc error, return with error \n");
      rc = -1;
      goto end;
    }
    for (i = 0; i < chan_report.n_op_class; i++) {
      chan_report.list[i].op_class = *pvalue++;
      chan_report.list[i].chan = *pvalue++;
    }
      chan_report.tx_pwr = *pvalue;

    i5TraceInfo("Neighbor Interface"I5_MAC_DELIM_FMT" Tx pwr: %d Number of opclasses: %d\n",
      I5_MAC_PRM(chan_report.radio_mac), chan_report.tx_pwr, chan_report.n_op_class);
    if (i5_config.cbs.operating_chan_report) {
      i5_config.cbs.operating_chan_report(i5MessageSrcMacAddressGet(pmsg), &chan_report);
    }

    /* Update the DFS status of the current operating channel in controller */
    i5DmUpdateDFSStatusFromChannelPreference(pdmif);
    free(chan_report.list);
  }
end:
  return rc;
}

/* TLV to add Ap Metric Query. All the BSSIDs are stored in the linear array */
int i5TlvAPMetricQueryTypeInsert(i5_message_type *pmsg, unsigned char *bssids, unsigned char count)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0, i;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  /* Number of BSSIDs */
  *pbuf = count;
  pbuf++;

  for (i = 0; i < count; i++) {
    memcpy(pbuf, &bssids[i*MAC_ADDR_LEN], MAC_ADDR_LEN);
    pbuf += MAC_ADDR_LEN;
  }

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvAPMetricQueryType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Ap Metric Query TLV */
int i5TlvAPMetricQueryTypeExtract(i5_message_type *pmsg, unsigned char **bssids_out,
  unsigned char *count)
{
  int ret = -1, i, index = 0;
  unsigned char *pvalue;
  unsigned int length;
  unsigned char tmpCount;
  unsigned char *bssids = NULL;

  if (i5MessageTlvExtract(pmsg, i5TlvAPMetricQueryType, &length, &pvalue,
    i5MessageTlvExtractWithReset) == 0) {
    if (length >= 1) {
      tmpCount = *pvalue;
      index++;

      if (tmpCount <= 0) {
        ret = 0;
        goto end;
      }

      bssids = (unsigned char*)malloc(tmpCount * MAC_ADDR_LEN);
      if (bssids == NULL) {
        i5TraceDirPrint("Malloc for bssids failed\n");
        goto end;
      }

      for (i = 0; i < tmpCount && length >= (index + MAC_ADDR_LEN); i++) {
        memcpy(&bssids[i*MAC_ADDR_LEN], &pvalue[index], MAC_ADDR_LEN);
        index += MAC_ADDR_LEN;
      }
      *bssids_out = bssids;
      *count = tmpCount;

      return 0;
    }
  }

end:
  if (bssids) {
    free(bssids);
  }

  *bssids_out = NULL;
  *count = 0;
  return ret;
}

/* TLV to add Ap Metrics */
int i5TlvAPMetricsTypeInsert(i5_message_type *pmsg, unsigned char *bssid, unsigned short sta_count,
  ieee1905_ap_metric *apMetric, ieee1905_interface_metric *ifrMetric)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  memcpy(pbuf, bssid, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  *pbuf = ifrMetric->chan_util;
  pbuf++;

  *((unsigned short *)pbuf) = htons(sta_count);
  pbuf += 2;
  *pbuf = apMetric->include_bit_esp;
  pbuf++;

  if (apMetric->include_bit_esp & IEEE1905_INCL_BIT_ESP_BE) {
    memcpy(pbuf, apMetric->esp_ac_be, IEEE1905_ESP_LEN);
    /* Swap bytes 0 and 2 to conform to IEEE1905 Endianness */
    I5_SWAP(pbuf[0], pbuf[2], unsigned char);
    pbuf += IEEE1905_ESP_LEN;
  }

  if (apMetric->include_bit_esp & IEEE1905_INCL_BIT_ESP_BK) {
    memcpy(pbuf, apMetric->esp_ac_bk, IEEE1905_ESP_LEN);
    /* Swap bytes 0 and 2 to conform to IEEE1905 Endianness */
    I5_SWAP(pbuf[0], pbuf[2], unsigned char);
    pbuf += IEEE1905_ESP_LEN;
  }

  if (apMetric->include_bit_esp & IEEE1905_INCL_BIT_ESP_VO) {
    memcpy(pbuf, apMetric->esp_ac_vo, IEEE1905_ESP_LEN);
    /* Swap bytes 0 and 2 to conform to IEEE1905 Endianness */
    I5_SWAP(pbuf[0], pbuf[2], unsigned char);
    pbuf += IEEE1905_ESP_LEN;
  }

  if (apMetric->include_bit_esp & IEEE1905_INCL_BIT_ESP_VI) {
    memcpy(pbuf, apMetric->esp_ac_vi, IEEE1905_ESP_LEN);
    /* Swap bytes 0 and 2 to conform to IEEE1905 Endianness */
    I5_SWAP(pbuf[0], pbuf[2], unsigned char);
    pbuf += IEEE1905_ESP_LEN;
  }

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvAPMetricsType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Ap Metrics TLV */
int i5TlvAPMetricsTypeExtract(i5_message_type *pmsg, unsigned char isCombined)
{
  i5_dm_device_type *pdevice = NULL;
  i5_dm_bss_type *pdmbss = NULL;
  unsigned char *pvalue, bssid[MAC_ADDR_LEN];
  unsigned int length, found = 0;
  int rc = 0;

  /* In the combined infrastructure metrics, we should find the device from network topology */
  if (!isCombined) {
    pdevice = i5DmDeviceFind(i5MessageSrcMacAddressGet(pmsg));
    if (pdevice == NULL) {
      i5TraceError("Neighbour device does not exist\n");
      rc = -1;
      goto end;
    }
  }

  i5MessageReset(pmsg);
  while ((rc = i5MessageTlvExtract(pmsg, i5TlvAPMetricsType, &length, &pvalue,
    i5MessageTlvExtractWithoutReset)) == 0) {
    int pos = 0;
    found = 1;
    if (length < i5TlvAPMetrics_Min_Length) {
      rc = -1;
      goto end;
    }
    memcpy(bssid, &pvalue[pos], MAC_ADDR_LEN);
    pos += MAC_ADDR_LEN;

    if (isCombined) {
      pdmbss = i5DmFindBSSFromNetwork(bssid);
    } else  {
      pdmbss = i5DmFindBSSFromDevice(pdevice, bssid);
    }
    if (pdmbss == NULL) {
      i5TraceError("BSSID " I5_MAC_DELIM_FMT " does not exist\n", I5_MAC_PRM(bssid));
      continue;
    }

    ((i5_dm_interface_type*)I5LL_PARENT(pdmbss))->ifrMetric.chan_util = pvalue[pos];
    pos++;
    pos += 2; /* For Number of STAs associated */
    pdmbss->APMetric.include_bit_esp = pvalue[pos];
    pos++;

    if (pdmbss->APMetric.include_bit_esp & IEEE1905_INCL_BIT_ESP_BE) {
      memcpy(pdmbss->APMetric.esp_ac_be, &pvalue[pos], IEEE1905_ESP_LEN);
      /* Swap bytes 0 and 2 to conform to IEEE802.11 Endianness */
      I5_SWAP(pdmbss->APMetric.esp_ac_be[0], pdmbss->APMetric.esp_ac_be[2], unsigned char);
      pos += IEEE1905_ESP_LEN;
    }

    if (pdmbss->APMetric.include_bit_esp & IEEE1905_INCL_BIT_ESP_BK) {
      if (length >= pos + IEEE1905_ESP_LEN) {
        memcpy(pdmbss->APMetric.esp_ac_bk, &pvalue[pos], IEEE1905_ESP_LEN);
        /* Swap bytes 0 and 2 to conform to IEEE802.11 Endianness */
        I5_SWAP(pdmbss->APMetric.esp_ac_bk[0], pdmbss->APMetric.esp_ac_bk[2], unsigned char);
        pos += IEEE1905_ESP_LEN;
      }
    }

    if (pdmbss->APMetric.include_bit_esp & IEEE1905_INCL_BIT_ESP_VO) {
      if (length >= pos + IEEE1905_ESP_LEN) {
        memcpy(pdmbss->APMetric.esp_ac_vo, &pvalue[pos], IEEE1905_ESP_LEN);
        /* Swap bytes 0 and 2 to conform to IEEE802.11 Endianness */
        I5_SWAP(pdmbss->APMetric.esp_ac_vo[0], pdmbss->APMetric.esp_ac_vo[2], unsigned char);
        pos += IEEE1905_ESP_LEN;
      }
    }

    if (pdmbss->APMetric.include_bit_esp & IEEE1905_INCL_BIT_ESP_VI) {
      if (length >= pos + IEEE1905_ESP_LEN) {
        memcpy(pdmbss->APMetric.esp_ac_vi, &pvalue[pos], IEEE1905_ESP_LEN);
        /* Swap bytes 0 and 2 to conform to IEEE802.11 Endianness */
        I5_SWAP(pdmbss->APMetric.esp_ac_vi[0], pdmbss->APMetric.esp_ac_vi[2], unsigned char);
        pos += IEEE1905_ESP_LEN;
      }
    }
  }
  if (!found) {
    rc = -1;
  }
end:
  return rc;
}

/* TLV to add Associated STA Traffic Stats TLV */
int i5TlvAssociatedSTATrafficStatsTypeInsert(i5_message_type *pmsg, unsigned char *mac,
  ieee1905_sta_traffic_stats *traffic_stats)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  memcpy(pbuf, mac, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  *((unsigned int *)pbuf) = htonl(traffic_stats->bytes_sent);
  pbuf += 4;

  *((unsigned int *)pbuf) = htonl(traffic_stats->bytes_recv);
  pbuf += 4;

  *((unsigned int *)pbuf) = htonl(traffic_stats->packets_sent);
  pbuf += 4;

  *((unsigned int *)pbuf) = htonl(traffic_stats->packets_recv);
  pbuf += 4;

  *((unsigned int *)pbuf) = htonl(traffic_stats->tx_packet_err);
  pbuf += 4;

  *((unsigned int *)pbuf) = htonl(traffic_stats->rx_packet_err);
  pbuf += 4;

  *((unsigned int *)pbuf) = htonl(traffic_stats->retransmission_count);
  pbuf += 4;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvAssociatedSTATrafficStatsType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Associated STA Traffic Stats TLV */
int i5TlvAssociatedSTATrafficStatsTypeExtract(i5_message_type *pmsg)
{
  i5_dm_device_type *pdevice;
  i5_dm_clients_type *pdmclient;
  unsigned char *pvalue, mac[MAC_ADDR_LEN];
  unsigned int length, found = 0;
  int rc = 0;

  pdevice = i5DmDeviceFind(i5MessageSrcMacAddressGet(pmsg));
  if (pdevice == NULL) {
    i5TraceError("Neighbour device does not exist\n");
    rc = -1;
    goto end;
  }

  i5MessageReset(pmsg);
  while ((rc = i5MessageTlvExtract(pmsg, i5TlvAssociatedSTATrafficStatsType, &length, &pvalue,
    i5MessageTlvExtractWithoutReset)) == 0) {
    int pos = 0;
    found = 1;
    if (length < i5TlvAssociatedSTATrafficStats_Min_Length) {
      rc = -1;
      goto end;
    }
    memcpy(mac, &pvalue[pos], MAC_ADDR_LEN);
    pos += MAC_ADDR_LEN;

    pdmclient = i5DmFindClientInDevice(pdevice, mac);
    if (pdmclient == NULL) {
      i5TraceError("STA " I5_MAC_DELIM_FMT " does not exist in device " I5_MAC_DELIM_FMT "\n",
        I5_MAC_PRM(mac), I5_MAC_PRM(pdevice->DeviceId));
      continue;
    }

    memcpy(&pdmclient->old_traffic_stats, &pdmclient->traffic_stats,
      sizeof(pdmclient->old_traffic_stats));

    if (length < pos + 4)
      continue;
    pdmclient->traffic_stats.bytes_sent = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    if (length < pos + 4)
      continue;
    pdmclient->traffic_stats.bytes_recv = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    if (length < pos + 4)
      continue;
    pdmclient->traffic_stats.packets_sent = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    if (length < pos + 4)
      continue;
    pdmclient->traffic_stats.packets_recv = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    if (length < pos + 4)
      continue;
    pdmclient->traffic_stats.tx_packet_err = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    if (length < pos + 4)
      continue;
    pdmclient->traffic_stats.rx_packet_err = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    if (length < pos + 4)
      continue;
    pdmclient->traffic_stats.retransmission_count = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;
  }
  if (!found) {
    rc = -1;
  }
end:
  return rc;
}

/* TLV to add STA MAC Address Type TLV */
int i5TlvSTAMACAddressTypeInsert(i5_message_type *pmsg, unsigned char *mac)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvClientAssociationEvent_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvSTAMACAddressType;
  ptlv->length = htons(i5TlvSTAMACAddress_Length);
  len += sizeof(i5_tlv_t);

  memcpy(&buf[len], mac, MAC_ADDR_LEN);
  len +=  MAC_ADDR_LEN;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract STA MAC Address Type TLV */
int i5TlvSTAMACAddressTypeExtract(i5_message_type *pmsg, unsigned char *mac)
{
  int rc = 0;
  unsigned char *pvalue;
  unsigned int length;

  rc = i5MessageTlvExtract(pmsg, i5TlvSTAMACAddressType, &length, &pvalue,
    i5MessageTlvExtractWithReset);
  if (rc != 0 && length < i5TlvSTAMACAddress_Length) {
    rc = -1;
    goto end;
  }

  memcpy(mac, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;

end:
  return rc;
}

/* TLV to add Associated STA Link Metrics TLV */
int i5TlvAssociatedSTALinkMetricsTypeInsert(i5_message_type *pmsg, unsigned char *mac,
  unsigned char *bssid, ieee1905_sta_link_metric *link_metric)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;
  struct timespec updateTime;
  long deltaTime = 0;

  if (link_metric) {
    clock_gettime(CLOCK_REALTIME, &updateTime);
    deltaTime = getDeltaTimeInMs(&link_metric->queried, &updateTime);
    link_metric->delta = (unsigned int)deltaTime;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  memcpy(pbuf, mac, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  if (bssid != NULL) {
    *pbuf = (unsigned char)1;
    pbuf++;

    memcpy(pbuf, bssid, MAC_ADDR_LEN);
    pbuf += MAC_ADDR_LEN;

    *((unsigned int *)pbuf) = htonl(link_metric->delta);
    pbuf += 4;

    *((unsigned int *)pbuf) = htonl(link_metric->downlink_rate);
    pbuf += 4;

    *((unsigned int *)pbuf) = htonl(link_metric->uplink_rate);
    pbuf += 4;

    *pbuf = link_metric->rcpi;
    pbuf++;
  } else {
    *pbuf = 0;
    pbuf++;
  }

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvAssociatedSTALinkMetricsType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Associated STA Link Metrics TLV */
int i5TlvAssociatedSTALinkMetricsTypeExtract(i5_message_type *pmsg, int *sta_found)
{
  i5_dm_device_type *pdevice;
  i5_dm_bss_type *pdmbss;
  i5_dm_clients_type *pdmclient;
  unsigned char *pvalue, mac[MAC_ADDR_LEN], bssid[MAC_ADDR_LEN];
  unsigned int length, found = 0;
  int rc = 0;

  pdevice = i5DmDeviceFind(i5MessageSrcMacAddressGet(pmsg));
  if (pdevice == NULL) {
    i5TraceError("Neighbour device does not exist\n");
    rc = -1;
    goto end;
  }

  i5MessageReset(pmsg);
  while ((rc = i5MessageTlvExtract(pmsg, i5TlvAssociatedSTALinkMetricsType, &length, &pvalue,
    i5MessageTlvExtractWithoutReset)) == 0) {
    int pos = 0;
    unsigned char bss_count = 0;
    found = 1;
    if (length < i5TlvAssociatedSTALinkMetric_Min_Length) {
      rc = -1;
      goto end;
    }
    memcpy(mac, &pvalue[pos], MAC_ADDR_LEN);
    pos += MAC_ADDR_LEN;

    bss_count = pvalue[pos];
    pos++;
    if (bss_count == 0)
      continue;

    if (length < pos + i5TlvAssociatedSTALinkMetric_BSS_Length)
      continue;

    *sta_found = 1;
    memcpy(bssid, &pvalue[pos], MAC_ADDR_LEN);
    pos += MAC_ADDR_LEN;

    if ((pdmbss = i5DmFindBSSFromDevice(pdevice, bssid)) == NULL) {
      i5TraceError("BSS " I5_MAC_DELIM_FMT " does not exist in device " I5_MAC_DELIM_FMT "\n",
        I5_MAC_PRM(bssid), I5_MAC_PRM(pdevice->DeviceId));
      continue;
    }

    pdmclient = i5DmFindClientInBSS(pdmbss, mac);
    if (pdmclient == NULL) {
      i5TraceError("STA " I5_MAC_DELIM_FMT " does not exist in device " I5_MAC_DELIM_FMT " In BSS "
        I5_MAC_DELIM_FMT "\n",
        I5_MAC_PRM(mac), I5_MAC_PRM(pdevice->DeviceId), I5_MAC_PRM(bssid));
      continue;
    }

    pdmclient->link_metric.delta = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    pdmclient->link_metric.downlink_rate = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    pdmclient->link_metric.uplink_rate = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    pdmclient->link_metric.rcpi = pvalue[pos];
    pos++;

    if (i5_config.cbs.assoc_sta_metric_resp) {
      i5_config.cbs.assoc_sta_metric_resp(i5MessageSrcMacAddressGet(pmsg), bssid, mac,
        &pdmclient->link_metric);
    }
  }
  if (!found) {
    rc = -1;
  }
end:
  return rc;
}

/* TLV to add UnAssociated STA Link Metrics Query TLV */
int i5TlvUnAssociatedSTALinkMetricsQueryTypeInsert(i5_message_type *pmsg,
  ieee1905_unassoc_sta_link_metric_query *query)
{
  unsigned char *pbuf, *pmem;
  unassoc_query_per_chan_rqst *buf = NULL;
  int i = 0;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  *pbuf = query->opClass;
  pbuf++;

  *pbuf = query->chCount;
  pbuf++;

  for(i = 0; i < query->chCount; i++) {
    buf = (unassoc_query_per_chan_rqst *)pbuf;
    buf->chan = query->data[i].chan;
    buf->n_sta = query->data[i].n_sta;
    memcpy(&(buf->mac_list), query->data[i].mac_list, (buf->n_sta * MAC_ADDR_LEN));
    pbuf += ((buf->n_sta * MAC_ADDR_LEN) + 2);
  }

  i5Debug(" %zu bytes read in Unassoc link metric\n", pbuf-pmem-sizeof(i5_tlv_t));
  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvUnAssociatedSTALinkMetricsQueryType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, (pbuf-pmem));

  free(pmem);
  return (rc);
}

/* Extract UnAssociated STA Link Metrics Query TLV */
int i5TlvUnAssociatedSTALinkMetricsQueryTypeExtract(i5_message_type *pmsg,
  ieee1905_unassoc_sta_link_metric_query **query)
{
  int rc = 0;
  unsigned char *pvalue;
  unsigned int length = 0;
  ieee1905_unassoc_sta_link_metric_query *pmetric_query = NULL;
  unassoc_query_per_chan_rqst *per_chan_rqst = NULL;
  unassoc_query_per_chan_rqst *prqst_in_msg = NULL;
  int bytes_rd = 0;
  int i = 0;

  rc = i5MessageTlvExtract(pmsg, i5TlvUnAssociatedSTALinkMetricsQueryType, &length, &pvalue,
    i5MessageTlvExtractWithReset);
  if (rc != 0 && length < 2) {
    i5TraceInfo(" Error in extracting Unassoc link metric query info, rc = %d, len = %d \n",rc, length);
    rc = -1;
    goto end;
  }
  pmetric_query = (ieee1905_unassoc_sta_link_metric_query*)malloc(sizeof(*pmetric_query));
  if (!pmetric_query) {
    i5TraceDirPrint("Memory malloc failed \n");
    rc = -1;
    goto end;
  }
  memset(pmetric_query, 0, sizeof(*pmetric_query));

  pmetric_query->opClass = pvalue[0];
  pmetric_query->chCount = pvalue[1];
  i5TraceInfo("opclass = %d , chCount = %d\n",pmetric_query->opClass, pmetric_query->chCount);
  if (!(pmetric_query->chCount)) {
    rc = -1;
    goto end;
  }
  pmetric_query->data = (unassoc_query_per_chan_rqst*)malloc(sizeof(unassoc_query_per_chan_rqst) * pmetric_query->chCount);
  if (!pmetric_query->data) {
    i5TraceDirPrint("Memory malloc failed \n");
    rc = -1;
    goto end;
  }
  memset(pmetric_query->data, 0, sizeof(*(pmetric_query->data)));

  per_chan_rqst = pmetric_query->data;
  bytes_rd = 2; /* points at data after opClass and chCount in received message */
  for(i = 0; i < pmetric_query->chCount; i++) {
    prqst_in_msg = (unassoc_query_per_chan_rqst *)&pvalue[bytes_rd];
    per_chan_rqst[i].chan = prqst_in_msg->chan;
    per_chan_rqst[i].n_sta = prqst_in_msg->n_sta;
    i5TraceInfo("chan is %d , n_sta = %d \n", per_chan_rqst[i].chan, per_chan_rqst[i].n_sta);
    if (per_chan_rqst[i].n_sta) {
      per_chan_rqst[i].mac_list = (unsigned char *)malloc(per_chan_rqst[i].n_sta * MAC_ADDR_LEN);
      if (!per_chan_rqst[i].mac_list) {
        i5TraceDirPrint("Memory malloc failed \n");
        goto end;
      }
      memcpy(per_chan_rqst[i].mac_list, &(prqst_in_msg->mac_list), (per_chan_rqst[i].n_sta * MAC_ADDR_LEN));
   }
   bytes_rd += ((per_chan_rqst[i].n_sta * MAC_ADDR_LEN) + 2);
 }
end:
  if (pmetric_query) {
    *query = pmetric_query;
  }
  return rc;
}

/* TLV to add UnAssociated STA Link Metrics Response TLV */
int i5TlvUnAssociatedSTALinkMetricsResponseTypeInsert(i5_message_type *pmsg,
  ieee1905_unassoc_sta_link_metric *metric)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;
  struct timespec updateTime;
  long deltaTime = 0;
  dll_t *item_p, *next_p;
  ieee1905_unassoc_sta_link_metric_list *staInfo;

  clock_gettime(CLOCK_REALTIME, &updateTime);

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  *pbuf = metric->opClass;
  pbuf++;
  *pbuf = (unsigned char)metric->sta_list.count;
  pbuf++;

  for (item_p = dll_head_p(&metric->sta_list.head);
    !dll_end(&metric->sta_list.head, item_p);
    item_p = next_p) {
    next_p = dll_next_p(item_p);
    staInfo = (ieee1905_unassoc_sta_link_metric_list*)item_p;

    deltaTime = getDeltaTimeInMs(&staInfo->queried, &updateTime);
    staInfo->delta = (unsigned int)deltaTime;

    memcpy(pbuf, staInfo->mac, MAC_ADDR_LEN);
    pbuf += MAC_ADDR_LEN;
    *pbuf = staInfo->channel;
    pbuf++;
    *((unsigned int *)pbuf) = htonl(staInfo->delta);
    pbuf += 4;
    *pbuf = staInfo->rcpi;
    pbuf++;
  }

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvUnAssociatedSTALinkMetricsResponseType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract UnAssociated STA Link Metrics Response TLV */
int i5TlvUnAssociatedSTALinkMetricsResponseTypeExtract(i5_message_type *pmsg,
  ieee1905_unassoc_sta_link_metric *metric)
{
  int rc = 0, idx;
  unsigned char *pvalue;
  unsigned char macCount = 0;
  unsigned int length, pos = 0;
  ieee1905_unassoc_sta_link_metric_list *staInfo;

  rc = i5MessageTlvExtract(pmsg, i5TlvUnAssociatedSTALinkMetricsResponseType, &length, &pvalue,
    i5MessageTlvExtractWithReset);
  if (rc != 0 && length < 2) {
    rc = -1;
    goto end;
  }

  metric->opClass = pvalue[pos];
  pos++;

  macCount = pvalue[pos];
  pos++;

  if (macCount <= 0) {
    rc = -1;
    goto end;
  }

  for (idx = 0; (idx < macCount) && (length >= (pos + 12)); idx++) {
    staInfo = (ieee1905_unassoc_sta_link_metric_list*)malloc(sizeof(*staInfo));
    if (!staInfo) {
      i5TraceDirPrint("malloc error\n");
      rc = -1;
      goto end;
    }

    memcpy(staInfo->mac, &pvalue[pos], MAC_ADDR_LEN);
    pos += MAC_ADDR_LEN;
    staInfo->channel = pvalue[pos];
    pos++;
    staInfo->delta = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;
    staInfo->rcpi = pvalue[pos];
    pos++;
    ieee1905_glist_append(&metric->sta_list, (dll_t*)staInfo);
  }
  return 0;

end:
  return rc;
}

/* TLV to add Beacons metrics query TLV */
int i5TlvBeaconMetricsQueryTypeInsert(i5_message_type *pmsg, ieee1905_beacon_request *query)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  memcpy(pbuf, query->sta_mac, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  *pbuf = query->opclass;
  pbuf++;

  *pbuf = query->channel;
  pbuf++;

  memcpy(pbuf, query->bssid, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  *pbuf = query->reporting_detail;
  pbuf++;

  *pbuf = query->ssid.SSID_len;
  pbuf++;

  if (query->ssid.SSID_len > 0) {
    memcpy(pbuf, query->ssid.SSID, query->ssid.SSID_len);
    pbuf += query->ssid.SSID_len;
  }

  *pbuf = query->ap_chan_report_count;
  pbuf++;

  if (query->ap_chan_report_count > 0 && query->ap_chan_report) {
    memcpy(pbuf, query->ap_chan_report, query->ap_chan_report_len);
    pbuf += query->ap_chan_report_len;
  }

  *pbuf = query->element_ids_count;
  pbuf++;

  if (query->element_ids_count > 0 && query->element_list) {
    memcpy(pbuf, query->element_list, query->element_ids_count);
    pbuf += query->element_ids_count;
  }

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvBeaconMetricsQueryType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Beacons metrics query TLV */
int i5TlvBeaconMetricsQueryTypeExtract(i5_message_type *pmsg, ieee1905_beacon_request *query)
{
  int rc = 0;
  unsigned char *pvalue;
  unsigned char ap_chan_report_len = 0, i, size = 0;
  unsigned int length, pos = 0, tmp_pos = 0;

  rc = i5MessageTlvExtract(pmsg, i5TlvBeaconMetricsQueryType, &length, &pvalue,
    i5MessageTlvExtractWithReset);
  if (rc != 0 && length < i5TlvBeaconMetricsQuery_Min_Length) {
    rc = -1;
    goto end;
  }

  memcpy(query->sta_mac, &pvalue[pos], MAC_ADDR_LEN);
  pos += MAC_ADDR_LEN;

  query->opclass = pvalue[pos];
  pos++;

  query->channel = pvalue[pos];
  pos++;

  memcpy(query->bssid, &pvalue[pos], MAC_ADDR_LEN);
  pos += MAC_ADDR_LEN;

  query->reporting_detail = pvalue[pos];
  pos++;

  query->ssid.SSID_len = pvalue[pos];
  pos++;

  rc = -1; /* Make it failed. make it 0 in the end */
  if (length < pos + query->ssid.SSID_len)
    goto end;

  if (query->ssid.SSID_len > 0) {
    memcpy(query->ssid.SSID, &pvalue[pos], query->ssid.SSID_len);
    pos += query->ssid.SSID_len;
  }

  if (length < pos + 1)
    goto end;

  query->ap_chan_report_count = pvalue[pos];
  pos++;

  if (query->ap_chan_report_count > 0) {
    /* Find the length of the AP channel report */
    tmp_pos = pos;
    for (i = 0; i < query->ap_chan_report_count; i++) {
      if (length < tmp_pos + 1)
        goto end;
      ap_chan_report_len = pvalue[tmp_pos];
      size++;
      tmp_pos++;
      if (length < tmp_pos + ap_chan_report_len)
        goto end;
      size += ap_chan_report_len;
      tmp_pos += ap_chan_report_len;
    }

    query->ap_chan_report_len = size;
    query->ap_chan_report = (unsigned char*)malloc(size);
    if (query->ap_chan_report == NULL) {
      i5TraceDirPrint("Malloc failed for AP channel repot\n");
      goto end;
    }

    memcpy(query->ap_chan_report, &pvalue[pos], query->ap_chan_report_len);
    pos += query->ap_chan_report_len;
  }

  /* If there is no Reporting Detail, there wont be any element IDs */
  if (query->reporting_detail > 0) {
    if (length < pos + 1)
      goto end;
    query->element_ids_count = pvalue[pos];
    pos++;

    if (query->element_ids_count > 0 && length < pos + query->element_ids_count) {
      query->element_list = (unsigned char*)malloc(query->element_ids_count);
      if (query->element_list == NULL) {
        i5TraceDirPrint("Malloc failed for Element IDs List\n");
        goto end;
      }
    }
  }

  rc = 0;

end:
  return rc;
}

/* TLV to add Beacons metrics response TLV */
int i5TlvBeaconMetricsResponseTypeInsert(i5_message_type *pmsg, ieee1905_beacon_report *report)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0, min_tlv_len = 0;

  min_tlv_len = sizeof(i5_tlv_t) + i5TlvBeaconMetricsRespomse_Min_Length;
  if (min_tlv_len + report->report_element_len > I5_PACKET_BUF_LEN) {
    i5TraceDirPrint("Cannot accommodate %d octates in a TLV. Send error\n",
      (min_tlv_len + report->report_element_len));
    report->response = IEEE1905_BEACON_REPORT_RESP_FLAG_UNSPECIFIED;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  memcpy(pbuf, report->sta_mac, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  *pbuf = report->response;
  pbuf++;

  if (report->response == IEEE1905_BEACON_REPORT_RESP_FLAG_SUCCESS) {
    *pbuf = report->report_element_count;
    pbuf++;

    if (report->report_element_count > 0 && report->report_element) {
      memcpy(pbuf, report->report_element, report->report_element_len);
      pbuf += report->report_element_len;
    }
  } else {
    *pbuf = 0;
    pbuf++;
  }

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvBeaconMetricsResponseType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Beacons metrics response TLV */
int i5TlvBeaconMetricsResponseTypeExtract(i5_message_type *pmsg, ieee1905_beacon_report *report)
{
  int rc = 0;
  unsigned char *pvalue;
  unsigned int length, pos = 0;

  rc = i5MessageTlvExtract(pmsg, i5TlvBeaconMetricsResponseType, &length, &pvalue,
    i5MessageTlvExtractWithReset);
  if (rc != 0 && length < i5TlvBeaconMetricsRespomse_Min_Length) {
    rc = -1;
    goto end;
  }

  memcpy(report->sta_mac, &pvalue[pos], MAC_ADDR_LEN);
  pos += MAC_ADDR_LEN;

  report->response = pvalue[pos];
  pos++;

  report->report_element_count = pvalue[pos];
  pos++;

  if (report->report_element_count > 0) {
    report->report_element_len = length - pos;
    if (report->report_element_len > 0) {
      report->report_element = (unsigned char*)malloc(report->report_element_len);
      if (report->report_element == NULL) {
        i5TraceDirPrint("Malloc failed for beacon report TLVs\n");
        goto end;
      }

      memcpy(report->report_element, &pvalue[pos], report->report_element_len);
      pos += report->report_element_len;
    }
  }

end:
  return rc;
}

/* TLV to insert the Backhaul Steering Request */
int i5TlvBhSteeringRequestTypeInsert(i5_message_type *pmsg,
  ieee1905_backhaul_steer_msg *bh_steer_req)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvBhSteeringRequest_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvBackhaulSteeringRequestType;
  ptlv->length = htons(i5TlvBhSteeringRequest_Length);
  len += sizeof(i5_tlv_t);

  /* Backhaul STA MAC */
  memcpy(&buf[len], bh_steer_req->bh_sta_mac, MAC_ADDR_LEN);
  len += MAC_ADDR_LEN;

  /* Target BSSID */
  memcpy(&buf[len], bh_steer_req->trgt_bssid, MAC_ADDR_LEN);
  len += MAC_ADDR_LEN;

  /* Target BSS Operating Class */
  buf[len++] = bh_steer_req->opclass;

  /* Target BSS Channel Number */
  buf[len++] = bh_steer_req->channel;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Backhaul Steering Request TLV */
int i5TlvBhSteeringRequestTypeExtract(i5_message_type *pmsg, ieee1905_backhaul_steer_msg *bh_steer_req)
{
  unsigned char *pvalue;
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvBackhaulSteeringRequestType, &length, &pvalue,
    i5MessageTlvExtractWithReset) != 0) {
    i5TraceInfo("Backhaul steering request: Invalid tlv type\n");
    return -1;
  }

  if (length < i5TlvBhSteeringRequest_Length) {
    i5TraceInfo("Backhaul steering request: Invalid tlv length: %d\n", length);
    return -1;
  }

  memcpy(bh_steer_req->bh_sta_mac, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;
  memcpy(bh_steer_req->trgt_bssid, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;
  bh_steer_req->opclass = *pvalue++;
  bh_steer_req->channel = *pvalue;

  return 0;
}

/* TLV to insert the Backhaul Steering Response */
int i5TlvBhSteeringResponseTypeInsert(i5_message_type *pmsg,
  ieee1905_backhaul_steer_msg *bh_steer_resp)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvBhSteeringResponse_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvBackhaulSteeringResponseType;
  ptlv->length = htons(i5TlvBhSteeringResponse_Length);
  len += sizeof(i5_tlv_t);

  /* Backhaul STA MAC */
  memcpy(&buf[len], bh_steer_resp->bh_sta_mac, MAC_ADDR_LEN);
  len += MAC_ADDR_LEN;

  /* Target BSSID */
  memcpy(&buf[len], bh_steer_resp->trgt_bssid, MAC_ADDR_LEN);
  len += MAC_ADDR_LEN;

  /* Backhaul steering response status code  */
  buf[len++] = bh_steer_resp->resp_status_code;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Backhaul Steering Response TLV */
int i5TlvBhSteeringResponseTypeExtract(i5_message_type *pmsg,
  ieee1905_backhaul_steer_msg *bh_steer_resp)
{
  unsigned char *pvalue;
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvBackhaulSteeringResponseType, &length, &pvalue,
    i5MessageTlvExtractWithReset) != 0) {
    i5TraceInfo("Backhaul steering response: Invalid tlv type\n");
    return -1;
  }

  if (length < i5TlvBhSteeringResponse_Length) {
    i5TraceInfo("Backhaul steering response: Invalid tlv length: %d\n", length);
    return -1;
  }

  memcpy(bh_steer_resp->bh_sta_mac, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;
  memcpy(bh_steer_resp->trgt_bssid, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;
  bh_steer_resp->resp_status_code = *pvalue;

  return 0;
}

#if defined(MULTIAPR2)
/* Insert the Channel Scan Request TLV */
int i5TlvChannelScanRequestTypeInsert(i5_message_type *pmsg,
  ieee1905_chscan_req_msg *chscan_req)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;
  ieee1905_per_radio_opclass_list *radio_info = NULL;

  if (chscan_req == NULL) {
    i5TraceError("Invalid Arguments\n");
    goto error;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    goto error;
  }
  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  /* Insert Channel Scan Request TLV Flags */
  *pbuf = chscan_req->chscan_req_msg_flag;
  pbuf += sizeof(chscan_req->chscan_req_msg_flag);

  /* Insert Number of radios : upon which channel scans are requested */
  *pbuf = chscan_req->num_of_radios;
  pbuf += sizeof(chscan_req->num_of_radios);

  /* Check for valid Number of Radio */
  if (chscan_req->num_of_radios <= 0) {
    goto no_error;
  }

  /* Insert details for each Radio */
  foreach_iglist_item(radio_info, ieee1905_per_radio_opclass_list, chscan_req->radio_list) {

    ieee1905_per_opclass_chan_list *opclass_info = NULL;

    /* Insert Radio Unique Identifier of a radio of the Multi-AP Agent */
    memcpy(pbuf, radio_info->radio_mac, MAC_ADDR_LEN);
    pbuf += MAC_ADDR_LEN;

    /* Insert Number of Operating Classes */
    *pbuf = radio_info->num_of_opclass;
    pbuf += sizeof(radio_info->num_of_opclass);

    /* Check for valid Number of Operating Class */
    if (radio_info->num_of_opclass <= 0) {
      continue;
    }

    /* Insert details for each Operating Class */
    foreach_iglist_item(opclass_info, ieee1905_per_opclass_chan_list, radio_info->opclass_list) {

      /* Insert Operating Class Value */
      *pbuf = opclass_info->opclass_val;
      pbuf += sizeof(opclass_info->opclass_val);

      /* Insert Number of Channels specified in the Channel List */
      *pbuf = opclass_info->num_of_channels;
      pbuf += sizeof(opclass_info->num_of_channels);

      /* Insert Octets of Channels specified in the Channel List */
      memcpy(pbuf, opclass_info->chan_list, opclass_info->num_of_channels);
      pbuf += opclass_info->num_of_channels;

    } /* end for foreach_iglist_item(opclass_info, */

  } /* end for foreach_iglist_item(radio_info, */

no_error:
  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvChannelScanRequestType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  i5TraceInfo("Success : Insert Channel Scan Request TLV\n");
  if (pmem) {
    free(pmem);
  }
  return (rc);

error:
  i5TraceError("Failed : Insert Channel Scan Request TLV\n");
  if (pmem) {
    free(pmem);
  }
  return -1;
}

/* Extract the Channel Scan Request TLV */
int i5TlvChannelScanRequestTypeExtract(i5_message_type *pmsg,
  ieee1905_chscan_req_msg *chscan_req)
{
  i5_dm_device_type *pDeviceNeighbor, *self_device = i5_dm_network_topology.selfDevice;
  i5_dm_interface_type *pdmif;
  unsigned char *pvalue;
  unsigned int length, pos = 0, idx_radio = 0, idx_rclass = 0, idx_chan = 0;
  int rc = 0;
  ieee1905_per_radio_opclass_list *radio_info = NULL;
  ieee1905_per_opclass_chan_list *opclass_info = NULL;

  /* Initialize radio list */
  ieee1905_glist_init(&chscan_req->radio_list);

  pDeviceNeighbor = i5DmDeviceFind(i5MessageSrcMacAddressGet(pmsg));
  if (pDeviceNeighbor == NULL) {
    i5TraceError("Neighbour device does not exist\n");
    goto error;
  }

  rc = i5MessageTlvExtract(pmsg, i5TlvChannelScanRequestType, &length, &pvalue,
    i5MessageTlvExtractWithReset);

  /* Validate the length */
  if (rc != 0) {
    goto error;
  }

  /* If TLV size is not greate or equal to 2, this TLV is corrupted */
  if (length < i5TlvChannelScanRequest_Min_Length) {
    i5TraceInfo("Channel Scan Request: Invalid tlv length: %d\n", length);
    goto error;
  }

  /* Extract Channel Scan Request TLV Flags */
  chscan_req->chscan_req_msg_flag = pvalue[pos];
  pos += sizeof(chscan_req->chscan_req_msg_flag);

  /* Extract Number of radios : upon which channel scans are requested */
  chscan_req->num_of_radios = pvalue[pos];
  pos += sizeof(chscan_req->num_of_radios);

  /* Check for valid Number of Radio */
  if (chscan_req->num_of_radios <= 0) {
    goto no_error;
  }

  /* Extract details for each Radio */
  for (idx_radio = 0; idx_radio < chscan_req->num_of_radios; idx_radio++) {

    /* Allocate per_radio_opclass Info structure */
    radio_info = (ieee1905_per_radio_opclass_list *)malloc(sizeof(*radio_info));
    if (!radio_info) {
      i5TraceDirPrint("malloc error\n");
      goto error;
    }
    memset(radio_info, 0, sizeof(*radio_info));

    /* Initialize opclass list */
    ieee1905_glist_init(&radio_info->opclass_list);

    /* Check for Radio MAC + Number of Operating Classes Octects */
    if (length < (pos + MAC_ADDR_LEN + sizeof(radio_info->num_of_opclass))) {
      goto error;
    }

    /* Extract Radio Unique Identifier of a radio of the Multi-AP Agent */
    memcpy(radio_info->radio_mac, &(pvalue[pos]), MAC_ADDR_LEN);
    pos += MAC_ADDR_LEN;

    /* Check if Radio is available in Multi-AP Agent topology */
    pdmif = i5DmInterfaceFind(self_device, radio_info->radio_mac);
    if (pdmif == NULL) {
      i5TraceError("Neighbour Interface " I5_MAC_DELIM_FMT " does not exist\n", I5_MAC_PRM(radio_info->radio_mac));
    }
    radio_info->supported = 1;

    /* Extract Number of Operating Classes */
    radio_info->num_of_opclass = pvalue[pos];
    pos += sizeof(radio_info->num_of_opclass);

    /* Check If Fresh Scan Not Requied, Number of Operating Classes should be = 0 */
    if (!(chscan_req->chscan_req_msg_flag & MAP_CHSCAN_REQ_FRESH_SCAN) &&
      (radio_info->num_of_opclass != 0)) {

      i5TraceError("If not Fresh Scan, Number of Operating Classes should be = 0.\n");
      goto error;

    /* Check If Fresh Scan is Requied, Number of Operating Classes should be > 0 */
    } else if ((chscan_req->chscan_req_msg_flag & MAP_CHSCAN_REQ_FRESH_SCAN) &&
      !(radio_info->num_of_opclass > 0)) {

      i5TraceError("If Fresh Scan, Number of Operating Classes should be > 0.\n");
      goto error;
    }

    /* Extract details for each Operating Class */
    for (idx_rclass = 0; idx_rclass < radio_info->num_of_opclass; idx_rclass++) {

      /* Allocate per_opclass_chan Info structure */
       opclass_info = (ieee1905_per_opclass_chan_list *)malloc(sizeof(*opclass_info));
      if (!opclass_info) {
	i5TraceDirPrint("malloc error\n");
	goto error;
      }
      memset(opclass_info, 0, sizeof(*opclass_info));

      /* Check for Operating Class Octect */
      if (length < pos + sizeof(opclass_info->opclass_val)) {
        i5TraceError("Operating Class field is not present.\n");
        goto error;
      }

      /* Extract Operating Class Value */
      opclass_info->opclass_val = pvalue[pos];
      pos += sizeof(opclass_info->opclass_val);

      /* Find Operating Class in Channel Scan Capability, If not found, Operating Class UnSupported */
      opclass_info->supported = i5DmISOperatingClassSupported(pdmif, opclass_info->opclass_val);

      /* Check for Number of Channels Octect */
      if (length < pos + sizeof(opclass_info->num_of_channels)) {
        i5TraceError("Number of Channels field is not present.\n");
        goto error;
      }

      /* Extract Number of Channels specified in the Channel List */
      opclass_info->num_of_channels = pvalue[pos];
      pos += sizeof(opclass_info->num_of_channels);

      /* Check for Channels Octects */
      if (length < (pos + opclass_info->num_of_channels)) {
        i5TraceError("Channels Octects field is not present.\n");
        goto error;
      }

      /* Extract Channel List : Each octet describes a single channel number in the Operating Class
          * on which the Agent is capable of performing a scan. An empty Channel List field (k=0)
          * indicates that the Agent is capable of scanning on all channels in the Operating Class.
          */
      for (idx_chan = 0; idx_chan < opclass_info->num_of_channels; idx_chan++) {

	/* Extract Channel Value */
        opclass_info->chan_list[idx_chan] = pvalue[pos];
        pos += sizeof(opclass_info->chan_list[0]);

        /* If Current Operating Class is Supported, IF not All Channels of this OpClass are UnSupported */
        if (opclass_info->supported) {
          /* Find Channel in Channel Scan Capability, If not found, Channel UnSupported */
          opclass_info->supported_chan_list[idx_chan] = i5DmISChannelSupported(pdmif,
            opclass_info->opclass_val, opclass_info->chan_list[idx_chan]);
        }
      }

      ieee1905_glist_append(&radio_info->opclass_list, (dll_t*)opclass_info);

    } /* end for idx_rclass = 0; */

    ieee1905_glist_append(&chscan_req->radio_list, (dll_t*)radio_info);

  } /* end for idx_radio = 0; */

no_error:
  i5TraceInfo("Success : Extract Channel Scan Request TLV\n");
  return 0;

error:
  i5TraceError("Failed : Extract Channel Scan Request TLV\n");
  i5DmChannelScanRequestInfoFree(chscan_req);
  return -1;
}

/* Insert all the Channel Scan Result TLVs */
int i5TlvChannelScanResultTypeInsert(i5_message_type *pmsg,
  ieee1905_chscan_report_msg *chscan_rpt)
{
  int rc = 0, iter_results = 0;
  unsigned char *pbuf = NULL, *pmem = NULL;
  i5_tlv_t *ptlv;
  ieee1905_chscan_result_item *emt_p = NULL;
  ieee1905_chscan_result_nbr_item *emt_nbr_p = NULL, emt_nbr;
  unsigned int pkt_len = 0, nbr_len = MAX_CHSCAN_RESULT_NBR_SIZE((emt_nbr));
  bool neighbors_pending = FALSE;
  unsigned short *tmpPtrLoc, iter_neighbor = 0, tot_nbrs_in_this_tlv = 0, tot_nbrs_done = 0;

  if (chscan_rpt == NULL || chscan_rpt->num_of_results <= 0) {
    i5TraceError("Invalid Arguments\n");
    goto error;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    goto error;
  }

  /* Go Through Item of Channel Scan Result List */
  foreach_iglist_item(emt_p, ieee1905_chscan_result_item, chscan_rpt->chscan_result_list) {

    neighbors_pending = FALSE;

    iter_neighbor = 0, tot_nbrs_done = 0;

    iter_results++;

create_new_tlv_of_same_result:

    tot_nbrs_in_this_tlv = 0;

    memset(pmem, 0, I5_PACKET_BUF_LEN);

    pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

    /* Insert Radio Unique Identifier of a radio of the Multi-AP Agent */
    memcpy(pbuf, emt_p->radio_mac, sizeof(emt_p->radio_mac));
    pbuf += sizeof(emt_p->radio_mac);

    /* Insert Operating Class Value */
    *pbuf = emt_p->opclass;
    pbuf += sizeof(emt_p->opclass);

    /* Insert Channel Value */
    *pbuf = emt_p->channel;
    pbuf += sizeof(emt_p->channel);

    /* Insert Scan Status Code */
    *pbuf = emt_p->scan_status_code;
    pbuf += sizeof(emt_p->scan_status_code);

    i5TraceInfo("Channel Scan Result[%d]: radio_mac[" I5_MAC_DELIM_FMT "] opclass[%d] "
      "channel[%d] scan_status_code[%d] Appended\n", iter_results,
      I5_MAC_PRM(emt_p->radio_mac), emt_p->opclass, emt_p->channel, emt_p->scan_status_code);

    /* Check for Scan Status Code Success & Following Fields presence */
    if (emt_p->scan_status_code != MAP_CHSCAN_STATUS_SUCCESS) {
      goto insert_tlv;
    }

    /* Insert Timestamp Length */
    *pbuf = emt_p->timestamp_length;
    pbuf += sizeof(emt_p->timestamp_length);

    /* Insert Timestamp Octets */
    if (emt_p->timestamp_length > 0) {
      memcpy(pbuf, emt_p->timestamp, emt_p->timestamp_length);
      pbuf += emt_p->timestamp_length;
    }

    /* Insert Utilization */
    *pbuf = emt_p->utilization;
    pbuf += sizeof(emt_p->utilization);

    /* Insert Noise */
    *pbuf = emt_p->noise;
    pbuf += sizeof(emt_p->noise);

    /* Insert Number Of Neighbors inserted in this TLV - At the End */
    tmpPtrLoc = ((unsigned short *)pbuf);
    pbuf += sizeof(emt_p->num_of_neighbors);

    /* Insert details for each Neighbor */
    foreach_iglist_item(emt_nbr_p, ieee1905_chscan_result_nbr_item, emt_p->neighbor_list) {

      /* Skip the neighbors, which are already Inserted in Prev TLVs */
      if (tot_nbrs_done > 0) {
        tot_nbrs_done--;
	continue;
      }

      /* Check if Remaining Packet Size, is sufficient for next Neighbor */
      pkt_len = ((pbuf - pmem) + nbr_len + sizeof(emt_p->aggregate_scan_duration) +
	      sizeof(emt_p->chscan_result_flag));

      if (pkt_len >= I5_PACKET_BUF_LEN) {

        i5TraceInfo("If we add neighbor of len[%u] on the packet fof len[%d] "
          "the length[%u] exceeds ethernet frame length[%d]\n", nbr_len, (int)(pbuf-pmem),
          pkt_len, I5_PACKET_BUF_LEN);

        /* Remaining Packet Size, is not sufficient for next Neighbor, Insert this TLV,
               * and Create next TLV with same Result for pending Neighbors
               */
	neighbors_pending = TRUE;

        goto cannot_hv_more_nbr_in_tlv;
      }

      i5TraceInfo("Current TLV len %zu\n", pbuf-pmem-sizeof(i5_tlv_t));

      iter_neighbor++;
      tot_nbrs_in_this_tlv++;

      /* Insert BSSID indicated by the Neighboring BSS */
      memcpy(pbuf, emt_nbr_p->nbr_bssid, sizeof(emt_nbr_p->nbr_bssid));
      pbuf += sizeof(emt_nbr_p->nbr_bssid);

      /* Insert Length of SSID of the Neighboring BSS */
      *pbuf = emt_nbr_p->nbr_ssid.SSID_len;
      pbuf += sizeof(emt_nbr_p->nbr_ssid.SSID_len);

      /* Insert SSID of the Neighboring BSS */
      if (emt_nbr_p->nbr_ssid.SSID_len > 0) {
        memcpy(pbuf, emt_nbr_p->nbr_ssid.SSID, emt_nbr_p->nbr_ssid.SSID_len);
        pbuf += emt_nbr_p->nbr_ssid.SSID_len;
      }

      /* Insert Neighboring RSSI */
      *pbuf = emt_nbr_p->nbr_rcpi;
      pbuf += sizeof(emt_nbr_p->nbr_rcpi);

      /* Insert Length of Channel Bandwidth field */
      *pbuf = emt_nbr_p->ch_bw_length;
      pbuf += sizeof(emt_nbr_p->ch_bw_length);

      /* Insert ChannelBandwidth Octets */
      if (emt_nbr_p->ch_bw_length > 0) {
        memcpy(pbuf, emt_nbr_p->ch_bw, emt_nbr_p->ch_bw_length);
        pbuf += emt_nbr_p->ch_bw_length;
      }

      /* Insert Channel Scan Result TLV Neighbor Flags */
      *pbuf = emt_nbr_p->chscan_result_nbr_flag;
      pbuf += sizeof(emt_nbr_p->chscan_result_nbr_flag);

      /* Insert ChannelUtilization & StationCount, if BSS Load Element Present. else it is omitted */
      if (emt_nbr_p->chscan_result_nbr_flag & MAP_CHSCAN_RES_NBR_BSSLOAD_PRESENT) {

        *pbuf = emt_nbr_p->channel_utilization;
        pbuf += sizeof(emt_nbr_p->channel_utilization);

        *((unsigned short *)pbuf) = htons(emt_nbr_p->station_count);
        pbuf += sizeof(emt_nbr_p->station_count);
      }

      i5TraceInfo("Channel Scan Result[%d]: Neighbor[%d] : BSSID["I5_MAC_DELIM_FMT"] "
        "Appended\n", iter_results, iter_neighbor, I5_MAC_PRM(emt_nbr_p->nbr_bssid));

      i5TraceInfo("Current TLV len %zu\n", pbuf-pmem-sizeof(i5_tlv_t));

    } /* end of foreach_iglist_item emt_nbr_p */

cannot_hv_more_nbr_in_tlv:
    /* Now fill Neighbors inserted in this TLV field */
    *tmpPtrLoc = htons(tot_nbrs_in_this_tlv);

    /* Insert AggregateScanDuration */
    *((unsigned int *)pbuf) = htonl(emt_p->aggregate_scan_duration);
    pbuf += sizeof(emt_p->aggregate_scan_duration);

    /* Insert Channel Scan Request TLV Flags */
    *pbuf = emt_p->chscan_result_flag;
    pbuf += sizeof(emt_p->chscan_result_flag);

insert_tlv:
    ptlv = (i5_tlv_t *)pmem;
    ptlv->type = i5TlvChannelScanResultType;
    ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

    rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

    /* Check if Neighbors are still pending in this Result */
    if (neighbors_pending == TRUE) {

      /* Reset the Flag */
      neighbors_pending = FALSE;

      /* Set How many Neighbors Inserted till now, those should be skipped in next TLV */
      tot_nbrs_done = iter_neighbor;

      /* If Neighbors are still pending in this Result, Go Create a new TLV with same Result */
      goto create_new_tlv_of_same_result;

    }

  } /* end of foreach_iglist_item emt_p */

  i5TraceInfo("Success : Insert Channel Scan Result TLV\n");
  if (pmem) {
    free(pmem);
  }
  return (rc);

error:
  i5TraceError("Failed : Insert Channel Scan Result TLV\n");
  if (pmem) {
    free(pmem);
  }
  return -1;

}

static int getYANGTimeStampFromString(time_t *out_time, char *buf)
{
  struct tm ts;
  memset(&ts, 0, sizeof(struct tm));
  strptime(buf, "%Y-%m-%dT%H:%M:%S.0Z%z", &ts);
  (*out_time) = mktime(&ts);
  return 1;
}

/* Extract all the Channel Scan Result TLVs */
int i5TlvChannelScanResultTypeExtract(i5_message_type *pmsg,
  ieee1905_chscan_report_msg *chscan_rpt)
{
  i5_dm_device_type *pdevice;
  unsigned char *pvalue;
  unsigned int length, pos = 0, idx_nbr = 0;
  ieee1905_chscan_result_item *emt_p = NULL;
  ieee1905_chscan_result_nbr_item *emt_nbr_p = NULL;
  unsigned char extrt_radio_mac[MAC_ADDR_LEN]; /* Extracted Radio mac address */
  unsigned char extrt_opclass; /* Extracted Operating Class */
  unsigned char extrt_channel; /* Extracted Channel Number */
  unsigned char extrt_nbr_bssid[MAC_ADDR_LEN]; /* Extracted Neighbor BSSID */
  unsigned short tot_nbrs_in_this_tlv = 0;

  /* Initialize Result list */
  chscan_rpt->num_of_results = 0;
  ieee1905_glist_init(&chscan_rpt->chscan_result_list);

  pdevice = i5DmDeviceFind(i5MessageSrcMacAddressGet(pmsg));
  if (pdevice == NULL) {
    i5TraceError("Neighbour device does not exist\n");
    goto error;
  }

  i5MessageReset(pmsg);
  while ((i5MessageTlvExtract(pmsg, i5TlvChannelScanResultType,
    &length, &pvalue, i5MessageTlvExtractWithoutReset)) == 0) {

    pos = 0;
    tot_nbrs_in_this_tlv = 0;

    /* If TLV size is not greate or equal to 9, this TLV is corrupted */
    if (length < i5TlvChannelScanResult_Min_Length) {
      i5TraceInfo("Channel Scan Result: Invalid tlv length: %d\n", length);
      goto error;
    }

    /* Extract Radio Unique Identifier of a radio of the Multi-AP Agent */
    memcpy(extrt_radio_mac, &(pvalue[pos]), sizeof(extrt_radio_mac));
    pos += sizeof(extrt_radio_mac);

    /* Extract Operating Class */
    extrt_opclass = pvalue[pos];
    pos += sizeof(extrt_opclass);

    /* Extract Channel */
    extrt_channel = pvalue[pos];
    pos += sizeof(extrt_channel);

    /* Find this Result Item In Existing Report */
    emt_p = i5DmFindChannelInScanResult(chscan_rpt, extrt_radio_mac, extrt_channel);

    /* If not found, Allocate & Append new chscan_result_item structure */
    if (!emt_p) {

      emt_p = (ieee1905_chscan_result_item *)malloc(sizeof(*emt_p));
      if (!emt_p) {
        i5TraceDirPrint("malloc error\n");
        goto error;
      }
      memset(emt_p, 0, sizeof(*emt_p));

      ieee1905_glist_append(&chscan_rpt->chscan_result_list, (dll_t*)emt_p);
      chscan_rpt->num_of_results++;

      /* Save Extracted (1) Radio MAC, (2) Operating Class, (3) Channel to new Result item */
      memcpy(emt_p->radio_mac, extrt_radio_mac, sizeof(emt_p->radio_mac));
      emt_p->opclass = extrt_opclass;
      emt_p->channel = extrt_channel;

      /* Initialize Neighbor list */
      emt_p->num_of_neighbors = 0;
      ieee1905_glist_init(&emt_p->neighbor_list);

    }

    /* Extract Scan Status Code */
    emt_p->scan_status_code = pvalue[pos];
    pos += sizeof(emt_p->scan_status_code);

    i5TraceInfo("Channel Scan Result[%d]: radio_mac[" I5_MAC_DELIM_FMT "] opclass[%d] "
      "channel[%d] scan_status_code[%d] Appended\n", chscan_rpt->num_of_results,
      I5_MAC_PRM(emt_p->radio_mac), emt_p->opclass, emt_p->channel, emt_p->scan_status_code);

    /* Check for Scan Status Code Success & Following Fields presence */
    if (emt_p->scan_status_code != MAP_CHSCAN_STATUS_SUCCESS) {
      continue;
    }

    /* Extract Timestamp Length */
    emt_p->timestamp_length = pvalue[pos];
    pos += sizeof(emt_p->timestamp_length);

    if (length < pos + emt_p->timestamp_length) {
      i5TraceError("Timestamp field is not present.\n");
      goto error;
    }

    /* Extract Timestamp Octets */
    if (emt_p->timestamp_length > 0) {
      memset(emt_p->timestamp, 0, sizeof(emt_p->timestamp));
      memcpy(emt_p->timestamp, &(pvalue[pos]), emt_p->timestamp_length);
      pos += emt_p->timestamp_length;
    }

    /* Get Current Timestamp */
    getYANGTimeStampFromString(&(emt_p->ts_scan_start), (char *)emt_p->timestamp);

    /* Extract Utilization */
    emt_p->utilization = pvalue[pos];
    pos += sizeof(emt_p->utilization);

    /* Extract Noise */
    emt_p->noise = pvalue[pos];
    pos += sizeof(emt_p->noise);

    /* Extract Number Of Neighbors - Append this count as Same Result TLV may come */
    tot_nbrs_in_this_tlv = ntohs(*((unsigned short *)&(pvalue[pos])));
    emt_p->num_of_neighbors += tot_nbrs_in_this_tlv;
    pos += sizeof(emt_p->num_of_neighbors);

    i5TraceInfo("Num_of_neighbors in this TLV[%d] Num_of_neighbors in this Result till now[%d]\n",
      tot_nbrs_in_this_tlv, emt_p->num_of_neighbors);

    /* Extract details for each Radio */
    for (idx_nbr = 0; idx_nbr < tot_nbrs_in_this_tlv; idx_nbr++) {

      /* Extract BSSID indicated by the Neighboring BSS */
      memcpy(extrt_nbr_bssid, &(pvalue[pos]), sizeof(extrt_nbr_bssid));
      pos += sizeof(extrt_nbr_bssid);

      /* Find this Neighbor Item In Existing Result */
      emt_nbr_p = i5DmFindBSSIDInScanResult(emt_p, extrt_nbr_bssid);

      /* If not found, Allocate & Append new chscan_result_nbr_item structure */
      if (!emt_nbr_p) {

        emt_nbr_p = (ieee1905_chscan_result_nbr_item *)malloc(sizeof(*emt_nbr_p));
        if (!emt_nbr_p) {
          i5TraceDirPrint("malloc error\n");
          goto error;
        }
        memset(emt_nbr_p, 0, sizeof(*emt_nbr_p));

        ieee1905_glist_append(&emt_p->neighbor_list, (dll_t*)emt_nbr_p);

	/* Save Extracted (1) Neighbor BSSID to new Neighbor item */
        memcpy(emt_nbr_p->nbr_bssid, extrt_nbr_bssid, sizeof(emt_nbr_p->nbr_bssid));

      }

      /* Extract Length of SSID of the Neighboring BSS */
      emt_nbr_p->nbr_ssid.SSID_len = pvalue[pos];
      pos += sizeof(emt_nbr_p->nbr_ssid.SSID_len);

      if ((emt_nbr_p->nbr_ssid.SSID_len > sizeof(emt_nbr_p->nbr_ssid.SSID)) ||
        (length < pos + emt_nbr_p->nbr_ssid.SSID_len)) {
        i5TraceError("SSID is too long or field is not present.\n");
        goto error;
      }

      /* Extract SSID of the Neighboring BSS */
      if (emt_nbr_p->nbr_ssid.SSID_len > 0) {
        memset(emt_nbr_p->nbr_ssid.SSID, 0, sizeof(emt_nbr_p->nbr_ssid.SSID));
        memcpy(emt_nbr_p->nbr_ssid.SSID, &(pvalue[pos]), emt_nbr_p->nbr_ssid.SSID_len);
        pos += emt_nbr_p->nbr_ssid.SSID_len;
      }

      /* Extract Neighboring RSSI */
      emt_nbr_p->nbr_rcpi = pvalue[pos];
      pos += sizeof(emt_nbr_p->nbr_rcpi);

      /* Extract Length of Channel Bandwidth field */
      emt_nbr_p->ch_bw_length = pvalue[pos];
      pos += sizeof(emt_nbr_p->ch_bw_length);

      if (length < pos + emt_nbr_p->ch_bw_length) {
        i5TraceError("Channel Bandwidth field is not present.\n");
        goto error;
      }

      /* Extract ChannelBandwidth Octets */
      if (emt_nbr_p->ch_bw_length > 0) {
        memset(emt_nbr_p->ch_bw, 0, sizeof(emt_nbr_p->ch_bw));
        memcpy(emt_nbr_p->ch_bw, &(pvalue[pos]), emt_nbr_p->ch_bw_length);
        pos += emt_nbr_p->ch_bw_length;
      }

      /* Extract Channel Scan  Result TLV Neighbor Flags */
      emt_nbr_p->chscan_result_nbr_flag = pvalue[pos];
      pos += sizeof(emt_nbr_p->chscan_result_nbr_flag);

      /* Extract ChannelUtilization & StationCount, if BSS Load Element Present. else it is omitted */
      if (emt_nbr_p->chscan_result_nbr_flag & MAP_CHSCAN_RES_NBR_BSSLOAD_PRESENT) {

        emt_nbr_p->channel_utilization = pvalue[pos];
        pos += sizeof(emt_nbr_p->channel_utilization);

        emt_nbr_p->station_count = ntohs(*((unsigned short *)&(pvalue[pos])));
        pos += sizeof(emt_nbr_p->station_count);
      }

      i5TraceInfo("Channel Scan Result[%d]: Neighbor[%d] : BSSID["I5_MAC_DELIM_FMT"] "
        "Appended\n", chscan_rpt->num_of_results, idx_nbr+1, I5_MAC_PRM(emt_nbr_p->nbr_bssid));

    } /* End of for idx_nbr */

    /* Extract AggregateScanDuration */
    emt_p->aggregate_scan_duration = ntohl(*((unsigned int *)&(pvalue[pos])));
    pos += sizeof(emt_p->aggregate_scan_duration);

    /* Extract Channel Scan Request TLV Flags */
    emt_p->chscan_result_flag = pvalue[pos];
    pos += sizeof(emt_p->chscan_result_flag);

  } /* End of while i5MessageTlvExtract */

  i5TraceInfo("Success : Extract Channel Scan Result TLV\n");
  return 0;

error:
  i5TraceError("Failed : Extract Channel Scan Result TLV\n");
  return -1;

}

/* Insert Timestamp TLV */
int i5TlvTimestampTypeInsert(i5_message_type *pmsg, time_t in_time)
{
  unsigned char buf[sizeof(i5_tlv_t) + IEEE1905_TS_MAX_LEN];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;
  unsigned char timestamp_len = 0;
  char timestamp[IEEE1905_TS_MAX_LEN];

  len += sizeof(i5_tlv_t);

  memset(timestamp, 0, sizeof(timestamp));
  timestamp_len = getYANGTimeStampToString(in_time, timestamp, sizeof(timestamp));
  i5TraceInfo("Timestamp TLV Value %s\n", timestamp);

  buf[len] = timestamp_len;
  len += sizeof(timestamp_len);

  memcpy(&buf[len], timestamp, timestamp_len);
  len += timestamp_len;

  ptlv->type = i5TlvTimestampType;
  ptlv->length = htons(len-sizeof(i5_tlv_t));

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Timestamp TLV */
int i5TlvTimestampTypeExtract(i5_message_type *pmsg, time_t *out_time)
{
  unsigned char *pvalue;
  unsigned int length;
  unsigned char timestamp_len = 0;
  char timestamp[IEEE1905_TS_MAX_LEN];

  if (i5MessageTlvExtract(pmsg, i5TlvTimestampType, &length, &pvalue,
    i5MessageTlvExtractWithReset) != 0) {
    i5TraceError("Timestamp TLV: Invalid tlv type\n");
    return -1;
  }

  timestamp_len = *pvalue;
  pvalue++;

  if (length < timestamp_len + sizeof(timestamp_len)) {
    i5TraceError("Timestamp TLV: Invalid tlv length: %d\n", length);
    return -1;
  }

  if (timestamp_len > 0) {
    memcpy(timestamp, pvalue, timestamp_len);
    pvalue += timestamp_len;
  }

  /* Get Current Timestamp */
  getYANGTimeStampFromString(out_time, timestamp);

  return 0;
}
#endif /* MULTIAPR2 */

/* TLV to insert the Error Codes */
int i5TlvErrorCodeTypeInsert(i5_message_type *pmsg, ieee1905_tlv_err_codes_t err,
  unsigned char *sta_mac)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvErrorCode_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvErrorCodeType;
  ptlv->length = htons(i5TlvErrorCode_Length);
  len += sizeof(i5_tlv_t);

  buf[len++] = err;

  if (sta_mac) {
    memcpy(&buf[len], sta_mac, MAC_ADDR_LEN);
    len += MAC_ADDR_LEN;
  }

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Error Codes */
int i5TlvErrorCodeTypeExtract(i5_message_type *pmsg, ieee1905_tlv_err_codes_t *err,
  unsigned char *sta_mac)
{
  unsigned char *pvalue;
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvErrorCodeType, &length, &pvalue,
    i5MessageTlvExtractWithReset) != 0) {
    i5TraceInfo("Error Code TLV: Invalid tlv type\n");
    return -1;
  }

  if (length < i5TlvErrorCode_Length) {
    i5TraceInfo("Error Code TLV: Invalid tlv length: %d\n", length);
    return -1;
  }

  *err = *pvalue;
  pvalue++;

  memcpy(sta_mac, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;

  return 0;
}

#if defined(MULTIAPR2)
/* TLV to Insert Reason Code info */
int i5TlvReasonCodeTypeInsert(i5_message_type *pmsg, unsigned short reason_code)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvReasonCode_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvReasonCodeType;
  ptlv->length = htons(i5TlvReasonCode_Length);
  len += sizeof(i5_tlv_t);
 *((unsigned short *)&buf[len]) = htons(reason_code);
  len += 2;
  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* TLV to Extract the Reason code info */
int i5TlvReasonCodeTypeExtract(i5_message_type *pmsg, unsigned short *reason_code)
{
  unsigned char *pvalue;
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvReasonCodeType, &length, &pvalue,
    i5MessageTlvExtractWithReset) == 0) {
    if (length >= i5TlvReasonCode_Length) {
     *reason_code = ntohs(*(unsigned short *)pvalue);
      i5TraceInfo(" Reason Code =  %d \n", *reason_code);
      return 0;
    }
  }
  return -1;
}

/* TLV to Insert MultiAP Profile */
int i5TlvMultiAPProfileInsert(i5_message_type *pmsg)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvMultiAPProfile_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  if (i5_config.map_profile < ieee1905_map_profile2) {
    i5Trace("MultiAP Profile[%d]. Do not insert\n", i5_config.map_profile);
    return 0;
  }

  i5Trace("\n");
  ptlv->type = i5TlvMultiAPProfileType;
  ptlv->length = htons(i5TlvMultiAPProfile_Length);
  len += sizeof(i5_tlv_t);

  buf[len] = (unsigned char)i5_config.map_profile;
  len++;

  i5Trace("MultiAP Profile[%d] \n", i5_config.map_profile);
  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract MultiAP Profile */
int i5TlvMultiAPProfileExtract(i5_message_type *pmsg, i5_dm_device_type *pdevice)
{
  unsigned char *pvalue;
  unsigned int length;

  i5Trace("\n");
  if (pdevice == NULL) {
    i5TraceError("Neighbor device["I5_MAC_DELIM_FMT"] does not exist\n",
      I5_MAC_PRM(i5MessageSrcMacAddressGet(pmsg)));
    goto end;
  }

  if (i5MessageTlvExtract(pmsg, i5TlvMultiAPProfileType, &length, &pvalue,
    i5MessageTlvExtractWithReset) == 0) {
    if (length >= i5TlvMultiAPProfile_Length) {
      if (pdevice) {
        pdevice->profile = *pvalue;
      }
      pvalue++;
    } else {
      goto end;
    }
  } else {
    goto end;
  }

  i5Trace("MultiAPProfile[%d]\n", pdevice->profile);
  return 0;

end:
  /* If the profile TLV doesnot exists, then it is profile-1 device */
  if (pdevice) {
    pdevice->profile = (unsigned char)ieee1905_map_profile1;
  }
  return -1;
}

/* TLV to Insert Profile2 AP Capability */
int i5TlvProfile2APCapabilityInsert(i5_message_type *pmsg, i5_dm_p2_ap_capability_type *p2ApCap)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvProfile2APCapability_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  i5Trace("\n");
  ptlv->type = i5TlvProfile2APCapabilityType;
  ptlv->length = htons(i5TlvProfile2APCapability_Length);
  len += sizeof(i5_tlv_t);

  *((unsigned short *)&buf[len]) = htons(p2ApCap->max_sp_rules);
  len += 2;
  buf[len++] = p2ApCap->byte_cntr_unit;
  buf[len++] = p2ApCap->max_vids;

  i5Trace("max_service_prio[%d] ByteCounterUnits[0x%01x] MaxTotalVIDs[%d]\n",
    p2ApCap->max_sp_rules, p2ApCap->byte_cntr_unit, p2ApCap->max_vids);
  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Profile-2 AP Capability */
int i5TlvProfile2APCapabilityExtract(i5_message_type *pmsg, i5_dm_device_type *pdevice)
{
  unsigned char *pvalue;
  unsigned int length;

  i5Trace("\n");
  if (pdevice == NULL) {
    i5TraceError("Neighbor device["I5_MAC_DELIM_FMT"] does not exist\n",
      I5_MAC_PRM(i5MessageSrcMacAddressGet(pmsg)));
    goto end;
  }

  if (i5MessageTlvExtract(pmsg, i5TlvProfile2APCapabilityType, &length, &pvalue,
    i5MessageTlvExtractWithReset) == 0) {
    if (length >= i5TlvProfile2APCapability_Length) {
      pdevice->p2ApCap.max_sp_rules = ntohs(*((unsigned short *)pvalue));
      pvalue += 2;
      pdevice->p2ApCap.byte_cntr_unit = *pvalue++;
      pdevice->p2ApCap.max_vids = *pvalue++;
    } else {
      goto end;
    }
  } else {
    goto end;
  }

  i5Trace("max_service_prio[%d] ByteCounterUnits[0x%01x] MaxTotalVIDs[%d]\n",
    pdevice->p2ApCap.max_sp_rules, pdevice->p2ApCap.byte_cntr_unit, pdevice->p2ApCap.max_vids);
  return 0;

end:
  return -1;
}

/* TLV to Insert Deafult 802.1Q Settings */
int i5TlvDefault8021QSettingsTypeInsert(i5_message_type *pmsg, ieee1905_policy_config *configlist)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvDefault8021QSettings_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  i5Trace("\n");

#ifdef MULTIAP_PLUGFEST
  if (i5_config.map_profile < ieee1905_map_profile2) {
    i5Trace("MultiAP Profile[%d]. Do not Insert\n", i5_config.map_profile);
    return 0;
  }
#endif /* MULTIAP_PLUGFEST */

  ptlv->type = i5TlvDefault8021QSettingsType;
  ptlv->length = htons(i5TlvDefault8021QSettings_Length);
  len += sizeof(i5_tlv_t);

  *((unsigned short *)&buf[len]) = htons(configlist->prim_vlan_id);
  len += 2;
  buf[len] = configlist->default_pcp << 5;
  len++;

  i5Trace("primary_vlan[%d] default_pcp[%d]\n", configlist->prim_vlan_id, configlist->default_pcp);
  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Default 802.1Q Settings */
int i5TlvDefault8021QSettingsTypeExtract(i5_message_type *pmsg)
{
  unsigned char *pvalue;
  unsigned int length;

  i5Trace("\n");

#ifdef MULTIAP_PLUGFEST
  if (i5_config.map_profile < ieee1905_map_profile2) {
    i5Trace("MultiAP Profile[%d]. Do not extract\n", i5_config.map_profile);
    return 0;
  }
#endif /* MULTIAP_PLUGFEST */

  if (i5MessageTlvExtract(pmsg, i5TlvDefault8021QSettingsType, &length, &pvalue,
    i5MessageTlvExtractWithReset) == 0) {
    if (length >= i5TlvDefault8021QSettings_Length) {
      i5_config.policyConfig.prim_vlan_id = ntohs(*((unsigned short *)pvalue));
      pvalue += 2;
      i5_config.policyConfig.default_pcp = *pvalue << 5;
      pvalue++;
    } else {
      goto end;
    }
  } else {
    goto end;
  }

  i5Trace("primary_vlan[%d] default_pcp[%d]\n", i5_config.policyConfig.prim_vlan_id,
    i5_config.policyConfig.default_pcp);
  return 0;

end:
  return -1;
}

/* TLV to Insert Traffic Separation Policy */
int i5TlvTrafficSeparationPolicyTypeInsert(i5_message_type *pmsg, ieee1905_policy_config *configlist)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  ieee1905_ts_policy_t *ts_policy;
  ieee1905_ssid_list_type *ssid_list;
  int rc = 0;
  unsigned char ssid_count = 0;
  dll_t *item_p;
  dll_t *ssid_item_p;

  i5Trace("\n");

#ifdef MULTIAP_PLUGFEST
  if (i5_config.map_profile < ieee1905_map_profile2) {
    i5Trace("MultiAP Profile[%d]. Do not Insert\n", i5_config.map_profile);
    return 0;
  }
#endif /* MULTIAP_PLUGFEST */

  if (!I5_IS_MULTIAP_CONTROLLER(i5_config.flags)) {
    /* send traffic separation policy only from controller */
    return 0;
  }

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t);

  /* Fill Number of SSIDs in the end */
  pbuf++;

  /* For All ssid's and corresponding vlan tags */
  for (item_p = dll_head_p(&configlist->ts_policy_list.head);
    !dll_end(&configlist->ts_policy_list.head, item_p);
    item_p = dll_next_p(item_p)) {
    ts_policy = (ieee1905_ts_policy_t *)item_p;

    for (ssid_item_p = dll_head_p(&ts_policy->ssid_list.head);
      !dll_end(&ts_policy->ssid_list.head, ssid_item_p);
      ssid_item_p = dll_next_p(ssid_item_p)) {
      ssid_list = (ieee1905_ssid_list_type *)ssid_item_p;
      *pbuf = (unsigned char)ssid_list->ssid.SSID_len;
      pbuf++;
      memcpy(pbuf, ssid_list->ssid.SSID, ssid_list->ssid.SSID_len);
      pbuf += ssid_list->ssid.SSID_len;
      *(unsigned short *)pbuf = htons(ts_policy->vlan_id);
      pbuf += 2;
      ssid_count++;
      i5Trace("ssid[%s] vlan_id[%d] ssid_count[%d]\n", ssid_list->ssid.SSID,
        ts_policy->vlan_id, ssid_count);
    }
  }

  /* Now fill the no. of tags */
  pmem[sizeof(i5_tlv_t)] = ssid_count;
  i5Trace("SSID count[%d]\n", pmem[sizeof(i5_tlv_t)]);

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvTrafficSeparationPolicyType;
  ptlv->length = htons(pbuf - pmem - sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf - pmem);

  free(pmem);
  return rc;
}

/* Extract Traffic Separation Policy */
int i5TlvTrafficSeparationPolicyTypeExtract(i5_message_type *pmsg)
{
  unsigned char *pvalue;
  unsigned int length, count;
  int rc = 0, idx = 0;
  unsigned short vlan_id = 0;
  ieee1905_ssid_type ssid;
  ieee1905_ts_policy_t *ts_policy = NULL;

  i5Trace("\n");

#ifdef MULTIAP_PLUGFEST
  if (i5_config.map_profile < ieee1905_map_profile2) {
    i5Trace("MultiAP Profile[%d]. Do not extract\n", i5_config.map_profile);
    return 0;
  }
#endif /* MULTIAP_PLUGFEST */

  rc = i5MessageTlvExtract(pmsg, i5TlvTrafficSeparationPolicyType, &length, &pvalue,
    i5MessageTlvExtractWithReset);
  if (rc < 0) {
    goto end;
  }

  if (length < i5TLvTrafficSeparationPolicy_Min_Length) {
    goto end;
  }

  count = *pvalue;
  pvalue++;
  i5Trace("Total SSIDs[%d]\n", count);

  /* If the traffic separation is already enabled and this TLV doesn't have any TS rules,
   * then delete first
   */
  if ((count == 0) && (i5_config.policyConfig.ts_policy_list.count > 0)) {
    i5TraceDirPrint("Remove all Traffic separation rules\n");
    i5GlueDeleteAllVlanInterfaces();
  }

  i5DmTSPolicyCleanup(&i5_config.policyConfig.ts_policy_list);
  ieee1905_glist_init(&i5_config.policyConfig.ts_policy_list);

  for (idx = 0; idx < count; idx++) {

    memset(&ssid, 0, sizeof(ssid));
    ssid.SSID_len = *pvalue;
    pvalue++;
    memcpy(ssid.SSID, pvalue, ssid.SSID_len);
    pvalue += ssid.SSID_len;

    vlan_id = ntohs(*((unsigned short *)pvalue));
    pvalue += 2;

    ts_policy = i5DmAddVLANIDToList(&i5_config.policyConfig.ts_policy_list, vlan_id);
    if (ts_policy == NULL) {
      goto end;
    }

    if (i5DmAddSSIDToList(&i5_config.policyConfig.ts_policy_list, &ts_policy->ssid_list,
      &ssid) == NULL) {
      goto end;
    }

    i5Trace("ssid[%s] vlan_id[%d] Total[%d]\n", ssid.SSID, ts_policy->vlan_id, count);
  }

  return 0;

end:
  return -1;

}

/* TLV to Insert Service Prioritization Rule */
int i5TlvServicePrioritizationRuleTypeInsert(i5_message_type *pmsg)
{
  return 0;
}

/* Extract Service Prioritization Rule */
int i5TlvServicePrioritizationRuleTypeExtract(i5_message_type *pmsg)
{
  return 0;
}

/* TLV to Insert R2 Error Code */
int i5TlvR2ErrorCodeTypeInsert(i5_message_type *pmsg)
{
  return 0;
}

/* Extract R2 Error Code */
int i5TlvR2ErrorCodeTypeExtract(i5_message_type *pmsg)
{
  return 0;
}

/* TLV to Insert AP Radio Advanced Capabilities */
int i5TlvAPRadioAdvancedCapabilitiesTypeInsert(i5_message_type *pmsg, i5_dm_interface_type *pdmif)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvAPRadioAdvancedCapabilities_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;
  unsigned char ts_flag = 0;

  i5Trace("\n");
  ptlv->type = i5TlvAPRadioAdvancedCapabilitiesType;
  ptlv->length = htons(i5TlvAPRadioAdvancedCapabilities_Length);
  len += sizeof(i5_tlv_t);

  memcpy(&buf[len], pdmif->InterfaceId, MAC_ADDR_LEN);
  len += MAC_ADDR_LEN;
  if (I5_IS_TS_MIX_FH_P1BH_SUPPORTED(pdmif->flags)) {
    ts_flag |= I5_TLV_TS_COMBINED_FH_P1BH_SUPPORT;
  }
  if (I5_IS_TS_MIX_P1BH_P2BH_SUPPORTED(pdmif->flags)) {
    ts_flag |= I5_TLV_TS_COMBINED_P1BH_P2BH_SUPPORT;
  }
  buf[len++] = (unsigned char)ts_flag;

  i5Trace("MAC "I5_MAC_DELIM_FMT" ts_flag[0x%01x]\n", I5_MAC_PRM(pdmif->InterfaceId), ts_flag);

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract AP Radio Advanced Capabilities */
int i5TlvAPRadioAdvancedCapabilitiesTypeExtract(i5_message_type *pmsg, i5_dm_device_type *pdevice)
{
  i5_dm_interface_type *pdmif;
  unsigned char *pvalue, mac[MAC_ADDR_LEN];
  unsigned int length = 0;
  unsigned char ts_flag = 0;

  i5Trace("\n");
  if (pdevice == NULL) {
    i5TraceError("Neighbor device["I5_MAC_DELIM_FMT"] does not exist\n",
      I5_MAC_PRM(i5MessageSrcMacAddressGet(pmsg)));
    goto end;
  }

  if (i5MessageTlvExtract(pmsg, i5TlvAPRadioAdvancedCapabilitiesType, &length, &pvalue,
    i5MessageTlvExtractWithReset) == 0) {
    if (length >= i5TlvAPRadioAdvancedCapabilities_Length) {
      memcpy(mac, pvalue, MAC_ADDR_LEN);
      pvalue += MAC_ADDR_LEN;
      pdmif = i5DmInterfaceFind(pdevice, mac);
      if (pdmif == NULL) {
        i5TraceError("Neighbour Interface " I5_MAC_DELIM_FMT " does not exist\n", I5_MAC_PRM(mac));
        goto end;
      }
      ts_flag = *pvalue;
      if (ts_flag & I5_TLV_TS_COMBINED_FH_P1BH_SUPPORT) {
        pdmif->flags |= I5_FLAG_IFR_TS_MIX_FH_P1BH_SUPPORTED;
      }
      if (ts_flag & I5_TLV_TS_COMBINED_P1BH_P2BH_SUPPORT) {
        pdmif->flags |= I5_FLAG_IFR_TS_MIX_P1BH_P2BH_SUPPORTED;
      }
    } else {
      goto end;
    }
  } else {
    goto end;
  }
  i5Trace("MAC "I5_MAC_DELIM_FMT" ts_flag[0x%01x]\n", I5_MAC_PRM(mac), ts_flag);

  return 0;

end:
  return -1;
}

/* Insert association status notification TLV */
int i5TlvAssociationStatusNotificationTypeInsert(i5_message_type *pmsg,
  ieee1905_association_status_notification *assoc_notif)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;
  unsigned char bss_count = 0;
  association_status_notification_bss *per_bss_assoc_status = NULL;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    printf("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  *pbuf++ = bss_count = assoc_notif->count;
  per_bss_assoc_status = (association_status_notification_bss *)pbuf;
  memcpy(per_bss_assoc_status, assoc_notif->list,
    bss_count * sizeof(*per_bss_assoc_status));

  pbuf += bss_count * sizeof(*per_bss_assoc_status);

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvAssociationStatusNotificationType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);
  free(pmem);
  return rc;
}

/* Extract Association status Notification TLV */
int i5TlvAssociationStatusNotificationTypeExtract(i5_message_type *pmsg)
{
  i5_dm_device_type *pdevice = NULL;
  i5_dm_bss_type *i5_bss = NULL;
  ieee1905_association_status_notification assoc_status;
  association_status_notification_bss *list = NULL;
  uint16 n_bytes = 0;
  uint16 i = 0;
  int rc = 0;
  unsigned char *pvalue;
  unsigned int length;

  pdevice = i5DmDeviceFind(i5MessageSrcMacAddressGet(pmsg));
  if (pdevice == NULL) {
    i5TraceError("Neighbour device does not exist\n");
    rc = -1;
    goto end;
  }

  i5MessageReset(pmsg);
  if ((rc = i5MessageTlvExtract(pmsg, i5TlvAssociationStatusNotificationType, &length, &pvalue,
    i5MessageTlvExtractWithoutReset)) == 0) {

    memset(&assoc_status, 0, sizeof(assoc_status));
    if (length < i5Tlv_AssociationStatusNotification_Min_Length) {
      i5TraceError("Length %d less than minimum association status notification Length\n", length);
      rc = -1;
      goto end;
    }
    assoc_status.count = *pvalue++;
    if (assoc_status.count == 0) {
      i5TraceInfo("empty list of bss in association status notification, exit \n");
      rc = -1;
      goto end;
    }

    n_bytes = assoc_status.count * sizeof(association_status_notification_bss);

    if (length < (n_bytes + i5Tlv_AssociationStatusNotification_Min_Length)) {
	/* mismatch in TLV length and number of entries */
	i5TraceError("Mismatch in Tlv length[%d] total bytes to decode[%d]\n", length, n_bytes);
	rc = -1;
	goto end;
    }

    list = (association_status_notification_bss*)pvalue;
    /* update bss association handling capability */
    for (i = 0; i < assoc_status.count; i++) {
      i5_bss = i5DmFindBSSFromDevice(pdevice, list[i].bssid);
      if (!i5_bss) {
	i5TraceInfo("BSSID not found, look for other BSSID in TLV\n");
        continue;
      }
      i5_bss->assoc_allowance_status = list[i].assoc_allowance_status;

      i5TraceInfo("i5_bssid["I5_MAC_DELIM_FMT"], assoc_status[%d] \n", I5_MAC_PRM(i5_bss->BSSID),
        i5_bss->assoc_allowance_status);
    }
  }
end:
  return rc;
}

/* Insert source info type TLV */
int i5TlvSourceInfoTypeInsert(i5_message_type *pmsg, unsigned char *sta_mac)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5Tlv_SourceTlv_Min_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvSourceInfoType;
  ptlv->length = htons(i5Tlv_SourceTlv_Min_Length);
  len += sizeof(i5_tlv_t);

  memcpy(&buf[len], sta_mac, MAC_ADDR_LEN);
  len += MAC_ADDR_LEN;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Source Info TLV from message */
int i5TlvSourceInfoTypeExtract(i5_message_type *pmsg, unsigned char *source_mac)
{
  int rc = 0;
  unsigned char *pvalue;
  unsigned int length;

  if ((rc = i5MessageTlvExtract(pmsg, i5TlvSourceInfoType, &length, &pvalue,
    i5MessageTlvExtractWithReset)) == 0) {
    if (length < i5Tlv_SourceTlv_Min_Length) {
	i5TraceError("Length %d less than minimum Source Info TLV Length\n", length);
      return -1;
    }
    memcpy(source_mac, pvalue, MAC_ADDR_LEN);
    return 0; /* success, return */
  }
  return rc;
}

/* Insert Tunnel Message type TLV */
int i5TlvTunneledMessageTypeInsert(i5_message_type *pmsg, ieee1905_tunnel_msg_payload_type_t payload_type)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5Tlv_Tunneled_Msg_Type_Min_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvTunneledMessgeType;
  ptlv->length = htons(i5Tlv_Tunneled_Msg_Type_Min_Length);
  len += sizeof(i5_tlv_t);

  buf[len] = (unsigned char)payload_type;
  len += sizeof(uint8); /* 1 byte payload type */

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Tunneled Message Type TLV */
int i5TlvTunneledMessageTypeExtract(i5_message_type *pmsg,
  ieee1905_tunnel_msg_payload_type_t *payload_type)
{
  int rc = 0;
  unsigned char *pvalue;
  unsigned int length;

  if (!payload_type) {
    i5TraceError("Invalid input argument, exit \n");
    return -1;
  }
  if ((rc = i5MessageTlvExtract(pmsg, i5TlvTunneledMessgeType, &length, &pvalue,
    i5MessageTlvExtractWithReset)) == 0) {
    if (length < i5Tlv_Tunneled_Msg_Type_Min_Length) {
      i5TraceError("Length %d less than Tunneled Message Type TLV Length\n", length);
      return -1;
    }
    *payload_type = *pvalue;
    return 0; /* success, return as there will be only one Tunnel message type TLV */
  }
  return rc;
}

/* Insert paylod for Tunneled TLV */
int i5TlvTunneledTypeInsert(i5_message_type *pmsg, unsigned char *payload, uint32 payload_len)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;
  uint32 n_bytes = 0;

  if (!pmsg || !payload) {
    printf(" Invalid arg, return with error \n");
    return -1;
  }
  /* TODO: payload len > 1500, confirm */
  if (payload_len > (I5_MESSAGE_MAX_TLV_SIZE - sizeof(i5_tlv_t))) {
    /* Unlikely situation */
    i5TraceDirPrint("Payload len bigger than MAX packet buf len, exit \n");
    return -1;
  }
  n_bytes = sizeof(i5_tlv_t) + payload_len;

  if ((pmem = (unsigned char *)malloc(n_bytes)) == NULL) {
    i5TraceDirPrint("malloc failed\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  memcpy(pbuf, payload, payload_len);
  pbuf += payload_len;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvTunneledType;
  ptlv->length = htons(payload_len);

  rc = i5MessageInsertTlv(pmsg, pmem, n_bytes);
  free(pmem);
  return rc;
}

/* Extract Tunneled TLV */
int i5TlvTunneledTypeExtract(i5_message_type *pmsg, ieee1905_tunnel_msg_t *tunnel_msg)
{
  int rc = 0;
  unsigned char *pvalue = NULL;
  unsigned int length = 0;

  if (!tunnel_msg) {
    i5TraceError("Invalid input argument tunnel msg, exit \n");
    return -1;
  }

  i5MessageReset(pmsg);
  while ((rc = i5MessageTlvExtract(pmsg, i5TlvTunneledType, &length, &pvalue,
    i5MessageTlvExtractWithoutReset)) == 0) {

    /* TODO: Add length check */
    tunnel_msg->payload = (unsigned char*)malloc(length);
    if (!tunnel_msg->payload) {
      i5TraceDirPrint("Malloc failed\n");
      return -1;
    }
    memcpy(tunnel_msg->payload, pvalue, length);
    tunnel_msg->payload_len = length;
    if (i5_config.cbs.process_tunneled_msg) {
      i5_config.cbs.process_tunneled_msg(i5MessageSrcMacAddressGet(pmsg), tunnel_msg);
    }
    if (tunnel_msg->payload) {
      free(tunnel_msg->payload);
    }
  }

  return rc;
}

/* TLV to insert the Profile-2 Client Steering Request */
int i5TlvProfile2SteeringRequestTypeInsert(i5_message_type *pmsg, ieee1905_steer_req *steer_req)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;
  dll_t *item_p, *next_p;
  ieee1905_sta_list *staInfo;
  ieee1905_bss_list *bssInfo;
  unsigned char req_flag = 0x00;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  /* BSSID */
  memcpy(pbuf, steer_req->source_bssid, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  /* Request Mode */
  if (IEEE1905_IS_STEER_MANDATE(steer_req->request_flags)) {
    req_flag |= I5_TLV_STEER_REQUEST_MODE;
  }
  if (IEEE1905_IS_DISASSOC_IMNT(steer_req->request_flags)) {
    req_flag |= I5_TLV_STEER_DISASSOC_IMNT;
  }
  if (IEEE1905_IS_BTM_ABRIDGED(steer_req->request_flags)) {
    req_flag |= I5_TLV_STEER_BTM_ABRIDGED;
  }
  *pbuf = (unsigned char)req_flag;
  pbuf++;

  *((unsigned short *)pbuf) = htons(steer_req->opportunity_window);
  pbuf += 2;

  /* BTM Disassociation Timer */
  *((unsigned short *)pbuf) = htons(steer_req->dissassociation_timer);
  pbuf += 2;

  /* STA List Count */
  *pbuf = (unsigned char)steer_req->sta_list.count;
  pbuf++;

  /* STA MAC address */
  for (item_p = dll_head_p(&steer_req->sta_list.head);
    !dll_end(&steer_req->sta_list.head, item_p);
    item_p = next_p) {
    next_p = dll_next_p(item_p);
    staInfo = (ieee1905_sta_list*)item_p;
    memcpy(pbuf, staInfo->mac, MAC_ADDR_LEN);
    pbuf += MAC_ADDR_LEN;
  }

  /* If request mode is 1(Means Steering Mandate) include Target BSSID List Count */
  if (IEEE1905_IS_STEER_MANDATE(steer_req->request_flags)) {
    *pbuf = (unsigned char)steer_req->bss_list.count;
    pbuf++;

    for (item_p = dll_head_p(&steer_req->bss_list.head);
      !dll_end(&steer_req->bss_list.head, item_p);
      item_p = next_p) {
      next_p = dll_next_p(item_p);
      bssInfo = (ieee1905_bss_list*)item_p;

      /* Target BSSID */
      memcpy(pbuf, bssInfo->bssid, MAC_ADDR_LEN);
      pbuf += MAC_ADDR_LEN;

      /* Target BSS Operating Class */
      *pbuf = bssInfo->target_op_class;
      pbuf++;

      /* Target BSS Channel Number */
      *pbuf = bssInfo->target_channel;
      pbuf++;

      /* Reason code for steering  */
      *pbuf = bssInfo->reason_code;
      pbuf++;
    }
  } else {
    *pbuf = (unsigned char)0;
    pbuf++;
  }

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvProfile2SteeringRequestType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Profile-2 Client Steering Request TLV */
int i5TlvProfile2SteeringRequestTypeExtract(i5_message_type *pmsg, ieee1905_steer_req *steer_req)
{
  unsigned char *pvalue;
  unsigned int length, extracted_len = i5TlvSteeringRequest_Min_Length;
  int rc = 0, i;
  unsigned char sta_count = 0, bss_count = 0;
  unsigned char req_flag = 0x00;

  /* Initialize sta and bss list */
  ieee1905_glist_init(&steer_req->sta_list);
  ieee1905_glist_init(&steer_req->bss_list);

  rc = i5MessageTlvExtract(pmsg, i5TlvProfile2SteeringRequestType, &length, &pvalue,
    i5MessageTlvExtractWithReset);
  if (rc != 0) {
    goto end;
  }

  if (length < i5TlvProfile2SteeringRequestType_Min_Length) {
    goto end;
  }

  /* This is profile-2 steering request TLV */
  steer_req->profile = map_steer_req_profile2;

  memcpy(steer_req->source_bssid, pvalue, MAC_ADDR_LEN);
  pvalue += MAC_ADDR_LEN;

  req_flag = *pvalue;
  pvalue++;
  if (req_flag & I5_TLV_STEER_REQUEST_MODE) {
    steer_req->request_flags |= IEEE1905_STEER_FLAGS_MANDATE;
  } else {
    steer_req->request_flags |= IEEE1905_STEER_FLAGS_OPPORTUNITY;
  }

  if (req_flag & I5_TLV_STEER_DISASSOC_IMNT) {
    steer_req->request_flags |= IEEE1905_STEER_FLAGS_DISASSOC_IMNT;
  }
  if (req_flag & I5_TLV_STEER_BTM_ABRIDGED) {
    steer_req->request_flags |= IEEE1905_STEER_FLAGS_BTM_ABRIDGED;
  }

  /* If request mode is not 1(Means its steering opportunity) read steering opportunity window */
  if (IEEE1905_IS_STEER_OPPORTUNITY(steer_req->request_flags)) {
    steer_req->opportunity_window = ntohs(*((unsigned short *)pvalue));
  }
  pvalue += 2;

  steer_req->dissassociation_timer = ntohs(*((unsigned short *)pvalue));
  pvalue += 2;

  sta_count = *pvalue;
  pvalue++;

  if (sta_count == 0) { /* No STAs */
    goto end;
  }

  if (length < (extracted_len + (sta_count * MAC_ADDR_LEN))) {
    rc = -1;
    goto end;
  }

  /* Get all the STAs */
  for (i = 0; i < sta_count; i++) {
    ieee1905_sta_list *sta_info;

    sta_info = (ieee1905_sta_list*)malloc(sizeof(*sta_info));
    if (!sta_info) {
      i5TraceDirPrint("malloc error\n");
      rc = -1;
      goto end;
    }

    memcpy(sta_info->mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;
    ieee1905_glist_append(&steer_req->sta_list, (dll_t*)sta_info);
  }

  extracted_len += (sta_count * MAC_ADDR_LEN);

  if (length < extracted_len + 1) {
    rc = -1;
    goto end;
  }

  bss_count = *pvalue;
  if (bss_count <= 0) { /* No BSS */
    goto end;
  }
  pvalue++;
  extracted_len++;

  if (length < (extracted_len + (bss_count * (MAC_ADDR_LEN + 1 + 1 + 1)))) {
    rc = -1;
    goto end;
  }

  /* Get all the BSS */
  for (i = 0; i < bss_count; i++) {
    ieee1905_bss_list *bss_info;

    bss_info = (ieee1905_bss_list*)malloc(sizeof(*bss_info));
    if (!bss_info) {
      i5TraceDirPrint("malloc error\n");
      rc = -1;
      goto end;
    }

    memcpy(bss_info->bssid, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;
    bss_info->target_op_class = *pvalue;
    pvalue++;
    bss_info->target_channel = *pvalue;
    pvalue++;
    bss_info->reason_code = *pvalue;
    pvalue++;
    ieee1905_glist_append(&steer_req->bss_list, (dll_t*)bss_info);
  }

end:
  return rc;
}

/* TLV to Insert Status Code info */
int i5TlvStatusCodeTypeInsert(i5_message_type *pmsg, unsigned short status_code)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvStatusCode_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvStatusCodeType;
  ptlv->length = htons(i5TlvStatusCode_Length);
  len += sizeof(i5_tlv_t);
  *((unsigned short *)&buf[len]) = htons(status_code);
  len += 2;
  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* TLV to Extract the Status code info */
int i5TlvStatusCodeTypeExtract(i5_message_type *pmsg, unsigned short *status_code)
{
  unsigned char *pvalue;
  unsigned int length;

  if (i5MessageTlvExtract(pmsg, i5TlvStatusCodeType, &length, &pvalue,
    i5MessageTlvExtractWithReset) == 0) {
    if (length >= i5TlvStatusCode_Length) {
      *status_code = ntohs(*(unsigned short *)pvalue);
      i5TraceInfo(" Status Code =  %d \n", *status_code);
      return 0;
    }
  }
  return -1;
}

/* Insert Metric Collection Interval TLV */
int i5TlvMetricCollectionIntervalTypeInsert(i5_message_type *pmsg, unsigned int interval)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvMetricCollectionInterval_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvMetricCollectionIntervalType;
  ptlv->length = htons(i5TlvMetricCollectionInterval_Length);
  len += sizeof(i5_tlv_t);

  *((unsigned int *)&buf[len]) = htonl(interval);
  len += 4;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Metric Collection Interval TLV */
int i5TlvMetricCollectionIntervalTypeExtract(i5_message_type *pmsg, unsigned int *interval)
{
  unsigned char *pvalue;
  unsigned int length;

  i5Trace("\n");

  if (i5MessageTlvExtract(pmsg, i5TlvMetricCollectionIntervalType, &length, &pvalue,
    i5MessageTlvExtractWithReset) == 0) {
    if (length >= i5TlvMetricCollectionInterval_Length) {
      *interval = ntohl(*((unsigned int *)pvalue));
      pvalue += 4;
    } else {
      goto end;
    }
  } else {
    goto end;
  }

  i5Trace("Interval[%d]\n", *interval);
  return 0;

end:
  return -1;
}

/* Insert Radio Metrics TLV */
int i5TlvRadioMetricsTypeInsert(i5_message_type *pmsg, unsigned char *mac,
  ieee1905_interface_metric *ifrMetric)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvRadioMetrics_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvRadioMetricsType;
  ptlv->length = htons(i5TlvRadioMetrics_Length);
  len += sizeof(i5_tlv_t);

  memcpy(&buf[len], mac, MAC_ADDR_LEN);
  len += MAC_ADDR_LEN;
  buf[len++] = (unsigned char)ifrMetric->noise;
  buf[len++] = (unsigned char)ifrMetric->transmit;
  buf[len++] = (unsigned char)ifrMetric->receive_self;
  buf[len++] = (unsigned char)ifrMetric->receive_other;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract Radio Metrics TLV */
int i5TlvRadioMetricsTypeExtract(i5_message_type *pmsg, i5_dm_device_type *pdevice)
{
  unsigned char *pvalue;
  unsigned char radio_mac[MAC_ADDR_LEN];
  unsigned int length;
  i5_dm_interface_type *pdmif;

  i5Trace("\n");

  i5MessageReset(pmsg);
  while (i5MessageTlvExtract(pmsg, i5TlvRadioMetricsType, &length, &pvalue,
    i5MessageTlvExtractWithoutReset) == 0) {

    if (length < i5TlvRadioMetrics_Length) {
      goto end;
    }
    memcpy(radio_mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;

    pdmif = i5DmInterfaceFind(pdevice, radio_mac);
    if (pdmif == NULL) {
      i5Trace("Radio["I5_MAC_DELIM_FMT"] Not found\n", I5_MAC_PRM(radio_mac));
      goto end;
    }

    pdmif->ifrMetric.noise = *pvalue++;
    pdmif->ifrMetric.transmit = *pvalue++;
    pdmif->ifrMetric.receive_self = *pvalue++;
    pdmif->ifrMetric.receive_other = *pvalue++;
    i5Trace("Radio["I5_MAC_DELIM_FMT"] Noise[%d] Transmit[%d] receive_self[%d] receive_other[%d]\n",
      I5_MAC_PRM(radio_mac), pdmif->ifrMetric.noise, pdmif->ifrMetric.transmit,
      pdmif->ifrMetric.receive_self, pdmif->ifrMetric.receive_other);
  }

  return 0;

end:
  return -1;
}

/* Insert AP Extended Metrics TLV */
int i5TlvAPExtendedMetricsTypeInsert(i5_message_type *pmsg, i5_dm_bss_type *pbss)
{
  unsigned char buf[sizeof(i5_tlv_t) + i5TlvAPExtendedMetrics_Length];
  i5_tlv_t *ptlv = (i5_tlv_t *)buf;
  int len = 0;

  ptlv->type = i5TlvAPExtendedMetricsType;
  ptlv->length = htons(i5TlvAPExtendedMetrics_Length);
  len += sizeof(i5_tlv_t);

  memcpy(&buf[len], pbss->BSSID, MAC_ADDR_LEN);
  len += MAC_ADDR_LEN;
  *((unsigned int *)&buf[len]) = htonl(pbss->APMetric.unicastBytesSent);
  len += 4;
  *((unsigned int *)&buf[len]) = htonl(pbss->APMetric.unicastBytesReceived);
  len += 4;
  *((unsigned int *)&buf[len]) = htonl(pbss->APMetric.multicastBytesSent);
  len += 4;
  *((unsigned int *)&buf[len]) = htonl(pbss->APMetric.multicastBytesReceived);
  len += 4;
  *((unsigned int *)&buf[len]) = htonl(pbss->APMetric.broadcastBytesSent);
  len += 4;
  *((unsigned int *)&buf[len]) = htonl(pbss->APMetric.broadcastBytesReceived);
  len += 4;

  return (i5MessageInsertTlv(pmsg, buf, len));
}

/* Extract AP Extended Metrics TLV */
int i5TlvAPExtendedMetricsTypeExtract(i5_message_type *pmsg, i5_dm_device_type *pdevice)
{
  unsigned char *pvalue;
  unsigned char BSSID[MAC_ADDR_LEN];
  unsigned int length;
  i5_dm_bss_type *pbss;

  i5Trace("\n");

  i5MessageReset(pmsg);
  while (i5MessageTlvExtract(pmsg, i5TlvAPExtendedMetricsType, &length, &pvalue,
    i5MessageTlvExtractWithoutReset) == 0) {

    if (length < i5TlvAPExtendedMetrics_Length) {
      goto end;
    }
    memcpy(BSSID, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;

    pbss = i5DmFindBSSFromDevice(pdevice, BSSID);
    if (pbss == NULL) {
      i5Trace("BSS["I5_MAC_DELIM_FMT"] Not found in Device["I5_MAC_DELIM_FMT"]\n",
        I5_MAC_PRM(BSSID), I5_MAC_PRM(pdevice->DeviceId));
      goto end;
    }

    pbss->APMetric.unicastBytesSent = ntohl(*((unsigned int *)pvalue));
    pvalue += 4;
    pbss->APMetric.unicastBytesReceived = ntohl(*((unsigned int *)pvalue));
    pvalue += 4;
    pbss->APMetric.multicastBytesSent = ntohl(*((unsigned int *)pvalue));
    pvalue += 4;
    pbss->APMetric.multicastBytesReceived = ntohl(*((unsigned int *)pvalue));
    pvalue += 4;
    pbss->APMetric.broadcastBytesSent = ntohl(*((unsigned int *)pvalue));
    pvalue += 4;
    pbss->APMetric.broadcastBytesReceived = ntohl(*((unsigned int *)pvalue));
    pvalue += 4;
    i5Trace("Device["I5_MAC_DELIM_FMT"] BSS["I5_MAC_DELIM_FMT"] unicastBytesSent[%d] "
      "unicastBytesReceived[%d] multicastBytesSent[%d] multicastBytesReceived[%d] "
      "broadcastBytesSent[%d] broadcastBytesReceived[%d]\n",
      I5_MAC_PRM(pdevice->DeviceId), I5_MAC_PRM(BSSID), pbss->APMetric.unicastBytesSent,
      pbss->APMetric.unicastBytesReceived, pbss->APMetric.multicastBytesSent,
      pbss->APMetric.multicastBytesReceived, pbss->APMetric.broadcastBytesSent,
      pbss->APMetric.broadcastBytesReceived);
  }

  return 0;

end:
  return -1;
}

/* TLV to add Associated STA Extended Link Metrics TLV */
int i5TlvAssociatedSTAExtendedLinkMetricsTypeInsert(i5_message_type *pmsg, unsigned char *mac,
  unsigned char *bssid, ieee1905_sta_link_metric *metric)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  memcpy(pbuf, mac, MAC_ADDR_LEN);
  pbuf += MAC_ADDR_LEN;

  /* Number of Bssid reported for this STA */
  if (bssid != NULL) {
    *pbuf = (unsigned char)1;
    pbuf++;

    memcpy(pbuf, bssid, MAC_ADDR_LEN);
    pbuf += MAC_ADDR_LEN;

    *((unsigned int *)pbuf) = htonl(metric->last_data_downlink_rate);
    pbuf += 4;

    *((unsigned int *)pbuf) = htonl(metric->last_data_uplink_rate);
    pbuf += 4;

    *((unsigned int *)pbuf) = htonl(metric->utilization_recv);
    pbuf += 4;

    *((unsigned int *)pbuf) = htonl(metric->utilization_tx);
    pbuf += 4;
  } else {
    *pbuf = 0;
    pbuf++;
  }

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvAssociatedSTAExtendedLinkMetricsType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* Extract Associated STA Extended Link Metrics TLV */
int i5TlvAssociatedSTAExtendedLinkMetricsTypeExtract(i5_message_type *pmsg)
{
  i5_dm_device_type *pdevice;
  i5_dm_bss_type *pdmbss;
  i5_dm_clients_type *pdmclient;
  unsigned char *pvalue, mac[MAC_ADDR_LEN], bssid[MAC_ADDR_LEN];
  unsigned int length, found = 0;
  int rc = 0;

  pdevice = i5DmDeviceFind(i5MessageSrcMacAddressGet(pmsg));
  if (pdevice == NULL) {
    i5TraceError("Neighbour device does not exist\n");
    rc = -1;
    goto end;
  }

  i5MessageReset(pmsg);
  while ((rc = i5MessageTlvExtract(pmsg, i5TlvAssociatedSTAExtendedLinkMetricsType, &length, &pvalue,
    i5MessageTlvExtractWithoutReset)) == 0) {
    int pos = 0;
    unsigned char bss_count = 0;
    found = 1;

    if (length < i5TlvAssociatedSTAExtendedLinkMetric_Min_Length) {
      rc = -1;
      goto end;
    }
    memcpy(mac, &pvalue[pos], MAC_ADDR_LEN);
    pos += MAC_ADDR_LEN;

    bss_count = pvalue[pos];
    pos++;
    if (bss_count == 0) {
      continue;
    }

    if (length < pos + i5TlvAssociatedSTAExtendedLinkMetric_BSS_Length) {
      continue;
    }

    memcpy(bssid, &pvalue[pos], MAC_ADDR_LEN);
    pos += MAC_ADDR_LEN;

      /* find the BSS in this device */
    if ((pdmbss = i5DmFindBSSFromDevice(pdevice, bssid)) == NULL) {
      i5TraceError("BSS " I5_MAC_DELIM_FMT " does not exist in device " I5_MAC_DELIM_FMT "\n",
        I5_MAC_PRM(bssid), I5_MAC_PRM(pdevice->DeviceId));
      continue;
    }

    pdmclient = i5DmFindClientInBSS(pdmbss, mac);
    if (pdmclient == NULL) {
      i5TraceError("STA " I5_MAC_DELIM_FMT " does not exist in device " I5_MAC_DELIM_FMT " In BSS "
        I5_MAC_DELIM_FMT "\n",
        I5_MAC_PRM(mac), I5_MAC_PRM(pdevice->DeviceId), I5_MAC_PRM(bssid));
      continue;
    }

    pdmclient->link_metric.last_data_downlink_rate = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    pdmclient->link_metric.last_data_uplink_rate = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    pdmclient->link_metric.utilization_recv = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    pdmclient->link_metric.utilization_tx = ntohl(*((unsigned int *)&pvalue[pos]));
    pos += 4;

    i5Trace("Device["I5_MAC_DELIM_FMT"] BSS["I5_MAC_DELIM_FMT"] LastDatadownlinkrate[%d] "
      "LastDataUplinkrate[%d] UtilizationReceive[%d] UtilizationTransmit[%d] \n",
      I5_MAC_PRM(pdevice->DeviceId), I5_MAC_PRM(bssid), pdmclient->link_metric.last_data_downlink_rate,
      pdmclient->link_metric.last_data_uplink_rate, pdmclient->link_metric.utilization_recv,
      pdmclient->link_metric.utilization_recv);

  }
  if (!found) {
    rc = -1;
  }
end:
  return rc;
}

/* add CAC request TLV */
int i5TlvCACRequestTypeInsert(i5_message_type *pmsg, ieee1905_cac_rqst_list_t *cac_rqst)
{
  unsigned char *pbuf, *pmem;
  ieee1905_radio_cac_rqst_t *buf = NULL;
  int i = 0;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  *pbuf = cac_rqst->count;
  pbuf++;

  for(i = 0; i < cac_rqst->count; i++) {
    buf = (ieee1905_radio_cac_rqst_t*)pbuf;
    buf->opclass = cac_rqst->params[i].opclass;
    buf->chan = cac_rqst->params[i].chan;
    buf->flags = cac_rqst->params[i].flags;
    memcpy(&(buf->mac), cac_rqst->params[i].mac, MAC_ADDR_LEN);
    pbuf += sizeof(ieee1905_radio_cac_rqst_t);
  }

  i5Debug("%zu bytes read in CAC request TLV insert \n", pbuf-pmem-sizeof(i5_tlv_t));
  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvCACRequestType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, (pbuf-pmem));

  free(pmem);
  return (rc);
}

/* Extract CAC request TLV */
int i5TlvCACRequestTypeExtract(i5_message_type *pmsg)
{
  ieee1905_cac_rqst_list_t *cac_msg = NULL;
  unsigned char *pvalue = NULL;
  unsigned int length = 0;
  int rc = 0;

  i5MessageReset(pmsg);

  rc = i5MessageTlvExtract(pmsg, i5TlvCACRequestType, &length, &pvalue,
    i5MessageTlvExtractWithoutReset);

  if (rc != 0) {
    i5TraceError("Error in extracting CAC Request TLV, exit \n");
    return -1;
  }

  if (length < i5TlvCACRequestType_Min_Length) {
    i5TraceError("Invalid len[%d] in CAC request, exit \n", length);
    return -1;
  }

  cac_msg = (ieee1905_cac_rqst_list_t*)malloc(length);

  if (!cac_msg) {
    i5TraceDirPrint("Malloc failed\n");
    return -1;
  }

  i5Trace(" CAC request rcvd with length[%d] \n", length);

  memcpy((unsigned char*)cac_msg, pvalue, length);

  i5Trace("CAC request rcvd for n radios[%d] request len[%d] \n", cac_msg->count, length);

  if (i5_config.cbs.process_cac_msg) {
    i5Trace("execute callback to process cac msg \n");
    i5_config.cbs.process_cac_msg(i5MessageSrcMacAddressGet(pmsg), cac_msg, MAP_CAC_RQST);
  }

  if (cac_msg) {
    free(cac_msg);
  }
  return rc;
}

/* add CAC Terminate TLV */
int i5TlvCACTerminationTypeInsert(i5_message_type *pmsg,
  ieee1905_cac_termination_list_t *cac_terminate)
{
  unsigned char *pbuf, *pmem;
  ieee1905_radio_cac_params_t *buf = NULL;
  int i = 0;
  i5_tlv_t *ptlv;
  int rc = 0;

  if ((pmem = (unsigned char *)malloc(I5_PACKET_BUF_LEN)) == NULL) {
    i5TraceDirPrint("malloc error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end

  *pbuf = cac_terminate->count;
  pbuf++;

  for(i = 0; i < cac_terminate->count; i++) {
    buf = (ieee1905_radio_cac_params_t*)pbuf;
    buf->opclass = cac_terminate->params[i].opclass;
    buf->chan = cac_terminate->params[i].chan;
    memcpy(&(buf->mac), cac_terminate->params[i].mac, MAC_ADDR_LEN);
    pbuf += sizeof(ieee1905_radio_cac_params_t);
  }

  i5Debug("%zu bytes read in CAC Terminate TLV insert \n", pbuf-pmem-sizeof(i5_tlv_t));
  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvCACTerminationType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));
  rc = i5MessageInsertTlv(pmsg, pmem, (pbuf-pmem));

  free(pmem);
  return (rc);
}

/* Extract CAC Termination TLV */
int i5TlvCACTerminationTypeExtract(i5_message_type *pmsg)
{
  ieee1905_cac_termination_list_t *cac_msg = NULL;
  ieee1905_radio_cac_params_t *radio_info = NULL;
  unsigned char *pvalue = NULL;
  unsigned int length = 0;
  int rc = 0;
  uint32 alloc_bytes = 0;
  uint8 i =0;
  uint8 count = 0;

  i5MessageReset(pmsg);

  rc = i5MessageTlvExtract(pmsg, i5TlvCACTerminationType, &length, &pvalue,
         i5MessageTlvExtractWithoutReset);

  if (rc != 0) {
    i5TraceError("Error in extracting CAC termination TLV, exit \n");
    return -1;
  }

  if (length < i5TlvCACTerminationType_Min_Length) {
    i5TraceError("Invalid len[%d] in CAC request, exit \n", length);
    return -1;
  }

  count = *pvalue;
  pvalue += sizeof(unsigned char);

  alloc_bytes = sizeof(ieee1905_cac_termination_list_t) +
                  ((count -1) * (sizeof(ieee1905_radio_cac_params_t)));

  i5Trace(" CAC terminate rcvd for n_radios[%d] msg len[%d] allocate memory[%d] \n", count,
    length, alloc_bytes);

  cac_msg = (ieee1905_cac_termination_list_t*)malloc(alloc_bytes);
  if (!cac_msg) {
    i5TraceDirPrint("Malloc failed\n");
    return -1;
  }
  cac_msg->count = count;

  radio_info = (ieee1905_radio_cac_params_t*)cac_msg->params;

  for (i = 0; i < count; i++) {
    memcpy(&radio_info[i].mac, pvalue, MAC_ADDR_LEN);
    pvalue += MAC_ADDR_LEN;
    radio_info[i].opclass = *pvalue;
    pvalue++;
    radio_info[i].chan = *pvalue;
    pvalue++;
  }

  if (i5_config.cbs.process_cac_msg) {
    i5Trace("execute callback to process cac msg: \n");
    i5_config.cbs.process_cac_msg(i5MessageSrcMacAddressGet(pmsg), cac_msg,
      MAP_CAC_TERMINATE);
  }

  if (cac_msg) {
    free(cac_msg);
  }

  ((void)(radio_info));

  return rc;
}

/* TLV to report the Cac completion report */
int i5TlvCacCompletionTypeInsert(i5_message_type *pmsg, uint8 *data, uint32 len)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;

  if (!data || !len) {
    i5TraceError("Invalid argument to prepare CAC complete TLV, exit \n");
    return -1;
  }
  if ((pmem = (unsigned char *)malloc(len + sizeof(i5_tlv_t))) == NULL) {
    i5TraceDirPrint("Out of memory error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end
  memcpy(pbuf, data, len);
  pbuf += len;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvCACCompletionReportType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* TLV to report the CAC Capabilities */
int i5TlvCacCapabilitiesTypeInsert(i5_message_type *pmsg, uint8 *data, uint32 len)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;

  if (!data || !len) {
    i5TraceError("Invalid argument to prepare CAC complete TLV, exit \n");
    return -1;
  }
  if ((pmem = (unsigned char *)malloc(len + sizeof(i5_tlv_t))) == NULL) {
    i5TraceDirPrint("Out of memory error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end
  memcpy(pbuf, data, len);
  pbuf += len;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvCACCapabilitiesType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}

/* TLV to report the Cac status */
int i5TlvCacStatusReportTypeInsert(i5_message_type *pmsg, uint8 *data, uint32 len)
{
  unsigned char *pbuf, *pmem;
  i5_tlv_t *ptlv;
  int rc = 0;

  if (!data || !len) {
    i5TraceError("Invalid argument to prepare CAC status report TLV, exit \n");
    return -1;
  }
  if ((pmem = (unsigned char *)malloc(len + sizeof(i5_tlv_t))) == NULL) {
    i5TraceDirPrint("Out of memory error\n");
    return -1;
  }

  pbuf = pmem + sizeof(i5_tlv_t); // Header filled at the end
  memcpy(pbuf, data, len);
  pbuf += len;

  ptlv = (i5_tlv_t *)pmem;
  ptlv->type = i5TlvCACStatusReportType;
  ptlv->length = htons(pbuf-pmem-sizeof(i5_tlv_t));

  rc = i5MessageInsertTlv(pmsg, pmem, pbuf-pmem);

  free(pmem);
  return (rc);
}
#endif /* MULTIAPR2 */
#endif /* MULTIAP */
