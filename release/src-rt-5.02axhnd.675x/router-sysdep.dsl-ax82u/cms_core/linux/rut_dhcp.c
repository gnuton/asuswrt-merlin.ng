/***********************************************************************
 *
 *  Copyright (c) 2017  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2017:proprietary:standard

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
 *
 ************************************************************************/

/*
 * rut_dhcp.c
 *
 *  Created on:  Sep. 2017
 *      Author: Fuguo Xu <fuguo.xu@broadcom.com>
 */



#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut_dhcp.h"
#include <arpa/inet.h>




#if defined(BRCM_PKTCBL_SUPPORT)
typedef struct {
    char *duid;    
}Option43InParms;

static int option43_getMacAddr(const void *parm, char *string, int *len)
{
    /* actually the SN in DeviceInfo is MacAddr, so get from SN directly */
    return option_getSN(parm, string, len);
}                 

/* EMTA */
static DhcpSubOptionTable option43_subOptionsEmta[] = 
{
    { 2,   OPTION_CHAR_STRING, "dev_type",        "EMTA",               NULL },
    { 4,   OPTION_CHAR_STRING, "sn",              "001018b0ff00",       option_getSN },
    { 5,   OPTION_CHAR_STRING, "hw_ver",          "V1.0",               option_getHwV },
    { 6,   OPTION_CHAR_STRING, "sw_ver",          "5.02L.04",           option_getSwV },
    { 7,   OPTION_CHAR_STRING, "bootroom_ver",    "CFE=1.0",            option_getBtV },
    { 8,   OPTION_CHAR_STRING, "OUI",             "001018",             option_getOUI },
    { 9,   OPTION_CHAR_STRING, "model_num",       "968580XREF",         option_getModelNum },
    { 10,  OPTION_CHAR_STRING, "vendor_name",     "Broadcom",           option_getVdrName },
    { 31,  OPTION_HEX_STRING,  "mac",             "001018b0ff00",       option43_getMacAddr },
    { 32,  OPTION_HEX_STRING,  "id_correlation",  "1DA82FF9",           NULL }
};

/* EPTA */
static DhcpSubOptionTable option43_subOptionsEpta[] = 
{
    { 2,   OPTION_CHAR_STRING, "dev_type",        "EPTA",               NULL },
    { 4,   OPTION_CHAR_STRING, "sn",              "001018b0ff00",       option_getSN },
    { 5,   OPTION_CHAR_STRING, "hw_ver",          "V1.0",               option_getHwV },
    { 6,   OPTION_CHAR_STRING, "sw_ver",          "5.02L.04",           option_getSwV },
    { 7,   OPTION_CHAR_STRING, "bootroom_ver",    "CFE=1.0",            option_getBtV },
    { 8,   OPTION_CHAR_STRING, "OUI",             "001018",             option_getOUI },
    { 9,   OPTION_CHAR_STRING, "model_num",       "968580XREF",         option_getModelNum },
    { 10,  OPTION_CHAR_STRING, "vendor_name",     "Broadcom",           option_getVdrName },
    { 31,  OPTION_HEX_STRING,  "mac",             "001018b0ff00",       option43_getMacAddr },
    { 32,  OPTION_HEX_STRING,  "id_correlation",  "1DA82FF9",           NULL }
};

CmsRet rutDhcp_createOption43(PKTCBL_WAN_TYPE wanType, const char *ifName, const char *duid)
{
    char code = DHCP_VDR_SPECIFIC_INFO;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0};
    int totalLen = 0, dataLen = 0;
    Option43InParms parm;

    parm.duid = (char *)duid;

    if (PKTCBL_WAN_EMTA == wanType) /* EMTA */
    {
        if (CMSRET_SUCCESS != cmsDhcp_encapsulateSubOption(code, option43_subOptionsEmta, 
            sizeof(option43_subOptionsEmta)/sizeof(DhcpSubOptionTable), (void *)&parm, 
            &optionFrame[VDR_OPTION_SUBCODE_OFFSET], &dataLen,
            OPTION_CODE_LEN1, OPTION_SIZE_LEN1))
        {
            return CMSRET_INTERNAL_ERROR;
        }
    }
    else if (PKTCBL_WAN_EPTA == wanType) /* EPTA */
    {
        if (CMSRET_SUCCESS != cmsDhcp_encapsulateSubOption(code, option43_subOptionsEpta, 
            sizeof(option43_subOptionsEpta)/sizeof(DhcpSubOptionTable), (void *)&parm, 
            &optionFrame[VDR_OPTION_SUBCODE_OFFSET], &dataLen,
            OPTION_CODE_LEN1, OPTION_SIZE_LEN1))
        {
            return CMSRET_INTERNAL_ERROR;
        }
    }
    else
    {
        cmsLog_error("unknown wanType %d", wanType);
        return CMSRET_INTERNAL_ERROR;
    }

    optionFrame[VDR_OPTION_CODE_OFFSET] = code;
    optionFrame[VDR_OPTION_LEN_OFFSET] = dataLen;
    totalLen = VDR_OPTION_SUBCODE_OFFSET + dataLen;
    return cmsDhcp_saveOption(DHCP_V4, ifName, code, optionFrame, totalLen);
}  

static int option60_getEndPointNum(const void * parm, char* string, int * len)
{
    /* TODO:  hard code tmp */
    int cpyLen = 2;

    strncpy(string, "02", cpyLen);
    *len = cpyLen;

    return 0;
}

static int option60_getIfIndex(const void * parm, char* string, int * len)
{
    /* TODO:  hard code tmp */
    int cpyLen = 2;

    strncpy(string, "09", cpyLen);
    *len = cpyLen;

    return 0;
}

/* For SubOption descripton, Pls refer to "PacketCable� 1.5 Specification":  "PKT-SP-PROV1.5-I04-090624.doc" */
static DhcpSubOptionTable option60_subOptions[] = {
    { 1,    OPTION_HEX_STRING,     "",      "02",             NULL },
    { 2,    OPTION_HEX_STRING,     "",      "02",             option60_getEndPointNum },
    { 3,    OPTION_HEX_STRING,     "",      "00",             NULL },
    { 4,    OPTION_HEX_STRING,     "",      "00",             NULL },
    { 9,    OPTION_HEX_STRING,     "",      "01" ,            NULL },
    { 11,   OPTION_HEX_STRING,     "",      "06090f",         NULL },
    { 12,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 13,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 15,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 16,   OPTION_HEX_STRING,     "",      "09",             option60_getIfIndex },
    { 18,   OPTION_HEX_STRING,     "",      "0007",           NULL },
    { 19,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 20,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 21,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 22,   OPTION_HEX_STRING,     "",      "01",             NULL },
    { 23,   OPTION_HEX_STRING,     "",      "02003f",         NULL },
    { 24,   OPTION_HEX_STRING,     "",      "00",             NULL },
    { 25,   OPTION_HEX_STRING,     "",      "00",             NULL },
    { 26,   OPTION_HEX_STRING,     "",      "00",             NULL },
    { 38,   OPTION_HEX_STRING,     "",      "01",             NULL }
};

/* to share the descripter with DHCPv6 option17->subOption35 */
int get_dhcpV4Option60DesPtr(DhcpSubOptionTable **desPtr, int *TableLen)
{
    *desPtr = option60_subOptions;
    *TableLen = sizeof(option60_subOptions)/sizeof(DhcpSubOptionTable);
    return 0;
}

CmsRet rutDhcp_createOption60(const char *ifName)
{
#define OPTION60_SUB_HEADER "pktc2.0:05"
#define OPTION60_SUB_LEN_SIZE  2
    char code = DHCP_VDR;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0};
    char dataFrame[VDR_MAX_DHCP_OPTION_LEN] = {0};
    int dataOffset, subLenOffset, dataLen = 0;
    int subHeaderLen = strlen(OPTION60_SUB_HEADER);
    int i, totalLen = 0;

    subLenOffset = VDR_OPTION_SUBCODE_OFFSET + subHeaderLen;
    dataOffset = subLenOffset + OPTION60_SUB_LEN_SIZE;
    if (CMSRET_SUCCESS != cmsDhcp_encapsulateSubOption(code, option60_subOptions, 
        sizeof(option60_subOptions)/sizeof(DhcpSubOptionTable), NULL, 
        dataFrame, &dataLen,
        OPTION_CODE_LEN1, OPTION_SIZE_LEN1))
    {
        return CMSRET_INTERNAL_ERROR;
    }

    optionFrame[VDR_OPTION_CODE_OFFSET] = code;
    optionFrame[VDR_OPTION_LEN_OFFSET] = subHeaderLen + OPTION60_SUB_LEN_SIZE + dataLen * 2;
    strncpy(&optionFrame[VDR_OPTION_SUBCODE_OFFSET], OPTION60_SUB_HEADER, subHeaderLen);

    /* Hex to Hex ASCII string */
    sprintf(&optionFrame[subLenOffset], "%02x", (char)dataLen);
    for (i = 0; i < dataLen && i < VDR_MAX_DHCP_OPTION_LEN; i++)
    {
        sprintf(&optionFrame[dataOffset], "%02x", dataFrame[i]);
        dataOffset += 2;        
    }

    totalLen = VDR_OPTION_SUBCODE_OFFSET + optionFrame[VDR_OPTION_LEN_OFFSET];

    return cmsDhcp_saveOption(DHCP_V4, ifName, code, optionFrame, totalLen);
}


static DhcpSubOptionTable option125_subOptions[] = {
    { 1,    OPTION_HEX_STRING,  "oro",             "0206",   NULL }, /* option 2, 6 */
};

CmsRet rutDhcp_createOption125(const char *ifName)
{
    char code = DHCP_VDR_VI_VENDOR;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLen = 0;
    uint32_t enterprise_num = DHCP_ENTERPRISE_NUMBER_CTL;

    buffPtr = &optionFrame[VDR_OPTION_CODE_OFFSET];
    *buffPtr = code;
    buffPtr += 1;
    totalLen += 1;
    
    /* skip option len first */
    buffPtr += 1;
    totalLen += 1;
    
    *(uint32_t *)buffPtr = htonl(enterprise_num);
    buffPtr += 4;
    totalLen += 4;

    /* skip data-len1 first */
    buffPtr += 1;
    totalLen += 1;

    if (CMSRET_SUCCESS != cmsDhcp_encapsulateSubOption(code, option125_subOptions, 
        sizeof(option125_subOptions)/sizeof(DhcpSubOptionTable), NULL, 
        buffPtr, &dataLen, OPTION_CODE_LEN1, OPTION_SIZE_LEN1))
    {
        cmsLog_error("fail");
        return CMSRET_INTERNAL_ERROR;
    }

    totalLen += dataLen;
    optionFrame[6] = dataLen; /* data-len1 */
    optionFrame[VDR_OPTION_LEN_OFFSET] = dataLen + 4 + 1; /* 4 for enterprise_num, 1 for data-len1 */    
    return cmsDhcp_saveOption(DHCP_V4, ifName, code, optionFrame, totalLen);
}

CmsRet rutDhcp_getAcsUrlFromOption125(const char *ifName, char *acsURL, int inLen)
{
    char code = DHCP_VDR_VI_VENDOR;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLenN = 0;
    char subOptionData[CMS_MAX_ACS_URL_LENGTH] = {0};
    int subDataLen = 0;
    uint32_t enterprise_num = 0;
    char urlType;

    if (!ifName || !acsURL)
    {
        cmsLog_error("invalid params");
        return CMSRET_INVALID_ARGUMENTS;
    }

    totalLen = sizeof(optionFrame);
    if (CMSRET_SUCCESS != cmsDhcp_readOption(DHCP_V4, ifName, code, optionFrame, &totalLen))
    {        
        cmsLog_error("get option%d fail", code);
        return CMSRET_INTERNAL_ERROR;
    }
    
    buffPtr = &optionFrame[2];
    while (buffPtr < (optionFrame + totalLen))
    {
        enterprise_num = ntohl(*(uint32_t *)buffPtr);
        buffPtr += 4;
        dataLenN = *buffPtr;
        buffPtr += 1;

        /* check enterprise_num */
        if (DHCP_ENTERPRISE_NUMBER_CTL != enterprise_num)
        {
            cmsLog_notice("unknow enterprise number %d", enterprise_num);
            buffPtr += dataLenN;
            continue;
        }

        subDataLen = sizeof(subOptionData);
        if (CMSRET_SUCCESS != cmsDhcp_getSubOptionData(DHCP_V4, buffPtr, dataLenN, 
                            DHCP4_OPTION125_SUBOPTION6, subOptionData, &subDataLen))
        {
            cmsLog_error("get sub-option%d fail", DHCP4_OPTION125_SUBOPTION6);
            return CMSRET_INVALID_ARGUMENTS;
        }

        urlType = subOptionData[0];
        if ((5 == subDataLen) && (1 == urlType)) /* subDataLen == 5 && type == IPv4 address */
        {
            inet_ntop(AF_INET, (void *)&subOptionData[1], acsURL, inLen);
        }
        else if (0 == urlType) /* type == FQDN */
        {
            int fqdnLen = subDataLen - 1; /* '-1' for removing size of 'type' */
            strncpy(acsURL, &subOptionData[1], 
                    (fqdnLen < inLen) ? fqdnLen : inLen);
        }
        else
        {
            cmsLog_error("unknown url type %d", urlType);
            return CMSRET_INVALID_ARGUMENTS;
        }

        break;
    }

    cmsLog_debug("acsURL=%s", acsURL);
    return CMSRET_SUCCESS;
}

CmsRet rutDhcp_getNtpserversFromOption42(const char *ifName, char *ntpServerList, int inLen)
{
    char code = DHCP_VDR_NTP_SERVERS;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLen = 0;
    char *dstPtr = ntpServerList;
    int serverNum = 0;

    if (!ifName || !ntpServerList)
    {
        cmsLog_error("invalid params");
        return CMSRET_INVALID_ARGUMENTS;
    }

    totalLen = sizeof(optionFrame);
    if (CMSRET_SUCCESS != cmsDhcp_readOption(DHCP_V4, ifName, code, optionFrame, &totalLen))
    {        
        cmsLog_error("get option%d fail", code);
        return CMSRET_INTERNAL_ERROR;
    }

    /* validate dataLen */
    dataLen = optionFrame[1];
    if ((dataLen < 4) || (dataLen%4 != 0))
    {        
        cmsLog_error("invalid dataLen=%d", dataLen);
        return CMSRET_INVALID_ARGUMENTS;
    }

    buffPtr = &optionFrame[2];
    ntpServerList[0] = '\0';
    while (buffPtr < (optionFrame + totalLen))
    {
        inet_ntop(AF_INET, (void *)buffPtr, dstPtr, inLen - strlen(ntpServerList));
        buffPtr += 4;
        dstPtr  += strlen(ntpServerList);

        serverNum++;
        if (serverNum > 5)
        {        
            cmsLog_notice("allows up to 5 NTP servers, ignore excess ones");
            break;
        }

        dstPtr += sprintf(dstPtr, "%s", ","); /* start next one */
    }

    cmsLog_debug("ntpServerList=%s", ntpServerList);
    return CMSRET_SUCCESS;
}


#endif // BRCM_PKTCBL_SUPPORT

