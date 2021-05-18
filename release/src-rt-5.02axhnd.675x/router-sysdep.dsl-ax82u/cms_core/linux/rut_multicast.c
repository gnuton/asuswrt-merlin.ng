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
#include <sys/file.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <errno.h>


#include "cms_core.h"
#include "cms_qos.h"
#include "odl.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_multicast.h"
#include "qdm_multicast.h"
#include "rut_wan.h"
#include "rut_lan.h"
#include "rut_pmap.h"
#include "bcm_mcast_api.h"
#include "rut_qos.h"
#include "rut_ethswitch.h"
#include "rut_wanlayer2.h"
#include "bridgeutil.h"
#include "beep_networking.h"


CmsRet rutMulti_IsMcastStrictWan(UBOOL8 *bStrict);

#ifdef DMP_X_BROADCOM_COM_IGMPSNOOP_1
UBOOL8 rutMulti_isIgmpSnoopingCfgChanged(const _IgmpSnoopingCfgObject *newObj,
                                       const _IgmpSnoopingCfgObject *currObj)
{
   if (!POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      return FALSE;
   }

   if (cmsUtl_strcmp(newObj->mode, currObj->mode) ||
       newObj->lanToLanEnable != currObj->lanToLanEnable)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


void rutMulti_configIgmpSnooping(const char *brIntfName,
                                 UINT32 mode, SINT32 lanToLanEnable)
{
   int ret;
   int ifi;

   cmsLog_debug("entered: brIntfName=%s mode=%d lanToLan=0x%x",
                brIntfName, mode, lanToLanEnable);

   if (IS_EMPTY_STRING(brIntfName))
   {
      cmsLog_debug("brIntfName is NULL, do nothing");
      return;
   }

   ifi = cmsNet_getIfindexByIfname(brIntfName);
   ret = bcm_mcast_api_set_snooping_cfg(-1, ifi, BCM_MCAST_PROTO_IPV4, mode, lanToLanEnable);
   if(ret < 0)
   {
      cmsLog_error("enable igmp snooping on %s mode %d l2l %d failed, ret=%d", 
                    brIntfName, mode, lanToLanEnable, ret);
   }
}


void rutMulti_updateIgmpSnooping(const char *brIntfName)
{
   char *fullPath = NULL;
   int mode;
   CmsRet ret;

   cmsLog_debug("Entered: brIntfName=%s", brIntfName);

   /* go from brIntfName to igmp snooping object */
   ret = qdmMulti_getAssociatedIgmpSnoopingFullPathLocked(brIntfName, &fullPath);
   if (ret != CMSRET_SUCCESS)
   {
      /* During delete, we cannot get the bridge object anymore.  Don't
       * complain loudly and just return.
       */
      cmsLog_debug("getAssociatedIgmpSnooping on %s failed, ret=%d", brIntfName, ret);
      return;
   }

   ret = qdmMulti_getAssociatedBridgeModeLocked(fullPath, &mode);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get bridge's mode, ret=%d", ret);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      return;
   }
   else if (mode != INTFGRP_BR_HOST_MODE)
   {
      cmsLog_debug("igmpsnooping is not supported for BEEP bridge %s", brIntfName);
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);
      return;
   }

   rutMulti_updateIgmpMldSnoopingObj(fullPath);

   CMSMEM_FREE_BUF_AND_NULL_PTR(fullPath);

   return;
}

#endif  /* DMP_X_BROADCOM_COM_IGMPSNOOP_1 */


#if defined(DMP_X_BROADCOM_COM_IGMPSNOOP_1) || defined(DMP_X_BROADCOM_COM_MLDSNOOP_1)
void rutMulti_updateIgmpMldSnooping(const char *brIntfName)
{
#ifdef DMP_X_BROADCOM_COM_IGMPSNOOP_1
   rutMulti_updateIgmpSnooping(brIntfName);
#endif

#ifdef DMP_X_BROADCOM_COM_MLDSNOOP_1
   rutMulti_updateMldSnooping(brIntfName);
#endif
}
#else
void rutMulti_updateIgmpMldSnooping(const char *brIntfName __attribute__((unused)))
{
}
#endif /* defined(DMP_X_BROADCOM_COM_IGMPSNOOP_1) || defined(DMP_X_BROADCOM_COM_MLDSNOOP_1) */


#if defined(DMP_X_BROADCOM_COM_IGMPSNOOP_1) || defined(DMP_X_BROADCOM_COM_MLDSNOOP_1)

/* this function works for both IGMP and MLD snooping objects */
void rutMulti_updateIgmpMldSnoopingObj(const char *snoopFullPath)
{
   MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
   void *snoopObj=NULL;
   CmsRet ret;

   cmsLog_debug("Entered: fullPath=%s", snoopFullPath);

   ret = cmsMdm_fullPathToPathDescriptor(snoopFullPath, &pathDesc);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("fullPathToPathDesc on %s failed, ret=%d", snoopFullPath, ret);
      return;
   }

   /* get snooping obj */
   ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, 0, &snoopObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("get of snoopObj failed, ret=%d", ret);
      return;
   }

   /* do a "set without changing param" and let logic in RCL handler take
    * care of the rest */
   ret = cmsObj_set(snoopObj, &pathDesc.iidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("set of snoopObj failed, ret=%d", ret);
   }

   cmsObj_free((void **) &snoopObj);

   return;
}

#endif /* DMP_X_BROADCOM_COM_IGMPSNOOP_1 || DMP_X_BROADCOM_COM_MLDSNOOP_1 */




CmsRet rutMulti_addIntfNameToList(const char *caller, const char *intfName,
                                  char *intfNamesBuf, UINT32 intfNamesLen)
{
   if (strlen(intfNamesBuf) + strlen(intfName) + 1 >= intfNamesLen)
   {
      cmsLog_error("buffer from %s is too small (%s) %d + %d + 1 >= %d",
                   caller, intfNamesBuf,
                   strlen(intfNamesBuf), strlen(intfName), intfNamesLen);
      return CMSRET_RESOURCE_EXCEEDED;
   }
   else
   {
      /* only add an interface once */
      char *listCopy = cmsMem_strdup(intfNamesBuf);
      char *ptr;
      char *savePtr = NULL;

      ptr = strtok_r(listCopy, " ", &savePtr);
      while(ptr != NULL)
      {
         if ( 0 == strcmp(ptr, intfName) )
         {
            break;
         }
         ptr = strtok_r(NULL, " ", &savePtr);
      }
      if ( NULL == ptr )
      {
         strcat(intfNamesBuf, intfName);
         strcat(intfNamesBuf, " ");
      }
      cmsMem_free(listCopy);
      return CMSRET_SUCCESS;
   }
}


/*
 * This is a generic function for IGMP and MLD snooping, works for
 * all data models.
 */
CmsRet rutMulti_getAllSnoopingIntfNames(UBOOL8 isMld,
                                        char *intfNamesBuf, 
                                        UINT32 intfNamesLen)
{
    unsigned int brindex;
    unsigned int numbridges = MAX_BRIDGES;
    char brname[IFNAMSIZ];
    int brindices[MAX_BRIDGES];
    CmsRet ret = CMSRET_SUCCESS;

    if (intfNamesLen == 0 || intfNamesBuf == NULL)
    {
       cmsLog_error("intfNamesBuf is NULL or length is 0");
       return CMSRET_INVALID_ARGUMENTS;
    }

    if (br_util_get_bridges(brindices, &numbridges) < 0)
    {
        cmsLog_error("no bridge interfaces found");
        return CMSRET_INTERNAL_ERROR;
    }

    memset(intfNamesBuf, 0, intfNamesLen); 
    for (brindex = 0; brindex < numbridges; brindex++)
    {
        if(0 == cmsNet_getIfnameByIndex(brindices[brindex], brname))
        {
            UBOOL8 snoopingEnabled = FALSE;
            UINT32 mode;
            int lan2Lan;
            CmsRet r2;
  
            r2 = qdmMulti_getSnoopingInfoLocked (brname, isMld, &mode, &lan2Lan, &snoopingEnabled);
            /* An Internal error indicates that the bridge is not in the MDM and must      *
             * have been created by GPON's OMCI. Snooping is always enabled on such links. */
            if ((r2 == CMSRET_INTERNAL_ERROR) || (snoopingEnabled == TRUE))
            {
                cmsLog_debug("Adding %s (r2=%d snoopingEnabled=%d)",
                           brname, r2, snoopingEnabled);
                ret = rutMulti_addIntfNameToList("snoop", brname,
                                           intfNamesBuf, intfNamesLen);
            }
        }
        else
        {
            ret = CMSRET_INTERNAL_ERROR;
        }
    }

    return ret;
}


void rutMulti_updateIgmpMldSnoopingIntfList()
{
    /* tell mcpd that igmp snooping bridge intf list has changed */
    rutMulti_updateIgmpSnoopingIntfList();

    /* tell mcpd that MLD snooping bridge intf list has changed */
    rutMulti_updateMldSnoopingIntfList();
}


void rutMulti_updateIgmpSnoopingIntfList()
{
#ifdef DMP_X_BROADCOM_COM_IGMP_1
   char igmpSnoopingIntfNames[128]={0};
   UBOOL8 isMld = FALSE;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   IgmpCfgObject *igmpObj = NULL;
   CmsRet ret;

   ret = rutMulti_getAllSnoopingIntfNames(isMld, igmpSnoopingIntfNames, sizeof(igmpSnoopingIntfNames));
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("getAllSnoopingIntfNames failed, ret=%d", ret);
      return;
   }

   if ((ret = cmsObj_get(MDMOID_IGMP_CFG,
                         &iidStack,
                         0, (void **) &igmpObj)) != CMSRET_SUCCESS)
   {
      cmsLog_notice("could not get MDMOID_IGMP_CFG, ret=%d", ret);
      return;
   }

   if (IS_EMPTY_STRING(igmpSnoopingIntfNames))
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(igmpObj->igmpBridgeIfNames);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(igmpObj->igmpBridgeIfNames, igmpSnoopingIntfNames, mdmLibCtx.allocFlags);
   }

   if ((ret = cmsObj_set(igmpObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("set of igmp obj failed, ret=%d", ret);
   }

   cmsObj_free((void **) &igmpObj);

#endif   /* DMP_X_BROADCOM_COM_IGMP_1 */

   return;
}




void rutMulti_getAllProxyIntfNames_igd(UBOOL8 isMld __attribute__((unused)),
      char *proxyIntfNames, UINT32 proxyIntfNamesLen __attribute__((unused)),
      char *sourceIntfNames, UINT32 sourceIntfNamesLen __attribute__((unused)))
{
   InstanceIdStack iidStack1 = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
   WanPppConnObject *wan_ppp_con = NULL;
   WanIpConnObject *wan_ip_con = NULL;

#if defined(SUPPORT_DM_HYBRID)
   /* In Hybrid mode, check DEV2 objects 
      proxyIntfNames and sourceIntfNames will be initialized by
      rutMulti_getAllProxyIntfNames_dev2 */
   rutMulti_getAllProxyIntfNames_dev2(isMld,
                                 proxyIntfNames, proxyIntfNamesLen,
                                 sourceIntfNames, sourceIntfNamesLen);
#else
   if (proxyIntfNames == NULL)
   {
      cmsLog_error("proxyIntfNames is NULL");
      return;
   }
   proxyIntfNames[0] = '\0';

   if (sourceIntfNames == NULL)
   {
      cmsLog_error("sourceIntfNames is NULL");
      return;
   }
   sourceIntfNames[0] = '\0';
#endif

   /*
    * mcpd can take a list of all routed interfaces, regardless of whether
    * they are currently up or not.  mcpd will learn the interface only if
    * it has an IP address assigned to it.  However, whenever a routed
    * wan interfaces comes up and down, we have to send a reload msg to mcpd.
    * (The current code is somewhat convoluted given the above description.
    * What it should do is update the IGMP/MLD interface list only when
    * the interfaces are configured or deleted.  Whenever an interface
    * comes up/down, just send a reload msg to mcpd.)
    */
   while (cmsObj_getNextFlags(MDMOID_WAN_IP_CONN, &iidStack1,
               OGF_NO_VALUE_UPDATE, (void **) &wan_ip_con) == CMSRET_SUCCESS)
   {
#ifdef DMP_X_BROADCOM_COM_IGMP_1
      if (!isMld)
      {
         if (wan_ip_con->X_BROADCOM_COM_IGMPEnabled == TRUE)
         {
            rutMulti_addIntfNameToList("igmp_proxy",
                                    wan_ip_con->X_BROADCOM_COM_IfName,
                                    proxyIntfNames, proxyIntfNamesLen);
         }

         /*
          * For backward compat reasons, put the interface on the SOURCE
          * list if SOURCEEnabled *OR* IGMPEnabled.
          */
         if ((wan_ip_con->X_BROADCOM_COM_IGMP_SOURCEEnabled == TRUE) ||
             (wan_ip_con->X_BROADCOM_COM_IGMPEnabled == TRUE))
         {
            rutMulti_addIntfNameToList("igmp_source",
                                    wan_ip_con->X_BROADCOM_COM_IfName,
                                    sourceIntfNames, sourceIntfNamesLen);
         }
      }
#endif

#if defined(DMP_X_BROADCOM_COM_MLD_1)
      if (isMld)
      {
         if (wan_ip_con->X_BROADCOM_COM_MLDEnabled == TRUE)
         {
            rutMulti_addIntfNameToList("mld_proxy",
                                    wan_ip_con->X_BROADCOM_COM_IfName,
                                    proxyIntfNames, proxyIntfNamesLen);
         }

         if ((wan_ip_con->X_BROADCOM_COM_MLD_SOURCEEnabled == TRUE) ||
             (wan_ip_con->X_BROADCOM_COM_MLDEnabled == TRUE))
         {
            rutMulti_addIntfNameToList("mld_source",
                                    wan_ip_con->X_BROADCOM_COM_IfName,
                                    sourceIntfNames, sourceIntfNamesLen);
         }
       }
#endif
       cmsObj_free((void **) &wan_ip_con);
    }

   while (cmsObj_getNextFlags(MDMOID_WAN_PPP_CONN, &iidStack2,
               OGF_NO_VALUE_UPDATE, (void **) &wan_ppp_con) == CMSRET_SUCCESS)
   {
#ifdef DMP_X_BROADCOM_COM_IGMP_1
      if (!isMld)
      {
         if (wan_ppp_con->X_BROADCOM_COM_IGMPEnabled == TRUE)
         {
            rutMulti_addIntfNameToList("igmp_proxy",
                                    wan_ppp_con->X_BROADCOM_COM_IfName,
                                    proxyIntfNames, proxyIntfNamesLen);
         }

         /*
          * For backward compat reasons, put the interface on the SOURCE
          * list if SOURCEEnabled *OR* IGMPEnabled.
          */
         if ((wan_ppp_con->X_BROADCOM_COM_IGMP_SOURCEEnabled == TRUE) ||
             (wan_ppp_con->X_BROADCOM_COM_IGMPEnabled == TRUE))
         {
            rutMulti_addIntfNameToList("igmp_source",
                                    wan_ppp_con->X_BROADCOM_COM_IfName,
                                    sourceIntfNames, sourceIntfNamesLen);
         }
      }
#endif

#if defined(DMP_X_BROADCOM_COM_MLD_1)
      if (isMld)
      {
         if (wan_ppp_con->X_BROADCOM_COM_MLDEnabled == TRUE)
         {
            rutMulti_addIntfNameToList("mld_proxy",
                                    wan_ppp_con->X_BROADCOM_COM_IfName,
                                    proxyIntfNames, proxyIntfNamesLen);
         }

         if ((wan_ppp_con->X_BROADCOM_COM_MLD_SOURCEEnabled == TRUE) ||
             (wan_ppp_con->X_BROADCOM_COM_MLDEnabled == TRUE))
         {
            rutMulti_addIntfNameToList("mld_source",
                                    wan_ppp_con->X_BROADCOM_COM_IfName,
                                    sourceIntfNames, sourceIntfNamesLen);
         }
       }
#endif
       cmsObj_free((void **) &wan_ppp_con);
    }

   return;
}



void rutMulti_updateIgmpProxyIntfList()
{
#ifdef DMP_X_BROADCOM_COM_IGMP_1
   char proxyIntfNames[128]={0};
   char sourceIntfNames[128]={0};
   IgmpCfgObject *igmpObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UBOOL8 isMld=FALSE;

   rutMulti_getAllProxyIntfNames(isMld,
                               proxyIntfNames, sizeof(proxyIntfNames),
                               sourceIntfNames, sizeof(sourceIntfNames));

   if ((ret = cmsObj_get(MDMOID_IGMP_CFG, &iidStack, 0,
                                     (void **) &igmpObj)) != CMSRET_SUCCESS)
   {
       cmsLog_error("could not get MDMOID_IGMP_CFG, ret=%d", ret);
       return;
   }

   if (IS_EMPTY_STRING(proxyIntfNames))
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(igmpObj->igmpProxyIfNames);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(igmpObj->igmpProxyIfNames, proxyIntfNames, mdmLibCtx.allocFlags);
   }

   if (IS_EMPTY_STRING(sourceIntfNames))
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(igmpObj->igmpMcastIfNames);
   }
   else
   {
      CMSMEM_REPLACE_STRING_FLAGS(igmpObj->igmpMcastIfNames, sourceIntfNames, mdmLibCtx.allocFlags);
   }

   if ((ret = cmsObj_set(igmpObj, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("IGMP set failed, ret=%d", ret);
   }

   cmsObj_free((void **) &igmpObj);

#endif /* DMP_X_BROADCOM_COM_IGMP_1 */

   return;
}


void rutMulti_updateIgmpMldProxyIntfList(void)
{
   /* tell mcpd that mcast proxy and source intf lists have changed */
   rutMulti_updateIgmpProxyIntfList();

   /* tell mcpd that mcast proxy and source intf lists have changed */
   rutMulti_updateMldProxyIntfList();
}


CmsRet rutMulti_isAdmissionRequired(UBOOL8 *isAdmissionReq, UBOOL8 *isAdmissionBrFilterReq)
{
   /* if MulticastSubscriberConfigInfoObject is not present then there is no admission
      if MulticastSubscriberConfigInfoObject is present then
         if OMCI light is defined then there is no admission
         otherwise admission is required */
    CmsRet ret = CMSRET_OBJECT_NOT_FOUND;

    *isAdmissionReq = FALSE;
    *isAdmissionBrFilterReq = FALSE;

#ifdef DMP_X_BROADCOM_COM_GPON_1
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *systemObj = NULL;
    BcmOmciRtdMcastObject *mcastCfg = NULL;
    UBOOL8 forceForward = FALSE;

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0, (void*)&systemObj))
      == CMSRET_SUCCESS)
    {
        if (systemObj->joinForceForward == TRUE)
        {
            forceForward = TRUE;
        }
        cmsObj_free((void **)&systemObj);
    }

    INIT_INSTANCE_ID_STACK(&iidStack);
    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_RTD_MCAST,
                              &iidStack,
                              0,
                              (void **)&mcastCfg)) == CMSRET_SUCCESS)
    {
        *isAdmissionReq = mcastCfg->igmpAdmission;
        /* enable the igmp packet bridge filter when admission is required */
        if ((forceForward != TRUE) && (mcastCfg->joinForceForward != TRUE))
        {
            *isAdmissionBrFilterReq = TRUE;
        }
        cmsObj_free((void **) &mcastCfg);
    }
#endif

    return ret;
}

CmsRet rutMulti_getIgmpRateLimitOnRgBridge(UINT32 *igmpRate)
{
    CmsRet ret = CMSRET_OBJECT_NOT_FOUND;
    *igmpRate = 0;

#ifdef DMP_X_BROADCOM_COM_GPON_1
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciRtdMcastObject *mcastCfg = NULL;

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_RTD_MCAST,
                              &iidStack,
                              0,
                              (void **)&mcastCfg)) == CMSRET_SUCCESS)
    {
        *igmpRate = mcastCfg->upstreamIgmpRate;
        cmsObj_free((void**)&mcastCfg);
    }
#endif

    return ret;
}

void rutMulti_createWanAssocList ( char *strictWanAssoc )
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   LanIpIntfObject *ipIntfObj=NULL;

   InstanceIdStack fltIidStack = EMPTY_INSTANCE_ID_STACK;
   L2BridgingFilterObject         *pBridgeFltObj = NULL;
   char wanIfName[BUFLEN_32]={0};
 
   SINT32 bridgeNumber = 0;

   INIT_INSTANCE_ID_STACK(&iidStack);

   while(cmsObj_getNext(MDMOID_LAN_IP_INTF, &iidStack, (void **) &ipIntfObj) == CMSRET_SUCCESS)
   {
      bridgeNumber = atoi(&(ipIntfObj->X_BROADCOM_COM_IfName[2]));
      /* cycle through all Bridging Filters */
      INIT_INSTANCE_ID_STACK(&fltIidStack);         
      while (cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, &fltIidStack, (void **)&pBridgeFltObj) == CMSRET_SUCCESS)
      {
         if (pBridgeFltObj->filterBridgeReference == bridgeNumber)
            /* && (cmsUtl_strcmp(pBridgeFltObj->filterInterface, MDMVS_LANINTERFACES)  I dunno about this part*/ 
         {
            L2BridgingIntfObject *availIntfObj=NULL;
            InstanceIdStack availIntfIidStack =  EMPTY_INSTANCE_ID_STACK;
            UINT32 key;
            
            cmsUtl_strtoul(pBridgeFltObj->filterInterface, NULL, 0, &key);
            if(rutPMap_getAvailableInterfaceByKey(key, &availIntfIidStack, &availIntfObj) != CMSRET_SUCCESS)
            {
               cmsLog_error("could not find avail intf for key %u", key);
            }
            else if (!cmsUtl_strcmp(availIntfObj->interfaceType, MDMVS_WANINTERFACE))
            {
               if (strictWanAssoc[0] != '\0')
               {
                  strcat(strictWanAssoc, "|");
               }
               //attach interface name
               rutPMap_availableInterfaceReferenceToIfName(availIntfObj->interfaceReference, wanIfName);
               strcat(strictWanAssoc, ipIntfObj->X_BROADCOM_COM_IfName);
               strcat(strictWanAssoc, " ");
               strcat(strictWanAssoc, wanIfName);
               cmsObj_free((void **) &availIntfObj);
            }            
         }
         cmsObj_free((void **) &pBridgeFltObj);
      }
      cmsObj_free((void **) &ipIntfObj);
   }

}

CmsRet rutMulti_CreateMcpdCfg( void )
{
   char basedir[CMS_MAX_FULLPATH_LENGTH]={0};
   FILE *fp = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 admissionReq = 0;
   UBOOL8 admissionBrFilter = 0;
#if defined(DMP_X_BROADCOM_COM_MCAST_1) && (defined(DMP_X_BROADCOM_COM_IGMP_1) || defined(DMP_X_BROADCOM_COM_MLD_1))
   UBOOL8 bStrict = FALSE;
   char strictWanAssoc [BUFLEN_256] = {0};
#endif 

   cmsLog_debug("Entered:");

   if ((ret = cmsUtl_getRunTimePath(MCPD_CONFIG_FILE, basedir, sizeof(basedir))) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not form rootdir, ret=%d", ret);
      return ret;
   }

   cmsLog_debug("opening config file %s", basedir);
   fp = fopen(basedir, "a");
   if (fp == NULL)
   {
      cmsLog_error("can't open %s error = %d\n", basedir, errno);
      return -1;
   }
   if (flock(fileno(fp), LOCK_EX )) 
   {
      cmsLog_error("can't get file lock for mcpd.conf error = %d\n", errno);
      fclose(fp);
      return -1;
   }    
   /* use truncate and rewind here instead of fopen(path, "a+" because of file locking? */
   if (0 != ftruncate(fileno(fp), 0))
   {
      cmsLog_error("ftruncate failed, errno=%d", errno);
      fclose(fp);
      return -1;
   }
   rewind(fp);

   rutMulti_isAdmissionRequired(&admissionReq, &admissionBrFilter);
   
#ifdef DMP_X_BROADCOM_COM_IGMP_1
   {
   IgmpCfgObject *igmpObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

#ifdef DMP_X_BROADCOM_COM_GPON_1
   BcmOmciRtdMcastObject *gponObj = NULL;
   InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
#endif

   ret = cmsObj_get(MDMOID_IGMP_CFG, &iidStack, 0, (void **) &igmpObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get igmpCfgObject, ret=%d", ret);
      fclose(fp);
      return CMSRET_INTERNAL_ERROR;
   }
   
   ret = rutMulti_IsMcastStrictWan(&bStrict);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("rutMulti_IsMcastStrictWan() failed, ret=%d", ret);
      cmsObj_free((void **)&igmpObj);
      fclose(fp);
      return CMSRET_INTERNAL_ERROR;
   }

#ifdef DMP_X_BROADCOM_COM_GPON_1
   if ((ret = cmsObj_get(MDMOID_BCM_OMCI_RTD_MCAST, &iidStack2, 0,
     (void**)&gponObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get MDMOID_BCM_OMCI_RTD_MCAST, ret=%d", ret);
      cmsObj_free((void **)&igmpObj);
      fclose(fp);
      return CMSRET_INTERNAL_ERROR;
   }
#endif

   fprintf(fp, "#\n");
   fprintf(fp, "#Begin IGMP configuration\n");
   fprintf(fp, "#\n");
   fprintf(fp, "igmp-default-version %d\n", igmpObj->igmpVer);
   fprintf(fp, "igmp-query-interval %d\n", igmpObj->igmpQI);
   fprintf(fp, "igmp-query-response-interval %d\n", igmpObj->igmpQRI);
   fprintf(fp, "igmp-last-member-query-interval %d\n", igmpObj->igmpLMQI);
   fprintf(fp, "igmp-robustness-value %d\n", igmpObj->igmpRV);
   fprintf(fp, "igmp-max-groups %d\n", igmpObj->igmpMaxGroups);
   fprintf(fp, "igmp-max-sources %d\n", igmpObj->igmpMaxSources);
   fprintf(fp, "igmp-max-members %d\n", igmpObj->igmpMaxMembers);
   fprintf(fp, "igmp-fast-leave %d\n", igmpObj->igmpFastLeaveEnable);
   fprintf(fp, "igmp-flood-enable %d\n", igmpObj->igmpFloodEnable);
   fprintf(fp, "igmp-admission-required %d\n", admissionReq);
   if ( admissionReq )
   {
      fprintf(fp, "igmp-admission-bridging-filter %d\n", admissionBrFilter);
   }

   if(igmpObj->igmpProxyIfNames && strlen(igmpObj->igmpProxyIfNames) > 0)
   {
      fprintf(fp, "igmp-proxy-interfaces %s\n", igmpObj->igmpProxyIfNames);
   }
   else
   {
      fprintf(fp, "igmp-proxy-interfaces\n");
   }

   if(igmpObj->igmpBridgeIfNames && strlen(igmpObj->igmpBridgeIfNames) > 0)
   {
      fprintf(fp, "igmp-snooping-interfaces %s\n", igmpObj->igmpBridgeIfNames);
   }
   else
   {
      fprintf(fp, "igmp-snooping-interfaces\n");
   }

#ifndef DMP_X_BROADCOM_COM_GPON_1
   if(igmpObj->igmpMcastIfNames && strlen(igmpObj->igmpMcastIfNames) > 0)
   {
      fprintf(fp, "igmp-mcast-interfaces %s\n", igmpObj->igmpMcastIfNames);
   }
   else
   {
      fprintf(fp, "igmp-mcast-interfaces\n");
   }
#else
   fprintf(fp, "igmp-mcast-interfaces");
   if(igmpObj->igmpMcastIfNames && strlen(igmpObj->igmpMcastIfNames) > 0)
   {
      fprintf(fp, " %s", igmpObj->igmpMcastIfNames);
   }

   if(gponObj->igmpMcastIfNames && strlen(gponObj->igmpMcastIfNames) > 0)
   {
      fprintf(fp, " %s", gponObj->igmpMcastIfNames);
   }

   fprintf(fp, "\n");
#endif

   fprintf(fp, "#\n");
   fprintf(fp, "#End IGMP configuration\n");
   fprintf(fp, "#\n");

#ifdef DMP_X_BROADCOM_COM_GPON_1
   cmsObj_free((void **)&gponObj);
#endif

   cmsObj_free((void **) &igmpObj);
   }
#endif /* DMP_X_BROADCOM_COM_IGMP_1 */

#ifdef DMP_X_BROADCOM_COM_MLD_1
   {
   MldCfgObject *mldObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

   ret = cmsObj_get(MDMOID_MLD_CFG, &iidStack, 0, (void **) &mldObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get mldCfgObject, ret=%d", ret);
      fclose(fp);
      return CMSRET_INTERNAL_ERROR;
   }

   fprintf(fp, "#\n");
   fprintf(fp, "#Begin MLD configuration\n");
   fprintf(fp, "#\n");

   fprintf(fp, "mld-default-version %d\n", mldObj->mldVer);
   fprintf(fp, "mld-query-interval %d\n", mldObj->mldQI);
   fprintf(fp, "mld-query-response-interval %d\n", mldObj->mldQRI);
   fprintf(fp, "mld-last-member-query-interval %d\n", mldObj->mldLMQI);
   fprintf(fp, "mld-robustness-value %d\n", mldObj->mldRV);
   fprintf(fp, "mld-max-groups %d\n", mldObj->mldMaxGroups);
   fprintf(fp, "mld-max-sources %d\n", mldObj->mldMaxSources);
   fprintf(fp, "mld-max-members %d\n", mldObj->mldMaxMembers);
   fprintf(fp, "mld-fast-leave %d\n", mldObj->mldFastLeaveEnable);
   fprintf(fp, "mld-flood-enable %d\n", mldObj->mldFloodEnable);
   fprintf(fp, "mld-admission-required 0\n");

   if(mldObj->mldProxyIfNames && strlen(mldObj->mldProxyIfNames) > 0)
   {
      fprintf(fp, "mld-proxy-interfaces %s\n", mldObj->mldProxyIfNames);
   }
   else
   {
      fprintf(fp, "mld-proxy-interfaces\n");
   }
   
   if(mldObj->mldBridgeIfNames && (strlen(mldObj->mldBridgeIfNames) > 0))
   {
      fprintf(fp, "mld-snooping-interfaces %s\n", mldObj->mldBridgeIfNames);
   }
   else
   {
      fprintf(fp, "mld-snooping-interfaces\n");
   }

   if(mldObj->mldMcastIfNames && strlen(mldObj->mldMcastIfNames) > 0)
   {
      fprintf(fp, "mld-mcast-interfaces %s\n", mldObj->mldMcastIfNames);
   }
   else
   {
      fprintf(fp, "mld-mcast-interfaces\n");
   }

   fprintf(fp, "#\n");
   fprintf(fp, "#End MLD configuration\n");
   fprintf(fp, "#\n");

   cmsObj_free((void **) &mldObj);
   }
#endif /* DMP_X_BROADCOM_COM_MLD_1 */

#if defined(DMP_X_BROADCOM_COM_MCAST_1) && (defined(DMP_X_BROADCOM_COM_IGMP_1) || defined(DMP_X_BROADCOM_COM_MLD_1))
   {
      McastCfgObject *mcastObj = NULL;
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   
      ret = cmsObj_get(MDMOID_MCAST_CFG, &iidStack, 0, (void **) &mcastObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get mcastCfgObject, ret=%d", ret);
         fclose(fp);
         return CMSRET_INTERNAL_ERROR;
      }

      fprintf(fp, "#\n");
      fprintf(fp, "#Begin mcast configuration\n");
      fprintf(fp, "#\n");
   
      if(mcastObj->mcastMaxGroupsPortList && strlen(mcastObj->mcastMaxGroupsPortList) > 0)
      {
         fprintf(fp, "mcast-max-groups-port-list %s\n", mcastObj->mcastMaxGroupsPortList);
      }
      else
      {
         fprintf(fp, "mcast-max-groups-port-list\n");
      }
      fprintf(fp, "mcpd-strict-wan %d\n", bStrict ? 1 : 0);
      if (bStrict) {
        rutMulti_createWanAssocList(strictWanAssoc);
        fprintf(fp, "mcpd-strict-wan-assoc %s\n", strictWanAssoc);
      }

      if(mcastObj->mcastIgmpSnoopExceptions && strlen(mcastObj->mcastIgmpSnoopExceptions) > 0)
      {
         fprintf(fp, "igmp-mcast-snoop-exceptions %s\n", mcastObj->mcastIgmpSnoopExceptions);
      }
      else
      {
         fprintf(fp, "igmp-mcast-snoop-exceptions\n");
      }

#ifdef DMP_X_BROADCOM_COM_MLD_1
      if(mcastObj->mcastMldSnoopExceptions && strlen(mcastObj->mcastMldSnoopExceptions) > 0)
      {
         fprintf(fp, "mld-mcast-snoop-exceptions %s\n", mcastObj->mcastMldSnoopExceptions);
      }
      else
      {
         fprintf(fp, "mld-mcast-snoop-exceptions\n");
      }
#endif

   
      fprintf(fp, "#\n");
      fprintf(fp, "#End mcast configuration\n");
      fprintf(fp, "#\n");      
      cmsObj_free((void **) &mcastObj);

   }
#endif

   fclose(fp);

   return ret;
} /* rutMulti_CreateMcpdCfg */



CmsRet rutMulti_reloadMcpdWithType(int protoType __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

#if defined(DMP_X_BROADCOM_COM_IGMP_1) || defined(DMP_X_BROADCOM_COM_MLD_1)
   CmsMsgHeader msg = EMPTY_MSG_HEADER;

   rutMulti_CreateMcpdCfg();

   /* make sure modules needed for mcpd are loaded */
   rutIpt_McpdLoadModules();

   /*reload mcpd*/
   msg.type = CMS_MSG_MCPD_RELOAD;
   msg.src = mdmLibCtx.eid;
   msg.dst = EID_MCPD;
   msg.flags_request = 1;
   msg.wordData = protoType;

   /*  NOTE: if mcpd is not in the system, 
    *        it will be launched to receive this message 
    */
   ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg);

   if(ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send CMS_MSG_MCPD_RELOAD msg to mcpd, ret=%d", ret);
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_debug("CMS_MSG_MCPD_RELOAD sent successfully");
   }

#endif /* DMP_X_BROADCOM_COM_IGMP_1 || DMP_X_BROADCOM_COM_IGMP_1 */

   return ret;
   
} /* rutMulti_reloadMcpd */

