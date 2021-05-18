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

#ifdef DMP_DEVICE2_DHCPV4_1

/** dhcp v4 server and client objects go into this file */

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"


CmsRet stl_dev2Dhcpv4Object(_Dev2Dhcpv4Object *obj __attribute((unused)),
                        const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Dhcpv4ClientObject(_Dev2Dhcpv4ClientObject *obj __attribute((unused)),
                              const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Dhcpv4ClientSentOptionObject(_Dev2Dhcpv4ClientSentOptionObject *obj __attribute((unused)),
                                            const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Dhcpv4ClientReqOptionObject(_Dev2Dhcpv4ClientReqOptionObject *obj __attribute((unused)),
                                           const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Dhcpv4ServerObject(_Dev2Dhcpv4ServerObject *obj __attribute((unused)),
                              const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Dhcpv4ServerPoolObject(_Dev2Dhcpv4ServerPoolObject *obj __attribute((unused)),
                                      const InstanceIdStack *iidStack)
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2Dhcpv4ServerPoolStaticAddressObject(_Dev2Dhcpv4ServerPoolStaticAddressObject *obj __attribute((unused)),
                                                   const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


#ifdef DMP_DEVICE2_DHCPV4SERVERCLIENTINFO_1

CmsRet stl_dev2Dhcpv4ServerPoolClientObject( _Dev2Dhcpv4ServerPoolClientObject *obj __attribute((unused)),
                const InstanceIdStack *iidStack)
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2Dhcpv4ServerPoolClientIPv4AddressObject( _Dev2Dhcpv4ServerPoolClientIPv4AddressObject *newObj __attribute((unused)),
                const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2Dhcpv4ServerPoolClientOptionObject( _Dev2Dhcpv4ServerPoolClientOptionObject *newObj __attribute((unused)),
                const InstanceIdStack *iidStack __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif  /* DMP_DEVICE2_DHCPV4SERVERCLIENTINFO_1 */


#ifdef DMP_DEVICE2_DHCPV4RELAY_1

CmsRet stl_dev2Dhcpv4RelayObject( _Dev2Dhcpv4RelayObject *obj __attribute((unused)),
                const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2Dhcpv4RelayForwardingObject( _Dev2Dhcpv4RelayForwardingObject *newObj __attribute((unused)),
                const InstanceIdStack *iidStack __attribute((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif  /* DMP_DEVICE2_DHCPV4RELAY_1 */

#endif  /* DMP_DEVICE2_DHCPV4_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */

