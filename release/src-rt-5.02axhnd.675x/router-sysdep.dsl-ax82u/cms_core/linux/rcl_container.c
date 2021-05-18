/***********************************************************************
<:copyright-BRCM:2017:proprietary:standard 

   Copyright (c) 2017 Broadcom 
   All Rights Reserved

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


#ifdef DMP_X_BROADCOM_COM_CONTAINER_1


#include "rut_util.h"


CmsRet rcl_containerObject(_ContainerObject *newObj __attribute__((unused)),
                           const _ContainerObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_containerInfoObject(_ContainerInfoObject *newObj,
                               const _ContainerInfoObject *currObj,
                               const InstanceIdStack *iidStack __attribute__((unused)),
                               char **errorParam __attribute__((unused)),
                               CmsRet *errorCode __attribute__((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumContainerEntry(iidStack, 1);
   }
   else if(DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumContainerEntry(iidStack, -1);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_containerStateObject(_ContainerStateObject *newObj __attribute__((unused)),
                                const _ContainerStateObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_containerNetworkObject(_ContainerNetworkObject *newObj __attribute__((unused)),
                                  const _ContainerNetworkObject *currObj __attribute__((unused)),
                                  const InstanceIdStack *iidStack __attribute__((unused)),
                                  char **errorParam __attribute__((unused)),
                                  CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_containerStatsObject(_ContainerStatsObject *newObj __attribute__((unused)),
                                const _ContainerStatsObject *currObj __attribute__((unused)),
                                const InstanceIdStack *iidStack __attribute__((unused)),
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


#endif /* DMP_X_BROADCOM_COM_CONTAINER_1 */

