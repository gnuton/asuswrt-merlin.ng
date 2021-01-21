/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
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

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"

/*!\file stl2_routing.c
 * \brief This file contains device 2 device.routing objects related functions.
 *
 */

#ifdef DMP_DEVICE2_ROUTING_1

CmsRet stl_dev2RoutingObject( _Dev2RoutingObject *newObj __attribute__((unused)),
                  const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* TODO: not implemented yet */
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2RouterObject( _Dev2RouterObject *newObj __attribute__((unused)),
                  const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* TODO: not implemented yet */
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2Ipv4ForwardingObject(_Dev2Ipv4ForwardingObject *newObj __attribute__((unused)),
                  const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* TODO: not implemented yet */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2RipObject( _Dev2RipObject *newObj __attribute__((unused)),
                  const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* TODO: not implemented yet */
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2RipIntfSettingObject(_Dev2RipIntfSettingObject *newObj __attribute__((unused)),
                  const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* TODO: not implemented yet */
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif  /* DMP_DEVICE2_ROUTING_1 */

#ifdef DMP_DEVICE2_IPV6ROUTING_1
/* the IPv6 routing functions should probably go into their own file.  rcl2_route6.c ? */

CmsRet stl_dev2Ipv6ForwardingObject( _Dev2Ipv6ForwardingObject *newObj __attribute__((unused)),
                  const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* TODO: not implemented yet */
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2RouteInfoObject( _Dev2RouteInfoObject *netObj __attribute__((unused)),
                  const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* TODO: not implemented yet */
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


CmsRet stl_dev2RouteInfoIntfSettingObject( _Dev2RouteInfoIntfSettingObject *netObj __attribute__((unused)),
                  const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* TODO: not implemented yet */
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

#endif  /* DMP_DEVICE2_IPV6ROUTING_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */



