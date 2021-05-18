/***********************************************************************
 *
 *  Copyright (c) 2008  Broadcom Corporation
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "odl.h"
#include "cms_core.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_math.h"
#include "rut_util.h"
#include "rut_lan.h"
#include "rut_wan.h"
#include "rut_qos.h"
#include "prctl.h"
#include "cms_actionlog.h"

#ifdef SUPPORT_RDPA
#include "rdpactl_api.h"
#endif

CmsRet rut_validateObjects(const void *newObj, const void *currObj)
{

   cmsLog_debug("newObj=%p, currObj=%p\n", newObj, currObj);

   if (currObj == NULL && newObj == NULL)
   {
      cmsLog_error("Both obj are NULL");
      return CMSRET_INVALID_PARAM_VALUE;
   }

   return CMSRET_SUCCESS;
}

/* Converts a commands string to an array of string to be used by execv and return
* the pointer to the array and fills the array size.
*
* CALLER NEEDS TO MAKE COPY OF cmdString and FREE THE ARRAY POINTER
*/
#ifndef DESKTOP_LINUX
static char** strToArray(char* cmdString, char separator, SINT32 *arrayCount)
{
   char** retArray = NULL;
   SINT32 arrayLen=0;
   char* c=NULL;

   if (cmdString == NULL || separator == '\0')
   {
      /* Invalid parameters */
      cmsLog_error("Invalid param");
      return retArray;
   }

   /* First calculate number of array */
   c = cmdString;
   while (*c != '\0')
   {
      if (*c == separator)
      {
         arrayLen++;
         c++;
         /* skip extra space here */
         while (*c == separator)
         {
            c++;
         }
      }
      else
      {
         c++;
      }
   }

   arrayLen = arrayLen + 1;
   retArray = cmsMem_alloc((arrayLen + 1) * sizeof(*retArray), ALLOC_ZEROIZE);
   retArray[arrayLen] = NULL;

   /* Now parse the string again to fill in the array */
   c = cmdString;
   retArray[0] = cmdString;
   arrayLen = 1;

   while (*c != '\0')
   {
      if (*c == separator)
      {
         *c = '\0';
         c++;
         /* skip extra space here */
         while (*c == separator)
         {
            c++;
         }
         retArray[arrayLen++] = &c[0];
      }
      else
      {
         c++;
      }
   }

   /*skip redirection parameter, it's handled by freopen*/
   if (cmsUtl_strstr(retArray[arrayLen-1], ">") && cmsUtl_strstr(retArray[arrayLen-1], "/dev/null"))
   {
      retArray[--arrayLen] = NULL;
   }


   if (arrayCount != NULL )
   {
      /* if last parameter in the retArray is "" (like from the calls from rutIpt_setupFirewallForDHCPv6,
      *  it should be removed.  Otherwise, execv will not run correctly.
      */
      if (!cmsUtl_strcmp(retArray[arrayLen-1], ""))
      {
         retArray[--arrayLen] = NULL;
         *arrayCount = arrayLen;
      }
      else
      {
         *arrayCount = arrayLen + 1;
      }
   }
   return retArray;
}
#endif /* DESKTOP_LINUX */


void rut_doSystemAction(const char* from, char *cmd)
{
   cmsLog_debug("%s -- %s", from, cmd);
   calLog_shell("%s\n", cmd);

#ifndef DESKTOP_LINUX
   /* For last char is '&', do a system call so that the call will
   * not be blocked.
   */
   if (cmsUtl_strstr(cmd, "&"))
   {
      SINT32 strEnd = cmsUtl_strlen(cmd)-1;
      SINT32 i;

      /* skip trailing spaces */
      for (i=strEnd; i > 0; i--)
      {
         if (cmd[i] == ' ')
         {
            cmsLog_notice("skip end space");
            continue;
         }

         if (cmd[i] == '&')
         {
            cmsLog_notice("None blocking system call on %s ", cmd);
            system(cmd);
            return;
         }
      }
   }

   /* For this two types of commands, use prctl_runCommandInShellBlocking which is a little faster than system()
   * 1). NOT redirect to /dev/null
   * 2). and other special char like single and double quotation mark,  backticks, ampersand, and parentheses and brackets
   */
   if (((cmsUtl_strstr(cmd, ">") && !cmsUtl_strstr(cmd, "/dev/null"))) ||
       (cmd[0] == '/') ||
       (cmsUtl_strstr(cmd, "?")) ||
       (cmsUtl_strstr(cmd, "??")) ||
       (cmsUtl_strstr(cmd, "'")) ||
       (cmsUtl_strstr(cmd, "\"")) ||
       (cmsUtl_strstr(cmd, "@")) ||
       (cmsUtl_strstr(cmd, ")")) ||
       (cmsUtl_strstr(cmd, "(")) ||
       (cmsUtl_strstr(cmd, "]")) ||
       (cmsUtl_strstr(cmd, "[")))
   {
      /* if there is any redirection, use the system call */
      cmsLog_notice("%s use prctl_runCommandInShellBlocking", cmd);
      prctl_runCommandInShellBlocking(cmd);
   }
   else
   {
      char **cmdArray=NULL;
      char cmdBuf[BUFLEN_1024]={0};
      SINT32 cmdCount;
      SINT32 pid;
      char programWithPath[BUFLEN_1024]={0};

      cmsUtl_strncpy(cmdBuf, cmd, sizeof(cmdBuf));
      if ((cmdArray = strToArray(cmdBuf, ' ', &cmdCount)) == NULL)
      {
         cmsLog_error("Failed to parse string to array.");
         return;
      }
	
      pid = fork ();
      if (pid != 0)
      {
         int status;

         waitpid(pid, &status, 0);
         cmsLog_notice("execlp exits");
      }
      else
      {
         if (cmsUtl_strstr(cmd, ">") && cmsUtl_strstr(cmd, "/dev/null"))
         {
            /* It's redirect to /dev/null
            */
            if (!freopen("/dev/null","w",stderr))
            {
                perror("failed to open file");
            }
         }
         /* For now only 4 busybox commands bellow are in /sbin/ instead of /bin
         * so just do a quick check on them.  Later on we may need to revisit this
         * and have a better way (populate a table at compile time, etc.) to do
         * do this automatically.
         */
         if ( !cmsUtl_strcmp(cmdArray[0], "route") ||
             !cmsUtl_strcmp(cmdArray[0], "insmod") ||
             !cmsUtl_strcmp(cmdArray[0], "ifconfig") ||
             !cmsUtl_strcmp(cmdArray[0], "sendarp") ||
             !cmsUtl_strcmp(cmdArray[0], "rmmod"))
         {
            cmsUtl_strcpy(programWithPath, "/sbin/");
         }
         else
         {
            cmsUtl_strcpy(programWithPath, "/bin/");
         }
         cmsUtl_strncat(programWithPath, BUFLEN_1024, cmdArray[0]);

         cmsLog_notice("programWithPath %s", programWithPath);
         execv(programWithPath,  &cmdArray[0]);
         perror("execv");
         cmsLog_error("execv failed!");
         exit(1);
      }

      /* Free the cmdArray which is allocate in strToArray */
      cmsMem_free(cmdArray);
   }
#endif

}


CmsRet rut_doSystemActionWithTimeout(const char* from, char *cmd)
{
   CmsRet ret = CMSRET_SUCCESS;
#ifndef DESKTOP_LINUX
   int rc = 0;
#endif

   cmsLog_debug("%s -- %s", from, cmd);
   calLog_shell("%s\n", cmd);

#ifndef DESKTOP_LINUX
   cmsLog_debug("%s use prctl_runCommandInShellWithTimeout", cmd);
   rc = prctl_runCommandInShellWithTimeout(cmd);
   if (rc == -1)
   {
      ret = CMSRET_TIMED_OUT;
   }
#endif
   return ret;
}




CmsRet rut_getBCastFromIpSubnetMask(char* inIpStr, char* inSubnetMaskStr, char *outBcastStr)
{
   struct in_addr ip;
   struct in_addr subnetMask;
   struct in_addr bCast;
   CmsRet ret = CMSRET_SUCCESS;

   if (inIpStr == NULL || inSubnetMaskStr == NULL || outBcastStr == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   ip.s_addr = inet_addr(inIpStr);
   subnetMask.s_addr = inet_addr(inSubnetMaskStr);
   bCast.s_addr = ip.s_addr | ~subnetMask.s_addr;
   strcpy(outBcastStr, inet_ntoa(bCast));

   return ret;

}


#if defined(DMP_BASELINE_1)
/*
 * These functions which deal with incrementing and decrementing number of
 * objects use TR98 objects in their switch statements so they are applicable
 * to Legacy TR98 mode and Hybrid TR98+TR181 mode.
 * TR181 objects will have their own set of count increment/decrement funcs.
 */

#ifdef DMP_DEVICEASSOCIATION_1
void rut_modifyNumManageableDevices(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_MANAGEMENT_SERVER,
                        MDMOID_MANAGEABLE_DEVICE,
                        iidStack,
                        delta);
}
#endif


void rut_modifyNumLanDev(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_IGD,
                        MDMOID_LAN_DEV,
                        iidStack,
                        delta);
}


void rut_modifyNumWanDev(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_IGD,
                        MDMOID_WAN_DEV,
                        iidStack,
                        delta);
}


void rut_modifyNumEthIntf(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_LAN_DEV,
                        MDMOID_LAN_ETH_INTF,
                        iidStack,
                        delta);
}

void rut_modifyNumUsbIntf(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_LAN_DEV,
                        MDMOID_LAN_USB_INTF,
                        iidStack,
                        delta);
}

void rut_modifyNumMocaIntf(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_LAN_DEV,
                        MDMOID_LAN_MOCA_INTF,
                        iidStack,
                        delta);
}

void rut_modifyNumWlanConf(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_LAN_DEV,
                        MDMOID_LAN_WLAN,
                        iidStack,
                        delta);
}

void rut_modifyNumEponIntf(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_LAN_DEV,
                        MDMOID_LAN_EPON_INTF,
                        iidStack,
                        delta);
}

void rut_modifyNumLanIpIntf(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_LAN_HOST_CFG,
                        MDMOID_LAN_IP_INTF,
                        iidStack,
                        delta);
}


void rut_modifyNumLanHosts(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_LAN_HOSTS,
                        MDMOID_LAN_HOST_ENTRY,
                        iidStack,
                        delta);
}


void rut_modifyNumWanConn(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_WAN_DEV,
                        MDMOID_WAN_CONN_DEVICE,
                        iidStack,
                        delta);
}


void rut_modifyNumIpConn(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_WAN_CONN_DEVICE,
                        MDMOID_WAN_IP_CONN,
                        iidStack,
                        delta);
}


void rut_modifyNumPppConn(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_WAN_CONN_DEVICE,
                        MDMOID_WAN_PPP_CONN,
                        iidStack,
                        delta);
}


void rut_modifyNumIpPortMapping(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_WAN_IP_CONN,
                        MDMOID_WAN_IP_CONN_PORTMAPPING,
                        iidStack,
                        delta);
}



void rut_modifyNumPppPortMapping(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_WAN_PPP_CONN,
                        MDMOID_WAN_PPP_CONN_PORTMAPPING,
                        iidStack,
                        delta);
}


void rut_modifyNumL3Forwarding(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_L3_FORWARDING,
                        MDMOID_L3_FORWARDING_ENTRY,
                        iidStack,
                        delta);
}


#ifdef DMP_BRIDGING_1

void rut_modifyNumL2Bridging(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_L2_BRIDGING,
                        MDMOID_L2_BRIDGING_ENTRY,
                        iidStack,
                        delta);
}

void rut_modifyNumL2BridgingFilter(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_L2_BRIDGING,
                        MDMOID_L2_BRIDGING_FILTER,
                        iidStack,
                        delta);
}

void rut_modifyNumL2BridgingIntf(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_L2_BRIDGING,
                        MDMOID_L2_BRIDGING_INTF,
                        iidStack,
                        delta);
}

#endif /* DMP_BRIDGING_1 */


#ifdef DMP_QOS_1

void rut_modifyNumQMgmtQueue(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_Q_MGMT,
                        MDMOID_Q_MGMT_QUEUE,
                        iidStack,
                        delta);
}

void rut_modifyNumQMgmtPolicer(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_Q_MGMT,
                        MDMOID_Q_MGMT_POLICER,
                        iidStack,
                        delta);
}

void rut_modifyNumQMgmtClassification(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_Q_MGMT,
                        MDMOID_Q_MGMT_CLASSIFICATION,
                        iidStack,
                        delta);
}

#endif /* DMP_QOS_1 */

#ifdef DMP_PERIODICSTATSBASE_1
void rut_modifyNumSampleSets(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_PERIODIC_STAT,
                        MDMOID_SAMPLE_SET,
                        iidStack,
                        delta);
}

void rut_modifyNumParameters(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_SAMPLE_SET,
                        MDMOID_SAMPLE_PARAMETER,
                        iidStack,
                        delta);
}
#endif

void rut_modifyNumVendorConfigFiles(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_IGD_DEVICE_INFO,
                        MDMOID_VENDOR_CONFIG_FILE,
                        iidStack,
                        delta);
}


void rut_modifyNumGeneric(MdmObjectId ancestorOid,
                          MdmObjectId decendentOid,
                          const InstanceIdStack *iidStack,
                          SINT32 delta)
{
   InstanceIdStack ancestorIidStack = *iidStack;
   void *mdmObj=NULL;
   UINT32 *num;
   CmsRet ret;

   if (mdmShmCtx->inMdmInit)
   {
      /*
       * During system startup, we might have loaded from a config file.
       * The config file already contains the correct count of these objects.
       * So don't update the count in the parent object.
       */
      cmsLog_debug("don't update count in oid %d for new object oid %d (delta=%d)",
                   ancestorOid, decendentOid, delta);
      return;
   }

   ret = cmsObj_getAncestor(ancestorOid, decendentOid, &ancestorIidStack, (void **) &mdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find ancestor obj %d (decendent oid %d) iidStack=%s",
                   ancestorOid, decendentOid,
                   cmsMdm_dumpIidStack(iidStack));
      return;
   }

   switch (decendentOid)
   {
#ifdef DMP_DEVICEASSOCIATION_1
   case MDMOID_MANAGEABLE_DEVICE:
      num = &(((_ManagementServerObject *) mdmObj)->manageableDeviceNumberOfEntries);
      break;
#endif

   case MDMOID_LAN_DEV:
      num = &(((_IGDObject *) mdmObj)->LANDeviceNumberOfEntries);
      break;

   case MDMOID_WAN_DEV:
      num = &(((_IGDObject *) mdmObj)->WANDeviceNumberOfEntries);
      break;

   case MDMOID_LAN_ETH_INTF:
      num = &(((_LanDevObject *) mdmObj)->LANEthernetInterfaceNumberOfEntries);
      break;

   case MDMOID_LAN_USB_INTF:
      num = &(((_LanDevObject *) mdmObj)->LANUSBInterfaceNumberOfEntries);
      break;
#ifdef DMP_X_BROADCOM_COM_EPON_1
   case MDMOID_LAN_EPON_INTF:
      num = &(((_LanDevObject *) mdmObj)->X_BROADCOM_COM_LANEponInterfaceNumberOfEntries);
      break;
#endif
   case MDMOID_LAN_WLAN:
      num = &(((_LanDevObject *) mdmObj)->LANWLANConfigurationNumberOfEntries);
      break;

   case MDMOID_LAN_MOCA_INTF:
      num = &(((_LanDevObject *) mdmObj)->X_BROADCOM_COM_LANMocaInterfaceNumberOfEntries);
      break;

   case MDMOID_LAN_IP_INTF:
      num = &(((_LanHostCfgObject *) mdmObj)->IPInterfaceNumberOfEntries);
      break;

   case MDMOID_LAN_HOST_ENTRY:
      num = &(((_LanHostsObject *) mdmObj)->hostNumberOfEntries);
      break;

   case MDMOID_WAN_CONN_DEVICE:
      num = &(((_WanDevObject *) mdmObj)->WANConnectionNumberOfEntries);
      break;

   case MDMOID_WAN_IP_CONN:
      num = &(((_WanConnDeviceObject *) mdmObj)->WANIPConnectionNumberOfEntries);
      break;

   case MDMOID_WAN_PPP_CONN:
      num = &(((_WanConnDeviceObject *) mdmObj)->WANPPPConnectionNumberOfEntries);
      break;

   case MDMOID_WAN_IP_CONN_PORTMAPPING:
      num = &(((_WanIpConnObject *) mdmObj)->portMappingNumberOfEntries);
      break;

   case MDMOID_WAN_PPP_CONN_PORTMAPPING:
      num = &(((_WanPppConnObject *) mdmObj)->portMappingNumberOfEntries);
      break;

   case MDMOID_L3_FORWARDING_ENTRY:
      num = &(((_L3ForwardingObject *) mdmObj)->forwardNumberOfEntries);
      break;

#ifdef DMP_BRIDGING_1
   case MDMOID_L2_BRIDGING_ENTRY:
      num = &(((_L2BridgingObject *) mdmObj)->bridgeNumberOfEntries);
      break;

   case MDMOID_L2_BRIDGING_FILTER:
      num = &(((_L2BridgingObject *) mdmObj)->filterNumberOfEntries);
      break;

   case MDMOID_L2_BRIDGING_INTF:
      num = &(((_L2BridgingObject *) mdmObj)->availableInterfaceNumberOfEntries);
      break;
#endif

#ifdef DMP_QOS_1
   case MDMOID_Q_MGMT_QUEUE:
      num = &(((_QMgmtObject *) mdmObj)->queueNumberOfEntries);
      break;

   case MDMOID_Q_MGMT_POLICER:
      num = &(((_QMgmtObject *) mdmObj)->policerNumberOfEntries);
      break;

   case MDMOID_Q_MGMT_CLASSIFICATION:
      num = &(((_QMgmtObject *) mdmObj)->classificationNumberOfEntries);
      break;
#endif

#ifdef DMP_PERIODICSTATSBASE_1
   case MDMOID_SAMPLE_SET:
      num = &(((_PeriodicStatObject *) mdmObj)->sampleSetNumberOfEntries);
      break;


   case MDMOID_SAMPLE_PARAMETER:
      num = &(((_SampleSetObject *) mdmObj)->parameterNumberOfEntries);
      break;
#endif

   case MDMOID_VENDOR_CONFIG_FILE:
      num = &(((_IGDDeviceInfoObject *) mdmObj)->vendorConfigFileNumberOfEntries);
      break;

      /* these are device2 objects that applies to both TR98 and TR181 */
#ifdef DMP_DEVICE2_XMPPBASIC_1	
   case MDMOID_DEV2_XMPP_CONN:
      num = &(((Dev2XmppObject*) mdmObj)->connectionNumberOfEntries);
      break;
#endif
#ifdef DMP_DEVICE2_XMPPADVANCED_1
   case MDMOID_DEV2_XMPP_CONN_SERVER:
      num = &(((Dev2XmppConnObject*) mdmObj)->serverNumberOfEntries);
      break;
#endif

#ifdef DMP_DEVICE2_SM_BASELINE_1
   case MDMOID_EXEC_ENV:
      num = &(((_SwModulesObject *) mdmObj)->execEnvNumberOfEntries);
      break;

   case MDMOID_DU:
      num = &(((_SwModulesObject *) mdmObj)->deploymentUnitNumberOfEntries);
      break;

   case MDMOID_EU:
      num = &(((_SwModulesObject *) mdmObj)->executionUnitNumberOfEntries);
      break;

   case MDMOID_BUS_OBJECT_PATH:
      num = &(((_BusObject *) mdmObj)->objectPathNumberOfEntries);
      break;

   case MDMOID_BUS_INTERFACE:
      num = &(((_BusObjectPathObject *) mdmObj)->interfaceNumberOfEntries);
      break;

   case MDMOID_BUS_METHOD:
      num = &(((_BusInterfaceObject *) mdmObj)->methodNumberOfEntries);
      break;

   case MDMOID_BUS_SIGNAL:
      num = &(((_BusInterfaceObject *) mdmObj)->signalNumberOfEntries);
      break;

   case MDMOID_BUS_PROPERTY:
      num = &(((_BusInterfaceObject *) mdmObj)->propertyNumberOfEntries);
      break;

   case MDMOID_BUS_CLIENT_PRIVILEGE:
      num = &(((_BusClientObject *) mdmObj)->privilegeNumberOfEntries);
      break;
#endif

#ifdef DMP_X_BROADCOM_COM_CONTAINER_1
   case MDMOID_CONTAINER_INFO:
      num = &(((_ContainerObject *) mdmObj)->containerNumberOfEntries);
      break;
#endif


#ifdef DMP_DEVICE2_ETHLAG_1
      case MDMOID_LA_G:
       num = &(((Dev2EthernetObject *) mdmObj)->LAGNumberOfEntries);
         break;
#endif


   default:
      cmsLog_error("cannot handle oid %d", decendentOid);
      cmsObj_free(&mdmObj);
      return;
   }

   if ((delta < 0) &&
       (((UINT32) (delta * -1)) > *num))
   {
      cmsLog_error("underflow detected for %s %s, delta=%d num=%d",
                   mdm_oidToGenericPath(ancestorOid),
                   cmsMdm_dumpIidStack(&ancestorIidStack),
                   delta, *num);
   }
   else
   {
      *num += delta;

      if ((ret = cmsObj_set(mdmObj, &ancestorIidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("set on %s %s failed, ret=%d",
                      mdm_oidToGenericPath(ancestorOid),
                      cmsMdm_dumpIidStack(&ancestorIidStack),
                      ret);
      }
   }

   cmsObj_free(&mdmObj);

   return;
}

#endif  /* DMP_BASELINE_1 */




UINT32 rut_sendMsgToOmcid(CmsMsgType msgType, void *msgData, UINT32 msgDataLen)
{
   UBOOL8 event=FALSE;
   UBOOL8 request=TRUE;
   UBOOL8 getReply=FALSE;
   UINT32 wordData=0;

   return (rut_sendMsgCommon(EID_OMCID, msgType, wordData,
                             event, request, getReply,
                             msgData, msgDataLen));
}


UINT32 rut_sendMsgToSmd(CmsMsgType msgType, UINT32 wordData,
                        const void *msgData, UINT32 msgDataLen)
{
   UBOOL8 event=FALSE;
   UBOOL8 request=TRUE;
   UBOOL8 getReply=TRUE;

   return (rut_sendMsgCommon(EID_SMD, msgType, wordData,
                             event, request, getReply,
                             msgData, msgDataLen));
}


UINT32 rut_sendEventMsgToSmd(CmsMsgType msgType, UINT32 wordData,
                             const void *msgData, UINT32 msgDataLen)
{
   UBOOL8 event=TRUE;
   UBOOL8 request=FALSE;
   UBOOL8 getReply=FALSE;

   return (rut_sendMsgCommon(EID_SMD, msgType, wordData,
                             event, request, getReply,
                             msgData, msgDataLen));
}


CmsRet rut_sendMsgToSsk(CmsMsgType msgType, UINT32 wordData, void *msgData, UINT32 msgDataLen)
{
   UBOOL8 event=FALSE;
   UBOOL8 request=TRUE;
   UBOOL8 getReply=FALSE;

   return ((CmsRet) rut_sendMsgCommon(EID_SSK, msgType, wordData,
                                     event, request, getReply,
                                     msgData, msgDataLen));
}


UINT32 rut_sendMsgCommon(CmsEntityId dst, CmsMsgType msgType, UINT32 wordData,
                       UBOOL8 event, UBOOL8 request, UBOOL8 getReply,
                       const void *msgData, UINT32 msgDataLen)
{
   UINT32 ret;
   CmsMsgHeader *msg;
   char *data;
   void *msgBuf;

   cmsLog_debug("dst=0x%x msgType=0x%x wordData=0x%x event=%d request=%d len=%d",
                 dst, msgType, wordData, event, request, msgDataLen);

   if (!event && !request)
   {
      cmsLog_error("Either event or request must be set!");
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (event && request)
   {
      cmsLog_error("event and request cannot both be set!");
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* for msg with user data */
   if (msgData != NULL && msgDataLen != 0)
   {
      msgBuf = cmsMem_alloc(sizeof(CmsMsgHeader) + msgDataLen, ALLOC_ZEROIZE);
   }
   else
   {
      msgBuf = cmsMem_alloc(sizeof(CmsMsgHeader), ALLOC_ZEROIZE);
   }

   if (msgBuf == NULL)
   {
      cmsLog_error("message allocation failed");
      return CMSRET_INTERNAL_ERROR;
   }

   msg = (CmsMsgHeader *)msgBuf;

   /* initialize some common fields */
   msg->src = mdmLibCtx.eid;
   msg->dst = dst;
   msg->flags_request = request;
   msg->flags_event = event;
   msg->type = msgType;
   msg->wordData = wordData;

   if (msgData != NULL && msgDataLen != 0)
   {
      data = (char *) (msg + 1);
      msg->dataLength = msgDataLen;
      memcpy(data, msgData, msgDataLen);
   }

   if (getReply)
   {
      cmsLog_debug("calling sendAndGetReply for msg 0x%x to 0x%x",
                    msgType, dst);
      /*
       * Note that for RESTART_PPP and RESTART_DHCP msg, the returned
       * value is the pid of the newly started process, not a CmsRet code.
       */
      ret = cmsMsg_sendAndGetReply(mdmLibCtx.msgHandle, msg);
   }
   else
   {
      cmsLog_debug("calling send for msg 0x%x to 0x%x", msgType, dst);
      ret = cmsMsg_send(mdmLibCtx.msgHandle, msg);
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(msgBuf);

   cmsLog_debug("Exit, ret=%d", ret);
   return ret;
}



void rut_sendReloadMsgToDhcpd(void)
{
   CmsMsgHeader msg=EMPTY_MSG_HEADER;
   CmsRet ret;

   if (qdmIpIntf_isAllBridgeWanServiceLocked())
   {
      cmsLog_debug("All bridge configuration and no need to send msg to dhcpd");
      return;
   }

   if (rutLan_isDhcpdEnabled() == FALSE)
   {
      cmsLog_debug("dhcpd is disabled and dhcpd will not be launched");
      return;
   }


   msg.type = CMS_MSG_DHCPD_RELOAD;
   msg.src = mdmLibCtx.eid;
   msg.dst = EID_DHCPD;
   msg.flags_event = 1;

   /*  NOTE: if dhcpd is not in the system, it will be launched to receive this message */
   if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send DHCPD_RELOAD msg to dhcpd, ret=%d", ret);
   }
   else
   {
      cmsLog_debug("CMS_MSG_DHCPD_RELOAD sent successfully");
   }

   return;
}


UBOOL8 rut_getIfAddr(const char *devname, char* ip)
{
   SINT32 skfd;
   struct ifreq intf;
   char *ptr;
   UBOOL8 ret = FALSE;

   if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      return ret;
   }

   strcpy(intf.ifr_name, devname);

   if (ioctl(skfd, SIOCGIFADDR, &intf) != -1)
   {
      if ((ptr = inet_ntoa(((struct sockaddr_in *)(&(intf.ifr_dstaddr)))->sin_addr)) != NULL)
      {
         strcpy(ip, ptr);
         ret = TRUE;
      }
   }
   close(skfd);

   return ret;
}


UBOOL8 rut_getIfMask(const char *devname, char* ip)
{
   SINT32  skfd;
   UBOOL8  ret = FALSE;
   struct ifreq intf;
   char *ptr;

   if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      return ret;
   }

   strcpy(intf.ifr_name, devname);

   if (ioctl(skfd, SIOCGIFNETMASK, &intf) != -1)
   {
      if ((ptr = inet_ntoa(((struct sockaddr_in *)(&(intf.ifr_netmask)))->sin_addr)) != NULL)
      {
         strcpy(ip, ptr);
         ret = TRUE;
      }
   }
   close(skfd);

   return ret;
}

UBOOL8 rut_getIfSubnet(const char *devname, char* ipSubnet)
{
   char *ptr;
   char ipaddr[BUFLEN_32], netmask[BUFLEN_32];
   struct in_addr ip, mask, subnet;
   UBOOL8 ret = FALSE;

   memset(&ip, 0, sizeof(ip));
   memset(&mask, 0, sizeof(mask));
   memset(&subnet, 0, sizeof(subnet));

   if (rut_getIfAddr(devname, ipaddr) && rut_getIfMask(devname, netmask))
   {
      if (inet_aton(ipaddr, &ip) && inet_aton(netmask, &mask))
      {
         subnet.s_addr = ip.s_addr & mask.s_addr;
         if ((ptr = inet_ntoa(subnet)) != NULL)
         {
            strcpy(ipSubnet, ptr);
            ret = TRUE;
         }
      }
   }

   return ret;
}


UBOOL8 rut_getIfDestAddr(char *devname, char* ip)
{
   SINT32 skfd;
   UBOOL8 ret = FALSE;
   struct ifreq intf;
   char *ptr;

   if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
     return ret;
   }

   strcpy(intf.ifr_name, devname);

   if (ioctl(skfd, SIOCGIFDSTADDR, &intf) != -1)
   {
      if ((ptr = inet_ntoa(((struct sockaddr_in *)(&(intf.ifr_dstaddr)))->sin_addr)) != NULL)
      {
         strcpy(ip,ptr);
         ret = TRUE;
      }
   }
   close(skfd);

   return ret;
}


CmsRet rut_getIfStatusHwaddr(const char *devname, char *statusStr, char *hwAddr)
{
   int socketFd = 0;
   struct ifreq ifr;
   CmsRet ret = CMSRET_SUCCESS;

   if (devname == NULL)
   {
      cmsLog_error("devname is NULL");
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
      {
         cmsLog_error("Cannot create socket to the NET kernel");
         ret = CMSRET_INTERNAL_ERROR;
      }
      else
      {
         cmsUtl_strncpy(ifr.ifr_name, devname, IFNAMSIZ);
         /* Get the flags from kernel NET */
         /* IFF_UP flag */
         if (ioctl(socketFd, SIOCGIFFLAGS, &ifr) != -1)
         {
            if (ifr.ifr_flags & IFF_UP)
            {
               sprintf(statusStr,MDMVS_UP);
            }
         }

         /* IFHWADDR flag */
         if (ioctl(socketFd, SIOCGIFHWADDR, &ifr) != -1)
         {
            cmsUtl_macNumToStr((UINT8 *) ifr.ifr_hwaddr.sa_data, hwAddr);
         }

         cmsLog_debug("Devname=%s, statusStr = %s, hwAddr=%s", devname, statusStr, hwAddr);
         close(socketFd);
      }
   }

   return ret;
}


#if defined(DMP_BASELINE_1)
/*
 * This function is doing a search in the TR98 style data model, so
 * this is used only in Legacy TR98 and Hybrid TR98+TR181.
 */
UBOOL8 rut_isAnyNatEnabled_igd(void)
{
   UBOOL8 enabled = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanPppConnObject *pppConn = NULL;
   _WanIpConnObject *ipConn = NULL;

   while ((enabled == FALSE) &&
          (cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppConn) == CMSRET_SUCCESS))
   {
      enabled = pppConn->NATEnabled;
      cmsObj_free((void **) &pppConn);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((enabled == FALSE) &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn) == CMSRET_SUCCESS))
   {
      enabled = ipConn->NATEnabled;
      cmsObj_free((void **) &ipConn);
   }

   return enabled;

}

#endif  /* DMP_BASELINE_1 */



void rut_getDefaultGatewayInterfaceName(char *ifcName)
{
   char col[11][BUFLEN_32];
   char line[BUFLEN_128];
   int count = 0;
   FILE* fsRoute = NULL;

   if (ifcName == NULL)
   {
      return;
   }

   ifcName[0] = '\0';

   if ((fsRoute = fopen("/proc/net/route", "r")) == NULL)
   {
      cmsLog_error("No route table found?");
      return;
   }

   while (fgets(line, sizeof(line), fsRoute))
   {
      /* read pass header line */
      if (count++ < 1)
      {
         continue;
      }
      sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s",
             col[0], col[1], col[2], col[3], col[4], col[5],
             col[6], col[7], col[8], col[9], col[10]);
      /* if destination && mask are 0 then it's default gateway */
      if (cmsUtl_strcmp(col[1], "00000000") == 0 && cmsUtl_strcmp(col[7], "00000000") == 0)
      {
         strcpy(ifcName, col[0]);
         break;
      }
   }

   fclose(fsRoute);

   cmsLog_debug("Default gateway=%s", ifcName);

}



#if defined(DMP_BASELINE_1)
/*
 * This function is doing a search in the TR98 style data model, so
 * this is used only in Legacy TR98 and Hybrid TR98+TR181.
 */

UBOOL8 rut_isWanInterfaceNatEnable_igd(const char *ifcName)
{
   UBOOL8 enable = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanPppConnObject *pppConn = NULL;
   WanIpConnObject *ipConn = NULL;

   cmsLog_debug("Enter, ifcName=%s", ifcName);

   while ((enable == FALSE) &&
          (cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &pppConn) == CMSRET_SUCCESS))
   {
      if (pppConn->NATEnabled && cmsUtl_strcmp(ifcName, pppConn->X_BROADCOM_COM_IfName) == 0)
      {
         enable = TRUE;
      }
      cmsObj_free((void **) &pppConn);
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((enable == FALSE) &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn) == CMSRET_SUCCESS))
   {
      if (ipConn->NATEnabled && cmsUtl_strcmp(ifcName, ipConn->X_BROADCOM_COM_IfName) == 0)
      {
         enable = TRUE;
      }
      cmsObj_free((void **) &ipConn);
   }

   return enable;

}


UBOOL8 rut_isWanInterfaceBridged(const char *ifcName)
{
   UBOOL8 isBridged = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanIpConnObject *ipConn = NULL;

   cmsLog_debug("Enter, ifcName=%s", ifcName);

#if 0
   if (strstr(ifcName, ATM_IFC_STR) == NULL &&
       strstr(ifcName, PTM_IFC_STR) == NULL)
   {
      return FALSE;
   }
#endif

   while ((isBridged == FALSE) &&
          (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipConn) == CMSRET_SUCCESS))
   {
      if (cmsUtl_strcmp(ipConn->X_BROADCOM_COM_IfName, ifcName)   == 0 &&
          cmsUtl_strcmp(ipConn->connectionType, MDMVS_IP_BRIDGED) == 0)
      {
         isBridged = TRUE;
      }
      cmsObj_free((void **) &ipConn);
   }

   return isBridged;

}

#endif  /* DMP_BASELINE_1 */


UBOOL8 rut_isApplicationRunning(CmsEntityId eid)
{
   UBOOL8 isAppRunning = FALSE;

   cmsLog_debug("Enter");
   if (rut_sendMsgToSmd(CMS_MSG_IS_APP_RUNNING, (UINT32) eid, NULL, 0) == CMSRET_SUCCESS)
   {
      isAppRunning = TRUE;
   }

   cmsLog_debug("App. with eid 0X%0X status = %d", eid, isAppRunning);

   return isAppRunning;

}


UBOOL8 rut_isApplicationActive(CmsEntityId eid)
{
   UBOOL8 isAppActive = FALSE;

   cmsLog_debug("Enter");
   if (rut_sendMsgToSmd(CMS_MSG_IS_APP_ACTIVE, (UINT32) eid, NULL, 0) == CMSRET_SUCCESS)
   {
      isAppActive = TRUE;
   }

   cmsLog_debug("App. with eid 0X%0X status = %d", eid, isAppActive);

   return isAppActive;

}


void rut_activateDmzRule(void)
{
#ifdef DMP_BASELINE_1
   /* this implementation is currently a TR98 feature only */
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   SecDmzHostCfgObject *dmzCfg = NULL;

   if (cmsObj_get(MDMOID_SEC_DMZ_HOST_CFG, &iidStack, 0, (void **) &dmzCfg) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get SecDmzHostCfgObject");
      return;
   }

   cmsObj_set(dmzCfg, &iidStack);
   cmsObj_free((void **) &dmzCfg);
#endif /* DMP_BASELINE_1 */

   return;
}



#if defined(DMP_BASELINE_1)
/*
 * This function is doing a search in the TR98 style data model, so
 * this is used only in Legacy TR98 and Hybrid TR98+TR181.
 */
CmsRet rut_intfnameToFullPath(const char *intfname, UBOOL8 layer2, char **mdmPath)
{
   InstanceIdStack iidStack;
   MdmObjectId oid;
   void *mdmObj = NULL;
   CmsRet ret;

#ifdef BRCM_WLAN
   int wl_index=0;
   char wlifname[8] = { 0 };
   if(sscanf(intfname,"wds%d",&wl_index) == 1) 
       snprintf(wlifname,8,"wl%d",wl_index);
#endif

   if (IS_EMPTY_STRING(intfname))
   {
      cmsLog_error("invalid argument. intfname=0x%x", (uintptr_t)intfname);
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_debug("Looking for %s layer2=%d", intfname, layer2);

   /*
    * First look for the interface on the LAN side.
    */
   if (strstr(intfname, ETH_IFC_STR) != NULL)
   {
      oid = MDMOID_LAN_ETH_INTF;
   }
   else if (!cmsUtl_strncmp(intfname, BRIDGE_IFC_STR, strlen(BRIDGE_IFC_STR)))
   {
      oid = MDMOID_LAN_IP_INTF;
   }
#ifdef BRCM_WLAN
   else if (wlifname[0] || strstr(intfname, WLAN_IFC_STR) != NULL)
   {
      oid = MDMOID_WL_VIRT_INTF_CFG;
   }
#endif
#ifdef SUPPORT_MOCA
   else if (strstr(intfname, MOCA_IFC_STR) != NULL)
   {
      oid = MDMOID_LAN_MOCA_INTF;
   }
#endif
#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
   else if (strstr(intfname, GPON_IFC_STR) != NULL && !rut_isWanTypeEpon())
   {
      oid = MDMOID_WAN_GPON_LINK_CFG;
   }
#endif
#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
   else if (strstr(intfname, EPON_IFC_STR) != NULL && rut_isWanTypeEpon())
   {
#ifdef EPON_SFU
      oid = MDMOID_WAN_EPON_INTF;
#else
      oid = MDMOID_WAN_EPON_LINK_CFG;
#endif
   }
#ifdef BRCM_PKTCBL_SUPPORT
   else if (strstr(intfname, EPON_VOICE_IFC_STR) != NULL && rut_isWanTypeEpon())
   {
      if (layer2)
      {
         oid = MDMOID_WAN_EPON_LINK_CFG;
      }
      else
      {
         oid = MDMOID_WAN_IP_CONN;
      }
   }
#endif // BRCM_PKTCBL_SUPPORT
#endif
   else if (strstr(intfname, USB_IFC_STR) != NULL)
   {
      oid = MDMOID_LAN_USB_INTF;
   }
   else if (strstr(intfname, ATM_IFC_STR)  != NULL ||
            strstr(intfname, IPOA_IFC_STR) != NULL)
   {
      oid = MDMOID_WAN_IP_CONN;
#ifdef DMP_X_BROADCOM_COM_ADSLWAN_1
      if (layer2)
      {
         oid = MDMOID_WAN_DSL_LINK_CFG;
      }
#endif
   }
   else if (strstr(intfname, PTM_IFC_STR) != NULL)
   {
      oid = MDMOID_WAN_IP_CONN;
#ifdef DMP_X_BROADCOM_COM_PTMWAN_1
      if (layer2)
      {
         oid = MDMOID_WAN_PTM_LINK_CFG;
      }
#endif
   }
   else if (strstr(intfname, PPP_IFC_STR) != NULL)
   {
      oid = MDMOID_WAN_PPP_CONN;
   }
   else
   {
      cmsLog_error("invalid intfname: %s", intfname);
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(&iidStack);
   while ((ret = cmsObj_getNextFlags(oid, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &mdmObj)) == CMSRET_SUCCESS)
   {
      char *mdmIfName;

      switch (oid)
      {
      case MDMOID_LAN_ETH_INTF:
         mdmIfName = ((LanEthIntfObject *)mdmObj)->X_BROADCOM_COM_IfName;
         break;
      case MDMOID_LAN_IP_INTF:
         mdmIfName = ((LanIpIntfObject *)mdmObj)->X_BROADCOM_COM_IfName;
         break;
#ifdef BRCM_WLAN
      case MDMOID_WL_VIRT_INTF_CFG:
         mdmIfName = ((WlVirtIntfCfgObject *)mdmObj)->wlIfcname;
         break;
#endif
#ifdef SUPPORT_MOCA
      case MDMOID_LAN_MOCA_INTF:
         mdmIfName = ((LanMocaIntfObject *)mdmObj)->ifName;
         break;
#endif
      case MDMOID_LAN_USB_INTF:
         mdmIfName = ((LanUsbIntfObject *)mdmObj)->X_BROADCOM_COM_IfName;
         break;
#ifdef DMP_X_BROADCOM_COM_ADSLWAN_1
      case MDMOID_WAN_DSL_LINK_CFG:
         mdmIfName = ((WanDslLinkCfgObject *)mdmObj)->X_BROADCOM_COM_IfName;
         break;
#endif
#ifdef DMP_X_BROADCOM_COM_PTMWAN_1
      case MDMOID_WAN_PTM_LINK_CFG:
         mdmIfName = ((WanPtmLinkCfgObject *)mdmObj)->X_BROADCOM_COM_IfName;
         break;
#endif
      case MDMOID_WAN_IP_CONN:
         mdmIfName = ((WanIpConnObject *)mdmObj)->X_BROADCOM_COM_IfName;
         break;
#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
      case MDMOID_WAN_GPON_LINK_CFG:
         mdmIfName = ((WanGponLinkCfgObject*)mdmObj)->ifName;
         break;
#endif
#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
      case MDMOID_WAN_EPON_INTF:
         mdmIfName = ((WanEponIntfObject*)mdmObj)->ifName;
         break;
      case MDMOID_WAN_EPON_LINK_CFG:
         mdmIfName = ((WanEponLinkCfgObject*)mdmObj)->ifName;
         break;
#endif

      default:
         mdmIfName = ((WanPppConnObject *)mdmObj)->X_BROADCOM_COM_IfName;
         break;
      }


#ifdef BRCM_WLAN
      if (cmsUtl_strcmp(mdmIfName, wlifname[0]?wlifname:intfname) == 0)
#else
      if (cmsUtl_strcmp(mdmIfName, intfname) == 0)
#endif
      {
         MdmPathDescriptor pathDesc;

         pathDesc.oid = oid;
         pathDesc.iidStack = iidStack;
         pathDesc.paramName[0] = '\0';

         if ((ret = cmsMdm_pathDescriptorToFullPath(&pathDesc, mdmPath)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
         }
         cmsObj_free(&mdmObj);
         return ret;
      }
      cmsObj_free(&mdmObj);
   }


   if (ret != CMSRET_NO_MORE_INSTANCES && ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_getNext returns error. ret=%d", ret);
   }
   else if (ret == CMSRET_NO_MORE_INSTANCES)
   {
#if defined (DMP_ETHERNETWAN_1) || (defined (SUPPORT_MOCA) && defined (DMP_X_BROADCOM_COM_MOCAWAN_1)) || \
   defined (DMP_X_BROADCOM_COM_GPONWAN_1) || defined (DMP_X_BROADCOM_COM_EPONWAN_1)
      /* Now look for interface on the WAN side */
      /* intfname could be a wanEthernet or wanMoca interface */
      oid = 0; /* invalid oid */
      if (strstr(intfname, ETH_IFC_STR) != NULL)
      {
         if (layer2)
         {
#ifdef DMP_ETHERNETWAN_1
            oid = MDMOID_WAN_ETH_INTF;
#endif
         }
         else
         {
            oid = MDMOID_WAN_IP_CONN;
         }
      }
      else if (strstr(intfname, MOCA_IFC_STR) != NULL)
      {
         if (layer2)
         {
            oid = MDMOID_WAN_MOCA_INTF;
         }
         else
         {
            oid = MDMOID_WAN_IP_CONN;
         }
      }
      else if (strstr(intfname, GPON_IFC_STR) != NULL && !rut_isWanTypeEpon())
      {
         if (layer2)
         {
            oid = MDMOID_WAN_GPON_LINK_CFG;
         }
         else
         {
            oid = MDMOID_WAN_IP_CONN;
         }
      }
      else if (strstr(intfname, EPON_IFC_STR) != NULL && rut_isWanTypeEpon())
      {
         if (layer2)
         {
#ifdef EPON_SFU
            oid = MDMOID_WAN_EPON_INTF;
#else
            oid = MDMOID_WAN_EPON_LINK_CFG;
#endif
         }
         else
         {
            oid = MDMOID_WAN_IP_CONN;
         }
      }
#ifdef BRCM_PKTCBL_SUPPORT
      else if (strstr(intfname, EPON_VOICE_IFC_STR) != NULL && rut_isWanTypeEpon())
      {
         if (layer2)
         {
            oid = MDMOID_WAN_EPON_LINK_CFG;
         }
         else
         {
            oid = MDMOID_WAN_IP_CONN;
         }
      }
#endif // BRCM_PKTCBL_SUPPORT

      if (oid != 0)
      {
         INIT_INSTANCE_ID_STACK(&iidStack);
         while ((ret = cmsObj_getNextFlags(oid, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &mdmObj)) == CMSRET_SUCCESS)
         {
            char *mdmIfName;

            switch (oid)
            {
#ifdef DMP_ETHERNETWAN_1
            case MDMOID_WAN_ETH_INTF:
               mdmIfName = ((WanEthIntfObject *)mdmObj)->X_BROADCOM_COM_IfName;
               break;
#endif
#if defined (SUPPORT_MOCA) && defined (DMP_X_BROADCOM_COM_MOCAWAN_1)
            case MDMOID_WAN_MOCA_INTF:
               mdmIfName = ((WanMocaIntfObject *)mdmObj)->ifName;
               break;
#endif
#if defined (DMP_X_BROADCOM_COM_GPONWAN_1)
            case MDMOID_WAN_GPON_LINK_CFG:
               mdmIfName = ((WanGponLinkCfgObject *)mdmObj)->ifName;
               break;
#endif
#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
            case MDMOID_WAN_EPON_INTF:
               mdmIfName = ((WanEponIntfObject*)mdmObj)->ifName;
               break;
            case MDMOID_WAN_EPON_LINK_CFG:
               mdmIfName = ((WanEponLinkCfgObject*)mdmObj)->ifName;
               break;
#endif

            default:
               mdmIfName = ((WanIpConnObject *)mdmObj)->X_BROADCOM_COM_IfName;
               break;
            }

            if (cmsUtl_strcmp(mdmIfName, intfname) == 0)
            {
               MdmPathDescriptor pathDesc;

               pathDesc.oid = oid;
               pathDesc.iidStack = iidStack;
               pathDesc.paramName[0] = '\0';

               if ((ret = cmsMdm_pathDescriptorToFullPath(&pathDesc, mdmPath)) != CMSRET_SUCCESS)
               {
                  cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
               }
               cmsObj_free(&mdmObj);

               return ret;
            }
            cmsObj_free(&mdmObj);
         }
#if defined (DMP_X_BROADCOM_COM_GPONWAN_1) || defined (DMP_X_BROADCOM_COM_EPONWAN_1)
         if ((oid == MDMOID_WAN_IP_CONN) && (ret == CMSRET_NO_MORE_INSTANCES))
         {
            char *mdmIfName;
            oid = MDMOID_WAN_PPP_CONN;
            INIT_INSTANCE_ID_STACK(&iidStack);
            while ((ret = cmsObj_getNextFlags(oid, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&mdmObj)) == CMSRET_SUCCESS)
            {
                mdmIfName = ((WanPppConnObject *)mdmObj)->X_BROADCOM_COM_IfName;
                if (cmsUtl_strcmp(mdmIfName, intfname) == 0)
                {
                   MdmPathDescriptor pathDesc;

                   pathDesc.oid = oid;
                   pathDesc.iidStack = iidStack;
                   pathDesc.paramName[0] = '\0';

                   if ((ret = cmsMdm_pathDescriptorToFullPath(&pathDesc, mdmPath)) != CMSRET_SUCCESS)
                   {
                      cmsLog_error("cmsMdm_pathDescriptorToFullPath returns error. ret=%d", ret);
                   }
                   cmsObj_free(&mdmObj);

                   return ret;
                }
                cmsObj_free(&mdmObj);
            }
         }
#endif
         if (ret != CMSRET_NO_MORE_INSTANCES && ret != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_getNext returns error. ret=%d", ret);
         }
         else if (ret == CMSRET_NO_MORE_INSTANCES)
         {
            cmsLog_error("intfname %s does not exist", intfname);
         }
      }
      else
#endif
         cmsLog_debug("Could not find %s layer2=%d", intfname, layer2);
   }  /* #ifdef ETHWAN, MOCAWAN, GPONWAN, or EPONWAN */

   return ret;

}  /* End of rut_intfnameToFullPath() */

CmsRet rut_fullPathToIntfname(const char *mdmPath, char *intfname)
{
   MdmPathDescriptor pathDesc;
   void *mdmObj = NULL;
   CmsRet ret;

   if (IS_EMPTY_STRING(mdmPath) || intfname == NULL)
   {
      cmsLog_error("invalid argument. mdmPath=0x%x intfname=0x%x", (uintptr_t)mdmPath, (uintptr_t)intfname);
      return CMSRET_INVALID_ARGUMENTS;
   }

   intfname[0] = '\0';

   if (mdmPath[strlen(mdmPath)-1] == '.')
   {
      ret = cmsMdm_fullPathToPathDescriptor(mdmPath, &pathDesc);
   }
   else
   {
      char intfFullPath[BUFLEN_512];

      /* add a dot at the end to indicate that the path is an object path */
      snprintf(intfFullPath, sizeof(intfFullPath), "%s.", mdmPath);
      ret = cmsMdm_fullPathToPathDescriptor(intfFullPath, &pathDesc);
   }
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor returns error. ret=%d", ret);
      return ret;
   }

   /* get the interface object */
   if ((ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, OGF_NO_VALUE_UPDATE, &mdmObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsObj_get returns error. ret=%d", ret);
      return ret;
   }

   switch (pathDesc.oid)
   {
   case MDMOID_LAN_ETH_INTF:
      strcpy(intfname, ((LanEthIntfObject *)mdmObj)->X_BROADCOM_COM_IfName);
      break;
   case MDMOID_LAN_IP_INTF:
	  strcpy(intfname, ((_LanIpIntfObject *)mdmObj)->X_BROADCOM_COM_IfName);
	  break;
#ifdef BRCM_WLAN
   case MDMOID_WL_VIRT_INTF_CFG:
      strcpy(intfname, ((WlVirtIntfCfgObject *)mdmObj)->wlIfcname);
      break;
#endif
   case MDMOID_LAN_USB_INTF:
      strcpy(intfname, ((LanUsbIntfObject *)mdmObj)->X_BROADCOM_COM_IfName);
      break;
#ifdef SUPPORT_MOCA
#ifdef DMP_X_BROADCOM_COM_MOCALAN_1
   case MDMOID_LAN_MOCA_INTF:
      strcpy(intfname, ((LanMocaIntfObject *)mdmObj)->ifName);
      break;
#endif
#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1
   case MDMOID_WAN_MOCA_INTF:
      strcpy(intfname, ((WanMocaIntfObject *)mdmObj)->ifName);
      break;
#endif
#endif

#ifdef DMP_X_BROADCOM_COM_GPONWAN_1
   case MDMOID_WAN_GPON_LINK_CFG:
      strcpy(intfname, ((WanGponLinkCfgObject *)mdmObj)->ifName);
      break;
#endif

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1
   case MDMOID_WAN_EPON_INTF:
      strcpy(intfname, ((WanEponIntfObject *)mdmObj)->ifName);
      break;
   case MDMOID_WAN_EPON_LINK_CFG:
      strcpy(intfname, ((WanEponLinkCfgObject *)mdmObj)->ifName);
      break;
#endif

#ifdef DMP_ETHERNETWAN_1
   case MDMOID_WAN_ETH_INTF:
      strcpy(intfname, ((WanEthIntfObject *)mdmObj)->X_BROADCOM_COM_IfName);
      break;
#endif
#ifdef DMP_X_BROADCOM_COM_ADSLWAN_1
   case MDMOID_WAN_DSL_LINK_CFG:
      strcpy(intfname, ((WanDslLinkCfgObject *)mdmObj)->X_BROADCOM_COM_IfName);
      break;
#endif
#ifdef DMP_X_BROADCOM_COM_PTMWAN_1
   case MDMOID_WAN_PTM_LINK_CFG:
      strcpy(intfname, ((WanPtmLinkCfgObject *)mdmObj)->X_BROADCOM_COM_IfName);
      break;
#endif
   case MDMOID_WAN_IP_CONN:
      strcpy(intfname, ((WanIpConnObject *)mdmObj)->X_BROADCOM_COM_IfName);
      break;
   case MDMOID_WAN_PPP_CONN:
      strcpy(intfname, ((WanPppConnObject *)mdmObj)->X_BROADCOM_COM_IfName);
      break;
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
   case MDMOID_DEV2_IP_INTERFACE:
      strcpy(intfname, ((Dev2IpInterfaceObject *)mdmObj)->name);
      break;
#endif
#ifdef DMP_WIFILAN_1
    case MDMOID_LAN_WLAN:
      strcpy(intfname, ((LanWlanObject *)mdmObj)->X_BROADCOM_COM_IfName);
      break;
#endif
   default:
      cmsLog_error("Invalid interface: %s", mdmPath);
      return CMSRET_INVALID_ARGUMENTS;
   }
   cmsObj_free(&mdmObj);

   return ret;

}  /* End of rut_fullPathToIntfname() */

/* these applies to TR98 and TR181 */
void rutUtil_modifyNumXmppConnServer_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_DEV2_XMPP_CONN, MDMOID_DEV2_XMPP_CONN_SERVER, iidStack, delta);
}

void rutUtil_modifyNumXmppConn_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_DEV2_XMPP, MDMOID_DEV2_XMPP_CONN, iidStack, delta);
}

#ifdef DMP_X_BROADCOM_COM_CONTAINER_1
void rutUtil_modifyNumContainerEntry_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_CONTAINER, MDMOID_CONTAINER_INFO, iidStack, delta);
}
#endif  /* DMP_X_BROADCOM_COM_CONTAINER_1 */

#ifdef DMP_DEVICE2_SM_BASELINE_1
void rutUtil_modifyNumExecEnvEntry_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_SW_MODULES, MDMOID_EXEC_ENV, iidStack, delta);
}

void rutUtil_modifyNumDeploymentUnitEntry_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_SW_MODULES, MDMOID_DU, iidStack, delta);
}

void rutUtil_modifyNumExecutionUnitEntry_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_SW_MODULES, MDMOID_EU, iidStack, delta);
}

void rutUtil_modifyNumBusObjectPathEntry_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_BUS, MDMOID_BUS_OBJECT_PATH, iidStack, delta);
}

void rutUtil_modifyNumBusInterfaceEntry_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_BUS_OBJECT_PATH, MDMOID_BUS_INTERFACE, iidStack, delta);
}

void rutUtil_modifyNumBusMethodEntry_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_BUS_INTERFACE, MDMOID_BUS_METHOD, iidStack, delta);
}

void rutUtil_modifyNumBusSignalEntry_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_BUS_INTERFACE, MDMOID_BUS_SIGNAL, iidStack, delta);
}

void rutUtil_modifyNumBusPropertyEntry_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_BUS_INTERFACE, MDMOID_BUS_PROPERTY, iidStack, delta);
}

void rutUtil_modifyNumBusClientPrivilegeEntry_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_BUS_CLIENT, MDMOID_BUS_CLIENT_PRIVILEGE, iidStack, delta);
}


#ifdef DMP_DEVICE2_ETHLAG_1
void rutUtil_modifyNumEthLag_igd(const InstanceIdStack *iidStack, SINT32 delta)
{
   rut_modifyNumGeneric(MDMOID_DEV2_ETHERNET,  MDMOID_LA_G, iidStack, delta);
}
#endif /* DMP_DEVICE2_ETHLAG_1 */


#endif  /* DMP_DEVICE2_SM_BASELINE_1 */
#endif  /* DMP_BASELINE_1 */



CmsRet rut_setIfState(char *ifName, UBOOL8 up)
{
    int sockfd = 0;
    struct ifreq ifr;
    CmsRet ret = CMSRET_SUCCESS;

    if (ifName == NULL)
    {
        cmsLog_error("Cannot bring up NULL interface");
        ret = CMSRET_INTERNAL_ERROR;
    }
    else
    {
        /* Create a channel to the NET kernel. */
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            cmsLog_error("Cannot create socket to the NET kernel");
            ret = CMSRET_INTERNAL_ERROR;
        }
        else
        {
            strncpy(ifr.ifr_name, ifName, IFNAMSIZ);
            // get interface flags
            if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) != -1)
            {
                if (up)
                    ifr.ifr_flags |= IFF_UP;
                else
                    ifr.ifr_flags &= (~IFF_UP);

                if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0)
                {
                    cmsLog_error("Cannot ioctl SIOCSIFFLAGS on the socket");
                    ret = CMSRET_INTERNAL_ERROR;
                }
            }
            else
            {
                cmsLog_error("Cannot ioctl SIOCGIFFLAGS on the socket");
                ret = CMSRET_INTERNAL_ERROR;
            }

            close(sockfd);
        }
    }

    return ret;
}




CmsRet rut_isGponIpHostInterface(char *ifname __attribute__((unused)),
                                 UINT32 *meId __attribute__((unused)))
{
#if defined(OMCI_TR69_DUAL_STACK)
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   BcmOmciRtdIpHostConfigDataExtObject *bcmIpHost = NULL;
   UBOOL8 found = FALSE;
   CmsRet ret;

   // Nested locks are allowed, but there must be an equal number of unlocks.
   ret = cmsLck_acquireLockWithTimeout(6*MSECS_IN_SEC);
   if (ret != CMSRET_SUCCESS)
      return ret;

   ret = CMSRET_OBJECT_NOT_FOUND;

#ifdef DMP_X_BROADCOM_COM_IPV6_1
   BcmOmciRtdIpv6HostConfigDataExtObject *bcmIpv6Host = NULL;
   while(!found &&
         cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IPV6_HOST_CONFIG_DATA_EXT,
                               &iidStack, (void **) &bcmIpv6Host) == CMSRET_SUCCESS)
   {
       if (cmsUtl_strcmp(bcmIpv6Host->interfaceName, ifname) == 0)
       {
           found = TRUE;
           ret = CMSRET_SUCCESS;
           if (meId)
               *meId = bcmIpv6Host->managedEntityId;
       }
       cmsObj_free((void **) &bcmIpv6Host);
   }

    INIT_INSTANCE_ID_STACK(&iidStack);
#endif    // DMP_X_BROADCOM_COM_IPV6_1

   while(!found &&
         cmsObj_getNext(MDMOID_BCM_OMCI_RTD_IP_HOST_CONFIG_DATA_EXT,
                               &iidStack, (void **) &bcmIpHost) == CMSRET_SUCCESS)
   {
       if (cmsUtl_strcmp(bcmIpHost->interfaceName, ifname) == 0)
       {
           found = TRUE;
           ret = CMSRET_SUCCESS;
           if (meId)
               *meId = bcmIpHost->managedEntityId;
       }
       cmsObj_free((void **) &bcmIpHost);
   }

   cmsLck_releaseLock();

   return ret;
#else
   return CMSRET_OBJECT_NOT_FOUND;
#endif
}

UBOOL8 rut_isWanTypeEpon(void)
{
#ifndef DESKTOP_LINUX
#ifdef SUPPORT_RDPA
    rdpa_wan_type wan_type = rdpa_wan_none;
    rdpa_if wan_if = rdpa_wan_type_to_if(rdpa_wan_epon); /* OK to assume same wan_if for epon/xepon */
    int ret;
    BOOL enable = TRUE;

    ret = rdpaCtl_get_wan_type(wan_if, &wan_type);
    if (ret)
    {
        return FALSE;
    }

    if((wan_type == rdpa_wan_epon) ||(wan_type == rdpa_wan_xepon))
    {
        ret = rdpaCtl_get_epon_status(&enable);
        if(ret || (enable == FALSE))
        {
            return FALSE;
        }
        else
            return TRUE;
    }
#endif /* SUPPORT_RDPA */
#endif /* DESKTOP_LINUX */
    return FALSE;
}

#ifndef DESKTOP_LINUX
UINT32 rut_getRdpaWanType(int wan_if)
{
#ifdef SUPPORT_RDPA
    rdpa_wan_type wan_type = rdpa_wan_none;
    int ret;

    ret = rdpaCtl_get_wan_type(wan_if, &wan_type);

    if (ret)
    {
        return rdpa_wan_none;
    }
    else
    {
        return wan_type;
    }
#endif /* SUPPORT_RDPA */
    return 0;
}
#else
UINT32 rut_getRdpaWanType(int wan_if __attribute__((unused)))
{
    return 0;
}
#endif /* DESKTOP_LINUX */

UINT32 rut_getTr69cConnReqPort (CmsExtendEntityIndex eeId)
{
   if (CMS_EE_INDEX_2 == eeId)
   {
      return TR69C_2_CONN_REQ_PORT;
   }
   else /* default */
   {
      return TR69C_CONN_REQ_PORT;
   }
}

/*
 * send a ACS_CONFIG_CHANGED event msg to smd with source acsConfigId,
 * then forwarded to registered process.
 */
CmsRet rut_sendAcsConfigChangedMsgToSmd(const char *acsConfigId)
{
   CmsMsgHeader *msgHdr;
   char *dataPtr = NULL;
   UINT32 dataLen = 0;

   if (acsConfigId != NULL)
   {
      dataLen = strlen(acsConfigId) + 1;
      cmsLog_debug("acsConfigId=%s", acsConfigId);
   }
   
   msgHdr = (CmsMsgHeader *) cmsMem_alloc(sizeof(CmsMsgHeader) + dataLen, ALLOC_ZEROIZE);
   if (msgHdr == NULL)
   {
      cmsLog_error("message header allocation failed, len of acsConfigId=%d", strlen(acsConfigId));
      return CMSRET_INTERNAL_ERROR;
   }

   msgHdr->type = CMS_MSG_ACS_CONFIG_CHANGED;
   msgHdr->src = mdmLibCtx.eid;
   msgHdr->dst = EID_SMD;
   msgHdr->flags_event = 1;
   msgHdr->dataLength = dataLen;
   if (acsConfigId != NULL)
   {
      dataPtr = (char *) (msgHdr+1);
      strncpy(dataPtr, acsConfigId, strlen(acsConfigId));
   }

   if (cmsMsg_send(mdmLibCtx.msgHandle, msgHdr) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send out ACS_CONFIG_CHANGED event msg");
   }
   else
   {
      cmsLog_debug("Send out ACS_CONFIG_CHANGED event msg.");
   }

   cmsMem_free(msgHdr);

   return CMSRET_SUCCESS;
}
