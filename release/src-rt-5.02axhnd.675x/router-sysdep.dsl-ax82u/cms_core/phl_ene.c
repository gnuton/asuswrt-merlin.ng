/***********************************************************************
 *
 *  Copyright (c) 2006-2018  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

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


/*
 * phl_ene.c
 *
 *  Created on:  Sep.16, 2018
 *  Author: Fuguo Xu <fuguo.xu@broadcom.com>
 */


/*
 * this file is an extension of "phl.c", to implement special features(i.e. nodes hidden  
 * from specified ACS) for MULTIPLE_TR69C_SUPPORT case.
 */


#include "cms.h"
#include "cms_log.h"
#include "cms_util.h"
#include "mdm.h"
#include "qdm_intf.h"





/*  By default, only objects hiding is enabled.
 *  If you really need to support hiding parameters from each other ACS, you can change this Macro to 1.
 *  But, this is not recommended. Because, it means that the routine also needs to walk through Blacklist 
 *  for thousands of parameter nodes. This may reduce efficiency of ACS retrieving entire datamodel 
 *  tree, especially when the Blacklist table is deeper.
 */
#define MULTIPLE_TR69C_HIDE_PARAMETERS_SUPPORT    0




/* define the data type for HiddenFromAcsBlacklist. Each tr69c has a separate BL table.
 *  there are 2 usages here:
 * 1. If only fill in an OID without callback(isHiddenFn==NULL), then all ojbects and sub-trees
 *     (regardless they are static/dynamic) with same OID will be hidden.
 * 2. If fill in both OID and callback(isHiddenFn != NULL), then it is the callback to determine
 *     whether to hide or not. the callback can test everything, like, specified parameters, dynamic 
 *     object instance, etc.
 */
typedef struct
{
   MdmObjectId       oid;   /* the OID to be hidden */
   UBOOL8 (*isHiddenFn)(const MdmPathDescriptor *pathDesc); /* the callback to further determine if needs to be hidden */
} eneHiddenFromAcsBlacklist;




#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)

#if defined(BRCM_PKTCBL_SUPPORT)

/* hidden from ACS Blacklist callback */


/* get hidden Wan IfName
 *  hide EMTA Wan(veip0.1) from ACS_2,
 *  hide EPTA Wan(veip0.2) from ACS_1 
 */
static char* pktcbl_getHiddenWanIfName(void)
{
   if (EID_TR69C == mdmLibCtx.eid)
   {
      return EPON_EPTA_WAN_IF_NAME;
   }
   else if (EID_TR69C_2 == mdmLibCtx.eid)
   {
      return EPON_VOICE_WAN_IF_NAME;
   }

   return NULL;
}

/* Device.IP.Interface.{i}.
 *  hide EMTA Wan(veip0.1) from ACS_2,
 *  hide EPTA Wan(veip0.2) from ACS_1 
 */
static UBOOL8 pktcbl_isIpInterfaceHidden(const MdmPathDescriptor *pathDesc)
{
   char *IfName_hidden = NULL;
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   UBOOL8 isHidden = FALSE;

   IfName_hidden = pktcbl_getHiddenWanIfName();
   if (NULL == IfName_hidden)
   {
      return isHidden;
   }

   if ((mdm_getObject(pathDesc->oid, &pathDesc->iidStack, (void **) &ipIntfObj)) != CMSRET_SUCCESS)
   {
      return isHidden;
   }

   if (!cmsUtl_strcmp(ipIntfObj->name, IfName_hidden))
   {
      cmsLog_notice("hide IP.Interface.{%d}.", PEEK_INSTANCE_ID(&pathDesc->iidStack));
      isHidden = TRUE;
   }

   mdm_freeObject((void **)&ipIntfObj);

   return isHidden;
}


/* Device.DHCPv4.Client.{i}.
 *  hide DHCPv4 Client associated with EMTA Wan(veip0.1) from ACS_2,
 *  hide DHCPv4 Client associated with EPTA Wan(veip0.2) from ACS_1
 */
static UBOOL8 pktcbl_isDhcpv4ClientHidden(const MdmPathDescriptor *pathDesc)
{
   char *IfName_hidden = NULL;
   Dev2Dhcpv4ClientObject *dhcp4cObj = NULL;
   UBOOL8 isHidden = FALSE;
   char intfNameBuf[CMS_IFNAME_LENGTH] = {0};

   IfName_hidden = pktcbl_getHiddenWanIfName();
   if (NULL == IfName_hidden)
   {
      return isHidden;
   }

   if ((mdm_getObject(pathDesc->oid, &pathDesc->iidStack, (void **) &dhcp4cObj)) != CMSRET_SUCCESS)
   {
      return isHidden;
   }

   qdmIntf_fullPathToIntfnameLocked(dhcp4cObj->interface, intfNameBuf);   
   if (!cmsUtl_strcmp(intfNameBuf, IfName_hidden))
   {
      cmsLog_notice("hide DHCPv4.Client.{%d}.", PEEK_INSTANCE_ID(&pathDesc->iidStack));
      isHidden = TRUE;
   }

   mdm_freeObject((void **)&dhcp4cObj);

   return isHidden;
}


/* Device.DHCPv6.Client.{i}.
 *  hide DHCPv6 Client associated with EMTA Wan(veip0.1) from ACS_2,
 *  hide DHCPv6 Client associated with EPTA Wan(veip0.2) from ACS_1
 */