CmsRet rutMulti_reloadMcpd(void)
{
   CmsRet ret = CMSRET_SUCCESS;

#if defined(DMP_X_BROADCOM_COM_IGMP_1) || defined(DMP_X_BROADCOM_COM_MLD_1)
   CmsMsgHeader msg = EMPTY_MSG_HEADER;

   rutMulti_CreateMcpdCfg();

   /* make sure modules needed for mcpd are loaded */
   rutIpt_McpdLoadModules();

   /*reload mcpd*/
   msg.type = CMS_MSG_MCPD_RELOAD;
   msg.src = mdmLibCtx.eid;
   msg.dst = EID_MCPD;
   msg.flags_request = 1;
   msg.wordData = BCM_MCAST_PROTO_ALL;

   /*  NOTE: if mcpd is not in the system, 
    *        it will be launched to receive this message 
    */
   ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg);

   if(ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send CMS_MSG_MCPD_RELOAD msg to mcpd, ret=%d", ret);
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_debug("CMS_MSG_MCPD_RELOAD sent successfully");
   }

#endif /* DMP_X_BROADCOM_COM_IGMP_1 || DMP_X_BROADCOM_COM_IGMP_1 */

   return ret;
   
} /* rutMulti_reloadMcpd */

CmsRet rutMulti_resetMcpd(void)
{
   CmsRet ret = CMSRET_SUCCESS;

#if defined(DMP_X_BROADCOM_COM_IGMP_1) || defined(DMP_X_BROADCOM_COM_MLD_1)
   CmsMsgHeader msg = EMPTY_MSG_HEADER;

   /*reload mcpd*/
   msg.type = CMS_MSG_MCPD_RESET;
   msg.src = mdmLibCtx.eid;
   msg.dst = EID_MCPD;
   msg.flags_request = 1;

   /*  NOTE: if mcpd is not in the system, 
    *        it will be launched to receive this message 
    */
   ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg);

   if(ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send CMS_MSG_MCPD_RESET msg to mcpd, ret=%d", ret);
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_debug("CMS_MSG_MCPD_RESET sent successfully");
   }

#endif /* DMP_X_BROADCOM_COM_IGMP_1 || DMP_X_BROADCOM_COM_IGMP_1 */

   return ret;

} /* rutMulti_reloadMcpd */



#if defined(DMP_X_BROADCOM_COM_MCAST_1)
CmsRet rutMulti_mcastObjCfg(_McastCfgObject *newObj)
{
   CmsRet             ret = CMSRET_SUCCESS;
   int                priorityQueue;
   UBOOL8             bStrict;

   /*
    * Update switch settings for port pause
    */
   rutEsw_updatePortPauseFlowCtrlSetting(QOS_CLS_INVALID_INDEX);


   /*
    * Map mcastPrecedence to PriorityQueue and send it to mcpd via
    * netlink msg.
    */
   if (newObj->mcastPrecedence < 1)
   {
      priorityQueue = -1;
   }
   else
   {
      priorityQueue = ETH_QOS_LEVELS - newObj->mcastPrecedence;
      if ( priorityQueue < 0 )
      {
         priorityQueue = 0;
      }
   }

   cmsLog_debug("mapped mcastPrec %d ==> priorityQ %d", newObj->mcastPrecedence, priorityQueue);
   if ( bcm_mcast_api_set_priority_queue(-1, priorityQueue) < 0 )
   {
      cmsLog_debug("Error setting priority queue");
      ret = CMSRET_INTERNAL_ERROR;
   }

   ret = rutMulti_IsMcastStrictWan(&bStrict);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get mcastCfgObject, ret=%d", ret);
      return CMSRET_INTERNAL_ERROR;
   }

   if (bStrict != newObj->mcastStrictWan) {
      rutMulti_reloadMcpd();
   }

#ifdef DMP_X_BROADCOM_COM_DCSP_MCAST_REMARK_1
   cmsLog_debug("mcastDscpRemarkEnable=%d, mcastDscpRemarkVal=%d ",
        newObj->mcastDscpRemarkEnable, newObj->mcastDscpRemarkVal);
   rdpaCtl_RdpaMwMCastSet(newObj->mcastDscpRemarkEnable ?
        newObj->mcastDscpRemarkVal : -1);
#endif
   return ret;
}
#else
CmsRet rutMulti_mcastObjCfg(_McastCfgObject *newObj __attribute__((unused)))
{
   CmsRet             ret = CMSRET_SUCCESS;
   return ret;
}
#endif

 
UBOOL8 rutMulti_isMcastQosEnabled()
{
    _McastCfgObject *mcastObj = NULL;
    InstanceIdStack iidStack;
    CmsRet ret;
    UBOOL8 enabled=FALSE;

    INIT_INSTANCE_ID_STACK(&iidStack);
    ret = cmsObj_get(MDMOID_MCAST_CFG, &iidStack, 0, (void **)&mcastObj);
    if ( CMSRET_SUCCESS == ret )
    {
        if (mcastObj->mcastPrecedence >= 1)
        {
            enabled = TRUE;
        }
        cmsObj_free((void **)&mcastObj);
    }
    else
    {
       cmsLog_error("Could not get mcastCfg obj, ret=%d", ret);
    }

   return enabled;
}

CmsRet rutMulti_IsMcastStrictWan(UBOOL8 *bStrict)
{
   _McastCfgObject *mcastObj = NULL;
   InstanceIdStack iidStack;
   CmsRet retVal;

   *bStrict = FALSE;
   INIT_INSTANCE_ID_STACK(&iidStack);
   retVal = cmsObj_get(MDMOID_MCAST_CFG, &iidStack, 0, (void **)&mcastObj);
   if ( CMSRET_SUCCESS == retVal )
   {
      if (mcastObj->mcastStrictWan == 1)
      {
         *bStrict = TRUE;
      }
      cmsObj_free((void **)&mcastObj);
   }
   return CMSRET_SUCCESS;
} /* rutMulti_IsMcastStrictWan */ 

#if defined(DMP_X_BROADCOM_COM_IGMPSNOOP_1) || defined(DMP_X_BROADCOM_COM_MLDSNOOP_1)
CmsRet rutMulti_processHostCtrlChange(UBOOL8 enable)
{
    CmsRet ret = CMSRET_SUCCESS;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
    LanIpIntfObject *ipIntfObj=NULL;
    LanDevObject *lanDeviceObj=NULL;
#ifndef DESKTOP_LINUX
    int sock_nl;
#endif
#ifdef DMP_X_BROADCOM_COM_IGMPSNOOP_1
    IgmpSnoopingCfgObject *igmpSnoopingObj = NULL;
#endif
#ifdef DMP_X_BROADCOM_COM_MLDSNOOP_1
    MldSnoopingCfgObject *mldSnoopingObj = NULL;
#endif

    INIT_INSTANCE_ID_STACK(&iidStack);
    INIT_INSTANCE_ID_STACK(&iidStack2);

    ret = cmsObj_getNext(MDMOID_LAN_IP_INTF, &iidStack, (void **) &ipIntfObj);
    if(ret == CMSRET_SUCCESS)
    {
        iidStack2 = iidStack;
        ret = cmsObj_getAncestor(MDMOID_LAN_DEV, MDMOID_LAN_IP_INTF, &iidStack2, (void **) &lanDeviceObj);
        
        /* we only need the instance id, not the lanDeviceObj  */
        cmsObj_free((void **)&lanDeviceObj);
        
#ifdef DMP_X_BROADCOM_COM_IGMPSNOOP_1
        ret = cmsObj_get(MDMOID_IGMP_SNOOPING_CFG, &iidStack2, 0, (void **) &igmpSnoopingObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("could not get igmpSnoopingCfgObject, ret=%d", ret);
            cmsObj_free((void **) &ipIntfObj);
            return CMSRET_INTERNAL_ERROR;
        }

        /* set object without changing it */
        cmsObj_set(igmpSnoopingObj, &iidStack2);
        
        cmsObj_free((void **) &igmpSnoopingObj);
#endif
        
#ifdef DMP_X_BROADCOM_COM_MLDSNOOP_1
        ret = cmsObj_get(MDMOID_MLD_SNOOPING_CFG, &iidStack2, 0, (void **) &mldSnoopingObj);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("could not get mldSnoopingObj, ret=%d", ret);
            cmsObj_free((void **) &ipIntfObj);
            return CMSRET_INTERNAL_ERROR;
        }

        /* set object without changing it */
        cmsObj_set(mldSnoopingObj, &iidStack2);
        
        cmsObj_free((void **) &mldSnoopingObj);
#endif
        cmsObj_free((void **) &ipIntfObj);
    }

#ifndef DESKTOP_LINUX
    if ( bcm_mcast_api_socket_create(&sock_nl, 0x4750514E) < 0 )
    {
        cmsLog_error("could not get mldSnoopingObj, ret=%d", ret);
        return CMSRET_INTERNAL_ERROR;
    }
	
    if (enable)
    {
        /* register with portID "GPON" */
        bcm_mcast_api_register(sock_nl, 1);
        close(sock_nl);
    }
    else
    {
        /* unregister with portID "GPON" */
        bcm_mcast_api_unregister(sock_nl, 1);
    } 
    close(sock_nl);
#endif /* DESKTOP_LINUX */

    cmsLog_notice("%s host-controlled multicast", (enable)?"enable":"disable");

    return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_IGMPSNOOP_1 || DMP_X_BROADCOM_COM_MLDSNOOP_1 */

#if defined(DMP_X_BROADCOM_COM_MCAST_1)
UBOOL8 rutMulti_getHostCtrlConfig(void)
{
    CmsRet ret;
    McastCfgObject *mcastObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    UBOOL8 mcastConfig = FALSE;
    
    ret = cmsObj_get(MDMOID_MCAST_CFG, &iidStack, 0, (void **) &mcastObj);
    if (ret == CMSRET_SUCCESS)
    { 
        mcastConfig = mcastObj->mcastHostControl;
        cmsObj_free((void **)&mcastObj);
    }
    return mcastConfig;
}
#endif

