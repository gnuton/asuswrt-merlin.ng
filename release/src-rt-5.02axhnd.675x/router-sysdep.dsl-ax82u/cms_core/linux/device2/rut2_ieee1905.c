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
#include "rut2_ieee1905.h"
#include "ieee1905_utils.h"

// figure out how to properly coordinate these:
#include "i5api.h"
//#include "i5api.c"

/*
 * Fill in ieee1905 generic message data and send the message to ieee1905d 
 * through CMS messaging.
 * Pre: CmsMsgHeader already contains specific data size and data content.
 */
CmsRet rutIeee1905_sendMsgTo1905(void *data, int dataSize, int msgType)
{
    CmsRet ret = CMSRET_SUCCESS;
    int sd; 
    sd = i5apiOpen();
    if (sd == -1) {
      cmsLog_error("could not open socket to ieee1905");
      return CMSRET_SOCKET_ERROR;
    }
    i5apiSendMessage(sd, msgType, data, dataSize);
    i5apiClose(sd);  
    return ret; 
}

CmsRet rutIeee1905_getDeviceLinkStats(char *deviceId, char *remoteInterfaceId,
          UINT32 *packetErrors, UINT32 *packetErrorsReceived, UINT32 *transmittedPackets, UINT32 *packetsReceived, UINT32 *MACThroughputCapacity,
          UINT32 *linkAvailability, UINT32 *PHYRate, UINT32 *RSSI)
{
   CmsRet                                  ret = CMSRET_SUCCESS;
   t_I5_API_CONFIG_GET_LINK_METRICS        msg;
   t_I5_API_CONFIG_GET_LINK_METRICS_REPLY  repData = {0};
   t_I5_API_CONFIG_GET_LINK_METRICS_REPLY* repDatap = &repData;
   int                                     replySize = 0;

   msg.subcmd = I5_API_CONFIG_GET_LINK_METRICS;
   sscanf(deviceId,          I5_MAC_SCANF, I5_MAC_SCANF_PRM(msg.ieee1905Id));
   sscanf(remoteInterfaceId, I5_MAC_SCANF, I5_MAC_SCANF_PRM(msg.remoteInterfaceId));
    
   replySize = i5apiTransaction(I5_API_CMD_GET_CONFIG, &msg, sizeof(t_I5_API_CONFIG_GET_LINK_METRICS), 
                                (void **)&repDatap, sizeof(t_I5_API_CONFIG_GET_LINK_METRICS_REPLY));

   if (replySize > 0) {
      *packetErrors = repDatap->packetErrors;
      *packetErrorsReceived = repDatap->packetErrorsReceived;
      *transmittedPackets = repDatap->transmittedPackets;
      *packetsReceived = repDatap->packetsReceived;
      *MACThroughputCapacity = repDatap->MacThroughputCapacity;
      *linkAvailability = repDatap->linkAvailability;
      *PHYRate = repDatap->phyRate;
      *RSSI = repDatap->rssi;
   }
   else {
      ret = CMSRET_INTERNAL_ERROR;
   }
   return ret;
}

CmsRet rutIeee1905_getLocalLinkStats(char *remoteInterfaceId,
          UINT32 *packetErrors, UINT32 *packetErrorsReceived, UINT32 *transmittedPackets, UINT32 *packetsReceived, UINT32 *MACThroughputCapacity,
          UINT32 *linkAvailability, UINT32 *PHYRate, UINT32 *RSSI)
{
  char deviceId[I5_MAC_STR_BUF_LEN] = "00:00:00:00:00:00";
  return rutIeee1905_getDeviceLinkStats (deviceId, remoteInterfaceId, packetErrors, packetErrorsReceived, 
                                         transmittedPackets, packetsReceived, MACThroughputCapacity,
                                         linkAvailability, PHYRate, RSSI);
}

CmsRet rutIeee1905_initialize( )
{
    SINT32 pid;

    pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_1905, NULL, 0);
    if (pid == CMS_INVALID_PID) {
       cmsLog_error("failed to start ieee1905");
       return CMSRET_INTERNAL_ERROR;
    }

    return CMSRET_SUCCESS;
}

CmsRet rutIeee1905_terminate(void)
{
    rutIeee1905_sendMsgTo1905(NULL, 0, I5_API_CMD_STOP);
    return CMSRET_SUCCESS;
}

CmsRet rutIeee1905_updateAl(const _Dev2Ieee1905AlObject *obj)
{
   CmsRet ret = CMSRET_SUCCESS;
   t_I5_API_CONFIG_BASE msg;

   msg.subcmd = I5_API_CONFIG_BASE;
   if (obj->deviceFriendlyName)
   {
       snprintf(msg.deviceFriendlyName, I5_DEVICE_FRIENDLY_NAME_LEN, "%s", obj->deviceFriendlyName);
   }
   else
   {
       msg.deviceFriendlyName[0] = '\0';
   }
   msg.isRegistrar = obj->isRegistrar;
   msg.isEnabled = obj->enable;
   msg.apFreqBand24En = obj->APFreqBand24Enable;
   msg.apFreqBand5En = obj->APFreqBand5Enable;

   rutIeee1905_sendMsgTo1905(&msg, sizeof(msg), I5_API_CMD_SET_CONFIG);
   return ret;
}

#if defined(DMP_DEVICE2_IEEE1905NETWORKTOPOLOGY_1)
CmsRet rutIeee1905_updateNtDevice(char *ieee1905Id, char *friendlyName)
{
   CmsRet                                             ret = CMSRET_SUCCESS;
   InstanceIdStack                                    iidStack = EMPTY_INSTANCE_ID_STACK; 
   Dev2Ieee1905AlNetworkTopologyObject               *ieee1905AlNetTopObj = NULL;
   InstanceIdStack                                    iidStackDev = EMPTY_INSTANCE_ID_STACK;
   Dev2Ieee1905AlNetworkTopologyIeee1905DeviceObject *ieee1905DevObj = NULL;

   ret = cmsObj_get(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&ieee1905AlNetTopObj);
   if((ret == CMSRET_SUCCESS) && (ieee1905AlNetTopObj->enable))
   {
      while (cmsObj_getNextFlags(MDMOID_DEV2_IEEE1905_AL_NETWORK_TOPOLOGY_IEEE1905_DEVICE,
                                 &iidStackDev,
                                 OGF_NO_VALUE_UPDATE, (void **)&ieee1905DevObj) == CMSRET_SUCCESS)
      {
         if (strcmp(ieee1905DevObj->IEEE1905Id, ieee1905Id) == 0) {
           CMSMEM_REPLACE_STRING(ieee1905DevObj->friendlyName, friendlyName);
           ret = cmsObj_set(ieee1905DevObj, &iidStackDev);
           cmsObj_free((void **)&ieee1905DevObj);
           break;
         }
         cmsObj_free((void **)&ieee1905DevObj);
      }
   }

   if (ieee1905AlNetTopObj) {
      cmsObj_free((void **)&ieee1905AlNetTopObj);
   }

   return ret;  
}

CmsRet rutIeee1905_updateNetworkTopology(const _Dev2Ieee1905AlNetworkTopologyObject *obj)
{
   CmsRet                               ret = CMSRET_SUCCESS;
   t_I5_API_CONFIG_SET_NETWORK_TOPOLOGY msg;

   msg.subcmd = I5_API_CONFIG_SET_NETWORK_TOPOLOGY;
   msg.isEnabled = obj->enable;

   rutIeee1905_sendMsgTo1905(&msg, sizeof(msg), I5_API_CMD_SET_CONFIG);
   return ret;
}
#endif /* DMP_DEVICE2_IEEE1905NETWORKTOPOLOGY_1 */

#endif    /* DMP_DEVICE2_IEEE1905BASELINE_1 */
