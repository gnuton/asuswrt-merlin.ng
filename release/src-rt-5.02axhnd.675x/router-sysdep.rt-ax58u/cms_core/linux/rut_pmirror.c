/***********************************************************************
 *
 *  Copyright (c) 2008  Broadcom Corporation
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

 #if defined(SUPPORT_DSL) || defined(DMP_X_ITU_ORG_GPON_1)
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "odl.h"
#include "rcl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "bcmnet.h"
#include "rut_pmirror.h"

#ifdef DMP_X_ITU_ORG_GPON_1
#include "rut_util.h"
#endif /* DMP_X_ITU_ORG_GPON_1 */

#if defined(DMP_X_ITU_ORG_GPON_1)
UBOOL8 rutPMirror_isPortMirroringEnabled(void)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanDebugPortMirroringCfgObject *pmObj = NULL;
   UBOOL8 found = FALSE;

   while( !found && cmsObj_getNextFlags(MDMOID_WAN_DEBUG_PORT_MIRRORING_CFG,
        &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pmObj) == CMSRET_SUCCESS )
   {
       if (pmObj->status)
       {
           found = TRUE;
       }
       cmsObj_free((void **)&pmObj);
   }
   
   return found;
}
#endif

CmsRet rutPMirror_startPortMirroring(_WanDebugPortMirroringCfgObject *newObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   int  socketFd;
   struct ifreq intf;
   MirrorCfg mirrorCfg;
#ifdef DMP_X_ITU_ORG_GPON_1
   UINT8 *bin = NULL;
   UINT32 binSize;
   static UBOOL8 hwPath = TRUE;
#endif


#ifndef DMP_X_ITU_ORG_GPON_1
   strcpy(mirrorCfg.szMonitorInterface, newObj->monitorInterface);
#endif
   strcpy(mirrorCfg.szMirrorInterface, newObj->mirrorInterface);
   mirrorCfg.nDirection = (newObj->direction) ? MIRROR_DIR_OUT : MIRROR_DIR_IN;
   mirrorCfg.nStatus = (newObj->status) ? MIRROR_ENABLED : MIRROR_DISABLED;

   if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
   {
       cmsLog_error("port mirroring: could not open socket");
       ret = CMSRET_INTERNAL_ERROR;
   }
   else 
   {
#ifdef DMP_X_ITU_ORG_GPON_1
       strcpy(intf.ifr_name, "eth0");
       memset(mirrorCfg.nGemPortMaskArray, 0, sizeof(mirrorCfg.nGemPortMaskArray));
       if (newObj->gemPortMaskArray)
       {
           /* Found port mirror for connection 'in' direction. */
           cmsUtl_hexStringToBinaryBuf(newObj->gemPortMaskArray, &bin, &binSize);
           if (bin && binSize)
           {
               memcpy(mirrorCfg.nGemPortMaskArray, bin, 
                   binSize < sizeof(mirrorCfg.nGemPortMaskArray) ? 
                   binSize : sizeof(mirrorCfg.nGemPortMaskArray));
               cmsMem_free(bin);
           }
       }
#else
       strcpy(intf.ifr_name, mirrorCfg.szMonitorInterface);
#endif
       intf.ifr_data = (char*)&mirrorCfg;
       if (ioctl(socketFd, SIOCPORTMIRROR, &intf) == -1) 
       {
           ret = CMSRET_INTERNAL_ERROR;
           cmsLog_error( "port mirroring: IOCTL to bcmxtmrt driver for port mirroring failed"); 
       }
       close(socketFd);
   }

#ifdef DMP_X_ITU_ORG_GPON_1
   if (hwPath == TRUE && newObj->status)
   {
       hwPath = FALSE;
       rut_doSystemAction("rutPMirror_startPortMirroring", "cmf reset");
       rut_doSystemAction("rutPMirror_startPortMirroring", "cmf init");
       rut_doSystemAction("rutPMirror_startPortMirroring", "/etc/cmf/cmfcfg");
       rut_doSystemAction("rutPMirror_startPortMirroring", "cmf cfgmiss --mips");
       rut_doSystemAction("rutPMirror_startPortMirroring", "cmf dynamic --disable");
   }
#endif

   return ret;
   
}
#endif

#if defined(SUPPORT_DSL)
void rutPMirror_enablePortMirrorIfUsed(const char *wanL2IfName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanDebugPortMirroringCfgObject *obj = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   
   while ((ret = cmsObj_getNextFlags(MDMOID_WAN_DEBUG_PORT_MIRRORING_CFG,
                                              &iidStack,
                                              OGF_NO_VALUE_UPDATE, 
                                              (void **) &obj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(obj->monitorInterface, wanL2IfName))
      {
         if ((ret = rutPMirror_startPortMirroring(obj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Fail to enable port mirror for %s.", wanL2IfName);
         }
         else
         {
            cmsLog_debug("Port mirror is enabled for %s.", wanL2IfName);
         }
      }

      /* Free X_BROADCOM_COM_DebugPortMirroringCfg object. */
      cmsObj_free((void **) &obj);
      
   }                    
       
}


#endif /* SUPPORT_DSL */


