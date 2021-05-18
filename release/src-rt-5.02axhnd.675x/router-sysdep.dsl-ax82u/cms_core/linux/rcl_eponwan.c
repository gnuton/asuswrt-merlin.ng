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
#include "rut_lan.h"
#include "rut_ethintf.h"
#include "rut_qos.h"
#include "rut_tmctl_wrap.h"
#include "rut_eponwan.h"
#ifdef SUPPORT_RDPA
#include "rdpa_types.h"
#include "rdpactl_api.h"
#endif


/*!\file rcl_eponwan.c
 * \brief This file contains epon WAN related functions.
 *
 */

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
#ifdef DMP_BASELINE_1

/* Init WAN queue upon creation or link up. */
CmsRet rcl_wanEponIntfObject( _WanEponIntfObject *newObj,
                const _WanEponIntfObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{


   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* if we are enabling ethernet as wan, then we can upgrade
       * the link status to NOLINK. */
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_NOLINK, mdmLibCtx.allocFlags);
      if (newObj->ifName)
      {
#ifndef DESKTOP_LINUX
         /* enable wan port setting of switch */
         //rutEth_setSwitchWanPort(newObj->ifName, TRUE);
         rutLan_enableInterface(newObj->ifName);
#endif
      }
      else
      {
         cmsLog_error("enabled, but ifName not set yet");
      }
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if (newObj)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
      }

#ifndef DESKTOP_LINUX
      /* disable wan port setting of switch */
      //rutEth_setSwitchWanPort(currObj->ifName, FALSE);
      rutLan_disableInterface(currObj->ifName);
#endif
   }

   return CMSRET_SUCCESS;
}


CmsRet stl_wanEponIntfObject(_WanEponIntfObject *obj, const InstanceIdStack *iidStack __attribute__((unused)))
{
   char currentStatus[BUFLEN_16];
   char hwAddr[BUFLEN_32];
   CmsRet ret=CMSRET_SUCCESS_OBJECT_UNCHANGED;

   cmsLog_debug("obj->ifName=%s", obj->ifName);
   /* TODO: We should avoid to use ifname */
   /* we enable it after we set wan connection.
   * Check ifname for layer2 iface be created */
   /* if (obj->enable) */
   if (obj->ifName)
   {
      /* check for status change */
      rut_getIfStatusHwaddr(obj->ifName, currentStatus, hwAddr);

      cmsLog_debug("currentStatus=%s, hwAddr=%s", currentStatus, hwAddr);
      /* 6838&6828 EPON link status should be determined by eponapp who know link,los,loid authentication status */
      // when eponapp message is ready, del line 135.137 and comments line 136
      //CMSMEM_REPLACE_STRING_FLAGS(obj->status, currentStatus, mdmLibCtx.allocFlags);
      ret = CMSRET_SUCCESS; /* let mdm know we changed the string */

      if (cmsUtl_strcmp(obj->MACAddress, hwAddr))
      {
         CMSMEM_REPLACE_STRING_FLAGS(obj->MACAddress, hwAddr, mdmLibCtx.allocFlags);
         ret = CMSRET_SUCCESS; /* let mdm know we changed the string */
      }
   }

   return ret;
}


CmsRet rcl_wanEponIntfStatsObject( _WanEponIntfStatsObject *newObj __attribute__((unused)),
                const _WanEponIntfStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* this is a stats object, so probably nothing to do in the RCL handler function. */
   return CMSRET_SUCCESS;
}


CmsRet stl_wanEponIntfStatsObject(_WanEponIntfStatsObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_wanEponLinkCfgObject( _WanEponLinkCfgObject *newObj __attribute__((unused)),
                const _WanEponLinkCfgObject *currObj __attribute__((unused)),
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


CmsRet rcl_wanEponLinkCfgObject( _WanEponLinkCfgObject *newObj,
                const _WanEponLinkCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 portInit = FALSE;
   UBOOL8 portUninit = FALSE;
#ifdef SUPPORT_RDPA
   XponObject *obj=NULL;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;

   if (cmsObj_get(MDMOID_XPON, &iidStack2, 0, (void **) &obj) != CMSRET_SUCCESS)
   {
      cmsLog_error("get of MDMOID_IGD_DEVICE_INFO object failed, ret=%d", ret);
   }
   else
   {
      if (obj->oamSelection & OAM_DPOE_SUPPORT)
        rdpaCtl_set_epon_mode(rdpa_epon_dpoe);
      else if (obj->oamSelection & OAM_BCM_SUPPORT)
        rdpaCtl_set_epon_mode(rdpa_epon_bcm);
      else if (obj->oamSelection & OAM_CUC_SUPPORT)
#ifdef EPON_HGU
        rdpaCtl_set_epon_mode(rdpa_epon_cuc_dyn);
#else
        rdpaCtl_set_epon_mode(rdpa_epon_cuc);
#endif
      else
      {
#ifdef EPON_HGU
        rdpaCtl_set_epon_mode(rdpa_epon_ctc_dyn);
#else
        rdpaCtl_set_epon_mode(rdpa_epon_ctc);
#endif
      }
   }

   cmsObj_free((void**)&obj);
#endif /* SUPPORT_RDPA */

   if (!rut_isWanTypeEpon())
   {
       cmsLog_debug("Skip WAN EPON configuration");
       return ret;
   }

#define SET_EXISTING(n, c) (((n) != NULL && (c) != NULL)) 

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

   if (portInit == TRUE)
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

#endif /* DMP_BASELINE_1 */
#endif  /* DMP_X_BROADCOM_COM_EPONWAN_1 */
