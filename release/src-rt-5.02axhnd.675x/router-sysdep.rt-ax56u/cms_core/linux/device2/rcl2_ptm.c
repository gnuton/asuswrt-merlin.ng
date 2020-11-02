/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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

#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_PTMLINK_1

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
#include "cms_qdm.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_ptm.h"
#include "rut_atm.h"
#include "rut2_ptm.h"
#include "rut_pmirror.h"
#include "rut_qos.h"

CmsRet rcl_dev2PtmObject( _Dev2PtmObject *newObj __attribute__((unused)),
                      const _Dev2PtmObject *currObj __attribute__((unused)),
                      const InstanceIdStack *iidStack __attribute__((unused)),
                      char **errorParam __attribute__((unused)),
                      CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2PtmLinkObject( _Dev2PtmLinkObject *newObj,
                          const _Dev2PtmLinkObject *currObj,
                          const InstanceIdStack *iidStack,
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

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumPtmLink(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumPtmLink(iidStack, -1);
   }

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* Fill ifName if it is NULL (from TR69 or other non httpd apps) */
      if (newObj->name == NULL)
      {
         /* create the layer 2 ifName */
         if ((ret = rutptm_fillL2IfName_dev2(PTM_EOA, &(newObj->name))) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutDsl_fillL2IfName failed. error=%d", ret);
            return ret;
         }  
         cmsLog_debug("L2IfName=%s", newObj->name);
      }
      if (newObj->lowerLayers == NULL)
      {
         if ((ret = rutptm_fillLowerLayer(&(newObj->lowerLayers))) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutDsl_fillLowerLayer failed. error=%d", ret);
            return ret;
         }  
         cmsLog_debug("LowerLayer=%s", newObj->lowerLayers);
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
      cmsLog_debug(" newObj->status=%s, currObj-status=%s", newObj->status, currObj->status);
      
      /* if old status is not "Up", and new is "Up", need to add the layer 2 interface */
      if (cmsUtl_strcmp(currObj->status, MDMVS_UP) && 
         !cmsUtl_strcmp(newObj->status, MDMVS_UP))
      {
         /* for checking dsl link up and status is not "UP" case */
         cmsLog_debug("Add layer 2 interface.");
         addIntf = TRUE;
      }
      else if (!cmsUtl_strcmp(currObj->status, MDMVS_UP) && 
               cmsUtl_strcmp(newObj->status, MDMVS_UP))
      {
         /* if old status is "Up", and new is not "Up", need to delete the layer 2 interface */
         cmsLog_debug("Delete layer 2 interface.");
         deleteIntf = TRUE;
      }
   }
   
   if (addIntf)
   {
      /* is the channel up ? */
      if (qdmDsl_isAnyLowerLayerChannelUpLocked_dev2(newObj->lowerLayers) == FALSE)
      {
         cmsLog_debug(" link is not up yet, so just return");
         return ret;
      }
      if ((ret = rutptm_setConnCfg_dev2(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutptm_setConnCfg failed. error=%d", ret);
         return ret;
      }
      if ((ret = rutptm_createInterface_dev2(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutptm_createInterface failed. error=%d", ret);
         return ret;      
      }
      
      snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s up", newObj->name);
      rut_doSystemAction("rcl_wanPtmLinkCfgObject: ifconfig L2IfName up", (cmdStr));

      /* If the interface is used for port mirroring, enable this feature when link is up */
      rutPMirror_enablePortMirrorIfUsed(newObj->name);

      /* Activate queues on this L2 intf */
      rutQos_reconfigAllQueuesOnLayer2Intf_dev2(newObj->name);
   }
   else if (deleteIntf && currObj->enable)
   {
      if (isRealDel)
      {
         /*
          * This PTM intf is being deleted.  Delete all QoS queues
          * defined for this interface.  When the QoS queue is deleted,
          * rcl_dev2QosQueueObject will delete all associated classifiers.
          */
         rutQos_deleteQueues_dev2(currObj->name);
      }
      else
      {
         /* link is down, unconfig queues on this L2 intf */
         rutQos_reconfigAllQueuesOnLayer2Intf_dev2(currObj->name);
      }

      /* Only delete L2 interface if it was UP  */
      if (!cmsUtl_strcmp(currObj->status, MDMVS_UP))
      {
         if ((ret = rutptm_deleteInterface_dev2(currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutptm_deleteInterface failed. error=%d", ret);
         }
         if ((ret = rutptm_deleteConnCfg_dev2(currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutptm_deleteConnCfg failed. error=%d", ret);
         }
      }
      
      /* change status to "DOWN".  if it is not delete  */
      if (!DELETE_EXISTING(newObj, currObj))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
         snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s down 2>/dev/null", newObj->name);
         rut_doSystemAction("rcl_ptmLinkObject: ifconfig L2IfName down", (cmdStr));
      }
   }

   return ret;

}

CmsRet rcl_dev2PtmLinkStatsObject( _Dev2PtmLinkStatsObject *newObj __attribute__((unused)),
                               const _Dev2PtmLinkStatsObject *currObj __attribute__((unused)),
                               const InstanceIdStack *iidStack __attribute__((unused)),
                               char **errorParam __attribute__((unused)),
                               CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

#endif   /* DMP_DEVICE2_PTMLINK_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */
