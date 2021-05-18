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

#ifdef DMP_ADSLWAN_1



#include "stl.h"
#include "cms_util.h"
#include "rut_lan.h"
#include "rut_system.h"
#include "rut_xtmlinkcfg.h"



CmsRet stl_wanDslLinkCfgObject(_WanDslLinkCfgObject *obj  __attribute((unused)),
                   const InstanceIdStack *iidStack __attribute__((unused)))
{
#ifdef DMP_X_BROADCOM_COM_ATMSTATS_1
/* XTM Interface Statistics object replaces ATM Interface Statistics object.
 * This will be removed in the near future.
 */

   SINT32 vpi;
   SINT32 vci;
   WanDslLinkCfgObject *dslCfgObj = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("Entered, obj %p",obj);

   if (obj == NULL)
   {
      if (cmsObj_get(MDMOID_WAN_DSL_LINK_CFG, iidStack, 0, (void **) &dslCfgObj) == CMSRET_SUCCESS)
      {
         cmsUtl_atmVpiVciStrToNum(dslCfgObj->destinationAddress, &vpi, &vci);
         rut_clearAtmVccStats(vpi,vci,(dslCfgObj->X_BROADCOM_COM_ATMInterfaceId));
         cmsObj_free((void **) &dslCfgObj);
      }
      else
      {
         cmsLog_debug("unable to get dslLinkcfg, obj %p",obj);
      }
   }
   else
   {
      cmsUtl_atmVpiVciStrToNum(obj->destinationAddress, &vpi, &vci);
      rut_getAtmVccStats(vpi,vci,(obj->X_BROADCOM_COM_ATMInterfaceId),
                         &obj->AAL5CRCErrors,&obj->X_BROADCOM_COM_VCC_OversizedSDU,
                         &obj->X_BROADCOM_COM_VCC_ShortPacketErrors, &obj->X_BROADCOM_COM_VCC_LengthError);
   }
   return (ret);


#endif /* DMP_X_BROADCOM_COM_ATMSTATS_1 */

#ifdef DMP_X_BROADCOM_COM_XTMSTATS_1
   UINT32 byteRx, packetRx, errRx, dropRx;
   UINT32 byteTx, packetTx, errTx, dropTx;
   UINT32 byteMultiRx, packetMulitRx, packetUniRx, packetBcastRx;
   UINT32 byteMultiTx, packetMulitTx, packetUniTx, packetBcastTx;

   if ((obj != NULL) && (obj->enable))
   {
      rut_getIntfStats(obj->X_BROADCOM_COM_IfName,
                       &byteRx,&packetRx,&byteMultiRx,&packetMulitRx,&packetUniRx,&packetBcastRx,&errRx,&dropRx,
                       &byteTx,&packetTx,&byteMultiTx,&packetMulitTx,&packetUniTx,&packetBcastTx,&errTx,&dropTx);

      obj->ATMTransmittedBlocks = (packetTx*48);
      obj->ATMReceivedBlocks = (packetRx*48);
      obj->ATMCRCErrors = errRx;
      return CMSRET_SUCCESS;
   }
#endif
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wanDslAtmParamsObject(_WanDslAtmParamsObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}


#ifdef DMP_PTMWAN_1
CmsRet stl_wanPtmLinkCfgObject(_WanPtmLinkCfgObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
   /* DONE.  This handler function does not need to do anything. */
   return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}
#endif


#ifdef DMP_X_BROADCOM_COM_ATMSTATS_1
/* XTM Interface Statistics object replaces ATM Interface Statistics object.
 * This will be removed in the near future.
 */

CmsRet stl_wanAtmIntfStatsObject(_WanAtmIntfStatsObject *obj __attribute__((unused)), const InstanceIdStack *iidStack __attribute__((unused)))
{
return CMSRET_SUCCESS_OBJECT_UNCHANGED;
}

CmsRet stl_wanAtmStatsObject(_WanAtmStatsObject *obj, const InstanceIdStack *iidStack)
{
   UINT32 interface = iidStack->instance[iidStack->currentDepth-1] - 1;

   cmsLog_debug("Enter(obj %p), wanDevice.instance %d",obj,interface);


   if (obj == NULL)
   {
      rut_getAtmIntfStats(obj, interface, 1);
   }
   else
   {
      rut_getAtmIntfStats(obj, interface,0);
   }
   return CMSRET_SUCCESS;
}

CmsRet stl_wanAal5StatsObject(_WanAal5StatsObject *obj, const InstanceIdStack *iidStack)
{
   UINT32 interface = iidStack->instance[iidStack->currentDepth-1] - 1;

   cmsLog_debug("Enter(obj %p), wanDevice.instance %d",obj,interface);

   if (obj == NULL)
   {
      rut_getAtmAal5IntfStats(obj, interface, 1);
   }
   else
   {
      rut_getAtmAal5IntfStats(obj, interface,0);
   }
   return CMSRET_SUCCESS;
}

CmsRet stl_wanAal2StatsObject(_WanAal2StatsObject *obj, const InstanceIdStack *iidStack)
{
   UINT32 interface = iidStack->instance[iidStack->currentDepth-1] - 1;

   cmsLog_debug("Enter(obj %p, interface %d)",obj,interface);

   if (obj == NULL)
   {
      rut_getAtmAal2IntfStats(obj, interface, 1);
   }
   else
   {
      rut_getAtmAal2IntfStats(obj, interface,0);
   }
   return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_ATMSTATS_1 */


#ifdef DMP_X_BROADCOM_COM_XTMSTATS_1
/* XTM Interface Statistics object replaces ATM Interface Statistics object */ 

CmsRet stl_xtmInterfaceStatsObject(_XtmInterfaceStatsObject *obj, const InstanceIdStack *iidStack)
{
   UINT32 interface = iidStack->instance[iidStack->currentDepth-1] - 1;

   cmsLog_debug("Enter(obj %p), wanDevice.instance %d",obj,interface);

   if (obj == NULL)
   {
      /* clear statistics */
      rut_getXtmIntfStats(obj, interface, 1);
   }
   else
   {
      rut_getXtmIntfStats(obj, interface,0);
   }
   return CMSRET_SUCCESS;
}

#endif /* DMP_X_BROADCOM_COM_XTMSTATS_1 */

#endif  /* DMP_ADSLWAN_1 */
