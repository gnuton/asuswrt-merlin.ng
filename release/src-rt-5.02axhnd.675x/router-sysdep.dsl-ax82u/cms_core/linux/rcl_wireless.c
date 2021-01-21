/*
* <:copyright-BRCM:2011:proprietary:standard
*
*    Copyright (c) 2011 Broadcom
*    All Rights Reserved
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
:>
*/

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_pmap.h"
#include "rut_lan.h"
#include "rut_wlan.h"
#include "rut_wifiwan.h"
#ifdef BRCM_WLAN
#include "wlcsm_lib_api.h"
#endif


#ifdef DMP_X_BROADCOM_COM_WIFILAN_1

static CmsRet wlMsgRestartWlan(char *cmd,int index)
{
#ifdef BRCM_WLAN
    CmsRet ret = CMSRET_SUCCESS;
#ifdef SUPPORT_UNIFIED_WLMNGR
    if(mdmLibCtx.eid == EID_TR69C || mdmLibCtx.eid == EID_CWMPD)
        wlcsm_mngr_restart(index,WLCSM_MNGR_RESTART_TR69C,WLCSM_MNGR_RESTART_NOSAVEDM,0);
    else if(mdmLibCtx.eid != EID_WLMNGR)
        wlcsm_mngr_restart(index,WLCSM_MNGR_RESTART_MDM,WLCSM_MNGR_RESTART_NOSAVEDM,0);
    ret= CMSRET_SUCCESS;
#else
    if (( mdmLibCtx.eid == EID_WLMNGR )||( mdmLibCtx.eid == EID_SSK ) ||( mdmLibCtx.eid == EID_HTTPD )) {
    }
    else {
        char buf[sizeof(CmsMsgHeader) + 100]= {0}, idBuf[8]= {0};
        CmsMsgHeader *msg=(CmsMsgHeader *) buf;
        strcpy((char *)(msg+1), cmd);

        sprintf(idBuf, ":%d", index);
        strcat((char *)(msg+1), idBuf);

        msg->type = CMS_MSG_WLAN_CHANGED;

        msg->src = mdmLibCtx.eid;
        msg->dst = EID_WLMNGR;

        msg->flags_event = 1;
        msg->flags_request = 0;

        if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, msg)) != CMSRET_SUCCESS)
        {
            printf("could not send CMS_MSG_WLAN_CHANGED msg to wlaD, ret=%d", ret);
        }
    }
#endif
    return ret;
#else
    return CMSRET_SUCCESS;
#endif
}

#endif

#ifdef DMP_WIFILAN_1
CmsRet rcl_lanWlanObject( _LanWlanObject *newObj,
                          const _LanWlanObject *currObj,
                          const InstanceIdStack *iidStack,
                          char **errorParam __attribute__((unused)),
                          CmsRet *errorCode __attribute__((unused)))
{

#ifdef BRCM_WLAN
    CmsRet ret = CMSRET_SUCCESS;


    if (newObj == NULL)
    {
        printf("\n%s@%d Error[Delete] Should not happen!", __FUNCTION__, __LINE__);
        rut_modifyNumEthIntf(iidStack, 1);
    }

    if (newObj != NULL && currObj == NULL )
    {
        /* this is a new object instance being added */
        rut_modifyNumEthIntf(iidStack, -1);

    }

    if (newObj != NULL && currObj != NULL)
    {
        if ( mdmLibCtx.eid != EID_WLMNGR && mdmLibCtx.eid != EID_SSK) {
            wlMsgRestartWlan(NULL,iidStack->instance[iidStack->currentDepth-1]-1);
        }
#ifdef DMP_X_BROADCOM_COM_NFC_1
        {
            CmsMsgHeader *msg=(CmsMsgHeader *) buf;
            strcpy((char *)(msg+1), "Modify" );
            msg->dataLength = 0;
            msg->type = CMS_MSG_NFCD_UPDATE_WLAN_INFO;

            msg->src = mdmLibCtx.eid;
            msg->dst = EID_NFCD;

            msg->flags_event = 1;
            msg->flags_request = 0;
            msg->flags_bounceIfNotRunning = 1;

            if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, msg)) != CMSRET_SUCCESS)
            {
                printf("could not send CMS_MSG_NFCD_UPDATE_WLAN_INFO msg to NFCD, ret=%d", ret);
            }
        }
#endif /* DMP_X_BROADCOM_COM_NFC_1 */
    }
#endif /* BRCM_WLAN */
    return ret;
}



CmsRet rcl_lanWlanAssociatedDeviceEntryObject( _LanWlanAssociatedDeviceEntryObject *newObj,
        const _LanWlanAssociatedDeviceEntryObject *currObj,
        const InstanceIdStack *iidStack,
        char **errorParam,
        CmsRet *errorCode)
{
    if (ADD_NEW(newObj, currObj)) {

        cmsLog_debug("add new STA\n");
        rutWifi_modify_sta_count(iidStack,1);

    } else if (DELETE_EXISTING(newObj, currObj)) {

        InstanceIdStack host_iidstack=EMPTY_INSTANCE_ID_STACK;
        LanHostEntryObject *lanhost=NULL;
        cmsLog_debug("delete existing STA\n");
        rutWifi_modify_sta_count(iidStack,-1);
        if(rutWlan_get_sta_Host(currObj,&host_iidstack,&lanhost)==CMSRET_SUCCESS) {
            lanhost->active = FALSE;
            cmsLog_debug("set host to invalid\n");
            cmsObj_set(lanhost,&host_iidstack);
            cmsObj_free((void **)&lanhost);
        } else {
            cmsLog_debug("could not get lanhost\n");
        }
    } else {
        cmsLog_debug("update exisint STA\n");
    }
    return CMSRET_SUCCESS;
}


CmsRet rcl_lanWlanWepKeyObject( _LanWlanWepKeyObject *newObj,
                                const _LanWlanWepKeyObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam,
                                CmsRet *errorCode)
{
#ifdef BRCM_WLAN
    CmsRet ret = CMSRET_SUCCESS;
    if (newObj != NULL && currObj == NULL )
    {
        /* this is a new object instance being added */
        return CMSRET_SUCCESS;
    }

    if (newObj != NULL && currObj != NULL)
    {
        cmsLog_debug("mdmLibCtx.eid=%d EID_WLMNGR=%d\n", mdmLibCtx.eid, EID_WLMNGR );
        if ( mdmLibCtx.eid != EID_WLMNGR ) {
            wlMsgRestartWlan(NULL,iidStack->instance[iidStack->currentDepth-2]-1);
            ret= CMSRET_SUCCESS;
        }
    }
#endif

    return ret;
}

CmsRet rcl_lanWlanPreSharedKeyObject( _LanWlanPreSharedKeyObject *newObj,
                                      const _LanWlanPreSharedKeyObject *currObj,
                                      const InstanceIdStack *iidStack,
                                      char **errorParam,
                                      CmsRet *errorCode)
{
#ifdef BRCM_WLAN
    CmsRet ret = CMSRET_SUCCESS;

    if (newObj != NULL && currObj == NULL )
    {
        /* this is a new object instance being added */
        return CMSRET_SUCCESS;
    }

    if (newObj != NULL && currObj != NULL)
    {
        if ( mdmLibCtx.eid != EID_WLMNGR ) {
            wlMsgRestartWlan(NULL,iidStack->instance[iidStack->currentDepth-2]-1);
            ret= CMSRET_SUCCESS;
        }
    }
#endif
    return ret;
}


#ifdef not_supported
CmsRet rcl_lanWlanStatsObject( _LanWlanStatsObject *newObj,
                               const _LanWlanStatsObject *currObj,
                               const InstanceIdStack *iidStack,
                               char **errorParam,
                               CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}
#endif

#endif /* DMP_WIFILAN_1 */


#ifdef DMP_X_BROADCOM_COM_WIFILAN_1
CmsRet rcl_wlanAdapterObject( _WlanAdapterObject *newObj,
                              const _WlanAdapterObject *currObj,
                              const InstanceIdStack *iidStack,
                              char **errorParam,
                              CmsRet *errorCode)
{
#ifdef BRCM_WLAN
    cmsLog_debug("mdmLibCtx.eid=%d EID_WLMNGR=%d\n", mdmLibCtx.eid, EID_WLMNGR );
#endif
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlBaseCfgObject( _WlBaseCfgObject *newObj,
                            const _WlBaseCfgObject *currObj,
                            const InstanceIdStack *iidStack,
                            char **errorParam,
                            CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
#ifdef BRCM_WLAN
    UBOOL8 isCreate=FALSE;
    /* do real work here */
    if (newObj != NULL && currObj == NULL )
    {
        isCreate=TRUE;
    }

    if (newObj != NULL && currObj != NULL)
    {
        /*
         * if WlEnbl is changed, go through all the WlVirtIntf objects and make sure
         * their filter object bridge refs are set accordingly.
         */
        if (newObj->wlEnbl != currObj->wlEnbl)
        {
            cmsLog_debug("wlEnbl %d->%d", currObj->wlEnbl, newObj->wlEnbl);
            rutWlan_modifyVirtIntfFilters(iidStack, newObj->wlEnbl);
        }
    }

    if ( mdmLibCtx.eid != EID_WLMNGR ) {
        if (isCreate) {
            cmsLog_debug("suppress startup WLAN_CHANGED, msg");
        } else {
            wlMsgRestartWlan(NULL,iidStack->instance[iidStack->currentDepth-1]-1);
            ret= CMSRET_SUCCESS;
        }
    }
#endif
    return ret;
}

CmsRet rcl_wlStaticWdsCfgObject( _WlStaticWdsCfgObject *newObj,
                                 const _WlStaticWdsCfgObject *currObj,
                                 const InstanceIdStack *iidStack,
                                 char **errorParam,
                                 CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlWdsCfgObject( _WlWdsCfgObject *newObj,
                           const _WlWdsCfgObject *currObj,
                           const InstanceIdStack *iidStack,
                           char **errorParam,
                           CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlScanWdsCfgObject( _WlScanWdsCfgObject *newObj,
                               const _WlScanWdsCfgObject *currObj,
                               const InstanceIdStack *iidStack,
                               char **errorParam,
                               CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlMimoCfgObject( _WlMimoCfgObject *newObj,
                            const _WlMimoCfgObject *currObj,
                            const InstanceIdStack *iidStack,
                            char **errorParam,
                            CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlSesCfgObject( _WlSesCfgObject *newObj,
                           const _WlSesCfgObject *currObj,
                           const InstanceIdStack *iidStack,
                           char **errorParam,
                           CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_wlVirtIntfCfgObject( _WlVirtIntfCfgObject *newObj,
                                const _WlVirtIntfCfgObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam,
                                CmsRet *errorCode)
{
#ifdef BRCM_WLAN
    CmsRet ret = CMSRET_SUCCESS;
    InstanceIdStack iidStack1 = *iidStack;
    _WlBaseCfgObject *wlBaseCfgObj=NULL;
    int wlEnbl =0;


#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
    /*
     * If this wl interface is a Wifi WAN interface, return immediately.
     * Otherwise, we will (incorrectly) add this interface to the LAN side
     * bridge.
     */
    if (newObj && newObj->wlIfcname)
    {
        InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
        if (rutWanWifi_getWlIntfByIfName(newObj->wlIfcname, &iidStack2, NULL))
        {
            return CMSRET_SUCCESS;
        }
    }
#endif



    /*
     * Get the main wlan enble param from
     * InternetGatewayDevice.LANDevice.{i}.WLANConfiguration.{i}.X_BROADCOM_COM_WlanAdapter.WlBaseCfg.
     * since this is an "uncle" of the wlVirtCfgObject, we can just use cmsObj_getAncestor
     */
    if ((ret = cmsObj_getAncestorFlags(MDMOID_WL_BASE_CFG, MDMOID_WL_VIRT_INTF_CFG, &iidStack1, OGF_NO_VALUE_UPDATE, (void **)&wlBaseCfgObj)) != CMSRET_SUCCESS) {
        cmsLog_error("MDM MDMOID_WL_BASE_CFG failure, ret=%d", ret);
        return ret;
    }
    wlEnbl = wlBaseCfgObj->wlEnbl;
    cmsObj_free((void **) &wlBaseCfgObj);

    cmsLog_debug("wlEnbl=%d", wlEnbl);

    if (newObj != NULL && currObj == NULL)
    {
        /*
         * this is system startup or dynamic add of object.
         */
        if (wlEnbl && newObj->wlEnblSsid)
        {
            /*
             * new object is already in the enabled state.  Must be from a config
             * file during system bootup.  Need to add this interface to the bridge,
             * but no need to modify the filter entry because it is already in the
             * config file.
             */
            if (cmsNet_isInterfaceUp(newObj->wlIfcname))
            {
                /*
                 * During system startup, wlmngr has not started wl0, wl0.1,
                 * wl0.2, etc yet.  wlmngr will do another cmsObj_set
                 * to this object after it has started those interfaces.
                 */
                rutLan_addInterfaceToBridge(newObj->wlIfcname, FALSE, newObj->wlBrName);
            }
            else
            {
                cmsLog_debug("%s not up yet, will add later", newObj->wlIfcname);
            }
        }
    }

    if (newObj != NULL && currObj != NULL)
    {
        cmsLog_debug("potential modify: wlIfcname %s wlEnblSsid %d->%d wlBrName %s->%s",
                     currObj->wlIfcname,
                     currObj->wlEnblSsid, newObj->wlEnblSsid,
                     currObj->wlBrName, newObj->wlBrName);


        cmsLog_debug("wlEnbleSsid:%d->%d for if:%s\n", currObj->wlEnblSsid, newObj->wlEnblSsid, currObj->wlIfcname);
        /*
         * This is a modify case.  The only thing that is covered in the
         * modify case is change to wlEnblSsid and parent bridge ifname.
         */
        if ((newObj->wlEnblSsid != currObj->wlEnblSsid) || (cmsUtl_strcmp(newObj->wlBrName, currObj->wlBrName)))
        {
            /* remove wlVirtCfg from current bridge */
            if ( currObj->wlEnblSsid )
            {
            	rutWlan_clear_ap_stas(currObj,iidStack);
#ifdef DMP_BRIDGING_1
                /*
                 * This ssid _was_ enabled, so first unconditionally set the filter to -1.
                 * Later, we check if it is still enabled, and if so, add it to some bridge.
                 */
                rutWlan_disableVirtIntfFilter(currObj->wlIfcname);
#else

                /* If bridging is not defined, then we just remove this virtintf from the bridge. */
                rutLan_removeInterfaceFromBridge(currObj->wlIfcname, currObj->wlBrName);
#endif
            }

            if ( newObj->wlEnblSsid && wlEnbl ) {
                /*Add Bridge*/
                cmsLog_debug("add case: newObj->wlIfcname=%s under newObj->wlBrName=%s", newObj->wlIfcname, newObj->wlBrName);

#ifdef DMP_BRIDGING_1
                /*
                 * Ssid is now enabled, update filter, which will add this virtIntf
                 * to the appropriate bridge.
                 */
                rutWlan_enableVirtIntfFilter(newObj->wlIfcname, atoi(&(newObj->wlBrName[2])));
#else
                /* If bridging is not defined, add this virtIntf to the bridge directly. */
                rutLan_addInterfaceToBridge(newObj->wlIfcname, FALSE, newObj->wlBrName);
#endif
#ifdef SUPPORT_UNIFIED_WLMNGR
                wlMsgRestartWlan(NULL,newObj->wlIfcname[2]-'0');
#endif
            }
        }
    }

    if (newObj == NULL && currObj != NULL)
    {
        /* === delete case ===
         * We do not support delete of wlVirtIntfCfg objects.
         * Print error message if we detect this case.
         */
        cmsLog_error("Unexpected delete of wlIfcname=%s !!", currObj->wlIfcname);
    }


#endif /* BRCM_WLAN */

    return CMSRET_SUCCESS;
}

CmsRet rcl_wlMacFltObject( _WlMacFltObject *newObj,
                           const _WlMacFltObject *currObj,
                           const InstanceIdStack *iidStack,
                           char **errorParam,
                           CmsRet *errorCode)
{
    char *cmd;
    if (newObj == NULL)
        cmd="Delete";
    else if (currObj == NULL )
        cmd="Create";
    else
        cmd="Modify";
    return wlMsgRestartWlan(cmd,iidStack->instance[1]);
}

CmsRet rcl_wlKey64CfgObject( _WlKey64CfgObject *newObj,
                             const _WlKey64CfgObject *currObj,
                             const InstanceIdStack *iidStack,
                             char **errorParam,
                             CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlKey128CfgObject( _WlKey128CfgObject *newObj,
                              const _WlKey128CfgObject *currObj,
                              const InstanceIdStack *iidStack,
                              char **errorParam,
                              CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wlWpsCfgObject( _WlWpsCfgObject *newObj,
                           const _WlWpsCfgObject *currObj,
                           const InstanceIdStack *iidStack,
                           char **errorParam,
                           CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_lanWlanVirtMbssObject( _LanWlanVirtMbssObject *newObj,
                                  const _LanWlanVirtMbssObject *currObj,
                                  const InstanceIdStack *iidStack,
                                  char **errorParam,
                                  CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}

#endif  /* DMP_X_BROADCOM_COM_WIFILAN_1 */


#ifdef DMP_X_BROADCOM_COM_WIFILAN_1
/*
 * these next 3 certificate related functions could be used for general
 * certificate purposes on the system, but for now, only used by WLAN code.
 */
CmsRet rcl_wapiCertificateObject( _WapiCertificateObject *newObj,
                                  const _WapiCertificateObject *currObj,
                                  const InstanceIdStack *iidStack,
                                  char **errorParam,
                                  CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wapiAsCertificateObject( _WapiAsCertificateObject *newObj,
                                    const _WapiAsCertificateObject *currObj,
                                    const InstanceIdStack *iidStack,
                                    char **errorParam,
                                    CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_wapiIssuedCertificateObject( _WapiIssuedCertificateObject *newObj,
                                        const _WapiIssuedCertificateObject *currObj,
                                        const InstanceIdStack *iidStack,
                                        char **errorParam,
                                        CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}
#endif  /* DMP_X_BROADCOM_COM_WIFILAN_1 */


#ifdef DMP_X_BROADCOM_COM_WIFILAN_1
CmsRet rcl_wlanNvramObject( _WlanNvramObject *newObj,
                            const _WlanNvramObject *currObj,
                            const InstanceIdStack *iidStack,
                            char **errorParam,
                            CmsRet *errorCode)
{
    return CMSRET_SUCCESS;
}
#endif  /* DMP_X_BROADCOM_COM_WIFILAN_1 */
