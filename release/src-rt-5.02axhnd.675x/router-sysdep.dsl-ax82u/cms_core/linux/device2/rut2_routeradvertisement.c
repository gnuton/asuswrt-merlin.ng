/***********************************************************************
 *
 *  Copyright (c) 2006-2011  Broadcom Corporation
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
/* this file touches TR181 IPv6 objects */
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1 /* aka SUPPORT_IPV6 */
#include <fcntl.h>
#include <unistd.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_qdm.h"
#include "cms_strconv2.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_ipv6.h"
#include "rut2_ra.h"


/*!\file rut2_routeradvertisement.c
 * \brief IPv6 helper functions for rcl2_routeradvertisement.c
 *
 */

const char *radvdConfigFile  = "/var/radvd.conf";

void rutRa_updatePrefixes(const char *ifpath, char **prefixes, UBOOL8 *foundPD)
{
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6PrefixObject *ipv6Prefix = NULL;
   CmsRet ret;

   if (!ifpath)
   {
      cmsLog_error("null ifpath");
      return;
   }

   cmsLog_debug("ifpath=%s", ifpath);

   memset(&pathDesc, 0, sizeof(MdmPathDescriptor));
         
   if ((ret = cmsMdm_fullPathToPathDescriptor(ifpath, &pathDesc)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert %s to pathDesc, ret=%d", ifpath, ret);
      return;
   }         

   CMSMEM_FREE_BUF_AND_NULL_PTR(*prefixes);
   *prefixes = (char *)cmsMem_alloc(CMS_DEV2_RA_PREFIX_LEN, ALLOC_SHARED_MEM);
   *foundPD = FALSE;

   /*
    * 1. Clear the prefixes
    * 2. Get all prefixes of the IP.Interface.i.IPv6Prefix.
    */
   memset(*prefixes, 0, CMS_DEV2_RA_PREFIX_LEN);

   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_IPV6_PREFIX, &(pathDesc.iidStack), 
           &iidStack, OGF_NO_VALUE_UPDATE, (void **) &ipv6Prefix) == CMSRET_SUCCESS)
   {
      UBOOL8 isPD;

      isPD = ((cmsUtl_strcmp(ipv6Prefix->origin, MDMVS_CHILD) == 0) ||
              ((cmsUtl_strcmp(ipv6Prefix->origin, MDMVS_STATIC) == 0) &&
               (cmsUtl_strcmp(ipv6Prefix->staticType, MDMVS_CHILD) == 0)));

      if (cmsUtl_strcmp(ipv6Prefix->status, MDMVS_DISABLED) &&
          ((cmsUtl_strcmp(ipv6Prefix->origin, MDMVS_AUTOCONFIGURED) == 0) || isPD)
         )
      {
         MdmPathDescriptor pathDesc_ipv6prefix;
         char *ipv6prefixFullPath=NULL;

         INIT_PATH_DESCRIPTOR(&pathDesc_ipv6prefix);
         pathDesc_ipv6prefix.oid = MDMOID_DEV2_IPV6_PREFIX;
         pathDesc_ipv6prefix.iidStack = iidStack;
         if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc_ipv6prefix, &ipv6prefixFullPath)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_pathDescriptorToFullPathNoEndDot returns error. ret=%d", ret);
            cmsObj_free((void **)&ipv6Prefix);
            return;
         }

         if (cmsUtl_strlen(*prefixes) > 0)
         {
            strcat(*prefixes, ",");
         }

         strcat(*prefixes, ipv6prefixFullPath);

         CMSMEM_FREE_BUF_AND_NULL_PTR(ipv6prefixFullPath);
         *foundPD |= isPD;
      }

      cmsObj_free((void **)&ipv6Prefix);
   }

   cmsLog_debug("prefixes<%s> foundPD", *prefixes, *foundPD);
   return;
}


void rutRa_createRadvdConf(UBOOL8 foundPD)
{
#ifdef DESKTOP_LINUX
   cmsLog_debug("skip create RadvdConf");
   return;
#else
const char *radvdConf0 = "\
interface br0\n\
{\n\
  AdvSendAdvert on;\n\
  AdvManagedFlag %s;\n\
  AdvOtherConfigFlag on;\n\
  %s\n\
  AdvDefaultPreference low;\n\
";

const char *radvdConf1 = "\
  prefix %s\n\
  {\n\
    AdvPreferredLifetime %s;\n\
    AdvValidLifetime %s;\n\
    AdvOnLink on;\n\
    AdvAutonomous %s;\n\
    AdvRouterAddr off;\n\
  };\n\
";

const char *radvdConf0Ext = "\
  MaxRtrAdvInterval 100;\n\
  MinRtrAdvInterval 30;\n\
";

const char *radvdConf1Ext = "\
  prefix %s\n\
  {\n\
    AdvPreferredLifetime 0;\n\
    AdvValidLifetime %s;\n\
    AdvOnLink on;\n\
    AdvAutonomous %s;\n\
    AdvRouterAddr off;\n\
    DecrementLifetimes on;\n\
  };\n\
";

const char *radvdConf3 = "\
  route %s\n\
  {\n\
    AdvRoutePreference high;\n\
    AdvRouteLifetime %s;\n\
  };\n\
";

const char *radvdConf2 = "\
};\n\
";

   const char *tmpConfFile = "/var/radvd_tmp.conf";
   FILE *fp;
   char *sp;
   char prefixStr[CMS_DEV2_RA_PREFIX_LEN];
   char *nextToken;
   char advManageStr[BUFLEN_8];
   char routerLifetime[BUFLEN_32]="";
   UBOOL8 statefulDhcp;
   UBOOL8 foundConf0Ext = FALSE;
   InstanceIdStack iidStack_raIntf = EMPTY_INSTANCE_ID_STACK;
   Dev2RouterAdvertisementInterfaceSettingObject *raIntfObj = NULL;

   cmsLog_debug("enter foundPD<%d>", foundPD);

   /* create a temporary radvd.conf file for write */
   if ((fp = fopen(tmpConfFile, "w")) == NULL)
   {
      cmsLog_error("failed to open %s\n", tmpConfFile);
      return;
   }

   /* 
    * RFC 6204: If there is no delegated prefix, set router lifetime to 0 
    * (item ULA-5 and L-4)
    *
    * If there is no default gateway at WAN, set router lifetime to 0 (except for 6rd)
    */
   if (!foundPD)  //FIXME AAA: rutWan_isDfltGtwyExist()
   {
      cmsLog_notice("NULL prefix in prefixObj or zero lifetime RA received!!");
      strcpy(routerLifetime, "AdvDefaultLifetime 0;");
   }

   /* FIXME: case of multiple bridges!! */
   while (cmsObj_getNext(MDMOID_DEV2_ROUTER_ADVERTISEMENT_INTERFACE_SETTING, 
                         &iidStack_raIntf, (void **) &raIntfObj) == CMSRET_SUCCESS)
   {
      statefulDhcp = raIntfObj->advManagedFlag;

      if (statefulDhcp)
      {
         strcpy(advManageStr, "on");
      }
      else
      {
         strcpy(advManageStr, "off");
      }

      /* write the first part of the conf file */
      fprintf(fp, radvdConf0, advManageStr, routerLifetime);

      /* 
       * Advertise prefixes based on RAIntf.i.prefixes which is already updated
       */

      /* get a local copy of prefixes because we are going to use strtok_r() */
      *prefixStr = '\0';
      if (!IS_EMPTY_STRING(raIntfObj->prefixes))
      {
         strncpy(prefixStr, raIntfObj->prefixes, sizeof(prefixStr));
      }

      /* retrieve each prefix from prefixStr */
      nextToken = NULL;
      sp = strtok_r(prefixStr, ",", &nextToken);

      while (!IS_EMPTY_STRING(sp))
      {
         MdmPathDescriptor pathDesc;
         Dev2Ipv6PrefixObject *ipv6Prefix = NULL; 
         char pltimeStr[BUFLEN_16];
         char vltimeStr[BUFLEN_16];
         char vltimeOldStr[BUFLEN_16];
         char slaacStr[BUFLEN_8];
         char snPrefix[CMS_IPADDR_LENGTH];
         char snPrefixOld[CMS_IPADDR_LENGTH];
         CmsRet ret;

         INIT_PATH_DESCRIPTOR(&pathDesc);
         ret = cmsMdm_fullPathToPathDescriptor(sp, &pathDesc);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s, ret=%d", sp, ret);
            cmsObj_free((void **) &raIntfObj);
            fclose(fp);
            remove(tmpConfFile);
            return;
         }

         if ((ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, 0, (void **)&ipv6Prefix)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_get failed for %s, ret=%d", sp, ret);
            cmsObj_free((void **) &raIntfObj);
            fclose(fp);
            remove(tmpConfFile);
            return;
         }

         if ((ret = cmsNet_subnetIp6SitePrefix(ipv6Prefix->prefix, 0, 64, snPrefix)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsNet_subnetIp6SitePrefix failed for %s, ret=%d", sp, ret);
            cmsObj_free((void **) &raIntfObj);
            fclose(fp);
            remove(tmpConfFile);
            return;
         }

         if (ipv6Prefix->X_BROADCOM_COM_Vlt_Old > 0)
         {
            if ((ret = cmsNet_subnetIp6SitePrefix(ipv6Prefix->X_BROADCOM_COM_Prefix_Old, 0, 64, snPrefixOld)) != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsNet_subnetIp6SitePrefix failed for %s, ret=%d", sp, ret);
               cmsObj_free((void **) &raIntfObj);
               fclose(fp);
               remove(tmpConfFile);
               return;
            }
         }

         if (cmsUtl_strcmp(ipv6Prefix->status, MDMVS_DISABLED))
         {
            if (ipv6Prefix->X_BROADCOM_COM_Plt < 0)
            {
               strcpy(pltimeStr, "infinity");
            }
            else
            {
               sprintf(pltimeStr, "%d", ipv6Prefix->X_BROADCOM_COM_Plt);
            }
            if (ipv6Prefix->X_BROADCOM_COM_Vlt < 0)
            {
               strcpy(vltimeStr, "infinity");
            }
            else
            {
               sprintf(vltimeStr, "%d", ipv6Prefix->X_BROADCOM_COM_Vlt);
            }

            if (statefulDhcp)
            {
               strcpy(slaacStr, "off");
            }
            else
            {
               strcpy(slaacStr, "on");
            }

            if (!foundConf0Ext && ipv6Prefix->X_BROADCOM_COM_Vlt_Old > 0)
            {
               foundConf0Ext = TRUE;
               fprintf(fp, radvdConf0Ext);
            }

            cmsUtl_strcat(snPrefix, "/64");
            fprintf(fp, radvdConf1, snPrefix, pltimeStr, vltimeStr, slaacStr);

            if (ipv6Prefix->X_BROADCOM_COM_Vlt_Old > 0)
            {
               if (!IS_EMPTY_STRING(snPrefixOld))
               {
                  cmsUtl_strcat(snPrefixOld, "/64");
                  sprintf(vltimeOldStr, "%d", ipv6Prefix->X_BROADCOM_COM_Vlt_Old);
                  fprintf(fp, radvdConf1Ext, snPrefixOld, vltimeOldStr, slaacStr);
               }
            }

            /* RFC 4191: Route Information Option FIXME: for multiple route!!! Sync with Prefix */
            fprintf(fp, radvdConf3, ipv6Prefix->prefix, vltimeStr);

            /* check for the cdrouter dhcpv6_pd_130 */
            cmsLog_debug("<%s>assigned for prefix definition", snPrefix);
            cmsLog_debug("<%s>assigned for route definition", ipv6Prefix->prefix);
         }

         cmsObj_free((void **) &ipv6Prefix);

         sp = strtok_r(NULL, ",", &nextToken);
      }

      cmsObj_free((void **) &raIntfObj);   
   }

   /* write the last part of the conf file */
   fprintf(fp, radvdConf2);
   fclose(fp);

   remove(radvdConfigFile);
   rename(tmpConfFile, radvdConfigFile);

   return;

#endif /* DESKTOP_LINUX */  
}

void rutRa_updateRouterAdvObj(const char *ipIntfFullPath)
{
   InstanceIdStack iidStackRaIntf = EMPTY_INSTANCE_ID_STACK;
   Dev2RouterAdvertisementInterfaceSettingObject *raIntfObj = NULL;
   UBOOL8 foundRa = FALSE;

   cmsLog_debug("radvd for PD:ip.intf<%s>", ipIntfFullPath);

   while (!foundRa && cmsObj_getNext(MDMOID_DEV2_ROUTER_ADVERTISEMENT_INTERFACE_SETTING,
                      &iidStackRaIntf, (void **) &raIntfObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(raIntfObj->interface, ipIntfFullPath))
      {
         foundRa = TRUE;
      }
      else
      {
         cmsObj_free((void **) &raIntfObj);
      }
   }

   if (foundRa)
   {
      CmsRet ret;

      ret = cmsObj_set(raIntfObj, &iidStackRaIntf);
      cmsObj_free((void **) &raIntfObj);

      if (ret != CMSRET_SUCCESS)
      {
          cmsLog_error("Failed setting RAInterfaceSetting. ret %d", ret);
      }
   }
   else
   {
      cmsLog_error("Failed getting RAInterfaceSetting.");
   }

   return;
}

void rutRa_restartRadvd(void)
{
   char cmdLine[BUFLEN_128];
   UINT32 pid;

   snprintf(cmdLine, sizeof(cmdLine), "-C %s", radvdConfigFile);
   if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_RADVD,
                               cmdLine, strlen(cmdLine)+1)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to start or restart radvd on br0");
      return;
   }
   else
   {
      cmsLog_debug("restarting radvd, pid=%d on br0", pid);
   }

   return;
}

void rutRa_stopRadvd(void)
{
   rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_RADVD, NULL, 0);
   remove(radvdConfigFile);
   return;

}  /* End of rut_stopRadvd() */


static CmsRet rutRa_setUlaForBridge(const char *bridgeName)
{  
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2Ipv6PrefixObject *ipv6Prefix = NULL;
   UBOOL8 found = FALSE;
   char   ULAddress[BUFLEN_48];
   char   cmdLine[BUFLEN_128];
   UINT32 prefixLen;

   while (cmsObj_getNextFlags(MDMOID_DEV2_IPV6_PREFIX,
                        &iidStack,
                        OGF_NO_VALUE_UPDATE,
                        (void **)&ipv6Prefix) == CMSRET_SUCCESS)
   {
      if (ipv6Prefix->X_BROADCOM_COM_UniqueLocalFlag)
      {
         found = TRUE;
         break;
      }
      
      cmsObj_free((void **) &ipv6Prefix);
   }

   /* ULA Prefix info */
   if (found && !IS_EMPTY_STRING(ipv6Prefix->prefix))
   {
      /* add UL Address to br0 at boot time*/
      *ULAddress = '\0';
      cmsUtl_getULAddressByPrefix(ipv6Prefix->prefix, bridgeName, ULAddress, &prefixLen);
      snprintf(cmdLine, sizeof(cmdLine), "ip -6 addr add %s/%u dev %s 2>/dev/null", ULAddress, prefixLen, bridgeName);
      rut_doSystemAction("rut", cmdLine);
   }
       
   cmsObj_free((void **) &ipv6Prefix);
  
   return CMSRET_SUCCESS;
}

CmsRet rutRa_restartRadvdForBridge(const char *bridgeName __attribute__((unused)))
{
   _Dev2RouterAdvertisementInterfaceSettingObject *adInfObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = cmsObj_getNextFlags(MDMOID_DEV2_ROUTER_ADVERTISEMENT_INTERFACE_SETTING, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &adInfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_debug("Could not get MDMOID_DEV2_ROUTER_ADVERTISEMENT_INTERFACE_SETTING, ret=%d", ret);
      return ret;
   }

   if (adInfObj->enable)
   {
      if (!cmsUtl_strcmp(bridgeName, "br0"))
      {
         rutRa_setUlaForBridge(bridgeName); /*radvd only works on br0*/
      }
#ifndef DESKTOP_LINUX;
      if (access(radvdConfigFile, F_OK) == 0)
      {
         rut_restartRadvd();
      }
#endif      
   }

   cmsObj_free((void **) &adInfObj);

   return CMSRET_SUCCESS;
}

#endif  /* SUPPORT_IPV6 */

#endif  /* DMP_DEVICE2_BASELINE_1 */

