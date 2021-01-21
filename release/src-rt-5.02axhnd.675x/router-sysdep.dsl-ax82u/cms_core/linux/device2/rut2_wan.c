/***********************************************************************
 *
 *  Copyright (c) 2009-2013  Broadcom Corporation
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



#include "cms_core.h"
#include "cms_util.h"
#include "rut_wan.h"
#include "cms_qdm.h"
#include "rut_ebtables.h"



UBOOL8 rutWan_findFirstRoutedAndConnected_dev2(char *ifName)
{
   return (rutWan_findFirstIpvxRoutedAndConnected_dev2(CMS_AF_SELECT_IPV4, ifName));
}


UBOOL8 rutWan_findFirstIpvxRoutedAndConnected_dev2(UINT32 ipvx, char *ifName)
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;


   cmsLog_debug("Enter: ipvx=%d", ipvx);
   ifName[0] = '\0';

   /*
    * Loop through all IP.Interface entries looking for an Upstream,
    * and IPv4ServiceStatus == SERVICEUP or IPv6ServiceStatus == SERVICEUP,
    * depending on ipvx.
    */
   while (!found &&
          cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &ipIntfObj) == CMSRET_SUCCESS)
   {
#ifdef SUPPORT_HOMEPLUG
      /* For homeplug, br0 is LAN ip interface and both X_BROADCOM_COM_BridgeService (flag for WAN bridge service)
      * X_BROADCOM_COM_Upstream is always FALSE.
      */
      if (!cmsUtl_strcmp(ipIntfObj->name, "br0") &&
#else      
      if (ipIntfObj->X_BROADCOM_COM_Upstream &&
#endif /* SUPPORT_HOMEPLUG */

          !ipIntfObj->X_BROADCOM_COM_BridgeService)
      {
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
         if ((ipvx & CMS_AF_SELECT_IPV6) &&
             !cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IPv6ServiceStatus, MDMVS_SERVICEUP))
         {
            found = TRUE;
            strcpy(ifName, ipIntfObj->name);
            cmsLog_debug("found IPv6 routed and connected ifName %s", ifName);
         }
#endif

         if (!found &&
             (ipvx & CMS_AF_SELECT_IPV4) &&
             !cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus, MDMVS_SERVICEUP))
         {
            found = TRUE;
            strcpy(ifName, ipIntfObj->name);
            cmsLog_debug("found IPv4 routed and connected ifName %s", ifName);
         }
      }

      cmsObj_free((void **) &ipIntfObj);
   }

   cmsLog_debug("Exit: found=%d ifName=%s", found, ifName);

   return found;
}


UBOOL8 rutWan_findFirstNattedAndConnected_dev2(char *natIntfName, const char *excludeIntfName)
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;

   cmsLog_debug("Entered: excludeIntfName=%s", excludeIntfName);

   natIntfName[0] = '\0';

   /*
    * Loop through all IP.Interface entries looking for an Upstream,
    * and IPv4ServiceStatus == SERVICEUP and
    * depending on ipvx.
    */
   while (!found &&
          cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &ipIntfObj) == CMSRET_SUCCESS)
   {
      if ((ipIntfObj->X_BROADCOM_COM_Upstream) &&
          (!ipIntfObj->X_BROADCOM_COM_BridgeService) &&
          (!cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IPv4ServiceStatus, MDMVS_SERVICEUP)) &&
          (cmsUtl_strcmp(ipIntfObj->name, excludeIntfName)) &&
          (qdmIpIntf_isNatEnabledOnIntfNameLocked(ipIntfObj->name)))
      {
         found = TRUE;
         strcpy(natIntfName, ipIntfObj->name);
      }

      cmsObj_free((void **) &ipIntfObj);
   }

   cmsLog_debug("Exit: found=%d natIntfName=%s", found, natIntfName);

   return found;
}


CmsRet rutWan_getDhcpDeviceInfo_dev2(char *oui, char *serialNum, char *productClass)
{
   Dev2DeviceInfoObject *deviceInfoObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = cmsObj_get(MDMOID_DEV2_DEVICE_INFO, &iidStack, 0, (void **) &deviceInfoObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get device info object!, ret=%d", ret);
   }
   else
   {
      if (oui && deviceInfoObj->manufacturerOUI)
         sprintf(oui, "%s", deviceInfoObj->manufacturerOUI);
      if (serialNum && deviceInfoObj->serialNumber)
         sprintf(serialNum, "%s", deviceInfoObj->serialNumber);
      if (productClass && deviceInfoObj->productClass)
         sprintf(productClass, "%s", deviceInfoObj->productClass);

      cmsObj_free((void**)&deviceInfoObj);
   }

   return ret;
}


