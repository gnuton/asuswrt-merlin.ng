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

#ifdef DMP_DEVICE2_PROCESSSTATUS_1


#include <stdlib.h>
#include "cms.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"

#define PS_TMP_FILE "/var/tmp_ps"

UINT32 rutSys_getNumberOfProcesses(void)
{
   UINT32 count = 0, pid = 0, vsz = 0;
   char cmd[BUFLEN_512], line[BUFLEN_512];
   char user[BUFLEN_32], stat[BUFLEN_32], command[BUFLEN_64];
   FILE *fs = NULL;

   sprintf(cmd, "ps > %s", PS_TMP_FILE);
   rut_doSystemAction("processes", cmd);

   fs = fopen(PS_TMP_FILE, "r");
   if ( fs == NULL )
   {
      cmsLog_error("Could not open %s, return 0", PS_TMP_FILE);
      return 0;
   }

   while ( fgets(line, sizeof(line), fs) )
   {
      sscanf(line, "%d %s %d %s %s", &pid, user, &vsz, stat, command);
      if (vsz != 0)
      {
         count++;
      } /* while */
   } /* while */

   fclose(fs);

   cmsLog_error("Before return, count = %d", count);

   sprintf(cmd, "rm %s", PS_TMP_FILE);
   rut_doSystemAction("processes", cmd);

   return count;
}


CmsRet rutSys_deleteProcessStatusTable(const InstanceIdStack *iidProcess)
{
    CmsRet ret = CMSRET_SUCCESS;
    InstanceIdStack iid = EMPTY_INSTANCE_ID_STACK;
    Dev2ProcessStatusEntryObject *processEntry = NULL;

    // clear all entries in the table
    while ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PROCESS_STATUS_ENTRY,
                                   iidProcess, &iid,
                                   (void **) &processEntry)) == CMSRET_SUCCESS)
    {
        ret = cmsObj_deleteInstance(MDMOID_DEV2_PROCESS_STATUS_ENTRY, &iid);

        if (ret  != CMSRET_SUCCESS) 
        { 
            cmsLog_error("Failed to delete processEntry");
        } 
        else 
        { 
            INIT_INSTANCE_ID_STACK(&iid);
        }
        cmsObj_free((void **)&processEntry);
    }

    return ret;
}


CmsRet rutSys_updateProcessStatusTable
    (const InstanceIdStack *iidProcess,
     UINT32 *numberOfEntries)
{
    UINT32 count = 0, pid = 0, vsz = 0;
    char cmd[BUFLEN_512], line[BUFLEN_512];
    char user[BUFLEN_32], stat[BUFLEN_32], command[BUFLEN_64];
    FILE *fs = NULL;
    CmsRet ret = CMSRET_SUCCESS;
    InstanceIdStack iid = EMPTY_INSTANCE_ID_STACK;
    Dev2ProcessStatusEntryObject *processEntry = NULL;

    sprintf(cmd, "ps > %s", PS_TMP_FILE);
    rut_doSystemAction("processes", cmd);

    fs = fopen(PS_TMP_FILE, "r");
    if ( fs == NULL )
    {
        cmsLog_error("Could not open %s, return 0", PS_TMP_FILE);
        return CMSRET_INTERNAL_ERROR;
    }

    // clear all entries in the process status table
    ret = rutSys_deleteProcessStatusTable(iidProcess);
    if (ret != CMSRET_SUCCESS && ret != CMSRET_NO_MORE_INSTANCES)
    {
        cmsLog_error("Could not delete process status table, ret=%d", ret);
        goto out;
    }

    // reset number of entries after deleting table
    *numberOfEntries = 0;

    while ( fgets(line, sizeof(line), fs) )
    {
        sscanf(line, "%d %s %d %s %s", &pid, user, &vsz, stat, command);
        if (vsz != 0)
        {
            // keep parent iid to add more entry
            memcpy(&iid, iidProcess, sizeof(InstanceIdStack));
            PUSH_INSTANCE_ID(&iid, ++count);
            // add entry to the table
            ret = cmsObj_addInstance(MDMOID_DEV2_PROCESS_STATUS_ENTRY, &iid);
            if (ret == CMSRET_SUCCESS)
            {
                ret = cmsObj_get(MDMOID_DEV2_PROCESS_STATUS_ENTRY, &iid, 0, (void **) &processEntry);
                if (ret == CMSRET_SUCCESS)
                {
                    processEntry->PID = pid;
                    processEntry->size = vsz;
                    CMSMEM_REPLACE_STRING_FLAGS(processEntry->command, command, mdmLibCtx.allocFlags);
                    switch (stat[0])
                    {
                        case 'D':
                            CMSMEM_REPLACE_STRING_FLAGS(processEntry->state, MDMVS_UNINTERRUPTIBLE, mdmLibCtx.allocFlags);
                            break;
                        case 'S':
                            CMSMEM_REPLACE_STRING_FLAGS(processEntry->state, MDMVS_SLEEPING, mdmLibCtx.allocFlags);
                            break;
                        case 'T':
                            CMSMEM_REPLACE_STRING_FLAGS(processEntry->state, MDMVS_STOPPED, mdmLibCtx.allocFlags);
                            break;
                        case 'Z':
                            CMSMEM_REPLACE_STRING_FLAGS(processEntry->state, MDMVS_ZOMBIE, mdmLibCtx.allocFlags);
                            break;
                        case 'R':
                        default:
                            CMSMEM_REPLACE_STRING_FLAGS(processEntry->state, MDMVS_RUNNING, mdmLibCtx.allocFlags);
                            break;
                    }

                    // update entry information
                    ret = cmsObj_set(processEntry, &iid);

                    // free entry that is already gotten
                    cmsObj_free((void **)&processEntry);

                    if (ret != CMSRET_SUCCESS)
                    {
                        // delete entry if set is failed
                        cmsObj_deleteInstance(MDMOID_DEV2_PROCESS_STATUS_ENTRY, &iid);
                        count--;
                        cmsLog_error("Could not set Dev2ProcessStatusEntryObject, ret=%d", ret);
                        goto out;
                    }
                }
                else
                {
                    cmsLog_error("Could not get Dev2ProcessStatusEntryObject, ret=%d", ret);
                    cmsObj_deleteInstance(MDMOID_DEV2_PROCESS_STATUS_ENTRY, &iid);
                    count--;
                    goto out;
                }
            }
            else
            {
                cmsLog_error("Could not add Dev2ProcessStatusEntryObject, ret=%d", ret);
                count--;
                goto out;
            }
        }
    } /* while */

out:

    fclose(fs);

    unlink(PS_TMP_FILE);

    // return number of entries if success
    if (ret == CMSRET_SUCCESS)
    {
        *numberOfEntries = count;
    }

    return ret;
}


#endif /* DMP_DEVICE2_PROCESSSTATUS_1 */

