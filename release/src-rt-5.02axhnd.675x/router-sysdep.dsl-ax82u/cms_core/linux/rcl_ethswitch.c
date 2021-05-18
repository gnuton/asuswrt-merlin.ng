/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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
#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_boardcmds.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_pmap.h"
#include "rut_ethswitch.h"
#include "rut_top.h"

#define IS_VIRT_PORT_CHIP ((chipId == 0x6338) || (chipId == 0x6358))

CmsRet rcl_ethernetSwitchObject( _EthernetSwitchObject *newObj,
                const _EthernetSwitchObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   UINT32 chipId;
   CmsRet ret = CMSRET_SUCCESS;

   devCtl_getChipId(&chipId);
   
   if (ADD_NEW(newObj, currObj))
   {
      /*
       * New object instance.  This only happens at startup time.
       */
#if ! (defined(CONFIG_BCM960333))
      if (newObj->numberOfVirtualPorts == 0)
      {
         char cmdBuf[BUFLEN_64]={0};
         /*
          * Virtual ports is always in the enabled state on all chips,
          * so set it here.  Don't set ifName though, it confuses
          * the special case check in rcl_lanEthIntfObject.
          */
         newObj->enableVirtualPorts = TRUE;
         newObj->numberOfVirtualPorts = devCtl_getNumEnetPorts();

         /* for 6368 only, must also ifconfig bcmsw up */
         /* Yuchen is going to look into the possibility of just initializing
          * the switch into the up state.  For now, use this workaround. */
         snprintf(cmdBuf, sizeof(cmdBuf), "ifconfig bcmsw up");
         rut_doSystemAction("ethswitch", cmdBuf);
      }
#endif

   }
   else if (newObj != NULL && currObj != NULL)
   {
      /*
       * Edit the current instance.
       */
      if (newObj->enableVirtualPorts != currObj->enableVirtualPorts)
      {
         if (newObj->enableVirtualPorts)
         {
            if (newObj->numberOfVirtualPorts <= 1)
            {
               cmsLog_error("cannot enable virtual ports on %s, num interfaces=%d", newObj->ifName, newObj->numberOfVirtualPorts);
               ret = CMSRET_INVALID_ARGUMENTS;
            }
            else
            {
               cmsLog_debug("enabling virtual ports on %s", newObj->ifName);
            
#ifdef DMP_BRIDGING_1  /* aka SUPPORT_PORT_MAP */
               char bridgeIfName[BUFLEN_32]; 
               /* Enable virtual ports can ONLY take place while eth1 is in default interface group (br0) */
               if (rutPMap_getBridgeIfNameFromIfName(newObj->ifName, bridgeIfName, FALSE) != CMSRET_SUCCESS)
               {
                  cmsLog_error("Cannot find interface %s belonging to which bridgeIfName", "eth1");
                  return CMSRET_INTERNAL_ERROR;
               }

               if (cmsUtl_strcmp(bridgeIfName, "br0"))
               {
                  cmsLog_error("cannot enable virtual ports because the interface must be in br0");
                  return CMSRET_INVALID_ARGUMENTS;
               }
#endif
               rutTop_beginChange();
               rutEsw_insertEthernetSwitchKernelModule();
#ifdef DM_BASELINE_1
/* Is this virtualPorts stuff still supported?  Keep it in TR98 only for now */
               rutEsw_createVirtualPorts(newObj->ifName, newObj->numberOfVirtualPorts);
#endif
               rutTop_endChange();
            }
         }
         else
         {
               cmsLog_error("cannot disable virtual ports on this board.");
               ret = CMSRET_INVALID_ARGUMENTS;
         }
      }
   }

   /* this object can never be deleted, so no need to check for that case. */

   return ret;
}
