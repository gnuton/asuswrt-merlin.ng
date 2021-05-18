/***********************************************************************
<:copyright-BRCM:2012:proprietary:standard 

   Copyright (c) 2012 Broadcom 
   All Rights Reserved

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
#include "cms_msg.h"
#include "rcl.h"
#include "rut_system.h"
#include "rut_util.h"
#include "rut_upnp.h"
#include "rut_dnsproxy.h"
#include "rut_lan.h"
#include "rut_network.h"


CmsRet rcl_syslogCfgObject( _SyslogCfgObject *newObj,
                const _SyslogCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;

   /* newObj can never be NULL since this is an indirect0 object, it
    * can never be deleted. */


   if (((currObj == NULL) && (!strcmp(newObj->status, MDMVS_ENABLED))) ||
       ((currObj != NULL) && (!strcmp(currObj->status, MDMVS_DISABLED)) && (!strcmp(newObj->status, MDMVS_ENABLED))))
   {
      cmsLog_debug("syslogd is being turned on");
      ret = rut_restartsysklogd(newObj);
   }
   else if ((currObj != NULL) && (!strcmp(currObj->status, MDMVS_ENABLED)) && (!strcmp(newObj->status, MDMVS_DISABLED)))
   {
      cmsLog_debug("syslogd is being turned off");
      rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_KLOGD, NULL, 0);
      rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_SYSLOGD, NULL, 0);
   }
   else if ((currObj != NULL) && (rut_isSyslogCfgChanged(newObj, currObj))) 
   {
      cmsLog_debug("syslogd config has changed, restart");
      ret = rut_restartsysklogd(newObj);
   }

   return ret;
}


CmsRet rcl_loginCfgObject( _LoginCfgObject *newObj,
                const _LoginCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam,
                CmsRet *errorCode)
{
   CmsRet ret = CMSRET_SUCCESS;
#ifdef SUPPORT_HASHED_PASSWORDS
   char * pPassWd;

   if ((currObj == NULL)  ||
       (strcmp(newObj->adminPasswordHash, currObj->adminPasswordHash)) ||
       (strcmp(newObj->supportPasswordHash, currObj->supportPasswordHash)) ||
       (strcmp(newObj->userPasswordHash, currObj->userPasswordHash)))
   {
      if (NULL == currObj)
      {
         /* if hashed password is NULL or empty then hash the clear
            password and set it */
         if (NULL == newObj->adminPasswordHash)
         {
            pPassWd = cmsUtil_pwEncrypt( newObj->adminPassword, cmsUtil_cryptMakeSalt());
            newObj->adminPasswordHash = cmsMem_strdupFlags(pPassWd, mdmLibCtx.allocFlags);
         }

         if (NULL == newObj->supportPasswordHash)
         {
            pPassWd = cmsUtil_pwEncrypt( newObj->supportPassword, cmsUtil_cryptMakeSalt());
            newObj->supportPasswordHash = cmsMem_strdupFlags(pPassWd, mdmLibCtx.allocFlags);
         }

         if (NULL == newObj->userPasswordHash)
         {
#ifdef SUPPORT_IEEE1905_GOLDENNODE
            pPassWd = cmsUtil_pwEncrypt( "ncap", cmsUtil_cryptMakeSalt());
#else
            pPassWd = cmsUtil_pwEncrypt( newObj->userPassword, cmsUtil_cryptMakeSalt());
#endif
            newObj->userPasswordHash = cmsMem_strdupFlags(pPassWd, mdmLibCtx.allocFlags);
         }
      }
      ret = rut_createLoginCfg(newObj->adminPasswordHash,
                               newObj->supportPasswordHash,
                               newObj->userPasswordHash );
#else
   if ((currObj == NULL)  ||
       (strcmp(newObj->adminPassword, currObj->adminPassword)) ||
       (strcmp(newObj->supportPassword, currObj->supportPassword)) ||
       (strcmp(newObj->userPassword, currObj->userPassword)))
   {
      ret = rut_createLoginCfg(newObj->adminPassword,
                               newObj->supportPassword,
                               newObj->userPassword);
#endif
      if (ret != CMSRET_SUCCESS)
      {
         /*
          * Only possible error from rut_createLoginCfg is if the
          * /etc/password file could not be created.
          * Just set the errorParam on the adminPassword and pass back
          * the error.
          * Not necessary to allocate errorParam in shared memory, but
          * its easier to consistently allocated from shared memory inside
          * RCL handler functions.
          */
         *errorParam = cmsMem_strdupFlags("adminPassword", mdmLibCtx.allocFlags);
         *errorCode = ret;
      }
   }

   return ret;
}


CmsRet rcl_appCfgObject( _AppCfgObject *newObj __attribute__((unused)),
                const _AppCfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /* DONE.  Nothing needs to be done in this rcl handler function. */
   return CMSRET_SUCCESS;
}

CmsRet rcl_httpdCfgObject( _HttpdCfgObject *newObj,
                const _HttpdCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */
   
   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_HTTPD, newObj->loggingLevel);

      rut_updateLogDestination(EID_HTTPD, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}

CmsRet rcl_vectoringCfgObject( _VectoringCfgObject *newObj,
                               const _VectoringCfgObject *currObj,
                               const InstanceIdStack *iidStack __attribute__((unused)),
                               char **errorParam __attribute__((unused)),
                               CmsRet *errorCode __attribute__((unused)))
{

   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */
   
   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_VECTORINGD, newObj->loggingLevel);

      rut_updateLogDestination(EID_VECTORINGD, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}

CmsRet rcl_tr69cCfgObject( _Tr69cCfgObject *newObj,
                const _Tr69cCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      char *ConfigId = MULTI_TR69C_CONFIG_INDEX_1; /* config#1 changed */

#ifndef SUPPORT_BEEP_TR69C
      rut_updateLogLevel(EID_TR69C, newObj->loggingLevel);
      rut_updateLogDestination(EID_TR69C, newObj->loggingDestination);
      rut_updateLogSOAP(EID_TR69C, ConfigId);
#else
      rut_updateLogSOAP(EID_SMD, ConfigId);
#endif
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}


#ifdef DMP_X_BROADCOM_COM_TR64_1
CmsRet rcl_tr64cCfgObject( _Tr64cCfgObject *newObj,
                const _Tr64cCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_TR64C, newObj->loggingLevel);

      rut_updateLogDestination(EID_TR64C, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}
#endif  /* DMP_X_BROADCOM_COM_TR64_1 */


CmsRet rcl_sshdCfgObject( _SshdCfgObject *newObj,
                const _SshdCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_SSHD, newObj->loggingLevel);

      rut_updateLogDestination(EID_SSHD, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}

CmsRet rcl_telnetdCfgObject( _TelnetdCfgObject *newObj,
                const _TelnetdCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_TELNETD, newObj->loggingLevel);

      rut_updateLogDestination(EID_TELNETD, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}

CmsRet rcl_consoledCfgObject( _ConsoledCfgObject *newObj,
                const _ConsoledCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_CONSOLED, newObj->loggingLevel);

      rut_updateLogDestination(EID_CONSOLED, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}

CmsRet rcl_smdCfgObject( _SmdCfgObject *newObj,
                const _SmdCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_SMD, newObj->loggingLevel);

      rut_updateLogDestination(EID_SMD, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}

CmsRet rcl_sskCfgObject( _SskCfgObject *newObj,
                const _SskCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_SSK, newObj->loggingLevel);

      rut_updateLogDestination(EID_SSK, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}


CmsRet rcl_snmpdCfgObject( _SnmpdCfgObject *newObj,
                const _SnmpdCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_SNMPD, newObj->loggingLevel);

      rut_updateLogDestination(EID_SNMPD, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}


CmsRet rcl_ftpdCfgObject( _FtpdCfgObject *newObj,
                const _FtpdCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_FTPD, newObj->loggingLevel);

      rut_updateLogDestination(EID_FTPD, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}


CmsRet rcl_tftpdCfgObject( _TftpdCfgObject *newObj,
                const _TftpdCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_TFTPD, newObj->loggingLevel);

      rut_updateLogDestination(EID_TFTPD, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}

CmsRet rcl_pppdCfgObject( _PppdCfgObject *newObj,
                const _PppdCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_PPP, newObj->loggingLevel);

      rut_updateLogDestination(EID_PPP, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}

CmsRet rcl_icmpCfgObject( _IcmpCfgObject *newObj __attribute__((unused)),
                const _IcmpCfgObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{

   /* nothing needs to be done for this object (delete it?) */
     
   return CMSRET_SUCCESS;
}


#ifdef DMP_X_BROADCOM_COM_UPNP_1 /* aka SUPPORT_UPNP */

CmsRet rcl_upnpCfgObject( _UpnpCfgObject *newObj,
                const _UpnpCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret =CMSRET_SUCCESS;

   /* newObj can never be NULL since this is an indirect0 object, it
    * can never be deleted. */

   if (newObj && !currObj)
   {
      /* do nothing in the system init */
      return ret;
   }
   else if  (currObj && (currObj->enable == FALSE) && (newObj->enable == TRUE))           /* change from disabled to enabled */
   {
      /* just restart upnp */
      if ((ret = rut_restartUpnp(NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to start upnp process.");
      }
   }
   else if(POTENTIAL_CHANGE_OF_EXISTING(newObj,currObj))
   {
      if(currObj->loggingLevel != newObj->loggingLevel)
      {
         rut_updateLogLevel(EID_UPNP, newObj->loggingLevel);
      }
      if(currObj->loggingDestination != newObj->loggingDestination)
      {
         rut_updateLogDestination(EID_UPNP, newObj->loggingDestination);
      }
   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /* was running, now need to stop it */
      if ((ret = rut_stopUpnp()) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to remove upnp process.");
      }
   }

   return ret;
   
}

#endif


#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1 /* aka SUPPORT_DNSPROXY */

CmsRet rcl_dnsProxyCfgObject( _DnsProxyCfgObject *newObj,
                const _DnsProxyCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret =CMSRET_SUCCESS;

   /* newObj can never be NULL since this is an indirect0 object, it
    * can never be deleted. */

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* on system init, start dnsproxy if it's enabled */
      ret = rutDpx_updateDnsproxy();
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      if (newObj->deviceHostName == NULL || newObj->deviceDomainName == NULL)
      {
         cmsLog_error("dproxy process requires domain/host name.");
         return CMSRET_INVALID_ARGUMENTS;
      }


      if (cmsUtl_strcmp(newObj->deviceHostName, currObj->deviceHostName) ||
          cmsUtl_strcmp(newObj->deviceDomainName, currObj->deviceDomainName))
      {

         rutDpx_updateDnsproxy();

         /* re-config and restart dhcpd because of domain name change */
         if (cmsUtl_strcmp(newObj->deviceDomainName, currObj->deviceDomainName))
         {
            rutLan_updateDhcpd();
         }
      }

      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.
       */
      rut_updateLogLevel(EID_DNSPROXY, newObj->loggingLevel);
      rut_updateLogDestination(EID_DNSPROXY, newObj->loggingDestination);

   }
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      /* was running, now need to stop it */
      if ((ret = rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_DNSPROXY, NULL,0)) != CMSRET_SUCCESS)
      {
         cmsLog_error("failed to stop dnsproxy.");
      }         

      /* Need to reconfigure the /etc/resolv.conf since dnsproxy is disabled. */
      rutDpx_updateDnsproxy();
      /*update the latest information to udhcpd.conf and inform dhcpd*/
      rutLan_updateDhcpd();
   }

   return ret;
   
}

#endif


#ifdef DMP_X_ITU_ORG_GPON_1
CmsRet rcl_omcidCfgObject( _OmcidCfgObject *newObj,
                const _OmcidCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_OMCID, newObj->loggingLevel);

      rut_updateLogDestination(EID_OMCID, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */

   return CMSRET_SUCCESS;
}

#endif /* DMP_X_ITU_ORG_GPON_1 */

#ifdef BRCM_VOICE_SUPPORT
#  ifdef DMP_X_BROADCOM_COM_DECTD_1  /* aka dectd */
CmsRet rcl_dectdCfgObject( _DectdCfgObject *newObj,
                const _DectdCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   CmsRet ret =CMSRET_SUCCESS;

   /* newObj can never be NULL since this is an indirect0 object, it
    * can never be deleted. */

   if (newObj && !currObj)
   {
      /* do nothing in the system init */
      return ret;
   }
   else if(POTENTIAL_CHANGE_OF_EXISTING(newObj,currObj))
   {
      if(currObj->loggingLevel != newObj->loggingLevel)
      {
         rut_updateLogLevel(EID_DECT, newObj->loggingLevel);
      }
      if(currObj->loggingDestination != newObj->loggingDestination)
      {
         rut_updateLogDestination(EID_DECT, newObj->loggingDestination);
      }
   }

   return ret;
}
#  endif
#endif

#ifdef DMP_X_BROADCOM_COM_EPON_1
CmsRet rcl_eponappCfgObject( _EponappCfgObject *newObj,
                const _EponappCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{   
   if (newObj != NULL && currObj != NULL)
   {
      rut_updateLogLevel(EID_EPON_APP, newObj->loggingLevel);
      rut_updateLogDestination(EID_EPON_APP, newObj->loggingDestination);
   }

   return CMSRET_SUCCESS;
}
#endif


CmsRet rcl_xmppcCfgObject( _XmppcCfgObject *newObj,
                const _XmppcCfgObject *currObj,
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   /*
    * Don't need to do anything during system startup/object creation.
    * The app will read the config and set its own log level and dest appropriately.
    */

   if (newObj != NULL && currObj != NULL)
   {
      /*
       * Always send the message to the destination in case
       * the target app and its configured log level/destination
       * is out of sync.  This can happen in smd & ssk if CMS_STARTUP_DEBUG
       * is set.
       */
      rut_updateLogLevel(EID_XMPPC, newObj->loggingLevel);

      rut_updateLogDestination(EID_XMPPC, newObj->loggingDestination);
   }

   /* This object cannot be deleted, so no need to handle that case. */
     
   return CMSRET_SUCCESS;
}


