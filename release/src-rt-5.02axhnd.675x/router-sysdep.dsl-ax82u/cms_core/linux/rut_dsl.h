/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2011:proprietary:standard

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
#ifndef __RUT_DSL_H__
#define __RUT_DSL_H__


/*!\file rut_dsl.h
 * \brief System level interface functions for DSL functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */


#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_atm.h"
#include "devctl_xtm.h"
#include "AdslMibDef.h"

UBOOL8 rutDsl_isDslConfigChanged(const _WanDslIntfCfgObject *newObj, const _WanDslIntfCfgObject *currObj);
CmsRet rutDsl_configUp(WanDslIntfCfgObject *dslIntfCfg);
void rutDsl_configDown(void);

/** get WanDev for ATM/PTM iidStack
 *
 * This function assumes both ATM and PTM WANDevice is initialized by mdm_init
 *
 * @param isAtm   (IN) TRUE is for DSL ATM WanDev, FALSE is PTM WanDev
 * @param iidStack (OUT) iidStack of the WanDev object for ATM/PTM found.
 *
 * @return UBOOL8 indicating whether the desired WanDev for ATM or PTM object was found.
 */
UBOOL8 rutDsl_getDslWanDevIidStack(UBOOL8 isAtm, InstanceIdStack *wanDevIid);


/** find the dslLinkCfg object with the given layer 2 ifName.
 *
 * @param ifName (IN) layer 2 ifName of DslLinkCfg to find.
 * @param iidStack (OUT) iidStack of the DslLinkCfg object found.
 * @param dslLinkCfg (OUT) if not null, this will contain a pointer to the found
 *                         dslLinkCfg object.  Caller is responsible for calling
 *                         cmsObj_free() on this object.
 *
 * @return UBOOL8 indicating whether the desired DslLinkCfg object was found.
 */
UBOOL8 rutDsl_getDslLinkByIfName(char *ifName, InstanceIdStack *iidStack, WanDslLinkCfgObject **dslLinkCfg);


/** find the ptmLinkCfg object with the given layer 2 ifName.
 *
 * @param ifName (IN) layer 2 ifName of ptmLinkCfg to find.
 * @param iidStack (OUT) iidStack of the ptmLinkCfg object found.
 * @param dslLinkCfg (OUT) if not null, this will contain a pointer to the found
 *                         ptmLinkCfg object.  Caller is responsible for calling
 *                         cmsObj_free() on this object.
 *
 * @return UBOOL8 indicating whether the desired ptmLinkCfg object was found.
 */
UBOOL8 rutDsl_getPtmLinkByIfName(char *ifName, InstanceIdStack *iidStack, WanPtmLinkCfgObject **ptmLinkCfg);



/** Create layer 2 interface name from interfac type (ATM_EoA, PTM_EoA, IPoA, PPPoA)
 * @param ifNameType (IN) interface type
 * @param ifName (OUT) layer 2 ifName.
 *
 * @return CmsRet enum.
 */
CmsRet rutDsl_fillL2IfName(const Layer2IfNameType ifNameType, char **ifName);


/** Initialize IPoA interface
 * 
 * @param *iidStack     (IN) iidStack of WanIpConnObject. 
 * @param *newObj       (IN) the new _WanPppConnObject.
 *
 * @return CmsRet enum
 */
CmsRet rutDsl_initIPoA(const InstanceIdStack *iidStack, _WanIpConnObject *newObj);



/** Initialize PPPoA interface
 * 
 * @param *iidStack     (IN) iidStack of object. 
 * @param *newObj       (IN) the new object.
 *
 * @return CmsRet enum
 */
CmsRet rutDsl_initPPPoA(const InstanceIdStack *iidStack,void *obj);
CmsRet rutDsl_initPPPoA_igd(const InstanceIdStack *iidStack,void *obj);
CmsRet rutDsl_initPPPoA_dev2(const InstanceIdStack *iidStack,void *obj);
#if defined(SUPPORT_DM_LEGACY98)
#define rutDsl_initPPPoA(a,b)   rutDsl_initPPPoA_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutDsl_initPPPoA(a,b)   rutDsl_initPPPoA_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutDsl_initPPPoA(a,b)   rutDsl_initPPPoA_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutDsl_initPPPoA(a,b)   (cmsMdm_isDataModelDevice2() ?  \
                                     rutDsl_initPPPoA_dev2((a),(b)) : \
                                     rutDsl_initPPPoA_igd((a),(b)))
#endif

/** Fill in the WanDslIntfStatsTotalObject based on current status as reported by driver.
 *
 * @param (IN) obj: DSL Interface Stats Total object to be filled in.
 * @param (IN) iidStack: iidStack of the object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_getAdslTotalStats(WanDslIntfStatsTotalObject *obj, const InstanceIdStack *iidStack);

/** Clear DSL stastictics object.
 *
 * @param (IN) lineId: DSL line ID.
 * @return CmsRet enum.
 */
CmsRet rutWan_clearAdslTotalStats(UINT32 lineId);

