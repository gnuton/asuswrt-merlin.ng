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
#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_ethintf.h"
#include "rut_lan.h"
#include "rut_wan.h"
#include "rut_xtmlinkcfg.h"
#include "devctl_xtm.h"
#include "bcmxtmcfg.h"
#include "rut_wanlayer2.h"
#include "rut_wifiwan.h"

CmsRet rutWl2_getL2LinkObj(MdmObjectId wanConnOid, 
                           const InstanceIdStack *iidStack,
                           void ** L2LinkCfgObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack wanConnIid = *iidStack;
   void *linkCfg =NULL;
   _WanCommonIntfCfgObject *wanCommIntf = NULL;
   UBOOL8 isDsl = FALSE;
   UBOOL8 isEthWan = FALSE;
   UBOOL8 isMocaWan = FALSE;
   UBOOL8 isL2tpAcWan = FALSE;
   UBOOL8 isPonWan = FALSE;
   UBOOL8 isWifiWan = FALSE;

   cmsLog_debug("Enter.");
   
   if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_COMMON_INTF_CFG,
                                      wanConnOid,
                                      &wanConnIid, 
                                      OGF_NO_VALUE_UPDATE,
                                      (void **) &wanCommIntf)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get WanCommonIntfCfgObject, ret=%d", ret);
      return ret;
   }

   if (!cmsUtl_strcmp(wanCommIntf->WANAccessType, MDMVS_DSL))
   {
      isDsl = TRUE;
   }   
   else if (!cmsUtl_strcmp(wanCommIntf->WANAccessType, MDMVS_ETHERNET))
   {
      isEthWan = TRUE;
   }
   else if (!cmsUtl_strcmp(wanCommIntf->WANAccessType, MDMVS_X_BROADCOM_COM_MOCA))
   {
      isMocaWan = TRUE;
   }      
   else if (!cmsUtl_strcmp(wanCommIntf->WANAccessType, MDMVS_X_BROADCOM_COM_L2TPAC))
   {
      isL2tpAcWan = TRUE;
   }      
   else if (!cmsUtl_strcmp(wanCommIntf->WANAccessType, MDMVS_X_BROADCOM_COM_PON))
   {
      isPonWan = TRUE;
   }      
   else if (!cmsUtl_strcmp(wanCommIntf->WANAccessType, MDMVS_X_BROADCOM_COM_WIFI))
   {
      isWifiWan = TRUE;
   }              


   cmsObj_free((void **) &wanCommIntf);


   if (isDsl)
   {
#ifdef DMP_ADSLWAN_1
   
      _WanDslIntfCfgObject *dslIntfCfg=NULL;
      
      wanConnIid = *iidStack; 
      
      /* need to find out if it is ATM or PTM */
      if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_DSL_INTF_CFG, 
                                         wanConnOid,
                                         &wanConnIid,
                                         0,
                                         (void **) &dslIntfCfg)) != CMSRET_SUCCESS)
           {
         cmsLog_error("Failed to get WanDslIntfCfgObj, ret=%d", ret);
         return ret;
      }
      else
      {
         UBOOL8 isAtm = !cmsUtl_strcmp(dslIntfCfg->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM);
         
         cmsObj_free((void **) &dslIntfCfg);

         wanConnIid = *iidStack; 
         if (isAtm)
         {
#ifdef DMP_X_BROADCOM_COM_ATMWAN_1
            if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_DSL_LINK_CFG, 
                                               wanConnOid, 
                                               &wanConnIid, 
                                               0,
                                               (void **) &linkCfg)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to get dslLinkCfgObj from %s", cmsMdm_dumpIidStack( (void *) &wanConnIid));
               return ret;
            }
            cmsLog_debug("Got dslLinkCfg (atm).");            

#endif /* DMP_X_BROADCOM_COM_ATMWAN_1 */
         }
         else 
         {
#ifdef DMP_PTMWAN_1            
            /* get ptmLinkcfg */
            if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_PTM_LINK_CFG, 
                                               wanConnOid, 
                                               &wanConnIid,
                                               0,
                                               (void **) &linkCfg)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to get dslLinkCfgObj from %s", cmsMdm_dumpIidStack( (void *) &wanConnIid));
               return ret;
            }
            cmsLog_debug("Got ptmLinkCfg.");            
#endif /* DMP_PTMWAN_1 */
         }
      }         
#endif /* DMP_ADSLWAN_1 */

   }


   if (isEthWan)
   {
#ifdef DMP_ETHERNETWAN_1

      wanConnIid = *iidStack; 
      
      if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_ETH_INTF, 
                                  wanConnOid,
                                  &wanConnIid, 
                                  0,
                                  (void **) &linkCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get ethIntfObj from %s", cmsMdm_dumpIidStack( (void *) &wanConnIid));
         return ret;
      }
      cmsLog_debug("Got EthIntfObj."); 
#endif /* DMP_ETHERNETWAN_1 */

   }

   if (isMocaWan)
   {
#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1
   
      wanConnIid = *iidStack; 
      
      if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_MOCA_INTF, 
                                  wanConnOid,
                                  &wanConnIid, 
                                  0,
                                  (void **) &linkCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get mocaIntfObj from %s", cmsMdm_dumpIidStack( (void *) &wanConnIid));
         return ret;
      }
      cmsLog_debug("Got MocaIntfObj."); 
