/***********************************************************************
 *
 *  Copyright (c) 2006-2009  Broadcom Corporation
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
#include "rut_wan.h"
#include "rut_wanlayer2.h"
#include "rut_l2tpac.h"

/*!\file rcl_l2tpac.c
 * \brief This file contains L2TPAC related functions.
 *
 */

CmsRet rcl_l2tpAcIntfConfigObject(_L2tpAcIntfConfigObject *newObj,
                const _L2tpAcIntfConfigObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))

{
    CmsRet ret=CMSRET_SUCCESS;
	InstanceIdStack L2tpAciidStack = EMPTY_INSTANCE_ID_STACK;
	L2tpAcClientCfgObject *L2tpAc = NULL;	
	
	if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
	{
		cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
		return ret;
	}
	
	if ((ret = cmsObj_get(MDMOID_L2TP_AC_CLIENT_CFG, &L2tpAciidStack, 0, (void **) &L2tpAc)) != CMSRET_SUCCESS)
	{
		cmsLog_error("Failed to get L2tpAcClientCfgObject, ret=%d", ret);
		return ret;
	}

	if (ADD_NEW(newObj,currObj))
    {		
		L2tpAc->numberOfClient += 1;
		L2tpAc->enable = TRUE;

		ret = cmsObj_set(L2tpAc, &L2tpAciidStack);
		if (ret != CMSRET_SUCCESS)
		{
			cmsLog_error("Failed to set L2tpAcClientCfgObject, ret = %d", ret);
			cmsObj_free((void **) &L2tpAc);
			return ret;
		}
		
        if (L2tpAc->numberOfClient == 1)
        {
			rutL2tp_startL2tpd();
		}
	}

	if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj) ||
		POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
	{	  
		ret = rutL2tp_createTunnel(newObj);
		cmsLog_debug("L2tp create Tunnel ret(%d)", ret);
	}
	else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
	{
		ret = rutL2tp_deleteTunnel(currObj);
		cmsLog_debug("L2tp delete Tunnel ret(%d)",ret);

		L2tpAc->numberOfClient -= 1;
		
		ret = cmsObj_set(L2tpAc, &L2tpAciidStack);
		if (ret != CMSRET_SUCCESS)
		{
			cmsLog_error("Failed to set L2tpAcClientCfgObject, ret = %d", ret);
			cmsObj_free((void **) &L2tpAc);
			return ret;
		}

		if (L2tpAc->numberOfClient == 0)
		{
			L2tpAc->enable = FALSE;
			rutL2tp_stopL2tpd();
		}
	}

	cmsObj_free((void **) &L2tpAc);
	
    return CMSRET_SUCCESS;
}

   
CmsRet rcl_l2tpAcLinkConfigObject(_L2tpAcLinkConfigObject *newObj,
                const _L2tpAcLinkConfigObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
	CmsRet ret                       = CMSRET_SUCCESS;
	InstanceIdStack iidStackAncestor = EMPTY_INSTANCE_ID_STACK;
	InstanceIdStack iidStackSub      = EMPTY_INSTANCE_ID_STACK;
	WanPppConnObject *pppCon         = NULL;
	void *obj                        = NULL;

	if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
	{
		cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
		return ret;
	}

	if (rut_isApplicationActive(EID_L2TPD) == FALSE)
	{
		cmsLog_debug("l2tpd is not ready");
		return ret;
	}

	if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj) ||
		POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
	{
		iidStackAncestor = *iidStack;

		/*find current object ancestor*/
		if (cmsObj_getAncestor(MDMOID_WAN_CONN_DEVICE, MDMOID_L2TP_AC_LINK_CONFIG,
							  &iidStackAncestor, (void **) &obj) != CMSRET_SUCCESS)
		{
			cmsLog_error("Current L2TP LINK CONFIG object have no ancestor.");
			return CMSRET_MDM_TREE_ERROR;
		}
		cmsObj_free((void **) &obj);

		INIT_INSTANCE_ID_STACK(&iidStackSub);
		while (((ret = cmsObj_getNextInSubTree(MDMOID_WAN_PPP_CONN, &iidStackAncestor, 
                                    &iidStackSub, (void **)&pppCon)) == CMSRET_SUCCESS))
		{
			if (!IS_EMPTY_STRING(newObj->tunnelName) && !IS_EMPTY_STRING(newObj->sessionName) && !IS_EMPTY_STRING(newObj->ifName)) 
			{
				ret = rutL2tp_createLink(newObj, pppCon);
	 			cmsLog_debug("create session ret(%d) username=%s,password=%s,tunnelName=%s,sessionName=%s,ifName=%s",
				        ret, pppCon->username, pppCon->password, newObj->tunnelName, newObj->sessionName, newObj->ifName);
			}
		}
	}
	else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
	{
		if (!IS_EMPTY_STRING(currObj->tunnelName) && !IS_EMPTY_STRING(currObj->sessionName))
	    {
			ret = rutL2tp_deleteLink(currObj);
	   		cmsLog_debug("delete session ret(%d) tunnelName=%s,sessionName=%s,ifName=%s", ret, currObj->tunnelName, currObj->sessionName, currObj->ifName);
		}
	}
   
	return CMSRET_SUCCESS;
}

CmsRet rcl_l2tpAcClientCfgObject(_L2tpAcClientCfgObject *newObj __attribute__((unused)),
                const _L2tpAcClientCfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{   
	return CMSRET_SUCCESS;
}
#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */
