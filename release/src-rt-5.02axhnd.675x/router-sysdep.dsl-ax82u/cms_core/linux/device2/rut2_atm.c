/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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
/* this only applies to device 2, and also us some function in rut_atm.c */

#if defined(DMP_DEVICE2_ATMLINK_1) || defined(DMP_DEVICE2_PTMLINK_1)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>

#include "bcmnet.h"
#include "cms_core.h"
#include "cms_util.h"
#include "cms_strconv2.h"
#include "rut_diag.h"
#include "rcl.h"
#include "rut_atm.h"
#include "rut2_atm.h"
#include "rut2_dsl.h"
#include "rut_util.h"
#include "rut_system.h"
#include "devctl_xtm.h"
#include "devctl_atm.h"
#include "cms_boardcmds.h"
#include "cms_qos.h"
#include "cms_qdm.h"

CmsRet rutatm_fillL2IfName_dev2(const Layer2IfNameType ifNameType, char **ifName)
{
   CmsRet ret;
   SINT32 intfArray[IFC_WAN_MAX];
   SINT32 index = 0;
   char *prefix;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char L2IfName[CMS_IFNAME_LENGTH];
   Dev2AtmObject *atmObj = NULL;
   Dev2AtmLinkObject *atmLinkObj = NULL;   

   memset((UINT8 *) &intfArray, 0, sizeof(intfArray));
   
   if ((ret = cmsObj_get(MDMOID_DEV2_ATM, &iidStack, 0, (void *) &atmObj)) == CMSRET_SUCCESS)
   {
      if (atmObj->linkNumberOfEntries >= IFC_WAN_MAX)
      {
         cmsLog_error("Only %d interface allowed", IFC_WAN_MAX);
         cmsObj_free((void **) &atmObj);
         return CMSRET_INTERNAL_ERROR;      
      }
   }
   else
   {
      cmsLog_debug("This should never happen, Device.ATM object is not found");
      return CMSRET_INTERNAL_ERROR;      
   }
   cmsObj_free((void **) &atmObj);

   switch (ifNameType)
   {
   case ATM_EOA:
   case ATM_PPPOA:
      while ((ret = cmsObj_getNext(MDMOID_DEV2_ATM_LINK, &iidStack, (void **) &atmLinkObj)) == CMSRET_SUCCESS)
      {
         if (atmLinkObj->name == NULL)
         {
            /* this is one we just created and is NULL, so break */
            cmsObj_free((void **) &atmLinkObj);
            break;
         }
         else
         {
            index = atoi(&(atmLinkObj)->name[strlen(ATM_IFC_STR)]);
            if (index <= IFC_WAN_MAX) 
            {
               cmsLog_debug("atmLinkObj->name=%s, index=%d", atmLinkObj->name, index);
               intfArray[index] = 1;            /* mark the interface used */
            }
         }
         cmsObj_free((void **) &atmLinkObj);
      }
      prefix = ATM_IFC_STR;
      break;
   case ATM_IPOA:
      while ((ret = cmsObj_getNext(MDMOID_DEV2_ATM_LINK, &iidStack, (void **) &atmLinkObj)) == CMSRET_SUCCESS)
      {
         if (atmLinkObj->name == NULL)
         {
            /* this is one we just created and is NULL, so break */
            cmsObj_free((void **) &atmLinkObj);
            break;
         }
         else if (strstr(atmLinkObj->name, IPOA_IFC_STR))
         {
            index = atoi(&(atmLinkObj)->name[strlen(IPOA_IFC_STR)]);
            if (index <= IFC_WAN_MAX) 
            {
               cmsLog_debug("atmLinkObj->ifname=%s, index=%d", atmLinkObj->name, index);
               intfArray[index] = 1;            /* mark the interface used */
            }
         }
         cmsObj_free((void **) &atmLinkObj);
      }
      prefix = IPOA_IFC_STR;
      break;
   default:
      cmsLog_error("Wrong type=%d", ifNameType);
      return CMSRET_INTERNAL_ERROR;
   }
   
   for (index = 0; index < IFC_WAN_MAX; index++)
   {
      cmsLog_debug("intfArray[%d]=%d", index, intfArray[index]);
      if (intfArray[index] == 0)
      {
         sprintf(L2IfName, "%s%d", prefix, index);
         ret = CMSRET_SUCCESS;
         break;
      }
   }

   CMSMEM_REPLACE_STRING_FLAGS(*ifName, L2IfName, mdmLibCtx.allocFlags);
   cmsLog_debug("Get Layer2 ifName=%s", *ifName);

   return ret;
}

/***************************************************************************
 * find the traffic descriptor index that matches the given
 * return traffic descriptor index or 0 if none found.
****************************************************************************/
#ifdef DESKTOP_LINUX
UINT32 rutAtm_getTrffDscrIndex_dev2(const _Dev2AtmLinkQosObject *atmLinkQosObj __attribute__((unused)))
#else
UINT32 rutAtm_getTrffDscrIndex_dev2(const _Dev2AtmLinkQosObject *atmLinkQosObj)
#endif
{  
#ifndef DESKTOP_LINUX
   UINT32 index = 0;
   char line[BUFLEN_264];
   char col[6][BUFLEN_16];
   char category[BUFLEN_32];
   UINT32 parm1 = 0, parm2 = 0, parm3 = 0;
   SINT32 parm4 = -1;
   FILE* fs = NULL;

   /* convert UBR, CBR, vbr-nrt,  ... to  "ubr", "cbr", nrtvbr.. */
   if (rutAtm_categoryConvertion(atmLinkQosObj->qoSClass, category, sizeof(category)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Do not understand the new atmLinkQosObj->qoSClass %s",atmLinkQosObj->qoSClass);
      return index;
   }
   rut_doSystemAction("rut_atm", atmTdteString "show > /var/tdteshow");

   if ((fs = fopen("/var/tdteshow", "r")) == NULL)
   {
      cmsLog_error("Failed to get /var/tdteshow");
      return index;
   }
   
   /* title = (index type pcr scr mbs mcr) */
   while ((fgets(line, BUFLEN_264, fs) != NULL ) && index == 0) 
   {
      if (strstr(line, category) != NULL) 
      {
         sscanf(line, "%s %s %s %s %s %s", col[0], col[1], col[2], col[3], col[4], col[5]);
         parm1 = strtoul(col[2], (char **)NULL, 10);
         parm2 = strtoul(col[3], (char **)NULL, 10);
         parm3 = strtoul(col[4], (char **)NULL, 10);
         parm4 = strtol(col[5], (char **)NULL, 10);


         if (parm4 == 0)
         {
            parm4 = -1;
         }
         if (strcmp(category, col[1]) == 0 &&
             atmLinkQosObj->peakCellRate == parm1 &&
             atmLinkQosObj->sustainableCellRate == parm2 &&
             /* we need proprietary parameter  */
             atmLinkQosObj->maximumBurstSize == parm3 &&
             atmLinkQosObj->X_BROADCOM_COM_MinimumCellRate == parm4)
         {
            index = strtoul(col[0], (char **)NULL, 10);
         }
      }
   }

   fclose(fs);
   rut_doSystemAction("rut_atm", "rm /var/tdteshow");
   return index;

#else
   return(1);
#endif


}

UBOOL8 rutatm_isTdteIndexActive(unsigned int index)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 active = FALSE;
   Dev2AtmLinkObject *atmLinkObj = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   SINT32 nVpi = 0;
   SINT32 nVci = 0;
   XTM_ADDR Addr;
   XTM_CONN_CFG ConnCfg;

   while ((ret = cmsObj_getNext(MDMOID_DEV2_ATM_LINK, &iidStack, (void **) &atmLinkObj)) == CMSRET_SUCCESS)
   {
      /* basically, we need to look at all the ATM_LINK connections, and see if anyone is using this tdte index */
      if ((ret = cmsUtl_atmVpiVciStrToNum_dev2(atmLinkObj->destinationAddress, &nVpi, &nVci)) != CMSRET_SUCCESS)
      {
         memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
         Addr.u.Vcc.ulPortMask = PORTID_TO_PORTMASK(atmLinkObj->X_BROADCOM_COM_ATMInterfaceId);
         Addr.u.Vcc.usVpi      = (UINT16) nVpi;
         Addr.u.Vcc.usVci      = (UINT16) nVci;
         if ((ret = devCtl_xtmGetConnCfg( &Addr, &ConnCfg )) != CMSRET_SUCCESS)
         {
            if (ConnCfg.ulTransmitTrafficDescrIndex == index)
            {
               active = TRUE;
               cmsObj_free((void **) &atmLinkObj);
               break;
            }
         }
      }
      cmsObj_free((void **) &atmLinkObj);
   }
   return (active);
}

/*
 * Compose atm traffic descriptor info.
 * Description  : add/delete vcc to BcmAtm using atmctl.
 * For correct ATM shaping, the transmit queue size needs to be
 * number_of_lan_receive_buffers / number_of_transmit_queues
 * (80 Ethernet buffers / 8 transmit queues = 10 buffer queue size)
 */
CmsRet rutAtm_ctlTrffDscrConfig_dev2(const _Dev2AtmLinkQosObject *atmLinkQosObject, int operation)
{
   int tdteIndex = 0;
   char cmdStr[BUFLEN_128];
   char atmCtlStr[BUFLEN_16];
   CmsRet ret=CMSRET_SUCCESS;

   cmdStr[0] = '\0';
   tdteIndex = rutAtm_getTrffDscrIndex_dev2(atmLinkQosObject);
   if (operation == ATM_TDTE_ADD)
   {
      if (tdteIndex == 0)
      {
         rutAtm_categoryConvertion(atmLinkQosObject->qoSClass, atmCtlStr, sizeof(atmCtlStr));
         snprintf(cmdStr, sizeof(cmdStr), "%sadd %s ", atmTdteString, atmCtlStr);

         if (strncmp(atmLinkQosObject->qoSClass, MDMVS_CBR, sizeof(MDMVS_CBR)) == 0 ||
             strncmp(atmLinkQosObject->qoSClass, MDMVS_UBRWPCR, sizeof(MDMVS_UBRWPCR)) == 0)
         {
            /* atmctl operate tdte --add cbr/ubr-w-pcr peakCellRate */
            snprintf(cmdStr + strlen(cmdStr), sizeof(cmdStr) - strlen(cmdStr), " %d", atmLinkQosObject->peakCellRate);
         }
         else if (strncmp(atmLinkQosObject->qoSClass, MDMVS_VBR_RT, sizeof(MDMVS_VBR_RT)) == 0 ||
                  strncmp(atmLinkQosObject->qoSClass, MDMVS_VBR_NRT, sizeof(MDMVS_VBR_NRT)) == 0) 
         {
            /* atmctl operate tdte --add rtvbr/nrtvbr peakCellRate SustainCellRate MaxBurstCellRate */
            snprintf(cmdStr + strlen(cmdStr), sizeof(cmdStr) - strlen(cmdStr), " %d %d %d", atmLinkQosObject->peakCellRate,
                     atmLinkQosObject->sustainableCellRate, atmLinkQosObject->maximumBurstSize);
         }

         if ((atmLinkQosObject->X_BROADCOM_COM_MinimumCellRate > 0) &&
             (strncmp(atmLinkQosObject->qoSClass, MDMVS_UBR, sizeof(MDMVS_UBR)) == 0 ||
              strncmp(atmLinkQosObject->qoSClass, MDMVS_UBRWPCR, sizeof(MDMVS_UBRWPCR)) == 0))
         {
            snprintf(cmdStr + strlen(cmdStr), sizeof(cmdStr) - strlen(cmdStr), " %d",
                     atmLinkQosObject->X_BROADCOM_COM_MinimumCellRate);
         }
      }
      else
      {
         cmsLog_debug("rutAtm_ctlTrffDscrConfig_dev2 traffic descriptor existed");
      }
   }
   else
   {
      if (tdteIndex == 0)
      {
         cmsLog_error("There is nothing to delete, no traffic descriptor matches input QoS parameters.");
         ret = CMSRET_INVALID_PARAM_VALUE;
      }
      else
      {
         /* if we want to delete, we need to make sure no other VCC is using this tdte index */
         /* tdteIndex 1 is created by default by the driver, so leave it */
         if (rutatm_isTdteIndexActive(tdteIndex) || (tdteIndex == 1))
         {
            /* just return */
            cmsLog_debug("Traffic descriptor is still used by other VCCs, traffic descriptor cannot be deleted.");
         }
         else
         {
            /* the tdte is not used, so just delete it from the driver completely. */
            /* atmctl operate tdte --add rtvbr/nrtvbr peakCellRate SustainCellRate MaxBurstCellRate */
            snprintf(cmdStr, sizeof(cmdStr), "%sdelete %d ", atmTdteString, tdteIndex);
         }
      }
   }
   if (cmdStr[0] != '\0')
   {
      rut_doSystemAction("rut_atm", cmdStr);
   }
   return (ret);
}


CmsRet rutAtm_ctlVccAdd_dev2(const _Dev2AtmLinkObject *atmLinkObject, const InstanceIdStack *iidStack)
{
   Dev2AtmLinkQosObject *atmLinkQosObj = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   XTM_ADDR Addr;
   XTM_CONN_CFG ConnCfg;
   PXTM_TRANSMIT_QUEUE_PARMS pTxQ;
   SINT32 nVpi = 0;
   SINT32 nVci = 0;
   int tdteIndex = 0;

   if ((ret = cmsUtl_atmVpiVciStrToNum_dev2(atmLinkObject->destinationAddress, &nVpi, &nVci)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   /* this is always created when ATM_LINK is created, so it should never fail */
   if ((ret = cmsObj_get(MDMOID_DEV2_ATM_LINK_QOS, iidStack, 0, (void **) &atmLinkQosObj)) != CMSRET_SUCCESS)
   {
      return ret;
   }
   /* if a default UBR descriptor is never added, this will add it */
   rutAtm_ctlTrffDscrConfig_dev2(atmLinkQosObj,ATM_TDTE_ADD);

   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
   memset((UINT8 *) &ConnCfg, 0x00, sizeof(ConnCfg));
   Addr.ulTrafficType    = TRAFFIC_TYPE_ATM;
   Addr.u.Vcc.ulPortMask = PORTID_TO_PORTMASK(atmLinkObject->X_BROADCOM_COM_ATMInterfaceId);
   Addr.u.Vcc.usVpi      = (UINT16) nVpi;
   Addr.u.Vcc.usVci      = (UINT16) nVci;
   ConnCfg.ulAtmAalType                = AAL_5;
   ConnCfg.ulAdminStatus = ((atmLinkObject->enable==TRUE) ? ADMSTS_UP : ADMSTS_DOWN);

   if ((tdteIndex = rutAtm_getTrffDscrIndex_dev2(atmLinkQosObj)) == 0)
   {
      cmsLog_error("Missing traffic descriptor configuration for specified QoS parameters; cannot add VCC");
      cmsObj_free((void **) &atmLinkQosObj);
      return CMSRET_INTERNAL_ERROR;
   }
   else 
   {
      ConnCfg.ulTransmitTrafficDescrIndex = rutAtm_getTrffDscrIndex_dev2(atmLinkQosObj);
   }
   cmsObj_free((void **) &atmLinkQosObj);
   ConnCfg.ulTransmitQParmsSize = 1;
   if (!cmsUtl_strcmp(atmLinkObject->X_BROADCOM_COM_GrpScheduler, MDMVS_WRR))
   {
      ConnCfg.ConnArbs[0][0].ulWeightAlg = WA_CWRR;
   }
   else
   {
      ConnCfg.ConnArbs[0][0].ulWeightAlg = WA_DISABLED;
   }
   ConnCfg.ConnArbs[0][0].ulWeightValue = atmLinkObject->X_BROADCOM_COM_GrpWeight;
   ConnCfg.ConnArbs[0][0].ulSubPriority = XTM_QOS_LEVELS - (atmLinkObject->X_BROADCOM_COM_GrpPrecedence);


   if (!cmsUtl_strcmp(atmLinkObject->linkType, MDMVS_EOA))
   {
      if (!cmsUtl_strcmp(atmLinkObject->encapsulation, MDMVS_LLC))
      {
         ConnCfg.ulHeaderType = HT_LLC_SNAP_ETHERNET;
      }
      else
      {
         ConnCfg.ulHeaderType = HT_VC_MUX_ETHERNET;
      }
   }
   else
   {
      if (!cmsUtl_strcmp(atmLinkObject->linkType, MDMVS_PPPOA))
      {
         if (!cmsUtl_strcmp(atmLinkObject->encapsulation, MDMVS_LLC))
         {
            ConnCfg.ulHeaderType = HT_LLC_ENCAPS_PPP;
         }
         else
         {
            ConnCfg.ulHeaderType = HT_VC_MUX_PPPOA;
         }
      }
      else
      {
         if (!cmsUtl_strcmp(atmLinkObject->linkType, MDMVS_IPOA))
         {
            if (!cmsUtl_strcmp(atmLinkObject->encapsulation, MDMVS_LLC))
            {
               ConnCfg.ulHeaderType = HT_LLC_SNAP_ROUTE_IP;
            }
            else
            {
               ConnCfg.ulHeaderType = HT_VC_MUX_IPOA;
            }
         }
         else
         {
            ConnCfg.ulHeaderType = HT_LLC_SNAP_ETHERNET;
         }
      }
   }

   /* Configure the default queue associated with this connection */
   pTxQ = &ConnCfg.TransmitQParms[0];
   pTxQ->usSize = HOST_XTM_NR_TXBDS;

   if (!cmsUtl_strcmp(atmLinkObject->X_BROADCOM_COM_SchedulerAlgorithm, MDMVS_WFQ))
   {
      pTxQ->ucWeightAlg = WA_WFQ;
   }
   else  /* WRR or SP */
   {
      pTxQ->ucWeightAlg = WA_CWRR;
   }
   pTxQ->ulWeightValue = atmLinkObject->X_BROADCOM_COM_QueueWeight;   //ConnCfg.ConnArbs[0][0].ulWeightValue;
   pTxQ->ucSubPriority = XTM_QOS_LEVELS - atmLinkObject->X_BROADCOM_COM_QueuePrecedence;
   pTxQ->ucQosQId      = 0;   /* qid of the default queue is 0 */

   if (!cmsUtl_strcmp(atmLinkObject->X_BROADCOM_COM_DropAlgorithm, MDMVS_RED))
   {
      pTxQ->ucDropAlg = WA_RED;
   }
   else if (!cmsUtl_strcmp(atmLinkObject->X_BROADCOM_COM_DropAlgorithm, MDMVS_WRED))
   {
      pTxQ->ucDropAlg = WA_WRED;
   }
   else
   {
      pTxQ->ucDropAlg = WA_DT;
   }
   pTxQ->ucLoMinThresh = atmLinkObject->X_BROADCOM_COM_LowClassMinThreshold;
   pTxQ->ucLoMaxThresh = atmLinkObject->X_BROADCOM_COM_LowClassMaxThreshold;
   pTxQ->ucHiMinThresh = atmLinkObject->X_BROADCOM_COM_HighClassMinThreshold;
   pTxQ->ucHiMaxThresh = atmLinkObject->X_BROADCOM_COM_HighClassMaxThreshold;

   if (Addr.u.Vcc.ulPortMask == (PORT_PHY0_PATH0 | PORT_PHY0_PATH1))
   {
      pTxQ->ulPortId = PORT_PHY0_PATH0;
   }
   else
   {
      pTxQ->ulPortId = Addr.u.Vcc.ulPortMask;
   }

   if ((ret = devCtl_xtmSetConnCfg( &Addr, &ConnCfg )) != CMSRET_SUCCESS)
   {
      cmsLog_error("devCtl_xtmSetConnCfg returns error. ret=%d", ret);
   }

   return ret;

}  /* End of rut_atmCtlVccAdd() */


CmsRet rutAtm_createInterface_dev2(const _Dev2AtmLinkObject *newObj)
{
   CmsRet ret;
   XTM_ADDR Addr;
   SINT32 vpi;
   SINT32 vci;
   char devName[BUFLEN_32];
   
   /* form the ptm interface name */
   cmsLog_debug("Create ATM interface %s", newObj->name);
   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));

   if ((ret = cmsUtl_atmVpiVciStrToNum_dev2(newObj->destinationAddress, &vpi, &vci)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert destinationAddress %s", newObj->destinationAddress);
      return ret;
   }
   
   Addr.ulTrafficType = TRAFFIC_TYPE_ATM;
   Addr.u.Vcc.ulPortMask = PORTID_TO_PORTMASK(newObj->X_BROADCOM_COM_ATMInterfaceId);
   Addr.u.Vcc.usVpi = (UINT16) vpi;
   Addr.u.Vcc.usVci = (UINT16) vci;

   strcpy(devName, newObj->name);

   if ((ret = devCtl_xtmCreateNetworkDevice(&Addr, devName)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to start atm interface %s. error=%d", devName, ret);
      return ret;
   }

   cmsLog_debug("devCtl_xtmCreateNetworkDevice ret=%d", ret);
   
   return ret;
}


CmsRet rutAtm_deleteInterface_dev2(const _Dev2AtmLinkObject *currObj)
{
   CmsRet ret;
   XTM_ADDR Addr;
   SINT32 vpi;
   SINT32 vci;

   cmsLog_debug("Delete ATM interface %s", currObj->name);

   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
   
   if ((ret = cmsUtl_atmVpiVciStrToNum_dev2(currObj->destinationAddress, &vpi, &vci)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not convert destinationAddress %s", currObj->destinationAddress);
      return ret;
   }
   
   Addr.ulTrafficType = TRAFFIC_TYPE_ATM;
   Addr.u.Vcc.ulPortMask = PORTID_TO_PORTMASK(currObj->X_BROADCOM_COM_ATMInterfaceId);
   Addr.u.Vcc.usVpi = (UINT16) vpi;
   Addr.u.Vcc.usVci = (UINT16) vci;

   if ((ret = devCtl_xtmDeleteNetworkDevice(&Addr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to delete atm interface %s. error=%d", currObj->name, ret);
   }

   cmsLog_debug("devCtl_xtmDeleteNetworkDevice ret=%d", ret);

   return ret;
}

CmsRet rutAtm_ctlVccDelete_dev2(const Dev2AtmLinkObject *atmLinkObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   XTM_ADDR Addr;
   SINT32 nVpi = 0;
   SINT32 nVci = 0;

   if ((ret = cmsUtl_atmVpiVciStrToNum_dev2(atmLinkObj->destinationAddress, &nVpi, &nVci)) != CMSRET_SUCCESS)
   {
      return ret;
   }

   memset((UINT8 *) &Addr, 0x00, sizeof(Addr));
   Addr.ulTrafficType    = TRAFFIC_TYPE_ATM;
   Addr.u.Vcc.ulPortMask = PORTID_TO_PORTMASK(atmLinkObj->X_BROADCOM_COM_ATMInterfaceId);
   Addr.u.Vcc.usVpi      = (UINT16) nVpi;
   Addr.u.Vcc.usVci      = (UINT16) nVci;
   if ((ret = devCtl_xtmSetConnCfg( &Addr, NULL )) != CMSRET_SUCCESS)
   {
      cmsLog_error("devCtl_xtmSetConnCfg returns error. ret=%d", ret);
   }

   return ret;

}  /* End of rut_atmCtlVccDelete() */

CmsRet rutAtm_runAtmOamLoopbackTest_dev2(int type, void *new,
                                         const void *curr __attribute__((unused)),
                                         const InstanceIdStack *iidStack __attribute__((unused)))
{
   Dev2AtmLinkObject *atmLinkObj = NULL;
   MdmPathDescriptor pathDesc;
   CmsRet ret = CMSRET_INVALID_ARGUMENTS;
   SINT32 vpi, vci, port;
   ATM_VCC_ADDR vccAddr;
   ATMDRV_OAM_LOOPBACK results;

   /* all objects are the same with diagnosticsState and interface, so just type cast to the same field */
   Dev2AtmDiagnosticsF5LoopbackObject *newObj = (Dev2AtmDiagnosticsF5LoopbackObject*)new;

   if (cmsUtl_strcmp(newObj->diagnosticsState,MDMVS_REQUESTED) == 0)
   {
      cmsLog_debug("newObj->diagnosticsState %s, repetitions %d, timeout %d",
                   newObj->diagnosticsState,(int)newObj->numberOfRepetitions,
                   newObj->timeout);


      if (newObj->interface != NULL)
      {
         /* interface contains full path of the interface to be tested: atm.link.{i}.  Derive the VPI/VCI. */
         if ((ret = cmsMdm_fullPathToPathDescriptor(newObj->interface, &pathDesc)) != CMSRET_SUCCESS)
         {
            cmsLog_debug("newObj->interface %s, invalid parameter, interface not found",
                         newObj->interface);
            return CMSRET_INVALID_ARGUMENTS;
         }
         if ( cmsObj_get(MDMOID_DEV2_ATM_LINK, &pathDesc.iidStack, 0, (void **) &atmLinkObj) == CMSRET_SUCCESS )
         {
            port = atmLinkObj->X_BROADCOM_COM_ATMInterfaceId;
            cmsUtl_atmVpiVciStrToNum_dev2(atmLinkObj->destinationAddress, &vpi, &vci);
            cmsObj_free((void **) &atmLinkObj);

            vccAddr.ulInterfaceId = port;
            vccAddr.usVpi = vpi;
            if (type == OAM_F4_LB_SEGMENT_TYPE)
            {
               vccAddr.usVci = VCI_OAM_F4_SEGMENT;
            }
            
            else if (type == OAM_F4_LB_END_TO_END_TYPE)
            {
               vccAddr.usVci = VCI_OAM_F4_END_TO_END;
            }
            else
            {
               vccAddr.usVci = vci;
            }

            ret = devCtl_atmSendOamLoopbackTest(type,&vccAddr,newObj->numberOfRepetitions,
                                                newObj->timeout, &results);

            /* Assuming we only do OAM loopback test with very small iteration (1);
             * we are blocking and waiting for the test result, update MDM immediately.
             * However, if when bigger number of repetition of OAM loopback test is required,
             * we would need to keep the result at the ATM driver level; periodically wakeUpMonitor
             * SSK which then calls the driver to get the current OAM test result, and then update
             * the MDM.   We cannot just block here and wait for results.
             */
            
            REPLACE_STRING_IF_NOT_EQUAL_FLAGS(newObj->diagnosticsState,MDMVS_COMPLETE, mdmLibCtx.allocFlags);
            newObj->successCount = results.received;
            newObj->failureCount = (results.sent - results.received);
            newObj->averageResponseTime = results.avgResponseTime;
            newObj->minimumResponseTime = results.minResponseTime;
            newObj->maximumResponseTime = results.maxResponseTime;

            if (mdmLibCtx.eid == EID_TR69C)
            {
               CmsMsgHeader msg = EMPTY_MSG_HEADER;
               msg.type = CMS_MSG_DIAG;
               msg.src =  EID_SMD;
               msg.dst = mdmLibCtx.eid;
               
               msg.flags_event = 1;
               if (cmsMsg_send(mdmLibCtx.msgHandle, &msg) != CMSRET_SUCCESS)
               {
                  cmsLog_error("could not send out CMS_MSG_DIAG event msg");
               }
               else
               {
                  cmsLog_debug("Send out CMS_MSG_DIAG event msg.");
               }
            }/* send msg_diag if tr69c initiated this test */
         } /* atmLinkObj */
      } /* interface is valid */
   } /* requested to do test */
   else
   {
      /* don't do anything if not requested */
      ret = CMSRET_SUCCESS;
   }
   return (ret);
}

void rutatm_getLinkStats_dev2(Dev2AtmLinkStatsObject *stats, char *linkName, int port, UBOOL8 reset)
{
   UINT64 dontCare;
   UINT32 ulPortId = PORTID_TO_PORTMASK(port);
   XTM_INTERFACE_STATS IntfStats;
   XTM_INTERFACE_CFG Cfg;
   int error = 0;
   CmsRet ret;
   UINT64 errorsRx, errorsTx;
   UINT64 discardPacketsRx, discardPacketsTx;

   if (reset)
   {
      devCtl_xtmGetInterfaceStatistics(ulPortId, &IntfStats, reset);
   }
   else
   {
      rut_getIntfStats_uint64(linkName,
                              &dontCare,&dontCare,
                              &dontCare/*byteMultiRx*/,&stats->multicastPacketsReceived,
                              &stats->unicastPacketsReceived,&stats->broadcastPacketsReceived,
                              &errorsRx,&discardPacketsRx,
                              &dontCare,&dontCare,
                              &dontCare/*byteMultiTx*/,&stats->multicastPacketsSent,
                              &stats->unicastPacketsSent,&stats->broadcastPacketsSent,
                              &errorsTx,&discardPacketsTx);
      stats->errorsReceived = (UINT32)errorsRx;
      stats->errorsSent = (UINT32)errorsTx;
      stats->discardPacketsReceived = (UINT32)discardPacketsRx;
      stats->discardPacketsSent = (UINT32)discardPacketsTx;

      /* then some SAR specific statistics */
      ret = devCtl_xtmGetInterfaceCfg(ulPortId,&Cfg);
      if ( (ret != CMSRET_SUCCESS) ||
           ((ret == CMSRET_SUCCESS) && (Cfg.ulIfOperStatus  == OPRSTS_DOWN)) )
      {
         error = 1;
      }
      else
      {
         memset((UINT8 *) &IntfStats, 0x00, sizeof(IntfStats));
         ret = devCtl_xtmGetInterfaceStatistics(ulPortId, &IntfStats, reset);
         if (ret != CMSRET_SUCCESS)
         {
            error = 1;
         }
         else
         {
            stats->bytesReceived = (UINT64) IntfStats.ulIfInOctets;
            stats->bytesSent =  (UINT64)IntfStats.ulIfOutOctets;
            stats->packetsReceived = (UINT64)IntfStats.ulIfInPackets;
            stats->packetsSent = (UINT64)IntfStats.ulIfOutPackets;
            stats->discardPacketsReceived = IntfStats.ulIfInPacketErrors;
            stats->X_BROADCOM_COM_InOAMCells = IntfStats.ulIfInOamRmCells;
            stats->X_BROADCOM_COM_OutOAMCells = IntfStats.ulIfOutOamRmCells;
            stats->X_BROADCOM_COM_InASMCells = IntfStats.ulIfInAsmCells;
            stats->X_BROADCOM_COM_OutASMCells = IntfStats.ulIfOutAsmCells;
            stats->X_BROADCOM_COM_InCellErrors = IntfStats.ulIfInCellErrors;
         }
      } /* no error from atm PORT */
      if (error)
      {
         stats->bytesReceived = 0;
         stats->bytesSent =  0;
         stats->packetsReceived = 0;
         stats->packetsSent = 0;
         stats->discardPacketsReceived = 0;
         stats->X_BROADCOM_COM_InOAMCells = 0;
         stats->X_BROADCOM_COM_OutOAMCells = 0;
         stats->X_BROADCOM_COM_InASMCells = 0;
         stats->X_BROADCOM_COM_OutASMCells = 0;
         stats->X_BROADCOM_COM_InCellErrors = 0;
      }
      stats->transmittedBlocks = (stats->packetsSent*48);
      stats->receivedBlocks = (stats->packetsReceived*48);
      stats->CRCErrors = stats->errorsReceived;   /* AAL level error */
      stats->HECErrors = stats->X_BROADCOM_COM_InCellErrors; /* ATM layer header error, I assume */
   } /* !reset */
}

CmsRet rutatm_fillLowerLayer(char **lowerLayer)
{
   Dev2DslChannelObject *dslChannelObj;
#ifdef DMP_DEVICE2_BONDEDDSL_1
   Dev2DslBondingGroupObject *dslBondingGroupObj;
   UBOOL8 isBondingEnabled;
   UBOOL8 operational = FALSE;
#endif
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc;
   UBOOL8 found = FALSE;
   char *fullPathString=NULL;

   /* in our system, in a non-bonding board, the second dsl line used for bonding is not configurable,
    * so, when a atm link is created, it is always on the top of ATM channel that is stacked on the
    * primary line.   
    */
   /* in a bonding board & image, then the ATM link is stacked on the 
    * top of the ATM bonding group.
    */
#ifdef DMP_DEVICE2_BONDEDDSL_1
   /* 1. if bonding is disabled, then this cannot be used for lowerLayer.
    * 2. if bonding is operational, then it can be used for lower layer.
    */
   qdmDsl_isDslBondingEnabled_dev2(&isBondingEnabled);
   if (isBondingEnabled)
   {
      qdmDsl_isDslBondingGroupStatusOperational_dev2(MDMVS_ATM,&operational);
      if (operational == TRUE)
      {
         while (!found && cmsObj_getNext(MDMOID_DEV2_DSL_BONDING_GROUP, &iidStack, (void **) &dslBondingGroupObj) == CMSRET_SUCCESS)
         {
            if (!cmsUtl_strcmp(dslBondingGroupObj->bondScheme, MDMVS_ATM))
            {
               found = TRUE;
               INIT_PATH_DESCRIPTOR(&pathDesc);
               pathDesc.oid = MDMOID_DEV2_DSL_BONDING_GROUP;
               pathDesc.iidStack = iidStack;
               cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathString);
               CMSMEM_REPLACE_STRING_FLAGS(*lowerLayer, fullPathString, mdmLibCtx.allocFlags);         
            }
            cmsObj_free((void **) &dslBondingGroupObj);      
         }
      }
   } /* isBondingEnabled */
#endif
   while (!found &&
          cmsObj_getNext(MDMOID_DEV2_DSL_CHANNEL, &iidStack, (void **) &dslChannelObj) == CMSRET_SUCCESS)
   {
      if (!strcmp(dslChannelObj->linkEncapsulationUsed, MDMVS_G_992_3_ANNEX_K_ATM))
      {
         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid = MDMOID_DEV2_DSL_CHANNEL;
         pathDesc.iidStack = iidStack;
         cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &fullPathString);
         CMSMEM_REPLACE_STRING_FLAGS(*lowerLayer, fullPathString, mdmLibCtx.allocFlags);         
         found = TRUE;
      }
      cmsObj_free((void **) &dslChannelObj);
   }
   if (found)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(fullPathString);
      return CMSRET_SUCCESS;
   }
   return CMSRET_INTERNAL_ERROR;
}

#ifdef DMP_DEVICE2_BONDEDDSL_1
void rutxtm_getBondingGroupStatus_dev2(Dev2DslBondingGroupObject *bondingGroupObj)
{
   CmsRet ret;
   XTM_BOND_INFO bondInfo;
   int bonded = 0;

   /* Calculate and return the TR181 LastChange */
   IF_OBJ_NOT_NULL_GET_LASTCHANGE(bondingGroupObj);

   memset(&bondInfo,0,sizeof(XTM_BOND_INFO));
   /* MAX_BOND_GROUPS, MAX_BOND_PORTS */
   ret = devCtl_xtmGetBondingInfo (&bondInfo);

   if (!strcmp(bondingGroupObj->bondScheme, MDMVS_ATM))
   {
      switch (bondInfo.ulTrafficType)
      {
      case TRAFFIC_TYPE_ATM_BONDED:
         CMSMEM_REPLACE_STRING_FLAGS(bondingGroupObj->groupStatus,MDMVS_NONE,mdmLibCtx.allocFlags);
         bonded = 1;
         break;
      case TRAFFIC_TYPE_ATM:
         CMSMEM_REPLACE_STRING_FLAGS(bondingGroupObj->groupStatus,MDMVS_PEERBONDSCHEMEMISMATCH,mdmLibCtx.allocFlags);
         break;
      case TRAFFIC_TYPE_NOT_CONNECTED:
         CMSMEM_REPLACE_STRING_FLAGS(bondingGroupObj->groupStatus,MDMVS_NOPEER,mdmLibCtx.allocFlags);
         break;
      default:
         CMSMEM_REPLACE_STRING_FLAGS(bondingGroupObj->groupStatus,MDMVS_NONE,mdmLibCtx.allocFlags);
         break;
      } /* traffic type */
   }
   else 
   {
      /* ptm bonding group */
      switch (bondInfo.ulTrafficType)
      {
      case TRAFFIC_TYPE_PTM_BONDED:
         CMSMEM_REPLACE_STRING_FLAGS(bondingGroupObj->groupStatus,MDMVS_NONE,mdmLibCtx.allocFlags);
         bonded = 1;
         break;
      case TRAFFIC_TYPE_PTM:
         CMSMEM_REPLACE_STRING_FLAGS(bondingGroupObj->groupStatus,MDMVS_PEERBONDSCHEMEMISMATCH,mdmLibCtx.allocFlags);
         break;
      case TRAFFIC_TYPE_NOT_CONNECTED:
         CMSMEM_REPLACE_STRING_FLAGS(bondingGroupObj->groupStatus,MDMVS_NOPEER,mdmLibCtx.allocFlags);
         break;
      default:
         CMSMEM_REPLACE_STRING_FLAGS(bondingGroupObj->groupStatus,MDMVS_NONE,mdmLibCtx.allocFlags);
         break;
      } /* traffic type */
   } /* read from driver ok */
   if (bonded)
   {
      /* update the statistics about the bonding group from driver; MaxBondGroup is 1, so it's always groupInfo[0] */
      /* Target rate is best effort which is 0 (default)
       * bondingGroupObj->targetUpRate = 0; 
       * bondingGroupObj->targetDownRate = 0; 
       */
      bondingGroupObj->upstreamDifferentialDelayTolerance = bondInfo.grpInfo[0].diffUSDelay;      
      bondingGroupObj->runningTime = bondingGroupObj->lastChange;
   }
   else
   {
      bondingGroupObj->runningTime = 0;
   }
}

void rutxtm_getBondingGroupStatsTotal(Dev2DslBondingGroupStatsTotalObject *statsTotalObj,const InstanceIdStack *iidStack)
{
   CmsRet ret;
   XTM_BOND_INFO bondInfo;
   int i;
   Dev2DslBondingGroupObject *bondingGroupObj = NULL;

   memset(&bondInfo,0,sizeof(XTM_BOND_INFO));

   /* MAX_BOND_GROUPS, MAX_BOND_PORTS */
   ret = devCtl_xtmGetBondingInfo (&bondInfo);
   if (ret == CMSRET_SUCCESS)
   {
      ret = cmsObj_get(MDMOID_DEV2_DSL_BONDING_GROUP, iidStack, 0, (void **) &bondingGroupObj);
      if (ret != CMSRET_SUCCESS)
      {
         return;
      }
      for (i=0; i<bondInfo.ulNumGroups; i++)
      {
         if ( ((!strcmp(bondingGroupObj->bondScheme, MDMVS_ATM)) && 
               (bondInfo.ulTrafficType == TRAFFIC_TYPE_ATM_BONDED)) ||
              ((!strcmp(bondingGroupObj->bondScheme, MDMVS_ETHERNET)) && 
               (bondInfo.ulTrafficType == TRAFFIC_TYPE_PTM_BONDED)))
         {
            statsTotalObj->upstreamRate = bondInfo.grpInfo[i].aggrUSRate;
            statsTotalObj->downstreamRate = bondInfo.grpInfo[i].aggrDSRate;;
            statsTotalObj->upstreamDifferentialDelay = bondInfo.grpInfo[i].diffUSDelay;
         }
         else
         {
            statsTotalObj->upstreamRate = 0;
            statsTotalObj->downstreamRate = 0;
            statsTotalObj->upstreamDifferentialDelay = 0;
         }
      } /* groups */
      cmsObj_free((void **) &bondingGroupObj);
   } /* read from driver ok */
   rutdsl_getBondingGroupTotalStats(statsTotalObj);
}

void rutxtm_getBondingGroupEthernetStats(Dev2DslBondingGroupEthernetStatsObject *statsObj)
{
   XTM_ERROR_STATS ErrStats;
   if (devCtl_xtmGetErrorStatistics(&ErrStats) != CMSRET_SUCCESS )
   {
      printf("Unable to get device's error statistics\n");
   }
   else
   {
      statsObj->framesDropped = ErrStats.ulFramesDropped;
      statsObj->overflowErrorsReceived = ErrStats.ulOverflowErrorsRx;
      statsObj->PAFLostFragments = ErrStats.ulPafLostFragments;
      statsObj->PAFErrors = ErrStats.ulPafErrs;
   } /* got error stats */
}

#endif /* DMP_DEVICE2_BONDEDDSL_1 */
#endif /* (DMP_DEVICE2_ATMLINK_1) || (DMP_DEVICE2_PTMLINK_1) */

#endif /* DMP_DEVICE2_BASELINE_1 */
