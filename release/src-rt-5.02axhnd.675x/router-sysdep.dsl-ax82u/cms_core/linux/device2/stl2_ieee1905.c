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
#ifdef DMP_DEVICE2_IEEE1905BASELINE_1
#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut2_ieee1905.h"
#include "i5api.h"

CmsRet stl_dev2Ieee1905Object(_Dev2Ieee1905Object *obj, const InstanceIdStack *iidStack)
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Ieee1905AlObject(_Dev2Ieee1905AlObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS;
}

CmsRet stl_dev2Ieee1905AlIfcObject(_Dev2Ieee1905AlIfcObject *obj, const InstanceIdStack *iidStack)
{
   CmsRet ret = CMSRET_SUCCESS;

#if 0
   char currentStatus[BUFLEN_16];
   char macAddrStr[MAC_STR_LEN+1]={0};

   if (cmsUtl_strlen(obj->X_BROADCOM_COM_Name) == 0)
   {
      cmsLog_debug("name not set, do nothing");
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   // obj->name is eth0, eth1, eth2, etc...
   rutLan_getIntfStatus(obj->X_BROADCOM_COM_Name, currentStatus, macAddrStr);

   if (!cmsUtl_strcmp(currentStatus, MDMVS_DISABLED))
   {
      /* map rutLan_getIntfStatus of TR98 DISABLED to our TR181 DORMANT */
      sprintf(currentStatus, MDMVS_DORMANT);
   }
   cmsLog_debug("name %s, status %s, currentStatus %s, macAddr %s",
                 obj->X_BROADCOM_COM_Name, obj->status, currentStatus, macAddrStr);


   if (cmsUtl_strcmp(obj->status, currentStatus))
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, currentStatus, mdmLibCtx.allocFlags);
      obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
   }

   /* Calculate and return the TR181 LastChange */
   IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);

#endif
   return ret;
}

CmsRet stl_dev2Ieee1905AlSecurityObject(_Dev2Ieee1905AlSecurityObject *obj, const InstanceIdStack *iidStack)
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#if defined(DMP_DEVICE2_IEEE1905LINKMETRIC_1)

CmsRet stl_dev2Ieee1905AlIfcLinkObject(_Dev2Ieee1905AlIfcLinkObject *obj, const InstanceIdStack *iidStack)
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Ieee1905AlIfcLinkMetricObject(_Dev2Ieee1905AlIfcLinkMetricObject *obj, const InstanceIdStack *iidStack)
{
   _Dev2Ieee1905AlIfcLinkObject *interfaceLink1905 = NULL;
   InstanceIdStack parentIidStack = *iidStack;
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsObj_getAncestor(MDMOID_DEV2_IEEE1905_AL_IFC_LINK,
                                 MDMOID_DEV2_IEEE1905_AL_IFC_LINK_METRIC,
                                 &parentIidStack,
                                 (void **) &interfaceLink1905)) == CMSRET_SUCCESS)
   {
      if (obj != NULL)
      {
         UINT32    packetErrors;
         UINT32    packetErrorsReceived;
         UINT32    transmittedPackets;
         UINT32    packetsReceived;
         UINT32    MACThroughputCapacity;
         UINT32    linkAvailability;
         UINT32    PHYRate;
         UINT32    RSSI;
         rutIeee1905_getLocalLinkStats(interfaceLink1905->interfaceId,
          &packetErrors, &packetErrorsReceived, &transmittedPackets, &packetsReceived, &MACThroughputCapacity,
          &linkAvailability, &PHYRate, &RSSI);

         obj->packetErrors = packetErrors;
         obj->packetErrorsReceived = packetErrorsReceived;
         obj->transmittedPackets = transmittedPackets;
         obj->packetsReceived = packetsReceived;
         obj->MACThroughputCapacity = MACThroughputCapacity;
         obj->linkAvailability = linkAvailability;
         obj->PHYRate = PHYRate;
         obj->RSSI = RSSI;
      }
      cmsObj_free((void **) &interfaceLink1905);
   }
   return ret;
}

#endif   /* DMP_DEVICE2_IEEE1905LINKMETRIC_1 */

#if defined(DMP_DEVICE2_IEEE1905NETWORKTOPOLOGY_1)

CmsRet stl_dev2Ieee1905AlNetworkTopologyObject(_Dev2Ieee1905AlNetworkTopologyObject *obj, const InstanceIdStack *iidStack)
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Ieee1905AlNetworkTopologyChangeLogObject(_Dev2Ieee1905AlNetworkTopologyChangeLogObject *obj, const InstanceIdStack *iidStack)
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceObject(_Dev2Ieee1905AlNetworkTopologyIeee1905DeviceObject *obj, const InstanceIdStack *iidStack)
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceIfcObject(_Dev2Ieee1905AlNetworkTopologyIeee1905DeviceIfcObject *obj, const InstanceIdStack *iidStack)
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceNonIeee1905NeighborObject(_Dev2Ieee1905AlNetworkTopologyIeee1905DeviceNonIeee1905NeighborObject *obj, const InstanceIdStack *iidStack)
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceNonL2NeighborObject(_Dev2Ieee1905AlNetworkTopologyIeee1905DeviceNonL2NeighborObject *obj, const InstanceIdStack *iidStack)
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceBridgingTupleObject(_Dev2Ieee1905AlNetworkTopologyIeee1905DeviceBridgingTupleObject *obj, const InstanceIdStack *iidStack)
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceIeee1905NeighborObject(_Dev2Ieee1905AlNetworkTopologyIeee1905DeviceIeee1905NeighborObject *obj, const InstanceIdStack *iidStack)
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceIeee1905NeighborMetricObject(_Dev2Ieee1905AlNetworkTopologyIeee1905DeviceIeee1905NeighborMetricObject *obj, const InstanceIdStack *iidStack)
{
   _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceObject *alNetTopDeviceObj = NULL;
   Dev2Ieee1905AlNetworkTopologyIeee1905DeviceIeee1905NeighborObject *alNetTopDeviceNeighborObj = NULL;
   InstanceIdStack parentIidStack = *iidStack;
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = cmsObj_getAncestor(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR,
                                 MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR_METRIC,
                                 &parentIidStack,
                                 (void **) &alNetTopDeviceNeighborObj)) == CMSRET_SUCCESS)
   {
      if ((ret = cmsObj_getAncestor(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE,
                                    MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR,
                                    &parentIidStack,
                                    (void **) &alNetTopDeviceObj)) == CMSRET_SUCCESS)

      {
         if (obj != NULL)
         {
            UINT32    packetErrors = 0;
            UINT32    packetErrorsReceived = 0;
            UINT32    transmittedPackets = 0;
            UINT32    packetsReceived = 0;
            UINT32    MACThroughputCapacity = 0;
            UINT32    linkAvailability = 0;
            UINT32    PHYRate = 0;
            UINT32    RSSI = 0;
            if (rutIeee1905_getDeviceLinkStats(alNetTopDeviceObj->IEEE1905Id ,obj->neighborMACAddress,
                                               &packetErrors, &packetErrorsReceived, &transmittedPackets, 
                                               &packetsReceived, &MACThroughputCapacity,
                                               &linkAvailability, &PHYRate, &RSSI) == CMSRET_SUCCESS)
            {    
               obj->packetErrors = packetErrors;
               obj->packetErrorsReceived = packetErrorsReceived;
               obj->transmittedPackets = transmittedPackets;
               obj->packetsReceived = packetsReceived;
               obj->MACThroughputCapacity = MACThroughputCapacity;
               obj->linkAvailability = linkAvailability;
               obj->PHYRate = PHYRate;
               obj->RSSI = RSSI;
            }
         }
      }
   }

   if (alNetTopDeviceNeighborObj)
      cmsObj_free((void **) &alNetTopDeviceNeighborObj);
   if (alNetTopDeviceObj)
      cmsObj_free((void **) &alNetTopDeviceObj);
   return ret;
}

#endif   /* DMP_DEVICE2_IEEE1905NETWORKTOPOLOGY_1 */

#endif   /* DMP_DEVICE2_IEEE1905BASELINE_1 */
