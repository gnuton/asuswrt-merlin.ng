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

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <arpa/inet.h> /* for inet_ntop */

#include "os_defs.h"
#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_strconv.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_wan.h"
#include "rut_dnsproxy.h"
#include "rut_iptables.h"
#include "rut_pon_voice.h"
#include "rut_network.h"
#include "rut_omci.h"

#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
#include "rut_route.h"
#endif

#define OMCI_ENTRY_SIZE_52 52
#define OMCI_ENTRY_SIZE_48 48
#define OMCI_ENTRY_SIZE_32 32
#define OMCI_ENTRY_SIZE_25 25
#define OMCI_ENTRY_SIZE_24 24
#define OMCI_ENTRY_SIZE_20 20
#define OMCI_ENTRY_SIZE_16 16
#define OMCI_ENTRY_SIZE_8 8

typedef BcmOmciRtdIpHostConfigDataObject                IpHostConfigDataObject;
typedef _BcmOmciRtdIpHostConfigDataObject               _IpHostConfigDataObject;
typedef BcmOmciRtdIpHostConfigDataExtObject             BCM_IpHostConfigDataObject;
typedef _BcmOmciRtdIpHostConfigDataExtObject            _BCM_IpHostConfigDataObject;
typedef BcmOmciRtdIpv6HostConfigDataObject              Ipv6HostConfigDataObject;
typedef _BcmOmciRtdIpv6HostConfigDataObject             _Ipv6HostConfigDataObject;
typedef BcmOmciRtdIpv6HostConfigDataExtObject           BCM_Ipv6HostConfigDataObject;
typedef _BcmOmciRtdIpv6HostConfigDataExtObject          _BCM_Ipv6HostConfigDataObject;
typedef BcmOmciRtdIpv6CurrentAddressTableObject         Ipv6CurrentAddressTableObject;
typedef _BcmOmciRtdIpv6CurrentAddressTableObject        _Ipv6CurrentAddressTableObject;
typedef BcmOmciRtdIpv6CurrentDefaultRouterTableObject	Ipv6CurrentDefaultRouterTableObject;
typedef _BcmOmciRtdIpv6CurrentDefaultRouterTableObject	_Ipv6CurrentDefaultRouterTableObject;
typedef BcmOmciRtdIpv6CurrentDnsTableObject             Ipv6CurrentDnsTableObject;
typedef _BcmOmciRtdIpv6CurrentDnsTableObject            _Ipv6CurrentDnsTableObject;
typedef BcmOmciRtdIpv6CurrentOnlinkPrefixTableObject	Ipv6CurrentOnlinkPrefixTableObject;
typedef _BcmOmciRtdIpv6CurrentOnlinkPrefixTableObject	_Ipv6CurrentOnlinkPrefixTableObject;
typedef BcmOmciRtdTcpUdpConfigDataObject               TcpUdpConfigDataObject;
typedef _BcmOmciRtdTcpUdpConfigDataObject              _TcpUdpConfigDataObject;

CmsRet rutOmci_refreshDnsServer(UINT32 dns1, UINT32 dns2, UINT8 *dns6);
CmsRet rutOmci_activateIpInterface(UINT32 oid, void *obj);
CmsRet rutOmci_startDhcp6c(_BCM_Ipv6HostConfigDataObject *obj);
CmsRet rutOmci_getIfNameByMeId(UINT32 meId, char *ifName);
void rutOmci_configTcpUdpByIpHost(UINT32 meId, UBOOL8 add);
CmsRet rutOmci_configDns(const char *dns1, const char *dns2);
CmsRet rutOmci_addAutoObject(UINT32 oid, UINT32 managedEntityId, UBOOL8 persistent);
CmsRet rutOmci_setAutoObject(UINT32 oid, UINT32 oldId, UINT32 newId);
CmsRet rutOmci_deleteAutoObject(UINT32 oid, UINT32 managedEntityId);
CmsRet rutOmci_getIfNameFromBcmIpv6Obj(UINT32 oid, const InstanceIdStack * iidStack, char *ifName);


/* Static functions. */


UBOOL8 rutOmci_isObjectExisted(UINT32 oid, UINT32 managedEntityId)
{
    UBOOL8 found = FALSE;
    void   *obj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

    /* search instance that has id matched with the given id */
    while ((!found) &&
           (cmsObj_getNextFlags(oid,
                                &iidStack,
                                OGF_NO_VALUE_UPDATE,
                                (void **) &obj) == CMSRET_SUCCESS))
    {
        // MacBridgePortBridgeTableDataObject is used as generic type
        // since it only has managedEntityId as its parameter
        found = (((IpHostConfigDataObject *)obj)->managedEntityId == managedEntityId);
        cmsObj_free((void **) &obj);
    }

    return found;
}

static void rutOmci_updateIpHostConfigDataObject
    (_IpHostConfigDataObject *newObj,
     const _IpHostConfigDataObject *currObj)
{
    UBOOL8 diff = FALSE;

    if (newObj->managedEntityId != currObj->managedEntityId)
    {
        diff = TRUE;
    }

    if (newObj->ipOptions != currObj->ipOptions)
    {
        diff = TRUE;
        // dynamic and static is changed
        if ((newObj->ipOptions & 1) != (currObj->ipOptions & 1))
        {
            //change to dynamic
            if (newObj->ipOptions & 1)
            {
                //clear all current settings
                newObj->currentAddress = 0;
                newObj->currentGateway = 0;
                newObj->currentMask = 0;
                newObj->currentPrimaryDns = 0;
                newObj->currentSecondaryDns = 0;
            }
            else // change to static
            {
                newObj->currentAddress = newObj->ipAddress;
                newObj->currentGateway = newObj->gateway;
                newObj->currentMask = newObj->mask;
                newObj->currentPrimaryDns = newObj->primaryDns;
                newObj->currentSecondaryDns = newObj->secondaryDns;
            }
        }
    }
    if ((newObj->ipOptions & 1)  == 0 && newObj->ipAddress != currObj->ipAddress)
    {
        newObj->currentAddress = newObj->ipAddress;
        diff = TRUE;
    }
    if ((newObj->ipOptions & 1)  == 0 && newObj->mask != currObj->mask)
    {
        newObj->currentMask = newObj->mask;
        diff = TRUE;
    }
    if ((newObj->ipOptions & 1)  == 0 && newObj->gateway != currObj->gateway)
    {
        newObj->currentGateway = newObj->gateway;
        diff = TRUE;
    }
    if ((newObj->ipOptions & 1)  == 0 && newObj->primaryDns != currObj->primaryDns)
    {
        newObj->currentPrimaryDns = newObj->primaryDns;
        diff = TRUE;
    }
    if ((newObj->ipOptions & 1)  == 0 && newObj->secondaryDns != currObj->secondaryDns)
    {
        newObj->currentSecondaryDns = newObj->secondaryDns;
        diff = TRUE;
    }

//remove policy route of ipHost
#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
    if (newObj->currentAddress != currObj->currentAddress)
    {
        char cmd[BUFLEN_1024];
        struct in_addr inAddr;

        inAddr.s_addr = htonl(currObj->currentAddress);
        snprintf(cmd, sizeof(cmd), "ip rule del from %s table %d 2>/dev/null",
                 inet_ntoa(inAddr), RT_TABLE_GPON_IPHOST);
        rut_doSystemAction("rcl_gpon", cmd);
    }
#endif

    // if any changes then update BcmIpHostConfigDataObject
    // so that rutOmci_activateIpInterface() is called
    if (diff == TRUE)
    {
        rutOmci_setAutoObject
            (MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA_EXT,
             currObj->managedEntityId, newObj->managedEntityId);
    }

    if (newObj->currentPrimaryDns != currObj->currentPrimaryDns ||
        newObj->currentSecondaryDns != currObj->currentSecondaryDns)
    {
        rutOmci_refreshDnsServer(newObj->currentPrimaryDns, newObj->currentSecondaryDns, NULL);
    }
}

CmsRet rcl_bcmOmciRtdIpHostConfigDataObject( _IpHostConfigDataObject *newObj,
                const _IpHostConfigDataObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;
    CmsRet ret = CMSRET_SUCCESS;

    /*
     * When IP Host Config Data is first configured, there's no externalIPAddress yet.
     * Just start dhcpc and return.  When dhcpc gets an IP address, it will send out
     * an event msg to ssk, which will do a cmsObj_set on BCM_IpHostConfigDataObject,
     * then call rutOmci_activateIpInterface with newly assigned externalIPAddress
     */
    if(ADD_NEW(newObj, currObj))
    {
        // create proprietary BCM_IpHostConfigDataObject to keep connection status
        ret = rutOmci_addAutoObject
            (MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA_EXT, newObj->managedEntityId, FALSE);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
//remove policy route of ipHost
#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
        if (currObj->currentAddress)
        {
            char cmd[BUFLEN_1024];
            struct in_addr inAddr;

            inAddr.s_addr = htonl(currObj->currentAddress);
            snprintf(cmd, sizeof(cmd), "ip rule del from %s table %d 2>/dev/null",
                     inet_ntoa(inAddr), RT_TABLE_GPON_IPHOST);
            rut_doSystemAction("rcl_gpon", cmd);
        }
#endif

        mdmLibCtx.hideObjectsPendingDelete = FALSE;
        // delete all existed flows for this IpHostConfigDataObject
        mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
        // delete proprietary BCM_IpHostConfigDataObject
        ret = rutOmci_deleteAutoObject
            (MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA_EXT, currObj->managedEntityId);
    }
    else
    {
        rutOmci_updateIpHostConfigDataObject(newObj, currObj);
        // start DHCPC or activate IpHost interface
        // for new IpHostConfigDataObject
        /* Only handle attribute value change notification for ipHostConfigData
         * if changes are not from OMCID
         */
        if (mdmLibCtx.eid != EID_OMCID)
        {
            //rcl_attributeValueChangeIpHost(newObj, currObj);
        }
    }

    return ret;
}

CmsRet rcl_bcmOmciRtdIpHostConfigDataExtObject( _BCM_IpHostConfigDataObject *newObj,
                const _BCM_IpHostConfigDataObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;

    /*
     * When IP Host Config Data is first configured, there's no externalIPAddress yet.
     * Just start dhcpc and return.  When dhcpc gets an IP address, it will send out
     * an event msg to ssk, which will do a cmsObj_set on this object, which will
     * then call rutOmci_activateIpInterface with newly assigned externalIPAddress
     */
    if (ADD_NEW(newObj, currObj))
    {
        newObj->connectionStatus = OMCI_CONN_UNCONFIGURE;
        newObj->dhcpcPid = 0;
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        // Stop dhcp client
        if (currObj->dhcpcPid)
        {
            UINT32 specificEid = MAKE_SPECIFIC_EID(currObj->dhcpcPid, EID_DHCPC);
            cmsLog_debug("stop dhcpc pid=%d", currObj->dhcpcPid);
            if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0) != CMSRET_SUCCESS)
            {
                cmsLog_error("failed to send msg to stop dhcpc");
            }
            else
            {
                cmsLog_debug("dhcpc stopped");
            }
        }
#ifdef OMCI_TR69_DUAL_STACK
        // reset to default tr069 settings if it be bound to this ipHost
        if (currObj->interfaceName)
        {
            ManagementServerObject *acsCfg = NULL;
            InstanceIdStack iidManServer = EMPTY_INSTANCE_ID_STACK;
            if (cmsObj_get(MDMOID_MANAGEMENT_SERVER,
                           &iidManServer, 0, (void *) &acsCfg) == CMSRET_SUCCESS)
            {
                if(cmsUtl_strcmp(acsCfg->X_BROADCOM_COM_BoundIfName, currObj->interfaceName) == 0)
                {
                    CMSMEM_REPLACE_STRING_FLAGS(acsCfg->X_BROADCOM_COM_BoundIfName, "Any_WAN", mdmLibCtx.allocFlags);
                    CMSMEM_FREE_BUF_AND_NULL_PTR(acsCfg->URL);
                    CMSMEM_REPLACE_STRING_FLAGS(acsCfg->username, "admin", mdmLibCtx.allocFlags);
                    CMSMEM_REPLACE_STRING_FLAGS(acsCfg->password, "admin", mdmLibCtx.allocFlags);
                    CMSMEM_FREE_BUF_AND_NULL_PTR(acsCfg->connectionRequestURL);

                    cmsObj_set(acsCfg, &iidManServer);
                }
                cmsObj_free((void **) &acsCfg);
            }
        }
#endif
    }
    else
    {
        UBOOL8 found = FALSE;
        InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
        IpHostConfigDataObject *ipHost = NULL;

        // look for IP Host that has its managedEntityId
        // match with managedEntityId of Broadcom IP Host
        while ((!found) &&
               (cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA,
                                     &iidStack2, (void **) &ipHost)
                == CMSRET_SUCCESS))
        {
            found = (ipHost->managedEntityId == newObj->managedEntityId);
            if (found == TRUE)
            {
                if ((ipHost->ipOptions & 0x0001) == 0)
                    rutOmci_activateIpInterface(MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA, (void *)ipHost);
                else
                {
                    UBOOL8 diff = FALSE;
                    if (newObj->interfaceName != NULL &&
                        currObj->interfaceName == NULL)
                       diff = TRUE;
                    else if (newObj->interfaceName != NULL &&
                             currObj->interfaceName != NULL &&
                             strcmp(newObj->interfaceName, currObj->interfaceName) != 0)
                        diff = TRUE;
                    else if (newObj->dhcpcPid != currObj->dhcpcPid)
                        diff = TRUE;
                    else if (newObj->connectionStatus != currObj->connectionStatus)
                        diff = TRUE;

                    // if interface name, or dhcpcPid, or connectionStatus
                    // is changed then re-configure IpHostConfigDataObject
                    if (diff == TRUE)
                        rutOmci_activateIpInterface(MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA, (void *)ipHost);
                }
            }
            cmsObj_free((void **) &ipHost);
        }
    }

   return ret;
}

#ifdef SUPPORT_IPV6

void rutOmci_clearTableAttribute
    (const UINT32 oid,
     const InstanceIdStack *pIidStack)
{
    InstanceIdStack iid = EMPTY_INSTANCE_ID_STACK;
    void *table = NULL;

    while (cmsObj_getNextInSubTree(oid, pIidStack, &iid, (void **) &table) == CMSRET_SUCCESS)
    {
        cmsObj_deleteInstance(oid, &iid);
        cmsObj_free((void **)&table);
        INIT_INSTANCE_ID_STACK(&iid);
    }
}

UBOOL8 ipv6TableAttributeExisted(const UINT32 oid,
                                 const InstanceIdStack *pIidStack,
                                 const HEXBINARY value)
{
   UBOOL8 exist = FALSE;
   void *table = NULL;
   InstanceIdStack iidAttribute = EMPTY_INSTANCE_ID_STACK;
   UINT32 objSize = 0;
   char *tableStr = NULL;

   /*
    * TODO: We don't support plt and vlt now. So we only compare the following:
    * currentAddressTable: address (16*2 bytes)
    * currentRouterTable: address (16*2 bytes)
    * currentDnsTable: address (16*2 bytes)
    * currentPrefixTable: prefixLen+A flag+prefix ((1+1+16)*2 bytes)
    */
   while (cmsObj_getNextInSubTree(oid, pIidStack, &iidAttribute, (void **) &table) == CMSRET_SUCCESS)
   {
      switch (oid)
      {
         case MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ADDRESS_TABLE:
            objSize = OMCI_ENTRY_SIZE_32;
            tableStr = ((Ipv6CurrentAddressTableObject *)table)->currentAddressEntry;
            break;
         case MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DEFAULT_ROUTER_TABLE:
            objSize = OMCI_ENTRY_SIZE_32;
            tableStr = ((Ipv6CurrentDefaultRouterTableObject *)table)->currentDefaultRouterEntry;
            break;
         case MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DNS_TABLE:
            objSize = OMCI_ENTRY_SIZE_32;
            tableStr = ((Ipv6CurrentDnsTableObject *)table)->currentDnsEntry;
            break;
         case MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ONLINK_PREFIX_TABLE:
            objSize = 36;
            tableStr = ((Ipv6CurrentOnlinkPrefixTableObject *)table)->currentOnlinkPrefixTableEntry;
            break;
         default :
            cmsLog_error("unknown oid");
            goto done;
      }

      if (memcmp(tableStr, value, objSize) != 0)
      {
         cmsObj_free((void **)&table);
      }
      else
      {
         exist = TRUE;
         goto done;
      }
   }

done:
   cmsObj_free((void **)&table);
   cmsLog_debug("exist<%d>", exist);

   return exist;
}

CmsRet rutOmci_addIpv6TableAttribute
    (const UINT32 oid,
     const InstanceIdStack *pIidStack,
     const HEXBINARY value)
{
    CmsRet ret = CMSRET_SUCCESS;
    InstanceIdStack iid = EMPTY_INSTANCE_ID_STACK;
    void *table = NULL;
    char *hexStr = NULL;
    UINT32 objSize = 0;

    if (ipv6TableAttributeExisted(oid, pIidStack, value))
    {
        cmsLog_debug("attribute existed");
        return ret;
    }

    memcpy(&iid, pIidStack, sizeof(InstanceIdStack));

    // TODO: check if it is duplicate before add
    // add given entry to the table
    ret = cmsObj_addInstance(oid, &iid);
    if (ret == CMSRET_SUCCESS)
    {
        ret = cmsObj_get(oid, &iid, 0, (void **) &table);
        if (ret == CMSRET_SUCCESS)
        {
            switch (oid)
            {
                case MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ADDRESS_TABLE:
                    objSize = OMCI_ENTRY_SIZE_48;
                    cmsMem_free(((Ipv6CurrentAddressTableObject *)table)->currentAddressEntry);
                    ((Ipv6CurrentAddressTableObject *)table)->currentAddressEntry = cmsMem_alloc(objSize + 1, ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
                    hexStr = ((Ipv6CurrentAddressTableObject *)table)->currentAddressEntry;
                    break;
                case MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DEFAULT_ROUTER_TABLE:
                    objSize = OMCI_ENTRY_SIZE_32;
                    cmsMem_free(((Ipv6CurrentDefaultRouterTableObject *)table)->currentDefaultRouterEntry);
                    ((Ipv6CurrentDefaultRouterTableObject *)table)->currentDefaultRouterEntry = cmsMem_alloc(objSize + 1, ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
                    hexStr = ((Ipv6CurrentDefaultRouterTableObject *)table)->currentDefaultRouterEntry;
                    break;
                case MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DNS_TABLE:
                    objSize = OMCI_ENTRY_SIZE_32;
                    cmsMem_free(((Ipv6CurrentDnsTableObject *)table)->currentDnsEntry);
                     ((Ipv6CurrentDnsTableObject *)table)->currentDnsEntry = cmsMem_alloc(objSize + 1, ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
                    hexStr = ((Ipv6CurrentDnsTableObject *)table)->currentDnsEntry;
                    break;
                case MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ONLINK_PREFIX_TABLE:
                    objSize = OMCI_ENTRY_SIZE_52;
                    cmsMem_free(((Ipv6CurrentOnlinkPrefixTableObject *)table)->currentOnlinkPrefixTableEntry);
                     ((Ipv6CurrentOnlinkPrefixTableObject *)table)->currentOnlinkPrefixTableEntry = cmsMem_alloc(objSize + 1, ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
                    hexStr = ((Ipv6CurrentOnlinkPrefixTableObject *)table)->currentOnlinkPrefixTableEntry;
                    break;
                default :
                    hexStr = NULL;
                    break;
            }
            if (hexStr != NULL)
            {
                memcpy(&hexStr[0], value, objSize);

                // do not call cmsMem_free(hexStr) since
                // it is free later in cmsObj_free((void **)&table)
                if (ret == CMSRET_SUCCESS)
                    ret = cmsObj_set(table, &iid);
            }
            else
            {
                cmsLog_error("failed to allocate memory");
                ret = CMSRET_INTERNAL_ERROR;
            }
            cmsObj_free((void **)&table);
        }
        else
            cmsLog_error("Could not get table attribute object, oid=%d, ret=%d", oid, ret);
    }
    else
        cmsLog_error("Could not add table attribute object, oid=%d, ret=%d", oid, ret);

    return ret;
}

#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
static void removeIpv6PolicyRoute(const HEXBINARY address)
{
    char cmd[BUFLEN_1024], ipv6str[CMS_IPADDR_LENGTH];
    UINT32 size = 0;
    UINT8 *buf = NULL;
    CmsRet ret = CMSRET_SUCCESS;

    // convert ipv6Address hexString (32 bytes) to struct in6_addr (16 bytes) buf
    ret = cmsUtl_hexStringToBinaryBuf(address, &buf, &size);
    if (ret == CMSRET_SUCCESS)
    {
        // struct in6_addr (16 bytes) buf to ipv6 string format (48 bytes)
        inet_ntop(AF_INET6, buf, ipv6str, CMS_IPADDR_LENGTH);

        snprintf(cmd, sizeof(cmd), "ip -6 rule del from %s table %d 2>/dev/null",
             ipv6str, RT_TABLE_GPON_IPV6HOST);
        rut_doSystemAction("rcl_gpon", cmd);
    }

    if (buf != NULL)
    {
        // free temporary memory
        cmsMem_free(buf);
    }
}
#endif

static void updateIpv6HostConfigDataObject
    (_Ipv6HostConfigDataObject *newObj,
     const _Ipv6HostConfigDataObject *currObj __attribute__((unused)),
     const InstanceIdStack *iidStack)
{
//    UBOOL8 diff = FALSE;  set but not used

    if (newObj->ipOptions != 0)
    {
        //static IPv6 configuration (IpOption has both RS (2nd bit) and DHCPv6 (3rd bit) are 0)
        //TODO: We create currentTable objects for addr, gw, dns, and prefix. Therefore,
        //all configuration actions will be triggered based on those .{i} objects.
        //The "currentTable parameters" of OMCI spec is only for AVS. So we need to gather all
        //information from our .{i} objects and copy to "currentTable parameters" for AVS while
        //implementing AVS
        if ((newObj->ipOptions & 0x06) == 0)
        {
            UINT32 newSize = 0;
//            UINT32 currSize = 0;  set but not used

            newSize = strlen(newObj->ipv6Address);
//            currSize = strlen(currObj->ipv6Address);
            if (newSize == OMCI_ENTRY_SIZE_32)
            {
//                if ((newSize != currSize) ||
//                    (newSize == currSize &&
//                     memcmp(newObj->ipv6Address, currObj->ipv6Address, OMCI_ENTRY_SIZE_32) != 0))
                {
                    UINT8 *buffer = NULL;
                    UINT32 size = 0;
                    UINT8 addrInfo[24];
                    UINT8 lifeTime[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                    char *tmpStr = NULL;

                    cmsUtl_hexStringToBinaryBuf(newObj->ipv6Address, &buffer, &size);

                    memcpy(&addrInfo[0], &buffer[0], OMCI_ENTRY_SIZE_16);
                    memcpy(&addrInfo[OMCI_ENTRY_SIZE_16], lifeTime, OMCI_ENTRY_SIZE_8);

                    cmsUtl_binaryBufToHexString(addrInfo, sizeof(addrInfo), &tmpStr);
                    CMSMEM_FREE_BUF_AND_NULL_PTR(buffer);

                    if (newObj->currentAddressTable != NULL)
                        cmsMem_free(newObj->currentAddressTable);

                    newObj->currentAddressTable = cmsMem_alloc(OMCI_ENTRY_SIZE_48 + 1, ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
                    memcpy(newObj->currentAddressTable, newObj->ipv6Address, OMCI_ENTRY_SIZE_32);

                    rutOmci_clearTableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ADDRESS_TABLE, iidStack);
                    rutOmci_addIpv6TableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ADDRESS_TABLE, iidStack, tmpStr);
                    CMSMEM_FREE_BUF_AND_NULL_PTR(tmpStr);
//                    diff = TRUE;
#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
                    //remove policy route of ipv6Host
                    removeIpv6PolicyRoute(currObj->ipv6Address);
#endif
                }
            }

            newSize = strlen(newObj->defaultRouter);
//            currSize = strlen(currObj->defaultRouter);
            if (newSize == OMCI_ENTRY_SIZE_32)
            {
//                if ((newSize != currSize) ||
//                    (newSize == currSize &&
//                     memcmp(newObj->defaultRouter, currObj->defaultRouter, OMCI_ENTRY_SIZE_32) != 0))
                {
                    if (newObj->currentDefaultRouterTable != NULL)
                        cmsMem_free(newObj->currentDefaultRouterTable);
                    newObj->currentDefaultRouterTable = cmsMem_alloc(OMCI_ENTRY_SIZE_32 + 1, ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
                    memcpy(newObj->currentDefaultRouterTable, newObj->defaultRouter, OMCI_ENTRY_SIZE_32);
                    rutOmci_clearTableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DEFAULT_ROUTER_TABLE, iidStack);
                    rutOmci_addIpv6TableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DEFAULT_ROUTER_TABLE, iidStack, newObj->defaultRouter);
//                    diff = TRUE;
                }
            }

            newSize = strlen(newObj->primaryDns);
//            currSize = strlen(currObj->primaryDns);
            if (newSize == OMCI_ENTRY_SIZE_32)
            {
//                if ((newSize != currSize) ||
//                    (newSize == currSize &&
//                     memcmp(newObj->primaryDns, currObj->primaryDns, OMCI_ENTRY_SIZE_32) != 0))
                {
                    if (newObj->currentDnsTable != NULL)
                        cmsMem_free(newObj->currentDnsTable);
                    newObj->currentDnsTable = cmsMem_alloc(OMCI_ENTRY_SIZE_32 + 1, ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
                    memcpy(newObj->currentDnsTable, newObj->primaryDns, OMCI_ENTRY_SIZE_32);
                    rutOmci_clearTableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DNS_TABLE, iidStack);
                    rutOmci_addIpv6TableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DNS_TABLE, iidStack, newObj->primaryDns);
//                    diff = TRUE;
                }
            }

            newSize = strlen(newObj->secondaryDns);
//            currSize = strlen(currObj->secondaryDns);
            if (newSize == OMCI_ENTRY_SIZE_32)
            {
//                if ((newSize != currSize) ||
//                    (newSize == currSize &&
//                     memcmp(newObj->secondaryDns, currObj->secondaryDns, OMCI_ENTRY_SIZE_32) != 0))
                {
                    rutOmci_addIpv6TableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DNS_TABLE, iidStack, newObj->secondaryDns);
//                    diff = TRUE;
                }
            }

            newSize = strlen(newObj->onlinkPrefix);
//            currSize = strlen(currObj->onlinkPrefix);
            if (newSize == 34)
            {
//                if ((newSize != currSize) ||
//                    (newSize == currSize &&
//                     memcmp(newObj->onlinkPrefix, currObj->onlinkPrefix, 34) != 0))
                {
                    UINT8 *buffer = NULL;
                    UINT32 size = 0;
                    UINT8 prefixInfo[26];
                    UINT8 lifeTime[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                    char *tmpStr = NULL;

                    cmsUtl_hexStringToBinaryBuf(newObj->onlinkPrefix, &buffer, &size);

                    prefixInfo[0] = buffer[0]; //prefix len
                    prefixInfo[1] = 0; //A-flag
                    memcpy(&prefixInfo[2], &buffer[1], OMCI_ENTRY_SIZE_16);
                    memcpy(&prefixInfo[2+OMCI_ENTRY_SIZE_16], lifeTime, OMCI_ENTRY_SIZE_8);

                    cmsUtl_binaryBufToHexString(prefixInfo, sizeof(prefixInfo), &tmpStr);
                    CMSMEM_FREE_BUF_AND_NULL_PTR(buffer);

                    if (newObj->currentOnlinkPrefixTable != NULL)
                        cmsMem_free(newObj->currentOnlinkPrefixTable);
                    rutOmci_clearTableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ONLINK_PREFIX_TABLE, iidStack);
                    rutOmci_addIpv6TableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ONLINK_PREFIX_TABLE, iidStack, tmpStr);
                    CMSMEM_FREE_BUF_AND_NULL_PTR(tmpStr);
//                    diff = TRUE;
                }
            }
        }
    }
}

void updateParamBasedOnObjs(_Ipv6HostConfigDataObject *obj, const InstanceIdStack *iidStack)
{
   InstanceIdStack iid = EMPTY_INSTANCE_ID_STACK;
   void *table = NULL;

   /*
    * TODO: we only copy the first table into the param.
    * We should copy all tables in the future
    */
   if (cmsObj_getNextInSubTree(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ADDRESS_TABLE, iidStack,
                              &iid, (void **) &table) == CMSRET_SUCCESS)
   {
      if (obj->currentAddressTable != NULL)
      {
         cmsMem_free(obj->currentAddressTable);
      }

      obj->currentAddressTable = cmsMem_alloc(OMCI_ENTRY_SIZE_48 + 1, ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
      memcpy(obj->currentAddressTable, ((Ipv6CurrentAddressTableObject *)table)->currentAddressEntry,
             OMCI_ENTRY_SIZE_48);
      cmsObj_free((void **)&table);
   }
   INIT_INSTANCE_ID_STACK(&iid);

   if (cmsObj_getNextInSubTree(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DEFAULT_ROUTER_TABLE, iidStack,
                              &iid, (void **) &table) == CMSRET_SUCCESS)
   {
      if (obj->currentDefaultRouterTable != NULL)
      {
         cmsMem_free(obj->currentDefaultRouterTable);
      }

      obj->currentDefaultRouterTable = cmsMem_alloc(OMCI_ENTRY_SIZE_32 + 1, ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
      memcpy(obj->currentDefaultRouterTable,
             ((Ipv6CurrentDefaultRouterTableObject *)table)->currentDefaultRouterEntry,
             OMCI_ENTRY_SIZE_32);
      cmsObj_free((void **)&table);
   }
   INIT_INSTANCE_ID_STACK(&iid);

   if (cmsObj_getNextInSubTree(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DNS_TABLE, iidStack,
                              &iid, (void **) &table) == CMSRET_SUCCESS)
   {
      if (obj->currentDnsTable != NULL)
      {
         cmsMem_free(obj->currentDnsTable);
      }

      obj->currentDnsTable = cmsMem_alloc(OMCI_ENTRY_SIZE_32 + 1, ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
      memcpy(obj->currentDnsTable,
             ((Ipv6CurrentDnsTableObject *)table)->currentDnsEntry,
             OMCI_ENTRY_SIZE_32);
      cmsObj_free((void **)&table);
   }
   INIT_INSTANCE_ID_STACK(&iid);

   if (cmsObj_getNextInSubTree(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ONLINK_PREFIX_TABLE, iidStack,
                              &iid, (void **) &table) == CMSRET_SUCCESS)
   {
      if (obj->currentOnlinkPrefixTable != NULL)
      {
         cmsMem_free(obj->currentOnlinkPrefixTable);
      }

      obj->currentOnlinkPrefixTable = cmsMem_alloc(OMCI_ENTRY_SIZE_52 + 1, ALLOC_SHARED_MEM | ALLOC_ZEROIZE);
      memcpy(obj->currentOnlinkPrefixTable,
             ((Ipv6CurrentOnlinkPrefixTableObject *)table)->currentOnlinkPrefixTableEntry,
             OMCI_ENTRY_SIZE_52);
      cmsObj_free((void **)&table);
   }
}

#endif /* SUPPORT_IPV6 */

CmsRet rcl_bcmOmciRtdIpv6HostConfigDataObject( _Ipv6HostConfigDataObject *newObj,
                const _Ipv6HostConfigDataObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;

#ifdef SUPPORT_IPV6

    UBOOL8 prevHideObjectsPendingDelete = mdmLibCtx.hideObjectsPendingDelete;

    if (ADD_NEW(newObj, currObj))
    {
        // create proprietary BCM_Ipv6HostConfigDataObject to keep connection status
        ret = rutOmci_addAutoObject
            (MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA_EXT, newObj->managedEntityId, FALSE);
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
        //remove policy route of ipv6Host
        removeIpv6PolicyRoute(currObj->ipv6Address);
#endif
        mdmLibCtx.hideObjectsPendingDelete = FALSE;
        // delete all existed flows for this Ipv6HostConfigDataObject
        mdmLibCtx.hideObjectsPendingDelete = prevHideObjectsPendingDelete;
        // delete proprietary BCM_Ipv6HostConfigDataObject
        ret = rutOmci_deleteAutoObject
            (MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA_EXT, currObj->managedEntityId);
    }
    else
    {
       char ifName[BUFLEN_32];

       if (newObj->managedEntityId != currObj->managedEntityId)
       {
           rutOmci_setAutoObject
               (MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA_EXT,
                currObj->managedEntityId, newObj->managedEntityId);
       }

       if (((newObj->ipOptions & 0x06) != 0 && (currObj->ipOptions & 0x06) == 0) ||
           ((newObj->ipOptions & 0x06) == 0 && (currObj->ipOptions & 0x06) != 0))
       {
           // dynamic and static is changed
           // clear all current table attributes
           rutOmci_clearTableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ADDRESS_TABLE, iidStack);
           rutOmci_clearTableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DEFAULT_ROUTER_TABLE, iidStack);
           rutOmci_clearTableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_DNS_TABLE, iidStack);
           rutOmci_clearTableAttribute(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ONLINK_PREFIX_TABLE, iidStack);
       }

       if ((rutOmci_getIfNameByMeId(newObj->managedEntityId, ifName) == CMSRET_SUCCESS) &&
           ((newObj->ipOptions & 0x06) == 0))
       {
           //this is only for static IPv6 configuration
           updateIpv6HostConfigDataObject(newObj, currObj, iidStack);
       }

        updateParamBasedOnObjs(newObj, iidStack);
    }

#endif /* SUPPORT_IPV6 */

   return ret;
}

CmsRet rcl_bcmOmciRtdIpv6HostConfigDataExtObject( _BCM_Ipv6HostConfigDataObject *newObj,
                const _BCM_Ipv6HostConfigDataObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;

#ifdef SUPPORT_IPV6

    if (ADD_NEW(newObj, currObj))
    {
        newObj->connectionStatus = OMCI_CONN_UNCONFIGURE;
        newObj->dhcpcPid = 0;
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        // Stop dhcp6c
        if (currObj->dhcpcPid)
        {
            UINT32 specificEid = MAKE_SPECIFIC_EID(currObj->dhcpcPid, EID_DHCP6C);
            cmsLog_debug("stop dhcpc pid=%d", currObj->dhcpcPid);
            if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0) != CMSRET_SUCCESS)
            {
                cmsLog_error("failed to send msg to stop dhcp6c");
            }
            else
            {
                cmsLog_error("dhcp6c stopped");
            }
        }
#ifdef OMCI_TR69_DUAL_STACK
        // reset to default tr069 settings if it be bound to this ipHost
        if (currObj->interfaceName)
        {
            ManagementServerObject *acsCfg = NULL;
            InstanceIdStack iidManServer = EMPTY_INSTANCE_ID_STACK;
            if (cmsObj_get(MDMOID_MANAGEMENT_SERVER,
                           &iidManServer, 0, (void *) &acsCfg) == CMSRET_SUCCESS)
            {
                if(cmsUtl_strcmp(acsCfg->X_BROADCOM_COM_BoundIfName, currObj->interfaceName) == 0)
                {
                    CMSMEM_REPLACE_STRING_FLAGS(acsCfg->X_BROADCOM_COM_BoundIfName, "Any_WAN", mdmLibCtx.allocFlags);
                    CMSMEM_FREE_BUF_AND_NULL_PTR(acsCfg->URL);
                    CMSMEM_REPLACE_STRING_FLAGS(acsCfg->username, "admin", mdmLibCtx.allocFlags);
                    CMSMEM_REPLACE_STRING_FLAGS(acsCfg->password, "admin", mdmLibCtx.allocFlags);
                    CMSMEM_FREE_BUF_AND_NULL_PTR(acsCfg->connectionRequestURL);

                    cmsObj_set(acsCfg, &iidManServer);
                }
                cmsObj_free((void **) &acsCfg);
            }
        }
#endif
    }
    else
    {
        UBOOL8 found = FALSE;
        InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
        Ipv6HostConfigDataObject *ipv6Host = NULL;

        // look for IPv6 Host that has its managedEntityId
        // match with managedEntityId of Broadcom IP Host
        while ((!found) &&
               (cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA,
                                     &iidStack2, (void **) &ipv6Host)
                == CMSRET_SUCCESS))
        {
            found = (ipv6Host->managedEntityId == newObj->managedEntityId);
            // if feature is locked or diable (ipOptions == 0) then do nothing
            if (found == TRUE && ipv6Host->ipOptions != 0)
            {
                if ((ipv6Host->ipOptions & 0x06) != 0)
                {
                    UBOOL8 diff = FALSE;
                    if (newObj->interfaceName != NULL &&
                        currObj->interfaceName == NULL)
                       diff = TRUE;
                    else if (newObj->interfaceName != NULL &&
                             currObj->interfaceName != NULL &&
                             strcmp(newObj->interfaceName, currObj->interfaceName) != 0)
                        diff = TRUE;
                    else if (newObj->MFlag != currObj->MFlag)
                        diff = TRUE;
                    else if (newObj->OFlag != currObj->OFlag)
                        diff = TRUE;

                    cmsLog_debug("diff<%d> new_ifName<%s> orig_ifName<%s>",
                                  diff, newObj->interfaceName, currObj->interfaceName);
                    // launch dhcp6c if necessary
                    if (diff && newObj->interfaceName != NULL)
                        rutOmci_startDhcp6c(newObj);
                    // TODO: set forwarding to 0
                    if (diff && newObj->interfaceName != NULL)
                    {
                       char cmd[512];
                       snprintf(cmd, sizeof(cmd), "echo 0 > /proc/sys/net/ipv6/conf/%s/forwarding",
                       newObj->interfaceName);
                       rut_doSystemAction("rcl_gpon", cmd);
                       snprintf(cmd, sizeof(cmd), "ifconfig %s mtu 1460",
                       newObj->interfaceName);
                       rut_doSystemAction("rcl_gpon", cmd);
                    }
                }
                else
                {
                   if (newObj->interfaceName != NULL &&
                       currObj->interfaceName == NULL)
                   {
                      cmsObj_set(ipv6Host, &iidStack2);
                   }
                }
            }
            cmsObj_free((void **) &ipv6Host);
        }
    }
#endif /* SUPPORT_IPV6 */

   return ret;
}

CmsRet rcl_bcmOmciRtdIpv6CurrentAddressTableObject(
                _Ipv6CurrentAddressTableObject *newObj,
                const _Ipv6CurrentAddressTableObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmd[BUFLEN_1024];
   char ipv6Address[CMS_IPADDR_LENGTH];

   memset(cmd, 0, BUFLEN_1024);
   memset(ipv6Address, 0, CMS_IPADDR_LENGTH);

   if (newObj && currObj && (newObj->currentAddressEntry != 0))
   {
      UINT8 *buf = NULL;
      UINT32 size = 0;
      char ifName[BUFLEN_32];

      if (rutOmci_getIfNameFromBcmIpv6Obj(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ADDRESS_TABLE,
                                                   iidStack, ifName) == CMSRET_SUCCESS)
      {
         // convert ipv6Address hexString (32 bytes) to struct in6_addr (16 bytes) buf
         cmsUtl_hexStringToBinaryBuf(newObj->currentAddressEntry, &buf, &size);
         // struct in6_addr (16 bytes) buf to ipv6 string format (48 bytes)
         inet_ntop(AF_INET6, buf, ipv6Address, CMS_IPADDR_LENGTH);
         // free temporary memory
         cmsMem_free(buf);

         snprintf(cmd, sizeof(cmd), "ip -6 addr add %s/64 dev %s 2>/dev/null",
                  ipv6Address, ifName);
         rut_doSystemAction("rut_gpon", cmd);
      }
   }
   else if ((newObj == NULL) && currObj)
   {
      UINT8 *buf = NULL;
      UINT32 size = 0;
      char ifName[BUFLEN_32];

      if (rutOmci_getIfNameFromBcmIpv6Obj(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ADDRESS_TABLE,
                                                   iidStack, ifName) == CMSRET_SUCCESS)
      {
         // convert ipv6Address hexString (32 bytes) to struct in6_addr (16 bytes) buf
         cmsUtl_hexStringToBinaryBuf(currObj->currentAddressEntry, &buf, &size);
         // struct in6_addr (16 bytes) buf to ipv6 string format (48 bytes)
         inet_ntop(AF_INET6, buf, ipv6Address, CMS_IPADDR_LENGTH);
         // free temporary memory
         cmsMem_free(buf);

         snprintf(cmd, sizeof(cmd), "ip -6 addr del %s/64 dev %s 2>/dev/null",
                  ipv6Address, ifName);
         rut_doSystemAction("rut_gpon", cmd);
      }
   }

   return ret;
}

CmsRet rcl_bcmOmciRtdIpv6CurrentDefaultRouterTableObject(
                _Ipv6CurrentDefaultRouterTableObject *newObj,
                const _Ipv6CurrentDefaultRouterTableObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmd[BUFLEN_1024];
   char gateway[CMS_IPADDR_LENGTH];

   memset(cmd, 0, BUFLEN_1024);
   memset(gateway, 0, CMS_IPADDR_LENGTH);

   if (newObj && currObj && (newObj->currentDefaultRouterEntry != 0))
   {
      UINT8 *buf = NULL;
      UINT32 size = 0;
      char ifName[BUFLEN_32];

      if (rutOmci_getIfNameFromBcmIpv6Obj(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ADDRESS_TABLE,
                                          iidStack, ifName) == CMSRET_SUCCESS)
      {
         // convert ipv6Address hexString (32 bytes) to struct in6_addr (16 bytes) buf
         cmsUtl_hexStringToBinaryBuf(newObj->currentDefaultRouterEntry, &buf, &size);
         // struct in6_addr (16 bytes) buf to ipv6 string format (48 bytes)
         inet_ntop(AF_INET6, buf, gateway, CMS_IPADDR_LENGTH);
         // free temporary memory
         cmsMem_free(buf);

         snprintf(cmd, sizeof(cmd), "ip -6 ro add default via %s dev %s 2>/dev/null",
                  gateway, ifName);
         rut_doSystemAction("rut_gpon", cmd);
      }
   }
   else if ((newObj == NULL) && currObj)
   {
      UINT8 *buf = NULL;
      UINT32 size = 0;
      char ifName[BUFLEN_32];

      if (rutOmci_getIfNameFromBcmIpv6Obj(MDMOID_BCM_OMCI_RTD_IPV6_CURRENT_ADDRESS_TABLE,
                                          iidStack, ifName) == CMSRET_SUCCESS)
      {
         // convert ipv6Address hexString (32 bytes) to struct in6_addr (16 bytes) buf
         cmsUtl_hexStringToBinaryBuf(currObj->currentDefaultRouterEntry, &buf, &size);
         // struct in6_addr (16 bytes) buf to ipv6 string format (48 bytes)
         inet_ntop(AF_INET6, buf, gateway, CMS_IPADDR_LENGTH);
         // free temporary memory
         cmsMem_free(buf);

         snprintf(cmd, sizeof(cmd), "ip -6 ro del default via %s dev %s 2>/dev/null",
                  gateway, ifName);
         rut_doSystemAction("rut_gpon", cmd);
      }
   }

   return ret;
}

CmsRet rcl_bcmOmciRtdIpv6CurrentDnsTableObject(
                _Ipv6CurrentDnsTableObject *newObj,
                const _Ipv6CurrentDnsTableObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   if (newObj && currObj && (newObj->currentDnsEntry != 0))
   {
      UINT32 size = 0;
      UINT8 *buf = NULL;

      cmsUtl_hexStringToBinaryBuf(newObj->currentDnsEntry, &buf, &size);
      rutOmci_refreshDnsServer(0, 0, buf);

      cmsMem_free(buf);
   }

   return ret;
}

CmsRet rcl_bcmOmciRtdIpv6CurrentOnlinkPrefixTableObject(
                _Ipv6CurrentOnlinkPrefixTableObject *newObj __attribute__((unused)),
                const _Ipv6CurrentOnlinkPrefixTableObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   //TODO: Configure a routing entry for the prefix??
   return ret;
}

CmsRet rcl_bcmOmciObject(
  _BcmOmciObject *newObj __attribute__((unused)),
  const _BcmOmciObject *currObj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)),
  char **errorParam __attribute__((unused)),
  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_bcmOmciRtdObject(
  _BcmOmciRtdObject *newObj __attribute__((unused)),
  const _BcmOmciRtdObject *currObj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)),
  char **errorParam __attribute__((unused)),
  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_bcmOmciRtdMcastObject(
 _BcmOmciRtdMcastObject *newObj __attribute__((unused)),
  const _BcmOmciRtdMcastObject *currObj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)),
  char **errorParam __attribute__((unused)),
  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_bcmOmciRtdLayer3Object(
  _BcmOmciRtdLayer3Object *newObj __attribute__((unused)),
  const _BcmOmciRtdLayer3Object *currObj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)),
  char **errorParam __attribute__((unused)),
  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_bcmOmciDebugObject(
  _BcmOmciDebugObject *newObj __attribute__((unused)),
  const _BcmOmciDebugObject *currObj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)),
  char **errorParam __attribute__((unused)),
  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_bcmOmciConfigObject(
  _BcmOmciConfigObject *newObj __attribute__((unused)),
  const _BcmOmciConfigObject *currObj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)),
  char **errorParam __attribute__((unused)),
  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_bcmOmciConfigSystemObject(
  _BcmOmciConfigSystemObject *newObj __attribute__((unused)),
  const _BcmOmciConfigSystemObject *currObj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)),
  char **errorParam __attribute__((unused)),
  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_bcmOmciStatsObject(
  _BcmOmciStatsObject *newObj __attribute__((unused)),
  const _BcmOmciStatsObject *currObj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)),
  char **errorParam __attribute__((unused)),
  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_bcmOmciStatsGponOmciStatsObject(
  _BcmOmciStatsGponOmciStatsObject *newObj __attribute__((unused)),
  const _BcmOmciStatsGponOmciStatsObject *currObj __attribute__((unused)),
  const InstanceIdStack *iidStack __attribute__((unused)),
  char **errorParam __attribute__((unused)),
  CmsRet *errorCode __attribute__((unused)))
{
    return CMSRET_SUCCESS;
}

CmsRet rcl_bcmOmciRtdTcpUdpConfigDataObject(
                _TcpUdpConfigDataObject *newObj,
                const _TcpUdpConfigDataObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
    CmsRet ret = CMSRET_SUCCESS;

    if (ADD_NEW(newObj, currObj))
    {
    }
    else if (DELETE_EXISTING(newObj, currObj))
    {
        // remove rule of currObj
        rutOmci_configTcpUdp(currObj, FALSE);
    }
    else
    {
        // remove rule of currObj
        rutOmci_configTcpUdp(currObj, FALSE);
        // add rule of newObj
        rutOmci_configTcpUdp(newObj, TRUE);
    }

    return ret;
}

CmsRet rutOmci_configIpv4Host(UINT32 meId, char *ifName)
{
    char cmd[BUFLEN_1024]; // 4 len 256 strings
    char *onuid = NULL;
    char onuidStr[BUFLEN_32];
    UINT32 onuidSize = 0;
    SINT32 pid = CMS_INVALID_PID, oldPid = CMS_INVALID_PID;
    UBOOL8 enblDhcp = FALSE;
    UBOOL8 found = FALSE, newName = FALSE;
    UINT8 ipOption = 0;
    IpHostConfigDataObject *ipHost = NULL;
    BCM_IpHostConfigDataObject *bcmIpHost = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet cmsResult = CMSRET_SUCCESS;

    if (ifName == NULL)
    {
        return CMSRET_INVALID_ARGUMENTS;
    }

    // Nested locks are allowed, but there must be an equal number of unlocks.
    cmsResult = cmsLck_acquireLockWithTimeout(6*MSECS_IN_SEC);

    // Test for locking success.
    if (cmsResult == CMSRET_SUCCESS)
    {
        /* search instance that has id matched with given meId */
        while ((!found) &&
               ((cmsResult = cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA, &iidStack, (void **) &ipHost))
                == CMSRET_SUCCESS))
        {
            found = (ipHost->managedEntityId == meId);
            if (found == TRUE)
            {
                ipOption = ipHost->ipOptions;
                enblDhcp = (ipHost->ipOptions & 0x0001) ? TRUE : FALSE;
                if (enblDhcp && ipHost->ontId && strlen(ipHost->ontId))
                {
                    if (cmsUtl_hexStringToBinaryBuf(ipHost->ontId, (UINT8 **)&onuid, &onuidSize) == CMSRET_SUCCESS)
                    {
                        memset(onuidStr, 0, BUFLEN_32);
                        memcpy(onuidStr, onuid, onuidSize);
                        cmsMem_free(onuid);
                        onuidSize = strlen(onuidStr);
                    }
                }
            }
            cmsObj_free((void **) &ipHost);
        }

        found = FALSE;
        INIT_INSTANCE_ID_STACK(&iidStack);

        // search bcmIpHost, if found and any attributes in this object
        // are changed then it MUST be set so that in RCL of bcmIpHost
        // rutOmci_activateIpInterface() function can be called
        while ((!found) &&
               ((cmsResult = cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA_EXT,
                                     &iidStack, (void **) &bcmIpHost))
                == CMSRET_SUCCESS))
        {
            found = (bcmIpHost->managedEntityId == meId);
            // only free obj when not found
            if (found == TRUE)
            {
                if (bcmIpHost->interfaceName == NULL)
                    newName = TRUE;
                else if (strcmp(bcmIpHost->interfaceName, ifName) != 0)
                    newName = TRUE;
                if (newName == TRUE)
                    CMSMEM_REPLACE_STRING_FLAGS(bcmIpHost->interfaceName,
                                                ifName,
                                                mdmLibCtx.allocFlags);
                bcmIpHost->connectionStatus = OMCI_CONN_UNCONFIGURE;
                oldPid = bcmIpHost->dhcpcPid;
                if (enblDhcp == TRUE &&
                    (newName == TRUE || bcmIpHost->dhcpcPid == 0))
                {
                    /* kill old DHCP client if any */
                    if (bcmIpHost->dhcpcPid != 0)
                    {
                        UINT32 specificEid = MAKE_SPECIFIC_EID(bcmIpHost->dhcpcPid, EID_DHCPC);
                        cmsLog_debug("stop dhcpc pid=%d", bcmIpHost->dhcpcPid);
                        if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0) != CMSRET_SUCCESS)
                        {
                            cmsLog_error("failed to send msg to stop dhcpc");
                        }
                        else
                        {
                            cmsLog_debug("dhcpc stopped");
                        }
                        bcmIpHost->dhcpcPid = 0;
                    }

                    if (onuidSize)
                    {
                        snprintf(cmd, sizeof(cmd), "-i %s -c %s", ifName, onuidStr);
                    }
                    else
                        snprintf(cmd, sizeof(cmd), "-i %s", ifName);

                    // start DHCP client
                    if ((pid = rut_sendMsgToSmd(CMS_MSG_START_APP,
                                                EID_DHCPC,
                                                cmd,
                                                strlen(cmd)+1)) == CMS_INVALID_PID)
                    {
                        cmsLog_error("failed to start dhcpc on %s", ifName);
                        cmsResult = CMSRET_INTERNAL_ERROR;
                    }
                    else
                    {
                        bcmIpHost->connectionStatus = OMCI_CONN_CONNECTING;    // connecting
                        bcmIpHost->dhcpcPid = pid;
                    }
                    cmsLog_debug("starting dhcpc, pid=%d on %s", pid, ifName);
                }
                else if (enblDhcp == FALSE && bcmIpHost->dhcpcPid != 0)
                {
                    //change form dhcp to static
                    UINT32 specificEid = MAKE_SPECIFIC_EID(bcmIpHost->dhcpcPid, EID_DHCPC);
                    cmsLog_debug("stop dhcpc pid=%d", bcmIpHost->dhcpcPid);
                    if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0) != CMSRET_SUCCESS)
                    {
                        cmsLog_error("failed to send msg to stop dhcpc");
                    }
                    else
                    {
                        cmsLog_debug("dhcpc stopped");
                    }
                    bcmIpHost->dhcpcPid = 0;
                }
                // iptables rules of ipOptions
                rutIpt_omciIpHostRules(MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA, ipOption, ifName);

                // save BCM_IpHostConfigDataObject configurations if any changes
                // it will trigger actions on IpHostConfigDataObject to call
                // rutOmci_activateIpInterface for configuring static information
                if (newName == TRUE || (UINT32) oldPid != bcmIpHost->dhcpcPid)
                    cmsObj_set(bcmIpHost, &iidStack);
            }
            cmsObj_free((void **) &bcmIpHost);
         }
         cmsLck_releaseLock();
    }

    return cmsResult;
}

static CmsRet activateIpv4Interface(IpHostConfigDataObject *ipHost)
{
    char cmd[BUFLEN_1024];
    char ipAddr[CMS_IPADDR_LENGTH];
    char mask[CMS_IPADDR_LENGTH];
    char gtwy[CMS_IPADDR_LENGTH];
    char bCast[CMS_IPADDR_LENGTH];
    char subnet[CMS_IPADDR_LENGTH];
    struct in_addr inAddr;
    CmsRet ret = CMSRET_SUCCESS;
#ifdef OMCI_TR69_DUAL_STACK
    CmsMsgHeader *msgHdr;
    char *dataPtr;
    UINT32 dataLen=0;
#endif

    memset(ipAddr, 0, sizeof(ipAddr));
    memset(mask, 0, sizeof(mask));
    memset(gtwy, 0, sizeof(mask));
    memset(bCast, 0, sizeof(bCast));
    memset(subnet, 0, sizeof(subnet));

    inAddr.s_addr = htonl(ipHost->currentAddress);
    cmsUtl_strncpy(ipAddr, inet_ntoa(inAddr), CMS_IPADDR_LENGTH);
    inAddr.s_addr = htonl(ipHost->currentMask);
    cmsUtl_strncpy(mask, inet_ntoa(inAddr), CMS_IPADDR_LENGTH);
    inAddr.s_addr = htonl(ipHost->currentGateway);
    cmsUtl_strncpy(gtwy, inet_ntoa(inAddr), CMS_IPADDR_LENGTH);

    if ((rutWan_getBcastStrAndSubnetFromIpAndMask(ipAddr,
                                                  mask,
                                                  bCast,
                                                  subnet)) != CMSRET_SUCCESS)
    {
        ret = CMSRET_INVALID_ARGUMENTS;
    }
    else
    {
        UBOOL8 found = FALSE;
        char dns1Addr[CMS_IPADDR_LENGTH];
        char dns2Addr[CMS_IPADDR_LENGTH];
        struct in_addr inAddr;
        BCM_IpHostConfigDataObject *bcmIpHost = NULL;
        InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

        /* search BCM_IpHostConfigDataObject that has id matched with its Id */
        while ((!found) &&
               ((ret = cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA_EXT,
                                     &iidStack, (void **) &bcmIpHost))
                == CMSRET_SUCCESS))
        {
            found = (bcmIpHost->managedEntityId == ipHost->managedEntityId);
            if (found == TRUE && bcmIpHost->interfaceName != NULL)
            {
                /* setup IpHost interface: such as
                 * "ifconfig br12.4 10.6.33.165 netmask 255.255.255.192 broadcast 10.6.33.191 up"
                 * "route add -net 0.0.0.0 netmask 0.0.0.0 metric 1 gw 10.6.33.191 dev br12.4"
                 */
                snprintf(cmd, sizeof(cmd), "ifconfig %s %s netmask %s broadcast %s up 2>/dev/null",
                    bcmIpHost->interfaceName, ipAddr, mask, bCast);
                rut_doSystemAction("rut_gpon", cmd);
#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
                snprintf(cmd, sizeof(cmd), "ip rule add from %s table %d 2>/dev/null",
                         ipAddr, RT_TABLE_GPON_IPHOST);
                rut_doSystemAction("rut_gpon", cmd);
                snprintf(cmd, sizeof(cmd), "ip route replace default via %s table %d 2>/dev/null",
                         gtwy, RT_TABLE_GPON_IPHOST);
                rut_doSystemAction("rut_gpon", cmd);
#else
                snprintf(cmd, sizeof(cmd), "route add -net 0.0.0.0 netmask 0.0.0.0 metric 1 gw %s 2>/dev/null", gtwy);
                rut_doSystemAction("rut_gpon", cmd);
#endif
                snprintf(cmd, sizeof(cmd), "sendarp -s %s -d %s", "br0", bcmIpHost->interfaceName);
                rut_doSystemAction("rut_gpon", cmd);

                /* Add iptables rule of tcpUdp config data */
                rutOmci_configTcpUdpByIpHost(ipHost->managedEntityId, TRUE);

                /* Configure DNS information */
                memset(dns1Addr, 0 , CMS_IPADDR_LENGTH);
                memset(dns2Addr, 0 , CMS_IPADDR_LENGTH);

                if (ipHost->primaryDns != 0 && ipHost->secondaryDns != 0)
                {
                    inAddr.s_addr = htonl(ipHost->primaryDns);
                    cmsUtl_strncpy(dns1Addr, inet_ntoa(inAddr), CMS_IPADDR_LENGTH);
                    inAddr.s_addr = htonl(ipHost->secondaryDns);
                    cmsUtl_strncpy(dns2Addr, inet_ntoa(inAddr), CMS_IPADDR_LENGTH);
                }
                else if (ipHost->primaryDns != 0 && ipHost->secondaryDns == 0)
                {
                    inAddr.s_addr = htonl(ipHost->primaryDns);
                    cmsUtl_strncpy(dns1Addr, inet_ntoa(inAddr), CMS_IPADDR_LENGTH);
                }
                else if (ipHost->primaryDns == 0 && ipHost->secondaryDns != 0)
                {
                    inAddr.s_addr = htonl(ipHost->secondaryDns);
                    cmsUtl_strncpy(dns1Addr, inet_ntoa(inAddr), CMS_IPADDR_LENGTH);
                }
                rutOmci_configDns(dns1Addr, dns2Addr);

#ifdef OMCI_TR69_DUAL_STACK
                //send CMS_MSG_WAN_CONNECTION_UP to smd of static ipHost
                if ((ipHost->ipOptions & 0x0001) == 0)
                {
                    msgHdr = (CmsMsgHeader *)cmd;
                    memset(cmd, 0, sizeof(cmd));
                    dataLen = strlen(bcmIpHost->interfaceName)+1;
                    msgHdr->src = mdmLibCtx.eid;
                    msgHdr->dst = EID_SMD;
                    msgHdr->type = CMS_MSG_WAN_CONNECTION_UP;
                    msgHdr->flags_event = 1;
                    msgHdr->dataLength = dataLen;

                    dataPtr = (char *) (msgHdr+1);
                    strcpy(dataPtr, bcmIpHost->interfaceName);
                    if (cmsMsg_send(mdmLibCtx.msgHandle, msgHdr) != CMSRET_SUCCESS)
                    {
                        cmsLog_error("could not send out CMS_MSG_WAN_CONNECTION_UP event msg");
                    }
                    else
                    {
                        cmsLog_debug("Send out CMS_MSG_WAN_CONNECTION_UP event msg.");
                    }
                }
#endif
#ifdef DMP_X_ITU_ORG_VOICE_SIP_1
                // Start voice stack by setting TR104 BoundIfName and BoundIfAddress.
                setVoipBoundIfNameAddress(bcmIpHost->interfaceName, ipAddr);
#endif
            }
            cmsObj_free((void **) &bcmIpHost);
        }
    }

    return ret;
}

#ifdef SUPPORT_IPV6

CmsRet rutOmci_startDhcp6c(_BCM_Ipv6HostConfigDataObject *obj)
{
   UINT8 ipOption = 0;
   UBOOL8 found = FALSE;
   Ipv6HostConfigDataObject *ipv6Host = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 m_flag, o_flag;
   UBOOL8 ia_na=FALSE, info_only=FALSE;
   SINT32 pid = CMS_INVALID_PID;

   if (obj != NULL && obj->interfaceName != NULL)
   {
      /* search instance that has id matched with given meId */
      while ((!found) &&
             (cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA,
                             &iidStack, (void **) &ipv6Host) == CMSRET_SUCCESS))
      {
         found = (ipv6Host->managedEntityId == obj->managedEntityId);
         if (found == TRUE)
         {
            ipOption = ipv6Host->ipOptions;
         }
         cmsObj_free((void **) &ipv6Host);
      }

      cmsLog_debug("get ipOption<%x>", ipOption);

      m_flag = obj->MFlag;
      o_flag = obj->OFlag;

      // 5 possible cases:
      // RS=1&DHCP=0, M=1     ==> DHCP for all
      // RS=1&DHCP=0, M=0&O=1 ==> DHCP for info_only
      // RS=1&DHCP=0, M=0&O=0 ==> no DHCP
      // RS=0&DHCP=1          ==> DHCP for all
      // RS=1&DHCP=1          ==> DHCP for all
      if ((ipOption & 0x06) == 0x2)
      {
         if (m_flag)
            ia_na = info_only = TRUE;
         else if (o_flag)
            info_only = TRUE;
      }
      else if ((ipOption & 0x06) == 0x4)
         ia_na = info_only = TRUE;
      else if ((ipOption & 0x06) == 0x6)
         ia_na = info_only = TRUE;

      cmsLog_debug("ia_na<%d> info_only<%d> dhcppid<%d>",
                    ia_na, info_only, obj->dhcpcPid);

      // TODO: FIXME
      {
         rutIpt_setupFirewallForDHCPv6(TRUE, obj->interfaceName);
      }
      if (ia_na || info_only)
      {
         /* kill old DHCPv6 client if any */
         if (obj->dhcpcPid != 0)
         {
            UINT32 specificEid = MAKE_SPECIFIC_EID(obj->dhcpcPid, EID_DHCP6C);
            cmsLog_debug("stop dhcp6c pid=%d", obj->dhcpcPid);
            if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0) != CMSRET_SUCCESS)
            {
               cmsLog_error("failed to send msg to stop dhcp6c");
            }
            else
            {
               cmsLog_debug("dhcp6c stopped");
            }
            obj->dhcpcPid = 0;
         }

         // start DHCPv6 client
         // TODO: control of info_only??
         if ((pid = rutWan_restartDhcp6c(obj->interfaceName, ia_na,
                                         FALSE, FALSE, FALSE, FALSE)) == CMS_INVALID_PID)
         {
            cmsLog_error("failed to start dhcp6c on %s", obj->interfaceName);
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
//          bcmIpHost->connectionStatus = OMCI_CONN_CONNECTING;    // connecting
            obj->dhcpcPid = pid;
         }
         cmsLog_debug("starting dhcp6c, pid=%d on %s", pid, obj->interfaceName);
      }
      else if (!(ia_na || info_only) && obj->dhcpcPid != 0)
      {
         //dhcp6c should not be launched??
         UINT32 specificEid = MAKE_SPECIFIC_EID(obj->dhcpcPid, EID_DHCP6C);
         cmsLog_debug("stop dhcp6c pid=%d", obj->dhcpcPid);
         if (rut_sendMsgToSmd(CMS_MSG_STOP_APP, specificEid, NULL, 0) != CMSRET_SUCCESS)
         {
            cmsLog_error("failed to send msg to stop dhcp6c");
         }
         else
         {
            cmsLog_debug("dhcp6c stopped");
         }
         obj->dhcpcPid = 0;
      }

      return ret;
                // TODO:ip6tables rules of ipOptions
//                rutIpt_omciIpHostRules(MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA, ipOption, ifName);
   }
   else
   {
      return CMSRET_INVALID_ARGUMENTS;
   }
}

CmsRet rutOmci_configIpv6Host(UINT32 meId, char *ifName)
{
    UBOOL8 found = FALSE, newName = FALSE;
    BCM_Ipv6HostConfigDataObject *bcmIpv6Host = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet cmsResult = CMSRET_SUCCESS;

    if (ifName == NULL)
    {
        return CMSRET_INVALID_ARGUMENTS;
    }

    // Nested locks are allowed, but there must be an equal number of unlocks.
    cmsResult = cmsLck_acquireLockWithTimeout(6*MSECS_IN_SEC);

    // Test for locking success.
    if (cmsResult == CMSRET_SUCCESS)
    {
        found = FALSE;
        INIT_INSTANCE_ID_STACK(&iidStack);

        while ((!found) &&
               ((cmsResult = cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA_EXT,
                                     &iidStack, (void **) &bcmIpv6Host))
                == CMSRET_SUCCESS))
        {
            found = (bcmIpv6Host->managedEntityId == meId);
            // only free obj when not found
            if (found == TRUE)
            {
                if (bcmIpv6Host->interfaceName == NULL)
                    newName = TRUE;
                else if (strcmp(bcmIpv6Host->interfaceName, ifName) != 0)
                    newName = TRUE;
                if (newName == TRUE)
                    CMSMEM_REPLACE_STRING_FLAGS(bcmIpv6Host->interfaceName,
                                                ifName,
                                                mdmLibCtx.allocFlags);

                if (newName == TRUE)
                    cmsObj_set(bcmIpv6Host, &iidStack);
            }
            cmsObj_free((void **) &bcmIpv6Host);
         }
         cmsLck_releaseLock();
    }

    return cmsResult;
}

#if 0
static CmsRet activateIpv6Interface(Ipv6HostConfigDataObject *ipv6Host)
{
    CmsRet ret = CMSRET_SUCCESS;
#ifdef OMCI_TR69_DUAL_STACK
    CmsMsgHeader *msgHdr;
    char *dataPtr;
    UINT32 dataLen=0;
#endif

    UBOOL8 found = FALSE;
    BCM_Ipv6HostConfigDataObject *bcmIpv6Host = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

    /* search BCM_Ipv6HostConfigDataObject that has id matched with its Id */
    while ((!found) &&
           ((ret = cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA_EXT,
                                 &iidStack, (void **) &bcmIpv6Host))
            == CMSRET_SUCCESS))
    {
        found = (bcmIpv6Host->managedEntityId == ipv6Host->managedEntityId);
        if (found == TRUE && bcmIpv6Host->interfaceName != NULL)
        {
            char cmd[BUFLEN_1024];
	     char ipv6Address[CMS_IPADDR_LENGTH], defaultRouter[CMS_IPADDR_LENGTH];
	     char primaryDns[CMS_IPADDR_LENGTH], secondaryDns[CMS_IPADDR_LENGTH];
            UINT32 size = 0;
            UINT8 *buf = NULL;

            memset(cmd, 0, BUFLEN_1024);
            memset(ipv6Address, 0, CMS_IPADDR_LENGTH);
            memset(defaultRouter, 0, CMS_IPADDR_LENGTH);
            memset(primaryDns, 0, CMS_IPADDR_LENGTH);
            memset(secondaryDns, 0, CMS_IPADDR_LENGTH);

            // convert ipv6Address hexString (32 bytes) to struct in6_addr (16 bytes) buf
            cmsUtl_hexStringToBinaryBuf(ipv6Host->ipv6Address, &buf, &size);
            // struct in6_addr (16 bytes) buf to ipv6 string format (48 bytes)
            inet_ntop(AF_INET6, buf, ipv6Address, CMS_IPADDR_LENGTH);
            // free temporary memory
            cmsMem_free(buf);

            snprintf(cmd, sizeof(cmd), "ip -6 addr add %s/64 dev %s 2>/dev/null",
                ipv6Address, bcmIpv6Host->interfaceName);
            rut_doSystemAction("rut_gpon", cmd);

            // convert defaultRouter hexString (32 bytes) to struct in6_addr (16 bytes) buf
            cmsUtl_hexStringToBinaryBuf(ipv6Host->defaultRouter, &buf, &size);
            // struct in6_addr (16 bytes) buf to ipv6 string format (48 bytes)
            inet_ntop(AF_INET6, buf, defaultRouter, CMS_IPADDR_LENGTH);
            // free temporary memory
            cmsMem_free(buf);

#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
            snprintf(cmd, sizeof(cmd), "ip -6 rule add from %s table %d 2>/dev/null",
                ipv6Address, RT_TABLE_GPON_IPV6HOST);
            rut_doSystemAction("rut_gpon", cmd);

            snprintf(cmd, sizeof(cmd), "ip -6 route replace default via %s table %d 2>/dev/null",
                defaultRouter, RT_TABLE_GPON_IPV6HOST);
            rut_doSystemAction("rut_gpon", cmd);
#else
            snprintf(cmd, sizeof(cmd), "ip -6 route add default via %s dev %s 2>/dev/null",
                defaultRouter, bcmIpv6Host->interfaceName);
            rut_doSystemAction("rut_gpon", cmd);
#endif
            snprintf(cmd, sizeof(cmd), "sendarp -s %s -d %s", "br0", bcmIpv6Host->interfaceName);
            rut_doSystemAction("rut_gpon", cmd);

            /* Add iptables rule of tcpUdp config data */
            rutOmci_configTcpUdpByIpHost(ipv6Host->managedEntityId, TRUE);

            // convert primaryDns hexString (32 bytes) to struct in6_addr (16 bytes) buf
            cmsUtl_hexStringToBinaryBuf(ipv6Host->primaryDns, &buf, &size);
            // struct in6_addr (16 bytes) buf to ipv6 string format (48 bytes)
            inet_ntop(AF_INET6, buf, primaryDns, CMS_IPADDR_LENGTH);
            // free temporary memory
            cmsMem_free(buf);
            // convert secondaryDns hexString (32 bytes) to struct in6_addr (16 bytes) buf
            cmsUtl_hexStringToBinaryBuf(ipv6Host->secondaryDns, &buf, &size);
            // struct in6_addr (16 bytes) buf to ipv6 string format (48 bytes)
            inet_ntop(AF_INET6, buf, secondaryDns, CMS_IPADDR_LENGTH);
            // free temporary memory
            cmsMem_free(buf);

            /* Configure DNS information */
            rutOmci_configDns(primaryDns, secondaryDns);
#ifdef OMCI_TR69_DUAL_STACK
            //send CMS_MSG_WAN_CONNECTION_UP to smd of static ipHost
            if (ipv6Host->ipOptions != 0)
            {
                msgHdr = (CmsMsgHeader *)cmd;
                memset(cmd, 0, sizeof(cmd));
                dataLen = strlen(bcmIpv6Host->interfaceName)+1;
                msgHdr->src = mdmLibCtx.eid;;
                msgHdr->dst = EID_SMD;
                msgHdr->type = CMS_MSG_WAN_CONNECTION_UP;
                msgHdr->flags_event = 1;
                msgHdr->dataLength = dataLen;

                dataPtr = (char *) (msgHdr+1);
                strcpy(dataPtr, bcmIpv6Host->interfaceName);
                if (cmsMsg_send(mdmLibCtx.msgHandle, msgHdr) != CMSRET_SUCCESS)
                {
                    cmsLog_error("could not send out CMS_MSG_WAN_CONNECTION_UP event msg");
                }
                else
                {
                    cmsLog_debug("Send out CMS_MSG_WAN_CONNECTION_UP event msg.");
                }
            }
#endif
#ifdef DMP_X_ITU_ORG_VOICE_SIP_1
            // Start voice stack by setting TR104 BoundIfName and BoundIfAddress.
            setVoipBoundIfNameAddress(bcmIpv6Host->interfaceName, ipv6Address);
#endif
        }
        cmsObj_free((void **) &bcmIpv6Host);
    }

    return ret;
}
#endif

#endif    // SUPPORT_IPV6


//=======================  Public GPON functions ========================

CmsRet rutOmci_addAutoObject
    (UINT32 oid,
     UINT32 managedEntityId,
     UBOOL8 persistent)
{
    UINT32 flags = 0;
    void   *obj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    /* do nothing if instance is already created */
    if (rutOmci_isObjectExisted(oid, managedEntityId) == TRUE)
    {
        cmsLog_debug("oid=%d, id = %d is already existed",
                     oid, managedEntityId);
        return ret;
    }

    /* add new instance */
    if ((ret = cmsObj_addInstance(oid, &iidStack)) != CMSRET_SUCCESS)
    {
        cmsLog_error("could not create oid=%d, managedEntityId=%d, ret=%d",
                     oid, managedEntityId, ret);
        return ret;
    }

    cmsLog_debug("new oid=%d, managedEntityId=%d, created at %s",
                 oid, managedEntityId, cmsMdm_dumpIidStack(&iidStack));

     /* get new instance */
    if ((ret = cmsObj_get(oid, &iidStack, flags, (void **) &obj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("could not get oid=%d, managedEntityId=%d, ret=%d",
                     oid, managedEntityId, ret);
        return ret;
    }

    /* link auto object with created object
       MacBridgePortBridgeTableDataObject is used as generic type
       since it only has managedEntityId as its parameter */
    ((IpHostConfigDataObject *)obj)->managedEntityId = managedEntityId;

    /* set new instance */
    ret = cmsObj_set(obj, &iidStack);

    cmsObj_free((void **) &obj);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("could not set oid=%d, managedEntityId=%d, ret=%d",
                     oid, managedEntityId, ret);
        return ret;
    }

    /* set non-persistent instance */
    if (persistent == FALSE)
        ret = cmsObj_setNonpersistentInstance(oid, &iidStack);

    return ret;
}

CmsRet rutOmci_setAutoObject(UINT32 oid, UINT32 oldId, UINT32 newId)
{
    UBOOL8 found = FALSE;
    void   *obj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    /* search instance that has id matched with the old id */
    while ((!found) &&
          ((ret = cmsObj_getNext(oid, &iidStack, (void **) &obj)) == CMSRET_SUCCESS))
    {
        // MacBridgePortBridgeTableDataObject is used as generic type
        // since it only has managedEntityId as its parameter
        found = (((IpHostConfigDataObject *)obj)->managedEntityId == oldId);
        // only free obj when not found
        if (found == FALSE)
            cmsObj_free((void **) &obj);
    }

    /* change managed entity id of instance to new id */
    if (found == TRUE)
    {
        cmsLog_debug("set oid=%d, id=%d, at %s",
                     oid, oldId, cmsMdm_dumpIidStack(&iidStack));
        /* link auto object with created object
           MacBridgePortBridgeTableDataObject is used as generic type
           since it only has managedEntityId as its parameter */
        ((IpHostConfigDataObject *)obj)->managedEntityId = newId;
        ret = cmsObj_set(obj, &iidStack);
        cmsObj_free((void **) &obj);
        if (ret != CMSRET_SUCCESS)
            cmsLog_error("could not set oid=%d, id=%d, ret=%d",
                         oid, oldId, ret);
    }
    else
    {
        cmsLog_error("could not find oid=%d with managedEntityId=%d",
                     oid, oldId);
        ret = CMSRET_OBJECT_NOT_FOUND;
    }

    return ret;
}

CmsRet rutOmci_deleteAutoObject(UINT32 oid, UINT32 managedEntityId)
{
    UBOOL8 found = FALSE;
    void   *obj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    /* search instance that has id matched with the given id */
    while ((!found) &&
          ((ret = cmsObj_getNext(oid, &iidStack, (void **) &obj)) == CMSRET_SUCCESS))
    {
        // MacBridgePortBridgeTableDataObject is used as generic type
        // since it only has managedEntityId as its parameter
        found = (((IpHostConfigDataObject *)obj)->managedEntityId == managedEntityId);
        cmsObj_free((void **) &obj);
    }

    /* delete instance if found */
    if (found == TRUE)
    {
        cmsLog_debug("deleting oid=%d, id=%d, at %s", oid,
                     managedEntityId, cmsMdm_dumpIidStack(&iidStack));
        ret = cmsObj_deleteInstance(oid, &iidStack);
    }
    else
    {
        // reduce error level to notice level when object cannot be found
        cmsLog_notice("could not find oid=%d with managedEntityId=%d",
                      oid, managedEntityId);
        ret = CMSRET_SUCCESS;
    }

    return ret;
}

CmsRet rutOmci_getIfNameByMeId(UINT32 meId, char *ifName)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack1;
   UBOOL8 found = FALSE;
   _BCM_Ipv6HostConfigDataObject *obj = NULL;

   INIT_INSTANCE_ID_STACK(&iidStack1);
   /* search instance that has id matched with the old id */
   while ((!found) &&
          (cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA_EXT,
                          &iidStack1, (void **) &obj) == CMSRET_SUCCESS))
   {
      found = (obj->managedEntityId == meId);

      if (found == TRUE)
      {
         if (obj->interfaceName != NULL)
         {
            cmsUtl_strncpy(ifName, obj->interfaceName, BUFLEN_32);
         }
         else
         {
            ret = CMSRET_INVALID_PARAM_VALUE;
         }
      }

      cmsObj_free((void **) &obj);
   }

   if (found == FALSE)
   {
      cmsLog_error("could not find BC_IPV6_HOST_OBJ with meID %d", meId);
      ret = CMSRET_OBJECT_NOT_FOUND;
   }

   return ret;
}

CmsRet rutOmci_getIfNameFromBcmIpv6Obj(UINT32 oid, const InstanceIdStack * iidStack, char *ifName)
{
   UINT32 meId;
   InstanceIdStack iidStack1;
   Ipv6HostConfigDataObject *ipv6Host = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("enter oid<%d>", oid);

   ifName[0] = '\0';
   iidStack1 = *iidStack;
   if (cmsObj_getAncestorFlags(MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA, oid,
                               &iidStack1, OGF_NO_VALUE_UPDATE,
                               (void **) &ipv6Host) != CMSRET_SUCCESS)
   {
      cmsLog_error("cannot get ipv6hostObj");
      return CMSRET_MDM_TREE_ERROR;
   }

   meId = ipv6Host->managedEntityId;
   cmsObj_free((void **) &ipv6Host);

   ret = rutOmci_getIfNameByMeId(meId, ifName);

   cmsLog_debug("exit ifName<%s>", ifName);

   return ret;
}

CmsRet rutOmci_configDns
    (const char *dns1, const char *dns2)
{
    CmsRet setupResult = CMSRET_INVALID_ARGUMENTS;
    char dnsServers[BUFLEN_1024];
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    NetworkConfigObject *networkCfg=NULL;

    if ((dns1 == NULL) || (dns2 == NULL))
    {
        cmsLog_error("NULL pointer");
        return setupResult;
    }

    if ((*dns1 == '\0') && (*dns2 == '\0'))
    {
        cmsLog_error("Both params strings are empty");
        return setupResult;
    }

    snprintf(dnsServers, sizeof(dnsServers), "%s,%s", dns1, dns2);

    if ((setupResult = cmsObj_get(MDMOID_NETWORK_CONFIG, &iidStack, 0, (void **) &networkCfg)) == CMSRET_SUCCESS)
    {
        CMSMEM_FREE_BUF_AND_NULL_PTR(networkCfg->DNSIfName);
        CMSMEM_REPLACE_STRING_FLAGS(networkCfg->DNSServers, dnsServers, mdmLibCtx.allocFlags);

        if ((setupResult = cmsObj_set(networkCfg, &iidStack)) != CMSRET_SUCCESS)
            cmsLog_error("Could not set NETWORK_CONFIG, ret=%d", setupResult);
        else
            cmsLog_debug("Dns set OK: dnsServers=%s", dnsServers);

        cmsObj_free((void **) &networkCfg);
    }
    else
        cmsLog_error("Could not get NETWORK_CONFIG, ret=%d", setupResult);

    return setupResult;
}

CmsRet rutOmci_activateIpInterface
    (UINT32 oid, void *obj)
{
    CmsRet ret = CMSRET_INVALID_ARGUMENTS;

#ifdef DMP_X_BROADCOM_COM_IPV6_1
    if (oid == MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA)
        ret = activateIpv4Interface((IpHostConfigDataObject *)obj);
//    else if (oid == MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA)
//        ret = activateIpv6Interface((Ipv6HostConfigDataObject *)obj);
#else
    if (oid == MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA)
        ret = activateIpv4Interface((IpHostConfigDataObject *)obj);
#endif    // DMP_X_BROADCOM_COM_IPV6_1

    return ret;
}

void rutOmci_configTcpUdpByIpHost(UINT32 meId, UBOOL8 add __attribute__((unused)))
{
    UBOOL8 hasTcpUdp = FALSE;
    TcpUdpConfigDataObject *tcpUdp = NULL;
    InstanceIdStack tcpUdpIidStack = EMPTY_INSTANCE_ID_STACK;

    // remove rule of tcpUdp config data
    while ((!hasTcpUdp) &&
           (cmsObj_getNext(MDMOID_BCM_OMCI_RTD_TCP_UDP_CONFIG_DATA,
                                 &tcpUdpIidStack, (void **) &tcpUdp)
             == CMSRET_SUCCESS))
    {
        hasTcpUdp = (tcpUdp->ipHostPointer == meId);
        if (hasTcpUdp == TRUE)
        {
            rutOmci_configTcpUdp(tcpUdp, FALSE);
        }
        cmsObj_free((void **) &tcpUdp);
    }
}

void rutOmci_configTcpUdp(const TcpUdpConfigDataObject *tcpUdp, UBOOL8 add __attribute__((unused)))
{
    UBOOL8 hasIpHost = FALSE;
    UBOOL8 hasBcIpHost = FALSE;
    Ipv6HostConfigDataObject* ipv6Host = NULL;
    BCM_Ipv6HostConfigDataObject* bcmIpv6Host = NULL;
    IpHostConfigDataObject* ipHost = NULL;
    BCM_IpHostConfigDataObject* bcmIpHost = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    char cmd[BUFLEN_1024];

    // Search for specified Ipv6HostConfigDataObject
    while ((hasIpHost == FALSE) &&
           (cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA,
                           &iidStack, (void**)&ipv6Host)
            == CMSRET_SUCCESS))
    {
        // Set local object found flag based on match between this TcpUdpConfigDataObject and MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA.
        hasIpHost = (tcpUdp->ipHostPointer == ipv6Host->managedEntityId);

        // Test if this MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA ME matches TcpUdpConfigDataObject pointer.
        if (hasIpHost == TRUE)
        {
            INIT_INSTANCE_ID_STACK(&iidStack);
            /* search BCM_Ipv6HostConfigDataObject that has id matched with its Id */
            while ((hasBcIpHost == FALSE) &&
                   (cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA_EXT,
                                   &iidStack, (void**)&bcmIpv6Host)
                    == CMSRET_SUCCESS))
            {
                // Set local object found flag based on match between this TcpUdpConfigDataObject and MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA.
                hasBcIpHost = (bcmIpv6Host->managedEntityId == ipv6Host->managedEntityId);

                // Test if this MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA_EXT ME found.
                if ((hasBcIpHost == TRUE) && (bcmIpv6Host->interfaceName != NULL))
                {
                    /* make sure that all the required modules for qos support are loaded */
                    rutIpt_qosLoadModule();
                    snprintf(cmd, sizeof(cmd), "iptables -w -t mangle -o %s -%c POSTROUTING -p %d --dport %d -j DSCP --set-dscp 0x%x 2>/dev/null",
                        bcmIpv6Host->interfaceName, add ? 'A':'D', tcpUdp->protocol, tcpUdp->portId, tcpUdp->tos >> 2);
                    rut_doSystemAction("rut_gpon", cmd);
                }
                // Release MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA_EXT record.
                cmsObj_free((void**)&bcmIpv6Host);
            }
        }
        // Release MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA record.
        cmsObj_free((void**)&ipv6Host);
    }

    INIT_INSTANCE_ID_STACK(&iidStack);
    // Search for specified IpHostConfigDataObject
    while ((hasIpHost == FALSE) &&
           (cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA,
                           &iidStack, (void**)&ipHost)
            == CMSRET_SUCCESS))
    {
        // Set local object found flag based on match between this TcpUdpConfigDataObject and MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA.
        hasIpHost = (tcpUdp->ipHostPointer == ipHost->managedEntityId);

        // Test if this MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA ME matches TcpUdpConfigDataObject pointer.
        if (hasIpHost == TRUE)
        {
            INIT_INSTANCE_ID_STACK(&iidStack);
            /* search BCM_IpHostConfigDataObject that has id matched with its Id */
            while ((hasBcIpHost == FALSE) &&
                   (cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA_EXT,
                                   &iidStack, (void**)&bcmIpHost)
                    == CMSRET_SUCCESS))
            {
                // Set local object found flag based on match between this TcpUdpConfigDataObject and MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA.
                hasBcIpHost = (bcmIpHost->managedEntityId == ipHost->managedEntityId);

                // Test if this MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA_EXT ME found.
                if ((hasBcIpHost == TRUE) && (bcmIpHost->interfaceName != NULL))
                {
                    /* make sure that all the required modules for qos support are loaded */
                    rutIpt_qosLoadModule();
                    snprintf(cmd, sizeof(cmd), "iptables -w -t mangle -o %s -%c POSTROUTING -p %d --dport %d -j DSCP --set-dscp 0x%x 2>/dev/null",
                        bcmIpHost->interfaceName, add ? 'A':'D', tcpUdp->protocol, tcpUdp->portId, tcpUdp->tos >> 2);
                    rut_doSystemAction("rut_gpon", cmd);
                }
                // Release MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA_EXT record.
                cmsObj_free((void**)&bcmIpHost);
            }
        }
        // Release MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA record.
        cmsObj_free((void**)&ipHost);
    }
}

CmsRet rutOmci_refreshDnsServer(UINT32 dns1, UINT32 dns2, UINT8 *dns6)
{
   CmsRet ret = CMSRET_SUCCESS;
   char cmdStr[BUFLEN_128];
   char tmp_file[BUFLEN_32];
   int tmp_fd;
   int rc;
   FILE *tmp_fp = NULL;
   struct in_addr ip;

   cmsLog_debug("entered");

   /*
    * create /var/fyi/sys/gpon_dns for dns proxy
    * /etc/resolv.conf symbolic link to /var/fyi/sys/dns
    * set name server of /etc/resolve as 127.0.0.1
    * then host dns query be redirected to dns proxy
    * Count query errors of dns proxy for ipHost PM
    */
   if (!cmsFil_isFilePresent(REAL_DNS_FYI_FILENAME))
   {
      rc = mkdir("/var/fyi", 0777);
      if ((rc < 0) && (errno != EEXIST))
      {
         cmsLog_error("mkdir() failed, %s (%d)", strerror(errno), errno);
         return CMSRET_INTERNAL_ERROR;
      }

      rc = mkdir("/var/fyi/sys", 0777);
      if ((rc < 0) && (errno != EEXIST))
      {
         cmsLog_error("mkdir() failed, %s (%d)", strerror(errno), errno);
         return CMSRET_INTERNAL_ERROR;
      }
   }

   /*
    * Being ultra paranoid here.  Some system app could be trying to read this
    * file while we are updating it, so the standard practice is to write to
    * a tmp file first, then do a rename.
    */
   strncpy(tmp_file, "/var/fyi/sys/dnsXXXXXX", sizeof(tmp_file));
   tmp_fd = mkstemp(tmp_file);
   if (tmp_fd == -1)
   {
      cmsLog_error("could not open tmp file %s", tmp_file);
      return CMSRET_INTERNAL_ERROR;
   }

   tmp_fp = fdopen(tmp_fd, "w+");
   if (tmp_fp == NULL)
   {
      cmsLog_error("could not open file stream for %s", tmp_file);
      close(tmp_fd);
      return CMSRET_INTERNAL_ERROR;
   }

   if (dns1)
   {
       ip.s_addr = htonl(dns1);
       snprintf(cmdStr, sizeof(cmdStr), "nameserver %s\n", inet_ntoa(ip));
       fputs(cmdStr, tmp_fp);
   }
   if (dns2)
   {
       ip.s_addr = htonl(dns2);
       snprintf(cmdStr, sizeof(cmdStr), "nameserver %s\n", inet_ntoa(ip));
       fputs(cmdStr, tmp_fp);
   }

   if (dns6)
   {
       char ipv6addr[CMS_IPADDR_LENGTH];

       inet_ntop(AF_INET6, dns6, ipv6addr, CMS_IPADDR_LENGTH);

       snprintf(cmdStr, sizeof(cmdStr), "nameserver %s\n", ipv6addr);
       fputs(cmdStr, tmp_fp);
   }

   fclose(tmp_fp);
   rc = rename(tmp_file, DNS_FYI_FILENAME);
   if (rc < 0)
   {
       cmsLog_error("Unable to rename file %s: %s (%d)",
         tmp_file, strerror(errno), errno);
       ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
}

#if defined(OMCI_TR69_DUAL_STACK)

CmsRet rutOmci_getIpHostAddress(char *ifname, char **ipAddress, UBOOL8 *isIPv4)
{
   InstanceIdStack iidIpHost = EMPTY_INSTANCE_ID_STACK;
   IpHostConfigDataObject *ipHost = NULL;
   UBOOL8 foundIpHost = FALSE;
   UINT32 ipHostMeId = 0;
   CmsRet ret;


   ret = rut_isGponIpHostInterface(ifname, &ipHostMeId);
   if (ret != CMSRET_SUCCESS)
       return ret;

   // Nested locks are allowed, but there must be an equal number of unlocks.
   ret = cmsLck_acquireLockWithTimeout(6*MSECS_IN_SEC);
   if (ret != CMSRET_SUCCESS)
      return ret;

   ret = CMSRET_OBJECT_NOT_FOUND;

#ifdef DMP_X_BROADCOM_COM_IPV6_1
   Ipv6HostConfigDataObject *ipv6Host = NULL;
   while (ipHostMeId && !foundIpHost &&
          cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA,
                          &iidIpHost, (void **) &ipv6Host) == CMSRET_SUCCESS)
   {
       foundIpHost = (ipv6Host->managedEntityId == ipHostMeId);
       if (foundIpHost && ipv6Host->currentAddressTable)
       {
            char ipv6Address[CMS_IPADDR_LENGTH]={0};
            UINT32 size = 0;
            UINT8 *buf = NULL;

            // convert ipv6Address hexString (32 bytes) to struct in6_addr (16 bytes) buf
            cmsUtl_hexStringToBinaryBuf(ipv6Host->currentAddressTable, &buf, &size);
            // struct in6_addr (16 bytes) buf to ipv6 string format (48 bytes)
            inet_ntop(AF_INET6, buf, ipv6Address, sizeof(ipv6Address));
            // free temporary memory
            cmsMem_free(buf);

           *ipAddress = cmsMem_strdupFlags(ipv6Address, mdmLibCtx.allocFlags);
           *isIPv4 = FALSE;
           ret = CMSRET_SUCCESS;
       }
       cmsObj_free((void **) &ipv6Host);
   }

    INIT_INSTANCE_ID_STACK(&iidIpHost);
#endif    // DMP_X_BROADCOM_COM_IPV6_1

   while (ipHostMeId && !foundIpHost &&
          cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA,
                          &iidIpHost, (void **) &ipHost) == CMSRET_SUCCESS)
   {
       foundIpHost = (ipHost->managedEntityId == ipHostMeId);
       if (foundIpHost && ipHost->currentAddress)
       {
           char ipAddr[CMS_IPADDR_LENGTH]={0};
           struct in_addr inAddr;

           inAddr.s_addr = htonl(ipHost->currentAddress);
           cmsUtl_strncpy(ipAddr, inet_ntoa(inAddr), CMS_IPADDR_LENGTH);
           *ipAddress = cmsMem_strdupFlags(ipAddr, mdmLibCtx.allocFlags);
           *isIPv4 = TRUE;
           ret = CMSRET_SUCCESS;
       }
       cmsObj_free((void **) &ipHost);
   }

   cmsLck_releaseLock();

   return ret;
}
#endif  // defined(OMCI_TR69_DUAL_STACK)

#endif /* DMP_X_ITU_ORG_GPON_1 */
