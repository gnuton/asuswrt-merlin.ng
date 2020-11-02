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


#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"
#include "cms_net.h"
#include "cms_qdm.h"

#if defined(DMP_X_BROADCOM_COM_RDPA_1)
const char * defaultWanFilters[] = {
    MDMVS_FILTER_ARP,
    MDMVS_FILTER_BCAST,
    MDMVS_FILTER_IP_FRAG,
    MDMVS_FILTER_HDR_ERR
};
const char * gbeDefaultExtraFilters[] = {
    MDMVS_FILTER_DHCP,    
    MDMVS_FILTER_PPPOE_D,
#ifndef BCM_PON_XRDP
    MDMVS_FILTER_PPPOE_S
#endif
};

static int checkEntry(const char *type, UINT32 val, int *index)
{
    CmsRet ret=CMSRET_SUCCESS;
    MdmPathDescriptor filterDataPathDesc;
    _IngressFiltersDataObject *filterData = NULL;
    INIT_PATH_DESCRIPTOR(&filterDataPathDesc);
    PUSH_INSTANCE_ID(&(filterDataPathDesc.iidStack),1); // for filter instance.1
    filterDataPathDesc.oid = MDMOID_INGRESS_FILTERS_DATA;

    /* Check if filters alredy configured or the first filter equal to type */
    if ((ret = mdm_getObject(filterDataPathDesc.oid, &(filterDataPathDesc.iidStack), (void **) &filterData)) != CMSRET_SUCCESS)
        return 0;
       
    if ((!cmsUtl_strcmp(filterData->type, type) && val == 0) ||
        (!cmsUtl_strcmp(filterData->type, type) && filterData->val == val))
    {
        *index = 1;
        mdm_freeObject((void **) &filterData);
        return 1;
    }
    mdm_freeObject((void **) &filterData);

    *index = *index+1;
    /* Search the filter type */
    while ((ret = mdm_getNextObject(filterDataPathDesc.oid, &(filterDataPathDesc.iidStack), (void **) &filterData)) == CMSRET_SUCCESS)
    {
        if((!cmsUtl_strcmp(filterData->type, type) && val == 0) ||
           (!cmsUtl_strcmp(filterData->type, type) && filterData->val == val))
        {
            mdm_freeObject((void **) &filterData);
            return 1;
        }
        *index = *index+1;
        mdm_freeObject((void **) &filterData);
    }
        
    return 0;
}

static CmsRet wanFilterEntryConfig(const char *type, UINT32 val)
{
    CmsRet ret = CMSRET_SUCCESS;
    int index=1;
    int i=0;
    MdmPathDescriptor filterDataPathDesc;
    _IngressFiltersDataObject *filterData = NULL;
    char strPortMask[BUFLEN_64+1]={};
    memset(strPortMask, '0', BUFLEN_64 + 1);
    strPortMask[BUFLEN_64] = '\0';

    /* Get the valid index for this wan filter */
    i = checkEntry(type, val, &index);
    INIT_PATH_DESCRIPTOR(&filterDataPathDesc);
    filterDataPathDesc.oid = MDMOID_INGRESS_FILTERS_DATA;
    PUSH_INSTANCE_ID(&(filterDataPathDesc.iidStack),index); 
    if (i)
    {
        /* Set exist */
        mdm_getObject(filterDataPathDesc.oid, &(filterDataPathDesc.iidStack), (void **) &filterData);
        cmsUtl_strncpy(strPortMask, filterData->ports, sizeof(strPortMask));
        /* set the last char in the arry to 1 means wan0 port will be marked in the port bit mask */
        strPortMask[BUFLEN_64-1] = '1';
        CMSMEM_REPLACE_STRING_FLAGS(filterData->ports, strPortMask, mdmLibCtx.allocFlags);
        filterData->val = val;

        if ((ret = mdm_setObject((void **) &filterData, &(filterDataPathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
        {
            mdm_freeObject((void **) &filterData);
            cmsLog_error("Could not set filter data object, ret=%d", ret);
        }
    }
    else
    {
        /* Set new */
        strPortMask[BUFLEN_64-1] = '1';
        mdm_addObjectInstance(&filterDataPathDesc, NULL, NULL);
        mdm_getObject(filterDataPathDesc.oid, &(filterDataPathDesc.iidStack), (void **) &filterData);
        /* Update the instance port map */
        CMSMEM_REPLACE_STRING_FLAGS(filterData->ports, strPortMask, mdmLibCtx.allocFlags);
        CMSMEM_REPLACE_STRING_FLAGS(filterData->type, type, mdmLibCtx.allocFlags);
        CMSMEM_REPLACE_STRING_FLAGS(filterData->action, MDMVS_FILTER_CPU, mdmLibCtx.allocFlags);
        filterData->val = val;
        if ((ret = mdm_setObject((void **) &filterData, &(filterDataPathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
        {
            mdm_freeObject((void **) &filterData);
            cmsLog_error("Could not set filter data object, ret=%d", ret);
        }
    }
    return ret;
}

CmsRet mdm_addDefaultWanFilterObjects(void)
{
    CmsRet ret = CMSRET_SUCCESS;
    SINT32 count;
    UINT32 i;
    char buf[BUFLEN_16]={0};

    count = cmsPsp_get(RDPA_WAN_TYPE_PSP_KEY, buf, sizeof(buf));    
    if (count == 0)
    {
        /* key not found or error, this is not a RDPA system, do nothing. */
#ifndef DESKTOP_LINUX
        cmsLog_error("Could not get WAN type from scratch pad");
        return CMSRET_INTERNAL_ERROR;
#else
        cmsLog_notice("Could not get WAN type from scratch pad, do nothing");
        return CMSRET_SUCCESS;
#endif /* DESKTOP_LINUX */
    }

    /* Configure common wan filters*/
    for (i = 0; i < sizeof(defaultWanFilters)/sizeof(char *); i++)
    {
        ret = wanFilterEntryConfig(defaultWanFilters[i],0);
        if (ret)
        {
            cmsLog_error("Could not set defaultWanFilters, ret=%d", ret);
            goto exit;
        }
    }

    /* GBE case */
    if ((!cmsUtl_strncasecmp((const char *)buf, "AE",strlen("AE"))) || (!cmsUtl_strncmp((const char *)buf,
        "GBE",strlen("GBE"))))
    {
        /* Configure the extra GBE filters*/
        for (i = 0; i < sizeof(gbeDefaultExtraFilters)/sizeof(char *); i++)
        {
            ret = wanFilterEntryConfig(gbeDefaultExtraFilters[i],0);
            if (ret)
            {
                cmsLog_error("Could not set gbeDefaultExtraFilters, ret=%d", ret);
                goto exit;
            }
        }
    }

exit:
    return ret;
}
#endif
