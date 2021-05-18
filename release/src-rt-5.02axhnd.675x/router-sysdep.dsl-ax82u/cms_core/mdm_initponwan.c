/***********************************************************************
 *
 *  Copyright (c) 2006-2010  Broadcom Corporation
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


#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"


/*!\file mdm_initponwan.c
 * \brief This file contains pon (gpon, epon, etc). mdm init related functions, for WAN side.
 *
 */

#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
CmsRet mdm_addDefaultWanGponObjects(void)
{
   SINT32 count;
   UINT32 added = 0;
   char gponIfNmae[CMS_IFNAME_LENGTH];
   WanDevObject *wanDev = NULL;
   WanPonIntfObject *ponObj = NULL;
   WanCommonIntfCfgObject *wanCommonObj = NULL;   
   WanGponLinkCfgObject *gponLinkCfg = NULL;
   MdmPathDescriptor pathDesc;
   InstanceIdStack savedIidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   /*
    * User has selected Gpon as a WAN interface.  See if there is aleady a
    * Gpon WAN device.  If not, create it at the fixed instance number.
    */
   PUSH_INSTANCE_ID(&savedIidStack, CMS_WANDEVICE_GPON);
   ret = mdm_getObject(MDMOID_WAN_DEV, &savedIidStack, (void **) &wanDev);

   if (ret == CMSRET_SUCCESS)
   {
      /* Gpon WANDevice is already present, no action needed */
      mdm_freeObject((void **) &wanDev);
      return ret;
   }

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WAN_DEV;
   PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_GPON);
   cmsLog_notice("adding Gpon WANDevice");

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not add Gpon WANDevice at %s, ret=%d", ret, cmsMdm_dumpIidStack(&pathDesc.iidStack));
      return ret;
   }

   added++;

   /* update CommonIntfConfigObject with the PON WAN WANAccessType */
   if ((ret = mdm_getObject(MDMOID_WAN_COMMON_INTF_CFG, &(pathDesc.iidStack), (void **) &wanCommonObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get WanCommonIntfCfgObject, ret=%d", ret);
      return ret;
   }

   CMSMEM_REPLACE_STRING_FLAGS(wanCommonObj->WANAccessType, MDMVS_X_BROADCOM_COM_PON, mdmLibCtx.allocFlags);

   if ((ret = mdm_setObject((void **) &wanCommonObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set WanCommonIntfCfgObject, ret = %d", ret);
   }
   mdm_freeObject((void **) &wanCommonObj);

   /* update ponInterfaceConfigObject with the GPON as WAN  */
   if ((ret = mdm_getObject(MDMOID_WAN_PON_INTF, &(pathDesc.iidStack), (void **) &ponObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get ponIntfCfgObj, ret=%d", ret);
      return ret;
   }

   ponObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ponObj->ponType, MDMVS_GPON, mdmLibCtx.allocFlags);

   if ((ret = mdm_setObject((void **) &ponObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set PON_INTF_CFG, ret=%d", ret);
   }
   mdm_freeObject((void **) &ponObj);


   /* Need to add CMS_MAX_GPONWAN_INTF gpon WANConnectionDevices since they are not physical
    * existed on the board, but enabled when selected layer on when omcid detects a connection
    */
   pathDesc.oid = MDMOID_WAN_CONN_DEVICE;
   savedIidStack = pathDesc.iidStack;

   for (count = 0; count < CMS_MAX_GPONWAN_INTF; count++)
   {
      cmsLog_debug("Create WANConnDevice Object for gpon%d", count);

      pathDesc.oid = MDMOID_WAN_CONN_DEVICE;
      pathDesc.iidStack = savedIidStack;

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not add WANConnectionDevice, ret=%d", ret);
         break;
      }

      /* Get WANGponLinkConfig object and fill the fixed layer 2 interface name (gpon0, gpon1, etc.) */
      cmsLog_notice("Get GponLinkcfg");
      if ((ret = mdm_getObject(MDMOID_WAN_GPON_LINK_CFG, &(pathDesc.iidStack), (void **) &gponLinkCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get MDMOID_WAN_GPON_LINK_CFG, ret=%d", ret);
         break;
      }

      sprintf(gponIfNmae, "%s%d", GPON_IFC_STR,count);

      CMSMEM_REPLACE_STRING_FLAGS(gponLinkCfg->ifName, gponIfNmae, mdmLibCtx.allocFlags);

      if ((ret = mdm_setObject((void **) &gponLinkCfg, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set gponLinkCfg, ret=%d", ret);
      }
      else
      {
         cmsLog_debug("Done adding wanConnectionDevice.");
      }
      mdm_freeObject((void **) &gponLinkCfg);
   }

   if (count != CMS_MAX_GPONWAN_INTF)
   {
      cmsLog_error("Failed to add gpon wanConnDevice.");
      return ret;
   }         

   /* All WanConnectionDevices are added sucessfully.
    * Need to update the device count
    */
   cmsLog_debug("iidStack=%s", cmsMdm_dumpIidStack(&(pathDesc.iidStack)));
   if ((ret = mdm_getAncestorObject(MDMOID_WAN_DEV,
                                    MDMOID_WAN_CONN_DEVICE,
                                    &(pathDesc.iidStack),
                                    (void **) &wanDev)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Fail to get WanDev object, ret=%d", ret);
      return ret;
   }

   /* update wanConnectionDevice counter */
   wanDev->WANConnectionNumberOfEntries = count;

   if ((ret = mdm_setObject((void **) &wanDev, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set wanDev, ret=%d", ret);
   }
   mdm_freeObject((void **) &wanDev);

   cmsLog_debug("Done setting WanConnDevice Count.");

   if (added > 0)
   {
      mdm_increaseWanDeviceCount(added);
   }

   return ret;
}

#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 */


#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
#ifdef EPON_SFU
extern CmsRet addDefaultWanIpConnection();
#ifdef BRCM_PKTCBL_SUPPORT
extern CmsRet addDefaultWanIpConnectionForVoice();
#endif
#endif

CmsRet mdm_addDefaultWanEponObject(void)
{
   UINT32 added = 0;
   WanDevObject *wanDevObj = NULL;
   WanPonIntfObject *ponObj = NULL;
   WanCommonIntfCfgObject *wanCommonObj = NULL;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;


   /*
    * User has selected Epon as a WAN interface.  See if there is aleady 
    * a Epon WAN device.  If not, create it at the fixed instance number.
    */

   PUSH_INSTANCE_ID(&iidStack, CMS_WANDEVICE_EPON);
   ret = mdm_getObject(MDMOID_WAN_DEV, &iidStack, (void **)&wanDevObj);

   if (ret == CMSRET_SUCCESS)
   {
      /* Epon WANDevice is already present, no action needed */
      mdm_freeObject((void **) &wanDevObj);
      cmsLog_debug("epon wan exist");
      return ret;
   }

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_WAN_DEV;
   PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_EPON);
   cmsLog_notice("adding Epon WANDevice");

   /* Create WANDevice.{i}. where i = CMS_WANDEVICE_EPON = 6 */
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not add WANDevice at %s, ret=%d", ret, cmsMdm_dumpIidStack(&pathDesc.iidStack));
      return ret;
   }

   added++;

   /* Update WANDevice.{i}.WANCommonInterfaceConfig. with WanAccessType as PON */
   if ((ret = mdm_getObject(MDMOID_WAN_COMMON_INTF_CFG, &iidStack, (void **) &wanCommonObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get WanCommonIntfCfgObject, ret=%d", ret);
      return ret;
   }

   CMSMEM_REPLACE_STRING_FLAGS(wanCommonObj->WANAccessType, MDMVS_X_BROADCOM_COM_PON, mdmLibCtx.allocFlags);

   if ((ret = mdm_setObject((void **) &wanCommonObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set WanCommonIntfCfgObject, ret = %d", ret);
   }
   mdm_freeObject((void **) &wanCommonObj);

   /* Update WANDevice.{i}.X_BROADCOM_COM_WANPonInterfaceConfig. with the EPON as WAN  */
   if ((ret = mdm_getObject(MDMOID_WAN_PON_INTF, &(pathDesc.iidStack), (void **) &ponObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get PonInterfaceConfig, ret=%d", ret);
      return ret;
   }

   ponObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(ponObj->ponType, MDMVS_EPON, mdmLibCtx.allocFlags);

   if ((ret = mdm_setObject((void **) &ponObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not set PonInterfaceConfig, ret=%d", ret);
   }
   mdm_freeObject((void **) &ponObj);

#ifdef EPON_SFU
   WanEponIntfObject *eponIntfObj = NULL;

   /* Update WANDevice.{i}.X_BROADCOM_COM_EponInterfaceConfig. */
   if ((ret = mdm_getObject(MDMOID_WAN_EPON_INTF, &(pathDesc.iidStack), (void **) &eponIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get EponInterfaceConfig, ret=%d", ret);
      return ret;
   }

   eponIntfObj->enable = TRUE;
   eponIntfObj->persistentDevice = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(eponIntfObj->ifName, EPON_WAN_IF_NAME, mdmLibCtx.allocFlags);

   if ((ret = mdm_setObject((void **) &eponIntfObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not set EponInterfaceConfig, ret=%d", ret);
   }
   mdm_freeObject((void **) &eponIntfObj);
   
   /*
    * Also create a single WANConnectionDevice in this WANDevice.
    */
   pathDesc.oid = MDMOID_WAN_CONN_DEVICE;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create new WanConnectionDevice, ret=%d", ret);
   }

#ifdef BRCM_PKTCBL_SUPPORT
   WanEponLinkCfgObject *eponLinkCfg = NULL;

   /* Get WANEponLinkConfig object and fill the fixed layer 2 interface name */
   cmsLog_notice("Get EponLinkcfg");
   if ((ret = mdm_getObject(MDMOID_WAN_EPON_LINK_CFG, &(pathDesc.iidStack), (void **) &eponLinkCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MDMOID_WAN_EPON_LINK_CFG, ret=%d", ret);
      return ret;
   }

   CMSMEM_REPLACE_STRING_FLAGS(eponLinkCfg->ifName, EPON_VOICE_WAN_IF_NAME, mdmLibCtx.allocFlags); 
      
   if ((ret = mdm_setObject((void **) &eponLinkCfg, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set eponLinkCfg, ret=%d", ret);
   }
   else
   {
      cmsLog_debug("Done adding wanConnectionDevice.");
   }
   mdm_freeObject((void **) &eponLinkCfg);
#endif // BRCM_PKTCBL_SUPPORT

   /* Need to update the WanConnDevice count */
   if ((ret = mdm_getAncestorObject(MDMOID_WAN_DEV, 
                                    MDMOID_WAN_CONN_DEVICE,
                                    &(pathDesc.iidStack),
                                    (void **) &wanDevObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Fail to get wanDevObj, ret=%d", ret);
      return ret;
   }

   /* update wanConnectionDevice counter */
   wanDevObj->WANConnectionNumberOfEntries = 1;

   if ((ret = mdm_setObject((void **) &wanDevObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set wanDevObj, ret=%d", ret);
   }
   mdm_freeObject((void **) &wanDevObj);

   addDefaultWanIpConnection();
#ifdef BRCM_PKTCBL_SUPPORT
   addDefaultWanIpConnectionForVoice();
#endif // BRCM_PKTCBL_SUPPORT
#else
   WanEponLinkCfgObject *eponLinkCfg = NULL;

   pathDesc.oid = MDMOID_WAN_CONN_DEVICE;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not add WANConnectionDevice, ret=%d", ret);
      return ret;
   }

   /* Get WANEponLinkConfig object and fill the fixed layer 2 interface name */
   cmsLog_notice("Get EponLinkcfg");
   if ((ret = mdm_getObject(MDMOID_WAN_EPON_LINK_CFG, &(pathDesc.iidStack), (void **) &eponLinkCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MDMOID_WAN_EPON_LINK_CFG, ret=%d", ret);
      return ret;
   }

   CMSMEM_REPLACE_STRING_FLAGS(eponLinkCfg->ifName, EPON_WAN_IF_NAME, mdmLibCtx.allocFlags); 
      
   if ((ret = mdm_setObject((void **) &eponLinkCfg, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set eponLinkCfg, ret=%d", ret);
   }
   else
   {
      cmsLog_debug("Done adding wanConnectionDevice.");
   }
   mdm_freeObject((void **) &eponLinkCfg);

   /* All WanConnectionDevices are added sucessfully.
    * Need to update the device count
    */
   cmsLog_debug("iidStack=%s", cmsMdm_dumpIidStack(&(pathDesc.iidStack)));
   if ((ret = mdm_getAncestorObject(MDMOID_WAN_DEV,
                                    MDMOID_WAN_CONN_DEVICE,
                                    &(pathDesc.iidStack),
                                    (void **) &wanDevObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Fail to get WanDev object, ret=%d", ret);
      return ret;
   }

   /* update wanConnectionDevice counter */
   wanDevObj->WANConnectionNumberOfEntries = 1;
      
   if ((ret = mdm_setObject((void **) &wanDevObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set wanDev, ret=%d", ret);
   }
   mdm_freeObject((void **) &wanDevObj);

   cmsLog_debug("Done setting WanConnDevice Count.");
#endif

   /* Increase WANDeviceNumberOfEntries  */
   if (added > 0)
   {
      mdm_increaseWanDeviceCount(added);
   }

   return ret;
}

#endif /* DMP_X_BROADCOM_COM_EPONWAN_1 */

