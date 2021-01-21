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
 * rut_dhcp_common.h
 *
 *  Created on:  Sep. 2017
 *      Author: Fuguo Xu <fuguo.xu@broadcom.com>
 */


/*
 * the defines in this file is used for both DHCPv4 and DHCPv6
 */


#ifndef __RUT_DHCP_COMMON_H__
#define __RUT_DHCP_COMMON_H__


#if defined(BRCM_PKTCBL_SUPPORT)
#if defined(DMP_DEVICE2_BASELINE_1)
#define option_getDevInfoX(X, outStr, len)               \
do                                                       \
{                                                        \
    Dev2DeviceInfoObject *deviceInfoObj=NULL;             \
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;  \
    CmsRet ret = 0;                                      \
                                                         \
    if ((ret = cmsObj_get(MDMOID_DEV2_DEVICE_INFO, &iidStack, 0, (void **) &deviceInfoObj)) != CMSRET_SUCCESS) \
    {                                                                        \
        cmsLog_error("could not get device info object!, ret=%d", ret);      \
        return -1;                                                           \
    }                                                                        \
                                                                             \
    if (deviceInfoObj->X) {                                                  \
        if (strlen(deviceInfoObj->X) < (*len)) {                             \
            *len = sprintf(outStr, "%s", deviceInfoObj->X);                  \
        }                                                                    \
        else {                                                               \
            cmsLog_error("str too long!");                                   \
            ret = -1;                                                        \
        }                                                                    \
    }                                                                        \
                                                                             \
    cmsObj_free((void**)&deviceInfoObj);                                     \
    return ret;                                                              \
} while(0)
#else
#define option_getDevInfoX(X, outStr, len)               \
do                                                       \
{                                                        \
    IGDDeviceInfoObject *deviceInfoObj=NULL;             \
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;  \
    CmsRet ret = 0;                                      \
                                                         \
    if ((ret = cmsObj_get(MDMOID_IGD_DEVICE_INFO, &iidStack, 0, (void **) &deviceInfoObj)) != CMSRET_SUCCESS) \
    {                                                                        \
        cmsLog_error("could not get device info object!, ret=%d", ret);      \
        return -1;                                                           \
    }                                                                        \
                                                                             \
    if (deviceInfoObj->X) {                                                  \
        if (strlen(deviceInfoObj->X) < (*len)) {                             \
            *len = sprintf(outStr, "%s", deviceInfoObj->X);                  \
        }                                                                    \
        else {                                                               \
            cmsLog_error("str too long!");                                   \
            ret = -1;                                                        \
        }                                                                    \
    }                                                                        \
                                                                             \
    cmsObj_free((void**)&deviceInfoObj);                                     \
    return ret;                                                              \
} while(0)  
#endif // DMP_DEVICE2_BASELINE_1

static inline int option_getSN(const void *parm, char *string, int *len)
{
    option_getDevInfoX(serialNumber, string, len);
}

static inline int option_getHwV(const void *parm, char *string, int *len)
{
    option_getDevInfoX(hardwareVersion, string, len);
}

static inline int option_getSwV(const void *parm, char *string, int *len)
{
    option_getDevInfoX(softwareVersion, string, len);
}

static inline int option_getBtV(const void *parm, char *string, int *len)
{
    option_getDevInfoX(additionalSoftwareVersion, string, len);
}

static inline int option_getOUI(const void *parm, char *string, int *len)
{
    option_getDevInfoX(manufacturerOUI, string, len);
}

static inline int option_getModelNum(const void *parm, char *string, int *len)
{
    option_getDevInfoX(modelName, string, len);
}

static inline int option_getVdrName(const void *parm, char *string, int *len)
{
    option_getDevInfoX(manufacturer, string, len);
}

#endif // BRCM_PKTCBL_SUPPORT

#endif // __RUT_DHCP_COMMON_H__

