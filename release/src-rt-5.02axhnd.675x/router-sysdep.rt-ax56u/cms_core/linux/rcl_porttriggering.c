/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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

#include "odl.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_iptables.h"


CmsRet rcl_wanPppConnPortTriggeringObject( _WanPppConnPortTriggeringObject *newObj __attribute__((unused)),
                const _WanPppConnPortTriggeringObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;

    WanPppConnObject *wan_ppp_conn = NULL;
    InstanceIdStack parentIidStack = *iidStack;
    char ifName[BUFLEN_32], cmd[BUFLEN_128];
    int trigger_proto, open_proto;

    cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));   
    if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
    {
        return ret;
    }

    if (cmsObj_getAncestor(MDMOID_WAN_PPP_CONN, MDMOID_WAN_PPP_CONN_PORT_TRIGGERING,    
                                                     &parentIidStack, (void **) &wan_ppp_conn) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get WanIpConnObject. ret=%d", ret);
        return CMSRET_INTERNAL_ERROR;
    }

    strncpy(ifName, wan_ppp_conn->X_BROADCOM_COM_IfName, sizeof(ifName));
    cmsObj_free((void **) &wan_ppp_conn);   

    rutIpt_insertPortTriggeringModules();

    if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
    {
        cmsLog_debug("PortTrigging add : %s %s %s %d %d %d %d %s\n", 
                newObj->name, newObj->triggerProtocol, newObj->openProtocol, newObj->triggerPortStart, newObj->triggerPortEnd, newObj->openPortStart, newObj->openPortEnd, ifName);

        if ( !strcmp(newObj->triggerProtocol, "TCP") ) 
            trigger_proto = 1;
        else if ( !strcmp(newObj->triggerProtocol, "UDP") )
            trigger_proto = 2;
        else
            trigger_proto = 3;

        if ( !strcmp(newObj->openProtocol, "TCP") ) 
            open_proto = 1;
        else if ( !strcmp(newObj->openProtocol, "UDP") )
            open_proto = 2;
        else
            open_proto = 3;

        sprintf(cmd, "echo a %d %d %d %d %d %d %s > /proc/net/nf_nat_pt ", 
                trigger_proto, newObj->triggerPortStart, newObj->triggerPortEnd, open_proto, newObj->openPortStart, newObj->openPortEnd, ifName);
        rut_doSystemAction("rut", cmd);
    }
    else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
    {
        cmsLog_debug("PortTrigging delete: %s %s %s %d %d %d %d %s\n", 
                currObj->name, currObj->triggerProtocol, currObj->openProtocol, currObj->triggerPortStart, currObj->triggerPortEnd, currObj->openPortStart, currObj->openPortEnd, ifName);
        if ( !strcmp(currObj->triggerProtocol, "TCP") ) 
            trigger_proto = 1;
        else if ( !strcmp(currObj->triggerProtocol, "UDP") )
            trigger_proto = 2;
        else
            trigger_proto = 3;

        if ( !strcmp(currObj->openProtocol, "TCP") ) 
            open_proto = 1;
        else if ( !strcmp(currObj->openProtocol, "UDP") )
            open_proto = 2;
        else
            open_proto = 3;
        sprintf(cmd, "echo d %d %d %d %d %d %d %s > /proc/net/nf_nat_pt ", 
                trigger_proto, currObj->triggerPortStart, currObj->triggerPortEnd, open_proto, currObj->openPortStart, currObj->openPortEnd, ifName);
        rut_doSystemAction("rut", cmd);
    }   
    return ret;
}

CmsRet rcl_wanIpConnPortTriggeringObject( _WanIpConnPortTriggeringObject *newObj __attribute__((unused)),
                const _WanIpConnPortTriggeringObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;
   
    WanIpConnObject *wan_ip_conn = NULL;
    InstanceIdStack parentIidStack = *iidStack;
    char ifName[BUFLEN_32], cmd[BUFLEN_128];
    int trigger_proto, open_proto;

    cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));
    if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
    {
        return ret;
    }

    if (cmsObj_getAncestor(MDMOID_WAN_IP_CONN, MDMOID_WAN_IP_CONN_PORT_TRIGGERING,  
                                                     &parentIidStack, (void **) &wan_ip_conn) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get WanIpConnObject. ret=%d", ret);
        return CMSRET_INTERNAL_ERROR;
    }

    strncpy(ifName, wan_ip_conn->X_BROADCOM_COM_IfName, sizeof(ifName));
    cmsObj_free((void **) &wan_ip_conn);    

    rutIpt_insertPortTriggeringModules();

    if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
    {
        cmsLog_debug("PortTrigging add : %s %s %s %d %d %d %d %s\n", 
                newObj->name, newObj->triggerProtocol, newObj->openProtocol, newObj->triggerPortStart, newObj->triggerPortEnd, newObj->openPortStart, newObj->openPortEnd, ifName);

        if ( !strcmp(newObj->triggerProtocol, "TCP") ) 
            trigger_proto = 1;
        else if ( !strcmp(newObj->triggerProtocol, "UDP") )
            trigger_proto = 2;
        else
            trigger_proto = 3;

        if ( !strcmp(newObj->openProtocol, "TCP") ) 
            open_proto = 1;
        else if ( !strcmp(newObj->openProtocol, "UDP") )
            open_proto = 2;
        else
            open_proto = 3;

        sprintf(cmd, "echo a %d %d %d %d %d %d %s > /proc/net/nf_nat_pt ", 
                trigger_proto, newObj->triggerPortStart, newObj->triggerPortEnd, open_proto, newObj->openPortStart, newObj->openPortEnd, ifName);
        rut_doSystemAction("rut", cmd);
    }
    else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
    {
        cmsLog_debug("PortTrigging delete: %s %s %s %d %d %d %d %s\n", 
                currObj->name, currObj->triggerProtocol, currObj->openProtocol, currObj->triggerPortStart, currObj->triggerPortEnd, currObj->openPortStart, currObj->openPortEnd, ifName);
        if ( !strcmp(currObj->triggerProtocol, "TCP") ) 
            trigger_proto = 1;
        else if ( !strcmp(currObj->triggerProtocol, "UDP") )
            trigger_proto = 2;
        else
            trigger_proto = 3;

        if ( !strcmp(currObj->openProtocol, "TCP") ) 
            open_proto = 1;
        else if ( !strcmp(currObj->openProtocol, "UDP") )
            open_proto = 2;
        else
            open_proto = 3;
        sprintf(cmd, "echo d %d %d %d %d %d %d %s > /proc/net/nf_nat_pt ", 
                trigger_proto, currObj->triggerPortStart, currObj->triggerPortEnd, open_proto, currObj->openPortStart, currObj->openPortEnd, ifName);
        rut_doSystemAction("rut", cmd);
    }   
    return ret;
}