#endif /* DMP_X_BROADCOM_COM_MOCAWAN_1 */
   }

   if (isL2tpAcWan)
   {
#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
   
      wanConnIid = *iidStack; 
      
      if ((ret = cmsObj_getAncestorFlags(MDMOID_L2TP_AC_LINK_CONFIG, 
                                  wanConnOid,
                                  &wanConnIid, 
                                  0,
                                  (void **) &linkCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get L2tpAcLinkCfgObj from %s", cmsMdm_dumpIidStack( (void *) &wanConnIid));
         return ret;
      }
      cmsLog_debug("Got L2tpAcLinkCfgObj."); 
#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */
   }

   if (isPonWan)
   {
#ifdef DMP_X_BROADCOM_COM_PONWAN_1
   
      _WanPonIntfObject *ponIntfCfg=NULL;
      
      wanConnIid = *iidStack; 
      
      /* need to find out if it is Gpon or Epon (later on) */
      if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_PON_INTF, 
                                         wanConnOid,
                                         &wanConnIid,
                                         0,
                                         (void **) &ponIntfCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get ponIntfCfg, ret=%d", ret);
         return ret;
      }
      else
      {

         wanConnIid = *iidStack; 
#ifdef DMP_X_BROADCOM_COM_GPONWAN_1     
         UBOOL8 isGpon = !cmsUtl_strcmp(ponIntfCfg->ponType, MDMVS_GPON);
         
         if (isGpon)
         {
            if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_GPON_LINK_CFG,
                                               wanConnOid,
                                               &wanConnIid,
                                               0,
                                               (void **) &linkCfg)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to get gponLinkCfgObj from %s", cmsMdm_dumpIidStack( (void *) &wanConnIid));
               cmsObj_free((void **) &ponIntfCfg);
               return ret;
            }
            cmsLog_debug("Got gponLinkCfgObj.");            
         }
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 */

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1         
         UBOOL8 isEpon = !cmsUtl_strcmp(ponIntfCfg->ponType, MDMVS_EPON);    

         if (isEpon)
         {
#ifdef EPON_SFU
            if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_EPON_INTF,
                                               wanConnOid,
                                               &wanConnIid,
                                               0,
                                               (void **) &linkCfg)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to get eponIntfCfgObj from %s", cmsMdm_dumpIidStack( (void *) &wanConnIid));
               cmsObj_free((void **) &ponIntfCfg);
               return ret;
            }
            cmsLog_debug("Got eponIntfCfgObj.");
#else
            if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_EPON_LINK_CFG,
                                               wanConnOid,
                                               &wanConnIid,
                                               0,
                                               (void **) &linkCfg)) != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to get eponLinkCfgObj from %s", cmsMdm_dumpIidStack( (void *) &wanConnIid));
               cmsObj_free((void **) &ponIntfCfg);
               return ret;
            }
            cmsLog_debug("Got eponLinkCfgObj.");
#endif
         }
#endif /* DMP_X_BROADCOM_COM_EPONWAN_1 */

         cmsObj_free((void **) &ponIntfCfg);
      }         
#endif /* DMP_X_BROADCOM_COM_PONWAN_1 */

   }

   if (isWifiWan)
   {
#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1

      wanConnIid = *iidStack; 
      
      if ((ret = cmsObj_getAncestorFlags(MDMOID_WAN_WIFI_INTF, 
                                  wanConnOid,
                                  &wanConnIid, 
                                  0,
                                  (void **) &linkCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get wifiIntfObj from %s", cmsMdm_dumpIidStack( (void *) &wanConnIid));
         return ret;
      }
      cmsLog_debug("Got WifiIntfObj."); 
#endif /* DMP_X_BROADCOM_COM_WIFIWAN_1 */

   }   


   if (linkCfg != NULL)
   {
      /* assign the layer 2 linkCfgObj */
      *L2LinkCfgObj = linkCfg;  
   }

   cmsLog_debug("Exit. L2LinkCfgObj=%p", *L2LinkCfgObj);
   
   return ret;
   
}

CmsRet rutWl2_getL2Info(MdmObjectId wanConnOid,  
                        const InstanceIdStack *iidStack,
                        UBOOL8 *isLayer2LinkUp,
                        char *l2IfName,
                        ConnectionModeType *connMode)
{
   void *L2LinkCfgObj=NULL;
   MdmObjectId L2LinkCfgOid;
   CmsRet ret=CMSRET_SUCCESS;

   cmsLog_debug("Enter");

   if ((ret = rutWl2_getL2LinkObj(wanConnOid, 
                                 iidStack, 
                                 &L2LinkCfgObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutWl2_getL2LinkObj faile. iidStack = %s", cmsMdm_dumpIidStack( (void *) &iidStack));
      return ret;
   }

   L2LinkCfgOid = GET_MDM_OBJECT_ID(L2LinkCfgObj);

   switch (L2LinkCfgOid)
   {
   
#ifdef DMP_ADSLWAN_1  
      case MDMOID_WAN_DSL_LINK_CFG:
      {
         if (((_WanDslLinkCfgObject *) L2LinkCfgObj)->X_BROADCOM_COM_IfName == NULL)
         {
            ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            *isLayer2LinkUp = rutXtm_isXDSLLinkUp(iidStack, MDMOID_WAN_DSL_LINK_CFG);
            strcpy(l2IfName, ((_WanDslLinkCfgObject *) L2LinkCfgObj)->X_BROADCOM_COM_IfName);
            *connMode = cmsUtl_connectionModeStrToNum
               (((_WanDslLinkCfgObject *) L2LinkCfgObj)->X_BROADCOM_COM_ConnectionMode);
         }               
      }
      break;
      
#ifdef DMP_PTMWAN_1  
      case MDMOID_WAN_PTM_LINK_CFG:
      {
         if (((_WanPtmLinkCfgObject *) L2LinkCfgObj)->X_BROADCOM_COM_IfName == NULL)
         {
            ret = CMSRET_INVALID_ARGUMENTS;
         }  
         else
         {
            *isLayer2LinkUp = rutXtm_isXDSLLinkUp(iidStack, MDMOID_WAN_PTM_LINK_CFG);
            strcpy(l2IfName, ((_WanPtmLinkCfgObject *) L2LinkCfgObj)->X_BROADCOM_COM_IfName);
            *connMode = cmsUtl_connectionModeStrToNum
               (((_WanPtmLinkCfgObject *) L2LinkCfgObj)->X_BROADCOM_COM_ConnectionMode);
         }
      }         
      break;
      
#endif /* DMP_PTMWAN_1 */
      
#endif /* DMP_ADSLWAN_1 */

#ifdef DMP_ETHERNETWAN_1 
      case MDMOID_WAN_ETH_INTF:
      {
         if (((_WanEthIntfObject *) L2LinkCfgObj)->X_BROADCOM_COM_IfName == NULL)
         {
            ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            *isLayer2LinkUp = 
               !cmsUtl_strcmp(((_WanEthIntfObject *) L2LinkCfgObj)->status, MDMVS_UP);
            strcpy(l2IfName, ((_WanEthIntfObject *) L2LinkCfgObj)->X_BROADCOM_COM_IfName);
            *connMode = cmsUtl_connectionModeStrToNum
               (((_WanEthIntfObject *) L2LinkCfgObj)->X_BROADCOM_COM_ConnectionMode);
         }            
      }
      break;      
#endif /* DMP_ETHERNETWAN_1 */


#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1 
      case MDMOID_WAN_MOCA_INTF:
      {
         if (((_WanMocaIntfObject *) L2LinkCfgObj)->ifName == NULL)
         {
            ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            *isLayer2LinkUp = 
               !cmsUtl_strcmp(((_WanMocaIntfObject *) L2LinkCfgObj)->status, MDMVS_UP);
            strcpy(l2IfName, ((_WanMocaIntfObject *) L2LinkCfgObj)->ifName);
            *connMode = cmsUtl_connectionModeStrToNum
               (((_WanMocaIntfObject *) L2LinkCfgObj)->connectionMode);
         }       
      }
      break;      
#endif /* DMP_X_BROADCOM_COM_MOCAWAN_1 */

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1 
      case MDMOID_L2TP_AC_LINK_CONFIG:
      {
         if (((_L2tpAcLinkConfigObject *) L2LinkCfgObj)->ifName == NULL)
         {
            ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            *isLayer2LinkUp = 
               !cmsUtl_strcmp(((_L2tpAcLinkConfigObject*) L2LinkCfgObj)->linkStatus, MDMVS_UP);
            strcpy(l2IfName, ((_L2tpAcLinkConfigObject *) L2LinkCfgObj)->ifName);
            *connMode = cmsUtl_connectionModeStrToNum
               (((_L2tpAcLinkConfigObject *) L2LinkCfgObj)->connectionMode);
         }       
      }
      break;      
#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */


#ifdef DMP_X_BROADCOM_COM_GPONWAN_1 
      case MDMOID_WAN_GPON_LINK_CFG:
      {
         if (((_WanGponLinkCfgObject*) L2LinkCfgObj)->ifName == NULL)
         {
            ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            *isLayer2LinkUp = 
               !cmsUtl_strcmp(((_WanGponLinkCfgObject*) L2LinkCfgObj)->linkStatus, MDMVS_UP);
            strcpy(l2IfName, ((_WanGponLinkCfgObject *) L2LinkCfgObj)->ifName);
            *connMode = cmsUtl_connectionModeStrToNum
               (((_WanGponLinkCfgObject *) L2LinkCfgObj)->connectionMode);
         }       
      }
      break;      
#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 */

#ifdef DMP_X_BROADCOM_COM_EPONWAN_1 
      case MDMOID_WAN_EPON_INTF:
      {
         if (((_WanEponIntfObject*) L2LinkCfgObj)->ifName == NULL)
         {
            ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            *isLayer2LinkUp = 
               !cmsUtl_strcmp(((_WanEponIntfObject*) L2LinkCfgObj)->status, MDMVS_UP);
            strcpy(l2IfName, ((_WanEponIntfObject *) L2LinkCfgObj)->ifName);
            *connMode = cmsUtl_connectionModeStrToNum
               (((_WanEponIntfObject *) L2LinkCfgObj)->connectionMode);
         }       
      }
      break;
      case MDMOID_WAN_EPON_LINK_CFG:
      {
         if (((_WanEponLinkCfgObject*) L2LinkCfgObj)->ifName == NULL)
         {
            ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            *isLayer2LinkUp = 
               !cmsUtl_strcmp(((_WanEponLinkCfgObject*) L2LinkCfgObj)->linkStatus, MDMVS_UP);
            strcpy(l2IfName, ((_WanEponLinkCfgObject *) L2LinkCfgObj)->ifName);
            *connMode = cmsUtl_connectionModeStrToNum
               (((_WanEponLinkCfgObject *) L2LinkCfgObj)->connectionMode);
         }       
      }
      break;
#endif /* DMP_X_BROADCOM_COM_EPONWAN_1 */

#ifdef DMP_X_BROADCOM_COM_WIFIWAN_1 
      case MDMOID_WAN_WIFI_INTF:
      {
         if (((_WanWifiIntfObject *) L2LinkCfgObj)->ifName == NULL)
         {
            ret = CMSRET_INVALID_ARGUMENTS;
         }
         else
         {
            *isLayer2LinkUp = 
               !cmsUtl_strcmp(((_WanWifiIntfObject *) L2LinkCfgObj)->status, MDMVS_UP);
            strcpy(l2IfName, ((_WanWifiIntfObject *) L2LinkCfgObj)->ifName);
            *connMode = cmsUtl_connectionModeStrToNum
               (((_WanWifiIntfObject *) L2LinkCfgObj)->connectionMode);
         }            
      }
      break;      
#endif /* DMP_X_BROADCOM_COM_WIFIWAN_1 */

      default:
      {
         cmsLog_error("Invalid layer 2 oid %d", L2LinkCfgOid);
         ret = CMSRET_INTERNAL_ERROR;
      }  
   }   
   
   cmsObj_free((void **) &L2LinkCfgObj);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("rutWl2_getL2Info failed, ret=%d", ret);
   }
   
   cmsLog_debug("Exit, l2IfName=%s connMode=%d linkUp=%d", l2IfName, (UINT32) *connMode, *isLayer2LinkUp);

   return ret;
   
}


CmsRet rutWl2_getXtmInfo(MdmObjectId wanConnOid __attribute((unused)),
                        const InstanceIdStack *wanConnIid __attribute((unused)),
                        UBOOL8 *isPPPoAorIPoA __attribute((unused)),
                        UBOOL8 *isVCMux __attribute((unused)),
                        XTM_ADDR *xtmAddr __attribute((unused)))
{
   CmsRet ret=CMSRET_INTERNAL_ERROR;
   
#ifdef DMP_ADSLWAN_1  

   void *L2LinkCfgObj=NULL;
   MdmObjectId L2LinkCfgOid;

   cmsLog_debug("Enter");

   if ((ret = rutWl2_getL2LinkObj(wanConnOid, 
                                 wanConnIid, 
                                 &L2LinkCfgObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutWl2_getL2LinkObj faile. wanConnIid = %s", cmsMdm_dumpIidStack( (void *) &wanConnIid));
      return ret;
   }

   L2LinkCfgOid = GET_MDM_OBJECT_ID(L2LinkCfgObj);

   switch (L2LinkCfgOid)
   {
#ifdef DMP_X_BROADCOM_COM_ATMWAN_1     
      case MDMOID_WAN_DSL_LINK_CFG:
      {
         SINT32 vpi;
         SINT32 vci;
         
         if (wanConnOid == MDMOID_WAN_PPP_CONN)
         {
            *isPPPoAorIPoA = !cmsUtl_strcmp(((_WanDslLinkCfgObject *) L2LinkCfgObj)->linkType, MDMVS_PPPOA) ;
         }
         else if (wanConnOid == MDMOID_WAN_IP_CONN)
         {
            *isPPPoAorIPoA = !cmsUtl_strcmp(((_WanDslLinkCfgObject *) L2LinkCfgObj)->linkType, MDMVS_IPOA);
         }   

         *isVCMux = !cmsUtl_strcmp(((_WanDslLinkCfgObject *) L2LinkCfgObj)->ATMEncapsulation, MDMVS_VCMUX);

         xtmAddr->ulTrafficType = TRAFFIC_TYPE_ATM;
         xtmAddr->u.Vcc.ulPortMask = 
            PORTID_TO_PORTMASK(((_WanDslLinkCfgObject *) L2LinkCfgObj)->X_BROADCOM_COM_ATMInterfaceId);
         if ((ret = cmsUtl_atmVpiVciStrToNum(((_WanDslLinkCfgObject *) L2LinkCfgObj)->destinationAddress, 
            &vpi, &vci)) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not convert destinationAddress %s",((_WanDslLinkCfgObject *) L2LinkCfgObj)->destinationAddress);
            cmsObj_free((void **) &L2LinkCfgObj);
            return ret;
         }
         xtmAddr->u.Vcc.usVpi = (UINT16) vpi;
         xtmAddr->u.Vcc.usVci = (UINT16) vci;
         
      }
      break;
      
#endif /* DMP_X_BROADCOM_COM_ATMWAN_1   */    

#ifdef DMP_PTMWAN_1
      case MDMOID_WAN_PTM_LINK_CFG:
      {
         xtmAddr->ulTrafficType = TRAFFIC_TYPE_PTM;
         xtmAddr->u.Flow.ulPortMask = PORTID_TO_PORTMASK(((_WanPtmLinkCfgObject *) L2LinkCfgObj)->X_BROADCOM_COM_PTMPortId);
         xtmAddr->u.Flow.ulPtmPriority = ((_WanPtmLinkCfgObject *) L2LinkCfgObj)->X_BROADCOM_COM_PTMPriorityHigh;
      }
      break;

#endif /* DMP_PTMWAN_1 */

      default:
      {
         cmsLog_error("Invalid layer 2 oid %d", L2LinkCfgOid);
         ret = CMSRET_INTERNAL_ERROR;
      }  
   }   
   
   cmsObj_free((void **) &L2LinkCfgObj);
   
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("rutWl2_getXtmInfo failed");
   }

   cmsLog_debug("Exit");

#endif /* DMP_ADSLWAN_1 */

   return ret;

}


UBOOL8 rutWl2_isWanLayer2DSL(MdmObjectId wanConnOid, const InstanceIdStack *iidStack)
{
   UBOOL8 isDSL=FALSE;
   void *L2LinkCfgObj=NULL;
   MdmObjectId L2LinkCfgOid;

   if ((rutWl2_getL2LinkObj(wanConnOid, 
                           iidStack, 
                           &L2LinkCfgObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutWl2_getL2LinkObj faile. wanConnIid = %s", cmsMdm_dumpIidStack((void *) &iidStack));
   }
   else
   {
      L2LinkCfgOid = GET_MDM_OBJECT_ID(L2LinkCfgObj);
      isDSL = (L2LinkCfgOid == MDMOID_WAN_DSL_LINK_CFG || L2LinkCfgOid == MDMOID_WAN_PTM_LINK_CFG);
      cmsObj_free((void **) &L2LinkCfgObj);
   }

   cmsLog_debug("Exit. isDSL=%d", isDSL);  
   
   return isDSL;
   
}


UBOOL8 rutWl2_isPPPoA(const InstanceIdStack *iidStack __attribute((unused)))
{
   UBOOL8 isPPPoA=FALSE;
   
#ifdef DMP_X_BROADCOM_COM_ATMWAN_1  
   UBOOL8 isVCMux;
   XTM_ADDR xtmAddr;

   /* Only get xtm when it is in DSL */
   if (rutWl2_isWanLayer2DSL(MDMOID_WAN_PPP_CONN, iidStack))
   {
      rutWl2_getXtmInfo(MDMOID_WAN_PPP_CONN,  iidStack, &isPPPoA, &isVCMux, &xtmAddr);
   }
   cmsLog_debug("isPPPoA is %d", isPPPoA);   
   
#endif /* DMP_X_BROADCOM_COM_ATMWAN_1 */

   return isPPPoA;
   
}


UBOOL8 rutWl2_isPPPoL2tp(const InstanceIdStack *iidStack __attribute((unused)))
{
   UBOOL8 isL2tp=FALSE;
#ifdef DMP_X_BROADCOM_COM_L2TPAC_1
   void *L2LinkCfgObj=NULL;
   MdmObjectId L2LinkCfgOid;
   
   if ((rutWl2_getL2LinkObj(MDMOID_WAN_PPP_CONN, 
                           iidStack, 
                           &L2LinkCfgObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rutWl2_getL2LinkObj faile. wanConnIid = %s", cmsMdm_dumpIidStack((void *) &iidStack));
   }
   else
   {
      L2LinkCfgOid = GET_MDM_OBJECT_ID(L2LinkCfgObj);
      isL2tp = (L2LinkCfgOid == MDMOID_L2TP_AC_LINK_CONFIG);
      cmsObj_free((void **) &L2LinkCfgObj);
   }
   cmsLog_debug("Exit. isL2tp=%d", isL2tp);  
   
#endif

   return isL2tp;
}


UBOOL8 rutWl2_isIPoA(const InstanceIdStack *iidStack __attribute((unused)))
{
   UBOOL8 isIPoA=FALSE;
   
#ifdef DMP_X_BROADCOM_COM_ATMWAN_1  
   UBOOL8 isVCMux;
   XTM_ADDR xtmAddr;


   /* Only get xtm when it is in DSL */
   if (rutWl2_isWanLayer2DSL(MDMOID_WAN_IP_CONN, iidStack))
   {   
      rutWl2_getXtmInfo(MDMOID_WAN_IP_CONN,  iidStack, &isIPoA, &isVCMux, &xtmAddr);
   }
   cmsLog_debug("isIPoA is %d", isIPoA);   
   
#endif /* DMP_X_BROADCOM_COM_ATMWAN_1 */

   return isIPoA;
   
}



UBOOL8 rutWl2_isVlanMuxEnabled(MdmObjectId wanConnOid, 
                               const InstanceIdStack *iidStack)
{
   UBOOL8 isLayer2LinkUp=FALSE;
   char l2IfName[CMS_IFNAME_LENGTH];
   ConnectionModeType connMode;
     
   rutWl2_getL2Info(wanConnOid, iidStack, &isLayer2LinkUp, l2IfName, &connMode);

   return (connMode == CMS_CONNECTION_MODE_VLANMUX);

}


UBOOL8 rutWl2_isWanLayer2LinkUp(MdmObjectId wanConnOid, 
                                const InstanceIdStack *iidStack)
{
   UBOOL8 isLayer2LinkUp=FALSE;
   char l2IfName[CMS_IFNAME_LENGTH];
   ConnectionModeType connMode;
   
   rutWl2_getL2Info(wanConnOid, iidStack, &isLayer2LinkUp, l2IfName, &connMode);
                        
   cmsLog_debug("isLayer2LinkUp = %d", isLayer2LinkUp);

   return isLayer2LinkUp;

 }


ConnectionModeType rutWl2_getConnMode(MdmObjectId wanConnOid, 
                          const InstanceIdStack *iidStack)
{
   UBOOL8 isLayer2LinkUp;
   char l2IfName[CMS_IFNAME_LENGTH];
   ConnectionModeType connMode;

   rutWl2_getL2Info(wanConnOid, iidStack, &isLayer2LinkUp, l2IfName, &connMode);

   cmsLog_debug("connMode = %d", connMode);
   
   return connMode;
   
}

CmsRet rutWl2_getL2IfName(MdmObjectId wanConnOid,  
                          const InstanceIdStack *iidStack,
                          char *l2IfName)
{
   UBOOL8 isLayer2LinkUp;
   ConnectionModeType connMode;
   CmsRet ret = rutWl2_getL2Info(wanConnOid, iidStack, &isLayer2LinkUp, l2IfName, &connMode);
   
   cmsLog_debug("ret=%d, l2IfName = %s", ret, l2IfName);

   return ret;
   
}


CmsRet rutWl2_getL2IfnameFromL3Ifname(const char *l3Ifname, char *l2Ifname)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   void *wanConnObj=NULL;
   UBOOL8 isPpp;
   CmsRet ret;

   cmsLog_debug("l3Ifname=%s", l3Ifname);

   *l2Ifname = '\0';

   ret = rutWan_getIpOrPppObjectByIfname(l3Ifname, &iidStack, &wanConnObj, &isPpp);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not find object for %s", l3Ifname);
      return ret;
   }

   ret = rutWl2_getL2IfName(GET_MDM_OBJECT_ID(wanConnObj), &iidStack, l2Ifname);

   cmsObj_free(&wanConnObj);
   
   if (ret == CMSRET_SUCCESS)
   {
      cmsLog_debug("got it: l2IfName=%s", l2Ifname);
   }
   else
   {
      cmsLog_error("could not convert to L2Ifname");
   }

   return ret;
}

#ifdef DMP_X_BROADCOM_COM_ATMWAN_1

CmsRet rutWl2_getAtmDslIntfObject(InstanceIdStack *iidStack,
                               WanDslIntfCfgObject **wanDslIntfObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 flags = 0;

   if (iidStack == NULL || wanDslIntfObj == NULL)
   {
      cmsLog_error("iidStack or wanDslIntfObj is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(iidStack);
   while ((ret = cmsObj_getNextFlags(MDMOID_WAN_DSL_INTF_CFG, iidStack, flags, (void **) wanDslIntfObj)) == CMSRET_SUCCESS)
   {
      if ((*wanDslIntfObj)->enable &&
          !cmsUtl_strcmp((*wanDslIntfObj)->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM))
      {
         /* we found the object we wanted, return immediately, don't free object */
         return ret;
      }

      cmsObj_free((void **) wanDslIntfObj);
   }


   cmsLog_debug("could not find ATM Intf object");
   return ret;
}

#endif

#ifdef DMP_PTMWAN_1

CmsRet rutWl2_getPtmDslIntfObject(InstanceIdStack *iidStack,
                               WanDslIntfCfgObject **wanDslIntfObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 flags = 0;

   if (iidStack == NULL || wanDslIntfObj == NULL)
   {
      cmsLog_error("iidStack or wanDslIntfObj is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(iidStack);
   while ((ret = cmsObj_getNextFlags(MDMOID_WAN_DSL_INTF_CFG, iidStack, flags, (void **) wanDslIntfObj)) == CMSRET_SUCCESS)
   {
      if ((*wanDslIntfObj)->enable &&
          !cmsUtl_strcmp((*wanDslIntfObj)->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM))
      {
         /* we found the object we wanted, return immediately, don't free object */
         return ret;
      }

      cmsObj_free((void **) wanDslIntfObj);
   }


   cmsLog_debug("could not find PTM Intf object");
   return ret;
}

#endif /* DMP_PTMWAN_1 */


#ifdef DMP_X_BROADCOM_COM_DSLBONDING_1
CmsRet rutWl2_getBondingDslIntfObjectByPeerName(const char     *peerName,
                                           InstanceIdStack     *iidStack,
                                           WanDslIntfCfgObject **wanDslIntfObj)
{
   MdmPathDescriptor pathDesc;
   CmsRet ret;

   if (IS_EMPTY_STRING(peerName))
   {
      cmsLog_error("invalid peerName.");
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (peerName[strlen(peerName)-1] == '.')
   {
      ret = cmsMdm_fullPathToPathDescriptor(peerName, &pathDesc);
   }
   else
   {
      char wanDevFullPath[BUFLEN_512];

      /* add a dot at the end to indicate that the path is an object path */
      snprintf(wanDevFullPath, sizeof(wanDevFullPath), "%s.", peerName);
      ret = cmsMdm_fullPathToPathDescriptor(wanDevFullPath, &pathDesc);
   }
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor returns error. ret=%d", ret);
      return ret;
   }

   /* get the WanDSLInterfaceConfig object */
   memcpy(iidStack, &pathDesc.iidStack, sizeof(InstanceIdStack)); 
   if ((ret = cmsObj_get(MDMOID_WAN_DSL_INTF_CFG, iidStack, OGF_NO_VALUE_UPDATE, (void **) wanDslIntfObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMdm_fullPathToPathDescriptor returns error. ret=%d", ret);
      return ret;
   }

   /* we found the object we wanted, return without free object */
   return ret;
}

CmsRet rutWl2_getBondingAtmDslIntfObject(InstanceIdStack *iidStack,
                                      WanDslIntfCfgObject **wanDslIntfObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 flags = 0;

   if (iidStack == NULL || wanDslIntfObj == NULL)
   {
      cmsLog_error("iidStack or wanDslIntfObj is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(iidStack);
   while ((ret = cmsObj_getNextFlags(MDMOID_WAN_DSL_INTF_CFG, iidStack, flags, (void **) wanDslIntfObj)) == CMSRET_SUCCESS)
   {
      if ((*wanDslIntfObj)->enable &&
          !cmsUtl_strcmp((*wanDslIntfObj)->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM) &&
          (*wanDslIntfObj)->X_BROADCOM_COM_BondingLineNumber == 1)
      {
         /* we found the object we wanted, return immediately, don't free object */
         return ret;
      }

      cmsObj_free((void **) wanDslIntfObj);
   }


   cmsLog_debug("could not find Bonding ATM Intf object");
   return ret;
}

CmsRet rutWl2_getBondingPtmDslIntfObject(InstanceIdStack *iidStack,
                                      WanDslIntfCfgObject **wanDslIntfObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   UINT32 flags = 0;

   if (iidStack == NULL || wanDslIntfObj == NULL)
   {
      cmsLog_error("iidStack or wanDslIntfObj is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(iidStack);
   while ((ret = cmsObj_getNextFlags(MDMOID_WAN_DSL_INTF_CFG, iidStack, flags, (void **) wanDslIntfObj)) == CMSRET_SUCCESS)
   {
      if ((*wanDslIntfObj)->enable &&
          !cmsUtl_strcmp((*wanDslIntfObj)->linkEncapsulationUsed, MDMVS_G_993_2_ANNEX_K_PTM) &&
          (*wanDslIntfObj)->X_BROADCOM_COM_BondingLineNumber == 1)
      {
         /* we found the object we wanted, return immediately, don't free object */
         return ret;
      }

      cmsObj_free((void **) wanDslIntfObj);
   }


   cmsLog_debug("could not find Bonding PTM Intf object");
   return ret;
}

#endif /* DMP_X_BROADCOM_COM_DSLBONDING_1 */



#ifdef DMP_ETHERNETWAN_1

CmsRet rutWl2_getWanEthObject(InstanceIdStack *iidStack,
                              WanEthIntfObject **wanEthIntfObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   _WanCommonIntfCfgObject *wanCommIntf = NULL;
   UINT32 flags = OGF_NO_VALUE_UPDATE;
   UBOOL8 found = FALSE;

   if (iidStack == NULL || wanEthIntfObj == NULL)
   {
      cmsLog_error("iidStack or wanEthIntfObj is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(iidStack);
   while (!found &&
          ((ret = cmsObj_getNextFlags(MDMOID_WAN_COMMON_INTF_CFG, iidStack, flags, (void **) &wanCommIntf)) == CMSRET_SUCCESS))
   {
      found = (0 == cmsUtl_strcmp(wanCommIntf->WANAccessType, MDMVS_ETHERNET));
      cmsObj_free((void **) &wanCommIntf);
   }

   if (!found)
   {
      cmsLog_error("could not find WanEth object");
      return ret;
   }

   /*
    * WanCommonInterfaceConfig is at the same level as WanEthernetInterface config,
    * so once we have found WanCommonInterfaceConfig, we can use the same iidStack
    * to get the WanEthernetInterfaceConfig.
    */
   ret = cmsObj_get(MDMOID_WAN_ETH_INTF, iidStack, 0, (void **) wanEthIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get ethIntfCfg object, ret=%d", ret);
   }
      
   return ret;
}

#endif /* DMP_ETHERNETWAN_1 */


#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1

CmsRet rutWl2_getWanMocaObject(InstanceIdStack *iidStack,
                               WanMocaIntfObject **wanMocaIntfObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   _WanCommonIntfCfgObject *wanCommIntf = NULL;
   UINT32 flags = OGF_NO_VALUE_UPDATE;
   UBOOL8 found = FALSE;

   if (iidStack == NULL || wanMocaIntfObj == NULL)
   {
      cmsLog_error("iidStack or wanMocaIntfObj is NULL");
      return CMSRET_INVALID_ARGUMENTS;
   }

   INIT_INSTANCE_ID_STACK(iidStack);
   while (!found &&
          ((ret = cmsObj_getNextFlags(MDMOID_WAN_COMMON_INTF_CFG, iidStack, flags, (void **) &wanCommIntf)) == CMSRET_SUCCESS))
   {
      found = (0 == cmsUtl_strcmp(wanCommIntf->WANAccessType, MDMVS_X_BROADCOM_COM_MOCA));
      cmsObj_free((void **) &wanCommIntf);
   }

   if (!found)
   {
      cmsLog_error("could not find WanMoca object");
      return ret;
   }

   /*
    * WanCommonInterfaceConfig is at the same level as WanEthernetInterface config,
    * so once we have found WanCommonInterfaceConfig, we can use the same iidStack
    * to get the WanEthernetInterfaceConfig.
    */
   ret = cmsObj_get(MDMOID_WAN_MOCA_INTF, iidStack, 0, (void **) wanMocaIntfObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get mocaIntfCfg object, ret=%d", ret);
   }
      
   return ret;
}

#endif /* DMP_X_BROADCOM_COM_MOCAWAN_1 */



#ifdef DMP_X_BROADCOM_COM_L2TPAC_1

CmsRet rutWl2_getL2tpWanIidStack(InstanceIdStack *l2tpWanIid)
{
   _WanDevObject *wanDev=NULL;
   _WanCommonIntfCfgObject *wanCommIntf = NULL;
   UBOOL8 found = FALSE;   
   CmsRet ret;

   INIT_INSTANCE_ID_STACK(l2tpWanIid);
   
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_DEV, l2tpWanIid, OGF_NO_VALUE_UPDATE, (void **)&wanDev)) == CMSRET_SUCCESS)
   {
      cmsObj_free((void **)&wanDev);  /* no longer needed */

      if ((ret = cmsObj_get(MDMOID_WAN_COMMON_INTF_CFG, l2tpWanIid, OGF_NO_VALUE_UPDATE, (void **)&wanCommIntf)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_get <MDMOID_WAN_COMMON_INTF_CFG> returns error. ret=%d", ret);
         return ret;
      }  
      
      if (!cmsUtl_strcmp(wanCommIntf->WANAccessType, MDMVS_X_BROADCOM_COM_L2TPAC))
      {
        found = TRUE;
        cmsObj_free((void **)&wanCommIntf);
      }
   }
   
   if (!found)
   {
      cmsLog_error("L2tp WanDev does not exist?");
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_debug("Found l2tpWan iidStack");
      ret = CMSRET_SUCCESS;
   }      
      
   return ret;
}

#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */

#ifdef DMP_X_BROADCOM_COM_GPONWAN_1

CmsRet rutWl2_getGponWanIidStack(InstanceIdStack *gponWanIid)
{


   _WanDevObject *wanDev=NULL;
   _WanCommonIntfCfgObject *wanCommIntf = NULL;
   _WanPonIntfObject *ponObj = NULL;
   UBOOL8 found = FALSE;   
   CmsRet ret;

   INIT_INSTANCE_ID_STACK(gponWanIid);
   
   /* Find gpon  WANDEV */
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_DEV, gponWanIid, OGF_NO_VALUE_UPDATE, (void **)&wanDev)) == CMSRET_SUCCESS)
   {
      cmsObj_free((void **)&wanDev);  /* no longer needed */

      if ((ret = cmsObj_get(MDMOID_WAN_COMMON_INTF_CFG, gponWanIid, OGF_NO_VALUE_UPDATE, (void **)&wanCommIntf)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_get <MDMOID_WAN_COMMON_INTF_CFG> returns error. ret=%d", ret);
         return ret;
      }         
      if (!cmsUtl_strcmp(wanCommIntf->WANAccessType, MDMVS_X_BROADCOM_COM_PON))
      {
         if ((ret = cmsObj_get(MDMOID_WAN_PON_INTF, gponWanIid, OGF_NO_VALUE_UPDATE, (void **)&ponObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_get  <MDMOID_WAN_PON_INTF> error. ret=%d", ret);
         }
         else
         {
            if (!cmsUtl_strcmp(ponObj->ponType, MDMVS_GPON))
            {
                found = TRUE;
            }
            cmsObj_free((void **)&ponObj);              
         }            
      }
      cmsObj_free((void **)&wanCommIntf);     
      
   }
   
   if (!found)
   {
      cmsLog_error("Gpon WanDev does not exist?");
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_debug("Found  gponWan iidStack");
      ret = CMSRET_SUCCESS;
   }      
      
   return ret;
   
}

#endif /* DMP_X_BROADCOM_COM_GPONWAN_1 */



#ifdef DMP_X_BROADCOM_COM_EPONWAN_1

CmsRet rutWl2_getEponWanIidStack(InstanceIdStack *eponWanIid)
{


   _WanDevObject *wanDev=NULL;
   _WanCommonIntfCfgObject *wanCommIntf = NULL;
   _WanPonIntfObject *ponObj = NULL;
   UBOOL8 found = FALSE;   
   CmsRet ret;

   INIT_INSTANCE_ID_STACK(eponWanIid);
   
   /* Find epon  WANDEV */
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_WAN_DEV, eponWanIid, OGF_NO_VALUE_UPDATE, (void **)&wanDev)) == CMSRET_SUCCESS)
   {
      cmsObj_free((void **)&wanDev);  /* no longer needed */

      if ((ret = cmsObj_get(MDMOID_WAN_COMMON_INTF_CFG, eponWanIid, OGF_NO_VALUE_UPDATE, (void **)&wanCommIntf)) != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsObj_get <MDMOID_WAN_COMMON_INTF_CFG> returns error. ret=%d", ret);
         return ret;
      }         
      if (!cmsUtl_strcmp(wanCommIntf->WANAccessType, MDMVS_X_BROADCOM_COM_PON))
      {
         if ((ret = cmsObj_get(MDMOID_WAN_PON_INTF, eponWanIid, OGF_NO_VALUE_UPDATE, (void **)&ponObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_get  <MDMOID_WAN_PON_INTF> error. ret=%d", ret);
         }
         else
         {
            if (!cmsUtl_strcmp(ponObj->ponType, MDMVS_EPON))
            {
                found = TRUE;
            }
            cmsObj_free((void **)&ponObj);              
         }            
      }
      cmsObj_free((void **)&wanCommIntf);     
      
   }
   
   if (!found)
   {
      cmsLog_error("Epon WanDev does not exist?");
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_debug("Found  eponWan iidStack");
      ret = CMSRET_SUCCESS;
   }      
      
   return ret;
   
}

#endif /* DMP_X_BROADCOM_COM_EPONWAN_1 */

