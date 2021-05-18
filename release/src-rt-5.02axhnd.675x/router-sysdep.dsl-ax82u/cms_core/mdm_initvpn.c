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


/*!\file mdm_initvpn.c
 * \brief This file contains VPN/L2TP mdm init related functions.
 *
 */


#ifdef DMP_X_BROADCOM_COM_L2TPAC_1


CmsRet mdm_addDefaultL2tpVpnObjects()
{
   void *mdmObj = NULL;
   _WanCommonIntfCfgObject *wanCommonObj = NULL;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 added = 0;
   UINT32 i;
   CmsRet ret;

   /*
    * Add all necessary VPN (only L2TP for now) WANDevice objects.
    */
   for (i=0; i < CMS_WANDEVICE_L2TPAC_COUNT; i++)
   {
      PUSH_INSTANCE_ID(&iidStack, CMS_WANDEVICE_L2TPAC+i);
      ret = mdm_getObject(MDMOID_WAN_DEV, &iidStack, &mdmObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_notice("adding L2TP AC WANDevice at %d", CMS_WANDEVICE_L2TPAC+i);
         added++;

         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_WAN_DEV;
         PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_L2TPAC+i);

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

         CMSMEM_REPLACE_STRING_FLAGS(wanCommonObj->WANAccessType, MDMVS_X_BROADCOM_COM_L2TPAC, mdmLibCtx.allocFlags); 

         if ((ret = mdm_setObject((void **) &wanCommonObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
         {
            mdm_freeObject((void **) &wanCommonObj);
            cmsLog_error("Failed to set WanCommonIntfCfgObject, ret = %d", ret);
         }
   
      }
      else
      {
         /* L2TPAC WANDevice is already present, no action needed */
         mdm_freeObject(&mdmObj);
      }

   }

   if (added > 0)
   {
      mdm_increaseWanDeviceCount(added);
   }

   return ret;
}


#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */

#ifdef DMP_X_BROADCOM_COM_PPTPAC_1


CmsRet mdm_addDefaultPptpVpnObjects()
{
   void *mdmObj = NULL;
   _WanCommonIntfCfgObject *wanCommonObj = NULL;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 added = 0;
   UINT32 i;
   CmsRet ret;

   /*
    * Add all necessary VPN  WANDevice objects.
    */
   for (i=0; i < CMS_WANDEVICE_PPTPAC_COUNT; i++)
   {
      PUSH_INSTANCE_ID(&iidStack, CMS_WANDEVICE_PPTPAC+i);
      ret = mdm_getObject(MDMOID_WAN_DEV, &iidStack, &mdmObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_notice("adding PPTP AC WANDevice at %d", CMS_WANDEVICE_PPTPAC+i);
         added++;

         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_WAN_DEV;
         PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_PPTPAC+i);

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

         CMSMEM_REPLACE_STRING_FLAGS(wanCommonObj->WANAccessType, MDMVS_X_BROADCOM_COM_PPTPAC, mdmLibCtx.allocFlags); 

         if ((ret = mdm_setObject((void **) &wanCommonObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
         {
            mdm_freeObject((void **) &wanCommonObj);
            cmsLog_error("Failed to set WanCommonIntfCfgObject, ret = %d", ret);
         }
   
      }
      else
      {
         /* L2TPAC WANDevice is already present, no action needed */
         mdm_freeObject(&mdmObj);
      }

   }

   if (added > 0)
   {
      mdm_increaseWanDeviceCount(added);
   }

   return ret;
}


#endif /* DMP_X_BROADCOM_COM_PPTPAC_1 */

