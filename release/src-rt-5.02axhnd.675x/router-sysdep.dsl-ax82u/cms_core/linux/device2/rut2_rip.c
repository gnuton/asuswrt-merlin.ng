/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "rut_rip.h"
#include "rut_util.h"
#include "rut_iptables.h"
#include "rut_wan.h"

#ifdef DMP_DEVICE2_ROUTING_1
#ifdef SUPPORT_RIP

static void rutRip_addRipEntryToConfigFile_dev2(FILE *fs, char *ifcName, UINT32 version, UBOOL8 acceptRA, UBOOL8 sendRA)
{

   /* add interface name */
   fprintf(fs, "interface %s\n", ifcName);
   /* RIP node */
   fputs("router rip\n", fs);
   fputs("redistribute kernel\n", fs);
   fputs("redistribute connected\n", fs);
   fputs("redistribute static\n", fs);
   fprintf(fs, "network %s\n", ifcName);

   if (!sendRA)
   {
     fprintf(fs, "passive-interface %s\n",ifcName);
   }

   fprintf(fs, "interface %s\n", ifcName);

   if (acceptRA)
   {
      if (version == 3)
         fputs("ip rip receive version 1 2\n", fs);
      else
         fprintf(fs, "ip rip receive version %d\n", version);
   }

   if (sendRA)
   {
      if (version == 3)
         fputs("ip rip send version 1 2\n", fs);
      else
         fprintf(fs, "ip rip send version %d\n", version);
   }
}


UBOOL8 rutRip_isConfigValid_dev2(const Dev2RipIntfSettingObject *newObj)
{
   UBOOL8 valid = TRUE;
   char l3IntfNameBuf[CMS_IFNAME_LENGTH]={0};
   CmsRet ret = CMSRET_SUCCESS;

   /* only bother to check validity if RIP is enabled */
   if (newObj && newObj->enable)
   {
      if ((ret = qdmIntf_fullPathToIntfnameLocked(newObj->interface, l3IntfNameBuf)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmIntf_fullPathToIntfnameLocked on %s ret=%d", newObj->interface, ret);
         valid = FALSE;
      }
      else if (qdmIpIntf_isNatEnabledOnIntfNameLocked_dev2(l3IntfNameBuf) && newObj->sendRA)
      {
         cmsLog_error("NAT and RIP sendRA are not compatible!");
         valid = FALSE;
      }
   }

   cmsLog_debug("valid=%d", valid);

   return valid;
}


void rutRip_addAllRipInterfacesToConfigFile_dev2(FILE *fs)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2RipIntfSettingObject *ripIntfSettingObj = NULL;
   char l3IntfNameBuf[CMS_IFNAME_LENGTH]={0};
   UBOOL8 isPureRipv1 = FALSE;
   UBOOL8 isPureRipv2 = FALSE;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("entered:");

   /* now look for WAN side RIP interfaces */
   while (cmsObj_getNextFlags(MDMOID_DEV2_RIP_INTF_SETTING, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &ripIntfSettingObj) == CMSRET_SUCCESS)
   {
      if (ripIntfSettingObj->enable)
      {
         if ((ret = qdmIntf_fullPathToIntfnameLocked(ripIntfSettingObj->interface, l3IntfNameBuf)) != CMSRET_SUCCESS)
         {
            cmsLog_error("qdmIntf_fullPathToIntfnameLocked on %s ret=%d", ripIntfSettingObj->interface, ret);
         }
         else		 
         {
            rutRip_addRipEntryToConfigFile_dev2(fs, l3IntfNameBuf, ripIntfSettingObj->X_BROADCOM_COM_Version,
                               ripIntfSettingObj->acceptRA, ripIntfSettingObj->sendRA);

            /* Record the WAN interface RIP versions */ 
            if (ripIntfSettingObj->X_BROADCOM_COM_Version == 1)
            {
               isPureRipv1 = TRUE;
            }
            else if (ripIntfSettingObj->X_BROADCOM_COM_Version == 2)
            {
               isPureRipv2 = TRUE;
            }
            else if (ripIntfSettingObj->X_BROADCOM_COM_Version == 3)
            {
               isPureRipv1 = TRUE;
               isPureRipv2 = TRUE;
            }

            /* Yucky side effect, but very convenient to do it here.
             * Add firewall exception for RIP on this interface.
            */
            if(qdmIpIntf_isFirewallEnabledOnIpIntfFullPathLocked(ripIntfSettingObj->interface))
            {
               rutIpt_ripAddIptableRule(l3IntfNameBuf);
            }
         }
      }

      cmsObj_free((void **) &ripIntfSettingObj);
   }

   /* add LAN side bridge interface according to the WAN interfaces RIP version configuration. For now br0
    * Should we also have a routeProtocolRx param on the LAN side interfaces?
    * Seems unlikely we will need to send RIP out on the LAN side,
    * and probably will not receive RIP on the LAN side.
    */
   if (isPureRipv1 && isPureRipv2)
   {
      rutRip_addRipEntryToConfigFile(fs, "br0", MDMVS_RIPV1V2, MDMVS_ACTIVE);
   }
   else if(isPureRipv1)
   {
      rutRip_addRipEntryToConfigFile(fs, "br0", MDMVS_RIPV1, MDMVS_ACTIVE);
   }
   else if(isPureRipv2)
   {
      rutRip_addRipEntryToConfigFile(fs, "br0", MDMVS_RIPV2, MDMVS_ACTIVE);
   }

   return;
}


UBOOL8 rut_isRipEnabled_dev2(void)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2RipObject *ripObj;
   CmsRet ret;
   UBOOL8 ripEnable = FALSE;

   cmsLog_debug("Enter");

   /* RIP object is only available for pure181 data model */
   if (!cmsMdm_isDataModelDevice2())
   {
      return ripEnable;
   }

   if ((ret = cmsObj_get(MDMOID_DEV2_RIP, &iidStack, 0, (void **) &ripObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MDMOID_DEV2_RIP, ret=%d", ret);
   }
   else
   {
      ripEnable = ripObj->enable;
      cmsObj_free((void **) &ripObj);
   }

   cmsLog_debug("rip enable status = %d", ripEnable);

   return ripEnable;
   
}


void rutRip_restart_dev2()
{   	
   char ifName[CMS_IFNAME_LENGTH];
   
   rutRip_stop();
   
   if (rutWan_findFirstIpvxRoutedAndConnected(CMS_AF_SELECT_IPVX, ifName))
   {
      rutRip_writeConfigFile();
      rutRip_restart();
   }
   else
   {
      cmsLog_debug("No wan connection available, ripd is not started");
   }
}

#endif /* SUPPORT_RIP */
#endif /* DMP_DEVICE2_ROUTING_1 */
