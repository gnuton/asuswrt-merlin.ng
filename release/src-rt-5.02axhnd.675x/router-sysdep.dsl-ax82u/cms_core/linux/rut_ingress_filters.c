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

#ifdef DMP_X_BROADCOM_COM_RDPA_1
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

/* ---- Include Files ----------------------------------------------------- */

#include "cms_core.h"
#include "cms_util.h"
#include "rut_util.h"
#include "rut_ingress_filters.h"

#define MAX_UDEF_INSTANCES 4

const char * ingressFiltersTypeStr[] = {
        MDMVS_FILTER_DHCP,
        MDMVS_FILTER_IGMP,
        MDMVS_FILTER_MLD,
        MDMVS_FILTER_ICMPV6,
        MDMVS_FILTER_ETYPE_UDEF,
        MDMVS_FILTER_ETYPE_UDEF,
        MDMVS_FILTER_ETYPE_UDEF,
        MDMVS_FILTER_ETYPE_UDEF,
        MDMVS_FILTER_PPPOE_D,
        MDMVS_FILTER_PPPOE_S,
        MDMVS_FILTER_ARP,
        MDMVS_FILTER_802_1X,
        MDMVS_FILTER_802_1AG_CFM,
        MDMVS_FILTER_PTP_1588,
        MDMVS_FILTER_L4_PTP_1588,
        "DUMMY",
        "DUMMY",
        "DUMMY",
        MDMVS_FILTER_MCAST,
        MDMVS_FILTER_BCAST,
        MDMVS_FILTER_MAC_ADDR_OUI,
        MDMVS_FILTER_HDR_ERR,
        MDMVS_FILTER_IP_FRAG,
        MDMVS_FILTER_TPID
};

const char * ingressFiltersActionStr[] = {
        MDMVS_FILTER_NONE,
        MDMVS_FILTER_FORWARD,
        MDMVS_FILTER_CPU,
        "DUMMY",
        MDMVS_FILTER_DROP
};

/* Represent the rdpa filter types enumaration (etype_udef will be count 4 times ETYPE_UDEF0 - ETYPE_UDEF3) */
static int filterTypeTranslation(const char *mdmStr, UINT32 udefCount )
{
    UINT32 count;

    for (count = 0; count < sizeof(ingressFiltersTypeStr); count++)
    {
        if (!cmsUtl_strcmp(ingressFiltersTypeStr[count], mdmStr))
        {
            /* special case in etype udef configuration */
            if (!cmsUtl_strcmp(mdmStr, MDMVS_FILTER_ETYPE_UDEF))
            {
                if (udefCount >= MAX_UDEF_INSTANCES)
                {
                    cmsLog_error("To many UDEF instances");
                    return (sizeof(ingressFiltersTypeStr)+1);
                }

                /* Add the udef count to the index of udef filter (get ) */
                return count+udefCount;
            }
            return count;
        }
    }
    return sizeof(ingressFiltersTypeStr);
};

static int filterActionTranslation(const char *mdmStr )
{
    UINT32 count;

    for (count = 0; count < sizeof(ingressFiltersActionStr); count++)
    {
        if (!cmsUtl_strcmp(ingressFiltersActionStr[count], mdmStr))
        {
                return count;
        }
    }
    return -1;
};




CmsRet rut_ingressFiltersSetLocalSw(UBOOL8 enable)
{
    CmsRet ret = CMSRET_SUCCESS;
#if !defined(G9991)
    rdpa_filter_global_cfg_t lSwState;
    int rc;

    lSwState.ls_enabled = (bdmf_boolean)enable;
    rc = rdpaCtl_filter_set_global_cfg(&lSwState);
    if (rc)
    {
        cmsLog_error("Local switching configuration error");
        ret = CMSRET_INTERNAL_ERROR;
    }
#endif
    return ret;
}

CmsRet rutIngressFilters_addFilterEntry(const char *filterType, const char *action, UINT32 val,
    UINT32 valId, UINT64 ports, UBOOL8 direction, UBOOL8 delete, UINT32 udefEntryInx)
{
    CmsRet ret = CMSRET_SUCCESS;
    int rc;
    rdpa_filter_key_t entry_key;
    rdpa_filter_ctrl_t entry_ctrl;
    rdpa_filter_tpid_vals_t tpidVals;
    rdpa_filter_oui_val_key_t oui_entry;

    entry_key.filter = filterTypeTranslation(filterType, udefEntryInx);

    /* Check if the filter type is invalid - wrong type string or to many udef instances */
    if (entry_key.filter == sizeof(ingressFiltersTypeStr) || entry_key.filter == (sizeof(ingressFiltersTypeStr)+1))
    {
        ret = CMSRET_INTERNAL_ERROR;
        goto exit;
    }

    oui_entry.val_id = valId;
    oui_entry.ports = (rdpa_ports)ports;
    entry_key.ports = (rdpa_ports)ports;
    entry_ctrl.enabled = !delete;
    entry_ctrl.action = (rdpa_forward_action)filterActionTranslation(action);

    if (entry_key.filter == RDPA_FILTER_TPID)
    {
        if (direction)
        {
            tpidVals.val_us = (uint16_t) val;
            tpidVals.val_ds = 0;
        }
        else
        {
            tpidVals.val_us = 0;
            tpidVals.val_ds = (uint16_t) val;
        }

        rc = rdpaCtl_filter_tpid_vals_cfg(&tpidVals, direction);
        if (rc)
        {
            cmsLog_error("TPID vals configuration error");
            ret = CMSRET_INTERNAL_ERROR;
            goto exit;
        }
    }


    if (entry_key.filter >= RDPA_FILTER_ETYPE_UDEF_0 && entry_key.filter <= RDPA_FILTER_ETYPE_UDEF_3)
    {
        rc = rdpaCtl_filter_etyp_udef_cfg(udefEntryInx, val);
        if (rc)
        {
            cmsLog_error("TPID vals configuration error");
            ret = CMSRET_INTERNAL_ERROR;
            goto exit;
        }
    }


    if (entry_key.filter == RDPA_FILTER_MAC_ADDR_OUI)
    {
        rc = rdpaCtl_filter_oui_cfg(&oui_entry, val);
        if (rc)
        {
            cmsLog_error("OUI filter entry configuration error");
            ret = CMSRET_INTERNAL_ERROR;
        }
	}

	rc = rdpaCtl_filter_entry_create(&entry_key, &entry_ctrl);
	if (rc)
	{
		cmsLog_error("Filter entry configuration error");
		ret = CMSRET_INTERNAL_ERROR;
	}

exit:
	return ret;
}

CmsRet rutIngressFilters_activateAllFilters()
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   IngressFiltersDataObject *filterObj=NULL;
   CmsRet ret, ret2;
   
   cmsLog_debug("Entered...");

   while ((ret = cmsObj_getNextFlags(MDMOID_INGRESS_FILTERS_DATA, &iidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **) &filterObj)) == CMSRET_SUCCESS)
   {
      cmsLog_debug("Activating Filter at %s", cmsMdm_dumpIidStack(&iidStack));
      ret2 = cmsObj_set(filterObj, &iidStack);
      cmsObj_free((void **) &filterObj);
      if (ret2 != CMSRET_SUCCESS)
      {
         cmsLog_error("Got error from set of instance %s, ret2=%d",
                       cmsMdm_dumpIidStack(&iidStack), ret2);
         return ret2;
      }
   }

   // If we get here, then we have successfully iterated through all the objs.
   return CMSRET_SUCCESS;
}

#endif  /* DMP_X_BROADCOM_COM_RDPA_1 */
