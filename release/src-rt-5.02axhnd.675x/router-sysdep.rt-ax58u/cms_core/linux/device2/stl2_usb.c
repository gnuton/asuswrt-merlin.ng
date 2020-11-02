/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2011:proprietary:standard

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
#include "stl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_system.h"

/*!\file stl2_usb.c
 * \brief This file contains USB related functions.
 *
 */

#ifdef DMP_DEVICE2_USBINTERFACE_1

CmsRet stl_dev2UsbObject(_Dev2UsbObject *obj __attribute__((unused)),
                         const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2UsbInterfaceObject(_Dev2UsbInterfaceObject *obj,
                                  const InstanceIdStack *iidStack __attribute__((unused)))
{
   char currentStatus[BUFLEN_16];
   char macAddrStr[MAC_STR_LEN+1]={0};
   CmsRet ret = CMSRET_SUCCESS;

   if (cmsUtl_strlen(obj->name) == 0)
   {
      cmsLog_debug("name not set, do nothing");
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   // obj->name is eth0, eth1, eth2, etc...
   rutLan_getIntfStatus(obj->name, currentStatus, macAddrStr);
   if (!cmsUtl_strcmp(currentStatus, MDMVS_DISABLED))
   {
      /* map rutLan_getIntfStatus of TR98 DISABLED to our TR181 DORMANT */
      sprintf(currentStatus, MDMVS_DORMANT);
   }
   cmsLog_debug("name %s, status %s, currentStatus %s, macAddr %s",
                 obj->name, obj->status, currentStatus, macAddrStr);


   if (cmsUtl_strcmp(obj->status, currentStatus))
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, currentStatus, mdmLibCtx.allocFlags);
      obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
   }

   /* Calculate and return the TR181 LastChange */
   IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);


   if (cmsUtl_strcmp(obj->MACAddress, macAddrStr))
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->MACAddress, macAddrStr, mdmLibCtx.allocFlags);
   }

   return ret;
}


CmsRet stl_dev2UsbInterfaceStatsObject(_Dev2UsbInterfaceStatsObject *obj,
                                       const InstanceIdStack *iidStack)
{
   Dev2UsbInterfaceObject *usbIntf = NULL;
   InstanceIdStack parentIidStack = *iidStack;
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsObj_getAncestor(MDMOID_DEV2_USB_INTERFACE,
                                 MDMOID_DEV2_USB_INTERFACE_STATS,
                                 &parentIidStack,
                                 (void **) &usbIntf)) == CMSRET_SUCCESS)
   {
      if (obj == NULL)
      {
         rut_clearIntfStats(usbIntf->name);
      }
      else
      {
         UINT64 bytesReceived = 0, packetsReceived = 0, multicastBytesReceived = 0, multicastPacketsReceived = 0;
         UINT64 unicastPacketsReceived = 0, broadcastPacketsReceived = 0, errorsReceived = 0, discardPacketsReceived = 0;
         UINT64 bytesSent = 0, packetsSent = 0, multicastBytesSent = 0, multicastPacketsSent = 0;
         UINT64 unicastPacketsSent = 0, broadcastPacketsSent = 0, errorsSent = 0, discardPacketsSent = 0;

         rut_getIntfStats_uint64(usbIntf->name, &bytesReceived, &packetsReceived,
                                 &multicastBytesReceived, &multicastPacketsReceived, &unicastPacketsReceived, &broadcastPacketsReceived, 
                                 &errorsReceived, &discardPacketsReceived,
                                 &bytesSent, &packetsSent,
                                 &multicastBytesSent, &multicastPacketsSent, &unicastPacketsSent, &broadcastPacketsSent,
                                 &errorsSent, &discardPacketsSent);

         obj->bytesReceived = bytesReceived;
         obj->packetsReceived = packetsReceived;
         obj->X_BROADCOM_COM_MulticastBytesReceived = multicastBytesReceived;
         obj->multicastPacketsReceived = multicastPacketsReceived;
         obj->unicastPacketsReceived = unicastPacketsReceived;
         obj->broadcastPacketsReceived = broadcastPacketsReceived;
         obj->errorsReceived = (UINT32)errorsReceived;
         obj->discardPacketsReceived = (UINT32)discardPacketsReceived;

         obj->bytesSent = bytesSent;
         obj->packetsSent = packetsSent;
         obj->X_BROADCOM_COM_MulticastBytesSent = multicastBytesSent;
         obj->multicastPacketsSent = multicastPacketsSent;
         obj->unicastPacketsSent = unicastPacketsSent;
         obj->broadcastPacketsSent = broadcastPacketsSent;
         obj->errorsSent = (UINT32)errorsSent;
         obj->discardPacketsSent = (UINT32)discardPacketsSent;
      }

      cmsObj_free((void **) &usbIntf);
   }
   
   return ret;
}




#endif /* DMP_DEVICE2_USBINTERFACE_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */


#ifdef DMP_DEVICE2_USBHOSTSBASIC_1
CmsRet stl_dev2UsbHostsObject(_Dev2UsbHostsObject *obj __attribute__((unused)),
                              const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2UsbHostObject(_Dev2UsbHostObject *obj __attribute__((unused)),
                             const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2UsbHostDeviceObject(_Dev2UsbHostDeviceObject *obj __attribute__((unused)),
                                   const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#ifdef DMP_DEVICE2_USBHOSTSADV_1
CmsRet stl_dev2UsbHostDeviceConfigObject(_Dev2UsbHostDeviceConfigObject *obj __attribute__((unused)),
                                   const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2UsbHostDeviceConfigIfcObject(_Dev2UsbHostDeviceConfigIfcObject *obj __attribute__((unused)),
                                   const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif
#endif

