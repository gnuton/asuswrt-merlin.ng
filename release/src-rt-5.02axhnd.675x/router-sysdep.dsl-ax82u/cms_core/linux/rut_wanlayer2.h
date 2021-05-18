/***********************************************************************
 *
 *  Copyright (c) 2009 Broadcom Corporation
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
#ifndef __RUT_WANLAYER2_H__
#define __RUT_WANLAYER2_H__


/** This function will use WanIPConnObject/WanPPPconnObject iidStack to get the underline layer 2 link 
 * config object.  It is a base function and is used by other helper functions rutWl2_getL2Info and 
 * rutWl2_getXtmInfo below in this source file.  
 *
 * NOTE --- The L2LinkCfgObj needs to be freed by called
 * 
 * @param wanConnOid (IN) oid is either MDMOID_WAN_PPP_CONN or MDMOID_WAN_IP_CONN
 * @param iidStack   (IN) iidStack of the WanIP/PPPConnObject.  This iidStack is
 *                        used to find the ancestor WanLinkCfg object 
 *
 * @param L2LinkCfgObj (OUT) layer 2 link object
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getL2LinkObj(MdmObjectId wanConnOid, 
                           const InstanceIdStack *iidStack,
                           void ** L2LinkCfgObj);


/** This function will use WanIPConnObject/WanPPPconnObject iidStack to get the underline layer 2 link 
 * config object for for the link status, layer 2inteface name and connMode.  It is a base function
 * and is used by other helper functions in this source file.
 * 
 * @param wanConnOid (IN) oid is either MDMOID_WAN_PPP_CONN or MDMOID_WAN_IP_CONN
 * @param iidStack   (IN) iidStack of the WanIP/PPPConnObject.  This iidStack is
 *                        used to find the ancestor WanLinkCfg object 
 *
 * @param isLayer2LinkUp (OUT) layer 2 link status
 * @param l2IfName       (OUT) layer 2 interface name
 * @param connMode       (OUT) Connection Mode
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getL2Info(MdmObjectId wanConnOid,  
                        const InstanceIdStack *iidStack,
                        UBOOL8 *isLayer2LinkUp,
                        char *l2IfName,
                        ConnectionModeType *connMode);


#ifdef DMP_ADSLWAN_1

#include "rut_xtmlinkcfg.h"
#include "devctl_xtm.h"
#include "bcmxtmcfg.h"

/** This function will use WanIPConnObject/WanPPPconnObject iidStack to get the dsl parameters from
 * layer 2 dsl/ptmCfg object for vpi/vci, portId/Priority, encap info and isIPoA/isPPPoA.  It is a base function
 * and is used by other helper functions in this source file.
 * 
 * @param wanConnOid (IN) oid is either MDMOID_WAN_PPP_CONN or MDMOID_WAN_IP_CONN
 * @param iidStack   (IN) iidStack of the WanIP/PPPConnObject.  This iidStack is
 *                        used to find the ancestor WanLinkCfg object 
 *
 * @param isPPPoAorIPoA (OUT) If it is MDMOID_WAN_IP_CONN, return isIpoA else isPPPoA boolean
 * @param isVCMux       (OUT) boolean on isVCMux (ATMEncapsulation)
 * @param xtmAddr       (OUT) info on vpi/vci for ATM portId/priority for PTM
 *
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getXtmInfo(MdmObjectId wanConnOid, 
                        const InstanceIdStack *iidStack,
                        UBOOL8 *isPPPoAorIPoA,
                        UBOOL8 *isVCMux,
                        XTM_ADDR *xtmAddr);
#endif

                           
/** This function finds out if the WanIP/PPPConnObject's underline layer 2 link is DSL or not
 *
 * @param wanConnOid (IN) oid is either MDMOID_WAN_PPP_CONN or MDMOID_WAN_IP_CONN
 * @param iidStack   (IN) iidStack of the WanIP/PPPConnObject.  This iidStack is
 *                        used to find the ancestor WanDslIntfCfg object
 *
 * @return UBOOL8
 */
UBOOL8 rutWl2_isWanLayer2DSL(MdmObjectId wanConnOid, const InstanceIdStack *iidStack);


/** This function finds out if the WanPppConnObject is either  PPPoA or PPPoE and is used by
 * rcl_WanPPPConnection
 *
 * @param iidStack (IN) iidStack of the WanPppConnObject.  This iidStack is
 *                  used to find the ancestor WanDslLinkCfg object what the linkType is
 *
 * @return UBOOL8 FALSE if it is not ATM or the link or linkeType is EOA (PPPoE)
 */
UBOOL8 rutWl2_isPPPoA(const InstanceIdStack *iidStack);


/** This function finds out if the WanPppConnObject is ppp over l2tp and is used by
 * rut_WanPPPConnection
 *
 * @param iidStack (IN) iidStack of the WanPppConnObject.  This iidStack is
 *                  used to find the ancestor object
 *
 * @return UBOOL8 FALSE if it is not L2tp
 */
UBOOL8 rutWl2_isPPPoL2tp(const InstanceIdStack *iidStack);


/** This function finds out if the WanIPConnObject is a IPoA and is called by rcl_wanIPConnectiObject
 *
 *
 * @param iidStack (IN) iidStack of the WanIPConnObject.  This iidStack is
 *                  used to find the ancestor WanDslLinkCfg object what the linkType is
 *
 * @return UBOOL8 FALSE if it is not ATM or the link or linkeType is EOA (IPoE and Bridge)
 */
UBOOL8 rutWl2_isIPoA(const InstanceIdStack *iidStack);


/** This function finds out if the connection mode is vlanMux for the WanIP/PPPConnObject
 *
 * @param wanConnOid (IN) oid is either MDMOID_WAN_PPP_CONN or MDMOID_WAN_IP_CONN
 * @param iidStack   (IN) iidStack of the WanIP/PPPConnObject.  This iidStack is
 *                        used to find the ancestor layer 2 object *
 * @return UBOOL8
 */
UBOOL8 rutWl2_isVlanMuxEnabled(MdmObjectId wanConnOid, 
                               const InstanceIdStack *iidStack);


/** This function finds out if the wan layer 2 link is up or for the WanIP/PPPConnObject.
 *
 * @param wanConnOid (IN) oid is either MDMOID_WAN_PPP_CONN or MDMOID_WAN_IP_CONN
 * @param iidStack   (IN) iidStack of the WanIP/PPPConnObject.  This iidStack is
 *                        used to find the ancestor WanLinkCfg object *
 * @return UBOOL8
 */
UBOOL8 rutWl2_isWanLayer2LinkUp(MdmObjectId wanConnOid, 
                                const InstanceIdStack *iidStack);

/** This function get the connMode for the WanIP/PPPConnObject
 *                          
 * @param wanConnOid (IN) oid is either MDMOID_WAN_PPP_CONN or MDMOID_WAN_IP_CONN
 * @param iidStack   (IN) iidStack of the WanIP/PPPConnObject.  This iidStack is
 *                        used to find the ancestor WanLinkCfg object
 *
 * @return ConnectionModeType connMode.
 */
ConnectionModeType rutWl2_getConnMode(MdmObjectId wanConnOid, 
                          const InstanceIdStack *iidStack);

/** This function get the layer 2 interface name for the WanIP/PPPConnObject
 *                          
 * @param wanConnOid (IN) oid is either MDMOID_WAN_PPP_CONN or MDMOID_WAN_IP_CONN
 * @param iidStack   (IN) iidStack of the WanIP/PPPConnObject.  This iidStack is
 *                        used to find the ancestor WanLinkCfg object
 * @param l2IfName   (OUT) layer 2 interface name for WanIP/PPPConnObject
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getL2IfName(MdmObjectId wanConnOid,
                          const InstanceIdStack *iidStack,
                          char *l2IfName);


/** Given a layer 3 ifname, get the underlying layer 2 ifname.
 *
 * @param l3Ifname (IN) The layer 3 ifname.
 * @param l2IfName (OUT) the layer 2 ifname, this buffer must be at least
 *                       CMS_IFNAME_LENGTH bytes long.
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getL2IfnameFromL3Ifname(const char *l3Ifname, char *l2Ifname);


/** Get the DSL InterfaceConfig object corresponding to the ATM line.
 *
 * @param iidStack (OUT) The iidStack of the requested object.
 * @param wanDslIntfObj (OUT) The requested object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getAtmDslIntfObject(InstanceIdStack *iidStack,
                                  WanDslIntfCfgObject **wanDslIntfObj);


/** Get the DSL InterfaceConfig object corresponding to the PTM line.
 *
 * @param iidStack (OUT) The iidStack of the requested object.
 * @param wanDslIntfObj (OUT) The requested object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getPtmDslIntfObject(InstanceIdStack *iidStack,
                                  WanDslIntfCfgObject **wanDslIntfObj);


/** Get the DSL InterfaceConfig object corresponding to peer name.
 *
 * @param iidStack (OUT) The iidStack of the requested object.
 * @param wanDslIntfObj (OUT) The requested object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getBondingDslIntfObjectByPeerName(const char *peerName,
                                       InstanceIdStack     *iidStack,
                                       WanDslIntfCfgObject **wanDslIntfObj);

/** Get the DSL InterfaceConfig object corresponding to the Bonding ATM line.
 *
 * @param iidStack (OUT) The iidStack of the requested object.
 * @param wanDslIntfObj (OUT) The requested object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getBondingAtmDslIntfObject(InstanceIdStack *iidStack,
                                      WanDslIntfCfgObject **wanDslIntfObj);

/** Get the DSL InterfaceConfig object corresponding to the Bonding PTM line.
 *
 * @param iidStack (OUT) The iidStack of the requested object.
 * @param wanDslIntfObj (OUT) The requested object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getBondingPtmDslIntfObject(InstanceIdStack *iidStack,
                                      WanDslIntfCfgObject **wanDslIntfObj);

/** Get the DSL InterfaceConfig object corresponding to the Bonding ATM line.
 *
 * @param iidStack (OUT) The iidStack of the requested object.
 * @param wanDslIntfObj (OUT) The requested object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getBondingAtmDslIntfObject(InstanceIdStack *iidStack,
                                      WanDslIntfCfgObject **wanDslIntfObj);


/** Get the (layer 2) Wan Ethernet InterfaceConfig object.
 *
 * @param iidStack (OUT) The iidStack of the requested object.
 * @param wanEthIntfObj (OUT) The requested object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getWanEthObject(InstanceIdStack *iidStack,
                              WanEthIntfObject **wanEthIntfObj);


/** Get the (layer 2) Wan Moca InterfaceConfig object.
 *
 * @param iidStack (OUT) The iidStack of the requested object.
 * @param wanMocaIntfObj (OUT) The requested object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getWanMocaObject(InstanceIdStack *iidStack,
                               WanMocaIntfObject **wanMocaIntfObj);


/** Get the (layer 2) Wan Gpon iidStack.
 *
 * @param iidStack (OUT) The iidStack of the requested object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getGponWanIidStack(InstanceIdStack *gponWanIid);



/** Get the (layer 2) Wan Epon iidStack.
 *
 * @param iidStack (OUT) The iidStack of the requested object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getEponWanIidStack(InstanceIdStack *eponWanIid);


/** Get the (layer 2) Wan L2tpAc iidStack .
 *
 * @param iidStack (OUT) The iidStack of the requested object.
 *
 * @return CmsRet enum.
 */
CmsRet rutWl2_getL2tpWanIidStack(InstanceIdStack *l2tpWanIid);
#endif /* __RUT_WANLAYER2_H__ */
                                   

