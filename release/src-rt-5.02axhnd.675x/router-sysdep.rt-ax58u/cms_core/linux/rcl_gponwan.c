/***********************************************************************
 *
 *  Copyright (c) 2010  Broadcom Corporation
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
#include "rut_gponwan.h"
#include "rut_qos.h"
#include "rut_tmctl_wrap.h"

/*!\file rcl_gponwan.c
 * \brief This file contains gpon WAN related functions.
 *
 */

#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
#ifdef DMP_BASELINE_1

/* Init WAN queue upon creation or link up. */
/* #ifdef INIT_WAN_QUEUE_ON_LINKUP */

#define SET_EXISTING(n, c) \
   (((n) != NULL && (c) != NULL))


CmsRet rcl_wanGponLinkCfgObject( _WanGponLinkCfgObject *newObj,
                const _WanGponLinkCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))

{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 portInit = FALSE;
   UBOOL8 portUninit = FALSE;

#ifdef INIT_WAN_QUEUE_ON_LINKUP
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* Link up */
      if (!strcmp(newObj->linkStatus, MDMVS_UP))
      {
         portInit = TRUE;
      }
   }
   else if (SET_EXISTING(newObj, currObj) && newObj->enable)
   {
      /* Link up */
      if (!strcmp(newObj->linkStatus, MDMVS_UP) &&
        strcmp(currObj->linkStatus, MDMVS_UP))
      {
         portInit = TRUE;
      }
#ifdef REMOVE_WAN_QUEUE_ON_LINKDOWN
      /* Link down */
      else if (strcmp(newObj->linkStatus, MDMVS_UP) &&
        !strcmp(currObj->linkStatus, MDMVS_UP))
      {
         portUninit = TRUE;
      }
#endif /* REMOVE_WAN_QUEUE_ON_LINKDOWN */
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
   	  if (!strcmp(currObj->linkStatus, MDMVS_UP))
      {
		portUninit = TRUE;
   	  }
   }
#else /* INIT_WAN_QUEUE_ON_LINKUP */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      portInit = (newObj->phyDetected == TRUE) ? TRUE : FALSE;
   }
   else if (SET_EXISTING(newObj, currObj) && newObj->enable)
   {
      portInit = ((newObj->phyDetected == TRUE) && 
        (currObj->phyDetected == FALSE)) ? TRUE : FALSE;
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      portUninit = (currObj->phyDetected == TRUE) ? TRUE : FALSE;
   }
#endif /* INIT_WAN_QUEUE_ON_LINKUP */

   if ((SET_EXISTING(newObj, currObj)) && 
     ((newObj->phyDetected == TRUE) && 
     (currObj->phyDetected == FALSE)))
   {
      if (rut_tmctl_getQueueOwner() == TMCTL_OWNER_BH)
      {
         rdpaCtl_set_sys_car_mode(FALSE);
      }
      else
      {
         rdpaCtl_set_sys_car_mode(TRUE);
      }
   }

   if ((portInit == TRUE) && (portUninit == TRUE))
   {
      cmsLog_error("Invalid flags, portInit=%d, portUninit=%d",
        portInit, portUninit);
   }

   if (portUninit == TRUE)
   {
      if ((ret = rutQos_tmPortUninit(currObj->ifName, TRUE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_tmPortUninit() failed. if=%s, ret=%d",
           currObj->ifName, ret);
      }
   }
#if defined(REMOVE_WAN_QUEUE_ON_LINKDOWN) || !defined(INIT_WAN_QUEUE_ON_LINKUP)
   if (portInit == TRUE)
#else
   if ((portInit == TRUE) && (getPonLinkCfgFlag(rut_getRdpaWanType(rdpa_wan_type_to_if(rdpa_wan_gpon))) == FALSE))
#endif /* REMOVE_WAN_QUEUE_ON_LINKDOWN || !INIT_WAN_QUEUE_ON_LINKUP */
   {
      if ((ret = rutQos_tmPortUninit(newObj->ifName, TRUE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_tmPortUninit() failed. if=%s, ret=%d",
           newObj->ifName, ret);
      }
      if ((ret = rutQos_tmPortInit(newObj->ifName, TRUE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_tmPortInit() failed. if=%s, ret=%d",
           newObj->ifName, ret);
      }
      else
      {
          rutQos_qMgmtQueueReconfig(newObj->ifName, TRUE);
          cmsLog_debug("rutQos_qMgmtQueueReconfig() if=%s\n",
            newObj->ifName);
      }
   }

   return ret;
}


CmsRet stl_wanGponLinkCfgObject( _WanGponLinkCfgObject *newObj __attribute__((unused)),
                const _WanGponLinkCfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))

{
   /*
    * Doesn't make much sense to "set" a status object, so probably no
    * code needed in this function.
    */
   return CMSRET_SUCCESS;
}


#endif  /* DMP_BASELINE_1 */
#endif  /* DMP_X_BROADCOM_COM_GPONWAN_1 */
