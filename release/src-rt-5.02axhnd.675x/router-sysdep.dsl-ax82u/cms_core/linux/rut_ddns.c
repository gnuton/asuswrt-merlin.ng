/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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

#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "cms_msg.h"
#include "rut_ddns.h"

#define DDNS_CONFIG_FILE  "/var/ddnsd.cfg"


void rutDDns_restart(void)
{
   _DDnsCfgObject *ddnsObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   FILE *fp;
   SINT32 pid;
   char path1[CMS_MAX_FULLPATH_LENGTH] = {0};
   char path2[CMS_MAX_FULLPATH_LENGTH*2] = {0};
   CmsRet ret;

   /*
    * build path to config file in a way that is friendly to desktop linux.
    */
   if ((ret = cmsUtl_getRunTimePath(DDNS_CONFIG_FILE, path2, sizeof(path2))) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create path to %s", DDNS_CONFIG_FILE);
      return;
   }

   if ((ret = cmsUtl_getRunTimePath("/var/ddnsd.cache", path1, sizeof(path1))) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create path to ddnsd.cache");
      return;
   }


   /*
    * w+ means create file if it does not exist.  Truncate to 0 if it does exist.
    */
   if ((fp = fopen(path2, "w+")) == NULL )
   {
      cmsLog_error("could not open %s", path2);
      return;
   }

   while ((ret = cmsObj_getNext(MDMOID_D_DNS_CFG, &iidStack, (void **) &ddnsObj)) == CMSRET_SUCCESS)
   {
      if (ddnsObj->enable)
      {
         cmsLog_debug("setting info: %s/%s/%s/%s/%s", ddnsObj->fullyQualifiedDomainName, ddnsObj->userName, ddnsObj->password, ddnsObj->ifName, ddnsObj->providerName);
   
         fprintf( fp, "[%s]\n", ddnsObj->fullyQualifiedDomainName );
         fprintf( fp, "username=%s\n", ddnsObj->userName );
         fprintf( fp, "password=%s\n", ddnsObj->password );
         fprintf( fp, "interface=%s\n", ddnsObj->ifName );
         fprintf( fp, "service=%s\n\n", ddnsObj->providerName);	 
      }

      cmsObj_free((void **) &ddnsObj);
   }

   fclose( fp );
   
   /*
    * Start ddnsd with the /var/ddnsd.cfg and /var/ddnsd.cache args on the
    * command line.
    */
   if ((ret = cmsUtl_strncat(path2, CMS_MAX_FULLPATH_LENGTH*2, " ")) != CMSRET_SUCCESS)
   {
      cmsLog_error("path2 buf too small, ret=%d", ret);
      return;
   }

   if ((ret = cmsUtl_strncat(path2, CMS_MAX_FULLPATH_LENGTH*2, path1)) != CMSRET_SUCCESS)
   {
      cmsLog_error("path2 buf too small, for %s + %s (ret=%d)",
                   path2, path1, ret);
      return;
   }

   if ((pid = rut_sendMsgToSmd(CMS_MSG_RESTART_APP, EID_DDNSD, path2, strlen(path2)+1)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to start or restart ddnsd");
   }
   
   return;
}


void rutDDns_stop(void)
{
   char path[CMS_MAX_FULLPATH_LENGTH]={0};
   CmsRet ret;

   /*
    * build path to config file in a way that is friendly to desktop linux.
    */
   if ((ret = cmsUtl_getRunTimePath(DDNS_CONFIG_FILE, path, sizeof(path))) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create path to %s", DDNS_CONFIG_FILE);
      return;
   }

   
   unlink(path);

   rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_DDNSD, NULL, 0);
   
   return;
}


UBOOL8 rutDDns_isAllRequiredValuesPresent(const _DDnsCfgObject *ddnsObj)
{
   if (ddnsObj->fullyQualifiedDomainName == NULL ||
       ddnsObj->userName == NULL ||
       ddnsObj->providerName == NULL)
   {
      return FALSE;
   }
   else
   {
      return TRUE;
   }
}


UBOOL8 rutDDns_isDuplicateFQDN(const char *fqdn, const InstanceIdStack *skipIidStack)
{
   _DDnsCfgObject *ddnsObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 isDuplicate=FALSE;
   CmsRet ret;


   while ((!isDuplicate) &&
          ((ret = cmsObj_getNext(MDMOID_D_DNS_CFG, &iidStack, (void **) &ddnsObj)) == CMSRET_SUCCESS))
   {
      if (skipIidStack == NULL || cmsMdm_compareIidStacks(&iidStack, skipIidStack))
      {
         /*
          * check for duplicate fully qualified domain name only when
          * the iidStack we got back from getNext is different from
          * the skipIidStack.
          */

         /* isDuplicate is TRUE if the strcmp == 0 */
          isDuplicate = (0 == cmsUtl_strcmp(fqdn, ddnsObj->fullyQualifiedDomainName));
      }

      cmsObj_free((void **) &ddnsObj);
   }

   return isDuplicate;
}


UBOOL8 rutDDns_isValuesChanged(const _DDnsCfgObject *newObj, const _DDnsCfgObject *currObj)
{

   if (cmsUtl_strcmp(newObj->fullyQualifiedDomainName, currObj->fullyQualifiedDomainName) ||
       cmsUtl_strcmp(newObj->userName, currObj->userName) ||
       cmsUtl_strcmp(newObj->password, currObj->password) ||
       cmsUtl_strcmp(newObj->ifName, currObj->ifName) ||
       cmsUtl_strcmp(newObj->providerName, currObj->providerName))
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


UINT32 rutDDns_getNumberOfEnabledEntries(void)
{
   _DDnsCfgObject *ddnsObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UINT32 count=0;
   CmsRet ret;


   while ((ret = cmsObj_getNext(MDMOID_D_DNS_CFG, &iidStack, (void **) &ddnsObj)) == CMSRET_SUCCESS)
   {
      if (ddnsObj->enable)
      {
         count++;
      }
      cmsObj_free((void **) &ddnsObj);
   }

   cmsLog_debug("count=%d", count);
   return count;
}

