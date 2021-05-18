/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
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

#ifndef __RUT_QOS_H__
#define __RUT_QOS_H__


/*!\file rut_qos.h
 * \brief System level interface functions for QoS functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */



#include "cms.h"
#include "cms_qos.h"
#include "qdm_qos.h"
#include "rut_tmctl_wrap.h"
#if defined(SUPPORT_RDPA)
#include "rdpactl_api.h"
#endif

typedef enum
{
   QOS_COMMAND_NONE     = 0,
   QOS_COMMAND_CONFIG   = 1,
   QOS_COMMAND_UNCONFIG = 2

} QosCommandType;


/** This is the main function for configuring a classification into the
 *  system/kernel.  This function is data model independent.  So it can
 *  be called by TR98 or TR181 code.  But you still pass in a TR98 or
 *  TR181 QoS classification object.  This function will do the conversion
 *  internally.
 *
 *  @param cmdType (IN) one of the QosCommandType enum above
 *  @param clsObj  (IN) TR98 or TR181 QoS Classification object.
 *
 *  @return CmsRet enum
 */
CmsRet rutQos_qMgmtClassConfig(QosCommandType cmdType, const void *clsObj);




/** This is the main function for configuring a queue.
 *  It configures all types of queues (XTM, FAP, RDPA, xPON) queues.
 *  But it only configures the specified queue.  No side effects.
 *  This function is data model independent, so can be called by TR98 and
 *  TR181 code.  But you still pass in a TR98 or TR181 QoS Queue object.
 *  The function will do the conversion internally.
 *
 *  @param cmdType (IN) one of the QosCommandType enum above
 *  @param qObj    (IN) TR98 or TR181 QoS Queue object.
 *
 *  @return CmsRet enum
 */
CmsRet rutQos_qMgmtQueueConfig(QosCommandType cmdType, const void *qObj);




CmsRet rutQos_rddTmEthQueueConfig(const _QMgmtQueueObject *qObj,
                                  QosCommandType cmdType);




/** This is the top level function that handles layer 2 intf link status
 *  changes and layer 3 WAN service status changes.  It contains high level
 *  logic for reconfiguring queues and classifications.  (Classifications
 *  includes policing and rate limiter).
 *  Seems like in TR98 code, it is only called on Link UP, but not Link down.
 *
 * @param ifname     (IN) name of interface that changed link status
 * @param layer2Intf (IN) TRUE if ifname is layer2 interface name. Otherwise, FALSE.
 * @return CmsRet         enum.
 */
CmsRet rutQos_qMgmtQueueReconfig(char *ifcname, UBOOL8 layer2Intf);
CmsRet rutQos_qMgmtQueueReconfig_igd(char *ifcname, UBOOL8 layer2Intf);

/* No data model switching macro needed for rutQos_qMgmtQueueReconfig */
/* All TR98 code call rutQos_qMgmtQueueReconfig() */
#define rutQos_qMgmtQueueReconfig(i, b)  rutQos_qMgmtQueueReconfig_igd((i), (b))


/* All TR181 layer 2 interfaces must call this function on Layer 2
 * link up/down.  Unlike the TR98 rutQos_qMgmtQueueReconfig, this
 * function only configures the Layer 2 queues.  It does not touch
 * the classifications.  (For classifications,
 * call void rutQos_reconfigAllClassifications_dev2)
 */
void rutQos_reconfigAllQueuesOnLayer2Intf_dev2(const char *l2IntfName);


/* All TR181 layer 2 interfaces must call this function on layer 2 link up/down.
 * This function re-configures shaper object on a specific layer 2 interface.
 */
void rutQos_reconfigShaperOnLayer2Intf_dev2(const char *l2IntfName);


/* Delete or unconfigure all the QoS queues associated with a layer 2 interface.
 * This function is used by TR98 code only.
 * isRealDel=TRUE: atm/ptm intf is being disabled or deleted
 *                 eth, moca, or GPON is being removed from bridge (rutLan_removeInterfaceFromBridge)
 *                 eth or moca being moved LAN <==> WAN
 * isRealDel=FALSE: atm/ptm link down
 *
 */
CmsRet rutQos_qMgmtQueueOperation(const char *l2Ifcname, UBOOL8 isRealDel);
CmsRet rutQos_qMgmtQueueOperation_igd(const char *l2Ifcname, UBOOL8 isRealDel);


/* No data model switching macro needed for rutQos_qMgmtQueueOperation */
/* All TR98 code call rutQos_qMgmtQueueOperation */
#define rutQos_qMgmtQueueOperation(i, b)  rutQos_qMgmtQueueOperation_igd((i), (b))


/* TR181 code will call reconfigAllQueuesOnLayer2Intf_dev2 for isRealDel=FALSE,
 * and call rutQos_deleteQueues_dev2() when isRealDel=TRUE.
 */

/** Delete all QoS queues associated with the specified intf
 */
void rutQos_deleteQueues_dev2(const char *intfName);




/** Traverse through all the QoS queues matching l2Ifname and fill in
 *  array to indicate which queueIds are already in use.
 */
CmsRet rutQos_fillQueueIdArray(const char *l2IfName, UINT32 maxQId, char *idArray);

CmsRet rutQos_fillQueueIdArray_igd(const char *l2IfName, UINT32 maxQId, char *idArray);

CmsRet rutQos_fillQueueIdArray_dev2(const char *l2IfName, UINT32 maxQId, char *idArray);


#if defined(SUPPORT_DM_LEGACY98)
#define rutQos_fillQueueIdArray(i, m, a)  rutQos_fillQueueIdArray_igd((i), (m), (a))
#elif defined(SUPPORT_DM_HYBRID)
#define rutQos_fillQueueIdArray(i, m, a)  rutQos_fillQueueIdArray_igd((i), (m), (a))
#elif defined(SUPPORT_DM_PURE181)
#define rutQos_fillQueueIdArray(i, m, a)  rutQos_fillQueueIdArray_dev2((i), (m), (a))
#elif defined(SUPPORT_DM_DETECT)
#define rutQos_fillQueueIdArray(i, m, a)  (cmsMdm_isDataModelDevice2() ? \
                             rutQos_fillQueueIdArray_dev2((i), (m), (a)) : \
                             rutQos_fillQueueIdArray_igd((i), (m), (a)))
#endif

/* Init QoS settings and create a default queue of the specified Ethernet port. */
CmsRet rutQos_tmPortInit(const char *l2ifname, UBOOL8 isWan);

/* Un-init QoS settings of the specified Ethernet port. */
CmsRet rutQos_tmPortUninit(const char *l2ifname, UBOOL8 isWan);

/* Configures or Un-configures a queue by queueId of specified interface.
 * It configures the queue weight, precedence, scheduler algorithm,
 * shaping rate, shaping burst size, and minimum rate. */
CmsRet rutQos_tmQueueConfig(QosCommandType cmdType, CmsQosQueueInfo *qInfo);

/* Configures the port shaper parameters of the specified interface.
 * It configures shaper rate in kbps and bust size in bytes. */
CmsRet rutQos_tmPortShaperCfg(const char* l2ifname, SINT32 portbps, SINT32 portMbs,
                              char* status, UBOOL8 isWan __attribute__((unused)));


/* Get queue statistics by queueId of specified port. */
CmsRet rutQos_tmGetQueueStats(const char* l2ifname, SINT32 queueId,
                              tmctl_queueStats_t *queueStats);

/** TR98 way of configuring and unconfiguring all classifiers.  Also
 *  does policing and rate limiting.
 */
CmsRet rclQos_classConfig(const _QMgmtClassificationObject *newObj, UBOOL8 isAll);
CmsRet rclQos_classUnconfig(const _QMgmtClassificationObject *cObj);


/** TR181 version of Configure or Unconfigure all classifications.  Also
 *  does policing and rate limiting.  This function is called when a
 *  Layer 3 service goes UP or DOWN or when a Layer 2 interface link
 *  goes UP or DOWN.
 *
 *  @intfName  (IN)  Layer 2 or Layer 3 interface.
 */
void rutQos_reconfigAllClassifications_dev2(const char *intfName);




CmsRet rutQos_convertPrecedenceToPriority(const char *qIntfPath, UINT32 prec, UINT32 *prio);
CmsRet rutQos_getAvailableQueueId(const char *l2Ifname,
                                  UINT32 prio, const char *alg, UINT32 *qId);


/** Get the number of Tx Queues and Levels on this given interface.
 *
 * @param l2IfName  (IN)  layer2 interface name for the queue.
 * @param numQueues (OUT) If pointer is not NULL, will be filled in with num
 *                        TX queues available on this intf.
 * @param numLevels (OUT) If pointer is not NULL, will be filled in with num
 *                        Levels available on this intf.
 *
 */
