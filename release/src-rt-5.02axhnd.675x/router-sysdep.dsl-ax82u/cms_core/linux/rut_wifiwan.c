/*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
:>
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>

#include "rcl.h"
#include "rut_lan.h"
#include "rut_util.h"
#include "rut_pmap.h"
#include "rut_wifiwan.h"


#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
UBOOL8 rutWanWifi_isWlWanMode()
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanCommonIntfCfgObject *wanCommon = NULL;
   UBOOL8 found = FALSE;

   while((!found) &&
         (CMSRET_SUCCESS == cmsObj_getNext(MDMOID_WAN_COMMON_INTF_CFG, &iidStack, (void **)&wanCommon)))
   {
      if (cmsUtl_strcmp(wanCommon->WANAccessType, MDMVS_X_BROADCOM_COM_WIFI) == 0)
      {
          found = TRUE;
      }         
      cmsObj_free((void **) &wanCommon);
   } /* while loop over WANCommon. */   
   return found;
}


UBOOL8 rutWanWifi_getWlIntfByIfName(char *ifName, InstanceIdStack *iidStack, WanWifiIntfObject **wlIntfCfg)
{
   WanWifiIntfObject *wanWl=NULL;
   CmsRet ret;
   UBOOL8 found=FALSE;

   cmsLog_debug("Entering...ifName=%s", ifName);
   
   if ((ret = rutWifiWan_getWanWifiObject(iidStack, &wanWl)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Cannot get WanWifiIntfObject, ret = %d", ret);
   }
   else
   {
      if(cmsUtl_strcmp(wanWl->ifName, ifName))
      {
         /* ifname is not same, skip this object. */
         cmsLog_debug("found WANWl, but ifNames do not match, looking for %s got %s",
                      ifName, wanWl->ifName);

         cmsObj_free((void **)&wanWl);
      }
      else
      {
         found = TRUE;
         if (wlIntfCfg != NULL)
         {
            /* give object back to caller, so don't free */
            *wlIntfCfg = wanWl;
         }
         else
         {
            cmsObj_free((void **)&wanWl);
         }
      }
   }

   cmsLog_debug("found=%d, ifName=%s", found, ifName);
   
   return found;
}


CmsRet rutWifiWan_getLayer3IntfNameByIidStack(const InstanceIdStack *parentIidStack,
                                              char *intfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   WanIpConnObject *ipConnObj=NULL;
   CmsRet ret;

   ret = cmsObj_getNextInSubTreeFlags(MDMOID_WAN_IP_CONN, parentIidStack,
                        &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConnObj);
   if (ret == CMSRET_SUCCESS)
   {
      /* any IP conn object we find in this subtree should be for wifi */
      cmsLog_debug("got IfName=%s", ipConnObj->X_BROADCOM_COM_IfName);
      if (cmsUtl_strlen(ipConnObj->X_BROADCOM_COM_IfName) > 0)
      {
         cmsUtl_strcpy(intfName, ipConnObj->X_BROADCOM_COM_IfName);
      }
      else
      {
         ret = CMSRET_OBJECT_NOT_FOUND;
      }
   }

   return ret;
}


void rutWan_moveWifiLanToWan(const char *ifName)
{
    _WlVirtIntfCfgObject *wlVirtIntfCfgObj = NULL;
   _WanWifiIntfObject *wanWifiObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("Entering: ifName=%s", ifName);

   if ((ret = rutLan_getWlanInterface(ifName, &iidStack, &wlVirtIntfCfgObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cannot find the interface (%s) in br0", ifName);
      return;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = rutWifiWan_getWanWifiObject(&iidStack, &wanWifiObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get WanWifiIntfObject, ret=%d", ret);
      cmsObj_free((void **) &wlVirtIntfCfgObj);
      return;
   }

   /*
    * Transfer settings of the LAN WlInterface object to the
    * WANWifiInterface object.  Don't enable it yet, let upper layer
    * do that when it is ready.
    */
   CMSMEM_REPLACE_STRING_FLAGS(wanWifiObj->MACAddress, wlVirtIntfCfgObj->wlBssMacAddr, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(wanWifiObj->ifName, wlVirtIntfCfgObj->wlIfcname, mdmLibCtx.allocFlags);
               
   ret = cmsObj_set(wanWifiObj, &iidStack);

   cmsObj_free((void **) &wanWifiObj);
   cmsObj_free((void **) &wlVirtIntfCfgObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set WanWifiIntfObject, ret = %d", ret);
      return;
   }

  /* Here interface grouping is not implemented for now */
   rutLan_deleteWlanInterface(ifName);

   return;
}

void rutWan_moveWifiWanToLan(const char *ifName)
{
   _WlVirtIntfCfgObject *wlVirtIntfCfgObj = NULL;
   _WanWifiIntfObject *wanWifiObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   cmsLog_debug("Entering: ifName=%s", ifName);

   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = rutWifiWan_getWanWifiObject(&iidStack, &wanWifiObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get WanWifiIntfObject, ret=%d", ret);
      return;
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(wanWifiObj->ifName);
   wanWifiObj->enable = FALSE;
   if ((ret = cmsObj_set(wanWifiObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not disable WanWifi object");
      cmsObj_free((void **) &wanWifiObj);
      return;
   }         

   if ((ret = rutLan_addWlanInterface(ifName)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add new LAN wl interface, ret=%d", ret);
      cmsObj_free((void **) &wanWifiObj);
      return;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   rutLan_getWlanInterface(ifName, &iidStack, &wlVirtIntfCfgObj);


   /*
    * Transfer settings of the WANWifiInterface object to the
    * LAN WlInterface object.  Don't enable it yet, let upper layer
    * do that when it is ready.
    */
   CMSMEM_REPLACE_STRING_FLAGS(wlVirtIntfCfgObj->wlBssMacAddr, wanWifiObj->MACAddress, mdmLibCtx.allocFlags);
               
   ret = cmsObj_set(wlVirtIntfCfgObj, &iidStack);

   cmsObj_free((void **) &wanWifiObj);
   cmsObj_free((void **) &wlVirtIntfCfgObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set wlVirtIntfCfgObj, ret = %d", ret);
      return;
   }

   /* need to hack here for interface grouping further*/
   {
      UBOOL8 isWanIntf = FALSE;
      UINT32 bridgeRef = 0;

      /*
       * Now that the interface is back on the LAN side, add it as an
       * available interface.  Also add corresponding filter entry.
       */
      rutPMap_addAvailableInterface(ifName, isWanIntf);
      rutPMap_addFilter(ifName, isWanIntf, bridgeRef);
   }

   return;
}

CmsRet rutWifiWan_getWanWifiObject(InstanceIdStack *iidStack,
                                   WanWifiIntfObject **wanWifiIntfObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   _WanCommonIntfCfgObject *wanCommIntf = NULL;
   UINT32 flags = OGF_NO_VALUE_UPDATE;
   UBOOL8 found = FALSE;

   if (iidStack == NULL || wanWifiIntfObj == NULL)
   {
      cmsLog_error("iidStack or wanWifiIntfObj is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(iidStack);
   while (!found &&
          ((ret = cmsObj_getNextFlags(MDMOID_WAN_COMMON_INTF_CFG, iidStack, flags, (void **) &wanCommIntf)) == CMSRET_SUCCESS))
   {
      found = (0 == cmsUtl_strcmp(wanCommIntf->WANAccessType, MDMVS_X_BROADCOM_COM_WIFI));
      cmsObj_free((void **) &wanCommIntf);
   }

   if (!found)
   {
      cmsLog_error("could not find WanWifi object");
      return ret;
   }

   /*
    * WanCommonInterfaceConfig is at the same level as WanWifiInterface config,
    * so once we have found WanCommonInterfaceConfig, we can use the same iidStack
    * to get the WanWifiInterfaceConfig.
    */
   ret = cmsObj_get(MDMOID_WAN_WIFI_INTF, iidStack, 0, (void **) wanWifiIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get wanWifiIntf object, ret=%d", ret);
   }
      
   return ret;
}

void rutWifiWan_getIntfStatus(const char *ifName, char *statusStr)
{

   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WlBaseCfgObject *wlBaseCfgObj=NULL;
   CmsRet ret;
   char cmdStr[BUFLEN_128]={0};
   const char *tmp_wlbssid_filename="/var/wlbssid";
   char buf[BUFLEN_32]={0};
   FILE *fp = NULL;
   char *s=NULL;
    
   sprintf(statusStr,MDMVS_DISABLED);
   sprintf(buf, "00:00:00:00:00:00");
   
   if (ifName == NULL || statusStr==NULL)
   {
      cmsLog_error("ifName or statusStr is NULL");
      return;
   }

   // This URE checking is not data model independent.  Should get moved
   // out to a QDM function.
   iidStack.instance[0] = 1;    // for LANDevice.1
   iidStack.instance[1] = (atoi(&ifName[2]) + 1);  // WLANConfiguration.{i}
   iidStack.currentDepth=2;

   if ((ret = cmsObj_get(MDMOID_WL_BASE_CFG, &iidStack, 0, (void **)&wlBaseCfgObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get wlBaseCfgObj, ret=%d", ret);
      return;
   }

   /*Check URE is enable or not first*/
   if(!wlBaseCfgObj->wlEnableUre) 
   {
      cmsLog_debug("wlEnableUre is not enabled");
      cmsObj_free((void **) &wlBaseCfgObj);
      return;
   }

   /*Check if STA Wl interface associated to AP or not*/
   snprintf(cmdStr, sizeof(cmdStr), "wlctl -i %s  bssid > %s 2>/dev/null",
            ifName, tmp_wlbssid_filename);
   rut_doSystemAction("rut", cmdStr);

   fp = fopen(tmp_wlbssid_filename, "r");
   if(fp != NULL ) 
   {
       /* I think buf contains the mac address of the AP.  Too bad we
        * do not save this information somewhere.
        */
       s = fgets( buf, sizeof(buf)-1, fp );
       fclose(fp);
   }
   
   if ((s != NULL) && strncmp(buf, "00:00:00:00:00:00",17))
   {
       sprintf(statusStr,MDMVS_UP);
   }

   unlink(tmp_wlbssid_filename);

   cmsObj_free((void **) &wlBaseCfgObj);

   return;
}

#endif /*DMP_X_BROADCOM_COM_WIFIWAN_1*/
