/***********************************************************************
 *
 *  Copyright (c) 2006-2008  Broadcom Corporation
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

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_iptables.h"

#ifdef DMP_X_BROADCOM_COM_SECURITY_1


/*
 * This is the outgoing filter from the LAN side.
 */
CmsRet rcl_ipFilterCfgObject( _IpFilterCfgObject*newObj,
                                 const _IpFilterCfgObject *currObj,
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   _IpFilterCfgObject *ipFilterCfg = NULL;
   _LanIpIntfObject *ipIntfObj = NULL;
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack parentIidStack = *iidStack;
   char ifName[CMS_IFNAME_LENGTH]={0};

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   /* TODO!!! For now, only support br0, we will support multi-subnet later */
   if (cmsObj_getAncestor(MDMOID_LAN_IP_INTF, MDMOID_IP_FILTER_CFG,
                                                     &parentIidStack, (void **) &ipIntfObj) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get LanIpIntfObject. ret=%d", ret);
      return CMSRET_INTERNAL_ERROR;
   }

   strncpy(ifName, ipIntfObj->X_BROADCOM_COM_IfName, sizeof(ifName)-1);
   cmsObj_free((void **) &ipIntfObj);

   if (newObj != NULL && newObj->enable)
   {
      cmsLog_debug("Enabling IP filter out feature");

      if (cmsUtl_strcmp(newObj->filterName, "\0") == 0)
      {
         cmsLog_error("Invalid filter name");
         return CMSRET_INVALID_ARGUMENTS;
      }

      if ((cmsUtl_strcmp(newObj->sourceIPAddress, "\0") != 0 && cmsUtl_isValidIpAddress(AF_INET, newObj->sourceIPAddress) == FALSE) ||\
           (cmsUtl_strcmp(newObj->destinationIPAddress, "\0") != 0 && cmsUtl_isValidIpAddress(AF_INET, newObj->destinationIPAddress) == FALSE) )
      {
         cmsLog_error("Invalid IP address");
         return CMSRET_INVALID_ARGUMENTS;
      }

      if (newObj->sourcePortStart > (64*1024-1) || newObj->sourcePortEnd > (64*1024-1) || \
          newObj->destinationPortStart > (64*1024-1) || newObj->destinationPortEnd > (64*1024-1) )
      {
         cmsLog_error("Invalid port number");
         return CMSRET_INVALID_ARGUMENTS;
      }

      if (currObj != NULL)
      {
         while (cmsObj_getNextInSubTree(MDMOID_IP_FILTER_CFG, &parentIidStack, &iidStack1,
                                                 (void **)&ipFilterCfg)  == CMSRET_SUCCESS)
         {
            if(cmsMdm_compareIidStacks(&iidStack1, iidStack) != 0)
            {
               if ( cmsUtl_strcmp(ipFilterCfg->filterName, newObj->filterName) == 0 )
               {
                  cmsLog_error("filter name exists already");
                  cmsObj_free((void **) &ipFilterCfg);
                  return CMSRET_INVALID_PARAM_VALUE;
               }
               else if (cmsUtl_strcmp(ipFilterCfg->sourceIPAddress, newObj->sourceIPAddress) == 0 && \
                           cmsUtl_strcmp(ipFilterCfg->sourceNetMask, newObj->sourceNetMask) == 0 &&\
                           cmsUtl_strcmp(ipFilterCfg->destinationIPAddress, newObj->destinationIPAddress) == 0 && \
                           cmsUtl_strcmp(ipFilterCfg->destinationNetMask, newObj->destinationNetMask) == 0 &&\
                           cmsUtl_strcmp(ipFilterCfg->protocol, newObj->protocol) == 0 && \
                           cmsUtl_strcmp(ipFilterCfg->IPVersion, newObj->IPVersion) == 0 && \
                           ipFilterCfg->sourcePortStart == newObj->sourcePortStart && ipFilterCfg->sourcePortEnd == newObj->sourcePortEnd &&\
                           ipFilterCfg->destinationPortStart == newObj->destinationPortStart && ipFilterCfg->destinationPortEnd == newObj->destinationPortEnd)
               {
                  cmsLog_error("filter rule exists already");
                  cmsObj_free((void **) &ipFilterCfg);
                  return CMSRET_INVALID_ARGUMENTS;
               }
               cmsObj_free((void **) &ipFilterCfg);
            }
         }
      }

      rutIpt_addIpFilterOut(newObj, ifName, TRUE);
   }

   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting IP filter out feature");

      /*
       * The set on the object from dalSec_addIpFilter could have failed.
       * In that case, filterName would be NULL, so don't take
       * any action since it was not put there in the first place.
       */
      if(currObj->filterName != NULL)
      {
         rutIpt_addIpFilterOut(currObj, ifName, FALSE);
      }
   }

   return ret;
}



