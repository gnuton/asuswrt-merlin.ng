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
#include "mdm_initdsl.h"

/*!\file mdm2_initdsl.c
 * \brief This file contains DSL mdm init related functions for Device2.
 *
 */

#ifdef DMP_DEVICE2_DSL_1


CmsRet mdm_addDslLineAndChannel(int lineNumber)
{
   CmsRet ret = CMSRET_SUCCESS;
   int i;
   _Dev2DslLineObject *dslLineObj=NULL;
   _Dev2DslChannelObject *dslChannelObj=NULL;
   MdmPathDescriptor pathDesc;
   char *lineFullPathString=NULL;
   char lineNameStr[32];
   char channelAliasStr[48];

   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_DSL_LINE;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for DEV2_DSL_LINE %d failed, ret=%d", lineNumber,ret);
      return ret;
   }
   if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &dslLineObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get dslLineObj object for line %d, ret=%d", lineNumber,ret);
      return ret;
   }
   /* init line object */
   dslLineObj->enable = TRUE;
   dslLineObj->X_BROADCOM_COM_BondingLineNumber = lineNumber;

   CMSMEM_REPLACE_STRING_FLAGS(dslLineObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
   
   sprintf(lineNameStr,"dsl%d",lineNumber);
   CMSMEM_REPLACE_STRING_FLAGS(dslLineObj->name, lineNameStr, mdmLibCtx.allocFlags);
   dslLineObj->upstream = TRUE;
   ret = mdm_setObject((void **) &dslLineObj, &pathDesc.iidStack, FALSE);
   mdm_freeObject((void **)&dslLineObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set dslLineObj for line %d. ret=%d", lineNumber,ret);
      return ret;
   }

   cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &lineFullPathString);
   /* add two channels, each physical line has two channels (atm, ptm) */
   for (i=0; i < 2; i++)
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_DSL_CHANNEL;
      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(lineFullPathString);
         cmsLog_error("failed add instance dslChannel object %d, ret=%d", i,ret);
         return ret;
      }
      if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &dslChannelObj)) != CMSRET_SUCCESS)
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(lineFullPathString);
         cmsLog_error("failed to get dslChannelObj object, ret=%d", ret);
         return ret;
      }
      
      /* init channel object */
      dslChannelObj->enable = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(dslChannelObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
      if (i == 0)
      {
         CMSMEM_REPLACE_STRING_FLAGS(dslChannelObj->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM, mdmLibCtx.allocFlags);
         sprintf(channelAliasStr,"cpe-%s-atm-channel",lineNameStr);
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(dslChannelObj->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM, mdmLibCtx.allocFlags);
         sprintf(channelAliasStr,"cpe-%s-ptm-channel",lineNameStr);
      }
      CMSMEM_REPLACE_STRING_FLAGS(dslChannelObj->alias, channelAliasStr, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(dslChannelObj->lowerLayers, lineFullPathString, mdmLibCtx.allocFlags);
      ret = mdm_setObject((void **) &dslChannelObj, &pathDesc.iidStack, FALSE);
      mdm_freeObject((void **)&dslChannelObj); 
   } /* create 2 channels */
   CMSMEM_FREE_BUF_AND_NULL_PTR(lineFullPathString);
   return ret;
}

#ifdef DMP_DEVICE2_BONDEDDSL_1
CmsRet mdm_addDslBondingGroups(void)
{
   _Dev2DslChannelObject *dslChannelObj=NULL;
   _Dev2DslBondingGroupObject *dslBondingObj=NULL;
   _Dev2DslBondingGroupBondedChannelObject *dslBondedChannelObj=NULL;
   MdmPathDescriptor channelPathDesc;
   MdmPathDescriptor pathDesc;
   char *atmChannel1FullPathStr=NULL;
   char *ptmChannel1FullPathStr=NULL;
   char *atmChannel2FullPathStr=NULL;
   char *ptmChannel2FullPathStr=NULL;
   char atmChannelsCSL[64]={0};
   char ptmChannelsCSL[64]={0};
   int i=0, group=0;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack bondingGroupIidstack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   /* gather the ATM channels and PTM channels' full paths */
   while ((mdm_getNextObject(MDMOID_DEV2_DSL_CHANNEL, &iidStack, (void **) &dslChannelObj)) == CMSRET_SUCCESS)
   {
      INIT_PATH_DESCRIPTOR(&channelPathDesc);
      channelPathDesc.oid = MDMOID_DEV2_DSL_CHANNEL;
      channelPathDesc.iidStack = iidStack;
      if (!cmsUtl_strcmp(dslChannelObj->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM))
      {
         if (atmChannel1FullPathStr == NULL)
         {
            cmsMdm_pathDescriptorToFullPathNoEndDot(&channelPathDesc,&atmChannel1FullPathStr);
            cmsUtl_addFullPathToCSL(atmChannel1FullPathStr,atmChannelsCSL,64);
         }
         else
         {
            cmsMdm_pathDescriptorToFullPathNoEndDot(&channelPathDesc,&atmChannel2FullPathStr);
            cmsUtl_addFullPathToCSL(atmChannel2FullPathStr,atmChannelsCSL,64);
         }
      }
      else
      {
         /* ptm */
         if (ptmChannel1FullPathStr == NULL)
         {
            cmsMdm_pathDescriptorToFullPathNoEndDot(&channelPathDesc,&ptmChannel1FullPathStr);
            cmsUtl_addFullPathToCSL(ptmChannel1FullPathStr,ptmChannelsCSL,64);
         }
         else
         {
            cmsMdm_pathDescriptorToFullPathNoEndDot(&channelPathDesc,&ptmChannel2FullPathStr);
            cmsUtl_addFullPathToCSL(ptmChannel2FullPathStr,ptmChannelsCSL,64);
         }
      }
      mdm_freeObject((void **) &dslChannelObj);
   }

   for (group=0; group < 2; group++)
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_DSL_BONDING_GROUP;

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed add instance dsl bonding group object (groupId %d), ret=%d", group,ret);
         goto dslBondingInitError;
      }
      if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &dslBondingObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to get dslBondingObj object group %d, ret=%d", group,ret);
         goto dslBondingInitError;
      }
      bondingGroupIidstack = pathDesc.iidStack;
      /* init bonding object */
      dslBondingObj->enable = TRUE;
      /* group 0 is ATM, group 1 is PTM */
      dslBondingObj->groupID = group;
      if (group == 0)
      {
         CMSMEM_REPLACE_STRING_FLAGS(dslBondingObj->name,"dslbonding-group-atm", mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(dslBondingObj->lowerLayers, atmChannelsCSL, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(dslBondingObj->bondScheme, MDMVS_ATM, mdmLibCtx.allocFlags);
      }
      else
      {
         CMSMEM_REPLACE_STRING_FLAGS(dslBondingObj->name,"dslbonding-group-ptm", mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(dslBondingObj->lowerLayers, ptmChannelsCSL, mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(dslBondingObj->bondScheme, MDMVS_ETHERNET, mdmLibCtx.allocFlags);
      }
      CMSMEM_REPLACE_STRING_FLAGS(dslBondingObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(dslBondingObj->groupStatus, MDMVS_NOPEER, mdmLibCtx.allocFlags);
         
      /* in this bonding group, there are 2 bonded channels */
      for (i=0; i < 2; i++)
      {
         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_DEV2_DSL_BONDING_GROUP_BONDED_CHANNEL;
         pathDesc.iidStack = bondingGroupIidstack;
         if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed add instance dsl bonded Channel object %d, ret=%d", i,ret);
            goto dslBondingInitError;
         }
         if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &dslBondedChannelObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to get dslBondedChannelObj object, ret=%d", ret);
            goto dslBondingInitError;
         }
         if (group == 0)
         {
            /* atm group */
            if (i == 0)
            {
               /* first atm channel */
               CMSMEM_REPLACE_STRING_FLAGS(dslBondedChannelObj->channel, atmChannel1FullPathStr, mdmLibCtx.allocFlags);
            }
            else
            {
               /* 2nd atm channel */
               CMSMEM_REPLACE_STRING_FLAGS(dslBondedChannelObj->channel, atmChannel2FullPathStr, mdmLibCtx.allocFlags);
            }
         }
         else
         {
            /* ptm group*/
            if (i == 0)
            {
               /* first ptm channel */
               CMSMEM_REPLACE_STRING_FLAGS(dslBondedChannelObj->channel, ptmChannel1FullPathStr, mdmLibCtx.allocFlags);
            }
            else
            {
               /* 2nd ptm channel */
               CMSMEM_REPLACE_STRING_FLAGS(dslBondedChannelObj->channel, ptmChannel2FullPathStr, mdmLibCtx.allocFlags);
            }
         }
         ret = mdm_setObject((void **) &dslBondedChannelObj, &pathDesc.iidStack, FALSE);
         mdm_freeObject((void **)&dslBondedChannelObj); 
      } /* create 2 channels */
      dslBondingObj->bondedChannelNumberOfEntries += 2;
      ret = mdm_setObject((void **) &dslBondingObj, &bondingGroupIidstack, FALSE);
      mdm_freeObject((void **)&dslBondingObj); 
   } /* bonding group */
   CMSMEM_FREE_BUF_AND_NULL_PTR(atmChannel1FullPathStr);
   CMSMEM_FREE_BUF_AND_NULL_PTR(atmChannel2FullPathStr);
   CMSMEM_FREE_BUF_AND_NULL_PTR(ptmChannel1FullPathStr);
   CMSMEM_FREE_BUF_AND_NULL_PTR(ptmChannel2FullPathStr);
   return ret;

   dslBondingInitError:
      CMSMEM_FREE_BUF_AND_NULL_PTR(atmChannel1FullPathStr);
      CMSMEM_FREE_BUF_AND_NULL_PTR(atmChannel2FullPathStr);
      CMSMEM_FREE_BUF_AND_NULL_PTR(ptmChannel1FullPathStr);
      CMSMEM_FREE_BUF_AND_NULL_PTR(ptmChannel2FullPathStr);
      if (dslBondingObj != NULL)
      {
         mdm_freeObject((void **) &dslBondingObj);
      }
      if (dslBondedChannelObj != NULL)
      {
         mdm_freeObject((void **) &dslBondedChannelObj);
      }
      return ret;
}
#endif /* DMP_DEVICE2_BONDEDDSL_1 */

/* By default, the objects needed to be are are Device.DSL.Line and Device.DSL.Channel */
CmsRet mdm_addDefaultDslObjects_dev2(void)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _Dev2DslObject *dslObj=NULL;
   void *mdmObj=NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Adding DSL. object (enter)");

   /* first check if there is a DSL.Line instance  first */
   ret = mdm_getNextObject(MDMOID_DEV2_DSL_LINE, &iidStack, &mdmObj);

   if (ret != CMSRET_SUCCESS)
   {
      /* Add the primary DSL LINE default object */
      mdm_addDslLineAndChannel(0);

      /* need to manually update the count when adding objects during mdm_init */
      if ((ret = mdm_getObject(MDMOID_DEV2_DSL, &iidStack, (void **) &dslObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get dslObj. ret=%d", ret);
         return ret;
      }
      dslObj->lineNumberOfEntries++;
      dslObj->channelNumberOfEntries += 2; /* 2 channels per line */

#ifdef DMP_DEVICE2_BONDEDDSL_1
      /* This is a bonded image running on bonded board, add another DSL line */
      if ((ret = mdm_addDslLineAndChannel(1)) == CMSRET_SUCCESS)
      {
         dslObj->lineNumberOfEntries++;
         dslObj->channelNumberOfEntries += 2; /* 2 channels per line */

         /* In addition, we need to add bonding groups as well.
          * There will be 2 default bonding groups: 1 with 2 ATM channels, and another with 2 PTM channels.
          * This is what we support on our reference hw/sw.  At runtime, the statuses are updated appropriately.
          */
         mdm_addDslBondingGroups();
         dslObj->bondingGroupNumberOfEntries+=2; /* 2 bonding groups added */
      }
#endif /* DMP_DEVICE2_BONDEDDSL_1 */
      ret = mdm_setObject((void **) &dslObj, &iidStack, FALSE);
   }
   else
   {
      /* DSL. present, */
#ifdef DMP_DEVICE2_BONDEDDSL_1
      INIT_INSTANCE_ID_STACK(&iidStack);      
      if ((ret = mdm_getObject(MDMOID_DEV2_DSL, &iidStack, (void **) &dslObj)) == CMSRET_SUCCESS)
      {
         if (dslObj->bondingGroupNumberOfEntries == 0)
         {
            /* This is a bonded image running on bonded board, add another DSL line */
            if ((ret = mdm_addDslLineAndChannel(1)) == CMSRET_SUCCESS)
            {
               dslObj->lineNumberOfEntries++;
               dslObj->channelNumberOfEntries += 2; /* 2 channels per line */

               /* In addition, we need to add bonding groups as well.
                * There will be 2 default bonding groups: 1 with 2 ATM channels, and another with 2 PTM channels.
                * This is what we support on our reference hw/sw.  At runtime, the statuses are updated appropriately.
                */
               mdm_addDslBondingGroups();
               dslObj->bondingGroupNumberOfEntries+=2; /* 2 bonding groups added */
               ret = mdm_setObject((void **) &dslObj, &iidStack, FALSE);
            }
         }
      }
#endif /* DMP_DEVICE2_BONDEDDSL_1 */

      mdm_freeObject(&mdmObj);
   } 

#ifdef DMP_DEVICE2_FAST_1
   /* in our device, there is always DSL line.  FAST can be an addition. */
   /* add primary fast line, if supported */
   ret = mdm_addDefaultFastObjects_dev2();
#endif
   return ret;
}

#endif /* DMP_DEVICE2_DSL_1 */

#ifdef DMP_DEVICE2_FAST_1

#ifdef DMP_DEVICE2_BONDEDFAST_1
CmsRet mdm_addFastBondingGroup(void)
{
   _Dev2FastLineObject *fastLineObj=NULL;
   _Dev2DslBondingGroupObject *dslBondingObj=NULL;
   _Dev2DslBondingGroupBondedChannelObject *dslBondedChannelObj=NULL;
   MdmPathDescriptor linePathDesc;
   MdmPathDescriptor pathDesc;
   int i=0, group=0;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack bondingGroupIidstack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;
   char *line1FullPathStr=NULL;
   char *line2FullPathStr=NULL;
   char lineCSL[64]={0};

   /* gather the FAST.Lines' full paths */
   while ((mdm_getNextObject(MDMOID_DEV2_FAST_LINE, &iidStack, (void **) &fastLineObj)) == CMSRET_SUCCESS)
   {
      INIT_PATH_DESCRIPTOR(&linePathDesc);
      linePathDesc.oid = MDMOID_DEV2_FAST_LINE;
      linePathDesc.iidStack = iidStack;

      if (line1FullPathStr == NULL)
      {
         cmsMdm_pathDescriptorToFullPathNoEndDot(&linePathDesc,&line1FullPathStr);
         cmsUtl_addFullPathToCSL(line1FullPathStr,lineCSL,64);
      }
      else
      {
         cmsMdm_pathDescriptorToFullPathNoEndDot(&linePathDesc,&line2FullPathStr);
         cmsUtl_addFullPathToCSL(line2FullPathStr,lineCSL,64);
      }
      mdm_freeObject((void **) &fastLineObj);
   }
   
   /* add the bonding group for FAST interfaces */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_DSL_BONDING_GROUP;

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed add instance fast bonding group object (groupId %d), ret=%d", group,ret);
      goto dslBondingInitError;
   }
   if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &dslBondingObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get dslBondingObj object group %d, ret=%d", group,ret);
      goto dslBondingInitError;
   }
   bondingGroupIidstack = pathDesc.iidStack;
   /* init bonding object */
   dslBondingObj->enable = TRUE;
   dslBondingObj->groupID = BONDING_GROUP_FAST;
   CMSMEM_REPLACE_STRING_FLAGS(dslBondingObj->name,"dslbonding-group-fast", mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(dslBondingObj->lowerLayers, lineCSL, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(dslBondingObj->bondScheme, MDMVS_ETHERNET, mdmLibCtx.allocFlags);

   /* in this bonding group, there are 2 lines */
   for (i=0; i < 2; i++)
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_DEV2_DSL_BONDING_GROUP_BONDED_CHANNEL;
      pathDesc.iidStack = bondingGroupIidstack;
      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed add instance fast bonded Channel object %d, ret=%d", i,ret);
         goto dslBondingInitError;
      }
      if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &dslBondedChannelObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to get dslBondedChannelObj object for fast line, ret=%d", ret);
         goto dslBondingInitError;
      }
      if (i == 0)
      {
         /* first FAST line */
         CMSMEM_REPLACE_STRING_FLAGS(dslBondedChannelObj->channel, line1FullPathStr, mdmLibCtx.allocFlags);
         CMSMEM_FREE_BUF_AND_NULL_PTR(line1FullPathStr);
      }
      else
      {
         /* 2nd FAST line */
         CMSMEM_REPLACE_STRING_FLAGS(dslBondedChannelObj->channel, line2FullPathStr, mdmLibCtx.allocFlags);
         CMSMEM_FREE_BUF_AND_NULL_PTR(line2FullPathStr);
      }

      ret = mdm_setObject((void **) &dslBondedChannelObj, &pathDesc.iidStack, FALSE);
      mdm_freeObject((void **)&dslBondedChannelObj); 
   }

   dslBondingObj->bondedChannelNumberOfEntries += 2;
   ret = mdm_setObject((void **) &dslBondingObj, &bondingGroupIidstack, FALSE);
   mdm_freeObject((void **)&dslBondingObj); 

   return ret;

 dslBondingInitError:
   CMSMEM_FREE_BUF_AND_NULL_PTR(line1FullPathStr);
   CMSMEM_FREE_BUF_AND_NULL_PTR(line2FullPathStr);

   if (dslBondingObj != NULL)
   {
      mdm_freeObject((void **) &dslBondingObj);
   }
   if (dslBondedChannelObj != NULL)
   {
      mdm_freeObject((void **) &dslBondedChannelObj);
   }
   return ret;
}
#endif /* DMP_DEVICE2_BONDEDFAST_1 */

