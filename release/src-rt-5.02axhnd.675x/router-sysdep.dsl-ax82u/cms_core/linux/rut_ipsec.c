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

int rutIPSec_numTunnelEntries(void);
int rutIPSec_numTunnelEntries_igd(void);
int rutIPSec_numTunnelEntries_dev2(void);
#if defined(SUPPORT_DM_LEGACY98)
#define rutIPSec_numTunnelEntries()  rutIPSec_numTunnelEntries_igd()
#elif defined(SUPPORT_DM_HYBRID)
#define rutIPSec_numTunnelEntries()  rutIPSec_numTunnelEntries_igd()
#elif defined(SUPPORT_DM_PURE181)
#define rutIPSec_numTunnelEntries()  rutIPSec_numTunnelEntries_dev2()
#elif defined(SUPPORT_DM_DETECT)
#define rutIPSec_numTunnelEntries()   (cmsMdm_isDataModelDevice2() ?   \
                                  rutIPSec_numTunnelEntries_dev2()   : \
                                  rutIPSec_numTunnelEntries_igd())
#endif

int rutIPSec_numTunnelEntries_igd()
{
   int numTunCfg = 0;
#ifdef DMP_X_BROADCOM_COM_IPSEC_1
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   IPSecCfgObject *ipsecObj = NULL;

   while ( (ret = cmsObj_getNext
         (MDMOID_IP_SEC_CFG, &iidStack, (void **) &ipsecObj)) == CMSRET_SUCCESS)
   {
      numTunCfg++;
      // Free the mem allocated this object by the get API.
      cmsObj_free((void **) &ipsecObj);
   }
#endif
   return numTunCfg;
}

int rutIPSec_numTunnelEntries_dev2()
{
   int numTunCfg = 0;
#ifdef DMP_DEVICE2_IPSEC_1
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2IpsecFilterObject *filterObj = NULL;

   while (cmsObj_getNextFlags(MDMOID_DEV2_IPSEC_FILTER, &iidStack, 
                    OGF_NO_VALUE_UPDATE, (void **)&filterObj) == CMSRET_SUCCESS)
   {
      if (filterObj->enable)
      {
         numTunCfg++;
      }
      cmsObj_free((void **)&filterObj);
   }
#endif

   return numTunCfg;
}

CmsRet rutIPSec_activateTunnel_igd(void)
{
   rutIPSec_config();
   rutIPSec_restart();

   return CMSRET_SUCCESS;
}

CmsRet rutIPSec_getWanIP(const char *wanIntf, char *ipaddr, int *firewall, UBOOL8 isIPv4)
{
   char ifNameBuf[CMS_IFNAME_LENGTH]={0};
   const char *searchIfName;
   char *ptr;

   if ( (wanIntf == NULL) || (0 == strlen(wanIntf)))
   {
      if ( FALSE == rutWan_findFirstIpvxRoutedAndConnected(isIPv4 ? CMS_AF_SELECT_IPV4 : CMS_AF_SELECT_IPV6, ifNameBuf) )
      {
         return CMSRET_OBJECT_NOT_FOUND;
      }
      searchIfName = ifNameBuf;
   }
   else
   {
      searchIfName = wanIntf;
      if ( FALSE == qdmIpIntf_isWanInterfaceUpLocked(searchIfName, isIPv4) )
      {
         /* the provided interface is not up */
         return CMSRET_OBJECT_NOT_FOUND;
      }
   
      if ( qdmIpIntf_isWanInterfaceBridgedLocked(searchIfName) )
      {
         /* the provided interface is not routed */
         return CMSRET_INVALID_ARGUMENTS;
      }
   }

   if ( isIPv4 )
   {
      if ( CMSRET_SUCCESS != qdmIpIntf_getIpv4AddressByNameLocked(searchIfName, ipaddr) )
      {
         return CMSRET_OBJECT_NOT_FOUND;
      }
   }
   else
   {
      if ( CMSRET_SUCCESS != qdmIpIntf_getIpv6AddressByNameLocked(searchIfName, ipaddr) )
      {
         return CMSRET_OBJECT_NOT_FOUND;
      }
   }

   ptr = cmsUtl_strstr(ipaddr, "/");
   if ( ptr )
   {
      *ptr = '\0';
   }

   *firewall = qdmIpIntf_isFirewallEnabledOnIntfnameLocked(searchIfName) || 
               qdmIpIntf_isNatEnabledOnIntfNameLocked(searchIfName);

   return CMSRET_SUCCESS;

}

