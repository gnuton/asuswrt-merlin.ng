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
#ifdef SUPPORT_IPSEC

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "cms_core.h"
#include "cms_util.h"
#include "cms_eid.h"
#include "cms_dal.h"
#include "rcl.h"
#include "prctl.h"
#include "rut_util.h"
#include "rut_ipsec.h"
#include "rut_wan.h"
#include "qdm_intf.h"


CmsRet rutIPSec_config_dev2(void)
{
#ifdef DMP_DEVICE2_IPSEC_1
   FILE *fp_conf, *fp_secret, *fp_setkey;
   int local_prefixlen;
   int remote_prefixlen;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2IpsecFilterObject *filterObj = NULL;
   const char *old_algo_hmac_prefix="hmac";
   char *adj_int_name;
   char *adj_enc_name;
   char *proto;
   struct stat st;

   /* create /var/ipsec folder for holding StrongSwan related config files */
   if (stat("/var/ipsec", &st) == -1) {
      if (mkdir("/var/ipsec", 0700)) {
         fprintf(stderr, "/var/ipsec: unable to create folder\n");
         return CMSRET_INTERNAL_ERROR;
      }
   }

   /* write out setkey file */
   fp_setkey = fopen("/var/ipsec/setkey.sh", "w");
   if (fp_setkey == NULL) {
      fprintf(stderr, "/var/ipsec/setkey.sh: unable to open file\n");
      return CMSRET_INTERNAL_ERROR;
   }

   /* write out ipsec.conf file */
   fp_conf = fopen("/var/ipsec/ipsec.conf", "w");
   if (fp_conf == NULL) {
      fprintf(stderr, "/var/ipsec/ipsec.conf: unable to open file\n");
      fclose(fp_setkey);
      return CMSRET_INTERNAL_ERROR;
   }

   /* write out ipsec.secrets file */
   fp_secret = fopen("/var/ipsec/ipsec.secrets", "w");
   if (fp_secret == NULL) {
      fprintf(stderr, "/var/ipsec/ipsec.secrets: unable to open file\n");
      fclose(fp_conf);
      fclose(fp_setkey);
      return CMSRET_INTERNAL_ERROR;
   }

   fprintf(fp_conf, "conn %%default\n");
   fprintf(fp_conf, "\tikelifetime=60m\n");
   fprintf(fp_conf, "\tkeylife=60m\n");
   fprintf(fp_conf, "\trekeymargin=3m\n");
   fprintf(fp_conf, "\tkeyingtries=1\n");
   fprintf(fp_conf, "\tkeyexchange=ikev1\n");
   fprintf(fp_conf, "\tauto=start\n");
   fprintf(fp_conf, "\tcloseaction=hold\n");
   fprintf(fp_conf, "\tdpdaction=hold\n");
   fprintf(fp_conf, "\n");

   INIT_INSTANCE_ID_STACK(&iidStack);

   while (cmsObj_getNextFlags(MDMOID_DEV2_IPSEC_FILTER, &iidStack, 
                    OGF_NO_VALUE_UPDATE, (void **)&filterObj) == CMSRET_SUCCESS)
   {
      Dev2IpsecProfileObject *profileObj = NULL;
      MdmPathDescriptor pathDesc;
      char wanIP[CMS_IPADDR_LENGTH];
      int firewall;
      int isIpV4;

      if (filterObj->enable != TRUE)
      {
         cmsObj_free((void **)&filterObj);
         continue;
      }

      INIT_PATH_DESCRIPTOR(&pathDesc);
      if (cmsMdm_fullPathToPathDescriptor(filterObj->profile, &pathDesc) != 
          CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_fullPathToPathDescriptor failed for %s", 
                      filterObj->profile);
         cmsObj_free((void **)&filterObj);
         continue;
      }
      if (cmsObj_get(pathDesc.oid, &pathDesc.iidStack, OGF_NO_VALUE_UPDATE,
                           (void *)&profileObj) != CMSRET_SUCCESS)
      {
         cmsLog_error("cannot get associated profileObj");
         cmsObj_free((void **)&filterObj);
         continue;
      }

      isIpV4 = (strchr(profileObj->remoteEndpoints, ':') == NULL);
      if (rutIPSec_getWanIP(profileObj->X_BROADCOM_COM_LocalIfName, wanIP,
                            &firewall, isIpV4) != CMSRET_SUCCESS)
      {
         cmsObj_free((void **)&filterObj);
         cmsObj_free((void **)&profileObj);
         continue;
      }

      if ( isIpV4 == TRUE )
      {
         local_prefixlen = rutIPSec_calPrefixLen(filterObj->sourceMask);
         remote_prefixlen = rutIPSec_calPrefixLen(filterObj->destMask);
      }
      else
      {
         local_prefixlen = atoi(filterObj->sourceMask);
         remote_prefixlen = atoi(filterObj->destMask);
      }

      if(!cmsUtl_strcmp(profileObj->X_BROADCOM_COM_KeyExchangeMode, "auto"))
      {
         Ikev1CfgObject *ikeObj = NULL;

         if (cmsObj_get(MDMOID_IKEV1_CFG, &pathDesc.iidStack,
                     OGF_NO_VALUE_UPDATE, (void *)&ikeObj) != CMSRET_SUCCESS)
         {
            cmsLog_error("cannot get ikeObj");
            cmsObj_free((void **)&filterObj);
            cmsObj_free((void **)&profileObj);
            continue;
         }

         fprintf(fp_conf, "conn \"%s\"\n", filterObj->X_BROADCOM_COM_TunnelName);
         fprintf(fp_conf, "\tleft=%s\n", wanIP);
         fprintf(fp_conf, "\tleftsubnet=%s/%d\n",
                 filterObj->sourceIP, local_prefixlen);
         fprintf(fp_conf, "\tright=%s\n", profileObj->remoteEndpoints);
         fprintf(fp_conf, "\trightsubnet=%s/%d\n",
                 filterObj->destIP, remote_prefixlen);

         if (!cmsUtl_strcmp(ikeObj->authenticationMethod, "pre_shared_key")) 
         {
            fprintf(fp_conf, "\tauthby=secret\n");
            fprintf(fp_secret, "%s : PSK \"%s\"\n",
                    profileObj->remoteEndpoints, ikeObj->preSharedKey);
         } 
         else if (!cmsUtl_strcmp(ikeObj->authenticationMethod, "certificate"))
         {
            fprintf(fp_conf, "\tauthby=rsasig\n");
            fprintf(fp_conf, "\tleftcert=/var/cert/%s.cert\n",
                    ikeObj->certificateName);
            fprintf(fp_conf, "\tleftsendcert=always\n");
            fprintf(fp_secret, ": RSA /var/cert/%s.priv\n",
                    ikeObj->certificateName);
         } /* unknown type */

         /* Phase 1 Settings */
         if (cmsUtl_strcmp(ikeObj->phase1Mode, "main"))
         {
            fprintf(fp_conf, "\taggressive=yes\n");
         }
         fprintf(fp_conf, "\tikelifetime=%ds\n", ikeObj->phase1KeyTime);
         fprintf(fp_conf, "\tike=%s-%s-%s\n", ikeObj->phase1AllowedEncryptionAlgorithms,
                 ikeObj->phase1AllowedIntegrityAlgorithms,
                 ikeObj->phase1AllowedDiffieHellmanGroupTransforms);

         /* Phase 2 Settings */
         fprintf(fp_conf, "\tlifetime=%ds\n", ikeObj->phase2KeyTime);
         /* Adjust algo name for backward compatibility */
         adj_int_name = ikeObj->phase2AllowedIntegrityAlgorithms;
         if (strncmp(adj_int_name, old_algo_hmac_prefix, 
                     strlen(old_algo_hmac_prefix)) == 0)
         {
            adj_int_name += strlen(old_algo_hmac_prefix)+1;
         }
         adj_enc_name = ikeObj->phase2AllowedEncryptionAlgorithms;
         if (0 == strcmp(adj_enc_name, "null_enc")) {
            adj_enc_name = "null";
         }
         if (!cmsUtl_strcmp(profileObj->protocol, "ESP"))
         {
            fprintf(fp_conf, "\tesp=%s-%s", adj_enc_name, adj_int_name);
         }
         else
         {
            fprintf(fp_conf, "\tah=%s", adj_int_name);
         }

         if (ikeObj->perfectFSEn) 
         {
            fprintf(fp_conf, "-%s\n", ikeObj->phase2AllowedDiffieHellmanGroupTransforms);
         }
         else
         {
            fprintf(fp_conf, "\n");
         }

         cmsObj_free((void **)&ikeObj);
      }
      else if (!cmsUtl_strcmp(profileObj->X_BROADCOM_COM_KeyExchangeMode,
                              "manual"))
      {
         ManualModeCfgObject *manualObj = NULL;

         if (cmsObj_get(MDMOID_MANUAL_MODE_CFG, &pathDesc.iidStack,
                     OGF_NO_VALUE_UPDATE, (void *)&manualObj) != CMSRET_SUCCESS)
         {
            cmsLog_error("cannot get manualObj");
            cmsObj_free((void **)&filterObj);
            cmsObj_free((void **)&profileObj);
            continue;
         }

         /* Adjust algo name for backward compatibility */
         adj_int_name = manualObj->allowedIntegrityAlgorithms;
         if (strncmp(adj_int_name, old_algo_hmac_prefix, 
                     strlen(old_algo_hmac_prefix)) == 0) {
            adj_int_name += strlen(old_algo_hmac_prefix)+1;
         }
         adj_enc_name = manualObj->allowedEncryptionAlgorithms;
         if (0 == strcmp(adj_enc_name, "des-cbc")) {
            adj_enc_name = "des";
         } else if (0 == strcmp(adj_enc_name, "3des-cbc")) {
            adj_enc_name = "des3_ede";
         } else if (0 == strcmp(adj_enc_name, "aes-cbc")) {
            adj_enc_name = "aes";
         }

         if (!cmsUtl_strcmp(profileObj->protocol, "ESP")) {
            proto = "esp";
            fprintf(fp_setkey, "ip xfrm state add src %s dst %s proto %s spi 0x%d reqid 0x%d mode tunnel enc %s 0x%s auth %s 0x%s\n",
                    wanIP, profileObj->remoteEndpoints, proto,
                    manualObj->SPI, manualObj->SPI,
                    adj_enc_name, manualObj->encryptionKey,
                    adj_int_name, manualObj->authenticationKey);
            fprintf(fp_setkey, "ip xfrm state add src %s dst %s proto %s spi 0x%d reqid 0x%d mode tunnel enc %s 0x%s auth %s 0x%s\n",
                    profileObj->remoteEndpoints, wanIP, proto,
                    manualObj->SPI, manualObj->SPI,
                    adj_enc_name, manualObj->encryptionKey,
                    adj_int_name, manualObj->authenticationKey);
         } else {
            proto = "ah";
            fprintf(fp_setkey, "ip xfrm state add src %s dst %s proto %s spi 0x%d reqid 0x%d mode tunnel auth %s 0x%s\n",
                    wanIP, profileObj->remoteEndpoints, proto,
                    manualObj->SPI, manualObj->SPI,
                    adj_int_name, manualObj->authenticationKey);
            fprintf(fp_setkey, "ip xfrm state add src %s dst %s proto %s spi 0x%d reqid 0x%d mode tunnel auth %s 0x%s\n",
                    profileObj->remoteEndpoints, wanIP, proto,
                    manualObj->SPI, manualObj->SPI,
                    adj_int_name, manualObj->authenticationKey);
         }
         fprintf(fp_setkey, "ip xfrm policy add src %s/%d dst %s/%d dir out tmpl src %s dst %s proto %s reqid 0x%d mode tunnel\n",
                 filterObj->sourceIP, local_prefixlen,
                 filterObj->destIP, remote_prefixlen,
                 wanIP, profileObj->remoteEndpoints,
                 proto, manualObj->SPI);
         fprintf(fp_setkey, "ip xfrm policy add src %s/%d dst %s/%d dir in tmpl src %s dst %s proto %s reqid 0x%d mode tunnel\n",
                 filterObj->destIP, remote_prefixlen,
                 filterObj->sourceIP, local_prefixlen, 
                 profileObj->remoteEndpoints, wanIP,
                 proto, manualObj->SPI);
         fprintf(fp_setkey, "ip xfrm policy add src %s/%d dst %s/%d dir fwd tmpl src %s dst %s proto %s reqid 0x%d mode tunnel\n\n",
                 filterObj->destIP, remote_prefixlen,
                 filterObj->sourceIP, local_prefixlen, 
                 profileObj->remoteEndpoints, wanIP,
                 proto, manualObj->SPI);

         cmsObj_free((void **)&manualObj);
      }

      if ( (isIpV4 == TRUE) && (1 == firewall) )
      {
          char cmd[BUFLEN_256];
          sprintf(cmd, "iptables -w -t nat -I POSTROUTING 1 -s %s/%d -o %s -d %s/%d -j ACCEPT",
                       filterObj->sourceIP, local_prefixlen,
                       profileObj->X_BROADCOM_COM_LocalIfName, 
                       filterObj->destIP, remote_prefixlen);
          fprintf(fp_conf, "\tleftfirewall=yes\n");
          rut_doSystemAction("rut", cmd);
      }
      fprintf(fp_conf, "\n");

      cmsObj_free((void **)&profileObj);
      cmsObj_free((void **)&filterObj);
   }

   fclose(fp_conf);
   fclose(fp_secret);
   fclose(fp_setkey);

   prctl_runCommandInShellBlocking("chmod 600 /var/ipsec/ipsec.secrets");
   prctl_runCommandInShellBlocking("chmod a+x /var/ipsec/setkey.sh");
#endif
   return CMSRET_SUCCESS;
}

CmsRet rutIPSec_activateTunnel_dev2(void)
{
   CmsRet ret = CMSRET_SUCCESS;

#ifdef DMP_DEVICE2_IPSEC_1
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2IpsecFilterObject *filterObj = NULL;

   while (cmsObj_getNextFlags(MDMOID_DEV2_IPSEC_FILTER, &iidStack, 
                 OGF_NO_VALUE_UPDATE, (void **)&filterObj) == CMSRET_SUCCESS)
   {
      if (filterObj->enable)
      {
         /* rcl handler will activate all tunnels. So break after found one */
         if ( (ret = cmsObj_set(filterObj, &iidStack)) != CMSRET_SUCCESS )
         {
            cmsLog_error("Failed to set filterObj");
         }

         cmsObj_free((void **) &filterObj);
         break;
      }

      cmsObj_free((void **)&filterObj);
   }

#endif
   return ret;
}

#endif /* SUPPORT_IPSEC */
