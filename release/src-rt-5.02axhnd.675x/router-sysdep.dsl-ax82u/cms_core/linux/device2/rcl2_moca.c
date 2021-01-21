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
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_moca.h"


/*!\file rcl2_moca.c
 * \brief This file contains Device2 MOCA related functions.
 *
 */

CmsRet rcl_dev2MocaObject( _Dev2MocaObject *newObj __attribute__((unused)),
                const _Dev2MocaObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2MocaInterfaceObject( _Dev2MocaInterfaceObject *newObj,
                const _Dev2MocaInterfaceObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Entered: iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   /* Unlikely to dynamically add or delete moca interfaces dynamically,
    * moca interfaces added at startup in adjustForHardware only,
    * so no ADD_NEW or DELETE_EXISTING code here.
    */

   /* enable moca device */
    if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
    {
       if ((ret = rutMoca_initialize(newObj->name,
                                     newObj->lastOperFreq)) != CMSRET_SUCCESS)
       {
          cmsLog_error("moca initialzation failed, ret=%d", ret);
          return ret;
       }

       /*
        * All the code on the TR98 side involving traceparams seems to do
        * nothing, so I did not port it to TR181 side.
        */

       ret = rutMoca_start(newObj->name,
               &newObj->X_BROADCOM_COM_AutoNwSearch, &newObj->privacyEnabled,
               &newObj->lastOperFreq,
               &newObj->keyPassphrase, &newObj->X_BROADCOM_COM_InitParmsString);
       if (ret != CMSRET_SUCCESS)
       {
          cmsLog_error("moca start failed");
       }


        rutLan_enableInterface(newObj->name);
        /*
         * On every new enable of Moca intf, make sure we have set WAN/LAN
         * on the switch correctly.  When moca intf is moved to/from WAN/LAN
         * intf will be disabled before the move, and then re-enabled after
         * move so this call will always get executed.
         */
        rutEth_setSwitchWanPort(newObj->name, newObj->upstream);
    }
    else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
    {
       /* some code here related to "AutoDetect".  Port later if necessary */


       /* check for update trace parameters command */
       if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_MocaControl, MDMVS_UPDATETRACE) == 0)
       {
          /* rutMoca_updateTraceParms() does nothing, so don't call */
          // ret = rutMoca_updateTraceParms(NULL, (LanMocaIntfObject *) newObj);

          /* This field should always read NONE */
          CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_MocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
       }

       /* rutMoca_setTrace does nothing.  Not ported to TR181 side */

       /* check for update configuration parameters command */
       if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_MocaControl, MDMVS_UPDATECONFIG) == 0)
       {
          /* rutMoca_updateConfiParms() does nothing, so don't call */
          // ret = rutMoca_updateConfigParms(NULL, (LanMocaIntfObject *) newObj);

          /* This field should always read NONE */
          CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_MocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
       }
#ifdef no_effect
       else if (cmsUtl_strcmp(newObj->configParmsString, currObj->configParmsString))
       {
          /* rutMoca_setParams() does nothing, so don't call */
          // ret = rutMoca_setParams((LanMocaIntfObject *) newObj, (LanMocaIntfObject *) currObj);
       }
#endif

       /* check for start command */
       if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_MocaControl, MDMVS_START) == 0)
       {
          ret = rutMoca_start(newObj->name,
                  &newObj->X_BROADCOM_COM_AutoNwSearch, &newObj->privacyEnabled,
                  &newObj->lastOperFreq,
                  &newObj->keyPassphrase, &newObj->X_BROADCOM_COM_InitParmsString);

          /* This field should always read NONE */
          CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_MocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
       }

       /* check for stop command */
       if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_MocaControl, MDMVS_STOP) == 0)
       {
          ret = rutMoca_stop(newObj->name);

          /* This field should always read NONE */
          CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_MocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
       }

       /* check for restart command */
       if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_MocaControl, MDMVS_RESTART) == 0)
       {
          ret = rutMoca_reinitialize(newObj->name,
                &newObj->X_BROADCOM_COM_AutoNwSearch, &newObj->privacyEnabled,
                &newObj->lastOperFreq,
                &newObj->keyPassphrase, &newObj->X_BROADCOM_COM_InitParmsString);

          /* This field should always read NONE */
          CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_MocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
       }

       /* check for update initialization parameters command */
       if (cmsUtl_strcmp(newObj->X_BROADCOM_COM_MocaControl, MDMVS_UPDATEINIT) == 0)
       {
          ret = rutMoca_getInitParms(NULL, newObj->name,
                &newObj->X_BROADCOM_COM_AutoNwSearch, &newObj->privacyEnabled,
                &newObj->lastOperFreq,
                &newObj->keyPassphrase, &newObj->X_BROADCOM_COM_InitParmsString);

          /* This field should always read NONE */
          CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_MocaControl, MDMVS_NONE, mdmLibCtx.allocFlags);
       }
    }

    else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
    {
       rutLan_disableInterface(currObj->name);
       rutMoca_stop(currObj->name);
    }

    return ret;
}

CmsRet rcl_dev2MocaInterfaceStatsObject( _Dev2MocaInterfaceStatsObject *newObj __attribute__((unused)),
                const _Dev2MocaInterfaceStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   cmsLog_debug("Entered");


   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2MocaInterfaceQosObject( _Dev2MocaInterfaceQosObject *newObj __attribute__((unused)),
                const _Dev2MocaInterfaceQosObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2MocaInterfaceQosFlowStatsObject( _Dev2MocaInterfaceQosFlowStatsObject *newObj __attribute__((unused)),
                const _Dev2MocaInterfaceQosFlowStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2MocaInterfaceAssociatedDeviceObject( _Dev2MocaInterfaceAssociatedDeviceObject *newObj __attribute__((unused)),
                const _Dev2MocaInterfaceAssociatedDeviceObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_MOCA_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
