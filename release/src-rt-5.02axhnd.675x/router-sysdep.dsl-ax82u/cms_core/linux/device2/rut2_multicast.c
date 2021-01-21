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

#ifdef DMP_DEVICE2_BASELINE_1

/*!\file rut2_multicast.c
 * \brief All of the functions in this file are TR181 multicast.
 */

#include <stdio.h>

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_network.h"
#include "rut_wan.h"
#include "rut_multicast.h"
#include "qdm_multicast.h"


void rutMulti_getAllProxyIntfNames_dev2(UBOOL8 isMld __attribute__((unused)),
      char *proxyIntfNames, UINT32 proxyIntfNamesLen __attribute__((unused)),
      char *sourceIntfNames, UINT32 sourceIntfNamesLen __attribute__((unused)))
{
   Dev2IpInterfaceObject *ipIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;


   if (proxyIntfNames == NULL)
   {
      cmsLog_error("proxyIntfNames is NULL");
      return;
   }
   proxyIntfNames[0] = '\0';

   if (sourceIntfNames == NULL)
   {
      cmsLog_error("sourceIntfNames is NULL");
      return;
   }
   sourceIntfNames[0] = '\0';

   /* XXX see comments above _igd version */

   while (cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                                    (void **) &ipIntfObj) == CMSRET_SUCCESS)
   {
#if defined(DMP_X_BROADCOM_COM_IGMP_1)
      if (!isMld)
      {
         if (ipIntfObj->X_BROADCOM_COM_IGMPEnabled)
         {
            rutMulti_addIntfNameToList("igmp_proxy",
                                       ipIntfObj->name,
                                       proxyIntfNames, proxyIntfNamesLen);
         }

         if (ipIntfObj->X_BROADCOM_COM_IGMP_SOURCEEnabled)
         {
            rutMulti_addIntfNameToList("igmp_source",
                                       ipIntfObj->name,
                                       sourceIntfNames, sourceIntfNamesLen);
         }
      }
#endif

#if defined(DMP_X_BROADCOM_COM_MLD_1)
      if (isMld)
      {
         if (ipIntfObj->X_BROADCOM_COM_MLDEnabled)
         {
            rutMulti_addIntfNameToList("mld_proxy",
                                       ipIntfObj->name,
                                       proxyIntfNames, proxyIntfNamesLen);
         }

         if (ipIntfObj->X_BROADCOM_COM_MLD_SOURCEEnabled)
         {
            rutMulti_addIntfNameToList("mld_source",
                                       ipIntfObj->name,
                                       sourceIntfNames, sourceIntfNamesLen);
         }
       }
#endif
       cmsObj_free((void **) &ipIntfObj);
    }

   return;
}


#endif  /* DMP_DEVICE2_BASELINE_1 */