/** Return true if this firewall exception object has valid fields.
 *
 * The field values checking is more complicated than the automatic MDM
 * checking can handle, so it is done in this special function.
 *
 * Note we are taking advantage of the fact that all 3 firewall exception
 * objects, (WanPppConnFirewallException, WanIpConnFirewallException, and
 * LanIpIntfFirewallException) are exactly the same, so we can use this
 * common function.  Even though the first param is of type
 * _WanPppConnFirewallExceptionObject, you can actually pass in any of
 * the 3 firewall exception objects.)
 */
static UBOOL8 isFirewallExceptionValid(_WanPppConnFirewallExceptionObject *commonExceptionObj)
{
   if (cmsUtl_strcmp(commonExceptionObj->filterName, "\0") == 0)
   {
      cmsLog_error("Invalid filter name");
      return FALSE;
   }

   if (cmsUtl_strcmp(commonExceptionObj->sourceIPAddress, "\0") != 0)
   {
      if (atoi(commonExceptionObj->IPVersion) == 4)
      {
         if (cmsUtl_isValidIpAddress(AF_INET, commonExceptionObj->sourceIPAddress) == FALSE)
         {
            cmsLog_error("Invalid IPv4 source address");
            return FALSE;
         }
      }
      else
      {
         if (cmsUtl_isValidIpAddress(AF_INET6, commonExceptionObj->sourceIPAddress) == FALSE)
         {
            cmsLog_error("Invalid IPv6 source address");
            return FALSE;
         }
      }
   }

   if (cmsUtl_strcmp(commonExceptionObj->destinationIPAddress, "\0") != 0)
   {
      if (atoi(commonExceptionObj->IPVersion) == 4)
      {
         if (cmsUtl_isValidIpAddress(AF_INET, commonExceptionObj->destinationIPAddress) == FALSE)
         {
            cmsLog_error("Invalid IPv4 destination address");
            return FALSE;
         }
      }
      else
      {
         if (cmsUtl_isValidIpAddress(AF_INET6, commonExceptionObj->destinationIPAddress) == FALSE)
         {
            cmsLog_error("Invalid IPv6 destination address");
            return FALSE;
         }
      }
   }

   /*
    * Simple port number range checking can be done in the MDM.
    * The relationship between portStart and portEnd must be done here.
    */
   if ((commonExceptionObj->sourcePortEnd != 0 && commonExceptionObj->sourcePortEnd < commonExceptionObj->sourcePortStart) ||
       (commonExceptionObj->destinationPortEnd != 0 && commonExceptionObj->destinationPortEnd < commonExceptionObj->destinationPortStart))
   {
      cmsLog_error("Invalid port number or start/end relationship");
      return FALSE;
   }

   return TRUE;
}


/** Return true if this firewall exception object is a duplicate of the other.
 *
 * Aagain, we are taking advantage of the fact that all 3 firewall exception
 * objects are exactly the same.
 */
static UBOOL8 isFirewallExceptionSame(_WanPppConnFirewallExceptionObject *obj1,
                                      _WanPppConnFirewallExceptionObject *obj2)
{

   if ( cmsUtl_strcmp(obj1->filterName, obj2->filterName) == 0 )
   {
      cmsLog_error("filter name exists already");
      return TRUE;
   }
   else if (cmsUtl_strcmp(obj1->sourceIPAddress, obj2->sourceIPAddress) == 0 &&
            cmsUtl_strcmp(obj1->sourceNetMask, obj2->sourceNetMask) == 0 &&
            cmsUtl_strcmp(obj1->destinationIPAddress, obj2->destinationIPAddress) == 0 &&
            cmsUtl_strcmp(obj1->destinationNetMask, obj2->destinationNetMask) == 0 &&
            cmsUtl_strcmp(obj1->protocol, obj2->protocol) == 0 && 
            cmsUtl_strcmp(obj1->IPVersion, obj2->IPVersion) == 0 && 
            obj1->sourcePortStart == obj2->sourcePortStart &&
            obj1->sourcePortEnd == obj2->sourcePortEnd &&
            obj1->destinationPortStart == obj2->destinationPortStart &&
            obj1->destinationPortEnd == obj2->destinationPortEnd)
   {
      cmsLog_error("filter rule exists already");
      return TRUE;
   }

   return FALSE;
}



