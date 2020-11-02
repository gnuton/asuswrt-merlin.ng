#ifdef DMP_X_CT_ORG_EPON_1
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

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_dal.h"
#include "rcl.h"
#include "rut_util.h"

#include "os_defs.h"


#ifdef BRCM_VOICE_SUPPORT
#include "cms_core.h"
#include "cms_util.h"
#include "rut_pon_voice.h"
#include "rut_eponwan.h"

CmsRet rcl_cTOAMObject( _CTOAMObject *newObj,
                const _CTOAMObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   CmsRet ret = CMSRET_SUCCESS;
   if (ADD_NEW(newObj, currObj))
   {

#if 0
   {
      WEB_NTWK_VAR tmpglbWebVar;  
      sprintf(tmpglbWebVar.wanL2IfName, "%s", EPON_WAN_IF_NAME);

      tmpglbWebVar.connMode = 0;
      if (dalEth_addEthInterface(&tmpglbWebVar) != CMSRET_SUCCESS)
      {
         return CMSRET_INTERNAL_ERROR;
      }
   }
#endif

#if defined(EPON_HGU)
   InstanceIdStack liidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 existingEthFound;
   InterfaceControlObject *ifcObj = NULL;
   WanEponIntfObject *wanEponObj = NULL;

   existingEthFound = rutEpon_getEponLinkByIfName(EPON_WAN_IF_NAME, &liidStack, NULL);
   if (existingEthFound)
   {
      cmsLog_error("EPON WAN interface already exists \n");
      return CMSRET_SUCCESS;
   }

   /*
    * Use the Interface Control object to move the epon interface from LAN side
    * to WAN side.
    */
   INIT_INSTANCE_ID_STACK(&liidStack);
   if ((ret = cmsObj_get(MDMOID_INTERFACE_CONTROL, &liidStack, 0, (void **) &ifcObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get InterfaceControlObject, ret=%d", ret);
      return ret;
   }

   CMSMEM_REPLACE_STRING_FLAGS(ifcObj->ifName, EPON_WAN_IF_NAME, mdmLibCtx.allocFlags);
   ifcObj->moveToLANSide = FALSE;
   ifcObj->moveToWANSide = TRUE;

   /* set InterfaceControlObject */
   if ((ret = cmsObj_set(ifcObj, &liidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set InterfaceControlObject, ret = %d", ret);
      cmsObj_free((void **) &ifcObj);
      return ret;
   }         

   cmsObj_free((void **) &ifcObj);

   /*
    * Find the WANDevice that our epon interface was created under.
    */
   INIT_INSTANCE_ID_STACK(&liidStack);
   existingEthFound = rutEpon_getEponIntfByIfName((char *) EPON_WAN_IF_NAME, &liidStack, &wanEponObj);
   if (!existingEthFound)
   {
      cmsLog_error("could not find the eponWan object!");
      return CMSRET_INTERNAL_ERROR;
   }

   cmsLog_debug("WAN %s created at %s", EPON_WAN_IF_NAME, cmsMdm_dumpIidStack(&liidStack));

#if 0 //dont need in EPON
   /* set connection mode value */
   if (webVar->connMode == CMS_CONNECTION_MODE_VLANMUX)
   {
      CMSMEM_REPLACE_STRING_FLAGS(wanEponObj->X_BROADCOM_COM_ConnectionMode, MDMVS_VLANMUXMODE, mdmLibCtx.allocFlags);
   }
   else
#endif
   {
      CMSMEM_REPLACE_STRING_FLAGS(wanEponObj->connectionMode, MDMVS_DEFAULTMODE, mdmLibCtx.allocFlags);
   }

   wanEponObj->enable = TRUE;

   if ((ret = cmsObj_set(wanEponObj, &liidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to set WanEponIntfObject, ret = %d", ret);
   }
   cmsObj_free((void **) &wanEponObj);


   /*
    * Also create a single WANConnectionDevice in this WANDevice.
    */
   if ((ret = cmsObj_addInstance(MDMOID_WAN_CONN_DEVICE, &liidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create new WanConnectionDevice, ret=%d", ret);
      return ret;
   }

   cmsLog_debug("Exit, ret=%d", ret);
   
   return ret;

#endif
   }
   return ret;
}

CmsRet rcl_oAMIADInfoObject( _OAMIADInfoObject *newObj,
                const _OAMIADInfoObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    if (ADD_NEW(newObj, currObj))
    {
        return ret;
    }
    return ret;
}

CmsRet rcl_oAMGlbParamCfgObject( _OAMGlbParamCfgObject *newObj,
                const _OAMGlbParamCfgObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    globalConfigRec rec;


    if (ADD_NEW(newObj, currObj))
    {
        return ret;
    }
   
    rec.voiceIpMode  = newObj->voiceIpMode;
    rec.iadIpAddress = newObj->ipAddr;
    rec.iadNetMask   = newObj->netMask;
    rec.iadDefaultGW = newObj->defaultGW;
    rec.pppoeMode    = newObj->PPPoEMode;
    if(newObj->PPPOEusername != NULL)
        sprintf((char *)rec.pppoeUsername, "%s", newObj->PPPOEusername);
    if(newObj->PPPOEpassword != NULL)
        sprintf((char *)rec.pppoePassword, "%s", newObj->PPPOEpassword);

    ret = SetGlobalConfig(POTS_PORT_0, &rec);

    return ret;
}


CmsRet rcl_oAMSIPParamCfgObject( _OAMSIPParamCfgObject *newObj,
                const _OAMSIPParamCfgObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    sipConfigRec rec;

    if (ADD_NEW(newObj, currObj))
    {
        return ret;
    }

    rec.sipProxyAddress[0] = newObj->sipProxyServIp >> 24;
    rec.sipProxyAddress[1] = (newObj->sipProxyServIp >> 16) & 0xff;
    rec.sipProxyAddress[2] = (newObj->sipProxyServIp >> 8) & 0xff;
    rec.sipProxyAddress[3] = newObj->sipProxyServIp & 0xff;
    printf("the new value or proxy port is %d\r\n", newObj->sipProxyServComPortNo);
    rec.sipProxyComPort[0] = (newObj->sipProxyServComPortNo >> 8) & 0xff;
    rec.sipProxyComPort[1] = (newObj->sipProxyServComPortNo) & 0xff;

    rec.sipRegistrarAddress[0] = newObj->sipRegServIP >> 24;
    rec.sipRegistrarAddress[1] = (newObj->sipRegServIP >> 16) & 0xff;
    rec.sipRegistrarAddress[2] = (newObj->sipRegServIP >> 8)  & 0xff;
    rec.sipRegistrarAddress[3] = newObj->sipRegServIP & 0xff;
    printf("the new value or comport is %d\r\n", newObj->sipRegServComPortNo);
    rec.sipRegistrarComPort[0] = (newObj->sipRegServComPortNo >> 8) & 0xff;
    rec.sipRegistrarComPort[1] = (newObj->sipRegServComPortNo) & 0xff;
    printf("the new value or regsrv port  is %d\r\n", newObj->sipRegServComPortNo);
    rec.outboundServerAddress[0] = newObj->outBoundServPortIP >> 24;
    rec.outboundServerAddress[1] = (newObj->outBoundServPortIP >> 16) & 0xff;
    rec.outboundServerAddress[2] = (newObj->outBoundServPortIP >> 8)  & 0xff;
    rec.outboundServerAddress[3] = newObj->outBoundServPortIP & 0xff;
    rec.outboundServerComPort[0] = (newObj->outBoundServPortNo>> 8) & 0xff;
    rec.outboundServerComPort[1] = (newObj->outBoundServPortNo) & 0xff;

    memcpy(&rec.regRefreshCycle , &newObj->heartbeatCycle, REGISTRATION_SIZE);

    ret = SetSipConfig(POTS_PORT_0, &rec);

    return ret;
}

CmsRet rcl_oAMSIPUsrParamCfgObject( _OAMSIPUsrParamCfgObject *newObj,
                const _OAMSIPUsrParamCfgObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    int portid = 0;
    sipUserConfigRec glbCfg;
    unsigned int  binSize;
    char  *bin=NULL;    
    _CTOAMObject *portobj;
    InstanceIdStack portiidStack=EMPTY_INSTANCE_ID_STACK;
   
    if (ADD_NEW(newObj, currObj))
    {
        return ret;
    }

    ret = cmsObj_get(MDMOID_CTOAM,&portiidStack,0,(void **)&portobj);
    if(ret == CMSRET_SUCCESS)
    {
        if(portobj->portID < POTS_PORT_MAX)
        portid = portobj->portID;
    }


    if(newObj->userAccount){
        cmsUtl_hexStringToBinaryBuf(newObj->userAccount, (UINT8 **)&bin, &binSize);
        memcpy(glbCfg.userAcct, bin, sizeof(glbCfg.userAcct)); 
        cmsMem_free(bin);
    }

    if(newObj->username){
        cmsUtl_hexStringToBinaryBuf(newObj->username, (UINT8 **)&bin, &binSize);
        memcpy(glbCfg.userName, bin, sizeof(glbCfg.userName)); 
        cmsMem_free(bin);
    }


    if(newObj->userPassword){
        cmsUtl_hexStringToBinaryBuf(newObj->userPassword, (UINT8 **)&bin, &binSize);
        memcpy(glbCfg.userPass, bin, sizeof(glbCfg.userPass)); 
        cmsMem_free(bin);
    }


    ret = SetSipUserConfig( portid, &glbCfg );
   
    return ret;

}

CmsRet rcl_oAMFAXMDCfgObject( _OAMFAXMDCfgObject *newObj,
                const _OAMFAXMDCfgObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;

   if (ADD_NEW(newObj, currObj))
   {
      return ret;
   }

    faxConfigRec rec;
    rec.voiceT38Enable = newObj->voiceT38Enable;
    rec.voiceFaxModemControl = newObj->voiceFaxModemControl;

    ret = SetFaxConfig(POTS_PORT_0,&rec);

    return ret;

}

CmsRet rcl_oAMPOTSStsObject( _OAMPOTSStsObject *newObj,
                const _OAMPOTSStsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    if (ADD_NEW(newObj, currObj))
    {
        return ret;
    }
    return ret;
}

CmsRet rcl_oAMIADOpObject( _OAMIADOpObject *newObj,
                const _OAMIADOpObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;
    static int firstTime = 1;

#if 0
#if defined(EPON_HGU)
    InstanceIdStack liidStack = EMPTY_INSTANCE_ID_STACK;
    UBOOL8 existingEthFound;
    InstanceIdStack parentIidStack;
    _WanPppConnObject *pppConn = NULL;
    _WanIpConnObject  *ipConn = NULL;
#endif
    char eponIpAddress[20] = "0.0.0.0";
    char ifName[20] = "";
    UBOOL8 found = FALSE;

    if (ADD_NEW(newObj, currObj))
    {
        return ret;
    }

#if defined(EPON_HGU)
    existingEthFound = rutEpon_getEponIntfByIfName(EPON_WAN_IF_NAME, &liidStack, NULL);
    if (!existingEthFound)
    {
       cmsLog_error("%s does not exist \n", EPON_WAN_IF_NAME);
       return CMSRET_OBJECT_NOT_FOUND;
    }
  
    /* Get Epon WanDevice iidStack first */      
    if ((ret = rutWl2_getEponWanIidStack(&parentIidStack)) != CMSRET_SUCCESS)
    {
       return ret;
    }    

    INIT_INSTANCE_ID_STACK(&liidStack);
    while (!found &&cmsObj_getNextInSubTreeFlags(MDMOID_WAN_IP_CONN, &parentIidStack, &liidStack, OGF_NO_VALUE_UPDATE, (void **)&ipConn) == CMSRET_SUCCESS)
    {
       if (!cmsUtl_strcmp(ipConn->X_BROADCOM_COM_IfName, "epon0"))
       {
          strcpy(eponIpAddress, ipConn->externalIPAddress);
          found = TRUE;
       }
       cmsObj_free((void **) &ipConn);
    }
   
    INIT_INSTANCE_ID_STACK(&liidStack);
    while (!found && cmsObj_getNextInSubTreeFlags(MDMOID_WAN_PPP_CONN, &parentIidStack, &liidStack, OGF_NO_VALUE_UPDATE, (void **)&pppConn) == CMSRET_SUCCESS)
    {
       if (!cmsUtl_strcmp(pppConn->X_BROADCOM_COM_IfName, "epon0"))
       {
          strcpy(eponIpAddress, pppConn->externalIPAddress);
          found = TRUE;
       }         
       cmsObj_free((void **) &pppConn);
    }
    strcpy(ifName, EPON_WAN_IF_NAME);
#else
    {
       InstanceIdStack ipIntfIidStack = EMPTY_INSTANCE_ID_STACK;
       LanIpIntfObject *ipIntfObj = NULL;            

       while (!found && 
                ((ret = cmsObj_getNextFlags(MDMOID_LAN_IP_INTF, &ipIntfIidStack, OGF_NO_VALUE_UPDATE, (void **) &ipIntfObj)) == CMSRET_SUCCESS))
       {
          if (cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_IfName,"br0") == 0)  
          {
             found = TRUE;      
          }
          else
          {
             cmsObj_free((void **) &ipIntfObj);      
          }
       }
       
       if (!found)
       {
          cmsLog_error("br0 is not found ?. ret=%d", ret);
          return CMSRET_INTERNAL_ERROR;
       }
    
       strcpy(eponIpAddress, ipIntfObj->IPInterfaceIPAddress);

       cmsObj_free((void **) &ipIntfObj);     
    }

    strcpy(ifName, "br0");
#endif

    printf("ifName = %s \n", ifName);
    printf("IP Address = %s \n", eponIpAddress);
#endif

    if (newObj->operation == 0) {
        /* Reregister with Soft Switch. Send SIP Register Message */
        cmsLog_error("Re register with softswitch is not yet supported \n");
    } else if (newObj->operation == 1) {
        /* Logout From Soft Switch. Send SIP Register with expiry = 0 */
        cmsLog_error("Logout from softswitch is not yet supported \n");
    } else if (newObj->operation == 2 /* RESET_VOICE_MODULE */) {
        /* Restart the Voice Thread */
        if (firstTime) {
            printf("Sending UPLOAD_COMPLETE message \n");
            ret = SendUploadComplete();
            firstTime = 0;
        } else {
            printf("Sending REBOOT_VOICE message \n");
            ret = SendRebootVoice();
#if 0
            ret = setVoipBoundIfNameAddress(ifName, "0.0.0.0");
            sleep(1);
            cmsLog_error(" op2: bound ipaddr is %x \n", eponIpAddress);
            ret = setVoipBoundIfNameAddress(ifName, eponIpAddress);
#endif
        }
    }
    return ret;
}

CmsRet rcl_oAMSIPDgtMpObject( _OAMSIPDgtMpObject *newObj,
                const _OAMSIPDgtMpObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
    CmsRet ret = CMSRET_SUCCESS;

    if (ADD_NEW(newObj, currObj))
    {
        return ret;
    }
   
    return ret;
}
#endif   //BRCM_VOICE_SUPPORT
#endif   //DMP_X_CT_ORG_EPON_1
