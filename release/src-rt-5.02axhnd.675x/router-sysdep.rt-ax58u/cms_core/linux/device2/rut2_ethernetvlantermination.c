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


#ifdef DMP_DEVICE2_VLANTERMINATION_1

#include "cms.h"
#include "cms_mdm.h"
#include "cms_core.h"
#include "cms_util.h"
#include "stl.h"
#include "rut_util.h"
#include "cms_qdm.h"

/*!\file rut2_ethernetvlantermination.c
 * \brief This file contains ethernet vlantermination related util functions.
 *
 */

CmsRet rut2Vlan_getAvailVlanIndex_dev2(SINT32 *nextVlanIndex)
{
   Dev2VlanTerminationObject *ethVlan=NULL;
   CmsRet ret = CMSRET_SUCCESS;
   SINT32 vlanIndex=1;
   SINT32 vlanIndexArray[IFC_WAN_MAX+1] = {0};
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   
   /* need go thru to find out all the connection Id used in ip and ppp connection objects */
   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_VLAN_TERMINATION, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&ethVlan)) == CMSRET_SUCCESS)
   {
      char *p;
      SINT32 vlanIndex = 0;

      if (!IS_EMPTY_STRING(ethVlan->name))
      {
      
         /* Find the digit after '.' and mark it  to 1 in the array for later use.  */
         p = strchr(ethVlan->name, '.');
         if (p)
         {
            if (isdigit(*(p+1)))
            {
               vlanIndex = atoi(p+1);
            }            
            cmsLog_debug(" vlanIndex %d", vlanIndex);
            if (vlanIndex > IFC_VLAN_MAX)
            {
               cmsLog_debug(" Max vlanIndex is %d, current vlanIndex %d", IFC_VLAN_MAX, vlanIndex);            
            }
            else
            {
               vlanIndexArray[vlanIndex] = 1;
            }
         }
         else
         {
            cmsLog_debug(" No ethVlan ifName %s or the name has no  '.' and digit yet", ethVlan->name);
         }
      }         
      cmsObj_free((void **) &ethVlan);
   }



   for (vlanIndex = 0; vlanIndex <= IFC_VLAN_MAX; vlanIndex++)
   {
      cmsLog_debug("vlanIndexArray[%d]=%d", vlanIndex, vlanIndexArray[vlanIndex]);
   }

   /* vlan index id starts at 1 */
   for (vlanIndex = 1; vlanIndex <= IFC_VLAN_MAX; vlanIndex++)
   {
      if (vlanIndexArray[vlanIndex] == 0)
      {
         cmsLog_debug("found available vlanIndexIn=%d", vlanIndex);
         break;
      }
   }

   if (vlanIndex > 0 && vlanIndex <= IFC_VLAN_MAX)
   {
      *nextVlanIndex = vlanIndex;
      ret = CMSRET_SUCCESS;
   }
   else
   {
      cmsLog_error("Failed to find vlanIndex %d", vlanIndex);
      ret = CMSRET_INTERNAL_ERROR;
   }

   cmsLog_debug("Exit ret %d *nextVlanIndex %d", ret, *nextVlanIndex );

   return ret;

}


#endif /* DMP_DEVICE2_VLANTERMINATION_1 */

#else
/* DMP_DEVICE2_BASELINE_1 is not defined */

#ifdef DMP_DEVICE2_ETHERNETINTERFACE_1
#error "Device2 ethernet vlantermination objects incompatible with current Data Model mode, go to make menuconfig to fix"
#endif


#endif /* DMP_DEVICE2_BASELINE_1 */

