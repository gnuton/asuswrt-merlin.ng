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

#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "bcmnet.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_atm.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "rut_wanlayer2.h"
#include "rut_gponwan.h"

#ifdef DMP_BRIDGING_1
#include "rut_pmap.h"
#endif

UBOOL8 rutGpon_getGponLinkByIfName(char *ifName, InstanceIdStack *iidStack, _WanGponLinkCfgObject **gponLinkCfg)
{
   _WanGponLinkCfgObject *gponLinkObj=NULL;
   UBOOL8 found=FALSE;
   InstanceIdStack wanDevIid;

   if (rutWl2_getGponWanIidStack(&wanDevIid) != CMSRET_SUCCESS)
   {
      return found;
   }
   
   while (cmsObj_getNextInSubTree(MDMOID_WAN_GPON_LINK_CFG, &wanDevIid, iidStack, (void **)&gponLinkObj) == CMSRET_SUCCESS)
   {
      cmsLog_debug("ifName = %s", gponLinkObj->ifName);
      if (!cmsUtl_strcmp(gponLinkObj->ifName, ifName))
      {
         found = TRUE;
         break;
      }
      else
      {
         cmsObj_free((void **) &gponLinkObj);
      }
   }

   if (found)
   {
      if (gponLinkCfg != NULL)
      {
         *gponLinkCfg = gponLinkObj;
      }
      else
      {
         cmsObj_free((void **) &gponLinkObj);
      }
   }

   cmsLog_debug("found=%d, ifName=%s", found, ifName);
   
   return found;
}

CmsRet rutGpon_getVeipVlanName(char *ifName)
{
    CmsRet ret = CMSRET_SUCCESS;

    if (ifName == NULL)
        return CMSRET_INVALID_ARGUMENTS;

    strncpy(ifName, GPON_WAN_IF_NAME, CMS_IFNAME_LENGTH);

    return ret;
}

CmsRet rutGpon_getServiceOidAndIidStack(const InstanceIdStack *gponLinkCfgIid,
                                        MdmObjectId *oid, InstanceIdStack *iidStack)
{
   CmsRet ret = CMSRET_SUCCESS;
   WanPppConnObject *pppConn = NULL;
   WanIpConnObject *ipConn = NULL;
   WanConnDeviceObject *wanConn = NULL;
   InstanceIdStack parentIidStack = *gponLinkCfgIid;

   /* Get the parent WAN_CONN)DEVICE */
   if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_CONN_DEVICE, 
      MDMOID_WAN_GPON_LINK_CFG, &parentIidStack, OGF_NO_VALUE_UPDATE, (void **)&wanConn)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Fail to get cmsObj_getAncestor(MDMOID_WAN_CONN_DEVICE). ret=%d", ret);
      return ret;
   }
   cmsObj_free((void **) &wanConn);

   INIT_INSTANCE_ID_STACK(iidStack);
   if ((ret = cmsObj_getNextInSubTree(MDMOID_WAN_PPP_CONN, &parentIidStack, iidStack, (void**)&pppConn)) == CMSRET_SUCCESS)
   {
      *oid = MDMOID_WAN_PPP_CONN;
      cmsObj_free((void**)&pppConn);
      return ret;
   }


   INIT_INSTANCE_ID_STACK(iidStack);
   if ((ret = cmsObj_getNextInSubTree(MDMOID_WAN_IP_CONN, &parentIidStack, iidStack, (void**)&ipConn)) == CMSRET_SUCCESS)
   {
      *oid = MDMOID_WAN_IP_CONN;
      cmsObj_free((void**)&ipConn);
      return ret;
   }
      
   return ret;
   
}  /* End of rutGpon_getServiceOidAndIidStack() */

CmsRet rutGpon_getWanServiceL2IfName(const MdmObjectId oid, 
                                     const InstanceIdStack *iidStack,
                                     char *pL2Ifname)
{
   CmsRet ret = CMSRET_OBJECT_NOT_FOUND;
   WanGponLinkCfgObject *l2LinkCfgObj = NULL;
   InstanceIdStack wanConnIid = *iidStack;

   if (iidStack == NULL)
   {
      cmsLog_error("iidStack must be provided");
      return ret;
   }
   if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_GPON_LINK_CFG, 
                                      oid, 
                                      &wanConnIid, 
                                      OGF_NO_VALUE_UPDATE,
                                      (void **) &l2LinkCfgObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get gponLinkCfgObj from %s", cmsMdm_dumpIidStack( (void *) iidStack));
      return ret;
   }

   if (l2LinkCfgObj->ifName != NULL)
      strncpy(pL2Ifname, l2LinkCfgObj->ifName, CMS_IFNAME_LENGTH);
   /* Free the link object */
   cmsObj_free((void**)&l2LinkCfgObj);
   return ret;
}

CmsRet rutGpon_getWanServiceParams(const MdmObjectId oid, 
                                   const InstanceIdStack *iidStack,
                                   GponWanServiceParams *pServiceParams)
{
   WanIpConnObject *ipConn = NULL;
   WanPppConnObject *pppConn = NULL;
   CmsRet ret = CMSRET_OBJECT_NOT_FOUND;

   if (iidStack == NULL)
   {
      cmsLog_error("iidStack must be provided");
      return ret;
   }
   switch (oid)
   {
      case MDMOID_WAN_IP_CONN:
         ret = cmsObj_get(oid, iidStack, OGF_NO_VALUE_UPDATE, (void **)&ipConn);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("could not get WanIpConnObject to do update on, ret=%d", ret);
         }
         else
         {
            pServiceParams->pbits = ipConn->X_BROADCOM_COM_VlanMux8021p;
            pServiceParams->vlanId = ipConn->X_BROADCOM_COM_VlanMuxID;
            pServiceParams->noMcastVlanFilter = ipConn->X_BROADCOM_COM_NoMcastVlanFilter;
            pServiceParams->igmpEnabled = ipConn->X_BROADCOM_COM_IGMPEnabled;
#ifdef DMP_BRIDGING_1
            /* If Multicast is not enabled at WAN service and service type is Bridged */
            if (!pServiceParams->igmpEnabled && !strcmp(ipConn->connectionType,MDMVS_IP_BRIDGED))
            {
                pServiceParams->igmpEnabled = rutPMap_getIgmpSnoopingForBridgedWanIf(ipConn->X_BROADCOM_COM_IfName);
            }
#endif /* DMP_BRIDGING_1 */
            cmsLog_debug("IP-CONN:<%d:%d> <%d %d %d %d>", pServiceParams->serviceStatus,ipConn->enable,
                         pServiceParams->pbits,pServiceParams->vlanId,pServiceParams->noMcastVlanFilter,pServiceParams->igmpEnabled);
            /* Free the IP_CONN object */
            cmsObj_free((void**)&ipConn);
         }
         break;
      case MDMOID_WAN_PPP_CONN:
         ret = cmsObj_get(oid, iidStack, OGF_NO_VALUE_UPDATE, (void **)&pppConn);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("could not get WanPppConnObject to do update on, ret=%d", ret);
         }
         else
         {
            pServiceParams->pbits = pppConn->X_BROADCOM_COM_VlanMux8021p;
            pServiceParams->vlanId = pppConn->X_BROADCOM_COM_VlanMuxID;
            pServiceParams->noMcastVlanFilter = pppConn->X_BROADCOM_COM_NoMcastVlanFilter;
            pServiceParams->igmpEnabled = pppConn->X_BROADCOM_COM_IGMPEnabled;
            cmsLog_debug("PPP-CONN:<%d:%d> <%d %d %d %d>", pServiceParams->serviceStatus,pppConn->enable,
                         pServiceParams->pbits,pServiceParams->vlanId,pServiceParams->noMcastVlanFilter,pServiceParams->igmpEnabled);
            /* Free the PPP object */
            cmsObj_free((void**)&pppConn);
         }
         break;
      default :
         break;
   }
   return ret;
}

#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 */
