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

#include "cms_util.h"
#include "cms_obj.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_virtualserver.h"

#ifdef DMP_BASELINE_1
/*
 * The PPPConnPortMappingObject and IPConnPortMappingObject are attached
 * to the TR98 data model, so they can only be used when IGD is the
 * root data model.
 */

CmsRet rcl_wanPppConnPortmappingObject( _WanPppConnPortmappingObject *newObj __attribute__((unused)),
                const _WanPppConnPortmappingObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   WanPppConnObject *wan_ppp_con = NULL;
   InstanceIdStack wanpppIidStack = *iidStack;

   if (cmsObj_getAncestor(MDMOID_WAN_PPP_CONN, MDMOID_WAN_PPP_CONN_PORTMAPPING, 
                          &wanpppIidStack, (void **) &wan_ppp_con) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get _WanPppConnObject. ret=%d", ret);
      return CMSRET_INTERNAL_ERROR;
   }
   
   /* add and enable virtual server, or enable existing virtual server */
   if (VRTSRV_ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* The end port number should be greater than the start port number when specifying a port range. */
      if(newObj->X_BROADCOM_COM_ExternalPortEnd < newObj->externalPort)
      {
          cmsLog_debug("eEnd < eStart. Removing eEnd.");
          newObj->X_BROADCOM_COM_ExternalPortEnd = newObj->externalPort;
      }

      if(newObj->X_BROADCOM_COM_InternalPortEnd < newObj->internalPort)
      {
          cmsLog_debug("iEnd < iStart. Removing iEnd.");
          newObj->X_BROADCOM_COM_InternalPortEnd = newObj->internalPort;
      }

      cmsLog_debug("Adding virtual server entry: srvName=%s, srvAddr=%s, proto=%s, eStart:eEnd=%u:%u, iStart:iEnd=%u:%u", 
            newObj->portMappingDescription, newObj->internalClient, newObj->portMappingProtocol, newObj->externalPort, 
            newObj->X_BROADCOM_COM_ExternalPortEnd, newObj->internalPort, newObj->X_BROADCOM_COM_InternalPortEnd);

      if (cmsUtl_isValidIpAddress(AF_INET, newObj->internalClient) == FALSE || !strcmp(newObj->internalClient, "0.0.0.0"))
      {
         cmsLog_error("Invalid virtual server IP address");
         cmsObj_free((void **) &wan_ppp_con);
         return CMSRET_INVALID_ARGUMENTS;		
      }
      /*If remoteHost is configured check if its valid Ip Address 
       * remoteHost in an optional feild so NULL or null string("\0") are valid */
      if((newObj->remoteHost != NULL) &&(newObj->remoteHost[0] != '\0'))
      {		
         if (cmsUtl_isValidIpAddress(AF_INET, newObj->remoteHost) == FALSE || !strcmp(newObj->remoteHost, "0.0.0.0"))
         {
            cmsLog_error("Invalid remote host IP address");
            cmsObj_free((void **) &wan_ppp_con);
            return CMSRET_INVALID_ARGUMENTS;		
         }
      }

      /* TODO: Should PCP of DSLite do iptables here?? */
      if ((wan_ppp_con->X_BROADCOM_COM_PCPMode != PCP_MODE_DSLITE) || 
          !cmsUtl_strstr(newObj->X_BROADCOM_COM_AppName, "upnp"))
      {
         ret = rutIpt_vrtsrvCfg(NULL, wan_ppp_con, newObj, NULL, TRUE);
      }
   }

   /* remove ifc, or disable ifc*/
   else if (VRTSRV_DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting virtual server entry: srvName=%s, srvAddr=%s, proto=%s, eStart:eEnd=%u:%u, iStart:iEnd=%u:%u", 
	  	                   currObj->portMappingDescription, currObj->internalClient, currObj->portMappingProtocol, currObj->externalPort, 
	  	                   currObj->X_BROADCOM_COM_ExternalPortEnd, currObj->internalPort, currObj->X_BROADCOM_COM_InternalPortEnd);
      if (currObj->portMappingDescription != NULL)
      {
         /* TODO: Should PCP of DSLite do iptables here?? */
         if ((wan_ppp_con->X_BROADCOM_COM_PCPMode != PCP_MODE_DSLITE) || 
             !cmsUtl_strstr(currObj->X_BROADCOM_COM_AppName, "upnp"))
         {
            ret = rutIpt_vrtsrvCfg(NULL, wan_ppp_con, currObj, NULL, FALSE);
         }
      }
   }   
   
   cmsObj_free((void **) &wan_ppp_con);


   /*
    * Update PortMappingNumberOfEntries on successful add or delete.
    */
   if (ret == CMSRET_SUCCESS)
   {
      if (ADD_NEW(newObj, currObj))
      {
         rut_modifyNumPppPortMapping(iidStack, 1);
      }
      else if (DELETE_EXISTING(newObj, currObj))
      {
         rut_modifyNumPppPortMapping(iidStack, -1);
      }
   }

   return ret;
}


CmsRet rcl_wanIpConnPortmappingObject( _WanIpConnPortmappingObject *newObj,
                const _WanIpConnPortmappingObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   WanIpConnObject *wan_ip_con = NULL;
   InstanceIdStack wanipIidStack = *iidStack;

   if (cmsObj_getAncestor(MDMOID_WAN_IP_CONN, MDMOID_WAN_IP_CONN_PORTMAPPING, 
                                                     &wanipIidStack, (void **) &wan_ip_con) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get _WanIpConnObject. ret=%d", ret);
      return CMSRET_INTERNAL_ERROR;
   }
   
   /* add and enable virtual server, or enable existing virtual server */
   if (VRTSRV_ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* The end port number should be greater than the start port number when specifying a port range. */
      if(newObj->X_BROADCOM_COM_ExternalPortEnd < newObj->externalPort)
      {
          cmsLog_debug("eEnd < eStart. Removing eEnd.");
          newObj->X_BROADCOM_COM_ExternalPortEnd = newObj->externalPort;
      }

      if(newObj->X_BROADCOM_COM_InternalPortEnd < newObj->internalPort)
      {
          cmsLog_debug("iEnd < iStart. Removing iEnd.");
          newObj->X_BROADCOM_COM_InternalPortEnd = newObj->internalPort;
      }

      cmsLog_debug("Adding virtual server entry: srvName=%s, srvAddr=%s, proto=%s, eStart:eEnd=%u:%u, iStart:iEnd=%u:%u", 
            newObj->portMappingDescription, newObj->internalClient, newObj->portMappingProtocol, newObj->externalPort, 
            newObj->X_BROADCOM_COM_ExternalPortEnd, newObj->internalPort, newObj->X_BROADCOM_COM_InternalPortEnd);

      if (cmsUtl_isValidIpAddress(AF_INET, newObj->internalClient) == FALSE || !strcmp(newObj->internalClient, "0.0.0.0"))
      {
         cmsLog_error("Invalid virtual server IP address");
         cmsObj_free((void **) &wan_ip_con);
         return CMSRET_INVALID_ARGUMENTS;		
      }

      /*If remoteHost is configured check if its valid Ip Address 
       * remoteHost in an optional feild so NULL or null string("\0") are valid */
      if((newObj->remoteHost != NULL) && (newObj->remoteHost[0] != '\0'))
      {		
         if (cmsUtl_isValidIpAddress(AF_INET, newObj->remoteHost) == FALSE || !strcmp(newObj->remoteHost, "0.0.0.0"))
         {
            cmsLog_error("Invalid remote host IP address");
            cmsObj_free((void **) &wan_ip_con);
            return CMSRET_INVALID_ARGUMENTS;		
         }
      }

      /* TODO: Should PCP of DSLite do iptables here?? */
      if ((wan_ip_con->X_BROADCOM_COM_PCPMode != PCP_MODE_DSLITE) || 
          !cmsUtl_strstr(newObj->X_BROADCOM_COM_AppName, "upnp"))
      {
         ret = rutIpt_vrtsrvCfg(wan_ip_con, NULL, NULL, newObj, TRUE);
      }
   }

   /* remove ifc, or disable ifc*/
   else if (VRTSRV_DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {

      cmsLog_debug("Deleting virtual server entry: srvName=%s, srvAddr=%s, proto=%s, eStart:eEnd=%u:%u, iStart:iEnd=%u:%u", 
	  	                   currObj->portMappingDescription, currObj->internalClient, currObj->portMappingProtocol, currObj->externalPort, 
	  	                   currObj->X_BROADCOM_COM_ExternalPortEnd, currObj->internalPort, currObj->X_BROADCOM_COM_InternalPortEnd);
      if (currObj->portMappingDescription != NULL)
      {
         /* TODO: Should PCP of DSLite do iptables here?? */
         if ((wan_ip_con->X_BROADCOM_COM_PCPMode != PCP_MODE_DSLITE) || 
             !cmsUtl_strstr(currObj->X_BROADCOM_COM_AppName, "upnp"))
         {
            ret = rutIpt_vrtsrvCfg(wan_ip_con, NULL, NULL, currObj, FALSE);
         }
      }
   }   
   
   cmsObj_free((void **) &wan_ip_con);


   /*
    * Update PortMappingNumberOfEntries on successful add or delete.
    */
   if (ret == CMSRET_SUCCESS)
   {
      if (ADD_NEW(newObj, currObj))
      {
         rut_modifyNumIpPortMapping(iidStack, 1);
      }
      else if (DELETE_EXISTING(newObj, currObj))
      {
         rut_modifyNumIpPortMapping(iidStack, -1);
      }
   }

   return ret;
}

#endif  /* DMP_BASELINE_1 */


CmsRet rcl_secDmzHostCfgObject( _SecDmzHostCfgObject *newObj __attribute__((unused)),
                                 const _SecDmzHostCfgObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

#ifdef DMP_BASELINE_1
   /* the secDmzHostCfg object is an object that can be inserted in the
    * IGD or Device data model. So this object must work correctly in
    * both data models.  However, the code in this function assumes IGD,
    * so that needs to be fixed.
    */
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
   _WanPppConnObject *wan_ppp_con = NULL;
   _WanIpConnObject *wan_ip_con = NULL;
   char cmd[BUFLEN_264];
   UBOOL8 init_process = FALSE;


   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   if (currObj == NULL)
   {
      init_process = TRUE;
   }

   cmsLog_debug("Dmz host entry: addr=%s ", newObj->IPAddress);

   cmd[0] = '\0';

   while (cmsObj_getNext(MDMOID_WAN_IP_CONN, &iidStack1, (void **) &wan_ip_con) == CMSRET_SUCCESS)
   {
      if ( wan_ip_con->NATEnabled == FALSE ) 
      {
         cmsObj_free((void **) &wan_ip_con);	
         continue;
      }

      if ( wan_ip_con->X_BROADCOM_COM_IfName[0] != '\0' ) 
      {
         /* delete the old DMZ host */
         if ( init_process == FALSE )
         {
            if (cmsUtl_isValidIpAddress(AF_INET, currObj->IPAddress))
            {
               sprintf(cmd, "iptables -w -t nat -D PREROUTING -i %s -j DNAT --to-destination %s",
                       wan_ip_con->X_BROADCOM_COM_IfName, currObj->IPAddress);
               rut_doSystemAction("rcl_secDmzHostCfgObject", cmd);
               sprintf(cmd, "iptables -w -D FORWARD -i %s -d %s -j ACCEPT",
                       wan_ip_con->X_BROADCOM_COM_IfName, currObj->IPAddress);
               rut_doSystemAction("rcl_secDmzHostCfgObject", cmd);	    
            }
  
         }
         /* add the new DMZ host */
         if (cmsUtl_isValidIpAddress(AF_INET, newObj->IPAddress))      
         {
            sprintf(cmd, "iptables -w -t nat -A PREROUTING -i %s -j DNAT --to-destination %s",
                    wan_ip_con->X_BROADCOM_COM_IfName, newObj->IPAddress);
            rut_doSystemAction("rcl_secDmzHostCfgObject", cmd);
            sprintf(cmd, "iptables -w -I FORWARD 1 -i %s -d %s -j ACCEPT",
                    wan_ip_con->X_BROADCOM_COM_IfName, newObj->IPAddress);
            rut_doSystemAction("rcl_secDmzHostCfgObject", cmd);	    
         }
      }
      cmsObj_free((void **) &wan_ip_con);		 
   }

   while (cmsObj_getNext(MDMOID_WAN_PPP_CONN, &iidStack2, (void **) &wan_ppp_con) == CMSRET_SUCCESS)
   {
      if ( wan_ppp_con->NATEnabled == FALSE ) 
      {
         cmsObj_free((void **) &wan_ppp_con);
         continue;
      }

      if ( wan_ppp_con->X_BROADCOM_COM_IfName[0] != '\0' ) 
      {
         /* delete the old DMZ host */
         if ( init_process == FALSE )
         {
            if (cmsUtl_isValidIpAddress(AF_INET, currObj->IPAddress))
            {
               sprintf(cmd, "iptables -w -t nat -D PREROUTING -i %s -j DNAT --to-destination %s",
                       wan_ppp_con->X_BROADCOM_COM_IfName, currObj->IPAddress);
               rut_doSystemAction("rcl_secDmzHostCfgObject", cmd);
               sprintf(cmd, "iptables -w -D FORWARD -i %s -d %s -j ACCEPT",
                       wan_ppp_con->X_BROADCOM_COM_IfName, currObj->IPAddress);
               rut_doSystemAction("rcl_secDmzHostCfgObject", cmd);	    
            }
         }
         /* add the new DMZ host */
         if (cmsUtl_isValidIpAddress(AF_INET, newObj->IPAddress))
         {
            sprintf(cmd, "iptables -w -t nat -A PREROUTING -i %s -j DNAT --to-destination %s",
                    wan_ppp_con->X_BROADCOM_COM_IfName, newObj->IPAddress);
            rut_doSystemAction("rcl_secDmzHostCfgObject", cmd);
            sprintf(cmd, "iptables -w -I FORWARD 1 -i %s -d %s -j ACCEPT",
                    wan_ppp_con->X_BROADCOM_COM_IfName, newObj->IPAddress);
            rut_doSystemAction("rcl_secDmzHostCfgObject", cmd);	    
         }
      }
      cmsObj_free((void **) &wan_ppp_con);		 
   }
   
#endif  /* DMP_BASELINE_1 */

   return ret;

}

