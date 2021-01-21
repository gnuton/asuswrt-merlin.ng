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
 * rut_dhcp6.c
 *
 *  Created on:  Sep. 2017
 *      Author: Fuguo Xu <fuguo.xu@broadcom.com>
 */

#ifdef SUPPORT_IPV6

#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut_dhcp6.h"
#include <arpa/inet.h>



#if defined(BRCM_PKTCBL_SUPPORT)

extern int get_dhcpV4Option60DesPtr(DhcpSubOptionTable **desPtr, int *TableLen);


/* For option17->subOption35: TLV5, CL_OPTION_MODEM_CAPABILITIES. Pls refer to [CL-SP-CANN-DHCP-Reg-I14-170111.pdf] section 5.2.15 
** which has same format as DHCPv4 option60_subOptions.
*/
static DhcpSubOptionTable *option17_35_subOptions = NULL;

static int option17_35getTLV5(const void *parm, char *string, int *len)
{
    uint16_t SubCode = 35;
    int i, dataOffset;
    char dataFrame[VDR_MAX_DHCP_OPTION_LEN] = {0};
    int TableLen, dataLen = 0;

    get_dhcpV4Option60DesPtr(&option17_35_subOptions, &TableLen);
    if(NULL == option17_35_subOptions)
    {
        cmsLog_error("get_dhcpV4Option60DesPtr failed!");
        return -1;
    }

    if (CMSRET_SUCCESS != cmsDhcp_encapsulateSubOption(SubCode, option17_35_subOptions, 
        TableLen, NULL, dataFrame, &dataLen, 
        OPTION_CODE_LEN1, OPTION_SIZE_LEN1))
    {
        return CMSRET_INTERNAL_ERROR;
    }

    /* Hex to Hex ASCII string */
    if ((dataLen * 2) >= (*len))
    {
        cmsLog_error("str too long!");
        return -1;
    }
    dataOffset = 0;
    for (i = 0; i < dataLen && i < VDR_MAX_DHCP_OPTION_LEN; i++)
    {
        sprintf(&string[dataOffset], "%02x", dataFrame[i]);
        dataOffset += 2;
    }
    *len = dataLen * 2;

    return 0;
}


/* Please refer to DHCPv4: option43_getMacAddr() */
static int option17_getMacAddr(const void *parm, char *string, int *len)
{
    /* actually the SN in DeviceInfo is MacAddr, so get from SN directly */
    return option_getSN(parm, string, len);
}

/* EMTA */
static DhcpSubOptionTable option17_subOptionsEmta[] = 
{
    { 1,    OPTION_HEX_STRING,  "oro",             "002000210022087b0028",   NULL }, /* option 32, 33, 34, 2171, 40 */
    { 2,    OPTION_CHAR_STRING, "dev_type",        "EDVA",               NULL },
    { 4,    OPTION_CHAR_STRING, "sn",              "001018b0ff00",       option_getSN },
    { 5,    OPTION_CHAR_STRING, "hw_ver",          "V1.0",               option_getHwV },
    { 6,    OPTION_CHAR_STRING, "sw_ver",          "5.02L.04",           option_getSwV },
    { 7,    OPTION_CHAR_STRING, "bootroom_ver",    "CFE=1.0",            option_getBtV },
    { 8,    OPTION_CHAR_STRING, "OUI",             "001018",             option_getOUI },
    { 9,    OPTION_CHAR_STRING, "model_num",       "968580XREF",         option_getModelNum },
    { 10,   OPTION_CHAR_STRING, "vendor_name",     "Broadcom",           option_getVdrName },
    { 35,   OPTION_HEX_STRING,  "TLV5",            "00",                 option17_35getTLV5 },
    { 36,   OPTION_HEX_STRING,  "mac",             "001018b0ff00",       option17_getMacAddr },
    { 2172, OPTION_HEX_STRING,  "id_correlation",  "1DA82FF9",           NULL }
};

/* EPTA */
static DhcpSubOptionTable option17_subOptionsEpta[] = 
{
    { 1,    OPTION_HEX_STRING,  "oro",             "0028",               NULL }, /* option 40 */
    { 2,    OPTION_CHAR_STRING, "dev_type",        "EPTA",               NULL },
    { 4,    OPTION_CHAR_STRING, "sn",              "001018b0ff00",       option_getSN },
    { 5,    OPTION_CHAR_STRING, "hw_ver",          "V1.0",               option_getHwV },
    { 6,    OPTION_CHAR_STRING, "sw_ver",          "5.02L.04",           option_getSwV },
    { 7,    OPTION_CHAR_STRING, "bootroom_ver",    "CFE=1.0",            option_getBtV },
    { 8,    OPTION_CHAR_STRING, "OUI",             "001018",             option_getOUI },
    { 9,    OPTION_CHAR_STRING, "model_num",       "968580XREF",         option_getModelNum },
    { 10,   OPTION_CHAR_STRING, "vendor_name",     "Broadcom",           option_getVdrName },
    { 36,   OPTION_HEX_STRING,  "mac",             "001018b0ff00",       option17_getMacAddr },
    { 2172, OPTION_HEX_STRING,  "id_correlation",  "1DA82FF9",           NULL }
};


CmsRet rutDhcp6_createOption17(PKTCBL_WAN_TYPE wanType, const char *ifName)
{
    uint16_t code = DHCP_V6_VDR_SPECIFIC_INFO;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLen = 0;
    uint32_t enterprise_num = DHCP_ENTERPRISE_NUMBER_CTL;

    buffPtr = &optionFrame[VDR_OPTION_CODE_OFFSET];
    *(uint16_t *)buffPtr = htons(code);
    buffPtr += 2;
    totalLen += 2;
    
    /* skip option len first */
    buffPtr += 2;
    totalLen += 2;
    
    *(uint32_t *)buffPtr = htonl(enterprise_num);
    buffPtr += 4;
    totalLen += 4;
    
    if (PKTCBL_WAN_EMTA == wanType) /* EMTA */
    {
        if (CMSRET_SUCCESS != cmsDhcp_encapsulateSubOption(code, option17_subOptionsEmta, 
                sizeof(option17_subOptionsEmta)/sizeof(DhcpSubOptionTable), NULL, 
                buffPtr, &dataLen, OPTION_CODE_LEN2, OPTION_SIZE_LEN2))
        {
            return CMSRET_INTERNAL_ERROR;
        }
    }
    else if (PKTCBL_WAN_EPTA == wanType) /* EPTA */
    {
        if (CMSRET_SUCCESS != cmsDhcp_encapsulateSubOption(code, option17_subOptionsEpta, 
                sizeof(option17_subOptionsEpta)/sizeof(DhcpSubOptionTable), NULL, 
                buffPtr, &dataLen, OPTION_CODE_LEN2, OPTION_SIZE_LEN2))
        {
            return CMSRET_INTERNAL_ERROR;
        }
    }
    else
    {
        cmsLog_error("unknown wanType %d", wanType);
        return CMSRET_INTERNAL_ERROR;
    }

    buffPtr = &optionFrame[VDR_OPTION_V6_LEN_OFFSET]; 
    *(uint16_t *)buffPtr = htons(dataLen + 4); /* 4 for enterprise_num */
    
    totalLen += dataLen;
    return cmsDhcp_saveOption(DHCP_V6, ifName, code, optionFrame, totalLen);
}  

CmsRet rutDhcp6_getAcsUrlFromOption17(const char *ifName, char *acsURL, int inLen)
{
    uint16_t code = DHCP_V6_VDR_SPECIFIC_INFO;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLen = 0;
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
    if (CMSRET_SUCCESS != cmsDhcp_readOption(DHCP_V6, ifName, code, optionFrame, &totalLen))
    {        
        cmsLog_error("get option%d fail", code);
        return CMSRET_INTERNAL_ERROR;
    }

    buffPtr = &optionFrame[2];
    dataLen = ntohs(*(uint16_t *)buffPtr) - 4; /* ' -4' for removing size of enterprise number */

    /* check enterprise_num */
    buffPtr = &optionFrame[4];
    enterprise_num = ntohl(*(uint32_t *)buffPtr);
    buffPtr += 4;

    /* check enterprise_num */
    if (DHCP_ENTERPRISE_NUMBER_CTL != enterprise_num)
    {        
        cmsLog_error("unknow enterprise number %d", enterprise_num);
        return CMSRET_INVALID_ARGUMENTS;
    }

    subDataLen = sizeof(subOptionData);
    if (CMSRET_SUCCESS != cmsDhcp_getSubOptionData(DHCP_V6, buffPtr, dataLen, 
                    DHCP6_OPTION17_SUBOPTION40, subOptionData, &subDataLen))
    {
        cmsLog_error("get sub-option%d fail", DHCP6_OPTION17_SUBOPTION40);
        return CMSRET_INVALID_ARGUMENTS;
    }

    urlType = subOptionData[0];
    if ((17 == subDataLen) && (1 == urlType)) /* subDataLen == 17 && type == IPv6 address */
    {
        inet_ntop(AF_INET6, (void *)&subOptionData[1], acsURL, inLen);
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

    cmsLog_debug("acsURL=%s", acsURL);
    return CMSRET_SUCCESS;
}

CmsRet rutDhcp6_getNtpserversFromOption56(const char *ifName, char *ntpServerList, int inLen)
{
    uint16_t code = DHCP_V6_VDR_NTP_SERVER;
    char optionFrame[VDR_MAX_DHCP_OPTION_LEN] = {0}, *buffPtr = NULL;
    int totalLen = 0, dataLen = 0;
    char subOptionData[CMS_MAX_ACS_URL_LENGTH] = {0};
    int subDataLen = 0;
    int subCode;
    UBOOL8 found = FALSE;

    if (!ifName || !ntpServerList)
    {
        cmsLog_error("invalid params");
        return CMSRET_INVALID_ARGUMENTS;
    }

    totalLen = sizeof(optionFrame);
    if (CMSRET_SUCCESS != cmsDhcp_readOption(DHCP_V6, ifName, code, optionFrame, &totalLen))
    {        
        cmsLog_error("get option%d fail", code);
        return CMSRET_INTERNAL_ERROR;
    }

    buffPtr = &optionFrame[2];
    dataLen = ntohs(*(uint16_t *)buffPtr);
    buffPtr += 2;

    for (subCode = 1; subCode <= 3; subCode++)
    {
        subDataLen = sizeof(subOptionData);
        memset(subOptionData, 0, subDataLen);
        if (CMSRET_SUCCESS != cmsDhcp_getSubOptionData(DHCP_V6, buffPtr, dataLen, 
                        subCode, subOptionData, &subDataLen))
        {
            cmsLog_notice("get sub-option%d fail", subCode);
            continue;
        }

        if ((DHCP6_OPTION56_SUBOPTION1 == subCode) || (DHCP6_OPTION56_SUBOPTION2 == subCode))
        {
            if (subDataLen != 16)
            {
                cmsLog_error("subCode=%d: invalid subDataLen=%d", subCode, subDataLen);
                return CMSRET_INVALID_ARGUMENTS;
            }

            inet_ntop(AF_INET6, (void *)&subOptionData[0], ntpServerList, inLen);
            found = TRUE;
            break;
        }
        else if ((DHCP6_OPTION56_SUBOPTION3 == subCode))
        {
            strncpy(ntpServerList, subOptionData, (subDataLen < inLen) ? subDataLen : inLen);
            found = TRUE;
            break;
        }      
    }

    if (!found)
    {
        cmsLog_error("not found");
        return CMSRET_INVALID_ARGUMENTS;
    }

    cmsLog_debug("ntpServerList=%s", ntpServerList);
    return CMSRET_SUCCESS;
}

#endif // BRCM_PKTCBL_SUPPORT

#endif // SUPPORT_IPV6
