/***********************************************************************
 *
 *  Copyright (c) 2006-2013  Broadcom Corporation
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

#ifdef DMP_DEVICE2_VLANTERMINATION_1


#include "cms_core.h"
#include "cms_util.h"
#include "stl.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "vlanctl_api.h"
#include "rut2_util.h"
#include "cms_qdm.h"
#include "rut2_ethernetvlantermination.h"


/*!\file rcl2_ethernetvlantermination.c
 * \brief This file contains ethernet vlantermination related functions.
 *
 */


static void modifyNumEthernetVlanTerminationEntry(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_ETHERNET,
                        MDMOID_DEV2_VLAN_TERMINATION,
                        iidStack,
                        delta);
}

CmsRet rcl_dev2VlanTerminationObject( _Dev2VlanTerminationObject *newObj,
                const _Dev2VlanTerminationObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      modifyNumEthernetVlanTerminationEntry(iidStack, 1);
   }

   IF_STATUS_HAS_CHANGED_SET_LASTCHANGE(newObj, currObj);

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("In ENABLE_NEW_OR_ENABLE_EXISTING");

      if (!IS_EMPTY_STRING(newObj->lowerLayers) &&
          IS_EMPTY_STRING(newObj->name))
      {
         /*Need to fill vlan interface name if is empty 
         */
         char ifName[CMS_IFNAME_LENGTH]={0};
         SINT32 vlanConId=0;
         
         cmsLog_debug("newObj->lowerLayers %s", newObj->lowerLayers);

         if ((ret = qdmIntf_fullPathToIntfnameLocked_dev2(newObj->lowerLayers, ifName)) != CMSRET_SUCCESS)
         {
            cmsLog_error("qdmIntf_getIntfnameFromFullPathLocked failed. ret %d", ret);
            return ret;
         }
         cmsLog_debug("newObj->lowerLayers %s  ifName=%s", newObj->lowerLayers, ifName);

         if (cmsMdm_isDataModelDevice2())
         {
            char vlanIfName[CMS_IFNAME_LENGTH]={0};

            /* LAN side vlan interface name is based on its VLAN ID. WAN vlanmux mode
            *  Need to get the vlan conId to form the vlan interface name (base3IfName in TR98). 
            * eg. ptm0.1
            */
#if SUPPORT_LANVLAN
            if (newObj->VLANID != -1 && qdmIntf_isLayer2IntfNameUpstreamLocked(ifName) == FALSE)
            {
               snprintf(vlanIfName, sizeof(vlanIfName), "%s.%d", ifName, newObj->VLANID);
            }
            else
#endif
#ifdef DMP_DEVICE2_VLANBRIDGE_1
            if ((ret = rutUtil_getAvailVlanIndex_dev2(ifName, &vlanConId)) == CMSRET_SUCCESS)
            {
               snprintf(vlanIfName, sizeof(vlanIfName), "%s.%d", ifName, vlanConId);
            }
#else
            if ((ret = rut2Vlan_getAvailVlanIndex_dev2(&vlanConId)) == CMSRET_SUCCESS)
            {
               snprintf(vlanIfName, sizeof(vlanIfName), "%s.%d", ifName, vlanConId);
            }
#endif
            CMSMEM_REPLACE_STRING_FLAGS(newObj->name, vlanIfName, mdmLibCtx.allocFlags);            
         }
         else
         {
           /* Just copy eth.link.name over in hybrid mode */
            CMSMEM_REPLACE_STRING_FLAGS(newObj->name, ifName, mdmLibCtx.allocFlags);            
         }
      }
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      char l2IntfNameBuf[CMS_IFNAME_LENGTH]={0};
      UBOOL8 isBridge=TRUE;

      cmsLog_debug("POTENTIAL_CHANGE_OF_EXISTING, newObj->status %s, currObj->status %s", 
         newObj->status, currObj->status);
         
      /* Action taken for vlan  in hybrid mode */
      if (!cmsMdm_isDataModelDevice2())
      {
#ifdef SUPPORT_IPV6
         cmsLog_debug("currObj->X_BROADCOM_COM_LastChange %d", currObj->X_BROADCOM_COM_LastChange);

         /* May need to add the network in a static IPv6 setup or
         * the "Cannot find device L3IfName" is usually observed in rest of boot
         */
         if (currObj->X_BROADCOM_COM_LastChange == 0 &&
             !cmsUtl_strcmp(newObj->status, MDMVS_UP) &&
             cmsUtl_strcmp(currObj->status, MDMVS_UP))
         {
            char ipv6AddrStrBuf[CMS_IPADDR_LENGTH]={0};
            char origin[BUFLEN_32];
            char prefixPath[BUFLEN_128];
            UBOOL8 isWan;

            ret = qdmIpIntf_getIpv6AddrInfoByNameLocked_dev2(newObj->name,
                                     ipv6AddrStrBuf, sizeof(ipv6AddrStrBuf),
                                     origin, sizeof(origin),
                                     prefixPath, sizeof(prefixPath), &isWan,
                                     CMS_IPV6_ORIGIN_ANY, CMS_IPV6_ORIGIN_STATIC);

            if (ret == CMSRET_SUCCESS &&
                !rut_checkInterfaceUp(newObj->name) &&
                !cmsUtl_strcmp(origin, MDMVS_STATIC) &&
                isWan)
            {
               if ((ret = qdmIpIntf_getLayer2IntfNameByLayer3IntfNameLocked(newObj->name, l2IntfNameBuf)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("qdmIpIntf_getLayer2IntfNameByLayer3IntfNameLocked failed. ret %d", ret);
                  return ret;
               }

               isBridge = qdmIpIntf_isWanInterfaceBridgedLocked(newObj->name);

               /* 
                * New status is "Up", need to config Vlan interface
                */
               ret = rutWan_startL3Interface(newObj->VLANID, 
                                             newObj->X_BROADCOM_COM_Vlan8021p, 
                                             newObj->TPID,
                                             l2IntfNameBuf, newObj->name,
                                             CMS_CONNECTION_MODE_VLANMUX, isBridge);
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("rutWan_startL3Interface failed. ret %d", ret);
                  return ret;
               }
            }
         }
#endif
         return CMSRET_SUCCESS;
      }
         
      if (!cmsUtl_strcmp(newObj->status, MDMVS_UP) &&
          cmsUtl_strcmp(currObj->status, MDMVS_UP))
      {
         cmsLog_debug("newObj->lowerLayers %s", newObj->lowerLayers);
         if ((ret = qdmIntf_fullPathToIntfnameLocked_dev2(newObj->lowerLayers, l2IntfNameBuf)) != CMSRET_SUCCESS)
         {
            cmsLog_error("qdmIntf_getIntfnameFromFullPathLocked failed. ret %d", ret);
            return ret;
         }

         if (qdmIntf_isLayer2IntfNameUpstreamLocked_dev2(l2IntfNameBuf))
         {
            isBridge = qdmIpIntf_isWanInterfaceBridgedLocked(newObj->name);
         }

         /* 
          * New status is "Up", need to config Vlan interface
          */
         ret = rutWan_startL3Interface(newObj->VLANID, 
                                       newObj->X_BROADCOM_COM_Vlan8021p, 
                                       newObj->TPID,
                                       l2IntfNameBuf, newObj->name,
                                       CMS_CONNECTION_MODE_VLANMUX, isBridge);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("rutWan_startL3Interface failed. ret %d", ret); 
         }
      }
      else if (cmsUtl_strcmp(newObj->status, MDMVS_UP) &&
               !cmsUtl_strcmp(currObj->status, MDMVS_UP))
      {
         /*
          * Seems like these 3 simple lines from rutWan_stopL3Interface is
          * good enough.  No need to call the full function?
          */

         /* Need to remove the network from the routing table by
         * doing  "ifconfig L3IfName 0 0.0.0.0" if the interface is still existed
         */
         if (rut_wanGetIntfIndex(currObj->name) > 0)
         {
            char cmdStr[BUFLEN_128];
            snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s 0.0.0.0", currObj->name);
            rut_doSystemAction("rutWan_stopL3Interface: ifconfig L3IfName 0.0.0.0", (cmdStr));
#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
#ifdef SUPPORT_MAP
            rutMap_unbindUpstreamL3Interface(currObj->name);
#endif
            vlanCtl_init();
      	    /* Delete the virtual interface. All rules associated with it will be purged. */
            vlanCtl_deleteVlanInterface(currObj->name);
            vlanCtl_cleanup();
#endif
         }
      }
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("In DELETE_OR_DISABLE_EXISTING");

      /*
       * Seems like these 3 simple lines from rutWan_stopL3Interface is
       * good enough.  No need to call the full function?
       */
   
      /* Need to remove the network from the routing table by
      * doing  "ifconfig L3IfName 0 0.0.0.0" if the interface is still existed
      */
      if (rut_wanGetIntfIndex(currObj->name) > 0)
      {
         char cmdStr[BUFLEN_128];
         snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s 0.0.0.0", currObj->name);
         rut_doSystemAction("rutWan_stopL3Interface: ifconfig L3IfName 0.0.0.0", (cmdStr));
#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
         vlanCtl_init();
         /* Delete the virtual interface. All rules associated with it will be purged. */
         vlanCtl_deleteVlanInterface(currObj->name);
         vlanCtl_cleanup();
#endif
            
      }

      if (DELETE_EXISTING(newObj, currObj))
      {
         modifyNumEthernetVlanTerminationEntry(iidStack, -1);
      }
   }

   cmsLog_debug("Exit ret %d", ret);
   
   return ret;
}


#endif /* DMP_DEVICE2_VLANTERMINATION_1 */

#else
/* DMP_DEVICE2_BASELINE_1 is not defined */

#ifdef DMP_DEVICE2_ETHERNETINTERFACE_1
#error "Device2 ethernet vlantermination objects incompatible with current Data Model mode, go to make menuconfig to fix"
#endif


#endif /* DMP_DEVICE2_BASELINE_1 */