int rutIPSec_calPrefixLen(char *ipAddr)
{
   UINT32 addr = ntohl(inet_addr(ipAddr));
   int i;

   if (addr == INADDR_NONE) {
      return 32;
   } else {
      for (i=0; i<=32;i++) {
         if ((addr>>i) & 1) {
            break;
         }
     }
     return 32 - i;
   }
}

CmsRet rutIPSec_config_igd(void)
{
#ifdef DMP_X_BROADCOM_COM_IPSEC_1
   CmsRet ret = CMSRET_SUCCESS;
   FILE *fp_conf, *fp_secret, *fp_setkey;
   int local_prefixlen;
   int remote_prefixlen;
   char wanIP[CMS_IPADDR_LENGTH];
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   IPSecCfgObject *ipsecObj = NULL;
   int firewall;
   int isIpV4;
   const char *old_algo_hmac_prefix="hmac";
   char *adj_int_name;
   char *adj_enc_name;
   struct stat st ;

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
   while (1)
   {

      ret = cmsObj_getNext(MDMOID_IP_SEC_CFG, &iidStack, (void **)&ipsecObj);
      if ( ret != CMSRET_SUCCESS ) {
         break;
      }

      wanIP[0] = '\0';
      if (0 == cmsUtl_strcmp(ipsecObj->ipVer, "6")) {
         isIpV4 = 0;
      } else {
         isIpV4 = 1;
      }
      ret = rutIPSec_getWanIP(ipsecObj->localGwIf, wanIP, &firewall, isIpV4);
      if (ret != CMSRET_SUCCESS) {
         cmsObj_free((void **)&ipsecObj);
         continue;
      }

      if ( 1 == isIpV4 ) {
         if (strcmp(ipsecObj->localIPMode, "subnet") == 0) {
            local_prefixlen = rutIPSec_calPrefixLen(ipsecObj->localMask);
         } else {
            local_prefixlen = 32;
         }
         if (strcmp(ipsecObj->remoteIPMode, "subnet") == 0) {
            remote_prefixlen = rutIPSec_calPrefixLen(ipsecObj->remoteMask);
         } else {
            remote_prefixlen = 32;
         }
      } else {
         if (strcmp(ipsecObj->localIPMode, "subnet") == 0) {
            local_prefixlen = atoi(ipsecObj->localMask);
         } else {
            local_prefixlen = 128;
         }
         if (strcmp(ipsecObj->remoteIPMode, "subnet") == 0) {
            remote_prefixlen = atoi(ipsecObj->remoteMask);
         } else {
            remote_prefixlen = 128;
         }
      }

      if (strcmp(ipsecObj->keyExM, "manual") == 0) {

         /* Adjust algo name for backward compatibility */
         adj_int_name = ipsecObj->manualAuthAlgo;
         if (strncmp(adj_int_name, old_algo_hmac_prefix, 
                     strlen(old_algo_hmac_prefix)) == 0) {
            adj_int_name += strlen(old_algo_hmac_prefix)+1;
         }
         adj_enc_name = ipsecObj->manualEncryptionAlgo;
         if (0 == strcmp(adj_enc_name, "des-cbc")) {
            adj_enc_name = "des";
         } else if (0 == strcmp(adj_enc_name, "3des-cbc")) {
            adj_enc_name = "des3_ede";
         } else if (0 == strcmp(adj_enc_name, "aes-cbc")) {
            adj_enc_name = "aes";
         }

         if(strcmp(ipsecObj->tunMode, "esp") == 0) {
            fprintf(fp_setkey, "ip xfrm state add src %s dst %s proto %s spi 0x%s reqid 0x%s mode tunnel enc %s 0x%s auth %s 0x%s\n",
                    wanIP, ipsecObj->remoteGWAddress, ipsecObj->tunMode,
                    ipsecObj->SPI, ipsecObj->SPI,
                    adj_enc_name, ipsecObj->manualEncryptionKey,
                    adj_int_name, ipsecObj->manualAthKey);
            fprintf(fp_setkey, "ip xfrm state add src %s dst %s proto %s spi 0x%s reqid 0x%s mode tunnel enc %s 0x%s auth %s 0x%s\n",
                    ipsecObj->remoteGWAddress, wanIP, ipsecObj->tunMode,
                    ipsecObj->SPI, ipsecObj->SPI,
                    adj_enc_name, ipsecObj->manualEncryptionKey,
                    adj_int_name, ipsecObj->manualAthKey);
         } else {
            fprintf(fp_setkey, "ip xfrm state add src %s dst %s proto %s spi 0x%s reqid 0x%s mode tunnel auth %s 0x%s\n",
                    wanIP, ipsecObj->remoteGWAddress, ipsecObj->tunMode,
                    ipsecObj->SPI, ipsecObj->SPI,
                    adj_int_name, ipsecObj->manualAthKey);
            fprintf(fp_setkey, "ip xfrm state add src %s dst %s proto %s spi 0x%s reqid 0x%s mode tunnel auth %s 0x%s\n",
                    ipsecObj->remoteGWAddress, wanIP, ipsecObj->tunMode,
                    ipsecObj->SPI, ipsecObj->SPI,
                    adj_int_name, ipsecObj->manualAthKey);
         }
         fprintf(fp_setkey, "ip xfrm policy add src %s/%d dst %s/%d dir out tmpl src %s dst %s proto %s reqid 0x%s mode tunnel\n",
                 ipsecObj->localIPAddress, local_prefixlen,
                 ipsecObj->remoteIPAddress, remote_prefixlen,
                 wanIP, ipsecObj->remoteGWAddress,
                 ipsecObj->tunMode, ipsecObj->SPI);
         fprintf(fp_setkey, "ip xfrm policy add src %s/%d dst %s/%d dir in tmpl src %s dst %s proto %s reqid 0x%s mode tunnel\n",
                 ipsecObj->remoteIPAddress, remote_prefixlen,
                 ipsecObj->localIPAddress, local_prefixlen,
                 ipsecObj->remoteGWAddress, wanIP,
                 ipsecObj->tunMode, ipsecObj->SPI);
         fprintf(fp_setkey, "ip xfrm policy add src %s/%d dst %s/%d dir fwd tmpl src %s dst %s proto %s reqid 0x%s mode tunnel\n\n",
                 ipsecObj->remoteIPAddress, remote_prefixlen,
                 ipsecObj->localIPAddress, local_prefixlen,
                 ipsecObj->remoteGWAddress, wanIP,
                 ipsecObj->tunMode, ipsecObj->SPI);

      } else {

         fprintf(fp_conf, "conn \"%s\"\n", ipsecObj->connName);
         fprintf(fp_conf, "\tleft=%s\n", wanIP);
         fprintf(fp_conf, "\tleftsubnet=%s/%d\n",
                 ipsecObj->localIPAddress, local_prefixlen);
         fprintf(fp_conf, "\tright=%s\n", ipsecObj->remoteGWAddress);
         fprintf(fp_conf, "\trightsubnet=%s/%d\n",
                 ipsecObj->remoteIPAddress, remote_prefixlen);

         if (strcmp(ipsecObj->authM, "pre_shared_key") == 0) {
            fprintf(fp_conf, "\tauthby=secret\n");
            fprintf(fp_secret, "%s : PSK \"%s\"\n",
                    ipsecObj->remoteGWAddress, ipsecObj->PSK);
         } else if (strcmp(ipsecObj->authM, "certificate") == 0) {
            fprintf(fp_conf, "\tauthby=rsasig\n");
            fprintf(fp_conf, "\tleftcert=/var/cert/%s.cert\n",
                    ipsecObj->certificateName);
            fprintf(fp_conf, "\tleftsendcert=always\n");
            fprintf(fp_secret, ": RSA /var/cert/%s.priv\n",
                    ipsecObj->certificateName);
         } /* unknown type */

         /* Phase 1 Settings */
         if (strcmp(ipsecObj->ph1Mode, "main") != 0) {
            fprintf(fp_conf, "\taggressive=yes\n");
         }
         fprintf(fp_conf, "\tikelifetime=%ds\n", ipsecObj->ph1KeyTime);
         fprintf(fp_conf, "\tike=%s-%s-%s\n", ipsecObj->ph1EncryptionAlo,
                 ipsecObj->ph1IntegrityAlgo, ipsecObj->ph1DHGroup);

         /* Phase 2 Settings */
         fprintf(fp_conf, "\tlifetime=%ds\n", ipsecObj->ph2KeyTime);
         /* Adjust algo name for backward compatibility */
         adj_int_name = ipsecObj->ph2IntegrityAlgo;
         if (strncmp(adj_int_name, old_algo_hmac_prefix, 
                     strlen(old_algo_hmac_prefix)) == 0) {
            adj_int_name += strlen(old_algo_hmac_prefix)+1;
         }
         adj_enc_name = ipsecObj->ph2EncryptionAlo;
         if (0 == strcmp(adj_enc_name, "null_enc")) {
            adj_enc_name = "null";
         }
         if(strcmp(ipsecObj->tunMode, "esp") == 0) {
            fprintf(fp_conf, "\tesp=%s-%s", adj_enc_name, adj_int_name);
         } else {
            fprintf(fp_conf, "\tah=%s", adj_int_name);
         }

         if (strcmp(ipsecObj->perfectFSEn, "enable") == 0) {
            fprintf(fp_conf, "-%s\n", ipsecObj->ph2DHGroup);
         } else {
            fprintf(fp_conf, "\n");
         }

      }

      if ((1 == isIpV4) && (1 == firewall)) {
         char cmd[BUFLEN_256];
         sprintf(cmd, "iptables -w -t nat -I POSTROUTING 1 -s %s/%d -o %s -d %s/%d -j ACCEPT",
                 ipsecObj->localIPAddress, local_prefixlen, ipsecObj->localGwIf, 
                 ipsecObj->remoteIPAddress, remote_prefixlen);
         fprintf(fp_conf, "\tleftfirewall=yes\n");
         rut_doSystemAction("rut", cmd);
      }
      fprintf(fp_conf, "\n");

      // Free the mem allocated this object by the get API.
      cmsObj_free((void **) &ipsecObj);
   }

   fclose(fp_conf);
   fclose(fp_secret);
   fclose(fp_setkey);

   prctl_runCommandInShellBlocking("chmod 600 /var/ipsec/ipsec.secrets");
   prctl_runCommandInShellBlocking("chmod a+x /var/ipsec/setkey.sh");
#endif

   return CMSRET_SUCCESS;
}

CmsRet rutIPSec_restart(void)
{
   CmsRet ret = CMSRET_SUCCESS;

   prctl_runCommandInShellWithTimeout("ipsec stop 2>/dev/null");
   sleep(1);

   if (rutIPSec_numTunnelEntries() == 0) {
      prctl_runCommandInShellBlocking("ip xfrm state flush");
      prctl_runCommandInShellBlocking("ip xfrm policy flush");
      return ret;
   }

   prctl_runCommandInShellBlocking("ip xfrm state flush");
   prctl_runCommandInShellBlocking("ip xfrm policy flush");
   prctl_runCommandInShellBlocking("/var/ipsec/setkey.sh");
   prctl_runCommandInShellWithTimeout("ipsec start 2>/dev/null");
   return ret;
}

#endif /* SUPPORT_IPSEC */
