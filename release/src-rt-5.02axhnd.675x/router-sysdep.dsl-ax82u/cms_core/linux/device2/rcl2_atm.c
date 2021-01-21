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

#ifdef DMP_DEVICE2_ATMLINK_1

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
#include "rut_atm.h"
#include "rut2_atm.h"
#include "rut_pmirror.h"
#include "rut_qos.h"
#include "devctl_atm.h"


CmsRet rcl_dev2AtmObject( _Dev2AtmObject *newObj, const _Dev2AtmObject *currObj,
                          const InstanceIdStack *iidStack __attribute__((unused)), 
                          char **errorParam __attribute__((unused)),
                          CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
   }
   return (ret);
}


CmsRet rcl_dev2AtmLinkObject( _Dev2AtmLinkObject *newObj,
                              const _Dev2AtmLinkObject *currObj,
                              const InstanceIdStack *iidStack,
                              char **errorParam __attribute__((unused)),
                              CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 addIntf = FALSE;
   UBOOL8 deleteIntf = FALSE;
   UBOOL8 isRealDel = FALSE;
   char cmdStr[BUFLEN_128];
   char aliasBuf[BUFLEN_64]={0};

   cmsLog_debug("Enter");
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }
      
   /*
    * Update ATM Link NumberOfEntries on successful add or delete.
    */
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumAtmLink(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumAtmLink(iidStack, -1);
   }

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* Fill ifName if it is NULL (from TR69 or other non httpd apps) */
      if (newObj->name == NULL)
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
         if ((ret = rutatm_fillL2IfName_dev2(ifNameType, &(newObj->name))) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutatm_fillL2IfName failed. error=%d", ret);
            return ret;
         } 
         snprintf(aliasBuf, sizeof(aliasBuf), "cpe-%s", newObj->name);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->alias, aliasBuf, mdmLibCtx.allocFlags);
         /* lower layer of ATM VCC is always pointing to dsl ATM-Channel */
         if (newObj->lowerLayers == NULL)
         {
            if ((ret = rutatm_fillLowerLayer(&(newObj->lowerLayers))) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutatm_fillLowerLayer failed. error=%d", ret);
               return ret;
            }  
            cmsLog_debug("LowerLayer=%s", newObj->lowerLayers);
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
      cmsLog_debug("newObj->status=%s, currObj-status=%s", newObj->status, currObj->status);

      /* if old status is no "Up", and new is "Up", need to add the layer 2 interface */
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
      if ((ret = rutAtm_ctlVccAdd_dev2(newObj,iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutAtm_ctlVccAdd failed. error=%d", ret);
         return ret;
      }
      if ((ret = rutAtm_createInterface_dev2(newObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutAtm_createInterface failed. error=%d", ret);
         return ret;
      }
      
      snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s up", newObj->name);
      rut_doSystemAction("rcl_atmLinkCfgObject: ifconfig L2IfName up", (cmdStr));

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
          * This ATM intf is being deleted.  Delete all QoS queues
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
         if ((ret = rutAtm_deleteInterface_dev2(currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutAtm_deleteInterface failed. error=%d", ret);
         }
         if ((ret = rutAtm_ctlVccDelete_dev2(currObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutAtm_ctlVccDelete failed. error=%d", ret);
         }
      }
      
      /* change status to "DOWN".  if it is not delete  */
      if (!DELETE_EXISTING(newObj, currObj))
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
         snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s down 2>/dev/null", newObj->name);
         rut_doSystemAction("rcl_atmLinkObject: ifconfig L2IfName down", (cmdStr));
      }
   }

   return ret;
}


CmsRet rcl_dev2AtmLinkStatsObject( _Dev2AtmLinkStatsObject *newObj __attribute__((unused)),
                                   const _Dev2AtmLinkStatsObject *currObj __attribute__((unused)),
                                   const InstanceIdStack *iidStack __attribute__((unused)),
                                   char **errorParam __attribute__((unused)),
                                   CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2AtmLinkQosObject( _Dev2AtmLinkQosObject *newObj,
                                 const _Dev2AtmLinkQosObject *currObj,
                                 const InstanceIdStack *iidStack,
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS;

   /* add new traffic descriptor entry */
   if (newObj != NULL && currObj == NULL)
   {
      if ((ret = rutAtm_ctlTrffDscrConfig_dev2(newObj,ATM_TDTE_ADD)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutAtm_ctlTrffDscrConfig_dev2 error adding traffic descriptor, ret=%d", ret);
         return ret;
      }
      cmsLog_debug("added new ATM traffic descriptor newObj->qoSClass %s", newObj->qoSClass);
   }
   /* edit existing traffic descriptor entry */
   else if (newObj != NULL && currObj != NULL)
   {
      if ((cmsUtl_strcmp(currObj->qoSClass, newObj->qoSClass)) ||
          (currObj->peakCellRate != newObj->peakCellRate) ||
          (currObj->maximumBurstSize != newObj->maximumBurstSize) ||
          (currObj->sustainableCellRate != newObj->sustainableCellRate) 
          || (currObj->X_BROADCOM_COM_MinimumCellRate != newObj->X_BROADCOM_COM_MinimumCellRate)
          )
      {
         /* need to delete the old entry first? */
         cmsLog_debug("need to change traffic descriptor's parameters");
         
         if ((ret = rutAtm_ctlTrffDscrConfig_dev2(newObj,ATM_TDTE_ADD)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not change the traffic descriptor parameter.");
         }
      }
   }
   /* delete traffic descriptor entry */
   else
   {
      cmsLog_debug("delete traffic descriptor");
      if ((ret = rutAtm_ctlTrffDscrConfig_dev2(currObj,ATM_TDTE_DELETE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Error deleting the traffic descriptor, ret %d.",ret);
      }
   }
   return ret;
}

#ifdef DMP_DEVICE2_ATMLOOPBACK_1
CmsRet rcl_dev2AtmDiagnosticsObject( _Dev2AtmDiagnosticsObject *newObj __attribute__((unused)),
                                     const _Dev2AtmDiagnosticsObject *currObj __attribute__((unused)),
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}

CmsRet rcl_dev2AtmDiagnosticsF5LoopbackObject( _Dev2AtmDiagnosticsF5LoopbackObject *newObj,
                                               const _Dev2AtmDiagnosticsF5LoopbackObject *currObj,
                                               const InstanceIdStack *iidStack,
                                               char **errorParam __attribute__((unused)),
                                               CmsRet *errorCode __attribute__((unused)))
{
   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }
   
   rutAtm_runAtmOamLoopbackTest_dev2(OAM_LB_SEGMENT_TYPE,newObj,currObj,iidStack);
   return CMSRET_SUCCESS;
}

#endif /* DMP_DEVICE2_ATMLOOPBACK_1 */

#ifdef DMP_DEVICE2_X_BROADCOM_COM_ATMLOOPBACK_1
CmsRet rcl_dev2AtmDiagnosticsF4EndToEndLoopbackObject( _Dev2AtmDiagnosticsF4EndToEndLoopbackObject *newObj,
                                                       const _Dev2AtmDiagnosticsF4EndToEndLoopbackObject *currObj,
                                                       const InstanceIdStack *iidStack,
                                                       char **errorParam __attribute__((unused)),
                                                       CmsRet *errorCode __attribute__((unused)))
{
   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }

   rutAtm_runAtmOamLoopbackTest_dev2(OAM_F4_LB_END_TO_END_TYPE,newObj,currObj,iidStack);
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2AtmDiagnosticsF4LoopbackObject( _Dev2AtmDiagnosticsF4LoopbackObject *newObj,
                                               const _Dev2AtmDiagnosticsF4LoopbackObject *currObj,
                                               const InstanceIdStack *iidStack,
                                               char **errorParam __attribute__((unused)),
                                               CmsRet *errorCode __attribute__((unused)))
{
   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }

   rutAtm_runAtmOamLoopbackTest_dev2(OAM_F4_LB_SEGMENT_TYPE,newObj,currObj,iidStack);
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2AtmDiagnosticsF5EndToEndLoopbackObject( _Dev2AtmDiagnosticsF5EndToEndLoopbackObject *newObj,
                                                       const _Dev2AtmDiagnosticsF5EndToEndLoopbackObject *currObj,
                                                       const InstanceIdStack *iidStack,
                                                       char **errorParam __attribute__((unused)),
                                                       CmsRet *errorCode __attribute__((unused)))
{
   /*
    * During startup, when activateObjects calls all the rcl handler functions,
    * this function does not need to do anything.
    * Also during a delete, don't need to do anything.
    */
   if ((ADD_NEW(newObj, currObj)) || (DELETE_EXISTING(newObj, currObj)))
   {
      return CMSRET_SUCCESS;
   }

   rutAtm_runAtmOamLoopbackTest_dev2(OAM_LB_END_TO_END_TYPE,newObj,currObj,iidStack);
   return CMSRET_SUCCESS;
}

#endif /* DMP_DEVICE2_X_BROADCOM_COM_DEVICE2_ATMLOOPBACK_1 */


#endif /* DMP_DEVICE2_ATMLINK_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */
