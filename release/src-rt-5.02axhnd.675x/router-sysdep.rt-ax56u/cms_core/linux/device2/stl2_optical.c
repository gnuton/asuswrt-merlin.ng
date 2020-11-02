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

#ifdef DMP_DEVICE2_OPTICAL_1

#include "stl.h"
#include "cms_util.h"
#include "cms_math.h"
#include "gponctl_api.h"
#include "rut_system.h"
#include "rut_lan.h"
#include "rut_util.h"
#include "laser.h"

#include <math.h>
#include <fcntl.h>

#if 0
#define AE_INTF_NAME "eth4"


/** Convert power in micro-watts to dBm as required by Broadband
 * Forum Data Model spec.
 *
 * @param dir (IN) string indicating which power this is (for debugging)
 * @param puw (IN) Power in 0.1 micro-watts
 * @param min (IN) min allowed value by BBF data model spec
 * @param max (IN) max allowed value by BBF data model spec
 * @param inc (IN) reporting increments (steps) required by BBF data model spec
 *
 * @return dBm * 1000 as required by BBF data model spec
 */
static SINT32 convertPointOneMicroWattsToBBFdBm(const char *dir, UINT32 puw,
                                                SINT32 min, SINT32 max, SINT32 inc)
{
   SINT32 rv;

   /* logf does not like 0, so check for it first */
   if (puw == 0)
   {
      cmsLog_notice("(%s) 0 uW, set to min %d", dir, min);
      return min;
   }

   rv = pointOneMicroWattsTodBm(puw);
   if (rv == INT32_MIN)
   {
      cmsLog_notice("(%s) %d uW => %d below min, limit to %d", dir, puw, rv, min);
      return min;
   }

   /* now round to nearest increment */
   if (rv > 0)
   {
      rv += (inc/2);
   }
   else
   {
      rv -= (inc/2);
   }
   rv = (rv / inc) * inc;

   if (rv < min)
   {
      cmsLog_notice("(%s) %d uW => %d below min, limit to %d", dir, puw, rv, min);
      rv = min;
   }
   else if (rv > max)
   {
      cmsLog_notice("(%s) %d uW => %d above max, limit to %d", dir, puw, rv, max);
      rv = max;
   }
   else
   {
      /* good conversion */
      cmsLog_debug("(%s) %d uW => %d", dir, puw, rv);
   }

   return rv;
}


static CmsRet updateOpticalSignal(OpticalInterfaceObject *obj)
{
    UINT32 tempPower;
    int LaserFd;
    
    LaserFd = open("/dev/laser_dev", O_RDWR);

    if (LaserFd >= 0)
    {
        if (ioctl(LaserFd, LASER_IOCTL_GET_RX_PWR, &tempPower) >= 0)
        {
           obj->opticalSignalLevel = convertPointOneMicroWattsToBBFdBm("RX", tempPower,
                                                           -65536, 65534, 2);
        }
        else
        {
           cmsLog_error("Laser driver IOCTL error on Receive Optical Signal Level");
           obj->opticalSignalLevel = -65536;
        }

        if (ioctl(LaserFd, LASER_IOCTL_GET_TX_PWR, &tempPower) >= 0)
        {
           obj->transmitOpticalLevel = convertPointOneMicroWattsToBBFdBm("TX", tempPower,
                                                           -127500, 0, 500);
        }
        else
        {
           cmsLog_error("Laser driver IOCTL error on Transmit Optical Signal Level");
           obj->transmitOpticalLevel = -127500;
        }

        close(LaserFd);
    }
    else
    {
        cmsLog_error("Laser driver open error");
        obj->opticalSignalLevel = -65536;
        obj->transmitOpticalLevel = -127500;
    }

    return CMSRET_SUCCESS;
}

static UBOOL8 isGponActive(void)
{
    UBOOL8 ret = TRUE;
    BCM_Ploam_StateInfo info;

    memset(&info, 0, sizeof(BCM_Ploam_StateInfo));

    if (gponCtl_getControlStates(&info) == CMSRET_SUCCESS)
    {
        switch (info.operState)
        {
            case BCM_PLOAM_OSTATE_INITIAL_O1:
            case BCM_PLOAM_OSTATE_STANDBY_O2:
            case BCM_PLOAM_OSTATE_SERIAL_NUMBER_O3:
            case BCM_PLOAM_OSTATE_RANGING_O4:
            case BCM_PLOAM_OSTATE_OPERATION_O5:
            case BCM_PLOAM_OSTATE_POPUP_O6:
            case BCM_PLOAM_OSTATE_EMERGENCY_STOP_O7:
            case BCM_PLOAM_OSTATE_DS_TUNING_O8:
            case BCM_PLOAM_OSTATE_US_TUNING_O9:
                ret = TRUE;
                break;
            default:
                ret = FALSE;
                break;
        }
    }

    return ret;
}

static CmsRet updateGponStatus(OpticalInterfaceObject *obj)
{
    CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
    char *linkStatus = NULL;
    BCM_Ploam_StateInfo info;

    memset(&info, 0, sizeof(BCM_Ploam_StateInfo));
    linkStatus = MDMVS_NOTPRESENT;
    obj->enable = FALSE;

    if (gponCtl_getControlStates(&info) == CMSRET_SUCCESS)
    {
        if (info.adminState == BCM_PLOAM_ASTATE_ON)
        {
            obj->enable = TRUE;
            switch (info.operState)
            {
                case BCM_PLOAM_OSTATE_OPERATION_O5:
                case BCM_PLOAM_OSTATE_DS_TUNING_O8:
                case BCM_PLOAM_OSTATE_US_TUNING_O9:
                    linkStatus = MDMVS_UP;
                    break;
                case BCM_PLOAM_OSTATE_INITIAL_O1:
                case BCM_PLOAM_OSTATE_STANDBY_O2:
                case BCM_PLOAM_OSTATE_SERIAL_NUMBER_O3:
                case BCM_PLOAM_OSTATE_RANGING_O4:
                case BCM_PLOAM_OSTATE_POPUP_O6:
                    linkStatus = MDMVS_DOWN;
                    break;
                case BCM_PLOAM_OSTATE_EMERGENCY_STOP_O7:
                    linkStatus = MDMVS_ERROR;
                    break;
                default:
                    linkStatus = MDMVS_UNKNOWN;
                    break;
            }
        }
        else
        {
            obj->enable = FALSE;
            linkStatus = MDMVS_DOWN;
        }
    }

    if (cmsUtl_strcmp(linkStatus, obj->status))
    {
        CMSMEM_REPLACE_STRING_FLAGS(obj->status, linkStatus, mdmLibCtx.allocFlags);
        // tell upper layers the value has changed
        ret = CMSRET_SUCCESS;
    }

    return ret;
}

static CmsRet updateActiveEthernetStatus(OpticalInterfaceObject *obj)
{
    CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
    char currentStatus[BUFLEN_16];
    char hwAddr[BUFLEN_32];

    rutLan_getIntfStatus(AE_INTF_NAME, currentStatus, hwAddr);

    cmsLog_debug("Active Ethernet IfName %s, status %s, currentStatus %s, hwAddr %s",
                AE_INTF_NAME, obj->status, currentStatus, hwAddr);

    if (cmsUtl_strcmp(currentStatus, MDMVS_UP) == 0)
    {
        obj->enable = TRUE;
    }
    else
    {
        obj->enable = FALSE;
    }

    // convert from intf4StatusValues to ifOperStatusValues
    if (cmsUtl_strcmp(currentStatus, MDMVS_UP) != 0 &&
        cmsUtl_strcmp(currentStatus, MDMVS_ERROR)  != 0)
    {
        strcpy(currentStatus, MDMVS_DOWN);
    }

    if (cmsUtl_strcmp(obj->status, currentStatus))
    {
        CMSMEM_REPLACE_STRING_FLAGS(obj->status, currentStatus, mdmLibCtx.allocFlags);
        // tell upper layers the value has changed
        ret = CMSRET_SUCCESS;
    }

    return ret;
}

static CmsRet updateOpticalStatus(OpticalInterfaceObject *obj)
{
    CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

    if (isGponActive() == TRUE)
    {
        ret = updateGponStatus(obj);
    }
    else
    {
        ret = updateActiveEthernetStatus(obj);
    }

    return ret;
}

static CmsRet updateGponStats(OpticalInterfaceStatsObject *obj)
{
    CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
    UINT32 errCounters = 0;
    BCM_Ploam_GemPortCounters counters;
    BCM_Ploam_GtcCounters gtcCounters;
    BCM_Ploam_fecCounters fecCounters;

    memset(&counters, 0, sizeof(BCM_Ploam_GemPortCounters));
    memset(&gtcCounters, 0, sizeof(BCM_Ploam_GtcCounters));

    counters.gemPortID = BCM_PLOAM_GEM_PORT_ID_UNASSIGNED;
    counters.gemPortIndex = BCM_PLOAM_GEM_PORT_IDX_ALL;

    // reset statistics counters
    if (obj == NULL)
    {
        counters.reset = 1;
        gtcCounters.reset = 1;
        fecCounters.reset = 1;

        if (gponCtl_getGemPortCounters(&counters) == CMSRET_SUCCESS &&
            gponCtl_getGtcCounters(&gtcCounters) == CMSRET_SUCCESS &&
            gponCtl_getFecCounters(&fecCounters) == CMSRET_SUCCESS)
        {
            // tell upper layers the counters has reset
            ret = CMSRET_SUCCESS;
        }
    }
    else
    {		
        counters.reset = 0;
        gtcCounters.reset = 0;
        fecCounters.reset = 0;

        if (gponCtl_getGemPortCounters(&counters) == CMSRET_SUCCESS &&
            gponCtl_getGtcCounters(&gtcCounters) == CMSRET_SUCCESS &&
            gponCtl_getFecCounters(&fecCounters) == CMSRET_SUCCESS)
        {
            obj->bytesSent = counters.txBytes;
            obj->bytesReceived = counters.rxBytes;
            obj->packetsSent = counters.txFrames;
            obj->packetsReceived = counters.rxFrames;
            errCounters = gtcCounters.bipErr + fecCounters.fecCerr + fecCounters.fecUerr;
            obj->errorsSent = errCounters;
            obj->errorsReceived = errCounters;
            obj->discardPacketsSent = counters.txDroppedFrames;
            obj->discardPacketsReceived = counters.rxDroppedFrames;
            // tell upper layers the value has changed
            ret = CMSRET_SUCCESS;
        }
    }

    return ret;
}

static CmsRet updateActiveEthernetStats(OpticalInterfaceStatsObject *obj)
{
    CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
    UINT32 bytesSent = 0, bytesReceived = 0, packetsSent = 0, packetsReceived = 0;

    // reset statistics counters
    if (obj == NULL)
    {
        rut_clearIntfStats(AE_INTF_NAME);
    }
    else
    {		
        rut_getIntfStats(AE_INTF_NAME, &bytesReceived, &packetsReceived, 
            &(obj->X_BROADCOM_COM_MulticastBytesReceived), &(obj->X_BROADCOM_COM_MulticastPacketsReceived), &(obj->X_BROADCOM_COM_UnicastPacketsReceived), &(obj->X_BROADCOM_COM_BroadcastPacketsReceived), 
            &(obj->errorsReceived), &(obj->discardPacketsReceived),
            &bytesSent, &packetsSent, 
            &(obj->X_BROADCOM_COM_MulticastBytesSent), &(obj->X_BROADCOM_COM_MulticastPacketsSent), &(obj->X_BROADCOM_COM_UnicastPacketsSent), &(obj->X_BROADCOM_COM_BroadcastPacketsSent),
            &(obj->errorsSent), &(obj->discardPacketsSent)); 

        obj->bytesReceived = bytesReceived;
        obj->packetsReceived = packetsReceived;
        obj->bytesSent= bytesSent;
        obj->packetsSent= packetsSent;

        // tell upper layers the value has changed
        ret = CMSRET_SUCCESS;
    }

    return ret;
}
#endif

CmsRet stl_deviceOpticalObject(_DeviceOpticalObject *obj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_opticalInterfaceObject(_OpticalInterfaceObject *obj,
                     const InstanceIdStack *iidStack __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    char prevStatusBuf[BUFLEN_32]={0};

    if (obj != NULL)
    {
       cmsUtl_strncpy(prevStatusBuf, obj->status, sizeof(prevStatusBuf));

       /* Early in the boot sequence, we do not know the status of the optical
        * interface. Change from default value of null to UNKNOWN.
        */
       if (obj->status == NULL)
       {
          obj->status = cmsMem_strdupFlags(MDMVS_UNKNOWN, mdmLibCtx.allocFlags);
       }
    }

#if 0
    ret = updateOpticalStatus(obj);
    if (ret != CMSRET_SUCCESS && ret != CMSRET_SUCCESS_OBJECT_UNCHANGED)
    {
       cmsLog_error("updateOpticalStatus failed, ret=%d", ret);
       return ret;
    }

    ret = updateOpticalSignal(obj);
    if (ret != CMSRET_SUCCESS && ret != CMSRET_SUCCESS_OBJECT_UNCHANGED)
    {
       cmsLog_error("updateOpticalSignal failed, ret=%d", ret);
       return ret;
    }
#endif

    if ((obj != NULL) && (ret == CMSRET_SUCCESS) &&
        cmsUtl_strcmp(prevStatusBuf, obj->status))
    {
       obj->X_BROADCOM_COM_LastChange = cmsTms_getSeconds();
    }

    /* Calculate and return the TR181 LastChange */
    IF_OBJ_NOT_NULL_GET_LASTCHANGE(obj);
    ret = CMSRET_SUCCESS;

    return ret;
}

CmsRet stl_opticalInterfaceStatsObject(_OpticalInterfaceStatsObject *obj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
#if 0
    OpticalInterfaceObject *parentObj=NULL;
    InstanceIdStack parentIidStack = *iidStack;

    ret = cmsObj_getAncestor(MDMOID_OPTICAL_INTERFACE, MDMOID_OPTICAL_INTERFACE_STATS, &parentIidStack, (void **) &parentObj);
    if (ret != CMSRET_SUCCESS)
    {
        // very unlikely
        return ret;
    }

    if (parentObj->enable)
    {
        if (isGponActive() == TRUE)
        {
            ret = updateGponStats(obj);
        }
        else
        {
            ret = updateActiveEthernetStats(obj);
        }
    }

    cmsObj_free((void **) &parentObj);
#endif

    return ret;
}

#endif    // DMP_DEVICE2_OPTICAL_1

#endif /* DMP_DEVICE2_BASELINE_1 */

