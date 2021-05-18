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

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "mdm.h"
#include "rut_util.h"
#include "rut_pmap.h"
#include "rut_lan.h"
#include "rut_wlan.h"
#ifdef WIRELESS
#include <wlcsm_lib_api.h>
#endif

#define  MDM_STRCPY(x, y)    CMSMEM_REPLACE_STRING_FLAGS( (x), (y), mdmLibCtx.allocFlags )

void rutWlan_requestRestart(char *cmd, int index) {

#ifdef SUPPORT_UNIFIED_WLMNGR
    if(mdmLibCtx.eid == EID_TR69C || mdmLibCtx.eid == EID_CWMPD)
        wlcsm_mngr_restart(index,WLCSM_MNGR_RESTART_TR69C,WLCSM_MNGR_RESTART_NOSAVEDM,0);
    else if(mdmLibCtx.eid != EID_WLMNGR)
        wlcsm_mngr_restart(index,WLCSM_MNGR_RESTART_MDM,WLCSM_MNGR_RESTART_NOSAVEDM,0);
#else

#if defined(SUPPORT_DM_DETECT)
    if(cmsMdm_isDataModelDevice2())
    {
#endif
#if defined(SUPPORT_TR181_WLMNGR)
        wlcsm_mngr_restart(index,WLCSM_MNGR_RESTART_MDM,WLCSM_MNGR_RESTART_NOSAVEDM,0);
#endif
#if defined(SUPPORT_DM_DETECT)
    } else
#endif
#if !defined(SUPPORT_TR181_WLMNGR) || defined(SUPPORT_DM_DETECT)
    {
        char buf[sizeof(CmsMsgHeader) + 100]= {0};
        CmsMsgHeader *msg=(CmsMsgHeader *) buf;
        sprintf((char *)(msg+1), "%s:%d", cmd,index+1);
        msg->type = CMS_MSG_WLAN_CHANGED;
        msg->src = mdmLibCtx.eid;
        msg->dst = EID_WLMNGR;
        msg->flags_bounceIfNotRunning = 1;
        msg->flags_event = 1;
        msg->flags_request = 0;
        msg->dataLength=strlen((char *)(msg+1));
        if (cmsMsg_send(mdmLibCtx.msgHandle, msg) != CMSRET_SUCCESS)
        {
            cmsLog_error("could not send CMS_MSG_WLAN_CHANGED msg to wlan");
        }
    }
#endif
#endif
}

void rutWlan_modifyVirtIntfFilters(const InstanceIdStack *iidStack __attribute__((unused)),
                                   UBOOL8 enable __attribute__((unused)))
{
#ifdef DMP_BRIDGING_1

    WlVirtIntfCfgObject *virtIntfObj=NULL;
    InstanceIdStack virtIidStack=EMPTY_INSTANCE_ID_STACK;
    CmsRet ret;

    cmsLog_debug("master wlEnbl=%d", enable);

    while ((ret = cmsObj_getNextInSubTreeFlags(MDMOID_WL_VIRT_INTF_CFG, iidStack, &virtIidStack, OGF_NO_VALUE_UPDATE, (void **) &virtIntfObj)) == CMSRET_SUCCESS)
    {
        if (enable && virtIntfObj->wlEnblSsid)
        {
            cmsLog_debug("enabling virtIntf %s", virtIntfObj->wlIfcname);
            rutWlan_enableVirtIntfFilter(virtIntfObj->wlIfcname, atoi(&(virtIntfObj->wlBrName[2])));
        }
        else if (!enable)
        {
            cmsLog_debug("disabling virtIntf %s", virtIntfObj->wlIfcname);
            rutWlan_disableVirtIntfFilter(virtIntfObj->wlIfcname);

            if (strcmp(virtIntfObj->wlBrName, "br0"))
            {
                /*
                 * wlBrName is an internal book-keeping parameter used by wlmngr.
                 * set it to br0 to be consistent with the filterBridgeRefence setting
                 * of -1.  This is also done at the bottom of rcl_wlVirtIntfCfgObject.
                 */
                CMSMEM_REPLACE_STRING_FLAGS(virtIntfObj->wlBrName, "br0", mdmLibCtx.allocFlags);
                cmsObj_set(virtIntfObj, &virtIidStack);
            }
        }

        cmsObj_free((void **) &virtIntfObj);
    }

#endif
}

#ifdef DMP_BRIDGING_1
void rutWlan_enableVirtIntfFilter(const char *wlIfcname, UINT32 bridgeKey)
{
    L2BridgingFilterObject *filterObj=NULL;
    InstanceIdStack filterIidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret;

    cmsLog_debug("wlIfcname=%s bridgeKey=%d", wlIfcname, bridgeKey);

    if ((ret = rutPMap_ifNameToFilterObject(wlIfcname, &filterIidStack, &filterObj)) != CMSRET_SUCCESS)
    {
        cmsLog_debug("could not get filterObj for %s", wlIfcname);
    }
    else
    {
        /*
         * The bridgeKey is just the number in the brx linux interface name.
         * So br0 has bridgeKey of 0,
         * br1 has bridgeKey of 1, etc.
         */
        cmsLog_debug("got filter at %p iidStack=%s", filterObj, cmsMdm_dumpIidStack(&filterIidStack));

        if (filterObj->filterBridgeReference != (int) bridgeKey)
        {
            filterObj->filterBridgeReference = bridgeKey;

            if ((ret = cmsObj_set(filterObj, &filterIidStack)) != CMSRET_SUCCESS)
            {
                cmsLog_error("could not set FilterBridgeReference to %d, ret=%d", filterObj->filterBridgeReference, ret);
            }

            cmsObj_free((void **) &filterObj);
        }
    }

    return;
}


void rutWlan_disableVirtIntfFilter(const char *wlIfcname)
{
    L2BridgingFilterObject *filterObj=NULL;
    InstanceIdStack filterIidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret;

    cmsLog_debug("wlIfcname=%s", wlIfcname);

    /*
     * by setting the FilterBridgeRef to -1, the rut_pmap will disassociate
     * this interface from its bridge.
     */
    if ((ret = rutPMap_ifNameToFilterObject(wlIfcname, &filterIidStack, &filterObj)) != CMSRET_SUCCESS)
    {
        cmsLog_debug("could not get filterObj for %s", wlIfcname);
    }
    else
    {
        cmsLog_debug("filterBridgeRef=%d", filterObj->filterBridgeReference);

        if (filterObj->filterBridgeReference != -1)
        {
            filterObj->filterBridgeReference = -1;
            if ((ret = cmsObj_set(filterObj, &filterIidStack)) != CMSRET_SUCCESS)
            {
                cmsLog_error("could not set FilterBridgeReference to -1, ret=%d", ret);
            }
        }

        cmsObj_free((void **) &filterObj);
    }

    return;
}
#endif /* DMP_BRIDGING_1 */

#ifdef WIRELESS
#ifdef DMP_BASELINE_1
CmsRet rutWlan_get_sta_Host(const _LanWlanAssociatedDeviceEntryObject *sta, InstanceIdStack *host_iidStack, LanHostEntryObject **host) {

    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack lan_iidStack = EMPTY_INSTANCE_ID_STACK;
    _WlVirtIntfCfgObject *wlVirtIntfCfgObj=NULL;
    CmsRet ret;
    LanDevObject *lanObj=NULL;

    rutLan_getWlanInterface(sta->X_BROADCOM_COM_Ifcname,&iidStack, &wlVirtIntfCfgObj);

    if(wlVirtIntfCfgObj) {

        cmsLog_debug("virtualinterface bridge name:%s \n",wlVirtIntfCfgObj->wlBrName);
        INIT_INSTANCE_ID_STACK(&iidStack);

        ret = rutLan_getLanDevByBridgeIfName(wlVirtIntfCfgObj->wlBrName, &lan_iidStack, &lanObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("could not find bridge ifname %s", wlVirtIntfCfgObj->wlBrName);
            cmsObj_free((void **)&wlVirtIntfCfgObj);
            return ret;
        }
        /* we don't need the lanObj, just its iidStack */
        cmsObj_free((void **) &lanObj);
        cmsObj_free((void **)&wlVirtIntfCfgObj);
    } else
        cmsLog_debug("could not found virutal interface \n");

    if((ret=rutLan_getHostEntryByMacAddr(&lan_iidStack,sta->associatedDeviceMACAddress,host_iidStack,host))!=CMSRET_SUCCESS) {
        cmsLog_debug("COULD not found host of sta:%s\n",sta->associatedDeviceMACAddress);
    }
    return ret;
}

static CmsRet _find_sta_in_mdm(char *macAddr,int radio_idx,void **assocDev,InstanceIdStack *iidStack)
{

    MdmPathDescriptor pathDesc;
    CmsRet ret;
    _LanWlanAssociatedDeviceEntryObject *assocDevObj=NULL;
    INIT_INSTANCE_ID_STACK(iidStack);
    INIT_PATH_DESCRIPTOR(&pathDesc);

    pathDesc.oid = MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY;
    PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
    PUSH_INSTANCE_ID(&(pathDesc.iidStack), radio_idx+1);

    while ( (ret = cmsObj_getNextInSubTree(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY,
                                           &(pathDesc.iidStack), iidStack, (void **)assocDev)) == CMSRET_SUCCESS )  {

        assocDevObj=(_LanWlanAssociatedDeviceEntryObject *)(*assocDev);
        if (!cmsUtl_strcasecmp(assocDevObj->associatedDeviceMACAddress, macAddr)) {
            return CMSRET_SUCCESS;
        }
        cmsObj_free((void **)assocDev);

    }
    *assocDev=NULL;
    return CMSRET_OBJECT_NOT_FOUND;
}


static inline CmsRet _find_sta_in_summary(WL_STALIST_SUMMARIES *sta_summaries, char *macAddr) {
    WL_STATION_LIST_ENTRY *pStation = sta_summaries->stalist_summary;
    int i=0;
    for (; i < sta_summaries->num_of_stas && pStation != NULL ; i++, pStation++) {
        if (!cmsUtl_strcasecmp(pStation->macAddress, macAddr))  return CMSRET_SUCCESS;
    }
    return CMSRET_OBJECT_NOT_FOUND;
}


static inline CmsRet _trim_mdm_sta(WL_STALIST_SUMMARIES *sta_summaries) {
    MdmPathDescriptor pathDesc;
    _LanWlanAssociatedDeviceEntryObject *assocDevObj=NULL;
    CmsRet ret;
    InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY;
    PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
    PUSH_INSTANCE_ID(&(pathDesc.iidStack), sta_summaries->radioIdx+1);
    cmsLog_debug("trim MDM and found sta:%d\n",sta_summaries->num_of_stas);
    while ( (ret = cmsObj_getNextInSubTree(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY,
                                           &(pathDesc.iidStack),&iidStack, (void **)&assocDevObj)) == CMSRET_SUCCESS )  {
        cmsLog_debug("found one sta:%s\n",assocDevObj->associatedDeviceMACAddress);
        if(!sta_summaries->num_of_stas || (sta_summaries->num_of_stas &&
                                           (ret= _find_sta_in_summary(sta_summaries,assocDevObj->associatedDeviceMACAddress))==CMSRET_OBJECT_NOT_FOUND)) {
            cmsLog_debug("delete it!!!");
            cmsObj_deleteInstance(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY,&iidStack);
            INIT_INSTANCE_ID_STACK(&iidStack);
        } else
            cmsLog_debug(" keep it!!!!");
        cmsObj_free((void **)&assocDevObj);
    }
    return ret;
}

static CmsRet inline _remove_stas_from_mdm(WL_STALIST_SUMMARIES *sta_summaries) {
    WL_STATION_LIST_ENTRY *pStation = sta_summaries->stalist_summary;
    _LanWlanAssociatedDeviceEntryObject *assocDevObj=NULL;
    InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
    CmsRet ret=0;
    int i=0;


    cmsLog_debug("number of stations:%d\n",sta_summaries->num_of_stas);
    for (; i < sta_summaries->num_of_stas && pStation != NULL ; i++, pStation++) {

        INIT_INSTANCE_ID_STACK(&iidStack);

        cmsLog_debug("try to remove sta:%s\n",pStation->macAddress);
        if(_find_sta_in_mdm(pStation->macAddress,sta_summaries->radioIdx,(void **)&assocDevObj,&iidStack)==CMSRET_SUCCESS) {
            /* remove sta */
            cmsLog_debug(" remove sta:%s\n",pStation->macAddress);
            cmsObj_free((void **)&assocDevObj);
            if((ret=cmsObj_deleteInstance(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY,&iidStack))!=CMSRET_SUCCESS) {
                fprintf(stderr, ":%s:%d:could not delnew STA    \n",__FUNCTION__,__LINE__);
                return ret;
            }

        }
    }
    return ret;
}

static CmsRet inline _update_stas_to_mdm(WL_STALIST_SUMMARIES *sta_summaries) {
    WL_STATION_LIST_ENTRY *pStation = sta_summaries->stalist_summary;
    _LanWlanAssociatedDeviceEntryObject *assocDevObj=NULL;
    InstanceIdStack iidStack;
    CmsRet ret=0;
    int i=0;


    cmsLog_debug("number of stations:%d\n",sta_summaries->num_of_stas);
    for (; i < sta_summaries->num_of_stas && pStation != NULL ; i++, pStation++) {

        INIT_INSTANCE_ID_STACK(&iidStack);
        if(_find_sta_in_mdm(pStation->macAddress,sta_summaries->radioIdx,(void **)&assocDevObj,&iidStack)==CMSRET_OBJECT_NOT_FOUND) {
            /* add sta */
            cmsLog_debug(" Add sta:%s\n",pStation->macAddress);
            INIT_INSTANCE_ID_STACK(&iidStack);
            PUSH_INSTANCE_ID(&iidStack,1);
            PUSH_INSTANCE_ID(&iidStack,sta_summaries->radioIdx+1);
            if((ret=cmsObj_addInstance(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY,&iidStack))!=CMSRET_SUCCESS) {
                fprintf(stderr, ":%s:%d:could not add new STA    \n",__FUNCTION__,__LINE__);
                return ret;
            } else {
                if ((ret = cmsObj_get(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY, &iidStack, 0, (void **) &assocDevObj)) != CMSRET_SUCCESS) {
                    printf("%s@%d Get Obj Error=%d\n", __FUNCTION__, __LINE__, ret );
                    return ret;
                }
                MDM_STRCPY(assocDevObj->associatedDeviceMACAddress, pStation->macAddress );
            }
        } else
            cmsLog_debug("update sta:%s\n",pStation->macAddress);

        /*by now assocDevObj should point ot an object */
        MDM_STRCPY(assocDevObj->associatedDeviceIPAddress, "" );
        assocDevObj->associatedDeviceAuthenticationState=TRUE;
        MDM_STRCPY(assocDevObj->lastRequestedUnicastCipher, "" );
        MDM_STRCPY(assocDevObj->lastRequestedMulticastCipher, "" );
        MDM_STRCPY(assocDevObj->lastPMKId, "" );
        MDM_STRCPY(assocDevObj->lastDataTransmitRate, "" );
        assocDevObj->X_BROADCOM_COM_Associated = pStation->associated;
        assocDevObj->X_BROADCOM_COM_Authorized = pStation->authorized;
        MDM_STRCPY(assocDevObj->X_BROADCOM_COM_Ssid,pStation->ssid);
        MDM_STRCPY(assocDevObj->X_BROADCOM_COM_Ifcname, pStation->ifcName);

        ret = cmsObj_set(assocDevObj, &iidStack);
        cmsObj_free((void **)&assocDevObj);

        if (ret != CMSRET_SUCCESS) {
            return ret;
        }
    }
    return ret;
}

void rutWlan_clear_ap_stas(const _WlVirtIntfCfgObject *virtualInf,const InstanceIdStack *iidStack) {

    CmsRet ret;
    /*iidStack is wlVirutalInterfCFG's stack,the node is deeper than associate list, so just
     *pop up to depth 2 */
    InstanceIdStack lan_iidStack = *iidStack;
    InstanceIdStack sta_iidStack = EMPTY_INSTANCE_ID_STACK;
    _LanWlanAssociatedDeviceEntryObject *assocDevObj=NULL;
    lan_iidStack.currentDepth=2;
    while ( (ret = cmsObj_getNextInSubTree(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY,
                                           &(lan_iidStack), &sta_iidStack, (void **)&assocDevObj)) == CMSRET_SUCCESS )  {
        if (!cmsUtl_strcasecmp(assocDevObj->X_BROADCOM_COM_Ifcname, virtualInf->wlIfcname)) {
            cmsLog_debug("try to delete sta:%s\n ",assocDevObj->associatedDeviceMACAddress);
            if((ret=cmsObj_deleteInstance(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY,&sta_iidStack))!=CMSRET_SUCCESS) {
                cmsLog_error("could nto delete sta:%s\n",assocDevObj->associatedDeviceMACAddress);
            }
            INIT_INSTANCE_ID_STACK(&sta_iidStack);
        }
        cmsObj_free((void **)&assocDevObj);
    }
}

CmsRet rutWifi_modify_sta_count(const InstanceIdStack *sta_iidStack,int delta) {

    CmsRet ret;
    _LanWlanObject *wllanWlanObj=NULL;
    InstanceIdStack iidStack;
    memcpy(&iidStack,sta_iidStack,sizeof(InstanceIdStack));

    if ((ret = cmsObj_getAncestor(MDMOID_LAN_WLAN,MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY,&iidStack,(void **)&wllanWlanObj))
            != CMSRET_SUCCESS) {

        cmsLog_debug("could not found lan objec of this STA");
        return ret;
    }

    wllanWlanObj->totalAssociations += delta;
    ret = cmsObj_set(wllanWlanObj, &(iidStack));
    cmsObj_free((void **)&wllanWlanObj);
    return ret;
}

#endif /* DMP_BASELINE_1 */

CmsRet rutWifi_AssociatedDeviceUpdate(void *stas) {
    CmsRet ret=CMSRET_SUCCESS;
#ifdef DMP_BASELINE_1
    WL_STALIST_SUMMARIES  *sta_summaries=(WL_STALIST_SUMMARIES *)stas;
    switch(sta_summaries->type) {
    case UPDATE_STA_ALL:
        _trim_mdm_sta(sta_summaries);
    /*contineu to add STAs if it is not in MDM */
    case STA_CONNECTED:
        if(ret!=CMSRET_SUCCESS)
            cmsLog_error("could not trim mdm for stas?\n");
        ret=_update_stas_to_mdm(sta_summaries);
        break;
    case STA_DISCONNECTED:
        ret=_remove_stas_from_mdm(sta_summaries);
        break;
    default:
        fprintf(stderr, ":%s:%d:unknonw type:%d    \n",__FUNCTION__,__LINE__,sta_summaries->type);
        ret=CMSRET_INVALID_ARGUMENTS;
        break;
    }
#endif
    return ret;

}
#endif /* WIRELESS */
