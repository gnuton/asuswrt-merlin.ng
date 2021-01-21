/***********************************************************************
 *
 *  Copyright (c) 2017 Broadcom
 *  All Rights Reserved
 *
 * <:label-BRCM:2017:proprietary:standard
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

#ifdef DMP_X_ITU_ORG_GPON_1

#include "cms_obj.h"
#include "cms_log.h"
#include "rut_omci.h"
#include "ethswctl_wrap.h"

static OmciEthPortType omciRut_getPortType(UINT8 port, UINT32 typesAll)
{
    OmciEthPortType portType = OMCI_ETH_PORT_TYPE_NONE;
    OmciEthPortType_t eth;

    eth.types.all = typesAll;

    switch (port)
    {
        case 0:
            portType = eth.types.ports.eth0;
            break;
        case 1:
            portType = eth.types.ports.eth1;
            break;
        case 2:
            portType = eth.types.ports.eth2;
            break;
        case 3:
            portType = eth.types.ports.eth3;
            break;
        case 4:
            portType = eth.types.ports.eth4;
            break;
        case 5:
            portType = eth.types.ports.eth5;
            break;
        case 6:
            portType = eth.types.ports.eth6;
            break;
        case 7:
            portType = eth.types.ports.eth7;
            break;
        default:
            cmsLog_notice("port=%d set to type ONT", port);
            portType = OMCI_ETH_PORT_TYPE_ONT;
            break;
    }

    return portType;
}

void rutOmci_printPorts(void)
{
    CmsRet ret = CMSRET_INVALID_ARGUMENTS;

    UINT32 port = 0;
    OmciEthPortType_t eth;
    OmciEthPortType portType;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
      (void*)&obj)) == CMSRET_SUCCESS)
    {
        eth.types.all = obj->ethernetTypes;
        for (port = 0; port < obj->numberOfEthernetPorts; port++)
        {
            portType = omciRut_getPortType(port, eth.types.all);
            printf("   Ethernet %d is in ", port);
            switch (portType)
            {
                case OMCI_ETH_PORT_TYPE_RG:
                    printf("RG mode\n");
                    break;
                case OMCI_ETH_PORT_TYPE_ONT:
                    printf("ONT mode\n");
                    break;
                case OMCI_ETH_PORT_TYPE_RG_ONT:
                    printf("RG_ONT mode\n");
                    break;
                default:
                    printf("unknown\n");
                    break;
            }
        }
        printf("\n");

        cmsObj_free((void **)&obj);
    }
}

CmsRet rutOmci_getEthPortTypeByName(char *name, OmciEthPortType *type)
{
    UINT32 port = 0;
    OmciEthPortType_t eth;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;
    CmsRet ret = CMSRET_OBJECT_NOT_FOUND;
    char uniName[BUFLEN_32];
    int rc;

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
      (void*)&obj)) == CMSRET_SUCCESS)
    {
        eth.types.all = obj->ethernetTypes;
        for (port = 0; port < obj->numberOfEthernetPorts; port++)
        {
            rc = bcm_oamindex_to_ifname_get_wrap((int)port, uniName); 
            if (rc == 0 && strcmp(name, uniName) == 0)
            {
                *type = omciRut_getPortType(port, eth.types.all);
                ret = CMSRET_SUCCESS;
                break;
            }
        }

        cmsObj_free((void**)&obj);
    }

    return ret;
}

CmsRet rutOmci_getBrgFwdMask(UINT32 *brgFwdMask)
{
    CmsRet ret = CMSRET_SUCCESS;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *omciSysObj = NULL;

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, OGF_NO_VALUE_UPDATE,
      (void*)&omciSysObj)) == CMSRET_SUCCESS)
    {
        *brgFwdMask = omciSysObj->bridgeGroupFwdMask;
        cmsObj_free((void**)&omciSysObj);
    }

    return ret;
}

#endif /* DMP_X_ITU_ORG_GPON_1 */
