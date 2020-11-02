/***********************************************************************
 *
 *  Copyright (c) 2009  Broadcom Corporation
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

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_wanlayer2.h"
#include "rut_l2tpac.h"

#define BUF_SIZE 1024
#define L2tpActResult "/tmp/L2tpActResult"
#define L2tpRpcServer "/var/openl2tp_rpc_server_addr"

static UBOOL8 rutL2tp_checkActionResult(const char *string)
{
	FILE *fs;
	char buf[64] = {0};
	UBOOL8 found = FALSE;
	
	if ((fs = fopen(L2tpActResult, "r")) != NULL)
	{
		while(fgets(buf, sizeof(buf), fs) != NULL)
		{
			if(cmsUtl_strstr(buf, string) != NULL)
			{	 
			    found = TRUE;
				break;
			}
		}
	}
	 
	fclose(fs);

	return found;
}

CmsRet rutL2tp_updateLinkSate(const InstanceIdStack *iidStack, UBOOL8 isUp)
{
	CmsRet ret = CMSRET_SUCCESS;
	InstanceIdStack connDevStack          = *iidStack;
	L2tpAcLinkConfigObject *L2tpAclinkCfg = NULL;   
	void *obj						      = NULL;
	
	/*find current object ancestor*/
	if (cmsObj_getAncestor(MDMOID_WAN_CONN_DEVICE, MDMOID_WAN_PPP_CONN,
						  &connDevStack, (void **) &obj) != CMSRET_SUCCESS)
	{
		cmsLog_error("Current L2TP LINK CONFIG object have no ancestor.");
		return CMSRET_MDM_TREE_ERROR;
	}
	cmsObj_free((void **) &obj);

	if ((ret = cmsObj_get(MDMOID_L2TP_AC_LINK_CONFIG, &connDevStack, 0, (void **)&L2tpAclinkCfg)) == CMSRET_SUCCESS)
	{
        if (isUp)
	    {
			CMSMEM_REPLACE_STRING(L2tpAclinkCfg->linkStatus, "up");
        }
		else
	    {
			CMSMEM_REPLACE_STRING(L2tpAclinkCfg->linkStatus, "down");
		}
		
		if ((ret = cmsObj_set(L2tpAclinkCfg, &connDevStack)) != CMSRET_SUCCESS)
		{
			cmsLog_error("set of L2tpAclinkCfg failed");
		}

		cmsLog_debug("cmsObj_set L2tpAc linkStatus ok.");
		cmsObj_free((void **) &L2tpAclinkCfg);
	}

	return CMSRET_SUCCESS;
}


void rutL2tp_refreshL2tp()
{
	CmsRet ret = CMSRET_SUCCESS;
	InstanceIdStack iidStack2          = EMPTY_INSTANCE_ID_STACK;
	L2tpAcIntfConfigObject *L2tpAcIntf = NULL;
	L2tpAcLinkConfigObject *L2tpAclinkCfg = NULL; 
	InstanceIdStack	iidStack = EMPTY_INSTANCE_ID_STACK;
    void *obj = NULL;
	
	if (rutWl2_getL2tpWanIidStack(&iidStack) == CMSRET_SUCCESS)
	{
	   while(cmsObj_getNextInSubTree(MDMOID_L2TP_AC_INTF_CONFIG, &iidStack, &iidStack2, (void **)&L2tpAcIntf) == CMSRET_SUCCESS)
	   {
			cmsLog_debug("refresh L2tpAcIntf \n");
			if ((ret = cmsObj_set(L2tpAcIntf, &iidStack2)) != CMSRET_SUCCESS)
			{
				cmsLog_error("refresh L2tpAcIntf error.");
			}
			
			cmsObj_free((void **) &L2tpAcIntf);
	   }

		INIT_INSTANCE_ID_STACK(&iidStack2);
		while (cmsObj_getNextInSubTree(MDMOID_WAN_CONN_DEVICE, &iidStack, &iidStack2, (void **)&obj)== CMSRET_SUCCESS)
		{
			if ((ret = cmsObj_get(MDMOID_L2TP_AC_LINK_CONFIG, &iidStack2, OGF_NO_VALUE_UPDATE, (void **) &L2tpAclinkCfg)) == CMSRET_SUCCESS)
			{
			    cmsLog_debug("refresh L2tpAclinkCfg \n");
				if ((ret = cmsObj_set(L2tpAclinkCfg, &iidStack2)) != CMSRET_SUCCESS)
				{
					cmsLog_error("refresh L2tpAclinkCfg error.");
				}
				cmsObj_free((void **)&L2tpAclinkCfg);
			}
			cmsObj_free((void **)&obj); 
		}
   	}	   
}

UBOOL8 rutL2tp_checkL2tpdWithDelay(void)
{
	int i = 0;
	while (i++ < 15)
	{
	  /* check L2tpd socket is ready. */
      /* if not, wait 100 ms for next check. */
		if (access(L2tpRpcServer, F_OK) == 0)
		{
			return TRUE;
		}
		else
		{
			usleep(100000); // 100 ms
		}
	}
   /* waited 1.5 seconds and L2tpd socket is still not ready, give up and return false */
	return FALSE;
}

CmsRet rutL2tp_startL2tpd()
{   
	UINT32 pid = 0;
	CmsRet ret = CMSRET_SUCCESS;
    char l2tpcmd[BUFLEN_8];
	/* start l2tpac */

	cmsLog_debug("Entered");
    sprintf(l2tpcmd, "-f");
	/* send message to ssk to launch l2tpd */
	if ((pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_L2TPD, l2tpcmd, strlen(l2tpcmd)+1)) == CMS_INVALID_PID)
	{
	   cmsLog_error("failed to start or restart l2tpd");
	   ret = CMSRET_INTERNAL_ERROR;
	}
	
    ret = rutL2tp_checkL2tpdWithDelay();

	return ret;
}

CmsRet rutL2tp_stopL2tpd()
{   
	CmsRet ret = CMSRET_SUCCESS;
	/* stop l2tpac */

	cmsLog_debug("Entered");

	if (rut_isApplicationActive(EID_L2TPD) == TRUE)
	{
		/* send message to ssk to stop l2tpd */
		if ((ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_L2TPD, NULL, 0) != CMSRET_SUCCESS))
		{
		   cmsLog_error("failed to stop l2tpd");
		}
	} 
	
	return ret;
}

UBOOL8 rutL2tp_isTunnelExist(const char *tunnelName)
{   
	char l2tpcmd[BUFLEN_128];
	UBOOL8 isExist = FALSE;

	cmsLog_debug("Entered");

	snprintf(l2tpcmd, sizeof(l2tpcmd), "l2tpconfig tunnel show tunnel_name=%s > %s", tunnelName, L2tpActResult);
	
	rut_doSystemAction("rutL2tp", l2tpcmd);

    isExist = rutL2tp_checkActionResult("Tunnel");

	return isExist;
}

UBOOL8 rutL2tp_isTunnelActive(const char *tunnelName)
{   
	char l2tpcmd[BUFLEN_128];
	UBOOL8 isExist = FALSE;

	cmsLog_debug("Entered");

	snprintf(l2tpcmd, sizeof(l2tpcmd), "l2tpconfig tunnel show tunnel_name=%s > %s", tunnelName, L2tpActResult);
	
	rut_doSystemAction("rutL2tp", l2tpcmd);

    isExist = rutL2tp_checkActionResult("ESTABLISHED");

	return isExist;
}

UBOOL8 rutL2tp_isLinkExist(const char *tunnelName, const char *sessionName)
{   
	char l2tpcmd[BUFLEN_128];
	UBOOL8 isExist = FALSE;

	cmsLog_debug("Entered");

	snprintf(l2tpcmd, sizeof(l2tpcmd), "l2tpconfig session show tunnel_name=%s session_name=%s > %s",
		                                tunnelName ,sessionName, L2tpActResult);

	rut_doSystemAction("rutL2tp", l2tpcmd);

    isExist = rutL2tp_checkActionResult("Session");

	return isExist;
}


