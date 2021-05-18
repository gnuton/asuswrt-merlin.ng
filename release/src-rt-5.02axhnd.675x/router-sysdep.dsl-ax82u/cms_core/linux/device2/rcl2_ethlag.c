/***********************************************************************
 *
 *  Copyright (c) 2006-2018  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2016:proprietary:standard

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

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_iptables.h"
#include "rut2_bridging.h"
#include "rut2_ethlag.h"
#include "rut_lan.h"

/*!\file rcl2_ethlag.c
 * \brief This file contains Ethernet LAG objects related functions.
 *
 */

#ifdef DMP_DEVICE2_ETHLAG_1


CmsRet rcl_dev2EthLAGObject(_Dev2EthLAGObject *newObj __attribute__((unused)),
                            const _Dev2EthLAGObject *currObj __attribute__((unused)),
                            const InstanceIdStack *iidStack __attribute__((unused)), 
                            char **errorParam __attribute__((unused)),
                            CmsRet *errorCode __attribute__((unused)))
{

   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 add = TRUE;
   
   cmsLog_debug("Enter");

   if (newObj)
   {
      cmsLog_debug("Entered: newObj->name=%s enable=%d currObj=%p iidStack=%s",
                    newObj->name, newObj->enable, currObj,
                    cmsMdm_dumpIidStack(iidStack));
   }
   else if (currObj)
   {
      cmsLog_debug("Entered: delete currObj->name=%s iidStack=%s",
                    currObj->name, cmsMdm_dumpIidStack(iidStack));
   }

   if (ADD_NEW(newObj, currObj))
   {
   
      cmsLog_debug("ADD_NEW");;
      insertEthBondingModule();
      /*todo when hybrid data model -- rutUtil_modifyNumEthLag(iidStack, 1); */
      rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_ETHERNET,  MDMOID_LA_G, iidStack, 1);
   }

   IF_STATUS_HAS_CHANGED_SET_LASTCHANGE(newObj, currObj);

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      
      cmsLog_debug("ENABLE_NEW_OR_ENABLE_EXISTING newObj->name %s", newObj->name);
   
      if (IS_EMPTY_STRING(newObj->name))
      {        
        /*Need to fill bond interface name from bonding driver 
         */
         char bondIntfName[CMS_IFNAME_LENGTH]={0};
         UBOOL8 layer2 = TRUE;
         char *eth1FullPath=NULL;   
         char *eth2FullPath=NULL;      
         char ethLagLowerLayers[MDM_MULTI_FULLPATH_BUFLEN];
         
         if ((ret = rutEthLag_getAvailableIntf(&bondIntfName, sizeof(bondIntfName)-1)) != CMSRET_SUCCESS)
         {
            cmsLog_error("No available bond interface., ret=%d", ret);
            return ret;
         }
                
         CMSMEM_REPLACE_STRING_FLAGS(newObj->name, bondIntfName, mdmLibCtx.allocFlags);            

         /* Need to append both eth fullpath to bondIntf lowerLayers */   
         if ((ret = qdmIntf_intfnameToFullPathLocked(newObj->X_BROADCOM_COM_EthIfName1, layer2, &eth1FullPath)) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to get fullpath of %s, ret=%d", newObj->X_BROADCOM_COM_EthIfName1, ret);
            return ret;
         }
         if ((ret = qdmIntf_intfnameToFullPathLocked(newObj->X_BROADCOM_COM_EthIfName2, layer2, &eth2FullPath)) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to get fullpath of %s, ret=%d", newObj->X_BROADCOM_COM_EthIfName2, ret);
            return ret;
         }
         
         cmsUtl_strncpy(ethLagLowerLayers, eth1FullPath, sizeof(ethLagLowerLayers)-1);
         CMSMEM_FREE_BUF_AND_NULL_PTR(eth1FullPath);      
         ret = cmsUtl_addFullPathToCSL(eth2FullPath, ethLagLowerLayers, sizeof(ethLagLowerLayers));
         CMSMEM_FREE_BUF_AND_NULL_PTR(eth2FullPath);   
         
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to add %s to ethLagLowerLayers. ret %d", eth2FullPath, ret);
         }
         else
         {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->lowerLayers, ethLagLowerLayers, mdmLibCtx.allocFlags);       
            cmsLog_debug("lagName is null and now is %s, newObj->lowerLayers %s", newObj->name,  newObj->lowerLayers);
         }
      }


     /* config ethLag object - add==TRUE*/
     rutEthLag_configInterface(newObj, add);

   }

   if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      add = FALSE;
      
      cmsLog_debug("Disable linux bonding interface %s", currObj->name);
 
      /* remove ethLag object actions - add==FALSE */
      rutEthLag_configInterface(currObj, add);
      
      if (!currObj->X_BROADCOM_COM_Upstream)
      {
         /* For lan ethLag, need to remove it from the bridge (action only) */
         rutLan_removeInterfaceFromBridge(currObj->name, "br0");  //todo br0
      }
      
      if (DELETE_EXISTING(newObj, currObj))
      {
         char ifNameBuf[CMS_IFNAME_LENGTH]={0};

         rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_ETHERNET,  MDMOID_LA_G, iidStack, -1);

         if (!currObj->X_BROADCOM_COM_Upstream)
         {
            /* For lan - ethLag object deletion involves, delete it from bridge and add 
            * back 2 ethernet interfaces back to bridge 
            */
#ifdef SUPPORT_LANVLAN
            /* for LAN eth port only */
            snprintf(ifNameBuf, sizeof(ifNameBuf), "%s.0", currObj->name);
#else
            cmsUtl_strncpy(ifNameBuf, currObj->name, sizeof(ifNameBuf)-1);
#endif  
            rutBridge_deleteIntfNameFromBridge_dev2(ifNameBuf);         
         }
                  
         if (!qdmIntf_isLayer2IntfNameUpstreamLocked(currObj->X_BROADCOM_COM_EthIfName1))
         {
#ifdef SUPPORT_LANVLAN
            /* for LAN eth port only */
            snprintf(ifNameBuf, sizeof(ifNameBuf), "%s.0", currObj->X_BROADCOM_COM_EthIfName1);
#else
            cmsUtl_strncpy(ifNameBuf, currObj->X_BROADCOM_COM_EthIfName1, sizeof(ifNameBuf)-1);
#endif  
            rutBridge_addLanIntfToBridge_dev2(ifNameBuf, "br0");
         }  
         if (!qdmIntf_isLayer2IntfNameUpstreamLocked(currObj->X_BROADCOM_COM_EthIfName2))
         {
#ifdef SUPPORT_LANVLAN
            /* for LAN eth port only */
            snprintf(ifNameBuf, sizeof(ifNameBuf), "%s.0", currObj->X_BROADCOM_COM_EthIfName2);
#else
            cmsUtl_strncpy(ifNameBuf, currObj->X_BROADCOM_COM_EthIfName2, sizeof(ifNameBuf)-1);
#endif  
            rutBridge_addLanIntfToBridge_dev2(ifNameBuf, "br0");
         }  
      }
   }

   cmsLog_debug(" Exit ret %d", ret);
   
   return ret;
}


CmsRet rcl_dev2EthLAGStatsObject(_Dev2EthLAGStatsObject *newObj __attribute__((unused)),
                                 const _Dev2EthLAGStatsObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)), 
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


#endif /* DMP_DEVICE2_ETHLAG_1  */
#endif /* DMP_DEVICE2_BASELINE_1 */
