/***********************************************************************
 *
 *  Copyright (c) 2009-2013  Broadcom Corporation
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


#ifdef DMP_DEVICE2_ETHERNETINTERFACE_1

#include "cms.h"
#include "cms_mdm.h"
#include "cms_core.h"
#include "cms_util.h"
#include "stl.h"
#include "rut_util.h"
#include "rut_qos.h"
#include "rut2_bridging.h"
#include "cms_qdm.h"

/*!\file rut2_ethernet.c
 * \brief This file contains ethernet related util functions.
 *
 */


static CmsRet getEthObjByIntfNameAndUpstream_dev2(const char *intfName,
                                    UBOOL8 isUpstream,
                                    InstanceIdStack *iidStack,
                                    Dev2EthernetInterfaceObject **ethIntfObj)
{
   CmsRet ret;

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_ETHERNET_INTERFACE,
                              iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) ethIntfObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp((*ethIntfObj)->name, intfName))
      {
         if ((*ethIntfObj)->upstream == isUpstream)
         {
            /* caller is responsible for freeing ethIntfObj */
            return CMSRET_SUCCESS;
         }
         else
         {
            cmsLog_error("Found %s but not matching upstream param, got %d wanted %d",
                  (*ethIntfObj)->name, (*ethIntfObj)->upstream, isUpstream);
            cmsObj_free((void **) ethIntfObj);
            return CMSRET_INVALID_ARGUMENTS;
         }
      }

      cmsObj_free((void **) ethIntfObj);
   }

   cmsLog_error("Could not find Eth Intf Obj for %s", intfName);
   return CMSRET_OBJECT_NOT_FOUND;
}


void rutEth_moveEthLanToWan_dev2(const char *ifName)
{
   Dev2EthernetInterfaceObject *ethIntfObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("Entered: ifName=%s", ifName);


   ret = getEthObjByIntfNameAndUpstream_dev2(ifName, FALSE,
                                             &iidStack, &ethIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      return;
   }

   /*
    * Caller must disable intf first (and release lock so ssk can process
    * link status changed messages).
    */
   if (ethIntfObj->enable)
   {
      cmsLog_error("Cannot move %s while it is enabled", ifName);
      cmsObj_free((void **) &ethIntfObj);
      return;
   }

#ifdef DMP_DEVICE2_QOS_1
   /*
    * Because QoS queues for Eth LAN port is different than QoS queues
    * for Eth WAN port, delete all QoS queues on this intf.  This will also
    * delete all classification which EGRESS on this intf.  (XXX TODO: but
    * it does not delete classifications which INGRESS on this intf)
    */
   rutQos_deleteQueues_dev2(ifName);
#endif

#ifdef DMP_DEVICE2_BRIDGE_1
   {
      const char *lanIfName;

#ifdef SUPPORT_LANVLAN
      char lanIfNameBuf[CMS_IFNAME_LENGTH]={0};

      if (strstr(ifName, ETH_IFC_STR) && (strchr(ifName, '.') == NULL))
      {
         snprintf(lanIfNameBuf, sizeof(lanIfNameBuf), "%s.0", ifName);
         lanIfName = lanIfNameBuf;
      }
      else
#endif
         lanIfName = ifName;

      /* Now remove intf from LAN side bridge. */
      rutBridge_deleteIntfNameFromBridge_dev2(lanIfName);
   }
#endif

   /* Finally set upstream to TRUE (WAN) */
   ethIntfObj->upstream = TRUE;
   ret = cmsObj_set(ethIntfObj, &iidStack);
   cmsObj_free((void **) &ethIntfObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Set of ethIntfObj %s to WAN side failed, ret=%d", ifName, ret);
   }

   /*
    * Once Eth intf is on the WAN side, the QoS and TM stuff is handled when
    * the interface is enabled and link up is detected.
    */
   return;
}


void rutEth_moveEthWanToLan_dev2(const char *ifName)
{
   Dev2EthernetInterfaceObject *ethIntfObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("Entered: ifName=%s", ifName);

   ret = getEthObjByIntfNameAndUpstream_dev2(ifName, TRUE,
                                             &iidStack, &ethIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      return;
   }

   /*
    * Caller must disable intf first (and release lock so ssk can process
    * link status changed messages).
    */
   if (ethIntfObj->enable)
   {
      cmsLog_error("Cannot move %s while it is enabled", ifName);
      cmsObj_free((void **) &ethIntfObj);
      return;
   }

#ifdef DMP_DEVICE2_QOS_1
   /*
    * Because QoS queues for Eth WAN port is different than QoS queues
    * for Eth LAN port, delete all QoS queues on this intf.  This will also
    * delete all classification which EGRESS on this intf.  (XXX TODO: but
    * it does not delete classifications which INGRESS on this intf)
    */
   rutQos_deleteQueues_dev2(ifName);
#endif


   /* Set upstream to FALSE (LAN) */
   ethIntfObj->upstream = FALSE;
   ret = cmsObj_set(ethIntfObj, &iidStack);
   cmsObj_free((void **) &ethIntfObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Set of ethIntfObj %s to LAN side failed, ret=%d", ifName, ret);
   }


#ifdef DMP_DEVICE2_BRIDGE_1
   {
      const char *lanIfName;

#ifdef SUPPORT_LANVLAN
      char lanIfNameBuf[CMS_IFNAME_LENGTH]={0};

      if (strstr(ifName, ETH_IFC_STR) && (strchr(ifName, '.') == NULL))
      {
         snprintf(lanIfNameBuf, sizeof(lanIfNameBuf), "%s.0", ifName);
         lanIfName = lanIfNameBuf;
      }
      else
#endif
         lanIfName = ifName;

      /* Now add intf to default LAN side bridge br0. */
      rutBridge_addLanIntfToBridge_dev2(lanIfName, "br0");
   }
#endif


   /*
    * Once Eth intf is on the LAN side, the QoS and TM stuff is handled when
    * the interface is enabled and link up is detected.
    */
   return;
}


#endif /* DMP_DEVICE2_ETHERNETINTERFACE_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */

