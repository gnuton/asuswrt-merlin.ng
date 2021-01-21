/***********************************************************************
 *
 *  Copyright (c) 2006-2013  Broadcom Corporation
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

#ifdef  DMP_DEVICE2_PPPINTERFACE_1

#ifdef  DMP_DEVICE2_PPPINTERFACE_2

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_wan.h"
#include "cms_qdm.h"

/*!\file rcl2_ppp.c
 * \brief This file contains device 2 device.PPP object related functions.
 *
 */

CmsRet rutPpp_getIpIntfFullPathByPppFullPath_dev2(const char *pppIntfFullPath, char **ipIntfFullPath)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;
   _Dev2IpInterfaceObject *ipIntf=NULL;
   CmsRet ret;


   if (cmsUtl_strlen(pppIntfFullPath) == 0)
   {
      /* for whatever reason, the ifname is not yet known.  This function
       * cannot do anything without the ifname.
       */
      ret = CMSRET_INVALID_ARGUMENTS;
      cmsLog_error("Invalid pppIntfFullPath string, ret %d", ret);
      return ret;
   }
   
   while (!found &&
          (ret = cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE,
                                &iidStack,
                                (void **)&ipIntf)) == CMSRET_SUCCESS)
   {
      
      if (!cmsUtl_strcmp(ipIntf->lowerLayers, pppIntfFullPath))
      {
         UBOOL8 isLayer2=FALSE;
         
         found = TRUE;
         ret = qdmIntf_intfnameToFullPathLocked_dev2(ipIntf->name,isLayer2, ipIntfFullPath);
      }
      cmsObj_free((void **) &ipIntf);
   }

   cmsLog_debug("ret %d, ipIntfFullPath %s", ret, *ipIntfFullPath);
   
   return ret;
   
}


void rutPpp_stopPppd_dev2(UINT32 pppPid, const char *lowerLayerFullPath)
{
   UINT32 specificEid;
   char cmdStr[BUFLEN_128];
   char baseL3IfName[CMS_IFNAME_LENGTH]={0};
   CmsRet ret = CMSRET_SUCCESS;
   
   cmsLog_debug("Enter: pppPid=%d lowerLayerFullPath=%s", pppPid, lowerLayerFullPath);

   specificEid = MAKE_SPECIFIC_EID(pppPid, EID_PPP); 
   if ((ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to stop pppd at pid %d", pppPid);
   }
   else
   {
      cmsLog_debug("pppd stopped");
   }

   if ((ret = qdmIntf_getIntfnameFromFullPathLocked_dev2(lowerLayerFullPath,
                                                         baseL3IfName, 
                                                         sizeof(baseL3IfName))) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert %s to intfName, ret %d",
                    lowerLayerFullPath, ret);
      return;
   }

   cmsLog_debug("got baseL3IfName=%s", baseL3IfName);

   /* do all these commands only if interface still exists */
   if (cmsNet_getIfindexByIfname(baseL3IfName) >= 0)
   {
      snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s down", baseL3IfName);
      rut_doSystemAction("rutWan_stopL3Interface: ifconfig L3IfName down", (cmdStr));

      /* Need to remove the network from the routing table by
      * doing  "ifconfig L3IfName 0 0.0.0.0"
      */
      snprintf(cmdStr, sizeof(cmdStr), "ifconfig %s 0.0.0.0", baseL3IfName);
      rut_doSystemAction("rutPpp_stopPppd_dev2: ifconfig L3IfName 0.0.0.0", (cmdStr));
   }
   
   return;
}

#endif  /* DMP_DEVICE2_PPPINTERFACE_1 */

#endif  /* DMP_DEVICE2_PPPINTERFACE_2 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
