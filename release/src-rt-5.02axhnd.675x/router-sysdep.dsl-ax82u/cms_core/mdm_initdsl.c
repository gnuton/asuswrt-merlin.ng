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


/*!\file mdm_initdsl.c
 * \brief This file contains DSL mdm init related functions.
 *
 */


#ifdef DMP_ADSLWAN_1

#if defined(DMP_X_BROADCOM_COM_XTMSTATS_1)
static CmsRet createDefaultXtmStatsObjects(InstanceIdStack wanDeviceIidStack);
#endif

CmsRet mdm_addDefaultWanDslObjects(void)
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 added = 0;

#ifdef DMP_X_BROADCOM_COM_ATMWAN_1
   void *mdmObj=NULL;
   _WanDslIntfCfgObject *dslObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc;

   /*
    * User has selected ATM as a WAN interface.  See if there is aleady an ATM
    * WAN device.  If not, create it at the fixed instance number.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   PUSH_INSTANCE_ID(&iidStack, CMS_WANDEVICE_ATM);
   ret = mdm_getObject(MDMOID_WAN_DEV, &iidStack, &mdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("adding ATM WANDevice");
      added++;

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_WAN_DEV;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_ATM);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not add WANDevice at %s, ret=%d", cmsMdm_dumpIidStack(&pathDesc.iidStack), ret);
         return ret;
      }
   
      if ((ret = mdm_getObject(MDMOID_WAN_DSL_INTF_CFG, &(pathDesc.iidStack), (void **) &dslObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get DSL_INTF_CFG, ret=%d", ret);
         return ret;
      }

      dslObj->enable = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(dslObj->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM, mdmLibCtx.allocFlags);

#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
      {
         MdmPathDescriptor peerPathDesc;
         char *fullPath=NULL;

         dslObj->X_BROADCOM_COM_EnableBonding = TRUE;
         dslObj->X_BROADCOM_COM_BondingLineNumber = 0;

         /* create a path to the other line */
         INIT_PATH_DESCRIPTOR(&peerPathDesc);
         peerPathDesc.oid = MDMOID_WAN_DEV;
         PUSH_INSTANCE_ID(&peerPathDesc.iidStack, CMS_WANDEVICE_ATMBONDING);
         cmsMdm_pathDescriptorToFullPath(&peerPathDesc, &fullPath);
         /* zero out the last . */
         fullPath[strlen(fullPath)-1] = '\0';
         dslObj->X_BROADCOM_COM_BondingPeerName = cmsMem_strdupFlags(fullPath, mdmLibCtx.allocFlags);
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      }
#endif

      if ((ret = mdm_setObject((void **) &dslObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set DLS_INTF_CFG, ret=%d", ret);
      }

#ifdef DMP_X_BROADCOM_COM_XTMSTATS_1
      /* need to add ATM interface statistics for this WAN device */
      if ((ret = createDefaultXtmStatsObjects(pathDesc.iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not create XTM Stats object, ret=%d", ret);
         return ret;
      }
#endif /* DMP_X_BROADCOM_COM_XTMSTATS_1 */

   }
   else
   {
#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_WAN_DEV;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_ATM);
      MdmPathDescriptor peerPathDesc;
      char *fullPath=NULL;

      if ((ret = mdm_getObject(MDMOID_WAN_DSL_INTF_CFG, &(pathDesc.iidStack), (void **) &dslObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get DSL_INTF_CFG, ret=%d", ret);
         return ret;
      }
      if (IS_EMPTY_STRING(dslObj->X_BROADCOM_COM_BondingPeerName))
      {
         /* create a path to the other line */
         INIT_PATH_DESCRIPTOR(&peerPathDesc);
         peerPathDesc.oid = MDMOID_WAN_DEV;
         PUSH_INSTANCE_ID(&peerPathDesc.iidStack, CMS_WANDEVICE_ATMBONDING);
         cmsMdm_pathDescriptorToFullPath(&peerPathDesc, &fullPath);
         /* zero out the last . */
         fullPath[strlen(fullPath)-1] = '\0';
         dslObj->X_BROADCOM_COM_BondingPeerName = cmsMem_strdupFlags(fullPath, mdmLibCtx.allocFlags);
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);

         dslObj->X_BROADCOM_COM_EnableBonding = TRUE;
         dslObj->X_BROADCOM_COM_BondingLineNumber = 0;

         if ((ret = mdm_setObject((void **) &dslObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not set DLS_INTF_CFG, ret=%d", ret);
         }
      }
#endif

      /* ATM WANDevice is already present, no action needed */
      mdm_freeObject(&mdmObj);
   }

#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
   /*
    * User has selected ATM Bonding.  Need to create another WAN interface for that.
    * See if there is aleady a ATM Bonding WAN device.  If not, create it at the fixed instance number.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   PUSH_INSTANCE_ID(&iidStack, CMS_WANDEVICE_ATMBONDING);
   ret = mdm_getObject(MDMOID_WAN_DEV, &iidStack, &mdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("adding ATM Bonding WANDevice");
      added++;

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_WAN_DEV;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_ATMBONDING);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not add WANDevice at %s, ret=%d", cmsMdm_dumpIidStack(&pathDesc.iidStack), ret);
         return ret;
      }
   
      if ((ret = mdm_getObject(MDMOID_WAN_DSL_INTF_CFG, &(pathDesc.iidStack), (void **) &dslObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get DSL_INTF_CFG, ret=%d", ret);
         return ret;
      }

      dslObj->enable = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(dslObj->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM, mdmLibCtx.allocFlags);

      {
         MdmPathDescriptor peerPathDesc;
         char *fullPath=NULL;

         dslObj->X_BROADCOM_COM_EnableBonding = TRUE;
         dslObj->X_BROADCOM_COM_BondingLineNumber = 1;

         /* create a path to the other line */
         INIT_PATH_DESCRIPTOR(&peerPathDesc);
         peerPathDesc.oid = MDMOID_WAN_DEV;
         PUSH_INSTANCE_ID(&peerPathDesc.iidStack, CMS_WANDEVICE_ATM);
         cmsMdm_pathDescriptorToFullPath(&peerPathDesc, &fullPath);
         /* zero out the last . */
         fullPath[strlen(fullPath)-1] = '\0';
         dslObj->X_BROADCOM_COM_BondingPeerName = cmsMem_strdupFlags(fullPath, mdmLibCtx.allocFlags);
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      }

      if ((ret = mdm_setObject((void **) &dslObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set DLS_INTF_CFG, ret=%d", ret);
      }

#ifdef DMP_X_BROADCOM_COM_XTMSTATS_1
      /* need to add ATM interface statistics for this WAN device */
      if ((ret = createDefaultXtmStatsObjects(pathDesc.iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not create XTM Stats object, ret=%d", ret);
         return ret;
      }
#endif /* DMP_X_BROADCOM_COM_XTMSTATS_1 */

   }
   else
   {
      /* ATM Bonding WANDevice is already present, no action needed */
      mdm_freeObject(&mdmObj);
   }

#endif /* DMP_X_BROADCOM_COM_DSLBONDING_1 */

#endif /* DMP_X_BROADCOM_COM_ATMWAN_1 */

#ifdef DMP_X_BROADCOM_COM_PTMWAN_1
   /*
    * User has selected PTM as a WAN interface.  See if there is aleady a PTM
    * WAN device.  If not, create it at the fixed instance number.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   PUSH_INSTANCE_ID(&iidStack, CMS_WANDEVICE_PTM);
   ret = mdm_getObject(MDMOID_WAN_DEV, &iidStack, &mdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("adding PTM WANDevice");
      added++;

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_WAN_DEV;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_PTM);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not add WANDevice at %s, ret=%d", cmsMdm_dumpIidStack(&pathDesc.iidStack), ret);
         return ret;
      }
   
      if ((ret = mdm_getObject(MDMOID_WAN_DSL_INTF_CFG, &(pathDesc.iidStack), (void **) &dslObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get DSL_INTF_CFG, ret=%d", ret);
         return ret;
      }

      dslObj->enable = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(dslObj->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM, mdmLibCtx.allocFlags);

#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
      {
         MdmPathDescriptor peerPathDesc;
         char *fullPath=NULL;

         dslObj->X_BROADCOM_COM_EnableBonding = TRUE;
         dslObj->X_BROADCOM_COM_BondingLineNumber = 0;

         /* create a path to the other line */
         INIT_PATH_DESCRIPTOR(&peerPathDesc);
         peerPathDesc.oid = MDMOID_WAN_DEV;
         PUSH_INSTANCE_ID(&peerPathDesc.iidStack, CMS_WANDEVICE_PTMBONDING);
         cmsMdm_pathDescriptorToFullPath(&peerPathDesc, &fullPath);
         /* zero out the last . */
         fullPath[strlen(fullPath)-1] = '\0';
         dslObj->X_BROADCOM_COM_BondingPeerName = cmsMem_strdupFlags(fullPath, mdmLibCtx.allocFlags);
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      }
#endif

      if ((ret = mdm_setObject((void **) &dslObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set DLS_INTF_CFG, ret=%d", ret);
      }

#ifdef DMP_X_BROADCOM_COM_XTMSTATS_1
      /* need to add PTM interface statistics for this WAN device */
      if ((ret = createDefaultXtmStatsObjects(pathDesc.iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not create XTM Stats object, ret=%d", ret);
         return ret;
      }
#endif /* DMP_X_BROADCOM_COM_XTMSTATS_1 */

   }
   else
   {

#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_WAN_DEV;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_PTM);
      MdmPathDescriptor peerPathDesc;
      char *fullPath=NULL;

      if ((ret = mdm_getObject(MDMOID_WAN_DSL_INTF_CFG, &(pathDesc.iidStack), (void **) &dslObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get DSL_INTF_CFG, ret=%d", ret);
         return ret;
      }
      if (IS_EMPTY_STRING(dslObj->X_BROADCOM_COM_BondingPeerName))
      {
         /* create a path to the other line */
         INIT_PATH_DESCRIPTOR(&peerPathDesc);
         peerPathDesc.oid = MDMOID_WAN_DEV;
         PUSH_INSTANCE_ID(&peerPathDesc.iidStack, CMS_WANDEVICE_PTMBONDING);
         cmsMdm_pathDescriptorToFullPath(&peerPathDesc, &fullPath);
         /* zero out the last . */
         fullPath[strlen(fullPath)-1] = '\0';
         dslObj->X_BROADCOM_COM_BondingPeerName = cmsMem_strdupFlags(fullPath, mdmLibCtx.allocFlags);
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);

         dslObj->X_BROADCOM_COM_EnableBonding = TRUE;
         dslObj->X_BROADCOM_COM_BondingLineNumber = 0;

         if ((ret = mdm_setObject((void **) &dslObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not set DLS_INTF_CFG, ret=%d", ret);
         }
      }
#endif

      /* PTM WANDevice is already present, no action needed */
      mdm_freeObject(&mdmObj);
   }

#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
   /*
    * User has selected PTM Bonding.  Need to create another WAN interface for that.
    * See if there is aleady a PTM Bonding WAN device.  If not, create it at the fixed instance number.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   PUSH_INSTANCE_ID(&iidStack, CMS_WANDEVICE_PTMBONDING);
   ret = mdm_getObject(MDMOID_WAN_DEV, &iidStack, &mdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("adding PTM Bonding WANDevice");
      added++;

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_WAN_DEV;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_PTMBONDING);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not add WANDevice at %s, ret=%d", cmsMdm_dumpIidStack(&pathDesc.iidStack), ret);
         return ret;
      }
   
      if ((ret = mdm_getObject(MDMOID_WAN_DSL_INTF_CFG, &(pathDesc.iidStack), (void **) &dslObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get DSL_INTF_CFG, ret=%d", ret);
         return ret;
      }

      dslObj->enable = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(dslObj->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM, mdmLibCtx.allocFlags);

      {
         MdmPathDescriptor peerPathDesc;
         char *fullPath=NULL;

         dslObj->X_BROADCOM_COM_EnableBonding = TRUE;
         dslObj->X_BROADCOM_COM_BondingLineNumber = 1;

         /* create a path to the other line */
         INIT_PATH_DESCRIPTOR(&peerPathDesc);
         peerPathDesc.oid = MDMOID_WAN_DEV;
         PUSH_INSTANCE_ID(&peerPathDesc.iidStack, CMS_WANDEVICE_PTM);
         cmsMdm_pathDescriptorToFullPath(&peerPathDesc, &fullPath);
         /* zero out the last . */
         fullPath[strlen(fullPath)-1] = '\0';
         dslObj->X_BROADCOM_COM_BondingPeerName = cmsMem_strdupFlags(fullPath, mdmLibCtx.allocFlags);
         CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      }

      if ((ret = mdm_setObject((void **) &dslObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set DLS_INTF_CFG, ret=%d", ret);
      }

#ifdef DMP_X_BROADCOM_COM_XTMSTATS_1
      /* need to add PTM interface statistics for this WAN device */
      if ((ret = createDefaultXtmStatsObjects(pathDesc.iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not create XTM Stats object, ret=%d", ret);
         return ret;
      }
#endif /* DMP_X_BROADCOM_COM_XTMSTATS_1 */

   }
   else
   {
      /* PTM Bonding WANDevice is already present, no action needed */
      mdm_freeObject(&mdmObj);
   }

#endif /* DMP_X_BROADCOM_COM_DSLBONDING_1 */

#endif /* DMP_X_BROADCOM_COM_PTMWAN_1 */


   if (added > 0)
   {
      mdm_increaseWanDeviceCount(added);
   }

   return ret;
}


#ifdef DMP_X_BROADCOM_COM_XTMSTATS_1
/*
 * Perhaps xtm and xdsl specific initialization should be moved to a
 * separate file, just like gpon and wlan are in their own files?
 */
CmsRet createDefaultXtmStatsObjects(InstanceIdStack wanDeviceIidStack)
{
   MdmPathDescriptor pathDesc;
   UINT32 port;
   CmsRet ret;
   XtmInterfaceStatsObject *xtmStatsObj = NULL;

   for (port = 1; port <= 4 ; port++)
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.iidStack = wanDeviceIidStack;
      pathDesc.oid = MDMOID_XTM_INTERFACE_STATS;

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to create WANDevice.1.X_BROADCOM_COM_XTM_INTERFACE_STATS.%d, ret=%d",port, ret);
         return ret;
      }

      /* get object just created */
      if ((ret = mdm_getObject(MDMOID_XTM_INTERFACE_STATS, &(pathDesc.iidStack), (void **) &xtmStatsObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get MDMOID_XTM_INTERFACE_STATS, ret=%d", ret);
         return ret;
      }

      xtmStatsObj->port = port;

      if ((ret = mdm_setObject((void **) &xtmStatsObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set MDMOID_XTM_INTERFACE_STATS, ret=%d", ret);
      }

   } /* port */
   return ret;
}
#endif  /* DMP_X_BROADCOM_COM_XTMSTATS_1 */



#endif /* DMP_ADSLWAN_1 */
