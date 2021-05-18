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

#ifdef DMP_ADSLWAN_1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_ptm.h"
#include "rut_atm.h"
#include "rut_xtmlinkcfg.h"
#include "rut_dsl.h"
#include "rut_pmirror.h"
#include "rut_qos.h"

#if 1 /* Support Ethernet OAM, Broadcom Yungan, 20130508. */
#include "rut_diag.h"
#endif

#ifdef DMP_ADSLWAN_1

CmsRet rcl_wanDslLinkCfgObject( _WanDslLinkCfgObject *newObj,
                                 const _WanDslLinkCfgObject *currObj,
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 addIntf = FALSE;
   UBOOL8 deleteIntf = FALSE;
   UBOOL8 isRealDel = FALSE;
   char cmdStr[BUFLEN_128];

   cmsLog_debug("Enter");
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }
      
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* Fill ifName if it is NULL (from TR69 or other non httpd apps) */
      if (newObj->X_BROADCOM_COM_IfName == NULL)
      {
         Layer2IfNameType ifNameType = ATM_EOA;
         
         /* get the correct ifNameType */
         if  (cmsUtl_strcmp(newObj->linkType, MDMVS_IPOA) == 0)
         {
            ifNameType = ATM_IPOA;
         }
         else if (cmsUtl_strcmp(newObj->linkType, MDMVS_PPPOA) == 0)
         {
            ifNameType = ATM_PPPOA;
         }
         /* create the layer 2 ifName */
         if ((ret = rutDsl_fillL2IfName(ifNameType, &(newObj->X_BROADCOM_COM_IfName))) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutDsl_fillL2IfName failed. error=%d", ret);
            return ret;
         }    
      }   
      addIntf = TRUE;
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /* remove or disable L2 interface */
      deleteIntf = TRUE;
      if (DELETE_EXISTING(newObj, currObj))
      {
         isRealDel = TRUE;
      }
   }   
   else if (newObj && currObj) 
   {
      cmsLog_debug("newObj->linkStatus=%s, currObj-linkStatus=%s", newObj->linkStatus, currObj->linkStatus);

      /* if old linkStatus is no "Up", and new is "Up", need to add the layer 2 interface */
      if (cmsUtl_strcmp(currObj->linkStatus, MDMVS_UP) && 
         !cmsUtl_strcmp(newObj->linkStatus, MDMVS_UP))
      {
         /* for checking dsl link up and linkStatus is not "UP" case */
         cmsLog_debug("Add layer 2 interface.");
         addIntf = TRUE;
      }
      else if (!cmsUtl_strcmp(currObj->linkStatus, MDMVS_UP) && 
               cmsUtl_strcmp(newObj->linkStatus, MDMVS_UP))
      {
         /* if old linkStatus is "Up", and new is not "Up", need to delete the layer 2 interface */
         cmsLog_debug("Delete layer 2 interface.");
         deleteIntf = TRUE;
      }
   }

   
   if (addIntf)
   {
      if (rutXtm_isXDSLLinkUp(iidStack, MDMOID_WAN_DSL_LINK_CFG) == FALSE)
      {
         cmsLog_debug(" link is not up yet");
         return ret;
      }
      
      if ((ret = rutAtm_ctlVccAdd(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutAtm_ctlVccAdd failed. error=%d", ret);
         return ret;
      }
      if ((ret = rutAtm_createInterface(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutAtm_createInterface failed. error=%d", ret);
         return ret;
      }
      
      snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s up", newObj->X_BROADCOM_COM_IfName);
      rut_doSystemAction("rcl_wanDslLinkCfgObject: ifconfig L2IfName up", (cmdStr));
      CMSMEM_REPLACE_STRING_FLAGS(newObj->linkStatus, MDMVS_UP, mdmLibCtx.allocFlags);

      /* If the interface is used for port mirroring, enable this feature when link is up */
      rutPMirror_enablePortMirrorIfUsed(newObj->X_BROADCOM_COM_IfName);      
      
   }
   else if (deleteIntf && currObj->enable)
   {
      if ((ret = rutQos_qMgmtQueueOperation(currObj->X_BROADCOM_COM_IfName, isRealDel)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtQueueOperation returns error. ret=%d", ret);
      }
      
      /* Only delete L2 interface if it was UP  */
      if (!cmsUtl_strcmp(currObj->linkStatus, MDMVS_UP))
      {
         if ((ret = rutAtm_deleteInterface(currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutAtm_deleteInterface failed. error=%d", ret);
         }
         if ((ret = rutAtm_ctlVccDelete(currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutAtm_ctlVccDelete failed. error=%d", ret);
         }
      }
      
      /* change linkStatus to "DOWN".  if it is not delete  */
      if (!DELETE_EXISTING(newObj, currObj))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->linkStatus, MDMVS_DOWN, mdmLibCtx.allocFlags);
         snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s down 2>/dev/null", newObj->X_BROADCOM_COM_IfName);
         rut_doSystemAction("rcl_wanDslLinkCfgObject: ifconfig L2IfName down", (cmdStr));
      }
   }

   return ret;
}

#endif /* DMP_X_BROADCOM_COM_ATMWAN_1 */


#ifdef DMP_PTMWAN_1
CmsRet rcl_wanPtmLinkCfgObject(_WanPtmLinkCfgObject *newObj,
                const _WanPtmLinkCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   UBOOL8 addIntf = FALSE;
   UBOOL8 deleteIntf = FALSE;
   UBOOL8 isRealDel = FALSE;
   CmsRet ret = CMSRET_SUCCESS;
   char cmdStr[BUFLEN_128];

   cmsLog_debug("Enter");
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* Fill ifName if it is NULL (from TR69 or other non httpd apps) */
      if (newObj->X_BROADCOM_COM_IfName == NULL)
      {
         /* create the layer 2 ifName */
         if ((ret = rutDsl_fillL2IfName(PTM_EOA, &(newObj->X_BROADCOM_COM_IfName))) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutDsl_fillL2IfName failed. error=%d", ret);
            return ret;
         }  
         cmsLog_debug("L2IfName=%s", newObj->X_BROADCOM_COM_IfName);
      }
      
      addIntf = TRUE;
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /* remove or disable L2 interface */
      deleteIntf = TRUE;
      if (DELETE_EXISTING(newObj, currObj))
      {
         isRealDel = TRUE;
      }
   }   
   else if (newObj && currObj) 
   {
      cmsLog_debug(" newObj->linkStatus=%s, currObj-linkStatus=%s", newObj->linkStatus, currObj->linkStatus);
      
      /* if old linkStatus is no "Up", and new is "Up", need to add the layer 2 interface */
      if (cmsUtl_strcmp(currObj->linkStatus, MDMVS_UP) && 
         !cmsUtl_strcmp(newObj->linkStatus, MDMVS_UP))
      {
         /* for checking dsl link up and linkStatus is not "UP" case */
         cmsLog_debug("Add layer 2 interface.");
         addIntf = TRUE;
      }
      else if (!cmsUtl_strcmp(currObj->linkStatus, MDMVS_UP) && 
               cmsUtl_strcmp(newObj->linkStatus, MDMVS_UP))
      {
         /* if old linkStatus is "Up", and new is not "Up", need to delete the layer 2 interface */
         cmsLog_debug("Delete layer 2 interface.");
         deleteIntf = TRUE;
      }
   }
   
   if (addIntf)
   {
      if (rutXtm_isXDSLLinkUp(iidStack, MDMOID_WAN_PTM_LINK_CFG) == FALSE)
      {
         cmsLog_debug(" link is not up yet");
         return ret;
      }   
      if ((ret = rutPtm_setConnCfg(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutPtm_setConnCfg failed. error=%d", ret);
         return ret;
      }
      if ((ret = rutPtm_createInterface(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutPtm_createInterface failed. error=%d", ret);
         return ret;      
      }
      
      snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s up", newObj->X_BROADCOM_COM_IfName);
      rut_doSystemAction("rcl_wanPtmLinkCfgObject: ifconfig L2IfName up", (cmdStr));
      CMSMEM_REPLACE_STRING_FLAGS(newObj->linkStatus, MDMVS_UP, mdmLibCtx.allocFlags);

      /* If the interface is used for port mirroring, enable this feature when link is up */
      rutPMirror_enablePortMirrorIfUsed(newObj->X_BROADCOM_COM_IfName);

      #ifdef DMP_X_BROADCOM_COM_ETHERNETOAM_1
      rutEthOam_set3ahService(TRUE);
      rutEthOam_set1agService(TRUE);
      #endif
    
   }
   else if (deleteIntf && currObj->enable)
   {
      if ((ret = rutQos_qMgmtQueueOperation(currObj->X_BROADCOM_COM_IfName, isRealDel)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_qMgmtQueueOperation returns error. ret=%d", ret);
      }

      #ifdef DMP_X_BROADCOM_COM_ETHERNETOAM_1
      rutEthOam_set3ahService(FALSE);
      rutEthOam_set1agService(FALSE);
      #endif
      
      /* Only delete L2 interface if it was UP  */
      if (!cmsUtl_strcmp(currObj->linkStatus, MDMVS_UP))
      {
         if ((ret = rutPtm_deleteInterface(currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutPtm_deleteInterface failed. error=%d", ret);
         }
         if ((ret = rutPtm_deleteConnCfg(currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutPtm_deleteConnCfg failed. error=%d", ret);
         }
      }
      
      /* change linkStatus to "DOWN".  if it is not delete  */
      if (!DELETE_EXISTING(newObj, currObj))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->linkStatus, MDMVS_DOWN, mdmLibCtx.allocFlags);
         snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s down 2>/dev/null", newObj->X_BROADCOM_COM_IfName);
         rut_doSystemAction("rcl_wanDslLinkCfgObject: ifconfig L2IfName down", (cmdStr));
      }
   }

   return ret;

}
#endif   /* DMP_PTMWAN_1 */


#ifdef DMP_X_BROADCOM_COM_ATMSTATS_1
/* XTM Interface Statistics object replaces ATM Interface Statistics object.
 * This will be removed in the near future.
 */

CmsRet rcl_wanAtmIntfStatsObject( _WanAtmIntfStatsObject *newObj __attribute__((unused)),
                const _WanAtmIntfStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_wanAtmStatsObject( _WanAtmStatsObject *newObj __attribute__((unused)),
                const _WanAtmStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_wanAal5StatsObject( _WanAal5StatsObject *newObj __attribute__((unused)),
                const _WanAal5StatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

CmsRet rcl_wanAal2StatsObject( _WanAal2StatsObject *newObj __attribute__((unused)),
                const _WanAal2StatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}


CmsRet rcl_wanDslAtmParamsObject( _WanDslAtmParamsObject *newObj __attribute__((unused)),
                const _WanDslAtmParamsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_ATMSTATS_1 */ 

#ifdef DMP_X_BROADCOM_COM_XTMSTATS_1
/* XTM Interface Statistics object replaces ATM Interface Statistics object */ 

CmsRet rcl_xtmInterfaceStatsObject( _XtmInterfaceStatsObject *newObj __attribute__((unused)),
                const _XtmInterfaceStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_wanDslAtmParamsObject( _WanDslAtmParamsObject *newObj __attribute__((unused)),
                const _WanDslAtmParamsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

#endif  /* DMP_X_BROADCOM_COM_XTMSTATS_1 */

#endif  /* DMP_ADSLWAN_1 */
