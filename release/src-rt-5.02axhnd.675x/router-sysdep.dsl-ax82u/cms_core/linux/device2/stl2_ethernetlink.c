/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2011:proprietary:standard

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
#include "stl.h"
#include "rut_util.h"


/*!\file stl2_ethernetlink.c
 * \brief This file contains ethernet link related functions.
 *
 */


CmsRet stl_dev2EthernetLinkObject(_Dev2EthernetLinkObject *obj,
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS;

   /*
    * There are a couple of tricky issues involving getting the mac address:
    * 1. During bootup, the interface, e.g. ptm0, might not exist yet,
    *    so only query for mac address if interface exists.
    * 2. mac address for bridge changes depending on what interfaces are on
    *    the bridge, so always get the value from the kernel if name starts
    *    with br
    */
   if ((obj->name != NULL) &&
       (cmsNet_isInterfaceExist(obj->name)) &&
       ((obj->MACAddress == NULL) || !cmsUtl_strncmp(obj->name, "br", 2)))
   {
      UINT8 macAddr[MAC_ADDR_LEN]={0};
      char macStr[MAC_STR_LEN+1]={0};
      CmsRet r2;

      cmsLog_debug("getting macAddr for %s", obj->name);
      if ((r2 = cmsNet_getMacAddrByIfname(obj->name,  macAddr)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsNet_getMacAddrByIfname for %s failed, ret = %d",
                       obj->name, r2);
      }
      else
      {
         cmsUtl_macNumToStr(macAddr, macStr);
         if (cmsUtl_strcmp(obj->MACAddress, macStr))
         {
            cmsLog_debug("MacAddr changed, was %s now %s", obj->MACAddress, macStr);
            CMSMEM_REPLACE_STRING_FLAGS(obj->MACAddress, macStr, mdmLibCtx.allocFlags);
         }
      }
   }

   /* Calculate and return the TR181 LastChange */
   IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);

   return ret;
}



#endif /* DMP_DEVICE2_ETHERNETLINK_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */

