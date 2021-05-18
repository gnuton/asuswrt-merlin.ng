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


#ifdef DMP_DEVICE2_BASELINE_1

#ifdef  DMP_DEVICE2_PPPINTERFACE_1

#ifdef  DMP_DEVICE2_PPPINTERFACE_2

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_wan.h"
#include "rut2_ip.h"
#include "rut2_dns.h"
#include "rut2_route.h"
#include "cms_qdm.h"
#include "rut2_ppp.h"
#include "rut_iptables.h"
#include "rut_lan.h"


/*!\file rcl2_ppp.c
 * \brief This file contains device 2 device.PPP object related functions.
 *
 */

CmsRet rcl_dev2PppObject( _Dev2PppObject *newObj __attribute__((unused)),
                const _Dev2PppObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet fillPppIfName(const char *fullPath, char *ifName);
CmsRet fillPppIfName_igd(const char *fullPath, char *ifName);
CmsRet fillPppIfName_dev2(const char *fullPath, char *ifName);
#if defined(SUPPORT_DM_LEGACY98)
#define fillPppIfName(a, b)  !ERROR: this function should not be used in this mode
#elif defined(SUPPORT_DM_HYBRID)
#define fillPppIfName(a, b)  fillPppIfName_igd((a), (b))
#elif defined(SUPPORT_DM_PURE181)
#define fillPppIfName(a, b)  fillPppIfName_dev2((a), (b))
#elif defined(SUPPORT_DM_DETECT)
#define fillPppIfName(a, b)  (cmsMdm_isDataModelDevice2() ? \
                          fillPppIfName_dev2((a), (b)) : \
                          fillPppIfName_igd((a), (b)))
#endif

static UBOOL8 isPPPIntfResetFlagChanged(_Dev2PppInterfaceObject *newObj, const _Dev2PppInterfaceObject *currObj)
{
   UBOOL8 isPPPRest = FALSE;

   if (newObj->reset && !currObj->reset)
   {
      isPPPRest = TRUE;
   }

   return isPPPRest;
}

CmsRet rcl_dev2PppInterfaceObject( _Dev2PppInterfaceObject *newObj __attribute__((unused)),
                const _Dev2PppInterfaceObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 pppIntfReset = FALSE;

   cmsLog_debug("Enter");

   if (ADD_NEW(newObj, currObj))
   {
      rutUtil_modifyNumPppInterface(iidStack, 1);
   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      rutUtil_modifyNumPppInterface(iidStack, -1);
   }

   IF_STATUS_HAS_CHANGED_SET_LASTCHANGE(newObj, currObj);

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /*
       * Need to fill interface name if is empty
       */
      cmsLog_debug("In ENABLE_NEW_OR_ENABLE_EXISTING");

      if (!IS_EMPTY_STRING(newObj->lowerLayers))
      {
         if (IS_EMPTY_STRING(newObj->name))
         {
            char ifName[CMS_IFNAME_LENGTH]={0};

            if ((ret = fillPppIfName(newObj->lowerLayers, ifName)) != CMSRET_SUCCESS)
            {
               return ret;
            }

            CMSMEM_REPLACE_STRING_FLAGS(newObj->name, ifName, mdmLibCtx.allocFlags);
         }
      }
   }

   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      cmsLog_debug("POTENTIAL_CHANGE_OF_EXISTING ppp curr->ConnStatus=%s ==> new->ConnStatus=%s  with new->LastConnError=%s",
                   currObj->connectionStatus, newObj->connectionStatus, newObj->lastConnectionError);


      /* No action needed for IPCP/IPv6CP in hybrid mode since pppd is started by the TR98 code. */
      if (!cmsMdm_isDataModelDevice2())
      {
         return CMSRET_SUCCESS;
      }

      if ((pppIntfReset = isPPPIntfResetFlagChanged(newObj, currObj)) == TRUE)
      {
         cmsLog_debug("PPPoE reset is TRUE");
      }

      
      /* 3 Cases for POTENTIAL_CHANGE_OF_EXISTING
      * 1).   ==> "Connecting"
      * 2).   ==> "Connected"
      * 3).   ==> "Disconnected"
      */

       /* if old status is "Up", and new is not "Up", need to stop ppp .
          Occur when interface stack propagation lowlayer status, just set status instead of connectionStatus.
        */
       if (!cmsUtl_strcmp(currObj->status, MDMVS_UP) && 
            cmsUtl_strcmp(newObj->status, MDMVS_UP))
       {
           if ((!qdmIntf_isStatusUpOnFullPathLocked_dev2(currObj->lowerLayers))
                && (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTED) || 
                    !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTING) ||
                    !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_DISCONNECTED))
              )
           {
              cmsLed_setWanDisconnected();                   
              /* for delete or disable wan interface */
              rutPpp_stopPppd_dev2(currObj->X_BROADCOM_COM_Pid, currObj->lowerLayers);
           }
       }

      /* 
       * Case 1 
       */
      else if (!cmsUtl_strcmp(newObj->connectionStatus, MDMVS_CONNECTING))
      {
         if (cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTING)  ||
             !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_UNCONFIGURED) ||
             !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_DISCONNECTED))
         {
            /* Need to start pppd with
            * 1) null ==> "Connecting"
            * 2) "Unconfigured" ==> "Connecting"
            * 3) "Disconnected" ==> "Connecting"
            */
            if(cmsUtl_strstr(newObj->name,PPPOA_IFC_STR))
            {
                if ((ret = rutWan_startPPPoA(iidStack,  newObj)) != CMSRET_SUCCESS)
                {
                   cmsLog_error("rutWan_startPPPoA failed. ret %d", ret);
                }
            }else if ((ret = rutWan_initPPPoE(iidStack,  newObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutWan_initPPPoE failed. ret %d", ret);
            }
         }
      }

      /*
      * Case 2
      */
      else if (!cmsUtl_strcmp(newObj->connectionStatus, MDMVS_CONNECTED))
      {
         /*
         * ConnectionStatus is not changed but X_BROADCOM_COM_UserRequest is changed from "None"
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
            msg.dst = MAKE_SPECIFIC_EID(newObj->X_BROADCOM_COM_Pid, EID_PPP);
            msg.flags_request = 1;

            rc2 = cmsMsg_send(mdmLibCtx.msgHandle, &msg);
            if (rc2 != CMSRET_SUCCESS)
            {
               cmsLog_error("Fail to send disconnect msg to ppp %s, ret=%d", newObj->name, rc2);
            }

            /* restore to "None" */
            CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_UserRequest, MDMVS_NONE, mdmLibCtx.allocFlags);
         }
         else if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTING) ||
                  !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_DISCONNECTED))
         {
            /*
            * Need to setup the related WAN PPP services for
            * 1) "Connecting" ==> "Connected"
            * 2) "Disconnected" ==> "Connected" Note: from on-demand, pppd always send "Disconnected" first then Connected")
            */
            UBOOL8 isLayer2 = TRUE;
            char *pppIntfFullPath=NULL;
            char *ipIntfFullPath=NULL;
            _Dev2PppInterfaceIpcpObject *ipcpObj=NULL;

            if ((ret = qdmIntf_intfnameToFullPathLocked_dev2(newObj->name, isLayer2, &pppIntfFullPath)) != CMSRET_SUCCESS)
            {
               cmsLog_error("qdmIntf_intfnameToFullPathLocked_dev2 failed. ret %d", ret);
               return ret;
            }

            /* Get the IP.Interface object which this ppp client runs on */
            ret = rutPpp_getIpIntfFullPathByPppFullPath_dev2(pppIntfFullPath, &ipIntfFullPath);
            CMSMEM_FREE_BUF_AND_NULL_PTR(pppIntfFullPath);

            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("Could not get IP.Interface fullpath from PPP  fullpath %s", pppIntfFullPath);
               return CMSRET_INTERNAL_ERROR;
            }

            /* Need to get localIPAddress (wan ip),  DNSServers (nameserver), and  remoteIPAddress (gateway)
            * from  ipcpObj saved earlier on in addPppIncpObject in ssk2_connstatus.c
            */
            if ((ret= cmsObj_get(MDMOID_DEV2_PPP_INTERFACE_IPCP,
                                 iidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **) &ipcpObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Could not get MDMOID_DEV2_PPP_INTERFACE_IPCP. ret %d", ret);
               CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
               return ret;
            }

            /* Configured the ip interface with the information from ipcp object.
            * This configuration part is same as IPoE which is done in rcl2_dhcpv4.c when
            * dhcpc has the wan ipaddress, dns and gateway info.
            */

            /* 
             * Both IPCP and IPv6CP may set this object status to connected. Need to check whether
             * IPCP object has proper information before updating data model
             */
            if (!cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPV4, 
                                          ipcpObj->localIPAddress))
            {

               /* Do we need this ???
               *
               * Adding ip address of PPP interface to IP interface is not 
               * really needed but does not seem to be a problem if an extra
               * command such as
               * "ifconfig ppp0 10.6.37.193 netmask 255.255.255.255 broadcast 10.6.37.255 up".  By doing this
               * IP interface will have the ipv4Adress object which is 
               * symmetrical with dynamtic IPoE.
               */
               if ((ret = rutIp_addIpv4AddressObject_dev2(ipIntfFullPath,
                                                       ipcpObj->localIPAddress,
                                                       "255.255.255.255",          /* PPPoE subnet mask seems like this  */
                                                       MDMVS_IPCP)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("rutIp_addIpv4AddressObject_dev2 failed. ret %d", ret);
               }


               if ((ret = rutDns_addServerObject_dev2(ipIntfFullPath,
                                                   ipcpObj->DNSServers,
                                                   MDMVS_IPCP)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("rutDns_addServerObject_dev2 failed. ret %d", ret);
               }


               if ((ret = rutRt_addIpv4ForwardingObject_dev2(ipIntfFullPath,
                                                             ipcpObj->remoteIPAddress,
                                                             MDMVS_IPCP)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("rutRt_addIpv4ForwardingObject_dev2 failed. ret %d", ret);
               }
            }

            cmsObj_free((void **) &ipcpObj);

            CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
         }

         else if (pppIntfReset)
         {
            UBOOL8 isLayer2 = TRUE;
            char *pppIntfFullPath=NULL;
            char propageLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};

            cmsLog_debug("Doing PPPoE Reset process...");

            /* 1). Need to disconnect pppd by setting this PPP interface ->connectionStatus to "Disconnected" and
            *      ->status to "Down"
            *  2). Reset Reset flag back to FALSE per TR181 spec.
            *  3)  Propagate pppoe down information to IP Interface to shut down the ip stack by
            *      calling rutIp_sendIntfStackPropagateMsgToSsk which is a non-blocking call.
            *  4). Real pppd shutdown action by calling rutPpp_stopPppd_dev2.
            *  5). Change the ppp interface ->status again to "Lowerlayerdown" and then call rutIp_sendIntfStackPropagateMsgToSsk
            *      again to kick the ip stack to restart this PPPoE/ip connection.
            */

            if ((ret = qdmIntf_intfnameToFullPathLocked_dev2(newObj->name, isLayer2, &pppIntfFullPath)) != CMSRET_SUCCESS)
            {
               cmsLog_error("qdmIntf_intfnameToFullPathLocked_dev2 failed. ret %d", ret);
               return ret;
            }

            cmsLog_debug("Seting connStatus to MDMVS_DISCONNECTED and newObj->status to MDMVS_DOWN.");
            CMSMEM_REPLACE_STRING_FLAGS(newObj->connectionStatus, MDMVS_DISCONNECTED, mdmLibCtx.allocFlags);
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
            newObj->reset = FALSE;

            /* This will shut down ip stack */
            rutIp_sendIntfStackPropagateMsgToSsk(pppIntfFullPath);

            /* Free this pppIntfFullPath buffer */
            CMSMEM_FREE_BUF_AND_NULL_PTR(pppIntfFullPath);

            /* stop pppd */
            rutPpp_stopPppd_dev2(newObj->X_BROADCOM_COM_Pid, newObj->lowerLayers);

            /* Need to start pppoe/ip again */
            cmsLog_debug("Set lowerlayer down and progagate again to start PPPoE/IP stack");
            CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_LOWERLAYERDOWN, mdmLibCtx.allocFlags);
            cmsUtl_strncpy(propageLayerBuf, newObj->lowerLayers, sizeof(propageLayerBuf));
            rutIp_sendIntfStackPropagateMsgToSsk(propageLayerBuf);

            cmsLog_debug("Done pppoe reset process");

            ret = CMSRET_SUCCESS;
         }

      }

      /*
      * Case 3
      */
      else if (!cmsUtl_strcmp(newObj->connectionStatus, MDMVS_DISCONNECTED))
      {
         /*
         * ConnectionStatus is not changed but X_BROADCOM_COM_UserRequest is changed from "None"
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
            msg.dst = MAKE_SPECIFIC_EID(newObj->X_BROADCOM_COM_Pid, EID_PPP);
            msg.flags_request = 1;

            rc2 = cmsMsg_send(mdmLibCtx.msgHandle, &msg);
            if (rc2 != CMSRET_SUCCESS)
            {
               cmsLog_error("Fail to send Connect msg to ppp %s, ret=%d", newObj->name, rc2);
            }

            /* restore to "None" */
            CMSMEM_REPLACE_STRING_FLAGS(newObj->X_BROADCOM_COM_UserRequest, MDMVS_NONE, mdmLibCtx.allocFlags);
         }
         else if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTED) ||
                  !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTING) ||
                  !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_DISCONNECTED))
         {
            cmsLog_debug("CurrentConnstatus  is  %s", currObj->connectionStatus);


            /* Even if connectionStatus is already "Disconnected", it might be from ppp server is down and pppd
            * may still be in the memory, and need to be stopped if the layer 2 link is down (in connstatus.c)
            */
            if (!cmsUtl_strcmp(newObj->lastConnectionError, MDMVS_ERROR_AUTHENTICATION_FAILURE))
            {
               /* for authentication failure, need to set LED differently */
               cmsLed_setWanFailed();
            }
            /* From CONNECTED/CONNECTING ==> DISCONNECED
            * always tear down Wan ppp services associated with this WAN connection
            */
            // XXX do we still need this?  If so, this should be in IP interface and set by ssk2_connstatus
            //cmsLog_debug("link is %s", newObj->X_BROADCOM_COM_TransientLayer2LinkStatus);

            /* Only stop pppd when layer 2 link is down (X_BROADCOM_COM_TransientLayer2LinkStatus from ssk)
            * Otherwise, keep pppd running when ConnectionStatus = "Disconnected" for following case:
            * 1). ppp connection is down - lastConnectionError = "ERROR_UNKNOWN",
            * 
            *  OR in auto detect mode and ppp cannot be up in the specified time frame, stop pppd as well
            *  X_BROADCOM_COM_StopPppD is set to TRUE during in ssk when setting connectionStatus
            * to "Connecting" OR disabled by manual selection.
            */
            {
               //XXX    later for auto-detect           cmsLog_debug("STOP_PPPD(newObj)=%d", STOP_PPPD(newObj));
               //XXX               if (!rutWan_isTransientLayer2LinkUp(newObj->X_BROADCOM_COM_TransientLayer2LinkStatus) || STOP_PPPD(newObj))

               if (!qdmIntf_isStatusUpOnFullPathLocked_dev2(newObj->lowerLayers))
               {  
                  /* layer 2 link is down --> need to stop pppd */
                   
                  rutPpp_stopPppd_dev2(newObj->X_BROADCOM_COM_Pid, newObj->lowerLayers);
                  newObj->X_BROADCOM_COM_Pid = CMS_INVALID_PID;
               }                  
            }

 #ifdef DMP_X_BROADCOM_COM_AUTODETECTION_1
            /* XXX later.  reset it to FALSE */
            newObj->X_BROADCOM_COM_StopPppD = FALSE;
 #endif /* DMP_X_BROADCOM_COM_AUTODETECTION_1 */
         }
      }

   }

   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLed_setWanDisconnected();
      if (!cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTED) ||
          !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_CONNECTING) ||
          !cmsUtl_strcmp(currObj->connectionStatus, MDMVS_DISCONNECTED))
      {
         /* Only do stop when currObj->connectionStatus is CONNECTED/CONNECTING */

         /* for delete or disable wan interface */
         rutPpp_stopPppd_dev2(currObj->X_BROADCOM_COM_Pid, currObj->lowerLayers);

         /* release the external ip address and default gateway,etc. if it is not a delete */
         if (newObj)
         {
            if (!newObj->enable)
            {
               cmsLog_debug("Seting connStatus to MDMVS_DISCONNECTED if it is disabled by users.");
               CMSMEM_REPLACE_STRING_FLAGS(newObj->connectionStatus, MDMVS_DISCONNECTED, mdmLibCtx.allocFlags);
               CMSMEM_REPLACE_STRING_FLAGS(newObj->status, MDMVS_DOWN, mdmLibCtx.allocFlags);
            }
         }

#ifdef DMP_X_BROADCOM_COM_IPV6_1 /* aka SUPPORT_IPV6 */
      // XXX todo rutWan_ipv6PppConnDisable(newObj, currObj, iidStack);
#endif
      }
   }

    /* do dhcpd updating during adding or deleteing Wan service */
   if (rutLan_updateDhcpd() != CMSRET_SUCCESS)
   {
      cmsLog_error("rutLan_updateDhcpd fail");
   }

   cmsLog_debug("Exit ret %d", ret);

   return ret;

}

CmsRet rcl_dev2PppInterfacePpoaObject( _Dev2PppInterfacePpoaObject *newObj __attribute__((unused)),
                const _Dev2PppInterfacePpoaObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}

CmsRet rcl_dev2PppInterfacePpoeObject( _Dev2PppInterfacePpoeObject *newObj __attribute__((unused)),
                const _Dev2PppInterfacePpoeObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}



CmsRet rcl_dev2PppInterfaceIpcpObject( _Dev2PppInterfaceIpcpObject *newObj __attribute__((unused)),
                const _Dev2PppInterfaceIpcpObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}


CmsRet rcl_dev2PppInterfaceIpv6cpObject( _Dev2PppInterfaceIpv6cpObject *newObj __attribute__((unused)),
                const _Dev2PppInterfaceIpv6cpObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}



CmsRet rcl_dev2PppInterfaceStatsObject( _Dev2PppInterfaceStatsObject *newObj __attribute__((unused)),
                const _Dev2PppInterfaceStatsObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   return CMSRET_SUCCESS;
}




CmsRet fillPppIfName_dev2(const char *fullPath, char *ifName)
{
    char lowLayerIfName[CMS_IFNAME_LENGTH]={0};
    char pppIfName[CMS_IFNAME_LENGTH]={0};
    char *p;
    CmsRet ret=CMSRET_SUCCESS;

    if ((ret = qdmIntf_fullPathToIntfnameLocked(fullPath, lowLayerIfName)) != CMSRET_SUCCESS)
    {
       cmsLog_error("qdmIntf_getIntfnameFromFullPathLocked failed. ret %d", ret);
       return ret;
    }

    p = strchr(lowLayerIfName, '.');
    if ((ret = rutWan_fillPppIfName((p) ? TRUE:FALSE , pppIfName)) != CMSRET_SUCCESS)
    {
       cmsLog_error("rutWan_fillPppIfName failed. ret %d", ret);
       return ret;
    }

    /* If lowlayer ifName is vlan interface (.?), need to create pppx.? */
    if (p)
    {
       sprintf(ifName, "%s%s", pppIfName, p);
    }
    else
    {
      sprintf(ifName, "%s", pppIfName);
    }

   return ret;

}

CmsRet fillPppIfName_igd(const char *fullPath, char *ifName)
{
   UBOOL8 isPPP=TRUE;
   return qdmIntf_getIfNameFromBottomLayer(isPPP, fullPath, ifName);
}

#endif  /* DMP_DEVICE2_PPPINTERFACE_1 */

#endif  /* DMP_DEVICE2_PPPINTERFACE_2 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
