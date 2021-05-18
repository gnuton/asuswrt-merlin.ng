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


#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"


/*!\file mdm_initmoca.c
 * \brief This file contains Moca mdm init related functions, for both
 *        WAN and LAN side.
 *
 */




#ifdef DMP_BRIDGING_1
CmsRet addDefaultL2BridgingAvailableInterfaceObject(const char *interfaceReference, SINT32 bridgeRef);
#endif

#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1

CmsRet mdm_addDefaultWanMocaObject()
{
   void *mdmObj = NULL;
   _WanCommonIntfCfgObject *wanCommonObj = NULL;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 added = 0;
   CmsRet ret;

   /*
    * User has selected Moca as a WAN interface.  See if there is aleady a Moca
    * WAN device.  If not, create it at the fixed instance number.
    */
   PUSH_INSTANCE_ID(&iidStack, CMS_WANDEVICE_MOCA);
   ret = mdm_getObject(MDMOID_WAN_DEV, &iidStack, &mdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("adding Moca WANDevice");
      added++;

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_WAN_DEV;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_MOCA);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not add WANDevice at %s, ret=%d", ret, cmsMdm_dumpIidStack(&pathDesc.iidStack));
         return ret;
      }

      if ((ret = mdm_getObject(MDMOID_WAN_COMMON_INTF_CFG, &iidStack, (void **) &wanCommonObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get WanCommonIntfCfgObject, ret=%d", ret);
         return ret;
      }

      CMSMEM_REPLACE_STRING_FLAGS(wanCommonObj->WANAccessType, MDMVS_X_BROADCOM_COM_MOCA, mdmLibCtx.allocFlags); 

      if ((ret = mdm_setObject((void **) &wanCommonObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set WanCommonIntfCfgObject, ret = %d", ret);
         /* mdm_setObject does not steal obj on error, so we must free it */
         mdm_freeObject((void **) &wanCommonObj);
      }
   }
   else
   {
      /* Moca WANDevice is already present, no action needed */
      mdm_freeObject(&mdmObj);
   }


   if (added > 0)
   {
      mdm_increaseWanDeviceCount(added);
   }

   return ret;
}


#endif /* DMP_X_BROADCOM_COM_MOCAWAN_1 */


#ifdef DMP_X_BROADCOM_COM_MOCALAN_1

CmsRet addLanMocaInterfaceObject(const char *ifName)
{
   MdmPathDescriptor pathDesc;
   _LanMocaIntfObject *mocaObj=NULL;
   CmsRet ret;

   cmsLog_notice("adding LAN %s intf object", ifName);
      
   INIT_PATH_DESCRIPTOR(&pathDesc);
   
   pathDesc.oid = MDMOID_LAN_MOCA_INTF;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); /* default moca interface always under LANDevice.1 */

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to create default %s interface, ret=%d", ifName, ret);
      return ret;
   }
   
   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &mocaObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get moca intf object, ret=%d", ret);
      return ret;
   }

   /* set the intf name and enable */   
   mocaObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(mocaObj->ifName, ifName, mdmLibCtx.allocFlags);

   if ((ret = mdm_setObject((void **) &mocaObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not set moca intf object, ret=%d", ret);
      /* mdm_setObject does not steal obj on error, so we must free it */
      mdm_freeObject((void **) &mocaObj);
   }

#ifdef DMP_BRIDGING_1
   {
      char *fullPath=NULL;

      cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath);
      /* Annoying TR-98 format: remove the last . */
      fullPath[strlen(fullPath)-1] = '\0';
      cmsLog_debug("fullpathname is %s", fullPath);

      ret = addDefaultL2BridgingAvailableInterfaceObject(fullPath, 0);
      cmsMem_free(fullPath);
   }
#endif

   return ret;
}

#endif /* DMP_X_BROADCOM_COM_MOCALAN_1 */
