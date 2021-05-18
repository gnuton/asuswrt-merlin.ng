/***********************************************************************
 *
 *  Copyright (c) 2015  Broadcom Corporation
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

#include "stl.h"
#include "cms_util.h"
#include "rut_dsl.h"
#include "rut2_dsl.h"
#include "rut2_fast.h"
#include "rut_util.h"


#ifdef DMP_DEVICE2_FAST_1
CmsRet stl_dev2FastObject(_Dev2FastObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_dev2FastLineObject(_Dev2FastLineObject *obj, const InstanceIdStack *iidStack __attribute__((unused)))
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
      ret = rutfast_getLineInfo_dev2(obj,iidStack);
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

CmsRet stl_dev2FastLineStatsObject(_Dev2FastLineStatsObject *obj, const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

   if (obj != NULL)
   {
      ret = rutfast_getLineStats_dev2(obj);
   }
   else
   {
      if ((ret = rutWan_clearAdslTotalStats(0)) != CMSRET_SUCCESS)
      {
         cmsLog_debug("error reseting fast stats");
      }
   }
   return ret;
#endif
}

CmsRet stl_dev2FastLineStatsTotalObject(_Dev2FastLineStatsTotalObject *obj, const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret= CMSRET_SUCCESS;

   if (obj == NULL)
   {
      ret = rutWan_clearAdslTotalStats(0);
   }
   else
   {
      ret = rutfast_getTotalStats_dev2(obj);
   }
   return ret;
#endif /* DESKTOP_LINUX */
}

CmsRet stl_dev2FastLineStatsShowtimeObject(_Dev2FastLineStatsShowtimeObject *obj, 
                                           const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret = CMSRET_SUCCESS;
   if (obj == NULL)
   {
      if ((ret = rutWan_clearAdslTotalStats(0)) != CMSRET_SUCCESS)
      {
         cmsLog_debug("error reseting ADSL stats");
      }      
   }
   else
   {
      /* same function used to call total and showtime stats */
      ret = rutfast_getShowTimeStats_dev2(obj);
   }
   return(ret);
#endif /* DESKTOP_LINUX */
}

#if 0
/* notSupported */
CmsRet stl_dev2FastLineStatsLastShowtimeObject(_Dev2FastLineStatsLastShowtimeObject *obj __attribute__((unused)),
                                               const InstanceIdStack *iidStack __attribute__((unused)))
{
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

CmsRet stl_dev2FastLineStatsCurrentDayObject(_Dev2FastLineStatsCurrentDayObject *obj, 
                                             const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret = CMSRET_SUCCESS;

   if (obj == NULL)
   {
      if ((ret = rutWan_clearAdslTotalStats(0)) != CMSRET_SUCCESS)
      {
         cmsLog_debug("error reseting ADSL stats");
      }      
   }
   else
   {
      ret = rutfast_getCurrentDayStats_dev2(obj);
   }
   return(ret);
#endif
}

CmsRet stl_dev2FastLineStatsQuarterHourObject(_Dev2FastLineStatsQuarterHourObject *obj, 
                                              const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DESKTOP_LINUX
   /* for desktop, just pretend adsl link is always up */
   return (CMSRET_SUCCESS_OBJECT_UNCHANGED);
#else
   CmsRet ret = CMSRET_SUCCESS;

   if (obj == NULL)
   {
      if ((ret = rutWan_clearAdslTotalStats(0)) != CMSRET_SUCCESS)
      {
         cmsLog_debug("error reseting ADSL stats");
      }      
   }
   else
   {
      ret = rutfast_getQuarterHourStats_dev2(obj);
   }
   return(ret);
#endif
}

CmsRet stl_dev2FastLineTestParamsObject(_Dev2FastLineTestParamsObject *obj, 
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
      ret = rutfast_getTestParamsInfo_dev2(obj);
   }
   return(ret);
#endif
}

#endif /* DMP_DEVICE2_FAST_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */
