/***********************************************************************
 *
 *  Copyright (c) 2014  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2014:proprietary:standard

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


#ifdef DMP_DEVICE2_PERIODICSTATSBASE_1

#include "stl.h"
#include "cms.h"
#include "cms_util.h"
#include "rut_util.h"

#include "periodicstat.h"

CmsRet stl_dev2PeriodicStatObject(_Dev2PeriodicStatObject *obj, const InstanceIdStack *iidStack)
{
   /* DONE.  This handler function does not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2SampleSetObject(_Dev2SampleSetObject *obj, const InstanceIdStack *iidStack)
{
   FILE *file;
   char fileName[BUFLEN_64];
   char bufResult[BUFLEN_1024];
   char *pValue;
   int targetID = PEEK_INSTANCE_ID(iidStack);

#ifdef DMP_DEVICE2_PERIODICSTATSADV_1
   /* if status is not disalbed, force it to enabled*/
   if ( cmsUtl_strcmp(obj->status, MDMVS_DISABLED) != 0 )
   {
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_ENABLED, mdmLibCtx.allocFlags);
   }
#endif /* DMP_DEVICE2_PERIODICSTATSADV_1 */

   sprintf(fileName, PERIODIC_STATS_SAMPLE_FILENAME_PREFIX"%d", targetID);
   file = fopen(fileName, "r");
   if (file == NULL)
      return CMSRET_SUCCESS;

   fscanf(file, "%s", bufResult);
   pValue = strstr(bufResult, "=");
   if ( pValue != NULL )
   {
      pValue++;
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->reportStartTime, pValue, mdmLibCtx.allocFlags);
   }

   fscanf(file, "%s", bufResult);
   pValue = strstr(bufResult, "=");
   if ( pValue != NULL )
   {
      pValue++;
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->reportEndTime, pValue, mdmLibCtx.allocFlags);
   }

   fscanf(file, "%s", bufResult);
   pValue = strstr(bufResult, "=");
   if ( pValue != NULL )
   {
      pValue++;
      if (strlen(pValue) > 0 && pValue[strlen(pValue)-1] == ',')
         pValue[strlen(pValue)-1] = '\0';
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->sampleSeconds, pValue, mdmLibCtx.allocFlags);
   }

   fclose(file);
   return CMSRET_SUCCESS;
}

CmsRet stl_dev2SampleParameterObject(_Dev2SampleParameterObject *obj, const InstanceIdStack *iidStack)
{
   FILE *file;
   char fileName[BUFLEN_64];
   char bufResult[BUFLEN_1024];
   char *pValue;
   int targetID = PEEK_INSTANCE_ID(iidStack);
   int smpsetID = INSTANCE_ID_AT_DEPTH(iidStack, 0);

   sprintf(fileName, PERIODIC_STATS_SAMPLE_FILENAME_PREFIX"%d_%d", smpsetID, targetID);
   file = fopen(fileName, "r");
   if (file == NULL)
      return CMSRET_SUCCESS;

   // Reference
   fscanf(file, "%s", bufResult);

   // Values
   fscanf(file, "%s", bufResult);
   pValue = strstr(bufResult, "=");
   if ( pValue != NULL )
   {
      pValue++;
      if (strlen(pValue) > 0 && pValue[strlen(pValue)-1] == ',')
         pValue[strlen(pValue)-1] = '\0';
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->values, pValue, mdmLibCtx.allocFlags);
   }

   // SuspectData
   fscanf(file, "%s", bufResult);
   pValue = strstr(bufResult, "=");
   if ( pValue != NULL )
   {
      pValue++;
      if (strlen(pValue) > 0 && pValue[strlen(pValue)-1] == ',')
         pValue[strlen(pValue)-1] = '\0';
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->suspectData, pValue, mdmLibCtx.allocFlags);
   }

   // SampleSeconds
   fscanf(file, "%s", bufResult);
   pValue = strstr(bufResult, "=");
   if ( pValue != NULL )
   {
      pValue++;
      if (strlen(pValue) > 0 && pValue[strlen(pValue)-1] == ',')
         pValue[strlen(pValue)-1] = '\0';
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->sampleSeconds, pValue, mdmLibCtx.allocFlags);
   }

#ifdef DMP_DEVICE2_PERIODICSTATSADV_1
   // Failures 
   fscanf(file, "%s", bufResult);
   pValue = strstr(bufResult, "=");
   if ( pValue != NULL )
   {
      pValue++;
      if (strlen(pValue) > 0) 
         obj->failures = atol(pValue);
   }
#endif

   fclose(file);
   return CMSRET_SUCCESS;
}

#endif /* DMP_DEVICE2_PERIODICSTATSBASE_1 */
