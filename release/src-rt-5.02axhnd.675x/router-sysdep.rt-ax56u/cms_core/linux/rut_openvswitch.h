/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom 
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

 This program is the proprietary software of Broadcom  and/or its
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

#ifndef __RUT_OPENVSWITCH_H__
#define __RUT_OPENVSWITCH_H__


/*!\file rut_dnsproxy.h
 * \brief System level interface functions for dnsproxy functionality.
 *
 * The functions in this file should only be called by
 * RCL, STL, and other RUT functions.
 */

/** Check if openvswtich is enabled or not
 *
 * @return  TRUE or FALSE
 */
UBOOL8 rutOpenVS_isEnabled(void);

/** Disable or enable openvswtich.
 *
 * @return CmsRet enum.
 */
 
UBOOL8 rutOpenVS_isOpenVSPorts(const char *ifName);
CmsRet rutOpenVS_updateOpenvswitch(UBOOL8 change,const char *oldOpenVSports);
CmsRet rutOpenVS_startOpenvswitch(void);
CmsRet rutOpenVS_stopOpenvswitch(void);
CmsRet rutOpenVS_updateOpenvswitchOFController( const char *ofControllerAddr,UINT32 ofControllerPort);
CmsRet rutOpenVS_updateOpenvswitchPorts(const char *oldOpenVSports,const char *newOpenVSports);
void rutOpenVS_startupOpenVSport(const char *ifName);
void rutOpenVS_shutdownOpenVSport(const char *ifName);
CmsRet rutOpenVS_deleteOpenVSport(const char *ifName);
#if defined(SUPPORT_DM_LEGACY98) || defined(SUPPORT_DM_HYBRID) || defined(SUPPORT_DM_DETECT)
CmsRet rutOpenVS_deleteOpenVSport_igd(const char *ifName);
#endif
#if defined(SUPPORT_DM_PURE181) || defined(SUPPORT_DM_DETECT)
CmsRet rutOpenVS_deleteOpenVSport_dev2(const char *ifName);
#endif
#if defined(SUPPORT_DM_LEGACY98)
#define rutOpenVS_deleteOpenVSport(o)   rutOpenVS_deleteOpenVSport_igd(o)
#elif defined(SUPPORT_DM_HYBRID)
#define rutOpenVS_deleteOpenVSport(o)   rutOpenVS_deleteOpenVSport_igd(o)
#elif defined(SUPPORT_DM_PURE181)
#define rutOpenVS_deleteOpenVSport(o)   rutOpenVS_deleteOpenVSport_dev2(o)
#elif defined(SUPPORT_DM_DETECT)
#define rutOpenVS_deleteOpenVSport(o)    (cmsMdm_isDataModelDevice2() ? \
                                          rutOpenVS_deleteOpenVSport_dev2((o)) : \
                                          rutOpenVS_deleteOpenVSport_igd((o)))
#endif
CmsRet rutOpenVS_addOpenVSport(const char *ifName);
#if defined(SUPPORT_DM_LEGACY98) || defined(SUPPORT_DM_HYBRID) || defined(SUPPORT_DM_DETECT)
CmsRet rutOpenVS_addOpenVSport_igd(const char *ifName);
#endif
#if defined(SUPPORT_DM_PURE181) || defined(SUPPORT_DM_DETECT)
CmsRet rutOpenVS_addOpenVSport_dev2(const char *ifName);
#endif
#if defined(SUPPORT_DM_LEGACY98)
#define rutOpenVS_addOpenVSport(o)   rutOpenVS_addOpenVSport_igd(o)
#elif defined(SUPPORT_DM_HYBRID)
#define rutOpenVS_addOpenVSport(o)   rutOpenVS_addOpenVSport_igd(o)
#elif defined(SUPPORT_DM_PURE181)
#define rutOpenVS_addOpenVSport(o)   rutOpenVS_addOpenVSport_dev2(o)
#elif defined(SUPPORT_DM_DETECT)
#define rutOpenVS_addOpenVSport(o)    (cmsMdm_isDataModelDevice2() ? \
                                       rutOpenVS_addOpenVSport_dev2((o)) : \
                                       rutOpenVS_addOpenVSport_igd((o)))
#endif
#endif


