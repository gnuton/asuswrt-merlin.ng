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
#include "rut_lan.h"
#include "rut_moca.h"
#include "rut_ethintf.h"

#include "rut_qos.h"

/*!\file rcl_moca.c
 * \brief This file contains moca WAN and LAN related functions.
 *
 */

#ifdef SUPPORT_MOCA

#ifdef DMP_X_BROADCOM_COM_MOCALAN_1

CmsRet rcl_lanMocaIntfObject( _LanMocaIntfObject *newObj,
                const _LanMocaIntfObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))

{
   char bridgeIfName[CMS_IFNAME_LENGTH]={0};
   UBOOL8 isWanIntf = FALSE;
   CmsRet ret=CMSRET_SUCCESS;

   
   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   if (ADD_NEW(newObj, currObj))
   {
      /*
       * add new object instance usually will succeed, so just update
       * the count at the beginning of the function.
       */
       rut_modifyNumMocaIntf(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      /*
       * This is a delete instance, update count.  Deletes always update the MDM,
       * even if the RCL handler function could not do the action.
       */
      rut_modifyNumMocaIntf(iidStack, -1);
   }

   /* enable moca device */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
       
      if ((ret = rutMoca_initialize(newObj->ifName,
                         newObj->lastOperationalFrequency)) != CMSRET_SUCCESS)
      {
         cmsLog_error("moca initialization failed");
         return ret;
      }

      if ((ret = rutMoca_setTrace(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("moca set trace failed");
         return ret;
      }

      ret = rutMoca_start(newObj->ifName,
                          &newObj->autoNwSearch, &newObj->privacy,
                          &newObj->lastOperationalFrequency,
                          &newObj->password, &newObj->initParmsString);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("moca start failed");
         return ret;
      }

      if ((ret = rutLan_getParentBridgeIfName(MDMOID_LAN_MOCA_INTF, iidStack, bridgeIfName)) == CMSRET_SUCCESS)
      {
         rutQos_tmPortInit(newObj->ifName, FALSE);      
         rutLan_enableInterface(newObj->ifName);
         rutLan_addInterfaceToBridge(newObj->ifName, isWanIntf, bridgeIfName);
#if defined(DMP_X_BROADCOM_COM_MOCAWAN_1)
         /* make sure interface is configured as LAN */
         rutEth_setSwitchWanPort(newObj->ifName, FALSE);
#endif
      }
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {

      /* check for update trace parameters command */
      if (cmsUtl_strcmp(newObj->mocaControl, MDMVS_UPDATETRACE) == 0)
      {
         ret = rutMoca_updateTraceParms(NULL, (LanMocaIntfObject *) newObj);
         /* This field should always read NONE */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->mocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
      }
      /* check for change in trace params, if changed */
      else if (cmsUtl_strcmp(newObj->traceParmsString, currObj->traceParmsString))
      {
         ret = rutMoca_setTrace(newObj);
      }

      /* check for update configuration parameters command */
      if (cmsUtl_strcmp(newObj->mocaControl, MDMVS_UPDATECONFIG) == 0)
      {
         ret = rutMoca_updateConfigParms(NULL, (LanMocaIntfObject *) newObj);
         /* This field should always read NONE */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->mocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
      }
      else if (cmsUtl_strcmp(newObj->configParmsString, currObj->configParmsString))
      {
         /* check for change in config params, if changed */
         ret = rutMoca_setParams((LanMocaIntfObject *) newObj, (LanMocaIntfObject *) currObj);
      }

      /* check for start command */
      if (cmsUtl_strcmp(newObj->mocaControl, MDMVS_START) == 0)
      {
         ret = rutMoca_start(newObj->ifName,
                             &newObj->autoNwSearch, &newObj->privacy,
                             &newObj->lastOperationalFrequency,
                             &newObj->password, &newObj->initParmsString);
         /* This field should always read NONE */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->mocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
      }

      /* check for stop command */
      if (cmsUtl_strcmp(newObj->mocaControl, MDMVS_STOP) == 0)
      {
         ret = rutMoca_stop(newObj->ifName);
         /* This field should always read NONE */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->mocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
      }

      /* check for restart command */
      if (cmsUtl_strcmp(newObj->mocaControl, MDMVS_RESTART) == 0)
      {
         ret = rutMoca_reinitialize(newObj->ifName,
                              &newObj->autoNwSearch, &newObj->privacy,
                              &newObj->lastOperationalFrequency,
                              &newObj->password, &newObj->initParmsString);
         /* This field should always read NONE */
        CMSMEM_REPLACE_STRING_FLAGS(newObj->mocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
      }

      /* check for update initialization parameters command */
      if (cmsUtl_strcmp(newObj->mocaControl, MDMVS_UPDATEINIT) == 0)
      {
         ret = rutMoca_getInitParms(NULL, newObj->ifName,
                              &newObj->autoNwSearch, &newObj->privacy,
                              &newObj->lastOperationalFrequency,
                              &newObj->password, &newObj->initParmsString);
         /* This field should always read NONE */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->mocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
      }

   }

   /* disable moca device */
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {

       if ((ret = rutLan_getParentBridgeIfName(MDMOID_LAN_MOCA_INTF, iidStack, bridgeIfName)) == CMSRET_SUCCESS)
      {
         rutLan_disableInterface(currObj->ifName);
         rutLan_removeInterfaceFromBridge(currObj->ifName, bridgeIfName);
      }
   }

   return ret;
}


CmsRet rcl_lanMocaIntfStatusObject( _LanMocaIntfStatusObject *newObj __attribute__((unused)),
                const _LanMocaIntfStatusObject *currObj __attribute__((unused)),
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


CmsRet rcl_lanMocaIntfStatsObject( _LanMocaIntfStatsObject *newObj __attribute__((unused)),
                const _LanMocaIntfStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))

{
   /*
    * Doesn't make much sense to "set" a stats object, so probably no
    * code needed in this function.
    */
   return CMSRET_SUCCESS;
}


#endif /* DMP_X_BROADCOM_COM_MOCALAN_1 */


#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1

CmsRet rcl_wanMocaIntfObject( _WanMocaIntfObject *newObj,
                const _WanMocaIntfObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))

{
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   /* enable moca device */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      if ((ret = rutMoca_initialize(newObj->ifName,
                        newObj->lastOperationalFrequency)) != CMSRET_SUCCESS)
      {
         cmsLog_error("moca initialzation failed, ret=%d", ret);
         return ret;
      }

      if ((ret = rutMoca_setTrace((LanMocaIntfObject *)newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("moca set trace failed");
         return ret;
      }

      rutEth_setSwitchWanPort(newObj->ifName, TRUE);
      rutLan_enableInterface(newObj->ifName);
      rutMoca_stop(newObj->ifName);  // why stop first?  is this to handle move from LAN to WAN?
      rutMoca_start(newObj->ifName,
                    &newObj->autoNwSearch, &newObj->privacy,
                    &newObj->lastOperationalFrequency,
                    &newObj->password, &newObj->initParmsString);
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      /* Send connection update msg to ssk only if the device link is up and auto detect flag changed */       
      if (!cmsUtl_strcmp(newObj->status, MDMVS_UP) &&
         newObj->limitedConnections != currObj->limitedConnections)
      {
         UBOOL8 isAutoDetectChange = TRUE;
         
         cmsLog_debug("Link status=%s, newObj->limitedConnections=%d, currobj->limitedConnections=%d",
            newObj->status, newObj->limitedConnections, currObj->limitedConnections); 
         
         /* Need to let ssk know the change on the auto detect flag */
         rutWan_sendConnectionUpdateMsg(MDMOID_WAN_MOCA_INTF, iidStack, FALSE, FALSE, FALSE, isAutoDetectChange);
      }
      

      /* check for update trace parameters command */
      if (cmsUtl_strcmp(newObj->mocaControl, MDMVS_UPDATETRACE) == 0)
      {
         ret = rutMoca_updateTraceParms(NULL, (LanMocaIntfObject *) newObj);
         /* This field should always read NONE */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->mocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
      }
      /* check for change in trace params, if changed */
      else if (cmsUtl_strcmp(newObj->traceParmsString, currObj->traceParmsString))
      {
         ret = rutMoca_setTrace((LanMocaIntfObject *)newObj);
      }

      /* check for update configuration parameters command */
      if (cmsUtl_strcmp(newObj->mocaControl, MDMVS_UPDATECONFIG) == 0)
      {
         ret = rutMoca_updateConfigParms(NULL, (LanMocaIntfObject *) newObj);
         /* This field should always read NONE */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->mocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
      }
      else if (cmsUtl_strcmp(newObj->configParmsString, currObj->configParmsString))
      {
         /* check for change in config params, if changed */
         ret = rutMoca_setParams((LanMocaIntfObject *) newObj, (LanMocaIntfObject *) currObj);
      }

      /* check for start command */
      if (cmsUtl_strcmp(newObj->mocaControl, MDMVS_START) == 0)
      {
         ret = rutMoca_start(newObj->ifName,
                             &newObj->autoNwSearch, &newObj->privacy,
                             &newObj->lastOperationalFrequency,
                             &newObj->password, &newObj->initParmsString);
         /* This field should always read NONE */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->mocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
      }

      /* check for stop command */
      if (cmsUtl_strcmp(newObj->mocaControl, MDMVS_STOP) == 0)
      {
         ret = rutMoca_stop(newObj->ifName);
         /* This field should always read NONE */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->mocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
      }

      /* check for restart command */
      if (cmsUtl_strcmp(newObj->mocaControl, MDMVS_RESTART) == 0)
      {
         ret = rutMoca_reinitialize(newObj->ifName,
                               &newObj->autoNwSearch, &newObj->privacy,
                               &newObj->lastOperationalFrequency,
                               &newObj->password, &newObj->initParmsString);
         /* This field should always read NONE */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->mocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
      }

      /* check for update initialization parameters command */
      if (cmsUtl_strcmp(newObj->mocaControl, MDMVS_UPDATEINIT) == 0)
      {
         ret = rutMoca_getInitParms(NULL, newObj->ifName,
                              &newObj->autoNwSearch, &newObj->privacy,
                              &newObj->lastOperationalFrequency,
                              &newObj->password, &newObj->initParmsString);
         /* This field should always read NONE */
         CMSMEM_REPLACE_STRING_FLAGS(newObj->mocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
      }
   }

   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      rutEth_setSwitchWanPort(currObj->ifName, FALSE);
      rutLan_disableInterface(currObj->ifName);
      rutMoca_stop(currObj->ifName);
      /*
       * Regardless of whether this is a disable or delete, we should
       * not call rutMoca_start here.  If this is a move from WAN to LAN,
       * the Moca interface will be started again on the LAN side.
       */
   }

   return ret;
}


CmsRet rcl_wanMocaIntfStatusObject( _WanMocaIntfStatusObject *newObj __attribute__((unused)),
                const _WanMocaIntfStatusObject *currObj __attribute__((unused)),
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


CmsRet rcl_wanMocaIntfStatsObject( _WanMocaIntfStatsObject *newObj __attribute__((unused)),
                const _WanMocaIntfStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))

{
   /*
    * Doesn't make much sense to "set" a stats object, so probably no
    * code needed in this function.
    */
   return CMSRET_SUCCESS;
}


#endif  /* DMP_X_BROADCOM_COM_MOCAWAN_1 */


#endif /* SUPPORT_MOCA */
