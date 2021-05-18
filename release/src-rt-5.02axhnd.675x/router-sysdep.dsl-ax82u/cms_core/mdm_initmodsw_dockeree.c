/***********************************************************************
 *
 *  Copyright (c) 2017  Broadcom Corporation
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

#ifdef DMP_DEVICE2_X_BROADCOM_COM_MODSW_DOCKEREE_1

#include "cms.h"
#include "cms_params_modsw.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"


/*!\file mdm_initmodsw_dockeree.c
 * \brief This file is responsible for creating the initial
 *        Docker Execution Environment (EE) entry in the MDM.
 *
 */

CmsRet mdm_addDefaultModSwDockerEeObjects(UINT32 *eeAddedCount)
{
   ExecEnvObject *eeObj = NULL;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 add = TRUE;
   CmsRet ret = CMSRET_SUCCESS;

   /*
    * If there is no Docker Exec Env object in the Data Model yet, add it.
    */
   while (add &&
          (ret = mdm_getNextObject(MDMOID_EXEC_ENV, &iidStack, (void **)&eeObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(eeObj->name, DOCKEREE_NAME))
      {
         /* already in there, so no need to add */
         add = FALSE;
      }
      mdm_freeObject((void **) &eeObj);
   }

   if (add)
   {
      cmsLog_notice("Adding initial Docker Execution Environment");

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_EXEC_ENV;
      INIT_INSTANCE_ID_STACK(&pathDesc.iidStack);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not add Docker Exec Env, ret=%d", ret);
         return ret;
      }
      else
      {
         (*eeAddedCount)++;
      }

      if ((ret = mdm_getObject(MDMOID_EXEC_ENV, &pathDesc.iidStack, (void **) &eeObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get Docker Exec Env Obj, ret=%d", ret);
         return ret;
      }

      /* set any non-default initial values for the Docker EE object here */
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->name, DOCKEREE_NAME, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->alias, DOCKEREE_NAME, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->type, DOCKEREE_TYPE, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->vendor, DOCKEREE_VENDOR, mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->version, DOCKEREE_VERSION, mdmLibCtx.allocFlags);
      eeObj->X_BROADCOM_COM_MngrEid = EID_DOCKERMD;

      if ((ret = mdm_setObject((void **) &eeObj, &pathDesc.iidStack, FALSE)) != CMSRET_SUCCESS)
      {
         mdm_freeObject((void **) &eeObj);
         cmsLog_error("Failed to set EXEC_ENV Object, ret = %d", ret);
      }
   }

   return ret;
}


#endif /* DMP_DEVICE2_X_BROADCOM_COM_MODSW_OPENWRTEE_1 */


