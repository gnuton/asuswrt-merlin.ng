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
#include "rut2_util.h"
#include "rut2_ieee1905.h"

CmsRet rcl_dev2Ieee1905Object( _Dev2Ieee1905Object *newObj,
                const _Dev2Ieee1905Object *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2Ieee1905AlObject( _Dev2Ieee1905AlObject *newObj,
                const _Dev2Ieee1905AlObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj,currObj)) {
        cmsLog_debug("Adding new ieee1905 object... %d, %d", newObj ? newObj->enable : 0xFF, currObj ? currObj->enable : 0xFF);
        rutIeee1905_initialize(newObj);
    }
    else if (POTENTIAL_CHANGE_OF_EXISTING(newObj,currObj)) {
        cmsLog_debug("Edit 1905 Entries... %d, %d", newObj ? newObj->enable : 0xFF, currObj ? currObj->enable : 0xFF);
        rutIeee1905_updateAl(newObj);
#if defined(DMP_DEVICE2_IEEE1905NETWORKTOPOLOGY_1)
        rutIeee1905_updateNtDevice(newObj->IEEE1905Id, newObj->deviceFriendlyName);
#endif
    }
    else if (DELETE_OR_DISABLE_EXISTING(newObj,currObj)) {
        cmsLog_debug("Disabling ieee1905 object... %d, %d", newObj ? newObj->enable : 0xFF, currObj ? currObj->enable : 0xFF);
        rutIeee1905_terminate();
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Ieee1905AlIfcObject( _Dev2Ieee1905AlIfcObject *newObj,
                const _Dev2Ieee1905AlIfcObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    /* Increase number of objects */
    if (ADD_NEW(newObj, currObj))
    {
       cmsLog_debug("rcl adding new IEEE1905 interface %s", newObj->interfaceId);
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL, MDMOID_DEV2_IEEE1905_AL_IFC, iidStack, 1);
    }
    /* if delete, update number of object entries */
    else if (DELETE_EXISTING(newObj, currObj))
    {
       cmsLog_debug("rcl delete IEEE1905 interface %s", currObj->interfaceId);
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL, MDMOID_DEV2_IEEE1905_AL_IFC, iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Ieee1905AlSecurityObject( _Dev2Ieee1905AlSecurityObject *newObj,
                const _Dev2Ieee1905AlSecurityObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}


#if defined(DMP_DEVICE2_IEEE1905LINKMETRIC_1)
CmsRet rcl_dev2Ieee1905AlIfcLinkObject( _Dev2Ieee1905AlIfcLinkObject *newObj,
                const _Dev2Ieee1905AlIfcLinkObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    /* Increase number of objects */
    if (ADD_NEW(newObj, currObj))
    {
       cmsLog_debug("rcl adding new IEEE1905 interface  link");
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_IFC, MDMOID_DEV2_IEEE1905_AL_IFC_LINK, iidStack, 1);
    }
    /* if delete, update number of object entries */
    else if (DELETE_EXISTING(newObj, currObj))
    {
       cmsLog_debug("rcl delete IEEE1905 interface link");
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_IFC, MDMOID_DEV2_IEEE1905_AL_IFC_LINK, iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Ieee1905AlIfcLinkMetricObject( _Dev2Ieee1905AlIfcLinkMetricObject *newObj,
                const _Dev2Ieee1905AlIfcLinkMetricObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

#endif   /* DMP_DEVICE2_IEEE1905LINKMETRIC_1 */

#if defined(DMP_DEVICE2_IEEE1905NETWORKTOPOLOGY_1)

CmsRet rcl_dev2Ieee1905AlNetworkTopologyObject( _Dev2Ieee1905AlNetworkTopologyObject *newObj,
                const _Dev2Ieee1905AlNetworkTopologyObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   if ( ENABLE_EXISTING(newObj, currObj) ||
        DELETE_OR_DISABLE_EXISTING(newObj, currObj) )
   {
      rutIeee1905_updateNetworkTopology(newObj);
   }
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Ieee1905AlNetworkTopologyChangeLogObject( _Dev2Ieee1905AlNetworkTopologyChangeLogObject *newObj,
                const _Dev2Ieee1905AlNetworkTopologyChangeLogObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceObject( _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceObject *newObj,
                const _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    /* Increase number of objects */
    if (ADD_NEW(newObj, currObj))
    {
       cmsLog_debug("rcl adding new MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE");
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY, MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE, iidStack, 1);
    }
    /* if delete, update number of object entries */
    else if (DELETE_EXISTING(newObj, currObj))
    {
       cmsLog_debug("rcl delete MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE");
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY, MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE, iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceIfcObject( _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceIfcObject *newObj,
                const _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceIfcObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    /* Increase number of objects */
    if (ADD_NEW(newObj, currObj))
    {
       cmsLog_debug("rcl adding new MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IFC");
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE, MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IFC, iidStack, 1);
    }
    /* if delete, update number of object entries */
    else if (DELETE_EXISTING(newObj, currObj))
    {
       cmsLog_debug("rcl delete MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IFC");
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE, MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IFC, iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceBridgingTupleObject( _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceBridgingTupleObject *newObj,
                const _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceBridgingTupleObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    /* Increase number of objects */
    if (ADD_NEW(newObj, currObj))
    {
       cmsLog_debug("rcl adding new MDMOID_DEV2_IEEE1905_AL_NET_TOPOLOGY_IEEE1905_DEVICE_BRIDGING_TUPLE");
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE, MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_BRIDGING_TUPLE, iidStack, 1);
    }
    /* if delete, update number of object entries */
    else if (DELETE_EXISTING(newObj, currObj))
    {
       cmsLog_debug("rcl delete MDMOID_DEV2_IEEE1905_AL_NET_TOPOLOGY_IEEE1905_DEVICE_BRIDGING_TUPLE");
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE, MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_BRIDGING_TUPLE, iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceIeee1905NeighborObject( _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceIeee1905NeighborObject *newObj,
                const _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceIeee1905NeighborObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    /* Increase number of objects */
    if (ADD_NEW(newObj, currObj))
    {
       cmsLog_debug("rcl adding new MDMOID_DEV2_IEEE1905_AL_NET_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR");
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE, MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR, iidStack, 1);
    }
    /* if delete, update number of object entries */
    else if (DELETE_EXISTING(newObj, currObj))
    {
       cmsLog_debug("rcl delete MDMOID_DEV2_IEEE1905_AL_NET_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR");
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE, MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR, iidStack, -1);
    }

    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceNonL2NeighborObject( _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceNonL2NeighborObject *newObj,
                const _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceNonL2NeighborObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceIeee1905NeighborMetricObject( _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceIeee1905NeighborMetricObject *newObj,
                const _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceIeee1905NeighborMetricObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    /* Increase number of objects */
    if (ADD_NEW(newObj, currObj))
    {
       cmsLog_debug("rcl adding new MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR_METRIC");
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR, 
                                     MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR_METRIC, 
                                     iidStack, 1);
    }
    /* if delete, update number of object entries */
    else if (DELETE_EXISTING(newObj, currObj))
    {
       cmsLog_debug("rcl delete MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR_METRIC");
       rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR, 
                                     MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_IEEE1905_NEIGHBOR_METRIC, 
                                     iidStack, -1);
    }

    return CMSRET_SUCCESS;}

CmsRet rcl_dev2Ieee1905AlNetworkTopologyIeee1905DeviceNonIeee1905NeighborObject( _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceNonIeee1905NeighborObject *newObj,
                const _Dev2Ieee1905AlNetworkTopologyIeee1905DeviceNonIeee1905NeighborObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   /* Increase number of objects */
   if (ADD_NEW(newObj, currObj))
   {
      cmsLog_debug("rcl adding new MDMOID_DEV2_IEEE1905_AL_NET_TOPOLOGY_IEEE1905_DEVICE_NON_IEEE1905_NEIGHBOR");
      rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE, MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_NON_IEEE1905_NEIGHBOR, iidStack, 1);
   }
   /* if delete, update number of object entries */
   else if (DELETE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("rcl delete MDMOID_DEV2_IEEE1905_AL_NET_TOPOLOGY_IEEE1905_DEVICE_NON_IEEE1905_NEIGHBOR");
      rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE, MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE_NON_IEEE1905_NEIGHBOR, iidStack, -1);
   }

   return CMSRET_SUCCESS;
}

#endif /* DMP_DEVICE2_IEEE1905NETWORKTOPOLOGY_1 */

#endif /* DMP_DEVICE2_IEEE1905BASELINE_1 */
