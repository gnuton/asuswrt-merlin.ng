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
#ifdef DMP_STORAGESERVICE_1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "device2/rut2_util.h"
#include "rut_storageservice.h"


#ifdef DMP_DEVICE2_BASELINE_1
static void modifyStorageServiceNumEntry(const InstanceIdStack * iidStack, SINT32 delta)
{
   rutUtil_modifyNumGeneric_dev2(MDMOID_DEV2_SERVICES,
                        MDMOID_STORAGE_SERVICE,
                        iidStack,
                        delta);
}
#endif

CmsRet rcl_storageServiceObject( _StorageServiceObject *newObj __attribute__((unused)),
                                 const _StorageServiceObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{

#ifdef DMP_DEVICE2_BASELINE_1
   if (ADD_NEW(newObj, currObj))
   {
      /* In the Pure181 data model, there is a NumberOfEntries param
       * (in TR98 and Hybrid, there is no such param, so no need to update)
       */
      if (cmsMdm_isDataModelDevice2())
      {
         modifyStorageServiceNumEntry(iidStack, 1);
      }
   }

   if (DELETE_EXISTING(newObj, currObj))
   {
      if (cmsMdm_isDataModelDevice2())
      {
         modifyStorageServiceNumEntry(iidStack, -1);
      }
   }
#endif /* DMP_DEVICE2_BASELINE_1 */

   return CMSRET_SUCCESS;
}



CmsRet rcl_capabilitesObject( _CapabilitesObject *newObj __attribute__((unused)),
                                 const _CapabilitesObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{

      return CMSRET_SUCCESS;

}

CmsRet rcl_networkServerObject( _NetworkServerObject *newObj,
                                 const _NetworkServerObject *currObj,
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
   /*start Samba here */

   if(ADD_NEW(newObj,currObj))
   {
#ifdef SUPPORT_SAMBA
     // SINT32 pid;

      newObj->SMBEnable = TRUE;
#if 0   //move to start smbd after config br0's ip
      if ((pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_SAMBA, NULL,0)) == CMS_INVALID_PID)
      {
         cmsLog_error("failed to start SAMBA server.");
         return CMSRET_INTERNAL_ERROR;
      }
#endif

#endif
   }
   return CMSRET_SUCCESS;

}

CmsRet rcl_networkInfoObject( _NetworkInfoObject *newObj __attribute__((unused)),
                                 const _NetworkInfoObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{

      return CMSRET_SUCCESS;

}



CmsRet rcl_physicalMediumObject( _PhysicalMediumObject *newObj __attribute__((unused)),
                                 const _PhysicalMediumObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{

      return CMSRET_SUCCESS;

}




CmsRet rcl_logicalVolumeObject( _LogicalVolumeObject *newObj __attribute__((unused)),
                                 const _LogicalVolumeObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
#ifdef SUPPORT_SAMBA
   char tmpBuf[BUFLEN_128];
   char cmd[BUFLEN_256];

   if(ENABLE_NEW_OR_ENABLE_EXISTING(newObj,currObj))
   {

      /*create a softlink to share directory,so that it will be exported by samba */
      sprintf(tmpBuf,"/mnt/%s/share",newObj->name);

      sprintf(cmd,"ln -s %s /var/samba/share/%s_share",tmpBuf,newObj->name);

      rut_doSystemAction("storage:samba", cmd);

   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      sprintf(cmd,"rm /var/samba/share/%s_share",currObj->name);
      rut_doSystemAction("storage:samba", cmd);
   }
#endif

   return CMSRET_SUCCESS;
}

CmsRet rcl_folderObject( _FolderObject *newObj __attribute__((unused)),
                                 const _FolderObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{

      return CMSRET_SUCCESS;


}


CmsRet rcl_userAccountObject( _UserAccountObject *newObj __attribute__((unused)),
                                 const _UserAccountObject *currObj __attribute__((unused)),
                                 const InstanceIdStack *iidStack __attribute__((unused)),
                                 char **errorParam __attribute__((unused)),
                                 CmsRet *errorCode __attribute__((unused)))
{
#ifdef SUPPORT_SAMBA

   char homeDir[BUFLEN_128];
   CmsRet ret;

   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {

      sprintf(homeDir,"/mnt/%s/%s",newObj->X_BROADCOM_volumeName,newObj->username);

      if((ret=rutStorage_addUserAccount(newObj->username, newObj->password, homeDir))!= CMSRET_SUCCESS)
      {
         return ret;
      }

   }
   else if (DELETE_EXISTING(newObj, currObj))
   {
      sprintf(homeDir,"/mnt/%s/%s",currObj->X_BROADCOM_volumeName,currObj->username);

      if((ret=rutStorage_deleteUserAccount(currObj->username, homeDir))!= CMSRET_SUCCESS)
      {
         return ret;
      }
   }
#endif /*SUPPORT_SAMBA */

   return CMSRET_SUCCESS;
}


#endif  /* DMP_STORAGESERVICE_1 */
