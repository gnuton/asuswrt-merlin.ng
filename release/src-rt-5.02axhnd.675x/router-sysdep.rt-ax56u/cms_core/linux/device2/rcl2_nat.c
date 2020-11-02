/***********************************************************************
 *
 *  Copyright (c) 2009-2013  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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


#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_NAT_1

/** all the TR181 NAT objects will go into this file,
 */

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"

static void modifyNatIntfSettingNumEntry(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_NAT,
                        MDMOID_DEV2_NAT_INTF_SETTING,
                        iidStack,
                        delta);
}


static void modifyNatPortMappingNumEntry(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_NAT,
                        MDMOID_DEV2_NAT_PORT_MAPPING,
                        iidStack,
                        delta);
}


CmsRet rcl_dev2NatObject( _Dev2NatObject *newObj __attribute((unused)),
                const _Dev2NatObject *currObj __attribute((unused)),
                const InstanceIdStack *iidStack __attribute((unused)),
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   /* TODO: not implemented yet */

   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2NatIntfSettingObject( _Dev2NatIntfSettingObject *newObj __attribute((unused)),
                const _Dev2NatIntfSettingObject *currObj __attribute((unused)),
                const InstanceIdStack *iidStack,
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   if (ADD_NEW(newObj, currObj))
   {
      modifyNatIntfSettingNumEntry(iidStack,1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      modifyNatIntfSettingNumEntry(iidStack, -1);
   }
   
   return CMSRET_SUCCESS;
}


CmsRet rutIpt_vrtsrvCfg_dev2(const _Dev2NatPortMappingObject *portmapObj, const UBOOL8 add, char *portmap_ifName);

CmsRet rcl_dev2NatPortMappingObject( _Dev2NatPortMappingObject *newObj __attribute((unused)),
                const _Dev2NatPortMappingObject *currObj __attribute((unused)),
                const InstanceIdStack *iidStack __attribute((unused)),
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack wanipIidStack = *iidStack;

   /* add and enable virtual server, or enable existing virtual server */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* The end port number should be greater than the start port number when specifying a port range. */
      if(newObj->externalPortEndRange < newObj->externalPort)
      {
          cmsLog_debug("eEnd < eStart. Removing eEnd.");
          newObj->externalPortEndRange = newObj->externalPort;
      }

      if(newObj->X_BROADCOM_COM_InternalPortEndRange < newObj->internalPort)
      {
          cmsLog_debug("iEnd < iStart. Removing iEnd.");
          newObj->X_BROADCOM_COM_InternalPortEndRange = newObj->internalPort;
      }

      cmsLog_debug("Adding virtual server entry: srvName=%s, srvAddr=%s, proto=%s, eStart:eEnd=%u:%u, iStart:iEnd=%u:%u", 
            newObj->description, newObj->internalClient, newObj->protocol, newObj->externalPort, 
            newObj->externalPortEndRange, newObj->internalPort, newObj->X_BROADCOM_COM_InternalPortEndRange);

      if (cmsUtl_isValidIpAddress(AF_INET, newObj->internalClient) == FALSE || !strcmp(newObj->internalClient, "0.0.0.0"))
      {
         cmsLog_error("Invalid virtual server IP address");
         return CMSRET_INVALID_ARGUMENTS;
      }

      /*If remoteHost is configured check if its valid Ip Address 
       * remoteHost in an optional feild so NULL or null string("\0") are valid */
      if((newObj->remoteHost != NULL) && (newObj->remoteHost[0] != '\0'))
      {
         if (cmsUtl_isValidIpAddress(AF_INET, newObj->remoteHost) == FALSE || !strcmp(newObj->remoteHost, "0.0.0.0"))
         {
            cmsLog_error("Invalid remote host IP address");
            return CMSRET_INVALID_ARGUMENTS;
         }
      }

      /* TODO: Should PCP of DSLite do iptables here?? */
//      if ((wan_ip_con->X_BROADCOM_COM_PCPMode != PCP_MODE_DSLITE) || 
//          !cmsUtl_strstr(newObj->X_BROADCOM_COM_AppName, "upnp"))
//      {
         ret = rutIpt_vrtsrvCfg_dev2(newObj,TRUE,NULL);

         if(newObj)
             CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_ENABLED,mdmLibCtx.allocFlags);
//      }
   }

   /* remove ifc, or disable ifc*/
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting virtual server entry: srvName=%s, srvAddr=%s, proto=%s, eStart:eEnd=%u:%u, iStart:iEnd=%u:%u",
                   currObj->description, currObj->internalClient, currObj->protocol, currObj->externalPort,
                   currObj->externalPortEndRange, currObj->internalPort, currObj->X_BROADCOM_COM_InternalPortEndRange);
      if (currObj->description != NULL)
      {
         /* TODO: Should PCP of DSLite do iptables here?? */

//         if ((wan_ip_con->X_BROADCOM_COM_PCPMode != PCP_MODE_DSLITE) || 
//             !cmsUtl_strstr(currObj->X_BROADCOM_COM_AppName, "upnp"))
//         {
         ret = rutIpt_vrtsrvCfg_dev2(currObj,FALSE,NULL);
         if(newObj)
             CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DISABLED,mdmLibCtx.allocFlags);
//         }
      }
   }

   /*
    * Update PortMappingNumberOfEntries on successful add or delete.
    */
   if (ret == CMSRET_SUCCESS)
   {
      if (ADD_NEW(newObj, currObj))
      {
         modifyNatPortMappingNumEntry(iidStack,1);
      }
      else if (DELETE_EXISTING(newObj, currObj))
      {
         modifyNatPortMappingNumEntry(iidStack,-1);
      }
   }

   return ret;
}


CmsRet rcl_dev2NatPortTriggeringObject( _Dev2NatPortTriggeringObject *newObj __attribute((unused)),
                const _Dev2NatPortTriggeringObject *currObj __attribute((unused)),
                const InstanceIdStack *iidStack __attribute((unused)),
                char **errorParam __attribute((unused)),
                CmsRet *errorCode __attribute((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2NatIntfSettingObject *natIntfObj=NULL;   
   InstanceIdStack parentIidStack = *iidStack;
   char ifName[BUFLEN_32], cmd[BUFLEN_64];
   int trigger_proto, open_proto;

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if (cmsObj_getAncestor(MDMOID_DEV2_NAT_INTF_SETTING, MDMOID_DEV2_NAT_PORT_TRIGGERING, 	
                                                     &parentIidStack, (void **) &natIntfObj) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get natIntfObj. ret=%d", ret);
      return CMSRET_INTERNAL_ERROR;
   }

   ret = qdmIntf_getIntfnameFromFullPathLocked_dev2(natIntfObj->interface, ifName, sizeof(ifName));
   cmsObj_free((void **) &natIntfObj);

   rutIpt_insertPortTriggeringModules_dev2();

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

#endif  /* DMP_DEVICE2_NAT_1 */

#endif /* DMP_DEVICE2_BASELINE_1 */


