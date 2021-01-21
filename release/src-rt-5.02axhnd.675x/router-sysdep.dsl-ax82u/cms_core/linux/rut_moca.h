/***********************************************************************
 *
 *  Copyright (c) 2009  Broadcom Corporation
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

#ifndef __RUT_MOCA_H__
#define __RUT_MOCA_H__

#ifdef SUPPORT_MOCA

/*!\file rut_moca.h
 * \brief System level interface functions for MOCA functionality.
 *
 * In most cases, the functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"
#include "cms_core.h"



/** find the WanMocaIntf object with the given layer 2 ifName.
 *
 * @param ifName (IN) layer 2 ifName of WanMocaIntf to find.
 * @param iidStack (OUT) iidStack of the WanMocaIntf object found.
 * @param ethIntfCfg (OUT) if not null, this will contain a pointer to the found
 *                         WanMocaIntf object.  Caller is responsible for calling
 *                         cmsObj_free() on this object.
 *
 * @return UBOOL8 indicating whether the desired WanMocaIntf object was found.
 */
UBOOL8 rutMoca_getWanMocaIntfByIfName(const char *ifName, InstanceIdStack *iidStack, WanMocaIntfObject **mocaIntfCfg);


/** Start the moca core.  
 * 
 * This function will start the moca core if it is currently stopped.
 *
 * @param mocaObj (IN) The LanMocaIntfObject or WanMocaIntfObject, both of
 *                     these objects are exactly the same.
 *
 * @return CmsRet enum.
 */
CmsRet rutMoca_start(const char *intfName,
      UBOOL8 *autoNwSearch, UBOOL8 *privacy,
      UINT32 *lastOperationalFrequency,
      char **password, char **initParmsString);


/** Stop the moca core.
 * 
 * This function will stop the moca core if it is currently started.
 *
 * @param intfName     (IN) Linux interface name, e.g. moca0
 *
 * @return CmsRet enum.
 */
CmsRet rutMoca_stop(const char *intfName);


/** Initialize the moca driver in the system.  
 * 
 * This function can be called multiple times, but it will only run once.
 *
 * @param intfName     (IN) Linux interface name, e.g. moca0
 * @param lastOperFreq (IN) last operational frequency
 *
 * @return CmsRet enum.
 */
CmsRet rutMoca_initialize(const char *intfName, UINT32 lastOperFreq);


/** Reinitialize the moca core.  
 * 
 * This function will reinitialize the MoCA core and will use up-to-date
 * parameters when doing so.
 *
 * @param mocaObj (IN) The LanMocaIntfObject or WanMocaIntfObject, both of
 *                     these objects are exactly the same.
 *
 * @return CmsRet enum.
 */
CmsRet rutMoca_reinitialize(const char *intfName,
      UBOOL8 *autoNwSearch, UBOOL8 *privacy,
      UINT32 *lastOperationalFrequency,
      char **password, char **initParmsString);


/** Configure the moca core.  
 * 
 * This function will check which parameters need to be set by comparing 
 * mocaObj with currObj and will set the appropriate parameters in the
 * moca core.
 *
 * @param mocaObj (IN) The new LanMocaIntfObject or WanMocaIntfObject, both
 *                     of these objects are exactly the same.
 * @param currObj (IN) The current LanMocaIntfObject parameters, both of
 *                     these objects are exactly the same.
 *
 * @return CmsRet enum.
 */
CmsRet rutMoca_setParams(LanMocaIntfObject *mocaObj, const LanMocaIntfObject *currObj);


/** Set the tracing functionality in the moca driver.
 *
 * @param mocaObj (IN) The LanMocaIntfObject or WanMocaIntfObject, both of
 *                     these objects are exactly the same. Field 
 *                     traceParmsString should be set.
 *
 * @return CmsRet enum
 */
CmsRet rutMoca_setTrace(LanMocaIntfObject *mocaObj);


/** Return a string of the link status of the specified moca device.
 *
 * @param ifName (IN) The interface name being queried.
 *
 * @return a string indicating the link status.
 */
const char *rutMoca_getLinkStatus(const char *ifName);


/** Get the current parameters of the specified moca device.
 *
 * @param ifName (IN) The interface name being queried.
 * @param mocaObj (OUT) On successful return, this moca status object will be
 *                         filled in with the interface info.
 *
 * @enum CmsRet enum.
 */
CmsRet rutMoca_getInterfaceData(const char *ifName, LanMocaIntfObject *mocaObj);


/** Get the status of the specified moca device.
 *
 * @param ifName (IN) The interface name being queried.
 * @param mocaStatus (OUT) On successful return, this moca status object will be
 *                         filled in with the status info.
 *
 * @enum CmsRet enum.
 */
CmsRet rutMoca_getStatus(const char *ifName, LanMocaIntfStatusObject *mocaStatus);


/** TR181 version of get interface status and data
 *
 */
void rutMoca_getIntfInfo_dev2(const char *intfName, Dev2MocaInterfaceObject *obj);


/** Get the stats of the specified moca device.
 *
 * @param ifName (IN) The interface name being queried.
 * @param mocaStatus (OUT) On successful return, this moca stats object will be
 *                         filled in with the status info.
 *
 * @enum CmsRet enum.
 */
CmsRet rutMoca_getStats(const char *ifName, LanMocaIntfStatsObject *mocaStats);


/** TR181 version of get interface statistics
 *
 */
void rutMoca_getStats_dev2(const char *intfName, Dev2MocaInterfaceStatsObject *obj);


/** Reset the stats of the specified moca device.
 *
 * @param ifName (IN) The interface name to have its stats reset.
 *
 * @enum CmsRet enum.
 */
CmsRet rutMoca_resetStats(const char *ifName);


/** Find the TR-098 moca object that corresponds to this GPON object.  
 *
 * This function is used by the GPON moca objects to sync any OMCI writes up to the
 * TR-098 data model.  I think in the GPON moca configuration, the corresponding
 * moca object should only be on the LAN side (not on the WAN side).  So return the
 * the moca object on the LAN side only.
 *
 * @param gponIidStack (IN) The iidstack of the gpon moca object.
 * @param mocaObj     (OUT) The corresponding moca object in the TR-098 data model.
 * @param iidStack    (OUT) The iidStack of the corresponding moca object in the TR-098 data model.
 *
 * @return CmsRet enum.
 */
CmsRet rutMoca_findPrimaryMocaObject(const InstanceIdStack *gponIidStack,
                                     LanMocaIntfObject **mocaObj,
                                     InstanceIdStack *iidStack);


/** Update the initialization parameters in the data model.
 *
 * This function will update the mocaObj structure with the current
 * initialization parameters in use in the moca interface. The initParmsString
 * will be updated in proper format.
 *
 * @param mocaCtx (IN) The handle to the MoCA interface name to retrieve init
 *                     parameters from. If NULL, the function will open a handle
 *                     to the interface specified by mocaObj->ifName.
 *
 *
 * @enum CmsRet enum.
 */
CmsRet rutMoca_getInitParms(void * mocaCtx, const char *intfName,
                            UBOOL8 *autoNwSearch, UBOOL8 *privacy,
                            UINT32 *lastOperationalFrequency,
                            char **password, char **initParmsString);


/** Update the configuration parameters in the data model.
 *
 * This function will update the mocaObj structure with the current
 * configuration parameters in use in the moca interface. The configParmsString
 * will be updated in proper format.
 *
 * @param mocaCtx (IN) The handle to the MoCA interface name to retrieve config
 *                     parameters from. If NULL, the function will open a handle
 *                     to the interface specified by mocaObj->ifName.
 * @param mocaObj (IN/OUT) The moca interface object to be updated with config
 *                         parameters. 
 *
 * @enum CmsRet enum.
 */
CmsRet rutMoca_updateConfigParms(void * mocaCtx, LanMocaIntfObject *mocaObj);


/** Update the trace parameters in the data model.
 *
 * This function will update the mocaObj structure with the current
 * trace parameters in use in the moca interface. The traceParmsString
 * will be updated in proper format.
 *
 * @param mocaCtx (IN) The handle to the MoCA interface name to retrieve trace
 *                     parameters from. If NULL, the function will open a handle
 *                     to the interface specified by mocaObj->ifName.
 * @param mocaObj (IN/OUT) The moca interface object to be updated with trace
 *                         parameters. 
 *
 * @enum CmsRet enum.
 */
CmsRet rutMoca_updateTraceParms(void * mocaCtx, LanMocaIntfObject *mocaObj);


#endif /* SUPPORT_MOCA */ 
#endif  /* __RUT_MOCA_H__ */
