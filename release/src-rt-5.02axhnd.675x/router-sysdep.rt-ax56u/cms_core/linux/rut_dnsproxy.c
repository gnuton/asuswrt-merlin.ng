/***********************************************************************
 *
 *  Copyright (c) 2006  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2012:proprietary:standard
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
 *
 ************************************************************************/

#include "cms_util.h"
#include "cms_msg.h"
#include "cms_core.h"
#include "rut_lan.h"
#include "rut_util.h"
#include "rut_system.h"
#include "mdm.h"
#include "rut_pmap.h"
#include "rut_route.h"
#include "rut_dns.h"

/** Send a reload message to dnsproxy process, which will tell it to reload
 *  all config files.
 *
 * @return CmsRet enum.
 */
#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1 /* aka SUPPORT_DNSPROXY */
static CmsRet sendReloadMsgToDnsproxy(void);
#endif

UBOOL8 rutDpx_isEnabled(void)
{
    UBOOL8 dnsProxyEnable = FALSE;

#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1 /* aka SUPPORT_DNSPROXY */
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   _DnsProxyCfgObject *dproxyCfg=NULL;
   CmsRet ret;


   cmsLog_debug("Enter");
   if ((ret = cmsObj_get(MDMOID_DNS_PROXY_CFG, &iidStack, 0, (void **) &dproxyCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get DPROXY_CFG, ret=%d", ret);
   }
   else
   {
      dnsProxyEnable = dproxyCfg->enable;
      cmsObj_free((void **) &dproxyCfg);
   }

   cmsLog_debug("dnsproxy enable status = %d", dnsProxyEnable);
#endif /* DMP_X_BROADCOM_COM_DNSPROXY_1 */

   return dnsProxyEnable;
   
}



CmsRet rutDpx_updateDnsproxy(void)
{
   CmsRet ret = CMSRET_SUCCESS;

   /* Always create /var/dnsinfo.conf, /etc/resolv.conf for the system even
   * dnsproxy is not used 
   */
   rutDns_createDnsInfoConf();
   rutLan_createResolvCfg();

#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1 /* aka SUPPORT_DNSPROXY */

   _DnsProxyCfgObject *dproxyCfg = NULL;   
   InstanceIdStack dnsProxyIidStack = EMPTY_INSTANCE_ID_STACK;


   if ((ret = cmsObj_get(MDMOID_DNS_PROXY_CFG, &dnsProxyIidStack, 0, (void **) &dproxyCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get DnsProxyCfgObject, ret=%d", ret);
      return ret;
   }
   
   if(dproxyCfg->enable)
   {
      /* dnsproxy needs this file, so make sure they are up to date */
#ifndef DESKTOP_LINUX
      rutSys_createHostsFile();
#endif

      /*
       * If DNS proxy is enabled, it should be running.  Even if it is not
       * running, sending a message to it will cause smd to launch it.
       */
      ret = sendReloadMsgToDnsproxy();
   }

   cmsObj_free((void **) &dproxyCfg);	

#endif /* DMP_X_BROADCOM_COM_DNSPROXY_1 */

   return ret;
}


#ifdef DMP_X_BROADCOM_COM_DNSPROXY_1 /* aka SUPPORT_DNSPROXY */

CmsRet sendReloadMsgToDnsproxy(void)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader msg=EMPTY_MSG_HEADER;

   msg.type = CMS_MSG_DNSPROXY_RELOAD;
   msg.src = mdmLibCtx.eid;
   msg.dst = EID_DNSPROXY;
   msg.flags_request = 1;


   /*
    * During dhcp stress tests, this function is called a lot.  So don't
    * bother waiting for a reply from dnsproxy.
    */
   if ((ret = cmsMsg_send(mdmLibCtx.msgHandle, &msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send CMS_MSG_DNSPROXY_RELOAD msg to dnsproxy, ret=%d", ret);
      ret = CMSRET_INTERNAL_ERROR;
   }
   else
   {
      cmsLog_debug("CMS_MSG_DNSPROXY_RELOAD sent successfully");
   }
  
   return ret;
}



#endif  /* SUPPORT_DNSPROXY */

