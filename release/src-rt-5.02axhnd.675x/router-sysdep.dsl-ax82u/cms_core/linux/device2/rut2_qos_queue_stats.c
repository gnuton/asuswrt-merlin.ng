/***********************************************************************
 *
 *  Copyright (c) 2009-2015  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2015:proprietary:standard

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

#ifdef DMP_DEVICE2_QOSSTATS_1
/* All of this code is part of the DMP_DEVICE2_QOSSTATS_1 profile 
 * (which is enabled only in Pure TR181 mode).
 */

#include "cms_core.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_qos.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_qos.h"


CmsRet rutQos_getQueueIdFromQueueFullPath
   (const char *fullPath,
    UINT32 *queueId)
{
   MdmPathDescriptor pathDesc;
   PhlGetParamValue_t *pParamValue = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   if (fullPath == NULL)
      return CMSRET_INVALID_ARGUMENTS;

   INIT_PATH_DESCRIPTOR(&pathDesc);

   ret = cmsMdm_fullPathToPathDescriptor(fullPath, &pathDesc);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get path descriptor from full path %s, ret %d",
                   fullPath, ret);
      return ret;
   }

   cmsUtl_strcpy(pathDesc.paramName, "X_BROADCOM_COM_QueueId");

   ret = cmsPhl_getParamValue(&pathDesc, &pParamValue);

   if (ret == CMSRET_SUCCESS)
   {
      sscanf(pParamValue->pValue, "%u", queueId);

      cmsLog_debug("TR181 oid %d %s queueId %d",
                   pathDesc.oid,
                   cmsMdm_dumpIidStack(&(pathDesc.iidStack)),
                   *queueId);

      cmsPhl_freeGetParamValueBuf(pParamValue, 1);
   }

   return ret;
}


CmsRet rutQos_getInterfaceNameFromFullPath
   (const char *fullPath,
    char *nameBuf,
    UINT32 nameBufLen)
{
   MdmPathDescriptor pathDesc;
   PhlGetParamValue_t *pParamValue = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   if (nameBuf == NULL || fullPath == NULL)
      return CMSRET_INVALID_ARGUMENTS;

   INIT_PATH_DESCRIPTOR(&pathDesc);

   ret = cmsMdm_fullPathToPathDescriptor(fullPath, &pathDesc);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get path descriptor from full path %s, ret %d",
                   fullPath, ret);
      return ret;
   }

   memset(nameBuf, 0, nameBufLen);

   cmsUtl_strcpy(pathDesc.paramName, "Name");

   ret = cmsPhl_getParamValue(&pathDesc, &pParamValue);

   if (ret == CMSRET_SUCCESS)
   {
      if (cmsUtl_strlen(pParamValue->pValue) > 0)
      {
         snprintf(nameBuf, nameBufLen, "%s", pParamValue->pValue);
         cmsLog_debug("TR181 oid %d %s name %s",
                      pathDesc.oid,
                      cmsMdm_dumpIidStack(&(pathDesc.iidStack)),
                      nameBuf);
      }

      cmsPhl_freeGetParamValueBuf(pParamValue, 1);
   }

   return ret;
}


#endif  /* DMP_DEVICE2_QOSSTATS_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */

