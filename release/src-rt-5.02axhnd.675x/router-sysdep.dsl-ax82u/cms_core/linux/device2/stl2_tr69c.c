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

#include "stl.h"
#include "cms_util.h"
#include "rut_lan.h"
#include "rut_wan.h"
#include "qdm_ipintf.h"

#ifdef SUPPORT_TR69C
/* because this object is a baseline object, it is mostly included.  And if TR69C is disabled, the functions 
 * should be empty.
 */
CmsRet stl_dev2ManagementServerObject(_Dev2ManagementServerObject *obj,
      const InstanceIdStack *iidStack __attribute((unused)))
{
   char url[BUFLEN_256], dateTimeBuf[BUFLEN_64];
   char ifName[CMS_IFNAME_LENGTH]={0};
   char ipAddress[CMS_IPADDR_LENGTH]={0};
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   /* the body of stl_managementServerObject and stl_dev2ManagementServerObject looks
    * exactly the same.  Consolidate into a single function?
    */
#ifdef OMCI_TR69_DUAL_STACK
   UBOOL8 isIPv4 = TRUE;
   char *pIpAddress = NULL;
   if ((ret = rutOmci_getIpHostAddress(obj->X_BROADCOM_COM_BoundIfName, &pIpAddress, &isIPv4)) == CMSRET_SUCCESS)
   {
         if (isIPv4)
         {
            snprintf(url, sizeof(url), "http://%s:%d%s", pIpAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
         }
         else
         {
            snprintf(url, sizeof(url), "http://[%s]:%d%s", pIpAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
         }
         CMSMEM_REPLACE_STRING_FLAGS(obj->connectionRequestURL, url, mdmLibCtx.allocFlags);
         CMSMEM_FREE_BUF_AND_NULL_PTR(pIpAddress);
   }
   else
#endif
   if (cmsUtl_strcmp(obj->X_BROADCOM_COM_BoundIfName, MDMVS_LOOPBACK) == 0)
   {
      snprintf(url, sizeof(url), "http://127.0.0.1:%d%s", TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
      CMSMEM_REPLACE_STRING_FLAGS(obj->connectionRequestURL, url, mdmLibCtx.allocFlags);
   }
   else if (cmsUtl_strcmp(obj->X_BROADCOM_COM_BoundIfName, MDMVS_LAN) == 0)
   {
#if defined(SUPPORT_UPNP_IGD_HTTP_CONNREQ) 
      if (obj->X_BROADCOM_COM_UPNPC_IGD_WAN_ADDRESS != NULL)
      {
         char *urlAddr = NULL, *urlPath = NULL;
         UrlProto urlProto;
         UINT16 urlPort;

         if ((obj->connectionRequestURL != NULL) &&
            ((ret = cmsUtl_parseUrl(obj->connectionRequestURL, &urlProto, &urlAddr, &urlPort, &urlPath)) == CMSRET_SUCCESS))
         {
            if (strcmp(urlAddr, obj->X_BROADCOM_COM_UPNPC_IGD_WAN_ADDRESS) || 
                urlPort != obj->X_BROADCOM_COM_UPNPC_IGD_EXT_PORT)
            {
               snprintf(url, sizeof(url), "http://%s:%d%s", obj->X_BROADCOM_COM_UPNPC_IGD_WAN_ADDRESS, obj->X_BROADCOM_COM_UPNPC_IGD_EXT_PORT, urlPath);
               CMSMEM_REPLACE_STRING_FLAGS(obj->connectionRequestURL, url, mdmLibCtx.allocFlags);
            }   
         }
         else
         {
            snprintf(url, sizeof(url), "http://%s:%d%s", obj->X_BROADCOM_COM_UPNPC_IGD_WAN_ADDRESS, obj->X_BROADCOM_COM_UPNPC_IGD_EXT_PORT, TR69C_CONN_REQ_PATH);
            CMSMEM_REPLACE_STRING_FLAGS(obj->connectionRequestURL, url, mdmLibCtx.allocFlags);
         }

         cmsMem_free(urlAddr);
         cmsMem_free(urlPath);
     }
#else
      /* LAN side is for testing only, use IPv4 only to make life simple */
      if (qdmIpIntf_getDefaultLanIntfNameLocked_dev2(ifName) == CMSRET_SUCCESS &&
          qdmIpIntf_isIpv4ServiceUpLocked_dev2(ifName, QDM_IPINTF_DIR_LAN))
      {
         if (qdmIpIntf_getIpvxAddressByNameLocked_dev2(CMS_AF_SELECT_IPV4, ifName, ipAddress) == CMSRET_SUCCESS)
         {
            snprintf(url, sizeof(url), "http://%s:%d%s", ipAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
            CMSMEM_REPLACE_STRING_FLAGS(obj->connectionRequestURL, url, mdmLibCtx.allocFlags);
            ret = CMSRET_SUCCESS;
         }
      }
#endif
   }
   else if (cmsUtl_strcmp(obj->X_BROADCOM_COM_BoundIfName, MDMVS_ANY_WAN) == 0)
   {
      UBOOL8 isAnyIntfUp;

      isAnyIntfUp = rutWan_findFirstIpvxRoutedAndConnected(CMS_AF_SELECT_IPVX, ifName);
      if (!isAnyIntfUp)
      {
         /*
          * This is no big deal, it just means that no WAN interface is up yet.
          */
         cmsLog_debug("cannot set connectionRequestURL yet because no WAN intf is up");
      }
      else
      {
         ret = qdmIpIntf_getIpvxAddressByNameLocked(CMS_AF_SELECT_IPVX,
                                                    ifName, ipAddress);
         if (ret != CMSRET_SUCCESS)
         {
            /*
             * Log the error, but reset the ret to CMSRET_SUCESS so that
             * this function does not indicate fatal error.
            */
            cmsLog_error("Could not get IPAddr for %s even though it is UP", ifName);
            ret = CMSRET_SUCCESS;
         }
         else
         {
            if (cmsUtl_isValidIpv4Address(ipAddress))
            {
               snprintf(url, sizeof(url), "http://%s:%d%s", ipAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
            }
            else
            {
               cmsUtl_truncatePrefixFromIpv6AddrStr(ipAddress);
               snprintf(url, sizeof(url), "http://[%s]:%d%s", ipAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
            }
            
            CMSMEM_REPLACE_STRING_FLAGS(obj->connectionRequestURL, url, mdmLibCtx.allocFlags);
         }
      }
   }
   else
   {
      /*
       * We are bound to a specific ifName.  First make sure it is UP, and
       * if it is, get IPAddress.
       */
      if (!qdmIpIntf_isWanInterfaceUpLocked(obj->X_BROADCOM_COM_BoundIfName, TRUE) &&
          !qdmIpIntf_isWanInterfaceUpLocked(obj->X_BROADCOM_COM_BoundIfName, FALSE))
      {
         /*
          * This is no big deal, it just means the specified WAN interface is not up yet.
          * Reset the ret to success so this function does not indicate error.
          */
         cmsLog_debug("cannot set connectionRequestURL yet because %s is not up yet", obj->X_BROADCOM_COM_BoundIfName);
         ret = CMSRET_SUCCESS;
      }
      else
      {
         ret = qdmIpIntf_getIpvxAddressByNameLocked(CMS_AF_SELECT_IPVX,
                                 obj->X_BROADCOM_COM_BoundIfName, ipAddress);
         if (ret != CMSRET_SUCCESS)
         {
            /*
             * Log the error, but reset the ret to CMSRET_SUCESS so that
             * this function does not indicate fatal error.
            */
            cmsLog_error("Could not get IPAddr for %s even though it is UP", ifName);
            ret = CMSRET_SUCCESS;
         }
         else
         {
            if (cmsUtl_isValidIpv4Address(ipAddress))
            {
               snprintf(url, sizeof(url), "http://%s:%d%s", ipAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
            }
            else
            {
               cmsUtl_truncatePrefixFromIpv6AddrStr(ipAddress);
               snprintf(url, sizeof(url), "http://[%s]:%d%s", ipAddress, TR69C_CONN_REQ_PORT, TR69C_CONN_REQ_PATH);
            }

            CMSMEM_REPLACE_STRING_FLAGS(obj->connectionRequestURL, url, mdmLibCtx.allocFlags);
         }
      }
   }

   /*
    * parameterKey should be updated by tr69c during protocol processing.
    * Handler function does not need to generate it or do anything with it.
    */


   /*
    * Set informtime to 0.  Does this get updated somewhere else?
    */
   cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
   CMSMEM_REPLACE_STRING_FLAGS(obj->periodicInformTime, dateTimeBuf, mdmLibCtx.allocFlags);
   if (obj->periodicInformTime == NULL)
   {
      cmsLog_error("insufficient memory to strdup");
      return CMSRET_RESOURCE_EXCEEDED;
   }
   else
   {
      ret = CMSRET_SUCCESS;
   }

   return ret;
}


#ifdef DMP_DEVICE2_DEVICEASSOCIATION_1
CmsRet stl_dev2ManagementServerManageableDeviceObject(_Dev2ManagementServerManageableDeviceObject *obj __attribute__((unused)),
   const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE.  This handler function does not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif  /* DMP_DEVICE2_DEVICEASSOCIATION_1 */

#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE 
CmsRet stl_dev2AutonXferCompletePolicyObject(_Dev2AutonXferCompletePolicyObject *obj __attribute__((unused)),
                                             const InstanceIdStack *iidStack __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}
#endif /* SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE */

#ifdef DMP_DEVICE2_DUSTATECHNGCOMPLPOLICY_1
CmsRet stl_dev2DuStateChangeComplPolicyObject(_Dev2DuStateChangeComplPolicyObject *obj __attribute__((unused)), 
                                              const InstanceIdStack *iidStack __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}
#endif /* DMP_DEVICE2_DUSTATECHNGCOMPLPOLICY_1 */

#else /* SUPPORT_TR69C */

#ifndef SUPPORT_RETAIL_DM
/* retail data model has even the management object stripped from baseline profile */
CmsRet stl_dev2ManagementServerObject(_Dev2ManagementServerObject *obj,
      const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

#ifdef DMP_DEVICE2_DEVICEASSOCIATION_1
CmsRet stl_dev2ManagementServerManageableDeviceObject(_Dev2ManagementServerManageableDeviceObject *obj __attribute__((unused)),
   const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE.  This handler function does not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif  /* DMP_DEVICE2_DEVICEASSOCIATION_1 */

#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE 
CmsRet stl_dev2AutonXferCompletePolicyObject(_Dev2AutonXferCompletePolicyObject *obj __attribute__((unused)),
                                             const InstanceIdStack *iidStack __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}
#endif /* SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE */

#ifdef DMP_DEVICE2_DUSTATECHNGCOMPLPOLICY_1
CmsRet stl_dev2DuStateChangeComplPolicyObject(_Dev2DuStateChangeComplPolicyObject *obj __attribute__((unused)), 
                                              const InstanceIdStack *iidStack __attribute__((unused)))
{
   return (CMSRET_SUCCESS);
}
#endif /* DMP_DEVICE2_DUSTATECHNGCOMPLPOLICY_1 */


#endif /* SUPPORT_TR69C */


#endif  /* DMP_DEVICE2_BASELINE_1 */

