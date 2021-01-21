/***********************************************************************
 *
 *  Copyright (c) 2009  Broadcom Corporation
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


#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_moca.h"
#include "rut_wanlayer2.h"

#include "mocalib.h"
#include "mocalib-cli.h"


#ifdef SUPPORT_MOCA

typedef struct origMocaParams 
{
   UINT32 o_rftype;
   UINT32 o_turboEn;
   UINT32 o_dontstartmoca;
} origMocaParams_t;


#ifdef DMP_X_BROADCOM_COM_MOCAWAN_1
UBOOL8 rutMoca_getWanMocaIntfByIfName(const char *ifName, InstanceIdStack *iidStack, WanMocaIntfObject **mocaIntfCfg)
{
   WanMocaIntfObject *wanMoca=NULL;
   CmsRet ret;
   UBOOL8 found=FALSE;

   if ((ret = rutWl2_getWanMocaObject(iidStack, &wanMoca)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Cannot get WanMocaIntfObject, ret = %d", ret);
   }
   else
   {
      if (cmsUtl_strcmp(wanMoca->ifName, ifName))
      {
         /* ifname is not same, skip this object. */
         cmsLog_debug("found WANMoca, but ifNames do not match, looking for %s got %s",
                      ifName, wanMoca->ifName);
         cmsObj_free((void **)&wanMoca);
      }
      else
      {
         found = TRUE;
         if (mocaIntfCfg != NULL)
         {
            /* give object back to caller, so don't free */
            *mocaIntfCfg = wanMoca;
         }
         else
         {
            cmsObj_free((void **)&wanMoca);
         }
      }
   }

   cmsLog_debug("found=%d, ifName=%s", found, ifName);
   
   return found;
}
#endif /*  DMP_X_BROADCOM_COM_MOCAWAN_1 */

static void mocacli_convert_init_string(const char * str)
{
   cmsLog_debug("executing str=%s", str);
   system(str);
   return;
}

static void mocacli_convert_cfg_string(char *mocaObj __attribute__((unused)))
{
   return;
}

static void mocacli_convert_trace_string(char * str __attribute__((unused)))
{
   return;
}

static CmsRet rutMoca_copyInitParms(   void * pMoca, 
                            UBOOL8 autoNwSearch, const char *initParmsString)
{
   origMocaParams_t   orig;
   origMocaParams_t   new;
   CmsRet nRet = CMSRET_SUCCESS;

   moca_get_rf_band(pMoca, &orig.o_rftype);
   moca_get_turbo_en(pMoca, &orig.o_turboEn);
   moca_get_dont_start_moca(pMoca, &orig.o_dontstartmoca);

   /* Use autoNwSearch from CMS if not set in --singleCh string option */
   moca_set_single_channel_operation(pMoca, autoNwSearch ? 0 : 1);
   mocacli_convert_init_string(initParmsString);

   moca_get_rf_band(pMoca, &new.o_rftype);
   moca_get_turbo_en(pMoca, &new.o_turboEn);
   moca_get_dont_start_moca(pMoca, &new.o_dontstartmoca);

   if ((orig.o_dontstartmoca != 2) && 
       (((orig.o_rftype != 0xFFFFFFFF) && (orig.o_rftype != new.o_rftype)) ||
        (new.o_turboEn != orig.o_turboEn) ))
   {
      // reset to defaults if user changes rfType
      printf("WARNING: rfType, or turbo mode, setting config and init parms to defaults\n");

      printf("TBD: %s - set init params\n", __FUNCTION__);
      
      nRet = moca_set_restore_defaults(pMoca);
      if (nRet != CMSRET_SUCCESS)
         return nRet;

//      nRet = MoCACtl2_GetInitParms( pMoca, pInitParms); 
//      if (nRet != CMSRET_SUCCESS)
//         return nRet;

//      pInitParms->initOptions.dontStartMoca = 0;

      // reparse for any changed parameters
      mocacli_convert_init_string(initParmsString);

      // if LOF not set, use 0
//      if (pInitParms->nvParams.lastOperFreq == MoCA_FREQ_UNSET)
//      {
//         if (pInitParms->rfType == MoCA_RF_TYPE_C4_BAND)
//            pInitParms->nvParams.lastOperFreq = 1000;
//         else
//            pInitParms->nvParams.lastOperFreq = 0;
//      }
   }
   else
   {   
      /* Use the legacy parameters as-is from the structure, except for autoNwSearch */
//      pInitParms->privacyEn = mocaObj->privacy;
//      pInitParms->nvParams.lastOperFreq = mocaObj->lastOperationalFrequency;
//      pInitParms->passwordSize = cmsUtl_strlen(mocaObj->password);
//      memcpy((char *)pInitParms->password, mocaObj->password, sizeof(pInitParms->password));
   }

   return( nRet );
}

static CmsRet rutMoca_copyConfigParms(const LanMocaIntfObject *mocaObj)
{
   mocacli_convert_cfg_string(mocaObj->configParmsString);
   
   return( CMSRET_SUCCESS );
}


CmsRet rutMoca_getInitParms(void * mocaCtx, const char *intfName,
                            UBOOL8 *autoNwSearch, UBOOL8 *privacy,
                            UINT32 *lastOperationalFrequency,
                            char **passwordString, char **initParmsString)
{
   CmsRet ret = CMSRET_SUCCESS;
   static char initString[20480];
   int freeCtx = 0;
   struct moca_password password;
   UINT32 temp;

   /* If the caller doesn't have a handle yet, try to open it */
   if (mocaCtx == NULL)
   {
      mocaCtx = moca_open((char *)intfName);
      if (mocaCtx != NULL)
         freeCtx = 1;
      else 
         return(CMSRET_REQUEST_DENIED);
   }
   
   
   /* Get the current init parameters to save to  flash */
   sprintf(initString, "mocap %s set ", intfName);
   moca_write_nvram(mocaCtx, &initString[strlen(initString)], (sizeof(initString) - strlen(initString)));
   if (strlen(initString) > 2048)
   {
      cmsLog_error("Warning: initString is %d bytes!! this may cause problems", strlen(initString));
   }
   cmsMem_free(*initParmsString);
   *initParmsString = cmsMem_strdupFlags(initString, mdmLibCtx.allocFlags);

   moca_get_single_channel_operation(mocaCtx, &temp);
   *autoNwSearch = (UBOOL8)temp ? FALSE : TRUE;

   moca_get_privacy_en(mocaCtx, &temp);
   *privacy = (UBOOL8)temp;

   moca_get_lof(mocaCtx, lastOperationalFrequency);

   moca_get_password(mocaCtx, &password);
   cmsMem_free(*passwordString);
   *passwordString = cmsMem_strdupFlags((const char *)password.password, mdmLibCtx.allocFlags);

   if (freeCtx)
      moca_close(mocaCtx);

   return( ret );
}

CmsRet rutMoca_updateConfigParms(void * mocaCtx __attribute__((unused)),
                     LanMocaIntfObject *mocaObj __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   return( ret );
}

CmsRet rutMoca_updateTraceParms(void * mocaCtx __attribute__((unused)),
                    LanMocaIntfObject *mocaObj __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   return( ret );
}

CmsRet rutMoca_initialize(const char *intfName, UINT32 lastOperFreq)
{
   CmsRet ret= CMSRET_SUCCESS;
   void * pMoca = NULL;
#ifdef BRCM_MOCA_DAEMON
   SINT32 pid;
   char args[128];
#endif
   int i;

   cmsLog_debug("Entered, mocaInitDone=%d", mdmShmCtx->mocaInitDone);

   if (0 == strncmp(intfName, "moca0", 5))
   {
      if (mdmShmCtx->mocaInitDone & 0x1)
      {
         cmsLog_debug("%s already initialized, no need to do it again", intfName);
         return CMSRET_SUCCESS;
      }
      else
      {
         mdmShmCtx->mocaInitDone |= 1;
      }
   }
   else if (0 == strncmp(intfName, "moca1", 5))
   {
      if (mdmShmCtx->mocaInitDone & 0x2)
      {
         cmsLog_debug("%s already initialized, no need to do it again", intfName);
         return CMSRET_SUCCESS;
      }
      else
      {
         mdmShmCtx->mocaInitDone |= 2;
      }
   }
   else
   {
      cmsLog_error("Unexpected moca intfName %s", intfName);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /*
    * This will happen during system bootup.
    * Moca (and GPON objects in general) does not have an enable
    * parameter like the DSL Forum objects, so I guess we just
    * enable the moca on system bootup.
    */

#ifdef DESKTOP_LINUX
   return CMSRET_SUCCESS;
#endif

#ifdef BRCM_MOCA_DAEMON
/*   snprintf(args, sizeof(args), "-D -f /etc/moca/moca1%dcore.bin -d /dev/bmoca0 -w -vvv",*/
   snprintf(args, sizeof(args), "-d /dev/b%s -w -f /etc/moca/moca20core.bin -i %s -F %d",
            intfName,
            intfName,
            lastOperFreq);
   {
      pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_MOCAD, args, strlen(args)+1);
      if (pid == CMS_INVALID_PID) {
         cmsLog_error("failed to start mocad");
         ret = CMSRET_INTERNAL_ERROR;
      }
   }
   usleep(1000);

   /* it can take a while for the first mocad to start
      wait up to 1 second */
   for (i = 0; i < 100; i++)
   {
      if (rut_isApplicationRunning(MAKE_SPECIFIC_EID(pid, EID_MOCAD)))
         break;
      usleep(10000);
   }
   cmsLog_debug("wait count for mocad %u", i);

#endif

   for (i = 0; i < 20; i++)
   {
      pMoca = moca_open((char *) intfName);
      if (pMoca != NULL)
      {
         moca_close(pMoca);
         break;
      }
      else
      {
         usleep(10000);
      }
   }
   if ( (20 == i) || (ret != CMSRET_SUCCESS) )
   {
      cmsLog_error("unable to initialize '%s'", intfName);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
}


CmsRet rutMoca_start(const char *intfName,
                     UBOOL8 *autoNwSearch, UBOOL8 *privacy,
                     UINT32 *lastOperationalFrequency,
                     char **password, char **initParmsString)
{
   CmsRet ret= CMSRET_SUCCESS;
   void * pMoca = NULL;
   int mocaRet;

   cmsLog_debug("Entered: intfName=%s autoNwSearch=%d initParmsString=%s",
                intfName, *autoNwSearch, *initParmsString);

   /*
    * This will happen during system bootup.
    * Moca (and GPON objects in general) does not have an enable
    * parameter like the DSL Forum objects, so I guess we just
    * enable the moca on system bootup.
    */

#ifdef DESKTOP_LINUX
   return CMSRET_SUCCESS;
#endif

   pMoca = moca_open((char *)intfName);
   if (pMoca != NULL)
   {
      ret = rutMoca_copyInitParms( pMoca, *autoNwSearch, *initParmsString );

      if (ret == CMSRET_SUCCESS)
      {
         cmsLog_debug("calling moca_set_start");
         mocaRet = moca_set_start ( pMoca ) ;

         if (mocaRet != MOCA_API_SUCCESS)
         {
            cmsLog_error("failed to start moca: ret = %d", mocaRet);
            ret = CMSRET_INTERNAL_ERROR;
         }
      }
      else
         cmsLog_error("failed to copy init '%s'", intfName);

      if (ret == CMSRET_SUCCESS)
      {
         ret = rutMoca_getInitParms(pMoca, intfName,
                                       autoNwSearch, privacy,
                                       lastOperationalFrequency,
                                       password, initParmsString);
         if (ret != CMSRET_SUCCESS)
            cmsLog_error("failed to update init string '%s'", intfName);

      }

      moca_close( pMoca );
   }
   else
   {
      cmsLog_error("failed to iniitalize '%s'", intfName);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
}


CmsRet rutMoca_stop(const char *intfName)
{
   CmsRet ret= CMSRET_SUCCESS;
   void * pMoca = NULL;

   cmsLog_debug("Entered: intfName=%s", intfName);

#ifdef DESKTOP_LINUX
   return CMSRET_SUCCESS;
#endif

   pMoca = moca_open((char *)intfName);
   if (pMoca != NULL)
   {
      cmsLog_debug("calling moca_set_stop");

      ret = moca_set_stop ( pMoca ) ;

      moca_close( pMoca );
   }
   else
   {
      cmsLog_error("unable to get handle for '%s'", intfName);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
}

CmsRet rutMoca_reinitialize(const char *intfName,
                            UBOOL8 *autoNwSearch, UBOOL8 *privacy,
                            UINT32 *lastOperationalFrequency,
                            char **password, char **initParmsString)
{
   CmsRet ret = CMSRET_SUCCESS;
   void * pMoca = NULL;

   cmsLog_debug("Entered: intfName=%s", intfName);

#ifdef DESKTOP_LINUX
   return CMSRET_SUCCESS;
#endif

   pMoca = moca_open((char *) intfName);
   if (pMoca != NULL)
   {
      ret = rutMoca_copyInitParms( pMoca, *autoNwSearch, *initParmsString );

      if (ret == CMSRET_SUCCESS)
         ret = moca_set_restart(pMoca);
      else
         cmsLog_error("failed to copy init '%s'", intfName);

      if (ret == CMSRET_SUCCESS)
      {
         ret = rutMoca_getInitParms(pMoca, intfName,
                                    autoNwSearch, privacy,
                                    lastOperationalFrequency,
                                    password, initParmsString);
         if (ret != CMSRET_SUCCESS)
            cmsLog_error("failed to update init string '%s'", intfName);

      }

      moca_close(pMoca);
   }
   else
   {
      cmsLog_error("unable to get handle for '%s'", intfName);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return (ret);
}

CmsRet rutMoca_setParams(LanMocaIntfObject *mocaObj,
                   const LanMocaIntfObject *currObj __attribute__((unused)))
{
   CmsRet ret= CMSRET_SUCCESS;
   void * pMoca = NULL;
//   MoCA_CONFIG_PARAMS cfgParms;
//   unsigned long long configMask = 0;

   cmsLog_debug("entered:");
#ifdef DESKTOP_LINUX
   return CMSRET_SUCCESS;
#endif

   pMoca = moca_open(mocaObj->ifName);
   if (pMoca != NULL)
   {
      //cmsLog_debug("calling MoCACtl2_SetCfg");

      //ret = MoCACtl2_GetCfg(pMoca, &cfgParms, MoCA_CFG_NON_LAB_PARAM_ALL_MASK);

      if (ret == CMSRET_SUCCESS)
         ret = rutMoca_copyConfigParms( mocaObj );
      else
         cmsLog_error("failed to get cfg parms '%s'", mocaObj->ifName);

//      if (ret == CMSRET_SUCCESS)
//         ret = MoCACtl2_SetCfg ( pMoca ) ;

      if (ret == CMSRET_SUCCESS)
      {
         ret = rutMoca_updateConfigParms(pMoca, mocaObj);
         if (ret != CMSRET_SUCCESS)
            cmsLog_error("failed to update config string '%s'", mocaObj->ifName);

      }

      moca_close( pMoca );
   }
   else
   {
      cmsLog_error("unable to get handle for '%s'", mocaObj->ifName);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
}


CmsRet rutMoca_setTrace(LanMocaIntfObject *mocaObj)
{
   CmsRet ret= CMSRET_SUCCESS;
//   MoCA_TRACE_PARAMS   traceParms ;
   void * pMoca = NULL;

   cmsLog_debug("Entered");

#ifdef DESKTOP_LINUX
   return CMSRET_SUCCESS;
#endif

   mocacli_convert_trace_string(mocaObj->traceParmsString);
   
   /* this function is still getting the settings from scratch pad.
    * Just need to convert the mocactl to use MDM, and we can make the switch */
   pMoca = moca_open(mocaObj->ifName);

   if (pMoca != NULL)
   {
      //ret = MoCACtl2_SetTraceConfig ( pMoca, &traceParms ) ;

      if (ret == CMSRET_SUCCESS)
      {
         ret = rutMoca_updateTraceParms(pMoca, mocaObj);
         if (ret != CMSRET_SUCCESS)
            cmsLog_error("failed to update trace string '%s'", mocaObj->ifName);

      }

      moca_close( pMoca );
   }
   else
   {
      cmsLog_error("unable to get handle for '%s'", mocaObj->ifName);
      ret = CMSRET_INTERNAL_ERROR;
   }
   
   return ret;
}


const char *rutMoca_getLinkStatus(const char *ifName)
{
   struct moca_interface_status driverStatus;
   void * pMoca = NULL;
   int mocaRet;

   cmsLog_debug("Entered");

#ifdef DESKTOP_LINUX
   return MDMVS_UP;
#endif

   memset(&driverStatus, 0, sizeof(struct moca_interface_status));

   pMoca = moca_open((char *)ifName);

   if (pMoca != NULL)
   {
      mocaRet = moca_get_interface_status(pMoca, &driverStatus);
      moca_close(pMoca);
      if (mocaRet != MOCA_API_SUCCESS)
      {
         cmsLog_error("get status on %s failed, ret=%d", ifName, mocaRet);
         return MDMVS_ERROR;
      }
   }
   else
   {
      cmsLog_error("open of %s failed", ifName);
      return MDMVS_ERROR;
   }

   if (driverStatus.link_status)
   {
      return MDMVS_UP;
   }
   else
   {
      return MDMVS_NOLINK;
   }
}

CmsRet rutMoca_getInterfaceData(const char *ifName, LanMocaIntfObject *mocaObj)
{
   CmsRet ret = CMSRET_SUCCESS;
   void * pMoca = NULL;
   struct moca_snr_margin_rs snr_mgn_rs;
   struct moca_password pwd;
   struct moca_taboo_channels taboo;
   UINT32 temp;
   
   cmsLog_debug("Entered");

#ifdef DESKTOP_LINUX
   return CMSRET_SUCCESS;
#endif

   pMoca = moca_open((char *)ifName);

   if (pMoca != NULL)
   {
      moca_get_max_frame_size(pMoca, &mocaObj->maxFrameSize);
      moca_get_max_transmit_time(pMoca, &mocaObj->maxTransmitTime);
      moca_get_min_bw_alarm_threshold(pMoca, &mocaObj->minBandwidthAlarmThreshold);
      mocaObj->outOfOrderLmo = 0;
      mocaObj->continuousIerrInsert = 0;
      mocaObj->continuousIeMapInsert = 0;
      moca_get_max_pkt_aggr(pMoca, &mocaObj->maxPktAggr);
      moca_get_pmk_exchange_interval(pMoca, &mocaObj->pmkExchangeInterval);
      moca_get_tek_exchange_interval(pMoca, &mocaObj->tekExchangeInterval);
      moca_get_snr_margin_rs(pMoca, &snr_mgn_rs);
      mocaObj->snrMargin = snr_mgn_rs.base_margin;

      moca_get_nc_mode(pMoca, &mocaObj->ncMode);
      moca_get_single_channel_operation(pMoca, &temp);
      mocaObj->autoNwSearch = (UBOOL8)temp ? FALSE : TRUE;
      moca_get_privacy_en(pMoca, &temp);
      mocaObj->privacy = (UBOOL8)temp;
      moca_get_tpc_en(pMoca, &temp);
      mocaObj->txPwrControl = (UBOOL8)temp;
      moca_get_continuous_power_tx_mode(pMoca, &mocaObj->continuousPowerMode);
      moca_get_lof(pMoca, &mocaObj->lastOperationalFrequency);
      moca_get_listening_freq_mask(pMoca, &mocaObj->frequencyMask); //initParms.freqMask; obsolete

      moca_get_password(pMoca, &pwd);
      if (mocaObj->password != NULL)
         cmsMem_free(mocaObj->password);
      mocaObj->password = cmsMem_strndupFlags((const char *)pwd.password,
                              sizeof(pwd.password), mdmLibCtx.allocFlags);

      moca_get_multicast_mode(pMoca, &mocaObj->mcastMode);
      moca_get_lab_mode(pMoca, &temp);
      mocaObj->labMode = (UBOOL8)temp;
      moca_get_taboo_channels(pMoca, &taboo);
      mocaObj->tabooMaskStart = taboo.taboo_fixed_mask_start; // initParms.tabooStartChan; obsolete
      mocaObj->tabooChannelMask = taboo.taboo_fixed_channel_mask; //initParms.tabooChanMask; obsolete
      moca_get_preferred_nc(pMoca, &temp);
      mocaObj->preferedNetworkController = (UBOOL8)temp;
      moca_get_loopback_en(pMoca, &mocaObj->loopbackConfiguration);
      mocaObj->mrNonDefSeqNumber = 0;
      printf("TBD: %s - non def seq num\n", __FUNCTION__);

      moca_get_verbose(pMoca, &mocaObj->traceLevel);
      mocaObj->traceLevelRestoreDefault = 0;

      moca_close(pMoca);
   }
   else
   {
      cmsLog_error("open of %s failed", ifName);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
}


CmsRet rutMoca_getStatus(const char *ifName, LanMocaIntfStatusObject *mocaStatus)
{
   CmsRet ret = CMSRET_SUCCESS;
   struct moca_drv_info drv_info;
   struct moca_node_status node_status;
   struct moca_fw_version fw_version;
   struct moca_network_status net_status;
   struct moca_interface_status if_status;
   void * pMoca = NULL;

   cmsLog_debug("Entered");

#ifdef DESKTOP_LINUX
   return CMSRET_SUCCESS;
#endif

   pMoca = moca_open((char *)ifName);

   if (pMoca != NULL)
   {
      moca_get_node_status(pMoca, &node_status);
      moca_get_drv_info(pMoca, 0, &drv_info);
      moca_get_fw_version(pMoca, &fw_version);
      moca_get_network_status(pMoca, &net_status);
      moca_get_interface_status(pMoca, &if_status);
         

      /* copy from driver structure to TR-098 object */
      mocaStatus->vendorId = node_status.vendor_id;
      mocaStatus->hwVersion = node_status.moca_hw_version;
      mocaStatus->softwareVersion = node_status.moca_sw_version_major;
      mocaStatus->selfMoCAVersion = node_status.self_moca_version;
      mocaStatus->networkVersionNumber = net_status.network_moca_version;
      mocaStatus->driverMajorVersion = node_status.moca_sw_version_major;
      mocaStatus->driverMinorVersion = node_status.moca_sw_version_minor;
      mocaStatus->driverBuildVersion = node_status.moca_sw_version_rev;
      mocaStatus->qam256Support = node_status.qam_256_support;
      mocaStatus->operationalStatus = 1;
      mocaStatus->linkStatus = if_status.link_status;
      mocaStatus->connectedNodes = net_status.connected_nodes;
      mocaStatus->nodeId = net_status.node_id;
      mocaStatus->networkControllerNodeId = net_status.nc_node_id;
      mocaStatus->upTime = drv_info.core_uptime;
      mocaStatus->linkUpTime = drv_info.link_uptime;
      mocaStatus->backupNetworkControllerNodeId = net_status.backup_nc_id;
      mocaStatus->rfChannel = if_status.rf_channel;
      mocaStatus->bwStatus = net_status.bw_status;

      moca_close(pMoca);
   }
   else
   {
      cmsLog_error("open of %s failed", ifName);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
}


CmsRet rutMoca_getStats(const char *ifName, LanMocaIntfStatsObject *mocaStats)
{
   CmsRet ret = CMSRET_SUCCESS;
   void * pMoca = NULL;
   struct moca_gen_stats stats;
   int mocaRet;
       

#ifdef DESKTOP_LINUX
   return CMSRET_SUCCESS;
#endif

   pMoca = moca_open((char *)ifName);

   if (pMoca != NULL)
   {
      mocaRet = moca_get_gen_stats(pMoca, 0, &stats);
      moca_close(pMoca);
      if (mocaRet != MOCA_API_SUCCESS)
      {
         cmsLog_error("get stats on %s failed, ret=%d", ifName, mocaRet);
         return CMSRET_INTERNAL_ERROR;
      }

      /* copy from driver structure to TR-098 object */
      mocaStats->inUcPkts = stats.ecl_tx_ucast_pkts;
      mocaStats->inDiscardPktsEcl = stats.ecl_tx_mcast_drops + stats.ecl_tx_ucast_drops;
      mocaStats->inDiscardPktsMac = 0; // TBD
      mocaStats->inUnKnownPkts = stats.ecl_tx_ucast_unknown + stats.ecl_tx_mcast_unknown;
      mocaStats->inMcPkts = stats.ecl_tx_mcast_pkts;
      mocaStats->inBcPkts = stats.ecl_tx_bcast_pkts;
      mocaStats->inOctetsLow = stats.ecl_tx_total_bytes;
      mocaStats->outUcPkts = stats.ecl_rx_ucast_pkts;
      mocaStats->outDiscardPkts = stats.ecl_rx_mcast_filter_pkts + stats.ecl_rx_ucast_drops;
      mocaStats->outBcPkts = stats.ecl_rx_bcast_pkts;
      mocaStats->outOctetsLow = stats.ecl_rx_total_bytes;
      mocaStats->networkControllerHandOffs = stats.nc_handoff_counter;
      mocaStats->networkControllerBackups = stats.nc_backup_counter;
   }
   else
   {
      cmsLog_error("open of %s failed", ifName);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
}


CmsRet rutMoca_resetStats(const char *ifName)
{
   CmsRet ret = CMSRET_SUCCESS;
   void * pMoca = NULL;
   int mocaRet;


#ifdef DESKTOP_LINUX
   return CMSRET_SUCCESS;
#endif

   pMoca = moca_open((char *)ifName);

   if (pMoca != NULL)
   {
      mocaRet = moca_set_reset_stats(pMoca);
      moca_close(pMoca);
      if (mocaRet != MOCA_API_SUCCESS)
      {
         cmsLog_error("reset stats on %s failed, ret=%d", ifName, mocaRet);
         ret = CMSRET_INTERNAL_ERROR;
      }
   }
   else
   {
      cmsLog_error("open of %s failed", ifName);
      ret = CMSRET_INTERNAL_ERROR;
   }

   return ret;
}


#ifdef DMP_X_ITU_ORG_GPON_1

CmsRet rutMoca_findPrimaryMocaObject(const InstanceIdStack *gponIidStack,
                                    LanMocaIntfObject **mocaObj,
                                    InstanceIdStack *iidStack)
{
   UINT32 id;
   CmsRet ret = CMSRET_SUCCESS;

   id = PEEK_INSTANCE_ID(gponIidStack);
   if (id != 1)
   {
      cmsLog_error("multiple instances of moca not supported, got %d, expected 1", id);
      return CMSRET_INTERNAL_ERROR;
   }

   INIT_INSTANCE_ID_STACK(iidStack);

   /*
    * Since we only support 1 instance, and it must be on the LAN side, a single
    * getNext is enough.
    */
   ret = cmsObj_getNextFlags(MDMOID_LAN_MOCA_INTF, iidStack, 0, (void **) mocaObj);

   return ret;
}

#endif /* DMP_X_ITU_ORG_GPON_1 */


#endif /* SUPPORT_MOCA */
