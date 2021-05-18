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

/*!\file mdm2_initlan6.c
 * \brief MDM initialization for PURE181 Device based tree,
 *        LAN side IPv6 objects.
 *
 */


#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1

#include "cms.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"


CmsRet mdm_addIpv6AddressObject_dev2(const MdmPathDescriptor *ipIntfPathDesc)
{
   Dev2Ipv6AddressObject *ipv6AddrObj=NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   INIT_PATH_DESCRIPTOR(&pathDesc);

   pathDesc.oid = MDMOID_DEV2_IPV6_ADDRESS;
   pathDesc.iidStack = ipIntfPathDesc->iidStack;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance for IPV6_ADDRESS failed, ret=%d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc.oid, &pathDesc.iidStack, (void **) &ipv6AddrObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get ipv6AddrObj object, ret=%d", ret);
      return ret;
   }
   ipv6AddrObj->enable = TRUE;
   /* TODO: probably more IPv6 address params need to be set */

   ret = mdm_setObject((void **) &ipv6AddrObj, &pathDesc.iidStack,  FALSE);
   mdm_freeObject((void **)&ipv6AddrObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set ipv6AddrObj. ret=%d", ret);
      return ret;
   }


   /* need to manually update the count when adding objects during mdm_init */
   {
      Dev2IpInterfaceObject *ipIntfObj=NULL;

      if ((ret = mdm_getObject(MDMOID_DEV2_IP_INTERFACE, &ipIntfPathDesc->iidStack, (void **) &ipIntfObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get ipIntfObj. ret=%d", ret);
         return ret;
      }

      ipIntfObj->IPv6AddressNumberOfEntries++;
      ret = mdm_setObject((void **) &ipIntfObj, &ipIntfPathDesc->iidStack, FALSE);
      mdm_freeObject((void **)&ipIntfObj);

      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set ipIntfObj. ret=%d", ret);
      }
   }

   return ret;
}


CmsRet mdm_addDefaultDhcpv6ServerObjects_dev2(const char *ipIntfFullPath)
{
   _Dev2Dhcpv6ServerPoolObject *serverPoolObj=NULL;
   _Dev2Dhcpv6ServerObject *serverObj=NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   /* First add and set dhcpv6 Server pool object */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_DHCPV6_SERVER_POOL;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add MDMOID_DHCPV6_SERVER_POOL  Instance, ret = %d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &serverPoolObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get MDMOID_DHCPV6_SERVER_POOL object, ret=%d", ret);
      return ret;
   }

#ifdef DMP_DEVICE2_HOMEPLUG_1
   serverPoolObj->enable = FALSE;
#else
   serverPoolObj->enable = TRUE;
#endif
   CMSMEM_REPLACE_STRING_FLAGS(serverPoolObj->interface, ipIntfFullPath, mdmLibCtx.allocFlags);
   /* do we need to set some address range here? */

   ret = mdm_setObject((void **) &serverPoolObj, &(pathDesc.iidStack),  FALSE);
   mdm_freeObject((void **)&serverPoolObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set serverPool. ret=%d", ret);
      return ret;
   }


   /* TODO: Sean's code added a DEV2_DHCPV6_SERVER_POOL_OPTION but did not
    * set any tag or value on the object.  Are we supposed to send out some
    * option?
    */
    
   _Dev2Dhcpv6ServerPoolOptionObject *serverPoolOption = NULL;
   pathDesc.oid = MDMOID_DEV2_DHCPV6_SERVER_POOL_OPTION;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add MDMOID_DEV2_DHCPV6_SERVER_POOL_OPTION  Instance, ret = %d", ret);
      return ret;
   }
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &serverPoolOption)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get MDMOID_DEV2_DHCPV6_SERVER_POOL_OPTION object, ret=%d", ret);
      return ret;
   }

   /* Set Pool.Option.Enable,  TODO: Tag/Value later.
    */
#ifdef DMP_DEVICE2_HOMEPLUG_1
   serverPoolOption->enable = FALSE;
#else
   serverPoolOption->enable = TRUE;
#endif
   if ((ret = mdm_setObject((void **) &serverPoolOption, &(pathDesc.iidStack),  FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set serverPoolOption. ret=%d", ret);
   }
   mdm_freeObject((void **)&serverPoolOption);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set MDMOID_DEV2_DHCPV6_SERVER_POOL_OPTION. ret=%d", ret);
      return ret;
   }

   /* Now set top level dhcpv6 Server object  */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_DHCPV6_SERVER;

   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &serverObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get MDMOID_DHCPV6_SERVER object, ret=%d", ret);
      return ret;
   }

#ifdef DMP_DEVICE2_HOMEPLUG_1
   serverObj->enable = FALSE;
   serverObj->poolNumberOfEntries = 0;
#else
   serverObj->enable = TRUE;
   serverObj->poolNumberOfEntries = 1;
#endif

   ret = mdm_setObject((void **) &serverObj, &(pathDesc.iidStack),  FALSE);
   mdm_freeObject((void **)&serverObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set serverObj. ret=%d", ret);
    }

   return ret;
}


CmsRet mdm_addDefaultRouterAdvertisementObjects_dev2(const char *ipIntfFullPath)
{
   Dev2RouterAdvertisementObject *raObj=NULL;
   Dev2RouterAdvertisementInterfaceSettingObject *raIntfObj=NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret;


   /* First add and set the RouterAdvertisementInterfaceSetting object */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_ROUTER_ADVERTISEMENT_INTERFACE_SETTING;
   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to add MDMOID_ROUTER_ADVERTISEMENT_INTERFACE_SETTING  Instance, ret = %d", ret);
      return ret;
   }

   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &raIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get MDMOID_ROUTER_ADVERTISEMENT_INTERFACE_SETTING object, ret=%d", ret);
      return ret;
   }

#ifdef DMP_DEVICE2_HOMEPLUG_1
   raIntfObj->enable = FALSE;
   CMSMEM_REPLACE_STRING_FLAGS(raIntfObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
#else
   raIntfObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(raIntfObj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
#endif
   CMSMEM_REPLACE_STRING_FLAGS(raIntfObj->interface, ipIntfFullPath, mdmLibCtx.allocFlags);

   ret = mdm_setObject((void **) &raIntfObj, &(pathDesc.iidStack),  FALSE);
   mdm_freeObject((void **)&raIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set RAIntf. ret=%d", ret);
      return ret;
   }


   /* Now update top level RouterAdvertisment object */
   INIT_PATH_DESCRIPTOR(&pathDesc);
   pathDesc.oid = MDMOID_DEV2_ROUTER_ADVERTISEMENT;

   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &raObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get RAObj, ret=%d", ret);
      return ret;
   }

#ifdef DMP_DEVICE2_HOMEPLUG_1
   raObj->enable = FALSE;
   raObj->interfaceSettingNumberOfEntries = 0;
#else
   raObj->enable = TRUE;
   raObj->interfaceSettingNumberOfEntries = 1;
#endif

   ret = mdm_setObject((void **) &raObj, &(pathDesc.iidStack),  FALSE);
   mdm_freeObject((void **)&raObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set RAObj. ret=%d", ret);
   }

   return ret;
}

#endif  /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