static UBOOL8 pktcbl_isDhcpv6ClientHidden(const MdmPathDescriptor *pathDesc)
{
   char *IfName_hidden = NULL;
   Dev2Dhcpv6ClientObject *dhcp6cObj = NULL;
   UBOOL8 isHidden = FALSE;
   char intfNameBuf[CMS_IFNAME_LENGTH] = {0};

   IfName_hidden = pktcbl_getHiddenWanIfName();
   if (NULL == IfName_hidden)
   {
      return isHidden;
   }

   if ((mdm_getObject(pathDesc->oid, &pathDesc->iidStack, (void **) &dhcp6cObj)) != CMSRET_SUCCESS)
   {
      return isHidden;
   }

   qdmIntf_fullPathToIntfnameLocked(dhcp6cObj->interface, intfNameBuf);   
   if (!cmsUtl_strcmp(intfNameBuf, IfName_hidden))
   {
      cmsLog_notice("hide DHCPv6.Client.{%d}.", PEEK_INSTANCE_ID(&pathDesc->iidStack));
      isHidden = TRUE;
   }

   mdm_freeObject((void **)&dhcp6cObj);

   return isHidden;
}



/* hidden from ACS Blacklist tables.
 *  for more details, pls refer to the description in typedef of eneHiddenFromAcsBlacklist.
 *
 *  Note: in most of case, you just need to expand these BL tables according to customer requirement.
 */

/* BL for tr69c_1 */
const static eneHiddenFromAcsBlacklist e1e_hiddenBL[] = 
{
   {MDMOID_DEV2_IP_INTERFACE,                 pktcbl_isIpInterfaceHidden},
   {MDMOID_DEV2_DHCPV4_CLIENT,                pktcbl_isDhcpv4ClientHidden},
   {MDMOID_DEV2_DHCPV6_CLIENT,                pktcbl_isDhcpv6ClientHidden},

   {INVALIDE_MDM_MAX_OID,                     NULL}  /* NOTE!! ensure this is the last one! */ 
};

/* BL for tr69c_2 */
const static eneHiddenFromAcsBlacklist e2e_hiddenBL[] = 
{
   {MDMOID_DEV2_IP_INTERFACE,                 pktcbl_isIpInterfaceHidden},
   {MDMOID_DEV2_DHCPV4_CLIENT,                pktcbl_isDhcpv4ClientHidden},
   {MDMOID_DEV2_DHCPV6_CLIENT,                pktcbl_isDhcpv6ClientHidden},

   {INVALIDE_MDM_MAX_OID,                     NULL}  /* NOTE!! ensure this is the last one! */ 
};

#else // defined(BRCM_PKTCBL_SUPPORT)

/* stub BL for tr69c_1 */
const static eneHiddenFromAcsBlacklist e1e_hiddenBL[] = 
{
   //{MDMOID_DEV2_IP_INTERFACE,                 NULL},

   {INVALIDE_MDM_MAX_OID,                     NULL}  /* NOTE!! ensure this is the last one! */ 
};

/* stub BL for tr69c_2 */
const static eneHiddenFromAcsBlacklist e2e_hiddenBL[] = 
{
   //{MDMOID_DEV2_IP_INTERFACE,                 NULL},

   {INVALIDE_MDM_MAX_OID,                     NULL}  /* NOTE!! ensure this is the last one! */ 
};
#endif // defined(BRCM_PKTCBL_SUPPORT)




/* general function for MULTIPLE_TR69C_SUPPORT, pls find more description in phl_ene.h */
CmsRet ene_getPathDescHiddenFromAcs(const MdmPathDescriptor *pathDesc,
                                              UBOOL8 *hidden)
{
   MdmObjectNode *objNode;
   CmsRet ret=CMSRET_SUCCESS;
   const eneHiddenFromAcsBlacklist *hiddenBL = NULL;
   int i = 0;

   if ((pathDesc == NULL) ||
       ((objNode = mdm_getObjectNode(pathDesc->oid)) == NULL) ||
       (hidden == NULL))
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

#if (MULTIPLE_TR69C_HIDE_PARAMETERS_SUPPORT != 1)
   if (IS_PARAM_NAME_PRESENT(pathDesc))
   {
      /* don't check parameters, so do nothing and return */
      return CMSRET_SUCCESS;
   }
#endif


   if (EID_TR69C == mdmLibCtx.eid)
   {
      hiddenBL = e1e_hiddenBL;
   }
   else if (EID_TR69C_2 == mdmLibCtx.eid)
   {
      hiddenBL = e2e_hiddenBL;
   }
   else
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   while(INVALIDE_MDM_MAX_OID != hiddenBL[i].oid)
   {
      if (pathDesc->oid == hiddenBL[i].oid)
      {
         if (NULL == hiddenBL[i].isHiddenFn) 
         {
            /* hidden all, pls refer to the description in typedef of eneHiddenFromAcsBlacklist */
            *hidden = TRUE;
         }
         else
         {
            /* hidden the specified, pls refer to the description in typedef of eneHiddenFromAcsBlacklist */
            *hidden = hiddenBL[i].isHiddenFn(pathDesc);
         }

         break;
      }
      i++;
   }

   return ret;
}



#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)


