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

#ifdef BRCM_WLAN

/*
 * The first part of this file contains functions used by both the TR98
 * and TR181 implementations of Wifi.  Wifi code used by both TR98 and
 * TR181 data model code is surrounded by BRCM_WLAN
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <netinet/if_ether.h>
#include <ctype.h>

#include <sys/stat.h>
#include <fcntl.h>

#include "board.h"

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include "cms.h"
#include "cms_util.h"
#include "mdm.h"
#include "mdm_private.h"
#include "mdm_initwlan.h"
#include "wlsysutil.h"

#define GUEST2_BSS_IDX 3
#ifdef DMP_X_BROADCOM_COM_RDPA_1
#ifndef G9991
#define RDPA_WLAN0_MASK_INX 30
#else
#define RDPA_WLAN0_MASK_INX 38 
#endif
#define RDPA_ETHTYPE_UDEF_VAL 0x888E
#endif


#ifdef DMP_WIFILAN_1

/*
 * The functions in the rest of this file are specific to TR89 data model.
 * The Wifi subsystem really needs both the TR98 and X_BROADCOM_COM wifi
 * objects, so it is not possible to just define DMP_WIFILAN_1 and not
 * X_BROADCOM_COM_WIFILAN_1 and expect things to work.
 */

extern CmsRet addDefaultL2BridgingAvailableInterfaceObject(const char *interfaceReference, SINT32 bridgeRef);



#if defined(SUPPORT_IEEE1905) && !defined(DMP_ADSLWAN_1) && !defined(DMP_X_BROADCOM_COM_GPONWAN_1) && !defined(DMP_X_BROADCOM_COM_EPONWAN_1)
CmsRet setDefaultWscConfigObject( int objId )
{
   MdmPathDescriptor pathDesc;
   CmsRet ret = CMSRET_SUCCESS;
   _WlWpsCfgObject *virtWpsObj=NULL;
   int WlMaxNumSsid = wlgetVirtIntfNo(objId-1);   
   int index = 0;
   
   for ( ; index < WlMaxNumSsid; index++ ) {

       INIT_PATH_DESCRIPTOR(&pathDesc);

       pathDesc.oid = MDMOID_WL_WPS_CFG;
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), index + 1);

       ret = mdm_getObject(MDMOID_WL_WPS_CFG, &(pathDesc.iidStack), (void **) &virtWpsObj);
       if (ret != CMSRET_SUCCESS)
       {
              cmsLog_error("getObject failed,  ret=%d", ret);
              return ret;
       }

       /* set the object enable */
       CMSMEM_REPLACE_STRING_FLAGS(virtWpsObj->wsc_mode, "enabled", mdmLibCtx.allocFlags );
#if defined(SUPPORT_IEEE1905_GOLDENNODE) && defined(SUPPORT_IEEE1905_REGISTRAR)
       CMSMEM_REPLACE_STRING_FLAGS(virtWpsObj->wsc_config_state, "1", mdmLibCtx.allocFlags );
#else
       CMSMEM_REPLACE_STRING_FLAGS(virtWpsObj->wsc_config_state, "0", mdmLibCtx.allocFlags );
#endif

       ret = mdm_setObject((void **) &virtWpsObj, &(pathDesc.iidStack), FALSE);
       if (ret != CMSRET_SUCCESS)
       {
              cmsLog_error("setObject failed, ret=%d", ret);
              return ret;
       }

   }
   return ret;
}
#endif

CmsRet addDefaultVirtMbssObject( int objId)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret = CMSRET_SUCCESS;
   _LanWlanVirtMbssObject *virtMbssObj=NULL;
   int WlMaxNumSsid = wlgetVirtIntfNo(objId-1);   
   int i=0;

    for ( i=0; i<(WlMaxNumSsid-1-GUEST2_BSS_IDX); i++ ) {
       INIT_PATH_DESCRIPTOR(&pathDesc);

       pathDesc.oid = MDMOID_LAN_WLAN_VIRT_MBSS;
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
       PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);

       if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
       {
              cmsLog_error("addObjectInstance failed, ret=%d", ret);
              return ret;
       }

       if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &virtMbssObj)) != CMSRET_SUCCESS)
       {
              cmsLog_error("getObject failed,  ret=%d", ret);
              return ret;
       }

       /* set the object enable */   
       virtMbssObj->enable = TRUE;       
       if ((ret = mdm_setObject((void **) &virtMbssObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
       {
              cmsLog_error("setObject failed, ret=%d", ret);
              return ret;
       }
   }
   return ret;
}

CmsRet addDefaultWepkeyObject( int objId)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   _LanWlanWepKeyObject *wepkeyObj=NULL;
   
   int i=0;

    for ( i=0; i<4; i++ ) {
	   INIT_PATH_DESCRIPTOR(&pathDesc);
	   
	   pathDesc.oid = MDMOID_LAN_WLAN_WEP_KEY;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);

	   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
	   {
		      cmsLog_error("mdm_addObjectInstance failed ret=%d", ret);
		      return ret;
	   }

	   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &wepkeyObj)) != CMSRET_SUCCESS)
	   {
		      cmsLog_error("mdm_getObject ret=%d", ret);
		      return ret;
	   }

	   
	   if ((ret = mdm_setObject((void **) &wepkeyObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
	   {
		      cmsLog_error("mdm_setObject failed ret=%d", ret);
		      return ret;
	   }

   }
   return ret;
}

CmsRet addDefaultPreSharedKeyObject( int objId)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   _LanWlanPreSharedKeyObject *preShredKeyObj=NULL;
   
   int i=0;

   for ( i=0; i<1; i++ ) 
   {
      INIT_PATH_DESCRIPTOR(&pathDesc);

      pathDesc.oid = MDMOID_LAN_WLAN_PRE_SHARED_KEY;
      PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
      PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_addObjectInstance failed ret=%d", ret);
         return ret;
      }

      if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &preShredKeyObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_getObject failed ret=%d", ret);
         return ret;
      }

      if ((ret = mdm_setObject((void **) &preShredKeyObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("mdm_setObject failed ret=%d", ret);
         return ret;
      }
   }
   return ret;
}

CmsRet addDefaultIntfWepkeyObject( int objId1, int objId2)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   _WlKey64CfgObject *wepkey64Obj=NULL;
   _WlKey128CfgObject *wepkey128Obj=NULL;
   
   int i=0;

	// Add WepKey64 instance
	   for ( i=0; i<4; i++ ) {
		   INIT_PATH_DESCRIPTOR(&pathDesc);
		   
		   pathDesc.oid = MDMOID_WL_KEY64_CFG;
		   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
		   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId1);
		   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId2);

		   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
		   {
		      cmsLog_error("mdm_addObjectInstance failed ret=%d", ret);
		      return ret;
		   }

		   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &wepkey64Obj)) != CMSRET_SUCCESS)
		   {
		      cmsLog_error("mdm_getObject failed ret=%d", ret);
		      return ret;
		   }

		   if ((ret = mdm_setObject((void **) &wepkey64Obj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
		   {
		      cmsLog_error("mdm_setObject failed ret=%d", ret);
		      return ret;
		   }

	}


	// Add WepKey128 instance 
	   for ( i=0; i<4; i++ ) {
		   INIT_PATH_DESCRIPTOR(&pathDesc);
		   
		   pathDesc.oid = MDMOID_WL_KEY128_CFG;
		   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);
		   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId1);
		   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId2);

		   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
		   {
		      cmsLog_error("mdm_addObjectInstance failed ret=%d", ret);
		      return ret;
		   }

		   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &wepkey128Obj)) != CMSRET_SUCCESS)
		   {
		      cmsLog_error("mdm_getObject failed ret=%d", ret);
		      return ret;
		   }


		   if ((ret = mdm_setObject((void **) &wepkey128Obj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
		   {
		      cmsLog_error("mdm_setObject failed ret=%d", ret);
		      return ret;
		   }

	   }
	   return ret;
}

static CmsRet addNewIntfObject( int objId, int startNum)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret = CMSRET_SUCCESS;
   _WlVirtIntfCfgObject *intfObj=NULL;
   int WlMaxNumSsid = wlgetVirtIntfNo(objId-1);

   int j;
   char ifcnameBuf[16], ssidBuf[48];

    for ( j=startNum; j<WlMaxNumSsid; j++ ) {
	   INIT_PATH_DESCRIPTOR(&pathDesc);
	   
	   pathDesc.oid = MDMOID_WL_VIRT_INTF_CFG;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1);     /* LANDevice.1 */
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); /* WLANConfiguration.objId */

	   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
	   {
		      cmsLog_error("mdm_addObjectInstance failed ret=%d", ret);
		      return ret;
	   }
	   
	   /* get the object we just created */
	   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &intfObj)) != CMSRET_SUCCESS)
	   {
		      cmsLog_error("mdm_getObject failed ret=%d", ret);
		      return ret;
	   }

	   /* set the intf name and enable */   
	 if ( j ==0 ) {
		intfObj->wlEnblSsid = 1;
		intfObj->wlIdx = j;
		sprintf(ssidBuf, "BrcmAP%d", objId-1 );
	 	CMSMEM_REPLACE_STRING_FLAGS( intfObj->wlSsid, ssidBuf, mdmLibCtx.allocFlags );
		sprintf(ifcnameBuf, "wl%d",objId-1 );
	    	CMSMEM_REPLACE_STRING_FLAGS( intfObj->wlIfcname, ifcnameBuf, mdmLibCtx.allocFlags );
	 }
	 else {
		intfObj->wlEnblSsid = 1;
		intfObj->wlIdx = j;
		sprintf(ssidBuf, "wl%d_Guest%d", objId-1, j );
	 	CMSMEM_REPLACE_STRING_FLAGS( intfObj->wlSsid, ssidBuf, mdmLibCtx.allocFlags );
		sprintf(ifcnameBuf, "wl%d.%d",objId-1, j );
	    	CMSMEM_REPLACE_STRING_FLAGS( intfObj->wlIfcname, ifcnameBuf, mdmLibCtx.allocFlags );
		intfObj->wlEnblSsid = 0; //disable Guest SSID
	 }
	    //wlBssMacAddr should be set
	   if ((ret = mdm_setObject((void **) &intfObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
	   {
		      cmsLog_error("mdm_setObject failed ret=%d", ret);
		      return ret;
	   }

#ifdef SUPPORT_QOS
      {
         char *wlanPath=NULL;
         cmsMdm_pathDescriptorToFullPath(&pathDesc, &wlanPath);
         /* Annoying TR-98 format: remove the last . */
         wlanPath[strlen(wlanPath)-1] = '\0';

         /* setup default queues for this wireless interfaces */
         /* In TR98, the default QoS Queues are created in the disabled
          * state, but later enabled by wlmngr.
          */
         ret = wladdDefaultWlanQueueObject(wlanPath, FALSE);
         cmsMem_free(wlanPath);

         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("wladdDefaultWlanQueueObject returns error. ret=%d", ret);
            return ret;
         }
      }
#endif

	  ret = addDefaultIntfWepkeyObject( objId, j+1);
	   if ( ret != CMSRET_SUCCESS) {
		      cmsLog_error("addDefaultIntfWepkeyObject failed ret=%d", ret);
		      return ret;
	   }

   }
   return ret;
}

static CmsRet addDefaultIntfObject( int objId)
{
   CmsRet ret;
   int startNum = 0;
   
   ret = addNewIntfObject(objId, startNum);
   return ret;
}

CmsRet addDefaultWlStaticWdsObject( int objId)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;
   _WlStaticWdsCfgObject *staticWdsObj=NULL;
   
   int j=0;

   
    for ( j=0; j<4; j++ ) {
	   INIT_PATH_DESCRIPTOR(&pathDesc);
	   
	   pathDesc.oid = MDMOID_WL_STATIC_WDS_CFG;
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
	   PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 


	   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
	   {
	         cmsLog_error("mdm_addObjectInstance failed ret=%d", ret);
	         return ret;
	   }
	   
	   /* get the object we just created */
	   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &staticWdsObj)) != CMSRET_SUCCESS)
	   {
	         cmsLog_error("mdm_getObject failed ret=%d", ret);
	         return ret;
	   }

	   /* set the intf name and enable */   
	
	 	CMSMEM_REPLACE_STRING_FLAGS( staticWdsObj->wlMacAddr, "", mdmLibCtx.allocFlags );

	   if ((ret = mdm_setObject((void **) &staticWdsObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
	   {
	         cmsLog_error("mdm_setObject failed ret=%d", ret);
		  return ret;
	   }
   }
   return ret;
}


CmsRet addDefaultWlanInterfaceObject(const char *ifName, int objId)
{
   MdmPathDescriptor pathDesc;
   _LanWlanObject *wlanObj=NULL;
   CmsRet ret;

#ifdef WL_INITMDM_DBG
   printf("\n\n adding WLAN Object[%d]\n\n", objId);
#endif

// create WLAN object
   INIT_PATH_DESCRIPTOR(&pathDesc);
   
   pathDesc.oid = MDMOID_LAN_WLAN;
   PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); /* default wlan interface always under LANDevice.1 */

   if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_addObjectInstance failed ret=%d", ret);
      return ret;
   }
   
   /* get the object we just created */
   if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **) &wlanObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_getObject failed ret=%d", ret);
      return ret;
   }

   /* set the intf name and enable */   
   wlanObj->enable = TRUE;
   CMSMEM_REPLACE_STRING_FLAGS(wlanObj->X_BROADCOM_COM_IfName, ifName, mdmLibCtx.allocFlags);
   
   if ((ret = mdm_setObject((void **) &wlanObj, &(pathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
   {
      cmsLog_error("mdm_setObject failed ret=%d", ret);
      return ret;
   }


//create 4 wekkey object
   ret = addDefaultWepkeyObject( objId );
   if ( ret != CMSRET_SUCCESS)  {
      cmsLog_error("addDefaultWepkeyObject failed ret=%d", ret);
      return ret;
   }

//create 1 PreSharedKey object
   ret = addDefaultPreSharedKeyObject( objId );
   if ( ret != CMSRET_SUCCESS)  {
      cmsLog_error("addDefaultPreSharedKeyObject failed ret=%d", ret);
      return ret;
   }

// Create Virtula intf
   ret = addDefaultIntfObject( objId );

   if ( ret != CMSRET_SUCCESS) {
      cmsLog_error("addDefaultIntfObject failed ret=%d", ret);
      return ret;
   }
//create Static WDS table
   if ( (ret =addDefaultWlStaticWdsObject( objId )) != CMSRET_SUCCESS ) {
	     cmsLog_error("addDefaultWlStaticWdsObject failed ret=%d", ret);
	     return ret;
   }
   
//create N virt mbss object
   ret = addDefaultVirtMbssObject( objId );
   if ( ret != CMSRET_SUCCESS)  {
      cmsLog_error("addDefaultVirtMbssObject failed ret=%d", ret);
      return ret;
   }
#if defined(SUPPORT_IEEE1905) && !defined(DMP_ADSLWAN_1) && !defined(DMP_X_BROADCOM_COM_GPONWAN_1) && !defined(DMP_X_BROADCOM_COM_EPONWAN_1)
//create unconfigured WSC object
   ret = setDefaultWscConfigObject( objId );
   if ( ret != CMSRET_SUCCESS)  {
      cmsLog_error("setDefaultWscConfigObject failed ret=%d", ret);
      return ret;
   }
#endif   

   return ret;
}

CmsRet addIntfToBridge(int startwlCnt, int startNumSSid)
{
#ifdef DMP_BRIDGING_1 /* aka SUPPORT_PORT_MAP */
   InstanceIdStack iidStack;
   L2BridgingObject *l2BridgingObj=NULL;
   char nameBuf[BUFLEN_1024];
   CmsRet ret;
   int  wl_cnt = wlgetintfNo();
   int j;
   int num_ssid;
   int virtIntfNo = 0;

    /*
    * If there is not a Layer2Bridging.Bridge object, create it, along
    * with the intial set of filter and available interfaces.
    */
      INIT_INSTANCE_ID_STACK(&iidStack);
      if ((ret = mdm_getObject(MDMOID_L2_BRIDGING, &iidStack, (void **) &l2BridgingObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get LAYER2 BRIDGING object, ret=%d", ret);
         return ret;
      }

      for ( j=startwlCnt; j<=wl_cnt; j++ ) {
          virtIntfNo = wlgetVirtIntfNo(j-1); 
          /* For every virtual ssid port*/
          for ( num_ssid=startNumSSid; num_ssid<= virtIntfNo; num_ssid++ ) {
             SINT32 bridgeRef;

        /*
         * By default, only first ssid is enabled, all others are disabled,
         * so bridgeRef for the disabled ones should be -1.
         */
              bridgeRef = (num_ssid == 1) ? 0 : -1;
      
             sprintf(nameBuf, "InternetGatewayDevice.LANDevice.1.WLANConfiguration.%d.X_BROADCOM_COM_WlanAdapter.WlVirtIntfCfg.%d", 
                            j, num_ssid);
             if ((ret = addDefaultL2BridgingAvailableInterfaceObject(nameBuf, bridgeRef)) != CMSRET_SUCCESS)
             {
                mdm_freeObject((void **)&l2BridgingObj);
                cmsLog_error("addDefaultL2BridgingAvailableInterfaceObject ERROR[%d]", ret);
                return ret;
             }
             l2BridgingObj->filterNumberOfEntries++;
             l2BridgingObj->availableInterfaceNumberOfEntries++;
          }
      }

      if ((ret = mdm_setObject((void **) &l2BridgingObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
      {
         mdm_freeObject((void **)&l2BridgingObj);
         cmsLog_error("could not set Layer2 Bridging object, ret=%d", ret);
      }

      return ret;
#else  /* DMP_BRIDGING_1  aka SUPPORT_PORT_MAP */
    return CMSRET_SUCCESS;
#endif
}

static CmsRet getUninitVirtIntfInstanceNum( int objId, int *currNum, int *addNum)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret = CMSRET_SUCCESS;
   _WlVirtIntfCfgObject *intfObj=NULL;
   int WlMaxNumSsid = wlgetVirtIntfNo(objId-1);
   int j=0;

   for ( j=0; j<WlMaxNumSsid; j++ ) {

      INIT_PATH_DESCRIPTOR(&pathDesc);

      pathDesc.oid = MDMOID_WL_VIRT_INTF_CFG;
      PUSH_INSTANCE_ID(&(pathDesc.iidStack), 1); 
      PUSH_INSTANCE_ID(&(pathDesc.iidStack), objId); 
      PUSH_INSTANCE_ID(&(pathDesc.iidStack), j+1); 

      if ((ret = mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **)&intfObj)) != CMSRET_SUCCESS)
      { 
         *currNum = j;
         *addNum = WlMaxNumSsid - j;
         break;
      }

      mdm_freeObject((void **)&intfObj);
   }

   return ret;
}

/*
 When virtue Mbss interface added from 4 up to 8 or 8 more, after new image is upgraded,
 new added virtue interface object instances need be added dynamically. Otherwise console
 will show some message and new mbss interface can't be taken effect unless using 'Restore 
 Default' from GUI.
 Some customer would not like to accept it. so function addNewVirtIntfInstance() is created for it. 
*/
static CmsRet addNewVirtIntfInstance(int wlCnt)
{
    CmsRet ret = CMSRET_SUCCESS;
    int j;
    int addNum = 0;
    int currNum = 0;

    for ( j=1; j<=wlCnt; j++ ) {

        if ((ret = getUninitVirtIntfInstanceNum(j, &currNum, &addNum)) != CMSRET_SUCCESS) {
            addNewIntfObject(j, addNum);
            addIntfToBridge(j, currNum+1);
            addDefaultVirtMbssObject(j);
        }
    }

    return ret;
}

CmsRet mdm_adjustWlanAdapter(void)
{
   InstanceIdStack iidStack;
   LanDevObject *lanDevObj=NULL;
   int i;
   char nameBuf[BUFLEN_1024];
   CmsRet ret;
   int  mdm_wl_cnt =0;
   int  wl_cnt = wlgetintfNo();
#ifdef DMP_BRIDGING_1 
   int num_ssid=1;
#endif

   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = mdm_getNextObject(MDMOID_LAN_DEV, &iidStack, (void **) &lanDevObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get LAN DEV object, ret=%d", ret);
      return ret;
   }

   mdm_wl_cnt = lanDevObj->LANWLANConfigurationNumberOfEntries;

   cmsLog_debug("mdm_wl_cnt=%d wl_cnt=%d\n", mdm_wl_cnt, wl_cnt);

   if (mdm_wl_cnt >= wl_cnt ) {
      addNewVirtIntfInstance(wl_cnt);
      mdm_freeObject((void **) &lanDevObj);
      return CMSRET_SUCCESS;
   }

   for ( i=mdm_wl_cnt; i<wl_cnt; i++ ) {
      sprintf(nameBuf, "wl%d", i);

      if ((ret = addDefaultWlanInterfaceObject( nameBuf, i+1) ) != CMSRET_SUCCESS)
      {
         mdm_freeObject((void **) &lanDevObj);
         return ret;
      }
   }

   lanDevObj->LANWLANConfigurationNumberOfEntries = wl_cnt;

   if ((ret = mdm_setObject((void **) &lanDevObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
   {
      mdm_freeObject((void **) &lanDevObj);               
      cmsLog_error("could not set LAN Dev object, ret=%d", ret);
   }

#ifdef DMP_BRIDGING_1 
   ret = addIntfToBridge(mdm_wl_cnt+1, num_ssid);
#endif
   return ret;
}

#endif  /* DMP_WIFILAN_1 */



#ifdef SUPPORT_QOS
/* Create initial queues associated with wlan adapter.  This function is
 * used by both TR98 and TR181 code.
 */
CmsRet wladdDefaultWlanQueueObject(const char *wlanPath, UBOOL8 enable)
{
   UINT32 qid;
   UINT32 precedence;
   char *qName;
   CmsRet ret;

   cmsLog_debug("Entered: wlanPath=%s", wlanPath);

   #ifdef WL_INITMDM_DBG
   printf("\nadding WLAN Queue Object for wlanPath %s\n", wlanPath);
   #endif

      /* create 8 queue objects for the default WLAN interface */
      for (precedence = 1; precedence <= 8; precedence++)
      {
         /* queueId calculation */
         qid = 1 + 8 - precedence;

         /* queueName */
         if (precedence == 6 || precedence == 7)
         {
            qName = "WMM Background";
         }
         else if (precedence == 3 || precedence == 4)
         {
            qName = "WMM Video Priority";
         }
         else if (precedence == 1 || precedence == 2)
         {
            qName = "WMM Voice Priority";
         }
         else
         {
            qName = "WMM Best Effort";
         }
   
         ret = mdmInit_addQosQueue(precedence, qid, wlanPath, qName, enable);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("rutQos_createQueue failed at precedence %d, ret=%d",
                         precedence, ret);
            return ret;
         }
      }
   return ret;

}
#endif  /* SUPPORT_QOS */



#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1
CmsRet addDefaultWanWifiObject(void)
{
   void *mdmObj=NULL;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _WanCommonIntfCfgObject *wanCommonObj = NULL;
   UINT32 added = 0;
   CmsRet ret;

   /*
    * User has selected Wifi as a WAN interface.  See if there is aleady an Wifi
    * WAN device.  If not, create it at the fixed instance number.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   PUSH_INSTANCE_ID(&iidStack, CMS_WANDEVICE_WIFI);
   ret = mdm_getObject(MDMOID_WAN_DEV, &iidStack, &mdmObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_notice("adding Wifi WANDevice");
      added++;

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_WAN_DEV;
      PUSH_INSTANCE_ID(&pathDesc.iidStack, CMS_WANDEVICE_WIFI);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not add WANDevice at %s, ret=%d", ret, cmsMdm_dumpIidStack(&pathDesc.iidStack));
         return ret;
      }

      if ((ret = mdm_getObject(MDMOID_WAN_COMMON_INTF_CFG, &iidStack, (void **) &wanCommonObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get WanCommonIntfCfgObject, ret=%d", ret);
         return ret;
      }

      CMSMEM_REPLACE_STRING_FLAGS(wanCommonObj->WANAccessType, MDMVS_X_BROADCOM_COM_WIFI, mdmLibCtx.allocFlags); 

      if ((ret = mdm_setObject((void **) &wanCommonObj, &iidStack, FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set WanCommonIntfCfgObject, ret = %d", ret);
      }
   
   }
   else
   {
      /* Wifi WANDevice is already present, no action needed */
      mdm_freeObject(&mdmObj);
   }

   if (added > 0)
   {
      mdm_increaseWanDeviceCount(added);
   }

   return ret;
}
#endif /* DMP_X_BROADCOM_COM_WIFIWAN_1 */

#ifdef DMP_X_BROADCOM_COM_RDPA_1
CmsRet addDefaultWifiLanFilters(void)
{
    CmsRet ret = CMSRET_SUCCESS;
    UBOOL8 isUdef = 0;
    int objInxCounter = 0;
    MdmPathDescriptor filterDataPathDesc;
    _IngressFiltersDataObject *filterData = NULL;
    char strPortMask[BUFLEN_64+1]={0};
    int  wl_cnt = wlgetintfNo();

    /* Check if we have wl device */
    if (wl_cnt == 0)
    {
        return ret;
    }
    
    memset(strPortMask, '0', BUFLEN_64 + 1);
    strPortMask[BUFLEN_64] = '\0';

    INIT_PATH_DESCRIPTOR(&filterDataPathDesc);
    PUSH_INSTANCE_ID(&(filterDataPathDesc.iidStack),1); // for filter instance.1
    filterDataPathDesc.oid = MDMOID_INGRESS_FILTERS_DATA;

    /* Now add the wan port to the existing filters entries */
    while ((ret = mdm_getNextObject(filterDataPathDesc.oid, &(filterDataPathDesc.iidStack), (void **) &filterData)) == CMSRET_SUCCESS)
    {
        objInxCounter++;
        if ((!cmsUtl_strcmp(filterData->type, MDMVS_FILTER_ARP)) ||
            (!cmsUtl_strcmp(filterData->type, MDMVS_FILTER_BCAST)) ||
            (!cmsUtl_strcmp(filterData->type, MDMVS_FILTER_IGMP)) ||
            (!cmsUtl_strcmp(filterData->type, MDMVS_FILTER_IP_FRAG)) ||
            (!cmsUtl_strcmp(filterData->type, MDMVS_FILTER_ETYPE_UDEF) &&
                filterData->val == RDPA_ETHTYPE_UDEF_VAL ))
        {
            /* If udef was configured set isUdef and skip on configure new entry */
            if (filterData->val != 0)
                isUdef = 1;

            /* Updaate the with the priv portmap */
            cmsUtl_strncpy(strPortMask, filterData->ports, sizeof(strPortMask));

            /* set the last char in the arry to 1 means wlan0 port will be marked in the port bit mask */
            strPortMask[(BUFLEN_64-1)-RDPA_WLAN0_MASK_INX] = '1';
            CMSMEM_REPLACE_STRING_FLAGS(filterData->ports, strPortMask, mdmLibCtx.allocFlags);

            if ((ret = mdm_setObject((void **) &filterData, &(filterDataPathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Could not set filter data object, ret=%d", ret);
            }
        }
    }
    
    ret = CMSRET_SUCCESS;
    
    /* If didnt found any filter entries go to end */
    if (objInxCounter == 0)
    {
        cmsLog_error("could not get filter data object, ret=%d", ret);
        goto exit;
    }

    /* If udef was already configured skip new configuration */
    if (isUdef)
        goto exit;

    /* Create new udef entry */
    INIT_PATH_DESCRIPTOR(&filterDataPathDesc);
    filterDataPathDesc.oid = MDMOID_INGRESS_FILTERS_DATA;
    PUSH_INSTANCE_ID(&(filterDataPathDesc.iidStack), (objInxCounter+2));

    if ((ret = mdm_addObjectInstance(&filterDataPathDesc, NULL, NULL)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to create filter data udef ");
        goto exit;
    }

    /* get the object we just created */
    if ((ret = mdm_getObject(filterDataPathDesc.oid, &(filterDataPathDesc.iidStack), (void **) &filterData)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not get filter data object, ret=%d", ret);
        goto exit;
    }

    /* set the last char in the arry to 1 means wlan0 port will be marked in the port bit mask */
    strPortMask[(BUFLEN_64-1)-RDPA_WLAN0_MASK_INX] = '1';
    CMSMEM_REPLACE_STRING_FLAGS(filterData->ports, strPortMask, mdmLibCtx.allocFlags);
    filterData->val = RDPA_ETHTYPE_UDEF_VAL;

    CMSMEM_REPLACE_STRING_FLAGS(filterData->type, MDMVS_FILTER_ETYPE_UDEF, mdmLibCtx.allocFlags);
    CMSMEM_REPLACE_STRING_FLAGS(filterData->action, MDMVS_FILTER_CPU, mdmLibCtx.allocFlags);

    if ((ret = mdm_setObject((void **) &filterData, &(filterDataPathDesc.iidStack), FALSE)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not set filter data object, ret=%d", ret);
    }

exit:
    if (filterData!=NULL)
        mdm_freeObject((void **) &filterData);
    return ret;
}
#endif
#endif  /* BRCM_WLAN */
