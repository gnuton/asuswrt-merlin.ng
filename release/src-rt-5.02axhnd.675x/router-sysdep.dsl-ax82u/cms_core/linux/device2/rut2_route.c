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

#ifdef DMP_DEVICE2_ROUTING_1

#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_route.h"
#include "rut2_route.h"
#include "cms_mdm.h"
#include "cms_obj.h"
#include "cms_phl.h"
#include "cms_qdm.h"
#include "cms_util.h"




/* This is the TR181 version of rutRt_fetchActiveDefaultGateway */
void rutRt_selectActiveIpvxDefaultGateway_dev2(UINT32 ipvx,
                                               const char *gatewayList,
                                               char *defaultGateway)
{
   UBOOL8 found = FALSE;

   cmsLog_debug("Entered: list=%s ipvx=%d", gatewayList, ipvx);

   defaultGateway[0] = '\0';

   if ((ipvx & CMS_AF_SELECT_IPVX) == CMS_AF_SELECT_IPVX)
   {
      cmsLog_error("Must choose IPv4 or IPv6, but not both");
      return;
   }

   /* this block works for both IPv4 and IPv6 */
   if (gatewayList != NULL)
   {
      char *tmpList, *ptr, *savePtr=NULL;
      UINT32 count=0;

      tmpList = cmsMem_strdup(gatewayList);
      ptr = strtok_r(tmpList, ",", &savePtr);

      /* need to find an interface which is in SERVICEUP state */
      while (!found && (ptr != NULL) && (count < CMS_MAX_DEFAULT_GATEWAY))
      {
         while ((isspace(*ptr)) && (*ptr != 0))
         {
            /* skip white space after comma */
            ptr++;
         }

         if (*ptr != 0)
         {
            cmsLog_debug("checking %s", ptr);
            /* Look for both WAN and LAN side since Homeplug may get
             * default gateway on LAN side.
             */
            if (qdmIpIntf_isIpvxServiceUpLocked_dev2(ptr,
                                                   QDM_IPINTF_DIR_ANY, ipvx))
            {
               strcpy(defaultGateway, ptr);
               cmsLog_debug("ifName %s is SERVICEUP (ipxv=%d)", ptr, ipvx);
               found = TRUE;
            }
         }

         count++;
         ptr = strtok_r(NULL, ",", &savePtr);
      }

      cmsMem_free(tmpList);
   }


   if (!found)
   {
      cmsLog_debug("No gateways from list, check other forwarding entries");
      if (ipvx & CMS_AF_SELECT_IPV4)
      {
         /*
          * Look for any IPv4ForwardingObject.  This will find WAN and LAN
          * interfaces.
          */
         InstanceIdStack forwardingIidStack = EMPTY_INSTANCE_ID_STACK;
         Dev2Ipv4ForwardingObject *ipv4ForwardingObj=NULL;

         while(!found &&
               cmsObj_getNextFlags(MDMOID_DEV2_IPV4_FORWARDING,
                                   &forwardingIidStack,
                                   OGF_NO_VALUE_UPDATE,
                                   (void **) &ipv4ForwardingObj) == CMSRET_SUCCESS)
         {
            cmsLog_debug("got IPv4Forwarding %s origin=%s ipIntf=%s",
                         cmsMdm_dumpIidStack(&forwardingIidStack),
                         ipv4ForwardingObj->origin,
                         ipv4ForwardingObj->interface);
            if (!IS_EMPTY_STRING(ipv4ForwardingObj->interface))
            {                         
               if (cmsUtl_strcmp(ipv4ForwardingObj->origin, MDMVS_STATIC))
               {
                  /* for non-static entries, if it is in the MDM, it is assumed
                   * to be UP because if the intf went down, it would be deleted.
                   */
                  qdmIntf_fullPathToIntfnameLocked(ipv4ForwardingObj->interface, defaultGateway);
                  found = TRUE;
               }
               else
               {
                  /* for static entries, we need to verify it is a default route
                   * (destIPAddress not set) and the interface is actually UP.
                   */
                  if (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, ipv4ForwardingObj->destIPAddress))
                  {
                     char statusBuf[BUFLEN_64]={0};

                     qdmIpIntf_getIpvxServiceStatusFromFullPathLocked_dev2(
                                                   ipv4ForwardingObj->interface,
                                                   CMS_AF_SELECT_IPV4,
                                                   statusBuf, sizeof(statusBuf));

                     if (!cmsUtl_strcmp(statusBuf, MDMVS_SERVICEUP))
                     {
                        qdmIntf_fullPathToIntfnameLocked(ipv4ForwardingObj->interface, defaultGateway);
                        found = TRUE;
                     }
                  }
               }
            }
            cmsObj_free((void *) &ipv4ForwardingObj);
         }
      }

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      if (ipvx & CMS_AF_SELECT_IPV6)
      {
         /*
          * Look for any IPv6ForwardingObject.  This will find WAN and LAN
          * interfaces.
          */
         InstanceIdStack forwardingIidStack = EMPTY_INSTANCE_ID_STACK;
         Dev2Ipv6ForwardingObject *ipv6ForwardingObj=NULL;

         while(!found &&
               cmsObj_getNextFlags(MDMOID_DEV2_IPV6_FORWARDING,
                                   &forwardingIidStack,
                                   OGF_NO_VALUE_UPDATE,
                                   (void **) &ipv6ForwardingObj) == CMSRET_SUCCESS)
         {
            cmsLog_debug("got IPv6Forwarding %s origin=%s ipIntf=%s",
                         cmsMdm_dumpIidStack(&forwardingIidStack),
                         ipv6ForwardingObj->origin,
                         ipv6ForwardingObj->interface);
            if (!IS_EMPTY_STRING(ipv6ForwardingObj->interface))
            {
               if (cmsUtl_strcmp(ipv6ForwardingObj->origin, MDMVS_STATIC))
               {
                  /* for non-static entries, if it is in the MDM, it is assumed
                   * to be UP because if the intf went down, it would be deleted.
                   */
                  qdmIntf_fullPathToIntfnameLocked(ipv6ForwardingObj->interface, defaultGateway);
                  found = TRUE;
               }
               else
               {
                  /* for static entries, we need to verify it is a default route
                   * (destIPPrefix not set) and the interface is actually UP.
                   */
                  if (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, ipv6ForwardingObj->destIPPrefix))
                  {
                     char statusBuf[BUFLEN_64]={0};

                     qdmIpIntf_getIpvxServiceStatusFromFullPathLocked_dev2(
                                                   ipv6ForwardingObj->interface,
                                                   CMS_AF_SELECT_IPV6,
                                                   statusBuf, sizeof(statusBuf));

                     if (!cmsUtl_strcmp(statusBuf, MDMVS_SERVICEUP))
                     {
                        qdmIntf_fullPathToIntfnameLocked(ipv6ForwardingObj->interface, defaultGateway);
                        found = TRUE;
                     }
                  }
               }
            }
            cmsObj_free((void *) &ipv6ForwardingObj);
         }
      }