CmsRet mdm_addFastLine(int lineNumber)
{
   CmsRet ret = CMSRET_SUCCESS;
   _Dev2FastLineObject *fastLineObj=NULL;
   MdmPathDescriptor pathDesc;
   char lineNameStr[32];
  
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_FAST_LINE;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for DEV2_FAST_LINE %d failed, ret=%d", lineNumber,ret);
      return ret;
   }
   if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &fastLineObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get fastLineObj object for line %d, ret=%d", lineNumber,ret);
      return ret;
   }
   /* init line object */
   fastLineObj->enable = TRUE;
   fastLineObj->X_BROADCOM_COM_BondingLineNumber = lineNumber;
   CMSMEM_REPLACE_STRING_FLAGS(fastLineObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
   sprintf(lineNameStr,"fast%d",lineNumber);
   CMSMEM_REPLACE_STRING_FLAGS(fastLineObj->name, lineNameStr, mdmLibCtx.allocFlags);
   fastLineObj->upstream = TRUE;
   ret = mdm_setObject((void **) &fastLineObj, &pathDesc.iidStack, FALSE);
   mdm_freeObject((void **)&fastLineObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set fastLineObj for line %d. ret=%d", lineNumber,ret);
   }
   return ret;
}

/* By default, the objects needed to be are are Device.FAST.Line */
CmsRet mdm_addDefaultFastObjects_dev2(void)
{
   _Dev2DslObject *dslObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _Dev2FastObject *fastObj=NULL;
   void *mdmObj=NULL;
   CmsRet ret = CMSRET_SUCCESS;

   /* first check if there is a FAST.Line instance  first */
   ret = mdm_getNextObject(MDMOID_DEV2_FAST_LINE, &iidStack, &mdmObj);

   if (ret != CMSRET_SUCCESS)
   {
      /* Add the primary FAST LINE default object */
      mdm_addFastLine(0);

      /* need to manually update the count when adding objects during mdm_init */
      if ((ret = mdm_getObject(MDMOID_DEV2_FAST, &iidStack, (void **) &fastObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get fastObj. ret=%d", ret);
         return ret;
      }
      fastObj->lineNumberOfEntries++;

#ifdef DMP_DEVICE2_BONDEDFAST_1
      /* This is a bonded image running on bonded board, add another DSL line */
      if ((ret = mdm_addFastLine(1)) == CMSRET_SUCCESS)
      {
         fastObj->lineNumberOfEntries++;

         /* In addition, we need to add bonding groups as well.
          * There will be 1 default bonding group consisting of these 2 FAST lines.
          * This is what we support on our reference hw/sw.  At runtime, the statuses are updated appropriately.
          */
         mdm_addFastBondingGroup();
         if ((ret = mdm_getObject(MDMOID_DEV2_DSL, &iidStack, (void **) &dslObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get dslObj. ret=%d", ret);
         }
         else
         {
            dslObj->bondingGroupNumberOfEntries+=1; /* 1 bonding group added for gfast lines */  
            ret = mdm_setObject((void **) &dslObj, &iidStack, FALSE);
            mdm_freeObject((void **)&dslObj);
         }
      }
#endif /* DMP_DEVICE2_BONDEDFAST_1 */
      ret = mdm_setObject((void **) &fastObj, &iidStack, FALSE);
      mdm_freeObject((void **)&fastObj);
   }
   else
   {
      /* FAST. is present */

#ifdef DMP_DEVICE2_BONDEDFAST_1
      INIT_INSTANCE_ID_STACK(&iidStack);      
      if ((ret = mdm_getObject(MDMOID_DEV2_FAST, &iidStack, (void **) &fastObj)) == CMSRET_SUCCESS)
      {
         if (fastObj->lineNumberOfEntries < 2)
         {
            /* This is a bonded image running on bonded board, add another FAST line */
            if ((ret = mdm_addFastLine(1)) == CMSRET_SUCCESS)
            {
               fastObj->lineNumberOfEntries++;
               mdm_addFastBondingGroup();
               ret = mdm_setObject((void **) &fastObj, &iidStack, FALSE);
               mdm_freeObject((void **)&fastObj);
               if ((ret = mdm_getObject(MDMOID_DEV2_DSL, &iidStack, (void **) &dslObj)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("Failed to get dslObj. ret=%d", ret);
               }
               else
               {
                  dslObj->bondingGroupNumberOfEntries+=1; /* 1 bonding group added for gfast lines */  
                  ret = mdm_setObject((void **) &dslObj, &iidStack, FALSE);
                  mdm_freeObject((void **)&dslObj);
               }
            }
         }
      }
#endif /* DMP_DEVICE2_BONDEDFAST_1 */

      mdm_freeObject(&mdmObj);
   }

   return ret;
}
#endif /* DMP_DEVICE2_FAST_1 */
