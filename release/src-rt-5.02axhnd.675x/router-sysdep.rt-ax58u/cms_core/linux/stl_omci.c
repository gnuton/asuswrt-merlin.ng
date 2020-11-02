/***********************************************************************
 *
 *  Copyright (c) 2017 Broadcom
 *  All Rights Reserved
 *
 * <:label-BRCM:2017:proprietary:standard
 *
 *  This program is the proprietary software of Broadcom and/or its
 *  licensors, and may only be used, duplicated, modified or distributed pursuant
 *  to the terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied), right
 *  to use, or waiver of any kind with respect to the Software, and Broadcom
 *  expressly reserves all rights in and to the Software and all intellectual
 *  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 *  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 *  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1. This program, including its structure, sequence and organization,
 *     constitutes the valuable trade secrets of Broadcom, and you shall use
 *     all reasonable efforts to protect the confidentiality thereof, and to
 *     use this information only in connection with your use of Broadcom
 *     integrated circuit products.
 *
 *  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 *     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 *     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
 *     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
 *     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
 *     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
 *     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
 *     PERFORMANCE OF THE SOFTWARE.
 *
 *  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
 *     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 *     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
 *     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
 *     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
 *     LIMITED REMEDY.
 * :>
 *
************************************************************************/

#ifdef DMP_X_ITU_ORG_GPON_1

#include "stl.h"
#include "rut_wan.h"


CmsRet stl_bcmOmciObject(
  _BcmOmciObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciRtdObject(
  _BcmOmciRtdObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciRtdMcastObject(
  _BcmOmciRtdMcastObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciRtdLayer3Object(
  _BcmOmciRtdLayer3Object *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciRtdIpHostConfigDataObject(
  _BcmOmciRtdIpHostConfigDataObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciRtdIpHostConfigDataExtObject(
  _BcmOmciRtdIpHostConfigDataExtObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciRtdIpv6HostConfigDataObject(
  _BcmOmciRtdIpv6HostConfigDataObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciRtdIpv6CurrentAddressTableObject(
  _BcmOmciRtdIpv6CurrentAddressTableObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciRtdIpv6CurrentDefaultRouterTableObject(
  _BcmOmciRtdIpv6CurrentDefaultRouterTableObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciRtdIpv6CurrentDnsTableObject(
  _BcmOmciRtdIpv6CurrentDnsTableObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciRtdIpv6CurrentOnlinkPrefixTableObject(
  _BcmOmciRtdIpv6CurrentOnlinkPrefixTableObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciRtdIpv6HostConfigDataExtObject(
  _BcmOmciRtdIpv6HostConfigDataExtObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciRtdTcpUdpConfigDataObject(
  _BcmOmciRtdTcpUdpConfigDataObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciDebugObject(_BcmOmciDebugObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciConfigObject(_BcmOmciConfigObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciConfigSystemObject(
  _BcmOmciConfigSystemObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciStatsObject(
  _BcmOmciStatsObject *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_bcmOmciStatsGponOmciStatsObject(_BcmOmciStatsGponOmciStatsObject *obj,
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;
    char opticalWanType[CMS_IFNAME_LENGTH];
    CmsMsgHeader *msg;
    CmsMsgHeader *rxMsg;
    _BcmOmciStatsGponOmciStatsObject *msgStats;

    if (rutWan_getOpticalWanType(&opticalWanType[0]) != CMSRET_SUCCESS)
    {
        cmsLog_error("rutWan_getOpticalWanType() failed");
        return ret;
    }

    if (cmsUtl_strcmp(opticalWanType, MDMVS_GPON) != 0)
    {
        cmsLog_error("Unexpected wan type %s", opticalWanType);
        return ret;
    }

    msg = cmsMem_alloc(sizeof(CmsMsgHeader), ALLOC_ZEROIZE);
    if (msg == NULL)
    {
        cmsLog_error("Memory allocation failed");
        return CMSRET_INTERNAL_ERROR;
    }

    msg->type = CMS_MSG_GET_GPON_OMCI_STATS;
    msg->src = mdmLibCtx.eid;
    msg->dst = EID_OMCID;
    msg->flags_request = 1;

    if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, msg)) != CMSRET_SUCCESS)
    {
        CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
        cmsLog_error("Failed to send message (ret=%d)", ret);
        return ret;
    }

    CMSMEM_FREE_BUF_AND_NULL_PTR(msg);

    if ((ret = cmsMsg_receiveWithTimeout(mdmLibCtx.msgHandle, &rxMsg, 500))
      != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to receive message (ret=%d)", ret);
        return ret;
    }

    if (rxMsg->type != CMS_MSG_GET_GPON_OMCI_STATS)
    {
        cmsLog_error("Invalid message type (%d)", (unsigned int)rxMsg->type);
        CMSMEM_FREE_BUF_AND_NULL_PTR(rxMsg);
        return CMSRET_INTERNAL_ERROR;
    }

    if (rxMsg->dataLength != sizeof(_BcmOmciStatsGponOmciStatsObject))
    {
        cmsLog_error("Invalid Data Length: %d != %d",
          rxMsg->dataLength, sizeof(_BcmOmciStatsGponOmciStatsObject));
        CMSMEM_FREE_BUF_AND_NULL_PTR(rxMsg);
        return CMSRET_INTERNAL_ERROR;
    }

    msgStats = (_BcmOmciStatsGponOmciStatsObject *)(rxMsg + 1);

    /* preserve object header, by copying only parameters */
    obj->rxGoodPackets = msgStats->rxGoodPackets;
    obj->rxLengthErrors = msgStats->rxLengthErrors;
    obj->rxCrcErrors = msgStats->rxCrcErrors;
    obj->rxOtherErrors = msgStats->rxOtherErrors;
    obj->txAvcPackets = msgStats->txAvcPackets;
    obj->txResponsePackets = msgStats->txResponsePackets;
    obj->txRetransmissions = msgStats->txRetransmissions;
    obj->txErrors = msgStats->txErrors;
    obj->rxBaseLinePackets = msgStats->rxBaseLinePackets;
    obj->rxExtendedPackets = msgStats->rxExtendedPackets;
    obj->txAlarmPackets = msgStats->txAlarmPackets;

    /* if we reply with CMSRET_SUCCESS, the MDM will try to set the object,
       but it will fail because the object is read only */
    ret = CMSRET_SUCCESS_OBJECT_UNCHANGED;

    CMSMEM_FREE_BUF_AND_NULL_PTR(rxMsg);

    return ret;
}

#if 0
CmsRet stl_(_ *obj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)))
{
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif

#endif /* DMP_X_ITU_ORG_GPON_1 */