#endif  /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */
   }

   cmsLog_debug("Exit: ActiveDefaultGateway=%s (ipvx=0x%x)",
                 defaultGateway, ipvx);

   return;
}


CmsRet rutRt_activateIpv4Routing_dev2(const char *ipIntfFullPath)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack routerIidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2RouterObject *routerObj=NULL;


   cmsLog_debug("Enter: ipIntfFullPath=%s", ipIntfFullPath);


   /*
    * Don't know why TR181 defines multiple router objects.  Just get the
    * first router object.
    */
   if ((ret = cmsObj_getNext(MDMOID_DEV2_ROUTER, &routerIidStack, (void **)&routerObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get MDMOID_DEV2_ROUTER Instance, ret = %d", ret);
      return ret;
   }

   /* just do a set and let the RCL handler func select default gateway */
   ret = cmsObj_set((void *) routerObj, &routerIidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Set of router obj failed, ret=%d", ret);
   }

   cmsObj_free((void *) &routerObj);


   /*
    * Now do a "set" on all:
    * (1) static
    * (2) non-default
    * (3) IPv4Forwarding objs which point to the interface which just came up.
    * This allows them to configure their route.
    */
   {
      InstanceIdStack forwardingIidStack=EMPTY_INSTANCE_ID_STACK;
      Dev2Ipv4ForwardingObject *ipv4ForwardingObj=NULL;

      while(cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV4_FORWARDING,
                                &routerIidStack,
                                &forwardingIidStack,
                                OGF_NO_VALUE_UPDATE,
                                (void **) &ipv4ForwardingObj) == CMSRET_SUCCESS)
      {
         if (((!cmsUtl_strcmp(ipv4ForwardingObj->origin, MDMVS_STATIC) 
                && !cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, ipv4ForwardingObj->destIPAddress))
#ifdef SUPPORT_POLICYROUTING
                || (!IS_EMPTY_STRING(ipv4ForwardingObj->X_BROADCOM_COM_PolicyRoutingName))
#endif             
              ) && (IS_EMPTY_STRING(ipv4ForwardingObj->interface) || !cmsUtl_strcmp(ipv4ForwardingObj->interface, ipIntfFullPath)))
         {
            if ((ret = cmsObj_set(ipv4ForwardingObj, &forwardingIidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsObj_set <MDMOID_DEV2_IPV4_FORWARDING> returns error. ret=%d", ret);
            }
         }

         cmsObj_free((void **)&ipv4ForwardingObj);
      }
   }

   cmsLog_debug("Exit. ret=%d", ret);

   return ret;
}


void rutRt_deactivateIpv4Routing_dev2(const char *ipIntfFullPath)
{
   InstanceIdStack routerIidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2RouterObject *routerObj=NULL;
   CmsRet ret;

   cmsLog_debug("Enter: ipIntfFullPath=%s", ipIntfFullPath);

   /*
    * Don't know why TR181 defines multiple router objects.  Just get the
    * first router object.
    */
   if ((ret = cmsObj_getNext(MDMOID_DEV2_ROUTER, &routerIidStack, (void **)&routerObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get MDMOID_DEV2_ROUTER Instance, ret = %d", ret);
      return;
   }

   /* just need routerIidStack right now, so free obj */
   cmsObj_free((void **) &routerObj);


   /*
    * Go through all IPv4Forwarding objects associated with this IP
    * interface.  For the dynamically created entries, just delete.
    * For the static entries, do a set.
    */
   {
      InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
      InstanceIdStack savedIidStack=EMPTY_INSTANCE_ID_STACK;
      Dev2Ipv4ForwardingObject *ipv4ForwardingObj=NULL;

      while(cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV4_FORWARDING,
                                &routerIidStack,
                                &iidStack,
                                OGF_NO_VALUE_UPDATE,
                                (void **) &ipv4ForwardingObj) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(ipv4ForwardingObj->interface, ipIntfFullPath))
         {
            if (cmsUtl_strcmp(ipv4ForwardingObj->origin, MDMVS_STATIC))
            {
               /* if this is not a static entry, just delete it.
                * Deleting has the same effect as disable.
                */
               cmsObj_deleteInstance(MDMOID_DEV2_IPV4_FORWARDING, &iidStack);

               /* since we are deleting while traversing, must restore to
                * last good/known instance.
                */
               iidStack = savedIidStack;
            }
            else
            {
               /* if this is a static entry, do a "set" without any changes
                * to the object to allow it to react to the fact that the
                * IP interface that it is pointing to went down.
                */
               if ((ret = cmsObj_set(ipv4ForwardingObj, &iidStack)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("cmsObj_set <MDMOID_DEV2_IPV4_FORWARDING> returns error. ret=%d", ret);
               }
            }
         }

         savedIidStack = iidStack;

         cmsObj_free((void **)&ipv4ForwardingObj);
      }
   }


   /*
    * Now that the dynamic forwarding entries for this interface has been
    * deleted, get the routerObj again (to get updated numberOfEntries) and
    * do a set and let the RCL handler func select default gateway
    */
   cmsObj_get(MDMOID_DEV2_ROUTER, &routerIidStack, 0, (void **)&routerObj);
   ret = cmsObj_set(routerObj, &routerIidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Set of router obj failed, ret=%d", ret);
   }

   cmsObj_free((void **) &routerObj);


   cmsLog_debug("Exit");

   return;
}


CmsRet rutRt_addIpv4ForwardingObject_dev2(const char *ipIntfFullPath,
                                          const char *gatewayIPAddr,
                                          const char *origin)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2RouterObject *routerObj=NULL;
   Dev2Ipv4ForwardingObject *ipv4ForwardingObj=NULL;
   CmsRet ret;

   cmsLog_debug("Enter: ipIntfFullPath=%s gatewayIPAddr=%s origin=%s",
                ipIntfFullPath, gatewayIPAddr, origin);

   if (!cmsUtl_strcmp(origin, MDMVS_STATIC))
   {
      cmsLog_error("This function cannot be used for static routes!");
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (IS_EMPTY_STRING(ipIntfFullPath))
   {
      cmsLog_error("fullpath is empty!");
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, gatewayIPAddr))
   {
      cmsLog_error("empty or zero gatewayIpAddr! (%s)", gatewayIPAddr);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /*
    * Don't know why TR181 defines multiple router objects.  Just get the
    * first router object and add the IPv4Forwarding entry under that one.
    * Router object is created in mdm2_init.c
    */
   if ((ret = cmsObj_getNext(MDMOID_DEV2_ROUTER, &iidStack, (void **)&routerObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get MDMOID_DEV2_ROUTER Instance, ret = %d", ret);
      return ret;
   }
   /* we only want the iidStack, not the object */
   cmsObj_free((void *) &routerObj);

   if ((ret = cmsObj_addInstance(MDMOID_DEV2_IPV4_FORWARDING, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add MDMOID_DEV2_IPV4_FORWARDING Instance, ret = %d", ret);
      return ret;
   } 

   if ((ret = cmsObj_get(MDMOID_DEV2_IPV4_FORWARDING, &iidStack, 0, (void **) &ipv4ForwardingObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get ipv4Forwarding, ret = %d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_IPV4_FORWARDING, &iidStack);
      return ret;
   }

   ipv4ForwardingObj->enable = TRUE;
   ipv4ForwardingObj->staticRoute = FALSE;
   CMSMEM_REPLACE_STRING_FLAGS(ipv4ForwardingObj->interface, ipIntfFullPath, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv4ForwardingObj->gatewayIPAddress, gatewayIPAddr, mdmLibCtx.allocFlags);
   CMSMEM_REPLACE_STRING_FLAGS(ipv4ForwardingObj->origin, origin, mdmLibCtx.allocFlags);

   if((ret = cmsObj_set (ipv4ForwardingObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Set of ipv4ForwardingObj object failed, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_IPV4_FORWARDING, &iidStack);
   }
   else
   {
      /*
       * Since this function is only used for dynamically acquired routes,
       * this object instance should not be written to config file.  Mark it
       * as non-persistent.
       */
      if ((ret = cmsObj_setNonpersistentInstance(MDMOID_DEV2_IPV4_FORWARDING, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set non-persistent ipv4ForwardingObj. ret=%d", ret);
      }
   }

   cmsObj_free((void **)&ipv4ForwardingObj);

   cmsLog_debug("Exit, ret=%d", ret);
   
   return ret;
}


void rutRt_removeDefaultGatewayIfUsed_dev2(const char* ifName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2RouterObject *routerObj=NULL;
   CmsRet ret;

   cmsLog_debug("Enter: ifName=%s", ifName);

   /*
    * Don't know why TR181 defines multiple router objects.  Just get the
    * first router object.
    */
   if ((ret = cmsObj_getNext(MDMOID_DEV2_ROUTER, &iidStack, (void **)&routerObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get MDMOID_DEV2_ROUTER Instance, ret = %d", ret);
      return;
   }

   /* remove this ifName from list of potential default gateways */
   rutRt_removeIfNameFromList(ifName, routerObj->X_BROADCOM_COM_DefaultConnectionServices);

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   rutRt_removeIfNameFromList(ifName, routerObj->X_BROADCOM_COM_DefaultIpv6ConnectionServices);
#endif

   /* when we do a set, a new default gateway will be selected */
   if ((ret = cmsObj_set(routerObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_set of routerObj returns error. ret=%d", ret);
   }

   cmsObj_free((void **) &routerObj);

   return;
}


UBOOL8 rutRt_useGatewayIfcNameOnly_dev2(const char *inGwIfc, char *outGwIpAddress)
{
   UBOOL8 useIfcNameOnly = FALSE;

   cmsLog_debug("Enter: inGwIfc=%s", inGwIfc);

   if (outGwIpAddress == NULL)
   {
      cmsLog_error("outGwIpAddress is NULL!");
      return FALSE;
   }

   /* See if inGwIfc is a PPP intf */
   {
      _Dev2PppInterfaceObject *pppIntfObj=NULL;
      InstanceIdStack pppIidStack=EMPTY_INSTANCE_ID_STACK;
      UBOOL8 found=FALSE;

      while (!found &&
             (cmsObj_getNextFlags(MDMOID_DEV2_PPP_INTERFACE,
                                  &pppIidStack, OGF_NO_VALUE_UPDATE,
                                  (void **) &pppIntfObj) == CMSRET_SUCCESS))
      {
         if (!cmsUtl_strcmp(inGwIfc, pppIntfObj->name))
         {
            /* for PPP interface, use ifName only (needed for on-demand ppp) */
            useIfcNameOnly = TRUE;
            found = TRUE;             
         }
         cmsObj_free((void **) &pppIntfObj);
      }
   }

   /* for IpoA, the interface name will be fixed (ipoa0, ipoa1, etc).
    * and useIfcNameOnly = TRUE */
   /* See if inGwIfc is a IPoA intf */
   if (cmsUtl_strstr(inGwIfc, IPOA_IFC_STR))
   {
      _Dev2IpInterfaceObject *ipIntfObj=NULL;
      InstanceIdStack IPoAIidStack=EMPTY_INSTANCE_ID_STACK;
      UBOOL8 found=FALSE;
   
      while (!found &&
             (cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE,
                                  &IPoAIidStack, OGF_NO_VALUE_UPDATE,
                                  (void **) &ipIntfObj) == CMSRET_SUCCESS))
      {
         if (!cmsUtl_strcmp(inGwIfc, ipIntfObj->name))
         {
             /* for IPoA interface, use ifName only */
             useIfcNameOnly = TRUE;
             found = TRUE;             
         }
         cmsObj_free((void **) &ipIntfObj);
      }
   }


   /* for IPoE (WAN or LAN), useIfcNameOnly remains FALSE, so now we need
    * to pass the gwIpAddr back to caller */
   if (useIfcNameOnly == FALSE)
   {
      InstanceIdStack forwardingIidStack = EMPTY_INSTANCE_ID_STACK;
      Dev2Ipv4ForwardingObject *ipv4ForwardingObj=NULL;
      char intfNameBuf[CMS_IFNAME_LENGTH]={0};
      UBOOL8 found = FALSE;

      while(!found &&
            cmsObj_getNextFlags(MDMOID_DEV2_IPV4_FORWARDING,
                                &forwardingIidStack,
                                OGF_NO_VALUE_UPDATE,
                                (void **) &ipv4ForwardingObj) == CMSRET_SUCCESS)
      {
         /* Look for default route forwarding entries (destIPAddr not set) */
         if (cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, ipv4ForwardingObj->destIPAddress) &&
             (qdmIntf_fullPathToIntfnameLocked(ipv4ForwardingObj->interface, intfNameBuf) == CMSRET_SUCCESS))
         {
            if (!cmsUtl_strcmp(intfNameBuf, inGwIfc))
            {
               found = TRUE;
               cmsUtl_strcpy(outGwIpAddress, ipv4ForwardingObj->gatewayIPAddress);
            }
         }

         cmsObj_free((void *) &ipv4ForwardingObj);
      }
   }


   cmsLog_debug("Exit: inGwIfc=%s useIfcNameOnly=%d outGwIpAddr=%s",
                inGwIfc, useIfcNameOnly, outGwIpAddress);
   
   return useIfcNameOnly;
}



#ifdef SUPPORT_POLICYROUTING
CmsRet rut_isValidPolicyRouting_dev2(Dev2Ipv4ForwardingObject *probj, InstanceIdStack prIidStack)
{
   Dev2Ipv4ForwardingObject *obj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

   while (cmsObj_getNext(MDMOID_DEV2_IPV4_FORWARDING,  &iidStack, (void **) &obj) == CMSRET_SUCCESS)
   {
      if (obj->X_BROADCOM_COM_PolicyRoutingName && cmsMdm_compareIidStacks(&prIidStack, &iidStack) )
      {
         if (cmsUtl_strcmp(probj->X_BROADCOM_COM_SourceIPAddress, "") && cmsUtl_isValidIpAddress(AF_INET, probj->X_BROADCOM_COM_SourceIPAddress) == FALSE)
         {
            cmsLog_error("Invalid source IP address");
            cmsObj_free((void **) &obj);
            return CMSRET_INVALID_ARGUMENTS;		
         }

         if (!cmsUtl_strcmp(obj->X_BROADCOM_COM_PolicyRoutingName, probj->X_BROADCOM_COM_PolicyRoutingName))
         {
            cmsLog_error("Policy routing name %s already exists", probj->X_BROADCOM_COM_PolicyRoutingName);
            cmsObj_free((void **) &obj);
            return CMSRET_INVALID_ARGUMENTS;		
         }
		 
         if (!cmsUtl_strcmp(obj->X_BROADCOM_COM_SourceIPAddress, probj->X_BROADCOM_COM_SourceIPAddress) && 
              !cmsUtl_strcmp(obj->X_BROADCOM_COM_SourceIfName, probj->X_BROADCOM_COM_SourceIfName) )
         {
            cmsLog_error("Policy routing source info %s/%s already exists", probj->X_BROADCOM_COM_SourceIPAddress, probj->X_BROADCOM_COM_SourceIfName);
            cmsObj_free((void **) &obj);
            return CMSRET_INVALID_ARGUMENTS;		
         }

      }
   
      cmsObj_free((void **) &obj);
   }

   return ret;
}

UBOOL8 getActionInfoFromPolicyRoutingRuleName_dev2(const char *ruleName, char *srcIfName, char *srcIP, char *gatewayIP, char *outIfName)
{
   Dev2Ipv4ForwardingObject *obj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;
   char l3IntfNameBuf[CMS_IFNAME_LENGTH]={0};

   /*  Loop through all policy routing  */
   while (!found && 
      cmsObj_getNextFlags(MDMOID_DEV2_IPV4_FORWARDING, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &obj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(ruleName, obj->X_BROADCOM_COM_PolicyRoutingName))
      {
         if (cmsUtl_strcmp(obj->X_BROADCOM_COM_SourceIfName, ""))
         {
            strcpy(srcIfName, obj->X_BROADCOM_COM_SourceIfName);
         }            
         if (cmsUtl_strcmp(obj->X_BROADCOM_COM_SourceIPAddress, ""))
         {
            strcpy(srcIP, obj->X_BROADCOM_COM_SourceIPAddress);
         }
         if (cmsUtl_strcmp(obj->gatewayIPAddress, ""))
         {
            strcpy(gatewayIP, obj->gatewayIPAddress);
         }            
         if (cmsUtl_strcmp(obj->interface, ""))
         {
            if (qdmIntf_fullPathToIntfnameLocked(obj->interface, l3IntfNameBuf) != CMSRET_SUCCESS)
            {
               cmsLog_error("qdmIntf_fullPathToIntfnameLocked on %s", obj->interface);
            }
            else         
            {
               strcpy(outIfName, l3IntfNameBuf);
            }
         }           
         
         found = TRUE;
      }
      cmsObj_free((void **) &obj);
   }
   
   return found;
}

#endif /* SUPPORT_POLICYROUTING */

#endif  /* DMP_DEVICE2_ROUTING_1 */

#ifdef DMP_DEVICE2_BRIDGE_1 /* aka SUPPORT_PORT_MAP */
UBOOL8 getActionInfoFromPortMappingRuleName_dev2(const char *ruleName, char *srcIfName, char *srcIP, char *gatewayIP, char *outIfName)
{
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;
   CmsRet ret;
   char *ipIntfFullPath;
   
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&ipIntfObj)) == CMSRET_SUCCESS)
   {
      if (ipIntfObj->X_BROADCOM_COM_Upstream &&
          !cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_GroupName, ruleName))
      {
         ret = qdmIntf_intfnameToFullPathLocked(ipIntfObj->name, FALSE, &ipIntfFullPath);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to convert intfname %s To FullPath", ipIntfObj->name);
            cmsObj_free((void **)&ipIntfObj);
            return found;
         }
         qdmRt_getGatewayIpv4AddrByFullPathLocked_dev2(ipIntfFullPath, gatewayIP);
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);		 
         cmsUtl_strcpy(srcIfName, ipIntfObj->X_BROADCOM_COM_BridgeName);
         cmsUtl_strcpy(srcIP, "");
         cmsUtl_strcpy(outIfName, ipIntfObj->name);
		 
         found = TRUE;
      }

      cmsObj_free((void **)&ipIntfObj);
   }

   return found;   
}

#endif

#endif /* DMP_DEVICE2_BASELINE_1 */

