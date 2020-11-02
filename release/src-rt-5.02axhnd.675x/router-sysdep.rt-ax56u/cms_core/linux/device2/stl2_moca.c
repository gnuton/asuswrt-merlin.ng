/***********************************************************************
 *
 *  Copyright (c) 2006-2009  Broadcom Corporation
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

#ifdef DMP_DEVICE2_MOCA_1

#include "cms_core.h"
#include "cms_util.h"
#include "stl.h"
#include "rut_util.h"
#include "rut_system.h"
#include "rut_moca.h"
#include "rut_qos.h"


/*!\file stl2_moca.c
 * \brief This file contains Device2 MOCA related functions.
 *
 */

CmsRet stl_dev2MocaObject(_Dev2MocaObject *obj __attribute__((unused)),
               const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2MocaInterfaceObject(_Dev2MocaInterfaceObject *obj,
                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   char currentStatus[BUFLEN_32]={0};

   cmsLog_debug("Entered: %s currStatus=%s", obj->name, obj->status);

   if (cmsUtl_strlen(obj->name) == 0)
   {
      /* NULL or empty IntfName, can't do anything yet. */
      return CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }

   snprintf(currentStatus, sizeof(currentStatus), "%s", obj->status);

   rutMoca_getIntfInfo_dev2(obj->name, obj);

   if (cmsUtl_strcmp(obj->status, currentStatus))
   {
      cmsLog_debug("status changed from %s ==> %s", currentStatus, obj->status);
      obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();

      if (obj->upstream)
      {
         CmsRet r2;
         if (!strcmp(obj->status, MDMVS_UP))
         {
            if ((r2 = rutQos_tmPortInit(obj->name, TRUE)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_tmPortInit() returns error. ret=%d", r2);
            }
         }
         else
         {
            if ((r2 = rutQos_tmPortUninit(obj->name, TRUE)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_tmPortUninit() returns error. ret=%d", r2);
            }
         }
      }
   }

    /* Calculate and return the TR181 LastChange */
    IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);

   return CMSRET_SUCCESS;
}


CmsRet stl_dev2MocaInterfaceStatsObject(_Dev2MocaInterfaceStatsObject *obj,
                                        const InstanceIdStack *iidStack)
{
   Dev2MocaInterfaceObject *mocaIntfObj=NULL;
   InstanceIdStack parentIidStack = *iidStack;
   CmsRet ret;

   /* Get the parent object for intfName */
   if ((ret = cmsObj_getAncestorFlags(MDMOID_DEV2_MOCA_INTERFACE,
                                 MDMOID_DEV2_MOCA_INTERFACE_STATS,
                                 &parentIidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **) &mocaIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get parent Moca intf obj, ret=%d", ret);
      return ret;
   }

   if (obj == NULL)
   {
      rut_clearIntfStats(mocaIntfObj->name);
      rutMoca_resetStats(mocaIntfObj->name);
   }
   else
   {
      rutMoca_getStats_dev2(mocaIntfObj->name, obj);
   }

   cmsObj_free((void **)&mocaIntfObj);

   return CMSRET_SUCCESS;
}

CmsRet stl_dev2MocaInterfaceQosObject(_Dev2MocaInterfaceQosObject *obj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2MocaInterfaceQosFlowStatsObject(_Dev2MocaInterfaceQosFlowStatsObject *obj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2MocaInterfaceAssociatedDeviceObject(_Dev2MocaInterfaceAssociatedDeviceObject *obj __attribute__((unused)),
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


#endif  /* DMP_DEVICE2_MOCA_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
