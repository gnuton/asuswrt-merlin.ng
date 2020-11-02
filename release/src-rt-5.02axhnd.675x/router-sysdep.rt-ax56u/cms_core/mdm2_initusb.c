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
#ifdef DMP_DEVICE2_USBHOSTSBASIC_1

#include <unistd.h>

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"

#define MAX_USB_HOSTS 16

CmsRet mdm_addDefaultUsbHostObject(void)
{
   Dev2UsbHostObject *usbHostObj;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   int i;   
   char usbDirName[BUFLEN_32] = "/sys/bus/usb/devices/usb";
   char buf[BUFLEN_64];   
   CmsRet ret;

   /* look for Device.USB.USBHosts.Host.1. */
   if(mdm_getNextObject(MDMOID_DEV2_USB_HOST, &iidStack, (void **)&usbHostObj) == CMSRET_SUCCESS)
   {
      /*Usb host object already created nothing to be done*/
      mdm_freeObject((void **)&usbHostObj);
      return  CMSRET_SUCCESS;  
   }

   cmsLog_notice("Creating Device.USB.USBHosts.Host. sub-tree");
   
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_USB_HOST;
 
   for (i=1; i<=MAX_USB_HOSTS; i++)
   {
      /*append i to usbDirName, make it to /sys/bus/usb/devices/usbX*/   
      sprintf(&usbDirName[24],"%d",i);   

      if (access(usbDirName, F_OK))
      {
         break;
      }
 
      cmsLog_debug("addObjectInstance for %s", usbDirName);

      INIT_INSTANCE_ID_STACK(&pathDesc.iidStack);
	  
      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_addDefault USB_HOST Objects: Failed\n");      
         return ret;
      }
 
     if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &usbHostObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to get USB_HOST object, ret=%d", ret);
         return ret;
      }

      usbHostObj->enable = TRUE;
      /*get usbX from usbDirName*/   
      CMSMEM_REPLACE_STRING_FLAGS(usbHostObj->name, &usbDirName[21], mdmLibCtx.allocFlags);

      if (cmsFil_readFirstlineFromFileWithBasedir(usbDirName, "product", buf, sizeof(buf)) == CMSRET_SUCCESS)
      {
         /* Terminate at XHCI */
         buf[4] = '\0'; 
         CMSMEM_REPLACE_STRING_FLAGS(usbHostObj->type, buf, mdmLibCtx.allocFlags);
      }

#ifdef DMP_DEVICE2_USBHOSTSADV_1
      //Sarah: todo: check if powerManagementEnable
      //usbHostObj->powerManagementEnable = FALSE; 
#endif
 	 
      if (cmsFil_readFirstlineFromFileWithBasedir(usbDirName, "version", buf, sizeof(buf)) == CMSRET_SUCCESS)
      {
        CMSMEM_REPLACE_STRING_FLAGS(usbHostObj->USBVersion, &buf[1], mdmLibCtx.allocFlags);
      }
 	 
      if ((ret = mdm_setObject((void **) &usbHostObj, &pathDesc.iidStack,  FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set USB_HOST. ret=%d", ret);
      }
	 
      mdm_freeObject((void **)&usbHostObj);
 
   }

   /* need to manually update the count when adding objects during mdm_init */
   if (i > 1)   
   {
      Dev2UsbHostsObject *usbHostsObj=NULL;
      InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;

      if ((ret = mdm_getObject(MDMOID_DEV2_USB_HOSTS, &iidStack, (void **) &usbHostsObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get DEV2_USB_HOSTS. ret=%d", ret);
         return ret;
      }

      usbHostsObj->hostNumberOfEntries = i - 1;
      ret = mdm_setObject((void **) &usbHostsObj, &iidStack,  FALSE);
      mdm_freeObject((void **)&usbHostsObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set DEV2_USB_HOSTS. ret=%d", ret);
      }
   }

   return  CMSRET_SUCCESS;  
}

#endif
