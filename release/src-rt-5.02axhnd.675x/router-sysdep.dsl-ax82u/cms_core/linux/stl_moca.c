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


#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "rut_moca.h"
#include "rut_qos.h"

/*!\file stl_moca.c
 * \brief This file contains moca WAN and LAN related functions.
 *
 */

#ifdef SUPPORT_MOCA

#ifdef DMP_X_BROADCOM_COM_MOCALAN_1

CmsRet stl_lanMocaIntfObject( _LanMocaIntfObject *obj,
                              const InstanceIdStack *iidStack __attribute__((unused)))
{
   const char *linkStatus=MDMVS_DISABLED;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   if (obj->enable)
   {
      linkStatus = rutMoca_getLinkStatus(obj->ifName);
   }

   if (cmsUtl_strcmp(linkStatus, obj->status))
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, linkStatus, mdmLibCtx.allocFlags);
      /* tell upper layers the value has changed */
      ret = CMSRET_SUCCESS;
   }

   return ret;
}


CmsRet stl_lanMocaIntfStatusObject( _LanMocaIntfStatusObject *obj,
                                   const InstanceIdStack *iidStack)
{
   LanMocaIntfObject *parentObj=NULL;
   InstanceIdStack parentIidStack = *iidStack;
   CmsRet ret;

   ret = cmsObj_getAncestor(MDMOID_LAN_MOCA_INTF, MDMOID_LAN_MOCA_INTF_STATUS, &parentIidStack, (void **) &parentObj);
   if (ret != CMSRET_SUCCESS)
   {
      /* very unlikely */
      return ret;
   }

   if (parentObj->enable)
   {
      ret = rutMoca_getStatus(parentObj->ifName, obj);
   }
   else
   {
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   cmsObj_free((void **) &parentObj);

   return ret;
}


CmsRet stl_lanMocaIntfStatsObject( _LanMocaIntfStatsObject *obj,
                                   const InstanceIdStack *iidStack)
{
   LanMocaIntfObject *parentObj=NULL;
   InstanceIdStack parentIidStack = *iidStack;
   CmsRet ret;
   ret = cmsObj_getAncestor(MDMOID_LAN_MOCA_INTF, MDMOID_LAN_MOCA_INTF_STATS, &parentIidStack, (void **) &parentObj);
   if (ret != CMSRET_SUCCESS)
   {
      /* very unlikely */
      return ret;
   }

   if (parentObj->enable)
   {
      if (obj == NULL)
      {
         ret = rutMoca_resetStats(parentObj->ifName);
      }
      else
      {
         ret = rutMoca_getStats(parentObj->ifName, obj);
      }
   }
   else
   {
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   cmsObj_free((void **) &parentObj);

   return ret;
}

#endif /* DMP_X_BROADCOM_COM_MOCALAN_1 */


#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1

CmsRet stl_wanMocaIntfObject( _WanMocaIntfObject *obj,
                              const InstanceIdStack *iidStack __attribute__((unused)))
{
   const char *linkStatus=MDMVS_DISABLED;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   cmsLog_debug("Entered: %s currStatus=%s", obj->ifName, obj->status);

   if (cmsUtl_strlen(obj->ifName) == 0)
   {
      /* NULL or empty IntfName, can't do anything yet. */
      return ret;
   }

   if (obj->enable)
   {
      linkStatus = rutMoca_getLinkStatus(obj->ifName);
   }

   if (cmsUtl_strcmp(linkStatus, obj->status))
   {
      CMSMEM_REPLACE_STRING_FLAGS(obj->status, linkStatus, mdmLibCtx.allocFlags);
      /* tell upper layers the value has changed */
      ret = CMSRET_SUCCESS;

      if (!strcmp(obj->status, MDMVS_UP))
      {
         CmsRet r2;
         if ((r2 = rutQos_tmPortInit(obj->ifName, TRUE)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_tmPortInit() returns error. ret=%d, %s", r2, obj->ifName);
         }
      }
      else
      {
         CmsRet r2;
         if ((r2 = rutQos_tmPortUninit(obj->ifName, TRUE)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_tmPortUninit() returns error. ret=%d, %s", r2, obj->ifName);
         }
      }
    }

   return ret;
}

CmsRet stl_wanMocaIntfStatusObject( _WanMocaIntfStatusObject *obj,
                                   const InstanceIdStack *iidStack __attribute__((unused)))
{
   LanMocaIntfObject *parentObj=NULL;
   InstanceIdStack parentIidStack = *iidStack;
   CmsRet ret;

   ret = cmsObj_getAncestor(MDMOID_WAN_MOCA_INTF, MDMOID_WAN_MOCA_INTF_STATUS, &parentIidStack, (void **) &parentObj);
   
   if (ret != CMSRET_SUCCESS)
   {
      /* very unlikely */
      return ret;
   }

   if (parentObj->enable)
   {
      /*
       * The LanMocaIntfStatus object and the WanMocaIntfStatus object
       * have the exact same fields, so doing this cast is OK.
       */
      ret = rutMoca_getStatus(parentObj->ifName, (LanMocaIntfStatusObject *) obj);
   }
   else
   {
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   cmsObj_free((void **) &parentObj);

   return ret;
}


CmsRet stl_wanMocaIntfStatsObject( _WanMocaIntfStatsObject *obj,
                                   const InstanceIdStack *iidStack __attribute__((unused)))
{
   WanMocaIntfObject *parentObj=NULL;
   InstanceIdStack parentIidStack = *iidStack;
   CmsRet ret;

   ret = cmsObj_getAncestor(MDMOID_WAN_MOCA_INTF, MDMOID_WAN_MOCA_INTF_STATS, &parentIidStack, (void **) &parentObj);
   if (ret != CMSRET_SUCCESS)
   {
      /* very unlikely */
      return ret;
   }

   if (parentObj->enable)
   {
      if (obj == NULL)
      {
         ret = rutMoca_resetStats(parentObj->ifName);
      }
      else
      {
         /*
          * The LanMocaIntfStats object and the WanMocaIntfStats object
          * have the exact same fields, so doing this cast is OK.
          */
         ret = rutMoca_getStats(parentObj->ifName, (LanMocaIntfStatsObject *) obj);
      }
   }
   else
   {
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   cmsObj_free((void **) &parentObj);

   return ret;
}

#endif  /* DMP_X_BROADCOM_COM_MOCAWAN_1 */



#endif /* SUPPORT_MOCA */
