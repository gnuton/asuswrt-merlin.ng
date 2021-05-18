/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
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

#include "stl.h"
#include "cms_util.h"
#include "rut_dsl.h"
#include "rut2_dsl.h"
#include "rut_util.h"


#ifdef DMP_DEVICE2_DSL_1
CmsRet stl_dev2DslObject(_Dev2DslObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2DslLineObject(_Dev2DslLineObject *obj,
                      const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   char prevStatusBuf[BUFLEN_32]={0};

   if (obj != NULL)
   {
      cmsUtl_strncpy(prevStatusBuf, obj->status, sizeof(prevStatusBuf));
   }

#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status,MDMVS_UP,mdmLibCtx.allocFlags);
#else

   if (obj == NULL)
   {
      /* there is no function to clear this stats */
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   else
   {
      ret = rutdsl_getLineInfo_dev2(obj,iidStack);
   }
#endif

   if ((obj != NULL) && (ret == CMSRET_SUCCESS) &&
       cmsUtl_strcmp(prevStatusBuf, obj->status))
   {
      obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
   }

   /* Calculate and return the TR181 LastChange */
   IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);

   return ret;
}

CmsRet stl_dslLineBertTestObject(_Dev2DslLineBertTestObject *obj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS; /* just assume this always changes the object */

   cmsLog_debug("Entered");

#ifndef DESKTOP_LINUX
   ret = rutWan_getAdslBertInfo(obj,iidStack);
#endif /* DESKTOP_LINUX */

   return ret;
}

CmsRet stl_dev2DslLineStatsObject(_Dev2DslLineStatsObject *obj, const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   UINT32 lineId;

   if (obj != NULL)
   {
      ret = rutdsl_getdslLineStats_dev2(obj,iidStack);
   }
   else
   {
      rutdsl_getLineIdByLineIidStack_dev2(iidStack,&lineId);
      if ((ret = rutWan_clearAdslTotalStats(lineId)) != CMSRET_SUCCESS)
      {
         cmsLog_debug("error reseting ADSL stats");
      }
   }
   return ret;
#endif
}

CmsRet stl_dev2DslLineStatsTotalObject(_Dev2DslLineStatsTotalObject *obj, const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret;
   UINT32 lineId;

   if (obj == NULL)
   {
      rutdsl_getLineIdByLineIidStack_dev2(iidStack,&lineId);
      ret = rutWan_clearAdslTotalStats(lineId);
   }
   else
   {
      ret = rutdsl_getAdslTotalStats_dev2(obj,iidStack);
   }
   return ret;
#endif /* DESKTOP_LINUX */
}

CmsRet stl_dev2DslLineStatsShowtimeObject(_Dev2DslLineStatsShowtimeObject *obj, 
                                      const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret;
   UINT32 lineId;

   if (obj == NULL)
   {
      rutdsl_getLineIdByLineIidStack_dev2(iidStack,&lineId);
      if ((ret = rutWan_clearAdslTotalStats(lineId)) != CMSRET_SUCCESS)
      {
         cmsLog_debug("error reseting ADSL stats");
      }      
   }
   else
   {
      /* same function used to call total and showtime stats */
      ret = rutdsl_getAdslShowTimeStats_dev2(obj,iidStack);
   }
   return(ret);
#endif /* DESKTOP_LINUX */
}

#ifdef NOT_SUPPORTED
CmsRet stl_dev2DslLineStatsLastShowtimeObject(_Dev2DslLineStatsLastShowtimeObject *obj __attribute__((unused)), 
                                          const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_dev2DslLineStatsCurrentDayObject(_Dev2DslLineStatsCurrentDayObject *obj, 
                                        const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret;
   UINT32 lineId;

   if (obj == NULL)
   {
      rutdsl_getLineIdByLineIidStack_dev2(iidStack,&lineId);
      if ((ret = rutWan_clearAdslTotalStats(lineId)) != CMSRET_SUCCESS)
      {
         cmsLog_debug("error reseting ADSL stats");
      }      
   }
   else
   {
      ret = rutdsl_getAdslCurrentDayStats_dev2(obj,iidStack);
   }
   return(ret);
#endif
}

CmsRet stl_dev2DslLineStatsQuarterHourObject(_Dev2DslLineStatsQuarterHourObject *obj, 
                                         const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret;
   UINT32 lineId;

   if (obj == NULL)
   {
      rutdsl_getLineIdByLineIidStack_dev2(iidStack,&lineId);
      if ((ret = rutWan_clearAdslTotalStats(lineId)) != CMSRET_SUCCESS)
      {
         cmsLog_debug("error reseting ADSL stats");
      }      
   }
   else
   {
      ret = rutdsl_getAdslQuarterHourStats_dev2(obj,iidStack);
   }
   return(ret);
#endif
}

/* this is only applicable for vdsl */
CmsRet stl_dev2DslLineTestParamsObject(_Dev2DslLineTestParamsObject *obj, 
                                   const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret;

   if (obj == NULL)
   {
      /* there is no function to clear this stats */
      ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   }
   else
   {
      ret = rutdsl_getAdslTestParamsInfo_dev2(obj,iidStack);
   }
   return(ret);
#endif
}

CmsRet stl_dev2DslChannelObject(_Dev2DslChannelObject *obj,
                    const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   char prevStatusBuf[BUFLEN_32]={0};
   UINT32 lineId;

   if (obj != NULL)
   {
      cmsUtl_strncpy(prevStatusBuf, obj->status, sizeof(prevStatusBuf));
   }

#ifdef DESKTOP_LINUX
   /* for desktop, just pretend channel is always up */
   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status,MDMVS_UP,mdmLibCtx.allocFlags);
#else

   if (obj == NULL)
   {
      rutdsl_getLineIdByChannelIidStack_dev2(iidStack,&lineId);
      if ((ret = rutWan_clearAdslTotalStats(lineId)) != CMSRET_SUCCESS)
      {
         cmsLog_debug("error reseting ADSL stats");
      }      
   }
   else
   {
      ret = rutdsl_getChannelInfo_dev2(obj,iidStack);
   }
#endif /* DESKTOP_LINUX */

   if ((obj != NULL) && (ret == CMSRET_SUCCESS) &&
       cmsUtl_strcmp(prevStatusBuf, obj->status))
   {
      obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
   }

   /* Calculate and return the TR181 LastChange */
   IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);

   return ret;
}

CmsRet stl_dev2DslChannelStatsObject(_Dev2DslChannelStatsObject *obj, const InstanceIdStack *iidStack)
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   Dev2DslChannelObject *dslChannelObj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   UINT32 lineId;

   if (cmsObj_get(MDMOID_DEV2_DSL_CHANNEL,iidStack,0,(void **)&dslChannelObj) == CMSRET_SUCCESS)
   {
      if (!strcmp(dslChannelObj->status, MDMVS_UP))
      {
         if (obj == NULL)
         {
            rutdsl_getLineIdByChannelIidStack_dev2(iidStack,&lineId);
            if ((ret = rutWan_clearAdslTotalStats(lineId)) != CMSRET_SUCCESS)
            {
               cmsLog_debug("error reseting ADSL stats");
            }      
         }
         else
         {
            ret = rutdsl_getdslChannelStats_dev2(obj,iidStack);
         }
      }
      cmsObj_free((void **) &dslChannelObj);
   }
   return (ret);
#endif /* DESKTOP_LINUX */
}

CmsRet stl_dev2DslChannelStatsTotalObject(_Dev2DslChannelStatsTotalObject *obj, const InstanceIdStack *iidStack)
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   Dev2DslChannelObject *dslChannelObj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   UINT32 lineId;

   if (cmsObj_get(MDMOID_DEV2_DSL_CHANNEL,iidStack,0,(void **)&dslChannelObj) == CMSRET_SUCCESS)
   {
      if (!strcmp(dslChannelObj->status, MDMVS_UP))
      {
         if (obj == NULL)
         {
            rutdsl_getLineIdByChannelIidStack_dev2(iidStack,&lineId);
            ret = rutWan_clearAdslTotalStats(lineId);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_debug("error reseting ADSL stats");
            }
         }
         else
         {
            ret = rutdsl_getTotalChannelStats_dev2(obj,iidStack);
         }         
      }
      cmsObj_free((void **) &dslChannelObj);
   }
   return (ret);
#endif /* DESKTOP_LINUX */
}

CmsRet stl_dev2DslChannelStatsShowtimeObject(_Dev2DslChannelStatsShowtimeObject *obj, const InstanceIdStack *iidStack)
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   Dev2DslChannelObject *dslChannelObj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   UINT32 lineId;

   if (cmsObj_get(MDMOID_DEV2_DSL_CHANNEL,iidStack,0,(void **)&dslChannelObj) == CMSRET_SUCCESS)
   {
      if (!strcmp(dslChannelObj->status, MDMVS_UP))
      {
         if (obj != NULL)
         {
            ret = rutdsl_getShowTimeChannelStats_dev2(obj,iidStack);
         }         
         else
         {
            rutdsl_getLineIdByChannelIidStack_dev2(iidStack,&lineId);
            ret = rutWan_clearAdslTotalStats(lineId);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_debug("error reseting ADSL stats");
            }
         }
      }
      cmsObj_free((void **) &dslChannelObj);
   }
   return (ret);
#endif /* DESKTOP_LINUX */
}

CmsRet stl_dev2DslChannelStatsLastShowtimeObject(_Dev2DslChannelStatsLastShowtimeObject *obj, const InstanceIdStack *iidStack)
{
   /* we do not have this now */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
}

CmsRet stl_dev2DslChannelStatsCurrentDayObject(_Dev2DslChannelStatsCurrentDayObject *obj, const InstanceIdStack *iidStack)
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   Dev2DslChannelObject *dslChannelObj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   UINT32 lineId;

   if (cmsObj_get(MDMOID_DEV2_DSL_CHANNEL,iidStack,0,(void **)&dslChannelObj) == CMSRET_SUCCESS)
   {
      if (!strcmp(dslChannelObj->status, MDMVS_UP))
      {
         if (obj != NULL)
         {
            ret = rutdsl_getCurrentDayChannelStats_dev2(obj,iidStack);
         }
         else
         {
            rutdsl_getLineIdByChannelIidStack_dev2(iidStack,&lineId);
            ret = rutWan_clearAdslTotalStats(lineId);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_debug("error reseting ADSL stats");
            }
         }
      }
      cmsObj_free((void **) &dslChannelObj);
   }
   return (ret);
#endif /* DESKTOP_LINUX */
}

CmsRet stl_dev2DslChannelStatsQuarterHourObject(_Dev2DslChannelStatsQuarterHourObject *obj, const InstanceIdStack *iidStack)
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   Dev2DslChannelObject *dslChannelObj;
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
   UINT32 lineId;

   if (cmsObj_get(MDMOID_DEV2_DSL_CHANNEL,iidStack,0,(void **)&dslChannelObj) == CMSRET_SUCCESS)
   {
      if (!strcmp(dslChannelObj->status, MDMVS_UP))
      {
         if (obj != NULL)
         {
            ret = rutdsl_getQuarterHourChannelStats_dev2(obj,iidStack);
         }
         else
         {
            rutdsl_getLineIdByChannelIidStack_dev2(iidStack,&lineId);
            ret = rutWan_clearAdslTotalStats(lineId);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_debug("error reseting ADSL stats");
            }
         }
      }
      cmsObj_free((void **) &dslChannelObj);
   }
   return (ret);
#endif /* DESKTOP_LINUX */
}

#ifdef DMP_X_BROADCOM_COM_DSL_1 
CmsRet stl_dev2DslLineBertTestObject(_Dev2DslLineBertTestObject *obj __attribute__((unused)),
                                     const InstanceIdStack *iidStack __attribute__((unused)))
{
   CmsRet ret=CMSRET_SUCCESS; /* just assume this always changes the object */

   cmsLog_debug("Entered");

#ifndef DESKTOP_LINUX
   ret = rutWan_getAdslBertInfo(obj,iidStack);
#endif /* DESKTOP_LINUX */

   return ret;
}
#endif

#ifdef DMP_DEVICE2_DSLDIAGNOSTICS_1
CmsRet stl_dev2DslDiagnosticsObject(_Dev2DslDiagnosticsObject *obj, const InstanceIdStack *iidStack)
{
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
}

CmsRet stl_dev2DslDiagnosticsADSLLineTestObject(_Dev2DslDiagnosticsADSLLineTestObject *obj, 
                                                const InstanceIdStack *iidStack)
{   
   if (cmsUtl_strcmp(obj->diagnosticsState,MDMVS_REQUESTED) == 0)
   {
      return(rutWan_getAdslLoopDiagStatus(obj,iidStack));
   }
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif /* DMP_DSLDIAGNOSTICS_1 */

#endif /* DMP_DEVICE2_DSL_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */
