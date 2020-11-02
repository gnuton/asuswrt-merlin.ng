/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_ingress_filters.h"

#ifdef DMP_X_BROADCOM_COM_RDPA_1

#define STRNCMP(src, dst) cmsUtl_strncmp(src, dst, strlen(dst))
#define MAX_UDEF_ENTRIES_NUM 4
UINT64 USportMask = 0xFFFFFFFFFFFFFFFCLL;
UINT64 DSportMask = 0x3;

static int valueFiltersEnumeration(char *filterType)
{
    if (!cmsUtl_strcmp(filterType, MDMVS_FILTER_ETYPE_UDEF))
        return 1;
    else if (!cmsUtl_strcmp(filterType, MDMVS_FILTER_MAC_ADDR_OUI))
        return 2;
    else if (!cmsUtl_strcmp(filterType, MDMVS_FILTER_TPID))
        return 3;
    else
        return 0;
}

CmsRet rcl_filtersCfgObject( _FiltersCfgObject *newObj,
                const _FiltersCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;

    cmsLog_debug("Entered...");
#ifdef BCM_PON
    if (ADD_NEW(newObj, currObj))
    {
      // There is always exactly 1 instance of this object in the system, so
      // "ADD_NEW" means system startup.  By default, ready is TRUE.  But PON
      // hardware is not ready until ssk receives the POST_ACTIVATING msg.
      newObj->ready = FALSE;
      return CMSRET_SUCCESS;
    }
#endif

    if (ADD_NEW(newObj, currObj) || SET_EXISTING(newObj, currObj))
    {
        /* If create new filter obj or modify the local SW parameter */
        ret = rut_ingressFiltersSetLocalSw(newObj->localSwitching);
        if (ret)
            return CMSRET_MDM_TREE_ERROR;
            
        if ((SET_EXISTING(newObj, currObj)) &&
            (newObj->ready == TRUE) && (currObj->ready == FALSE))
        {
           // When ready transitions from FALSE to TRUE, we can activate all
           // our child filters.  Part of the Post Activation feature for PON. 
           ret = rutIngressFilters_activateAllFilters();
        }
    }
    return ret;
} 

static CmsRet valueFiltersHandler(_IngressFiltersDataObject *newObj,
                                  const InstanceIdStack *iidStack,
                                  UINT32 *udefIndexOut,
                                  UINT64 *portsOut,
                                  UBOOL8 *direction)
{
    CmsRet ret = CMSRET_SUCCESS;
    _IngressFiltersDataObject *otherFilterData = NULL;
    InstanceIdStack iidStackSub = EMPTY_INSTANCE_ID_STACK;
    UINT32 numOfSameEntries = 0;
    UINT32 myEntryCount = 0;
    UINT64 newPorts = 0, currPorts = 0;
    UINT64 otherPorts;
    UINT32 ouiValId = 0;

    /*look into substree, count the number of entries of this filter*/
    while (((ret = cmsObj_getNext(MDMOID_INGRESS_FILTERS_DATA,
            &iidStackSub,
            (void **)&otherFilterData)) == CMSRET_SUCCESS))
    {
        /* Skip if this is the same object */
        if (iidStackSub.instance[0] == iidStack->instance[0])
        {
            myEntryCount = numOfSameEntries;
            numOfSameEntries++;             
            cmsObj_free((void **)&otherFilterData);
            continue;
        }

        if (!cmsUtl_strcmp(otherFilterData->type, newObj->type))
        {

			cmsUtl_strtoul64(otherFilterData->ports, NULL, 2, &currPorts);
			cmsUtl_strtoul64(newObj->ports, NULL, 2, &newPorts);

            switch (valueFiltersEnumeration(newObj->type))
            {
            case 1:
				/* Udef case */
				/* Can not add another entry with same value, the same action and commonal ports */
				if (otherFilterData->val == newObj->val)
				{
					if (STRNCMP(otherFilterData->action, newObj->action) && (currPorts & newPorts))
					{
						cmsLog_error("Wrong udef filter configuration same value for different actions on same ports!!! ");
						ret = CMSRET_MDM_TREE_ERROR;
						goto exit;
					}
				}
				else
					numOfSameEntries++;
				break;
			case 2:
                /* Oui case */
                /* If oui is already configured use the same valId */
                if (otherFilterData->val == newObj->val)
                {
                    newObj->valId = otherFilterData->valId;
                    goto exit;
                }
                ouiValId++;
                break;
            case 3:
                /* Tpid case */
                /* Check if both DS/US entry and the values are different exit with error */
                cmsUtl_strtoul64(otherFilterData->ports, NULL, 2, &otherPorts);
                if (((DSportMask&(*portsOut)) && (DSportMask&otherPorts)) ||
                        ((USportMask&(*portsOut)) && (USportMask&otherPorts)))
                {
                    if (otherFilterData->val == newObj->val)
                    {
                        cmsLog_error("Same direction different tpid value");
                        ret = CMSRET_MDM_TREE_ERROR;
                        goto exit;
                    }
                }
                break;
            default:
                cmsLog_error("Invalid filter type");
                ret = CMSRET_INVALID_ARGUMENTS;
                goto exit;
            }
        }
        cmsObj_free((void **)&otherFilterData);
    }

    /* Initial ret parameter after while loop */
    ret = CMSRET_SUCCESS;

    switch (valueFiltersEnumeration(newObj->type))
    {
    case 1:
        /* Udef case */
        /* Check if arrived to max udef entries number (4) */
        if (myEntryCount > (MAX_UDEF_ENTRIES_NUM-1))
        {
            cmsLog_error("Max udef entries number!");
            ret = CMSRET_MDM_TREE_ERROR;
            goto exit;
        }
		newObj->valId = myEntryCount;
        *udefIndexOut = myEntryCount;
        break;
    case 2:
        /* Oui case */
        newObj->valId = ouiValId;
        break;
    case 3:
        /* Tpid case */
        if (USportMask&(*portsOut))
            *direction = 1;
        break;
    default:
        break;
    }

exit:
    if (otherFilterData!=NULL)
        cmsObj_free((void **)&otherFilterData);
    return ret;
}

static UBOOL8 isFiltersCfgReady()
{
   FiltersCfgObject *filtersCfgObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   UBOOL8 ready=FALSE;
   CmsRet ret;

   if ((ret = cmsObj_get(MDMOID_FILTERS_CFG, &iidStack, OGF_NO_VALUE_UPDATE,
                         (void **)&filtersCfgObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get filtersCfg obj, ret=%d", ret);
      return FALSE;
   }
   ready = filtersCfgObj->ready;
   cmsObj_free((void **) &filtersCfgObj);

   return ready;
}

CmsRet rcl_ingressFiltersDataObject( _IngressFiltersDataObject *newObj,
		const _IngressFiltersDataObject *currObj,
		const InstanceIdStack *iidStack,
		char **errorParam __attribute__((unused)),
		CmsRet *errorCode __attribute__((unused)))
{
	CmsRet ret = CMSRET_SUCCESS;
	UINT32 udefIndex = 0;
	UINT64 ports = 0;
	UBOOL8 direction = 0;
	UBOOL8 deleation = 0;
	_IngressFiltersDataObject confObj;

   cmsLog_debug("Entered...");

   if (ADD_NEW(newObj, currObj))
   {
      // ADD_NEW means either system startup or new instance has been added.
      // If hardware is not ready, return for now.  This object will be
      // activated later when ssk receives the POST_ACTIVATING msg.
      if (!isFiltersCfgReady())
      {
         cmsLog_debug("skipping activation of instance %s, not ready",
                      cmsMdm_dumpIidStack(iidStack));
         return CMSRET_SUCCESS;
      }
   }

	if (ADD_NEW(newObj, currObj) || SET_EXISTING(newObj, currObj))
	{
		/* addInstance will create newObj with NULL values */
		if (!newObj->ports)
			goto exit;

		memcpy(&confObj, newObj, sizeof(_IngressFiltersDataObject));
		cmsUtl_strtoul64(confObj.ports, NULL, 2, &ports);

		/* Entry classification - regular, oui, tpid etc... */
		if (newObj->val != 0 && valueFiltersEnumeration(newObj->type))
		{
			/* Handler for valued filters */
			ret = valueFiltersHandler(newObj,
					iidStack,
					&udefIndex,
					&ports,
					&direction);
			if (ret)
				goto exit;
			confObj.valId = newObj->valId;
		}
		else
		{
			confObj.val = 0;
		}
	}
	else if (DELETE_EXISTING(newObj, currObj))
	{
		deleation = 1;
		memcpy(&confObj, currObj, sizeof(_IngressFiltersDataObject));
		cmsUtl_strtoul64(confObj.ports, NULL, 2, &ports);
	}
	else
		goto exit;

	if (SET_EXISTING(newObj, currObj) && currObj->ports != NULL && !deleation)
	{
		UINT64 prev_ports;
		
		cmsUtl_strtoul64(currObj->ports, NULL, 2, &prev_ports);
		if (ports != prev_ports)
		{
			prev_ports = prev_ports & ( ~ ports); 
			ret = rutIngressFilters_addFilterEntry(confObj.type, confObj.action, confObj.val,
					confObj.valId, prev_ports, direction, 1, udefIndex);
		}
	}

	/* Entry configuration */
	ret = rutIngressFilters_addFilterEntry(confObj.type, confObj.action, confObj.val,
			confObj.valId, ports, direction, deleation, udefIndex);
exit:
	return ret;
}

#endif  /* DMP_X_BROADCOM_COM_RDPA_1 */
