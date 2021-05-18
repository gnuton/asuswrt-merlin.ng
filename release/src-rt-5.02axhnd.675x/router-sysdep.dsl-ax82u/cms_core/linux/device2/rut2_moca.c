/***********************************************************************
 *
 *  Copyright (c) 2006-2009  Broadcom Corporation
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

#ifdef DMP_DEVICE2_MOCA_1

#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_moca.h"
#include "rut_system.h"

#include "mocalib.h"
#include "mocalib-cli.h"

/*!\file rcl2_moca.c
 * \brief This file contains Device2 MOCA related functions.
 *
 */


void rutMoca_getIntfInfo_dev2(const char *intfName, Dev2MocaInterfaceObject *obj)
{
   struct moca_drv_info drv_info;
   struct moca_node_status node_status;
   struct moca_fw_version fw_version;
   struct moca_network_status net_status;
   struct moca_interface_status if_status;
   void * pMoca = NULL;
   char swVersionBuf[BUFLEN_128]={0};
   char macAddrBuf[MAC_STR_LEN+1]={0};
   UINT32 temp;
   char tempBuf[BUFLEN_32];

   cmsLog_debug("Entered: intfName=%s", intfName);

#ifdef DESKTOP_LINUX
   /* in Desktop Linux, moca is always UP */
   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status, MDMVS_UP, mdmLibCtx.allocFlags);
   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->firmwareVersion, "1.2.3", mdmLibCtx.allocFlags);
   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->currentVersion, "2.0", mdmLibCtx.allocFlags);
   obj->X_BROADCOM_COM_HwVersion = 7;
   obj->X_BROADCOM_COM_SelfMoCAVersion = 5;
   return;
#endif


   pMoca = moca_open((char *)intfName);
   if (pMoca == NULL)
   {
      cmsLog_error("open of %s failed", intfName);
      return;
   }

   /* Get all info from the driver */
   moca_get_node_status(pMoca, &node_status);
   moca_get_drv_info(pMoca, 0, &drv_info);
   moca_get_fw_version(pMoca, &fw_version);
   moca_get_network_status(pMoca, &net_status);
   moca_get_interface_status(pMoca, &if_status);


   /*
    * Transfer driver info to TR181 object, copied from rutMoca_getStatus
    * and rutMoca_getInterfaceData.
    * We may need to fill in more of the TR181 params.  We may also need to
    * define some Broadcom proprietary params to hold the important info
    * which the driver provides.
    */
   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->status,
                          (if_status.link_status ? MDMVS_UP : MDMVS_DORMANT),
                          mdmLibCtx.allocFlags);

   snprintf(swVersionBuf, sizeof(swVersionBuf), "%d.%d.%d",
            node_status.moca_sw_version_major,
            node_status.moca_sw_version_minor,
            node_status.moca_sw_version_rev);
   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->firmwareVersion, swVersionBuf, mdmLibCtx.allocFlags);

   sprintf(tempBuf, "%d.0", net_status.network_moca_version);
   REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->currentVersion, tempBuf, mdmLibCtx.allocFlags);

   obj->X_BROADCOM_COM_VendorId = node_status.vendor_id;
   obj->X_BROADCOM_COM_HwVersion = node_status.moca_hw_version;
   obj->X_BROADCOM_COM_SelfMoCAVersion = node_status.self_moca_version;

   obj->networkCoordinator = net_status.nc_node_id;
   obj->nodeID = net_status.node_id;
   obj->backupNC = net_status.backup_nc_id;
   moca_get_preferred_nc(pMoca, &temp);
   obj->preferredNC = (UBOOL8)temp;

   obj->QAM256Capable = node_status.qam_256_support;

   if (cmsNet_getMacAddrStringByIfname(intfName, macAddrBuf) == CMSRET_SUCCESS)
   {
      REPLACE_STRING_IF_NOT_EQUAL_FLAGS(obj->MACAddress, macAddrBuf, mdmLibCtx.allocFlags);
   }

   obj->currentOperFreq = if_status.rf_channel;
   moca_get_lof(pMoca, &obj->lastOperFreq);
   obj->X_BROADCOM_COM_BwStatus = net_status.bw_status;

   moca_get_privacy_en(pMoca, &temp);
   obj->privacyEnabled = (UBOOL8)temp;

   moca_close(pMoca);

   return;
}


void rutMoca_getStats_dev2(const char *intfName, Dev2MocaInterfaceStatsObject *obj)
{
   UINT64 errorsReceived = 0, discardPacketsReceived = 0;
   UINT64 errorsSent = 0, discardPacketsSent = 0;

   /*
    * We are getting the stats from the enet driver instead of directly
    * from the Moca driver.  See also rutMoca_getStats.
    */
   rut_getIntfStats_uint64(intfName, &obj->bytesReceived, &obj->packetsReceived,
                           &obj->multicastBytesReceived, &obj->multicastPacketsReceived,
                           &obj->unicastPacketsReceived, &obj->broadcastPacketsReceived,
                           &errorsReceived, &discardPacketsReceived,
                           &obj->bytesSent, &obj->packetsSent,
                           &obj->multicastBytesSent, &obj->multicastPacketsSent,
                           &obj->unicastPacketsSent, &obj->broadcastPacketsSent,
                           &errorsSent, &discardPacketsSent);

   obj->errorsReceived = (UINT32)errorsReceived;
   obj->discardPacketsReceived = (UINT32)discardPacketsReceived;

   obj->errorsSent = (UINT32)errorsSent;
   obj->discardPacketsSent = (UINT32)discardPacketsSent;

#ifndef DESKTOP_LINUX
   {
      void * pMoca = NULL;
      struct moca_gen_stats stats;
      int mocaRet;

      pMoca = moca_open((char *)intfName);

      if (pMoca != NULL)
      {
         mocaRet = moca_get_gen_stats(pMoca, 0, &stats);
         moca_close(pMoca);
         if (mocaRet != MOCA_API_SUCCESS)
         {
            cmsLog_error("get stats on %s failed, ret=%d", intfName, mocaRet);
         }
         else
         {
            obj->X_BROADCOM_COM_NCHandOffs = stats.nc_handoff_counter;
            obj->X_BROADCOM_COM_NCBackups = stats.nc_backup_counter;
         }
      }
      else
      {
         cmsLog_error("moca_open of %s failed", intfName);
      }
   }

#endif /* DESKTOP_LINUX */

   return;
}


#endif  /* DMP_DEVICE2_MOCA_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */
