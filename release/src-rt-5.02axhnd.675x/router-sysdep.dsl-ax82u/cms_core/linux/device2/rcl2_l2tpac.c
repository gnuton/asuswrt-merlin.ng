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

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1

#include "odl.h"
#include "cms_core.h"
#include "cms_util.h"
#include "rcl.h"
#include "rut_util.h"
#include "rut_wan.h"
#include "rut_wanlayer2.h"
#include "rut_l2tpac.h"

/*!\file rcl_l2tpac.c
 * \brief This file contains L2TPAC related functions.
 *
 */


CmsRet rcl_dev2PppInterfaceL2tpObject( _Dev2PppInterfaceL2tpObject *newObj __attribute__((unused)),
                const _Dev2PppInterfaceL2tpObject *currObj __attribute__((unused)),
                const InstanceIdStack *iidStack __attribute__((unused)),
                char **errorParam __attribute__((unused)),
                CmsRet *errorCode __attribute__((unused)))
{
   Dev2PppInterfaceObject *pppIntfObj = NULL;
   Dev2PppInterfaceL2tpObject *l2tpObj = NULL;           
   char l2tpAcUsername[BUFLEN_128]="username";
   char l2tpAcPassword[BUFLEN_128]="password"; 
   CmsRet ret = CMSRET_SUCCESS; 
   
   if ((ret = rut_validateObjects(newObj, currObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("rut_validateObjects returns error. ret=%d", ret);
      return ret;
   }
   
   /* enable L2tpAc connection */
   if (ENABLE_NEW_OR_ENABLE_EXISTING(newObj, currObj))
   {
      /* Get PPP interface object */
      if ((ret = cmsObj_get(MDMOID_DEV2_PPP_INTERFACE, iidStack, 0, (void **) &pppIntfObj)) != CMSRET_SUCCESS)
      {
          cmsLog_error("Failed to get pppIntfObj, ret = %d", ret);
          return ret;
      }
      else
      {
		  strcpy(l2tpAcUsername, pppIntfObj->username);
          strcpy(l2tpAcPassword, pppIntfObj->password);
          cmsObj_free((void **) &pppIntfObj);
		  if ((ret = rutL2tpAC_start_dev2(newObj, newObj->lnsIpAddress, l2tpAcUsername, l2tpAcPassword)) != CMSRET_SUCCESS)
          {
             cmsLog_error("L2tpAc initialzation failed, ret=%d", ret);
             return ret;
          }
	  }
   }
   else if (POTENTIAL_CHANGE_OF_EXISTING(newObj, currObj))
   {
      /* check for change in config params, if changed */
   }
   /* delete L2tpAc connection */
   else if (DELETE_OR_DISABLE_EXISTING(newObj, currObj))
   {
      rutL2tpAC_stop_dev2((Dev2PppInterfaceL2tpObject *) currObj);
   }

   
   return ret;
}


#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */
