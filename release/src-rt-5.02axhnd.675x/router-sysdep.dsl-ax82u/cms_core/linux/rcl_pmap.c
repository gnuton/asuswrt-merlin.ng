/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
 *  All Rights Reserved
 *
 *  <:label-BRCM:2011:proprietary:standard
 *  
 *   This program is the proprietary software of Broadcom and/or its
 *   licensors, and may only be used, duplicated, modified or distributed pursuant
 *   to the terms and conditions of a separate, written license agreement executed
 *   between you and Broadcom (an "Authorized License").  Except as set forth in
 *   an Authorized License, Broadcom grants no license (express or implied), right
 *   to use, or waiver of any kind with respect to the Software, and Broadcom
 *   expressly reserves all rights in and to the Software and all intellectual
 *   property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 *   NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 *   BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *  
 *   Except as expressly set forth in the Authorized License,
 *  
 *   1. This program, including its structure, sequence and organization,
 *      constitutes the valuable trade secrets of Broadcom, and you shall use
 *      all reasonable efforts to protect the confidentiality thereof, and to
 *      use this information only in connection with your use of Broadcom
 *      integrated circuit products.
 *  
 *   2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 *      AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 *      WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *      RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
 *      ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
 *      FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
 *      COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
 *      TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
 *      PERFORMANCE OF THE SOFTWARE.
 *  
 *   3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *      ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *      INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
 *      WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 *      IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
 *      OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *      SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
 *      SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
 *      LIMITED REMEDY.
 *  :>
 *
 ************************************************************************/

#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PMAP */

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
#include "rut_pmap.h"
#include "rut_lan.h"
#include "rut_route.h"
#include "rut_ethswitch.h"
#include "beep_networking.h"
#include "qdm_route.h"
#include "rut_iptables.h"

CmsRet rcl_l2BridgingObject( _L2BridgingObject *newObj __attribute__((unused)),
                const _L2BridgingObject *currObj __attribute__((unused)),
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

   if (newObj != NULL && currObj == NULL)
   {
      // This table is read only and we do not need to add anything.
      ;
   }
   else if (newObj != NULL && currObj != NULL)
   {
      // Edit the current instance.  No edit of this table is allowed.
      ;
   }
   else
   {
      // No delete is possible.
   }

   return CMSRET_SUCCESS;
}

CmsRet rcl_l2BridgingEntryObject( _L2BridgingEntryObject *newObj,
                const _L2BridgingEntryObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   char bridgeIfName[BUFLEN_32];
   CmsRet ret=CMSRET_SUCCESS;

   /* add new bridging entry */
   if (newObj != NULL && currObj == NULL)
   {
      if (newObj->bridgeName)
      {
         /*
          * If bridgeName is already defined, that means we are during
          * system bootup (MDM has filled the object from the config file
          * and are telling us to initialize it.
          * (don't need to do anything in this if block)
          */
         cmsLog_debug("startup condition: bridgeName=%s", newObj->bridgeName);
         /* 
          * BEEP requires interface group to create separated bridge
          * and need to add firewall rules and NAT rules
          */
         if (newObj->X_BROADCOM_COM_Mode == INTFGRP_BR_BEEP_SECONDARY_MODE)
         {
             char brifname[CMS_IFNAME_LENGTH]={0};
             char activeGwIfName[CMS_IFNAME_LENGTH]={0};
             UBOOL8 isIPv4 = TRUE;

             sprintf(brifname, "br%d", newObj->bridgeKey);
             /* firewall rules similar to guest wifi */
             rutIpt_beepNetworkingSecurity(brifname, INTFGRP_BR_BEEP_SECONDARY_MODE);

             qdmRt_getActiveDefaultGatewayLocked(activeGwIfName);
             if (!IS_EMPTY_STRING(activeGwIfName))
             {
                if (qdmIpIntf_isWanInterfaceUpLocked(activeGwIfName, isIPv4) &&
                    qdmIpIntf_isNatEnabledOnIntfNameLocked(activeGwIfName))
                {
                    rutIpt_beepNetworkingMasqueurade(brifname, activeGwIfName);
                }
             }
         }
         else if (newObj->X_BROADCOM_COM_Mode == INTFGRP_BR_BEEP_WANONLY_MODE)
         {
             char brifname[CMS_IFNAME_LENGTH]={0};
             char activeGwIfName[CMS_IFNAME_LENGTH]={0};
             UBOOL8 isIPv4 = TRUE;

             sprintf(brifname, "br%d", newObj->bridgeKey);
             /* firewall rules to only allow WAN access */
             rutIpt_beepNetworkingSecurity(brifname, INTFGRP_BR_BEEP_WANONLY_MODE);

             qdmRt_getActiveDefaultGatewayLocked(activeGwIfName);
             if (!IS_EMPTY_STRING(activeGwIfName))
             {
                if (qdmIpIntf_isWanInterfaceUpLocked(activeGwIfName, isIPv4) &&
                    qdmIpIntf_isNatEnabledOnIntfNameLocked(activeGwIfName))
                {
                    rutIpt_beepNetworkingMasqueurade(brifname, activeGwIfName);
                }
             }
         }
      }
      else
      {
         /*
          * bridgeName is not defined, that means we are in run-time and
          * the caller has just done a cmsObj_addInstance().
          * Create a LANDevice.
          */
         if ((ret = rutLan_addBridge(&(newObj->bridgeKey))) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutLan_addBridge failed, ret=%d", ret);
            return ret;
         }
         
         cmsLog_debug("added new bridge br%d", newObj->bridgeKey);
      }

      rut_modifyNumL2Bridging(iidStack, 1);
   }
   
   /* edit existing bridging entry */
   else if (newObj != NULL && currObj != NULL)
   {
      /* the only thing that can change is the bridgeName and the enable.
       * We don't care if the bridgeName changes. */
      if (newObj->bridgeEnable != currObj->bridgeEnable)
      {
         cmsLog_debug("new bridge state is %d", newObj->bridgeEnable);
         sprintf(bridgeIfName, "br%d", newObj->bridgeKey);
         if ((ret = rutPMap_setLanBridgeEnable(bridgeIfName, newObj->bridgeEnable)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not set LAN bridge to enable %d", newObj->bridgeEnable);
         }

         if (newObj->X_BROADCOM_COM_Mode == INTFGRP_BR_BEEP_SECONDARY_MODE)
         {
             char activeGwIfName[CMS_IFNAME_LENGTH]={0};
             UBOOL8 isIPv4 = TRUE;

             rutIpt_beepNetworkingSecurity(bridgeIfName, INTFGRP_BR_BEEP_SECONDARY_MODE);

             qdmRt_getActiveDefaultGatewayLocked(activeGwIfName);
             if (!IS_EMPTY_STRING(activeGwIfName))
             {
                if (qdmIpIntf_isWanInterfaceUpLocked(activeGwIfName, isIPv4) &&
                    qdmIpIntf_isNatEnabledOnIntfNameLocked(activeGwIfName))
                {
                    rutIpt_beepNetworkingMasqueurade(bridgeIfName, activeGwIfName);
                }
             }
         }
         else if (newObj->X_BROADCOM_COM_Mode == INTFGRP_BR_BEEP_WANONLY_MODE)
         {
             char activeGwIfName[CMS_IFNAME_LENGTH]={0};
             UBOOL8 isIPv4 = TRUE;

             rutIpt_beepNetworkingSecurity(bridgeIfName, INTFGRP_BR_BEEP_WANONLY_MODE);

             qdmRt_getActiveDefaultGatewayLocked(activeGwIfName);
             if (!IS_EMPTY_STRING(activeGwIfName))
             {
                if (qdmIpIntf_isWanInterfaceUpLocked(activeGwIfName, isIPv4) &&
                    qdmIpIntf_isNatEnabledOnIntfNameLocked(activeGwIfName))
                {
                    rutIpt_beepNetworkingMasqueurade(bridgeIfName, activeGwIfName);
                }
             }
         }
      }
   }
   
   /* delete bridging entry */
   else
   {

      rut_modifyNumL2Bridging(iidStack, -1);

      /*
       * All filter objects which point to a real LAN interface
       * should have been moved from the LANDevice.x to LANDevice.1
       * so we can now delete the LANDevice.x subtree.
       */
      sprintf(bridgeIfName, "br%d", currObj->bridgeKey);
      rutLan_deleteBridge(bridgeIfName);
   }
   
   
   return ret;
}


CmsRet rcl_l2BridgingFilterObject( _L2BridgingFilterObject *newObj,
                const _L2BridgingFilterObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("entered, newObj=%p currObj=%p", newObj, currObj);

   if (newObj != NULL && currObj == NULL)
   {
      // Add a new instance
      if (newObj->filterKey != 0)
      {
         /*
          * startup condition: the object is already populated with non-default values.
          * Do action now.
          */
         if (newObj->filterBridgeReference < 0)
         {
            cmsLog_debug("filter does not belong to a bridge, iidStack=%s filterBridgeRef=%d",
                         cmsMdm_dumpIidStack(iidStack), newObj->filterBridgeReference);
         }
         else
         {
            if (rutPMap_isLanInterfaceFilter(newObj->filterInterface))
            {
               /*
                * During startup, when initializing from config file, we
                * don't have to do anything for the LAN interfaces.
                * The rcl/rut functions for LANDevice. will do everything.
                */
            }
            else if (rutPMap_isWanInterfaceFilter(newObj->filterInterface))
            {
               /*
                * Hmm, this could be a problem.  The wan intf and the bridge is
                * not up yet.
                */
               if ((ret = rutPMap_associateWanIntfWithBridge(newObj->filterInterface, newObj->filterBridgeReference)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("could not associate wan intf with bridge, ret=%d", ret);
                  return ret;
               }
            }
            else
            {
               /* this must be a dhcp vendor id filter */
               if ((ret = rutPMap_associateDhcpVendorIdWithBridge(newObj->sourceMACFromVendorClassIDFilter, newObj->filterBridgeReference)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("could not associate dhcp vendor id with bridge, ret=%d", ret);
                  return ret;
               }
            }
         }
      }
      else
      {
         /*
          * runtime object create.  Assign a new filter key.  But don't do any action
          * yet because not all the fields are populated yet.
          */
         newObj->filterKey = PEEK_INSTANCE_ID(iidStack);
      }

      rut_modifyNumL2BridgingFilter(iidStack, 1);
   }
   else if (newObj != NULL && currObj != NULL)
   {
      /*
       * This is the edit case.
       * There are 3 types of filters that we need to handle separately.
       * (1) when virtual LAN ports are moved.
       * (2) when WAN interface is created and then assigned to another bridge group,
       *     or when it is moved from one bridge group to another.
       * (3) when DHCP vendor id associated with a port group.
       */
      if (rutPMap_isLanInterfaceFilter(newObj->filterInterface))
      {
         cmsLog_debug("LAN intf case, filterBridgeRef %d->%d", currObj->filterBridgeReference, newObj->filterBridgeReference);
#ifdef SUPPORT_LANVLAN
         if (currObj->X_BROADCOM_COM_VLANIDFilter > 0)
         {
            char bridgeIfName[CMS_IFNAME_LENGTH];
            char vlanIfName[CMS_IFNAME_LENGTH];
            char vlanIfName2[BUFLEN_8]={0};

            if (currObj->filterBridgeReference >= 0)
            {
               sprintf(bridgeIfName, "br%d", currObj->filterBridgeReference);
  
               if ((ret = rutPMap_filterInterfaceToIfName(currObj->filterInterface, vlanIfName)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("could not convert filterInterface %s to lanIfName", currObj->filterInterface);
               }
               else
               {
                  snprintf(vlanIfName2, sizeof(vlanIfName2), ".%d", currObj->X_BROADCOM_COM_VLANIDFilter);
                  cmsUtl_strncat(vlanIfName, CMS_IFNAME_LENGTH, vlanIfName2);
                  rutLan_removeInterfaceFromBridge(vlanIfName, bridgeIfName);
               }
            }
			
            if (newObj->filterBridgeReference >= 0)
            {
               sprintf(bridgeIfName, "br%d", newObj->filterBridgeReference);
  
               if ((ret = rutPMap_filterInterfaceToIfName(newObj->filterInterface, vlanIfName)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("could not convert filterInterface %s to lanIfName", newObj->filterInterface);
               }
               else
               {
                  snprintf(vlanIfName2, sizeof(vlanIfName2), ".%d", newObj->X_BROADCOM_COM_VLANIDFilter);
                  cmsUtl_strncat(vlanIfName, CMS_IFNAME_LENGTH, vlanIfName2);
                  rutLan_addInterfaceToBridge(vlanIfName, FALSE, bridgeIfName);
               }
            }

            /* since we have changed VLAN config, may need to change HW switching setting */
            rutEsw_updateRealHwSwitchingSetting(QOS_CLS_INVALID_INDEX);
         }
         else
#endif
         {
           /*
            * LAN interfaces are never deleted in a port map operation.
            * They only get moved from one bridge group to another.  (even
            * for wireless.)
            * LAN interfaces can get deleted by disabling virtual ports, but
            * all the LANDevice. sub-tree manipulations are done by rut_ethswitch.c
            * (In the wlan case, if a virtIntfCfg (SSID) is diabled,
            * the filterBridgeReference goes to -1 and the interface is taken out
            * of the bridge).
            */
            if (newObj->filterBridgeReference < 0 && currObj->filterBridgeReference >= 0)
            {
               char bridgeIfName[CMS_IFNAME_LENGTH];
               char lanIfName[CMS_IFNAME_LENGTH];

              /* 
               * This is done by the wlan code.  Remove the interface from the bridge.
               */
               sprintf(bridgeIfName, "br%d", currObj->filterBridgeReference);

               if ((ret = rutPMap_filterInterfaceToIfName(newObj->filterInterface, lanIfName)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("could not convert filterInterface %s to lanIfName", newObj->filterInterface);
               }
               else
               {
                  rutLan_disableInterface(lanIfName);
                  rutLan_removeInterfaceFromBridge(lanIfName, bridgeIfName);
               }

            }
            else if (newObj->filterBridgeReference >= 0 && currObj->filterBridgeReference < 0)
            {
               char bridgeIfName[CMS_IFNAME_LENGTH];
               char lanIfName[CMS_IFNAME_LENGTH];
               
              /* 
               * This is done by the wlan code.  Add the interface to the bridge.
               */
               sprintf(bridgeIfName, "br%d", newObj->filterBridgeReference);

               if ((ret = rutPMap_filterInterfaceToIfName(newObj->filterInterface, lanIfName)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("could not convert filterInterface %s to lanIfName", newObj->filterInterface);
               }
               else
               {
                  rutLan_addInterfaceToBridge(lanIfName, FALSE, bridgeIfName);
               }
            }
            else if (currObj->filterBridgeReference >= 0 && newObj->filterBridgeReference >= 0)
            {
               InstanceIdStack bridgeIId = EMPTY_INSTANCE_ID_STACK;
               _L2BridgingEntryObject *bridgeObj = NULL;
               
                if (currObj->filterBridgeReference == newObj->filterBridgeReference)
                {
                   cmsLog_debug("edit of LAN filter, but bridge ref did not change (hmm, what changed)");
                }
                else
                {
                   if ((ret = rutPMap_moveLanInterface(currObj->filterInterface, currObj->filterBridgeReference, newObj->filterBridgeReference)) != CMSRET_SUCCESS)
                   {
                      cmsLog_error("could not move LAN interface from bridge %d to %d",
                                   currObj->filterBridgeReference,
                                   newObj->filterBridgeReference);
                   }
                }

#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */
               /*
                * Try to remove policy route by first get the bridge name (group name). For bridged interface group, 
                * rutRt_deletePolicyRouting will not be able to find any policy route rules and
                * just do nothing and return.
                */
               if ((ret = rutPMap_getBridgeByKey((UINT32) currObj->filterBridgeReference, &bridgeIId, &bridgeObj)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("Error retrieving the bridge ref %d", currObj->filterBridgeReference);
                  return ret;
               }
               else
               {
                  UBOOL8 fromPolicyRoute = FALSE;
                  rutRt_deletePolicyRouting(fromPolicyRoute, bridgeObj->bridgeName);
                  cmsObj_free((void **) &bridgeObj);
               }
#endif /* DMP_BRIDGING_1 */
               
            }
            else
            {
                 cmsLog_notice("unexpected change in bridge ref, from %d to %d",
                            currObj->filterBridgeReference,
                            newObj->filterBridgeReference);
            }
         }
      }
      else if (rutPMap_isWanInterfaceFilter(newObj->filterInterface))
      {
         UBOOL8 isEditWanFilter = FALSE;
         cmsLog_debug("edit WAN filter: currBridgeRef=%d newBridgeRef=%d", currObj->filterBridgeReference, newObj->filterBridgeReference);
         if ((currObj->filterBridgeReference >= 0) && (newObj->filterBridgeReference >= 0) &&
            (cmsUtl_strcmp(currObj->filterInterface, newObj->filterInterface) == 0) && (currObj->filterBridgeReference != newObj->filterBridgeReference))
         {
            isEditWanFilter = TRUE;/*special for interface group creat/delete or similar change*/
         }
         cmsLog_debug("isEditWanFilter=%d", isEditWanFilter);
         
         if (currObj->filterBridgeReference >= 0)
         {
            if ((ret = rutPMap_disassociateWanIntfFromBridge(currObj->filterInterface, currObj->filterBridgeReference, isEditWanFilter)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Error disassociating from the current bridge ret %d", ret);
               return ret;
            }
         }
         
         if (newObj->filterBridgeReference >= 0)
         {
            if ((ret = rutPMap_associateWanIntfWithBridge(newObj->filterInterface, newObj->filterBridgeReference)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Error associating filter to new bridge");
               return ret;
            }
         }
      }
      else
      {
         /*
          * The dhcp vendor id filter is being associated with a bridge.
          * The only edit we support for dhcp vendor id filters is moving
          * from bridge ref -1 to bridge ref >= 0 because that is how the
          * web UI does it.  If we really need to support the ACS moving
          * a dhcp vendor id filter from one bridge group to another, we
          * can add support for that later.
          */
         if ((ret = rutPMap_associateDhcpVendorIdWithBridge(newObj->sourceMACFromVendorClassIDFilter, newObj->filterBridgeReference)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not associate dhcp vendor id with bridge, ret=%d", ret);
            return ret;
         }
      }
   }
   else
   {

      rut_modifyNumL2BridgingFilter(iidStack, -1);

      /*
       * this is the delete case, currObj!=NULL and newObj == NULL.
       * Filters are deleted in three cases:
       * (1) when virtual LAN ports are disabled
       * (2) when WAN interface is deleted.
       * (3) when DHCP vendor id associated with a port group is deleted.
       */
      if (rutPMap_isLanInterfaceFilter(currObj->filterInterface))
      {
         rutEsw_updateRealHwSwitchingSetting(QOS_CLS_INVALID_INDEX);

         /*
          * Don't have to do anything in this case.  rut_ethswitch.c
          * has already deleted the LAN interfaces for us.
          */
      }
      else if (rutPMap_isWanInterfaceFilter(currObj->filterInterface))
      {
          /*
           * The WAN interface is being deleted (which triggers a delete of the
           * filter), undo any policy routing rules channelling traffic to
           * this WAN interface.
           */
          if ((ret = rutPMap_disassociateWanIntfFromBridge(currObj->filterInterface, currObj->filterBridgeReference, FALSE)) != CMSRET_SUCCESS)
          {
            cmsLog_error("could not disassociate WAN intf from bridge for delete, ret=%d", ret);
          }
      }
      else
      {
         /*
          * The dhcp vendor id filter is being deleted.
          */
         if ((ret = rutPMap_disassociateDhcpVendorIdFromBridge(currObj->sourceMACFromVendorClassIDFilter, currObj->filterBridgeReference)) != CMSRET_SUCCESS)
         {
            cmsLog_error("error disassociating dhcp vendor id from bridge for delete, ret=%d", ret);
         }
      }
   }

   cmsLog_debug("done, returning %d", ret);
   return ret;
}

#ifdef NOT_SUPPORTED
CmsRet rcl_l2BridgingMarkingObject( _L2BridgingMarkingObject *newObj __attribute__((unused)),
                const _L2BridgingMarkingObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_INTERNAL_ERROR;
}
#endif


/** This is the Layer2Bridging.AvailableInterface.{i}. object */
CmsRet rcl_l2BridgingIntfObject( _L2BridgingIntfObject *newObj,
                const _L2BridgingIntfObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }

   if (newObj != NULL && currObj == NULL)
   {
      // Add a new instance
      if (newObj->availableInterfaceKey == 0)
      {
         // this is a runtime object create, generate a unique key, which is just its instance number.
         newObj->availableInterfaceKey = PEEK_INSTANCE_ID(iidStack);
      }

      rut_modifyNumL2BridgingIntf(iidStack, 1);
   }
   else if (newObj != NULL && currObj != NULL)
   {
      // Edit the current instance
      // Is there anything in here to edit?
   }
   else
   {
      // Delete current instance
      rut_modifyNumL2BridgingIntf(iidStack, -1);

      // No action needed when an available instance is deleted.

   }

   return ret;
}


#endif /* DMP_BRIDGING_1  aka SUPPORT_PMAP */