/** Fill in the WanDslIntfStatsShowTime Object based on current status as reported by driver.
 *
 * @param (IN) obj: DSL Interface Stats Showtime object to be filled in.
 * @param (IN) iidStack: iidStack of the object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_getAdslShowTimeStats(WanDslIntfStatsShowtimeObject *obj, const InstanceIdStack *iidStack);


/** Fill in the WanDslIntfStatsCurrentDay Object based on current status as reported by driver.
 *
 * @param (IN) obj: DSL Interface Stats Current Day object to be filled in.
 * @param (IN) iidStack: iidStack of the object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_getAdslCurrentDayStats(WanDslIntfStatsCurrentDayObject *obj, const InstanceIdStack *iidStack);


/** Fill in the WanDslIntfStatsQuarterHourStats Object based on current status as reported by driver.
 *
 * @param (IN) obj: DSL Interface Stats Quarter Hour object to be filled in.
 * @param (IN) iidStack: iidStack of the object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_getAdslQuarterHourStats(WanDslIntfStatsQuarterHourObject *obj, const InstanceIdStack *iidStack);

/** Fill in the WanDslTestParamsObject Object based on current status as reported by driver.
 *
 * @param (IN) obj: DSL Interface Test Parameters object to be filled in.
 * @param (IN) iidStack: iidStack of the object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_getAdslTestParamsInfo(void *obj, const InstanceIdStack *iidStack);

/** Fill in the DslIntfCfg object based on current status as reported by driver.
 *
 * @param (OUT) obj: DSL Interface object to be filled in.
 * @param (IN) iidStack: iidStack of the object.
 *
 * @return CmsRet enum.
 */
CmsRet rutDsl_getIntfInfo(WanDslIntfCfgObject *obj, const InstanceIdStack *iidStack);

/** Fill in the WanBertTestObject as reported by driver.
 *
 * @param (IN) obj: BERT Test object to be filled in.
 *
 * @return CmsRet enum.
 */
CmsRet rutWan_getAdslBertInfo(void *obj, const InstanceIdStack *iidStack);
CmsRet rutWan_getAdslBertInfo_igd(void *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getAdslBertInfo_dev2(void *obj, const InstanceIdStack *iidStack);
#if defined(SUPPORT_DM_LEGACY98)
#define rutWan_getAdslBertInfo(a,b)   rutWan_getAdslBertInfo_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutWan_getAdslBertInfo(a,b)   rutWan_getAdslBertInfo_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_getAdslBertInfo(a,b)   rutdsl_getAdslBertInfo_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutWan_getAdslBertInfo(a,b)   (cmsMdm_isDataModelDevice2() ?  \
                                     rutdsl_getAdslBertInfo_dev2((a),(b)) : \
                                     rutWan_getAdslBertInfo_igd((a),(b)))
#endif

/** SET the WanBertTestObject to start doing BERT test
 *
 * @param (IN) new: new BERT Test object.
 * @param (IN) curr: current BERT Test object.
 * @param (IN) curr: instance
 * @return CmsRet enum.
 */
CmsRet rutWan_setAdslBertInfo(void *new, const void *curr, const InstanceIdStack *iidStack);
CmsRet rutWan_setAdslBertInfo_igd(void *new, const void *curr, const InstanceIdStack *iidStack);
CmsRet rutdsl_setAdslBertInfo_dev2(void *new, const void *curr, const InstanceIdStack *iidStack);
#if defined(SUPPORT_DM_LEGACY98)
#define rutWan_setAdslBertInfo(a,b,c)   rutWan_setAdslBertInfo_igd((a),(b),(c))
#elif defined(SUPPORT_DM_HYBRID)
#define rutWan_setAdslBertInfo(a,b,c)   rutWan_setAdslBertInfo_igd((a),(b),(c))
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_setAdslBertInfo(a,b,c)   rutdsl_setAdslBertInfo_dev2((a),(b),(c))
#elif defined(SUPPORT_DM_DETECT)
#define rutWan_setAdslBertInfo(a,b,c)   (cmsMdm_isDataModelDevice2() ?  \
                                         rutdsl_setAdslBertInfo_dev2((a),(b),(c)) : \
                                         rutWan_setAdslBertInfo_igd((a),(b),(c)))
#endif


/** Return true if DSL bonding is enabled.
 *
 * @return TRUE if DSL bonding is enabled.
 */
UBOOL8 rutDsl_isDslBondingEnabled(void);

CmsRet rutWan_getSeltStatus(void *obj);

#ifdef DMP_DSLDIAGNOSTICS_1
CmsRet rutWan_getAdslLoopDiagStatus(void *obj, const InstanceIdStack *iidStack);
CmsRet rutWan_getAdslLoopDiagStatus_igd(void *obj, const InstanceIdStack *iidStack);
CmsRet rutdsl_getAdslLoopDiagStatus_dev2(void *obj, const InstanceIdStack *iidStack);
#if defined(SUPPORT_DM_LEGACY98)
#define rutWan_getAdslLoopDiagStatus(a,b)   rutWan_getAdslLoopDiagStatus_igd((a),(b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutWan_getAdslLoopDiagStatus(a,b)   rutWan_getAdslLoopDiagStatus_igd((a),(b))
#elif defined(SUPPORT_DM_PURE181)
#define rutWan_getAdslLoopDiagStatus(a,b)   rutdsl_getAdslLoopDiagStatus_dev2((a),(b))
#elif defined(SUPPORT_DM_DETECT)
#define rutWan_getAdslLoopDiagStatus(a,b)   (cmsMdm_isDataModelDevice2() ? \
                                             rutdsl_getAdslLoopDiagStatus_dev2((a),(b)) : \
                                             rutWan_getAdslLoopDiagStatus_igd((a),(b)))
#endif
#endif /* #ifdef DMP_DSLDIAGNOSTICS_1 */
#endif  /* __RUT_DSL_H__ */

