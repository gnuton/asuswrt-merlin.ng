/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
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


/*!\file rut_lan6.c
 * \brief All of the functions in this file are for IPv6 multicast.
 */

#include <stdio.h>

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_network.h"
#include "rut_wan.h"
#include "rut_multicast.h"
#include "qdm_multicast.h"
#include "bridgeutil.h"
#include "bcm_mcast_api.h"
#include "beep_networking.h"

#ifdef DMP_X_BROADCOM_COM_MLDSNOOP_1

UBOOL8 rutMulti_isMldSnoopingCfgChanged(const _MldSnoopingCfgObject *newObj,
                                       const _MldSnoopingCfgObject *currObj) 
{
   if (!POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      return FALSE;
   }

   if (cmsUtl_strcmp(newObj->mode, currObj->mode) ||
       newObj->lanToLanEnable != currObj->lanToLanEnable)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


void rutMulti_configMldSnooping(const char *brIntfName,
                                UINT32 mode, SINT32 lanToLanEnable)
{
   int ret;
   int ifi;

   cmsLog_debug("entered: brIntfName=%s mode=%d lanToLan=0x%x",
                brIntfName, mode, lanToLanEnable);

   if (IS_EMPTY_STRING(brIntfName))
   {
      cmsLog_debug("brIntfName is NULL, do nothing");
      return;
   }

   ifi = cmsNet_getIfindexByIfname(brIntfName);
   ret = bcm_mcast_api_set_snooping_cfg(-1, ifi, BCM_MCAST_PROTO_IPV6, mode, lanToLanEnable);
   if(ret < 0)
   {
      cmsLog_error("enable mld snooping on %s mode %d l2l %d failed, ret=%d", 
                          brIntfName, mode, lanToLanEnable, ret);
   }
}


void rutMulti_updateMldSnooping(const char *brIntfName)
{
   char *fullPath = NULL;
   int mode;
   CmsRet ret;

   cmsLog_debug("Entered: brIntfName=%s", brIntfName);

   /* go from brIntfName to MLD snooping object */
   ret = qdmMulti_getAssociatedMldSnoopingFullPathLocked(brIntfName, &fullPath);
   if (ret != CMSRET_SUCCESS)
   {
      /* During delete, we cannot get the bridge object anymore.  Don't
       * complain loudly and just return.
       */
      cmsLog_debug("getAssociatedMldSnooping on %s failed, ret=%d", brIntfName, ret);
      return;
   }

   ret = qdmMulti_getAssociatedBridgeModeLocked(fullPath, &mode);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get bridge's mode, ret=%d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      return;
   }
   else if (mode != INTFGRP_BR_HOST_MODE)
   {
      cmsLog_debug("mldsnooping is not supported for BEEP bridge %s", brIntfName);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      return;
   }

   rutMulti_updateIgmpMldSnoopingObj(fullPath);

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);

   return;
}
#endif  /* DMP_X_BROADCOM_COM_MLDSNOOP_1 */



void rutMulti_updateMldSnoopingIntfList()
{
#ifdef DMP_X_BROADCOM_COM_MLD_1
   char mldSnoopingIntfNames[128]={0};
   UBOOL8 isMld = TRUE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MldCfgObject *mldObj = NULL;
   CmsRet ret;

   ret = rutMulti_getAllSnoopingIntfNames(isMld, mldSnoopingIntfNames, sizeof(mldSnoopingIntfNames));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("getAllSnoopingIntfNames failed, ret=%d", ret);
      return;
   }

   if ((ret = cmsObj_get(MDMOID_MLD_CFG,
                         &iidStack,
                         0, (void **) &mldObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MDMOID_MLD_CFG, ret=%d", ret);
      return;
   }
   
   if (IS_EMPTY_STRING(mldSnoopingIntfNames))
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(mldObj->mldBridgeIfNames);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(mldObj->mldBridgeIfNames, mldSnoopingIntfNames, mdmLibCtx.allocFlags);
   }

   if ((ret = cmsObj_set(mldObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of MLDp obj failed, ret=%d", ret);
   }
   
   cmsObj_free((void **) &mldObj);

#endif /* DMP_X_BROADCOM_COM_MLD_1 */

   return;
}


void rutMulti_updateMldProxyIntfList()
{
#ifdef DMP_X_BROADCOM_COM_MLD_1
   char proxyIntfNames[128]={0};
   char sourceIntfNames[128]={0};
   MldCfgObject *mldObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UBOOL8 isMld=TRUE;

   rutMulti_getAllProxyIntfNames(isMld,
                               proxyIntfNames, sizeof(proxyIntfNames),
                               sourceIntfNames, sizeof(sourceIntfNames));

   if ((ret = cmsObj_get(MDMOID_MLD_CFG, &iidStack, 0,
                                     (void **) &mldObj)) != CMSRET_SUCCESS)
   {
       cmsLog_error("could not get MDMOID_MLD_CFG, ret=%d", ret);
       return;
   }

   if (IS_EMPTY_STRING(proxyIntfNames))
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(mldObj->mldProxyIfNames);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(mldObj->mldProxyIfNames, proxyIntfNames, mdmLibCtx.allocFlags);
   }

   if (IS_EMPTY_STRING(sourceIntfNames))
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(mldObj->mldMcastIfNames);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(mldObj->mldMcastIfNames, sourceIntfNames, mdmLibCtx.allocFlags);
   }

   if ((ret = cmsObj_set(mldObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("MLD set failed, ret=%d", ret);
   }

   cmsObj_free((void **) &mldObj);

#endif   /* DMP_X_BROADCOM_COM_MLD_1 */

   return;
}


