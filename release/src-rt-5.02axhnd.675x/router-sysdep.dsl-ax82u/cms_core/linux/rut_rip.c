/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
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

#include "cms_core.h"
#include "cms_util.h"
#include "rut_rip.h"
#include "rut_util.h"
#include "rut_iptables.h"

#ifdef SUPPORT_RIP


#ifdef DMP_BASELINE_1

UBOOL8 rutRip_isConfigChanged_igd(const _WanIpConnObject *newObj,
                                  const _WanIpConnObject *currObj)
{
   UBOOL8 change = FALSE;

   if (newObj == NULL || currObj == NULL)
   {
      cmsLog_debug("One of object is NULL. return no change.");
   }
   else
   {
      if (cmsUtl_strcmp(newObj->routeProtocolRx, currObj->routeProtocolRx) ||
          cmsUtl_strcmp(newObj->X_BROADCOM_COM_RipOperationMode, currObj->X_BROADCOM_COM_RipOperationMode) ||
          cmsUtl_strcmp(newObj->X_BROADCOM_COM_RipDebugMode, currObj->X_BROADCOM_COM_RipDebugMode) ||
          newObj->NATEnabled != currObj->NATEnabled)
      {
         change = TRUE;
      }
   }

   cmsLog_debug("rip change=%d", change);
   
   return change;
}


UBOOL8 rutRip_isConfigValid_igd(const _WanIpConnObject *newObj)
{
   UBOOL8 valid = TRUE;

   /* only bother to check validity if RIP is enabled */
   if (cmsUtl_strcmp(newObj->routeProtocolRx, MDMVS_OFF))
   {
      if (newObj->NATEnabled &&
          !cmsUtl_strcmp(newObj->X_BROADCOM_COM_RipOperationMode, MDMVS_ACTIVE))
      {
         cmsLog_error("NAT and RIP ACTIVE mode are not compatible!");
         valid = FALSE;
      }
   }

   cmsLog_debug("valid=%d", valid);

   return valid;
}


UINT32 rutRip_getNumberOfRipInterfaces_igd()
{
   UINT32 num=0;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanIpConnObject *ipConnObj = NULL;

   while (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &ipConnObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(ipConnObj->routeProtocolRx, MDMVS_OFF))
      {
         num++;
      }
      cmsLog_debug("got %s routeProtocol=%s num=%d",
            ipConnObj->X_BROADCOM_COM_IfName,
            ipConnObj->routeProtocolRx, num);
      cmsObj_free((void **) &ipConnObj);
   }

   cmsLog_debug("returning num=%d", num);
   return num;
}


void rutRip_addAllRipInterfacesToConfigFile_igd(FILE *fs)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   WanIpConnObject *ipConnObj = NULL;
   UBOOL8 isPureRipv1 = FALSE;
   UBOOL8 isPureRipv2 = FALSE;

   cmsLog_debug("entered:");


   /* now look for WAN side RIP interfaces */
   while (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &ipConnObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(ipConnObj->routeProtocolRx, MDMVS_OFF))
      {
         rutRip_addRipEntryToConfigFile(fs, ipConnObj->X_BROADCOM_COM_IfName,
                               ipConnObj->routeProtocolRx,
                               ipConnObj->X_BROADCOM_COM_RipOperationMode);

            /* Record the WAN interface RIP versions */ 
         if (cmsUtl_strcmp(ipConnObj->routeProtocolRx, MDMVS_RIPV1) == 0)
         {
            isPureRipv1 = TRUE;
         }
         else if (cmsUtl_strcmp(ipConnObj->routeProtocolRx, MDMVS_RIPV2) == 0)
         {
            isPureRipv2 = TRUE;
         }
         else if (cmsUtl_strcmp(ipConnObj->routeProtocolRx, MDMVS_RIPV1V2) == 0)
         {
            isPureRipv1 = TRUE;
            isPureRipv2 = TRUE;
         }

         /* Yucky side effect, but very convenient to do it here.
          * Add firewall exception for RIP on this interface.
          */
         if (ipConnObj->X_BROADCOM_COM_FirewallEnabled)
         {
            rutIpt_ripAddIptableRule(ipConnObj->X_BROADCOM_COM_IfName);
         }
      }

      cmsObj_free((void **) &ipConnObj);
   }

   /* add LAN side bridge interface according to the WAN interfaces RIP version configuration. For now br0
    * Should we also have a routeProtocolRx param on the LAN side interfaces?
    * Seems unlikely we will need to send RIP out on the LAN side,
    * and probably will not receive RIP on the LAN side.
    */
   if (isPureRipv1 && isPureRipv2)
   {
      rutRip_addRipEntryToConfigFile(fs, "br0", MDMVS_RIPV1V2, MDMVS_ACTIVE);
   }
   else if(isPureRipv1)
   {
      rutRip_addRipEntryToConfigFile(fs, "br0", MDMVS_RIPV1, MDMVS_ACTIVE);
   }
   else if(isPureRipv2)
   {
      rutRip_addRipEntryToConfigFile(fs, "br0", MDMVS_RIPV2, MDMVS_ACTIVE);
   }

   return;
}

#endif /* DMP_BASELINE_1 */


void rutRip_addRipEntryToConfigFile(FILE *fs, char *ifcName, char *version, char *operation)
{

   /* add interface name */
   fprintf(fs, "interface %s\n", ifcName);
   /* RIP node */
   fputs("router rip\n", fs);
   fputs("redistribute kernel\n", fs);
   fputs("redistribute connected\n", fs);
   fputs("redistribute static\n", fs);
   fprintf(fs, "network %s\n", ifcName);

   if (cmsUtl_strcmp(operation, MDMVS_PASSIVE) == 0)
   {
     fprintf(fs, "passive-interface %s\n",ifcName);
   }

   fprintf(fs, "interface %s\n", ifcName);

   if (cmsUtl_strcmp(version, MDMVS_RIPV1) == 0)
   {
      if (cmsUtl_strcmp(operation, MDMVS_PASSIVE) == 0)
      {
         fputs("ip rip receive version 1\n", fs);
      }
      else
      { /* RIP_OP_RX_TX */
         fputs("ip rip send version 1\n", fs);
         fputs("ip rip receive version 1\n", fs);
      }
   }
   else if (cmsUtl_strcmp(version, MDMVS_RIPV2) == 0)
   {
      if (cmsUtl_strcmp(operation, MDMVS_PASSIVE) == 0)
      {
         fputs("ip rip receive version 2\n", fs);
      }
      else
      { /* RIP_OP_RX_TX */
         fputs("ip rip send version 2\n", fs);
         fputs("ip rip receive version 2\n", fs);
      }
   }
   else /* both versions */
   {
      if (cmsUtl_strcmp(operation, MDMVS_PASSIVE) == 0)
      {
         fputs("ip rip receive version 1 2\n",fs);
      }
      else
      { /* RIP_OP_RX_TX */
         fputs("ip rip send version 1 2\n", fs);
         fputs("ip rip receive version 1 2\n", fs);
      }
   }
}


void rutRip_writeConfigFile()
{
   FILE* fs = NULL;
   FILE* zebra_fs = NULL;
   CmsRet ret;
   char basedir[CMS_MAX_FULLPATH_LENGTH]={0};
   char cmd[CMS_MAX_FULLPATH_LENGTH+BUFLEN_32]={0};

   cmsLog_debug("Entered:");

   if ((ret = cmsUtl_getRunTimePath("/var/zebra", basedir, sizeof(basedir))) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not form rootdir, ret=%d", ret);
      return;
   }

   /* remove any leftover files from previous ripd config */
   cmsLog_debug("rm -rf %s and mkdir again", basedir);
   snprintf(cmd,sizeof(cmd), "rm -rf %s", basedir);
   rut_doSystemAction("rut_startRipd", cmd);
   snprintf(cmd, sizeof(cmd), "mkdir %s", basedir);
   rut_doSystemAction("rut_startRipd", cmd);


   snprintf(cmd, sizeof(cmd), "%s/zebra.conf", basedir);
   if ((zebra_fs = fopen(cmd, "w+")) == NULL)
   {
      cmsLog_error("Fail to open %s", cmd);
      return;
   }
   /* I guess we just create an empty zebra.conf */
   fclose(zebra_fs);
   zebra_fs = NULL;


   snprintf(cmd, sizeof(cmd), "%s/ripd.conf", basedir);
   if ((fs = fopen(cmd, "w+")) == NULL)
   {
      cmsLog_error("Fail to open %s.", cmd);
      return;
   }


   /* Remove all of the RIP firewall exceptions because they will be added
    * back in as a side effect of rutRip_addAllRipInterfacesToConfigFile.
    */
   rutIpt_ripRemoveAllIptableRules();

   /* Loop over all RIP enabled interfaces to write out config block,
    * plus side effect: add firewall exceptions
    */
   rutRip_addAllRipInterfacesToConfigFile(fs);


   /*
     * for debug purpose, do the following fputs accordingly
     */
#ifdef debugRip
      if ( m_ripCfg.logEnable == RIP_LOG_STDOUT )
         fputs("log stdout\n", fs);
      else if ( m_ripCfg.logEnable == RIP_LOG_FILE )
         fputs("log file /var/log/ripd.log\n", fs);
      if ( m_ripCfg.debugFlag & RIP_DEBUG_EVENTS )
         fputs("debug rip events\n", fs);
      if ( m_ripCfg.debugFlag & RIP_DEBUG_PACKET_TX )
         fputs("debug rip packet send detail\n", fs);
      if ( m_ripCfg.debugFlag & RIP_DEBUG_PACKET_RX )
         fputs("debug rip packet recv detail\n", fs);
#endif /* debugRip */


   fclose(fs);
}


void rutRip_restart()
{
   SINT32 pid;
   CmsRet ret = CMSRET_SUCCESS;
   char basedir[CMS_MAX_FULLPATH_LENGTH]={0};
   char cmd[CMS_MAX_FULLPATH_LENGTH*2+BUFLEN_32]={0};

   cmsLog_debug("Enter");

   if ((ret = cmsUtl_getRunTimePath("/var/zebra", basedir, sizeof(basedir))) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not form rootdir, ret=%d", ret);
      return;
   }


   cmsLog_notice("send restart zebra msg to smd");
   snprintf(cmd, sizeof(cmd), "-f %s/zebra.conf -i %s/zebra.pid", basedir, basedir);
   if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_ZEBRA, cmd, strlen(cmd)+1)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to restart zebra");
   }

   cmsLog_notice("send restart ripd msg to smd");
   snprintf(cmd, sizeof(cmd), "-f %s/ripd.conf -i %s/ripd.pid", basedir, basedir);
   if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_RIPD, cmd, strlen(cmd)+1)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to restart ripd");
   }         

   return;
}


void rutRip_stop()
{
   CmsRet ret;

   cmsLog_debug("Enter");

   if ((ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_ZEBRA, NULL,0)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to stop zebra, ret=%d", ret);
   }

   if ((ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_RIPD, NULL,0)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to stop ripd, ret=%d", ret);
   }

   /* since ripd is not running anymore, remove all of the firewall exceptions */
   rutIpt_ripRemoveAllIptableRules();

   return;
}

#endif /* SUPPORT_RIP */