void dalQos_getIntfNumQueuesAndLevels(const char *l2IfName,
                                      UINT32 *numQueues, UINT32 *numLevels);



/** Fill the given array with which keys are already in use.
 *
 */
CmsRet rutQos_fillClassKeyArray(UINT32 *keyArray);
CmsRet rutQos_fillClassKeyArray_igd(UINT32 *keyArray);
CmsRet rutQos_fillClassKeyArray_dev2(UINT32 *keyArray);

#if defined(SUPPORT_DM_LEGACY98)
#define rutQos_fillClassKeyArray(i)  rutQos_fillClassKeyArray_igd((i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutQos_fillClassKeyArray(i)  rutQos_fillClassKeyArray_igd((i))
#elif defined(SUPPORT_DM_PURE181)
#define rutQos_fillClassKeyArray(i)  rutQos_fillClassKeyArray_dev2((i))
#elif defined(SUPPORT_DM_DETECT)
#define rutQos_fillClassKeyArray(i)  (cmsMdm_isDataModelDevice2() ? \
                                      rutQos_fillClassKeyArray_dev2((i)) : \
                                      rutQos_fillClassKeyArray_igd((i)))
#endif



/** Find an unused class key
 *
 */
CmsRet rutQos_getAvailableClsKey(UINT32 *clsKey);




/** Delete all classification entries which has this layer 3 interface
 *  as the ingress or egress interface.  This function is called when a
 *  Layer 3 WAN service is deleted.
 */
CmsRet rutQos_qMgmtClassDelete(const char *l3Ifcname);
CmsRet rutQos_qMgmtClassDelete_igd(const char *l3Ifcname);
CmsRet rutQos_qMgmtClassDelete_dev2(const char *l3Ifcname);

#if defined(SUPPORT_DM_LEGACY98)
#define rutQos_qMgmtClassDelete(i)  rutQos_qMgmtClassDelete_igd((i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutQos_qMgmtClassDelete(i)  rutQos_qMgmtClassDelete_igd((i))
#elif defined(SUPPORT_DM_PURE181)
#define rutQos_qMgmtClassDelete(i)  rutQos_qMgmtClassDelete_dev2((i))
#elif defined(SUPPORT_DM_DETECT)
#define rutQos_qMgmtClassDelete(i)  (cmsMdm_isDataModelDevice2() ? \
                                     rutQos_qMgmtClassDelete_dev2((i)) : \
                                     rutQos_qMgmtClassDelete_igd((i)))
#endif


/*
 *  In TR181, when layer 2 ATM/PTM interface is deleted, the associated
 *  QoS queues are deleted, and the RCL handler function for the QoS queues
 *  will delete all the classifiers with EGRESS queue on the deleted
 *  queue instance.
 */
void rutQos_deleteClassByEgressQueueInstance_dev2(SINT32 queueInstance);


/** Used by rcl_dev2QosClassificationObject to determine if the
 * classification object has been changed.  Note this func only looks
 * at the real classification parameters, but not status or name.
 */
UBOOL8 rutQos_isClassificationChanged_dev2(const Dev2QosClassificationObject *newObj,
                                  const Dev2QosClassificationObject *currObj);




/** TR98 only function.  Used when wan service goes down or is deleted.
 *
 */
CmsRet rutQos_qMgmtClassOperation(SINT32 queueInstance, UBOOL8 isRealDel);
CmsRet rutQos_qMgmtClassOperation_igd(SINT32 queueInstance, UBOOL8 isRealDel);

#define rutQos_qMgmtClassOperation(i, r)  rutQos_qMgmtClassOperation_igd((i), (r))

/* TR181 code should call rutQos_reconfigAllClassifications_dev2 when
 * wan service goes down.  And call rutQos_qMgmtClassDelete_dev2 for delete.
 */




/** This is now an internal helper function.  Call qdmQos_referenceCheckLocked
 *  instead.
 */
CmsRet rutQos_referenceCheck(MdmObjectId oid, SINT32 instance, UBOOL8 *isRefered);




/** Return TRUE if there is one or more classification object with an
 *  egress queue on the specified l2IntfName.  Used by policer code to
 *  determine if the entire policer Queue Disc can be deleted.
 */
UBOOL8 rutQos_isAnotherClassPolicerExist(UINT32 excludeClsKey,
                                         const char *egressL2IntfName);

UBOOL8 rutQos_isAnotherClassPolicerExist_igd(UINT32 excludeClsKey,
                                         const char *egressL2IntfName);

UBOOL8 rutQos_isAnotherClassPolicerExist_dev2(UINT32 excludeClsKey,
                                         const char *egressL2IntfName);

#if defined(SUPPORT_DM_LEGACY98)
#define rutQos_isAnotherClassPolicerExist(x, i)  rutQos_isAnotherClassPolicerExist_igd((x), (i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutQos_isAnotherClassPolicerExist(x, i)  rutQos_isAnotherClassPolicerExist_igd((x), (i))
#elif defined(SUPPORT_DM_PURE181)
#define rutQos_isAnotherClassPolicerExist(x, i)  rutQos_isAnotherClassPolicerExist_dev2((x), (i))
#elif defined(SUPPORT_DM_DETECT)
#define rutQos_isAnotherClassPolicerExist(x, i)  (cmsMdm_isDataModelDevice2() ? \
                       rutQos_isAnotherClassPolicerExist_dev2((x), (i)) : \
                       rutQos_isAnotherClassPolicerExist_igd((x), (i)))
#endif



/** Return TRUE if there is one or more classification object with a valid
 *  rate limit.  Used by rate limit code to determine if the entire
 *  rate limit Queue Disc can be deleted.
 */
UBOOL8 rutQos_isAnotherClassRateLimitExist(UINT32 excludeClsKey,
                                           const char *egressL2IntfName);

UBOOL8 rutQos_isAnotherClassRateLimitExist_igd(UINT32 excludeClsKey,
                                           const char *egressL2IntfName);

UBOOL8 rutQos_isAnotherClassRateLimitExist_dev2(UINT32 excludeClsKey,
                                           const char *egressL2IntfName);

#if defined(SUPPORT_DM_LEGACY98)
#define rutQos_isAnotherClassRateLimitExist(x, i)  rutQos_isAnotherClassRateLimitExist_igd((x), (i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutQos_isAnotherClassRateLimitExist(x, i)  rutQos_isAnotherClassRateLimitExist_igd((x), (i))
#elif defined(SUPPORT_DM_PURE181)
#define rutQos_isAnotherClassRateLimitExist(x, i)  rutQos_isAnotherClassRateLimitExist_dev2((x), (i))
#elif defined(SUPPORT_DM_DETECT)
#define rutQos_isAnotherClassRateLimitExist(x, i)  (cmsMdm_isDataModelDevice2() ? \
                       rutQos_isAnotherClassRateLimitExist_dev2((x), (i)) : \
                       rutQos_isAnotherClassRateLimitExist_igd((x), (i)))
#endif


/** This function update a QoS policer X_BROADCOM_COM_UsPolicerInfo and X_BROADCOM_COM_DsPolicerInfo.
 *
 * @param instance       (IN) instance of the policer obj
 * @param dir            (IN) direction of the policer info
 * @param policerInfo    (IN) policerInfo
 * @return CmsRet         enum.
 */
CmsRet rutQos_fillPolicerInfo(const SINT32 instance, const tmctl_dir_e dir, const UINT32 policerInfo);
CmsRet rutQos_fillPolicerInfo_igd(const SINT32 instance, const tmctl_dir_e dir, const UINT32 policerInfo);
CmsRet rutQos_fillPolicerInfo_dev2(const SINT32 instance, const tmctl_dir_e dir, const UINT32 policerInfo);

#if defined(SUPPORT_DM_LEGACY98)
#define rutQos_fillPolicerInfo(i, d, p)           rutQos_fillPolicerInfo_igd((i), (d), (p))
#elif defined(SUPPORT_DM_HYBRID)
#define rutQos_fillPolicerInfo(i, d, p)           rutQos_fillPolicerInfo_igd((i), (d), (p))
#elif defined(SUPPORT_DM_PURE181)
#define rutQos_fillPolicerInfo(i, d, p)           rutQos_fillPolicerInfo_dev2((i), (d), (p))
#elif defined(SUPPORT_DM_DETECT)
#define rutQos_fillPolicerInfo(i, d, p)  (cmsMdm_isDataModelDevice2() ? \
                                       rutQos_fillPolicerInfo_dev2(((i), (d), (p))) : \
                                       rutQos_fillPolicerInfo_igd(((i), (d), (p))))
#endif


/** Given the various parameters of the QoS Queue, return the qidMark.
 *  This is a data model independent function.  Currently only used
 *  by the QDM layer as a helper function.
 *
 *  @return qidMark.
 */
UINT32 rutQos_getQidMark(const char *queueIntfName,
                         UBOOL8 isWan,
                         UINT32 queueId,
                         UINT32 queuePrecedence,
                         char *schedulerAlgorithm,
                         SINT32 dslLat,
                         SINT32 ptmPriority);


/** configure all Ethernet port shapings on LAN and WAN side.
 *  This is only used by TR98 code.  In TR181, we configure port shaping
 *  for a specific ethernet port when the ethernet port changes link status
 *  by calling rutQos_portShapingConfig.
 *
 */
void rutQos_portShapingConfigAll(void);
void rutQos_portShapingConfigAll_igd(void);

#define rutQos_portShapingConfigAll()  rutQos_portShapingConfigAll_igd()




#ifdef SUPPORT_POLICING
CmsRet rutQos_policer(QosCommandType cmdType,
                      const char *queueIfname,
                      const CmsQosClassInfo *cInfo,
                      const CmsQosPolicerInfo *pInfo);
#endif

UBOOL8 rutQos_isPolicerChanged_dev2(const Dev2QosPolicerObject *newObj,
                                    const Dev2QosPolicerObject *currObj);

void rutQos_deletePolicerByInstance_dev2(SINT32 instance);




#ifdef SUPPORT_RATE_LIMIT
CmsRet rutQos_rateLimit(QosCommandType cmdType,
                        const char *queueIntfName,
                        UINT32 clsKey,
                        SINT32 classRate);
#endif




/** Update the interface fullpaths in the QoS Queue and Classification objs
 *  which is needed when we move an interface from one LAN bridge to another
 *  LAN bridge in TR98.  This is not an issue in TR181 due to the way
 *  the data model implements interface grouping.
 */
void rutQos_updateInterfaceFullPaths(const char *srcIntfFullPath,
                                     const char *destIntfFullPath);


/** This function sets the default Strict Priority queues associated
 *  with the Ethernet interface.  It is called when is put in service.
 *
 * @param ifname (IN) Ethernet interface name.
 * @param isWan (IN) boolean indicating EthWAN or EthLAN
 */
CmsRet rutQos_setDefaultEthQueues(const char *ifname, UBOOL8 isWan);
CmsRet rutQos_setDefaultEthQueues_igd(const char *ifname, UBOOL8 isWan);
CmsRet rutQos_setDefaultEthQueues_dev2(const char *ifname, UBOOL8 isWan);

#if defined(SUPPORT_DM_LEGACY98)
#define rutQos_setDefaultEthQueues(a, b) \
                           rutQos_setDefaultEthQueues_igd((a), (b))
#elif defined(SUPPORT_DM_HYBRID)
#define rutQos_setDefaultEthQueues(a, b) \
                           rutQos_setDefaultEthQueues_igd((a), (b))
#elif defined(SUPPORT_DM_PURE181)
#define rutQos_setDefaultEthQueues(a, b) \
                           rutQos_setDefaultEthQueues_dev2((a), (b))
#elif defined(SUPPORT_DM_DETECT)
#define rutQos_setDefaultEthQueues(a, b) (cmsMdm_isDataModelDevice2() ? \
                           rutQos_setDefaultEthQueues_dev2((a), (b)) : \
                           rutQos_setDefaultEthQueues_igd((a), (b)))
#endif


/** This function enables/disables the queues associated with the
 *  wifi interface.  It is called whenever wifi link is up/down.
 *
 * @param ifname   (IN) wireless interface name.
 * @param enabled  (IN) boolean indicating link up or down
 */
void rutQos_setDefaultWlQueues(const char *ifname, UBOOL8 enabled);
void rutQos_setDefaultWlQueues_igd(const char *ifname, UBOOL8 enabled);

/* No data model switching macro needed for rutQos_setDefaultWlQueues */
/* All TR98 code call rutQos_qMgmtQueueOperation */
#define rutQos_setDefaultWlQueues(i, b)  rutQos_setDefaultWlQueues_igd((i), (b))

/* TR181 code will call rutQos_reconfigAllQueuesOnLayer2Intf_dev2 */



/** activate or deactivate the default classifiers for this wifi interface.
 *  This function is data model independent.
 */
void   rutQos_doDefaultWlPolicy(const char *ifname, UBOOL8 enabled);


/** Configure the default DSCP marking.
 */
void rutQos_doDefaultDSCPMarkPolicy(QosCommandType cmdType, SINT32 defaultDSCPMark);


/** activate or deactivate default/hardcoded classifications which give
 *  certain protocols and flows higher priority.  Curiously, this function
 *  is only available on DSL(?!) platforms.
 */
void rutQos_doDefaultPolicy(void);




/** Return info about presence of "QoS enabled" ATM or PTM WAN connections,
 *  and also Eth WAN connections.  (But not moca, pon, wifi, etc.?!?).
 *  This is a helper function to rutQos_doDefaultPolicy().
 */
CmsRet rutQos_getWanQosInfo(UBOOL8 *bridgeQos, UBOOL8 *routedQos);
CmsRet rutQos_getWanQosInfo_igd(UBOOL8 *bridgeQos, UBOOL8 *routedQos);
CmsRet rutQos_getWanQosInfo_dev2(UBOOL8 *bridgeQos, UBOOL8 *routedQos);

#if defined(SUPPORT_DM_LEGACY98)
#define rutQos_getWanQosInfo(x, i)  rutQos_getWanQosInfo_igd((x), (i))
#elif defined(SUPPORT_DM_HYBRID)
#define rutQos_getWanQosInfo(x, i)  rutQos_getWanQosInfo_igd((x), (i))
#elif defined(SUPPORT_DM_PURE181)
#define rutQos_getWanQosInfo(x, i)  rutQos_getWanQosInfo_dev2((x), (i))
#elif defined(SUPPORT_DM_DETECT)
#define rutQos_getWanQosInfo(x, i)  (cmsMdm_isDataModelDevice2() ? \
                       rutQos_getWanQosInfo_dev2((x), (i)) : \
                       rutQos_getWanQosInfo_igd((x), (i)))
#endif


/** retrieve queue ID from the full path of QoS Queue
 *
 *  @fullPath  (IN)  full path of QoS Queue such as
 *                   Device.QoS.Queue.{i}.
 *  @queueId   (OUT) queue ID is found from full path
 *
 *  @return CmsRet enum
 */
CmsRet rutQos_getQueueIdFromQueueFullPath
   (const char *fullPath,
    UINT32 *queueId);


/** retrieve interface name from the full path of layer2 link
 *
 *  @fullPath    (IN)   full path of layer2 link such as
 *                      Device.Ethernet.Interface.{i}.
 *                      Device.ATM.Link.{i}.
 *                      DEvice.PTM.Link.{i}.
 *  @nameBuf     (OUT)  interface name is found from full path
 *  @nameBufLen  (IN)   interface name length
 *
 *  @return CmsRet enum
 */
CmsRet rutQos_getInterfaceNameFromFullPath
   (const char *fullPath,
    char *nameBuf,
    UINT32 nameBufLen);


/** retrieve number of queue entries that has egress interface matching the name of the queueObj's interface
 *
 *  @queueObj    (IN)   void pointer to queueObj (Device.QoS.Queue.{i} or IGD.QueueManagement.Queue.{i})
 *  @numQueues   (OUT)  number of queue found matching the input's egress interface name
 *
 */
void rutQos_getIntfNumOfCreatedQueues(const void *queueObj, UINT32 *numQueues);
void rutQos_getIntfNumOfCreatedQueues_igd(const void *queueObj, UINT32 *numQueues);
void rutQos_getIntfNumOfCreatedQueues_dev2(const void *queueObj, UINT32 *numQueues);

#if defined(SUPPORT_DM_LEGACY98)
#define rutQos_getIntfNumOfCreatedQueues(q, n)  rutQos_getIntfNumOfCreatedQueues_igd((q), (n))
#elif defined(SUPPORT_DM_HYBRID)
#define rutQos_getIntfNumOfCreatedQueues(q, n)  rutQos_getIntfNumOfCreatedQueues_igd((q), (n))
#elif defined(SUPPORT_DM_PURE181)
#define rutQos_getIntfNumOfCreatedQueues(q, n)  rutQos_getIntfNumOfCreatedQueues_dev2((q), (n))
#elif defined(SUPPORT_DM_DETECT)
#define rutQos_getIntfNumOfCreatedQueues(q, n)  (cmsMdm_isDataModelDevice2() ? \
                                                 rutQos_getIntfNumOfCreatedQueues_dev2((q), (n)) : \
                                                 rutQos_getIntfNumOfCreatedQueues_igd((q), (n)))
#endif

#endif // __RUT_QOS_H__

