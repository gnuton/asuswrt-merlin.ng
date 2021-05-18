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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "cms_core.h"
#include "cms_util.h"
#include "cms_dal.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_multicast.h"
#include "qdm_multicast.h"
#include "beep_networking.h"
#include "bcm_mcast_api.h"
#ifdef DMP_X_BROADCOM_COM_DCSP_MCAST_REMARK_1
#include "rdpactl_api.h"
#endif

#ifdef DMP_X_BROADCOM_COM_IGMPSNOOP_1

CmsRet rcl_igmpSnoopingCfgObject( _IgmpSnoopingCfgObject *newObj,
                const _IgmpSnoopingCfgObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   char brIntfName[CMS_IFNAME_LENGTH]={0};
   char *fullPath=NULL;
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   UBOOL8 isAssocBrIntfStatusUp = FALSE;
   UBOOL8 doUpdate = FALSE;
   UBOOL8 mcastHostCtrl = FALSE;
   int mode;

   cmsLog_debug("entered, newObj=%p currObj=%p", newObj, currObj);


   /* create a fullPath to me */
   pathDesc.oid = MDMOID_IGMP_SNOOPING_CFG;
   pathDesc.iidStack = *iidStack;
   ret = cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create fullPath to myself, ret=%d", ret);
      return ret;
   }

   ret = qdmMulti_getAssociatedBridgeModeLocked(fullPath, &mode);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get bridge's mode, ret=%d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      return ret;
   }
   else if (mode != INTFGRP_BR_HOST_MODE)
   {
      cmsLog_debug("igmpsnooping is not supported for BEEP bridge");
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      return ret;
   }

   /* Get the bridge intfname that this snooping obj is associated with */
   {
      CmsRet r2;

      r2 = qdmMulti_getAssociatedBridgeIntfNameLocked(fullPath, brIntfName);
      if (r2 != CMSRET_SUCCESS)
      {
         /*
          * During a delete of an interface group, the entire
          * LanHostConfigManagement.IPInterface subtree has already been
          * deleted prior to this function getting called.  So if I can't
          * get brIntfName, that means this is a delete but don't print
          * error message.
          */
         cmsLog_debug("could not get associatedBridge for fullPath %s "
                      " --delete in progress", fullPath);
         /* keep going, need to update igmpSnoopingIntfList at the end */
      }
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);  // done with it, free it now

   /*
    * Even though the snooping object has an Enable param, it is also
    * dependent on the status of the associated bridge.  If the assoc bridge
    * is not configured and UP, we cannot configure snooping on it.
    */
   if (!IS_EMPTY_STRING(brIntfName))
   {
      isAssocBrIntfStatusUp = cmsNet_isInterfaceUp(brIntfName);
      cmsLog_debug("associated %s isUp = %d", brIntfName, isAssocBrIntfStatusUp);
   }

#ifdef DMP_X_BROADCOM_COM_MCAST_1
   mcastHostCtrl = rutMulti_getHostCtrlConfig();
#endif
       
   /*
    * if we have already configured snooping, but now we are being
    * deleted, disabled, put into host control Mode
    * or assoc bridge is not up, we have to unconfigure.
    */
   if ( currObj && !cmsUtl_strcmp(currObj->status, MDMVS_ENABLED) &&
        (DELETE_OR_DISABLE_EXISTING(newObj, currObj) ||
        (TRUE == mcastHostCtrl) ||
        (FALSE == isAssocBrIntfStatusUp)) )
   {
      if (!IS_EMPTY_STRING(brIntfName))
      {
         rutMulti_configIgmpSnooping(brIntfName, 0, 0);
      }

      if (newObj)
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);
      }

      doUpdate = TRUE;
   } 
   /*
    * The normal rules for transitions of enable false->true or true->false
    * still apply but in order to accommodate a "set" without any parameter
    * changes (done by rutMulti_updateIgmpSnooping), the
    * logic is a little different here:
    * if snoopingObj is enabled, AND assoc bridge is UP, and either
    * (a) we have not already configured snooping, or
    * (b) snooping config has changed
    */
   else if ((newObj && newObj->enable) &&
            (TRUE == isAssocBrIntfStatusUp) &&
            (cmsUtl_strcmp(newObj->status, MDMVS_ENABLED) ||
             rutMulti_isIgmpSnoopingCfgChanged(newObj, currObj)))
   {
      UINT32 mode;
      mode = (cmsUtl_strcmp(newObj->mode, MDMVS_STANDARD) == 0) ? 1 : 2;

      rutMulti_configIgmpSnooping(brIntfName, mode, newObj->lanToLanEnable);

      /* record the fact we configured igmp snooping by setting status */
      CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,
                                  mdmLibCtx.allocFlags);

      doUpdate = TRUE;
   }
   else if (!IS_EMPTY_STRING(brIntfName) && 
            (newObj && (newObj->enable == FALSE)) && 
            (currObj && (currObj->enable == FALSE))) 
   {
       /* Handle the bootup snooping disabled scenario.
          Note 1: During bootup, this function gets invoked twice.
          first time, the bridge interface is not up
          (i.e. isAssocBrIntfStatusUp = FALSE). So, we want
          to specifically take action on the 2nd time when this
          function gets called. This check (currObj &&
          (currObj->enable == FALSE)) ensures that we only
          handle the snooping disabled scenario on the 2nd call
          to this function during bootup.
          Note 2: newObj->status is not set to MDMVS_ENABLED and
          we leave it untouched for this case since snooping is disabled.
          Snooping disabled case is treated like MDMVS_DISABLED */
       rutMulti_configIgmpSnooping(brIntfName, 0, 0);
       doUpdate = TRUE;
   }

   if (doUpdate)
   {
#ifdef DMP_X_BROADCOM_COM_IGMP_1
      /* tell mcpd that igmp snooping bridge intf list has changed */
      rutMulti_updateIgmpSnoopingIntfList();
#endif
   }

   cmsLog_debug("done");

   return CMSRET_SUCCESS;
}

#endif  /* DMP_X_BROADCOM_COM_IGMPSNOOP_1 */


#ifdef DMP_X_BROADCOM_COM_MLDSNOOP_1

CmsRet rcl_mldSnoopingCfgObject( _MldSnoopingCfgObject *newObj,
                const _MldSnoopingCfgObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   char brIntfName[CMS_IFNAME_LENGTH]={0};
   char *fullPath=NULL;
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   UBOOL8 isAssocBrIntfStatusUp = FALSE;
   UBOOL8 doUpdate = FALSE;
   UBOOL8 mcastHostCtrl = FALSE;
   int mode;

   cmsLog_debug("entered, newObj=%p currObj=%p", newObj, currObj);


   /* create a fullPath to me */
   pathDesc.oid = MDMOID_MLD_SNOOPING_CFG;
   pathDesc.iidStack = *iidStack;
   ret = cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create fullPath to myself, ret=%d", ret);
      return ret;
   }

   ret = qdmMulti_getAssociatedBridgeModeLocked(fullPath, &mode);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get bridge's mode, ret=%d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      return ret;
   }
   else if (mode != INTFGRP_BR_HOST_MODE)
   {
      cmsLog_debug("mldsnooping is not supported for BEEP bridge");
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      return ret;
   }

   /* Get the bridge intfname that this snooping obj is associated with */
   {
      CmsRet r2;

      r2 = qdmMulti_getAssociatedBridgeIntfNameLocked(fullPath, brIntfName);
      if (r2 != CMSRET_SUCCESS)
      {
         /*
          * During a delete of an interface group, the entire
          * LanHostConfigManagement.IPInterface subtree has already been
          * deleted prior to this function getting called.  So if I can't
          * get brIntfName, that means this is a delete but don't print
          * error message.
          */
         cmsLog_debug("could not get associatedBridge for fullPath %s "
                      " --delete in progress", fullPath);
         /* keep going, need to update igmpSnoopingIntfList at the end */
      }
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);  // done with it, free it now

   /*
    * Even though the snooping object has an Enable param, it is also
    * dependent on the status of the associated bridge.  If the assoc bridge
    * is not configured and UP, we cannot configure snooping on it.
    */
   if (!IS_EMPTY_STRING(brIntfName))
   {
      isAssocBrIntfStatusUp = cmsNet_isInterfaceUp(brIntfName);
      cmsLog_debug("associated %s isUp = %d", brIntfName, isAssocBrIntfStatusUp);
   }

#ifdef DMP_X_BROADCOM_COM_MCAST_1
   mcastHostCtrl = rutMulti_getHostCtrlConfig();
#endif

   /* see comments above in rcl_igmpSnoopingCfgObject */
   if ( currObj && !cmsUtl_strcmp(currObj->status, MDMVS_ENABLED) &&
        (DELETE_OR_DISABLE_EXISTING(newObj, currObj) ||
        (TRUE == mcastHostCtrl) ||
        (FALSE == isAssocBrIntfStatusUp)) )
   {
      if (!IS_EMPTY_STRING(brIntfName))
      {
         rutMulti_configMldSnooping(brIntfName, 0, 0);
      }

      if (newObj)
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);
      }

      doUpdate = TRUE;
   }
   else if ((newObj && newObj->enable) &&
            (TRUE == isAssocBrIntfStatusUp) &&
            (cmsUtl_strcmp(newObj->status, MDMVS_ENABLED) ||
             rutMulti_isMldSnoopingCfgChanged(newObj, currObj)))
   {
      UINT32 mode;
      mode = (cmsUtl_strcmp(newObj->mode, MDMVS_STANDARD) == 0) ? 1 : 2;

      rutMulti_configMldSnooping(brIntfName, mode, newObj->lanToLanEnable);


      /* record the fact we configured MLD snooping by setting status */
      CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,
                                  mdmLibCtx.allocFlags);

      doUpdate = TRUE;
   }


   if (doUpdate)
   {
#ifdef DMP_X_BROADCOM_COM_MLD_1
      /* tell mcpd that MLD snooping bridge intf list has changed */
      rutMulti_updateMldSnoopingIntfList();
#endif
   }

   cmsLog_debug("done");

   return CMSRET_SUCCESS;
}

#endif  /* DMP_X_BROADCOM_COM_MLDSNOOP_1 */

#ifdef DMP_X_BROADCOM_COM_MCAST_1
CmsRet rcl_mcastCfgObject( _McastCfgObject *newObj,
                const _McastCfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * This object cannot be deleted, so newObj is never NULL.
    */
   cmsLog_debug("Entered: mcastPrecedence=%d ", newObj->mcastPrecedence);

   /* clear or update the multicast priority queue */
   rutMulti_mcastObjCfg(newObj);

#ifdef DMP_X_BROADCOM_COM_GPON_1   
   if ((newObj != NULL) && (currObj != NULL))
   {
       /* doesn't configure host-control when initialization */
       rutMulti_processHostCtrlChange(newObj->mcastHostControl);
   }
#endif

   cmsLog_debug("done");

   return CMSRET_SUCCESS;
}
#endif

#ifdef DMP_X_BROADCOM_COM_IGMP_1 /* aka SUPPORT_IGMP */
CmsRet rcl_igmpCfgObject(_IgmpCfgObject *newObj,
                const _IgmpCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }
   return rutMulti_reloadMcpdWithType(BCM_MCAST_PROTO_IPV4);
} /* rcl_IgmpCfgObject */
#endif /* aka SUPPORT_IGMP */

#ifdef DMP_X_BROADCOM_COM_MLD_1 /* aka SUPPORT_MLD */
CmsRet rcl_mldCfgObject(_MldCfgObject *newObj,
                const _MldCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   return rutMulti_reloadMcpdWithType(BCM_MCAST_PROTO_IPV6);
} /* rcl_mldCfgObject */
#endif /* aka SUPPORT_MLD */
