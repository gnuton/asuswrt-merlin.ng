/***********************************************************************
 *
 *  Copyright (c) 2014  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2014:proprietary:standard

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

#include "odl.h"
#include "cms.h"
#include "mdm.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut2_util.h"

static void modifyNumVendorConfigFiles(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DEVICE_INFO,
                                 MDMOID_DEV2_DEVICE_VENDOR_CONFIG_FILE, iidStack, delta);
}

static void modifyNumSupportedDataModel(const InstanceIdStack *iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_DEVICE_INFO,
                                 MDMOID_DEV2_SUPPORTED_DATA_MODEL, iidStack, delta);
}

CmsRet rcl_dev2DeviceVendorConfigFileObject( _Dev2DeviceVendorConfigFileObject *newObj,
                                             const _Dev2DeviceVendorConfigFileObject *currObj,
                                             const InstanceIdStack *iidStack,
                                             char **errorParam __attribute__((unused)),
                                             CmsRet *errorCode __attribute__((unused)))
{
   /* this object is mainly used by TR69.  It records the vendor config file(s)
    * downloaded into the modem via RPC DOWNLOAD method.
    */
   if (ADD_NEW(newObj, currObj))
   {
      modifyNumVendorConfigFiles(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      modifyNumVendorConfigFiles(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2SupportedDataModelObject(_Dev2SupportedDataModelObject *newObj,
                                        const _Dev2SupportedDataModelObject *currObj,
                                        const InstanceIdStack *iidStack,
                                        char **errorParam __attribute__((unused)),
                                        CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      modifyNumSupportedDataModel(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      modifyNumSupportedDataModel(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_BASELINE_1 */