CmsRet rutL2tp_createTunnel(_L2tpAcIntfConfigObject *l2tpIntfObj __attribute__((unused)))
{   
	char l2tpcmd[BUFLEN_128];
	CmsRet ret = CMSRET_SUCCESS;

	cmsLog_debug("Entered");

	if (rutL2tp_isTunnelActive(l2tpIntfObj->tunnelName))
	{
		return CMSRET_SUCCESS;
	}

	rutL2tp_deleteTunnel(l2tpIntfObj);

	snprintf(l2tpcmd, sizeof(l2tpcmd), "l2tpconfig tunnel create dest_ipaddr=%s our_udp_port=1701 tunnel_name=%s > %s",
		                               l2tpIntfObj->lnsIpAddress, l2tpIntfObj->tunnelName, L2tpActResult);
								   
	rut_doSystemAction("rutL2tp", l2tpcmd);

    if (rutL2tp_checkActionResult("Error"))
	{
	    cmsLog_error("tunnel create error");
		ret = CMSRET_INTERNAL_ERROR;
    }
	
	return ret;
}


CmsRet rutL2tp_deleteTunnel(const _L2tpAcIntfConfigObject *l2tpIntfObj __attribute__((unused)))
{   
    char l2tpcmd[BUFLEN_128];

    cmsLog_debug("Entered");

    if (rutL2tp_isTunnelExist(l2tpIntfObj->tunnelName))
    {
		snprintf(l2tpcmd, sizeof(l2tpcmd), "l2tpconfig tunnel delete tunnel_name=%s", l2tpIntfObj->tunnelName);	
		rut_doSystemAction("rutL2tp", l2tpcmd);
	}

	return CMSRET_SUCCESS;
}

CmsRet rutL2tp_createLink(_L2tpAcLinkConfigObject *l2tpLinkObj __attribute__((unused)), _WanPppConnObject *pppObj __attribute__((unused)))
{   
    char l2tpcmd[BUFLEN_256];
	CmsRet ret = CMSRET_SUCCESS;
	
    cmsLog_debug("Entered");

    if (!rutL2tp_isTunnelExist(l2tpLinkObj->tunnelName))
    {
        cmsLog_error("no tunnel");
		return CMSRET_INTERNAL_ERROR;
	}

	if (rutL2tp_isLinkExist(l2tpLinkObj->tunnelName, l2tpLinkObj->sessionName))
	{
		if (!cmsUtl_strcmp(pppObj->connectionStatus, MDMVS_CONNECTED) ||
			!cmsUtl_strcmp(pppObj->connectionStatus, MDMVS_CONNECTING))
		{
		   return CMSRET_SUCCESS;
		}
		else if (!cmsUtl_strcmp(pppObj->connectionStatus, MDMVS_DISCONNECTED))
		{
			rutL2tp_deleteLink(l2tpLinkObj);
		}
	}

	snprintf(l2tpcmd, sizeof(l2tpcmd), "l2tpconfig session create user_name=%s user_password=%s tunnel_name=%s session_name=%s interface_name=%s > %s",
		                                  pppObj->username, pppObj->password, l2tpLinkObj->tunnelName, l2tpLinkObj->sessionName, l2tpLinkObj->ifName, L2tpActResult);

	rut_doSystemAction("rutL2tp", l2tpcmd);

    if (rutL2tp_checkActionResult("Error"))
	{
	    cmsLog_error("Link create error");
		ret = CMSRET_INTERNAL_ERROR;
    }

	return ret;
}

CmsRet rutL2tp_deleteLink(const _L2tpAcLinkConfigObject *l2tpLinkObj __attribute__((unused)))
{   
    char l2tpcmd[BUFLEN_256];

    cmsLog_debug("Entered");

	if (rut_isApplicationActive(EID_L2TPD) == TRUE)
	{
		if (rutL2tp_isLinkExist(l2tpLinkObj->tunnelName, l2tpLinkObj->sessionName))
		{
			snprintf(l2tpcmd, sizeof(l2tpcmd), "l2tpconfig session delete tunnel_name=%s session_name=%s", l2tpLinkObj->tunnelName, l2tpLinkObj->sessionName);
			rut_doSystemAction("rutL2tp", l2tpcmd);
		}
	}

    return CMSRET_SUCCESS;
}
#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */
