/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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

#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_WIFIENDPOINT_1

/*!\file rcl2_wifi_endpoint.c
 * \brief This file contains TR181 and X_BROADCOM_COM Wifi End Point (WAN side
 *        or Upstream facing) functions.  General Wifi objects (wifi and radio)
 *        and Access Point specific functions are in separate files.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "cms_qdm.h"



CmsRet rcl_dev2WifiEndPointObject( _Dev2WifiEndPointObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   /* example for getting radio index
   UINT32 radioIndex;
   radioIndex = qdmWifi_getRadioIndexBySsidFullPathLocked_dev2(newObj->SSIDReference);
   */

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointStatsObject( _Dev2WifiEndPointStatsObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointStatsObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointSecurityObject( _Dev2WifiEndPointSecurityObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointSecurityObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointProfileObject( _Dev2WifiEndPointProfileObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointProfileObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointProfileSecurityObject( _Dev2WifiEndPointProfileSecurityObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointProfileSecurityObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointWpsObject( _Dev2WifiEndPointWpsObject *newObj __attribute__((unused)),
                      const _Dev2WifiEndPointWpsObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointAcObject( _Dev2WifiEndPointAcObject *newObj,
                const _Dev2WifiEndPointAcObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiEndPointAcStatsObject( _Dev2WifiEndPointAcStatsObject *newObj,
                const _Dev2WifiEndPointAcStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_WIFIENDPOINT_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */
