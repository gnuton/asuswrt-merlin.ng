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


#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "rut_wifiwan.h"

#ifdef DMP_BASELINE_1
/*
 * All this WAN stuff is probably highly dependent on TR98, so only use
 * in Legacy TR98 and Hybrid TR98+TR181 mode.  The InterfaceControl object
 * is used for all data models though.
 */
#include "rut_wan6.h"
#include "rut_ipconcfg.h"
#include "rut_pppconcfg.h"
#include "rut_ethintf.h"
#include "rut_lan.h"
#include "rut_pmap.h"
#include "rut_xtmlinkcfg.h"
#include "rut_wanlayer2.h"

#include "rut_rip.h"
#include "rut_qos.h"

#if defined(DMP_X_BROADCOM_COM_AUTODETECTION_1) || defined(DMP_X_BROADCOM_COM_GPONWAN_1)
#define AUTO_DETECT_SEND_UPDATE_MSG(oid, iidStack, isDeleted) \
rutWan_sendConnectionUpdateMsg(oid, iidStack, FALSE, FALSE, isDeleted, FALSE);
#else
/* do nothing if auto detect is not defined */
#define AUTO_DETECT_SEND_UPDATE_MSG(oid, iidStack, isDeleted) \
cmsLog_debug("AUTO_DETECT_SEND_UPDATE_MSG do nothing if auto is not defined");
#endif   
                           
/*
 * This file contains generic WAN objects.
 * Wan objects that are related to xDSL, ATM, and PTM have been moved to their
 * own files.
 */


CmsRet rcl_wanDevObject( _WanDevObject *newObj,
                const _WanDevObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (newObj == NULL)
   {
      /*
       * This is a delete instance.  Deletes always update the MDM, even if the
       * RCL handler function could not do the action.
       */
      rut_modifyNumWanDev(iidStack, -1);
   }


   if (newObj != NULL && currObj == NULL)
   {
      /* new instance of Wan Dev is being added */
      rut_modifyNumWanDev(iidStack, 1);
   }

   /* no other work needs to be done for Wan Dev Object */


   return CMSRET_SUCCESS;
}


CmsRet rcl_wanCommonIntfCfgObject( _WanCommonIntfCfgObject *newObj __attribute__((unused)),
                const _WanCommonIntfCfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}


CmsRet rcl_wanConnDeviceObject( _WanConnDeviceObject *newObj,
                const _WanConnDeviceObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   if (newObj == NULL)
   {
      /*
       * This is a delete instance.  Deletes always update the MDM, even if the
       * RCL handler function could not do the action.
       */
      rut_modifyNumWanConn(iidStack, -1);
   }


   if (newObj != NULL && currObj == NULL)
   {
      /* new instance of Wan Dev is being added */
      rut_modifyNumWanConn(iidStack, 1);
   }

   /* no other work needs to be done for Wan Conn Dev Object */

   return CMSRET_SUCCESS;
}



CmsRet rcl_wanIpConnObject(_WanIpConnObject *newObj,
                           const _WanIpConnObject *currObj,
                           const InstanceIdStack *iidStack __attribute__((unused)),
                           char **errorParam __attribute__((unused)),
                           CmsRet *errorCode __attribute__((unused)))
{
   UBOOL8 isAddPvc=TRUE;
   UBOOL8 isStatic=TRUE;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Entered");
   
   if (ADD_NEW(newObj, currObj))
   {
      /*
       * add new object instance usually will succeed, so just update
       * the count here.  The only failure scenario I can think of is if
       * we exceed the max number of ip connections.  we can check that here.
       */
      rut_modifyNumIpConn(iidStack, 1);
      
      if (!newObj->enable)
      {
         /* this is a run-time add of the object.
          * All the code below assumes the object has already been filled
          * in with reasonable values, but when an object is added at
          * run time, all the fields are filled with default values.
          * Rather than trying to figure it out down there, just return here.
          */
          return CMSRET_SUCCESS;
      }
      
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      /*
       * This is a delete instance, update count.  Deletes always update the MDM,
       * even if the RCL handler function could not do the action.
       */
      rut_modifyNumIpConn(iidStack, -1);
   }
   
   /* Create WAN layer 3 linux user friendly interface name if it has not existed yet */
   if (newObj && newObj->X_BROADCOM_COM_IfName == NULL)
   {
      if ((ret = rutWan_fillWanL3IfNameAndServiceName(newObj, iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutWan_fillWanL3IfNameAndServiceName Failed.");
         /* This is to delete this connection object even if it does not have a valid configuration but
         * can be safely removed if all following conditions are met: (ie. return CMSRET_SUCCESS so MDM will not reboot)
         * 1). In delete state
         * 2) Fail to get connection info due to invalid arguments.
         * 3). This ConnectionObject is UNCONFIGURED. (no RCL actions need to be undone)
         */
         if (DELETE_EXISTING(newObj, currObj) && 
            ret == CMSRET_INVALID_ARGUMENTS &&
            cmsUtl_strcmp(currObj->connectionStatus, MDMVS_UNCONFIGURED) == 0)
         {   
            ret = CMSRET_SUCCESS;
         }
      }  
   }

#ifdef DMP_BRIDGING_1
   if ( ENABLE_EXISTING(newObj, currObj) && !rutWl2_isIPoA(iidStack) )
   {
      UBOOL8 isWanIntf = TRUE;
      UINT32 bridgeRef = 0;
      /*
      * Only need to modify the MDM structure on an enable existing.
      * If this is an enable new, this must be bootup time,
      * and if this is bootup, the structure is already reflected in
      * the config file.
      *
      * Only need to do this for bridge and IPoE connections.
      */
      rutPMap_addAvailableInterface(newObj->X_BROADCOM_COM_IfName, isWanIntf);
      rutPMap_addFilter(newObj->X_BROADCOM_COM_IfName, isWanIntf, bridgeRef);
   }
#endif
   
#ifdef SUPPORT_RIP
   if (rutRip_isConfigChanged_igd(newObj, currObj))
   {
      if (!rutRip_isConfigValid_igd(newObj))
      {
         return CMSRET_INVALID_ARGUMENTS;
      }
   }
#endif

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /*
       * Obj is enabled.  Set connectionStatus to "Unconfigured" if it is not
       * already in that state.
       *
       * Send a messge to ssk to let it know a new wan connection has been added so
       * that ssk can move the state machine forward if the underlying link is already up.
       */
      cmsLog_debug("Sending WAN connection %s to ssk.", newObj->X_BROADCOM_COM_IfName);

      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->connectionStatus, MDMVS_UNCONFIGURED, mdmLibCtx.allocFlags);
      RESET_IPV6_CONNSTATUS(newObj);

      /* isStatic is TRUE for bridge/ipoa and static IPoE. For dynamic IPoE, it's FALSE */
      isStatic = !(!cmsUtl_strcmp(newObj->connectionType, MDMVS_IP_ROUTED) && 
                   cmsUtl_strcmp(newObj->addressingType, MDMVS_STATIC));

      rutWan_sendConnectionUpdateMsg(MDMOID_WAN_IP_CONN, iidStack, isAddPvc, isStatic, FALSE, FALSE);

      return CMSRET_SUCCESS;
   }  
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      if ( rutWan_isIpv4ConnStatusChanged(newObj, currObj, TRUE) )
      {
         /*
          * Change in connectionStatus state machine or some other run-time change.
          */
         cmsLog_debug("new->ConnStatus=%s curr->ConnStatus=%s",
                      newObj->connectionStatus, currObj->connectionStatus);
  
         if (!cmsUtl_strcmp(newObj->connectionStatus, MDMVS_CONNECTING))
         {     
            if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_UNCONFIGURED) ||
                !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_DISCONNECTED))
            {
               /* from "Unconfigured"/Disconnected" to "Connecting" */
               if ((ret = rutCfg_startWanIpConnection(iidStack, newObj)) == CMSRET_SUCCESS)
               {
                  cmsLog_debug("rutCfg_startWanIpConnection ok. pid=%d", newObj->X_BROADCOM_COM_DhcpcPid);
               }
               else
               {
                  cmsLog_error("rutCfg_startWanIpConnection failed, error %d", ret);
               }
            }
            else if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTED) && 
                     rutWan_isTransientLayer2LinkUp(newObj->X_BROADCOM_COM_TransientLayer2LinkStatus))
               
            {
               /* If the WAN link is  up (the link status X_BROADCOM_COM_TransientLayer2LinkStatus is passed in from the cmsObj_set in ssk, 
               * not real layer2 link status which could be different) and the connectionStatus goes from "CONNECTED" to "CONNECTING", 
               * it must be dhcpc lease expired.  Need to tear down layer 3 ip related services but do not 
               * stop the layer 3 interface so that dhcpc is still running
               */
               if ((ret = rutCfg_tearDownWanIpConnection(iidStack, 
                                                         currObj, TRUE)) == CMSRET_SUCCESS)
               {
                  cmsLog_debug("rutCfg_tearDownWanIpConnection ok (Connected to Disconnected).");
               }
               else
               {
                  cmsLog_error("rutCfg_tearDownWanIpConnection failed, error %d", ret);
               }

               /* Need to clear wan interface ip address to prevent the LAN traffic going through 
               * this interface
               */
               rutWan_clearWanConnIpAdress(currObj->X_BROADCOM_COM_IfName);
            }
         }
         else if (!cmsUtl_strcmp(newObj->connectionStatus, MDMVS_CONNECTED))
         {
            UBOOL8 skipWanIpSetup = FALSE;
               
            if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTED))

            {
               /* From "Connected" to "Connected".  Must be dhcpc get a new external ip/default gateway, dns info.
                * Just teardown first and will be setup again below.
                */
               if((cmsUtl_strcmp(newObj->externalIPAddress, currObj->externalIPAddress) != 0) ||
                     (cmsUtl_strcmp(newObj->subnetMask, currObj->subnetMask) != 0) ||
                     (cmsUtl_strcmp(newObj->defaultGateway, currObj->defaultGateway) != 0) ||
                     (newObj->X_BROADCOM_COM_FirewallEnabled != currObj->X_BROADCOM_COM_FirewallEnabled) ||
                     (cmsUtl_strcmp(newObj->DNSServers, currObj->DNSServers) != 0) ) 
               {

                  if ((ret = rutCfg_tearDownWanIpConnection(iidStack, 
                              currObj, TRUE)) == CMSRET_SUCCESS)
                  {
                     cmsLog_debug("rutCfg_tearDownWanIpConnection ok");
                     cmsLed_setWanDisconnected();            
                  }
                  else
                  {
                     cmsLog_error("rutCfg_tearDownWanIpConnection failed. ret=%d", ret);
                  }
               }
               else
               {
                  /*dont restrart WAN services if the change is not related to connection status(ex:portmappingNumberOfEntries) */
                  skipWanIpSetup = TRUE;
               }

#ifdef SUPPORT_RIP
               /* CONNECTED to CONNECTED: ripd config might have changed */
               if (rutRip_isConfigChanged_igd(newObj, currObj))
               {
                  if (rutRip_getNumberOfRipInterfaces() == 0)
                  {
                     /* I have been disabled, and I was last RIP interface, so
                      * stop ripd.
                      */
                     rutRip_stop();
                  }
                  else
                  {
                     rutRip_writeConfigFile();
                     rutRip_restart();
                  }
               }
#endif
            }
            else if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_UNCONFIGURED) ||
                     !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_DISCONNECTED))
            {
               /* For bridge/IPoA and static IPoE, when layer 2 is up, ssk directly sets
               * connectionStatus to "Connected".  Need to start the interface.
               */

               /* from "Unconfigured"/Disconnected" to "Connecting" */
               if ((ret = rutCfg_startWanIpConnection(iidStack, newObj)) == CMSRET_SUCCESS)
               {
                  cmsLog_debug("rutCfg_startWanIpConnection ok.");
               }
               else
               {
                  cmsLog_error("rutCfg_startWanIpConnection failed, error %d", ret);
                  
               }            
            }
   
            /* From "Unconfigured", "Connecting" or "Disconnected" to "Connected", OR
            * from "Connected" to "Connected" where Wan connection is already tearDown above.
            * Just setup the Wan Ip connection.
            */
            if ((skipWanIpSetup == FALSE) && (ret == CMSRET_SUCCESS))
            {
               if ((ret = rutCfg_setupWanIpConnection(iidStack, newObj)) == CMSRET_SUCCESS)
               {
                  cmsLog_debug("rutCfg_setupWanIpConnection ok");
                  /*
                   * TR-124 says we should turn the WAN LED green when we have
                   * IP address on WAN.  But not for bridge conn.
                   */
                  if (!cmsUtl_strcmp(newObj->connectionType, MDMVS_IP_ROUTED))
                  {
                     cmsLed_setWanConnected();
                  }
               }
               else
               {
                  cmsLog_error("rutCfg_setupWanIpConnection failed. ret=%d", ret);
               }
            }
         }
         else if (!cmsUtl_strcmp(newObj->connectionStatus, MDMVS_DISCONNECTED))
         {
            if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTED) || 
                !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTING))
            {
               InstanceIdStack portMappingIidStack = EMPTY_INSTANCE_ID_STACK;
               WanIpConnPortmappingObject *port_mapping = NULL;
               /* From CONNECTED/CONNECTING -> DISCONNECED 
               * always tear down Wan services associated with this WAN connection
               */
   
               cmsLed_setWanDisconnected();
   
               if ((ret = rutCfg_tearDownWanIpConnection(iidStack, 
                                                         currObj, TRUE)) == CMSRET_SUCCESS)
               {
                  cmsLog_debug("rutCfg_tearDownWanIpConnection ok (Connected to Disconnected).");
               }
               else
               {
                  cmsLog_error("rutCfg_tearDownWanIpConnection failed, error %d", ret);
               }
   
               if ((ret = rutCfg_stopWanIpConnection(iidStack, currObj, TRUE)) == CMSRET_SUCCESS)
               {
                  cmsLog_debug("rutCfg_stopWanIpConnection ok.");
               }
               else
               {
                  cmsLog_error("rutCfg_stopWanIpConnection failed, error %d", ret);
               }

               /* remove none-persistent portmapping instance from this connection*/
               INIT_INSTANCE_ID_STACK(&portMappingIidStack);
               while ((cmsObj_getNextInSubTree(MDMOID_WAN_IP_CONN_PORTMAPPING, iidStack, &portMappingIidStack, (void **)&port_mapping) == CMSRET_SUCCESS))
               {
                  if (cmsObj_isNonpersistentInstance(MDMOID_WAN_IP_CONN_PORTMAPPING, &portMappingIidStack) == TRUE)
                  {
                     cmsObj_deleteInstance(MDMOID_WAN_IP_CONN_PORTMAPPING, &portMappingIidStack);
                     INIT_INSTANCE_ID_STACK(&portMappingIidStack);
                  }
                  cmsObj_free((void **) &port_mapping);
               }
            }
         }
      }
      
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
      if ( rutWan_isIpv6ConnStatusChanged(newObj, currObj, TRUE) )
      {
         ret = rutWan_ipv6IpConnProcess(newObj, currObj, iidStack);

         cmsLog_debug("return from rutWan_ipv6IpConnProcess with ret=%d", ret);
         return ret;
      }
#endif
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {   
      if ( currObj->X_BROADCOM_COM_IPv4Enabled )
      {
         if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTED) || 
	          !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTING))
         {
            /* Only tearDown when currObj->connectionStatus is CONNECTED/CONNECTING  */
	   
            cmsLed_setWanDisconnected();

            /* for delete or disable wan interface */
            if ((ret = rutCfg_tearDownWanIpConnection(iidStack, currObj, TRUE)) == CMSRET_SUCCESS)
            {                                                    
               cmsLog_debug("rutCfg_tearDownWanIpConnection ok.");
            }           
            else
            {
               cmsLog_error("rutCfg_tearDownWanIpConnection failed, ret=%d", ret);
            }

            if ((ret = rutCfg_stopWanIpConnection(iidStack, currObj, TRUE)) == CMSRET_SUCCESS)
            {
               cmsLog_debug("rutCfg_stopWanIpConnection ok");
            }
            else
            {
               cmsLog_error("rutCfg_stopWanIpConnection failed, ret=%d", ret);
            }
         }
      
         /* For Dynamic IPoE, wipe out  the external ip address, default gateway, etc.  */
         if (newObj && !cmsUtl_strcmp(newObj->addressingType, MDMVS_DHCP))
         {
            CMSMEM_REPLACE_STRING_FLAGS(newObj->externalIPAddress, "0.0.0.0", mdmLibCtx.allocFlags);         
            CMSMEM_REPLACE_STRING_FLAGS(newObj->subnetMask, "", mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->defaultGateway, "", mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->DNSServers, "", mdmLibCtx.allocFlags);

            if (!newObj->enable)
            {
               cmsLog_debug("Setting connStatus to UNCONFIGURED if it is disabled by users.");
               CMSMEM_REPLACE_STRING_FLAGS(newObj->connectionStatus, MDMVS_UNCONFIGURED, mdmLibCtx.allocFlags);
            } 
         }
         if (newObj && !cmsUtl_strcmp(newObj->addressingType, MDMVS_STATIC))
         {
            if (!newObj->enable)
            {
               cmsLog_debug("Setting connStatus to UNCONFIGURED if it is disabled by users.");
               CMSMEM_REPLACE_STRING_FLAGS(newObj->connectionStatus, MDMVS_UNCONFIGURED, mdmLibCtx.allocFlags);
            }
         }
      }
#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
      rutWan_ipv6IpConnDisable(newObj, currObj, iidStack);
#endif
      
      /* for delete */
      if (DELETE_EXISTING(newObj, currObj))
      {   

         /* need to update the auto detect connection info on ssk link list to remove the wan conn from the list */
         AUTO_DETECT_SEND_UPDATE_MSG(MDMOID_WAN_IP_CONN, iidStack, TRUE);
         
         if ((ret = rutCfg_deleteWanIpConnection(iidStack, currObj)) == CMSRET_SUCCESS)

         {
            cmsLog_debug("rutCfg_deleteWanIpConnection ok.");
         }
         else
         {
            cmsLog_error("rutCfg_deleteWanIpConnection failed, error %d", ret);
         }
      }  
    
   }
    
    /* do dhcpd updating during adding or deleteing Wan service */
   if (rutLan_updateDhcpd() != CMSRET_SUCCESS)
   {
      cmsLog_error("rutLan_updateDhcpd fail");
   }

   cmsLog_debug("Exit ret=%d", ret);
   
   return ret;

}


CmsRet rcl_wanIpConnStatsObject( _WanIpConnStatsObject *newObj __attribute__((unused)),
                const _WanIpConnStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}



CmsRet rcl_wanPppConnObject( _WanPppConnObject *newObj,
                const _WanPppConnObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   UBOOL8 isAddPvc=TRUE;
   UBOOL8 isStatic=FALSE;
   CmsRet ret=CMSRET_SUCCESS;
   
   cmsLog_debug("Entered");

   if (ADD_NEW(newObj, currObj))
   {
      /*
       * add new object instance usually will succeed, so just update
       * the count here.  The only failure scenario I can think of is if
       * we exceed the max number of ppp connections.  we can check that here.
       */
      rut_modifyNumPppConn(iidStack, 1);

      if (!newObj->enable)
      {
         /* this is a run-time add of the object.
          * All the code below assumes the object has already been filled
          * in with reasonable values, but when an object is added at
          * run time, all the fields are filled with default values.
          * Rather than trying to figure it out down there, just return here.
          */
          return CMSRET_SUCCESS;
      }

   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      /*
       * This is a delete instance, update count.  Deletes always update the MDM,
       * even if the RCL handler function could not do the action.
       */
      rut_modifyNumPppConn(iidStack, -1);
   }
   
   /* Create WAN layer 3 linux user friendly interface name if it has not existed yet */
   if (newObj && newObj->X_BROADCOM_COM_IfName == NULL)
   {
      if ((ret = rutWan_fillWanL3IfNameAndServiceName(newObj, iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutWan_fillWanL3IfNameAndServiceName Failed.");
         /* This is to delete this connection object even if it does not have a valid configuration but
         * can be safely removed if all following conditions are met: (ie. return CMSRET_SUCCESS so MDM will not reboot)
         * 1). In delete state
         * 2) Fail to get connection info due to invalid arguments.
         * 3). This ConnectionObject is UNCONFIGURED. (no RCL actions need to be undone)
         */
         if (DELETE_EXISTING(newObj, currObj) && 
            ret == CMSRET_INVALID_ARGUMENTS &&
            cmsUtl_strcmp(currObj->connectionStatus, MDMVS_UNCONFIGURED) == 0)
         {   
            ret = CMSRET_SUCCESS;
         }
      }  
   }
   
#ifdef DMP_BRIDGING_1
   if (!rutWl2_isPPPoA(iidStack) && ENABLE_EXISTING(newObj, currObj))
   {
      UBOOL8 isWanIntf = TRUE;
      UINT32 bridgeRef = 0;
      /*
       * Only need to modify the MDM structure on an enable existing.
       * If this is an enable new, this must be bootup time,
       * and if this is bootup, the structure is already reflected in
       * the config file.
       *
       * Only need to do this for a PPPoE connection.
       */
      rutPMap_addAvailableInterface(newObj->X_BROADCOM_COM_IfName, isWanIntf);
      rutPMap_addFilter(newObj->X_BROADCOM_COM_IfName, isWanIntf, bridgeRef);
   }
#endif
   

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /*
       * Obj is enabled.  Set connectionStatus to "Unconfigured" if it is not
       * already in that state.
       *
       * Send a messge to ssk to let it know a new wan connection has been added so
       * that ssk can move the state machine forward if the underlying link is already up.
       */
      cmsLog_debug("Sending WAN connection %s to ssk.", newObj->X_BROADCOM_COM_IfName);
      
      rutWan_sendConnectionUpdateMsg(MDMOID_WAN_PPP_CONN, iidStack, isAddPvc, isStatic, FALSE, FALSE);

      return CMSRET_SUCCESS;
   }  
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      if ( rutWan_isIpv4ConnStatusChanged(newObj, currObj, FALSE) )
      {
         /*
          * Change in connectionStatus state machine or some other run-time change.
          */
         cmsLog_debug("new->ConnStatus=%s new->LastConnError=%s curr->ConnStatus=%s",
                      newObj->connectionStatus,
                      newObj->lastConnectionError,
                      currObj->connectionStatus);

         if (!cmsUtl_strcmp(newObj->connectionStatus, MDMVS_CONNECTING))
         {      
            if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_UNCONFIGURED) ||
                !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_DISCONNECTED))
            {
               /* from "Unconfigured"/Disconnected" to "Connecting" */
               if ((ret = rutCfg_startWanPppConnection(iidStack, newObj)) == CMSRET_SUCCESS)
               {
                  cmsLog_debug("rutCfg_startWanPppConnection ok.");
               }
               else
               {
                  cmsLog_error("rutCfg_startWanPppConnection failed, error %d", ret);
               }
            }
         }
         else if (!cmsUtl_strcmp(newObj->connectionStatus, MDMVS_CONNECTED))
         {
            /* ConnectionStatus is not changed but X_BROADCOM_COM_UserRequest is changed from "None"
            * to "Down", need to send message to pppd to get disconnected
            */
            if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTED) &&
                !cmsUtl_strcmp(currObj->X_BROADCOM_COM_UserRequest, MDMVS_NONE) &&
                !cmsUtl_strcmp(newObj->X_BROADCOM_COM_UserRequest, MDMVS_DOWN))
            {
               CmsMsgHeader msg = EMPTY_MSG_HEADER;
               CmsRet rc2;

               msg.type = CMS_MSG_SET_PPP_DOWN;
               msg.src =mdmLibCtx.eid;
               msg.dst = MAKE_SPECIFIC_EID(newObj->X_BROADCOM_COM_PppdPid, EID_PPP); 
               msg.flags_request = 1;

               rc2 = cmsMsg_send(mdmLibCtx.msgHandle, &msg);
               if (rc2 != CMSRET_SUCCESS)
               {
                  cmsLog_error("Fail to send disconnect msg to ppp %s, ret=%d", newObj->X_BROADCOM_COM_IfName, rc2);
               }	

               /* restore to "None" */
               CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_UserRequest, MDMVS_NONE, mdmLibCtx.allocFlags); 
            }
            else if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTING) ||
                     !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_DISCONNECTED))
            {
               /* From "Connecting" or "Disconnected" (from on-demand, pppd always send "Disconnected" first then Connected")
               * to "Connected".  Just setup the related WAN services 
               */
               if ((ret = rutCfg_setupWanPppConnection(iidStack, newObj)) == CMSRET_SUCCESS)
               {
                  cmsLog_debug("rutCfg_setupWanPppConnection ok.");
                  cmsLed_setWanConnected();               
               }
               else
               {
                  cmsLog_error("rutCfg_setupWanPppConnection failed. ret=%d", ret);

                  if ((ret = rutCfg_tearDownWanPppConnection(iidStack, newObj, currObj, TRUE)) == CMSRET_SUCCESS)
                     cmsLog_debug("rutCfg_tearDownWanPppConnection ok.");
                  else
                  {
                     cmsLog_error("rutCfg_tearDownWanPppConnection failed, error %d", ret);
                  }
                  if ((ret = rutCfg_stopWanPppConnection(iidStack, currObj, TRUE)) == CMSRET_SUCCESS)
                  {
                     cmsLog_debug("rutCfg_stopWanPppConnection ok.");
                  }
                  else
                  {
                     cmsLog_error("rutCfg_stopWanPppConnection failed, error %d", ret);
                  }
               }
            }
         }
         else if (!cmsUtl_strcmp(newObj->connectionStatus, MDMVS_DISCONNECTED))
         {
            /* ConnectionStatus is not changed but X_BROADCOM_COM_UserRequest is changed from "None"
            * to "Up", need to send message to pppd to get connected
            */
            if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_DISCONNECTED) &&
                !cmsUtl_strcmp(currObj->X_BROADCOM_COM_UserRequest, MDMVS_NONE) &&
                !cmsUtl_strcmp(newObj->X_BROADCOM_COM_UserRequest, MDMVS_UP))
            {
               CmsMsgHeader msg = EMPTY_MSG_HEADER;
               CmsRet rc2;

               msg.type = CMS_MSG_SET_PPP_UP;
               msg.src =mdmLibCtx.eid;
               msg.dst = MAKE_SPECIFIC_EID(newObj->X_BROADCOM_COM_PppdPid, EID_PPP); 
               msg.flags_request = 1;

               rc2 = cmsMsg_send(mdmLibCtx.msgHandle, &msg);
               if (rc2 != CMSRET_SUCCESS)
               {
                  cmsLog_error("Fail to send Connect msg to ppp %s, ret=%d", newObj->X_BROADCOM_COM_IfName, rc2);
               }	

               /* restore to "None" */
               CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_UserRequest, MDMVS_NONE, mdmLibCtx.allocFlags); 
            }
            else if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTED) ||
                     !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTING) ||
                     !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_DISCONNECTED))
            {
               InstanceIdStack portMappingIidStack = EMPTY_INSTANCE_ID_STACK;
               WanPppConnPortmappingObject *port_mapping = NULL;

               /* Even if connectionStatus is already "Disconnected", it might be from ppp server is down and pppd
               * may still be in the memory, and need to be stopped if the layer 2 link is down (in connstatus.c)
               */
               if (!cmsUtl_strcmp(newObj->lastConnectionError, MDMVS_ERROR_AUTHENTICATION_FAILURE))
               {
                  /* for authentication failure, need to set LED differently */
                  cmsLed_setWanFailed();
               }
               
               /* From CONNECTED/CONNECTING -> DISCONNECED 
               * always tear down Wan services associated with this WAN connection
               */
               if ((ret = rutCfg_tearDownWanPppConnection(iidStack, newObj, currObj, TRUE)) == CMSRET_SUCCESS)
               {
                  cmsLog_debug("rutCfg_tearDownWanPppConnection ok (Connected to Disconnected).");
               }
               else
               {
                  cmsLog_error("rutCfg_tearDownWanPppConnection failed, error %d", ret);
               }

               cmsLog_debug("link is %s", newObj->X_BROADCOM_COM_TransientLayer2LinkStatus);

               /* remove none-persistent portmapping instance from this connection*/
               INIT_INSTANCE_ID_STACK(&portMappingIidStack);
               while ((cmsObj_getNextInSubTree(MDMOID_WAN_PPP_CONN_PORTMAPPING, iidStack, &portMappingIidStack, (void **)&port_mapping) == CMSRET_SUCCESS))
               {
                  if (cmsObj_isNonpersistentInstance(MDMOID_WAN_PPP_CONN_PORTMAPPING, &portMappingIidStack) == TRUE)
                  {
                     cmsObj_deleteInstance(MDMOID_WAN_PPP_CONN_PORTMAPPING, &portMappingIidStack);
                     INIT_INSTANCE_ID_STACK(&portMappingIidStack);
                  }
                  cmsObj_free((void **) &port_mapping);
               }
               
               /* Only stop pppd when layer 2 link is down (X_BROADCOM_COM_TransientLayer2LinkStatus from ssk)
               * Otherwise, keep pppd running when ConnectionStatus = "Disconnected" for following case:
               * 1). ppp connection is down - lastConnectionError = "ERROR_UNKNOWN",
               *  
               *  OR in auto detect mode and ppp cannot be up in the specified time frame, stop pppd as well
               *  X_BROADCOM_COM_StopPppD is set to TRUE during in ssk when setting connectionStatus
               * to "Connecting" OR disabled by manual selection.
               */
               cmsLog_debug("STOP_PPPD(newObj)=%d", STOP_PPPD(newObj));
               if (!rutWan_isTransientLayer2LinkUp(newObj->X_BROADCOM_COM_TransientLayer2LinkStatus) || STOP_PPPD(newObj))
               {
                  if ((ret = rutCfg_stopWanPppConnection(iidStack, currObj, TRUE)) == CMSRET_SUCCESS)
                  {
                     cmsLog_debug("rutCfg_stopWanPppConnection ok.");
                  }
                  else
                  {
                     cmsLog_error("rutCfg_stopWanPppConnection failed, error %d", ret);
                  }
                  
#ifdef DMP_X_BROADCOM_COM_AUTODETECTION_1          
                  /* reset it to FALSE */
                  newObj->X_BROADCOM_COM_StopPppD = FALSE;
#endif /* DMP_X_BROADCOM_COM_AUTODETECTION_1 */   

               }
            }
         }
      }

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
      if ( rutWan_isIpv6ConnStatusChanged(newObj, currObj, FALSE) )
      {
         ret = rutWan_ipv6PppConnProcess(newObj, currObj, iidStack);

         cmsLog_debug("return from rutWan_ipv6PppConnProcess with ret=%d", ret);
         return ret;
      }
#endif
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {   
      cmsLed_setWanDisconnected();
      if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTED) || 
          !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTING) ||
          !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_DISCONNECTED))
      {
         /* Only do tearDown/stop when currObj->connectionStatus is CONNECTED/CONNECTING */
   
         /* for delete or disable wan interface */
         if ((ret = rutCfg_tearDownWanPppConnection(iidStack, newObj, currObj, TRUE)) == CMSRET_SUCCESS)
         {                                                    
            cmsLog_debug("rutCfg_tearDownWanPppConnection ok.");
         }           
         else
         {
            cmsLog_error("rutCfg_tearDownWanPppConnection failed, ret=%d", ret);
         }

         if ((ret = rutCfg_stopWanPppConnection(iidStack, currObj, TRUE)) == CMSRET_SUCCESS)
         {
            cmsLog_debug("rutCfg_stopWanPppConnection ok");
         }
         else
         {
            cmsLog_error("rutCfg_stopWanPppConnection failed, ret=%d", ret);
         }
      }
      
      /* release the external ip address and default gateway,etc. if it is not a delete */
      if (newObj)
      {
         CMSMEM_REPLACE_STRING_FLAGS(newObj->externalIPAddress, "0.0.0.0", mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_DefaultGateway, "", mdmLibCtx.allocFlags);
         CMSMEM_REPLACE_STRING_FLAGS(newObj->DNSServers, "", mdmLibCtx.allocFlags);

         if (!newObj->enable)
         {
            cmsLog_debug("Seting connStatus to UNCONFIGURED if it is disabled by users.");
            CMSMEM_REPLACE_STRING_FLAGS(newObj->connectionStatus, MDMVS_UNCONFIGURED, mdmLibCtx.allocFlags);
         }            
      }

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
      rutWan_ipv6PppConnDisable(newObj, currObj, iidStack);
#endif

      /* for delete */
      if (DELETE_EXISTING(newObj, currObj))
      {   

          /* need to update the auto detect connection info on ssk link list to remove the wan conn from the list */
          AUTO_DETECT_SEND_UPDATE_MSG(MDMOID_WAN_PPP_CONN, iidStack, TRUE);
      
         if ((ret = rutCfg_deleteWanPppConnection(iidStack, currObj)) == CMSRET_SUCCESS)
         {
            cmsLog_debug("rutCfg_deleteWanPppConnection ok.");
            if (!rutWl2_isPPPoA(iidStack))
            {
               if ((ret = rutWan_cleanUpPPPoE(iidStack, currObj)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("Clean up PPPoE failed.");
               }
            }
         }
         else
         {
            cmsLog_error("rutCfg_deleteWanPppConnection failed, error %d", ret);
         }
      }  
    
   }
    
    /* do dhcpd updating during adding or deleteing Wan service */
   if (rutLan_updateDhcpd() != CMSRET_SUCCESS)
   {
      cmsLog_error("rutLan_updateDhcpd fail");
   }


   cmsLog_debug("Exit ret=%d", ret);
   
   return ret;
   
}


CmsRet rcl_wanPppConnStatsObject( _WanPppConnStatsObject *newObj __attribute__((unused)),
                const _WanPppConnStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}

#endif  /* DMP_BASELINE_1 */




/** This object is used by both the TR98 and TR181 data models.
 *
 */
CmsRet rcl_interfaceControlObject( _InterfaceControlObject *newObj,
                const _InterfaceControlObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("interfaceControl object info: ifName=%s, moveToWANSide=%d, moveToWANSide=%d ", 
                              newObj->ifName, newObj->moveToWANSide, newObj->moveToLANSide);

   if ( newObj->moveToWANSide )
   {
#ifdef SUPPORT_ETHWAN
      if (!cmsUtl_strncmp(newObj->ifName, ETH_IFC_STR, strlen(ETH_IFC_STR)))
      {
         rutWan_moveEthLanToWan(newObj->ifName);
      }
#endif
#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1
      if (!cmsUtl_strncmp(newObj->ifName, MOCA_IFC_STR, strlen(MOCA_IFC_STR)))
      {
         rutWan_moveMocaLanToWan(newObj->ifName);
      }
#endif
#if defined(DMP_X_BROADCOM_COM_WIFIWAN_1) || defined(DMP_X_BROADCOM_COM_DEV2_WIFIWAN_1)
      if (!cmsUtl_strncmp(newObj->ifName, WLAN_IFC_STR, strlen(WLAN_IFC_STR)))
      {
         rutWan_moveWifiLanToWan(newObj->ifName);
      }
#endif
   }
   else if ( newObj->moveToLANSide )
   {
#ifdef SUPPORT_ETHWAN
      if (!cmsUtl_strncmp(newObj->ifName, ETH_IFC_STR, strlen(ETH_IFC_STR)))
      {
         rutWan_moveEthWanToLan(newObj->ifName);
      }
#endif
#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1
      if (!cmsUtl_strncmp(newObj->ifName, MOCA_IFC_STR, strlen(MOCA_IFC_STR)))
      {
         rutWan_moveMocaWanToLan(newObj->ifName);
      }
#endif
#if defined(DMP_X_BROADCOM_COM_WIFIWAN_1) || defined(DMP_X_BROADCOM_COM_DEV2_WIFIWAN_1)
      if (!cmsUtl_strncmp(newObj->ifName, WLAN_IFC_STR, strlen(WLAN_IFC_STR)))
      {         
         rutWan_moveWifiWanToLan(newObj->ifName);
      }
#endif

   }

   /*
    * When you do a "set" on this object, the rcl handler does what you
    * requested, and then resets all the parameters back to their default
    * state.  So when you do a read on the object again, you do not see
    * the values from the previous set.
    */
   newObj->moveToWANSide = FALSE;
   newObj->moveToLANSide = FALSE;
   CMSMEM_FREE_BUF_AND_NULL_PTR(newObj->ifName);

   return ret;
}




#ifdef DMP_BASELINE_1

#ifdef DMP_ETHERNETWAN_1

CmsRet rcl_wanEthIntfObject( _WanEthIntfObject *newObj,
                const _WanEthIntfObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{


   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* if we are enabling ethernet as wan, then we can upgrade
       * the link status to NOLINK. */
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_NOLINK, mdmLibCtx.allocFlags);
      if (newObj->X_BROADCOM_COM_IfName)
      {
         if(rut_wanGetIntfIndex(newObj->X_BROADCOM_COM_IfName) <= 0)
            return CMSRET_RESOURCE_NOT_CONFIGURED;
         /* enable wan port setting of switch */
         if (rutEth_setSwitchWanPort(newObj->X_BROADCOM_COM_IfName, TRUE) != CMSRET_SUCCESS)
         {
            cmsLog_error("Fail to set wan port.");
            return CMSRET_INTERNAL_ERROR;
         }
         rutLan_enableInterface(newObj->X_BROADCOM_COM_IfName);
      }
      else
      {
         cmsLog_error("enabled, but ifName not set yet");
      }
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      /* Send connection update msg to ssk only if the device link is up and auto detect flag changed */       
      if (!cmsUtl_strcmp(newObj->status, MDMVS_UP) &&
         newObj->X_BROADCOM_COM_LimitedConnections != currObj->X_BROADCOM_COM_LimitedConnections)
      {
         UBOOL8 isAutoDetectChange = TRUE;
         
         cmsLog_debug("Link status=%s, newObj->limitedConnections=%d, currobj->limitedConnections=%d",
            newObj->status, newObj->X_BROADCOM_COM_LimitedConnections, currObj->X_BROADCOM_COM_LimitedConnections); 
         
         /* Need to let ssk know the change on the auto detect flag */
         rutWan_sendConnectionUpdateMsg(MDMOID_WAN_ETH_INTF, iidStack, FALSE, FALSE, FALSE, isAutoDetectChange);
      }
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if (newObj)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
      }

      /* disable wan port setting of switch */
      if (rutEth_setSwitchWanPort(currObj->X_BROADCOM_COM_IfName, FALSE) != CMSRET_SUCCESS)
      {
         cmsLog_error("Fail to disable wan port.");
         return CMSRET_INTERNAL_ERROR;
      }
      
      rutLan_disableInterface(currObj->X_BROADCOM_COM_IfName);
   }

   /* QoS port shaping */
   if(POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj)
      && (currObj->shapingRate != (SINT32) newObj->shapingRate))
   {
      rutQos_tmPortShaperCfg(newObj->X_BROADCOM_COM_IfName,
                             newObj->shapingRate,
                             newObj->shapingBurstSize,
                             newObj->status,
                             TRUE);
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_ipv6AddrObject( _IPv6AddrObject *newObj __attribute__((unused)),
                const _IPv6AddrObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* this is a stats object to hold ipv6 address info, so nothing to do in the RCL handler function. */
   return CMSRET_SUCCESS;
}


CmsRet rcl_wanEthIntfStatsObject( _WanEthIntfStatsObject *newObj __attribute__((unused)),
                const _WanEthIntfStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* this is a stats object, so probably nothing to do in the RCL handler function. */
   return CMSRET_SUCCESS;
}


CmsRet rcl_wanEthLinkCfgObject( _WanEthLinkCfgObject *newObj __attribute__((unused)),
                const _WanEthLinkCfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
return CMSRET_SUCCESS;
}
#endif  /* DMP_ETHERNETWAN_1 */

#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1

CmsRet rcl_wanWifiIntfObject( _WanWifiIntfObject *newObj,
                const _WanWifiIntfObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{

   cmsLog_debug("Entering...");

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* if we are enabling wl as wan, then we can upgrade
       * the link status to NOLINK.
       */
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_NOLINK, mdmLibCtx.allocFlags);
      if (newObj->ifName)
      {
         cmsLog_debug("ifName = %s is set",newObj->ifName);
      }
      else
      {
         cmsLog_error("enabled, but ifName not set yet");
      }
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      /* Send connection update msg to ssk only if the device link is up and auto detect flag changed */       
      if (!cmsUtl_strcmp(newObj->status, MDMVS_UP) &&
         newObj->limitedConnections != currObj->limitedConnections)
      {
         UBOOL8 isAutoDetectChange = TRUE;
         
         cmsLog_debug("Link status=%s, newObj->limitedConnections=%d, currobj->limitedConnections=%d",
         newObj->status, newObj->limitedConnections, currObj->limitedConnections); 
         
         /* Need to let ssk know the change on the auto detect flag */
         rutWan_sendConnectionUpdateMsg(MDMOID_WAN_WIFI_INTF, iidStack, FALSE, FALSE, FALSE, isAutoDetectChange);
      }
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      if (newObj)
      {
         REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->status, MDMVS_DISABLED, mdmLibCtx.allocFlags);
      }
   }

   return CMSRET_SUCCESS;
}


CmsRet rcl_wifiIPv6AddrObject( _WifiIPv6AddrObject *newObj,
                const _WifiIPv6AddrObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   /* this is a stats object to hold ipv6 address info, so nothing to do in the RCL handler function. */
   return CMSRET_SUCCESS;
}


CmsRet rcl_wanWifiIntfStatsObject( _WanWifiIntfStatsObject *newObj,
                const _WanWifiIntfStatsObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   /* this is a stats object, so probably nothing to do in the RCL handler function. */
   return CMSRET_SUCCESS;
}


CmsRet rcl_wanWifiLinkCfgObject( _WanWifiLinkCfgObject *newObj,
                const _WanWifiLinkCfgObject *currObj,
                const InstanceIdStack *iidStack,
                char **errorParam,
                CmsRet *errorCode)
{
   return CMSRET_SUCCESS;
}

#endif  /* DMP_X_BROADCOM_COM_WIFIWAN_1 */

#endif  /* DMP_BASELINE_1 */

