/***********************************************************************
 *
 *  Copyright 2011, Broadcom Corporation
 *
<:label-BRCM:2011:proprietary:standard

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

#ifdef SUPPORT_QOS

/** This file contains a mix of common QoS function and TR98 specific
 *  Qos functions.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <ctype.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "bcmnet.h"
#include "cms_core.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_eid.h"
#include "cms_qos.h"
#include "cms_dal.h"
#include "cms_boardcmds.h"
#include "rcl.h"
#include "odl.h"
#include "rut_util.h"
#include "rut_qos.h"
#include "rut_atm.h"
#include "rut_dsl.h"
#include "rut_wan.h"
#include "rut_wanlayer2.h"
#include "rut_xtmlinkcfg.h"
#include "devctl_xtm.h"
#include "skb_defines.h"
#include "rut_ethswitch.h"
#include "vlanctl_api.h"




/** Local functions **/
static CmsRet rutQos_roLocalClass(const CmsQosClassInfo *clsObj,
                                  QosCommandType cmdType);
static CmsRet rutQos_brLocalClass(const CmsQosClassInfo *clsObj,
                                  QosCommandType cmdType);
static CmsRet rutQos_roIngressClass(const CmsQosClassInfo *clsObj,
                                    QosCommandType cmdType);
static CmsRet rutQos_brIngressClass(const CmsQosClassInfo *clsObj,
                                    QosCommandType cmdType);
static CmsRet rutQos_roEgressMark(const CmsQosClassInfo *clsObj,
                                  UINT32 priority,
                                  QosCommandType cmdType);
static CmsRet rutQos_brEgressMark(const CmsQosClassInfo *clsObj,
                                  UINT32 priority,
                                  UBOOL8 egressWlan,
                                  QosCommandType cmdType);
#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
static CmsRet rutQos_vlanTag(const CmsQosClassInfo *clsObj,
                             const char *l2IfName, QosCommandType cmdType);
#endif
static void rutQos_sendDhcp(SINT32 type, char *text);



CmsRet rutQos_qMgmtClassConfig(QosCommandType cmdType, const void *cObj)
{
   CmsQosClassInfo classInfo;
   CmsQosClassInfo *cInfo = &classInfo;
   char egressQueueIntfNameBuf[CMS_IFNAME_LENGTH]={0};
   UBOOL8 egressQueueEnabled=FALSE;
#ifdef SUPPORT_POLICING
   CmsQosPolicerInfo policerInfo;
#endif
   UINT32 priority = 0;
   UBOOL8 egressWlan = FALSE;
   CmsRet ret;

   /* convert TR98 or TR181 QoS qObj to common classInfo struct */
   qdmQos_convertDmClassObjToCmsClassInfoLocked(cObj, cInfo);

   cmsLog_debug("%s classification instance:", (cmdType == QOS_COMMAND_CONFIG)? "Config" : "Unconfig");
   cmsLog_debug("name=%s", cInfo->name);
   cmsLog_debug("classificationEnable=%d", cInfo->enable);
   cmsLog_debug("classificationOrder=%d", cInfo->order);
   cmsLog_debug("ethertype=%x", cInfo->etherType);
   cmsLog_debug("classInterface=%s", cInfo->ingressIntfFullPath);
   cmsLog_debug("destIP=%s", cInfo->destIP);
   cmsLog_debug("destMask=%s", cInfo->destMask);
   cmsLog_debug("destIPExclude=%d", cInfo->destIPExclude);
   cmsLog_debug("sourceIP=%s", cInfo->sourceIP);
   cmsLog_debug("sourceMask=%s", cInfo->sourceMask);
   cmsLog_debug("sourceIPExclude=%d", cInfo->sourceIPExclude);
   cmsLog_debug("protocol=%d", cInfo->protocol);
   cmsLog_debug("protocolExclude=%d", cInfo->protocolExclude);
   cmsLog_debug("destPort=%d", cInfo->destPort);
   cmsLog_debug("destPortRangeMax=%d", cInfo->destPortRangeMax);
   cmsLog_debug("destPortExclude=%d", cInfo->destPortExclude);
   cmsLog_debug("sourcePort=%d", cInfo->sourcePort);
   cmsLog_debug("sourcePortRangeMax=%d", cInfo->sourcePortRangeMax);
   cmsLog_debug("sourcePortExclude=%d", cInfo->sourcePortExclude);
   cmsLog_debug("destMACAddress=%s", cInfo->destMACAddress);
   cmsLog_debug("destMACMask=%s", cInfo->destMACMask);
   cmsLog_debug("destMACExclude=%d", cInfo->destMACExclude);
   cmsLog_debug("sourceMACAddress=%s", cInfo->sourceMACAddress);
   cmsLog_debug("sourceMACMask=%s", cInfo->sourceMACMask);
   cmsLog_debug("sourceMACExclude=%d", cInfo->sourceMACExclude);
   cmsLog_debug("sourceVendorClassID=%s", cInfo->sourceVendorClassID);
   cmsLog_debug("sourceVendorClassIDExclude=%d", cInfo->sourceVendorClassIDExclude);
   cmsLog_debug("sourceUserClassID=%s", cInfo->sourceUserClassID);
   cmsLog_debug("sourceUserClassIDExclude=%d", cInfo->sourceUserClassIDExclude);
   cmsLog_debug("DSCPCheck=%d", cInfo->DSCPCheck);
   cmsLog_debug("DSCPExclude=%d", cInfo->DSCPExclude);
   cmsLog_debug("ethernetPriorityCheck=%d", cInfo->ethernetPriorityCheck);
   cmsLog_debug("ethernetPriorityExclude=%d", cInfo->ethernetPriorityExclude);
   cmsLog_debug("DSCPMark=%d", cInfo->DSCPMark);
   cmsLog_debug("ethernetPriorityMark=%d", cInfo->ethernetPriorityMark);
   cmsLog_debug("classQueue=%d", cInfo->egressQueueInstance);
   cmsLog_debug("classPolicer=%d", cInfo->policerInstance);

   /* config the classification rule only if it is enabled */
   if (cmdType == QOS_COMMAND_CONFIG && !cInfo->enable)
   {
      cmsLog_debug("classification is not enabled. No action is taken.");
      return CMSRET_SUCCESS;
   }

   if (cInfo->egressQueueInstance > 0)
   {
      if ((ret = qdmQos_getQueueInfoByClassQueueLocked(cInfo->egressQueueInstance,
                                                       &egressQueueEnabled,
                                                       &priority,
                                                       egressQueueIntfNameBuf)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmQos_getQueueInfoByClassQueueLocked on %d returns error. ret=%d",
                      cInfo->egressQueueInstance, ret);
         return ret;
      }

      if (cmdType == QOS_COMMAND_CONFIG && !egressQueueEnabled)
      {
         cmsLog_debug("class queue is not enabled. No action is taken.");
         return CMSRET_SUCCESS;
      }
   }
   else
   {
      if (cmdType == QOS_COMMAND_UNCONFIG)
      {
         cmsLog_debug("unconfig classQueue=%d. No action is taken.", cInfo->egressQueueInstance);
         return CMSRET_SUCCESS;
      }
      cmsLog_error("invalid classQueue=%d", cInfo->egressQueueInstance);
      return CMSRET_INVALID_ARGUMENTS;
   }


#ifdef BRCM_WLAN
   if (cmsUtl_strncmp(cInfo->egressIntfName, WLAN_IFC_STR, strlen(WLAN_IFC_STR)) == 0)
   {
      egressWlan = TRUE;
   }
#endif

#ifdef SUPPORT_POLICING
   if (cInfo->policerInstance > 0)
   {
      if ((ret = qdmQos_getClassPolicerInfoLocked(cInfo->policerInstance, &policerInfo)) != CMSRET_SUCCESS)
      {
         cmsLog_error("qdmQos_getClassPolicerInfo returns error. ret=%d", ret);
         return ret;
      }

      if (cmdType == QOS_COMMAND_CONFIG && !policerInfo.enable)
      {
         cmsLog_error("class policer is not enabled. No action is taken.");
         return CMSRET_INVALID_ARGUMENTS;
      }
   }
#endif

#ifndef SUPPORT_IPV6
   if (cInfo->etherType == ETH_P_IPV6)
   {
      cmsLog_error("IPv6 is not enabled.");
      return CMSRET_INVALID_ARGUMENTS;
   }
#endif

   if (cmsUtl_strcmp(cInfo->ingressIntfFullPath, MDMVS_LOCAL) == 0)
   {
      /* LOCAL traffic QoS */
      cmsLog_debug("Local: ingress=%s", cInfo->ingressIntfFullPath);
      if (cmdType == QOS_COMMAND_CONFIG)
      {
         if ((ret = rutQos_roLocalClass(cInfo, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_roLocalClass returns error. ret=%d", ret);
         }
         else if ((ret = rutQos_brLocalClass(cInfo, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_brLocalClass returns error. ret=%d", ret);
         }
         else if ((ret = rutQos_roEgressMark(cInfo, priority, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_roEgressMark returns error. ret=%d", ret);
         }
         else if ((ret = rutQos_brEgressMark(cInfo, priority, egressWlan, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_brEgressMark returns error. ret=%d", ret);
         }
      }
      else
      {
         /* delete classification rules. Do it in reverse order. */
         if ((ret = rutQos_brEgressMark(cInfo, priority, egressWlan, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_brEgressMark returns error. ret=%d", ret);
         }
         else if ((ret = rutQos_roEgressMark(cInfo, priority, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_roEgressMark returns error. ret=%d", ret);
         }
         else if ((ret = rutQos_brLocalClass(cInfo, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_brLocalClass returns error. ret=%d", ret);
         }
         else if ((ret = rutQos_roLocalClass(cInfo, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_roLocalClass returns error. ret=%d", ret);
         }
      }
   }
   else if (cmsUtl_strcmp(cInfo->ingressIntfFullPath, MDMVS_LAN) == 0 ||
            cInfo->ingressIsSpecificLan)
   {
      /* upstream QoS */
      cmsLog_debug("LAN: ingress=%s specificLan=%d",
                   cInfo->ingressIntfFullPath, cInfo->ingressIsSpecificLan);

      if (cmdType == QOS_COMMAND_CONFIG)
      {
         /* config classification rules */
         if ((ret = rutQos_brIngressClass(cInfo, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_brIngressClass returns error. ret=%d", ret);
         }
         else if ((ret = rutQos_brEgressMark(cInfo, priority, egressWlan, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_brEgressMark returns error. ret=%d", ret);
         }
         else if ((ret = rutQos_roEgressMark(cInfo, priority, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_roEgressMark returns error. ret=%d", ret);
         }
#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
         else if ((ret = rutQos_vlanTag(cInfo, egressQueueIntfNameBuf, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_vlanTag returns error. ret=%d", ret);
         }
#endif
      }
      else
      {
         /* delete classification rules. Do it in reverse order. */
#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
         if ((ret = rutQos_vlanTag(cInfo, egressQueueIntfNameBuf, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_vlanTag returns error. ret=%d", ret);
         }
         else
#endif
         if ((ret = rutQos_roEgressMark(cInfo, priority, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_roEgressMark returns error. ret=%d", ret);
         }
         else if ((ret = rutQos_brEgressMark(cInfo, priority, egressWlan, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_brEgressMark returns error. ret=%d", ret);
         }
         else if ((ret = rutQos_brIngressClass(cInfo, cmdType)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_brIngressClass returns error. ret=%d", ret);
         }
      }
   }	     
   else
   {
      /* downstream QoS */

      if (cmsUtl_strcmp(cInfo->ingressIntfFullPath, MDMVS_WAN) == 0)
      {
         /* classInterface is WAN */
         cmsLog_debug("generic WAN: ingress=%s specificWan=%d",
                       cInfo->ingressIntfFullPath, cInfo->ingressIsSpecificWan);

         if (cmdType == QOS_COMMAND_CONFIG)
         {
            /* config classification rules */
            if ((ret = rutQos_brIngressClass(cInfo, cmdType)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_brIngressClass returns error. ret=%d", ret);
            }
            else if ((ret = rutQos_roIngressClass(cInfo, cmdType)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_roIngressClass returns error. ret=%d", ret);
            }
            else if ((ret = rutQos_brEgressMark(cInfo, priority, egressWlan, cmdType)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_brEgressMark returns error. ret=%d", ret);
            }
         }
         else
         {
            /* delete classification rules. Do it in reverse order. */
            if ((ret = rutQos_brEgressMark(cInfo, priority, egressWlan, cmdType)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_brEgressMark returns error. ret=%d", ret);
            }
            else if ((ret = rutQos_roIngressClass(cInfo, cmdType)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_roIngressClass returns error. ret=%d", ret);
            }
            else if ((ret = rutQos_brIngressClass(cInfo, cmdType)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_brIngressClass returns error. ret=%d", ret);
            }
         }
      }
      else
      {
         char intfname[CMS_IFNAME_LENGTH]={0};

         /* classInterface is a specific WAN interface */
         cmsLog_debug("specific WAN: ingress=%s specificWan=%d",
                       cInfo->ingressIntfFullPath, cInfo->ingressIsSpecificWan);

         if ((ret = qdmIntf_fullPathToIntfnameLocked(cInfo->ingressIntfFullPath, intfname)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not convert %s to intfName, ret=%d",
                         cInfo->ingressIntfFullPath, ret);
         }
         else if (cmdType == QOS_COMMAND_CONFIG)
         {
            /* config classification rules */
            if (qdmIpIntf_isWanInterfaceBridgedLocked(intfname))
            {
               if ((ret = rutQos_brIngressClass(cInfo, cmdType)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("rutQos_brIngressClass returns error. ret=%d", ret);
               }
            }
            else
            {
               if ((ret = rutQos_roIngressClass(cInfo, cmdType)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("rutQos_roIngressClass returns error. ret=%d", ret);
               }
            }
            if (ret == CMSRET_SUCCESS)
            {
               if ((ret = rutQos_brEgressMark(cInfo, priority, egressWlan, cmdType)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("rutQos_brEgressMark returns error. ret=%d", ret);
               }
            }
         }
         else
         {
            /* delete classification rules. Do it in reverse order. */
            if ((ret = rutQos_brEgressMark(cInfo, priority, egressWlan, cmdType)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_brEgressMark returns error. ret=%d", ret);
            }
            if (ret == CMSRET_SUCCESS)
            {
               if (qdmIpIntf_isWanInterfaceBridgedLocked(intfname))
               {
                  if ((ret = rutQos_brIngressClass(cInfo, cmdType)) != CMSRET_SUCCESS)
                  {
                     cmsLog_error("rutQos_brIngressClass returns error. ret=%d", ret);
                  }
               }
               else
               {
                  if ((ret = rutQos_roIngressClass(cInfo, cmdType)) != CMSRET_SUCCESS)
                  {
                     cmsLog_error("rutQos_roIngressClass returns error. ret=%d", ret);
                  }
               }
            }
         }
      }
   }

#ifdef SUPPORT_POLICING
   if (cInfo->policerInstance > 0)
   {
      if (ret == CMSRET_SUCCESS)
      {
         if ((ret = rutQos_policer(cmdType,
                                   egressQueueIntfNameBuf,
                                   cInfo,
                                   &policerInfo)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_policer returns error. ret=%d", ret);
         }
      }
   }
#endif

#ifdef SUPPORT_RATE_LIMIT
   if (ret == CMSRET_SUCCESS)
   {
      if ((ret = rutQos_rateLimit(cmdType,
                                  egressQueueIntfNameBuf,
                                  cInfo->key,
                                  cInfo->classRate)) != CMSRET_SUCCESS)
      {
         cmsLog_error("rutQos_rateLimit returns error. ret=%d", ret);
      }
   }
#endif

#ifndef BCM_PON /* Not for PON product*/
   /* Update switch flow control and Real HW Switching settings */
   /* It is inefficient to put the check here.  I think the rutEsw functions
    * only look at the static configurations of the classifiers, so we
    * only need to check once at bootup, and then whenever the classifiers
    * change.  We don't need to re-evaluate when link comes up or down (this function).
    */
   {
      UINT32 excludeClsKey=QOS_CLS_INVALID_INDEX;
      if (cmdType == QOS_COMMAND_UNCONFIG)
      {
         /* if we are doing an UNCONFIG, then exclude the current clsKey */
         excludeClsKey = cInfo->key;
      }

      rutEsw_updatePortPauseFlowCtrlSetting(excludeClsKey);
      rutEsw_updateRealHwSwitchingSetting(excludeClsKey);
   }
#endif

#ifdef SUPPORT_FCCTL
   rut_doSystemAction("rutQos_qMgmtClassConfig", "fcctl flush --silent");
#endif

   return ret;

}  /* End of rutQos_qMgmtClassConfig() */

/***************************************************************************
// Function Name: rutQos_brLocalClass
// Description  : set up OUTPUT nat ebtables rules for local traffic classification.
// Parameters   : clsObj - QoS class information.
//                cmdType - command type either config or unconfig.
// Returns      : CmsRet.
****************************************************************************/
CmsRet rutQos_brLocalClass(const CmsQosClassInfo *cInfo,
                           QosCommandType cmdType)
{
   char cmd[BUFLEN_1024];
   char ethertype[BUFLEN_16];
   char proto[BUFLEN_24];   
   char sport[BUFLEN_24], dport[BUFLEN_24];
   char dscpCheck[BUFLEN_24];
   char egressIfc[BUFLEN_40];
   UINT32 clsKey;
   UINT32 flowId = 0;
   char *exclude = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmd[0] = '\0';
   ethertype[0] = '\0';
   proto[0] = '\0';
   sport[0] = '\0'; dport[0] = '\0';
   dscpCheck[0] = '\0';
   egressIfc[0] = '\0';

   clsKey = cInfo->key;

   if (!IS_EMPTY_STRING(cInfo->egressIntfName))
   {
      sprintf(egressIfc, "-o %s", cInfo->egressIntfName);
   }

   exclude = cInfo->etherTypeExclude? "!" : "";
   switch (cInfo->etherType)
   {
      case ETH_P_IP:
         sprintf(ethertype, "-p %s 0x%x", exclude, ETH_P_IP);

         /* DSCP check */
         if (cInfo->DSCPCheck != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->DSCPExclude? "!" : "";
            sprintf(dscpCheck, "--ip-dscp %s 0x%x", exclude, cInfo->DSCPCheck << 2);
         }

         /* protocol */
         exclude = cInfo->protocolExclude? "!" : "";
         switch (cInfo->protocol)
         {
            case IPPROTO_TCP:
            case IPPROTO_UDP:
            case IPPROTO_ICMP:
            case IPPROTO_IGMP:
               sprintf(proto, "--ip-proto %s %d", exclude, cInfo->protocol);
               break;
         }

         /* source port */
         if (cInfo->sourcePort != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->sourcePortExclude? "!" : "";
            if (cInfo->sourcePortRangeMax != QOS_CRITERION_UNUSED)
            {
               sprintf(sport, "--ip-sport %s %d:%d", exclude, cInfo->sourcePort, cInfo->sourcePortRangeMax);
            }
            else
            {
               sprintf(sport, "--ip-sport %s %d", exclude, cInfo->sourcePort);
            }
         }

         /* destination port */
         if (cInfo->destPort != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->destPortExclude? "!" : "";
            if (cInfo->destPortRangeMax != QOS_CRITERION_UNUSED)
            {
               sprintf(dport, "--ip-dport %s %d:%d", exclude, cInfo->destPort, cInfo->destPortRangeMax);
            }
            else
            {
               sprintf(dport, "--ip-dport %s %d", exclude, cInfo->destPort);
            }
         }
         break;

      case ETH_P_IPV6:
         sprintf(ethertype, "-p %s 0x%x", exclude, ETH_P_IPV6);

         /* DSCP check */
         if (cInfo->DSCPCheck != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->DSCPExclude? "!" : "";
            sprintf(dscpCheck, "--ip6-tclass %s 0x%x", exclude, cInfo->DSCPCheck << 2);
         }

         /* protocol */
         exclude = cInfo->protocolExclude? "!" : "";
         switch (cInfo->protocol)
         {
            case IPPROTO_TCP:
            case IPPROTO_UDP:
            case IPPROTO_ICMPV6:
               sprintf(proto, "--ip6-proto %s %d", exclude, cInfo->protocol);
               break;
         }

         /* source port */
         if (cInfo->sourcePort != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->sourcePortExclude? "!" : "";
            if (cInfo->sourcePortRangeMax != QOS_CRITERION_UNUSED)
            {
               sprintf(sport, "--ip6-sport %s %d:%d", exclude, cInfo->sourcePort, cInfo->sourcePortRangeMax);
            }
            else
            {
               sprintf(sport, "--ip6-sport %s %d", exclude, cInfo->sourcePort);
            }
         }

         /* destination port */
         if (cInfo->destPort != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->destPortExclude? "!" : "";
            if (cInfo->destPortRangeMax != QOS_CRITERION_UNUSED)
            {
               sprintf(dport, "--ip6-dport %s %d:%d", exclude, cInfo->destPort, cInfo->destPortRangeMax);
            }
            else
            {
               sprintf(dport, "--ip6-dport %s %d", exclude, cInfo->destPort);
            }
         }
         break;
   }

   /* flowId mark */
   flowId = SKBMARK_SET_FLOW_ID(flowId, clsKey);

   /* set up ebtable rules
    */
   if (cmdType == QOS_COMMAND_CONFIG)
   {
      /* Set matching rule in ebtables OUTPUT chain to mark flowId */
      sprintf(cmd, "ebtables -t nat -A OUTPUT %s %s %s %s %s %s -j mark --mark-or 0x%x",
              egressIfc, ethertype, proto, sport, dport, dscpCheck, flowId);
   }
   else
   {
      sprintf(cmd, "ebtables -t nat -D OUTPUT %s %s %s %s %s %s -j mark --mark-or 0x%x 2>/dev/null",
              egressIfc, ethertype, proto, sport, dport, dscpCheck, flowId);
   }
   rut_doSystemAction("rutQos_brLocalClass", cmd);

   return ret;

}  /* End of rutQos_brLocalClass() */

/***************************************************************************
// Function Name: rutQos_roLocalClass
// Description  : set up OUTPUT mangle iptables rules for local traffic classification.
// Parameters   : clsObj - QoS class information.
//                cmdType - command type either config or unconfig.
// Returns      : CmsRet.
****************************************************************************/
CmsRet rutQos_roLocalClass(const CmsQosClassInfo *cInfo,
                           QosCommandType cmdType)
{
   char cmd[BUFLEN_1024];
   char tblStr[BUFLEN_16];
   char proto[BUFLEN_24];   
   char sport[BUFLEN_24], dport[BUFLEN_24];
   char dscpCheck[BUFLEN_24];
   char egressIfc[BUFLEN_40];
   UINT32 clsKey;
   UINT32 flowId = 0;
   char *exclude = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmd[0] = '\0';
   proto[0] = '\0';
   sport[0] = '\0'; dport[0] = '\0';
   dscpCheck[0] = '\0';
   egressIfc[0] = '\0';

   if (cInfo->etherType == ETH_P_IPV6)
   {
      strcpy(tblStr, "ip6tables");
   }
   else
   {
      strcpy(tblStr, "iptables");
   }

   clsKey = cInfo->key;

   if (!IS_EMPTY_STRING(cInfo->egressIntfName))
   {
      sprintf(egressIfc, "-o %s", cInfo->egressIntfName);
   }
   
   /* DSCP check */
   if (cInfo->DSCPCheck != QOS_CRITERION_UNUSED)
   {
      exclude = cInfo->DSCPExclude? "!" : "";
      sprintf(dscpCheck, "-m dscp --dscp %s 0x%x", exclude, cInfo->DSCPCheck);
   }

   /* protocol */
   exclude = cInfo->protocolExclude? "!" : "";
   switch (cInfo->protocol)
   {
      case IPPROTO_TCP:
      case IPPROTO_UDP:
      case IPPROTO_ICMP:
      case IPPROTO_ICMPV6:
      case IPPROTO_IGMP:
         sprintf(proto, "-p %s %d", exclude, cInfo->protocol);
         break;
   }

   /* source port */
   if (cInfo->sourcePort != QOS_CRITERION_UNUSED)
   {
      exclude = cInfo->sourcePortExclude? "!" : "";
      if (cInfo->sourcePortRangeMax != QOS_CRITERION_UNUSED)
      {
         sprintf(sport, "--sport %s %d:%d", exclude, cInfo->sourcePort, cInfo->sourcePortRangeMax);
      }
      else
      {
         sprintf(sport, "--sport %s %d", exclude, cInfo->sourcePort);
      }
   }

   /* destination port */
   if (cInfo->destPort != QOS_CRITERION_UNUSED)
   {
      exclude = cInfo->destPortExclude? "!" : "";
      if (cInfo->destPortRangeMax != QOS_CRITERION_UNUSED)
      {
         sprintf(dport, "--dport %s %d:%d", exclude, cInfo->destPort, cInfo->destPortRangeMax);
      }
      else
      {
         sprintf(dport, "--dport %s %d", exclude, cInfo->destPort);
      }
   }

   /* flowId mark */
   flowId = SKBMARK_SET_FLOW_ID(flowId, clsKey);

   /* set up iptable rules
    */
   if (cmdType == QOS_COMMAND_CONFIG)
   {
      /* Set matching rule in iptables OUTPUT chain to mark flowId */
      sprintf(cmd, "%s -w -t mangle -A OUTPUT %s %s %s %s %s -j MARK --or-mark 0x%x",
              tblStr, egressIfc, proto, sport, dport, dscpCheck, flowId);
   }
   else
   {
      sprintf(cmd, "%s -w -t mangle -D OUTPUT %s %s %s %s %s -j MARK --or-mark 0x%x 2>/dev/null",
              tblStr, egressIfc, proto, sport, dport, dscpCheck, flowId);
   }
#ifdef SUPPORT_NF_MANGLE
   rut_doSystemAction("rutQos_roLocalClass", cmd);
#endif // SUPPORT_NF_MANGLE

   return ret;

}  /* End of rutQos_roLocalClass() */

/***************************************************************************
// Function Name: rutQos_roIngressClass
// Description  : set up PREROUTING mangle iptables rules for traffic classification.
// Parameters   : clsObj - QoS class information.
//                cmdType - command type either config or unconfig.
// Returns      : CmsRet.
****************************************************************************/
CmsRet rutQos_roIngressClass(const CmsQosClassInfo *cInfo,
                             QosCommandType cmdType)
{
   char cmd[BUFLEN_1024];
   char tblStr[BUFLEN_16];
   char ifcStr[BUFLEN_40];
   char proto[BUFLEN_24];   
   char smac[BUFLEN_48];
   char src[BUFLEN_80], dst[BUFLEN_64];
   char sport[BUFLEN_24], dport[BUFLEN_24];
   char dscpCheck[BUFLEN_24];
   char ifName[CMS_IFNAME_LENGTH]={0};
   UINT32 clsKey;
   UINT32 flowId = 0;
   char *exclude = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmd[0] = '\0';
   ifcStr[0] = '\0';
   proto[0] = '\0';
   smac[0] = '\0';
   src[0] = '\0'; dst[0] = '\0'; sport[0] = '\0'; dport[0] = '\0';
   dscpCheck[0] = '\0';

   if (cInfo->etherType == ETH_P_IPV6)
   {
      strcpy(tblStr, "ip6tables");
   }
   else
   {
      strcpy(tblStr, "iptables");
   }

   clsKey = cInfo->key;

   if (cmsUtl_strcmp(cInfo->ingressIntfFullPath, MDMVS_WAN) == 0)
   {
      sprintf(ifcStr, "-m mark --mark 0x%x/0x%x", SKBMARK_IFFWAN_MARK_M, SKBMARK_IFFWAN_MARK_M);
   }
   else if (!IS_EMPTY_STRING(cInfo->ingressIntfFullPath))
   {
      if ((ret = qdmIntf_fullPathToIntfnameLocked(cInfo->ingressIntfFullPath, ifName)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not convert %s to ifName, ret=%d", cInfo->ingressIntfFullPath, ret);
         return ret;
      }
      sprintf(ifcStr, "-i %s", ifName);
   }

   /* Source MAC address */
   if (!IS_EMPTY_STRING(cInfo->sourceMACAddress))
   {
      exclude = cInfo->sourceMACExclude? "!" : "";
      sprintf(smac, "-m mac --mac-source %s %s", exclude, cInfo->sourceMACAddress);
   }

   /* source address/mask */
   if (!IS_EMPTY_STRING(cInfo->sourceIP))
   {
      exclude = cInfo->sourceIPExclude? "!" : "";
      if (!IS_EMPTY_STRING(cInfo->sourceMask))
      {
         sprintf(src, "-s %s %s/%d", exclude, cInfo->sourceIP,
                 cmsNet_getLeftMostOneBitsInMask(cInfo->sourceMask));
      }
      else
      {
         sprintf(src, "-s %s %s", exclude, cInfo->sourceIP);
      }
   }

   /* destination address/mask */
   if (!IS_EMPTY_STRING(cInfo->destIP))
   {
      exclude = cInfo->destIPExclude? "!" : "";
      if (!IS_EMPTY_STRING(cInfo->destMask))
      {
         sprintf(dst, "-d %s %s/%d", exclude, cInfo->destIP,
                 cmsNet_getLeftMostOneBitsInMask(cInfo->destMask));
      }
      else
      {
         sprintf(dst, "-d %s %s", exclude, cInfo->destIP);
      }
   }

   /* DSCP check */
   if (cInfo->DSCPCheck != QOS_CRITERION_UNUSED)
   {
      exclude = cInfo->DSCPExclude? "!" : "";
      sprintf(dscpCheck, "-m dscp --dscp %s 0x%x", exclude, cInfo->DSCPCheck);
   }

   /* protocol */
   exclude = cInfo->protocolExclude? "!" : "";
   switch (cInfo->protocol)
   {
      case IPPROTO_TCP:
      case IPPROTO_UDP:
      case IPPROTO_ICMP:
      case IPPROTO_ICMPV6:
      case IPPROTO_IGMP:
         sprintf(proto, "-p %s %d", exclude, cInfo->protocol);
         break;
   }

   /* source port */
   if (cInfo->sourcePort != QOS_CRITERION_UNUSED)
   {
      exclude = cInfo->sourcePortExclude? "!" : "";
      if (cInfo->sourcePortRangeMax != QOS_CRITERION_UNUSED)
      {
         sprintf(sport, "--sport %s %d:%d", exclude, cInfo->sourcePort, cInfo->sourcePortRangeMax);
      }
      else
      {
         sprintf(sport, "--sport %s %d", exclude, cInfo->sourcePort);
      }
   }

   /* destination port */
   if (cInfo->destPort != QOS_CRITERION_UNUSED)
   {
      exclude = cInfo->destPortExclude? "!" : "";
      if (cInfo->destPortRangeMax != QOS_CRITERION_UNUSED)
      {
         sprintf(dport, "--dport %s %d:%d", exclude, cInfo->destPort, cInfo->destPortRangeMax);
      }
      else
      {
         sprintf(dport, "--dport %s %d", exclude, cInfo->destPort);
      }
   }

   /*
    * Now construct strings for marking.
    */
   /* flowId mark */
   flowId = SKBMARK_SET_FLOW_ID(flowId, clsKey);

   /* set up iptable rules
    */
   if (cmdType == QOS_COMMAND_CONFIG)
   {
      /* Set matching rule in iptables PREROUTING chain to mark flowId */
      sprintf(cmd, "%s -w -t mangle -A PREROUTING %s %s %s %s %s %s %s %s -j MARK --or-mark 0x%x",
              tblStr, proto, ifcStr, smac, src, dst, sport, dport, dscpCheck, flowId);
   }
   else
   {
      sprintf(cmd, "%s -w -t mangle -D PREROUTING %s %s %s %s %s %s %s %s -j MARK --or-mark 0x%x 2>/dev/null",
              tblStr, proto, ifcStr, smac, src, dst, sport, dport, dscpCheck, flowId);
   }
#ifdef SUPPORT_NF_MANGLE
   rut_doSystemAction("rutQos_roIngressClass", cmd);
#endif // SUPPORT_NF_MANGLE

   return ret;

}  /* End of rutQos_roIngressClass() */

/***************************************************************************
// Function Name: rutQos_brIngressClass
// Description  : set up BROUTING broute ebtables rules for traffic classification.
// Parameters   : clsObj - QoS class information.
//                cmdType - command type either config or unconfig.
// Returns      : CmsRet.
****************************************************************************/
CmsRet rutQos_brIngressClass(const CmsQosClassInfo *cInfo,
                             QosCommandType cmdType)
{
   char cmd[BUFLEN_1024];
   char ethertype[BUFLEN_16];
   char proto[BUFLEN_24];   
   char src[BUFLEN_80], dst[BUFLEN_64];
   char smac[BUFLEN_48], dmac[BUFLEN_48];
   char sport[BUFLEN_24], dport[BUFLEN_24];
   char ifcStr[BUFLEN_40], vlan8021p[BUFLEN_40];
   char dscpCheck[BUFLEN_24];
   char ifName[CMS_IFNAME_LENGTH]={0};
   UINT32 clsKey;
   UINT32 flowId = 0;
   char *exclude = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmd[0] = '\0';
   ethertype[0] = '\0'; proto[0] = '\0';
   src[0] = '\0'; dst[0] = '\0'; sport[0] = '\0'; dport[0] = '\0';
   smac[0] = '\0'; dmac[0] = '\0';
   ifcStr[0] = '\0'; vlan8021p[0] = '\0';
   dscpCheck[0] = '\0';

   clsKey = cInfo->key;

   /*
    * Construct the criterion strings.
    */
   if (cmsUtl_strcmp(cInfo->ingressIntfFullPath, MDMVS_LAN) == 0)
   {
      sprintf(ifcStr, "--mark ! 0x%x/0x%x", SKBMARK_IFFWAN_MARK_M, SKBMARK_IFFWAN_MARK_M);
   }
   else if (cmsUtl_strcmp(cInfo->ingressIntfFullPath, MDMVS_WAN) == 0)
   {
      sprintf(ifcStr, "--mark 0x%x/0x%x", SKBMARK_IFFWAN_MARK_M, SKBMARK_IFFWAN_MARK_M);
   }
   else if (!IS_EMPTY_STRING(cInfo->ingressIntfFullPath))
   {
      if ((ret = qdmIntf_fullPathToIntfnameLocked(cInfo->ingressIntfFullPath, ifName)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not convert %s to ifName, ret=%d", cInfo->ingressIntfFullPath, ret);
         return ret;
      }
      if (strstr(ifName, ETH_IFC_STR) != NULL && strstr(ifName, ".") == NULL)
      {
         sprintf(ifcStr, "-i %s+", ifName);
      }
      else
      {
         sprintf(ifcStr, "-i %s", ifName);
      }
   }

   /* Source MAC address and mask */
   if (!IS_EMPTY_STRING(cInfo->sourceMACAddress))
   {
      exclude = cInfo->sourceMACExclude? "!" : "";
      if (!IS_EMPTY_STRING(cInfo->sourceMACMask))
      {
         sprintf(smac, "--src %s %s/%s", exclude, cInfo->sourceMACAddress, cInfo->sourceMACMask);
      }
      else
      {
         sprintf(smac, "--src %s %s", exclude, cInfo->sourceMACAddress);
      }
   }

   /* Destination MAC address and mask */
   if (!IS_EMPTY_STRING(cInfo->destMACAddress))
   {
      exclude = cInfo->destMACExclude? "!" : "";
      if (!IS_EMPTY_STRING(cInfo->destMACMask))
      {
         sprintf(dmac, "--dst %s %s/%s", exclude, cInfo->destMACAddress, cInfo->destMACMask);
      }
      else
      {
         sprintf(dmac, "--dst %s %s", exclude, cInfo->destMACAddress);
      }
   }

   exclude = cInfo->etherTypeExclude? "!" : "";
   switch (cInfo->etherType)
   {
      case ETH_P_IP:
         sprintf(ethertype, "-p %s 0x%x", exclude, ETH_P_IP);

         /* source address/mask */
         if (!IS_EMPTY_STRING(cInfo->sourceIP))
         {
            exclude = cInfo->sourceIPExclude? "!" : "";
            if (!IS_EMPTY_STRING(cInfo->sourceMask))
            {
               sprintf(src, "--ip-src %s %s/%d", exclude, cInfo->sourceIP,
                       cmsNet_getLeftMostOneBitsInMask(cInfo->sourceMask));
            }
            else
            {
               sprintf(src, "--ip-src %s %s", exclude, cInfo->sourceIP);
            }
         }
         else if (!IS_EMPTY_STRING(cInfo->sourceVendorClassID))
         {
            sprintf(src, "--ip-src [%s]", cInfo->sourceVendorClassID);
         }
         else if (!IS_EMPTY_STRING(cInfo->sourceUserClassID))
         {
            sprintf(src, "--ip-src [%s]", cInfo->sourceUserClassID);
         }

         /* destination address/mask */
         if (!IS_EMPTY_STRING(cInfo->destIP))
         {
            exclude = cInfo->destIPExclude? "!" : "";
            if (!IS_EMPTY_STRING(cInfo->destMask))
            {
               sprintf(dst, "--ip-dst %s %s/%d", exclude, cInfo->destIP,
                       cmsNet_getLeftMostOneBitsInMask(cInfo->destMask));
            }
            else
            {
               sprintf(dst, "--ip-dst %s %s", exclude, cInfo->destIP);
            }
         }

         /* DSCP check */
         if (cInfo->DSCPCheck != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->DSCPExclude? "!" : "";
            sprintf(dscpCheck, "--ip-dscp %s 0x%x", exclude, cInfo->DSCPCheck << 2);
         }

         /* protocol */
         exclude = cInfo->protocolExclude? "!" : "";
         switch (cInfo->protocol)
         {
            case IPPROTO_TCP:
            case IPPROTO_UDP:
            case IPPROTO_ICMP:
            case IPPROTO_IGMP:
               sprintf(proto, "--ip-proto %s %d", exclude, cInfo->protocol);
               break;
         }

         /* source port */
         if (cInfo->sourcePort != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->sourcePortExclude? "!" : "";
            if (cInfo->sourcePortRangeMax != QOS_CRITERION_UNUSED)
            {
               sprintf(sport, "--ip-sport %s %d:%d", exclude, cInfo->sourcePort, cInfo->sourcePortRangeMax);
            }
            else
            {
               sprintf(sport, "--ip-sport %s %d", exclude, cInfo->sourcePort);
            }
         }

         /* destination port */
         if (cInfo->destPort != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->destPortExclude? "!" : "";
            if (cInfo->destPortRangeMax != QOS_CRITERION_UNUSED)
            {
               sprintf(dport, "--ip-dport %s %d:%d", exclude, cInfo->destPort, cInfo->destPortRangeMax);
            }
            else
            {
               sprintf(dport, "--ip-dport %s %d", exclude, cInfo->destPort);
            }
         }
         break;

      case ETH_P_IPV6:
         sprintf(ethertype, "-p %s 0x%x", exclude, ETH_P_IPV6);

         /* source address/mask */
         if (!IS_EMPTY_STRING(cInfo->sourceIP))
         {
            exclude = cInfo->sourceIPExclude? "!" : "";
            if (!IS_EMPTY_STRING(cInfo->sourceMask))
            {
               sprintf(src, "--ip6-src %s %s/%d", exclude, cInfo->sourceIP,
                       cmsNet_getLeftMostOneBitsInMask(cInfo->sourceMask));
            }
            else
            {
               sprintf(src, "--ip6-src %s %s", exclude, cInfo->sourceIP);
            }
         }

         /* destination address/mask */
         if (!IS_EMPTY_STRING(cInfo->destIP))
         {
            exclude = cInfo->destIPExclude? "!" : "";
            if (!IS_EMPTY_STRING(cInfo->destMask))
            {
               sprintf(dst, "--ip6-dst %s %s/%d", exclude, cInfo->destIP,
                       cmsNet_getLeftMostOneBitsInMask(cInfo->destMask));
            }
            else
            {
               sprintf(dst, "--ip6-dst %s %s", exclude, cInfo->destIP);
            }
         }

         /* DSCP check */
         if (cInfo->DSCPCheck != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->DSCPExclude? "!" : "";
            sprintf(dscpCheck, "--ip6-tclass %s 0x%x", exclude, cInfo->DSCPCheck << 2);
         }

         /* protocol */
         exclude = cInfo->protocolExclude? "!" : "";
         switch (cInfo->protocol)
         {
            case IPPROTO_TCP:
            case IPPROTO_UDP:
            case IPPROTO_ICMPV6:
               sprintf(proto, "--ip6-proto %s %d", exclude, cInfo->protocol);
               break;
         }

         /* source port */
         if (cInfo->sourcePort != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->sourcePortExclude? "!" : "";
            if (cInfo->sourcePortRangeMax != QOS_CRITERION_UNUSED)
            {
               sprintf(sport, "--ip6-sport %s %d:%d", exclude, cInfo->sourcePort, cInfo->sourcePortRangeMax);
            }
            else
            {
               sprintf(sport, "--ip6-sport %s %d", exclude, cInfo->sourcePort);
            }
         }

         /* destination port */
         if (cInfo->destPort != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->destPortExclude? "!" : "";
            if (cInfo->destPortRangeMax != QOS_CRITERION_UNUSED)
            {
               sprintf(dport, "--ip6-dport %s %d:%d", exclude, cInfo->destPort, cInfo->destPortRangeMax);
            }
            else
            {
               sprintf(dport, "--ip6-dport %s %d", exclude, cInfo->destPort);
            }
         }
         break;

      case ETH_P_8021Q:
#ifdef SUPPORT_LANVLAN 
         /* ethernet skbvlan check */
         if (cInfo->ethernetPriorityCheck != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->ethernetPriorityExclude? "!" : "";
            sprintf(vlan8021p, "--skbvlan-prio %s %d", exclude, cInfo->ethernetPriorityCheck);
         } 
#else
         sprintf(ethertype, "-p %s 0x%x", exclude, ETH_P_8021Q);

         /* ethernet priority (8012p) check */
         if (cInfo->ethernetPriorityCheck != QOS_CRITERION_UNUSED)
         {
            exclude = cInfo->ethernetPriorityExclude? "!" : "";
            sprintf(vlan8021p, "--vlan-prio %s %d", exclude, cInfo->ethernetPriorityCheck);
         } 
#endif
         break;

      case ETH_P_ARP:
      case ETH_P_PPP_DISC:
      case ETH_P_PPP_SES:
      case 0x8865:
      case 0x8866:
         sprintf(ethertype, "-p %s 0x%x", exclude, cInfo->etherType);
         break;
   }

   /* flowId mark */
   flowId = SKBMARK_SET_FLOW_ID(flowId, clsKey);

   /* Set up ebtables rules.
    */
   if (cmdType == QOS_COMMAND_CONFIG)
   {
      /* Set the matching rule in ebtables BROUTING chain */
      sprintf(cmd, "ebtables -t broute -A BROUTING %s %s %s %s %s %s %s %s %s %s %s -j mark --mark-or 0x%x",
              ethertype, proto, src, sport, dst, dport, smac, dmac, vlan8021p, ifcStr, dscpCheck, flowId);
   }
   else
   {
      sprintf(cmd, "ebtables -t broute -D BROUTING %s %s %s %s %s %s %s %s %s %s %s -j mark --mark-or 0x%x 2>/dev/null",
              ethertype, proto, src, sport, dst, dport, smac, dmac, vlan8021p, ifcStr, dscpCheck, flowId);
   }
   if (!IS_EMPTY_STRING(cInfo->sourceVendorClassID))
   {
      rutQos_sendDhcp(CMS_MSG_QOS_DHCP_OPT60_COMMAND, cmd);
   }
   else if (!IS_EMPTY_STRING(cInfo->sourceUserClassID))
   {
      rutQos_sendDhcp(CMS_MSG_QOS_DHCP_OPT77_COMMAND, cmd);
   }
   else
   {
      rut_doSystemAction("rutQos_brIngressClass", cmd);
   }
   
   return ret;

}  /* End of rutQos_brIngressClass() */

/***************************************************************************
// Function Name: rutQos_brEgressMark
// Description  : set up POSTROUTING nat ebtables rules for traffic class marking.
// Parameters   : clsObj - QoS class information.
//                priority - egress queue priority information.
//                egressWlan - egress Wlan interface flag.
//                cmdType - command type either config or unconfig.
// Returns      : CmsRet.
****************************************************************************/
CmsRet rutQos_brEgressMark(const CmsQosClassInfo *cInfo,
                           UINT32 priority,
                           UBOOL8 egressWlan __attribute__((unused)),
                           QosCommandType cmdType)
{
   char cmd[BUFLEN_1024];
   char egressIfc[BUFLEN_40];
   char set_ftos[BUFLEN_32];
   char set_vtag[BUFLEN_64];
   UINT32 clsKey;
   UINT32 flowId = 0;
   UINT32 queueMark = 0;
   SINT32 vlanIdMark = 0;
   SINT32 vlan8021pMark = 0;
   CmsRet ret = CMSRET_SUCCESS;

   cmd[0]       = '\0';
   egressIfc[0] = '\0';
   set_ftos[0]  = '\0';
   set_vtag[0]  = '\0';

   clsKey = cInfo->key;

//   if (!IS_EMPTY_STRING(clsObj->X_BROADCOM_COM_egressInterface))
//   {
//      sprintf(egressIfc, "-o %s", clsObj->X_BROADCOM_COM_egressInterface);
//   }

   /* flowId mark */
   flowId = SKBMARK_SET_FLOW_ID(flowId, clsKey);

   /* priority queue mark */
   queueMark = SKBMARK_SET_Q(queueMark, priority);
   queueMark = SKBMARK_SET_FLOW_ID(queueMark, clsKey); 

   /* dscp mark */
   if (cInfo->DSCPMark != QOS_RESULT_NO_CHANGE)
   {
      if (cInfo->DSCPMark == QOS_RESULT_AUTO_MARK)
      {
         /* set_ftos is used by ebtables command */
         sprintf(set_ftos, "-j ftos --8021q-ftos");
      }
      else
      {
         /* set_ftos is used by ebtables command */
         sprintf(set_ftos, "-j ftos --set-ftos 0x%x", cInfo->DSCPMark << 2);
      }
   }

   /* see if egress is vlanmux interface */
   if (!IS_EMPTY_STRING(cInfo->egressIntfName))
   {
      char *p = strchr(cInfo->egressIntfName, '.');

      /* if egress is not vlanmux interface and not ipoa and not pppoa,
       * and either vid or pbits is specified, set up ebtables rule to
       * do vlan remarking.
       * Since all EoA type wan interfaces are now in vlanmux mode, this
       * code is only applicable to LAN or WLAN egress.
       */
      if (p == NULL &&
          strstr(cInfo->egressIntfName, IPOA_IFC_STR)  == NULL &&
          strstr(cInfo->egressIntfName, PPPOA_IFC_STR) == NULL &&
          (cInfo->vlanIdTag != QOS_RESULT_NO_CHANGE ||
           cInfo->ethernetPriorityMark     != QOS_RESULT_NO_CHANGE))
      {
         /* VLAN ID and Ethernet priority mark */
         /* Valid vlan id ranges from 0 to 4094. 
          * When setting vlanIdMark, we shift the id range to (1, 4095)
          * and use the value 0 to indicate no change.
          */
         if (cInfo->vlanIdTag != QOS_RESULT_NO_CHANGE)
         {
            vlanIdMark = (cInfo->vlanIdTag & 0xfff) + 1;
         }
         else
         {
            vlanIdMark = 0;   /* no change */
         }
         /* Valid 8021p priority value ranges from 0 to 7.
          * When setting vlan8021pMark, we shift the priority value range to (1, 8)
          * and use the value 0 to indicate no change.
          */
         if (cInfo->ethernetPriorityMark != QOS_RESULT_NO_CHANGE)
         {                                                                                 
            vlan8021pMark = (cInfo->ethernetPriorityMark & 0x7) + 1;
         }
         else
         {
            vlan8021pMark = 0;   /* no change */
         }
         sprintf(set_vtag, "-j mark --vtag-set 0x%x%x", vlanIdMark, vlan8021pMark);
   
         /* change the mark target to be non-terminating */
         strcat(set_vtag, " --mark-target CONTINUE");
      }
   }

   /* Set up ebtables and iptables rules.
    */
   if (cmdType == QOS_COMMAND_CONFIG)
   {
      /* Set the marking rules in ebtables POSTROUTING chain for bridged egress interface
       */
      sprintf(cmd, "ebtables -t nat -N rule%d", clsKey);
      rut_doSystemAction("rutQos_brEgressMark", cmd);

#ifdef BRCM_WLAN
      if (egressWlan)
      {
         /* egress interface is wlan */
         char priorityStr[BUFLEN_128]; 
         char ethertype[BUFLEN_16];
         char *exclude = NULL;

         ethertype[0] = '\0';

         exclude = cInfo->etherTypeExclude? "!" : "";
         switch (cInfo->etherType)
         {
            case ETH_P_IP:
            case ETH_P_ARP:
            case ETH_P_IPV6:
            case ETH_P_8021Q:
            case ETH_P_PPP_DISC:
            case ETH_P_PPP_SES:
            case 0x8865:
            case 0x8866:
               sprintf(ethertype, "-p %s 0x%x", exclude, cInfo->etherType);
               break;
         }

         sprintf(priorityStr, "%s -j wmm-mark --wmm-markset 0x%x0%02x", ethertype, vlan8021pMark, priority);
         if (cInfo->etherType == ETH_P_8021Q)
         {
            strcat(priorityStr, " --wmm-marktag vlan");
         }
         sprintf(cmd, "ebtables -t nat -A rule%d %s --wmm-mark-target CONTINUE", clsKey, priorityStr);
      }
      else
#endif
      {
         /* egress interface is not wlan */
         sprintf(cmd, "ebtables -t nat -A rule%d -j mark --mark-or 0x%x --mark-target CONTINUE",
                 clsKey, queueMark);
      }
      /* mark priority queue */
      rut_doSystemAction("rutQos_brEgressMark", cmd);

      if (!IS_EMPTY_STRING(set_ftos))
      {
         /* mark dscp */
         sprintf(cmd, "ebtables -t nat -A rule%d %s", clsKey, set_ftos);
         rut_doSystemAction("rutQos_brEgressMark", cmd);
      }
      
      if (!IS_EMPTY_STRING(set_vtag))
      {
         /* mark 8021p */
         sprintf(cmd, "ebtables -t nat -A rule%d %s", clsKey, set_vtag);
         rut_doSystemAction("rutQos_brEgressMark", cmd);
      }

      sprintf(cmd, "ebtables -t nat -A POSTROUTING %s --mark 0x%x/0x%x -j rule%d",
              egressIfc, flowId, SKBMARK_FLOW_ID_M, clsKey);
      rut_doSystemAction("rutQos_brEgressMark", cmd);
   }
   else
   {
      sprintf(cmd, "ebtables -t nat -D POSTROUTING %s --mark 0x%x/0x%x -j rule%d 2>/dev/null",
              egressIfc, flowId, SKBMARK_FLOW_ID_M, clsKey);
      rut_doSystemAction("rutQos_brEgressMark", cmd);
      sprintf(cmd, "ebtables -t nat -F rule%d 2>/dev/null", clsKey);
      rut_doSystemAction("rutQos_brEgressMark", cmd);
      sprintf(cmd, "ebtables -t nat -X rule%d 2>/dev/null", clsKey);
      rut_doSystemAction("rutQos_brEgressMark", cmd);
   }
   
   return ret;

}  /* End of rutQos_brEgressMark() */

/***************************************************************************
// Function Name: rutQos_roEgressMark
// Description  : set up POSTROUTING mangle iptables rules for traffic class marking.
// Parameters   : clsObj - QoS class information.
//                priority - egress queue priority information.
//                cmdType - command type either config or unconfig.
// Returns      : CmsRet
****************************************************************************/
CmsRet rutQos_roEgressMark(const CmsQosClassInfo *cInfo,
                           UINT32 priority,
                           QosCommandType cmdType)
{
   CmsRet ret = CMSRET_SUCCESS;
#ifdef SUPPORT_NF_MANGLE
   char cmd[BUFLEN_1024];
   char tblStr[BUFLEN_16];
   char egressIfc[BUFLEN_40];
   char set_dscp[BUFLEN_32];
   UINT32 clsKey;
   UINT32 flowId = 0;
   UINT32 queueMark = 0;

   cmd[0]       = '\0';
   egressIfc[0] = '\0';
   set_dscp[0]  = '\0';

   if (cInfo->etherType == ETH_P_IPV6)
   {
      strcpy(tblStr, "ip6tables");
   }
   else
   {
      strcpy(tblStr, "iptables");
   }

   clsKey = cInfo->key;

//   if (!IS_EMPTY_STRING(clsObj->X_BROADCOM_COM_egressInterface))
//   {
//      sprintf(egressIfc, "-o %s", clsObj->X_BROADCOM_COM_egressInterface);
//   }

   /* flowId mark */
   flowId = SKBMARK_SET_FLOW_ID(flowId, clsKey);

   /* priority queue mark */
   queueMark = SKBMARK_SET_Q(queueMark, priority);
   queueMark = SKBMARK_SET_FLOW_ID(queueMark, clsKey); 

   /* dscp mark */
   if (cInfo->DSCPMark != QOS_RESULT_NO_CHANGE &&
       cInfo->DSCPMark != QOS_RESULT_AUTO_MARK)
   {
      sprintf(set_dscp, "-j DSCP --set-dscp 0x%x", cInfo->DSCPMark);
   }

   /* Set up iptables rules.
    */
   if (cmdType == QOS_COMMAND_CONFIG)
   {
      /* Also set the marking rules in iptables POSTROUTING chain for routed egress interface
       */
      sprintf(cmd, "%s -w -t mangle -N rule%d", tblStr, clsKey);
      rut_doSystemAction("rutQos_roEgressMark", cmd);

      /* mark priority queue */
      sprintf(cmd, "%s -w -t mangle -A rule%d -j MARK --or-mark 0x%x", tblStr, clsKey, queueMark);
      rut_doSystemAction("rutQos_roEgressMark", cmd);

      if (!IS_EMPTY_STRING(set_dscp))
      {
         /* mark dscp */
         sprintf(cmd, "%s -w -t mangle -A rule%d %s", tblStr, clsKey, set_dscp);
         rut_doSystemAction("rutQos_roEgressMark", cmd);
      }
      
      sprintf(cmd, "%s -w -t mangle -A rule%d -j ACCEPT", tblStr, clsKey);
      rut_doSystemAction("rutQos_roEgressMark", cmd);

      sprintf(cmd, "%s -w -t mangle -A POSTROUTING %s -m mark --mark 0x%x/0x%x -j rule%d",
              tblStr, egressIfc, flowId, SKBMARK_FLOW_ID_M, clsKey);
      rut_doSystemAction("rutQos_roEgressMark", cmd);
   }
   else
   {
      sprintf(cmd, "%s -w -t mangle -D POSTROUTING %s -m mark --mark 0x%x/0x%x -j rule%d 2>/dev/null",
              tblStr, egressIfc, flowId, SKBMARK_FLOW_ID_M, clsKey);
      rut_doSystemAction("rutQos_roEgressMark", cmd);
      sprintf(cmd, "%s -w -t mangle -F rule%d 2>/dev/null", tblStr, clsKey);
      rut_doSystemAction("rutQos_roEgressMark", cmd);
      sprintf(cmd, "%s -w -t mangle -X rule%d 2>/dev/null", tblStr, clsKey);
      rut_doSystemAction("rutQos_roEgressMark", cmd);
   }
#endif // SUPPORT_NF_MANGLE

   return ret;

}  /* End of rutQos_roEgressMark() */

#if defined(SUPPORT_WANVLANMUX) && defined(SUPPORT_VLANCTL)
/***************************************************************************
// Function Name: rutQos_vlanTag
// Description  : set up vlanctl rules for traffic egress to a vlanmux interface.
// Parameters  : clsObj - QoS class information.
//               l2IfName - egress L2 intf name.
//                    cmdType - command type either config or unconfig.
// Returns      : CmsRet
****************************************************************************/
CmsRet rutQos_vlanTag(const CmsQosClassInfo *cInfo,
                      const char *l2IfName,
                      QosCommandType cmdType)
{
   if (cInfo->ethernetPriorityMark != QOS_RESULT_NO_CHANGE)
   {
      UINT32 flowId = 0;
      char l3IfName[CMS_IFNAME_LENGTH]={0};
      char *p;

      /* Do nothing if egress interface is not a vlan mux interface */
      if ((p = strchr(cInfo->egressIntfName, '.')) == NULL)
         return CMSRET_SUCCESS;

      /* Get layer3(base) interface name */
      if (cmsUtl_strstr(cInfo->egressIntfName, "ppp"))
         snprintf(l3IfName, sizeof(l3IfName), "%s%s", l2IfName, p);
      else  
         cmsUtl_strncpy(l3IfName, cInfo->egressIntfName, sizeof(l3IfName));

      /* flowId mark */
      flowId = SKBMARK_SET_FLOW_ID(flowId, cInfo->key);

      if (cmdType == QOS_COMMAND_CONFIG)
      {
         UINT32 tagRuleId = VLANCTL_DONT_CARE;
         SINT32 vlanId;

         /* Get interface VID */
         vlanId = qdmVlan_getVlanIdByIntfNameLocked(cInfo->egressIntfName);
         if (vlanId < 0)
         {
            cmsLog_error("could not get vlanId on ifName %s", cInfo->egressIntfName);
            return CMSRET_INTERNAL_ERROR;
         }

         /* setup vlanctl rules */
         vlanCtl_init();
		 
         /* Non-vlan packets egress to an untagged vlanmux interface shall be tagged with VID 0 and the class rule p-bits;
             Non-vlan packets egress to a tagged vlanmux interface shall be tagged with the interface VID and the class rule p-bits; */
         vlanCtl_initTagRule();
         vlanCtl_filterOnTxVlanDevice(l3IfName);
         vlanCtl_filterOnSkbMarkFlowId(SKBMARK_GET_FLOW_ID(flowId));
         vlanCtl_cmdPushVlanTag();
         vlanCtl_cmdSetTagVid(vlanId == -1 ? 0 : vlanId, 0);
         vlanCtl_cmdSetTagPbits(cInfo->ethernetPriorityMark, 0);
         vlanCtl_insertTagRule(l2IfName, VLANCTL_DIRECTION_TX, 0, VLANCTL_POSITION_BEFORE,
                            VLANCTL_DONT_CARE, &tagRuleId);

         /* Vlan packets egress to an untagged vlanmux interface shall have the packet p-bits re-marked by the class rule p-bits. No additional vlan tag is added;
             Vlan packets egress to a tagged vlanmux interface shall be additionally tagged with the packet VID, and the class rule p-bits. */
         vlanCtl_initTagRule();
         vlanCtl_filterOnTxVlanDevice(l3IfName);
         vlanCtl_filterOnSkbMarkFlowId(SKBMARK_GET_FLOW_ID(flowId));
         if (vlanId != -1)
         {
            vlanCtl_cmdPushVlanTag();
            vlanCtl_cmdCopyTagVid(1, 0);
         }
         vlanCtl_cmdSetTagPbits(cInfo->ethernetPriorityMark, 0);
         vlanCtl_insertTagRule(l2IfName, VLANCTL_DIRECTION_TX, 1, VLANCTL_POSITION_BEFORE,
                               VLANCTL_DONT_CARE, &tagRuleId);
		 
         vlanCtl_cleanup();

         cmsLog_debug("vlanCtl_insertTagRule: l2IfName = %s, l3IfName = %s, flowId = %d", l2IfName, l3IfName, SKBMARK_GET_FLOW_ID(flowId));
      }
      else
      {
         /* cleanup vlanctl rules */		 
         vlanCtl_init();
         vlanCtl_initTagRule();
         vlanCtl_filterOnTxVlanDevice(l3IfName);
         vlanCtl_filterOnSkbMarkFlowId(SKBMARK_GET_FLOW_ID(flowId));
         vlanCtl_removeTagRuleByFilter((char *) l2IfName, VLANCTL_DIRECTION_TX, 0);
         vlanCtl_removeTagRuleByFilter((char *) l2IfName, VLANCTL_DIRECTION_TX, 1);
         vlanCtl_cleanup();

         cmsLog_debug("vlanCtl_removeTagRuleByFilter: l2IfName = %s, l3IfName = %s, flowId = %d", l2IfName, l3IfName, SKBMARK_GET_FLOW_ID(flowId));
      }
   }
   
   return CMSRET_SUCCESS;
}  /* End of rutQos_vlanTag() */
#endif          

/** This function sends a message to DHCP daemon.
 *
 * @param type    (IN) the message type.
 * @param text    (IN) the message text.
 * @return void.
 */
void rutQos_sendDhcp(SINT32 type, char *cmd)
{
   char msg[sizeof(CmsMsgHeader) + BUFLEN_1024] = {0};
   CmsMsgHeader *hdr = (CmsMsgHeader *)msg;
   char *body = &msg[sizeof(CmsMsgHeader)];

   hdr->type          = type;
   hdr->src           = mdmLibCtx.eid;
   hdr->dst           = EID_DHCPD;
   hdr->flags_request = 1;
   hdr->flags_bounceIfNotRunning = 1;
   hdr->dataLength    = strlen(cmd);
   cmsUtl_strncpy(body, cmd, BUFLEN_1024);

   cmsMsg_send(mdmLibCtx.msgHandle, hdr);

}  /* End of rutQos_sendDhcp() */




#ifdef DMP_QOS_1

UBOOL8 rutQos_isAnotherClassPolicerExist_igd(UINT32 excludeClsKey,
                                             const char *egressL2IntfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   QMgmtClassificationObject *cObj = NULL;
   UBOOL8 exist=FALSE;

   cmsLog_debug("Entered: egressL2IntfName=%s exclude=%d", egressL2IntfName, excludeClsKey);

   while (!exist &&
          cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack, (void **) &cObj) == CMSRET_SUCCESS)
   {
      if (cObj->X_BROADCOM_COM_ClassKey != excludeClsKey &&
          cObj->classificationEnable &&
          cObj->classQueue > 0)
      {
         CmsRet r2;
         char l2IntfName[CMS_IFNAME_LENGTH]={0};

         r2 = qdmQos_getQueueInfoByClassQueueLocked(cObj->classQueue,
                                             NULL, NULL, l2IntfName);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get QueueInfo on %d, ret=%d",
                          cObj->classQueue, r2);
         }
         else
         {
            if (cmsUtl_strcmp(egressL2IntfName, l2IntfName) == 0)
            {
               cmsLog_debug("Found exist l2IntfName %s at %d",
                             l2IntfName, cObj->classQueue);
               exist = TRUE;
            }
         }
      }

      cmsObj_free((void **)&cObj);
   }

   return exist;
}

CmsRet rutQos_fillPolicerInfo_igd(const SINT32 instance, const tmctl_dir_e dir, const UINT32 policerInfo)
{
    QMgmtPolicerObject *pObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret;
    
    if (instance <= 0)
    {
        cmsLog_error("invalid instance=%d", instance);
        return CMSRET_INVALID_ARGUMENTS;
    }
    
    cmsLog_debug("policerUpdateIndex insance %d, dir %d, policerInfo %d.", instance, dir, policerInfo);
    /* the fullpath of the Policer table is InternetGatewayDevice.QueueManagement.Policer.{i}.
     * so we just need to push the instance number into the first position
     * of the instance id stack.
     */
    PUSH_INSTANCE_ID(&iidStack, instance);
    if ((ret = cmsObj_get(MDMOID_Q_MGMT_POLICER, &iidStack, 0, (void **) &pObj)) != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsObj_get <MDMOID_Q_MGMT_POLICER> returns error. ret=%d", ret);
        return ret;
    }
    
    if (dir == TMCTL_DIR_UP)
        pObj->X_BROADCOM_COM_UsPolicerInfo = policerInfo;
    else
        pObj->X_BROADCOM_COM_DsPolicerInfo = policerInfo;
    
    if ((ret = cmsObj_set((void *)pObj, &iidStack)) != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsObj_set <MDMOID_Q_MGMT_POLICER> returns error. ret=%d", ret);	  
    }
    
    cmsObj_free((void **) &pObj);	
    return ret;
}

UBOOL8 rutQos_isAnotherClassRateLimitExist_igd(UINT32 excludeClsKey,
                                               const char *egressL2IntfName)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   QMgmtClassificationObject *cObj = NULL;
   UBOOL8 exist=FALSE;

   cmsLog_debug("Entered: egressL2IntfName=%s exclude=%d", egressL2IntfName, excludeClsKey);

   while (!exist &&
          cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack, (void **) &cObj) == CMSRET_SUCCESS)
   {
      if (cObj->X_BROADCOM_COM_ClassKey != excludeClsKey &&
          cObj->X_BROADCOM_COM_ClassRate != QOS_RESULT_NO_CHANGE &&
          cObj->classificationEnable &&
          cObj->classQueue > 0)
      {
         CmsRet r2;
         char l2IntfName[CMS_IFNAME_LENGTH]={0};

         r2 = qdmQos_getQueueInfoByClassQueueLocked(cObj->classQueue,
                                             NULL, NULL, l2IntfName);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not get QueueInfo on %d, ret=%d",
                          cObj->classQueue, r2);
         }
         else
         {
            if (cmsUtl_strcmp(egressL2IntfName, l2IntfName) == 0)
            {
               cmsLog_debug("Found exist l2IntfName %s at %d",
                             l2IntfName, cObj->classQueue);
               exist = TRUE;
            }
         }
      }

      cmsObj_free((void **)&cObj);
   }

   return exist;
}


/* Delete all the QoS classes associated with a layer 3 interface.
 */
CmsRet rutQos_qMgmtClassDelete_igd(const char *l3Ifcname)
{
   InstanceIdStack iidStack, iidStackPrev;
   _QMgmtClassificationObject *cObj = NULL;
   char   ifcname[CMS_IFNAME_LENGTH];
   CmsRet ret;

   cmsLog_debug("enter. ifcname=%s", l3Ifcname);

   INIT_INSTANCE_ID_STACK(&iidStack);
   iidStackPrev = iidStack;
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack, (void **)&cObj)) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(l3Ifcname, cObj->X_BROADCOM_COM_egressInterface) == 0)
      {
         MdmPathDescriptor pathDesc;

         /* delete this class object instance */
         pathDesc.oid = MDMOID_Q_MGMT_CLASSIFICATION;
         pathDesc.iidStack = iidStack;
         pathDesc.paramName[0] = '\0';
         if ((ret = cmsObj_deleteInstance(pathDesc.oid, &pathDesc.iidStack)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_deleteInstance error %d", ret);
            cmsObj_free((void **)&cObj);
            return ret;
         }

         /* since this instance has been deleted, we want to set the iidStack to
          * the previous instance, so that we can continue to do getNext.
          */
         iidStack = iidStackPrev;
      }
      else
      {
         ifcname[0] = '\0';

         if (cmsUtl_strcmp(cObj->classInterface, MDMVS_LOCAL) == 0 ||
             cmsUtl_strcmp(cObj->classInterface, MDMVS_LAN)   == 0 ||
             cmsUtl_strcmp(cObj->classInterface, MDMVS_WAN)   == 0)
         {
            cmsObj_free((void **)&cObj);
            /* save this iidStack in case we want to do a getNext from this instance */
            iidStackPrev = iidStack;
            continue;
         }

         /* find intfname from class interface full path */
         if ((ret = rut_fullPathToIntfname(cObj->classInterface, ifcname)) != CMSRET_SUCCESS)
         {
            cmsLog_error("rut_fullPathToIntfname returns error. ret=%d", ret);
            cmsLog_error("classIntf=%s", cObj->classInterface);
            cmsObj_free((void **)&cObj);
            return ret;
         }

         if (cmsUtl_strcmp(l3Ifcname, ifcname) == 0)
         {
            MdmPathDescriptor pathDesc;

            /* delete this class object instance */
            pathDesc.oid = MDMOID_Q_MGMT_CLASSIFICATION;
            pathDesc.iidStack = iidStack;
            pathDesc.paramName[0] = '\0';
            if ((ret = cmsObj_deleteInstance(pathDesc.oid, &pathDesc.iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsObj_deleteInstance error %d", ret);
               cmsObj_free((void **)&cObj);
               return ret;
            }

            /* since this instance has been deleted, we want to set the iidStack to
             * the previous instance, so that we can continue to do getNext.
             */
            iidStack = iidStackPrev;
         }
      }
      cmsObj_free((void **)&cObj);

      /* save this iidStack in case we want to do a getNext from this instance */
      iidStackPrev = iidStack;
   }
   if (ret != CMSRET_NO_MORE_INSTANCES && ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_getNext <MDMOID_Q_MGMT_CLASSIFICATION> returns error. ret=%d", ret);
      return ret;
   }

   return CMSRET_SUCCESS;

}  /* End of rutQos_qMgmtClassDelete() */

/* Delete or unconfigure all the QoS classes associated with a queue instance.
 */
CmsRet rutQos_qMgmtClassOperation_igd(SINT32 queueInstance, UBOOL8 isRealDel)
{
   InstanceIdStack iidStack, iidStackPrev;
   _QMgmtClassificationObject *cObj = NULL;
   CmsRet ret;

   cmsLog_debug("Enter: instance=%d isRealDel=%d", queueInstance, isRealDel);

   INIT_INSTANCE_ID_STACK(&iidStack);
   iidStackPrev = iidStack;
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack, (void **)&cObj)) == CMSRET_SUCCESS)
   {
      if (cObj->classQueue == queueInstance)
      {
         if (isRealDel)
         {
            MdmPathDescriptor pathDesc;

            /* delete this class object instance */
            pathDesc.oid = MDMOID_Q_MGMT_CLASSIFICATION;
            pathDesc.iidStack = iidStack;
            pathDesc.paramName[0] = '\0';
            if ((ret = cmsObj_deleteInstance(pathDesc.oid, &pathDesc.iidStack)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsObj_deleteInstance error %d", ret);
               cmsObj_free((void **)&cObj);
               return ret;
            }

            /* since this instance has been deleted, we want to set the iidStack to
             * the previous instance, so that we can continue to do getNext.
             */
            iidStack = iidStackPrev;
         }
         else
         {
            /* just unconfig the class */
            if ((ret = rutQos_qMgmtClassConfig(QOS_COMMAND_UNCONFIG, cObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("rutQos_qMgmtClassConfig <QOS_COMMAND_UNCONFIG> returns error. ret=%d", ret);
               cmsObj_free((void **)&cObj);
               return ret;
            }
         }
      }
      cmsObj_free((void **)&cObj);

      /* save this iidStack in case we want to do a getNext from this instance */
      iidStackPrev = iidStack;
   }
   if (ret != CMSRET_NO_MORE_INSTANCES && ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_getNext <MDMOID_Q_MGMT_CLASSIFICATION> returns error. ret=%d", ret);
      return ret;
   }

   return CMSRET_SUCCESS;

}  /* End of rutQos_qMgmtClassOperation() */


CmsRet rutQos_fillClassKeyArray_igd(UINT32 *keyArray)
{
   InstanceIdStack iidStack;
   QMgmtClassificationObject *cObj = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNext(MDMOID_Q_MGMT_CLASSIFICATION, &iidStack, (void **)&cObj)) == CMSRET_SUCCESS)
   {
      if (cObj->X_BROADCOM_COM_ClassKey < 1 || cObj->X_BROADCOM_COM_ClassKey > QOS_CLS_MAX_ENTRY)
      {
         cmsLog_error("Found invalid clsKey %d", cObj->X_BROADCOM_COM_ClassKey);
         ret = CMSRET_INTERNAL_ERROR;
         cmsObj_free((void **)&cObj);
         break;
      }

      keyArray[cObj->X_BROADCOM_COM_ClassKey - 1] = 1;
      cmsObj_free((void **)&cObj);
   }

   if (ret == CMSRET_NO_MORE_INSTANCES)
   {
      ret = CMSRET_SUCCESS;
   }

   return ret;
}

#endif  /* DMP_QOS_1 */


CmsRet rutQos_getAvailableClsKey(UINT32 *clsKey)
{
   UINT32 key;
   UINT32 keyArray[QOS_CLS_MAX_ENTRY] = {0};
   CmsRet ret = CMSRET_SUCCESS;

   /* initialize the return class key to 0 */
   *clsKey = 0;
      
   /* find out which keys are in use, this is data model dependent */
   ret = rutQos_fillClassKeyArray(keyArray);
   if (ret == CMSRET_SUCCESS)
   {
      for (key = 0; key < QOS_CLS_MAX_ENTRY; key++)
      {
         cmsLog_debug("keyArray[%d]=%d", key, keyArray[key]);
      }

      for (key = 0; key < QOS_CLS_MAX_ENTRY; key++)
      {
         if (keyArray[key] == 0)
         {
            cmsLog_debug("found available clsKey=%d", key);
            *clsKey = key + 1;   /* class key starts at 1 */
            ret = CMSRET_SUCCESS;
            break;
         }
      }

      if (key >= QOS_CLS_MAX_ENTRY)
      {
         cmsLog_error("Classification key is not available");
      }
   }
   
   return ret;
   
}  /* End of rutQos_getAvailableClsKey() */


#endif  /* SUPPORT_QOS */
