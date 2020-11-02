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

#ifdef DMP_DEVICE2_MEMORYSTATUS_1


#include <stdlib.h>
#include "cms.h"
#include "cms_util.h"


CmsRet rutSys_getMemoryStatus(UINT32 *total, UINT32 *free)
{
   CmsRet ret = CMSRET_OBJECT_NOT_FOUND;
   char line[BUFLEN_512];
   char *pChar = NULL;
   FILE *fs = NULL;

   *total = *free = 0;

   fs = fopen("/proc/meminfo", "r");
   if (fs == NULL)
   {
      cmsLog_error("Could not open /proc/meminfo");
      return ret;
   }

   while ( ret != CMSRET_SUCCESS &&
           fgets(line, sizeof(line), fs) )
   {
      // search for MemTotal
      if (strncmp(line, "MemTotal", 8) == 0 &&
          (pChar = strstr(line, ":")) != NULL &&
          (pChar + 1) != NULL)
      {
         // pChar+1: read pass ":"
         *total = (UINT32) strtoul(pChar+1, (char **)NULL, 10);
      }
      
      // search for MemFree
      if (strncmp(line, "MemFree", 7) == 0 &&
          (pChar = strstr(line, ":")) != NULL &&
          (pChar + 1) != NULL)
      {
         // pChar+1: read pass ":"
         *free = (UINT32) strtoul(pChar+1, (char **)NULL, 10);
         ret = CMSRET_SUCCESS;
      }
   } /* while */
         
   fclose(fs);

   cmsLog_debug("Before return: total=%u, free=%u",
                *total, *free);

   return ret;
}


#endif /* DMP_DEVICE2_MEMORYSTATUS_1 */

