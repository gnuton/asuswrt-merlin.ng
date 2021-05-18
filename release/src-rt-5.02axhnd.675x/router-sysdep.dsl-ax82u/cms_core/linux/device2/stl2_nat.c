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

#ifdef DMP_DEVICE2_NAT_1

/** all the TR181 NAT stl functions will go into this file,
 */

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"


CmsRet stl_dev2NatObject( _Dev2NatObject *newObj __attribute((unused)),
                        const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2NatIntfSettingObject( _Dev2NatIntfSettingObject *newObj,
                        const InstanceIdStack *iidStack __attribute((unused)))
{
   /* Update Device.NAT.InterfaceSetting.{i}.Status */
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   char statusBuf[BUFLEN_64]={0};

   if (newObj->enable == FALSE)
   {
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status,
                                  MDMVS_DISABLED,
                                  mdmLibCtx.allocFlags);

      return CMSRET_SUCCESS;
   }

   qdmIpIntf_getIpvxServiceStatusFromFullPathLocked_dev2(
                                 newObj->interface,
                                 CMS_AF_SELECT_IPV4,
                                 statusBuf, sizeof(statusBuf));

   if (!cmsUtl_strcmp(statusBuf, MDMVS_SERVICEUP))
   {

      /* Interface UP , IPv4 Enable */
      ret = CMSRET_SUCCESS;

      if(newObj->enable)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status,
                                     MDMVS_ENABLED,
                                     mdmLibCtx.allocFlags);
      }
      else
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status,
                                     MDMVS_DISABLED,
                                     mdmLibCtx.allocFlags);
      }
   }
   else
   {
      ret = CMSRET_SUCCESS;      
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status,
                                  MDMVS_ENABLED,
                                  mdmLibCtx.allocFlags);
   }
   
   return ret;
}



CmsRet stl_dev2NatPortMappingObject( _Dev2NatPortMappingObject *newObj __attribute((unused)),
                        const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2NatPortTriggeringObject( _Dev2NatPortTriggeringObject *newObj __attribute((unused)),
                        const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


#endif  /* DMP_DEVICE2_NAT_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */

