/***********************************************************************
 *
 *  Copyright (c) 2009-2016  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2016:proprietary:standard

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
#include "stl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_system.h"
#include "rut_qos.h"
#include "ethswctl_api.h"
#include "cms_qdm.h"

/*!\file stl2_lag.c
 * \brief This file contains LAG device objects related functions.
 *
 */
#ifdef DMP_DEVICE2_ETHLAG_1
CmsRet stl_dev2EthLAGObject(_Dev2EthLAGObject *obj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
       
   char currentStatus[BUFLEN_16]={0};
   char macAddrStr[MAC_STR_LEN+1]={0};
   
   cmsLog_debug("Entered: name=%s", obj->name);
   
   if (cmsUtl_strlen(obj->name) == 0)
   {
      /* NULL or empty IntfName, can't do anything yet. */
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   
   /*
    * rutLan_getIntfStatus uses SIOCGLINKSTATE to get link state, which
    * seems to be slow (delay of about 1 second) to show real state.
    * Use the newer function to get link state, but still use
    * rutLan_getIntfStatus to get macAddr.
    */
   rutLan_getIntfStatus(obj->name, currentStatus, macAddrStr);
   if (cmsNet_isInterfaceLinkUp(obj->name))
   {
      sprintf(currentStatus, MDMVS_UP);
   }
   else
   {
      sprintf(currentStatus, MDMVS_DORMANT);
   }
   
   cmsLog_debug("%s (upstream=%d): oldStatus %s, currentStatus %s, macAddr %s",
                obj->name, obj->X_BROADCOM_COM_Upstream,  obj->status, currentStatus,
                macAddrStr);
   
   /* always update the mac addr if it has changed */
   if (cmsUtl_strcmp(obj->MACAddress, macAddrStr))
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->MACAddress, macAddrStr, mdmLibCtx.allocFlags);
      obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
   }
   
   if (cmsUtl_strcmp(obj->status, currentStatus))
   {
      UBOOL8 prevStatusWasUp = (cmsUtl_strcmp(obj->status, MDMVS_UP) == 0);

      /* link status has changed */
      obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();

      CMSMEM_REPLACE_STRING_FLAGS(obj->status, currentStatus, mdmLibCtx.allocFlags);

      /* if we went from link down to up, update speed and duplex info */
      if (!cmsUtl_strcmp(currentStatus, MDMVS_UP))
      {
#ifdef DMP_DEVICE2_QOS_1_TODO
               /* Todo: Need qos on ethLag ?
                * Due to complications with DMP_DEVICE2_ETHERNET_INTERFACE
                * and DMP_DEVICE2_ETHERNET_LINK in Hybrid mode, we need to put
                * #ifdef DMP_DEVICE2_QOS_1 around just this particular call to
                * rutQos_reconfigAllQueuesOnLayer2Intf_dev2() because it is
                * not present in a Hybrid build.
                */
               rutQos_reconfigAllQueuesOnLayer2Intf_dev2(obj->name);

               /* some classifications may reference this intf on ingress side,
                * so check if they need to be reconfigured.
                */
               rutQos_reconfigAllClassifications_dev2(obj->name);
#endif  /* DMP_DEVICE2_QOS_1 */
      }
      else if (prevStatusWasUp && cmsUtl_strcmp(currentStatus, MDMVS_UP))
      {
      
         if (obj->X_BROADCOM_COM_Upstream)
         {
            /*  Todo: Need qos on ethLag ?  EthLag WAN Down (specifically: from UP to some non-UP status )*/
            
            /*
            CmsRet r2;
            if ((r2 = rutQos_tmPortUninit(obj->name, TRUE)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_tmPortUninit() returns error. ret=%d", r2);
            }
            */
         }
         else
         {
            /* EthLag LAN Down (specifically: from UP to some non-UP status )*/
            /* We don't deconfig FAP TM when link goes down? */
         }
      }
   }

   /* Always update TR181 time since LastChange before return */
   IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);


   /* this function always returns SUCCESS (not SUCCESS_OBJECT_UNCHANGED)
    * because it needs to report updated LastChange value.
    */

   return CMSRET_SUCCESS;

}   
CmsRet stl_dev2EthLAGStatsObject(_Dev2EthLAGStatsObject *obj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)))
{
   Dev2EthLAGObject *ethLagIntf = NULL;
   InstanceIdStack parentIidStack = *iidStack;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Enter");               
   
   if ((ret = cmsObj_getAncestor(MDMOID_LA_G,
                                 MDMOID_LAG_STATS,
                                 &parentIidStack,
                                 (void **) &ethLagIntf)) == CMSRET_SUCCESS)
   {
      if (obj == NULL)
      {
         rut_clearIntfStats(ethLagIntf->name);
      }
      else
      {
         UINT64 bytesReceived = 0, packetsReceived = 0, multicastBytesReceived = 0, multicastPacketsReceived = 0;
         UINT64 unicastPacketsReceived = 0, broadcastPacketsReceived = 0, errorsReceived = 0, discardPacketsReceived = 0;
         UINT64 bytesSent = 0, packetsSent = 0, multicastBytesSent = 0, multicastPacketsSent = 0;
         UINT64 unicastPacketsSent = 0, broadcastPacketsSent = 0, errorsSent = 0, discardPacketsSent = 0;

         rut_getIntfStats_uint64(ethLagIntf->name, &bytesReceived, &packetsReceived,
                                 &multicastBytesReceived, &multicastPacketsReceived, &unicastPacketsReceived, &broadcastPacketsReceived, 
                                 &errorsReceived, &discardPacketsReceived,
                                 &bytesSent, &packetsSent,
                                 &multicastBytesSent, &multicastPacketsSent, &unicastPacketsSent, &broadcastPacketsSent,
                                 &errorsSent, &discardPacketsSent);

         obj->bytesSent = bytesSent;
         obj->bytesReceived = bytesReceived;
         obj->packetsSent = packetsSent;         
         obj->packetsReceived = packetsReceived;
         obj->errorsSent = (UINT32)errorsSent;
         obj->errorsReceived = (UINT32)errorsReceived;         
         obj->unicastPacketsSent = unicastPacketsSent;
         obj->unicastPacketsReceived = unicastPacketsReceived;   
         obj->discardPacketsSent = (UINT32)discardPacketsSent;        
         obj->discardPacketsReceived = (UINT32)discardPacketsReceived;
         obj->multicastPacketsSent = multicastBytesReceived;
         obj->multicastPacketsReceived = multicastPacketsReceived;
         obj->broadcastPacketsSent = broadcastPacketsSent;
         obj->broadcastPacketsReceived = broadcastPacketsReceived;
      }

      cmsObj_free((void **) &ethLagIntf);
  
   }

   cmsLog_debug("Exit. ret %d", ret);

   return ret;
}


#endif /* DMP_DEVICE2_ETHLAG_1 */
#endif /* DMP_DEVICE2_BASELINE_1 */
