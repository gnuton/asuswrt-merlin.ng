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

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut2_util.h"
#include "rut_wan.h"
#include "rut2_l2tpac.h"

CmsRet rut_getWanL2tpAcObject_dev2(InstanceIdStack *iidStack,
                               Dev2PppInterfaceL2tpObject **L2tpAcIntf)
{
   Dev2PppInterfaceObject *pppIntfObj = NULL;
   Dev2PppInterfaceL2tpObject *l2tpObj = NULL;      
   CmsRet ret = CMSRET_SUCCESS;     
   UBOOL8 found = FALSE;
      
   /* Look thru  all l interfaces and check for the lowerlayer matches with
   * the layer 2 interface (vlantermation here).  If found, get the pppoe object for
   * X_BROADCOM_COM_AddPppToBridge and other params.
   */
   while (!found &&
          (cmsObj_getNextFlags(MDMOID_DEV2_PPP_INTERFACE, iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&pppIntfObj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(pppIntfObj->name, "PPPoL2tpAc"))
      {
         InstanceIdStack l2tpIidStack = EMPTY_INSTANCE_ID_STACK;
               
         if ((ret = cmsObj_getNextInSubTree(MDMOID_DEV2_PPP_INTERFACE_L2TP, 
                                                iidStack, 
                                                &l2tpIidStack,
                                                (void **) &l2tpObj)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to get l2tpObj, error=%d", ret);
            cmsObj_free((void **)&pppIntfObj);
            return ret;
         }
            
         found = TRUE;
         
         if (L2tpAcIntf)
         {
             /* return obj to caller, don't free */
             *L2tpAcIntf = l2tpObj;
         }
      }
      else
      {
         cmsObj_free((void **)&pppIntfObj);
      }
   }

   if (!found)
   {
      cmsLog_debug("Failed to find the matching pppIntfObj");
      return CMSRET_INTERNAL_ERROR;
   }
  
   cmsObj_free((void **)&pppIntfObj); 
      
   return ret;
}

CmsRet rutL2tpAC_start_dev2(Dev2PppInterfaceL2tpObject *newObj, const char *server, const char *userid, const char *password)
{
   UINT32 pid = 0;
   CmsRet ret = CMSRET_SUCCESS;
   char l2tpcmd[BUFLEN_128];
    /* start l2tpac connection */
   snprintf(l2tpcmd, sizeof(l2tpcmd), "-r %s -U %s -P %s -f -c ppp0", server, userid, password);
   
   cmsLog_debug("l2tpcmd is %s", l2tpcmd);
   /* send message to ssk to launch l2tpd */
   if ((pid = rut_sendMsgToSmd(CMS_MSG_START_APP, EID_L2TPD, l2tpcmd, strlen(l2tpcmd)+1)) == CMS_INVALID_PID)
   {
      cmsLog_error("failed to start or restart dproxy");
      ret = CMSRET_INTERNAL_ERROR;
   }
   
   return ret;
}

CmsRet rutL2tpAC_stop_dev2(Dev2PppInterfaceL2tpObject *currObj __attribute__((unused)))
{
   CmsRet ret = CMSRET_SUCCESS;
   FILE* fs = NULL;
   char cmd[BUFLEN_128];
   char buf[BUFLEN_128];
   
   cmsLog_debug("Entered");

   if ((fs = fopen(PPPoL2TPAC_PID_FILENAME, "r")) == NULL)
   {
        cmsLog_error("Failed to get %s\n", PPPoL2TPAC_PID_FILENAME);
   }
   else
   {
        if (fread(buf, 1, sizeof(buf), fs) > 0)
        {
            snprintf(cmd, sizeof(cmd), "kill %d", atoi(buf));
            rut_doSystemAction("DEL PPPoL2tpAc", cmd);
        }
        fclose(fs);
        unlink(PPPoL2TPAC_PID_FILENAME);  
   }
   		   
   sleep(3);
     
   if ((rut_sendMsgToSmd(CMS_MSG_STOP_APP, EID_L2TPD, NULL,0)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to stop l2tpd.");
      return CMSRET_INTERNAL_ERROR;
   }         
   return ret;
}

#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */
