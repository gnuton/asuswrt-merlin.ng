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

#ifdef DMP_DEVICE2_WIFIRADIO_1

/*!\file rcl2_wifi.c
 * \brief This file contains TR181 and X_BROADCOM_COM Wifi objects which
 *        are used by both the AccessPoint and EndPoint.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "mdm.h"
#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_wifi.h"
#include "cms_qdm.h"

#include "wlcsm_lib_api.h"


CmsRet rcl_dev2WifiObject( _Dev2WifiObject *newObj __attribute__((unused)),
                           const _Dev2WifiObject *currObj __attribute__((unused)),
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_dev2WifiRadioObject( _Dev2WifiRadioObject *newObj,
                                const _Dev2WifiRadioObject *currObj,
                                const InstanceIdStack *iidStack,
                                char **errorParam __attribute__((unused)),
                                CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    UINT32 radioIndex;
    IF_STATUS_HAS_CHANGED_SET_LASTCHANGE(newObj, currObj);

	if (newObj && currObj && mdmLibCtx.eid!=EID_WLMNGR) {
		if (newObj->autoChannelEnable !=currObj->autoChannelEnable) {
			if(newObj->autoChannelEnable) 
				newObj->X_BROADCOM_COM_WlChannel=0;
			else if(!newObj->channel) 
				newObj->X_BROADCOM_COM_WlChannel=1;
		} else if(newObj->channel != currObj->channel && newObj->channel) {
			newObj->autoChannelEnable=0;
			newObj->X_BROADCOM_COM_WlChannel=newObj->channel;
		}

		if(cmsUtl_strcasecmp(newObj->regulatoryDomain,currObj->regulatoryDomain)) {
			/*if the regulatoryDomain is changed, we need to split it to two parts
			 *for broadcom entries*/
			char ccode[4]= {0};
			char regrev=-1;
			if(!wlcsm_wl_parse_countryrev(newObj->regulatoryDomain,ccode,&regrev)) {
				if(cmsUtl_strcasecmp(newObj->X_BROADCOM_COM_WlCountry,ccode))
					CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_WlCountry,ccode,mdmLibCtx.allocFlags);
			}
		}
	}

    /* example of getting radioIndex */
    sscanf(newObj->name, "wl%u", &radioIndex);

    if (mdmLibCtx.eid == EID_TR69C || mdmLibCtx.eid == EID_CWMPD) {
        paramNodeList *changedParams = NULL;

        if ((ret = cmsObj_compareObjects(newObj, currObj, &changedParams)) == CMSRET_SUCCESS)
        {
            if (changedParams != NULL)
            {
                ret = rutWifi_updateWlmngr(MDMOID_DEV2_WIFI_RADIO, radioIndex, 0, 0, changedParams);

                while (changedParams)
                {
                    paramNodeList *tmpParamNode = changedParams;
                    changedParams = tmpParamNode->nextNode;
                    cmsMem_free(tmpParamNode);
                }
            }
        }
    }  else if (mdmLibCtx.eid != EID_WLMNGR && mdmLibCtx.eid != EID_SSK) {
        wlcsm_mngr_restart(radioIndex,WLCSM_MNGR_RESTART_MDM,WLCSM_MNGR_RESTART_NOSAVEDM,0);
        ret=CMSRET_SUCCESS;
    }
    return ret;
}


CmsRet rcl_dev2WifiRadioStatsObject( _Dev2WifiRadioStatsObject *newObj __attribute__((unused)),
                                     const _Dev2WifiRadioStatsObject *currObj __attribute__((unused)),
                                     const InstanceIdStack *iidStack __attribute__((unused)),
                                     char **errorParam __attribute__((unused)),
                                     CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}


CmsRet rcl_dev2WifiSsidObject( _Dev2WifiSsidObject *newObj,
                               const _Dev2WifiSsidObject *currObj,
                               const InstanceIdStack *iidStack __attribute__((unused)),
                               char **errorParam __attribute__((unused)),
                               CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
    UINT32 radioIndex;
    UBOOL8 existInBridge=FALSE;
    UBOOL8 portObjFound=FALSE;
    UBOOL8 ChangeBridgeGroup=FALSE;
    char brIntfNameBuf[CMS_IFNAME_LENGTH]= {0};
    InstanceIdStack iidStack2=EMPTY_INSTANCE_ID_STACK;
    Dev2BridgePortObject *portObj=NULL;

    IF_STATUS_HAS_CHANGED_SET_LASTCHANGE(newObj, currObj);

    if (ADD_NEW(newObj, currObj))
    {
        rutUtil_modifyNumWifiSsid(iidStack, 1);
        if (mdmLibCtx.eid == EID_SSK)
            return CMSRET_SUCCESS;
        else
            return CMSRET_REQUEST_DENIED;
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        rutUtil_modifyNumWifiSsid(iidStack, -1);
        return CMSRET_SUCCESS;
    }

    radioIndex = newObj->X_BROADCOM_COM_Adapter;

    /* find non-mgmt port with specified intfName */
    while (!portObjFound &&
            cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE_PORT, &iidStack2,
                                OGF_NO_VALUE_UPDATE,
                                (void **) &portObj) == CMSRET_SUCCESS)
    {
        if (!portObj->managementPort &&
                !cmsUtl_strcmp(portObj->name, newObj->name))
        {
            portObjFound = TRUE;
        }

        cmsObj_free((void **)&portObj);
        if(portObjFound)
        {
            ret = rutBridge_getParentBridgeIntfName_dev2(&iidStack2, brIntfNameBuf);
            if (ret == CMSRET_SUCCESS)
            {
                if(!cmsUtl_strcmp(brIntfNameBuf, newObj->X_BROADCOM_COM_WlBrName))
                {
                    cmsLog_debug("#### FOUND!! %s is in %s group ####\n", newObj->name, newObj->X_BROADCOM_COM_WlBrName);
                    existInBridge = TRUE;
                }
            }
            break;
        }
    }

   
   if (   (  newObj &&  newObj->enable )
        &&( currObj && currObj->enable )
        && portObjFound && !existInBridge 
        && (cmsUtl_strcmp(currObj->X_BROADCOM_COM_WlBrName, newObj->X_BROADCOM_COM_WlBrName)))
    {
       /* Check ssid in target bridge or not. If exist in target bridge,
        * nothing to do for change bridge group 
        */

        ChangeBridgeGroup = TRUE;
    }

    cmsLog_debug("inf:%s(En:%d) CurBr:%s NewBr:%s portObj:%d extInBr:%d ChaBr:%d\n"
                ,newObj->name,newObj->enable
                ,currObj->X_BROADCOM_COM_WlBrName
                ,newObj->X_BROADCOM_COM_WlBrName
                ,portObjFound,existInBridge,ChangeBridgeGroup
            );

    if ((!newObj->enable && existInBridge) || ChangeBridgeGroup)
    {
        cmsLog_debug("===> delete interface %s from  bridge %s!\n",newObj->name,brIntfNameBuf);
        rutBridge_deleteIntfNameFromBridge_dev2(currObj->name);
    }

    if((newObj->enable && !existInBridge) || ChangeBridgeGroup)
    {
        cmsLog_debug("===> add interface %s to  bridge %s!\n",newObj->name,newObj->X_BROADCOM_COM_WlBrName);
        ret = rutBridge_addIntfNameToBridge_dev2(newObj->name, newObj->X_BROADCOM_COM_WlBrName);
        if(ret != CMSRET_SUCCESS)
            cmsLog_error("add interface %s to  bridge %s error!\n",newObj->name,newObj->X_BROADCOM_COM_WlBrName);
    }

    if (mdmLibCtx.eid == EID_TR69C || mdmLibCtx.eid == EID_CWMPD)
    {
        MdmPathDescriptor pathDesc;
        char *fullPathString=NULL;
        paramNodeList *changedParams = NULL;

        INIT_PATH_DESCRIPTOR(&pathDesc);
        pathDesc.oid = MDMOID_DEV2_WIFI_SSID;
        memcpy(&pathDesc.iidStack, iidStack, sizeof(InstanceIdStack));
        cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathString);

        radioIndex = newObj->X_BROADCOM_COM_Adapter;

        if ((ret = cmsObj_compareObjects(newObj, currObj, &changedParams)) == CMSRET_SUCCESS)
        {
            int ssidIndex = newObj->X_BROADCOM_COM_Index;
            if (changedParams != NULL)
            {
                ret = rutWifi_updateWlmngr(MDMOID_DEV2_WIFI_SSID, radioIndex, ssidIndex, 0, changedParams);

                while (changedParams)
                {
                    paramNodeList *tmpParamNode = changedParams;
                    changedParams = tmpParamNode->nextNode;
                    cmsMem_free(tmpParamNode);
                }
            }
        }
        CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
    }  else if (mdmLibCtx.eid != EID_WLMNGR && mdmLibCtx.eid != EID_SSK) {
        wlcsm_mngr_restart(radioIndex,WLCSM_MNGR_RESTART_MDM,WLCSM_MNGR_RESTART_NOSAVEDM,0);
        ret=CMSRET_SUCCESS;
    }

    return ret;
}

CmsRet rcl_dev2WifiSsidStatsObject( _Dev2WifiSsidStatsObject *newObj __attribute__((unused)),
                                    const _Dev2WifiSsidStatsObject *currObj __attribute__((unused)),
                                    const InstanceIdStack *iidStack __attribute__((unused)),
                                    char **errorParam __attribute__((unused)),
                                    CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

#endif  /* DMP_DEVICE2_WIFIRADIO_1 */

#endif    /* DMP_DEVICE2_BASELINE_1 */
