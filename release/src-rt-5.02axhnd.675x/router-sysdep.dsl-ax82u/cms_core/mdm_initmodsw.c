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


#include "cms.h"
#include "cms_params_modsw.h"
#include "cms_util.h"
#include "cms_core.h"
#include "mdm.h"
#include "mdm_private.h"
#include "odl.h"
#include "oal.h"
#include "osgid.h"
#include "beep_common.h"

/*!\file mdm_initmodsw.c
 * \brief This file calls other Exe Env functions (osgi, linux) to initialize
 *        the MDM.
 *
 */


#ifdef DMP_DEVICE2_SM_BASELINE_1

#ifdef SUPPORT_BEEP_HOST_EE
CmsRet mdm_addDefaultModSwBeepEeObjects(UINT32 *eeAddedCount);
#endif

#ifdef DMP_DEVICE2_X_BROADCOM_COM_MODSW_OSGIEE_1
CmsRet mdm_addDefaultModSwOsgiEeObjects(UINT32 *eeAddedCount);
#endif

#ifdef DMP_DEVICE2_X_BROADCOM_COM_MODSW_LINUXEE_1
CmsRet mdm_addDefaultModSwLinuxEeObjects(UINT32 *eeAddedCount);
#endif

#ifdef DMP_DEVICE2_X_BROADCOM_COM_MODSW_DOCKEREE_1
CmsRet mdm_addDefaultModSwDockerEeObjects(UINT32 *eeAddedCount);
#endif

#if !defined(DMP_DEVICE2_X_BROADCOM_COM_MODSW_OSGIEE_1) || !defined(DMP_DEVICE2_X_BROADCOM_COM_MODSW_LINUXEE_1) || !defined(DMP_DEVICE2_X_BROADCOM_COM_MODSW_OPENWRTEE_1) || !defined(SUPPORT_BEEP)
static CmsRet mdm_delModSwEeObjects(const char *eeName);
#endif

static int mdm_deleteUser(const char *username);

static int mdm_deleteDMAccess(const char *euid);

static int mdm_cleanupEuRelated(char *executionUnitList, UINT32 *deletedEUinstance);

static CmsRet mdm_delModSwPreinstalledEeObjects(UINT32 *eeDeletedCount, 
                                                UINT32 *duDeletedCount,
                                                UINT32 *euDeletedCount);
static CmsRet mdm_delModSwPreinstalledDuObjects(UINT32 *duDeletedCount,
                                                UINT32 *euDeletedCount);

CmsRet mdm_addDefaultModSwObjects()
{
   CmsRet ret=CMSRET_SUCCESS;
   UINT32 eeDefinedCount=0;
   UINT32 eeAddedCount=0;
   UINT32 eeDeletedCount=0;
   UINT32 duDeletedCount=0;
   UINT32 euDeletedCount=0;
   UBOOL8 updateNeeded = FALSE;
   SwModulesObject *swObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;

   ret = mdm_getObject(MDMOID_SW_MODULES, &iidStack, (void **) &swObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get SW_MODULES object, ret=%d", ret);
   }
   else
   {
      /* Should execute purely based on control flags */
      {
         if (swObj->X_BROADCOM_COM_PreinstallNeeded == TRUE)
         {
            if (swObj->X_BROADCOM_COM_PreinstallOverwrite == TRUE)
            {
               /* 
                * go through all the EEs that are preinstalled and 
                * remove them from MDM 
                */
               mdm_delModSwPreinstalledEeObjects(&eeDeletedCount,
                                              &duDeletedCount, &euDeletedCount);
               if (eeDeletedCount)
               {
                  swObj->execEnvNumberOfEntries -= eeDeletedCount;
                  updateNeeded = TRUE;
               }
               if (duDeletedCount)
               {
                  swObj->deploymentUnitNumberOfEntries -= duDeletedCount;
                  updateNeeded = TRUE;
               }
               if (euDeletedCount)
               {
                  swObj->executionUnitNumberOfEntries -= euDeletedCount;
                  updateNeeded = TRUE;
               }

               /* 
                * go through all the DUs that are preinstalled and 
                * remove them from MDM 
                */
               duDeletedCount = euDeletedCount = 0;
               mdm_delModSwPreinstalledDuObjects(&duDeletedCount,
                                                 &euDeletedCount);
               if (duDeletedCount)
               {
                  swObj->deploymentUnitNumberOfEntries -= duDeletedCount;
                  updateNeeded = TRUE;
               }
               if (euDeletedCount)
               {
                  swObj->executionUnitNumberOfEntries -= euDeletedCount;
                  updateNeeded = TRUE;
               }
            }
         }
      }
   }

#ifdef SUPPORT_BEEP_HOST_EE
   eeDefinedCount++;
   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_addDefaultModSwBeepEeObjects(&eeAddedCount);
   }
#else
   mdm_delModSwEeObjects(BEEP_HOSTEE_NAME);
#endif

#ifdef DMP_DEVICE2_X_BROADCOM_COM_MODSW_OSGIEE_1
   eeDefinedCount++;
   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_addDefaultModSwOsgiEeObjects(&eeAddedCount);
   }
#else
   mdm_delModSwEeObjects(OSGI_NAME);
#endif

#ifdef DMP_DEVICE2_X_BROADCOM_COM_MODSW_LINUXEE_1
   eeDefinedCount++;
   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_addDefaultModSwLinuxEeObjects(&eeAddedCount);
   }
#else
   mdm_delModSwEeObjects(LINUXEE_NAME);
#endif

#ifdef DMP_DEVICE2_X_BROADCOM_COM_MODSW_DOCKEREE_1
   eeDefinedCount++;
   if (ret == CMSRET_SUCCESS)
   {
      ret = mdm_addDefaultModSwDockerEeObjects(&eeAddedCount);
   }
#else
   mdm_delModSwEeObjects(DOCKEREE_NAME);
#endif

   if (eeDefinedCount == 0)
   {
      cmsLog_notice("No Exec Envs were built into the system.");
   }

   if (eeAddedCount > 0)
   {
      swObj->execEnvNumberOfEntries += eeAddedCount;
      cmsLog_notice("setting exe env num entries=%d", swObj->execEnvNumberOfEntries);
      updateNeeded = TRUE;
   }

   if (updateNeeded)
   {
      ret = mdm_setObject((void **) &swObj, &iidStack, FALSE);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not set SW_MODULES object, ret=%d", ret);
      }
   }

   return ret;

}  /* End of mdm_addDefaultModSwObjects() */


#ifdef SUPPORT_BEEP_HOST_EE
CmsRet mdm_addDefaultModSwBeepEeObjects(UINT32 *eeAddedCount)
{
   ExecEnvObject *eeObj = NULL;
   MdmPathDescriptor pathDesc;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 add = TRUE;
   CmsRet ret;

   /* If there is no BEEP Host Exec Env object yet, add it. */
   while (add &&
          (ret = mdm_getNextObject(MDMOID_EXEC_ENV, &iidStack,
                                   (void **)&eeObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(eeObj->name, BEEP_HOSTEE_NAME))
      {
         /* already in there, so no need to add */
         add = FALSE;
      }
      cmsObj_free((void **) &eeObj);
   }

   if (add)
   {
      cmsLog_notice("Adding initial BEEP host Execution Environment");

      INIT_PATH_DESCRIPTOR(&pathDesc);
      pathDesc.oid = MDMOID_EXEC_ENV;
      INIT_INSTANCE_ID_STACK(&pathDesc.iidStack);

      if ((ret = mdm_addObjectInstance(&pathDesc, NULL, NULL))!=CMSRET_SUCCESS)
      {
         cmsLog_error("could not add BEEP host Exec Env, ret=%d", ret);
         return ret;
      }
      else
      {
         (*eeAddedCount)++;
      }

      if ((ret = mdm_getObject(MDMOID_EXEC_ENV, &pathDesc.iidStack,
                               (void **) &eeObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get host Exec Env Obj, ret=%d", ret);
         return ret;
      }

      /* set any non-default initial values for the BEEP default EE object */
      eeObj->enable = TRUE;
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->name, BEEP_HOSTEE_NAME,
                                  mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->alias, BEEP_HOSTEE_NAME,
                                  mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->type, BEEP_HOSTEE_TYPE,
                                  mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->vendor, BEEP_HOSTEE_VENDOR,
                                  mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->version, BEEP_HOSTEE_VERSION,
                                  mdmLibCtx.allocFlags);
      CMSMEM_REPLACE_STRING_FLAGS(eeObj->X_BROADCOM_COM_ContainerName, "",
                                  mdmLibCtx.allocFlags);
      eeObj->X_BROADCOM_COM_MngrEid = EID_BBCD;

      if ((ret = mdm_setObject((void **) &eeObj, &pathDesc.iidStack,
                               FALSE)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to set EXEC_ENV Object, ret = %d", ret);
      }

      cmsObj_free((void **) &eeObj);
   }

   return ret;
}
#endif


#if !defined(DMP_DEVICE2_X_BROADCOM_COM_MODSW_OSGIEE_1) || !defined(DMP_DEVICE2_X_BROADCOM_COM_MODSW_LINUXEE_1) || !defined(DMP_DEVICE2_X_BROADCOM_COM_MODSW_OPENWRTEE_1) || !defined(DMP_DEVICE2_X_BROADCOM_COM_MODSW_DOCKEREE_1) || !defined(SUPPORT_BEEP)
static CmsRet mdm_delModSwEeObjects(const char *eeName)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   ExecEnvObject *eeObj = NULL;
   char path[CMS_MAX_FULLPATH_LENGTH];
   char *duDir = NULL;

   cmsLog_debug("Enter. eeName=%s", eeName);

   /* Clear out any old DU files (pkgs) that may be on the filesystem. */
   if (!cmsUtl_strcmp(eeName, LINUXEE_NAME))
   {
      duDir = CMS_MODSW_LINUXEE_DU_DIR;
   }
   else if (!cmsUtl_strcmp(eeName, OPENWRTEE_NAME))
   {
      duDir = CMS_MODSW_OPENWRTEE_DU_DIR;
   }
   else if (!cmsUtl_strcmp(eeName, OSGI_NAME))
   {
      duDir = CMS_MODSW_OSGIEE_DU_DIR;
   }
   else if (!cmsUtl_strcmp(eeName, DOCKEREE_NAME))
   {
      duDir = CMS_MODSW_DOCKEREE_DU_DIR;
   }
   else
   {
      cmsLog_error("Invalid eeName %s", eeName);
      return CMSRET_INVALID_ARGUMENTS;
   }

   if (cmsUtl_getRunTimePath(duDir, path, sizeof(path)) == CMSRET_SUCCESS)
   {
      if (cmsFil_isFilePresent(path))
      {
         cmsFil_removeDir(path);
      }
   }
   else
   {
      cmsLog_error("Could not make path for %s. ret=%d", duDir, ret);
   }

   /* Delete the EE object instance if exist */
   while ((ret = mdm_getNextObject(MDMOID_EXEC_ENV, &iidStack, (void **)&eeObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(eeObj->name, eeName))
      {
         MdmPathDescriptor pathDesc;

         cmsLog_debug("Deleting %s Execution Environment", eeName);

         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid      = MDMOID_EXEC_ENV;
         pathDesc.iidStack = iidStack;

         if ((ret = mdm_deleteObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not delete %s Execution Environment, ret=%d", eeName, ret);
         }
         cmsObj_free((void **)&eeObj);
         break;
      }
      cmsObj_free((void **)&eeObj);
   }

   if (ret == CMSRET_NO_MORE_INSTANCES)
   {
      ret = CMSRET_SUCCESS;
   }

   return ret;

}  /* End of mdm_delModSwEeObjects() */
#endif


static int mdm_deleteUser(const char *username)
{
   FILE  *fp, *tmpfp;
   char  tmpfile[BUFLEN_64];
   char  name[BEEP_USERNAME_LEN_MAX+1];
   char  line[BUFLEN_128];

   cmsLog_debug("Enter: username=%s", username);

   if ((fp = fopen(BEEP_PASSWD_FILE, "r")) == NULL)
   {
      cmsLog_notice("Failed open %s for read.", BEEP_PASSWD_FILE);
      return 0;
   }

   /* open a temporary file */
   sprintf(tmpfile, "%s--", BEEP_PASSWD_FILE);

   if ((tmpfp = fopen(tmpfile, "w")) == NULL)
   {
      fclose(fp);
      cmsLog_error("Failed open %s for write", tmpfile);
      return -1;
   }

   while (fgets(line, sizeof(line), fp))
   {
      sscanf(line, "%[^:]:%*s", name);
      cmsLog_debug("name=%s", name);
      if (strcmp(name, username))
      {
         if (fputs(line, tmpfp) == EOF) goto err_out;
      }
   }

   fclose(fp);
   fclose(tmpfp);
   rename(tmpfile, BEEP_PASSWD_FILE);

   /* delete user from group file */
   if ((fp = fopen(BEEP_GROUP_FILE, "r")) == NULL)
   {
      cmsLog_notice("Failed open %s for read.", BEEP_GROUP_FILE);
      return 0;
   }

   /* open a temporary file */
   sprintf(tmpfile, "%s--", BEEP_GROUP_FILE);

   if ((tmpfp = fopen(tmpfile, "w")) == NULL)
   {
      fclose(fp);
      cmsLog_error("Failed open %s for write", tmpfile);
      return -1;
   }

   while (fgets(line, sizeof(line), fp))
   {
      sscanf(line, "%[^:]:%*s", name);
      cmsLog_debug("name=%s", name);
      if (strcmp(name, username))
      {
         if (fputs(line, tmpfp) == EOF) goto err_out;
      }
   }

   fclose(fp);
   fclose(tmpfp);
   rename(tmpfile, BEEP_GROUP_FILE);

   return 0;

err_out:
   fclose(fp);
   fclose(tmpfp);
   remove(tmpfile);
   return -1;

}  /* End of mdm_deleteUser() */


static int mdm_deleteDMAccess(const char *euid)
{
   CmsRet ret = CMSRET_SUCCESS;
   DmAccessObject *obj = NULL;
   InstanceIdStack iidStack;
   MdmPathDescriptor pathDesc;

   INIT_INSTANCE_ID_STACK(&iidStack);
   /* walk through all the EEs and removed all the preinstalled ones */
   while (mdm_getNextObject(MDMOID_DM_ACCESS, &iidStack, (void **)&obj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcmp(euid, obj->EUID) == 0)
      {
         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid      = MDMOID_DM_ACCESS;
         pathDesc.iidStack = iidStack;

         if ((ret = mdm_deleteObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not delete DMA instance for euid=%s, ret=%d", euid, ret);
         }
         else
         {
            INIT_INSTANCE_ID_STACK(&iidStack);
         }
      }

      cmsObj_free((void **) &obj);
   }   

   return 0;

}  /* End of mdm_deleteDMAccess() */


/* this function goes through executionUnitList and do all the clean up */
static int mdm_cleanupEuRelated(char *executionUnitList, UINT32 *deletedEUinstance)
{
   CmsRet ret = CMSRET_SUCCESS;
   EUObject *euObj = NULL;
   MdmPathDescriptor pathDesc;    
   char *euFullPath;
   char conf[BEEP_USERNAME_LEN_MAX+BUFLEN_64]={0};

   cmsLog_debug("Enter: euList=%s deletedEUinstance=%d", executionUnitList, *deletedEUinstance);

   /* go through the list one EU at a time, get the EU object.
    * 1. delete the bus gate info.
    * 2. delete the EU instance from MDM
    */
   while (executionUnitList != NULL)
   {
      euFullPath = strsep(&executionUnitList, ",");
      if (IS_EMPTY_STRING(euFullPath))
         continue;

      cmsLog_debug("euFullPath=%d", euFullPath);

      INIT_PATH_DESCRIPTOR(&pathDesc);
      ret = cmsMdm_fullPathToPathDescriptor(euFullPath, &pathDesc);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("cmsMdm_fullPathToPathDescriptor failed <%s>, ret=%d", euFullPath, ret);
         continue;
      }

      ret= mdm_getObject(pathDesc.oid, &(pathDesc.iidStack), (void **)&euObj);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get euObject, ret=%d", ret);
         continue;
      }

      if (!IS_EMPTY_STRING(euObj->X_BROADCOM_COM_Username) &&
          strcmp(euObj->X_BROADCOM_COM_Username, BEEP_ADMIN_USER))
      {
         /* delete dbus security policy for username */
         sprintf(conf, "%s/%s.conf", BEEP_DBUS_POLICY_DIR, euObj->X_BROADCOM_COM_Username);
         cmsLog_debug("removing conf=%s", conf);
         remove(conf);

         /* delete username from passwd and group files. */
         mdm_deleteUser(euObj->X_BROADCOM_COM_Username);
      }

      /* delete all DMA Access privileges of this EU */
      mdm_deleteDMAccess(euObj->EUID);

      if ((ret = mdm_deleteObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not delete EU %s, ret=%d", euObj->name, ret);
      }
      else
      {
         *deletedEUinstance += 1;
      }

      cmsObj_free((void **)&euObj);
   }

   return 0;

}  /* End of mdm_cleanupEuRelated() */


static CmsRet mdm_delModSwPreinstalledEeObjects(UINT32 *eeDeletedCount, 
                                                UINT32 *duDeletedCount,
                                                UINT32 *euDeletedCount)
{
   CmsRet ret = CMSRET_SUCCESS;
   char *eeFullPath=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor pathDesc;
   ExecEnvObject *eeObj = NULL;
   InstanceIdStack duIidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor duPathDesc;
   DUObject *duObj = NULL;

   UINT32 deletedEEinstance = 0;
   UINT32 deletedDUinstance = 0;
   UINT32 deletedEUinstance = 0;
   
   /* walk through all the EEs and removed all the preinstalled ones */
   while ((ret = mdm_getNextObject(MDMOID_EXEC_ENV, &iidStack, (void **)&eeObj)) == CMSRET_SUCCESS)
   {
      if (eeObj->X_BROADCOM_COM_IsPreinstall == TRUE)
      {
         cmsLog_debug("Deleting %s Execution Environment", eeObj->name);

         /* this involves cleaning the bus gate info for all the EUs in this EE.
          * Cleaning DU and EU in MDM that are referenced to this EE.
          * Lastly, clean the EE info in MDM
          */
         
         INIT_PATH_DESCRIPTOR(&pathDesc);
         pathDesc.oid      = MDMOID_EXEC_ENV;
         pathDesc.iidStack = iidStack;

         /* get the DUs that is referencing to this EE */
         cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc,&eeFullPath);

         /* walk through all the EEs and removed all the preinstalled ones */
         while ((ret = mdm_getNextObject(MDMOID_DU, &duIidStack, (void **)&duObj)) == CMSRET_SUCCESS)
         {
            if (!cmsUtl_strcmp(duObj->executionEnvRef, eeFullPath))
            {
               /* 
                * loop through all the EUs in this duObj->executionUnitList, 
                * and clean the bus gate and password file
                */
               mdm_cleanupEuRelated(duObj->executionUnitList,
                                    &deletedEUinstance);

               INIT_PATH_DESCRIPTOR(&duPathDesc);
               duPathDesc.oid      = MDMOID_DU;
               duPathDesc.iidStack = duIidStack;
               if ((ret = mdm_deleteObjectInstance(&duPathDesc, NULL, NULL)) !=
                          CMSRET_SUCCESS)
               {
                  cmsLog_error("Could not delete DU %s, ret=%d", duObj->name,
                               ret);
               }
               else
               {
                  deletedDUinstance++;
                  INIT_INSTANCE_ID_STACK(&duIidStack);
               }
            }
         }

         /* now delete the EE */
         if ((ret = mdm_deleteObjectInstance(&pathDesc, NULL, NULL)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not delete %s Execution Environment, ret=%d", eeObj->name, ret);
         }
         else
         {
            deletedEEinstance++;
            INIT_INSTANCE_ID_STACK(&iidStack);
         }
         CMSMEM_FREE_BUF_AND_NULL_PTR(eeFullPath);
      } /* preinstalled EE */

      cmsObj_free((void **)&eeObj);
   }

   if (ret == CMSRET_NO_MORE_INSTANCES)
   {
      ret = CMSRET_SUCCESS;
   }

   /* return deleted instance count */
   if (deletedEEinstance > 0)
   {
      *eeDeletedCount = deletedEEinstance;
   }
   if (deletedDUinstance > 0)
   {
      *duDeletedCount = deletedDUinstance;
   }
   if (deletedEUinstance > 0)
   {
      *euDeletedCount = deletedEUinstance;
   }

   return ret;

}  /* End of mdm_delModSwEeObjects() */


static CmsRet mdm_delModSwPreinstalledDuObjects(UINT32 *duDeletedCount,
                                                UINT32 *euDeletedCount)
{
   CmsRet ret = CMSRET_SUCCESS;
   DUObject *duObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   MdmPathDescriptor duPathDesc;

   UINT32 deletedDUinstance = 0;
   UINT32 deletedEUinstance = 0;
   
   /* walk through all the DUs and removed all the preinstalled ones */
   while (mdm_getNextObject(MDMOID_DU, &iidStack, (void **)&duObj) ==
          CMSRET_SUCCESS)
   {
      if (duObj->X_BROADCOM_COM_IsPreinstall == TRUE)
      {
         cmsLog_debug("Deleting %s Deployment Unit", duObj->name);

         mdm_cleanupEuRelated(duObj->executionUnitList, &deletedEUinstance);

         INIT_PATH_DESCRIPTOR(&duPathDesc);
         duPathDesc.oid      = MDMOID_DU;
         duPathDesc.iidStack = iidStack;
         if ((ret = mdm_deleteObjectInstance(&duPathDesc, NULL, NULL)) !=
                    CMSRET_SUCCESS)
         {
            cmsLog_error("Could not delete DU %s, ret=%d", duObj->name, ret);
         }
         else
         {
            deletedDUinstance++;
            INIT_INSTANCE_ID_STACK(&iidStack);
         }
      } /* preinstalled DU */

      cmsObj_free((void **)&duObj);
   }

   /* return deleted instance count */
   if (deletedDUinstance > 0)
   {
      *duDeletedCount = deletedDUinstance;
   }
   if (deletedEUinstance > 0)
   {
      *euDeletedCount = deletedEUinstance;
   }

   return ret;
}


#else /* DMP_DEVICE2_SM_BASELINE_1 */


void mdm_removeBeepDatabase(void)
{
   /* it's NON-BEEP image --> remove BEEP database */
   if (cmsFil_isFilePresent(BEEP_DB_FILE))
   {
      unlink(BEEP_DB_FILE);
   }
}


#endif /* DMP_DEVICE2_SM_BASELINE_1 */
