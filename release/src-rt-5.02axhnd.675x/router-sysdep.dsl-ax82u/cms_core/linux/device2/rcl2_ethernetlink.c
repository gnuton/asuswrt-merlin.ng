/***********************************************************************
 *
 *  Copyright (c) 2006-2009  Broadcom Corporation
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


#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_ETHERNETLINK_1

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "cms_qdm.h"

/*!\file rcl_ethernetlink.c
 * \brief This file contains Device2 ethernetlink related functions.
 *
 */


CmsRet rcl_dev2EthernetLinkObject( _Dev2EthernetLinkObject *newObj,
                const _Dev2EthernetLinkObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumEthernetLink(iidStack, 1);
   }

   IF_STATUS_HAS_CHANGED_SET_LASTCHANGE(newObj, currObj);

   /*
    * If we are not in a delete situation (newObj != NULL) and
    * we don't have a Linux interface name yet, but we do have a LowerLayers
    * param, then fill in our name (ifname).
    */
   if ((newObj != NULL) &&
       IS_EMPTY_STRING(newObj->name) &&
       !IS_EMPTY_STRING(newObj->lowerLayers))
   {
      char ifName[CMS_IFNAME_LENGTH]={0};

      cmsLog_debug("newObj->lowerLayers %s", newObj->lowerLayers);

      /* get the ifName of lowerlayer interfaces.
      * For hybrid mode where ethernet.link points to
      * ether WANIP/WANPPP objects instead of TR181 layer2 objects
      */
      if ((ret = qdmIntf_fullPathToIntfnameLocked(newObj->lowerLayers, ifName)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmIntf_getIntfnameFromFullPathLocked failed. ret %d", ret);
         return ret;
      }

      CMSMEM_REPLACE_STRING_FLAGS(newObj->name, ifName, mdmLibCtx.allocFlags);
   }
   
   if(DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumEthernetLink(iidStack, -1);
   }
   return ret;
}



#endif /* DMP_DEVICE2_ETHERNETINTERFACE_1 */

#else
/* DMP_DEVICE2_BASELINE_1 is not defined */

#ifdef DMP_DEVICE2_ETHERNETINTERFACE_1
#error "Device2 ethernet link objects incompatible with current Data Model mode, go to make menuconfig to fix"
#endif

#endif  /* DMP_DEVICE2_BASELINE_1 */
