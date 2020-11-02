/***********************************************************************
 *
 *  Copyright (c) 2010  Broadcom Corporation
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

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "bcmnet.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_atm.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "rut_wanlayer2.h"
#include "rut_eponwan.h"

UBOOL8 rutEpon_getEponIntfByIfName(const char *ifName, InstanceIdStack *iidStack, _WanEponIntfObject**eponIntfCfg)
{
   _WanEponIntfObject *wanEpon=NULL;
   UBOOL8 found=FALSE;
   InstanceIdStack wanDevIid;

   if (rutWl2_getEponWanIidStack(&wanDevIid) != CMSRET_SUCCESS)
   {
      return found;
   }
   
   if (cmsObj_getNextInSubTreeFlags(MDMOID_WAN_EPON_INTF, &wanDevIid, iidStack, OGF_NO_VALUE_UPDATE, (void **)&wanEpon) == CMSRET_SUCCESS)
   {
      cmsLog_debug("ifName = %s", wanEpon->ifName);
      if (!cmsUtl_strcmp(wanEpon->ifName, ifName))
      {
         found = TRUE;
      }
      else
      {
         cmsObj_free((void **) &wanEpon);
      }
   }

   if (found)
   {
      if (eponIntfCfg != NULL)
      {
         *eponIntfCfg = wanEpon;
      }
      else
      {
         cmsObj_free((void **) &wanEpon);
      }
   }

   cmsLog_debug("found=%d, ifName=%s", found, ifName);
   
   return found;
}

UBOOL8 rutEpon_getEponLinkByIfName(const char *ifName, InstanceIdStack *iidStack, _WanEponLinkCfgObject **eponLinkCfg)
{
   _WanEponLinkCfgObject *eponLinkObj=NULL;
   UBOOL8 found=FALSE;
   InstanceIdStack wanDevIid;

   cmsLog_debug("searching ifName = %s", ifName);

   if (rutWl2_getEponWanIidStack(&wanDevIid) != CMSRET_SUCCESS)
   {
      return found;
   }

   while (cmsObj_getNextInSubTree(MDMOID_WAN_EPON_LINK_CFG, &wanDevIid, iidStack, (void **)&eponLinkObj) == CMSRET_SUCCESS)
   {
      cmsLog_debug("eponLinkObj ifName = %s", eponLinkObj->ifName);
      if (!cmsUtl_strcmp(eponLinkObj->ifName, ifName))
      {
         found = TRUE;
         break;
      }
      else
      {
         cmsObj_free((void **) &eponLinkObj);
      }
   }

   if (found)
   {
      if (eponLinkCfg != NULL)
      {
         *eponLinkCfg = eponLinkObj;
      }
      else
      {
         cmsObj_free((void **) &eponLinkObj);
      }
   }

   cmsLog_debug("found=%d, ifName=%s", found, ifName);
   
   return found;
}

#endif /* DMP_X_BROADCOM_COM_EPONWAN_1 */

