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

#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_WIFIRADIO_1

/*!\file rut2_wifi.c
 * \brief This file contains common TR181 Wifi helper functions.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <mdm.h>
#include "cms_core.h"
#include "cms_util.h"
#include "rut2_wifi.h"

#ifndef BUILD_BRCM_UNFWLCFG
#include "wlcsm_lib_api.h"
#include "wlcsm_linux.h"
#endif

/* tmp buf used to form a config line */
char wifi_configBuf[BUFLEN_128];

#ifdef DMP_DEVICE2_WIFIACCESSPOINT_1

static void findStationByMacAddr_locked(char *macAddr, InstanceIdStack *apIidStack, InstanceIdStack *adIidStack, void **obj)
{
    CmsRet ret = CMSRET_SUCCESS;
    _Dev2WifiAssociatedDeviceObject *associatedDeviceObj = NULL;

    while ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE, apIidStack, adIidStack, (void **)&associatedDeviceObj)) == CMSRET_SUCCESS)
    {
        if (cmsUtl_strcasecmp(macAddr, associatedDeviceObj->MACAddress) == 0) // find related associated device object
        {
            cmsLog_debug("find associated device MAC:%s", associatedDeviceObj->MACAddress);
            break;
        }
        cmsObj_free((void **) &associatedDeviceObj);
    }

    if (associatedDeviceObj)
        *((_Dev2WifiAssociatedDeviceObject **)obj) = associatedDeviceObj;
}



Dev2WifiSsidObject* rutWifi_get_AP_SSID_dev2(_Dev2WifiAccessPointObject  *obj)
{
    int ssid_idx=0;
    int sta_count=-1;
    sscanf(obj->SSIDReference,"Device.WiFi.SSID.%d",&ssid_idx);
    if (ssid_idx) {
        InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
        _Dev2WifiSsidObject *ssidObj=NULL;
        iidStack.instance[0]=ssid_idx;
        iidStack.currentDepth=1;
        if ((cmsObj_get(MDMOID_DEV2_WIFI_SSID, &iidStack, 0, (void **) &ssidObj)) == CMSRET_SUCCESS) {
            return ssidObj;
        }
    }
    return NULL;
}


CmsRet rutWifi_get_AP_Radio_dev2(_Dev2WifiAccessPointObject  *apObj,void **radioObj,InstanceIdStack *iidStack) {

    iidStack->instance[0] = apObj->X_BROADCOM_COM_Adapter +1 ;
    iidStack->currentDepth=1;
    return cmsObj_get(MDMOID_DEV2_WIFI_RADIO, iidStack, 0, radioObj);
}


void rutWifi_Clear_AssocicatedDevices(Dev2WifiAccessPointObject *apObj,InstanceIdStack *iidStack) {
    InstanceIdStack adIidStack = EMPTY_INSTANCE_ID_STACK;
    Dev2WifiAssociatedDeviceObject *associatedDeviceObj=NULL;
    while ((cmsObj_getNextInSubTree(MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE,
                                    iidStack,
                                    &adIidStack,
                                    (void **) &associatedDeviceObj)) == CMSRET_SUCCESS) {
        cmsObj_deleteInstance(MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE, &adIidStack);
        INIT_INSTANCE_ID_STACK(&adIidStack);
        cmsObj_free((void **) &associatedDeviceObj);

    }
}

#ifndef BUILD_BRCM_UNFWLCFG
void rutWifi_AssociatedDeviceUpdated_dev2(WL_STALIST_SUMMARIES *sta_summaries,InstanceIdStack *on_apIidStack,
        _Dev2WifiAssociatedDeviceObject *sta)
{
    CmsRet ret;
    Dev2WifiAssociatedDeviceObject *associatedDeviceObj = NULL;
    InstanceIdStack *p_apIidStack, apIidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack adIidStack = EMPTY_INSTANCE_ID_STACK;
    WL_STATION_LIST_ENTRY *pStation = sta_summaries->stalist_summary;
    int i = 0;
    if(!on_apIidStack) {
        cmsLog_debug("to find apObject\n");
        INIT_INSTANCE_ID_STACK(&apIidStack);
        Dev2WifiAccessPointObject *apObj=NULL;
        rutWifi_find_AP_ByIndex_locked(sta_summaries->radioIdx, sta_summaries->ssidIdx, &apIidStack,(void **)&apObj);
        if (apObj == NULL)
        {
            cmsLog_error("Could not find AccessPointObj for associated device %s", pStation->macAddress);
            return;
        }
        p_apIidStack=&apIidStack;
        cmsObj_free((void **) &apObj);
    } else {
        p_apIidStack= on_apIidStack;
    }

    if(sta_summaries->type == UPDATE_STA_ALL) {
        sta_summaries=wlcsm_wl_get_sta_summary(sta_summaries->radioIdx,sta_summaries->ssidIdx);
        if(!sta_summaries)  {
            cmsLog_error("Could not get sta_summaries for radio:%d and ssid:%d\n",sta_summaries->radioIdx,sta_summaries->ssidIdx);
            return;
        }
    }

    if(sta_summaries->type == STA_CONNECTED || sta_summaries->type == UPDATE_STA_ALL) {
        cmsLog_debug( "To add or update stas\n");
        for (i = 0 ; i < sta_summaries->num_of_stas && pStation != NULL ; i++, pStation++)
        {
            INIT_INSTANCE_ID_STACK(&adIidStack);
            associatedDeviceObj = NULL;
            findStationByMacAddr_locked(pStation->macAddress, p_apIidStack, &adIidStack, (void **)&associatedDeviceObj);
            if (associatedDeviceObj == NULL) {
                // Add new instance
                cmsLog_debug( "could not find sta, need to add a new one");
                memcpy(&adIidStack, p_apIidStack, sizeof(InstanceIdStack));
                if ((ret = cmsObj_addInstance(MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE, &adIidStack) ) == CMSRET_SUCCESS)
                {
                    cmsObj_get(MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE, &adIidStack, 0, (void **)&associatedDeviceObj);
                    CMSMEM_REPLACE_STRING(associatedDeviceObj->MACAddress, pStation->macAddress);
                    associatedDeviceObj->active = pStation->associated;
                    associatedDeviceObj->authenticationState = pStation->authorized;
                    cmsObj_set(associatedDeviceObj, &adIidStack);
                    rutWifi_update_STA_HostEntry(associatedDeviceObj,&adIidStack,MDM_ACTIVE_ENTRY);
                    cmsObj_free((void **) &associatedDeviceObj);
                }
                else {
                    cmsLog_error("could not add _Dev2WifiAssociatedDeviceObject instance, ret=%d", ret);
                }
            }
            else  {
                // Update existed instance
                cmsLog_debug( "find sta and update it\n");
                associatedDeviceObj->active = pStation->associated;
                associatedDeviceObj->authenticationState = pStation->authorized;
                cmsObj_set(associatedDeviceObj, &adIidStack);
                rutWifi_update_STA_HostEntry(associatedDeviceObj,&adIidStack,MDM_ACTIVE_ENTRY);
                cmsObj_free((void **) &associatedDeviceObj);
            }
        }
    }

    if(sta_summaries->type== UPDATE_STA_ALL) {
        /* Delete non-existed associated devices */
        cmsLog_debug( "remove all idle stas\n");
        INIT_INSTANCE_ID_STACK(&adIidStack);
        while ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE, p_apIidStack, &adIidStack, (void **) &associatedDeviceObj)) == CMSRET_SUCCESS) {
            int found = 0;
            pStation = sta_summaries->stalist_summary;
            for (i = 0 ; i < sta_summaries->num_of_stas && pStation != NULL ; i++, pStation++) {
                if (!cmsUtl_strcmp(pStation->macAddress, associatedDeviceObj->MACAddress)) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                if(sta!=associatedDeviceObj) {
                    cmsObj_deleteInstance(MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE, &adIidStack);
                    INIT_INSTANCE_ID_STACK(&adIidStack);
                } else {
                    associatedDeviceObj->active = 0;
                    associatedDeviceObj->authenticationState = 0;
                    rutWifi_update_STA_HostEntry(associatedDeviceObj,&adIidStack,MDM_INVALID_ONLY);
                }
            }
            cmsObj_free((void **) &associatedDeviceObj);
        }

    } else if(sta_summaries->type== STA_DISCONNECTED) {

        pStation = sta_summaries->stalist_summary;
        for (i = 0 ; i < sta_summaries->num_of_stas && pStation != NULL ; i++, pStation++) {
            cmsLog_debug( "sta:%s disconnected,to remove it now\n",pStation->macAddress);
            INIT_INSTANCE_ID_STACK(&adIidStack);
            while ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE, p_apIidStack, &adIidStack, (void **) &associatedDeviceObj)) == CMSRET_SUCCESS)
            {
                if (!cmsUtl_strcmp(pStation->macAddress, associatedDeviceObj->MACAddress)) {
                    if(sta!=associatedDeviceObj) {
                        cmsObj_deleteInstance(MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE,&adIidStack);
                        INIT_INSTANCE_ID_STACK(&adIidStack);
                    }  else {
                        associatedDeviceObj->active = 0;
                        associatedDeviceObj->authenticationState = 0;
                        cmsObj_set(associatedDeviceObj, &adIidStack);
                    }
                    break;
                }
                cmsObj_free((void **)&associatedDeviceObj);
            }
        }
    }
    cmsLog_debug(" update STAs done!\n");
    return;
}
#endif  /* BUILD_BRCM_UNFWLCFG */

#if 1
void rutWifi_update_STA_DHCPV4_client(char *brname,Dev2WifiAssociatedDeviceObject *staObj,int op) {

    Dev2Dhcpv4ServerPoolObject *dhcpv4ServerPoolObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    char ifName[CMS_IFNAME_LENGTH]= {0};
    UBOOL8 found=FALSE;
    CmsRet ret = CMSRET_OBJECT_NOT_FOUND;;
    InstanceIdStack dhcp_client_iidStack=EMPTY_INSTANCE_ID_STACK;
    Dev2Dhcpv4ServerPoolClientObject *dhcp_clientObj = NULL;
    /* first to get dhcp server pool */
    while (found == FALSE &&
            cmsObj_getNext(MDMOID_DEV2_DHCPV4_SERVER_POOL, &iidStack,
                           (void **)&dhcpv4ServerPoolObj) == CMSRET_SUCCESS)
    {
        cmsLog_debug("....\n");
        if (qdmIntf_fullPathToIntfnameLocked_dev2(dhcpv4ServerPoolObj->interface, ifName) == CMSRET_SUCCESS &&
                cmsUtl_strcmp(ifName,brname) == 0)
        {

            found=TRUE;
            cmsLog_debug("...\n");
            INIT_INSTANCE_ID_STACK(&dhcp_client_iidStack);
            while ( cmsObj_getNextFlags(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT,
                                        &dhcp_client_iidStack,
                                        OGF_NO_VALUE_UPDATE,
                                        (void **)&dhcp_clientObj) == CMSRET_SUCCESS)
            {
                cmsLog_debug("....\n");
                if (!cmsUtl_strcasecmp(dhcp_clientObj->chaddr, staObj->MACAddress))
                {
                    cmsLog_debug("remote dhdcp client object as well\n");
                    if(op==MDM_REMOVE_ENTRY)
                        ret = cmsObj_deleteInstance(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT,&dhcp_client_iidStack);
                    else {
                        dhcp_clientObj->active=(op==MDM_ACTIVE_ENTRY)?1:0;
                        cmsObj_set(dhcp_clientObj,&dhcp_client_iidStack);
                    }
                    cmsObj_free((void **) &dhcp_clientObj);
                    break;
                }
                cmsObj_free((void **) &dhcp_clientObj);
            }
        }
        cmsObj_free((void **)&dhcpv4ServerPoolObj);
    }
}
#endif

CmsRet rutWifi_update_STA_HostEntry(Dev2WifiAssociatedDeviceObject *staObj,InstanceIdStack *sta_iidStack,int op)
{
    char *layer3Interface = NULL;
    UBOOL8 layer2 = FALSE, found = FALSE;
    Dev2HostObject *hostObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;
    _Dev2WifiAccessPointObject  *apObj;
    Dev2WifiSsidObject   *ssidObj;

    char *fullpath=NULL;
    MdmPathDescriptor pathDesc;
    INIT_PATH_DESCRIPTOR(&pathDesc);
    memcpy(&(pathDesc.iidStack),sta_iidStack,sizeof(InstanceIdStack));
    pathDesc.oid=staObj->_oid;

    cmsLog_debug("remote staObj:%s\n",staObj->MACAddress);
    if((ret=cmsObj_getAncestor(MDMOID_DEV2_WIFI_ACCESS_POINT,
                               MDMOID_DEV2_WIFI_ASSOCIATED_DEVICE,
                               sta_iidStack,(void **)&apObj))!=CMSRET_SUCCESS)
    {
        cmsLog_error("Fail to get AP object by staObj, ret=%d", ret);
        return ret;
    }
    ssidObj= rutWifi_get_AP_SSID_dev2(apObj);
    cmsObj_free((void **)&apObj);

    if(!ssidObj) {
        cmsLog_error("could not ger refered ssid of obj");
        return ret;
    }

    cmsLog_debug("ssidObj->name:%s\n",ssidObj->name);
    cmsLog_debug("staObj->MACAddress:%s\n",staObj->MACAddress);

    ret = qdmIntf_intfnameToFullPathLocked_dev2(ssidObj->X_BROADCOM_COM_WlBrName, layer2, &layer3Interface);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not find layer 3 full path name for IP interface %s", ssidObj->name);
        cmsObj_free((void **)&ssidObj);
        return CMSRET_INVALID_ARGUMENTS;
    }

    if(layer3Interface) {
        cmsLog_debug("....\n");
    } else {
        cmsLog_debug("....\n");
        cmsObj_free((void **)&ssidObj);
        return CMSRET_INVALID_ARGUMENTS;
    }


    while (found == FALSE &&
            (ret = cmsObj_getNext(MDMOID_DEV2_HOST, &iidStack, (void **)&hostObj)) == CMSRET_SUCCESS)
    {

        cmsLog_debug("....\n");
        if (cmsUtl_strcasecmp(hostObj->layer3Interface, layer3Interface) == 0 &&
                cmsUtl_strcasecmp(hostObj->physAddress, staObj->MACAddress) == 0)
        {
            found = TRUE;
            cmsLog_debug("Delete Dev2HostObject for layer3Interface=%s, macAddr=%s",
                         layer3Interface,staObj->MACAddress );
            cmsLog_debug(".....");
            if(op==MDM_REMOVE_ENTRY)
                cmsObj_deleteInstance(MDMOID_DEV2_HOST, &iidStack);
            else {

                hostObj->active=(op==MDM_ACTIVE_ENTRY)?1:0;
                if(hostObj->active) {
                    if(cmsMdm_pathDescriptorToFullPath(&pathDesc, &fullpath)== CMSRET_SUCCESS) {
                        /* remove last . */
                        if(fullpath[strlen(fullpath)-1]=='.') fullpath[strlen(fullpath)-1]='\0';
                        REPLACE_STRING_IF_NOT_EQUAL_FLAGS(hostObj->associatedDevice,fullpath,mdmLibCtx.allocFlags);
                        cmsMem_free(fullpath);
                    }
                } else {
                    if(hostObj->associatedDevice)
                        cmsMem_free(hostObj->associatedDevice);
                    hostObj->associatedDevice=NULL;
                }
                cmsObj_set(hostObj,&iidStack);
            }
            if(op!=MDM_ACTIVE_ENTRY)
                rutWifi_update_STA_DHCPV4_client(ssidObj->X_BROADCOM_COM_WlBrName,staObj,op);
        }

        cmsObj_free((void **)&hostObj);
    }

    cmsObj_free((void **)&ssidObj);
    cmsLog_debug("....\n");
    CMSMEM_FREE_BUF_AND_NULL_PTR(layer3Interface);

    return ret;
}


void rutWifi_find_AP_ByIndex_locked(int radioIndex, int ssidIndex, InstanceIdStack *iidStack, void **obj)
{
    CmsRet ret = CMSRET_SUCCESS;
    _Dev2WifiAccessPointObject *apObj = NULL;

    while ((ret = cmsObj_getNext(MDMOID_DEV2_WIFI_ACCESS_POINT, iidStack, (void **)&apObj )) == CMSRET_SUCCESS)
    {
        if (apObj->X_BROADCOM_COM_Index == ssidIndex && apObj->X_BROADCOM_COM_Adapter == radioIndex)
        {
            cmsLog_debug("find access point ID:%d/%d", apObj->X_BROADCOM_COM_Adapter,apObj->X_BROADCOM_COM_Index);
            break;
        }
        cmsObj_free((void **) &apObj);
    }

    if (apObj != NULL)
        *((_Dev2WifiAccessPointObject **)obj) = apObj;
}

#endif  /* DMP_DEVICE2_WIFIACCESSPOINT_1 */

#ifdef DESKTOP_LINUX
static void writeNvram_desktop(const char *configStr);
#endif

UBOOL8 rutWifi_getWlanSsidObjByIfName(const char *ifName,
                                      Dev2WifiSsidObject **ssidObj,
                                      InstanceIdStack *iidStack)
{
    UBOOL8 found=FALSE;

    while (!found &&
            cmsObj_getNextFlags(MDMOID_DEV2_WIFI_SSID, iidStack, OGF_NO_VALUE_UPDATE, (void **) ssidObj) == CMSRET_SUCCESS)
    {
        if ( !cmsUtl_strcmp((*ssidObj)->name, ifName))
            found = TRUE;
        else
            cmsObj_free((void **) ssidObj);
    }
    return found;
}

void rutWifi_writeNvram(const char *configStr)
{
#ifdef DESKTOP_LINUX
    writeNvram_desktop(configStr);
    return;
#endif

    // cmsLog_error("not implemented yet, configStr=%s", configStr);
    return;
}

#ifndef BUILD_BRCM_UNFWLCFG
CmsRet rutWifi_updateWlmngr(MdmObjectId oid, UINT32 radioIndex, UINT32 secondIndex, UINT32 thirdIndex, const paramNodeList *changedParams)
{
    paramNodeList *tmpParamNode;
    UINT32 isValid = 0, subIndex=0, ret;

    subIndex = secondIndex + (thirdIndex << 16);
    cmsLog_debug("SUBINDEX=%08x\n", __func__, subIndex);

    for (tmpParamNode = changedParams ; tmpParamNode != NULL ; tmpParamNode = tmpParamNode->nextNode)
    {
        ret = wlcsm_mngr_dm_validate(radioIndex, subIndex, oid, tmpParamNode->offset, tmpParamNode->value);
        isValid = !ret;
        if (ret != 0)
        {
            cmsLog_error("wlcsm_mngr_dm_set error, ret=%d", ret);
            break;
        }
    }

    if (isValid)
    {
        if (mdmLibCtx.eid == EID_TR69C || mdmLibCtx.eid == EID_CWMPD)
            wlcsm_mngr_restart(radioIndex,WLCSM_MNGR_RESTART_TR69C,WLCSM_MNGR_RESTART_NOSAVEDM,0);
        else
            wlcsm_mngr_restart(radioIndex,WLCSM_MNGR_RESTART_MDM,WLCSM_MNGR_RESTART_NOSAVEDM,0);
        return CMSRET_SUCCESS;
    }
    else
        return CMSRET_INVALID_PARAM_VALUE;
}
#endif

static int isDeviceFound(const char *devName)
{
   int count = 0;
   char *pChar = NULL;
   char line[BUFLEN_512], buf[BUFLEN_512];
   char *pcDevNameStart;

   /* getstats put device statistics into this file, read the stats */
   /* Be sure to read the page with the extended stats */
   FILE* fs = fopen("/proc/net/dev", "r");
   if ( fs == NULL ) 
   {
      return 0;
   }

   // find interface
   while ( fgets(line, sizeof(line), fs) ) 
   {
      /* read pass 2 header lines */
      if ( count++ < 2 ) 
      {
         continue;
      }

      /* normally line will have the following example value
       * "eth0: 19140785 181329 0 0 0 0 0 0 372073009 454948 0 0 0 0 0 0"
       * but when the number is too big then line will have the following example value
       * "eth0:19140785 181329 0 0 0 0 0 0 372073009 454948 0 0 0 0 0 0"
       * so to make the parsing correctly, the following codes are added
       * to insert space between ':' and number
       */
      pChar = strchr(line, ':');
      if ( pChar != NULL )
      {
         pChar++;
      }
      if ( pChar != NULL && isdigit(*pChar) ) 
      {
         strcpy(buf, pChar);
         *pChar = ' ';
         strcpy(++pChar, buf);
      }
	  
      /* Find and test the interface name to see if it's the one we want.
         If so, then store statistic values.	  */
      pcDevNameStart = strstr(line, devName);
      if ( (pcDevNameStart != NULL) && *(pcDevNameStart + strlen(devName)) == ':' )
      {
          fclose(fs);
          return 1;
      }
   }

   fclose(fs);
   return 0;
}


CmsRet rutWifi_getRadioCounters(const char *devName, struct RadioCounters *rCounters)
{
    char cmdBuf[BUFLEN_128] = {0};
    FILE *fp = NULL;

    if (!rCounters || !devName || devName[0]!='w')
        return CMSRET_INVALID_ARGUMENTS;

    if (!isDeviceFound(devName))
        return CMSRET_INVALID_ARGUMENTS; 

    sprintf(cmdBuf, "wlctl -i %s counters > /var/%scounters", devName, devName);
    rut_doSystemAction("rutWifi", cmdBuf);
    sprintf(cmdBuf, "/var/%scounters", devName);
    fp = fopen(cmdBuf, "r");
    if (fp)
    {
        char buf[BUFLEN_1024];
        while(fgets(buf, 1024, fp))
        {
            char *ptr;
            if ((ptr = strstr(buf, "rxbadplcp"))!= NULL)
                sscanf(ptr+10, "%d", &(rCounters->PLCPErrorCount));
            else if ((ptr = strstr(buf, "rxbadfcs")) != NULL)
                sscanf(ptr+9, "%d", &(rCounters->FCSErrorCount));
            else if ((ptr = strstr(buf, "rxbadproto")) != NULL)
                sscanf(ptr+11, "%d", &(rCounters->invalidMACCount));
            else if ((ptr = strstr(buf, "rxbadda")) != NULL)
                sscanf(ptr+8, "%d", &(rCounters->packetsOtherReceived));
        }
        fclose(fp);
    }
    unlink(cmdBuf);
    return CMSRET_SUCCESS;
}

CmsRet rutWifi_getSSIDCounters(const char *devName, struct SSIDCounters *sCounters)
{
    char cmdBuf[BUFLEN_128] = {0};
    FILE *fp = NULL;

    if (!sCounters || !devName || devName[0]!='w')
        return CMSRET_INVALID_ARGUMENTS;

    if (!isDeviceFound(devName))
        return CMSRET_INVALID_ARGUMENTS; 

    sprintf(cmdBuf, "wlctl -i %s counters > /var/%scounters", devName, devName);
    rut_doSystemAction("rutWifi", cmdBuf);
    sprintf(cmdBuf, "/var/%scounters", devName);
    fp = fopen(cmdBuf, "r");
    if (fp)
    {
        char buf[BUFLEN_1024];
        while(fgets(buf, 1024, fp))
        {
            char *ptr;
            if ((ptr = strstr(buf, "txretrans"))!= NULL)
                sscanf(ptr+10, "%d", &(sCounters->retransCount));
            if ((ptr = strstr(buf, "txfail")) != NULL)
                sscanf(ptr+7, "%d", &(sCounters->failedRetransCount));
            if ((ptr = strstr(buf, "d11_txretry")) != NULL)
                sscanf(ptr+12, "%d", &(sCounters->retryCount));
            if ((ptr = strstr(buf, "d11_txretrie")) != NULL)
                sscanf(ptr+13, "%d", &(sCounters->multipleRetryCount));
            if ((ptr = strstr(buf, "d11_txnoack")) != NULL)
                sscanf(ptr+12, "%d", &(sCounters->ACKFailureCount));
            if ((ptr = strstr(buf, "txampdu")) != NULL)
                sscanf(ptr+8, "%d", &(sCounters->aggregatedPacketCount));
        }
        fclose(fp);
    }
    unlink(cmdBuf);
    return CMSRET_SUCCESS;
}


#ifdef DESKTOP_LINUX
#define WIFI_DESKTOP_NVRAM  "wifi-nvram.txt"

static void writeNvram_desktop(const char *configStr)
{
    FILE *fp;
    size_t count;
    UINT32 len;

    len = cmsUtl_strlen(configStr);
    if (len == 0)
    {
        cmsLog_error("configStr is NULL or 0 len");
        return;
    }

    fp = fopen(WIFI_DESKTOP_NVRAM, "a+");
    if (fp == NULL)
    {
        cmsLog_error("open of %s failed", WIFI_DESKTOP_NVRAM);
        return;
    }

    count = fwrite(configStr, len, 1, fp);
    if (count != (size_t) 1)
    {
        cmsLog_error("fwrite error, got %d expected %d", (int)count, 1);
    }

    /* for desktop only: write a newline (real nvram does not need it?) */
    count = fwrite("\n", 1, 1, fp);
    if (count != 1)
    {
        cmsLog_error("fwrite of newline failed!");
    }

    fclose(fp);
}
#endif


#endif  /* DMP_DEVICE2_WIFIRADIO_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */

