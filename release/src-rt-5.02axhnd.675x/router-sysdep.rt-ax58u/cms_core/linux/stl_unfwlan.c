/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
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

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <assert.h>

#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut_system.h"
#include "rut_wifiwan.h"
#include "rut_unfwlan.h"
#include "rut_qos.h"

#include "stl.h"


#ifdef DMP_WIFILAN_1


CmsRet stl_lanWlanObject(_LanWlanObject *obj, const InstanceIdStack *iidStack)
{
   UBOOL8 isLinkUp;
   int radioIndex;
   char buf[BUFLEN_1024] = {0};
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   if (obj->X_BROADCOM_COM_IfName != NULL)
   {
       isLinkUp = cmsNet_isInterfaceLinkUp(obj->X_BROADCOM_COM_IfName);
       if (isLinkUp && cmsUtl_strcmp(obj->status, MDMVS_UP))
       {
           CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
       }
       else if (!isLinkUp && cmsUtl_strcmp(obj->status, MDMVS_DISABLED))
       {
           CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
       }

       // Get statistics and put the data into the MDM object
       rut_getIntfStats(obj->X_BROADCOM_COM_IfName,
                        &(obj->totalBytesReceived),
                        &(obj->totalPacketsReceived),
                        &(obj->X_BROADCOM_COM_MulticastBytesReceived),
                        &(obj->X_BROADCOM_COM_MulticastPacketsReceived),
                        &(obj->X_BROADCOM_COM_UnicastPacketsReceived),
                        &(obj->X_BROADCOM_COM_BroadcastPacketsReceived),
                        &(obj->X_BROADCOM_COM_RxErrors),
                        &(obj->X_BROADCOM_COM_RxDrops),
                        &(obj->totalBytesSent),
                        &(obj->totalPacketsSent),
                        &(obj->X_BROADCOM_COM_MulticastBytesSent),
                        &(obj->X_BROADCOM_COM_MulticastPacketsSent),
                        &(obj->X_BROADCOM_COM_UnicastPacketsSent),
                        &(obj->X_BROADCOM_COM_BroadcastPacketsSent),
                        &(obj->X_BROADCOM_COM_TxErrors),
                        &(obj->X_BROADCOM_COM_TxDrops));

       ret = CMSRET_SUCCESS;
   }

   radioIndex = PEEK_INSTANCE_ID(iidStack);
   assert(radioIndex > 0);
   radioIndex -= 1;
   /*comment it since radioEnabled need to use in cdrouter test*/
#if 0
   radioEnabled = rutWlan_getRadioEnabled(radioIndex);
   if (obj->radioEnabled != radioEnabled)
   {
       obj->radioEnabled = radioEnabled;
       ret = CMSRET_SUCCESS;
   }
#endif

   rutWlan_getPossibleChannels(buf, sizeof(buf), radioIndex);
   if ((obj->possibleChannels == NULL) || (0 != cmsUtl_strcmp(obj->possibleChannels, buf)))
   {
       CMSMEM_REPLACE_STRING_FLAGS(obj->possibleChannels, buf, mdmLibCtx.allocFlags);
       ret = CMSRET_SUCCESS;
   }
   return ret;
}

CmsRet stl_lanWlanClearStats(_LanWlanObject *obj)
{
   //const InstanceIdStack *iidStack;
   rut_clearIntfStats(obj->X_BROADCOM_COM_IfName);
   stl_lanWlanObject(obj, NULL);
   return CMSRET_SUCCESS;
}

CmsRet stl_lanWlanAssociatedDeviceEntryObject(_LanWlanAssociatedDeviceEntryObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_lanWlanWepKeyObject(_LanWlanWepKeyObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_lanWlanPreSharedKeyObject(_LanWlanPreSharedKeyObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#if not_supported
CmsRet stl_lanWlanStatsObject(_LanWlanStatsObject *obj, const InstanceIdStack *iidStack)
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif	

#endif /* DMP_WIFILAN_1 */


#ifdef DMP_X_BROADCOM_COM_WIFILAN_1
CmsRet stl_wlanAdapterObject(_WlanAdapterObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wlBaseCfgObject(_WlBaseCfgObject *obj, const InstanceIdStack *iidStack)
{
   int radioIndex;
   char *phyType = NULL;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   radioIndex = PEEK_INSTANCE_ID(iidStack);
   if (radioIndex <= 0)
   {
       return CMSRET_INTERNAL_ERROR;
   }
   radioIndex -= 1;
   phyType = rutWlan_getPhyType(radioIndex);
   if (phyType != NULL)
   {
      if (0 != cmsUtl_strcmp(obj->wlPhyType, phyType))
      {
          CMSMEM_REPLACE_STRING_FLAGS(obj->wlPhyType, phyType, mdmLibCtx.allocFlags);
          ret = CMSRET_SUCCESS;
      }
   }
   return ret;
}

CmsRet stl_wlStaticWdsCfgObject(_WlStaticWdsCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
CmsRet stl_wlWdsCfgObject(_WlWdsCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wlScanWdsCfgObject(_WlScanWdsCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wlMimoCfgObject(_WlMimoCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wlSesCfgObject(_WlSesCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wlVirtIntfCfgObject(_WlVirtIntfCfgObject *obj, const InstanceIdStack *iidStack)
{
   UBOOL8 isLinkUp;
   UBOOL8 setLinkDown = FALSE;
   InstanceIdStack iidStackLocal;
   _WlMimoCfgObject *mimoCfgObj = NULL;
   _WlBaseCfgObject *baseCfgObj = NULL;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   iidStackLocal = *iidStack;
   POP_INSTANCE_ID(&iidStackLocal);
   ret = cmsObj_get(MDMOID_WL_MIMO_CFG, &iidStackLocal, 0, (void **)&mimoCfgObj);
   if (ret != CMSRET_SUCCESS)
   {
       return CMSRET_INTERNAL_ERROR;
   }
   ret = cmsObj_get(MDMOID_WL_BASE_CFG, &iidStackLocal, 0, (void **)&baseCfgObj);
   if (ret != CMSRET_SUCCESS)
   {
       return CMSRET_INTERNAL_ERROR;
   }

   /*
    * If WiFi nmode is "on" or "auto", and the detected WiFi Phy mode is either "n" or "v",
    * we support crypto method "tkip+aes". Otherwise, we support "tkip" only.
    */
   if ((0 != cmsUtl_strcmp(mimoCfgObj->wlNmode, "off")) &&
       ((0 == cmsUtl_strcmp(baseCfgObj->wlPhyType, "n")) ||
        (0 == cmsUtl_strcmp(baseCfgObj->wlPhyType, "v"))))
   {
       if (0 == cmsUtl_strcmp(obj->wlWpa, "tkip"))
       {
          CMSMEM_REPLACE_STRING_FLAGS(obj->wlWpa, "tkip+aes", mdmLibCtx.allocFlags);
          ret = CMSRET_SUCCESS;
       }
   }
   cmsObj_free((void **)&mimoCfgObj);
   cmsObj_free((void **)&baseCfgObj);

   if (obj->wlIfcname == NULL)
   {
       return ret;
   }
   /*
    * If this SSID is not enabled or a Wifi WAN interface, the status
    * should always be in the DISABLED (meaning link down) state.
    */
   if (obj->wlEnblSsid == 0)
   {
      setLinkDown = TRUE;
   }

#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
   {
      /* check if this intf on the WAN side */
      InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
      if (rutWanWifi_getWlIntfByIfName(obj->wlIfcname, &iidStack2, NULL))
      {
         setLinkDown = TRUE;
      }
   }
#endif

    if (setLinkDown)
    {
       if (cmsUtl_strcmp(obj->status, MDMVS_DISABLED))
       {
          CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
          rutQos_setDefaultWlQueues(obj->wlIfcname, FALSE);
          ret = CMSRET_SUCCESS;
       }
       return ret;
    }

   /*
    * If we get here, this intf is enabled and on LAN side, need to
    * determine link status.
    */
   isLinkUp = cmsNet_isInterfaceLinkUp(obj->wlIfcname);
   if (isLinkUp && cmsUtl_strcmp(obj->status, MDMVS_UP))
   {
      cmsLog_debug("SSID %s link state changed to UP", obj->wlIfcname);
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
      rutQos_setDefaultWlQueues(obj->wlIfcname, TRUE);
      ret = CMSRET_SUCCESS;
   }
   else if (!isLinkUp && cmsUtl_strcmp(obj->status, MDMVS_DISABLED))
   {
      cmsLog_debug("SSID %s link state changed to DOWN", obj->wlIfcname);
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
      rutQos_setDefaultWlQueues(obj->wlIfcname, FALSE);
      ret = CMSRET_SUCCESS;
   }

   return ret;
}

CmsRet stl_wlMacFltObject(_WlMacFltObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wlKey64CfgObject(_WlKey64CfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wlKey128CfgObject(_WlKey128CfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wlWpsCfgObject(_WlWpsCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_lanWlanVirtMbssObject(_LanWlanVirtMbssObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif  /* DMP_X_BROADCOM_COM_WIFILAN_1 */


#ifdef DMP_X_BROADCOM_COM_WIFILAN_1
/*
 * these next 3 certificate related functions could be used for general
 * certificate purposes on the system, but for now, only used by WLAN code.
 */
CmsRet stl_wapiCertificateObject(_WapiCertificateObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wapiAsCertificateObject(_WapiAsCertificateObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wapiIssuedCertificateObject(_WapiIssuedCertificateObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif  /* DMP_X_BROADCOM_COM_WIFILAN_1 */


#ifdef DMP_X_BROADCOM_COM_WIFILAN_1
CmsRet stl_wlanNvramObject(_WlanNvramObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif  /* DMP_X_BROADCOM_COM_WIFILAN_1 */
