#ifdef DMP_X_BROADCOM_COM_EPON_1
/***********************************************************************
 *
 *  Copyright (c) 2009-2012  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2012:proprietary:standard
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

#include "cms_core.h"
#include "cms_util.h"
#include "stl.h"

#include "rut_pon_voice.h"

#include "rut_eponwan.h"

#ifdef BRCM_VOICE_SUPPORT

CmsRet stl_cTOAMObject(_CTOAMObject *obj, const InstanceIdStack *iidStack)
{

    return CMSRET_SUCCESS_OBJECT_UNCHANGED;


}

CmsRet stl_oAMIADInfoObject(_OAMIADInfoObject *obj, const InstanceIdStack *iidStack)
{

#define SIZE_MACADDR     6
    iadRec iadInfo;
    char *str = NULL;
    memset(&iadInfo, 0, sizeof(iadRec));
    if(GetIadInfo(POTS_PORT_0, &iadInfo) == CMSRET_SUCCESS)
    {


	    obj->protocolSupported = iadInfo.protocolSupported;
            obj->MACAddress = cmsMem_alloc((SIZE_MACADDR*4)+1, 
			                      ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
            if(obj->MACAddress == NULL)
	    {
		    cmsLog_error("fail to allocate mem for iadInfo.macAddress");
		    return CMSRET_INTERNAL_ERROR;
	    }
	    cmsUtl_binaryBufToHexString(iadInfo.macAddress, SIZE_MACADDR, &str);
	    memcpy(obj->MACAddress, str, SIZE_MACADDR * 2);
	    CMSMEM_FREE_BUF_AND_NULL_PTR(str);

            obj->iadSoftwareVersion = cmsMem_alloc((SW_VERSION_SIZE*2)+1, 
			                      ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
            if(obj->iadSoftwareVersion == NULL)
	    {
		    cmsLog_error("fail to allocate mem for iadInfo.iadSwVersion");
		    return CMSRET_INTERNAL_ERROR;
	    }
	    cmsUtl_binaryBufToHexString(iadInfo.iadSwVersion, SW_VERSION_SIZE, &str);
	    memcpy(obj->iadSoftwareVersion, str, SW_VERSION_SIZE * 2);
	    CMSMEM_FREE_BUF_AND_NULL_PTR(str);

            obj->iadSoftwaretime = cmsMem_alloc((SW_TIME_SIZE*2)+1, 
			                      ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
            if(obj->iadSoftwaretime == NULL)
	    {
		    cmsLog_error("fail to allocate mem for iadInfo.iadSwTime");
		    return CMSRET_INTERNAL_ERROR;
	    }
	    cmsUtl_binaryBufToHexString(iadInfo.iadSwTime, SW_TIME_SIZE, &str);
	    memcpy(obj->iadSoftwaretime, str, SW_TIME_SIZE * 2);
	    CMSMEM_FREE_BUF_AND_NULL_PTR(str);

	    obj->voipUserCount = iadInfo.voipUserCount;



    }
    else
    {
	    cmsLog_error("error updating the iadInfo");
    }

    return CMSRET_SUCCESS_OBJECT_UNCHANGED;

}

CmsRet stl_oAMGlbParamCfgObject(_OAMGlbParamCfgObject *obj, const InstanceIdStack *iidStack)
{

    
    globalConfigRec glbCfg;
    CmsRet ret = CMSRET_SUCCESS;
    memset(&glbCfg, 0, sizeof(globalConfigRec));
    if( (ret = GetGlobalConfig( POTS_PORT_0, &glbCfg )) == CMSRET_SUCCESS )
    { 
	    obj->voiceIpMode = glbCfg.voiceIpMode;
	    obj->ipAddr= glbCfg.iadIpAddress;
	    obj->netMask = glbCfg.iadNetMask;
	    obj->defaultGW = glbCfg.iadDefaultGW;
	    obj->PPPoEMode = glbCfg.pppoeMode;

	    obj->PPPOEusername = cmsMem_strdupFlags((char *)glbCfg.pppoeUsername, mdmLibCtx.allocFlags);
	    obj->PPPOEpassword = cmsMem_strdupFlags((char *)glbCfg.pppoePassword, mdmLibCtx.allocFlags);
	    obj->status = glbCfg.status;
           return CMSRET_SUCCESS_OBJECT_UNCHANGED;
    }

    return ret;

}

CmsRet stl_oAMSIPParamCfgObject(_OAMSIPParamCfgObject *obj, const InstanceIdStack *iidStack)
{

    sipConfigRec glbCfg;
    if( GetSipConfig( POTS_PORT_0, &glbCfg ) == CMSRET_SUCCESS )
    { 

	obj->sipProxyServIp = glbCfg.sipProxyAddress[3] | 
                                 glbCfg.sipProxyAddress[2]<<8 | 
                                 glbCfg.sipProxyAddress[1]<<16 |
                                 glbCfg.sipProxyAddress[0]<<24;
	
	obj->sipProxyServComPortNo = glbCfg.sipProxyComPort[0]<<8 | glbCfg.sipProxyComPort[1];
        obj->backupSipProxyServIp = 0;

	obj->sipRegServIP = glbCfg.sipRegistrarAddress[3] | 
                                 glbCfg.sipRegistrarAddress[2]<<8 | 
                                 glbCfg.sipRegistrarAddress[1]<<16 |
                                 glbCfg.sipRegistrarAddress[0]<<24;

	obj->sipRegServComPortNo = glbCfg.sipRegistrarComPort[0]<<8 | 
		                               glbCfg.sipRegistrarComPort[1];

	obj->outBoundServPortIP  = glbCfg.outboundServerAddress[3] | 
                                 glbCfg.outboundServerAddress[2]<<8 | 
                                 glbCfg.outboundServerAddress[1]<<16 |
                                 glbCfg.outboundServerAddress[0]<<24;

	obj->outBoundServPortNo  = glbCfg.outboundServerComPort[0]<<8 | 
		                               glbCfg.outboundServerComPort[1];

	obj->sipRegIntervalSIP = glbCfg.regRefreshCycle[3] |
                                 glbCfg.regRefreshCycle[2]<<8 |
                                 glbCfg.regRefreshCycle[1]<<16 |
                                 glbCfg.regRefreshCycle[0]<<24;
    }
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;


}


					       							

CmsRet stl_oAMSIPUsrParamCfgObject(_OAMSIPUsrParamCfgObject *obj, const InstanceIdStack *iidStack)
{
    sipUserConfigRec glbCfg;
    char *str = NULL;
    unsigned int portid=0;
    _CTOAMObject *portobj;
   InstanceIdStack portiidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;

      ret = cmsObj_get(MDMOID_CTOAM,&portiidStack,0,(void **)&portobj);
      if(ret == CMSRET_SUCCESS)
      {
            if(portobj->portID < POTS_PORT_MAX)
            portid = portobj->portID;
      	}

	 	 	
#if 1
    if( GetSipUserConfig( portid, &glbCfg ) == CMSRET_SUCCESS )
    {
	obj->userAccount = cmsMem_alloc(sizeof(glbCfg.userAcct)*2+1,
			ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
	cmsUtl_binaryBufToHexString(glbCfg.userAcct, sizeof(glbCfg.userAcct), &str);

	if(str){
   	      memcpy(obj->userAccount, str, sizeof(glbCfg.userAcct)*2);
   	      CMSMEM_FREE_BUF_AND_NULL_PTR(str);
	}

	obj->username = cmsMem_alloc(sizeof(glbCfg.userName)*2+1,
			ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
	cmsUtl_binaryBufToHexString(glbCfg.userName, sizeof(glbCfg.userName), &str);
	if(str){	
	   memcpy(obj->username, str, sizeof(glbCfg.userName)*2);
	   CMSMEM_FREE_BUF_AND_NULL_PTR(str);
	}

	obj->userPassword = cmsMem_alloc(sizeof(glbCfg.userPass)*2+1,
			ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
	cmsUtl_binaryBufToHexString(glbCfg.userPass, sizeof(glbCfg.userPass), &str);
	if(str){	
	   memcpy(obj->userPassword, str, sizeof(glbCfg.userPass)*2);
	   CMSMEM_FREE_BUF_AND_NULL_PTR(str);
	}

    }
#endif
    return CMSRET_SUCCESS_OBJECT_UNCHANGED;


}

CmsRet stl_oAMFAXMDCfgObject(_OAMFAXMDCfgObject *obj, const InstanceIdStack *iidStack)
{

    faxConfigRec glbCfg;

    if( GetFaxConfig( POTS_PORT_0, &glbCfg ) == CMSRET_SUCCESS )
    {
	obj->voiceT38Enable = glbCfg.voiceT38Enable;
	obj->voiceFaxModemControl = glbCfg.voiceFaxModemControl;
    }

    return CMSRET_SUCCESS_OBJECT_UNCHANGED;


}

CmsRet stl_oAMPOTSStsObject(_OAMPOTSStsObject *obj, const InstanceIdStack *iidStack)
{
    potsStatusRec glbCfg;
    _CTOAMObject *portobj;
   InstanceIdStack portiidStack=EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;
    unsigned int portid=0;
    ret = cmsObj_get(MDMOID_CTOAM,
                         &portiidStack,0,
			 (void **)&portobj);

      if(ret == CMSRET_SUCCESS)
      {
            if(portobj->portID < POTS_PORT_MAX)
            portid = portobj->portID;
      	}
	  
    if( GetPotsStatus( portid, &glbCfg ) == CMSRET_SUCCESS )
    {
	memcpy(&obj->IADPortStatus, &glbCfg.iadPortStatus, sizeof(glbCfg.iadPortStatus));
	memcpy(&obj->portServiceState, &glbCfg.iadPortServiceState, sizeof(glbCfg.iadPortServiceState));
	memcpy(&obj->portCodecMode, &glbCfg.iadPortCodecMode, sizeof(glbCfg.iadPortCodecMode));
    }

    return CMSRET_SUCCESS_OBJECT_UNCHANGED;


}

CmsRet stl_oAMIADOpObject(_OAMIADOpObject *obj, const InstanceIdStack *iidStack)
{

    return CMSRET_SUCCESS_OBJECT_UNCHANGED;


}

CmsRet stl_oAMSIPDgtMpObject(_OAMSIPDgtMpObject *obj, const InstanceIdStack *iidStack)
{

    return CMSRET_SUCCESS_OBJECT_UNCHANGED;


}

#endif //BRCM_VOICE_SUPPORT

#endif /* DMP_X_BROADCOM_COM_EPON_1 */