CmsRet rcl_lanIpIntfFirewallExceptionObject( _LanIpIntfFirewallExceptionObject*newObj,
                                 const _LanIpIntfFirewallExceptionObject *currObj,
                                 const InstanceIdStack *iidStack,
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   _LanIpIntfObject *ipIntfObj = NULL;
   InstanceIdStack parentIidStack = *iidStack;
   char ifName[CMS_IFNAME_LENGTH]={0};
   UBOOL8 isFirewallEnabled;

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   if (cmsObj_getAncestor(MDMOID_LAN_IP_INTF, MDMOID_LAN_IP_INTF_FIREWALL_EXCEPTION,
                          &parentIidStack, (void **) &ipIntfObj) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get LanIpIntfObject. ret=%d", ret);
      return CMSRET_INTERNAL_ERROR;
   }

   strncpy(ifName, ipIntfObj->X_BROADCOM_COM_IfName, sizeof(ifName)-1);
   isFirewallEnabled = ipIntfObj->X_BROADCOM_COM_FirewallEnabled;
   cmsObj_free((void **) &ipIntfObj);

   if (newObj != NULL && newObj->enable)
   {
      cmsLog_debug("IP filter In on %s is enabled, currObj=%p", ifName, currObj);

      if (!isFirewallExceptionValid((_WanPppConnFirewallExceptionObject *) newObj))
      {
         return CMSRET_INVALID_ARGUMENTS;
      }

      if (currObj != NULL)
      {
         UBOOL8 valid=TRUE;
         InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
         _LanIpIntfFirewallExceptionObject *firewallobj = NULL;

         /*
          * By checking for currObj != NULL, we are saying we don't execute
          * this block if this function is called during modem bootup
          * and for object creation.  (During these two conditions, the
          * currObj is NULL.)
          */
         while (valid &&
                (cmsObj_getNextInSubTree(MDMOID_LAN_IP_INTF_FIREWALL_EXCEPTION, &parentIidStack, &iidStack1,
                                         (void **)&firewallobj)  == CMSRET_SUCCESS))
         {
            if(cmsMdm_compareIidStacks(&iidStack1, iidStack))
            {
               /* this is a different object instance than me, does it have same values as me? */
               if (isFirewallExceptionSame((_WanPppConnFirewallExceptionObject *)newObj,
                                           (_WanPppConnFirewallExceptionObject *)firewallobj))
               {
                  valid = FALSE;
               }
            }

            cmsObj_free((void **) &firewallobj);
         }

         if (!valid)
         {
            return CMSRET_INVALID_ARGUMENTS;
         }
      }
      
      /*
       * Add the FirewallException rule.
       * Note we don't check if the fields have changed, which we normally do.
       * This is because rut_pppConnectionUp->rutIpt_initFirewall relies
       * on causing a system action by simply doing a "set".  This is not
       * the best way to do it, but I don't want to make a big change now.
       *
       * Extra isFirewallEnabled check is because we allow exception objects
       * to get added before the firewall is enabled.  So don't insert the
       * rule if the firewall is not enabled yet.
       */
      if (isFirewallEnabled)
      {
        rutIpt_doIpFilterIn(newObj, ifName, TRUE);
      }
   }

   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting/Disable IP filter in feature on %s", ifName);

      /*
       * The set on the object from dalSec_addIpFilter could have failed.
       * In that case, filterName would be NULL, so don't take
       * any action since it was not put there in the first place.
       */
      if(currObj->filterName != NULL)
      {
         rutIpt_doIpFilterIn(currObj, ifName, FALSE);
      }
   }

   return ret;

}


CmsRet rcl_wanPppConnFirewallExceptionObject( _WanPppConnFirewallExceptionObject*newObj,
                                 const _WanPppConnFirewallExceptionObject *currObj,
                                 const InstanceIdStack *iidStack,
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   _WanPppConnObject *wan_ppp_conn = NULL;
   InstanceIdStack parentIidStack = *iidStack;
   char ifName[CMS_IFNAME_LENGTH]={0};

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   if (cmsObj_getAncestor(MDMOID_WAN_PPP_CONN, MDMOID_WAN_PPP_CONN_FIREWALL_EXCEPTION,
                          &parentIidStack, (void **) &wan_ppp_conn) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get WanPppConnObject. ret=%d", ret);
      return CMSRET_INTERNAL_ERROR;
   }

   strncpy(ifName, wan_ppp_conn->X_BROADCOM_COM_IfName, sizeof(ifName)-1);
   cmsObj_free((void **) &wan_ppp_conn);

   if (newObj != NULL && newObj->enable)
   {
      cmsLog_debug("IP filter In on %s is enabled, currObj=%p", ifName, currObj);

      if (!isFirewallExceptionValid(newObj))
      {
         return CMSRET_INVALID_ARGUMENTS;
      }

      if (currObj != NULL)
      {
         UBOOL8 valid=TRUE;
         InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
         _WanPppConnFirewallExceptionObject *firewallobj = NULL;

         /*
          * By checking for currObj != NULL, we are saying we don't execute
          * this block if this function is called during modem bootup
          * and for object creation.  (During these two conditions, the
          * currObj is NULL.)
          */
         while (valid &&
                (cmsObj_getNextInSubTree(MDMOID_WAN_PPP_CONN_FIREWALL_EXCEPTION, &parentIidStack, &iidStack1,
                                         (void **)&firewallobj)  == CMSRET_SUCCESS))
         {
            if(cmsMdm_compareIidStacks(&iidStack1, iidStack))
            {
               /* this is a different object instance than me, does it have same values as me? */
               if (isFirewallExceptionSame(newObj, firewallobj))
               {
                  valid = FALSE;
               }
            }

            cmsObj_free((void **) &firewallobj);
         }

         if (!valid)
         {
            return CMSRET_INVALID_ARGUMENTS;
         }
      }
      
      /*
       * Add the FirewallException rule.
       * Note we don't check if the fields have changed, which we normally do.
       * This is because rut_pppConnectionUp->rutIpt_initFirewall relies
       * on causing a system action by simply doing a "set".  This is not
       * the best way to do it, but I don't want to make a big change now.
       */
      rutIpt_doIpFilterIn(newObj, ifName, TRUE);
   }

   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting/Disable IP filter in feature on %s", ifName);

      /*
       * The set on the object from dalSec_addIpFilter could have failed.
       * In that case, filterName would be NULL, so don't take
       * any action since it was not put there in the first place.
       */
      if(currObj->filterName != NULL)
      {
         rutIpt_doIpFilterIn(currObj, ifName, FALSE);
      }
   }

   return ret;
}