CmsRet rutWan_fillInPPPIndexArray_dev2(SINT32 *intfArray)
{
   SINT32 index = 0;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _Dev2PppInterfaceObject *pppIntf;

   while (cmsObj_getNextFlags(MDMOID_DEV2_PPP_INTERFACE, &iidStack, OGF_NORMAL_UPDATE, (void **) &pppIntf) == CMSRET_SUCCESS)
   {
      if (pppIntf->name == NULL)
      {
         cmsLog_debug("This is one in creating so ifName is NULL, continue...");
         /* this is one we just created and is NULL, so just skip it */
      }
      else
      {
         index = atoi(&(pppIntf)->name[cmsUtl_strlen(PPP_IFC_STR)]);

         cmsLog_debug("pppIntf->name =%s, index=%d", pppIntf->name , index);

         if (index > IFC_WAN_MAX)
         {
            cmsLog_error("Only %d interface allowed", IFC_WAN_MAX);
            cmsObj_free((void **) &pppIntf);
            return CMSRET_INTERNAL_ERROR;
         }
         *(intfArray+index) = 1;            /* mark the interface used */
      }
      cmsObj_free((void **) &pppIntf);
      
   }

   return CMSRET_SUCCESS;
   
}


CmsRet rutWan_initPPPoE_dev2(const InstanceIdStack *iidStack __attribute__((unused)), void *obj)
{
   SINT32 pppPid;
   char cmdLine[BUFLEN_128];
   char serverFlag[BUFLEN_32];
   char staticIPAddrFlag[BUFLEN_32];
   char passwordFlag[BUFLEN_32];
   passwordFlag[0] = serverFlag[0] = staticIPAddrFlag[0] = '\0';
   CmsRet ret=CMSRET_SUCCESS;
   InstanceIdStack pppoeIidStack = EMPTY_INSTANCE_ID_STACK;
   _Dev2PppInterfacePpoeObject *pppoeObj = NULL;
   char baseL3IfName[CMS_IFNAME_LENGTH]={0};
   _Dev2PppInterfaceObject *newObj = (_Dev2PppInterfaceObject *) obj;
   
   cmsLog_debug("Enter");

   /* From lowerlayer full path, get the lowerlayer ifName (baseL3IfName) */
   if (IS_EMPTY_STRING(newObj->lowerLayers))
   {
      cmsLog_error("PPP interface is not pointing to lowerlayer object!");
      return CMSRET_INTERNAL_ERROR;
   }      

   if (rut_wanGetIntfIndex(newObj->name) > 0)
   {
      cmsLog_debug("ppp interface is already created");
      return ret;
   }

   if ((ret = qdmIntf_getIntfnameFromFullPathLocked_dev2(newObj->lowerLayers, 
                                                         baseL3IfName, 
                                                         sizeof(baseL3IfName))) != CMSRET_SUCCESS)
   {
      cmsLog_error("qdmIntf_getIntfnameFromFullPathLocked_dev2 failed. ret %d", ret);
      return ret;
   }
   
   if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PPP_INTERFACE_PPOE, 
                                          iidStack, 
                                          &pppoeIidStack,
                                          (void **) &pppoeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get pppoeObj, ret=%d", ret);
      return ret;
   }


  /* password is an optional parameter */
   if (newObj->password && newObj->password[0] != '\0')
   {
      snprintf(passwordFlag, sizeof(passwordFlag), "-p %s", newObj->password);
   }

   /*
    *  server name (on web display it is service name)
    * The -r argument is passed to pppoe.  So it cannot be an empty string,
    * and it cannot be a string containing any white spaces.  The check below
    * only guards against empty string, but it should also guard against white
    * space.  So far, this has not been a problem.
    */

   if ((pppoeObj->serviceName != NULL) && (strlen(pppoeObj->serviceName ) > 0))
   {
      snprintf(serverFlag, sizeof(serverFlag), "-r %s", pppoeObj->serviceName);
   }

   /* static IP address */
// for IPV4 only, check ipcp for IPv4 ?   if (newObj->X_BROADCOM_COM_IPv4Enabled && newObj->X_BROADCOM_COM_UseStaticIPAddress)
   if (newObj->X_BROADCOM_COM_UseStaticIPAddress)
   {
      snprintf(staticIPAddrFlag, sizeof(staticIPAddrFlag), "-A %s", newObj->X_BROADCOM_COM_LocalIPAddress);
   }
   snprintf(cmdLine, sizeof(cmdLine), "%s %s -i %s -u %s %s -f %d %s",
                                       newObj->name, 
                                       serverFlag, 
                                       baseL3IfName,
                                       newObj->username, 
                                       passwordFlag,
                                       cmsUtl_pppAuthToNum(newObj->authenticationProtocol), 
                                       staticIPAddrFlag); 

   /* enable ppp debugging if it is selected */
   if (newObj->X_BROADCOM_COM_Enable_Debug)
   {
      strncat(cmdLine, " -d", sizeof(cmdLine)-1);
   }

   /* IP extension */
   if (pppoeObj->X_BROADCOM_COM_IPExtension)
   {
      strncat(cmdLine, " -x", sizeof(cmdLine)-1);
   }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1 /* aka SUPPORT_IPV6 */
   /* IPv6 ??? */
   if (newObj->IPv6CPEnable)
   {
      strncat(cmdLine, " -6", sizeof(cmdLine)-1);
   }

   /* disable IPCP if it's IPv6 only */
   if (!newObj->IPCPEnable)
   {
      strncat(cmdLine, " -4", sizeof(cmdLine)-1);
   }
#endif

   ret = rutWan_configPPPoE(cmdLine, baseL3IfName, pppoeObj->X_BROADCOM_COM_AddPppToBridge, &pppPid);
   cmsObj_free((void **) &pppoeObj);
   
   if (ret == CMSRET_SUCCESS)
   {
      newObj->X_BROADCOM_COM_Pid = pppPid;
   }

   cmsLog_debug("ret %d", ret);
   
   return ret;

}

   
CmsRet rutWan_cleanUpPPPoE_dev2(const InstanceIdStack *iidStack, const void *obj)
{
   char baseL3IfName[CMS_IFNAME_LENGTH]={0};
   CmsRet ret=CMSRET_SUCCESS;
   _Dev2PppInterfaceObject *currObj = (_Dev2PppInterfaceObject *) obj;
   InstanceIdStack pppoeIidStack = EMPTY_INSTANCE_ID_STACK;
   _Dev2PppInterfacePpoeObject *pppoeObj = NULL;
   UINT32 specificEid;
   
   /* Need baseL3IfName for PPPoE */
   /* From lowerlayer full path, get the lowerlayer ifName (baseL3IfName) */
   if (IS_EMPTY_STRING(currObj->lowerLayers))
   {
      cmsLog_error("PPP interface is not pointing to lowerlayer object!");
      return CMSRET_INTERNAL_ERROR;
   }      

   if ((ret = qdmIntf_getIntfnameFromFullPathLocked_dev2(currObj->lowerLayers, 
                                                         baseL3IfName, 
                                                         sizeof(baseL3IfName))) != CMSRET_SUCCESS)
   {
      cmsLog_error("qdmIntf_getIntfnameFromFullPathLocked_dev2 failed. ret %d", ret);
      return ret;
   }
   
   if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PPP_INTERFACE_PPOE, 
                                          iidStack, 
                                          &pppoeIidStack,
                                          (void **) &pppoeObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get pppoeObj, ret=%d", ret);
      return ret;
   }


   specificEid = MAKE_SPECIFIC_EID(currObj->X_BROADCOM_COM_Pid, EID_PPP); 

   if (pppoeObj->X_BROADCOM_COM_AddPppToBridge && rut_isApplicationRunning(specificEid) == FALSE)
   {
      rutEbt_removePppIntfFromBridge(baseL3IfName);
   }
   
   cmsObj_free((void **) &pppoeObj);

   return ret;
}

#ifdef DMP_DEVICE2_OPTICAL_1
UBOOL8 rutOptical_getIntfByIfName(const char *ifName, InstanceIdStack *iidStack, OpticalInterfaceObject **optIntfObj)
{
    OpticalInterfaceObject *mdmObj = NULL;
    int found = FALSE;

    if (optIntfObj == NULL)
    {
        cmsLog_error("optIntfObj is NULL");
        return FALSE;
    }

    while (cmsObj_getNextFlags(MDMOID_OPTICAL_INTERFACE, iidStack, OGF_NO_VALUE_UPDATE, (void **) &mdmObj) == CMSRET_SUCCESS)
    {
        if (cmsUtl_strcmp(mdmObj->name, ifName) == 0)
        {
            *optIntfObj = mdmObj;
            found = TRUE;
            break;
        }

        cmsObj_free((void **) &mdmObj);
    }

    return found;
} 

UBOOL8 rutOptical_getIntfByIfNameEnabled(const char *ifName, InstanceIdStack *iidStack, OpticalInterfaceObject **optIntfObj, UBOOL8 enabled)
{
    OpticalInterfaceObject *mdmObj = NULL;

    if (rutOptical_getIntfByIfName(ifName, iidStack, &mdmObj) == FALSE)
        return FALSE;

    if (mdmObj->enable == enabled)
        *optIntfObj = mdmObj;
    else
        cmsObj_free((void **) &mdmObj);

    return *optIntfObj != NULL;
}
#endif /* DMP_DEVICE2_OPTICAL_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */

