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

#ifdef DMP_DEVICE2_ETHERNETINTERFACE_1

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut2_util.h"
#include "rut_ethintf.h"

/*!\file rcl2_ethernet.c
 * \brief This file contains Device2 ethernet related functions.
 *
 */


CmsRet rcl_dev2EthernetObject( _Dev2EthernetObject *newObj __attribute__((unused)),
                const _Dev2EthernetObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2EthernetInterfaceObject( _Dev2EthernetInterfaceObject *newObj,
                const _Dev2EthernetInterfaceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   if (newObj)
   {
      cmsLog_debug("Entered: newObj->name=%s enable=%d currObj=%p iidStack=%s",
                    newObj->name, newObj->enable, currObj,
                    cmsMdm_dumpIidStack(iidStack));
   }
   else if (currObj)
   {
      cmsLog_debug("Entered: delete currObj->name=%s iidStack=%s",
                    currObj->name, cmsMdm_dumpIidStack(iidStack));
   }

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumEthInterface(iidStack, 1);
   }


   /* enable eth device */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      if (newObj->upstream == FALSE)
      {
         rutQos_tmPortInit(newObj->name, FALSE);
      }
      rutLan_enableInterface(newObj->name);
#if !(defined(CHIP_60333))
      rutEth_setSwitchWanPort(newObj->name, newObj->upstream);
#endif
#ifdef SUPPORT_LANVLAN
      if (newObj->name && newObj->upstream == FALSE)
      {
         rutLan_AddDefaultLanVlanInterface(newObj->name);
      }
#endif
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {

      rutLan_disableInterface(currObj->name);
      // TR98 code also does rutLan_removeInterfaceFromBridge(currObj->X_BROADCOM_COM_IfName, bridgeIfName);
      // But there is no reason to delete an interface from bridge just because it is disabled, right?
#ifdef SUPPORT_LANVLAN
      if (currObj->name && currObj->upstream == FALSE)
      {
         rutLan_RemoveDefaultLanVlanInterface(currObj->name);
      }
#endif

      if (DELETE_EXISTING(newObj, currObj))
      {
         rutUtil_modifyNumEthInterface(iidStack, -1);
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2EthernetInterfaceStatsObject( _Dev2EthernetInterfaceStatsObject *newObj __attribute__((unused)),
                const _Dev2EthernetInterfaceStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


#endif /* DMP_DEVICE2_ETHERNETINTERFACE_1 */

#else
/* DMP_DEVICE2_BASELINE_1 is not defined */

#ifdef DMP_DEVICE2_ETHERNETINTERFACE_1
#error "Device2 ethernet interface objects incompatible with current Data Model mode, go to make menuconfig to fix"
#endif

#endif  /* DMP_DEVICE2_BASELINE_1 */