CmsRet rcl_wanIpConnFirewallExceptionObject( _WanIpConnFirewallExceptionObject*newObj,
                                 const _WanIpConnFirewallExceptionObject *currObj,
                                 const InstanceIdStack *iidStack,
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   _WanIpConnObject *wan_ip_conn = NULL;
   InstanceIdStack parentIidStack = *iidStack;
   char ifName[CMS_IFNAME_LENGTH]={0};

   cmsLog_debug("entered, iidStack=%s", cmsMdm_dumpIidStack(iidStack));

   if (cmsObj_getAncestor(MDMOID_WAN_IP_CONN, MDMOID_WAN_IP_CONN_FIREWALL_EXCEPTION,
                          &parentIidStack, (void **) &wan_ip_conn) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get WanIpConnObject. ret=%d", ret);
      return CMSRET_INTERNAL_ERROR;
   }

   strncpy(ifName, wan_ip_conn->X_BROADCOM_COM_IfName, sizeof(ifName)-1);
   cmsObj_free((void **) &wan_ip_conn);

   if (newObj != NULL && newObj->enable)
   {
      cmsLog_debug("IP filter In on %s is enabled, currObj=%p", ifName, currObj);

      if (!isFirewallExceptionValid((_WanPppConnFirewallExceptionObject *) newObj))
      {
         return CMSRET_INVALID_ARGUMENTS;
      }

      if (currObj != NULL)
      {
         UBOOL8 valid=TRUE;
         InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
         _WanIpConnFirewallExceptionObject *firewallobj = NULL;

         /*
          * By checking for currObj != NULL, we are saying we don't execute
          * this block if this function is called during modem bootup
          * and for object creation.  (During these two conditions, the
          * currObj is NULL.)
          */
         while (valid &&
                (cmsObj_getNextInSubTree(MDMOID_WAN_IP_CONN_FIREWALL_EXCEPTION, &parentIidStack, &iidStack1,
                                         (void **)&firewallobj)  == CMSRET_SUCCESS))
         {
            if(cmsMdm_compareIidStacks(&iidStack1, iidStack))
            {
               /* this is a different object instance than me, does it have same values as me? */
               if (isFirewallExceptionSame((_WanPppConnFirewallExceptionObject *)newObj,
                                           (_WanPppConnFirewallExceptionObject *)firewallobj))
               {
                  valid = FALSE;
               }
            }

            cmsObj_free((void **) &firewallobj);
         }

         if (!valid)
         {
            return CMSRET_INVALID_ARGUMENTS;
         }
      }
      
      /*
       * Add the FirewallException rule.
       * Note we don't check if the fields have changed, which we normally do.
       * This is because rut_pppConnectionUp->rutIpt_initFirewall relies
       * on causing a system action by simply doing a "set".  This is not
       * the best way to do it, but I don't want to make a big change now.
       */
      rutIpt_doIpFilterIn(newObj, ifName, TRUE);
   }

   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      cmsLog_debug("Deleting/Disable IP filter in feature on %s", ifName);

      /*
       * The set on the object from dalSec_addIpFilter could have failed.
       * In that case, filterName would be NULL, so don't take
       * any action since it was not put there in the first place.
       */
      if(currObj->filterName != NULL)
      {
         rutIpt_doIpFilterIn(currObj, ifName, FALSE);
      }
   }

   return ret;

}


#endif  /* DMP_X_BROADCOM_COM_SECURITY_1 */

