/***********************************************************************
 *
 *  Copyright (c) 2006-2012  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2012:proprietary:standard
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
 ************************************************************************/

#include "rcl.h"
#include "rut_util.h"
#include "cms_util.h"
#include "rut_system.h"


/*
 * Runtime Config Layer (RCL) handler functions go here.
 * This file contains the RCL handler functions for the top level objects.
 * Other RCL handler functions have been broken out into their own file, i.e.
 * rcl_xxx.c
 *
 * Note this file is in the OS dependent sub-directory because
 * all RCL handler functions are likely to be OS dependent.
 */

CmsRet rcl_igdObject( _IGDObject *newObj __attribute__((unused)),
                const _IGDObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_igdDeviceInfoObject( _IGDDeviceInfoObject *newObj __attribute__((unused)),
                const _IGDDeviceInfoObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_deviceConfigObject( _DeviceConfigObject *newObj,
                const _DeviceConfigObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam,
                CmsRet *errorCode)
{
   CmsRet ret = CMSRET_SUCCESS;

   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    */
   if (ADD_NEW(newObj, currObj))
   {
      return ret;
   }
   if (cmsUtl_strcmp(newObj->persistentData,currObj->persistentData) != 0)
   {
      rutSys_setDevicePersistentData(newObj->persistentData, strlen(newObj->persistentData));
   }
   if ((newObj->configFile != NULL) && (newObj->configFile[0] != '\0'))
   {
      ret = rutSys_setRunningConfigFile(newObj->configFile,strlen(newObj->configFile));
      if (ret == CMSRET_SUCCESS)
      {
         /*
          * New config file was accepted and written to flash, but the system
          * needs to be rebooted before settings can take effect.
          */
         ret = CMSRET_SUCCESS_REBOOT_REQUIRED;
      }
      else
      {
         *errorParam = cmsMem_strdupFlags("ConfigFile",mdmLibCtx.allocFlags);
         *errorCode = ret;
      }
   }
   return ret;
}

#ifdef NOT_SUPPORTED
CmsRet rcl_userInterfaceCfgObject( _UserInterfaceCfgObject *newObj __attribute__((unused)),
                const _UserInterfaceCfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}
#endif

#ifdef NOT_SUPPORTED
CmsRet rcl_lanconfigSecurityObject( _LanconfigSecurityObject *newObj __attribute__((unused)),
                const _LanconfigSecurityObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}
#endif

CmsRet rcl_servicesObject( _ServicesObject *newObj __attribute__((unused)),
                const _ServicesObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}


#ifdef DMP_BASELINE_1
/* this is the TR98 version of TR69 */

CmsRet rcl_vendorConfigFileObject( _VendorConfigFileObject *newObj,
                const _VendorConfigFileObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* this object is mainly used by TR69.  It records the vendor config file(s)
    * downloaded into the modem via RPC DOWNLOAD method.
    */
   if (ADD_NEW(newObj, currObj))
   {
      rut_modifyNumVendorConfigFiles(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rut_modifyNumVendorConfigFiles(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}
#endif  /* DMP_BASELINE_1 */


#ifdef DMP_X_BROADCOM_COM_EPON_1
CmsRet rcl_eponSoftwareImageObject( _EponSoftwareImageObject *newObj __attribute__((unused)),
                const _EponSoftwareImageObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    return ret;
}

CmsRet rcl_eponSwUpgradeStatusObject( _EponSwUpgradeStatusObject *newObj __attribute__((unused)),
                const _EponSwUpgradeStatusObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    return ret;
}
#endif

