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

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1

#ifdef DMP_DEVICE2_IPV6ROUTING_1

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





CmsRet rutRt_activateIpv6Routing_dev2(const char *ipIntfFullPath)
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
    * (3) IPv6Forwarding objs which point to the interface which just came up.
    * This allows them to configure their route.
    */
   {
      InstanceIdStack forwardingIidStack=EMPTY_INSTANCE_ID_STACK;
      Dev2Ipv6ForwardingObject *ipv6ForwardingObj=NULL;

      while(cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_FORWARDING,
                                &routerIidStack,
                                &forwardingIidStack,
                                OGF_NO_VALUE_UPDATE,
                                (void **) &ipv6ForwardingObj) == CMSRET_SUCCESS)
      {
         if (!cmsUtl_strcmp(ipv6ForwardingObj->origin, MDMVS_STATIC) &&
             !cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, ipv6ForwardingObj->destIPPrefix) &&
             (IS_EMPTY_STRING(ipv6ForwardingObj->interface) ||!cmsUtl_strcmp(ipv6ForwardingObj->interface, ipIntfFullPath)))
         {
            if ((ret = cmsObj_set(ipv6ForwardingObj, &forwardingIidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsObj_set <MDMOID_DEV2_IPV6_FORWARDING> returns error. ret=%d", ret);
            }
         }

         cmsObj_free((void **)&ipv6ForwardingObj);
      }
   }

   cmsLog_debug("Exit. ret=%d", ret);

   return ret;
}


void rutRt_deactivateIpv6Routing_dev2(const char *ipIntfFullPath)
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
    * Go through all IPv6Forwarding objects associated with this IP
    * interface.  For the dynamically created entries, just delete.
    * For the static entries, do a set.
    */
   {
      InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
      InstanceIdStack savedIidStack=EMPTY_INSTANCE_ID_STACK;
      Dev2Ipv6ForwardingObject *ipv6ForwardingObj=NULL;

      while(cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_FORWARDING,
                                &routerIidStack,
                                &iidStack,
                                OGF_NO_VALUE_UPDATE,
                                (void **) &ipv6ForwardingObj) == CMSRET_SUCCESS)
      {
         if (IS_EMPTY_STRING(ipv6ForwardingObj->interface) || !cmsUtl_strcmp(ipv6ForwardingObj->interface, ipIntfFullPath))
         {
            if (cmsUtl_strcmp(ipv6ForwardingObj->origin, MDMVS_STATIC))
            {
               /* if this is not a static entry, just delete it.
                * Deleting has the same effect as disable.
                */
               cmsObj_deleteInstance(MDMOID_DEV2_IPV6_FORWARDING, &iidStack);

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
               if ((ret = cmsObj_set(ipv6ForwardingObj, &iidStack)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("cmsObj_set <MDMOID_DEV2_IPV6_FORWARDING> returns error. ret=%d", ret);
               }
            }
         }

         savedIidStack = iidStack;

         cmsObj_free((void **)&ipv6ForwardingObj);
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



#endif  /* DMP_DEVICE2_IPV6ROUTING_1 */

#endif  /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */

