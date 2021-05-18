/***********************************************************************
 *
 *  Copyright (c) 2006-2009  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"


/*!\file rcl_usb.c
 * \brief This file contains Device2 USB related functions.
 *
 */

#ifdef DMP_DEVICE2_USBINTERFACE_1

CmsRet rcl_dev2UsbObject( _Dev2UsbObject *newObj __attribute__((unused)),
                const _Dev2UsbObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2UsbInterfaceObject( _Dev2UsbInterfaceObject *newObj,
                const _Dev2UsbInterfaceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumEthInterface(iidStack, 1);
   }

   if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumEthInterface(iidStack, -1);
   }
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2UsbInterfaceStatsObject( _Dev2UsbInterfaceStatsObject *newObj __attribute__((unused)),
                const _Dev2UsbInterfaceStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#endif /* DMP_DEVICE2_USBINTERFACE_1 */

#else
/* DMP_DEVICE2_BASELINE_1 is not defined */

#ifdef DMP_DEVICE2_USBINTERFACE_1
#error "Device2 USB interface objects incompatible with current Data Model mode, go to make menuconfig to fix"
#endif

#endif  /* DMP_DEVICE2_BASELINE_1 */


/*
In TR181, DMP_DEVICE2_USBHOSTSBASIC_1 shoud depend on DMP_DEVICE2_BASELINE_1 and DMP_DEVICE2_USBINTERFACE_1.
Now the dependency is made at make.common, DMP_DEVICE2_USBHOSTSBASIC_1 relies on DMP_DEVICE2_BASELINE_1 & DMP_DEVICE2_USBINTERFACE_1. 
In future, new datamodel can also be added to tr98 so that this dependency could easily be changed. 
*/
#ifdef DMP_DEVICE2_USBHOSTSBASIC_1
CmsRet rcl_dev2UsbHostsObject( _Dev2UsbHostsObject *newObj __attribute__((unused)),
                const _Dev2UsbHostsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2UsbHostObject( _Dev2UsbHostObject *newObj,
                const _Dev2UsbHostObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   //Sarah: todo: config usb host
   
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2UsbHostDeviceObject( _Dev2UsbHostDeviceObject *newObj,
                const _Dev2UsbHostDeviceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   cmsLog_debug("Enter");
   
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumUsbDevice(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumUsbDevice(iidStack, -1);
   }  

   return CMSRET_SUCCESS;
}

#ifdef DMP_DEVICE2_USBHOSTSADV_1
CmsRet rcl_dev2UsbHostDeviceConfigObject( _Dev2UsbHostDeviceConfigObject *newObj,
                const _Dev2UsbHostDeviceConfigObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   cmsLog_debug("Enter");
   
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumUsbDeviceConfig(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumUsbDeviceConfig(iidStack, -1);
   }  

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2UsbHostDeviceConfigIfcObject( _Dev2UsbHostDeviceConfigIfcObject *newObj,
                const _Dev2UsbHostDeviceConfigIfcObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   cmsLog_debug("Enter");
   
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumUsbDeviceConfigIfc(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumUsbDeviceConfigIfc(iidStack, -1);
   }  

   return CMSRET_SUCCESS;
}
#endif
#endif

